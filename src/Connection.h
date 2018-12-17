// -*- C++ -*-  $Id: Connection.h,v 2.132 2018/10/05 15:29:57 jormal Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_CONNECTION_H_
#define _PICSOM_CONNECTION_H_

#include <XMLutil.h>
#include <RemoteHost.h>
#include <PicSOM.h>

#ifdef HAVE_EXT_STDIO_FILEBUF_H
#include <ext/stdio_filebuf.h>
#endif // HAVE_EXT_STDIO_FILEBUF_H

#if PICSOM_USE_CSOAP
#include <nanohttp/nanohttp-logging.h>
#include <libcsoap/soap-server.h>
#include <libcsoap/soap-client.h>
#include <libcsoap/soap-service.h>
#include <libcsoap/soap-router.h>
#endif // PICSOM_USE_CSOAP

#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#endif // HAVE_OPENSSL_SSL_H

#ifdef HAVE_MPI_H
//#define PICSOM_USE_MPI 1
#endif // HAVE_MPI_H

#ifdef PICSOM_USE_MPI
#include <mpi.h>
#endif // PICSOM_USE_MPI

// #define PICSOM_USE_READLINE
#ifdef PICSOM_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
/**
   A global variable containing the command line history.
   Used by the GNU readline library.
*/
extern HIST_ENTRY **history_list ();
#endif // PICSOM_USE_READLINE

#define PICSOM_DEFAULT_REDIRS 10

// timeout poll after 1 minute
#define PICSOM_POLL_TIMEOUT   (60*1000)

static const string Connection_h_vcid =
  "@(#)$Id: Connection.h,v 2.132 2018/10/05 15:29:57 jormal Exp $";

namespace picsom {
  typedef list<pair<string,string> > http_headers_t;

  /**
     Client connections.
  */
  class Connection {
  public:
    /// Describes the source of information.
    enum connection_type {
      conn_none, conn_terminal, conn_file, conn_stream,
      conn_pipe, conn_listen, conn_socket,
      conn_http_server, conn_http_client,
      conn_peer, conn_up, conn_down,
      conn_soap_server, conn_soap_client,
      conn_mpi_listen, conn_mpi_down, conn_mpi_up,
      conn_nonclosed=127, conn_closed=128
    };

    /// Describes the content of information
    enum connection_protocol {
      xml_language, mrml_language, line_language, unspecified_language
    };

    /// Describes 
    enum connection_state {
      idle, active, blocked, inrun, _closed
    };

    ///
    class proxy_data_t {
    public:
      ///
      proxy_data_t(const string& u) : url(u) {}

      ///
      string url;

      ///
      list<string> in;

      ///
      list<string> out;
    }; // class proxy_data_t 

    /// The constructor.
    Connection(PicSOM*);

    /// Poison.
    Connection() { SimpleAbort(); }

    /// The destructor.
    virtual ~Connection();

    /// Link upwards.
    PicSOM *Picsom() const { return pic_som; }

    /// Removes client and other information.
    void InitializeSome(bool);

    /// Like Simple::Dump()...
    void Dump(ostream& os = cout) const;

    /// Dumps relevant information.
    void DumpRequestInfo(ostream& = cout) const;

    /// Is it a terminal or otherwise interactive.
    bool Interactive() const { return interactive; }

    /// Sets the interaction mode to non default.
    void Interactive(bool i) { interactive = i; }

    /// Gives reference to servant's address.
    const RemoteHost& Servant() const { return servant; }

    /// Gives reference to client's address.
    const RemoteHost& Client() const { return client; }

    /// Sets client's address.
    void ClientAddress(const string& a) { client.Address(a); }

    /// Returns client's address.
    const string& ClientAddress() const { return client.Address(); }

    /// Sets client's cookie.
    void ClientCookie(const string& a) { client.Cookie(a); }

    /// Gets client's cookie.
    const string& ClientCookie() const { return client.Cookie(); }

    /// Gets's client's address:cookie.
    string ClientAddressCookie() const { return client.AddressCookie(); }

    /// Searches for <client> in XML and checks permissions.
    bool CheckClient(XmlDom, bool);

    /// Servant' and Client's addresses if socket.
    string Addresses(bool = false) const;

    /// Sets identity string.
    void Identity(const string& txt) { identity_str = txt; }

    /// Returns identity string.
    const string& Identity() const { return identity_str; }

    /// What is it doing now?
    connection_state State() const { return state; }

    /// Type of connection.
    connection_type Type() const { return type; }

    /// Type in four char or long form.
    string TypeStringStr(bool = false) const;

    ///
    int Port() const { return port; }

    /// Returns true if attached stream can be read/write/exception.
    bool State(int,int=0) const;

    /// State in "rwe" string form.
    string StateString() const;

    /// All info in one string.
    string DisplayString() const {
      const char *s = DisplayStringM();
      string ret = s;
      delete s;
      return ret;
    }

    /// All info in one string. Delete after use!
    const char *DisplayStringM() const;

