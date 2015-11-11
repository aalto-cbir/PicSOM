// -*- C++ -*-  $Id: picsom-soap.C,v 2.31 2010/07/02 08:24:40 jorma Exp $
// 
// Copyright 1998-2010 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <PicSOMserver.h>

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

string voc2007indices = 
  "zo5:texture,colorlayout,dominantcolor,"
  "edgehistogram,scalablecolor,zo5:colm,"
  "zo5:colour,zo5:edgecoocc,zo5:edgehist";

string testdbindices = "zo5:texture,colorlayout,dominantcolor,edgehistogram";

//-----------------------------------------------------------------------------

string find(const PicSOMserver::keyvaluelist_t& kvl, const string& k) {
  for (PicSOMserver::keyvaluelist_t::const_iterator i=kvl.begin();
       i!=kvl.end(); i++)
    if (i->first==k)
      return i->second;
  return "";
}

vector<string> find_all(const PicSOMserver::keyvaluelist_t& kvl,
                        const string& k) {
  vector<string> ret;
  for (PicSOMserver::keyvaluelist_t::const_iterator i=kvl.begin();
       i!=kvl.end(); i++)
    if (i->first==k)
      ret.push_back(i->second);
  return ret;
}

//-----------------------------------------------------------------------------

string find_label(const PicSOMserver::keyvaluelist_t& kvl) {
  return find(kvl, "label");
}

vector<string> find_label_all(const PicSOMserver::keyvaluelist_t& kvl) {
  return find_all(kvl, "label");
}

//-----------------------------------------------------------------------------

string find_error(const PicSOMserver::keyvaluelist_t& kvl) {
  return find(kvl, "error");
}

bool error(const PicSOMserver::keyvaluelist_t& kvl) {
  return find_error(kvl)!="";
}

//-----------------------------------------------------------------------------

PicSOMserver::keyvaluelist_t find_fields(const
                                         PicSOMserver::keyvaluelist_t& kvl,
                                         const string& l) {
  PicSOMserver::keyvaluelist_t ret;
  for (PicSOMserver::keyvaluelist_t::const_iterator i=kvl.begin();
       i!=kvl.end(); i++)
    if (i->first.find(l+".")==0)
      ret.push_back(make_pair(i->first.substr(l.size()+1), i->second));

  return ret;
}

//-----------------------------------------------------------------------------

int databaselist(PicSOMserver& picsom, const vector<string>& args,
                 const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=0 || kvin.size()!=0) {
    cout << "--databaselist cannot have arguments nor key=value pairs" << endl;
    return 2;
  }

  cout << "PicSOMserver::DataBaseList()" << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.DataBaseList();
  vector<string> dbs = find_all(kvout, "database");
  for (size_t i=0; i<dbs.size(); i++)
    cout << "  \"" << dbs[i] << "\"" << endl;
  cout << endl;

  return 0;
}

//-----------------------------------------------------------------------------

int featurelist(PicSOMserver& picsom, const vector<string>& args,
                const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=1) {
    cout << "--featurelist has exactly one argument: <database>" << endl;
    return 2;
  }

  if (kvin.size()!=0) {
    cout << "--featurelist has no key=value pairs" << endl;
    return 2;
  }

  string db = args[0];

  cout << "PicSOMserver::FeatureList() of \"" << db << "\""
       << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.FeatureList(db);
    vector<string> feats = find_all(kvout, "feature");
    for (size_t i=0; i<feats.size(); i++)
      cout << "  \"" << feats[i] << "\"" << endl;
    cout << endl;

  return 0;
}

//-----------------------------------------------------------------------------

