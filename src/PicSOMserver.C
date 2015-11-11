// -*- C++ -*-  $Id: PicSOMserver.C,v 2.47 2012/08/20 09:18:38 jorma Exp $
// 
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef NO_PTHREADS

#include <PicSOMserver.h>
#include <base64.h>

#include <string.h>

#include <iostream>
#include <fstream>

#ifdef PICSOM_USE_CSOAP
#include <libcsoap/soap-xml.h>
#include <nanohttp/nanohttp-server.h>
#endif // PICSOM_USE_CSOAP

//=============================================================================

PicSOMserver::PicSOMserver() {
#ifdef PICSOM_USE_CSOAP
  soap_ctx    = NULL;
  soap_url    = strdup("http://localhost:10000/picsom-soap");
  soap_urn    = strdup("urn:picsom");
  soap_method = strdup("execute");
  soap_err    = H_OK;
#endif // PICSOM_USE_CSOAP
  soap_trials = 1;

  async = false;
  debug_soap = debug_save = debug_parse = false;
}

//=============================================================================

PicSOMserver::~PicSOMserver() {
#ifdef PICSOM_USE_CSOAP
  free(soap_url);
  free(soap_urn);
  free(soap_method);

  if (true)
    soap_client_destroy();
#endif // PICSOM_USE_CSOAP
}

//=============================================================================

#ifdef PICSOM_USE_CSOAP
bool PicSOMserver::InitializeSOAP(const string& url, bool do_init) {
  if (url!="") {
    free(soap_url);
    soap_url = strdup(url.c_str());
  }

  httpd_set_timeout(300);

  // HLOG_VERBOSE>HLOG_DEBUG>HLOG_INFO>HLOG_WARN>HLOG_ERROR>HLOG_FATAL
  // hlog_set_level(HLOG_VERBOSE);

  if (do_init) {
    int argc = 1;
    char *argv[] = { NULL, NULL };
    herror_t err = soap_client_init_args(argc, argv);
    if (err != H_OK) {
      log_error4("%s() : %s [%d]", herror_func(err), herror_message(err),
               herror_code(err));
      herror_release(err);
      return false;
    }
  }

  return true;
}

//=============================================================================

void PicSOMserver::SoapSetKeyVal(SoapCtx *ctx,
                                 const string& k, const string& v) {
  soap_env_add_item(ctx->env, "xsd:string", k.c_str(),
		    v!="" ? v.c_str() : NULL);
}

//=============================================================================

void PicSOMserver::SoapSetKeyVal(SoapCtx *ctx,
                                 const PicSOMserver::keyvaluelist_t& kv) {
  for (keyvaluelist_t::const_iterator i=kv.begin(); i!=kv.end(); i++)
    SoapSetKeyVal(ctx, i->first, i->second);
}

//=============================================================================

SoapCtx *PicSOMserver::SOAPstart() {
  SoapCtx *ctx = NULL;
  herror_t err = soap_ctx_new_with_method(soap_urn, soap_method, &ctx);
  if (err != H_OK) {
    log_error4("%s() : %s [%d]", herror_func(err), herror_message(err),
               herror_code(err));
    herror_release(err);
    exit(1);
  }

  return ctx;
}

//=============================================================================

SoapCtx *PicSOMserver::SOAPend(SoapCtx *ctx) {
  SoapCtx *ctx2 = NULL;

  if (debug_soap) {
    DumpXML(ctx->env->root->doc, false);
    cout << endl;
  }

  if (debug_save)
    DumpXML(ctx->env->root->doc, true);

  size_t i = 0;
  do {
    i++;

    herror_t err = soap_client_invoke(ctx, &ctx2, soap_url, "");

    if (err != H_OK) {
      log_error4("%s() : %s [%d]", herror_func(err),
                 herror_message(err), herror_code(err));
      herror_release(err);

      if (soap_trials && i>=soap_trials) {
        // soap_ctx_free(ctx);

        cout << endl << endl << "PicSOMserver::SOAPend() failing "
             << soap_url << endl << endl;

        return NULL;
      }
    }

    if (ctx2 && debug_soap) {
      DumpXML(ctx2->env->root->doc, false);
      cout << endl;
    }

    if (ctx2 && debug_save)
      DumpXML(ctx2->env->root->doc, true);

    if (!ctx2)
      cout << endl << endl << "PicSOMserver::SOAPend() retrying "
           << soap_url << endl << endl;

  } while (!ctx2);

  return ctx2;
}