    /// Conditionally notifies creations and closings.
    void Notify(bool = true, const char* = NULL, bool = false) const;

    /// Open a connection to terminal.
    static Connection *CreateTerminal(PicSOM*);

    /// Open a connection for reading a file.
    static Connection *CreateFile(PicSOM*, const char*);

    /// Open a connection for reading a stream.
    static Connection *CreateStream(PicSOM*, istream&);

    /// Open a connection for reading and writing a stream.
    static Connection *CreatePipe(PicSOM*, const vector<string>&,
                                  bool = false, bool = true);

    /// Open a socket for listening a port.
    static Connection *CreateMPIListen(PicSOM*);

    /// Open a socket for listening a port.
    static Connection *CreateMPI(PicSOM*, const string&, bool);

    /// Open a socket for listening a port.
    static Connection *CreateListen(PicSOM*, int, int);

    /// Open a socket for listening a port.
    static Connection *CreateSocket(PicSOM*, int, const RemoteHost&);

    /// Open an uplink for listening a port.
    static Connection *CreateUplink(PicSOM*, const string&, bool, bool);

    /// Open HTTP client port.
    static Connection *CreateHttpClient(PicSOM*, const string&,
					bool = false, int = 0);

    /// Open a SOAP client.
    static Connection *CreateSoapClient(PicSOM*, const string&);

    /// Open a SOAP server.
    static Connection *CreateSoapServer(PicSOM*, int);

#ifdef PICSOM_USE_CSOAP
    ///
    static void SoapServerSingle(bool v) { soap_server_single = v; }

    ///
    imagedata SoapClientGetFrame(const string&, const string&);

    /// Helper for CreateSoapServer().
    bool SoapRouterRegisterService(SoapRouter*, 
 				   SoapServiceFunc,
 				   const string&, const string&);

    /// Helper for CreateSoapServer().
    bool SoapServerRegisterRouter(SoapRouter*, const string&);

    /// Helper for CreateSoapServer().
    static void *SoapServerThread(void*);

    /// A tester routine.
    static herror_t SoapServer_sayHello(SoapCtx*, SoapCtx*);

    /// First real routine.
    static herror_t SoapServer_execute(SoapCtx*, SoapCtx*);

    /// A helper to make Python ZSI happy.
    static void AddSOAP_ENC_arrayType(XmlDom&);

#endif // PICSOM_USE_CSOAP

    ///
    static bool InitializeSSL();

    ///
    static bool ShutdownSSL();

    /// Reads contents of an URL.
    bool HttpClientGetString(const string&, const string&,
			     const list<pair<string,string> >&,
			     const string&, string&, string&,
			     int = -1, int = -1, size_t = 1024L*1024*1024,
			     float = 120);

    /// Runs steps needed for processing a HTTP request.
    bool HttpServerProcess(bool);

    /// Runs steps needed for processing a HTTP request.
    bool HttpServerProcessAfterRedirect(bool);

    /// Returns true if \r\n\r\n is found.
    bool HttpServerFullHeaderFound(const string&) const;

    /// Returns true if Content-Length is found but data is short.
    bool HttpServerContentMissing(const string&) const;

    /// Returns value of Host: request header field.
    string HttpServerSolveRequestField(const string&, const string&) const;

    /// Returns value of Host: request header field.
    string HttpServerSolveHost() const {
      return HttpServerSolveRequestField("Host", "nohost.xx");
    }

    /// Returns value of Host: request header field.
    string HttpServerSolveUserAgent() const {
      return HttpServerSolveRequestField("User-Agent", "no-agent");
    }

    /// Tries to find \r\nContent-Length: and returns it, otherwise -1.
    int HttpServerSolveContentLength(const string&) const;

    ///
    static bool HttpParseHeaders(const string&, http_headers_t&);

    /// Converts http_request to http_request_lines.
    bool HttpServerParseRequest();

    /// Returns MIME type of request.
    const string& HttpServerSolveRequestType(const string&);

    /// Returns a new url the client is to be redirected if any.
    string HttpServerSolveRedirection(const string&, const string&, bool&);

    /// Helper for the above one.
    string HttpServerAnalyseInteractiveRedirection(const string&);

    ///
    string HttpServerClickRedirection(const string&, const string&,
				      const string&);

    /// Adds <meta erf=""> tag.
    bool HttpServerMetaErf(XmlDom&, const Query*) const;

    /// Returns the URL of Ajax calls for given query.
    string HttpServerAjaxURL(const Query*) const;

    /// Sets http_analysis to point to named Analysis object.
    Analysis *HttpServerSolveAnalysis(const string&);

    /// Sets http_analysis to point to named Analysis object.
    bool HttpServerSolveAnalysisInner(const string&);

    ///
    list<pair<string,string> > HttpServerParseFormData();

    /// Performs HTTP redirect.
    bool HttpServerRedirect(const string&);

    /// Sends 404 Not Found
    bool HttpServerNotFound();

    ///
    bool HttpServerWriteOutSqlite3db(DataBase*);