int insert(PicSOMserver& picsom, const vector<string>& args,
           const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=1 && (args.size()<4 || (args[1]!="0" && args[1]!="1"))) {
    cout << "--insert can have one argument:"
      " <database>" << endl;
    cout << "    possibly with descriptions:"
      " <database>(longname=longer databasename,shorttext=some words)" << endl;
    cout << "    or three or more arguments:"
      " <database> 0/1 <mime-type> <file> ..." << endl;
    return 2;
  }

  string db = args[0], mime;
  bool att = false;
  list<string> flist;

  if (args.size()>=4) {
    mime = args[2];
    att = args[1]=="1";
    flist.insert(flist.begin(), args.begin()+3, args.end());
  }

  cout << "PicSOMserver::InsertObject() in \"" << db << "\"";
  if (flist.size())
    cout << " of type [" << mime << "]"
         << (att?" as attachments":"");
  for (list<string>::const_iterator i=flist.begin(); i!=flist.end(); i++)
    cout << " \"" << *i << "\"";
  cout << endl << endl;

  if (args.size()==1) {
    flist.push_back("");
  }

  PicSOMserver::keyvaluelist_t kvout
    = picsom.InsertObject(db, att, mime, flist, kvin);

  if (error(kvout)) {
    cout << "  ERROR=\"" << find_error(kvout) << "\"" << endl << endl;
    return 3;
  }

  vector<string> l = find_label_all(kvout);
  for (size_t i=0; i<l.size(); i++)
    cout << "  label=\"" << l[i] << "\"" << endl;

  if (l.size()==0)
    cout << "  status=\"" << find(kvout, "status") << "\""  << endl;

  cout << endl;

  return 0;
}

//-----------------------------------------------------------------------------

int defineclass(PicSOMserver& picsom, const vector<string>& args,
		const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()<3) {
    cout << "--defineclass needs at least three arguments: "
      "<database> <class> <label>" << endl;
    return 2;
  }

  const string db    = args[0];
  const string cls   = args[1];
  list<string> clist(args.begin()+2, args.end());

  cout << "PicSOMserver::DefineClass() in \"" << db << "\",\"" << cls << "\"";
  for (list<string>::const_iterator i=clist.begin(); i!=clist.end(); i++)
    cout << ", \"" << *i << "\"";
  cout << endl << endl;

  PicSOMserver::keyvaluelist_t kvout
    = picsom.DefineClass(db, cls, clist, kvin);
  
  if (error(kvout)) {
    cout << "  ERROR=\"" << find_error(kvout) << "\"" << endl << endl;
    return 3;
  }

  return 0;
}

//-----------------------------------------------------------------------------

int request(PicSOMserver& picsom, const vector<string>& args,
              const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=2) {
    cout << "--request has exactly two arguments: <database> <label>" << endl;
    return 2;
  }

  string db = args[0], label = args[1];

  cout << "PicSOMserver::RequestObject() for \"" << label << "\" in \""
       << db << "\"" << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.RequestObject(db, "", "object",
                                                            label, "", kvin);

  string ll = find_label(kvout), fname = find(kvout, "filename");
  string d  = find(kvout, "<DATA>");

  cout << "  \"" << ll << "\" -> \"" << fname << "\" "
       << d.size() << " bytes" << endl << endl;

  if (fname!="") {
    ofstream of(fname.c_str());
    of << d;
  }

  return 0;
}

//-----------------------------------------------------------------------------

int objectinfo(PicSOMserver& picsom, const vector<string>& args,
               const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=3) {
    cout << "--objectinfo has exactly three arguments: <database> <query>"
         << " <label or class>" << endl;
    return 2;
  }

  string db = args[0], q = args[1];
  list<string> clist(args.begin()+2, args.end());

  cout << "PicSOMserver::GetObjectInfo() in \"" << db << "\" with <"
       << q << "> for";
  for (list<string>::const_iterator i=clist.begin(); i!=clist.end(); i++)
    cout << " \"" << *i << "\"";
  cout << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.GetObjectInfo(db,q, clist, kvin);
  vector<string> labs = find_label_all(kvout);
  for (size_t i=0; i<labs.size(); i++) {
    cout << "  \"" << labs[i] << "\"";
    PicSOMserver::keyvaluelist_t kvsub = find_fields(kvout, labs[i]);
    for (PicSOMserver::keyvaluelist_t::const_iterator j=kvsub.begin();
         j!=kvsub.end(); j++)
      cout << " " << j->first << "=" << j->second;
    cout << endl;
  }
  cout << endl;

  return 0;
}

//-----------------------------------------------------------------------------

