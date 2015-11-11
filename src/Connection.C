// -*- C++ -*-  $Id: Connection.C,v 2.369 2015/11/09 11:35:15 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <PicSOM.h>
#include <Connection.h>
#include <Query.h>
#include <Analysis.h>
#include <RwLock.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif // HAVE_SYS_IOCTL_H

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif // HAVE_FCNTL_H

#ifdef HAVE_POLL_H
#include <poll.h>
#endif // HAVE_POLL_H

#include <sys/types.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif // HAVE_NETDB_H

#include <netinet/tcp.h>

#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
extern "C" {
void WSAAPI freeaddrinfo (struct addrinfo*);
int WSAAPI getaddrinfo (const char*,const char*,const struct addrinfo*,
		        struct addrinfo**);
int WSAAPI getnameinfo(const struct sockaddr*,socklen_t,char*,DWORD,
		       char*,DWORD,int);
}
#endif // HAVE_WS2TCPIP_H

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#if PICSOM_USE_CSOAP
#include <libcsoap/soap-xml.h>
#include <nanohttp/nanohttp-server.h>
#endif // PICSOM_USE_CSOAP

#include <base64.h>

#define XML_VERSION 0.50
#define MRML_VERSION 1.0
#define REQUIRED_CONNECTION_VERSION 1088

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string Connection_C_vcid =
    "@(#)$Id: Connection.C,v 2.369 2015/11/09 11:35:15 jorma Exp $";

  /// A dummy initializer.
  const list<pair<string,string> > Connection::empty_list_pair_string;

#ifdef HAVE_OPENSSL_SSL_H
  SSL_CTX *Connection::ssl_ctx = NULL;
#endif // HAVE_OPENSSL_SSL_H

#ifdef PICSOM_USE_CSOAP
  ///
  Connection *Connection::soap_server_instance = NULL;

  ///
  bool Connection::soap_server_single = false;