    ///
    bool HttpServerWriteOutClassFile(DataBase*, const string&);

    ///
    bool HttpServerWriteOutFile(DataBase*, const string&, bool);

    ///
    bool HttpServerWriteOutFile(const string&, bool);

    /// This writes out HTTP header + imagedata.
    bool HttpServerWriteOutImage(const string& path, const string& mime);

    /// This writes out HTTP header + videodata.
    bool HttpServerWriteOutVideo(const string& path, const string& mime,
				 const string& range);

    /// This writes out HTTP header + trackdata.
    bool HttpServerWriteOutTrack(const string& path, const string& spec);

    /// This writes out HTTP header and then calls WriteOutXml().
    bool HttpServerWriteOut(const string&, const string&, bool,
			    const string&, int = 200, bool = true,
			    size_t = 0);

    /// This writes out HTTP header and then calls WriteOutXml().
    bool HttpServerWriteOut(const XmlDom& doc, const string&, bool, int = 200,
			    bool = true);

    /// Dummy message.
    XmlDom HttpServerErrorDocument(int) const;

    /// Startup page.
    XmlDom HttpServerStartupPage() const;

    /// Some xml page.
    XmlDom HttpServerGeneratePage(const string&,
                                  const list<string>& = list<string>());

    /// Some xml page.
    XmlDom HttpServerGeneratePageInner(const string&,
				       const list<string>&);

    /// D2I Multimedia REST API
    XmlDom HttpServerRestCaAddTask(const string&);

    /// D2I Multimedia REST implementation
    string HttpServerRestCaRunTask(const string&, const XmlDom&);

    /// D2I Multimedia REST implementation
    string HttpServerRestCaRunTaskAnalysis(const XmlDom&);

    /// D2I Multimedia REST implementation
    string HttpServerRestCaRunTaskBackendFeedback(const XmlDom&);

    /// D2I Multimedia REST
    XmlDom HttpServerRestCaQueryTaskStatus(const string&);

    /// D2I Multimedia REST implementation
    static bool RestCaCallback(const string&, Analysis*, const vector<string>&);

    /// D2I Multimedia REST implementation
    static bool RestCaCallbackTaskAnalysis(Analysis*, const vector<string>&);

    /// D2I Multimedia REST implementation
    static bool RestCaCallbackTaskBackendFeedback(Analysis*,
						  const vector<string>&);

    /// D2I Multimedia REST
    static bool RestCaMysql() { return restca_mysql; }

    /// OpenStack specifics...
    static string GetOpenStackAuthToken(PicSOM*, const string&);

    /// OpenStack specifics...
    static string GetOpenStackAuthToken(PicSOM*);

    /// OpenStack specifics...
    static string GetOpenStackDownloadUrl(PicSOM*);

    /// Creates a new query from ajax call.
    bool HttpServerCreateAjaxQuery();

    /// New query started by selecting the database.
    bool HttpServerNewQuery(const string&);

    /// response to from etc. actions.
    XmlDom HttpServerActionResponsePage();

    /// Various xml-stylesheets.
    XmlDom HttpServerStyleSheet(const string&) const;
    
    /// JavaScripts.
    string HttpServerJavaScript(const string&) const;
    
    /// PicSOM logo
    const pair<string,string>& HttpServerImagePicSOMlogo() const;

    /// Not available image
    const pair<string,string>& HttpServerImageNotAvailable() const;

    /// Not available video
    const pair<string,string>& HttpServerVideoNotAvailable() const;

    /// favicon image
    const pair<string,string>& HttpServerImageFavIcon() const;

    /// Splits /queryid/objectspec
    static pair<string,string>
    HttpServerQueryAndLabel(PicSOM*, const string&, bool = false,
			    bool = false, bool = false);

    /// Splits /databasename/objectspec
    static pair<string,string>
    HttpServerDataBaseAndLabel(PicSOM*, const string&, bool = false,
                               bool = false, bool = false);

    /// Splits /databasename/classes/classname
    static pair<string,string>
    HttpServerDataBaseAndClassFile(PicSOM*, const string&, bool = false,
				   bool = false);

    /// Splits /databasename/foo/bar
    static pair<string,string>
    HttpServerDataBaseAndFile(PicSOM*, const string&, bool = false,
			      bool = false);

    ///
    bool HttpServerDojo(const string&);

    ///
    bool HttpServerDoc(const string&);

    ///
    bool HttpServerDataBaseHtml(const DataBase*, const string&);

    ///
    bool HttpServerProxy(const string&, const string&, const string&);

    ///
    bool ConnectUplink(connection_protocol, const string&, bool);

#ifdef USE_MRML
    // for Analysis MRML implementation
    /// Re-uses old Opened uplink.
    bool ReOpenUplink(const char*);

    // sets session_id
    void setSessionID(string s) { session_id = s; }

    // gets session_id
    XMLS getSessionID() { return (XMLS)session_id.c_str(); }

#endif //USE_MRML

    /// Open a socket for listening a port.
    static Connection *CreateSocketOld(PicSOM*, int, const RemoteHost&);