int bestmatch(PicSOMserver& picsom, const vector<string>& args,
              const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=3) {
    cout << "--bestmatch has exactly three arguments: <database> <indices>"
         << " <label>" << endl;
    return 2;
  }

  string db = args[0], f = args[1], label = args[2];

  cout << "PicSOMserver::FindBestMatch() for \"" << label << "\" in \""
       << db << "\" with \"" << f << "\"" << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.FindBestMatch(db, f, label,kvin);
  vector<string> labs = find_label_all(kvout);
  for (size_t i=0; i<labs.size(); i++)
    cout << "  \"" << labs[i] << "\" score=\""
         << find(kvout, labs[i]+".score") << "\"" << endl;
  cout << endl;

  return 0;
}

//-----------------------------------------------------------------------------

int classify(PicSOMserver& picsom, const vector<string>& args,
             const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=4) {
    cout << "--classify has exactly four arguments: <database> <label>"
         << " <indices> <classset>" << endl;
    return 2;
  }

  string db = args[0], label = args[1], feat = args[2], set = args[3];

  cout << "PicSOMserver::Classify() \"" << label << "\" in \""
       << db << "\" with \"" << feat << "\" from \"" << set << "\""
       << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.Classify(db, label, feat,
                                                       set, kvin);
  vector<string> classes = find_all(kvout, "class");
  for (size_t i=0; i<classes.size(); i++) {
    string cls = classes[i];
    cout << "  class=\"" << cls << "\"";
    string value = find(kvout, cls+".value");
    if (value!="")
      cout << " value=\"" << value << "\"";
    cout << endl;
  }
  cout << endl;

  return 0;
}

//-----------------------------------------------------------------------------

int query(PicSOMserver& picsom, const vector<string>& args,
         const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=2) {
    cout << "--query has exactly two arguments: <query> <relevance feedback>"
	 << endl;
    return 2;
  }

  string q = args[0], rf = args[1];

  cout << "PicSOMserver::Query() query \"" << q << "\" relevance feedback \""
       << rf << "\"" << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.Query(q, rf, kvin);
  
  if (error(kvout)) {
    cout << "  ERROR=\"" << find_error(kvout) << "\"" << endl << endl;
    return 3;
  }

  string qid = find(kvout, "identity");
  cout << "  identity=\"" << qid << "\"" << endl;

  vector<string> labs = find_label_all(kvout);
  for (size_t i=0; i<labs.size(); i++) {
    cout << "    \"" << labs[i] << "\"";
    PicSOMserver::keyvaluelist_t kvsub = find_fields(kvout, labs[i]);
    for (PicSOMserver::keyvaluelist_t::const_iterator j=kvsub.begin();
         j!=kvsub.end(); j++) {
      cout << " " << j->first << "=" << j->second;
      if (j->first=="score")
	break;
    }
    cout << endl;
  }
  cout << endl;

  return 0;
}

//-----------------------------------------------------------------------------

int ajax(PicSOMserver& picsom, const vector<string>& args,
         const PicSOMserver::keyvaluelist_t& kvin) {
  if (args.size()!=2) {
    cout << "--ajax has exactly two arguments: <file> <query>" << endl;
    return 2;
  }

  string file = args[0], q = args[1];

  cout << "PicSOMserver::SendAjax() \"" << file << "\" query \"" << q << "\""
       << endl << endl;

  PicSOMserver::keyvaluelist_t kvout = picsom.SendAjax(file, q, kvin);

  for (PicSOMserver::keyvaluelist_t::const_iterator i=kvout.begin();
       i!=kvout.end(); i++)
    cout << " " << i->first << "=" << i->second << endl;    
  
  return 0;
}