#endif // PICSOM_USE_CSOAP

  ///
  static const string http_vers = "HTTP/1.1";

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  int  Connection::debug_selects = 0;
  int  Connection::debug_proxy   = 0;
  int  Connection::debug_http    = 0;
  int  Connection::debug_soap    = 0;
  int  Connection::debug_writes  = 0;
  bool Connection::debug_reads   = false;
  bool Connection::debug_sockets = false;
  bool Connection::debug_locks   = false;
  bool Connection::debug_expire  = false;
  bool Connection::debug_objreqs = false;
  bool Connection::debug_bounces = false;

  string Connection::timeout;
  string Connection::default_xsl = "picsom";

  map<string,pair<string,map<string,string>>> Connection::restca_task_stat;
  bool Connection::restca_mysql = false;

  map<string,Connection::proxy_data_t> Connection::proxy_data_map;
  
  /////////////////////////////////////////////////////////////////////////////
  
  Connection::Connection(PicSOM *p) : client(NULL), servant(NULL) {
    pic_som = p;
    type = conn_none;
    port = 0;
    wfd = rfd = -1;
    rfile = NULL;
    istr  = NULL;
    child = -1;
    interactive = can_do_mpi = false;
    pm_version = xml_version = mrml_version = 0;
    queries = 0;

    msgtype = msg_undef;
    query = NULL;
    query_own = true;
    state = idle;

    notify_delete = true;
    http_analysis = NULL;

    SetAccessTime();
    first = last;
  
    obj_type = ot_no_object;

    protocol = unspecified_language;
    xmldoc__ = NULL;

    is_active = is_failed = false;
    can_possibly_be_selected = true;

    has_own_thread = false;

#ifdef HAVE_OPENSSL_SSL_H
    ssl = NULL;
#endif // HAVE_OPENSSL_SSL_H

#ifdef PICSOM_USE_MPI
    mpi_root = 0;
    mpi_msgtag = 0;
#endif // PICSOM_USE_MPI
  }

  /////////////////////////////////////////////////////////////////////////////

  Connection::~Connection() { 
    if (!(type&conn_closed))
      Close(true, notify_delete);

    DeleteQuery();
    DeleteXMLdoc();
  }

  /////////////////////////////////////////////////////////////////////////////

  void Connection::Dump(ostream& os) const { 
    os << Simple::Bold("Connection ") << (void*)this
       << " type="                    << TypeStringStr(true)
       << " state="                   << (int)state
       << " is_active="               << is_active
       << " is_failed="               << is_failed
       << " port="                    << port
       << " rfd="                     << rfd
       << " wfd="                     << wfd
       << " rfile="                   << (void*)rfile
       << " istr="                    << (void*)istr
       << " child="                   << child
       << " msgtype="                 << MsgTypeP(msgtype)
       << " query="                   << (void*)query
       << " query_own="               << query_own
       << " identity_str="            << Simple::ShowStringStr(identity_str)
       << " xml_version="             << xml_version
       << " pm_version="              << pm_version
       << " interactive="             << interactive
       << " queries="                 << queries
       << " obj_type="                << obj_type
       << " obj_name="                << Simple::ShowStringStr(obj_name)
       << " obj_spec="                << Simple::ShowStringStr(obj_spec)
       << " servant="                 << servant.Address()
       << " client="                  << client.Address()
       << " bounce="                  << (int)bounce
       << " pthread="                 << OwnThreadIdentity()
       << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Connection::DumpRequestInfo(ostream& os) const {
    os << "*"
       << " /" << OwnThreadIdentity()
       << " comm.type="  << MsgTypeP(msgtype)
       << " obj.type="   << ObjectTypeP(obj_type)
       << " obj.name="   << obj_name
       << " obj.spec="   << obj_spec
       << " q.ident="    << (query?query->Identity():"NULL")
       << " client="     << ClientAddress()
       << " *" << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::CanBeSelected() const {
    if (!can_possibly_be_selected)
      return false;

    if (HasOwnThread())
      return false;

    if (type==conn_mpi_down || type==conn_mpi_up)
      return true;

    if (Rfd()<0)
      return false;

    if (type==conn_pipe && !State(0))
      return false; // added 2010-06-15 for PicSOM::speech_recognizer

    if (IsFailed() && Picsom()->IsSlave())
      return false; // added 2013-02-08 to support slave processes...

    bool listen = type!=conn_none;
    if (type==conn_up && !Picsom()->IsSlave())
      listen = false;

    return listen && type<conn_closed;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Connection::InitializeSome(bool is_old) {
    if (!is_old) {
      // these were commented out 2015-04-08
      // if (query)
      //   ShowError("Connection::InitializeSome() query!=NULL");
      // DeleteQuery();

      ObjectRequest(ot_no_object, "", "");
      msgtype = msg_undef;
    }

    ClientAddress("");
    ClientCookie("");

    DeleteXMLdoc();
    bounce = false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::Interpret(const string& key, const string& val,
			     int& res) {
    res = 1;

    /*>> connection_interpret
      client
      *DOCUMENTATION MISSING* */
    
    if (key=="client") {           // this includes "cookie"
      size_t c = val.find(':');
      if (c!=string::npos) {
	ClientCookie( val.substr(c+1));
	ClientAddress(val.substr(0, c));
      } else {
	ClientCookie("");
	ClientAddress(val);
      }
      return true;
    }

    object_type object = ObjectType(key);
    if (object>0) {
      if (!query) {
	ShowError("Connection::Interpret() query==NULL");
	res = 0;
	return true;
      }

      string valtmp = val;
      StripWhiteSpaces(valtmp);
      if (valtmp=="") {
	ShowError("Connection::Interpret() object_type=="+key, " val empty");
	res = 0;
	return true;
      }

      msgtype = msg_objectrequest; // query->SetObjectRequest();
      ObjectRequest(object, valtmp, "");
      // res = -1; OBS!
      return true;
    }

    if (key=="objectspec") {
      ObjectSpec(val);
      return true;
    }

    if (key=="interactive") {
      Interactive(IsAffirmative(val));
      return true;
    }

    return false;
  }

///////////////////////////////////////////////////////////////////////////////

Connection *Connection::CreateTerminal(PicSOM *p) {
  Connection *c = new Connection(p);
  c->type = conn_terminal;
  c->rfd  = STDIN_FILENO;
  c->wfd  = STDOUT_FILENO;
  c->interactive = true;

  const char *uname = p->UserName().c_str();
  c->Identity(uname&&*uname?uname:"????");
  c->Notify(true, NULL, true);

  return c;
}

///////////////////////////////////////////////////////////////////////////////

Connection *Connection::CreateFile(PicSOM *p, const char *n) {
  Connection *c = new Connection(p);
  c->type  = conn_file;
  c->rfd   = open(n, O_RDONLY, 0664);
  c->rfile = fdopen(c->rfd, "r");

  c->Identity(n);

  if (c->rfd<0) {
    c->Close();
    c->Notify(true, "FAILED: file could not be read", true);

  } else
    c->Notify(true, NULL, true);

  return c;
}

///////////////////////////////////////////////////////////////////////////////

Connection *Connection::CreateStream(PicSOM *p, istream& s) {
  Connection *c = new Connection(p);
  c->type  = conn_stream;
  c->istr  = &s;
  c->Identity("*stream*");
  c->Notify(true, NULL, true);

  return c;
}

///////////////////////////////////////////////////////////////////////////////

Connection *Connection::CreatePipe(PicSOM *p, const vector<string>& cmd,
                                   bool errnull, bool notify) {
#ifdef HAVE_PIPE
  int out[2], in[2];
  if (pipe(out))
    return NULL;
  if (pipe(in)) {
    close(out[0]);
    close(out[1]);
    return NULL;
  }

  Connection *c = new Connection(p);
  c->type  = conn_pipe;
  c->Identity("*"+ToStr(cmd)+"*");

  sync();

  c->child = fork();
  if (c->child==-1) {
    p->ShowError("Connection::CreatePipe() : fork() failed with \""
		 +string(strerror(errno))+"\"");
    close(out[0]);
    close(out[1]);
    close(in[0]);
    close(in[1]);
    delete c;
    return NULL;
  }
  
  if (c->child) {
    c->wfd = out[1];
    c->rfd = in[0];
    if (notify)
      c->Notify(true, NULL, true);
    c->notify_delete = notify;
    return c;
  }

  dup2(out[0], 0);
  dup2(in[1],  1);

  if (errnull) {
    int errfd = open("/dev/null", O_APPEND);
    if (errfd!=-1)
      dup2(errfd, 2);
  }

  char **a = new char*[cmd.size()+1];
  a[cmd.size()] = NULL;
  for (size_t i=0; i<cmd.size(); i++)
    a[i] = (char*)cmd[i].c_str();
  
  execv(a[0], a);
#else
  p->ShowError("Warning: pipe() not implemented for <"+ToStr(cmd));
#endif // HAVE_PIPE

  return NULL;
}

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_MPI
  void Connection::ConvertToMPIListen() {
    MPI::Info info;
    char port_name[MPI_MAX_PORT_NAME];
    MPI::Open_port(info, port_name);
    type = conn_mpi_listen;
    mpi_port = port_name;
    mpi_intra = MPI_COMM_WORLD;
    Identity(mpi_port);
  }
#endif // PICSOM_USE_MPI

  /////////////////////////////////////////////////////////////////////////////

  Connection *Connection::CreateMPIListen(PicSOM *p) {
#ifdef PICSOM_USE_MPI
    Connection *c = new Connection(p);
    c->ConvertToMPIListen();
    c->Notify(true, NULL, true);

    return c;
#else
    ShowError("Connection::CreateMPIListen() : No MPI support available"
	      " p="+ToStr((void*)p));
    return NULL;
#endif // PICSOM_USE_MPI
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_MPI
  void Connection::ConvertToMPI(const string& paddr, bool do_conn) {
    type = conn_mpi_up;
    mpi_port = paddr;
    mpi_intra = MPI_COMM_WORLD;
    if (do_conn)
      mpi_inter = mpi_intra.Connect(paddr.c_str(), mpi_info, mpi_root);
    Identity((do_conn?"CONNECT:":"ACCEPT:")+paddr);
    Notify(true, NULL, true);
  }
#endif // PICSOM_USE_MPI

  /////////////////////////////////////////////////////////////////////////////

  Connection *Connection::CreateMPI(PicSOM *p, const string& paddr,
				    bool do_conn) {
#ifdef PICSOM_USE_MPI
    Connection *c = new Connection(p);
    c->ConvertToMPI(paddr, do_conn);
    c->Notify(true, NULL, true);

    return c;
#else
    ShowError("Connection::CreateMPIListen() : No MPI support available "
	      "p="+ToStr((void*)p)+" "+paddr+" "+string(do_conn?"yes":"no"));
    return NULL;
#endif // PICSOM_USE_MPI
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_MPI
  void Connection::MPIaccept() {
    mpi_inter = mpi_intra.Accept(mpi_port.c_str(), mpi_info, mpi_root);
    type = conn_mpi_down;
  }
#endif // PICSOM_USE_MPI

///////////////////////////////////////////////////////////////////////////////

Connection *Connection::CreateListen(PicSOM *p, int porta, int portb) {
#ifdef HAVE_WINSOCK2_H
  typedef u_short in_port_t;
#endif // HAVE_WINSOCK2_H

  struct sockaddr_in name;
  int lp, s = MAXINT;
  for (lp=porta; lp<=portb; lp++) {
    name.sin_family      = AF_INET;
    name.sin_port        = htons((in_port_t)lp);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    
    s = ::socket(AF_INET, SOCK_STREAM, 0);
#ifdef HAVE_WINSOCK2_H
    if (s==INVALID_SOCKET) {
      int err = WSAGetLastError();
      ShowError("winsock2 socket() returned "+ToStr(err));
      exit(1);
    }
#else
    if (s==-1) {
      perror("socket");
      exit(1);
    }
#endif // HAVE_WINSOCK2_H

    int e = ::bind(s, (sockaddr*)&name, sizeof name);
    if (e) {
      s = MAXINT;
      continue;
    }
  
    e = ::listen(s, 10);
    if (e) {
      perror("listen");
      exit(1);
    }

    //  cout << endl << ">>>>>>>>>>>>> listening port " << lp << endl;

    break;
  }
  if (s==MAXINT) {
    ShowError("Failed to bind() socket in Connection::CreateListen(): ",
	      strerror(errno));
    exit(1);
  }

  Connection *c = new Connection(p);
  c->type = conn_listen;
  c->port = lp;
  c->wfd  = c->rfd = s;
  c->servant = RemoteHost(NULL, name);
  c->Identity("talking to me?");
  c->Notify(true, NULL, true);

  p->DoSshForwarding(lp);

  return c;
}

  /////////////////////////////////////////////////////////////////////////////

  Connection *Connection::CreateSocket(PicSOM *p, int s, const RemoteHost& rh) {
    string hdr = "Connection::CreateSocket() : ";

    Connection *c = new Connection(p);
    c->type = conn_socket;  // this should be very short-lasting type...
    c->rfd  = c->wfd  = s;
    c->servant = rh;
    c->hostname = rh.Address();

    bool http = false;

    bool read_ok = c->ReadAndParseXML(http, true);
    if (!read_ok) {
      ShowError(hdr+"Connection::ReadAndParseXML() failed");
      c->is_failed = true;
      return c;
    }

    if (c->IsFailed()) {
      if (debug_http>3)
	cout << hdr << "falling thru because IsFailed()" << endl;
      return c;
    }

    if (http) {
      c->type = conn_http_server;  // should these two
      c->protocol = xml_language;  // be unified?

      if (!c->HttpServerProcess(true))
	ShowError(hdr+"HttpServerProcess(true) failed");

      return c;
    }

    if (!c->HasXMLdoc()) {
      c->Notify(true, "non-XML input socket ???", true);
      return c;
    }

    c->type = conn_down;
    c->protocol = xml_language; // or mrml_language
    c->ExtractIdentityAndSelectProtocol();

    if (c->protocol == xml_language) {
      if (!c->XMLVersionOK()) {
	c->SendRefusal("Version mismatch.");
	c->Notify(true, "FAILED for version mismatch", true);
	c->Close();
    
      } else {
#ifdef PICSOM_USE_MPI
	connection_type cto = c->type, ctm = conn_none;
#endif // PICSOM_USE_MPI
	map<string,string> extra;
	if (c->can_do_mpi) {
#ifdef PICSOM_USE_MPI
	  cout << "CAN DO MPI" << endl;
	  c->ConvertToMPIListen();
	  ctm = c->type;
	  c->type = cto;
	  extra["mpiconnect"] = c->mpi_port;
#endif // PICSOM_USE_MPI
	}
	c->LaunchThreadAndOrSalute(extra);
	if (c->can_do_mpi) {
#ifdef PICSOM_USE_MPI
	  c->CloseFDs();
	  c->type = ctm;
	  c->MPIaccept(); // timeout?
	  read_ok = c->ReadAndParseXML(http, true);
	  if (!read_ok)
	    ShowError(hdr+"ReadAndParseXML() after MPI transfer failed");
#endif // PICSOM_USE_MPI
	}
	c->Notify(true, NULL, true);
      }
    }
    else if (c->protocol == mrml_language) {
      if (!c->MRMLVersionOK()) {
	c->SendRefusal("Version mismatch.");
	c->Notify(true, "FAILED for version mismatch", true);
	c->Close();
      }
      else { // continue mrml parsing
#ifdef USE_MRML 
	if (!c->MRMLCommunication()) {
	  c->SendRefusal("MRML handshake failed.");
	  c->Notify(true, "FAILED for MRML handshake");
	  c->Close();
	}	
	c->LaunchThreadAndOrSalute();
	//c->Notify(true, NULL, true); 
	//tarvitaanko vapauttaa c->xmldoc jossain????
	c->Close();
#endif //USE_MRML
      }
    }
    else {
      // do some checks for others (line, unspecified) as well?
    }

    return c;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerProcess(bool first) {
    string hdr = "HttpServerProcess() : ";

    http_analysis = NULL;
    http_analysis_args = "";

    bool ok = true;

    if (!HttpServerParseRequest())
      ok = ShowError(hdr+"HttpServerParseRequest() failed");

    if (http_request_lines.size()==0)
      return true;

    //for (;;);

    if (first) {  // this is a stupid bubble gum
      string id = HttpServerSolveUserAgent();
      size_t p = id.find_first_of(" \t");
      if (p!=string::npos)
	id.erase(p);
      id = Servant().Address()+" "+id;
      Identity(id);
      if (debug_http)
	Notify(true, NULL, true);
    }

    const string& query_strx = http_request_lines.begin()->second;
    string query_str = query_strx;
    if (query_str.find("/picsom/")==0 || query_str=="/picsom")
      query_str.erase(0, 7);

    if (query_str.substr(0, 6)=="/dojo/")
      return HttpServerDojo(query_str);

    if (query_str.substr(0, 5)=="/doc/")
      return HttpServerDoc(query_str);

    DataBase *db = NULL;
    if (query_str.substr(0, 10)=="/database/") {
      size_t ss = query_str.find_first_of("/?", 10);
      if (ss!=string::npos && query_str[ss]=='/') {
	string dbn = query_str.substr(10, ss-10);
	string fn  = query_str.substr(ss+1);
	if (Picsom()->IsAllowedDataBase(dbn)) {
	  db = Picsom()->FindDataBase(dbn);
	  if (db && Simple::FileExists(db->ExpandPath("html", fn)))
	    return HttpServerDataBaseHtml(db, fn);
	}
      }
    }

    size_t qsl = query_str.size();
    if (qsl>=7 && query_str.substr(qsl-5)==".html" &&
	query_str.find("/../")==string::npos) {
      string fpath, dotpath = "."+query_str;
      string srcpath = Picsom()->UserHomeDir()+"/picsom/c++"+query_str;
      if (Simple::FileExists(dotpath))
	fpath = dotpath;
      else if (Simple::FileExists(srcpath))
	fpath = srcpath;
      if (fpath!="") {
	string data = FileToString(fpath);
	string otyp = "text/html";
	bool expnow = true;
      
	return HttpServerWriteOut(data, otyp, expnow, "");
      }
    }

    HttpServerSolveAnalysis(query_str);
    string xslname = http_analysis ? http_analysis->XslName() : "";
    list<string> xsllist;
    if (xslname!="")
      xsllist.push_back(xslname);
    else
      xsllist = DefaultXSL();

    bool needs_query = false;
    string ref = HttpServerSolveRequestField("Referer", "");
    string url = HttpServerSolveRedirection(query_str, ref, needs_query);
    if (url!="")
      ok = HttpServerRedirect(url) &&
	HttpServerProcessAfterRedirect(needs_query);
    
    else {
      string req_type = HttpServerSolveRequestType(query_str);

      if (req_type=="proxy") {
	if (DebugProxy()>0)
	  WriteLog(hdr+"processing proxy request ["+query_str+"]");
	string proxyhdr = Picsom()->HttpListenConnection();
	string pre = query_str, post;
	size_t q = pre.find("/proxy/");
	if (q!=string::npos) {
	  post = pre.substr(q+7);
	  proxyhdr += pre.substr(0, q+7);
	  pre.erase(q);
	}
	ok = HttpServerProxy(proxyhdr, pre, post);

      } else if (req_type=="OPTIONS") {
	ok = HttpServerWriteOut("", "", false, "");

      } else if (req_type=="text/xml" || req_type=="*/*") {
	if (query_str.size()>5 && query_str.substr(query_str.size()-3)==".js") {
	  string jf = query_str.substr(1), js = HttpServerJavaScript(jf);
	  if (js=="")
	    ok = ShowError(hdr+"javascript <"+jf+"> not found or empty");
	  else
	    ok = HttpServerWriteOut(js, "text/javascript", false, "");

	} else {

	  int statuscode = 200; // == OK
	  string ctype = "text/xml"; 
	  XmlDom doc;

	  if (query_str.size()>5 && query_str.find('?')==string::npos &&
	      query_str.substr(query_str.size()-4)==".xsl") {
	    doc = HttpServerStyleSheet(query_str.substr(1));
	    ctype = "text/xsl";

	  } else if (query_str=="/")
	    doc = HttpServerStartupPage();

	  else
	    doc = HttpServerGeneratePage(query_str, xsllist);

	  if (!doc.DocOK()) {
	    statuscode = 404; // == Not Found
	    ctype = "text/html";
	    doc = HttpServerErrorDocument(statuscode);  // or, redirect to "/" ?
	    ok = ShowError(hdr+"failed with req_type=["+req_type
			   +"] query_str=["+query_str+"]");
	  }

	  bool expire = false;
	  ok = HttpServerWriteOut(doc, ctype, expire, statuscode);
	  doc.DeleteDoc();
	}

      } else if (req_type.find("image/")==0)
	ok = HttpServerWriteOutImage(query_str, req_type);

      else if (req_type.find("video/")==0) {
	string range = HttpServerSolveRequestField("Range", "");
	ok = HttpServerWriteOutVideo(query_str, req_type, range);

      } else if (req_type.find("text/vtt")==0) {
	ok = HttpServerWriteOutTrack(query_str, req_type);

      } else if (req_type=="sqlite3db")
	ok = HttpServerWriteOutSqlite3db(db);

      else if (req_type=="classfile") {
	string cls = query_str;
	size_t p = cls.rfind('/');
	if (p!=string::npos)
	  cls.erase(0, p+1);
	ok = HttpServerWriteOutClassFile(db, cls);
      }

      else if (req_type=="miscfile") {
	if (db) {
	  string file = query_str;
	  size_t p = file.find(db->Name());
	  if (p!=string::npos) {
	    p += db->Name().size();
	    file.erase(0, p+1);
	  }
	  ok = HttpServerWriteOutFile(db, file, false);

	} else {
	  string fp = query_str;
	  string file = Picsom()->RootDir()+fp;
	  if (fp=="/picsom.bin")
	    file = Picsom()->MyBinary();

	  if (fp=="/robots.txt" && !FileExists(file)) {
	    string useragent = "???";
	    for (auto i=http_request_lines.begin();
		 i!=http_request_lines.end(); i++) {
	      // cout << "\"" << i->first << "\"=\"" << i->second << "\""
	      //      << endl;
	      if (LowerCase(i->first)=="user-agent")
		useragent = i->second;
	    }

	    string robotstxt = "User-agent: *\nDisallow: /\n";
	    ok = HttpServerWriteOut(robotstxt, "text/plain", false, "");
	    WriteLog(hdr+"Sent robots.txt to user-agent \""+useragent
		     +"\" from \""+identity_str+"\"");
	    file = "";
	  }

	  if (file!="")
	    ok = HttpServerWriteOutFile(file, false);
	}
      }
      
      else
	ok = ShowError(hdr+"req_type=["+req_type+"] not understood");
    }
    
    // connection keep-alive/close and socket closing should all happen in
    // HttpServerWriteOut()...
    //
    // bool temp_redirection_done = false;
    // if (true || temp_redirection_done) {
    //   // this should not be needed! temporary redirection doesn't work without...
    //   Picsom()->CloseConnection(this, false, true, false);
    // }
    
    return ok;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerProcessAfterRedirect(bool needs_query) {
    string hdr = "HttpServerProcessAfterRedirect() : ";

    if (!query) 
      return needs_query ? ShowError(hdr+"query==NULL") : true;

    bool debug = debug_http>3;
    if (debug)
      cout << hdr << endl;

    string algorithm;
    set<string> parameters; // parameters will be in alphabetical order!

    bool firstclassaugm = true;
      
    typedef list<pair<string,string> > var_t;
    var_t var = HttpServerParseFormData();
    for (var_t::const_iterator i=var.begin(); i!=var.end(); i++) {
      if (debug)
	cout << "  [" << i->first << "] = [" << i->second << "]" << endl;

      if (i->first=="featsel") {
        if (algorithm!="") {
          if (debug)
            cout << "  featsel algorithm=" << algorithm << endl;

          string algparam = algorithm;
          for (set<string>::const_iterator j=parameters.begin();
               j!=parameters.end(); j++) {
            if (debug)
              cout << "    [" << *j << "]";
            if (j->substr(0, algorithm.size())==algorithm &&
                ((*j)[algorithm.size()]==':' ||
		 (*j)[algorithm.size()]=='=')) {
              algparam += "_"+j->substr(algorithm.size()+1);
              if (debug)
                cout << " HIT algparam="  << algparam;
            }
            if (debug)
              cout << endl;
          }
	  if (debug)
	    cout << "  algparam=[" << algparam << "]" << endl;

          Picsom()->DoInterpret("algorithm", algparam, NULL, query, this, NULL,
                                NULL, NULL, false, true);
        }

        if (query->Algorithms().empty())
          query->SelectIndex(NULL, i->second);
        else
          for (size_t j=0; j<query->Algorithms().size(); j++)
            query->SelectIndex(&query->Algorithms()[j], i->second);

      } else if (i->first=="textquery") {
        query->TextQuery(i->second);

      } else if (i->first=="classaugment") {
        if (firstclassaugm) {
          query->ClearClassAugmentations();
          firstclassaugm = false;
        }
        query->ProcessClassState("plus", i->second);

      } else if (i->first=="algsel") {
        algorithm = i->second;

      } else if (i->first.find("paramsel::")==0) {
	string pre = i->first.substr(10);
        if (debug)
          cout << "  paramsel " << pre << "=" << i->second << endl;
        parameters.insert(pre+"="+i->second);

      } else if (i->first=="start") {
        // nothing?

      } else if (i->first=="continue") {
	// this is the standard submit button
        query->SkipOptionPage(false);

      } else if (i->first=="annotations") {	
	// the submit button for the annotation page
	query->SkipOptionPage(true);

      } else if (i->first.substr(0, 3)=="obj") {      
        vector<string> valvec = SplitInCommas(i->second);
        string label  = valvec[0], aspect = valvec[1];
        int idx = query->LabelIndex(label);
        if (idx>=0 && !query->IsSeen(idx)) {
          query->MarkAsSeenNoAspects(idx, 1.0);
          query->ProcessAspectInfo(aspect, idx, 1.0);
        }

      } else if (i->first.find("classdef::")==0) {
	string cdef = i->first.substr(10);
	vector<string> cdefc = SplitInCommas(cdef);
	if (debug)
          cout << "  classdef " << cdef << "=" << i->second << endl;
	int val = 0;
	if (i->second=="yes")
	  val = 1;
	else if (i->second=="no")
	  val = -1;
	if (val!=0)
	  query->WriteClassdefToFile(cdefc[1], cdefc[0], val);

      } else
        ShowError(hdr+"["+i->first+"]=["+i->second+"] not processed");
    }

    if (debug)
      cout << endl << endl
           << "query->Parent()=" << query->Parent()
           << " query->SkipOptionPage()=" << query->SkipOptionPage()
           << " query->HasAlgorithmAndFeatures()="
           << query->HasAlgorithmAndFeatures() << endl << endl << endl;

    // algorithm&feature selection page doesn't have parent
    if (query->Parent() && !query->SkipOptionPage()
        && query->HasAlgorithmAndFeatures()) {
      query->MarkAllNewObjectsAsSeenAspect(-1.0, "");
      query->ExecuteRunPartThree();
      if (query->Parent())
        query->Parent()->CloseVideoOutput();
    }

    if (debug)
      cout << "query->NnewObjects()=" << query->NnewObjects()
           << endl << endl << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpParseHeaders(const string& header,
                                    http_headers_t& hlines) {
    bool debug_pairs = debug_http>1, debug_data = debug_http>2;

    string msg = "HttpParseHeaders() : ";

    // cout << "[[[" << http_request << "]]]" << endl;

    typedef pair<string,string> P;

    string lend;

    size_t p = 0;
    for (int i=0; p!=string::npos; i++) {
      if (lend=="") {
	size_t q1 = header.find_first_of("\n\r");
	if (q1!=string::npos) {
	  size_t q2 = header.find_first_not_of("\n\r", q1);
	  if (q2==string::npos)
	    q2 = header.size();
	  lend = header.substr(q1, q2-q1);
	}
      }
      if (lend=="")
	break; // return ShowError(msg+"unable to solve <CRLF>");

      size_t q = header.find(lend, p);
      if ((i<=1 && q==p) || q==string::npos)
        break;
      
      string tmp = header.substr(p, q-p);
      if (i>1 && tmp=="") {
        size_t pt = q+lend.size();
        hlines.push_back(P("", header.substr(pt)));
        if (debug_pairs) {
          const string& s = hlines.back().second;
          cout << msg << s.size() << " bytes of data" <<endl;
          if (debug_data)
            cout << "[" << s << "]" << endl;
        }
        break;
      }

      size_t c = tmp.find(i ? ':' : ' ');
      if (c==string::npos || c==0 || c>tmp.size()-2)
        return ShowError(msg+"failing A ["+tmp+"]");

      size_t l = tmp.find_first_not_of(" \t", c+(i?2:1));
      if (l==string::npos && i==0)
        return ShowError(msg+"failing B");
      
      string k = tmp.substr(0, c), v;
      if (l!=string::npos)
	v = tmp.substr(l, tmp.size()-l);

      if (i==0) {
        size_t x = v.find(' ');
        if (x!=string::npos)
          v.erase(x);
      }

      hlines.push_back(P(k, v));

      if (debug_pairs)
        cout << msg << " <" << k << "><" << v << ">" << endl;

      p = q+2;
    }
    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::HttpServerParseRequest() {
  string msg = "HttpServerParseRequest() : ";

  ConditionallyDumpXML(cout, false, NULL, "", http_request, true);

  http_request_lines.clear();

  HttpParseHeaders(http_request, http_request_lines);

  // obs! disabled 2012-12-28 due to some empty requests...
  // return http_request_lines.empty() ? ShowError(msg+"failing C") : true;
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::HttpServerFullHeaderFound(const string& s) const {
  return s.find("\r\n\r\n")!=string::npos;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::HttpServerContentMissing(const string& s) const {
  string h = "\r\nContent-Length:";
  size_t l = s.find(h);
  size_t e = s.find("\r\n\r\n");

  if (l==string::npos)
    return false;

  if (e<l) // error?
    return false;

  stringstream ss;
  ss.str(s.substr(l+h.size(), 15));
  size_t r = string::npos;
  ss >> r;

  return s.size()-e-4<r;
}

///////////////////////////////////////////////////////////////////////////////

int Connection::HttpServerSolveContentLength(const string& s) const {
  string h = "\r\nContent-Length:";
  size_t p = s.find(h);
  if (p!=string::npos) {
    stringstream ss;
    ss.str(s.substr(p+h.size(), 15));
    int r = -1;
    ss >> r;
    return r;
  }
  return -1;
}

  /////////////////////////////////////////////////////////////////////////////

  const string& Connection::HttpServerSolveRequestType(const string& path) {
    static const string options   = "OPTIONS";
    static const string textxml   = "text/xml";
    static const string image     = "image/*";
    static const string proxy     = "proxy";
    static const string sqlite3db = "sqlite3db";
    static const string classfile = "classfile";
    static const string miscfile  = "miscfile";

    string msg  = "HttpServerSolveRequestType("+path+") : ";
    string msg1 = "HttpServerSolveRequestType() returns ", msg2 = " due to ";

    if (http_request_lines.size()) {
      http_headers_t::const_iterator i = http_request_lines.begin();
      if (i->first=="OPTIONS")
	return options;
    }

    static set<string> knownfiles {
      "/share/lscom/lscom.map",
      "/share/lscom/tv11.sin.relations.txt",
      "/linux64/lib/libcaffe.so",
      "/linux64/lib/libcublas.so.5.5",
      "/linux64/lib/libcudart.so.5.5",
      "/linux64/lib/libcurand.so.5.5",
      "/linux64/lib/libgflags.so",
      "/linux64/lib/libglog.so.0",
      "/linux64/lib/libvl.so",
      "/picsom.bin",
      "/robots.txt"
    };

    if (knownfiles.find(path)!=knownfiles.end()) {
      if (debug_http)
	cout << msg1+miscfile+msg2+" <"+path+"> found in knownfiles" << endl;
      return miscfile;
    }

    string fname = path;
    size_t sp = fname.rfind('/');
    if (sp==string::npos)
      fname = "";
    fname.erase(0, sp+1);

    string extra;
    sp = fname.find('?');
    if (sp!=string::npos)
      extra = fname.substr(sp+1);

    if (fname=="favicon.ico") {
      if (debug_http)
	cout << msg1+image+msg2+"/favicon.ico" << endl;
      return image;
    }

    if (fname=="picsom.jpg") {
      if (debug_http)
	cout << msg1+image+msg2+"/picsom.jpg" << endl;
      return image;
    }

    if (fname=="sqlite3.db") {
      if (debug_http)
	cout << msg1+sqlite3db+msg2+"/sqlite3.db" << endl;
      return sqlite3db;
    }

    sp = path.find("/proxy/");
    if (sp!=string::npos) {
      if (debug_http)
	cout << msg1+proxy+msg2+"/proxy/" << endl;
      return proxy;
    }

    sp = path.find("/timeline/");
    if (sp!=string::npos) {
      if (debug_http)
	cout << msg1+image+msg2+"/timeline/" << endl;
      return image;
    }

    sp = path.rfind("/database/");
    if (sp!=string::npos) {
      bool test_obj = false;  // used to be true until 2015-04-14
      pair<string,string> dbobj =
	HttpServerDataBaseAndLabel(Picsom(), path.substr(sp+10),
				   true, true, test_obj);
      if (dbobj.first!="") {
	string ret = image;
	DataBase *db = Picsom()->FindDataBase(dbobj.first);
	int oidx = db->LabelIndex(dbobj.second);
	map<string,string> m = db->ReadOriginsInfo(oidx, false, true);
	if (m["format"]!="")
	  ret = m["format"];
	static const string mp4 = "video/mp4";
	if (debug_http)
	  cout << msg1+ret+msg2+"/database/" << endl;
	return ret=="video/mp4"&&extra!="tn" ? mp4 : image;
      }
      pair<string,string> dbcls =
	HttpServerDataBaseAndClassFile(Picsom(), path.substr(sp+10),
				       true, true);
      if (dbcls.second!="") {
	if (debug_http)
	  cout << msg1+classfile+msg2+"/database/xxx/classes/" << endl;
	return classfile;
      }
      pair<string,string> dbfile =
	HttpServerDataBaseAndFile(Picsom(), path.substr(sp+10));
      if (dbfile.second!="") {
	if (dbfile.second.find("track-")==0) {
	  string tty = dbfile.second.substr(6);
	  size_t p = tty.find ('/');
	  if (p!=string::npos) {
	    tty.erase(p);
	    size_t p = tty.find ('-');
	    if (p!=string::npos)
	      tty.erase(p);
	    if (tty=="captions" || tty=="descriptions" || tty=="chapters"
		|| tty=="metadata" || tty=="subtitles") {
	      static const string textvtt = "text/vtt";
	      if (debug_http)
		cout << msg1+textvtt+msg2+"/database/track-XXX" << endl;
	      return textvtt;
	    }
	  }
	}

	// if (path.find("/object/")!=string::npos) {
	//   if (debug_http)
	//     cout << msg1+textxml+msg2+"/database/.../object/" << endl;
	//   return textxml;
	// }

	if (debug_http)
	  cout << msg1+miscfile+msg2+"/database/foo/bar" << endl;
	return miscfile;
      }
    }

    if (path.find("/objectinfo/")!=string::npos) {
      if (debug_http)
	cout << msg1+textxml+msg2+"/objectinfo/" << endl;
      return textxml;
    }

    if (path.size() && path[path.size()-1]=='/') {
      if (debug_http)
	cout << msg1+textxml+msg2+"trailing /" << endl;
      return textxml;
    }

    // if (path.find("/query/")!=string::npos &&
    //     path.find("/tssom/")!=string::npos) {
    //   if (debug_http)
    //     cout << msg1+textxml+msg2+"/query/.../tssom/" << endl;
    //   return image;
    // }

    if (path.find("/query/")!=string::npos &&
	path.find("/image/")!=string::npos) {
      if (debug_http)
	cout << msg1+textxml+msg2+"/query/.../image/" << endl;
      return image;
    }

    for (list<pair<string,string> >::const_iterator
	   i=http_request_lines.begin(); i!=http_request_lines.end(); i++) {
      if (debug_http)
	cout << msg << i->first << "=" << i->second << endl;
      string lc = i->first;  
      // lower-case lc...
      if (lc=="Accept") {
	if (i->second.find("*/*")==0) { // MSIE6
	  if (debug_http)
	    cout << msg1+textxml+msg2+"*/* MSIE6" << endl;
	  return textxml;
	}

	if (i->second.find("application/msword")!=string::npos) { // MSIE6
	  if (debug_http)
	    cout << msg1+textxml+msg2+"application/msword" << endl;
	  return textxml;
	}

	if (i->second.find("image")==0) {
	  if (debug_http)
	    cout << msg1+image+msg2+"image" << endl;
	  return image;
	}

	if (i->second.find("text")==0) {
	  if (debug_http)
	    cout << msg1+textxml+msg2+"textxml" << endl;
	  return textxml;
	}

	if (i->second=="*/*") {
	  if (debug_http)
	    cout << msg1+i->second+msg2+"*/*" << endl;
	  return i->second;
	}
      }
    }

    return textxml;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOutSqlite3db(DataBase *db) {
    string hdr = "HttpServerWriteOutSqlite3db() : ";
    
    if (debug_http)
      cout << hdr << "CALLED" << endl;
    
    if (!db) 
      return ShowError(hdr+"database=NULL");

    string data = db->SqliteDBstr("http");

    if (data=="")
      return ShowError(hdr+"empty content");
      
    string otyp = "application/x-sqlite3";
    return HttpServerWriteOut(data, otyp, true, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOutClassFile(DataBase *db,
					       const string& cls) {
    string hdr = "HttpServerWriteOutClassFile() : ";
    
    if (debug_http)
      cout << hdr << "CALLED" << endl;
    
    if (!db) 
      return ShowError(hdr+"database=NULL");

    string data = FileToString(db->ExpandPath("classes", cls));

    if (data=="")
      return ShowError(hdr+"empty content");
      
    string otyp = "application/x-picsom-classfile";
    return HttpServerWriteOut(data, otyp, true, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOutFile(DataBase *db,
					  const string& file, bool angry) {
    string hdr = "HttpServerWriteOutFile() : ";
    
    if (debug_http)
      cout << hdr << "CALLED" << endl;
    
    if (!db) 
      return ShowError(hdr+"database=NULL");

    string fname = db->ExpandPath(file);
    string data  = FileToString(fname);

    if (data=="") {
      if (angry)
	return ShowError(hdr+"empty content in <"+fname+"> expanded from <"+
			 db->Name()+"> and <"+file+">");
      else if (debug_http)
	cout << hdr << "<"+fname+"> expanded from <"+
	  db->Name()+"> and <"+file+"> is empty" << endl;
    }

    string otyp = "application/x-anything";
    return HttpServerWriteOut(data, otyp, true, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOutFile(const string& fname, bool angry) {
    string hdr = "HttpServerWriteOutFile() : ";
    
    if (debug_http)
      cout << hdr << "CALLED" << endl;
    
    string data  = FileToString(fname);

    if (data=="") {
      if (angry)
	return ShowError(hdr+"empty content in <"+fname+">");
      else if (debug_http)
	cout << hdr << "<"+fname+"> is empty" << endl;
    }

    string otyp = "application/x-anything";
    return HttpServerWriteOut(data, otyp, true, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOutImage(const string& path,
					   const string& mi) {
    string hdr = "HttpServerWriteOutImage("+path+","+mi+") : ";

    if (debug_http)
      cout << hdr << "CALLED" << endl;

    if (path=="/favicon.ico") {
      const pair<string,string>& f = HttpServerImageFavIcon();
      return HttpServerWriteOut(f.second, f.first, false, "");
    }

    if (path=="/picsom.jpg") {
      const pair<string,string>& f = HttpServerImagePicSOMlogo();
      return HttpServerWriteOut(f.second, f.first, false, "");
    }

    string foundel;
    if (path.find("/database/")==0)
      foundel = "database";
    else if (path.find("/timeline/")==0)
      foundel = "timeline";

    if (foundel!="") {
      string pathss = path.substr(foundel.size()+2), spec;
      size_t p = pathss.find('?');
      if (p!=string::npos) {
	spec = pathss.substr(p+1);
	pathss.erase(p);
	if (debug_http)
	  cout << hdr << "spec [" << spec << "] detected" << endl;
      }
    
      bool test_obj = false;  // used to be true until 2015-04-14      
      pair<string,string> dbobj = HttpServerDataBaseAndLabel(Picsom(), pathss,
							     true, true, test_obj);
      if (dbobj.first!="") {
	/*const*/ DataBase *db = Picsom()->FindDataBase(dbobj.first);
	if (!db)
	  return ShowError(hdr+"database <"+dbobj.first+"> not found");
      
	string objreq = dbobj.second, otyp, path, data;

	int idx = db->LabelIndex(objreq);

	if (foundel=="timeline") {
	  imagedata tline = db->CreateTimelineImage(idx, spec);
	  data = imagefile::stringify(tline, "image/png");
	  otyp = "image/png";

	} else { // foundel=="database"
	  if (db->SqlObjects()) {
	    map<string,string> m = db->ReadOriginsInfo(idx, false, false);
	    otyp = m["format"];
	    data = db->SqlObjectData(idx);

	  } else {
	    if (spec=="tn") {
	      pair<string,string> tnpathotyp = db->ThumbnailPath(idx);
	      path = tnpathotyp.first;
	      otyp = tnpathotyp.second;
	    }

	    if (path=="" &&
		db->ObjectsTargetTypeContains(idx, target_videosegment)) {
	      vector<int> so = db->SubObjects(idx);
	      bool forceit = false;
	      if (so.size()==0) {
		auto fl = db->VideoOrSegmentFramesOrdered(idx);
		if (fl.size()) {
		  so.push_back(fl[fl.size()/2].first);
		  forceit = true;
		}
	      }
	      for (size_t s=0; s<so.size(); s++)
		if (db->ObjectsTargetTypeContains(so[s], target_image)&&
		    (db->Label(so[s]).find("kf")!=string::npos || forceit)) {
		  path = db->ObjectPathEvenExtract(so[s]);
		  if (path!="") {
		    otyp = "image/jpeg";  // obs!?
		    if (mi!="image/*")
		      otyp = mi;
		    break;
		  }
		}
	    }

	    if (path=="") {
	      pair<string,string> tnpathotyp = db->VirtualThumbnailPath(idx);
	      path = tnpathotyp.first;
	      otyp = tnpathotyp.second;
	    }

	    if (path=="") {
	      otyp = "image/jpeg";  // obs!?
	      if (mi!="image/*")
		otyp = mi;
	      // in the following it was: db->SolveObjectPath(objreq) 
	      path = db->ObjectPathEvenExtract(idx);
	    }
	    data = FileToString(path);

	    if (data=="") {
	      ShowError(hdr+"failed to read image data from <"+path+">");
	      auto unavail = HttpServerImageNotAvailable();
	      data = unavail.second;	      
	      otyp = unavail.first;
	    }
	  }
	}

	return HttpServerWriteOut(data, otyp, false, "");
      }
    }

    size_t q = path.find("/query/"), i = path.find("/image/");
    if (q!=string::npos && i!=string::npos && q<i) {
      string qid = path.substr(q+7, i-q-7);
      Query *qq = Picsom()->FindQuery(qid, this, NULL);
      if (!qq) {
	if (debug_http)
	  cout << hdr << "query\"" << qid << "\" not found" << endl;
      } else {
	string itype = path.substr(i+7), rest;
	size_t z = itype.find("/");
	if (z!=string::npos) {
	  rest = itype.substr(z+1);
	  itype.erase(z);
	}

	if (itype=="tssom") {
	  size_t t = i+6;
	  string som = path.substr(t+7), spec;
	  size_t x = som.find('/');
	  if (x!=string::npos) {
	    spec = som.substr(x+1);
	    som.erase(x);
	  }
	  while ((x=som.find("%5B"))!=string::npos)
	    som.replace(x, 3, "[");
	  while ((x=som.find("%5D"))!=string::npos)
	    som.replace(x, 3, "]");
	  string data = qq->MapImageString(som, spec);

	  if (data!="") {
	    string otyp = "image/png";
	    return HttpServerWriteOut(data, otyp, false, "");
	  }

	} else if (itype=="erf-detections") {
	  string name = rest, spec;
	  imagedata idata = qq->ErfDetectionsImage(name, spec);
	  string otyp = "image/png", istr = imagefile::stringify(idata, otyp);
	  return HttpServerWriteOut(istr, otyp, false, "");
      
	} else if (debug_http)
	  cout << hdr << "sending NOT AVAILABLE because itype=\""
	       << itype << "\" is not known" << endl;
      }
    }

    if (debug_http)
      cout << hdr << "sending NOT AVAILABLE" << endl;

    const pair<string,string>& unavail = HttpServerImageNotAvailable();
    return HttpServerWriteOut(unavail.second, unavail.first, true, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOutVideo(const string& path,
					   const string& mi,
					   const string& range) {
    string hdr = "HttpServerWriteOutVideo("+path+","+mi+","+range+") : ";

    if (debug_http)
      cout << hdr << "CALLED" << endl;

    size_t maxsize = 1024*1024;

    size_t start = 0;
    if (range!="") {
      if (range.find("bytes=")!=0)
	return ShowError(hdr+"range <"+range+"> not understood");
      start = atol(range.substr(6).c_str());
    }

    if (path.find("/database/")==0) {
      string pathss = path.substr(10);
      pair<string,string> dbobj = HttpServerDataBaseAndLabel(Picsom(), pathss,
							     true, true, true);
      if (dbobj.first!="") {
	DataBase *db = Picsom()->FindDataBase(dbobj.first);
	if (!db)
	  return ShowError(hdr+"database <"+dbobj.first+"> not found");
	
	string objreq = dbobj.second, otyp, path, data;
	
	int idx = db->LabelIndex(objreq);

	if (db->ObjectsTargetTypeContains(idx, target_videosegment)) {}
	
	if (db->ObjectsTargetTypeContains(idx, target_video)) {
	  // string opath = db->SolveObjectPath(objreq);
	  string opath = db->ObjectPathEvenExtract(idx);
	  string data = FileToString(opath);
	  size_t s = data.size();
	  if (debug_http)
	     cout << hdr << "READ " << data.size() << " bytes from <"
		  << opath << "> for <" << objreq << ">" << endl;
	  if (start) {
	    if (debug_http)
	      cout << hdr << "SKIPPING " << start << " bytes" << endl;
	    data.erase(0, start);
	  }
	  if (range!="" && data.size()>maxsize) {
	    if (debug_http)
	      cout << hdr << "TRUNCATING to " << maxsize << " bytes" << endl;
	    data.erase(maxsize);
	  }
	  size_t end = start+data.size()-1;
	  if (range!="" /*&& s!=data.size()*/) {
	    string rrange = "bytes "+ToStr(start)+"-"+ToStr(end)+"/"+ToStr(s);
	    return HttpServerWriteOut(data, mi, false, rrange, 206, true,
				      data.size());
	  }
	  return HttpServerWriteOut(data, mi, false, "", 200);
	}
      }
    }
    
    if (debug_http)
      cout << hdr << "sending NOT AVAILABLE" << endl;
    
    const pair<string,string>& unavail = HttpServerVideoNotAvailable();
    return HttpServerWriteOut(unavail.second, unavail.first, true, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOutTrack(const string& path,
					   const string& type) {
    string hdr = "HttpServerWriteOutTrack("+path+","+type+") : ";

    if (debug_http)
      cout << hdr << "CALLED" << endl;

    if (type!="text/vtt")
      return ShowError(hdr+"type <"+type+"> not understood");

    if (path.find("/database/")==0) {
      string s = path.substr(10), o, extra;
      size_t p = s.rfind('?');
      if (p!=string::npos)
	extra = s.substr(p+1);
      pair<string,string> dbobj = HttpServerDataBaseAndLabel(Picsom(), s);
      s = dbobj.second;
      if (s.find("track-")==0) {
	s.erase(0, 6);
	p = s.find('/');
	if (p!=string::npos) {
	  o = s.substr(p+1);
	  s.erase(p);
	  if (debug_http)
	    cout << hdr << "PRODUCING [" << s << "] for <" << o << ">" << endl;
	  
	  DataBase *db = Picsom()->FindDataBase(dbobj.first);
	  if (!db)
	    return ShowError(hdr+"database <"+dbobj.first+"> not found");
	  int idx = db->LabelIndex(o);
	  string tmp = db->TempFile("text-vtt/"+dbobj.second)+"__"+extra;
	  if (!FileExists(tmp)) {
	    // segm=bb-25-05:svmdbHASHlscom-demo;
	    // detections=f::c_in12_z_fc6_a_ca3::bb::;
	    // thr=0.9;showsegm=no;showprob=yes
	    string spec = extra;
	    for (;;) {
	      size_t p = spec.find("HASH");
	      if (p==string::npos)
		break;
	      spec.replace(p, 4, "#");
	    }
	    db->GenerateSubtitles(idx, "vtt", spec, tmp);
	  }
	  string vttdata = FileToString(tmp);
	  bool ok = HttpServerWriteOut(vttdata, "text/vtt", true, "");
	  return ok;
	}
      }
    }

    if (debug_http)
      cout << hdr << "sending NOT AVAILABLE" << endl;

    string vtt = "WEBVTT\n\n00:00.000 --> 59:59.999\n path: "+
      path+" type: "+type+" not available\n\n";

    return HttpServerWriteOut(vtt, "text/vtt", true, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOut(const XmlDom& dom, const string& ctype,
				      bool expnow, int scode, bool keepa) {
    string dump = XML2String(dom.doc, true);
    return HttpServerWriteOut(dump, ctype, expnow, "", scode, keepa);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerWriteOut(const string& data, const string& ctype,
				      bool expnowin, const string& range,
				      int scode, bool keepain, size_t fulls) {
    bool expnow = expnowin || debug_expire;
    string exp  = expnow ? ExpirationNow() : ExpirationPlusYear();

    if (debug_http)
      cout << TimeStamp() << "HttpServerWriteOut() : " << data.size()
	   << " bytes of [" << ctype << "] expires=" << exp
	   << " scode=" << scode << " keepa=" << (int)keepain
	   << " fulls=" << fulls << endl;

    string stxt = "Internal Server Error";
    if (scode==200)
      stxt = "OK";
     if (scode==206)
      stxt = "Partial Content";
    if (scode==404)
      stxt = "Not Found";

    size_t clength = fulls ? fulls : data.size();

    bool keepa = keepain; // used to be false until 2015-04-21...
    const string crlf = "\r\n";
    stringstream ss;
    ss   << http_vers << " " << scode << " " << stxt  << crlf
	 << "Connection: " << (keepa?"keep-alive":"close") << crlf
	 << "Access-Control-Allow-Methods: POST, GET" << crlf
	 << "Accept-Ranges: bytes"               << crlf
	 << "Content-Length: "  << clength       << crlf;
    if (range!="")
      ss << "Content-Range: "   << range         << crlf;
    if (data!="") {
      ss << "Expires: "         << exp           << crlf
	 << "Content-Type: "    << ctype         << crlf << crlf
	 << data;
    } else
      ss << crlf;

    string str = ss.str();

    bool ret = WriteOutXml(NULL, str);
    if (!keepa)
      Picsom()->CloseConnection(this, false, true, false);

    return ret;
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::HttpServerRedirect(const string& url) {
  string msg = "HttpServerRedirect("+url+") : ";

  string perm = "301 Moved Permanently";
  string movd = "302 Moved";
  string temp = "307 Temporary Redirect";

  const string crlf = "\r\n";
  stringstream ss;
  ss << http_vers << " " << movd << crlf
     << "Location: " << url  << crlf << crlf;

  if (debug_http)
    cout << msg+movd << endl;

  return WriteOutXml(NULL, ss.str());
}

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerNotFound() {
    string msg = "HttpServerNotFound() : ";

    string html = "<html>File not found</html>\n";

    const string crlf = "\r\n";
    stringstream ss;
    ss << http_vers << " 404 Not Found"  << crlf
       << "Content-Type: "   << "text/html" << crlf
       << "Content-Length: " << html.size() << crlf << crlf
       << html;
    string str = ss.str();

    return WriteOutXml(NULL, str);
  }

///////////////////////////////////////////////////////////////////////////////

list<pair<string,string> > Connection::HttpServerParseFormData() {
  list<pair<string,string> > ret;

  string delim;

  for (list<pair<string,string> >::const_iterator i=http_request_lines.begin();
       delim=="" && i!=http_request_lines.end(); i++)
    if (i->first=="Content-Type")
      delim = i->second;

  if (delim=="")
    return ret;

  size_t p = delim.find("boundary=");
  if (p==string::npos)
    return ret;

  delim.erase(0, p+9);

  if (debug_http)
    cout << "DELIM=[" << delim <<"]" << endl;

  const string& str = http_request_lines.back().second;
  for (p=str.find(delim, 0); p<str.size(); ) {
    size_t q = str.find(delim, p+1);
    if (q==string::npos)
      break;

    string part = str.substr(p, q-p);
    if (debug_http)
      cout << "[" << part << "]" << endl;

    size_t h = part.find("\r\n\r\n");
    if (h!=string::npos) {
      string hdr = part.substr(0, h);
      size_t n = hdr.find("form-data; name=\"");
      if (n!=string::npos) {
	hdr.erase(0, n+17);
	n = hdr.find('"');
	if (n!=string::npos) {
	  string key = hdr.substr(0, n);
	  string val = part.substr(h+4);
	  n = val.find_first_of("\r\n");
	  if (n!=string::npos)
	    val.erase(n);
	  if (debug_http)
	    cout << "multipart/form-data : [" << key << "]=[" << val << "]"
		 << endl;
	  ret.push_back(make_pair(key, val));
	}
      }
    }

    p = q+delim.size()+2; // OBS! firefox uses two extra??? -- !
  }
  
  return ret;
}

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HttpServerErrorDocument(int statuscode) const {
    XmlDom html = HTMLstub();

    XmlDom head = html.Element("head");
    head.Element("title", "PicSOM HTTP server error");

    XmlDom body = html.Element("body");
    body.Element("h2", "PicSOM HTTP server: error "+ToStr(statuscode));

    return html;
  }

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HttpServerStartupPage() const {
    pair<XmlDom,XmlDom> docroot = XMLdocroot("result", IsDemo(), DefaultXSL());

    list<pair<string,string> > links;
    links.push_back(make_pair("databaselist", ""));
    links.push_back(make_pair("connectionlist", ""));
    links.push_back(make_pair("querylist", ""));
    links.push_back(make_pair("slavelist", ""));
    links.push_back(make_pair("threadlist", ""));
    links.push_back(make_pair("analysislist", ""));
    links.push_back(make_pair("versions", ""));
    links.push_back(make_pair("status", ""));

    string doxyindex = "html/index.html";
    if (FileExists(Picsom()->RootDir("doc.link")+"/"+doxyindex) ||
	FileExists(Picsom()->RootDir("doc")+"/"+doxyindex))
      links.push_back(make_pair("doc/"+doxyindex, ""));

    string dojotests = "dojo/dojo/tests/runTests.html";
    if (FileExists(Picsom()->RootDir()+"/"+dojotests))
      links.push_back(make_pair(dojotests, ""));

    const set<string>& adb = Picsom()->AllowedDataBases();
    for (set<string>::const_iterator n=adb.begin(); n!=adb.end(); n++) {
      DataBase *db = Picsom()->FindDataBase(*n);
      if (db) {
        string dir = db->ExpandPath("html");
        if (Simple::DirectoryExists(dir)) {
          list<string> l = Simple::SortedDir(dir);
          for (list<string> ::const_iterator f=l.begin(); f!=l.end(); f++)
            if (f->find(".html")==f->length()-5) {
              string url = "database/"+*n+"/"+*f, fn = dir+"/"+*f;
              XmlDom html = XmlDom::Parse(fn);
              string title = html.FindContent("/html/head/title", false);
              links.push_back(make_pair(url, title));
            }
        }
      }
    }

    if (links.size()) {
      XmlDom linklist = docroot.second.Element("infolinklist");
      for (list<pair<string,string> >::const_iterator i=links.begin();
	   i!=links.end(); i++) {
	XmlDom link = linklist.Element("infolink");
	link.Prop("href",     Picsom()->HttpListenConnection()+"/"+i->first);
	link.Prop("linktext", i->first);
	link.Prop("text",     i->second);
      }
    }

    return docroot.first.doc;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerSolveRequestField(const string& f,
						 const string& d) const {

    // obs! case-insensitivity should be added.

    for (list<pair<string,string> >::const_iterator
	   i = http_request_lines.begin(); i!= http_request_lines.end(); i++)
      if (i->first==f)
	return i->second;

    return d;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerSolveRedirection(const string& qstr,
                                                const string& ref,
						bool& needs_query) {
    string msg = "HttpServerSolveRedirection() : ";
    string hdr = "http://"+HttpServerSolveHost(), url;

    needs_query = true;

    if (IsDemo()) {
      needs_query = false;  // obs! the real reason should be solved...
      hdr += "/picsom";
    }

    if (qstr=="/rest/ca" || qstr=="/rest/ca/") {
      needs_query = false;
      url = hdr+"/doc/d2i-m_rest_api.html";
    }

    if (url=="" && qstr=="/action") {
      size_t q = ref.find("/query/");
      if (q!=string::npos) {
	string qname = ref.substr(q+7);
	if (qname.size() && *qname.rbegin()=='/')
	  qname.erase(qname.size()-1);

	Query *old_query = Picsom()->FindQuery(qname, this);
	if (!old_query) {
	  ShowError(msg+"FindQuery() failed");
	  return "";
	}

	size_t maxr = old_query->MaxRounds();

	if (maxr && old_query->Round()>=(int)maxr) 
	  url = old_query->EndPageUrl();
	else {
	  query = old_query->CreateChild(this);
	  QueryOwn(false);
	  url = ref.substr(0, q+7)+query->Identity()+"/";
	}

	old_query->CloseVideoOutput();
      }

      if (url=="")
	url = "http://users.ics.aalto.fi/jorma/dummy.cgi";
    }

    if (url=="" && IsDemo() && (qstr=="/" || qstr=="")) {
      url = hdr+"/databaselist";
    }

    if (url=="" && qstr=="/analyse/interactive/") {  // obs! hard-coded
      url = HttpServerAnalyseInteractiveRedirection(hdr);
    }

    if (url=="" && qstr.find("/click/")==0) {
      pair<string,string> qobj =
	HttpServerQueryAndLabel(Picsom(), qstr.substr(7));

      if (qobj.first!="" && qobj.second!="") {
	size_t p = qstr.rfind('?');
	if (p!=string::npos) {
	  string pos = qstr.substr(p+1);
	  url = HttpServerClickRedirection(qobj.first, qobj.second, pos);
	}

      } else {
	pair<string,string> dbobj =
	  HttpServerDataBaseAndLabel(Picsom(), qstr.substr(7));
	string label = dbobj.second;
	size_t q = label.find('?');
	if (q!=string::npos) {
	  string xy = label.substr(q+1);
	  label.erase(q);
	  // cout << dbobj.first << " " << label << " " << xy << endl;
	  
	  if (!http_analysis)
	    ShowError(msg+"http_analysis==NULL");
	  else {
	    vector<string> args;
	    args.push_back("**");
	    args.push_back("click "+label+" "+xy);
	    http_analysis->AnalyseInteractive(args);  // obs! hard-coded
	    
	    url = HttpServerAnalyseInteractiveRedirection(hdr);
	  }
	}
      }
    }

    if (url=="" && qstr.find("/database/")==0 && qstr.size()>10) {
      pair<string,string> dbobj =
	HttpServerDataBaseAndLabel(Picsom(), qstr.substr(10));
      if (dbobj.first!="") {
	if (debug_http)
	  cout << msg << "returning EMPTY due to dbobj.first!=\"\"" << endl;
	return "";
      }

      if (url=="") {
	if (!HttpServerNewQuery(qstr.substr(1))) {
	  ShowError(msg+"HttpServerNewQuery() failed");
	  url = hdr+"/error";
	} else
	  url = hdr+"/query/"+query->Identity()+"/";
      }
    }

    if (debug_http)
      cout << msg << "returning <" << url << ">" << endl;

    return url;
  }

  /////////////////////////////////////////////////////////////////////////////

    string Connection::HttpServerClickRedirection(const string& que,
						  const string& obj,
						  const string& pos) { 
    string msg = "HttpServerClickRedirection() : ";

    string defaultseg = "bb-25-05"; // OBS!

    query = Picsom()->FindQuery(que, this);
    if (!query || !query->GetDataBase())
      return "";

    DataBase *db = query->GetDataBase();

    int idxi = db->LabelIndexGentle(obj, false);
    if (idxi<0)
      return "";
    size_t pidx = idxi;
    idxi = db->ParentObject(pidx, false);
    if (idxi>=0)
      pidx = idxi;

    float dur = db->VideoDuration(pidx);
    if (dur==0)
      return "";

    size_t p = pos.find(".x=");
    if (p==string::npos)
      return "";

    size_t x = atoi(pos.substr(p+3).c_str()), w = 500;
    float r = float(x)/w, t = r*dur;
    
    string seg = obj;
    p = seg.find(':');
    if (p!=string::npos)
      seg.erase(p+1);
    else
      seg = defaultseg+":";
    seg += db->Label(pidx)+":t";

    for (size_t i=0; i<1000000; i++) {
      string clab = seg+ToStr(i);
      int cidx = db->LabelIndexGentle(clab, false);
      if (cidx<0)
	return "";

      auto psd = db->ParentStartDuration(cidx, target_video);
      if (t>=psd.second.first && t<psd.second.first+psd.second.second) {
	string hdr = "http://"+HttpServerSolveHost(), url;
	url = "/objectinfo/"+que+"/"+clab;

	return hdr+url;
      }
    }

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerAnalyseInteractiveRedirection(const
							     string& hdr) {
    string msg = "HttpServerAnalyseInteractiveRedirection() : ";

    if (!http_analysis || !query ) {
      ShowError(msg+"http_analysis==NULL || query==NULL");
      return "";
    }

    vector<string> args;
    args.push_back("**");
    args.push_back("next");

    http_analysis->AnalyseInteractive(args);  // obs! hard-coded
    string url = "/", lab;
    if (query->NnewObjects())
      lab = query->NewObject(0).Label();

    if (lab!="")
      url = hdr+"/analyse/interactive/"+lab+"/";  // obs! hard-coded

    if (debug_http)
      cout << msg << "returning <"+url+">" << endl;

    return url;    
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerAjaxURL(const Query *q) const {
    return Picsom()->HttpListenConnection()+"/ajax/"+q->Identity()+"/";
  }

  /////////////////////////////////////////////////////////////////////////////

  Analysis *Connection::HttpServerSolveAnalysis(const string& url) {
    string msg = "HttpServerSolveAnalysis() : ";

    bool hardforce = false;

    if (!HttpServerSolveAnalysisInner(url)) {
      string ref = HttpServerSolveRequestField("Referer", "");
      if (!HttpServerSolveAnalysisInner(ref)) {
	if (hardforce)  // obs! hard-coded
	  HttpServerSolveAnalysisInner("analyse/interactive");
      }
    }

    if (!http_analysis && hardforce)
      ShowError(msg+"HttpServerSolveAnalysisInner() failed");

    return http_analysis;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerSolveAnalysisInner(const string& str) {
    string msg = "HttpServerSolveAnalysisInner("+str+") : ";

    http_analysis      = NULL;
    http_analysis_args = "";

    size_t p = str.find("analyse/");
    if (p==string::npos) {
      if (debug_http)
	cout << msg+"returning FALSE due to no \"analyse/\"" << endl;
      return false;
    }

    string aname = str.substr(p+8), args;
    p = aname.find('/');
    if (p!=string::npos) {
      args = aname.substr(p+1);
      aname.erase(p);
    }

    Analysis *a = Picsom()->FindAnalysis(aname);
    if (!a)
      return ShowError(msg+"PicSOM::FindAnalysis() failed with <"+aname+">");

    query = a->GetQuery();
    if (!query)
      return ShowError(msg+"PicSOM::GetQuery() failed");

    http_analysis      = a;
    http_analysis_args = args;

    if (debug_http)
      cout << msg+"SUCCESS with args=["+args+"]" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HttpServerGeneratePage(const string& path,
					    const list<string>& xsllin) {
    string xslj = CommaJoin(xsllin);
    string msg  = "HttpServerGeneratePage("+path+","+xslj+") : ";

    if (debug_http)
      cout << TimeStamp() << msg << "CALLED" << endl;

    if (path.find("/rest/ca/addTask")==0) {
      XmlDom xml = HttpServerRestCaAddTask(path.substr(16));
      return xml;
    }

    if (path.find("/rest/ca/queryTaskStatus")==0) {
      XmlDom xml = HttpServerRestCaQueryTaskStatus(path.substr(24));
      return xml;
    }

    return HttpServerGeneratePageInner(path, xsllin);
  }    

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::RestCaCallback(const string& type, Analysis *a,
				  const vector<string>& s) {
    string msg = "Connection::RestCaCallback(Analysis,"+type+","+
      ToStr(s)+") : ";

    a->Picsom()->PossiblyShowDebugInformation("RestCaCallback");

    if (type=="taskanalysis")
      return RestCaCallbackTaskAnalysis(a, s);

    if (type=="taskbackendfeedback")
      return RestCaCallbackTaskBackendFeedback(a, s);

    return ShowError(msg+"callback <"+type+"> unknown");    
  }

  /////////////////////////////////////////////////////////////////////////////

  /// http://193.166.24.251:8080/SummarizerWeb/tag_list.html
  /// 2467791 30313925

  XmlDom Connection::HttpServerRestCaAddTask(const string& argsin) {
    string msg  = "HttpServerRestCaAddTask("+argsin+") : ", err;

    bool debug = true; // debug_http
    if (debug)
      cout << TimeStamp()+ThreadIdentifierUtil()+" "
	   << msg << "CALLED" << endl;

    if (argsin!="") {
      ShowError(msg+"arguments deprecated \""+argsin+"\"");
      err = "Extraneous parameters";
    }

    string workload = http_request_lines.back().second;

    if (debug)
      cout << " workload=<" << workload << ">" << endl;
    
    XmlDom xmlwl = XmlDom::FromString(workload);
    if (!xmlwl.DocOK() && err=="")
      err = "Workload XML invalid";

    if (err=="" && xmlwl.Root().NodeName()!="taskDetails")
      err = "workload root is not taskDetails";

    string taskId, taskType = "UNKNOWN", backendId;

    if (err=="") {
      taskId         = xmlwl.Root().FindContent("taskId",    false);
      taskType       = xmlwl.Root().FindContent("taskType",  false);
      backendId      = xmlwl.Root().FindContent("backendId", false);
    }

    static size_t nro = 0;
    string xmlf = "rest-ca-addtask-"+taskType+"-taskId="+taskId+"-"
      +ToStr(getpid())+"-"+ToStr(nro++)+".xml";
    StringToFile(workload, xmlf);
    WriteLog("Wrote REST CA addTask workload in file <"+xmlf+">");

    if (err=="" && backendId!="2")
      err = "backendId should be '2' for PicSOM";

    if (err=="" && taskType!="ANALYSIS" && taskType!="BACKEND_FEEDBACK" )
      err = "taskType should be 'ANALYSIS' or 'BACKEND_FEEDBACK'";

    if (err=="" && taskId=="")
      err = "taskId should be specified";

    set<string> analysistype; // AUDIO FACE_DETECTION KEYWORD_EXTRACTION VISUAL
    float nan = numeric_limits<float>::max(), sequenceduration = nan;
    string sequencetype; // SECOND SHOT FULL
    list<pair<float,float> > timecode;

    if (err=="") {
      XmlDom taskParameters = xmlwl.Root().FirstElementChild("taskParameters");
      if (taskParameters) {
	XmlDom analysisTypeList = taskParameters.FirstElementChild("analysisTypeList");
	if (analysisTypeList) {
	  XmlDom analysisType = analysisTypeList.FirstElementChild("analysisType");
	  while (analysisType) {
	    string atype = analysisType.FirstChildContent();
	    analysistype.insert(atype);
	    analysisType = analysisType.NextElement("analysisType");
	  }
	}
	XmlDom sequenceDuration = taskParameters.FirstElementChild("sequenceDuration");
	if (sequenceDuration)
	  sequenceduration = atof(sequenceDuration.FirstChildContent().c_str());
	XmlDom sequenceType = taskParameters.FirstElementChild("sequenceType");
	if (sequenceType)
	  sequencetype = sequenceType.FirstChildContent().c_str();
	XmlDom timeCodeList = taskParameters.FirstElementChild("timeCodeList");
	if (timeCodeList) {
	  XmlDom timeCode = timeCodeList.FirstElementChild("timeCode");
	  while (timeCode) {
	    string from = timeCode.FirstElementChild("from").FirstChildContent();
	    string to   = timeCode.FirstElementChild("to"  ).FirstChildContent();
	    timecode.push_back(make_pair(atof(from.c_str()), atof(to.c_str())));
	    timeCode = timeCode.NextElement("timeCode");
	  }
	}
      }
    }

    for (auto ai=analysistype.begin(); ai!=analysistype.end(); ai++)
      cout << "analysistype=" << *ai << endl;
    if (sequenceduration!=nan)
      cout << "sequenceduration=" << sequenceduration << endl;
    if (sequencetype!="")
      cout << "sequencetype=" << sequencetype << endl;
    for (auto ti=timecode.begin(); ti!=timecode.end(); ti++)
      cout << "timecode=" << ti->first << "," << ti->second << endl;

    if (err=="") {
      restca_task_stat[taskId].first = "EXECUTING";
      err = HttpServerRestCaRunTask(taskType, xmlwl);
      if (err!="")
	restca_task_stat[taskId].first = "ERROR";
    }

    XmlDom xmlout = XmlDom::Doc();
    XmlDom response = xmlout.Root("response");
    response.Prop("method",  "addTask");
    response.Prop("service", "ca");
    response.Element("stat", err==""?"ok":"fail");
    
    if (err!="") {
      response.Element("error_code", "PicSOMxxxx");
      response.Element("error_msg",  "... ... ...");
      response.Element("error",      err);
    }

    if (debug)
      cout << TimeStamp()+ThreadIdentifierUtil()+" "
	   << msg << "RETURNING w/\"" << err << "\"" << endl;

    return xmlout;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerRestCaRunTask(const string& taskType,
					     const XmlDom& task) {
    if (taskType=="ANALYSIS")
      return HttpServerRestCaRunTaskAnalysis(task);

    if (taskType=="BACKEND_FEEDBACK")
      return HttpServerRestCaRunTaskBackendFeedback(task);

    return "Neither ANALYSIS nor BACKEND_FEEDBACK";
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerRestCaRunTaskBackendFeedback(const
							    XmlDom& task) {
    XmlDom taskDetails = task.Root();
    string taskId = taskDetails.FindContent("taskId", false);
    string callbackUri = taskDetails.FindContent("callbackUri", false);

    list<string> script;
    script.push_back("analyse=sleep");
    script.push_back("callback=restcataskbackendfeedback("+taskId+",["+
		     callbackUri+"],Hello world!)");

    vector<string> argv { "-", "10" };
    script_list_e entry = make_pair("restca-backendfeedback-"+taskId,
				    make_pair(script, argv));
    script_list_t list;
    list.push_back(entry);

    Picsom()->RunInAnalysisThread(list);

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerRestCaRunTaskAnalysis(const XmlDom& task) {
    bool facerecognition = !true;
    bool zo5rgb = false;
    bool slavepipe = false;

    bool rootless_slaves = true;

    string sqlstr = restca_mysql ? "mysql" : "sqlite";
    string restcadbplain = "restca";
    string restcadbshort = string(restca_mysql?"mysql:":"")+restcadbplain;
    string restcadblong  = restcadbshort+"(use"+sqlstr+"=yes,sqlsettings=1,"
      "bin_features=yes,storedetections=bin,settings=settings-restca.xml)";

    string msg  = "HttpServerRestCaRunTask() : ", err;

    bool debug     = true;
    size_t verbose = 1;
    
    size_t labellength = 8; // == 2+2+2+2
    // 0.1 is a "good" value for caffe
    // 0.7 is a "good" value for svm
    float confidence_threshold = 0.7;
    //float confidence_threshold = 0; // obs! with testing only...

    if (debug)
      cout << TimeStamp()+ThreadIdentifierUtil()+" "
	   << msg << "starting" << endl;

    DataBase::OpenReadWriteFea(true);
    DataBase::OpenReadWriteDet(true);
    DataBase *db = Picsom()->FindDataBaseEvenNew(restcadblong, true);

    XmlDom taskDetails = task.Root();
    string taskId = taskDetails.FindContent("taskId", false);

    size_t media_count = 0;
    list<upload_object_data> imagelist;
    vector<pair<XmlDom,string> > media_xml;
    XmlDom mediaList = taskDetails.FirstElementChild("mediaList");
    XmlDom media = mediaList.FirstElementChild("media");

    while (media) {
      string url = media.FirstElementChild("url").FirstChildContent();
      string UID = media.FirstElementChild("UID").FirstChildContent();

      restca_task_stat[taskId].second[UID] = "EXECUTING";

      ground_truth uidgt = db->GroundTruthExpression("$sql(auxid='"+UID+"')");
      vector<size_t> uidgti = uidgt.indices(1);
      if (uidgti.size())
	cout << TimeStamp()+ThreadIdentifierUtil()+" "
	     << "UID=<"+UID+"> already found as " << uidgti.size()
	     << " objects, e.g. " << db->ObjectDump(uidgti[0]) << endl;

      bool fake = false && uidgti.size()>0;
      string ctype, file;
      if (!fake) {
	size_t maxsize = 10L*1024*1024*1024;
	float maxtime  = 1200.0;
	list<pair<string,string> > hdrs;
	file = DownloadFile(Picsom(), url, hdrs, ctype, -1, maxsize, maxtime);
	if (file=="")
	  ShowError(msg+"downloading \""+url+"\" failed");
	else if (ctype.find("image/")!=0 && ctype.find("video/")!=0 ) {
	  ShowError(msg+"\""+url+"\" is not image/* nor video/* but \""+
		    ctype+"\"");
	  Unlink(file);
	  file = "";
	}
	if (file=="")
	  restca_task_stat[taskId].second[UID] = "ERROR";
      }

      media_xml.push_back(make_pair(media, file));

      if (file!="" || fake) {
	imagelist.push_back(upload_object_data());
	upload_object_data& uod = imagelist.back();

	string ct;
	if (!fake) {
	  ct = libmagic_mime_type(file).first;
	  if (ct=="") {
	    ct = "image/unknown";
	    ShowError(msg+"setting MIME content type of <", file, "> to ", ct);
	  }
	}
	// uod.data  = DataBase::FileToString(file);
	uod.path     = file;
	uod.use      = "regular";
	uod.ctype    = ct;
	uod.url      = url;
	//uod.page   = UID=="" ? "-" : UID;
	uod.auxid    = UID=="" ? "-" : UID;
	uod.date     = PicSOM::OriginsDateStringNow(true);
	uod.keywords = vector<string>(); // SplitInCommas(classname);
	uod.fake     = fake;

	if (fake)
	  uod.indices.push_back(uidgti[0]);

	if (debug)
	  cout << TimeStamp()+ThreadIdentifierUtil()+" "
	       << "media file #" << media_count++ << " <" << uod.url << "> "
	       << FileSize(uod.path) << " bytes" << endl;
      }

      media = media.NextElement("media");
    }

    // if (media_xml.empty())
    //   return "All downloads failed";

    list<string> featlist, detectlist, segmentlist;
    XmlDom xml;
    string segout;
    if (!db->InsertObjects(imagelist, featlist, detectlist, segmentlist,
			   false, false, vector<string>(), xml, segout))
      ShowError(msg+"File insert failed");

    for (auto i=media_xml.begin(); i!=media_xml.end(); i++)
      if (i->second!="")
	Unlink(i->second);

    // if (err!="")
    //   return err;

    string threshold   = ToStr(confidence_threshold);
    string callbackUri = taskDetails.FindContent("callbackUri", false);

    string clscont;
    for (auto i=imagelist.begin(); i!=imagelist.end(); i++)
      clscont += string(clscont==""?"":"|")+
	"$exp("+db->Label(i->indices[0])+",0-1)";

    if (clscont!="")
      clscont = "= "+clscont+"\n";

    string clsname = "rest-ca-analysis-"+ToStr(getpid())+"-"+taskId;
    string clsfile = db->ExpandPath("classes", clsname);
    if (!db->WriteClassFile(clsfile, vector<string>(), clscont,
			    ground_truth(),
			    "rest-ca analysis taskId="+taskId, false, false))
      ShowError(msg+"Storing class file <"+clsfile+"> failed");
  
    Analysis::UsePthreads(1);

    list<string> script;
    script.push_back("analyse=segmentdetect");
    script.push_back("verbose="+ToStr(verbose));
    //script.push_back("debugtrap=yes");
    //script.push_back("threads=2");
    script.push_back("database="+restcadbplain);

    if (rootless_slaves)
      for (auto i=imagelist.begin(); i!=imagelist.end(); i++) {
	string str = db->Label(i->indices[0])+"|"+
	  db->ObjectsTargetTypeString(i->indices[0])+"|"+i->dbname+"|"+i->auxid;
	script.push_back("addobject="+str);
      }

    string features;
    //features = "c_in10_r_fc6_d_a,c_in12_r_fc6_d_a";

    if (facerecognition)
      features += string(features==""?"":",")+
	"of:OCVLbp-crop8550-8x8-nbh82-u2-nh1";
    if (zo5rgb)
      features += string(features==""?"":",")+"zo5:rgb";
    if (features=="")
      features = "**";
    features = "c_in12_r_fc6_d_ca3";
    if (features!="")
      script.push_back("features="+features);
    script.push_back("extractfeatures=yes");
    script.push_back("slavenofeaturedata=yes");
    script.push_back("check_first_feature_only=no");
    script.push_back("featextbatchsize="+ToStr(labellength));

    // string dets;
    // string dets = "aratio_nopad"; // i.e. caffe
    // string dets = "lin-s+cs-ds-svmdb-hkm,fusion-g-lin-hkm";
    // string dets = "lin-hi2,f-lin-hi2";
    // string dets = "lin-hi2-x,f-lin-hi2-x";
    string dets = "lin-hi2-1";

    if (dets!="") {
      script.push_back("detections="+dets);
      script.push_back("dodetections=yes");
    }
    if (facerecognition)
      script.push_back("segmentations=of");

    script.push_back("target=image");
    script.push_back("threshold="+threshold);
    script.push_back("queryrestriction="+clsname);
    script.push_back("callback=restcataskanalysis("+taskId+",["+
		     callbackUri+"],"+clsname+")");

    if (slavepipe)
      script.push_back("slavepipe=features+detections");

    vector<string> argv;
    script_list_e entry = make_pair("restca-analysis-"+taskId,
				    make_pair(script, argv));
    script_list_t list;
    list.push_back(entry);

    Picsom()->RunInAnalysisThread(list);

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HttpServerRestCaQueryTaskStatus(const string& argsin) {
    string msg  = "HttpServerRestCaQueryTaskStatus("+argsin+") : ", err;

    bool debug = true; // debug_http
    if (debug)
      cout << TimeStamp()+ThreadIdentifierUtil()+" "
	   << msg << "CALLED" << endl;

    string args = argsin;
    if (args[0]!='?') {
      ShowError(msg+"missing \"?\"");
      err = "Missing parameters";
      args = "?";
    }

    string task_id, data_groups, limits;

    vector<string> a = SplitInSomething("&", false, args.substr(1));
    for (size_t i=0; i<a.size(); i++) {
      pair<string,string> kv;
      try {
	kv = SplitKeyEqualValueNew(a[i]);
      } catch (const logic_error& exp) {
	ShowError(msg+"SplitKeyEqualValueNew() : "+exp.what());
	err = "Equal sign missing in parameter "+a[i];
	break;
      }
      if (kv.first=="task_id")
	task_id = kv.second;
      else if (kv.first=="data_groups")
	data_groups = kv.second;
      else if (kv.first=="limits")
	limits = kv.second;
      else {
        ShowError(msg+"parameter <"+kv.first+"> unknown");
        err = "Parameter "+kv.first+" unknown";
        break;
      }
    }

    if (task_id=="") {
      ShowError(msg+"task_id should be specified");
      err = "Missing a mandatory parameter";
    }

    if (data_groups=="")
      data_groups = "basic";

    auto p = restca_task_stat.find(task_id);
    if (p==restca_task_stat.end())
      err = "Task not found";

    XmlDom xmlout = XmlDom::Doc();
    XmlDom response = xmlout.Root("response");
    response.Prop("method",  "queryTaskStatus");
    response.Prop("service", "ca");
    response.Element("status", err==""?"ok":"fail");
    
    if (err!="") {
      response.Element("error_code", "PicSOMxxxx");
      response.Element("error_msg",  "... ... ...");
      response.Element("error",      err);

    } else {
      XmlDom task = response.Element("task");
      task.Element("taskId", task_id);
      task.Element("taskType", "ANALYSIS");
      task.Element("backendId", "2");
      task.Element("status", p->second.first);
      XmlDom mediaList = task.Element("mediaList");
      for (auto i=p->second.second.begin(); i!=p->second.second.end(); i++) {
	XmlDom media = mediaList.Element("media");
	media.Element("UID",  i->first);
	media.Element("status", i->second);
      }
    }

    if (debug)
      cout << TimeStamp()+ThreadIdentifierUtil()+" "
	   << msg << "RETURNING w/\"" << err << "\"" << endl;

    return xmlout;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::RestCaCallbackTaskBackendFeedback(Analysis *a,
						     const vector<string>& s) {
    string msg = "Connection::RestCaCallbackTaskBackendFeedback(Analysis,"+
      ToStr(s)+") : ";

    bool debug = DataBase::DebugDetections();
    bool savexml = true;

    if (s.size()!=3)
      return ShowError(msg+"s.size()!=3");

    string taskId = s[0], callbackUri = s[1];
    if (callbackUri.size()<3 || callbackUri[0]!='[' ||
	callbackUri[callbackUri.size()-1]!=']')
      return ShowError(msg+"malformed callbackUri <"+callbackUri+">");

    callbackUri = callbackUri.substr(1, callbackUri.size()-2);
    XmlDom taskxml = XmlDom::Doc();
    XmlDom task = taskxml.Root("taskResults");
    task.Element("taskId",    taskId);
    task.Element("taskType",  "BACKEND_FEEDBACK");
    task.Element("backendId", "2");
    task.Element("status",    "COMPLETED");
    task.Element("message",   "Backend feedback completed successfully.");

    PicSOM *picsom = a->Picsom();
    // DataBase *db = a->CheckDB();

    string xmlstr = taskxml.Stringify(false);
    if (debug)
      cout << taskxml.Stringify(true) << endl;

    if (savexml) {
      static int resno = 0;
      taskxml.Write("rest-ca-result--BACKEND_FEEDBACK-"
		    "taskId="+taskId+"-"
		    +ToStr(getpid())+"-"+ToStr(resno++)+".xml");
    }

    int timeout = 100;

    list<pair<string,string> > hdrs;
    string dstr, ctype;
    bool ok = Connection::DownloadString(picsom, "POST", callbackUri, hdrs,
					 xmlstr, dstr, ctype, 5, timeout);
    if (!ok)
      picsom->ShowError(msg+"DownloadString() <"+callbackUri+
			"> failed, ctype=["+ctype+"] dstr=["+dstr+"]");
    else
      picsom->WriteLog(msg+"cb=["+callbackUri+"] ctype=["+ctype+
			    "] dstr=["+dstr+"]");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::RestCaCallbackTaskAnalysis(Analysis *a,
					      const vector<string>& s) {
    string msg = "Connection::RestCaCallbackTaskAnalysis(Analysis,"+
      ToStr(s)+") : ";

    bool debug = DataBase::DebugDetections();
    bool savexml = true;

    if (s.size()!=3)
      return ShowError(msg+"s.size()!=3");

    string taskId = s[0], callbackUri = s[1];
    if (callbackUri.size()<3 || callbackUri[0]!='[' ||
	callbackUri[callbackUri.size()-1]!=']')
      return ShowError(msg+"malformed callbackUri <"+callbackUri+">");

    callbackUri = callbackUri.substr(1, callbackUri.size()-2);

    float threshold = a->Threshold();

    XmlDom taskxml = XmlDom::Doc();
    XmlDom task = taskxml.Root("taskResults");
    task.Element("taskId",    taskId);
    task.Element("taskType",  "ANALYSIS");
    task.Element("backendId", "2");
    task.Element("status",    "COMPLETED");
    task.Element("message",   "Analysis completed successfully.");
    XmlDom mediaList = task.Element("mediaList");

    PicSOM *picsom = a->Picsom();
    DataBase *db = a->CheckDB();

    size_t dimension = 1;
    vector<string> clsname;

    // vector<string> idxstr = SplitInSomething("|", false, s[2]);
    // for (auto is=idxstr.begin(); is!=idxstr.end(); is++) {
    //   string idxexpr = *is; // something like "$exp(#8,0-1)"
    //   size_t idx = 0, q = idxexpr.find('(');
    //   if (q!=string::npos) {// should always happen...
    // 	size_t p = idxexpr.find_first_of(",)", q);
    // 	if (p==string::npos)
    // 	  p = idxexpr.size();  // should never happen
    // 	string lab = idxexpr.substr(q+1, p-q-1);
    // 	idx = db->LabelIndex(lab);
    //   }
    // }

    string clsfilename = s[2];
    // in a rootless slave we should check that the file has been downloaded...
    ground_truth gt = db->GroundTruthExpression(clsfilename);
    vector<size_t> idxs = gt.indices(1);
    for (auto idxp=idxs.begin(); idxp!=idxs.end(); idxp++) {
      size_t idx = *idxp;
      string spec = "";
      bool angry = true, exists = false;
      map<string,vector<float> >
	detects = db->RetrieveDetectionData(idx, spec, angry, exists);

      multimap<float,string> res;

      if (debug)
	cout << endl << "threshold=" << threshold << " detections:" << endl;

      for (auto i=detects.begin(); i!=detects.end(); i++) {
	string full = i->first;
	// if (full.find("fusion")!=0 && full.find("caffe::")!=0 &&
	//     full.find("f-lin-hi2")!=0)
	//  continue;
	if (full.find("linear::c_in12_r_fc6_d_ca3::hkm-int2::")!=0)
	  continue;

	string raw = full;
	size_t p = raw.rfind('#');
	if (p!=string::npos)
	  raw.erase(0, p+1);
	else if (clsname.size()==0) {
	  DataBase *ilsvrdb = picsom->FindDataBase("ilsvrc2012-ics"); // obs!
	  list<string> clsl = ilsvrdb->SplitClassNames("all"); // obs!
	  clsname.insert(clsname.end(), clsl.begin(), clsl.end());
	  dimension = clsname.size();
	  if (dimension!=1000)
	    return ShowError(msg+"dimension!=1000");
	}

	if (i->second.size()!=dimension) {
	  ShowError(msg+"i->second.size()!=dimension");
	  continue;
	}

	for (size_t j=0; j<dimension; j++) {
	  if (dimension!=1)
	    raw = clsname[j];

	  float value = i->second[j];

	  if (debug)
	    cout << "  idx=" << idx << " : " << full << " = " << value
		 << " " << raw;

	  if (false) {
	    DataBase *svmdb = picsom->FindDataBaseEvenNew("svmdb", true);
	    
	    string params;
	    Index *svm = svmdb->GetSVM(i->first, params);
	    if (svm)
	      svm->CreatePDFmodels(100, false, false);
	    float cdf = svm ? svm->ValueToCDF(i->second[0], false) : -1;
	    float cor = svm ? svm->PDFmodelCorProb(i->second[0])   : -1;
	
	    if (debug)
	      cout  << " cdf=" << cdf << " cor=" << cor;

	    // value = cdf;
	    value = cor;
	  }

	  res.insert(make_pair(value, raw));

	  if (debug)
	    cout << endl;
	}
      }
      if (debug)
	cout << endl;
    
      map<string,string> m = db->ReadOriginsInfo(idx, false, false);

      if (debug)
	for (auto p=m.begin(); p!=m.end(); p++)
	  cout << "<" << p->first << ">=<" << p->second << ">" << endl;

      string UID = m["auxid"];
      if (UID=="")
	UID = "???";

      XmlDom media = mediaList.Element("media");
      media.Element("UID", UID);
      XmlDom objectList = media.Element("objectList");

      for (multimap<float,string>::const_reverse_iterator i=res.rbegin();
	   i!=res.rend(); i++)
	if (i->first>threshold) {
	  string fval = ToStr(i->first);
	  if (fval.find("0.")==0 && fval.size()>5)
	    fval.erase(5);

	  string raw = i->second, cooked, final;
	  if (raw.find("lscom")==0) {
	    cooked = DataBase::LscomName(picsom, raw, true);
	    pair<string,string> cp = DataBase::SplitLscomName(cooked);
	    final = cp.first;
	  }
	  if (raw.size()>1 && raw[0]=='c' &&
	      raw.substr(1).find_first_not_of("0123456789")==string::npos) {
	    DataBase *ilsvrdb = picsom->FindDataBase("ilsvrc2012-ics"); // obs!
	    const ground_truth *gt = ilsvrdb->ContentsFind(raw);
	    if (!gt) {
	      ilsvrdb->ConditionallyReadClassFile(raw, false, false);
	      gt = ilsvrdb->ContentsFind(raw);
	    }
	    if (gt) {
	      string gttxt = gt->text();
	      size_t q = gttxt.find(" : ");
	      if (q!=string::npos)
		gttxt.erase(0, q+3);
	      final = gttxt;
	    }
	  }

	  if (final=="")
	    final = "UNKNOWN";

	  string objectID = "picsom-"+db->Label(idx)+"-"+raw;

	  XmlDom object = objectList.Element("object");
	  object.Element("objectId",   objectID);
	  object.Element("objectType", "KEYWORD");
	  object.Element("status",     "CANDIDATE");
	  object.Element("backendId",  "2");
	  object.Element("value",      final);
	  object.Element("confidence", fval);
	}

      if (true) {
	//string fname = db->SqlTempFileName(idx);
	string fname = db->ObjectPathEvenExtract(idx);
	string segfile = fname;
	size_t p = segfile.rfind('/');
	if (p!=string::npos) {
	  segfile.replace(p, 1, "/segments/of:");
	  size_t q = segfile.rfind('.');
	  if (q!=string::npos && q>p)
	    segfile.replace(q, string::npos, ".seg");
	}

	// if (!FileExists(segfile) &&
	//     !db->SqlRetrieveSegmentationFile(idx, "of", segfile))
	//   return ShowError(msg+"SqlRetrieveSegmentationFile() failed");
	
	if (FileExists(segfile)) {
	  string imgfile;
	  picsom::segmentfile segf(imgfile, segfile);

	  set<int> flist = segf.listProcessedFrames();
	  if (flist.size()) {
	    int f = *flist.begin();
	    SegmentationResultList *r = segf.readFrameResultsFromXML(f, "", "");
	    for (auto j=r->begin(); j!=r->end(); j++) {
	      if (false) {
		cout << "  NAME:     " << j->name  << endl;
		cout << "  TYPE:     " << j->type  << endl;
		cout << "  VALUE:    " << j->value << endl;
		cout << "  METHODID: " << j->methodid << endl;
		cout << "  RESULTID: " << j->resultid << endl << endl;
	      }

	      string name = j->name;
	      if (j->type!="box" || name.find("face")!=0 || name.size()<5)
		continue;
	    
	      string facefeat = "of:OCVLbp-crop8550-8x8-nbh82-u2-nh1";
	      list<string> ffl { facefeat };
	    
	      // int flidx =
	      //   db->PossiblyInsertFaceObjectAndExtractFeatures(idx, segf,
	      // 						     f, name, ffl);
	      // string flabel = db->Label(flidx);
	    
	      string flabel = "of:"+db->Label(idx)+"_"+name;
	      int flidx = db->LabelIndex(flabel);

	      string objectID = "picsom-"+flabel;
	      XmlDom object = objectList.Element("object");
	      object.Element("objectId",   objectID);
	      object.Element("objectType", "FACE");
	      object.Element("status",     "CANDIDATE");
	      object.Element("backendId",  "2");

	      string value = j->value;
	      for (;;) {
		size_t p = value.find(" ");
		if (p==string::npos)
		  break;
		value[p] = ',';
	      }
	      XmlDom shape = object.Element("shape");
	      shape.Element("value", value);
	      shape.Element("shapeType", "RECTANGLE");

	      picsom->AddAllowedDataBase("d2ifaces");
	      DataBase *facedb = NULL; // picsom->FindDataBase("d2ifaces");
	      if (facedb) {
		FloatVector *ffvec = db->FeatureData(facefeat, flidx, true);
		list<pair<string,FloatVector*> > fnvlist;
		fnvlist.push_back(make_pair(facefeat, ffvec));
		string facename = facedb->RecognizeFace(fnvlist);

		if (facename!="" && facename!="error") {
		  object.Element("value", facename);
		  object.Element("confidence", "0.50");
		}

		delete ffvec;
	      }
	    }
	    delete r; // obs! should r be deleted somehow else?
	  }
	}
      }

      if (restca_task_stat[taskId].second[UID]=="EXECUTING")
	restca_task_stat[taskId].second[UID] = "COMPLETED";
    }

    restca_task_stat[taskId].first = "COMPLETED";

    string xmlstr = taskxml.Stringify(false);
    if (debug)
      cout << taskxml.Stringify(true) << endl;

    if (savexml) {
      static int resno = 0;
      taskxml.Write("rest-ca-result--ANALYSIS-"
		    "taskId="+taskId+"-"
		    +ToStr(getpid())+"-"+ToStr(resno++)+".xml");
    }

    int timeout = 100;

    list<pair<string,string> > hdrs;
    string dstr, ctype;
    bool ok = Connection::DownloadString(picsom, "POST", callbackUri, hdrs,
					 xmlstr, dstr, ctype, 5, timeout);
    if (!ok)
      picsom->ShowError(msg+"DownloadString() <"+callbackUri+
			"> failed, ctype=["+ctype+"] dstr=["+dstr+"]");
    else
      picsom->WriteLog(msg+"cb=["+callbackUri+"] ctype=["+ctype+
			    "] dstr=["+dstr+"]");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HttpServerGeneratePageInner(const string& path,
						 const list<string>& xsllin) {
    string xslj = CommaJoin(xsllin);
    string msg  = "HttpServerGeneratePageInner("+path+","+xslj+") : ";

    if (debug_http)
      cout << TimeStamp() << msg << "CALLED" << endl;

    XmlDom empty;

    string str = path;
  
    if (str[0]!='/') {
      if (debug_http)
	cout << msg << "returning EMPTY due to 1" << endl;
      return empty;
    }

    str.erase(0, 1);
    if (str=="") {
      if (debug_http)
	cout << msg << "returning EMPTY due to 2" << endl;
      return empty;
    }

    if (str[str.size()-1]=='/')
      str.erase(str.size()-1);
    if (str=="") {
      if (debug_http)
	cout << msg << "returning EMPTY due to 3" << endl;
      return empty;
    }

    if (str=="database")
      str = "databaselist";

    if (str=="query")
      str = "querylist";

    string query_name, query_spec;
    object_type ot = ot_no_object;
  
    string otstr = str, extra;
    size_t p = otstr.find_first_of("?/");
    if (p!=string::npos) {
      extra = otstr.substr(p+1);
      otstr.erase(p);
    }

    ot = ObjectType(otstr);
    if (ot==ot_error)
      ot = ot_no_object;

    if (ot==ot_no_object && str.find("query/")==0) {
      extra = "";

      string qname = str.substr(6), qextra;
      // size_t suf = qname.find("!S");
      // if (suf!=string::npos)
      //   qname.erase(suf);

      p = qname.find_first_of("?");
      if (p!=string::npos) {
	extra = qname.substr(p+1);
	qname.erase(p);
      }

      p = qname.find_first_of("/");
      if (p!=string::npos) {
	qextra = qname.substr(p+1);
	qname.erase(p);
      }

      query = Picsom()->FindQuery(qname, this);
      if (!query) {
	ShowError(msg+"FindQuery() failed");
	return empty;
      }
      QueryOwn(false);

      if (ot==ot_no_object && qextra!="") {
	if (qextra.find("erf-detections/")==0) {
	  ot = ot_queryimage;
	  query_name = "erf-detections";
	  query_spec = qextra.substr(15);
	} else {
	  ShowError(msg+"qextra=\""+qextra+"\" failed");
	  return empty;
	}
      }

      if (ot==ot_no_object && query->GetDataBase()) {
	if (query->NindicesNew()==0)
	  ot = ot_newidentity;
	else
	  ot = ot_objectlist;
      }
    }

    if (ot==ot_no_object && (str=="ajax" || str.find("ajax/")==0)) {
      extra = "";

      if (str.size()>5) {
	string qname = str.substr(5);
	if (qname=="Q:previous" && Picsom()->Nqueries())
	  query = Picsom()->GetQuery(Picsom()->Nqueries()-1);
	else {
	  if (qname.substr(0, 2)=="Q:" && !Query::SaneIdentity(qname)) {
	    DataBase *dbtmp = Picsom()->FindDataBase(qname.substr(2));
	    if (dbtmp) {
	      HttpServerCreateAjaxQuery();
	      query->CopySome(dbtmp->DefaultQuery(), true, false, false);
	      query->InterpretDefaults(0);
	      query->InterpretDefaults(1);
	    }
	  }
	  if (!query)
	    query = Picsom()->FindQuery(qname, this);
	}
	QueryOwn(false);

      } else 
	HttpServerCreateAjaxQuery();

      if (!query) {
	ShowError(msg+"FindQuery() failed");
	return empty;
      }
    
      string reqstr = http_request_lines.back().second;
      while (reqstr[0]=='\n')
	reqstr.erase(0, 1);

      XmlDom reqxml = XmlDom::FromString(reqstr);
      /*
	if (!reqxml.SchemaValidate(Picsom()->RootDir("xsd")+"/ajax.xsd"))
	ShowError(msg+"SchemaValidate() failed");
      */

      if (Query::DebugAjax()>3)
	DumpXML(cout, false, reqxml.doc, "ajaxrequest", "", true, false);

      query->ProcessAjaxRequest(reqxml, true, true);
      reqxml.DeleteDoc();

      ot = ot_ajaxresponse;
    }

    if (ot==ot_no_object && str.find("analyse/")==0) {
      extra = "";

      if (!http_analysis) {
	ShowError(msg+"http_analysis==NULL");
	return empty;
      }

      if (!query || query->NnewObjects()!=1) {
	ShowError(msg+"query==NULL || NnewImages()!=1");
	return empty;
      }

      ot = ot_objectlist;
    }

    // if (ot==ot_no_object && otstr=="database") {
    //   vector<string> pp = SplitInSomething("/", false, extra);
    //   if (pp.size()==2 && pp[1]=="sqlite3.db") {
    // 	string f = ExpandDataBasePath(pp[0], "sqlite3.db");
    // 	if (FileExists(f))
    // 	  {}
    //   }
    // }

    DataBase *db = query ? query->GetDataBase() : NULL;
    if (!db && ot==ot_no_object) {
      string dbstr = str, otstr2;
      size_t l = dbstr.find('/');
      if (l!=string::npos) {
	otstr2 = dbstr.substr(l+1);
	dbstr.erase(l);

	l = otstr2.find_first_of("/?");
	if (l!=string::npos) {
	  extra = otstr2.substr(l+1);
	  otstr2.erase(l);
	} else
	  extra = "";

	db = Picsom()->FindDataBase(dbstr);
	if (db) {
	  ot = ObjectType(otstr2);
	  if (ot==ot_error)
	    ot = ot_no_object;
	  else
	    otstr = otstr2;
	}
      }
    }

    string object;
    if (ot==ot_objectinfo) {
      extra = "";
      string dborquery = str;
      size_t p = dborquery.find("/");
      if (p!=string::npos) {
	dborquery.erase(0, p+1);
	p = dborquery.find("/");
	if (p!=string::npos) {
	  object = dborquery.substr(p+1);
	  dborquery.erase(p);
	}
	if (!query && Query::SaneIdentity(dborquery)) {
	  query = Picsom()->FindQuery(dborquery, this);
	  if (query) {
	    QueryOwn(false);
	    if (!db)
	      db = query->GetDataBase();
	  }
	}
	if (!db)
	  db = Picsom()->FindDataBase(dborquery);
      }
    }

    map<string,string> extras;
    if (extra!="") {
      vector<string> epart = SplitInSomething("+", false, extra);
      for (auto ev=epart.begin(); ev!=epart.end(); ev++) {
	try {
	  pair<string,string> kv = SplitKeyEqualValueNew(*ev);
	  extras[kv.first] = kv.second;
	} catch (...) {
	}
      }
    }

    list<string> xslleff;
    list<pair<string,string> > xsltparam;
    if (extras.find("xsl")!=extras.end()) {
      xslleff = list<string> { extras["xsl"] };
      extras.erase(extras.find("xsl"));
    }

    if (xslleff.empty() && query) {
      xslleff   = query->Xsl();
      xsltparam = query->XsltParam();
    }
    if (xslleff.empty())
      xslleff = xsllin;

    pair<XmlDom,XmlDom> xml = XMLdocroot("result", IsDemo(),
					 xslleff, xsltparam);

    if (extras.find("refresh")!=extras.end()) {
      // as of 2011-08-16 refresh was not used anymore
      // XmlDom refresh = xml.second.Element("refresh", extra.substr(8));
      // why not ... 2014-04-10:
      xml.second.Element("refresh", extras["refresh"]);
      extras.erase(extras.find("refresh"));
    }

    for (auto ee=extras.begin(); ee!=extras.end(); ee++) 
      ShowError(msg+"unable to process extra parameter \""+
		ee->first+"="+ee->second+"\"");
    
    if (IsPicSOM(ot)) {
      string name, spec;
      if (ot==ot_databaselist)
	spec = "flat";
    
      Picsom()->AddToXML(xml.second, ot, name, spec, this);

    } else if (query && IsQuery(ot)) {
      query->WaitUntilReady();

      if (ot==ot_objectlist) {
	HttpServerMetaErf(xml.second, query);
        query_spec = "brief";
      }

      if (ot==ot_newidentity && query->HasErfImageData())
	ot = ot_queryinfo;

      query->AddToXML(xml.second, ot, query_name, query_spec, this);
    }

    else if (db && IsDataBase(ot)) {
      XmlDom node = xml.second;

      string namestr, specstr;
      if (ot==ot_objectinfo) {
	node = node.Element("singleobject");
	namestr = object;
	specstr = "nodbinfo,novariables";
	size_t p = namestr.find('?');
	if (p!=string::npos) {
	  specstr += ","+namestr.substr(p+1);
	  namestr.erase(p);
	}
      }

      db->AddToXML(node, ot, namestr, specstr, query, this);

    } else if (IsTSSOM(ot)) {
      // ...
    }

    else {
      const char *ots = ObjectTypeP(ot);
      ShowError(msg+"failing with ot="+(ots?ots:"NULL")+ " ("+otstr+")");

      xml.first.DeleteDoc();
      return empty;
    }

    if (debug_http)
      cout << TimeStamp() << msg << "OK" << endl;

    return xml.first;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerCreateAjaxQuery() {
    query = new Query(Picsom());
    QueryOwn(false);
    query->SetIdentity();
    query->FirstClientAddress(ClientAddress());
    query->Target(target_image);
    query->SetLastModificationTimeNow();
    query->SetLastAccessTimeNow();
    Picsom()->AppendQuery(query, true);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerMetaErf(XmlDom& xml, const Query *q) const {
    XmlDom meta = xml.Element("meta");
    meta.Ns("", "");
    meta.Prop("name",    "erf/*");
    meta.Prop("content", HttpServerAjaxURL(q));
    meta.Prop("scheme",  "pinview/1.0");

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::HttpServerNewQuery(const string& path) {
  string msg = "HttpServerNewQuery(\""+path+"\") : ";

  string str = path;
  
  if (str.find("database/")!=0)
    return false;

  str.erase(0, 9);

  vector<string> a;
  size_t q = str.find('?');
  if (q!=string::npos) {
    string tmp = str.substr(q+1);
    str.erase(q);
    a =  SplitInSomething("+", false, tmp);
  }

  // from TryToRunXMLcommon()...
  if (query)
    return ShowError(msg+"query already set");

  DataBase *db = Picsom()->FindDataBase(str);
  if (!db)
    return ShowError(msg+"database <"+str+"> not found");

  query = new Query(Picsom());
  QueryOwn(false);
  query->SetIdentity();
  query->FirstClientAddress(ClientAddress());
  query->SetDataBase(db);
  query->CopySome(db->DefaultQuery(), true, false, false);
  query->InterpretDefaults(0);
  query->InterpretDefaults(1);
  if (query->Target()==target_no_target)
    query->Target(target_image);
  
  for (vector<string>::const_iterator i=a.begin(); i!=a.end(); i++) {
    try {
      pair<string,string> kv = SplitKeyEqualValueNew(*i);
      int r = 0;
      if (!query->Interpret(kv.first, kv.second, r, this) || !r)
        ShowError(msg+"interpreting <"+kv.first+">=<"+kv.second+"> failed");
    } catch (...) {
      ShowError(msg+"no = in <"+*i+"> ?");
    }
  }

  Picsom()->AppendQuery(query, true);

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HttpServerStyleSheet(const string& ssname) const {
    string xslname = Picsom()->RootDir("xsl")+"/"+ssname;

    if (debug_http)
      cout << "HttpServerStyleSheet("+ssname+") : xslname=<"+
	xslname+">" << endl;

    return XmlDom::Parse(xslname);
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::HttpServerJavaScript(const string& jsname) const {
    string name = Picsom()->RootDir("js")+"/"+jsname;
    if (debug_http)
      cout << "HttpServerJavaScript("+jsname+") : name=<"+name+">"
	   << endl;
    return FileToString(name);
  }
  
  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HttpServerActionResponsePage() {
    XmlDom html = HTMLstub();
    /*XmlDom head =*/ html.Element("head");
    XmlDom meta = html.Element("meta");
    meta.Prop("http-equiv", "refresh"); 
    meta.Prop("content",    "0; URL=http://picsom.ics.aalto.fi/picsom"); 

    return html;
  }

  /////////////////////////////////////////////////////////////////////////////

  const pair<string,string>& Connection::HttpServerImagePicSOMlogo() const {
    static pair<string,string> img;
    if (img.second=="") {
      string f = Picsom()->RootDir("icons")+"/picsom.jpg";
      if (FileExists(f))
	img = make_pair("image/jpeg", FileToString(f));
    }
    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  const pair<string,string>& Connection::HttpServerImageNotAvailable() const {
    static pair<string,string> img;
    if (img.second=="") {
      string f = Picsom()->RootDir("icons")+"/na.jpg";
      if (FileExists(f))
	img = make_pair("image/jpeg", FileToString(f));
    }
    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  const pair<string,string>& Connection::HttpServerVideoNotAvailable() const {
    static pair<string,string> img;
    if (img.second=="") {
      string f = Picsom()->RootDir("icons")+"/na.mp4";
      if (FileExists(f))
	img = make_pair("video/mp4", FileToString(f));
    }
    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  const pair<string,string>& Connection::HttpServerImageFavIcon() const {
    static pair<string,string> img;
    if (img.second=="") {
      string f = Picsom()->RootDir("icons")+"/favicon.bmp";
      if (FileExists(f))
	img = make_pair("image/bmp", FileToString(f));
    }
    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> 
  Connection::HttpServerQueryAndLabel(PicSOM *ps, const string& path,
				      bool test_q, bool test_lab,
				      bool test_obj) {
    string qname = path;
    size_t p = qname.find('?');
    if (p!=string::npos)
      qname.erase(p);

    p = qname.find('/');
    if (p==string::npos)
      return make_pair("", "");

    string label = qname.substr(p+1);
    qname.erase(p);

    if (test_q) {
      Query *q = ps->FindQuery(qname, NULL);
      if (!q)
	return make_pair("", "");

      DataBase *db = q->GetDataBase();
      if (db && test_lab) {
	int idx = db->LabelIndexGentle(label);

	if (idx<0)
	  return make_pair("", "");

	// in the following it was: db->SolveObjectPath(label) 
	if (test_obj && db->ObjectPathEvenExtract(idx)=="")
	  return make_pair("", "");
      }
    }

    return make_pair(qname, label);
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> 
  Connection::HttpServerDataBaseAndLabel(PicSOM *ps, const string& path,
					 bool test_db, bool test_lab,
					 bool test_obj) {
    string dbname = path;
    size_t p = dbname.find('?');
    if (p!=string::npos)
      dbname.erase(p);

    p = dbname.find('/');
    if (p==string::npos)
      return make_pair("", "");

    string label = dbname.substr(p+1);
    dbname.erase(p);

    if (test_db) {
      DataBase *db = ps->FindDataBase(dbname);
      if (!db)
	return make_pair("", "");

      if (test_lab) {
	int idx = db->LabelIndexGentle(label);

	if (idx<0)
	  return make_pair("", "");

	// in the following it was: db->SolveObjectPath(label) 
	if (test_obj && db->ObjectPathEvenExtract(idx)=="")
	  return make_pair("", "");
      }
    }

    return make_pair(dbname, label);
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> 
  Connection::HttpServerDataBaseAndClassFile(PicSOM *ps, const string& path,
					     bool test_db, bool test_cls) {
    string dbname = path;
    size_t p = dbname.find('?');
    if (p!=string::npos)
      dbname.erase(p);

    p = dbname.find('/');
    if (p==string::npos)
      return make_pair("", "");

    string cls = dbname.substr(p+1);
    dbname.erase(p);

    if (cls.find("classes/")!=0 || cls.size()<9)
      return make_pair("", "");

    cls.erase(0, 8);

    if (test_db) {
      DataBase *db = ps->FindDataBase(dbname);
      if (!db)
	return make_pair("", "");

      if (test_cls && !FileExists(db->ExpandPath("classes", cls)))
	return make_pair("", "");
    }

    return make_pair(dbname, cls);
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> 
  Connection::HttpServerDataBaseAndFile(PicSOM *ps, const string& path,
					bool test_db, bool test_file) {
    string dbname = path;
    size_t p = dbname.find('?');
    if (p!=string::npos)
      dbname.erase(p);

    p = dbname.find('/');
    if (p==string::npos)
      return make_pair("", "");

    string file = dbname.substr(p+1);
    dbname.erase(p);

    if (file=="")
      return make_pair("", "");

    if (test_db) {
      DataBase *db = ps->FindDataBase(dbname);
      if (!db)
	return make_pair("", "");

      if (test_file && !FileExists(db->ExpandPath(file)))
	return make_pair("", "");
    }

    return make_pair(dbname, file);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerDojo(const string& s) {
    string msg = "HttpServerDojo("+s+") : ";

    if (debug_http)
      cout << msg << endl;

    string f = s;
    size_t p = f.find('?');
    if (p!=string::npos)
      f.erase(p);
    
    f = Picsom()->RootDir()+f;
    if (debug_http)
      cout << msg << "-> <" << f << ">" << endl;
     
    if (!FileExists(f)) {
      if (debug_http)
	cout << msg << "file <"+f+"> not found" << endl;

      // return ShowError(msg+"file <"+f+"> not found");
      return HttpServerNotFound();
    }

    string data = FileToString(f);
    string otyp = "text/html";

    return HttpServerWriteOut(data, otyp, false, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerDoc(const string& s) {
    string msg = "HttpServerDoc("+s+") : ";

    if (debug_http)
      cout << msg << endl;

    string f = s;
    size_t p = f.find('?');
    if (p!=string::npos)
      f.erase(p);
    
    f.erase(0, 4); // should remove "/doc"
    if (FileExists(Picsom()->RootDir("doc.link")+f))
      f = Picsom()->RootDir("doc.link")+f;
    else
      f = Picsom()->RootDir("doc")+f;

    if (debug_http)
      cout << msg << "-> <" << f << ">" << endl;
     
    if (!FileExists(f)) {
      if (debug_http)
	cout << msg << "file <"+f+"> not found" << endl;

      // return ShowError(msg+"file <"+f+"> not found");
      return HttpServerNotFound();
    }

    string data = FileToString(f);
    string otyp = "text/html";

    return HttpServerWriteOut(data, otyp, false, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerDataBaseHtml(const DataBase *db,
                                          const string& s) {
    string msg = "HttpServerDataBaseHtml("+db->Name()+","+s+") : ";

    string f = db->ExpandPath("html", s);

    string data = FileToString(f);
    string otyp = "text/html";
    bool expnow = true;

    string log;

    list<string> ml { "TimeStamp", "Q:previous", "HttpListenConnection" };

    for (auto m=ml.begin(); m!=ml.end(); m++) {
      for (size_t v=0; v<2; v++) {
	string a = v ? "&lt;" : "<";
	string b = v ? "&gt;" : ">";
	string x = a+"!--%"+*m+"%--"+b;

	for (size_t p=0;;) {
	  p = data.find(x, p);
	  if (p==string::npos)
	    break;

	  string rep = "???";
	  if (*m=="TimeStamp")
	    rep = TimeStamp();
	  if (*m=="Q:previous") {
	    rep = "Q:none";
	    if (Picsom()->Nqueries())
	      rep = Picsom()->GetQuery(Picsom()->Nqueries()-1)->Identity();
	  }
	  if (*m=="HttpListenConnection")
	    rep = Picsom()->HttpListenConnection();

	  data.replace(p, x.size(), rep);
	  log += " "+x+"="+rep;
	  p++;
	}
      }
    }

    if (debug_http)
      WriteLog(msg+f+log);

    return HttpServerWriteOut(data, otyp, expnow, "", 200, true);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpServerProxy(const string& proxyhdr, const string& pre,
				   const string& url) {
    string msg = "HttpServerProxy("+proxyhdr+","+pre+","+url+") : ";
    if (DebugProxy()>1)
      WriteLog(msg+"starting");

    timespec_t time = TimeNow();

    list<pair<string,string> > hdrs;
    string datain, ctype;
    bool r = DownloadString(Picsom(), "GET", url, hdrs, "", datain, ctype);
    if (!r)
      return false;

    string urlbase = url;
    size_t q = 0;
    if (urlbase.find("http://")==0)
      q = 8;
    else if (urlbase.find("https://")==0)
      q = 9;
    q = urlbase.find("/", q);
    if (q!=string::npos)
      urlbase.erase(q);
    if (DebugProxy()>1)
      WriteLog(msg+"urlbase=\""+urlbase+"\"");

    string dataout = datain;
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("href=\"http://", q);
      if (q==string::npos)
	break;
      dataout.insert(q+6, proxyhdr);
    }
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("HREF=\"http://", q);
      if (q==string::npos)
	break;
      dataout.insert(q+6, proxyhdr+urlbase);
    }
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("href=\"https://", q);
      if (q==string::npos)
	break;
      dataout.insert(q+6, proxyhdr);
    }
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("HREF=\"https://", q);
      if (q==string::npos)
	break;
      dataout.insert(q+6, proxyhdr+urlbase);
    }
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("href=\"/", q);
      if (q==string::npos)
	break;
      if (dataout[q+7]=='/')
	continue;
      dataout.insert(q+6, proxyhdr+urlbase);
    }
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("HREF=\"/", q);
      if (q==string::npos)
	break;
      if (dataout[q+7]=='/')
	continue;
      dataout.insert(q+6, proxyhdr);
    }
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("src=\"/", q);
      if (q==string::npos)
	break;
      if (dataout[q+6]=='/')
	continue;
      dataout.insert(q+5, proxyhdr+urlbase);
    }
    for (size_t q=0; q!=string::npos; q++) {
      q = dataout.find("SRC=\"/", q);
      if (q==string::npos)
	break;
      if (dataout[q+6]=='/')
	continue;
      dataout.insert(q+5, proxyhdr);
    }

    bool expnow = true;
    r = HttpServerWriteOut(dataout, ctype, expnow, "");
    
    if (pre.find("/query/")==0) {
      string qname = pre.substr(7);
      Query *q = Picsom()->FindQuery(qname, this);
      if (q) {
	q->AddVisitedLink(url, ctype, datain.size(), time);
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  Connection *Connection::CreateUplink(PicSOM *p, const string& addr,
				       bool do_conn, bool do_ack) {
    if (addr=="") {
      ShowError("Connection::CreateUplink(): no host specified");
      return NULL;
    }

    const char *address = addr.c_str(), *leftend = address;
    char portno[100] = "", hostn[256] = "";

    Connection *c = new Connection(p);

    //separate hostname and port from "hostname:port" string
    for (int n=strlen(address); n>0; n--) {
      if (*(leftend+n)==':') {
	strcpy(portno, leftend+n+1);
	strncpy(hostn, address, n);
	break;
      }
    }

    c->type = conn_up;
    c->port = atoi(portno);
    c->serv_addr.sin_family = AF_INET;
    c->serv_addr.sin_port = htons(c->port);

    // get the server info from the DNS, returns a pointer to struct hostent 
    struct hostent myhost;
    const struct hostent* host = GetHostByName(hostn, myhost);

    if (!host) {
      ShowError("Connection::CreateUplink() : unknown server <"+
		string(hostn)+">");
      c->Close();
      delete c;
      return NULL;
    }

    c->hostname = addr;

    // copy the server address into the server address structure
    memcpy(&c->serv_addr.sin_addr, host->h_addr, host->h_length);

    if (do_conn && !c->ConnectUplink(xml_language, "", do_ack)) {
      c->Close();
      delete c;
      return NULL;
    }

    c->Notify(true, NULL, true);

    return c;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ConnectUplink(connection_protocol proto,
				 const string& msg, bool ack) {
    string hdr = "Connection::ConnectUplink() : ";
    protocol = proto;

    socklen_t serv_addrlen = sizeof(serv_addr);

    // create a socket stream using TCP for connection
    int sfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 

    if (::connect(sfd, (struct sockaddr*)&serv_addr, serv_addrlen))
      return ShowError(hdr+"connect() failed");

    // int one = 1;
    // if (setsockopt(sfd, SOL_TCP, TCP_NODELAY, &one, sizeof(one)))
    //   return ShowError(hdr+"setsockopt(TCP_NODELAY) failed");

    wfd = rfd = sfd;

    if (msg!="" && write(Wfd(), msg.c_str(), msg.size())==-1)
      return ShowError(hdr+"write() failed, errno="+ToStr(errno));

    map<string,string> extra;
    if (ack && !SendAcknowledgment(extra))
      return ShowError(hdr+"SendAcknowledgment() #1 failed");

    bool html = false;
    bool read_ok = ReadAndParseXML(html, false);
    if (!read_ok || html)
      return ShowError(hdr+"ReadAndParseXML() #1 failed");

    if (!HasXMLdoc()) {
      Notify(true, "non-XML input socket ???", true);
      return false;
    }

    xmlDocPtr xmldoc = XMLdoc();
    XmlDom xml(xmldoc);
    XmlDom root = xml.Root();
    if (root.NodeName()=="acknowledgment") {
      XmlDom mpiconnect = root.FirstElementChild("mpiconnect");
      string addr = mpiconnect.FirstChildContent();
      if (addr!="") {
#ifdef PICSOM_USE_MPI
	ConvertToMPI(addr, true);
	CloseFDs();
	if (!SendAcknowledgment(map<string,string>()))
	  return ShowError(hdr+"SendAcknowledgment() #2 failed");
	read_ok = ReadAndParseXML(html, false);
	if (!read_ok)
	  return ShowError(hdr+"ReadAndParseXML() #2 failed");
#endif // PICSOM_USE_MPI
      }
    }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_MRML   

bool Connection::ReOpenUplink(const char *dumpp) {
  string dumpstr(dumpp?dumpp:"");

  if (!ConnectUplink(mrml_language, dumpstr, false))
    return ShowError("Connection::ReOpenUplink() : ConnectUplink() failed"); 

  // tarvitaanko ??
  //LaunchThread();

  //tarvitaanko???
  if (close(wfd)==0)
    printf("Socket closed!\n");
  else {
    printf("Socket not closed, trying to close socket 2nd time! \n");
    if (close(wfd)==0)
      printf("Socket closed!\n");
    else
      printf("Socket still open!\n");
  }

  return true;
}

#endif //USE_MRML
  
  /////////////////////////////////////////////////////////////////////////////

  Connection *Connection::CreateHttpClient(PicSOM *p, const string& addr,
                                           bool ssl, int port) {
    bool debug = debug_sockets;

    string msg = "Connection::CreateHttpClient("+addr+","+ToStr(ssl)+","+
      ToStr(port)+") : ";

    int timeoutsec = 10;

    if (addr=="") {
      ShowError(msg+"no host specified");
      return NULL;
    }

    // get the server info from the DNS, returns a pointer to struct hostent 
    // struct hostent myhost;
    // const struct hostent* host = GetHostByName(addr, myhost);
    // if (!host) {
    //   ShowError(msg+"unknown server");
    //   return NULL;
    // }

    struct addrinfo *ai = NULL;
    int rrr = getaddrinfo(addr.c_str(), NULL, NULL, &ai);
    if (rrr || !ai)  {
      ShowError(msg+"unknown server");
      return NULL;
    }

    if (port==0)
      port = ssl ? 443 : 80;

    Connection *c = new Connection(p);
    c->type = conn_http_client;
    c->Identity("http"+string(ssl?"s":"")+"://"+addr+":"+ToStr(port));
    c->hostname = addr;
    c->port = port;
    c->serv_addr.sin_family = AF_INET;
    c->serv_addr.sin_port = htons(c->port);

    // copy the server address into the server address structure
    // memcpy(&c->serv_addr.sin_addr, host->h_addr, host->h_length);
    memcpy(&c->serv_addr.sin_addr,
	   &((struct sockaddr_in*)(ai->ai_addr))->sin_addr,
	   ai->ai_addrlen);
    freeaddrinfo(ai);

    socklen_t serv_addrlen = sizeof(c->serv_addr);

    // create a socket stream using TCP for connection
    int sfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
    if (debug)
      cout << "socket() returns sfd=" << sfd << endl;
    c->wfd = c->rfd = sfd;

// #ifdef SOCK_NONBLOCK
//     int type_extra = SOCK_NONBLOCK;
// #else
//     int type_extra = 0;  // obs!
// #endif // SOCK_NONBLOCK
//     int sfd = ::socket(AF_INET, SOCK_STREAM|type_extra, IPPROTO_TCP); 

    int fd_flags = fcntl(sfd, F_GETFL, 0);
    fd_flags |= O_NONBLOCK;
    int r = fcntl(sfd, F_SETFL, fd_flags);

    if (r) {
      ShowError(msg+"fcntl() failed (1)");
      c->Close();
      delete c;
      return NULL;
    }

    r = ::connect(sfd, (struct sockaddr*)&c->serv_addr, serv_addrlen);
    if (debug)
      cout << "connect() returns r=" << r << " errno=" << errno << endl;

#ifndef HAVE_WINSOCK2_H
    if (r && errno!=EINPROGRESS) {
      ShowError(msg+"connect() failed with errno="+ToStr(errno));
      c->Close();
      delete c;
      return NULL;
    }
#endif // HAVE_WINSOCK2_H

    if (r) {
      for (int loop=0; loop<1000*timeoutsec; loop++) {
	fd_set set;
	FD_ZERO(&set);
	FD_SET(sfd, &set);
	timeval wait;
	wait.tv_sec  = 0;
	wait.tv_usec = 1000; // 1 ms
	int x = select(FD_SETSIZE, NULL, &set, NULL, &wait);
	bool isset = FD_ISSET(sfd, &set);
	if (debug)
	  cout << "  loop=" << loop << " select() x=" << x
	       << " isset=" << isset << endl;
	if (isset) {
	  int myerrno = -1;
	  socklen_t len = sizeof(int), lenwas = len;
	  x = getsockopt(sfd, SOL_SOCKET, SO_ERROR, (char*)&myerrno, &len);
	  if (debug)
	    cout << "  getsockopt() x=" << x << " myerrno=" << myerrno
		 << " len=" << len << " lenwas=" << lenwas << endl;

	  if (myerrno) {
	    ShowError(msg+"connection open error errno="+ToStr(myerrno));
	    c->Close();
	    delete c;
	    return NULL;
	  }
	  r = 0;
	  break;
	}
      }
    }

    if (r) {
      ShowError(msg+"connection timeout");
      c->Close();
      delete c;
      return NULL;
    }

    bool connectok = false;
    for (size_t x=0; !connectok && int(x)<1000*timeoutsec; x++) {
      if (!c->State(1)) {
	timespec_t snap = { 0, 1000000 };
	nanosleep(&snap, NULL);

      } else
	connectok = true;
    }

    if (!connectok) {
      ShowError(msg+"connect() failed");
      c->Close();
      delete c;
      return NULL;
    }

    // int zero = 0;
    //int r = setsockopt(sfd, SOL_SOCKET, O_NONBLOCK, &zero, sizeof(zero));

    fd_flags = fcntl(sfd, F_GETFL, 0);
    fd_flags &= ~O_NONBLOCK;
    r = fcntl(sfd, F_SETFL, fd_flags);

    if (r) {
      ShowError(msg+"fcntl() failed (2)");
      c->Close();
      delete c;
      return NULL;
    }

#ifdef HAVE_OPENSSL_SSL_H
    if (ssl) {
      c->ssl = SSL_new(ssl_ctx);
      if (!c->ssl)
	ShowError(msg+"SSL_new() failed");
      else if (debug_http) {
	cout << msg << "SSL_new() successful" << endl;
	c->ssl->debug = 1;
      }

      SSL_set_connect_state(c->ssl);

      if (!SSL_set_fd(c->ssl, sfd))
	ShowError(msg+"SSL_set_fd() failed");
      else if (debug_http)
	cout << msg << "SSL_set_fd() successful" << endl;

      int r = SSL_connect(c->ssl);
      if (r!=1)
	ShowError(msg+"SSL_connect() failed, r="+ToStr(r));
      else if (debug_http) {
	cout << msg << "SSL_connect() successful" << endl;

	printf("SSL connection using %s\n", SSL_get_cipher(c->ssl));
	X509 *server_cert = SSL_get_peer_certificate(c->ssl);
	printf("Server certificate:\n");
	char *str = X509_NAME_oneline(X509_get_subject_name (server_cert),0,0);
	printf ("\t subject: %s\n", str);
	OPENSSL_free(str);
	str = X509_NAME_oneline(X509_get_issuer_name(server_cert),0,0);
	printf("\t issuer: %s\n", str);
	OPENSSL_free(str);
	X509_free(server_cert);
      }

      r = SSL_do_handshake(c->ssl);
      if (r!=1)
	ShowError(msg+"SSL_do_handshake() failed, r="+ToStr(r));
      else if (debug_http)
	cout << msg << "SSL_do_handshake() successful" << endl;

      if (c->ssl->state!=SSL_ST_OK)
	ShowError(msg+"c->ssl->state!=SSL_ST_OK : "+ToStr(c->ssl->state));
      else if (debug_http)
	cout << msg << "SSL_ST_OK" << endl;
    }
#endif // HAVE_OPENSSL_SSL_H

    c->Notify(true, NULL, true);

    return c;
  }

///////////////////////////////////////////////////////////////////////////////

#if PICSOM_USE_CSOAP

  /////////////////////////////////////////////////////////////////////////////

  imagedata Connection::SoapClientGetFrame(const string& urn,
					   const string& method) {
    imagedata img;

    SoapCtx *ctx = NULL, *ctx2 = NULL;
    herror_t err = soap_ctx_new_with_method(urn.c_str(), method.c_str(), &ctx);
    if (err != H_OK) {
      log_error4("[%d] %s():%s ", herror_code(err), herror_func(err),
		 herror_message(err));
      herror_release(err);
      return img;
    }

    err = soap_client_invoke(ctx, &ctx2, soap_client_url.c_str(), "");
    if (!ctx2) {
      ShowError("Connection::SoapClientGetFrame() : ctx2==NULL");
      return imagedata();
    }
    
    xmlNodePtr fault = soap_env_get_fault(ctx2->env);
    if (fault)
      DumpXML(cout, false, ctx2->env->root->doc, "", "", false, false);
    // soap_xml_doc_print(ctx2->env->root->doc);

    else if (ctx2->attachments) {
      cout << "FILE <" << ctx2->attachments->parts->filename << "]" << endl;
      imagefile ifile(ctx2->attachments->parts->filename);
      img = ifile.frame(0);

    } else {
      printf("No attachments!");
      DumpXML(cout, false, ctx2->env->root->doc, "", "", false, false);
      // soap_xml_doc_print(ctx2->env->root->doc);
    }

    soap_ctx_free(ctx2);
    soap_ctx_free(ctx);

    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  Connection *Connection::CreateSoapClient(PicSOM *p, const string& url) {
    if (url=="") {
      ShowError("Connection::CreateSoapClient(): no url specified");
      return NULL;
    }

    int sport  = 0;
    size_t col = url.find(':');
    if (col!=string::npos)
      sport = atoi(url.substr(col+1).c_str());

    Connection *c = new Connection(p);
    c->type = conn_soap_client;
    c->port = sport;
    c->soap_client_url = url;
    c->Identity("soap-client-"+url);
    c->Notify(true, NULL, true);

    char *argv[] = { NULL };
    int argc = 0;
    /*herror_t err =*/ soap_client_init_args(argc, argv); // the only instance?

    return c;
  }

#else
  Connection *Connection::CreateSoapClient(PicSOM *, const string&) {
    ShowError("Connection::CreateSoapClient() not implemented");
    return NULL;
  }
#endif // PICSOM_USE_CSOAP

  /////////////////////////////////////////////////////////////////////////////

#if PICSOM_USE_CSOAP && defined(PICSOM_USE_PTHREADS)

  herror_t Connection::SoapServer_sayHello(SoapCtx *req, SoapCtx *res) {
    herror_t err = soap_env_new_with_response(req->env, &res->env);
    if (err!=H_OK) {
      cout << "SoapServer_sayHello() : failure" << endl;
      return err;
    }

    xmlNodePtr method = soap_env_get_method(req->env);
    xmlNodePtr node   = soap_xml_get_children(method);

    while (node) {
      char *name = (char *) xmlNodeListGetString(node->doc,
						 node->xmlChildrenNode, 1);
      soap_env_add_itemf(res->env, "xsd:string", "echo", "Hello '%s'", name);
      node = soap_xml_get_next(node);
      xmlFree(name);
    }

    cout << "SoapServer_sayHello() : success" << endl;

    return H_OK;
  }

  /////////////////////////////////////////////////////////////////////////////

  herror_t Connection::SoapServer_execute(SoapCtx *req, SoapCtx *res) {
    string msg = "Connection::SoapServer_execute() : ";

    timespec stime = TimeNow();
    if (debug_soap&32)
      cout << endl << endl << TimeStamp()
	   << msg << "entered" << endl << endl << endl;

    static RwLock lock, countlock;
    if (soap_server_single)
      lock.LockWrite();

    PicSOM *picsom = soap_server_instance->Picsom();

    picsom->MutexLock();
    picsom->Tic("SoapServer_execute");

    if (debug_soap&2) {
      XmlDom copy = XmlDom::Copy(req->env->root->doc);
      copy.RemoveBlanks();
      DumpXML(cout, false, copy.doc, "SoapServer_execute", "", true, false);
      copy.DeleteDoc();
      // soap_xml_doc_print(req->env->root->doc);
    }

    xmlNodePtr method = soap_env_get_method(req->env);
    xmlNodePtr node   = soap_xml_get_children(method);

    vector<string> vs, upload, tmpdirs, tmpfiles;

    bool is_insert = false, is_ajax = false;
    string mode;

    while (node) {
      bool add = true;

      string key = NodeName(node);
      string val = NodeChildContent(node);
      if (debug_soap&4)
	cout << "key=[" << key << "] val=[" << val << "]" << endl;

      if (key=="element") { // python ZSI
	node = soap_xml_get_children(node);
	continue;
      }

      if (key=="analyse") {
        mode = val;
        if (val=="insert")
          is_insert = true;
        if (val=="ajax")
          is_ajax = true;
      }

      if (key=="script") {
        vector<string> ar = SplitInSpaces(val);
        vs.insert(vs.end(), ar.begin(), ar.end());
        add = false;
      }

      if (key=="upload") {
	upload.push_back(val);
        add = false;
      }

      if (add)
	vs.push_back(key+"="+val);

      node = soap_xml_get_next(node);
    }

    if ((is_insert||is_ajax) && !upload.empty() && req->attachments) {
      vector<string>::iterator i = vs.begin();
      while (i!=vs.end())
	if (i->find("args=")==0)
	  break;
	else
	  i++;

      if (i==vs.end())
	i = vs.insert(i, "args=");

      vector<string>::iterator u = upload.begin();
      for (part_t *part=req->attachments->parts; part; part = part->next) {
	int size = -1;
	string fname = part->filename;
	if (u!=upload.end()) {
	  string xname = *u;
	  size_t slash = xname.rfind('/');
	  if (slash!=string::npos)
	    xname.erase(0, slash+1);
	  string tmpname = picsom->TempDirPersonal();

          static int count = 0;
	  countlock.LockWrite();
	  tmpname += "/"+ToStr(count++);
	  countlock.UnlockWrite();

          picsom->MkDir(tmpname.c_str(), 0700);
          tmpdirs.push_back(tmpname);
	  tmpname += "/"+xname;

	  string s = FileToString(fname);
	  bool ok = s!="" ? StringToFile(s, tmpname) : false;
	  if (ok) {
	    size = s.size();
	    fname = tmpname;
	    tmpfiles.push_back(tmpname);
	  } else
	    size = -2;

	  u++;
	}

	*i += string((*i)[i->size()-1]=='='?"":" ")+fname+"//"
	  +part->content_type;

	if (debug_soap&4)
	  cout << "part->filename=[" << part->filename
	       << "] part->content_type=[" << part->content_type
	       << "] fname=[" << fname << "] size=" << size << endl;
      }

      if (debug_soap&4)
	cout << "args=[" << *i << "]" << endl;
    }

    herror_t err = soap_env_new_with_response(req->env, &res->env);
    if (err!=H_OK) {
      cout << "SoapServer_execute() : failure" << endl;
      return err;
    }

    //Query    *q = soap_server_instance->NewQuery();
    Query    *q = new Query(picsom);

    if (req->http->in->type!=HTTP_TRANSFER_FILE)
      q->FirstClientAddress(inet_ntoa(req->http->in->sock->addr.sin_addr));

    Analysis *a = new Analysis(picsom, NULL, q, vs);
    a->HasCin(false);
    q->SetAnalysis(a);

    string tname = picsom->RegisterThread(pthread_self(), gettid(),
                                          "soapexecute", mode, a, NULL);

    // picsom->AppendAnalysis(a);

    Analysis::analyse_result ares = a->Analyse();

    // cout << TimeStamp() << msg << "A" << endl;

    if (a->XmlResultIsError())
      soap_server_instance->WriteLog("SoapServer_execute() analyse="+mode
                                     +" failed with \""
                                     +a->XmlResultErrorText()+"\"");
    else if (ares.errored())
      picsom->ShowError("Analysis::Analyse() failed in SoapServer_execute()");

    xmlNodePtr np = soap_env_add_item(res->env, NULL, //"picsom:result",
				      "result", NULL);
    XmlDom xml = a->XmlResult();
    if (xml.DocOK()) {
      if (debug_soap&1)
	DumpXML(cout, true, xml.doc, "Analysis::xml_result", "", true, false);

    } else {

      /// this functionality should be somewhere else...

      DataBase *db = a->GetDataBase();
      pair<XmlDom,XmlDom> docroot = XMLdocroot("xmlresult", false);
      xml = docroot.first;
      XmlDom objlist = docroot.second.Element("objectlist");
      bool minim = q->IncludeMinimal();
      for (size_t i=0; i<q->NnewObjects()&&i<q->MaxQuestions(); i++) {
	const Object& ni = q->NewObject(i);
	if (debug_soap&1)
	  cout << "i=" << i << " " << ni.Label() << " " << ni.Value() << endl;
	pair<XmlDom,XmlDom> docroot = XMLdocroot("extrainfo", false);
	XmlDom info = docroot.second;
	info.Element("score", ni.Value());
	if (!minim)
	  ni.AddToXMLstageinfo_detail(info, q);
        if (db) {
	  bool oinfo = false;
	  bool full = false;  // used to be true until 2012-08-20
          db->AddToXMLobjectinfo(objlist, ni.Label(), q, minim, true, oinfo,
				 full, &info, "");
	}

        docroot.first.DeleteDoc();  // added 2009-09-14
      }

      if (debug_soap&1)
	DumpXML(cout, true, xml.doc, "SoapServer::xml_result", "",
		true, false);
    }

    // cout << TimeStamp() << msg << "B" << endl;

    XmlDom root = xml.Root();
    AddSOAP_ENC_arrayType(root);

    xmlAddChild(np, xmlCopyNode(root.node, 1));

    if (debug_soap&2) {
      XmlDom copy = XmlDom::Copy(res->env->root->doc);
      copy.RemoveBlanks();
      DumpXML(cout, true, copy.doc, "SoapServer_execute", "", true, false);
      copy.DeleteDoc();
      // soap_xml_doc_print(res->env->root->doc);
    }

    if (!(debug_soap&8)) {
      for (size_t ti=0; ti<tmpfiles.size(); ti++)
	Unlink(tmpfiles[ti]);

      for (size_t ti=0; ti<tmpdirs.size(); ti++)
	RmDir(tmpdirs[ti]);
    }

    // cout << TimeStamp() << msg << "C" << endl;

    picsom->Tac("SoapServer_execute");
    picsom->PossiblyShowDebugInformation("SoapServer_execute ending");
    picsom->UnregisterThread(tname);

    delete a; //picsom->RemoveAnalysis(a);

    if (!picsom->FindQuery(q->Root()))
      delete q;

    picsom->MutexUnlock();

    if (soap_server_single)
      lock.UnlockWrite();

    timespec etime = TimeNow();
    if (debug_soap&32) {
      cout << endl << endl << TimeStamp() << msg << "returning : " 
	   << Analysis::ElapsedTime(stime, etime).second
      << endl << endl << endl;
    }

    return H_OK;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Connection::AddSOAP_ENC_arrayType(XmlDom& xml) {
    string n = xml.NodeName();
    // cout << n << endl;
    if (n=="databaselist" || n=="viewlist" || n =="featurelist" ||
	n=="objectlist")
      xml.AddSOAP_ENC_arrayType(true);

    XmlDom e = xml.FirstElementChild();
    while (e) {
      AddSOAP_ENC_arrayType(e);
      e = e.NextElement();
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void *Connection::SoapServerThread(void *cp) {
    Connection *c = (Connection*)cp;

    c->HasOwnThread(true); // this is now set in both threads

    c->Picsom()->RegisterThread(c->Thread(), gettid(),
                                "soapserver", "server", NULL, NULL);

    soap_server_instance = (Connection*)c;

    soap_server_run();

    log_info1("shutting down\n");
    soap_server_destroy();

    soap_server_instance = NULL;

    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::SoapRouterRegisterService(SoapRouter *rter,
					     SoapServiceFunc f,
					     const string& method,
					     const string& urn) {
    soap_router_register_service(rter, f, method.c_str(), urn.c_str());
    WriteLog("SOAP service registered method="+method+" urn="+urn);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::SoapServerRegisterRouter(SoapRouter *rter,
					    const string& url) {
    soap_server_register_router(rter, url.c_str());
    WriteLog("SOAP router registered url="+url);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  Connection *Connection::CreateSoapServer(PicSOM *p, int sport) {
    string hdr = "Connection::CreateSoapServer() : ";
    if (!sport) {
      ShowError(hdr+"no port specified");
      return NULL;
    }

    stringstream portsstr;
    portsstr << sport;
    string portstr = portsstr.str();

    string soapurl = "http://"+p->HostName(true)+":"+portstr+"/picsom-soap";

    Connection *c = new Connection(p);
    c->type = conn_soap_server;
    c->port = sport;
    c->Identity(soapurl);
    c->Notify(true, NULL, true);

    // HLOG_VERBOSE>HLOG_DEBUG>HLOG_INFO>HLOG_WARN>HLOG_ERROR>HLOG_FATAL
    hlog_set_level(debug_soap&16 ? HLOG_VERBOSE : HLOG_INFO);

    char *argv[3];
    argv[0] = strdup(NHTTPD_ARG_PORT);
    argv[1] = strdup((char*)portstr.c_str());
    argv[2] = NULL;
    herror_t err = soap_server_init_args(2, argv); // the only instance?;
    free(argv[0]);
    free(argv[1]);

    if (err!=H_OK) {
      stringstream emsg;
      emsg << "func=" << herror_func(err) << " message=" << herror_message(err)
	   << " code=" << herror_code(err);
      herror_release(err);
      ShowError(hdr+"soap_server_init_args() "+emsg.str());
      return NULL;
    }

    SoapRouter *r1 = soap_router_new();
    if (!r1) {
      herror_release(err);
      soap_server_destroy();
      ShowError(hdr+"soap_router_new() 1 failed");
      return NULL;
    }

    c->SoapRouterRegisterService(r1, SoapServer_sayHello,
				 "sayHello", "urn:examples");
    c->SoapServerRegisterRouter(r1, "/csoapserver");

    SoapRouter *r2 = soap_router_new();
    if (!r2) {
      herror_release(err);
      soap_server_destroy();
      ShowError(hdr+"soap_router_new() 2 failed");
      return NULL;
    }

    c->SoapRouterRegisterService(r2, SoapServer_execute,
				 "execute", "urn:picsom");
    c->SoapServerRegisterRouter(r2, "/picsom-soap");

    if (c->HasOwnThread()) {
      ShowError(hdr+"connection already has own thread");
      return NULL;
    }
      
    int r = pthread_create(&c->Thread(), NULL, SoapServerThread, c);
    if (r) {
      ShowError(hdr+"pthread_create() failed");
      return NULL;
    }

    c->HasOwnThread(true); // this is now set in both threads

    return c;
  }
#else
  Connection *Connection::CreateSoapServer(PicSOM*, int) {
    ShowError("Connection::CreateSoapServer() not implemented");
    return NULL;
  }
#endif // PICSOM_USE_CSOAP

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ExtractIdentityAndSelectProtocol() {
    xmlDocPtr doc = XMLdoc();
    xml_version = mrml_version = pm_version = 0;

    char idstr[1000] = "";

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlDtdPtr dtd = xmlGetIntSubset(doc);

    if (dtd) {
      if (dtd->ExternalID) {
	const char *eid = (const char*)dtd->ExternalID;
	float f = 0;
	if (sscanf(eid, "-//PicSOM//DTD PicSOM %f", &f)==1) {
	  xml_version = f;
	  sprintf(idstr+strlen(idstr), "XML/%g", f);
	}

      } else if (dtd->SystemID) { 
	mrml_version = 1.0;
	protocol = mrml_language;
	sprintf(idstr+strlen(idstr), "MRML/%g", mrml_version);
      }
    }

    // make sure that MRML gets selected even without DTD
    if (root->name && !xmlStrcmp(root->name, (XMLS)"mrml")) {
      mrml_version = 1.0;
      protocol = mrml_language;
      sprintf(idstr+strlen(idstr), "MRML/%g", mrml_version);
    }

    bool is_connection_pm = false;
    float v = 0;
    int pid = 0;

    if (protocol == xml_language) {
      if (!xmlStrcmp(root->name, (XMLS)"acknowledgment"))
	for (xmlNodePtr node = root->children; node; node = node->next)
	  if (node->children && node->children->content) {
	    if (!xmlStrcmp(node->name, (XMLS)"version"))
	      v = atof((const char*)node->children->content);
	    else if (!xmlStrcmp(node->name, (XMLS)"pid"))
	      pid = atoi((const char*)node->children->content);
	    else if (!xmlStrcmp(node->name, (XMLS)"name"))
	      is_connection_pm = !xmlStrcmp(node->children->content,
					    (XMLS)"Connection.pm");
	    else if (!xmlStrcmp(node->name, (XMLS)"mpi"))
	      can_do_mpi = IsAffirmative((const char*)node->children->content);
	  }
    
      if (is_connection_pm) {
	pm_version = v;
	sprintf(idstr+strlen(idstr), " Connection.pm/%g", v);
	if (pid)
	  sprintf(idstr+strlen(idstr), " pid=%d", pid);
      }
    }

    else if (protocol == mrml_language) {
      if (root->name)
	if (!xmlStrcmp(root->name, (XMLS)"mrml"))
	  if (root->properties && root->properties->name &&
	      root->properties->children &&
	      root->properties->children->content)
	    if (!xmlStrcmp(root->properties->name, (XMLS)"session-id")) {
	      sprintf(idstr+strlen(idstr), " %s %s=%s",root->name,
		      root->properties->name,
		      root->properties->children->content);
	      // if this is the 1st time to connect try to print more info
	      if (root->children && root->children->next &&
		  root->children->next->name) {
		if (!xmlStrcmp(root->children->next->name,
			       (XMLS)"open-session")) {
		  if (root->children->next->properties &&
		      root->children->next->properties->name &&
		      root->children->next->properties->children &&
		      root->children->next->properties->children->content)
		    if (!xmlStrcmp(root->children->next->properties->name, 
				   (XMLS)"user-name")) {
		      //open-session username=default-user-name
		      sprintf(idstr+strlen(idstr), " %s %s=%s",
			      root->children->next->name,
			      root->children->next->properties->name,
			      root->children->next->
			      properties->children->content);
		    }
		  if (root->children->next->properties->next)
		    if (root->children->next->properties->next->name)
		      if (root->children->next->properties->next->children)
			if (root->children->next->
			    properties->next->children->content) 
			  if (!xmlStrcmp(root->children->next->
					 properties->next->name, 
					 (XMLS)"session-name")) {
			    //session-name=default-session-name
			    sprintf(idstr+strlen(idstr), " %s=%s",
				    root->children->next->
				    properties->next->name,
				    root->children->next->
				    properties->next->children->content);
			  }
		} else {
		  sprintf(idstr+strlen(idstr), " %s ",
			  root->children->next->name);
		}
	      }
	    }
    }
  
    Identity(idstr);

    servant.Pid(pid);

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

// bool Connection::ExtractIdentity(xmlDocPtr doc) {
//   xml_version = pm_version = 0;

//   char idstr[1000] = "";

//   xmlNodePtr root = xmlDocGetRootElement(doc);
//   xmlDtdPtr dtd = xmlGetIntSubset(doc);
//   if (dtd) {
//     const char *eid = (const char*)dtd->ExternalID;
//     float f = 0;
//     if (sscanf(eid, "-//PicSOM//DTD PicSOM %f", &f)==1) {
//       xml_version = f;
//       sprintf(idstr+strlen(idstr), "XML/%g", f);
//     }
//   }

//   bool is_connection_pm = false;
//   float v = 0;
//   int pid = 0;

//   if (!xmlStrcmp(root->name, (XMLS)"handshake"))
//     for (xmlNodePtr node = root->children; node; node = node->next)
//       if (node->children && node->children->content)
// 	if (!xmlStrcmp(node->name, (XMLS)"version"))
// 	  v = atof((const char*)node->children->content);
// 	else if (!xmlStrcmp(node->name, (XMLS)"pid"))
// 	  pid = atoi((const char*)node->children->content);
// 	else if (!xmlStrcmp(node->name, (XMLS)"name"))
// 	  is_connection_pm = !xmlStrcmp(node->children->content,
// 					(XMLS)"Connection.pm");
  
//   if (is_connection_pm) {
//     pm_version = v;
//     sprintf(idstr+strlen(idstr), " Connection.pm/%g", v);
//     if (pid)
//       sprintf(idstr+strlen(idstr), " pid=%d", pid);
//   }

//   Identity(idstr);

//   servant.Pid(pid);

//   return true;
// }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef USE_MRML  
///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLCommunication() {
  xmlDocPtr doc = XMLdoc();

  Query *oldq = NULL;
  session_id = "";
  xmlNodePtr unknown = NULL;
  xmlNodePtr root = xmlDocGetRootElement(doc);

  xmlNsPtr   ns2   = NULL;
  xmlDocPtr  doc2  = xmlNewDoc((XMLS)"1.0");
  xmlNodePtr root2 = NewDocNode(doc2, ns2, "mrml");
  xmlDocSetRootElement(doc2, root2);

  if(root->name)
  if(xmlStrcmp(root->name, (XMLS)"mrml")) {
      return ShowError("Not MRML content!");
  }

  if(root->properties && root->properties->name) {
  if(!xmlStrcmp(root->properties->name, (XMLS)"session-id")) {

    // read session-id (whatever it is)
    if(root->properties->children && root->properties->children->content)
      session_id =(string)(const char *)root->properties->children->content;
      
    // try to find already created Query based on given session ID.
    if( query->SaneIdentity( (const char *)getSessionID() ) )
      oldq = Picsom()->FindQuery((const char *)getSessionID(), this);      
    if( oldq ) { // if query was found	
      // create child of original query
      oldq = oldq->FindLastChild();
	
      NewQuery();
      QueryOwn(false);

      query->Parent(oldq);
      query->SetIdentity();

      // should be se to some reasonable value later
      query->FirstClientAddress("1.2.3.4");
      query->CopySome(*oldq, true, false, false);

      query->SetName( oldq->GetName() );
      query->FirstClient().UserPassword(oldq->FirstClient().UserPassword());
      query->PerMapObjects(100);

      // add session-id to the reply (session-id="Qxxx:yyy:zzz:wwww")
      xmlNewProp(root2, (XMLS)"session-id", getSessionID());      
      // add current time to the reply
      MRMLAddTime(root2, ns2);
    }
    else session_id = "";
    // else we don't have any session-id yet, so we open one IF we have to
  }
  }

  if(root->children)
    unknown = root->children;

  while(unknown) {
    if(unknown->name) {

      if(!xmlStrcmp(unknown->name, (XMLS)"get-server-properties")) {
	if( !MRMLGetServerProperties( /* root2, ns2 */ ) )
	  return ShowError("Connection:MRMLGetServerProperties() failed!");
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"get-configuration")) {
	if( !MRMLGetConfiguration( root2, ns2 ) )
	  return ShowError("Connection:MRMLGetConfiguration() failed!");
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"get-sessions")) {
	if( !MRMLGetSessions( unknown, root2, ns2 ) )
	  return ShowError("Connection:MRMLGetSessions() failed!");
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"get-collections")) {
	if( !MRMLGetCollections( root2, ns2 ) )
	  return ShowError("Connection:MRMLGetCollections() failed!");
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"get-algorithms")) {
	if( !MRMLGetAlgorithms( unknown, root2, ns2 ) )
	  return ShowError("Connection:MRMLGetAlgorithms() failed!");
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"open-session")) {
	printf("Entering OpenSession!\n");
	if( !MRMLOpenSession( unknown, root2, ns2 ) )
	  return ShowError("Connection:MRMLOpenSession() failed!");
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"configure-session")) {
	if( !MRMLConfigureSession( unknown, root2, ns2 ) )
	  return ShowError("Connection:MRMLConfigureSession() failed!"); 
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"rename-session")) {
	if( !MRMLRenameSession( unknown, root2, ns2 ) )
	  return ShowError("Connection:MRMLRenameSession() failed!");
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"query-step")) {
	if( !MRMLQueryStep(unknown, root2, ns2) ) {
	  return ShowError("Connection::MRMLQueryStep failed!");
	}
      }	    

      else if(!xmlStrcmp(unknown->name, (XMLS)"user-data")) {
	if( !MRMLUserData(unknown, root2, ns2) ) {
	  return ShowError("Connection::MRMLUserData failed!");
	}
      }	

      else if(!xmlStrcmp(unknown->name, (XMLS)"begin-transaction")) {
	if( !MRMLBeginTransaction(unknown, root2, ns2) ) {
	  return ShowError("Connection::MRMLBeginTransaction failed!");
	}
      }	

      else if(!xmlStrcmp(unknown->name, (XMLS)"end-transaction")) {
	if( !MRMLEndTransaction(unknown, root2, ns2) ) {
	  return ShowError("Connection::MRMLEndTransaction failed!");
	}
      }	

      else if(!xmlStrcmp(unknown->name, (XMLS)"close-session")) {
	if( !MRMLCloseSession(unknown, root2, ns2) ) {
	  return ShowError("Connection::MRMLCloseSession failed!");
	}
      }

      else if(!xmlStrcmp(unknown->name, (XMLS)"text")) {
	// just do nothing
	//printf("%s",(const char *)unknown->content);
      }
      else {
	return ShowError("Unrecognized MRML command: ",
			 (const char *)unknown->name,
			 " in Connection::MRMLCommunication()");
      }    
    }//if(unknown->name)
    else // unknown has no unknown->name
      return ShowError(" Connection::MRMLCommunication() Empty MRML content.");

    unknown = unknown->next;
  }//while(unknown)


  // add current time to the reply
  MRMLAddTime(root2, ns2);
    
  const char *dump = XML2StringM(doc2, true);
  printf("%s\n",dump);
  xmlFreeDoc(doc2); // added 250203 by mrk

  if (write(Wfd(), dump, strlen(dump)) == -1)
    return ShowError("Write failed!\n");
  delete dump;

  if (fflush(0) != 0) 
    cout << "Flushing of socket failed!" << endl;

  //closing down socket because of the way howto communicate with Charmer
  if (close(Wfd()) != 0 ) {
    cout << "Socket not closed, trying to close socket 2nd time..." << endl;

    if( close(Wfd() == 0))
      cout << "Socket closed!" << endl;
    else 
      return ShowError("Socket still open!\n");
  }
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Connection::MRMLGetServerProperties(/*xmlNodePtr rt2, 
						 xmlNsPtr ns*/) {
  // server-properties is empty in this version of mrml
  // If new version of mrml specifies new properties they should be 
  // implemented below as demonstrated below (in comments)
  /*
  xmlNodePtr n = xmlNewChild(rt2, ns, (XMLS)"server-properties", NULL);
  xmlNewProp(n, (XMLS)"new-property-attribute-name", 
             (XMLS)"new-property-attribute-value");
  if(n==NULL) return ShowError("Connection::MRMLGetServerProperties() failed");
  */
  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool Connection::MRMLGetConfiguration(xmlNodePtr rt2, 
						 xmlNsPtr ns) {
  // empty configuration-description content == PicSOM uses standard MRML
  xmlNodePtr n = xmlNewChild(rt2, ns, (XMLS)"configuration-description", NULL);
  if(n==NULL) return false; 
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLGetSessions(xmlNodePtr node, xmlNodePtr root2, 
					 xmlNsPtr ns2) {
  XMLS username=NULL, password=NULL;
  Query *q;
  string userpwd;
  
  xmlAttrPtr tmp = node->properties;
  if (!tmp) return false;

  while (tmp) {
    if (tmp->name && tmp->children && tmp->children->content) {
      if (!xmlStrcmp( tmp->name, (XMLS)"user-name")) {
	username = tmp->children->content;		
      }
      else if (!xmlStrcmp( tmp->name, (XMLS)"password")) {
	password = tmp->children->content;
      }
    }
    tmp = tmp->next;
  }

  if (!username) 
    return ShowError("Connection::MRMLGetSessions() Missing username!\n");

  xmlNodePtr node2 = xmlNewChild(root2, ns2, (XMLS)"session-list", NULL);

  // create username:password combination
  userpwd = (string)(const char *)username + (string)":";// + 
  if(password)
    userpwd = userpwd + (string)(const char *)password;   

  int i = 1;
  do {
    q = Picsom()->FindNthQueryInMemory(userpwd.c_str(), i);
    if (q) {
      xmlNodePtr node3 = xmlNewChild(node2, ns2, (XMLS)"session", NULL);

      if (q->Identity()!="")  
	xmlNewProp(node3, (XMLS)"session-id", (XMLS)q->Identity().c_str());

      if (q->GetName()!="")
	xmlNewProp(node3, (XMLS)"session-name", (XMLS)q->GetName().c_str());
	   
    }
    i++;
  } while (q);

  // if session-list is still empty, add empty session to list
  if (!node2->children)
    xmlNewChild(node2, ns2, (XMLS)"session", NULL);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLOpenSession(xmlNodePtr node, xmlNodePtr root2, 
                                 xmlNsPtr ns2) {
  XMLS username=NULL, sessionname=NULL, password=NULL;
  xmlAttrPtr tmp;

  tmp = node->properties;
  if(tmp==NULL) return false;

  while(tmp) {
    if(tmp->name && tmp->children && tmp->children->content) {
      if(!xmlStrcmp( tmp->name, (XMLS)"user-name")) {
	username = tmp->children->content;		
      }
      // that dumb BenchAthlonStarterPackage testing client uses user-id!
      else if(!xmlStrcmp( tmp->name, (XMLS)"user-id")) {
	username = tmp->children->content;		
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"session-name")) {
	sessionname = tmp->children->content;
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"password")) {
	password = tmp->children->content;
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"session-id")) {
	//old_session_id = tmp->children->content;
      }
      else { // for the unrecognized options, do nothing
      }
    }
    tmp = tmp->next;
  }

  if(!username)
    return ShowError("Connection::MRMLOpenSession() No user-name!");

  // Here OPEN-SESSION assumes that 
  // a) if session_id was given in mrml root tag (ie. <mrml session-id=123>)
  // then it will be checked at the same place and if session was found that session
  // will be used for all ops. 


  //if(old_session_id) 
  // RetrieveOldSession( old_session_id )
  // else continue creating new, empty session

  // Creating empty query to get session id (1st time query creation!!)
  //if( session_id.length()==0 ) { 
  // if there is already existing query was found do nothing here.
    Query *q = new Query(Picsom());
    q->SetIdentity();
    q->PerMapObjects(100);
    Picsom()->AppendQuery(q, true);
    session_id = (string)q->Identity();
    if(session_id=="") return false;
    //}

  // store username and password for later use
  string usrpwd;
  usrpwd = (string)(const char *)username + (string)":";
  if(password)
    usrpwd = usrpwd + (string)(const char *)password;

  q->FirstClient().UserPassword( usrpwd.c_str() );

  // store human-readable name of session if one exists
  if(sessionname)
    q->SetName( (const char *)sessionname);


  // ADD SESSION-ID to ROOT2 properties ( session-id="Qxxx-yyy-zzz")
  xmlNewProp(root2, (XMLS)"session-id", getSessionID());

  // ADD TIME-STAMP message
  MRMLAddTime(root2, ns2); // !!! !!! changed this AFTER ack-session-op. 
  // See how it works !!!! Why?

  xmlNodePtr n = xmlNewChild(root2, ns2,(XMLS)"acknowledge-session-op" ,NULL);
  xmlNewProp(n, (XMLS)"user-name", username);

  if(password)    xmlNewProp(n, (XMLS)"password",password);
  if(sessionname) xmlNewProp(n, (XMLS)"session-name",sessionname);

  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Connection::MRMLGetCollections(xmlNodePtr root2, xmlNsPtr ns2) {
  xmlNodePtr n = xmlNewChild(root2, ns2,(XMLS)"collection-list" ,NULL);
  if(n==NULL) return false;

  Picsom()->FindAllDataBases(false); // or true?
  for (int i=0; i<Picsom()->NdataBases(); i++) {
    xmlNodePtr node5 = xmlNewChild(n, ns2, (XMLS)"collection", NULL); 

    // changed 15.01.03 collection-id to name instead of number!
    //xmlNewProp(node5, (XMLS)"collection-id", Int2XMLS(i));
    xmlNewProp(node5, (XMLS)"collection-id",
	       (XMLS)Picsom()->GetDataBase(i)->Name().c_str() );
    xmlNewProp(node5, (XMLS)"collection-name",
	       (XMLS)Picsom()->GetDataBase(i)->Name().c_str() );

    xmlNodePtr node6 = xmlNewChild(node5,ns2,(XMLS)"query-paradigm-list",NULL);
    xmlNodePtr node7 = xmlNewChild(node6, ns2, (XMLS)"query-paradigm", NULL); 
    xmlNewProp(node7, (XMLS)"type",
	       (XMLS)Picsom()->GetDataBase(i)->Name().c_str());
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Connection::MRMLGetAlgorithms(xmlNodePtr node, xmlNodePtr root2,
					   xmlNsPtr ns2){
  int insertOnlyThisDB = -1;
  
  
  xmlNodePtr node0 = xmlNewChild(root2, ns2, (XMLS)"algorithm-list", NULL);

  xmlNodePtr node1 = xmlNewChild(node0, ns2, (XMLS)"algorithm", NULL); 
  xmlNewProp(node1, (XMLS)"algorithm-id",(XMLS)"adefault");
  xmlNewProp(node1, (XMLS)"algorithm-name",(XMLS)"no algorithm");
  xmlNewProp(node1, (XMLS)"algorithm-type",(XMLS)"adefault");
  xmlNewProp(node1, (XMLS)"collection-id",(XMLS)"__COLLECTION__");
  xmlNodePtr node1_1 = xmlNewChild(node1,ns2,(XMLS)"query-paradigm-list",NULL);
  xmlNewChild(node1_1, ns2, (XMLS)"query-paradigm", NULL); 
  
  Picsom()->FindAllDataBases(false); // or true?

  // if only features of single database(collection) were asked
  xmlAttrPtr tmp = node->properties;

  if(tmp && tmp->name && tmp->children && tmp->children->content) {
    // no need to check does tmp has children since either one of
    // collection-id or query-paradigm-id is enough for our purposes
    // because all the features will be extracted from single database
    // which is spesified similarly in both collection-id and query-paradigm-id
    // by the name of the database
      if( !xmlStrcmp( tmp->name, (XMLS)"collection-id") ||
	  !xmlStrcmp( tmp->name, (XMLS)"query-paradigm-id") ){

	for (int i=0; i<Picsom()->NdataBases(); i++) {
	  DataBase *d = Picsom()->GetDataBase(i);
	  if(!xmlStrcmp(tmp->children->content, (XMLS)d->Name().c_str()) )
	    insertOnlyThisDB = i; // we want only algorithms for one collection
	  else if(!xmlStrcmp(tmp->children->content, (XMLS)"cdefault") )
	    insertOnlyThisDB = 0; // change default database HERE !!!!!!!
	}
      }
  }

  // start forloop from zero or from selected collection (insertOnlyThisDB)
  for (int i=(insertOnlyThisDB!=-1?insertOnlyThisDB:0);
	i<Picsom()->NdataBases(); i++) {
    DataBase *d = Picsom()->GetDataBase(i);
    int nfeats = d->FindAllIndices();

    for (int alg=0; alg<5; alg++) {
    string aid;
    switch (alg) {
        case 0: aid = (string)"picsom-"       + (string)ToStr(i); break;
        case 1: aid = (string)"picsom_bottom-"+ (string)ToStr(i); break;
        case 2: aid = (string)"vq-"           + (string)ToStr(i); break;
        case 3: aid = (string)"vqw-"          + (string)ToStr(i); break;
        case 4: aid = (string)"sq-"           + (string)ToStr(i); break;
        default:aid = (string)"no_algorithm-" + (string)ToStr(i); break; 
    }
    //cout << "$$$$$ $$$$$ $$$$$ Selected algorithm_id: "<< aid << endl;

    string aname = aid.substr(0, aid.find("-"));
    xmlNodePtr node2 = xmlNewChild(node0, ns2, (XMLS)"algorithm", NULL); 
    xmlNewProp(node2, (XMLS)"algorithm-id",(XMLS)aid.c_str());
    xmlNewProp(node2, (XMLS)"algorithm-name",(XMLS)aname.c_str());
    xmlNewProp(node2, (XMLS)"algorithm-type",(XMLS)aid.c_str());
    xmlNewProp(node2, (XMLS)"collection-id",(XMLS)"__COLLECTION__");

    xmlNodePtr node2_1 = xmlNewChild(node2,ns2,(XMLS)"query-paradigm-list",
				     NULL); 
    xmlNodePtr node2_11 = xmlNewChild(node2_1, ns2, 
				      (XMLS)"query-paradigm", NULL); 
    // ACCORDING TO THE SPECS THE NEXT SHOULD BE "query-paradigm-id"
    // instead of "type" !!!
    xmlNewProp(node2_11, (XMLS)"type",(XMLS)d->Name().c_str());

    xmlNodePtr node3 = xmlNewChild(node2,ns2,(XMLS)"property-sheet",NULL); 
    xmlNewProp(node3, (XMLS)"maxsubsetsize",(XMLS)"1");
    xmlNewProp(node3, (XMLS)"minsubsetsize",(XMLS)"0");
    xmlNewProp(node3, (XMLS)"property-sheet-id",(XMLS)"cui-p-1");
    xmlNewProp(node3, (XMLS)"property-sheet-type",(XMLS)"subset");
    xmlNewProp(node3, (XMLS)"send-type",(XMLS)"none");

    xmlNodePtr node4 = xmlNewChild(node3,ns2,(XMLS)"property-sheet",NULL); 
    xmlNewProp(node4, (XMLS)"caption",(XMLS)"Modify default configuration");
    xmlNewProp(node4, (XMLS)"property-sheet-id",(XMLS)"cui-p0");
    xmlNewProp(node4, (XMLS)"property-sheet-type",(XMLS)"set-element");
    xmlNewProp(node4, (XMLS)"send-type",(XMLS)"none");

    xmlNodePtr node6 = xmlNewChild(node4,ns2,(XMLS)"property-sheet",NULL); 
    xmlNewProp(node6, (XMLS)"maxsubsetsize", Int2XMLS( nfeats ) );
    xmlNewProp(node6, (XMLS)"minsubsetsize",(XMLS)"1");
    xmlNewProp(node6, (XMLS)"property-sheet-id",(XMLS)"cui-p1");
    xmlNewProp(node6, (XMLS)"property-sheet-type",(XMLS)"subset");
    xmlNewProp(node6, (XMLS)"send-type",(XMLS)"none");

    for (int j=0; j<nfeats; j++) {
      xmlNewProp(node2, (XMLS)d->GetIndex(j)->Name().c_str(),(XMLS)"no");
	     
      xmlNodePtr node7 = xmlNewChild(node6, ns2, (XMLS)"property-sheet",
				     NULL); 
      xmlNewProp(node7, (XMLS)"caption",(XMLS)d->GetIndex(j)->Name().c_str());
      xmlNewProp(node7, (XMLS)"property-sheet-id", Int2XMLS(j) );
      xmlNewProp(node7, (XMLS)"property-sheet-type",(XMLS)"set-element");
      xmlNewProp(node7, (XMLS)"send-boolean-inverted",(XMLS)"yes");
      xmlNewProp(node7, (XMLS)"send-name",(XMLS)d->GetIndex(j)->Name().c_str());
      xmlNewProp(node7, (XMLS)"send-type",(XMLS)"attribute");
      xmlNewProp(node7, (XMLS)"send-value",(XMLS)"yes");
    }

    }//for alg

    // if collection-id was given then break after algorithms of one db
    if( insertOnlyThisDB != -1 )
      break;

  }//for (Ndatabases())

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void Connection::MRMLAddTime(xmlNodePtr root2, xmlNsPtr ns2) {
  char buf[100] = "";
  time_t tt;
  time(&tt);
  char *timep = ctime_r(&tt, buf);
  char *nl = strchr(buf, '\n');
  if (nl)
    *nl = 0;
  xmlNodePtr node2 = xmlNewChild(root2, ns2, (XMLS)"cui-time-stamp", NULL);
  xmlNewProp(node2, (XMLS)"calendar-time", (XMLS)timep);
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLConfigureSession(xmlNodePtr node, 
					      xmlNodePtr root2, xmlNsPtr ns2) {
  if(node->properties && node->properties->name && 
     node->properties->children && node->properties->children->content)
  if(xmlStrcmp( node->properties->name, (XMLS)"session-id")) 
    return 
      ShowError("Connection::MRMLConfigureSession()-No session-id spesified!");

  //double check that it is still same session we're talking about
  if(!xmlStrcmp( node->properties->children->content, getSessionID())) {
    // ADD ACKNOWLEDGE message (w/ session-id only)
    xmlNodePtr node3  = xmlNewChild(root2, ns2, 
				    (XMLS)"acknowledge-session-op", NULL);
    xmlNewProp(node3, (XMLS)"session-id", getSessionID());
  }
  else
    return 
      ShowError("Connection::MRMLConfigureSession() - Different sessions!");


  xmlNodePtr tmp = node->children;
  while(tmp) {
    // <algorithm algorithm-id="ad" algorithm-type="ad" collection-id="c-123">
    if(tmp->name) {
      if(!xmlStrcmp(tmp->name, (XMLS)"algorithm")) {
	if(!MRMLAlgorithm( tmp )) {
	  return ShowError("Connection::MRMLAlgorithm() failed!");
	}      
      }
      else if(!xmlStrcmp(tmp->name, (XMLS)"text")) {
	//printf("%s", tmp->content);
      }
      else {
	ShowError("Connection::MRMLConfigureSession() Unknown parameter",
		  (const char *)tmp->name);
      }
    }
    tmp = tmp->next;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLRenameSession(xmlNodePtr node, 
					      xmlNodePtr root2, xmlNsPtr ns2) {
  xmlAttrPtr tmp = node->properties;
  if(tmp && tmp->name && tmp->children && tmp->children->content) {
    while(tmp) {
      if(!xmlStrcmp( tmp->name, (XMLS)"session-id")) {
	//double check that it is still same session we're talking about
	if(!xmlStrcmp( tmp->children->content, getSessionID())) {
	  // ADD ACKNOWLEDGE message (w/ session-id only)
	  xmlNodePtr node3  = xmlNewChild(root2, ns2, 
					  (XMLS)"acknowledge-session-op",
					  NULL);
	  xmlNewProp(node3, (XMLS)"session-id", getSessionID());
	}
	else
	  return 
	    ShowError("Connection::MRMLRenameSession() - Incorrect session!");
      }
      else if(!xmlStrcmp(tmp->name, (XMLS)"session-name")) {
	query->SetName((const char *)tmp->children->content);

      }
      else if(!xmlStrcmp(tmp->name, (XMLS)"text")) {
	//printf("%s", (const char *)tmp->children->content);
      }
      else {
	ShowError("Connection::MRMLConfigureSession() Unknown parameter",
		  (const char *)tmp->name);
      }    
      tmp = tmp->next;
    }
  }
  return true;
}
///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLAlgorithm(xmlNodePtr node) {
  XMLS a_aid=NULL, a_atype=NULL, a_cid=NULL;
  xmlAttrPtr tmp=NULL;
  DataBase *d=NULL;
  int nfeats = 0;
  const char *features[100]; // huom. kiintea koko!!!!
  char feats[10000] = "";

  Picsom()->FindAllDataBases(false); // or true?


  if(node->properties)
    tmp = node->properties;

  while(tmp) {
    if(tmp->name && tmp->children && tmp->children->content) {
      if(!xmlStrcmp(tmp->name, (XMLS)"algorithm-id")) {
	a_aid = tmp->children->content;
      }
      else if(!xmlStrcmp(tmp->name, (XMLS)"algorithm-type")) {
	//algorithm-type is quite useless
	a_atype = tmp->children->content;
      }
      else if(!xmlStrcmp(tmp->name, (XMLS)"collection-id")) {
	if(!xmlStrcmp(tmp->children->content, (XMLS)"cdefault")) {
	  //removed numbering and changed it to textnames 15.01.03
	  //a_cid = (XMLS)"0"; 
	  //a_cid = (XMLS)"TV";
	  a_cid = (XMLS)"done128x192";
	}
	else a_cid = tmp->children->content;

	// retrieve wanted database (based on collection-id)
	//changed number based search to name based search 15.01.03
	//d = Picsom()->GetDataBase(atoi((const char *)a_cid));
	for(int nn=0; nn<Picsom()->NdataBases(); nn++) {
	  if(!strcmp( Picsom()->GetDataBase(nn)->Name().c_str(),
		      (const char *)a_cid ))
	    d = Picsom()->GetDataBase(nn);
	}
	if(!d) return false;

	// count number of features
	nfeats = d->FindAllIndices();
	if(nfeats>0) {
	  //features muistinvaraus pitaisi tehda vasta tassa !!!!
	  // read the names of all features
	  for(int j=0; j<nfeats; j++)
	    features[j] = d->GetIndex(j)->Name().c_str();
	}
      }
      // add algorithm specific options (cui-***-***) as well
      else if(a_cid && nfeats>0) {
	// collect algorithm specific, selected features
	for(int k=0; k<nfeats; k++)
	  if(!xmlStrcmp(tmp->name, (XMLS)features[k]) && 
	     !xmlStrcmp(tmp->children->content, (XMLS)"yes")) {	    
	      strcat( feats, (const char *)tmp->name );
	      strcat( feats, (const char *)"," );
	  }
      }
      else {      
	printf("Unrecognized parameter %s\n",tmp->name);
      }
    }
    tmp = tmp->next;
  }//while(tmp)

  if(a_aid==NULL || a_cid==NULL) return false;
  if(a_atype==NULL) printf("Warning: Algorithm type missing.\n");

  string aid = (string)(const char *)a_aid; 

  if( aid == "adefault" ) 
    aid = "picsom";
  else 
    aid = aid.substr(0, aid.find("-"));

  //cout << "$$$$$ $$$$$ $$$$$  Selected algorithm: "<< aid << endl;
      
  query->SetDataBase( d );  
  // aft. setting database some default values still need to be copied in query
  query->CopySome(query->GetDataBase()->DefaultQuery(), true, false, false);
  query->InterpretDefaults();
  d->ReadOriginsIntoHash(); // read file "origins" into a hash table

  query->Algorithm( Picsom()->CbirAlgorithm( aid.c_str() )); 

  //if no feats selected select them all
  if(feats[0]=='\0') {
    feats[0] = '*'; feats[1] = '\0';
  }
  cout << "Suoritetaan SelectIndices featureille:" << feats << endl;
  query->SelectIndices(NULL, feats); 

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLQueryStep(xmlNodePtr node, xmlNodePtr root2, 
				       xmlNsPtr ns2) { 
  XMLS qs_rsize=NULL, /*qs_cutoff=NULL,*/ qs_cid=NULL, qs_aid=NULL;
  int rimages;
  const char *feats=NULL;

  xmlAttrPtr tmp = NULL;
  xmlNodePtr unknown = NULL;

  if(node->properties)
    tmp = node->properties;

  // Read and Handle query-step parameters
  while(tmp) {
    if(tmp->name && tmp->children && tmp->children->content) {
      if(!xmlStrcmp( tmp->name, (XMLS)"session-id")) {
	//double check that it is still same session we're talking about
	if( xmlStrcmp( tmp->children->content, getSessionID())){
	  return ShowError("Connection::MRMLQueryStep() different sessions!");
	}
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"result-size")) {
	qs_rsize = tmp->children->content;
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"algorithm-id")) {
	qs_aid = tmp->children->content;
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"collection") ||
	      !xmlStrcmp( tmp->name, (XMLS)"collection-id") ) {
	qs_cid = tmp->children->content;

	// third place to set !!!!! default database. 
	//changed 15.01.03 numbering to names (still set default somewhere else
	//qs_cid = (XMLS)"0";
	if(!xmlStrcmp( qs_cid, (XMLS)"cdefault"))
	  //qs_cid = (XMLS)"TV";
	  qs_cid = (XMLS)"done128x192";
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"result-cutoff")) {
	// qs_cutoff = tmp->children->content;
      }
      else {
	printf("(in query):Unrecognized parameter %s\n", tmp->name);
      }
    }
    tmp = tmp->next;
  }
  // -------------------------------------------------

  rimages = atoi( (const char *)qs_rsize);

  // set maximum number of resulting images
  query->MaxQuestions(rimages);
  DataBase* db = query->GetDataBase();


  // if no database has not yet been selected (no one ran configure-session!)
  if(!db) { // then select wanted database

    // changed 15.01.03 database numbering into naming search
    //db = Picsom()->GetDataBase(atoi((const char *)qs_cid));
    
    for(int nn=0; nn<Picsom()->NdataBases(); nn++) {
      if( !strcmp(Picsom()->GetDataBase(nn)->Name().c_str(),
		  (const char *)qs_cid) )
	db = Picsom()->GetDataBase(nn);
    }
    
    if(!db) return ShowError("MRMLQueryStep: Database selection failed");;

    int nfeats = db->FindAllIndices();
    if(nfeats>0) feats = "*";

    string aid = (string)(const char *)qs_aid;

    if( aid == "adefault")
      aid = "picsom";
    else 
      aid = aid.substr(0, aid.find("-")); 

    query->SetDataBase( db );
    db->ReadOriginsIntoHash(); // read file "origins" into a hash table
    query->Algorithm( Picsom()->CbirAlgorithm( aid.c_str() )); 
    query->SelectIndices(NULL, feats); 

    //  cout << "$$$$$ $$$$$ $$$$$ Selected algorithm: "<< aid << endl;

  } 

  // create the header for response RESULT LIST (common)
  xmlNodePtr qResList = xmlNewChild(root2, ns2, 
				    (XMLS)"query-result", NULL);
  xmlNodePtr qRes = xmlNewChild(qResList, ns2, 
				(XMLS)"query-result-element-list", NULL);


  // check & skip only-text children
  unknown=node->children;
  while ( unknown && !xmlStrcmp( unknown->name,(XMLS)"text") )
    unknown = unknown->next;
  
  // moved here @20.01.03 so that queries without random images 1st would work!
  query->ReadFiles();


  //---- RANDOM images were asked ----//
  if(!node->children || !unknown) { 

    //moved this before if so that readfiles will be done all the times!
    //query->ReadFiles();
    query->RandomUnseenObjects(false,false);

    for(int n=0; n<rimages; n++) {
      const char *l = query->NewObject(n).LabelP();
      string op = db->SolveObjectPath(l, "tn-120x90");

      char tnobj[1000];
      sprintf(tnobj, "file:%s", op.c_str());

      op = db->SolveObjectPath(l);
      char imgobj[1000];
      sprintf(imgobj, "file:%s", op.c_str());

      xmlNodePtr qResElem  = xmlNewChild(qRes, ns2, 
					 (XMLS)"query-result-element", NULL);
      xmlNewProp(qResElem, (XMLS)"calculated-similarity", (XMLS)"1.0");
      xmlNewProp(qResElem, (XMLS)"image-location",(XMLS)imgobj); 
      xmlNewProp(qResElem, (XMLS)"thumbnail-location",(XMLS)tnobj);
     }
    
  }
  else { // there are some more tags!
    unknown = node->children;
  
    while(unknown) {
      if(unknown->name) {
	if(!xmlStrcmp( unknown->name, (XMLS)"mpeg-7-qbe")) {
	  if( !MRMLMpeg7qbe( unknown, root2, ns2) ) {
	    ShowError("Mpeg-7-qbe failed!");
	  }
	}
	else if(!xmlStrcmp(unknown->name,(XMLS)"user-relevance-element-list")){
	  if( !MRMLUserRelevanceElementList( unknown )) {
	    return ShowError("User relevance element list failed!");
	  }
	}
	else if(!xmlStrcmp( unknown->name, (XMLS)"text")) {
	  //printf("%s",(const char *)unknown->content);
	}
	else {
	  ShowError("Unknown tag ",(const char*)unknown->name,
		    " in Connection::MRMLQueryStep()!");
	}
      }
      unknown = unknown->next;
    }

    query->CbirStages();

    // calculate similarities to given images //
    for (size_t n=0; n<query->NnewObjects(); n++) {
      if (!query->NewObject(n).Retained())
	break;

      const char *l = query->NewObject(n).LabelP();

      string op = db->SolveObjectPath(l, "tn-120x90");
      char tnobj[1000];
      sprintf(tnobj, "file:%s", op.c_str());

      op = db->SolveObjectPath(l);
      char imgobj[1000];
      sprintf(imgobj, "file:%s", op.c_str());

      xmlNodePtr qResElem  = xmlNewChild(qRes, ns2, 
					 (XMLS)"query-result-element", NULL);
      xmlNewProp(qResElem, (XMLS)"calculated-similarity",
	    Float2XMLS(query->NewObject(n).Value()/query->NewObject(0).Value()));
      xmlNewProp(qResElem, (XMLS)"image-location",(XMLS)imgobj); 
      xmlNewProp(qResElem, (XMLS)"thumbnail-location",(XMLS)tnobj);
     }
 
  }//else
						       
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Connection::MRMLUserRelevanceElementList(xmlNodePtr nodein) { 
  xmlNodePtr node = NULL;

  if(nodein->children)
    node = nodein->children;

  while(node) {
    if(node->name) {
      if(!xmlStrcmp(node->name, (XMLS)"user-relevance-element")) {
	if(!MRMLUserRelevanceElement(node)) {
	  return 
	    ShowError("Connection::MRMLUserRelevanceElementList() failed");
	}
      }
    }
    node = node->next;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLUserRelevanceElement(xmlNodePtr nodein) {
  XMLS imgloc=NULL, thnloc=NULL, usrrel=NULL;
  xmlAttrPtr tmp=NULL;

  if(nodein->properties)
    tmp = nodein->properties;

  while(tmp) {
    if(tmp->name) {
      if(!xmlStrcmp( tmp->name, (XMLS)"image-location")) {
	  if(tmp->children && tmp->children->content)
	    imgloc = tmp->children->content;
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"thumbnail-location")) {
	if(tmp->children && tmp->children->content)
	  thnloc = tmp->children->content;
      }
      else if(!xmlStrcmp( tmp->name, (XMLS)"user-relevance")) {
	if(tmp->children && tmp->children->content)
	  usrrel = tmp->children->content;
      }
      else {
	printf("Connection::MRMLUserRelevanceElement-Unknown parameter %s \n",
	       tmp->name);
      }      
    }
    tmp = tmp->next;
  }
  if( imgloc==NULL && thnloc==NULL ) return false;


  const char *fullname = (const char *)imgloc;  
  char filename[255] =""; // BAD assumption??

  // 1st try to find picsom-filename from hash if we got HTTP-path
  const string& fnamestr = query->GetDataBase()->FindFnameFromHash(fullname);
  const char *fname = fnamestr.c_str();

  if (*fname==0) cout << "Warning: Not matching httppath - \"origins\" "
		      << "will not be used "<< endl;
  else cout << "HttpServerpath match! dbname="<<fname
	    <<" fullname= "<<fullname<<endl;

  int idx = -1;

  // in case we did not find it assume we got direct reference (path+filename)
  // into correct file (or plain filename) and extract only filename part out
  if (*fname==0) {

    // test if we did get picsom-filename (8digits) straight away
    if( query->GetDataBase()->IsLabel(fullname) ) {
      fname = fullname;
    }
    else {      
      unsigned int pos = 0, i = 0;
      // extracting only filename part from full path 
      // (from last '/' until next '.')
      do {   
	if(*(fullname+i) == '/')
	  pos = i;
	i++;
      } while(i < strlen(fullname) );
      i = pos; 
      while( *(fullname+i) != '.' ) {
	if( i >= strlen(fullname)) break;
	i++;
      } 
      pos++; // pos points to beginning of filename, i points to "."
    
      strncpy(&filename[0], (fullname+pos), i-pos);

      fname = filename;
    }
      
    cout << "Found filename: " << fname << endl;
  }

  char val = 'x';
  if(!xmlStrcmp( usrrel, (XMLS)"1"))
    val = '+';
  else if(!xmlStrcmp( usrrel, (XMLS)"-1"))
    val = '-';
  else if(!xmlStrcmp( usrrel, (XMLS)"0"))
    // !!! hack - all not selected images will be dropped picsom-style
    val = '-';  //val = '0';
  else
    return ShowError("Connection::MRMLUserRelevanceElement() : not 1/-1/0");

  idx = query->LabelIndex(fname);
  //cout << " !!!! IDX: " << idx << " fname: " << fname
  //     << " filename: " << filename << " fullname: " <<fullname <<  endl;

  // query->MarkAsSeenObsoleted(fname, val, idx);

  // bool dval = val=='+' ? 1 : -1;  // dunno how long this had been "bool"!
  double dval = val=='+' ? 1.0 : -1.0;
  query->MarkAsSeenNoAspects(idx, dval, query->GetDataBase());

  return true;
}

#endif //USE_MRML

///////////////////////////////////////////////////////////////////////////////

bool Connection::XMLVersionOK() const {
  /*
  cout << "Connection::XMLVersionOK() : xml_version=" << xml_version
       << " XML_VERSION=" << XML_VERSION << endl;
  return xml_version==XML_VERSION;
  */
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::MRMLVersionOK() const {
  return mrml_version==MRML_VERSION;
}
  
  /////////////////////////////////////////////////////////////////////////////

  void Connection::Notify(bool create, const char *txt,
			  bool force) const {
    if (debug_sockets || force) {
      string thrid = "???";
      thrid = OwnThreadIdentity();

      string chstr;
      if (child!=-1)
	chstr = " ["+ToStr(child)+"]";

      if (Picsom()) {
	ostringstream cstr;
	cstr << (create?"Created ":"Closing ")
	     << TypeStringStr(true) << " connection \""
	     << (identity_str.size()?identity_str.c_str():"--empty-identity--")
	     << "\" (" << rfd << "/" << wfd << ") /"
	     << thrid << chstr << " " << (txt?txt:"") << (txt?" ":"")
	     << "#conn=" << Picsom()->Connections(false, true)
	     << "/" << Picsom()->Connections(true, false)
	     << "/" << Picsom()->Connections(false, false);
	WriteLog(cstr);
      }
    }
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::Close(bool angry, bool notify) {
  string msg = "Connection::Close() : ";

  if (type&conn_closed && !angry)
    return true;

  if (notify)
    Notify(false, NULL, true);

#ifdef PICSOM_USE_PTHREADS
  if (HasOwnThread() && pthread_equal(pthread, pthread_self()))
    CancelThread();
#endif // PICSOM_USE_PTHREADS

  if (type&conn_closed)
    return ShowError(msg+"already closed");

#ifdef HAVE_OPENSSL_SSL_H
  if (ssl)
    SSL_shutdown(ssl);
#endif // HAVE_OPENSSL_SSL_H

  if (rfile) {
    fclose(rfile);  // I dunno if this is necessary nor allowed...
    rfile = NULL;
  }

  if (rfd!=wfd && rfd>=0)
    if (!DoClose(rfd))
      ShowError("DoClose(rfd) failed");

  if (wfd>=0)
    if (!DoClose(wfd))
      ShowError("DoClose(wfd) failed");

  if (child>0) {
    if (!kill(child, SIGTERM)) {
      if (notify)
	WriteLog("Terminated child process "+ToStr(child));

    } else if (angry)
      return ShowError(msg+"kill() failed");

#ifndef HAVE_WINSOCK2_H
    // obs! no diagnostics yet
    int status = 0;
    waitpid(child, &status, WNOHANG|WUNTRACED|WCONTINUED);
#endif // HAVE_WINSOCK2_H
  }

  // rfd = wfd = -1;

#ifdef HAVE_OPENSSL_SSL_H
  if (ssl)
    SSL_free(ssl);
  ssl = NULL;
#endif // HAVE_OPENSSL_SSL_H

#ifdef PICSOM_USE_MPI
  if (type==conn_mpi_listen || type==conn_mpi_down || type==conn_mpi_up) {
    MPI::Close_port(mpi_port.c_str());
    mpi_port = "";
  }
#endif // PICSOM_USE_MPI

  type = connection_type(type|conn_closed);

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::DoClose(int fd) {
#ifdef HAVE_WINSOCK2_H
    if (IsSocket()) {
      shutdown(fd, SD_BOTH);
      return closesocket(fd)==0;
    }
#endif // HAVE_WINSOCK2_H
    return close(fd)==0;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::CloseFDs() {
    bool ok = true;
    if (rfd>=0 && rfd!=wfd)
      ok = DoClose(rfd) && ok;
    if (wfd>=0)
      ok = DoClose(wfd) && ok;

    rfd = wfd = -1;

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t Connection::ReadableBytes() {
    int ravail = -1;
    ioctl(rfd, FIONREAD, &ravail);
    return ravail>0 ? ravail : 0;
  }
    
///////////////////////////////////////////////////////////////////////////////

const char *Connection::DisplayStringM() const {
  char ret[1024];
  //const char *sfirst = TimeString(first).c_str();
  //const char *slast  = TimeString(last).c_str();

  char thrinfo[1000] = "";
  strcpy(thrinfo, "/");
  string tmp = OwnThreadIdentity();
  strcat(thrinfo, tmp.c_str());
  strcat(thrinfo, " ");

  sprintf(ret, "%4s %3d %3d %s%3s %1s",
	  TypeStringStr().c_str(),
	  rfd, wfd, thrinfo, StateString().c_str(), CanBeSelected()?"+":" ");

  int ravail = -1;
  ioctl(rfd, FIONREAD, &ravail);
  int rflags = fcntl(rfd, F_GETFL, 0);
  int wflags = fcntl(wfd, F_GETFL, 0);
  sprintf(ret+strlen(ret), " ravail=%d rflags=%d wflags=%d", ravail, rflags, wflags);

//   sprintf(ret+strlen(ret), " %5d %14s %14s %-35s %4d %-35s",
// 	  queries, sfirst, slast, Addresses(), version, identity);

  const char *ot = query ? "-undef-" /*QueryTypeP(query->Type())*/ : "-null-";

  sprintf(ret+strlen(ret), " %s %15s %12s %20s %12s %-35s",
	  is_active?"*":" ", ot, ObjectTypeP(ObjectRequest()),
	  ObjectName().c_str(), ObjectSpec().c_str(), Identity().c_str());

  return Simple::CopyString(ret);
}

///////////////////////////////////////////////////////////////////////////////

string Connection::TypeStringStr(bool lng) const {
  const char *r = "????";

  switch (type&conn_nonclosed) {
  case conn_none:      	 r = !lng?"none":"NONE";       break;
  case conn_terminal:  	 r = !lng?"term":"TERMINAL";   break;
  case conn_file:      	 r = !lng?"file":"FILE";       break;
  case conn_stream:    	 r = !lng?"strm":"STREAM";     break;
  case conn_pipe:      	 r = !lng?"pipe":"PIPE";       break;
  case conn_listen:    	 r = !lng?"list":"LISTEN";     break;
  case conn_socket:    	 r = !lng?"sock":"SOCKET";     break;
  case conn_http_server: r = !lng?"htsr":"HTTPSERVER"; break;
  case conn_http_client: r = !lng?"htcl":"HTTPCLIENT"; break;
  case conn_peer:      	 r = !lng?"peer":"PEER";       break;
  case conn_up:    	 r = !lng?"upln":"UPLINK";     break;
  case conn_down:    	 r = !lng?"dwln":"DOWNLINK";   break;
  case conn_soap_server: r = !lng?"soas":"SOAPSERVER"; break;
  case conn_soap_client: r = !lng?"soac":"SOAPCLIENT"; break;
  case conn_mpi_listen:  r = !lng?"mpil":"MPILISTEN";  break;
  case conn_mpi_down:    r = !lng?"mpid":"MPIDOWN";    break;
  case conn_mpi_up:      r = !lng?"mpiu":"MPIUP";      break;
  }

  if (type&conn_closed) {
    char tmp[100];
    strcpy(tmp, lng?"closed-":"C/");
    strcat(tmp, r);
    if (!lng)
      tmp[4] = 0;

    return tmp;
  }

  return r;
}

///////////////////////////////////////////////////////////////////////////////

  bool Connection::State(int type, int secs) const {
    if (type<0 || type>2)
      return false;
    
    fd_set set;
    FD_ZERO(&set);
    if ((type==0 || type==2) && rfd>=0)
      FD_SET(rfd, &set);
    if ((type==1 || type==2) && wfd>=0)
      FD_SET(wfd, &set);

    timeval wait;
    wait.tv_sec = (long)secs; wait.tv_usec = 0;

    if (type==0)
      select(FD_SETSIZE, &set, NULL, NULL, &wait);
    if (type==1)
      select(FD_SETSIZE, NULL, &set, NULL, &wait);
    if (type==2)
      select(FD_SETSIZE, NULL, NULL, &set, &wait);

    if (type==0)
      return rfd>=0 && FD_ISSET(rfd, &set);

    if (type==1)
      return wfd>=0 && FD_ISSET(wfd, &set);

    return (rfd>=0 && FD_ISSET(rfd, &set)) || (wfd>=0 && FD_ISSET(wfd, &set));
  }

///////////////////////////////////////////////////////////////////////////////

string Connection::StateString() const {
  /*
  fd_set rset, wset, eset; 
  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_ZERO(&eset);
  FD_SET(rfd, &rset);
  FD_SET(rfd, &eset);
  FD_SET(wfd, &wset);
  FD_SET(wfd, &eset);

  timeval wait;
  wait.tv_sec = 0; wait.tv_usec = 0;
  select(FD_SETSIZE, &rset, &wset, &eset, &wait);

  static char ret[4];
  ret[0] = FD_ISSET(rfd, &rset) ? 'r' : '-';
  ret[1] = FD_ISSET(wfd, &wset) ? 'w' : '-';
  ret[2] = FD_ISSET(rfd, &eset)||FD_ISSET(wfd, &eset) ? 'e' : '-';
  ret[3] = 0;
  */

  char ret[4]; // OBS! STATIC
  ret[0] = State(0) ? 'r' : '-';
  ret[1] = State(1) ? 'w' : '-';
  ret[2] = State(2) ? 'e' : '-';
  ret[3] = 0;

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

string Connection::Addresses(bool access_style) const {
  char ret[1024];
  strcpy(ret, "<>");
  const char *tty;

  switch (type&conn_nonclosed) {
  case conn_terminal:
#ifdef HAVE_TTYNAME
    tty = ttyname(rfd);
#else
    tty = "TTY00";
#endif // HAVE_TTYNAME
    strcpy(ret, tty?tty:"-closed-");
    break;

  case conn_file:
    strcpy(ret, identity_str.size()?identity_str.c_str():"-NULL-");
    break;

  case conn_listen: case conn_socket: case conn_up: case conn_down:
    if (!access_style) {
      sprintf(ret, "%s:%d/%d",
	      servant.Address().c_str(), servant.Pid(), servant.Port());
      if (client.IsSet())
	sprintf(ret+strlen(ret), "/%s", client.Address().c_str());
    } else {
      sprintf(ret, "%s %s",servant.Address().c_str(),client.Address().c_str());
      if (client.Cookie()!="")
	sprintf(ret+strlen(ret), ":%s", client.Cookie().c_str());
    }
    break;
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

const char *Connection::ReadLineM() {
  SetAccessTime();
  if (type==conn_terminal) {
    char *line = NULL;

#ifdef PICSOM_USE_READLINE
    char prompt[] = "picsom > ", empty[] = "";
    
    // read the current input line to "line":
    line = readline(Interactive() ? prompt : empty);
    
    // add the line to history, if non-empty:
    if (line && *line)
      add_history(line);
#endif // PICSOM_USE_READLINE

    return line;
  }

  if (type!=conn_file)
    Obsoleted("Connection::ReadLine() non-file");

  if (!Rfile()) {
    ShowError("Connection::ReadLine() Rfile()==NULL");
    return NULL;
  }

  char inbuf[1024] = "";
  if (!fgets(inbuf, sizeof inbuf, Rfile()))
    return NULL;

  //int nr = read(rfd, inbuf, sizeof(inbuf));
  //if (!nr) return NULL;
  //char *ptr = strpbrk(inbuf, "\n\r");
  //if (ptr) {
  //  *ptr = 0;
  //  if (ptr[1] && ptr[2] && strpbrk(ptr+2, "\n\r"))
  //    ShowError("Connection::ReadLine() multi-line received...");
  //}

  //X// rstream.getline(inbuf, sizeof inbuf);
  //X// if (!rstream)
  //X//   return NULL;
  //X// if (rstream.gcount()==sizeof inbuf)
  //X//   ShowError("Connection::ReadLine() line too long...");
  // ShowError("Connection::ReadLine(): parts of code commented out!");

  char *ptr = strpbrk(inbuf, "\n\r");
  if (ptr)
    *ptr = 0;

  //  if (debug_reads && rfd!=STDIN_FILENO && !is_xml)
  if (debug_reads && rfd!=STDIN_FILENO && protocol!=xml_language)
    cout << rfd << ">>>" << inbuf << "<<<" << endl;

  return Simple::CopyString(inbuf);
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::SendObjectXML() {
  // pic_som->Tic("Connection::SendObjectXML");

  if (debug_objreqs)
    DumpRequestInfo(cout);

  bool ret = true;

  xmlNsPtr   ns   = NULL;
  xmlDocPtr  doc  = xmlNewDoc((XMLS)"1.0");
  xmlNodePtr root = NewDocNode(doc, ns, "result");
  xmlDocSetRootElement(doc, root);

  SetProperty(root, "type", ObjectTypeP(obj_type));

  if (query->Identity()!="")
    AddTag(root, ns, "identity", query->Identity());

  stringstream ss;
  ss << "ot=" << (int)obj_type << "=" << ObjectTypeP(obj_type) << " "
     << IsQuery(obj_type)    << IsPicSOM(obj_type) 
     << IsDataBase(obj_type) << IsTSSOM(obj_type) << " "
     << "<" << obj_name << "> <" << obj_spec << ">";
  string str = ss.str();

  try {
    ret = SendObjectXMLinner(root, ns);
  }
  catch (const string& s) {
    return ShowError("Connection::SendObjectXML() : ERROR <", s, "> with "+
		     str);
  }
  catch (const std::exception& ex) {
    return ShowError("Connection::SendObjectXML() : ERROR [", ex.what(),
		     "] with "+str);
  }
  catch (...) {
    return ShowError("Connection::SendObjectXML() : ERROR ... with "+str);
  }

  bool wok = WriteOutXml(doc);
  xmlFreeDoc(doc);

  // pic_som->Tac("Connection::SendObjectXML");

  // cout << "WriteOutXml finished" << endl;

  return ret && wok;
}

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::SendObjectXMLinner(xmlNodePtr root, xmlNsPtr ns) {
    XmlDom xml(NULL, ns, root);

    return Picsom()->AddObjectToXML(xml, query, this,
				    obj_type, obj_name, obj_spec);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::WriteOutXml(xmlDocPtr doc, const string& hdrx) {
    string msg = "Connection::WriteOutXml() "+LogIdentity()+" : ";
    bool err = false, done = false;

    string all = hdrx;
    if (doc)
      all += XML2String(doc, true);

#ifdef HAVE_WINSOCK2_H
    if (IsSocket()) {
      int r = send(wfd, all.c_str(), all.size(), 0);
      err = r==SOCKET_ERROR;
      done = true;
    }
#endif // HAVE_WINSOCK2_H

#ifdef PICSOM_USE_MPI
    if (!done && (type==conn_mpi_up || type==conn_mpi_down)) {
      err = !WriteOutMPI("xml", all);
      done = true;
    }
#endif // PICSOM_USE_MPI

    if (!done) {
      if (type==conn_up || type==conn_down) {
	uint32_t htype = 3; // == "xml"
	uint64_t alls = all.size();
	char hdrbuf[16] = "PSMP";
	memcpy(hdrbuf+4, &htype, 4);
	memcpy(hdrbuf+8, &alls,  8);
	ssize_t r = 0;
	while (r!=16) {
	  ssize_t rr = write(wfd, hdrbuf+r, 16-r);
	  if (rr>0)
	    r += rr;
	  if (r!=16) {
	    ShowError(msg+"failed to write() PSMP");
	    if (rr<1) {
	      err = true;
	      break;
	    }
	    sleep(1);
	  }
	}
	if (debug_writes)
	  cout << TimeStamp() << "<<< /" << ThreadIdentifierUtil()
	       << " PSMP " << htype << " " << all.size() << " >>> "
	       << (err?"FAILED":"OK")<< endl;
      }
      // needed for partial ranges like html5 mp4 video
      //int fd_flags1 = fcntl(wfd, F_GETFL, 0);
      //int fd_flags2 = fd_flags1|SOCK_NONBLOCK;
      //fcntl(wfd, F_SETFL, fd_flags2);
      ssize_t r = 0;
      while (!err && r!=(ssize_t)all.size()) {
	ssize_t rr = write(wfd, all.c_str()+r, all.size()-r);
	if (rr>0)
	  r += rr;
	if (r!=(ssize_t)all.size()) {
	  ShowError(msg+"failed to write() payload");
	  if (rr<1) {
	    break;
	    err = true;
	  }
	  sleep(1);
	}
      }
      // cout << "XXXXXXXXXXXXXXXXXXX r=" << r << " all.size()=" << all.size()
      // 	   << endl;
      //fcntl(wfd, F_SETFL, fd_flags1);
    }

    if (err) {
      stringstream ss;
      Dump(ss);
      ShowError(msg+"failed write() or send() in "+ss.str());
    }

    ConditionallyDumpXML(cout, true, doc, "", hdrx, type==conn_http_server);

    return !err;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_MPI
  bool Connection::WriteOutMPI(const string& t, const string& s) {
    string msg = "Connection::WriteOutMPI("+t+","+ToStr(s.size())+" bytes) : ";

    if (type!=conn_mpi_down && type!=conn_mpi_up)
      return ShowError(msg+"not conn_mpi_*");

    mpi_write_buf.clear();

    uint32_t tint = 0;
    if (t=="raw")
      tint = 1;
    else if (t=="txt")
      tint = 2;
    else if (t=="xml")
      tint = 3;
    else if (t=="bin")
      tint = 4;
    else
      return ShowError(msg+"type unimplemented");

    uint64_t ssize = s.size();

    char hdr[16];
    memcpy(hdr,   "PSMP", 4);
    memcpy(hdr+4, &tint,  4);
    memcpy(hdr+8, &ssize, 8);
    string buf(hdr, 16);

    mpi_write_buf.push_back(make_pair(MPI::Request(), buf));
    const string& xx = mpi_write_buf.back().second;
    mpi_request = mpi_inter.Isend(xx.c_str(), xx.size(), MPI::CHAR, mpi_root,
				  mpi_msgtag++);
    mpi_write_buf.back().first = mpi_request;

    if (false)
      for (;;) {
	bool ok = mpi_request.Test();
	cout << "checking A ... " << (int)ok << endl;
	if (ok)
	  break;
      }
    // mpi_request.Wait();

    if (!s.size())
      return true;

    mpi_write_buf.push_back(make_pair(MPI::Request(), s));
    const string& yy = mpi_write_buf.back().second;
    mpi_request = mpi_inter.Isend(yy.c_str(), yy.size(), MPI::CHAR, mpi_root,
				  mpi_msgtag++);
    mpi_write_buf.back().first = mpi_request;

    if (false)
      for (;;) {
	bool ok = mpi_request.Test();
	cout << "checking B ... " << (int)ok << endl;
	if (ok)
	  break;
      }
    // mpi_request.Wait();

    return true;
  }
#endif // PICSOM_USE_MPI

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_MPI
  bool Connection::MPIavailable() {
    string msg = "Connection::MPIavailable() : ";

    if (type!=conn_mpi_down && type!=conn_mpi_up)
      return ShowError(msg+"not conn_mpi_*");

    bool a = mpi_inter.Iprobe(mpi_root, mpi_msgtag, mpi_status);

    return a;
  }
#endif // PICSOM_USE_MPI

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::Available() {
    string msg = "Connection::Available() : ";

    if (IsClosed())
      return false;

#ifdef PICSOM_USE_MPI
    if (type!=conn_mpi_down && type!=conn_mpi_up)
      return MPIavailable();
#endif // PICSOM_USE_MPI

    if (rfd<0)
      return false;

    struct pollfd pfd = { rfd, POLLIN, 0 };

    int r = poll(&pfd, 1, 0);

    return r>0;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_MPI
  pair<string,string> Connection::ReadInMPI() {
    string msg = "Connection::ReadInMPI() : ";

    if (type!=conn_mpi_down && type!=conn_mpi_up) {
      ShowError(msg+"not conn_mpi");
      return pair<string,string>();
    }

    // mpi_request.Wait();

    while (!mpi_inter.Iprobe(mpi_root, mpi_msgtag, mpi_status)) {
      // cout << " nothing yet..." << endl;
      timespec_t ts = { 0, 1000000 };
      nanosleep(&ts, NULL);      
    }

    uint64_t h[2] = { 0, 0 };
    mpi_request = mpi_inter.Irecv(h, sizeof(h),
				  MPI::CHAR, mpi_root, mpi_msgtag );
    mpi_msgtag++;

    mpi_request.Wait();

    char hdr[4];
    memcpy(hdr, &h, 4);
    string magic(hdr, 4);
    if (magic!="PSMP") {
      ShowError(msg+"not PSMP");
      return pair<string,string>();
    }

    for (;;) {
      bool a = mpi_inter.Iprobe(mpi_root, mpi_msgtag, mpi_status);
      if (a)
	break;

      timespec_t snap = { 0, 1000000 }; // 1ms
      nanosleep(&snap, NULL);
    }

    string buf(h[1], ' ');
    if (buf.size()) {
      mpi_request = mpi_inter.Irecv((void*)buf.c_str(), buf.size(),
				    MPI::CHAR, mpi_root, mpi_msgtag );
      mpi_msgtag++;
    }

    mpi_request.Wait();

    uint32_t tint = 0;
    memcpy(&tint, ((char*)h)+4, sizeof(short));
    string mtype;
    if (tint==1)
      mtype="raw";
    else if (tint==2)
      mtype = "txt";
    else if (tint==3)
      mtype = "xml";
    else if (tint==4)
      mtype = "bin";
    else
      ShowError(msg+"type "+ToStr(tint)+" unimplemented");

    return make_pair(mtype, buf);
  }
#endif // PICSOM_USE_MPI

///////////////////////////////////////////////////////////////////////////////

char *Connection::XML2StringM(xmlDocPtr doc, bool pretty) {
  int tree_was = xmlIndentTreeOutput;
  xmlIndentTreeOutput = true;

  xmlChar *dump = NULL;
  int dumplen;
  const char *enc = NULL; // "ISO-8859-1"

  if (pretty)
    xmlDocDumpFormatMemory(doc, &dump, &dumplen, true);
  else
    xmlDocDumpMemoryEnc(doc, &dump, &dumplen, enc);

  xmlIndentTreeOutput = tree_was;

  if (!dump)
    ShowError("Connection::XML2StringM(): xmlDocDump...() failed");

  return (char*)dump;
}

  /////////////////////////////////////////////////////////////////////////////

  void Connection::DumpXML(ostream& os, bool out,
			   xmlDocPtr doc, const string& txt, 
			   const string& hdr, bool all, bool http) {
    string dump = doc ? XML2String(doc, true) : "";
    DumpXML(os, out, dump, txt, hdr, all, http);
  }

  /////////////////////////////////////////////////////////////////////////////

  void Connection::DumpXML(ostream& os, bool out,
			   const string& docstr,
			   const string& txt,
			   const string& hdr_in, bool all, bool http) {
    const char *dashes = "------------";
    const char *a = out?"<<< ":">>> ";
    const char *b = out?" >>>":" <<<";

    string thr = "/"+ThreadIdentifierUtil()+" ";

    const string t = http ? "HTTP" : "XML";

    string extra;
    // if (type==conn_socket)
    //   extra = "socc fd="+ToStr(rfd)+","+ToStr(wfd)+" ";
    // if (type==conn_mpi_up || type==conn_mpi_down)
    //   extra = "mpi ";

    bool show_full_hdr = !http || !out ||
      hdr_in.find("<?xml version=")!=string::npos;

    if (hdr_in.find("SQLite format 3")!=string::npos)
      show_full_hdr = false;

    string elf = "\x7F" "ELF";
    if (hdr_in.find(elf)!=string::npos)
      show_full_hdr = false;

    string hdr_show = hdr_in;
    size_t rest_length = hdr_show.size();
    if (!show_full_hdr) {
      size_t p = hdr_show.find("\r\n\r\n");
      if (p!=string::npos) 
	hdr_show.erase(p+4);
      rest_length -= hdr_show.size();
    }

    os << TimeStamp() << a << thr << extra << dashes << " <" << t << ">  "
       << (txt!=""?txt:dashes) << b << endl << hdr_show;

    if (!show_full_hdr && rest_length)
      os << "+ BINARY DATA " << rest_length << " BYTES" << endl;

    bool first = true;
    size_t s, e = 0;
    while (e!=docstr.size()) {
      s = e;
      e = docstr.find('\n', s);
      if (e!=string::npos) {
	e++;
	bool b64line = e-s==73 && docstr[s]!=' ' && docstr[s]!='<';
	
	if (all || first || !b64line)
	  os << docstr.substr(s, e-s);

	if (!all && b64line && first) {
	  os << "..." << '\n';
	  first = false;
	}

	if (!first && !b64line)
	  first = true;

      } else {
	os << docstr.substr(s) << '\n';
	break;
      }
    }

    os << TimeStamp() << a << thr << extra << dashes << " </" << t << "> "
       << (txt!=""?txt:dashes) << b << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::XMLstub(bool dtd, bool isdemo, const list<string>& xsll,
                             const list<pair<string,string> >& param) {
    string pub = !dtd ? "" : "-//PicSOM//DTD PicSOM 0.5//EN";
    string ext = !dtd ? "" : "http://picsom.ics.aalto.fi/picsom/picsom-xml-0.5.dtd";
    ext = "";
    
    // XmlDom doc = XmlDom::Doc("picsom-xml", pub, ext);
    XmlDom doc = XmlDom::Doc(); // MSIE6

    for (list<string>::const_iterator i=xsll.begin(); i!=xsll.end(); i++) {
      string xsl = *i, ssurl  = xsl;
      if (ssurl[0]!='/')
	ssurl = "/"+ssurl;
      if (isdemo)
	ssurl = "/picsom"+ssurl;
      if (ssurl.size()<4 || ssurl.substr(ssurl.size()-4)!=".xsl")
	ssurl += ".xsl";
      string name = i==xsll.begin() ? "Main" : "Additional";
      string sshref = "href=\""+ssurl+"\"";
      string sstxt  = sshref+" title=\""+name+" style\" type=\"text/xsl\"";
      doc = doc.PiNode("xml-stylesheet", sstxt);
    }

    for (list<pair<string,string> >::const_iterator p=param.begin();
         p!=param.end(); p++) {
      string ptxt = "name=\""+p->first+"\" value=\""+p->second+"\"";
      doc = doc.PiNode("xslt-param", ptxt);
    }

    return doc;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<XmlDom,XmlDom> Connection::XMLdocroot(const string& r, bool isdemo,
					     const list<string>& xsl,
                                             const list<pair<string,string> >&
                                             param) {
    bool use_ns = true;

    string nsref = "http://picsom.ics.aalto.fi/picsom/ns";
    string nsn   = "picsom";
    string nsp   = "";  // was "picsom:"; 

    if (!use_ns)
      nsref = nsn = nsp = "";
    
    XmlDom doc  = XMLstub(true, isdemo, xsl, param);
    XmlDom root = doc.Root(nsp+r, nsref, nsn);

    return make_pair(doc, root);
  }

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Connection::HTMLstub() {
    XmlDom doc = XMLstub(false, false);
    return doc.Root("html");
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::ReadAcknowledgment(bool angry) {
  string msg = "ReadAcknowledgment() : ";

  if (protocol!=xml_language)
    return ShowError(msg+"not XML connection");

  bool dummy = false;
  if (!ReadAndParseXML(dummy, false))
    return angry ? ShowError(msg+"ReadAndParseXML() failed") : false;

  XmlDom res(XMLdoc());
  XmlDom ack = res.Root();
  if (ack.NodeName()!="acknowledgment")
    return ShowError(msg+"no <acknowledgment>");

  /// we _really_ don't care about the contents...

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::SendAcknowledgment(const map<string,string>& extra) {
    if (protocol!=xml_language)
      return true;

    bool full = true;

    XmlDom xmlx = XMLstub(true, false);
    xmlDocPtr   ack = xmlx.doc;
    xmlNsPtr     ns = xmlx.ns;
    xmlNodePtr root = NewDocNode(ack, ns, "acknowledgment");
    xmlDocSetRootElement(ack, root);
    
    XmlDom xml(root, ns);

    if (full) {
      xml.Element("hello",   "Just acknowledging.");
      xml.Element("version", PicSOM::PicSOM_C_Version());  // XML_VERSION?
      xml.Element("pid",     ToStr(getpid()));
      xml.Element("thread",  OwnThreadIdentity());
#ifdef PICSOM_USE_MPI
      bool mpi = Picsom()->IsSlave() || Picsom()->UseMpiSlaves();
      xml.Element("mpi",     mpi?"yes":"no");
#endif // PICSOM_USE_MPI
      for (auto i=extra.begin(); i!=extra.end(); i++)
	xml.Element(i->first, i->second);

    } else
      xmlNodeAddContent(root, (XMLS)"\n  Just acknowledging.\n");

    bool ok = WriteOutXml(ack);
    xmlFreeDoc(ack);

    return ok;
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::SendRefusal(const string& reason) {
  stringstream ss;
  ss << "Connection refused." << (reason!=""?" ":"") << reason;

  XmlDom xml = XMLstub(true, false); //?
  XmlDom err = xml.Root("error");
  err.Content(ss.str());

  return WriteOutXml(xml);
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::AddToXMLimage(XmlDom& xml, const string& name, 
                               const imagedata& imgd, const string& label,
                               bool persists) {
  stringstream png;

  png << imagefile::stringify(imgd, "image/png");
  return AddToXMLdata(xml, name, "image/png", png, persists, label);
}

///////////////////////////////////////////////////////////////////////////////

string Connection::ExpirationString(const timespec_t& tt) {
  tm mytime;
  tm *time = localtime_r(&tt.tv_sec, &mytime);

  static char buf[100];
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S GMT", time); 

  return buf;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::AddToXMLconnection(XmlDom& xml) const {
  XmlDom c = xml.Element("connection");

  c.Element("type",     TypeStringStr(true));
  c.Element("protocol", ToStr((int)protocol));
  c.Element("state",    StateString());
  c.Element("port",     ToStr(port));
  c.Element("wfd",      ToStr(wfd));
  c.Element("rfd",      ToStr(rfd));
  c.Element("identity", identity_str);

  return true;  
}

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::AddToXMLdata(XmlDom& xml, const string& name,
                                const string& type, const string& buf,
                                bool persists, const string& label,
                                const string& filename) {
    stringstream str;
    str << buf;

    return AddToXMLdata(xml, name, type, str, persists, label, filename);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::AddToXMLdata(XmlDom& xml, const string& name,
                                const string& type, stringstream& str,
                                bool persists, const string& label,
                                const string& filename) {
    stringstream mmstr;
    b64::encode(str, mmstr);
    int len = str.str().size();
    string mmstring = mmstr.str();
    const char *mmdata = mmstring.c_str();

    if (type=="")
      ShowError("Connection::AddToXMLdata(): no content-type");

    XmlDom bin = xml.Element(name, "\n");
    bin.Prop("content-type",     type);
    bin.Prop("content-length",   len);
    bin.Prop("content-encoding", "base64");

    if (persists)
      bin.Prop("expires", ExpirationPlusYear());

    if (label!="")
      bin.Prop("label", label);

    if (filename!="")
      bin.Prop("filename", filename);

    xmlNodeAddContent(bin.node, (XMLS)mmdata);
    xmlNodeAddContent(bin.node, (XMLS)"  ");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::LaunchThreadAndOrSalute(const map<string,string>& extra) {
#ifndef PICSOM_USE_PTHREADS
    return SendAcknowledgment(extra);
#else
    if (!pic_som->PthreadsConnection() || pic_som->ExpectSlaves())
      return SendAcknowledgment(extra);

    if (HasOwnThread()) {
      cout << "!!!!! " << DisplayString() << " !!!!!" << endl;
      return ShowError("Connection::LaunchThread() : pthread already set");
    }

    void *(*xxx)(void*) = PthreadLoop;
    int r = pthread_create(&Thread(), NULL, (CFP_pthread) xxx, this);

    return !r;
#endif // PICSOM_USE_PTHREADS
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::CancelThread() {
#ifndef PICSOM_USE_PTHREADS
  return true;
#else
  if (!HasOwnThread())
    return ShowError("Connection::CancelThread() cannot cancel main thread");
  if (pthread_equal(Thread(), pthread_self()))
    return ShowError("Connection::CancelThread() cannot cancel itself");

  WriteLog("Cancelling connection pthread /", OwnThreadIdentity());

  int r = pthread_cancel(Thread());
  if (r) {
    stringstream s;
    s << "/" << OwnThreadIdentity();
    return ShowError("Connection::CancelThread() pthread_cancel() failed"
		     " with ", s.str());
  }

  pthread_join(Thread(), NULL);
  HasOwnThread(false);

  return true;
#endif // PICSOM_USE_PTHREADS
}

/////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_PTHREADS

void *Connection::PthreadLoop() {
  // WriteLog("Entered PthreadLoop");

  HasOwnThread(true); // added 2014-10-12

  pic_som->BlockSignals();
  pic_som->ConditionallyAnnounceThread("Connection::PthreadLoop()");

  map<string,string> extra;
  if (SendAcknowledgment(extra))
    while (!IsClosed()) {
      pthread_testcancel();
      ReadAndRunXMLOrMRML();
    }

  HasOwnThread(false);

  return NULL;
}

#endif // PICSOM_USE_PTHREADS

///////////////////////////////////////////////////////////////////////////////

bool Connection::RfdOK() {
  if (child==-1)
    return !State(2);

#ifdef HAVE_WAITPID
  int status;
  pid_t pid = waitpid(child, &status, WNOHANG);
  bool exited = WIFEXITED(status), signalled = WIFSIGNALED(status);
  if (pid && (exited||signalled)) {
    Close(false);
    return false;
  }
 
  return true;
#else
  return ShowError("Warning: waitpid() not implemented");
#endif // HAVE_WAITPID
}

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ReadAndParseXML(bool& http, bool do_ack) {
    string msg = "Connection::ReadAndParseXML() : ";

    bool debug = debug_http>3;
    if (debug)
      WriteLog(msg+"entered: "+TypeStringStr(true)+
	       " child="+ToStr(child)+
	       " Rfd()="+ToStr(Rfd())+" State(2)="+ToStr(State(2))+
	       " state="+ToStr(state));

    http = false;

    DeleteXMLdoc();

    bool ok = true;
    string buf;

    if (type==conn_mpi_up || type==conn_mpi_down)
      ok = ReadXMLmpi(buf);
    
    else if (type==conn_up || type==conn_down)
      ok = ReadXMLsocketWithHeader(buf);

    else {
      ok = ReadXMLsocket(buf, http);
      if (buf=="")
	http = true;
    }

    if (!http && ok) {
      ok = ParseXML(buf);
      ConditionallyDumpXML(cout, false, xmldoc__);
    }

    bool is_slaveoffer = false;
    if (ok && xmldoc__) {
      XmlDom xml(xmldoc__);
      if (xml.Root().NodeName()=="slaveoffer")
	is_slaveoffer = true;    
    }

    if (ok && !http && do_ack && !is_slaveoffer)
      ok = SendAcknowledgment(map<string,string>());

    // if (is_slaveoffer) {
    //   int one = 1;
    //   if (setsockopt(wfd, SOL_TCP, TCP_NODELAY, &one, sizeof(one)))
    // 	return ShowError(msg+"setsockopt(TCP_NODELAY) failed");
    //   else
    // 	WriteLog(msg+"setsockopt(TCP_NODELAY) set for slave");
    // }

    if (debug)
      WriteLog(msg+"Returning http="+ToStr(http)
	       +" ok="+ToStr(ok)+" do_ack="+ToStr(do_ack)
	       +" is_slaveoffer="+ToStr(is_slaveoffer)
	       +" buf.size()="+ToStr(buf.size()));

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ReadXMLmpi(string& buf) {
    string msg = "Connection::ReadXMLmpi() : ";

    bool debug = debug_http>3;
    if (debug)
      WriteLog(msg+"entered: "+TypeStringStr(true));
    
    buf = "";

#ifdef PICSOM_USE_MPI
    auto r = ReadInMPI();
    if (r.first!="xml")
      return false;

    buf = r.second;

    return true;			 
#else
    return false;
#endif // PICSOM_USE_MPI
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ReadXMLsocketWithHeader(string& buf) {
    string msg = "Connection::ReadXMLsocketWithHeader() "+LogIdentity()+" : ";

    bool debug = debug_reads;
    if (debug)
      WriteLog(msg+"entered: "+TypeStringStr(true)+
	       " child="+ToStr(child)+
	       " Rfd()="+ToStr(Rfd())+" State(2)="+ToStr(State(2))+
	       " state="+ToStr(state));
    buf = "";

    if (IsClosed())
      return false;

    char hdr[16];
    ssize_t n = read(Rfd(), hdr, 16);
    if (n!=16)
      return ShowError(msg+"read() failed with n="+ToStr(n)
		       +" errno="+ToStr(errno));

    if (memcmp(hdr, "PSMP", 4))
      return ShowError(msg+"not PSMP");

    uint32_t tint = 0;
    memcpy(&tint, hdr+4, 4);
    if (tint!=3)
      return ShowError(msg+"not xml");

    uint64_t ss = 0, rr = 0;
    memcpy(&ss, hdr+8, 8);

    if (debug_reads)
      cout << TimeStamp() << ">>> /" << ThreadIdentifierUtil()
	   << " PSMP " << tint << " " << ss << " <<<" << endl;

    bool ok = true;
    if (ss) {
      char *cbuf = new char[ss];
      size_t i = 0;
      while (ok && rr<ss) {
	ssize_t r = read(Rfd(), cbuf+rr, ss-rr);
	if (r<1)
	  ShowError(msg+"r="+ToStr(r)+" i="+ToStr(i));
	if (r<0)
	  ok = false;
	else
	  rr += r;
	i++;
      }
      if (ok)
	buf = string(cbuf, ss);
      delete [] cbuf;
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ReadXMLsocket(string& buf, bool& http) {
    string msg = "Connection::ReadXMLsocket() : ";

    bool debug = debug_http>3, xdebug = false;
    if (debug)
      WriteLog(msg+"entered: "+TypeStringStr(true)+
	       " child="+ToStr(child)+
	       " Rfd()="+ToStr(Rfd())+" State(2)="+ToStr(State(2))+
	       " state="+ToStr(state));

    if (IsClosed())
      return true;

    if (xdebug)
      WriteLog(msg+"starting to read");

    static set<string> mlist;
    if (mlist.empty()) {
      mlist.insert("GET");
      mlist.insert("POST");
      mlist.insert("PUT");
      mlist.insert("OPTIONS");
      mlist.insert("HEAD");
      mlist.insert("DELETE");
      mlist.insert("TRACE");
      mlist.insert("CONNECT");
    }

    http_request.clear();
    // int http_content_length = -1;

    const size_t tmpbufsize = 50*1024*1024;
    char *tmpbufp = new char[tmpbufsize];

    bool very_short = false;
    char root[1000] = "";
    char *bufptr = NULL;
    size_t bufptrlen = 0, i = 0, nrealloc = 0, start = 0;

    uint32_t tint = 0;
    uint64_t tss = 0;

    for (;; i++) {
      if (debug)
	WriteLog(" i="+ToStr(i));

      if (!RfdOK()) {
	if (debug)
	  WriteLog(" !RfdOK() -> return false");
	free(bufptr);
	delete tmpbufp;
	return false;
      }

      ssize_t n = 0;
      bool done = false;
#ifdef HAVE_WINSOCK2_H
      if (IsSocket()) {
	n = recv(Rfd(), tmpbufp, tmpbufsize, 0);
	done = true;
      }
#endif // HAVE_WINSOCK2_H
      if (!done)
	n = read(Rfd(), tmpbufp, tmpbufsize);

      if (i==0 && strncmp(tmpbufp, "PSMP", 4)==0) {
	memcpy (&tint, tmpbufp+4, 4);
	memcpy (&tss, tmpbufp+8, 8);
	start = 16;
	if (debug)
	  WriteLog("PSMP "+ToStr(tint)+" "+ToStr(tss));
      }

      // if (n==0 && i>0 && start && bufptrlen!=start+tss) {
      // 	if (debug)
      // 	  WriteLog(" ... sleeping ... bufptrlen="+ToStr(bufptrlen)
      // 		   +" strt+tss="+ToStr(start+tss));
      // 	NanoSleep(0, 100000); // 100us
      // 	continue;
      // }

      if (n<1) {
	if (http)
	  break;

	free(bufptr);
	delete tmpbufp;

	if (debug)
	  WriteLog(" n="+ToStr(n)+" -> return "+(n==0?"true":"false"));

	if (false && n==0) // check disabled 2015-04-21: now never fails...
	  is_failed = true;

	return n==0;
      }

      if (http) {
	string tmp(tmpbufp, n);
	http_request += tmp;

	bool cont = HttpServerContentMissing(http_request);

	if (debug_http)
	  WriteLog(msg+"reading many HTTP chunks, n="+
		   ToStr(n)+" cont="+ToStr(cont)+" ["+tmp+"]");

	if (cont)
	  continue;
	else
	  break;
      }

      if (!i) {
	string first(tmpbufp+start, n-start);

	if (debug)
	  WriteLog("["+first+"]");

	size_t nl = first.find('\r');
	if (nl!=string::npos) {
	  string tmp(first);
	  tmp.erase(nl);
	  string method(tmp);
	  size_t ws = method.find_first_of(" \t");
	  method.erase(ws!=string::npos ? ws : 0);

	  if (mlist.find(method)!=mlist.end()) {
	    http_request = first;
	    http = true;

	    /*http_content_length=*/HttpServerSolveContentLength(http_request);

	    bool cont = !HttpServerFullHeaderFound(http_request) ||
	      HttpServerContentMissing(http_request);

	    if (debug)
	      WriteLog(msg+"read first HTTP chunk,"
		       " n="+ToStr(n)+" cont="+ToStr(cont)+" ["+tmp+"]");
	    if (cont)
	      continue;
	    else
	      break;
	  }
	}
      }

      // size_t ol = bufptr ? strlen(bufptr) : 0;
      // char *newbuf = new char[ol+n+1];
      // if (bufptr)
      //   strcpy(newbuf, bufptr);
      // strncpy(newbuf+ol, tmpbufp, n);
      // newbuf[ol+n] = 0;
      // delete bufptr;
      // bufptr = newbuf;
      const char *obufptr = bufptr;
      bufptr = (char*)realloc(bufptr, bufptrlen+n+1);
      memcpy(bufptr+bufptrlen, tmpbufp, n);
      bufptrlen += n;
      bufptr[bufptrlen] = 0;
      if (obufptr && bufptr!=obufptr)
	nrealloc++;

      if (!*root) {
	char *ptr = bufptr+start;
	while ((ptr = strchr(ptr, '<'))) {
	  ptr++;
	  if (*ptr!='?' && *ptr!='!') {
	    char *eptr = strpbrk(ptr, " \t\n/>");
	    if (eptr) {
	      strncpy(root, ptr, eptr-ptr);
	      root[eptr-ptr] = 0;

	      eptr = strpbrk(eptr, ">");
	      if (eptr && eptr[-1]=='/')
		very_short = true;
	      break;
	    }
	  }
	}
      }

      if (debug && *root)
	WriteLog("[[[["+string(root)+"]]]]");

      if (very_short)
	break;
  
      bool do_cont = !*root || n==(int)tmpbufsize;
      if (!do_cont && *root) {
	size_t l = strlen(root);
	char *eptr = bufptr+start;
	eptr += strlen(eptr)-1;
	while (eptr>bufptr+start && isspace(*eptr))
	  eptr--;
	if (eptr-l-2<=bufptr+start || *eptr!='>' ||
	    eptr[-l-2]!='<' || eptr[-l-1]!='/'
	    || strncmp(eptr-l, root, l))
	  do_cont = true;
      }

      if (!do_cont)
	break;

      if (debug)
	WriteLog(msg+"reading many XML chunks");
    }

    delete tmpbufp;

    SetAccessTime();

    if (xdebug)
      WriteLog(msg+"content size="+ToStr(bufptrlen)+ " read in "+ToStr(i+1)+
	       " chunks in "+ToStr(float(bufptrlen)/tmpbufsize)+" * "+
	       ToStr(tmpbufsize)+" input buffer with "+ToStr(nrealloc)+
	       " true reallocs");

    if (bufptr) {
      const char *bufptr2 = bufptr+start;
      int buflen2 = (int)strlen(bufptr2);
      while (*bufptr2==' ' || *bufptr2=='\t' ||
	     *bufptr2=='\n' || *bufptr2=='\r') {
    	bufptr2++;
    	buflen2--;
      }

      buf = string(bufptr2, buflen2);
      free(bufptr);
      
      return true;
    }

    buf = http_request;
    size_t p = buf.find("\n\n"), pl = 2;
    if (p==string::npos) {
      pl = 4;
      p = buf.find("\n\r\n\r");
      if (p==string::npos)
	p = buf.find("\r\n\r\n");
    }
    if (p!=string::npos) {
      buf.erase(0, p+pl);
      return true;
    }

    buf = "";

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ParseXML(const string& buf) {
    string msg = "Connection::ParseXML() : ";

    DeleteXMLdoc();
    if (buf=="")
      return ShowError(msg+"called with empty buffer");

    bool debug = debug_http>3, xdebug = false;
    if (debug)
      WriteLog(msg+"entered: "+TypeStringStr(true)+
	       " child="+ToStr(child)+
	       " Rfd()="+ToStr(Rfd())+" State(2)="+ToStr(State(2))+
	       " state="+ToStr(state));

    bool ok = true;

    SetXMLdoc(xmlParseMemory(buf.c_str(), buf.size()));
    if (!HasXMLdoc()) {
      cerr << msg << "xmlParseMemory() failed" << endl
	   << msg << "BEGIN DUMP" << endl
	   << buf.size() << " : [" << buf << "]" << endl;
      ShowError(msg+"END DUMP");
      ok = false;
    }

    if (xdebug)
      WriteLog(msg+"parsed memory");

    return ok;
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::ReadAndRunXMLOrMRML() {
  InitializeSome(false);
  bool http = false, do_ack = !Picsom()->IsSlave() && !Picsom()->HasSlaves();
  if (!ReadAndParseXML(http, do_ack)) {
    Close();
    return false;
  }

  if (type==conn_http_server) {
    // check disabled 2012-12-28 due to some empty requests that caused
    // unnecessary message to be shown...
    // if (!http)
    //   ShowError("Connection::ReadAndRunXMLOrMRML() : http==false");

    return HttpServerProcess(false);
  }

  //   if (DebugLocks())
  //     WriteLog("Calling RunXML() in PthreadLoop");

  if (!IsFailed())
    RunXML();

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::RunXML() {
  if (!xmldoc__) {
    BackTraceMeHere(cout);
    return ShowError("Connection::RunXML() : xmldoc__==NULL");
  }

  bool ok = TryToRunXML();

  if (!ok) {
    string dump = XML2String(xmldoc__, true);
    WriteLog("Failed to run XML document:\n", dump);
    ShowError("Connection::RunXML() : TryToRunXML() failed");

    xmlNsPtr   ns   = NULL;
    xmlDocPtr  doc  = xmlNewDoc((XMLS)"1.0");
    xmlNodePtr root = NewDocNode(doc, ns, "result");
    xmlDocSetRootElement(doc, root);
    AddTag(root, ns, "error", "Operation failed.");
    WriteOutXml(doc);
    xmlFreeDoc(doc);
  }
  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::TryToRunXML() {
  xmlNodePtr root = xmlDocGetRootElement(xmldoc__);
  if (!root)
    return ShowError("Connection::TryToRunXML() : root==NULL");

  string rname = NodeName(root);
  if (rname=="request")
    rname = "objectrequest";

  msgtype = MsgType(rname);

  string bounce_ok = GetProperty(root, "bounce_ok");
  bounce = IsAffirmative(bounce_ok);

#ifndef PICSOM_USE_PTHREADS
  string pthread = "no threads";
#endif // PICSOM_USE_PTHREADS

  if (DebugBounces())
    cout << "************** /" << OwnThreadIdentity()
	 << " in TryToRunXML() bounce=" << bounce << endl;
 
  switch (msgtype) {
  case msg_response:
    return TryToRunXMLresponse(root->children);

  case msg_objectrequest:
    return TryToRunXMLrequest(root->children);

  case msg_upload:
    return TryToRunXMLupload(root->children);

  case msg_slaveoffer:
    return TryToRunXMLslaveoffer(root->children);

  case msg_command:
    return TryToRunXMLcommand(root->children);

  case msg_acknowledgment:
  default:
    return ShowError("Connection::TryToRunXML() : wrong message type: <",
		     rname, ">");
  }
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::TryToRunXMLrequest(xmlNodePtr node) {
  string msg = "TryToRunXMLrequest() : ";

  while (node && node->type!=XML_ELEMENT_NODE)
    node = node->next;

  if (!node)
    return ShowError(msg+"no ELEMENT children");

  bool angry = !Picsom()->IsSlave();
  if (!CheckClient(XmlDom(node), angry))
    return false;

  string otype = NodeName(node), oname = NodeChildContent(node), ospec;

  if (otype=="imageinfo")
    otype = "objectinfo";
  if (otype=="imagelist")
    otype = "objectlist";
  if (otype=="image")
    otype = "object";

  object_type ot = ObjectType(otype);
  if (ot<0)
    return ShowError(msg+"unknown objecttype: "+ObjectTypeP(ot, true)
		     +" <"+otype+">");

  if (ot==ot_command) {
    oname = GetProperty(node, "name");
    if (oname=="")
      return ShowError(msg+"<command> without name");
  }

  if (ot==ot_analysis) { // this is the place where slaves come...
    static int anum = 0;
    string aid = "slave-analysis-"+ToStr(anum++);
    oname = GetProperty(node, "name");
    if (oname=="")
      oname = Picsom()->CreateAnalysis(node, aid);
    if (oname=="")
      return ShowError(msg+"CreateAnalysis() failed");
  }

  if (ot==ot_thread) {
    oname = GetProperty(node, "name");
    if (oname=="")
      return ShowError(msg+"no name in <thread>");
    ospec = GetProperty(node, "command");
    if (ospec=="")
      return ShowError(msg+"no command in <thread>");
  }

  node = node->next;

  if (NodeName(node)=="objectspec" && node->children) {
    ospec = NodeChildContent(node);
    node = node->next;
  }

  ObjectRequest(ot, oname, ospec);

  if (!TryToRunXMLcommon(node))
    return false;

  return RunPartTwo();
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::TryToRunXMLresponse(xmlNodePtr node) {
  const string msg = "Connection::TryToRunXMLresponse() : ";

  if (!TryToRunXMLcommon(node))
    return ShowError(msg+" TryToRunXMLcommon() failed");

  if (!query->ProcessObjectResponseList(node))
    return ShowError(msg+" ProcessObjectResponseList() failed");

  // query->ProcessObjectResponseList() must be run before 
  // query->ProcessSelectedAspects() to ensure that the list of seen 
  // objects is up to date.
  if (!query->ProcessSelectedAspects(node))
    return ShowError(msg+" ProcessSelectedAspects() failed");

  if (!query->ProcessClassStates(node))
    return ShowError(msg+" ProcessClassStates() failed");

  if (!query->ProcessTextQueryOptions(node))
    return ShowError(msg+" ProcessTextQueryOptions() failed");

  if (!RunPartTwo())
    return ShowError(msg+" RunPartTwo() failed");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::CheckClient(XmlDom xml, bool angry) {
  string msg = "Connection::CheckClient() : ";

  string cli = GetXMLkeyValue(xml.node, "client");

  if (cli!="") {
    int r;
    if (!Interpret("client", cli, r) || r!=1)
      return ShowError(msg+"setting client failed");

  } else if (angry)
    return ShowError(msg+"<client> missing");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::TryToRunXMLcommon(xmlNodePtr node) {
  string msg = "Connection::TryToRunXMLcommon() : ";

  if (query)
    return ShowError(msg+"query!=NULL");

  string id  = GetXMLkeyValue(node, "identity");
  string db  = GetXMLkeyValue(node, "database");
  string ide = id;

  if (ide=="UNKNOWN") {
    ide = "";
    WarnOnce("This server is sloppy and accepts UNKNOWN identities");
  }

  bool angry = !Picsom()->IsSlave() && GetMsgType()!=msg_slaveoffer;

  if (!CheckClient(node, angry))
    return false;

  bool ok = true;

  Query *old_query = ok&&ide!="" ? pic_som->FindQuery(ide, this) : NULL;
  if (ok&&ide!="" && !old_query)
    ok = ShowError(msg+"FindQuery(", ide, ") failed for client ",
		   ClientAddress());

  if (old_query && !old_query->CheckPermissions(*this))
    ok = ShowError(msg+"+++ Permissions failed identity=<", ide,
		   "> client=<", ClientAddressCookie(), ">");
  if (!ok)
    return false;

  Query *qqq = NULL;

  if (GetMsgType()!=msg_slaveoffer) {
    if ((obj_type==ot_no_object && db=="") || !old_query) {  
      // response (except database selection response) or databaselist
      qqq = query = new Query(pic_som);
      QueryOwn(old_query==NULL);
      query->Parent(old_query);
      query->SetIdentity();
      query->FirstClientAddress(ClientAddress());
      if (old_query) {
	query->CopySome(*old_query, true, false, false);
	old_query->CloseVideoOutput();
      }

    } else                                      // request...
      query = old_query;

    if (!query || query->Identity()=="")
      return ShowError(msg+"no query / identity");
  }

  if (query && !query->HasAnalysis()) // obs!  Why don't we wait here?
    query->WaitUntilReady();

  Analysis *ana = NULL;

  do {
    while (node && node->type!=XML_ELEMENT_NODE)
      node = node->next;
    if (!node)
      break;

    string key = NodeName(node);
    string val = GetXMLchildContent(node);
    
    // cout << "*** [" << key << "]=[" << value << "]" << endl;

    static set<string> skipkeys;
    if (skipkeys.empty()) {
      skipkeys.insert("identity");
      skipkeys.insert("client");
      skipkeys.insert("imageresponselist");
      skipkeys.insert("keywords");
      skipkeys.insert("classstates");
      skipkeys.insert("selectedaspects");
      skipkeys.insert("status");
      skipkeys.insert("slavekey");
      skipkeys.insert("jobid");
    }

    bool skip = val=="" || skipkeys.find(key)!=skipkeys.end();
    if (GetMsgType()==msg_upload && (key=="image" || key=="object"))
      skip = true;

    if (!skip && !PicSOM::DoInterpret(key, val, pic_som, qqq, this, this,
				      ana, NULL, true, true))
      ok = false;

    if (qqq && !ana)
      ana = qqq->GetAnalysis();      

  } while (ok && (node = node->next));

  if (ok) {
    if (query)
      query->SetLastAccessTimeNow();
    if (!old_query && qqq && qqq->GetDataBase())
      pic_som->AppendQuery(qqq, true);

  } else {
    if (qqq && old_query)
      old_query->DeleteChild(qqq);
    else
      delete qqq;
    query = NULL;
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool Connection::TryToRunXMLupload(xmlNodePtr node) {
  if (!TryToRunXMLcommon(node))    
    return false;

  const char *errhead = "Connection::TryToRunXMLupload() : ";

  DataBase *db = query->GetDataBase();

  if (!db)
    return ShowError(errhead, "has to know database");

  list<upload_object_data> objects;

  for (; node; node=node->next)
    if (node->type==XML_ELEMENT_NODE) {
      string nname = (const char*)node->name;
      if (nname=="image" || nname=="object")
	objects.push_back(ExtractUploadObject(node));
    }

  if (objects.empty())
    return ShowError(errhead, "no objects were extracted");

  XmlDom dummy;
  bool zipped = true;
  list<string> feats, detects, segments;
  feats.push_back("*");
  vector<string> dummyodlist;
  if (!db->InsertObjects(objects, feats, detects, segments,
			 true, zipped, dummyodlist, dummy, ""))
    return ShowError(errhead, "DataBase::InsertObjects() failed");
  
  vector<int> all;
  for (auto i=objects.begin(); i!=objects.end(); i++)
    all.insert(all.end(), i->indices.begin(), i->indices.end());

  WriteLog(ToStr(all.size()), " objects uploaded and processed");

  target_type tt = target_no_target;
  for (size_t i=0; i<all.size(); i++) {
    int index = all[i];
    const string& label = db->Label(index);

    if (tt==target_no_target)
      tt = PicSOM::TargetTypeFileMasked(db->ObjectsTargetType(index));

    query->UploadLabel(label);
    if (!query->MarkAsSeenNoAspects(index, +1.0))
      return ShowError(errhead, "MarkAsSeenNoAspects() failed");
  }
  
  if (all.size()) {
    if (query->Nindices()==0) {
      // obs! should be possible to restrict to a subset of "*"
      if (query->Algorithms().empty())
	query->SelectIndices(NULL, "*");
      else
	for (size_t i=0; i<query->Algorithms().size(); i++)
	  query->SelectIndices(&query->Algorithms()[i], "*");
    }

    query->CreateIndexData(false);
    query->PlaceSeenOnMap();
    query->Convolve();
  }

  if (query->Target()==target_no_target)
    query->Target(tt);

  return RunPartTwo();
}

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::TryToRunXMLslaveoffer(xmlNodePtr node) {
    if (!TryToRunXMLcommon(node))
      return false;

    xmlNodePtr knode = FindXMLchildElem(node->parent, "slavekey");
    string slave_key = knode ? GetXMLchildContent(knode) : "";

    knode = FindXMLchildElem(node->parent, "jobid");
    string jobid = knode ? GetXMLchildContent(knode) : "";
  
    if (!Picsom()->TakeSlave(this, slave_key, jobid)) {
      Close();
      return ShowError("Connection::TryToRunXMLslaveoffer() failing");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::TryToRunXMLcommand(xmlNodePtr node) {
    if (!TryToRunXMLcommon(node))
      return false;

    string command;
    if (node->next && node->next->name)
      command = (const char*)node->next->name;

    if (command=="terminate") {
      WriteLog("*** terminating PicSOM::MainLoop() by command ***");
      Picsom()->Quit();
      return true;
    }

    return ShowError("Connection::TryToRunXMLcommand() : failed with \""
		     +command+"\"");
  }

///////////////////////////////////////////////////////////////////////////////

bool Connection::RunPartTwo() {
  // when this was Query::RunPartTwo() SingleWriteMultiReadLock(false)
  // was called when not entering SinglePass()

  msg_type qt = GetMsgType();

  bool to_single_pass = qt==msg_response && query->GetDataBase() &&
    (query->Nindices()||query->HasViewClass()||query->HasAnalysis());

  if (DebugLocks())
    WriteLog("Starting to process <", ObjectName(), ">");

  if (qt==msg_response || qt==msg_upload)
    ObjectRequest(ot_newidentity, "", "");
  
  SendObjectXML();

  if (to_single_pass && query->SetnameComment()!="")
    query->SetnameComment(DisplayString());

  Query *q = query;
  query = NULL;

  return to_single_pass ? q->ExecuteRunPartThree() : true;
}

///////////////////////////////////////////////////////////////////////////////

static string xml_to_string(XMLS p) {
  string ret = p ? (char*)p : "";
  delete p;
  return ret;
}

upload_object_data
Connection::ExtractUploadObject(xmlNodePtr node) const {
  upload_object_data obj;

  int clen = -1;

  string errhead = "Connection::ExtractUploadObject() : ";

  obj.use = GetProperty(node, "use");

  for (xmlNodePtr child = node->children; child; child=child->next) {
    if (child->type!=XML_ELEMENT_NODE)
      continue;

    const string key = NodeName(child);
    if (key=="imageinfo" || key=="objectinfo") {
      for (xmlNodePtr ichld = child->children; ichld; ichld=ichld->next)
	if (ichld->type==XML_ELEMENT_NODE) {
	  const string chld_name = NodeName(ichld);
	  if (chld_name=="url")
	    obj.url = xml_to_string(xmlNodeGetContent(ichld));
	  else if (chld_name=="page")
	    obj.page = xml_to_string(xmlNodeGetContent(ichld));
	  else if (chld_name=="date")
	    obj.date = xml_to_string(xmlNodeGetContent(ichld));
	}

    } else if (key=="text") {
      obj.text = xml_to_string(xmlNodeGetContent(child));

    } else if (key=="content") {
      obj.ctype      = GetProperty(child, "content-type");
      string clenstr = GetProperty(child, "content-length");
      if (clenstr=="")
	ShowError(errhead, "no content-length");
      else 
	clen = atoi(clenstr.c_str());

      string cenc  = GetProperty(child, "content-encoding");
      if (cenc!="" && cenc!="base64")
	ShowError(errhead, "non-base64 data");

      const string content = xml_to_string(xmlNodeGetContent(child));
      if (content=="")
	ShowError(errhead, "no content");

      else {
	istringstream str(content);
	ostringstream bin;
	b64::decode(str, bin);
	obj.data = bin.str();

	if (bin.str().length()!=(size_t)clen) {
	  char errtmp[100];
	  sprintf(errtmp, "bin.pcount()=%d clen=%d", (int)bin.str().size(),
		  clen);
	  ShowError(errhead, errtmp);
	}
      }

    } else
      ShowError(errhead, "failed with <image><", key, ">...</",
		key, "></image>");
  }

  if ((clen>0 || obj.data.length()>0) && clen!=(int)obj.data.length())
    ShowError(errhead, "failed with mime content's actual & declared length");

  return obj;
}

  /////////////////////////////////////////////////////////////////////////////
  
/// Deletes processed query if own.
  void Connection::DeleteQuery() {
    if (query_own && query) {
      // if (!Picsom()->FindQuery(query))
      //   delete query;
      // this commented out 2015-04-21, the whole thing should be reconsidered...
      // else
      // 	ShowError("Connection::DeleteQuery() : query <", query->Identity(),
      // 		  "> owned and held by PicSOM class simultaneously!");
    }
    query = NULL;
    query_own = true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  Query *Connection::NewQuery() {
    DeleteQuery();
    query_own = true;
    return query = new Query(pic_som);
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& Connection::QueryIdentity() const {
    static const string undef = "UNDEF";
    return query ? query->Identity() : undef;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::HttpClientGetString(const string& mtd, const string& path, 
				       const list<pair<string,string> >& hdrs,
				       const string& postdata,
				       string& content, string& ctype,
				       int redirsin, int tosec,
				       size_t maxsize, float maxtime) {
    string msg = "HttpClientGetString("+mtd+","+path+","+//atype+","+
      ToStr(postdata.size())+" bytes,"+ToStr(redirsin)+","+ToStr(tosec)+
      ","+ToStr(maxsize)+","+ToStr(maxtime)+") : ";

    int timeoutsec = tosec, timeoutsecdefault = 10;
    if (timeoutsec<0)
      timeoutsec = timeoutsecdefault;

    int redirs = redirsin;
    if (redirs<0)
      redirs = PICSOM_DEFAULT_REDIRS;

    if (mtd!="GET" && mtd!="POST")
      return ShowError(msg+"only GET and POST methods are allowed");

    content = ctype = "";

    if (type!=conn_http_client)
      return ShowError(msg+"not conn_http_client");

    stringstream ss;
    ss << mtd << " " << path << " HTTP/1.0" << "\r\n";

    string hostnameport = hostname;
    if (port!=80)
      hostnameport += ":"+ToStr(port);
    ss << "Host: " << hostnameport << "\r\n";

    bool content_type_set = false;
    for (auto a=hdrs.begin(); a!=hdrs.end(); a++) {
      ss << a->first << ": " << a->second << "\r\n";
      if (a->first=="Content-Type") // obs! lower case etc.
	content_type_set = true;
    }

    if (mtd=="POST") {
      if (!content_type_set)
	ss << "Content-Type: text/xml; charset=UTF-8\r\n";
      ss << "Content-Length: " << postdata.size() << "\r\n";
    }
    ss << "\r\n";
    if (mtd=="POST")
      ss << postdata;
    
    string str = ss.str();
    if (debug_http)
      cout << "Writing to remote HTTPD:" << endl << str;

    ssize_t nw = -1;
#ifdef HAVE_OPENSSL_SSL_H
    if (ssl)
      nw = SSL_write(ssl, str.c_str(), str.size());
    else
#endif // HAVE_OPENSSL_SSL_H
#ifdef HAVE_WINSOCK2_H
      nw = send(Wfd(), str.c_str(), str.size(), 0);
#else
      nw = write(Wfd(), str.c_str(), str.size());
#endif // HAVE_WINSOCK2_H

    if (nw<=0)
      return ShowError(msg+"write/send() to fd="+ToStr(Wfd())+
		       " failed, errno="+ToStr(errno)+" "+
		       string(strerror(errno)));

    // if (!Poll())
    //   return ShowError(msg+"poll() failed");

    // float maxtime = 120.0; // seconds
    timespec_t end_time, now_time;
    SetTimeNow(end_time);
    TimeAdd(end_time, maxtime);

    size_t bufsize = 1024*1024;
    char *buf = new char[bufsize];

    string hdrbody;
    for (;;) {
      if (!State(0, timeoutsec)) {
	delete [] buf;
        return ShowError(msg+"select() failed, timeoutsec="+ToStr(timeoutsec)
			 +", hdrbody.size()="+ToStr(hdrbody.size()));
      }

      ssize_t nr = -1;
#ifdef HAVE_OPENSSL_SSL_H
      if (ssl)
	nr = SSL_read(ssl, buf, bufsize);
      else
#endif // HAVE_OPENSSL_SSL_H
#ifdef HAVE_WINSOCK2_H
	nr = recv(Rfd(), buf, bufsize, 0);
#else
	nr = read(Rfd(), buf, bufsize);
#endif // HAVE_WINSOCK2_H

      if (debug_http>1)
	cout << "Read " << nr << " bytes from remote HTTPD, total "
	     << hdrbody.size() << endl;
      if (nr<0) {
	delete [] buf;
        return ShowError(msg+"read/recv() failed");
      }
      if (!nr)
        break;

      hdrbody += string(buf, nr);
      if (hdrbody.size()>maxsize) {
	delete [] buf;
        return ShowError(msg+"download size exceeded maxsize="+
			 ToStr(maxsize));
      }

      SetTimeNow(now_time);
      if (MoreRecent(now_time, end_time)) {
	delete [] buf;
        return ShowError(msg+"download time exceeded maxtime="+
			  ToStr(maxtime));
      }
    }
    delete [] buf;

    http_headers_t hlines;
    if (!HttpParseHeaders(hdrbody, hlines))
      return ShowError(msg+"Bad HTTP header!");
  
    int status = 0;
    string location, datastr;
    
    for (http_headers_t::const_iterator i=hlines.begin();
         i!=hlines.end(); i++) {
      string key = i->first, keylc = LowerCase(key);
      string value = i->second;
      if (key == "HTTP/1.0" || key == "HTTP/1.1")
        status = atoi(value.c_str());
      else if (keylc == "location")
        location = value;
      else if (keylc == "content-type")
        ctype = value;
      else if (key == "")
        datastr = value;
//       if (key != "")
//         cout << "[" << key << "] = " << value << endl;
    } 
    
    // http://tools.ietf.org/html/rfc2616#section-6.1.1
    switch (status) {
    case 200: { // OK
      content = datastr;
      return true;
    }
    case 301: // Moved Permanently
    case 302: // Found
    case 303: // See Other
    case 307: // Temporary Redirect
    case 308: // Permanent Redirect
      bool ret = false;
      if (!location.empty() && redirs>0)
	ret = DownloadString(pic_som, mtd, location, hdrs, postdata,
			     content, ctype, redirs-1, timeoutsec, maxsize,
			     maxtime);
      return ret;
    }

    content = datastr;

    return ShowError(msg+"Bad HTTP status code "+ToStr(status)+"!");
  }
 
  /////////////////////////////////////////////////////////////////////////////

  bool Connection::Poll(int type, int timeout) const {
#ifndef HAVE_WINSOCK2_H
    struct pollfd fds;
    fds.fd = Wfd();
    if (type == 0)
      fds.events = POLLIN;  // read
    else if (type == 1)
      fds.events = POLLOUT; // write
    else if (type == 2)
      // (From man poll) Stream socket peer closed connection, or shut
      // down writing half of connection.
      // The line below was changed as it didn't work in Macs:
      fds.events = POLLHUP; // was fds.events = POLLRDHUP; 
    else
      return ShowError("Connection::Poll(): unknown type="+ToStr(type));

    poll(&fds, (unsigned long)1, timeout);
    
    return fds.revents == fds.events;
#else
    return false;
#endif // HAVE_WINSOCK2_H
 }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::DownloadString(PicSOM* picsom, const string& mtd,
				  const string& url,
				  const list<pair<string,string> >& hdrs,
				  const string& postdata,
				  string& dstr, string& ctype, int redirs,
				  int timeout, size_t maxsize, float maxtime) {
    if (debug_http)
      picsom->WriteLog("Downloading "+url);
    
    // fixme: ugly hack for unavailable flickr images
    if (url.find("photo_unavailable.gif") != string::npos)
      return false;
    
    bool ssl = false;
    string proto, host, portstr, path;
    SplitURL(url, proto, host, portstr, path);
    if (proto=="https")
      ssl = true;
    int port = portstr!="" ? atoi(portstr.c_str()) : 0;
    Connection *c = CreateHttpClient(picsom, host, ssl, port);
    if (!c) 
      return false;

    bool ok = c->HttpClientGetString(mtd, path, hdrs, postdata,
				     dstr, ctype, redirs, timeout,
				     maxsize, maxtime);
    bool ret = c->Close();
    delete c;

    return ok&&ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::DownloadFileWithTricks(PicSOM* picsom, const string& url,
					    const list<pair<string,string> >&
					    hdrs,
					    string& ctype, int timeout) {
    bool yledl = false;
    if (url.find("http://yle.fi/aihe/artikkeli/")==0 ||
	url.find("http://areena.yle.fi/tv/")==0)
      yledl = true;

    if (yledl)
      return DownloadFileYleDl(picsom, url, ctype, timeout);

    return Connection::DownloadFile(picsom, url, hdrs, ctype, timeout);
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::DownloadFileYleDl(PicSOM* picsom, const string& url,
				       string& ctype, int /*timeout*/) {
    string msg = "Connection::DownloadFileYleDl("+url+") : ";

    string base = url;
    size_t p = base.rfind('/');
    if (p==string::npos)
      return ShowErrorS(msg+"url error");

    base.erase(0, p+1);
    p = base.rfind('.');
    if (p!=string::npos)
      base.erase(p);

    string out = picsom->TempDirPersonal("yle-dl")+"/"+base+".flv";
    string log = picsom->TempDirPersonal("yle-dl")+"/"+base+".log";
    
    vector<string> cmd {
      "yle-dl", "-o", out, url, ">", log, "2>&1"
	};

    if (picsom->ExecuteSystem(cmd, true, true, true))
      return ShowErrorS(msg+"yle-dl failed");

    ctype = "video/x-flv";

    return out;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::DownloadFile(PicSOM* picsom, const string& url,
				  const list<pair<string,string> >& hdrs,
				  string& ctype, int timeout,
				  size_t maxsize, float maxtime) {
    string imgstr;
    if (!DownloadString(picsom, "GET", url, hdrs, "", imgstr, ctype,
			-1, timeout, maxsize, maxtime))
      return "";

    string tmpname = picsom->TempDirPersonal();
    if (tmpname.size() && tmpname[tmpname.size()-1]=='/')
      tmpname.erase(tmpname.size()-1);
    
    size_t dotpos = url.find_last_of('.');
    for (size_t x=0; dotpos!=string::npos && x<3; x++) {
      string xx = "/?&";
      size_t pos = url.find_last_of(xx[x]);
      if (pos!=string::npos && pos>dotpos)
	dotpos = string::npos;
    }
    string ext = dotpos == string::npos ? "" : url.substr(dotpos);
    
    static int imgcount = 0;
#ifdef PICSOM_USE_PTHREADS
    static RwLock countlock;
    countlock.LockWrite();
#endif // PICSOM_USE_PTHREADS
    tmpname += "/download-"+ToStr(imgcount++)+ext;
#ifdef PICSOM_USE_PTHREADS
    countlock.UnlockWrite();
#endif // PICSOM_USE_PTHREADS
    StringToFile(imgstr, tmpname);

    return tmpname;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::InitializeSSL() {
    string msg = "Connection::InitializeSSL() : ";

    static bool done = false, ret = false;
    if (!done) {
      done = true;
      
#ifdef HAVE_OPENSSL_SSL_H
    SSL_library_init();
    ssl_ctx = SSL_CTX_new(TLSv1_client_method());
    if (!ssl_ctx)
      throw msg+"SSL_CTX_new(SSLv3_client_method()) failed";
#endif // HAVE_OPENSSL_SSL_H
    }

    // SSL_shutdown (ssl);  /* send SSL/TLS close_notify */
    // close (sd);
    // SSL_free (ssl);
    // SSL_CTX_free (ctx);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Connection::ShutdownSSL() {
    // string msg = "Connection::ShutdownSSL() : ";

#ifdef HAVE_OPENSSL_SSL_H
    SSL_CTX_free(ssl_ctx);
    ssl_ctx = NULL;
    EVP_cleanup(); 
    CRYPTO_cleanup_all_ex_data(); 
#endif // HAVE_OPENSSL_SSL_H

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<vector<string> > Connection::SparqlQuery(const string& url,
						const string& s,
						bool show_head) {
    string msg = "Connection::SparqlQuery("+url+","+s+") : ";

    list<vector<string> > res;

    string x = s;
    for (size_t i=0; i<x.size(); i++) {
      if (x[i]<47 || (x[i]>57&&x[i]<64) || (x[i]>90&&x[i]<97) || x[i]>122) {
	char hex[10];
	sprintf(hex, "%02x", x[i]);
	string hexs = string("%")+hex;
	x.replace(i, 1, hexs);
      }
    }
    string fullurl = url+"?query="+x, reply, ctype;

    list<pair<string,string> > hdrs;
    if (!DownloadString(Picsom(), "GET", fullurl, hdrs, "", reply, ctype)
	|| reply=="") {
      ShowError(msg+"DownloadString() failed");
      return res;
    }

    map<string,size_t> variable;

    XmlDom xml = XmlDom::FromString(reply);
    if (xml.Root().NodeName()!="sparql") {
      ShowError(msg+"root element should be <sparql>");
      return res;
    }
    XmlDom head = xml.Root().FirstElementChild("head");
    vector<string> varv;
    size_t i = 0;
    for (XmlDom var=head.FirstElementChild("variable");
	 var; var=var.NextElement("variable"), i++) {
      string vname = var.Property("name");
      variable[vname] = i;
      varv.push_back(vname);
    }

    if (show_head)
      res.push_back(varv);

    XmlDom results = xml.Root().FirstElementChild("results");
    for (XmlDom result=results.FirstElementChild("result");
	 result; result=result.NextElement("result")) {
      vector<string> v(variable.size());
      for (XmlDom binding=result.FirstElementChild("binding");
	   binding; binding=binding.NextElement("binding")) {
	string key = binding.Property("name");
	XmlDom cont = binding.FirstElementChild();
	string val = cont.FirstChildContent();
	v[variable[key]] = val;
      }
      res.push_back(v);
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<vector<string> > Connection::RdfQuery(const string& url) {
    string msg = "Connection::RdfQuery("+url+") : ";

    list<vector<string> > res;

    list<pair<string,string> > hdrs;
    hdrs.push_back(make_pair("Accept", "text/plain"));

    string reply, ctype;
    if (!DownloadString(Picsom(), "GET", url, hdrs, "", reply, ctype)
	|| reply=="") {
      ShowError(msg+"DownloadString() failed");
      return res;
    }

    for (size_t p=0; p!=string::npos; ) {
      size_t q = reply.find_first_of("\r\n", p);
      string s = reply.substr(p, q==string::npos ? string::npos : q-p);
      
      if (q==string::npos)
	break;

      if (s.size()>2 && s.substr(s.size()-2)==" .")
	s.erase(s.size()-2);

      size_t a = s.find(' '), b = s.find(' ', a+1);
      vector<string> v;
      v.push_back(s.substr(0, a));
      v.push_back(s.substr(a+1, b-a-1));
      v.push_back(s.substr(b+1));
      res.push_back(v);

      p = reply.find_first_not_of("\r\n", q);
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::GetOpenStackDownloadUrl(PicSOM*) {
    string msg = "Connection::GetOpenStackDownloadUrl() : ";

    const char *tenant_name = getenv("OS_TENANT_NAME");
    if (!tenant_name) {
      ShowError(msg+"OS_TENANT_NAME is not set");
      return "";
    }

    string s = "https://cloud.forgeservicelab.fi:8081/v1/AUTH_"+
      string(tenant_name);

    return s;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::GetOpenStackAuthToken(PicSOM *picsom) {
    string msg = "Connection::GetOpenStackAuthToken() : ";

    const char *auth_url    = getenv("OS_AUTH_URL");
    const char *tenant_name = getenv("OS_TENANT_NAME");
    const char *username    = getenv("OS_USERNAME");
    const char *password    = getenv("OS_PASSWORD");
    if (!auth_url || !tenant_name || !username || !password) {
      ShowError(msg+"database name starts with os:// but"
		" OS_AUTH_URL, OS_TENANT_NAME, OS_USERNAME or"
		" OS_PASSWORD is not set");
      return "";
    }
    string s = "auth_url="+string(auth_url)+" tenantName="+string(tenant_name)
      +" username="+string(username)+" password="+string(password);

    return GetOpenStackAuthToken(picsom, s);
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::GetOpenStackAuthToken(PicSOM *picsom, const string& s) {
    map<string,string> par;
    string a = s;
    for (;;) {
      if (a=="")
	break;
      while (a.find_first_of(" \t\n")==0)
	a.erase(0, 1);
      string b = a;
      size_t p = b.find_first_of(" \t\n");
      if (p!=string::npos) {
	b.erase(p);
	a.erase(0, p);
      } else
	a = "";

      if (b.find('=')!=string::npos) {
	pair<string,string> kv = SplitKeyEqualValueNew(b);
	par.insert(kv);
      }
    }

    XmlDom xml = XmlDom::Doc();
    XmlDom auth = xml.Root("auth");
    auth.Prop("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    auth.Prop("xmlns", "http://docs.openstack.org/identity/api/v2.0");
    auth.Prop("tenantName", par["tenantName"]);
    XmlDom passwordCredentials = auth.Element("passwordCredentials");
    passwordCredentials.Prop("username", par["username"]);
    passwordCredentials.Prop("password", par["password"]);

    list<pair<string,string> > hdrs;
    hdrs.push_back(make_pair("Content-Type", "application/xml"));
    hdrs.push_back(make_pair("Accept", "application/xml"));

    string url = par["auth_url"];
    if (url=="")
      return "";
    
    url += "/tokens";

    string xmlin = xml.Stringify(), xmlout, ctype;
    xml.DeleteDoc();

    if (debug_http)
      cout << xmlin << endl;

    if (!DownloadString(picsom, "POST", url, hdrs, xmlin, xmlout, ctype))
      return "";

    if (debug_http)
      cout << xmlout << endl;

    XmlDom tokxml = XmlDom::FromString(xmlout);
    XmlDom tokelem = tokxml.Root().FindChild("token");
    string token = tokelem.Property("id");
    tokxml.DeleteDoc();

    return token;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::EscapedMPIport() const {
#ifdef PICSOM_USE_MPI
    string s = mpi_port;
#else
    string s = "no-mpi-support";
#endif // PICSOM_USE_MPI
    for (;;) {
      size_t p = s.find(';');
      if (p==string::npos)
	break;
      s.replace(p, 1, "%3B");
    }

    return s;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Connection::UnEscapeMPIport(const string& u) {
    string s = u;
    for (;;) {
      size_t p = s.find("%3B");
      if (p==string::npos)
	break;
      s.replace(p, 3, ";");
    }

    return s;
  }

  /////////////////////////////////////////////////////////////////////////////

  // Connection::() {
  // 
  // }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