    /// Extracts xml_version and pm_version from XML and sets protocol type
    bool ExtractIdentityAndSelectProtocol();

    /// Extracts xml_version and pm_version from XML.
    //bool ExtractIdentity(xmlDocPtr);

#ifdef USE_MRML
    /// Forms basis for MRML communication with Charmer
    bool MRMLCommunication();

    /// Performs official MRML Handshake (Charmer don't use this)
    bool MRMLGetServerProperties(  /* xmlNodePtr, xmlNsPtr */  );
    
    /// Client asks for anything nonstandard MRML
    bool MRMLGetConfiguration(xmlNodePtr, xmlNsPtr);

    /// Client asks for list of sessions for spesific user
    bool MRMLGetSessions(xmlNodePtr, xmlNodePtr, xmlNsPtr);
    
    /// Client wants to open new session
    bool MRMLOpenSession(xmlNodePtr, xmlNodePtr, xmlNsPtr);

    /// Configures MRML Session and adds acknowledge session to reply
    bool MRMLConfigureSession(xmlNodePtr, xmlNodePtr, xmlNsPtr);

    /// Renames MRML Session and adds acknowledge session to reply
    bool MRMLRenameSession(xmlNodePtr, xmlNodePtr, xmlNsPtr);

    bool MRMLAlgorithm(xmlNodePtr);

    /// Client wants to close session (!!! not yet implemented)
    bool MRMLCloseSession(xmlNodePtr, xmlNodePtr, xmlNsPtr) {
      ShowError(" bool MRMLCloseSession Not really implemented. ");
      return true;
    }

    /// Client asks for list of collections
    bool MRMLGetCollections(xmlNodePtr, xmlNsPtr);

    /// Client asks for list of algorithms
    bool MRMLGetAlgorithms(xmlNodePtr, xmlNodePtr, xmlNsPtr);

    /// Charmer wants to handshake
    //bool MRMLCharmerHandshake(xmlNodePtr, xmlNodePtr, xmlNsPtr);

    /// Add cui-time-message to reply document (are these needed?)
    void MRMLAddTime(xmlNodePtr, xmlNsPtr);

    /// Reads Charmer's Query parameters 
    bool MRMLQueryStep(xmlNodePtr, xmlNodePtr, xmlNsPtr);

    /// No idea of purpose, not yet implemented.
    bool MRMLUserData(xmlNodePtr, xmlNodePtr, xmlNsPtr) {
      ShowError(" bool MRMLUserData NOT YET IMPLEMENTED! ");
      return true;
    }
    /// No idea of purpose, not yet implemented.
    bool MRMLBeginTransaction(xmlNodePtr, xmlNodePtr, xmlNsPtr) {
      ShowError(" bool MRMLBeginTransaction NOT YET IMPLEMENTED! ");
      return true;
    }
    /// No idea of purpose, not yet implemented.
    bool MRMLEndTransaction(xmlNodePtr, xmlNodePtr, xmlNsPtr) {
      ShowError(" bool MRMLEndTransaction NOT YET IMPLEMENTED! ");
      return true;
    }
    /// No idea of purpose, not yet implemented.
    bool MRMLMpeg7qbe(xmlNodePtr nodein, xmlNodePtr nodeout, xmlNsPtr replyns){
      ShowError(" bool MRMLMpeg7qbe NOT YET IMPLEMENTED! ");
      if(nodein && nodeout && replyns) //hack so that compiler won't complain
	return true;
      return true;
    }

    /// user-relevance-element-list (list of images)
    bool MRMLUserRelevanceElementList(xmlNodePtr);

    /// user-relevance-element (single image information)
    bool MRMLUserRelevanceElement(xmlNodePtr);

#endif //USE_MRML

    /// True if servant's version is OK.
    bool XMLVersionOK() const;

    /// True if servant's version is OK.
    bool MRMLVersionOK() const;

    /// This is the end...
    bool Close(bool = true, bool = true);

    /// A helper
    bool DoClose(int);

    /// Called if select() tells data is available, but zero bytes can be read.
    bool ReadEofAndClose();

    /// Used when MPI connection accepted 
    bool CloseFDs();

    /// Returns true if connection is of socket type.
    bool IsSocket() const { 
      int t = type&conn_nonclosed;
      return t>=conn_pipe;
    }

    /// Returns true if connection is of http server type.
    bool IsHttpServer() const { 
      int t = type&conn_nonclosed;
      return t==conn_http_server;
    }

    /// Returns true if connection is demo http server.
    bool IsDemo() const { 
      return IsHttpServer() && DefaultXSL().size() &&
	DefaultXSL().front()=="demo";
    }

    ///
    size_t ReadableBytes();

    /// Returns true if it has been closed.
    bool IsClosed() const { return type&conn_closed; }

    /// Returns true if it has failed in ReadAndParseXML().
    bool IsFailed() const { return is_failed; }

    /// Returns true if Connection has its own thread.
    bool HasOwnThread() const { return has_own_thread; }