//-----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  bool use_testdb  = true;
  bool use_voc2007 = false;

  string voc2007name = "voc2007-test";

  string pattern = " [-switches] [--options] --<command> [arg] ..."
    " [= [key=value] ...]";

  if (argc<2) {
    cout << "USAGE: " << argv[0] << pattern << endl;
    cout << "  try: " << argv[0] << " --help" << endl;
    return 1;
  }

  bool debug_soap = false, debug_save = false, debug_parse = false;

  string cmd, soapurl;
  vector<string> args;
  PicSOMserver::keyvaluelist_t kvin;

  bool eqfound = false;
  for (int i=1; i<argc; i++) {
    string a = argv[i];
    if (cmd=="" && a.find("--url=")==0) {
      soapurl = a.substr(6);
      continue;
    }
    if (cmd=="" && a=="-Dsoap") {
      debug_soap = true;
      continue;
    }
    if (cmd=="" && a=="-Dsave") {
      debug_save = true;
      continue;
    }
    if (cmd=="" && a=="-Dparse") {
      debug_parse = true;
      continue;
    }
    if (cmd=="") {
      cmd = a;
      continue;
    }
    if (a=="=") {
      eqfound = true;
      continue;
    }

    if (!eqfound)
      args.push_back(a);

    else {
      string k = a, v;
      size_t p = k.find('=');
      if (p!=string::npos) {
        v = k.substr(p+1);
        k.erase(p);
      }
      kvin.push_back(make_pair(k, v));
    }
  }

  PicSOMserver picsom;
  picsom.DebugSOAP(debug_soap);
  picsom.DebugSave(debug_save);
  picsom.DebugParse(debug_parse);
#ifdef PICSOM_USE_CSOAP
  picsom.InitializeSOAP(soapurl);
#endif // PICSOM_USE_CSOAP

  if (cmd=="--databaselist" || cmd=="-l")
    return databaselist(picsom, args, kvin);

  if (cmd=="--featurelist" || cmd=="-f")
    return featurelist(picsom, args, kvin);

  if (cmd=="--insert" || cmd=="-i")
    return insert(picsom, args, kvin);

  if (cmd=="--defineclass" || cmd=="-d")
    return defineclass(picsom, args, kvin);

  if (cmd=="--request" || cmd=="-r")
    return request(picsom, args, kvin);

  if (cmd=="--objectinfo" || cmd=="-o")
    return objectinfo(picsom, args, kvin);

  if (cmd=="--bestmatch" || cmd=="-b")
    return bestmatch(picsom, args, kvin);

  if (cmd=="--classify" || cmd=="-c")
    return classify(picsom, args, kvin);

  if (cmd=="--query" || cmd=="-q")
    return query(picsom, args, kvin);

  if (cmd=="--ajax" || cmd=="-a")
    return ajax(picsom, args, kvin);

  if (cmd=="--help" || cmd=="-h") {
    string h = string("    ")+string(argv[0])+" ";
    string e(h.size(), ' ');
    cout << argv[0] << pattern << endl;
    cout << "  COMMANDS:" << endl;
    cout << h+"--help" << endl;
    cout << h+"--databaselist" << endl;
    cout << h+"--featurelist <database>" << endl;
    cout << h+"--insert <database>" << endl;
    cout << h+"--insert <database>(longname=longer databasename,"
      "shorttext=some words)" << endl;
    cout << h+"--insert <database> 0/1 <mime-type> <file> ... "
      "[= [updatediv=yes] [indices=...]]" << endl;
    cout << e+"  0=insertion by filenames  1=insertion as attacments" << endl;
    cout << h+"--defineclass <database> <target class> <label or class 1> "
      "[<label or class 2> ...]" << endl;
    cout << h+"--request <database> <label> ..." << endl;
    cout << h+"--objectinfo <database> <query> <label or class>" << endl;
    cout << h+"--bestmatch <database> <indices> <label>" << endl;
    cout << h+"--classify <database> <label> <indices> <classset>" << endl;
    cout << h+"--query <query> <relevance feedback> [= [database=...]" 
      " [indices=...] [maxquestions=...] [target=...] ...]" << endl;
    cout << h+"--ajax <file> [<query>]" << endl;
    cout << "  SWITCHES:" << endl; 
    cout << "    -Dsoap"  << endl; 
    cout << "    -Dsave"  << endl; 
    cout << "    -Dparse" << endl; 
    cout << "  OPTIONS:" << endl;
    cout << "    --url=http://host:port/path" << endl;  
    return 0;
  }

  cout << argv[0] << " unknown command: " << cmd << endl;
  cout << "try: " << argv[0] << " --help" << endl;

  return 1;
}

