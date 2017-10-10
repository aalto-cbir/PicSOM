// -*- C++ -*-  $Id: PicSOMserver.h,v 2.29 2017/04/28 07:46:07 jormal Exp $
// 
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOMSERVER_H_
#define _PICSOMSERVER_H_
#ifndef NO_PTHREADS

//#include <missing-c-utils.h>
#include <XMLutil.h>
#include <picsom-config.h>

// #undef PICSOM_USE_CSOAP

#ifdef PICSOM_USE_CSOAP
#include <nanohttp/nanohttp-logging.h>
#include <libcsoap/soap-client.h>
#endif // PICSOM_USE_CSOAP

#include <list>
#include <string>
#include <map>
using namespace std;

#include <pthread.h>

/**

*/
class PicSOMserver {
 public:
  /**

  */
  typedef list<pair<string,string> > keyvaluelist_t;
  
  /**

  */
  PicSOMserver();
  
  /**

  */
  ~PicSOMserver();
  
  /**

  */
  void Asynchronous(bool a) { async = a; }

  /**

  */
  bool Asynchronous() const { return async; }

  /**

  */
  bool InitializeSOAP(const string& = "", bool = true);

  /**

  */
  size_t SOAPtrials(size_t n) {
    size_t r = soap_trials;
    soap_trials = n;
    return r;
  }

  /**

  */
  bool DebugSOAP() const { return debug_soap; }

  /**

  */
  void DebugSOAP(bool d) { debug_soap = d; }

  /**

  */
  bool DebugSave() const { return debug_save; }

  /**

  */
  void DebugSave(bool d) { debug_save = d; }

  /**

  */
  bool DebugParse() const { return debug_parse; }

  /**

  */
  void DebugParse(bool d) { debug_parse = d; }

  /**

  */
  keyvaluelist_t DataBaseList();
  
  /**

  */
  keyvaluelist_t FeatureList(const string&);

  /**

  */
  keyvaluelist_t InsertObject(const string&, bool, const string&,
                              const list<string>&,
                              const keyvaluelist_t& = keyvaluelist_t());

  /**

  */
  keyvaluelist_t DefineClass(const string&, const string&,
			     const list<string>&,
			     const keyvaluelist_t& = keyvaluelist_t());

  /**

  */
  keyvaluelist_t Query(const string&, const string&, 
		       const keyvaluelist_t& = keyvaluelist_t());
  
  /**

  */
  keyvaluelist_t SendAjax(const string&, const string& = "", 
                          const keyvaluelist_t& = keyvaluelist_t());
  
  /**

  */
  keyvaluelist_t FindBestMatch(const string&, const string&, const string&,
                               const keyvaluelist_t& = keyvaluelist_t());
  
  /**

  */
  keyvaluelist_t Classify(const string&, const string&,
                          const string&, const string&,
                          const keyvaluelist_t& = keyvaluelist_t());

  /**

  */
  keyvaluelist_t RequestObject(const string&, const string&,
                               const string&, const string&, const string&,
                               const keyvaluelist_t& = keyvaluelist_t());

  /**

  */
  keyvaluelist_t GetObjectInfo(const string&, const string&,
			       const list<string>&,
                               const keyvaluelist_t& = keyvaluelist_t());

  /**

  */
  bool IsAsyncResult(const keyvaluelist_t&) const;

  /**
   
  */
  string AsyncResultID(const keyvaluelist_t&) const;

  /**
   
  */
  bool AsyncResultAvailable(const keyvaluelist_t&) const;

  /**
   
  */
  bool AsyncResultAvailable(const string&) const;

  /**

  */
  keyvaluelist_t AsyncResult(const keyvaluelist_t&);

  /**

  */
  keyvaluelist_t AsyncResult(const string&);

  /**

  */
  static bool AsyncResultIsError(const keyvaluelist_t&);

  /**

  */
  static string AsyncResultError(const keyvaluelist_t&);

  /**

  */
  void AsyncShowAll() const;

  /**

  */
  static bool ServerTest();

 private:
  //
  static const list<string>& el() {
    static list<string> l;
    return l;
  }

  //
  class pthread_data {
  public:
    //
    pthread_data(PicSOMserver* p, const string& x, const string& a,
                 const string& b, const string& c, const string& d,
                 const string& e, bool f, const list<string>& g,
		 const PicSOMserver::keyvaluelist_t& kv) :
      thread_set(false), ready(false), server(p), func(x),
      s1(a), s2(b), s3(c), s4(d), s5(e), b1(f), l1(g), kvlist(kv) {

      static int n = 0;
      char tmp[100];
      sprintf(tmp, "*async*%d*", n++);
      asyncid = tmp;
    }
    