    /// Returns true if Connection has its own thread.
    void HasOwnThread(bool r) { has_own_thread = r; }

#ifdef PICSOM_USE_PTHREADS
    /// Returns Connection's own thread id.
    pthread_t Thread() const { 
      if (!has_own_thread)
 	SimpleAbort();
      return pthread;
    }

    /// Returns Connection's own thread id.
    pthread_t& Thread() { return pthread; }

    ///
    string OwnThreadIdentity() const {
      if (!has_own_thread)
	return "NO THREAD";
      // gettid() won't work if a wrong thread calls this...
      return ThreadIdentifierUtil(pthread, gettid());
    }
#else
    ///
    string OwnThreadIdentity() const {
      return "THREADLESS";
    }
#endif // PICSOM_USE_PTHREADS

    ///
    void CanPossiblyBeSelected(bool b) { can_possibly_be_selected = b; }

    /// Is it worth listening?
    bool CanBeSelected() const;

    /// File descriptor for reading.
    int Rfd() const { return rfd; }

    ///
    istream *RfdIstream() const { return rfdistream; }
    
    /// File descriptor for writing.
    int Wfd() const { return wfd; }

    ///
    ostream *WfdOstream() const { return wfdostream; }
    
    /// FILE pointer for reading a pure file.
    FILE *Rfile() const { return rfile; }

    /// Returns next line.  Delete after use!
    const char *ReadLineM();

    /// Sets access time NOW.
    void SetAccessTime() { Simple::SetTimeNow(last); }

    /// Increments number of queries by one.
    void IncrementQueries() { queries++; }

    /// Version of Connection.pm at the other end.
    //    int Version() const { return version; }
    
    /// Records the Connection.pm version.
    // void Version(int v) { version = v; }

    /// Sets debugging in object requests.
    static void DebugObjectRequests(bool d) { debug_objreqs = d; }

    /// Sets debugging in bouncing.
    static void DebugBounces(bool d) { debug_bounces = d; }

    /// Returns state of debugging in bouncing.
    static bool DebugBounces() { return debug_bounces; }

    /// Sets debugging in socket reads.
    static void DebugReads(bool d) { debug_reads = d; }

    /// Sets debugging in socket writes.
    static void DebugWrites(int d) { debug_writes = d; }

    /// Sets debugging of proxy actions.
    static void DebugProxy(int d) { debug_proxy = d; }

    /// Debugging of proxy actions.
    static int DebugProxy() { return debug_proxy; }

    /// Sets debugging of http headers.
    static void DebugHTTP(int d) { debug_http = d; }

    /// Debugging of http headers.
    static int DebugHTTP() { return debug_http; }

    /// Sets debugging of SOAP calls.
    static void DebugSOAP(int d) { debug_soap = d; }

    /// Sets debugging of socket creations and deletions.
    static void DebugSockets(bool d) { debug_sockets = d; }

    /// Debugging of socket creations and deletions.
    static bool DebugSockets() { return debug_sockets; }

    /// Sets debugging of connection states in SelectInput().
    static void DebugSelects(int d) { debug_selects = d; }

    /// This is called by SelectInput().
    static int DebugSelects() { return debug_selects; }

    /// Sets debugging of thread locking.
    static void DebugLocks(bool d) { debug_locks = d; }

    /// Gives value of debug_locks.
    static bool DebugLocks() { return debug_locks; }

    /// Sets expiration.
    static void DebugExpire(bool d) { debug_expire = d; }

    /// Sets sec.usec string for timeout in SelectInput().
    static void TimeOut(const string& s) { timeout = s; }

    /// Rteurns sec.usec string for timeout in SelectInput().
    static const string& TimeOut() { return timeout; }

    /// Interprets key=val.
    bool Interpret(const string&, const string&, int&);

    /// Is it now a response/request/upload or something else.
    msg_type GetMsgType() const { return msgtype; }

    /// Sets information on requested object.
    void ObjectRequest(object_type t, const string& l, const string& s) {
      obj_type = t;
      obj_name = l;
      obj_spec = s;
    }

    /// Return the type of requested object.
    object_type ObjectRequest() const { return obj_type; }

    /// Returns name of requested object.
    const string& ObjectName() const { return obj_name; }

    /// Sets additional object specifier.
    void ObjectSpec(const string& s) { obj_spec = s; }

    /// Returns additional object specifier.
    const string& ObjectSpec() const { return obj_spec; }

    /// Reads the XML acknowledgment.
    bool ReadAcknowledgment(bool = true);

    /// Sends an acknowledgment.
    bool SendAcknowledgment(const map<string,string>&);

    /// Sends a refusal with optional message.
    bool SendRefusal(const string& = "");

    /// Sends the object whose info is stored.
    bool SendObjectXML();

    /// This is the inner part of the above.
    bool SendObjectXMLinner(xmlNodePtr, xmlNsPtr);

    /// This is a wrapper.
    bool SendObjectXMLinner(const XmlDom& xml) {
      return SendObjectXMLinner(xml.node, xml.ns);
    }