//=============================================================================

void PicSOMserver::DumpXML(xmlDocPtr doc, bool file) {
  string msg = "PicSOMserver::DumpXML() : ";

  if (!doc)  {
    cerr << msg+"xmlDocPtr is NULL!" << endl;
    return;
  }

  xmlNodePtr root = xmlDocGetRootElement(doc);
  if (!root)  {
    cerr << msg+"Empty document!" << endl;
    return;
  }

  xmlBufferPtr buffer = xmlBufferCreate();
  xmlNodeDump(buffer, doc, root, 1, 0);

  if (!file)
    cout << (const char *)xmlBufferContent(buffer) << endl;
  else {
    static int n = 1;
    stringstream ss;
    ss << "picsomserver-" << n++ << ".xml";
    string str = ss.str();
    ofstream os(str.c_str());
    os << (const char *)xmlBufferContent(buffer) << endl;
    cout << "Wrote SOAP in <" << str << ">" << endl;
  }

  xmlBufferFree(buffer);
}
#endif // PICSOM_USE_CSOAP

//=============================================================================

PicSOMserver::keyvaluelist_t PicSOMserver::DataBaseList() {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  SoapSetKeyVal(ctx1, "databaselist", "");
  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:databaselist>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:database>
  while (node) {
    if (DebugParse())
      cout << NodeName(node) << " : ";
    xmlNodePtr p = FindXMLchildElem(node, "name");
    if (p) {
      string name = NodeChildContent(p);
      if (DebugParse())
        cout << NodeName(p) << "=" << name << " ";
      ret.push_back(make_pair("database", name));
    }
    p = FindXMLchildElem(node, "size");
    if (DebugParse()) {
      if (p)
        cout << NodeName(p) << "=" << NodeChildContent(p) << " ";
      cout << endl;
    }
    node = soap_xml_get_next(node);                   // <picsom:database>
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#endif // PICSOM_USE_CSOAP

  SetErrorIfEmpty(ret, "PicSOMserver::DataBaseList()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t PicSOMserver::FeatureList(const string& db) {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  SoapSetKeyVal(ctx1, "database",    db);
  SoapSetKeyVal(ctx1, "featurelist", "");
  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:featurelist>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:feature>
  while (node) {
    if (DebugParse())
      cout << NodeName(node) << " : ";
    if (NodeName(node)=="feature") {
      xmlNodePtr p = FindXMLchildElem(node, "name");
      if (p) {
        string name = NodeChildContent(p);
        if (DebugParse())
          cout << NodeName(p) << "=" << name << " ";
        ret.push_back(make_pair("feature", name));
      }
      p = FindXMLchildElem(node, "size");
      if (DebugParse()) {
        if (p)
          cout << NodeName(p) << "=" << NodeChildContent(p) << " ";
        cout << endl;
      }

    } else if (NodeName(node)=="featurealias") {
      for (xmlAttrPtr p = node->properties; p; p=p->next)
	if (AttrName(p)=="name" || AttrName(p)=="value")
          if (DebugParse())
            cout << AttrName(p) << "=" << AttrContent(p) << " ";
      if (DebugParse())
        cout << endl;      
    }
    node = soap_xml_get_next(node);                   // <picsom:feature>
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  string x = db;
  x = x;
#endif // PICSOM_USE_CSOAP

  SetErrorIfEmpty(ret, "PicSOMserver::FeatureList()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::InsertObject(const string& db, bool att, const string& mt,
                           const list<string>& fi,                           
                           const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return InsertObjectImpl(db, att, mt, fi, kv);

  pthread_data *d = new pthread_data(this, "InsertObject",
                                     db, mt, "", "", "", att, fi, kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, InsertObjectPthread, d);
  d->thread_set = true;

  if (!r)
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::InsertObjectPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->InsertObjectImpl(d.s1, d.b1, d.s2, d.l1, d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::InsertObjectImpl(const string& db, bool att, const string& mt,
                               const list<string>& fi,
                               const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart(), *ctx2 = NULL;
  SoapSetKeyVal(ctx1, "analyse",   "insert");
  SoapSetKeyVal(ctx1, "database",  db);
  SoapSetKeyVal(ctx1, kv);

  if (!att && fi.size()) {
    string fl;
    for (list<string>::const_iterator i=fi.begin(); i!=fi.end(); i++)
      fl += string(fl!=""?" ":"")+*i;
    SoapSetKeyVal(ctx1, "args", fl);
  }

  bool ok = true;
  if (att)
    for (list<string>::const_iterator i=fi.begin(); i!=fi.end(); i++) {
      char href[MAX_HREF_SIZE];
      herror_t err = soap_ctx_add_file(ctx1, i->c_str(), mt.c_str(), href);
      if (err != H_OK) {
        log_error4("%s() : %s [%d]", herror_func(err), herror_message(err),
                   herror_code(err));
        herror_release(err);
        ok = false;
        break;
      }
      xmlNodePtr u = soap_env_add_attachment(ctx1->env, "upload", href);
      xmlNodeAddContent(u, (const xmlChar*)i->c_str());
    }

  if (ok) {
    ctx2   = SOAPend(ctx1);
    xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
    if (node)
      node = soap_xml_get_children(node);               // <picsom:result>
    if (node)
      node = soap_xml_get_children(node);    // <error> or <picsom:objectlist>

    CheckPicSOMerrorOK(node, ret);

    if (node)
      node = soap_xml_get_children(node);               // <picsom:objectinfo>
    while (node) {
      if (DebugParse())
        cout << NodeName(node) << " : ";
      xmlNodePtr p = FindXMLchildElem(node, "label");
      if (p) {
        string label = NodeChildContent(p);
        if (DebugParse())
          cout << NodeName(p) << "=" << label << " ";
        ret.push_back(make_pair("label", label));
      }

      xmlNodePtr fi = FindXMLchildElem(node, "objectfileinfo"); 
      if (fi) {                                     // <picsom:objectfileinfo>
        p = FindXMLchildElem(fi, "filename");
        if (p && DebugParse())
          cout << NodeName(p) << "=" << NodeChildContent(p) << " ";

        p = FindXMLchildElem(fi, "original-name");
        if (p && DebugParse())
          cout << NodeName(p) << "=" << NodeChildContent(p) << " ";

        p = FindXMLchildElem(fi, "format");
        if (p && DebugParse())
          cout << NodeName(p) << "=" << NodeChildContent(p) << " ";
      }
      if (DebugParse())
        cout << endl;
      node = soap_xml_get_next(node);               // <picsom:objectinfo>
    }
    if (DebugParse())
      cout << endl;

    soap_ctx_free(ctx2);
  }

  soap_ctx_free(ctx1);
#else
  size_t x = db.size()+att+mt.size()+fi.size()+kv.size();
  x += x;
#endif // PICSOM_USE_CSOAP

  SetErrorIfEmpty(ret, "PicSOMserver::InsertObjectImpl()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::DefineClass(const string& db, const string& cls, 
			  const list<string>& cl, 
			  const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return DefineClassImpl(db, cls, cl, kv);

  pthread_data *d = new pthread_data(this, "DefineClass",
                                     db, cls, "", "", "", false, cl, kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, DefineClassPthread, d);
  d->thread_set = true;

  if (!r)
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::DefineClassPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->DefineClassImpl(d.s1, d.s2, d.l1, d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::DefineClassImpl(const string& db, const string& cls, 
			      const list<string>& cl, 
			      const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  SoapSetKeyVal(ctx1, "analyse", "defineclass");
  SoapSetKeyVal(ctx1, "database", db);
  SoapSetKeyVal(ctx1, "class",    cls);
  SoapSetKeyVal(ctx1, kv);

  if (cl.size()) {
    string clstr;
    for (list<string>::const_iterator i=cl.begin(); i!=cl.end(); i++)
      clstr += string(clstr!=""?" ":"")+*i;
    SoapSetKeyVal(ctx1, "args", clstr);
  }

  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:featurelist>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:feature>
  while (node) {
    if (DebugParse())
      cout << NodeName(node) << " : ";
    node = soap_xml_get_next(node);                   // <picsom:feature>
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  size_t x = db.size()+cls.size()+cl.size()+kv.size();
  x += x;
#endif // PICSOM_USE_CSOAP

  //SetErrorIfEmpty(ret, "PicSOMserver::DefineClass()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::Query(const string& q, const string& rf,
		    const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return QueryImpl(q, rf, kv);

  pthread_data *d = new pthread_data(this, "Query",
                                     q, rf, "", "", "", false, el(), kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, QueryPthread, d);
  d->thread_set = true;

  if (!r)
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::QueryPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->QueryImpl(d.s1, d.s2, d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::QueryImpl(const string& q, const string& rf,
			const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  SoapSetKeyVal(ctx1, "analyse",   "query");
  if (q!="")
    SoapSetKeyVal(ctx1, "query",  q);
  if (rf!="")
    SoapSetKeyVal(ctx1, "args",  rf);
  SoapSetKeyVal(ctx1, kv);

  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);    // <error> or <picsom:queryinfo>

  CheckPicSOMerrorOK(node, ret);

  if (node)
    node = soap_xml_get_children(node);               // <picsom:identity>
  while (node) {
    string key = NodeName(node);
    if (key=="identity") {
      string val = NodeChildContent(node);
      if (DebugParse())
        cout << key << "=" << val << endl;
      ret.push_back(make_pair(key, val));

    } else if (key=="objects") {
      xmlNodePtr p = FindXMLchildElem(node, "questionobjectlist");
      xmlNodePtr o = soap_xml_get_children(p);        // <picsom:objectinfo>

      ProcessObjectinfos(o, ret);
    }
    node = soap_xml_get_next(node);                   //
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  size_t x = q.size()+rf.size()+kv.size();
  x += x;
#endif // PICSOM_USE_CSOAP

  // SetErrorIfEmpty(ret, "QueryImpl()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::SendAjax(const string& fi, const string& q,
                       const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return SendAjaxImpl(fi, q, kv);

  pthread_data *d = new pthread_data(this, "SendAjax",
                                     fi, q, "", "", "", false, el(), kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, SendAjaxPthread, d);
  d->thread_set = true;

  if (!r)
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::SendAjaxPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->SendAjaxImpl(d.s1, d.s2, d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::SendAjaxImpl(const string& fi, const string& q,
                           const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  SoapSetKeyVal(ctx1, "analyse",   "ajax");
  if (q!="")
    SoapSetKeyVal(ctx1, "query",  q);
  SoapSetKeyVal(ctx1, kv);

  if (fi!="") {
    char href[MAX_HREF_SIZE];
    herror_t err = soap_ctx_add_file(ctx1, fi.c_str(), "text/xml", href);
    if (err != H_OK) {
      log_error4("%s() : %s [%d]", herror_func(err), herror_message(err),
               herror_code(err));
      herror_release(err);
    }

    xmlNodePtr u = soap_env_add_attachment(ctx1->env, "upload", href);
    xmlNodeAddContent(u, (const xmlChar*)fi.c_str());
  }

  string nname;

  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);               // <pinview:ajaxresponse>
  if (node) {
    nname = NodeName(node);
    if (DebugParse())
      cout << nname << endl;
    node = soap_xml_get_children(node);               // <gotopage> etc
  }
  while (node) {
    string nn = NodeName(node);
    if (DebugParse())
      cout << nname << " " << nn << " : ";
    xmlAttrPtr p = node->properties;
    while (p) {
      string aname = AttrName(p), acont = AttrContent(p);
      if (DebugParse())
        cout << " " << aname << "=" << acont;
      ret.push_back(make_pair(nname+"."+nn+"."+aname, acont));
      p = p->next;
    }
    node = soap_xml_get_next(node);               // <gotopage> etc
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  size_t x = fi.size()+q.size()+kv.size();
  x += x;
#endif // PICSOM_USE_CSOAP

  // SetErrorIfEmpty(ret, "SendAjaxImpl()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::FindBestMatch(const string& db, const string& fi,
                            const string& p,
                            const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return FindBestMatchImpl(db, fi, p, kv);

  pthread_data *d = new pthread_data(this, "FindBestMatch",
                                     db, fi, p, "", "", false, el(), kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, FindBestMatchPthread, d);
  d->thread_set = true;

  if (!r)
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::FindBestMatchPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->FindBestMatchImpl(d.s1, d.s2, d.s3, d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::FindBestMatchImpl(const string& db, const string& f,
                                const string& p,
                                const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  SoapSetKeyVal(ctx1, "analyse",         "best");
  SoapSetKeyVal(ctx1, "database",        db);
  SoapSetKeyVal(ctx1, "target",          "image");
  SoapSetKeyVal(ctx1, "permapobjects",   "500");
  SoapSetKeyVal(ctx1, "maxquestions",    "10");
  SoapSetKeyVal(ctx1, "indices",         f);
  SoapSetKeyVal(ctx1, "positive",        p);
  SoapSetKeyVal(ctx1, "relativescores",  "yes");
  SoapSetKeyVal(ctx1, kv);
  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:objectlist>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:objectinfo>
  while (node) {
    string label;
    if (DebugParse())
      cout << NodeName(node) << " : ";
    xmlNodePtr p = FindXMLchildElem(node, "label");
    if (p) {
      label = NodeChildContent(p);
      if (DebugParse())
        cout << NodeName(p) << "=" << label << " ";
      ret.push_back(make_pair("label", label));
    }
    p = FindXMLchildElem(node, "score");
    if (p) {
      string score = NodeChildContent(p);
      if (DebugParse())
        cout << NodeName(p) << "=" << score << " ";
      if (label!="")
        ret.push_back(make_pair(label+".score", score));
    }
    p = FindXMLchildElem(node, "stageinfolist");
    if (p) {
      if (DebugParse())
        cout << "( ";
      for (xmlNodePtr q = FindXMLchildElem(p, "stageinfo"); q ; q=q->next) {
	xmlAttrPtr a = q->properties;
	if (AttrName(a)=="stage" && AttrContent(a)=="2") {
	  a = a->next;
	  if (AttrName(a)=="feature" && DebugParse())
	    cout << AttrContent(a) << "=";
	  a = a->next;
	  if (AttrName(a)=="score" && DebugParse())
	    cout << AttrContent(a) << " ";
	}
      }
      if (DebugParse())
        cout << ")";
    }
    if (DebugParse())
      cout << endl;
    node = soap_xml_get_next(node);                   // <picsom:objectinfo>
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  size_t x = db.size()+f.size()+p.size()+kv.size();
  x = x;
#endif // PICSOM_USE_CSOAP

  SetErrorIfEmpty(ret, "PicSOMserver::FindBestMatchImpl()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::Classify(const string& db, const string& l,
                       const string& f, const string& cset,
                       const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return ClassifyImpl(db, l, f, cset, kv);

  pthread_data *d = new pthread_data(this, "Classify",
                                     db, l, f, cset, "", false, el(), kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, ClassifyPthread, d);
  d->thread_set = true;

  if (!r) 
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::ClassifyPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->ClassifyImpl(d.s1, d.s2, d.s3, d.s4, d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::ClassifyImpl(const string& db, const string& l,
                           const string& f, const string& cset,
                           const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;
  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  SoapSetKeyVal(ctx1, "analyse",           "class");
  SoapSetKeyVal(ctx1, "database",          db);
  SoapSetKeyVal(ctx1, "target",            "image");
  SoapSetKeyVal(ctx1, "permapobjects",     "500");
  SoapSetKeyVal(ctx1, "matrixcount",       "3");
  SoapSetKeyVal(ctx1, "indices",           f);
  SoapSetKeyVal(ctx1, "positive",          l);
  SoapSetKeyVal(ctx1, "classifymethod",    "euclidean");
  SoapSetKeyVal(ctx1, "classifynclasses",  "3");
  SoapSetKeyVal(ctx1, "classificationset", cset);
  //SoapSetKeyVal(ctx1, "classparams",     "euclidean,3,"+cset);
  SoapSetKeyVal(ctx1, kv);

  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:guessedkeywords>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:keyword>
  while (node) {
    if (DebugParse())
      cout << NodeName(node) << " : ";

    string name = GetProperty(node, "name");
    if (DebugParse())
      cout << "name=" << name << " ";
    if (name!="")
      ret.push_back(make_pair("class", name));

    string value = GetProperty(node, "value");
    if (DebugParse())
      cout << "value=" << value << endl;

    if (name!="" && value!="")
      ret.push_back(make_pair(name+".value", value));

    node = soap_xml_get_next(node);                   // <picsom:keyword>
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  size_t x = db.size()+l.size()+f.size()+cset.size()+kv.size();
  x = x;
#endif // PICSOM_USE_CSOAP

  SetErrorIfEmpty(ret, "PicSOMserver::ClassifyImpl()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::RequestObject(const string& db, const string& q,
                            const string& ot, const string& on,
                            const string& os,
                            const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return RequestObjectImpl(db, q, ot, on, os, kv);

  string f = "RequestObject";
  if (db!="") f += " database="+db;
  if (q!="")  f += " query="+q;
  if (ot!="") f += " type="+ot;
  if (on!="") f += " name="+on;
  if (os!="") f += " spec="+os;

  pthread_data *d = new pthread_data(this, f,
                                     db, q, ot, on, os, false, el(), kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, RequestObjectPthread, d);
  d->thread_set = true;

  if (!r) 
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::RequestObjectPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->RequestObjectImpl(d.s1, d.s2, d.s3, d.s4, d.s5,
                                         d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::RequestObjectImpl(const string& db, const string& q,
                                const string& ot, const string& on,
                                const string& os,
                                const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;

  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  if (q!="")
    SoapSetKeyVal(ctx1, "query",  q);

  if (db!="")
    SoapSetKeyVal(ctx1, "database",  db);

  SoapSetKeyVal(ctx1, kv);

  string onos = on;
  if (os!="")
    onos += " "+os;

  SoapSetKeyVal(ctx1, ot, onos);

  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:*objecttype*>
  if (node) {
    string enc;
    if (DebugParse())
      cout << NodeName(node) << " :";
    xmlAttrPtr p = node->properties;
    while (p) {
      string aname = AttrName(p), acont = AttrContent(p);
      if (DebugParse())
        cout << " " << aname << "=" << acont;
      if (aname=="content-encoding")
        enc = acont;
      else
        ret.push_back(make_pair(aname, acont));
      p = p->next;
    }
    string data = GetXMLchildContent(node);
    if (data!="") {
      if (DebugParse())
        cout << " <DATA> " << data.length(); 
      if (enc=="base64") {
        istringstream mmstr(data);
        ostringstream rawstr;
        b64::decode(mmstr, rawstr);
        data = rawstr.str();
        if (DebugParse())
          cout << " -> " << data.length();
      }
      if (DebugParse())
        cout << " bytes";
      ret.push_back(make_pair("<DATA>", data));
    }
    if (DebugParse())
      cout << endl;
    node = soap_xml_get_children(node);               // anything...
  }

  map<string,int> count;

  while (node) {
    string nn = NodeName(node);
    ostringstream oss;
    oss << nn << "[" << count[nn]++ << "]";
    string nncount = oss.str();
    if (DebugParse())
      cout << "  " << nn << " (" << nncount << ") : ";
    xmlAttrPtr p = node->properties;
    while (p) {
      string aname = AttrName(p), acont = AttrContent(p);
      if (DebugParse())
        cout << " " << aname << "=" << acont;
      ret.push_back(make_pair(nncount+"."+aname, acont));
      p = p->next;
    }
    if (DebugParse())
      cout << endl;
    node = soap_xml_get_next(node);                   // anything...
  }
  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  size_t x = db.size()+q.size()+ot.size()+on.size()+os.size()+kv.size();
  x += x;
#endif // PICSOM_USE_CSOAP

  SetErrorIfEmpty(ret, "PicSOMserver::RequestObjectImpl()");

  return ret;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::GetObjectInfo(const string& db, const string& q,
			    const list<string>& cl,
                            const PicSOMserver::keyvaluelist_t& kv) {
  if (!Asynchronous())
    return GetObjectInfoImpl(db, q, cl, kv);

  string f = "GetObjectInfo";
  if (db!="") f += " database="+db;
  if (q!="")  f += " query="+q;

  pthread_data *d = new pthread_data(this, f,
                                     db, q, "", "", "", false, cl, kv);
  AsyncAdd(d);

  int r = pthread_create(&d->thread, NULL, GetObjectInfoPthread, d);
  d->thread_set = true;

  if (!r) 
    return d->asyncresult();

  AsyncErase(d->asyncid);

  return pthread_data::asyncerror("CREATE FAILED");
}

//=============================================================================

void *PicSOMserver::GetObjectInfoPthread(void *p) {
  pthread_data& d = *(pthread_data*)p;
  d.thread_set = true;
  d.result = d.server->GetObjectInfoImpl(d.s1, d.s2, d.l1, d.kvlist);
  d.ready  = true;
  return p;
}

//=============================================================================

PicSOMserver::keyvaluelist_t
PicSOMserver::GetObjectInfoImpl(const string& db, const string& q,
				const list<string>& cl,
                                const PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;

  keyvaluelist_t ret;

#ifdef PICSOM_USE_CSOAP
  SoapCtx *ctx1 = SOAPstart();
  if (db!="")
    SoapSetKeyVal(ctx1, "database",  db);
  if (q!="")
    SoapSetKeyVal(ctx1, "query",  q);
  SoapSetKeyVal(ctx1, "objectinfo", *cl.begin());
  SoapSetKeyVal(ctx1, kv);

  SoapCtx *ctx2   = SOAPend(ctx1);
  xmlNodePtr node = CheckSOAPerror(ctx2, ret);        // <m:result>
  if (node)
    node = soap_xml_get_children(node);               // <picsom:result>
  if (node)
    node = soap_xml_get_children(node);      // <picsom:objectinfo / objectlist>
  if (node && NodeName(node)=="objectlist")
    node = soap_xml_get_children(node);               // <picsom:objectinfo>

  ProcessObjectinfos(node, ret);

  if (DebugParse())
    cout << endl;

  soap_ctx_free(ctx2);
  soap_ctx_free(ctx1);
#else
  size_t x = db.size()+q.size()+cl.size()+kv.size();
  x += x;
#endif // PICSOM_USE_CSOAP

  SetErrorIfEmpty(ret, "PicSOMserver::GetObjectInfoImpl()");

  return ret;
}

//=============================================================================

#ifdef PICSOM_USE_CSOAP
bool PicSOMserver::ProcessObjectinfos(xmlNodePtr node, keyvaluelist_t& ret) {
  using namespace picsom;

  map<string,int> count;

  while (node) {
    if (NodeName(node)!="objectinfo") {
      node = soap_xml_get_next(node);
      continue;
    }

    string label;
    xmlNodePtr p = soap_xml_get_children(node);       // <picsom::label>...
    while (p) {
      string name = NodeName(p), cont = NodeChildContent(p);
      if (name=="label") {
        label = cont;
        ret.push_back(make_pair("label", label));
        if (DebugParse())
          cout << "  label=" << label << endl;

      } else if (label!="" && cont!="") {
        string k = label+"."+name;
        ret.push_back(make_pair(k, cont));
        if (DebugParse())
          cout << "  " << k << "=" << cont << endl;

      } else if ((name=="objectfileinfo" || name=="frameinfo" ||
                  name=="keywordlist") && label!="") {
        xmlNodePtr q = soap_xml_get_children(p);      // <picsom::checksum>...
        while (q) {
          string qname = NodeName(q), qcont = NodeChildContent(q);
          string k = label+"."+qname;
          ret.push_back(make_pair(k, qcont));
          if (DebugParse())
            cout << "  " << k << "=" << qcont << endl;
          
          q = q->next;
        }

      } else if (name=="erfdata" && label!="") {
        xmlNodePtr q = soap_xml_get_children(p);
        while (q) {
          string qname = NodeName(q), qcont = NodeChildContent(q);
          string k = label+"."+qname;
          ret.push_back(make_pair(k, qcont));
          if (DebugParse())
            cout << "  " << k << "=" << qcont << endl;
          
          q = q->next;
        }

      } else if (DebugParse())
        cout << "  " << name << "=" << cont << " UNPROCESSED" << endl;

      p = p->next;
    }
    if (DebugParse())
      cout << endl;
    node = soap_xml_get_next(node);
  }

  return true;
}
#endif // PICSOM_USE_CSOAP

//=============================================================================

#ifdef PICSOM_USE_CSOAP
xmlNodePtr PicSOMserver::CheckSOAPerror(SoapCtx *ctx,
                                        PicSOMserver::keyvaluelist_t& kv) {
  // method == <m:executeResponse>
  xmlNodePtr method = ctx && ctx->env ? soap_env_get_method(ctx->env) : NULL;

  if (!method) {
    kv.push_back(make_pair("error", "SOAP error"));
    return NULL;
  }

  return soap_xml_get_children(method);  // <m:result>
}
#endif // PICSOM_USE_CSOAP

//=============================================================================

bool PicSOMserver::AsyncAdd(pthread_data *d) {
  bool debug = false;

  pthread_data_map[d->asyncid] = d;

  if (debug)
    cout << "PicSOMserver::AsyncAdd() [" << d->asyncid << "]" << endl;

  return true;
}

//=============================================================================

bool PicSOMserver::IsAsyncResult(const PicSOMserver::keyvaluelist_t& s) const {
  if (s.empty() || s.begin()->first!="asyncid" ||
      s.begin()->second.find("*async*")!=0)
    return false;

  return pthread_data_map.find(s.begin()->second)!=pthread_data_map.end();
}

//=============================================================================

string PicSOMserver::AsyncResultID(const PicSOMserver::keyvaluelist_t& s)
  const {
  if (!IsAsyncResult(s))
    return "";

  return s.begin()->second;
}

//=============================================================================

bool PicSOMserver::AsyncResultAvailable(const PicSOMserver::keyvaluelist_t& s) 
  const {
  return IsAsyncResult(s) && AsyncResultAvailable(AsyncResultID(s));
}

//=============================================================================

bool PicSOMserver::AsyncResultAvailable(const string& id) const {
  pthread_data_map_t::const_iterator i = pthread_data_map.find(id);
  if (i==pthread_data_map.end()) {
    cerr << "*async*ID INEXISTENT* <" << id << ">" << endl;
    return false;
  }

  return i->second->ready;
}

//=============================================================================

PicSOMserver::keyvaluelist_t 
PicSOMserver::AsyncResult(const PicSOMserver::keyvaluelist_t& s) {
  if (!IsAsyncResult(s))
    return pthread_data::asyncerror("ID INEXISTENT");

  return AsyncResult(AsyncResultID(s));
}

//=============================================================================

PicSOMserver::keyvaluelist_t PicSOMserver::AsyncResult(const string& id) {
  pthread_data_map_t::iterator i = pthread_data_map.find(id);
  if (i==pthread_data_map.end())
    return pthread_data::asyncerror("ID INEXISTENT");

  if (!i->second->ready)
    return pthread_data::asyncerror("RESULT UNAVAILABLE");

  keyvaluelist_t ret = i->second->result;
  AsyncErase(id);

  return ret;
}

//=============================================================================

void PicSOMserver::AsyncErase(const string& id) {
  pthread_data_map_t::iterator i = pthread_data_map.find(id);
  if (i==pthread_data_map.end())
    return;

  pthread_data *d = i->second;

  if (d->thread_set) {
    void *p = NULL;
    int r = pthread_join(d->thread, &p);
    if (r)
      cerr << "pthread_join() failed" << endl;
  }

  delete d;

  pthread_data_map.erase(i);
}

//=============================================================================

void PicSOMserver::AsyncShowAll() const {
  for (pthread_data_map_t::const_iterator i = pthread_data_map.begin();
       i!=pthread_data_map.end(); i++) {
    cout << i->first << " func=" << i->second->func;
    /*
    for (keyvaluelist_t::const_iterator j=i->second->result.begin();
         j!=i->second->result.end(); j++)
      cout << " " << j->first << "=" << j->second;
    */
    cout << endl;
  }
}

//=============================================================================

void PicSOMserver::CheckPicSOMerrorOK(xmlNodePtr node,
                                      PicSOMserver::keyvaluelist_t& kv) {
  using namespace picsom;

  if (!node) {
    // most likely this is an error situation too...
    // kv.push_back(make_pair("error", "NULL pointer");
    // if (DebugParse())
    //   cout << "error=NULL pointer (CheckPicSOMerrorOK)" << endl;
    return;
  }

  string name = NodeName(node), r = NodeChildContent(node);

  if (name=="error" || name=="status") {
    kv.push_back(make_pair(name, r));
    if (DebugParse())
      cout << name << "=" << r << " (CheckPicSOMerrorOK)" << endl;
  }
}
//=============================================================================

void PicSOMserver::SetErrorIfEmpty(PicSOMserver::keyvaluelist_t& kv,
                                   const string& msg) {
  if (kv.empty()) {
    kv.push_back(make_pair("error", msg));
    if (DebugParse())
      cout << "error=" << msg << " (SetErrorIfEmpty)" << endl;
  }
}

//=============================================================================

bool PicSOMserver::AsyncResultIsError(const PicSOMserver::keyvaluelist_t& kv) {
  for (keyvaluelist_t::const_iterator i=kv.begin(); i!=kv.end(); i++)
    if (i->first=="error")
      return true;
  return false;
}

//=============================================================================

string PicSOMserver::AsyncResultError(const PicSOMserver::keyvaluelist_t& kv) {
  for (keyvaluelist_t::const_iterator i=kv.begin(); i!=kv.end(); i++)
    if (i->first=="error")
      return i->second;
  return "";
}

//=============================================================================

bool PicSOMserver::ServerTest() {
  PicSOMserver server;
  server.Asynchronous(true);
#ifdef PICSOM_USE_CSOAP
  server.InitializeSOAP();
#endif // PICSOM_USE_CSOAP
  server.DataBaseList();
  server.FeatureList("hello");
  server.InsertObject("", true, "", list<string>());
  server.Query("", "");
  server.SendAjax("", "");
  server.FindBestMatch("", "", "");
  server.Classify("", "", "", "");
  server.RequestObject("", "", "", "", "");
  server.GetObjectInfo("", "", list<string>());
  server.IsAsyncResult(PicSOMserver::keyvaluelist_t());
  server.AsyncResultAvailable("");
  server.AsyncResult("");
  server.AsyncShowAll();
  return true;
}

//=============================================================================

#endif // NO_PTHREADS