    //
    static keyvaluelist_t asyncresult(const string& s, const string& k = "",
                                      const string& v = "") {
      keyvaluelist_t ret;
      ret.push_back(make_pair("asyncid", s));
      if (k!="")
        ret.push_back(make_pair(k, v));
      return ret;
    }

    //
    static keyvaluelist_t asyncerror(const string& s) {
      string a = "*async*"+s+"*";
      return asyncresult(a, "", "");
    }

    //
    keyvaluelist_t asyncresult() const { return asyncresult(asyncid); }

    //
    pthread_t thread;

    //
    bool thread_set;

    //
    bool ready;

    //
    PicSOMserver *server;

    //
    string func;

    //
    string s1, s2, s3, s4, s5;

    //
    bool b1;

    //
    list<string> l1;

    //
    keyvaluelist_t kvlist;

    //
    keyvaluelist_t result;

    //
    string asyncid;

  }; // class pthread_data

  //
  bool AsyncAdd(pthread_data*);

  //
  void AsyncErase(const string&);

  //
  typedef map<string,pthread_data*> pthread_data_map_t;

  //
  pthread_data_map_t pthread_data_map;

  //
  static void *InsertObjectPthread(void*);

  //
  keyvaluelist_t InsertObjectImpl(const string& db, bool att, const string& mt,
                                  const list<string>& fi,
                                  const PicSOMserver::keyvaluelist_t& kv);

  //
  static void *DefineClassPthread(void*);

  //
  keyvaluelist_t DefineClassImpl(const string&, const string&,
				 const list<string>&,
				 const keyvaluelist_t& = keyvaluelist_t());
  //
  static void *QueryPthread(void*);

  //
  keyvaluelist_t QueryImpl(const string&, const string&,
			       const keyvaluelist_t& = keyvaluelist_t());

  //
  static void *SendAjaxPthread(void*);

  //
  keyvaluelist_t SendAjaxImpl(const string&, const string&,
                              const keyvaluelist_t& = keyvaluelist_t());

  //
  static void *FindBestMatchPthread(void*);

  //
  keyvaluelist_t FindBestMatchImpl(const string&, const string&, const string&,
                                   const keyvaluelist_t& = keyvaluelist_t());

  //
  static void *ClassifyPthread(void*);

  //
  keyvaluelist_t ClassifyImpl(const string&, const string&,
                              const string&, const string&,
                              const keyvaluelist_t&);


  //
  static void *RequestObjectPthread(void*);

  //
  keyvaluelist_t RequestObjectImpl(const string& db, const string& q,
                                   const string& ot, const string& on,
                                   const string& os,
                                   const PicSOMserver::keyvaluelist_t& kv);
  //
  static void *GetObjectInfoPthread(void*);

  //
  keyvaluelist_t GetObjectInfoImpl(const string& db, const string& q,
				   const list<string>& l,
                                   const PicSOMserver::keyvaluelist_t& kv);

  //
  bool ProcessObjectinfos(xmlNodePtr, keyvaluelist_t&);

  //
  void CheckPicSOMerrorOK(xmlNodePtr, PicSOMserver::keyvaluelist_t& kv);

  //
  void SetErrorIfEmpty(PicSOMserver::keyvaluelist_t& kv, const string& msg);

#ifdef PICSOM_USE_CSOAP
  //
  xmlNodePtr CheckSOAPerror(SoapCtx*, PicSOMserver::keyvaluelist_t& kv);

  //
  void DumpXML(xmlDocPtr, bool);

  //
  void SoapSetKeyVal(SoapCtx *ctx, const string&, const string&);

  //
  void SoapSetKeyVal(SoapCtx *ctx, const keyvaluelist_t&);

  //
  SoapCtx *SOAPstart();

  //
  SoapCtx *SOAPend(SoapCtx*);

  //
  SoapCtx *soap_ctx;

  //
  char *soap_url;

  //
  char *soap_urn;

  //
  char *soap_method;

  //
  herror_t soap_err;
#endif // PICSOM_USE_CSOAP
  //
  size_t soap_trials;

  //
  bool async;

  //
  bool debug_soap;

  //
  bool debug_save;

  //
  bool debug_parse;

}; // class PicSOMserver

#endif // NO_PTHREADS
#endif // _PICSOMSERVER_H_