    /// This is the last part of SendObjectXML().
    bool WriteOutXml(const XmlDom& doc, const string& hdr = "") {
      return WriteOutXml(doc.doc, hdr);
    }

    /// This is the last part of SendObjectXML().
    bool WriteOutXml(xmlDocPtr doc, const string& = "");

    /// A XML dumping subroutine common for other routines. Delete after use!
    static char *XML2StringM(xmlDocPtr doc, bool = false);

    /// A XML dumping subroutine common for other routines.
    static string XML2String(xmlDocPtr doc, bool pretty = false) {
      char *d = XML2StringM(doc, pretty);
      string ret = d ? d : "*empty XML doc*";
      xmlFree(d);
      return ret;
    }

    /// A XML dumping subroutine common for other routines.
    static void ConditionallyDumpXML(ostream& os, bool out, xmlDocPtr doc,
				     const string& txt = "",
				     const string& hdr = "",
				     bool http = false) {
      if ((out && debug_writes) || (!out && debug_reads))
	DumpXML(os, out, doc, txt, hdr, debug_writes>1, http);
    }

    /// A XML dumping subroutine common for other routines.
    static void ConditionallyDumpXML(ostream& os, bool out, const XmlDom& doc,
				     const string& txt = "",
				     const string& hdr = "",
				     bool http = false) {
      if ((out && debug_writes) || (!out && debug_reads))
	DumpXML(os, out, doc.doc, txt, hdr, debug_writes>1, http);
    }

    /// A XML dumping subroutine common for other routines.
    static void DumpXML(ostream&, bool, xmlDocPtr,
			const string&, const string&, bool, bool);

    /// A XML dumping subroutine common for other routines.
    static void DumpXML(ostream&, bool, const string&,
			const string&, const string&, bool, bool);

    /// Sends any object data.
    bool SendObject(const char*, int, const char*);

    /// Sends any object data.
    bool SendObject(const char*, const char*);

    /// Sets the processed query.
    void SetQuery(Query *q, bool own) {
      DeleteQuery();
      query = q;
      query_own = own;
    }

    /// Returns pointer to processed query.
    Query *GetQuery() { return query; }

    /// Sets the owning status of the processed query.
    void QueryOwn(bool own) { query_own = own; }

    /// Returns true if owns the processed query.
    bool QueryOwn() const { return query_own; }

    /// Deletes processed query if own.
    void DeleteQuery();

    /// This gives query's Identity().
    const string& QueryIdentity() const;

    Query *NewQuery();

    Query *ReuseQuery(Query *q) {
      DeleteQuery();
      query_own = false;
      return query = q;
    }

    /// True if has xmldoc.
    bool HasXMLdoc() const { return xmldoc__!=NULL; }

    /// Sets xmldoc after deleting its prior contents.
    void SetXMLdoc(xmlDocPtr p) {
      if (xmldoc__)
	xmlFreeDoc(xmldoc__);
      xmldoc__ = p;
    }

    /// Deletes xmldoc and sets NULL pointer.
    void DeleteXMLdoc() { SetXMLdoc(NULL); }

    /// Accesss to xmldoc.
    xmlDocPtr XMLdoc() const { return xmldoc__; }

    /// Checks if Rfd() is readable.
    bool RfdOK();

    /// Read XML from rfd and store it in xmldoc.
    bool ReadAndParseXML(bool&, bool);

    /// Read XML from mpi.
    bool ReadXMLmpi(string&);

    /// Read XML from rfd.
    bool ReadXMLsocket(string&, bool&);

    /// Read header and XML from rfd.
    bool ReadXMLsocketWithHeader(string&);

    /// Read XML from rfd and store it in xmldoc.
    bool ParseXML(const string&);

    /// This is called from MainLoop/PthreadLoop() for XML/MRML sockets.
    bool ReadAndRunXMLOrMRML();

    /// This does the job and sends error message if necessary.
    bool RunXML();

    /// This does the real job.
    bool TryToRunXML();

    /// This does the real job for common things.
    bool TryToRunXMLcommon(xmlNodePtr);

    /// This does the real job for requests.
    bool TryToRunXMLrequest(xmlNodePtr);

    /// This does the real job for responses.
    bool TryToRunXMLresponse(xmlNodePtr);

    /// This does the real job for uploads.
    bool TryToRunXMLupload(xmlNodePtr);

    /// This does the real job for uploads.
    bool TryToRunXMLslaveoffer(xmlNodePtr);

    /// This does the real job for uploads.
    bool TryToRunXMLcommand(xmlNodePtr);

    /// This does the real job for uploads.
    bool RunPartTwo();

    /// Inner loop for the TryToRunXMLupload() above.
    upload_object_data ExtractUploadObject(xmlNodePtr) const;

    /// Set to use XML.
    void XML(bool x) { protocol = x ? xml_language : unspecified_language; }

    /// True if to use XML.
    bool XML() const { return protocol==xml_language; }

    /// True if to use MRML.
    bool MRML() const { return protocol==mrml_language; }

