// -*- C++ -*-  $Id: Query.C,v 2.630 2015/04/20 14:52:49 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <Query.h>
#include <Analysis.h>
#include <WordHist.h>

#include <aspects.h>
#include <textdata.h>
#include <base64.h>

#include <numeric>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  using namespace std;

  static const string Query_C_vcid =
    "@(#)$Id: Query.C,v 2.630 2015/04/20 14:52:49 jorma Exp $";

  static string splitchars = " \t\r\n/";

  /////////////////////////////////////////////////////////////////////////////

  bool Query::debug_all_keys       = false;
  bool Query::debug_restriction    = false;
  bool Query::debug_featsel        = false;
  bool Query::debug_aspects        = false;
  bool Query::debug_phases         = false;
  bool Query::debug_placeseen      = false;
  bool Query::debug_algorithms     = false;
  bool Query::debug_stages         = false;
  int  Query::debug_lists          = 0;
  bool Query::debug_check_lists    = false;
  bool Query::debug_weights        = false;
  bool Query::debug_parameters     = false;
  bool Query::debug_subobjects     = false;
  bool Query::debug_can_be_shown   = false;
  bool Query::debug_selective      = false;
  bool Query::debug_classify       = false;
  bool Query::debug_prf            = false;
  int  Query::debug_ajax           = 0;
  int  Query::debug_erf_features   = 0;
  int  Query::debug_erf_relevance  = 0;

  bool Query::pre2122bug = false;

  bool Query::relevance_to_siblings = true;
  bool Query::zero_matrices_allowed = true;

  int  Query::ajax_save         = 2;
  bool Query::ajax_save_audio   = false;
  bool Query::ajax_play_audio   = false;
  bool Query::ajax_mplayer      = false;
  bool Query::ajax_mplayer_file = false;

  ofstream Query::erfdatafile;

  struct relop_stage_type Query::expand_operation = 
    { relop_none, relop_copy };

  struct relop_stage_type Query::exchange_operation = 
    { relop_none, relop_none };

  struct relop_stage_type Query::converge_operation = 
    { relop_sum, relop_none };

  bool Query::store_selfinfluence = false;
  hitstype_t Query::hitstype = trad;

  float Query::comb_factor = 1.0;
  float Query::hit_comb_factor=5.0;

  string Query::matlabdump;
  string Query::datdump;
  IntVector Query::watchset;

  /////////////////////////////////////////////////////////////////////////////

  inline float do_nothing(float f) {
    return f;
  }

  // Copied from /home/vvi/picsom_kokeet/sfbs/sfbs.C
  float guardedlog(float f){
    float m = 0.00000001;
    float r = (f<m)? log(m): log(f);
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query::Query(PicSOM *p, const string& fname, const Connection *conn,
               Analysis *a) : first_client(NULL) {
    Initialize(p);
    SetAnalysis(a);
    if (fname!="" && Retrieve(fname, true, conn))
      from_disk = true;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query::~Query() {
    // WriteLog("Destructing query");

    ajax_data_all.DeleteDoc();

    UnselectIndices(NULL, true);

    // WriteLog("Running algorithms[]->Cleanup()");
    for (size_t i=0; i<algorithms.size(); i++)
      delete algorithms[i].data;

    if (IsRoot() && querytreeformerselections != NULL)
      delete querytreeformerselections;

    for (size_t i=0; i<child_new.size(); i++)
      delete child_new[i];
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::Initialize(PicSOM *p) {
    picsom     = p;
    parent     = NULL;
    the_db     = NULL;

    _sa_limit_per_map_units  = 0;
    _sa_limit_total_units    = 0;
    _sa_limit_per_map_images = 0;
    _sa_permapobjects        = 0;

    maxquestions = 10; 
    maxrounds    = 0;
    tncolumns    = 5;

    random_image_count = 0;

    imgcols        = 5;
    showstats      = 0;
    pointed_map    = -1;
    pointed_level  = -1;
    rndseed_value  = 0; 
    rndseed_random = false;
    rndseed_random_each_round = false;

    classify = 0;
    classifynclasses = 5;

    okstatus = true;
    inrun = from_disk = can_show_seen = false;
    usezero = true;
    can_be_shown_and = false;
    limitrelevance = false;
    bordercompensation = true;
    relativescores = false;
    readnbr = false;
    ajax_data_piece_count = 0;
    ajax_gotopage_sent = false;
    mapnormalize = true;
    use_google_image_search = false;

    restricted_objects_seen = 0;

    analysis = NULL;

    query_target_ = target_no_target;
    algorithm    = cbir_no_algorithm;
    round        = 0;

    include_minimal = false;
    include_snippet = include_origins_info = true;

    SetTimeNow(start);
    SetTimeNow(last_modification);
    SetTimeZero(last_access);

    _sa_vqlevel = 0;

    calculate_entropy_force = false;

    matrixcount_xx   = 1;
    entropymatrix = -1;

    negative_weight = 1.0;

    write_to_log = false;

    for (int f=stage_first; f<=stage_last; f++) {
      StageFunc((cbir_stage)f, func_default);
      StageArgs((cbir_stage)f, "");
    }

    SetTimeZero(saved_time);

    backfromsettings = skipoptionpage = false;
    greymaps = 0;

    maxposvalue = maxnegvalue = MAXFLOAT;

    textquery_correct_person = textquery_correct_location = 
    check_commons_classes = commons_classes_useaspects = 
    add_classfeatures = false;

    ajax_image_x = ajax_image_y = -1;
    ajax_image_p = NULL;
    erf_features_when = 2;
    erfsplitdata = erfptrstogaze = false;
    forcecbirstages = false;
    default_gaze_relevance = 0.15;
    speechsync = "pointer";
    keysync    = "pointer";

#ifdef QUERY_USE_PTHREADS
    main_thread_set = false;
    pthread_rwlock_init(&rwlock, NULL);
#endif // QUERY_USE_PTHREADS

    querytreeformerselections = NULL;

    initialset_pre = "";
    prescore_function = &do_nothing;

    //   cout << "Created query " << (void*)this << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::IdentityServerPart() {
    pair<int,int> ports = Picsom()->GetListenPortRange();
    char tmp[20];
    sprintf(tmp, "%d:%d", ports.first, ports.second);
    return "!S=" + Picsom()->HostName() + ":" + tmp + "!";
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::SetIdentity(const string& txt) {
    if (identity!="" && txt=="" &&
        (!parent || Identity()!=parent->Identity()))
      ShowError("Query::SetIdentity(): identity already set: ",
                Identity());

    if (txt!="")
      identity = txt;

    else {
      static int cnt;
      char tmp[200];
      if (!parent) {
        char timestr[100];
        tm *time = localtime(&start.tv_sec);
        strftime(timestr, sizeof timestr, "%Y%m%d:%H%M%S", time);  // was %y
        sprintf(tmp, "Q:%s:%d:%d", timestr+2, getpid(), cnt++);
      } else {
        string pident = parent->Identity();
        // remove the server part:
        size_t n = pident.find_first_of("!");
        if(n != string::npos)
          pident.erase(n);
        if (pident=="")
          pident = "EMPTY-PARENT-IDENTITY-";
        sprintf(tmp, "%s%c", pident.c_str(), int(96+parent->Nchildren()));
      }
      identity = tmp;
      // identity_str = string(tmp) + IdentityServerPart();
    }

    //cout << "Set query identity:[" << identity << "] " << (void*)this <<endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::RootIdentity(const string& ident) {
    if (!SaneIdentity(ident))
      return "";

    string ret = ident;
    // the starting point of the server part:
    size_t nserv = ret.find_first_of("!");
 
    size_t n = ret.find_first_not_of("0123456789:", 2);
    if(n < nserv)
      ret.erase(n, nserv-n);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SaneIdentity(const string& identstr) {
    const char *ident = identstr.c_str();

    // ident =~ Q:yymmdd:HHMMSS:pid:1aaa
    //          012345678901234567890123

    if (!ident || strlen(ident)<19 || strncmp(ident, "Q:", 2) ||
        ident[8]!=':' || ident[15]!=':')
      return false;

    const char *ptr = strchr(ident+16, ':');
    if (!ptr)
      return false;

    const char *exptr = strchr(ptr+1, '!');

    // there shouldn't be a ':' between ptr and the next '!' (or between ptr 
    // and the end of the string if a '!' doesn't exist)
    ptr = strchr(ptr+1, ':');
    if (ptr && ( !exptr || (exptr && ptr < exptr) ) )
      return false;

    // there should be "S=servername" after the first '!', and another '!' 
    // should also be found if already one was found
    if (exptr) {
      // string host = "S=" + PicSOM::HostName();
      // size_t l = host.length();
      // if( strncmp(exptr+1, host.c_str(), l) )
      //   return false;

      exptr = strchr(exptr+1, '!');
      if (!exptr)
        return false;
    }

    size_t n = strspn(ident+2, "0123456789:");
    if (n<17)
      return false;
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::UnEscapeIdentity(const string& srcstr) {
    const char *src = srcstr.c_str();
    const size_t l = 1000;
    char dst[l];

    strncpy(dst, src, l);
    dst[l-1] = 0;
    for (char *p = dst; *p; p++)
      if (*p=='%' && strlen(p)>2) {
        char tmp[] = "XX";
        strncpy(tmp, p+1, 2);
        *p = (char)strtol(tmp, NULL, 16);
        memmove(p+1, p+3, strlen(p+3)+1);
      }

    return dst;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query *Query::CreateChild(Connection *c) {
    Query *q = new Query(Picsom());
	
    q->Parent(this);
    q->SetIdentity();
    q->CopySome(*this, true, false, false);
    if (c)
      q->FirstClientAddress(c->ClientAddress());

    return q;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::CopySome(const Query& source, bool b, bool c, bool a) {
    if (b) {
      the_db             = source.the_db;
      seenobject         = source.seenobject;
      newobject          = source.newobject;
      algorithm          = source.algorithm;
      external_algorithm = source.external_algorithm;
      algorithms         = source.algorithms;
      for (size_t i=0; i<algorithms.size(); i++)
        algorithms[i].data = algorithms[i].data->Copy(this);
      _sa_permapobjects  = source._sa_permapobjects;
      _sa_vqlevel        = source._sa_vqlevel;
      maxquestions       = source.maxquestions;
      maxrounds          = source.maxrounds;
      endpage            = source.endpage;
      matrixcount_xx     = source.matrixcount_xx;
      entropymatrix      = source.entropymatrix;
      round              = source.round+1;
      imgcols            = source.imgcols;
      showstats          = source.showstats;
      usezero            = source.usezero;
      can_show_seen      = source.can_show_seen;
      can_be_shown_and   = source.can_be_shown_and;
      limitrelevance     = source.limitrelevance;
      bordercompensation = source.bordercompensation;
      relativescores     = source.relativescores;
      readnbr            = source.readnbr;
      maxposvalue        = source.maxposvalue;
      maxnegvalue        = source.maxnegvalue;
      setupname          = source.setupname;
      speechrecognizer   = source.speechrecognizer;
      speechsync         = source.speechsync;
      keysync            = source.keysync;

      use_google_image_search = source.use_google_image_search;
      default_gaze_relevance = source.default_gaze_relevance;

      if (!FirstClientSet())
        FirstClientAddress(source.FirstClientAddress()); // added 2009-10-07

      CopyBasicIndexData(source.index_data);

#ifdef USE_MRML
      listoffeatures     = source.listoffeatures;
#endif // USE_MRML

      stage_map        = source.stage_map;
      stage_arg_map    = source.stage_arg_map;
      keyvaluelist     = source.keyvaluelist;
      rndseed_random   = source.rndseed_random;
      rndseed_value    = source.rndseed_value;
      // or RndSeedRandomize() or RndSeedStep() ???
      rndseed_random_each_round   = source.rndseed_random_each_round;
      if (rndseed_random_each_round)
	rndseed_value = RndSeedRandomize();

      _sa_default_convtype = source._sa_default_convtype;
      _sa_tssom_convtype = source._sa_tssom_convtype;

      _sa_default_bmuconvtype = source._sa_default_bmuconvtype;
      _sa_tssom_bmuconvtype = source._sa_tssom_bmuconvtype;

      greymaps         = source.greymaps;
      query_target_     = source.query_target_;

      calculate_entropy_force = source.calculate_entropy_force;

      restriction = source.restriction;

      textsearch = source.textsearch;

      textquery = source.textquery;
      textquery_correct_person = source.textquery_correct_person;
      textquery_correct_location = source.textquery_correct_location;
      check_commons_classes = source.check_commons_classes;
      commons_classes_useaspects = source.commons_classes_useaspects;
      add_classfeatures = source.add_classfeatures;
      commonsfile = source.commonsfile;
      classfeaturesfile = source.classfeaturesfile;
      
      classaugment = source.classaugment;

      classify          = source.classify;
      classifymethod    = source.classifymethod;
      classifynclasses  = source.classifynclasses;
      classificationset = source.classificationset;
      classmodel        = source.classmodel;

      phase_script  = source.phase_script;
      phase_rule    = source.phase_rule;
      phase_current = source.phase_current;

      negative_weight = source.negative_weight;

      erfpolicy            = source.erfpolicy;
      erfkeymap            = source.erfkeymap;
      erfsplitdata         = source.erfsplitdata;
      erfptrstogaze        = source.erfptrstogaze;
      erf_features_when    = source.erf_features_when;
      erf_image_data_cache = source.erf_image_data_cache;

      forcecbirstages      = source.forcecbirstages;

      xsl       = source.xsl;
      tnsize    = source.tnsize;
      tncolumns = source.tncolumns;

      aspect_weight = source.aspect_weight;

      if (debug_featsel)
        cout << Identity() << " : CopySome() : copied ["
             << BriefIndexList() << "] from " << source.Identity() << endl;
    }
  
    if (c) {
      viewclass = source.viewclass;
      positive  = source.positive;
      negative  = source.negative;
    }

    scorefile = source.scorefile;
    hitstype  = source.hitstype;

    initialset_pre = source.initialset_pre;

    if (a) {
      // this doesn't do anything with these yet:
      // query_operation_mode = source.query_operation_mode;
      // analysis = analysis ? new Analysis(*source.analysis) : NULL;
    }
  
    // query_main_thread ???
  
    //   cout << "Query::CopySome() called : " << source.Identity() << " -> "
    //        << Identity() << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::Dump(Simple::DumpMode, ostream& os) const {
    os << Bold("Query ")           << (void*)this
       << " picsom="               << (void*)picsom
       << " identity="             << Simple::ShowString(identity)
       << " saved_time="           << TimeString(saved_time)
       << " from_disk="            << from_disk
       << " parent="               << (void*)parent
       << " children="             << child_new.size()
       << " inrun="                << inrun
       << " usezero="              << usezero
       << " can_show_seen="        << can_show_seen
       << " can_be_shown_and="     << can_be_shown_and
       << " write_to_log="         << write_to_log
       << " the_db="               << (void*)the_db
       << " restriction.Label()="  << restriction.Label()
       << " restriction.Length()=" << restriction.Length()
       << " Nindices()="           << Nindices()
       << " NseenObjects()="       << NseenObjects()
       << " NnewObjects()="        << NnewObjects()
       << " FirstClientAddress()=" << FirstClientAddress()
       << " pointed_map="          << pointed_map
       << " pointed_level="        << pointed_level
       << " mappoint="             << mappoint
       << " maparea_ul="           << maparea_ul
       << " maparea_lr="           << maparea_lr
       << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DeleteChild(Query *c) {
    for (size_t i=0; i<child_new.size(); i++)
      if (child_new[i]==c) {
        delete c;
        child_new.erase(child_new.begin()+i, child_new.begin()+i+1);
        return true;
      }
    return ShowError("Query::DeleteChild() no match");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ReadFiles(bool labonly, bool do_create) {
    if (!the_db)
      return ShowError("Query::ReadFiles() database not set");

    bool force = false;
    bool cod = true, div = true, cnv = true, ord = false, dat = false;
    bool nbr = readnbr;

    // obs! HAS_CBIRALGORITHM_NEW problem?:
    if (!algorithms.empty())
      return true;
  
    if (!labonly)
      dat = true;

    if (algorithm==cbir_sq) {
      div = cnv = false;
      cod = StageFunction(stage_process_images)==func_process_images_vqw;
      ord = true;
    }

    if (algorithm==cbir_vq || algorithm==cbir_vqw) {
      cnv = false;
      dat = true;
      labonly = false;
    }

    if (algorithm==cbir_external)
      cod = div = cnv = false;

    if (do_create) {
      cod = div = cnv = ord = false;
      dat = true;
      labonly = false;
    }

    if (ConvType(-1)!="")
      cnv = false;

    //    cout << "Reading files, nbr="<<nbr<<endl; 

    bool ok = true;
    for (size_t i=0; ok && i<Nindices(); i++) 
      if (IsTsSom(i)) 
        ok = TsSom(i).ReadFiles(force, labonly, dat, cod, div, cnv, ord, nbr);
      else if (false && IsVectorIndex(i) && dat)
	ok = vectorIndex(i).ReadDataFile(force, labonly);

    return ok; 
  }

  /////////////////////////////////////////////////////////////////////////////

  Query *Query::Find(const string& ident, bool debug) {
    if (debug)
      cout << "Query::Find(" << ident << ") match with <"
	   << identity << "> " << (void*)this << endl;

    if (Identity()==ident)
      return this;

    // this is an obvious place for optimization

    for (size_t i=0; i<Nchildren(); i++) {
      Query *q = Child(i)->Find(ident, debug);
      if (q)
        return q;
    }

    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *Query::DisplayStringM(bool intera) const {
    if (!intera) {
      bool notsaved = !parent&&!MoreRecent(saved_time,
                                           LastModificationTime(true));

      const char *ts = TimeString(start).c_str();
      const char *tm = TimeString(last_modification).c_str();
      const char *ta = TimeString(last_access).c_str();
      const char *tM = TimeString(LastModificationTime(true)).c_str();
      const char *tA = TimeString(LastAccessTime(true)).c_str();
      const char *td = TimeString(saved_time).c_str();

      char tmp[1024];
      sprintf(tmp, "%s %s %15s %d first=%s mod=%s access=%s",
              notsaved?"**":"  ",
              Identity().c_str(),
              FirstClientAddress().c_str(),
              (int)Nchildren(),
              ts, tm, ta);

      if (!parent)
        sprintf(tmp+strlen(tmp), " rec.mod=%s rec.acsess=%s saved=%s",
                tM, tA, td);

      return CopyString(tmp);
    }

    const char *ts = TimeString(start).c_str();
    char *ret = NULL, tmp[1024];
    sprintf(tmp, "identity = %s\n", Identity().c_str()); 
    Simple::AppendString(ret, tmp);
    sprintf(tmp, "database = %s\n",DataBaseName().c_str());
    Simple::AppendString(ret,tmp);
    sprintf(tmp, "client = %s\n", FirstClientAddress().c_str());
    Simple::AppendString(ret, tmp);
    sprintf(tmp, "starttime = %s\n", ts);
    Simple::AppendString(ret, tmp);

    return CopyString(ret);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CheckPermissions(const Query& rq,
                               const Connection& conn) const {
    if (Identity()=="")
      return false;

    if (this==&rq || (rq.Identity()!="" && Identity()!=rq.Identity())) {
      cout << (void*)this << " " << (void*)&rq << " [" << Identity() << "] ["
           << rq.Identity() << "]" << endl;
      SimpleAbort();
    }
    //   cout << "++++++ Query::CheckPermissions " 
    //        << rq.GetConnection()->Addresses() << " wants to access "
    //        << Identity() << " ["
    //        << DataBaseName() << ":";
    //   for (int i=0; i<Nindices(); i++)
    //     cout << tssom[i].Name() << (i<Nindices()-1?",":"");
    //   cout << "] of " << FirstClientAddress() << endl;
  
    //  return FirstClient().Match(rq.GetConnection()->Client());

    return the_db ? the_db->CheckPermissions(conn) : false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CheckPermissions(const Connection& conn) const {
    if (Identity()=="")
      return false;

    return the_db ? the_db->CheckPermissions(conn) : false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Interpret(const string& keyin, const string& valstr,
                        int& res, const Connection *conn) {
    const string h = "Query::Interpret() : ";
    res = 1;

    int   intval   = atoi(valstr.c_str());
    float floatval = atof(valstr.c_str());

    string key = keyin, aspect = "";
    size_t dot = key.find('.');
    if (dot!=string::npos) {
      const string a = key.substr(0, dot), b = key.substr(dot+1);
      bool is_posneg = a=="positive" || a=="negative" || a=="posneg";

      if (a=="algorithm" && algorithms.empty())
        return ShowError(h+"Setting algorithm variables before setting "+
                         "algorithm is not allowed.");
      if (a.find("algorithm")==0 || (!is_posneg && algorithms.size())) {
	size_t ai = numeric_limits<size_t>::max();
	if (a.find("algorithm")==0 && a[9]>='0' && a[9]<='9') {
	  ai = atoi(a.substr(9).c_str());
	}
        bool any_ready = false;
        for (size_t i=0; i<algorithms.size(); i++) 
          if (i==ai || a=="algorithm" ||
	      a==algorithms[i].algorithm->FullName()) {
	    bool ready = false;

	    if (b=="indices" || b=="features") {
	      SelectIndices(&algorithms[i], valstr, false);
	      ready = true;

	    } else
	      ready = algorithms[i].algorithm->Interpret(algorithms[i].data,
							 b, valstr, res);

            PicSOM::TraceInterpret(ready, "    CbirAlgorithm", b, valstr, res);
            any_ready |= ready;
          }

        return any_ready;
      }

      if (!is_posneg || b=="")
        return false;

      key = a;
      aspect = b;
    }

    /*>> query_interpret
      identity
      *DOCUMENTATION MISSING* */
    
    if (key=="identity") {
      identity = valstr;
      return true;
    }

    if (key=="analyse") {
      // QueryModeXXX(operation_analyse);
      vector<string> a;
      a.push_back(string("analyse=")+valstr);
      analysis = new Analysis(Picsom(), NULL, this, a);
      Picsom()->AppendAnalysis(analysis);
      return true;
    }

    if (key=="starttime") {
      start = PicSOM::InverseTimeString(valstr);
      return true;
    }

    if (key=="modtime") {
      last_modification = PicSOM::InverseTimeString(valstr);
      return true;
    }

    if (key=="accesstime") {
      last_access = PicSOM::InverseTimeString(valstr);
      return true;
    }

    if (key=="savetime") {  // this wasn't here before trecvid2005 deadline!
      saved_time = PicSOM::InverseTimeString(valstr);
      return true;
    }

    if (key=="client") {
      FirstClientAddress(valstr);
      return true;
    }

    if (key=="database") {
      SetDataBase(picsom->FindDataBaseEvenNew(valstr, IsInsert()));

      target_type tt = Target();
      if (the_db && !Parent()) {
        CopySome(the_db->DefaultQuery(), true, false, false);
        // InterpretDefaults();
        if (IsCreate()||IsAnalyse()||tt!=target_no_target)
          Target(tt);  // override <defaulttarget> from settings2
        if (!IsAnalyse())
          UnselectIndices(NULL);
      }

      res = the_db && (!conn || the_db->CheckPermissions(*conn));

      return true;
    }

    if (key=="features" || key=="indices") {
      if (!the_db) {
        ShowError(h+"database not selected");
        res = 0;
        return true;
      }

#ifdef USE_MRML
      listoffeatures = valstr;
#endif // USE_MRML

      if (algorithm != cbir_external && algorithms.empty())
        res = SelectIndices(NULL, valstr, IsCreate()||IsInsert());

      if (!algorithms.empty())
        for (size_t i=0; i<algorithms.size(); i++)
          SelectIndices(&algorithms[i], valstr, false);

      return true;
    }

    if (key=="textsearch") {
      textsearch = valstr;
      return true;
    }

    if (key=="textindex") {
      textindex = valstr;
      return true;
    }

    if (key=="textfield") {
      textfield = valstr;
      return true;
    }

    if (key=="queryfield") {
      queryfield = valstr;
      return true;
    }

    if (key=="mappoint") {
      if (valstr.length()==1 && parent && parent->IsMapPointSet())
        RelativeMapPointMovement(valstr[0]);
      else
        pointed_map = FindTsSom(valstr, &pointed_level, NULL, &mappoint);

      if (!IsMapPointSet()) {
        res = 0;
        ShowError(h+"Failed to set mappoint: [", valstr, "]");

      } else {
        TsSom(pointed_map).ReadMapFile(false, true);
        // this was moved here from SelectAroundMapPoint() beware!
        TsSom(pointed_map).ZoomArea(pointed_level, mappoint, imgcols,
                                    maparea_ul, maparea_lr);
      }

      return true;
    }

    if (key=="mapnormalize") {
      mapnormalize = IsAffirmative(valstr);
      return true;
    }

    if (key=="viewclass") {
      viewclass = valstr;
      return true;
    }

    if (key=="positive")
      return SetAspectInfo(positive, aspect, valstr);

    if (key=="negative")
      return SetAspectInfo(negative, aspect, valstr);

    if (key=="posneg") {
      positive[aspect] = make_pair(valstr,          1.0);
      negative[aspect] = make_pair("~("+valstr+")", 1.0);
      return true;
    }

    if (key=="comment") {
      comment = valstr;
      return true;
    }

    if (key=="setname") {
      setname = valstr;
      return true;
    }

    if (key=="permapobjects" || key=="permapnewimages") {
      PerMapObjects(intval);
      return true;
    }

    if (key=="canshowseen") {
      can_show_seen = IsAffirmative(valstr);
      return true;
    }

    if (key=="zero_matrices_allowed") {
      zero_matrices_allowed = IsAffirmative(valstr);
      return true;
    }

    if (key=="usezero") {
      usezero = IsAffirmative(valstr);
      return true;
    }

    if (key=="limitrelevance"){
      limitrelevance = IsAffirmative(valstr);
      return true;
    }

    if (key=="bordercompensation"){
      bordercompensation = IsAffirmative(valstr);
      return true;
    }

    if (key=="relativescores"){
      relativescores = IsAffirmative(valstr);
      return true;
    }

    if(key=="readnbr"){
      readnbr = IsAffirmative(valstr);
      return true;
    }

    if (key=="canbeshownlogic"){
      if (valstr=="and") {
        can_be_shown_and = true;
        return true;
      }
      if (valstr=="or") {
        can_be_shown_and = false;
        return true;
      }
    }

    if (key=="imgcols") {
      imgcols = intval;
      return true;
    }

    if (key=="showstats") {
      showstats = intval;
      return true;
    }

    if (key=="maxquestions") {
      MaxQuestions(intval);
      return true;
    }

    if (key=="skipoptionpage") {
      skipoptionpage = IsAffirmative(valstr);
      return true;
    }

    if (key=="vqlevel") {
      VQlevel(intval);
      return true;
    }

    if (key=="matrixcount") {
      DefaultMatrixCount(intval);
      return true;
    }

    if (key=="entropymatrix") {
      entropymatrix = intval;
      return true;
    }

    if (key=="calculate_entropy") {
      CalculateEntropy(IsAffirmative(valstr));
      return true;
    }

    if (key=="negative_weight") {
      negative_weight = floatval;
      return true;
    }

    if (key=="aspects") {
      if (!SetAspectWeights(valstr))
	return ShowError(h+"SetAspectWeights("+valstr+") failed");
      return true;
    }

    if (key.find("algorithm")==0) {
      if (valstr.substr(0, 9)=="external/") {
        algorithm = cbir_external;
        external_algorithm = valstr.substr(9);
      } else {
        algorithm = PicSOM::CbirAlgorithm(valstr);

        size_t ai = 0;
        if (!InterpretIndex(key.substr(9), ai))
          return ShowError(h+"InterpretIndex("+key.substr(9)+") failed");

        if (ai>algorithms.size())
          return ShowError(h+"algorithm[index] too large in <"+key+">");

        if (algorithm==cbir_no_algorithm && valstr!="no_algorithm") {
          if (the_db==NULL)
             return ShowError(h+"Setting algorithm before database "
                              "is not allowed.");

          string alg, paren, algorname = valstr;
          if (SplitParentheses(valstr, alg, paren))
            algorname = alg;

          CbirAlgorithm *a = the_db->FindOrCreateAlgorithm(algorname);

          algorithm_data algor = {
            ai, algorname, a, NULL, ObjectList()
          };

          bool ok = a;
          ok = ok && a->Initialize(this, paren, algor.data);
          //          ok = ok && algs[0].algorithm->SetDatabase(the_db);
          if (!ok)
            return ShowError(h+"Initialization of CbirAlgorithm "+algorname+
                             " failed");

          if (ai==algorithms.size())
            algorithms.push_back(algor);
          else
            algorithms[ai] = algor; // should something be deleted first?
        }
        return true;

      }
      return algorithm!=cbir_no_algorithm || valstr=="no_algorithm";
    }

    if (SetStageFunction(key, valstr, false))
      return true;

    if (key=="bottomonly") {
      ShowError(h+"Option \"bottomonly\" is obsoleted. ", 
                "Use algorithm \"picsom_bottom\" instead.");
      picsom->SaveAllAndExit();
    }

    if (key=="hash_modulo") {
      Obsoleted("hash_modulo=xx");
      return true;
    }

    if (key=="statistics") {
      if (!the_db) {
        ShowError(h+"statistics directive encountered when db not set");
        res = 0;

      } else the_db->NoStatistics(!IsAffirmative(valstr));

      return true;
    }

    if (key=="include_minimal") {
      include_minimal = IsAffirmative(valstr);
      return true;
    }

    if (key=="include_snippet") {
      include_snippet = IsAffirmative(valstr);
      return true;
    }

    if (key=="include_origins_info") {
      include_origins_info = IsAffirmative(valstr);
      return true;
    }

    if (key=="rndseed") {
      if (valstr=="random")
        RndSeedRandomize();
      else if (valstr=="random_each_round") {
	RndSeedRandomize();
	rndseed_random_each_round = true;
      } else {
        rndseed_random = false;
        rndseed_value  = intval;
      }
      return true;
    }

    if (key=="convtype") {
      _sa_default_convtype = valstr;
      return true;
    }

    if (key=="bmuconvtype") {
      _sa_default_bmuconvtype = valstr;
      return true;
    }

    if (key=="greymaps" || key=="graymaps") {
      greymaps = intval;
      return true;
    }

    if (key=="maxposvalue") {
      maxposvalue = floatval;
      return true;
    }

    if (key=="maxnegvalue") {
      maxnegvalue = floatval;
      return true;
    }

    if (key=="mapimagename") {
      mapimagename = valstr;
      return true;
    }

    if (key=="mapmatlabname") {
      mapmatlabname = valstr;
      return true;
    }

    if (key=="mapdatname") {
      mapdatname = valstr;
      return true;
    }

    if (key=="scorefile") {
      scorefile = valstr;
      return true;
    }

    if (key=="matlabdump") {
      matlabdump = valstr;
      return true;
    }

    if (key=="datdump") {
      datdump = valstr;
      return true;
    }

    if (key.find("response")==0)
      return ProcessResponse(key.substr(8), valstr);

    if (key=="target") {
      Target(SolveTargetTypes(valstr));
      if ((int)Target()==0 && valstr!="no_target") {
        ShowError("Query::Interpret(", key+","+valstr,
                  ") target type not known");
        res = 0;
      }
      return true;
    }

    if (key=="relevance_to_siblings") {
      RelevanceToSiblings(IsAffirmative(valstr));
      return true;
    }

    if (key=="store_selfinfluence") {
      store_selfinfluence = IsAffirmative(valstr);
      return true;
    }

    if (key=="hitstype") {
      hitstype=string2hitstype_t(valstr);
      if (hitstype==unknown) {
        ShowError(h+"Unknown hitstype ",valstr);
        res = 0;
      }      
      return true;
    }

    if (key=="hit_comb_factor") {
      hit_comb_factor = floatval;
      return true;
    }

    if (key=="use_google_image_search") {
      use_google_image_search = IsAffirmative(valstr);
      return true;
    }

    if (key=="textquery") {
      textquery = valstr;
      return true;
    }

    if (key=="correct_person") {
      textquery_correct_person = IsAffirmative(valstr);
      return true;
    }

    if (key=="correct_location") {
      textquery_correct_location = IsAffirmative(valstr);
      return true;
    }

    if (key=="check_commons") {
      check_commons_classes = IsAffirmative(valstr);
      return true;
    }

    if (key=="commons_useaspects") {
      commons_classes_useaspects = IsAffirmative(valstr);
      return true;
    }

    if (key=="add_classfeatures") {
      add_classfeatures = IsAffirmative(valstr);
      return true;
    }

    if (key=="pre2122bug") {
      Pre2122Bug(IsAffirmative(valstr));
      return true;
    }

    if (key=="classaugment") {
      SetAugmentValue(floatval);
      return true;
    }
    
    if (key=="commonsfile") {
      commonsfile = valstr+".dict";
      return true;
    }
    
    if (key=="classfeaturesfile") {
      classfeaturesfile = valstr+".dict";
      return true;
    }
    
    if (key=="classifyparams") {
      vector<string> vals = SplitInCommas(valstr);
      vals.push_back("");
      vals.push_back("");
      SetClassifyParams(vals[0], atoi(vals[1].c_str()), vals[2]);
      return true;
    }

    if (key=="classifymethod") {
      ClassifyMethod(valstr);
      return true;
    }

    if (key=="classifynclasses") {
      ClassifyNClasses(intval);
      return true;
    }
    
    if (key=="classificationset") {
      classificationset = valstr;
      return true;
    }
    
    if (key.find("phase")==0) {
      string s = key.substr(5);
      size_t c = s.find(':'), l = 1;
      if (c==string::npos) // setting.xml cannot have <phase2:features>
        c = 0;             // so we'll use <phase2features>
      else
        l = c;
      if (c>=s.size()-1)
        return ShowError(h+"processing phaseXX:YYY=ZZZ failed");
      phase_script[s.substr(0, l)].push_back(make_pair(s.substr(c+1), valstr));
      return true;
    }
    
    if (key=="maxrounds") {
      maxrounds = intval;
      return true;
    }

    if (key=="defaultgazerelevance") {
      default_gaze_relevance = floatval;
      return true;
    }

    if (key=="endpage") {
      endpage = valstr;
      return true;
    }

    if (key=="erfpolicy") {
      erfpolicy = valstr;
      return true;
    }

    if (key=="erfkeymap") {
      erfkeymap = valstr;
      return true;
    }

    if (key=="erfsplitdata") {
      erfsplitdata = IsAffirmative(valstr);
      return true;
    }

    if (key=="erfptrstogaze") {
      erfptrstogaze = IsAffirmative(valstr);
      return true;
    }

    if (key=="erfdatafile") {
      erfdatafile.open(valstr.c_str());
      if (!erfdatafile) {
	res = 0;
	return ShowError(h+"unable to open <"+valstr+">");
      }
      return true;
    }

    if (key=="forcecbirstages") {
      forcecbirstages = IsAffirmative(valstr);
      return true;
    }

    if (key=="debugerf_features") {
      DebugErfFeatures(intval);
      return true;
    }

    if (key=="debugerf_relevance") {
      DebugErfRelevance(intval);
      return true;
    }

    if (key=="querydefault") {
      string dkey = valstr, dval;
      size_t e = dkey.find('=');
      if (e!=string::npos) {
        dval = dkey.substr(e+1);
        dkey.erase(e);
      }
      Picsom()->StoreDefaultKeyValue("query", dkey, dval);
      return true;
    }

    if (key=="xsl") {
      vector<string> xslv = SplitInCommas(valstr);
      xsl.clear();
      xsl.insert(xsl.begin(), xslv.begin(), xslv.end());
      return true;
    }

    if (key=="tnsize") {
      tnsize = valstr;
      return true;
    }

    if (key=="tncolumns") {
      tncolumns = (size_t)intval;
      return true;
    }

    if (key=="setup") {
      if (!the_db)
	res = ShowError(h+"database not set before setup="+valstr);
      else {
        vector<string> part = SplitInSomething("+", false, valstr);
        if (part.size()>1) {
          for (size_t i=0; i<part.size(); i++)
            if (!Interpret(key, part[i], res, conn) || res!=1) {
              ShowError(h+"setup="+valstr+" failed with <setup>=<"+
                        part[i]+">");
              break;
            }
          return true;
        }

	script_exp_t script = the_db->SetupScript(valstr);
	if (script.empty())
	  res = ShowError(h+"setup="+valstr+" is empty");
	else {
          for (script_exp_t::const_iterator i=script.begin();
               i!=script.end(); i++)
            if (!Interpret(i->first, i->second, res, conn) || res!=1) {
              ShowError(h+"setup="+valstr+" failed with <"+i->first+
                        ">=<"+i->second+">");
              break;
            }
        }
      }
      return true;
    }

    if (key=="setupname") {
      if (setupname!="")
        setupname += "+";
      setupname += valstr;
      return true;
    }

    if (key=="speechrecognizer") {
      speechrecognizer = valstr;

      if (!Picsom()->SpeechRecognizerRunning() &&
	  !Picsom()->StartSpeechRecognizer(valstr)) {
	speechrecognizer = "";
	res = ShowError(h+"PicSOM::StartSpeechRecognizer() failed");
      }

      return true;
    }

    if (key=="speechsync") {
      speechsync = valstr;
      return true;
    }

    if (key=="keysync") {
      keysync = valstr;
      return true;
    }

    if (key=="queryrestriction") {
      SetTemporalRestriction(valstr, Target(), -1);
      return true;
    }

    if (key=="initialset_pre") { 
      initialset_pre = valstr;
      SetStageFunction("initial_set", "select_from_pre", true);
      return true;
    }

    if (key=="prescore_function") {
      if (valstr=="log")
        prescore_function = &guardedlog;
      else if (valstr=="nothing")
        prescore_function = &do_nothing;
      else
        return false;
      return true;
    }

    string n, a;
    if (SplitParentheses(key, n, a)) {
      // this logic is a bit odd...
      // would it better to define these like:
      //   features=...,xxx(convtype=zzz),...
      // than like:
      //   features=...,xxx,...
      //   convtype(xxx)=zzz
      //   background(xxx)=zzz

      if (n=="classaugment") {
        //n=classaugment a=person,rgb floatval=2
        string::size_type comma = a.find_first_of(",");
        if (comma == string::npos)
          SetAugmentValue(floatval,a);
        else {
          string b=a.substr(comma+1);
           a = a.substr(0,comma);
          SetAugmentValue(floatval,a,b);
        }
        return true;

      } else if (n=="convtype" || n=="weight" || n=="background" ||
                 n=="bmuconvtype") {
        int idx = IndexShortNameIndex(a); // might be IndexFullNameIndex()
        if (idx>=0) {
          if (n=="convtype")
            ConvType(idx, valstr);
          else if (n=="weight")
            Weight(idx, floatval);
          else if (n=="background")
            TsSom(idx).SetBackGround(valstr);
          else if (n=="bmuconvtype")
            BmuConvType(idx, valstr);
        } else {
          ShowError(key, " : feature name not resolved");
          res = 0;
        }
        return true;

      } else if (n=="classweight") {
        if (a=="")
          classweight.clear();
        else {
          if (!GetDataBase()) {
            ShowError(key, " : database not set");
            res = 0;
          }
          ground_truth g = GetDataBase()->GroundTruthExpression(a);
          classweight.push_back(pair<ground_truth,float>(g, floatval));
        }
        return true;
      }
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::InterpretDefaults(int stage) {
    string msg = "Query::InterpretDefaults() stage="+ToStr(stage)+" : ";
    bool ok = true, debug = Picsom()->DebugInterpret();

    static set<string> stage1;
    if (stage1.empty()) {
      stage1.insert("algorithm");
      stage1.insert("maxquestions");
      stage1.insert("erfsplitdata");
    }

    int ret = 1, r = ret;
    list<pair<string,string> > kvl = Picsom()->DefaultKeyValues("query");
    for (list<pair<string,string> >::const_iterator i=kvl.begin();
         i!=kvl.end(); i++) {
      bool is_stage1 =  stage1.find(i->first)!=stage1.end();
      if ((is_stage1 && stage!=1) || (!is_stage1 && stage==1)) {
	if (debug)
	  cout << msg << Identity() << " " << (void*)this << " "
	       << i->first << "=" << i->second << " SKIPPED" << endl;
	
	continue;
      }

      bool okx = Interpret(i->first, i->second, r, NULL);
      ok = ok && okx;
      if (!okx)
	ShowError(msg+"key <"+i->first+"> unknown");

      if (debug)
        cout << msg << Identity() << " " << (void*)this << " "
	     << i->first << "=" << i->second << " r=" << r << " okx=" << okx
	     << " ok=" << ok << endl;
      if (r!=1)
        ret = r;
    }
    return ok && ret==1;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::InterpretIndex(const string& s, size_t& i) {
    i = 0;
    if (s=="")
      return true;

    string t = s;
    if (s[0]=='[') {
      if (s[s.size()-1]!=']')
        return false;
      else
        t = t.substr(1, t.size()-2);
    }

    if (t.find_first_not_of("0123456789")!=string::npos)
      return false;

    i = atoi(t.c_str());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PhaseChangeByAspect(const set<string>& a) {
    bool ok = true;

    for (set<string>::const_iterator i=a.begin(); ok && i!=a.end(); i++)
      for (list<phase_rule_t>::const_iterator j=phase_rule.begin();
           ok && j!=phase_rule.end(); j++)
        if (*i==j->aspect && phase_current==j->from) {
          if (debug_phases)
            cout << "PHASE: using rule [" << j->str() << "]" << endl;
          ok = ChangePhase(j->to);
        }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ChangePhase(const string& p) {
    string err = "Query::ChangePhase(\""+p+"\") : ";
    string msg = "from \""+phase_current+"\" to \""+p+"\"";

    if (phase_script.find(p)==phase_script.end())
      return ShowError(err+"phase not found");

    if (debug_phases)
      cout << "PHASE: about to change " << msg << endl;

    const list<pair<string,string> >& pl = phase_script[p];

    if (!Picsom()->Interpret(pl.begin(), pl.end(), this, analysis, NULL))
      return ShowError(err+"failed");

    if (debug_phases)
      cout << "PHASE: successfully changed " << msg << endl;

    phase_current = p;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::ParseResponseList(xmlNodePtr node, const string& nodename, 
                                const string& childname, response_list& rl) {
    for (; node; node=node->next) {
      if (node->children && NodeName(node)==nodename) {
        xmlNodePtr listnode = node->children;
        for (; listnode; listnode=listnode->next) {          
          if (NodeName(listnode)==childname) {
            string resvalue = GetProperty(listnode, "value");
            string reslabel = GetProperty(listnode, "label");
            if (resvalue!="") {
              pair<string,string> p(resvalue,reslabel);
              rl.push_back(p);
            }
          }
        }
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessResponse(const string& vstr, const string& oin) {
    string msg = "ProcessResponse("+vstr+","+oin+") : ";

    string o = oin;

    if (oin.find("http://")==0) {
      XmlDom xml;
      vector<size_t> idxs;
      Analysis a(Picsom(), NULL, this, vector<string>());
      if (!a.InsertOneFileSerial(oin, xml, &idxs, "", "", "", false))
	return ShowError(msg+"failed to download \""+oin+"\"");
      o = Label(idxs[0]);
    }

    int idx = LabelIndex(o);
    if (idx<0)
      return ShowError(msg+"failed idx<0");

    /* // this "v" does not seem to be used any more as of 2011-08-16
       // perhaps we should use "v" instead of "1.0" below
    double v = 1.0;
    if (vstr=="plus")
      v = 1.0;
    else if (vstr=="minus")
      v = -1.0;
    else if (vstr=="zero")
      v = 0.0;
    else
      return ShowError(msg+"failed");
    */

    string aspect = "";
    MarkAsSeenNoAspects(idx, 1.0);
    ProcessAspectInfo(aspect, idx, 1.0);
 
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessObjectResponseList(xmlNodePtr node) {
    response_list rl;
    ParseResponseList(node, "imageresponselist", "imageresponse", rl);
    for (response_list::iterator it=rl.begin(); it != rl.end(); it++)
      ProcessObjectInfo(it->first, it->second, false);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::ProcessObjectInfo(const string& v, const string& l, bool a) {
    // lengthen some vectors if they are not long enough
    // ConditionallyLengthenVectors(); now in Connection::TryToRunXMLresponse()
    // obs! Should the metadatas be updated according to ReadMetaDataFile() ???

    // Case 0: "X" (meaning leave all images as they were)
    if (v.substr(0, 1)=="X") {
      SetBackFromSettings();
      return;
    }

    // Case 1: "[+-0] imagelabel | ?* | +*" 
    if (l!="") {    
      char ch = v[0];

      if (l=="?*")
        MarkAllNewObjectsAsSeen(ch, a);

      else if (l=="+*")
        ChangeAllSeenObjects('+', ch, a);

      else {
        int idx = LabelIndex(l);
        double val = MarkCharToValue(ch);
        MarkAsSeenNoAspects(idx, val);
        if (a)
          ProcessAspectInfo("", idx, val);
      }

      return;   
    }

    Obsoleted("Query::ProcessObjectInfo() from the middle...");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessTextQueryOptions(xmlNodePtr node) {
    for (; node; node=node->next) {
      string nodename = NodeName(node);

      if (nodename.substr(0,18)=="textquery_correct_" || 
          nodename.substr(0,16)=="textquery_check_") {
        string corrname = nodename.substr(10);
        
        int res;
        Interpret(corrname,NodeChildContent(node) , res, NULL);
      }
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessClassStates(xmlNodePtr node) {
    string msg = Identity()+" : ProcessClassStates() : ";

    if (debug_classify)
      cout << msg << "Begin" << endl;

    // Remove commentation when classstate information gets correctly
    // propageted back and forth to the servant...  MATS?
    // ClearClassAugmentations();

    response_list rl;
    ParseResponseList(node, "classstates", "classstate", rl);
    
    for (response_list::iterator it=rl.begin(); it != rl.end(); it++)
      ProcessClassState(it->first, it->second);

    if (debug_classify)
      cout << msg << "End, containing a total of " 
           << GetDataBase()->NclassModelsAll() << " class models" << endl;

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  void Query::ProcessClassState(const string& v, const string& l) {
    string msg = Identity()+" : ProcessClassState() : ";
    
    if (debug_classify)
      cout << msg << "Class state: " << l << " = " << v << endl;

    if (v == "off") {
      if (debug_classify)
        cout << msg << "Skipping class: " << l << endl;
      return;
    } else {
      if (v != "plus" && v != "minus")
        ShowError("ProcessClassState(): Unrecognized value [", v, 
                  "] for class [", l, "]");

      if (debug_classify)
        cout << msg << "Adding class: " << l << " as "
             << ((v == "minus") ? "negative" : "positive") << endl;

      SetAugmentValue((v == "minus"?-1.0:1.0),l);
    }
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessSelectedAspects(xmlNodePtr node) {
    bool ok = true;

    set<string> all_a;

    for (; ok && node; node=node->next) {
      if (node->children && NodeName(node)=="selectedaspects") {

        // the aspect relevance information is cleared only if the 
        // <selectedaspects> node is found
        ClearAspectRelevance(); // first set all the aspects as not selected

        xmlNodePtr listnode = node->children;
        for (; ok && listnode; listnode=listnode->next) {          
          if (NodeName(listnode)=="aspect") {
            string aspect  = GetProperty(listnode, "name");
            string imlabel = GetProperty(listnode, "objectlabel");
            string type    = GetProperty(listnode, "type");
            vector<string> params;
            if (type.compare("coordinates") == 0) {
              string xcoord = GetProperty(listnode, "x");
              string ycoord = GetProperty(listnode, "y");
              params.push_back(xcoord);
              params.push_back(ycoord);
            }
        
            all_a.insert(aspect);
    
            double v = 1.0; // obs! hard-coded value
            ok = ProcessAspectInfo(aspect, LabelIndex(imlabel), v,
                                   type, params);
          }
        }
      }
    }

    PhaseChangeByAspect(all_a);

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAspectInfo(const string& aspect, int idx, double v,
                                const string& type,
                                const vector<string>& params) {
    string msg = "ProcessAspectInfo() : ";

    if (!IsSeen(idx))
      return ShowError(msg+"can't set aspect relevance \""+aspect+"\"="+
                       ToStr(v)+" for an unseen object #"+ToStr(idx));

    Object *o = FindInSeen(idx);
    if (!o)
      return ShowError(msg+"object is seen but not found");

    if (debug_aspects)
      cout << "ASPECTS: aspect=[" << aspect << "] idx=" << idx
           << " v=" << v << " type=<" << type << ">" << endl;

    if (!o->AspectRelevance(aspect, v, type, params)) {
      aspect_data ad = o->AspectData(aspect);
      return ShowError(msg+"Object::AspectRelevance() failed with idx="
		       +ToStr(idx)+" aspect=["+aspect+"] data="+ad.str());
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::ClearAspectRelevance() {
    for (size_t i=0; i<NseenObjects(); i++) {
      seenobject[i].ClearAspectRelevance();
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetAspectInfo(map<string,pair<string,float> >& wm,
                            const string& a, const string& s) {
    float  w = 1.0;
    string c = s;

    size_t p = s.find('/');
    if (p!=string::npos) {
      w = atof(s.substr(p+1).c_str());
      c = s.substr(0, p);
    }

    wm[a] = make_pair(c, w);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetAspectWeights(const string& s) {
    aspect_weight.ClearAspects();
    vector<string> av = SplitInCommasObeyParentheses(s);
    if (av.size()==0)
      return false;

    for (size_t i=0; i<av.size(); i++) {
      string aname, apar;
      SplitParentheses(av[i], aname, apar, true);
      float weight = 1.0;
      if (apar!="") {
	if (apar.find('=')==string::npos)
	  return false;

	pair<string,string> kv = SplitKeyEqualValueNew(apar);
	if (kv.first!="weight")
	  return false;
	
	weight = atof(kv.second.c_str());
      }
      aspect_weight.AspectRelevance(aname, weight, "");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AspectsMatch(const Index::State& fd, const Object& o,
                           double& pos, double& neg) const {
    bool debug = debug_aspects, ret = false;

    const vector<string>&          fda = fd.aspects;
    const map<string,aspect_data>& oba = o.Aspects();

    bool fda_star = fda.size()==1 && fda[0]=="*";
    set<string> fds;
    if (!fda_star)
      fds.insert(fda.begin(), fda.end());
    
    if (debug)
      cout << "AspectsMatch() : " << o.DumpStr() << " fda.size()="
	   << fda.size() << " [" << CommaJoin(fda) << "]" << endl;

    pos = neg = 0.0;

    for (map<string,aspect_data>::const_iterator j = oba.begin();
         j!=oba.end(); j++) {
      if (debug)
        cout << "   \"" << j->first << "\"=" << j->second.value;
      if (j->second.value!=0.0 &&
          (fda_star || fds.find(j->first)!=fds.end())) {
	double weight = GetAspectWeight(j->first);
	if (debug)
	  cout << " weight=" << weight;

        if (j->second.value>0) {
	  ret = true;
          pos += j->second.value*weight;
        } else if (j->second.value<0) {
	  ret = true;
          neg += j->second.value*weight;
	}
        if (debug)
          cout << " pos=" << pos << " neg=" << neg;
      }
      if (debug)
        cout << endl;
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  textsearchspec_t Query::TextSearchParams(const string& specin) const {
    string spec = specin;
    if (spec=="")
      spec = textsearch;

    textsearchspec_t ret(spec);
    ret.db = GetDataBase();
    ret.textindex  = textindex;
    ret.textfield  = textfield;
    ret.queryfield = queryfield;

    if (ret.db && spec!="")
      ret = ret.db->TextSearch(spec);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessTextQuery() {
    string msg = "Query::ProcessTextQuery() : ";

    bool my_debug = true; // set to true for debug output

//     const string& dbname = GetDataBase()->BaseName();

    string txtq = TextQuery();
    if (txtq=="")
      return true;

    vector<string> log;
    set<string> common_classes_pos, common_classes_neg;

    RegExp re("(Find\\s+)(\\w+\\s+)?(shots\\s+"
              "(of|with|in\\s+which|that\\s+contain|containing)\\s+)");
    vector<RegExpMatch> m = re.match(txtq, 4);
    
    if (m.size()) {
      txtq = txtq.substr(0, m[0].start())+
        (m[1].start()>=0?txtq.substr(m[1].start(),m[1].length()):"")+
        txtq.substr(m[2].end());
      log.push_back("DELETE("+m[0].str()+m[2].str()+")");
    }

    vector<string> words = textdata::split_string(txtq, splitchars);

    // variables for accumulation of proper words (ie consecutive
    // capitalised words)
    vector<string> proper_accum;
    //    vector<string>::iterator proper_start = words.end();
    unsigned int proper_start = words.size();
    bool neg_found = false;

    for (size_t idx=0; idx<words.size(); idx++) {
      string orig_word = words[idx];

      // if first characters is upper case add word to proper_accum
      if (isupper(orig_word[0])) {
        if (proper_accum.empty()) proper_start = idx;
        proper_accum.push_back(orig_word);
      } 
      
      bool uppercase = isupper(orig_word[0]);

      // if proper name accumulated 
      if (!proper_accum.empty() && 
          (!uppercase || idx==words.size()-1)) {
        bool is_person;
        double shortest_dist = -1;
        string proper_string = JoinWords(proper_accum,true);

        string found_word = CheckProperName(proper_string, is_person, 
                                            shortest_dist);
        
        // known proper name found
        if (shortest_dist >= 0)
          log.push_back(string(is_person?"PERSON":"LOCATION")+"("+
                        proper_string+")");

        // known proper name, but needs correction
        if (shortest_dist > 0) {
          vector<string> proper_words = 
            textdata::split_string(found_word, splitchars);

          words.erase(words.begin()+proper_start,
                      words.begin()+proper_start+proper_accum.size());
          words.insert(words.begin()+proper_start,proper_words.begin(),
                            proper_words.end());
          idx += proper_words.size() - proper_accum.size();
          log.push_back("CORRECT("+found_word+")");

        } else if (shortest_dist == -1)
          log.push_back(proper_string);
        
        if (shortest_dist >= 0 && is_person && check_commons_classes) {
          common_classes_pos.insert("face");
          common_classes_pos.insert("person");
          log.push_back("CLASS(face)");
          log.push_back("CLASS(person)");
        }

        // initialise proper_accum
        proper_accum.clear(); proper_start = words.size();
      }
      
      if (!uppercase) {
        set<pair<string,string> > found_classes;

        if (check_commons_classes) {
          // CheckCommons(orig_word, found_classes);
          CheckConcepts("svmdb#lscom-demo", orig_word, found_classes);
        }

        if (found_classes.size()) {
          log.push_back("COMMON("+orig_word+")");
            
          for (set<pair<string,string> >::iterator fcit = found_classes.begin();
               fcit != found_classes.end(); fcit++) {
            string fclassandpar = fcit->first, namebase;
            bool feat = fclassandpar.find("feature:", 0)==0;
            float w_common = 1.0;
            if (feat) {
              fclassandpar = fclassandpar.substr(8);
              string inpar;
              if (!SplitParentheses(fclassandpar, namebase, inpar))
                namebase = fclassandpar;
              vector<string> kvlist = SplitInCommas(inpar);
              for (size_t kvi=0; kvi<kvlist.size(); kvi++) {
                pair<string,string> kv = SplitKeyEqualValueNew(kvlist[kvi]);
                if (kv.first=="weight")
                  w_common = atof(kv.second.c_str());
                else
                  ShowError(msg+"feature key <"+kv.first+"> not processed");
              }

            } else
              namebase = fclassandpar;

            const string& args = fcit->second;
            float w_own = 1.0, w = w_common;
            if (args!="")
              w = w_own = atof(args.c_str());

            log.push_back(string(neg_found?"NOT_":"")+(feat?"FEATURE":"CLASS")
                          +"("+namebase+(w!=1.0?"="+ToStr(w):"")+")");
            if (feat) {
              // obs! HAS_CBIRALGORITHM_NEW problem
              SelectIndex(NULL, namebase);
              Weight(IndexShortNameIndex(namebase),neg_found?-w:w);
            } else {
              if (neg_found)
                common_classes_neg.insert(namebase);
              else {
                common_classes_pos.insert(namebase);
                if (add_classfeatures)
                  AddClassFeatures(namebase, log);
              }
            }
          } 
          neg_found = false;
        } else
          log.push_back(orig_word);
      }

      if (orig_word == "not" || orig_word == "no")
        neg_found = true;
    }

    // hard-coded removal of studio class
//     if (check_commons_classes) {
//       string studio_name = dbname == "trecvid2007" ? "Studio" : "studio";
//       common_classes_neg.insert(studio_name);
//       log.push_back("NOT_CLASS("+studio_name+")");
//     }

    // join words into string and preprocess
    string newtextquery = JoinWords(words, true);
    TextQueryProcessed(newtextquery);

    textsearchspec_t tsp = TextSearchParams();

    implicit_textquery = newtextquery;
    DataBase * db = GetDataBase();
    if (tsp.textfield!="")
      for (size_t i=0; i<NseenObjects(); i++)
	if (seenobject[i].Value()>0) {
	  list<pair<string,string>> doc =
	    db->TextIndexRetrieve(seenobject[i].Index(), tsp.textindex);
	  map<string,string> docmap(doc.begin(), doc.end());
	  string txt = docmap[tsp.textfield];
	  implicit_textquery += (implicit_textquery!=""?" ":"")+txt;
	}

    if (my_debug) {
      cout << "Setting textquery to: \"" << newtextquery << "\"" << endl;
      cout << "Setting implicit textquery based on textfield=\""
	   << tsp.textfield << "\" to: \"" << implicit_textquery << "\""
	   << endl;

      cout << "Textquery process log: " << endl;
      for (vector<string>::iterator lit = log.begin(); lit != log.end(); lit++)
        cout << *lit << " ";
      cout << endl << endl;
    }

    if (check_commons_classes) {
      for (set<string>::iterator it=common_classes_pos.begin(); 
           it != common_classes_pos.end(); it++) {
        cout << "Processing positive class: " << *it << endl;
        if (!algorithms.empty())
          for (size_t i=0; i<algorithms.size(); i++)
            algorithms[i].data->SetClassAugmentValue(1.0,*it);
        else 
          commons_classes_useaspects ? 
            SetAugmentValueBasedOnAspect(1.0,*it) : SetAugmentValue(1.0,*it);
      }

      for (set<string>::iterator it=common_classes_neg.begin(); 
           it != common_classes_neg.end(); it++) {
        cout << "Processing negative class: " << *it << endl;
        if (!algorithms.empty())
          for (size_t i=0; i<algorithms.size(); i++)
            algorithms[i].data->SetClassAugmentValue(-1.0,*it);
        else 
          commons_classes_useaspects ? 
            SetAugmentValueBasedOnAspect(-1.0,*it) : SetAugmentValue(-1.0,*it);
      }
    }

    bool use_db_text_index = true;
    // bool expand_to_subobjects = false; // true;
    if (use_db_text_index && GetDataBase()->HasTextIndices()) {
      string qtxt = newtextquery + " " + implicit_textquery;

      ground_truth rgt = RestrictionGT();

      string txtclass = ":"+newtextquery;
      for (;;) {
	size_t p = txtclass.find(' ');
	if (p==string::npos)
	  break;
	txtclass[p] = '_';
      }

      ground_truth txtgt(GetDataBase()->Size(), -1);
      txtgt.label(txtclass);
      txtgt.text("\""+newtextquery+"\"");

      if (my_debug)
	cout << "running textindexsearch textsearch=\"" << textsearch
	     << "\" textindex=\"" << tsp.textindex << "\" textfield=\""
	     << tsp.textfield << "\" with \"" << qtxt << "\"" << endl;

      textsearchresult = GetDataBase()->TextIndexSearch(qtxt, tsp.textindex,
							tsp.textfield);

      for (auto i=textsearchresult.begin(); i!=textsearchresult.end(); i++) {
	cout << i->idx << " " << Label(i->idx) << " " << i->val
	     << " " << (int)rgt[i->idx]  << endl;
	float val = i->val;
	if (val>1.0)
	  val = 1.0;

	if (rgt[i->idx]==1) {
	  SetAugmentValue(val, txtclass);
	  txtgt[i->idx] = 1;
	}

	i->txt = "<b>"+i->txt+"</b>";

	// const vector<int>& so =  GetDataBase()->SubObjects(i->idx);
	// if (expand_to_subobjects)
	//   for (auto j=so.begin(); j!=so.end(); j++) {
	//     cout << "  " << *j << " " << Label(*j) << " " << i->val
	// 	 << " " << (int)rgt[*j] << endl;
	//     if (rgt[*j]==1)
	//       SetAugmentValue(1.0, Label(*j));
	//   }
      }

      GetDataBase()->ContentsUpdateOrInsert(txtgt);
    }

    if (use_google_image_search)
      DoGoogleImageSearch(newtextquery, 20);

    GetDataBase()->TextQueryLog(JoinWords(log));

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddClassFeatures(const string& c, vector<string>& log) {
    bool debug = true;

    if (debug)
      cout << "AddClassFeatures(" << c << ")" << endl;

    typedef multimap<string,string> m_t;
    typedef m_t::const_iterator i_t;
    const m_t& m = GetDataBase()->GetClassFeatures(classfeaturesfile);

    pair<i_t,i_t> range = m.equal_range(c);

    bool ok = true;

    for (i_t i=range.first; ok && i!=range.second; i++) {
      bool added = false;
      if (IndexFullNameIndex(NULL, i->second)==-1)
        ok = added = SelectIndex(NULL, i->second);

      if (debug)
        cout << "  " << i->second << (added?" added":" existed") << endl; 

      if (added)
        log.push_back("+FEATURE("+i->second+")");
    }

    if (debug)
      cout << endl;

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::JoinWords(vector<string>& words, bool preprocess) {
    string res;
    WordHist tb;

    for (vector<string>::iterator wp=words.begin(); wp!=words.end(); wp++) {
      string word = *wp;
      if (preprocess) 
        word = tb.WordPreProcess(word);
      if (!word.empty())
        res += (res.empty() ? "" : " ") + word;
    }
    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::CheckProperName(const string& proper_string,
                                bool& is_person, double& shortest_dist) {
    bool my_debug = false;
    string found_word;

    DataBase *db = GetDataBase();
    const set<string> &proper_persons = db->GetProperPersons();
    const set<string> &proper_locations = db->GetProperLocations();

    // max_dist depends on length of proper_string (the shorter the
    // proper name the smaller the tolerance)
    double max_dist = floor((double)(proper_string.length()+4)/5);
    if (my_debug) 
      cout << "Checking " << proper_string << " with max_dist=" <<
        max_dist << endl;

    // find person name closest to our word(s)
    double person_dist = max_dist;
    string closest_person = 
      ClosestWord(proper_persons, proper_string, person_dist);

    // find location name closest to our word(s)
    double location_dist = max_dist;
    string closest_location = 
      ClosestWord(proper_locations, proper_string, location_dist);

    bool pick_person = person_dist < location_dist;
    is_person = (!closest_person.empty() && 
                 (person_dist <= max_dist) && pick_person);

    //    shortest_dist = max_dist+1;
    shortest_dist = -1;

    if (textquery_correct_person && 
        (pick_person || !textquery_correct_location)) {
      found_word = closest_person;
      shortest_dist = person_dist;
    }

    if (textquery_correct_location && 
        (!pick_person || !textquery_correct_person)) {
      found_word = closest_location;
      shortest_dist = location_dist;
    }

    if (shortest_dist > max_dist)
      shortest_dist = -1;
      
    return found_word;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::ClosestWord(const set<string>& wordlist, const string& word,
                            double& dist) {
    string ret;
    dist++;  // should now contain max_dist+1

    for (set<string>::const_iterator it = wordlist.begin();
         it != wordlist.end(); it++) {
      double wdist = CalculateWordDistance(*it, word);
      if (wdist < dist) {
        dist = wdist;
        ret = *it;
      }
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CheckCommons(string cword, 
                           set<pair<string,string> >& found_classes) {
    const multimap<string,string>& commons = 
      GetDataBase()->GetCommons(commonsfile);

    typedef multimap<string, string>::const_iterator map_iter_t;

    for (map_iter_t i=commons.begin(); i!=commons.end(); i++) {
      string word=i->first, args="";
      SplitParentheses(i->first, word, args);
      double ld_th = 1.0;
      if (word.length()<4)
        ld_th = 0.001;
      if (word.substr(0, 1) == cword.substr(0, 1) &&
          LevenshteinDistance(word, cword) <= ld_th) {
        vector<string> clist = SplitInCommasObeyParentheses(i->second);
        for (size_t i=0; i<clist.size(); i++)
          found_classes.insert(make_pair(clist[i], args));
      }
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CheckConcepts(const string& cset, const string& cword, 
			    set<pair<string,string> >& found_classes) {
    /// this should be cached...
    list<string> clist = CheckDB()->SplitClassNames(cset);

    string pref = "detection("+cset+"):";

    for (auto i=clist.begin(); i!=clist.end(); i++) {
      string s = *i;
      if (s.find("lscom")==0) {
	s = CheckDB()->LscomName(s, true);
	s = DataBase::SplitLscomName(s).first;
	s = LowerCase(s);
      }

      if (s==cword)
	found_classes.insert(make_pair(pref+*i, ""));
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  double Query::LevenshteinDistance(const string &a, const string &b) {
    size_t i, j, w, h;
    double c;
    w = a.length()+1;
    h = b.length()+1;
    double *dist = new double[w*h];
    
    for(i=0; i<=a.length(); i++)
      dist[i] = i;
    for(i=1; i<=b.length(); i++)
      dist[i*w] = i;
    
    for(i=1; i<=a.length(); i++)
      for(j=1; j<=b.length(); j++) {
        c = a.at(i-1)==b.at(j-1) ? 0 : 1;
        
        // distance is the minimum of the three: deletion, insertion, 
        // substitution cost
        double de, in, su;
        de = dist[i-1 + j     * w] + 1;
        in = dist[i   + (j-1) * w] + 1;
        su = dist[i-1 + (j-1) * w] + c;
        dist[i + j*w] = min(de, min(in, su));
      }
    
    double d = dist[w*h-1];
    delete dist;
    return d;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddOtherKeyValue(const string& key, const string& val) {
    //   keyvaluelist.erase(key);
    //   keyvaluelist.insert(pair<string,string>(key, val));
    keyvaluelist[key] = val;

    if (!picsom->TraceOtherKeys() && !debug_all_keys)
      return true;

    static set<string> known;
    if (known.empty()) {
      known.insert("showarrows");
      known.insert("showmaps");
      known.insert("extrainfo");
      known.insert("mapextrainfo");
      known.insert("showzero");
      known.insert("showpos");
      known.insert("showneg");
      known.insert("showgreen");
      known.insert("imgrows");
      known.insert("thumbnails");
      known.insert("mapclick");
      known.insert("class");
      known.insert("imageinfopopup");
      known.insert("displaymode");
    }

    if (known.find(key)==known.end() || debug_all_keys)
      WriteLog("Imported variable ", key, "=[", val, "]");
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::DumpKeyValues() const {
    cout << "Keys and values: ---" << endl;

    for (map<string,string>::const_iterator i=keyvaluelist.begin();
         i!=keyvaluelist.end(); i++)
      cout << i->first << "=" << i->second << endl;
  
    cout << "--------------------" << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t Query::NlowerAlgorithms(const CbirAlgorithm::QueryData *d) const {
    for (size_t i=0; i<algorithms.size(); i++)
      if (algorithms[i].data==d)
        return algorithms.size()-1-i;

    return ShowError("Query::NlowerAlgorithms() failed");
  }

  /////////////////////////////////////////////////////////////////////////////

  const Query::algorithm_data
  *Query::LowerAlgorithm(const CbirAlgorithm::QueryData *d, size_t j,
			 bool angry) const {
    for (size_t i=0; i<algorithms.size(); i++)
      if (algorithms[i].data==d && i+j+1<algorithms.size())
        return &algorithms[i+j+1];

    if (angry)
      ShowError("Query::LowerAlgorithm() failed");

    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::UnselectIndices(algorithm_data *a, bool silent) {
    if (debug_featsel && !silent)
      cout << Identity() << " : UnselectIndices() : unselecting all" <<endl;
    
    if (!a) {
      for (size_t i=0; i<Nfeatures(); i++)
        delete index_data[i];
      
      index_data.clear();

      return;
    } 

    for (size_t i=0; i<Nindices(a); i++)
      a->algorithm->RemoveIndex(a->data, i);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SelectIndices(algorithm_data *a, const vector<string>& lin,
                            bool create, bool unselect) {
    if (unselect)
      UnselectIndices(a);

    if (!the_db)
      return false;

    if (debug_featsel)
      cout << Identity() << " : SelectIndices(a=" << (a?"non-":"")
	   << "NULL,create=" << (create?"true":"false")
	   << ",unselect=" << (unselect?"true":"false") << "): called with ["
           << CommaJoin(lin) << "]" << endl;

    if (lin.size()==1 && lin[0]=="")
      return true;

    // minus sign in front of alias/regexp is not processed correctly!
    list<string> ll;
    for (vector<string>::const_iterator i=lin.begin(); i!=lin.end(); i++) {
      list<string> exp = the_db->ExpandFeatureAlias(*i);
      if (exp.size()==1)
	exp = the_db->FeatureRegExpList(*i);
      ll.insert(ll.end(), exp.begin(), exp.end());
    }

    if (debug_featsel)
      cout << "SelectIndices() -> [" << CommaJoin(ll) << "]" <<endl;

    bool ok = true;
    for (list<string>::const_iterator i=ll.begin(); i!=ll.end(); i++)
      if (!SelectIndex(a, *i, create)) {
        UnselectIndices(a);
        ok = false;
        break;
      }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SelectIndex(algorithm_data *a,
                          const string& n, bool create) {
    string msg = Identity()+" : SelectIndex(a="+ (a?"non-":"")
      +"NULL,"+n+",create="+(create?"true":"false")+") : ";

    if (!the_db) 
      return ShowError(msg+"no db set");

    string namefull = n;

    bool minus = namefull[0]=='-';
    if (minus)
      namefull.erase(0, 1);

    if (namefull=="")
      return ShowError(msg+"empty feature name");

    string namebase, inpar;
    if (!SplitParentheses(namefull, namebase, inpar))
      namebase = namefull;
    if (namebase=="binary") { // backwards compatibility for now...
      namebase = namefull;
      inpar = "";
    }

    if (debug_featsel)
        cout << msg+"[" << namefull << "] -> [" << namebase
             << "][" << inpar << "]" << endl;

    Index *ts = the_db->FindIndex(namebase, inpar, create);
    if (!ts)
      return ShowError(msg+"<", namebase, "> inexistent in [",
                       the_db->Path(), "]");
    if (ts->IsDummy())
      ts = the_db->FindIndex(namebase, inpar, true, false);

    int i = IndexFullNameIndex(a, namefull);

    if (i<0 && !minus) {
      AddIndex(a, ts, namefull);

      if (debug_featsel)
        cout << msg+"selected " << namebase << " " << ts->MethodName()
             << " [" << ts->FeatureTargetString() << "]" << endl;
      
      i = Nindices(a)-1;
    }

    if (i>=0 && minus) {
      RemoveIndex(a, size_t(i));

      if (debug_featsel)
        cout << msg+"removed " << namebase << endl;

      i = -1;
    }

    if (i>=0 && inpar!="")
      SetIndexParameters(a, size_t(i), inpar);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetIndexParameters(algorithm_data* a, 
                                 size_t fdi, const string& data) {
    if (a)
      return ShowError("SetIndexParameters() : called with a!=NULL");

    if (!IndexIndexOK(fdi))
      return ShowError("SetIndexParameters() : index error");

    // obs! HAS_CBIRALGORITHM_NEW problem
    Index::State& fd = IndexData(NULL, fdi);

    string msg = "Query::SetIndexParameters("+fd.fullname+", ["+data+"]) : ";

    if (debug_featsel)
      cout << "Now in " << msg << endl;

    bool ok = true;
    vector<string> dv = SplitInCommasObeyParentheses(data);
    for (size_t i=0; ok && i<dv.size(); i++) {
      const string& d = dv[i];
      if (d.find('=')==string::npos)
        return ShowError(msg+"= not found in <"+d+">");

      pair<string,string> kv = SplitKeyEqualValueNew(d);

      if (kv.first=="aspects")
        ok = SetIndexAspects(fd, kv.second);

      else if (kv.first=="weight")
        ok = SetIndexWeight(fd, kv.second);

      else if (kv.first=="background") {
        TSSOM& tssom = TsSom(fdi);
        tssom.SetBackGround(kv.second);
        ok = true;
      }

      else if (kv.first=="target")
        ok = true;  // Processed later in Index::Index().

      else
        return ShowError(msg+"key ["+kv.first+"] not understood");
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetIndexWeight(Index::State& fd, const string& a) {
    fd.Weight(atof(a.c_str()));
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetIndexAspects(Index::State& fd, const string& a) {
    fd.aspects.clear();

    // remove the possible parenthesis, since it seems like 
    // SplitInCommasObeyParentheses does not remove them in cases like
    // "aspecs=(color,texture)". Without this hack the output would be aspect
    // strings "(color" and "texture)".
    string as;
    if(a.at(0)=='(' && a.at(a.length()-1)==')')
      as = a.substr(1,a.length()-2);
    else
      as = a;
      
    fd.aspects = SplitInCommasObeyParentheses(as);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  map<string,aspect_data> Query::AspectsFromIndices() const {
    map<string,aspect_data> a;
    set<target_type> tts;

    for (size_t i=0; i<Nfeatures(); i++) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      const vector<string>& fa = IndexData(NULL, i).aspects;
      target_type tt = IndexData(NULL, i).StaticPart()->FeatureTarget();
      tts.insert(tt);

      for (size_t j=0; j<fa.size(); j++)
        if (a.find(fa[j])==a.end()) {
          a[fa[j]] = GetDataBase()->DefaultAspects(tt).AspectData(fa[j]);
          if (debug_aspects)
            cout << "ASPECTS: found [" << fa[j] << "] in <"
                 << IndexData(NULL, i).fullname << "> tt=" << TargetTypeString(tt)
                 << " [" << a[fa[j]].str() << "]" << endl;

        }
    }

    aspects datmp = GetDataBase()->DefaultAspects(target_any_target);
    const map<string,aspect_data>& da = datmp.Aspects();

    for (map<string,aspect_data>::const_iterator i=da.begin();
         i!=da.end(); i++)
      if (i->second.sticky) {
        a[i->first] = i->second;
        if (debug_aspects)
          cout << "ASPECTS: added [" << i->first << "] ["
               << a[i->first].str() << "]" << endl;
      }

    return a;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXML(XmlDom& dom, object_type ot,
                       const string& namestr, const string& specstr,
                       const Connection *conn) {
    bool ok = true;

    switch (ot) {
    case ot_queryinfo:
      ok = ok && AddToXMLqueryinfo(dom, false, false, false, false, conn);
      break;

    case ot_newidentity:
    case ot_variables:
      ok = ok && AddToXMLvariables(dom, conn);
      ok = ok && AddToXMLchildlist(dom, false, false, false, false, conn);
      break;
    
    case ot_objectlist: {
      bool childinfo = specstr.find("brief")==string::npos, full = childinfo;
      ok = ok && AddToXMLobjectlist(dom, conn, childinfo, full);
      break;
    }

    case ot_ajaxresponse:
      ok = ok && AddToXMLajaxresponse(dom, conn);
      break;
    
    case ot_mapimage:
      ok = ok && AddToXMLmapimage(dom, namestr, specstr);
      break;

    case ot_queryimage:
      ok = ok && AddToXMLqueryimage(dom, namestr, specstr);
      break;

    default:
      ok = ShowError("Query::AddToXML() unknown objecttype: ",
                     ObjectTypeP(ot, true));
    }
  
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLqueryinfo(XmlDom& dom, bool longish, bool recurs,
                                bool childinfo, bool full,
                                const Connection *conn) /*const*/ {
    XmlDom info = dom.Element("queryinfo");
    info.Element("identity",     Identity());
    info.Element("databasename", DataBaseName());
    info.Element("client",       FirstClientAddress());
    info.Element("starttime",    TimeString(start));
    info.Element("modtime",      TimeString(last_modification));
    info.Element("accesstime",   TimeString(last_access));

    if (IsRoot()) {
      info.Element("savetime",   TimeString(saved_time));
      info.Element("needs_save", NeedsSave());
      info.Element("from_disk",  from_disk);
      info.Element("nodes",      Nodes());
      info.Element("leaves",     Leaves());
      info.Element("imagecount", Objects());
    }

    if (longish) {
      AddToXMLvariables(info, conn);
      AddToXMLobjects(info, conn, childinfo, full);
    }
  
    AddToXMLajaxdata(info);

    AddToXMLerfdata(info);

    XmlDom link = info.Element("image");
    link.Prop("src", "image/erf-detections/hut-rectors");

    if (full) {// don't want to save these in file...
      AddToXMLinfolinklist(info, "", "");
      AddToXMLvisitedlinklist(info, "", "");
    }

    if (recurs)
      AddToXMLchildlist(info, longish, recurs, childinfo, full, conn);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLinfolinklist(XmlDom& dom, const string&,
				   const string&) const {
    XmlDom ill = dom.Element("infolinklist");

    // XmlDom link = ill.Element("infolink");
    // link.Prop("href", "?refresh=1");
    // link.Prop("linktext", "refresh every second");

    // AddToXMLinfolinklist_detectedobject(ill, "hut-rectors/names-links.xml",
    // 					"small-15-Uronen.jpg");

    // AddToXMLinfolinklist_detectedobject(ill, "hut-rectors/names-links.xml",
    // 					"poster.jpeg");

    multimap<float,string> objrel;

    if (seen_objects.size()) {
      timespec_t last = TimeZero(), gaze_first = last, gaze_last = last;
      for (auto i=seen_objects.begin(); i!=seen_objects.end(); i++) {
	if (MoreRecent(i->second.end, last))
	  last = i->second.end;
	if (MoreRecent(i->second.gaze_last, gaze_last))
	  gaze_last = i->second.gaze_last;
	if (IsTimeZero(gaze_first))
	  gaze_first = i->second.gaze_first;
	if (MoreRecent(gaze_first, i->second.gaze_first))
	  gaze_first = i->second.gaze_first;
      }
      for (auto i=seen_objects.begin(); i!=seen_objects.end(); i++) {
	float r = TimeDiff(i->second.end, last);
	if (!IsTimeZero(gaze_last)) {
	  if (IsTimeZero(i->second.gaze_last))
	    r += TimeDiff(gaze_first, gaze_last);
	  else
	    r += TimeDiff(i->second.gaze_last, gaze_last);
	}
	objrel.insert(make_pair(r, i->first));
      }
    }

    if (objrel.empty()) {
      objrel.insert(make_pair(0.9, "small-15-Uronen.jpg"));
      objrel.insert(make_pair(0.8, "poster.jpeg"));
    }

    size_t n = 0, nmax = 3;
    for (auto i=objrel.rbegin(); i!=objrel.rend() && n<nmax; i++, n++)
      AddToXMLinfolinklist_detectedobject(ill, "hut-rectors/names-links.xml",
					  i->second);

    if (seen_words.size()) {
      string sw;
      for (auto i=seen_words.begin(); i!=seen_words.end(); i++)
	sw += (sw!=""?" ":"")+i->word;

      XmlDom link = ill.Element("infolink");
      link.Prop("href", "http://dummy.com/");
      link.Prop("text", sw);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLinfolinklist_detectedobject(XmlDom& dom, const string& s,
						  const string& o) const {

    // AddToXMLinfolinklist_detectedobject_xml(dom, s, o);

    AddToXMLinfolinklist_detectedobject_sparql(dom, s, o);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLinfolinklist_detectedobject_xml(XmlDom& dom,
						      const string& s,
						      const string& o) const {
    list<string> oips { "linktext", "text", "href", "imgsrc" };

    list<XmlDom> ilist =
      GetDataBase()->FindDetectedObjectInfo(s, o);
    for (auto i=ilist.begin(); i!=ilist.end(); i++) {
      XmlDom il = dom.Element("infolink");
      for (auto p=oips.begin(); p!=oips.end(); p++)
	il.Prop(*p, i->Property(*p));
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool
  Query::AddToXMLinfolinklist_detectedobject_sparql(XmlDom& dom,
						    const string& s,
						    const string& o) const {
    string sx;
    if (s.find("hut-rectors")!=string::npos)
      sx = "hut-rectors";
    if (o.find("poster")!=string::npos)
      sx = "sab2014-posters";

    auto i = sparql_query_cache.find(o);
    if (i==sparql_query_cache.end()) {
      vector<string> xargs { o };
      list<vector<string> > res
	= GetDataBase()->LinkedDataQuery("sparql", sx,
					 xargs);
      ((Query*)this)->sparql_query_cache[o] = res;
      i = sparql_query_cache.find(o);
    }

    const list<vector<string> >& res = i->second;

    if (sx=="hut-rectors")
      for (auto i=res.begin(); i!=res.end(); i++) {
	XmlDom il = dom.Element("infolink");
	il.Prop("linktext", (*i)[0]);
	il.Prop("text",     (*i)[1]);
	il.Prop("href",     (*i)[2]);
	il.Prop("imgsrc",   (*i)[3]);
      }

    else if (sx=="sab2014-posters") {
      set<string> txt;
      for (auto i=res.begin(); i!=res.end(); i++)
	if (txt.find((*i)[0])==txt.end()) {
	  txt.insert((*i)[0]);
	  XmlDom il = dom.Element("infolink");
	  il.Prop("linktext", (*i)[0]);
	  il.Prop("text",     (*i)[1]);
	  il.Prop("href",     (*i)[2]);
	  il.Prop("imgsrc",   (*i)[2]);
	}
      for (auto i=res.begin(); i!=res.end(); i++)
	if (txt.find((*i)[3])==txt.end()) {
	  txt.insert((*i)[3]);
	  XmlDom il = dom.Element("infolink");
	  il.Prop("linktext", (*i)[3]);
	  il.Prop("text",     (*i)[3]);
	  il.Prop("href",     (*i)[4]);
	  // il.Prop("imgsrc",   (*i)[]);
	}
      for (auto i=res.begin(); i!=res.end(); i++) {
	string n = (*i)[5]+" "+(*i)[6];
	if (txt.find(n)==txt.end()) {
	  txt.insert(n);
	  XmlDom il = dom.Element("infolink");
	  il.Prop("linktext", n);
	  il.Prop("text",     n+"'s Aalto People homepage");
	  il.Prop("href",     (*i)[7]);
	  il.Prop("imgsrc",   (*i)[8]);
	}
      }
    }
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLvisitedlinklist(XmlDom& dom, const string&,
				      const string&) const {
    XmlDom vll = dom.Element("visitedlinklist");
    for (auto i=visited_links.begin(); i!=visited_links.end(); i++) {
      XmlDom link = vll.Element("visitedlink");
      link.Prop("url",          i->url);
      link.Prop("content-type", i->type);
      link.Prop("size",         ToStr(i->size));
      link.Prop("time",         TimeString(i->time));
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLvariables(XmlDom& dom, const Connection *conn) const {
    xmlNodePtr prnt = dom.node;
    xmlNsPtr   ns   = dom.ns;

    //Tic("Query::AddToXMLvariables");

    xmlNodePtr n = AddTag(prnt, ns, "inrun", InRun()?"yes":"no");
    if (InRun()) {
      timespec_t now;
      SetTimeNow(now);
      float s = now.tv_sec-last_modification.tv_sec;
      s += (now.tv_nsec-last_modification.tv_nsec)*1e-9;
      SetProperty(n, "seconds", s);
    }

    if (the_db) {
      bool ok = true;

      ok = ok && the_db->AddToXMLdatabaseinfo(dom, "", conn, true, true);
      ok = ok && the_db->AddToXMLfeaturelist(dom, this);
      ok = ok && the_db->AddToXMLalgorithmlist(dom, this);
      ok = ok && the_db->AddToXMLnamedcmdlines(prnt, ns);

      if (!ok)
        return false;
    }

    if (true)
      AddToXMLclassaugment(dom);

    if (HasViewClass())
      AddToXMLselection(prnt, ns, "viewclass", ViewClass());

    if (HasPositive())
      AddToXMLselection(prnt, ns, "positive", Positive());

    if (HasNegative())
      AddToXMLselection(prnt, ns, "negative", Negative());

    if (upload_label!="")
      AddTag(prnt, ns, "label", upload_label);

    /* commented out temporarily
    xmlNodePtr alg = AddTag(prnt, ns, "algorithm");
    AddTag(alg, ns, "name", PicSOM::CbirAlgorithmP(algorithm));
    for (int s=stage_first; s<=stage_last; s++) {
      cbir_function f = StageFunction((cbir_stage)s, true);
      if ((int)f==-1)
        break;
      if (f!=func_default)
        AddTag(alg, ns, StageNameP((cbir_stage)s), FunctionNameP(f));
    }
    */

    if (_sa_default_convtype!="")
      AddTag(prnt, ns, "convtype", _sa_default_convtype);

    xmlNodePtr var = AddTag(prnt, ns, "variables");
    // AddTag(var, ns, "maxquestions",      maxquestions);  // activate this???
    AddTag(var, ns, "permapobjects", 	  _sa_permapobjects);
    AddTag(var, ns, "imgcols",       	  imgcols);
    AddTag(var, ns, "showstats",     	  showstats);
    AddTag(var, ns, "vqlevel",       	  _sa_vqlevel);
    AddTag(var, ns, "target",        	  TargetTypeString(Target()));
    AddTag(var, ns, "matrixcount",   	  DefaultMatrixCount());
    AddTag(var, ns, "entropymatrix", 	  entropymatrix);
    AddTag(var, ns, "textquery",     	  textquery);
    AddTag(var, ns, "implicit_textquery", implicit_textquery);
    AddTag(var, ns, "textsearch",         textsearch);
    AddTag(var, ns, "textindex",          textindex);
    AddTag(var, ns, "textfield",          textfield);

    // AddTag(var, ns, "calculate_entropy", calculate_entropy);

    AddTag(var, ns, "erfpolicy",     erfpolicy);

    if (IsMapPointSet() && maparea_ul.IsDefined() && maparea_lr.IsDefined()) {
      char tmp[1000];
      sprintf(tmp, "%s[%d](%g,%g)",
              TsSom(pointed_map).Name().c_str(), pointed_level,
              mappoint.X(), mappoint.Y());
      AddTag(prnt, ns, "mappoint", tmp);

      sprintf(tmp, "%s[%d][%d:%d,%d:%d]",
              TsSom(pointed_map).Name().c_str(), pointed_level,
              maparea_ul.X(), maparea_lr.X(), maparea_ul.Y(), maparea_lr.Y());
      sprintf(tmp+strlen(tmp), ":%s", AllowedMapPointMovements());
      AddTag(prnt, ns, "maparea", tmp);
    }
  
    if (parent && parent->Identity()!="")
      AddTag(prnt, ns, "parent_identity", parent->Identity());

    if (keyvaluelist.size()) {
      xmlNodePtr other = AddTag(prnt, ns, "otherkeys");
      for (map<string,string>::const_iterator i=keyvaluelist.begin();
           i!=keyvaluelist.end(); i++) {
        AddTag(other, ns, i->first, i->second);
        if (debug_all_keys)
          WriteLog("Exported variable ", i->first, "=[", i->second, "]");
      }
    } else if (debug_all_keys)
      WriteLog("Exported empty variable list");

    //Tac("Query::AddToXMLvariables");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string> > Query::XsltParam() const {
    list<pair<string,string> > ret;

    string tnw = "120", tnh = "90", videosegm = "bb-25-05";
    size_t p = tnsize.find('x');
    if (p!=string::npos) {
      tnw = tnsize.substr(0, p);
      tnh = tnsize.substr(p+1);
    }
    ret.push_back(make_pair("tnwidth",   tnw));
    ret.push_back(make_pair("tnheight",  tnh));
    ret.push_back(make_pair("tncolumns", ToStr(tncolumns)));
    ret.push_back(make_pair("videosegm", videosegm));

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLclassaugment(XmlDom& dom) const {

    // FIXME MATS specific feature vs all vs "global" ??

    XmlDom xlist = dom.Element("classaugment");
    
    map<string,float> classes;
    for (classaugment_citer_t it=classaugment.begin(); it!=classaugment.end();
         it++)
      classes[it->first.first] = it->second;
    
    for (map<string,float>::iterator it=classes.begin(); it!=classes.end();
         it++) {
      string classname = it->first;
      float val = it->second;

      if (classname=="")
        continue;

      if (val != 0.0) {
        XmlDom cl = xlist.Element("class");
        cl.Prop("name", classname);
        cl.Prop("value", val);
      }        
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLselection(xmlNodePtr prnt, xmlNsPtr ns,
                                const string& t, const string& c) const {
    //
    // this has effect when running analysis=best positive=blue,hats
    //
    if (c.find(',')!=string::npos)
      return true;

    xmlNodePtr zz = AddTag(prnt, ns, t);
    if (the_db) {
      int other = -1;  // obs! should this be 0 ???
      bool expand = true;
      ground_truth gt = the_db->GroundTruthExpression(c, Target(), other,
                                                      expand);
      int p = gt.NumberOfEqual(1), n = gt.NumberOfEqual(-1), s = gt.Length();
      the_db->AddToXMLcontentitem(zz, ns, gt.Label(), p, n, s, "");
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLobjectlist(XmlDom& xml, const Connection *conn,
                                 bool childinfo, bool full) /*const*/ {
    bool ok = true;

    ok = ok && AddToXMLvariables(xml, conn);
    ok = ok && AddToXMLstatistics(xml);
    ok = ok && AddToXMLchildlist(xml, false, false, false, false, conn);
    ok = ok && AddToXMLobjects(xml, conn, childinfo, full);
    ok = ok && AddToXMLguessedkeywords(xml);
    ok = ok && AddToXMLmapvalues(xml);

    if (ok && HasAnalysis())
      ok = GetAnalysis()->AddToXML(xml);
  
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLmapvalues(XmlDom& xml) {
    if (!IsMapPointSet() || !TsSom(pointed_map).ShowMapValues())
      return true;

    TSSOM& t = TsSom(pointed_map);
    t.ReadMapFile();

    IntPoint xy = t.ZoomCenter(pointed_level, mappoint);

    const TreeSOM& ts = PointedMapLevel();
    const FloatVector *vec = ts.Unit(xy);

    XmlDom comps(ts.Root()->XMLDescription());
    comps = comps.Root().FindChild("feature").FindChild("components");
    if (comps&&vec) {
      comps = xml.Element("mapvalues").AddCopy(comps).FindChild("component");
      int idx = 0;

      while (comps && idx<vec->Length()) {
        if (!comps.IsElement()) {
          comps = comps.Next();
          continue;
        }
        
        comps.Prop("value", vec->Get(idx++));

        comps = comps.Next();
      }

      return true;
    }

    stringstream values;
    values << t.Name() << "[" << pointed_level << "](" << xy.X() << ","
           << xy.Y() << ")=[";
    if (vec)
      for (int i=0; i<vec->Length(); i++)
        values << " " << vec->Get(i);
    else // should not happen...
      values << " NULL";
    values << " ]";
    xml.Element("mapvalues", values.str());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLchildlist(XmlDom& dom, bool longish, bool recurs,
                                bool childinfo, bool full,
                                const Connection *conn) const {
    bool ok = true;

    if (Nchildren()) {
      XmlDom clist = dom.Element("childlist");
      for (size_t i=0; i<Nchildren(); i++)
        Child(i)->AddToXMLqueryinfo(clist, longish, recurs, childinfo, full,
                                    conn);
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLstatistics(XmlDom& xml) const {
    // this should probably be somewhere else:
    if (showstats) 
      the_db->NoStatistics(false);
    else
      the_db->NoStatistics(true);
    
    if (the_db->NoStatistics())
      return true;

    // Tic("Query::AddToXMLstatistics");

    char tmp[1000];
    size_t i, sum=0;

    char seentxt[1000];
    sprintf(seentxt, "%d = %d + %d + %d", (int)NseenObjects(),
            NpositiveSeenObjects(), NzeroSeenObjects(), NnegativeSeenObjects());

    // for (sum=i=*tmp=0; i<VQUnitsSize(); i++) {
    //   const VQUnitList& ul = PerMapVQUnit(i);
    //   sprintf(tmp+strlen(tmp), "%s%d", *tmp?" + ":"", ul.Nitems());
    //   sum += ul.Nitems();
    // }
    char consunitstxt[1000];
    sprintf(consunitstxt, "%d = %s", (int)sum, tmp);

    // for (sum=i=*tmp=0; i<VQUnitsSize(); i++) {
    //   const VQUnitList& ul = PerMapVQUnit(i);
    //   int n = 0;
    //   for (int j=0; j<ul.Nitems(); j++)
    //     n += ul[j].Retained();
    //   sprintf(tmp+strlen(tmp), "%s%d", *tmp?" + ":"", n);
    //   sum += n;
    // }
    char selunitstxt[1000];
    sprintf(selunitstxt, "%d = %s", (int)sum, tmp);

    for (sum=i=*tmp=0; i<PerMapObjectsSize(); i++) {
      const ObjectList& ol = PerMapNewObject(i);
      sprintf(tmp+strlen(tmp),"%s%d", *tmp?" + ":"", (int)ol.size());
      sum += ol.size();
    }
    char foundimgtxt[1000];
    sprintf(foundimgtxt, "%d = %s", (int)sum, tmp);
    if (sum!=combinedimage.size())
      ShowError("Query::AddToXMLstatistics() sum!=combinedimage.size()");

    char uniqueimgtxt[1000];
    sprintf(uniqueimgtxt, "%d", (int)uniqueimage.size());

    for (sum=i=0; i<uniqueimage.size(); i++)
      sum += uniqueimage[i].Retained();
    char consimgtxt[1000];
    sprintf(consimgtxt, "%d", (int)sum);
    if (NseenObjects() && uniqueimage.size() && sum!=NnewObjects()) {
      char tmptxt[100];
      sprintf(tmptxt, " (%d!=%d)", (int)sum, (int)NnewObjects());
      ShowError("Query::AddToXMLstatistics() sum!=NnewObjects()", tmptxt);
    }

    char rndimgtxt[1000];
    sprintf(rndimgtxt, "%d", random_image_count);

    for (sum=i=0; i<NnewObjects(); i++)
      sum += newobject[i].Retained();
    char selimgtxt[1000];
    sprintf(selimgtxt, "%d ~ %s", (int)sum, tmp);

    XmlDom stats = xml.Element("statistics");
    stats.Element("round", round); 
    stats.Element("seen",  string(seentxt)); 
    stats.Element("unseen", DataBaseSize()-NseenObjects()); 
    stats.Element("considered_units",  string(consunitstxt)); 
    stats.Element("selected_units",    string(selunitstxt)); 
    stats.Element("found_images",      string(foundimgtxt)); 
    stats.Element("unique_images",     string(uniqueimgtxt)); 
    stats.Element("considered_images", string(consimgtxt)); 
    stats.Element("random_images",     string(rndimgtxt)); 
    stats.Element("selected_images",   string(selimgtxt));

    // Tac("Query::AddToXMLstatistics");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLobjects(XmlDom& dom, const Connection *conn,
                              bool childinfo, bool full) /*const*/ {
    // obs! childinfo and full might be member variables
    // include_childinfo and include_full 

    if (HasAnalysis() && GetAnalysis()->SkipObjectLists())
      return true;

    if (!the_db) {
      return true;
      // e.g. ajax calls can create qeyries witout databases...
      // return ShowError("Query::AddToXMLobjects() database not set");
    }

    bool ok = true;

    XmlDom  obj = dom.Element("objects");

    XmlDom plusptr  = obj.Element("plusobjectlist");
    XmlDom minusptr = obj.Element("minusobjectlist");
    XmlDom zeroptr  = obj.Element("zeroobjectlist");
    XmlDom showptr  = obj.Element("showobjectlist");
    XmlDom qstnptr  = obj.Element("questionobjectlist");
    XmlDom mapptr   = obj.Element("mapobjectlist");
    XmlDom frmrptr  = obj.Element("formerobjectlist");
    XmlDom txtptr   = obj.Element("textsearchresultobjectlist");

    bool minim = include_minimal;
    bool oinfo = include_origins_info;

    for (size_t i=0; i<NseenObjects(); i++) 
      if (PicSOM::TargetTypesIntersect(seenobject[i].TargetType(), Target()) ||
          seenobject[i].SelectType()==select_show) {
        XmlDom imgptr = seenobject[i].Value()>0 ? plusptr :
          seenobject[i].Value()<0 ? minusptr : zeroptr;
    
        ok = ok && the_db->AddToXMLobjectinfo(imgptr, seenobject[i].Label(),
                                              this, minim, childinfo, oinfo,
					      full, NULL, "");
      }

    bool select_trick = false; // for testing borders around selected images...
    for (size_t i=0; i<NnewObjects(); i++)
      if (NewObject(i).Retained()) {
        const Object& ni = NewObject(i);
        pair<XmlDom,XmlDom> info = Connection::XMLdocroot("info", false);
        XmlDom root = info.second;
  
        switch (ni.SelectType()) {
        case select_question:
          if (select_trick) {
            ((Object*)&ni)->AspectRelevance("", 1.0);
            select_trick = false;
          }

          root.Element("score", ni.Value());
          ok = ok && ni.AddToXMLstageinfo(root);
          ok = ok && the_db->AddToXMLobjectinfo(qstnptr, ni.Label(), this,
                                                minim, childinfo, oinfo,
						full, &root, "");
          break;

        case select_map:
          if (ni.LabelP() && strlen(ni.LabelP())) {
            root.Element("mapunit", ni.Extra());
            ok = ok && the_db->AddToXMLobjectinfo(mapptr, ni.Label(), this,
                                                  minim, childinfo, oinfo,
						  full, &root, "");
          }
          break;

        case select_show:
          ok = ok && the_db->AddToXMLobjectinfo(showptr, ni.Label(), this,
                                                minim, childinfo, oinfo,
						full, NULL, "");
          break;
        
        default:
          SimpleAbort();
        }
        
        info.first.DeleteDoc();
      }

    // Add the list of former objects
    set<string> *qt = QueryTreeFormerSelections();
    if (qt != NULL)
      for (set<string>::iterator i=qt->begin(); i!=qt->end(); i++)
        ok = ok && the_db->AddToXMLobjectinfo(frmrptr, *i, this,
                                              minim, childinfo, oinfo,
					      full, NULL, "");

    for (auto i=textsearchresult.begin(); ok&&i!=textsearchresult.end(); i++) {
      pair<XmlDom,XmlDom> info = Connection::XMLdocroot("info", false);
      XmlDom root = info.second;
      root.Element("score",   i->val);
      root.Element("snippet", i->txt);

      the_db->AddToXMLobjectinfo(txtptr, Label(i->idx), this,
				 minim, childinfo, oinfo, full, &root, "");
    }

    ok = ok && the_db->AddToXMLarrivedobjectinfo(obj, this, conn);

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLguessedkeywords(XmlDom& xml) const {
    if (classification_data.empty())
      return true;

    XmlDom set = xml.Element("guessedkeywords");

    for (size_t i=0; i<classification_data.size(); i++) {
      const classification_data_t& d = classification_data[i];
      XmlDom kw = set.Element("keyword");
      kw.Prop("name",  d.classname);
      kw.Prop("value", d.value);
      if (d.extrafloat > 0.0 && d.extrastring != "") {
	XmlDom kw = set.Element("extra");
	kw.Prop("name",  d.extrastring);
	kw.Prop("value", d.extrafloat);
      }
      kw.Prop("type", "guessed");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLmapimage(XmlDom& xml, const string& name,
                               const string& spec) const {
    // Tic("Query::AddToXMLmapimage");

    string istr = MapImageString(name, spec);
    if (istr=="")
      return ShowError("Query::AddToXMLmapimage() : MapImageString(",
                       name+","+spec, ") failed");

    stringstream istrstr;
    istrstr << istr;

    bool persists = true;
    bool ok = Connection::AddToXMLdata(xml, "mapimage",
                                       "image/png", istrstr, persists);
    
    if (!ok)
      ShowError("Query::AddToXMLmapimage(", name, ") failed");

    // Tac("Query::AddToXMLmapimage");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::MapImageString(const string& name, const string& spec) const {
    int l, som = FindTsSom(name, &l), k = 0;
    if (!IsTsSomAndLevelOK(som, l)) {
      ShowError("Query::MapImageString() : FindTsSom(", name, ") failed");
      return "";
    }

    imagedata idata = CreateMapImage(som, l, k, spec);
    string istr = imagefile::stringify(idata, "image/png");

    return istr;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLqueryimage(XmlDom& /*xml*/, const string& /*name*/,
				 const string& /*spec*/) const {
    return ShowError("AddToXMLqueryimage() is not implemented");
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata Query::ErfDetectionsImage(const string& name,
				      const string& /*spec*/) {
 
    erf_image_data& aid = ErfImageData(name);
    const list<map<string,string>>& detections = aid.detections;

    if (detections.size()==0) {
      imagedata img(400, 300);
      vector<float> blue { 0, 0, 1 }, bluewh;
      for (size_t i=0; i<img.width()*img.height(); i++)
	bluewh.insert(bluewh.end(), blue.begin(), blue.end());;
      img.set(bluewh);
      return img;
    }

    map<string,string> f = detections.front();
    int w = atoi(f["width"].c_str());
    int h = atoi(f["height"].c_str());

    if (!w || !h)  {
      imagedata img(400, 300);
      vector<float> green { 0, 1, 0 }, greenwh;
      for (size_t i=0; i<img.width()*img.height(); i++)
	greenwh.insert(greenwh.end(), green.begin(), green.end());;
      img.set(greenwh);
      return img;
    }

    imagedata img(w, h, 3, imagedata::pixeldata_uchar);
    vector<unsigned char> white { 255, 255, 255 }, whitewh;
    for (int i=0; i<w*h; i++)
      whitewh.insert(whitewh.end(), white.begin(), white.end());
    img.set(whitewh);

    vector<int> pg { 0, 0,  w-1, 0,  w-1, h-1,  0, h-1 };
    img.polygon(pg, 1, "black");

    static map<string,imagedata> icache;  // obs!
    static map<string,vector<int> > pcache;  // obs!

    map<string,list<pair<vector<float>,string> > > & word_box
    = GetDataBase()->OdWordBox(name);    

    for (auto i=detections.begin(); i!=detections.end(); i++) {
      map<string,string> m = *i;
      string toc = m["toc"];
      string dir = toc;
      size_t q = dir.find("/");
      if (q!=string::npos)
	dir.erase(q);

      string imagelab = m["image"], imagelaborig = imagelab;
      if (imagelab=="small-16-Pursula.jpg")
	imagelab = "poster.jpeg";

      string imgname = GetDataBase()->ExpandPath("ods")+"/"+dir+"/"+imagelab;
      if (icache.find(imgname)==icache.end()) {
	imagedata idata = imagefile(imgname).frame(0);
	idata.convert(imagedata::pixeldata_uchar);

	string ptsname = imgname;
	q = ptsname.rfind(".");
	if (q!=string::npos) {
	  ptsname.erase(q);
	  ptsname += ".pts";
	  string pgstr = FileToString(ptsname);
	  if (pgstr[0]=='4' && pgstr[1]=='\n') {
	    pgstr.erase(0, 2);
	    for (;;) {
	      q = pgstr.find('\n');
	      if (q==string::npos)
		break;
	      pgstr[q] = ' ';
	    }
	    vector<int> pg;
	    q = pgstr.find_first_not_of(" \t");
	    while (q<pgstr.size()) {
	      pg.push_back(atoi(pgstr.substr(q).c_str()));
	      size_t r = pgstr.find_first_of(" \t", q);
	      if (r==string::npos)
		break;
	      q = pgstr.find_first_not_of(" \t", r);
	    }
	    if (pg.size()==8) {
	      pcache[imgname] = pg;
	      imagedata idatapts = idata;
	      idata.copyAsSubimageInv(idatapts, pg);
	    }
	  }
	}

	icache[imgname] = idata;

	string htmlname = imgname;
	q = htmlname.rfind(".");
	if (q!=string::npos) {
	  htmlname.erase(q);
	  htmlname += ".html";
	  XmlDom xml = XmlDom::ReadDoc(htmlname);
	  XmlDom html = xml.Root();
	  if (html.NodeName()=="html") {
	    XmlDom body = html.FirstElementChild("body");
	    XmlDom doc  = body.FirstElementChild("doc");
	    XmlDom page =  doc.FirstElementChild("page");
	    for (XmlDom word=page.FirstElementChild("word"); word;
		 word=word.NextElement("word")) {
	      string txt = word.FirstChildContent();
	      for (;;) {
		size_t q = txt.find_first_of("\"'()!?.,:;");
		if (q==string::npos)
		  break;
		txt.erase(q, 1);
	      }
	      if (txt!="") {
		float xMin = atof(word.Property("xMin").c_str());
		float xMax = atof(word.Property("xMax").c_str());
		float yMin = atof(word.Property("yMin").c_str());
		float yMax = atof(word.Property("yMax").c_str());
		vector<float> c { xMin, yMin, xMax, yMax };
		word_box[imagelab].push_back(make_pair(c, txt));
	      }
	    }
	  }
	}

      }
      const imagedata& idata = icache[imgname];
      imagedata idatahm = idata;

      float x0f = -1, y0f = -1; 

      erf_image_data& imageid = ErfImageData(imagelaborig);
      for (auto a=imageid.detections.begin();
	   a!=imageid.detections.end(); a++) {
	if (a->find("gazex")!=a->end()) {
	  x0f = atof((*a)["gazex"].c_str());
	  y0f = atof((*a)["gazey"].c_str());

	  if (pcache.find(imgname)!=pcache.end()) {
	    const vector<int>& pol = pcache[imgname];
	    // obs! bad approximation...
	    x0f -= pol[0];
	    y0f -= pol[1];

	    x0f *= float(idata.width()) /(pol[2]-pol[0]); 
	    y0f *= float(idata.height())/(pol[7]-pol[1]); 

	  } else if (imagelab=="poster.jpeg") {
	    x0f *= 2380.0/768;
	    y0f *= 3368.0/1024;
	  }

	  int x0i = (int)floor(x0f+0.5);
	  int y0i = (int)floor(y0f+0.5);

	  idatahm.circle(x0i, y0i, 20, 0, "green");
	}
      }

      string pgstr = m["polygon"];
      vector<int> pg;
      q = pgstr.find_first_not_of(" \t");
      while (q<pgstr.size()) {
	pg.push_back(atoi(pgstr.substr(q).c_str()));
	size_t r = pgstr.find_first_of(" \t", q);
	if (r==string::npos)
	  break;
	q = pgstr.find_first_not_of(" \t", r);
      }
      if (pg.size()) {
	img.polygon(pg, 1, "red");
	img.copyAsSubimage(idatahm, pg);
      }
    }

    erf_image_data& cid = ErfImageData("*collage*");
    if (cid.gaze_sample_center.size()) {
      const uiart::GazeData& gv = *cid.gaze_sample_center.rbegin();
      int x = (int)floor(gv.x+0.5), y = (int)floor(gv.y+0.5);
      if (img.coordinates_ok(x, y)) {
	size_t d = 5;
	img.line(x-d, y,   x+d, y,   3, "red");
	img.line(x,   y-d, x,   y+d, 3, "red");
      }
    }

    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLfeatureinfo(XmlDom& xml, size_t idx) const {
    if (!IndexIndexOK(idx))
      return ShowError("Query::AddToXMLfeatureinfo() : index error");

    const TSSOM& ts = IndexDataTSSOM(idx).TsSom();
    XmlDom tree = xml.Element("feature");

    // obs! HAS_CBIRALGORITHM_NEW problem
    return ts.AddToXMLfeatureinfo_static(tree) &&
      AddToXMLfeatureinfo_dynamic(tree, &IndexData(NULL, idx));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLfeatureinfo_dynamic(XmlDom& xml,
                                          const Index::State *idxst) const {
    bool inc = (IsDefaultQuery() && Nindices()==0) || idxst;
    xml.Element("selected", inc);

    if (idxst && idxst->Is("tssom")) {
      const TSSOM::State& fd = *(TSSOM::State*)idxst;
      const TSSOM *ts = (TSSOM*)fd.StaticPart();

      stringstream ss;
      if (!HasAnalysis()) {
        if (algorithm==cbir_picsom) {
          for (size_t i=0; i<ts->Nlevels(); i++)
            ss << (i?" ":"") << i;
        }
        if (algorithm==cbir_picsom_bottom ||
            algorithm==cbir_picsom_bottom_weighted)
          ss << ts->Nlevels()-1;
      }
      xml.Element("showlevels", ss.str());

      const string fname = fd.fullname!="" ? fd.fullname : ts->Name();
      xml.Element("fullname", fname);

      if (fd.aspects.size())
        xml.Element("aspects", CommaJoin(fd.aspects));

      for (int j=0; j<(CalculateEntropy()?5:1); j++) {
        const char *txt[] = { "weight", "entropy", "entropy_r", "entropy_p",
                              "entropy_n" };
        const vector<float>& e
          = j==0 ? fd.tssom_level_weight : j==1 ? fd.entropy :
          j==2 ? fd.entropy_r : j==3 ? fd.entropy_p : fd.entropy_n;

        stringstream s;
        for (size_t i=0; i<e.size(); i++) {
          if (i)
            s << " ";
          if (e[i]==Index::State::nan())
            s << "na";
          else
            s << e[i];
        }
        xml.Element(txt[j], s.str());
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLobjectinfo_dynamic(XmlDom& xml, int idx) {
    xml.Element("query", Identity());

    bool inseen = true;
    Object *img = FindInSeen(idx);

    if (!img) {
      inseen = false;
      img = FindInNew(idx);
    }

    if (!img)
      return false;

    XmlDom list = xml.Element("aspectlist");

    /*
    float val = 0;
    if (img)
      val = img->Value();
    */

    for (map<string,aspect_data>::iterator i=img->Aspects().begin();
         i!=img->Aspects().end(); i++) {
      XmlDom a = list.Element("aspect");
      a.Prop("name",  i->first);
      a.Prop("value", i->second.value);

      if (i->second.type != aspect_data::aspect_default)
        a.Prop("type", i->second.typeStr());

      if (i->second.type == aspect_data::aspect_coordinates && 
         i->second.params.size() >= 2) {
        a.Prop("x", i->second.params[0]);
        a.Prop("y", i->second.params[1]);
      }

      if (debug_aspects)
        cout << "ASPECTS: idx=" << idx << " in "
             << (inseen?"seen":"new") << " name=[" << i->first
             << "] " << i->second.str() << endl;
    }

    XmlDom erf = xml.Element("erfdata");
    // erf.Element("belowtext", "cat");
      
    if (ErfImageDataExists(img->Label(), true)) {
      const erf_image_data& ed = ErfImageDataByLabel(img->Label());
      erf.Element("keystring",    ed.keystring());
      erf.Element("tagstring",    ed.tagstring());
      erf.Element("speechstring", ed.speechstring(speechrecognizer));
    }

    string sniptxt = FindTextSnippet(idx);

    if (include_snippet && sniptxt=="")
      sniptxt = MakeTextSnippet(idx);

    if (sniptxt=="")
      sniptxt = "-- no text --";

    if (sniptxt.find("<b>")==0) {
      XmlDom ts = xml.Element("textsnippet");
      XmlDom b  = ts.Element("b", sniptxt.substr(3, sniptxt.size()-7));
      b.Ns("", "");
    } else 
      xml.Element("textsnippet", sniptxt);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::FindTextSnippet(size_t idx) const {
    for (auto i=textsearchresult.begin(); i!=textsearchresult.end(); i++)
      if (i->idx==idx)
	return i->txt;

    auto i = textsnippetcache.find(idx);
    if (i!=textsnippetcache.end())
      return i->second;

    if (parent)
      return parent->FindTextSnippet(idx);

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::MakeTextSnippet(size_t idx) {
    string txt;
    const string& tiname = the_db->DefaultTextIndex();
    if (tiname!="") {
       list<pair<string,string>> tidescr = the_db->TextIndexDescription(tiname);
       string textfield;
       for (auto i=tidescr.begin(); i!=tidescr.end(); i++)
	 if (i->first=="textfield")
	   textfield = i->second;
       if (textfield!="") {
	 list<pair<string,string>> doc = the_db->TextIndexRetrieve(idx,
								   textindex);
	 for (auto i=doc.begin(); i!=doc.end(); i++)
	   if (i->first==textfield)
	     txt = i->second;
       }
    }

    if (txt.size()>80) {
      txt.erase(80);
      size_t p = txt.rfind(' ');
      if (p!=string::npos)
	txt.erase(p);
      while (txt.size()) {
	auto c = txt[txt.size()-1];
	if (c=='.' || c==',' || c==' ')
	  txt.erase(txt.size()-1);
	else
	  break;
      }
      txt += "...";
    }

    textsnippetcache[idx] = txt;

    return txt;
  }

  /////////////////////////////////////////////////////////////////////////////

  set<string> *Query::QueryTreeFormerSelections() {
    if (querytreeformerselections==NULL) {
      if (IsRoot())
        querytreeformerselections = new set<string>;
      else {
        if (Root()->querytreeformerselections==NULL)
          Root()->querytreeformerselections = new set<string>;
        querytreeformerselections = Root()->querytreeformerselections;
      }
    
      if (querytreeformerselections==NULL)
        ShowError("Query::QueryTreeFormerSelections() : ",
                  "NULL querytreeformerselections in the root node");
      
    } 
    return querytreeformerselections;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void Query::AddToQueryTreeFormerSelections(int i) {
    set<string> *qt = QueryTreeFormerSelections();
    if(qt==NULL)
      return;
    string lab = LabelP(i);
    if (lab != "")
      qt->insert(lab);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MarkAsSeenEither(int idx, double v, bool set_e, DataBase *db) {
    string msg = "Query::MarkAsSeenEither() : ";

    bool imgexists = IsSeen(idx);  // this also traps invalid idx!

    
    Object *no = FindInNew(idx), *o = NULL;
    int rank  = no ? no->Rank()  : -1;
    int round = no ? no->Round() : -1;

    if (imgexists) {
      // obs! explain here when this can happen...
      o = FindInSeen(idx);
      if (!o)
        return ShowError(msg+"logic failure");

      double old_v = o->Value();
      o->Value(v);

      if (debug_subobjects)
        cout << "ReMarked seen " << old_v << " : " << o->DumpStr() << endl;

    } else {
      size_t old_n = NseenObjects();
      // obs! this doesn't yet handle aspects via AspectsFromIndices();
      o = new Object(db ? db : the_db, idx, select_answer, v);
      o = AddToSeen(o);
      o->Rank(rank);
      o->Round(round);
      if (debug_subobjects)
        cout << "Marked seen " << o->DumpStr() << endl;

      if (NseenObjects()!=old_n+1)
        return ShowError(msg+"NseenImages()!=old_n+1");
    }

    if (set_e && !o->AspectRelevance("", v))
      return ShowError(msg+"Object::AspectRelevance() failed");

    RemoveFromNew(idx);  // we don't care if it didn't exist there...

    if (debug_aspects)
      cout << o->DumpStr() << endl;

    return true;  
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RemoveFromNew(int idx) {
    if (!IsInNewObjects(idx))
      return false;

    for (size_t j=0; j<NnewObjects(); j++)
      if (NewObject(j).Retained() && NewObject(j).Index()==idx) {
        newobject.erase(j);
        return true;
      }

    return ShowError("RemoveFromNew() error?");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MarkAsSeenNoAspects(const ground_truth *p, const ground_truth *n,
                                  DataBase *db) {
    string msg = "Query::MarkAsSeenNoAspects(IntVector*,IntVector*) : ";

    if (p && n && p->Length()!=n->Length())
      return ShowError(msg+"lengths differ");
    if (!p)
      return ShowError(msg+"p==NULL");

    Tic("MarkAsSeenNoAspects");

    for (size_t l = p->size(), i=0; i<l; i++)
      if ((*p)[i]==1)
        MarkAsSeenNoAspects(i, +1.0, db);
      else if ((n&&(*n)[i]==1) || (!n&&(*p)[i]==-1))
        MarkAsSeenNoAspects(i, -1.0, db);
    
    Tac("MarkAsSeenNoAspects");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MarkAsSeenAspects(const aspect_map_t& p, const aspect_map_t& n,
                                DataBase *db) {
    ground_truth posi(DataBaseSize(), -1), nega(DataBaseSize(), -1);

    for (aspect_map_t::const_iterator i=p.begin(); i!=p.end(); i++)
      posi = posi.TernaryOR(i->second.first);
        
    for (aspect_map_t::const_iterator i=n.begin(); i!=n.end(); i++)
      nega = nega.TernaryOR(i->second.first);

    if (!MarkAsSeenNoAspects(&posi, &nega, db))
      return ShowError("MarkAsSeenNoAspects(aspect_map_t,...) : failed 1");

    bool ok = true;
    for (aspect_map_t::const_iterator i=p.begin(); ok && i!=p.end(); i++)
      ok = MarkAspectRelevance(i->first, i->second.first, i->second.second);
        
    for (aspect_map_t::const_iterator i=n.begin(); ok && i!=n.end(); i++)
      ok = MarkAspectRelevance(i->first, i->second.first, -i->second.second);
    
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MarkAsSeenEmptyAspect(const ground_truth *p,
                                    const ground_truth *n,
                                    DataBase *db) {
    if (!MarkAsSeenNoAspects(p, n, db))
      return ShowError("MarkAsSeenNoAspects(...) : failed 1");

    bool ok = true;
    if (p)
      ok = MarkAspectRelevance("", *p, +1.0);
        
    if (n&&ok)
      ok = MarkAspectRelevance("", *n, -1.0);
    
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::RemoveEmptyAspectFromSeen() {
    for (size_t i=0; i<NseenObjects(); i++)
      seenobject[i].Aspects().erase("");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MarkAspectRelevance(const string& a, const ground_truth& gt,
                                  double v) {
    bool ok = true;

    for (size_t i=0; ok && i<gt.size(); i++)
      if (gt[i]==1)
        ok = ProcessAspectInfo(a, i, v);

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RemoveFromSeen(int idx) {
    bool imgexists = false;
    // int imgplace = PlaceInSeen_lab(LabelP(idx), imgexists);
    int imgplace = PlaceInSeen_idx(idx, imgexists);
    if (imgexists && imgplace>=0 && imgplace<(int)seenobject.size()) {
      seenobject.erase(imgplace);
      return true;
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MarkAllNewObjectsAsSeen(char ch, bool a) {
    bool ret = true;

    //   cout << (Stages(stage_initial_set)==func_only_forced_images_user_test)
    //        << " " <<round << endl;

    while (NnewObjects()) {
      Object& im = NewObject(NnewObjects()-1);
      if (!im.Retained()) {
        newobject.erase(newobject.size()-1);
        continue;
      }

      const char *lab = CopyString(im.LabelP());
      // const char *lab = im.Label(); // obs! why is copystring()
      int idx = im.Index();

      // remove transp. new images
      if (!lab || !strlen(lab)) 
        newobject.erase(newobject.size()-1);

      else {
        if (im.SelectType() == select_map)
          ch = '0'; //nonclick map images set '0' not '-'

        if (StageFunc(stage_initial_set)==func_only_forced_images_user_test &&
            (!parent || !parent->NseenObjects())) {
          // cout << "[" << lab << "] is set to zero" << endl;
          ch = '0';
        }

        double val = MarkCharToValue(ch);
        ret = ret && MarkAsSeenNoAspects(idx, val);
        if (a && ret)
          ProcessAspectInfo("", idx, val);
      }

      delete lab;
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MarkAllNewObjectsAsSeenAspect(float val, const string& asp) {
    while (NnewObjects()) {
      Object& obj = NewObject(NnewObjects()-1);
      if (!obj.Retained()) {
        newobject.erase(newobject.size()-1);
        continue;
      }
      int idx = obj.Index();
      MarkAsSeenNoAspects(idx, val);
      ProcessAspectInfo(asp, idx, val);
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ChangeAllSeenObjects(char from, char to, bool a) {
    for (size_t i=0; i<NseenObjects(); i++)
      if (seenobject[i].Matches(from)) {
        seenobject[i].Set(to);
        if (a)
          ProcessAspectInfo("", seenobject[i].Index(), seenobject[i].Value());
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CopyBasicIndexData(const vector<Index::State*>& d) {
    UnselectIndices(NULL, true);

    for (size_t i=0; i<d.size(); i++)
      index_data.push_back(d[i]->MakeCopy(false));

    // _may_ copy too much as it used to copy only index_p, fullname, aspects !

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CreateIndexData(bool keep) {
    // obs! HAS_CBIRALGORITHM_NEW problem
    ReadFiles();

    for (size_t i=0; i<Nfeatures(); i++)
      if (IsWordInverse(i))
        IndexDataWordInverse(i).Initialize();

      else if (IsPreCalculatedIndex(i))
        IndexDataPreCalculated(i).Initialize();
                
      else if (IsTsSom(i))
        IndexDataTSSOM(i).Initialize(keep, DefaultMatrixCount(),
                                     CalculateEntropy());
      else if (IsSvm(i))
        IndexDataSVM(i).Initialize();

      else
        return ShowError("CreateIndexData() failing with ["+
                         IndexStaticData(NULL, i).IndexName()+"] ["+
                         IndexFullName(i)+"]");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::RemoveNongenuineFromSeen() {
    size_t current = 0;
    
    while (current<NseenObjects()) {
      if (seenobject[current].GenuineRelevance())
        current++;
      else
        seenobject.erase(current);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  int Query::PlaceInSeen_idx(size_t idx, bool& exists, int r) const { 
    // Tic("PlaceInSeen");

    int ret = 0, i, l=0, intidx = idx;
    exists = false;
    if (!NseenObjects())
      goto do_tac;

    if (r==MAXINT)
      r = NseenObjects()-1;

    if (!pre2122bug && intidx>seenobject[r].Index()) {
      ret = r+1;
      goto do_tac;
    }

    if (intidx>seenobject[r].Index()) 
      i=l=r;
    else{
      do {
        i = (l+r)/2;
        if (intidx<seenobject[i].Index()) 
          r=i-1;
        else
          l=i+1;
      } while (intidx!=seenobject[i].Index() && l<=r);
    }

    if (intidx==seenobject[i].Index()) {
      exists = true;
      ret = i;
    } else 
      ret = l;

  do_tac:
    // Tac("PlaceInSeen");

    return ret;
  }  

  /////////////////////////////////////////////////////////////////////////////

  int Query::PlaceInSeen_lab(const char* lab, bool& exists, int r) const { 
    if (!lab) {
      ShowError("Query::PlaceInSeen(): lab==NULL");
      return -1;
    }

    // Tic("PlaceInSeen");

    int ret = 0, i, l=0;
    exists = false;
    if (!NseenObjects())
      goto do_tac;

    if (r==MAXINT)
      r = NseenObjects()-1;

    if (!pre2122bug && strcmp(lab,seenobject[r].LabelP())>0) {
      ret = r+1;
      goto do_tac;
    }

    if (strcmp(lab,seenobject[r].LabelP())>=0) 
      i=l=r;
    else{
      do {
        i = (l+r)/2;
        if (strcmp(lab,seenobject[i].LabelP())<0) 
          r=i-1;
        else
          l=i+1;
      } while (strcmp(lab,seenobject[i].LabelP())!=0 && l<=r);
    }

    if (strcmp(lab,seenobject[i].LabelP())==0) {
      exists = true;
      ret = i;
    } else 
      ret = l;

  do_tac:
    // Tac("PlaceInSeen");

    return ret;
  }  

  /////////////////////////////////////////////////////////////////////////////

  int Query::PlaceInSeenReal(const char* lab, bool& exists, int r) const { 
    // Tic("PlaceInSeen");

    if (!lab) {
      ShowError("Query::PlaceInSeenReal(): lab==NULL");
      return -1;
    }

    int ret = 0, i, l=0;
    exists = false;
    if (!seenobjectreal.size())
      goto do_tac;

    if (r==MAXINT)
      r = seenobjectreal.size()-1;

    do {
      i = (l+r)/2;
      if (strcmp(lab,seenobjectreal[i].LabelP())<0) 
        r=i-1;
      else
        l=i+1;
    } while (strcmp(lab,seenobjectreal[i].LabelP())!=0 && l<=r);
  
    if (strcmp(lab,seenobjectreal[i].LabelP())==0) {
      exists = true;
      ret = i;
    } else 
      ret = l;

  do_tac:
    // Tac("PlaceInSeen");

    return ret;
  }  

  /////////////////////////////////////////////////////////////////////////////

  void Query::SortSeen() {
    for (size_t i=1; i<NseenObjects(); i++) {
      bool dummy;
      // int j = PlaceInSeen_lab(seenimage[i].LabelP(), dummy, i-1);
      int j = PlaceInSeen_idx(seenobject[i].Index(), dummy, i-1);
      seenobject.move(i, j);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  Object *Query::AddToSeen(Object *o) {
    bool imgexists = false;
    // int imgplace = PlaceInSeen_lab(o->LabelP(), imgexists);
    int imgplace = PlaceInSeen_idx(o->Index(), imgexists);
    if (imgexists) {
      ShowError("Query::AddToSeen() : object <", o->Label(),"> exists");
      return NULL;
    }

    // the following is now called once in:
    //   - Connection::TryToRunXMLresponse()
    //   - Query::MarkAsSeen(const ground_truth*, ...)
    //   - Analysis::AnalyseSimulateOneRun()
    // ConditionallySetImageIndexVectorLengths();

    // cout << "Adding " <<o->LabelP()<< "w/ value "<<o->Value()
    //      << "to seenimage." << endl;

    seenobject.insert(imgplace, o);  

    return &seenobject[imgplace];
  }

  /////////////////////////////////////////////////////////////////////////////

  Object *Query::AddToSeenReal(Object *o) {
    bool imgexists = false;
    int imgplace = PlaceInSeenReal(o->LabelP(), imgexists);
    if (imgexists)
      return NULL;

    //  return ShowError("Query::AddToSeenReal() : object exists");

    //caller usually does no duplicate checking

    //  ConditionallySetImageIndexVectorLengths();
    //  cout << "Adding " <<o->LabelP()<< "w/ value "<<o->Value()<< "to seenimagereal." << endl;

    seenobjectreal.insert(imgplace, o);

    return &seenobjectreal[imgplace];
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t Query::NSeenObjects(int sign, target_type tt) const {
    if (tt==target_no_target)
      return ShowError("NSeenObjects() : target_type==target_no_target");

    size_t ret = 0;
    for (size_t i=0; i<NseenObjects(); i++)
      if (tt==target_any_target ||
          PicSOM::TargetTypeContains(seenobject[i].TargetType(), tt))
        if ((sign<0  && seenobject[i].Value()<0)  ||
            (sign==0 && seenobject[i].Value()==0) ||
            (sign>0  && seenobject[i].Value()>0))
          ret++;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  int Query::SeenObjectCount(int m, int l) const {
    const TreeSOM& ts = Map(m, l);
    int cnt = 0;
    for (IntPoint p = ts; p; ++p)
      if (IsSeen(ts.Unit(p)->LabelIndex()))
        cnt++;

    return cnt;
  }

  /////////////////////////////////////////////////////////////////////////////

  double Query::Entropy(int m, int l, int k) {
    if (k!=0)
      ShowError("Query::Entropy(): k!=0");

    PlaceSeenOnMap(m, l);

    // ConvolveOneDimMask(m, l, TsSom(m).CreateCVector(0));

    Convolve(m, l, k);
    if (l!=0)
      ShowError("Query::Entropy() maybe something strange, l!=0");

    return Convolved(m, l, k).Entropy();
  }

  /////////////////////////////////////////////////////////////////////////////

  double Query::TotalEntropy(int l, int k) {
    double sum = 0;
    for (size_t i=0; i<Nindices(); i++) 
      if (IsTsSom(i)) {
        sum += Entropy(i, l, k);
        Convolved(i, l, k).Zero(); // ???
      }
    return sum;
  }

  /////////////////////////////////////////////////////////////////////////////

  int Query::MinimumEntropyMap(int l, int k) {
    double min = MAXFLOAT;
    int r = -1;
    for (size_t i=0; i<Nindices(); i++) {
      double e = Entropy(i, l, k);
      if (e<min) {
        min = e;
        r = i;
      }
      Convolved(i, l, k).Zero(); // ???
    }
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  int Query::LeastSeenMap(int l) const {
    int min = MAXINT, r = -1;
    for (size_t i=0; i<Nindices(); i++) {
      int cnt = SeenObjectCount(i, l);
      if (cnt<min) {
        min = cnt;
        r = i;
      }
    }
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  Object *Query::MakeNewObject(int idx, bool quest) {
    // obs! this doesn't yet handle aspects via AspectsFromIndices();
    return new Object(the_db, idx, quest?select_question:select_answer, 1);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::TopLevelsByEntropy() {
    Tic("TopLevelsByEntropy");
    // DeleteNew();
    for (; NnewObjects()<MaxQuestions(); ) {
      seenobject = newobject;  // obs! ugly, ugly, ugly trick...
      SortSeen();

      int me = -1;
      IntPoint q;

      IntVector rndperm = RandVar::Permutation(Nindices(), RndSeed());

      //    if (true) {
      float maxentropy = 0;
      for (size_t mm=0; mm<Nindices(); mm++) {
	int m = rndperm[mm];

        if (!IsTsSom(m))
          continue;

        // if (!TsSom(m).TypeIsNormal())
        //   continue;

        TsSom(m).ReadMapFile(false, true);
        const TreeSOM& ts = Map(m, 0); // was EvenAlien
        for (IntPoint p = ts; p; ++p) {
          int idx = ts.Unit(p)->LabelIndex();
          if (idx<0)
            continue;

          // was: IsSeen() || !OkWithRestriction()
          if (!CanBeShownRestricted(idx, false))
            continue;

          // this could be used if we could later find and use the 
          // correct target!  see also DataBase::AddToXMLthumbnail()!

	  // the_db->ObjectDump(idx);
          
	  // int idxlinked = -1;
          if (!the_db->ObjectHasLinkToTargetType(idx, Target()))
	    continue;

          // if (!(the_db->ObjectsTargetType(idx)&Target())) {
	  //   // idx = idxlinked;
	  //   if (idx==-1)
	  //     return ShowError("Query::TopLevelsByEntropy() : link failure");
	  // }

          //
          // instead, we just keep on searching...
          //

          // cout << TargetTypeString(the_db->ObjectsTargetType(idx))
          //      << "/" << TargetTypeString(query_target) << endl;

          // if (!(the_db->ObjectsTargetType(idx)&Target()))
          //   continue;

          if (IsSeen(idx)) {
            TrapMeHere();
            return !CanBeShownRestricted(idx, false);
          }

          MarkAsSeenNoAspects(idx, +1.0);

          float totentropy = TotalEntropy(0, 0);
          if (totentropy>maxentropy) {
            maxentropy = totentropy;
            me = m;
            q = p;
          }
          
          if (!RemoveFromSeen(idx))
            return ShowError("Query::TopLevelsByEntropy() : logic failure");
        }
      }

      // cout << " >>> me=" << me << " q=" << q << endl;
      if (!q.IsDefined()) {
	// cout << "BREAKING" << endl;
        break;
      }

      const TreeSOM& ts = MapEvenAlien(me, 0);
      int idx = ts.Unit(q)->LabelIndex();
      // obs! this doesn't yet handle aspects via AspectsFromIndices();
      AppendNewObject(MakeNewObject(idx, true));
    }

    DeleteSeen();
    for (size_t k=0; k<NnewObjects(); k++)
      newobject[k].Value(0);

    Tac("TopLevelsByEntropy");

    return NnewObjects();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::OnlyForcedObjects(const string& arg) {
    if (arg.empty())
      return ShowError("Query::OnlyForcedImages() : empty argument");

    bool expand = false;
    ground_truth sel = GetDataBase()->GroundTruthExpression(arg, Target(),
                                                            -1, expand);
    vector<size_t> idx = sel.indices(1);
    // obs! this should handle also ternary gt case with negative objects!
    // see also Analysis::AnalyseSimulatePerClass()

    if (!idx.size())
      return ShowError("Query::OnlyForcedImages() : set [", arg, "] is empty");

    for (size_t i=0; i<idx.size(); i++) {
      // obs! this doesn't yet handle aspects via AspectsFromIndices();
      Object *img = new Object(the_db, idx[i], select_question, 1);
      AppendNewObject(img);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RandomUnseenObjects(bool deletenewobjects,
                                  bool justonepositive) {
    if (!DataBaseSize())
      return ShowError("Query::RandomUnseenImages() : DataBaseSize()==0");

    Tic("RandomUnseenImages");
    if (deletenewobjects) {
      DeleteNew();
      random_image_count = 0;
    }

    int initialsize = NnewObjects();
    target_type tt = target_any_target; // obs! added when type was required...
    int availablei = DataBaseRestrictionSize(tt)-NseenObjects();
    size_t available = availablei>0 ? availablei : 0;
    size_t lll;

    if (justonepositive)
      lll = 1;
    else 
      lll = MaxQuestions()<available ? MaxQuestions() : available;

    map<string,aspect_data> am = AspectsFromIndices();
    map<string,aspect_data> *ap = &am; // NULL;

    IntVector idx;
    idx.SetIndices(DataBaseSize());

    RandVar rnd(RndSeed());
    int r = -1;
    while (NnewObjects()<lll && idx.Length()) {
      int s = rnd.RandomInt(idx.Length());
      r = idx[s];
      int p = idx.Pop();
      if (s!=idx.Length())
        idx[s] = p;

      if (!LabelP(r) || !*LabelP(r)) // obs! can this really ever be true ???
        return ShowError("No label in Query::RandomUnseenImages()");

      // was: IsSeen() || !OkWithRestriction()
      if (!CanBeShownRestricted(r, false))
        continue;

      // this could be used if we could later find and use the correct target!
      // see also DataBase::AddToXMLthumbnail()!
      //
      // if (!the_db->ObjectHasLinkToTargetType(r, query_target))
      //   continue;
      //
      // instead, we just keep on searching...
      //
      if (!(the_db->ObjectsTargetType(r)&Target()))
        continue;

      bool found = false;
      for (int i=0; !found && i<initialsize; i++)
        if (Simple::StringsMatch(LabelP(r), NewObject(i).LabelP()))
          found = true;
      if (found)
        continue;

      Object *img = new Object(the_db, r, select_question, 1, "", -1,NULL, ap);

      AppendNewObject(img);

      random_image_count++;
    }

    if (justonepositive)
      MarkAsSeenNoAspects(r, +1.0);

    Tac("RandomUnseenImages");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SelectFromPre() {
    bool debug = true;

    string msg = "Query::SelectFromPre(): ";
    if (!DataBaseSize())
      return ShowError(msg, "DataBaseSize()==0");

    // int pre_i = -1;
    // for (size_t i=0; i<Nindices() && pre_i<0; i++)
    //   if (IsPreCalculatedIndex(i))
    //     pre_i = i;

    // if (pre_i<0)
    //   return ShowError(msg, "No pre-calculated index found.");

    // Index* i_pt = IndexDataPreCalculated(pre_i).StaticPart();
    // PreCalculatedIndex* pre = dynamic_cast<PreCalculatedIndex*>(i_pt);

    if (initialset_pre.empty())
      return ShowError(msg, "No initialset_pre specified.");

    DataBase* db = CheckDB();

    PreCalculatedIndex pre(db, initialset_pre, initialset_pre,
			   db->ExpandPath("features"), "", NULL);

    // if (!pre)
    //   return ShowError(msg, "Failed to get pre calculated index data.");

    map<string, double>& p = pre.precalc;
    multimap<double, string> mp;

    for (map<string,double>::const_iterator it=p.begin(); it!=p.end(); it++)
      mp.insert(make_pair(it->second, it->first));

    size_t i=0;
    for (multimap<double, string>::reverse_iterator it = mp.rbegin(); 
         i<MaxQuestions() && it!=mp.rend();  it++) {
      const string& label = it->second;

      int r = LabelIndex(label);

      Object *obj = new Object(db, r, select_question, 1);

      // Add object itself if not restricted
      if (CanBeShownRestricted(r, false) && 
          (db->ObjectsTargetType(r) & Target())) {
        AppendNewObject(obj);
        if (debug)
          cout << "SelectFromPre: " << obj->Label() << endl;
      }

      // Add children it is has such ...
      if (obj->hasChildren()) {
        const vector<int>& chldrn = obj->Children();
        size_t num_chldrn = chldrn.size();
        for (size_t j=0; j<num_chldrn; j++) {
          int cidx = chldrn[j];
          if (CanBeShownRestricted(cidx, false) && 
              (db->ObjectsTargetType(cidx) & Target())) {
            Object *cobj = new Object(db, cidx, select_question, 1);
            AppendNewObject(cobj);
            if (debug)
              cout << "SelectFromPre:  \\_ " << cobj->Label() << endl;
          }
        }
      }

      i++;
    }

    if (i)
      WriteLog(msg, "Added "+ToStr(i)+" objects from "+pre.Name());

    ConvergeDown("");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AllUnseenObjects(bool deletenewobjects) {
    Tic("AllUnseenImages");
    if (deletenewobjects)
      DeleteNew();

    IntVector idx;
    idx.SetIndices(DataBaseSize());

    for (int i=0; i<idx.Length(); i++) {
      int r = idx[i];

      if (!LabelP(r) || !*LabelP(r)) // obs! can this really ever be true ???
        return ShowError("No label in Query::AllUnseenImages()");

      // was: IsSeen() || !OkWithRestriction()
      if (!CanBeShownRestricted(r, true))
        continue;

      // obs! this doesn't yet handle aspects via AspectsFromIndices();
      Object *img = new Object(the_db, r, select_question, 1);
      AppendNewObject(img);
    }

    Tac("AllUnseenImages");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::FindMissingIndices() {
    for (size_t i=0; i<NseenObjects(); i++)
      seenobject[i].Index();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PlaceSeenOnMap(int ii, int jj, bool norm, bool limit) {
    // 
    // THIS IS REALLY REALLY REALLY BAD DESIGN !!!
    // THERE SHOULD BE A STAGE FOR THIS
    // THE OPERATIONS SHOULD DEPEND ON STAGE FUNCTIONS NOT ON ALGORITHMS !!!!!!
    //

    if (!IsTsSomAndLevelOK(ii, jj))
      return ShowError("Query::PlaceSeenOnMap(): not TSSOM or invalid level");

    TsSom(ii).ReadDivisionFile();

    Tic("PlaceSeenOnMap");

    bool ret = false;
    switch (algorithm) {
    case cbir_picsom:
    case cbir_picsom_bottom:
    case cbir_picsom_bottom_weighted:
      ret = PlaceSeenOnMapPicSOM(ii, jj, norm, limit);
      break;

    case cbir_vq:
    case cbir_vqw:
      ret = PlaceSeenOnMapVQ(ii, jj);
      break;
    
    default:
      ShowError("Query::PlaceSeenOnMap(): Unsupported algorithm.");
      ret = false;
    }                                       
  
    Tac("PlaceSeenOnMap");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PlaceSeenOnMapPicSOM(int ii, int jj,
                                   bool norm, bool limit) {
    if (GetMatrixCount(ii)==1)
      return PlaceSeenOnMapPicSOMoneMap(ii, jj, norm);

    if (GetMatrixCount(ii)==3)
      return PlaceSeenOnMapPicSOMthreeMaps(ii, jj, norm, limit);

    //  if (GetMatrixCount(ii)==4)
    //  return PlaceSeenOnMapPicSOMfourMaps(ii, jj, norm);

    return ShowError("Query::PlaceSeenOnMapPicSOM(): matrixcount!=1|3");
  }
    
  /////////////////////////////////////////////////////////////////////////////

  vector<float> Query::DivMultiplierVector(int m, int l, int ind) {
    vector<float> v;
    stringstream ss;
    ss <<  "Query::DivMultiplierVector(" << m << ", " << l << ", " << ind
       << ") : ";
    string msg = ss.str();
    
    const TSSOM& ts = TsSom(m);
    int dd = ts.DivisionDepth(l);
    
    if (ind != -1) { // we are checking for specific object
      if (!DivMulPerObject(m))
        ShowError(msg+"Checking for specific object even though no weights "
                      "are given in div-file!");
      else 
        v = ts.DivMulWeights(l,ind);
    } else {
      if (DivMulPerObject(m))
        ShowError(msg+"We are not checking for specific object even though"
                      "weights are given in div-file!");
      
      for (int i=0; i<dd; i++)
        v.push_back(1.0);
    }

    if ((int)v.size() != dd)
      ShowError(msg+ToStr(v.size())+" != "+ToStr(dd)+"!");
    else {
      string ct = BmuConvType(m);
      const FloatVector* fv = ConvolutionVector(ct,l);
      int fvlen = fv->Length();
      if (debug_placeseen)
        cout << msg << "bmuconv(" << ct << ") = [";

      for (int i=0; i<dd; i++) {
        float ff = i<fvlen ? fv->Value(i) : 0.0;
        v[i] *= ff;
        if (debug_placeseen) cout << " " << ff;
      }
      if (debug_placeseen) cout << " ]" << endl;
    }
    return v;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  const vector<float>& Query::DivHistoryVector(int /*m*/, int /*l*/) {
    static vector<float> v1, v5;
    if (!v1.size())
      v1.push_back(1.0);

    if (!v5.size()) {
      v5.push_back(1.0);
      v5.push_back(0.8);
      v5.push_back(0.6);
      v5.push_back(0.4);
      v5.push_back(0.2);
    }
  
    return v5;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  bool Query::PlaceSeenOnMapPicSOMoneMap(int ii, int jj, bool norm) {
    bool use_aspect_weights = true;

    string msg = "Query::PlaceSeenOnMapPicSOMoneMap() : ";

    if (GetMatrixCount(ii)!=1)
      return ShowError(msg+"matrixcount!=1");

    if (debug_placeseen)
      cout << "PlaceSeenOnMapPicSOMoneMap(" << ii << "," << jj << "," << norm
           << ") : " << MapName(false, ii, jj)
           << " aspects=[" << IndexAspects(NULL, ii) << "]";

    const TreeSOM& ts = MapEvenAlien(ii, jj);
    simple::FloatMatrix&  hts = Hits(ii, jj, 0);
    SetSizeOrZero(hts, ts);

    bool si=store_selfinfluence && Map(ii, jj).IsBottomLevel();

    double positives = 0, negatives = 0, pos_sum = 0, neg_sum = 0;         
    double res_sum_neg = 0, res_sum_pos = 0;
    if (norm)
      SumPositivesAndNegatives(Division(ii, jj), IndexData(NULL, ii),
                               positives, negatives, true);
    else
      positives = negatives = 1;

    if (debug_placeseen)
      cout << " total=" << NseenObjects() << " positives=" << positives
           << " negatives=" << negatives << endl;

    vector<float> divmul;
    if (!DivMulPerObject(ii))
      divmul = DivMultiplierVector(ii, jj);
      
    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& obj = seenobject[i];
      
      int idx = obj.Index();
      if (idx==-1) {
        ShowError(msg+"index not solved for ", obj.Label());
        continue;
      }
    
      double pos = 0.0, neg = 0.0;
      bool match = AspectsMatch(IndexData(NULL, ii), obj, pos, neg);
      double objval = obj.Value();
      double sumval = pos+neg;

      if (debug_placeseen)
        cout << "  i=" << i << " " << the_db->ObjectDump(idx)
             << " aspects=[" << obj.AspectNamesValues()
             << "] match=" << match << " objval=" << objval
             << " pos=" << pos << " neg=" << neg
             << " sumval=" << sumval << endl;

      if (!match)
        continue;

      if (DivMulPerObject(ii))
        divmul = DivMultiplierVector(ii, jj, idx);

      for (size_t d=0; d<divmul.size(); d++) {
        const IntVector& divvec = Division(ii, jj, d);
        double mul = divmul[d];
        int mapidx = divvec.IndexOK(idx) ? divvec[idx] : -1;

        if (debug_placeseen)
          cout << "    d=" << d << " mul=" << mul << " mapidx=" << mapidx;

        if (mapidx!=-1) {
          IntPoint p = ts.ToPoint(mapidx);

          if (debug_placeseen)
            cout << " (" << p.Y() << "," << p.X() << ")";

          double val = 0.0;

          if (pos>0) {
            if (!positives)
              return ShowError(msg+"division by zero: positives==0");

            double pval = pos/positives;
            if (use_aspect_weights)
              pval *= pos;

            val += pval;
            if (!d)
              pos_sum += pos;
            res_sum_pos += pval;
          }

          if (neg<0) {
            if (!negatives)
              return ShowError(msg+"division by zero: negatives==0");
            
            double nval = neg/negatives*negative_weight;
            if (use_aspect_weights)
              nval *= -neg;

            val += nval;
            if (!d)
              neg_sum -= neg;
            res_sum_neg += nval;
          }

          double res = mul*val;
          hts.Add(p.Y(), p.X(), res);

          if (debug_placeseen)
            cout << " res=" << res << " => " << hts.Get(p.Y(), p.X());

          if(si){
            if(d>0) continue; // currently only the first influence is taken
                              // into account

            if(selfinfluence.count(idx)==0)
              selfinfluence[idx]=vector<float>(Nindices(),0.0);
            selfinfluence[idx][ii]=+res;
          }
        }

        if (debug_placeseen)
          cout << endl;
      }
    }

    if (norm && (positives!=pos_sum || negatives!=neg_sum)) {
      stringstream ss;
      ss << "positives="  << positives << " pos_sum=" << pos_sum
         << " negatives=" << negatives << " neg_sum=" << neg_sum;
      return ShowError(msg+"check sum pos_sum or neg_sum mismatches: ",
                       ss.str());
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PlaceSeenOnMapPicSOMoneMapSmoothedFraction(int ii, int jj) {
    string msg = "Query::PlaceSeenOnMapPicSOMoneMapSmoothedFraction() : ";
    if (GetMatrixCount(ii)<1)
      return ShowError(msg+"matrixcount<1");

    cout << "entering Query::PlaceSeenOnMapPicSOMoneMapSmoothedFraction()"  
         << endl;

    if (UsingAspects())
      return ShowError(msg+"incompatible with aspects");

    const TreeSOM& ts = MapEvenAlien(ii, jj);
    simple::FloatMatrix&  hts = Hits(ii, jj, 0);
    SetSizeOrZero(hts, ts);

    const int r = hts.Rows(), c = hts.Columns();
    float *tmppos,*tmpneg;

    tmppos=new float[r*c];
    tmpneg=new float[r*c];

    for(int i=0;i<r*c;i++)
      tmppos[i]=tmpneg[i]=0;

    vector<float> divmul;
    if (!DivMulPerObject(ii))
      divmul = DivMultiplierVector(ii, jj);
    for (size_t i=0; i<seenobject.size(); i++) {
      int idx = seenobject[i].Index();
      if (idx==-1) {
        ShowError("Query::PlaceSeenOnMapPicSOMoneMap() index not solved for ",
                  seenobject[i].Label());
        continue;
      }
      
      if (DivMulPerObject(ii))
        divmul = DivMultiplierVector(ii, jj, idx);
      for (size_t d=0; d<divmul.size(); d++) {
        const IntVector& divvec = Division(ii, jj, d);
        double mul = divmul[d];
        int mapidx = divvec.IndexOK(idx) ? divvec[idx] : -1;

        if (mapidx!=-1) {
          IntPoint p = ts.ToPoint(mapidx);
          double val = seenobject[i].Value();            
          if(val>0)
            tmppos[p.X()+c*p.Y()] += mul;
          else
            tmpneg[p.X()+c*p.Y()] += mul;
        }
      }
    }
    
    bool si=store_selfinfluence && Map(ii, jj).IsBottomLevel();

    double bias=0.1;

    for(int x=0;x<c;x++) for(int y=0;y<r;y++){
      int idx=x+y*c;
      float val=(tmppos[idx]+bias)/(tmppos[idx]+tmpneg[idx]+2*bias);
      hts.FastSet(y,x,val);
      if(si){
        if(selfinfluence.count(idx)==0)
          selfinfluence[idx]=vector<float>(Nindices(),0.0);
        selfinfluence[idx][ii]=val;
      }
    }
     return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PlaceSeenOnMapPicSOMthreeMaps(int ii, int jj, bool norm,
                                            bool limit) {
    bool use_aspect_weights = true;

    string msg = "Query::PlaceSeenOnMapPicSOMthreeMaps() : ";

    if (GetMatrixCount(ii)!=3)
      return ShowError(msg+"matrixcount!=3");

    if (debug_placeseen)
      cout << "PlaceSeenOnMapPicSOMthreeMaps(" << ii << "," << jj << ","
           << norm << "," << limit << ") : " << MapName(false, ii, jj) 
           << " aspects=[" << IndexAspects(NULL, ii) << "]";

    if (debug_aspects)
      cout << endl;

    const TreeSOM& ts = Map(ii, jj);
    simple::FloatMatrix&  hts_pos = Hits(ii, jj, 1);
    simple::FloatMatrix&  hts_neg = Hits(ii, jj, 2);
    SetSizeOrZero(hts_pos, ts);
    SetSizeOrZero(hts_neg, ts);

    bool si=store_selfinfluence && Map(ii, jj).IsBottomLevel();

    const IntVector& div = Division(ii, jj);

    double positives = 0, negatives = 0, pos_sum = 0, neg_sum = 0;
    double res_sum_neg = 0, res_sum_pos = 0;
    if (norm)
      SumPositivesAndNegatives(div, IndexData(NULL, ii),
			       positives, negatives, true);
    else
      positives = negatives = 1;

    if (debug_placeseen)
      cout << " total=" << NseenObjects() << " positives=" << positives
           << " negatives=" << negatives << endl;

    if (debug_selective)
      cout << "Normalising w/ positives=" << positives
           << " negatives=" << negatives << endl;

    vector<float> divmul;
    if (!DivMulPerObject(ii))
      divmul = DivMultiplierVector(ii, jj);
      
    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& obj = seenobject[i];

      int idx = obj.Index();
      if (idx==-1) {
        ShowError(msg+"index not solved for ", obj.Label());
        continue;
      }

      double pos = 0.0, neg = 0.0;
      bool match  = AspectsMatch(IndexData(NULL, ii), obj, pos, neg);
      double objval = obj.Value();
      double sumval = pos+neg;
   
      if (debug_placeseen)
        cout << "  i=" << i << " " << the_db->ObjectDump(idx)
             << " aspects=[" << obj.AspectNamesValues()
             << "] match=" << match << " objval=" << objval
             << " pos=" << pos << " neg=" << neg
             << " sumval=" << sumval << endl;

      if (!match)
        continue;

      if (DivMulPerObject(ii))
        divmul = DivMultiplierVector(ii, jj, idx);

      for (size_t d=0; d<divmul.size(); d++) {
        const IntVector& divvec = Division(ii, jj, d);
        double mul = divmul[d];
        int mapidx = divvec.IndexOK(idx) ? divvec[idx] : -1;

        if (debug_placeseen)
          cout << "    d=" << d << " mul=" << mul << " mapidx=" << mapidx;

        double res = 0.0;

        if (mapidx!=-1) {
          IntPoint p = ts.ToPoint(mapidx);

          if (debug_placeseen)
            cout << " (" << p.Y() << "," << p.X() << ")";

          if (pos>0) {
            if (!positives)
              return ShowError(msg+"division by zero: positives==0");
           
            double pval = mul*pos/positives;
            if (use_aspect_weights) // this is strange, looks like square
              pval *= pos;

            hts_pos.Add(p.Y(), p.X(), pval);
            if (!d)
              pos_sum += pos;
            res_sum_pos += pval;
            res += pval;
            
            if (debug_placeseen)
              cout << " pval=" << pval << " => " << hts_pos.Get(p.Y(), p.X());
          }

          if (neg<0) {
            if (!negatives)
              return ShowError(msg+"division by zero: negatives==0");

            double nval = mul*neg/negatives*negative_weight;
            if (use_aspect_weights) // this is strange, looks like square
              nval *= -neg;

            hts_neg.Add(p.Y(), p.X(), nval);
            if (!d)
              neg_sum -= neg;
            res_sum_neg += nval;
            res += nval;

            if (debug_placeseen)
              cout << " nval=" << nval << " => " << hts_neg.Get(p.Y(), p.X());;
          }

          if(si){
            if(d>0) continue; // currently only the first influence is taken
                              // into account

            if(selfinfluence.count(idx)==0)
              selfinfluence[idx]=vector<float>(Nindices(),0.0);
            selfinfluence[idx][ii]=+res;
          }

        }
        if (debug_placeseen)
          cout << endl;
      }
    }

    if (debug_placeseen)
      cout << "PlaceSeenOnMapPicSOMthreeMaps(" << ii << "," << jj
           << ",...) ending: res_sum_pos=" << res_sum_pos
           << " res_sum_neg=" << res_sum_neg << endl;

    if (norm && (positives!=pos_sum || negatives!=neg_sum)) {
      stringstream ss;
      ss << "positives="  << positives << " pos_sum=" << pos_sum
         << " negatives=" << negatives << " neg_sum=" << neg_sum;
      return ShowError(msg+"check sum pos_sum or neg_sum mismatches: ",
                       ss.str());
    }
  
    if (limit) {
      cout << "PlaceSeenOnMapPicSOMthreeMaps() limiting relevance" << endl;

      int r=hts_pos.Rows();
      int c=hts_pos.Columns();

      double discount_pos=0.0, discount_neg=0.0;

      for(int x=0;x<c;x++) for(int y=0;y<r;y++)
        {
          double before;
        
          before=hts_pos(y,x);
          if(before && before > 1.0/positives){
            double newval= (2-2*pow(0.5,before*positives))/positives;
            discount_pos += before-newval;
            hts_pos.Set(y,x,newval);
          }

          before=hts_neg(y,x);
          if(before && before > 1.0/negatives){
            double newval= (2-2*pow(0.5,before*negatives))/negatives;
            discount_neg += before-newval;
            hts_neg.Set(y,x,newval);
          }

        }

      // here we could have renormalisation based on 
      // discount_[pos,neg]
    }

    if (hitstype==smoothed_log) {
      int r=hts_pos.Rows();
      int c=hts_pos.Columns();

      double bias=0.05;

      for(int x=0;x<c;x++) for(int y=0;y<r;y++)
        {
          double before;
        
          before=hts_pos(y,x)*positives+bias;
          hts_pos.Set(y,x,log(before));

          before=-hts_neg(y,x)*negatives+bias;
          hts_neg.Set(y,x,-log(before));
        }
    }
    else if (hitstype==posneg_lc) {
      int r=hts_pos.Rows();
      int c=hts_pos.Columns();

      for(int x=0;x<c;x++) for(int y=0;y<r;y++)
        {
          double before;
          float coeff=hit_comb_factor;
     
          before=hts_pos(y,x)*positives*coeff;
          hts_pos.Set(y,x,before);

          before=-hts_neg(y,x)*negatives;
          hts_neg.Set(y,x,-before);
        }
    }

    if(hitstype==smoothed_fraction)
      return PlaceSeenOnMapPicSOMoneMapSmoothedFraction(ii,jj);
    else{
      // cout << "Summing hit matrices" << endl;
      Hits(ii, jj, 0) = hts_pos;
      Hits(ii, jj, 0)+= hts_neg;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PlaceSeenOnMapPicSOMfourMaps(int ii, int jj, bool norm) {
    string msg = "Query::PlaceSeenOnMapPicSOMfourMaps() : ";

    if (GetMatrixCount(ii)!=4)
      return ShowError(msg+"matrixcount!=4");

    if (debug_placeseen)
      cout << "PlaceSeenOnMapPicSOMfourMaps(" << ii << "," << jj << "," << norm
           << ") : " << MapName(false, ii, jj) 
           << " aspects=[" << CommaJoin(IndexData(NULL, ii).aspects)
           << "]" << endl;

    if (UsingAspects())
      return ShowError(msg+"incompatible with aspects");

    const TreeSOM& ts = Map(ii, jj);
    simple::FloatMatrix&  hts_pos = Hits(ii, jj, 1);
    simple::FloatMatrix&  hts_neg = Hits(ii, jj, 2);
    simple::FloatMatrix&  hts_real = Hits(ii, jj, 3);
    SetSizeOrZero(hts_pos, ts);
    SetSizeOrZero(hts_neg, ts);
    SetSizeOrZero(hts_real, ts);

    double positives, negatives;         
    if (norm)
      SumPositivesAndNegatives(Division(ii, jj), IndexData(NULL, ii),
                               positives, negatives, true);
    else
      positives=negatives=1;

    for (size_t i=0; i<NseenObjects(); i++) {
      int idx = seenobject[i].Index();
    
      if (idx==-1) {
        ShowError("Query::PlaceSeenOnMapPicSOMfourMaps() index not solved for ",
                  seenobject[i].Label());
        continue;
      }
    
      int mapidx = Division(ii, jj)[idx];
      if (mapidx!=-1) {
        IntPoint p = ts.ToPoint(mapidx);
        double val = seenobject[i].Value();
        if (val>0)
          hts_pos.Add(p.Y(), p.X(), val/positives);
        else
          hts_neg.Add(p.Y(), p.X(), val/negatives);
      }
    }

    int ctr=0;
    for (size_t i=0; i<seenobjectreal.size(); i++) {
      int idx = seenobjectreal[i].Index();
    
      if (idx==-1) {
        ShowError("Query::PlaceSeenOnMapPicSOMfourMaps() index not solved for ",
                  seenobjectreal[i].Label());
        continue;
      }
    
      int mapidx = Division(ii, jj)[idx];
      if (mapidx!=-1) {
        IntPoint p = ts.ToPoint(mapidx);
        double val = seenobjectreal[i].Value();
        if (val>0){
          hts_real.Add(p.Y(), p.X(), val);
          ctr++;
        }
      }
    }

    cout << "Marked " << ctr
         << "positive images to fourth map, positives (in seen) ="
         << positives << endl;

    Hits(ii, jj, 0) = hts_pos;
    Hits(ii, jj, 0)+= hts_neg;

    return true;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  bool Query::CountPositivesAndNegatives(const IntVector& div,
                                         int& pos, int& neg,
                                         bool nonzero,
                                         bool divcheck) const {
    pos = neg = 0;

    bool ok = true;
    for (size_t i=0; i<NseenObjects(); i++)
      if (divcheck==false ||
          (div.IndexOK(seenobject[i].Index()) && div[seenobject[i].Index()]!=-1)) {
        float val = seenobject[i].Value();

        if (val!=-1 && val!=1 && val!=0)
          ok = ShowError("Query::CountPositivesAndNegatives() : val!=+/-1/0");
        if (val>0)
          pos++;    
        else if (val<0)
          neg++;                   
      }

    if (nonzero) {
      if (pos == 0) pos = 1;    
      if (neg == 0) neg = 1;    
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SumPositivesAndNegatives(const IntVector& div,
                                       const Index::State& fd,
                                       double& pos, double& neg,
                                       bool divcheck) const {

    bool use_val = false;  // used to be like true until 14.8.2006

    pos = neg = 0;
    
    bool ok = true;
    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& obj = seenobject[i];
      int idx = obj.Index();
      if (divcheck==false || (div.IndexOK(idx) && div[idx]!=-1)) {
        double val = obj.Value(), mul = 0.0, pp = 0.0, nn = 0.0;
        if (!AspectsMatch(fd, obj, pp, nn))
          continue;

        if (use_val) {
          if (val>0)
            pos += val*mul;
          else if (val<0)
            neg -= val*mul;
        } else {
          pos += pp;
          neg -= nn;
        }
      }
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PlaceSeenOnMapVQ(int ii, int jj) {
    if (GetMatrixCount(ii)!=1)
      return ShowError("Query::PlaceSeenOnMapVQ(): matrixcount!=1");

    const TreeSOM& ts = Map(ii, jj);
    simple::FloatMatrix&  hts = Hits(ii, jj, 0);
    SetSizeOrZero(hts, ts);

    for (size_t i=0; i<NseenObjects(); i++) { 
      int mapidx = Division(ii, jj)[seenobject[i].Index()];
      IntPoint p = ts.ToPoint(mapidx);        
    
      if (seenobject[i].Value() > 0) 
        hts.Add(p.Y(), p.X(), 1.0); 
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::IsInNewObjects(size_t idx) const {
    return newobject.contains(idx);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Convolve() {
    Tic("Convolve");

    bool ok = true;

    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i))
        for (size_t j=0; j<TsSom(i).Nlevels(); j++)
          for (size_t k=0; k<GetMatrixCount(i); k++)
            ok = ok && Convolve(i, j, k);

    Tac("Convolve");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Convolve(int i, int j, int k) {
    const string& ct = ConvType(i);

    if (ct.empty() || ct=="one-dim-mask")
      return ConvolveOneDimMask(i, j, k, TsSom(i).CreateCVector(j));

    if (ct.substr(0, 8)=="umatrix-")
      return ConvolveUmatrix(ct.substr(8), i, j, k);
    
    if (ct.substr(0,8)=="nbrrank-")
      return ConvolveNbrRank(ct.substr(8),i, j, k);

    if(ct.substr(0,10)=="ratioconv-")
      return ConvolveRatio(ct.substr(10),i,j,k);
  

    const FloatVector *cvec = ConvolutionVector(ct, j);
    if (!cvec)
      return ShowError("Query::Convolve() : convtype==[", ct,
                       "] unknown or failed");

    return ConvolveOneDimMask(i, j, k, *cvec);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveUmatrix(const string& t, int ii, int jj, int kk) {
    string type = t;

    //float& (SOM::*ufunc)(int,int,bool) const = NULL;
    SOM::UmatrixFunc ufunc = NULL;

    if (type.substr(0, 4)=="one-") {
      ufunc = &SOM::OneUmatrix;
      type.erase(0, 4);

    } else if (type.substr(0, 9)=="unitmean-") {
      ufunc = &SOM::UnitMeanUmatrix;
      type.erase(0, 9);
      if (!Map(ii, jj).HasUnitMeanUmatrix() &&
          !Map(ii, jj).CalculateUnitMeanUmatrix())
        return ShowError("ConvolveUmatrix() no raw Umatrix available ?");
    
    } else
      return ShowError("ConvolveUmatrix() [", type, "] unimplemented 1");

    if (type.substr(0, 3)=="sum")
      return ConvolveUmatrixSum(type.substr(3), ufunc, ii, jj, kk);

    if (type.substr(0, 4)=="path")
      return ConvolveUmatrixPath(type.substr(4), ufunc, ii, jj, kk);

    return ShowError("ConvolveUmatrix() [", type, "] unimplemented 2");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveUmatrixSum(const string& t, SOM::UmatrixFunc uf,
                                 int ii, int jj, int kk) {
    string type = t, order = "hv+vh";

    if (type.substr(0, 1)=="-")
      type.erase(0, 1);
    else if (type.substr(0, 1)==":") {
      order = type.substr(1);
      order.erase(order.find("-"));
      type.erase(0, 2+order.length());
    } else
      return ShowError("ConvolveUmatrixSum() couldn't parse [", t, "]");

    // cout << "[" << order << "]" << "[" << type << "]" << endl;

    float len, par;
    ConvolutionFunction cf = ConvolutionVectorFunc(type, jj, len, par);
    if (!cf)
      return ShowError("ConvolveUmatrixSum() failed [", type, "]");

    simple::FloatMatrix& cnv = Convolved(ii, jj, kk);
    const simple::FloatMatrix& hits = Hits(ii, jj, kk);
    cnv.SizeOrZero(hits);

    const int r = hits.Rows(), c = hits.Columns();
    simple::FloatMatrix tmp1(r, c), tmp2(r, c);

    if (!ConvolveUmatrixSum(order, &Map(ii, jj), uf,cf,len,par,hits,tmp1,cnv))
      return ShowError("ConvolveUmatrixSum() fail 1 [", order, "]");

    // cout << "===================== 1 " << endl;
    // tmp1.Dump(DumpLong);
    // cnv.Dump(DumpLong);

    if (!ConvolveUmatrixSum(order, &Map(ii, jj), uf,cf,len,par,tmp1,tmp2,cnv))
      return ShowError("ConvolveUmatrixSum() fail 2 [", order, "]");

    // cout << "===================== 2 " << endl;
    // tmp2.Dump(DumpLong);
    // cnv.Dump(DumpLong);

    if (order.substr(0, 1)=="+") {
      order.erase(0, 1);

      if (!ConvolveUmatrixSum(order, &Map(ii, jj), uf,cf,len,par,hits,tmp1,cnv))
        return ShowError("ConvolveUmatrixSum() fail 1 [", order, "]");

      // cout << "===================== 3 " << endl;
      //   tmp2.Dump(DumpLong);
      //   cnv.Dump(DumpLong);

      if (!ConvolveUmatrixSum(order, &Map(ii, jj), uf,cf,len,par,tmp1,cnv,cnv))
        return ShowError("ConvolveUmatrixSum() fail 2 [", order, "]");

      // cout << "===================== 4 " << endl;
      //   cnv.Dump(DumpLong);

      cnv += tmp2;

      // cout << "===================== 5 " << endl;
      //   cnv.Dump(DumpLong);

    }

    if (order.length())
      return ShowError("ConvolveUmatrixSum() fail n [", order, "]");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveUmatrixSum(string& order,
                                 const SOM *som, SOM::UmatrixFunc ufunc,
                                 ConvolutionFunction cfunc,
                                 double len, double par,
                                 const simple::FloatMatrix& s,
                                 simple::FloatMatrix& d1, simple::FloatMatrix& d2) {
    if (!order.length())
      return true;

    simple::FloatMatrix *dp = &d1;

    bool horiz = false;
    if (order.substr(0, 1)=="h")
      horiz = true;
    else if (order.substr(0, 1)!="v")
      return false;

    order.erase(0, 1);

    if (!order.length())
      dp = &d2;

    return ConvolveUmatrixSum(som, ufunc, cfunc, len, par, s, *dp, horiz);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveUmatrixSum(const SOM *som, SOM::UmatrixFunc ufunc,
                                 ConvolutionFunction cfunc,
                                 double len, double par,
                                 const simple::FloatMatrix& s, simple::FloatMatrix& d,
                                 bool horiz) {

    const int w = som->Width(), h = som->Height();

    /*
      cout << "Query::ConvolveUmatrixSum() horiz=" << horiz
      << " w=" << w << " h=" << h
      << " d.Columns()=" << d.Columns() << " d.Rows()=" << d.Rows()
      << endl;
    */

    for (int x=0; x<w; x++)
      for (int i, y=0; y<h; y++) {
        double dist = 0;
        double val = (*cfunc)(dist, len, par);
        double sum = s.FastGet(y, x)*val, div = val; 

        if (horiz) {
          for (i=0; ; i++) {
            if (x+i+1==w)
              break;
            dist += (som->*ufunc)(x+i, y, true);
            val = (*cfunc)(dist, len, par);
            if (val<=0)
              break;
            sum += s.FastGet(y, x+i+1)*val;
            div += val; 
          }
          dist = 0;
          for (i=-1; ; i--) {
            if (x+i==-1)
              break;
            dist += (som->*ufunc)(x+i, y, true);
            val = (*cfunc)(dist, len, par);
            if (val<=0)
              break;
            sum += s.FastGet(y, x+i)*val;
            div += val; 
          }

        } else {
          for (i=0; ; i++) {
            if (y+i+1==h)
              break;
            dist += (som->*ufunc)(x, y+i, false);
            val = (*cfunc)(dist, len, par);
            if (val<=0)
              break;
            sum += s.FastGet(y+i+1, x)*val;
            div += val; 
          }
          dist = 0;
          for (i=-1; ; i--) {
            if (y+i==-1)
              break;
            dist += (som->*ufunc)(x, y+i, false);
            val = (*cfunc)(dist, len, par);
            if (val<=0)
              break;
            sum += s.FastGet(y+i, x)*val;
            div += val; 
          }
        }

        d.Set(y, x, sum/div);
        // cout << "x=" << x << " y=" << y << " v=" << sum/div << endl;
      } 
      
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveUmatrixPath(const string& t, SOM::UmatrixFunc uf,
                                  int ii, int jj, int kk) {
    string type = t;
    if (type.substr(0, 1)=="-")
      type.erase(0, 1);

    float len, par;
    ConvolutionFunction cf = ConvolutionVectorFunc(type, jj, len, par);
    if (!cf)
      return ShowError("ConvolveUmatrixPath() failed [", type, "]");

    TreeSOM& map = Map(ii, jj);
    float old_limit = map.UmatrixDistanceLimit();
    bool ok = map.SolveUmatrixDistances(len, uf);
    if (!ok)
      return false;

    if (old_limit!=map.UmatrixDistanceLimit()) {
      char tmp[1000];
      int tl = map.UmatrixDistanceCount();
      sprintf(tmp, "%s[%d] units=%d tot.len=%d avg.len=%f",
              TsSom(ii).Name().c_str(), jj, map.Units(), tl, float(tl)/map.Units());
      WriteLog("Solved U-matrix distances for ", tmp);
    }

    simple::FloatMatrix& cnv = Convolved(ii, jj, kk);
    const simple::FloatMatrix& hits = Hits(ii, jj, kk);
    cnv.SizeOrZero(hits);

    for (int y=0; y<cnv.Rows(); y++)
      for (int x=0; x<cnv.Columns(); x++) {
        const vector<SOM::umatrix_dist_t>& umd = map.GetUmatrixDistances(x, y);
        double sum = 0, div = 0;
        for (size_t i=0; i<umd.size(); i++) {
          int    u = umd[i].Unit(), ux = map.XCoord(u), uy = map.YCoord(u);
          double d = umd[i].Distance();
          double v = (*cf)(d, len, par);
          sum += v*hits.FastGet(uy, ux);
          div += v;
          // if (hits.FastGet(uy, ux))
          //   cout << "y=" << y << " x=" << x << " u=" << u << " ux=" << ux
          //        << " uy=" << uy << " d=" << d << " v=" << v << endl; 
        }
        double cv = sum/div;
        cnv.Set(y, x, cv);
        //       cout << "x=" << x << " y=" << y << " cv=" << cv << endl;
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveOneDimMask(int ii, int jj, int kk,
                                 const FloatVector& v) {
    if (!v)
      return false;

    char msgtmp[1000];
    sprintf(msgtmp, "Convolve(%s, %d, %d)", TsSom(ii).Name().c_str(), jj, kk);

    // Tic(msgtmp);

    const simple::FloatMatrix& hits = Hits(ii, jj, kk);
    simple::FloatMatrix& cnv = Convolved(ii, jj, kk);
    cnv.SizeOrZero(hits);

    const int r = hits.Rows(), c = hits.Columns(), l = v.Length();
    register int x, y;

    simple::FloatMatrix tmp(r, c);

    const TreeSOM& map = Map(ii, jj); // was EvenAlien

    bool store_si=store_selfinfluence && map.IsBottomLevel();
    if (store_si) {
      vector<float> colvec(r,0.0);
      location_selfinfluence[ii]=vector<vector<float> >(c,colvec);
    }

    double masksum = 2*v.Sum()-v.Get(0);

    bool circular = (TsSom(ii).Map(jj).Topology()==simple::Planar::Torus);
    if (circular==false) {
      bool non_zero_found = false;
      for (x=0; x<c; x++) {
        const int imax = l<c-x ? l : c-x, imin = l<=x ? l : x+1;
        for (y=0; y<r; y++) {
          register double sum = 0, div = 0;
          register int i;

          for (i=0; i<imax; i++) {
            register double t = v.FastGet(i);
            sum += t*hits.FastGet(y, x+i); 
            div += t;
          }
          for (i=1; i<imin; i++) {
            register double t = v.FastGet(i);
            sum += t*hits.FastGet(y, x-i); 
            div += t;
          }
          if (bordercompensation==false)
            div = masksum;
          tmp.FastSet(y, x, sum/div);
          if (store_si)
            location_selfinfluence[ii][x][y]=1/div; 
          if (sum!=0.0)
            non_zero_found = true;
        }
      }
      if (!non_zero_found && !zero_matrices_allowed)
        ShowError("only zero values found in "+string(msgtmp));

    } else { // circular
      for (x=0; x<c; x++) {
        for (y=0; y<r; y++) {
          register double sum = 0, div = 0;
          register int i;
          for (i=0; i<l; i++) {
            register double t = v.FastGet(i);
            sum += t*hits.FastGet(y, (x+i)%c); 
            div += t;
          }
          for (i=1; i<l; i++) {
            register double t = v.FastGet(i);
            sum += t*hits.FastGet(y, (x-i+c)%c); 
            div += t;
          }
          tmp.FastSet(y, x, sum/div);
          if (store_si)
            location_selfinfluence[ii][x][y]=1/div; 
        }
      }
    }

    if (circular==false) {
      for (y=0; y<r; y++) {
        const int imax = l<r-y ? l : r-y, imin = l<=y ? l : y+1;
        for (x=0; x<c; x++) {
          register double sum = 0, div = 0;
          register int i;
        
          for (i=0; i<imax; i++) {
            register double t = v.FastGet(i);
            sum += t*tmp.FastGet(y+i, x); 
            div += t;
          }
          for (i=1; i<imin; i++) {
            register double t = v.FastGet(i);
            sum += t*tmp.FastGet(y-i, x); 
            div += t;
          }
          if (bordercompensation==false)
            div = masksum;
          cnv.FastSet(y, x, sum/div);
          if (store_si)
            location_selfinfluence[ii][x][y]*=1/div; 
        }
      }

    } else { // circular
      for (y=0; y<r; y++) {
        for (x=0; x<c; x++) {
          register double sum = 0, div = 0;
          register int i;
        
          for (i=0; i<l; i++) {
            register double t = v.FastGet(i);
            sum += t*tmp.FastGet((y+i)%r, x); 
            div += t;
          }
          for (i=1; i<l; i++) {
            register double t = v.FastGet(i);
            sum += t*tmp.FastGet((y-i+r)%r, x); 
            div += t;
          }
          cnv.FastSet(y, x, sum/div);
          if(store_si)
            location_selfinfluence[ii][x][y]*=1/div; 
        }
      }
    } // circular 
    // Tac(msgtmp);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveNbrRank(const string& t,int ii, int jj, int kk) {

    string type(t);

    const TSSOM &tssom=TsSom(ii);

    bool bubble=false;

    const TreeSOM& map = Map(ii, jj); // was EvenAlien
    bool store_si=store_selfinfluence && map.IsBottomLevel();

    // const int NN=100;

    if(type.substr(0,7)=="bubble-"){
      bubble=true;
      type=type.substr(7);
    }
      

    int nn=atoi(type.c_str());

    char msgtmp[1000];
    sprintf(msgtmp, "ConvolveNbrRank(%s, %d %d) nn=%d", TsSom(ii).Name().c_str(), jj, kk,nn);

    // Tic(msgtmp);

    cout << msgtmp << endl;

    const simple::FloatMatrix& hits = Hits(ii, jj, kk);
    simple::FloatMatrix& cnv = Convolved(ii, jj, kk);
    cnv.SizeOrZero(hits);

    const int r = hits.Rows(), c = hits.Columns();

    if(store_si){
      vector<float> colvec(r,0.0);
      location_selfinfluence[ii]=vector<vector<float> >(c,colvec);
    }
    
    vector<float> cnvVect(c*r,0.0);
    register int x, y;

    if ((int)tssom.nearest_neighbour.size()<=jj ||
        (int)tssom.nearest_neighbour[jj].size()<r*c)
      {
        cout << "neighbour information not found" << endl;
        return false;
      }

    int src_ind=0;

    for(y=0;y<r;y++) 
      for (x=0; x<c; x++,src_ind++){
        float h=hits.FastGet(y, x); 
        int n_nbr=nn;
        if(n_nbr>(int)tssom.nearest_neighbour[jj][src_ind].size())
          n_nbr=tssom.nearest_neighbour[jj][src_ind].size();

        if(store_si)
          location_selfinfluence[ii][x][y]= bubble? 2.0/((n_nbr+1)*n_nbr) :
            (2.0*n_nbr*n_nbr)/((n_nbr+1)*n_nbr);

        if(h==0.0) continue;

        float mul=2.0*h/((n_nbr+1)*n_nbr);

        for(int n=0;n<n_nbr;n++){
          const int &nind=tssom.nearest_neighbour[jj][src_ind][n];
          float val=bubble ? mul : (n_nbr*n_nbr-n*n)*mul;

          cnvVect[nind] += val;

        }




      }

    src_ind=0;
    for(y=0;y<r;y++) 
      for (x=0; x<c; x++,src_ind++){
        cnv.FastSet(y, x, cnvVect[src_ind]);
      }

    return true;
  }


  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvolveRatio(const string &rstr,int ii, int jj, int kk) {

    float ratio;

    if(sscanf(rstr.c_str(),"%f",&ratio)!=1 || ratio<0 || ratio>=1)
      return ShowError("ConvolveRatio() Valid ratio not found in [", rstr.c_str(), "]");
    
    //    cout <<"ConvolveRatio("<<ii<<","<<jj<<","<<kk<<"): r="<<ratio<<endl;

    const simple::FloatMatrix& hits = Hits(ii, jj, kk);
    simple::FloatMatrix& cnv = Convolved(ii, jj, kk);
    cnv.SizeOrZero(hits);

   


    const int r = hits.Rows(), c = hits.Columns();
    register int x, y;

    const TreeSOM& map = Map(ii, jj);
    bool store_si=store_selfinfluence && map.IsBottomLevel();

    if(store_si){
      vector<float> colvec(r,0.0);
      location_selfinfluence[ii]=vector<vector<float> >(c,colvec);
    }

    float *temp =new float[r*c];

    float *right= new float[c];
    
    //    cout << "starting horisontal propagation" << endl;

    for(y=0;y<r;y++){
      int rowoffs=y*c;

      right[0]=hits.FastGet(0,y);
      for(x=1;x<c;x++)
        right[x]=ratio*right[x-1] + hits.FastGet(x,y); 
    
      temp[c-1+rowoffs] = right[c-1];
    
      float accu=hits.FastGet(c-1,y);
      for(x=c-2;x>=0;x--){
        accu=ratio*accu;
        temp[rowoffs+x]=right[x]+accu;
        accu += hits.FastGet(x,y);
      }
    }
    delete right;

    float *down= new float[r];

    //    cout << "starting vertical propagation" << endl;

    for(x=0;x<c;x++){
      int rowind;

      down[0]=temp[x];
      for(y=1,rowind=c;y<r;y++,rowind+=c)
        down[y]=ratio*down[y-1]+temp[x+rowind];
    
      rowind -= c; // now (r-1)*c
      cnv.FastSet(x,r-1,down[r-1]);

      float accu=temp[x+rowind];
      for(y=r-2,rowind-=c;y>=0;y--, rowind-=c){
        accu=ratio*accu;
        cnv.FastSet(x,y,down[y]+accu);
        accu += temp[x+rowind];
      }
    }
    delete down;
    delete temp;

    //    cout << "finished vertical propagation" << endl;

    if(store_si)
      for(x=0;x<c;x++) for(y=0;y<r;y++)
        location_selfinfluence[ii][x][y]=1; 

    //    cout << "ConvolveRatio() finishing" << endl;

    return true;
  }

/////////////////////////////////////////////////////////////////////////////

  bool Query::BernoulliMAP(int ii, int jj) {
    MapSurfaceMAPOpt opt;

    char msgtmp[1000];
    sprintf(msgtmp, "BernoulliMAP(%s, %d)", TsSom(ii).Name().c_str(), jj);
    // Tic(msgtmp);

    //cout << msgtmp << endl;

    const simple::FloatMatrix& pos = Hits(ii, jj,1);
    const simple::FloatMatrix& neg = Hits(ii, jj,2);

    simple::FloatMatrix& cnv = Convolved(ii, jj, 0);
    cnv.SizeOrZero(pos);

    const int r = pos.Rows(), c = pos.Columns();

    opt.w=c;
    opt.h=r;

    opt.ConstructInverseCovariance();
    

    int *posdata=new int[r*c];
    int *negdata=new int[r*c];

     // obs! HAS_CBIRALGORITHM_NEW problem
    double positives = 0, negatives = 0;
    SumPositivesAndNegatives(Division(ii, jj), IndexData(NULL, ii),
                             positives, negatives, true);

    int x, y;

    for(y=0;y<r;y++) 
      for (x=0; x<c; x++){
        int ind=x+c*y;
        posdata[ind]=(int)(pos.FastGet(y,x)*positives+0.5);
        negdata[ind]=(int)(neg.FastGet(y,x)*negatives+0.5);
      }

    opt.SetHitCounts(posdata,negdata);

    // construct initial guess
    vector<double> theta(r*c,0.0);

    theta=opt.Optimise(theta);
    opt.ParamToProb(theta);

    for(y=0;y<r;y++) 
      for (x=0; x<c; x++){
        int ind=x+y*c;
        cnv.FastSet(y, x, theta[ind]);
      }

    return true;
  }


  /////////////////////////////////////////////////////////////////////////////

  bool Query::IsEmptyMappoint(int m, int l, int x, int y) const {
    const TreeSOM& ts = Map(m, l);
  
    if (ts.Unit(x,y)->HasLabel())
      return false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *Query::AllowedMapPointMovements() const {
    if (!IsMapPointSet())
      return "";

    static char ret[10];
    int i = 0;
    const TreeSOM& ts = PointedMapLevel();
    if (pointed_level)                                      ret[i++] = 'n';
    if (pointed_level<int(TsSom(pointed_map).Nlevels())-1)  ret[i++] = 's';
    if (pointed_map)                                        ret[i++] = 'w';
    if (pointed_map<(int)Nindices()-1)                      ret[i++] = 'e';
    if (maparea_ul.Y())                                     ret[i++] = 'u';
    if (maparea_lr.Y()<ts.Height()-1)                       ret[i++] = 'd';
    if (maparea_ul.X())                                     ret[i++] = 'l';
    if (maparea_lr.X()<ts.Width()-1)                        ret[i++] = 'r';

    ret[i] = 0;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RelativeMapPointMovement(int ch) {
    if (!parent || !parent->IsMapPointSet())
      return false;

    mappoint = parent->mappoint;
    pointed_map = parent->pointed_map;
    pointed_level = parent->pointed_level;
    const TreeSOM& ts = parent->PointedMapLevel();

    switch (ch) {
    case 'u': mappoint.Y(mappoint.Y()-1.0/ts.Height()); break;
    case 'd': mappoint.Y(mappoint.Y()+1.0/ts.Height()); break;
    case 'l': mappoint.X(mappoint.X()-1.0/ts.Width());  break;
    case 'r': mappoint.X(mappoint.X()+1.0/ts.Width());  break;
    case 'n': pointed_level--; break;
    case 's': pointed_level++; break;
    case 'w': pointed_map = pointed_map-1; break;
    case 'e': pointed_map = pointed_map+1; break;
    default:  ShowError("mappoint direction undefined"); return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::RoundMapPoint() {
    if (!IsMapPointSet())
      return;

    const TreeSOM& ts =  PointedMapLevel();
    int w = ts.Width(), h = ts.Height();
    int x = (int)floor(w*mappoint.X()), y = (int)floor(h*mappoint.Y());
    mappoint.Set((2*x+1.0)/(2*w), (2*y+1.0)/(2*h));
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::SelectAroundMapPoint() {
    if (!IsMapPointSet())
      return;

    const TreeSOM& ts = PointedMapLevel();

    for (int y=maparea_ul.Y(); y<=maparea_lr.Y(); y++)
      for (int x=maparea_ul.X(); x<=maparea_lr.X(); x++) {
        char extra[1000];
        sprintf(extra, "%s[%d][%d,%d]",
                TsSom(pointed_map).Name().c_str(), pointed_level, x, y);

        int idx = ts.Unit(x, y)->LabelIndex();
        // obs! this doesn't yet handle aspects via AspectsFromIndices();
        AppendNewObject(new Object(the_db, idx, select_map, 0, extra));

        // cout << "x=" << x << " y=" << y << " " << ts.Unit(x,y)->Label()
        //      << " " << idx << endl;
      }
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::SelectByContents() {
    if (!HasViewClass())
      return;

    target_type tt = Target();
    // if (tt==target_no_target) // commented out 2.5.2006 by jl
    //   tt = target_image;  // this might be redundant... 

    bool expand = true;
    ground_truth view = the_db->GroundTruthExpression(viewclass, tt,
                                                      -1, expand);
    ground_truth posi(DataBaseSize()), nega(DataBaseSize());

    if (HasPositive())
      posi = the_db->GroundTruthExpression(Positive(), tt, -1,expand);
    if (HasNegative())
      nega = the_db->GroundTruthExpression(Negative(), tt, -1,expand);
  
    for (size_t i=0; i<DataBaseSize(); i++) {
      if (view.IndexOK(i) && view[i]==1) {
        double val = 1.0; // was 0.0 until 27.9.2005
        // obs! this doesn't yet handle aspects via AspectsFromIndices();
        AppendNewObject(new Object(the_db, i, select_show, val));
      }

      bool is_posi = posi.IndexOK(i) && posi[i]==1;
      bool is_nega = nega.IndexOK(i) && nega[i]==1;

      if (is_posi || is_nega) {
        double val = is_posi ? 1 : -1;
        // obs! this doesn't yet handle aspects via AspectsFromIndices();
        AddToSeen(new Object(the_db, i, select_answer, val));
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CanBeShown(size_t idx, bool checkparents) const {
    const DataBase *db = CheckDB();

    if (!db->IndexOK(idx))
      return ShowError("Query::CanBeShown("+ToStr(idx)+") index not OK");

    bool retained = false, retval = false, parent_ok = true, nrs = false,
      set_by_parent = false;
      
    bool is_seen = false;
    if (checkparents && Target()==target_type(target_image+target_full) &&
	db->ObjectsTargetType(idx)==target_type(target_image+target_segment)) {
      is_seen = IsSeen(db->ParentObject(idx));
      retval = !is_seen;
      set_by_parent = true;

    } else
      is_seen = IsSeen(idx);

    if (!set_by_parent && (can_show_seen || !checkparents || !can_be_shown_and)) {
      if (!is_seen)
        retval = true;

      else if (can_show_seen)
        retval = true;

      else {
        const Object *img = FindInSeen(idx);
        if (!img) {
          ShowError("Query::CanBeShown() : logic failure");
          img = FindInSeen(idx);
          return false;
        }

        if (checkparents && img->Parent()!=-1)
          parent_ok = CanBeShown(img->Parent(), false);  // or true ???

        retained = img->Retained();

        if (img->not_really_seen) 
          retval = nrs = true;
        else 
          retval = !retained && parent_ok;
      }
    } else { // set_by_parent || !canshowseen && can_be_shown_and && checkparents

      if (is_seen)
        retval = false;

      else if (!set_by_parent) {
        const int pind=the_db->ParentObject(idx);
        if (pind != -1)
          parent_ok = CanBeShown(pind, false);
        retval = parent_ok;
      }
    }

    if (debug_can_be_shown) {
      cout << "  CanBeShown() ";
      db->FindObject(idx)->dump_nonl(cout);
      cout << " : " << checkparents
           << " " << can_show_seen << " " << can_be_shown_and << " " << is_seen
           << " " << nrs << " " << retained
           << " " << parent_ok << " -> " << retval << endl;
    }

    return retval;
  }

  /////////////////////////////////////////////////////////////////////////////

  VQUnitList Query::BestVQUnits(int ii, int jj) const {
    char ticmsg[100];
    sprintf(ticmsg, "BestVQUnits(%d, %d)", ii, jj);
    // Tic(ticmsg);
    bool seenparents = true;
    bool allow_empty_labels = GetDataBase()->EmptyLabelsAllowed();

    const simple::FloatMatrix& cnv = Convolved(ii, jj, 0);

    //int showable_units=0,img_tot=0;
    VQUnitList unitret;

    if (MapIndexOK(ii) && TsSom(ii).LevelOKEvenAlien(jj) && cnv.Size()) {
      int pmni = _sa_permapobjects;
      bool non_zero_found = false;

      const TreeSOM& map = Map(ii, jj); // was EvenAlien
      for (IntPoint p=map; p; ++p) {
        int mapidx = map.ToIndex(p);
        int labidx = map.Unit(mapidx)->LabelIndex();

        const IntVector& brf = BackReference(ii, jj, mapidx);
        const char *lab = map.Unit(mapidx)->Label();
        if (!lab) {
          //
          // we might also set lab and labidx on-fly from brf!
          //

          if (!allow_empty_labels) {
            if (brf.Length())
              ShowError("Query::BestVQUnits() label=NULL, backrefs exist "
                        "consider setting empty_labels_allowed=true");
            continue;
          }
        
          if (labidx!=-1) {
            ShowError("Query::BestVQUnits() label=NULL, labidx!=-1");
            continue;
          }

        } else if (labidx<0) {
          ShowError("Query::BestVQUnits(): labidx<0");
          continue;
        }

        //img_tot += BackReference(ii, jj, mapidx).Length();

        if (labidx==-1 || !CanBeShownRestricted(labidx, seenparents)) {
          if (!map.IsBottomLevel())
            continue;
          else {
            bool found = false;
            for (int i=0; !found && i<brf.Length(); i++)
              if (CanBeShownRestricted(brf[i], seenparents))
                found = true;

            if (!found) {
                //cout << "Query::BestVQUnits(): rejected unit with brf count"
                //     << brf.Length() << endl;
              continue;
            }
          }
        }

        float val = cnv.Get(p.Y(), p.X());
        if (val!=0.0)
          non_zero_found = true;

        VQUnit candidate(mapidx, val, ii, jj);
        //showable_units++;
        unitret.add_and_sort((size_t)pmni, candidate);
      }

      if (!non_zero_found && !zero_matrices_allowed)
        ShowError("only zero values in "+string(ticmsg));
    }

    // Tac(ticmsg);

    //   cout << "BestVQUnits: "<<showable_units<<" showable units." << endl;
    //cout << "Total images in units: "<<img_tot<<endl;
    return unitret;
  }

  /////////////////////////////////////////////////////////////////////////////

  VQUnitList Query::BestVQUnitsVQ(int ii, int jj) const {
    char ticmsg[100];
    sprintf(ticmsg, "BestVQUnitsVQ(%d, %d)", ii, jj);
    Tic(ticmsg);
  
    FloatVector score;
    VQUnitList unitret;

    if (MapIndexOK(ii) && TsSom(ii).LevelOK(jj)) {
      int pmni = _sa_permapobjects;
  
      score = MapUnitScore(ii, jj);
      if (!score.Length()) {
        cerr << "Query::BestVQUnitsVQ(): MapUnitScore(" <<ii<< ") failed!"
             <<endl;
        exit(1);
      }

      const TreeSOM& map = Map(ii, jj);    
      for (IntPoint p=map; p; ++p) {
        int idx = map.ToIndex(p);
    
        if (score[idx] < 0.0001) // skips units without images (score == 0.0) 
          continue;

        VQUnit vqu(idx, score[idx], ii, jj);

        unitret.add_and_sort((size_t)pmni, vqu);
      } 
    }

    Tac(ticmsg);

    return unitret;
  }

  /////////////////////////////////////////////////////////////////////////////

  ObjectList Query::SelectObjectsFromVQUnits(double weight,
                                             const VQUnitList& list,
                                             const TreeSOM& map, bool aliendata,
                                             int midx, int nmat) const {
    bool trace = false;

    bool seenparents = true;
    bool allow_empty_labels = GetDataBase()->EmptyLabelsAllowed();

    size_t n_extracted = 0;
  
    Tic("SelectObjectsFromVQUnits");

    int pmni = _sa_permapobjects;

    DataBase *db = GetDataBase();

    ObjectList imageret;
    FloatVector stvec(nmat+1);
    stvec.Set(MAXFLOAT);

    for (size_t j=0; j<list.size(); j++) {
      select_type q = select_question;
      int idx = list[j].Index();
      double valw = list[j].Value();

      valw *= weight;
      if (debug_weights)
        cout << "1st stage weighting: " << weight << " * " << list[j].Value()
             << " = " << valw << endl;

      if (!aliendata) {
        // const char *lab = map.Unit(idx)->Label();
        // int labidx = lab ? LabelIndex(lab) : -1;
        int labidx = map.Unit(idx)->LabelIndex();
        if (labidx<0 && !allow_empty_labels)
          ShowError("Query::SelectImagesFromVQUnits() empty label problem");

        stvec.Last() = stvec[midx] = valw;

        if (labidx>=0 && CanBeShownRestricted(labidx, seenparents)) {
          Object candidate(db, labidx, q, valw, "", 0, &stvec);
          imageret.add_and_sort((size_t)pmni, candidate);
          n_extracted++;
        }
        //else cout << "SelectImagesFromVQUnits() rejected label "
        //                << lab << endl;
      }

      if (map.IsBottomLevel()) {

        int ii = list[j].MapNumber();
        int jj = list[j].MapLevel();
        const IntVector& brf = BackReference(ii, jj, idx);

        for (int i=0; i<brf.Length(); i++) {
          int labidx = brf[i];
          const char *lab = db->LabelP(labidx);

          if (!Simple::StringsMatch(lab, map.Unit(idx)->Label()) &&
              CanBeShownRestricted(labidx, seenparents)) {
            Object candidate(db, labidx, q, valw, "", 0, &stvec);
            imageret.add_and_sort((size_t)pmni, candidate);
            n_extracted++;
            //if (map.Unit(idx)->Label()==NULL)
            //  cout << "OK OK OK OK OK OK OK OK OK OK OK OK OK OK" << endl;
          }

        }
      }
    }

    Tac("SelectObjectsFromVQUnits");

    if (trace)
      cout << "SelectObjectsFromVQUnits() : n_extracted=" << n_extracted
	   <<" pmni=" << pmni << " imageret.size()=" << imageret.size()
	   << endl;

    return imageret;
  }

  /////////////////////////////////////////////////////////////////////////////

  ObjectList Query::SelectObjectsFromVQUnitsVQ(const VQUnitList& list) const {
    ObjectList imageret;

    Tic("SelectImagesFromVQUnitsVQ");

    int pmni = _sa_permapobjects;
    DataBase *db = GetDataBase();

    for (size_t j=0; j<list.size(); j++) {
      int idx = list[j].Index();
      double trueval = list[j].Value(), val = trueval;

      int ii = list[j].MapNumber();
      int jj = list[j].MapLevel();
      const IntVector& brf = BackReference(ii, jj, idx);

      for (int i=0; i<brf.Length(); i++)
        if (CanBeShownRestricted(brf[i], true)) {
          Object obj(db, brf[i], select_question, val);
          imageret.add_and_sort((size_t)pmni, obj);

          if (trueval==1.0)
            val = 0.5+(val-0.5)*0.5;

          // OBS! could this condition be removed ?
          //if (val<=1.0) // breadth-first search for units w/o positives
          //  continue;
        }
    }

    Tac("SelectImagesFromVQUnitsVQ");

    return imageret;
  }

  /////////////////////////////////////////////////////////////////////////////

  DoubleVector Query::DistancesToPositives(const FloatVectorSet& pos,
                                           int m) {
    Tic("DistancesToPositives");

    const int n = NnewObjects();
    DoubleVector dist(n);
    const FloatVectorSet &data = TsSom(m).Data();
  
    for (int i=0; i<n; i++) {
      const FloatVector& dataitem = data[newobject[i].Index()];
      Tic("MeanSquaredDistance");
      dist[i] = pos.MeanSquaredDistance(dataitem);    
      Tac("MeanSquaredDistance");
    }
    dist /= dist.Sum();

    Tac("DistancesToPositives");

    return dist;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::ResolveNearestNewObjectsVQ(bool /*dowrite*/) {
    Tic("ResolveNearestNewObjectsVQ");

    for (size_t i=1; i<NnewObjects(); i++)
      for (int j=i-1; j>=0; j--)
        if (newobject[j].Value()>newobject[j+1].Value()) 
          NewObject().swap_objects(j, j+1);

    while (NnewObjects()>MaxQuestions()) 
      newobject.erase(newobject.size()-1);

    Tac("ResolveNearestNewObjectsVQ");
  }

  /////////////////////////////////////////////////////////////////////////////

//   simple::FloatMatrix Query::CreateContentMatrix(int m, int l, const char *cnt) {
//     const SOM& map = Map(m, l);
//     simple::FloatMatrix ret(map.Height(), map.Width());
//     simple::FloatMatrix total(map.Height(), map.Width());
//     int i, idx = -1;

//     for (i=0; i<GetDataBase()->ContentsColumns(); i++)
//       if (idx==-1 && StringsMatch(GetDataBase()->ContentsColumnLabel(i), cnt))
//         idx = i;

//     const IntVector& div = Division_y(m, l);
//     for (i=0; i<GetDataBase()->ContentsRows(); i++) {
//       total.Add(map.YCoord(div[i]), map.XCoord(div[i]), 1);
//       if (GetDataBase()->Contents(i, idx))
//         ret.Add(map.YCoord(div[i]), map.XCoord(div[i]), 1);
//     }

//     //float score = CalculateScore(m,l,idx);
//     //float score = CalculateScore(m,l,idx,ret);
 
//     int y, x, Y=map.Height(), X=map.Width();
//     for (y=0; y<Y; y++)
//       for (x=0; x<X; x++) {
//         if (total.Get(y, x))
//           ret.Multiply(y, x, 1.0/total.Get(y, x));
//       }

// //   picsom->Log() << "PicSOM::CreateContentMatrix():"
// //               << " (" << m << "," << l << ")"
// //               << " idx=" << idx;
// //   if (idx!=-1)
// //     picsom->Log() << " label=" << GetDataBase()->ContentsColumnLabel(idx)
// //                 << " sum="   << GetDataBase()->ContentsColumnSum(idx);
// //picsom->Log() << " score=" << score;
// //   picsom->Log() << endl;

//     return ret;
//   }

  /////////////////////////////////////////////////////////////////////////////

  void Query::WriteSelectionsToFile() const {
    if (setname=="")
      return;
  
    bool negimages = false;
    char *timename = NULL;
    if (setname == "-@-store-@-results-@-") {
      negimages = true;
      time_t currtime = time(NULL); 
      tm *time = localtime(&currtime);
      char tmp[1024];
      strftime(tmp, sizeof tmp, "%Y-%m-%d-%H-%M-%S", time);
      CopyString(timename, tmp);
    }

    ground_truth set(the_db->Size());
    for (size_t i=0; i<NseenObjects(); i++) {
      if (seenobject[i].Value()>0)
        set.Set(seenobject[i].Index(), 1);
      else if (negimages && seenobject[i].Value()<0)
        set.Set(seenobject[i].Index(), -1);    
    }

    string px = the_db->ExpandPath("classes", timename?timename:setname);
    string fname = Simple::UniqueFileNameStr(px);

    string mycomment;
    mycomment += string("# ") + TimeStamp()     + "\n";
    mycomment += string("# ") + setname_comment + "\n";

    if (parent && HasViewClass())
      mycomment += "# Subset of contents selection: ["
        + parent->viewclass + "]\n";

    if (parent && parent->HasPositive())
      mycomment += "# Originally selected set was:  ["
        + parent->Positive() + "]\n";

    string currency(1, char(0xA4));
    string separator = currency+"*"+currency; // currency-asterisk-currency

    string com = comment;
    size_t esc;
    while ((esc = com.find(separator))!=string::npos)
      com.replace(esc, 3, "\n");
    mycomment += com;

    the_db->WriteClassFile(fname, set, mycomment, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::WriteClassdefToFile(const string& cls, const string& lbl, 
				  int val) const {
    
    int idx = LabelIndex(lbl);
    if (idx>=0) {
      ground_truth set(the_db->Size());

      ground_truth *gtp = the_db->ContentsFind(cls);
      ground_truth newgt = gtp ? *gtp : set;

      {
	set[idx] = newgt[idx] = val;
      }

      string mycls = /*"classdef::"+*/ cls;
      WriteLog("Writing label=", lbl, " (idx=", ToStr(idx),
	       ") to class file ...");
      WriteLog("  ", mycls, " with value=", ToStr(val));
      string fullmycls = the_db->ExpandPath("classes", mycls);	  
      string mycomment = TimeStamp();
      the_db->WriteClassFile(fullmycls, set, mycomment, true);
      
      the_db->ContentsUpdateOrInsert(newgt);
      
      if (the_db->DeleteClassModel(cls))
	WriteLog("Deleted outdated class model for [", cls, "]");
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SinglePass() {
    //   WriteLog("Starting Query::SinglePass(). Used algorithm: ",
    //            CbirAlgorithm(algorithm));

    bool skipcheck = IsMapPointSet() || HasViewClass() || BackFromSettings();

    bool hasold = false;
    for (size_t i=0; !hasold && i<NnewObjects(); i++)
      if (NewObject(i).Retained())
        hasold = true;

    if (hasold && !skipcheck) {
      ShowError("All images from previous round have not yet been classified");
      DumpNew();
      return false;
    }

    if (IsMapPointSet()) {
      DeleteNew();
      SelectAroundMapPoint();
      goto endit;
    }

    if (HasViewClass()) {
      DeleteNew();
      DeleteSeen();
      SelectByContents();
      seenobject = newobject;
      CreateIndexData(false);
      PlaceSeenOnMap();
      Convolve();
      goto endit;
    }

    if (setname!="")
      WriteSelectionsToFile();

    FindMissingIndices();

    if (HasAnalysis()) {
      if (GetAnalysis()->Analyse().errored())
        return ShowError("Query::SinglePass() : Analysis::Analyse() failed");
      return true;
    }

    // cout << endl << endl << "Query::SinglePass() Nindices()=" << Nindices()
    //      << " NindicesNew()=" << NindicesNew() << endl << endl << endl;

    if (NindicesNew())
      return CbirStages();

  endit:
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef USE_MRML

  bool Query::MRMLSinglePass(Connection *c) {
    Tic("MRMLSinglePass");

    c->DeleteXMLdoc();

    bool res = false;

    if (NseenObjects()==0 &&
        StageFunction(stage_initial_set)==func_only_forced_images) {
      res = MRMLOpenSession(c);  
      picsom->PossiblyShowDebugInformation("MRMLOpenSession() just run");
      if( res ) {
        res = MRMLGetCollections(c);
        picsom->PossiblyShowDebugInformation("MRMLGetCollections() just run");
      }
      if( res ) {
        res = MRMLGetAlgorithms(c);
        picsom->PossiblyShowDebugInformation("MRMLGetAlgorithms() just run");
      }
      if( res ) {
        res = MRMLConfigureSession(c);
        picsom->PossiblyShowDebugInformation("MRMLConfigureSession() just run");
      }
    }  
    else { 
      res = MRMLQueryStep(c);
      picsom->PossiblyShowDebugInformation("MRMLQueryStep() just run");
    }

    c->DeleteXMLdoc();

    Tac("MRMLSinglePass");

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MRMLOpenSession(Connection *c) {
    Tic("MRMLOpenSession");

    // Create XML-document for opening a new MRML session into MRML server //
    xmlDocPtr  doc2  = xmlNewDoc((XMLS)"1.0");
    xmlNsPtr   ns2   = NULL;

    xmlNodePtr root2 = NewDocNode(doc2, ns2, "mrml");
    xmlDocSetRootElement(doc2, root2);
    xmlNewProp(root2, (XMLS)"session-id", (XMLS)"just_something");      

    xmlNodePtr ops = xmlNewChild(root2, ns2, (XMLS)"open-session", NULL);
    xmlNewProp(ops, (XMLS)"user-name", (XMLS)"PicSOM-Analysis");      
    xmlNewProp(ops, (XMLS)"session-name", (XMLS)"new-session");      
  

    // Perform write and read operations //
    const char *dump = c->XML2StringM(doc2, true);
    bool resdoc_ok = c->ReOpenUplink(dump);

    xmlFreeDoc(doc2); // added 250203 by mrk
    delete dump;

    if (!resdoc_ok) {
      Tac("MRMLOpenSession");
      return ShowError("Empty resdoc");
    }

    xmlDocPtr resdoc = c->XMLdoc();

    // Begin MRML response message parsing //

    // first assumption, move this to correct place later
    mrmlserver_repeats_oldimages = false;

    // read and store received session-id from HEADER for next connections!
    xmlNodePtr root = xmlDocGetRootElement(resdoc);
    if(root->name)
      if(xmlStrcmp(root->name, (XMLS)"mrml")) {
        Tac("MRMLOpenSession");
        return ShowError("Query::MRMLOpenSession: Result is not MRML!!");
      }

    if(root->properties && root->properties->name) {
      xmlAttrPtr tmp = root->properties;
    
      while(tmp) {
        if(!xmlStrcmp(tmp->name, (XMLS)"session-id")) {
          if(tmp->children && tmp->children->content)
            c->setSessionID( (string)(const char *)tmp->children->content );
        }
        else if(!xmlStrcmp(tmp->name, (XMLS)"text")) {
          //no CR/LF printing
        }
        else if(!xmlStrcmp(tmp->name, (XMLS)"just-for-test")) {
          //identify GIFT on this basis
          mrmlserver_repeats_oldimages = true;
        }
        else {
          ShowError("Query::MRMLOpenSession: Unknown parameter:\n",
                    (const char *)tmp->name);
        }
        tmp = tmp->next;
      }
    }

    xmlNodePtr anynode = root->children;
    while(anynode) {
      if(!xmlStrcmp(anynode->name, (XMLS)"acknowledge-session-op")) {
        printf("Query::MRMLOpenSession: Open-session operation acknowledged");
      }
      else if(!xmlStrcmp(anynode->name, (XMLS)"text")) {
        //printf("%s", (const char *)anynode->content);
      }
      else if(!xmlStrcmp(anynode->name, (XMLS)"cui-time-stamp")) {
        //printf("%s", (const char *)anynode->content);
      }
      else {
        ShowError("Query::MRMLOpenSession: Unknown parameter:\n",
                  (const char *)anynode->name);
      }
      anynode = anynode->next;
    }

    if(!c->getSessionID()) {
      Tac("MRMLOpenSession");
      return ShowError("Query::MRMLOpenSession: NO Session-ID found!!");
    }

    Tac("MRMLOpenSession");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MRMLGetCollections(Connection *c) {
    Tic("MRMLGetCollections");

    //create configure-session mrml message
    xmlDocPtr  doc  = xmlNewDoc((XMLS)"1.0");
    xmlNsPtr   ns   = NULL;

    xmlNodePtr root = NewDocNode(doc, ns, "mrml");
    xmlDocSetRootElement(doc, root);
    xmlNewProp(root, (XMLS)"session-id", c->getSessionID());      
    xmlNewChild(root, ns, (XMLS)"get-collections", NULL);

    // dump message
    const char *dump = c->XML2StringM(doc, true);
    bool resdoc_ok = c->ReOpenUplink( dump );

    xmlFreeDoc(doc); // added 250203 by mrk
    delete dump;

    if (!resdoc_ok) {
      Tac("MRMLGetCollections");
      return false;
    }

    xmlDocPtr resdoc = c->XMLdoc();

    //parse result mrml-message

    root = xmlDocGetRootElement(resdoc);
    xmlNodePtr anynode = NULL;

    if(root->name)
      if(xmlStrcmp(root->name, (XMLS)"mrml")) {
        Tac("MRMLGetCollections");
        return ShowError("Query::MRMLGetCollections: Result is not MRML!");
      }
    if(root->properties && root->properties->name) {
      if(!xmlStrcmp(root->properties->name, (XMLS)"session-id")) {
        printf("Good. Root has session-id.\n");
      }
    }

    if(!root->children) {
      Tac("MRMLGetCollections");
      return ShowError("Query::MRMLGetCollections: Root has no children!");
    }

    //const char *cid=NULL, *cname=NULL;
    string cid, cname;
    anynode = root->children;
    while(anynode) {
      if(!xmlStrcmp(anynode->name, (XMLS)"collection-list")) {

        xmlNodePtr coll = anynode->children;
        while(coll) {
          if(!xmlStrcmp(coll->name, (XMLS)"collection")) {

            xmlAttrPtr catr = coll->properties;
            while(catr) {
              if(!xmlStrcmp(catr->name, (XMLS)"collection-id")) {
                if(catr->children && catr->children->content)
                  cid =(string)(const char *)catr->children->content;
              }
              else if(!xmlStrcmp(catr->name, (XMLS)"collection-name")) {
                if(catr->children && catr->children->content)
                  cname =(string)(const char *)catr->children->content;
              }
              else if(!xmlStrcmp(catr->name, (XMLS)"text")) {
                //printf("%s", (const char *)catr->children->content);
              }
              else {
                // do nothing for the rest of the attributes
              }
              catr = catr->next;
            }
            //tahan cnamen vertailu valitun databasen nimeen & cid:n talletus!
            //if( !strcmp( cname, GetDataBase()->Name())  && cid!=NULL ) {
            if (cname==GetDataBase()->Name()&&cid.length()!=0){
              setCollectionID( cid.c_str() );
            }
                
          }
          else if(!xmlStrcmp(coll->name, (XMLS)"text")) {
            //printf("%s", (const char *)coll->content);
          }
          else {
            // do nothing for unrecognized nodes, since there will be many..
          }
          coll = coll->next;
        }

      }
      anynode = anynode->next;
    }
    if( !getCollectionID() ) {
      Tac("MRMLGetCollections");
      return ShowError("Query::MRMLGetCollections:No Collection found!");
    }

    Tac("MRMLGetCollections");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MRMLGetAlgorithms(Connection *c) {
    Tic("MRMLGetAlgorithms");

    //create configure-session mrml message
    xmlDocPtr  doc  = xmlNewDoc((XMLS)"1.0");
    xmlNsPtr   ns   = NULL;

    xmlNodePtr root = NewDocNode(doc, ns, "mrml");
    xmlDocSetRootElement(doc, root);
    xmlNewProp(root, (XMLS)"session-id", c->getSessionID());      
    xmlNewChild(root, ns, (XMLS)"get-algorithms", NULL);


    // dump message
    const char *dump = c->XML2StringM(doc, true);
    bool resdoc_ok = c->ReOpenUplink( dump );

    xmlFreeDoc(doc); // added 250203 by mrk
    delete dump;
 
    if (!resdoc_ok) {
      Tac("MRMLGetAlgorithms");
      return false;
    }

    xmlDocPtr resdoc = c->XMLdoc();

    //parse result mrml-message
    root = xmlDocGetRootElement(resdoc);
    xmlNodePtr anynode = NULL;

    if(root->name)
      if(xmlStrcmp(root->name, (XMLS)"mrml")) {
        Tac("MRMLGetAlgorithms");
        return ShowError("Query::MRMLGetAlgorithms: Result is not MRML!");
      }
    if(root->properties && root->properties->name) {
      if(!xmlStrcmp(root->properties->name, (XMLS)"session-id")) {
        printf("Good. Root has session-id.\n");
      }
    }

    if(!root->children) {
      Tac("MRMLGetAlgorithms");
      return ShowError("Query::MRMLGetAlgorithms: Root has no children!");
    }

    string aid, aname;
    anynode = root->children;
    while(anynode) {
      if(!xmlStrcmp(anynode->name, (XMLS)"algorithm-list")) {

        xmlNodePtr algo = anynode->children;
        while(algo) {
          if(!xmlStrcmp(algo->name, (XMLS)"algorithm")) {

            xmlAttrPtr aatr = algo->properties;
            while(aatr) {
              if(!xmlStrcmp(aatr->name, (XMLS)"algorithm-id")) {
                if(aatr->children && aatr->children->content)
                  aid = (string)(const char *)aatr->children->content;
                //cout << "### : " << aid << endl;
              }
              else if(!xmlStrcmp(aatr->name, (XMLS)"algorithm-name")) {
                if(aatr->children && aatr->children->content)
                  aname = (string)(const char *)aatr->children->content;
                //cout << "### : " << aname << endl;
              }
              else if(!xmlStrcmp(aatr->name, (XMLS)"text")) {
                //printf("%s", (const char *)aatr->children->content);
              }
              else {
                // do nothing for the rest of the attributes
              }
              aatr = aatr->next;

            }//while(aatr)                  
            const char *current_alg = picsom->CbirAlgorithmP(Algorithm());
            if( !strcmp( current_alg, "external" ) ) {
              if( aid.length()!=0 && aname.length()!=0 && 
                  external_algorithm==aname ) {
                //cout<<"#####ASETETTU: aname:aid = "<<aname<<" : "<<aid<<endl;
                setAlgorithmID( aid.c_str() );
              }
            }
            else if( aid.length()!=0 && aname.length()!=0 && 
                     strncmp( current_alg, aname.c_str(), 
                              strlen(current_alg)>aname.length()? 
                              strlen(current_alg) : aname.length() ) ==0 ) {
              //cout << "#### : " <<  picsom->CbirAlgorithm(Algorithm())
              //<<" ---aname: "<< aname << ":"<<aid<< endl;
              //if( !xmlStrcmp(algo->children->children->name, 
              //(XMLS)"query-paradigm") && 
              //!xmlStrcmp(algo->children->children->properties
              //->children->content, getCollectionID()) ) {
              setAlgorithmID( aid.substr(0,aid.find("-")).c_str() );
              //cout<< "#####ASETETTU: aname:aid = "<<aname<<" : "<<aid<<endl;
              //}            
            }
                
          }
          else if(!xmlStrcmp(algo->name, (XMLS)"text")) {
            //printf("%s", (const char *)algo->content);
          }
          else {
            // do nothing for unrecognized nodes, since there will be many..
          }
          algo = algo->next;
        }//while(algo)

      }
      anynode = anynode->next;
    }

    if( !getAlgorithmID() ) {
      Tac("MRMLGetAlgorithms");
      return ShowError("Query::MRMLGetAlgorithms:No Algorithm found frm list!");
    }

    Tac("MRMLGetAlgorithms");

    return true;
  }


  /////////////////////////////////////////////////////////////////////////////

  bool Query::MRMLConfigureSession(Connection *c) {
    Tic("MRMLConfigureSession");

    //create configure-session mrml message
    xmlDocPtr  doc  = xmlNewDoc((XMLS)"1.0");
    xmlNsPtr   ns   = NULL;

    xmlNodePtr root = NewDocNode(doc, ns, "mrml");
    xmlDocSetRootElement(doc, root);
    xmlNewProp(root, (XMLS)"session-id", c->getSessionID());      

    xmlNodePtr cfg = xmlNewChild(root, ns, (XMLS)"configure-session", NULL);
    xmlNewProp(cfg, (XMLS)"session-id", c->getSessionID());      
  
    xmlNodePtr alg = xmlNewChild(cfg, ns, (XMLS)"algorithm", NULL);
    //(XMLS) picsom->CbirAlgorithm(Algorithm())
    xmlNewProp(alg, (XMLS)"algorithm-id",   getAlgorithmID() );  
    xmlNewProp(alg, (XMLS)"algorithm-type", getAlgorithmID() );      
    xmlNewProp(alg, (XMLS)"collection-id",  getCollectionID() );      

    if(!mrmlserver_repeats_oldimages) {    
      int lpos = 0; 
      int rpos = listoffeatures.find(",");
      string feat;

      while (lpos != rpos && rpos != -1) {
        rpos = rpos + lpos;
        feat = listoffeatures.substr(lpos, rpos);
        xmlNewProp(alg, (XMLS)feat.c_str(), (XMLS)"yes");
        lpos = rpos+1;
        rpos = listoffeatures.substr(lpos,listoffeatures.size()).find(",");
      } 
      feat = listoffeatures.substr(lpos, listoffeatures.size());
      xmlNewProp(alg, (XMLS)feat.c_str(), (XMLS)"yes");
    }

    if (!OnlyForcedObjects(StageArgs(stage_initial_set))) {
      xmlFreeDoc(doc); // added 250203 by mrk
      Tac("MRMLConfigureSession");
      return ShowError("Query::MRMLConfigureSession OnlyForcedImages()==FALSE");
    }

    // dump message
    const char *dump = c->XML2StringM(doc, true);
    bool resdoc_ok = c->ReOpenUplink( dump );

    xmlFreeDoc(doc); // added 250203 by mrk
    delete dump;

    if (!resdoc_ok) {
      Tac("MRMLConfigureSession");
      return false;
    }

    xmlDocPtr resdoc = c->XMLdoc();

    //parse result mrml-message
    root = xmlDocGetRootElement(resdoc);
    xmlNodePtr unknown = NULL;

    if(root->name)
      if(xmlStrcmp(root->name, (XMLS)"mrml")) {
        Tac("MRMLConfigureSession");
        return ShowError("Not MRML content!");
      }
    if(root->properties && root->properties->name) {
      if(!xmlStrcmp(root->properties->name, (XMLS)"session-id")) {
        printf("Good. Root has session-id.\n");
      }
    }

    if(root->children)
      unknown = root->children;

    while(unknown) {
      if(unknown->name) {
        if(!xmlStrcmp(unknown->name, (XMLS)"acknowledge-session-op")) {
          printf("Query::MRMLConfigureSession: ack session - OK\n");
        }
        else if(!xmlStrcmp(unknown->name, (XMLS)"text")) {
          //printf("%s\n",unknown->content);
        }
        else if(!xmlStrcmp(unknown->name, (XMLS)"cui-time-stamp")) {
          // just process this as well
          //printf("%s\n",unknown->children->content);
        }
        else printf("Query::MRMLConfigureSession: unknown tag found: %s\n", 
                    unknown->name);
      }
      else { // unknown has no unknown->name
        Tac("MRMLConfigureSession");
        return ShowError("Query::MRMLConfigureSession: empty MRML message!");
      }
      unknown = unknown->next;
    }//while(unknown)
    
    Tac("MRMLConfigureSession");
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MRMLQueryStep(Connection *c) {
    Tic("MRMLQueryStep");

    //create query-step mrml message
    xmlDocPtr  doc  = xmlNewDoc((XMLS)"1.0");
    xmlNsPtr   ns   = NULL;

    xmlNodePtr root = NewDocNode(doc, ns, "mrml");
    xmlDocSetRootElement(doc, root);
    xmlNewProp(root, (XMLS)"session-id", c->getSessionID());      

    xmlNodePtr qst = xmlNewChild(root, ns, (XMLS)"query-step", NULL);

    xmlNewProp(qst, (XMLS)"session-id", c->getSessionID());      
    int nimgs = mrmlserver_repeats_oldimages ? 
      MaxQuestions()+NseenObjects() : MaxQuestions();

    xmlNewProp(qst, (XMLS)"result-size", Int2XMLS(nimgs) );  
    xmlNewProp(qst, (XMLS)"result-cutoff", (XMLS)"0");   //=?
    //const char *a = picsom->CbirAlgorithm( Algorithm() );
    xmlNewProp(qst, (XMLS)"algorithm-id", getAlgorithmID() );
    xmlNewProp(qst, (XMLS)"collection-id",getCollectionID() );
  
    xmlNodePtr url =
      xmlNewChild(qst, ns, (XMLS)"user-relevance-element-list", NULL);

    for (size_t n=0; n<NseenObjects(); n++) {

      xmlNodePtr ure = 
        xmlNewChild(url, ns, (XMLS)"user-relevance-element", NULL);
      if(ure==NULL) {
        Tac("MRMLQueryStep");
        return ShowError("Query::MRMLQueryStep xmlNewChild");
      }

      const char *l = SeenObject(n).LabelP();
      if(l==NULL) {
        Tac("MRMLQueryStep");
        return ShowError("Query::MRMLQueryStep: NULL Label!");
      }

      /* For thumbnail-location, if needed
         string op = db->SolveObjectPath(l, "tn-120x90");
         char tnobj[1000];
         sprintf(tnobj, "file:%s", op.c_str());
      */

      string op = GetDataBase()->SolveObjectPath(l);
      if (op=="") {
        Tac("MRMLQueryStep");
        return ShowError("Query::MRMLQueryStep: NULL Path!");
      }

      char imgobj[1000]="";
      sprintf(imgobj, "file:%s", op.c_str());
    
      /*
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
        string tmptmptmp = GetDataBase()->ImgHTTPpath(l);
        cout << "################### "<< tmptmptmp <<endl;
    
        xmlNewProp(ure, (XMLS)"image-location", tmptmptmp );
      */
      xmlNewProp(ure, (XMLS)"image-location", (XMLS)imgobj );
      xmlNewProp(ure, (XMLS)"thumbnail-location", (XMLS)"");      
      xmlNewProp(ure, (XMLS)"user-relevance", 
                 Int2XMLS((int)SeenObject(n).Value()) );
      //cout<<"#@@@@@@@@@@@@:"<< Int2XMLS((int)SeenImage(n).Value()) << endl;
    }


    //dump message
    const char *dump = c->XML2StringM(doc, true);
    bool resdoc_ok = c->ReOpenUplink( dump );
    delete dump;
    xmlFreeDoc(doc); // added 250203 by mrk

    if (!resdoc_ok) {
      Tac("MRMLQueryStep");
      return false;
    }

    xmlDocPtr resdoc = c->XMLdoc();

    const char *imgfname=NULL;
    float val;

    root = xmlDocGetRootElement(resdoc);
    xmlNodePtr unknown     = NULL;
    xmlNodePtr elementlist = NULL;
    xmlNodePtr listitem    = NULL;
    xmlAttrPtr qreattrib   = NULL;

    if(root->name)
      if(xmlStrcmp(root->name, (XMLS)"mrml")) {
        Tac("MRMLQueryStep");
        return ShowError("Query::MRMLQueryStep: Result is not MRML!!");
      }

    if(root->properties && root->properties->name) 
      if(!xmlStrcmp(root->properties->name, (XMLS)"session-id")) {
        printf("Root has session-id: %s attached. Good. \n",
               (const char *)root->properties->children->content);
      }

    if(!root->children) {
      Tac("MRMLQueryStep");
      return ShowError("Query::MRMLQueryStep: XMLdoc root has no children!");
    }

    // remove N 1st images from the list. ??GIFT vs PicSOM toteutus.
    //int removeNimgs = NseenImages();

    int imgcounter=0;
    int totcounter=0;

    unknown = root->children;
    while(unknown) {
      if(!unknown->name) {
        Tac("MRMLQueryStep");
        return ShowError("Query::QueryStep: Empty MRML tag!");
      }

      if(!xmlStrcmp(unknown->name, (XMLS)"query-result")) {        
        if(!unknown->children) {
          Tac("MRMLQueryStep");
          return ShowError("Query::MRMLQueryStep: Empty query-result!");
        }

        elementlist = unknown->children;
        while(elementlist) {

          if(!xmlStrcmp(elementlist->name, 
                        (XMLS)"query-result-element-list")) {
            listitem = elementlist->children;
          
            while(listitem) {
              if(listitem->name) {
                if(!xmlStrcmp(listitem->name, (XMLS)"query-result-element")) {

                  if(listitem->properties)
                    qreattrib = listitem->properties;

                  while(qreattrib) {
                    if(qreattrib->name && qreattrib->children->content) {
                      if(!xmlStrcmp(qreattrib->name, (XMLS)"image-location")) {
                        //printf("!!!!!!!!!!!!! IMAGE-LOCATION:  %s\n",
                        //(const char *)qreattrib->children->content);
                        imgfname = (const char *)qreattrib->children->content;
                      }                
                      else if(!xmlStrcmp(qreattrib->name, 
                                         (XMLS)"calculated-similarity")) {
                        //printf("!!!!!!!!!!!!! CALCULATED SIMILARITY:  %s\n",
                        //(const char *)qreattrib->children->content);
                        val = atof((const char *)qreattrib->children->content);
                      }
                      else if(!xmlStrcmp(qreattrib->name, (XMLS)"text")) {
                        //printf("%s\n",qreattrib->children->content);
                      }
                      else if(!xmlStrcmp(qreattrib->name, 
                                         (XMLS)"thumbnail-location")) {
                        // just process thumbnail-location
                      }
                      else if(!xmlStrcmp(qreattrib->name, 
                                         (XMLS)"cui-time-stamp")) {
                        // just process cui-time-stamp
                      }
                      else {
                        printf("Query::MRMLQueryStep: Unknown MRML attribute:%s",
                               (const char *)qreattrib->name);
                
                      }
                    }
                    qreattrib = qreattrib->next;
                  }//while(qreattrib)


                  //printf("#### imgf ja val %s %f\n",imgfname,val);
                  if(!imgfname || !val) {
                    Tac("MRMLQueryStep");
                    return ShowError("Query::MRMLQueryStep: No Image found!");
                  }
                  else {
                    int idx = -1;
                    const char *fname = NULL;
                    char filename[255]="";

                    if( GetDataBase()->IsLabel(imgfname) ) {
                      fname = imgfname;
                    }
                    else {
                      // 1st try to find picsom-filename from hash ( HTTP-path )
                      const string& fnamestr
                        = GetDataBase()->FindFnameFromHash(imgfname);
                      fname = fnamestr.c_str();
                    }

                    // in case we did not find it 
                    // assume we got direct reference (path+filename)
                    if( !fname || *fname=='\0' ) {
                      // test if we got  picsom-filename (8digits) straight away
                      if( GetDataBase()->IsLabel(imgfname) ) {
                        fname = imgfname;
                      }
                      else {      
                        unsigned int pos = 0, i = 0;
                        // extracting only filename part from full path 
                        // (from last '/' until next '.')
                        do {   
                          if(*(imgfname+i) == '/')
                            pos = i;
                          i++;
                        } while(i < strlen(imgfname) );
                        i = pos; 
                        while( *(imgfname+i) != '.' ) {
                          if( i >= strlen(imgfname)) break;
                          i++;
                        } 
                        pos++; // pos points to beg. of filename, i to "."      
                        strncpy(&filename[0], (imgfname+pos), i-pos);
                        fname = filename;
                      }
                      //cout << "Found filename: " << fname << endl;
                    }
                
                    //cout << "Finally. Got filename: " << fname << endl;
                    // finally, get the index.
                    totcounter++;
                    idx = LabelIndex(fname);
                    if(idx==-1) {
                      cout << "What's wrong with:"<<endl<<"### "<<imgfname<<" ###"
                           <<endl<<" or "<<" ##"<<fname<<"## "<<endl;
                      Tac("MRMLQueryStep");
                      return ShowError("Query::MRMLQueryStep: IDX=-1!");
                    }
                
                    if (int(NnewObjects())<MaxQuestions() &&
                        CanBeShownRestricted(idx, false) 
                        && imgcounter<=MaxQuestions()) {

                      // obs! this doesn't handle aspects via AspectsFromIndices();
                      AppendNewObject(Object(the_db,idx,select_question,val));

                      imgcounter++;
                      printf("%s    %d     %f    %d\n",
                             fname,idx,val,(int)NnewObjects());
                    }
                  }
              
                }//if query-result-element
                else if(!xmlStrcmp(listitem->name, (XMLS)"text")) {
                  //printf("%s", listitem->content);
                }
                else printf("Query::MRMLQueryStep: Unknown MRML attribute:\n%s", 
                            listitem->name);
              }//if(listitem->name)
            
              listitem = listitem->next;
            }//while(listitem)
          
          
          }//if query-result-element-list
          else if(!xmlStrcmp(elementlist->name, (XMLS)"text")) {
            //printf("%s", elementlist->content);
          }
          else printf("Query::MRMLQueryStep: Unknown MRML attribute:\n%s\n", 
                      elementlist->name);
      
          elementlist = elementlist->next;
        }//while(elementlist)


      }//if query-result
      else if(!xmlStrcmp(unknown->name, (XMLS)"text")) {
        //printf("%s", unknown->content);
      }
      else if(!xmlStrcmp(unknown->name, (XMLS)"cui-time-stamp")) {
        //printf("%s", unknown->content);
      }
      else 
        printf("Query::MRMLQueryStep: Unknown MRML attribute found: \n%s\n", 
               unknown->name);
    
      unknown = unknown->next;
    }//while(unknown)
    //cout << "################### GIFTILTA saatuja: uudet/kaikki= " << 
    //imgcounter << " / " <<   totcounter << endl << "##################
    //###########################################################" << endl 
    //<< endl << endl ;

    if (debug_lists)
      ShowUnitAndObjectLists(true,
                             false, false, false, false, false, false,
                             true, true);

    Tac("MRMLQueryStep");

    return true;
  }

#endif //USE_MRML

  /////////////////////////////////////////////////////////////////////////////

  string Query::ListHead(const string& txt, const string& fns,
                         int m, int l) const {
    char line[1000];
    strcpy(line, txt.c_str());
    if (m>=0 && l>=0) 
      sprintf(line+strlen(line), "(%d,%d):", m, l);

    sprintf(line+strlen(line), " query_target=%s",
            TargetTypeString(Target()).c_str());
    sprintf(line+strlen(line), " round=%d", round);

    if (fns!="")
      sprintf(line+strlen(line), " %s[%d]", fns.c_str(), l);
    if (m>=0) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      string fts = IndexStaticData(NULL, m).FeatureTargetString();
      sprintf(line+strlen(line), " feature_target=%s", fts.c_str());
    }

    if (m>=0 && l>=0 && IsTsSomAndLevelOK(m, l)) {
      const TSSOM::State& fd = IndexDataTSSOM(m);

      if (fd.aspects.size())
        sprintf(line+strlen(line), " aspects=[%s]",
                CommaJoin(fd.aspects).c_str());      

      size_t ls = l;
      const float nan = Index::State::nan();
      if (fd.tssom_level_weight[l]!=nan)
        sprintf(line+strlen(line), " weight=%f", fd.tssom_level_weight[l]);
      else
        strcat(line, " weight=NA");
      strcat(line, " entropy=[");
      if (fd.entropy.size()>ls && fd.entropy[l]!=nan)
        sprintf(line+strlen(line), "%f", fd.entropy[l]);
      strcat(line, "/");
      if (fd.entropy_r.size()>ls && fd.entropy_r[l]!=nan)
        sprintf(line+strlen(line), "%f", fd.entropy_r[l]);
      strcat(line, "/");
      if (fd.entropy_p.size()>ls && fd.entropy_p[l]!=nan)
        sprintf(line+strlen(line), "%f", fd.entropy_p[l]);
      strcat(line, "/");
      if (fd.entropy_n.size()>ls && fd.entropy_n[l]!=nan)
        sprintf(line+strlen(line), "%f", fd.entropy_n[l]);
      strcat(line, "]");
    }

    strcat(line, " ---");

    return line;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CbirStages() {
    bool trace = true;

    const string h = "Query::CbirStages() : ";

    if (trace)
      picsom->PossiblyShowDebugInformation("CbirStages() starting");

    if (Target()==target_no_target) {
      Target(CheckDB()->SolveDefaultTargetType());
      if (!forcecbirstages) // a temporal hack...
        ShowError(h+"target not specified, defaults to ",
                  TargetTypeString(Target()));
    }

    bool sloppy = true;
    if (!ConditionallyCheckObjectIndices(sloppy)) {
      ShowUnitAndObjectLists();
      return ShowError(h+"failing due to object list disintegrity");
    }

    Tic("CbirStages");

    bool ok = true;

    if (CalculateEntropy() && DefaultMatrixCount()<3) {
      ShowError(h+"setting matrixcount=3");
      DefaultMatrixCount(3);
    }

    ProcessTextQuery();

    CreateIndexData(false);

    if (textquery!="")
      for (size_t i=0; i<Nindices(); i++)
        if (IsWordInverse(i) && wordInverse(i).IsDynamicBinaryTextFeature())
          wordInverse(i).
            UpdateDynamicBinaryTextFeature(TextQueryProcessed(), 
                                           IndexDataWordInverse(i).binary_data);

    if (debug_parameters) {
      string footmp;
      WriteAnalyseVariablesOld("???", -1, NULL, NULL, 0, false, footmp);
    }

    if (store_selfinfluence) {
      selfinfluence.clear();
      location_selfinfluence=vector<vector<vector<float> > >(Nindices());
    }

    cbir_stage stage = stage_first;

    if (NseenObjects() ||
        StageFunc(stage_initial_set)!=func_only_forced_images) { 

      if (algorithm!=cbir_no_algorithm &&
	  StageRestrict(StageFunction(stage_restrict),
			StageArgs(stage_restrict))!=stage_enter)
	ok = ShowError(h+"stage_restrict failed with new algorithms");

      for (size_t j=0; j<algorithms.size(); j++) {
        size_t i = algorithms.size()-1-j;
        if (CbirAlgorithm::DebugStages())
          cout << h << "cbir_classbased: " << algorithms[i].fullname << endl;
        algorithms[i].result
          = algorithms[i].algorithm->CbirRound(algorithms[i].data, seenobject,
                                               MaxQuestions());
	if (debug_lists>1)
	  DumpList(algorithms[i].result, "After "+algorithms[i].fullname);
      }
      if (!algorithms.empty()) {
        newobject = algorithms[0].result;
        stage = stage_ready;
      }
    }

    if (trace)
      picsom->PossiblyShowDebugInformation("CbirStages() entering stage loop");

    for (; ok; ) {
      PicSOM::ThreadTestCancel();

      if (algorithm==cbir_no_algorithm)
	break;

      if (debug_stages)
        cout << "CBIR stage: " << StageNameP(stage) << " = "
             << FunctionNameP(StageFunction(stage, false, false, false))
             << " approaching switch () " << endl;

      if (stage==stage_ready)
        break;

      cbir_stage old_stage = stage;
      cbir_function func = StageFunction(stage);
      const string& args = StageArgs(stage);
  
      string msg = string("stage=<")+StageNameP(stage)+"> func=<"
        +FunctionNameP(func)+">";

      if (func==func_error)
        ok = ShowError(h+"func==func_error in "+msg);

      if (func==func_unknown)
        ok = ShowError(h+"func==func_unknown in "+msg);

      if (ok)
        switch (stage) {
        case stage_restrict:
          stage = StageRestrict(func, args);
          break;

        case stage_enter:
          stage = StageEnter(func, args);
          break;

        case stage_initial_set:
          stage = StageInitialSet(func, args);
          break;

        case stage_check_image_count:
          stage = StageCheckNimages(func, args);  // obs! naming differs...
          break;

        case stage_no_positives:
          stage = StageNoPositives(func, args);
          break;

        case stage_has_positives:
          stage = StageHasPositives(func, args);
          break;

        case stage_special_first_round:
          stage = StageSpecialFirstRound(func, args);
          break;

        case stage_expand_relevance:
          stage = StageExpandRelevance(func, args);
          break;

        case stage_run_per_map:
          stage = StageRunPerMap(func, args);
          break;

        case stage_set_map_weights:
          stage = StageSetMapWeights(func, args);
          break;

        case stage_combine_unit_lists:
          stage = StageCombineUnitLists(func, args);
          break;

        case stage_extract_images:
          stage = StageExtractObjects(func, args);
          break;

        case stage_combine_image_lists:
          stage = StageCombineObjectLists(func, args);
          break;

        case stage_remove_duplicates:
          stage = StageRemoveDuplicates(func, args);
          break;

        case stage_exchange_relevance:
          stage = StageExchangeRelevance(func, args);
          break;

        case stage_select_images_to_process:
          stage = StageSelectObjectsToProcess(func, args);
          break;

        case stage_process_images:
          stage = StageProcessObjects(func, args);
          break;

        case stage_converge_relevance:
          stage = StageConvergeRelevance(func, args);
          break;

        case stage_final_select:
          stage = StageFinalSelect(func, args);
          break;

        case stage_error:
          ok = ShowError(h+"stage_error in switch () after "+msg);
          break;

        default:
          ok = ShowError(h+"default in switch () after "+msg);
        }

      if (debug_stages) {
        stringstream ss;
        ss << " seen: " << NseenObjects() << " comb: " << combinedimage.size()
           << " uniq: " << uniqueimage.size() << "/" << NuniqueRetained()
           << " new: "  << NnewObjects() << "/" << NnewRetained() << "/"
           << NnewObjectsRetainedNonShow();
        string listinfo = ss.str();
        cout << "CBIR stage: " << StageNameP(old_stage) << " = "
             << FunctionNameP(StageFunction(old_stage, false, false, false))
             << " exited switch () with ok=" << ok
             << " next stage=" << StageNameP(stage) << listinfo << endl;
      }

      if (trace)
	picsom->PossiblyShowDebugInformation(string("In CbirStages() ")+
					     StageNameP(old_stage)+" ended");

      if (ok && stage==stage_error)
        ok = ShowError(h+"stage_error resulted after "+msg);

      if (ok && stage==old_stage)
        ok = ShowError(h+"stage==old_stage after "+msg);

      if (ok && !ConditionallyCheckObjectIndices(sloppy))
        ok = ShowError(h+"object list disintegrity after "+msg);
    }

    int rank = NseenObjects();
    for (size_t i=0; i<NnewObjects(); i++)
      if (NewObject(i).Retained()) {
        NewObject(i).Rank(rank++);
        NewObject(i).Round(round);
      }

    if (!ok || (debug_lists &&
        (algorithms.empty() ||
         StageFunc(stage_initial_set)==func_only_forced_images))) {
      bool sseen = true, sother = debug_lists>2 || !ok;
      ShowUnitAndObjectLists(sseen, sother, sother, sother, sother,
                             sother, sother, sother, sother);
    }

    if (matlabdump!="")
      DoDump(0, matlabdump);

    if (datdump!="")
      DoDump(1, datdump);

    if (debug_selective) {
      cout << "dumping newobject.rel_distribution";

      for (size_t i=0; i<NnewObjects(); i++)
        if (newobject[i].Retained())
          newobject[i].rel_distribution.dump();
    }

    if (classifymethod!="" || Classify())
      DoClassification();

    Tac("CbirStages");

    picsom->PossiblyShowDebugInformation("CbirStages() ending");

    if (HasParentAnalysis())
      GetParentAnalysis()->CbirStagesEnded(this);

    return ok;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageRestrict(cbir_function func, const string& args) {
    bool ok = false;

    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_restriction_spec:
      ok = RestrictionSpec(args);
      break;

    default:
      ShowError("Query::StageRestrict(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_enter : stage_error;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  bool Query::RestrictionSpec(const string& spec) {
    RemoveTemporalRestriction();

    if (debug_restriction)
      cout << "  starting to process [" << spec << "]" << endl;

    vector<string> a = SplitInCommas(spec);
    for (size_t i=0; i<a.size(); i++) {
      vector<string> p = SplitInSomething(":", false, a[i]);
      if (p.size()>2)
        return ShowError("Query::RestrictionSpec() failed with ["+a[i]+"] A");

      const string& gts = p.back();
      bool now = true;
      if (p.size()==2) {
        vector<string> t = SplitInSomething("-", false, p[0]);
        if (t.size()!=2)
          return ShowError("Query::RestrictionSpec() failed with ["+a[i]+"] B");

        int first = t[0]!="" ? atoi(t[0].c_str()) : 0;
        int last  = t[1]!="" ? atoi(t[1].c_str()) : MAXINT;
        now = round>=first && round<=last;
        if (debug_restriction)
          cout << "    round=" << round << " first=" << first
               << " last=" << last << endl;
      }
      if (debug_restriction)
        cout << "    " << (now?"":"NOT ") << "setting restriction by " << a[i]
             << endl;
      if (now) {
        SetTemporalRestriction(gts, Target(), -1);

        if (restriction.NumberOfEqual(1)<=round*MaxQuestions())
          RemoveTemporalRestriction();
        break;
      }
    }

    if (debug_restriction)
      cout << "  ready processing [" << spec << "]" << endl;

    return true;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageEnter(cbir_function func,
                                       const string& /*args*/) {
    if (func==func_goto_has_positives)
      return stage_has_positives;

    if (func!=func_default) {
      ShowError("Query::StageEnter(): operation for <",
                FunctionNameP(func), "> undefined");
      return stage_error;
    }
    
    if (!NseenObjects())
      return stage_initial_set;

    if (!NpositiveSeenObjects())
      return stage_no_positives;

    target_type tt = target_any_target; // obs! added when type was required...
    if (NseenObjects()<DataBaseRestrictionSize(tt)+RestrictedObjectsSeen() ||
        CanShowSeen())
      return stage_has_positives;

    return stage_ready;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageInitialSet(cbir_function func,
                                            const string& args) {
    switch (func) {
    case func_only_forced_images:
    case func_only_forced_images_user_test:
      return OnlyForcedObjects(args) ? stage_ready : stage_error;

    case func_top_levels_by_entropy:
      TopLevelsByEntropy();
      return stage_check_image_count;

    case func_random_unseen_if_no_classmodels:
      // classaugment is not empty even if there are no classmodels:
      if (!classaugment.empty())
	for (classaugment_iter_t it=classaugment.begin(); 
	     it != classaugment.end(); it++) {
	  const string& classn = it->first.first;
	  const ground_truth *gt = the_db->ContentsFind(classn);
	  if (gt && gt->positives())
	    return stage_has_positives;		  
	}
      
    case func_random_unseen_images:
      RandomUnseenObjects(true, false);
      return stage_ready;

    case func_random_positive_image:
      RandomUnseenObjects(true, true);
      return stage_expand_relevance;

    case func_select_from_pre:
      SelectFromPre();
      return stage_check_image_count;

    default:
      ShowError("Query::StageInitialSet(): operation for <",
                FunctionNameP(func), "> undefined");
      return stage_error;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageCheckNimages(cbir_function func,
                                              const string&) {
    if (NnewObjects()==MaxQuestions())
      return stage_ready;
  
    switch (func) {
    case func_top_levels_by_entropy:    
      ShowError("Query::StageCheckNimages(): operation for <",
                FunctionNameP(func), "> not yet defined");
      return stage_error;
      //entropy_level++;
      //TopLevelsByEntropy();
      //return stage_check_nimages;
    
    case func_random_unseen_images:
      RandomUnseenObjects(false, false); // first false disables DeleteNew();
      return stage_ready;
    
    default:
      ShowError("Query::StageCheckNimages(): operation for <",
                FunctionNameP(func), "> undefined");
      return stage_error;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageNoPositives(cbir_function func, const string&) {
    switch (func) {
    case func_goto_has_positives:
      return stage_has_positives;
    
    case func_goto_initial_set:
      return stage_initial_set;

    default:
      ShowError("Query::StageNoPositives(): operation for <",
                FunctionNameP(func), "> undefined");
      return stage_error;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageHasPositives(cbir_function func,
                                              const string&) {
    switch (func) {
    case func_goto_initial_set:
      return stage_initial_set;

    case func_goto_expand_relevance:
      return stage_expand_relevance;

    case func_goto_special_first_round:
      return stage_special_first_round;
      
    default:
      ShowError("Query::StageHasPositives(): operation for <", 
                FunctionNameP(func), "> undefined");
      return stage_error;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageSpecialFirstRound(cbir_function func,
                                                   const string& args) {
    //  if (NseenImages()>NpositiveSeenImages())
    if (NseenObjects()>1)
      return stage_expand_relevance;  // obs! was stage_run_per_map

    WriteLog("This round is handled by StageSpecialFirstRound()");

    bool ok = true;
    switch (func) {

    case func_all_unseen_images:
      ok = ok && AllUnseenObjects();
      break;

    case func_limit_images_with_vq:
      ok = ok && AllUnseenObjects();
      break;

    default:
      ShowError("Query::StageSpecialFirstRound(): operation for <", 
                FunctionNameP(func), "> undefined");
      return stage_error;
    }

    ok = ok && ProcessObjectsVQ(args);

    return ok ? stage_final_select : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::StrVsRelop(string& op, enum relop_type& r, bool setrelop) {
    static struct string_to_relop {
      string long_str, short_str;
      enum relop_type relop;
    } table[] = {
      { "none",    "-",   relop_none },
      { "default", ".",   relop_default },
      { "sum",     "+",   relop_sum },
      { "maximum", "max", relop_max },
      { "average", "avg", relop_avg },
      { "copy",    "=",   relop_copy },
      { "divide",  "div", relop_divide },
      { "divide",  "/",   relop_divide },
      { "",        "",    relop_none }
    };
  
    for (const struct string_to_relop *p = table; p->long_str.size(); p++) {
      if (setrelop) {
        if (p->long_str == op || p->short_str == op) {
          r = p->relop;
          return true;
        }
      } else {
        if (p->relop == r) {
          op = p->long_str;
          return true;
        }
      }
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::StrToRelopStage(const string& s, 
                              struct relop_stage_type& rp) {
    string format_msg = "Arguments should be in the format: "
      "(none|sum|max|avg, none|copy|divide) "
      "where the first parameter is the relevance up operation "
      "and the last is the down operation.";

    if (!s.size())
      return ShowError(format_msg);

    string::size_type commapos = s.find(',');
    if (commapos==string::npos)
      return ShowError(format_msg);
    string op1 = s.substr(0,commapos);
    string op2 = s.substr(commapos+1);
      
    relop_type up = StrToRelop(op1); 
    relop_type down = StrToRelop(op2);

    if (up == relop_default) up = rp.up;
    if (down == relop_default) down = rp.down;

    if (up!=relop_sum && up!=relop_max && up!=relop_avg && up!=relop_none)
      return ShowError("Relevance up operation must be "
                       "sum, max, avg or none.");
    if (down!=relop_copy && down!=relop_divide && down!=relop_none)
      return ShowError("Relevance down operation must be "
                       "copy, divide or none.");

    rp.up = up;
    rp.down = down;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Expand(const string& args) {
    static string oldargs="";
    if (args != oldargs) {
      if (!StrToRelopStage(args,expand_operation)) return false;
      oldargs = args;
      LogRelopStage("Expand relevance  ",expand_operation);
    }

    switch (RelopStageToChoice(expand_operation)) {
    case RELEVANCE_SKIP:
      return true;
    case RELEVANCE_UP:
      return ExpandUp(args);
    case RELEVANCE_DOWN:
      return ExpandDown(args);
    }
    return ExpandUpDown(args);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Exchange(const string& args) {
    static string oldargs="";
    if (args != oldargs) {
      if (!StrToRelopStage(args,exchange_operation)) return false;
      oldargs = args;
      LogRelopStage("Exchange relevance",exchange_operation);
    }

    switch (RelopStageToChoice(exchange_operation)) {
    case RELEVANCE_SKIP:
      return true;
    case RELEVANCE_UP:
      return ExchangeUp(args);
    case RELEVANCE_DOWN:
      return ExchangeDown(args);
    }
    return ExchangeUpDown(args);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Converge(const string& args) {
    static string oldargs="";
    if (args != oldargs) {
      if (!StrToRelopStage(args,converge_operation)) return false;
      oldargs = args;
      LogRelopStage("Converge relevance",converge_operation);
    }

    switch (RelopStageToChoice(converge_operation)) {
    case RELEVANCE_SKIP:
      return true;
    case RELEVANCE_UP:
      return ConvergeUp(args);
    case RELEVANCE_DOWN:
      return ConvergeDown(args);
    }
    return ConvergeUpDown(args);
  }


  /////////////////////////////////////////////////////////////////////////////
  // Relevance going DOWN
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExpandDown(const string& s) {
    bool res = RelevanceDown(s, expand_operation, seenobject, false, true);
    SortSeen();
    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExchangeDown(const string& s) {
    return RelevanceDown(s, exchange_operation, uniqueimage, true, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvergeDown(const string& s) {
    return RelevanceDown(s, converge_operation, newobject, true, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RelevanceDown(const string&, struct relop_stage_type& rs, 
                            ObjectList& il, bool divide, bool seenimg) {
    if (!divide)
      divide = (rs.down == relop_divide);

    size_t nl = il.size();

    map<int,int> idx;
    for (size_t i=0; i<nl; i++)  // only temporary solution, should use ivp 
      idx[il[i].Index()]=i;

    if (seenimg) AllSeenReally();

    ZeroRelevanceCounters(il);

    for (size_t i=0; i<nl; i++) {  
      if(!il[i].Retained()) continue;

      double val = il[i].Value();
   
      if (il[i].hasChildren())
        PushRelevanceDown(i, rs, il, val, idx, divide, seenimg, -1);
    }
                
    SumRelevanceCounters(il/*,seenimg*/);
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::PushRelevanceDown(size_t i, struct relop_stage_type& rs,
                                ObjectList& il, double val, map<int,int> &idx, 
                                bool divide, bool seenimg, int sibling_i) {
    //    if (!val) return;

    //size_t nl = il.size();

    const vector<int>& chldrn = il[i].Children();
    size_t num_chldrn = chldrn.size();

    if (!num_chldrn)
      return;

/*    if (debug_subobjects)
      cout << "PushRelevanceDown(): divide=" << (divide?"true":"false") << endl;*/

    if (divide)
      val = comb_factor*val/num_chldrn;
    else
      val = comb_factor*val;

    for (size_t subi=0; subi<num_chldrn; subi++) {
      int cidx = chldrn[subi];

      // search for child in il list...
      bool found = idx.count(cidx) > 0;

      //for (size_t j=0; j<nl && !found; j++) {
      //  if (il[j].Index()==cidx) {
      //        found = true;
      if(found){
        size_t j=idx[cidx];
        if (sibling_i != -1) {
          if (sibling_i != int(j))
            il[j].SiblingValue(val);
        } else
          il[j].AboveValue(val);
          
        if (sibling_i == -1 || sibling_i != int(j)) {
          if (debug_subobjects)
            cout << "PushRelevanceDown [" << val << "]: " << il[i].DumpStr()
                 << " +> " << il[j].DumpStr() << endl;
          
          PushRelevanceDown(j, rs, il, val, idx,
                            true /*divide???*/, seenimg, -1);
        }
      }
    

      // NOTE: we skip this check in case of expand rel. stage
      if (!seenimg && !CanBeShownRestricted(cidx, false))
        continue;

      if (!found) {
        select_type st = il[i].SelectType();
        if (st==select_show)
          st = select_question;
        // obs! this doesn't yet handle aspects via AspectsFromIndices();
        Object *obj = new Object(the_db, cidx, st, 0, il[i].Extra());
        obj->CopyStageInfo(il[i]);
        obj->Aspects(il[i].Aspects());

        if (sibling_i != -1) 
          obj->SiblingValue(val);
        else
          obj->AboveValue(val);

        obj->GenuineRelevance(false);

        if (seenimg && Target() == obj->TargetType())
          obj->not_really_seen = true;
      
        /*if (seenimg) {
          AddToSeen(obj);
          } else {*/

        il.push_back(*obj);
        idx[obj->Index()]=il.size()-1;
        //}

        if (debug_subobjects)
          cout <<  "PushRelevanceDown [" << val << "]: " << il[i].DumpStr()
               << " => " << obj->DumpStr() << endl;
        
        delete obj;
      }
    } // for (...chldrn...)
  } 


  /////////////////////////////////////////////////////////////////////////////
  // Relevance going UP
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExpandUp(const string& s) {
    bool res = RelevanceUp(s, expand_operation, seenobject, false, true);
    SortSeen();
    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExchangeUp(const string& s) {
    return RelevanceUp(s, exchange_operation, uniqueimage, true, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvergeUp(const string&s) {
    return RelevanceUp(s, converge_operation, newobject, true, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RelevanceUp(const string &arg, struct relop_stage_type& rs, 
                          ObjectList& il, bool divide, bool seenimg) {
    Tic("RelevanceUp");

    bool discard_children=arg.find("discard_children")!=string::npos;

    if (debug_subobjects && discard_children)
      cout << "RelevanceUp: discard_children=true" << endl;

    size_t nl = il.size();

    ZeroRelevanceCounters(il);

    if (seenimg)
      AllSeenReally();

    map<int,int> idx;
    for (size_t i=0; i<nl; i++)  // only temporary solution, should use ivp 
      idx[il[i].Index()]=i;

    for (size_t i=0; i<nl; i++) {
      if (!il[i].Retained())
        continue;

      if (il[i].hasParents()) {
        double val = il[i].Value();
        PushRelevanceUp(i, rs, il, val, idx, divide, seenimg);
        if (discard_children)
          il[i].Discard();
      }
    }

    SumRelevanceCounters(il/*,seenimg*/);
  
    Tac("RelevanceUp");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::PushRelevanceUp(size_t i, struct relop_stage_type& rs, 
                              ObjectList& il, double val, map<int,int> &idx,
                              bool divide, bool seenimg) {

    bool divup = (rs.up == relop_avg);
    bool max = (rs.up == relop_max);

    if (debug_subobjects&&true) {
      cout << "PushRelevanceUp(): divide=" << (divide?"true":"false") 
           << " divup=" << (divup?"true":"false")
           << " max=" << (max?"true":"false") << endl;
    }

//    if (!val) return;

    //size_t nl = il.size();

    vector<int> parents = il[i].Parents();
    if (parents.empty())
      return;

    for (size_t pari=0; pari<parents.size(); pari++) {
      int pidx = parents[pari];

      //  // search for parent in il list...
      //     bool found = false;
      //     for (size_t j=0; j<nl && !found; j++) {
      //       if (il[j].Index()==pidx) {
      //         found = true;

      //         int ch_num = il[j].Children().size();
      //         double upval = val;
      //         if (divup && divide) upval /= ch_num;

      //         il[j].BelowValue(upval);
      //         if (debug_subobjects)
      //           cout << "PushRelevanceUp   [" << upval << "]: " << il[j].DumpStr()
      //                << " <+ " << il[i].DumpStr() << endl;
        
      //         PushRelevanceUp(j, il, ivp, upval);

      //         if (relevance_to_siblings && il[j].Children().size() > 1) 
      //           PushRelevanceDown(j, il, ivp, upval, divide, i);
      //       }

      //     }

      // search for parent in il list...
      bool found = idx.count(pidx)>0;
      if(found){
        size_t j=idx[pidx];
        //     for (size_t j=0; j<nl && !found; j++) {
        //       if (il[j].Index()==pidx) {
        //         found = true;

        int ch_num = il[j].Children().size();
        double upval = val;
        if (divup && divide) upval /= ch_num;

        il[j].BelowValue(upval,max);
        if (debug_subobjects)
          cout << "PushRelevanceUp   [" << upval << "]: " << il[j].DumpStr()
               << " <+ " << il[i].DumpStr() << endl;
      
        PushRelevanceUp(j, rs, il, upval, idx,
                        true /*divide???*/, seenimg);
      
        if (relevance_to_siblings && il[j].Children().size() > 1) 
          PushRelevanceDown(j, rs, il, upval, idx, divide, seenimg, i);
      }

      if (!seenimg && !CanBeShownRestricted(pidx, false))
        continue;

      if (!found) {
        select_type st = il[i].SelectType();
        if (st==select_show)
          st = select_question;
        // obs! this doesn't yet handle aspects via AspectsFromIndices();
        Object *obj = new Object(the_db, pidx, st, 0, il[i].Extra());

        int ch_num = obj->Children().size();
        double upval = val; 
        if (divup && divide) upval /= ch_num;

        obj->CopyStageInfo(il[i]);
        obj->BelowValue(upval,max);
        obj->GenuineRelevance(false);
      
        if (seenimg && Target() == obj->TargetType())
          obj->not_really_seen = true;

        /*      if (seenimg) {
                AddToSeen(obj);
                } else {*/
        il.push_back(*obj);
        //}
        idx[obj->Index()]=il.size()-1;

        if (debug_subobjects) 
          cout <<  "PushRelevanceUp   [" << upval << "]: " 
               << obj->DumpStr() << " <= " 
               << il[i].DumpStr() << endl;
      
        if (relevance_to_siblings && obj->Children().size() > 1) 
          PushRelevanceDown(il.size()-1, rs, il, upval, idx,
                            divide, seenimg, i);

        delete obj;
      }
    } // for (...parents...)
  }

  /////////////////////////////////////////////////////////////////////////////
  // Relevance going UP & DOWN
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExpandUpDown(const string& s) {
    bool res = RelevanceUpDown(s, expand_operation, seenobject, false, true);
    SortSeen();
    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExchangeUpDown(const string& s) {
    return RelevanceUpDown(s, exchange_operation, uniqueimage, true, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConvergeUpDown(const string& s) {
    return RelevanceUpDown(s, converge_operation, newobject, true, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RelevanceUpDown(const string& /*s*/, 
                              struct relop_stage_type& rs, ObjectList& il, 
                              bool divide, bool seenimg) {
  
    size_t nl = il.size();

    map<int,int> idx;
    for (size_t i=0; i<nl; i++)  // only temporary solution, should use ivp 
      idx[il[i].Index()]=i;

    if (seenimg) AllSeenReally();

    ZeroRelevanceCounters(il);

    for (size_t i=0; i<nl; i++) {
      if (il[i].hasChildren())
        PushRelevanceDown(i, rs, il, il[i].Value(), idx,
                          divide, seenimg, -1);
    }
  
    for (size_t i=0; i<nl; i++)
      if (il[i].hasParents())
        PushRelevanceUp(i, rs, il, il[i].Value(), idx, divide, seenimg);
  
    SumRelevanceCounters(il/*,seenimg*/);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExpandSelective(const string &arg) {

    bool maxonly=arg.find("maxonly") != string::npos;

    //cout << "about to copy seenimage" << endl;

    //  seenimagereal = seenimage; // copy parent objects to the list of all seen images

    seenobjectreal.clear();

    //   for (int i=0; i<NseenImages(); i++) {
    //     int idx = seenimage[i].Index();

    //     if(idx != -1){
    //       Object *o = new Object(seenimage[i]);

    //       AddToSeenReal(o);
    //     }
    //     //cout << "copied " << seenimage[i].LabelP() << endl;
    //   }



    //cout << "Seenimage copied" << endl;

    // prepare matrices for relevance determination

    if (DefaultMatrixCount()<3)
      return ShowError("Query:: ExpandSelective(): matrixcount<3");

    ExpandDown("");

    int nm = Nindices();
    if (!nm)
      ShowError("Query::ExpandSelective(...) : Nindices()==0");

    for (int i=0; i<nm; i++) {
      int l = NlevelsEvenAlien(i);
      for (int j=l-1; j<l; j++) { // bottom level only
      
        if (!PlaceSeenOnMap(i, j, false)) {
          char tmp[100];
          sprintf(tmp, "(%d,%d)", i, j);
          return ShowError("Query::ExpandSelective", tmp,
                           ": PlaceSeenOnMap() failed");
        }

        Convolve(i, j, 1) && Convolve(i, j, 2);
        if(mapimagename!="" && j==l-1)
          MapSnapshots(i,j,"rel");
      }
    }

    RemoveNongenuineFromSeen();

    size_t Nseen = NseenObjects();
    for (size_t ind=0; ind<Nseen; ind++) {
    
      // if(debug_selective){
      //       cout << "Incoming relevance distribution of " 
      //            << seenimage[ind].Label() << ":"<<endl;
      //       relevance_distribution & rd=seenimage[ind].rel_distribution;

      //       cout << "rootval " << rd.rootval << endl;
      //       for (size_t i=0; i<rd.subind.size(); i++){
      //         cout << "(" << Label(rd.subind[i]) << "): " <<rd.subval[i] << endl;
      //         for(int map=0;map<Nindices();map++){
      //           int level=NlevelsEvenAlien(map)-1;
      //           int mapidx = Division(map,level)[rd.subind[i]];
      //           if (mapidx!=-1) {
      //             IntPoint p = MapEvenAlien(map,level).ToPoint(mapidx);
      //             cout << "on maplevel ("<<map<<","<<level<<") -> ("<<
      //               p.X()<<p.Y()<<")"<<endl;
      //           }
      //         }
      //       }
      //       cout << "/dump" << endl;

      //       //      seenimage[ind].rel_distribution.dump();
      //     }
      vector<int>& suba = seenobject[ind].rel_distribution.subind;
      const vector<float> &subvals = seenobject[ind].rel_distribution.subval;
      
      if(Nseen <2 || seenobject[ind].Value() <0){
        // distribute relevance evenly if negative example
        // or first round

        if(debug_selective)
          cout << "Evenly distributing relevance to subobjects of "
               << seenobject[ind].Label() << endl;

        int idx = seenobject[ind].Index();    
        const vector<int> &subo = the_db->SubObjects(idx);

        for (size_t subi=0; subi<subo.size(); subi++)
          if (!IsSeen(subo[subi])) {
            // obs! this doesn't yet handle aspects via AspectsFromIndices();
            Object *newimg = new Object(the_db, subo[subi], select_answer, 
                                        seenobject[ind].Value());
            newimg->GenuineRelevance(false);
            // Object *o2=new Object(*newimg);
            //           o2->GenuineRelevance(false);
            //          AddToSeenReal(o2);
            if (debug_subobjects)
              cout << "ExpandSelective  " << seenobject[ind].DumpStr() << " -> "
                   << newimg->DumpStr() << "(negative or unknown distr.)"<<endl;
            AddToSeen(newimg);
          }
      } // if negative or no subvals
      else if(seenobject[ind].Value() >0){
        StoreSubobjectContribution(seenobject[ind],"");

        //      vector<int>& sub = seenimage[ind].rel_distribution.subind;
        // const vector<float> &subvals = seenimage[ind].rel_distribution.subval;

        //      float zerolevel=*min_element(subvals.begin(),subvals.end());
        //      if(zerolevel>1) zerolevel=1;


        float maxval=*max_element(subvals.begin(),subvals.end());

        int ns = subvals.size();

        // calculate mean without maxelement
      
        float mean_other = 0.0, variance;
        if (ns>1)
          mean_other=(accumulate(subvals.begin(),subvals.end(),0.0)-maxval)/(ns-1);

        float sum=0,sum_other=0;
        float sqrsum_other=0;

        int n_other=0;

        // calculate variance
        for(int i=0;i<ns;i++){
          sum += subvals[i];
          if(subvals[i] < maxval){
            sum_other += subvals[i];
            sqrsum_other += subvals[i]*subvals[i];
            n_other++;
          }
        }
        
        sum /= ns;
        if(n_other){
          sum_other /= n_other;
          sqrsum_other /= n_other;
          // estimate not unbiased
          variance=sqrsum_other-sum_other*sum_other;
        } else variance=0;

        if(variance<0) variance=0;
      
        const float fraction=0.8;
        float threshold=fraction*(maxval-mean_other)+mean_other;

        const float devfraction = 1;
        float devthreshold=maxval-devfraction*sqrt(variance);

        if(debug_selective){
          cout << "Selectively distributing relevance to subobjects of "
               << seenobject[ind].Label() << endl;
          cout << "devthreshold=" << devthreshold <<" stddev: " << sqrt(variance)<<endl;
          cout << "threshold=" << threshold << " mean="<< sum << endl;
        }
      
        //float absolutelimit=1.55;

        //if(threshold>absolutelimit) threshold=absolutelimit;


        for (size_t subi=0; subi<suba.size(); subi++)
          if ( !IsSeen(suba[subi])) {
            // obs! this doesn't yet handle aspects via AspectsFromIndices();
            Object *newimg = new Object(the_db, suba[subi], select_answer, 
                                        seenobject[ind].Value());
            newimg->GenuineRelevance(false);
            //AddToSeenReal(newimg);
            if((maxonly && subvals[subi]==maxval) || 
               (!maxonly && (ns==1 ||n_other==0 ||
                             subvals[subi] >= threshold || 
                             subvals[subi] >= devthreshold))){
              //Object *o2=new Object(*newimg);
              //o2->GenuineRelevance(false);
              AddToSeen(newimg);
              if (debug_selective) {
                cout << "marked (" << Label(suba[subi]) << ") : " 
                     <<subvals[subi]<<" ";
                for(size_t map=0;map<Nindices();map++){
                  int level=NlevelsEvenAlien(map)-1;
                  int mapidx = Division(map,level)[suba[subi]];
                  if (mapidx!=-1) {
                    IntPoint p = MapEvenAlien(map,level).ToPoint(mapidx);
                    cout << "on maplevel ("<<map<<","<<level<<") -> ("<<
                      p.X()<<","<<p.Y()<<")"<<endl;
                  }
                }
              }
            } 

            // obs! newimg is overloaded, which one should be used here?
            if (debug_subobjects)
              cout << "ExpandSelective  " << seenobject[ind].DumpStr() << " -> "
                /* << newimg->DumpStr()*/ << "(positive)"<<endl;
          } // !IsSeen()
      } // if positive 
    } // for ind < Nseen

    if(debug_selective){
      cout << "after ExpandSelective, seenimage = " << endl;
      DumpSeen();
    }

    //cout << "After ExpandSelective:" << endl;
    //DumpSeenRelevance();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef QUERY_USE_PTHREADS

  bool Query::ForAllMapLevelsPthread(bool (Query::*f)(const string&,
                                                      int, int, bool), 
                                     const string& args, bool botonly) {
    if (DefaultMatrixCount()!=1)
      return ShowError("Query::ForAllMapLevelsPthread() matrixcount!=1");

    Tic("ForAllMapLevelsPthread");

    query_main_thread = pthread_self();

    bool ok = true;

    int n = NumberOfScoreValues();
    int nthrs = picsom->Threads()>=0 ? picsom->Threads() : n;
    int con = pthread_getconcurrency();
    if (pthread_setconcurrency(nthrs))
      ShowError("Query::ForAllMapLevelsPthread() ",
                "pthread_setconcurrency() failed");

    map_level_pthread_data *data = new map_level_pthread_data[n];

    IntVector nlevels = Nlevels();
    int m = 0;
    while (nlevels.Sum())
      for (size_t i=0; i<Nindices(); i++)
        if (nlevels[i]) {
          int j = --nlevels[i];
          map_level_pthread_data *dp = data+m;
          dp->thread_set = false;
          dp->query = this;
          dp->f = f;
          dp->args = &args;
          dp->map = i;
          dp->level = j;
          dp->botonly = botonly;
          // WriteLog("--- calling pthread_create()");
          // #ifdef __alpha
          void *(*xxx)(void*) = ForAllMapLevelsPthread;  // alpha's cxx cannot solve overloading
          // #else
          pthread_create(&dp->pthread, NULL, (CFP_pthread) xxx, dp);
          dp->thread_set = true;
          // #endif // __alpha
          // WriteLog("--- exited pthread_create()");
          m++;
          if (botonly)
            nlevels[i] = 0;
        }
  
    for (int l=0; l<m; l++) {
      // WriteLog("--- calling pthread_join()");
      pthread_join(data[l].pthread, NULL);
      // WriteLog("--- exited pthread_join()");
    }

    delete [] data;

    pthread_setconcurrency(con);

    Tac("ForAllMapLevelsPthread");

    main_thread_set = false;

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  void *Query::ForAllMapLevelsPthread(void *data) {
    // picsom->ConditionallyAnnounceThread("ForAllMapLevelsPthread()");

    map_level_pthread_data *dp = (map_level_pthread_data*)data;
    dp->query->picsom->BlockSignals();

    /*
      cout << "Now in Query::ForAllMapLevelsPthread(void *data)"
      << " pthread_self()=" << pthread_self()
      << " dp->map=" << dp->map
      << " dp->level=" << dp->level
      << endl;
    */

    ((dp->query)->*(dp->f))(*dp->args, dp->map, dp->level, dp->botonly);

    return NULL;
  }
#endif // QUERY_USE_PTHREADS

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ForAllMapLevels(bool (Query::*f)(const string&,
                                               int,int,bool),
                              const string& args, bool botonly) {
#ifdef QUERY_USE_PTHREADS
    if (picsom->PthreadsTssom())
      return ForAllMapLevelsPthread(f, args, botonly);
#endif // QUERY_USE_PTHREADS

    Tic("ForAllMapLevels");

    bool ok = true;
    int n = Nindices();
    if (!n)
      ShowError("Query::ForAllMapLevels(...) : Nindices()==0");

#ifdef sgi
#pragma parallel
#pragma pfor
#endif // sgi
    for (int i=0; i<n; i++) {
      int l = NlevelsEvenAlien(i);
      for (int j=0; j<l; j++) {
        if (!(botonly && j<l-1)) {
          PicSOM::ThreadTestCancel();
          ok = ok && (this->*f)(args, i, j, botonly);
        }
      }
    }

    Tac("ForAllMapLevels");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ForAllIndices(bool (Query::*f)(int)) {
    bool ok = true;
    for (size_t i=0; ok && i<Nindices(); i++) {
      PicSOM::ThreadTestCancel();
      ok = ok && (this->*f)(i);
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageExpandRelevance(cbir_function func,
                                                 const string& args) {
    bool ok = false;

    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_expand:
      ok = Expand(args);
      break;

    case func_expand_up:
      ok = Expand("sum,none");
      break;

    case func_expand_down:
      ok = Expand("none,copy");
      break;

    case func_expand_up_down:
      ok = Expand("sum,copy");
      break;

    case func_expand_down_up:
      Obsoleted("ExpandDownUp()");
      ok = ExpandDownUp(args);
      break;
    
    case func_expand_selective:
      ok = ExpandSelective(args);
      break;

    default:
      ShowError("Query::StageExpandRelevance(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_run_per_map : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageRunPerMap(cbir_function func,
                                           const string& args) {
    bool ok = false;

    switch (func) {
    case func_per_map_picsom_all:
      ok = ForAllMapLevels(&Query::PerMapPicSOM, args, false);
      break;

    case func_per_map_picsom_bottom:
      ok = ForAllMapLevels(&Query::PerMapPicSOM, args, true);
      break;

    case func_per_map_vq:
      ok = ForAllIndices(&Query::PerMapVQ);
      break;
    
    case func_goto_extract_images:
      return stage_extract_images;
    
    default:
      ShowError("Query::StageRunPerMap(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_set_map_weights : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageSetMapWeights(cbir_function func,
                                       const string& args) {

    if (CalculateEntropy() && !DoEntropyCalculation()) {
      ShowError("Query::StageSetMapWeights(): DoEntropyCalculation() failed");
      return stage_error;
    }

    bool ok = false;

    switch (func) {
    case func_one_for_all:
      ok = ValueOrNaN(1, true);
      break;

    case func_one_for_bottom:
      ok = ValueOrNaN(1, false);
      break;

    case func_const_for_bottom:
      ok = TsSomWeightValue(false);
      break;
      
    case func_distances_for_bottom:
      ok = TsSomWeightValueAndDistances(false);
      break;      

    case func_entropy_n_polynomial:
      ok = EntropyNpolynomial(args);
      break;

    case func_entropy_n_leave_out_worst:
      ok = EntropyNleaveOutWorst(args);
      break;
    
    default:
      ShowError("Query::StageSetMapWeights(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    if (ok && debug_weights) {
      cout << "weights: ";
      for (size_t i=0; i<Nindices(); i++) {
        cout << IndexFullName(i) << "=[";
        const TSSOM::State& fd = IndexDataTSSOM(i);
        for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
          if (fd.tssom_level_weight[j]==Index::State::nan())
            cout << (j?" ":"") << "NA";
          else
            cout << (j?" ":"") << fd.tssom_level_weight[j];
        cout << "] ";
      }
      cout << endl;
    }
  
    return ok ? stage_combine_unit_lists : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageCombineUnitLists(cbir_function func, const string&) {
    bool debug = false;

    if (func==func_offline_shortcut)
      return OfflineShortcut();

    if (func!=func_default)
      ShowError("StageCombineUnitLists() : func!=func_default");

    bool has_binary = false, has_non_binary = false; // term "binary" here includes
                                                     // precalculated features
    for (size_t i=0; i<Nindices(); i++)
      if (!IsTsSom(i))
        has_binary = true;
      else
        has_non_binary = true;

    int n = 0, nn = 0;
    for (size_t ii=0; ii<Nfeatures(); ii++)
      if (IsTsSom(ii))
        for (size_t jj=0; jj<TsSom(ii).Nlevels(); jj++, nn++)
          n += PerMapVQUnit(ii, jj).size();

    if (debug)
      cout << "StageCombineUnitLists(): " << n
           << " units in total, VQUnitsSize=" << nn << endl;

    // If limit_total_units really would be non-zero we should rethink
    // the way TSSOM weighting is implemented.  Now it is in
    // SelectImagesFromVQUnits().

    if (_sa_limit_total_units && _sa_limit_total_units<n) {
      FloatVector values;
      for (size_t i=0; i<Nfeatures(); i++)
        if (IsTsSom(i))
          for (size_t l=0; l<TsSom(i).Nlevels(); l++) {
            const VQUnitList& ul = PerMapVQUnit(i, l);
            FloatVector add(ul.size());
            for (size_t j=0; j<ul.size(); j++)
              add[j] = ul[j].Value();
            values.Append(add);
          }

      values.QuickSortDecreasingly();
      float ref = values[_sa_limit_total_units-1];
      n = 0;
      for (size_t k=0; k<Nfeatures(); k++)
        if (IsTsSom(k))
          for (size_t l=0; l<TsSom(k).Nlevels(); l++) {
            VQUnitList& ul = PerMapVQUnit(k, l);
            for (size_t j=0; j<ul.size(); j++)
              if (n>=_sa_limit_total_units || ul[j].Value()<ref)
                ul[j].Discard();
              else
                n++;
          }
    }
  
    if (has_non_binary)
      return n ? stage_extract_images : stage_error;

    return has_binary ? stage_extract_images : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageExtractObjects(cbir_function func,
                                        const string& args) {
    bool ok = false;

    switch (func) {
    case func_extract_images_picsom_all:
      ok = ForAllMapLevels(&Query::ForUnitsPicSOM, args, false);
      break;

    case func_extract_images_picsom_bottom:
      ok = ForAllMapLevels(&Query::ForUnitsPicSOM, args, true);
      break;

    case func_extract_images_vq:
      ok = ForAllIndices(&Query::ForUnitsVQ);
      break;
   
    case func_extract_images_sq:
      ok = ForAllIndices(&Query::PerFeatureSQ);
      break;
   
    default:
      ShowError("Query::StageRunForUnits(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_combine_image_lists : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageCombineObjectLists(cbir_function func,
                                            const string&) {
    if (func!=func_default)
      ShowError("StageCombineObjectLists() : func!=func_default");

    DeleteCombined();
    for (size_t i=0; i<PerMapObjectsSize(); i++)
      combinedimage.append(PerMapNewObject(i));

    return combinedimage.size() ? stage_remove_duplicates : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageRemoveDuplicates(cbir_function func,
                                                  const string&) {
    bool ok = false;

    switch (func) {
    case func_remove_duplicates_picsom:
      ok = RemoveDuplicatesPicSOM();
      break;

    case func_remove_duplicates_vq:
      ok = RemoveDuplicatesVQ();
      break;
    
    default:
      ShowError("Query::StageRemoveDuplicates(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_exchange_relevance : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageExchangeRelevance(cbir_function func,
                                                   const string& args) {
    bool ok = false;

    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_exchange:
      ok = Exchange(args);
      break;
    
    case func_exchange_up:
      ok = Exchange("sum,none");
      break;
    
    case func_exchange_down:
      ok = Exchange("none,div");
      break;
    
    case func_exchange_up_down:
      ok = Exchange("sum,div");
      break;
    
    case func_exchange_down_up:
      Obsoleted("ExchangeDownUp()");
      ok = ExchangeDownUp(args);
      break;
    
    default:
      ShowError("Query::StageExchangeRelevance(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_select_images_to_process : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageSelectObjectsToProcess(cbir_function func,
                                                const string& a) {
    if (func!=func_default)
      ShowError("StageSelectImagesToProcess() : func!=func_default");

    int n = uniqueimage.size();

    int limit_total_images = a.size() ? atoi(a.c_str()) : 0;
  
    if (limit_total_images && limit_total_images<n) {

      FloatVector values(uniqueimage.size());
      for (int i=0; i<values.Length(); i++)
        values[i] = uniqueimage[i].Value();
      values.QuickSortDecreasingly();

      float ref = values[limit_total_images-1];
      n = 0;
      for (size_t k=0; k<uniqueimage.size(); k++)
        if (n>=limit_total_images || uniqueimage[k].Value()<ref)
          uniqueimage[k].Discard();
        else
          n++;
    }

    DeleteNew();
    random_image_count = 0;

    map<string,aspect_data> am = AspectsFromIndices();

    for (size_t i=0; i<uniqueimage.size(); i++)
      if (uniqueimage[i].Retained()) {
        Object o(uniqueimage[i]);
        o.Aspects(am);
        AppendNewObject(o);
      }

    //  cout << "StageSelectObjectsToProcess(): NnewObjects()=" 
    //       << NnewObjects() << endl;
    return NnewObjects() ? stage_process_images : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageProcessObjects(cbir_function func,
                                        const string& args) {
    bool ok = false;

    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_process_images_picsom_bottom:
      ok = ProcessObjectsPicSOMBottom(args);
      break;

    case func_process_images_vq:
      ok = ProcessObjectsVQ(args);
      break;

    case func_process_images_vqw:
      ok = ProcessObjectsVQW(args);
      break;
    
    default:
      ShowError("Query::StageProcessImages(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_converge_relevance : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageConvergeRelevance(cbir_function func,
                                                   const string& args) {
    bool ok = false;

    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_converge:
      ok = Converge(args);
      break;

    case func_converge_up:
      ok = Converge("sum,none");
      break;

    case func_converge_down:
      ok = Converge("none,div");
      break;

    case func_converge_up_down:
      ok = Converge("sum,div");
      break;

    case func_converge_down_up:
      Obsoleted("ConvergeDownUp()");
      ok = ConvergeDownUp(args);
      break;

    default:
      ShowError("Query::StageConvergeRelevance(): operation for <",
                FunctionNameP(func), "> undefined");
    }

    return ok ? stage_final_select : stage_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageFinalSelect(cbir_function func,                     
                                             const string &arg) {
    if (func==func_true) { // bypass this stage
      for (size_t i=0; i<NnewObjects(); i++)
        NewObject(i).Discard();

      PossiblyRelativizeScores();

      return stage_ready;
    }

    if (func!=func_default)
      ShowError("StageFinalSelect() : func!=func_default");

    string filterclass;

    size_t filterpos = arg.find("filter=");
    if (filterpos!=string::npos){
      const char *str = arg.substr(filterpos+7).c_str();
      int incr = 0;

      while (str[incr] && str[incr]!=',' && !isspace(str[incr]) && 
            str[incr] != ')')
        incr++;

      filterclass = arg.substr(filterpos+7,incr);
      cout << "Parsed final filter "<<filterclass<<endl;
    }

    //bool store_relevance= arg.find("store_relevance")!=string::npos;

    //if(store_relevance && matrixcount < 3)
    //  ShowError("StageFinalSelect() : store_relevance && matrixcount < 3");

    bool productboost = arg.find("productboost")!=string::npos;

    if (productboost && DefaultMatrixCount()<3)
      ShowError("StageFinalSelect() : productboost && matrixcount < 3");

    Tic("StageFinalSelect");

    //  if (productboost) {

    //     const int largeval = 10;

    //     StoreSubobjectContribution("");
    //     for (int i=0; i<NnewObjects(); i++) {
    //       const vector<float> &vals=newobject[i].rel_distribution.subval;

    //       bool reaches_unity=false;
    //       for (size_t n=0; n<vals.size(); n++)
    //         if (vals[n]>= 1.0) {
    //           reaches_unity=true;
    //           break;
    //         }

    //       if (reaches_unity) {
    //         newobject[i].AddValue(largeval);
    //         if (debug_selective)
    //           cout << "Added value to label " << newobject[i].Label() <<
    //             " val now " << newobject[i].Value() << endl;
    //       }
    //     }
    //   }

    if (!filterclass.empty()){
      target_type tt = Target();
      // if (tt==target_no_target) // commented out 2.5.2006 by jl
      //   tt = target_image;  // this might be redundant...
      
      bool expand = true;
      ground_truth ffilter = the_db->GroundTruthExpression(filterclass, tt,
                                                           -1, expand);

      for (size_t i=0; i<NnewObjects(); i++) {
        if (!NewObject(i).Retained())
          continue;
        int idx=NewObject(i).Index();

        bool infilter = ffilter.IndexOK(idx) && ffilter[idx]==1;

        if (!infilter) {
          // cout << "finalfilter: discarding " << NewObject(i).Label()<<endl;
          NewObject(i).Discard();        
        }
        //else
        //  cout << "finalfilter: letting through " 
        //       << NewObject(i).Label()<<endl;
      }
    }
 
    //Object::SortList(newobject);

    vector<double> vals;        
    size_t n = 0;

    for (size_t i=0; i<NnewObjects(); i++) {
      if (!NewObject(i).Retained())
        continue;
      target_type tt = NewObject(i).TargetType();
      // bool type_ok = tt&Target();
      bool type_ok = PicSOM::TargetTypeContains(tt, Target());
      if (type_ok) {
        vals.push_back(NewObject(i).Value());
        n++;
      } else
        NewObject(i).Discard();
    }

    // cout <<"n="<<n<<" maxquestions="<<MaxQuestions()<<endl;

    double lastval = 0.0;
    size_t count = 0;
    if (n>MaxQuestions()) {
      sort(vals.begin(),vals.end());
      lastval = vals[n-MaxQuestions()];

      for (size_t i=0; i<NnewObjects(); i++) {
        if (!NewObject(i).Retained())
          continue;
        if (NewObject(i).Value()<lastval) {
          // this is a bit sloppy, see also CheckObjectIdx()
          NewObject(i).Discard();
        } else
          count++;
      }
    }

    //int retainedcount=0;
    
//     for (int i=0; i<NnewObjects(); i++){
//       if (NewObject(i).Retained())
//         {
//         retainedcount++;
//         //        cout << i << "retained." << endl;
//         }}


//     cout << "NnewObjects()="<<NnewObjects()<<" ("<<retainedcount<<" retained)"<<endl;

//     // move discarded items to the end of the newobject

//     int lastretained=NnewObjects()-1;
//     while(lastretained >=0 && !NewObject(lastretained).Retained()) lastretained--;

//     for(int i=0;i<lastretained;i++){
//       if(NewObject(i).Retained()) continue;
//       newobject.Swap(i,lastretained);
//       while(lastretained >=0 && !NewObject(lastretained).Retained()) lastretained--;
//     }

    Tic("sort");
    if (arg.find("nosort")==string::npos)
      Object::SortListByValue(newobject);
    Tac("sort");

    // this is needed if many objects in the end of the list  have the same value
    while (count-->MaxQuestions())
      NewObject(count).Discard();

    // if (store_relevance && !productboost) {
    //   StoreSubobjectContribution("");
    // }

    if (debug_selective)
      cout << "StageFinalSelect(): score threshold " << lastval << endl;

    Tac("StageFinalSelect");

    // return NnewObjects()&&NewObject(0).Retained() ? stage_ready : stage_error; 
    PossiblyRelativizeScores();

    return n ? stage_ready : stage_error;
 }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PossiblyRelativizeScores() {
    if (!relativescores)
      return true;

    float div = 0.0;
    for (size_t i=0; div==0.0 && i<NnewObjects(); i++)
      if (NewObject(i).Retained())
        div = NewObject(i).Value();

    for (size_t i=0; div>0 && i<NnewObjects(); i++) {
      NewObject(i).Value(NewObject(i).Value()/div);
      NewObject(i).MultiplyStageInfo(1/div);
    }

    return true;
 }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetOrSumNewObjects(const Query& q) {
    bool debug = false, sloppy = true;

    for (size_t i=0; i<q.NnewObjects(); i++) {
      const Object& o = q.NewObject(i);

      Object *p = FindInNew(o.Index());
      if (p) {
        if (o.Retained()!=p->Retained())
          return ShowError("Query::SetOrSumNewObjects() error 4");
      
        p->Combine(o, true);
      
        if (debug)
          cout << " COMBINED with " << p->DumpString(p->Index()) << endl;
      
      } else {
        AppendNewObject(o);

        if (debug)
          cout << " ADDED as index " << NnewObjects()-1 << endl;
      }

      if (debug)
        CheckNewObjectIdx(sloppy);
    }

    Object::SortListByValue(newobject);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::StageNames(int i, cbir_stage& s, const char*& t) {
    struct stage_entry {
      cbir_stage stage;
      const char *name;
    };

    static stage_entry list[] = {
#define make_stage_entry(e) { stage_ ## e , # e }
      make_stage_entry(restrict),
      make_stage_entry(enter),
      make_stage_entry(initial_set),
      make_stage_entry(check_image_count),
      make_stage_entry(no_positives),
      make_stage_entry(has_positives),
      make_stage_entry(special_first_round),
      make_stage_entry(expand_relevance),
      make_stage_entry(run_per_map),
      make_stage_entry(set_map_weights),
      make_stage_entry(combine_unit_lists),
      make_stage_entry(extract_images),
      make_stage_entry(combine_image_lists),
      make_stage_entry(remove_duplicates),
      make_stage_entry(exchange_relevance),
      make_stage_entry(select_images_to_process),
      make_stage_entry(process_images),
      make_stage_entry(converge_relevance),
      make_stage_entry(final_select),
      make_stage_entry(ready),
      make_stage_entry(error)
#undef make_stage_entry
    };

    int n = sizeof(list)/sizeof(stage_entry);
    if (i<0 || i>=n) {
      s = (cbir_stage) -1;
      t = NULL;
      // f = NULL;
      return false;
    }
  
    s = list[i].stage;
    t = list[i].name;

    // f = stages+i;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *Query::StageNameP(cbir_stage x) const {
    for (int i=0;; i++) {
      cbir_stage s;
      const char *name;
      if (!StageNames(i, s, name))
        break;

      if (x==s)
        return name;
    }
    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_stage Query::StageName(const string& n) const {
    for (int i=0;; i++) {
      cbir_stage s;
      const char *name;
      if (!StageNames(i, s, name))
        break;

      if (n==name)
        return s;
    }
    return (cbir_stage)-1;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetStageFunction(const string& s, const string& f,
                               bool e) {
    string fname = f, args;

    size_t p = fname.find('(');
    if (p!=string::npos) {
      args = fname.substr(p+1);
      fname.erase(p);

      p = args.find(')');
      if (p!=string::npos)
        args.erase(p);
      else
        return ShowError("Query::SetStageFunction() unclosed parenthesis");
    }

    cbir_stage   stage = StageName(s);
    cbir_function func = fname.length()?FunctionName(fname):func_default;

    if ((int)stage==-1 || (int)func==-1) {
      if (e)
        ShowError("Query::SetStageFunction(", s, ",", f, ") failed (1)");
      return false;
    }
    for (int i=0;; i++) {
      cbir_stage x;
      const char *dummy;
      if (!StageNames(i, x, dummy))
        break;

      if (x==stage) {
        StageFunc(stage, func);
        StageArgs(stage, args);
        return true;
      }
    }

    if (e)
      ShowError("Query::SetStageFunction(", s, ",", f, ") failed (2)");

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageFunction(cbir_stage x, bool def,
                                             bool show_def,
                                             bool warn) const {
    cbir_function ff, err = func_unknown;

    try {
      ff = StageFunc(x);
    }
    catch (...) {
      return err;
    }

    if ((ff!=func_default || def) && !show_def)
      return ff;

    // obs! HAS_CBIRALGORITHM_NEW problem?:
    if (!algorithms.empty())
      return StageDefaultPicSOMBottom(x, warn);

    switch (algorithm) {
    case cbir_random:
      return StageDefaultRandom(x, warn);
    
    case cbir_picsom:
      return StageDefaultPicSOMAll(x, warn);
    
    case cbir_picsom_bottom:
      return StageDefaultPicSOMBottom(x, warn);

    case cbir_picsom_bottom_weighted:
      return StageDefaultPicSOMBottomWeighted(x, warn);

    case cbir_vq:
      return StageDefaultVQ(x, warn);
    
    case cbir_vqw:
      return StageDefaultVQW(x, warn);
    
    case cbir_sq:
      return StageDefaultSQ(x, warn);
    
    case cbir_external:
      return StageDefaultExternal(x, warn);
    
    default:
      WarnOnce("Query::StageFunction() algorithm ("+ToStr(algorithm)+
               ") unknown");
      return err;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageDefaultRandom(cbir_stage x,
                                                  bool warn) const {
    switch (x) {
    case stage_restrict:                 return func_true;
    case stage_enter:                    return func_default;
    case stage_initial_set:              return func_random_unseen_images;
    case stage_no_positives:             return func_goto_initial_set;
    case stage_has_positives:            return func_goto_initial_set;

    case stage_set_map_weights:          return func_default;

    default:
      if (warn)
        ShowError("Query::StageDefaultRandom() stage <",
                  StageNameP(x), "> unknown");
      return func_unknown;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageDefaultPicSOMAll(cbir_stage x,
                                                     bool warn) const {
    switch (x) {
    case stage_restrict:                 return func_true;
    case stage_enter:                    return func_default;
    case stage_initial_set:              return func_top_levels_by_entropy;
    case stage_check_image_count:        return func_random_unseen_images;
    case stage_no_positives:             return func_goto_has_positives;
    case stage_has_positives:            return func_goto_expand_relevance;
    case stage_expand_relevance:         return func_expand_down;
    case stage_run_per_map:              return func_per_map_picsom_all;
    case stage_set_map_weights:          return func_one_for_all;
    case stage_combine_unit_lists:       return func_default;
    case stage_extract_images:           return func_extract_images_picsom_all;
    case stage_combine_image_lists:      return func_default;
    case stage_remove_duplicates:        return func_remove_duplicates_picsom;
    case stage_exchange_relevance:       return func_true;
    case stage_select_images_to_process: return func_default;
    case stage_process_images:           return func_true;
    case stage_converge_relevance:       return func_converge_up;
    case stage_final_select:             return func_default;

    default:
      if (warn)
        ShowError("Query::StageDefaultPicSOM()All stage <",
                  StageNameP(x), "> unknown");
      return func_unknown;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageDefaultPicSOMBottom(cbir_stage x,
                                                        bool warn) const{
    switch (x) {
    case stage_run_per_map:       return func_per_map_picsom_bottom;
    case stage_set_map_weights:   return func_one_for_bottom;
    case stage_extract_images:    return func_extract_images_picsom_bottom;
    case stage_process_images:    return func_process_images_picsom_bottom;

    default:
      return StageDefaultPicSOMAll(x, warn);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function
  Query::StageDefaultPicSOMBottomWeighted(cbir_stage x, bool warn) const{
    switch (x) {
    case stage_set_map_weights:
      return func_entropy_n_polynomial;
      // return func_entropy_n_leave_out_worst;

    default:
      return StageDefaultPicSOMBottom(x, warn);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageDefaultVQ(cbir_stage x,
                                              bool warn) const {
    switch (x) {
    case stage_restrict:                 return func_true;
    case stage_enter:                    return func_default;
    case stage_initial_set:                   return func_random_unseen_images;
    case stage_no_positives:                  return func_goto_initial_set;
    case stage_has_positives:                 return func_goto_expand_relevance;
    case stage_expand_relevance:         return func_true;
    case stage_run_per_map:                   return func_per_map_vq;
    case stage_combine_image_lists:      return func_default;
    case stage_combine_unit_lists:       return func_default;
    case stage_extract_images:                return func_extract_images_vq;
    case stage_remove_duplicates:        return func_remove_duplicates_vq;
    case stage_process_images:           return func_process_images_vq;
    case stage_set_map_weights:          return func_one_for_all;
    case stage_exchange_relevance:       return func_true;
    case stage_converge_relevance:       return func_true;
    case stage_select_images_to_process: return func_default;
    case stage_final_select:             return func_default;

    default:
      if (warn)
        ShowError("Query::StageDefaultVQ() stage <", StageNameP(x), "> unknown");
      return func_unknown;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageDefaultVQW(cbir_stage x,
                                               bool warn) const {
    switch (x) {
    case stage_process_images:    return func_process_images_vqw;

    default:
      return StageDefaultVQ(x, warn);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageDefaultSQ(cbir_stage x,
                                              bool warn) const {
    switch (x) {
    case stage_run_per_map:            return func_goto_extract_images;
    case stage_extract_images:    return func_extract_images_sq;
    case stage_remove_duplicates: return func_remove_duplicates_picsom;

    default:
      return StageDefaultVQ(x, warn);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::StageDefaultExternal(cbir_stage,
                                                    bool) const {
    return func_true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::FunctionNames(int i, cbir_function& f, const char*& t) {
    struct function_entry {
      cbir_function func;
      const char *name;
    };

    static function_entry list[] = {
#define make_func_entry(e) { func_ ## e , # e }
      make_func_entry(unknown),
      make_func_entry(error),
      make_func_entry(true),
      make_func_entry(default),
      make_func_entry(restriction_spec),
      make_func_entry(only_forced_images),
      make_func_entry(only_forced_images_user_test),
      make_func_entry(top_levels_by_entropy),
      make_func_entry(random_unseen_if_no_classmodels),
      make_func_entry(random_unseen_images),
      make_func_entry(random_positive_image),    
      make_func_entry(select_from_pre),
      make_func_entry(goto_initial_set),
      make_func_entry(goto_has_positives),
      make_func_entry(goto_expand_relevance),
      make_func_entry(goto_run_per_map),
      make_func_entry(goto_special_first_round),
      make_func_entry(all_unseen_images),
      make_func_entry(limit_images_with_vq),
      make_func_entry(expand),
      make_func_entry(expand_up),
      make_func_entry(expand_down),
      make_func_entry(expand_up_down),
      make_func_entry(expand_down_up),
      make_func_entry(expand_selective),
      make_func_entry(exchange),
      make_func_entry(exchange_up),
      make_func_entry(exchange_down),
      make_func_entry(exchange_up_down),
      make_func_entry(exchange_down_up),
      make_func_entry(converge),
      make_func_entry(converge_up),
      make_func_entry(converge_down),
      make_func_entry(converge_up_down),
      make_func_entry(converge_down_up),
      make_func_entry(per_map_picsom_all),
      make_func_entry(per_map_picsom_bottom),
      make_func_entry(per_map_vq),
      make_func_entry(goto_extract_images),
      make_func_entry(one_for_all),
      make_func_entry(one_for_bottom),
      make_func_entry(const_for_bottom),
      make_func_entry(distances_for_bottom),
      make_func_entry(entropy_n_polynomial),
      make_func_entry(entropy_n_leave_out_worst),
      make_func_entry(offline_shortcut),
      make_func_entry(extract_images_picsom_all),
      make_func_entry(extract_images_picsom_bottom),
      make_func_entry(extract_images_vq),
      make_func_entry(extract_images_sq),
      make_func_entry(remove_duplicates_picsom),
      make_func_entry(remove_duplicates_vq),
      make_func_entry(process_images_picsom_bottom),
      make_func_entry(process_images_vq),
      make_func_entry(process_images_vqw)
#undef make_func_entry
    };

    int n = sizeof(list)/sizeof(function_entry);
    if (i<0 || i>=n) {
      f = func_unknown;
      t = NULL;
      return false;
    }
  
    f = list[i].func;
    t = list[i].name;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *Query::FunctionNameP(cbir_function x) {
    for (int i=0;; i++) {
      cbir_function f;
      const char *name;
      if (!FunctionNames(i, f, name))
        break;

      if (x==f)
        return name;
    }
    static char msg[1000];
    sprintf(msg, "--unknown (%d)--", (int)x);

    return msg;
  }

  /////////////////////////////////////////////////////////////////////////////

  cbir_function Query::FunctionName(const string& n) {
    for (int i=0;; i++) {
      cbir_function f;
      const char *name;
      if (!FunctionNames(i, f, name))
        break;

      if (n==name)
        return f;
    }
    return func_unknown;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RemoveDuplicatesPicSOM() {
    Tic("RemoveDuplicatesPicSOM");

    bool debug = false;
    int na = 0, nc = 0;

    DeleteUnique();

    map<int,set<int> > possibly_in_unique;         

    for (size_t i=0; i<combinedimage.size(); i++) {
      const Object& img = combinedimage[i];
        
      //    for (j=0; j<ListSize(uniqueimage); j++)
      //      if (uniqueimage.FastGet(j)->Match(img))
      //        break;
      // if (uniqueimage.FastGet(j)->LabelsMatch(img))
      //   break;

      //    if (j==ListSize(uniqueimage)) {
      int j=-1;
      if(possibly_in_unique[img.Index()].size()>0){
        set<int> &s=possibly_in_unique[img.Index()];
        set<int>::iterator it;
        for(it=s.begin();it!=s.end();it++)
          if (uniqueimage[*it].Match(img)){
            j=*it;
            break;        
          }
      }
      if(j<0){        
        na++;
        uniqueimage.push_back(img);
        possibly_in_unique[img.Index()].insert(uniqueimage.size()-1);
      } else {
        nc++;
        uniqueimage[j].Combine(img, true);//was (img, false)
      }
    }
  
    if (debug)
      cout << "RemoveDuplicatesPicSOM() : na=" << na << " nc=" << nc
           << " combined images:" << combinedimage.size()
           << " uniqueimages:" << uniqueimage.size() << endl;

    Tac("RemoveDuplicatesPicSOM");

    return uniqueimage.size();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RemoveDuplicatesVQ() {
    DeleteUnique();
    for (size_t i=0, j; i<combinedimage.size(); i++) {
      for (j=0; j<uniqueimage.size(); j++)
        if (uniqueimage[j].Match(combinedimage[i]))
          break;

      if (j==uniqueimage.size())
        uniqueimage.push_back(combinedimage[i]);
      else 
        if (combinedimage[i].Value()>uniqueimage[j].Value())
          uniqueimage[j].Value(combinedimage[i].Value());
    }
  
    //   cout << "RemoveDuplicatesVQ(): ListSize(uniqueimage)=" 
    //        << ListSize(uniqueimage) << endl;
    return uniqueimage.size();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessObjectsPicSOMBottom(const string& arg) {
    string argcopy=arg;

    size_t proppos=argcopy.find("propagate_by_segment_hierarchy");
    bool propagate_by_segment_hierarchy=(proppos!=string::npos);

    if(proppos!=string::npos){
      // remove from string to prevent error message & trapping

      size_t eraselen=strlen("propagate_by_segment_hierarchy");
      if(proppos>0&&argcopy[proppos-1]==','){
        proppos--;
        eraselen++;
      }

      argcopy.erase(proppos,eraselen);

    }

    size_t mlpos=argcopy.find("mlweighting");
    bool mlweighting=(mlpos!=string::npos);

    if(mlpos!=string::npos){
      // remove from string to prevent error message & trapping

      size_t eraselen=strlen("mlweighting");
      if(mlpos>0&&argcopy[mlpos-1]==','){
        mlpos--;
        eraselen++;
      }

      argcopy.erase(mlpos,eraselen);

    }


    size_t mlest=argcopy.find("mlestimate");
    bool mlestimate=(mlest!=string::npos);

    if(mlest!=string::npos){
      // remove from string to prevent error message & trapping

      size_t eraselen=strlen("mlestimate");
      if(mlest>0&&argcopy[mlest-1]==','){
        mlest--;
        eraselen++;
      }

      argcopy.erase(mlest,eraselen);

    }

    size_t afterpos=argcopy.find("argafter");
    if(afterpos!=string::npos){
      int after;
      const char *cptr=argcopy.c_str()+afterpos;
      while(*cptr && *cptr !='=') cptr++;

      if(sscanf(cptr,"=%d",&after)==1){
        if(Round()<=after){
          argcopy="";
          cout << "Clearing args to ProcessImagesPicSOMBottom() on round"
               << Round() << endl; 
        }
      }
      else
        return ShowError("Query::ProcessImagesPicSOMBottom() : StageArgs <",
                         argcopy, "> could not be understood");
    }
    picsom_bottom_score_options_t opt=ParseBottomOptions(argcopy);
  
    Tic("ProcessImagesPicSOMBottom");

    bool ok=true;
    
    scorefile_pointers fp;
    OpenScoreFiles(fp);
      
    vector<float> cvals;
    vector<float> posnegvals;

    double *theta=NULL;

    if(mlweighting||mlestimate){

      vector<vector<float> > scores;
      vector<vector<float> > pos_scores;

      cout << "collecting scores for ml weighting" << endl;
      size_t poscount=0,negcount=0;
      for (size_t i=0; i<NseenObjects(); i++) {
        Object& img = seenobject[i];
        if(!(img.TargetType()&Target())){
          //cout << "rejected "<<img.Label()<<" bc of targettype."<<endl;
          continue;
        }
        if(img.Value()!=0){
          int idx = img.Index();

          if (idx<0) 
            ShowError("Query::ProcessImagesPicSOMBottom(): Index not solved for ",                    img.Label());
          
          FloatVector stvec(Nindices()+1);
          stvec.Set(MAXFLOAT);

          float val=PicSOMBottomScore(idx,opt,&stvec,&img,&cvals,NULL);

          if(mlestimate) cvals=vector<float>(1,val); // one map model
          
          //          cout << "Score components for "<<((img.Value()>0)?"positive":"negative")<<" example image " << img.Label()<<":"<<endl;
          //for(size_t c=0;c<cvals.size();c++)
          //  cout<< " "<<cvals[c];
          //cout << endl;

          if(img.Value()>0){
            poscount++;
            pos_scores.push_back(cvals);
          }
          else{
            negcount++;
            scores.push_back(cvals);
          }
        }
      }

      cout << "collected "<<poscount<<" positive and "<<negcount<<" negative scores."<<endl;
      
      for(size_t i=0;i<pos_scores.size();i++)
        scores.push_back(pos_scores[i]);

      theta=MLEstimateMapWeights(scores,negcount);

      cout << "Dumping train set probs.:" << endl;

      size_t total=scores.size(); 

      double *h=new double[total];

      ml_calculate_h(scores,theta,h);

           cout << "negatives: " << endl;
           size_t i;
      for(i=0;i<negcount;i++){
        cout << h[i] << " (";
        for(size_t j=0;j<scores[i].size();j++)
         cout << " " << scores[i][j];
        cout <<")"<<endl;
      }

          cout << "positives: " << endl;

          for(i=negcount;i<total;i++){
            cout << h[i] // << endl;
                 << " (";
            for(size_t j=0;j<scores[i].size();j++)
              cout << " " << scores[i][j];
            cout <<")"<<endl;
          }

          delete[] h;

    }

    for (size_t i=0; i<NnewObjects(); i++) {
      Object& img = newobject[i];
      int idx = img.Index();

      // cout << "processing object "<<i<<": " << img.Label()<<endl;

      if (idx<0) 
        ShowError("Query::ProcessImagesPicSOMBottom(): Index not solved for ",
                  img.Label());
      
      FloatVector stvec(Nindices()+1);
      stvec.Set(MAXFLOAT);

      img.Value(0.0);
      float val=PicSOMBottomScore(idx,opt,&stvec,&img,
                                  &cvals,&posnegvals);

      if(scorefile!="" && (opt.filedump_ttypecheck==false || 
                           img.TargetType()&Target()))
        WriteScoreFiles(fp,img.Label(),idx,
                        val,cvals,posnegvals);

      if(mlweighting)
        val=ml_calculate_h(cvals,theta);
      else if(mlestimate){
        cvals=vector<float>(1,val);
        val=ml_calculate_h(cvals,theta);
      }

      img.Value(val); 

      stvec.Last() = img.Value();
      img.HandleStageInfo(1, &stvec);
    }

    fp.close();

    for(int i=0;i<watchset.Length();i++){
      bool seen= FindInSeen(watchset[i]) != NULL;
      bool innew= IsInNewObjects(watchset[i]);
      cout << "Status of watch set image " << the_db->Label(watchset[i]) << ":";
      if(seen) cout << " seen earlier";
      else if(innew) cout <<" among considered images" ;
      else cout << " not shown";
      
      cout << endl;

      float score=PicSOMBottomScore(watchset[i],opt,NULL);
      cout << "Score for watch set image " << the_db->Label((watchset)[i])
           << " is " << score << endl;
    }
    

    if(propagate_by_segment_hierarchy){
      PropagateBySegmentHierarchy();
      if(scorefile!=""){
        string prop_fn=scorefile+"_prop";
        
        FILE *ffp;
        if((ffp=fopen(prop_fn.c_str(),"w"))==NULL)
          ShowError("Query::OpenScoreFiles(): "
                    "failed to open score file ",prop_fn);
        for (size_t i=0; i<NnewObjects(); i++) {
          Object& img = newobject[i];
          float val=img.Value();
          fprintf(ffp,"%s %f\n",img.Label().c_str(),val);
        }

        fclose(ffp);
      }
    }

    //cout << "ProcessImagesPicsomBottom finishing with new list" << endl;
    //for (int i=0; i<NnewObjects(); i++) {
    //  Object& img = newobject[i];
    //  cout << "object" << img.Label()<<": "<<img.Value()<<endl;      
    //}

    delete[] theta;

      
    Tac("ProcessImagesPicSOMBottom");
    
    return ok;
  }


  /////////////////////////////////////////////////////////////////////////////

  Query::picsom_bottom_score_options_t 
  Query::ParseBottomOptions(const string &arg) {

      picsom_bottom_score_options_t opt;

      if(arg=="") opt.add=true;

      list<pair<string,string> > pairList=SplitArgs(arg);
      pair<string,string> argpair;
      
      while(pairList.size()>0){
        argpair=pairList.front();
        pairList.pop_front();
        if(argpair.first=="add"){
          opt.add = true;
        } else if(argpair.first=="mul"){
          opt.mul = true;
        } else if(argpair.first=="zero"){
          opt.zero =true;
        } else if(argpair.first=="rho"){
          opt.rho  = true;
        } else if(argpair.first=="ratioscore"){
          opt.ratioscore = true;
        } else if(argpair.first=="smoothed_fraction"){
          opt.smoothed_fraction = true;
        } else if(argpair.first=="relevancemodulation"){
          opt.relevancemodulation = true;
        }        else if(argpair.first=="subcombination"){
          if(argpair.second=="maximum"){
            opt.maxofsubs = true;
          } else if(argpair.second=="sqrsum"){
            opt.geomsumsubs = true;
          } else if(argpair.second=="sublinearrank"){
            opt.sublinearrank = true;
          }
          else
            return ShowError("Query::ParseBottomOptions() : unknown"
                             "subcombination type ",argpair.second);
        }
        else if(argpair.first=="storesubscores"){
          opt.storesubscores = true;
        }        else if(argpair.first=="posonly"){
          opt.posonly = true;
        }        else if(argpair.first=="no_propagation"){
          opt.no_propagation = true;
        }        else if(argpair.first=="filedump_ttypecheck"){
          opt.filedump_ttypecheck=true;
        }        else if(argpair.first=="equalise_subobject_count"){
          opt.equalise_subobject_count=true;
        }else{
          ShowError("Query::ParseBottomOptions() : couldn't "
                           "process argument pair (",argpair.first,",",
                           argpair.second,")");
        }
      }

      if (opt.mul==opt.add)
        return ShowError("Query::ParseBottomOptions() : mul==add");

      if (opt.zero && opt.rho)
        return ShowError("Query::ParseBottomOptions() : zero && rho");

      if (!opt.zero && !opt.rho && !opt.add && !opt.mul)
        return ShowError("Query::ParseBottomOptions() : StageArgs <",
                       arg, "> could not be understood");

      if (opt.mul && DefaultMatrixCount()<3)
        return ShowError("Query::ParseBottomOptions() : "
                         "mul && matrixcount<3");
      

      if( (opt.maxofsubs ? 1 : 0) + (opt.geomsumsubs ? 1 : 0) +
          (opt.sublinearrank ? 1 : 0) > 1)
        return ShowError("Query::ParseBottomOptions() : "
                       "several methods for combining subobject scores given");

      if (opt.ratioscore && DefaultMatrixCount()<3)
        return ShowError("Query::ParseBottomOptions() : "
                         "ratioscore && matrixcount<3");
      if (opt.smoothed_fraction && DefaultMatrixCount()<3)
        return ShowError("Query::ParseBottomOptions() : "
                         "smoothed_fraction && matrixcount<3");
      if (opt.relevancemodulation && DefaultMatrixCount()<3)
        return ShowError("Query::ParseBottomOptions() : "
                         "relevancemodulation && matrixcount<3");
      if (opt.posonly && DefaultMatrixCount()<3)
        return ShowError("Query::ParseBottomOptions() : "
                         "posonly && matrixcount<3");

      return opt;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::OpenScoreFiles(scorefile_pointers& fp) {
    if (scorefile!="") {
      //cout << "opening scorefiles w/ prefix "<<scorefile<<endl;
      string component_fn=scorefile+"_comp", combined_fn=scorefile, 
        //reduced_fn=scorefile+"_red", 
        posneg_fn=scorefile+"_posneg"; 
        //posneg_red_fn=scorefile+"_posneg_red",
        //sum_red_fn=scorefile+"_sum_red";
   
      if ((fp.fp=fopen(combined_fn.c_str(),"w"))==NULL)
        ShowError("Query::OpenScoreFiles(): "
                  "failed to open score file ",combined_fn);

      if ((fp.fp_comp=fopen(component_fn.c_str(),"w"))==NULL)
        ShowError("Query::OpenScoreFiles(): "
                  "failed to open score file ",component_fn);
      
      if (DefaultMatrixCount()>=3){
        if((fp.fp_posneg=fopen(posneg_fn.c_str(),"w"))==NULL)
          ShowError("Query::OpenScoreFiles(): "
                    "failed to open score file ",posneg_fn);
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::StoreScores(vector<float>& s) const {
    s = vector<float>(DataBaseSize());

    for (int i=0, n=NnewObjects(); i<n; i++) {
      const Object& o = NewObject(i);
      int j = o.Index();
      s[(size_t)j] = o.Value();
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::WriteScoreFiles(const scorefile_pointers &fp, const string &lbl,
                              int /*idx*/,
                              float val, const vector<float> &cvals,
                              const vector<float> &posnegvals){
    if(fp.fp){
      fprintf(fp.fp,"%s %f\n",lbl.c_str(),val);
    }

    if(fp.fp_comp){
      fprintf(fp.fp_comp,"%s",lbl.c_str());
      for(size_t i=0;i<cvals.size();i++)
        fprintf(fp.fp_comp," %f",cvals[i]);
      fprintf(fp.fp_comp,"\n");
    }

    if(fp.fp_posneg){
      fprintf(fp.fp_posneg,"%s",lbl.c_str());
      for(size_t i=0;i<posnegvals.size();i++)
        fprintf(fp.fp_posneg," %f",posnegvals[i]);
      fprintf(fp.fp_posneg,"\n");
    }
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  float Query::PicSOMBottomScore(int idx,
                                 const picsom_bottom_score_options_t &opt,
                                 FloatVector *stvec, Object *img,
                                 vector<float> *cvals,
                                 vector<float> *posnegvals) {
    float ret;
    vector<float> rootvals;
    vector<float> subvals;
    const vector<int> sub= the_db->SubObjects(idx);
    const string &lbl = the_db->Label(idx);
  
    bool store_posneg= posnegvals && DefaultMatrixCount()>=3;

    bool si_stored = store_selfinfluence && selfinfluence.count(idx)>0 && 
      selfinfluence[idx].size()==Nindices();

    if(opt.equalise_subobject_count && (opt.add==false||opt.ratioscore)){
      ShowError("Query::PicSOMBottomScore() : equalise_subobject_count"
                " currently implemented only for additive scoring");
      return 0;
    }

    if(debug_selective){
      cout << "Calculating score for <" << lbl <<">"<< endl;  
    }

    float classw = ClassWeight(idx);
    if (debug_weights)
      cout << "Class weight = " << classw << " for:"
           << the_db->ObjectDump(idx) << endl;

    if(opt.ratioscore)
      subvals=vector<float>(sub.size(),1.0);
    else
      subvals=vector<float>(sub.size(),0.0);

    double pprod = 1, nprod = 1;
    double rscore=1;
    
    if(cvals){
      cvals->clear();
      cvals->resize(Nindices(),0.0);
    }
    if(store_posneg){
      posnegvals->clear();
      posnegvals->resize(2*Nindices(),0.0);
    } else if(posnegvals)
      posnegvals->resize(0);

    for (size_t j=0; j<Nindices(); j++) {
      double v = 0;

      if (IsInverseIndex(j))
        v = BinaryFeatureScoreForObject(j, idx);

      else if (IsPreCalculatedIndex(j))
        v = PreCalculatedScoreForObject(j, idx);

      else if (IsTsSom(j)) {
        int bot = NlevelsEvenAlien(j)-1;
        const TreeSOM &ts = MapEvenAlien(j, bot);
        const IntVector& div = Division(j, bot);

        const TSSOM::State& fd = IndexDataTSSOM(j);
        
        float weight = fd.tssom_level_weight[bot];
        
        // OBS! this is a hack
        
        weight *= Weight(j);
        
        if (weight==Index::State::nan()) {
          WarnOnce("Query::PicSOMBottomScore() : "
                   "feature weight == NaN detected for "+
                   TsSom(j).MapName(false, bot));
          weight = 0.0;
        }

        int positives = 0, negatives = 0;         
        double bias = DetermineCountBias();
        if(opt.ratioscore||opt.relevancemodulation||opt.smoothed_fraction)
          CountPositivesAndNegatives(div, positives, negatives, true);
        
        int mapidx = div.IndexOK(idx) ? div[idx] : -1;
        
        if(mapidx>=0){
          
          IntPoint p = ts.ToPoint(mapidx);
          
          float sij=0;
          if(si_stored){
            sij=selfinfluence[idx][j];
            //              cout << "Self-influence for root object "
            //                   << the_db->Label(idx)
            //                   << ": "<<si<<endl; 
          }
          
          v = opt.add ? (Convolved(j, bot, 0).Get(p.Y(), p.X())-sij) : 0;

          if(opt.smoothed_fraction){
            const float bbias=0.05;
            double pos = positives*Convolved(j, bot, 1).Get(p.Y(), p.X());
            double neg = -negatives*Convolved(j, bot, 2).Get(p.Y(), p.X());
            v=(pos+bbias)/(pos+neg+2*bbias);
          }

          if(store_posneg){
            
            (*posnegvals)[2*j]=Convolved(j, bot, 1).Get(p.Y(), p.X());
            (*posnegvals)[2*j+1]=Convolved(j, bot, 2).Get(p.Y(), p.X());
            if(sij>0)
              (*posnegvals)[2*j] -= sij;
            else
              (*posnegvals)[2*j+1] -= sij;
          }

          if(debug_selective)
            cout << "root object score: " << v <<endl; 

          if(opt.ratioscore||opt.relevancemodulation){

            double pos = positives*Convolved(j, bot, 1).Get(p.Y(), p.X());
            double neg = -negatives*Convolved(j, bot, 2).Get(p.Y(), p.X());
            
            if(opt.ratioscore){
              rscore *= (pos+bias)/(neg+bias);
              if(cvals) (*cvals)[j]= (pos+bias)/(neg+bias);
            }
            if(opt.relevancemodulation){
              v *= (pos+bias)/(neg+bias);        
              if(debug_selective)
                cout << " * relevance multiplier " << (pos+bias)/(neg+bias) 
                     << " ->"<<v <<endl; 
            }
          }
        
          if (opt.mul) {
            pprod *=  Convolved(j, bot, 1).Get(p.Y(), p.X());
            nprod *= -Convolved(j, bot, 2).Get(p.Y(), p.X());
          }

        } else v=0; // mapidx >= 0

        if (opt.no_propagation==false /*use_subobjects*/) {

          int subcount=0;

          if(opt.equalise_subobject_count) // count subobjects on this map
            for (size_t subi=0; subi<sub.size(); subi++) 
              if(div.IndexOK(sub[subi])&&div[sub[subi]]>=0) subcount++;

          for (size_t subi=0; subi<sub.size(); subi++) {
            
            int mapidx_sub = div.IndexOK(sub[subi]) ? div[sub[subi]] : -1;
            if(mapidx_sub>=0){
              IntPoint p = ts.ToPoint(mapidx_sub);
              
              float ssi=0;
              bool ssi_stored = store_selfinfluence &&
                selfinfluence.count(sub[subi])>0 && 
                selfinfluence[sub[subi]].size()==Nindices();
              
              if(ssi_stored){
                ssi=selfinfluence[sub[subi]][j];
              }

              if (opt.add){
                float multiplier=1.0*weight;
                if(opt.ratioscore||opt.relevancemodulation){
                  double pos = positives*Convolved(j, bot, 1).Get(p.Y(), p.X());
                  double neg = -negatives*Convolved(j, bot, 2).Get(p.Y(), p.X());
                  if(opt.ratioscore){
                    subvals[subi] *= (pos+bias)/(neg+bias);
                    if(cvals) (*cvals)[j]*= (pos+bias)/(neg+bias);
                  }

                  if(opt.relevancemodulation)
                    multiplier = (pos+bias)/(neg+bias);
                }
                
                if(debug_selective){
                  cout << "   subobject " << the_db->Label(sub[subi])
                       << " ->("<<p.X()<<","<<p.Y()<<") on ("<<j<<","
                       <<bot<<"):"<< endl;
                  cout << "    " << multiplier <<"*"
                       << Convolved(j, bot, 0).Get(p.Y(), p.X()) <<"="
                       << multiplier*Convolved(j, bot, 0).Get(p.Y(), p.X())
                       << endl;
                }
              
                float subscore=multiplier*
                  Convolved(j, bot, opt.posonly ? 1 :0).Get(p.Y(), p.X());
                
                if(opt.posonly==false || ssi>0)
                  subscore -= multiplier*ssi;
                
                if(opt.smoothed_fraction){
                  const float bbias=0.05;
                  double pos = positives*Convolved(j, bot, 1).Get(p.Y(), p.X());
                  double neg = -negatives*Convolved(j, bot, 2).Get(p.Y(), p.X());
                  subscore=(pos+bbias)/(pos+neg+2*bbias);
                }
                
                if(opt.equalise_subobject_count){
                  subscore /= subcount;
                  if(debug_selective)
                    cout << "Map "<<IndexFullName(j)<<":"
                      "dividing by count "<<subcount<<endl; 
                }
                
                subvals[subi] += subscore;
                if(cvals) (*cvals)[j] += subscore;
                if(store_posneg){
                  (*posnegvals)[2*j]  +=Convolved(j, bot, 1).Get(p.Y(), p.X());
                  (*posnegvals)[2*j+1]+=Convolved(j, bot, 2).Get(p.Y(), p.X());
                  if(ssi>0)
                    (*posnegvals)[2*j] -= ssi;
                  else
                  (*posnegvals)[2*j+1] -= ssi;
                }
              }
              if (opt.mul)
                ShowError("Query::PicSOMBottomScore() : cannot handle ",
                          "multiplication of subobjects");

            } // mapidx_sub >= 0

            
          }
        } // if use_subobjects
        float vwas = v;
        v *= (weight*classw);
        if (debug_weights)
          cout << "2nd stage weighting: " << weight << " * " << classw
               << " * " << vwas << " = " << v << endl;
      }

      rootvals.push_back(v); // this is senseless if add==false
      if(opt.add && cvals) (*cvals)[j]+=v;

      if(stvec)
        (*stvec)[j] = v;
    }
  
    if(img && opt.storesubscores){
      img->rel_distribution.subind=sub;
      img->rel_distribution.subval=subvals;
    }
  
    if(subvals.size()==0)
      {
        if(debug_selective)
          cout << "no subobjects, returning root score" << endl;
        ret =accumulate(rootvals.begin(),rootvals.end(),0.0); 
      }
    else{
      if(opt.maxofsubs){
        ret =accumulate(rootvals.begin(),rootvals.end(),0.0)+ 
          *max_element(subvals.begin(),subvals.end());
      }
      else if(opt.geomsumsubs){
        for(size_t ind=0;ind<subvals.size();ind++)
          if(subvals[ind]>0)
            subvals[ind] *= subvals[ind];
          else
            subvals[ind]=0;
        ret =accumulate(rootvals.begin(),rootvals.end(),0.0)+ 
          sqrt(accumulate(subvals.begin(),subvals.end(),0.0));
      }
      else if(opt.sublinearrank){
        sort(subvals.begin(),subvals.end());

        float subscore=0;
        for (size_t rank=1; rank<=subvals.size(); rank++)
          subscore += subvals[subvals.size()-rank]/rank;
    
        ret = accumulate(rootvals.begin(),rootvals.end(),0.0)+subscore; 
      }
      else // default is to sum the contributions of subobjects
        ret = accumulate(subvals.begin(),subvals.end(),0.0)+
          accumulate(rootvals.begin(),rootvals.end(),0.0);
    }

    if (opt.mul && opt.zero)
      ret=pprod;
  
    if (opt.mul && opt.rho) {
      // obs! NpositiveSeenImages() may not return what we expect...
      double tot = NseenObjects();
      double mul1= NpositiveSeenObjects()/tot, mul2= NnegativeSeenObjects()/tot;
      ret=mul1*pprod-mul2*nprod;
    }

    if (opt.mul && !opt.zero && !opt.rho)
      ret=pprod-nprod;
  
    if(debug_selective){
      cout << "Total score returned is: " << ret << endl;
    }

    return ret;
    //if(ratioscore){
    //  double maxval=*max_element(subvals.begin(),subvals.end());
    //  double average = accumulate(subvals.begin(),subvals.end(),0.0);
    //  average /= sub.size();
    //  img.Value(rscore+maxval+average);
    //}
}
   
/////////////////////////////////////////////////////////////////////////////
  double *Query::MLEstimateMapWeights(const vector<vector<float> > &scores,
                                     size_t negatives){

    const int maxiter = 100;
    int itercount=0;

    if(scores.size()==0) return NULL;

    const size_t nmaps=scores[0].size();
    const size_t total=scores.size();


    double *h=new double[total],*old_h=new double[total];
    double *theta=new double[nmaps+1],*old_theta=new double[nmaps+1];
    double *gradient=new double[nmaps+1];

    typedef double *dptr;
    dptr *hessian=new dptr[nmaps+1];

    for(size_t i=0;i<nmaps+1;i++)
      hessian[i]=new double[nmaps+1];

    double stepsize=300;
    double old_val;

    
    const double stopratio=0.001;
    const double fvallimit=-0.0001;
    double deltaf;
    double fval;

    ml_initTheta(scores,old_theta);

    ml_calculate_h(scores,old_theta,old_h);
    //cout << "h calculated, total-negatives=" <<total-negatives<< endl;
    ml_calculate_gradient(scores,negatives,old_h,gradient);

    ml_calculate_hessian(scores,old_h,hessian);

    old_val=ml_calculate_fval(old_h,negatives,total);

    //cout << "Initial: "<<endl;
    //cout << "fval="<<old_val<<endl;

    do{ 

      for(size_t i=0;i<nmaps+1;i++){
        theta[i]=old_theta[i];
        for(size_t j=0;j<nmaps+1;j++)
          theta[i] -= hessian[i][j]*gradient[j];
        //        cout << "step["<<i<<"]="<<theta[i]-old_theta[i]<<endl;
      }
    
      ml_calculate_h(scores,theta,h);

      fval=ml_calculate_fval(h,negatives,total);

      double sd=0;
      for(size_t i=0;i<nmaps+1;i++){
        double d=theta[i]-old_theta[i];
        sd += d*d;
      }

      stepsize=sqrt(sd);

      cout << "stepsize "<<stepsize<<" -> fval="<<fval<<endl;
      
      deltaf=abs((fval-old_val)/old_val);
      
      while(!(fval>old_val) && deltaf>stopratio && fval<fvallimit ){
        
        //cout << "Rejected step (";
//         for(size_t i=0;i<nmaps+1;i++)
//           cout <<old_theta[i]<<" ";
        
//         cout <<") -> (";
//         for(size_t i=0;i<nmaps+1;i++)
//           cout <<theta[i]<<" "; 
        
//         cout <<") fval "<<old_val
//              <<" -> "<<fval<<" step="<<stepsize<< endl;

        for(size_t i=0;i<nmaps+1;i++)
          theta[i]=0.5*(theta[i]+old_theta[i]);

        stepsize *= 0.5;
        ml_calculate_h(scores,theta,h);
        
        fval=ml_calculate_fval(h,negatives,total);
        deltaf=abs((fval-old_val)/old_val);
      }

      deltaf=abs((fval-old_val)/old_val);

      if(fval>old_val){
        
//         cout << "Accepted step (";
//         for(size_t i=0;i<nmaps+1;i++)
//           cout <<old_theta[i]<<" ";
        
//         cout <<") -> (";
//         for(size_t i=0;i<nmaps+1;i++)
//           cout <<theta[i]<<" "; 

//         cout <<") fval "<<old_val
//              <<" -> "<<fval<<" step="<<stepsize<< endl;


        double *ptr=old_theta;
        old_theta=theta;
        theta=ptr;
        
        ptr=old_h;
        old_h=h;
        h=ptr;
        
        ml_calculate_gradient(scores,negatives,old_h,gradient);
        ml_calculate_hessian(scores,old_h,hessian);

        old_val=ml_calculate_fval(old_h,negatives,total);

      }
   
    } while (itercount++<maxiter && deltaf>stopratio && fval < fvallimit);


    if(itercount>maxiter){
      //      cout << "EM max. iterations reached, returning zeros"<<endl;
      cout << "EM max. iterations reached"<<endl;
      //      for(size_t i=0;i<nmaps+1;i++)
      //        theta[i]=0;
    }


    cout << "EM terminated with the result theta=[";
    for(size_t i=0;i<nmaps+1;i++)
      cout <<old_theta[i]<<" ";
    cout << "]"<<endl;

    for(size_t i=0;i<nmaps+1;i++)
      delete[] hessian[i];

    delete[] hessian; 

    delete[] old_theta;
    delete[] h;
    delete[] old_h;

    return theta;

  }

///////////////////////////////////////////////////////////////////////////

void Query::ml_initTheta(const vector<vector<float> > &scores, double *tgt){

  double min,max,min_sum=0,max_sum=0;

  const int nmaps=scores[0].size();

  for(int k=0;k<nmaps;k++){
   min=max=scores[0][k];

   for(size_t i=1;i<scores.size();i++){
     if(scores[i][k]<min) min=scores[i][k];
     if(scores[i][k]>max) max=scores[i][k];
   }
  
   //tgt[k+1]=-2.2*(max+min)/(max-min);
   tgt[k+1]=0;
 
   min_sum += min;
   max_sum += max;

  }

  tgt[0]=0;

}

///////////////////////////////////////////////////////////////////////////

void Query::ml_calculate_h(const vector<vector<float> > &scores,
                         const double *theta, double *tgt){
  //cout <<"calculate_h()..."<<endl;

  const size_t n=scores.size();
  const size_t nmaps=scores[0].size();

  for(size_t i=0;i<n;i++){
    double u=theta[0];
    for(size_t k=0;k<nmaps;k++){
      u +=scores[i][k]*theta[k+1];
    }
    tgt[i]=1/(1+exp(-u));
  }    
}
///////////////////////////////////////////////////////////////////////////

float Query::ml_calculate_h(const vector<float> &score,
                         const double *theta){
  //cout <<"calculate_h()..."<<endl;

  const size_t nmaps=score.size();

  double u=theta[0];
  for(size_t k=0;k<nmaps;k++){
    u +=score[k]*theta[k+1];
  }
   
  return 1.0/(1+exp(-u));
    
}

///////////////////////////////////////////////////////////////////////////

  void Query::ml_calculate_gradient(const vector<vector<float> > &scores, size_t negcount,
                        const double *hval,
                        double *tgt){

  const size_t total=scores.size();
  const size_t nmaps=scores[0].size();

  for(size_t k=0;k<=nmaps;k++)
    tgt[k]=0;


  size_t i;
  for(i=0;i<negcount;i++){
    const int idx=i;
    tgt[0] -= hval[idx];
    for(size_t k=0;k<nmaps;k++)
      tgt[k+1] -= hval[idx]*scores[idx][k];
  }
  for(;i<total;i++){
    const int idx=i;
    tgt[0] += 1-hval[idx];
    for(size_t k=0;k<nmaps;k++)
      tgt[k+1] += (1-hval[idx])*scores[idx][k];
  }
}

///////////////////////////////////////////////////////////////////////////

void Query::ml_calculate_hessian(const vector<vector<float> > &scores,
                              const double *hval,
                              double **tgt){

  const size_t total=scores.size();
  const size_t nmaps=scores[0].size();

  for(size_t k=0;k<=nmaps;k++)
    for(size_t kk=0;kk<=nmaps;kk++)
      tgt[k][kk]=0;
    

  for(size_t i=0;i<total;i++){
    const int idx=i;
    double common=hval[idx]*(1-hval[idx]);
       // cout <<"common:"<<common<<endl;
     // d�Q/dt0�
    tgt[0][0] -= common;
    
       // d�Q/(dt0dtl)
    for(size_t l=0;l<nmaps;l++)
      tgt[l+1][0] -= common*scores[idx][l];
    
    for(size_t k=0;k<nmaps;k++)
      for(size_t l=k;l<nmaps;l++)
        tgt[l+1][k+1] -= common*scores[idx][l]*scores[idx][k];
  }


  // augment the upper triangle of hessian by symmetry

  for(size_t k=0;k<nmaps;k++)
    for(size_t l=k+1;l<nmaps;l++)
      tgt[k][l]=tgt[l][k];


  // then invert the hessian

  // we have to offset the indices

  typedef double *dptr; 

  dptr *tgtp=new dptr[nmaps+1];
 for(size_t i=0;i<nmaps+1;i++)
   tgtp[i]=tgt[i]-1;

 InvertMatrix(tgtp-1,nmaps+1);

 delete[] tgtp;


}

///////////////////////////////////////////////////////////////////////////

double Query::ml_calculate_fval(const double *hval,size_t negcount,size_t total){
  //cout <<"calculate_fval()..."<<endl;
  double val=0;
  size_t i;
  for(i=0;i<negcount;i++){
    const int idx=i;
    val += log(1-hval[idx]);
  }
  for(;i<total;i++){
    const int idx=i;
    val += log(hval[idx]);
  }
  
  return val;
}
    

/////////////////////////////////////////////////////////////////////////////

bool Query::PropagateBySegmentHierarchy() {

  // implements (eventually) propagation of score along segment hierarchy

  // currently the following simple model is used:

  // For each image the average is subtracted from the 
  // raw scores of the segments. Then scores are normalised by dividing
  // wit the most positive score

  // let pos=score if score > 0, 0 otherwise
  //     neg=-score if score < 0, 0 otherwise  
 
  // the score is given by following resursion

  // score of a combined segment p is given by 
  // pos(p)*(pos(p)+pos(c1)+pos(c2))-neg(p)(neg(p)+neg(c1)+neg(c2)
  // where c1 and c2 are the (recursively calculated) scores 
  // of immediate children of p,
  // p denotes 
  // the normalised score of the parent

  // alternatively, use the propagation formula
  // sign(pos(p))*(pos(p)+pos(c1)+pos(c2))-sign(neg(p))(neg(p)+neg(c1)+neg(c2)

  // score of a leaf segment is directly it's normalised score 

  // after propagation the scores within the image are
  // normalised to unit interval if postnorm==true

  bool postnorm=true;
  bool debug=false;
  bool  alternativeformula=true;

  map<string,vector<int> > prefixes;
  vector<int> emptyvec;

  // collect the prefixes method:number into a map of vectors
  // vectors hold indices to newobject
  if(debug)
  cout << "Now in Query::PropagateBySegmentHierarchy()" << endl;

 for (size_t i=0; i<NnewObjects(); i++) {
   Object& img = newobject[i];

   if (!img.Retained())
     continue;
   if ((int(img.TargetType())&int(target_segment))==0)
     continue;

   size_t prefend = img.Label().find("_");
   if (prefend==string::npos)
     continue;

   string pref=img.Label().substr(0, prefend);
   if (!prefixes.count(pref))
     prefixes[pref] = emptyvec;
   prefixes[pref].push_back(i);
 }

 if(debug)
   cout << "prefixes collected" << endl;

 map<string,vector<int> >::iterator it;
 for(it=prefixes.begin();it!=prefixes.end();it++){
   // for each prefix (~image)
   const vector<int> &v=it->second;

   if(debug)
     cout << "propagating score among prefix " << it->first << endl;

   int n=v.size();

   // parse sets of primitive segments from labels

   vector<set<int> > primi(n);

   double avg=0, maxval=newobject[v[0]].Value();

   for(int i=0;i<n;i++){
     if(debug)
       cout << "object "<<i<<" -> seenimage["<<v[i]<<"]"<<endl;
     Object& o = newobject[v[i]];
     const char *l=o.Label().c_str();

     //cout << "parsing primiset from label " << l << endl;

     while(*l && *l!='_') l++;
     if(*l==0) {
       cout << "_ not found, proceeding to next label" << endl;
       continue;
     }
     l++;
     do{
       int d;
       if(sscanf(l,"%d",&d)==1)
         primi[i].insert(d);
       while(isdigit(*l)) l++;
       while(*l && !isdigit(*l))l++;
     }while(*l);

     avg += o.Value();
     if(o.Value()>maxval) maxval=o.Value();

     if(debug){
       cout<<" ->";
     
       set<int>::iterator iit;
       for(iit=primi[i].begin();iit!=primi[i].end();iit++)
         cout <<" "<<*iit;
       cout << endl;
     }
   }
   if(debug) cout << "primisets parsed" << endl;

   avg /= n;
   maxval -= avg;

   if(debug) cout << "maxval="<<maxval<<", avg="<<avg<<endl;

   if(maxval==0){ // all values are the same !
     avg -= 1;
     maxval=1;
   }
 
   // calculate leaf segment scores and initialise active list

   vector<int> active(n,0),remaining(n,0);

   int remainingcount=0;

   for(int i=0;i<n;i++){
     if(primi[i].size()==1){
       active[i]=1;
       Object& o = newobject[v[i]];
       o.Value((o.Value()-avg)/maxval);
          if(debug) cout << "score for leaf node " << o.Label() << ": "
                         <<o.Value()<<endl;
     }
     else{
       remaining[i]=1;
       remainingcount++;
     }

     // cout << newobject[v[i]].Label() << " " << remaining[i]<<" "<<active[i]<<endl; 

   }

   // cycle through remaining
   int size=2;
  
   while(remainingcount){

     for(int rptr=0;rptr<n;rptr++){
      //  cout << "size="<<size<<endl<<"active:";
//        for(int i=0;i<n;i++)
//          cout<<" "<<active[i];
//        cout<<endl<<"remaining: ";  
//        for(int i=0;i<n;i++)
//          cout<<" "<<remaining[i];
//        cout<<endl;
     


       if(!remaining[rptr]) continue;
       if((int)primi[rptr].size()==size){
         // found parent node, now find children
         Object& o = newobject[v[rptr]];
         double rootscore=(o.Value()-avg)/maxval;

         if(debug)
           cout << "normalised score for combined node " << o.Label() << ": "
                << rootscore<<" (raw score :"<< o.Value()<<")"<<endl;

         int aptr1,aptr2;
         for (aptr1=0;
              active[aptr1]==0||
                primi[rptr].count(*primi[aptr1].begin())==0;
              aptr1++) {}

         for (aptr2=aptr1+1;
              active[aptr2]==0||
                primi[rptr].count(*primi[aptr2].begin())==0;
              aptr2++) {}

         // children found

         active[aptr1]=active[aptr2]=0;
         active[rptr]=1;
         remaining[rptr]=0;
         remainingcount--;

         double sc1=newobject[v[aptr1]].Value();
         double sc2=newobject[v[aptr2]].Value();

         // calculate score
         if(rootscore>0){
           double val=rootscore;
           if(sc1>0) val += sc1;
           if(sc2>0) val += sc2;

           if(!alternativeformula)
             rootscore *= val;
           else
             rootscore=val;
         } else if(rootscore<0){
           double val=-rootscore;
           if(sc1<0) val -= sc1;
           if(sc2<0) val -= sc2;

           if(!alternativeformula)
             rootscore *= val;
           else
             rootscore=val;
         }

         o.Value(rootscore);
         if(debug){
           cout << "score for combined node " << o.Label() << ": "<<o.Value()<<endl;
           cout<<"(children:"<<newobject[v[aptr1]].Label()
               <<","<<newobject[v[aptr2]].Label()<<endl;
         }

       }
     }

     size++;
   }

   // possibly post-normalise scores within an image;

   if(postnorm){
     float maxv,minv;

     maxv=minv=newobject[v[0]].Value();

     for(int i=1;i<n;i++){
       Object& o = newobject[v[i]];
       if(o.Value()>maxv) maxv=o.Value();
       if(o.Value()<minv) minv=o.Value();
     }
     
     for(int i=0;i<n;i++){
       Object& o = newobject[v[i]];
       o.Value((maxv!=minv)? (o.Value()-minv)/(maxv-minv) : 0.5);
     }
   }

 }

 return true;
} 
   
  /////////////////////////////////////////////////////////////////////////////

  bool Query::StoreSubobjectContribution(const string& arg) {
    //bool add = arg=="" || arg.find("add")!=string::npos;
    //bool mul = arg.find("mul")!=string::npos;;
    //if (mul==add)
    //  return ShowError("Query::ProcessImagesPicSOMBottom() : mul==add");

    //if (mul && MatrixCount()!=3)
    //  return ShowError("Query::ProcessImagesPicSOMBottom() : "
    //                     "mul && matrixcount!=3");

    Tic("StoreSubobjectContribution");

    bool ok = true;

    // estimate contributions for new images

    for (size_t i=0; i<NnewObjects(); i++) {
      if (newobject[i].Retained())
        StoreSubobjectContribution(newobject[i],arg);
    }

    // recalculate estimates for images seen earlier
    for (size_t i=0; i<NseenObjects(); i++) {
      if (seenobject[i].Retained())
        StoreSubobjectContribution(seenobject[i],arg);
    }

    Tac("StoreSubobjectContribution");

    return ok;

  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::StoreSubobjectContribution(Object &img,const string& /*arg*/) {
    int idx = img.Index();

    if (DefaultMatrixCount()<3)
      ShowError("Query::StoreSubobjectContribution(): matrixcount < 3");
  
    if (idx<0) 
      ShowError("Query::StoreSubobjectContribution(): Index not solved for ",
                img.Label());

    vector<float> subvals;
    const vector<int> sub= the_db->SubObjects(idx);
    if(sub.size()){
      subvals = vector<float>(sub.size(),1.0);
      //    sub_neg = vector<float>(sub.size(),0.0);
      double val=1.0;
      
      for (size_t j=0; j<Nindices(); j++) {
        if (IsTsSom(j)){ 
          // binary feature case not yet implemented
        
          int bot = NlevelsEvenAlien(j)-1;
          const TreeSOM &ts = MapEvenAlien(j, bot);
          const IntVector& div = Division(j, bot);

          //        int positives=0, negatives=0;         
          //CountPositivesAndNegatives(div, positives, negatives, true);

          // to undo the normalisation
          //         cout << "De-normalising w/ positives="<<positives<<" negatives="
          //              <<negatives<<endl;
  
          IntPoint p = ts.ToPoint(div[idx]);

          double pos =  Convolved(j, bot, 1).Get(p.Y(), p.X());
          double neg = -Convolved(j, bot, 2).Get(p.Y(), p.X());

          if (GetMatrixCount(j)>=4)
            pos = Convolved(j, bot, 3).Get(p.Y(), p.X());

          double bias = DetermineCountBias();

          val *= (pos+bias)/(neg+bias);

          for (size_t subi=0; subi<sub.size(); subi++) {
            p = ts.ToPoint(div[sub[subi]]);

            if(debug_selective)
              cout << "On map "<<j<<" "<<Label(sub[subi])<<" maps to ("<<p.X()<<
                ","<<p.Y()<<")" <<endl;

            pos =  Convolved(j, bot, 1).Get(p.Y(), p.X());
            neg = -Convolved(j, bot, 2).Get(p.Y(), p.X());
            if (GetMatrixCount(j)>=4)
              pos=Convolved(j, bot, 3).Get(p.Y(), p.X());          

            if(debug_selective){
              //            cout << "subval before: " << subvals[subi] << endl;
              cout << "pos="<<pos<<" neg="<<neg<<" bias="<<bias<<endl;
            }

            subvals[subi] *= (pos+bias)/(neg+bias);
            //          if(debug_selective)
            //cout << "subval after: " << subvals[subi]<< endl;
          }
        }
      }

      img.rel_distribution.rootval=val;
    
      // some avoidable copying follows
      img.rel_distribution.subind=sub;
      img.rel_distribution.subval=subvals;
    
      if (debug_selective) {
        cout << "Storing relevance distribution for image w/ label " 
             << img.Label() << endl;
        for (size_t s=0; s<subvals.size(); s++)
          cout << "subobj " << Label(sub[s]) << ": " << subvals[s] << endl;
      }
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  double Query::DetermineCountBias() const {

    // determine the histogram count bias based on the 
    // concolution kernel

    // bias = 0.5*sum _{kernel} / |support_kernel|;  

    // currently a placeholder that always returns the value
    // for a triangle-8 convolution mask:
    // bias = [(K*K+3K+1)/((K+1)(2K+1))]^2 | K=8


    return 0.5*7921.0/23409.0;
  }

  /////////////////////////////////////////////////////////////////////////////

  float Query::ClassWeight(int idx) const {
    if (classweight.empty())
      return 1;

    for (list<pair<ground_truth,float> >::const_iterator i=classweight.begin();
         i!=classweight.end(); i++)
      if (i->first[idx]==1)
        return i->second;

    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessObjectsVQ(const string&, bool weighted) {
    bool ok = true;

    Tic("SeenImageIndices");    
    IntVector posidx = SeenObjectIndices(true);
    Tac("SeenImageIndices");
  
    DoubleVector sumdist(NnewObjects());
    DoubleVector weights(Nindices());

    for (size_t i=0; i<Nindices(); i++) {
      Tic("DataByIndices");
      FloatVectorSet posdata = TsSom(i).DataByIndices(posidx);
      Tac("DataByIndices");
    
      if (!posdata.Nitems()) {
        ShowError("Query::ProcessImagesVQ(): DataByIndices() failed.");
        SimpleAbort();
      }

      if (weighted && Nindices()>1) { // VQW algorithm (if more than one map) 
        for (int j=0; j<posdata.Nitems(); j++) {
          Tic("MeanSquaredDistance");
          weights[i] += posdata.MeanSquaredDistance(posdata[j]);
          Tac("MeanSquaredDistance");
        }      
        double md = TsSom(i).MeanDistance();
        if (md<=0.0) {
          ShowError ("Meandistance of ", IndexShortName(i), " is invalid: ",
                     ToStr(md), " (1)");
          SimpleAbort();
        }
        if (posdata.Nitems() > 1) // distance calculation needs >1 points
          weights[i] = (md * posdata.Nitems()) / weights[i];      
        else 
          weights[i] = 1.0;
        // cout << weights[i] << "\t";

      } else // VQ algorithm
        weights[i] = 1.0;
    
      Tic("DistancesToPositives");
      sumdist += DistancesToPositives(posdata, i) * weights[i];
      Tac("DistancesToPositives");
    }
    // cout << endl;
  
    for (size_t j=0; j<NnewObjects(); j++) 
      newobject[j].Value(1/sumdist[j]);
  
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PerMapPicSOM(const string& args, int i, int j, bool) {
    CreateAllClassModels();

    if (IsInverseIndex(i) && j==0)
      return SimpleBinaryFeatureScores(i);

    if (IsPreCalculatedIndex(i) && j==0)
      return SimplePreCalculatedScores(i);

    if (IsSvm(i) && j==0)
      return SimpleSvmScores(i);

    if (GetMatrixCount(i)!=1 && GetMatrixCount(i)<3)
      return ShowError("Query::PerMapPicSOM() : matrixcount != 1 and <3");

    bool trad = false, zero = false, rho = false;
    if (GetMatrixCount(i)>=3) {
      if (args=="")
        trad = true;
      else if (args.find("zero")!=string::npos)
        zero = true;
      else if (args.find("rho")!=string::npos)
        rho = true;
      else
        return ShowError("Query::PerMapPicSOM() : StageArgs <", args,
                         "> not understood");
    }

#ifdef QUERY_USE_PTHREADS
    char txt[1000];
    sprintf(txt, "Query::PerMapPicSOM(%d, %d)", i, j);
    picsom->ConditionallyAnnounceThread(txt, query_main_thread, gettid());
    bool do_tictac = !picsom->PthreadsTssom();
#else
    bool do_tictac = true;
#endif // QUERY_USE_PTHREADS

    bool ok = true;

    if (do_tictac)
      Tic("PerMapPicSOM");

    if (!PlaceSeenOnMap(i, j, mapnormalize, limitrelevance)) {
      char tmp[100];
      sprintf(tmp, "(%d,%d)", i, j);
      ShowError("Query::PerMapPicSOM", tmp, ": PlaceSeenOnMap() failed");
      ok = false;
    }
    
    bool si=store_selfinfluence && Map(i, j).IsBottomLevel();

    if (ok) {
      if (do_tictac)
        Tic("Convolve");

      if (hitstype==bernoullimap) {
        if (GetMatrixCount(i)<3) {
          ShowError("Query::PerMapPicSOM: hitstype bernoullimap"
                      " requires matrixcount>=3");
        }
        BernoulliMAP(i,j);

      } else { //hitstype != bernoullimap
        if (GetMatrixCount(i)==1) {
          ok = Convolve(i, j, 0);
          DoClassModelAugmentation(i, j, 0);

        } else {                  // matrixcount>=3

          ok = Convolve(i, j, 1) && Convolve(i, j, 2);

          if (hitstype==smoothed_fraction)
            ok = ok && Convolve(i, j, 0);
          
          if (GetMatrixCount(i)>=4)
            ok = ok && Convolve(i, j, 3);
 
          if (trad) {
            if (hitstype != smoothed_fraction) {
              Convolved(i, j, 0) = Convolved(i,j,1) + Convolved(i,j,2);
              DoClassModelAugmentation(i, j, 0);
            }

            ok = true;
          
          } else {
            if (si)
              ShowError("Query::PerMapPicSOM: for matrixcount != 1 only"
                        " traditional combining implemented");
            double tmp = Convolved(i, j, 1).Sum();
            if (tmp!=0 && tmp!=1) Convolved(i, j, 1) /= tmp;
            tmp = Convolved(i, j, 2).Sum();
            if (tmp!=0 && tmp!=-1) Convolved(i, j, 2) /= -tmp;
            double mul1 = 1, mul2 = 1;
            if (zero)
              mul2 = 0;

            else if (rho==true) { // this is always true...
              // obs! NpositiveSeenImages() may not return what we expect...
              double tot = NseenObjects();
              mul1 = NpositiveSeenObjects()/tot;
              mul2 = NnegativeSeenObjects()/tot;
            }
            Convolved(i, j, 0) = Convolved(i,j,1)*mul1 + Convolved(i,j,2)*mul2;
          }
        }
      
        if (si) {
        // multiply influences by a location specific factor

          bool debug=false;
          
          for (size_t ii=0; ii<NseenObjects(); ii++) {
            int idx = seenobject[ii].Index();
            if(idx<0) continue;
            int mapidx = Division(i, j)[idx];
            if(mapidx>=0 ){
              if(!selfinfluence.count(idx))
                ShowError("Query::PerMapPicSOM: selfinfluence not found for"
                          " seen object",seenobject[ii].Label());
              const TreeSOM& ts = MapEvenAlien(i, j);
              IntPoint p = ts.ToPoint(mapidx);
              
              if(debug)
                cout << "Multiplying self-influence on map "<<i
                     <<" level="<<j<<endl;

              char locstring[80];
              sprintf(locstring,"(%d,%d)",p.X(),p.Y());
              if((int)location_selfinfluence.size()<=i ||
                 (int)location_selfinfluence[i].size()<=p.X() ||
                 (int)location_selfinfluence[i][p.X()].size()<=p.Y())
                ShowError("Query::PerMapPicSOM: location_selfinfluence "
                          " not found for point ",locstring,".");

              if(debug)
                cout << "Self-influence of " << seenobject[ii].Label()
                     << " on map "<<i<<" "<<locstring<<": "
                     << selfinfluence[idx][i] << " * " 
                     << location_selfinfluence[i][p.X()][p.Y()] << " = ";

              selfinfluence[idx][i] *= location_selfinfluence[i][p.X()][p.Y()];

              if(debug)
                cout << selfinfluence[idx][i] <<endl;
            }
          }
        }
      } // hitstype != bernoullimap
 
      bool bottom_only = true;
      if (!bottom_only || Map(i, j).IsBottomLevel())
        MapSnapshots(i, j);

      if (do_tictac) {
        Tac("Convolve");
        Tic("BestVQUnits");
      }

      if (ok)
        PerMapVQUnit(i, j) = BestVQUnits(i, j);

      if (do_tictac)
        Tac("BestVQUnits");
    }

    if (do_tictac)
      Tac("PerMapPicSOM");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoClassModelAugmentation(int ii, int j, int k) {
    const string& mapname = IndexShortName(ii);

    set<string> classes = GetAugmentClasses(mapname);
    
    for (set<string>::iterator it=classes.begin(); it!=classes.end(); it++) {
      const string& classname = *it;

      // MATS: why can empty string come here?
      if (classname=="")
        continue;

//       float my_value = GetAugmentValue(classname, mapname);
//       float default_value = DefaultQuery()->GetAugmentValue(classname,mapname);
      float value = GetAugmentValue(classname, mapname);

      if (debug_classify)
        cout << " * " << (value?"A":"Not a") << "ugmenting map " << mapname 
             << " with class [" << classname << "] with value " << value 
             << endl;

      if (value != 0) {
        // shouldn't do anything if class model and/or map already exists
        //        the_db->AddClassModel(classname,mapname);
        Query *q = the_db->ClassModel(classname);
        if (!q)
          return ShowError("DoClassModelAugmentation("+ToStr(ii)+",...) failed"
                           " : no model for classname=["+classname+"]");

        simple::FloatMatrix& qm = Convolved(ii, j, k);
	const simple::FloatMatrix& am = q->Convolved(mapname, j, k);
	if (debug_classify) {
	  float amin = am.Minimum(), amax = am.Maximum();
	  float qmin = qm.Minimum(), qmax = qm.Maximum();
	  cout << " *   original  values min=" << qmin << " max=" << qmax
	       << endl;
	  cout << " *   augmented values min=" << amin << " max=" << amax
	       << endl;
	}
        qm += am*value;
	if (debug_classify) {
	  float rmin = qm.Minimum(), rmax = qm.Maximum();
	  cout << " *   resulting values min=" << rmin << " max=" << rmax
	       << endl;
	}
      }        
    }
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  void Query::CreateAllClassModels() {
    for (classaugment_iter_t it=classaugment.begin(); 
         it != classaugment.end(); it++) {
      const string& classn = it->first.first;
      const string& featn  = it->first.second;
      if (classn.empty())
        continue;
      if (!featn.empty())
        GetDataBase()->AddClassModel(classn, featn);
      else 
        for (size_t m=0; m<Nindices(); m++) 
          GetDataBase()->AddClassModel(classn, IndexShortName(m));
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  
  float Query::GetAugmentValue(const string& classname, const string& mapname){
    pair<string,string> aug_pair(classname,mapname);

    // check with class,map
    classaugment_iter_t it = classaugment.find(aug_pair);
    float val1 = it==classaugment.end() ? 1.0 : it->second;

    // check with e.g. class,""
    aug_pair.second = "";
    it = classaugment.find(aug_pair);
    float val2 = it==classaugment.end() ? 1.0 : it->second;

    // check with "", ""
    aug_pair.first = "";
    it = classaugment.find(aug_pair);
    float val3 = it==classaugment.end() ? 1.0 : it->second;

    if (debug_classify)
      cout << "GetAugmentValue(" << classname << "," << mapname << ") = "
           << val1 << " * " 
           << val2 << " * " 
           << val3 << endl;
    
    // if nothing found return zero
    return val1*val2*val3;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ForUnitsPicSOM(const string& /*args*/,
                             int i, int j, bool botonly) {
#ifdef QUERY_USE_PTHREADS
    char txt[1000];
    sprintf(txt, "Query::ForUnitsPicSOM(%d, %d)", i, j);
    picsom->ConditionallyAnnounceThread(txt, query_main_thread, gettid());
    bool do_tictac = !picsom->PthreadsTssom();
#else
    bool do_tictac = true;
#endif // QUERY_USE_PTHREADS

    bool ret = true;

    if (do_tictac)
      Tic("ForUnitsPicSOM");

    int idx = botonly ? i           : ScoreValueIndex(i, j);
    int len = botonly ? Nfeatures() : NumberOfScoreValues();

    //    cout << "ForUnitsPicSOM() : i=" << i << " idx=" << idx << " len=" << len << endl;

    if (IsInverseIndex(i) && j==0) 
      PerMapNewObject(i, j) = SelectObjectsByBinaryFeatureScores(i, idx, len);

    else if (IsPreCalculatedIndex(i) && j==0)
      PerMapNewObject(i, j) = SelectObjectsByPreCalculatedScores(i, idx, len);
    
    else if (IsSvm(i) && j==0)
      PerMapNewObject(i, j) = SelectObjectsBySvmScores(i, idx, len);
    
    else {
      bool aliendata = TsSom(i).Type()==TSSOM::tssom_alien_data;

      const TSSOM::State& fd = IndexDataTSSOM(i);
      float weight = fd.tssom_level_weight[j];
      if (weight==Index::State::nan()) {
        WarnOnce("Query::ForUnitsPicSOM() : "
                 "feature weight == NaN detected for "+
                 TsSom(i).MapName(false, j));
        weight = 0.0;
      }

      PerMapNewObject(i, j)
	= SelectObjectsFromVQUnits(weight, PerMapVQUnit(i, j),
				   Map(i, j), //was EvenAlien
				   aliendata, idx, len);
    }

    if (do_tictac)
      Tac("ForUnitsPicSOM");
  
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PerMapVQ(int i) {
    bool ret = true;

    Tic("PerMapVQ");

    int lvl = VQlevel();

    if (!PlaceSeenOnMap(i, lvl)) {
      char tmp[100];
      sprintf(tmp, "(%d,%d)", i, lvl);
      ShowError("Query::PerMapVQ", tmp, ": PlaceSeenOnMap() failed");
      ret = false;
    }

    if (ret) {
      PerMapVQUnit(i, lvl) = BestVQUnitsVQ(i, lvl);
    }
    
    Tac("PerMapVQ");
  
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ForUnitsVQ(int i) {
    bool ret = true;

    int lvl = VQlevel();
  
    Tic("ForUnitsVQ");
    //  cout << "ForUnitsVQ: PerMapVQUnit(" << i << ", " << lvl <<") contains " 
    //       << PerMapVQUnit(i, lvl).Nitems() << " items " << endl;
    PerMapNewObject(i, lvl) = SelectObjectsFromVQUnitsVQ(PerMapVQUnit(i, lvl));
    Tac("ForUnitsVQ");
  
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CombineMapsVQ() {
    bool ret = true;
  
    Tic("CombineMapsVQ");
    ret = ret && GatherNewObjects();
    ret = ret && NewObjectValues(false);
    Tac("CombineMapsVQ");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CombineMapsVQW() {
    bool ret = true;
  
    Tic("CombineMapsVQW");
    ret = ret && GatherNewObjects();
    ret = ret && NewObjectValues(true);
    Tac("CombineMapsVQW");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::GatherNewObjects() {
    bool ret = true;

    int lvl = VQlevel();  
  
    for (size_t i=0; i<Nindices(); i++) {  
      ObjectList& images = PerMapNewObject(i, lvl);        
      cout << "GatherNewObjects(): PerMapNewObject(" << i << ", "
           << lvl << ") contains " 
           << PerMapNewObject(i, lvl).size() << " items " << endl;
      int oldl = NnewObjects();
      NewObject().append(images);
      images.clear();

      int newl = NnewObjects();
      Tic("InsertAndRemoveDuplicates");
      Object::InsertAndRemoveDuplicates(NewObject(), oldl);
      Tac("InsertAndRemoveDuplicates");
      cout << "GatherNewObjects(): removed " <<  newl-NnewObjects()
           << " duplicates" << endl;
    }

    WarnOnce("Query::GatherNewObjects(): "
             "USE OF uniqueimage SHOULD BE CORRECTED!");

    uniqueimage = NewObject();
  
    cout << "GatherNewObjects(): Total of " << NnewObjects() 
         << " new images" << endl;
  
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::NewObjectValues(bool weighted) {
    bool ret = true;
  
    Tic("SeenObjectIndices");    
    IntVector posidx = SeenObjectIndices(true);
    Tac("SeenObjectIndices");
  
    DoubleVector sumdist(NnewObjects());
    DoubleVector weights(Nindices());

    for (size_t i=0; i<Nindices(); i++) {
      Tic("DataByIndices");
      FloatVectorSet posdata = TsSom(i).DataByIndices(posidx);
      Tac("DataByIndices");
    
      if (!posdata.Nitems()) {
        ShowError("Query::NewObjectValues(): DataByIndices() failed.");
        SimpleAbort();
      }

      if (weighted) { // VQW algorithm
        for (int j=0; j<posdata.Nitems(); j++) {
          Tic("MeanSquaredDistance");
          weights[i] += posdata.MeanSquaredDistance(posdata[j]);
          Tac("MeanSquaredDistance");
        }      
        double md = TsSom(i).MeanDistance();
        if (md<=0.0)
          cerr << "Meandistance of " << IndexShortName(i) << " is invalid: "
               << md << " (2)" << endl;
        weights[i] = (md * posdata.Nitems()) / weights[i];      
        cout << "NewObjectValues(): weights[" << i << "]: " << weights[i]  <<endl;

      } else // VQ algorithm
        weights[i] = 1.0;
    
      Tic("DistancesToPositives");
      sumdist += DistancesToPositives(posdata, i) * weights[i];
      Tac("DistancesToPositives");
    }
  
    for (size_t j=0; j<NnewObjects(); j++) 
      newobject[j].Value(sumdist[j]);
  
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::FinalSelectVQ() {
    Obsoleted("Query::FinalSelectVQ()");

    bool ret = true;

    Tic("FinalSelectVQ");

    for (size_t i=1; i<NnewObjects(); i++)
      for (int j=i-1; j>=0; j--)
        if (newobject[j].Value()>newobject[j+1].Value()) 
          NewObject().swap_objects(j, j+1);

    while (NnewObjects()>MaxQuestions()) 
      newobject.erase(newobject.size()-1);

    for (size_t j=NnewObjects(); j<uniqueimage.size(); j++)
      uniqueimage[j].Discard();

    Tac("FinalSelectVQ");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoEntropyCalculation(int i, int j, int k) {
    if (!IsTsSom(i)) 
      return true;

    const float nan = Index::State::nan();
    const simple::FloatMatrix& mm = Convolved(i, j, k);

    bool check = k==1;
    float entropy = mm.EntropyWithCheck(check), e0 = nan, entropy_n = nan;
    float maxentr = MaximumEntropy(mm);
  
    if (maxentr==0.0) // zero-sized map, perhaps non-bottom level
      return true;

    TSSOM::State& fd = IndexDataTSSOM(i);
    fd.entropy[j]   = entropy;
    fd.entropy_r[j] = entropy/maxentr;

    const FloatVector *cvec = ConvolutionVector(ConvType(i), j);
    if (cvec) {
      e0 = ConvolutionEntropy(cvec);
      if (e0>=0.0) {
        fd.entropy_p[j] = entropy-e0;

        // obs! NpositiveSeenImages() may not return what we expect...
        int npos = NpositiveSeenObjects();
        if (npos>1) {
          float e_div = (e0>0 && npos*e0<maxentr ? npos*e0 : maxentr) - e0;
          if (e_div)
            entropy_n = (entropy-e0)/e_div;
        } else
          entropy_n = 0.0;

        fd.entropy_n[j] = entropy>e0 ? entropy_n : 0.0;
      }
    }

    if (debug_weights)
      cout << "entropies " << MapName(false, i, j, k)
           << " H=" << entropy << " Hmax=" << maxentr
           << " Hr=" << fd.entropy_r[j] << " e0=" << e0
           << " Hp=" << fd.entropy_p[j] << " Hnraw=" << entropy_n
           << " Hn=" << fd.entropy_n[j] << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ValueOrNaN(float v, bool all) {
    const float nan = Index::State::nan();
    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
        TSSOM::State& fd = IndexDataTSSOM(i);
        for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
          fd.tssom_level_weight[j] =
            all||j==fd.tssom_level_weight.size()-1 ? v : nan;
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::TsSomWeightValue(bool all) {
    const float nan = Index::State::nan();
    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
        float v = Weight(i);
        TSSOM::State& fd = IndexDataTSSOM(i);
        for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
          fd.tssom_level_weight[j] =
            all||j==fd.tssom_level_weight.size()-1 ? v : nan;
      }

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::TsSomWeightValueAndDistances(bool all) {
    const float nan = Index::State::nan();
    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
        float v = Weight(i);
        TSSOM::State& fd = IndexDataTSSOM(i);

        const TreeSOM& ts = MapEvenAlien(i, fd.tssom_level_weight.size()-1);

        double meandist = ts.MeanDistance();
        // double rmsdist = ts.RmsDistance();
        
        double dist = 0.0;
        for (size_t i=0; i<NseenObjects(); i++) {
          const Object& obj = seenobject[i];
          dist += ts.TrainDist(obj.Index());
        }
        dist /= NseenObjects();
        
//        double dist_weight = dist > meandist ? meandist/dist : 1.0;
        double dist_weight = meandist/(meandist+dist);
        
//        cout << endl << "USING WEIGHT: " << dist_weight << endl << endl;

        for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
          fd.tssom_level_weight[j] =
            all||j==fd.tssom_level_weight.size()-1 ? v*dist_weight : nan;
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::EntropyNpolynomial(const string& arg) {
    const string defa = "0.33333333,1.0";
    const string& a = arg!="" ? arg : defa;

    vector<string> val = SplitInCommas(a);
    if (val.size()!=2)
      return ShowError("Query::EntropyNpolynomial(", a, ") failed");
  
    double min = atof(val[0].c_str()), ex = atof(val[1].c_str());
    const float nan = Index::State::nan();

    if (debug_weights)
      cout << "EntropyNpolynomial(" << a << ") -> " << min << " " << ex << endl;

    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
        TSSOM::State& fd = IndexDataTSSOM(i);
        for (size_t j=0; j<fd.tssom_level_weight.size(); j++) {
          fd.tssom_level_weight[j] = fd.entropy_n[j]==nan ? nan :
            pow(1+(min-1)*fd.entropy_n[j], ex);
          if (debug_weights)
            cout << "  " << MapName(false, i, j) << " : " << ToStr(fd.entropy_n[j])
                 << " -> " << ToStr(fd.tssom_level_weight[j]) << endl;
        }
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::EntropyNleaveOutWorst(const string&) {
    typedef multimap<float,int> order_t;
    order_t order;

    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
        TSSOM::State& fd = IndexDataTSSOM(i);
        if (fd.entropy_n.size())
          order.insert(pair<float,int>(fd.entropy_n.back(), i));
      }

    int worst = -1;

    if (order.size()>1) {
      while (order.size()>2)
        order.erase(order.begin());
      if (order.begin()->first!=order.rbegin()->first)
        worst = order.rbegin()->second;
    }

    const float nan = Index::State::nan();
    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
        TSSOM::State& fd = IndexDataTSSOM(i);
        for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
          if (fd.entropy_n[j]==nan)
            fd.tssom_level_weight[j] = nan;
          else
            fd.tssom_level_weight[j] = int(i)==worst ? 0.0 : 1.0;
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  IntVector Query::SeenObjectIndices(bool only_positives) const {
    Tic("SeenImageIndices");
    IntVector res(NseenObjects());

    int n = 0;
    for (size_t j=0; j<NseenObjects(); j++) {
      const Object& img = seenobject[j];
      int idx = img.Index();
      if (idx < 0)
        ShowError("Query::SeenImageIndices(): Index is -1"); 

      if (only_positives && img.Value()<=0)
        continue;

      res[n++] = idx;
    }
    res.Lengthen(n);

    Tac("SeenImageIndices");

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  cbir_stage Query::OfflineShortcut(){
    // bypasses parts of the stage chain and acts 
    // like stage_select_images_to_process and
    // populates the newobject list with all the database images

    DeleteNew();
    random_image_count = 0;

    DataBase *db = GetDataBase();

    for (size_t i=0; i<DataBaseSize(); i++){

      // target types hard coded

      if(db->ObjectsTargetType(i)!=target_image &&
         db->ObjectsTargetType(i)!=target_imagefile) continue;

      Object img(db, i, select_question);
      AppendNewObject(img);
    }

    return NnewObjects() ? stage_process_images : stage_error;
   }

  /////////////////////////////////////////////////////////////////////////////

  FloatVector Query::MapUnitScore(int m, int l) const {

    const SOM& som = Map(m, l);
    const int s = som.Units();
    IntVector N(s), seen(s), pos(s), neg(s), rem(s);  

    int len = som.Width()*som.Height();
    FloatVector res;
  
    if (len > N.Length() || len > seen.Length() || len > pos.Length() || 
        len > neg.Length() || len > rem.Length()) 
      return res;

    Tic("MapUnitScore");

    seen.Zero();
    pos.Zero();
    neg.Zero();

    IntVector seenidx = SeenObjectIndices(false);  
    const IntVector& div = Division(m, l);

    for (int j=0; j<seenidx.Length(); j++) {
      int idx = div[seenidx[j]];
      seen[idx]++;
      register double val = seenobject[j].Value();
      if (val<-0.5) 
        neg[idx]++;
      else if (val>0.5)
        pos[idx]++;
    }

    res.Length(len);

    //   for (IntPoint p=som; p; ++p) {
    //     int idx = som.ToIndex(p);
    //     const IntVector& br = BackReference(m, l, idx);
    //     N[idx] = br.Length();
    //     rem[idx] = N[idx] - seen[idx];
    //     res[idx] = seen[idx] ? (float)pos[idx]/(float)seen[idx] : 0.0;
  
    //     if (rem[idx] == 0)
    //       res[idx] = 0.0;
    //   }
  
    for (IntPoint p=som; p; ++p) {
      int idx = som.ToIndex(p);
      const IntVector& br = BackReference(m, l, idx);
      N[idx] = br.Length();
      rem[idx] = N[idx] - seen[idx];

      // scoring:
      if (rem[idx] == 0) 
        res[idx] = 0.0;

      else if (pos[idx] > 0)
        res[idx] = (float)pos[idx]/(float)seen[idx] + 1.0; // 1 < res <= 2 
    
      else if (seen[idx] == 0) 
        res[idx] = 1.0;
    
      else 
        res[idx] = 0.5*rem[idx]/N[idx]; //  0 < res < 0.5

      // SelectImagesFromVQUnitsVQ() uses: 0.5 < res < 1
    }

    Tac("MapUnitScore");

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::PerFeatureSQ(int /*fea*/) {
    return ShowError("Query::PerFeatureSQ() not working NEW_OBJECT_INDICES");

    /*
    bool ret = true;

    Tic("PerFeatureSQ");

    FloatVectorSet &cfr = Data(fea);
    ScalarQuantize(fea);

    ValuedList hitcount;
  
    IntVector pos(DataBaseSize());
    pos.Set(-1);

    double point = 1.0/cfr.VectorLength();

    Tic("PerFeatureSQ loop");
    for (size_t i=0; i<NseenImages(); i++)
      if (seenimage[i].Value()>0) {
        IntVector sq = cfr.ScalarQuantized(seenimage[i].Index());
        for (int jv=0; jv<cfr.VectorLength(); jv++) {
          IntVector eqs = cfr.ScalarQuantBackRefs(jv, sq[jv]);
          for (int jl=0; jl<eqs.Length(); jl++) {
            int idx = eqs[jl];
            if (!IsSeen(idx)) {  // CanBeShownRestricted() ???
              int ipos = pos[idx];
              if (ipos<0) {
                pos[idx] = ipos = hitcount.Nitems();
                // obs! this doesn't yet handle aspects via AspectsFromIndices();
                hitcount.Append(new Object(the_db, idx, select_question, 0));
              }
              hitcount[ipos].AddValue(point);  // OBS! AddValue()
            }
          }
        }
      }
    Tac("PerFeatureSQ loop");

    Tic("PerFeatureSQ sort");
    //Object::SortList(hitcount);
    int pmni = _sa_permapobjects;
    ValuedList sorted;
    for (int s=0; s<hitcount.Nitems(); s++) {
      hitcount.Relinquish(s);
      Valued::AddToSorted(sorted, pmni, hitcount.Get(s));
    }
    ObjectList &ilist = PerMapNewObject(fea, 0);
    ilist.Delete();
    Object::MakeObjectList(ilist, sorted);
  
    Tac("PerFeatureSQ sort");

    Tac("PerFeatureSQ");
  
    return ret;
    */
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ScalarQuantize(bool force) {
    bool ret = true;
    for (size_t i=0; i<Nindices(); i++)
      ret = ret && ScalarQuantize(i, force);
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ScalarQuantize(int i, bool force) {
    FloatVectorSet &cfr = Data(i);

    if (cfr.HasScalarQuantization() && !force)
      return true;

    bool ret = true;

    Tic("ScalarQuantize");

    const char *sqbins = NULL; // GetKeyValue("sqbins");
    int nbins = sqbins ? atoi(sqbins) : 4;
  
    ListOf<FloatVector> eq;
    char addtxt[1000] = "";

    if (TsSom(i).OrdFileName()=="" || !TsSom(i).Ord().Nitems())
      eq = cfr.CreateEqualizedScalarQuantization(nbins);
    else {
      eq = cfr.CreateEqualizedScalarQuantization(nbins, TsSom(i).Ord());
      sprintf(addtxt, " from %s", TsSom(i).OrdFileName().c_str());
    }
  
    WriteLog("Created equalized scalar quantization in ", ToStr(nbins),
             " bins of ", IndexFullName(i), addtxt);
    // eq.Dump(DumpRecursiveLong);
    cfr.ScalarQuantize(eq);
    WriteLog("Scalar quantized ", IndexFullName(i));
    cfr.MakeScalarQuantBackRefs();
    WriteLog("Made scalar quantization back references of ", IndexFullName(i));
    Tac("ScalarQuantize");
  
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SimpleBinaryFeatureScores(int i) {
    bool ok = true;
    bool scaled = wordInverse(i).UseBinaryScaling();

    if (!wordInverse(i).IsLatentBinaryFeature())
      ok = SimpleBinaryFeatureScoresNonLatent(i);
    
    else if (!wordInverse(i).UseBinaryClassIndices())
      ok = SimpleBinaryFeatureScoresViaBinaryClassNew(i, scaled); // mats!:Old

    else
      ok = SimpleBinaryFeatureScoresViaBinaryFeature(i, scaled);

    if (!ok || !WordInverse::DebugBinary())
      return ok;

    WordInverse::State& data = IndexDataWordInverse(i);
    vector<float>& f = data.bin_feat;

    for (size_t k=0; k<f.size(); k++)
      cout << "BINFEAT Query::SimpleBinaryFeatureScores(" << i << ") ["
           << IndexFullName(i) << "] k=" << k << " : <"
           << BinaryFeatureData(i).ClassName(k) << "> -> " << f[k] << endl;
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SimpleBinaryFeatureScoresViaBinaryClassNew(int i, bool scaled) {
    string msg = "Query::SimpleBinaryFeatureScoresViaBinaryClass() : ";

    if (!IsInverseIndex(i))
      return ShowError(msg+"not a binary feature");

    double weight = Weight(i);

    if (WordInverse::DebugBinary())
      WriteLog(msg+"scaling=", scaled ? "on" : "off",
               ", weight=", ToStr(weight));

    WordInverse::State& data = IndexDataWordInverse(i);
    vector<float>& f = data.bin_feat;
    const WordInverse::BinaryFeaturesData& binfdat = BinaryFeatureData(i);

    f = vector<float>(binfdat.ClassCountXXX(), 0.0);
    // obs! this is wrong

    // if ((int)f.size()!=tssom.BinaryClassCount()) {
    //   ShowError("Query::SimpleBinaryFeatureScoresViaBinaryClass()",
    //                 " : vector length problem");
    //  TrapMeHere();
    // }

    // obs! NpositiveSeenImages() may not return what we expect...
    // int np = NpositiveSeenImages(), nn = NnegativeSeenImages();

    // obs! HAS_CBIRALGORITHM_NEW problem
    if (debug_placeseen)
      cout << msg << MapName(false, i)
           << " aspects=[" << IndexAspects(NULL, i) << "]" << endl;

    target_type tt = wordInverse(i).FeatureTarget();
    size_t np = NSeenObjects(+1, tt), nn = NSeenObjects(-1, tt);

    // const IntVector& div = Division_y(i, 0); //this should be implemented...
    IntVector dummy(DataBaseSize());
    for (size_t j=0; j<DataBaseSize(); j++)
      dummy[j] = the_db->ObjectsTargetTypeContains(j,tt) ? 1 : -1;

    double positives = 0, negatives = 0;
    if (!SumPositivesAndNegatives(dummy, data, positives, negatives, true))
      return ShowError(msg+"SumPositivesAndNegatives() failed"); 

    if (WordInverse::DebugBinary())
      cout << msg+"np="+ToStr(np)+" nn="+ToStr(nn)+" positives="+
        ToStr(positives)+" negatives="+ToStr(negatives) << endl;

    for (size_t j=0; j<NseenObjects(); j++) {
      const Object& obj = seenobject[j];
      int idx    = obj.Index();
      int mapidx = dummy.IndexOK(idx) ? dummy[idx] : -1;

      if (debug_placeseen)
        cout << "  j=" << j << " " << the_db->ObjectDump(idx)
             << " aspects=[" << obj.AspectNamesValues() << "] mapidx="
             << mapidx;

      if (mapidx==-1) {
        if (debug_placeseen) cout << endl;
        continue;
      }
      
      double pos = 0.0, neg = 0.0;
      bool match  = AspectsMatch(data, obj, pos, neg);
      double objval = obj.Value();
      double sumval = pos+neg;
      
      if (debug_placeseen)
        cout << "match=" << match << " objval=" << objval
             << " pos=" << pos << " neg=" << neg
             << " sumval=" << sumval << " mapidx=" << mapidx
             << endl;

      if (!match)
        continue;

      // double v = SeenImage(j).Value();
      // v /= v>0 ? np : nn;
      // v *= weight;

      double pval = positives ? pos/positives : 0.0;
      double nval = negatives ? neg/negatives : 0.0;
      double sval = pval+nval;

      float mul = 1.0;

      if (scaled) {
        int classcount = 0;
        for (size_t k=0; k<f.size(); k++)
          if (binfdat.GroundTruth(k)[idx]==1)
            classcount++;
        if (classcount)
          mul = 1.0/classcount;
      }

      double val = sval*mul*weight;

      if (debug_placeseen)
        cout << "    pval=" << pval << " nval=" << nval << " sval=" << sval
             << " mul=" << mul << " val=" << val << endl;

      for (size_t k=0; k<f.size(); k++)
        if (binfdat.GroundTruth(k)[idx]==1) {
          if (debug_placeseen)
            cout << "      k=" << k << " [" << binfdat.GroundTruth(k).label()
                 << "] f[k]=" << f[k] << " -> ";
          f[k] += val;
          if (debug_placeseen)
            cout << f[k] << endl;
        }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SimpleBinaryFeatureScoresViaBinaryClassOld(int i, bool scaled) {
    ///
    /// This function was used for the Trecvid2006 experiments...
    ///

    if (!IsInverseIndex(i)) {
      ShowError("Query::SimpleBinaryFeatureScoresViaBinaryClassOld()",
                " : not a binary feature");
      TrapMeHere();
    }

    double weight = Weight(i);

    if (WordInverse::DebugBinary())
      WriteLog("Using SimpleBinaryFeatureScoresViaBinaryClassOld() : scaling=", 
               scaled ? "on" : "off", ", weight=", ToStr(weight));

    WordInverse::State& data = IndexDataWordInverse(i);
    vector<float>& f = data.bin_feat;
    f = vector<float>(BinaryFeatureData(i).ClassCountXXX(), 0.0);
    // obs! this is wrong

    // if ((int)f.size()!=tssom.BinaryClassCount()) {
    //   ShowError("Query::SimpleBinaryFeatureScoresViaBinaryClassOld()",
    //                 " : vector length problem");
    //  TrapMeHere();
    // }

    int np = NpositiveSeenObjects(), nn = NnegativeSeenObjects();

    for (size_t j=0; j<NseenObjects(); j++) {
      int  idx = SeenObject(j).Index();
      double v = SeenObject(j).Value();
      v /= v>0 ? np : nn;
      v *= weight;

      float mul = 1.0;

      if (scaled){
        int classcount = 0;
        for (size_t k=0; k<f.size(); k++) {
          int gt = BinaryFeatureData(i).GroundTruth(k)[idx];
          if (gt==1)
            classcount++;
        }
        mul = 1.0/classcount;
      }

      for (size_t k=0; k<f.size(); k++) {
        int gt = BinaryFeatureData(i).GroundTruth(k)[idx];
        if (gt==1)
          f[k] += v*mul;
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SimpleBinaryFeatureScoresViaBinaryFeature(int i, bool scaled) {
    if (!IsInverseIndex(i)) {
      ShowError("Query::SimpleBinaryFeatureScoresViaBinaryFeature()",
                " : not a binary feature");
      TrapMeHere();
    }

    double weight = Weight(i);

    if (WordInverse::DebugBinary())
      WriteLog("Using SimpleBinaryFeatureScoresViaBinaryFeature() : scaling=", 
               scaled ? "on" : "off", ", weight=", ToStr(weight));

    vector<float>& f = IndexDataWordInverse(i).bin_feat;
    f = vector<float>(BinaryFeatureData(i).ClassCountXXX(), 0.0);
    // obs! this is wrong

    // if ((int)f.size()!=tssom.BinaryClassCount()) {
    //   ShowError("Query::SimpleBinaryFeatureScoresViaBinaryFeature()",
    //                 " : vector length problem");
    //   TrapMeHere();
    // }

    // obs! NpositiveSeenImages() may not return what we expect...
    // int np = NpositiveSeenImages(), nn = NnegativeSeenImages();

    target_type tt = wordInverse(i).FeatureTarget();
    size_t np = NSeenObjects(+1, tt), nn = NSeenObjects(-1, tt);

    if (UsingAspects())
      WarnOnce("I think SimpleBinaryFeatureScoresViaBinaryFeature() "
               " doesn't work with aspects -- Jorma");

    for (size_t j=0; j<NseenObjects(); j++) {
      size_t idx = SeenObject(j).Index();
      double v = SeenObject(j).Value();
      v /= v>0 ? np : nn;
      v *= weight; 

      const vector<int>& featurelist = BinaryFeatureData(i).ClassIndices(idx);
      float mul = scaled ? 1.0/featurelist.size() : 1;

      for (size_t k=0; k<featurelist.size(); k++)
        f[featurelist[k]] += v*mul;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SimpleBinaryFeatureScoresNonLatent(int ii) {
    if (!IsInverseIndex(ii)) {
      ShowError("Query::SimpleBinaryFeatureScoreNonLatent()",
                " : not a binary feature");
      TrapMeHere();
    }

    double weight = Weight(ii);

    if (WordInverse::DebugBinary())
      WriteLog("Using SimpleBinaryFeatureScoresNonLatent() : scaling=", 
               "n/a, weight=", ToStr(weight));

    vector<float>& f = IndexDataWordInverse(ii).bin_feat;
    f = vector<float>(BinaryFeatureData(ii).ClassCountXXX(), 1.0);
    // obs! this is wrong

    for (int i=0; i<BinaryFeatureData(ii).ClassCountXXX(); i++) {
      int gtsize = BinaryFeatureData(ii).GroundTruth(i).NumberOfEqual(1);
      //cout << " #" << i << ": " << gtsize << endl;
      f[i] = 1.0*weight/gtsize;
    }

    // f = vector<float>(tssom.binary_data.ClassCount(), 1.0*weight);

    // should the f[k] be scaled down by the size of the class ?

    return true;
  }


  /////////////////////////////////////////////////////////////////////////////

  ObjectList Query::SelectObjectsByBinaryFeatureScores(int i, int midx, 
                                                       int nmat) {
    if (!IsInverseIndex(i)) {
      ShowError("Query::SelectObjectsByBinaryFeatureScores() : ",
                "not a binary feature");
      TrapMeHere();
    }

    // int debug = 0;

    ObjectList imageret;

    int pmni = _sa_permapobjects;
    bool seenparents = true;
    DataBase *db = GetDataBase();
    select_type q = select_question;
  
    FloatVector stvec(nmat+1);
    stvec.Set(MAXFLOAT);

    for (size_t l=0; l<DataBaseSize(); l++)
      if (CanBeShownRestricted(l, seenparents)) {
        float score = BinaryFeatureScoreForObject(i, l);
        if (score!=0.0 && WordInverse::DebugBinary())
          cout << "BINFEAT [" << IndexFullName(i) << "] : [" << Label(l)
               << "] score=" << score << endl;
        stvec.Last() = stvec[midx] = score;
        Object candidate(db, l, q, score, "", 0, &stvec);
        imageret.add_and_sort((size_t)pmni, candidate);
      }

     if (WordInverse::DebugBinary())
       cout << "Query::SelectObjectsByBinaryFeatureScores() : imageret has " 
            << imageret.size() << " objects" << endl;
    return imageret;
  }

  /////////////////////////////////////////////////////////////////////////////

  double Query::BinaryFeatureScoreForObject(int map, int idx) const {
    const WordInverse::State& data = IndexDataWordInverse(map);
    const WordInverse& wdinv = wordInverse(map);
    const vector<float>& f = data.bin_feat;
    if ((int)f.size()!=BinaryFeatureData(map).ClassCountXXX()) {
      ShowError("Query::BinaryFeatureScoreForObject() : vector problem");
      TrapMeHere();
    }

    double score = 0.0;
      
    if (!wdinv.UseBinaryClassIndices()) { // using BinaryClass();
      for (size_t k=0; k<f.size(); k++)
        if (BinaryFeatureData(map).GroundTruth(k)[idx]==1) {
          score += f[k];
          
          if (WordInverse::DebugBinary())
            cout << "BINFEAT bc [" << IndexFullName(map) << "] : ["
                 << Label(idx) << "] k=" << k << " <"
                 << BinaryFeatureData(map).ClassName(k)
                 << "> score+=" << f[k] << endl;
        }

    } else { // using BinaryFeature():
      const vector<int>& featurelist = BinaryFeatureData(map).ClassIndices(idx);
      for (size_t k=0; k<featurelist.size(); k++) {
        score += f[featurelist[k]];

        if (WordInverse::DebugBinary())
          cout << "BINFEAT bf [" << IndexFullName(map) << "] : ["
               << Label(idx) << "] k=" << k << " <"
               << BinaryFeatureData(map).ClassName(featurelist[k])
               << "> score+=" << f[featurelist[k]] << endl;
      }
    }
      
    return score;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SimplePreCalculatedScores(int i) {
    if (!IsPreCalculatedIndex(i)) {
      ShowError("Query::SimplePreCalculatedScores()",
                " : not a precalculated feature");
      TrapMeHere();
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SimpleSvmScores(int i) {
    if (!IsSvm(i)) {
      ShowError("Query::SimpleSvmScores()",
                " : not a precalculated feature");
      TrapMeHere();
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  ObjectList Query::SelectObjectsByPreCalculatedScores(int i, int midx, 
                                                       int nmat) {
    if (!IsPreCalculatedIndex(i)) {
      ShowError("Query::SelectObjectsByPreCalculatedScores() : ",
                "not a precalculated feature");
      TrapMeHere();
    }

    //    int debug = 1;

    ObjectList imageret;

    int pmni = _sa_permapobjects;
    bool seenparents = true;
    DataBase *db = GetDataBase();
    select_type q = select_question;
     
    FloatVector stvec(nmat+1);
    stvec.Set(MAXFLOAT);

    for (size_t l=0; l<DataBaseSize(); l++)
      if (CanBeShownRestricted(l, seenparents)) {
        float score = PreCalculatedScoreForObject(i, l);
        if (score!=0.0 && PreCalculatedIndex::DebugPreCalculated())
          cout << "PRECALC sel [" << IndexFullName(i) << "] : [" << Label(l)
               << "] score=" << score << endl;
        stvec.Last() = stvec[midx] = score;
        Object candidate(db, l, q, score, "", 0, &stvec);
        imageret.add_and_sort((size_t)pmni, candidate);
      }

    if (PreCalculatedIndex::DebugPreCalculated())
      cout << "Query::SelectObjectsByPreCalculatedScores() : imageret has " 
           << imageret.size() << " objects" << endl;
    return imageret;
  }

  /////////////////////////////////////////////////////////////////////////////

  double Query::PreCalculatedScoreForObject(int m, int idx) const {
    //    const PreCalculatedIndex::State& data = IndexDataPreCalculated(map);
    const PreCalculatedIndex& pci = preCalculated(m);    
    const map<string,double> &pcm = pci.precalc;
    
    double score = 0.0;

    //    cout << "XXX " << idx << " : " << Label(idx) << endl;

    map<string,double>::const_iterator p = pcm.find(Label(idx));
    if (p!=pcm.end()) {
      score = prescore_function(p->second)*Weight(m);
      if (PreCalculatedIndex::DebugPreCalculated())
         cout << "PRECALC sco [" << IndexFullName(m) << "] : ["
              << Label(idx) << "] idx=" << idx 
              << " score=" << score << endl;
    }
      
    return score;
  }

  /////////////////////////////////////////////////////////////////////////////

  ObjectList Query::SelectObjectsBySvmScores(int i, int midx, int nmat) {
    if (!IsSvm(i)) {
      ShowError("Query::SelectObjectsBySvmScores() : ",
                "not a precalculated feature");
      TrapMeHere();
    }

    DataBase *db = GetDataBase();
    size_t idx = 0;
    select_type q = select_question;
    float score = 0.5;
    FloatVector stvec(nmat+1);
    stvec.Set(MAXFLOAT);
    stvec.Last() = stvec[midx] = score;
        
    Object candidate(db, idx, q, score, "", 0, &stvec);

    ObjectList imageret;
    imageret.add_and_sort(0, candidate);

    return imageret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExecuteRunPartThree() {
#ifndef QUERY_USE_PTHREADS
    return RunPartThree();
#else
    if (!Picsom()->PthreadsConnection())
      return RunPartThree();

    // if this comes too late in RunPartThree(), some other thread may
    // start joining this in RemoveFinishedQueries()...
    inrun = true;

    void *(*xxx)(void*) = ExecuteRunPartThreeThread;
    pthread_t p;
    if (pthread_create(&p, NULL, (CFP_pthread) xxx, (void*)this))
      return ShowError("Query::ExecuteRunPartThree() :"
                       " pthread_create() failed");

    Picsom()->AppendRunningQuery(p, this);

    return true;
#endif // QUERY_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef QUERY_USE_PTHREADS
  void *Query::ExecuteRunPartThreeThread(void *p) {
    return ((Query*)p)->ExecuteRunPartThreeThread();
  }

  /////////////////////////////////////////////////////////////////////////////

  void *Query::ExecuteRunPartThreeThread() {
    Picsom()->ConditionallyAnnounceThread("ExecuteRunPartThree()");

    static bool t = true, f = false;

    bool ok = RunPartThree();

    return ok ? &t : &f;
  }
#endif // QUERY_USE_PTHREADS

  /////////////////////////////////////////////////////////////////////////////

  bool Query::RunPartThree() {
    SingleWriteMultiReadLock(true);

    inrun = true;
    SetLastModificationTimeNow();
    ReadFiles();

    okstatus = SinglePass();
    if (!okstatus)
      ShowError("Query::RunPartThree() : SinglePass() failed");

    inrun = false;

    SetLastModificationTimeNow();
    picsom->PossiblyShowDebugInformation("RunPartThree() ending");
    SingleWriteMultiReadUnlock();

    return okstatus;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::SingleWriteMultiReadLock(bool w) {
#ifdef QUERY_USE_PTHREADS
    PthreadLock(w);
#else
    w = !w;
#endif // QUERY_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::SingleWriteMultiReadUnlock() {
#ifdef QUERY_USE_PTHREADS
    PthreadUnlock();
#endif // QUERY_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string> > Query::SplitArgs(const string &arg){

    string strleft=arg;

    list<pair<string,string> > ret;
    pair<string,string> tmppair;

    while(strleft!=""){
      string pairStr;
      size_t commapos=strleft.find(',');
      if(commapos==string::npos){ //last argument
        pairStr=strleft;
        strleft="";
      }
      else{
        pairStr=strleft.substr(0,commapos);
        strleft=strleft.substr(commapos+1);
      }
      
      size_t eqpos=pairStr.find('=');
      if(eqpos==string::npos){
        tmppair.first=pairStr;
        tmppair.second="";
      } else{
        tmppair.first=pairStr.substr(0,eqpos);
        tmppair.second=pairStr.substr(eqpos+1);
      }
      ret.push_back(tmppair);
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoClassification() {
    bool analysismode = ( 2 == ClassifyInt()), ok = true;

    classification_data.resize(0);
    if (!NpositiveSeenObjects() || classificationset=="")
      return true;

    string msg = "Query::DoClassification() : ";

    if (DefaultMatrixCount()!=3)
      return ShowError(msg+"matrixcount!=3");

    if (classifymethod=="")
      return ShowError(msg+"classifymethod not set");

    if (classifymethod == "list" || classifymethod == "help") {
      cout << msg+"Supported classification methods are: " << endl 
           << msg+"dotproduct, absdiff, intersect, euclidean, and jeffreydiv"
           << endl
           << msg+"a priori weighting is switched on "
        "using the \"+apriori\" suffix" << endl;
      exit(0);
    }

    if (classifynclasses < 1) {
      classifynclasses = MAXINT;
      if (debug_classify)
        cout << msg+"classifynclasses was not positive " 
             << "integer, now set to MAXINT." << endl;
    }
  
    DataBase *db = GetDataBase();

    if (debug_classify)
      cout << endl
           << msg+"classificationset = " << classificationset
           << " NclassModels() = " << NclassModels() << "/"
           << db->NclassModelsAll() << ", nclasses = " << classifynclasses
           << ", classifymethod = " << classifymethod << endl;

    string classmethd = classifymethod;
    bool useapriori = false; 
    size_t apr = classmethd.find("+apriori");
    if (apr != string::npos) {
      useapriori = true;
      classmethd.erase(apr);
      if (debug_classify)
        cout << msg+"using a priori weighting, classifymethod now = "
             << classmethd << endl;
    }

    vector<string> feats;
    for (size_t i=0; i<Nindices(); i++)
      feats.push_back(IndexShortName(i)); 

    bool do_knn = classmethd.find("knn")==0;
    bool do_svm = classmethd.find("svm")==0;
      
    RemoveClassModels();
    list<string> cls = db->SplitClassNames(classificationset);
    for (list<string>::const_iterator i=cls.begin(); i!=cls.end(); i++) {
      if (true) { // obs! should test modification time?
        db->DeleteClassModel(*i);
      }
       
      AddClassModelName(*i);
      if (!db->ClassModel(*i)) {
        list<string> one;
        one.push_back(*i);
        db->PrepareClassification(one, false, feats, do_knn, do_svm);
      }
    }

    for (size_t ci=0; ci<NclassModels(); ci++) {
      Query *q = db->ClassModel(ClassModelName(ci));
      if (!q) {
        ok = ShowError(msg+"class model of \""+
                       ClassModelName(ci)+"\" not found");
        continue;
      }
          
      // obs! NpositiveSeenImages() may not return what we expect...
      double apriori = (double)q->NpositiveSeenObjects() / q->DataBaseSize();

      if (debug_classify)
        cout << endl << q->Identity() << " : " << q->Nindices() << endl;

      if (classmethd=="dotproduct" || classmethd=="absdiff" ||
          classmethd=="intersect"  || classmethd=="euclidean" ||
          classmethd=="jeffreydiv" || classmethd=="0") {
        DoClassificationTsSom(q, classmethd, useapriori?apriori:1.0);
      }
    }

    if (do_knn) {
      int k = atoi(classmethd.substr(3).c_str());
      DoClassificationKnn(k);
    }

    if (do_svm)
      DoClassificationSvm();

    const char *method[] = {
      "dotproduct",
      "absdiff",
      "intersect",
      "euclidean",
      "jeffreydiv",
      "knn",
      "svm",
      NULL
    };

    for (size_t m=0; method[m]; m++) {
      string mstr = method[m];
      if (classresult.find(mstr)==classresult.end())
        continue;

      bool use = classmethd==mstr ||
	(classmethd.find("knn")==0 && mstr=="knn");

      typedef multimap<double,string> res_t;
      res_t& res = classresult[mstr];
      float  mul = mstr=="dotproduct" || mstr=="intersect" || mstr=="knn"
        ? -1.0 : 1.0;

      if (debug_classify || analysismode)
        cout << endl << endl << (use?"* ": "  ") << mstr+": ";

      int n = 0;
      for (res_t::const_iterator i=res.begin();
           i!=res.end() && n<classifynclasses; i++, n++) {
        double val = mul*i->first;
        if (debug_classify || analysismode)
          cout << n << ": " << i->second << "(" << val << ") ";

        if (use) {
	  classification_data_t cdt(i->second, val);
	  res_t& resinfo = classresult["knninfo"];
	  cdt.extrafloat = resinfo.begin()->first;
	  cdt.extrastring = resinfo.begin()->second;
          //cout << cdt.extrafloat << " : " << cdt.extrastring << endl;
          classification_data.push_back(cdt);
	}
      }
    }
    if (debug_classify || analysismode)
      cout << endl << endl;

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoClassificationTsSom(Query *q, const string& /*classmethd*/,
                                    double apriori) {
    DataBase *db = GetDataBase();
    int level = -1;
    bool debug_display = false;

    double dotpsum = 0.0, diffsum = 0.0, isecsum = 0.0;
    double euclsum = 0.0, jdivsum = 0.0;

    typedef multimap<double,string> res_t;
    res_t& dotpres = classresult["dotproduct"];
    res_t& diffres = classresult["absdiff"];
    res_t& isecres = classresult["intersect"];
    res_t& euclres = classresult["euclidean"];
    res_t& jdivres = classresult["jeffreydiv"];

    for (size_t ii=0; ii<Nindices(); ii++) {
      const string& fn = IndexShortName(ii); //  or IndexFullName()
      int kk = q->IndexShortNameIndex(fn);   // and IndexFullNameIndex()
      if (kk<0) {
        if (debug_classify)
          cout << "   " << fn << " not created yet ..." << endl;

        db->AddClassModel(q->Identity(), fn);
        kk = q->IndexShortNameIndex(fn);    // and IndexFullNameIndex()
        if (kk<0) {
          if (debug_classify)
            cout << "   " << fn << " FAILED to create !!!" << endl;
          continue;
        }
      }

      int jj = level>=0 ? level : TsSom(ii).Nlevels()-1;

      const simple::FloatMatrix& m1 =    Convolved(ii, jj, 1);
      const simple::FloatMatrix& m2 = q->Convolved(kk, jj, 0);

      if (debug_display) {
        imagedata m1img = CreateMapImage(m1, NULL);
        imagefile::display(m1img);
        imagedata m2img = CreateMapImage(m2, NULL);
        imagefile::display(m2img);
      }

      double dotp = m1.DotProduct(m2)*apriori;
      double diff = m1.AbsDiffSum(m2)*apriori;

      double isec = 0.0, eucl = 0.0, jdiv = 0.0;
      for (int y=0; y<m1.Rows(); y++)
        for (int x=0; x<m1.Columns(); x++) {
          double m1val = m1.Get(y, x), m2val = m2.Get(y, x);
          isec += ( m1val > m2val ? m2val : m1val );
          eucl += (m1val-m2val)*(m1val-m2val);
          double avgval = (m1val+m2val)/2.0;
          if (m1val > 0.000000001) 
            jdiv += m1val*::log(m1val/avgval);
          if (m2val > 0.000000001) 
            jdiv += m2val*::log(m2val/avgval);
        }

      isec *= apriori/m1.Sum();
      jdiv *= apriori;
      eucl  = sqrt(eucl)*apriori;

      if (debug_classify)
        cout << "   " << fn << " - " << q->IndexFullName(kk)
             << " " << dotp << " " << diff 
             << endl;

      dotpsum += dotp; diffsum += diff; isecsum += isec;
      euclsum += eucl; jdivsum += jdiv;
    }
    if (debug_classify)
      cout << "   SUM     " << dotpsum << " " << diffsum << endl;

    dotpres.insert(pair<double,string>(-dotpsum, q->Identity()));
    diffres.insert(pair<double,string>( diffsum, q->Identity()));
    isecres.insert(pair<double,string>(-isecsum, q->Identity()));
    euclres.insert(pair<double,string>( euclsum, q->Identity()));
    jdivres.insert(pair<double,string>( jdivsum, q->Identity()));

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoClassificationKnn(size_t k) {
    string msg = "DoClassificationKnn() : ";

    DataBase *db = GetDataBase();

    typedef multimap<double,string> res_t;
    res_t& knnres  = classresult["knn"];
    res_t& knninfo = classresult["knninfo"];

    bool ok = true;

    if (debug_classify)
      cout << endl << msg << endl;

    map<string,size_t> hits;
    for (size_t ci=0; ci<NclassModels(); ci++)
      hits[ClassModelName(ci)] = 0;    

    for (size_t m=0; m<Nindices(); m++) {
      if (debug_classify)
        cout << IndexFullName(m) << endl;
      
      multimap<float,string> nnseen;

      for (size_t p=0; p<NseenObjects(); p++)
        if (SeenObject(p).Value()>0 &&
            PicSOM::TargetTypeContains(SeenObject(p).TargetType(),
                                       Target())) {
          size_t idx = SeenObject(p).Index();
          if (debug_classify)
            cout << "   " << SeenObject(p).Label() << " #" << idx << endl;

	  VectorIndex *vidx =
	    dynamic_cast<VectorIndex*>(IndexDataVectorIndex(m).StaticPart());
          const FloatVector *vec =
            IndexDataVectorIndex(m).Data().GetByNumber(idx);
          if (!vec) {
            ok = ShowError(msg+IndexFullName(m)+" vector for <"+
                           SeenObject(p).Label()+"> not found among "+
                           ToStr(IndexDataVectorIndex(m).Data().Nitems())+
                           " vectors, idx="+ToStr(idx)+", vidx="+ToStr(vidx));
            continue;
          }
          if (SeenObject(p).Label()!=vec->LabelStr()) {
            ok = ShowError(msg+IndexFullName(m)+" vector labels <"+
                           SeenObject(p).Label()+"> and <"+vec->LabelStr()+
                           "> do not match, idx="+ToStr(idx));
            continue;
          }

          multimap<float,string> nncls;

          for (size_t ci=0; ci<NclassModels(); ci++) {
            Query *q = db->ClassModel(ClassModelName(ci));
            if (!q) {
              ok = ShowError(msg+"class model of \""+
                             ClassModelName(ci)+"\" not found");
              continue;
            }

            FloatVectorSet& sample = q->IndexDataVectorIndex(m).sample;
	    sample.Metric(FloatVector::MakeDistance("euclidean"));
	    //sample.Metric(FloatVector::MakeDistance("histogram_intersection"));
	    //sample.Metric(FloatVector::MakeDistance("chisquare"));
	    //sample.Metric(FloatVector::MakeDistance("kullback"));

            if (debug_classify)
              cout << "      " << q->Identity() << " " << sample.Nitems()
                   << " samples" << endl;

	    //using FloatVector::DistanceType;
	    FloatVector::DistanceType disttype(FloatVector::DistanceType::cosine);

            IntVector nnidx(k);
            FloatVector nndist;
	    
            sample.NearestNeighbors(*vec, nnidx, nndist, &disttype);
            
            for (int n=0; n<nnidx.Length(); n++) {
              if (nnidx[n]==-1) {
                if (debug_classify)
                  cout << "         n=" << n << " nnidx[n]=-1" << endl;
                continue;
              }

              size_t sidx = sample[nnidx[n]].Number();
              if (debug_classify)
                cout << "         " << nnidx[n] << " " << sidx
                     << " <" << Label(sidx) << "> " << nndist[n]
                     << endl;

	      if (n==0)
		knninfo.insert(make_pair(nndist[n], Label(sidx)));
              
              nncls.insert(make_pair(nndist[n], q->Identity()));
            }
          }

          size_t nnadd = 0;
          for (multimap<float,string>::const_iterator nni=nncls.begin();
               nni!=nncls.end(); nni++) {
            if (debug_classify)
              cout << "   " << SeenObject(p).Label() << " "
                   << nni->first << " " << nni->second << endl;
            if (nnadd++<k)
              nnseen.insert(*nni);
          }
        }

      for (multimap<float,string>::const_iterator nni=nnseen.begin();
               nni!=nnseen.end(); nni++) {
        if (debug_classify)
          cout << IndexFullName(m) << " "
               << nni->first << " " << nni->second << endl;
        hits[nni->second]++;
      }
    }

    for (map<string,size_t>::const_iterator i=hits.begin(); i!=hits.end(); i++)
      knnres.insert(make_pair(-float(i->second), i->first));

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoClassificationSvm() {
    string msg = "DoClassificationSvm() : ";

    DataBase *db = GetDataBase();

    // typedef multimap<double,string> res_t;
    // res_t& svmres = classresult["svm"];

    bool ok = true;

    if (debug_classify)
      cout << endl << msg << endl;

    for (size_t m=0; m<Nindices(); m++) {
      if (debug_classify)
        cout << IndexFullName(m) << endl;
      
      multimap<float,string> nnseen;

      for (size_t p=0; p<NseenObjects(); p++)
        if (SeenObject(p).Value()>0 &&
            PicSOM::TargetTypeContains(SeenObject(p).TargetType(),
                                       Target())) {
          size_t idx = SeenObject(p).Index();
          if (debug_classify)
            cout << "   " << SeenObject(p).Label() << " #" << idx << endl;

	  VectorIndex *vidx =
	    dynamic_cast<VectorIndex*>(IndexDataVectorIndex(m).StaticPart());
          const FloatVector *vec =
            IndexDataVectorIndex(m).Data().GetByNumber(idx);
          if (!vec) {
            ok = ShowError(msg+IndexFullName(m)+" vector for <"+
                           SeenObject(p).Label()+"> not found among "+
                           ToStr(IndexDataVectorIndex(m).Data().Nitems())+
                           " vectors, idx="+ToStr(idx)+", vidx="+ToStr(vidx));
            continue;
          }
          if (SeenObject(p).Label()!=vec->LabelStr()) {
            ok = ShowError(msg+IndexFullName(m)+" vector labels <"+
                           SeenObject(p).Label()+"> and <"+vec->LabelStr()+
                           "> do not match, idx="+ToStr(idx));
            continue;
          }

          multimap<float,string> nncls;

          for (size_t ci=0; ci<NclassModels(); ci++) {
            Query *q = db->ClassModel(ClassModelName(ci));
            if (!q) {
              ok = ShowError(msg+"class model of \""+
                             ClassModelName(ci)+"\" not found");
              continue;
            }

            const FloatVectorSet& sample = q->IndexDataVectorIndex(m).sample;

            if (debug_classify)
              cout << "      " << q->Identity() << " " << sample.Nitems()
                   << " samples" << endl;

            /*
            IntVector nnidx(k);
            FloatVector nndist;
            sample.NearestNeighbors(*vec, nnidx, nndist);
            
            for (int n=0; n<nnidx.Length(); n++) {
              if (nnidx[n]==-1) {
                if (debug_classify)
                  cout << "         n=" << n << " nnidx[n]=-1" << endl;
                continue;
              }

              size_t sidx = sample[nnidx[n]].Number();
              if (debug_classify)
                cout << "         " << nnidx[n] << " " << sidx
                     << " <" << Label(sidx) << "> " << nndist[n]
                     << endl;
              
              nncls.insert(make_pair(nndist[n], q->Identity()));
            }
            */
          }

          /*
          size_t nnadd = 0;
          for (multimap<float,string>::const_iterator nni=nncls.begin();
               nni!=nncls.end(); nni++) {
            if (debug_classify)
              cout << "   " << SeenObject(p).Label() << " "
                   << nni->first << " " << nni->second << endl;
            if (nnadd++<k)
              nnseen.insert(*nni);
          }
          */
        }

      for (multimap<float,string>::const_iterator nni=nnseen.begin();
               nni!=nnseen.end(); nni++) {
        if (debug_classify)
          cout << IndexFullName(m) << " "
               << nni->first << " " << nni->second << endl;
        // hits[nni->second]++;
      }
    }

    /*
    for (map<string,size_t>::const_iterator i=hits.begin(); i!=hits.end(); i++)
      svmres.insert(make_pair(-float(i->second), i->first));
    */

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::ConvolutionTest(int i, int j) {
    simple::FloatMatrix& hits = Hits(i, j, 0);
    hits.Zero();
    hits.Set(hits.Rows()/2, hits.Columns()/2, 1);

    Convolve(i, j, 0);

    Convolved(i, j, 0).WriteMatlab("convolutiontest.m");
  }

  /////////////////////////////////////////////////////////////////////////////

  int Query::FindTsSom(const string& namestr, int *level,
                       IntPoint *ip, FloatPoint *fp) const {
    const char *name = namestr.c_str();
    for (size_t i=0; i<Nindices(); i++) {
      const string& somname = IndexShortName(i);
      size_t l = somname.size();
      if (somname==namestr.substr(0,l))
        if (!name[l] || name[l]=='[') {
          TsSom(i).SolveLevelAndUnit(name+l, level, ip, fp);
          return i;
        }
    }

    return -1;
  }

  /////////////////////////////////////////////////////////////////////////////

  const simple::FloatMatrix *Query::GetConvolved(int m, int l, int k) const {
    if (IsTsSom(m))
      return IndexDataTSSOM(m).GetConvolved(l, k);
    else
      return parent ? parent->GetConvolved(m, l, k) : NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  const simple::FloatMatrix *Query::GetHits(int m, int l, int k) const {
    if (IsTsSom(m))
      return IndexDataTSSOM(m).GetHits(l, k);
    else
      return parent ? parent->GetHits(m, l, k) : NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata Query::CreateMapImage(const simple::FloatMatrix& mat,
                                  const imagedata *bg) const {
    return BlueWhiteRedMap(mat, NULL, NULL, NULL, NULL, 1, bg);
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata Query::CreateMapImage(int m, int l, int k,
                                  const string& spec) const {
    bool convolved = spec.find("convolved")!=string::npos;
    bool nogreen   = spec.find("nogreen")!=string::npos;
    bool redonly   = spec.find("redonly")!=string::npos;

    simple::FloatMatrix zero(Map(m, l).Height(), Map(m, l).Width()); 
    const simple::FloatMatrix *mat =
      convolved ? GetConvolved(m, l, k) : GetHits(m, l, k);

    if (!mat || !mat->Rows())
      mat = &zero;    

    const int sl = 256;
    int w = mat->Columns()<sl ? mat->Columns() : sl;
    int h = mat->Rows()<sl    ? mat->Rows()    : sl;

    float dx = (float)mat->Columns()/w;
    float dy = (float)mat->Rows()/h;

    simple::FloatMatrix val(h, w);
    IntMatrix shift(h, w);
    IntMatrix green(h, w);

    for (int x=0; x<w; x++) {
      int sx = (int)floor(dx*x);
      for (int y=0; y<h; y++) {
        int sy = (int)floor(dy*y);
      
        val.Set(y, x, (*mat)(sy, sx));
      
        if (IsInZoomArea(m, l, sx, sy))
          shift.Set(y, x, 1);

        // int idx = Map(m, l).Unit(sx, sy)->LabelIndex();
        // if (!nogreen && idx>=0 && IsInNewObjects(idx))
        //   green.Set(y, x, 1);
      }
    }

    if (!nogreen) {
      const TreeSOM& ts = MapEvenAlien(m, l);
      const IntVector& div = Division(m, l);
      for (size_t i=0; i<NnewObjects(); i++) {
	const Object &o = NewObject(i);
	if (o.Retained() && div.IndexOK(o.Index())) {
	  int mapidx = div[o.Index()];
	  IntPoint p = ts.ToPoint(mapidx);
	  int x = (int)floor(p.X()/dx), y = (int)floor(p.Y()/dy);
	  if (green.IndexOK(y, x))
	    green.Set(y, x, 1);
	}
      }
    }

    const map<string,imagedata>& bg = TsSom(m).BackGround();
    const imagedata *imgd = bg.empty() || int(TsSom(m).Nlevels())-1!=l ? NULL
      : &bg.begin()->second;

    if (redonly) { // offset the values to be non-negative
      float mi = val.Minimum();
      val.Add(-mi);
    }

    return BlueWhiteRedMap(val, &shift, &green, NULL, NULL, 1, imgd);
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata Query::BlueWhiteRedMap(const simple::FloatMatrix& mat,
                                   const IntMatrix *shift,
                                   const IntMatrix *green,
                                   const IntMatrix *hbar,
                                   const IntMatrix *vbar,
                                   int mag, const imagedata *bg) const {

    imagedata bwrm = BlueWhiteRedMapInner(mat, shift, green, hbar, vbar, mag);

    if (!bg)
      return bwrm;

    imagedata bgimg = *bg;
    bgimg.force_three_channel();
    scalinginfo si(bwrm.width(), bwrm.height(), bgimg.width(), bgimg.height());
    bwrm.rescale(si);

    for (size_t x=0; x<bwrm.width(); x++)
      for (size_t y=0; y<bwrm.height(); y++) {
        vector<unsigned char> bgv = bgimg.get_uchar(x, y);
        vector<unsigned char> brv =  bwrm.get_uchar(x, y);

        int sum = 0;
        for (size_t i=0; i<brv.size(); i++)
          sum += brv[i];
        
        const int min = 255, max = 765;
        if (sum<min) {
          cout << "SUM=" << sum << endl;
          sum = min;
        }

        float v = float(sum-min)/(max-min);

        for (size_t i=0; i<bgv.size(); i++)
          brv[i] = (unsigned int)floor(v*bgv[i]+(1-v)*brv[i]+0.5);

        bwrm.set(x, y, brv);
      }

    return bwrm;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata Query::BlueWhiteRedMapInner(const simple::FloatMatrix& mat,
                                        const IntMatrix *shift,
                                        const IntMatrix *green,
                                        const IntMatrix *hbar,
                                        const IntMatrix *vbar,
                                        int mag) const {

    //    cout << "in Query::BlueWhiteRedMap()" << endl;
    float maxg = 15;
    float ma = maxposvalue!=MAXFLOAT ? maxposvalue : fabs(mat.Maximum());
    float mi = maxnegvalue!=MAXFLOAT ? maxnegvalue : fabs(mat.Minimum());

    if (ma)
      ma = maxg/(2*ma);
    if (mi)
      mi = maxg/(2*mi);

    int ng = (int)(2*maxg+5);

    float rv[] = { 0, 2, 5, 7, 9,11,13,15,15,15,15,15,15,15,15,15,
                   2, 3, 5, 6, 7, 8, 9,10,10,10,10,10,10,10,10,10, 13,  0, 0 };

    float gv[] = { 0, 2, 5, 7, 9,11,13,15,15,13,11, 9, 7, 5, 2, 0,
                   4, 5, 6, 7, 8, 9,10,11,11,10, 9, 8, 7, 6, 5, 4, 13, 15, 0 };

    float bv[] = { 15,15,15,15,15,15,15,15,15,13,11, 9, 7, 5, 2, 0,
                   9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 7, 6, 5, 4, 2, 1,  0,  0, 0 };

    int rows = mat.Rows(), cols = mat.Columns();

    imagedata imgd(mag*cols+(vbar?cols-1:0), mag*rows+(hbar?rows-1:0),
                   3, imagedata::pixeldata_uchar);

    FloatVector r(ng, rv), g(ng, gv), b(ng, bv);
    r /= 15; g /= 15; b /= 15;

    if (greymaps==1) {
      green = NULL;
      for (int i=0; i<r.Length(); i++)
        if (r[i]>b[i])
          r[i] = g[i] = b[i] = 1-0.5*b[i];
        else if (b[i]>r[i])
          r[i] = g[i] = b[i] = 0.5*r[i];
        else
          r[i] = g[i] = b[i] = 0.5;
    }

    if (greymaps==2) {
      green = NULL;
      for (int i=0; i<r.Length(); i++)
        if (r[i]>b[i])
          g[i] = r[i] = b[i];
        else
          r[i] = g[i] = b[i] = 1;
    }

    if (greymaps==3) {
      green = NULL;
      for (int i=0; i<r.Length(); i++)
        if (r[i]>b[i])
          g[i] = b[i] = r[i];
        else
          r[i] = g[i] = b[i] = 0;
    }

    if (greymaps==4) {
      green = NULL;
      for (int i=0; i<r.Length(); i++)
        if (r[i]>b[i]) {
          r[i] -= b[i];
          g[i] = b[i] = 0;
        } else
          r[i] = g[i] = b[i] = 0;
    }

    vector<vector<unsigned char> > vals;
    for (int i=0; i<ng; i++) {
      vector<unsigned char> vv(3);
      vv[0] = (int)(255.0*r[i]);
      vv[1] = (int)(255.0*g[i]);
      vv[2] = (int)(255.0*b[i]);
      // cout << (int)vv[0] << " " << (int)vv[1] << " " << (int)vv[2] << endl;
      vals.push_back(vv);
    }

    int greenvalue = (int)(2*maxg+3), blackvalue = greenvalue+1;

    bool graybars = false;
    if (hbar && Simple::StringsMatch(hbar->Label(), "graybars"))
      graybars = true;

    try {
      for (int y=0; y<rows; y++)
        for (int x=0; x<cols; x++) {
          int value = ValueToColorIndex(mat.Get(y, x), maxg, ma, mi);
          int shiftval = 0;

          if (shift && shift->Get(y, x))
            shiftval = (int)maxg+1;
      
          if (green && green->Get(y, x)) {
            shiftval = 0;
            value = greenvalue;
          } 

          if (value+shiftval<0 || value+shiftval>=ng) {
            WarnOnce("BlueWhiteRedMapInner() strange value+shiftval");
            continue;
          }

          int x0 = (mag+(vbar!=NULL))*x, y0 = (mag+(hbar!=NULL))*y, xi, yi;
          for (xi=0; xi<mag; xi++)
            for (yi=0; yi<mag; yi++)
              imgd.set(x0+xi, y0+yi, vals[value+shiftval]);

          // cout << "x=" << x << " y=" << y << " v=" << value+shiftval
          //      << " (" << mat.Get(y, x) << ")" << endl;

          if (!graybars) {
            if (hbar && y<rows-1) {
              value = hbar->Get(y, x) ? blackvalue :
                ValueToColorIndex((mat.Get(y, x)+mat.Get(y+1, x))/2, maxg,ma,mi);
              for (xi=0; xi<mag; xi++)
                imgd.set(x0+xi, y0+mag, vals[value]);
            }
            if (vbar && x<cols-1) {
              value = vbar->Get(y, x) ? blackvalue :
                ValueToColorIndex((mat.Get(y, x)+mat.Get(y, x+1))/2, maxg, ma, mi);
              for (yi=0; yi<mag; yi++)
                imgd.set(x0+mag, y0+yi, vals[value]);
            }
            if (hbar && y<rows-1 && vbar && x<cols-1) {
              int nb = vbar->Get(y, x) + hbar->Get(y, x) +
                vbar->Get(y+1, x) + hbar->Get(y, x+1);
              value = nb>1 ? blackvalue :
                ValueToColorIndex((mat.Get(y+1, x+1)+mat.Get(y, x+1)+
                                   mat.Get(y+1, x)+mat.Get(y, x))/4, maxg, ma, mi);
              imgd.set(x0+mag, y0+mag, vals[value]);
            }
          } else {
            vector<unsigned char> grayv(3);
            if (y<rows-1) {
              grayv[0] = grayv[1] = grayv[2]
                = 255 - (hbar->Get(y, x) > 255 ? 255 : hbar->Get(y, x));
              for (xi=0; xi<mag; xi++)
                imgd.set(x0+xi, y0+mag, grayv);
            }
            if (x<cols-1) {
              grayv[0] = grayv[1] = grayv[2]
                = 255 - (hbar->Get(y, x) > 255 ? 255 : vbar->Get(y, x));
              for (yi=0; yi<mag; yi++)
                imgd.set(x0+mag, y0+yi, grayv);
            }
            if (y<rows-1 && x<cols-1) {
              int nb = ( vbar->Get(y, x) + hbar->Get(y, x) +
                         vbar->Get(y+1, x) + hbar->Get(y, x+1) )/4;
              grayv[0] = grayv[1] = grayv[2] = 255 - (nb > 255 ? 255 : nb);
              imgd.set(x0+mag, y0+mag, grayv);
            }
          }
        }
    }
    catch (const string& sexc) {
      ShowError("Query::BlueWhiteRedMapInner() catched : ", sexc);
    }

    return imgd;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata Query::ColorIndexMap(const IntMatrix& mat,
                                 const IntMatrix *hbar,
                                 const IntMatrix *vbar,
                                 int mag) const {
    vector<vector<float> > c;
    vector<float> v(3);
    v[0] = 0; v[1] = 0; v[2] = 0; c.push_back(v);
    v[0] = 1; v[1] = 1; v[2] = 1; c.push_back(v);
    v[0] = 1; v[1] = 0; v[2] = 0; c.push_back(v);
    v[0] = 0; v[1] = 1; v[2] = 0; c.push_back(v);
    v[0] = 0; v[1] = 0; v[2] = 1; c.push_back(v);
    v[0] = 1; v[1] = 1; v[2] = 0; c.push_back(v);
    v[0] = 1; v[1] = 0; v[2] = 1; c.push_back(v);
    v[0] = 0; v[1] = 1; v[2] = 1; c.push_back(v);

    int rows = mat.Rows(), cols = mat.Columns();

    imagedata imgd(mag*cols+(vbar?cols-1:0), mag*rows+(hbar?rows-1:0),
                   3, imagedata::pixeldata_uchar);

    for (int y=0; y<rows; y++)
      for (int x=0; x<cols; x++) {
        int pv = mat(y, x);
        if (pv>=(int)c.size())
          pv = 0;

        int x0 = (mag+(vbar!=NULL))*x, y0 = (mag+(hbar!=NULL))*y, z = (x0+y0)%2;
        int xi, yi;
        for (xi=0; xi<mag; xi++)
          for (yi=0; yi<mag; yi++)
            imgd.set(x0+xi, y0+yi, c[pv]);

        if (hbar && y<rows-1)
          for (xi=0; xi<mag; xi++)
            imgd.set(x0+xi, y0+mag, c[ hbar->Get(y, x) ? (z+mag+xi)%2 : pv ]);

        if (vbar && x<cols-1)
          for (yi=0; yi<mag; yi++)
            imgd.set(x0+mag, y0+yi, c[ vbar->Get(y, x) ? (z+mag+yi)%2 : pv ]);

        if (hbar && y<rows-1 && vbar && x<cols-1) {
          int nb = vbar->Get(y, x) + hbar->Get(y, x) +
            vbar->Get(y+1, x) + hbar->Get(y, x+1);
          imgd.set(x0+mag, y0+mag, c[nb>1 ? z : pv]);
        }
      }
    return imgd;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ShowMapPoints(const string& lab)  {
    if (!the_db)
      return ShowError("Query::ShowMapPoints() the_db not set");

    if (!Nindices())
      return ShowError("Query::ShowMapPoints() no maps selected");

    ReadFiles();

    int idx = LabelIndex(lab);
    Object img(the_db, idx, select_answer);
    cout << "label=" << lab << "  index=" << idx << endl;

    for (size_t i=0; i<Nindices(); i++)
      for (size_t j=0; j<TsSom(i).Nlevels(); j++) {
        int u = TsSom(i).Division(j, 0)[idx];
        IntPoint p = Map(i, j).ToPoint(u);
        cout << "label=" << lab << "  index=" << idx << "  "
             << IndexShortName(i) << "[" << j << "] -> " << u
             << " (" << p.X() << "," << p.Y() << ")" << endl;
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::MapSnapshots(int i, int j, const string &xs) const {
    bool do_hits = true, do_conv = true;

    string n;

    for (size_t k=0; k<GetMatrixCount(i); k++) {
           bool scale_to_red = k>0 ? false: true;
      // bool scale_to_red = false;

      stringstream ss;
      ss << Identity() << "_" << IndexShortName(i) << "_" << j;
      if (GetMatrixCount(i)!=1)
        ss << "_" << k;
      ss << xs;

      string hname = ss.str(), cname = hname+"_conv", ext = ".png";
    
      if (mapimagename!="") {
        if (do_hits) {
          n =  mapimagename+hname+ext;
          string argstr;
          // if(scale_to_red)
          //  argstr="redonly";
          imagedata hdata = CreateMapImage(i, j, k, argstr);

          imagefile::write(hdata, n);

          WriteLog("Wrote map hit image <", n, ">");
        }
        if (do_conv) {
          n = mapimagename+cname+ext;
          string argstr="convolved";

          if(scale_to_red)
            argstr+=",redonly";
          imagedata cdata = CreateMapImage(i, j, k, argstr);
          imagefile::write(cdata, n);
          WriteLog("Wrote map convolution image <", n, ">");
        }
      }

      if (mapmatlabname!="" || mapdatname!="") {
        float *ch = Hits(i, j, k).TransposeConcatenate();
        FloatVector vh(Hits(i, j, k).Size(), ch);
        delete ch;
        float *cc = Convolved(i, j, k).TransposeConcatenate();
        FloatVector vc(Convolved(i, j, k).Size(), cc);
        delete cc;
      
        if (mapmatlabname!="") {
          if (do_hits) {
            n = Simple::MakeMatlabCompliant(mapmatlabname+hname);
            vh.Label(n).WriteMatlab(n+".m");
            WriteLog("Wrote map hit matlab vector file <", n, ".m>");
            n += "_m";
            simple::FloatMatrix mm = Hits(i, j, k);
            mm.Label(n).WriteMatlab(n+".m");
            WriteLog("Wrote map hit matlab matrix file <", n, ".m>");
          }
          if (do_conv) {
            n = Simple::MakeMatlabCompliant(mapmatlabname+cname);
            vc.Label(n).WriteMatlab(n+".m");
            WriteLog("Wrote map convolution matlab vector file <", n, ".m>");
            n += "_m";
            simple::FloatMatrix mm = Convolved(i, j, k);
            mm.Label(n).WriteMatlab(n+".m");
            WriteLog("Wrote map convolution matlab matrix file <", n, ".m>");
          }
        }
        if (mapdatname!="") {
          if (do_hits) {
            n = mapdatname+hname;
            vh.Label(n).Write(n+".dat");
            WriteLog("Wrote map hits in <", n, ".dat>");
          }
          if (do_conv) {
            n = mapdatname+cname;
            vc.Label(n).Write(n+".dat");
            WriteLog("Wrote map convolution in <", n, ".dat>");
          }
        }
      }
    }
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::WriteAnalyseVariablesOld(const string& head, int map,
                                       const ground_truth *correct, 
                                       const ground_truth *tested,
                                       int nrounds, bool do_chain,
                                       string& mat) {
    string clsfile, testfile;
    if (correct)
      clsfile  = CommaJoin(vector<string>(correct->depends().begin(),
                                          correct->depends().end()));
    if (tested)
      testfile = CommaJoin(vector<string>(tested->depends().begin(),
                                          tested->depends().end()));

    string hh = "Analyse"+head+": ";

    DoubleLogSeparator(mat);

    if (Identity()!="")
      DoubleLog(mat, hh+"identity=", Identity());

    DoubleLog(mat, hh+"database=", GetDataBase()->Name(),
              " in ", GetDataBase()->Path());

    list<string> dbtts = GetDataBase()->TargetTypeStatisticsLong();
    for (list<string>::const_iterator i=dbtts.begin(); i!=dbtts.end(); i++)
      DoubleLog(mat, "  ", *i);

    if (DataBaseRestrictionSize(target_any_target)!=DataBaseSize()) {
      string rns = GetDataBase()->RestrictionName();
      stringstream s;
      s << " size(any)=" << DataBaseRestrictionSize(target_any_target)
        << " size(" << TargetTypeString(Target()) << ")="
        << DataBaseRestrictionSize(Target());
      DoubleLog(mat, hh+"restriction="+rns, s.str());

      // this affects some other numbers below too...
    }

    char tmps[1000];
    sprintf(tmps, " (%d)", NumberTargetTypeObjects());
    DoubleLog(mat, hh+"target=", TargetTypeString(Target()), tmps);

    if (correct) {
      sprintf(tmps, " %d positives (%d zeros, %d negatives) of total %d",
              (int)correct->NumberOfEqual(1), (int)correct->NumberOfEqual(0),
              (int)correct->NumberOfEqual(-1), correct->Length());

      string tmp = correct->Label()!="" ? correct->Label() : "???";
      DoubleLog(mat, hh+"class="+tmp+tmps);
    }

    if (clsfile!="")
      DoubleLog(mat, hh+"classfile=", clsfile);

    if (tested) {
      sprintf(tmps, " %d positives (%d zeros, %d negatives) of total %d",
              (int)tested->NumberOfEqual(1), (int)tested->NumberOfEqual(0),
              (int)tested->NumberOfEqual(-1), tested->Length());

      string tmp = tested->Label()!="" ? tested->Label() : "???";
      DoubleLog(mat, hh+"testset="+tmp+tmps);
    }

    if (testfile!="")
      DoubleLog(mat, hh+"testsetfile=", testfile);

    return WriteAnalyseVariablesCommon(hh, mat, map, nrounds, do_chain);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::WriteAnalyseVariablesNew(const string& head, string& mat,
                                       const ground_truth_list& gtl_in,
                                       const list<string>& xtra) {
    string hh = "Analyse"+head+": ";

    DoubleLogSeparator(mat);

    if (Identity()!="")
      DoubleLog(mat, hh+"identity=", Identity());

    DataBase *db = GetDataBase();
    if (!db)
      return ShowError("WriteAnalyseVariablesNew() : no database set");

    DoubleLog(mat, hh+"database=", db->Name(), " in ", db->Path());
    DoubleLog(mat, hh+"setup=", setupname);

    list<string> dbtts = db->TargetTypeStatisticsLong();
    for (list<string>::const_iterator i=dbtts.begin(); i!=dbtts.end(); i++)
      DoubleLog(mat, "  ", *i);

    ground_truth_list gtl = gtl_in;
    bool rstr_found = false;
    for (ground_truth_list::const_iterator i=gtl.begin();
         i!=gtl.end() && !rstr_found; i++)
      if (i->first=="restriction")
        rstr_found = true;

    const ground_truth& db_rstr = db->RestrictionGT();
    if (!rstr_found && db_rstr.Label()!="*") // !db_rstr.empty()
      Analysis::AddGroundTruthInfoFirst(gtl, "restriction", db_rstr);

    for (ground_truth_list::const_iterator i=gtl.begin(); i!=gtl.end(); i++)
      if (i->second.first.label()!="") {
        DoubleLog(mat, " ");
        string s = db->GroundTruthSummaryStr(i->second.first,
                                             true, true, true,
                                             i->second.second);
        DoubleLog(mat, hh+i->first+"="+s);
      }
    DoubleLog(mat, " ");

    char tmps[100];
    sprintf(tmps, " (%d/%d)", (int)db->RestrictionSize(Target()),
            NumberTargetTypeObjects());
    DoubleLog(mat, hh+"target=", TargetTypeString(Target()), tmps);

    bool ok = WriteAnalyseVariablesCommon(hh, mat, -1, 0, false);

    for (list<string>::const_iterator i=xtra.begin(); i!=xtra.end(); i++)
      DoubleLog(mat, hh+*i);

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::WriteAnalyseVariablesCommon(const string& hh, string& mat,
                                          int map, int /*nrounds*/,
                                          bool /*do_chain*/) {

    DoubleLog(mat, hh+"relevance_to_siblings=", 
              (RelevanceToSiblings()?"true":"false"));

    string algo = PicSOM::CbirAlgorithmP(algorithm);
    if (algorithm==cbir_external)
      algo += "/"+external_algorithm;
    if (!algorithms.empty())
      algo = algorithms[0].fullname;

    if (algorithm==cbir_vq || algorithm==cbir_vqw) {
      stringstream ss;
      ss << VQlevel();
      algo += " vqlevel="+ss.str();
    }
    DoubleLog(mat, hh+"algorithm=", algo);

    // obs! HAS_CBIRALGORITHM_NEW problem?:
    if (!algorithms.empty()) // || CbirAlgorithm->StageBased()
      for (int s=stage_first; s<=stage_last; s++) {
        cbir_function fd = StageFunction((cbir_stage)s, true);
        cbir_function fr = StageFunction((cbir_stage)s, false, false, false);
        if ((int)fd==-1)
          break;
        if ((int)fr==-1)
          continue;
        
        cbir_function sd = StageFunction((cbir_stage)s, true, true);
        
        const string& a = StageArgs((cbir_stage)s);
        string sargs = a!="" ? "("+a+")" : "";
        
        DoubleLog(mat, fd!=func_default||a.size()?fr!=sd?"  * ":"  + ":"    ",
                  StageNameP((cbir_stage)s), "=", FunctionNameP(fr), sargs);
      }
    
    DoubleLog(mat, hh+"aspect_weight=", aspect_weight.str());

    const string& def_conv = ConvType(-1);
    if (def_conv!="")
      DoubleLog(mat, hh+"default_convtype=", def_conv);
  
    if (map==-1)
      DoubleLog(mat, hh+"using features:");

    const algorithm_data *a = NULL;
    if (Algorithms().size())
      a = &Algorithms()[0];

    for (size_t qq=0; qq<Nindices(a); qq++) {
      const Index& ind = IndexStaticData(a, qq);
      string str = "  "+ind.Name()+" ["+ind.FeatureTargetString()+"] ";
      if (UsingAspects())
        str += "aspects="+IndexAspects(a, qq)+" ";
      str += ind.TargetCounts();
    
      if (IsTsSom(qq)) {
        str += " "+TsSom(qq).Map().Structure();
        if (ConvType(qq)!=def_conv)
          str += " conv="+ConvType(qq); 
      }        
      if (map==-1)
        DoubleLog(mat, str);

      else if (int(qq)==map)
        DoubleLog(mat, hh+"using feature:"+str);
    }

    DoubleLog(mat, hh+"maxquestions=",  ToStr(maxquestions));
    DoubleLog(mat, hh+"can_show_seen=", ToStr(can_show_seen));
    
    // DoubleLog(mat, hh+"do_chain=",        ToStr(do_chain));

    DoubleLog(mat, hh+"rndseed=",         RndSeedStr());

    // if (nrounds)
    //   DoubleLog(mat, hh+"rounds=", ToStr(nrounds));

    if (!TextQuery().empty()) {
      DoubleLog(mat, hh+"textquery=\"", TextQuery(), "\"");
      DoubleLog(mat, hh+"textquerylog=\"", GetDataBase()->TextQueryLog(), "\"");
      DoubleLog(mat, hh+"textquery_processed=\"", TextQueryProcessed(), "\"");
      DoubleLog(mat, hh+"correct_person=", textquery_correct_person?"on":"off");
      DoubleLog(mat, hh+"correct_location=", 
                textquery_correct_location?"on":"off");
      DoubleLog(mat, hh+"commons_classes=", check_commons_classes?"on":"off");
      DoubleLog(mat, hh+"commons_classes_useaspects=",
		commons_classes_useaspects?"on":"off");
      DoubleLog(mat, hh+"add_classfeatures=", add_classfeatures?"on":"off");
      // classaugment

      if (!classaugment.empty()) {
        DoubleLog(mat, hh+"Class model augmentations used:");
        for (classaugment_iter_t it=classaugment.begin(); 
             it != classaugment.end(); it++) {
          const string& classn = it->first.first;
          const string& featn = it->first.second;
          const float value = it->second;
          DoubleLog(mat, hh+" * ", classn, (featn.empty()?"":"/"), featn,
                    " with value "+ToStr(value));
        }
      }
    }
  
    DoubleLog(mat, hh+"matrixcount=",   ToStr(DefaultMatrixCount()));
    DoubleLog(mat, hh+"entropymatrix=", ToStr(entropymatrix));
    DoubleLog(mat, hh+"permapobjects=", ToStr(_sa_permapobjects));

    string bugmsg = hh+"pre 1.707 version bug compatibility ";
    bugmsg += Valued::Pre707Bug() ? "ON" : "OFF";
    DoubleLog(mat, bugmsg);
    bugmsg = hh+"pre 2.122 version bug compatibility ";
    bugmsg += Pre2122Bug() ? "ON" : "OFF";
    DoubleLog(mat, bugmsg);

#ifdef QUERY_USE_PTHREADS
    if (picsom->PthreadsTssom())
      DoubleLog(mat, hh+"tssom threads=", ToStr(picsom->Threads()));
#endif // QUERY_USE_PTHREADS

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::BmuConvType(int m) { 
    string ct;
    if (m==-1)
      ct = _sa_default_bmuconvtype;
    else {
      map<int,string>::const_iterator i = _sa_tssom_bmuconvtype.find(m);
      ct = i!=_sa_tssom_bmuconvtype.end() ? 
            i->second : _sa_default_bmuconvtype;
    }
    if (ct.empty()) { // generate default bmuconvtype as triangle-<bmudepths>
      const TSSOM& ts = TsSom(m);
      ct = "triangle";
      for (int i=0; i<(int)ts.Nlevels(); i++)
        ct += "-"+ToStr(ts.DivisionDepth(i));
      _sa_default_bmuconvtype = ct;
    }
    return ct;
  }


  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoDump(int mode, const string& fn) const {
    if (mode==0 && !Analysis::MatlabNameOK(fn)) {
      ShowError("<", fn, "> is not a proper matlab file name prefix");
      matlabdump = "";
      return false;
    }

    bool use_round = round!=1;

    string vcname = viewclass!=""?viewclass:"*";
    ground_truth vclass = CheckDB()->GroundTruthExpression(vcname,
                                                           Target(),
                                                           -1, false);
    stringstream namess;
    namess << fn;
    if (use_round)
      namess << "_" << round;
    string name = namess.str(), fname = name+(mode==0?".m":".dat");
    const string com = mode==0 ? "%" : "#";

    int n = 0;
    ofstream os(fname.c_str());
    os << com << endl;
    os << com << " dump of Query::newobject" << endl;
    os << com << endl;
    os << com << " features:";
    for (size_t i=0; i<Nindices(); i++)
      os << " " << IndexShortName(i);
    os << endl << com << endl;

    bool first = true;
    for (size_t i=0; i<NnewObjects(); i++) {
      const Object& o = NewObject(i);
      if (!o.NstageInfo() || vclass[o.Index()]!=1)
        continue;

      FloatVector v = o.StageInfo(o.NstageInfo()-1);
      if (v.Length())
        v.Pop();

      if (first && mode==1)
        os << v.Length() << endl;
      first = false;

      if (mode==0) {
        v.Label(name+"_"+o.Label());
        v.WriteMatlab(os, false);

      } else {
        v.Label(o.Label());
        const char *line = v.WriteToString();
        os << line << endl;
        delete line;
      }

      n++;
    }

    WriteLog("Wrote ", ToStr(n), " newobject vectors in <", fname, ">");

    return n && os;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::ShowObjectListsRecursively() const {
    cout << Identity() << " ";
    DumpSeen();

    if (!Nchildren()) {
      cout << Identity() << " ";
      DumpNew();
    }

    for (size_t i=0; i<Nchildren(); i++)
      Child(i)->ShowObjectListsRecursively();
  }

  /////////////////////////////////////////////////////////////////////////////

  void Query::ShowUnitAndObjectLists(bool seen, bool pmu, bool pmur,
                                     bool pmi, bool comb,
                                     bool uni, bool unir,
                                     bool nwi, bool nwir) const {
    if (seen)
      DumpSeen();

    for (size_t i=0; i<Nindices(); i++)
      for (int j=0; j<Nlevels(i); j++) {
        if (pmu && IsTsSom(i))
          DumpVQUnits(i, j);
        if (pmur && IsTsSom(i))
          DumpVQUnits(i, j, true);
        if (pmi && IsTsSom(i))
          DumpNew(i, j);
      }
    if (comb)
      DumpCombined();

    if (uni)
      DumpUnique();

    if (unir)
      DumpUnique(true);

    if (nwi)
      DumpNew();

    if (nwir)
      DumpNew(true);

    cout << "random_image_count = " << random_image_count << endl;
    cout << "-------------" << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Save(const string& file, bool force) /*const*/ {
    // cout << "Query::Save() called with " << identity << endl;
    if (parent) {
      /*const*/ Query *p = this;
      while (p->parent)
        p = p->parent;

      return p->Save(file, force);
    }

    if (!force && !NeedsSave()) {
      // cout << "Skipping save of " << identity << " to " << file << endl;
      return true;
    }

    picsom->MakeDirectory(file, true);
    SetTimeNow(saved_time);
    pair<XmlDom,XmlDom> docroot = Connection::XMLdocroot("savedquery", false);
    bool ok = AddToXMLqueryinfo(docroot.second, true, true,
				false, false, NULL);
    ok = ok && docroot.first.Write(file, true, true);
    docroot.first.DeleteDoc();

    if (ok) {
      SetTimeNow(saved_time);
      WriteLog("Saved in <"+file+">");
    } else
      SetTimeZero(saved_time);

    // saved_time is really set twice,
    // * first to get the <savetime> field about correct
    // * second to be more recent than file's modification time

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Retrieve(const string& file, bool force,
                       const Connection *conn) {
    if (!FileExists(file)) {
      /// if file =~ database/Q:name then call recursively... 
      return ShowError("Query::Retrieve() could not read <", file, ">");
    }

    if (!force && MoreRecent(file, saved_time)) {
      WriteLog("Reread changed ", file);
      force = true;
    }
    if (!force)
      return true;

    for (size_t i=0; i<child_new.size(); i++)
      delete child_new[i];
    child_new.clear();

    Tic("Retrieve");

    XmlDom doc  = XmlDom::Parse(file);
    XmlDom root = doc.Root(); // savedquery
    XmlDom node = root.FirstChild();

    Connection::ConditionallyDumpXML(cout, false, doc, file);

    bool ok = false;
    for (; node && !ok; node = node.NextElement())
      if (node.NodeName()=="queryinfo")
        ok = Retrieve(node, conn);
  
    doc.DeleteDoc();

    Tac("Retrieve");

    if (!ok)
      return ShowError("Query::Retrieve(", file, ") failed");

    WriteLog("Retrieved from <"+file+">");
    // SetTime(saved_time, file);  // commented out for trecvid2005...

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Retrieve(const XmlDom& innode, const Connection *conn) {
    string msg = "Query::Retrieve() : ";
    if (innode.NodeName()!="queryinfo")
      return ShowError(msg+"not <queryinfo>");

    set<string> do_skip {
        "database", "viewclass", "positive", "negative", "classaugment",
	"namedcmdlines", "erf-imagedata", "infolinklist", "visitedlinklist",
	  "image" /*<-that should be removed...*/
	  };

    // if (!parent)
    InterpretDefaults(0);

    bool debug = false, show_failure = true, defaults_1_done = false;

    for (XmlDom node=innode.FirstElementChild(); node;
         node=node.NextElement()) {
      if (GetDataBase() && !defaults_1_done) {
	InterpretDefaults(1);
	defaults_1_done = true;
      }

      XmlDom child = node.FirstChild();

      string name          = node.NodeName();
      string content       = node.Content();
      string child_name    = child.NodeName();
      string child_content = child.Content();

      StripWhiteSpaces(content);
      StripWhiteSpaces(child_content);

      bool empty_content       = content=="";
      bool empty_child_content = child_content=="";

      if (debug)
        cout << "## name=[" << name
             << "] content=[" << content
             << "] child_name=[" << child_name
             << "] child_content=[" << child_content
             << "] empty_content=[" << empty_content
             << "] empty_child_content=[" << empty_child_content
             << "]" << endl;

      bool error = false;

      if (child.IsText() && !empty_child_content) {
        if (debug)
          cout << "## (1)" << endl;
        if (!InterpretXML(node, false, conn))
          return false;
      }
    
      else if (name=="childlist") {
        if (debug)
          cout << "## (2)" << endl;
        for (XmlDom cnode=node.FirstElementChild(); cnode;
             cnode=cnode.NextElement()) {
          Query *cq = new Query(picsom);
          cq->Parent(this);
          bool ok = cq->Retrieve(cnode, conn);
          if (!ok)
            return false;
        }
      }
    
      else if (empty_child_content) {
        if (debug)
          cout << "## (3)" << endl;
        // cout << "+++++++++" << endl;

	if (do_skip.find(name)!=do_skip.end())
	  continue;

        if (name=="algorithmlist") {
          algorithm = cbir_no_algorithm;  // this should be somewhere else...
          for (XmlDom alg = node.FirstElementChild(); alg;
               alg = alg.NextElement())
            if (alg.NodeName()=="algorithm") {
              map<string,string> keyval = ParseXMLKeyValues(alg, true);
              if (keyval["default"]=="1") {
                int res = 0;
                if (!Interpret("algorithm", keyval["name"], res, NULL) || !res)
                  ShowError(msg+"Interpret(algorithm,"+keyval["name"]
                            +") failed");
              }
            } 
          continue;
        }

        if (name=="algorithm") {
          bool ok = false;
          for (XmlDom cnode=child; cnode; cnode = cnode.Next())
            if (cnode.IsElement() &&
                cnode.NodeName()=="name" && cnode.FirstChildContent()!="") 
              ok = PicSOM::DoInterpret(name, cnode.FirstChildContent(), NULL,
                                       this, conn, NULL, NULL, NULL,
                                       false, true);
          if (ok)
            continue;
        }

        if (name=="featurelist") {
          const char *val = XMLextractFeatureNamesM(node.node);
          bool r = PicSOM::DoInterpret("features", val, NULL, this, conn,
                                       NULL, NULL, NULL, true, true);
          delete val;

          if (!r)
            return false;
        
          continue;
        }
      
        if (name=="variables" || name=="otherkeys") {
          if (!InterpretXML(child, true, conn))
            return false;
        
          continue;
        }

        if (name=="objects" || name=="images") {
          if (!InterpretXMLobjects(child))
            return false;

          continue;
        }      
      
        if (name=="ajaxdata") {
          if (!InterpretXMLajaxdata(node, false))
            return false;

          continue;
        }      
      
        error = true;
      
      } else

        error = true;
  
      if (error && show_failure) {
        stringstream ss;
        ss << "Query::Retrieve(XmlDom,...) failed with : "
           << "name=<"          << name          << "> "
           << "content=<"       << content       << "> "
           << "child_name=<"    << child_name    << "> "
           << "child_content=<" << child_content << ">";
        ShowError(ss.str());
      }
    }
    
    if (algorithm>=cbir_picsom && algorithm<=cbir_picsom_bottom_weighted)
      ReadFiles();
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::InterpretXML(const XmlDom& innode, bool iterate,
                           const Connection *conn) {
    set<string> empty_allowed { "textquery", "erfpolicy", "textsearch",
	"implicit_textquery", "textindex", "textfield" };

    xmlNodePtr snode = innode.node;

    for (xmlNodePtr node = snode; node; node = node->next) {
      if (!iterate && node!=snode)
        break;
    
      string key = NodeName(node);

      xmlNodePtr child = node->children;

      if (!child) {
        if (node->type==XML_ELEMENT_NODE &&
	    empty_allowed.find(key)!=empty_allowed.end())
          continue;

        if (node->type!=XML_TEXT_NODE)
          ShowError("Query::InterpretXML() : <"+key+"> has no children");

        continue;
      }

      if (!child->content) {
        ShowError("Query::InterpretXML() : <"+key+"> has no child->content");
        continue;
      }

      string val = NodeContent(child);
      StripWhiteSpaces(val);

      if (val=="")
        continue;

      static set<string> skips;
      if (skips.empty()) {
        const char *skipn[] = {
          "parent_identity", "maparea", "needs_save", "from_disk",
          "nodes", "leaves", "imagecount"
          // , "savetime" // removed for trecvid2005 manual results
        };
        skips.insert(skipn, skipn+sizeof(skipn)/sizeof(*skipn));
      }

      if (skips.find(key)!=skips.end())
        continue;

      if (key=="databasename")
        key = "database";

      //    cout << "KEY=[" << key << "] VAL=[" << val << "]" << endl;

      if (!PicSOM::DoInterpret(key, val, NULL, this, conn,
                               NULL, NULL, NULL, true, true))
        return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::InterpretXMLobjects(const XmlDom& innode) {
    xmlNodePtr snode = innode.node;

    for (xmlNodePtr node = snode; node; node = node->next) {
      if (node->type!=XML_ELEMENT_NODE)
        continue;

      string oname = NodeName(node);
      string name = oname;
      size_t p = name.find("imagelist");
      if (p!=string::npos)
        name.replace(p, 5, "object");

      char type = '*';
      if (name=="plusobjectlist")
        type = '+';
      else if (name=="minusobjectlist")
        type = '-';
      else if (name=="zeroobjectlist")
        type = '0';
      else if (name=="questionobjectlist")
        type = '?';
      else if (name=="mapobjectlist")
        type = '%';
      else if (name=="showobjectlist")
        type = '>';
      else if (name=="formerobjectlist")
        continue;
      else if (name=="textsearchresultobjectlist")
        continue;
      else
        return ShowError("Query::InterpretXMLobjects(\""+oname+"\") failed");

      for (xmlNodePtr l_node=node->children; l_node; l_node=l_node->next) {
        string nname = NodeName(l_node);
        if (nname=="imageinfo" || nname=="objectinfo") {
          string label, mapunit;
          float score = 0.0;

          for (xmlNodePtr i_node=l_node->children; i_node;i_node=i_node->next){
            string iname = NodeName(i_node);
            string icont = i_node->children && i_node->children->content ?
              (const char*)i_node->children->content : "";

            if (iname=="label")
              label = icont;
            else if (iname=="mapunit")
              mapunit = icont;
            else if (iname=="score")
              score = atof(icont.c_str());
            else if (iname=="erfdata")
              InterpretXMLerfdata(i_node, label);
          }

          if (label.empty()) {
            ShowError("Query::InterpretXMLobjects() : No image label.");
            goto ENDOFLOOP;
          }

          int idx = LabelIndex(label);

          if (type=='+' || type=='-' || type=='0')
            MarkAsSeenNoAspects(idx, MarkCharToValue(type));

          else {
            select_type t = type=='?' ? select_question:
              type=='%' ? select_map : select_show;
            // obs! this doesn't yet handle aspects via AspectsFromIndices();
            AppendNewObject(new Object(the_db, idx, t, score, mapunit));
          }
        }
      ENDOFLOOP:;
      }
    }
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::InterpretXMLerfdata(const XmlDom& ednode, const string& l) {
    XmlDom sse = ednode.FirstElementChild("speechstring");
    string ss = sse.FirstChildContent();
    erfdata_speechstring[l] = ss;
    // cout << "speechstring: " << l << "=\"" << ss << "\"" << endl;

    sse = ednode.FirstElementChild("tagstring");
    ss = sse.FirstChildContent();
    erfdata_tagstring[l] = ss;
    // cout << "tagstring: " << l << "=\"" << ss << "\"" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *Query::XMLextractFeatureNamesM(const xmlNodePtr list) {
    char val[1000] = "";
    for (xmlNodePtr cnode=list->children; cnode; cnode=cnode->next) {
      int selected = 0;
      char fname[1000];
      for (xmlNodePtr fnode = cnode->children; fnode; fnode=fnode->next) 
        if (!xmlStrcmp(fnode->name, (XMLS)"selected"))
          selected = atoi((const char*)fnode->children->content);
        else if (!xmlStrcmp(fnode->name, (XMLS)"name"))
          strcpy(fname, (const char*)fnode->children->content);

      if (selected) 
        sprintf(val+strlen(val), "%s%s", *val?",":"", fname);
    }
    return CopyString(val);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::InterpretXMLajaxdata(const XmlDom& ajaxdata, bool save) {
    string hdr = "Query::InterpretXMLajaxdata() : ";

    if (ajaxdata.NodeName()!="ajaxdata")
      return ShowError(hdr+"not <ajaxdata>");

    bool stream_found = false;

    for (XmlDom ss = ajaxdata.FirstElementChild(); ss; ss = ss.NextElement()) {
      if (ss.NodeName()=="erf-featurelist")
	for (XmlDom ef = ss.FirstElementChild(); ef; ef = ef.NextElement()) {
	  if (ef.NodeName()!="erf-features")
	    return ShowError(hdr+"not <erf-features>");
	  string label     = ef.Property("label");
	  string method    = ef.Property("method");
	  string relevance = ef.Property("relevance");
	  erfdatafile << "f " << Identity() << " <" << label << "> "
		      << method << "=" << relevance << " : "
		      << ef.FirstChildContent() << endl;
	}

      if (ss.NodeName()=="stream.start") {
	stream_found = true;
        if (!ProcessAjaxRequest(ss, false, save))
          return ShowError(hdr+"ProcessAjaxRequest() failed");
      }

      if (ss.NodeName()=="ajaxdatafile") {
	stream_found = true;
	string fname = ss.FirstChildContent();
	string qname = AjaxSaveFileName(fname);
	if (FileSize(qname)<=0)
	  WriteLog("Skipping empty explicit ajaxdata file <"+qname+">");
	else {
	  WriteLog("Reading explicit ajaxdata file <"+qname+">");
	  XmlDom ad = XmlDom::Parse(qname);
	  bool ok = ProcessAjaxRequest(ad.Root(), false, save);
	  ad.DeleteDoc();

	  if (!ok)
	    return ShowError(hdr+"ProcessAjaxRequest() failed with explicit <"
			     +qname+"> ajaxdata file");
	}
      }
    }

    if (!stream_found)
      for (size_t i=0; i<100000; i++) {
	string qname = AjaxSaveFileName(i, false);
	if (!FileExists(qname))
	  break;
	WriteLog("Reading implicit ajaxdata file <"+qname+">");
	XmlDom ad = XmlDom::Parse(qname);
        bool ok = ProcessAjaxRequest(ad.Root(), false, save);
	ad.DeleteDoc();

	if (!ok)
          return ShowError(hdr+"ProcessAjaxRequest() failed with implicit <"
			   +qname+"> ajaxdata file");
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  timespec_t Query::LastAccessTime(bool recurse) const {
    if (!recurse || !Nchildren())
      return last_access;

    timespec_t t = last_access;
    for (size_t i=0; i<Nchildren(); i++) {
      timespec_t c = Child(i)->LastAccessTime(true);
      if (MoreRecent(c, t))
        t = c;
    }

    return t;
  }

  /////////////////////////////////////////////////////////////////////////////

  timespec_t Query::LastModificationTime(bool recurse) const {
    if (!recurse || !Nchildren())
      return last_modification;

    timespec_t t = last_modification;
    for (size_t i=0; i<Nchildren(); i++) {
      timespec_t c = Child(i)->LastModificationTime(true);
      if (MoreRecent(c, t))
        t = c;
    }

    return t;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConditionallyRecalculate() {
    // obs! HAS_CBIRALGORITHM_NEW problem
    if (!algorithms.empty())
      return true;

    if (HasAnalysis())
      return true;

    if (!NseenObjects())
      return true;

    CreateIndexData(true);

    const int k = 0;

    for (size_t m=0; m<Nindices(); m++)
      for (size_t l=0; l<(size_t)Nlevels(m); l++)
        if (!GetConvolved(m, l, k)) {
          // if (GetMatrixCount(m)!=1)
          //   WarnOnce("Query::ConditionallyRecalculate(): matrixcount!=1");

          const string args = ""; // obs!
          PerMapPicSOM(args, m, l, false);
          WriteLog("Recalculated ", IndexFullName(m), "[", ToStr(l), "]");
        }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DumpObjectList() const {
    for (size_t i=0; i<NnewObjects(); i++)
      if (NewObject(i).Retained()) {
        bool plus = false;
        Query *c = Child(0);
        if (c) {
          for (size_t j=0; j<c->NseenObjects(); j++)
            if (c->SeenObject(j).Match(NewObject(i))) {
              if (c->SeenObject(j).Value()>0)
                plus = true;
              break;
            }
        }
        cout << (plus?"+":" ") << NewObject(i).Label() << endl;
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CheckObjectIdx(const ObjectList& l, 
                             const string& n, bool /*sloppy*/) const {
    // USED TO BE: sloppy = true; // see also StageFinalSelect()
    bool ok = true; 
    for (size_t i=0; i<l.size(); i++)
      for (size_t j=i+1; j<l.size(); j++)
        if (l[i].Index()==l[j].Index())
          ok = ShowError(n+" error C i="+ToStr(i)+" j="+ToStr(j)+ " #",
                         ToStr(l[i].Index())+" <"+l[i].Label()+">");

    bool r = l.check(false);
    if (!r)
      ok = ShowError(n+" ObjectList::check() failed");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CrossCheckSeenObjectIdx() const {
    bool ok = true;
    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& o = seenobject[i];
      if (FindInObjectIndex(o.Index(), uniqueimage))
        ok = ShowError("seenimage crosscheck with uniqueimage : #",
                       ToStr(o.Index()), " <", o.Label(), ">");

      if (FindInObjectIndex(o.Index(), newobject))
        ok = ShowError("seenimage crosscheck with newobject : #",
                       ToStr(o.Index()), " <", o.Label(), ">");
    }
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  const map<string,string>& Query::OriginsInfo(size_t idx,
                                               bool prune) {
    if (!IsRoot())
      return Root()->OriginsInfo(idx, prune);

    string label = Label(idx);

    string msg = "Now in Query::OriginsInfo("+label+","+ToStr(prune)
      +") "+Identity()+ " : ";

    origins_info_t::iterator i = origins_info.find(label);
    if (i!=origins_info.end()) {
      if (DataBase::DebugOrigins())
        cout << msg << "CACHE HIT" << endl;
      return i->second;
    }

    bool prunable = GetDataBase()->IsPrunable(label);
  
    bool w = !prune, p = false, pruned = false;

    map<string,string> hash = GetDataBase()->ReadOriginsInfo(idx, p, w);
    if (hash.empty() && prunable && prune) {
      hash = GetDataBase()->ReadOriginsInfo(idx, true, true);
      pruned = true;
    }

    if (pruned) {
      string old = hash["name"];
      size_t dot = old.find('.');
      if (dot!=string::npos) {
        // string tmp = old.substr(0, dot)+".img";
        string tmp = label+".img";
        hash["name"] = tmp;
      }
      if (GetDataBase()->DebugOrigins())
        cout << msg << endl << "  file=<" << old << "> changed to <"
             << hash["name"] << ">" << endl;
    }

    origins_info[label] = hash;
    i = origins_info.find(label);
  
    if (DataBase::DebugOrigins())
      cout << msg << "cache fill" << endl;

    return i->second;
  }

  /////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

double Query::MaximumEntropy(const simple::FloatMatrix& m) {
  simple::FloatMatrix em(m.Rows(), m.Columns());
  em.Add(1);
  return em.Entropy();
}

///////////////////////////////////////////////////////////////////////////////

double Query::ConvolutionEntropy(const FloatVector *v) {
  if (!v || !v->Length())
    return -1;

  if (v->Length()==1)
    return 0;

  double div = 2*v->Sum()-v->Get(0);
  double sum = 0;
  for (int i=0; i<v->Length(); i++) {
    double p = v->Get(i)/div;
    sum += (i?2:1)*p*::log(p);
  }

  return -2*sum/::log(2.0);
}

///////////////////////////////////////////////////////////////////////////////

const FloatVector *Query::ConvolutionVector(const string& str, int l) {
  static FloatVectorSet vset;

  float len, par;
  string t, vname = ConvolutionVectorName(str, l, t, len, par);
  if (!vname.length())
    return NULL;

  stringstream ss;
  ss << vname << " [" << t << " " << len << " " << par << "]";

#ifdef PICSOM_USE_PTHREADS
#ifdef PTHREAD_RWLOCK_INITIALIZER
  static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
#else
  static bool first=true;
  static pthread_rwlock_t rwlock;
  if (first) 
    pthread_rwlock_init(&rwlock, NULL);
  first=false;
#endif // PTHREAD_RWLOCK_INITIALIZER
  pthread_rwlock_rdlock(&rwlock);
#endif // PICSOM_USE_PTHREADS

  FloatVector *vec = vset.GetByLabel(vname.c_str());

#ifdef PICSOM_USE_PTHREADS
  pthread_rwlock_unlock(&rwlock);
#endif // PICSOM_USE_PTHREADS

  if (vec) {
    // cout << "ConvolutionVector() reused " << ss.str() << endl;
    goto destroy;
  }

#ifdef PICSOM_USE_PTHREADS
  pthread_rwlock_wrlock(&rwlock);
#endif // PICSOM_USE_PTHREADS

  vec = vset.GetByLabel(vname.c_str());
  if (!vec) {
    vec = CreateConvolutionVector(t, len, par);
    if (vec) {
      vec->Label(vname);
      vset.Append(vec);
      ss << " len=" << vec->Length() << (vec->Length() ? " :" : "");
      for (int i=0; i<vec->Length() && i<3; i++)
        ss << " " << vec->Get(i) << (vec->Length()>i+1 ? "," : "");
      if (vec->Length()>3)
        ss << " ...";
      WriteLog("Created convolution vector ", ss.str());
    }
  }

#ifdef PICSOM_USE_PTHREADS
  pthread_rwlock_unlock(&rwlock);
 destroy:
  pthread_rwlock_destroy(&rwlock);
#else 
  destroy:
#endif // PICSOM_USE_PTHREADS

  return vec;
}

///////////////////////////////////////////////////////////////////////////////

bool Query::ConvolutionVectorNameAndFunc(const string& str, string& t,
                                          ConvolutionFunction& f
                                          ) {
  static const struct conv_name_and_func_entry {
    /*const*/ string name;
    ConvolutionFunction func;
  } conv_name_and_func[] = {
    { "point",       PointVectorValue  },
    { "triangle",    TriangleVectorValue  },
    { "oneperrank",  OnePerRankVectorValue },
    { "gaussian",    GaussianVectorValue  },
    { "dog",         DoGVectorValue },
    { "rectangle",   RectangleVectorValue },
    { "exponential", ExponentialVectorValue },
    //{ "cosine",      CosineVectorValue },
    { "hann",        HannVectorValue },
    { "", NULL }
  };
  for (const conv_name_and_func_entry *e = conv_name_and_func; e->func; e++)
    if (e->name == str.substr(0, e->name.length())) {
      t = e->name;
      f = e->func;
      return true;
    }

  t = "";
  f = NULL;
  return false;
}

///////////////////////////////////////////////////////////////////////////////

string Query::ConvolutionVectorName(const string& str, int l,
                                     string& t, float& len, float& par) {
  ConvolutionFunction f;

  if (!ConvolutionVectorNameAndFunc(str, t, f))
    return "";

  const char *ps = str.length()>t.length() ? str.c_str()+t.length()+1 : NULL;

  string param = ConvolutionVectorParam(ps, l, len, par);

  return param.length() ? (t + "-" + param) : t;
}

///////////////////////////////////////////////////////////////////////////////

ConvolutionFunction Query::ConvolutionVectorFunc(const string& str,
                                                         int l, float& len,
                                                         float& par) {
  ConvolutionFunction f;

  string t;
  if (!ConvolutionVectorNameAndFunc(str, t, f))
    return NULL;

  const char *ps = str.length()>t.length() ? str.c_str()+t.length()+1 : NULL;

  ConvolutionVectorParam(ps, l, len, par);

  return f;
}

///////////////////////////////////////////////////////////////////////////////

string Query::ConvolutionVectorParam(const char *ptr, int l,
                                      float& len, float& par) {
  if (ptr) 
    while (l--) {
      const char *dash = strchr(ptr, '-');
      if (dash)
        ptr = dash+1;
      else
        break;
    }

  len = 1;
  par = 0;

  string param = ptr ? string(ptr, strcspn(ptr, "-")) : "";
  float p, q;
  int n = sscanf(param.c_str(), "%f:%f", &p, &q);
  if (n==1)
    len = p;
  else if (n==2) {
    len = q;
    par = p;
  }

  return param;
}

///////////////////////////////////////////////////////////////////////////////

FloatVector *Query::CreateConvolutionVector(const string& str,
                                             double len, double par) {
  string t;
  ConvolutionFunction f;
  if (!ConvolutionVectorNameAndFunc(str, t, f))
    return NULL;

  FloatVector *vec = new FloatVector(1000);
  for (int i=0; i<vec->Length(); i++) {
    double val = (*f)(i, len, par);
    if (val)
      vec->Set(i, val);
    else {
      vec->Lengthen(i);
      break;
    }
  }

  return vec;
}

///////////////////////////////////////////////////////////////////////////////

double Query::PointVectorValue(double i, double, double) {
  return i==0 ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////////////

double Query::TriangleVectorValue(double i, double len, double) {
  return i>len ? 0 : (len-i)/len;
}

///////////////////////////////////////////////////////////////////////////////

double Query::OnePerRankVectorValue(double i, double len, double) {
  return i>len ? 0 : 1/(i+1);
}

///////////////////////////////////////////////////////////////////////////////

double Query::GaussianVectorValue(double i, double len, double par) {
  if (i>len)
    return 0;

  static const double m = sqrt(::log(2.0));
  double v = i*m/par;
 
  return exp(-v*v);
}

///////////////////////////////////////////////////////////////////////////////

double Query::DoGVectorValue(double i, double len, double par) {
  if (i>len)
    return 0;

  static const double m = sqrt(2.0/::log(2.0));
 
  return GaussianVectorValue(i,len,m*par)-GaussianVectorValue(i,len,m*1.6*par)/1.6;
}

///////////////////////////////////////////////////////////////////////////////

double Query::RectangleVectorValue(double i, double len, double) {
  return i>len ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////

double Query::ExponentialVectorValue(double i, double len, double par) {
  return i>len ? 0 : exp(-i*::log(2.0)/par);
}

///////////////////////////////////////////////////////////////////////////////

double Query::HannVectorValue(double i, double len, double) {
  return i>len ? 0 : 0.5*(1+cos(6.28322*i/(2*(len)+1)));
}

  /////////////////////////////////////////////////////////////////////////////

  list<pair<int,float> > Query::DepthFirstSeenObjects() const {
    typedef list<pair<int,float> > list_t;
    list_t ret;
    set<int> idxset;

    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& obj = seenobject[i];
      if (idxset.find(obj.Index())==idxset.end()) {
        ret.push_back(pair<int,float>(obj.Index(), obj.Value()));
        idxset.insert(obj.Index());
      }
    }

    for (size_t i=0; i<Nchildren(); i++) {
      const list_t chret = Child(i)->DepthFirstSeenObjects();
      for (list_t::const_iterator j=chret.begin(); j!=chret.end(); j++)
        if (idxset.find(j->first)==idxset.end()) {
          ret.push_back(*j);
          idxset.insert(j->first);
        } else {
          list_t::iterator p = ret.begin();
          while (p->first!=j->first)
            p++;
          p->second = j->second;
        }
    }
    
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query *Query::DepthFirstRelevantObjects(target_type tt) const {
    // cout << "QQQQQQ " << Identity() << endl; DumpSeen();

    Query *ret = new Query(Picsom());
    ret->CopySome(*this, true, false, false);
    ret->DeleteSeen();
    ret->DeleteNew();

    set<int> idxset;

    if (!Nchildren())
      for (size_t i=0; i<NseenObjects(); i++) {
        const Object& obj = seenobject[i];
        if (idxset.find(obj.Index())==idxset.end() && obj.Retained()
            && obj.SelectType()!=select_show &&
            PicSOM::TargetTypeContains(obj.TargetType(), tt)) {
          ret->AppendNewObject(obj);
          idxset.insert(obj.Index());
        }
      }

    for (size_t i=0; i<Nchildren(); i++) {
      const Query *chret = Child(i)->DepthFirstRelevantObjects(tt);
      for (size_t j=0; j<chret->NnewObjects(); j++) {
        const Object& obj = chret->NewObject(j);

        if (idxset.find(obj.Index())==idxset.end()) {
          ret->AppendNewObject(obj);
          idxset.insert(obj.Index());

        } else
          for (size_t k=0; k<ret->NnewObjects(); k++)
            if (ret->NewObject(k).Index()==obj.Index()) {
              //cout << "ZZZZ found duplicate: " << obj.Index() << " "
              //     << ret->NewObject(k).Value() << " " << obj.Value() << endl;
              if (obj.Value()>ret->NewObject(k).Value())
                ret->NewObject(k).Value(obj.Value());
              break;
            }
      }
      delete chret;
    }
    
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t Query::NnewObjectsRetainedNonShow() const {
    size_t n = 0;
    for (size_t i=0; i<NnewObjects(); i++) {
      const Object& obj = NewObject(i);
      if (obj.Retained() && obj.SelectType()!=select_show)
        n++;
    }
    // cout << "XXX " << Identity() << " " << n << endl;

    return n;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query *Query::FindLargestChild() {
    int j = -1, n = NnewObjectsRetainedNonShow();

    for (size_t i=0; i<Nchildren(); i++) {
      Query *q = Child(i)->FindLargestChild();
      int m = q->NnewObjectsRetainedNonShow();
      if (m>n) {
        n = m;
        j = i;
      }
    }

    Query *q = j==-1 ? this : Child(j)->FindLargestChild();

    //cout << "RRR " << Identity() << " " << q->Identity() << " " << n << endl;

    return q;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query::cls_dist_map_list Query::ClassDistances(const vector_cfier_list& l) {
    string err = "Query::ClassDistances() : ";
    
    bool debug = false;

    cls_dist_map_list ret;

    for (vector_cfier_list::const_iterator i=l.begin(); i!=l.end(); i++) {
      const cox::knn::lvector_t& vvvector = i->vector;
      TSSOM& tssom = *i->tssom;
      TSSOM::classifier_data *c = i->cfier;
      
      if (debug)
        cout << " " << tssom.Name() << "/" << c->name;

      typedef vector<pair<string,double> > cdist_t;
      cdist_t cdist = c->class_distances(vvvector);

      ret.push_back(multimap<double,string>());
      for (cdist_t::const_iterator j=cdist.begin(); j!=cdist.end(); j++)
        ret.back().insert(make_pair(j->second, j->first));
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> Query::Classify(const vector_cfier_list& l) {
    string err = "Query::Classify() : ";

    bool debug = false;

    list<string> ret;

    for (vector_cfier_list::const_iterator i=l.begin(); i!=l.end(); i++) {
      const cox::knn::lvector_t& vector = i->vector;
      TSSOM& tssom = *i->tssom;
      TSSOM::classifier_data *c = i->cfier;
      
      if (debug)
        cout << " " << tssom.Name() << "/" << c->name;

      string cres = c->classify(vector);
      if (debug)
        cout << " [" << vector.label() << "->" << cres << "]";

      ret.push_back(cres);
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::IsInsert() const {
    return analysis && analysis->IsInsert();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::IsCreate() const {
    return analysis && analysis->IsCreate();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::IsAnalyse() const {
    return analysis && !IsInsert() && !IsCreate();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ExpansionByPRF(int nrounds, int nobjects) {
    string msg = "ExpansionByPRF() : ";
    bool ok = true;
    if (debug_prf)
      cout << msg << "Running PRF, nrounds=" << nrounds
           << ", nobjects=" << nobjects << endl;
    int mq_was = MaxQuestions();
    MaxQuestions(nobjects);
    
    for (int r=0; r<nrounds; r++) {
      if (debug_prf)
        cout << msg << "PRF round=" << r << endl    
             << "Calling CbirStages() for PRF" << endl;    
      
      CbirStages();
      
      int length = NnewObjects();
      if (debug_prf)
        cout  << msg << "NnewObjects()=" << length << endl;    
      if (!length)
        ShowError(msg+"NnewObjects() empty");
      
      int skip = 0;
      for (int j=0; j<length; j++) {
        if (NewObject(skip).Retained() &&
            NewObject(skip).SelectType() == select_question) {
          int idx = -1;
          const Object& img = NewObject(skip);
          idx = img.Index();
          if (idx<0) {
            ShowError(msg + "Object::FindIndex() with <",
                      img.Label(), "> failed");
            skip++;
          } else {
            if (debug_prf)
              cout << msg << "Marking " << Label(idx) 
                   << " as seen positive" << endl;
            
            ok = MarkAsSeenEither(idx, +1.0, true);
            if (!ok) {
              ShowError(msg+"MarkAsSeenEither() failed for idx="+ToStr(idx));
              skip++;
            }
          }
          
        } else
          skip++;
      }
      
    }
    MaxQuestions(mq_was);
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxRequest(const XmlDom& xml, bool isdoc, bool save) {
    string msg = "Query::ProcessAjaxRequest(isdoc="+
      string(isdoc?"true":"false")+") : ";
    if (debug_ajax) {
      cout << msg << "called identity=["+Identity() << "] database="
	   << (the_db?the_db->Name():string("NULL")) << endl;
    }

    XmlDom root = isdoc ? xml.Root() : xml;
    if (root.NodeName()!="stream.start") {
      /*return*/ ShowError(msg+"root is <"+root.NodeName()+
			   ">, should be <stream.start>, returning TRUE");
      return true;
    }

    Tic("ProcessAjaxRequest");

    XmlDom erf = root.FirstElementChild();
    if (erf.NodeName()!="erf")
      return ShowError(msg+"<erf> element not found");
    
    string href    = erf.Property("href");
    string type    = erf.Property("type");
    string scheme  = erf.Property("scheme");
    string version = erf.Property("version");
    string query   = erf.Property("query");
    string docID   = erf.Property("docID");

    if (href=="" || type=="" || scheme=="")
      return ShowError(msg+"href, type or scheme attribute not set");

    if (debug_ajax>1) {
      cout << "    href=\"" << href    << "\"" << endl;
      cout << "    type=\"" << type    << "\"" << endl;
      cout << "  scheme=\"" << scheme  << "\"" << endl;
      cout << " version=\"" << version << "\"" << endl;
      cout << "   query=\"" << query   << "\"" << endl;
      cout << "   docID=\"" << docID   << "\"" << endl;
      cout << " stored as ajax data #" << ajax_data_piece_count << endl;
    }

    Tic("ajax_data");

    // ajax_data_pieces.push_back(xml);

    if (AjaxSave()&1)
      AddDataTo_ajaxdata(root, XmlDom());

    if (save) {
      if (AjaxSave()&6) {
	string qname = AjaxSaveFileName(ajax_data_piece_count, false);
	string lname = AjaxSaveFileName(ajax_data_piece_count, true);
      
	if ((AjaxSave()&2)&&qname!="") {
	  Picsom()->MakeDirectory(qname, true);
	  if (xml.Write(qname)) {
	    //WriteLog("Saved ajax data in <"+qname+">");
	    if (!(AjaxSave()&1)) {
	      Ensure_ajaxdata();
	      ajax_data_all.Element("ajaxdatafile", lname);
	    }
	  } else
	    ShowError(msg+"failed to save ajax data in <"+qname+">");
	}

	if ((AjaxSave()&4)&&lname!="") {
	  if (xml.Write(lname))
	    WriteLog("Saved ajax data in <"+lname+">");
	  else
	    ShowError(msg+"failed to save ajax data in <"+lname+">");
	}
      }
    
      ajax_data_piece_count++;
    }

    Tac("ajax_data");

    static int n=0;    
    if (false && ++n >= 10) {
      ajaxresponse.insert(make_pair("gotopage",
                                    "http://www.cis.hut.fi/picsom/"));
      n = 0;
    }

    bool data_found = false, ok = true, first = true;
    Query *erfq = this;
    for (XmlDom data=erf.NextElement(); data&&ok; data = data.NextElement()) {
      if (data.NodeName()!="data")
        return ShowError(msg+"element <"+data.NodeName()
                         +"> found instead of <data>");

      data_found = true;

      if (erfsplitdata) {
        // ConditionallyCalculateErfFeatures() might be called here...
        if (!first)
          erfq = erfq->CreateChild(NULL);
        erfq->ErfImageDataClear();
        erfq->AddDataTo_ajaxdata(root, data);
      }
      first = false;

      for (XmlDom cont = data.FirstElementChild(); cont&&ok;
           cont = cont.NextElement()) {
        string dtype = cont.NodeName();
      
        if (debug_ajax>2)
          cout << "   data=\"" << dtype  << "\"" << endl;

        if (dtype=="pointer") {
          bool last = true;
          XmlDom tmp = data.NextElement();
          if (tmp && tmp.NodeName()=="data") {
            tmp = tmp.FirstElementChild();
            if (tmp && tmp.NodeName()=="pointer") {
              // cout << "LAST=FALSE" << endl;
              last = false;
            }
          }
          ok = erfq->ProcessAjaxDataPointer(cont, last);

        } else if (dtype=="keyboard")
          ok = erfq->ProcessAjaxDataKeyboard(cont);

        else if (dtype=="audio")
          ok = erfq->ProcessAjaxDataAudio(cont);

        else if (dtype=="screen" || dtype=="window" || dtype=="image")
          ok = erfq->ProcessAjaxDataGaze(cont);

        else if (dtype=="objecttag")
          ok = erfq->ProcessAjaxDataObjectTag(cont);
        
        else if (dtype=="objectdetection")
          ok = erfq->ProcessAjaxDataObjectDetection(cont);
        
        else if (dtype=="imagelist")
          ok = erfq->ProcessAjaxImageList(cont);
        
        else
          ok =  ShowError(msg+"data type <"+dtype+"> is unknown");
      }
    }

    if (!ok)
      return ShowError(msg+"processing failed");

    if (!data_found && 0) // let's now think that <erf> alone is sufficient
      return ShowError(msg+"no <data> element was found");

    // this might be inside the previous loop...
    for (Query *q = this; q;) {
      q->ConditionallyCalculateErfFeatures(2);
      if (debug_erf_relevance)
        q->ShowErfRelevances();
      Analysis *a = q->GetParentAnalysis();
      if (a)
        a->StoreErfRelevances(q);

      bool debug = true;
      if (forcecbirstages) {
        if (q->parent)
          q->seenobject = q->parent->seenobject;

        set<string> done;
        list<string> erf = q->ErfImageList();
        for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++) {
          if (*i=="*collage*")
            continue;

          const Query::erf_image_data& e = q->ErfImageData(*i);

          bool marked = false;
          if (a && a->CheckQuery()->HasPositive()) {
            int v = a->PositiveGT()[e.index];
            if (debug)
              cout << "d <" << e.label << "> " << e.index << " " << v << endl;
            q->MarkAsSeenNoAspects(e.index, v);
            q->ProcessAspectInfo("", e.index, v);
            marked = true;
          }

          for (map<string,float>::const_iterator r = e.relevance.begin();
               r!=e.relevance.end(); r++) {
            if (r->first=="gaze-33") {
              float v = ThresholdGazeRelevance(r->second);
              if (debug)
                cout << "g [" << r->first << "] <" << e.label << "> "
                     << e.index << " " << r->second << " " << v << endl;
              if (!marked)
                q->MarkAsSeenNoAspects(e.index, v);
              q->ProcessAspectInfo("gaze-cont", e.index, r->second);
              q->ProcessAspectInfo("gaze",      e.index, v);
              marked = true;
            }
          }
          done.insert(e.label);
        }

        if (a && a->CheckQuery()->HasPositive())
          for (map<string,string>::const_iterator i=collage_position.begin();
               i!=collage_position.end(); i++)
            if (done.find(i->second)==done.end()) {
              size_t idx = LabelIndex(i->second);
              int v = a->PositiveGT()[idx];
              if (debug)
                cout << "c <" << i->second << "> " << idx << " " << v << endl;
              q->MarkAsSeenNoAspects(idx, v);
              q->ProcessAspectInfo("", idx, v);
            }

        q->CbirStages();
      }

      q = erfsplitdata && q->Nchildren() ? q->Child(0) : NULL;
    }

    SetLastModificationTimeNow();

    if (!ajax_gotopage_sent && ErfCheck_gotopage()) {
      Query *q = CreateChildWith_gotopage();
      if (q)
        q->PrepareAfter_gotopage();
      ajax_gotopage_sent = true;
    }

    ErfCheckAndSet_setaspect();
    ErfCheckAndSet_changetext();

    Tac("ProcessAjaxRequest");

    Picsom()->PossiblyShowDebugInformation("ProcessAjaxRequest()");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::Ensure_ajaxdata() {
    if (!ajax_data_all)
      ajax_data_all = Connection::XMLdocroot("ajaxdata", false).second;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddDataTo_ajaxdata(const XmlDom& sstart, const XmlDom& data) {
    if (!erfsplitdata || data)
      Ensure_ajaxdata();

    if (!erfsplitdata) {
      ajax_data_all.AddCopy(sstart);
      return true;
    }

    if (!data)
      return true;

    XmlDom xmlss = ajax_data_all.Element("stream.start");
    XmlDom erf   = sstart.FirstElementChild();
    XmlDom erfc  = xmlss.Element("erf");
    erfc.Prop("href",   erf.Property("href"));
    erfc.Prop("type",   erf.Property("type"));
    erfc.Prop("scheme", erf.Property("scheme"));
    xmlss.AddCopy(data);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxDataPointer(const XmlDom& cont, bool last) {
    string msg = "Query::ProcessAjaxDataPointer() : ";
    if (debug_ajax>2)
      cout << msg+"called" << endl;

    Tic("ProcessAjaxDataPointer");

    XmlDom ele  = cont.FirstElementChild();
    string name = ele.NodeName();
    if (name!="sample" && name!="click")
      return ShowError(msg+"<"+name+"> found instead of <sample> or <click>");

    bool click = name=="click";

    map<string,string> keyval = ParseXMLKeyValues(ele);
    uiart::TimeSpec ft = AjaxTimeSpec(keyval["timestamp"]);

    if (debug_ajax>2) {
      cout << "  " << name << endl;
      for (map<string,string>::const_iterator i=keyval.begin();
	   i!=keyval.end(); i++) {
        cout << "    " << i->first << " = " << i->second;
	if (i->first=="timestamp")
	  cout << " = " << ft.Str();
	cout << endl;
      }
      cout << endl;
    }

    ProcessAjaxImages(ele, ft, 0.0, 0.0, true, false, click, last);

    int xabs = atoi(keyval["xabs"].c_str());
    int yabs = atoi(keyval["yabs"].c_str());
    
    erf_image_data& aid = ErfImageData("*collage*");
    if (click)
      aid.pointer_click.push_back( uiart::GazeData(xabs, yabs, ft, 0.0, 0.0));
    else
      aid.pointer_sample.push_back(uiart::GazeData(xabs, yabs, ft, 0.0, 0.0));

    ProcessXMLcollageData(xabs, yabs, true, click, last);

    Tac("ProcessAjaxDataPointer");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxDataKeyboard(const XmlDom& cont) {
    string msg = "Query:ProcessAjaxDataKeyboard() : ";
    if (debug_ajax>2)
      cout << msg+"called" << endl;

    XmlDom ele = cont.FirstElementChild();
    string name = ele.NodeName();
    if (name!="key")
      return ShowError(msg+"<"+name+"> found instead of <key>");

    map<string,string> keyval = ParseXMLKeyValues(ele);

    if (debug_ajax>2) {
      cout << "  " << name << endl;
      for (map<string,string>::const_iterator i=keyval.begin();
	   i!=keyval.end(); i++)
        cout << "    " << i->first << " = \"" << i->second << "\"" << endl;
      cout << endl;
    }

    string k = keyval["name"], ts = keyval["timestamp"];
    uiart::TimeSpec t = AjaxTimeSpec(ts);
    pair<string,uiart::TimeSpec> kt = make_pair(k, t);
    erf_image_data& col = ErfImageData("*collage*");
    col.keydata.push_back(kt);
    erf_image_data *i = ErfImageDataByTime(t, keysync);
    if (i)
      i->keydata.push_back(kt);
    
    if (erfkeymap!="") {
      string km = ErfKeyMapAlias(erfkeymap);
      vector<string> row = SplitInCommas(km);
      for (size_t r=0; r<row.size(); r++) {
        size_t c = row[r].find(k);
        if (c==string::npos)
          continue;
        string lab = LabelInCollage(c, r);
        col.keyclicks++;
        erf_image_data& aid = ErfImageDataByLabel(lab);
        aid.key = k;
        aid.keyclicks++;
        cout << msg+"key=["+k+"] <" << aid.label
             << "> @" << (void*)&aid << " keyclicks="+ToStr(aid.keyclicks)
             << "/" << ToStr(col.keyclicks) << endl;
      }
    }

    if (k=="" || k==" ")
      return true;

    if (ajax_image_x>=0 && ajax_image_y>=0 && ajax_image_p) {
      erf_image_data& idata = *ajax_image_p;
      if (!idata.cumulative.empty()) {
	imagedata txtimg = imagefile::render_text(k, 24.0);
	txtimg.force_three_channel();
	txtimg.convert(imagedata::pixeldata_uchar);

	imagedata itemp = idata.scaled;
	itemp.copyAsSubimage(txtimg, ajax_image_x-txtimg.width()/2,
			     ajax_image_y-txtimg.height()/2);

	imagedata &ihist = idata.cumulative;
	ihist.copyAsSubimage(txtimg, ajax_image_x-txtimg.width()/2,
			     ajax_image_y-txtimg.height()/2);

        if (AjaxMplayer())
          try {
            imagedata iboth = CombineImages(itemp, ihist);
            ajax_image_mplayer.add_frame(iboth);
          } catch (...) {
            return ShowError(msg+"add_frame() failed");
          }
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& Query::ErfKeyMapAlias(const string& s) {
    static map<string,string> amap;
    if (amap.empty()) {
      amap["q15"] = "qwert,asdfg,zxcvb";
    }
    map<string,string>::const_iterator i = amap.find(s);

    return i==amap.end() ? s : i->second;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxDataAudio(const XmlDom& cont) {
    bool debug_play = false;

    string msg = "Query::ProcessAjaxDataAudio() : ";
    if (debug_ajax>2)
      cout << msg+"called" << endl;

    map<string,string> keyval = ParseXMLKeyValues(cont, true);

    XmlDom ele  = cont.FirstElementChild("waveform");
    if (!ele)
      return ShowError(msg+"<waveform> not found");

    string rate     = ele.Property("rate");
    string format   = ele.Property("format");
    string channels = ele.Property("channels");
    string encoding = ele.Property("encoding");
    string status   = ele.Property("status");

    if (debug_ajax>2) {
      cout << "  timestamp   = " << keyval["timestamp"]   << endl;
      cout << "  duration    = " << keyval["duration"]    << endl;
      cout << "  messagetype = " << keyval["messagetype"] << endl;
      cout << "  urgent      = " << keyval["urgent"]      << endl;

      cout << "  waveform" << endl;
      cout << "    rate      = " << rate     << endl;
      cout << "    format    = " << format   << endl;
      cout << "    channels  = " << channels << endl;
      cout << "    encoding  = " << encoding << endl;
      cout << "    status    = " << status   << endl;
    }
    if (encoding!="base64")
      return ShowError(msg+"encoding!=base64");

    string base64 = ele.FirstChildContent();
    istringstream input(base64);
    ostringstream output;
    b64::decode(input, output);
    string binary = output.str();

    static int count = 0;
    char number[20];
    sprintf(number, "%05d", count++);
    string bname = Identity()+"_audio_"+string(number)+".bin";

    string timestmp = keyval["timestamp"];
    uiart::TimeSpec stime = AjaxTimeSpec(timestmp);
    uiart::TimeSpec etime = stime+atof(keyval["duration"].c_str())/1000.0;

    if (debug_ajax>2) {
      cout << "    base64    = ["
	   << (base64.empty()?"":base64.substr(0, 50)+" ...")
	   << "]" << endl;
      cout << "    binary    = "  << binary.size() << " bytes" << endl;
      cout << "    timestamp = "  << stime.Str()
	   << " -- " << etime.Str() << endl;
      cout << endl;
    }

    if (binary.size()) {
      if (AjaxPlayAudio() || AjaxSaveAudio()) {
	if (StringToFile(binary, bname)) {
	  if (debug_play || debug_ajax>2)
	    WriteLog(msg+"ajax audio data written "+ToStr(binary.size())+
		     " bytes in <"+bname+">");

	  if (AjaxPlayAudio()) {
	    // USAGE: play -r 16000 -2 -s -t raw Q:*.bin
	    string cmd = "( play -r 16000 -2 -s -t raw "+bname;
	    if (!debug_play)
	      cmd += " 1>/dev/null 2>&1";
	    if (!AjaxSaveAudio())
	      cmd += " ; /bin/rm -f "+bname;
	    cmd += " ) &";
	    int r = system(cmd.c_str());
	    if (r==-1)
	      return ShowError(msg+"system() failed");
	  }

	} else
	  return ShowError(msg+"failed to write in <"+bname+">");
      }

      if (speechrecognizer!="") {
	static uiart::TimeSpec prev_etime(0);
	uiart::TimeSpec zero_time(0);
	if (!(prev_etime==zero_time)) {
	  uiart::TimeSpec test = prev_etime + 0.1;
	  if (test<stime) {
	    float ratef = atof(rate.c_str());
	    size_t nsamples = (size_t)floor(ratef*(stime-prev_etime));
	    string zerobuf(2*nsamples, '\0');
	    if (PicSOM::DebugSpeechRecognizer() || debug_ajax>2)
	      cout << msg << "feeding " << ToStr(nsamples) << " zero samples"
		   << endl;
	    Picsom()->FeedSpeechRecognizer(zerobuf, Identity(),
					   prev_etime.Ts(), stime.Ts());
	  }
	}
	Picsom()->FeedSpeechRecognizer(binary, Identity(),
				       stime.Ts(), etime.Ts());
	prev_etime = etime;
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxDataGaze(const XmlDom& cont) {
    string msg = "Query::ProcessAjaxDataGaze() : ";
    if (debug_ajax>2)
      cout << msg+"called" << endl;

    Tic("ProcessAjaxDataGaze");

    // cont may be either <screen>, <window> or <image>
 
    XmlDom gaze  = cont.FindChild("gaze");    
    XmlDom temps = gaze.FirstElementChild();

    using uiart::GazeData;
    erf_image_data& aid = ErfImageData("*collage*");
    vector<GazeData>& collage_l = aid.gaze_sample_left;
    vector<GazeData>& collage_r = aid.gaze_sample_right;
    vector<GazeData>& collage_c = aid.gaze_sample_center;
    vector<GazeData>& gdfixation_l = aid.gaze_fixation_left;
    vector<GazeData>& gdfixation_r = aid.gaze_fixation_right;
    vector<GazeData>& gdfixation_c = aid.gaze_fixation_center;
  
    while (temps) {
      XmlDom samp, fix;

      if (temps.NodeName() == "samples") {
        samp  = temps.FindChild("sample");
        temps = temps.NextElement();

      } else if (temps.NodeName() == "fixations") {
        fix   = temps.FindChild("fixation");
        temps = temps.NextElement();
      }
     
      while (samp) {                
        XmlDom time, eyes, eyel, eyer, eyec;  
        time = samp.FirstElementChild("timestamp");
        eyes = samp.FirstElementChild("eye");
        while (eyes) {
	  string side = eyes.Property("side");

          if (side=="left")
            eyel = eyes;            
          else if (side=="right")
            eyer = eyes;
          else if (side=="center")
            eyec = eyes; 

          eyes = eyes.NextElement("eye"); 
        }  
        string stime = time.FirstChildContent(); 
        //string snum  = num.FirstChildContent();     
        //float fnum   = atof(snum.c_str());    
        uiart::TimeSpec ftime = AjaxTimeSpec(stime);

        map<string,string> keyval_l = ParseXMLKeyValues(eyel, true);
        map<string,string> keyval_r = ParseXMLKeyValues(eyer, true);
        map<string,string> keyval_c = ParseXMLKeyValues(eyec, true);

        float xl_    = atof(keyval_l["x"].c_str());
        float yl_    = atof(keyval_l["y"].c_str());
        float psl    = atof(keyval_l["pupil.size"].c_str());
        //float xl_rel = atof(keyval_l["xrel"].c_str());
        //float yl_rel = atof(keyval_l["yrel"].c_str());
        float xl_abs = atof(keyval_l["xabs"].c_str());
        float yl_abs = atof(keyval_l["yabs"].c_str());
        
        float xr_    = atof(keyval_r["x"].c_str());
        float yr_    = atof(keyval_r["y"].c_str());
        float psr    = atof(keyval_r["pupil.size"].c_str());
        //float xr_rel = atof(keyval_r["xrel"].c_str());
        //float yr_rel = atof(keyval_r["yrel"].c_str());
        float xr_abs = atof(keyval_r["xabs"].c_str());
        float yr_abs = atof(keyval_r["yabs"].c_str());

        float xc_    = atof(keyval_c["x"].c_str());
        float yc_    = atof(keyval_c["y"].c_str());;
        float psc    = atof(keyval_c["pupil.size"].c_str());
        //float xc_rel = atof(keyval_c["xrel"].c_str());
        //float yc_rel = atof(keyval_c["yrel"].c_str());
        float xc_abs = atof(keyval_c["xabs"].c_str());
        float yc_abs = atof(keyval_c["yabs"].c_str());
        
        //float  xc = (xl+xr)/2;
        //float  yc = (yl+yr)/2;
        //float psc = (psl+psr)/2;

        if (debug_ajax>2) { // see outputs
          /*
          cout << " The node is: " << cont.NodeName() << endl;
          cout << " The node is: " << gaze.NodeName() << endl;
          cout << " The node is: " << samps.NodeName() << endl;
          cout << " The node is: " << samp.NodeName() << endl;
          cout << " The node is: " << time.NodeName() << endl;
          //cout << " time = " << ftime.Str() << endl;
          cout << " GMT time = " << stime << endl;
          cout << " The node is: " << num.NodeName() << endl;
          cout << " number = " << fnum << endl;          
          */
          //cout << " The eye =: " << eyel.Property("side") << endl;         
          cout << " GMT time = " << stime << endl;
          cout << " psl = " << psl << " xl = " << xl_ << " yl = " << yl_ 
               << endl;
          //cout << " xrel = " << xl_rel << " yrel = " << yl_rel << endl;
          cout << " xlabs = " << xl_abs << " ylabs = " << yl_abs << endl;
          //cout << " buttons = " << sbuttons_l << endl;
          //cout << endl;
          
          //cout << " The eye =: " << eyer.Property("side") << endl;
          cout << " psr = " << psr << " xr = " << xr_ << " yr = " << yr_
               << endl;
          //cout << " xrel = " << xr_rel << " yrel = " << yr_rel << endl;
          cout << " xrabs = " << xr_abs << " yrabs = " << yr_abs << endl;
          //cout << " buttons = " << sbuttons_r << endl;
          //cout << " The eye =: " << eyec.Property("side") << endl;
          cout << " xc = " << xc_ << " yc = " << yc_ << endl;
          //cout << " xcrel = " << xc_rel << " ycrel = " << yc_rel << endl;
          cout << " xcabs = " << xc_abs << " ycabs = " << yc_abs << endl;
          cout << endl;  
        }

        float xl = xl_abs, yl = yl_abs;
        float xr = xr_abs, yr = yr_abs;
        float xc = xc_abs, yc = yc_abs;

        int limit = -1000;
        bool do_l = xl>=limit && yl>=limit;
        bool do_r = xr>=limit && yr>=limit;
        bool do_c = do_l && do_r;

        if (do_l) {
          ProcessAjaxImages(eyel, ftime, psl, 0.0, false, 0, false, true);
          collage_l.push_back(GazeData(xl, yl, ftime, psl));
          ProcessXMLcollageData((int)xl, (int)yl, false, false, true);
        }

        if (do_r) {
          ProcessAjaxImages(eyer, ftime, psr, 0.0, false, 1, false, true);
          collage_r.push_back(GazeData(xr, yr, ftime, psr));          
        }

        if (do_c) {
          ProcessAjaxImages(eyec, ftime, psc, 0.0, false, 2, false, true);
          //ProcessAjaxImagesCenter(eyec, eyec, ftime, 0, 0, psc, true);
          collage_c.push_back(GazeData(xc, yc, ftime, psc));
        }

        samp = samp.NextElement("sample");
      }
   
      while (fix) {
        XmlDom eyef, fixl, fixr, fixc;
        map<string,string> keyval_f = ParseXMLKeyValues(fix, true);            
        uiart::TimeSpec ftime = AjaxTimeSpec(keyval_f["timestamp"]);
        string stime = keyval_f["timestamp"];
        //float fnum = atof(keyval_f["number"].c_str()); 
        float fdur  = atof(keyval_f["duration"].c_str());       
        float x_    = atof(keyval_f["x"].c_str());
        float y_    = atof(keyval_f["y"].c_str());
      //float x_rel = atof(keyval_f["xrel"].c_str());
      //float y_rel = atof(keyval_f["yrel"].c_str());
        float x_abs = atof(keyval_f["xabs"].c_str());
        float y_abs = atof(keyval_f["yabs"].c_str());
        eyef = fix.FindChild("eye");
        //if (eyef)
          //cout << " The eye =: " << eyef.Property("side") << endl; 
        //else 
          //cout << " The eye =: " << "Center" << endl; 
        
        if (debug_ajax>2) { // see outputs
          /*
          cout << " The node is: " << cont.NodeName() << endl;
          cout << " The node is: " << gaze.NodeName() << endl;
          cout << " The node is: " << fixs.NodeName() << endl;
          cout << " The node is: " << fix.NodeName() << endl;
          cout << " The node is: " << time.NodeName() << endl;
          //cout << " time = " << ftime.Str() << endl;
          */
          cout << " GMT time = " << stime << endl;          
          //cout << " The node is: " << num.NodeName() << endl;
          //cout << " number = " << fnum << endl;
          cout << " duration = " << fdur << endl;
          cout << " x = " << x_ << " y = " << y_ << endl;
          //cout << " xrel = " << xrel << " yrel = " << yrel << endl;
          cout << " xabs = " << x_abs << " yabs = " << y_abs << endl;
          //cout << " buttons = " << sbuttons_f << endl;
          cout << endl;
        }

	float x = x_abs; // used to be like x = x_
	float y = y_abs; // used to be like y = y_
       
	string side = eyef.Property("side");
        if (side =="left") {
          ProcessAjaxImages(fix, ftime, 0.0, fdur, false, 0, true, true);
          ProcessAjaxImages(fix, ftime, 0.0, fdur, false, 2, true, true);
          gdfixation_l.push_back(GazeData(x, y, ftime, 0, fdur));
	  gdfixation_c.push_back(GazeData(x, y, ftime, 0, fdur)); //obs!
          ProcessXMLcollageData((int)x, (int)y, false, true, true);

        } else if (side=="right") {
          ProcessAjaxImages(fix, ftime, 0.0, fdur, false, 1, true, true);
          ProcessAjaxImages(fix, ftime, 0.0, fdur, false, 2, true, true);
          gdfixation_r.push_back(GazeData(x, y, ftime, 0, fdur));
	  gdfixation_c.push_back(GazeData(x, y, ftime, 0, fdur)); //obs!
          ProcessXMLcollageData((int)x, (int)y, false, true, true);

        } else {
	  ProcessAjaxImages(fix, ftime, 0.0, fdur, false, 2, true, true);
	  gdfixation_c.push_back(GazeData(x, y, ftime, 0, fdur));
	  ProcessXMLcollageData((int)x, (int)y, false, true, true);
        }

        fix = fix.NextElement("fixation");
      } 
    }

    // ConditionallyCalculateErfFeatures(aid, 1);

    Tac("ProcessAjaxDataGaze");

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxDataObjectTag(const XmlDom& cont) {
    string msg = "Query::ProcessAjaxDataObjectTag() : ";
    if (debug_ajax>2)
      cout << msg+"called" << endl;

    string name = cont.NodeName();
    if (name!="objecttag")
      return ShowError(msg+"<"+name+"> found instead of <objecttag>");

    map<string,string> keyval = ParseXMLKeyValues(cont, true);
    string timestamp = keyval["timestamp"];
    string object    = keyval["object"];
    string tag       = keyval["tag"];

    if (debug_ajax)
      cout << msg+"object=["+object+"] tag=["+tag+"] timestamp=["
	   << timestamp << "]" << endl;

    uiart::TimeSpec t = AjaxTimeSpec(timestamp);
    pair<string,uiart::TimeSpec> kt = make_pair(tag, t);
    erf_image_data& aid = ErfImageDataByLabel(object);
    aid.tagdata.push_back(kt);
    ErfImageData("*collage*").tagdata.push_back(kt);    

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxDataObjectDetection(const XmlDom& cont) {
    string msg = "Query::ProcessAjaxDataObjectDetection() : ";
    if (debug_ajax>2)
      cout << msg+"called" << endl;

    string name = cont.NodeName();
    if (name!="objectdetection")
      return ShowError(msg+"<"+name+"> found instead of <objectdetection>");

    bool first = true;

    map<string,string> keyval = ParseXMLKeyValues(cont, true);
    string timestamp = keyval["timestamp"];
    for (size_t i=0;; i++) {
      XmlDom o = cont.FindChild("object", i);
      if (!o)
	break;

      map<string,string> pm = o.PropertyMap(), pmcopy = pm;
      string toc      = pm["toc"];
      string name     = pm["name"];
      string image    = pm["image"];
      string polygon  = pm["polygon"];
      string width    = pm["width"];
      string height   = pm["height"];
      string gazex    = pm["gazex"];
      string gazey    = pm["gazey"];
    
      if (debug_ajax)
	cout << msg+"timestamp=["+timestamp+"] toc=["+toc+
	  "] name=["+name+"] image=["+image+"] polygon=["+polygon+
	  "] width=["+width+"] height=["+height+
	  "] gazex=["+gazex+"] gazey=["+gazey+"]" << endl;

      pmcopy["timestamp"] = timestamp;
      uiart::TimeSpec uats = AjaxTimeSpec(timestamp);
      timespec_t ts = uats.Ts();

      erf_image_data& eid_image = ErfImageData(image);
      eid_image.label = image;
      eid_image.detections.push_back(pmcopy);

      erf_image_data& eid_toc = ErfImageData(toc);
      if (first)
	eid_toc.detections.clear();
      eid_toc.label = toc;
      eid_toc.detections.push_back(pmcopy);
      first = false;

      if (image=="small-16-Pursula.jpg")
	image = "poster.jpeg";

      if (seen_objects.find(image)==seen_objects.end()) {
	seen_object_t so(image, ts, ts);
	seen_objects[image] = so;
      }
      seen_object_t& so = seen_objects.find(image)->second;
      so.end = ts;

      map<string,list<pair<vector<float>,string> > > & word_box
	= GetDataBase()->OdWordBox(toc);    

      float gx = -1, gy = -1;
      if (gazex!="") {
	if (IsTimeZero(so.gaze_first))
	  so.gaze_first = ts;
	so.gaze_last = ts;

	gx = atof(gazex.c_str());
	gy = atof(gazey.c_str());
	if (image=="poster.jpeg") {
	  gx *= 2380.0/768;
	  gy *= 3368.0/1024;
	}
      }

      if (gx>=0 && word_box.find(image)!=word_box.end()) {
	const list<pair<vector<float>,string > >& l = word_box[image];
	for (auto a=l.begin(); a!=l.end(); a++)
	  if (gx>=a->first[0] && gx<=a->first[2] &&
	      gy>=a->first[1] && gy<=a->first[3]) {
	    seen_word_t sw(a->second, ts, ts, image, gx, gy);
	    seen_words.push_back(sw);
	    cout << endl << endl << endl
		 << ">>>>>>>>>>>>>>>>>>>> " << sw.str() << " <<<<<<<<<<<"
		 << endl << endl << endl;
	  }
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConditionallyCalculateErfFeatures(int s) {
    if (s!=erf_features_when)
      return true;

    const erf_image_data& col = ErfImageData("*collage*");
    double gaze_inv_freq = DetectGazeSamplingPeriod(col);

    list<string> erf = ErfImageList();
    for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++) {
      Ensure_ajaxdata();
      XmlDom xml = ajax_data_all.Element("erf-featurelist");
      erf_image_data& id = ErfImageData(*i);
      ConditionallyCalculateErfFeatures(id, col, s, xml, gaze_inv_freq);
    }

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ErfShowDataAndRecurse() {
    bool show_empty = true;
    bool show_bogus = false;

    list<string> il = ErfImageList(), ll = ErfImageListSrcs();
    set<string> m, l(ll.begin(), ll.end());

    for (list<string>::const_iterator i=il.begin(); i!=il.end(); i++) {
      erf_image_data& d = ErfImageData(*i);

      if (!show_bogus && !FindInNew(d.index))
	continue;

      for (map<string,vector<float> >::const_iterator f=d.features.begin();
	   f!=d.features.end(); f++) {
	map<string,float>::const_iterator ri = d.relevance.find(f->first);
	float r = ri!=d.relevance.end() ? ri->second : -1;
	erfdatafile << "s " << Identity() << " <" << *i << "> "
		    << f->first << "=" << r << " :";
	for (size_t j=0; j<f->second.size(); j++)
	  erfdatafile << " " << f->second[j];
        erfdatafile << endl;

	m.insert(f->first);
	l.erase(*i);
      }
    }

    if (show_empty) {
      map<string,size_t> vlen;
      vlen["ptrs-1"]  = 1;
      vlen["ptrc-1"]  = 1;
      vlen["gaze-33"] = 33;

      for (set<string>::const_iterator i=l.begin(); i!=l.end(); i++) {
	//if (!show_bogus && !FindInNew(LabelIndex(*i)))
	//  continue;

	for (set<string>::const_iterator n=m.begin(); n!=m.end(); n++) {
	  erfdatafile << "s " << Identity() << " <" << *i << "> "
		      << *n << "=0 :";
	  for (size_t j=0; j<vlen[*n]; j++)
	    erfdatafile << " 0";
	  erfdatafile << endl;
	} 
      } 
    }

    for (size_t c=0; c<Nchildren(); c++)
      Child(c)->ErfShowDataAndRecurse();

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  const Query::erf_image_data
  *Query::ErfImageDataFixation(const uiart::TimeSpec& t) const {
    bool debug = debug_erf_features;

    if (debug)
      cout << "ErfImageDataFixation(" << t.Str() << ")" << endl;

    for (map<string,erf_image_data>::const_iterator
           i=erf_image_data_cache.begin(); i!=erf_image_data_cache.end(); i++){
      if (debug)
        cout << "  <" << i->first << "> <" << i->second.label << "> ("
             << i->second.position.collage_position.first << ","
             << i->second.position.collage_position.second << ")" << endl;
      if (i->first!="*collage*")
        for (vector<uiart::GazeData>::const_iterator
               j=i->second.gaze_fixation_center.begin();
               j!=i->second.gaze_fixation_center.end(); j++) {
          if (debug)
            cout << "    " << j->start_time.Str() << " " << (j->start_time==t)
                 << endl;
          if (j->start_time==t) {
            if (debug)
              cout << "    HIT" << endl;
            return &i->second;
          }
        }
    }

    if (debug)
      cout << "    MISS" << endl;

    return NULL;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ErfImageDataExists(const string& l, bool islabel) const {
    string s = l;

    if (islabel) {
      map<string,string>::const_iterator i = erf_image_lab2src.find(l);
      if (i!=erf_image_lab2src.end())
	s = i->second;
    }

    return erf_image_data_cache.find(s)!=erf_image_data_cache.end();
  }
    
  /////////////////////////////////////////////////////////////////////////////

  Query::erf_image_data& Query::ErfImageData(const string& l) {
    bool debug = debug_erf_features;

    if (erf_image_data_cache.find(l)==erf_image_data_cache.end()) {
      erf_image_data& ed = erf_image_data_cache[l];
      ed.url = l;
      ed.db = GetDataBase();
      string lab;
      if (ed.db) {
        lab = HTMLimgURLtoLabel(l);
        if (ed.db->IsLabel(lab)) {
          ed.index = GetDataBase()->LabelIndex(lab);
          if (ed.index!=-1)
            ed.label = lab;
        }
      }
      if (debug)
        cout << "ErfImageData() : added \"" << l << "\" -> <" << lab
             << "> db=" << (ed.db?ed.db->Name():string("NULL"))
	     << " index=" << ed.index << endl;
    }
    return erf_image_data_cache[l];
  }
  
  /////////////////////////////////////////////////////////////////////////////

  list<string> Query::ErfImageListLabels() const {
    list<string> ret;

    for (map<string,string>::const_iterator i=erf_image_lab2src.begin();
	 i!=erf_image_lab2src.end(); i++)
      ret.push_back(i->first);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> Query::ErfImageListSrcs() const {
    list<string> ret;

    for (map<string,string>::const_iterator i=erf_image_lab2src.begin();
	 i!=erf_image_lab2src.end(); i++)
      ret.push_back(i->second);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query::erf_image_data& Query::ErfImageDataByLabel(const string& l) {
    return ErfImageData(erf_image_lab2src[l]);

    /*
    for (map<string,erf_image_data>::iterator
           i=erf_image_data_cache.begin(); i!=erf_image_data_cache.end(); i++)
      if (i->second.label==l)
        return i->second;

    ShowError("Query::ErfImageDataByLabel("+l+") failing");

    static erf_image_data dummy;
    return dummy;
    */
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ConditionallyCalculateErfFeatures(erf_image_data& aid,
                                                const erf_image_data& col,
                                                int s, XmlDom& xml,
						double gaze_inv_freq) {
    if (s!=erf_features_when)
      return true;

    if (aid.url=="*collage*")
      return true;

    if (!GetDataBase())
      return true;

    const string collage_spec = aid.position.collage_spec();
    bool collage_spec_empty = collage_spec=="(0,0)/0x0";
    if (collage_spec_empty) {// we should study when these occur...
      if (DebugErfFeatures())
        cout << "Skipping erf feature extraction for <" << aid.url
             << "> due to collage_spec==\"(0,0)/0x0\"" << endl;

      // return true;
    }

    if (!col.gaze_sample_left.empty() || !col.gaze_fixation_center.empty()) {
      if (collage_spec_empty)
        return true;

      //int eye = 0;
      //if (!col.gaze_sample_center.empty())
      //eye = 2;

      // Jussi's suggestion begins
      int eye = -1;
      if (!aid.gaze_sample_left.empty())
	eye = 0;
      if (!aid.gaze_sample_right.empty())
	eye = 1;
      if (!aid.gaze_sample_center.empty())
	eye = 2;
      if (eye == -1) {
	// ShowError("ConditionallyCalculateErfFeatures() : eye index error");
	eye = 1;
      } 
      eye = 2;
      // Jussi's suggestion ends
      //eye = 0; // hard coded for testing by He Zhang Apr.10,2010
      const string fid = "gaze-33";
      
      //SolveGazeCenter(aid);
      
      int fix = 2;
      vector<float> v = aid.features[fid]
	= CalculateGazeFeatures(aid, col, eye, fix, gaze_inv_freq);

      // Nov. 2009 by He Zhang: write feature vectors into file features.txt
      /*FILE *file = fopen("features.txt", "a");
	if(file==NULL) {
	printf("He Zhang: An error has occurred and no such file exits!\n");
	return 1;
	}
	for (vector<float>::const_iterator i=v.begin(); i!=v.end(); i++) {
	//cout << *i <<" ";
	fprintf(file, "%f ", *i);    
	}
	fprintf(file, "\n");
	fclose(file);*/
      float r = default_gaze_relevance;
    
      if (!v.empty()) // !aid.gaze_sample_center.empty()
	r = GetDataBase()->GazeRelevance(v, fid);
    
      aid.relevance[fid] = r;

      if (debug_erf_relevance>1)
	cout << endl << "Estimated gaze relevance of <" << aid.label
	     << "> = " << r << endl << endl;

      stringstream fval;
      for (size_t i=0; i<v.size(); i++)
	fval << (i?" ":"") << v[i];

      ErfStoreFeatures(xml, fval.str(), aid.label, fid, r);
    }

    if (!col.pointer_sample.empty() || !col.pointer_click.empty()) {
      if (collage_spec_empty)
        return true;

      const string pfid = "ptrs-1";
      vector<float> v = aid.features[pfid] = CalculatePointerFeatures(aid,col);
      float r = aid.relevance[pfid] = GetDataBase()->PointerRelevance(v, pfid);

      if (debug_erf_relevance>1)
        cout << endl << "Estimated pointer sample relevance of <" << aid.label
             << "> = " << r << endl << endl;

      stringstream fval;
      for (size_t i=0; i<v.size(); i++)
        fval << (i?" ":"") << v[i];

      ErfStoreFeatures(xml, fval.str(), aid.label, pfid, r);
    }

    if (!col.pointer_click.empty() || col.keyclicks) {
      // we will have collage_spec_empty==true for images without
      // pointer/gaze data ie. those that have only keyclicks.

      const string pfid = "ptrc-1";
      size_t tot = aid.pointer_click.size()+aid.keyclicks;
      
      float r = aid.relevance[pfid] = tot%2 ? 1.0 : 0.0;
      aid.features[pfid] = vector<float>(1, float(tot));

      if (debug_erf_relevance>1)
        cout << endl << "Estimated pointer click relevance of <" << aid.label
             << "> @" << (void*)&aid << " = " << r << " clicks="
             <<  aid.pointer_click.size()
             << " keyclicks=" << aid.keyclicks << "/" << col.keyclicks
             << endl << endl;

      ErfStoreFeatures(xml, ToStr(tot), aid.label, pfid, r);
    }

    return true;
  }
    
  /////////////////////////////////////////////////////////////////////////////
  
  bool Query::ErfStoreFeatures(XmlDom& xml, const string& v,
			       const string& lab, const string& m,
			       float r) {
    
    XmlDom ef = xml.Element("erf-features", v);
    ef.Prop("label",     lab);
    ef.Prop("method",    m);
    ef.Prop("relevance", ToStr(r));

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  vector<float> Query::FeatureVector(const string& method,
				     const string& label, bool recurse) const {

    map<string,string>::const_iterator i = erf_image_lab2src.find(label);
    if (i!=erf_image_lab2src.end()) {
      const erf_image_data& d = ErfImageData(i->second);
      map<string,vector<float> >::const_iterator j = d.features.find(method);
      if (j!=d.features.end())
	return j->second;
    }
    
    return recurse&&Parent() ? Parent()->FeatureVector(method, label, recurse)
      : vector<float>();
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  float Query::ThresholdGazeRelevance(float in) const {
    return in>=0.25 ? 1 : -1;
  }
  
  /////////////////////////////////////////////////////////////////////////////
    
  bool Query::SolveGazeCenter(erf_image_data& aid) {
   
    string msg = "SolveGazeCenter() : ";
    
    using uiart::GazeData;
   
    float xl, yl, psl;
    float xr, yr, psr;
    float xc, yc, psc;

    vector<GazeData>& gdsample_l = aid.gaze_sample_left;
    vector<GazeData>& gdsample_r = aid.gaze_sample_right;
    vector<GazeData>& gdsample_c = aid.gaze_sample_center;

    for (size_t i=0, j=0; i<gdsample_l.size()&&j<gdsample_r.size();) { 
      if (gdsample_l[i].start_time==gdsample_r[j].start_time) {
        xl = gdsample_l[i].x;
        yl = gdsample_l[i].y;
        psl = gdsample_l[i].pupil_size;

        xr = gdsample_r[j].x;
        yr = gdsample_r[j].y;
        psr = gdsample_r[j].pupil_size;

        xc = (xl+xr)/2;
        yc = (yl+yr)/2;
        psc = (psl+psr)/2;

        uiart::TimeSpec ftime_l = gdsample_l[i].start_time;
        gdsample_c.push_back(GazeData(xc, yc, ftime_l, psc));

        i++;
        j++;

        continue;
      }

      if (gdsample_l[i].start_time>gdsample_r[j].start_time) {
        j++;
        continue;
      }

      if (gdsample_l[i].start_time<gdsample_r[j].start_time) {
        i++;
        continue;
      }
    }

    if (debug_ajax>2) { // show debuggings
      cout << msg << "label= " << aid.label << " "
           << " #gaze_sample_center.size() = " << aid.gaze_sample_center.size() 
           << endl << endl; 
    
    for (size_t i=0; i<aid.gaze_sample_center.size(); i++)
      cout << aid.label << " center " << i << " " << gdsample_c[i].Str()
           << endl;
    }

    cout << endl;
   
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<float> Query::CalculateGazeFeatures(const erf_image_data& aid,
                                             const erf_image_data& col,
					     int e, int f,
					     double SamplingFrequency) {
    using uiart::GazeData;

    string msg = "CalculateGazeFeatures() : ";

    const vector<GazeData>& gdsample    = aid.coorddata(false, e, false);
    const vector<GazeData>& gdfixation  = aid.coorddata(false, f, true);
    const vector<GazeData>& colfixation = col.coorddata(false, f, true); 
    
    bool normalize = false;

    const string collage_spec = aid.position.collage_spec();
    
    string label;

    DataBase *db = GetDataBase();

    if (debug_erf_features) {
      cout << msg;
      if (aid.db)
        cout << "database=" << aid.db->Name() << " ";
      if (aid.label!="")
        cout << "label=" << (label=aid.label) << " index=" << aid.index << " ";
      else
        cout << "url=" << (label=aid.url);
      
      cout << " in collage " << collage_spec << endl;
    
      cout << " #gaze_sample_left="     << aid.gaze_sample_left.size() 
           << " #gaze_sample_right="    << aid.gaze_sample_right.size() 
           << " #gaze_sample_center="   << aid.gaze_sample_center.size() 
           << " #gaze_fixation_center=" << aid.gaze_fixation_center.size()
           << " #gaze_fixation_left="   << aid.gaze_fixation_left.size()
           << " #gaze_fixation_right="  << aid.gaze_fixation_right.size()  
           << " #pointer_sample="       << aid.pointer_sample.size() 
           << " #pointer_click="        << aid.pointer_click.size() 
           << endl;

      cout << " shown location=" << aid.position.top << ","
           << aid.position.left << " shown size=" << aid.position.width << "x"
           << aid.position.height << " original size=" << aid.original_width
           << "x" << aid.original_height << endl;    

      cout << " collage contains " << col.gaze_sample_left.size()
           << " left eye samples, " << col.gaze_sample_right.size()
           << " right eye samples, " << col.gaze_sample_center.size()
           << " center eye samples, " << col.gaze_fixation_center.size()
           << " fixations, " << col.gaze_fixation_left.size() 
           << " left fixations, " << col.gaze_fixation_right.size()
           << " right fixations, " << col.pointer_sample.size()
           << " pointer samples and " << col.pointer_click.size()
           << " pointer clicks" << endl << endl;
   
      for (size_t i=0; i<gdfixation.size(); i++)
        cout << label << " fixation " << i << " " << gdfixation[i].Str()
             << endl;
      cout << endl;
    }

    for (size_t i=0; i < gdsample.size(); i++) {
      int fix = -1;

      for (size_t j=0; j < gdfixation.size(); j++)
        if (gdfixation[j].IsInsideFixation(gdsample[i].start_time))
          fix = j;

      if (debug_erf_features) {
        cout << label << " sample " << i << " " << gdsample[i].Str();
        if (fix!=-1)
          cout << " fix=" << fix;
        cout << endl;
      }
    }

    vector<float> ret(33);

    /*** calculate raw data features (16 features, eye=left) ***/
    
    float& numMeasurements    = ret[0]; 
    float& numOutsideFix      = ret[1];
    float& ratioInsideOutside = ret[2];
    float& xSpread            = ret[3];
    float& ySpread            = ret[4];
    float& elongation         = ret[5];
    float& speed              = ret[6];
    float& coverage           = ret[7];
    float& normCoverage       = ret[8];
    float& landX              = ret[9];
    float& landY              = ret[10];
    float& exitX              = ret[11];
    float& exitY              = ret[12];
    float& pupil              = ret[13];
    float& nJumps1            = ret[14];
    float& nJumps2            = ret[15];
    
    //float SamplingFrequency = DetectGazeSamplingPeriod(col);
    //float SamplingFrequency = 20; // == 50 Hz 
    //float SamplingFrequency = 8.3; // == 120 Hz 

    float numInsideFix = 0.0;
    float xMax = 0.0, xMin = 0.0, yMax = 0.0, yMin = 0.0;
    float eucliDist = 0.0, dx = 0.0, dy = 0.0;
    float x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    float grids[4][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    float width = 0.0, height = 0.0;
    double nbreaksR = 0.0; 
    
    if (gdsample.size()) {
    
      // numMeasurements 
      for (size_t i=0; i < gdsample.size(); i++) {       
        for (size_t j=0; j < gdfixation.size(); j++)
          if (gdfixation[j].IsInsideFixation(gdsample[i].start_time)) {
            numInsideFix += 1;
            break;      
          }
      }

      numMeasurements = gdsample.size()*SamplingFrequency;

      // numOutsideFix
      numInsideFix = numInsideFix*SamplingFrequency;
      numOutsideFix = numMeasurements-numInsideFix;
    
      // ratioInsideOutside
      if (numOutsideFix != 0)
        ratioInsideOutside = numInsideFix/numOutsideFix;  
      else 
        ratioInsideOutside = numInsideFix;
  
      // xSpread, ySpread
      for (size_t i=0; i< gdsample.size(); i++) {
        if (i==0) {
          xMax = xMin = gdsample[0].x;    
          yMax = yMin = gdsample[0].y;
        }

        if (gdsample[i].x > xMax)
          xMax = gdsample[i].x;

        if (gdsample[i].x < xMin)
          xMin = gdsample[i].x;
      
        if (gdsample[i].y > yMax)
          yMax = gdsample[i].y;

        if (gdsample[i].y < yMin)
          yMin = gdsample[i].y;

        if (i < gdsample.size()-1) { 
          dx = (gdsample[i+1].x-gdsample[i].x);
          dy = (gdsample[i+1].y-gdsample[i].y);
          eucliDist += sqrt(dx*dx+dy*dy);          
        }
      }
    
      xSpread = xMax-xMin+1;
      ySpread = yMax-yMin+1;
    
      // elongation
      elongation = ySpread/xSpread;
    
      // speed  
      if (gdsample.size() > 1)
        speed = eucliDist/(gdsample.size()-1);
   
      // coverage 
      width = aid.position.width;
      height = aid.position.height;
    
      for (size_t k=0; k < gdsample.size(); k++)   
        for (size_t i = 0; i < 4; i++)
          for (size_t j = 0; j < 4; j++) {     
            x1 = gdsample[k].x -j*(width/4);
            x2 = gdsample[k].x -(j+1)*(width/4);
            y1 = gdsample[k].y-i*(height/4);
            y2 = gdsample[k].y-(i+1)*(height/4);
            
            if (x1 >= 0 && x2 < 0 && y1 >= 0 && y2 < 0)
              grids[i][j] += 1;          
          }
   
      for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++) 
          if (grids[i][j] != 0)
            coverage += 1;
    
      // normCoverage
      if (numMeasurements != 0)
        normCoverage = coverage/numMeasurements;
      else 
        normCoverage = coverage;

      // landX, landY, exitX, exitY  
      if (!gdsample.empty()) {     
        landX = gdsample.front().x;
        landY = gdsample.front().y; 
        exitX = gdsample.back().x;
        exitY = gdsample.back().y;    
       
        if (normalize) {
          string normLx = "landX", normspecLx = normLx+collage_spec;
          string normLy = "landY", normspecLy = normLy+collage_spec;
          string normEx = "exitX", normspecEx = normEx+collage_spec;
          string normEy = "exitY", normspecEy = normEy+collage_spec;
          
          landX = db->NormalizeGazeCoordInCollage(landX, normspecLx);
          landY = db->NormalizeGazeCoordInCollage(landY, normspecLy); 
          exitX = db->NormalizeGazeCoordInCollage(exitX, normspecEx); 
          exitY = db->NormalizeGazeCoordInCollage(exitY, normspecEy);
        }
      }     
    
      // pupil
      for (size_t i=0; i<gdsample.size(); i++) 
        if (i==0 || gdsample[i].pupil_size > pupil)
          pupil = gdsample[i].pupil_size;
  
      // nJumps1, nJumps2 
      for (size_t i=1; i< gdsample.size(); i++) {
        nbreaksR = gdsample[i].start_time-gdsample[i-1].start_time;
         
        if (nbreaksR > 0.06)
          nJumps1 += 1;
       
        if (nbreaksR > 0.6)
          nJumps2 += 1;
      }
      
      if (normalize) {
        string normJp1 = "nJumps1", normspecJp1 = normJp1+collage_spec;
        string normJp2 = "nJumps2", normspecJp2 = normJp2+collage_spec;
        nJumps1 = db->NormalizeGazeCoordInCollage(nJumps1, normspecJp1);
        nJumps2 = db->NormalizeGazeCoordInCollage(nJumps2, normspecJp2);
      } 
    }

    /*** calculate fixation features (17 features, eye=left) ***/
    
    float& numFix        = ret[16]; 
    float& meanFixLen    = ret[17];
    float& totalFixLen   = ret[18];
    float& fixPrct       = ret[19];
    float& nJumpsFix     = ret[20];
    float& maxAngle      = ret[21];
    float& landXFix      = ret[22];
    float& landYFix      = ret[23];
    float& exitXFix      = ret[24];
    float& exitYFix      = ret[25];
    float& xSpreadFix    = ret[26];
    float& ySpreadFix    = ret[27];
    float& elongationFix = ret[28];
    float& firstFixLen   = ret[29];
    float& firstFixNum   = ret[30];
    float& distPrev      = ret[31];
    float& durPrev       = ret[32];
    
    double nbreaksF = 0.0;
    float  daF = 0.0, dbF = 0.0, dcF = 0.0, angle_2saccades = 0.0;
    float  xMaxFix = 0.0, xMinFix = 0.0, yMaxFix = 0.0, yMinFix = 0.0;
      
    // numFix    
    numFix = gdfixation.size();
     
    // totalFixLen
    for (size_t i = 0; i < numFix; i++)
      totalFixLen += gdfixation[i].fixation_time; 
 
    // meanFixLen
    if (gdfixation.size()) 
      meanFixLen = totalFixLen/numFix;
    else 
      meanFixLen = totalFixLen;
      
    // fixPrct
    if (numMeasurements != 0)
      fixPrct = totalFixLen/numMeasurements;
    else
      fixPrct = totalFixLen;

    // nJumpsFix
    for (size_t i=1; i< numFix; i++) {
      nbreaksF = gdfixation[i].start_time-gdfixation[i-1].start_time;
      nbreaksF = nbreaksF-gdfixation[i-1].fixation_time/1000;
     
      if (nbreaksF > 0.02)
        nJumpsFix += 1;
    } 
    
    if (normalize) {
      string normJpFix = "nJumpsFix", normspecJpFix = normJpFix+collage_spec;
      nJumpsFix = db->NormalizeGazeCoordInCollage(nJumpsFix, normspecJpFix);
    }

    // maxAngle (using cosine formula of a triangle)
    for (size_t i=1; i< numFix-1; i++) {
      daF = (gdfixation[i].x-gdfixation[i-1].x)*(gdfixation[i].x-gdfixation[i-1].x)
           +(gdfixation[i].y-gdfixation[i-1].y)*(gdfixation[i].y-gdfixation[i-1].y);
      dbF = (gdfixation[i+1].x-gdfixation[i].x)*(gdfixation[i+1].x-gdfixation[i].x)
           +(gdfixation[i+1].y-gdfixation[i].y)*(gdfixation[i+1].y-gdfixation[i].y);
      dcF = (gdfixation[i+1].x-gdfixation[i-1].x)*(gdfixation[i+1].x-gdfixation[i-1].x)
           +(gdfixation[i+1].y-gdfixation[i-1].y)*(gdfixation[i+1].y-gdfixation[i-1].y); 

      angle_2saccades = 3.1416-acos((daF+dbF-dcF)/(2*sqrt(daF)*sqrt(dbF)));
   
      if (angle_2saccades > maxAngle)
        maxAngle = angle_2saccades;                  
    }
           
    // landXFix, landYFix, exitXFix, exitYFix   
    if (numFix != 0) { 
      landXFix = gdfixation.front().x;
      landYFix = gdfixation.front().y; 
      exitXFix = gdfixation.back().x;
      exitYFix = gdfixation.back().y;    
    }
    
    if (normalize) {
      string normLxFix = "landXFix", normspecLxFix = normLxFix+collage_spec;
      string normLyFix = "landYFix", normspecLyFix = normLyFix+collage_spec;
      string normExFix = "exitXFix", normspecExFix = normExFix+collage_spec;
      string normEyFix = "exitYFix", normspecEyFix = normEyFix+collage_spec;
      
      landXFix = db->NormalizeGazeCoordInCollage(landXFix, normspecLxFix);
      landYFix = db->NormalizeGazeCoordInCollage(landYFix, normspecLyFix); 
      exitXFix = db->NormalizeGazeCoordInCollage(exitXFix, normspecExFix); 
      exitYFix = db->NormalizeGazeCoordInCollage(exitYFix, normspecEyFix);
    }

    // xSpreadFix, ySpreadFix, elongationFix
    if (numFix == 1) {
      xSpreadFix = 1;
      ySpreadFix = 1;
      elongationFix = 1;
    }else { 
      for (size_t i=0; i< numFix; i++) {
        if (i==0) {
          xMaxFix = xMinFix = gdfixation[0].x;    
          yMaxFix = yMinFix = gdfixation[0].y;
        }

        if (gdfixation[i].x > xMaxFix)
          xMaxFix = gdfixation[i].x;

        if (gdfixation[i].x < xMinFix)
          xMinFix = gdfixation[i].x;
      
        if (gdfixation[i].y > yMaxFix)
          yMaxFix = gdfixation[i].y;

        if (gdfixation[i].y < yMinFix)
          yMinFix = gdfixation[i].y;
      } 
    
      xSpreadFix = xMaxFix-xMinFix;
      ySpreadFix = yMaxFix-yMinFix;
    
      if (xSpreadFix != 0)
        elongationFix = ySpreadFix/xSpreadFix;
      else 
        elongationFix = ySpreadFix;
    }
    // firstFixLen 
    if (numFix != 0)
      firstFixLen = gdfixation.front().fixation_time;
      
    // firstFixNum
    firstFixNum = 0;
    if (numFix && gdsample.size()) {
      uiart::TimeSpec first_visit_start = gdsample[0].start_time;
      uiart::TimeSpec first_visit_end = first_visit_start;
      for (size_t i=1; i<gdsample.size(); i++)
        if (double(gdsample[i].start_time-gdsample[i-1].start_time)<0.030)
          first_visit_end = gdsample[i].start_time;
        else
          break;
      for (size_t i=0; i<gdfixation.size(); i++)
        if (gdfixation[i].start_time>=first_visit_start &&
            gdfixation[i].start_time<=first_visit_end)
          firstFixNum++;
        else
          break;
    }

    // distPrev, durPrev 
    if (numFix != 0)
      for (size_t i=1; i<colfixation.size(); i++) 
        if (colfixation[i].start_time==gdfixation.front().start_time) {
          //float dxF = (colfixation[i-1].x-colfixation[i].x);
          //float dyF = (colfixation[i-1].y-colfixation[i].y);

          bool found = false;
          for (size_t p=i-1; !found; p--) {
            const erf_image_data *prev
              = ErfImageDataFixation(colfixation[p].start_time);
            if (prev) {
              float dxF = float(prev->position.collage_position.first)-
                aid.position.collage_position.first;
              float dyF = float(prev->position.collage_position.second)-
                aid.position.collage_position.second;
              distPrev = sqrt(dxF*dxF+dyF*dyF);
              durPrev = colfixation[p].fixation_time;
              if (false)
                cout << "distPrev found i=" << i << " p=" << p << " "
                     << aid.label << " ("
                     << aid.position.collage_position.first << ","
                     << aid.position.collage_position.second << ") "
                     << prev->label << " ("
                     << prev->position.collage_position.first << ","
                     << prev->position.collage_position.second << ") "
                     << endl;
              found = true;
            }
            if (!p)
              break;
          }
          if (false && !found)
            cout << "distPrev not found" << endl;
            
          break;
        }
    
    float lognumMeasurements = numMeasurements ? log(numMeasurements) : 0.0;

    // debuggings
    if (debug_erf_features) {
      if (e == 0)
        cout << endl << "Left eye: finished examining image: " << label << endl; 
      if (e == 1)
        cout << endl << "Right eye: finished examining image: " << label << endl;
      if (e == 2)
        cout << endl << "Center eye: finished examining image: " << label << endl;  
      
      cout << "*** 16 raw data features ***" << endl << endl;  
      cout << "1 numMeasurements:(ms): " << numMeasurements
           << " (log=" << lognumMeasurements << ")" << endl;
      cout << "2 numOutsideFix: " << numOutsideFix << endl;
      cout << "3 ratioInsideOutside: " << ratioInsideOutside << endl;
      cout << "4 xSpread: " << xSpread
           << " (xMax=" << xMax << " xMin="<< xMin << ")" << endl;
      cout << "5 ySpread: " << ySpread
           << " (yMax=" << yMax << " yMin=" << yMin << ")" << endl;
      cout << "6 elongation: " << elongation << endl;
      cout << "7 speed: " << speed << endl;
      cout << "8 coverage: " << coverage << endl; 
      cout << "9 normCoverage: " << normCoverage << endl;
      cout << "10 (norm) landX: " << landX << endl;  
      cout << "11 (norm) landY: " << landY << endl;
      cout << "12 (norm) exitX: " << exitX << endl;
      cout << "13 (norm) exitY: " << exitY << endl;
      cout << "14 pupil: " << pupil << endl;
      cout << "15 (norm) nJumps1: " << nJumps1 << endl;
      cout << "16 (norm) nJumps2: " << nJumps2 << endl << endl;

      cout << "*** 17 fixation features ***" << endl << endl;
      cout << "17 numFix: " << numFix << endl;
      cout << "18 meanFixLen: " << meanFixLen << endl;
      cout << "19 totalFixLen: " << totalFixLen << endl;
      cout << "20 fixPrct: " << fixPrct << endl;
      cout << "21 (norm) nJumpsFix: " << nJumpsFix << endl;
      cout << "22 maxAngle: " << maxAngle << endl;
      cout << "23 (norm) landXFix: " << landXFix << endl;  
      cout << "24 (norm) landYFix: " << landYFix << endl;
      cout << "25 (norm) exitXFix: " << exitXFix << endl;
      cout << "26 (norm) exitYFix: " << exitYFix << endl;
      cout << "27 xSpreadFix: " << xSpreadFix
           << " (xMaxFix=" << xMaxFix << " xMinFix="<< xMinFix << ")" << endl;
      cout << "28 ySpreadFix: " << ySpreadFix
           << " (yMaxFix=" << yMaxFix << " yMinFix=" << yMinFix << ")" << endl;
      cout << "29 elongationFix: " << elongationFix << endl;
      cout << "30 firstFixLen: " << firstFixLen << endl;
      cout << "31 firstFixNum: " << firstFixNum << endl;
      cout << "32 distPrev: " << distPrev << endl;
      cout << "33 durPrev: " << durPrev << endl;
      cout << endl;
    }

    numMeasurements = lognumMeasurements;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  double Query::DetectGazeSamplingPeriod(const erf_image_data& col) {
    double period = 20; // 20 ms == 50 Hz   OR   8.3 ms == 120 Hz

    using uiart::GazeData;

    string msg = "DetectGazeSamplingPeriod : ";
  
    const vector<GazeData>& colsample = col.gaze_sample_center;

    double threshold = (20+8.3)/2;
    size_t nbelow = 0, nabove = 0;
    for (size_t i=1; i<colsample.size(); i++) {
      double d = colsample[i].start_time-colsample[i-1].start_time;
      if (false)
	cout << "  "  << colsample[i-1].start_time.Str()
	     << " - " << colsample[i].start_time.Str()
	     << " = " << d << endl;
      if (d)
	(1000*d>threshold ? nabove : nbelow)++;
    }

    if (nbelow>nabove)
      period = 8.3;

    // NOTE: due to possible duplicate, one should first check
    // collage's consecutive start_times to remove duplicates, so as
    // to calculate the real sampling frequency.
    //
    // period = 1000*(colsample.back().start_time-colsample.front().start_time)
    //               /(colsample.size()-1);

    if (debug_erf_features) {
      cout << endl << msg << endl;
      cout << " Collage: col.sample_size=" << colsample.size();
      if (colsample.size()>1) 
	cout << " col.start_time=" << colsample.front().start_time
	     << " col.end_time=" << colsample.back().start_time
	     << " threshold=" << threshold
	     << " nabove=" << nabove << " nbelow=" << nbelow;
      cout << " period=" << period << " ms" << endl;
    }
 
    return period;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<float> Query::CalculatePointerFeatures(const erf_image_data& aid,
                                                const erf_image_data& /*col*/) {
    using uiart::GazeData;

    string msg = "CalculatePointerFeatures() : ";

    vector<float> v;
    v.push_back(aid.pointer_sample.size());

    return v;
  }    
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ShowErfRelevances() const {
    list<string> erf = ErfImageList();
    if (erf.size()<2) {
      // cout << "!!!!!!!!!!!!!!!!! size()<2" << endl;
      return false;
    }

    string lab;
    for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++)
      if (*i!="*collage*") {
        lab = *i;
        break;
      }
    if (lab=="") {
      // cout << "!!!!!!!!!!!!!!!!! no lab" << endl;
      return false;
    }

    const erf_image_data& d = ErfImageData(lab);
    size_t w = d.position.collage_size.first;
    size_t h = d.position.collage_size.second;

    /*
    cout << "lab=" << lab << " colpos="
	 << d.position.collage_position.first << ","
	 << d.position.collage_position.second << endl;
    */

    const erf_image_data& col = ErfImageData("*collage*");
    // size_t w = col.position.collage_size.first;
    // size_t h = col.position.collage_size.second;

    if (!w || !h) {
      // cout << "!!!!!!!!!!!!!!!!! !w || !h" << endl;
      return false;
    }

    string colspeech = col.speechstring(speechrecognizer);

    string q = "|";
    size_t ll = GetDataBase()->LabelLength();
    size_t cw = 18+ll, nw = 7, vw = cw-nw;

    cout << endl;
    cout << Identity() << " keys=["
	 << StringTailDotted(col.keystring(), 20)
	 << "] tags={"
	 << StringTailDotted(col.tagstring(), 30)
	 << "} speech=\""
	 << StringTailDotted(colspeech, 40)
	 << "\"" << endl;

    for (size_t y=0; y<h; y++) {
      // typedef map<string,string> ss_t;
      // ss_t ss;
      // ss["gaze"]    = "";
      // ss["ptr"]     = "";
      // ss["gaze-33"] = "";
      // ss["ptrs-1"]  = "";
      // ss["ptrc-1"]  = "";
      typedef list<pair<string,string> > ss_t;
      ss_t ss;
      ss.push_back(make_pair("keys",    ""));
      ss.push_back(make_pair("tags",    ""));
      ss.push_back(make_pair("speech",  ""));
      ss.push_back(make_pair("key",     ""));
      ss.push_back(make_pair("gaze",    ""));
      ss.push_back(make_pair("ptr",     ""));
      ss.push_back(make_pair("gaze-33", ""));
      ss.push_back(make_pair("ptrs-1",  ""));
      ss.push_back(make_pair("ptrc-1",  ""));

      for (size_t x=0; x<w; x++) {
        bool found = false;
        for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++) {
          if (*i=="*collage*")
            continue;

          const erf_image_data& e = ErfImageData(*i);
          if (e.position.collage_position.first!=x ||
              e.position.collage_position.second!=y)
            continue;

          bool data = e.pointer_sample.size()+e.pointer_click.size()+
            e.gaze_sample_left.size()+e.gaze_sample_center.size()+
            e.gaze_sample_right.size()+e.gaze_fixation_center.size();

	  string labeltxt = e.label;
	  if (labeltxt.size()>15)
	    labeltxt.erase(15);
	  labeltxt = "<"+labeltxt+">";
	  labeltxt += string(17-labeltxt.size(), ' ');

          cout << q+labeltxt+StrFieldL(cw-17, data?"":" no data");

          char tmp[1000];
          for (ss_t::iterator j=ss.begin(); j!=ss.end(); j++) {
            if (j->first=="keys") {
	      string keystr = e.keystring();
	      j->second += q+"["+StringTailDotted(keystr, cw-2)+"]";
	      continue;
	    }
            if (j->first=="tags") {
	      string tagstr = e.tagstring();
	      j->second += q+"{"+StringTailDotted(tagstr, cw-2)+"}";
	      continue;
	    }
            if (j->first=="speech") {
	      string speechstr = e.speechstring(speechrecognizer);
	      j->second += q+"\""+StringTailDotted(speechstr, cw-2)+"\"";
	      continue;
	    }
            if (j->first=="gaze") {
              sprintf(tmp, " %2d/%2d/%2d +%2d/%2d/%2d",
		      (int)e.gaze_sample_left.size(),
		      (int)e.gaze_sample_center.size(),
                      (int)e.gaze_sample_right.size(),
                      (int)e.gaze_fixation_left.size(),
                      (int)e.gaze_fixation_center.size(),
                      (int)e.gaze_fixation_right.size());
              j->second += q+StrFieldR(nw, j->first)+StrFieldL(vw, tmp);
              continue;
            }
            if (j->first=="ptr") {
              sprintf(tmp, " %3d +%2d", (int)e.pointer_sample.size(),
                      (int)e.pointer_click.size());
              j->second += q+StrFieldR(nw, j->first)+StrFieldL(vw, tmp);
              continue;
            }
            if (j->first=="key") {
              sprintf(tmp, " [%s]%3d", e.key.c_str(), (int)e.keyclicks);
              j->second += q+StrFieldR(nw, j->first)+StrFieldL(vw, tmp);
              continue;
            }
            
            map<string,float>::const_iterator r = e.relevance.find(j->first);
            if (r!=e.relevance.end()) {
              sprintf(tmp, "=%09.7f", r->second);
              j->second += q+StrFieldR(nw, j->first)+StrFieldL(vw, tmp);

            } else
              j->second += q+StrFieldL(cw);
          }

          found = true;
          break;
        }

        if (!found) {
          string lab = LabelInCollage(x, y);
          if (lab=="")
            lab = string(ll, '?');
          cout << q+"<"+lab+">"+StrFieldL(cw-ll-2, " no data");

          for (ss_t::iterator j=ss.begin(); j!=ss.end(); j++)
            j->second += q+StrFieldL(cw);
        }

      }
      cout << endl;
      for (ss_t::iterator j=ss.begin(); j!=ss.end(); j++)
        if (j->second.find_first_not_of(q+" ")!=string::npos)
          cout << j->second << endl;
      cout << endl;
    }
    cout << "-----------------------------------------------------------------"
         << endl;

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  string Query::LabelInCollage(size_t x, size_t y) const {
    stringstream ss;
    ss << x << " " << y;
    string xy = ss.str();

    map<string,string>::const_iterator i = collage_position.find(xy);

    string ret = i==collage_position.end() ? "" : i->second;

    // cout << "Query::LabelInCollage(" << x << "," << y
    //      << ") = " << ret << endl;

    return ret;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  string Query::erf_image_data::keystring() const {
    string s;
    for (textdata_t::const_iterator i=keydata.begin(); i!=keydata.end(); i++)
      s += i->first;

    return s;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  string Query::erf_image_data::tagstring() const {
    string s;
    for (textdata_t::const_iterator i=tagdata.begin(); i!=tagdata.end(); i++)
      s += (i==tagdata.begin()?"":"+")+i->first;

    return s;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  string Query::erf_image_data::speechstring(const string& languser) const {
    string lang = languser;
    size_t p = lang.find('/');
    if (p!=string::npos)
      lang.erase(p);

    bool clean = true;
    string s;

    for (textdata_t::const_iterator i=speechdata.begin();
	 i!=speechdata.end(); i++)
      if (!clean)
	s += (s!=""?" ":"")+i->first;
      else {
	string t = i->first;
	size_t p = t.find("<s>");
	if (p!=string::npos)
	  t.erase(p, 3);
	p = t.find("</s>");
	if (p!=string::npos)
	  t.erase(p, 4);
	if (lang=="fin") {
	  p = t.find("<w>");
	  if (p!=string::npos)
	    t.replace(p, 3, " ");
	} else if (lang=="eng") {
	  if (s!="")
	    t = " "+t;
	}
	s += t;
      }

    return s;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessSpeechRecognizerResult(const string& s,
					    const uiart::TimeSpec& t) {
    pair<string,uiart::TimeSpec> st = make_pair(s, t);
    erf_image_data& col = ErfImageData("*collage*");
    col.speechdata.push_back(st);    
    erf_image_data *i = ErfImageDataByTime(t, speechsync);
    if (i)
      i->speechdata.push_back(st);
    
    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  Query::erf_image_data *Query::ErfImageDataByTime(const uiart::TimeSpec& t,
						   const string& s) {

    Query::erf_image_data *ret = NULL;

    double mind = 10; // 10 seconds

    for (map<string,erf_image_data>::iterator i=erf_image_data_cache.begin();
	 i!=erf_image_data_cache.end(); i++)
      if (i->first!="*collage*") {
	uiart::TimeSpec dummy(0, 0);
	double d = i->second.minimum_time_distance(t, s, dummy);
	if (d<mind) {
	  mind = d;
	  ret = &i->second;
	}
      }
    
    if (PicSOM::DebugSpeechRecognizer()) {
      cout << "ErfImageDataByTime(" << t.Str() << ") : ";
      if (!ret)
	cout << "returning NULL";
      else {
	uiart::TimeSpec dummy(0, 0);
	ret->minimum_time_distance(t, s, dummy);
	cout << "returning <" << ret->label << "> with mind=" << mind
	     << " @ " << dummy.Str();
      }
      cout << endl;
    }

    return ret;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  double 
  Query::erf_image_data::minimum_time_distance(const uiart::TimeSpec& t,
					       const string& s,
					       uiart::TimeSpec& w) const {
    if (s=="pointer")
      return minimum_time_distance(t, pointer_sample, w);

    double mind = numeric_limits<double>::max();

    if (s=="gaze" || s=="gaze-left") {
      uiart::TimeSpec x(0, 0);
      double d = minimum_time_distance(t, gaze_sample_left, x);
      if (d<mind) {
	mind = d;
	w = x;
      }
    }
    if (s=="gaze" || s=="gaze-right") {
      uiart::TimeSpec x(0, 0);
      double d = minimum_time_distance(t, gaze_sample_right, x);
      if (d<mind) {
	mind = d;
	w = x;
      }
    }
    if (s=="gaze" || s=="gaze-center") {
      uiart::TimeSpec x(0, 0);
      double d = minimum_time_distance(t, gaze_sample_center, x);
      if (d<mind) {
	mind = d;
	w = x;
      }
    }

    return mind;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  double
  Query::erf_image_data::minimum_time_distance(const uiart::TimeSpec& t,
					       const vector<uiart::GazeData>&
					       v, uiart::TimeSpec& w) const {
    double mind = numeric_limits<double>::max();
    for (size_t i=0; i<v.size(); i++) {
      double d = v[i].start_time-t;
      d = fabs(d);
      if (d<mind) {
	mind = d;
	w = v[i].start_time;
      }
    }

    return mind;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ErfCheckAndSet_setaspect() {
    const list<string>& erf = ErfImageList();

    bool ret = false;
    if (ErfPolicy().find("toggle")!=string::npos)
      for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++)
        if (*i!="*collage*") {
          const erf_image_data& e = ErfImageData(*i);
          if (!IsInNewObjects(e.index))
            continue;

          map<string,float>::const_iterator r = e.relevance.find("ptrc-1");
          if (r!=e.relevance.end()) {
            ajaxresponse.insert(make_pair("setaspect", e.label+" \"\" "+
                                          ToStr(r->second)));
            ret = true;
          }
        }

    return ret;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ErfCheckAndSet_changetext() {
    bool sk = ErfPolicy().find("showkeys")!=string::npos;
    bool st = ErfPolicy().find("showtags")!=string::npos;
    bool sp = ErfPolicy().find("showspeech")!=string::npos;

    if (!sk && !st && !sp)
      return false;

    const list<string>& erf = ErfImageList();

    bool ret = false;
    for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++)
      if (*i!="*collage*") {
	const erf_image_data& e = ErfImageData(*i);
	if (!IsInNewObjects(e.index))
	  continue;
	
	string txt;

	if (sk) {
	  string ss = e.keystring();
	  if (ss!="") {
	    txt += "["+ss+"]";
	    // txt += ss;
	  }
	}

	if (st) {
	  string ss = e.tagstring();
	  if (ss!="") {
	    //txt += "{"+ss+"}";
	    txt += ss;
	  }
	}

	if (sp) {
	  string ss = e.speechstring(speechrecognizer);
	  //txt += "\""+ss+"\"";
	  txt += ss;
	}

	if (txt!="") {
	  size_t p;
	  while ((p = txt.find(' '))!=string::npos)
	    txt[p] = '+';

	  // truncation to the last word:
	  p = txt.rfind('+');
	  if (p!=string::npos)
	    txt.erase(0, p);

	  ajaxresponse.insert(make_pair("changetext", e.label+" "+txt));
	  ret = true;
	}
      }

    return ret;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ErfCheck_gotopage() {
    if (ErfPolicy().find("space")!=string::npos) {
      const erf_image_data& col = ErfImageData("*collage*");
      if (col.keystring().find(' ')!=string::npos)
        return true;
    }

    const list<string>& erf = ErfImageList();

    if (ErfPolicy().find("1click")!=string::npos)
      for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++)
        if (*i!="*collage*") {
          const erf_image_data& e = ErfImageData(*i);
          if (!IsInNewObjects(e.index))
            continue;

          map<string,float>::const_iterator r = e.relevance.find("ptrc-1");
          if (r!=e.relevance.end() && r->second==1.0)
            return true;
        }

    if (ErfPolicy().find("samples")!=string::npos)
      for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++)
        if (*i!="*collage*") {
          const erf_image_data& e = ErfImageData(*i);
          if (!IsInNewObjects(e.index))
            continue;
          
          map<string,float>::const_iterator r = e.relevance.find("ptrs-1");
          if (r!=e.relevance.end() && r->second>0.7)
            return true;
        }

    if (ErfPolicy().find("20s")!=string::npos &&
        !MoreRecent(StartTime(), 20))
      return true;

    return false;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  Query *Query::CreateChildWith_gotopage() {
    if (MaxRounds() && Round()>=(int)MaxRounds()) {
      if (NeedsSave()) {
	Query *p = this;
	while (p->parent)
	  p = p->parent;

        string sn = p->SavePath(false);
        if (sn!="")
          Save(sn, false);
      }
      ajaxresponse.insert(make_pair("gotopage", EndPageUrl()));
      return NULL;
    }

    Query *q = CreateChild(NULL);
    string url = Picsom()->HttpListenConnection()+"/query/"+q->Identity()+"/";
    ajaxresponse.insert(make_pair("gotopage", url));

    return q;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::PrepareAfter_gotopage() {
    MoveErfRelevancesToAspects();
    MarkAllNewObjectsAsSeenAspect(-1.0, "");
    ExecuteRunPartThree();

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::MoveErfRelevancesToAspects() {
    bool oldAr = false;
    bool debug = DebugAspects()||DebugErfRelevance()>1;

    const list<string>& erf = ErfImageList();

    if (debug)
      cout << "MoveErfRelevancesToAspects() coll.size()="
           << collage_position.size() << " erf.size()=" << erf.size()
           << " oldAr=" << oldAr << endl;

    for (list<string>::const_iterator i=erf.begin(); i!=erf.end(); i++) {
      if (*i=="*collage*")
        continue;

      const erf_image_data& e = ErfImageData(*i);

      if (debug)
        cout << "  #" << e.index << " <" << e.label << ">" << endl;
      
      if (IsInNewObjects(e.index)) {
        float unset = numeric_limits<float>::max(), no_aspect = unset;
        map<string,float> a;

	a["ptrc-1"]    =  0.0;
	a["ptrs-1"]    =  0.0;
	a["gaze-cont"] =  0.0;
	a["gaze"]      = -1;

        for (map<string,float>::const_iterator r = e.relevance.begin();
             r!=e.relevance.end(); r++) {
          if (debug)
            cout << "    [" << r->first << "]=" << r->second;

	  // until 2010-12-10 A rule required ptrc-1==+1.0 obs!
	  //
	  bool a_rule_match = r->first=="ptrc-1";
	  if (oldAr && a_rule_match)
	    a_rule_match = r->second==1.0;

          if (a_rule_match) {
	    float v = r->second==1.0 ? 1.0 : -1.0;
	    if (debug)
              cout << " A rule " << r->second << " -> " << v;
            no_aspect   = v;
            a["ptrc-1"] = v;
            a[""]       = v;
            
          } else {
            if (debug)
              cout << " B rule";
            if (no_aspect==unset)
              no_aspect = -1.0;
            a[r->first] = r->second;

            if (r->first=="gaze-33") {
              if (debug)
                cout << " Bg rule";
              a["gaze"] = ThresholdGazeRelevance(r->second);
              a["gaze-cont"] = r->second;
            }

            if (erfptrstogaze && r->first=="ptrs-1") {
              if (debug)
                cout << " C rule";
              a["gaze"] = ThresholdGazeRelevance(r->second);
              a["gaze-cont"] = r->second;
            }
          }

          if (debug)
            cout << endl;
        }

        if (no_aspect!=unset) {
          if (debug)
            cout << "    no_aspect=" << no_aspect << endl;
          MarkAsSeenNoAspects(e.index, no_aspect);
        }          
        for (map<string,float>::const_iterator i=a.begin(); i!=a.end(); i++) {
          if (debug)
            cout << "    aspect \"" << i->first << "\"=" << i->second << endl;
          ProcessAspectInfo(i->first, e.index, i->second);
        }
      }
    }

    size_t nobjidx = 0;
    while (NnewObjects()>nobjidx) {
      if (!NewObject(nobjidx).Retained()) {
        nobjidx++;
        continue;
      }

      const string& lab = NewObject(nobjidx).Label();
      size_t idx = LabelIndex(lab);
      if (debug) {
        cout << "  #" << idx << " <" << lab << "> D rule" << endl;
        cout << "    aspect \"\"=-1"         << endl;
        cout << "    aspect \"gaze\"=-1"     << endl;
        cout << "    aspect \"gaze-cont\"=0" << endl;
      }
      MarkAsSeenNoAspects(idx, -1);
      if (!oldAr)
	ProcessAspectInfo("",          idx, -1); // added 2010-12-10
      ProcessAspectInfo(  "gaze",      idx, -1);
      ProcessAspectInfo(  "gaze-cont", idx, default_gaze_relevance);
    }

    ErfImageDataClear();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  uiart::TimeSpec Query::AjaxTimeSpec(const string& t) {
    if (t.find_first_not_of("0123456789")==string::npos) {
      // milliseconds
      long ms = atol(t.c_str());
      long s  = ms/1000;
      long ns = (ms%1000)*1000000;

      return uiart::TimeSpec(s, ns);
    }

    long s = 0, ns = 0;
    tm time;
    memset(&time, 0, sizeof time);
    const char *p = strptime(t.c_str(), "%a, %d %b %Y %H:%M:%S", &time);
    s = mktime(&time);

    if (*p=='.') {
      string part = p+1;
      size_t e = part.find_first_not_of("0123456789");
      if (e!=string::npos)
        part.erase(e);
      part += "000000000";
      part.erase(9);
      ns = atol(part.c_str());
    } else
      cout << "Query::AjaxTimeSpec() : [" << t << "]->[" << p << "]" << endl;

    return uiart::TimeSpec(s, ns);
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxImageList(const XmlDom& cont) {
    string msg = "Query::ProcessAjaxImageList() : ";
    if (debug_ajax>2)
      cout << msg+"called "+Identity() << endl;

    Tic("ProcessAjaxImageList");

    ajax_collage_xml.DeleteDoc();
    ajax_collage_xml = AjaxImageListToCollage(cont);
    if (!ajax_collage_xml.DocOK())
      return ShowError(msg+"AjaxImageListToCollage() failed");

    if (AjaxMplayer()) {
      ajax_collage_image = DataBase::CreateImageCollage(ajax_collage_xml,
                                                        GetDataBase(),
                                                        this);
      size_t w = ajax_collage_image.width();
      size_t h = ajax_collage_image.height();
      scalinginfo sca(w, h, w/2, h/2);
      ajax_collage_image.rescale(sca);
      if (debug_ajax>2)
        cout << msg+"created collage of size " << ajax_collage_image.width()
             << "x" << ajax_collage_image.height() << endl;

      ajax_collage_image_cumulative = ajax_collage_image;

      // imagefile::display(ajax_collage_image);
    }

    Tac("ProcessAjaxImageList");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  XmlDom Query::AjaxImageListToCollage(const XmlDom& imglist) {
    string msg = "AjaxImageListToCollage() : ";
    
    bool allow_zero = false;

    collage_ranges_x.clear();
    collage_ranges_y.clear();
    collage_position.clear();
    erf_image_lab2src.clear();

    //cout << endl << endl << msg << "cleared collage_ranges" << endl << endl;

    XmlDom empty;
    
    if (false && !GetDataBase()) {
      ShowError(msg+"database not set");
      return empty;
    }

    if (imglist.NodeName()!="imagelist") {
      ShowError(msg+"not <imagelist>");
      return empty;
    }

    int dw = imglist.PropertyInt("document_width");
    int dh = imglist.PropertyInt("document_height");
    int vw = imglist.PropertyInt("viewport_width");
    int vh = imglist.PropertyInt("viewport_height");
    int vt = imglist.PropertyInt("viewport_top");
    int vl = imglist.PropertyInt("viewport_left");

    int cw = dw>vw ? dw : vw;
    int ch = dh>vh ? dh : vh;

    XmlDom xml = XmlDom::Doc();
    XmlDom col = xml.Root("collage");
    XmlDom fst = col.Element("frameset");
    XmlDom fra = fst.Element("frame");

    fst.Prop("width",  cw);
    fst.Prop("height", ch);

    size_t n_tot = 0, n_use = 0;
    for (XmlDom e=imglist.FirstElementChild(); e; e=e.NextElement()) {
      string type = e.NodeName();
      if (type!="img" && type!="imagine.thumb") {
        xml.DeleteDoc();
        ShowError(msg+"not <img> nor <imagine.thumb>");
        return empty;
      }

      n_tot++;
      string src = e.Property("src");

      if (SkipXMLimage(type, src)) {
        if (debug_ajax)
          cout << msg << "skipping collage image type=" << type
               << " src=" << src << endl;
        continue;
      }
      n_use++;

      string top = e.Property("top");
      string lef = e.Property("left");
      string wid = e.Property("width");
      string hei = e.Property("height");

      string lab = HTMLimgURLtoLabel(src);

      int tlx = atoi(lef.c_str()), tly = atoi(top.c_str());
      int wth = atoi(wid.c_str()), hht = atoi(hei.c_str());

      if (!tlx && !tly && !wth && !hht)
        continue;

      if (!wth && !hht)
        continue;

      if (type=="imagine.thumb") {
        string db = "voc2007-current";
        if (!SetDataBase(db))
          ShowError(msg+"SetDataBase("+db+") failed");

        erf_image_data& aid = ErfImageData(src);
        if (aid.original.empty())
          ErfInitializeImageData(aid, aid.label, AjaxMplayer());

        XMLimgSquareCorrection(aid, true, tlx, tly, wth, hht);
      }

      if (!allow_zero && (!wth || !hht)) {
        ShowError(msg+"zero width or height, label=<"+lab+"> src=\""+
                  src+"\"");
        if (!wth)
          wth = 1;
        if (!hht)
          hht = 1;
      }

      int brx = tlx+wth-1, bry = tly+hht-1;

      stringstream ss;
      ss << tlx << " " << tly << " " << brx << " " << bry;
      string box = ss.str();

      XmlDom img = fra.Element("image");
      img.Prop("label",    GetDataBase() ? lab : src);
      img.Prop("location", box);

      SolveRanges(collage_ranges_x, tlx, brx);
      SolveRanges(collage_ranges_y, tly, bry);

      erf_image_lab2src[lab] = src;

      if (debug_ajax>2)
        cout << "  added collage image type=" << type << " src=" << src
             << " label=" << lab << " location=" << box << endl;

      if (true || GetDataBase()) { // is not yet set though should?
        stringstream ss;
        ss << lef << " " << top;
        collage_position[lab] = ss.str();
      }
    }

    map<string,string> collage_position_tmp;
    for (map<string,string>::const_iterator i=collage_position.begin();
         i!=collage_position.end(); i++) {
      int x = -1, y = -1;
      sscanf(i->second.c_str(), "%d%d", &x, &y);
      pair<size_t,size_t> kl = SolveCollagePosition(x, y);
      
      stringstream ss;
      ss << kl.first << " " << kl.second;
      string xystr = ss.str();

      collage_position_tmp[xystr] = i->first;

      // cout << msg << i->first << "->" << i->second << " (" << x << "," << y
      //      << ") => " << xystr << "->" << i->first
      //      << endl;
    }
    collage_position = collage_position_tmp;

    if (debug_ajax)
      cout << Identity() << ": d_width=" << dw
           << " d_height=" << dh
           << " vp_width=" << vw
           << " vp_height=" << vh
           << " vp_top=" << vt
           << " vp_left=" << vl
           << " #images=" << n_use << "/" << n_tot
           << " " << collage_ranges_x.size() << "x" << collage_ranges_y.size()
           << endl;

    return xml;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Query::HTMLimgURLtoLabel(const string& src) const {
    string lab = src;

    size_t p = lab.find("&imagename="); // celum userscenario
    if (p!=string::npos) {
      lab.erase(0, p+11);
      p = lab.find('&');
      if (p!=string::npos)
	lab.erase(p);
      return lab;
    }

    p = lab.rfind('/');
    if (p!=string::npos)
      lab.erase(0, p+1);
    p = lab.rfind('?');
    if (p!=string::npos)
      lab.erase(p);
    p = lab.rfind('.');
    if (p!=string::npos)
      lab.erase(p);

    return lab;
  }

  /////////////////////////////////////////////////////////////////////////////

  map<string,string> Query::ParseXMLKeyValues(const XmlDom& cont,
                                              bool allow) const {
    string msg = "ParseXMLKeyValues() : ";

    map<string,string> keyval, empty;

    for (XmlDom ele = cont.FirstElementChild(); ele; ele = ele.NextElement()) {
      string key = ele.NodeName();
      string val = ele.FirstChildContent();

      if (key=="img" || key=="imagine.thumb") 
        continue;

      if (key=="" || val=="") {
        if (allow)
          continue;
        else {
          ShowError(msg+"failed with empty key or value key=<"+key+
                    "> value=<"+val+">");
          return empty;
        }
      }

      if (keyval.find(key)!=keyval.end()) {
        ShowError(msg+"<"+key+"> multiply defined");
        return empty;
      }

      keyval[key] = val;
    }

    return keyval;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<Query::html_image_data> Query::ParseXMLimgData(const XmlDom& cont,
                                                      string& db) const {
    string msg = "ParseXMLimgData() : ";

    if (debug_ajax>2)
      cout << msg << "called" << endl;

    list<html_image_data> ret;
    bool voc2007trick = false;

    for (XmlDom ele = cont.FirstChild(); ele; ele = ele.Next()) {
      string type = ele.NodeName();
      if (debug_ajax>2)
	cout << "    type=" << type << endl;

      if (type!="img" && type!="imagine.thumb")
        continue;

      string src = ele.Property("src");
      if (debug_ajax>2)
	cout << "    src=" << src << endl;

      bool sqr = false;
      if (type=="imagine.thumb") {
        sqr = true;
        db = "voc2007-current";
        size_t p = src.rfind('/');
        if (p!=string::npos)
          src.erase(0, p+1);
        p = src.rfind('.');
        if (p!=string::npos)
          src.erase(p);
	if (debug_ajax>2)
	  cout << "    srcx=" << src << " has_db=" << (GetDataBase()!=NULL)
	       << " islabel=" << (GetDataBase()&&GetDataBase()->IsLabel(src))
	       << endl;
        if (!GetDataBase() || !GetDataBase()->IsLabel(src))
          continue;
      }

      string proto, host, port, path;
      if (type=="img" && SplitURL(src, proto, host, port, path) &&
          proto=="http") {
	if (debug_ajax>2)
	  cout << "    proto=" << proto << " host=" << host
	       << " port=" << port << " path=" << path << endl;

	if (path.find("/database/")==0) { // picsom http
	  path.erase(0, 10);
	  pair<string,string> dblab =
	    Connection::HttpServerDataBaseAndLabel(Picsom(), path, true, true);
	  if (dblab.first!="") {
	    db = dblab.first;
	    if (voc2007trick && db=="voc2007-20080403")
	      db = "voc2007-current";
	  }

	} else {
	  size_t p = path.find("&imagedb="); // celum userscenario
	  if (p!=string::npos) { 
	    path.erase(0, p+9);
	    p = path.find('&');
	    if (p!=string::npos)
	      path.erase(p);
	    db = path;
	  }
	}
      }

      if (SkipXMLimage(type, src)) {
        cout << msg << "skipping type=" << type << " src=" << src << endl;
        continue;
      }

      int top    = ele.PropertyInt("top");
      int left   = ele.PropertyInt("left");
      int width  = ele.PropertyInt("width");
      int height = ele.PropertyInt("height");
      int x      = ele.PropertyInt("x");
      int y      = ele.PropertyInt("y");

      if (debug_ajax>2)
	cout << "    db=" << db << " top=" << top << " left=" << left
	     << " width=" << width << " height=" <<height
	     << " x=" << x << " y=" << y << endl;

      map<string,string> a = ParseXMLKeyValues(ele); // is this needed???

      if (collage_ranges_x.size() && collage_ranges_y.size()) {
	html_image_data img(top, left, width, height, x, y, sqr, type, src);
	ret.push_back(img);

      } else
	if (debug_ajax>2)
	  cout << "    SKIPPING due to empty ranges" << endl;
    }

    SetCollageData(ret);

    if (debug_ajax>2)
      cout << endl;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SkipXMLimage(const string& t, const string& s) const {
    // celum IMAGINE
    if (t=="img" && s.find("celum")!=string::npos &&
        (s.find("documentview.do")!=string::npos ||
           s.find("resources/ui/images")!=string::npos ||
           s.find("img/aci/folder")!=string::npos ||
         s.find("main.do")!=string::npos))
      return true;
    
    // flickr
    if (t=="img" && s.find("ads.bluelithium.com/pixel")!=string::npos)
      return true;
    if (t=="img" && s.find("http://l.yimg.com/")==0)
      return true;
    if (t=="img" && s.find(".flickr.com/")!=string::npos &&
        s.find("http://farm")!=0)
      return true;
    if (t=="img" && s.find(".flickr.com/")!=string::npos &&
        s.find("/buddyicons/")!=string::npos)
      return true;
    if (t=="img" && s.find(".flickr.com/")!=string::npos &&
        s.find("_s.jpg")==s.size()-6)
      return true;

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SetCollageData(list<html_image_data>& l) const {
    for (list<html_image_data>::iterator i=l.begin(); i!=l.end(); i++) {
      i->collage_position = SolveCollagePosition(i->left, i->top);
      i->collage_size     = make_pair(collage_ranges_x.size(),
                                      collage_ranges_y.size());

      // cout << "src=" << i->src << " collage_position=" << i->collage_spec()
      //      << endl;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<size_t,size_t> Query::SolveCollagePosition(size_t x, size_t y) const {
    size_t j = 0, k = 0;

    if (collage_ranges_x.size() && collage_ranges_y.size()) {
      for (; j<collage_ranges_x.size()-1; j++)
        if (x<=collage_ranges_x[j].second)
          break;

      for (; k<collage_ranges_y.size()-1; k++)
        if (y<=collage_ranges_y[k].second)
          break;
    }

    return make_pair(j, k);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::SolveRanges(vector<pair<size_t,size_t> >& l,
                          size_t a, size_t b) const {
    string msg = "Query::SolveRanges() : ";

    bool debug = false;

    if (debug) {
      cout << msg;
      for (size_t i=0; i<l.size(); i++)
        cout << "[" << l[i].first << "," << l[i].second << "] ";
      cout << "+ [" << a << "," << b << "]" << endl;
    }

    if (l.empty() || a>l.back().second) {
      if (debug)
        cout << " push_back" << endl;
      l.push_back(make_pair(a, b));
      return true;
    }

    size_t ai = l.size(), bi = l.size();
    for (size_t i=0; i<l.size(); i++) {
      if (a<=l[i].second && ai==l.size())
        ai = i;
      if (i<l.size()-1 && b<l[i+1].first && ai!=l.size() && bi==l.size())
        bi = i;
      if (b<=l[i].second && bi==l.size())
        bi = i;
    }

    if (ai==l.size())
      return ShowError(msg+"logic failure");

    if (debug)
      cout << " ai=" << ai << " bi=" << bi << endl;

    if (bi==l.size())
      bi--;

    if (ai==bi && b<l[ai].first) {
      l.insert(l.begin()+ai, make_pair(a, b));
      if (debug)
        cout << " insert " << ai << endl;
      return true;
    }

    if (a<l[ai].first) {
      l[ai].first = a;
      if (debug)
        cout << " change " << ai << ".first=" << a << endl; 
    }

    if (ai!=bi) {
      l[ai].second = l[bi].second;
      l.erase(l.begin()+ai+1, l.begin()+bi+1);
      if (debug)
        cout << " change " << ai << ".second=" << l[ai].second
             << " and erase " << ai+1 << "..." << bi << endl;
    }

    if (b>l[ai].second) {
      l[ai].second = b;
      if (debug)
        cout << " change " << ai << ".second=" << b << endl; 
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxImages(const XmlDom& ele, const uiart::TimeSpec& ft,
                                float pup, float fix, bool pointer, int left,
                                bool click_fixation, bool show) {
    string db;
    list<html_image_data> img = ParseXMLimgData(ele, db);
    
    if (debug_ajax>2) {
      cout << "  " << ele.NodeName() << " pointer=" << pointer << " left="
           << left << " click_fixation=" << click_fixation
           << " img.size()=" << img.size();
      if (db!="")
        cout << " db=" << db;
      cout << endl;

      for (list<html_image_data>::const_iterator i=img.begin();
           i!=img.end(); i++) {
        cout << "    img.top = "    << i->top    << endl;
        cout << "    img.left = "   << i->left   << endl;
        cout << "    img.width = "  << i->width  << endl;
        cout << "    img.height = " << i->height << endl;
        cout << "    img.x = "      << i->x      << endl;
        cout << "    img.y = "      << i->y      << endl;
        cout << "    img.square = " << i->square << endl;
        cout << "    img.type = "   << i->type   << endl;
        cout << "    img.src = "    << i->src    << endl;
      }
      cout << endl;
    }

    if (db!="")
      if (!SetDataBase(db))
        return ShowError("Query::ProcessAjaxImages() : SetDataBase("+
                         db+") failed");

    if (img.size()) {
      /* erf_image_data *aid = */ 
      ProcessXMLimgData(*img.begin(), ft, pup, fix,
                        pointer, left, click_fixation,
                        show, 0, 0);
      // if (aid)
      //   ConditionallyCalculateErfFeatures(*aid, 0);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessAjaxImagesCenter(const XmlDom& l, const XmlDom& r,
                                      const uiart::TimeSpec& ft,
                                      float dx, float dy,
                                      float pup, bool show) {
    string db;
    list<html_image_data> ll = ParseXMLimgData(l, db);
    list<html_image_data> rr = ParseXMLimgData(r, db);

    if (debug_ajax>2) {
      cout << "ProcessAjaxImagesCenter(): " << l.NodeName()
           << " dx=" << dx << " dy=" << dy
           << " ll.size()=" << ll.size() << " rr.size()=" << rr.size();
      if (db!="")
        cout << " db=" << db;
      cout << endl;

      for (size_t i=0; i<2; i++) {
        const list<html_image_data>& ii = i ? rr : ll;
        if (ii.size()) {
          cout << "  " << (i?"right":"left") << " eye:" << endl;
          cout << "    img.top = "    << ii.begin()->top    << endl;
          cout << "    img.left = "   << ii.begin()->left   << endl;
          cout << "    img.width = "  << ii.begin()->width  << endl;
          cout << "    img.height = " << ii.begin()->height << endl;
          cout << "    img.x = "      << ii.begin()->x      << endl;
          cout << "    img.y = "      << ii.begin()->y      << endl;
          cout << "    img.square = " << ii.begin()->square << endl;
          cout << "    img.type = "   << ii.begin()->type   << endl;
          cout << "    img.src = "    << ii.begin()->src    << endl;
        }
      }
    }

    // we now assume that ll/rr.size() == 0/1.
    
    bool done = false;
    if (ll.size()) {
      const html_image_data& img = *ll.begin();
      if (img.x+dx>=0 && img.x+dx<img.width &&
          img.y+dy>=0 && img.y+dy<img.height) {
        ProcessXMLimgData(img, ft, pup, 0.0, false, 2, false,
                          show, dx, dy);
        done = true;
        if (debug_ajax>2)
          cout << "CENTER EYE MATCH due to left eye " << img.src << endl
               << endl;
      }
    }

    if (!done && rr.size()) {
      const html_image_data& img = *rr.begin();
      if (img.x-dx>=0 && img.x-dx<img.width &&
          img.y-dy>=0 && img.y-dy<img.height)
        ProcessXMLimgData(img, ft, pup, 0.0, false, 2, false,
                          show, dx, dy);
        if (debug_ajax>2)
          cout << "CENTER EYE MATCH due to right eye" << img.src << endl
               << endl;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
    
  const string& Query::ErfImageColor(const string& u) const {
    static string blue    = "blue";
    static string red     = "red";
    static string cyan    = "cyan";
    static string magenta = "magenta";
    static string black   = "black";

    if (u=="gazesample")    return magenta;
    if (u=="fixation")      return red;
    if (u=="pointersample") return cyan;
    if (u=="click")         return blue;

    return black;
  }

  /////////////////////////////////////////////////////////////////////////////
    
  const string& Query::ErfImageColor(bool p, bool f) const {
    return ErfImageColor(p ? f ? "click"    : "pointersample"
                         :   f ? "fixation" : "gazesample");
  }

  /////////////////////////////////////////////////////////////////////////////

  Query::erf_image_data *Query::ProcessXMLimgData(const html_image_data& img,
                                                  const uiart::TimeSpec& ft,
                                                  float pup, float fix,
                                                  bool pointer, int eye,
                                                  bool click_fixation,
                                                  bool feedin, float dx,
                                                  float dy) {
    string msg = "ProcessXMLimgData() : ";

    bool feed = feedin;

    if (AjaxMplayer() && feed) {
      string spec = "-geometry 90%:90%";

      if (ajax_image_mplayer.is_broken())
        ajax_image_mplayer.close();
      if (!ajax_image_mplayer.is_open()) {
        if (AjaxMplayerFile()) {
          spec = Identity()+"-image.mpeg";
          WriteLog("Starting to create image video file <"+spec+">");
        }
        ajax_image_mplayer.open(spec, true);
      }
    }

    erf_image_data& aid = ErfImageData(img.src);
    aid.position = img;

    if (aid.original.empty())
      ErfInitializeImageData(aid, img.src, AjaxMplayer());

    int imgx = img.x, imgy = img.y;
    int imgw = img.width, imgh = img.height;

    if (img.square && !XMLimgSquareCorrection(aid, false,
                                              imgx, imgy, imgw, imgh))
      return NULL;

    // this would be used if we normalized gaze/pointer coordinates here
    // float xo = float(imgx)/imgw *aid.original_width;
    // float yo = float(imgy)/imgh*aid.original_height;
    // uiart::GazeData gd(xo, yo, ft, pup, fix);
    
    uiart::GazeData gd(imgx+dx, imgy+dy, ft, pup, fix);

    if (pointer)
      (click_fixation ? aid.pointer_click : aid.pointer_sample).push_back(gd);
    else
      (click_fixation ? aid.coorddata(false, eye, true) :
       eye==0 ? aid.gaze_sample_left : eye==1 ? aid.gaze_sample_right
       : aid.gaze_sample_center).push_back(gd);

    if (!aid.cumulative.empty() && AjaxMplayer()) {
      imagedata& ihist = aid.cumulative;

      int x = (int)floor(float(imgx)/imgw*ihist.width() +0.5);
      int y = (int)floor(float(imgy)/imgh*ihist.height()+0.5);

      ajax_image_x = x;
      ajax_image_y = y;
      ajax_image_p = &aid;

      const string& col = ErfImageColor(pointer, click_fixation);
      if (click_fixation) {
        ihist.line(x-5, y-5, x+5, y+5, 3, col);
        ihist.line(x+5, y-5, x-5, y+5, 3, col);
      } else {
        ihist.line(x,   y-5, x,   y+5, 3, col);
        ihist.line(x-5, y,   x+5, y,   3, col);
      }

      if (feed) {
	imagedata itemp = aid.scaled;;
	if (click_fixation) {
	  itemp.line(x-5, y-5, x+5, y+5, 3, col);
	  itemp.line(x+5, y-5, x-5, y+5, 3, col);
	} else {
	  itemp.line(x,   y-5, x,   y+5, 3, col);
	  itemp.line(x-5, y,   x+5, y,   3, col);
	}

        if (AjaxMplayer())
          try {
            imagedata iboth = CombineImages(itemp, ihist);
            ajax_image_mplayer.add_frame(iboth);
          } catch (...) {
            ShowError(msg+"add_frame() failed");
            return NULL;
          }
      }
    }

    return &aid;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::XMLimgSquareCorrection(const erf_image_data& aid, bool box,
                                     int& imgx, int& imgy,
                                     int& imgw, int& imgh) const {
    bool debug = false;

    int imgwin = imgw, imghin = imgh;
    double mul = double(aid.original_width)/aid.original_height;

    if (debug)
      cout << "XMLimgSquareCorrection : box=" << box << " "
           << imgx << " " << imgy << " " << imgw << " " << imgh
           << " (" << aid.original_width << "x" << aid.original_height
           << "=" << mul << ") ";

    if (mul<1.0) {
      imgw  = (int)floor(imgw*mul+0.5);
      imgx += (box?1:-1)*(int)floor(0.5*(imgwin-imgw)+0.5);
      
    } else {
      imgh  = (int)floor(imgh/mul+0.5);
      imgy += (box?1:-1)*(int)floor(0.5*(imghin-imgh)+0.5);
    }

    if (debug)
      cout << "-> " << imgx << " " << imgy << " " << imgw << " " << imgh
           << endl;

    return !(imgx<0 || imgx>=imgw || imgy<0 || imgy>=imgh);
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata Query::CombineImages(const imagedata& l, const imagedata& r) {
    imagedata iboth(1, 1, 1);
    iboth.set(0, 0, float(1));

    size_t dx = 10, w = l.width()+r.width()+dx;
    size_t h = l.height()>r.height() ? l.height() : r.height();
    scalinginfo os(1, 1, w, h);

    iboth.rescale(os);
    iboth.convert(imagedata::pixeldata_uchar);
    iboth.force_three_channel();
    iboth.copyAsSubimage(l, 0, 0);
    iboth.copyAsSubimage(r, l.width()+dx, 0);

    return iboth;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ErfInitializeImageData(erf_image_data& aid, const string& url,
                                     bool images) {
    string lab = url;
    size_t p = lab.rfind('/');
    if (p!=string::npos)
      lab.erase(0, p+1);

    if (!images && GetDataBase()) {
      map<string,string> hash = GetDataBase()->ReadOriginsInfo(LabelIndex(lab),
                                                               false, false);
      int w = 0, h = 0;
      if (sscanf(hash["dimensions"].c_str(), "%dx%d", &w, &h)==2) {
        aid.original_width  = w;
        aid.original_height = h;
      }

      return true;
    }

    imagedata idata;
    
    if (GetDataBase()) {
      int idx = GetDataBase()->LabelIndex(lab);
      idata = GetDataBase()->ImageData(idx);

    } else {
      list<pair<string,string> > hdrs;
      string ctype, tmpname = Connection::DownloadFile(Picsom(), url,
						       hdrs, ctype);
      if (!tmpname.empty()) {
        idata = imagefile(tmpname).frame(0);
        // imagefile ifile = imagefile::unstringify(imgstr);
        Unlink(tmpname);
      }
    }
       
    aid.original = idata;

    if (idata.iszero()) //!isempty
      return false;

    aid.original_width  = idata.width();
    aid.original_height = idata.height();

    cout << url << " -> " << lab << " " << idata.info() << endl;

    size_t iwidth = 400, iheight = 300;
    if (idata.width()*iheight>idata.height()*iwidth)
      iheight = idata.height()*iwidth/idata.width();
    else
      iwidth  = idata.width()*iheight/idata.height();

    scalinginfo si(idata.width(), idata.height(), iwidth, iheight);

    idata.convert(imagedata::pixeldata_uchar);
    idata.rescale(si);
    idata.force_three_channel();

    aid.scaled = aid.cumulative = idata;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::ProcessXMLcollageData(int xabs, int yabs, bool pointer,
                                    bool click, bool feedin) {
    string msg = "ProcessXMLcollageData() : ";

    bool feed = feedin;

    if (!ajax_collage_image.width())
      return true;

    if (AjaxMplayer() && feed) {
      string spec = "-geometry 10%:10%";
      if (AjaxMplayerFile())
        spec = Identity()+"-collage.mpeg";

      if (ajax_collage_mplayer.is_broken())
        ajax_collage_mplayer.close();
      if (!ajax_collage_mplayer.is_open()) {
        ajax_collage_mplayer.open(spec, true);
        if (spec[0]!='-')
          WriteLog("Starting to create collage video file <"+spec+">");
      }

      if (!ajax_collage_mplayer.is_open() || ajax_collage_mplayer.is_broken())
        return true;
    }

    imagedata& ctmp = ajax_collage_image_cumulative;
    imagedata  itmp = ajax_collage_image;
    int x = xabs/2;
    int y = yabs/2;

    const string& col = ErfImageColor(pointer, click);
    if (click) {
      itmp.line(x-5, y-5, x+5, y+5, 3, col);
      itmp.line(x+5, y-5, x-5, y+5, 3, col);
      ctmp.line(x-5, y-5, x+5, y+5, 3, col);
      ctmp.line(x+5, y-5, x-5, y+5, 3, col);
    } else {
      itmp.line(x,   y-5, x,   y+5, 3, col);
      itmp.line(x-5, y,   x+5, y,   3, col);
      ctmp.line(x,   y-5, x,   y+5, 3, col);
      ctmp.line(x-5, y,   x+5, y,   3, col);
    }

    if (AjaxMplayer() && feed)
      try {
        imagedata iboth = CombineImages(itmp, ctmp);
        ajax_collage_mplayer.add_frame(iboth);
      } catch (...) {
        return ShowError(msg+"add_frame() failed");
      }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::CloseVideoOutput() {
    if (AjaxMplayer()) {
      if (ajax_collage_mplayer.is_open()) {
        if (AjaxMplayerFile())
          ajax_collage_mplayer.write(25);
        else
          ajax_collage_mplayer.close();
      }

      if (ajax_image_mplayer.is_open()) {
        if (AjaxMplayerFile())
          ajax_image_mplayer.write(25);
        else
          ajax_image_mplayer.close();
      }
    }

    for (size_t i=0; i<Nchildren(); i++)
      Child(i)->CloseVideoOutput();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLajaxdata(XmlDom& xml) {
    if (debug_ajax)
      cout << "Query::AddToXMLajaxdata() called" << endl;

    if (ajax_data_all.Root())
      xml.AddCopy(ajax_data_all.Root());
    else
      xml.Element("ajaxdata");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLajaxresponse(XmlDom& xml, const Connection *cc) {
    if (debug_ajax)
      cout << "Query::AddToXMLajaxresponse() called" << endl;

    if (cc)
      cc->HttpServerMetaErf(xml, this);

    XmlDom r = xml.Element("ajaxresponse");
    r.Ns("http://www.pinview.eu/ns", "pinview");

    if (ajaxresponse.count("gotopage")) {
      string url = ajaxresponse.find("gotopage")->second;
      r.Element("gotopage", url);
      if (debug_ajax)
        cout << Identity() << " returns AJAX gotopage <"
             << url << ">" << endl;
    }

    if (ajaxresponse.count("changetext")) {
      typedef multimap<string,string> amap;
      pair<amap::const_iterator,amap::const_iterator> ar
        = ajaxresponse.equal_range("changetext");
      for (amap::const_iterator i=ar.first; i!=ar.second; i++) {
        string txt = i->second;
        r.Element("changetext", txt);
        if (debug_ajax)
	  cout << Identity() << " returns AJAX changetext <"
	       << txt << ">" << endl;
      }
    }

    if (ajaxresponse.count("setaspect")) {
      typedef multimap<string,string> amap;
      pair<amap::const_iterator,amap::const_iterator> ar
        = ajaxresponse.equal_range("setaspect");
      for (amap::const_iterator i=ar.first; i!=ar.second; i++) {
        string a = i->second;
        r.Element("setaspect", a);
        if (debug_ajax)
          cout << Identity() << " returns AJAX setaspect <"
               << a << ">" << endl;
      }
    }

    ajaxresponse.clear();

    if (cc && debug_ajax>1)
      cc->DumpXML(cout, true, xml.doc, "ajaxresponse", "", true, false);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::AddToXMLerfdata(XmlDom& xml) {
    if (debug_ajax)
      cout << "Query::AddToXMLerfdata() called" << endl;

    XmlDom eid = xml.Element("erf-imagedata");
    for (auto i=erf_image_data_cache.begin();
	 i!=erf_image_data_cache.end(); i++) {
      XmlDom image = eid.Element("erf-image");
      image.Prop("label", i->second.label);
      for (auto j=i->second.detections.begin();
	   j!=i->second.detections.end(); j++) {
	XmlDom det = image.Element("detection");
	for (auto k=j->begin(); k!=j->end(); k++)
	  det.Prop(k->first, k->second);
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::DoGoogleImageSearch(const string& sin, size_t n) {
    list<string> urls = GoogleCustomSearch(sin, "image", n);
    for (auto i=urls.begin(); i!=urls.end(); i++)
      cout << *i << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> Query::GoogleCustomSearch(const string& sin,
					 const string& spec,
					 size_t n) {
    string msg = "Query::GoogleCustomSearch("+sin+","+spec+","+ToStr(n)+
      ") : ";

    bool debug = true, save = true, fake = true, only_one = false;

    string apikey = "AIzaSyAQYldUvPpHWEOx1UfDna6GA1XdSg7cH34"; // obs!
    string cx = "014141179952337231122:sf3dv13t6le";           // obs!
    //https://code.google.com/apis/console/#project:1014344100183:billing

    bool image = spec.find("image")!=string::npos;

    string s = sin;
    while (s.size() && (s[0]==' ' || s[0]=='\t'))
      s.erase(0, 1);
    while (s.size() && (s[s.size()-1]==' ' || s[s.size()-1]=='\t'))
      s.erase(s.size()-1, 1);

    for (;;) {
      size_t p = s.find(' ');
      if (p==string::npos)
	break;
      s.replace(p, 1, "%20");
    }

    list<string> ret;

    size_t page = 0;
    while (ret.size()<n) {
      string url = "https://www.googleapis.com/customsearch/v1?";
      url += "key="+apikey;
      url += "&cx="+cx;
      url += "&alt=atom";
      url += "&q="+s;

      if (image)
	url += "&searchType=image";

      if (ret.size())
	url += "&start="+ToStr(ret.size()+1);

      // &num=10
      // &fileType=jpg&imgSize=small

      if (debug)
	cout << "|" << url << "|" << endl;

      string out = "google-custom-search-"+s+"-"+ToStr(page)+".xml";
      string ctype, atomf;
      if (!fake) {
	list<pair<string,string> > hdrs;
	atomf = Connection::DownloadFile(Picsom(), url, hdrs, ctype);
	if (debug)
	  cout << "FILE: " << atomf << " CTYPE: " << ctype << endl;
	if (atomf=="" || FileSize(atomf)<1) {
	  ShowError(msg+"query("+url+") failed");
	  return ret;
	}

	if (save) {
	  CopyFile(atomf, out);
	  WriteLog("Google custom search result saved in <"+out+">");
	}

      } else
	atomf = "/home/jorma/picsom/c++/"
	  "google-custom-search-ellis%20island_kept-10.xml";
      
      XmlDom doc = XmlDom::Parse(atomf);
      XmlDom feed = doc.Root();
      XmlDom entry = feed.FirstElementChild("entry");
      while (entry) {
	XmlDom id = entry.FirstElementChild("id");
	string txt = id.FirstChildContent();
	ret.push_back(txt);
	entry = entry.NextElement("entry");
	if (only_one || ret.size()>=n)
	  break;
      }

      if (debug)
	cout << "page #" << page << " contained " << ret.size()-10*n
	     << " images" << endl;

      page++;
      if (ret.size()!=10*page)
	break;
    }

    if (debug)
      cout << "returning total of " << ret.size() << " images" << endl;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Query::WaitUntilReady() const {
    while (InRun()) {
#ifdef PICSOM_USE_PTHREADS
      if (Picsom()->DebugThreads())
	cout << TimeStamp() << "query " << Identity()
	     << " is being run and waited..." << endl;
#endif // PICSOM_USE_PTHREADS

      timespec_t snap = { 0, 1000 };
      nanosleep(&snap, NULL);      
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  // bool Query::() const {
  // }

  // class Query ends here

  /////////////////////////////////////////////////////////////////////////////

  vector<double> NewtonOpt::Optimise(const vector<double> &initial){

    typedef double * dptr;

    nparam=initial.size();
    if(nparam<1) return initial;

    // allocate memory for theta,newtheta, gradient and hessian

    current=new double[nparam];
    newtheta=new double[nparam];
    gradient=new double[nparam];
    
    hdata=new double[nparam*nparam];
    // set up pointers to rows of hessian
    hessian= new dptr[nparam];

    for(int i=0;i<nparam;i++)
      hessian[i]=hdata+nparam*i;

    // call initialisation of the derived classes
    InitOpt();

    for(int i=0;i<nparam;i++)
      current[i]=initial[i];

    NewWPrecalculate();    

    CalculateGradient();
    CalculateHessian();
    InvertHessian();

  double fval;
  double old_val;
  double deltaf;

  double stepsize;

  const double stopratio=0.00001;
  const double fvallimit=-0.0001;

  old_val=CalculateFval(current);

  do{ 
    for(int i=0;i<nparam;i++){
      newtheta[i]=current[i];
      for(int j=0;j<nparam;j++)
        newtheta[i] -= hessian[i][j]*gradient[j];
      // cout << "step["<<i<<"]="<<theta[i]-old_theta[i]<<endl;
    }
    
    fval=CalculateFval(newtheta);

    double sd=0;
    for(int i=0;i<nparam;i++){
      double d=newtheta[i]-current[i];
      sd += d*d;
    }

    stepsize=sqrt(sd);

    //cout << "stepsize "<<stepsize<<" -> fval="<<fval<<endl;

    deltaf=abs((fval-old_val)/old_val);

    while(!(fval>old_val) && deltaf>stopratio && fval<fvallimit ){

      cout << "Rejected step (";
              for(int i=0;i<nparam;i++)
                cout <<current[i]<<" ";
              cout <<") -> (";
              for(int i=0;i<nparam;i++)
                cout <<newtheta[i]<<" "; 
              cout <<") fval "<<old_val
                   <<" -> "<<fval<<" step="<<stepsize<< endl;

        for(int i=0;i<nparam;i++)
          newtheta[i]=0.5*(newtheta[i]+current[i]);

        stepsize *= 0.5;
        
        fval=CalculateFval(newtheta);
        deltaf=abs((fval-old_val)/old_val);
    }

    deltaf=abs((fval-old_val)/old_val);

    if(fval>old_val){

            cout << "Accepted step (";
              for(int i=0;i<nparam;i++)
                cout <<current[i]<<" ";

              cout <<") -> (";
              for(int i=0;i<nparam;i++)
                cout <<newtheta[i]<<" "; 

        cout <<") fval "<<old_val
                   <<" -> "<<fval<<" step="<<stepsize<< endl;

        double *ptr=current;
        current=newtheta;
        newtheta=ptr;

        NewWPrecalculate();    

        CalculateGradient();
        CalculateHessian();
        InvertHessian();
        
        old_val=fval;

    }
   
    } while (deltaf>stopratio && fval < fvallimit);


  cout << "EM terminated with the result theta=[";
  for(int i=0;i<nparam;i++)
    cout <<current[i]<<" ";
  cout << endl;

  // collect the result

    vector<double> ret;
    for(int i=0;i<nparam;i++)
      ret.push_back(current[i]);


    // free resources allocated by the derived classes
    EndOpt();

    //de-allocate theta, gradient and hessian

    delete[] hessian;
    delete[] hdata;
    delete[] gradient;
    delete[] newtheta;
    delete[] current;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  void NewtonOpt::InvertHessian(){
    // we have to offset the indices

    typedef double * dptr;

    double **tgtp=new dptr[nparam];
    for(int i=0;i<nparam;i++)
      tgtp[i]=hessian[i]-1;

    InvertMatrix(tgtp-1,nparam);

    delete[] tgtp;

  }

  /////////////////////////////////////////////////////////////////////////////

  double MapSurfaceMAPOpt::ParamToProb(double d){
    return 1/(1+exp(-d));
  }

  /////////////////////////////////////////////////////////////////////////////

  void MapSurfaceMAPOpt::ParamToProb(vector<double> &v){
    for(size_t i=0;i<v.size();i++)
      v[i]=ParamToProb(v[i]);
  }

  /////////////////////////////////////////////////////////////////////////////

  void MapSurfaceMAPOpt::ConstructInverseCovariance(){
    // simple covariance matrix
    // c_ij = 1, if i==j
    // c_ij = a, if 4-neighbours(i,j)
    // c_ij = 0 otherwise

    // after this scaling with factor var
    // (chosen to correspond range [0.25 0.75] if the mean is 1/2

    double a= 0.5;
    double var=0.9; // approximation of (ln 3)^3*2/3  
    
    int nparam=w*h;

    // allocate memory

    typedef double *dptr;

    cdata=new double[nparam*nparam];
    c_inv=new dptr[nparam];
    for(int i=0;i<nparam;i++) c_inv[i]=cdata+i*nparam;

    // construct the covariance matrix

    for (int i=0; i<nparam; i++)
      for (int j=0; j<nparam; j++) {
        if (i==j)
          c_inv[i][j] = var;
        else if (((i==j-w || i==j+w)&&(i%w==j%w)) ||
                 ((i==j-1 || i==j+1)&&(i/w==j/w)))
          c_inv[i][j] = var*a;
        else
          c_inv[i][j] = 0;
      }

    // invert the matrix

    typedef double * dptr;

    double **tgtp=new dptr[nparam];
    for(int i=0;i<nparam;i++)
      tgtp[i]=c_inv[i]-1;

    InvertMatrix(tgtp-1,nparam);

    delete[] tgtp;

  }

  /////////////////////////////////////////////////////////////////////////////

  double MapSurfaceMAPOpt::CalculateFval(double *w){

    // implements ret = \sum_i (pos[i] ln u(w[i])+neg[i]*ln(1-u(w[i])) 
    // - 1/2 sum_{ij} C^{-1}_{ij}*w[i]*w[j]

    double ret=0;

    for(int i=0;i<nparam;i++){
      ret += pos[i]*log(ParamToProb(w[i]));
      ret += neg[i]*log(1-ParamToProb(w[i]));
    }
      
    for(int i=0;i<nparam;i++)
      for(int j=0;j<nparam;j++)
        ret -= 0.5*c_inv[i][j]*w[i]*w[j];

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  void MapSurfaceMAPOpt::CalculateGradient(){
    // implements g_k = \partial f/\partial w_k 
    //                = pos[k](1-u(w_k))-neg[k]*u(w_k)-\sum_i w_i C^{-1}_{ik}

    for(int k=0;k<nparam;k++){
      gradient[k]=pos[k]*(1-uval[k]);
      gradient[k]-=neg[k]*uval[k];
      for(int i=0;i<nparam;i++)
        gradient[k] -= current[i]*c_inv[i][k];
    }

  }

  /////////////////////////////////////////////////////////////////////////////

  void MapSurfaceMAPOpt::CalculateHessian(){
    // implemenmts h_kl = -\delta_kl u(w_k)(1-u(w_k))(pos[k]+neg[k])
    //                    - C^{-1}_{kl}

    for(int k=0;k<nparam;k++){
      for(int l=0;l<nparam;l++)
        hessian[k][l] = -c_inv[k][l];
      hessian[k][k] -= uval[k]*(1-uval[k])*(pos[k]+neg[k]);
    }

  }

  /////////////////////////////////////////////////////////////////////////////

  void MapSurfaceMAPOpt::NewWPrecalculate(){
    int nparam=w*h;
    for(int i=0;i<nparam;i++)
      uval[i]=ParamToProb(current[i]);
  }

  /////////////////////////////////////////////////////////////////////////////

  void MapSurfaceMAPOpt::InitOpt(){
    uval=new double[h*w];
  }

  /////////////////////////////////////////////////////////////////////////////

  void MapSurfaceMAPOpt::EndOpt(){
    delete[] uval;
    uval=NULL;
  } // place for freeing memory etc.

  /////////////////////////////////////////////////////////////////////////////

#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;} 

  void InvertMatrix(double **a, int n){

    // matrix inversion by Gauss-Jordan elimination. 
    // code snippet adapted from Numerical Recipes

    // NOTE: matrix indices start at one.
    // (some pointer arithmetic needed in the calling end
    // if zero-offset indices are used)


    // a[1..n][1..n] is the input matrix. 
    // On output, a is replaced by its matrix inverse

    int *indxc,*indxr,*ipiv; 
    int i,icol=-1,irow=-1,j,k,l,ll; 
    double big,dum,pivinv,temp; 

    //indxc=ivector(1,n);
    indxc=new int[n+1];
    // The integer arrays ipiv, indxr, and indxc are used for bookkeeping on the pivoting. 
    //indxr=ivector(1,n); 
    indxr=new int[n+1]; 
    //ipiv=ivector(1,n);
    ipiv=new int[n+1];
    
    for (j=1;j<=n;j++) 
      ipiv[j]=0; 
    for (i=1;i<=n;i++) {
      // This is the main loop over the columns to be reduced. 
      big=0.0; 
      for (j=1;j<=n;j++) 
        //This is the outer loop of the search for a pivot element. 
        if (ipiv[j] != 1) 
          for (k=1;k<=n;k++) { 
            if (ipiv[k] == 0) { 
              if (fabs(a[j][k]) >= big) { 
                big=fabs(a[j][k]); 
                irow=j; 
                icol=k; 
              } 
            } 
          } 
      ++(ipiv[icol]);
      // We now have the pivot element, so we interchange rows, if needed, 
      // to put the pivot element on the diagonal. The columns are not 
      // physically interchanged, only relabeled: indxc[i], the column of 
      // the ith pivot element, is the ith column that is reduced, 
      // while indxr[i] is the row in which that pivot element was originally 
      // located. If indxr[i]  = indxc[i] there is an implied column 
      // interchange. With this form of bookkeeping, the solution b s will 
      // end up in the correct order, and the inverse matrix will be scrambled 
      // by columns. 
      if (irow != icol) { 
        for (l=1;l<=n;l++) SWAP(a[irow][l],a[icol][l]); 
        //for (l=1;l<=m;l++) SWAP(b[irow][l],b[icol][l]);
      } 
      indxr[i]=irow; 
      // We are now ready to divide the pivot row by the pivot element, 
      // located at irow and icol. 
      indxc[i]=icol; 
      if (a[icol][icol] == 0.0) {
        cerr<<("invertmatrix(): Singular Matrix")<<endl;
        exit(-1);
      }
      pivinv=1.0/a[icol][icol]; 
      a[icol][icol]=1.0; 
      for (l=1;l<=n;l++) a[icol][l] *= pivinv; 
      //for (l=1;l<=m;l++) b[icol][l] *= pivinv;
      
      for (ll=1;ll<=n;ll++) 
        // Next, we reduce the rows... 
        if (ll != icol) { 
          //...except for the pivot one, of course. 
          dum=a[ll][icol]; 
          a[ll][icol]=0.0; 
          for (l=1;l<=n;l++) a[ll][l] -= a[icol][l]*dum; 
          //for (l=1;l<=m;l++) b[ll][l] -= b[icol][l]*dum; 
        } 
    }
    // This is the end of the main loop over columns of the reduction. 
    // It only remains to unscramble the solution in view of the column 
    // interchanges. We do this by interchanging pairs of columns 
    // in the reverse order that the permutation was built up. 
    for (l=n;l>=1;l--) { 
      if (indxr[l] != indxc[l]) 
        for (k=1;k<=n;k++) 
          SWAP(a[k][indxr[l]],a[k][indxc[l]]); 
    } 
    // And we are done. 
    delete[] ipiv;
    delete[] indxr;
    delete[] indxc;
  }


  std::vector<float> MSEFitParabola(const std::vector<float>&x,
                                    const std::vector<float>&y){

    // returns parameter vector [a b c] of a parabola
    // y=ax^2+bx+c that best fits to the observations 
    // in MSE sense

    size_t i,j;

    size_t n=x.size();
    if(n!=y.size())
      throw string(" MSEFitParabola(): vector length mismatch");

    vector<double> mx(5,0.0);
    vector<double> cm(3,0.0);

    for(i=0;i<n;i++){
      float p=1;
      for(j=0;j<4;j++){
        mx[j] += p;
        p *= x[i];
      }
      mx[j] += p;

      p=y[i];
      for(j=0;j<2;j++){
        cm[j] += p;
        p *= x[i];
      }
      cm[j] += p;
    }

    // construct the matrix

    double m[3][3];

    for(i=0;i<3;i++)
      for(j=0;j<3;j++)
        m[i][j]=mx[i+2-j];


    cout << "Matrix before inversion" << endl;
    for(i=0;i<3;i++){
      for(j=0;j<3;j++)    
        cout << m[i][j]<<" ";
      cout << endl;
    }

    typedef double * dptr;

    double **tgtp=new dptr[3];
    for(i=0;i<3;i++)
      tgtp[i]=m[i]-1;

    InvertMatrix(tgtp-1,3);

    delete[] tgtp;

    cout << "Matrix after inversion" << endl;
    for(i=0;i<3;i++){
      for(j=0;j<3;j++)    
        cout << m[i][j]<<" ";
      cout << endl;
    }

    vector<float> ret(3,0.0);

    for(i=0;i<3;i++)
      for(j=0;j<3;j++)
        ret[i] += m[i][j]*cm[j];
    
    return ret;

  }

  /////////////////////////////////////////////////////////////////////////////

  static float maxarg1,maxarg2;
#define FMAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
(maxarg1) : (maxarg2))

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

  void mnbrak(float *ax, float *bx, float *cx, 
              float *fa, float *fb, float *fc,
              floatEvaluator *func) {
#define GOLD 1.618034
#define GLIMIT 10.0
#define TINY 1.0e-20
#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);
    // Here GOLD is the default ratio by which successive intervals 
    // are magnified; 
    // GLIMIT is the
    // maximum magnification allowed for a parabolic-fit step.
    
    // Given a function func, and given distinct initial points ax and bx, 
    // this routine searches in the downhill direction (defined by the 
    // function as evaluated at the initial points) and returns
    // new points ax, bx, cx that bracket a minimum of the function. 
    // Also returned are the function values at the three points, fa, fb, 
    // and fc.

    // to save function evaluations, input args *fa and *fb specify the 
    // function values at the initial points ax, bx

    
    float ulim,u,r,q,fu,dum;
    // *fa=(*func)(*ax); commented out
    //*fb=(*func)(*bx);
    if (*fb > *fa) { 
      // Switch roles of a and b so that we can go
      // downhill in the direction from a to b. 
      SHFT(dum,*ax,*bx,dum);
      SHFT(dum,*fb,*fa,dum);
    }
    //*cx=(*bx)+GOLD*(*bx-*ax); // First guess for c.
    *cx=(*bx)+GOLD*(*bx-*ax)*0.3; // First guess for c.
    *fc=func->eval(*cx);
    while (*fb > *fc) { //Keep returning here until we bracket.
      r=(*bx-*ax)*(*fb-*fc); 
      // Compute u by parabolic extrapolation from a, b, c. TINY is used 
      // to prevent any possible division by zero.
      q=(*bx-*cx)*(*fb-*fa);
      u=(*bx)-((*bx-*cx)*q-(*bx-*ax)*r)/
        (2.0*SIGN(FMAX(fabs(q-r),TINY),q-r));
      ulim=(*bx)+GLIMIT*(*cx-*bx);
      // We wont go farther than this. Test various possibilities:
      if ((*bx-u)*(u-*cx) > 0.0) { 
        // Parabolic u is between b and c: try it.
        fu=func->eval(u);
        if (fu < *fc) { // Got a minimum between b and c.
          *ax=(*bx);
          *bx=u;
          *fa=(*fb);
          *fb=fu;
          return;
        } else if (fu > *fb) { // Got a minimum between between a and u.
          *cx=u;
          *fc=fu;
          return;
        }
        u=(*cx)+GOLD*(*cx-*bx); // Parabolic fit was no use. 
        // Use default magnification.
        fu=func->eval(u);
      } else if ((*cx-u)*(u-ulim) > 0.0) { 
        // Parabolic fit is between c and its allowed limit. 
        fu=func->eval(u);
        if (fu < *fc) {
          SHFT(*bx,*cx,u,*cx+GOLD*(*cx-*bx));
          SHFT(*fb,*fc,fu,func->eval(u));
        }
      } else if ((u-ulim)*(ulim-*cx) >= 0.0) { 
        // Limit parabolic u to maximum allowed value. 
        u=ulim;
        fu=func->eval(u);
      } else { 
        // Reject parabolic u, use default magnification.
        u=(*cx)+GOLD*(*cx-*bx);
        fu=func->eval(u);
      }
      SHFT(*ax,*bx,*cx,u); // Eliminate oldest point and continue.
      SHFT(*fa,*fb,*fc,fu);
    }
  }
  
  ////////////////////////////////////////////////////////////////////////////// 
  float golden(float ax, float bx, float cx, floatEvaluator *f, float tol,
               float *xmin,float fb){
    // the golden ratios
#define R 0.61803399 
#define C (1.0-R)
#define SHFT2(a,b,c) (a)=(b);(b)=(c);
#define SHFT3(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);
    
    // Given a function f, and given a bracketing triplet of 
    // abscissas ax, bx, cx (such that bx is between ax and cx, 
    // and f(bx) is less than both f(ax) and f(cx)), this routine performs 
    // a golden section search for the minimum, isolating it to a fractional 
    // precision of about tol. The abscissa of the minimum is returned as 
    // xmin, and the minimum function value is returned as
    // golden, the returned function value.

    // pre-evaluated function value f(bx) is given as argument 
    // to avoid unnecessary function evaluation

    float f1,f2,x0,x1,x2,x3;

    x0=ax; 
    // At any given time we will keep track of four points, x0,x1,x2,x3. 
    x3=cx;



    // The initial function evaluations are now combined with
    // the initial point selection. Note that we never need to evaluate 
    // the function at the original endpoints.


    if (fabs(cx-bx) > fabs(bx-ax)) { //Make x0 to x1 the smaller segment,
      x1=bx; f1=fb;
      x2=bx+C*(cx-bx); // and fill in the new point to be tried.
      f2=f->eval(x2);
    } else {
      x2=bx; f2=fb;
      x1=bx-C*(bx-ax);
      f1=f->eval(x1); 
    }

    while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2))) {
      if (f2 < f1) { //One possible outcome,
        SHFT3(x0,x1,x2,R*x1+C*x3); // its housekeeping,
        SHFT2(f1,f2,f->eval(x2));     // and a new function evaluation.
      } else { // The other outcome,
        SHFT3(x3,x2,x1,R*x2+C*x0);
        SHFT2(f2,f1,f->eval(x1)); // and its new function evaluation.
      }
    } //Back to see if we are done.
    if (f1 < f2) { 
      // We are done. Output the best of the two current values. 
      *xmin=x1;
      return f1;
    } else {
      *xmin=x2;
      return f2;
    }
  }

  //////////////////////////////////////////////////////////////////////////////

  float brent(float ax, float bx, float cx, floatEvaluator *f, float tol,
              float *xmin, float fb){

    // adapted from numerical recipes

#define ITMAX 100
#define CGOLD 0.3819660
#define ZEPS 1.0e-10
    // Here ITMAX is the maximum allowed number of iterations; 
    // CGOLD is the golden ratio; 
    // ZEPS is a small number that protects against trying to achieve 
    // fractional accuracy for a minimum that happens to be exactly zero.

#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);
    // Given a function f, and given a bracketing triplet of 
    // abscissas ax, bx, cx (such that bx is between ax and cx, 
    // and f(bx) is less than both f(ax) and f(cx)), this routine isolates
    // the minimum to a fractional precision of about tol 
    // using Brents method. The abscissa of the minimum is returned as xmin, 
    // and the minimum function value is returned as brent, 
    // the returned function value.

    // pre-evaluated function value f(bx) is given as argument 
    // to avoid unnecessary function evaluation

    int iter;
    float a,b,d=0.0,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
    float e=0.0; 

    // This will be the distance moved on the step before last.
    a=(ax < cx ? ax : cx); 
    // a and b must be in ascending order,
    //            but input abscissas need not be. 
    b=(ax > cx ? ax : cx);
    x=w=v=bx; // Initializations...
    fw=fv=fx=fb; // was originally (*f)(x);
    for (iter=1;iter<=ITMAX;iter++) { // Main program loop.
      xm=0.5*(a+b);
      tol2=2.0*(tol1=tol*fabs(x)+ZEPS);
      if (fabs(x-xm) <= (tol2-0.5*(b-a))) { //Test for done here.
        *xmin=x;
        return fx;
      }
      if (fabs(e) > tol1) { //Construct a trial parabolic fit.
        r=(x-w)*(fx-fv);
        q=(x-v)*(fx-fw);
        p=(x-v)*q-(x-w)*r;
        q=2.0*(q-r);
        if (q > 0.0) p = -p;
        q=fabs(q);
        etemp=e;
        e=d;
        if (fabs(p) >= fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x))
          d=CGOLD*(e=(x >= xm ? a-x : b-x));
        // The above conditions determine the acceptability of the 
        // parabolic fit. Here we
        // take the golden section step into the larger of the two segments.
        else {
          d=p/q; //Take the parabolic step.
          u=x+d;
          if (u-a < tol2 || b-u < tol2)
            d=SIGN(tol1,xm-x);
        }
      } else {
        d=CGOLD*(e=(x >= xm ? a-x : b-x));
      }
      u=(fabs(d) >= tol1 ? x+d : x+SIGN(tol1,d));
      fu=f->eval(u);
      // This is the one function evaluation per iteration.
      if (fu <= fx) { 
        // Now decide what to do with our function
        // evaluation. 
        if (u >= x) a=x; else b=x;
        SHFT(v,w,x,u); //Housekeeping follows:
        SHFT(fv,fw,fx,fu);
      } else {
        if (u < x) a=u; else b=u;
        if (fu <= fw || w == x) {
          v=w;
          w=u;
          fv=fw;
          fw=fu;
        } else if (fu <= fv || v == x || v == w) {
          v=u;
          fv=fu;
        }
      } //Done with housekeeping. Back for another iteration. 
    }
    throw string("Too many iterations in brent");
    *xmin=x; // Never get here.
    return fx;
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __alpha
#include <List.C>
#endif // __alpha

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