    /// default_xsl is initialized to "picsom".
    static list<string> DefaultXSL() {
      list<string> ret;
      ret.push_back(default_xsl);
      // if (default_xsl=="picsom")
      //   ret.push_back("picsom-default");
      return ret;
    }

    /// Setting of default_xsl to something else than "picsom".
    static void DefaultXSL(const string& x) { default_xsl = x; }

    /// A dummy initializer.
    static const list<pair<string,string> > empty_list_pair_string;

    /// Creates XML stuff common to all structures.
    static XmlDom XMLstub(bool set_dtd, bool isdemo,
                          const list<string>& xsl = list<string>(),
                          const list<pair<string,string> >& param =
                          empty_list_pair_string);

    /// Creates XML stuff common to all structures and root inside it.
    static pair<XmlDom,XmlDom> 
    XMLdocroot(const string&, bool, const list<string>& = list<string>(),
               const list<pair<string,string> >& param = empty_list_pair_string);

    /// Creates XML stuff common to all structures.
    static XmlDom HTMLstub();

    /// Adds binary object to XML.
    static bool AddToXMLdata(XmlDom&, const string&, const string&,
                             const string&, bool, const string& = "",
                             const string& = "");

    /// Adds binary object to XML.
    static bool AddToXMLdata(XmlDom&, const string&, const string&,
                             stringstream&, bool, const string& = "",
                             const string& = "");

    /// Adds image data to XML by writing it first to a disk file
    static bool AddToXMLimage(XmlDom&, const string&, const imagedata&,
			      const string&, bool);

    /// Add connection status information to XML.
    bool AddToXMLconnection(XmlDom&) const;

    /// Tells if it is active.
    bool Active() const { return is_active; }

    /// Sets activation bit.
    void Active(bool a) { is_active = a; }

    /// Writes strings to log.
    void WriteLog(const string& m1, const string& m2 = "",
		  const string& m3 = "", const string& m4 = "", 
		  const string& m5 = "") const {
      pic_som->WriteLogStr(LogIdentity(), m1, m2, m3, m4, m5); }

    /// Writes contents of a stream to log.
    void WriteLog(ostringstream& os) const {
      pic_som->WriteLogStr(LogIdentity(), os); }

    /// Gives log form of Connection identity.
    string LogIdentity() const {
      stringstream s;
      s << '[' << TypeStringStr(true) << ' ' << Addresses() << ']';
      return s.str();
    }

    /// Launches a (p)thread if they are used.
    bool LaunchThreadAndOrSalute(const map<string,string>&);

    /// Cancels a (p)thread if they are used.
    bool CancelThread();

#ifdef PICSOM_USE_PTHREADS
    /// Main loop for pthreads, static interface.
    static void *PthreadLoop(void *c) {
      return ((Connection*)c)->PthreadLoop(); }

    /// Main loop for pthreads.
    void *PthreadLoop();
#endif // PICSOM_USE_PTHREADS

    /// True if opposite end accepts bouncing.
    bool BounceOK() const { return bounce; }

    ///
    bool Poll(int type=0, int timeout=PICSOM_POLL_TIMEOUT) const;

    /// Utility to convert struct timespec to format of HTTP Expires header.
    static string ExpirationString(const struct timespec&);

    /// HTTP Expires header format time given numebr of second in future.
    static string ExpirationAdd(int n) {
      struct timespec now;
      Simple::SetTimeNow(now);
      now.tv_sec += n;
      return ExpirationString(now);
    }

    /// HTTP Expires header format time one year in future.
    static string ExpirationPlusYear() { return ExpirationAdd(365*24*3600); }

    /// HTTP Expires header "0" means "already expired".
    static string ExpirationNow() { return "0"; }
  
    ///
    static bool DownloadString(PicSOM*, const string&, const string&,
			       const list<pair<string,string> >&,
			       const string&, string&, string&, 
                               int = -1, int = -1,
			       size_t = 1024L*1024*1024, float = 120);

    ///
    static string DownloadFile(PicSOM*, const string&,
			       const list<pair<string,string> >&, string&,
			       int = -1, size_t = 1024L*1024*1024, float = 120);

    ///
    static string DownloadFileWithTricks(PicSOM*, const string&,
					 const list<pair<string,string> >&,
					 string&, int = -1);

    ///
    static string DownloadFileYleDl(PicSOM*, const string&, string&, int = -1);

    ///
    const string& HostName() const { return hostname; }

    ///
    list<vector<string> > SparqlQuery(const string&, const string&,
				      bool);

    ///
    list<vector<string> > RdfQuery(const string&);

    ///
    string EscapedMPIport() const;

    ///
    static string UnEscapeMPIport(const string&);

#ifdef PICSOM_USE_MPI
    ///
    void ConvertToMPIListen();

    ///
    void ConvertToMPI(const string&, bool);

    ///
    void MPIaccept();

    ///
    bool WriteOutMPI(const string&, const string&);

    ///
    bool MPIavailable();

    ///
    pair<string,string> ReadInMPI();
#endif // PICSOM_USE_MPI

    ///
    bool Available();

  protected:
    /// Link upwards.
    PicSOM *pic_som;

    /// Type of the connection.
    connection_type type;

  protected:
    //  of the connection.
    connection_protocol protocol;

    /// Current state.
    connection_state state;

    /// Remote host's name;
    string hostname;

    /// Port number if listening.
    int port;

    /// File descriptor for writing.
    int wfd;

    /// File descriptor for reading.
    int rfd;

    /// FILE pointer for pure file reading.
    FILE *rfile;

    /// Pointer to input stream.
    istream *istr;

    ///
    __gnu_cxx::stdio_filebuf<char> *rfdibuf;
    
    ///
    __gnu_cxx::stdio_filebuf<char> *wfdobuf;
    
    ///
    istream *rfdistream;

    ///
    ostream *wfdostream;
    
    /// CHild process id.
    pid_t child;

    /// Query being created or reused.
    Query *query;

    /// Is the query freeable?
    bool query_own;

    /// Number of queries run.
    int queries;

    /// The far end of connection.
    RemoteHost client;

    /// The mod_perl running servant.
    RemoteHost servant;

    /// Servant's identifier.
    string identity_str;

    /// Client's DOCTYPE xml version in Connection.pm.
    float xml_version;

    /// Client's DOCTYPE mrml version in Connection.pm.
    float mrml_version;

    /// Client's version in Connection.pm.
    float pm_version;

    /// Is the connection human oriented or not.
    bool interactive;

    /// Creation time.
    struct timespec first;

    /// Last access time.
    struct timespec last;

    /// Is this a real query or something else.
    msg_type msgtype;

    /// TryToRunXMLcommon() uses this.
    static bool debug_objreqs;

    /// ReadLine() uses this.
    static bool debug_reads;

    /// WriteLine() uses this.
    static int debug_writes;

    ///
    static int debug_http;

    ///
    static int debug_soap;

    ///
    static int debug_proxy;

    /// SelectInput() and Close() use this.
    static bool debug_sockets;

    /// SelectInput() uses this.
    static int debug_selects;

    /// True if locks and unlocks of thread operations should show.
    static bool debug_locks;

    /// True if all HTTP files should be sent with Expires: 0
    static bool debug_expire;

    /// sec.usec for of timeout in SelectInput().
    static string timeout;

    /// default XSL to be used with XML files
    static string default_xsl;

    /// Type of requested object.
    object_type obj_type;

    /// Name of requested object.
    string obj_name;

    /// Additional specifier for requested object.
    string obj_spec;

    /// XML document last read.
    xmlDocPtr xmldoc__;

    /// Activation state.
    bool is_active;

    /// Set if ReadAndParseXML() failed.
    bool is_failed;

    /// True if blocked requests should be bounced.
    bool bounce;

    /// True if bounced requests should be traced.
    static bool debug_bounces;

    ///
    bool notify_delete;

    /// Unparsed request recognized by a HTTP compliant first line.
    string http_request;

    /// HTTP request lines rudimentally split to keyword-value pairs.
    http_headers_t http_request_lines;

    ///
    Analysis *http_analysis;

    ///
    string http_analysis_args;

    ///
    bool can_possibly_be_selected;

#ifdef USE_MRML
    // used by both Analysis and PicSOM Server MRML implementations!!
    string session_id;
#endif //USE_MRML

    // server_address structure stored here for ReOpenUplink
    sockaddr_in serv_addr;

#ifdef PICSOM_USE_PTHREADS
    /// Connection's own thread.
    pthread_t pthread;
#endif // PICSOM_USE_PTHREADS

    ///
    bool has_own_thread;

#ifdef PICSOM_USE_CSOAP
    ///
    static Connection *soap_server_instance;

    ///
    static bool soap_server_single;

    ///
    string soap_client_url;
#endif // PICSOM_USE_CSOAP

#ifdef HAVE_OPENSSL_SSL_H
    ///
    static SSL_CTX *ssl_ctx;

    ///
    SSL *ssl;
#endif // HAVE_OPENSSL_SSL_H

    ///
    static bool restca_mysql;

    ///
    static map<string,pair<string,map<string,string>>> restca_task_stat;

    ///
    static map<string,proxy_data_t> proxy_data_map;

#ifdef PICSOM_USE_MPI
  public:
    ///
    string mpi_port;

    ///
    MPI::Info mpi_info;

    ///
    MPI::Status mpi_status;

    ///
    MPI::Request mpi_request;

    ///
    MPI::Intracomm mpi_intra;

    ///
    MPI::Intercomm mpi_inter;

    ///
    int mpi_root;

    ///
    int mpi_msgtag;

    ///
    // size_t mpi_hdr[2];

    ///
    vector<pair<MPI::Request,string> > mpi_write_buf;

#endif // PICSOM_USE_MPI
    
    ///
    bool can_do_mpi;

  };  // class Connection

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

} // namespace picsom

#endif // _PICSOM_CONNECTION_H_

