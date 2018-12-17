// -*- C++ -*-  $Id: DataBase.C,v 2.1015 2018/12/15 23:12:04 jormal Exp $
// 
// Copyright 1998-2018 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <Analysis.h>
#include <SubSpace.h>
#include <DataSet.h>
#include <Feature.h>
#include <base64.h>

#ifdef HAVE_GCRYPT_H
#include <NGram.h>
#endif // HAVE_GCRYPT_H

#include <cox/lsc>

#include <videofile.h>

#include <locale>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif // HAVE_FCNTL_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_ASPELL_H
#include <aspell.h>
#endif // HAVE_ASPELL_H

#ifdef HAVE_WINSOCK2_H
#undef GetProp
#endif // HAVE_WINSOCK2_H

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

#if defined(HAVE_JAULA_H) && defined(PICSOM_USE_JAULA)
#include <jaula.h>
#endif // HAVE_JAULA_H && PICSOM_USE_JAULA

#include <boost/algorithm/string/replace.hpp>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string DataBase_C_vcid =
    "@(#)$Id: DataBase.C,v 2.1015 2018/12/15 23:12:04 jormal Exp $";

  // a special guest appearance...
  const string& object_info::db_name() const {
    static const string noname = "NONAME";
    return db ? db->Name() : noname;
  }

  int RandomNumber(int n) { return rand()%n; }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::force_compile    = false;

  bool DataBase::debug_locks      = false;
  bool DataBase::debug_gt         = false;
  bool DataBase::debug_gt_exp     = false;
  bool DataBase::debug_vobj       = false;
  bool DataBase::debug_tn         = false;
  bool DataBase::debug_origins    = false;
  bool DataBase::debug_opath      = false;
  bool DataBase::debug_dumpobjs   = false;
  bool DataBase::debug_classfiles = false;
  bool DataBase::debug_sql        = false;
  bool DataBase::debug_text       = false;

  size_t DataBase::debug_leafdir      = 0;
  size_t DataBase::debug_detections   = 0;
  size_t DataBase::debug_captionings  = 0;
  size_t DataBase::debug_features     = 0;
  size_t DataBase::debug_segmentation = 0;
  size_t DataBase::debug_images       = 0;

  bool DataBase::open_read_write_sql = false;
  bool DataBase::open_read_write_fea = false;
  bool DataBase::open_read_write_det = false;
  bool DataBase::open_read_write_txt = false;

  bool DataBase::force_lucene_unlock = false;

  bool DataBase::parse_external_metadata = true;

  bool DataBase::insert_mplayer          = false;
  bool DataBase::remove_broken_datafiles = false;

  bool DataBase::use_pthreads_features   = false;
  bool DataBase::use_pthreads_detection  = false;
  bool DataBase::mysql_library_initialized = false;

  DataBase::lscom_map_t DataBase::lscom_map;
  map<string,string> DataBase::imagenet_map;

  ontology DataBase::lscom_ontology;

  /////////////////////////////////////////////////////////////////////////////

  DataBase::DataBase(PicSOM *ps, const string& nin, const string& p,
		     const string& secpath, bool createxx, bool sqlxx) 
#ifdef PICSOM_USE_CONTEXTSTATE
    : context(NULL, NULL)
#endif // PICSOM_USE_CONTEXTSTATE
  {
    pic_som       = ps;
    permanentpath = p;
    path          = p;
    secondarypath = secpath;

    analysis_async_deprecated = NULL;

    string sqlite3db = SqliteDBFile(false);
    bool is_sqlite = (sqlxx && p.size()>8 && p.substr(p.size()-8)==".sqlite3")
      || FileExists(sqlite3db);

    sqlite_mode = is_sqlite ? OpenReadWriteSql() ? 2 : 1 : 0;
    mysql_mode  = 0;

    sqlobjects = sqlfeatures = sqlclasses = false;
    sqlindices = sqlsettings = sqlall     = false;
    if (DebugSql())
      cout << "DataBase::DataBase() *" << nin << "* SQL set to "
	   << (sqlxx?"on":"off") << endl;
#ifdef HAVE_SQLITE3_H
    sql3 = NULL;
    if (DebugSql())
      cout << "  sql3 = NULL" << endl;
#endif // HAVE_SQLITE3_H
#ifdef HAVE_MYSQL_H
    mysql = NULL;
    if (DebugSql())
      cout << "  mysql = NULL, sqlserver = " << Picsom()->SqlServer() << endl;
#endif // HAVE_MYSQL_H

#ifdef HAVE_MYSQL_H
    if (nin.find("mysql:")==0) {
      mysql_mode = OpenReadWriteSql() ? 2 : 1;
      if (DebugSql())
	cout << "  mysql_mode = " << mysql_mode << endl;
    }
#endif // HAVE_MYSQL_H

    labels  = "labels";
    origins = "origins";

    use_bin_features_read = use_bin_features_write = true;
    nofeaturedata = false;
    storedetections = "bin"; // good values: "no", "bin", "sql"

    LabelFormat("2+2+2+2");

    parent_view = NULL;

    longtext_xml = NULL;

    contentsname = ExpandPath("contents");
    features     = ExpandPath("features");

    // this is a cruel hack to prevent ExpandPath() from finding
    // the parent database's images subdirectory if this is a view database ...
    secondarypath = "";
    objdir = DirectoryExists(ExpandPath("images")) ? "images" : "objects";
    secondarypath = secpath;

    insertmode = insert_copy;
    original_size = startup_size = 0;

    videoinsert  = "file"; // file+frames+seconds+minutes+shots 
    videoextract = "file"; // file+frames+seconds+minutes+shots 

    insertobjectsrealinfo = true;
    insertallow_no_target = false;
    
    no_statistics        = false;
    created_from_scratch = false;
    textindices_solved   = false;

    allowalgorithmselection = false;
    allowconvolutionselection = false;
    allowcontentselection = false;
    allowinsertion = allowownimage = allowsegimage = allowtextquery = false;
    allowclassdefinition = allowstoreresults = false;
    allowobjecttypeselection = false;
    allowdisplaymodeselection = false;
    allowarrivedobjectnotification = false;
    allowimagescaling = true;

    deliverlocalcopies = false;
    visible = showsegmentations = true;
    debug_access = nondevelopmentuse = false;
    extractions_solved = false;
    all_classes_found_plain = all_classes_found_contents = false;
    use_context = false;
    write_changed_divfiles = true;
    tempmediafiles = tempimagefiles = true;
    overwritedetections = false;

    isdummydb               = false;
    has_origins             = true;
    wxh_reversed_in_origins = false;
    empty_labels_allowed    = false;
    ignore_leaf_features    = false;
    use_subobject_text      = false;
    mediaclips              = false;
    segment_is_modifier     = true;  // finally changed on 2011-09-06
    use_implicit_subobjects = false;
    extracted_files         = false;
    extractobjectpath       = "";
    extra_features_dir      = "";
    alwaysusetarfiles       = false;
    extractfulltars         = true;
    extractfullzips         = false;
    uselocalzipfiles        = false;

    defaultquery = new Query(Picsom());
    defaultquery->SetDefaultQueryIdentity();
    defaultquery->SetDataBase(this);
    defaultquery->Algorithm(cbir_picsom);
    defaultquery->SetAugmentValue(1.0);

    default_feature_target = target_no_target;

    ZeroTime(access_read_time);
    ZeroTime(sqlite_mod_time);

    ThumbnailType("file");

    framestep = 1;
    margin    = 0;

    InitializeDefaultAspects();

    string n = nin;
    size_t ppos = nin.find('(');
    if (ppos!=string::npos && *nin.rbegin()==')') {
      n.erase(ppos);
      vector<string> spec = SplitInCommas(nin.substr(ppos+1,
						     nin.size()-ppos-2));
      for (vector<string>::const_iterator i=spec.begin(); i!=spec.end(); i++) {
	pair<string,string> kv = SplitKeyEqualValueNew(*i);
	int res = 0;
	if (!Interpret(kv.first, kv.second, res) || !res)
	  ShowError("failed to parse ["+*i+"] in database <"+Name()+">");
      }
    }

    if (n.find("mysql:")==0)
      n.erase(0, 6);
    name = n;

    if (name.substr(0, 1)=="*")
      name = UserDbPrefix()+name.substr(1);

    if (sqlxx) {
      path = TempDir("", true);
      secondarypath = "";
      WriteLog("path=<"+path+">");
      TempDir("features");
    }

    features_expanded_xml = XmlDom::Doc();
    features_expanded_xml.Root("extractions");

    if (name=="")
      return;

    WriteLog("Opening "+string(createxx?"new ":"")+"database with"
	     " sql="+(OpenReadWriteSql()?"rw":"ro")+
	     " fea="+(OpenReadWriteFea()?"rw":"ro")+
	     " det="+(OpenReadWriteDet()?"rw":"ro"));

    stringstream pathss;
    pathss << "Path()=" << Path();

    struct stat st;
    if (!lstat(Path().c_str(), &st) && (st.st_mode&S_IFMT)==S_IFLNK) {
      char buf[100000] = "";
      ssize_t r = readlink(Path().c_str(), buf, sizeof(buf));
      if (r>=0) {
	buf[r] = 0;
	pathss << " really=" << buf;
      }
    }

    vector<string> cmd { "/bin/df", "-P", Path() };
    auto r = Picsom()->ShellExecute(cmd, true, false);
    if (r.first && r.second.size()>1) {
      vector<string> col = SplitInSpaces(r.second[1]);
      if (col.size()==6) {
	pathss << " filesystem=" << col[0];
	pathss << " mountpoint=" << col[5];
      }
    }
    WriteLog(pathss.str());

#ifdef PICSOM_USE_OD
    odone.Initialize(Picsom()->Quiet());
    odone.SetUsedDetector("sift");
    odone.SetUsedDescriptor("sift");
    odone.SetBlur(false);
    odone.SetShowKeypoints(false);
    odone.SetShowResults(false);
#endif // PICSOM_USE_OD

    bool use_sql_settings = SqlAll() || SqlSettings(), sql_ok = false;
    string tmpsfile, dbfile = SqliteDBFile(false);
      
#ifdef HAVE_SQLITE3_H
    if (SqliteMode() && FileExists(dbfile)) {
      if (Picsom()->TempSqliteDB()) {
	off_t fsize = FileSize(dbfile);
	string tempdbfile = SqliteDBFile(true);
	stringstream ss;
	ss << "<" << dbfile << "> to <" << tempdbfile << "> "
	   << fsize << " bytes (" << FileSizeHumanReadable(dbfile) << ")";
	struct timespec start = TimeNow();
 	if (!CopyFile(dbfile, tempdbfile)) {
	  ShowError("Copying "+ss.str()+" failed");
	  exit(1);
	}
	double took = TimeDiff(TimeNow(), start), speed = fsize/took;
	ss << " in " << took << " s (" << HumanReadableBytes(speed) << "/s)";
	WriteLog("Copied "+ss.str());
	vector<string> touch { "/bin/touch", "-r", dbfile, tempdbfile };
	Picsom()->ExecuteSystem(touch, true, true, true);
	dbfile = tempdbfile;
      }

      sql_ok = SqliteOpen(dbfile, false, SqliteMode()==2);
    }
#endif // HAVE_SQLITE3_H
#ifdef HAVE_MYSQL_H
    if (MysqlMode() && MysqlDataBaseExists(name, Picsom()->SqlServer()))
      sql_ok = MysqlOpen(name, false, MysqlMode()==2);
#endif // HAVE_MYSQL_H
      
    if (sql_ok && !FileExists(ExpandPath("settings.xml"))) {
      tmpsfile = TempFile("settings.xml");
      if (!SqlRetrieveFileToFile("settings.xml", tmpsfile)) 
	ShowError("SqlRetrieveFileToFile() <settings.xml> -> <"+tmpsfile
		  +"> failed");
      else {
	use_sql_settings = true;
	SqlUpdateFileList();
      }
    }

    string settings = use_sql_settings ? tmpsfile : ExpandPath("settings.xml");

    if (FileExists(settings)) {
      if (createxx) {
	ShowError("Settings file <"+settings+
		  "> exists though the database was being created");
	exit(1);
      }

      int set_mysql_mode = sqlxx && mysql_mode ? mysql_mode : -1;
      settingsfile = settings;
      ApplySettingsXML(true);
      if (set_mysql_mode>0) { // settings.xml may contain "old" usesqlite=yes
	mysql_mode  = set_mysql_mode;
	sqlite_mode = 0;
	if (DebugSql()) {
	  cout << "  mysql_mode = "  << mysql_mode  << " FORCED" << endl;
	  cout << "  sqlite_mode = " << sqlite_mode << " FORCED" << endl;
	}
      }

    } else if (createxx) {
      bool do_settings = true;
      bool do_labels   = do_settings && !SqlMode();
      bool do_features = do_settings && !SqlFeatures();
      bool do_objects  = do_settings && !SqlObjects();
      bool do_classes  = do_settings && !SqlClasses();

      CreateFromScratch(do_settings, do_labels, do_features,
			do_objects, do_classes);
      WriteLog("Settings file <"+string(settingsfile)+
	       "> was created");

    } else {
      CreateFromScratch(true, false, false, false, false);
      ShowError("Settings file <"+string(settingsfile)+
		"> did not exist though should and was created");
      exit(1);
    }

    if (tmpsfile!="")
      Unlink(tmpsfile);

    if (UseSql())
      labels = origins = "";

    // this _should_ come after ApplySettingsXML()...
    // but evaluating "defaultindices=..." may require labels to be known...
    /*
    if (SqliteMode() && !SqlAll()) {
      string dbfile = ExpandPath("sqlite3.db");
#ifdef HAVE_SQLITE3_H
      SqliteOpen(dbfile, false, SqliteMode()==2);
#endif // HAVE_SQLITE3_H
    }
    */

    SolveExtractions();
    SolveTextIndices();
    SolveUrlRules();

    if (Picsom()->IsServer() && !Picsom()->IsSlave())
      ApplyMainLoopArgs();

    // this logic broke the use of restca in picsom.ics.hut.fi...
    // if (Picsom()->Development()==false && nondevelopmentuse==false) {
    //   WriteLog("Non-development use denied");
    //   return;
    // }

    string labelsFileName = ExpandPath(labels);
    if (FileExists(labelsFileName))
      labelsfile = labelsFileName;

    if (Size()==0) // can be true if externalmetadata
      ReadLabels();
    
    if (!UseSql()) {
      ReadDuplicates(); //obs! doesn't work with SQL
      ReadSubobjects(); //obs! doesn't work with SQL
    }

    if (UseSql())
      SqlReadAliases();

    // so this is quite a mess with SQL and implicit subobjects...
    // but be advised that SQL subobjects are always _explicit_ (I think)
    if (use_implicit_subobjects || UseSql())
      MakeSubObjectIndex();

    ReadURLBlacklist();

    for (size_t kv=0; kv<postponed.size(); kv++)
      Interpret(postponed[kv]);
    postponed.clear();

    // this was finally commented out 2011-12-14
    // please add it where it belongs...
    // FindAllIndices();

    string cat2wn = ExpandPath("cat2wn.txt");
    if (FileExists(cat2wn))
      ReadCat2Wn(cat2wn);

    Picsom()->PossiblyShowDebugInformation("After DataBase construction");
  }

  /////////////////////////////////////////////////////////////////////////////

  DataBase::~DataBase() {
    Picsom()->PossiblyShowDebugInformation("Start of destructing "+Name());

    JoinAsyncAnalysesDeprecated();

    CloseLabelsFile(false);

    delete defaultquery;

    for (size_t i=0; i<class_models_new.size(); i++)
      delete class_models_new[i];

    access.FullDelete();

    Picsom()->PossiblyShowDebugInformation("Before deleting indices in "
					   +Name());
    // tssom.FullDelete();
    for (size_t i=0; i<Nindices(); i++)
      delete GetIndex(i);
    index.clear();

    bool tc_ok = CloseTextIndices();
    if (!tc_ok)
      WriteLog("Closing textindices failed");

    Picsom()->PossiblyShowDebugInformation("After deleting indices in "
					   +Name());
    if (longtext_xml)
      xmlFreeNode(longtext_xml);
  
    features_expanded_xml.DeleteDoc();
    settingsxml.DeleteDoc();

#ifdef HAVE_SQLITE3_H
    SqliteClose();
    if (name!="" && Picsom()->TempSqliteDB() && SqliteMode()==2) {
      string tempdbfile = SqliteDBFile(true), dbfile = SqliteDBFile(false);
      if (MoreRecent(tempdbfile, dbfile)) {
	stringstream ss;
	ss << "<" << tempdbfile << "> to <" << dbfile << "> "
	   << FileSize(tempdbfile) << " bytes";
	if (!CopyFile(tempdbfile, dbfile))
	  ShowError("Copying "+ss.str()+" failed");
	else
	  WriteLog("Copied "+ss.str());
      } else
	WriteLog("<"+tempdbfile+"> was not changed");
    }      
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    MysqlClose();
#endif // HAVE_MYSQL_H

#ifdef HAVE_ZIP_H
    for (auto i=zipfiles.begin(); i!=zipfiles.end(); i++) {
      if (zip_close(i->second)==0)
	WriteLog("Closed ZIP file "+ShortFileName(i->first));
      else
	ShowError("Failed to close ZIP file "+i->first);
    }
#endif // HAVE_ZIP_H

    Picsom()->PossiblyShowDebugInformation("Before svms.clear() in "+Name());

    // for (map<string,SVM*>::iterator i=svms.begin(); i!=svms.end(); i++)
    //  delete i->second;
    svms.clear();

    Picsom()->PossiblyShowDebugInformation("After svms.clear() in "+Name());

    if (!Picsom()->KeepTemp()) {
      UnlinkTempFiles();

      if (tempdirs.size()) {
	string tmpbase = *tempdirs.begin();
	if (PicSOM::DebugTempFiles())
	  cout << "Destructing temporary directory hierarchy <" << tmpbase
	       << ">" << endl;
	RmDirRecursive(tmpbase);
      }
    }

    if (bindetections.size())
      WriteLog("Closing "+ToStr(bindetections.size())+
	       " binary detection files");
    bindetections.clear();

    if (_objects.size())
      WriteLog("Clearing object data");
    _objects.clear();

    if (name!="")
      WriteLog("Database closed");

    Picsom()->PossiblyShowDebugInformation("End of destructing "+Name());
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::Dump(Simple::DumpMode /*dt*/, ostream& os) const {
    os << Bold("Database ")<< (void*)this
       << " name="  	      	 << ShowString(name)
       << " longname="             << ShowString(longname)
       << " shorttext="            << ShowString(shorttext)
       << " path="  	      	 << ShowString(path)
       << " secondarypath="     	 << ShowString(secondarypath)
       << " objdir="          	 << ShowString(objdir)
       << " features="          	 << ShowString(features)
       << " contentsname="      	 << ShowString(contentsname)
       << " thumbnailsize="        << ShowString(thumbnailsize)
       << " virtualimagesize="     << ShowString(virtualimagesize)
      //<< " virtualthumbnailsize=" << ShowString(virtualthumbnailsize)
       << " tn_type="              << ShowString(tn_type)
       << " insertmode="           << int(insertmode)
       << " restriction.Label()="  << restriction.Label()
       << " restriction.Length()=" << restriction.Length()
       << " debug_access="         << debug_access
       << " deliverlocalcopies="   << deliverlocalcopies
       << " visible="              << visible
       << " allowcontentselection="<< allowcontentselection
       << " allowinsertion="       << allowinsertion
       << " allowownimage="        << allowownimage
       << " allowsegimage="        << allowsegimage
       << " allowtextquery="       << allowtextquery
       << " allowclassdefinition=" << allowclassdefinition
       << " allowstoreresults="    << allowstoreresults
       << " showsegmentations="    << showsegmentations
       << " nondevelopmentuse="    << nondevelopmentuse
       << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ApplySettingsXML(bool no_labels_yet) {
    string hdr = "DataBase::ApplySettingsXML("+ToStr(no_labels_yet)+"): ";

    bool check = false;

    for (int lock=0; lock<2; lock++) {
      RwLockEither(lock, "ApplySettingsXML()");
      bool mod = settingsfile.Modified();
      if (!mod || !lock)
	RwUnlockEither(lock, "ApplySettingsXML()");
      if (!mod)
	return true;
    }

    if (longtext_xml)
      xmlFreeNode(longtext_xml);
    longtext_xml = NULL;

    bool ok = true;

    settingsxml.DeleteDoc();
  
    settingsxml = XmlDom::Parse(settingsfile);
    xmlNodePtr node = settingsxml.Root().FirstChild().node;

    for (; node; node=node->next) {
      if (node->type==XML_TEXT_NODE || node->type==XML_COMMENT_NODE)
	continue;
    
      string key = NodeName(node), val = GetXMLchildContent(node);
      StripWhiteSpaces(val);
    
      if (key=="longname") {
	if (check)
	  CheckEmptyText(val);
	longname = val;
	continue;
      }
    
      if (key=="shorttext") {
	if (check)
	  CheckEmptyText(val);
	shorttext = val;
	continue;
      }
    
      if (key=="longtext") {
	if (check)
	  CheckEmptyText(val);
	longtext_xml = xmlCopyNode(node, true);
	continue;
      }
    
      if (key=="access") {
	ApplyAccessRulesXML(node);
	continue;
      }

      if (key=="extraction" || key=="textindices" ||
	  key=="urlrules")
	continue;

      if (key=="set") {
	if (node->children)
	  ok = ShowError(hdr, "<set> is non-empty");
	
	xmlAttrPtr a = node->properties;
	if (!a)
	  ok = ShowError(hdr, "no key=value in <set/> ");

	for (; a; a = a->next)
	  ok &= InterpretOrPostpone(AttrName(a), AttrContent(a), no_labels_yet);

	continue;
      }

      if (key=="defaultaspects") {
	ApplyDefaultAspectsXML(node);
	continue;
      }
 
      if (key=="gt") {
	SetGroundTruth(node);
	continue;
      }

      if (key=="featurealias") {
	SetFeatureAlias(node);
	continue;
      }

      if (key=="featureaugmentation") {
	SetFeatureAugmentation(node);
	continue;
      }

      if (key=="namedcmdline") {
	SetNamedCmdLine(node);
	continue;
      }

      if (key=="textquery-option") {
	SetTextQueryOption(node);
	continue;
      }

      if (key=="classaugment") {
	SetClassAugment(node);
	continue;
      }

      if (key=="externalmetadata") {
	if (ParseExternalMetadata()) {
	  XmlDom ele(settingsxml.doc, settingsxml.ns, node);
	  map<string,string> par = ele.PropertyMap();
	  string type = par["type"];
	  if (!ReadExternalMetaData(type, par))
	    ShowError(hdr+"ReadExternalMetaData() failed");
	}
	continue;
      }

      if (key=="phase_rule") {
	string aspect, from, to;
	for (xmlAttrPtr a = node->properties; a; a = a->next)
	  if (AttrName(a)=="aspect")
	    aspect = AttrContent(a);
	  else if (AttrName(a)=="from")
	    from = AttrContent(a);
	  else if (AttrName(a)=="to")
	    to = AttrContent(a);

	ok &= defaultquery->AddPhaseRule(aspect, from, to);
	continue;
      }

      if (key=="macros") {
	XmlDom m(node->doc, node->ns, node);
	XmlDom n = m.FirstElementChild("macro");
	for (;;) {
	  if (!n)
	    break;

	  string name = n.Property("name");
	  string cont = n.FirstChildContent();
	  macro[name] = cont;

	  n = n.NextElement("macro");
	}
	continue;
      }

      if (key=="linked_data_mappings") {
	XmlDom m(node->doc, node->ns, node);
	XmlDom n = m.FirstElementChild("linked_data_mapping");
	for (;;) {
	  if (!n)
	    break;

	  string toc  = n.Property("sparq");
	  string stoc = "sparq/"+toc;

	  map<string,string> kv;
	  XmlDom r = n.FirstElementChild("mapping");
	  for (;;) {
	    if (!r)
	      break;

	    string src = r.Property("src");
	    string dst = r.Property("dst");
	    kv[dst] = src;

	    if (Query::DebugInfo())
	      cout << stoc << " " << dst << " " << src << endl;

	    r = r.NextElement("mapping");
	  }
	  if (Query::DebugInfo())
	    cout << endl;

	  if (linked_data_mappings.find(stoc)==linked_data_mappings.end())
	    linked_data_mappings[stoc] = list<map<string,string> >();
	  linked_data_mappings[stoc].push_back(kv);

	  n = n.NextElement("linked_data_mapping");
	}
	continue;
      }

      if (val!="" || key=="defaultfeatures" || key=="defaultindices" ||
	   key=="erf_detection_images") {
	ok &= InterpretOrPostpone(key, val, no_labels_yet);
	continue;
      }

      if (node->properties &&
	  !xmlStrcmp(node->properties->name, (XMLS)"value")) {
	xmlAttrPtr a = node->properties;
	XMLS valstrp = a->children ? a->children->content : NULL;
	string valstr = valstrp ? (const char*)valstrp : "";
	ok &= InterpretOrPostpone(key, valstr, no_labels_yet);
	continue;
      }

      ok = ShowError(hdr+"\""+key+"\"=\""+val+"\" not understood");
    }
    
    // xmlFreeDoc(doc);
    settingsfile.WasRead();

    extractions_solved = false;
    index_to_featurefile.clear();
    featurefile_to_method.clear();
    described_segmentations.clear();

    WriteLog("Settings read from <", settingsfile, ">");
    WriteLog("Format of labels is "+LabelFormatStr(true));
    WriteLog(string("Using binary features read=")
	     +string(use_bin_features_read?"yes":"no")+" write="
	     +string(use_bin_features_write?"yes":"no"));

    RwUnlockWrite("ApplySettingsXML()");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InterpretOrPostpone(const string& k, const string& v,
				     bool nly) {
    bool pp = nly;

    if (pp) {
      static set<string> ppset;
      if (ppset.empty()) {
	ppset.insert("restriction");
	ppset.insert("dbrestriction");
	ppset.insert("classificationset");
	ppset.insert("reverse");
	ppset.insert("defaultindices");  // added 2011-09-06
	ppset.insert("defaultfeatures"); // added 2011-09-06
	ppset.insert("faces");
	ppset.insert("objects");
	ppset.insert("locations");
      }
      if (ppset.find(k)==ppset.end())
	pp = false;
    }
  
    if (!pp) {
      if (k.find("debug")==0) {
	int r = 0;
	if (Picsom()->Interpret(k, v, r) && r)
	  return true;
      }

      return Interpret(k, v);
    }

    postponed.push_back(make_pair(k, v));

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ApplyAccessRulesXML(xmlNodePtr l) {
  access.Delete();

  for (xmlNodePtr node = l ? l->children : NULL; node; node = node->next) {
    if (!node->children)
      continue;

    if (!xmlStrcmp(node->name, (XMLS)"rule")) {
      //ShowError("DataBase::ApplyAccessRulesXML() doesn't process rules yet");
      if (!node->children) {
	ShowError("DataBase::ApplyAccessRulesXML() <rule> without children");
	continue;
      }
      ProcessAccessEntryXML(node);
    }
    else if (!xmlStrcmp(node->name, (XMLS)"oldrule")) {
      if (!node->children || !node->children->content) {
	ShowError("DataBase::ApplyAccessRulesXML() <oldrule> without content");
	continue;
      }
      
      string tmp = NodeChildContent(node);
      StripWhiteSpaces(tmp);
      if (tmp[0]!='#')
	ProcessAccessEntry(tmp.c_str());
    }
    else 
      ShowError("DataBase::ApplyAccessRulesXML() entry <",
		(const char*)node->name, "> not understood");
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ApplyDefaultAspectsXML(xmlNodePtr l) {
  defaultaspects.clear();
  defaultaspects_cache.clear();

  for (xmlNodePtr node = l ? l->children : NULL; node; node = node->next) {
    if (!node->children)
      continue;

    if (NodeName(node)=="aspects")
      ProcessAspectsEntryXML(node);
    else 
      ShowError("DataBase::ApplyDefaultAspectsXML() entry <", NodeName(node),
		"> not understood");
  }

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CreateFromScratch(bool do_settings, bool do_labels,
				   bool do_features, bool do_objects,
				   bool do_classes) {
    string msg = "CreateFromScratch() : ";

    created_from_scratch = true;

    if (SqlMode()==1)
      return ShowError(msg+"cannot be SQLmode==1, use -rw=...sql...");

    if (!SqlAll())
      Picsom()->MakeDirectory(path); // was in PicSOM::FindDataBaseEvenNew()

    if (do_settings && !SqlSettings()) {
      string settingsfilename = ExpandPath("settings.xml");
      if (!FileExists(settingsfilename)) {
	CopyOrCreateSettingsXml(settingsfilename);
	SetFilePermissions(settingsfilename);
	
      } else {
	settingsfile = settingsfilename;
	ApplySettingsXML(true);
      }
    }

    if (do_labels && !SqlObjects()) {
      string lfile = ExpandPath(labels);
      if (!FileExists(lfile)) {
	ofstream labels(lfile.c_str());
	labels.close();
	WriteLog("Created empty labels file <", lfile, ">");
	SetFilePermissions(lfile);
      }

      string nflfile = ExpandPath("next-free-label");
      if (!FileExists(nflfile) && Path()!=".") {
	string zero = "00000000000000000000000000";
	string lab0 = zero.substr(0, LabelLength()) ;
	ofstream nextfree(nflfile.c_str());
	nextfree << lab0 << endl;
	nextfree.close();
	WriteLog("Created next-free-label file <"+nflfile+"> with ["+lab0+"]");
	SetFilePermissions(nflfile);
      }
    }

    if (Path()!=".") {
      if (do_features && !SqlIndices()) {
	string fdir = ExpandPath("features");
	Picsom()->MakeDirectory(fdir);
      }

      if (do_objects && !SqlObjects()) {
	string idir = ExpandPath(objdir);
	Picsom()->MakeDirectory(idir);
      }

      if (do_classes && !SqlClasses()) {
	string cdir = ExpandPath("classes");
	Picsom()->MakeDirectory(cdir);
      }
    }

    bool ok = true;

    if (MysqlMode()==2) {
#ifdef HAVE_MYSQL_H
      ok = CreateFromScratchMysql(do_settings, do_labels, do_features,
				  do_objects, do_classes);
#else
      return ShowError(msg+"mysql not available");
#endif // HAVE_MYSQL_H
    }
      
    if (SqliteMode()==2) {
#ifdef HAVE_SQLITE3_H
      ok = CreateFromScratchSqlite(do_settings, do_labels, do_features,
				   do_objects, do_classes);
#else
      ok = ShowError(msg+"sqlite3 not available");
#endif // HAVE_SQLITE3_H
    }

    return ok && SolveExtractions() && SolveTextIndices();
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  bool DataBase::CreateFromScratchSqlite(bool /*do_settings*/, bool do_labels,
					 bool do_features, bool do_objects,
					 bool do_classes) {
    string msg = "DataBase::CreateFromScratchSqlite() : ";

    string f = SqliteDBFile(Picsom()->TempSqliteDB());

    if (!SqliteOpen(f, true, true))
      return ShowError(msg+"failed to open <"+f+">");

    stringstream cmdss1;
    SqlDumpVersion(cmdss1, "sqlite3");
    string cmd = cmdss1.str();
    if (DebugSql())
      cout << TimeStamp() << msg << cmd << endl;

    if (!SqliteExec(cmd, true)) {
      string why = SqliteErrMsg();
      ShowError(msg+"SqliteExec() failed with ["+cmd+"] : "+why);
      SqliteClose();
      return false;
    }

    stringstream cmdss2;
    SqlDumpFiles(cmdss2, "sqlite3");
    cmd = cmdss2.str();
    if (DebugSql())
      cout << TimeStamp() << msg << cmd << endl;

    if (!SqliteExec(cmd, true)) {
      string why = SqliteErrMsg();
      ShowError(msg+"SqliteExec() failed with ["+cmd+"] : "+why);
      SqliteClose();
      return false;
    }

    if (SqlSettings()) {
      string tmpsfile = TempFile("settings.xml", false);
      CopyOrCreateSettingsXml(tmpsfile);
      bool ok = SqlStoreFileFromFile("settings.xml", tmpsfile);
      // Unlink(tmpsfile); // commented out 2017-08-16
      if (!ok) {
	ShowError(msg+"SqlStoreFileFromFile() failed");
	SqliteClose();
	return false;
      }
    }

    if (true || !do_labels) {
      stringstream cmdss;
      SqlDumpObjects(cmdss, "sqlite3");
      string cmd = cmdss.str();
      if (!SqliteExec(cmd, true)) {
	string why = SqliteErrMsg();
	ShowError(msg+"SqliteExec() failed with ["+cmd+"] : "+why);
	SqliteClose();
	return false;
      }
    }

    SqlCreateTable("aliases", "keyz UNIQUE,value UNIQUE");
    
    SqlCreateTable("segmentations", "indexz INTEGER,method,"
		   "moddate BLOB(16),insdate BLOB(16),user,"
		   "data BLOB");

    SqlCreateTable("history", "startdate BLOB(16),enddate BLOB(16),"
		   "user,comment,script,args");

    if (!do_features) {
    }

    if (!do_objects) {
    }

    if (!do_classes) {
    }
    
    return true; // used to be SqliteClose() why?
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  string DataBase::SqliteDBstr(const string& spec) {
    string msg = "DataBase::SqliteDBstr("+spec+") : ";

    string f = ExpandPath("sqlite3.db");
    if (f=="") {
      ShowError(msg+"no sqlite3.db file");
      return "";
    }
    if (FileSize(f)<1) {
      f = ConvertGlobalToLocalDiskName(f);
      // cout << "[[[" << f << "]]]" << endl;
    }

    return FileToString(f);
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  bool DataBase::CreateFromScratchMysql(bool, bool, bool, bool, bool) {
    string msg = "DataBase::CreateFromScratchMySql() : ";

    string dbname = name;
    if (dbname.find("mysql:")==0)
      dbname.erase(0, 6);

    if (!MysqlOpen(dbname, true, true))
      return ShowError(msg+"failed to open <"+dbname+">");

    stringstream cmdss1;
    SqlDumpVersion(cmdss1, "mysql");
    string cmd = cmdss1.str();
    if (!MysqlQuery(cmd, true)) {
      string why = MysqlErrMsg(mysql);
      ShowError(msg+"MysqlQuery() failed with ["+cmd+"] : "+why);
      MysqlClose();
      return false;
    }

    stringstream cmdss2;
    SqlDumpFiles(cmdss2, "mysql");
    cmd = cmdss2.str();
    if (DebugSql())
      cout << TimeStamp() << msg << cmd << endl;

    if (!MysqlQuery(cmd, true)) {
      string why = MysqlErrMsg(mysql);
      ShowError(msg+"MysqlQuery() failed with ["+cmd+"] : "+why);
      MysqlClose();
      return false;
    }

    string tmpsfile = TempFile("settings.xml", false);
    CopyOrCreateSettingsXml(tmpsfile);
    bool ok = SqlStoreFileFromFile("settings.xml", tmpsfile);
    Unlink(tmpsfile);
    if (!ok) {
      ShowError(msg+"SqlStoreFileFromFile() failed");
      MysqlClose();
      return false;
    }

    stringstream cmdss3;
    SqlDumpObjects(cmdss3, "mysql");
    cmd = cmdss3.str();
    if (!MysqlQuery(cmd, true)) {
      string why = MysqlErrMsg(mysql);
      ShowError(msg+"MysqlQuery() failed with ["+cmd+"] : "+why);
      MysqlClose();
      return false;
    }

    SqlCreateTable("aliases", "keyz VARCHAR(10) UNIQUE,"
		   "value VARCHAR(200) UNIQUE");
    
    SqlCreateTable("segmentations", "indexz INTEGER,method VARCHAR(10),"
		   "moddate BINARY(16),insdate BINARY(16),user VARCHAR(8),"
		   "data LONGBLOB");

    SqlCreateTable("history", "startdate BINARY(16),enddate BINARY(16),"
		   "user VARCHAR(8),comment TEXT,script TEXT,args TEXT");

    return true;  // used to be MysqlClose();
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CopyOrCreateSettingsXml(const string& file) {
    if (settings=="")
      return CreateNewSettingsXml(file);

    if (!CopyFile(settings, file))
      return ShowError("DataBase::CopyOrCreateSettingsXml() : "
		       "failed to copy <"+settings+"> to <"+file+">");

    WriteLog("Copied <"+settings+"> to <"+file+">");

    settingsfile = file;

    return ApplySettingsXML(true);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CreateNewSettingsXml(const string& file) {
    settingsxml.DeleteDoc();

    settingsxml = XmlDom::Doc();
    XmlDom db = settingsxml.Root("database");

    string nstr = name;
    if (nstr.find(UserDbPrefix())==0)
      nstr.erase(0, UserDbPrefix().size());
    nstr += " WRITE HERE ";

    string lntmp = longname!=""  ? longname  : nstr+"1-3 WORDS";
    string sttmp = shorttext!="" ? shorttext : nstr+"5-10 WORDS";
    string lttmp = shorttext!="" ? shorttext : nstr+"10-40 WORDS of XHTML";

    db.Element("longname",  lntmp);
    db.Element("shorttext", sttmp);
    db.Element("longtext",  lttmp);

    db.Element("labelformat", LabelFormatStr(false));
    if (labelimport!="")
      db.Element("labelimport", labelimport);

    if (externalmetadatatype!="") {
      XmlDom emd = db.Element("externalmetadata");
      emd.Prop("type", externalmetadatatype);
    }

    db.Element("usesqlite", sqlite_mode?"yes":"no");
    db.Element("usemysql",   mysql_mode?"yes":"no");

    db.Element("sqlobjects",   SqlObjects()?"yes":"no");
    db.Element("sqlfeatures", SqlFeatures()?"yes":"no");
    db.Element("sqlclasses",   SqlClasses()?"yes":"no");
    db.Element("sqlindices",   SqlIndices()?"yes":"no");
    db.Element("sqlsettings", SqlSettings()?"yes":"no");

    db.Element("visible",             	  "yes");
    db.Element("deliverlocalcopies",  	  "yes");
    db.Element("segment_is_modifier", 	  "yes");
    db.Element("use_implicit_subobjects", "no");
    db.Element("use_bin_features_read",   use_bin_features_read?"yes":"no");
    db.Element("use_bin_features_write", use_bin_features_write?"yes":"no");
    db.Element("storedetections", storedetections);

    db.Element("videoinsert",  	    videoinsert);
    db.Element("videoextract", 	    videoextract);
    db.Element("mediaclips",   	    mediaclips?"yes":"no");
    db.Element("alwaysusetarfiles", alwaysusetarfiles?"yes":"no");

    db.Element("algorithm",           "picsom_bottom");
    db.Element("defaultindices",      "*");
    db.Element("convtype",            "triangle-1-2-4-8");
    db.Element("permapobjects",       "500");

    db.Element("erf_detection_images", "");

    AddExtraction(db);

    db.Element("allowalgorithmselection",       "yes");
    db.Element("allowcontentselection",         "yes");
    db.Element("allowobjecttypeselection",      "yes");
    db.Element("allowdisplaymodeselection",     "yes");
    db.Element("allowinsertion",                "yes");
    db.Element("allowownimage",                 "yes");
    db.Element("allowtextquery",                "no");
    db.Element("allowclassdefinition",          "no");
    db.Element("allowarrivedobjectnotification","no");
    db.Element("allowimagescaling",             "yes");

    XmlDom a = db.Element("access");
    a.Element("oldrule", "130.233.173.0/24 130.233.173.0/24");

    if (Path()!=".") {
      settingsxml.Write(file, true, false);
      WriteLog("Created empty settings.xml file <", file, ">");
    }

    settingsfile = file;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadCat2Wn(const string& f) {
    string msg = "DataBase::ReadCat2Wn("+f+") : ";

    string key = f;
    size_t p = key.rfind('/');
    if (p!=string::npos)
      key.erase(0, p+1);
    p = key.rfind('.');
    if (p!=string::npos)
      key.erase(p);
    if (key=="cat2wn")
      key = "";

    cat2wn mmm;

    set<string> tset, sset;
    size_t n = 0;
    string fs = FileToString(f);
    vector<string> ll = SplitInSomething("\n", true, fs);
    for (auto l=ll.begin(); l!=ll.end(); l++) {
      string aa = *l;
      StripWhiteSpaces(aa);
      if (aa=="" || aa[0]=='%')
	continue;

      size_t p = aa.find("->");
      if (p==string::npos)
	return ShowError(msg+"failed with missing -> in ["+aa+"]");

      string t = aa.substr(0, p);
      StripWhiteSpaces(t);
      string s = aa.substr(p+2);
      p = s.find('%');
      if (p!=string::npos)
	s.erase(p);
      StripWhiteSpaces(s);

      vector<string> tt = SplitInSpaces(t);
      vector<string> ss = SplitInSpaces(s);
      for (auto ti=tt.begin(); ti!=tt.end(); ti++)
	for (auto si=ss.begin(); si!=ss.end(); si++) {
	  // cout << "[" << *ti << "] = [" << *si << "]" << endl;
	  mmm.cmap.insert(make_pair(*ti, *si));
	  tset.insert(*ti);
	  sset.insert(*si);
	  n++;
	}
    }

    cat2wm_map[key] = mmm;

    WriteLog("Read cat2wn data from <"+ShortFileName(f)+"> where "+
	     ToStr(tset.size())+" categories matched "+
	     ToStr(sset.size())+" synsets in "+ToStr(n)+" rules");	     

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  int DataBase::LabelIndex(const string& l) const {
    if (isdummydb)
      return 0;

    if (l.empty()) {
      ShowError("DataBase <"+name+"> LabelIndex(\"\") failed");
      return -1;
    }

    int r = -1;
    if (l[0]=='#') {
      if (l.size()>1 && l.find_first_not_of("0123456789", 1)==string::npos) {
	r = atoi(l.c_str()+1);
	if (r>=(int)Size())
	  r = -1;
      }

      if (l.find("#last")==0 && Size()) {
	r = Size()-1;
	string ot = l.substr(5);
	if (ot!="") {
	  target_type tt = TargetType(ot);
	  if (tt==target_error || tt==target_no_target)
	    r = -1;
	  while (r>=0 && !ObjectsTargetTypeContains(r, tt))
	    r--;
	}
      }
      if (r==-1)
	ShowError("DataBase <"+name+"> LabelIndex(", l, ") failed");

      return r;
    }

    Tic("LabelIndex");
    try {
      r = _objects.index(l);
    }
    catch (const out_of_range&) {
      ShowError("DataBase <"+name+"> LabelIndex(", l, ") failed");
    }
    Tac("LabelIndex");

    return r; 
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddExtraction(XmlDom& xml) {
    xmlNodePtr db = xml.node;
    xmlNsPtr   ns = xml.ns;

    string hdr = "DataBase::AddExtraction() : ";
    bool debug = DebugFeatures();

    if (debug)
      cout << hdr+"creating XML internally" << endl;

    vector<string> cmd;
    cmd.push_back("*picsom-features-internal*");
    cmd.push_back("-lx");
    list<incore_feature_t> incore;
    feature_result feat_res;
    int r = Feature::Main(cmd, incore, &feat_res);
    xmlDocPtr doc = r==0 ? feat_res.xml.doc : NULL;
  
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (NodeName(root)!="featurelist" || !IsElement(root)) {
      xmlFreeDoc(doc);
      return ShowError(hdr, "failed to find <featurelist>");
    }

    if (!root->children) {
      xmlFreeDoc(doc);
      return ShowError(hdr, "empty <featurelist> A");
    }
  
    static set<string> banned;
    if (banned.empty()) {
      banned.insert("MPEG7-ContourShape");
      banned.insert("MPEG7-HomogeneousTexture");
    }

    xmlNodePtr df = AddTag(db, ns, "extraction");

    for (xmlNodePtr node = root->children; node; node=node->next)
      if (IsElement(node)) {
	string exttype = NodeName(node);
	if (exttype=="feature") {
	  xmlNodePtr f = xmlCopyNode(node, 1);
	  string name = GetProperty(node, "name");
	  bool yes = name!="" && banned.find(name)==banned.end();
	  SetProperty(f, "use", yes ? "yes" : "no");
	  xmlAddChild(df, f);

	  if (debug)
	    cout << NodeName(f) << " name=\"" << GetProperty(f, "name")
		 << "\" outputname=\"" << GetProperty(f, "outputname")
		 << "\" target=\"" << GetProperty(f, "target")
		 << "\" hasrawin=\"" << GetProperty(f, "hasrawin")
		 << "\" hasrawout=\"" << GetProperty(f, "hasrawout")
		 << "\" segmentation=\"" << GetProperty(f, "segmentation")
		 << "\"" << endl;

	} else if (exttype=="segmentation") {
	  // these structures are not yet generated
	}
      }

    xmlFreeDoc(doc);

    // would it be better to have a template file .../picsom/share/xxx.xml ?
    string spec = extractionspec;
    while (1) {
      size_t p = spec.find('+');
      if (p==string::npos)
	break;
      spec[p] = ',';
    }
    vector<string> espec = SplitInCommasObeyParentheses(spec);
    for (size_t i=0; spec!="" && i<espec.size(); i++) {
      XmlDom xml = XmlDom::Doc();
      xmlNodePtr f = NULL;
      if (espec[i]=="csds-s90-2") {
	XmlDom node = xml.Root("feature");
	node.Prop("name", "ColorSIFT");
	node.Prop("outputname", "ColorSIFTds-soft90-1x1-2x2");
	node.Prop("target", "image");
	node.Prop("hasrawin", "no");
	node.Prop("hasrawout", "yes");
	node.Prop("switches", "-Z1 -cn -NColorSIFTds-soft90-1x1-2x2");
	node.Prop("options", "-o detector=densesampling "
		  "-o codebook=ColorSIFT/trecvid2010-random100dense.cdb.tab "
		  "-o pointselector=pyramid-1x1-2x2 "
		  "-o softsigma=90 "
		  "-o codebookdim=5000");
	f = xmlCopyNode(node.node, 1);
      }
      if (espec[i]=="svm-csds-3lscom") {
	XmlDom node = xml.Root("detection");
	node.Prop("name", "svm-csds-3lscom");
	node.Prop("type", "svmpred");
	node.Prop("target", "image");
	node.Prop("database", "@svmdb");
	node.Prop("params", "kernel_type=CHISQUARED");
	node.Prop("features", "ColorSIFTds-soft90-1x1-2x2");
	node.Prop("class", "lscom001,lscom002,lscom003");
	//node.Prop("class", "lscom001,lscom002,lscom003");
	f = xmlCopyNode(node.node, 1);
      }
      xml.DeleteDoc();

      if (f) {
	SetProperty(f, "use", "yes");
	xmlAddChild(df, f);
      } else
	return ShowError(hdr, "extraction specifier ["+espec[i]+"] failed");
    }

    if (!df->children)
      return ShowError(hdr, "empty <featurelist> B");

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ApplyMainLoopArgs() {
  bool ok = true;

  const vector<string>& al = Picsom()->CmdLineArgs();
  for (vector<string>::const_iterator i=al.begin(); i!=al.end(); i++) {
    pair<string,string> keyval = SplitKeyEqualValueNew(*i);
    ok &= InterpretOrPostpone(keyval.first, keyval.second, true);
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

void DataBase::CheckEmptyText(const string& txt) {
  if (txt.find(" WRITE HERE ")!=string::npos && !created_from_scratch)
    WarnOnce("Database <"+Name()+"> has uninitialized settings.xml file");
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::Interpret(const string& k, const string& v) {
    int res = 0;
    bool ok = Interpret(k, v, res);
    if (!ok || !res) {
      // BackTraceMeHere(cerr);
      ShowError("DataBase::Interpret() : [", k, "]=[", v, "] failed");
      return false;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::Interpret(const string& key, const string& val,
			   int& res) {
    const string h = "DataBase::Interpret() : ";
    bool ret = true;
    res = 1;

    int intval = atoi(val.c_str());

    /*>> database_interpret
      secondarypath
      *DOCUMENTATION MISSING* */
    
    if (key=="database")
      ret = ShowError("database="+val+" encountered in DataBase::Interpret()");

    else if (key=="longname")
      longname = val;

    else if (key=="shorttext")
      shorttext = val;

    else if (key=="extractions")
      extractionspec = val;

    else if (key=="storedetections")
      storedetections = val;

    else if (key=="showdetections")
      showdetections = val;

    else if (key=="labels")
      labels = val;

    else if (key=="labelformat") {
      if (!LabelFormat(val))
	ShowError("labelformat failed");
      
    } else if (key=="labelimport")
      labelimport = val;

    else if (key=="settings")
      settings = val;

    else if (key=="usemysql") {
      if (IsAffirmative(val)&&!HasMysql())
	ret = ShowError("usemysql=true impossible without MySql");

      mysql_mode = IsAffirmative(val) ? OpenReadWriteSql() ? 2 : 1 : 0;
      if (DebugSql())
	cout << h << "mysql_mode = " << mysql_mode << endl;

    } else if (key=="usesqlite") {
      if (IsAffirmative(val)&&!HasSqlite())
	ret = ShowError("usesqlite=true impossible without Sqlite3");

      sqlite_mode = IsAffirmative(val) ? OpenReadWriteSql() ? 2 : 1 : 0;
      if (DebugSql())
	cout << h << "sqlite_mode = " << sqlite_mode << endl;

    } else if (key=="sqlobjects") {
      sqlobjects = IsAffirmative(val);
      if (DebugSql())
	cout << h << "sqlobjects = " << sqlobjects << endl;
      if (sqlobjects && !UseSql())
	ret = ShowError("sqlobjects==true while UseSql()==false");

    } else if (key=="sqlfeatures") {
      sqlfeatures = IsAffirmative(val);
      if (DebugSql())
	cout << h << "sqlfeatures = " << sqlfeatures << endl;
      if (sqlfeatures && !UseSql())
	ret = ShowError("sqlfeatures==true while UseSql()==false");

    } else if (key=="sqlclasses") {
      sqlclasses = IsAffirmative(val);
      if (DebugSql())
	cout << h << "sqlclasses = " << sqlclasses << endl;
      if (sqlclasses && !UseSql())
	ret = ShowError("sqlclasses==true while UseSql()==false");

    } else if (key=="sqlindices") {
      sqlindices = IsAffirmative(val);
      if (DebugSql())
	cout << h << "sqlindices = " << sqlindices << endl;
      if (sqlindices && !UseSql())
	ret = ShowError("sqlindices==true while UseSql()==false");

    } else if (key=="sqlsettings") {
      sqlsettings = IsAffirmative(val);
      if (DebugSql())
	cout << h << "sqlsettings = " << sqlsettings << endl;
      if (sqlsettings && !UseSql())
	ret = ShowError("sqlsettings==true while UseSql()==false");

    } else if (key=="sqlall") {
      sqlall = IsAffirmative(val);
      if (DebugSql())
	cout << h << "sqlall = " << sqlall << endl;
      if (sqlall) {
	if (!UseSql())
	  ret = ShowError("sqlall==true while UseSql()==false");
	else
	  sqlobjects = sqlfeatures = sqlclasses =
	    sqlindices = sqlsettings = true;
      }

    } else if (key=="externalmetadata")
      externalmetadatatype = val;

    else if (key=="frag")
      frag = val;

    else if (key=="*visualgenome*") {
      map<string,string> param = externalmetadata_param;
      if (frag!="")
	param["frag"] = frag;
      WriteLog("Calling ReadVisualGenome()");
      res = ReadVisualGenome(param);
      WriteLog("Done ReadVisualGenome()");

    } else if (key=="secondarypath")
      secondarypath = ExpandPath(val);
      
    else if (key=="features")
      features = ExpandPath(val);
      
    else if (key=="contents")
      contentsname = ExpandPath(val);

    else if (key=="thumbnailtype")
      ThumbnailType(val);

    else if (key=="thumbnailmime")
      thumbnailmime = val;

    else if (key=="thumbnailsize")
      thumbnailsize = val;

    else if (key=="virtualimagesize")
      virtualimagesize = val;

    else if (key=="virtualthumbnailsize") {
      vector<string> vv = SplitInSomething("/", false, val);
      string c = vv.size()>1 ? vv[1] : "";
      virtualthumbnailsize.push_back(make_pair(c, vv[0]));
    }

    else if (key=="insertmode")
      insertmode = InsertModeName(val);

    else if (key=="insertobjectsrealinfo")
      insertobjectsrealinfo = IsAffirmative(val);

    else if (key=="insertallow_no_target")
      insertallow_no_target = IsAffirmative(val);

    else if (key=="isdummydb")
      isdummydb = IsAffirmative(val);

    else if (key=="debugaccess")
      debug_access = IsAffirmative(val);

    else if (key=="featuredebug" || key=="debugfeatures")
      DebugFeatures(intval);

    else if (key=="segmentationdebug" || key=="debugsegmentation")
      DebugSegmentation(intval);

    else if (key=="imagedebug" || key=="debugimages")
      DebugImages(intval);

    else if (key=="videoinsert")
      videoinsert = val;

    else if (key=="videoextract")
      videoextract = val;

    else if (key=="deliverlocalcopies")
      deliverlocalcopies = IsAffirmative(val);

    else if (key=="visible")
      visible = IsAffirmative(val);

    else if (key=="allowcontentselection")
      allowcontentselection = IsAffirmative(val);

    else if (key=="allowalgorithmselection")
      allowalgorithmselection = IsAffirmative(val);

    else if (key=="allowconvolutionselection")
      allowconvolutionselection = IsAffirmative(val);

    else if (key=="allowinsertion")
      allowinsertion = IsAffirmative(val);

    else if (key=="allowownimage")
      allowownimage = IsAffirmative(val);

    else if (key=="allowsegimage")
      allowsegimage = IsAffirmative(val);

    else if (key=="allowtextquery" || key=="allowtextsearch")
      allowtextquery = IsAffirmative(val);

    else if (key=="allowclassdefinition")
      allowclassdefinition = IsAffirmative(val);

    else if (key=="allowstoreresults")
      allowstoreresults = IsAffirmative(val);

    else if (key=="allowobjecttypeselection")
      allowobjecttypeselection = IsAffirmative(val);

    else if (key=="allowdisplaymodeselection")
      allowdisplaymodeselection = IsAffirmative(val);

    else if (key=="allowarrivedobjectnotification")
      allowarrivedobjectnotification = IsAffirmative(val);

    else if (key=="allowimagescaling")
      allowimagescaling = IsAffirmative(val);

    else if (key=="showsegmentations")
      showsegmentations = IsAffirmative(val);

    else if (key=="nondevelopmentuse")
      nondevelopmentuse = IsAffirmative(val);

    else if (key=="context")
      use_context = IsAffirmative(val);

    else if (key=="write_changed_divfiles")
      write_changed_divfiles = IsAffirmative(val);

    else if (key=="dbrestriction")
      Restriction(val);

    else if (key=="dbrestrictionsilent")
      Restriction(val, false);

    else if (key=="restriction")  // obs! this should be deprecated
      Restriction(val);

    else if (key=="faces")
      RestrictionInner(face_restriction, "faces", val, true,
		       target_image, -1, false);

    else if (key=="objects")
      RestrictionInner(object_restriction, "objects", val, true,
		       target_image, -1, false);

    else if (key=="locations")
      RestrictionInner(location_restriction, "locations", val, true,
		       target_image, -1, false);

    else if (key=="facebox")
      facebox = val;

    else if (key=="label")
      res = NextFreeLabel(val);

    else if (key=="allcontentsclasses")
      allcontentsclasses = val;

    else if (key=="alldetections")
      alldetections = val;

    else if (key=="overwritedetections")
      overwritedetections = IsAffirmative(val);

    else if (key=="has_origins")
      has_origins = IsAffirmative(val);

    else if (key=="wxh_reversed_in_origins")
      wxh_reversed_in_origins = IsAffirmative(val);

    else if (key=="empty_labels_allowed")
      empty_labels_allowed = IsAffirmative(val);

    else if (key=="ignore_leaf_features")
      ignore_leaf_features = IsAffirmative(val);

    else if (key=="use_subobject_text")
      use_subobject_text = IsAffirmative(val);

    else if (key=="nofeaturedata") {
      if (!Picsom()->IsSlave())
	nofeaturedata = IsAffirmative(val);
    }
    else if (key=="slavenofeaturedata") {
      if (Picsom()->IsSlave())
	nofeaturedata = IsAffirmative(val);
    }

    else if (key=="bin_features")
      use_bin_features_read = use_bin_features_write = IsAffirmative(val);

    else if (key=="use_bin_features" || key=="use_bin_features_read")
      use_bin_features_read = IsAffirmative(val);

    else if (key=="use_bin_features_write")
      use_bin_features_write = IsAffirmative(val);

    else if (key=="framestep")
      framestep = intval;

    else if (key=="margin")
      margin = intval;

    else if (key=="mediaclips") // was mpeg7videoclip
      mediaclips = IsAffirmative(val);

    else if (key=="tempmediafiles")
      tempmediafiles = IsAffirmative(val);

    else if (key=="tempimagefiles")
      tempimagefiles = IsAffirmative(val);

    else if (key=="segment_is_modifier")
      segment_is_modifier = IsAffirmative(val);

    else if (key=="use_implicit_subobjects")
      use_implicit_subobjects = IsAffirmative(val);

    else if (key=="extractobjectpath")
      extractobjectpath = val;

    else if (key=="reverse")
      ret = MapReverseURLs(GroundTruthExpression(val), true);

    else if (key=="linkiconmode")
      linkiconmode = val;

    else if (key=="default_feature_target")
      default_feature_target = TargetType(val);

    else if (key=="loadandmatch")
      LoadAndMatchFeatures(val);

    else if (key=="extra_features_dir")
      extra_features_dir = val;

    else if (key=="alwaysusetarfiles")
      alwaysusetarfiles = IsAffirmative(val);

    else if (key=="extractfulltars")
      extractfulltars = IsAffirmative(val);

    else if (key=="extractfullzips")
      extractfullzips = IsAffirmative(val);

    else if (key=="uselocalzipfiles")
      uselocalzipfiles = IsAffirmative(val);

    else if (key=="concepts")
      concepts = val;

    else if (key=="erf_detection_images") {
      if (val=="")
	erf_detection_images.clear();
      else {
	vector<string> v = SplitInCommas(val);
	erf_detection_images = list<string>(v.begin(), v.end());
      }

    } else if (key=="addobject") {
      vector<string> v = SplitInSomething("|", false, val);
      if (v.size()==4) {
	if (LabelIndexGentle(v[0], false)<0) {
	  // this _should_ always happen, but it doesn't if
	  // downloaded sqlite3.db already contains the label...
	  size_t idx = Size();
	  target_type tt = SolveTargetTypes(v[1]);
	  AddObject(v[0], tt);
	  map<string,string> imap {
	    { "auxid",      v[3] },
	    { "colors",     ">256" },
	    { "dimensions", "107x701" },
	    { "checksum",   "12345" }
	  };
	  int nf = 0;
	  float fr = 0;
	  XmlDom xml;
	  InsertOriginsInfo(idx, false, 
			    v[2], v[2], "image/x", "-", "-", "", imap,
			    "-", tt, nf, fr, xml);
	}
      } else
	ret = 0;      
    }

#ifdef PICSOM_USE_OD
    else if (key=="odmatch") {
      string dblist = val, index;
      if (dblist.find("./")!=0 && dblist.find("../")!=0 &&
	  dblist.find("/")!=0)
	dblist = ExpandPath("ods", dblist);
      ret = odone.LoadDbs(dblist, index);
      if (ret)
	WriteLog("Loaded od match file <"+ShortFileName(dblist)+">");
      else
	ShowError("Failed to load od match file <"+dblist+">");

    } else if (key=="oddebug") {
      odone.SetDebug(intval);

    } else if (key=="odshowkeypoints") {
      odone.SetShowKeypoints(IsAffirmative(val));

    } else if (key=="odshowresults") {
      odone.SetShowResults(IsAffirmative(val));

    // } else if (key=="od") {
    //   odone.();
    }
#endif // PICSOM_USE_OD

#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
    else if (key=="caffefusion")
      caffefusion = val;
#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE

    else if (key.find("gt(")==0 && *key.rbegin()==')')
      ret = SetGroundTruth(key.substr(3, key.size()-4), val);

    else if (key.find("featurealias(")==0 && *key.rbegin()==')')
      ret = SetFeatureAlias(key.substr(13, key.size()-14), val);

    else if (key.find("namedcmdline(")==0 && *key.rbegin()==')')
      ret = SetNamedCmdLine(key.substr(13, key.size()-14), val);

    else if (key=="defaultfeatures" || key=="defaultindices")
      ret = defaultquery->Interpret("indices", val, res, NULL);

    else if (key=="defaulttarget")
      ret = defaultquery->Interpret("target", val, res, NULL);

    else
      ret = defaultquery->Interpret(key, val, res, NULL);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InterpretDefaults() {
    string msg = "DataBase::InterpretDefaults() : ";
    bool ok = true, debug = false;
    int ret = 1, r = ret;
    list<pair<string,string> > kvl = Picsom()->DefaultKeyValues("database");
    for (list<pair<string,string> >::const_iterator i=kvl.begin();
         i!=kvl.end(); i++) {
      bool okx = Interpret(i->first, i->second, r);
      ok = ok && okx;
      if (!okx)
	ShowError(msg+"key <"+i->first+"> unknown");

      if (debug)
        cout << TimeStamp() << msg << Name() << " " << (void*)this << " "
	     << i->first << "=" << i->second << " r=" << r << " okx=" << okx
	     << " ok=" << ok << endl;
      if (r!=1)
        ret = r;
    }
    return ok && ret==1;
  }

  /////////////////////////////////////////////////////////////////////////////

  CbirAlgorithm *DataBase::FindOrCreateAlgorithm(const string& n) {
    if (Query::DebugAlgorithms())
      cout << "DataBase::FindOrCreateAlgorithm(" << n
           << ") algorithms.size()=" << algorithms.size() << endl;

    for (size_t i=0; i<algorithms.size(); i++) {
      if (Query::DebugAlgorithms())
        cout << "  " << algorithms[i]->FullName();
      if (algorithms[i]->FullName()==n) {
	if (Query::DebugAlgorithms())
	  cout << "  found" << endl;
	return algorithms[i];
      }
      if (Query::DebugAlgorithms())
        cout << endl;
    }
    if (Query::DebugAlgorithms())
      cout << "  NOT FOUND, calling CreateAlgorithm()" << endl;

    return CreateAlgorithm(n);
  }

  /////////////////////////////////////////////////////////////////////////////

  CbirAlgorithm *DataBase::CreateAlgorithm(const string& n) {
    if (Query::DebugAlgorithms())
      cout << "DataBase::CreateAlgorithm(" << n << ")" << endl;

    CbirAlgorithm *a = CbirAlgorithm::FindAndCreateAlgorithm(this, n);
    if (a!=NULL)
      algorithms.push_back(a);

    if (Query::DebugAlgorithms()) {
      cout << "DataBase::CreateAlgorithm(" << n << ") create ";
      if (!a)
        cout << "failed";
      else
        cout << "succeeded with name=" << a->FullName() << endl;
    }

    return a;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ExpandPath(const string& p, const string& q,
			      bool primaryonly) const {
    if (false&&MysqlMode())
      return "";

    if (p[0]=='/')
      return p;

    string prim = path+"/"+p;
    if (q!="")
      prim += "/"+q;

    if (secondarypath.empty() || primaryonly || Exists(prim))
      return prim;

    string sec = secondarypath+"/"+p;
    if (q!="")
      sec += "/"+q;

    if (Exists(sec))
      return sec;

    return prim;
  }

  /////////////////////////////////////////////////////////////////////////////
  /*
  const char *DataBase::ExpandPathM(const char *p, const char *q,
				    bool primaryonly) const {
    if (MysqlMode())
      return NULL;

    if (p && *p=='/')
      return CopyString(p);

    char *prim = NULL;
    CopyString(prim, path.c_str());
    AppendString(prim, "/");
    AppendString(prim, p);
    if (q) {
      AppendString(prim, "/");
      AppendString(prim, q);
    }
    if (secondarypath.empty() || primaryonly || Exists(prim))
      return prim;

    char *sec = NULL;
    CopyString(sec, secondarypath.c_str());
    AppendString(sec, "/");
    AppendString(sec, p);
    if (q) {
      AppendString(sec, "/");
      AppendString(sec, q);
    }
    if (Exists(sec)) {
      delete prim;
      return sec;
    }

    return prim;
  }
  */
  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ExpandViewPath(const string& p,
				  const string& q) const {
    if (p.substr(0, 1)=="/")
      return p;

    string ret = ExpandPath("views", p);

    if (q!="")
      ret += string("/")+q;

    return ret;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ProcessAccessEntry(const char *line) {
  access.Append(new RemoteHost(this));
  bool r = access.Peek()->Set(true, line);
  if (!r)
    ShowError("DataBase::ProcessAccessEntry() failed ", name,
	      " <", line, ">");
  // access.Peek()->Dump();

  return r;
}

///////////////////////////////////////////////////////////////////////////////

RemoteHost *DataBase::GetNewRemoteHost(bool is_serv, const string cook, 
				       const string passwd, const char* addr,
				       map<string,string> *rights) {
  RemoteHost* r = new RemoteHost(this);
  r->IsServant(is_serv);
  if (cook.size())
    r->Cookie(cook);
  if (passwd.size())
    r->UserPassword(passwd);

  string s(addr?addr:"");
  StripWhiteSpaces(s);
  if (s == "ALL")
    s = "0.0.0.0/0";
  r->Address(s.c_str());
  r->SetRights(rights);

  return r;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddNewRemoteHost(bool is_serv, const string cookie, 
					const string passwd, const char* addr,
					map<string,string>* rights,
					RemoteHost** list_first, 
					RemoteHost** list_last) {

  RemoteHost* r = 
    GetNewRemoteHost(is_serv,cookie,passwd,addr,rights);

  if (!r->IsSet()) {
    delete r;
    return false;
  }

  // set the pointers:
  if(*list_first == NULL)
    *list_first = r;
  if(*list_last != NULL)
    (*list_last)->SetNext(r);
  *list_last = r;
  return true;
}

///////////////////////////////////////////////////////////////////////////////

pair<string,string> DataBase::ParseRightString(const char* rightstr) {
  string val(rightstr?rightstr:"");
  StripWhiteSpaces(val);

  string key = "";
  string value = "";
  // check if we have a key-value pair or only the key
  string::size_type i = val.find_first_of(' ');
  if(i == string::npos)
    key = val;
  else {
    key = val.substr(0,i);
    value = val.substr(i+1);
  }

  pair<string,string> p(key,value);
  return p;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ProcessAccessEntryXML(xmlNodePtr l) {

  // pointers to various elements in the linked list of RemoteHost-objects:
  RemoteHost* first_serv = NULL;
  RemoteHost* last_serv = NULL;
  RemoteHost* first_cli = NULL;
  RemoteHost* last_cli = NULL;
  string cook;
  string passwd;
  map<string,string> rights; // the rights are stored here

  xmlNodePtr current = l->children;
  while(current) {
    if (!current->children) {
      current = current->next;
      continue;
    }
    if (!current->children->content) {
      ShowError("DataBase::ProcessAccessEntryXML() : subnode of <rule> ",
		"without content");
      current = current->next;
      continue;
    }

    // parse <servant> node:
    if (!xmlStrcmp(current->name, (XMLS)"servant")) {
      if(!AddNewRemoteHost(true,cook,passwd,
			   (const char*)current->children->content,
			   &rights, &first_serv, &last_serv))
	ShowError("DataBase::ProcessAccessEntryXML() : invalid <servant> ",
		  "node");
    }

    // parse <client> node:
    else if (!xmlStrcmp(current->name, (XMLS)"client")) {

      if(!AddNewRemoteHost(false,cook,passwd,
			   (const char*)current->children->content,
			   &rights, &first_cli, &last_cli))
	ShowError("DataBase::ProcessAccessEntryXML() : invalid <client> ",
		  "node");
    }

    // How should the password & cookie settings behave? Should we define that
    // all the <client> nodes after the <passwd> & <cookie> use the given
    // password/cookie (like it's done now), or should all the <client> nodes 
    // in the <rule> use the same defined password/cookie? If we want 
    // per-client passwords/cookies, we should perhaps make <passwd> an 
    // optional subnode of <client>.

    else if (!xmlStrcmp(current->name, (XMLS)"passwd")) {
      string tmp = NodeChildContent(current);
      StripWhiteSpaces(tmp);
      passwd = tmp;
    }
    else if (!xmlStrcmp(current->name, (XMLS)"cookie")) {
      string tmp = NodeChildContent(current);
      StripWhiteSpaces(tmp);
      cook = tmp;
    }

    // All the <right>-nodes specified BEFORE a <client> or <server> node
    // affect those nodes.

    else if (!xmlStrcmp(current->name, (XMLS)"right")) 
      rights.insert(ParseRightString((const char*)current->children->content));
 
    // else 
    //   ShowError("DataBase::ProcessAccessEntryXML() : subnode <",
    //   	(char*)current->name,"> of node <rule> not (yet) supported");
 
    current = current->next;
  }

  // ensure that we have both servants and clients
  if(first_cli == NULL || first_serv == NULL) {
    ShowError("DataBase::ProcessAccessEntryXML() : missing either <servant> ",
	      "or <client> subnode in the <rule> node");
    if(first_cli != NULL)
      delete first_cli;
    if(first_serv != NULL)
      delete first_serv;
    return false;
  }

  // Now we have a linked list of servant and client RemoteHosts. Combine
  // these two:
  last_serv->SetNext(first_cli);

  access.Append(first_serv);
  // access.Peek()->Dump();

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ProcessAspectsEntryXML(xmlNodePtr l) {
  string msg = "DataBase::ProcessAspectsEntryXML() : ";

  string targett = GetProperty(l, "targettype");
  if (targett=="")
    return ShowError(msg+"no targettype attribute in an <aspects> node");

  target_type tt = TargetType(targett);
  if (tt == target_error)
    return ShowError(msg+"invalid targettype in an <aspects> node");

  aspects a;

  for (xmlNodePtr current = l->children; current; current = current->next) {
    if (NodeName(current)=="aspect") {
      string name  = GetProperty(current, "name");
      string type  = GetProperty(current, "type");
      string value = GetProperty(current, "value");
      bool sticky  = IsAffirmative(GetProperty(current, "sticky"));

      if (name=="") {
	ShowError(msg+"no name attribute in the <aspect> node");
	continue;
      }

      double v =  atof(value.c_str());
      a.AspectRelevance(name, v, type, vector<string>(), sticky);
    }
  }

  DefaultAspects(tt, a);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

aspects DataBase::DefaultAspects(target_type t) const {
  typedef map<target_type,aspects> map_t;
  map_t::const_iterator i = defaultaspects_cache.find(t);
  if (i!=defaultaspects_cache.end())
    return i->second;

  aspects a;

  for (i=defaultaspects.begin(); i!=defaultaspects.end(); i++)
    if (i->first==target_any_target ||
	PicSOM::TargetTypeContains(t, i->first))
      a.PushAspects(i->second);

  if (!a.Size())
    ShowError("DataBase::DefaultAspects(): Default aspects for target type <",
	      TargetTypeString(t), "> ("+ToStr((int)t)+") not found");

  ((DataBase*)this)->CacheDefaultAspects(t, a);  // dirty trick...

  return a;
}

///////////////////////////////////////////////////////////////////////////////

void DataBase::FindAllViews() {
  //
  // There should be a modification time check here
  //

  string views = ExpandPath("views");
#ifdef HAVE_DIRENT_H
  DIR *dv = opendir(views.c_str());  
  for (dirent *dpv; dv && (dpv = readdir(dv)); ) {
    if (!strcmp(dpv->d_name, ".") || !strcmp(dpv->d_name, ".."))
      continue;
 
    FindView(dpv->d_name);
  }
  if (dv)
    closedir(dv);
#endif // HAVE_DIRENT_H
}

///////////////////////////////////////////////////////////////////////////////

DataBase *DataBase::FindView(const string& vname) {
  if (vname=="")
    return this;

  string viewname = Name()+ViewPartSeparator()+vname;

  if (!Picsom()->IsAllowedDataBase(viewname))
    return this;

  for (size_t i=0; i<Nviews(); i++)
    if (View(i).Name()==viewname)
      return &View(i);

  string vnameroot = vname;
  size_t ppos = vnameroot.find('(');
  if (ppos!=string::npos && *vnameroot.rbegin()==')')
    vnameroot.erase(ppos);

  bool ok = true;
  string p = ExpandViewPath(vnameroot);
  if (!DirectoryExists(p))
    ok = ShowError("DataBase::FindView(", vname, ") failed for no dir ", p);

  p = ExpandViewPath(vnameroot, "settings.xml");
  if (!FileExists(p))
    p = ExpandViewPath(vnameroot, "settings2");
  if (!FileExists(p))
    ok = ShowError("Database view <", viewname,
		   "> does not have settings.xml file !");

  if (!ok) {
    ShowError("DataBase::FindView(", vname, ") failed for no file ", p);
    return NULL;
  }

  WriteLog("Registered view ", viewname);

  string viewpath = ExpandPath("views", vnameroot);
  DataBase *newview = new DataBase(Picsom(), viewname, viewpath, path);
  newview->InterpretDefaults(); // added 2010-03-01
  newview->parent_view = this;
  AppendView(newview);

  return newview;
}

///////////////////////////////////////////////////////////////////////////////

string DataBase::FindThumbnailSizes() {
  if (thumbnailsize=="") {
    // if (virtualthumbnailsize!="")
    //  thumbnailsize = virtualthumbnailsize;
    if (false) {
    } else {
      // ReadDataLabels(false);
   
      if (Size()) {
	// first define a fallback label (this is used unless an image
	// object is found):
	string lab = Label(0);
	
	// solve the label of the first object whith target type target_image
	for (size_t i=0; i<Size(); i++) 
	  if (ObjectsTargetType(i) & target_image) {
	    lab = Label(i);
	    break;
	  }
	if (lab.empty())
	  ShowError("DataBase::FindThumbnailSizes() strange: no labels!");

	string dir = SolveObjectDirectory(lab);
#ifdef HAVE_DIRENT_H
	DIR *d = opendir(dir.c_str());  
	for (dirent *dp; d && (dp = readdir(d)); ) {
	  int w, h;
	  if (sscanf(dp->d_name, "tn-%dx%d", &w, &h)==2) {
	    const char *exts[] = { ".gif", ".png", NULL };
	    bool found = false;
	    for (const char **ext = exts; !found && *ext; ext++) {
	      char tn[1024];
	      sprintf(tn,"%s/%s/%s%s",dir.c_str(),dp->d_name,lab.c_str(),*ext);
	      if (FileExists(tn))
		found = true;
	    }
	    if (found) {
	      stringstream tmp;
	      tmp << (thumbnailsize.empty() ? "" : ",") << w << "x" << h;
	      thumbnailsize += tmp.str();
	    }
	  }
	}
	if (d)
	  closedir(d);
#endif // HAVE_DIRENT_H
	
	// if (thumbnailsize=="")
	//   ShowError("DataBase::FindThumbnailSizes() failed for db=",
	// 	       NameP(), " lab=", lab, " dir=", dir);
      }

      if (thumbnailsize=="")
	thumbnailsize = LastResortThumbnailSize();
   
    }
    WriteLog("Registered thumbnail size(s): <", thumbnailsize, ">");
  }

  return thumbnailsize;
}

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::DumpIndexList(ostream& os) const {
    for (size_t j=0; j<Nindices(); j++) {
      Index *i = GetIndex(j);
      os << "j=" << j << " i=" << (void*)i
	 << " indexname=" << i->IndexName()
	 << " featuremethodname=" << i->FeatureMethodName()
	 << " featurefilename=" << i->FeatureFileName()
	 << endl;
    }
  }

///////////////////////////////////////////////////////////////////////////////

Index *DataBase::FindIndexInternal(const string& base, bool lock) {
  Index *ret = NULL;

  if (lock)
    RwLockRead("FindIndexInternal()");

  for (size_t i=0; !ret && i<Nindices(); i++)
    if (base==GetIndex(i)->Name())
      ret = GetIndex(i);

  if (lock)
    RwUnlockRead("FindIndexInternal()");

  if (ret)
    ret->CheckModifiedFiles();

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

Index *DataBase::FindIndex(const string& fn, const string& params,
                           bool create, bool allow_dummy_in) {
  string msg = "DataBase::FindIndex() : ";

  Index *ind = FindIndexInternal(fn, true);

  if (ind && !(!allow_dummy_in && create && ind->IsDummy()))
    return ind;

  size_t lastdot = fn.find_last_of('.');
  if (lastdot != string::npos) {
    string afterdot = fn.substr(lastdot+1);
    if (afterdot == "pre" || afterdot == "svm" || afterdot == "dat") {
      ShowError(msg, "You might have a superfluous \"" + afterdot +
                "\" at the end of feature="+fn);
      return NULL;
    }
  }

  string dir = name=="." ? "." : ExpandPath("features");

  if (!SqlIndices() && !DirectoryExists(dir)) {
    ShowError(msg,"directory <"+dir+"> inexistent");
    return NULL;
  }

  list<string> dirs;
  dirs.push_back(dir);
  if (!extra_features_dir.empty())
    dirs.push_front(extra_features_dir);

  bool allow_dummy = allow_dummy_in || isdummydb;

  RwLockWrite("FindIndex()");
  ind = FindIndexInternal(fn, false);
  if (!ind || (!allow_dummy && create && ind->IsDummy())) {
    Index *tmp = Index::Find(this, fn, dirs, params, create, allow_dummy, ind);
    if (ind) {
      vector<Index*>::iterator i = find(index.begin(), index.end(), ind);
      delete *i;  //obs!  should this be called?
      if (tmp)
	*i = tmp;
      else
	index.erase(i);

    } else if (tmp)
      index.push_back(tmp);

    ind = tmp;
  }
  RwUnlockWrite("FindIndex()");

  return ind;
}

  /////////////////////////////////////////////////////////////////////////////

  size_t DataBase::FindAllIndices(bool met_et_pre) {
    if (Query::DebugFeatSel())
      cout << "DataBase::FindAllIndices() starting" << endl;

    if (SqlFeatures())
      return FindAllIndicesSql();

    return FindAllIndicesClassical(met_et_pre);
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t DataBase::FindAllIndicesSql() {
    string hdr = "DataBase::FindAllIndicesSql() : ";

    if (Query::DebugFeatSel() || DebugSql())
      cout << "DataBase::FindAllIndicesSql() starting" << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      string cmd = "SELECT name FROM sqlite_master WHERE type='table';";
      if (DebugSql())
	cout << hdr << cmd << endl;

      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return 0;

      bool ok = true;

      for (; ok;) {
	int r = sqlite3_step(stmt);
   
	if (r!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	string fname = SqliteString(stmt, 0);

	ok = SqlFindIndex(fname);
      }
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      string cmd = "SELECT table_name FROM INFORMATION_SCHEMA.TABLES"
	" WHERE table_schema='"+mysql_dbname+"'"
	" AND table_name LIKE 'features_%';";
      if (DebugSql())
	cout << hdr << cmd << endl;

      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return 0;

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      char tablename[1000] = "";
      unsigned long tablename_length = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = tablename;
      bind[0].buffer_length = sizeof(tablename);
      bind[0].length        = &tablename_length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      bool ok = true;

      for (; ok;) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(hdr+"data truncated");
	  else
	    ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	string fname(tablename, tablename_length);

	ok = SqlFindIndex(fname);
      }
      MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H

    return Nindices();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlExec(const string& cmd, bool angry) const {
    string hdr = "DataBase::SqlExec("+cmd+") : ";

    if (!UseSql())
      return ShowError(hdr+"not in SQL mode");

    if (DebugSql())
      cout << hdr << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode())
      return SqliteExec(cmd, angry);
#endif // HAVE_SQLITE3_H
 
#ifdef HAVE_MYSQL_H
    if (MysqlMode())
      return MysqlQuery(cmd, angry);
#endif // HAVE_MYSQL_H
 
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlClose() {
    string hdr = "DataBase::SqlClose() : ";

    if (!UseSql())
      return ShowError(hdr+"not in SQL mode");

    if (DebugSql())
      cout << hdr << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode())
      return SqliteClose();
#endif // HAVE_SQLITE3_H
 
#ifdef HAVE_MYSQL_H
    if (MysqlMode())
      return MysqlClose();
#endif // HAVE_MYSQL_H
    
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDeleteDataBase(const string& n, PicSOM *picsom) {
    if (picsom) {/*unused*/}

    string hdr = "DataBase::SqlDeleteDataBase("+n+") : ";

    if (DebugSql())
      cout << hdr << endl;

#ifdef HAVE_SQLITE3_H
    // return ShowError(hdr+"not implemented for sqlite3");
#endif // HAVE_SQLITE3_H
 
#ifdef HAVE_MYSQL_H
    MYSQL *mysqltmp = MysqlInit();
    if (!mysqltmp)
      return false;

    string s = picsom->SqlServer();
    pair<string,string> up = MysqlUserPassword(s);
    const string& mysqlserver = s;

    if (!mysql_real_connect(mysqltmp, mysqlserver.c_str(), up.first.c_str(),
			    up.second.c_str(), NULL, 0, NULL, 0)) {
      mysql_close(mysqltmp);
      if (DebugSql())
	cout << hdr << "mysql_real_connect() A and return false" << endl;
      return false;
    }
    string pn = n;
    if (pn.find("mysql:")!=0 || pn.size()<7)
      return ShowError(hdr+"not a MySQL database");

    pn.erase(0, 6);
    pn = "picsom_"+pn;

    string cmd = "DROP DATABASE "+pn+";";
    if (DebugSql())
      cout << hdr << cmd << endl;
    
    if (!MysqlQuery(cmd, true, mysqltmp))
      return ShowError(hdr+cmd+" failed");

    picsom->WriteLog("Successfully deleted MySQL database "+n+
		     " (hope this is want you wanted)");

#endif // HAVE_MYSQL_H
 
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  sql_table_info DataBase::SqlTableInfo(const string& t) const {
    string hdr = "DataBase::SqlTableInfo("+t+") : ";

    sql_table_info info;
    info.exists = false;
    info.rows = 0;

    if (!UseSql()) {
      ShowError(hdr+"not in SQL mode");
      return info;
    }

    string spec;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      string cmd = "SELECT sql FROM sqlite_master WHERE type='table'"
	" AND name='"+t+"';";
      if (DebugSql())
	cout << hdr << cmd << endl;

      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return info;

      int r = sqlite3_step(stmt);
      if (r!=SQLITE_ROW)
	return info;

      if (DebugSql())
	SqliteShowRow(stmt);

      string sql = SqliteString(stmt, 0);
      sqlite3_finalize(stmt);

      info.schema = sql;
      info.name = t;
      info.spec = SqlParseSpec(sql);

      cmd = "SELECT Count(*) FROM '"+t+"'";
      if (DebugSql())
	cout << hdr << cmd << endl;

      stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return info;

      r = sqlite3_step(stmt);
      if (r!=SQLITE_ROW)
	return info;

      if (DebugSql())
	SqliteShowRow(stmt);

      info.rows = atoi(SqliteString(stmt, 0).c_str());
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      string cmd = "SELECT column_name,column_type FROM "
	"INFORMATION_SCHEMA.COLUMNS"
	" WHERE table_schema='"+mysql_dbname+"' AND table_name='"+t+"';";
      if (DebugSql())
	cout << hdr << cmd << endl;

      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return info;

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      char name[1000] = "", type[1000] = "";
      unsigned long name_length = 0, type_length = 0;
      MYSQL_BIND bind[2];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = name;
      bind[0].buffer_length = sizeof(name);
      bind[0].length        = &name_length;
      bind[1].buffer_type   = MYSQL_TYPE_STRING;
      bind[1].buffer        = type;
      bind[1].buffer_length = sizeof(type);
      bind[1].length        = &type_length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      string spec;
      while (true) {
	int rrr = mysql_stmt_fetch(stmt);
	if (rrr!=0)
	  break;

	string sname((char*)bind[0].buffer, name_length);
	string stype((char*)bind[1].buffer, type_length);

	info.spec.push_back(make_pair(sname, stype));

	if (spec!="")
	  spec += ",";
	spec += sname;
	if (stype!="")
	  spec += " "+stype;
      }
      MysqlStmtClose(stmt);

      info.name = t;
      info.schema = "CREATE TABLE "+t+"("+spec+");";

      cmd = "SELECT COUNT(*) FROM "+t+";";
      if (DebugSql())
	cout << hdr << cmd << endl;

      stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return info;

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      long n = -1;
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_LONGLONG;
      bind[0].buffer        = &n;
      bind[0].buffer_length = sizeof(n);

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }
    
      int rrr = mysql_stmt_fetch(stmt);
      if (rrr==0)
	info.rows = n;

      MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H

    return info;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<sql_table_info> DataBase::SqlTableInfoList(const string& t) const {
    string hdr = "DataBase::SqlTableInfoList("+t+") : ";

    list<sql_table_info> ilist;
    list<string> nlist;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      string cmd = "SELECT name FROM sqlite_master WHERE type='table'";
      if (t!="")
	cmd += " AND name LIKE '"+t+"'";
      cmd += ";";
      if (DebugSql())
	cout << hdr << cmd << endl;

      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return ilist;

      while (true) {
	int r = sqlite3_step(stmt);
	if (r!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	string name = SqliteString(stmt, 0);
	nlist.push_back(name);
      }
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      string cmd = "SELECT table_name FROM INFORMATION_SCHEMA.TABLES"
	" WHERE table_schema='"+mysql_dbname+"'";
      if (t!="")
	cmd += " AND table_name LIKE '"+t+"'";
      cmd += ";";
      if (DebugSql())
	cout << hdr << cmd << endl;

      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return ilist;

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      char tablename[1000] = "";
      unsigned long tablename_length = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = tablename;
      bind[0].buffer_length = sizeof(tablename);
      bind[0].length        = &tablename_length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      while (true) {
	int rrr = mysql_stmt_fetch(stmt);
	if (rrr!=0)
	  break;

	string name((char*)bind[0].buffer, tablename_length);
	nlist.push_back(name);
      }
      MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H

    for (list<string>::const_iterator i=nlist.begin(); i!=nlist.end(); i++) {
      sql_table_info info = SqlTableInfo(*i);
      ilist.push_back(info);
    }

    return ilist;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<pair<string,string> > DataBase::SqlParseSpec(const string& s) const {
    vector<pair<string,string> > r;

    size_t a = s.find('('), b = s.rfind(')');
    if (b!=string::npos && a<b) {
      string ss = s.substr(a+1, b-a-1);
      vector<string> vs = SplitInCommasObeyParentheses(ss);
      for (size_t i=0; i<vs.size(); i++) {
	string k = vs[i], v;
	while (isspace(k[0]))
	  k.erase(0, 1);
	a = k.find_first_of(" \t");
	if (a!=string::npos) {
	  v = k.substr(a+1);
	  while (isspace(v[0]))
	    v.erase(0, 1);
	  k.erase(a);
	  a = v.find_last_not_of(" \t");
	  if (a<v.size()-1)
	    v.erase(a);
	}
	r.push_back(make_pair(k, v));
      }
    }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlFileExists(const string& t) const {
    string hdr = "DataBase::SqlFileExist("+t+") : ";

    // sql_file_info info = SqlFileInfo(t);
    // return info.exists;

    return sql_files.find(t)!=sql_files.end();
  }

  /////////////////////////////////////////////////////////////////////////////

  sql_file_info DataBase::SqlFileInfo(const string& t) const {
    string hdr = "DataBase::SqlFileInfo("+t+") : ";

    list<sql_file_info> il = SqlFileInfoList(t);

    if (il.size()==1)
      return *il.begin();

    sql_file_info info = {
      t, 0, TimeZero(), TimeZero(), "", false
    };

    return info;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<sql_file_info> DataBase::SqlFileInfoList(const string& t) const {
    string hdr = "DataBase::SqlFileInfoList("+t+") : ";

    list<sql_file_info> ilist;

    Tic("SqlFileInfoList");

    string cmd = "SELECT name,moddate,insdate,user FROM "+
      SqlTableName("files", SqlStyle());
    if (t!="") {
      string op = t.find('%')==string::npos ? "=" : " LIKE ";
      cmd += " WHERE name"+op+"'"+t+"'";
    }
    cmd += ";";

    if (DebugSql())
      cout << hdr << cmd << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, false);
      if (!stmt)
	return ilist;

      bool ok = true;

      for (; ok;) {
	int r = sqlite3_step(stmt);
   
	if (r!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	string name = SqliteString(stmt, 0);
	string modd = SqliteString(stmt, 1);
	string insd = SqliteString(stmt, 2);
	string user = SqliteString(stmt, 3);

	string data;
	ok = SqlRetrieveFile(name, data, true);
	if (!ok)
	  ShowError(hdr+"retrieving <"+name+"> failed");
	else {
	  struct timespec modtime, instime;
	  memcpy(&modtime, modd.c_str(), sizeof modtime);
	  memcpy(&instime, insd.c_str(), sizeof instime);
	  sql_file_info info = {
	    name, data.size(), modtime, instime, user, true
	  };
	  ilist.push_back(info);
	}
      }
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, DebugSql());
      if (!stmt)
	return ilist;
   
      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      char name[1000] = "", user[8] = "";
      unsigned long name_length = 0, moddate_length = 0, insdate_length = 0,
	user_length = 0;

      struct timespec moddate = TimeZero(), insdate = TimeZero();

      MYSQL_BIND bind[4];
      memset(bind, 0, sizeof(bind));

      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = name;
      bind[0].buffer_length = sizeof(name);
      bind[0].length        = &name_length;

      bind[1].buffer_type   = MYSQL_TYPE_STRING;
      bind[1].buffer        = &moddate;
      bind[1].buffer_length = sizeof(moddate);
      bind[1].length        = &moddate_length;

      bind[2].buffer_type   = MYSQL_TYPE_STRING;
      bind[2].buffer        = &insdate;
      bind[2].buffer_length = sizeof(insdate);
      bind[2].length        = &insdate_length;

      bind[3].buffer_type   = MYSQL_TYPE_STRING;
      bind[3].buffer        = user;
      bind[3].buffer_length = sizeof(user);
      bind[3].length        = &user_length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      bool ok = true;
   
      for (; ok;) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(hdr+"data truncated");
	  else
	    ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	sql_file_info info = {
	  string(name, name_length), 0, moddate, insdate,
	  string(user, user_length), true
	};
	ilist.push_back(info);
      }
      MysqlStmtClose(stmt);

      for (list<sql_file_info>::iterator i=ilist.begin();
	   i!=ilist.end(); i++) {
	string data;
	SqlRetrieveFile(i->name, data);
	i->size = data.size();
      }
    }
#endif // HAVE_MYSQL_H

    Tac("SqlFileInfoList");

    return ilist;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlUpdateFileList() {
    string hdr = "DataBase::SqlUpdateFileList() : ";

    sql_files.clear();
    list<sql_file_info> ilist = SqlFileInfoList("");
    for (list<sql_file_info>::const_iterator i=ilist.begin();
	 i!=ilist.end(); i++) {
      sql_files.insert(i->name);
      if (DebugSql())
	cout << hdr << "inserting <" << i->name << ">" << endl;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlTableExists(const string& t) const {
    string hdr = "DataBase::SqlTableExists("+t+") : ";

    if (!UseSql())
      return ShowError(hdr+"not in SQL mode");

    if (sql_table.find(t)!=sql_table.end())
      return true;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      string cmd = "SELECT name FROM sqlite_master WHERE type='table'"
	" AND name='"+t+"';";
      if (DebugSql())
	cout << hdr << cmd << endl;

      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return false;

      int r = sqlite3_step(stmt);
      bool ok = r==SQLITE_ROW;

      if (DebugSql())
	SqliteShowRow(stmt);

      sqlite3_finalize(stmt);

      if (ok)
	((DataBase*)this)->sql_table.insert(t);

      return ok;
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      string cmd = "SELECT table_name FROM INFORMATION_SCHEMA.TABLES"
	" WHERE table_schema='"+mysql_dbname+"'"
	" AND table_name='"+t+"';";
      if (DebugSql())
	cout << hdr << cmd << endl;

      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return 0;

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      char tablename[1000] = "";
      unsigned long tablename_length = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = tablename;
      bind[0].buffer_length = sizeof(tablename);
      bind[0].length        = &tablename_length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      int rrr = mysql_stmt_fetch(stmt);
      bool ok = rrr==0;
	
      if (rrr) {
	if (rrr==MYSQL_NO_DATA)
	  ; // cout << "no data" << endl;
	else if (rrr==MYSQL_DATA_TRUNCATED)
	  ShowError(hdr+"data truncated");
	else
	  ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
      }
      MysqlStmtClose(stmt);

      if (ok)
	((DataBase*)this)->sql_table.insert(t);

      return ok;
    }
#endif // HAVE_MYSQL_H

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlCreateTable(const string& t, const string& s) const {
    string hdr = "DataBase::SqlCreateTable("+t+","+s+") : ";

    if (!UseSql())
      return ShowError(hdr+"not in SQL mode");

    string cmd = t!="" ? "CREATE TABLE "+t+" ("+s+");" : s+";";
 
    return SqlExec(cmd, true);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlAddAlias(const string& a, const string& f) {
    string hdr = "DataBase::SqlAddAlias() : ";

    if (SqlMode()!=2)
      return ShowError(hdr+"not in SQL rw mode");

    string cmd = "INSERT INTO aliases VALUES('"+a+"','"+f+"');";
    if (DebugSql())
      cout << hdr << cmd << endl;
    
    return SqlExec(cmd, true);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlReadAliases() {
    string hdr = "DataBase::SqlReadAliases() : ";
    sql_alias.clear();
    string cmd = "SELECT keyz,value FROM aliases;";
    if (DebugSql())
      cout << hdr << cmd << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return false;

      while (true) {
	int r = sqlite3_step(stmt);

	if (r!=SQLITE_ROW) {
	  sqlite3_finalize(stmt);
	  break;
	}

	if (DebugSql())
	  SqliteShowRow(stmt);

	string keyz  = (const char*)sqlite3_column_text(stmt, 0);
	string value = (const char*)sqlite3_column_text(stmt, 1);

	sql_alias[keyz] = value;
      }
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return false;

      if (!MysqlExecute(stmt, true))
	return false;

      char keyz[10], value[200];
      long unsigned int keyz_length = 0, value_length = 0;
      MYSQL_BIND bind[2];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = keyz;
      bind[0].buffer_length = sizeof keyz;
      bind[0].length        = &keyz_length;
	
      bind[1].buffer_type   = MYSQL_TYPE_STRING;
      bind[1].buffer        = value;
      bind[1].buffer_length = sizeof value;
      bind[1].length        = &value_length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      while (true) {
	int r = mysql_stmt_fetch(stmt);

	if (r) {
	  // cout << "SQL error <" << mysql_stmt_error(stmt) << ">" << endl;
	  MysqlStmtClose(stmt);
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	string keyzs(keyz, keyz_length);
	string values(value, value_length);

	sql_alias[keyzs] = values;

	if (DebugSql())
	  cout << "ADDING SQL alias <" << keyzs << ">=<" << values
	       << ">" << endl;
      }
    }
#endif // HAVE_MYSQL_H
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlFindIndex(const string& fin) {
    string hdr = "DataBase::SqlFindIndex("+fin+") : ";

    if (fin.find("features_")==0) {
      string fname = fin.substr(9);
      fname = SqlEscape(fname, false, false);
      if (DebugSql())
	cout << hdr << " [" << fin << "] -> [" << fname << "]" << endl;
   
      if (!FindIndex(fname, "", true, false))
	return ShowError(hdr+"FindIndex("+fname+") failed");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t DataBase::FindAllIndicesClassical(bool met_et_pre) {
    //
    // There should be a modification time check here
    //

    bool debug = false;

    if (Query::DebugFeatSel())
      cout << "DataBase::FindAllIndicesClassical() starting" << endl;

    string dir = ExpandPath("features");
#ifdef HAVE_DIRENT_H
    DIR *d = opendir(dir.c_str());  
    for (dirent *dp; d && (dp = readdir(d)); ) {
      const string fname = dp->d_name;
      string base;
      if (fname[0]=='.')
	continue;

      bool ok = false;

      if (FileNameExtension(fname, ".div", base))
	ok = FindIndex(base);

      if (FileNameExtension(fname, ".cod", base) &&
	  fname.find(TSSOM::AlienPartSeparator())!=string::npos)
	ok = FindIndex(base);

      if (met_et_pre && FileNameExtension(fname, ".met", base))
	ok = FindIndex(base);

      if (met_et_pre && FileNameExtension(fname, ".pre", base))
	ok = FindIndex(base);

      if (debug && Query::DebugFeatSel())
	cout << "DataBase::FindAllIndices() <" << fname << "> -> <"
	     << base << "> " << (ok?"SUCCESS":"FAILURE") << endl;
    }
    if (d)
      closedir(d);
#endif // HAVE_DIRENT_H

    if (!startup_size)
      original_size = startup_size = Size();

    CreateDefaultFeatureAliases(true, false); 

    if (Query::DebugFeatSel())
      cout << "DataBase::FindAllIndices() ending" << endl;

    return Nindices();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SolveExtractions() {
    if (!settingsxml.Root().FirstChild())
      return false;

    if (extractions_solved)
      return true;

    extractions_solved = true;

    string msg = "SolveExtractions() : ";

    set<string> onames;

    XmlDom extr = settingsxml.Root().FirstElementChild("extraction");

    for (XmlDom node = extr.FirstElementChild(); node;
	 node=node.NextElement()) {
      bool ok = true;

      string key = node.NodeName();
      if (key=="feature")
	ok = DescribedIndex(node);

      else if (key=="segmentation")
	ok = DescribedSegmentation(node);

      else if (key=="objectinsertion")
	ok = DescribedObjectInsertion(node);

      else if (key=="detection")
	ok = DescribedDetection(node);

      else if (key=="captioning")
	ok = DescribedCaptioning(node);

      else if (key=="media")
	ok = DescribedMedia(node);

      else
	return ShowError(msg+"parser error : not <feature>/<segmentation>"
			 "/<detection>/<captioning>/<media> but <"+key+">");

      if (!ok)
	return ShowError(msg+"processing <"+key+"> failed");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SolveUrlRules() {
    if (!settingsxml.Root().FirstChild())
      return false;

    string msg = "SolveUrlRules() : ";

    url_rules.clear();

    XmlDom extr = settingsxml.Root().FirstElementChild("urlrules");

    for (XmlDom node = extr.FirstElementChild(); node;
	 node=node.NextElement()) {
      string key = node.NodeName();
      if (key!="urlrule")
	return ShowError(msg+"parser error : not <urlrule>"
			 " but <"+key+">");

      string name = node.Property("name");
      if (name=="")
	return ShowError(msg+"<urlrule> should have a name");
      if (url_rules.find(name)!=url_rules.end())
	return ShowError(msg+"<urlrule> should have a unique name");

      map<string,string> m;
      for (XmlDom c = node.FirstElementChild(); c; c=c.NextElement()) {
	string a = c.NodeName(), b = c.FirstChildContent();
	if (m.find(a)!=m.end())
	  return ShowError(msg+"<urlrule name=\""+name+"\"> has multiple <"+
			   a+">");
	if (0) // this was used with X-Auth-Token...
	  for (;;) {
	    size_t p = b.find("\n");
	    if (p==string::npos)
	      break;
	    b.erase(p, 1);
	    while (b[p]==' ' || b[p]=='\t')
	      b.erase(p, 1);
	    
	    for (;;) {
	      if (p--==0)
		break;
	      if (b[p]==' ' || b[p]=='\t')
		b.erase(p, 1);
	      else
		break;
	    }
	  }
	m[a] = b;
      }
      if (m.find("address")==m.end())
	return ShowError(msg+"<urlrule name=\""+name+"\"> should have <"+
			   "address>");
      url_rules[name] = m;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SolveTextIndices() {
    if (!settingsxml.Root().FirstChild())
      return false;

    if (textindices_solved)
      return true;

    textindices_solved = true;

    string msg = "SolveTextIndices() : ";

    set<string> onames;

    XmlDom extr = settingsxml.Root().FirstElementChild("textindices");

    for (XmlDom node = extr.FirstElementChild(); node;
	 node=node.NextElement()) {
      bool ok = true;

      string key = node.NodeName();
      if (key=="textindex")
	ok = DescribedTextIndex(node);

      else
	return ShowError(msg+"parser error : not <textindex>"
			 " but <"+key+">");

      if (!ok)
	return ShowError(msg+"processing <"+key+"> failed");
    }

    if (!TextIndexInput(textindices_stored_lines))
	return ShowError(msg+"TextIndexInput() failed");

    textindices_stored_lines.clear();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedTextIndex(const XmlDom& node) {
    string msg = "DataBase::DescribedTextIndex() ";

    string idxname = node.Property("name");
    if (idxname=="")
      return ShowError(msg+": no or empty name=\"\"");

    list<pair<string,string> > lo, prop = node.Properties();

    for (auto& p : prop) {
      if (p.first=="use")
	continue;

      if (debug_text) 
	WriteLog(msg+idxname+" : <"+p.first+">=<"+p.second+">");

      lo.push_back(p);
    }

    auto ll = ExpandDescription(idxname, lo, debug_text);

    for (auto& li : ll) {
      auto l = li.second;

      size_t rno = 0;
      XmlDom rule = node.FirstElementChild("rule");
      while (rule) {
	string name = "rule#"+ToStr(rno++);
	string val = rule.FirstChildContent();
	if (debug_text) 
	  WriteLog(msg+idxname+" : <"+name+">=<"+val+">");
	l.push_back(make_pair(name, val));
	rule = rule.NextElement("rule");
      }

      if (debug_text) {
	stringstream ss;
	ss << "<" << li.first << ">";
	for (auto& i : li.second)
	  ss << " " << i.first << "=" << i.second;
	WriteLog(msg+ss.str());
      }

      if (described_textindices.find(li.first)!=described_textindices.end())
	return ShowError(msg+idxname+" <"+li.first+"> already defined");

      described_textindices[li.first] = l;

      XmlDom search = node.FirstElementChild("search");
      while (search) {
	map<string,string> pmap = search.PropertyMap();
	string name = pmap["name"];
	if (name=="")
	  ShowError(msg+"textindex/search specified without name!");
	textsearchspec_t sspec(pmap["name"]);
	sspec.textindex  = pmap["textindex"];
	sspec.textfield  = pmap["textfield"];
	sspec.queryfield = pmap["queryfield"];
	sspec.db = this;
	described_textsearches[name] = sspec;

	// obs!  we should support more complex expressions that could be used
	// to specify operations like:
	//   textquery=meta+asr(foo bar) 
	// OR (alternatively OR equally):
	//   textquery="foo bar" textsearch=meta+asr
	//     => meta:"foo bar" OR asr:"foo bar"^0.5 
	//     => (subject:"foo bar" OR content:"foo bar") OR asr:"foo bar"^0.5

	search = search.NextElement("search");
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<string> DataBase::SolvePicSOMLuceneArguments(const string& n) {
    string msg = "DataBase::SolvePicSOMLuceneArguments("+n+") : ";

    const list<pair<string,string> >& d = TextIndexDescription(n);
    map<string,string> m(d.begin(), d.end());
    string l = m["lucene"];

    if (l!="" && l!="3" && l!="4" && l!="5") {
      ShowError(msg+"lucene argument should be empty|3|4|5");
      return vector<string>();
    }

    list<string> vlist;
    if (l=="3")
      vlist = list<string> { "3.6.1", "3.5.0", "3.4.0", "3.3.0" };
    else
      vlist = list<string> { "5.4.0", "5.3.1" };

    string r = Picsom()->Path()+"/lucene", v;
    for (auto i=vlist.begin(); v=="" && i!=vlist.end(); i++)
      if (DirectoryExists(r+"/lucene-"+*i))
	v = *i;

    if (v=="") {
      ShowError(msg+"solving lucene version failed");
      return vector<string>();
    }

    vector<string> ret;
    // ret.push_back("--debug");
    // ret.push_back("--compile");
    ret.push_back("--root");
    ret.push_back(r);
    ret.push_back("--lucene");
    ret.push_back(v);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  Connection *DataBase::LuceneTextIndex(const string& n) {
    string msg = "DataBase::LuceneTextIndex("+n+") : ";

    auto p = textindices.find(n);
    if (p!=textindices.end())
      return p->second;

    vector<string> cmd {
      Picsom()->UserHomeDir()+"/picsom/java/lucene/picsom-lucene",
    };
    vector<string> arg = SolvePicSOMLuceneArguments(n);
    cmd.insert(cmd.end(), arg.begin(), arg.end());
    cmd.push_back("--");

    if (debug_text)
      WriteLog(msg+"command ["+JoinWithString(cmd, "][")+"]");

    Connection *c = Picsom()->CreatePipeConnection(cmd, false);
    if (!c) {
      ShowError(msg+"creating lucene pipe failed : ["+
		JoinWithString(cmd, "][")+"]");
      return NULL;
    }

    int r = fcntl(c->Wfd(), F_GETFL);
    if (r==-1) {
      ShowError(msg+"fcntl(F_GETFL) failed");
      return NULL;
    }

    r = r | O_NONBLOCK;

    r = fcntl(c->Wfd(), F_SETFL, r);
    if (r==-1) {
      ShowError(msg+"fcntl(F_SETFL) failed");
      return NULL;
    }

    textindices[n] = c;

    string s = LuceneVersion(n);
    WriteLog("Created connection \""+n+"\" to picsom-lucene "+s);

    if (n!="") {
      auto id = TextIndexDescription(n);

      /* this shouldn't be needed...
      string idir, write_lock;
      for (auto i=id.begin(); i!=id.end(); i++)
	if (i->first=="index")
	  idir = i->second;
      if (idir!="")
	write_lock = Path()+"/lucene/"+idir+"/write.lock";
      if (write_lock!="") {
	if (force_lucene_unlock)
	  Unlink(write_lock);
	if (FileExists(write_lock))
	  ShowError(msg+"write lock <"+write_lock+"> exists...");
      }
      */

      if (id.size()) {
	list<string> in;
	if (debug_gt || debug_text)
	  in.push_back("verbose");
	for (auto i=id.begin(); i!=id.end(); i++)
	  if (i->first!="name" && i->first!="type" && i->first!="fields" &&
	      i->first!="lucene" && i->first.substr(0, 5)!="rule#" &&
	      i->first.substr(0, 1)!="_") {
	    string val = i->second;
	    if (i->first=="index")
	      val = Path()+"/lucene/"+val;
	    in.push_back(i->first+" "+val);
	  }
	in.push_back("flush-index");
	bool ok = LuceneOperation(n, in).first;
	if (!ok) {
	  ShowError(msg+"initialization failed");
	  c->Close();
	  return NULL;
	}
      }
    }

    return c;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CloseTextIndices() {
    string msg = "DataBase::CloseTextIndices() : ";
    bool has_any = textindices.size();
    if (has_any)
      WriteLog(msg+"starting");
    list<string> lucenequit { /*"flush-index",*/ "quit" };
    while (textindices.size()) {
      const string& name = textindices.begin()->first;
      Connection *c = LuceneTextIndex(name);
      WriteLog(msg+"   <"+name+"> "+string(c?"non-NULL":"NULL")+" "
	       +string(c?c->IsClosed()?"closed":"open":""));
      if (c && !c->IsClosed())
	LuceneOperation(name, lucenequit);

      textindices.erase(textindices.begin());
    }
    if (has_any)
      WriteLog(msg+"ending");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<bool,list<string> > DataBase::LuceneOperation(const string& name,
						     const list<string>& in,
						     bool syncin) {
    string msg = "DataBase::LuceneOperation("+name+", "+ToStr(in.size())+
      " lines, "+(syncin?"true":"false")+") : ";
    list<string> out;

    if (debug_text)
      WriteLog(msg+"starting");
    
    Connection *c = LuceneTextIndex(name);
    if (!c) {
      ShowError(msg+"failing because LuceneTextIndex() failed");
      return make_pair(false, out);
    }
    
    if (c->IsClosed()) {
      ShowError(msg+"failing because connection "+c->Identity()+" has been closed");
      return make_pair(false, out);
    }

    istream& is = *c->RfdIstream();
    ostream& os = *c->WfdOstream();
    
    bool errored = false;
    string errmsg;
    size_t n = 0, m = 0;
    for (auto i=in.begin();; i++) {
      bool sync = syncin || i==in.end() || (m+1)%10==0;

      if (i!=in.end()) {
	os << *i << endl;
	if (debug_text)
	  cout << TimeStamp() << "lucene in  " 
	       << m << " " << n << " [" << *i << "]" << endl;
	m++;
      }

      if (sync)
	for (; !errored && n<m; n++)
	  for (;;) {
	    string s;
	    getline(is, s);
	    if (debug_text)
	      cout << TimeStamp() << "lucene out "
		   << m << " " << n << " [" << s << "]" << endl;
	    if (s=="* ok")
	      break;
	    if (s.find("* ERROR")==0) {
	      errored = true;
	      errmsg = s.substr(8);
	      break;
	    }
	    if (s.find("* "))
	      continue;
	    out.push_back(s.substr(2));
	  }

      if (i==in.end())
	break;
    }

    if (errored) {
      ShowError(msg+"failing because ERROR \""+errmsg+"\"");
      c->Close();

      return make_pair(false, list<string>());
    }

    if (debug_text)
      WriteLog(msg+"ending");
    
    return make_pair(true, out);
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::LuceneVersion(const string& n) {
    list<string> in { "version" };
    list<string> out = LuceneOperation(n, in).second;
    if (out.empty())
      out.push_back("*unknown*");

    string vers;
    for (auto i=out.begin(); i!=out.end(); i++)
      vers += (vers!=""?" / ":"")+*i;

    return vers;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::TextIndexInput(const list<string>& listlin) {
    string msg = "DataBase::TextIndexInput(...) : ";
    if (listlin.size()==0)
      return true;

    if (!open_read_write_txt)
      return ShowError(msg+"not opened for writing, use -rw=...txt...");

    list<string> all;
    string nnn;

    for (auto lliter=listlin.begin(); lliter!=listlin.end(); lliter++) {
      const string& lin = *lliter;
      msg = "DataBase::TextIndexInput("+lin+") : ";

      string l = lin;
      while (l[0]==' ' || l[0]=='\t')
	l.erase(0, 1);
      size_t p = l.find_first_of(" \t");
      if (p==string::npos)
	return ShowError(msg+"empty line");

      string n = l.substr(0, p);
      if (n!=".") {
	if (!HasTextIndex(n))
	  return  ShowError(msg+"text index <"+n+"> unknown");
      
      } else {
	n = DefaultTextIndex();
	if (n=="")
	  return ShowError(msg+"no default text index defined");
      }

      if (nnn!="" && nnn!=n)
	return ShowError(msg+"multiple index names");
      nnn = n;

      l.erase(0, p);
      while (l[0]==' ' || l[0]=='\t')
	l.erase(0, 1);

      if (l.find("add-attribute")==0) {
	string field = l.substr(14);
	while (field.size() && (field[0]==' ' || field[0]=='\t'))
	  field.erase(0, 1);
	string txt = field;
	size_t p = field.find_first_of(" \t");
	if (p!=string::npos) {
	  field.erase(p);
	  txt.erase(0, p+1);
	  while (txt[0]==' ' || txt[0]=='\t')
	    txt.erase(0, 1);
	}

	/* commented out 2016-01-07 as now any field name is allowed
	list<string> fields_list = TextIndexFields(n);
	set<string> fields_set(fields_list.begin(), fields_list.end());
	if (fields_set.find(field)==fields_set.end())
	  return ShowError(msg+"text index <"+n+"> field <"+field+
			   "> not defined");
	*/
	
	textindex_fields[n].push_back(make_pair(field, txt));
      }

      list<string> rr;

      if (l.find("add-document")==0 || l.find("update-document")==0) {
	string label = l;
	size_t p = label.find("document");
	label.erase(0, p+9);
	while (label.size() && (label[0]==' ' || label[0]=='\t'))
	  label.erase(0, 1);
	p = label.find_first_of(" \t");
	if (p!=string::npos)
	  label.erase(p);
      
	int idx = LabelIndex(label);
	if (idx<0)
	  return ShowError(msg+"label <"+label+"> inexistent");

	rr = TextIndexApplyRules(n, textindex_fields[n]);
	textindex_fields.erase(n);
      }

      list<string> ll { l };
      ll.insert(ll.begin(), rr.begin(), rr.end());

      all.insert(all.end(), ll.begin(), ll.end());
    }

    if (nnn!="" && all.size())
      return LuceneOperation(nnn, all, false).first;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string> > DataBase::TextIndexRetrieve(size_t idx,
							 const string& nin) {
    return TextIndexRetrieve(Label(idx), nin);
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string> > DataBase::TextIndexRetrieve(const string& label,
							 const string& nin) {
    string msg = "DataBase::TextIndexRetrieve() : ";

    string n = nin;
    if (n=="")
      n = DefaultTextIndex();

    list<pair<string,string> > res;
    string dir = Path()+"/lucene/"+n;
    if (!DirectoryExists(dir))
      return res;

    string l = "retrieve-document "+label;
    list<string> ll { l };
    auto bl = LuceneOperation(n, ll);
    if (!bl.first)
      ShowError(msg+"failed");

    const list<string>& o = bl.second;
    for (auto i=o.begin(); i!=o.end(); i++) {
      auto p = i->find(" : ");
      if (p!=string::npos)
	res.push_back(make_pair(i->substr(0, p), i->substr(p+3)));
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::TextIndexUpdate(size_t idx, const string& nin,
				 const list<pair<string,string> >& lkv) {
    return TextIndexUpdate(Label(idx), nin, lkv);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::TextIndexUpdate(const string& label, const string& nin,
				 const list<pair<string,string> >& lkv) {
    string msg = "DataBase::TextIndexUpdate(...) : ";
    if (!open_read_write_txt)
      return ShowError(msg+"not opened for writing, use -rw=...txt...");

    string n = nin;
    if (n=="")
      n = DefaultTextIndex();

    list<string> ll;
    for (auto i=lkv.begin(); i!=lkv.end(); i++)
      ll.push_back("add-attribute "+i->first+" "+i->second);
    ll.push_back("update-document "+label);

    return LuceneOperation(n, ll).first;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string>> DataBase::TextIndexRetrieveFake(size_t idx,
							    const string&) {
    static map<size_t,list<pair<string,string>>> db;
    if (db.empty()) {
      string txtf = ExpandPath("lucene", "fake.txt");
      ifstream is(txtf);
      while (is) {
	string line;
	getline(is, line);
	if (!is)
	  break;
	size_t idx = atoi(line.c_str());
	size_t p = line.find(' ');
	line.erase(0, p+1);
	p = line.find(' ');
	string field = line.substr(0, p);
	line.erase(0, p+1);
	db[idx].push_back(make_pair(field, line));
      }
    }

    return db[idx];
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::TextIndexAllLines(const string& n, size_t i) {
    string msg = "DataBase::TextIndexAllLines("+n+","+ToStr(i)+") : ";

    //WriteLog(msg+"starting");

    list<textline_t> ret;

    const list<pair<string,string> > td = TextIndexRetrieve(i, n);

    if (td.size()) {
      double tp = -1, dur = 0;
      if (ObjectsTargetTypeContains(i, target_video) &&
	  ParentObject(i, false)>=0) {
	auto psd = ParentStartDuration(i, target_videofile);
	tp  = psd.second.first;
	dur = psd.second.second;
      }

      for (auto& ti : td) {
	textline_t aline(this, i);
	aline.index = n;
	aline.field = ti.first;

	if (tp!=-1) {
	  aline.start = tp;
	  aline.end   = tp+dur;
	}

	vector<string> pp = SplitOnWord(" # ", ti.second);
	for (auto& t : pp) {
	  string txt = t;
	  if (txt.size()>2 && txt[0]=='(' && txt[txt.size()-1]==')' &&
	      txt.find('(', 1)!=string::npos)
	    txt = " "+txt;
	  double v = 0.0;
	  size_t p = txt.rfind(" (");
	  if (p!=string::npos) {
	    v = atof(txt.substr(p+2).c_str());
	    txt.erase(p);
	  }
	  p = 0;
	  for (;;) {
	    p = txt.find("##", p);
	    if (p==string::npos)
	      break;
	    txt.erase(p, 1);
	    p++;
	  }
	  aline.add(txt, v);
	}

	ret.push_back(aline);
      }
    }

    return ret;
  }

   /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::TextIndexLines(const string& n, const string& f,
					    const ground_truth& gt) {
    string msg = "DataBase::TextIndexLines("+n+",...) : ";

    // WriteLog(msg+"starting");

    return TextIndexLines(n, f, gt.indices(1));
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::TextIndexLines(const string& n, const string& f,
					    const vector<size_t>& idxs) {
    string msg = "DataBase::TextIndexLines("+n+",...) : ";

    // WriteLog(msg+"starting");

    list<textline_t> l;

    for (auto& i : idxs) {
      textline_t tl = TextIndexLine(n, f, i);
      if (!tl.empty())
	l.push_back(tl);
    }

    return l;
  }

  /////////////////////////////////////////////////////////////////////////////

  textline_t DataBase::TextIndexLine(const string& n, const string& f, 
				     size_t i) {
    string msg = "DataBase::TextIndexLine("+n+",...) : ";

    const list<textline_t> t = TextIndexAllLines(n, i);
    for (auto& i : t)
      if (i.field==f)
	return i;

    textline_t aline(this, i);
    aline.index = n;
    aline.field = f;

    return aline;
  }

  /////////////////////////////////////////////////////////////////////////////

  textline_t DataBase::TextIndexLineOld(const string& n, const string& f, 
					size_t i) {
    string msg = "DataBase::TextIndexLine("+n+",...) : ";

    // WriteLog(msg+"starting");

    textline_t aline(this, i);
    aline.index = n;
    aline.field = f;

    const list<pair<string,string> > td = TextIndexRetrieve(i, n);

    if (td.size()) {
      double tp = -1, dur = 0;
      if (ObjectsTargetTypeContains(i, target_video) &&
	  ParentObject(i, false)>=0) {
	auto psd = ParentStartDuration(i, target_videofile);
	tp  = psd.second.first;
	dur = psd.second.second;
      }

      for (auto& ti : td)
	if (ti.first==f) {
	  if (tp!=-1) {
	    aline.start = tp;
	    aline.end   = tp+dur;
	  }

	  vector<string> pp = SplitOnWord(" # ", ti.second);
	  for (auto& t : pp) {
	    string txt = t;
	    if (txt.size()>2 && txt[0]=='(' && txt[txt.size()-1]==')')
	      txt = " "+txt;
	    double v = 0.0;
	    size_t p = txt.find(" (");
	    if (p!=string::npos) {
	      v = atof(txt.substr(p+2).c_str());
	      txt.erase(p);
	    }
	    aline.add(txt, v);
	  }

	  return aline;
	}
    }
      
    return aline;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::TextIndexSearchByLabel(size_t idx, const string& n,
					  const string& f) {
    // dunno if this really is useful for anything...

    // Lucene supports escaping special characters that are part of 
    // the query syntax. The current list special characters are
    // ^ " ~ * ? : \ + - && || ! ( ) { } [ ] 
    // To escape these character use the \ before the character. 

    string labelesc = Label(idx);
    for (size_t p=0;;) {
      size_t q = labelesc.find(':', p);
      if (q==string::npos)
	break;
      labelesc.insert(q, "\\");
      p = q+2;
    }

    list<string> in {
      "textfield "+f,
      "search-string label:"+labelesc
    };
    auto bl = LuceneOperation(n, in);
    list<string>& out = bl.second;
    if (!bl.first || out.empty())
      return "";

    string s = out.front();
    size_t p = s.find(' ');
    if (p==string::npos)
      return "";
    p = s.find(' ', p+1);
    if (p==string::npos)
      return "";
    s.erase(0, p+1);
    if (s=="null") // obs?
      return "";
    return s;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& DataBase::TextIndexDescriptionValue(const string& iname,
						    const string& vname) {
    const list<pair<string,string>>& l = TextIndexDescription(iname);
    for (auto i=l.begin(); i!=l.end(); i++)
      if (i->first==vname)
	return i->second;

    static string empty;

    return empty;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::TextIndexFields(const string& n) {
    const string& s = TextIndexDescriptionValue(n, "fields");
    vector<string> v = SplitInSpaces(s);
    list<string> res(v.begin(), v.end());

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<textsearchresult_t> DataBase::TextIndexSearch(const string& q,
						       const string& nin,
						       const string& fieldin) {
    string err = " DataBase::TextIndexSearch("+q+","+nin+","+fieldin+") : ";

    bool debug = debug_text;

    vector<textsearchresult_t> res;

    if (q.find_first_not_of(" \t\n")==string::npos) {
      ShowError(err+"empty query text");
      return res;
    }

    string n = nin;
    if (n=="")
      n = DefaultTextIndex();
    if (n=="") {
      ShowError(err+"no existing text index");
      return res;
    }

    string field = fieldin;
    if (field=="")
      field = TextIndexDescriptionValue(n, "textfield");
    if (field=="") {
      ShowError(err+"textfield not specified");
      return res;
    }

    list<string> in {
      "textfield "+field,
      "search-string "+q
    };
    auto bl = LuceneOperation(n, in);
    list<string>& out = bl.second;
    if (!bl.first || out.empty()) {
      // ShowError(err+"empty result");
      return res;
    }

    for (auto i=out.begin(); i!=out.end(); i++) {
      string s = *i;
      size_t p = s.find(' ');
      if (p==string::npos) {
	ShowError(err+"missing space 1");
	return res;
      }
      string lab = s.substr(0, p);
      s.erase(0, p+1);
      while (s[0]==' ')
	s.erase(0, 1);

      p = s.find(' ');
      if (p==string::npos) {
	ShowError(err+"missing space 2");
	return res;
      }
      string txt = s.substr(p+1);
      while (txt[0]==' ')
	txt.erase(0, 1);

      s.erase(p);
      if (s=="null") {
	ShowError(err+"null");
	return res;
      }

      int idx = LabelIndex(lab);
      float val = atof(s.c_str());
      bool rok = OkWithRestriction(idx);

      if (debug)
	cout << "<" << lab << "> #" << idx << " " << val << " \""
	     << txt << "\" " << (rok?1:0) << endl;

      if (rok)
	res.push_back(textsearchresult_t(idx, val, txt));
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::TextIndexApplyRules(const string& n,
					     const list<pair<string,string>>& f
					     ) {
    string msg = "DataBase::TextIndexApplyRules() : ";

    bool debug = debug_text;

    typedef string (DataBase::*func_t)(const string&, const vector<string>&);
    map<string,func_t> flist {
      {   "aspell",      &DataBase::TextIndex_aspell },
	{ "aspell_keep", &DataBase::TextIndex_aspell_keep }
    };

    list<string> fields_list = TextIndexFields(n);
    set<string> fields_set(fields_list.begin(), fields_list.end());

    list<string> rules = TextIndexRules(n);
    list<string> res;

    map<string,string> ff(f.begin(), f.end());

    for (auto i=rules.begin(); i!=rules.end(); i++) {
      string r = *i;
      size_t p = r.find('=');
      if (p==string::npos) {
	ShowError(msg+"\""+*i+"\" '=' not found") ;
	continue;
      }
    
      string dst = r.substr(0, p), str;
      r.erase(0, p+1);

      bool done = false;
      if (r.find('+')!=string::npos) {
	done = true;
	vector<string> a = SplitInSomething("+", false, r);
	for (auto j=a.begin(); j!=a.end(); j++) {
	  if (ff.find(*j)==ff.end()) {
	    ShowError(msg+"\""+*i+"\" field \""+*j+"\" not found");
	    continue;
	  }
	  str += (str==""?"":" ")+ff[*j];
	}
      }

      if (!done) {
	size_t p = r.find('(');
	if (p!=string::npos) {
	  done = true;
	  string fname = r.substr(0, p);
	  auto fp = flist.find(fname);
	  if (fp==flist.end()) {
	    ShowError(msg+"\""+*i+"\" function \""+fname+"()\" not found");
	    continue;
	  }

	  string arg = r.substr(p+1);
	  while (arg.size() && (arg[arg.size()-1]==' ' ||
				arg[arg.size()-1]=='\t'))
	    arg.erase(arg.size()-1);
	  if (!arg.size() || arg[arg.size()-1]!=')') {
	    ShowError(msg+"\""+*i+"\" error parsing (...)");
	    continue;
	  }
	  arg.erase(arg.size()-1);
	  vector<string> av = SplitInCommas(arg);
	  if (ff.find(av[0])==ff.end()) {
	    ShowError(msg+"\""+*i+"\" field \""+av[0]+"\" not found");
	    continue;
	  }
	  string strin = ff[av[0]];
	  auto apa = av.begin(), apb = apa;
	  av.erase(apa, ++apb);
	  str = (this->*fp->second)(strin, av);
	}
      }
    
      if (!done) {
	ShowError(msg+"\""+*i+"\" could not be parsed");
	continue;
      }
      
      if (fields_set.find(dst)==fields_set.end()) {
	ShowError(msg+"text index <"+n+"> field <"+dst+"> not defined");
	continue;
      }
      
      string line = "add-attribute "+dst+" "+str;

      res.push_back(line);
      ff[dst] = str;

      if (debug)
	cout << TimeStamp() << msg << "\""+*i+"\" => " << line << endl;
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::TextIndexRules(const string& n) {
    list<string> res;
    const list<pair<string,string>>& l = TextIndexDescription(n);
    for (auto i=l.begin(); i!=l.end(); i++)
      if (i->first.substr(0, 5)=="rule#") {
	string r = i->second;
	for (;;) {
	  size_t p = r.find_first_of(" \t");
	  if (p==string::npos)
	    break;
	  r.erase(p, 1);
	}
	res.push_back(r);
      }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::TextIndex_aspell_common(const string& in,
					   const vector<string>& a,
					   bool keep) {
    string msg = "TextIndex_aspell_common() : ";

    bool debug = debug_text;

    if (a.size()!=1)
      return ShowErrorS(msg+"a.size()!=1");

#ifdef HAVE_ASPELL_H
    AspellConfig *spell_config = new_aspell_config();
    aspell_config_replace(spell_config, "lang", a[0].c_str());

    AspellCanHaveError *possible_err = new_aspell_speller(spell_config);
    if (aspell_error_number(possible_err))
       return ShowErrorS(msg+aspell_error_message(possible_err));

    AspellSpeller *spell_checker = to_aspell_speller(possible_err);

    vector<string> wordsin = SplitInSpaces(in), wordsout;
    for (auto ii=wordsin.begin(); ii!=wordsin.end(); ii++) {
      string w = *ii;
      for (;;) {
	size_t p = w.find_first_of(".,;:+-*()[]{}\"'`");
	if (p==string::npos)
	  break;
	w.erase(p, 1);
      }
      if (w=="")
	continue;

      int correct = aspell_speller_check(spell_checker, w.c_str(), -1);
      if (correct) {
	wordsout.push_back(w);
	if (debug)
	  cout << TimeStamp() << msg << "\"" << w << "\" is correct" << endl;
	continue;
      }

      const AspellWordList *sugg = aspell_speller_suggest(spell_checker,
							  w.c_str(), -1);
      AspellStringEnumeration *elem = aspell_word_list_elements(sugg);
      string s;
      size_t n = 0, nmax = 1;
      const char *word = NULL;
      while ((word = aspell_string_enumeration_next(elem))) {
	s += (s==""?"":" ")+string(word);
	if (nmax && ++n>=nmax)
	  break;
      }
      delete_aspell_string_enumeration(elem);

      if (debug)
	cout << TimeStamp() << msg << "\"" << w << "\" is replaced by \"" << s << "\"" << endl;

      if (keep || s=="")
	wordsout.push_back(w);

      if (s!="")
	wordsout.push_back(s);
    }

    delete_aspell_speller(spell_checker);
    //delete_aspell_can_have_error(possible_err);
    delete_aspell_config(spell_config);

    string out = JoinWithString(wordsout, " ");

    return out;

#else // HAVE_ASPELL_H
    bool foo = keep || debug;
    ShowError(msg+"aspell not available here"+(foo?"!":"!"));
    return in;
#endif // HAVE_ASPELL_H
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::TextIndex_aspell(const string& in,
				    const vector<string>& a) {
    return TextIndex_aspell_common(in, a, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::TextIndex_aspell_keep(const string& in,
					 const vector<string>& a) {
    return TextIndex_aspell_common(in, a, true);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedIndexComplex(const XmlDom& xml) {
    string msg = "DataBase::DescribedIndexComplex() : ";

    bool debug = DebugFeatures()>2;

    string outputpat = xml.Property("outputname");
    if (outputpat=="")
      return ShowError(msg+"attribute outputname should be set");

    vector<vector<list<pair<string,string> > > > f;
    vector<size_t> m;

    XmlDom txt;
    for (XmlDom l=xml.FirstElementChild("featurepartlist"); l;
	 l=l.NextElement("featurepartlist")) {
      f.push_back(vector<list<pair<string,string> > >());
      for (XmlDom p=l.FirstElementChild("featurepart"); p;
	   p=p.NextElement("featurepart")) {
	list<pair<string,string> > a = p.Properties();
	for (auto i=a.begin(); i!=a.end(); i++) {
	  if (debug)
	    cout << i->first << "=\"" << i->second << "\"" << endl;
	  if (i->first=="nameparts") {
	    vector<string> kvl = SplitInSpaces(i->second);
	    for (auto j=kvl.begin(); j!=kvl.end(); j++) {
	      if (*j!="") {
		pair<string,string> kv = SplitKeyEqualValueNew(*j);
		if (debug)
		  cout << "   \"" << kv.first << "\"=\"" << kv.second
		       << "\"" << endl;
	      }
	    }
	  }
	}
	f.back().push_back(a);
	if (debug)
	  cout << "-----" << endl;
      }
      for (size_t i=0; i<m.size(); i++)
	m[i] *= f.back().size();
      m.push_back(1);
      if (debug)
	cout << "============" << endl;

      txt = l;
    }

    if (debug)
      cout << "#";

    size_t nn = 1;
    vector<size_t> n;
    for (auto a=f.begin(); a!=f.end(); a++) {
      if (debug)
	cout << " " << a->size();
      n.push_back(a->size());
      nn *= a->size();
    }
    if (debug)
      cout << " total=" << nn << endl;

    list<RegExp*> regexps;

    for (;;) {
      txt = txt.Next();
      if (!txt)
	break;
      if (!txt.IsText())
	continue;

      string s = txt.Content();
      vector<string> ss = SplitInSpaces(s);
      for (size_t i=0; i<ss.size(); i++)
	if (ss[i]!="")  {
	  string sx = ss[i];
	  if (sx.size()<3 || sx[0]!='/' || sx[sx.size()-1]!='/')
	    sx = "^"+sx+"$";
	  else
	    sx = sx.substr(1, sx.size()-2);
	  if (debug)
	    cout << "   [" << ss[i] << "] -> [" << sx << "]" << endl;
	  RegExp *re = new RegExp(sx);
	  if (re->ok())
	    regexps.push_back(re);
	  else {
	    delete re;
	    return ShowError(msg+"regexp <"+sx+"> failed");
	  }
	}
    }

    set<string> outnames;

    for (size_t i=0; i<nn; i++) {
      if (debug)
	cout << i;
      list<pair<string,string> > p;
      for (size_t b=0; b<n.size(); b++) {
	size_t a = (i/m[b])%n[b];
	if (debug)
	  cout << " " << a;
	p.insert(p.end(), f[b][a].begin(), f[b][a].end());
      }
      p.push_back(make_pair("outputname", outputpat));

      map<string,string> e, var;
      for (auto j=p.begin(); j!=p.end(); j++) {
	if (debug)
	  cout << " \"" << j->first << "\"=\"" << j->second << "\"";
	if (j->first=="nameparts") {
	  vector<string> v = SplitInSpaces(j->second);
	  for (size_t k=0; k<v.size(); k++) {
	    if (v[k]!="") {
	      pair<string,string> kv = SplitKeyEqualValueNew(v[k]);
	      if (var.find(kv.first)!=var.end())
		return ShowError(msg+kv.first+" multiply defined variable");
	      var[kv.first] = kv.second;
	    }
	  }

	} else {
	  string s = j->second;
	  for (;;) {
	    size_t q = s.find('$');
	    if (q==string::npos)
	      break;
	    string w = s.substr(q, 2);
	    if (var.find(w)==var.end())
	      return ShowError(msg+w+" undefined variable");
	    s.replace(q, 2, var[w]);
	  }
	  if (j->first=="options")
	    e[j->first] += (e[j->first]==""?"":" ")+s;
	  else if (e.find(j->first)!=e.end())
	    return ShowError(msg+j->first+" multiply defined property");
	  else
	    e[j->first] = s;
	}
      }

      if (debug)
	cout << " => ";
      
      string on = e["outputname"];
      if (on=="")
	return ShowError(msg+"empty outputname");

      bool found = regexps.empty();
      for (auto j=regexps.begin(); !found && j!=regexps.end(); j++)
	if ((*j)->match(on))
	  found = true;
      
      if (!found) {
	if (debug)
	  cout << on << " NOT found in the regexps" << endl;
	continue;
      }

      if (debug)
	  cout << on << " FOUND in the regexps (or no regexps)" << endl;

      if (outnames.find(on)!=outnames.end())
	return ShowError(msg+"outputname \""+on+"\" multiply defined");
      outnames.insert(on);

      XmlDom vdoc = XmlDom::Doc();
      XmlDom vfea = vdoc.Root("feature");
      for (auto j=e.begin(); j!=e.end(); j++) {
	if (debug)
	  cout << " \"" << j->first << "\"=\"" << j->second << "\"";
	vfea.Prop(j->first, j->second);
      }

      bool ok = DescribedIndex(vfea);
      vdoc.DeleteDoc();

      if (!ok)
	return ShowError(msg+"failed");

      if (debug)
	cout << endl;
    }

    for (auto i=regexps.begin(); i!=regexps.end(); i++)
      delete *i;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedIndex(const XmlDom& xml) {
    xmlNodePtr node = xml.node;
    string debug_or_fast = "fast";

    string msg = "DataBase::DescribedIndex() : ";

    if (xml.FirstChild())
      return DescribedIndexComplex(xml);

    string fn = GetProperty(node, "name");
    string on = GetProperty(node, "outputname");
    size_t odp = on.find('#');
    if (fn=="" && odp!=string::npos) {
      string otherdbn = on.substr(0, odp);
      Picsom()->AddAllowedDataBase(otherdbn);
      DataBase *otherdb = Picsom()->FindDataBase(otherdbn);
      if (!otherdb)
	return ShowError(msg+"database <"+otherdbn+"> in <"+on+"> not found");

      on.erase(0, odp+1);
      if (on=="")
	on = ".*";

      XmlDom ext = otherdb->FeaturesExpandedXML();
      RegExp re(on);
      bool found = false;
      for (XmlDom f=ext.FirstElementChild("feature"); f;
	   f=f.NextElement("feature")) {
	string oon = f.Property("outputname");
	if (oon!="" && re.match(oon)) {
	  found = true;
	  if (!DescribedIndex(f))
	    return ShowError(msg+"outputname=\""+on+"\" failed with \""+oon+
			     "\" from <"+otherdb->Name()+">");
	}
      }
      if (found)
	return true;

      return ShowError(msg+"extraction/feature with outputname=\""+on+
		       "\" not found in <"+otherdb->Name()+">");
    }

    if (fn=="")
      return ShowError(msg+"no or empty name=\"\"");

    if (on=="")
      on = fn;

    if (featurefile_to_method.find(on)!=featurefile_to_method.end())
      ShowError(msg+"duplicate described featurefile <"+on+">");
    featurefile_to_method.insert(pair<string,string>(on, fn));
    if (DebugFeatures())
      cout << TimeStamp() << msg+"added file->method : "+on+"->"+fn << endl;

    string mn = GetProperty(node, "indexname");
    if (mn=="")
      mn = GetProperty(node, "mapname");
    if (mn=="")
      mn = on;
    if (index_to_featurefile.find(mn)!=index_to_featurefile.end())
      ShowError(msg+"duplicate described index <"+mn+">");
    index_to_featurefile.insert(pair<string,string>(mn, on));
    if (DebugFeatures())
      cout << TimeStamp() << msg+"added index->file  : "+mn+"->"+on << endl;

    msg += mn+" : ";

    target_type tt = SolveTargetTypes(GetProperty(node, "target"));
    if (tt==target_no_target)
      return ShowError(msg+"no or erroneous target=\"\"");

    string use = GetProperty(node, "use");
    if (use!="" && !IsAffirmative(use)) {
      /*if (DebugFeatures())
	cout << endl;*/
      return true;
    }

    string subdir = GetProperty(node, "subdir");
    if (DebugFeatures())
      cout << TimeStamp() << msg+"subdir=\"" << subdir << "\"" << endl;

    string sn = GetProperty(node, "segmentation");
    if (sn=="region.box" || (true && sn=="face.box")) {
      /*if (DebugFeatures())
	cout << endl;*/
      return true; // this is not wise
    }

    DataBase *db = this;
    string dbname = GetProperty(node, "database");
    if (dbname!="" && dbname!=name) {
      db = Picsom()->FindDataBase(dbname);
      if (!db)
	return ShowError(msg+"database <"+dbname+"> not found");
    }

    string cmd = "*picsom-features-internal*";
    vector<string> cmdvec;
    cmdvec.push_back(cmd);

    if (subdir!="")
      cmdvec.push_back("-n"+subdir);

    vector<string> switches = SplitInSpaces(GetProperty(node, "switches"));
    cmdvec.insert(cmdvec.end(), switches.begin(), switches.end());

    Index *f = FindIndex(mn, "", false, true);
    if (!f)
      return ShowError(msg+"FindIndex() failed");

    string detections = GetProperty(node, "detections");
    if (DebugFeatures())
      cout << TimeStamp() << msg+"detections=\"" << detections << "\"" << endl;
    f->SetProperty("detections", detections);

    string features = GetProperty(node, "features");
    if (DebugFeatures())
      cout << TimeStamp() << msg+"features=\"" << features << "\"" << endl;
    f->SetProperty("features", features);

    string prodquant = GetProperty(node, "prodquant");
    if (DebugFeatures())
      cout << TimeStamp() << msg+"prodquant=\"" << prodquant << "\"" << endl;
    f->SetProperty("prodquant", prodquant);

    string recipe = GetProperty(node, "recipe");
    if (DebugFeatures())
      cout << TimeStamp() << msg+"recipe=\"" << recipe << "\"" << endl;
    if (recipe!="")
      f->SetProperty("recipe", recipe);

    f->FeatureTarget(tt);

    if (false && sn!="" && !PicSOM::TargetTypeContains(tt, target_segment))
      return ShowError(msg+"segmentation set but target ["+
		       TargetTypeString(tt)+"] is not segment");

    if (false && sn=="" && PicSOM::TargetTypeContains(tt, target_segment))
      return ShowError(msg+"target is segment but segmentation not set");

    if (sn!="" && sn!="any") { // I yet dunno what to do with "any"... 
      if (described_segmentations.find(sn)==described_segmentations.end())
	return ShowError(msg+"described segmentation <"+sn+"> not found");

      // obs! I think this is (or at least should be) outdated way...
      // segmentations should be called explicitly before feature extractions
      // for (auto i=described_segmentations[sn].begin();
      //   i!=described_segmentations[sn].end(); i++)
      // if (i->first=="command")
      //   f->SegmentationCommand(i->second);

      // cmdvec.push_back("-Ib");
      cmdvec.push_back("-IB");
      cmdvec.push_back(sn);
    }

    cmdvec.push_back("-OR");
    cmdvec.push_back(fn);

    vector<string> options = SplitInSpaces(GetProperty(node, "options"));
    for (vector<string>::const_iterator oi=options.begin();
	 oi!=options.end(); oi++)
      cmdvec.push_back(db->PathifyFeatureOption(*oi, fn));

    /*if (DebugFeatures())
      cout << endl;*/

    f->Subdir(subdir);
    f->FeaturesCommand(cmdvec, DebugFeatures());

    // for (size_t i=0; i<Nindices(); i++)
    //   cout << "i=" << i << " " << GetIndex(i)->Name() << "   ";
    // cout << endl;

    FeaturesExpandedXML().AddCopy(xml);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::PathifyFeatureOption(const string& o,
					const string& fname) const {
    size_t e = o.find('=');
    if (e==string::npos)
      return o;

    string key = o.substr(0, e), val = o.substr(e+1);

    set<string> keys;
    keys.insert("histbins");    // ColorSIFT
    keys.insert("z-normalize"); // ColorSIFT
    keys.insert("codebook");    // ColorSIFT
    keys.insert("gmm");         // vlfeat
    keys.insert("kmeans");      // vlfeat
    keys.insert("model");       // caffe torch
    keys.insert("densetraj");   // trajectory
    keys.insert("hog");         // trajectory
    keys.insert("hof");         // trajectory
    keys.insert("mbhx");        // trajectory
    keys.insert("mbhy");        // trajectory

    if (keys.find(key)==keys.end())
      return o;

    if (val[0]=='/' || val.find("./")==0 || val.find("../")==0)
      return o;

    string ret = features+"/"+val;

    if (!FileExists(ret))
      ret = Picsom()->RootDir("share")+"/"+val;

    if (!FileExists(ret))
      ret = Picsom()->RootDir("share", true)+"/"+val;

    if (!FileExists(ret) && key=="model" && (fname=="caffe"||fname=="torch")) {
      ret = ExpandPath(fname, "models")+"/"+val;
      if (fname=="caffe" && !DirectoryExists(ret) && IsLocalDiskDb()) {
	string prototxt = ExpandPath("caffe/models")+"/"+val+"/prototxt";
	string model    = ExpandPath("caffe/models")+"/"+val+"/model";
	string mean     = ExpandPath("caffe/models")+"/"+val+"/mean.png";
	string map      = ExpandPath("caffe/models")+"/"+val+"/idxmap.txt";
      
	RootlessDownloadFile(prototxt, true);
	RootlessDownloadFile(model,    true);
	RootlessDownloadFile(mean,     true);
	RootlessDownloadFile(map,      false);
      }

      if (DirectoryExists(ret))
	return key+"="+ret;
    }

    if (!FileExists(ret))
      ret = val;

    return key+"="+ret;
  }

///////////////////////////////////////////////////////////////////////////////

double DataBase::NormalizeGazeCoordInCollage(double v, const string& n) {
  string msg = "DataBase::NormalizeGazeCoordInCollage() : ";

  if (gaze_coord_norm.empty()) {
    string fname = ExpandPath("features", "gaze-coord-norm.txt");
    //string fname = ExpandPath("features", "gaze-coord-norm-left-test01-user01.txt");
    string f = FileToString(fname);
    vector<string> l = SplitInSomething("\n", true, f);
    for (size_t i=0; i<l.size(); i++) {
      if (l[i][0]=='#' || l[i].find_first_not_of(" \t\r")==string::npos)
        continue;
      // cout << "[" << l[i] << "]" << endl;

      vector<string> a = SplitInSpaces(l[i]);
      gaze_coord_norm[a[0]] = make_pair(atof(a[1].c_str()), atof(a[2].c_str()));
    }
  }

  gaze_coord_norm_t::const_iterator i = gaze_coord_norm.find(n);
  if (i==gaze_coord_norm.end()) {
    ShowError(msg+"normalization <"+n+"> not found");
    return v;
  }

  return (v-i->second.first)/i->second.second;
}

///////////////////////////////////////////////////////////////////////////////

double DataBase::GazeRelevance(const vector<float>& vin, const string& n) {
  string msg = "DataBase::GazeRelevance() : ";

  bool debug = false;

  static simple::SubSpace ss;
  static bool first = true;
  if (first) {
    first = false;
    string f = "gaze-relevance-"+n+".ss";
    //string f = "gaze-relevance-left-test01-user01-"+n+".ss";

    string fname = ExpandPath("features", f);
    if (FileExists(fname)) {
      WriteLog("Reading subspace file <"+ShortFileName(fname)+">");
      ss.Read(fname.c_str());
      if (debug) {
        int l = ss.VectorLength();
        cout << "  VectorLength()=" << l << " BaseSize()=" << ss.BaseSize()
             << endl;

        cout << "  mean=" << ss.Mean()[0] << " " << ss.Mean()[1] << " "
             << ss.Mean()[2] << " " << ss.Mean()[3] << " ... " << ss.Mean()[l-1]
             << endl;
     
        cout << "  coeff=" << ss[0][0] << " " << ss[0][1] << " "
             << ss[0][2] << " " << ss[0][3] << " ... " << ss[0][l-1]
             << endl;
      }
    }
    else if (!isdummydb)
      ShowError("Failed to locate subspace file <"+f+">");
  }

  if (ss.VectorLength()!=(int)vin.size()+1)
    return 0.0;

  FloatVector v(vin.size(), &vin[0]);
  v.Insert(0, 1.0);

  FloatVector p = ss.Coefficients(v, true);
  if (p.Length()!=1)
    return -1.0;

  float z = 0.0, f = 0.0;

  z = p[0];
  f = 1.0/(1+exp(-z));

  if (debug)
    cout << TimeStamp() << msg << "z=" << z << " f=" << f << endl;

  return f;
}

///////////////////////////////////////////////////////////////////////////////

double DataBase::PointerRelevance(const vector<float>& vin, const string& n) {
  string msg = "DataBase::PointerRelevance() : ";

  bool debug = false;

  static simple::SubSpace ss;
  static bool first = true;
  if (first) {
    first = false;
    string f = "pointer-relevance-"+n+".ss";
    string fname = ExpandPath("features", f);
    if (FileExists(fname)) {
      WriteLog("Reading subspace file <"+ShortFileName(fname)+">");
      ss.Read(fname.c_str());
      if (debug) {
        int l = ss.VectorLength();
        cout << "  VectorLength()=" << l << " BaseSize()=" << ss.BaseSize()
             << endl;

        cout << "  mean=" << ss.Mean()[0] << " " << ss.Mean()[1] << " "
             << ss.Mean()[2] << " " << ss.Mean()[3] << " ... " << ss.Mean()[l-1]
             << endl;
     
        cout << "  coeff=" << ss[0][0] << " " << ss[0][1] << " "
             << ss[0][2] << " " << ss[0][3] << " ... " << ss[0][l-1]
             << endl;
      }
    }
    else if (!isdummydb)
      ShowError("Failed to locate subspace file <"+f+">");
  }

  if (ss.VectorLength()!=(int)vin.size()+1)
    return 0.0;

  FloatVector v(vin.size(), &vin[0]);
  v.Insert(0, 1.0);

  FloatVector p = ss.Coefficients(v, true);
  if (p.Length()!=1)
    return -1.0;

  float z = p[0], f = 1.0/(1+exp(-z));

  return f;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadGazeRelevanceGroupData() {
    bool debug = false;

    string msg = "DataBase::ReadGazeRelevanceGroupData() : ";

    gaze_relevance_group_data_pos.clear();
    gaze_relevance_group_data_neg.clear();

    string fname = "eye_movement_examples_newregressor_groups.csv";
    ifstream is(ExpandPath(fname).c_str());
    while (is) {
      string line;
      getline(is, line);
      if (!is)
	break;

      vector<string> l = SplitInCommas(line);
      if (l.size()<3)
	return ShowError(msg+"splitting a line faile");

      bool   pos = atoi(l[0].c_str());
      size_t grp = atoi(l[1].c_str());
      float    v = atof(l[2].c_str());

      if (v==-1) v = 0.15;

      map<size_t,vector<float> >& data = pos ?
	gaze_relevance_group_data_pos : gaze_relevance_group_data_neg;
   
      data[grp].push_back(v);
    }

    if (debug) {
      cout << endl << msg << "statistics" << endl;
      for (size_t d=0; d<2; d++) {
	bool pos = d==0;
	map<size_t,vector<float> >& data = pos ?
	  gaze_relevance_group_data_pos : gaze_relevance_group_data_neg;
	for (size_t j=0; j<6; j++)
	  cout << (pos?"POS ":"NEG ") << j << " : " << data[j].size()
	       << endl;
      }
      cout << endl;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::GazeRelevanceGroupValue(RandVar& rv, bool pos, size_t n) {
    if (gaze_relevance_group_data_pos.empty() &&
	gaze_relevance_group_data_neg.empty())
      ReadGazeRelevanceGroupData();

    size_t g = 0;
    if (n>=1)  g++;
    if (n>=2)  g++;
    if (n>=4)  g++;
    if (n>=7)  g++;
    if (n>=10) g++;

    map<size_t,vector<float> >& data = pos ?
      gaze_relevance_group_data_pos : gaze_relevance_group_data_neg;

    int l = data[g].size();
    int m = rv.RandomInt(l);

    return data[g][size_t(m)];
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedSegmentation(const XmlDom& xml) {
    xmlNodePtr node = xml.node;
    string debug_or_fast = "fast";

    string msg = "DataBase::DescribedSegmentation() ";

    string name = GetProperty(node, "name");
    if (name=="")
      return ShowError(msg+": no or empty name=\"\"");

    if (described_segmentations.find(name)!=described_segmentations.end())
      ShowError(msg+"duplicate described segmentation <"+name+">");
    described_segmentations[name].clear();
    msg += name+" : ";

    string use = GetProperty(node, "use");
    if (use!="" && !IsAffirmative(use))
      return true;

    string method = GetProperty(node, "method");
    if (method=="")
      return ShowError(msg+"no or empty method=\"\"");

    string switches  = GetProperty(node, "switches");
    string options   = GetProperty(node, "options");
    string target    = GetProperty(node, "target");
    string addlabels = GetProperty(node, "addlabels");

    string cmd = "*picsom-segmentation-internal*";
    vector<string> cmdvec;
    cmdvec.push_back(cmd);

    // "-Ob" or "-OB" should now be explicitly in switches
    // cmdvec.push_back("-Ob");
    vector<string> args = SplitInSpaces(switches);
    if (args.size() && args[0]!="")
      cmdvec.insert(cmdvec.end(), args.begin(), args.end());

    args = SplitInSpaces(method);
    if (args.size()!=1)
      return ShowError(msg+"method \""+method+"\" should be a bare word");
    cmdvec.insert(cmdvec.end(), args.begin(), args.end());

    args = SplitInSpaces(options);
    if (args.size() && args[0]!="")
      cmdvec.insert(cmdvec.end(), args.begin(), args.end());

    string cmdvecstr = JoinWithString(cmdvec, " ");

    list<pair<string,string>> l {
      make_pair(  "name",      name),
	make_pair("method",    method),
	make_pair("switches",  switches),
	make_pair("options",   options),
	make_pair("command",   cmdvecstr),
	make_pair("target",    target),
	make_pair("addlabels", addlabels),
	};
    
    WriteLog("Registered segmentation <"+name+"> = ["+ToStr(cmdvec)+"]");
    if (DebugSegmentation())
      for (auto i=l.begin(); i!=l.end(); i++)
	WriteLog("  "+i->first+"=\""+i->second+"\"");

    described_segmentations[name].insert(described_segmentations[name].end(),
					 l.begin(), l.end());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedObjectInsertion(const XmlDom& node) {
    string msg = "DataBase::DescribedObjectInsertion() : ";
 
    bool debug = true;

    string use = node.Property("use");
    if (use!="" && !IsAffirmative(use))
      return true;

    string desname = node.Property("name");
    if (desname=="")
      return ShowError(msg+"name should be defined");

    string type = node.Property("type");
    if (type=="")
      return ShowError(msg+"type should be defined");

    if (described_objectinsertions.find(name)!=described_objectinsertions.end())
      return ShowError(msg+"object insertion method with name <"+name+
		       "> already exists");

    string dbname;

    list<pair<string,string> > l, prop = node.Properties();
    for (auto p=prop.begin(); p!=prop.end(); p++) {
      string name = p->first, val = p->second;

      if (name=="use")
	continue;

      if (debug) 
	cout << TimeStamp() << msg << desname << " : <" << name << ">=<"
	     << val << ">" << endl;

      l.push_back(make_pair(name, val));
    }

    described_objectinsertions[desname] = l;

    WriteLog("Registered object insertion method <"+desname+">");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedDetection(const XmlDom& node) {
    string msg = "DataBase::DescribedDetection() : ";
 
    bool expand = true, debug = DebugDetections();

    string use = node.Property("use");
    if (use!="" && !IsAffirmative(use))
      return true;

    string desname = node.Property("name");
    if (desname=="")
      return ShowError(msg+"name should be defined");

    string type = node.Property("type");
    if (type=="")
      return ShowError(msg+"type should be defined");

    if (described_detections.find(name)!=described_detections.end())
      return ShowError(msg+"detection with name <"+name+"> already exists");

    string dbname;

    list<pair<string,string> > l;

    for (_xmlAttr *p = node.node->properties; p; p=p->next) {
      string name = (char*)p->name, val;
      if (p->children && p->children->content)
	val = (char*)p->children->content;

      if (name=="use")
	continue;

      if (name=="database")
	dbname = val;

      if (name=="features" || name=="detectors" || name=="evaluators")
	for (;;) {
	  size_t p = val.find_first_of(" \t\n");
	  if (p==string::npos)
	    break;
	  size_t q = val.find_first_not_of(" \t\n", p);
	  val.erase(p, q-p);
	}
	  
      if (name=="class") {
	DataBase *db = this;
	if (dbname!="") {
	  string dbnamex = dbname+"(frag=min)";
	  Picsom()->AddAllowedDataBase(dbname);
	  db = Picsom()->FindDataBaseEvenNew(dbnamex, false);
	  if (!db)
	    return ShowError(msg+"database <"+dbnamex+"> not found");
	}

	// upto version 2.765 metaclassfile was solved in current db
	if (db->IsMetaClassFile(val)) {
	  string mcname = dbname=="" ? val : dbname+"#"+val;
	  l.push_back(make_pair("metaclassfile", val));
	  list<string> cl_list = db->SplitClassNames(val);
	  val = CommaJoin(cl_list);
	}
      }

      if (debug) 
	cout << TimeStamp() << msg << desname << " : <"
	     << name << ">=<" << val << ">" << endl;

      l.push_back(make_pair(name, val));
    }

    if (expand)
      return ExpandAndStoreDescribedDetections(desname, l);
    else {
      described_detections[desname] = l;
      return true;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExpandAndStoreDescribedDetections(const string& desnamein,
						   const list<pair<string,
						   string> >& lin) {
    string msg = "DataBase::ExpandAndStoreDescribedDetections() <"+
      desnamein+"> : ";

    string key;
    for (auto i=lin.begin(); key=="" && i!=lin.end(); i++)
      if (i->first!="class" && i->first!="detectors" && i->first!="multi" &&
	  i->first!="features" && i->first!="evaluators" &&
	  i->second.find(',')!=string::npos)
	key = i->first;
    
    if (key!="") {
      string desname = desnamein, pat = "${"+key+"}", val;
      size_t p = desname.find(pat);
      if (p==string::npos)
	return ShowError(msg+"attribute "+pat+" not found in <"+desname+">");

      // obs! until 2015-05-21 old name remained...
      list<pair<string,string> > l;
      for (auto i=lin.begin(); i!=lin.end(); i++)
	if (i->first==key)
	  val = i->second;
	else if (i->first!="name")
	  l.push_back(*i);

      if (val.find('{')==string::npos)
	val = "{"+val+"}";

      list<string> a = BraceCommaExpand(val);
      for (auto i=a.begin(); i!=a.end(); i++) {
	string name = desname;
	name.replace(p, pat.size(), *i);
	list<pair<string,string> > lx = l;
	lx.push_back(make_pair(key, *i));
	lx.push_back(make_pair("name", name)); // added 2015-05-21
	if (!ExpandAndStoreDescribedDetections(name, lx))
	  return false;
      }

      return true;
    }

    if (described_detections.find(desnamein)!=described_detections.end())
      return ShowError(msg+"<"+desnamein+"> already defined");

    if (desnamein.find("$")!=string::npos)
      return ShowError(msg+"unsolved ${attribute} in <"+desnamein+">");

    described_detections[desnamein] = lin;
   
    if (DebugDetections())
      WriteLog("Added description for detection <"+desnamein+">");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,list<pair<string,string> > > >
  DataBase::ExpandDescription(const string& namein,
			      const list<pair<string,string> >& lin,
			      bool debug) {
    string msg = "DataBase::ExpandDescription() <"+namein+"> : ";

    auto lx = ExpandDescriptionVariables(lin, false, debug);
    map<string,string> ml(lx.begin(), lx.end());
    string name = ml["name"];
    if (name!=namein)
      WriteLog(msg+"name changed <"+namein+"> -> <"+name+">");

    auto ll = ExpandDescriptionInner(name, lx, debug);

    for (auto& l : ll)
      l.second = ExpandDescriptionVariables(l.second, true, debug);

    return ll;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string> >
  DataBase::ExpandDescriptionVariables(const list<pair<string,string> >& lin,
				       bool expand_all, bool debug) {
    string msg = "DataBase::ExpandDescriptionVariables() : ";

    auto l = lin;

    for (;;) {
      bool changed = false;
      map<string,string> m(l.begin(), l.end());
      for (auto& i : l) {
	string& v = i.second;
	size_t p = v.find("${");
	if (p!=string::npos) {
	  size_t q = v.find("}", p);
	  if (q!=string::npos) {
	    string s = v.substr(p+2, q-p-2);
	    if ((expand_all||s.substr(0, 1)!="_") && m.find(s)!=m.end()) {
	      string vo = v;
	      bool this_changed = v.substr(p, q-p+1)!=m[s];
	      changed |= this_changed;
	      v.replace(p, q-p+1, m[s]);
	      if (debug && this_changed)
		WriteLog(msg+i.first+" : "+vo+" -> "+v);
	    }
	  }
	}
      }
      if (!changed)
	  break;
    }
    return l;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,list<pair<string,string> > > >
  DataBase::ExpandDescriptionInner(const string& namein,
				   const list<pair<string,string> >& lin,
				   bool debug) {
    string msg = "DataBase::ExpandDescriptionInner() <"+namein+"> : ";

    string key;
    for (auto i=lin.begin(); key=="" && i!=lin.end(); i++)
      if (i->first.substr(0, 1)=="_")
	key = i->first;
    
    if (debug)
      WriteLog(msg+"key=<"+key+">");

    list<pair<string,list<pair<string,string> > > > ret;

    if (key!="") {
      string name = namein, pat = "${"+key+"}", val;
      size_t p = name.find(pat);
      if (p==string::npos) {
	ShowError(msg+"attribute "+pat+" not found in <"+name+">");
	return ret;
      }

      list<pair<string,string> > l;
      for (auto i : lin)
	if (i.first==key) {
	  val = i.second;
	  if (debug)
	    WriteLog(msg+"read   <"+key+">=<"+val+">");
	} else if (i.first!="name") {
	  l.push_back(i);
	  if (debug)
	    WriteLog(msg+"copied <"+i.first+">=<"+i.second+">");
	}
      
      if (val.find('{')==string::npos)
	val = "{"+val+"}";

      list<string> a = BraceCommaExpand(val);
      for (auto i=a.begin(); i!=a.end(); i++) {
	string xname = name;
	xname.replace(p, pat.size(), *i);
	list<pair<string,string> > lx = l;
	lx.push_back({key, *i});
	lx.push_back({"name", xname});
	if (xname.find('$')==string::npos) {
	  if (debug)
	    WriteLog(msg+"storing with name=<"+xname+"> "+key+"=<"+*i+">");
	  ret.push_back({xname, lx});

	} else {
	  if (debug)
	    WriteLog(msg+"starting recursion with name=<"+xname+"> "+key+"=<"+*i+">");
	  auto ll = ExpandDescriptionInner(xname, lx, debug);
	  if (debug)
	    WriteLog(msg+"recursion returned "+ToStr(ll.size())+" entries");
	  if (ll.empty())
	    return ll;
	  ret.insert(ret.end(), ll.begin(), ll.end());
	}
      }
    }

    if (ret.size()==0) {
      if (namein.find('$')!=string::npos) {
	ShowError(msg, "not expanded and name contains $");
	return ret;
      }
      ret.push_back({namein, lin});
      if (debug)
	WriteLog(msg+"returning unexpanded input");
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedCaptioning(const XmlDom& node) {
    string msg = "DataBase::DescribedCaptioning() : ";
 
    bool debug = DebugCaptionings();

    string use = node.Property("use");
    if (use!="" && !IsAffirmative(use))
      return true;

    string desname = node.Property("name");
    if (desname=="")
      return ShowError(msg+"name should be defined");

    string type = node.Property("type");
    if (type=="")
      return ShowError(msg+"type should be defined");

    list<pair<string,string> > lo, prop = node.Properties();

    for (auto& p : prop) {
      if (p.first=="use")
	continue;

      if (debug) 
	WriteLog(msg+desname+" : <"+p.first+">=<"+p.second+">");

      lo.push_back(p);
    }

    auto ll = ExpandDescription(desname, lo, debug);

    for (auto& l : ll) {
      if (described_captionings.find(l.first)!=described_captionings.end())
	return ShowError(msg+desname+" captioning <"+l.first+
			 "> already defined");

      string ti;
      stringstream ss;
      ss << desname << " <" << l.first << ">";
      for (auto& i : l.second) {
	ss << " " << i.first << "=" << i.second;
	if (i.first=="textindex")
	  ti = i.second;
      }

      if (debug)
	WriteLog(msg+ss.str());

      described_captionings[l.first] = l.second;
      if (ti!="") {
	if (captioning_textindex2name.find(ti)!=
	    captioning_textindex2name.end())
	  return ShowError(msg+"textindex name <"+ti+"> multiply defined: <"
			   +captioning_textindex2name[ti]+"> and <"
			   +l.first+">");
	captioning_textindex2name[ti] = l.first;
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoAllMediaExtractions(bool /*force*/,
				       const vector<size_t>& /*idx*/,
				       const vector<string>& namein,
				       const string& /*classname*/,
				       Segmentation* /*ad*/) {
    // obs! not implemented yet...
    string msg = "DataBase::DoAllMediaExtractions() : ";

    vector<string> name = namein;
    if (name.size()==1 && name[0]=="*") {
      name.clear();
      for (map<string,list<pair<string,string> > >::const_iterator
	     d=described_detections.begin(); d!=described_detections.end();
	   d++)
	name.push_back(d->first);
    }

    // for (size_t i=0; i<name.size(); i++)
    //   if (!DoOneDetectionForAll(force, idx, name[i], classname, ad))
    // 	return ShowError(msg+"<"+name[i]+"> failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoAllSegmentations(bool force, const vector<size_t>& idx,
				    const vector<string>& namein) {
    string msg = "DataBase::DoAllSegmentations() : ";

    vector<string> name = namein;
    if (name.size()==1 && name[0]=="*") {
      name.clear();
      for (auto d=described_segmentations.begin();
	   d!=described_segmentations.end(); d++)
	name.push_back(d->first);
    }

    for (size_t i=0; i<name.size(); i++)
      if (!DoOneSegmentationForAll(force, idx, name[i]))
	return ShowError(msg+"<"+name[i]+"> failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoAllSegmentations(bool force,
				    const list<upload_object_data>& objs,
				    const list<string>& segmin) {
    string msg = "DataBase::DoAllSegmentations() : ";
    
    list<string> segm;
    if (segmin.size()!=1 || segmin.front()!="*")
      segm = segmin;
    else
      for (auto d=described_segmentations.begin();
	   d!=described_segmentations.end(); d++)
	segm.push_back(d->first);

    for (auto dname=segm.begin(); dname!=segm.end(); dname++) {
      auto d = described_segmentations.find(*dname);
      if (d==described_segmentations.end())
	return ShowError(msg+"segmentation <"+*dname+"> not described");
      
      string tts;
      for (list<pair<string,string> >::const_iterator i=d->second.begin();
	   i!=d->second.end(); i++)
	if (i->first=="target")
	  tts = i->second;
      target_type tt = TargetType(tts);

      vector<size_t> idxvec;
      for (auto o=objs.begin(); o!=objs.end(); o++)
	for (size_t i=0; i<o->indices.size(); i++) {
	  size_t idx = o->indices[i];
	  target_type tti = ObjectsTargetType(idx);
	  bool doit = PicSOM::TargetTypeContains(tti, tt);
	  if (doit)
	    idxvec.push_back(idx);
	}
      
      string classname; // obs!
      if (idxvec.size() && !DoOneSegmentationForAll(force, idxvec, *d))
	return false;
    }
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoOneSegmentationForAll(bool force, const vector<size_t>& idx,
					 const string& name) {
    string msg = "DataBase::DoOneSegmentationForAll("+name+") : ";

    auto d = described_segmentations.find(name);

    if (d==described_segmentations.end())
      return ShowError(msg+"not found");

    return DoOneSegmentationForAll(force, idx, *d);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool 
  DataBase::DoOneSegmentationForAll(bool force, const vector<size_t>& idx,
				    const pair<string,
					       list<pair<string,string> > >
				    &descr) {
    string name = descr.first, ttype;

    bool debug = DebugSegmentation();

    string msg = "DataBase::DoOneSegmentationForAll("+name+") : ";

    for (auto i=descr.second.begin(); i!=descr.second.end(); i++)
      if (i->first=="target")
	ttype = i->second;

    target_type tt = SolveTargetTypes(ttype);

    for (size_t i=0; i<idx.size(); i++) {
      bool skip = !ObjectsTargetTypeContains(idx[i], tt) ||
	ObjectsTargetType(idx[i])==target_imagesegment;

      if (debug)
	cout << TimeStamp() << msg << idx[i] << " " << ObjectDump(idx[i])
	     << " skipping=" << (skip?1:0) << endl;

      if (skip)
	continue;

      if (!DoOneSegmentation(force, idx[i], descr))
	return ShowError(msg+"DoOneSegmentation() failed");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoOneSegmentation(bool force, size_t idx,
				   const pair<string,
					      list<pair<string,string> > >
				   &descr) {
    string name = descr.first;
    string msg = "DataBase::DoOneSegmentation("+ToStr(idx)+","+name+") : ";

    if (false) {
      string outseg = "segseg.seg";
      Analysis ana(Picsom(), NULL, NULL, vector<string>());
      AddDescription *ad = NULL;
      segmentfile *image = NULL;
      string sname = "foo", lname = "FooBar";
      if (!ana.OpenAddDescription(ad, image, outseg, sname, lname))
	return ShowError(msg+"OpenAddDescription() failed");
      string rname  = "svmpred";
      string rtype  = "detection";
      string rvalue = "foo=bar";
      SegmentationResult sres(rname, rtype, rvalue);
      ad->addPendingAnnotation(sres, 0, false, false);
      ana.CloseSegmentation(ad, image);
    }

    bool addlabels = false;
    string method, command;
    for (auto i=descr.second.begin(); i!=descr.second.end(); i++) {
      if (i->first=="method")
	method = i->second;
      if (i->first=="command")
	command = i->second;
      if (i->first=="addlabels")
	addlabels = IsAffirmative(i->second);
    }

    string segres = SolveSegmentFilePath(idx, method, false);
    //obs! SqlRetrieveSegmentationFile() ???
    if (!force && segres!="" && FileExists(segres)) {
      if (DebugSegmentation())
	cout << msg << "[" << segres << "] exists, no force, skipping" << endl;
      return true;
    }

    string imgpath = ObjectPathEvenExtract(idx);
    /*
    vector<string> mvec = SplitInSpaces(method);
    segm_command.push_back("*internal*");
    segm_command.push_back("-Ob");
    segm_command.insert(segm_command.end(), mvec.begin(), mvec.end());
    */

    vector<string> segm_command = SplitInSpaces(command);
    segm_command.push_back(imgpath);

    int r = Segmentation::Main(segm_command);

    if (r)
      return ShowError(msg+JoinWithString(segm_command, " ")+" FAILED");

    if (SqlObjects()) {
      /*
      string segres = imgpath;
      size_t p = segres.rfind('/');
      if (p==string::npos)
	p = 0;
      else
	p++;
      segres.insert(p, "segments/"+method+":");
      p = segres.rfind('.');
      if (p!=string::npos)
	segres.replace(p, string::npos, ".seg");
      */
      if (!SqlStoreSegmentationFile(idx, method, segres))
	ShowError(msg+"SqlStoreSegmentationFile() failed");
    }

    if (addlabels) {
      if (segres=="")
	segres = SolveSegmentFilePath(idx, method, true);

      const string& label = Label(idx);
      segmentfile segf(imgpath, segres);
      set<int> flist = segf.listProcessedFrames();
      for (auto i=flist.begin(); i!=flist.end(); i++) {
	int f = *i;
	SegmentationResultList *r = segf.readFrameResultsFromXML(f, "", "");
	for (auto j=r->begin(); j!=r->end(); j++) {
	  if (DebugSegmentation()) {
	    cout << "  NAME:     " << j->name  << endl;
	    cout << "  TYPE:     " << j->type  << endl;
	    cout << "  VALUE:    " << j->value << endl;
	    cout << "  METHODID: " << j->methodid << endl;
	    cout << "  RESULTID: " << j->resultid << endl << endl;
	  }
	  string nlabel = method+":"+label+"_"+j->name;
	  int nidx = LabelIndexGentle(nlabel, false);
	  bool exists = nidx>=0;
	  if (!exists) {
	    target_type tt = target_imagesegment; // obs! hard-coded now
	    nidx = AddOneLabel(nlabel, tt, false, false);
	    if (nidx<0)
	      return ShowError(msg+"AddOneLabel("+nlabel+") failed");

	    FindObject( idx)->children.push_back(nidx);
	    FindObject(nidx)->parents .push_back( idx);
	    // obs! this is now volatile information in core
	    // should it persist to next sessions? YES!
	    if (UseSql()) {
	      map<string,string> m;
	      m["date"]      = PicSOM::OriginsDateStringNow();
	      m["type"]      = TargetTypeString(tt);
	      m["label"]     = nlabel;
	      m["width"]     = "0";
	      m["height"]    = "0";
	      m["frames"]    = "1";
	      m["framerate"] = "0";
	      m["bytes"]     = "0";
	      UpdateOriginsInfoSql(nidx, m, false);
	    }
	  }
	  WriteLog(msg+(exists?"already exists ":"added ")+
		   ObjectDump(nidx));
	}
      }
    }

    return true;
    // return ShowError(msg+"segmentation type <"+type+"> unknown");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoAllDetections(bool force, const vector<size_t>& idx,
				 const vector<string>& namein,
				 const string& classname,
				 const string& instance,
				 const list<string>& feats,
				 const string& augm,
				 bool tolerate_missing,
				 Segmentation *ad, XmlDom& xml,
				 PicSOM::detection_stat_t& dstat) {
    string msg = "DataBase::DoAllDetections() : ";

    vector<string> name = namein;
    if (name.size()==1 && name[0]=="*") {
      name.clear();
      for (map<string,list<pair<string,string> > >::const_iterator
	     d=described_detections.begin(); d!=described_detections.end();
	   d++)
	name.push_back(d->first);
    }

    list<string> cc = SplitClassNames(classname);

    for (auto cci=cc.begin(); cci!=cc.end(); cci++) {
      for (size_t i=0; i<name.size(); i++)
	if (!DoOneDetectionForAll(force, idx, name[i], *cci, instance, feats,
				  augm, tolerate_missing, ad, xml, dstat))
	  return ShowError(msg+"<"+*cci+"> <"+name[i]+"> failed");
    }
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::DetectionName(const string& ddn,
				 const list<string>& f, const string& augm,
				 const string& c, const string& i,
				 bool eout) const {
    const string msg = "DataBase::DetectionName("+ddn+") : ";

    map<string,string> r = DescribedDetection(ddn);
    list<pair<string,string> > ll(r.begin(), r.end());
    
    return DetectionName(ll, f, augm, c, i, eout);
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::DetectionName(const list<pair<string,string> >& l,
				 const list<string>& f, const string& augm,
				 const string& c, const string& i,
				 bool eout) const {
    const string msg = "DataBase::DetectionName() : ";

    map<string,string> m(l.begin(), l.end());

    string t = m["type"];

    if (t=="svmpred" && f.size()==1) {
      // obs! should be coordinated with SvmPredDetectionSingleSelf()

      // this logic was added 2015-06-29.
      // earlier extraout was used even when empty and extra were set
      // now eout hints to use extraout rather then extra, if defined...
      string extra, svmopts;
      if (eout && m.find("extraout")!=m.end())
      	extra = m["extraout"];
      else
      	extra = m["extra"];

      // if (extra!="" && augm!="") {
      // 	ShowError(msg+"extra=<"+extra+"> augm=<"+augm+">");
      // 	return "";
      // }
      // if (extra=="")
      // 	extra = augm;
      extra += augm;

      if (m["svm_homker"]=="intersection") {
	svmopts = "hkm-int";
	svmopts += m["svm_homkerorder"];

      } else if (m["svm_kernel_type"]!="")
	svmopts = m["svm_kernel_type"];

      string s = m["svm_library"]+"::"+f.front()+"::"+svmopts+"::"+extra+"#"+c;

      if (i!="")
	s += "§"+i;

      return s;
    }

    if (t=="lsc" && f.size()==1)
      return "lsc::"+f.front()+"::"+m["k"]+"-"+m["n_max"]+m["posneg"]+"::"
	+m["extra"]+"#"+c;

    if (t=="caffe" && f.size()==0) {
      string d = "caffe::"+m["name"];
      if (m["extra"]!="")
	d += "::"+m["extra"];
      return d;
    }

    // obs! added 2014-06-23
    // obs! this convention should be coordinated with XXX()

    if ((t=="fusion" || t=="children" || t=="timefusion" ||
	 t=="timethreshold" || t=="sentenceselection") &&
	f.size()==0 && i=="") {
      if (c!="")
	return m["name"]+"#"+c;
      else
	return m["name"]; // this happens for caffe output layers
    }

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoAllDetections(bool force,
				 const list<upload_object_data>& objs,
				 const list<string>& detectin,
				 const list<string>& feats,
				 bool tolerate_missing,
				 Segmentation *ad) {
    string msg = "DataBase::DoAllDetections() : ";
    
    list<string> detect;
    if (detectin.size()!=1 || detectin.front()!="*")
      detect = detectin;
    else
      for (auto d=described_detections.begin(); d!=described_detections.end();
	   d++)
	detect.push_back(d->first);

    for (auto dname=detect.begin(); dname!=detect.end(); dname++) {
      auto d = described_detections.find(*dname);
      if (d==described_detections.end())
	return ShowError(msg+"detection <"+*dname+"> not described");
      
      string tts;
      for (list<pair<string,string> >::const_iterator i=d->second.begin();
	   i!=d->second.end(); i++)
	if (i->first=="target")
	  tts = i->second;
      target_type tt = TargetType(tts);

      vector<size_t> idxvec;
      for (auto o=objs.begin(); o!=objs.end(); o++)
	for (size_t i=0; i<o->indices.size(); i++) {
	  size_t idx = o->indices[i];
	  target_type tti = ObjectsTargetType(idx);
	  bool doit = PicSOM::TargetTypeContains(tti, tt);
	  if (doit)
	    idxvec.push_back(idx);
	}
      
      string classname, instance; // obs!
      XmlDom xml; // obs!
      PicSOM::detection_stat_t dstat;
      string augm;
      if (idxvec.size() && !DoOneDetectionForAll(force, idxvec, *d,
						 classname, instance,
						 feats, augm, tolerate_missing,
						 ad, xml, dstat))
	return false;
    }
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoOneDetectionForAll(bool force, const vector<size_t>& idx,
				      const string& name,
				      const string& classname,
				      const string& instancein,
				      const list<string>& feats,
				      const string& augm,
				      bool tolerate_missing,
				      Segmentation *ad, XmlDom& xml,
				      PicSOM::detection_stat_t& dstat) {
    string msg = "DataBase::DoOneDetectionForAll("+name+") : ";

    map<string,list<pair<string,string> > >::const_iterator d
      = described_detections.find(name);

    if (d==described_detections.end())
      return ShowError(msg+"not found");

    string instance;
    for (auto i=d->second.begin(); i!=d->second.end(); i++)
      if (i->first=="instance")
	instance = i->second;

    if (instancein!="" && instance!="" && instancein!=instance)
      return ShowError(msg+"instancein=<"+instancein+"> instance=<"+
		       instance+">");

    if (instance=="")
      instance = instancein; // don't know when this would be needed...

    return DoOneDetectionForAll(force, idx, *d, classname, instance,
				feats, augm, tolerate_missing, ad, xml, dstat);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool 
  DataBase::DoOneDetectionForAll(bool force, const vector<size_t>& idx,
				 const pair<string,list<pair<string,string> > >
				 &descr, const string& classname,
				 const string& instance,
				 const list<string>& feats,
				 const string& augm,
				 bool tolerate_missing,
				 Segmentation *ad, XmlDom& xml,
				 PicSOM::detection_stat_t& dstat) {
    string name = descr.first, ttype;

    bool debug1 = DebugDetections()>1, debug2 = DebugDetections()>2;

    string msg = "DataBase::DoOneDetectionForAll("+name+") : ";

    PicSOM::detection_stat_t dorig = dstat;

    for (auto i=descr.second.begin(); i!=descr.second.end(); i++)
      if (i->first=="target")
	ttype = i->second;

    target_type tt = SolveTargetTypes(ttype);
    if (tt==target_no_target)
      return ShowError(msg+"detection target not specified or insane");

    // 2015-06-29 this uses "extra" if "extraout" is not defined even with last
    //            argument is true
    string dname = DetectionName(descr.second, feats, augm, classname,
				 instance, true);
    // obs! should we warn if this is empty??? yes!!!
    //if (dname=="")
    //  return ShowError(msg+"dname=\"\"");

    dstat.njobs++;
    bool exists = true;
    for (size_t i=0; i<idx.size(); i++) {
      dstat.ntot++;

      target_type  ott = ObjectsTargetType(idx[i]);
      target_type mott = PicSOM::TargetTypeFileFullMasked(ott);
      bool skip = mott!=tt;

      if (debug2)
	cout << TimeStamp() << msg << idx[i] << " " << ObjectDump(idx[i])
	     << " target type skipping=" << (skip?1:0) << endl;

      if (skip) {
	dstat.nskip++;
	continue;
      }

      // until 2014-10-06 this could only be "turned off" with exist=false
      // now checks for existing data are made for every item
      // which one is more efficient, god only knows...
      exists = true; // obs!
      if (!overwritedetections && dname!="" && exists) {
	map<string,vector<float> > r = RetrieveDetectionData(idx[i], dname,
							     false, exists);
	if (r.size()==1) {
	  if (debug1) {
	    string vals;
	    if (r.begin()->second.size()<5)
	      vals = ToStr(r.begin()->second);
	    else
	      vals = ToStr(vector<float>(&r.begin()->second[0],
					 &r.begin()->second[5]))+" ...";
	    WriteLog(msg+"detection "+dname+" for #"+ToStr(idx[i])
		     +" already found as "+vals);
	  }
	  dstat.nfound++;
	  continue;
	}
      }

      bool hasdata = true;
      if (!DoOneDetection(force, idx[i], descr, classname, instance, feats, 
			  augm, ad, tolerate_missing, hasdata, xml))
	return ShowError(msg+"DoOneDetection() failed");

      dstat.ndone   +=  hasdata;
      dstat.nnodata += !hasdata;
    }

    if (DebugDetections())
      WriteLog("Done "+ToStr(dstat.ndone-dorig.ndone)+" detections \""
	       +descr.first+"\" -> \""+dname+"\", "
	       +ToStr(dstat.nskip-dorig.nskip)+" skipped by type, "
	       +ToStr(dstat.nfound-dorig.nfound)+" existed, "
	       +ToStr(dstat.nnodata-dorig.nnodata)+" missing data, "
	       +ToStr(dstat.ntot-dorig.ntot)+" total processed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoOneDetection(bool /*force*/, size_t idx,
				const pair<string,list<pair<string,string> > >
				&descrin, const string& classname,
				const string& instance,
				const list<string>& feats, const string& augm,
				Segmentation *ad, bool tolerate_missing,
				bool& hasdata, XmlDom& xml) {
    string name = descrin.first, type;
    string msg = "DataBase::DoOneDetection("+ToStr(idx)+","+name+") : ";

    pair<string,list<pair<string,string> > > descr = descrin;
    bool has_class = false;
    for (list<pair<string,string> >::const_iterator i=descr.second.begin();
	 i!=descr.second.end(); i++)
      if (i->first=="type")
	type = i->second;
      else if (i->first=="class")
	has_class = true;

    if (!has_class && type=="caffe")
      descr.second.push_back(make_pair("class", "*caffe*"));

    if (type=="svmpred" || type=="fusion" || type=="lsc" ||
	type=="children" || type=="combine" || type=="random" ||
	type=="caffe" || type=="timefusion" || type=="timethreshold" ||
	type=="sentenceselection" || type=="superclass") {

      if (instance!="")
	descr.second.push_back(make_pair("multi", "single,"+instance));

      bool fsep = type=="svmpred" || type=="lsc";
      return CommonDetection(type, idx, fsep, descr.second, classname,
			     feats, augm, ad, tolerate_missing, hasdata, xml);
    }

    // if (type=="caffe")
    //   return DoOneCaffeDetection(idx, descr.second);

    if (type=="od")
      return DoOneObjectDetection(idx, descr.second);

    if (type=="osrs")
      return OSRSspeechRecognition(idx, descr.second);

    if (type=="tesseract")
      return TesseractOCR(idx, descr.second);

    return ShowError(msg+"detection type <"+type+"> unknown");
  }

  /////////////////////////////////////////////////////////////////////////////

  // bool DataBase::DoOneCaffeDetection(size_t idx,
  // 				     const list<pair<string,string> >& d) {
  //   string msg = "DataBase::DoOneCaffeDetection("+ToStr(idx)+") : ";
    
  //   map<string,string> kvmap(d.begin(), d.end());
  //   string name = kvmap["name"];
  //   if (name=="")
  //     return ShowError(msg+"name not defined");

  //   vector<size_t> idxv { idx };
  //   list<vector<float> > det = RunCaffe(name, idxv);
  //   if (det.size()!=1)
  //     return ShowError(msg+"det.size()!=1");

  //   if (!StoreDetectionResult(idx, name, det.front()))
  //     return ShowError(msg+"StoreDetectionResult() failed");

  //   return true;
  // }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoOneObjectDetection(size_t idx,
				      const list<pair<string,string> >& d) {
    string msg = "DataBase::DoOneObjectDetection("+ToStr(idx)+") : ";

#ifdef PICSOM_USE_OD
    map<string,string> m(d.begin(), d.end());
    string name = m["name"];
    auto p = odset.find(name);

    if (p==odset.end()) {
      string toc = m["toc"], tocfile = toc;
      if (toc=="")
	return ShowError(msg+"toc not specified");

      if (tocfile.find("./")!=0 && tocfile.find("../")!=0 &&
	  tocfile.find("/")!=0)
	tocfile = ExpandPath("ods", tocfile);

      ObjectDetection &od = odset[name];
      od.Initialize();

      set<string> ignore { "name", "type", "target", "toc" };

      for (auto i=m.begin(); i!=m.end(); i++)
	if (ignore.find(i->first)==ignore.end() &&
	    !od.Interpret(i->first, i->second))  // SetOption() ??
	  return ShowError(msg+"setting od option \""+i->first+"="+
			   i->second+"\" failed");
      
      od.SetDebug(1);

      od.SetUsedDetector("sift");
      od.SetUsedDescriptor("sift");
      
      od.SetShowKeypoints(false);
      od.SetShowResults(false);
      if (!od.LoadDbs(tocfile, "")) 
	ShowError(msg+"Loading database \""+toc+"\" failed");

      p = odset.find(name);
    }

    ObjectDetection &od = p->second;
    string fname = ObjectPathEvenExtract(idx);
    savetype savemode = NO_SAVING;
    dbitem best;
    od.ProcessImage(fname, savemode, best);
    WriteLog("Matched keypoint pairs: "+ToStr(best.nsurvivors)+" with "+
     	     best.objectname);

    return true;

#else
    bool dummy = d.size(); dummy = !dummy;
    return ShowError(msg+"PICSOM_USE_OD not defined");
#endif // PICSOM_USE_OD
  }

  /////////////////////////////////////////////////////////////////////////////

  list<XmlDom> DataBase::FindDetectedObjectInfo(const string& oixml,
						const string& image) {
    string msg = "DataBase::FindDtetectedObjectInfo("+oixml+","+image+") : ";

    if (detected_object_info.find(oixml)==detected_object_info.end()) {
      string dname = ExpandPath("ods", oixml);
      cout << msg << "[" << dname << "]" << endl;
      XmlDom xml = XmlDom::ReadDoc(dname);
      detected_object_info[oixml] = xml;
    }

    list<XmlDom> res;

    XmlDom xml = detected_object_info[oixml];
    XmlDom ill = xml.Root();
    for (XmlDom e = ill.FirstElementChild("infolink"); e;
	 e = e.NextElement("infolink")) {
      if (e.Property("image")==image)
	res.push_back(e);
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::VideoFrameRate(size_t idx) {
    bool angry = false;

    string msg = "DataBase::VideoFrameRate("+ToStr(idx)+") : ";

    size_t pidx = RootParent(idx);

    map<string,string> o = ReadOriginsInfo(pidx, false, true);
    string framerate = o["framerate"];
    if (framerate=="")
      return ShowError(msg+"framerate not known");

    float fps = atof(framerate.c_str());
    if (!fps && angry)
      ShowError(msg+"framerate <"+framerate+"> is zero");

    return fps;

    /*
    string dim = o["dimensions"];
    size_t p = dim.find('@');
    if (p==string::npos) {
      ShowError(msg+"no @ found in \""+dim+"\" of <"+Label(pidx)+">");
      return 0;
    }

    return atof(dim.substr(p+1).c_str());
    */
  }

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::VideoDuration(size_t idx) {
    string msg = "DataBase::VideoDuration("+ToStr(idx)+") : ";

    if (!ObjectsTargetTypeContains(idx, target_video))
      return ShowError(msg+"not a video object");

    if (ObjectsTargetTypeContains(idx, target_videosegment)) {
      auto psd = ParentStartDuration(idx, target_video);
      return psd.second.second;
    }

    map<string,string> o = ReadOriginsInfo(idx, false, true);
    string frames = o["frames"], framerate = o["framerate"];
    if (frames=="")
      return ShowError(msg+"number of frames not known");
    if (framerate=="")
      return ShowError(msg+"framerate not known");

    float fps = atof(framerate.c_str());
    if (!fps)
      return ShowError(msg+"framerate <"+framerate+"> is zero");

    int nf = atoi(frames.c_str());

    return nf/fps;

    /*
    string dim = o["dimensions"];
    size_t p = dim.find('@');
    if (p==string::npos)
      return ShowError(msg+"no @ found in \""+dim+"\" of <"+Label(idx)+">");

    float fps = atof(dim.substr(p+1).c_str());

    size_t q = dim.rfind('x', p);
    if (q==string::npos)
      return ShowError(msg+"no x found in \""+dim+"\" of <"+Label(idx)+">");

    int nf = atoi(dim.substr(q+1).c_str());

    return nf/fps;
    */
  }

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::ParentRelativeStartTime(size_t idx) {
    string msg = "DataBase::ParentRelativeStartTime("+ToStr(idx)+") : ";

    map<string,string> o = ReadOriginsInfo(idx, false, true);

    string url = o["url"];
    size_t p = url.find('[');
    if (p==string::npos) {
      ShowError(msg+"no [] found in \""+url+"\"");
      return 0;
    }
    float fra = atof(url.substr(p+1).c_str());

    // string dim = o["dimensions"];
    // p = dim.find('@');
    // if (p==string::npos) {
    //   ShowError(msg+"no @ found in \""+dim+"\"");
    //   return 0;
    // }
    // float fps = atof(dim.substr(p+1).c_str());

    float fps = VideoFrameRate(idx);
 
    if (!fps) {
      ShowError(msg+"fps==0");
      return 0;
    }

    return fra/fps;
  }

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::SolveVideoLength(size_t idx) const {
    map<string,string> o = ReadOriginsInfo(idx, false, true);
    string dim = o["dimensions"];
    cout << dim << endl;
    size_t p = dim.find('x');
    if (p!=string::npos) {
      p = dim.find('x', p+1);
      if (p!=string::npos) {
	size_t n = atoi(dim.substr(p+1).c_str());
	p = dim.find('@', p+1);
	if (p!=string::npos) {
	  float fps = atof(dim.substr(p+1).c_str());
	  return n/fps;
	}
      }
    }

    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<double,string> > 
  DataBase::OCRlinesOrCombine(size_t idx, const string& name, bool combine) {
    bool debug = true;

    list<pair<double,string> > ret;

    string lab = Label(idx);
    string vfile = SolveObjectPath(lab), asrfile;

    // cout << "vfile=<" << vfile << ">" << endl;

    string fname = vfile;
    size_t p = fname.rfind('.'), s = fname.rfind('/');
    if (p!=string::npos && (s==string::npos||p>s)) {
      if (s!=string::npos && s>=2 && fname.substr(s-2, 2)==".d")
	fname.erase(s+1);
      else {
	fname.erase(p);
	fname += ".d/";
      }
      fname += name+":"+lab+".txt";
    }

    bool found = fname!=vfile && FileExists(fname);
    if (found) {
      string txt = FileToString(fname);
      for (;;) {
	p = txt.find('\n');
	if (p==string::npos)
	  break;
	txt[p] = ' ';
      }

      while (txt.size() && txt[txt.size()-1]==' ')
	txt.erase(txt.size()-1);

      if (txt!="") {
	double time = 0.0;
	p = lab.find(":s");
	if (p!=string::npos)
	  time = atof(lab.substr(p+2).c_str());

	if (debug)
	  cout << fname << " : " << time << " " << txt << endl;
	
	ret.push_back(make_pair(time, txt));
      }
    }

    if (found || !combine)
      return ret;

    object_info *oi = FindObject(idx);
    for (size_t i=0; i<oi->children.size(); i++)
      if (ObjectsTargetTypeContains(oi->children[i], target_image)) {
	list<pair<double,string> > al
	  = OCRlinesOrCombine(oi->children[i], name, combine);
	ret.insert(ret.end(), al.begin(), al.end());
      }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::ASRlinesOrCombine(size_t idx, bool combine) {
    bool debug = false;

    string lab = Label(idx);
    target_type tt = target_video; // obs! hard-coded
    string vfile = ObjectTypeAndPath(lab, tt), asrfile;
    list<textline_t> asrraw = OSRSraw(vfile, asrfile);
    if (asrraw.size()>0 || !combine) {
      if (debug) {
	cout << idx << " " << lab << " REAL ASR lines:" << endl;
	size_t j = 0;
	for (auto i=asrraw.begin(); i!=asrraw.end(); i++, j++)
	  cout << j << " " << i->txt_display() << endl;
      }

      return asrraw;
    }

    object_info *oi = FindObject(idx);
    for (size_t i=0; i<oi->children.size(); i++)
      if (ObjectsTargetTypeContains(oi->children[i], tt)) {
	list<textline_t> al = ASRlinesOrCombine(oi->children[i], combine);
	float dt = ParentRelativeStartTime(oi->children[i]);
	for (auto j=al.begin(); j!=al.end(); j++) {
	  j->start += dt;
	  j->end   += dt;
	}
	asrraw.insert(asrraw.end(), al.begin(), al.end());
      }

    if (debug) {
      cout << idx << " " << lab << " COMBINED ASR lines:" << endl;
      size_t j = 0;
      for (auto i=asrraw.begin(); i!=asrraw.end(); i++, j++)
	cout << j << " " << i->txt_display() << endl;
    }

    return asrraw;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::OSRSfileName(const string& f) {
    string asrfile = f, suffix = ".osrs";
    size_t p = asrfile.rfind('.');
    if (p!=string::npos) {
      asrfile.erase(p);
      p = asrfile.rfind('/');
      if (p!=string::npos) {
	bool is_d = p>2 && asrfile.substr(p-2, 2)==".d";
	if (is_d) {
	  asrfile += suffix;
	} else {
	  string s = asrfile.substr(p)+suffix;
	  asrfile += ".d"+s;
	}
	return asrfile;
      }
    }
    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::OSRSraw(const string& f, string& asrfile) {
    list<textline_t> ret;

    asrfile = f[0]=='/' ? f : OSRSfileName(f);
    bool asrok = FileExists(asrfile);
    if (asrok)
      ret = ReadOSRS(asrfile);
    else
      asrfile = "";

    return ret;    
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::OSRScooked(const string& f, string& asrfile) {
    list<textline_t> raw = OSRSraw(f, asrfile);

    if (asrfile!="")
      return SplitASR(raw);

    return list<textline_t>();
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ASStimeStr(double sec) {
    int min, hour;
    SecToMinHour(sec, min, hour);
    string secstr = ToStr(sec);
    if (secstr.find('.')==string::npos)
      secstr += ".";
    secstr += "00";
    size_t p = secstr.find('.');
    secstr.erase(p+3);
    stringstream ss;
    ss << hour << ":" << min << ":" << secstr;

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VTTtimeStr(double sec) {
    int min, hour;
    SecToMinHour(sec, min, hour);
    string secstr = ToStr(sec), minstr = ToStr(min), hourstr = ToStr(hour);
    if (secstr.find('.')==string::npos)
      secstr += ".";
    secstr += "000";
    size_t p = secstr.find('.');
    secstr.erase(p+4);
    if (p==1)
      secstr = "0"+secstr;
    if (minstr.size()==1)
      minstr = "0"+minstr;
    if (hourstr.size()==1)
      hourstr = "0"+hourstr;

    stringstream ss;
    if (hour)
      ss << hourstr << ":" << minstr << ":" << secstr;
    else
      ss << minstr << ":" << secstr;

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::ReadOSRS(const string& f) {
    list<textline_t> r;

    string s = FileToString(f);
    size_t a = 0, b;
    while (1) {
      b = s.find('\n', a);
      if (b==string::npos)
	b = s.size();

      string l = s.substr(a, b-a), start, end, text;
      // cout << "|" << l << "|" << endl;
      istringstream iss(l);
      textline_t w;
      iss >> start >> end >> text;
      w.start = TimeStrToSec(start);
      w.end   = TimeStrToSec(end);

      while (true) {
	string t;
	iss >> t;
	if (!iss)
	  break;
	text += " "+t;
      }
      w.add(text, 0);

      if (PicSOM::DebugSpeechRecognizer())
	cout << "  [" << w.start << "] [" << w.end << "] ["
	     << w.txt_val[0].first << "]" << endl;

      r.push_back(w);

      a = b+1;
      if (a>=s.size())
	break;
    }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t> DataBase::SplitASR(const list<textline_t>& l) {
    list<textline_t> r;

    auto a = l.begin();
    for (auto i=l.begin();; i++)
      if (i->empty() || i->txt_val[0].first=="." || i==l.end()) {
	list<textline_t> p = ProcessASRrange(a, i);
	r.insert(r.end(), p.begin(), p.end());
	a = i;
	if (i==l.end())
	  break;
      }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<textline_t>
  DataBase::ProcessASRrange(list<textline_t>::const_iterator a,
			    list<textline_t>::const_iterator b) {
    list<textline_t> r;

    size_t llen = 30;
    float tlim = 1.5;
    float tdur = 8;

    vector<textline_t> ll; 
    for (list<textline_t>::const_iterator i=a; i!=b; i++)
      if (i->empty() || i->txt_val[0].first!=".")
	ll.push_back(*i);

    if (!ll.size())
      return r;

    vector<pair<size_t,size_t> > s;

    size_t n = 0, c = 0;
    for (size_t i=0; i<ll.size(); i++) {
      size_t ns = n+ll[i].txt_val[0].first.size();
      bool split = ns>llen;
      if (i>0 && ll[i].start-ll[i-1].end>tlim)
	split = true;

      if (i && split) {
	s.push_back(make_pair(c, i-1));
	c = i;
	ns = 0;
      }
      n = ns;
    }
    s.push_back(make_pair(c, ll.size()-1));
    
    for (size_t i=0; i<s.size();) {
      textline_t al = ll[s[i].first];
      al.add("", 0);
      string txt1, txt2;
      bool two = true;
      for (size_t k=0; k<2; k++)
	if (i+k<s.size()) {
	  if (k==1)
	    if (ll[s[i+k].first].start-ll[s[i].second].end>tlim ||
		ll[s[i+k].second].end-al.start>tdur) {
	      two = false;
	      break;
	    }
	  al.end = ll[s[i+k].second].end;
	  string& txt = k?txt2:txt1;
	  for (size_t j=s[i+k].first; j<=s[i+k].second; j++)
	    txt += (txt==""?"":" ")+ll[j].txt_val[0].first;
	}

      al.set(txt1, 0);
      if (al.get_text()!="" && txt2!="")
	al.txt_val[0].first += "\\n"+txt2;

      if (i==0) {
	char t = al.txt_val[0].first[0];  // scandinavians? utf-8?
	if (t>='a' && t<='z')
	  al.txt_val[0].first[0] -= 32;
      }
      if (i+2>=s.size())
	al.txt_val[0].first += ".";

      cout << al.start << " " << al.end << " " << al.txt_val[0].first << endl;

      r.push_back(al);
      i += 1+two;
    }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::OSRSspeechRecognition(size_t idx,
				       const list<pair<string,string> >&) {
    string msg = "DataBase::OSRSspeechRecognition("+ToStr(idx)+") : ";

    bool skip = false;

    if (skip)
      return true;

    Tic("OSRSspeechRecognition");

    cout << TimeStamp() << msg << "starting" << endl;
    
    string file = SolveObjectPath(Label(idx));
    string osrs = OSRSfileName(file);
    cout << "  <" << file << "> -> <" << osrs << ">" << endl; 

    string tmpdir = TempDir();

    string dir = Picsom()->RootDir(Picsom()->SolveArchitecture());
    string sname = "recognize_file.pl";
    string script = dir+"/asr-for-picsom/"+sname;
    if (!FileExists(script))
      script = "./"+sname;

    vector<string> cmd;
    cmd.push_back(script);
    cmd.push_back("-t");
    cmd.push_back(tmpdir);
    cmd.push_back(file);
    cmd.push_back(osrs);
    
    if (PicSOM::DebugSpeechRecognizer()==0) {
      cmd.push_back("1>/dev/null");
      cmd.push_back("2>&1");
    }

    int s = Picsom()->ExecuteSystem(cmd, true, true, true);

    // RmDirRecursive(tmpdir);

    Tac("OSRSspeechRecognition");

    if (s==-1)
      return ShowError(msg+"running OSRS failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::TesseractOCR(size_t idxin,
			      const list<pair<string,string> >& spec) {
    string msg = "DataBase::TesseractOCR("+ToStr(idxin)+") : ";

    bool debug = true, keeptmp = true;

    if (debug)
      cout << TimeStamp() << msg << "starting" << endl;

    string name, switches, preprocess, postprocess;
    for (auto i=spec.begin(); i!=spec.end(); i++)
      if (i->first=="name")
	name = i->second;
      else if (i->first=="switches")
	switches = i->second;
      else if (i->first=="preprocess")
	preprocess = i->second;
      else if (i->first=="postprocess")
	postprocess = i->second;
 
    //int   pidx = FindObject(idxin)->default_parent();
    int pidx = GetParentVideo(Label(idxin), target_video);
    size_t idx = pidx>=0 ? pidx : idxin;

    string subd = Label(idx)+".d";
    string img  = SolveObjectPath(Label(idxin));
    string out  = SolveObjectPath(Label(idxin), subd, name, true);

    if (out=="")
      return ShowError(msg+"failed to solve output file name");

    if (!Picsom()->MakeDirectory(out, true))
      return ShowError(msg+"failed to create directory for output file <"
		       +ShortFileName(out)+">");

    string imgin = img, tmpfile;
    if (preprocess!="") {
      string imgrest = img;
      size_t p = imgrest.rfind('/');
      if (p!=string::npos)
	imgrest.erase(0, p+1);
      imgin = tmpfile = TempFile(name+"-preproc-"+imgrest);
      imagedata idata = imagefile(img).frame(0);
      idata.force_one_channel();
      idata.convert(imagedata::pixeldata_uchar);
      unsigned char white = 255, black = 0;

      vector<string> pp = SplitInSpaces(preprocess);
      for (auto i=pp.begin(); i!=pp.end(); i++) {
	string a = *i;
	if (a.find("crop(")==0 && a[a.size()-1]==')') {
	  a.erase(0, 5);
	  a.erase(a.size()-1);
	  unsigned int tlx, tly, brx, bry;
	  if (sscanf(a.c_str(), "%u,%u,%u,%u", &tlx, &tly, &brx, &bry)==4) {
	    for (size_t x=0; x<idata.width(); x++)
	      for (size_t y=0; y<idata.height(); y++)
		if (x<tlx || x>=brx || y<tly || y>=bry)
		    idata.set(x, y, white);
	  }
	}
	if (a=="inverse") {
	  for (size_t x=0; x<idata.width(); x++)
	    for (size_t y=0; y<idata.height(); y++)
	      idata.set(x, y, (unsigned char)(255-idata.get_one_uchar(x, y)));
	}
	if (a.find("threshold")==0) {
	  unsigned char limit = 128;
	  if (a.size()>9)
	    limit = atoi(a.substr(9).c_str());
	  for (size_t x=0; x<idata.width(); x++)
	    for (size_t y=0; y<idata.height(); y++)
	      if (idata.get_one_uchar(x, y)<limit)
		idata.set(x, y, black);
	      else
		idata.set(x, y, white);
	}
      }

      imagefile::write(idata, tmpfile);
    }

    string tessbin = Picsom()->FindExecutable("", "tesseract", "");

    vector<string> cmd { tessbin, imgin, out };

    vector<string> swt = SplitInSpaces(switches);
    if (swt.size() && swt[0]!="")
      cmd.insert(cmd.end(), swt.begin(), swt.end());
 
    if (!debug) {
      cmd.push_back("1>/dev/null");
      cmd.push_back("2>&1");
      cmd.push_back("&");
    }

    Picsom()->ExecuteSystem(cmd, true, true, true);

    if (!keeptmp && tmpfile!="")
      Unlink(tmpfile);

    if (postprocess!="") {
      vector<string> pp = SplitInSpaces(postprocess);
      for (auto i=pp.begin(); i!=pp.end(); i++) {
	string a = *i;

	if (a=="finnish-names") {
	  string txtfile = out+".txt";
	  string txtin = FileToString(txtfile);
	  for (;;) {
	    size_t p = txtin.find('\n');
	    if (p==string::npos)
	      break;
	    txtin[p] = ' ';
	  }
	  for (;;) {
	    size_t p = txtin.find("  ");
	    if (p==string::npos)
	      break;
	    if (txtin.size() && txtin[txtin.size()-1]==' ')
	      txtin.erase(txtin.size()-1);
	    txtin.erase(p, 1);
	  }
	  string txtout = ExtractFinnishNames(txtin);

	  if (debug)
	    cout << "tesseract finnish names: [" << txtin << "]->["
		 << txtout << "]" << endl;

	  if (txtout.size())
	    txtout += "\n";
	  StringToFile(txtout, txtfile);
	}
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ExtractFinnishNames(const string& txtin) {
    if (finnish_firstnames.empty()) {
      string sdir = "/names/finnish", dir = Picsom()->RootDir("share")+sdir;
      if (!DirectoryExists(dir))
	dir = Picsom()->RootDir("share", true)+sdir;

      string name;
      int count;

      ifstream fn_vrk(dir+"/firstnames-vrk.txt");
      for (;;) {
	fn_vrk >> name >> count;
	if (!fn_vrk)
	  break;
	name = LowerCase(name);
	finnish_firstnames[name] = count;
      }
      ifstream fn(dir+"/firstnames.txt");
      for (;;) {
	fn >> name;
	if (!fn)
	  break;
	if (name.find('.')!=string::npos)
	  continue;
	size_t p = name.find('(');
	if (p!=string::npos)
	  name.erase(p);
	name = LowerCase(name);
	if (finnish_firstnames.find(name)!=finnish_firstnames.end())
	  continue;
	finnish_firstnames[name] = 1;
      }

      ifstream ln_vrk(dir+"/lastnames-vrk.txt");
      for (;;) {
	ln_vrk >> name >> count;
	if (!ln_vrk)
	  break;
	name = LowerCase(name);
	finnish_lastnames[name] = count;
      }
      ifstream ln(dir+"/lastnames.txt");
      for (;;) {
	ln >> name;
	if (!ln)
	  break;
	if (name.find('.')!=string::npos)
	  continue;
	size_t p = name.find('(');
	if (p!=string::npos)
	  name.erase(p);
	name = LowerCase(name);
	if (finnish_lastnames.find(name)!=finnish_lastnames.end())
	  continue;
	finnish_lastnames[name] = 1;
      }
    }

    vector<string> txtinv = SplitInSpaces(txtin), txtoutv;
    for (auto n=txtinv.begin(); n!=txtinv.end(); n++) {
      string lcn = LowerCase(*n);
      bool found = false;
      if (finnish_firstnames.find(lcn)!=finnish_firstnames.end()) {
	float c = finnish_firstnames[lcn];
	if (c>1 || lcn.size()>=4)
	  found = true;
      }
      if (finnish_lastnames.find(lcn)!=finnish_lastnames.end()) {
	float c = finnish_lastnames[lcn];
	if (c>1 || lcn.size()>=4)
	  found = true;
      }
      if (found)
	txtoutv.push_back(*n);
    }
    string txtout;
    if (txtoutv.size()>1)
      txtout = JoinWithString(txtoutv, " ");

    return txtout;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CommonDetection(const string& type, size_t idx,
				 bool splitfeat,
				 const list<pair<string,string> >& min,
				 const string& classname,
				 const list<string>& feats,
				 const string& augm, Segmentation *adseg,
				 bool tolerate_missing, bool& hasdata,
				 XmlDom& xml) {
    string msg = "DataBase::CommonDetection("+type+","+ToStr(idx)+") : ";
    bool debug = DebugDetections()>2;

    if (debug)
      cout << TimeStamp() << msg << "starting" << endl;

    string cls_spec, feat_spec, multi_spec, extraout;
    size_t dimension = 1;
    list<pair<string,string> > m;
    for (list<pair<string,string> >::const_iterator i=min.begin();
	 i!=min.end(); i++) {
      if (debug)
	cout << "  " << i->first << "=" << i->second << endl;
      if (i->first=="class")
	cls_spec = i->second;
      else if (i->first=="features")
	feat_spec = i->second;
      else if (i->first=="multi")
	multi_spec = i->second;
      else if (i->first=="dimension")
	dimension = atoi(i->second.c_str());
      else if (i->first=="extraout")
	extraout = i->second;
      else
	m.push_back(*i);
    }

    string fusion;
    vector<string> multi_list, multi_part;
    if (multi_spec!="")
      multi_part = SplitInCommas(multi_spec);
    if (multi_part.size()) {
      // this is a trick to get detections store in a file from single "instance=n1"
      if (multi_part.size()==2 && multi_part[0]=="single") {
	multi_list.push_back(multi_part[1]);
	fusion = "none";

      } else {
	if (multi_part.size()!=3)
	  return ShowError(msg+"multi="+multi_spec+" could not be interpreted");
	fusion = multi_part[0];
	if (!IsKnownFusion(fusion))
	  return ShowError(msg+"multifusion="+fusion+" not implemented");

	if (multi_part[1]=="n") {
	  // this would typically used with hard_negative_mining
	  multi_list.push_back("");
	  int b = atoi(multi_part[2].c_str());
	  for (int i=1; i<=b; i++)
	    multi_list.push_back("n"+ToStr(i));

	} else {

	  // this is the original multi-learn setup of "identical" §1 .. §10
	  int a = atoi(multi_part[1].c_str());
	  int b = atoi(multi_part[2].c_str());
	  for (int i=a; i<=b; i++)
	    multi_list.push_back(ToStr(i));
	}
      }
    }
    
    vector<string> feat_spec_split = SplitInCommas(feat_spec), feat_spec_e;
    for (auto i=feat_spec_split.begin(); i!=feat_spec_split.end(); i++) {
      list<string> al = ExpandFeatureAlias(*i);
      feat_spec_e.insert(feat_spec_e.end(), al.begin(), al.end());
    }
    string feat_spec_exp = CommaJoin(feat_spec_e);

    vector<string> feat_list1;
    if (splitfeat)
      feat_list1 = SplitInCommas(feat_spec_exp);
    else
      feat_list1.push_back(feat_spec_exp);

    if (debug)
      cout << TimeStamp() << msg << "feat_spec_exp=" << feat_spec_exp
	   << " feat_list.size()=" << feat_list1.size()
	   << " [ " << ToStr(feat_list1) << " ]" << endl;

    vector<string> feat_list;
    for (auto q=feat_list1.begin(); q!=feat_list1.end(); q++) {
      const list<string>& fl = FeatureRegExpList(*q);
      feat_list.insert(feat_list.end(), fl.begin(), fl.end());
    }

    if (debug)
      cout << TimeStamp() << msg << "after regexps feat_list.size()=" << feat_list.size()
	   << " [ " << ToStr(feat_list) << " ]" << endl;

    if (feats.size() && splitfeat) { //obs! what about splitfeat==false?
      set<string> fset(feats.begin(), feats.end());
      for (auto i=feat_list.begin(); i!=feat_list.end(); ) {
	if (fset.find(*i)==fset.end())
	  i = feat_list.erase(i);
	else
	  i++;
      }
      if (debug)
	cout << TimeStamp() << msg << "feats.size()=" << feats.size()
	     << " [ " << ToStr(feats) << " ]" << endl
	     << TimeStamp() << msg << "feat_list.size()=" << feat_list.size()
	     << " [ " << ToStr(feat_list) << " ]" << endl;
    }

    vector<string> cls_list = SplitInCommas(cls_spec);

    string classcheck = classname;
    // size_t z = classcheck.find("-2012-pos"); // obs! obs! obs!
    // if (z!=string::npos)
    //   classcheck.erase(z);

    if (debug)
      cout << TimeStamp() << msg << "cls_spec=\"" << cls_spec
	   << "\" classname=\"" << classname << "\" classcheck=\""
	   << classcheck << "\" dimension=" << dimension << endl
	   << TimeStamp() << msg << "cls_list.size()=" << cls_list.size()
	   << " [ " << ToStr(cls_list) << " ]" << endl;

    if (dimension!=1 && dimension!=cls_list.size())
      return ShowError(msg+"dimension="+ToStr(dimension)+" cls_list.size()="+
		       ToStr(cls_list.size()));

    if (cls_list.size()==0 && type=="sentenceselection")
      cls_list.push_back("");

    for (size_t i=0; i<cls_list.size(); i++) {
      string classs = cls_list[i];
      if (classcheck!="" && classs!=classcheck)
	continue;

      if (dimension!=1)
	classs = cls_spec;

      for (size_t j=0; j<feat_list.size(); j++) {
	string feat = feat_list[j];
	vector<double> multi_val(multi_list.size());
	vector<string> inst_name(multi_list.size());

	for (size_t h=0; h<1 || h<multi_list.size(); h++) {
	  string instance;
	  if (multi_list.size())
	    instance = multi_list[h];

	  list<pair<string,string> > mcopy = m;
	  mcopy.push_back(make_pair("class",    classs));
	  mcopy.push_back(make_pair("features", feat));
	  if (multi_list.size())
	    mcopy.push_back(make_pair("instance", instance));
	  
	  double *valp  = NULL;
	  string *namep = NULL;
	  if (multi_val.size() && fusion!="none") {
	    valp  = &multi_val[h];
	    namep = &inst_name[h];
	  }
	  if (!CommonDetectionSingle(type, idx, mcopy, feats, augm, adseg,
				     valp, namep, tolerate_missing, hasdata,
				     xml))
	    return ShowError(msg+"features="+feat+" class="+classs+
			     " instance="+instance+" failed");
	}

	if (multi_val.size() && fusion!="none") {
	  if (debug) {
	    cout << endl << msg << "multi results:" << endl;
	    for (size_t h=0; h<multi_val.size(); h++) 
	      cout << "  " << inst_name[h] << "=" << multi_val[h] << endl;
	  }

	  string detname = inst_name[0];
	  auto p = detname.find("§");
	  if (p!=string::npos)
	    detname.erase(p);
	  p = detname.find("#");
	  if (p!=string::npos) {
	    if (extraout!="")
	      detname.insert(p, extraout);
	    else {
	      string s = "multi";
	      if (fusion=="arithmetic")
		s += "_arith";
	      if (fusion=="geometric")
		s += "_geom";
	      else if (fusion=="maximum")
		s += "_max";
	      else
		s += "_"+fusion;
	      s += "_"+ToStr(multi_val.size());
	      detname.insert(p, s);
	    }
	  }

	  size_t vcomp = 0;
	  double val = FusionResult(multi_val, fusion, vector<string>(),
				    vector<double>(), vcomp);
	  if (debug)
	    cout << TimeStamp() << msg << "combined result with " << fusion
		 << " : " << detname << " = " << val << endl;

	  bool incore = false;
	  if (!StoreDetectionResult(idx, detname, val, incore))
	    return ShowError(msg+"failed to store detection data");
	}
      }

      if (dimension!=1)
	break;
    }
    
    if (debug)
      cout << TimeStamp() << msg << "ending" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const list<string>& DataBase::FeatureRegExpList(const string& f) {
    string msg = "DataBase::DataBase::FeatureRegExpList() : ";

    feature_re_list_lock.LockWrite();
    if (feature_re_list.find(f)!=feature_re_list.end()) {
      feature_re_list_lock.UnlockWrite();
      return feature_re_list[f];
    }

    if (f.size()<2 || f[0]!='/' || f[f.size()-1]!='/') 
      feature_re_list[f] = list<string> { f };

    else {
      RegExp re(f.substr(1, f.size()-2));
      list<string> l;
      for (size_t i=0; i<Nindices(); i++)
	if (re.match(IndexName(i)))
	  l.push_back(IndexName(i));
      feature_re_list[f] = l;
    }

    feature_re_list_lock.UnlockWrite();

    return feature_re_list[f];
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::StoreDetectionResult(size_t idx, const string& detname,
				      float val, bool incore) {
    string msg = "DataBase::StoreDetectionResult(float) : ";

    Tic("StoreDetectionResult");

    bool ok = true, found = false;

    if (ok && storedetections.find("sql")!=string::npos) {
      found = true;
      ok = SqlInsertDetectionData(idx, detname, val);
    }

    if (ok && storedetections.find("bin")!=string::npos) {
      found = true;
      vector<float> vec { val };
      ok = BinInsertDetectionData(idx, detname, vec, incore);
    }

    if (!ok)
      return ShowError(msg+"storing failed");

    if (!found)
      return ShowError(msg+"no storing method specified");

    Tac("StoreDetectionResult");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::StoreDetectionResult(size_t idx, const string& detname,
				      const vector<float>& val, XmlDom& xml,
				      bool incore) {
    string msg = "DataBase::StoreDetectionResult(vector<float>) : ";

    Tic("StoreDetectionResult");

    bool ok = true, found = false;

    if (ok && storedetections.find("sql")!=string::npos) {
      found = true;
      ok = ShowError(msg+"vector form not available for SQL");
    }

    bool skip_bin = false;
    if (Picsom()->IsSlave() &&
	Picsom()->SlavePipe().find("detections")!=string::npos) {
      found = true;
      if (!xml)
	return ShowError(msg+"slave/pipe/xml mismatch");
      
      /// someday, unify this with Index::CalculateFeatures...
      string fdata((char*)&val[0], val.size()*sizeof(float));
      istringstream b64in(fdata);
      ostringstream b64out;
      if (!b64::encode(b64in, b64out))
	return ShowError(msg+"b64::encode() failed");

      XmlDom fe = xml.Element("detectionvector", "\n"+b64out.str());
      fe.Prop("database", Name());
      fe.Prop("name", detname);
      fe.Prop("label", Label(idx));
      fe.Prop("length", val.size());
      fe.Prop("type", "float");

      skip_bin = true;
    }

    if (!skip_bin && ok && storedetections.find("bin")!=string::npos) {
      found = true;
      ok = BinInsertDetectionData(idx, detname, val, incore);
    }

    if (!ok)
      return ShowError(msg+"storing failed");

    if (!found)
      return ShowError(msg+"no storing method specified");

    Tac("StoreDetectionResult");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::FindMatchingBinDetections(const string& s,
						   bool force) {
    string msg = "DataBase::FindMatchingBinDetections("+ToStr(s)+") : ";

    auto p = bindetections.find(s);
    if (p!=bindetections.end())
      return list<string> { s };

    vector<string> sv = SplitInCommasObeyParentheses(s);
    if (sv.size()>1) {
      list<string> ret;
      for (auto i=sv.begin(); i!=sv.end(); i++) {
	list<string> reti = FindMatchingBinDetections(*i, force);
	ret.insert(ret.end(), reti.begin(), reti.end());
      }
      return ret;
    }

    if (s!="" && s!="*" && s[0]!='/') {
      string f = ExpandPath("detections")+"/"+s+".bin";
      if (FileExists(f))
	return list<string> { s };
    }

    string sx = s;
    if (sx.size()>2 && sx[0]=='/' && sx[sx.size()-1]=='/')
      sx = sx.substr(1, sx.size()-2);

    list<string> d = FindAllBinDetectionNames(force), ret, empty;
    RegExp reg(sx, false);
    if (!reg.ok()) {
      ShowError(msg+"regcomp() failed "+reg.error());
      return empty;
    }

    for (auto i=d.begin(); i!=d.end(); i++) {
      bool match = reg.match(*i);
      if (DebugDetections()>2)
	WriteLog(msg+"["+*i+"] "+(match?"MATCH":"miss"));
      if (match) {
	// if (bindetections.find(*i)!=bindetections.end()) {
	//   string dir = ExpandPath("detections");
	//   if (!OpenBinDetection(*i, dir+"/"+*i+".bin")) {
	//     ShowError(msg+"OpenBinDetection() failed");
	//     return rmpty;
	//   }
	// }
	ret.push_back(*i);
      }
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::FindAllBinDetectionNames(bool force) {
    string msg = "DataBase::FindAllDetectionNames("+ToStr(force)+") : ";

    if (bindetectionnames.empty()||force) {
      list<string> allowed = SplitClassNames(alldetections);
      set<string> aset(allowed.begin(), allowed.end());

      bindetectionnames.clear();
      string dir = ExpandPath("detections");
      list<string> fl = Simple::SortedDir(dir, "", true, false);
      for (auto i=fl.begin(); i!=fl.end(); i++) {
	size_t l = i->size();
	if (l>4 && i->substr(l-4)==".bin") {
	  string d = i->substr(0, l-4);
	  if (aset.empty() || aset.find(d)!=aset.end()) {
	    bindetectionnames.push_back(d);
	    // OpenBinDetection(d, dir+"/"+*i);
	    if (DebugDetections()>2)
	      WriteLog(msg+"["+d+"]");
	  }
	}
      }
    }

    return bindetectionnames;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::OpenBinDetection(const string& dname, const string& fname,
				  size_t dim) {
    string msg = "DataBase::OpenBinDetection("+dname+","+fname+","+ToStr(dim)
      +") : ";
    if (bindetections.find(dname)!=bindetections.end())
      return ShowError(msg+"named detection already exists");

    bool incore = fname.find("/dev/null")==0;
    bool rw = OpenReadWriteDet()||incore;
    try {
      bindetections[dname].open(fname, rw,
				bin_data::header::format_float, 0, dim);
      if (!bindetections[dname].is_ok())
	return ShowError(msg+"open() failed");
    } catch (const string& emsg) {
      return ShowError(msg+"open() failed <"+emsg+">");
    }

    WriteLog("Opened binary detection data file <"+ShortFileName(fname)
	     +"> for "+(rw?"updating":"reading")
	     +"\n  "+bindetections[dname].str());
    
    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  map<string,vector<float> >
  DataBase::BinRetrieveDetectionData(size_t idx, const string& dnin,
				     bool angry, bool& exists) {
    string msg = "DataBase::BinRetrieveDetectionData("+
      ToStr(idx)+","+dnin+") : ";

    RwLockWrite("BinRetrieveDetectionData");

    map<string,vector<float> > ret;

    vector<string> dd = SplitInCommas(dnin);
    if (dd.size()==0 || (dd.size()==1 && dd[0]=="*")) {
      list<string> ll = FindAllBinDetectionNames(false);
      dd = vector<string>(ll.begin(), ll.end());

    } else if (dd.size()==1 && dd[0].size()>2 && dd[0][0]=='/' &&
	       dd[0][dd[0].size()-1]=='/') {
      list<string> ll = FindMatchingBinDetections(dnin);
      dd = vector<string>(ll.begin(), ll.end());
    }

    exists = false;

    for (size_t i=0; i<dd.size(); i++) {
      auto bd = bindetections.find(dd[i]);
      if (bd==bindetections.end()) {
	string fname = ExpandPath("detections")+"/"+dd[i]+".bin";
	if (!FileExists(fname))
	  fname = ExpandPath("features")+"/"+dd[i]+".bin";

	if (!FileExists(fname)) {
	  if (angry) {
	    ShowError(msg+"detection data for <"+dd[i]+"> not found");
	    ret.clear();
	    break;
	  }
	  continue;
	}

	if (!OpenBinDetection(dd[i], fname, 0)) { // obs! dim==0?
	  ShowError(msg+"opening bin data for <"+dd[i]+"> failed");
	  ret.clear();
	  break;
	}
	bd = bindetections.find(dd[i]);
      }

      if (dd.size()==1)
	exists = true;

      if (!bd->second.is_ok()) {
	ShowError(msg+"detection data for <"+dd[i]+"> not ok");
	ret.clear();
	break;
      }

      if (!bd->second.exists(idx))
	continue;

      vector<float> vv = bd->second.get_float(idx);
      // if (vv.size()!=1) {
      // 	ShowError(msg+"detection data size for <"+dd[i]+"> != 1");
      // 	ret.clear();
      // 	break;
      // }

      ret[dd[i]] = vv;
    }

    RwUnlockWrite("BinRetrieveDetectionData");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::BinInsertDetectionData(size_t idx, const string& detname,
					const vector<float>& vecval,
					bool incore) {
    float val = vecval[0];
    size_t dim = vecval.size();

    string msg = "DataBase::BinInsertDetectionData("+ToStr(idx)+","+
      detname+","+ToStr(val)+") : ";

    if (!incore && !OpenReadWriteDet())
      return ShowError(msg+"database should be opened -rw=...det...");

    if (idx>=Size())
      return ShowError(msg+"error, db size="+ToStr(Size()));

    // bool debug = true;

    RwLockWrite("BinInsertDetectionData");
    bool ret = true;

    string fname = "/dev/null/"+detname+".bin";
    if (bindetections.find(detname)==bindetections.end()) {
      if (!incore) {
	string dname = ExpandPath("detections");
	fname = dname+"/"+detname+".bin";
	if (!FileExists(fname)) {
	  Picsom()->MkDirHier(dname, 02775);
	  WriteLog("Creating binary detection file <"+ShortFileName(fname)+">");
	}
      }
      if (!OpenBinDetection(detname, fname, dim))
	return ShowError(msg+"OpenBinDetection() failed");
    }
    bin_data& bd = bindetections[detname];
    if (bd.vdim()!=dim)
      ret = ShowError(msg+"dimension mismatch, "+ToStr(bd.vdim())+" vs "+
		      ToStr(dim));

    try {
      if (!bd.is_open()) {  // this should never happen...
	bd.open(fname, true, bin_data::header::format_float, 0, 1);
	WriteLog("  "+bd.str());
      }

      if (bd.nobjects()<Size()) {
	WriteLog("Resizing binary detection file <"+ShortFileName(fname)+"> "
		 +ToStr(bd.nobjects())+" "+ToStr(Size()));
	bd.resize(Size());
	WriteLog("  "+bd.str());
      }
      // vector<float> vval { (float)val };
      if (!bd.set_float(idx, vecval))
	ret = ShowError(msg+"failed to set float value in bin data");

    } catch (const string& e) {
      ret = ShowError(msg+e);
    }

    RwUnlockWrite("BinInsertDetectionData");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::BinInsertDetectionDataOld(size_t idx, const string& detname,
					   float val) {
    string msg = "DataBase::BinInsertDetectionDataOld("+ToStr(idx)+","+
      detname+","+ToStr(val)+") : ";

    if (idx>=Size())
      return ShowError(msg+"error, db size="+ToStr(Size()));

    bool debug = true;

    string dname = ExpandPath("detections");
    Picsom()->MkDirHier(dname, 02775);

    string fname = dname+"/"+detname;

    off_t fsize = FileSize(fname);
    size_t endsize = Size()*sizeof(float);

    if (fsize>(off_t)endsize)
      return ShowError(msg+"excessively long file");

    if (fsize<(off_t)endsize) {
      bool is_new = fsize<0;
      if (is_new)
	fsize = 0;
      size_t bufsize = endsize-fsize;
      char *buf = new char[bufsize];
      memset(buf, 0, bufsize);

      if (is_new)
	WriteLog("Creating <"+ShortFileName(fname)+"> with "+ToStr(endsize)+
		 " bytes");

      ios_base::openmode mode = ios_base::out;
      if (!is_new)
	mode |= ios_base::app;

      ofstream fo(fname, mode);
      fo.write(buf, bufsize);

      if (!fo)
	return ShowError(msg+"padding failed");

      delete [] buf;
    }

    off_t fsize2 = FileSize(fname);
    if (debug)
      cout << "@ " << fsize << " " << fsize2 << " " << endsize << endl;

    if (fsize2!=(off_t)endsize)
      return ShowError(msg+"failed to create file of correct size");

    fstream fx(fname, ios::out|ios::in) ; // |ios_base::app

    if (debug)
      cout << "A " << fx.tellp() << endl;

    //fx.seekp(idx*sizeof(float), ios_base::beg);
    fx.seekp(idx*sizeof(float));
    if (!fx)
      return ShowError(msg+"seek failed A");

    if (debug)
      cout << "B " << fx.tellp() << endl;

    float fval = val;
    fx.write((char*)&fval, sizeof(fval));
    if (!fx)
      return ShowError(msg+"write failed");

    if (debug)
      cout << "C " << fx.tellp() << endl;

    fx.seekp(endsize);
    if (!fx)
      return ShowError(msg+"seek failed B");

    fx.close();
    if (debug)
      cout << "D " << FileSize(fname) << " was " << fsize
	   << " should be " << endsize << endl;

    if (FileSize(fname)!=(off_t)endsize)
      return ShowError(msg+"file size changed");

    return fx.good();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CommonDetectionSingle(const string& type, size_t idx,
				       const list<pair<string,string> >& m,
				       const list<string>& feats,
				       const string& augm, 
				       Segmentation *adseg, double *valp,
				       string *namep,
				       bool tolerate_missing,
				       bool& hasdata,
				       XmlDom& xml) {
    string msg = "DataBase::CommonDetectionSingle("+type+","
      +ToStr(idx)+") : ";

    bool debug = DebugDetections()>4;

    Tic("CommonDetectionSingle");

    AddDescription *ad = (AddDescription*)adseg;

    int frno = 0;
    map<string,string> hash = ReadOriginsInfo(idx, false, true);
    string url = hash["url"];
    size_t p1 = url.find('['), p2 = url.find(']');
    if (p1!=string::npos && p2!=string::npos && p1+1<p2)
      frno = atoi(url.substr(p1+1, p2-p1-1).c_str());
    
    if (ad)
      ad->getImg()->Description().sequenceinfo.lastframe = frno+1;

    if (debug)
      cout << TimeStamp() << msg << "starting" << endl;

    set<string> mandatory { "type", "name", "target", "class", "features" };
    set<string> allowed   { "metaclassfile", "database", "instance", "extra",
	"components" };
    // allowed += "frameno"

    map<string,string> par;
    list<pair<string,string> > kv;

    for (list<pair<string,string> >::const_iterator i=m.begin();
	 i!=m.end(); i++) {
      if (debug)
	cout << "  " << i->first << "=" << i->second << endl;

      if (mandatory.find(i->first)!=mandatory.end() ||
	  allowed.find(i->first)!=allowed.end()) {
	par[i->first] = i->second;
	continue;
      }

      kv.push_back(*i);
    }

    par["frameno"] = ToStr(frno);

    for (auto i=mandatory.begin(); i!=mandatory.end(); i++)
      if (par.find(*i)==par.end())
	return ShowError(msg+"param <"+*i+"> should be given");

    DataBase *db = this;
    if (par.find("database")!=par.end()) {
      db = Picsom()->FindDataBaseEvenNew(par["database"], false);
      if (!db)
	return ShowError(msg+"database <"+par["database"]+"> not found");
    }

    bool ret = false, known = false;

    if (type=="svmpred") {
      known = true;
      ret = SvmPredDetectionSingle(idx, par, kv, db, feats, augm,
				   adseg, valp, namep, tolerate_missing, hasdata);
    }

    if (type=="fusion") {
      known = true;
      ret = FusionDetectionSingle(idx, par, kv, db, feats, augm, adseg/*, valp*/);
    }

    if (type=="children") {
      known = true;
      ret = ChildrenDetectionSingle(idx, par, kv, db, feats, augm, adseg/*, valp*/);
    }

    if (type=="superclass") {
      known = true;
      ret = SuperClassDetectionSingle(idx, par, kv, db, feats, augm, adseg/*, valp*/);
    }

    if (type=="timefusion") {
      known = true;
      ret = TimeFusionDetectionSingle(idx, par, kv, db, feats, augm, adseg/*, valp*/);
    }

    if (type=="timethreshold") {
      known = true;
      ret = TimeThresholdDetectionSingle(idx, par, kv, db, feats, augm, adseg
					 /*, valp*/);
    }

    if (type=="sentenceselection") {
      known = true;
      ret = SentenceSelectionDetectionSingle(idx, par, kv, db, feats, augm,
					     adseg/*, valp*/);
    }

    if (type=="combine") {
      known = true;
      ret = CombineDetectionSingle(idx, par, kv, db, feats, augm, adseg/*, valp*/);
    }

    if (type=="lsc") {
      known = true;
      ret = LscDetectionSingle(idx, par, kv, db, feats, augm, adseg, valp, namep);
    }

    if (type=="random") {
      known = true;
      ret = RandomDetectionSingle(idx, par, kv, db, feats, augm, adseg, valp, namep);
    }

    if (type=="caffe") {
      known = true;
      ret = CaffeDetectionSingle(idx, par, kv, db, feats, augm, adseg, valp, namep,
				 xml);
    }

    Tac("CommonDetectionSingle");

    if (!known)
      return ShowError(msg+"type=<"+type+"> not understood");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::FusionDetectionSingle(size_t idx,
				       const map<string,string>& parin,
				       const list<pair<string,string> >& m,
				       DataBase*, const list<string>& /*feat*/,
				       const string& /*augm*/,
				       Segmentation*) {
    string msg = "DataBase::FusionDetectionSingle("+ToStr(idx)+") : ";

    Tic("FusionDetectionSingle");

    bool debug1 = DebugDetections()>1;
    bool debug2 = DebugDetections()>2;
    bool debug3 = DebugDetections()>3;

    map<string,string> par = parin;

    if (debug2)
      cout << TimeStamp() << msg << "starting class=" << par["class"]
	   << " features=" << par["features"] << endl;

    string fusion, detectors, components;
    bool incore = false;
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="fusion")
	fusion = i->second;
      else if (i->first=="detectors")
	detectors = i->second;
      else if (i->first=="components")
	components = i->second;
      else if (i->first=="incore")
	incore = IsAffirmative(i->second);
      else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (fusion=="" || detectors=="")
      return ShowError(msg+"fusion and detectors should be specified");

    if (!IsKnownFusion(fusion))
      return ShowError(msg+"fusion="+fusion+" not implemented");

    if (debug2)
      cout << TimeStamp() << msg << "fusion=" << fusion
	   << " detectors=" << detectors << endl;

    auto dets = SplitInCommas(detectors);

    float wgt_sum = 0.0;
    vector<vector<double> > val_vec;
    vector<double> wgt_vec;
    vector<string> det_vec;
    vector<string> f = SplitInCommasObeyParentheses(par["features"]);
    for (auto i=f.begin(); i!=f.end(); i++) {
      bool found = false;
      if (debug2)
	cout << TimeStamp() << msg << "  \"" << *i << "\" -> ";

      for (auto k=dets.begin(); k!=dets.end(); k++) {
	float wgt = 1;
	string d_w = *k, d = d_w;
	size_t slash = d_w.find('/');
	if (slash!=string::npos) {
	  d.erase(slash);
	  wgt = atof(d_w.substr(slash+1).c_str());
	}
	size_t j = d.find("%f");
	if (j!=string::npos)
	  d.replace(j, 2, *i);
	j = d.find("%c");
	if (j!=string::npos)
	  d.replace(j, 2, par["class"]);

	if (debug2)
	  cout << d << " / w=" << wgt << " ";

	bool dummy = true;
	map<string,vector<float> > detects
	  = RetrieveDetectionData(idx, d, true, dummy);
	if (debug3)
	  for (auto i=detects.begin(); i!=detects.end(); i++)
		cout << TimeStamp() << msg << "    " << i->first << " = "
		     << i->second[0] << endl; //obs! [0]

	auto p = detects.find(d);
	if (p!=detects.end()) {
	  wgt_sum += wgt;
	  wgt_vec.push_back(wgt);
	  det_vec.push_back(d);

	  for (size_t ii=0; ii<p->second.size(); ii++) {
	    if (debug2)
	      cout << p->second[ii] << "  ";

	    int cfy = fpclassify(p->second[ii]);
	    if (cfy==FP_NAN || cfy==FP_INFINITE)
	      ShowError(msg+"value is nan or infinite");

	    while (ii>=val_vec.size())
	      val_vec.push_back(vector<double>());

	    val_vec[ii].push_back(p->second[ii]);
	    found = true;
	  }
	} else
	  if (debug2)
	    cout << "not found  ";
      }
      
      if (debug2) {
	if (!found)
	  cout << "NOT FOUND AT ALL a";
	cout << endl;
      }
    }

    if (wgt_vec.size()&&wgt_sum) {
      if (debug3)
	cout << "  weights:";
      for (auto i=wgt_vec.begin(); i!=wgt_vec.end(); i++) {
	if (debug3)
	  cout << " " << *i;
	*i /= wgt_sum;
	if (debug3)
	  cout << " -> " << *i;
      }
      if (debug3)
	cout << endl;
    }

    set<size_t> pick_comp;
    string comp = par["components"];
    vector<string> cs = SplitInSpaces(comp);
    for (auto cc=cs.begin(); cc!=cs.end(); cc++)
      pick_comp.insert(atoi(cc->c_str()));

    bool onefound = false;
    vector<float> detresvec;
    for (size_t ii=0; ii<val_vec.size(); ii++) {
      if (val_vec[ii].size()) {
	size_t vcomp = 0;
	if (pick_comp.find(ii)!=pick_comp.end())
	  vcomp = 1;

	double val = FusionResult(val_vec[ii], fusion, det_vec, wgt_vec, vcomp);

	if (debug1)
	  cout << TimeStamp() << msg << "  ii=" << ii
	       << " total of " << val_vec[ii].size() << " values with "
	       << fusion << " fusion result of " << val << endl;

	detresvec.push_back(val);
	onefound = true;

      } else if (debug1)
	cout << TimeStamp() << msg << "  no input results for "
	     << fusion << " fusion, storing nothing" << endl;
    }

    if (onefound) {
      string detname = par["name"];
      // obs! until 2016-03-02 incore fusion was named fusion()#class
      // even though class name were given...
      bool class_missing = true;
      size_t p = detname.find("#");
      if (p!=string::npos && detname.substr(p, 3)!="#%c")
	class_missing = false;
      if (detresvec.size()==1 && class_missing) {
	bool replaced = false;
	for (;;) {
	  p = detname.find("#%c");
	  if (p==string::npos)
	    break;
	  detname.replace(p+1, 2, par["class"]);
	  replaced = true;
	}
	// until 2016-04-13 this was done always, then never
	// since 2016-05-02 this conditioned on !replaced...
	if (!replaced)
	  detname += "#"+par["class"];  
      }
      XmlDom xml; // obs!
      if (!StoreDetectionResult(idx, detname, detresvec, xml, incore))
	return ShowError(msg+"failed to store detection data");
    }

    Tac("FusionDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ChildrenDetectionSingle(size_t idx,
					 const map<string,string>& parin,
					 const list<pair<string,string> >& m,
					 DataBase*, const list<string>& feat,
					 const string& /*augm*/,
					 Segmentation*) {
    string msg = "DataBase::ChildrenDetectionSingle("+ToStr(idx)+") : ";

    Tic("ChildrenDetectionSingle");

    bool report_missing = false; // these happen for incore(incore(...))

    bool debug1 = DebugDetections()>1;
    bool debug2 = DebugDetections()>2;
    // bool debug3 = DebugDetections()>3;

    set<string> featset(feat.begin(), feat.end());
    map<string,string> par = parin;
    string classs = par["class"];

    if (debug2)
      cout << TimeStamp() << msg << "starting class=" << classs
	   << " features=" << par["features"] << endl;

    string fusion, detectors;
    bool incore = false;
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="fusion")
	fusion = i->second;
      else if (i->first=="detectors")
	detectors = i->second;
      else if (i->first=="incore")
	incore = IsAffirmative(i->second);
      else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (fusion=="" || detectors=="")
      return ShowError(msg+"fusion and detectors should be specified");

    if (!IsKnownFusion(fusion))
      return ShowError(msg+"fusion="+fusion+" not implemented");

    if (debug2)
      cout << TimeStamp() << msg << "fusion=" << fusion
	   << " detectors=" << detectors << endl;

    const object_info *oi = FindObject(idx);

    // ALL THIS SHOULD BE REMOVED
    // map<string,float> alldetects;
    // for (size_t c=0; c<oi->children.size(); c++) {
    //   // obs! is this stupid that all detections are fetched? YES.
    //   map<string,float> detects = RetrieveDetectionData(oi->children[c]);
    //   for (auto i=detects.begin(); i!=detects.end(); i++) {
    // 	string k = "#"+ToStr(oi->children[c])+"::"+i->first;
    // 	alldetects[k] = i->second;
    // 	if (debug3)
    // 	  cout << TimeStamp() << msg << "   " << k << " = " << i->second
    // 	       << endl;
    //   }
    // }

    auto dets = SplitInCommasObeyParentheses(detectors);

    vector<double> val_vec;
    vector<string> det_vec;
    vector<string> f = SplitInCommasObeyParentheses(par["features"]);
    for (auto i=f.begin(); i!=f.end(); i++) {
      if (featset.size() && featset.find(*i)==featset.end())
	continue;

      bool found = false;
      if (debug2)
	cout << TimeStamp() << msg << "  \"" << *i << "\" -> ";

      for (auto k=dets.begin(); k!=dets.end(); k++) {
	string dd = *k;
	size_t j = dd.find("%f");
	if (j!=string::npos)
	  dd.replace(j, 2, *i);
	string ddmatch = dd;
	j = ddmatch.find("#%c");
	if (j==ddmatch.size()-3)
	  ddmatch.replace(j+1, 2, classs);
	else if (ddmatch[ddmatch.size()-1]==')')
	  ddmatch += "#"+classs;

	if (debug2)
	  cout << ddmatch << " | ";

	for (size_t c=0; c<oi->children.size(); c++) {
	  string d = "#"+ToStr(oi->children[c])+"@"+ddmatch;

	  if (debug2)
	    cout << d << "=";

	  bool dummy = true;
	  map<string,vector<float> > alldetects =
	    RetrieveOrProduceDetectionData(oi->children[c], dd, classs,
					   true, dummy, incore);
	  auto p = alldetects.find(ddmatch);

	  if (p!=alldetects.end()) {
	    if (p->second.size()!=1) {
	      if (report_missing)
		ShowError(msg+"detection dimensionality != 1");
	    } else {
	      if (debug2)
		cout << p->second[0] << "  ";

	      int cfy = fpclassify(p->second[0]);
	      if (cfy==FP_NAN || cfy==FP_INFINITE)
		ShowError(msg+"value is nan or infinite");

	      val_vec.push_back(p->second[0]);
	      det_vec.push_back(dd);
	      found = true;
	    }

	  } else
	    if (debug2)
	      cout << "not found  ";
	}
	// obs! should report stronger if not all children have the value?
      }
      
      if (debug2) {
	if (!found)
	  cout << "NOT FOUND AT ALL b";
	cout << endl;
      }

      if (val_vec.size()) {
	size_t vcomp = 0;
	double val = FusionResult(val_vec, fusion, det_vec, vector<double>(),
				  vcomp);
	
	if (debug1)
	  cout << TimeStamp() << msg << "  total of " << val_vec.size()
	       << " values with "
	       << fusion << " fusion result of " << val << endl;

	string detname = par["name"]+"#"+classs;
	size_t p = detname.find("%f");
	if (p!=string::npos && *i!="")
	  detname.replace(p, 2, *i);

	vector<float> valvec { (float)val };
	XmlDom xml; // obs!
	if (!StoreDetectionResult(idx, detname, valvec, xml, incore))
	  return ShowError(msg+"failed to store detection data");
	
      } else if (debug1)
	cout << TimeStamp() << msg << "  no input results for "
	     << fusion << " fusion, storing nothing" << endl;
    }

    Tac("ChildrenDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SuperClassDetectionSingle(size_t idx,
					   const map<string,string>& parin,
					   const list<pair<string,string> >& m,
					   DataBase*, const list<string>&/*feat*/,
					   const string& /*augm*/,
					   Segmentation*) {
    string msg = "DataBase::SuperClassDetectionSingle("+ToStr(idx)+") : ";

    Tic("SuperClassDetectionSingle");

    bool report_missing = false; // these happen for incore(incore(...))

    bool debug1 = DebugDetections()>1;
    bool debug2 = DebugDetections()>2;
    // bool debug3 = DebugDetections()>3;

    map<string,string> par = parin;
    string classs = par["class"];

    string fusion, detectors;
    bool incore = false;
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="fusion")
	fusion = i->second;
      else if (i->first=="detectors")
	detectors = i->second;
      else if (i->first=="incore")
	incore = IsAffirmative(i->second);
      else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (fusion=="" || detectors=="")
      return ShowError(msg+"fusion and detectors should be specified");

    if (!IsKnownFusion(fusion))
      return ShowError(msg+"fusion="+fusion+" not implemented");

    if (debug2)
      cout << TimeStamp() << msg << "starting metaclass=" << classs
	   << " fusion=" << fusion << " detectors=" << detectors << endl;

    // const object_info *oi = FindObject(idx);

    auto dets = SplitInCommasObeyParentheses(detectors);

    if (dets.size()==1) {
      string d = dets[0];
      size_t p = d.find("%C");
      if (p!=string::npos) {
	dets.clear();
	list<string> cl = SplitClassNames(classs);
	for (auto i=cl.begin(); i!=cl.end(); i++) {
	  string dd = d;
	  dd.replace(p, 2, *i);
	  dets.push_back(dd);
	}
      }
    }

    vector<double> val_vec;
    vector<string> det_vec;

    bool found = false;

    if (debug2)
      cout << classs << " #" << idx << " <" << Label(idx) << "> ";
      
    for (auto k=dets.begin(); k!=dets.end(); k++) {
      string dd = *k;

      if (debug2)
	cout << "| #" << idx << "@" << dd << "=";
      
      bool dummy = true;
      map<string,vector<float> > alldetects =
	RetrieveOrProduceDetectionData(idx, dd, classs, true, dummy, incore);
      auto p = alldetects.find(dd);

      if (p!=alldetects.end()) {
	if (p->second.size()!=1) {
	  if (report_missing)
	    ShowError(msg+"detection dimensionality != 1");
	} else {
	  if (debug2)
	    cout << p->second[0] << " ";
	  
	  int cfy = fpclassify(p->second[0]);
	  if (cfy==FP_NAN || cfy==FP_INFINITE)
	    ShowError(msg+"value is nan or infinite");
	  
	  val_vec.push_back(p->second[0]);
	  det_vec.push_back(dd);
	  found = true;
	}
	
      } else
	if (debug2)
	  cout << "not found  ";
    }
    // obs! should report stronger if not all children have the value?
      
    if (debug2) {
      if (!found)
	cout << "NOT FOUND AT ALL b";
      cout << endl;
    }

    if (val_vec.size()) {
      size_t vcomp = 0;
      double val = FusionResult(val_vec, fusion, det_vec, vector<double>(),
				vcomp);
	
      if (debug1)
	cout << TimeStamp() << msg << "  total of " << val_vec.size()
	     << " values with "
	     << fusion << " fusion result of " << val << endl;

      string detname = par["name"];
      size_t p = detname.find("%C");
      if (p!=string::npos)
	detname.replace(p, 2, classs);	

      vector<float> valvec { (float)val };
      XmlDom xml; // obs!
      if (!StoreDetectionResult(idx, detname, valvec, xml, incore))
	return ShowError(msg+"failed to store detection data");
	
    } else if (debug1)
      cout << TimeStamp() << msg << "  no input results for "
	   << fusion << " fusion, storing nothing" << endl;

    Tac("ChildrenDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::TimeFusionDetectionSingle(size_t idx,
					   const map<string,string>& parin,
					   const list<pair<string,string> >& m,
					   DataBase*, const list<string>& feat,
					   const string& /*augm*/,
					   Segmentation*) {
    string msg = "DataBase::TimeFusionDetectionSingle("+ToStr(idx)+") : ";

    Tic("TimeFusionDetectionSingle");

    bool debug1 = DebugDetections()>1;
    bool debug2 = DebugDetections()>2;
    // bool debug3 = DebugDetections()>3;

    set<string> featset(feat.begin(), feat.end());
    map<string,string> par = parin;

    string features = par["features"];

    if (debug2)
      cout << TimeStamp() << msg << "starting class=" << par["class"]
	   << " features=[" << features << "]" << endl;

    string fusion, detectors, length, segment;
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="fusion")
	fusion = i->second;
      else if (i->first=="detectors")
	detectors = i->second;
      else if (i->first=="length")
	length = i->second;
      else if (i->first=="segment")
	segment = i->second;
      else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (fusion=="" || detectors=="" || (length=="" && segment==""))
      return ShowError(msg+"fusion, detectors and length/segment"
		       " should be specified");

    if (segment!="") {
      const string& label = Label(idx);
      if (label.find(segment)!=0 || label[segment.size()]!=':') {
	if (debug2)
	  cout << TimeStamp() << msg << "label=<" << label
	       << "> doesn't match segment=<" << segment << ">" << endl;
	return true;
      }
    }

    int length_i = atoi(length.c_str());
    size_t ai = length_i/2, bi = ai;

    if (!IsKnownFusion(fusion))
      return ShowError(msg+"fusion="+fusion+" not implemented");

    if (debug2)
      cout << TimeStamp() << msg << "fusion=" << fusion
	   << " detectors=" << detectors << endl;

    auto dets = SplitInCommas(detectors);

    vector<double> val_vec;
    vector<string> det_vec;
    vector<string> f = SplitInCommasObeyParentheses(features);
    for (auto i=f.begin(); i!=f.end(); i++) {
      if (featset.size() && featset.find(*i)==featset.end() && features!="")
	continue;

      bool found = false;
      if (debug2)
	cout << TimeStamp() << msg << "  \"" << *i << "\" "
	     << -(int)ai << ".." << bi << " -> ";

      for (auto k=dets.begin(); k!=dets.end(); k++) {
	string dd = *k;
	size_t j = dd.find("%f");
	if (j!=string::npos)
	  dd.replace(j, 2, *i);
	j = dd.find("%c");
	if (j!=string::npos)
	  dd.replace(j, 2, par["class"]);

	if (debug2)
	  cout << dd << " | ";

	vector<pair<size_t,size_t> > idx_vec;
	if (segment=="") {
	  // obs! nothing actually ensures here that these are
	  // consequtive frames
	  size_t idx_a = idx>ai ? idx-ai : 0;
	  size_t idx_b = idx+bi<Size() ? idx+bi : Size()-1;
	  for (size_t c=idx_a; c<=idx_b; c++)
	    if (ObjectsTargetType(c)==ObjectsTargetType(idx)&&
		ParentObject(c)==ParentObject(idx))
	      idx_vec.push_back(make_pair(c, 0)); // obs! hard-coded 0

	} else
	  idx_vec = VideoOrSegmentFramesOrdered(idx);

	for (size_t cc=0; cc<idx_vec.size(); cc++) {
	  size_t c = idx_vec[cc].first;
	  string d = "#"+ToStr(c)+"@"+dd;

	  if (debug2)
	    cout << d << "=";

	  bool dummy = true;
	  map<string,vector<float> > alldetects =
	    RetrieveDetectionData(c, dd, true, dummy);
	  auto p = alldetects.find(dd);

	  if (p!=alldetects.end()) {
	    if (p->second.size()!=1)
	      ShowError(msg+"detection dimensionality != 1");

	    if (debug2)
	      cout << p->second[0] << "  ";

	    int cfy = fpclassify(p->second[0]);
	    if (cfy==FP_NAN || cfy==FP_INFINITE)
	      ShowError(msg+"value is nan or infinite");

	    val_vec.push_back(p->second[0]);
	    det_vec.push_back(dd);
	    found = true;

	  } else
	    if (debug2)
	      cout << "not found  ";
	}
      }
      
      if (debug2) {
	if (!found)
	  cout << "NOT FOUND AT ALL c";
	cout << endl;
      }

      if (val_vec.size()) {
	size_t vcomp = 0;
	double val = FusionResult(val_vec, fusion, det_vec, vector<double>(),
				  vcomp);
	
	if (debug1)
	  cout << TimeStamp() << msg << "  total of " << val_vec.size()
	       << " values with "
	       << fusion << " fusion result of " << val << endl;

	string detname = par["name"]+"#"+par["class"];
	size_t p = detname.find("%f");
	if (p!=string::npos && *i!="")
	  detname.replace(p, 2, *i);

	bool incore = false;
	if (!StoreDetectionResult(idx, detname, val, incore))
	  return ShowError(msg+"failed to store detection data");
	
      } else if (debug1)
	cout << TimeStamp() << msg << "  no input results for "
	     << fusion << " fusion, storing nothing" << endl;
    }

    Tac("TimeFusionDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool
  DataBase::TimeThresholdDetectionSingle(size_t idx,
					 const map<string,string>& parin,
					 const list<pair<string,string> >& m,
					 DataBase*, const list<string>& feat,
					 const string& /*augm*/,
					 Segmentation*) {
    string msg = "DataBase::TimeThresholdDetectionSingle("+ToStr(idx)+") : ";

    Tic("TimeThresholdDetectionSingle");

    bool debug1 = DebugDetections()>1;
    bool debug2 = DebugDetections()>2;
    // bool debug3 = DebugDetections()>3;

    set<string> featset(feat.begin(), feat.end());
    map<string,string> par = parin;

    if (debug2)
      cout << TimeStamp() << msg << "starting class=" << par["class"]
	   << " features=" << par["features"] << endl;

    string detectors, minlength, threshold;
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="detectors")
	detectors = i->second;
      else if (i->first=="minlength")
	minlength = i->second;
      else if (i->first=="threshold")
	threshold = i->second;
      else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (detectors=="" || minlength=="" || threshold=="")
      return ShowError(msg+"detectors, minlength "
		       "and threshold should be specified");

    float minlength_f = atof(minlength.c_str());
    size_t p = minlength.find_first_not_of("0123456789.");
    if (p!=string::npos) {
      if (minlength[p]!='s')
	return ShowError(msg+"minlength=\""+minlength+
			 "\" could not be interpreted");
      float fps = 25; // obs!
      minlength_f *= fps;
    }
    size_t minlength_i = size_t(floor(minlength_f+0.5));

    float thr_min = atof(threshold.c_str()), thr_max = thr_min;
    p = threshold.find(' ');
    if (p!=string::npos)
      thr_max = atof(threshold.substr(p).c_str());

    if (debug2)
      cout << TimeStamp() << msg << " detectors=" << detectors
	   << " minlength=" << minlength_i
	   << " thr_min=" << thr_min << " thr_max=" << thr_max << endl;

    auto dets = SplitInCommas(detectors);

    vector<string> f = SplitInCommasObeyParentheses(par["features"]);
    for (auto i=f.begin(); i!=f.end(); i++) {
      if (featset.size() && featset.find(*i)==featset.end())
	continue;

      if (debug2)
	cout << TimeStamp() << msg << "  \"" << *i << "\" -> ";

      for (auto k=dets.begin(); k!=dets.end(); k++) {
	string dd = *k;
	size_t j = dd.find("%f");
	if (j!=string::npos)
	  dd.replace(j, 2, *i);
	j = dd.find("%c");
	if (j!=string::npos)
	  dd.replace(j, 2, par["class"]);

	if (debug2)
	  cout << dd << " | ";

	size_t last = idx;
	bool found = false, hit_found = false;
	float validx = 0, maxval = 0;

	map<size_t,float> stored;

	for (size_t c=idx; c<Size(); c++) {
	  if (ObjectsTargetType(c)!=ObjectsTargetType(idx)||
	      ParentObject(c)!=ParentObject(idx))
	    break;

	  string d = "#"+ToStr(c)+"@"+dd;

	  if (debug2)
	    cout << d << "=";

	  bool dummy = true;
	  map<string,vector<float> > alldetects =
	    RetrieveDetectionData(c, dd, true, dummy);
	  auto p = alldetects.find(dd);

	  if (p!=alldetects.end()) {
	    if (p->second.size()!=1)
	      ShowError(msg+"detection dimensionality != 1");

	    if (debug2)
	      cout << p->second[0] << "  ";

	    int cfy = fpclassify(p->second[0]);
	    if (cfy==FP_NAN || cfy==FP_INFINITE)
	      ShowError(msg+"value is nan or infinite");

	    found = true;

	    float val = p->second[0];
	    if (c==idx)
	      validx = val;

	    if (val>maxval)
	      maxval = val;

	    if (val<thr_min)
	      break;

	    if (val>=thr_max)
	      hit_found = true;

	    stored[c] = val;

	    last = c;

	  } else {
	    if (debug2)
	      cout << "not found  ";
	    break;
	  }
	}
      
	if (debug2) {
	  if (!found)
	    cout << "NOT FOUND AT ALL d";
	  cout << endl;
	}

	size_t length = last-idx+1;
	bool length_ok = length>=minlength_i;

	if (debug1)
	  cout << TimeStamp() << msg
	       << " idx=" << idx << " last=" << last
	       << " length=" << length
	       << " val@idx=" << validx
	       << " maxval=" << maxval
	       << " hit_found=" << int(hit_found)
	       << " length_ok=" << int(length_ok)
	       << endl;

	string detname = par["name"]+"#"+par["class"];
	size_t p = detname.find("%f");
	if (p!=string::npos && *i!="")
	  detname.replace(p, 2, *i);
	
	for (size_t c=idx; c<=last; c++) {
	  float val = hit_found&&length_ok ? stored[c] : 0.0;
	
	  bool incore = false;
	  if (!StoreDetectionResult(c, detname, val, incore))
	    return ShowError(msg+"failed to store detection data");
	}
      }
    }

    Tac("TimeThresholdDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::
  SentenceSelectionDetectionSingle(size_t idx,
				   const map<string,string>& parin,
				   const list<pair<string,string> >& m,
				   DataBase*, const list<string>& /*feat*/,
				   const string& /*augm*/,
				   Segmentation*) {
    string msg = "DataBase::SentenceSelectionDetectionSingle("+
      ToStr(idx)+") : ";

    Tic("SentenceSelectionDetectionSingle");

    // bool debug1 = DebugDetections()>1;
    bool debug2 = DebugDetections()>2;

    map<string,string> par = parin;
    string name      = par["name"];
    string features  = par["features"];
    string classname = par["class"];

    if (features!="" || classname!="" /* || feat.size()*/ )
      return ShowError(msg+"features nor classname should be set.");

    size_t incount = 0, evalcount = 0, outcount = 0;
    string fusion, measure, segment, detectors, evaluators;
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="fusion")
	fusion = i->second;
      else if (i->first=="measure")
	measure = i->second;
      else if (i->first=="segment")
	segment = i->second;
      else if (i->first=="detectors")
	detectors = i->second;
      else if (i->first=="evaluators")
	evaluators = i->second;
      else if (i->first=="incount")
	incount	= atoi(i->second.c_str());
      else if (i->first=="evalcount")
	evalcount = atoi(i->second.c_str());
      else if (i->first=="outcount")
	outcount = atoi(i->second.c_str());
      else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (debug2)
      cout << TimeStamp() << msg << "starting with fusion=" << fusion
	   << " measure=" << measure << " segment=" << segment
	   << " detectors=" << detectors << " evaluators=" << evaluators
	   << " incount=" << incount << " evalcount=" << evalcount
	   << " outcount=" << outcount << endl;

    if (detectors=="")
      return ShowError(msg+"detectors should be set");

    vector<string> dets = SplitInCommasObeyParentheses(detectors);

    if (fusion=="")
      return ShowError(msg+"fusion should be specified");

    if (segment!="") {
      const string& label = Label(idx);
      if (label.find(segment)!=0 || label[segment.size()]!=':') {
	if (debug2)
	  cout << TimeStamp() << msg << "label=<" << label
	       << "> doesn't match segment=<" << segment << ">" << endl;
	return true;
      }
    }

    if (!IsKnownFusion(fusion))
      return ShowError(msg+"fusion="+fusion+" not implemented");

    if (debug2)
      cout << TimeStamp() << msg << "fusion=" << fusion
    	   << " segment=[" << segment << "] detectors=["
	   << CommaJoin(dets) << "]" << endl;

    vector<pair<size_t,size_t> > idx_vec;
    if (segment!="")
      idx_vec = VideoOrSegmentFramesOrdered(idx);
    else
      idx_vec.push_back(make_pair(idx, (size_t)-1));

    cout << Label(idx) << " : " << endl;

    size_t ns = 0;
    map<string,size_t> candidate_count;
    for (size_t cc=0; cc<idx_vec.size(); cc++) {
      size_t idx = idx_vec[cc].first;

      for (auto s=dets.begin(); s!=dets.end(); s++) {
	string spec = "gene-"+*s;
	if (debug2) {
	  string tf = ObjectTextFileSubdirPath(idx, "", spec);
	  cout << "  " << tf << endl;
	}

	textline_t tl = ObjectTextLineRetrieve(idx, "", spec, true);
	size_t ic = 0;
	for (auto i=tl.txt_val.begin();
	     i!=tl.txt_val.end() && (!incount || ic<incount); i++, ic++) {
	  string ss = i->first;
	  if (debug2) 
	    cout << "    " << Label(idx) << " " << *s << " gene " << ss
		 << " (" << i->second << ")" << endl;
	  candidate_count[ss]++;
	  ns++;
	}
      }
    }

    textline_t res;
    res.db  = this;
    res.idx = idx;

    if (measure=="counts") {
      if (fusion!="maximum")
	return ShowError(msg+"with measure=counts should be fusion=maximum");

      multimap<size_t,string> count_order;
      for (auto i=candidate_count.begin(); i!=candidate_count.end(); i++)
	count_order.insert(make_pair(i->second, i->first));

      if (debug2) {
	cout << "  total " << ns << " sentences, "
	     << candidate_count.size() << " unique: " << endl;

	for (auto i=count_order.rbegin(); i!=count_order.rend(); i++)
	  cout << "    " << i-> first << " x " << i->second << endl;
      }

      if (segment!="") {
	auto psd = ParentStartDuration(idx, target_video);
	res.start = psd.second.first;
	res.end   = res.start+psd.second.second;
      }

      if (count_order.size())
	res.txt_val.push_back(make_pair(count_order.rbegin()->second, 0));

    } else if (measure=="logprob") {
      if (segment!="")
	return ShowError(msg+"measure=logprob possibly doesn't work "
			 "with segment");

      if (fusion!="arithmetic")
	return ShowError(msg+"with measure=logprob should be "
			 "fusion=arithmetic");
      if (evaluators=="")
	return ShowError(msg+"with measure=logprob evaluators should be set");

      vector<string> evaluations = SplitInCommasObeyParentheses(evaluators);
      map<string,vector<double> > evals;
      for (auto s=evaluations.begin(); s!=evaluations.end(); s++) {
	string spec = "eval-"+*s;
	if (debug2) {
	  string tf = ObjectTextFileSubdirPath(idx, "", spec);
	  cout << "  " << tf << endl;
	}

	bool angry = false;
	textline_t tl = ObjectTextLineRetrieve(idx, "", spec, angry);
	// cout << "#" << idx << " <" << *s << "> " << tl.str() << endl;

	for (size_t j=0; j<tl.txt_val.size(); j++) {
	  if (debug2) 
	    cout << "    " << Label(idx) << " " << *s
		 << " eval " << tl.txt_val[j].first
		 << " (" << tl.txt_val[j].second << ")" << endl;
	  evals[tl.txt_val[j].first].push_back(tl.txt_val[j].second);
	}
      }

      multimap<double,string> score;
      for (auto j=candidate_count.begin(); j!=candidate_count.end(); j++) {
	const vector<double>& v = evals[j->first];
	double sum = 0;
	cout << "#" << idx << " \"" << j->first << "\" n=" << v.size();
	for (size_t k=0; k<v.size(); k++) {
	  cout << " " << v[k];
	  sum += v[k];
	}
	sum /= v.size();
	cout << " => " << sum << endl; 
	score.insert(make_pair(sum, j->first));
	if (v.size()!=evaluations.size())
	  return ShowError(msg+"evaluations.size()="+ToStr(evaluations.size()));
      }

      size_t oc = 0;
      for (auto j=score.rbegin();
	   j!=score.rend() && (!outcount || oc<outcount); j++, oc++) {
	res.add(j->second, j->first);
	
	if (debug2)
	  cout << " >>>> " << j->first << " \"" << j->second << "\"" << endl;
      }

    } else
      return ShowError(msg+"measure="+measure+" unknown");

    ObjectTextLineStore(res, "", name);

    Tac("SentenceSelectionDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CombineDetectionSingle(size_t idx,
					const map<string,string>& parin,
					const list<pair<string,string> >& m,
					DataBase*, const list<string>& feat,
					const string& /*augm*/,
					Segmentation*) {
    string msg = "DataBase::CombineDetectionSingle("+ToStr(idx)+") : ";

    Tic("CombineDetectionSingle");

    // bool debug1 = DebugDetections()>1;
    bool debug2 = DebugDetections()>2;
    // bool debug3 = DebugDetections()>3;

    if (feat.size()!=0)
      return ShowError(msg+"currently no feature arguments are supported");

    set<string> featset(feat.begin(), feat.end());
    map<string,string> par = parin;

    if (debug2)
      cout << TimeStamp() << msg << "starting class=" << par["class"]
	   << endl;

    string operation, detectors;
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="operation")
	operation = i->second;
      else if (i->first=="detectors")
	detectors = i->second;
      else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (operation=="" || detectors=="")
      return ShowError(msg+"operation and detectors should be specified");

    if (debug2)
      cout << TimeStamp() << msg << "operation=" << operation
	   << " detectors=" << detectors << endl;

    if (operation.find("and")!=0)
      return ShowError(msg+"currently only operation=and[<or-frac>] "
		       "is supported");

    float or_frac = atof(operation.substr(3).c_str());

    auto dets = SplitInCommas(detectors);
    if (dets.size()!=1)
      return ShowError(msg+"currently only one detector is supported");

    string cls1p2 = par["class"];
    size_t p = cls1p2.find('+');
    if (p==string::npos)
      return ShowError(msg+"'+' not found in class=<"+cls1p2+">");

    string cls1 = cls1p2.substr(0, p), cls2 = cls1p2.substr(p+1);

    string dd1 = detectors+"#"+cls1, dd2 = detectors+"#"+cls2;
    string dd = dd1+","+dd2;
    bool dummy = true;
    map<string,vector<float> > det = RetrieveDetectionData(idx, dd,
							   true, dummy);
    if (det.size()!=2)
      return ShowError(msg+"2 detection results not retrieved");

    vector<float>& av = det[dd1], bv = det[dd2];
    if (av.size()!=1 || bv.size()!=1)
      return ShowError(msg+"detection dimensionality != 1");

    float a = av[0], b = bv[0];
    float vand = a*b, vor = a+b-vand, val = (1-or_frac)*vand+or_frac*vor;

    if (debug2)
      cout << TimeStamp() << msg << " : " << dd1 << "=" << a
	   << " " << dd2 << "=" << b << " -> "
	   << vand << " / " << vor << " (" << or_frac << ") -> " << val
	   << endl;
    
    string detname = par["name"]+"#"+par["class"];

    bool incore = false;
    if (!StoreDetectionResult(idx, detname, val, incore))
      return ShowError(msg+"failed to store detection data");

    Tac("CombineDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsKnownFusion(const string& m) {
    if (m=="")
      return false;

    vector<string> v = SplitInSomething("-", false, m);
    const string a = v[0];
    if (a!="maximum" && a!="geometric" && a!="arithmetic" &&
	a!="harmonic" && a!="pick")
      return false;

    if (v.size()==1)
      return true;

    if (v.size()==2 && v[1]=="cdf")
      return true;

    if (v.size()!=3)
      return false;

    if (v[1]=="inv" && v[2]=="rank")
      return true;

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::FusionResult(const vector<double>& vv,
			       const string& min,
			       const vector<string>& det,
			       const vector<double>& weight,
			       size_t comp) {
    string msg = "DataBase::FusionResult("+ToStr(vv.size())+","+min+","
      +ToStr(det.size())+","+ToStr(comp)+") : ";

    bool debug = DebugDetections()>3;

    if (debug)
      cout << msg;

    if (!IsKnownFusion(min) || vv.size()==0)
      return 0.0;

    string rankf;
    string m = min;
    size_t p = m.find("-inv-rank");
    if (p!=string::npos) {
      rankf = "inv";
      m.erase(p);
    }
    p = m.find("-cdf");
    if (p!=string::npos) {
      rankf = "cdf";
      m.erase(p);
    }

    bool has_zero = false;
    double prod = 1.0, max = 0.0, sum = 0.0, isum = 0.0;
    size_t j = 0;
    for (auto i=vv.begin(); i!=vv.end(); i++, j++) {
      float v = *i;
      float mul = weight.size()==vv.size() ? weight[j] : 1.0;
      if (debug)
	cout << " " << v << " w=" << mul;

      if (rankf!="") {
	size_t rr = 0;
	v = RankBasedValue(det[j], rankf, v, rr);
	if (debug)
	  cout << "->" << rr << "->" << v;
      }

      prod *= v;
      sum  += v*mul;  // obs! NOT YET USED FOR OTHER MEASURES...
      if (v>max)
	max = v;
      if (v>0)
	isum += 1/v;
      else
	has_zero = true;
    }

    bool done = false;
    float val = 0.0;
    if (m=="maximum") {
      val = max;
      done = true;
    }

    else if (m=="geometric") {
      val = pow(prod, 1.0/double(vv.size()));
      done = true;
    }

    else if (m=="arithmetic") {
      val = weight.size()==vv.size() ? sum : sum/double(vv.size());
      done = true;
    }

    else if (m=="harmonic") { 
      if (!has_zero)
	val = double(vv.size())/isum;
      done = true;
    }

    else if (m=="pick") { 
      if (comp>=vv.size())
	ShowError(msg+"index "+ToStr(comp)+" out of bounds for pick");
      else
	val = vv[comp];
      done = true;
    }

    if (!done) 
      ShowError(msg+"fusion method \""+m+"\" not known");

    if (debug)
      cout << "   val=" << val << endl;

    return val;
  }

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::RankBasedValue(const string& det, const string& f, float v,
				 size_t& rr) {
    pair<size_t,size_t> r = Rank(det, v);

    rr = r.first;

    if (r.second==0)
      return 0;

    if (f=="inv")
      return 1.0/(r.first+1);

    if (f=="cdf")
      return ((float)r.second-r.first)/r.second;

    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<size_t,size_t> DataBase::Rank(const string& det, float v) {
    auto rm = ranks.find(det);
    if (rm==ranks.end()) {
      bin_data& bd = bindetections[det];
      vector<size_t> idxs = RestrictionGT().indices(1);
      map<float,size_t> vmap, rmap;
      size_t n = 0, t = 0;
      for (size_t i=0; i<idxs.size(); i++)
	if (bd.exists(idxs[i])) {
	  float x = bd.get_float(idxs[i])[0];
	  vmap[x]++;
	  n++;
	}
      for (auto i=vmap.rbegin(); i!=vmap.rend(); i++) {
	rmap[i->first] = t;
	t += i->second;
      }
      ranks[det] = make_pair(t, rmap);
      rm = ranks.find(det);

      stringstream ss;
      if (DebugDetections()>4) {
	ss << endl << "  idxs.size()=" << idxs.size() << " n=" << n << endl;
	size_t imax = 5, i = imax;
	for (auto p=vmap.begin(); i!=0 && p!=vmap.end(); p++, i--)
	  ss << "  vmap front " << p->first << " " << p->second << endl;
	i = imax;
	for (auto p=vmap.rbegin(); i!=0 && p!=vmap.rend(); p++, i--)
	  ss << "  vmap back  " << p->first << " " << p->second << endl;
	i = imax;
	for (auto p=rmap.begin(); i!=0 && p!=rmap.end(); p++, i--)
	  ss << "  rmap front " << p->first << " " << p->second << endl;
	i = imax;
	for (auto p=rmap.rbegin(); i!=0 && p!=rmap.rend(); p++, i--)
	  ss << "  rmap back  " << p->first << " " << p->second << endl;
      }

      if (DebugDetections()>2)
	WriteLog("Created rank hash for <"+det+"> with "+ToStr(t)
		 +" values and "+ToStr(rmap.size())+" entries"+ss.str());
    }

    auto r = rm->second.second.find(v);
    
    return r==rm->second.second.end() ? make_pair((size_t)0, (size_t)0) :
      make_pair(r->second, rm->second.first);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SvmPredDetectionSingle(size_t idx,
					const map<string,string>& par,
					const list<pair<string,string> >& m,
					DataBase *db, const list<string>& feats,
					const string& augm,
					Segmentation *adseg,
					double *valp, string *namep,
					bool tolerate_missing,
					bool& hasdata) {
    string msg = "DataBase::SvmPredDetectionSingle("+ToStr(idx)+") : ";

    Tic("SvmPredDetectionSingle");

    bool is_main = Picsom()->IsMainThread(), ret = false;

    // WriteLog(msg+"is_main="+ToStr(is_main));

    if (is_main && use_pthreads_detection)
      ret = SvmPredDetectionSingleBatch(idx, par, m, db, feats, augm,
					adseg, valp, namep,
					tolerate_missing, hasdata);
    else
      ret = SvmPredDetectionSingleSelf(idx, par, m, db, feats, augm,
				       adseg, valp, namep,
				       tolerate_missing, hasdata);

    Tac("SvmPredDetectionSingle");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool 
  DataBase::SvmPredDetectionSingleSelf(size_t idx,
				       const map<string,string>& parin,
				       const list<pair<string,string> >& m,
				       DataBase *db,
				       const list<string>& /*feats*/,
				       const string& augm,
				       Segmentation *adseg,
				       double *valp, string *namep,
				       bool tolerate_missing,
				       bool& hasdata) {
    string msg = "DataBase::SvmPredDetectionSingleSelf("+ToStr(idx)+") : ";
    bool debug = DebugDetections()>1, debug2 = DebugDetections()>2;

    AddDescription *ad = (AddDescription*)adseg;

    Tic("SvmPredDetectionSingleSelf");

    map<string,string> par = parin;

    string feat = par["features"], cls = par["class"];
    string inst = par["instance"], extra = par["extra"];

    string svmlib, svmopts, extraout;
    list<pair<string,string> > kv;
    bool incore = false;
    
    // obs! should be coordinated with DetectionName()
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="svm_library")
	svmlib = i->second;
      else if (i->first.find("svm_")==0) {
	kv.push_back(*i);
	if (i->first=="svm_homker" && i->second=="intersection") {
	  svmopts = "hkm-int";
	  map<string,string> mm(m.begin(), m.end());
	  svmopts += mm["svm_homkerorder"];
	}

	if (i->first=="svm_homkerorder") {}

	if (i->first=="svm_kernel_type")
	  svmopts = i->second;

      } else if (i->first=="extraout") {
	extraout = i->second; // obs! should be used in detname???

      } else if (i->first=="incore") {
	incore = IsAffirmative(i->second);

      } else
	return ShowError(msg+"parameter <"+i->first+">=<"+i->second+
			 "> not understood");

    if (svmlib=="")
      return ShowError(msg+"svm_library should be given");

    string instsep = inst!="" ? "§" : "";

    if (!db->HasSVM(svmlib, feat, svmopts, extra, cls, inst)) {
      kv.push_back(make_pair("${feat}",       feat));
      kv.push_back(make_pair("${cname}",      cls));
      kv.push_back(make_pair("${inst}",       inst));
      kv.push_back(make_pair("${instsep}",    instsep));
      // kv.push_back(make_pair("${restr}",      "~*"));  // obs! commented out 2015-05-11
      kv.push_back(make_pair("${svmlibrary}", svmlib));
      kv.push_back(make_pair("${svmopts}",    svmopts));
      kv.push_back(make_pair("${svmextra}",   extra));
      kv.push_back(make_pair("verbose",       "0"));

      if (!db->SvmCommon(kv))
	return ShowError(msg+"create step failed");
    }

    string params; // obs!
    SVM *svm = db->GetSVM(svmlib, feat, svmopts, extra, cls, inst, params); 
    while (!svm->IsReady()) {
      struct timespec ts = { 0, 1000000 }; // 1 millisecond
      nanosleep(&ts, NULL);
    }

    if (debug2)
      svm->ShowAllNames();

    // string vecidxname = svm->IndexName(), vecidxnameorig = vecidxname;
    // size_t pp = vecidxname.find("::");
    // string prefix = pp!=string::npos ? vecidxname.substr(0, pp) : "";
    // if (prefix!="svm" && prefix!="linear")
    //   return ShowError(msg+"svm:: or linear:: prefix not found in <"+
    // 		       vecidxnameorig+">");
    // vecidxname.erase(0, pp+2);
    // size_t z = vecidxname.find("#");
    // if (z==string::npos)
    //   return ShowError(msg+"# separator not found in <"+vecidxnameorig+">");
    // vecidxname.erase(z);
    // z = vecidxname.find("-hkm-int");
    // if (z!=string::npos)
    //   vecidxname.erase(z);

    string vecidxnameorig = svm->IndexName();
    size_t z = vecidxnameorig.find("::");
    string prefix = vecidxnameorig.substr(0, z);
    string vecidxnamebase = SVMfeaturename(vecidxnameorig);
    string vecidxnameaugm = vecidxnamebase+augm;

    if (prefix!="svm" && prefix!="linear")
      return ShowError(msg+"svm:: or linear:: prefix not found in <"+
     		       vecidxnameorig+">");

    // Index *baseidx = FindIndexInternal(vecidxname, true);
    Index *baseidx = FindIndex(vecidxnameaugm, "", true, false);
    if (!baseidx)
      return ShowError(msg+"failed to find index <"+vecidxnameaugm+">");
    if (baseidx->IsDummy())
      baseidx = FindIndex(vecidxnameaugm, "", true, false);
    VectorIndex *vecidx = dynamic_cast<VectorIndex*>(baseidx);
    if (!vecidx)
      return ShowError(msg+"index <"+vecidxnameaugm+"> is not VectorIndex but "
		       +baseidx->MethodName());

    bool delete_v = false;
    const FloatVector *v = vecidx->DataVector(idx);
    if (!v) {
      if (SqlFeatures())
	v = SqlFeatureData(vecidxnameaugm, idx, true);
      else {
	vector<size_t> idxv { idx };
	FloatVectorSet fvs = vecidx->DataByIndices(idxv, tolerate_missing);
	if (fvs.Nitems()) {
	  v = fvs.Get(0);
	  fvs.Relinquish(0);
	  // v->Dump(DumpLong);
	}
      }
      if (v)
	delete_v = true;
    }

    if (!v) {
      hasdata = false;
      string err = msg+"failed to get data vector for #"+ToStr(idx)+
	" <"+Label(idx)+"> of <"+vecidxnameaugm+">";

      if (tolerate_missing)
	WriteLog(err+" will have zero value");
      else
	return ShowError(err+" interrupting");;
    }

    int frno = atoi(par["frameno"].c_str());

    string predict_str = "no";
    int n = 0;
    double val = v ? svm->PredictProb(*v, predict_str, n) : 0.0;

    if (delete_v)
      delete v;

    if (debug) {
      ostringstream ss;
      ss << msg << "idx=" << idx << " label=" << Label(idx)
	 << " frame#=" << frno
	 << " prefix=" << prefix
	 << " incore=" << incore
	 << " feat=" << feat
	 << " class=" << cls
	 << " instance=" << inst
	 << " val=" << val;
      if (svm->HasPDFmodel())
	ss << " cor=" << svm->PDFmodelCorProb(val)
	   << " prec=" << svm->PDFmodelPrecision(val)
	   << " rec=" << svm->PDFmodelRecall(val);
      WriteLog(ss);
    }

    if (ad) {
      string rname  = "svmpred";
      string rtype  = "detection";
      string rvalue = cls+"="+ToStr(val);
      SegmentationResult sres(rname, rtype, rvalue);
      ad->addPendingAnnotation(sres, frno, false, false);
    }

    string detname = vecidxnameorig;
    // if (augm!="") {
    //   size_t p = detname.find("::#");
    //   if (p==string::npos)
    // 	return ShowError(msg+"augm=<"+augm+"> but \"::#\" not found in <"
    // 			 +vecidxnameorig+">");
    //   detname.insert(p+2, augm);
    // }
    if (augm!="") {
      size_t p = detname.rfind("#");
      if (p==string::npos)
    	return ShowError(msg+"augm=<"+augm+"> but \"#\" not found in <"
    			 +vecidxnameorig+">");
      detname.insert(p, augm);
    }

    if (valp)
      *valp = val;
    if (namep)
      *namep = detname;

    if (!valp && !StoreDetectionResult(idx, detname, val, incore))
      return ShowError(msg+"failed to store detection data");

    Tac("SvmPredDetectionSingleSelf");

    Picsom()->PossiblyShowDebugInformation("After SVM detection");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool 
  DataBase::SvmPredDetectionSingleBatch(size_t idx,
					const map<string,string>& parin,
					const list<pair<string,string> >&,
					DataBase*, const list<string>& feats,
					const string& augm,
					Segmentation *adseg,
					double *valp, string*, 
					bool tolerate_missing,
					bool& hasdata) {
    string msg = "DataBase::SvmPredDetectionSingleBatch("+ToStr(idx)+") : ";

    hasdata = false;

    if (adseg)
      return ShowError(msg+"cannot handle adseg!=NULL");

    if (valp)
      return ShowError(msg+"cannot handle valp!=NULL");

    if (augm!="")
      return ShowError(msg+"cannot handle augm=<"+augm+">");

    string featseltxt;
    if (feats.size())
      featseltxt = "features="+CommaJoin(feats);

    map<string,string> par = parin;
    list<string> script;
    script.push_back("analyse=detect");
    script.push_back("debugtrap=yes");
    script.push_back("database="+Name());
    script.push_back("detections="+par["name"]);
    script.push_back("class="+par["class"]);
    if (featseltxt!="")
      script.push_back(featseltxt);
    script.push_back("tolerate_missing_features="+
		     string(tolerate_missing?"yes":"no"));
    script.push_back("args=#"+ToStr(idx));

    vector<string> argv;
    script_list_e entry = make_pair("svmpred", make_pair(script, argv));
    script_list_t list;
    list.push_back(entry);

    return LaunchAsyncAnalysisDeprecated(list);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::LscDetectionSingle(size_t idx,
				    const map<string,string>& parin,
				    const list<pair<string,string> >& m,
				    DataBase* /*db*/,
				    const list<string>& featl,
				    const string& /*augm*/,
				    Segmentation* /*adseg*/,
				    double *valp, string *namep) {
    string msg = "DataBase::LscDetectionSingle("+ToStr(idx)+") : ";

    bool debug1 = DebugDetections()>0, debug2 = DebugDetections()>1;

    Tic("LscDetectionSingle");

    set<string> featset(featl.begin(), featl.end());
    map<string,string> par = parin;
    string feat = par["features"], clsx = par["class"], inst = par["instance"];
    string extra = par["extra"], posneg;

    size_t lsc_k = 1, n_max = 0, n_0 = 0;
    lsc::convex_t convex = lsc::none, convex_def = lsc::by_value;

    bool sum_break = true; 

    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="k") {
	lsc_k = atoi(i->second.c_str());
	if (i->second.find('+')!=string::npos)
	  convex = convex_def;

      } else if (i->first=="n_max")
	n_max = atoi(i->second.c_str());

      else if (i->first=="posneg")
	posneg = i->second;;

    if (posneg!="" && posneg!="lin" && posneg!="log" && posneg!="exp" &&
	posneg!="rat" && posneg!="chi" )
      return ShowError(msg+"posneg != lin/log/exp/rat/chi");

    size_t n_lsc = posneg=="" ? 1 : 2;

    int myrandseed = 0;

    VectorIndex *featvidx = NULL;

    string lscname = "lsc::"+feat+"::"+ToStr(lsc_k)+
      (convex==lsc::none?"":"+")+"-"+ToStr(n_max)+posneg+"::"+extra+"#"+clsx;
 
    if (lscset.find(lscname)==lscset.end()) {
      lscset[lscname] = make_pair(lsc(lsc_k), lsc(lsc_k));
      lsc& lscpos = lscset[lscname].first;
      lsc& lscneg = lscset[lscname].second;

      for (size_t l=0; l<n_lsc; l++) {
	lsc&  lscx = l==0 ? lscpos     : lscneg;
	string pnl = l==0 ? "positive" : "negative";
	string pns = l==0 ? "pos"      : "neg";
	size_t n_tot = 0, n_zero = 0, n_dup = 0;
	string clseff = string("$exp(")+clsx+"-2012-"+pns+",1)"; // obs! obs!
	// clseff = clsx;

	lscx.convex(convex);
	lscx.sum_break(sum_break);

	ground_truth gt = GroundTruthExpression(clseff);
	vector<size_t> idxiir = gt.indices(1), idxiip;
	if (n_max==0 || idxiir.size()<=n_max)
	  idxiip = idxiir;
	else {
	  IntVector rp = RandVar::Permutation(idxiir.size(), myrandseed);
	  // int maxj = n_max ? n_max : 0;
	  // if (maxj==0 || maxj>rp.Length())
	  //   maxj = rp.Length();
	  int maxj = rp.Length();
	  for (int j=0; j<maxj; j++)
	    idxiip.push_back(idxiir[rp[j]]);
	}

	size_t n_max_eff = n_max ? n_max : idxiip.size();
	if (l==1)
	  n_max_eff = n_0;

	set<vector<float> > vset;
	for (size_t i=0; i<idxiip.size() && n_tot<n_max_eff; i++) {
	  const FloatVector *v = FeatureData(feat, idxiip[i], false);
	  bool do_del = v;
	  if (!v) {
	    if (!featvidx) {
	      Index *featidx = FindIndex(feat, "", true);
	      featvidx = dynamic_cast<VectorIndex*>(featidx);
	      if (!featvidx)
		return ShowError(msg+"feature <"+feat+"> not found");
	    }
	    // featvidx->ReadDataFile(false, false, true);

	    v = featvidx->DataVector(idxiip[i]);
	  }
	  if (!v)
	    return ShowError(msg+"feature <"+feat+"> for #"+ToStr(idxiip[i])+
			     " not found");

	  vector<float> vx(v->Length());
	  memcpy(&vx[0], (float*)*v, vx.size()*sizeof(float));
	  if (do_del)
	    delete v;

	  bool is_zero = true;
	  for (auto j=vx.begin(); is_zero && j!=vx.end(); j++)
	    is_zero = *j==0;

	  if (is_zero) {
	    n_zero++;
	    continue;
	  }

	  if (vset.find(vx)!=vset.end()) {
	    n_dup++;
	    continue;
	  }
	  vset.insert(vx);

	  labeled<vector<float> > lv(vx, lscname);
	  lscx.add(lv);
	  n_tot++;
	}

	if (l==0)
	  n_0 = n_tot;

	if (debug1)
	  WriteLog(msg+"Created "+pnl+" LSC \""+lscname+"\" with "+
		   ToStr(n_tot)+" vectors of total "+ToStr(idxiir.size())+
		   " (randseed="+ToStr(myrandseed)+", "+ToStr(n_zero)+
		   " zero and "+ToStr(n_dup)+
		   " duplicate vectors skipped) and k="+ToStr(lsc_k)+
		   " convex="+lsc::convex_name(convex)+
		   " "+lscx.description());
      }
    }

    const FloatVector *v = FeatureData(feat, idx, false);
    bool do_del = v;
    if (!v) {
      Index *featidx = FindIndex(feat);
      VectorIndex *featvidx = dynamic_cast<VectorIndex*>(featidx);
      if (!featvidx)
	return ShowError(msg+"feature <"+feat+"> not found");
      v = featvidx->DataVector(idx);
    }

    if (!v) {
      return ShowError(msg+"failed to get <"+feat+
		       "> feature vector for #"+ToStr(idx));

    } else {
      float *vx = *v;
      labeled<vector<float> > vv;
      vv.insert(vv.end(), vx, vx+v->Length());
      if (do_del)
	delete v;

      pair<lsc,lsc>& lscpn = lscset[lscname];
      double maxd = numeric_limits<double>::max(), dp = maxd, dn = maxd;
      if (n_lsc)
	dp = lscpn.first.class_distance(lscname, vv, 0.0);
      if (n_lsc>1)
	dn = lscpn.second.class_distance(lscname, vv, 0.0);
      double val = 0;
      if (n_lsc==1 && dp!=maxd)
	val = 1/(dp+1);
      if (n_lsc==2) {
	if (dp==0 && dn==0)
	  val = 0.5;
	else if (dp==0)
	  val = 1;
	else if (dn==0)
	  val = 0;
	else {
	  double diff = dn-dp; // posneg=="lin"
	  if (posneg=="log")
	    diff = log(dn)-log(dp);
	  if (posneg=="exp")
	    diff = exp(dn)-exp(dp);
	  if (posneg=="rat")
	    diff /= dn+dp;
	  if (posneg=="chi")
	    diff = diff*diff/(dn+dp);

	  val = (1+tanh(diff))/2;
	}
      }

      if (debug2)
	WriteLog(msg+ToStr(idx)+" "+feat+" "+clsx+" "+ToStr(dp)+" "+ToStr(dn)
		 +" "+ToStr(val));

      if (valp)
	*valp = val;
      if (namep)
	*namep = lscname;

      bool incore = false;
      if (!valp && !StoreDetectionResult(idx, lscname, val, incore))
	return ShowError(msg+"failed to store detection data");
    }

    Tac("LscDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::RandomDetectionSingle(size_t idx,
				       const map<string,string>& parin,
				       const list<pair<string,string> >& m,
				       DataBase* /*db*/,
				       const list<string>& /*featl*/,
				       const string& /*augm*/,
				       Segmentation* /*adseg*/,
				       double *valp, string* /*namep*/) {
    string msg = "DataBase::RandomDetectionSingle("+ToStr(idx)+") : ";

    bool debug = DebugDetections()>0;

    Tic("RandomDetectionSingle");

    map<string,string> par = parin;
    string clsx = par["class"], extra = par["extra"], rnd = "42";

    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first=="rndseed")
	rnd = i->second;

    int rndseed = atoi(rnd.c_str());

    RandVar randvar1(rndseed);
    int newseed = randvar1.RandomInt(Size())+idx;
    for (size_t i=0; i<clsx.size(); i++)
      newseed += (int)clsx[i]*(1+i);
    for (size_t i=0; i<extra.size(); i++)
      newseed += (int)extra[i]*(1+i);
    RandVar randvar2(newseed);
    double val = randvar2.Erand();
    val = pow(val, 4.0);

    if (debug)
      WriteLog(msg+ToStr(idx)+" "+clsx+" "+ToStr(val)+" ("+ToStr(rndseed)+")");

    string outname = "random::"+extra+"#"+clsx;

    bool incore = false;
    if (!valp && !StoreDetectionResult(idx, outname, val, incore))
      return ShowError(msg+"failed to store detection data");

    Tac("RandomDetectionSingle");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
  bool DataBase::CaffeDetectionSingle(size_t idx,
				      const map<string,string>& parin,
				      const list<pair<string,string> >& /*m*/,
				      DataBase* db,
				      const list<string>& /*featl*/,
				      const string& /*augm*/,
				      Segmentation* /*adseg*/,
				      double *valp, string* /*namep*/,
				      XmlDom& xml) {
    string msg = "DataBase::CaffeDetectionSingle("+ToStr(idx)+") : ";

    bool debug = DebugDetections()>0;

    Tic("CaffeDetectionSingle");

    map<string,string> par = parin;
    string extra = par["extra"], name = par["name"];

    vector<size_t> idxv { idx };
    list<vector<float> > det = RunCaffe(db, name, idxv);
    if (det.size()!=1)
      return ShowError(msg+"det.size()!=1");

    const vector<float>& val = det.front();
    if (debug) {
      string vstr;
      if (val.size()<5)
	vstr = ToStr(val);
      else
	vstr = ToStr(vector<float>(&val[0], &val[5]))+" ...";
      WriteLog(msg+ToStr(idx)+" "+name+" ("+extra+") "+vstr);
    }

    string outname = "caffe::"+name;
    if (extra!="")
      outname += "::"+extra;
    if (!valp && !StoreDetectionResult(idx, outname, val, xml, false))
      return ShowError(msg+"failed to store detection data");

    Tac("CaffeDetectionSingle");

    return true;
  }
#else
  bool DataBase::CaffeDetectionSingle(size_t idx,
				      const map<string,string>&,
				      const list<pair<string,string> >&,
				      DataBase*,
				      const list<string>&,
				      const string&,
				      Segmentation*,
				      double *, string*,
				      XmlDom&) {
    string msg = "DataBase::CaffeDetectionSingle("+ToStr(idx)+") : ";
    return ShowError(msg+"HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE not defined");
  }
#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE

  /////////////////////////////////////////////////////////////////////////////

  float DataBase::ThresholdValue(const string& thrfilein) {
    string msg = "DataBase::ThresholdValue() : ";

    string thrfile = thrfilein;
    if (thrfile[0]!='.' && thrfile[0]!='/')
      thrfile = ExpandPath("thresholds")+"/"+thrfile;
    if (!FileExists(thrfile)) {
      ShowError(msg+"threshold file <"+thrfile+"> does not exist");
      return -1;
    }
    float thr = -123;
    ifstream ithr(thrfile);
    while (true) {
      string line;
      getline(ithr, line);
      if (!ithr)
	break;
      if (line[0]=='-' || (line[0]>='0' && line[0]<='9'))
	thr = atof(line.c_str());
    }
    if (thr==-123) {
      ShowError(msg+"no threshold value was found in file <"+thrfile+">");
      return -1;
    }
    if (thr<0 || thr>1) {
      return ShowError(msg+"threshold value "+ToStr(thr)+" in <"+thrfile+
			 "> is out of bounds");
      return -1;
    }
    return thr;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::LaunchAsyncAnalysisDeprecated(const script_list_t& list) {
    vector<string> a;

    if (!analysis_async_deprecated)
      analysis_async_deprecated = new Analysis(Picsom(), NULL, NULL, a);

    script_list_t clist = list;
    analysis_async_deprecated->Analyse(clist, false, false);
    // analysis_async->AnalysePthread(list, false);
    // analysis_async->ShowThreadDataInfo();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SvmCommon(const list<pair<string,string> >& kvin) {
    string msg = "DataBase::SvmCommon() : ";

    // string filename =
    //   "${svmlibrary}::${feat}${svmextra}#${cname}${instsep}${inst}";

    string filename =
      "${svmlibrary}::${feat}::${svmopts}::${svmextra}#"
      "${cname}${instsep}${inst}";

    list<pair<string,string> > kv;
    kv.push_back(make_pair("analyse",  "create"));
    kv.push_back(make_pair("database", Name()));
    kv.push_back(make_pair("target",   "image"));  // obs! hard-coded

    kv.insert(kv.end(), kvin.begin(), kvin.end());

    kv.push_back(make_pair("features",            "${feat}"));
    // kv.push_back(make_pair("dbrestrictionsilent", "${restr}")); // obs! commented out 2015-05-11
    kv.push_back(make_pair("class",        	  "${cname}"));
    kv.push_back(make_pair("instance",     	  "${inst}"));
    kv.push_back(make_pair("filename",     	  filename));

    kv.push_back(make_pair("svm",            	  "${svmlibrary}"));
    kv.push_back(make_pair("svm_options",     	  "${svmopts}"));
    kv.push_back(make_pair("svm_extra",     	  "${svmextra}"));
    kv.push_back(make_pair("svm_trainmodel", 	  "false"));
    kv.push_back(make_pair("svm_predict",    	  "no"));
    kv.push_back(make_pair("svm_use_cache",  	  "false"));

    kv.push_back(make_pair("positive",   	  "${cname}"));
    // the following was commented out 2014-06-20 as it is not always correct
    //kv.push_back(make_pair("negative",   	  "${cname}-neg")); // obs!

    vector<string> argv;

    return RunAnalysis(kv, argv);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::RunAnalysis(const list<pair<string,string> >& kv,
			     const vector<string>& argv) {
    list<string> script;
    for (list<pair<string,string> >::const_iterator i=kv.begin();
	 i!=kv.end(); i++)
      script.push_back(i->first+"="+i->second);
    
    return RunAnalysis(script, argv);
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::RunAnalysis(const list<string>& script,
			     const vector<string>& argv) {
    bool batch = false; // use_pthreads_detection;

    return batch ? RunAnalysisBatch(script, argv) :
      RunAnalysisSelf(script, argv);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::RunAnalysisSelf(const list<string>& script,
				 const vector<string>& argv) {
    Tic("RunAnalysisSelf");

    Analysis analysis(Picsom(), NULL, NULL, argv);
    analysis.Script(script);
    analysis.ScriptExecuteAndShow();
    bool ret = !analysis.Analyse().errored();

    Tac("RunAnalysisSelf");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::RunAnalysisBatch(const list<string>& script,
				  const vector<string>& argv) {

    script_list_e entry = make_pair("dbanalysis", make_pair(script, argv));
    script_list_t list;
    list.push_back(entry);

    return LaunchAsyncAnalysisDeprecated(list);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::HasSVM(const string& pre, const string& f, const string& o,
			const string& e, const string& cls,
			const string& inst) const {
    if (svms.empty())
      return false;

    string id = svms.begin()->second->Identifier(pre, f, o, e, cls, inst);

    map<string,SVM*>::const_iterator i=svms.find(id);
    return i!=svms.end();
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SVMfeaturename(const string& id) {
    size_t a = id.find("::"), b = id.find('#');
    if (a==string::npos || b==string::npos || b<a)
      return ShowErrorS("Failed to split SVM featurename from <"+id+">");

    string tmp = id.substr(a+2, b-a-2);
    a = tmp.find("::");
    if (a!=string::npos)
      tmp.erase(a);    

    return tmp;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SVMoptions(const string& id) {
    size_t a = id.find("::"), b = id.find('#');
    if (a==string::npos || b==string::npos || b<a)
      return ShowErrorS("Failed to split SVM specification from <"+id+">");

    string tmp = id.substr(a+2, b-a-2);
    a = tmp.find("::");
    if (a==string::npos)
      return "";
    
    tmp.erase(0, a+2);
    a = tmp.find("::");
    return tmp.substr(0, a);    
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SVMextra(const string& id) {
    size_t a = id.find("::"), b = id.find('#');
    if (a==string::npos || b==string::npos || b<a)
      return ShowErrorS("Failed to split SVM specification from <"+id+">");

    string tmp = id.substr(a+2, b-a-2);
    a = tmp.find("::");
    if (a==string::npos)
      return "";
    
    tmp.erase(0, a+2);
    a = tmp.find("::");
    return a!=string::npos ? tmp.substr(a+2) : "";    
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SVMclass(const string& id) {
    size_t a = id.find("::"), b = id.find('#');
    if (a==string::npos || b==string::npos || b<a)
      return ShowErrorS("Failed to split SVM specification from <"+id+">");

    return id.substr(b+1);
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM *DataBase::GetSVM(const string& pre, const string& f, const string& o,
			const string& e, const string& cls, const string& inst, 
			const string& params) {
    string msg = "DataBase::GetSVM("+pre+","+f+","+o+","+e+","+cls+","+inst+
      ") : ";
    string id = pre+Index::MethodSeparator()+f+"::"+o+"::"+e+"#"+cls;
    if (inst!="")
      id += "§"+inst;

    return GetSVM(id, params);
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM *DataBase::GetSVM(const string& id, const string& params) {
    string f = SVMfeaturename(id);
    string msg = "DataBase::GetSVM("+id+","+params+")["+f+"] : ";
    bool debug = SVM::DebugSVM();

    RwLockWrite("GetSVM");

    Tic("GetSVM");
    
    SVM *ret = NULL;

    map<string,SVM*>::iterator i = svms.find(id);
    if (i!=svms.end()) {
      if (debug)
	cout << TimeStamp() << " " << ThreadIdentifierUtil() << " "
	     << msg << "FOUND in MEMORY and REUSED" << endl;
      ret = i->second;
    }

    if (!ret) {
      if (debug)
	cout << TimeStamp() << " " << ThreadIdentifierUtil() << " "
	     << msg << "NOT FOUND and will be CONSTRUCTED to MEMORY" << endl;

      const Index *src = NULL;  // was always until 2013-10-25
      src = FindIndexInternal(f, true);
      ret = new SVM(this, id, f, "", params, src);

      if (!ret->IsFailed()) {
	index.push_back(ret);
	svms[id] = ret;
      } else {
	delete ret;
	ret = NULL;
      }
    }

    Tac("GetSVM");

    RwUnlockWrite("GetSVM");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlInsertDetectionData(size_t idx, const string& namein,
					double val) {
    string msg = "DataBase::SqlInsertDetectionData("+ToStr(idx)+","
      +namein+","+ToStr(val)+") : ";

    if (!UseSql() || SqlMode()!=2)
	return ShowError(msg+"SQL not opened for writing");

    if (DebugSql())
      cout << TimeStamp() << msg << "starting" << endl;

    string name = SqlEscape(namein, true, true);

    MysqlLock();

    string cmd, tname = "detections";
    if (!SqlTableExists(tname)) {
      string indexzspec = "INTEGER PRIMARY KEY";
      string spec = "indexz "+indexzspec+", "+name+" FLOAT";
      if (!SqlCreateTable(tname, spec))
	return ShowError(msg+"SqlCreateTable() failed");
    }

#ifdef HAVE_SQLITE3_H
    cmd = "SELECT "+name+" FROM "+tname+";";
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, false);
      if (stmt)
	sqlite3_finalize(stmt);
      else {
	cmd = "ALTER TABLE "+tname+" ADD COLUMN "+name+" FLOAT";
	if (!SqlExec(cmd, true))
	  return ShowError(msg+"SqlExec() failed with <"+cmd+">");
      }
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      string tnamex = MysqlFindTableColumn(tname, name);
      if (tnamex=="") {
	tnamex = MysqlAddTableColumn(tname, name, "FLOAT");
	if (tnamex=="")
	  return ShowError(msg+"failed fo find/create column <"+name+">");
      }
      tname = tnamex;
    }
#endif // HAVE_MYSQL_H

    cmd = "INSERT INTO "+tname+" ( indexz ) VALUES ( "+ToStr(idx)+" )";
    SqlExec(cmd, false);

    cmd = "UPDATE "+tname+" SET "+name+"="
      +ToStr(val)+" WHERE indexz="+ToStr(idx);
    if (!SqlExec(cmd, true)) {
      return ShowError(msg+"SqlExec() failed with <"+cmd+">");
    }
    
    MysqlUnlock();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  map<string,vector<float> > 
  DataBase::RetrieveOrProduceDetectionData(size_t idx, const string& spec,
					   const string& clsname,
					   bool angry, bool& exists,
					   bool allow_incore) {
    // string msg = "DataBase::RetrieveOrProduceDetectionData("+ToStr(idx)
    //   +","+spec+","+clsname+") : ";

    bool tolerate_missing = false;  // this should be an argument

    if (spec.find("fusion(")==0 || spec.find("incore(")==0)
      return ProduceDetectionData(idx, spec, clsname, tolerate_missing);

    if (allow_incore) {
      size_t p = spec.find(",");
      if (p==string::npos) { // added 2016-04-20
	string f = ExpandPath("detections", spec+".bin");
	p = f.find("%c");
	if (p!=string::npos)
	  f.replace(p, 2, clsname);
	
	if (!FileExists(f)) 
	  return ProduceDetectionData(idx, "incore("+spec+")", clsname,
				      tolerate_missing);
      }
    }

    // it _would_ be better to always have clsname defined and no #xxx in spec...
    // but instance information comes _after_ class...  
    string specx = spec;
    if (clsname!="") {
      size_t p = specx.rfind("#%c");
      if (/*p==specx.size()-3 &&*/ p!=string::npos && p==specx.find("#%c")) {
	// p==specx.size()-3 condition removed because can be e.g. #%c§s1
	specx.replace(p+1, 2, clsname); 
      } else {
	p = specx.rfind("#");
	if (p==string::npos || specx.find("(),/", p+1)!=string::npos)
	  specx += "#"+clsname;
      }
    }

    return RetrieveDetectionData(idx, specx, angry, exists);
  }

  /////////////////////////////////////////////////////////////////////////////
  
  map<string,vector<float> > 
  DataBase::ProduceDetectionData(size_t idx, const string& spec,
				 const string& cname, bool tolerate_missing) {
    string msg = "DataBase::ProduceDetectionData("+ToStr(idx)
      +","+spec+","+cname+") : ";

    bool debug = debug_detections>3;

    map<string,vector<float> > empty, ret;

    bool is_fusion = spec.find("fusion(")==0;
    bool is_incore = spec.find("incore(")==0;

    if (!is_fusion && !is_incore) {
      ShowError(msg+"only fusion() and incore() supported");
      return empty;
    }

    if (cname=="") {
      ShowError(msg+"cname argument empty");
      return empty;
    }

    if (debug)
      cout << TimeStamp() << msg << "starting" << endl;

    string ssx = spec;
    size_t p = ssx.find('(');
    ssx.erase(0, p+1);
    p = ssx.rfind(')');
    if (p!=ssx.size()-1) {
      ShowError(msg+"spec \""+spec+"\" doesn't end with ')'");
      return empty;
    }
    ssx.erase(p);

    list<pair<string,string> > l {
      make_pair("name",   spec),
      make_pair("class",  cname),
      make_pair("incore", "true")
    };

    if (is_fusion) {
      l.push_back(make_pair("type", "fusion"));
	
      vector<string> a = SplitInCommasObeyParentheses(ssx);
      string dets;
      for (size_t i=0; i<a.size(); i++)
	if (a[i][0]=='/') {
	  try {
	    l.push_back(SplitKeyEqualValueNew(a[i].substr(1)));
	  } catch (const logic_error& em) {
	    ShowError(msg+em.what());
	    return empty;
	  }
	} else 
	  dets += (dets==""?"":",")+a[i];
      l.push_back(make_pair("detectors", dets));
    }

    // until 2016-03-02 ss==ssx for fusion() too...
    string ss = is_incore ? ssx : spec; 

    if (is_incore) {
      string sstmp = ss;
      if (described_detections.find(sstmp)==described_detections.end()) {
	size_t p = sstmp.find("#%c");
	if (p==sstmp.size()-3) {
	  sstmp.erase(p);
	  if (described_detections.find(sstmp)==described_detections.end()) {
	    ShowError(msg+"detection with name <"+ss+"> not found");
	    return empty;
	  }
	}
      }
      auto dd = described_detections[sstmp];
      l.insert(l.end(), dd.begin(), dd.end());
    }

    pair<string,list<pair<string,string> > > descr { make_pair(spec, l) };

    string instance;
    list<string> feats;
    string augm;
    bool hasdata = false;
    XmlDom xml;

    if (!DoOneDetection(true, idx, descr, cname, instance, feats, augm,
			NULL, tolerate_missing, hasdata, xml)) {
      ShowError(msg+"DoOneDetection() failed");
      return empty;
    }

    string detname = ss;
    p = detname.find("#");
    if (p==string::npos)
      detname += "#%c";
    for (;;) {
      p = detname.find("#%c");
      if (p==string::npos)
	p = detname.find("#%C");
      if (p==string::npos)
	break;
      detname.replace(p+1, 2, cname);
    }

    auto i = bindetections.find(detname);
    if (i==bindetections.end()) {
      stringstream tmpss;
      FindObject(idx)->dump_nonl(tmpss);
      tmpss << " available bindetections:";
      for (auto bb=bindetections.begin(); bb!=bindetections.end(); bb++)
	tmpss << " \"" << bb->first << "\"";
      ShowError(msg+"detection data <"+detname+"> not found for "+tmpss.str());
      return empty;
    }

    vector<float> v = i->second.get_float(idx);
    ret.insert(make_pair(detname, v));

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  map<string,vector<float> > 
  DataBase::RetrieveDetectionData(size_t idx, const string& spec,
				  bool angry, bool& exists) {
    string msg = "DataBase::RetrieveDetectionData("+ToStr(idx)
      +","+spec+") : ";

    if (storedetections.find("bin")!=string::npos)
      return BinRetrieveDetectionData(idx, spec, angry, exists);

    map<string,vector<float> > ret;

    if (storedetections.find("sql")!=string::npos) {
      map<string,float> v = SqlRetrieveDetectionData(idx, spec);
      for (auto i=v.begin(); i!=v.end(); i++) {
	vector<float> vv { i->second };
	ret[i->first] = vv;
      }
      return ret;
    }

    ShowError(msg+"storedetections should contain either sql or bin");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  map<string,float> DataBase::SqlRetrieveDetectionData(size_t idx,
						       const string& spec) {
    string msg = "DataBase::SqlRetrieveDetectionData("+ToStr(idx)
      +","+spec+") : ";
    map<string,float> res;

    Tic("SqlRetrieveDetectionData");

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      string cmd = "SELECT * FROM detections WHERE indexz="+ToStr(idx);
      if (spec!="")
	cmd += " AND xxxxx obs!";
      cmd += ";";
      if (DebugSql())
	cout << cmd << endl;

      sqlite3_stmt *stmt = SqlitePrepare(cmd, false);
      if (!stmt)
	return res;

      int rrr = sqlite3_step(stmt);
      
      if (rrr==SQLITE_ROW) {
	if (DebugSql())
	  SqliteShowRow(stmt);

	int nc = sqlite3_column_count(stmt);
	for (int i=1; i<nc; i++)
	  if (sqlite3_column_type(stmt, i)==SQLITE_FLOAT) {
	    string nr = sqlite3_column_name(stmt, i);
	    string nc = SqlEscape(nr, false, false);
	    float v = sqlite3_column_double(stmt, i);
	    res[nc] = v;
	  }
      }
      rrr = sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H
    
#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      if (spec!="") {
	ShowError(msg+"MysqlMode() and spec non-empty");
	SimpleAbort();
      }

      string cmd1 = "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
	"WHERE table_name='detections' AND table_schema='"+mysql_dbname+
	"';";

      if (DebugSql())
	cout << cmd1 << endl;

      MYSQL_STMT *stmt = MysqlPrepare(cmd1, false);
      if (!stmt)
	return res;

      if (mysql_stmt_execute(stmt)) {
	ShowError(msg+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      char name[100] = "";
      unsigned long name_length = 0;
      MYSQL_BIND bind1[1];
      memset(bind1, 0, sizeof(bind1));
      bind1[0].buffer_type   = MYSQL_TYPE_STRING;
      bind1[0].buffer        = name;
      bind1[0].buffer_length = sizeof(name);
      bind1[0].length        = &name_length;

      if (mysql_stmt_bind_result(stmt, bind1)) {
	ShowError(msg+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      vector<string> cols;

      bool ok = true;
      for (; ok;) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(msg+"data truncated");
	  else
	    ShowError(msg+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	string cname(name, name_length);
	if (cname=="indexz")
	  continue;

	cols.push_back(cname);
      }
      MysqlStmtClose(stmt);

      string cmd2 = "SELECT "+CommaJoin(cols)
	+ " FROM "+mysql_dbname+".detections WHERE indexz="+ToStr(idx)+";";

      if (DebugSql())
	cout << cmd2 << endl;

      stmt = MysqlPrepare(cmd2, false);
      if (!stmt)
	return res;

      if (mysql_stmt_execute(stmt)) {
	ShowError(msg+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      vector<float> val(cols.size());
      MYSQL_BIND *bind = new MYSQL_BIND[cols.size()];
      for (size_t i=0; i<cols.size(); i++) {
	memset(bind+i, 0, sizeof(MYSQL_BIND));
	bind[i].buffer_type   = MYSQL_TYPE_FLOAT;
	bind[i].buffer        = &val[i];
	bind[i].buffer_length = sizeof(float);
      }

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(msg+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      for (; ok;) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(msg+"data truncated");
	  else
	    ShowError(msg+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}
	
	for (size_t i=0; i<cols.size(); i++) {
	  string nc = SqlEscape(cols[i], false, false);
	  res[nc] = val[i];
	}

	break;
      }

      delete bind;

      MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H

    Tac("SqlRetrieveDetectionData");

    return res;    
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DescribedMedia(const XmlDom& node) {
    string msg = "DataBase::DescribedMedia() : ";
 
    bool debug = false;

    string use = node.Property("use");
    if (use!="" && !IsAffirmative(use))
      return true;

    string desname = node.Property("name");
    if (desname=="")
      return ShowError(msg+"name should be defined");

    string type = node.Property("type");
    if (type=="")
      return ShowError(msg+"type should be defined");

    if (described_medias.find(name)!=described_medias.end())
      return ShowError(msg+"media extraction with name <"+name+
		       "> already exists");

    string dbname;

    list<pair<string,string> > l, prop = node.Properties();
    for (auto p=prop.begin(); p!=prop.end(); p++) {
      string name = p->first, val = p->second;

      if (name=="use")
	continue;

      if (debug) 
	cout << TimeStamp() << msg << desname << " : <" << name << ">=<"
	     << val << ">" << endl;

      l.push_back(make_pair(name, val));
    }

    described_medias[desname] = l;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::MostAuthoritativeDataFile(int idx, Index *ts, bool most) {
    string label = Label(idx);
    string fname = LabelFileNamePart(label);
    if (fname.empty())
      ShowError("LabelFileNamePart("+label+") FAILED!");

    const string& name = ts->Name();
    string nstr = FeatureOutputName(name);
    if (nstr.empty()) nstr = name;

    string fn = nstr+".dat";

    vector<size_t> v = ObjectDirectoryComponents(label);

    string dir = LeafDirectory(v)+"/features", datfile = "";

    if (most) {
      if (DirectoryExists(dir)) {
	// try leafdir/features/00000001:22-rgb.dat
	datfile = FileIfOKgz(label+"-"+fn, dir);

	// try leafdir/features/00000001-rgb.dat
	if (datfile.empty() && label != fname) 
	  datfile = FileIfOKgz(fname+"-"+fn, dir);

	// try leafdir/features/rgb.dat
	if (datfile.empty())
	  datfile = FileIfOKgz(fn, dir);
      }

      // finally try features/rgb.dat
      if (datfile.empty()) datfile = ts->FindFile(nstr, ".dat", true);
    } else { 
      // looking for LEAST authorative, ie reverse order

      // try features/rgb.dat
      datfile = ts->FindFile(nstr, ".dat", true);

      if (datfile.empty() && DirectoryExists(dir)) {
	// try leafdir/features/rgb.dat
	datfile = FileIfOKgz(fn, dir);

	// try leafdir/features/00000001-rgb.dat
	if (datfile.empty() && label != fname) 
	  datfile = FileIfOKgz(fname+"-"+fn, dir);

	// try leafdir/features/00000001:1-rgb.dat
	if (datfile.empty()) datfile = FileIfOKgz(label+"-"+fn, dir);
      }
    }

    return datfile;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::IsOrphanFileName(const string& sin) const {
  if (sin.find(path)!=0)
    return false;

  string s = sin.substr(path.size());
  if (s[0]!='/' || s.find(objdir)!=1)
    return false;

  size_t ls = 0;
  for (size_t i=0; i+1<labelformat.size(); i++)
    ls += labelformat[i]+1;

  const string fs = "/features/";
  s.erase(0, objdir.size()+1);
  if (s.find(fs)!=ls)
    return false;

  s.erase(0, ls+fs.size());
  size_t p = s.find_first_not_of("0123456789");
  if (p!=LabelLength() || (s[p]!=':' && s[p]!='-'))
    return false;

  size_t l = s.size();
  if (s.substr(l-4)!=".dat" && s.substr(l-7)!=".dat.gz")
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

string DataBase::OrphanToLeafFileName(const string& sin) const {
  string msg = "DataBase::OrphanToLeafFileName("+sin+") : ", s = sin;

  size_t l = s.size();
  if (s.substr(l-4)!=".dat")
    return ShowErrorS(msg+"problem 1");

  l = s.rfind("/");
  if (l<8 || s.substr(l-8, 8)!="features")
    return ShowErrorS(msg+"problem 2");

  size_t p = s.find_first_not_of("0123456789", l+1);
  if (p!=l+1+LabelLength() || (s[p]!=':' && s[p]!='-'))
    return ""; // ShowErrorS(msg+"problem 3");

  if (s[p]==':')
    p = s.find('-', p);

  if (p==string::npos)
    return ShowErrorS(msg+"problem 4");

  s.erase(l+1, p-l);

  return s;
}

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::Restriction(const string& s, bool verbose) {
    target_type tt = target_any_target; // obs! this is now hard-coded
    int other      = -1;                // obs! this is now hard-coded
    bool expand    = true;              // obs! this is now hard-coded

    RestrictionInner(restriction, "", s, verbose, tt, other, expand);
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::RestrictionInner(ground_truth& gt, const string& n,
				  const string& s, bool verbose,
				  target_type tt, int other, bool expand) {
    RwLockWrite("Restriction");
    if (s=="")
      RemoveRestrictionInner(gt, n);
    else {
      gt = GroundTruthExpression(s, tt, other, expand);
      if (verbose) {
	string gts  = GroundTruthSummaryStr(gt, true, false, false);
	WriteLog("Restriction "+(n!=""?n+" ":"")+"set to: "+gts);
      }
    }
    RwUnlockWrite("Restriction");
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::CreateRestrictionVectors(const ground_truth& restr, 
					  vector<vector<bool> >&
					  dir_rest) const {
    bool debug = debug_leafdir>2;

    dir_rest.clear();
    size_t l = labelformat.size();
    if (l<2)
      return;

    if (debug)
      cout << "RESTR SIZE l=" << l << " ";
    for (size_t i=0; i+1<l; i++) {
      size_t m = ObjectDirectoryWidth(i, true);
      dir_rest.push_back(vector<bool>(m));
      if (debug)
	cout << " i=" << i << " m=" << m << " ";
    }
    if (debug)
      cout << endl;

    for (size_t i=0; i<restr.size(); i++) 
      if (restr[i]==1) {
	const string lab = LabelFileNamePart(Label(i));
   
	size_t y = atol(lab.c_str());
	if (debug)
	  cout << "RESTR " << i << "/" << restr.size() << " <" << Label(i)
	       << "> <" << lab << "> y=" << y;

	for (size_t j=l-2;; j--) {
	  size_t m = ObjectDirectoryWidth(j+1, false), x = y/m;
	  dir_rest[j][x] = true;

	  if (debug)
	    cout << " / j=" << j << " m=" << m << " x=" << x;

	  y = x;
	  if (!j)
	    break;
	}

	if (debug)
	  cout << endl;
      }
  }

///////////////////////////////////////////////////////////////////////////////

//void DataBase::CreateRestrictionVectorsOld(const ground_truth& restr, 
//					   vector<bool>& d1_rest,
//					   vector<bool>& d2_rest) const {
//  for (size_t i=0; i<restr.size(); i++) {
//    if (restr[i]==1) {
//      const string& lab = LabelFileNamePart(Label(i));
//   
//      size_t d1 = atoi(lab.substr(0,2).c_str());
//      size_t d2 = atoi(lab.substr(0,4).c_str());
//   
//      d1_rest[d1] = true;
//      d2_rest[d2] = true;
//   
//      //	  cout << "RESTR: " << lab << " " << d1 << " / " << d2 << endl; 
//    }
//  }
//}

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::LeafDirectory(const vector<size_t>& v) const {
    string ret = path+"/"+objdir, zero = "0000000000";
    for (size_t i=0; i<v.size() && i<labelformat.size(); i++) {
      string s = zero+ToStr(v[i]);
      ret += "/"+s.substr(s.size()-labelformat[i]);
    }
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<size_t> DataBase::ObjectDirectoryComponents(const string& label) {
    string fname = LabelFileNamePart(label);
    vector<size_t> v;
    if (fname=="") {
      ShowError("DataBase::ObjectDirectoryComponents("+label+") : "
		"LabelFileNamePart() returned empty string");
      return v;
    }
    for (size_t i=0, j=0; i+1<labelformat.size(); i++) {
      v.push_back(atoi(fname.substr(j, labelformat[i]).c_str()));
      j += labelformat[i];
    }
    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::LeafDirectoryFileListInner(list<string>& ret,
					    const vector<size_t>& v,
					    const string& subdir,
					    const string& fn,
					    bool first_only,
					    bool all,
					    const ground_truth& restr,
					    const vector<vector<bool> >&
					    dir_restr) const {
 
    string msg = "LeafDirectoryFileListInner() : ";

    size_t l = v.size(), rwas = ret.size();
   
    if (l+1>labelformat.size())
      return ShowError(msg+"l>=labelformat.size()");

    if (l+1==labelformat.size()) {
      string leaf = LeafDirectory(v)+"/"+subdir;
      if (!DirectoryOK(leaf, debug_leafdir>1))
	return false;
	  
      string leaffn = leaf+"/"+fn;
      leaffn = FileIfOKgz(leaffn, "", debug_leafdir>1);

      if (leaffn!="" && FileSize(leaffn)<=0)
	leaffn = "";

      if (leaffn!="") {
	ret.push_back(leaffn);
	if (debug_leafdir)
	  cout << TimeStamp() << msg << "<" << ShortFileName(leaffn) << "> A n="
	       << ret.size() << endl;
	if (first_only)
	  return true;
      }

      if (!all || !HasAnyOrphans(leaf, fn))
	return false;

      vector<int> objs = AllObjectsIn(v);
      for (size_t i=0; i<objs.size(); i++) {
	if (debug_leafdir>1)
	  cout << "i=" << i << " objs[i]=" << objs[i] << " \"" << Label(objs[i])
	       << "\" restr[objs[i]]=" << int(restr[objs[i]]) << endl;
	
	leaffn = leaf+"/"+Label(objs[i])+"-"+fn;
	leaffn = FileIfOKgz(leaffn, "", debug_leafdir>1);
	
	if (leaffn!="" && FileSize(leaffn)<=0)
	  leaffn = "";

	// mats: 8.6.2006
	// do not add orphans if they are not in RestrictionGT()
	      
	if (leaffn!="" && (restr.empty() || restr[objs[i]]==1)) {
	  ret.push_back(leaffn);
	  if (debug_leafdir)
	    cout << TimeStamp() << msg << "<" << ShortFileName(leaffn) << "> B "
		 << i << "/" << objs.size()
		 << " n=" << ret.size() << endl;
	  if (first_only)
	    return true;
	  continue;
	}
      }

      return rwas!=ret.size();
    }

    vector<size_t> w = v;
    w.push_back(0);

    size_t a = 0;
    for (size_t b=0; b<l; b++) {
      a += v[b];
      a *= ObjectDirectoryWidth(b+1, false);
    }

    size_t m = ObjectDirectoryWidth(l, false);
    for (size_t d=0; d<m; d++, a++) {
      if (!restr.empty() && !dir_restr[l][a])
	continue;

      w[l] = d;
      string dir = LeafDirectory(w);
      bool exists = DirectoryOK(dir, debug_leafdir>1);
      if (!exists)
	continue;

      bool r = LeafDirectoryFileListInner(ret, w, subdir, fn,
					  first_only, all, restr, dir_restr);
      if (r && first_only)
	return true;
    }

    return rwas!=ret.size();
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::LeafDirectoryFileList(const string& subdir,
					       const string& fn,
					       bool first_only,
					       bool all) const {
    if (debug_leafdir)
      cout << "LeafDirectoryFileList(" << subdir << "," << fn << ","
	   << first_only << "," << all << ")" << endl;

    ground_truth restr = RestrictionGT();
    vector<vector<bool> > dir_restr;
    CreateRestrictionVectors(restr, dir_restr);

    vector<size_t> v;
    list<string> ret;

    LeafDirectoryFileListInner(ret, v, subdir, fn, first_only, all,
			       restr, dir_restr);

    return ret;
  }

///////////////////////////////////////////////////////////////////////////////

//list<string> DataBase::LeafDirectoryFileListOld(const string& subdir,
//						const string& fn,
//						bool first_only,
//						bool all) const {
//  if (debug_leafdir)
//    cout << "LeafDirectoryFileList(" << subdir << "," << fn << ","
//	 << first_only << "," << all << ")" << endl;
//
//  bool ignore_empty = true;  // used to be like false...
//
//  list<string> ret;
//
//  ground_truth restr = RestrictionGT();
//  vector<bool> d1_rest(100);
//  vector<bool> d2_rest(10000);
//
//  CreateRestrictionVectorsOld(restr, d1_rest, d2_rest);
//
//  for (int d1=0; d1<100; d1++) {
//    if (!restr.empty() && !d1_rest[d1]) continue;
//
//    string dir = LeafDirectoryOld(d1);
//    bool exists = DirectoryExists(dir);
//    if (debug_leafdir)
//      cout << " <" << dir << ">\t" << exists << endl;
//
//    if (exists)
//      for (int d2=0; d2<100; d2++) {
//	if (!restr.empty() && !d2_rest[d1*100+d2]) continue;
//
//	dir = LeafDirectoryOld(d1, d2);
//	exists = DirectoryExists(dir);
//	if (debug_leafdir)
//	  cout << " <" << dir << ">\t" << exists << endl;
//
//	if (exists)
//	  for (int d3=0; d3<100; d3++) {
//	    string leaf = LeafDirectoryOld(d1, d2, d3);
//	    
//	    if (!DirectoryOK(leaf,debug_leafdir)) continue;
//
//	    leaf += string("/")+subdir;
//	    if (!DirectoryOK(leaf,debug_leafdir)) continue;
//
//	    string leaffn = leaf+"/"+fn;
//	    leaffn = FileIfOKgz(leaffn, "", debug_leafdir);
//
//	    if (leaffn!="" && ignore_empty && FileSize(leaffn)<=0)
//	      leaffn = "";
//
//	    if (leaffn!="") {
//	      ret.push_back(leaffn);
//	      if (first_only)
//		return ret;
//	      continue;
//	    }
//
//	    if (!all)
//	      continue;
//	    
//	    if (!HasAnyOrphans(leaf, fn))
//	      continue;
//
//	    vector<int> objs = AllObjectsInOld(d1, d2, d3);
//	    for (size_t i=0; i<objs.size(); i++) {
//	      leaffn = leaf+"/"+Label(objs[i])+"-"+fn;
//	      leaffn = FileIfOKgz(leaffn, "", debug_leafdir);
//
//	      if (leaffn!="" && ignore_empty && FileSize(leaffn)<=0)
//		leaffn = "";
//
//	      // mats: 8.6.2006
//	      // do not add orphans if they are not in RestrictionGT()
//	      
//	      if (leaffn!="" && (restr.empty() || restr[objs[i]]==1)) {
//		ret.push_back(leaffn);
//		if (first_only)
//		  return ret;
//		continue;
//	      }
//	
//	    }
//	  }
//      }
//  }
//  return ret;
//}

  /////////////////////////////////////////////////////////////////////////////

//  vector<int> DataBase::AllObjectsInOld(int d1, int d2, int d3) const {
//    char dir[100];
//    sprintf(dir, "%02d%02d%02d", d1, d2, d3);
//
//    vector<int> ret;
//    for (int i=0; i<100; i++) {
//      char par[100];
//      sprintf(par, "%s%02d", dir, i);
//      int idx = LabelIndexGentle(par);
//      if (idx>=0) {
//	ret.push_back(idx);
//	const object_info *oi = FindObject(idx);
//	for (vector<int>::const_iterator c = oi->children.begin();
//	     c!=oi->children.end(); c++) {
//	  if (Label(*c).find(dir)==string::npos)
//	    continue;
//
//	  if (ObjectsTargetTypeContains(idx, target_video) &&
//	      ObjectsTargetTypeContains(*c,  target_image)) {
//	    string frame = Label(*c);
//	    size_t col = frame.find(':');
//	    if (col!=string::npos) {
//	      frame.erase(0, col+1);
//	      if (frame.find_first_not_of("0123456789")==string::npos)
//		continue;
//	    }
//	  }
//
//	  ret.push_back(*c);
//	}
//      }
//    }
//
//    return ret;
//  }

  /////////////////////////////////////////////////////////////////////////////

  vector<int> DataBase::AllObjectsIn(size_t d) const {
    size_t l = labelformat.size();
    if (l<2)
      return vector<int>();

    vector<size_t> v(l-1);    

    for (size_t j=l-2;; j--) {
      size_t m = ObjectDirectoryWidth(j, false), x = d/m, y = d%m;
      v[j] = y;
      d = x;
      if (!j)
	break;
    }

    return AllObjectsIn(v);
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<int> DataBase::AllObjectsIn(const vector<size_t>& v) const {
    string msg = "AllObjectsIn() : ";

    if (v.size()!=labelformat.size()-1) {
      ShowError(msg+"failed 1");
      return vector<int>();
    }

    string dir;
    for (size_t i=0; i<v.size(); i++) {
      string s = "000000000000"+ToStr(v[i]);
      dir += s.substr(s.size()-labelformat[i]);
    }

    vector<int> ret;

    size_t l = labelformat[labelformat.size()-1];
    size_t n = ObjectDirectoryWidth(labelformat.size()-1, false);

    if (n<Size()) 
      for (int i=0; i<(int)n; i++) {
	string s = "000000000000"+ToStr(i);
	string par = dir+s.substr(s.size()-l);
	int idx = LabelIndexGentle(par);
	if (idx>=0)
	  AllObjectsInInner(idx, dir, ret);
      }

    else 
      for (int i=0; i<(int)Size(); i++) {
	if (Label(i).substr(0, dir.size())==dir)
	  AllObjectsInInner(i, dir, ret);
      }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AllObjectsInInner(int idx, const string& dir,
				   vector<int>& ret) const {
    ret.push_back(idx);
    const object_info *oi = FindObject(idx);
    for (vector<int>::const_iterator c = oi->children.begin();
	 c!=oi->children.end(); c++) {
      if (Label(*c).find(dir)==string::npos)
	continue;

      if (ObjectsTargetTypeContains(idx, target_video) &&
	  ObjectsTargetTypeContains(*c,  target_image)) {
	string frame = Label(*c);
	size_t col = frame.find(':');
	if (col!=string::npos) {
	  frame.erase(0, col+1);
	  if (frame.find_first_not_of("0123456789")==string::npos)
	    continue;
	}
      }

      ret.push_back(*c);
    }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::HasAnyOrphans(const string& leaf, const string& fn) const {
  bool found = false;

  const string xx = "-"+fn;

#ifdef HAVE_DIRENT_H
  DIR *d = opendir(leaf.c_str());
  for (dirent *dp; !found && d && (dp = readdir(d)); ) {
    string s = dp->d_name;
    if (s.find(xx)!=string::npos)
      found = true;
  }
  if (d)
    closedir(d);
#endif // HAVE_DIRENT_H

  return found;
}

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::FindAllClassNames(bool force, bool also_contents,
					   bool also_meta) {
    //
    // Should here be a modification time check too?
    //
    
    // this cache doesn't yet handle different combinations of contents&meta!
    if (!force && !class_name_cache.empty())
      return class_name_cache;

    bool expand = true;
    if (also_contents)
      ReadContents(false, expand);
    // obs! class names from contents are not included...

    list<string> ret;

    string dir = ExpandPath("classes");
#ifdef HAVE_DIRENT_H
    DIR *d = opendir(dir.c_str());  
    for (dirent *dp; d && (dp = readdir(d)); ) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
	continue;

#ifndef __MINGW32__
      if (dp->d_type==DT_DIR)
	continue;
#endif // __MINGW32__

      // if (DirectoryExists(dir+"/"+dp->d_name))
      //   continue;
 
      static const char *ext[] = { "~", ".old", ".bak", ".orig", NULL };

      bool skip = false, has_digits = false;
      size_t l = strlen(dp->d_name);
      for (int i=0; !skip && ext[i]; i++)
	if (!strcmp(ext[i], dp->d_name+l-strlen(ext[i])))
	  skip = true;

      for (size_t j=l-1; !skip && j>0; j--)
	if (isdigit(dp->d_name[j]))
	  has_digits = true;
	else if (dp->d_name[j]=='.' && has_digits)
	  skip = true;
	else
	  break;
 
      if (skip)
	continue;

      if (!also_meta && IsMetaClassFile(dp->d_name)) // they may contain garbage
	continue;

      ret.push_back(dp->d_name);
    }
    if (d)
      closedir(d);
#endif // HAVE_DIRENT_H

    class_name_cache = ret;

    return ret;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::FindAllClasses(bool force, bool also_contents) {
  //
  // There should be a modification time check here
  //
  if (DebugGroundTruth())
    cout << "Now in DataBase::FindAllClasses(" << force << ","
         << also_contents << ") " << "allcontentsclasses=<"
         << allcontentsclasses << "> all_classes_found_plain="
         << all_classes_found_plain << " all_classes_found_contents="
         << all_classes_found_contents << endl;

  if (!force && ((all_classes_found_plain && !also_contents) ||
      all_classes_found_contents))
    return true;

  list<string> allowed;
  if (allcontentsclasses!="")
    allowed = SplitClassNames(allcontentsclasses);

  set<string> allowed_not_found(allowed.begin(), allowed.end());

  bool expand = true;
  if (also_contents)
    ReadContents(false, expand);

  list<string> clist = FindAllClassNames(false, also_contents, false);

  size_t nclasses_bef = classfiles.size();

  for (list<string>::const_iterator i=clist.begin(); i!=clist.end(); i++) {
    if (!allowed.empty()) {
      bool found = false;
      for (list<string>::const_iterator a=allowed.begin();
	   !found && a!=allowed.end(); a++)
	found = *a==*i;
      if (!found)
	continue;

      allowed_not_found.erase(*i);
    }

    ConditionallyReadClassFile(*i, expand);
  }

  size_t nclasses_aft = classfiles.size();
  if (nclasses_bef!=nclasses_aft)
    WriteLog("Read "+ToStr(nclasses_aft-nclasses_bef)+" new class file"+
             string(nclasses_aft-nclasses_bef==1?"":"s"));

  for (set<string>::const_iterator a=allowed_not_found.begin();
       a!=allowed_not_found.end(); a++)
    if (a->find_first_of("()*^|&\\")==string::npos)
      ShowError("Allowed class <", *a, "> not found");

  // WriteLog("Scanned class files in ", dir);

  all_classes_found_plain    = true;
  all_classes_found_contents = also_contents;

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::ConditionallyReadClassFile(const string& cls,
						    bool expand, bool force) {
    RwLockWrite("ConditionallyReadClassFile");

    bool read_really = true;

    RootlessDownloadClassFile(cls, true);
    string f = ExpandPath("classes", cls);

    classfiles_t::iterator i = classfiles.find(f);
    if (!force && i!=classfiles.end() && !i->second.Modified())
      read_really = false;

    if (read_really) {
      ReallyReadClassFile(cls, expand);
      if (i==classfiles.end())
	i = AddClassFileEntry(f);
      i->second.WasRead();
    }

    const ground_truth *gt = ContentsFind(cls);
    ground_truth ret;
    if (gt)
      ret = *gt;

    RwUnlockWrite("ConditionallyReadClassFile");

    if (!gt) {
      ShowError("DataBase::ConditionallyReadClassFile("+cls+") failed");
      return ground_truth();
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> DataBase::SolveClassFileNames(const string& label) const {
    string file = ExpandPath("classes", label), compiled;

    if (!FileExists(file)) {
      const string longclasspath =  "classes/" + label.substr(0, 1);
      file = ExpandPath(longclasspath, label);
    }

    if (!FileExists(file))
      file = label;

    if (FileExists(file) && file[0]=='/') {
      compiled = ExpandPath("classes-compiled", label, true);

      if (!FileExists(compiled)) {
	const string longclasspath =  "classes-compiled/" + label.substr(0, 1);
	compiled = ExpandPath(longclasspath, label,true);
      }
    }

    return make_pair(file, compiled);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::ReallyReadClassFile(const string& label, bool expand) {
    string msg = "DataBase::ReallyReadClassFile("+label+") : ";
    
    pair<string,string> file_comp = SolveClassFileNames(label);

    const string& file = file_comp.first, comp = file_comp.second;

    if (FileExists(comp) && !FileExists(file))
      throw msg+"comp exist, file doesn't";

    bool do_compile = comp!="", use_compiled = !force_compile;
    do_compile   = do_compile   && FileExists(file);
    use_compiled = use_compiled && MoreRecent(comp, file);
    use_compiled = use_compiled && MoreRecent(comp, sqlite_mod_time);
    if (!UseSql()) {
      use_compiled = use_compiled && MoreRecent(comp, ExpandPath(labels));
      string subob = ExpandPath("subobjects");
      if (FileExists(subob))
	use_compiled = use_compiled && MoreRecent(comp, subob);
    }

    if (use_compiled) {
      list<string> deps = CompiledClassFileDepends(comp);
      for (list<string>::const_iterator dep = deps.begin();
	   use_compiled && dep!=deps.end(); dep++)
	use_compiled = use_compiled && MoreRecent(comp, *dep);
    }

    if (use_compiled)
      do_compile = false;

    msg += "["+file+"] : ";

    if (debug_gt)
      cout << TimeStamp() << msg << " [" << comp << "] use_compiled="
	   << use_compiled << " do_compile=" << do_compile << endl;

    ifstream classfile(file.c_str());
    if (!classfile)
      return ground_truth();

    Tic("ReallyReadClassFile");

    int gsize = Size();

    ground_truth correct(gsize);
    correct.expandable(expand);
    correct.depends(file);
    bool multi_valued = false, question_found = false, first_line = true;

    if (use_compiled) {
      ReadCompiledClassFile(correct, comp);
      goto ready;
    }

    for (;;) {
      string line;
      getline(classfile, line);
      if (!classfile)
	break;

      while (line.size() && line[line.size()-1]=='\r')
	line.erase(line.size()-1);

      if (line[0]=='#' || line.find_first_not_of(" \t")==string::npos) {
	if (line.find("# TERNARY")==0) {
	  multi_valued = true;
	  if (debug_gt)
	    cout << TimeStamp() << msg << " found keyword TERNARY" << endl ;
	} else if (IsClassDescription(line))
	  correct.text(line.substr(2));
	continue;
      }
      if (line[0]=='=') {
	if (first_line) {
	  // here it is possible to end up in an infinite recursion...
	  string txt = correct.text();
	  correct = GroundTruthExpression(line.substr(1), target_any_target,
					  -1, expand);
	  correct.depends(file);
	  correct.text(txt);
	  goto ready;
	}
	else
	  ShowError(msg+" = construct not on the first line.");
      }

      first_line = false;

      int val = line[0]=='-' ? -1 : line[0]=='?' ? 0 : 1;

      if (val!=1)
	multi_valued = true;
      if (val==0 && !question_found) {
	question_found = true;
	correct.setImplicitEquals(0); // mark all zeros up til now implicit
      }
 
      const char *l = line.c_str()+(val!=1 ? 1 : 0);
      const char *dots = strstr(l, " .. ");
      if (!dots) {
	string ll = l;
	size_t p = ll.find(' ');  // if ordered file or otherwise contains
	if (p!=string::npos)      // "label value" type lines
	  ll.erase(p);            // should we issue a message here?

	xymap_ve xyvec;
	int idx = LabelIndexGentleWithXY(ll, xyvec);
	if (idx>=0 && idx<gsize) {
	  correct[idx] = val;
	  correct.setImplicit(idx,false);

	  if (!xyvec.empty()) {
	    InsertImageXYpoints(label, idx, xyvec);
	    do_compile = false;
	    if (debug_gt)
	      cout << TimeStamp() << msg
		   << " !xyvec.empty() => do_compile = false" << endl;
	  }
	}

	else {
	  // here it is possible to end up in an infinite recursion...
	  ground_truth subset = GroundTruthExpression(l, target_any_target, -1,
						      expand);
	  if (subset.size()==0) {
	    ShowError(msg+"<", l, "> not understood as label nor subclass.");
	    return ground_truth();
	  }
	  if (subset.indices(0).size()!=0)
	    multi_valued = true;
	  if (debug_gt) {
	    cout << "result before TernaryOR : ";
	    GroundTruthSummaryLine(correct);
	    cout << endl << "subset to be TernaryORed : ";
	    GroundTruthSummaryLine(subset);
	    cout << endl;
	  }
	  correct = correct.TernaryOR(subset);
	  if (debug_gt) {
	    cout << "result after TernaryOR : ";
	    GroundTruthSummaryLine(correct);
	    cout << endl;
	  }
	}

      } else {
	char first_str[100] = "", last_str[100] = "";
	if (sscanf(l, "%s .. %s", first_str, last_str)!=2) {
	  ShowError(msg+"parsing of <", l, "> failed.");
	  return ground_truth();
	}
	bool ok = false;
	if (*first_str=='#' && *last_str=='#')
	  ok = MarkIndexRange(first_str, last_str, correct, val);
	else if (LabelIndex(first_str)>=0 && LabelIndex(last_str)>=0)
	  ok = MarkLabelRange(first_str, last_str, correct, val);
	if (!ok) {
	  ShowError(msg+"interpreting <", l, "> failed.");
	  return ground_truth();
	}
      }
    }
    classfile.close();

    if (!multi_valued)
      correct.change(0, -1);

  ready:
    correct.label(label);
    // ReadContents();

    ContentsUpdateOrInsert(correct);

    Tac("ReallyReadClassFile");

    if (debug_classfiles) {
      char info[100000];
      sprintf(info, " : %d (%d,%d,%d,\"%s\")", (int)correct.size(),
	      (int)correct.NumberOfEqual(-1), (int)correct.NumberOfEqual(0),
	      (int)correct.NumberOfEqual(1), correct.text().c_str());
      WriteLog("Read class file <"+ShortFileName(use_compiled?comp:file)+">",
	       info);
    }

    if (debug_gt)
      GroundTruthSummaryTable(correct);

    // Picsom()->PossiblyShowDebugInformation("ReadClassFile() ending");

    if (do_compile)
      CompileClassFile(correct, comp);

    return correct;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CompileClassFile(const ground_truth& gt,
				  const string& f) {
    RwLockWrite("CompileClassFile");

    bool check = true;

    Picsom()->MakeDirectory(f, true);

    RenameToOld(f);

    ofstream os(f.c_str());
    os << "# COMPILED GROUND TRUTH FILE" << endl;
    if (gt.text()!="") {
      string txt = gt.text();
      for (; txt!="" ;) {
	size_t p = txt.find('\n');
	if (p==string::npos) {
	  os << "# " << txt << endl;
	  break;
	}
	os << "# " << txt.substr(0, p) << endl;;
	txt = txt.substr(p+1);
      }
    }
    const set<string>& dep = gt.depends();
    for (set<string>::const_iterator i=dep.begin(); i!=dep.end(); i++)
      os << "#dep " << *i << endl;

    os << gt.size() << endl;
    os << gt.pack();
    os.close();

    bool ok = true;
    if (!os)
      ok = ShowError("Failed to write compiled class file <"+f+">");
    else if (debug_classfiles)
      WriteLog("Wrote compiled class file <"+ShortFileName(f)+">");

    if (debug_gt && ok)
      cout << "CompileClassFile() successfully COMPILED "
	   << "compiled class file <"+f+">" << endl;

    if (ok && check) {
      ground_truth tmp;
      ReadCompiledClassFile(tmp, f);
      if (tmp.size()!=gt.size() && false) // simultaneous SOAP insertions...
	ok = ShowError("CompileClassFile() check failed A");
      for (size_t i=0; ok && i<tmp.size() && i<gt.size(); i++)
	if (tmp[i]!=gt[i])
	  ok = ShowError("CompileClassFile() check failed B");

      if (ok && debug_gt)
	cout << "CompileClassFile() successfully CHECKED "
	     << "compiled class file <"+f+">" << endl;
    }

    RwUnlockWrite("CompileClassFile");

    return ok;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ReadCompiledClassFile(ground_truth& gt, const string& f) const {
  string err = "Failed to read compiled class file <"+f+"> ";

  Tic("ReadCompiledClassFile");

  ifstream is(f.c_str());

  bool first = true;
  for (;;) {
    string line;
    getline(is, line);
    if (first && line.find("# COMPILED GROUND TRUTH FILE")!=0)
      return ShowError(err+"A");
    if (first) {
      first = false;
      continue;
    }

    if (IsClassDescription(line))
      gt.text(line.substr(2));

    if (line.find("#dep ")==0 && line.size()>5)
      gt.depends(line.substr(5));

    if (line[0]=='#' || line.find_first_not_of(" \t")==string::npos)
      continue;

    int l = 0;
    l = atoi(line.c_str());
    if (l<=0)
      return ShowError(err+"B");

    size_t ll = size_t(l);
    string buf(ll, 'z');
    is.read((char*)buf.c_str(), buf.size());
    if (!is) {
      stringstream ss;
      ss << " l=" << l << " gcount=" << is.gcount();
      return ShowError(err+"C"+ss.str());
    }

    gt.unpack(buf);

    break;
  }

  if (gt.size()>(size_t)Size())
    return ShowError(err+"size too large : "+
		     ToStr(gt.size())+">"+ToStr(Size()));

  string lenmsg;
  if (gt.size()<(size_t)Size()) {
    bool has_zeros = false;
    for (size_t i=0; i<gt.size() && !has_zeros; i++)
      if (gt[i]==0)
	has_zeros = true;
    signed char val = has_zeros ? 0 : -1;
    lenmsg = " lengthened with "+ToStr(Size()-gt.size())+" x "+ToStr(int(val));
    gt.resize(Size(), val);

    // WarnOnce("Lengthened compiled ground truth <"+f+">");
  }

  if (debug_gt)
    cout << "ReadCompiledClassFile() successfully read "
	 << "compiled class file <"+f+">"+lenmsg << endl;

  Tac("ReadCompiledClassFile");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

list<string> DataBase::CompiledClassFileDepends(const string& f) const {
  string err = "Failed to read compiled class file <"+f+">";

  list<string> ret;

  ifstream is(f.c_str());
  if (!is) {
    ShowError(err);
    return ret;
  }

  for (;;) {
    string line;
    getline(is, line);

    if (line[0]!='#')
      break;

    if (line.find("#dep ")==0 && line.size()>5)
      ret.push_back(line.substr(5));
  }

  return ret;
}

  /////////////////////////////////////////////////////////////////////////////

  int DataBase::LabelIndexGentleWithXY(const string& str,
				       xymap_ve& xys) const {
    bool debug = !false;

    xys.clear();

    size_t ll = LabelLength();

    string d = "0123456789", s = " \t";
    if (str.size()>=ll+4 &&
	str.find_first_of(d)==0 && str.find_first_not_of(d)==ll &&    
	str.find(',', 1+ll)!=string::npos) {
      int idx = LabelIndexGentle(str.substr(0, ll));
      if (idx>=0) {
	if (debug)
	  cout << str << " -> [" << idx << "]";

	size_t p = ll;
	p = str.find_first_not_of(s, p);
	if (p!=string::npos && str.substr(p, 3)=="xy=")
	  p += 3;

	for (; p!=string::npos; p = str.find_first_of(s, p)) {
	  p = str.find_first_not_of(s, p);
	  if (p==string::npos)
	    break;

	  if (str[p]=='(')
	    p++;

	  int tx, ty;
	  if (sscanf(str.substr(p).c_str(), "%d,%d", &tx, &ty)==2) {
	    xys.push_back(xymap_e(tx, ty));
	    if (debug)
	      cout << str << " x=" << tx << ",y=" << ty;
	    p = str.find_first_not_of("0123456789,)", p);
	    if (p==string::npos)
	      break;

	  } else {
	    ShowError("LabelIndexGentleWithXY() failed with p="+ToStr(p),
		      " in ["+str+"]");
	    break;
	  }
	}

	if (debug)
	  cout << endl;

	return idx;
      }
    }

    return LabelIndexGentle(str);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsMetaClassFile(const string& cls) const {
    string msg = "DataBase::IsMetaClassFile("+cls+") : ";

    size_t p = cls.find('#');
    if (p!=string::npos) {
      string dbname = cls.substr(0, p), clspart = cls.substr(p+1);
      Picsom()->AddAllowedDataBase(dbname);
      DataBase *db = Picsom()->FindDataBase(dbname);
      return db ? db->IsMetaClassFile(clspart) :
	ShowError(msg+"database <"+dbname+"> not found");
    }

    RootlessDownloadClassFile(cls, false);

    string fname = ExpandPath("classes", cls);
    if (FileSize(fname)<1)
      return false;

    ifstream is(fname.c_str());
    string line;
    getline(is, line);

    if (!is)
      return ShowError(msg+"file <"+fname+"> could not be opened or read");
 
    return line.find("# METACLASSFILE")==0;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::SplitClassNames(const string& str) /*const*/ {
    string msg = "SplitClassNames("+str+") : ";
    if (debug_gt)
      cout << msg << "starting" << endl;

    size_t p = str.find('#');
    if (p!=string::npos) {
      string dbname = str.substr(0, p), clspart = str.substr(p+1);
      Picsom()->AddAllowedDataBase(dbname);
      DataBase *db = Picsom()->FindDataBase(dbname);
      if (!db) {
	ShowError(msg+"database <"+dbname+"> not found");
	return list<string>();
      }
      return db->SplitClassNames(clspart);
    }

    if (split_class_names_cache.find(str)!=split_class_names_cache.end()) {
      if (debug_gt)
	cout << msg << "found in cache" << endl;
      return split_class_names_cache[str];
    }
    
    string clstr = str;

    if (clstr.find("**")==0) {
      bool also_contents = clstr[2]=='c';
      FindAllClasses(false, also_contents);  // might end up in an infine loop
      list<string> l = KnownClassNames();
      split_class_names_cache[str] = l;
      return l;
    }

    // For supporting multiple classes like "face:known&&face:Matti&face:Teppo"
    // where the first is a meta class and the others are normal classes:
    // changed "&" to "&&" 2011-12-21
    vector<string> clstrv = SplitOnWord("&&", clstr);
    if (clstrv.size()>1) {
      if (debug_gt)
        cout << "SplitOnWord(\"&&\", false, " << clstr
             << ") resulted in clstr = " << clstrv[0] << endl;

      clstr = clstrv[0];
    }

    vector<string> xxx = SplitInCommasObeyParentheses(clstr);
    list<string> classes(xxx.begin(), xxx.end());

    if (debug_gt)
      cout << "SplitInCommasObeyParentheses(" << clstr << ") produced: "
           << JoinWithString(xxx, "|") << endl;

    if (classes.size()<2) {
      classes.clear();
      string fname = ExpandPath("classes", clstr);
      if (!FileExists(fname))
	fname = ExpandPath("classes/"+clstr.substr(0, 1), clstr);

      if (FileExists(fname)) {
	bool first = true;
	ifstream is(fname.c_str());
	while (is) {
	  string line;
	  getline(is, line);
	  if (first && line.find("# METACLASSFILE")!=0)
	    break;
	  first = false;
	  size_t i = line.find_first_not_of(" \t");
	  if (line.size()>0 && line.substr(0, 1)!="#" && i!=string::npos) {
	    line.erase(0, i);
	    i = line.find_last_not_of(" \t");
	    if (i!=string::npos)
	      line.erase(i+1);
	    // cout << "<" << line << ">" << endl;
	    classes.push_back(line);
	  }
	}
      }
    }

    if (clstrv.size()>1) {
      clstrv.erase(clstrv.begin());
      for (vector<string>::const_iterator i = clstrv.begin(); 
	   i!=clstrv.end(); i++)
	classes.push_back(*i);
    }

    if (classes.empty() && clstr!="")
      classes.push_back(clstr);

    if (debug_gt)
      cout << msg << "ending, classes=[" << CommaJoin(classes)
	   << "]" << endl;

    split_class_names_cache[str] = classes;
    
    return classes;
  }

///////////////////////////////////////////////////////////////////////////////

list<string> DataBase::KnownClassNames() const {
  list<string> classes;

  for (classfiles_t::const_iterator i = classfiles.begin();
       i!=classfiles.end(); i++) {
    string s = i->first;
    size_t x = s.rfind('/');
    classes.push_back(x!=string::npos ? s.substr(x+1) : s);
  }

  for (size_t i=0; i<ContentsClasses(); i++)
    classes.push_back(ContentsClassLabel(i));

  return classes;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MarkIndexRange(const string& first, const string& last,
				ground_truth& correct, int val) const {
    int first_idx = atoi(first.substr(1).c_str());
    int last_idx  = atoi( last.substr(1).c_str());

    if (!IndexOK(first_idx) || !IndexOK(last_idx) || first_idx>last_idx)
      return false;

    for (int li=first_idx; li<=last_idx; li++)
      correct[li] = val;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MarkLabelRange(const string& first, const string& last,
				ground_truth& correct, int val) const {
    int fidx = LabelIndex(first), lidx = LabelIndex(last);
    if (fidx<0 || lidx<0)
      return false;

    for (int li=fidx; li<=lidx; li++) {
      if (!IndexOK(li))
	return ShowError("DataBase::MarkLabelRange(): inexistent index #"+
			 ToStr(li)+">");
      correct[li] = val;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<size_t,string> >
  DataBase::ReadClassWithExtra(const string& label) const {
    string file = ExpandPath("classes", label);
    if (!FileExists(file))
      file = label;
    
    ifstream classfile(file);
    
    list<pair<size_t,string> > ret;
  
    for (;;) {
      string line, extra;
      getline(classfile, line);
      if (!classfile)
	break;
      
      if (line[0]=='#')
	continue;
    
      size_t p = line.find_first_of(" \t");
      if (p!=string::npos) {
	size_t q = line.find_first_not_of(" \t", p);
	if (q!=string::npos)
	  extra = line.substr(q);
	line.erase(p);
      }
      
      size_t idx = LabelIndex(line);
      ret.push_back(make_pair(idx, extra));
    }
    
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<size_t,pair<vector<size_t>,float> > >
  DataBase::ReadClassWithBoxesValues(const string& label) const {
    string msg = "ReadClassWithBoxesValues("+label+") : ";

    list<pair<size_t,string> > l = ReadClassWithExtra(label);

    list<pair<size_t,pair<vector<size_t>,float> > > ret, empty;
    for (auto i=l.begin(); i!=l.end(); i++) {
      vector<string> v = SplitInSpaces(i->second);
      if (v.size()<4 || v.size()>5) {
	ShowError(msg+"v.size()=="+ToStr(v.size())+" with <"
		  +Label(i->first));
	return empty;
      }
      float val = v.size()==5 ? atof(v[4].c_str())
	: numeric_limits<float>::quiet_NaN();
      vector<size_t> b(4);
      for (size_t j=0; j<4; j++)
	b[j] = atoi(v[j].c_str());

      ret.push_back(make_pair(i->first, make_pair(b, val)));
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ReadClassDescription(const string& label) const {
    string file = ExpandPath("classes", label);
    if (!FileExists(file))
      file = label;

    ifstream classfile(file);
    
    char line[1024];
    while (classfile.getline(line, sizeof line), classfile.good())
      if (IsClassDescription(line))
	return line+2;
    
    return "";
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ConditionallyWriteClassFile(bool cwd, const string& cname,
					   const ground_truth& set,
					   const string& commstr,
					   bool append, bool overwrite) const {
  string msg = "DataBase::ConditionallyWriteClassFile() : ";

  string file = cwd ? "./"+cname : ExpandPath("classes", cname);
  if (!overwrite && !cwd && FileExists(file))
    return ShowError(msg+"file <"+file +"> exists, not overwriting");

  return WriteClassFile(file, set, commstr, append);
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::WriteClassFile(const string& file,
                                const vector<string>& cls,
				const string& equation,
                                const ground_truth& set,
                                const string& commstr,
                                bool append, bool metaclass) const {
    int m = 0, z = 0, p = 0;
    set.TernaryCounts(m, z, p);
    stringstream istr;
    istr << m << "+" << z << "+" << p;
    if (metaclass)
      istr << " metaclass file with " << cls.size() << " entries";
    string info = istr.str();

    bool exists = FileExists(file);

    ios_base::openmode mode = ios_base::out;
    if (append)
      mode |= ios_base::app;

    ofstream classfile(file.c_str(), mode);

    WriteLog(append && classfile.tellp()!=streampos(0) ? 
             "Appending to":"Writing", " class file <",
	     ShortFileName(file), "> : "+info);

    if (metaclass && !exists)
      classfile << "# METACLASSFILE" << endl;

    if (commstr!="")
      for (size_t p=0; ; ) {
	size_t q = commstr.find('\n', p);
	string l = q==string::npos ? commstr.substr(p) : commstr.substr(p, q-p);
	// classfile << "#" << (l[0]=='#'?"":" ") << l << endl;
	classfile << (l[0]=='#'?"":"# ") << l << endl;
	if (q==string::npos)
	  break;
	p = q+1;
      }

    if (equation!="")
      classfile << equation << endl;

    for (size_t i=0; i<cls.size(); i++)
      classfile << cls[i] << endl;

    bool has_zeros = false;
    for (int i=0; i<set.Length() && !has_zeros; i++)
      if (set[i]==0)
        has_zeros = true;

    for (int i=0; i<set.Length(); i++)
      if (set[i]!=0) {
        if (has_zeros)
          classfile << (set[i]<0 ? "-" : "") << Label(i) << endl;
        else if (set[i]>0)
          classfile << Label(i) << endl;
      }

    if (!classfile.good())
      return ShowError("WriteClassFile() : Failed to write in file <",
                       file, ">");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::WriteOrderedClassFile(const list<pair<size_t,double> >& l,
				       const string& f, const string& txt,
				       bool val, bool append) const {
    bool exists = FileExists(f);

    ios_base::openmode mode = ios_base::out;
    if (append)
      mode |= ios_base::app;

    ofstream os(f.c_str(), mode);

    if (!append || !exists) {
      os << "# ORDERED CLASS FILE";
      if (l.size())
	os << " " << l.size() << " LABELS" << (val?" WITH SCORES":"");
      os << endl;
    }
    if (txt!="")
      os << (txt[0]=='#'?"":"# ") << txt << endl;

    for (list<pair<size_t,double> >::const_iterator i=l.begin();
	 i!=l.end(); i++) {
      os << Label(i->first);
      if (val)
	os << " " << i->second;
      os << endl;
    }

    if (!os)
      return ShowError("WriteOrderedClassFile() : failed to write in <"
		       +f+">");

    WriteLog("Wrote "+ToStr(l.size())+" labels in <"+f+">");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<size_t> DataBase::ReadOrderedClassFile(const string& label,
						int n, bool angry) const {
    pair<string,string> file_comp = SolveClassFileNames(label);
    const string& file = file_comp.first;

    vector<size_t> ret;

    ifstream is(file.c_str());
    if (!is) {
      if (angry)
	ShowError("Could not read ordered class file <"+file+">");
      return ret;
    }
 
    for (;;) {
      if (n>=0 && ret.size()>=size_t(n))
	break;

      string line;
      getline(is, line);
      if (!is)
	break;

      if (line[0]=='#')
	continue;
 
      int idx = LabelIndex(line);
      if (idx==-1)
	break;

      ret.push_back(idx);
    }

    return ret;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::MapReverseURLs(const ground_truth& gt, bool clear) {
  if (clear)
    url_to_index.clear();

  const string xxx = "/m/fs/imagedb/feature.search/devel/"; // trecvid2005

  bool warn = false, debug = false;
  int n_ok = 0;
  vector<size_t> idx = gt.indices(1);
  for (size_t i=0; i<idx.size(); i++) {
    const object_info *info = FindObject(idx[i]);
    if (!info)
      return ShowError("MapReverseURLs() : info==NULL");
    map<string,string> hash = ReadOriginsInfo(idx[i], false, warn);
    string url = hash["url"];
    if (url=="")
      continue;

    if (url.find(xxx)==0)
      url.erase(0, xxx.size());

    if (url[0]=='[' && url[url.size()-1]==']')
      url += hash["pageurl"];

    url_to_index[url] = idx[i];

    if (debug)
      cout << i << " " << idx[i] << " : " << ObjectDump(idx[i])
	   << " <- \"" << url << "\"" << endl;
    n_ok++;
  }

  if (n_ok || Size()==0)
    WriteLog("Successfully reversed URLs of "+ToStr(n_ok)+ " objects out"
	     " of "+ToStr(idx.size())+ " in <"+gt.Label()+">");

  Picsom()->PossiblyShowDebugInformation("After MapReverseURLs");

  return n_ok || Size()==0;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadURLBlacklist() {
    string fname = ExpandPath("url_blacklist");
    if (!FileExists(fname))
      return false;
    ifstream fp(fname.c_str());

    string line;
    while (getline(fp, line))
      url_blacklist.push_back(line);
    fp.close();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::URLIsBlacklisted(const string& url) const {
    for (size_t i=0; i<url_blacklist.size(); i++) {
      const string& bstr = url_blacklist[i];
      size_t blen = bstr.size();
      if (url.size() >= blen && url.substr(0,blen) == bstr)
        return true;
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef USE_MRML

bool DataBase::ReadOriginsIntoHash() {
  // reads "origins" file into memory and build a hashtable from it
  // Note: Parsing of "origins" is based on two "facts":
  // 1) filename and httppath contain no spaces
  // 2) between filename and httppath there is only one space.
  // This whole method should be rewritten when better parsing is needed.
  // cout << "DataBase::ReadOriginsIntoHash() entered" << endl;

  stringstream originsfile;
  originsfile << Path() << "/images/"+origins;
  ifstream from( originsfile.str().c_str() );
  if (!from) {
    ShowError("Cannot open input file origins!");
    return false;
  }

  do {
    char t[1000]="";
    char fname[100]="";
    char httppath[1000]="";

    from.getline(t,sizeof(t));
    if(!from)
      break;

    unsigned int i,j;
    for(i=0; i<sizeof(fname); i++) {
      if(t[i] == '.') break;
      fname[i] = t[i];
    }
    for(; i<sizeof(fname); i++) {
      if(t[i] == ' ') break;
    }      
 
    for(j=i+1; j<sizeof(fname); j++) {
      if(t[j] == ' ') break;
    }      

    strncpy(httppath, &t[i+1], (j-i)-1);

    originalfname[httppath]=fname;

    //cout << "Setting httppath= "<<httppath<<" & fname= "<<fname<<endl;

  } while( true );

  return true;
}

#endif //USE_MRML

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::LabelFormat(const string& val) {
    bool ok = true;
    vector<string> v = SplitInSomething("+", false, val);
    labelformat.resize(v.size());
    for (size_t i=0; i<v.size(); i++) {
      int len = atoi(v[i].c_str()), min = i==v.size()-1 ? 0 : 1;
      if (len<min || len>20)
	ok = false;
      labelformat[i] = len;
    }
    if (ok)
      SetLabelRegExps();

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::ClearObjects() { 
    _objects.clear(); 
    origins_entry_cache_map.clear();
    //bindetections.clear();
    for (auto i=bindetections.begin(); i!=bindetections.end(); i++)
      i->second.erase_all();
    
    //index.clear();
    for (size_t i=0; i<Nindices(); i++)
      if (IndexIs(i, "vectorindex") || IndexIs(i, "tssom")) {
	VectorIndex *vidx = (VectorIndex*)GetIndex(i);
	vidx->BinDataEraseAll();
	vidx->Data().FullDelete();
      }
  }

  /////////////////////////////////////////////////////////////////////////////

  object_info& DataBase::AddObject(const string& l, target_type t) {
    try {
      //Tic("AddObject");
      object_info& ret = _objects.add(this, l, t);
      //Tac("AddObject");
      return ret;
    } 
    catch (const logic_error& e) {
      ShowError("DataBase::AddObject(", l, ") failed : ", e.what());
    }
    catch (...) {
      ShowError("DataBase::AddObject(", l, ") failed ...");
    }
    static object_info dummy(NULL, -1, "", target_no_target);
    return dummy;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadLabels(bool append) {
    Tic("ReadLabels");

    if (!append) {
      tt_stat.clear();
      ClearObjects();
    }

    bool ret = true;

    if (UseSql())
      ret = ReadLabelsSql(append);
    else if (FileExists(ExpandPath(labels)))
      ret = ReadLabelsClassical(append);

    Tac("ReadLabels");

    return ret;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadLabelsSql(bool append) {
    WriteLog("Starting to read labels from SQL");

    string msg = "DataBase::ReadLabelsSql() : ";

    if (append)
      return ShowError(msg+"append not allowed");

    Tic("ReadLabelsSql");

    struct timespec now = TimeNow();
    size_t n = 0, ll = LabelLength();

    string cmd = "SELECT indexz,label,type FROM objects ORDER BY indexz;";
    if (DebugSql())
      cout << TimeStamp() << msg << cmd << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd);
      if (!stmt) {
	SqliteClose();
	return false;
      }

      int rrr = 0;
      for (;; n++) {
	rrr = sqlite3_step(stmt);
      
	if (rrr!=SQLITE_ROW) {
	  break;
	}

	if (DebugSql())
	  SqliteShowRow(stmt);

	size_t idx   = (size_t)SqliteInt(stmt, 0);
	string label = SqliteString(stmt, 1);
	int type     = SqliteInt(stmt, 2);

	if (idx!=n || type<0 || label=="")
	  SimpleAbort();

	if (type==0 && false)
	  WarnOnce(msg+"object's target_type==0 possibly due an empty file?");

	target_type ttraw    = (target_type)type; // SolveTargetTypes(type);
	//target_type ttcooked = TargetTypeTricks(ttraw);
	target_type ttuse = ttraw;
	object_info& oi = AddObject(label, ttuse);
	if (label[ll]==':' &&
	    label.find_first_not_of("0123456789", ll+1)==string::npos)
	  oi.frame = atoi(label.substr(ll+1).c_str()); 
	IncrementTargetTypeStatistics(ttuse);
      }

      rrr = sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H
    
#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt) {
	MysqlClose();
	return false;
      }

      if (mysql_stmt_execute(stmt)) {
	ShowError(msg+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }

      int index = -1, type = -1;
      char label[100] = "";
      unsigned long label_length = 0;
      MYSQL_BIND bind[3];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_LONG;
      bind[0].buffer        = &index;
      bind[0].buffer_length = sizeof(index);

      bind[1].buffer_type   = MYSQL_TYPE_STRING;
      bind[1].buffer        = label;
      bind[1].buffer_length = sizeof(label);
      bind[1].length        = &label_length;

      bind[2].buffer_type   = MYSQL_TYPE_LONG;
      bind[2].buffer        = &type;
      bind[2].buffer_length = sizeof(type);

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(msg+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      for (;; n++) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(msg+"data truncated");
	  else
	    ShowError(msg+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	string lab = label;

	if (size_t(index)!=n || type<=0 || lab=="")
	  SimpleAbort();

	target_type ttraw    = (target_type)type;
	//target_type ttcooked = TargetTypeTricks(ttraw);
	target_type ttuse = ttraw;
	object_info& oi = AddObject(label, ttuse);
	if (label[ll]==':' &&
	    label.find_first_not_of("0123456789", ll+1)==string::npos)
	  oi.frame = atoi(label.substr(ll+1).c_str()); 
	IncrementTargetTypeStatistics(ttuse);
      }
      MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H
    
    if (n) {
      struct timespec end = TimeNow();
      float secf = TimeDiff(end, now);
      char sec[100], nsec[100];
      sprintf(sec, "%.3f", secf);
      sprintf(nsec, "%.0f", n/secf);

      ostringstream ss;
      ss << "Read " << n << " object labels from SQL in " << sec <<
	" seconds (" << nsec << "/s), total size now " << Size();
      WriteLog(ss);
      ShowTargetTypeStatistics();
    }

    Tac("ReadLabelsSql");

    return n;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadLabelsClassical(bool append) {
    WriteLog("Starting to read labels from <"+labels+">");

    Tic("ReadLabelsClassical");

    string last_old = append && Size() ? Label(Size()-1) : "";

    bool ok = true, add = !append;

    int n_added = 0;

    int oldsize = Size();

    string fname = ExpandPath(labels);
    ifstream is(fname.c_str());
    if (!is) 
      ok = ShowError("DataBase::ReadLabels() : no file ", fname);
    else {
      for (int line_no=0;; line_no++) {
	string line;
	getline(is, line);
	if (!is)
	  break;

	size_t s = line.find_first_of(" \t");
	string label = s!=string::npos ? line.substr(0, s) : line;
	if (s!=string::npos)
	  s = line.find_first_not_of(" \t", s);
	string type  = s!=string::npos ? line.substr(s) : "";

	if (append && !add && label==last_old) {
	  add = true;
	  continue;
	}

	if (!add)
	  continue;

	target_type tt = type!="" ? SolveTargetTypes(type)
	  : SolveTargetTypeFromLabel(label, segment_is_modifier);

	if (type=="" && tt!=target_imagefile)
	  WarnOnce(fname+" should be updated to contain target_type info");

	tt = TargetTypeTricks(tt);

	if (tt==target_no_target)
	  ShowError("DataBase::ReadLabels() : not solved type of <"+label+">");

	if (AddObject(label, tt).type!=tt)
	  ShowError("DataBase::ReadLabels() : check contents of <"+fname+">");

	n_added++;

	IncrementTargetTypeStatistics(tt);
      }
    }
    Tac("ReadLabelsClassical");

    if (ok) {
      ostringstream ss;
      ss << "Read " << n_added << " object labels from <" << fname
	 << ">, total size now " << Size();
      WriteLog(ss);
      ShowTargetTypeStatistics();
    }

    // update the labels file read time:
    labelsfile.WasRead();

    if (append) {
      MakeSubObjectIndex(oldsize);
      for (size_t i=0; i<Picsom()->Nqueries(); i++) {
	Picsom()->GetQuery(i)->ConditionallyLengthenVectors();
	Picsom()->GetQuery(i)->CheckModifiedDivFiles();
      }
    }

    if (debug_dumpobjs && !append)
      DumpObjects();

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  target_type DataBase::TargetTypeTricks(target_type tt) const {
    if (segment_is_modifier) {
      if (tt==target_segment) {
        tt = target_imagesegment;
        // WarnOnce(fname+" should combine segment with real types");
      }

      if (PicSOM::TargetTypeContains(tt, target_image) &&
          !PicSOM::TargetTypeContains(tt, target_segment)) 
        tt = target_type(tt|target_full);
    }
    return tt;
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::IncrementTargetTypeStatistics(target_type tt) {
    //Tic("IncrementTargetTypeStatistics");
    tt_stat[tt]++;
    if (IsCombined(tt)) {
      vector<target_type> all_tt = TargetTypeSet(tt);
      for (vector<target_type>::const_iterator i=all_tt.begin();
           i!=all_tt.end(); i++)
        tt_stat[*i]++;
    }
    //Tac("IncrementTargetTypeStatistics");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AppendSimpleListFile(const string& fname, const string& l) {
    if (UseSql())
      return ShowError("AppendSimpleListFile(\""+fname+"\") called with SQL");

    ofstream os(fname, ios::app);
    if (!os) 
      return ShowError("DataBase::AppendSimpleListFile() : opening <"+
		       fname+"> failed");
    
    os << l;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AppendSubobjectsFile(size_t idx, const vector<size_t>& s) {
    if (UseSql())
      return AppendSubobjectsFileSql(idx, s);

    return AppendSubobjectsFileClassical(idx, s);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AppendSubobjectsFileSql(size_t idx, const vector<size_t>& s) {
    string msg = "DataBase::AppendSubobjectsFileSql() : ";

    if (!SqlTableExists("subobjects") &&
	!SqlCreateTable("subobjects", "parent INTEGER,child INTEGER"))
      return ShowError(msg+"SqlCreateTable() failed");

    // transactions are now handled on higher level
    // if (!SqlBeginTransaction())
    //   return ShowError(msg+"SqlBeginTransaction() failed");

    for (auto i=s.begin(); i!=s.end(); i++) {
      string cmd = "INSERT INTO "+SqlTableName("subobjects", SqlStyle())
	+" VALUES("+ToStr(idx)+","+ToStr(*i)+");";
      if (!SqlExec(cmd, true))
	return ShowError("msg+SqlExec() failed with <"+cmd+">");
    }

    // if (!SqlEndTransaction())
    //   return ShowError(msg+"SqlEndTransaction() failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AppendSubobjectsFileClassical(size_t idx,
					       const vector<size_t>& s) {
    ostringstream os;
    os << Label(idx) << " +";
    for (size_t i=0; i<s.size(); i++)
      os << " " << Label(s[i]);
    os << endl;

    string sl_name = SimpleListFilename(simplelist_subobjects, "");
    string fname = ExpandPath(sl_name);

    return AppendSimpleListFile(fname, os.str());
  }

///////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadSimpleListFile(const string& fname,
				    list<list<string> >& ll,
				    bool words) {
    // if (UseSql())
    //   return ShowError("ReadSimpleListFile(\""+fname+"\") called with SQL");

    ifstream is(fname.c_str());
    if (!is) 
      return false;
    
    bool ok = true;
    for (;ok;) {
      list<string> nl;

      char line[409600]; // enough? what if more than ~40000 subobjects?
      char *next = NULL;
      char *item = NULL;
      is.getline(line, sizeof line);
      if (!is)
	break;

      string linein = line;
      if (!words)
	nl.push_back(linein);
      else {
        item = &line[0];
        while (ok && item != NULL && *item != '\0') {
          next = strpbrk(item," ");
          if(next != NULL) {
            *next = '\0';
            next++;
          }
          
          nl.push_back(item);
          item = next;
        }
      }
      ll.push_back(nl);
    }
  
    return ok;
    
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ReadSimpleList(simplelist_type type, const string& name) {
  string sl_name = SimpleListFilename(type, name);
  if (simplelist_map.find(sl_name) != simplelist_map.end())
    return true;

  SimpleList sl;

  string fname = ExpandPath(sl_name);
  bool obj_info = 
    (type == simplelist_duplicates || type == simplelist_subobjects);
  
  list<list<string> > ll;
  bool words = type != simplelist_proper_persons 
    && type != simplelist_proper_locations;
  
  if (!ReadSimpleListFile(fname, ll, words))
    return true; // no error displayed -- file isn't mandatory

  sl.clear();

  Tic("ReadSimpleList");

  bool ok = true;
  int n = 0, nn = 0;
  list<list<string> >::const_iterator it = ll.begin();
  for (; it!=ll.end(); it++) {
    const list<string>& line = *it;
    if (line.empty())
      continue;

    const string& firstword = line.front();
    if (firstword.find('#')!=string::npos)
      continue;

    if (!words) {
      if (sl.contains(firstword))
	WarnOnce(string("Line \"")+firstword+"\" duplicated in <"+
		 ShortFileName(fname)+">");
      sl.insert(firstword);
      n++;
      continue;
    }

    list<string>::const_iterator wit=line.begin();
    if (++wit == line.end()) continue;
    string next = *wit;

    object_info *master = NULL;
    
    if (obj_info) {
      master = FindObject(firstword);
      if (!master) continue;
    } 

    n++;

    if (type==simplelist_duplicates) {
      master->master = master->index;
    } else if (type==simplelist_subobjects) {
      if (wit == line.end() || (next != "+" && next != "="))
	ok = ShowError("Failed to process <"+firstword+">: `+' nor `=' found");

      if (next == "=") {
	ok = ShowError("Not yet implemented `=' in subobjects file");
	// fixme: add code here for clearing master children list
	// and their parent-pointers
      }
      next = *(++wit);
    }

    string prev = "";
    list<string> items;
    bool rangefound = false, rangedebug = false;

    if (rangedebug)
      cout << "XXX: Processing starts for "<< ShortFileName(fname) << endl;

    for (;wit!=line.end();wit++) {
      const string& item = *wit;
      items.clear();

      if (rangedebug)
	cout << "XXX: Analyzing " << item << endl;

      if (rangefound) { // if previous entry was a ".."

	// Shamelessly copied from MarkLabelRange():
	// but MarkLabelRange() was changed as of 2.486 on 2011-04-10 ...
	int fidx = atoi(prev.c_str()), lidx = atoi(item.c_str());
	for (int li=fidx+1; li<=lidx; li++) {
	  char ltmp[100];
	  sprintf(ltmp, "%0*d", li, (int)LabelLength());
	  int idx = LabelIndex(ltmp);
	  if (!IndexOK(idx))
	    ShowError("DataBase::ReadSimpleList(): inexistent label <",
		      ltmp, ">");
	  else {
	    items.push_back(ltmp);
	    if (rangedebug)
	      cout << "XXX: Adding " << ltmp << endl;
	  }
	}
	rangefound = false;

      } else if (item == "..") {
	rangefound = true;
	if (rangedebug)
	  cout << "XXX: Found \"..\", prev=" << prev << endl; 

      } else { // normal case, no ranges defined
	items.push_back(item);
	if (rangedebug)
	  cout << "XXX: Adding " << item << endl;
	prev = item;
      }

      list<string>::const_iterator itit = items.begin();
      for (; itit!=items.end(); itit++) {
	const string& lbl = *itit;

	if (rangedebug)
	  cout << "XXX: Processing " << lbl << endl;
      
	object_info *obj = NULL;
	if (obj_info) {	
	  obj = FindObject(lbl);
	  if (!obj) continue;
	}
	
	nn++;
	
	if (type==simplelist_duplicates) {
	  obj->master = master->index;
	  master->duplicates.push_back(obj->index);	
	  
	} else if (type==simplelist_subobjects) {
	  obj->parents.push_back(master->index);
	  master->children.push_back(obj->index);
	  
	} else if (type==simplelist_commons)
	  sl.insert(make_pair(lbl, firstword));
	
	else if (type==simplelist_classfeatures)
	  sl.insert(make_pair(firstword, lbl));

      }
    }
  }

  Tac("ReadSimpleList");

  if (ok) {
    simplelist_map[sl_name] = sl;
    
    if (obj_info) 
      WriteLog("Successfully read information on "+ToStr(nn)+" "+
	       SimpleListFilename(type)+" of "+ToStr(n)+
	       " objects from <"+ShortFileName(fname)+">");
    else {
      int totlen = sl.size();
      
      if (type==simplelist_commons)
	WriteLog("Successfully read information on "+ToStr(nn)+"/"+
		 ToStr(totlen)+" "+SimpleListFilename(type)+
		 " relations of "+ToStr(n)+" classes from <"+
		 ShortFileName(fname)+">");
      else
	WriteLog("Successfully read "+ToStr(n)+"/"+ToStr(totlen)+" lines of "+
		 SimpleListFilename(type)+
		 " data from <"+ShortFileName(fname)+">"); 
    }
    
  } else
    ShowError("Processing of <"+fname+"> failed");

  return ok;
}

  /////////////////////////////////////////////////////////////////////////////

  target_type DataBase::SolveTargetTypeFromLabel(const string& l,
						 bool segm_is_mod) {
    // in the future target_segment should not exits alone, but in
    // combinations like image+segment or video+segment.
    // segm_is_mod==true is used to move in this direction...
    // nota that target_file already is such a modifier!

    size_t ll = LabelLength();

    if (l.length()==ll && l.find_first_not_of("0123456789")==string::npos)
      return target_imagefile;

    string meth, imgname, fr, seg;
    DataBase::SplitLabel(l, meth, imgname, fr, seg); 
    if (imgname.length()!=ll)
      return target_no_target;

    if (seg.length()) {
      if (meth.length()) 
	return segm_is_mod ? target_imagesegment : target_segment;
      else {
	Simple::ShowError("SolveTargetTypeFromLabel("+l+") : ",
			  "segment without method");
	return target_no_target;
      }
    }

    if (meth.length()) 
      return target_image;
    
    if (fr.length()) 
      return target_image;
    
    return target_file;  // this should not happen or should be detected...
  }

///////////////////////////////////////////////////////////////////////////////

target_type DataBase::SolveDefaultTargetType() {
  if (defaultquery->Target()!=target_no_target)
    return defaultquery->Target();

  target_type tt = target_no_target;
  for (size_t i=0; i<Size() && tt==target_no_target; i++)
    tt = PicSOM::TargetTypeFileMasked(ObjectsTargetType(i));

  defaultquery->Target(tt);
  return tt;
}

///////////////////////////////////////////////////////////////////////////////

void DataBase::ShowTargetTypeStatistics() const {
  //Tic("ShowTargetTypeStatistics");
  WriteLog("TargetType statistics of ", Name(), " :");

  list<string> ll = TargetTypeStatisticsLong();
  for (list<string>::const_iterator i = ll.begin(); i!=ll.end(); i++)
    WriteLog(*i);
  //Tac("ShowTargetTypeStatistics");
}

///////////////////////////////////////////////////////////////////////////////

list<string>
DataBase::TargetTypeStatisticsLong(const tt_stat_t& tts) const {
  const char *fmt = "  %-15s%8d";

  list<string> ret;
  char logtext[100];
  sprintf(logtext, fmt, "= total =", Size());
  ret.push_back(logtext);
  
  for (map<target_type,int>::const_iterator i=tts.begin(); i!=tts.end(); i++) {
    sprintf(logtext, fmt, TargetTypeString(i->first).c_str(), i->second);
    ret.push_back(logtext);
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

string DataBase::TargetTypeStatistics(const tt_stat_t& tts) const {
  int all = 0;
  for (map<target_type,int>::const_iterator i=tts.begin(); i!=tts.end(); i++)
    all += i->second;

  // this "all" goes wrong if there are both file:s and file+image:s ...

  stringstream ss;
  ss << all << "/" << Size() << ": ";

  for (map<target_type,int>::const_iterator i=tts.begin(); i!=tts.end(); i++)
    ss << (i==tts.begin()?"":", ") << i->second << " x "
       << TargetTypeString(i->first);
  
  return ss.str();
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MakeSubObjectIndex(int start_idx) {
    struct timespec now = TimeNow();

    bool ok = UseSql() ?
      MakeSubObjectIndexSql(start_idx) :
      MakeSubObjectIndexClassical(start_idx);

    if (!ok)
      return false;

    size_t n = 0, t = 0;
    for (int i=start_idx; i<(int)Size(); i++) {
      const object_info *o = FindObject(i);
      n += o->children.size()!=0;
      t += o->children.size();
    }

    struct timespec end = TimeNow();
    float secf = TimeDiff(end, now);
    char sec[100];
    sprintf(sec, "%.3f", secf);

    WriteLog("Built subobject index in "+ToStr(sec)+" seconds, "+
	     ToStr(n)+" new objects have total of "+ToStr(t)+" subobjects");

    if (debug_dumpobjs)
      DumpObjects();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MakeSubObjectIndexSql(int start_idx) {
    string msg = "DataBase::MakeSubObjectIndexSql("+ToStr(start_idx)+") : ";

    if (!SqlTableExists("subobjects"))
      return true;

    string cmd = "SELECT parent,child FROM subobjects WHERE child>="
      +ToStr(start_idx)+";";
    if (DebugSql())
      cout << TimeStamp() << msg << cmd << endl;    

    list<pair<size_t,size_t>> pc;
    
#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return ShowError(msg+"SqlitePrepare() failed with ["+cmd+"]");

      int rrr = 0;
      size_t n = 0;
      for (;; n++) {
	rrr = sqlite3_step(stmt);
	if (rrr!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	size_t pidx = (size_t)SqliteInt(stmt, 0);
	size_t cidx = (size_t)SqliteInt(stmt, 1);

	pc.push_back(make_pair(pidx, cidx));
      }
      rrr = sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return ShowError(msg+"MysqlPrepare() failed with ["+cmd+"]");

      if (mysql_stmt_execute(stmt)) {
	ShowError(msg+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
	
      size_t pidx, cidx;
      MYSQL_BIND bind[2];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_LONGLONG;
      bind[0].buffer        = &pidx;
      bind[0].buffer_length = sizeof(pidx);
      bind[1].buffer_type   = MYSQL_TYPE_LONGLONG;
      bind[1].buffer        = &cidx;
      bind[1].buffer_length = sizeof(cidx);

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(msg+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      int rrr = 0;
      size_t n = 0;
      for (;; n++) {
	rrr = mysql_stmt_fetch(stmt);

	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(msg+"data truncated");
	  else
	    ShowError(msg+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	pc.push_back(make_pair(pidx, cidx));
      }

      rrr = MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H

    for (auto i=pc.begin(); i!=pc.end(); i++) {
      size_t pidx = i->first, cidx = i->second;

      if (!IndexOK(pidx)||!IndexOK(cidx))
	ShowError("Invalid db index in either parent=#"+ToStr(pidx)+
		  " or child=#"+ToStr(cidx));
      else {
	object_info *oi_i = FindObject(cidx);
	object_info *oi_p = FindObject(pidx);
	bool found = false;
	if (oi_i->parents.size()<oi_p->children.size()) {
	  for (size_t j=0; j<oi_i->parents.size() && !found; j++)
	    if (oi_i->parents[j]==(int)pidx)
	      found = true;
	} else 
	  for (size_t j=0; j<oi_p->children.size() && !found; j++)
	    if (oi_p->children[j]==(int)cidx)
	      found = true;
	
	if (!found) {
	  oi_i->parents.push_back(pidx);
	  oi_p->children.push_back(cidx);
	}
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MakeSubObjectIndexClassical(int start_idx) {
    Tic("MakeSubObjectIndexClassical_oi");
    for (int i=start_idx; i<(int)Size(); i++) {
      int p = ParentObjectByPruning(Label(i));
      if (p>=0 && p!=i) {
	object_info *oi_i = FindObject(i);
	object_info *oi_p = FindObject(p);
	bool found = false;
	if (oi_i->parents.size()<oi_p->children.size()) {
	  for (size_t j=0; j<oi_i->parents.size() && !found; j++)
	    if (oi_i->parents[j]==p)
	      found = true;
	} else 
	  for (size_t j=0; j<oi_p->children.size() && !found; j++)
	    if (oi_p->children[j]==i)
	      found = true;

	if (!found) {
	  oi_i->parents.push_back(p);  // was parents[0] = ...
	  oi_p->children.push_back(i);
	}
      }
    }
    Tac("MakeSubObjectIndexClassical_oi");

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

/*
bool DataBase::MakeSubObjectIndex() {
  Tic("MakeSubObjectIndex");
  subobjects.clear();
  subobjects.insert(subobjects.begin(), Size(), vector<int>());
  for (int i=0; i<Size(); i++) {
    string fname = LabelFileNamePart(Label(i));
    if (SolveTargetTypeFromLabel(Label(i)) != target_file) { // obs!??
      int j = LabelIndex(fname);
      if (!IndexOK(j))
	ShowError("DataBase::MakeSubObjectIndex() : <", Label(i),
		  ">'s root object <", fname.c_str(), "> unknown");
      else
	subobjects[j].push_back(i);
    }
  }
  Tac("MakeSubObjectIndex");

  bool ok = true;
  if (ok) {
    int n = 0, t = 0;
    for (size_t i=0; i<subobjects.size(); i++) {
      n += subobjects[i].size()!=0;
      t += subobjects[i].size();
    }
    char ns[100], ts[100];
    sprintf(ns, "%d", n);
    sprintf(ts, "%d", t);
    WriteLog("Built subobject index, ", ns, " objects have total of ",
	     ts, " subobjects");
  }

  return ok;
}
*/

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ObjectHasLinkToTargetType(int idx, target_type tt) const {
    bool debug = false;

    if (debug)
      cout << "ObjectHasLinkToTargetType() : idx=" << idx << " label=" <<
	Label(idx) << " tt=" << TargetTypeString(tt) << endl;

    if (ObjectsTargetType(idx)&tt) {
      if (debug)
	cout << "  return true 1" << endl;
      return true;
    }

    int parent = ParentObject(idx);
    if (parent==-1) {
      if (debug)
	cout << "  return false 1" << endl;
      return false;
    }

    if (ObjectsTargetType(parent)&tt) {
      if (debug)
	cout << "  return true 2" << endl;
      return true;
    }
  
    const vector<int>& so = SubObjects(parent);
    for (size_t i=0; i<so.size(); i++)
      if (ObjectsTargetType(so[i])&tt) {
	if (debug)
	  cout << "  return true 3: " << so[i] << " " << Label(so[i]) << endl;
	return true;
      }
  
    if (debug)
      cout << "  return false 2" << endl;

    return false;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::OpenLabelsFile(bool strict) {
  if (labels_ofs.is_open())
    return strict ? ShowError("OpenLabelsFile(strict) : already open") : true;

  RwLockWrite("OpenLabelsFile");

  string p = ExpandPath(labels);
  labels_ofs.open(p.c_str(), ios::app);

  if (!labels_ofs) {
    RwUnlockWrite("OpenLabelsFile failed");
    return ShowError("OpenLabelsFile() : failed to open <"+p+">");
  }

  WriteLog("Opened <"+ShortFileName(p)+"> for appending");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::CloseLabelsFile(bool strict) {
  if (!labels_ofs.is_open())
    return strict ? ShowError("CloseLabelsFile(strict) : not open") : true;

  labels_ofs.close();

  RwUnlockWrite("CloseLabelsFile");

  if (!labels_ofs)
    return ShowError("CloseLabelsFile() : stream errored");

  WriteLog("Closed <"+ShortFileName(ExpandPath(labels))+">");

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  int DataBase::AddOneLabel(const string& label, target_type tt,
			    bool keep_open, bool cached_only) {
    Tic("AddOneLabel");

    bool really_use_labels = !UseSql() && !cached_only;

    if (really_use_labels && !OpenLabelsFile(!keep_open))
      ShowError("DataBase::AddOneLabel() : failed to open labels");

    string tts = TargetTypeString(tt), line = label + " " + tts;

    if (really_use_labels)
      labels_ofs << line << endl;

    stringstream msg;
    msg << "object #" << Size() << " <" << label << "> type=" << tts;
    if (really_use_labels)
      msg << " in " << ShortFileName(ExpandPath(labels));

    int idx = -1;

    if (really_use_labels && !labels_ofs)
      ShowError("DataBase::AddOneLabel() : failed to add ", msg.str());

    else {
      if (!cached_only)
	WriteLog("Added ", msg.str());
      idx = AddObject(label, tt).index;
      ContentsResize();
    }

    if (really_use_labels && !keep_open)
      CloseLabelsFile(true);

    Tac("AddOneLabel");

    return idx;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddLabelAndParents(const string& l, target_type ttt,
				   bool keep_open) {
    if (LabelIndexGentle(l)!=-1)
      return true;

    target_type tt = PicSOM::TargetTypeFileMasked(ttt);

    string msg = "DataBase::AddLabelAndParents("+l+") : ";

    if (tt==target_no_target && !insertallow_no_target)
      return ShowError(msg+"failing with tt==no_target");

    string p = ParentObjectStringByPruning(l);
    if (p=="")
      return ShowError(msg+"failing in pruning");

    if (PicSOM::TargetTypeContains(tt, target_imagesegment)) { // was target_segment
      target_type ptt = target_image;  // obs! hard-coded!!!!
      if (!AddLabelAndParents(p, ptt, keep_open))
	return ShowError(msg+"failing in segment=>parent recursion");

    } else if (tt==target_image || tt==target_text || tt==target_video ||
	       tt==target_videosegment|| tt==target_sound ||
	       tt==target_imageset || tt==target_no_target) {
      // this should be a dummy call and the parent already exist.
      // if not then this gets trapped inside recursion.
      // obs! as of 2013-04-08 tt==target_videosegment was added in the hope
      // that it will work when **subobjects** ... are specified afterwards
      // if (AddLabelAndParents(p, target_no_target)==-1)
      //   return ShowError(msg+"failing in image=>no_target recursion");

    } else
      return ShowError(msg+"failing with unimplemented tt="+
		       TargetTypeString(tt));

    return AddOneLabel(l, ttt, keep_open, false)!=-1;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ReadContents(bool force, bool expand) {
  if (!force && ContentsClasses())
    return true;

  Tic("ReadContents");

  int size = Size();

  if (!size)
    return ShowError("DataBase::ReadContents() size==0");

  IntVector contents_seen(size);

  if (!FileExists(ContentsName()))
    WriteLog("Contents class file <"+ShortFileName(ContentsName())+
	     "> does not exist");

  ifstream cf(ContentsName());

  const size_t label_count_limit = 1000;

  for (int l=0; l<size;) {
    char line[1024];
    cf.getline(line, sizeof line);
    if (!cf) {
      // ShowError("DataBase::ReadContents() failed to read line:");
      // return false;
      goto doexit;
    }
    
    if (strspn(line, " \t")==strlen(line) || *line=='#')
      continue;
    l++;

    char label[1024];
    strcpy(label, line);
    char *ptr = strpbrk(label, " \t");
    if (ptr)
      *ptr = 0;
    else {
      continue;
//       ShowError("DataBase::ReadContents() malformed line: \"",
// 		line, "\"");
//       return false;
    }

    int i = LabelIndex(label);
    if (i<0 || i>=size || contents_seen[i]) {
      char tmp[10000];
      sprintf(tmp, "label=<%s>, i=%d, size=%d, contents_seen[i]=%d",
	      label, i, size, contents_seen.Get(i));
      ShowError("DataBase::ReadContents() vector number not good: ",
		tmp);
      // return false;
      continue;
    }
    contents_seen[i] = 1;

    ptr = strpbrk(line, " \t");
    ptr += strspn(ptr, " \t");
    for (; ptr && *ptr;) {
      char str[1024];
      strcpy(str, ptr);
      char *comma = strchr(str, ',');
      if (comma)
	*comma = 0;
      if (!strlen(str))
	break;

      ground_truth *col = ContentsFind(str);
      if (!col) {
	if (ContentsClasses()<label_count_limit) {
	  ground_truth gtx(str, size);
	  col = ContentsInsert(gtx);
	  col->Set(-1);
	  col->expandable(expand);
	} else
	  WarnOnce("Database <"+name+
		   "> has too many contentitems. Skipping the rest.");
      }

      if (col)
	col->Set(i, 1);
      
      // cout << "label=" << label << " i=" << i << " str=" << str
      //      << " col->Number()=" << col->Number() << endl;

      if (comma)
	ptr = strchr(ptr, ',')+1;
      else
	break;
    }
  }

 doexit:
  Tac("ReadContents");  

  WriteLog("Read contents file ", ContentsName());  

  return contents_seen.Sum()>0;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::SetTextQueryOption(const xmlNodePtr &n) {
  string name = GetProperty(n,"name");
  string value = GetProperty(n,"value");
  string text = GetProperty(n,"text");
  textqueryopt[name] = make_pair(value, text);
  
  bool optvalue = IsAffirmative(value);
  
  if (name == "correct-person")
    DefaultQuery().textquery_correct_person = optvalue;
  if (name == "correct-location")
    DefaultQuery().textquery_correct_location = optvalue;
  if (name == "check-commons")
    DefaultQuery().check_commons_classes = optvalue;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::SetClassAugment(const xmlNodePtr &n) {
  string classn = GetProperty(n,"class");  
  string featuren = GetProperty(n,"feature");
  string value = GetProperty(n,"value");  
  
  if (value.empty())
    return ShowError("DataBase::SetClassAugment: no value set for "
		     "classaugment.");

  DefaultQuery().SetAugmentValue(atof(value.c_str()),classn,featuren);
  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruth(const string& str, target_type tt,
				     int other, bool expand) {
    string msg = "DataBase::GroundTruth("+str+") : ";

    ground_truth ret;
    // if (expand)
    //   TrapMeHere();

    expand = false;

    if (debug_gt)
      cout << "GroundTruth(" << str << "," << TargetTypeString(tt)
	   << "," << other << "," << expand << ")" << endl;

    if (tt==target_no_target) {
      ShowError(msg+"target_type==target_no_target");
      return ret;
    }

    if (str[0]=='$' && str.find('(')!=string::npos && *str.rbegin()==')')
      return GroundTruthFunction(str, tt);

    bool star = false;
    if (str=="*") {
      ret = ground_truth("*", Size());
      ret.One();
      ret.text("All objects of the database");
      star = true;
    }

    if (!star && !ret.Length()) {
      int idx = -1;
      if (sscanf(str.c_str(), "#%d", &idx)==1) {
	if (!IndexOK(idx)) {
	  ShowError(msg+"<"+str+"> contains invalid object index");
	  return ret;
	}
      } else
	idx = LabelIndexGentle(str);

      if (IndexOK(idx)) {
	ret = ground_truth(Size(), -1);
	ret[idx] = 1;
	ret.Label(str);
      }
    }

    if (!star && !ret.Length() && str[0]==':') {
      const ground_truth *gt = ContentsFind(str);
      if (gt)
	ret = *gt;
    } 

    if (!star && !ret.Length() && str[0]!=':') {
      ret = ConditionallyReadClassFile(str, expand);

      if (!ret.Length()) {
	ReadContents(false, expand);
	const ground_truth *col = ContentsFind(str);
	if (col) {
	  if (tt!=target_file) // obs!??
	    ShowError(msg+"target_type!=target_file");
	
	  ret = *col;
	  ret.depends(ContentsName());
	}
      }
    }
  
    if (!star && !ret.Length()) {
      ShowError(msg+"expression <", str, "> could not be evaluated");
      return ret;
    }

    if (expand)
      GroundTruthExpandOld(ret);

    if (expand && tt!=target_any_target) {
      Tic("GroundTruth:expandloop");
  
      int changes = 0;
      map<string,int> info;

      for (int i=0; i<ret.Length(); i++)
	if ((ObjectsTargetType(i)&tt)!=tt && ret[i]!=other) {
	  int old = ret[i];
	  ret[i] = other;  // was 0
	  if (debug_gt) {
	    stringstream s;
	    s << ObjectsTargetTypeString(i) << " " << old << " -> " << other;
	    info[s.str()]++;
	    changes++;
	  }
	}
    
      if (debug_gt) {
	cout << " * changed ground truth of " << changes
	     << " of total of " << ret.Length() << " objects: ";
	for (map<string,int>::const_iterator i=info.begin(); i!=info.end(); i++)
	  cout << (i!=info.begin()?",  ":"") << i->second << " x " << i->first;
	cout << endl;
	GroundTruthSummary(ret);
      }

      Tac("GroundTruth:expandloop");
    }

    ret.expandable(expand);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthFunction(const string& str,
					     target_type tt) {
    stringstream ss;
    ss << "GroundTruthFunction([" << str << "], " << TargetTypeString(tt)
       << ")";
    string msg = ss.str();

    if (debug_gt)
      cout << TimeStamp() << msg << endl;

    ground_truth empty(Size());

    if (tt==target_no_target) {
      ShowError(msg+" : target_type==target_no_target");
      return empty;
    }

    if (str[0]!='$') {
      ShowError(msg+" : ["+str+"] dollar sign missing");
      return empty;
    }

    string f = str.substr(1);
    vector<string> aa;

    size_t p = f.find('(');
    if (p!=string::npos) {
      string a = f.substr(p+1);
      if (*a.rbegin()!=')') {
	ShowError(msg+" : ["+str+"] closing parenthesis missing");
	return empty;
      }
      a.erase(a.size()-1);
      aa = SplitInCommasObeyParentheses(a);
      f.erase(p);
    }

    if (debug_gt) {
      cout << " [" << f << "]";
      if (aa.size()) {
	cout << " ( ";
	for (size_t i=0; i<aa.size(); i++)
	  cout << "[" << aa[i] << "] ";
	cout << ")";
      }
      cout << endl;
    }

    typedef vector<string> V;

    if ((f=="eye" || f=="") && aa.size()>=1)      // $eye()
      return GroundTruthEye(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="exp" && aa.size()>=1)                 // $exp()
      return GroundTruthExp(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="type" && aa.size()>=1)                // $type()
      return GroundTruthType(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if ((f=="re" || f=="regexp") && aa.size()>=1) // $re() $regexp()
      return GroundTruthRegExp(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="parents" && aa.size()>=1)             // $parents() 
      return GroundTruthParents(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="children" && aa.size()>=1)            // $children()
      return GroundTruthChildren(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="siblings" && aa.size()>=1)            // $siblings()
      return GroundTruthSiblings(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="nchildren" && aa.size()>=2)           // $nchildren()
      return GroundTruthNchildren(tt, f, aa[0], aa[1], V(aa.begin()+2, aa.end()));

    if (f=="duplicates" && aa.size()>=1)          // $duplicates()
      return GroundTruthDuplicates(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="masters" && aa.size()>=1)             // $masters()
      return GroundTruthMasters(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="data" && aa.size()>=1)                // $data()
      return GroundTruthData(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="date" && aa.size()==1)                // $date()
      return GroundTruthDate(tt, f, aa[0]);

    if (f=="split" && aa.size()==5)               // $split()
      return GroundTruthSplit(tt, f, aa[0], aa[1], atoi(aa[2].c_str()),
			      atoi(aa[3].c_str()), aa[4]);

    if (f=="nkeys" && aa.size()>=2)               // $nkeys()
      return GroundTruthNkeys(tt, f, aa[0], aa[1], V(aa.begin()+2, aa.end()));

    if (f=="only" && aa.size()>=2)                // $only()
      return GroundTruthOnly(tt, f, aa[0], aa[1], V(aa.begin()+2, aa.end()));

    if (f=="switch" && aa.size()>=2)              // $switch()
      return GroundTruthSwitch(tt, f, aa[0], aa[1], V(aa.begin()+2, aa.end()));

    if (f=="head" && aa.size()>=2)                // $head()
      return GroundTruthHead(tt, f, aa[0], aa[1], V(aa.begin()+2, aa.end()));

    if (f=="randcut" && aa.size()>=3)             // $randcut()
      return GroundTruthRandcut(tt, f, aa[0], aa[1], aa[2],
                                V(aa.begin()+3, aa.end()));

    if (f=="text" && aa.size()>=1)                // $text()
      return GroundTruthText(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="sql" && aa.size()>=1)                 // $sql()
      return GroundTruthSql(tt, f, aa[0], V(aa.begin()+1, aa.end()));

    if (f=="labelrange" && aa.size()>=2)         // $labelrange()
      return GroundTruthLabelRange(tt, f, aa[0], aa[1],
				   V(aa.begin()+2, aa.end()));

    if (f=="indexrange" && aa.size()>=2)         // $indexrange()
      return GroundTruthIndexRange(tt, f, aa[0], aa[1],
				   V(aa.begin()+2, aa.end()));

    if (f=="lscom2trecvid" && aa.size()>=3)      // $lscom2trecvid()
      return GroundTruthLscom2Trecvid(tt, f, aa[0], aa[1], aa[2],
				      V(aa.begin()+3, aa.end()));

    if (f=="middleframe" && aa.size()>=1)        // $middleframe()
      return GroundTruthMiddleFrame(tt, f, aa[0], 
				    V(aa.begin()+1, aa.end()));

    if (f=="auxid" && aa.size()>=2)              // $auxid()
      return GroundTruthAuxid(tt, f, aa[0], aa[1], 
			      V(aa.begin()+2, aa.end()));

    ShowError(msg+" : function ["+f+"] with ",
	      ToStr(aa.size()), " arguments is unknown");

    return empty;
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthEye(target_type tt, const string& f,
					const string& e,
					const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthEye/" << f << "(" << TargetTypeString(tt)
	   << ", [" << e << "]," << CommaJoin(a) << ")" << endl;

    ground_truth gt = GroundTruthExpression(e);

    return GroundTruthCommons(gt, tt, f, e, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthExp(target_type tt, const string& f,
					const string& e,
					const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthExp/" << f << "(" << TargetTypeString(tt)
	   << ", [" << e << "]," << CommaJoin(a) << ")" << endl;

    ground_truth gt  = GroundTruthExpression(e);
    ground_truth gte = GroundTruthExpandNew(gt, a);
    
    return GroundTruthCommons(gte, tt, f, e, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthType(target_type tt, const string& f,
					 const string& t,
					 const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthType/" << f << "(" << TargetTypeString(tt)
	   << ", [" << t << "])" << endl;

    target_type ttx = target_no_target;

    if (t=="qt" || t=="")
      ttx = tt;

    else {
      ttx = SolveTargetTypes(t);
      if (ttx==target_no_target && t!="no_target") {
	ShowError("GroundTruthType() : failed with <"+t+">");
	return ground_truth();
      }
    }

    ground_truth gt(Size(), -1);
    if (ttx==target_any_target) // now ==target_no_target
      gt.Set(1);

    else // if (ttx!=target_no_target)
      for (size_t i=0; i<gt.size(); i++)
	if (PicSOM::TargetTypeContains(ObjectsTargetType(i), ttx))
	  gt[i] = 1;

    return GroundTruthCommons(gt, tt, f, t, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthRegExp(target_type tt, const string& f,
					   const string& re,
					   const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthRegExp/" << f << "(" << TargetTypeString(tt)
	   << ", [" << re << "]," << CommaJoin(a) << ")" << endl;

    //Tic("GroundTruthRegExp");

    string r = re;
    if (r[0]=='/')
      r.erase(0, 1);
    if (r.size() && r[r.size()-1]=='/')
      r.erase(r.size()-1);

    if (r=="") {
      ShowError("GroundTruthRegExp() : empty regexp");
      return ground_truth();
    }

    RegExp reg(r, false);
    if (!reg.ok()) {
      ShowError("GroundTruthRegExp() : regcomp() failed with <"+r+"> ", reg.error());
      return ground_truth();
    }

    ground_truth gt(Size(), -1);

    for (size_t i=0; i<gt.size(); i++)
      if (reg.match(Label(i)))
	gt[i] = 1;

    bool also_meta = true; // used to be false which is much slower...
    list<string> clist = FindAllClassNames(false, false, also_meta);

    for (list<string>::const_iterator j=clist.begin(); j!=clist.end(); j++)
      if (reg.match(*j)) {
        bool expand = true;
        ground_truth gtj = ConditionallyReadClassFile(*j, expand);
        gt = gt.TernaryOR(gtj);
      }

    //Tac("GroundTruthRegExp");

    return GroundTruthCommons(gt, tt, f, re, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthParents(target_type tt, const string& f,
					    const string& expr,
					    const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthParents/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << CommaJoin(a) << ")" << endl;

    ground_truth gtb = GroundTruthExpression(expr);
    ground_truth gt(Size(), -1);

    for (size_t i=0; i<gtb.size(); i++)
      if (gtb[i]==1) {
	const object_info& o = *FindObject(i);
	for (size_t j=0; j<o.parents.size(); j++)
	  gt[o.parents[j]] = 1;
      }

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthChildren(target_type tt, const string& f,
					     const string& expr,
					     const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthChildren/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << CommaJoin(a) << ")" << endl;

    ground_truth gtb = GroundTruthExpression(expr);
    ground_truth gt(Size(), -1);

    for (size_t i=0; i<gtb.size(); i++)
      if (gtb[i]==1) {
	const object_info& o = *FindObject(i);
	for (size_t j=0; j<o.children.size(); j++)
	  gt[o.children[j]] = 1;
      }

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthSiblings(target_type tt, const string& f,
					     const string& expr,
					     const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthSiblings/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << CommaJoin(a) << ")" << endl;

    ground_truth gtb = GroundTruthExpression(expr);
    ground_truth gt(Size(), -1);

    for (size_t i=0; i<gtb.size(); i++)
      if (gtb[i]==1) {
	const object_info& o = *FindObject(i);
	for (size_t j=0; j<o.parents.size(); j++) {
	  const object_info& p = *FindObject(o.parents[j]);
	  for (size_t k=0; k<p.children.size(); k++)
	    gt[p.children[k]] = 1;
	}
      }

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthNchildren(target_type tt, const string& f,
					      const string& gtexpr,
					      const string& nexpr,
					      const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthNchildren/" << f << "(" << TargetTypeString(tt)
	   << ", [" << gtexpr << "," << nexpr <<"]," << CommaJoin(a) << ")"
	   << endl;

    size_t num = atoi(nexpr.c_str()); // obs! also inequalities and ranges
                                      // should be supported
    
    ground_truth gtb = GroundTruthExpression(gtexpr);
    ground_truth gt(Size(), -1);

    for (size_t i=0; i<gtb.size(); i++)
      if (gtb[i]==1) {
	const object_info& o = *FindObject(i);
	if (o.children.size()==num)
	  gt[i] = 1;
      }

    return GroundTruthCommons(gt, tt, f, gtexpr+","+nexpr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthDuplicates(target_type tt, const string& f,
					       const string& expr,
					       const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthDublicates/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << CommaJoin(a) << ")" << endl;

    ground_truth gtb = GroundTruthExpression(expr);
    ground_truth gt(Size(), -1);

    for (size_t i=0; i<gtb.size(); i++)
      if (gtb[i]==1) {
	const object_info& o = *FindObject(i);
	for (size_t j=0; j<o.duplicates.size(); j++)
	  gt[o.duplicates[j]] = 1;
      }

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthMasters(target_type tt, const string& f,
					    const string& expr,
					    const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthMasters/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << CommaJoin(a) << ")" << endl;

    ground_truth gtb = GroundTruthExpression(expr);
    ground_truth gt(Size(), -1);

    for (size_t i=0; i<gtb.size(); i++)
      if (gtb[i]==1) {
	const object_info& o = *FindObject(i);
	gt[o.master!=-1 ? o.master : i] = 1;
      }

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthData(target_type tt, const string& f,
					 const string& expr,
					 const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthData/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << CommaJoin(a) << ")" << endl;

    size_t pos = expr.find_first_of("<=>");
    if (pos==string::npos) {
      ShowError("GroundTruthData() : no <=> found");
      return ground_truth();
    }
    
    string value = expr.substr(0, pos), oper = expr.substr(pos);

    pos = oper.find_first_not_of("<=>");
    if (pos==string::npos) {
      ShowError("GroundTruthData() : part after <=> empty");
      return ground_truth();
    }
    
    string limit = oper.substr(pos);
    oper.erase(pos);

    string feat, comp;
    int idx = -1;

    pos = value.find('.');
    if (pos!=string::npos && pos!=0 && pos!=value.size()-1) {
      feat = value.substr(0, pos);
      comp = value.substr(pos+1);
    }

    if (comp=="") {
      pos = value.find('[');
      size_t pose = value.find(']');
      if (pose==string::npos || pos>pose-2) {
	ShowError("GroundTruthData() : [...] not found");
	return ground_truth();
      }
      string idxstr = value.substr(pos+1, pose-pos-1);
      if (idxstr.find_first_not_of("0123456789")!=string::npos) {
	ShowError("GroundTruthData() : [...] found, but not index");
	return ground_truth();
      }

      idx = atoi(idxstr.c_str());

      feat = value.substr(0, pos);
    }

    if (debug_gt)
      cout << "  value=[" << value << "] feat=[" << feat << "] comp=["
	   << comp << "] idx=" << idx << " oper=[" << oper << "] limit=["
	   << limit << "]" << endl;

    ground_truth gt(Size(), -1);

    Index *ts = FindIndex(feat);
    if (!ts) {
      ShowError("GroundTruthData() : could not create TSSOM <"+feat+">");
      return ground_truth();
    }

    TSSOM *tssom = (TSSOM*)ts; // obs!

    tssom->ReadMapFile();

    if (idx==-1) {
      tssom->ReadMapFile();
      idx = tssom->GetComponentIndex(comp);
    }
    if (idx==-1) {
      ShowError("GroundTruthData() : idx==-1 from comp=["+comp+
		"] feat=["+feat+"]");
      return ground_truth();
    }

    float flimit = atof(limit.c_str());

    tssom->ReadDataFile();
    for (int i=0; i<tssom->Data().Nitems(); i++) {
      int didx  = LabelIndex(tssom->Data()[i].Label());
      float val = tssom->Data()[i][idx];
      gt[didx] = (oper=="<" && val<flimit) || (oper=="=" && val==flimit) || 
	(oper==">" && val>flimit) ? 1 : -1;
    }

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthDate(target_type tt, const string& f,
					 const string& date) {
    if (debug_gt)
      cout << "GroundTruthDate/" << f << "(" << TargetTypeString(tt)
	   << ", [" << date << "])" << endl;

    const char *mhead = "DataBase::GroundTruthDate() : ";

    ground_truth empty;

    if (date=="") {
      ShowError(mhead, "date expression empty");
      return empty;
    }

    char op = date[0];
    float lim = atof(date.substr(1).c_str());
    if ((op!='<' && op!='=' && op!='>') || lim==0.0) {
      ShowError(mhead, "date expression invalid");
      return empty;
    }

    Index *ts = FindIndex("date");
    if (!ts) {
      ShowError(mhead, "feature <date> not found in database <", Name(), ">");
      return empty;
    }

    TSSOM *tssom = (TSSOM*)ts; // obs!

    tssom->ReadDataFile();
    if (!tssom->Data().Nitems()) {
      ShowError(mhead, "feature <date> empty in database <", Name(), ">");
      return empty;
    }
  
    if (!tssom->Data().VectorLength()) {
      ShowError(mhead, "feature <date> zero length in database <", Name(), ">");
      return empty;
    }

    ground_truth ret(Size());
    ret.Label("$"+f+"("+date+")");

    for (int i=0; i<tssom->Data().Nitems(); i++) {
      int idx = LabelIndex(tssom->Data()[i].Label());
      float val = tssom->Data()[i][0];
      ret[idx] = (op=='<' && val<lim) ||
	(op=='=' && val==lim) || (op=='>' && val>lim) ? 1 : -1;
    }

    ret.expandable(false);

    if (debug_gt)
      GroundTruthSummary(ret);
  
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthSplit(target_type ttt, const string& f,
					  const string& str,
					  const string& ttstr,
					  size_t n, size_t m,
					  const string& how) {
    bool save_result = false;

    if (debug_gt)
      cout << "GroundTruthSplit/" << f << "(" << TargetTypeString(ttt)
	   << ", [" << str << "], " << ttstr << ", " << n << ", "
	   << m << ", " << how << ", " << ")" << endl;

    ground_truth ret(Size());
    ret.expandable(false);

    if (how!="H" && how!="V" && how!="Hx" && how!="Vx") {
      ShowError("GroundTruthSplit() : last arg should be H, Hx, V or Vx");
      return ret;
    }

    target_type ttx = SolveTargetTypes(ttstr);
    if (ttx==target_no_target) {
      ShowError("GroundTruthSplit() : target_type ["+ttstr+"] not solved");
      return ret;
    }
    // ttt not used at all...

    if (n<1 || m>=n) {
      ShowError("GroundTruthSplit() : strange args");
      return ret;
    }

    int o = -1;  //obs!
    ground_truth gt = GroundTruthExpression(str, ttx, o, false);
    ret.depends(gt);

    vector<size_t> v = gt.indices(1);
    size_t np = v.size();
    if (np<n) {
      ShowError("GroundTruthSplit() : too small class");
      return ret;
    }

    ret.Set(-1);
    ret.label("$"+f+"("+str+","+ToStr(n)+","+ToStr(m)+","+how+")");

    bool neg = how.size()>1;

    if (debug_gt)
      cout << "GroundTruthSplit(...) np=" << np << endl;

    // H: 0 0 0 1 1 1 2 2 2 3 3 3
    // V: 0 1 2 3 0 1 2 3 0 1 2 3
  
    if (how[0]=='H') {
      size_t fst = m*np/n, lst = (m+1)*np/n;
      for (size_t i=0; i<np; i++)
	if ((!neg && i>=fst && i<lst) || (neg && (i<fst || i>=lst)))
	  ret[v[i]] = 1;

    } else {
      for (size_t i=0; i<np; i++)
	if ((!neg && i%n==m) || (neg && i%n!=m))
	  ret[v[i]] = 1;
    }

    if (save_result)
      WriteClassFile("./xval-"+ToStr(n)+"-"+ToStr(m)+"-"+how,
		     ret, "", false);

    if (debug_gt)
      GroundTruthSummary(ret);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthNkeys(target_type tt, const string& f,
					  const string& expr,
					  const string& meta,
					  const vector<string>& a) {

    if (debug_gt)
      cout << "GroundTruthNkeys/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << meta << ","
	   << CommaJoin(a) << ")" << endl;

    ground_truth_set set = GroundTruthExpressionListNew(meta);

    size_t min = 0, max = set.size();
    int b, c;
    if (sscanf(expr.c_str(), ">%d", &b)==1)
      min = b+1;
    else if (sscanf(expr.c_str(), ">=%d", &b)==1)
      min = b;
    else if (sscanf(expr.c_str(), "<%d", &b)==1)
      max = b+1;
    else if (sscanf(expr.c_str(), "<=%d", &b)==1)
      max = b;
    else if (sscanf(expr.c_str(), "%d..%d", &b, &c)==2) {
      min = b;
      max = c;
    } else if (sscanf(expr.c_str(), "%d", &b)==1)
      min = max= b;
    else {
      ShowError("DataBase::GroundTruthNkeys() : failed to parse ["+expr+"]");
      return ground_truth();
    }

    ground_truth gt(Size(), -1);
    for (size_t j=0; j<set.size(); j++)
      gt.depends(set[j]);

    for (size_t i=0; i<gt.size(); i++) {
      size_t n = 0;
      for (size_t j=0; j<set.size() && n<=max; j++)
	if (set[j][i]==1)
	  n++;
      if (n>=min && n<=max)
	gt[i] = 1;
    }

    return GroundTruthCommons(gt, tt, f, expr+","+meta, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthOnly(target_type tt, const string& f,
					  const string& expr,
					  const string& meta,
					  const vector<string>& a) {

    if (debug_gt)
      cout << "GroundTruthOnly/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << meta << ","
	   << CommaJoin(a) << ")" << endl;

    ground_truth_set set = GroundTruthExpressionListNew(meta);

    ground_truth gt = GroundTruthExpression(expr);
    for (size_t j=0; j<set.size(); j++)
      gt.depends(set[j]);

    size_t o = set.find(expr);

    if (!set.index_ok(o)) {
      ShowError("GroundTruthOnly() : ["+expr+"] not in ["+meta+"]");
      return ground_truth();
    }

    for (size_t i=0; i<gt.size(); i++)
      for (size_t j=0; j<set.size() && gt[i]==1; j++)
	if (j!=o && set[j][i]==1)
	  gt[i] = -1;

    return GroundTruthCommons(gt, tt, f, expr+","+meta, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthSwitch(target_type tt, const string& f,
					  const string& expr,
					  const string& meta,
					  const vector<string>& a) {

    if (debug_gt)
      cout << "GroundTruthSwitch/" << f << "(" << TargetTypeString(tt)
	   << ", [" << expr << "]," << meta << ","
	   << CommaJoin(a) << ")" << endl;

    int i = ToIndex(expr);
    if (!IndexOK(i)) {
      ShowError("GroundTruthSwitch() : ["+expr+"] isn't proper index");
      return ground_truth();
    }

    ground_truth_set set = GroundTruthExpressionListNew(meta);

    ground_truth gt(Size(), -1);

    for (size_t j=0; j<set.size(); j++)
      if (set[j][i]==1) {
	gt = set[j];
	gt.text(set[j].label());
	break;
      }

    return GroundTruthCommons(gt, tt, f, expr+","+meta, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthHead(target_type tt, const string& f,
					 const string& file,
					 const string& nstr,
					 const vector<string>& a) {

    if (debug_gt)
      cout << "GroundTruthHead/" << f << "(" << TargetTypeString(tt)
	   << ", " << file << "," << nstr << ","
	   << CommaJoin(a) << ")" << endl;

    ground_truth gt(Size(), -1);
    
    int n = -1;
    if (nstr!="")
      n = atoi(nstr.c_str());

    list<string> files;
    if (IsMetaClassFile(file))
      files = SplitClassNames(file);
    else
      files.push_back(file);

    for (auto fi=files.begin(); fi!=files.end(); fi++) {
      vector<size_t> l = ReadOrderedClassFile(*fi, n);
      for (size_t i=0; i<l.size(); i++)
	gt[l[i]] = 1;
    }

    return GroundTruthCommons(gt, tt, f, file+","+nstr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthRandcut(target_type tt, const string& f,
                                            const string& cls,
                                            const string& nstr,
                                            const string& seedstr,
                                            const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthRandcut/" << f << "(" << TargetTypeString(tt)
	   << ", " << cls << "," << nstr << ","
	   << CommaJoin(a) << ")" << endl;

    int n = -1;
    if (nstr!="")
      n = atoi(nstr.c_str());

    int rseed = 42;
    if (seedstr!="")
      rseed = atoi(seedstr.c_str());

    ground_truth gt = GroundTruthExpression(cls);

    return GroundTruthCommons(GTRandcut(gt, n, rseed),
                              tt, f, cls+","+nstr+","+seedstr, a);
    
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GTRandcut(const ground_truth& gt, int n, int rseed) {
    srand(rseed);
    
    vector<size_t> ids = gt.indices(1);

    int (*rp)(int) = RandomNumber; // don't ask...
    random_shuffle(ids.begin(), ids.end(), rp);

    if (n <= 0 || (int)ids.size() < n)
      n=ids.size();
    
    ground_truth ret(Size(), -1);
    for (int i=0; i<n; i++)
      ret[ids[i]]=1;

    ret.label("randcut("+ToStr(n)+","+gt.label()+")");

    return ret;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthText(target_type tt, const string& f,
					 const string& expr,
					 const vector<string>& a) {
    if (debug_gt)
      cout << "GroundTruthText/" << f << "(" << TargetTypeString(tt)
	   << ", " << expr << "," << CommaJoin(a) << ")" << endl;

    ground_truth gt(Size(), -1);

    string iname = DefaultTextIndex();
    if (iname=="") {
      ShowError("GroundTruthText() failing due to undefined default index");
      return gt;
    }

    list<string> in { "search-string "+expr };
    if (debug_gt)
      cout << "SEARCH \"" << expr << "\" in \"" << iname << "\"" << endl;

    list<string> out = LuceneOperation(iname, in).second;
    for (auto i=out.begin(); i!=out.end(); i++) {
      if (debug_gt)
	cout << *i << endl;

      string label = *i;
      auto p = label.find(' ');
      if (p!=string::npos)
	label.erase(p);
      int idx = LabelIndex(label);
      gt[idx] = 1;
    }

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthSql(target_type tt, const string& f,
					const string& expr,
					const vector<string>& a) {
    stringstream msgss;
    msgss << "GroundTruthSql/" << f << "(" << TargetTypeString(tt)
	  << ", " << expr << "," << CommaJoin(a) << ") ";
    string msg = msgss.str();

    if (debug_gt)
      cout << TimeStamp() << msg << endl;

    ground_truth gt(Size(), -1);

    string keyeqval = expr;

    string cmd = "SELECT indexz FROM objects WHERE "+keyeqval+";";
    if (DebugSql() || debug_gt)
      cout << TimeStamp() << msg << cmd << endl;    

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt) {
	ShowError("SqlitePrepare() failed with ["+cmd+"]");
	return gt;
      }

      int rrr = 0;
      size_t n = 0;
      for (;; n++) {
	rrr = sqlite3_step(stmt);
	// SQLITE_BUSY SQLITE_DONE SQLITE_ROW SQLITE_ERROR
	// SQLITE_MISUSE SQLITE_ERROR
      
	if (rrr!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	string r = SqliteString(stmt, 0);
	size_t idx = atoi(r.c_str());
	if (IndexOK(idx))
	  gt[idx] = 1;
	else
	  ShowError("Invalid db index ["+r+"]");
      }

      rrr = sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt) {
	ShowError("MysqlPrepare() failed with ["+cmd+"]");
	return gt;
      }

      if (mysql_stmt_execute(stmt)) {
	ShowError(msg+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
	
      char buffer[1024];
      unsigned long length = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = buffer;
      bind[0].buffer_length = sizeof buffer;
      bind[0].length        = &length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(msg+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      int rrr = 0;
      size_t n = 0;
      for (;; n++) {
	rrr = mysql_stmt_fetch(stmt);

	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(msg+"data truncated");
	  else
	    ShowError(msg+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	string r = string(buffer, length);
	size_t idx = atoi(r.c_str());
	if (IndexOK(idx))
	  gt[idx] = 1;
	else
	  ShowError("Invalid db index ["+r+"]");
      }

      rrr = MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H

    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthLabelRange(target_type tt, const string& f,
					       const string& l1,
					       const string& l2,
					       const vector<string>& a) {
    stringstream msgss;
    msgss << "GroundTruthLabelRange/" << f << "(" << TargetTypeString(tt)
	  << ", " << l1 << "," << l2 << "," << CommaJoin(a) << ") ";
    string msg = msgss.str();

    if (debug_gt)
      cout << TimeStamp() << msg << endl;

    ground_truth empty, gt(Size(), -1);

    string digits = "0123456789";

    if (l1.find_first_not_of(digits)!=string::npos ||
	l2.find_first_not_of(digits)!=string::npos) {
      ShowError(msg+"only digit-only labels have been implemented");
      return empty;
    }

    int li1 = LabelIndex(l1);
    if (li1<0) {
      ShowError(msg+"label <"+l1+"> unknown");
      return empty;
    }

    int li2 = LabelIndex(l2);
    if (li2<0) {
      ShowError(msg+"label <"+l2+"> unknown");
      return empty;
    }

    if (debug_gt)
      cout << "  " << l1 << "=>#" << li1 << " " << l2 << "=>#" << li2 << endl; 

    int ii1 = atoi(l1.c_str()), ii2 = atoi(l2.c_str());

    for (int i=ii1; i<=ii2; i++) {
      char ll[100];
      sprintf(ll, "%0*d", (int)LabelLength(), i);
      string lab(ll);
      int idx = LabelIndexGentle(lab);
      if (debug_gt)
	cout << "  " << i << "=>" << lab << "=>#" << idx << endl; 
      if (idx<0) {
	// ShowError(msg+"label <"+lab+"> unknown");
	// return empty;
	continue;
      }

      gt[idx] = 1;
    }

    return GroundTruthCommons(gt, tt, f, l1+","+l2, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthIndexRange(target_type tt, const string& f,
					       const string& l1,
					       const string& l2,
					       const vector<string>& a) {
    stringstream msgss;
    msgss << "GroundTruthIndexRange/" << f << "(" << TargetTypeString(tt)
	  << ", " << l1 << "," << l2 << "," << CommaJoin(a) << ") ";
    string msg = msgss.str();

    if (debug_gt)
      cout << TimeStamp() << msg << endl;

    ground_truth empty, gt(Size(), -1);

    int li1 = LabelIndex(l1);
    if (li1<0) {
      ShowError(msg+"label <"+l1+"> unknown");
      return empty;
    }

    int li2 = l2=="end" ? Size() : LabelIndex(l2);
    if (li2<0) {
      ShowError(msg+"label <"+l2+"> unknown");
      return empty;
    }

    if (debug_gt)
      cout << "  " << l1 << "=>#" << li1 << " " << l2 << "=>#" << li2 << endl; 

    for (int idx=li1; idx<=li2; idx++) {
      if (debug_gt)
	cout << "  #" << idx << "=>" << Label(idx) << endl; 
      gt[idx] = 1;
    }

    return GroundTruthCommons(gt, tt, f, l1+","+l2, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthLscom2Trecvid(target_type tt,
						  const string& f,
						  const string& cls,
						  const string& pre,
						  const string& pos,
						  const vector<string>& a) {
    stringstream msgss;
    msgss << "GroundTruthLscom2Trecvid/" << f << "(" << TargetTypeString(tt)
	  << ", " << cls << "," << pre << "," << pos << ", "
	  << CommaJoin(a) << ") ";
    string msg = msgss.str();

    if (debug_gt)
      cout << TimeStamp() << msg << endl;

    ground_truth empty;

    string lscomnum = LscomName(cls, true, "lscom/trecvid2011");
    if (lscomnum=="")
      return empty;

    string lscom = SplitLscomName(lscomnum).first;

    string cname = pre+lscom+pos;

    ground_truth gt = GroundTruthExpression(cname);
    if (gt.size())
      WriteLog("Successfully mapped class name ["+pre+"]["+cls+"]["+pos+
	       "] to ["+cname+"]");

    return GroundTruthCommons(gt, tt, f, cls+","+pre+","+pos, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthMiddleFrame(target_type tt,
						const string& f,
						const string& expr,
						const vector<string>& a) {
    stringstream msgss;
    msgss << "GroundTruthMiddleFrame/" << f << "(" << TargetTypeString(tt)
	  << ", [" << expr << "]," << CommaJoin(a) << ") ";
    string msg = msgss.str();

    if (debug_gt)
      cout << TimeStamp() << msg << endl;

    ground_truth gtb = GroundTruthExpression(expr);
    ground_truth gt(Size(), -1);
    
    for (size_t i=0; i<gtb.size(); i++)
      if (gtb[i]==1) {
	auto m = VideoOrSegmentMiddleFrame(i, true);
	if (m.first!=(size_t)-1)
	  gt[m.first] = 1;
      }
    
    return GroundTruthCommons(gt, tt, f, expr, a);
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthAuxid(target_type tt,
					  const string& f,
					  const string& id,
					  const string& s,
					  const vector<string>& a) {
    stringstream msgss;
    msgss << "GroundTruthAuxid/" << f << "(" << TargetTypeString(tt)
	  << ", [" << id << "][" << s << "]," << CommaJoin(a) << ") ";
    string msg = msgss.str();

    if (debug_gt)
      cout << TimeStamp() << msg << endl;

    if (s=="0")
      return GroundTruthSql(tt, "sql", "auxid=\""+id+"\"", a);
    
    if (s=="1") {
      string e = "(auxid=\""+id+"\" OR auxid LIKE \"%/"+id+"\")";
      return GroundTruthSql(tt, "sql", e, a);
    }
    
    ShowError(msg+": s==\""+s+"\" unknown");
    
    return ground_truth();
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthApplyOperations(target_type tt,
						    const ground_truth& gt,
						    const vector<string>& aa) {
    if (debug_gt)
      cout << "GroundTruthApplyOperations() : tt=" << TargetTypeString(tt)
	   << endl;

    vector<string> no_args;

    ground_truth ret = gt;
    ret.expandable(false);

    for (size_t i=0; i<aa.size(); i++) {
      const string& a = aa[i];
      if (debug_gt)
	cout << "  [" << a << "]" << endl;

      target_type att = SolveTargetTypes(a, false);
      bool a_is_tt = att!=target_no_target || a=="no_target";
      bool is_numrange = isdigit(a[0]) || (a.at(0)=='-' && isdigit(a[1]));

      if (a_is_tt || a=="qt" || a.substr(0, 5)=="type=") {
	string type = a == "qt" || a_is_tt ? a :  a.substr(5);
	ground_truth tgt = GroundTruthType(tt, "type", type, no_args);
	ret = ret.TernaryAND(tgt);

      } else if (a=="exp")
	ret = GroundTruthExpandNew(ret);

      else if (!is_numrange) {
	ShowError("GroundTruthApplyOperations() : unknown op <"+a+">");
	return ground_truth();
      }
    }

    return ret;
  }

///////////////////////////////////////////////////////////////////////////////

set<int> DataBase::NumberRangesToSet(const vector<string>& a) const {
  set<int> levels;
  for (vector<string>::const_iterator it=a.begin(); it!=a.end(); it++) {
    const string& arg = *it;          
    // the following examples are possible: 1, 2-3, -3--6, -3-5
    size_t linepos = arg.find("-",1);
    if (linepos != string::npos) {
      int start = atoi(arg.substr(0,linepos).c_str());
      int end   = atoi(arg.substr(linepos+1).c_str());
      if (end < start) swap_int(start,end);
      for (int i=start; i<=end; i++) 
	levels.insert(i);
    } else {
      levels.insert(atoi(arg.c_str()));
    }
  }
  return levels;
}

///////////////////////////////////////////////////////////////////////////////

vector<int> DataBase::ExpandDownOneLevel(ground_truth& ret, 
					 const vector<int> oldidx, int nval,
					 bool keep_parent, 
					 bool set_child) const {
  string msg = "ExpandDownOneLevel() : ";

  map<string,int> info;
  size_t dst_count = 0, src_count = 0;
  vector<int> newidx;

  for (vector<int>::const_iterator it=oldidx.begin();it!=oldidx.end();it++) {
    int i = *it;

    const vector<int>& s = SubObjects(i);
    bool hit = false; 
      
    for (size_t j=0; j<s.size(); j++) {
      const int idx = s[j];
      if (!ret.IndexOK(idx)) {
	ShowError(msg+"index error");
	return vector<int>();
      }
      
      if (ret[idx]!=1) {
	newidx.push_back(idx);
	if (set_child) {
	  int old = ret[idx];
	  ret[idx] = ret[i];
	  dst_count++;
	  hit = true;
	
	  if (debug_gt) {
	    stringstream ss;
	    ss << ObjectsTargetTypeString(i) << "=>"
	       << ObjectsTargetTypeString(idx) << " " << old
	       << " -> " << int(ret[i]);;
	    info[ss.str()]++;
	  }
	}
      }
    }
    if (hit)
      src_count++;

    if (!keep_parent)
      ret[i] = nval;
  }
  
  if (debug_gt) {
    cout << " * expanded ground truth to " << dst_count << " subobjects from "
	 << src_count << " parent objects: ";
    for (map<string,int>::const_iterator i=info.begin(); i!=info.end(); i++)
      cout << (i!=info.begin()?",  ":"") << i->second << " x " << i->first;
    cout << endl;
    
  }

  return newidx;
}

///////////////////////////////////////////////////////////////////////////////

ground_truth DataBase::GroundTruthExpandNew(const ground_truth& c,
                                            const vector<string>& a) const {
  string msg = "GroundTruthExpandNew() : ";

  if (debug_gt || debug_gt_exp)
    cout << TimeStamp() << msg  << " entered <" << c.label() << "> expandable size="
	 << c.size() << endl;

  set<int> levels = NumberRangesToSet(a);

  // if nothing is given, insert default levels
  if (levels.empty()) {
    levels.insert(0);
    levels.insert(1);
  }

  if (debug_gt) {
   cout << "Expand to levels: ";
   for (set<int>::const_iterator it=levels.begin(); it!=levels.end(); it++)
     cout << (it==levels.begin()?"":", ") << *it;
   cout << endl;
  }

  // true for 3-valued gt:s (-1,0,1) false for 2-valued (-1,1)
  bool expand_minus = c.has_zero();

  // create new ground_truth object as a copy of c with modified label
  // and text
  ground_truth ret = c;
  //  ret.label(c.label()+"[exp]");
  if (c.text()!="")
    ret.text(c.text()+" EXPANDED");

  // nval equals the "neutral" value, i.e. -1 for 2-valued gt:s and 0
  // for 3-valued gt:s
  int nval = expand_minus?0:-1;

  vector<int> newidx;

  for (size_t i=0; i<ret.size(); i++)
    if (ret[i] != nval)
      newidx.push_back(i);

  int max = *(--levels.end());
  for (int j=0; j<max; j++) {
    newidx = ExpandDownOneLevel(ret,newidx,nval,levels.find(j)!=levels.end(),
				true);
  }

  if (debug_gt) 
    GroundTruthSummary(ret);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::GroundTruthExpandOld(ground_truth& c) const {
#if 0
  bool do_test = false;
  ground_truth tmp;
#else
  bool do_test = true;
  ground_truth tmp(c);
#endif

  bool okf = GroundTruthExpandOld_fast(c);
  if (!do_test)
    return okf;

  bool oks = GroundTruthExpandOld_slow(tmp);
  if (!tmp.DataEquals(c)) {
    cout << "***fast***" << endl;
    GroundTruthSummaryTable(c);
    cout << "***slow***" << endl;
    GroundTruthSummaryTable(tmp);
    return ShowError("GroundTruthExpandOld() : _fast and _slow differ");
  }

  return okf&&oks;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::GroundTruthExpandOld_slow(ground_truth& c) const {
  string msg = "GroundTruthExpandOld_slow() : ";

  if (!c.expandable()) {
    if (debug_gt || debug_gt_exp)
      cout << TimeStamp() << msg << "<" << c.Label() << "> unexpandable" << endl;
    return true;
  }

  if (debug_gt || debug_gt_exp)
    cout << TimeStamp() << msg  << "entered <" << c.Label() << "> expandable length="
	 << c.Length() << endl;

  if (debug_gt_exp)
    ShowError(msg+"entered");

  Tic("GroundTruthExpandOld_slow");

  // now +1 values of subobjects are NOT OVERWRITTEN by -1 values of parents

  map<string,int> info;
  int dst_count = 0, src_count = 0;
  for (int i=0; i<c.Length(); i++) {
    if (c[i]==0)
      continue;  // added 2.5.2006

    const vector<int>& s = SubObjects(i);
    bool hit = false;
    for (size_t j=0; j<s.size(); j++)
      if (!c.IndexOK(s[j]))
	return ShowError(msg+"index error");
      else if (c[s[j]]!=1) {
	int old = c[s[j]];
	c[s[j]] = c[i];
	dst_count++;
	hit = true;
	if (debug_gt) {
	  stringstream ss;
	  ss << ObjectsTargetTypeString(i) << "=>"
	     << ObjectsTargetTypeString(s[j]) << " " << old
	     << " -> " << int(c[i]);
	  info[ss.str()]++;
	}
      }
    if (hit)
      src_count++;
  }
  
  if (debug_gt) {
    cout << " * expanded ground truth to " << dst_count << " subobjects from "
	 << src_count << " parent objects: ";
    for (map<string,int>::const_iterator i=info.begin(); i!=info.end(); i++)
      cout << (i!=info.begin()?",  ":"") << i->second << " x " << i->first;
    cout << endl;
    GroundTruthSummary(c);
  }

  Tac("GroundTruthExpandOld_slow");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::GroundTruthExpandOld_fast(ground_truth& c) const {
  string msg = "GroundTruthExpandOld_fast() : ";

  if (!c.expandable()) {
    if (debug_gt || debug_gt_exp)
      cout << TimeStamp() << msg << "<" << c.label() << "> unexpandable" << endl;
    return true;
  }

  c = GroundTruthExpandNew(c);

//   if (debug_gt || debug_gt_exp)
//     cout << TimeStamp() << msg  << " entered <" << c.label() << "> expandable size="
// 	 << c.size() << endl;

//   if (debug_gt_exp)
//     ShowError(msg+"entered");

//   Tic("GroundTruthExpandOld_fast");

//   bool expand_minus = c.has_zero();

//   map<string,int> info;
//   int dst_count = 0, src_count = 0;

//   for (size_t i=0; i<c.size(); i++)
//     if (c[i]==1 || expand_minus&&c[i]==-1) {
//       const vector<int>& s = SubObjects(i);
//       bool hit = false;
//       for (size_t j=0; j<s.size(); j++) {
// 	const int idx = s[j];
// 	if (!c.IndexOK(idx))
// 	  return ShowError(msg+"index error");

// 	if (c[idx]!=1) {
// 	  int old = c[idx];
// 	  c[idx] = c[i];
// 	  dst_count++;
// 	  hit = true;
// 	  if (debug_gt) {
// 	    stringstream ss;
// 	    ss << ObjectsTargetTypeString(i) << "=>"
// 	       << ObjectsTargetTypeString(idx) << " " << old
// 	       << " -> " << int(c[i]);
// 	    info[ss.str()]++;
// 	  }
// 	}
//       }
//       if (hit)
// 	src_count++;
//     }
  
//   if (debug_gt) {
//     cout << " * expanded ground truth to " << dst_count << " subobjects from "
// 	 << src_count << " parent objects: ";
//     for (map<string,int>::const_iterator i=info.begin(); i!=info.end(); i++)
//       cout << (i!=info.begin()?",  ":"") << i->second << " x " << i->first;
//     cout << endl;
//     GroundTruthSummary(c);
//   }

//   Tac("GroundTruthExpandOld_fast");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

ground_truth 
DataBase::GroundTruthExpression(const string& ssin, target_type tt,
				int other, bool expand) {
  if (debug_gt)
    cout << "GroundTruthExpression([" << ssin << "]," << TargetTypeString(tt)
	 << "," << other << "," << expand << ")" << endl;

  ground_truth empty(Size());
  empty.Set(-1);
  empty.expandable(expand);

  string ss = ssin;
  if (ss=="")
    return empty;

  // A nice hack follows...
  if (ss.substr(0, 3)=="///") {
    string ssr = "$auxid("+ss.substr(3)+",0)";
    if (debug_gt)
      cout << " * replacing [" << ss << "] with [" << ssr << "]" << endl;
    ss = ssr;
  }
  if (ss.substr(0, 2)=="//") {
    string ssr = "$auxid("+ss.substr(2)+",1)";
    if (debug_gt)
      cout << " * replacing [" << ss << "] with [" << ssr << "]" << endl;
    ss = ssr;
  }
  
  bool err = false;

  string::const_iterator beg = ss.begin();
  string::const_iterator end = ss.end()-1;

  while (isspace(*beg) && beg<end)
    beg++;
  while (isspace(*end) && beg<end)
    end--;

  if (debug_gt)
    cout << " * extracted range [" << ss.substr(beg-ss.begin(), end-beg+1)
        << "]" << endl;

  string::const_iterator null = ss.end();
  string::const_iterator xorp = null, orp = null, andp = null, smp = null;

  int np = 0, ns = 0;
  bool outpars = *beg=='(' && *end==')';
  bool outrexp = *beg=='/' && *end=='/';
  for (string::const_iterator i = beg; i<=end; i++) {
    if (!np) {
      // obs! ^ is confused with aaa&/^123/ which has to written as aaa&(/^123/)
      if (*i=='^') 
	xorp = i;
      if (*i=='|')
	orp = i;
      if (*i=='&')
	andp = i;
      if (*i=='\\')
	smp = i;
    }

    if (*i=='/')
      ns++;

    if (*i=='(')
      np++;
    if (*i==')') {
      np--;
      if (np==0 && i!=end)
	outpars = false;
    }
    if (np<0)
      err = true;
  }

  if (err||np /* ||ns%2 */) {
    ShowError("DataBase::GroundTruthExpression(): syntax error in ["+ss+"]");
    return empty;
  }

  if (outrexp && ns==2)
    xorp = orp = andp = smp = null;

  if (outpars) {
    string::size_type b = beg-ss.begin()+1, n = end-beg-1;
    ground_truth inner = GroundTruthExpression(ss.substr(b, n), tt, other,
					       expand);
    if (inner.Label()!="") {
      stringstream sss, sstxt;
      sss << "(" << inner.label() << ")";
      inner.label(sss.str());
      sstxt << "( " << inner.text() << " )"; 
      inner.text(sstxt.str());
    }
    return inner;
  }

  if (xorp!=null || orp!=null || andp!=null || smp!=null) {
    bool do_xor = xorp!=null;
    bool do_or  =  orp!=null && !do_xor;
    bool do_and = andp!=null && !do_xor && !do_or;
    bool do_sm  =  smp!=null && !do_xor && !do_or && !do_and;

    string::const_iterator p = do_xor ? xorp : do_or ? orp :
      do_and ? andp : do_sm ? smp : null;

    if (debug_gt)
      cout << "   * splitting by " << *p << endl;

    string lstr = ss.substr(beg-ss.begin(), p-beg);
    string rstr = ss.substr(p-ss.begin()+1, end-p);
    ground_truth left = GroundTruthExpression(lstr, tt, other, expand);
    ground_truth rght = GroundTruthExpression(rstr, tt, other, expand);

    if (!left.Length() || !rght.Length()) {
      ShowError("DataBase::GroundTruthExpression(): failed with ["+ss+"]");
      return empty;
    }

    if (debug_gt)
      cout << (do_xor?"XOR":do_or?"OR":do_and?"AND":do_sm?"MINUS":"???")
	   << "ing (" << lstr << ") and (" << rstr << ")" << endl;

    const string delim(1, *p);
    CombineImageXYpoints(left, rght, delim);

    if (do_xor) return left.TernaryXOR(rght);
    if (do_or)  return left.TernaryOR( rght);
    if (do_and) return left.TernaryAND(rght);
    if (do_sm)  return left.TernarySetMinus(rght);

    ShowError("DataBase::GroundTruthExpression(): unexpected error...");
    return empty;
  }

  if (*beg=='~' || *beg=='!' || *beg=='%') {
    string ex = ss.substr(beg-ss.begin()+1, end-beg);
    ground_truth r = GroundTruthExpression(ex, tt, other, expand);

    if (debug_gt)
      cout << "NOTing (" << ex << ")" << endl;

    return *beg=='~' ? r.TernaryNOT() :
      *beg=='!' ? r.TernaryExclusive() : r.TernaryDefined();
  }

  string cls = ss.substr(beg-ss.begin(), end-beg+1);

  if (cls.size()>2 && cls[0]=='/' && cls[cls.size()-1]=='/') {
    if (debug_gt)
      cout << " * * calling GroundTruthRegExp(" << cls << ")" << endl;

    // it seems that tt is not really effective...
    return GroundTruthRegExp(tt, "regexp", cls, vector<string>());

    // until 2016-12-07:
    // return GroundTruthRegExp(target_any_target, "regexp", cls,
    //			     vector<string>());  
    // that is new interface: tt=any_target, other=-1, expand=false !
  }

  /* I don't remember nor understand why this short-cut was sometimes needed.
     It should anyway go through GroundTruthExpression()... 
  size_t dpos = cls.find('.');
  size_t spos = cls.find('[');
  size_t epos = cls.find(']');
  size_t opos = cls.find_first_of("<=>");
  if (opos!=string::npos && (dpos<opos || spos<epos && epos<opos)) {
    if (debug_gt)
      cout << " * * calling GroundTruthData(" << cls << ")" << endl;
    return GroundTruthData(target_any_target, "", cls,
			   vector<string>());  
    // that is new interface: tt=any_target, other=-1, expand=false !
  }
  */

  if (debug_gt)
    cout << " * * calling GroundTruth(" << cls << ")" << endl;

  return GroundTruth(cls, tt, other, expand);
}

  /////////////////////////////////////////////////////////////////////////////

  ground_truth_set DataBase::GroundTruthExpressionListNew(const string& s) {
    list<string> labels = SplitClassNames(s);
    
    ground_truth_set ret;
    for (list<string>::const_iterator i=labels.begin(); i!=labels.end(); i++)
      ret.insert(GroundTruthExpression(*i));

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth_set DataBase::GroundTruthExpressionListOld(const string& s,
							  bool exp) {
    // to be Obsoleted("DataBase::GroundTruthExpressionListOld()";

    vector<string> labels = SplitInCommas(s);
    
    int other = -1;                  // obs! hardcoded other==-1
    ground_truth_set ret;            // obs! hardcoded target_type==file
    for (size_t i=0; i<labels.size(); i++) 
      ret.insert(GroundTruthExpression(labels[i], target_file,
				       other, exp)); 

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  ground_truth DataBase::GroundTruthLine(const string& line) {
    ground_truth empty(Size(), -1), ret = empty;

    bool ok = true;
    for (size_t q = 0, p; ok; ) {
      p = line.find_first_not_of(" \t", q);
      if (p==string::npos)
	break;

      q = line.find_first_of(" \t", p);
      if (q==string::npos)
	q = line.size();
      int idx = LabelIndex(line.substr(p, q-p));
      // if (debug_gt)
      //   cout << "DataBase::GroundTruthLine() <" << line.substr(p, q-p)
      //        << "> -> " << idx << endl;
      if (idx<0)
	ok = ShowError("DataBase::GroundTruthLine() failed");
      else
	ret[idx] = 1;
    }

    if (ok && debug_gt)
      GroundTruthSummary(ret);

    return ok ? ret : empty;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<size_t,size_t> DataBase::GroundTruthTypeCount(const ground_truth& g,
						     target_type t) const {
    int neg = 0, pos = 0;

    for (int i=0; i<g.Length(); i++) {
      if (ObjectsTargetType(i)&t) {
	if (g[i]==-1)
	neg++;
	else if (g[i]==1)
	  pos++;
      }
    }
  
    return pair<size_t,size_t>(pos, pos+neg);
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::GroundTruthSummaryStr(const ground_truth& gt,
					 bool tab, bool txt, bool dep,
					 float w) const {
    int neg, zero, pos;
    gt.TernaryCounts(neg, zero, pos);

    stringstream os;

    os << "<" << gt.Label() << ">";
    if (w!=numeric_limits<float>::max())
      os << " weight=" << w;
      
    os << (gt.expandable() ? " EXP" : "")
       << " size=" << gt.Length() << " :"
       << " #neg=" << neg << " #zero=" << zero << " #pos=" << pos;
    size_t gt_icount = gt.countImplicit();
    if (gt_icount > 0)
      os << " (" << gt_icount << " implicit)";
    os << endl;

    typedef map<target_type, map<int,size_t> > info2_t;
    info2_t info2;
    for (int i=0; i<gt.Length(); i++)
      info2[ObjectsTargetType(i)][gt[i]]++;

    int maxv = 0, maxl = 0;
    for (info2_t::iterator i=info2.begin(); i!=info2.end(); i++) {
      string s = TargetTypeString(i->first);
      if ((int)s.size()>maxl)
	maxl = s.size();
      for (int j=-1; j<=1; j++)
	if ((int)i->second[j]>maxv)
	  maxv = i->second[j];
    }

    maxv = 1+(int)ceil(log10((float)maxv));

    if (!tab)
      os << "              ";

    bool comma = false, first = false;  // ( might be first = tab )
    for (info2_t::iterator i=info2.begin(); i!=info2.end(); ) {
      const string t = first ? "" : TargetTypeString(i->first);

      if (tab) {
	int a = i->second[-1], b = i->second[0], c = i->second[1];
	if (first) {
	  a = -1; b = 0; c = 1;
	}
	char tmp[10000];
	sprintf(tmp, "%-*s %*d %*d %*d   (%6.3g%%)", maxl, t.c_str(),
		maxv, a, maxv, b, maxv, c, 100.0*c/(a+b+c));
	os << "    " << tmp << endl;
	
      } else {
	for (int k=-1; k<=1; k++)
	  if (i->second[k]) {
	    os << (comma ? ",  " : "") << i->second[k]
	       << " x " << t << " " << k;
	    comma = true;
	  }
      }

      if (!first)
	i++;

      first = false;
    }

    if (!tab)
      os << endl;

    if (txt && gt.text()!="")
      os << "      " << gt.text() << endl;

    if (dep)
      for (set<string>::const_iterator i=gt.depends().begin();
	   i!=gt.depends().end(); i++)
	os << "      " << *i << endl;

    return os.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::GroundTruthSummaryStr(const aspect_map_t& gt,
					 bool tab, bool txt, bool dep) const {
    stringstream os;
    for (aspect_map_t::const_iterator i=gt.begin(); i!=gt.end(); i++)
      os << "aspect=\"" << i->first << "\" : "
	 << GroundTruthSummaryStr(i->second.first, tab, txt, dep,
				  i->second.second)
	 << endl;

    return os.str();
  }

  /////////////////////////////////////////////////////////////////////////////

bool DataBase::CheckPermissions(const Connection& conn) const {
  if (conn.Type()!=Connection::conn_socket)
    return true;

  if (nondevelopmentuse==false && pic_som->Development()==false) {
    if (DebugAccess() || RemoteHost::Debug())
      WriteLog("DataBase::CheckPermissions() non-development use denied");
    return false;
  }

  if (DebugAccess() || RemoteHost::Debug())
    WriteLog("DataBase::CheckPermissions() access contains ",
	     ToStr(access.Nitems()), " rules");
    
  for (int i=0; i<access.Nitems(); i++)
    if (access[i].Match(conn.Servant(), conn.Client(), conn.ClientCookie(),
			DebugAccess()))
      return true;

  if (DebugAccess() || RemoteHost::Debug()) {
    char cookie[1024] = "";
    if (conn.ClientCookie()!="")
      sprintf(cookie, " cookie=[%s]", conn.ClientCookie().c_str());
    char msg[2000];
    sprintf(msg, "++++++ DataBase::CheckPermissions %s%s failed to access"
	    " *%s*. access.Nitems()=%d", conn.Addresses().c_str(), cookie,
	    Name().c_str(), access.Nitems());
    WriteLog(msg);
  }
  
  return false;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::IsAllowed(const Connection *conn, 
				 const string &right) const {

  // What should be the correct return value if conn==NULL ?
  // (This actually happens sometimes, and causes segmentation 
  //  fault without the check)
  if (conn == NULL || conn->Type()!=Connection::conn_socket) {
    if (DebugAccess() || RemoteHost::Debug())
      WriteLog("DataBase::IsAllowed() : conn=NULL right=",right);
    return false;
  }

  // Check if any of the access rules allow the user from the given IP to 
  // do the given thing:
  for (int i=0; i<access.Nitems(); i++)
    if (access[i].Match(conn->Servant(), conn->Client(), conn->ClientCookie(),
			false)
	&& access[i].HasRight(right)) {
      if (DebugAccess() || RemoteHost::Debug()) {
	char msg[2000];
	sprintf(msg, "DataBase::IsAllowed() : addresses=%s right=%s : ALLOWED",
		conn->Addresses().c_str(), right.c_str());
	WriteLog(msg);
      }
      return true;
    }

  if (DebugAccess() || RemoteHost::Debug()) {
    char msg[2000];
    sprintf(msg, "DataBase::IsAllowed() : addresses=%s right=%s : DENIED",
	    conn->Addresses().c_str(), right.c_str());
    WriteLog(msg);
  }

  return false;
}

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SolveDirectory(const string& head,
				  const string& label) const {
    string name;

    for (int i=0; i<3; i++) {
      if (i==1 && secondarypath.empty())
	continue;

      name = i%2 ? secondarypath : path;

      if (head!="")
	name += string("/")+head;

      if (i<2 && !Exists(name))
	continue;
  
      if (label.length()>=LabelLength()) {
	size_t p = 0;
	while (p<label.size() && (label[p]<'0' || label[p]>'9'))
	  p++;
	for (size_t j=0; j<labelformat.size()-1; j++) {
	  name += string("/")+label.substr(p, labelformat[j]);
	  p += labelformat[j];
	}
      }

      if (Exists(name))
	break;
    }

    return name;
  }

  /////////////////////////////////////////////////////////////////////////////

  //   const char *DataBase::SolveDirectoryMold(const char *head,
  // 					   const char *label) const {
  //     char name[1024];
  // 
  //     for (int i=0; i<3; i++) {
  //       if (i==1 && secondarypath.empty())
  // 	continue;
  // 
  //       strcpy(name, i%2 ? secondarypath.c_str() : path.c_str());
  // 
  //       if (head && *head) {
  // 	strcat(name, "/");
  // 	strcat(name, head);
  //       }
  // 
  //       if (i<2 && !Exists(name))
  // 	continue;
  //   
  //       if (label && strlen(label)>=LabelLength())
  // 	for (int ii=0; ii<3; ii++) {  // THIS IS DA PLACE (one of them)
  // 	  strcat(name, "/");
  // 	  strncat(name, label+2*ii, 2);
  // 	}
  // 
  //       if (Exists(name))
  // 	break;
  //     }
  // 
  //     return CopyString(name);
  //   }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SolveObjectPath(const string& namein, const string& subdir,
                                   const string& prefix, bool new_object,
                                   bool *exactin, bool may_fail,
                                   bool must_exist) const {
    string msg = "SolveObjectPath("+namein+") : ";

    string name = namein;

    if (name[0]=='#') {
      int idx = LabelIndex(name);
      if (idx<0)
	return ShowErrorS(msg+"LabelIndex() failed");
      
      name = Label(idx);
    }

    if (UseSql())
      return SolveObjectPathSql(name, subdir, prefix, new_object,
                                exactin, may_fail, must_exist);

    if (origins_entry_cache_map.size()) {
      int idx = LabelIndex(name);
      auto p = origins_entry_cache_map.find(idx);
      if (p!=origins_entry_cache_map.end()) {
	string method, body, frame, segm;
	if (!SplitLabel(name, method, body, frame, segm))
	  return ShowErrorS(msg+"failed in SplitLabel()");
	map<string,string> m = p->second;
	string file = m["file"];
	if (file=="")
	  return ShowErrorS(msg+"empty 'file' for <"+namein+">");

	if (subdir=="")
	  return SolveObjectDirectory(body)+"/"+file;

	return SolveObjectDirectory(body)+"/"+subdir+"/"+body;
      }
    }

    return SolveObjectPathClassical(name, subdir, prefix, new_object,
                                    exactin, may_fail, must_exist);
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SolveObjectPathSql(const string& name, const string& subdir,
                                      const string& prefix, bool new_object,
                                      bool *exactin, bool may_fail,
                                      bool must_exist) const {
    string msg = "DataBase::SolveObjectPathSql("+name+") : ";

    if (new_object)
      return SolveObjectPathClassical(name, subdir, prefix, new_object,
				      exactin, may_fail, must_exist);

    string ret;

    if (subdir=="segments" && prefix!="") {
      ret = SolveObjectPathSql(name, "", "", false, exactin, may_fail,
			       must_exist);
      size_t p = ret.rfind('/');
      if (p==string::npos)
	return "";
      ret.insert(p+1, "segments/"+prefix);
      p = ret.rfind('.');
      if (p!=string::npos)
	ret.erase(p);
      ret += ".seg";

      return ret;
    }

    if (subdir!="" || prefix!="" || exactin /*||may_fail*/ /*||!must_exist*/)
      return ret;

    int idx = ToIndex(name);
    string cmd = "SELECT file FROM objects WHERE indexz = "+ToStr(idx)+";";
    if (DebugSql())
      cout << TimeStamp() << msg << cmd << endl;    

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt) {
	ShowError("SqlitePrepare() failed with ["+cmd+"]");
	return ret;
      }

      int rrr = 0;
      size_t n = 0;
      for (;; n++) {
	rrr = sqlite3_step(stmt);
	// SQLITE_BUSY SQLITE_DONE SQLITE_ROW SQLITE_ERROR
	// SQLITE_MISUSE SQLITE_ERROR
      
	if (rrr!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	ret = SqliteString(stmt, 0);
      }

      rrr = sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt) {
	ShowError("MysqlPrepare() failed with ["+cmd+"]");
	return ret;
      }

      if (mysql_stmt_execute(stmt)) {
	ShowError(msg+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
	
      char buffer[1024];
      unsigned long length = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = buffer;
      bind[0].buffer_length = sizeof buffer;
      bind[0].length        = &length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(msg+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      int rrr = 0;
      size_t n = 0;
      for (;; n++) {
	rrr = mysql_stmt_fetch(stmt);

	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(msg+"data truncated");
	  else
	    ShowError(msg+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	ret = string(buffer, length);
      }

      rrr = MysqlStmtClose(stmt);
    }
#endif // HAVE_MYSQL_H

    if (ret!="") {
      string method, body, frame, segm;
      if (!SplitLabel(name, method, body, frame, segm))
        return ShowErrorS(msg+"failed in SplitLabel()");
      ret = SolveObjectDirectory(body)+"/"+ret;
    }

    return ret;
  }

///////////////////////////////////////////////////////////////////////////////

string 
DataBase::SolveObjectPathClassical(const string& name, const string& subdir,
                                   const string& prefix, bool new_object,
                                   bool *exactin, bool may_fail,
                                   bool must_exist) const {
  bool ex = false;
  bool *exact = exactin ? exactin : &ex;
  *exact = ex;

  string msg = "DataBase::SolveObjectPathClassical(" + name + "," + subdir
    + "," + prefix + "," + (new_object?"1":"0") + ","
    + (exactin?"non-":"")+"NULL" + ") : ";

  if (debug_opath)
    cout << "Now in " << msg << endl;

  string method, body, frame, segm;
  if (!SplitLabel(name, method, body, frame, segm))
    return ShowErrorS(msg+"failed in SplitLabel()");

  if (debug_opath)
    cout << "method=" << method << ", body=" << body << ", frame=" << frame
	 << ", segm=" << segm << endl;

  string dir = SolveObjectDirectory(body);  // cannot be name!
  if (dir=="")
    return ShowErrorS(msg+"failed in SolveObjectDirectory("+body+")");

  if (debug_opath)
    cout << "dir=" << dir << ", objdir=" << ObjectRoot() << endl;

  if (prefix!="" && method!="" && new_object)
    return ShowErrorS(msg+"prefix!=\"\" && method!=\"\" && new_object");

  if (new_object) {
    string ret = dir+"/"+(subdir!=""?subdir+"/":"")+
      BuildLabel(method!=""?method:prefix, body, frame, segm);
    if (debug_opath)
      cout << TimeStamp() << msg << "returning <" << ret << ">" << endl;
    return ret;
  }

  // name might contain a segmentation method prefix.  Should we then
  // use body instead of name in the following call ?
  // CreateSegmentImage() atleast does the splitting by itself...

  if (subdir!="")
    return SolveObjectPathSubDir(dir+"/"+subdir, prefix+name, exact);

  if (prefix!="")
    return ShowErrorS(msg+"prefix!=\"\"");

  string rebuild      = BuildLabel("", body, frame, segm);
  string rebuildnoseg = BuildLabel("", body, frame, "");
  set<string> tested;

  string non_exact_origins;

  for (int i=0; i<4; i++) {
    const string& xxx = i==0 ? name : i==1 ? rebuild : i==2 ? rebuildnoseg : body;
    if (tested.find(xxx)!=tested.end())
      continue;

    string ret = SolveObjectPathOrigins(dir, xxx, body+".d", must_exist);
    tested.insert(xxx);

    if (ret=="")
      continue;

    if (i<3) {
      *exact = true;
      if (debug_opath)
	cout << TimeStamp() << msg << "returning <" << ret << "> with i=" << i
	     << " *exact=" << *exact << endl;

      return ret;
    }

    non_exact_origins = ret;
  }

  if (subdir=="") {
    string sdir = body+".d";
    if (debug_opath)
      cout << TimeStamp() << msg << "checking subdir " << sdir << endl;
    if (DirectoryExists(dir+"/"+sdir)) {
      string ret = SolveObjectPath(name, sdir, prefix, new_object, exactin);
      if (ret!="") {
	if (debug_opath)
	  cout << TimeStamp() << msg << "returning <" << ret << "> with *exact=" << *exact
	       << endl;
	return ret;
      }
    }
  }

  if (non_exact_origins!="") {
    *exact = false;
    if (debug_opath)
      cout << TimeStamp() << msg << "returning <" << non_exact_origins
	   << "> with i=2 (delayed)" << " *exact=" << *exact << endl;

    return non_exact_origins;
  }

  if (parent_view)
    return parent_view->SolveObjectPath(name, subdir, prefix,
					new_object, exactin, may_fail);

  if (may_fail) {
    if (debug_opath)
      cout << TimeStamp() << msg << "returning EMPTY with *exact=" << *exact << endl;
    return "";
  }

  cout << endl << "[" << dir << "]" << endl;

  Obsoleted(msg, true);

  bool first = true;
  if (first)
    SolveObjectPath(name, subdir, prefix, new_object, exactin);
  first = false;

  return "";
}

///////////////////////////////////////////////////////////////////////////////

string DataBase::SolveObjectPathSubDir(const string& dir,
					       const string& prefname,
					       bool *exact) const {
  if (debug_opath)
    cout << "Now in DataBase::SolveObjectPathSubDir("
	 << dir << "," << prefname << ")" << endl;

  size_t l = prefname.size();

  string ret;

#ifdef HAVE_DIRENT_H
  DIR *ds = opendir(dir.c_str());  
  for (dirent *dpv; ds && (dpv = readdir(ds)) && ret==""; ) {
    string e = dpv->d_name;
    
    if (e.substr(0, l)==prefname && (e.size()==l || e[l]=='.'))
      ret = e;
  }
  if (ds)
    closedir(ds);
#endif // HAVE_DIRENT_H

  if (ret=="") {
    // ShowError("DataBase::SolveObjectPathSubDir("+dir+","
    //           +prefname+") failed");
    if (debug_opath)
      cout << "DataBase::SolveObjectPathSubDir() returning empty" << endl;

    return "";
  }

  *exact = true;

  string dirret = dir+"/"+ret;

  if (debug_opath)
    cout << "DataBase::SolveObjectPathSubDir() returning <" << dirret
	 << "> with *exact=" << *exact << endl;

  return dirret;
}

///////////////////////////////////////////////////////////////////////////////

string DataBase::SolveObjectPathOrigins(const string& dir, const string& obj,
					const string& sdir,
					bool must_exist) const {
  
  if (debug_opath)
    cout << "Now in DataBase::SolveObjectPathOrigins("
	 << dir << "," << obj << "," << sdir << ")" << endl;

  map<string,string> hash =
    ((DataBase*)this)->ReadOriginsInfoInner(dir+"/"+origins, obj, false);

  if (hash.empty()) {
    if (debug_opath)
      cout << "DataBase::SolveObjectPathOrigins() returning empty 1" << endl;

    return "";
  }

  const string& filename = hash["name"];
  if (filename=="")
    return ShowErrorS("DataBase::SolveObjectPathOrigins("+dir+","+obj+","
		      +sdir+",false) failed with filename==\"\"");

  for (int i=0; i<2; i++) {
    string f = dir+"/"+(i==0 ? "" : sdir+"/")+filename;
    if (FileExists(f)) {
      if (debug_opath)
	cout << "DataBase::SolveObjectPathOrigins() returning <" << f << ">"
	     << endl;

      return f;
    }
  }

  if (!must_exist) {
    int i = 0;
    string f = dir+"/"+(i==0 ? "" : sdir+"/")+filename;
    if (debug_opath)
	cout << "DataBase::SolveObjectPathOrigins() returning non-existing <"
	     << f << ">" << endl;
    return f;
  }
   
  if (debug_opath)
    cout << "DataBase::SolveObjectPathOrigins() returning empty 2" << endl;

  return "";
}
					 
  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ObjectTextLineStore(const textline_t& tl,
				     const string& pref,
				     const string& suff) {
    string err = "DataBase::ObjectTextLineStore("+ToStr(tl.idx)+","+
      pref+","+suff+") : ";

    if (!OpenReadWriteDet())
      return ShowError(err+"database not opened dor detection writing, "
		       "use -rw=det");
		
    string f = ObjectTextFileSubdirPath(tl.idx, pref, suff);
    if (f=="")
      return ShowError(err+"file path not solved");

    if (FileExists(f))
      return ShowError(err+"file <"+f+"> exists, not overwriting");

    size_t p = f.rfind('/');
    if (!Picsom()->MkDirHier(f.substr(0, p), 02775))
      return ShowError(err+"MkDirHier("+f.substr(0, p)+") failed");

    string line = tl.txt_display()+"\n";
    if (!StringToFile(line, f))
      return ShowError(err+"failed to write file <"+f+">");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  textline_t DataBase::ObjectTextLineRetrieve(size_t idx,
					      const string& pref,
					      const string& suff,
					      bool angry) {
    string err = "DataBase::ObjectTextLineRetrieve("+ToStr(idx)+","+
      pref+","+suff+",angry="+(angry?"yes":"no")+") : ";
		
    string f = ObjectTextFileSubdirPath(idx, pref, suff);
    if (f=="") {
      ShowError(err+"file path not solved");
      return textline_t();
    }

    string ss = FileToString(f);
    textline_t l;
    if (ss=="" && angry)
      ShowError(err+"empty content in <"+f+">");
    if (ss!="" && !l.txt_decode(ss)) {
      ShowError(err+"parsing ["+ss+"] failed");
      return textline_t();
    }

    l.db  = this;
    l.idx = idx;

    return l;
  }
			 
  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ObjectTextFileSubdirPath(size_t idx, const string& pref,
					    const string& suff) {
    string err = "DataBase::ObjectTextFileSubdirPath("+ToStr(idx)+","+
      pref+","+suff+") : ";
		
    // string vfile = ObjectPathEvenExtract(idx);
    string vfile = SolveObjectPath(Label(idx));
    if (vfile=="")
      return ShowErrorS(err+"path for <"+vfile+"> not solved");

    string tf = vfile;
    size_t p = tf.rfind('.');
    size_t q = tf.rfind('/', p);
    tf.erase(p+1);
    tf += "d/"+pref+(pref!=""?":":"")+tf.substr(q+1, p-q-1)
      +(suff!=""?"-":"")+suff+".txt";

    return tf;    
  }
			 
  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ObjectTypeAndPath(const string& vlab,
				     target_type tt) /*const*/ {
    string err = "DataBase::ObjectTypeAndPath() : ";

    int idx = LabelIndex(vlab);

    if (idx==-1)
      return ShowErrorS(err+"label <"+vlab+"> unknown");
  
    if (!ObjectsTargetTypeContains(idx, tt))
      return ShowErrorS(err+"label <"+vlab+"> is not a "+TargetTypeString(tt));
	
    // string vlabreal = Label(idx);
    string vfile = ObjectPathEvenExtract(idx);
    if (vfile=="")
      return ShowErrorS(err+"path for <"+vfile+"> not solved");

    if (!FileExists(vfile))
      return ShowErrorS(err+"file <"+vfile+"> does not exist");

    return vfile;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlTempFileName(int idx) {
    string msg = "DataBase::SqlTempFileName("+ToStr(idx)+") : ";

    string tmpd = TempDir("objects", true);
    int mode = 02775;
    Picsom()->MkDirHier(tmpd, mode);
    map<string,string> m = ReadOriginsInfo(idx, false, false);
    string fname = tmpd+"/"+m["name"];

    return fname;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ObjectPathEvenExtract(size_t idx, bool local) {
    vector<size_t> idxs { idx  };
    auto r = ObjectPathsEvenExtract(idxs, local);
    return r.size() ? r.front() :
      ShowErrorS("DataBase::ObjectPathEvenExtract() failing");
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ObjectPathEvenExtractOld(size_t idx, bool local) {
    return ShowErrorS(" DataBase::ObjectPathEvenExtractOld() deprecated");

    string obj = InsertedObjectPath(idx, false, true, true);
    if (obj!="" && !IsMissingObject(idx))
      return obj;

    string errhead = "DataBase::ObjectPathEvenExtract("+ToStr(idx)+") : ";

    if (SqlObjects()) {
      int oidx = idx;
      if (ObjectsTargetType(oidx)==target_imagesegment)
	oidx = ParentObject(oidx);
      if (IsMissingObjectInSql(oidx)) { // always true...
	string fname = SqlTempFileName(oidx);
	string data = SqlObjectData(oidx);
	if (!StringToFile(data, fname))
	  ShowError(errhead+"failed to write "+ToStr(data.size())+" bytes in <"
		    +fname+">");
	return fname;
      }
    }

    obj = InsertedObjectPath(idx, false, false, false);
    if (obj=="")
      return ShowErrorS(errhead, "could not solve filename");

    // obs! does this logic work only when all images are in one tar file?
    if (extracted_files) {
      if (!local)
	return obj;

      string objloc = ConvertGlobalToLocalDiskName(obj);
      // cout << " B [" << obj << "]->[" << objloc << "]" << endl;
      if (FileExists(objloc))
	return objloc;
    }

    const string& label = Label(idx);

    WriteLog("Trying to extract file <"+ShortFileName(obj)
	     +"> for object <"+label+">");

    bool use_subdir = false;  // was like true until 2013-10-23
    if (!ExtractObject(idx, use_subdir))
      return ShowErrorS(errhead, "failed to extract 1");

    // So, there is no explicit link between label and obj.  We just hope...

    if (extracted_files || UseSql()) {
      string objold = obj;
      obj = ConvertGlobalToLocalDiskName(obj);
      // cout << " A [" << objold << "]->[" << obj << "]" << endl;
    }

    if (!FileExists(obj))
      return ShowErrorS(errhead, "failed to extract 2 <"+obj+">");

    if (false && FileSize(obj)<1) // obs! we now allow empty files...
      return ShowErrorS(errhead, "failed to extract 3 <"+obj+">");

    return obj;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::ObjectPathsEvenExtract(const vector<size_t>& idxs,
						bool local) {
    string msg = "DataBase::ObjectPathsEvenExtract() : ";

    list<string> empty;
    map<size_t,string> retmap;
    vector<pair<size_t,string> > ext;

    for (auto oi=idxs.begin(); oi!=idxs.end(); oi++) {
      size_t idx = *oi;

      string obj = InsertedObjectPath(idx, false, true, true);
      if (obj!="" && !IsMissingObject(idx)) {
	// added 2014-01-22:
	if (!FileExists(obj)) {
	  size_t p = obj.rfind('/');
	  if (p!=string::npos) {
	    size_t q = obj.find_first_not_of("0123456789", p+1);
	    if (q==p+LabelLength()+1) {
	      string objd = obj;
	      objd.insert(p, obj.substr(p, q-p)+".d");
	      if (FileExists(objd))
		obj = objd;
	    }
	  }
	}
	if (!FileExists(obj) && Picsom()->NoRootDir())
	  RootlessDownloadObject(Label(idx), obj);

	retmap[idx] = obj;
	continue;
      }

      string errhead = "DataBase::ObjectPathsEvenExtract("+ToStr(idx)+") : ";

      if (ObjectsTargetType(idx)==target_imagesegment) {
	int pidx = ParentObject(idx);
	if (pidx>=0 && ObjectsTargetType(pidx)==target_imagefile) {
	  string obj = InsertedObjectPath(pidx, false, true, true);
	  if (obj!="") {
	    retmap[idx] = obj;
	    continue;
	  }
	}
      }

      if (SqlObjects()) {
	int oidx = idx;
	if (ObjectsTargetType(oidx)==target_imagesegment)
	  oidx = ParentObject(oidx);
	if (IsMissingObjectInSql(oidx)) { // always true...
	  string fname = SqlTempFileName(oidx);
	  string data = SqlObjectData(oidx);
	  if (!StringToFile(data, fname))
	    ShowError(errhead+"failed to write "+ToStr(data.size())+
		      " bytes in <"+fname+">");
	  retmap[idx] = fname;
	  continue;
	}
	ShowError(errhead+"problem 1");
	return empty;
      }

      obj = InsertedObjectPath(idx, false, false, false);
      if (obj=="") {
	ShowError(errhead, "could not solve filename");
	return empty;
      }

      // obs! does this logic work only when all images are in one tar file?
      if (extracted_files) {
	if (!local) {
	  retmap[idx] = obj;
	  continue;
	}
      }

      // obs! Since 2015-03-06 this is no more conditioned with 
      // if (extracted_files)
      string objloc = ConvertGlobalToLocalDiskName(obj);
      // cout << " B [" << obj << "]->[" << objloc << "]" << endl;
      if (FileExists(objloc)) {
	retmap[idx] = objloc;
	continue;
      }

      ext.push_back(make_pair(idx, obj));
    }

    if (ext.size()) {
      stringstream ss;
      ss << "Trying to extract file";
      if (ext.size()==1)
	ss << " <" << ShortFileName(ext[0].second) << "> for object <"
	   << Label(ext[0].first) << ">";
      else
	ss << "s <" << ShortFileName(ext.front().second) << "> ... <"
	   << ShortFileName(ext.back().second) << "> for objects <"
	   << Label(ext.front().first) << "> ... <"
	   << Label(ext.back().first) << "> total " << ext.size()
	   << " objects";

      WriteLog(ss.str());

      vector<size_t> extidx;
      for (auto i=ext.begin(); i!=ext.end(); i++)
	extidx.push_back(i->first);

      bool subd = false; // obs! until 2015-03-04 this was implicitly true
      if (!ExtractObjects(extidx, subd)) {
	ShowError(msg, "failed to extract 1");
	return empty;
      }

      // So, there is no explicit link between label and obj.  We just hope...

      for (auto i=ext.begin(); i!=ext.end(); i++) {
	string errhead = msg+"idx="+ToStr(i->first)+" : ";
	string obj = i->second;

	if (extracted_files || UseSql()) {
	  // string objold = i->second;
	  obj = ConvertGlobalToLocalDiskName(obj);
	  // cout << " A [" << objold << "]->[" << obj << "]" << endl;
	}

	if (!FileExists(obj)) {
	  ShowError(errhead, "failed to extract 2 <"+obj+">");
	  return empty;
	}

	if (false && FileSize(obj)<1) { // obs! we now allow empty files...
	  ShowError(errhead, "failed to extract 3 <"+obj+">");
	  return empty;
	}

	retmap[i->first] = obj;
      }
    }
    
    list<string> ret;
    for (auto oi=idxs.begin(); oi!=idxs.end(); oi++) {
      string f = retmap[*oi];
      if (f=="") {
	ShowError(msg, "empty filename for #"+ToStr(*oi));
	return empty;

      } else
	ret.push_back(f);
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool DataBase::RootlessDownloadObject(const string& label,
					const string& path) const {
    string msg = "DataBase::RootlessDownloadObject("+label+","+path+") : ";
    WriteLog(msg+"starting");

    if (Picsom()->MasterAddress()=="")
      return ShowError(msg+"cannot download because master_address not set");

    string url = Picsom()->MasterAddress()+"/database/"+name+"/"+label, ctype;
    list<pair<string,string> > hdrs;
    hdrs.push_back(make_pair("X-Auth-Token", Picsom()->MasterAuthToken()));
    string tmp = Connection::DownloadFile(Picsom(), url, hdrs, ctype);
    
    if (tmp=="")
      return ShowError(msg+"downloading <"+url+"> failed");

    string dir = path;
    size_t p = dir.rfind('/');
    if (p!=string::npos)
      dir.erase(p);

    int mode = 02775;
    if (!Picsom()->MkDirHier(dir, mode))
      return ShowError(msg+"creating directory <"+dir+"> failed");

    if (!Rename(tmp, path))
      return ShowError(msg+"moving <"+tmp+"> to <"+path+"> failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool DataBase::RootlessDownloadClassFile(const string& cls,
					   bool angry) const {
    if (Picsom()->MasterAddress()=="")
      return true;

    string cpath = ExpandPath("classes", cls), dir = cpath;
    if (FileExists(cpath))
      return true;

    string msg = "DataBase::RootlessDownloadClassFile("+cls+") : ";
    WriteLog(msg+"starting");

    string url = Picsom()->MasterAddress()+"/database/"+name+"/classes/"+cls;
    string ctype;
    list<pair<string,string> > hdrs;
    hdrs.push_back(make_pair("X-Auth-Token", Picsom()->MasterAuthToken()));

    /*XXX*/
    string tmp = Connection::DownloadFile(Picsom(), url, hdrs, ctype);
    
    if (tmp=="")
      return angry ? ShowError(msg+"downloading <"+url+"> failed") : false;

    size_t p = dir.rfind('/');
    if (p!=string::npos)
      dir.erase(p);

    int mode = 02775;
    if (!Picsom()->MkDirHier(dir, mode))
      return ShowError(msg+"creating directory <"+dir+"> failed");

    if (!Rename(tmp, cpath))
      return ShowError(msg+"moving <"+tmp+"> to <"+cpath+"> failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool DataBase::RootlessDownloadFile(const string& fname, bool angry) const {
    if (Picsom()->MasterAddress()=="")
      return true;

    if (FileExists(fname))
      return true;

    string msg = "DataBase::RootlessDownloadFile("+fname+") : ";
    WriteLog(msg+"starting");

    string d = fname, dir = fname;
    if (d.find(path)!=0)
      return ShowError(msg+"<"+d+"> does not start with <"+path+">");

    d.erase(0, path.size());

    string url = Picsom()->MasterAddress()+"/database/"+name+d, ctype;
    list<pair<string,string> > hdrs;
    hdrs.push_back(make_pair("X-Auth-Token", Picsom()->MasterAuthToken()));
    int timeout = 50;
    string tmp = Connection::DownloadFile(Picsom(), url, hdrs, ctype, timeout);
    
    if (tmp=="")
      return angry ? ShowError(msg+"downloading <"+url+"> failed") : false;

    size_t p = dir.rfind('/');
    if (p!=string::npos)
      dir.erase(p);

    int mode = 02775;
    if (!Picsom()->MkDirHier(dir, mode))
      return ShowError(msg+"creating directory <"+dir+"> failed");

    if (!Rename(tmp, fname))
      return ShowError(msg+"moving <"+tmp+"> to <"+fname+"> failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::PossiblyDownloadFeatureData(const string& fname) {
    string msg = "DataBase::PossiblyDownloadFeatureData("+fname+") : ";

    if (!IsLocalDiskDb() || !UseBinFeaturesRead() || NoFeatureData())
      return true;

    string binf = ExpandPath("features/"+fname+".bin");
    if (FileExists(binf))
      return true;

    string url   = Connection::GetOpenStackDownloadUrl(Picsom());
    string token = Connection::GetOpenStackAuthToken(Picsom());

    if (url=="" || token=="")
      return ShowError(msg+"url or token not solved");

    string n = Name();
    if (n.find("os://")==0)
      n.erase(0, 5);
    url += "/"+n+"/"+fname+".bin";

    list<pair<string,string> > hdrs;
    hdrs.push_back(make_pair("X-Auth-Token", token));
    string ctype, data = Connection::DownloadFile(Picsom(), url, hdrs, ctype);
    
    if (data=="")
      return ShowError(msg+"downloading <"+url+"> failed");

    int mode = 02775;
    if (!Picsom()->MkDirHier(ExpandPath("features"), mode))
      return ShowError(msg+"creating fetaures directory failed");

    if (!Rename(data, binf))
      return ShowError(msg+"moving <"+data+"> to <"+binf+"> failed");

    WriteLog("Downloaded "+ToStr(FileSize(binf))+" bytes of <"+
	     ShortFileName(binf)+"> from OpenStack");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /*
  string DataBase::SolveSegmentFilePath:(const string& l, const string& m) {
    string sf = db->SolveObjectPath(l);
    if (sf=="")
      return ShowError(msg+"path of object<"+l+"> not solved");

    p = sf.rfind('/');
    if (p==string::npos)
      return ShowError(msg+"no / in path <"+l+">");
    sf.insert(p+1, "segments/"+m+":");
    p = sf.rfind('.');
    if (p!=string::npos)
      sf.erase(p);
    sf += ".seg";

    if (!FileExists(sf))
      return ShowErrorS(msg+"file <"+sf+"> not found");
    
    return sf;
  }
  */

  string DataBase::SolveSegmentFilePath(int idx, const string& m,
					bool existing) const {
    if (m=="")
      return "";

    string vlab = Label(idx);
    
    string pref = m;
    if (*pref.rbegin()!=':')
      pref += ":";

    string sfile = SolveObjectPathClassical(vlab, "segments", pref,
					    false, NULL, false, existing);

    if (existing && !FileExists(sfile))
      return ShowErrorS(string("DataBase::SolveSegmentFilePath() : ")+
                        "segmentation file <"+sfile+
                        "> not found for ["+m+"] and #"+ToStr(idx));
    return sfile;
  }

  /////////////////////////////////////////////////////////////////////////////

  int
  DataBase::PossiblyInsertFaceObjectAndExtractFeatures(int idx,
						       const segmentfile& segf,
						       int frno,
						       const string& name,
						       const list<string>& f) {
    string msg = "DataBase::PossiblyInsertFaceObjectAndExtractFeatures("+
      ToStr(idx)+","+name+","+f.front()+") : ";

    string label = Label(idx), flabel = label+"_"+name;

    int flidx = LabelIndexGentle(flabel, false);
    if (flidx==-1) {
      imagedata img  = ImageData(idx);
      imagedata fimg = ExtractLfwFace(img, segf, frno, name); 
      if (fimg.empty())
	return ShowError(msg+"ExtractLfwFace("+flabel+") failed");
      
      string ffname = TempDir("input")+"/"+flabel+".png";
      try {
	imagefile::write(fimg, ffname);
      } catch (const string& e) {
	return ShowError(msg+"imagefile::write("+ffname+") failed: "+e);
      }
      
      list<upload_object_data> objs;
      objs.push_back(upload_object_data());
      upload_object_data& obj = objs.front();
      obj.path   	    = ffname;
      obj.ctype 	    = "image/png";
      obj.use   	    = "regular";
      obj.forcedlabel       = flabel;
	      
      list<string> detect;
      list<string> segments;
      vector<string> odlist;
      XmlDom xmlx;
      
      if (!InsertObjects(objs, f, detect, segments,
			 false, false, odlist, xmlx, ""))
	return ShowError(msg+"InsertObjects("+flabel+") failed");
      // what about parent-child relation?
	      
      flidx = obj.indices.front();
      
      Unlink(ffname);
    }

    return flidx;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::ExtractLfwFace(int idx) {
    string msg = "DataBase::ExtractLfwFace("+ToStr(idx)+") : ";

    int pidx = ParentObject(idx, false);
    if (pidx<0)
      return imagedata();

    string meth, imgname, fr, seg;
    DataBase::SplitLabel(Label(idx), meth, imgname, fr, seg); 
    if (meth=="")
      return imagedata();

    string segf = SolveSegmentFilePath(pidx, meth, true);
    if (segf=="")
      return imagedata();

    imagedata img = ImageData(pidx);
    picsom::segmentfile segm("", segf);

    int frno = atoi(fr.c_str());

    return ExtractLfwFace(img, segm, frno, seg);
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::ExtractLfwFace(const imagedata& imgin,
				     const segmentfile& seg,
				     int frno, const string& face) {
    string msg = "DataBase::ExtractLfwFace("+ToStr(frno)+","+face+") : ";
    WriteLog(msg+"starting");

    float nw = 112, nh = 112;

    imagedata piece;
    set<int> flist = seg.listProcessedFrames();
    for (auto ff=flist.begin(); ff!=flist.end(); ff++) {
      int f = *flist.begin();
      if (f!=frno)
	continue;

      SegmentationResultList *r = seg.readFrameResultsFromXML(f, "", "");
      for (auto j=r->begin(); j!=r->end(); j++) {
	if (false) {
	  cout << "  NAME:     " << j->name  << endl;
	  cout << "  TYPE:     " << j->type  << endl;
	  cout << "  VALUE:    " << j->value << endl;
	  cout << "  METHODID: " << j->methodid << endl;
	  cout << "  RESULTID: " << j->resultid << endl << endl;
	}
      
	if (j->type!="box" || j->name!=face)
	  continue;

	int tlx, tly, brx, bry;
	if (sscanf(j->value.c_str(), "%d %d %d %d",
		   &tlx, &tly, &brx, &bry)!=4) {
	  ShowError(msg+"failed to interpret \""+j->value+"\"");
	  delete r;
	  return piece;
	}

	int w = brx-tlx+1, h = bry-tly+1;
	float scale = sqrt(float(w)*h/(nw*nh));

	int x0 = (int)floor(tlx-0.5*scale*(250-nw)+0.5);
	int x1 = (int)floor(brx+0.5*scale*(250-nw)+0.5);
	int y0 = (int)floor(tly-0.5*scale*(250-nh)+0.5);
	int y1 = (int)floor(bry+0.5*scale*(250-nh)+0.5);

	imagedata img = imgin;

	try {
	  scalinginfo sinfo(x1-x0+1, y1-y0+1, 250, 250, x0, y0);
	  sinfo.stretch(true);
	  img.rescale(sinfo);

	} catch (const string& e) {
	  ShowError(msg+e);
	  return piece;
	}

	piece = img;
	break;
      }
      
      delete r;
    }

    return piece;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::RecognizeFace(const list<pair<string,FloatVector*>>& fl) {
    string msg = "DataBase::RecognizeFace() : ";

    string facefeat = fl.front().first;
    const FloatVector *ffvec = fl.front().second;

    static list<ground_truth> facegt;
    if (facegt.empty()) {
      list<string> clsnames = SplitClassNames("faces");
      for (auto j=clsnames.begin(); j!=clsnames.end(); j++)
	facegt.push_back(GroundTruthExpression(*j));
    }

    string facename;

    Index *fdbfeat = FindIndex(facefeat);
    VectorIndex *fdbvi = dynamic_cast<VectorIndex*>(fdbfeat);
    if (fdbvi) {
      fdbvi->ReadDataFile();
      fdbvi->SetDataSetNumbers(false, false);
      const FloatVectorSet& dd = fdbvi->Data();
      // cout << dd.Nitems() << " " << dd.VectorLength()
      //      << " " << ffvec->Length() << endl;
      IntVector nn(3);
      FloatVector nnd;
      dd.NearestNeighbors(*ffvec, nn, nnd);
      for (int j=0; j<nn.Length(); j++)
	cout << " " << j << " " << nn[j] << " " << nnd[j]
	     << " <" << dd[nn[j]].Label() << "> #"
	     << dd[nn[j]].Number() << endl;

      size_t hit = dd[nn[0]].Number();
      for (auto g=facegt.begin(); g!=facegt.end(); g++)
	if ((*g)[hit]==1) {
	  facename = g->label();
	  cout << ">>>> " << facename << endl;
	  break;
	}
    }

    return facename;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExtractObjects(const vector<size_t>& idxs, bool subd) {
    string msg = "DataBase::ExtractObjects() : ";
    WriteLog(msg+"starting with "+ToStr(idxs.size())+" objects");

    vector<size_t> vframes, images;

    for (auto i=idxs.begin(); i!=idxs.end(); i++) {
      size_t idx = *i;

      bool ok = true;

      if (IsMissingVideoFrame(idx))
	vframes.push_back(idx);
      
      else if (IsMissingVideoClip(idx))
	ok = ExtractVideoClip(idx, subd);

      else if (IsMissingAudioClip(idx))
	ok = ExtractAudioClip(idx, subd);

      else if (IsMissingImageSegment(idx))
	ok = ExtractImageSegment(idx, subd);

      else if (IsMissingImageFromContainer(idx))
	images.push_back(idx);

      else
	ok = ShowError(msg+"type of #"+ToStr(idx)+" not known");

      if (!ok)
	return false;
    }

    if (vframes.size() && !ExtractVideoFrames(vframes))
      return false;

    if (images.size() && !ExtractImagesFromContainer(images))
      return false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExtractObject(size_t idx, bool use_subdir) {
    string msg = "DataBase::ExtractObject("+ToStr(idx)+") : ";
    WriteLog(msg+"starting");

    if (IsMissingVideoFrame(idx))
      return ExtractVideoFrame(idx, use_subdir);

    if (IsMissingVideoClip(idx))
      return ExtractVideoClip(idx, use_subdir);

    if (IsMissingAudioClip(idx))
      return ExtractAudioClip(idx, use_subdir);

    if (IsMissingImageFromContainer(idx))
      return ExtractImageFromContainer(idx);

    return ShowError(msg+"type not known");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsMissingObject(size_t idx) const {
    return IsMissingVideoClip(idx) ||
      IsMissingAudioClip(idx) || IsMissingVideoFrame(idx) ||
      IsMissingImageFromContainer(idx) || IsMissingObjectInSql(idx) ||
      IsMissingImageSegment(idx);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsMissingObjectInSql(size_t /*idx*/) const {
    if (!SqlObjects())
      return false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsImageFromContainer(size_t idx) const {
    string msg = "IsImageFromContainer("+ToStr(idx)+") : ";

    if (ObjectsTargetType(idx)==target_imagefile)
      return false;

    auto h = ReadOriginsInfo(idx, false, true);
    string url = h["url"];
    if (url=="")
      return ShowError(msg+"empty url");
    //cout << url << endl;

    vector<string> tar_or_recipe = SplitInSomething("|", false, url);
    if (!tar_or_recipe.size())
      return ShowError(msg+"no | in url <"+url+">");

    string tarspec = tar_or_recipe[0];

    if (tarspec.size()>7 && tarspec[tarspec.size()-1]==']' &&
	(tarspec.find(".tar[")   !=string::npos ||
	 tarspec.find(".tar.gz[")!=string::npos ||
	 tarspec.find(".tgz[")   !=string::npos ||
	 tarspec.find(".zip[")   !=string::npos))
      return true;

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsMissingImageFromContainer(size_t idx) const {
    string msg = "IsMissingImageFromContainer("+ToStr(idx)+") : ";

    if (ObjectsTargetType(idx)==target_imagefile)
      return false;

    if (!alwaysusetarfiles) {
      const string& lab = Label(idx);
      string p = SolveObjectPath(lab, "", "", false, NULL, true);
      if (FileExists(p))
	return false;
    }

    return IsImageFromContainer(idx);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExtractFromContainerCommon(size_t idx, string& type, string& cont,
				      string& contesc, string& file) {

    type = cont = file = "";

    if (ObjectsTargetType(idx)==target_imagefile)
      return false;

    auto h = ReadOriginsInfo(idx, false, true);
    string url = h["url"], ext = h["name"];
    size_t p = ext.rfind(".");
    if (p!=string::npos)
      ext.erase(0, p);

    //cout << url << endl;

    vector<string> tar_or_recipe = SplitInSomething("|", false, url);
    string tarspec = tar_or_recipe[0];

    p = tarspec.find(".tar[");
    if (p==string::npos)
      p = tarspec.find(".tar.gz[");
    if (p==string::npos)
      p = tarspec.find(".tgz[");
    if (p!=string::npos)
      type = "tar";
    
    if (p==string::npos) {
       p = tarspec.find(".zip[");
       if (p!=string::npos)
	 type = "zip";
    }

    if (p==string::npos || p<1 || tarspec[tarspec.size()-1]!=']')
      return false;

    size_t pp = tarspec.find('[', p);
    cont  = tarspec.substr(0, pp);
    file = tarspec.substr(pp+1);
    if (file.size())
      file.erase(file.size()-1);

    //cout << "[" << cont << "] [" << file << "]" << endl;

    contesc = cont;
    for (;;) {
      bool found = false;
      size_t p = contesc.find("://");
      if (p!=string::npos) {
	found = true;
	contesc.replace(p, 3, "___");
      }
      p = contesc.find_first_of("+&?;\\\"'()[]{}!~");
      if (p!=string::npos) {
	found = true;
	contesc.replace(p, 1, "_");
      }
      if (!found)
	break;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExtractImagesFromContainer(const vector<size_t>& idxs) {
    string msg = "DataBase::ExtractImagesFromContainer() : ";

    bool extractfull = extractfulltars;
    if (idxs.size()) {
      auto h = ReadOriginsInfo(idxs[0], false, true);
      string url = h["url"];
      if (url.find(".zip[")!=string::npos || url.find(".ZIP[")!=string::npos)
	extractfull = extractfullzips;
    }

    if (extractfull) {
      for (auto i=idxs.begin(); i!=idxs.end(); i++)
	if (!ExtractImageFromContainer(*i))
	  return false;
      return true;
    }

    map<string,list<pair<size_t,string> > > files;

    bool is_zip = false;

    for (auto i=idxs.begin(); i!=idxs.end(); i++) {
      string type, cont, contesc, file;
      if (!ExtractFromContainerCommon(*i, type, cont, contesc, file))
	return false;

      is_zip = type=="zip";
#ifndef HAVE_ZIP_H
      if (is_zip)
	return ShowError(msg+"cannot handle zip");
#endif // HAVE_ZIP_H

      if (cont!=contesc)
	return ShowError(msg+"cannot handle cont!=contesc");

      if (file=="" || cont=="")
	return ShowError(msg+"empty cont or file");
      
      files[cont].push_back(make_pair(*i, file));
    }

    if (files.empty())
      return ShowError(msg+"no files found");

    string tdir = TempDir("tars_extracted_partial");

    for (auto i=files.begin(); i!=files.end(); i++) {
      string contf_o = path+"/tars/"+i->first, contfile = contf_o;
      if (is_zip && uselocalzipfiles)
	contfile = CopyFileFromGlobalToLocalDisk(contf_o);

      string ttdir = tdir+"/"+i->first;
      Picsom()->MkDirHier(ttdir, 0777);

      bool done = false;
#ifdef HAVE_ZIP_H
      if (is_zip) {
#if (LIBZIP_VERSION_MAJOR>0)
        zip_error_t err;
	zip_error_init(&err);
#endif // LIBZIP_VERSION_MAJOR>0
	int ierr = 0;
        auto z = zipfiles.find(contfile);
	if (z==zipfiles.end()) {
	  int flags = 0;
#ifdef ZIP_RDONLY
	  flags |= ZIP_RDONLY;
#endif // ZIP_RDONLY

          zip_t *x = zip_open(contfile.c_str(), flags, &ierr);
	  if (!x) {
#if (LIBZIP_VERSION_MAJOR>0)
            zip_error_init_with_code(&err, ierr);
            const char *es = zip_error_strerror(&err);
#else
	    char errtmp[1000];
	    zip_error_to_str(errtmp, sizeof errtmp, ierr, errno);
	    const char *es = errtmp;
#endif // LIBZIP_VERSION_MAJOR>0
	    return ShowError(msg+"zip_open("+contfile+") failed: "+es);
          }
          zipfiles[contfile] = x;
          z = zipfiles.find(contfile);
        }
	for (auto j=i->second.begin(); j!=i->second.end(); j++) {
	  struct timespec start = TimeNow();
          zip_file_t *zf = zip_fopen(z->second, j->second.c_str(), 0);
	  if (zf==NULL) {
#if (LIBZIP_VERSION_MAJOR>0)
            zip_error_t *ep = zip_get_error(z->second);
            const char *es = zip_error_strerror(ep);
#else
	    const char *es = zip_strerror(z->second);
#endif // LIBZIP_VERSION_MAJOR>0
	    return ShowError(msg+"zip_fopen("+contfile+","+j->second+
			     ") failed: "+es);
          }
	  zip_stat_t st;
	  zip_stat_init(&st);
	  if (zip_stat(z->second, j->second.c_str(), 0, &st)) {
#if (LIBZIP_VERSION_MAJOR>0)
            zip_error_t *ep = zip_get_error(z->second);
            const char *es = zip_error_strerror(ep);
#else
	    const char *es = zip_strerror(z->second);
#endif // LIBZIP_VERSION_MAJOR>0
	    return ShowError(msg+"zip_stat("+contfile+","+j->second+
			     ") failed: "+es);
          }
	  string str(st.size, ' ');
	  size_t n = zip_fread(zf, &str[0], st.size);
	  if (n!=st.size) {
	    return ShowError(msg+"zip_fread("+contfile+","+j->second+
			     ") failed: "+ToStr(n)+"!="+ToStr(st.size));
          }
	  zip_fclose(zf);
	  string dst = ttdir+"/"+j->second, dstd = dst;
	  size_t p = dstd.rfind('/');
	  if (p!=string::npos)
	    dstd.erase(p);
	  Picsom()->MkDirHier(dstd, 0777);
	  if (!StringToFile(str, dst)) {
	    return ShowError(msg+"storing in <"+dst+"> failed");
	  }
	  double took = TimeDiff(TimeNow(), start);
	  WriteLog(msg+"extracted <"+j->second+"> from <"+z->first
		   +"> in "+ToStr(1000*took)+" ms");
        }	
        done = true;
      }
#endif // HAVE_ZIP_H

      if (!done) {
        string cmd = "cd "+ttdir+" && tar xf "+contfile;

	for (auto j=i->second.begin(); j!=i->second.end(); j++)
	  cmd += " "+j->second;

	if (Picsom()->ExecuteSystem(cmd, true, true, true))
	  return ShowError(msg+"ExecuteSystem() failed");
      }
      
      for (auto j=i->second.begin(); j!=i->second.end(); j++) {
	string z = ttdir+"/"+j->second;
	string r = InsertedObjectPath(j->first, false, false, false);
	string d = ConvertGlobalToLocalDiskName(r);
	// cout  << "[" << z << "] [" << r << "] [" << d << "]" << endl;

	if (!Picsom()->MakeDirectory(d, true))
	  return ShowError(msg+"MakeDirectory("+d+") failed");
	
	if (!Rename(z, d))
	  return ShowError(msg+"Rename("+z+","+d+") failed");

	extracted_files = true;
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExtractImageFromContainer(size_t idx) {
    string msg = "DataBase::ExtractImageFromContainer() : ";

    string type, tar, taresc, file;
    if (!ExtractFromContainerCommon(idx, type, tar, taresc, file))
      return false;

    bool is_zip = type=="zip";
    
    // if (ObjectsTargetType(idx)==target_imagefile)
    //   return false;

    // auto h = ReadOriginsInfo(idx, false, true);
    // string url = h["url"], ext = h["name"];
    // size_t p = ext.rfind(".");
    // if (p!=string::npos)
    //   ext.erase(0, p);

    // //cout << url << endl;

    // vector<string> tar_or_recipe = SplitInSomething("|", false, url);
    // string tarspec = tar_or_recipe[0];

    // bool is_zip = false;
    // p = tarspec.find(".tar[");
    // if (p==string::npos)
    //    p = tarspec.find(".tar.gz[");
    // if (p==string::npos)
    //    p = tarspec.find(".tgz[");
    // if (p==string::npos) {
    //    p = tarspec.find(".zip[");
    //    if (p!=string::npos)
    // 	 is_zip = true;
    // }

    // if (p==string::npos || p<1 || tarspec[tarspec.size()-1]!=']')
    //   return false;

    // size_t pp = tarspec.find('[', p);
    // string tar  = tarspec.substr(0, pp), tarkey = tar;
    // string file = tarspec.substr(pp+1);
    // if (file.size())
    //   file.erase(file.size()-1);

    // //cout << "[" << tar << "] [" << file << "]" << endl;

    // string taresc = tar;
    // for (;;) {
    //   bool found = false;
    //   size_t p = taresc.find("://");
    //   if (p!=string::npos) {
    // 	found = true;
    // 	taresc.replace(p, 3, "___");
    //   }
    //   p = taresc.find_first_of("+&?;\\\"'()[]{}!~");
    //   if (p!=string::npos) {
    // 	found = true;
    // 	taresc.replace(p, 1, "_");
    //   }
    //   if (!found)
    // 	break;
    // }

    string tarkey = tar;

    string tdir = TempDir("tars_extracted");
    string ttdir = tdir+"/"+taresc;
    string z = ttdir+"/"+file;

    if (tars_extracted.find(tarkey)==tars_extracted.end()) {
      Picsom()->MkDirHier(ttdir, 0777);

      string tarfile = path+"/tars/"+tar;
      bool tarfile_is_temp = false;

      list<pair<string,string> > hdrs;
      if (tar.find("url://")==0) {
	string n = tar.substr(6), r;
	size_t p = n.find('/');
	if (p!=string::npos) {
	  r = n.substr(p);
	  n.erase(p);
	}
	if (0)
	  cout << "<<<" << n << ">>> <<<" << r << ">>>" << endl;
	if (url_rules.find(n)!=url_rules.end()) {
	  string newtar = url_rules[n]["address"]+r;
	  if (0)
	    cout << "<<<||" << newtar << "||>>>" << endl
		 << "Mapped <" << tar << "> to <" << newtar << ">"
		 << endl;
	  tar = newtar;
	  for (auto a=url_rules[n].begin(); a!=url_rules[n].end();
	       a++)
	    if (a->first=="openstack") {
	      string token = Connection::GetOpenStackAuthToken(Picsom(),
							       a->second);
	      if (token=="")
		return ShowError(msg+"getting OpenStack token for \""+n+
				 "\" failed");

	      hdrs.push_back(make_pair("X-Auth-Token", token));

	    } else if (a->first!="address")
	      hdrs.push_back(*a);
	}
      }

      if (tar.find("http://")==0 || tar.find("https://")==0 ||
	  tar.find("ftp://")==0) {
	string ctype, file;
	file = Connection::DownloadFile(Picsom(), tar, hdrs, ctype);
	if (file=="")
	  return ShowError(msg+"downloading <"+tar+"> failed");

	if (0)
	  cout << "Downloaded in <" << file << ">" << endl;
	tarfile = file;
	tarfile_is_temp = true;
      }

      string exe = is_zip ? "unzip -q" : "tar xf";
      string cmd = "cd "+ttdir+" && "+exe+" "+tarfile;
      //cmd = "echo hello && echo world";
      if (Picsom()->ExecuteSystem(cmd, true, true, true))
	return false;

      if (tarfile_is_temp)
	Unlink(tarfile);

      tars_extracted.insert(tarkey);
    }

    string r = InsertedObjectPath(idx, false, false, false);
    string d = ConvertGlobalToLocalDiskName(r);
    // cout  << "[" << z << "] [" << r << "] [" << d << "]" << endl;

    if (!Picsom()->MakeDirectory(d, true))
      return false;

    if (!Rename(z, d))
      return false;

    extracted_files = true;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExtractVideoFrames(const vector<size_t>& idxs) {
    string msg = "DataBase::ExtractVideoFrames() : ";

    bool debug = videofile::debug() || imagefile::debug_impl();

    // these are now members with these defaults
    // bool tempmediafiles = true, tempimagefiles = true;

    if (debug)
      cout << msg << ToStr(idxs) << endl;

    map<size_t,set<size_t> > parch;

    for (auto i=idxs.begin(); i!=idxs.end(); i++) {
      if (ObjectsTargetType(*i)!=target_image)
	return ShowError(msg+"object's #"+ToStr(*i)+" target type is"
			 " not image but "+ObjectsTargetTypeString(*i));
      int p = ParentObject(*i, false);
      if (p<0)
	return ShowError(msg+"object's #"+ToStr(*i)+" does not have parent");

      target_type tt = PicSOM::TargetTypeFileFullMasked(ObjectsTargetType(p));

      if (tt!=target_video && tt!=target_imageset)
	return ShowError(msg+"object's #"+ToStr(*i)+" parent's target type is"
			 " not video/imageset but "+ObjectsTargetTypeString(p));
      
      parch[p].insert(*i);
    }

    for (auto i=parch.begin(); i!=parch.end(); i++) {
      bool first = true;
      imagefile img;
      img.frame_cacheing(false); // added 2013-09-16
      target_type ptt = PicSOM::TargetTypeFileMasked(ObjectsTargetType(i->first));
      
      for (auto j=i->second.begin(); j!=i->second.end(); j++) {
	const string& label = Label(*j);
	string parobj, timepoint, duration;

	if (ptt==target_video &&
	    !SolveObjectSubrange(label, target_image,
				 parobj, timepoint, duration))
	return ShowError(msg+"SolveObjectSubrange() failed");

	if (ptt==target_imageset)
	  parobj = ObjectPathEvenExtract(i->first, true);
	
	map<string,string> ohash = ReadOriginsInfo(*j, false, true);
	int fno = -1;
	string url = ohash["url"];
	size_t p = url.find('[');
	if (p!=string::npos)
	  fno = atoi(url.substr(p+1).c_str());

	if (debug)
	  cout << "  #" << *j << " <" << label << "> \"" << parobj
	       << "\" \"" << timepoint << "\" \"" << duration << "\" "
	       << fno << endl;
	
	if (fno<0)
	  return ShowError(msg+"couild not solve frame number");

	if (tempmediafiles)
	  parobj = CopyFileFromGlobalToLocalDisk(parobj);

	if (first) {
	  first = false;
	  img.open(parobj);
	}

	// this is not fully compatible with ExtractMediaClip()...

	string filename = ohash["filename"];
	p = filename.rfind('/');
	if (p!=string::npos)
	  filename.erase(0, p+1);
	p = filename.find('.');
	string ext = p!=string::npos ? filename.substr(p) : "";
	string subdir = ""; // SqlObjects() ? "" : LabelSubDir(label);
	string opath = SolveObjectPath(label, subdir, "",
				       true, NULL, true)+ext;
	string destname = opath;
	if (tempimagefiles)
	  destname = ConvertGlobalToLocalDiskName(destname);
	
	Picsom()->MakeDirectory(destname, true);

	imagedata idat = img.frame(fno);
	imagefile::write(idat, destname);

	WriteLog("Extracted "+ToStr(FileSize(destname))+" bytes for image <"+
		 label+"> from <"+ShortFileName(parobj)+"> to <"+
		 ShortFileName(destname)+">");

	AddCreatedFile(destname);
	extracted_files = true;
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ConvertGlobalToLocalDiskName(const string& rin) {
    if (IsLocalDiskDb())
      return rin;

    string td = TempDir("");
    if (rin.substr(0, td.size())==td)
      return rin;
    
    string r = rin;
    r.erase(0, path.size());
    string d = TempDir("")+r;
    return d;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::CopyFileFromGlobalToLocalDisk(const string& src) {
    string msg = "DataBase::CopyFileFromGlobalToLocalDisk() : ";

    string dst = ConvertGlobalToLocalDiskName(src);

    if (!FileExists(dst)) {
      if (!Picsom()->MakeDirectory(dst, true))
	  ShowError(msg+"failed to create directory for <"+dst+">");

      struct timespec start = TimeNow();
      if (!CopyFile(src, dst))
	ShowError(msg+"failed to copy <"+src+"> to <"+dst+">");
      else {
	off_t fsize = FileSize(dst);
	double took = TimeDiff(TimeNow(), start), speed = fsize/took;
	WriteLog("Copied <"+src+"> to <"+dst+"> "+ToStr(fsize)+" bytes ("
		 +FileSizeHumanReadable(dst)+") in "+ToStr(took)
		 +" s ("+HumanReadableBytes(speed)+"/s)");
      }
      // obs! commented out 2018-10-04 to avoid repeated copies...
      // AddCreatedFile(dst);
    }

    return dst;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXML(XmlDom& dom, object_type ot,
			const string& namestr, const string& specstr,
			/*const*/ Query *q, const Connection *conn) {
  bool ok = true;

  switch (ot) {
  case ot_databaseinfo:
    ok = ok && AddToXMLdatabaseinfo(dom, specstr, conn, true, true);
    break;

  case ot_featurelist:
    ok = ok && AddToXMLfeaturelist(dom, q);
    break;

  case ot_object:
    ok = ok && AddToXMLobject(dom, namestr, specstr);
    break;

  case ot_thumbnail:
    ok = ok && AddToXMLthumbnail(dom, namestr, specstr);
    break;

  case ot_message: 
    ok = ok && AddToXMLmessage(dom, namestr, specstr);
    break;

  case ot_plaintext: 
    ok = ok && AddToXMLplaintext(dom, namestr, specstr);
    break;

  case ot_objectinfo:
    ok = ok && AddToXMLobjectinfo(dom, namestr, q, false,
				  true, true, true, NULL, specstr);
    if (q) {
      if (specstr.find("novariables")==string::npos)
	ok = ok && q->AddToXMLvariables(dom, conn);
    } else if (specstr.find("nodbinfo")==string::npos)
      ok = ok && AddToXMLdatabaseinfo(dom, specstr, conn, false, true);
    break;

  case ot_contentitems:
    ok = ok && AddToXMLcontentitemlist(dom);
    break;

  case ot_segmentationinfo:
    ok = ok && AddToXMLdatabaseinfo(dom, specstr, conn, false, true);
    ok = ok && AddToXMLsegmentationinfo(dom, namestr, specstr);
    break;

  case ot_segmentationinfolist:
    ok = ok && AddToXMLsegmentationinfolist(dom, namestr);
    break;

  case ot_segmentimg:
    ok = ok && AddToXMLsegmentimg(dom, namestr, specstr);
    break;

  case ot_contextstate:
    ok = ok && AddToXMLcontextstate(dom, namestr, specstr);
    break;

  default:
    ShowError("DataBase::AddToXML() unknown objecttype: ",
	      ObjectTypeP(ot, true));
    ok = false;
  }
    
  return ok;
}  

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLdata(XmlDom& xml, object_type type,
                              const string& label, const string& subdir,
                              bool /*to_root*/, bool /*use_d_dir*/) {
    if (!IsLabel(label))
      return ShowError("DataBase::AddToXML(label=<", label, ">) is not proper");

    string op = SolveObjectPath(label, subdir);
    if (op=="") {
      stringstream tmp;
      tmp << "label=<" << label<< "> subdir=<" << subdir << ">";
      return ShowError("DataBase::AddToXML(", tmp.str(),
                       ") SolveObjectPath() failed");
    }

    struct stat st;
    if (stat(op.c_str(), &st))
      return ShowError("DataBase::AddToXML(): stat(", op ,") failed");

    string buf = FileToString(op);
    if ((int)buf.size()!=st.st_size) {
      stringstream tmp;
      tmp << "label=<" << label << "> subdir=<" << subdir << "> op=<" << op
          << "> stat=" << st.st_size << " read=" << buf.size();
      return ShowError("DataBase::AddToXML(): read size and stat size differ: ",
                       tmp.str());
    }

    string fname = op;
    size_t p = fname.rfind('/');
    if (p!=string::npos)
      fname.erase(0, p+1);

    string typestr = ObjectTypeP(type);
    bool persists = true;
    return Connection::AddToXMLdata(xml, typestr, FileExtensionToMIMEtype(op),
                                    buf, persists, label, fname);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLvirtual(XmlDom& xml, object_type ot,
                                 const string& name, const string& spec) {

    if (debug_tn && ot==ot_thumbnail)
      cout << "Now in DataBase::AddToXMLvirtual(tn," << name << "," << spec
           << ")" << endl;

    // here should be a test that object really is image...
    if (ot!=ot_thumbnail && ot!=ot_object)
      return ShowError("DataBase::AddToXMLvirtual(", name, ",", spec,
                       ") ot!=thumbnail && ot!=object");

    string otstr = ObjectTypeP(ot);

    stringstream png;
    bool persists = true;
    if (CreateVirtualImage(png, name, spec))
      return Connection::AddToXMLdata(xml, otstr, "image/png", png,
                                      persists, name);

    return ShowError("DataBase::AddToXMLvirtual(", name, ",", spec,
                     ") CreateVirtualImage() failed");
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLthumbnail(XmlDom& xml, const string& name_in,
                                 const string& sp) {
  // Tic("DataBase::AddToXMLthumbnail");

  if (debug_tn)
    cout << "Now in DataBase::AddToXMLthumbnail(" << name_in
	 << "," << sp << ")" << endl;

  string size = sp;
  if (size=="") {
    size = FindThumbnailSizes();
    if (size!="") {
      size_t c = size.find(',');
      if (c!=string::npos)
	size.erase(c);
    }
  }
  if (size=="")
    size = LastResortThumbnailSize();
  
  int lbindex = LabelIndexGentle(name_in);
  if (lbindex == -1) 
    return false;

  string pre, suf, tn_type = ThumbnailType();
  size_t bp = tn_type.find(":_");
  if (bp!=string::npos) {
    pre = tn_type.substr(0, bp+1);
    suf = tn_type.substr(bp+1);
  }

  string name = pre+name_in+suf, tndir = "tn-"+size;

  bool ok = IsVirtualImage(name) ?
    AddToXMLvirtual(xml, ot_thumbnail, name, size) :
    TryToAddToXMLthumbnail(xml, tndir, name);

  if (!ok && ThumbnailType()=="create") 
    ok = InsertImageThumbnails(SolveObjectPath(name));

  if (!ok) {
    string fnp = LabelFileNamePart(name);

    // try to add thumbnail from parent
    if (name!=fnp)
      ok = TryToAddToXMLthumbnail(xml, tndir, fnp);

    if (!ok) {
      // try to add thumbnail from subobjects
      int idx = LabelIndex(name);
      const vector<int>& so = SubObjects(idx);
      for (size_t i=0; !ok && i<so.size(); i++)
	ok = TryToAddToXMLthumbnail(xml, tndir, Label(so[i]));

      for (size_t i=0; !ok && i<so.size(); i++)
	ok = AddToXMLvirtual(xml, ot_thumbnail, Label(so[i]), size);

      // try to add thumbnail from parents subobjects
      if (!ok && ParentObject(idx)!=-1) {
	const vector<int>& sop = SubObjects(ParentObject(idx));
	for (size_t i=0; !ok && i<sop.size(); i++)
	  ok = TryToAddToXMLthumbnail(xml, tndir, Label(sop[i]));
      }
    }
  }

  if (!ok)
    ShowError("Thumbnail for <", name, "> not found");

  // Tac("DataBase::AddToXMLthumbnail");
  
  return ok;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::TryToAddToXMLthumbnail(XmlDom& xml, const string& dir,
                                        const string& name) {
    if (debug_tn)
      cout << "DataBase::TryToAddToXMLthumbnail(" << dir << "," << name << ")"
           << endl;
  
    string op = SolveObjectPath(name, dir);
    return op=="" ? false : AddToXMLdata(xml, ot_thumbnail, name,
                                         dir, false, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLmessage(XmlDom& xml, const string& name,
                                 const string& /*spec*/) {
    // Tic("DataBase::AddToXMLmessage");

    bool ok = true;
    object_type ot = ot_message;

    ok = ok && AddToXMLdata(xml, ot, name, "", true, false);

    // Tac("DataBase::AddToXMLmessage");
 
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLplaintext(XmlDom& xml, const string& namestr,
                                   const string& /*spec*/) {
    // Tic("DataBase::AddToXMLplaintext");

    const char *name = namestr.c_str();

    bool ok = true;
    object_type ot = ot_plaintext;

    char subdir[11];
    subdir[0] = 0;
    size_t ll = LabelLength();
    if (strlen(name)>=ll) {
      strncat(subdir, name, ll);
      strcat(subdir, ".d");
    }
    else
      return false;
    
    ok = ok && AddToXMLdata(xml, ot, name, subdir, false, false);

    // Tac("DataBase::AddToXMLplaintext");
 
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLobject(XmlDom& xml, const string& label,
                                const string& sp) {
    string spx = allowimagescaling ? sp : "";

    bool is_virtual = IsVirtualImage(label);
    if (!is_virtual && spx!="") {
      int idx = LabelIndexGentle(label);
      if (idx>=0) {
        target_type ttype = ObjectsTargetType(idx);
        if (ttype&target_video) {              // we'll now assume that types 
          WarnOnce("Videos cannot be scaled"); // other than videos can be scaled
          spx = "";
        } else
          is_virtual = true;        
      }
    }

    if (is_virtual)
      return AddToXMLvirtual(xml, ot_object, label, spx!=""?spx:"400x0");

    int idx = LabelIndex(label);

    if (IsMissingObject(idx) && !ExtractObject(idx))
      return ShowError("DataBase::AddToXMLobject() : ExtractObject("+label+
                       ") failed");

    return AddToXMLdata(xml, ot_object, label, "", false, true);

    // this used to try also:
    //   AddToXMLdata(xml, ot_object, label, "", true, false);
    // but it sould not make any difference, would it?
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsMediaClip(size_t idx,
			     target_type ott, target_type ptt) const {
    if (!mediaclips) // obs! this bad, should check if we could get rid of it
      return false;

    if (!(ObjectsTargetType(idx)&ott))
      return false;
 
    // so this relies only on the _first_ parent...
    for (int pidx=ParentObject(idx); pidx>=0; pidx = ParentObject(pidx)) {
      // if we have a sound, the mpeg7 video is two steps up the tree...
      // if (ott&target_sound)
      //   pidx = ParentObject(pidx);

      if (pidx>=0 && ObjectsTargetType(pidx)&target_message)
	return true;

      if (pidx>=0 && ott==target_imagesegment && ptt==target_image &&
	  (ObjectsTargetType(idx)&target_imagesegment)==target_imagesegment&&
	  (ObjectsTargetType(pidx)&target_imagesegment)==target_image)
	return true; // added 2018-01-09

      if (pidx>=0 && ott==target_image && ptt==target_imageset)
	return true; // added 2018-10-12

      if (pidx<0 || !(ObjectsTargetType(pidx)&target_video))
	return false;

      // commented out 2013-04-09
      // string p = SolveObjectPath(Label(pidx));
      // if (!FileExists(p))
      //   return false;

      string xml = Mpeg7MCXML(pidx);
      if (FileExists(xml))
	return true;

      map<string,string> obj_hash = ReadOriginsInfo(idx, false, true);
      string subrange = obj_hash["subrange"];
      if (subrange=="")
	return false;

      map<string,string> par_hash = ReadOriginsInfo(pidx, false, true);

      string pageurl = obj_hash["pageurl"];
      string parfile = par_hash["url"];
      string parname = par_hash["name"];

      if (pageurl==parfile)
	return true;

      string url = obj_hash["url"];
      size_t q = url.rfind('[');
      if (q==string::npos)
	return false;
      url.erase(q);
      q = url.rfind('/');
      if (q!=string::npos)
	url.erase(0, q+1);
    
      if (url==parname || url==Label(pidx))
	return true;
    }
	   
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsMissingMediaClip(size_t idx, target_type tt) const {
    bool empty_is_missing = true;

    int pidx = ParentObject(idx);
    if (pidx==-1)
      return false;

    if (!IsMediaClip(idx, tt, ObjectsTargetType(pidx)))
      return false;
    
    string labstr = Label(idx);

    string p = SolveObjectPath(labstr, LabelSubDir(labstr), "", 
			       false, NULL, true);
    
    return !FileExists(p) || (empty_is_missing && FileSize(p)==0);
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLsegmentimg(XmlDom& xml, const string& name,
                                    const string& spec) const {
    string s = spec;
    vector<string> p = SplitInSomething(":", false, s);
    if (p.size()==3)
      s = p[1]+":"+p[2];

    imagedata imgd = CreateSegmentImage(name, s);
    if (imgd.iszero()) //isempty!
      return ShowError("DataBase::AddToXMLsegmentimg() : CreateSegmentImage()"
                       " failed");

    int width = 120, height = 90;
    if (sscanf(p[0].c_str(), "%dx%d", &width, &height)!=2)
      ShowError("DataBase::AddToXMLsegmentimg() sscanf(wxh) failed");

    if (!width || !height)
      return ShowError("DataBase::AddToXMLsegmentimg() : <"+name+
                       "> width="+ToStr(width), 
                       " height="+ToStr(height), " from ["+spec+"]");

    scalinginfo scale(imgd.width(), imgd.height(), width, height);  
    imgd.rescale(scale);

    bool persists = true;
    return !imgd.iszero() && //!isempty
      Connection::AddToXMLimage(xml, ObjectTypeP(ot_segmentimg),
                                imgd, name, persists);
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateSegmentImage(const string& name,
					 const string& spec) const {

    const string msg = "DataBase::CreateSegmentImage(" + name
      + "," + spec + ") : ";

    if (DebugVirtualObjects())
      cout << "Now in " << msg << endl;

    imagedata empty;

    char vismethod = 'd';
    string segmspec;

    vector<string> p = SplitInSomething(":", false, spec);

    if (p.size()>0 && p[0]!="")
      segmspec = p[0];

    if (p.size()>1 && p[1]!="")
      vismethod = p[1][0];

    string method, label, frame, segm;
    bool ok = SplitLabel(name, method, label, frame, segm);
    if (!ok) {
      ShowError(msg, "error in SplitLabel()");
      return empty;
    }

    if (segm!="")
      segmspec = segm;

    // check some of the components
  
    if (!IsLabel(label)) {
      ShowError(msg, "label <", label, "> is not proper");
      return empty;
    }

    string ip = SolveObjectPath(label);
    if (ip=="") {
      ShowError(msg, " image file for label=<", label, "> doesn't exist");
      return empty;
    }

    string vismethods = "mdhcbx";
    if (vismethods.find(vismethod)==string::npos) {
      ShowError(msg, "vismethod not any of ", vismethods);
      return empty;
    }

    //  cout << "Parsed segmentimg name " << name << endl;
    //  cout << "label=" <<label << " method="<<method<<endl;
    //  cout << "segmspec="<<segmspec<<" vismethod="<<vismethod<<endl;

    vector<int> intsegms;

    if (segmspec=="n") {
      intsegms.push_back(-1); // denotes all the nonzero segms
      segmspec = "";
    }

    for (size_t pos=0; pos<segmspec.size(); pos++)
      if (segmspec[pos]=='+')
	segmspec[pos] = ',';

    vector<string> strsegms = SplitInCommas(segmspec);
  
    if (strsegms.empty() && intsegms.empty()) {
      ShowError(msg, "no segments specified");
      return empty;
    }

    for (size_t i=0; i<strsegms.size(); i++)
      if (strsegms[i].find_first_not_of("0123456789")==string::npos) {
	int a;
	stringstream(strsegms[i]) >> a;
	intsegms.push_back(a);
      }

    if (!strsegms.empty() && !intsegms.empty() &&
	intsegms.size()!=strsegms.size()) {
      ShowError(msg, "string and integer segms mixed");
      return empty;
    }

    string sp = SolveObjectPath(label, "segments", method+":");

    if (sp=="" && method.find("zo")!=0) {
      ShowError(msg, " segment file for label=<", label, ">, method=<",
		method, "> doesn't exist");
      return empty;
    } 

    if (intsegms.empty())
      return CreateSegmentImage(ip, sp, strsegms, vismethod);

    return CreateSegmentImage(ip, sp, intsegms, vismethod, method);
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateSegmentImage(const string& ipath,
					 const string& spath,
					 const vector<int>& segs,
					 char vismethod,
					 const string& method) const {

    const string msg = "DataBase::CreateSegmentImage(vector<int>) : ";

    if (DebugVirtualObjects())
      cout << "Now in " << msg << endl;
    
    picsom::segmentfile segf(ipath, spath);

    if (spath=="")
      segf.prepareZoning(method.substr(2));

    segf.parseRegionsFromXML();

    const vector<Region*> *region_spec_ptr = segf.regionCount() ?
      &segf.regionSpec() : NULL;

    imagedata *imgd = NULL;

    switch (vismethod) {
    case 'm': 
      imgd = segf.getDimmedImage(&segs, NULL, 0);
      break;
    case 'd':
      imgd = segf.getDimmedImage(&segs, NULL, 0.5, region_spec_ptr);
      break;
    case 'h':
      imgd = segf.getHueMasking(&segs, NULL, 0);
      break;
    case 'x':
      imgd = segf.getCrossedImage(&segs, 1, 6);
      break;
    case 'c':
      imgd = segf.getColouring(&segs);
      break;
    case 'b':
      imgd = segf.markBoundaries(&segs);
      break;
    }

    imagedata img = *imgd;
    delete imgd;

    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateSegmentImage(const string& ipath,
					 const string& spath,
					 const vector<string>& segs,
					 char /*vismethod*/) const {

    const string msg = "DataBase::CreateSegmentImage(vector<string>) : ";

    if (segs.size()!=1) {
      ShowError(msg+"cannot handle segs.size()!=1");
      return imagedata();
    }

    if (DebugVirtualObjects())
      cout << "Now in " << msg << endl;

    string segspec = segs[0];
    imagedata img = Feature::ExtractSegment(ipath, spath, segspec);

    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateImageCollage(const list<string>& labset,
					 const string& imagespec,
					 size_t ncols, int boxw, int boxh,
					 const imagedata& b, float c) {
    Tic("CreateImageCollage(list)");

    string spec = imagespec;
    list<imagedata> img;
    size_t maxw = 0, maxh = 0, n = 0;
    for (list<string>::const_iterator o=labset.begin(); o!=labset.end(); o++) {
      imagedata i(0, 0, 3, imagedata::pixeldata_uchar);
      if (*o!="") {
	i = CreateImage(*o, spec);
	i.convert(imagedata::pixeldata_uchar);
	i.force_three_channel();
      }
      cout << n++ << "/" << labset.size() << " <" << *o << ">" << endl;
      img.push_back(i);
      if (i.width()>maxw)
	maxw = i.width();
      if (i.height()>maxh)
	maxh = i.height();
    }

    if (!maxw || !maxh)
      return imagedata(0, 0);

    size_t nrows = (size_t)ceil(labset.size()/double(ncols));
    if (boxw<0 && boxh<0) {
      boxw = maxw-boxw;
      boxh = maxh-boxh;
    }

    imagedata white(1, 1, 1);
    white.set(0, 0, float(c));
    imagedata out = b.width() ? b : white;

    scalinginfo os(out.width(), out.height(), ncols*boxw, nrows*boxh);
    os.stretch(true);
    out.rescale(os);
    out.convert(imagedata::pixeldata_uchar);
    out.force_three_channel();

    size_t x = 0, y = 0;
    for (list<imagedata>::const_iterator i=img.begin(); i!=img.end();
	 i++) {
      if (i->width()) {
	size_t x0 = (boxw-i->width())/2, y0 = (boxh-i->height())/2;
	out.copyAsSubimage(*i, x0+x*boxw, y0+y*boxh);
	cout << " X";
      } else
	cout << " -";
      x++;
      if (x==ncols||y*ncols+x==img.size()) {
	x = 0;
	y++;
	cout << endl;
      }
    }

    Tac("CreateImageCollage(list)");

    return out;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateImageCollage(const XmlDom& xmlin,
                                         DataBase* db, Query* q) {
    string msg = "DataBase::CreateImageCollage() : ";

    //Tic("CreateImageCollage(xml)");

    imagedata err(0, 0);

    XmlDom root = xmlin.NodeOK() ? xmlin : xmlin.Root();

    if (root.NodeName()!="collage") {
      ShowError(msg+"root is not <collage>");
      return err;
    }

    XmlDom frameset = root.FirstChild();
    while (frameset && !frameset.IsElement())
      frameset = frameset.Next();
    if (frameset.NodeName()!="frameset") {
      ShowError(msg+"no <frameset> found");
      return err;
    }
    
    string wi = frameset.Property("width");
    string he = frameset.Property("height");
    string bw = frameset.Property("border");
    string bg = frameset.Property("background");
    string bc = frameset.Property("bordercolor");

    size_t fs_width  = atoi(wi.c_str());
    size_t fs_height = atoi(he.c_str());
    size_t fs_border = atoi(bw.c_str());

    imagecolor<float> fs_bg(bg!=""?bg:"white");
    imagecolor<float> fs_bc(bc!=""?bc:"black");

    for (XmlDom frame = frameset.FirstChild(); frame; frame = frame.Next()) {
      while (frame && !frame.IsElement())
	frame = frame.Next();
      if (frame.NodeName()!="frame") {
	ShowError(msg+"no <frame> found");
	return err;
      }
      
      wi = frame.Property("width");
      he = frame.Property("height");
      bw = frame.Property("border");
    
      size_t f_width  = wi!="" ? atoi(wi.c_str()) : fs_width;
      size_t f_height = he!="" ? atoi(he.c_str()) : fs_height;
      size_t f_border = bw!="" ? atoi(bw.c_str()) : fs_border;

      if (!f_width || !f_height) {
	ShowError(msg+"no width or height given");
	return err;
      }
      
      bg = frame.Property("background");
      bc = frame.Property("bordercolor");

      imagecolor<float> f_bg = bg!="" ? imagecolor<float>(bg) : fs_bg;
      imagecolor<float> f_bc = bc!="" ? imagecolor<float>(bc) : fs_bc;

      imagedata bg_img(1, 1, 3, imagedata::pixeldata_float);
      bg_img.set(0, 0, f_bg);
      imagedata tbg_img(0, 0); // placeholder for true background image
      imagedata out = tbg_img.width() ? tbg_img : bg_img;

      scalinginfo os(out.width(), out.height(), f_width, f_height);
      out.rescale(os);
      out.convert(imagedata::pixeldata_uchar);
      out.force_three_channel();

      for (XmlDom img = frame.FirstChild(); img; img = img.Next()) {
	while (img && !img.IsElement())
	  img = img.Next();
	if (!img)
	  break;
	if (img.NodeName()!="image") {
	  ShowError(msg+"non <image> element found");
	  break;
	}

	bw = img.Property("border");
	size_t i_border = bw!="" ? atoi(bw.c_str()) : f_border;

	bc = img.Property("bordercolor");
	imagecolor<float> i_bc = bc!="" ? imagecolor<float>(bc) : f_bc;
	imagedata img_bc(1, 1, 3, imagedata::pixeldata_float);
	img_bc.set(0, 0, i_bc);
	img_bc.convert(imagedata::pixeldata_uchar);
	vector<unsigned char> i_bc_uchar = img_bc.get_uchar(0, 0);

	string lab = img.Property("label");
	string cro = img.Property("crop");
	string loc = img.Property("location");
	
	if (lab=="") {
	  ShowError(msg+"<image> without label");
	  continue;
	}
	
	if (loc=="") {
	  ShowError(msg+"<image label=\""+lab+"\"> without location");
	  continue;
	}

	size_t l_xmin, l_ymin, l_xmax, l_ymax;
	stringstream(loc) >> l_xmin >> l_ymin >> l_xmax >> l_ymax;

        imagedata *qimg = NULL;
        if (q) {
          Query::erf_image_data& aid = q->ErfImageData(lab);
          if (aid.original.empty())
            q->ErfInitializeImageData(aid, lab, q->AjaxMplayer());
          qimg = &aid.original;
        }

        //Tic("ImageData");
	int idx = db ? db->LabelIndex(lab) : -1;
        imagedata dat = qimg ? *qimg : db ? db->ImageData(idx) : imagedata();
        //Tac("ImageData");

        if (dat.empty()) {
          ShowError(msg+"ImageData("+lab+") failed: isempty()");
          continue;
        }

	if (cro!="") {
	  size_t c_xmin, c_ymin, c_xmax, c_ymax;
	  stringstream(cro) >> c_xmin >> c_ymin >> c_xmax >> c_ymax;
	  dat = dat.subimage(c_xmin, c_ymin, c_xmax, c_ymax);
	}

	scalinginfo is(dat.width(), dat.height(), 
		       l_xmax-l_xmin+1, l_ymax-l_ymin+1);
        //Tic("convert");
	dat.convert(imagedata::pixeldata_uchar);
        //Tac("convert");
	dat.force_three_channel();
        //Tic("rescale");
	dat.rescale(is);
        //Tac("rescale");
        //Tic("copyAsSubimage");
	out.copyAsSubimage(dat, l_xmin, l_ymin);
        //Tac("copyAsSubimage");

	for (size_t d=1; d<=i_border; d++) {
	  for (int a=l_xmin-d; a<=int(l_xmax+d); a++) {
	    if (out.coordinates_ok(a, l_ymin-d))
	      out.set(a, l_ymin-d, i_bc_uchar);
	    if (out.coordinates_ok(a, l_ymax+d))
	      out.set(a, l_ymax+d, i_bc_uchar);
	  }
	  for (int a=l_ymin-d; a<=int(l_ymax+d); a++) {
	    if (out.coordinates_ok(l_xmin-d, a))
	      out.set(l_xmin-d, a, i_bc_uchar);
	    if (out.coordinates_ok(l_xmax+d, a))
	      out.set(l_xmax+d, a, i_bc_uchar);
	  }
	}
      }

      //Tac("CreateImageCollage(xml)");

      return out;
    }

    ShowError(msg+"failed");

    return err;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLdatabaseinfo(XmlDom& xml, const string& spec,
                                    const Connection *conn,
                                    bool fullviews, bool also_invisibles) {
  ApplySettingsXML(false);

  if (conn && !CheckPermissions(*conn))
    return false;

//   cout << name << ": visible=" << visible << " development="
//        << pic_som->Development() << endl;

  if (!also_invisibles && !visible && !pic_som->Development())
    return false;

  string tns = FindThumbnailSizes();
  int tnw = 0, tnh = 0;
  sscanf(tns.c_str(), "%dx%d", &tnw, &tnh);

  target_type tt = target_any_target; // obs! added when a type was required...
  int size = GuessRestrictionSize(tt);

  int view_count = 0;
  for (size_t j=0; j<Nviews(); j++)
    if (!conn || (View(j).ApplySettingsXML(false) &&
                  View(j).CheckPermissions(*conn)))
      view_count++;
  
  XmlDom tree = xml.Element("database");
  tree.Element("name",       Name());
  tree.Element("longname",   LongName());
  tree.Element("shorttext",  ShortText());
  if (longtext_xml) {
    XmlDom lt = tree.CopyElement(longtext_xml, true);
    lt.Prop("content", "xml");
  }
  tree.Element("size",       size);
  tree.Element("tnsizes",    tns);
  tree.Element("tnwidth",    tnw);
  tree.Element("tnheight",   tnh);
  tree.Element("views",      view_count);
  if (RestrictionName()!="") {
    tree.Element("restriction", RestrictionName()); // obs! to be deprecated
    tree.Element("dbrestriction", RestrictionName());
  }

  if (fullviews && view_count) {
    XmlDom viewlist = spec=="flat" ? xml : tree.Element("viewlist");
    // viewlist.AddSOAP_ENC_arrayType(false);
    for (size_t i=0; i<Nviews(); i++)
      View(i).AddToXMLdatabaseinfo(viewlist, spec, conn, true, false);
  }

  XmlDom ttyp = tree.Element("objecttypes");
  for (map<target_type,int>::const_iterator i=tt_stat.begin(); 
       i != tt_stat.end(); i++)
    if (!IsCombined(i->first))
      ttyp.Element(TargetTypeString(i->first, true), i->second);

  if (allcontentsclasses!="" || ContentsClasses())
    AddToXMLcontentitemlist(tree);

  if (allowalgorithmselection || IsAllowed(conn, "algorithselection"))
    tree.Element("allowalgorithmselection", "yes");

  if (allowconvolutionselection || IsAllowed(conn, "convolutionselection"))
    tree.Element("allowconvolutionselection", "yes");

  if (allowcontentselection || IsAllowed(conn, "contentselection"))
    tree.Element("allowcontentselection", "yes");

  if (allowinsertion || IsAllowed(conn, "insertion"))
    tree.Element("allowinsertion", "yes");

  if ((allowownimage || IsAllowed(conn, "ownimage")) && CanCalculateFeatures())
    tree.Element("allowownimage", "yes");

  if (allowsegimage || IsAllowed(conn, "segimage"))
    tree.Element("allowsegimage", "yes");

  if (allowtextquery || IsAllowed(conn, "textquery")) {
    tree.Element("allowtextquery", "yes");
    for (textqueryopt_t::iterator i=textqueryopt.begin();
	 i!=textqueryopt.end(); i++) {
      XmlDom node = tree.Element("textquery-option");
      node.Prop("name",  i->first);
      node.Prop("value", i->second.first);
      node.Prop("text",  i->second.second);
    }
    tree.Element("textquery-log", TextQueryLog());
  }

  if (allowclassdefinition || IsAllowed(conn, "classdefinition"))
    tree.Element("allowclassdefinition", "yes");

  if (allowstoreresults || IsAllowed(conn, "storeresults"))
    tree.Element("allowstoreresults", "yes");

  if (allowobjecttypeselection || IsAllowed(conn, "objecttypeselection"))
    tree.Element("allowobjecttypeselection", "yes");

  if (allowdisplaymodeselection || IsAllowed(conn, "displaymodeselection"))
    tree.Element("allowdisplaymodeselection", "yes");

  if (allowimagescaling || IsAllowed(conn, "imagescaling"))
    tree.Element("allowimagescaling", "yes");

  if (allowarrivedobjectnotification 
      || IsAllowed(conn, "arrivedobjectnotification")) {
    tree.Element("allowarrivedobjectnotification", "yes");
    LookForArrivedObjects();
  }

  if (linkiconmode!="")
    tree.Element("linkiconmode", linkiconmode);

  if (use_context)
     tree.Element("context", "yes");

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLfeaturelist(XmlDom& xml, const Query *q) const {
    string msg = "DataBase::AddToXMLfeaturelist() : ";

    if (q->Nindices()==0 && !q->Parent())
      q = defaultquery;

    bool ok = true;
    for (size_t i=0; ok && i<Nindices(); i++)
      ok = ((DataBase*)this)->FindIndex(GetIndex(i)->FeatureFileName(),
    					"", true, false);

    // FindAllIndices();
    XmlDom tree = xml.Element("featurelist");
    // tree.AddSOAP_ENC_arrayType(true);

    for (size_t i=0; ok && i<Nindices(); i++) {
      // cout << i << " " << GetIndex(i)->Name()
      // 	   << (GetIndex(i)->IsSelectable() ?
      // 	       " SELECTABLE" : " non-selectable") << endl;
      if (GetIndex(i)->IsSelectable())
        if (!GetIndex(i)->AddToXMLfeatureinfo(tree, q))
          ok = ShowError(msg+"AddToXMLfeatureinfo() "+GetIndex(i)->Name()
                         +" failed");
      
      vector<size_t> feat_idx;
      if (q)
	feat_idx = q->IndexShortNameIndexList(GetIndex(i)->Name());
      for (size_t j=0; ok && j<feat_idx.size(); j++)
	if (q->IndexData(NULL, j).IsComplex())
	  if (!q->AddToXMLfeatureinfo(tree, j))
            ok = ShowError(msg+"AddToXMLfeatureinfo() "+ToStr(j)
                           +" failed");
    }
    
    AddToXMLfeaturealiases(tree);

    return ok;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLalgorithmlist(XmlDom& dom, const Query *q) {
  if (q->Nindices()==0 && !q->Parent())
    q = defaultquery;

  cbir_algorithm at = q->Algorithm();

  struct {
    cbir_algorithm id;
    string name, label, descr;
  } coremethods[] = {
    { cbir_picsom, 
      "picsom", "original picsom", 
      "Use the whole TS-SOM structure."},

    { cbir_picsom_bottom, 
      "picsom_bottom", "only bottom level",
      "Process only the largest (bottommost) SOM level." },

    { cbir_picsom_bottom_weighted,
      "picsom_bottom_weighted", "bottom level with weighting",
      "Process the bottommost SOM level with entropy weighting."},

    { cbir_random,
      "random", "random images",
      "Returns randomly chosen unseen images."},

    /*
    { cbir_vq,
      "vq", "vector quantization",
      "Selection of images with simple vector quantization."},

    { cbir_vqw,
      "vqw", "weighted vector quantization",
      "Selection of images with simple weighted vector quantization."},

    { cbir_sq,
      "sq", "scalar quantization",
      "Selection of images with simple scalar quantization."},
    */

    { cbir_no_algorithm, "", "", ""}
  }, *method = coremethods;

  int order = 1;
  XmlDom tree = dom.Element("algorithmlist");
  while (method->name!="") {
    XmlDom alg = tree.Element("algorithm");
    alg.Element("name",        method->name);
    alg.Element("label",       method->label);
    alg.Element("description", method->descr);
    alg.Element("default",     method->id==at);
    alg.Element("order",       order++);
    method++;
  }

  // obs! HAS_CBIRALGORITHM_NEW problem:
  // obs! still only one algorithm at time supported
  string an;
  if (at==cbir_no_algorithm && q->Algorithms().size()==1)
    an = q->Algorithms()[0].fullname;

  list<string> l = CbirAlgorithm::AlgorithmList();
  for (list<string>::const_iterator i=l.begin(); i!=l.end(); i++) {
    vector<CbirAlgorithm*> alist = Algorithms(*i);
    for (size_t j=0; j<=alist.size(); j++) {
      if (j==alist.size() && alist.size()!=0)
	break;

      string dummy;

      const CbirAlgorithm *a = j<alist.size() ? alist[j] :
	CbirAlgorithm::FindAlgorithm(*i, dummy);
      
      XmlDom alg = tree.Element("algorithm");
      alg.Element("name",        a->BaseName());
      alg.Element("label",       a->LongName());
      alg.Element("description", a->Description());
      alg.Element("default",     a->FullName()==an);
      alg.Element("order",       order++);

      const list<CbirAlgorithm::parameter> pars = a->Parameters();
      if (!pars.empty()) {
	XmlDom plist = alg.Element("parameterlist");
	for (list<CbirAlgorithm::parameter>::const_iterator p=pars.begin();
	     p!=pars.end(); p++) {
	  XmlDom par = plist.Element("parameter");
	  par.Prop("name",   	  p->name);
	  par.Prop("type",   	  p->type);
	  par.Prop("description", p->description);
          par.Prop("defval", 	  p->defval);
          if (p->type=="xsd:token") {
            vector<string> clist = SplitInSpaces(p->range);
            for (size_t c=0; c<clist.size(); c++) {
              XmlDom cho = par.Element("choice");
              cho.Prop("name", clist[c]);
            }
          }
	}
      }
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

vector<CbirAlgorithm*> DataBase::Algorithms(const string& bname) const {
  if (Query::DebugAlgorithms())
    cout << "DataBase::Algorithms(" << bname << ") called" << endl;
  
  vector<CbirAlgorithm*> l;
  
  const vector<CbirAlgorithm*>& alist = Algorithms();
  for (size_t j=0; j<alist.size(); j++) {
    string n = alist[j]->FullName();
    size_t p = n.find('_');
    if (p!=string::npos)
      n.erase(p);
    if (n==bname)
      l.push_back(alist[j]);

    if (Query::DebugAlgorithms())
      cout << "  " << n << " " << alist[j]->FullName() << " "
	   << (n==bname?"":"not ") << "added" << endl;
  }
  if (Query::DebugAlgorithms())
    cout << "DataBase::Algorithms(" << bname << ") returns "
         << l.size() << endl;

  return l;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::LookForArrivedObjects() {
  // don't do anything if labels file hasn't been modified (returns
  // true -- no error)
  if (!labelsfile.Modified()) 
    return true;

  size_t oldsize = Size();
  if (!ReadLabels(true))
    return false;

  for(size_t i=oldsize; i<Size(); i++)
    arrivedobjects.push_back(i);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLarrivedobjectinfo(XmlDom& xml, /*const*/ Query *q,
					 const Connection *c) {

  // do nothing if we don't want to display info on arrived objects
  if (!(allowarrivedobjectnotification 
        || IsAllowed(c,"arrivedobjectnotification")))
    return true;

  // don't even print the xml tag arrivedobjectlist if there's no arrived objs
  if (arrivedobjects.empty())
    return true;

  XmlDom tree = xml.Element("arrivedobjectlist");

  for (vector<int>::iterator i = arrivedobjects.begin();
      i != arrivedobjects.end(); i++) 
    // add the object only if it's of the right target_type
    if((int)q->Target() & (int)ObjectsTargetType(*i) ) {
      const string &lab = Label(*i);
      AddToXMLobjectinfo(tree, lab, q, false, true, true, true, NULL, "");
    }

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLtextinfo(XmlDom& xml, const string& lab) const {
    string dir = SolveObjectDirectory(lab);
    string subdir = lab.substr(0, LabelLength());

    string textfile = dir+"/"+lab+".txt";
    if (!FileExists(textfile)) {
      textfile = dir+"/"+subdir+".txt";
      if (!FileExists(textfile)) {
        textfile = dir+"/"+subdir+".d/"+lab+".txt";
        if (!FileExists(textfile)) {
          textfile = dir+"/"+subdir+".d/message_utf8.txt";
          if (!FileExists(textfile))
            return ShowError("AddToXMLtextinfo("+lab+") : text file not found");
        }
      }
    }

    ifstream ofile(textfile.c_str());
    string message, row;
    while (getline(ofile, row)) {
      message.append(row);
      message.append("\n");
    }

    string message_escaped = Latin1ToUTF(message);
    xml.Element("text", message_escaped);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLtextindextexts(XmlDom& xml, size_t idx) const {
    XmlDom l = xml.Element("textindextexts");
    list<string> til = TextIndices();
    for (auto in=til.begin(); in!=til.end(); in++) {
      list<pair<string,string> > td = ((DataBase*)this)->TextIndexRetrieve(idx, *in);
      for (auto ti=td.begin(); ti!=td.end(); ti++)
	if (ti->first!="label" || ti->second!=Label(idx)) {
	  cout << "  " << *in << " : " << ti->first
	       << " = \"" << ti->second << "\"" << endl;
	  XmlDom t = l.Element("text", ti->second);
	  t.Prop("index", *in);
	  t.Prop("field", ti->first);
	}
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLobjectinfo(XmlDom& xml, const string& label,
                                    Query *query, bool minimal,
				    bool childinfo, bool oinfo, bool full,
                                    XmlDom *xtrax, const string& spec) const {
    bool add_textindextexts = true;

    if (label=="")
      return ShowError("AddToXMLobjectinfo() entered with empty label");

    if (IsVirtualImage(label))
      return AddToXMLimageinfovirtual(xml, label, query, xtrax);

    size_t ll = LabelLength();

    if (BaseName()=="trecvid2006" &&
        (label.find(":audio")==ll || label.find(":asr")==ll
         || label.find(":cc")==ll))
      return true;

    if (!IsLabel(label))
      return ShowError("DataBase::AddToXMLobjectinfo(label=<", label,
                       ">) is not proper");

    XmlDom vi = xml.Element("objectinfo");

    vi.Element("label", label);

    if (xtrax && xtrax->node) {
      xmlNodePtr xtra = xtrax->node->children;
      while (xtra) {
        xmlNodePtr next = xtra->next;
	if (!minimal || NodeName(xtra)=="score")
	  xmlAddChild(vi.node, xmlDocCopyNode(xtra, vi.doc, 1));
        xtra = next;
      }
    }

    if (minimal)
      return true;

    int idx = LabelIndex(label);
    vi.Element("index", idx);

    target_type ott = ObjectsTargetType(idx);
    vi.Element("targettype", TargetTypeString(ott));

    int pidx = ParentObject(idx);
    if (pidx>=0)
      vi.Element("parent", Label(pidx));

    vi.Element("database", Name());

    vi.Element("selectable", (query && query->ObjectSelectable(idx))
	       ? "true" : "false");

    DataBase *ncdb = (DataBase*)this;
    if (ObjectsTargetTypeContains(idx, target_video)) {
      float dur = ncdb->VideoDuration(idx);
      vi.Element("videoduration", dur);
    }

    if (ObjectsTargetTypeContains(idx, target_videosegment)) {
      auto psd = ncdb->ParentStartDuration(idx, target_video);
      vi.Element("videostart", psd.second.first);
      vi.Element("videoend",   psd.second.first+ psd.second.second);
      size_t p = label.find(":t");
      if (p!=string::npos) {
	int  tno = atoi(label.substr(p+2).c_str());
	size_t q = label.find_first_not_of("0123456789", p+2);
	string suf = q==string::npos ? "" : label.substr(q);
	string prev = label.substr(0, p+2)+ToStr(tno-1)+suf;
	string next = label.substr(0, p+2)+ToStr(tno+1)+suf;
	if (LabelIndexGentle(prev, false)>=0)
	  vi.Element("videoprevious", prev);
	if (LabelIndexGentle(next, false)>=0)
	  vi.Element("videonext", next);
      }
    }

    if (add_textindextexts)
      AddToXMLtextindextexts(vi, idx);

    for (auto tn=virtualthumbnailsize.begin();
	 tn!=virtualthumbnailsize.end(); tn++) {
      ground_truth gt = ((DataBase*)this)->GroundTruthExpression(tn->first);
      if (gt[idx]==1) {
	vector<string> wh;
	if (tn->second=="lfw")
	  wh = vector<string> { "250", "250" };
	else if (tn->second=="of")
	  wh = vector<string> { "120", "120" };
	else
	  wh = SplitInSomething("x", false, tn->second);

	if (wh.size()==2) {
	  vi.Element("width",  wh[0]);
	  vi.Element("height", wh[1]);
	}
	break;
      }
    }

    if (full) {
      AddToXMLsegmentationinfolist(vi, label);
      AddToXMLextraobjectinfo(vi, label);
    }

    XmlDom fi  = vi.Element("objectfileinfo");
    XmlDom fri = vi.Element("frameinfo");  

    if (oinfo) {
      string filename;
      AddToXMLoriginsobjectfileinfo(fi, fri, idx, filename, query);
    }

    if (full) {
      AddToXMLavailabilityinfo(fi, label, query);
      //AddToXMLkeywordlist(vi, idx, false, false);
    }

    // moved here 2013-03-07 for testing with tartest, restca, etc.
    size_t ndet = 0;
    string detformat;
    if (showdetections != "") {
      ndet = 5;
      //detspec = "fusion-g-lin-hkm";
      //detspec = "linear::OCVCentrist";
      detformat = "lscom";
    }
    AddToXMLkeywordlist(vi, idx, true, false, ndet, showdetections, 
			detformat, query);

    if (ott & target_text) 
      AddToXMLtextinfo(vi, label);

    else if (use_subobject_text) {
      const vector<int>& chs = SubObjects(idx);
      for (size_t i=0; i<chs.size(); i++) {
        if (ObjectsTargetType(chs[i]) & target_text) {
          AddToXMLtextinfo(vi, Label(chs[i]));
          break;
        }
      }
    }

    if (ott & target_message)
      AddToXMLobjecttext(vi, label);

    if (childinfo) {
      string segm;
      vector<string> ss = SplitInCommasObeyParentheses(spec);
      for (auto i=ss.begin(); i!=ss.end(); i++)
	if (i->find("segm=")==0)
	  segm = i->substr(5);
      AddToXMLsubobjects(vi, label, query, oinfo, full, segm);
    }

    if (query)
      query->AddToXMLobjectinfo_dynamic(vi, idx);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLimageinfovirtual(XmlDom& xml, const string& label, 
                                          Query *query, XmlDom *xtrax) const {
    if (Is8plus8(label))
      return AddToXMLimageinfo8plus8(xml, label, query, xtrax);

    string method, imagename, frame, segment;
    bool ok = SplitLabel(label, method, imagename, frame, segment);
    if (!ok)
      return ShowError("AddToXMLimageinfovirtual() : SplitLabel("+label
		       +") failed");

    XmlDom vi = xml.Element("objectinfo");
    vi.Element("note",  "this is a segmentation image");
    vi.Element("label", label);

    XmlDom fi  = vi.Element("objectfileinfo"); // was "imagefileinfo"
    XmlDom fri = vi.Element("frameinfo");

    size_t idx = LabelIndex(imagename);
    string tmp;
    ok = ok && AddToXMLoriginsobjectfileinfo(fi, fri, idx, tmp, query);
    ok = ok && AddToXMLavailabilityinfo(     fi, imagename, query);
    ok = ok && AddToXMLsegmentinfo(          vi, method,    segment);

    // is this still really needed:
    ok = ok && AddToXMLframeinfo(            vi, imagename, frame);

    return ok; 
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLframeinfo(XmlDom& xml, const string& filename, 
                                   const string& number) const {
    XmlDom fi = xml.Element("frameinfo");
    fi.Element("file",   filename);
    fi.Element("number", number);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLsegmentinfo(XmlDom& xml, const string& method,
                                     const string& number) const {
    XmlDom fi = xml.Element("segmentinfo");
    fi.Element("method", method);
    fi.Element("number", number);
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLimageinfo8plus8(XmlDom& xml, const string& lab,
                                         Query *q, XmlDom*) const {

    XmlDom vi = xml.Element("objectinfo");
  
    vi.Element("note",   "this is a virtual image");
    vi.Element("label",  lab);
    vi.Element("name",   lab);
    vi.Element("format", "PNG");

    XmlDom plist = vi.Element("partlist");
    size_t p = lab.find('+');

    return AddToXMLobjectinfo(plist, lab.substr(0, p), q,
			      false, true, true, true, NULL, "")
      &&   AddToXMLobjectinfo(plist, lab.substr(p+1),  q,
			      false, true, true, true, NULL, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLextraobjectinfo(XmlDom& xml,
					 const string& labstr) const {
    string dirstr = SolveObjectDirectory(labstr);
    const char *label = labstr.c_str(), *dir = dirstr.c_str(); //obs! clarify?

    char xmlinfo[1024];
    sprintf(xmlinfo, "%s/info/%s.xml", dir, label);

    if (!FileExists(xmlinfo))
      sprintf(xmlinfo, "%s/%s.d/info-%s.xml", dir, label, label);

    if (FileExists(xmlinfo)) {
      xmlDocPtr   doc = xmlParseFile(xmlinfo);
      xmlNodePtr root = doc ? xmlDocGetRootElement(doc) : NULL; 
      xmlNodePtr xi   = xmlNewTextChild(xml.node, xml.ns, root->name, NULL);
      xmlNodePtr node = root ? root->children : NULL;      

      XMLS itemname, itemvalue;
      for (; node; node = node->next) {
        itemname  = xmlGetProp(node, (XMLS)"name");
        itemvalue = xmlGetProp(node, (XMLS)"value");
        if (!itemname)
          continue;
        xmlNodePtr xii = xmlNewTextChild(xi, xml.ns, node->name, NULL);
        xmlSetProp(xii, (XMLS)"name",  itemname);
        xmlSetProp(xii, (XMLS)"value", itemvalue);
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLobjecttext(XmlDom& xml, const string& labstr) const {
    string dirstr = SolveObjectDirectory(labstr);
    const char *label = labstr.c_str(), *dir = dirstr.c_str(); // obs! clarify?

    char imagetext[1024];
    sprintf(imagetext, "%s/%s.d/message_utf8.txt", dir, label);

    if (FileExists(imagetext)) {
      //cerr << "Opening file " << imagetext << endl;
      ifstream ofile(imagetext);
      char textrow[1024];
      string messagestring("");

      ofile.getline(textrow, sizeof textrow); // get the from text
      string msg_from = textrow;
      ofile.getline(textrow, sizeof textrow); // get the to text
      string msg_to = textrow;
      ofile.getline(textrow, sizeof textrow); // get the date text
      string msg_date = textrow;
      ofile.getline(textrow, sizeof textrow); // get the subject text
      string msg_title = textrow;

      // get the whole message text:
      while (ofile.getline(textrow, sizeof textrow)) {
        messagestring.append(textrow);
        messagestring.append("\n");
      }
      //cerr << " File contents: \"" << messagestring << "\"" << endl;

      XmlDom xii = xml.Element("messageheader");
      xii.Element("from",  msg_from);
      xii.Element("to",    msg_to);
      xii.Element("date",  msg_date);
      xii.Element("title", msg_title);

      xml.Element("messagetext", messagestring);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLsubobjects(XmlDom& xml, const string& label,
                                    Query *q, bool oinfo, bool full,
				    const string& segm) const {
    const int countlimit = 100;

    bool ok = true;
    XmlDom xii = xml.Element("subobjects");
  
    int idx = LabelIndex(label), count = 0;
    const vector<int>& sub = SubObjects(idx);
    for (size_t subi=0; subi<sub.size(); subi++) {
      const string& lab = Label(sub[subi]);
      if (segm!="" && lab.find(segm+":")!=0)
	continue;

      // obs! here should be a check to prevent infinite loops etc.
      bool skip = sub[subi]==idx || lab==label;
      if (!skip)
        ok = ok && AddToXMLobjectinfo(xii, lab, q,
				      false, true, full, oinfo, NULL, "");
      else
        ShowError("Trying to avoid infinite loops in AddToXMLsubobjects()");

      if (++count>countlimit && countlimit>0) {
        WarnOnce("DataBase::AddToXMLsubobjects() : countlimit exceeded");
        break;
      }
    }
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLoriginsobjectfileinfo(XmlDom& fi, XmlDom& fri,
                                               size_t idx, string& fname,
					       Query *query) const {
    map<string,string> hashtmp;
    if (!query)
      hashtmp = ReadOriginsInfo(idx, false, true);
    const map<string,string>& hash =
      query ? query->OriginsInfo(idx, true) : hashtmp;
  
    if (hash.empty()) {
      fi.Element("name", "???");
      return false;
    }

    fname = "";

    for (map<string,string>::const_iterator i = hash.begin();
         i!=hash.end(); i++) {

      XmlDom node = i->first=="colors" || i->first=="dimensions" ? fri : fi;
      node.Element(i->first, i->second);

      if (i->first=="dimensions") {
        int w = 0, h = 0;
        if (sscanf(i->second.c_str(), "%dx%d", &w, &h)==2) {
          node.Element("width",  w);
          node.Element("height", h);
        }
      }

      if (i->first=="name")
        fname = i->second;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLavailabilityinfo(XmlDom& xml, const string& labstr,
                                          Query * /*query*/) const {
    const long sizelimit = 10*1024*1024;

    int idx = LabelIndex(labstr);

    bool av = false;
    if (deliverlocalcopies) {
      string p = SolveObjectPath(labstr, "", "", false, NULL, true);
      if (FileExists(p))
        av = sizelimit==0 || FileSize(p)<=sizelimit;

      else if (IsMissingVideoClip(idx) ||
               IsMissingAudioClip(idx))
        av = true;
    }

    string avstr = av?"1":"0";
    xml.Element("available", avstr);

    if (DebugOrigins()) 
      cout << "  available=<" << avstr << "> set on fly" << endl;

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLfeaturealiases(XmlDom& xml) const {
  // CreateDefaultFeatureAliases(true, false); 

  for (featurealias_t::const_iterator i=featurealias.begin();
       i!= featurealias.end(); i++) {
    if (i->first.find("**")==0)
      continue;

    XmlDom fa = xml.Element("featurealias");
    fa.Prop("name",  i->first);
    fa.Prop("text",  i->second.first); 

    stringstream value;
    for (list<string>::const_iterator j=i->second.second.begin(); 
	j!=i->second.second.end(); j++) 
      value << (value.str()!=""?",":"") << *j;

    fa.Prop("value", value.str());
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void DataBase::CreateDefaultFeatureAliases(bool force, bool find) {
  RwLockWrite("CreateDefaultFeatureAliases");
  
  // create '*' and other aliases if it does not already exist
  featurealias_t::const_iterator f = featurealias.find("*");
  if (!force && f!=featurealias.end() && !f->second.second.empty()) {
    RwUnlockWrite("CreateDefaultFeatureAliases");
    return;
  }

  // featurealias.clear(); // commented out 2015-11-25

  typedef pair<string,list<string> > p_t;

  if (DefaultQuery().Nindices()) {
    list<string> llist;
    for (size_t i=0; i<DefaultQuery().Nindices(); i++)
      llist.push_back(DefaultQuery().TsSom(i).Name());
    featurealias["*default"] = p_t("All default features", llist);
  }

  if (find)
    FindAllIndices();

  map<target_type,list<string> > l_all, l_sel;

  for (size_t i=0; i<Nindices(); i++) {
    if (Query::DebugFeatSel())
      cout << "index i=" << i << " name=" << GetIndex(i)->Name()
	   << " target=" << TargetTypeString(GetIndex(i)->FeatureTarget())
	   << " selectable=" << GetIndex(i)->IsSelectable() << endl;

    l_all[target_any_target].push_back(GetIndex(i)->Name());
    if (GetIndex(i)->FeatureTarget()!=target_any_target)
      l_all[GetIndex(i)->FeatureTarget()].push_back(GetIndex(i)->Name());

    if (GetIndex(i)->IsSelectable()) {
      l_sel[target_any_target].push_back(GetIndex(i)->Name());
      if (GetIndex(i)->FeatureTarget()!=target_any_target)
	l_sel[GetIndex(i)->FeatureTarget()].push_back(GetIndex(i)->Name());
    }
  }

  for (size_t ss=0; ss<2; ss++) {
    const map<target_type,list<string> >& l = ss ? l_sel : l_all;

    for (map<target_type,list<string> >::const_iterator i=l.begin();
	 i!=l.end(); i++) {
      string ttstr;
      if (i->first!=target_any_target)
	ttstr = TargetTypeString(i->first);
      string aname = (ss?"*":"**")+ttstr;
      string descr = "All "+string(ss?"selectable ":"")+
	ttstr+(ttstr!=""?" ":"")+"features.";

      featurealias[aname] = p_t(descr, i->second);
      
      if (Query::DebugFeatSel())
	cout << Name() << " default feature alias [" << aname << "]=\""
	     << descr << "\" [" << CommaJoin(i->second) << "]" << endl;
    }
  }

  if (featurealias.find("*")==featurealias.end())
    featurealias["*"] = p_t("", list<string>());

  RwUnlockWrite("CreateDefaultFeatureAliases");
}

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::ExpandFeatureAlias(const string& n) {
    string msg = "DataBase::ExpandFeatureAlias("+n+") : ";

    // DumpFeatureAlias();

    featurealias_t::iterator i = featurealias.find(n);
    if (i==featurealias.end() && n[0]=='*') {
      CreateDefaultFeatureAliases(true, true);
      i = featurealias.find(n);
    }

    size_t p = n.find("§");
    if (i==featurealias.end() && p!=string::npos) {
      featurealias_t::iterator j = featurealias.find(n.substr(p));
      if (j!=featurealias.end()) {
	list<string> l;
	for (auto a=j->second.second.begin(); a!=j->second.second.end(); a++) {
	  string fbase = n.substr(0, p), faugm = fbase+*a;
	  l.push_back(faugm);

	  if (!PossiblyImplementFeatureAugmentation(faugm)) {
	    ShowError(msg+"implementing augmentation <"+faugm+"> failed");
	    return list<string>();
	  }

	}
	return l;

      } else if (!PossiblyImplementFeatureAugmentation(n)) {
	ShowError(msg+"PossiblyImplementFeatureAugmentation() failed");
	return list<string>();
      }
    }

    return i==featurealias.end() ? list<string>(1, n) : i->second.second;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SetFeatureAlias(const string &n, const string &e, 
				 const string &t) {
    string msg = "DataBase::SetFeatureAlias("+n+","+e+","+t+") : ";

    vector<string> fe = SplitInCommas(e);
    list<string> l;
      
    vector<string>::iterator i;
    featurealias_t::iterator j;
    for (i=fe.begin(); i!=fe.end(); i++) 
      if ((j = featurealias.find(*i)) != featurealias.end()) {
	// is an alias ==> insert all the features of the alias
	l.insert(l.end(), j->second.second.begin(), j->second.second.end());

      } else if (i->size()>2 && (*i)[0]=='/' && (*i)[i->size()-1]=='/') {
	// is a /xxx/ regexp.
	RegExp re(i->substr(1, i->size()-2),false);
	if (!re.ok())
	  return ShowError("SetFeatureAlias() : RegExp() failed", re.error());

	FindAllIndices();
	for (size_t jj=0; jj<Nindices(); jj++)
	  if (re.match(IndexName(jj)))
	    l.push_back(IndexName(jj));

      } else {
	// is not an alias ==> insert just the given feature name
	l.push_back(*i);
      }
      
    featurealias[n] = make_pair(t, l);

    if (DebugFeatures())
      cout << msg << "[" << n << "]=(" << t << ")+(" << CommaJoin(l) << ")"
	   << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SetFeatureAugmentation(const string &n, const string &e, 
					const string &t) {
    string msg = "DataBase::SetFeatureAugmentation("+n+","+e+","+t+") : ";

    vector<string> v = SplitInSpaces(e);
    list<string> l(v.begin(), v.end());
      
    featureaugmentation[n] = make_pair(t, l);

    if (DebugFeatures())
      cout << msg << "[" << n << "]=(" << t << ")+(" << CommaJoin(l) << ")"
	   << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::PossiblyImplementFeatureAugmentation(const string& n) {
    string msg = "DataBase::PossiblyImplementFeatureAugmentation("+n+") : ";

    size_t p = n.find("§");
    if (p==string::npos)
      return true;

    Index *idxaugm = FindIndexInternal(n, true);
    if (idxaugm)
      return true;

    string fbase = n.substr(0, p);
    string augm  = n.substr(p);

    auto fa = featureaugmentation.find(augm);
    if (fa==featureaugmentation.end())
      return ShowError(msg+"augmentation <"+augm+"> not known");

    const list<string>& aopts = fa->second.second;

    Index *idxbase = FindIndex(fbase);
    idxaugm = FindIndex(n, "", false, true);
    *idxaugm = *idxbase;
    idxaugm->SetIndexName(n);
    vector<string> cmdvec = idxbase->FeaturesCommand();
	    
    cmdvec.insert(cmdvec.end(), aopts.begin(), aopts.end());
    idxaugm->FeaturesCommand(cmdvec, DebugFeatures());
    if (DebugFeatures())
      cout << msg+"augmented " << fbase << " to " << n << endl;

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLnamedcmdlines(xmlNodePtr n, xmlNsPtr ns) {
  if (namedcmdline.size()==0)
    return true;

  xmlNodePtr namedcmdlines = AddTag(n, ns, "namedcmdlines");

  for (namedcmdline_t::const_iterator i=namedcmdline.begin();
       i!= namedcmdline.end(); i++) {

    xmlNodePtr ncl = AddTag(namedcmdlines, ns, "namedcmdline");
    SetProperty(ncl, "name",  i->first);
    SetProperty(ncl, "text",  i->second.first); 

    stringstream value;
    for (list<string>::const_iterator j=i->second.second.begin(); 
	j!=i->second.second.end(); j++) 
      value << (j!=i->second.second.begin()?" ":"") << *j;

    SetProperty(ncl, "value", value.str());
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::IsPrunable(const string& ll) const {
  int idx  = LabelIndex(ll);
  int pidx = ParentObject(idx);
  if (pidx>=0) {
    target_type tt = ObjectsTargetType(idx);
    target_type pt = ObjectsTargetType(pidx);
    if ((PicSOM::TargetTypeContains(pt, target_video) &&
         PicSOM::TargetTypeContains(tt, target_image)) ||
	(PicSOM::TargetTypeContains(pt, target_image) &&
         PicSOM::TargetTypeContains(tt, target_segment)))
      return true;
  }
  return false;
}

  /////////////////////////////////////////////////////////////////////////////

  map<string,string> DataBase::ReadOriginsInfo(size_t idx,
                                               bool prune,
                                               bool warn) const {
    if (UseSql())
      return ReadOriginsInfoSql(idx);

    auto p = origins_entry_cache_map.find(idx);
    if (p!=origins_entry_cache_map.end())
      return p->second;

    return ReadOriginsInfoClassical(Label(idx), prune, warn);
  }

  /////////////////////////////////////////////////////////////////////////////

  map<string,string> DataBase::ReadOriginsInfoSql(size_t idx) const {
    string hdr = "DataBase::ReadOriginsInfoSql("+ToStr(idx)+" : ";

    Tic("ReadOriginsInfoSql");

    map<string,string> ret;

    string f = "file,url,page,mime,colors,bytes,md5,insdate," // 0-7
      "width,height,frames,framerate,auxid"; // 8-12
    string cmd = "SELECT "+f+" FROM objects WHERE indexz = "+ToStr(idx)+";";
    if (DebugSql())
      cout << hdr << cmd << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd);
      if (!stmt) {
	ShowError("SqlitePrepare() failed with ["+cmd+"]");
	return ret;
      }

      int rrr = 0;
      size_t n = 0;
      for (;; n++) {
	rrr = sqlite3_step(stmt);
	if (rrr!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	string tsstr = SqliteString(stmt, 7);  // insdate (not moddate)
	if (tsstr.size()==sizeof(struct timespec)) {
	  struct timespec ts;
	  memcpy(&ts, tsstr.c_str(), sizeof ts);
	  ret["date"]     = Picsom()->OriginsDateString(ts, false);
	} else
	  ret["date"]     = "???";

	ret["name"]       = SqliteString(stmt, 	0);
	ret["url"]        = SqliteString(stmt, 	1);
	ret["pageurl"]    = SqliteString(stmt, 	2);
	ret["format"]     = SqliteString(stmt, 	3);
	ret["colors"]     = SqliteString(stmt, 	4);
	ret["size"]       = SqliteString(stmt, 	5);
	ret["checksum"]   = SqliteString(stmt, 	6);
	ret["width"]      = SqliteString(stmt,  8);
	ret["height"]     = SqliteString(stmt,  9);
	ret["frames"]     = SqliteString(stmt, 10);
	ret["framerate"]  = SqliteString(stmt, 11);
	ret["auxid"]      = SqliteString(stmt, 12);

	ret["page"] = ret["pageurl"]; //obs! this is a _mess_ ...

	break;
      }

      rrr = sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd);
      if (!stmt) {
	ShowError("MysqlPrepare() failed with ["+cmd+"]");
	return ret;
      }

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
      
      if (false && DebugSql()) {
	int nc = mysql_stmt_field_count(stmt);
	cout << "nc=" << nc << endl;
      }

      int nf = 13, fwidth = 1000;
      char **col = new char*[nf];
      unsigned long *len = new unsigned long[nf];
      MYSQL_BIND *bind = new MYSQL_BIND[nf];
      for (int i=0; i<nf; i++) {
	memset(bind+i, 0, sizeof(MYSQL_BIND));
	bind[i].buffer_type   = MYSQL_TYPE_STRING;
	bind[i].buffer        = col[i] = new char[fwidth];
	bind[i].buffer_length = fwidth;
	bind[i].length        = len+i;
     }	

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      size_t n = 0;
      for (;; n++) {
	int rrr = mysql_stmt_fetch(stmt);
      
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(hdr+"data truncated");
	  else
	    ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	string tsstr(col[ 7], len[ 7]);  // insdate (not moddate)
	struct timespec ts;
	memcpy(&ts, tsstr.c_str(), sizeof ts);

	ret["name"]       = string(col[ 0], len[ 0]);
	ret["url"]        = string(col[ 1], len[ 1]);
	ret["pageurl"]    = string(col[ 2], len[ 2]);
	ret["format"]     = string(col[ 3], len[ 3]);
	ret["colors"]     = string(col[ 4], len[ 4]);
	ret["size"]       = string(col[ 5], len[ 5]);
	ret["checksum"]   = string(col[ 6], len[ 6]);
	ret["date"]       = Picsom()->OriginsDateString(ts, false);
	ret["width"]      = string(col[ 8], len[ 8]);
	ret["height"]     = string(col[ 9], len[ 9]);
	ret["frames"]     = string(col[10], len[10]);
	ret["framerate"]  = string(col[11], len[11]);
	ret["auxid"]      = string(col[12], len[12]);
	break;
      }

      MysqlStmtClose(stmt);

      for (int i=0; i<nf; i++)
	delete col[i];

      delete bind;
      delete len;
      delete col;
    }
#endif // HAVE_MYSQL_H

    int   i_frames    = atoi(ret["frames"].c_str());
    float f_framerate = atof(ret["framerate"].c_str());

    string d = ret["width"]+"x"+ret["height"];
    if (i_frames>1)
      d += "x"+ret["frames"];
    if (f_framerate>0.0)
      d += "@"+ret["framerate"];
    ret["dimensions"] = d;
    
    ret["filename"] = ret["name"];
    
    string url = ret["url"];
    size_t p = url.find('[');
    if (p!=string::npos && url[url.size()-1]==']') {
      vector<string> tar_or_recipe = SplitInSomething("|", false, url);
      string recipe = tar_or_recipe.back();
      p = recipe.find('[');
      ret["subrange"] = recipe.substr(p+1, recipe.size()-2-p);
    }

    Tac("ReadOriginsInfoSql");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  map<string,string> DataBase::ReadOriginsInfoClassical(const string& ll,
							bool prune,
							bool warn) const {
    string l = ll;
    map<string,string> ret;

    if (UseSql()) {
      ShowError("DataBase::ReadOriginsInfoClassical() : called in SQL mode");
      return ret;
    }

    if (l=="") {
      ShowError("DataBase::ReadOriginsInfoClassical() : empty label");
      return ret;
    }

    string msg = "DataBase::ReadOriginsInfoClassical("+l+","+(prune?"1":"0")
      +","+(warn?"1":"0")+") : ";

    if (DebugOrigins())
      cout << "Now in "+msg << endl;

    if (prune) {
      string method, file, frame, segm;
      if (!SplitLabel(l, method, file, frame, segm))
	ShowError(msg+"failed in SplitLabel()");
      else
	l = file;
    }

    string dir = SolveObjectDirectory(l), ofname = dir+"/"+origins;

    ret = ((DataBase*)this)->ReadOriginsInfoInner(ofname, l,
						  warn&&!parent_view);
    if (!ret.empty())
      return ret;

    if (parent_view) {
      if (DebugOrigins())
	cout << "Descending to parent view in "+msg << endl;
      ret = parent_view->ReadOriginsInfoClassical(ll, prune, warn);
      if (!ret.empty())
	return ret;
      ShowError(msg+"failed in both view and its parent");
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  map<string,string> DataBase::ReadOriginsInfoInner(const string& fn,
						    const string& l,
						    bool warn) {

    string msg = "DataBase::ReadOriginsInfoInner("+fn+","+l+","
      +(warn?"1":"0")+") : ";

    bool use_cache = true;

    map<string,string> ret;

    ifstream ofile;
    if (!use_cache) {
      ofile.open(fn.c_str());

      if (!ofile) {
	if (has_origins && warn)
	  WarnOnce(msg+"origins file not found");

	return ret;
      }
    }

    if (DebugOrigins())
      cout << "Now in "+msg << endl;

    const list<string>& cache = use_cache ?
      ReadOriginsInfoInnerCache(fn) : list<string>();

    list<string>::const_iterator cachei = cache.begin();

    string line;
    size_t ls = l.size();

    bool found = false;
    for (; !found;) {
      if (!use_cache) {
	getline(ofile, line);
	if (!ofile)
	  break;
      } else {
	if (cachei==cache.end())
	  break;
	line = *cachei++;
      }

      if (line.substr(0, ls)==l &&
	  (line[ls]=='.' || line[ls]==' ' || line[ls]=='\t'))
	found = true;
    }

    if (!found) {
      if (warn)
	ShowError(msg, "line not found in <", fn, ">");
      return ret;
    }

    vector<string> values = SplitInSpaces(line);
    int nvalues = values.size();
    if (nvalues>9) {
      ShowError(msg, "nvalues>9");
      nvalues = 9;
    }

    static const char *fields[] = {
      "name",   "url",      "pageurl",
      "format", "colors",   "dimensions",
      "size",   "checksum", "date"
    };
    
    for (int i=0; i<nvalues; i++) {
      if (DebugOrigins())
	cout << "  " << fields[i] << "=<" << values[i] << "> ";
      if (values[i]!="" && values[i]!="-") {
	string val = values[i];
	if (i==3)
	  EnsureMIMEtype(val);

	if (i==5 && wxh_reversed_in_origins) {
	  vector<string> split = SplitInSomething("x", false, val);
	  if (split.size()==2 || split.size()==3) {
	    val = split[1]+"x"+split[0];
	    if (split.size()==3)
	      val += "x"+split[2];
	  }
	}

	ret[fields[i]] = val;
	if (DebugOrigins()) {
	  if (val==values[i])
	    cout << "set as such" << endl;
	  else
	    cout << "set as <" << val << ">" << endl;
	}

      } else if (DebugOrigins())
	cout << " skipped" << endl;
    }

    if (ret.find("name")!=ret.end()) {
      ret["filename"] = ret["name"];
      if (DebugOrigins())
	cout << "  filename=<" << ret["filename"] << "> set on fly" << endl;
    }

    if (nvalues>1 && values[1]!="" && values[1]!="-") {
      const string& ff = values[1];
      size_t p = ff.find('[');
      if (p!=string::npos && ff.size()>2 && ff[ff.size()-1]==']') {
	string tmp = ff.substr(p+1, ff.size()-p-2);
	ret["subrange"] = tmp;
	if (DebugOrigins())
	  cout << "  subrange=<" << tmp << "> set on fly" << endl;
      } else {
	size_t s = ff.rfind('/');
	string tmp = s!=string::npos ? ff.substr(s+1) : ff;
	ret["original-name"] = tmp;
	if (DebugOrigins())
	  cout << "  original-name=<" << tmp << "> set on fly" << endl;
      }
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  const list<string>& DataBase::ReadOriginsInfoInnerCache(const string& fn) {
    // there should be a mechanism for reducing the size of the map...

    string msg = "DataBase::ReadOriginsInfoInnerCache("+fn+") : ";

    RwLockWrite/*Read*/("ReadOriginsInfoInnerCache");

    if (origins_file_line_cache_map.find(fn)==origins_file_line_cache_map.end()) {
      // RwLockWrite("ReadOriginsInfoInnerCache");

      list<string>& origins_cache = origins_file_line_cache_map[fn];

      ifstream ofile(fn.c_str());

      if (!ofile) {
	if (has_origins)
	  WarnOnce(msg+"origins file not found");
      }

      if (DebugOrigins())
	cout << "Now in "+msg << endl;

      while (ofile) {
	string line;
	getline(ofile, line);
	origins_cache.push_back(line);
      }

      // RwUnlockWrite("ReadOriginsInfoInnerCache");
    }

    RwUnlockWrite/*Read*/("ReadOriginsInfoInnerCache");

    return origins_file_line_cache_map[fn];
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLsegmentationinfo(XmlDom& xml, const string& label,
                                          const string& spec) const {
    if (!IsLabel(label))
      return ShowError("DataBase::AddToXMLsegmentationinfo(label=<",
                       label, ">) is not proper");

    XmlDom vi = xml.Element("segmentationinfo");
    // AddToXMLobjectinfo(vi, label, NULL, true, true, NULL);
    // as of 2011-03-10 <segmentationinfo> is inside <objectinfo> and it
    // doesn't make sense to have them in recursion, this was commented out

    vi.Element("methodname", spec);

    const string nf = " -- not found -- ";
    const char /* *d = objdir.c_str(),*/ *s = "segments";

    string obj = SolveObjectPath(label);
    string ip = obj.empty() ? nf : obj;

    string sp = spec;
    sp += ":";
    obj = SolveObjectPath(label, s, sp);
    sp = obj.empty() ? nf : obj;
 
    if (!FileExists(ip)||!FileExists(sp))
      return false;

    picsom::segmentfile segf(ip, sp);

    stringstream tmp;
    tmp << segf.GetLabelCount();
    vi.Element("components", tmp.str());

    tmp.str("");
    tmp << segf.GetSeparateCount();
    vi.Element("separate", tmp.str());

    // tmp.str("");
    // tmp << st.st_mtime;
    // vi.Element("date", tmp.str());

    //xmlDocPtr doc=segf.getXMLDescription();
    // if(doc) cout << "Got xmlDescription." << endl;
    //  xmlNodePtr ml_old=Segmentation::XMLTools::xmlDocGetMethodlist(doc);
    //xmlNodePtr segm_old=picsom::XMLTools::xmlDocGetSegmentation(doc);
    //vector<string> path;
    //path.push_back("methodinfolist");

    xmlAddChild(vi.node, segf.getMethodInfoListXML());
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLsegmentationinfolist(XmlDom& xml,
                                              const string& label) const {
    if (!showsegmentations)
      return true;

    if (!IsLabel(label))
      return ShowError("DataBase::AddToXMLsegmentationinfolist(label=<",
                       label, ">) is not proper");

    if (LabelLength()!=8)
      return true; // should be implemented...

    //  cout << "Now in DataBase::AddToXMLsegmentationinfolist" << endl

    XmlDom slist = xml.Element("segmentationinfolist");

    char segmpath[1000];
    char objpath[9];

    objpath[0]=label[0];
    objpath[1]=label[1];
    objpath[2]='/';
    objpath[3]=label[2];
    objpath[4]=label[3];
    objpath[5]='/';
    objpath[6]=label[4];
    objpath[7]=label[5];
    objpath[8]=0;

    string pxs = ExpandPath(objdir);
    strcpy(segmpath, pxs.c_str());
    strcat(segmpath, "/");
    strcat(segmpath, objpath);
    strcat(segmpath, "/segments");

    if (!Exists(segmpath)) {
      // ShowError("DataBase::AddToXMLsegmentationinfolist(label=<",
      // 	      label, ">) path <",  segmpath, "> doesn't exist");
      return false;
    }

#ifdef HAVE_DIRENT_H
    DIR *ds = opendir(segmpath);  
    for (dirent *dpv; ds && (dpv = readdir(ds)); ) {
      if (!strcmp(dpv->d_name, ".") || !strcmp(dpv->d_name, ".."))
        continue;

      const char *ptr = strchr(dpv->d_name, ':'); 
      if (!ptr || ptr==dpv->d_name)
        continue;

      string mname = dpv->d_name, lblstr = string(":")+label+".seg";
      string::size_type colon = mname.find(':');
      if (mname.compare(colon, string::npos, lblstr))
        continue;
    
      slist.Element("method", mname.substr(0, colon));
    }
    if (ds)
      closedir(ds);
#endif // HAVE_DIRENT_H

    return slist;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLkeywordlist(XmlDom& xml, int idx,
                                     bool val_too, bool find,
				     size_t ndet, const string& detspec,
				     const string& detformat,
				     const Query *q) const {
    // obs! let's start with an ugly trick...
    keyword_list kwl = ((DataBase*)this)->FindKeywords(idx, true, find);
    XmlDom kw = xml.Element("keywordlist");

    for (keyword_list::const_iterator i=kwl.begin(); i!=kwl.end(); i++) {
      // XmlDom k = kw.Element("keyword", i->first);
      // if (val_too)
      //   k.Prop("value", i->second);
      XmlDom k = kw.Element("keyword");
      k.Prop("name", i->first);
      k.Prop("type", "gt"); // obs! gt=ground_truth, anything else? 
      if (val_too)
	k.Prop("value", i->second);
    }
    
    if (ndet) {
      set<string> cls;
      if (q)
	cls = q->GetAugmentClasses("");

      bool dummy = true;
      map<string,vector<float> > dets =
	((DataBase*)this)->RetrieveDetectionData(idx, detspec, true, dummy);
      multimap<float,string> val;
      for (auto i=dets.begin(); i!=dets.end(); i++) {
	if (i->second.size()!=1)
	  ShowError("detection dimensionality != 1");

	val.insert(make_pair(i->second[0], i->first));
      }
      size_t n = ndet;

      for (auto i=val.rbegin(); n && i!=val.rend(); i++)
	if (AddToXMLkeywordlistInner(kw, kwl, i->second, i->first, detformat,
				     &cls, true))
	  n--;

      for (auto i=val.rbegin(); n && i!=val.rend(); i++)
	if (AddToXMLkeywordlistInner(kw, kwl, i->second, i->first, detformat,
				     &cls, false))
	  n--;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddToXMLkeywordlistInner(XmlDom& kw, const keyword_list &kwl,
					  string name, float value, 
					  const string& detformat, 
					  const set<string> *cset, 
					  bool firstcall) const {

    string detector;
    if (detformat=="lscom") {
      string namex = name;
      size_t p = namex.rfind('#');
      if (p!=string::npos) {
	detector = namex.substr(0, p);
	namex.erase(0, p+1);
      }
      name = ((DataBase*)this)->LscomName(namex, true);
      p = name.rfind('|');
      if (p!=string::npos)
	name.erase(p);
    }
    string cfname = /*"classdef::"+*/ name;

    if (cset) {
      bool clsfound = cset->find(cfname)!=cset->end();
      if (clsfound != firstcall) 
	return false;
    }

    for (keyword_list::const_iterator i=kwl.begin(); i!=kwl.end(); i++)
      if (cfname == i->first) {
	return false;
      }

    XmlDom k = kw.Element("keyword");
    k.Prop("name", name);
    k.Prop("value", value);
    k.Prop("type", "detection"); // obs! gt=ground_truth, anything else? 
    k.Prop("detector", detector);
    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLcontentitemlist(XmlDom& xml) {
  xmlNodePtr prnt = xml.node;
  xmlNsPtr   ns   = xml.ns;

  if (allcontentsclasses=="")
    return AddToXMLcontentitemlist_random(prnt, ns);

  FindAllClasses(false, false /*?*/);

  xmlNodePtr itemlist = AddTag(prnt, ns, "contentitemlist");

  list<string> allowed = SplitClassNames(allcontentsclasses);

  bool ok = true;
  for (list<string>::const_iterator j=allowed.begin(); j!=allowed.end(); j++) {
    // is this really necessary or faster than calling GroundTruthExpression()
    // anyway... ?
    bool found = false;
    for (size_t i=0; !found && i<ContentsClasses(); i++) {
      const ground_truth& c = Contents(i);
      if (*j==c.Label()) {
	AddToXMLcontentitem(itemlist, ns, c);
	found = true;
      }
    }
    if (!found)
      AddToXMLcontentitem(itemlist, ns, GroundTruthExpression(*j));
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLcontentitemlist_random(xmlNodePtr prnt, xmlNsPtr ns) {
  FindAllClasses(false, false /*?*/);

  xmlNodePtr itemlist = AddTag(prnt, ns, "contentitemlist");

  list<string> allowed;
  if (allcontentsclasses!="")
    allowed = SplitClassNames(allcontentsclasses);
  else
    AddToXMLcontentitem(itemlist, ns, GroundTruthExpression("*"));

  set<string> allowed_set(allowed.begin(), allowed.end());

  for (size_t i=0; i<ContentsClasses(); i++) {
    const ground_truth& c = Contents(i);
    if (allowed_set.empty() ||
	allowed_set.find(c.Label())!=allowed_set.end()) {
      AddToXMLcontentitem(itemlist, ns, c);
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AddToXMLcontentitem(xmlNodePtr list, xmlNsPtr ns,
					   const string& c,
					   int p, int n, int t,
					   const string& d) {
  xmlNodePtr item = AddTag(list, ns, "contentitem");
  AddTag(item, ns, "name",  c);
  AddTag(item, ns, "true",  p);
  AddTag(item, ns, "false", n);
  AddTag(item, ns, "total", p+n);

  // cout << c << " " << p << " " << n << endl;

  if (t) {
    char txt[100];
    sprintf(txt, "%.2f%%", 100.0*(p+n)/t);
    AddTag(item, ns, "cover", txt);
  }
  if (p+n) {
    char txt[100];
    sprintf(txt, "%.2f%%", 100.0*p/(p+n));
    AddTag(item, ns, "apriori", txt);
  }

  if (d!="")
    AddTag(item, ns, "description", d);

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_CONTEXTSTATE
  bool DataBase::AddToXMLcontextstate(XmlDom& xml, const string& namestr,
                                      const string& specstr) const {
    return context.AddToXML(xml, namestr, specstr);
  }
#else
  bool DataBase::AddToXMLcontextstate(XmlDom&, const string&,
                                      const string&) const {
    return true;
  }
#endif // PICSOM_USE_CONTEXTSTATE

  /////////////////////////////////////////////////////////////////////////////

  int DataBase::GetParentVideo(const string& label, const target_type tt) {
    if (tt & target_sound) {
      int pidx = ParentObject(LabelIndex(label));
      pidx = ParentObject(pidx);
      return pidx;
    }
    const vector<int>& p = _objects.find(LabelIndex(label)).parents;
    if (p.size()==0)
      return -1;

    for (size_t i=0; i<p.size(); i++)
      if (ObjectsTargetTypeContains(p[i], target_videofile))
	return p[i];

    for (size_t i=0; i<p.size(); i++) {
      int pidx = GetParentVideo(Label(p[i]), tt);
      if (pidx>=0)
	return pidx;
    }

    return -1;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ExtractMediaClip(size_t idx, const target_type tt,
				  bool use_subdir) {
    // ExtractVideoFrames() should be used instead if tt==target_image

    string msg = "DataBase::ExtractMediaClip("+ToStr(idx)+","+
      TargetTypeString(tt)+") : ";

    bool may_copy_parent = true;
    float min_dur = 2.0; // obs!

    // these are now members with these defaults
    // bool tempmediafiles = true, tempimagefiles = true;

    bool audio = tt & target_sound, video = tt & target_video;
    bool image = tt & target_image, imagesegment = tt & target_imagesegment;
    if (imagesegment)
      image = false;

    const string& label = Label(idx);
    int pidx = -1;
    bool video_parent = false;
    if (audio||video||image) {
      pidx = GetParentVideo(label, tt);
      if (pidx==-1)
	return ShowError(msg, "GetParentVideo("+label+") returned -1");
      video_parent = true;
    }
    pidx = ParentObject(idx);
    if (pidx==-1)
      return ShowError(msg, "ParentObject(#"+ToStr(idx)+") returned -1");

    /* 
    // obs! ObjectsTargetType(pidx) & target_message doesn't work anymore
    string parobj, timepoint, duration;
    if (ObjectsTargetType(pidx) & target_message) {
      if (!SolveObjectSubrangeOrigins(label, parobj, timepoint, duration))
	return ShowError(msg+"SolveObjectSubrange() failed");
    } else {
      if (!SolveObjectSubrange(label, tt, parobj, timepoint, duration))
	return ShowError(msg+"SolveObjectSubrange() failed");
    }
    
    double tp = 0.0, dur = 0.0;
    if (timepoint.find("T")==0 && duration.find("PT")==0) {
      tp  = GetMpeg7TimePoint(timepoint);
      dur = GetMpeg7Duration(duration);
    } else {
      tp  = atof(timepoint.c_str());
      dur = atof(duration.c_str());
    }
    */

    string parobj = SolveObjectPath(Label(pidx), "", "", false, NULL, true);
    if (!FileExists(parobj))
      parobj = ObjectPathEvenExtract(pidx, true);  // obs! local==true?

    double pardur = video_parent ? VideoDuration(pidx) : 0.0;
    bool copy_parent = false;

    double tp_orig = 0, dur_orig = 0;
    if (pardur) {
      auto psd = ParentStartDuration(idx, tt);
      tp_orig  = psd.second.first;
      dur_orig = psd.second.second;
    }
    double tp_mod  = tp_orig, dur_mod = dur_orig;

    if (fabs(dur_mod-pardur)<1.0/15 && may_copy_parent)
      copy_parent = true;

    if (dur_mod<min_dur) {
      tp_mod -= (min_dur-dur_mod)/2;
      dur_mod = min_dur;
      if (tp_mod<0) {
	dur_mod -= tp_mod;
	tp_mod = 0;
      }
      // obs! what happens if tp_mod+dur_mod exceeds parent's duration?
    }

    auto oi = ReadOriginsInfo(idx, false, true);
    string filename = oi["filename"];
    size_t p = filename.rfind('/');
    if (p!=string::npos)
      filename.erase(0, p+1);
    p = filename.find('.');
    string ext = p!=string::npos ? filename.substr(p) : "";

    // string subdir = SqlObjects() ? "" : LabelSubDir(label);
    string subdir = use_subdir ? LabelSubDir(label) : "";
    string opath  = SolveObjectPath(label, subdir, "",
				    true, NULL, true)+ext;
 
    string finalname = opath, destname = opath, tempname;
    if (UseSql()) {
      string tdir = TempDir("media_extracted");
      finalname = tdir+"/"+label+ext;
      if (SqlObjects() || tempimagefiles)
	destname = ConvertGlobalToLocalDiskName(destname);
    }

    if (tempmediafiles)
      parobj = CopyFileFromGlobalToLocalDisk(parobj);

    if (audio)
      copy_parent = false;

    if (imagesegment) {
      string s = oi["url"];
      size_t p = s.find('(');
      if (p==string::npos)
	return ShowError(msg, "(bbox) missing in ["+s+"]");
      string plabel = s.substr(0, p);
      if (plabel!=Label(pidx))
	return ShowError(msg, "parent object does not match in ["+s+"]");
      string spec1 = "0x0";
      string spec2 = "extractbox"+s.substr(p);
      //string spec2 = "showbox"+s.substr(p);
      imagedata img = CreateVirtualImage(Label(idx), spec1, spec2);
      tempname = TempFile(label+".jpeg"); // obs!
      imagefile::write(img, tempname, "image/jpeg"); // obs!

    } else if (copy_parent) {
      tempname = TempFile("copied.file");
      CopyFile(parobj, tempname);

    } else {
      videofile vf(parobj, false, TempDir("video-tmp"));

      /// obs!  this shoud be database-specific!
      video_codec vcodec = vcodec_mpeg4; // vcodec_copy
      try {
	tempname = vf.extract_video_segment_to_tmp(ext, tp_mod, dur_mod, vcodec,
						   acodec_copy, VF_FFMPEG, "",
						   TempDir("tmp-video"));
	if (audio) {
	  string wavname = vf.extract_audio(tempname);
	  Unlink(tempname);
	  tempname = wavname;
	}
      } catch (const string& emsg) {
	return ShowError(msg, "extracting video segment from <",
			 ShortFileName(parobj), "> failed");
      } 
    }

    if (!extractobjectpath.empty())
      destname = extractobjectpath+"/"+label+MediaExt(tt);

    Picsom()->MakeDirectory(destname, true);

    string mvcmd = "mv "+tempname+" "+destname; // Rename()???
  
    if (Picsom()->ExecuteSystem(mvcmd, DebugVirtualObjects(), false, true)) {
      Unlink(tempname);
      return ShowError(msg, "could not move file <", tempname,
		       "> to ", destname);
    } else if (DebugVirtualObjects())
      cout << TimeStamp() << msg << "succesfully moved file <" << tempname
	   << "> to <" << ShortFileName(destname) << ">" << endl;
  
    WriteLog((copy_parent?"Copied ":"Extracted ")+
	     ToStr(FileSize(destname))+" bytes for "+
	     (audio ? "audio " : image ? "image " : video ? "video "
	      : imagesegment ? "image segment " : "unknown ")+label+" from <"+
	     ShortFileName(parobj)+"> to <"+ShortFileName(destname)+">");
    AddCreatedFile(destname);

    if (destname != finalname)
      Symlink(destname, finalname);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SolveObjectSubrange(const string& label, target_type tt,
				     string& parobj, string& timepoint,
				     string& duration) {
    string msg = "DataBase::SolveObjectSubrange("+label+",...) : ";

    int pidx = GetParentVideo(label, tt);
    const string& parent_label = Label(pidx);
				    
    if ((ObjectsTargetType(pidx) & target_video) == 0)
      return ShowError(msg, "parent ", parent_label, " is bad.");

    parobj = SolveObjectPath(parent_label, "", "", false, NULL, true);
    if (parobj=="")
      return ShowError(msg, "SolveObjectPath() for parent failed");

    string tmp;
    return SolveObjectSubrangeOrigins(label, tmp, timepoint, duration) ||
      SolveObjectSubrangeXML(label, pidx, timepoint, duration);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SolveObjectSubrangeOrigins(const string& label,
					    string& parobj,
					    string& timepoint, 
					    string& duration) {
    string msg = "DataBase::SolveObjectSubrangeOrigins("+label+",...) : ";

    size_t idx = LabelIndex(label);
    map<string,string> ohash = ReadOriginsInfo(idx, false, true);
    string subrange = ohash["subrange"];
    if (subrange=="")
      return false; // ShowError(msg, "subrange not defined");

    int pidx = ParentObject(idx);
    parobj = pidx>=0 ? SolveObjectPath(Label(pidx)) : "";

    size_t p  = subrange.find('/');
    if (p!=string::npos) {
      timepoint = subrange.substr(0, p);
      duration  = subrange.substr(p+1);
      if (timepoint=="" || duration=="")
	return ShowError(msg, "empty part");
      return true;
    }

    size_t pframes = 0;
    float fps = 0;
    string dim = ohash["dimensions"];
    size_t at = dim.find('@');
    if (at!=string::npos)
      fps = atof(dim.substr(at+1).c_str());
    while (!fps && pidx>=0) {
      fps = FpsCache(pidx);
      if (!fps) {
	map<string,string> parohash = ReadOriginsInfo(pidx, false, true);
	string podim = parohash["dimensions"];
	size_t poat = podim.find('@');
	if (poat!=string::npos) {
	  fps = atof(podim.substr(poat+1).c_str());
	  if (fps) {
	    string s = podim.substr(0, poat);
	    vector<string> dd = SplitInSomething("x", false, s);
	    if (dd.size()==3)
	      pframes = atoi(dd[2].c_str());
	  }
	}
        int ppidx = ParentObject(pidx);
	if (!fps && ppidx<0) {
	  string opath  = SolveObjectPath(Label(pidx));
	  string tmpdir = TempDir("objsubranges", true);
	  videofile fv(opath, false, tmpdir);
	  fps = fv.get_frame_rate(); // obs! this should be stored...
	  FpsCache(pidx, fps);
	}
	pidx = ppidx;
      }
    }
    if (!fps) 
      return ShowError(msg, "frame rate not solved");

    p = subrange.find('-');
    if (p!=string::npos) {
      string sframe = subrange.substr(0, p);
      string eframe = subrange.substr(p+1);
      if (sframe=="" || eframe=="")
	return ShowError(msg, "empty part");
      int sf = atoi(sframe.c_str()), ef = atoi(eframe.c_str());
      timepoint = ToStr(sf/fps); // check the format
      duration  = ToStr((ef-sf+1)/fps); // check the format...
      return true;

    } else if (subrange.find_first_not_of("0123456789")==string::npos) {
      int f = atoi(subrange.c_str());
      timepoint = ToStr(f/fps); // check the format
      duration  = "0";

    } else if (subrange.find("T")==0) {
      size_t p = subrange.find("PT");
      timepoint = p==string::npos ? subrange   : subrange.substr(0, p);
      duration  = p==string::npos ? "PT0S0N1F" : subrange.substr(p);

    } else if (subrange=="*") {
      timepoint = "0";
      duration  = ToStr(pframes/fps);

    } else
      return ShowError(msg, "could not interpret \""+subrange+"\"");

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::SolveObjectSubrangeXML(const string& label, int pidx,
				      string& timepoint, string& duration) {
  string msg = "DataBase::SolveObjectSubrangeXML("+label+",...) : ";

  const string& parent_label = Label(pidx);

  string xmlfile   = Mpeg7MCXML(pidx);
  string mov_code  = RemoveZeros(parent_label.substr(1,3));  // trecvid...
  string shot_code = RemoveZeros(label.substr(4,4));         // trecvid...
  string seek_id   = "shot"+mov_code+"_"+shot_code;          // trecvid...

  if (!FileExists(xmlfile))
    return ShowError(msg, "Mpeg7MCXML file <"+xmlfile+"> inexistent");

  if (DebugVirtualObjects())
    cout << TimeStamp() << msg << "fetching subrange for <"+seek_id+"> from <"+
      ShortFileName(xmlfile)+">" << endl;

  xmlDocPtr   doc = xmlParseFile(xmlfile.c_str());
  xmlNodePtr root = doc ? xmlDocGetRootElement(doc) : NULL;
  xmlNodePtr node = XMLSearchNode(root, "VideoSegment", "id", seek_id);
  if (!node) {
    xmlFreeDoc(doc);
    return ShowError(msg, "XML node not found");
  }

  xmlNodePtr timenode = XMLSearchNode(node, "MediaTime");

  if (!timenode) {
    xmlFreeDoc(doc);
    return ShowError(msg, "XML timenode not found");
  }

  xmlNodePtr tpnode  = XMLSearchNode(timenode, "MediaTimePoint");
  xmlNodePtr durnode = XMLSearchNode(timenode, "MediaDuration");

  XMLS ctp  = xmlNodeListGetString(doc,  tpnode->xmlChildrenNode, 1);
  XMLS cdur = xmlNodeListGetString(doc, durnode->xmlChildrenNode, 1);
  timepoint = string((char *)ctp);
  duration  = string((char *)cdur);

  xmlFreeDoc(doc);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

xmlNodePtr DataBase::XMLSearchNode(xmlNodePtr node, 
					   const string& name,
					   const string& pname, 
					   const string& pval) {
  for (xmlNodePtr cur = node ? node->xmlChildrenNode : NULL;
       cur; cur = cur->next) {
    string prop = pname!="" ? GetProperty(cur, pname) : "";
    
    if (NodeName(cur)==name && (pname=="" || (prop!="" && prop==pval)))
      return cur;
    
    xmlNodePtr p = XMLSearchNode(cur, name, pname, pval);
    if (p)
      return p;
  }

  return NULL;
}

  /////////////////////////////////////////////////////////////////////////////

  double DataBase::GetMpeg7TimeValue(const string& str,
				     string regex_pattern) {
    const int SUBS = 5;
    RegExp re(regex_pattern);
    int t[SUBS];
    
    vector<RegExpMatch> m = re.match(str,SUBS);
    if (!re.ok()) return -1; 
    
    for (int i=0; i<SUBS; i++) 
      t[i] = atoi(m[i].c_str());
    
    return t[0]*60*60 + t[1]*60 + t[2] + double(t[3])/t[4];
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CreateVirtualImage(ostream& os, const string& name,
                                    const string& spec) const {

    imagedata imgd = CreateVirtualImage(name, spec);

    if (imgd.iszero()) //!isempty
      return ShowError("DataBase::CreateVirtualImage() : imgd empty");

    os << imagefile::stringify(imgd, "image/png");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateVirtualImage(const string& name,
					 const string& spec1,
					 const string& spec2) const {
    string msg = "DataBase::CreateVirtualImage("+name+","+spec1+","+
      spec2+") : ";

    if (Is8plus8(name))
      return CreateVirtualImage8plus8(name, spec1);

    if (DebugVirtualObjects())
      cout << "Now in "+msg << endl;

    imagedata imgd;

    int width = 0, height = 0;
    if (sscanf(spec1.c_str(), "%dx%d", &width, &height)!=2) {
      ShowError(msg+"could not parse WxH");
      return imgd;
    }

    string method, imagename, frame, segment;
    if (!SplitLabel(name, method, imagename, frame, segment)) {
      ShowError(msg+"SplitLabel(", name, ") failed");
      SplitLabel(name, method, imagename, frame, segment);
      return imgd;
    }
  
    if (method!="") {
      string specx; // obs! other arguments except WxH should be passed
      imgd = CreateSegmentImage(name, specx);
      if (imgd.iszero()) //!isempty
	return imgd;

    } else if (spec2!="") {
      int idx = LabelIndex(name);
      if (idx<0) {
	ShowError(msg+"label index error");
	return imgd;
      }
      int pidx = ParentObject(idx);
      if (pidx<0) {
	ShowError(msg+"parent index error");
	return imgd;
      }
      imagedata img = ((DataBase*)this)->ImageData((size_t)pidx);
      if (img.empty()) {
	ShowError(msg+"image reading error");
	return imgd;
      }
      if (spec2.find("showbox(")==0 || spec2.find("extractbox(")==0) {
	size_t p = spec2.find("(");
	string m = spec2.substr(0, p);
	int tlx = 0, tly = 0, brx = 0, bry = 0;
	if (sscanf(spec2.substr(p+1).c_str(), "%d,%d,%d,%d",
		   &tlx, &tly, &brx, &bry)!=4)  {
	  ShowError(msg+"image coordinate error");
	  return imgd;
	}
	if (m=="showbox") {
	  size_t w = 3;
	  string c = "red";
	  img.convert(imagedata::pixeldata_uchar);
	  img.force_three_channel();
	  img.line(tlx,   tly,   tlx,   bry, w, c);
	  img.line(tlx,   bry, brx, bry, w, c);
	  img.line(brx, bry, brx, tly,   w, c);
	  img.line(brx, tly,   tlx,   tly,   w, c);
	  imgd = img;
	}
	if (m=="extractbox")
	  imgd = img.subimage(tlx, tly, brx, bry);
      }

    } else {
      bool exact = false;
      string ifile = SolveObjectPath(name, "", "", false, &exact);
      imagefile f(ifile);
      int framen = 0;
      if (!exact && frame!="")
	framen = atoi(frame.c_str());

      if (DebugVirtualObjects())
	cout << " objectpath=" << ifile << " exact=" << exact << " framen="
	     << framen  << endl;

      imgd = f.frame(framen);
    }

    if (width && height) {
      scalinginfo scale(imgd.width(), imgd.height(), width, height);  
      imgd.rescale(scale, 1);
    }

    return imgd;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateTimelineImage(size_t sidx,
					  const string& specin) {

    string msg = "DataBase::CreateTimelineImage("+ToStr(sidx)+","+specin+") : ";

    bool debug = false;

    map<string,string> specs;
    vector<string> ss = SplitInCommasObeyParentheses(specin);
    for (auto i=ss.begin(); i!=ss.end(); i++)
      if (i->find("=")!=string::npos)
	specs.insert(SplitKeyEqualValueNew(*i));

    string hitlab = Label(sidx);

    string segm = hitlab;
    size_t p = segm.find(':');
    if (p!=string::npos)
      segm.erase(p);
    else 
      segm = specs["segm"];

    size_t pidx = sidx;
    if (ParentObject(sidx, false)>=0)
      pidx = ParentStartDuration(sidx, target_videosegment).first;

    string det = specs["detection"];
    p = det.find("HASH");
    if (p!=string::npos)
      det.replace(p, 4, "#");

    if (debug)
      cout << msg << "STARTING pidx=" << pidx << " segm=\""
	   << segm << "\" hitlab=<" << hitlab << "> det="
	   << det << endl;

    imagedata imgd(99, 29);
    vector<float> black { 0, 0, 0 };
    vector<float> white { 1, 1, 1 };
    vector<float> grey  { .5, .5, .5 };
    vector<float> red { 1, 0, 0 };
    for (size_t i=0; i<imgd.width()*imgd.height(); i+=2) {
      size_t x = i%imgd.width(), y = i/imgd.width();
      imgd.set(x, y, red);
    }

    vector<vector<float> > colset { black, grey };

    float fps = VideoFrameRate(pidx);
    float dur = VideoDuration(pidx);
    if (dur) {
      size_t sec = ceil(dur), w = sec, h = 100;
      if (w>700)
	w = 700;
      
      imagedata im(1, 1);
      im.set(0, 0, white);
      scalinginfo si(1, 1, w, h);
      si.stretch(true);
      im.rescale(si);

      const string& label = Label(pidx);

      vector<size_t> idxvec;

      float maxval = 0;
      map<size_t,float> frval;
      if (det!="") {
	auto ff = VideoOrSegmentFramesOrdered(pidx);
	for (auto i=ff.begin(); i!=ff.end(); i++) {
	  bool dummy = true;
	  auto d = RetrieveDetectionData(i->first, det, true, dummy);
	  if (d.size()==1 && d.begin()->second.size()==1) {
	    float v = d.begin()->second[0];
	    if (v>maxval)
	      maxval = v;
	    frval[i->second] = v;
	  }
	}
	if (maxval>0 && maxval<1)
	  maxval = 1;
      }

      for (size_t i=0; i<10000000; i++) {
	string slab = segm+":"+label+":t"+ToStr(i);
	int sidx = LabelIndexGentle(slab, false);
	if (sidx<0)
	  break;
	idxvec.push_back(sidx);
      }

      size_t coli = 0;
      for (auto i=idxvec.begin(); i!=idxvec.end(); i++, coli++) {
	auto psd = ParentStartDuration(*i, target_video);
	float sstart = psd.second.first, sdur = psd.second.second;

	//float sstart = ParentRelativeStartTime(*i);
	//float sdur   = VideoDuration(*i);

	size_t startx = floor(sstart/dur*w);
	size_t endx   = ceil((sstart+sdur)/dur*w);

	if (debug)
	  cout << "  " << Label(*i) << " : " << sstart << " "
	       << sdur << " " << startx << " " << endx << endl;

	auto col = colset[coli%colset.size()];
	if (Label(*i)==hitlab)
	  col = red;

	if (frval.size()==0 || Label(*i)==hitlab)
	  for (size_t x=startx; x<endx; x++)
	    for (size_t y=0; y<h; y++)
	      im.set(x, y, col);

	if (frval.size() && maxval>0) {
	  col = colset[coli%colset.size()];
	  size_t f0 = floor(sstart*fps), f1 = ceil((sstart+sdur)*fps);
	  for (size_t f=f0; f<f1; f++) {
	    float v = frval[f];
	    size_t l = ceil(v/maxval*h);
	    size_t x = floor(f*w/(fps*dur));
	    for (size_t y=h-l; y<h; y++)
	      im.set(x, y, col);
	  }	    
	}
      }

      for (size_t x=0; x<im.width(); x++) {
	im.set(x,             0, red);
	im.set(x, im.height()-1, red);
      }

      imgd = im;
    }

    if (debug)
      cout << msg << "ENDING" << endl;

    return imgd;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateImage(const string& lab,
				  const string& specin) const {

    string msg = "DataBase::CreateImage("+lab+","+specin+") : ";

    string aspec = specin;
    string spec  = aspec[0]=='a' ? aspec.substr(1) : aspec;

    imagedata img;

    if (IsVirtualImage(lab)) {
      img = CreateVirtualImage(lab, spec); // might not understand "a120x90"
      if (img.iszero()) //!isempty
	ShowError(msg+"CreateVirtualImage("+lab+","+spec+") failed");
      return img;
    }

    /*
    string segm, imgn, frameno, segno;
    if (!SplitLabel(lab, segm, imgn, frameno, segno)) {
      ShowError(msg+"SplitLabel("+lab+") failed");
      return img;
    }
    int fnum = frameno=="" ? 0 : atoi(frameno.c_str());

    string filename = SolveObjectPath(lab);
    if (filename=="") {
      ShowError(msg+"SolveObjectPath("+lab+") failed");
      return img;
    }

    // pic_som->GetTicTac()->Verbose(5);
    // pic_som->GetTicTac()->Log(cout);

    Tic("CreateImage/frame");

    imagefile f(filename, imagedata::pixeldata_uchar);
    img = f.frame(fnum);
    if (img.isempty())
      ShowError(msg+"imagefile("+filename+") frame="+ToStr(fnum)+" failed");

    Tac("CreateImage/frame");
    */

    int idx = LabelIndex(lab);
    DataBase *db = (DataBase*)this;

    pair<string,string> vtp = db->VirtualThumbnailPath(idx);
    if (vtp.first!="") {
      imagefile f(vtp.first, imagedata::pixeldata_uchar);
      img = f.frame(0);
      return img;
    }

    img = db->ImageData(idx);

    pic_som->GetTicTac()->Verbose(0);

    if (spec!="") {
      int w = img.width(), h = img.height();
      if (sscanf(spec.c_str(), "%dx%d", &w, &h)!=2 || w<1 || h<1)
	ShowError(msg+"spec does not contain WxH");

      if (aspec[0]=='a') {
	double xm = double(w)/img.width(), ym = double(h)/img.height();
	double m = xm<ym ? xm : ym;
	w = (int)floor(m*img.width() +0.5);
	h = (int)floor(m*img.height()+0.5);
      }

      Tic("CreateImage/rescale");

      scalinginfo si(img.width(), img.height(), w, h);
      img.rescale(si, 1);

      Tac("CreateImage/rescale");
    }

    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::CreateVirtualImage8plus8(const string& name,
                                               const string&
                                               spec) const {
    if (DebugVirtualObjects())
      cout << "Now in CreateVirtualImage8plus8(" << name << ")" << endl;

    int width = 0, height = 0;
    if (sscanf(spec.c_str(), "%dx%d", &width, &height)!=2) {
      ShowError("DataBase::CreateVirtualImage() : coud not parse WxH");
      return imagedata();
    }

    char tmp[1024];
    strcpy(tmp, name.c_str());
    char *ptr = strchr(tmp, '+');
    if (!ptr)
      return imagedata();

    *ptr = 0;

    const char *nf = " -- not found -- ";
    const char /* *d = objdir.c_str(),*/ *s = "segments", *p="km+bi+pl+rh+am:";
    char i1[1024], i2[1024];
    string obj = SolveObjectPath(tmp);
    strcpy(i1, obj!=""?obj.c_str():nf);

    obj = SolveObjectPath(ptr+1);
    strcpy(i2, obj!=""?obj.c_str():nf);

    char s1[1024], s2[1024];
    obj = SolveObjectPath(tmp,   s, p);
    strcpy(s1, obj!=""?obj.c_str():nf);

    obj = SolveObjectPath(ptr+1, s, p);
    strcpy(s2, obj!=""?obj.c_str():nf);

    //  cout << "NOW in DataBase::CreateVirtualImage(" << i1 << ","
    //       << s1 << " , " << i2 << "," << s2 << ")" << endl;

    struct stat st;

    if (stat(i1, &st)||stat(s1, &st)||stat(i2, &st)||stat(s2, &st))
      return imagedata();

    //  Segmentation::Verbose(2);

    /*SegmentInterface seg1(s1,i1);
      SegmentInterface seg2(s2,i2);*/
    picsom::segmentfile segf1(i1,s1);
    picsom::segmentfile segf2(i2,s2);

    //  cout << "Interface objects created." << endl;

    picsom::coord top_mark_1,top_mark_2,bottom_mark_1,bottom_mark_2,c;
    picsom::coord top_nw,top_se,bottom_nw,bottom_se;

    if(!segf1.getAlignmentMarks(c,c,top_mark_1,top_mark_2) ||
       !segf2.getAlignmentMarks(bottom_mark_1,bottom_mark_2,c,c))
      // alignment marks not found
      return imagedata();

    //   cout << name << ":" << endl;
    //   cout << "AlignmentMarks " << top_mark_1 << " " << top_mark_2 << endl;
    //   cout << bottom_mark_1 << " " << bottom_mark_2 << endl;
  
    // force horisontal alignment lines
    top_mark_2.y = top_mark_1.y = (top_mark_2.y+ top_mark_1.y)/2; ; 
    bottom_mark_2.y = bottom_mark_1.y = (bottom_mark_2.y+ bottom_mark_1.y)/2;  

    // find bounding boxes of objects
    if (!segf1.FindBox(top_nw,top_se)) {
      ShowError("DataBase::CreateVirtualImage(..., ", name, ",...) "
                "object not found in <", s1, ">");
      return imagedata();
    }
    if (!segf2.FindBox(bottom_nw,bottom_se)) {
      ShowError("DataBase::CreateVirtualImage(..., ", name, ",...) "
                "object not found in <", s2, ">");
      return imagedata();
    }

    top_nw.y = min(top_mark_1.y,top_nw.y);
    top_se.y = max(top_mark_1.y,top_se.y);

    bottom_nw.y = min(bottom_mark_1.y,bottom_nw.y);
    bottom_se.y = max(bottom_mark_1.y,bottom_se.y);

    // determine relative scaling of the boxes

    double top_scaling = ((double)(bottom_mark_2.x-bottom_mark_1.x))/
      (top_mark_2.x-top_mark_1.x);
    double bottom_scaling=1.0;

    double sourceheight 
      = (top_mark_1.y-top_nw.y)*top_scaling+(bottom_se.y-bottom_mark_1.y)*bottom_scaling;

    double sourcewidth 
      = max((-top_nw.x+top_mark_1.x)*top_scaling,
            (-bottom_nw.x+bottom_mark_1.x)*bottom_scaling) 
      +  max((top_se.x-top_mark_1.x)*top_scaling,
             (bottom_se.x-bottom_mark_1.x)*bottom_scaling) ;

    double heightscaling=1.0, widthscaling=1.0;

    if(width  && sourcewidth > width) widthscaling=width/sourcewidth;
    if(height && sourceheight > height) heightscaling=height/sourceheight;

    if(widthscaling < heightscaling){
      top_scaling *= widthscaling;
      bottom_scaling *= widthscaling;
      sourcewidth *= widthscaling;
      sourceheight *= widthscaling;
    }
    else{
      top_scaling *= heightscaling;
      bottom_scaling *= heightscaling;
      sourcewidth *= heightscaling;
      sourceheight *= heightscaling;
    }

    if (height==0) height = (int)ceil(sourceheight);
    if (width==0)  width  = (int)ceil(sourcewidth);

    // cout << "width=" << width << " height=" << height 
    //         << "{top,bottom}_scaling " << top_scaling << " " 
    //         << bottom_scaling << endl
    //         << "sourcewidth=" << sourcewidth << " sourceheight=" 
    //         << sourceheight << endl;

    int top_margin  = (int)((height-sourceheight)/2);
    int left_margin = (int)((width-sourcewidth)/2);

    //   cout << "top_margin=" << top_margin
    //        << " left_margin=" << left_margin << endl;

    double top_begin_x,bottom_begin_x,bottom_begin_y;

    top_begin_x=top_nw.x;
    bottom_begin_x=bottom_nw.x;

    if((top_mark_1.x-top_nw.x)*top_scaling > 
       (bottom_mark_1.x-bottom_nw.x)*bottom_scaling)
      // top part goes farther left -> we insert empty space in bottom
      bottom_begin_x -= (top_mark_1.x-top_nw.x)*top_scaling/bottom_scaling - 
        (bottom_mark_1.x-bottom_nw.x);
    else
      // the other way around
      top_begin_x -= (bottom_mark_1.x-bottom_nw.x)*bottom_scaling/top_scaling
        -(top_mark_1.x-top_nw.x);

    int l=width*height*3;
  
    imagedata imgd = imagedata(width, height, 3, imagedata::pixeldata_uchar);

    //  cout << "Image object created. (w,h)=("<<width<<","<<height<<")" << endl;

    //  unsigned char *result_data = (unsigned char *)img->getDataPtr();
    vector <unsigned char> result_data = imgd.get_uchar();
 
    for(int i=0;i<l;i++)
      result_data[i] = 255; // set background to white

    bottom_begin_y = bottom_nw.y;

    bottom_begin_y -= (top_mark_1.y-top_nw.y)*top_scaling -
      floor((top_mark_1.y-top_nw.y)*top_scaling);
  
    int tgt_start_y = (int)(top_margin+(top_mark_1.y-top_nw.y)*top_scaling
                            -(bottom_mark_1.y-bottom_nw.y)*bottom_scaling);

    int tgt_stop_y = (int)(tgt_start_y + 
                           (bottom_se.y-bottom_nw.y)*bottom_scaling);

    segf2.CopyPixelsWithMask
      (bottom_begin_x,bottom_begin_y ,bottom_scaling, true,
       result_data,width,left_margin,
       tgt_start_y,left_margin+(int)sourcewidth,tgt_stop_y);

    segf1.CopyPixelsWithMask
      (top_begin_x,top_nw.y,top_scaling,true,
       result_data,width,left_margin,top_margin,
       left_margin+(int)sourcewidth,
       top_margin+int((top_se.y-top_nw.y)*top_scaling));

    //  cout << "Image data copied." << endl;

    //*result_img = img;

    //return true;

    imgd.set(result_data);

    return imgd;
  }

  /////////////////////////////////////////////////////////////////////////////

  keyword_list DataBase::FindKeywords(size_t idx, bool allow, bool force) {
    if (DebugGroundTruth())
      cout << "Now in DataBase::FindKeywords(" << idx << "," << allow
           << "," << force << ") : "
           << "ContentsClasses()=" << ContentsClasses() << endl;

    if (force||allow)
      FindAllClasses(force, false/*?*/);

    keyword_list list;
    for (size_t i=0; i<ContentsClasses(); i++) {
      int val = Contents(idx, i);
      if (val) //was: if (Contents(idx, i)==1) 
        list.push_back(pair<string,float>(ContentsClassLabel(i), (float)val));
    }

    return list;
  }

///////////////////////////////////////////////////////////////////////////////

const char *DataBase::FindCookieP(const char *userpw) const {
  for (int i=0; i<access.Nitems(); i++) {
    const char *cookie = access[i].FindCookieP(userpw);
    if (cookie) {
//       cout << "DataBase::FindCookie(" << userpw << ") " << name
// 	   << " found cookie [" << cookie << "]" << endl;
	
      return cookie;
    }
  }

  return NULL;  
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::LabelTest() {
  Tic("LabelTest");
  size_t corr = 0;
  for (size_t i=0; i<Size(); i++) {
    string l = Label(i);
    int j = LabelIndex(l);
    if (int(i)==j)
      corr++;
    else
      cout << " *** error : " << i << " -> " << l << " -> " << j << endl;
  }
  Tac("LabelTest");

  cout << "There were " << corr << " correct label/index pairs out of total "
       << Size() << endl;

  return corr==Size();
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::PrepareClassification(const string& metacls, bool del,
                                       const vector<string>& feat, bool vec,
                                       bool svm) {
    return PrepareClassification(SplitClassNames(metacls), del, feat,
                                 vec, svm);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::PrepareClassification(const list<string>& cls, bool del,
                                       const vector<string>& feat, bool vec,
                                       bool svm) {
    if (del)
      DeleteClassModels();

    WriteLog("Preparing class models ", CommaJoin(cls));

    if (DefaultQuery().DefaultMatrixCount()!=3) {
      DefaultQuery().DefaultMatrixCount(3);
      WriteLog("Setting to use 3 matrices in queries");
    }

    //  ReadDataLabels(false);

    list<string> classes(cls.begin(), cls.end());
    if (classes.size()==1 && *classes.begin()=="*") {
      FindAllClasses(false, true);
      classes = KnownClassNames();
    }

    vector<string> featv = feat;
    if (featv.empty())
      featv = SplitInCommasObeyParentheses(DefaultIndices());

    for (list<string>::const_iterator i=classes.begin(); i!=classes.end(); i++)
      AddClassModel(*i, featv, false, vec, svm);

    return NclassModelsAll()>0;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::AddClassModel(const string& name, const vector<string>& feat, 
                               bool negative, bool vectors, bool /*svm*/) {
    stringstream msg;
    msg << name << " [" << CommaJoin(feat) << "]";

    Query *q = ClassModel(name);
    if (!q) {
      int other     = -1;           // obs! hard-coded
      target_type t = target_file;  // obs! hard-coded
      bool expand   = false;        // obs! hard-coded

      ground_truth gt = GroundTruthExpression(name, t, other, expand);
      ground_truth zero(gt.Length()); 
    
      int nneg = 0, nzero = 0, npos = 0;
      gt.TernaryCounts(nneg, nzero, npos);
      msg << " (" << npos << " positives /" << gt.Length() << ")";
  
      q = new Query(pic_som);
      class_models_new.push_back(q);

      q->SetIdentity(name);
      q->CopySome(DefaultQuery(), true, false, false);

      // obs! HAS_CBIRALGORITHM_NEW problem:
      q->UnselectIndices(NULL, false); // added 2009-11-03

      if (negative)
        q->MarkAsSeenEmptyAspect(&zero, &gt, NULL);
      else
        q->MarkAsSeenEmptyAspect(&gt, &zero, NULL);

      q->ExpandDown("");       // obs! should this be below too?
    }

    // In 2.237 this was changed and since then it has been unsure
    // whether SelectIndex() or SelectIndices() should be called.
    // Now boolean argument #4 to SelectIndices() can be used to tell
    // that the existing indices will not be unselect.
    // obs! HAS_CBIRALGORITHM_NEW problem:
    q->SelectIndices(NULL, feat, false, false);

    // There used to be a conditional "return true" here to avoid do the
    // following steps if the class model already existed.  Now it is not
    // so simple as we have the obs! HAS_CBIRALGORITHM_NEW problem...
    
    q->ReadFiles();
    //  q->DeleteIndexData();  // this might later be removed...
    q->CreateIndexData(false);

    for (size_t i=0; i<q->Nindices(); i++) {
      if (!q->IsTsSom(i))
        continue;

      VectorIndex::State& vis = q->IndexDataVectorIndex(i);
      if (vectors && vis.sample.Nitems()==0) {
	bool force = false;
	vis.Vectorindex().ReadDataFile(force);
	vis.Vectorindex().SetDataSetNumbers(false, true);
	const FloatVectorSet& data = vis.Data();
	for (int s=0; s<data.Nitems(); s++) {
	  size_t oidx = data.Get(s)->Number();
	  if (q->IsSeen(oidx))
	    vis.AddSample(*data.Get(s));
	}
        if (Query::DebugClassify())
          cout << "*** extracted samples for class model "
               << q->Identity() << "/" << q->IndexFullName(i) 
               << " samples: " << vis.sample.Nitems() << endl;
      }

      int l = q->Nlevels(i)-1, k = 0;
      if (l<0) {
        // ShowError("Creating class model failing");
        if (Query::DebugClassify())
	  cout << "*** skipping creation of TSSOM  class model "
               << q->Identity() << "/" << q->IndexFullName(i)  << endl;
        continue;
      }
      simple::FloatMatrix& m = q->Convolved(i, l, k);
      if (m.Size()==0) {
        q->PlaceSeenOnMap(i, l);
        q->Convolve(i, l, k);

        if (Query::DebugClassify())
          cout << "*** calculated TSSOM convolution for class model "
               << q->Identity() << "/" << q->IndexFullName(i) 
               << " MAX: " << m.Maximum() << " MIN: " << m.Minimum() << endl;
      }
    }

    WriteLog("Created class model for ", msg.str());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query *DataBase::ClassModel(const string& n) const { 
    for (size_t i=0; i<NclassModelsAll(); i++) {
      Query *q = ClassModel(i);
      if (n==q->Identity())
        return q;
    }
    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::DeleteClassModels() {
    for (size_t i=0; i<class_models_new.size(); i++)
      delete class_models_new[i];
    class_models_new.clear();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DeleteClassModel(const string& m) {
    for (size_t i=0; i<class_models_new.size(); i++)
      if (ClassModel(i)->Identity()==m) {
	delete class_models_new[i];
	class_models_new.erase(class_models_new.begin()+i);
	return true;
      }

    return false;
  }

///////////////////////////////////////////////////////////////////////////////

string DataBase::DefaultIndices() const {
  string ret;
  if (defaultquery)
    for (size_t i=0; i<defaultquery->Nindices(); i++)
      ret += string(ret.empty()?"":",") + defaultquery->IndexFullName(i);
  
  return ret.empty() ? "*" : ret;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::GetSubstring(const string& str, string& result, 
				    int start, int stop) {

  if (start == -1 || stop == -1) return 0;
  result = str.substr(start,stop-start);
  return 1;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SetLabelRegExps() {
    string lls = ToStr(LabelLength());

    string meth = "^([a-zA-Z][a-zA-Z0-9+-]+:)?";
    string lab1 = "([0-9]{"+lls+"})";
    // string lab2 = "([0-9]{"+lls+"}:[0-9]+)";
    string lab2 = "([0-9]{"+lls+"}:[a-zA-Z]+[0-9]+)";
    string frme = "(:[-0-9a-zA-Z]+)?";
    string segm = "(_[0-9a-zA-Z,\\+]+)?$";

    string pat1 = meth+lab1+frme+segm;
    string pat2 = meth+lab2+frme+segm;

    label_re1.comp(pat1);
    label_re2.comp(pat2);
    if (!label_re1.ok() || !label_re2.ok())
      return ShowError("DataBase::SetLabelRegExps() failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SplitLabel(const string& label, string& segmethod, 
			    string& imagename, 
			    string& frame, string& segment, 
			    bool multiframe) const {

    const RegExp& prec = multiframe ? label_re2 : label_re1;
    RegExp& pre = *(RegExp*)&prec; // a naughty, naughty trick...
    vector<RegExpMatch> pm = pre.match(label, 4);

    if (!pre.ok() || pm.empty())
      return false;
  
    if (pm[0].success())
      segmethod = pm[0].str().substr(0, pm[0].str().length()-1);
    if (pm[1].success())
      imagename = pm[1].str();
    if (pm[2].success())
      frame = pm[2].str().substr(1);
    if (pm[3].success())
      segment = pm[3].str().substr(1);

    //   cout << "SPLIT (" << label << "): " << segmethod << " " << imagename 
    //        << " " << frame << " " << segment << (multiframe?" (multi)":"") 
    //        << endl;
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::LabelFileNamePart(const string& label) const {
    if (label.size()==LabelLength() &&
	label.find_first_not_of("0123456789")==string::npos)
      return label;

    string segmethod, imagename, frame, segment;
    if (SplitLabel(label, segmethod, imagename, frame, segment)) 
      return imagename;

    else if (label.length()==17 && label[8]=='+') // 8plus8 virtual
      return label;

    else
      return ShowErrorS("LabelFileNamePart("+label+") failing");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::IsLabel(const string& label) const {
    string segmethod, imagename, frame, segment;
    return SplitLabel(label, segmethod, imagename, frame, segment);
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ParentObjectStringByPruning(const string& l) const {
    if (l=="")
      return ShowErrorS("DataBase::ParentObjectStringByPruning() :"
			" empty label");

    string method, body, frame, segm;

    if (SplitLabel(l, method, body, frame, segm)
	|| SplitLabel(l, method, body, frame, segm, true)) {

      if (frame!="" && segm!="")
	return body+":"+frame;

      if (segm=="") {
	size_t p = frame.find("kf");
	if (p!=string::npos && p!=0)
	  return body+":"+frame.substr(0, p);
      }

      return body;
    }

    if (l[0]>='0' && l[0]<='9') {
      size_t lc = l.find(':'), rc = l.rfind(':');
      if (lc!=string::npos && lc==LabelLength() && lc!=rc)
	return l.substr(0, rc);
    }

    return ShowErrorS("DataBase::ParentObjectStringByPruning(", l,
		      ") failed");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::FillOrigins(const ground_truth *gt) {
    // currently works only for images...

    string hdr = "DataBase::FillOrigins() : ";

    bool prune = false;  // hard-coded...
  
    static vector<string> ext, suf;
    if (ext.empty()) {
      ext.push_back(".jpg"); ext.push_back(".jpeg"); ext.push_back(".gif");
      ext.push_back(".png"); ext.push_back(".tiff"); ext.push_back(".bmp");
      ext.push_back(".pbm"); ext.push_back(".pgm");  ext.push_back(".ppm");
      ext.push_back(".mov"); ext.push_back(".m2v");  ext.push_back(".mpg");
      ext.push_back(".avi"); 
      suf.push_back("");     suf.push_back(".gz");   suf.push_back(".bz2");
      size_t j = ext.size();
      for (size_t i=0; i<j; i++) {
	string tmp = ext[i];
	char *p = &tmp[0];
	use_facet<ctype<char> >(locale()).toupper(p, p+tmp.size());
	ext.push_back(tmp);
	// cout << ext[i] << " -> " << tmp << endl;
      }
    }

    bool ok = true;
    for (size_t i=0; ok && i<Size(); i++) {
      if (gt && (*gt)[i]!=1)
	continue;

      if (PicSOM::TargetTypeContains(ObjectsTargetType(i), target_file)) {
	const string label = Label(i);
	if (ReadOriginsInfo(i, prune, false).empty()) {
	  bool exact = false;
	  string obj = SolveObjectPath(label, "", "", true, &exact, true);
	  string file, tmp;
	  for (size_t s=0; s<suf.size() && file==""; s++)
	    for (size_t e=0; e<ext.size() && file==""; e++) {
	      string tmpf = obj+ext[e]+suf[s];
	      bool exists = FileExists(tmpf);
	      if (DebugOrigins())
		cout << "FileExists(" << tmpf << ") -> " << exists << endl;
	      if (exists)
		file = tmpf;
	    }

	  if (file=="")
	    return ShowError(hdr+"file for <"+label+"> not found");

	  WriteLog("Starting to originsinate <"+label+">");	  

	  string type = FileExtensionToMIMEtype(file), pagin, d;
	  if (type=="")
	    type = "unknown";
	  target_type tt = target_imagefile;
	  if (type.find("video")==0)
	    tt = target_videofile;

	  string urlin;
	  struct stat sbuf;

#ifdef HAVE_READLINK
	  if (stat(file.c_str(), &sbuf)==0 && sbuf.st_mode&S_IFLNK) {
	    char lbuf[10000];
	    int ll = readlink(file.c_str(), lbuf, sizeof lbuf);
	    if (ll>0) {
	      urlin = string(lbuf, ll);
	      // cout << "<<<" << urlin << ">>>" << endl;
	      size_t sl = urlin.rfind('/');
	      if (sl!=string::npos)
		urlin.erase(0, sl+1);
	    }
	  }
#endif // HAVE_READLINK

	  string len0str;
	  map<string,string> oi;
	  int nframes = 0;
	  float framerate = 0.0;
	  XmlDom xml;
	  ok = InsertOriginsInfo(i, false, file, file, type, urlin, pagin,
				 len0str, oi, d, tt, nframes, framerate,
				 xml);
	}
      }
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertObjects(list<upload_object_data>& objs,
			       const list<string>& extract,
			       const list<string>& detect,
			       const list<string>& segments,
			       bool do_match,
			       bool zipped, const vector<string>& odlist,
			       XmlDom& xml, const string& outsegin) {
    string msg = "DataBase::InsertObjects() : ";

    bool debug_l = debug_locks;
  
    if (debug_l)
      WriteLog("InsertObjects() entered");
    RwLockWrite("InsertObjects");
    if (debug_l)
      WriteLog("InsertObjects() passed lock");

    Tic("DataBase::InsertObjects");

    XmlDom fs = xml.FirstChild();
    XmlDom ol = fs.NodeName()=="objectlist" ? fs : xml.Element("objectlist");

    FindAllIndices(false);

    bool nodata = !do_match;
    bool dat = false;  // was: true
    bool nbr = false;
    bool ret = true;

    if (do_match) {
      for (size_t i=0; i<Nindices(); i++)
	if (IndexIs(i, "tssom"))
	  TsSomDeprecated(i).ReadFiles(false, nodata,
				       dat, true, true, false, false, nbr);
	else
	  cout << "DataBase::InsertObjects() skipping matching with index "
	       << MethodAndIndexName(i) << endl;;
    }
    
    FindAllIndices(false);  // looks really stupid but is necessary...

    int old_size = Size();

    for (auto i=objs.begin(); i!=objs.end(); i++) {
      if (i->fake)
	continue;
  
      if (i->data.empty() && i->text=="" && i->path=="") {
	ShowError("Skipping object with empty data");
	continue;
      }
  
      if (i->use == "segmentation") {
	if (!InsertSegmentationObject(*i))
	  continue;
      }
      else if (i->use == "regular") {
	Tic("InsertRegularObject");
	bool ins_ok = InsertRegularObject(*i, ol);
	Tac("InsertRegularObject");
	if (!ins_ok) {
	  ret = false;
	  continue;
	}

#ifdef PICSOM_USE_OD
	// this should be moved to the part that can be sent to threads/slaves
	if (odlist.size()) {
	  XmlDom oi = ol.LastElementChild();
	  XmlDom odl = oi.Element("odresultlist");
	  for (size_t j=0; j<odlist.size(); j++) {
	    XmlDom odr = odl.Element("odresult");
	    odr.Prop("name", odlist[j]);
	    string fname = i->path;
	    savetype savemode = NO_SAVING;
	    dbitem best;
	    OdProcessImage(fname, savemode, best);
	    if (best.nsurvivors >= 8) {
	      odr.Prop("boundingbox", best.invcorners);
	      odr.Prop("bestobject", best.objectname);
	      WriteLog("OD: Match found with "+best.objectname+
		       " with "+ToStr(best.nsurvivors)+"  keypoint pairs");
	    } else {
	      WriteLog("OD: No match found");
	    }
	  }
	}
#else
	if (odlist.size())
	  ShowError("odlist given but PICSOM_USE_OD not defined");
#endif // PICSOM_USE_OD
      }
      else if (i->use == "container-regular") {
	WriteLog("Preparing to insert objects from container");
	if (!InsertContainerObjectsAndFile(*i, ol))
	  ShowError(msg+"inserting container <"+i->path+"> failed");
      }
      else
	return ShowError(msg+"use=\""+i->use+"\" not understood"); 
    }

    // Now that all the pictures have been inserted
    // (including the segmentation images later on),
    // we can calculate features for the ones that
    // didn't have any problems

    // Maybe this could be placed in Connection::TryToRunXMLupload(),
    // since only an image's label and index are needed here

    bool can_unlock = !(do_match&&write_changed_divfiles);
    if (can_unlock) {
      RwUnlockWrite("InsertObjects");
      if (debug_l)
	WriteLog("InsertObjects() unlocked due to can_unlock");
    }

    Tac("DataBase::InsertObjects");

    return ret && InsertObjectsProcessing(objs, extract, detect, segments,
					  do_match, zipped,
					  odlist, xml, outsegin, old_size);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertObjectsProcessing(list<upload_object_data>& objs,
					 const list<string>& extract,
					 const list<string>& detect,
					 const list<string>& segments,
					 bool do_match, bool zipped,
					 const vector<string>& /*odlist*/,
					 XmlDom& /*xml*/,
					 const string& outsegin,
					 int old_size) {
    string msg = "DataBase::InsertObjectsProcessing() : ";

    bool skip_makesubobjectindex = true;  // obs! added 2013-08-16

    bool debug_l = debug_locks;

    Tic("DataBase::InsertObjectsProcessing");

    if (false && Picsom()->HasSlaves()) {
      cout << TimeStamp() << msg << "COULD USE SLAVES?" << endl;
      Picsom()->ShowSlaveInfoList();
    }

    if (segments.size() && CanDoSegmentation()) {
      Tic("_segments_");

      WriteLog("Doing segmentations for "+ToStr(objs.size())+
	       " inserted object"+(objs.size()!=1?"s":""));
      for (auto i=objs.begin(); i!=objs.end(); i++) {
	string id = i->url;
	if (i->indices.size()) {
	  id = Label(i->indices[0]);
	  WriteLog("Object #"+ToStr(i->indices[0])+" <"+id+"> has "+
		   ToStr(i->indices.size())+" part"+
		   (i->indices.size()!=1?"s":""));
	} else
	  WriteLog("Segments cannot be extracted for <"+id+">");

	vector<size_t> soidx = i->featext;
	
	for (auto sm=segments.begin(); sm!=segments.end(); sm++) {
	  DoOneSegmentationForAll(false, soidx, *sm);
	}
      }
      Tac("_segments_");
    }

    if (extract.size() && CanCalculateFeatures()) {
      Tic("_features_");

      WriteLog("Extracting features from "+ToStr(objs.size())+
	       " inserted object"+(objs.size()!=1?"s":""));
      for (auto i=objs.begin(); i!=objs.end(); i++) {
	string id = i->url;
	if (i->indices.size()) {
	  id = Label(i->indices[0]);
	  WriteLog("Object #"+ToStr(i->indices[0])+" <"+id+"> has "+
		   ToStr(i->indices.size())+" part"+
		   (i->indices.size()!=1?"s":"")+" of which "+
		   ToStr(i->featext.size())+" require feature extraction");
	} else
	  WriteLog("Features cannot be extracted for <"+id+">");

	for (vector<size_t>::iterator j=i->featext.begin();
	     j<i->featext.end(); j++) {
	  bool ok = true;
	  ok = ok && CalculateFeatures(*j, extract);
	  if (do_match)
	    ok = ok && LoadAndMatchFeatures(*j, extract, true, false);

	  // Problematic objects are removed from list
	  if (!ok) {
	    i->featext.erase(j, j+1); // this should be done to indices too!
	    j--;
	    continue;
	  }
	}
      }

      if (write_changed_divfiles) {
	bool use_cwd = false;
	ReWriteChangedDivisionFiles(use_cwd, zipped);
      }

      Tac("_features_");
    }

    // this is problematic if there should be a different .seg file name
    // for each object...?
    string outseg = outsegin;
    if (outseg=="" && objs.size() && objs.begin()->indices.size()) {
      size_t oidx = objs.begin()->indices[0];
      string p = SolveObjectPath(Label(oidx));
      //cout << "1 [[[[[" << p << "]]]]]" << endl;
      size_t sl = p.rfind('/');
      if (sl!=string::npos)
	p.insert(sl+1, "segments/vcd:");
      size_t d = p.rfind('.');
      if (d!=string::npos && d>sl)
	p.replace(d, p.size()-d, ".seg");
      //cout << "2 [[[[[" << p << "]]]]]" << endl;
      outseg = p;
    }

    Analysis ana(Picsom(), NULL, NULL, vector<string>());
    AddDescription *ad = NULL;
    segmentfile *image = NULL;
    bool do_ad = CanDoDetections() && detect.size() &&
      storedetections.find("seg")!=string::npos;
    string sname = "vcd", lname = "VisualConceptDetection";
    if (do_ad && !ana.OpenAddDescription(ad, image, outseg, sname, lname))
      return ShowError(msg+"OpenAddDescription() failed");
  
    // ad->addPendingAnnotation("resname", "frange", "123");

    if (detect.size()) {
      Tic("_detections_");
      list<string> feats;
      bool tolerate_missing = true, force = true;
      DoAllDetections(force, objs, detect, feats, tolerate_missing, ad);
      Tac("_detections_");
    }

    if (ad) {
      ana.CloseSegmentation(ad, image);

      if (SqlObjects()) {
	/*
	string sqlname = outseg;
	size_t p = sqlname.find("/segments/");
	bool ok = p!=string::npos;
	if (ok)
	  sqlname.erase(0, p+1);
	ok = SqlStoreFileFromFile(sqlname, outseg);
	if (!ok)
	  ShowError(msg+"SqlStoreFileFromFile() failed");
	*/
	size_t oidx = objs.begin()->indices[0];
	string segmname = outseg;
	size_t p = segmname.rfind('/');
	if (p!=string::npos)
	  segmname.erase(0, p+1);
	p = segmname.find(':');
	if (p!=string::npos)
	  segmname.erase(p);
	if (!SqlStoreSegmentationFile(oidx, segmname, outseg))
	  ShowError(msg+"SqlStoreSegmentationFile() failed");

	Unlink(outseg);
      }
    }

    bool can_unlock = !(do_match&&write_changed_divfiles);
    if (can_unlock) {
      RwLockWrite("InsertObjects");
      if (debug_l)
	WriteLog("InsertObjects() relocked due to can_unlock");
    }

    ostringstream msgstr;
    msgstr << "OriginalSize()=" << OriginalSize()
	   << " StartUpSize()=" << StartUpSize()
	   << " Size()="        << Size();

    WriteLog(msgstr);

    if (!skip_makesubobjectindex)
      MakeSubObjectIndex(old_size);

    Tac("DataBase::InsertObjectsProcessing");
    RwUnlockWrite("InsertObjectsProcessing");
    if (debug_l)
      WriteLog("InsertObjects() unlocked");
  
    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::InsertSegmentationObject(const upload_object_data& i) {
  const char *errhead = "DataBase::InsertSegmentationObject() : ";

  if (i.indices.size()==0)
    return ShowError(errhead, "Segmentation image without a preceding",
		     "regular image!");
  
  int index = i.indices.back();
  const string& label = Label(index);;

  string tempname = SolveObjectDirectory(label) + "picsomXXXXXX";
  char* tempn = new char[tempname.length()+1];
  strcpy(tempn, tempname.c_str());
  int fd = -1;
  FILE* tempf;
  
  if ((fd = mkstemp(tempn)) == -1 || (tempf = fdopen(fd, "w+")) == NULL) {
    if (fd != -1) {
      unlink(tempn);
      close(fd);
    }
    return ShowError(errhead, "Failed to open temporary file!");
  }
  
  size_t rr = fwrite(i.data.c_str(), 1, i.data.length(), tempf);
  fclose(tempf);
  close(fd);
  
  if (rr!=i.data.length())
    return ShowError(errhead, "Writing failed");

  // Now temporary file needs to be put through segmentation...
  
  //string label = ret.back().first;
  
  string out = SolveObjectPath(label, "segments", "ib+ps:"/*"%m%o:"*/);
  
  if (!Picsom()->MakeDirectory(out, true)){
    unlink(tempn);
    delete [] tempn;
    return ShowError("DataBase::InsertSegmentationObject() :MakeDirectory(",
		     out, ") failed");
  }
  
  string cmd = Picsom()->FindExecutable("segmentation", "segmentation")
    + " -o " + out + " ib + ps " + tempn;  

  // this might have worked when HAS_SEGMENTATION_INTERNAL was undef
  int r = Picsom()->ExecuteSystem(cmd, true, false, false);

  unlink(tempn);
  delete [] tempn;
  
  if (r)
    return ShowError(errhead, "Segmentation failed");

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertRegularObject(upload_object_data& info, XmlDom& ol) {
    string errhead = "DataBase::InsertRegularObject() : ";

    // obs! this hack should be made better...
    bool convert_gif2png = true;

    string tmp = info.text;
	
    for (unsigned int j=0; j < tmp.length(); ++j)
      if (tmp[j]<32 || ((unsigned)tmp[j]>126&&(unsigned)tmp[j]<160))
	tmp[j] = ' ';

    ostringstream msgstr;
    msgstr << "DataBase::InsertRegularObject() processing ["
	   << InsertModeName(insertmode) << "] object:"
	   << " path="   << info.path
	   << " type="   << info.ctype  << " len="      << info.data.length()
	   << " url="    << info.url    << " page="     << info.page
	   << " date="   << info.date   << " txt="      << tmp
	   << " use="    << info.use    << " comprext=" << info.comprext
	   << " dbname=" << info.dbname << " colors="   << info.colors
	   << " dimensions=" << info.dimensions
	   << " checksum=" << info.checksum
	   << " auxid="  << info.auxid << " forcedlabel=" << info.forcedlabel
	   << " fake=" << (info.fake?"yes":"no")
	   << " nofile=" << (info.nofile?"yes":"no")
	   << " keywords=";
    for (size_t j=0; j<info.keywords.size(); j++)
      msgstr << (j?",":"") << info.keywords[j];

    WriteLog(msgstr);
  
    bool is_video = info.ctype.find("video/")==0;
    bool insert_this  = !is_video ||  videoinsert.find("file")!=string::npos;
    bool extract_this = !is_video || videoextract.find("file")!=string::npos;

    // it might be difficult to implemement insert_this==false...
    if (!insert_this)
      return ShowError(errhead, "insert_this==false not implemented");

    string auxid;
    string label = info.forcedlabel;
    string oname = info.path!="" ? info.path : info.auxid;
    if (label=="")
      label = labelimport=="" ?
	NextFreeLabel() : ImportedLabel(oname, auxid);
    if (!label.length())
      return ShowError(errhead, "NextFreeLabel() or ImportedLabel() failed");
    if (label=="*skipped*")
      return true;

    if (labelimport!="" && info.auxid=="")
      info.auxid = auxid;

    if (info.data.length()==0 && info.text!="") {
      info.ctype = "text/plain";
      info.data  = info.text;
      if (info.data[info.data.size()-1]!='\n')
	info.data += "\n";
    }

    // mostly for OpenCV...
    if (convert_gif2png && info.ctype=="image/gif") {
      string png = info.path+".png";
      vector<string> cmd;
      cmd.push_back("convert");
      cmd.push_back(info.path);
      cmd.push_back(png);
      cmd.push_back("1>/dev/null");
      cmd.push_back("2>&1");
      if (!Picsom()->ExecuteSystem(cmd, true, true, true)) {
	info.path  = png;
	info.ctype ="image/png";
      }
    }

    target_type tt = MIMEtypeToTargetType(info.ctype);
    if (info.nofile)
      tt = PicSOM::TargetTypeFileMasked(tt);

    if (tt==target_error)
      return ShowError(errhead, "MIMEtypeToTargetType() failed");

    bool in_container = false;
    if (info.url.find(".zip[")!=string::npos ||
	info.url.find(".tar[")!=string::npos ||
	info.url.find(".tar.gz[")!=string::npos)
      in_container = true;
    
    if (tt&target_image && !in_container) {
      bool ok = true;
      try {
	imagefile file(info.path);
	ok = file.is_open();
      } catch (...) {
	ok = false;
      }
      if (!ok) {
	WriteLog("DataBase::InsertRegularObject(): ",info.path,
		 " not true image. Skipping...");
	return false;
      }
    }

    if (!in_container) {
      string name = InsertObjectFile(label, info.data, info.ctype,
				     info.comprext, info.path);
      info.dbname = name;
    } else {
      string name = info.url;
      size_t p = name.rfind('[');
      if (p==string::npos)
	return ShowError(errhead, "ERR-1");
      name.erase(0, p+1);
      p = name.rfind('.');
      if (p==string::npos)
	return ShowError(errhead, "ERR-2");
      name.erase(0, p);
      p = name.rfind(']');
      if (p==string::npos)
	return ShowError(errhead, "ERR-3");
      name.erase(p);
      info.dbname = label+name;
    }
    
    string truefile = SqlObjects() ? info.path : info.dbname;

    map<string,string> oi;
    if (info.colors+info.dimensions+info.checksum!="") {
      oi["colors"]     = info.colors    ==""?"-"  : info.colors;
      oi["dimensions"] = info.dimensions==""?"0x0": info.dimensions;
      oi["checksum"]   = info.checksum  ==""?"-"  : info.checksum;
    }
    oi["auxid"] = info.auxid;

    /*
      if (tt & (target_image|target_video)) { // only for images
      string col = "-", dim = "0x0", csm = "-";
      SolveMissingOriginsInfo(info.dbname, col, dim, csm);
      oi["colors"]     = col;
      oi["dimensions"] = dim;
      oi["checksum"]   = csm;
      }
    */

    int nframes = 0;
    float framerate = 0.0;

    // the following is bad fix. we shoud know the index when we are here.
    // info.indices is always(?) empty here
    size_t idx = info.indices.size() ? info.indices[0] : Size();

    bool insertobjectsrealinfo_was = insertobjectsrealinfo;
    if (tt==target_videofile &&
	(videoinsert.find("frames")!=string::npos ||
	 videoinsert.find("seconds")!=string::npos))
      insertobjectsrealinfo = true;

    XmlDom xml1;
    bool ok = !info.dbname.empty();
    ok = ok && InsertImageText(label, tmp);
    ok = ok && InsertOriginsInfo(idx, false, truefile, info.dbname,
				 info.ctype, info.url, info.page,
				 info.data, oi, info.date, tt,
				 nframes, framerate, xml1);
    insertobjectsrealinfo = insertobjectsrealinfo_was;

    if (tt & target_image && ThumbnailType()=="create")
      ok = ok && InsertImageThumbnails(info.dbname);

    // now this is the place where we know the index...
    int index = AddOneLabel(label, tt, false, false);
    if (index<0)
      return ShowError(errhead+"AddOneLabel("+label+") failed");
    if (index!=(int)idx)
      return ShowError(errhead+"AddOneLabel("+label+") returned "+ToStr(index)
		       +"!="+ToStr(idx));

    KeywordsToClasses(index, info);

    // do we need to extract parts from a collection (message)?
    if (tt & target_message) 
      if (!InsertMessageParts(info, info.dbname, label))
	return ShowError(errhead, "InsertMessageParts() failed");

    if (!ok)
      ShowError(errhead, "something minor? failed");
  
    info.indices.push_back(index);
    if (extract_this)
      info.featext.push_back(index);

    if (ol && insertobjectsrealinfo) {
      XmlDom dummy;
      bool oinfo = true;
      bool full = false;  // was true untl 2012-9-12
      AddToXMLobjectinfo(ol, Label(index), NULL,
			 false, true, oinfo, full, &dummy, "");
    }
  
    PossiblyDisplay(info, tt);
    
    XmlDom xml2;  // or ol?? no....
    bool upp = true; // update also parent....
    InsertVideoSubObjects(index, target_video, info, nframes, framerate,
			  upp, xml2);

#ifdef PICSOM_USE_CONTEXTSTATE
    ConditionallyAddToContext(info);
#endif // PICSOM_USE_CONTEXTSTATE

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertContainerObjectsAndFile(upload_object_data& info,
					       XmlDom& ol) {
    string msg = "DataBase::InsertContainerObjectsAndFile() <"+info.path+"> : ";

    WriteLog(msg+"starting");

    string f;
    if (!InsertContainerObjects(info, ol, f))
      return ShowError(msg+"InsertContainerObjects() failed");

    if (f=="")
      return InsertContainerFile(info.path, false);

    else
      return InsertContainerFile(f, true);
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertContainerFile(const string& f, bool created) {
    string msg = "DataBase::InsertContainerFile("+f+","+ToStr(created)+") :";

    WriteLog(msg+"starting");

    insertmode_t insertmode_was = insertmode;
    if (created)
      insertmode = insert_move;

    string r = InsertObjectData("*CONTAINER*", "", "", "", f);

    insertmode = insertmode_was;

    if (r=="")
      return ShowError(msg+"InsertObjectData() failed");
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertContainerObjects(upload_object_data& info, XmlDom& ol,
					string& tmpfile) {
    string msg = "DataBase::InsertContainerObjects() <"+info.path+"> : ";

    WriteLog(msg+"starting");

    if (info.ctype=="application/zip") {
      tmpfile = "";
#ifdef HAVE_ZIP_H
#if (LIBZIP_VERSION_MAJOR>0)
      zip_error_t err;
      zip_error_init(&err);
#endif // LIBZIP_VERSION_MAJOR>0
      int ierr = 0;
      int flags = 0;
#ifdef ZIP_RDONLY
      flags |= ZIP_RDONLY;
#endif // ZIP_RDONLY

      zip_t *x = zip_open(info.path.c_str(), flags, &ierr);
      if (!x) {
#if (LIBZIP_VERSION_MAJOR>0)
	zip_error_init_with_code(&err, ierr);
	const char *es = zip_error_strerror(&err);
#else
	char errtmp[1000];
	zip_error_to_str(errtmp, sizeof errtmp, ierr, errno);
	const char *es = errtmp;
#endif // LIBZIP_VERSION_MAJOR>0
	return ShowError(msg+"zip_open() failed: "+es);
      }
      // SqlBeginTransaction();
      for (size_t j=0;; j++) {
#ifdef ZIP_FL_ENC_GUESS
	zip_flags_t nflags = ZIP_FL_ENC_GUESS;
#else
	int nflags = 0;
#endif // ZIP_FL_ENC_GUESS
	const char *fn = zip_get_name(x, j, nflags);
	if (!fn)
	  break;
	WriteLog("** "+string(fn));

	string fnx = fn;
	size_t p = fnx.rfind('/');
	if (p!=string::npos)
	  fnx.erase(0, p+1);

	if (fnx=="") {
	  WriteLog("**** skipping...");
	  continue;
	}
	  
	string url = info.path;
	p = url.rfind('/');
	if (p!=string::npos)
	  url.erase(0, p+1);
	url += string("[")+fn+"]";

	upload_object_data i = info;
	i.ctype  = FileExtensionToMIMEtype(fnx);
	i.path   = "";
	i.url    = url;
	i.auxid  = fn;
	i.use    = "regular";
	i.nofile = true;
	bool insertobjectsrealinfo_was = insertobjectsrealinfo;
	insertobjectsrealinfo = false;
	bool ok = InsertRegularObject(i, ol);
	insertobjectsrealinfo = insertobjectsrealinfo_was;
	if (!ok)
	  ShowError(msg+"InsertRegularObject("+i.path+") failed");
      }
      // SqlEndTransaction();

      if (zip_close(x))
	ShowError("Failed to close ZIP file <"+info.path+">");
      
      return true;
#else
      return ShowError(msg+"libzip not available");
#endif // HAVE_ZIP_H
    }

    if (info.ctype=="application/x-tar") {
      string bname = info.path;
      size_t p = bname.rfind('/');
      if (p!=string::npos)
	bname.erase(0, p+1);
      p = bname.rfind('.');
      if (p!=string::npos && 
	  (bname.substr(p)==".gz" || bname.substr(p)==".GZ")) {
	bname.erase(p);
	p = bname.rfind('.');
      }
      if (p!=string::npos)
	bname.erase(p);
      tmpfile = TempDir("tar_to_zip")+"/"+bname+".zip";
      TarToZip(info.path, tmpfile);
      string fname = tmpfile;
      p = fname.rfind('/');
      if (p!=string::npos)
	fname.erase(0, p+1);
      upload_object_data i = info;
      // i.path = fname;
      i.path = tmpfile;
      i.ctype = "application/zip";
      string dummy;
      return InsertContainerObjects(i, ol, dummy);
    }
    
    return ShowError(msg+"MIME type <"+info.ctype+"> unknown");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ContainsImages(const string& f) {
    string msg = "DataBase::ContainsImages("+f+") : ";
    WriteLog(msg+"returns true");
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::TarToZip(const string& tar, const string& zip) {
    string msg = "DataBase::TarToZip("+tar+","+zip+") : ";

    bool debug = true;

    WriteLog(msg+"starting");
    string bname = tar;
    size_t p = bname.rfind('/');
    if (p!=string::npos)
      bname.erase(0, p+1);
    string dir = TempDir("tar_to_zip_tmp/"+bname);

    string tar_opts = debug ? "xvf" : "xf";
    vector<string> tar_cmd {
      "cd", dir, ";", "tar", tar_opts, Picsom()->Cwd()+"/"+tar
	};
    if (Picsom()->ExecuteSystem(tar_cmd, debug, debug, debug))
      return ShowError("tar extraction failed");
      
    string zip_opts = debug ? "-vry" : "-ry";
    vector<string> zip_cmd {
      "cd", dir, ";", "zip", zip_opts, zip, "."
	};
    if (Picsom()->ExecuteSystem(zip_cmd, debug, debug, debug))
      return ShowError("zip packing failed");

    vector<string> rm_cmd {
      "/bin/rm", "-rf", dir
	};
    if (Picsom()->ExecuteSystem(rm_cmd, debug, debug, debug))
      return ShowError("rm failed");
    
    WriteLog(msg+"ending");
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertVideoSubObjects(size_t indexin, target_type ott,
				       upload_object_data& infoin,
				       int nframes, float framerate,
				       bool also_parent,
				       XmlDom& xml) {
    string msg = "DataBase::InsertVideoSubObjects() : ";

    if (infoin.ctype.find("video/")!=0)
      return true;

    if (videoinsert.find("shots")!=string::npos ||
	videoextract.find("shots")!=string::npos)
      return ShowError("videoinsert/extract=shots not implemented yet");

    Tic("InsertVideoSubObjects");

    bool sizeunknown = nframes==-1;
    string nframesstr = sizeunknown ? "?" : ToStr(nframes);
    
    upload_object_data info = infoin;

    vector<size_t> subobjv;

    bool ok = true;

    bool insert_seconds  =  videoinsert.find("seconds")!=string::npos;
    bool extract_seconds = videoextract.find("seconds")!=string::npos;

    bool insert_minutes  =  videoinsert.find("minutes")!=string::npos;
    bool extract_minutes = videoextract.find("minutes")!=string::npos;

    bool insert_frames   =  videoinsert.find("frames")!=string::npos;
    bool extract_frames  = videoextract.find("frames")!=string::npos;

    if (insert_seconds || extract_seconds || insert_frames || extract_frames
	|| insert_minutes || extract_minutes) {
      string dir = info.dbname;
      if (dir=="")
	dir = info.url; // added 2014-01-21 when extract_minutes was implemented
      if (extract_seconds || extract_frames || extract_minutes) {
	size_t p = dir.rfind('.');
	if (p!=string::npos) {
	  dir.erase(p);
	  dir += ".d";
	  int mode = 02775;
	  if (!Picsom()->MkDirHier(dir, mode))
	    return ShowError(msg+"MkDirHier("+dir+") failed");
	}
      }
      
      if (framerate==0) {
	if (ott==target_video)
	  return ShowError(msg+"failing with framerate=0.0");
	else
	  framerate = 1;
      }
      
      size_t nsec = sizeunknown ? 0 : (size_t)ceil(nframes/framerate);
      size_t nmin = sizeunknown ? 0 : (size_t)ceil(nframes/(framerate*60));
      const string& label = Label(indexin);

      WriteLog(msg+label+" "+nframesstr+" @"+ToStr(framerate)
	       +" "+info.dimensions);

      size_t w = 0, h = 0;
      videofile vf;
      if (ott==target_video) {
	string tmpd = TempDir("videosubs", true);
	vf = videofile(info.url, false, tmpd);    // obs! does this work?
	float  a = vf.get_aspect();
	h = vf.get_height();
	w = vf.get_width();
	if (a)
	  w = (size_t)floor(h*a+0.5);
      } else {
	try {
	  imagefile ifile(info.url);
	  imagedata idata = ifile.frame(0);
	  w = idata.width();
	  h = idata.height();
	} catch (const string& e) {
	  return ShowError(msg+"image <"+info.url+"> could not be read");
	}
      }
      
      if (also_parent) {
	// InsertOriginsInfo();
      }

      imagefile f(info.url); // (info.url, info.ctype);
      f.frame_cacheing(false);

      size_t index = Size()-1; // was indexin

      if (extract_seconds || insert_seconds)
	for (size_t i=0; sizeunknown||i<nsec; i++) {
	  index++;

	  string flabel = label+":s"+ToStr(i);
	  string fname = dir+"/"+flabel+".jpeg";
	  size_t fno = size_t(floor(i*framerate+0.5));

	  if (fno>=(size_t)f.nframes())
	    break;

	  WriteLog("Starting to access frame #"+ToStr(fno)+"/"+
		   ToStr(f.nframes()));
	  imagedata d = f.frame(fno);
	  if (d.isempty()) {
	    WriteLog("Frame access failed #"+ToStr(fno)+"/"+
		     ToStr(f.nframes())+" in <"+info.url+">");
	    index--;
	    continue;
	    // obs! origins info for the parent is not changed to reflect
	    // the actual number of frames available
	  }

	  if (extract_seconds) {
	    WriteLog("Frame extracted");
	    picsom::scalinginfo sinfo(d.width(), d.height(), w, h);
	    if (d.width()!=w || d.height()!=h) {
	      int intrp = 1;
	      WriteLog("Starting to scale frame "+sinfo.info());
	      d.rescale(sinfo, intrp);
	      WriteLog("Frame scaled");
	    }
	    try {
	      if (!SqlObjects()) {
		WriteLog("Starting to write frame <"+ShortFileName(fname)+">");
		imagefile::write(d, fname, "image/jpeg");
		WriteLog("Frame written");
	      } else {
		WriteLog("Starting to stringify");
		info.data = imagefile::stringify(d, "image/jpeg");
		WriteLog("Frame stringified");
	      }
	    }
	    catch (const string& e) {
	      return ShowError(msg+"writing video frame to <"+fname
			       +"> failed: "+e);
	    }

	    infoin.featext.push_back(index);
	  }

	  if (insert_seconds) {
	    string ext = info.url;
	    size_t p = ext.rfind('.');
	    ext.erase(0, p==string::npos?ext.size():p);
	    string truefile = fname;
	    string fileandframe = label+ext+"["+ToStr(fno)+"]";
	    target_type tt = extract_seconds ? target_imagefile : target_image;
	    map<string,string> oi;
	    if (!extract_seconds) {
	      oi["colors"] = ">256";
	      oi["dimensions"] = ToStr(w)+"x"+ToStr(h);
	      oi["checksum"] = "0";
	    }
	    int nframesx = 0;
	    float framerate = 0.0;
	    ok = InsertOriginsInfo(index, false, truefile, fname,
				   "image/jpeg", fileandframe, info.page,
				   info.data, oi, info.date, tt, nframesx,
				   framerate, xml);
	    if (!ok) {
	      ShowError(msg+"InsertOriginsInfo() failed");
	      break;
	    }

	    int tindex = AddOneLabel(flabel, tt, false, false);
	    if (tindex!=(int)index) {
	      ok = ShowError(msg+"index="+ToStr(index)+" tindex="
			     +ToStr(tindex)+" mismatch");
	      break;
	    }

	    infoin.indices.push_back(index);
	    infoin.labels.push_back(flabel);
	    subobjv.push_back(index);
	  }
	}

      if (extract_frames || insert_frames)
	for (size_t i=0; i<(size_t)f.nframes(); i++) {
	  index++;

	  string flabel = label+":"+ToStr(i);
	  string fname = dir+"/"+flabel+".jpeg";

	  if (extract_frames) {
	    WriteLog("Starting to extract frame #"+ToStr(i)+"/"+
		     ToStr(f.nframes()));
	    imagedata d = f.frame(i);
	    WriteLog("Frame extracted");
	    picsom::scalinginfo sinfo(d.width(), d.height(), w, h);
	    int intrp = 1;
	    WriteLog("Starting to scale frame");
	    d.rescale(sinfo, intrp);
	    WriteLog("Frame scaled");
	    try {
	      if (!SqlObjects()) {
		WriteLog("Starting to write frame <"+ShortFileName(fname)+">");
		imagefile::write(d, fname, "image/jpeg");
		WriteLog("Frame written");
	      } else {
		WriteLog("Starting to stringify");
		info.data = imagefile::stringify(d, "image/jpeg");
		WriteLog("Frame stringified");
	      }
	    }
	    catch (const string& e) {
	      return ShowError(msg+"writing video frame to <"+fname
			       +"> failed: "+e);
	    }

	    infoin.featext.push_back(index);
	  }

	  if (insert_frames) {
	    string truefile = fname;
	    // string fileandframe = info.url+"["+ToStr(i)+"]";
	    string fileandframe = info.dbname;
	    if (fileandframe=="") // this was the case 2015-03-03
	      fileandframe = info.url; 
	    size_t p = fileandframe.rfind('/');
	    if (p!=string::npos)
	      fileandframe.erase(0, p+1);
	    fileandframe += "["+ToStr(i)+"]";
	    target_type tt = extract_frames ? target_imagefile : target_image;
	    map<string,string> oi;
	    if (!extract_frames) {
	      oi["colors"] = ">256";
	      oi["dimensions"] = ToStr(w)+"x"+ToStr(h);
	      oi["checksum"] = "0";
	    }
	    int nframesx;
	    float framerate;
	    ok = InsertOriginsInfo(index, false, truefile, fname,
				   "image/jpeg", fileandframe, info.page,
				   info.data, oi, info.date, tt, nframesx,
				   framerate, xml);
	    if (!ok) {
	      ShowError(msg+"InsertOriginsInfo() failed");
	      break;
	    }

	    int tindex = AddOneLabel(flabel, tt, false, false);
	    if (tindex!=(int)index) {
	      ok = ShowError(msg+"index mismatch");
	      break;
	    }

	    infoin.indices.push_back(index);
	    infoin.labels.push_back(flabel);
	    subobjv.push_back(index);
	  }
	}

      if (extract_minutes || insert_minutes)
	for (size_t i=0; sizeunknown||i<nmin; i++) {
	  index++;

	  string flabel = label+":m"+ToStr(i);
	  string fname = dir+"/"+flabel+".mp4"; // obs! hard-coded
	  size_t fno_a = size_t(floor(i*framerate*60+0.5));
	  size_t fno_b = size_t(floor((i+1)*framerate*60-0.5));

	  if (fno_b>=(size_t)f.nframes())
	    break;

	  WriteLog("Starting to access frames #"+ToStr(fno_a)+"..."
		   +ToStr(fno_b)+"/"+ToStr(f.nframes()));
	  imagedata d = f.frame(fno_b);
	  if (d.isempty()) {
	    WriteLog("Frame access failed #"+ToStr(fno_b)+"/"+
		     ToStr(f.nframes())+" in <"+info.url+">");
	    index--;
	    continue;
	    // obs! origins info for the parent is not changed to reflect
	    // the actual number of frames available
	  }

	  if (extract_minutes) {
	    try {
	      if (!SqlObjects()) {
	    	WriteLog("Starting to write frame <"+ShortFileName(fname)+">");
		double tp = i*60, dur = 60;
		vf.extract_video_segment(fname, tp, dur, vcodec_copy, acodec_copy, VF_FFMPEG);
	    	WriteLog("Video minute written");
	      } else {
		WarnOnce(msg+"not creting SQL object data from video minutes");
	      }
	    }
	    catch (const string& e) {
	      return ShowError(msg+"writing video minute to <"+fname
	    		       +"> failed: "+e);
	    }

	    infoin.featext.push_back(index);
	  }

	  if (insert_minutes) {
	    string ext = info.url;
	    size_t p = ext.rfind('.');
	    ext.erase(0, p==string::npos?ext.size():p);
	    string truefile = fname;
	    string fileandframe =
	      label+ext+"["+ToStr(fno_a)+"-"+ToStr(fno_b)+"]";
	    target_type tt = extract_minutes ? target_videofile : target_video;
	    map<string,string> oi;
	    if (!extract_minutes) {
	      oi["colors"] = ">256";
	      oi["dimensions"] = ToStr(w)+"x"+ToStr(h)+"x"+ToStr(fno_b-fno_a+1)
		+"@"+ToStr(framerate);;
	      oi["checksum"] = "0";
	    }
	    int nframesx = 0;
	    float framerate = 0.0;
	    ok = InsertOriginsInfo(index, false, truefile, fname,
				   "video/mp4", fileandframe, info.page,
				   info.data, oi, info.date, tt, nframesx,
				   framerate, xml);
	    if (!ok) {
	      ShowError(msg+"InsertOriginsInfo() failed");
	      break;
	    }

	    int tindex = AddOneLabel(flabel, tt, false, false);
	    if (tindex!=(int)index) {
	      ok = ShowError(msg+"index="+ToStr(index)+" tindex="
			     +ToStr(tindex)+" mismatch");
	      break;
	    }

	    infoin.indices.push_back(index);
	    infoin.labels.push_back(flabel);
	    subobjv.push_back(index);
	  }
	}
    }

    // This cannot be called if this is a slave.  In that case the information
    // to the slave is passed in AnalyseInsertSubObjects().
    if (SqlMode()==2)
      AppendSubobjectsFile(indexin, subobjv);
    
    Tac("InsertVideoSubObjects");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::KeywordsToClasses(int index, const upload_object_data& info) {
    for (size_t j=0; j<info.keywords.size(); j++) {
      string cname = info.keywords[j];

      // Support for "class=face:1272447204-170-0&face:Markus" type classes
      string etname = "";
      size_t et = cname.find('&');    
      if (et!=string::npos && cname.size()>et+1) {
	etname = cname.substr(et+1);
	cname = cname.substr(0,et);      
      }

      string fname = ExpandPath("classes", cname);
      ground_truth gtappend(Size());
      gtappend.Set(index, 1);
      WriteClassFile(fname, gtappend, info.kw_comment, true);
      ground_truth *gtall = ContentsFind(cname);
      if (gtall)
	(*gtall)[index] = 1;
    
      // Insert class to e.g. "face:"
      size_t c = cname.find(':');
      if (c!=string::npos && cname.size()>c+1) {
	string bname = cname.substr(0, c+1);
	AppendToMetaClassFile(bname, cname, true);
      }

      // Insert class to e.g. "face:Markus"
      if (etname != "")
	AppendToMetaClassFile(etname, cname, true);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::PossiblyDisplay(const upload_object_data& info,
				 target_type tt) {
    string errhead = "DataBase::PossiblyDisplay() : ";

    if (insert_mplayer && PicSOM::TargetTypeContains(tt, target_image)) {
      bool is_face = false;
      for (size_t j=0; j<info.keywords.size(); j++)
	if (info.keywords[j].find("face:")==0)
	  is_face = true;

      videofile& mplayer = is_face ?
	input_image_mplayer_face : input_image_mplayer_scene;

      string geom = is_face ? "-geometry 90%:60%" : "-geometry 90%:90%";

      try {
	if (mplayer.is_broken())
	  mplayer.close();
	if (!mplayer.is_open())
	  mplayer.open(geom, true);
      
	imagedata idata = imagefile(name).frame(0);
	mplayer.add_frame(idata);
      } catch (...) {
	ShowError(errhead+"mplayer operation failed");
      }
    }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::AppendToMetaClassFile(const string &metaclassfile, 
				     const string &outputline, bool reload) {  
  string errhead = "DataBase::AppendToMetaClassFile() : ";

  if (!IsMetaClassFile(metaclassfile)) {
    ShowError(errhead+metaclassfile+" not a METACLASSFILE");
    return false;
  }

  list<string> cl_list = SplitClassNames(metaclassfile);
  set<string>  cl_set(cl_list.begin(), cl_list.end());
  if (cl_set.find(outputline)==cl_set.end()) {
    string fullname = ExpandPath("classes", metaclassfile);
    ofstream os(fullname.c_str(), ios::app);
    os << "# Added " << Picsom()->OriginsDateStringNow() << endl 
       << outputline << endl;
    if (!os) {
      ShowError(errhead+"writing in <"+fullname+"> failed");
      return false;
    } else {
      WriteLog("Appended class \""+outputline+"\" in METACLASSFILE <"+
	       metaclassfile+">");
    }
  }

  if (reload) {
    ground_truth gt = ConditionallyReadClassFile(metaclassfile, true, true);
    WriteLog("DataBase::AppendToMetaClassFile(): Class " , metaclassfile , 
	     " now has ", ToStr(gt.NumberOfEqual(1)), " objects");
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_CONTEXTSTATE
bool DataBase::AddToContext(int idx) {
  map<string,string> oi = ReadOriginsInfo(idx, false, true);

  upload_object_data uid;
  
  uid.dbname = SolveObjectPath(Label(idx));
  uid.ctype  = oi["format"];
  uid.indices.push_back(idx);

  context.Add(uid);

  return true;
}
#endif // PICSOM_USE_CONTEXTSTATE

///////////////////////////////////////////////////////////////////////////////

// reads the given eml-file, extracts the attachments and adds the
// parts to db with labels like label:*
bool DataBase::InsertMessageParts(upload_object_data& info,
					  string &filename, string &label) {
  const char *errhead = "DataBase::InsertMessageParts() : ";

  srand( time(NULL) );
  string tempname = SolveObjectDirectory(label) + "/picsom_tmpdata_"
    + ToStr(rand()) + ".tmp";

  // Obs! Machine name hard-coded! We'll have to do this, since the
  // perl script won't work on all the machines
  string cmd = string("LD_PRELOAD=\"\" ssh itl-pc55 ")
    + Picsom()->FindExecutable("perl", "split_mail_to_tmp_files", "")
    + string(" -mailfile=") + filename 
    + string(" -baselabel=") + label 
    + string(" > ") + tempname;

  int r = Picsom()->ExecuteSystem(cmd, true, false, false);
  if (r)
    return ShowError(errhead, "Mail file splitting failed");

  // read the output of the system call (there is probably a better
  // way to read the standard output of the system call?)
  ifstream is(tempname.c_str());
  if (!is)
    return ShowError(errhead, "Temporary file couldn't be opened");

  // read all lines an add the temporary files generated by the mail splitter
  // to the database
  for (;;) {
    char line[1024]; // Obs! Hard-coded max line length (shouldn't
		     // cause any problems)
    is.getline(line, sizeof line);
    if(!is)
      break;

    if (!InsertMessagePart(info, label, line))
      return false;
  }  

  // remove the temporary file
  if (remove(tempname.c_str()))
    ShowError(errhead, "Failed to remove temporary file: ", tempname);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

/// Replaces the first occurance of space character with a '\0' in the
/// given string. Returns a pointer to the position after the replaced
/// character (NULL if error)
char *DataBase::GetNextSpaceSeparatedString(char *previous) {
  const char *errhead = "DataBase::GetNextSpaceSeparatedString() : ";

  char *next = strpbrk(previous," ");
  if(next == NULL) {
    ShowError(errhead, "Invalid character string format");
    return next;
  }

  *next = '\0';
  next++;
  return next;
}

///////////////////////////////////////////////////////////////////////////////

/// Inserts one message part. The label of the parent message and the
/// line containing the part label, file type, file name etc must be
/// given
bool DataBase::InsertMessagePart(upload_object_data& info, 
					 string &label, char* line) {

  const char *errhead = "DataBase::InsertMessagePart() : ";

  // solve target type string:
  char *ttype = GetNextSpaceSeparatedString(line);
  if (ttype == NULL) 
    return false;
  
  // solve mime type string:
  char *mimetype = GetNextSpaceSeparatedString(ttype);
  if (mimetype == NULL) 
    return false;
  
  // solve file extension string:
  char *fext = GetNextSpaceSeparatedString(mimetype);
  if (fext == NULL) 
    return false;
  
  // solve file length string:
  char *flength = GetNextSpaceSeparatedString(fext);
  if (flength == NULL) 
    return false;
  
  // solve temporary file name string:
  char *fname = GetNextSpaceSeparatedString(flength);
  if (fname == NULL) 
    return false;
    
  // extract info from the strings:
  target_type tt = SolveTargetTypes(ttype);
  int lenx = atoi(flength);
  string lenxstr((size_t)lenx, '\0');

  // Use InsertObjectData() in move mode to move the temporary files
  // to database:

  insertmode_t old_i = insertmode;
  insertmode = insert_move;
  string opath = InsertObjectData(string(line), string(label)+string(".d"), 
				  string(".")+string(fext), "", string(fname));
  insertmode = old_i;

  bool ok = true;
  map<string,string> oi;
  size_t idx = LabelIndex(label);
  int nframes = 0;
  float framerate = 0.0;
  XmlDom xml;
  ok = ok && InsertOriginsInfo(idx, false, opath, opath, string(mimetype), "",
			       "", lenxstr, oi, "", tt, nframes, framerate,
			       xml);
  if (tt & target_image && ThumbnailType()=="create")
    ok = ok && InsertImageThumbnails(opath, false);

  int index = AddOneLabel(line, tt, false, false);
  ok = ok && (index>=0);
  
  // If the object is a text object, call another perl script to
  // generate a utf8 compatible text of the message
  if (tt & target_text) {
    string utf8filename = SolveObjectDirectory(line)
      + "/" + label + ".d/message_utf8.txt";

    string cmd = string("LD_PRELOAD=\"\" ssh itl-pc55 ") 
      + Picsom()->FindExecutable("perl","utf8_escape", "")
      + " -textfile=" + opath + " > " + utf8filename;

    int r = Picsom()->ExecuteSystem(cmd, true, false, false);
    if (r)
      ok = ShowError(errhead, "message_utf8.txt creation failed.");
  }

  if (!ok)
    return false;

  info.indices.push_back(index);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::ReWriteChangedDivisionFiles(bool cwd, bool zipped) {
  Tic("ReWriteChangedDivisionFiles");

  bool ok = true;
  for (size_t i=0; i<Nindices(); i++)
    if (IndexIs(i, "tssom") &&
	TsSomDeprecated(i).DivisionChanged())
      ok &= TsSomDeprecated(i).ReWriteDivisionFile(cwd, zipped);

  Tac("ReWriteChangedDivisionFiles");

  return ok;
}

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::InsertObjectFile(const string& label,
				    const string& data,
				    const string& type,
				    const string& comprext,
				    const string& srcpath) {
    Tic("InsertObjectFile");

    string exts = MIMEtypeToFileExtension(type);
    if (exts=="") {
      exts = ".xxx";
      ShowError("DataBase::InsertObjectFile() file extension for [",
		type, "] not solved, using ", exts);
    }

    string ext = exts+comprext, ret = label+ext;

    if (!SqlObjects())
      ret = InsertObjectData(label, "", ext, data, srcpath);

    Tac("InsertObjectFile");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::WriteDataToFile(const string& file, 
				 const string& data) {
    if (data.empty())
      return false;

    // ofstream os(file.c_str());
    // os << data;
    // int count = os.tellp();
    // os.close();
    // if (!os)
    //   return ShowError("DataBase::WriteDataToFile() writing <", file,
    // 		       "> failed");
    if (!StringToFile(data, file))
      return ShowError("DataBase::WriteDataToFile() writing <", file,
		       "> failed");

    int count = FileSize(file);

    WriteLog("Saved file <", ShortFileName(file),"> ", ToStr(count)," bytes");
      
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::InsertObjectData(const string& label,
				    const string& sdir,
				    const string& ext, 
				    const string& indata,
				    const string& srcpath) {

    string msg = "DataBase::InsertObjectData() : ";

    bool ignore_symlink_errors = true;

    string opath;

    if (label!="*CONTAINER*") {
      opath = SolveObjectPath(label, sdir, "", true);
      if (opath.empty()) {
	stringstream err;
	err << "label=<"<<label << ">, sdir=<"<<sdir << ">, ext=<"<<ext << ">";
	ShowError(msg+"SolveObjectPath(", err.str(), ") failed");
	return "";
      }
      opath += ext;

    } else {
      string fname = srcpath;
      size_t p = fname.rfind('/');
      if (p!=string::npos)
	fname.erase(0, p+1);
      opath = ExpandPath("tars/"+fname);
      if (FileExists(opath)) {
	ShowError(msg+"file <"+opath+"> already exists");
	// we could store foo.zip as tars/foo-1.zip or tars/foo-2.zip
	// but that should have been known already in InsertContainerObjects()
	// so we have a potential problem here...
	return "";
      }
    }

    bool ok = Picsom()->MakeDirectory(opath, true);
    if (!ok) {
      ShowError(msg+"MakeDirectory(", opath,") failed");
      return "";
    }

    if (indata=="" && srcpath=="") {
      ShowError(msg+": both data and srcpath empty");
      return "";
    }

    if (indata!="" && srcpath!="") {
      ShowError(msg+": both data and srcpath nonempty");
      return "";
    }

    string tmpdata;
    if (insertmode==insert_copy && srcpath!="")
      tmpdata = FileToString(srcpath);

    const string& data = indata!="" ? indata : tmpdata;
    if (data!="") {
      if (WriteDataToFile(opath, data)) {
	WriteLog("Inserted file <", ShortFileName(opath), ">");
	return opath;
      }
      else {
	ShowError(msg+"writing <", opath, "> failed");
	return "";
      }
    }

    if (insertmode==insert_move && srcpath!="") {
      ok = Rename(srcpath, opath);
    
      // fall back to copying & deleting old file if renaming failed:
      if (!ok) {
	// ShowError(msg+": rename() failed. ", 
	//           "Copying and then unlinking the source file instead.");

	tmpdata = FileToString(srcpath);
	if (WriteDataToFile(opath,tmpdata)) {
	  ok = Unlink(srcpath); // should we care if this fails?
	  if (!ok)
	    ShowError(msg+": copying succeeded, but unlink() failed. "
		      "Not a grave error.");
	  ok = true; // not a grave error
	} 
	else {
	  ShowError(msg+": ", " both rename() and copying failed");
	  return "";
	}
      }
    }
    else if (insertmode==insert_softlink && srcpath!="") {
      ok = Symlink(FullPath(srcpath), opath);
      // obs! should check that existing and aimed links are identical
      if (!ok && ignore_symlink_errors)
	ok = true;

    } else if (insertmode==insert_relativelink && srcpath!="") {
      string fullpath = FullPath(srcpath);
      vector<string> fv = SplitInSomething("/", false, fullpath);
      vector<string> ov = SplitInSomething("/", false, opath);
      size_t cd = 0;
      while (cd<fv.size() && cd<ov.size() && fv[cd]==ov[cd])
	cd++;
      string relpath = "";
      for (size_t i=cd; i<ov.size()-1; i++)
	relpath += "../";
      for (size_t i=cd; i<fv.size(); i++)
	relpath += fv[i]+(i<fv.size()-1?"/":"");
      ok = Symlink(relpath, opath);
      // obs! should check that existing and aimed links are identical
      if (!ok && ignore_symlink_errors)
	ok = true;

    } else if (insertmode==insert_hardlink && srcpath!="") 
      ok = Link(FullPath(srcpath), opath);

    else {
      ostringstream os;
      os << "insertmode==[" << InsertModeName(insertmode) << "], srcpath=<"
	 << srcpath << ">, indata " << (indata.empty()?"empty":"nonempty")
	 << ", tmpdata " << (tmpdata.empty()?"empty":"nonempty");
      ShowError(msg+": cannot yet handle this case: ", os.str());
      return "";
    }

    if (!ok) {
      // char tmp[1024]; strerror_r(errno, tmp, sizeof tmp);
      char *tmp = strerror(errno);  //IRIX doesn't have strerror_r !!!
      ostringstream os;
      os << "[" << InsertModeName(insertmode) << "] with <"
	 << srcpath << "> -> <" << opath << ">";
      ShowError(msg+"failed : ", os.str(), " : ", tmp); 
      return "";
    }

    WriteLog(string(insertmode==insert_move?"Moved":
		    insertmode==insert_relativelink?"Relative-linked":
		    insertmode==insert_softlink?"Soft-linked":
		    insertmode==insert_hardlink?"Hard-linked":
		    "Inserted somehow")+
	     " file <", opath, ">");

    return opath;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ImportedLabel(const string& fname, string& auxid) {
    string hdr = "DataBase::ImportedLabel() : ";

    bool debug = true;
    
    vector<string> li = SplitInSomething("+", false, labelimport);
    string import;
    bool skip = false;
    for (auto i=li.begin(); i!=li.end(); i++)
      if (*i=="skip")
	skip = true;
      else
	import += *i;
    
    if (import!="simple") {
      ShowError(hdr+"rule for labelimport="+labelimport+" not known");
      return "";
    }
    
    string lab = fname;
    size_t s = lab.rfind('/');
    if (s!=string::npos)
      lab.erase(0, s+1);

    string fname2 = lab;

    auxid = lab;
    s = auxid.rfind('.');
    if (s!=string::npos)
      auxid.erase(s);

    for (size_t i=0; i<lab.size(); i++) {
      if (lab[i]>='0' && lab[i]<='9')
	continue;
      lab.erase(i, 1);
      i--;
    }

    size_t l = LabelLength();
    while (lab.size()<l)
      lab = "0"+lab;

    if (lab.size()!=l) {
      ShowError(hdr+"failed with length <"+fname2+"> => <"+lab+">");
      return "";
    }

    int idx = LabelIndexGentle(lab);
    if (idx>=0)  {
      if (skip)
	return "*skipped*";

      ShowError(hdr+"duplicate label would result <"+fname2+"> => <"+lab+">");
      return "";
    }

    if (debug)
      WriteLog(hdr+"["+fname+"] -> ["+lab+"]+["+auxid+"]");
    
    return lab;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::NextFreeLabel() {
    string hdr = "DataBase::NextFreeLabel() : ";

    RwLockWrite("NextFreeLabel");

    bool ok = true;
    string label;

    if (UseSql()) {
      if (next_free_label=="") {
	if (!Size())
	  NextFreeLabel(0);
	else {
	  long int max = 0;
	  for (size_t j=0; j<Size(); j++) {
	    // map<string,string> m = ReadOriginsInfo(j, false, false);
	    // long int i = atol(m["name"].c_str());
	    long int i = atol(Label(j).c_str());
	    if (i>max)
	      max = i;
	  }
	  NextFreeLabel(max+1);
	}
      }

      label = next_free_label;
      ok = label.size()==LabelLength();

      if (DebugSql())
	cout << hdr << "returning <" << label << ">" << endl;

    } else {
      string f = ExpandPath("next-free-label");
      // cout << hdr+"file is <" << f << ">" << endl;

      if (!Exists(f))
	ok = ShowError(hdr+"<", f, "> inexistent");

      if (ok && ::access(f.c_str(), W_OK))
	ok = ShowError(hdr+"<", f, "> is write protected");

      if (ok) {
	ifstream str(f.c_str());
	getline(str, label);
	if (!str || !IsLabel(label))
	  ok = ShowError(hdr+"reading <", f, "> failed -> [", label, "]");

	str.close();
	// cout << hdr+"read [" << label << "]" << endl;
      }
    }

    if (ok && !NextFreeLabel(atol(label.c_str())+1))
      ok = false;

    RwUnlockWrite("NextFreeLabel");

    return ok ? label : "";
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::NextFreeLabel(const string& lo) {
    string hdr = "DataBase::NextFreeLabel(const string&) : ";

    if (lo=="")
      return ShowError(hdr+"empty string");

    size_t ll = LabelLength();
    string l = lo;
    if (l.size()!=ll && l.find_first_not_of("0123456789")==string::npos)
      l = (string(ll, '0')+l).substr(l.size());

    if (!IsLabel(l))
      return ShowError(hdr+"<", l, "> is not a valid label");
  
    if (UseSql()) {
      next_free_label = l;
      if (DebugSql())
	cout << hdr << "setting <" << l << ">" << endl;

    } else {
      string f = ExpandPath("next-free-label");

      if (!Exists(f))
	return ShowError(hdr+"<", f, "> inexistent");

      if (::access(f.c_str(), W_OK))
	return ShowError(hdr+"<", f, "> is write protected");

      ofstream str(f.c_str());
      str << l << endl;
      if (!str)
	return ShowError(hdr+"writing <", f, "> failed");
    }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////
      
bool DataBase::FixUnknownOrigins(size_t i) {
  bool ok = true;
  map<string, string> oi = ReadOriginsInfo(i, true, true), oioi;
  string format = oi.count("format") ? oi["format"] : "";
  if (format == "image/unknown") {
    string lab = Label(i);
    string fname = SolveObjectPath((lab), LabelSubDir(lab), "", false, 
                                   NULL, true);
    if (fname.empty()) {
      // hack to take care of when files have been changed but origins
      // still has old .xxx extension
      fname = SolveObjectDirectory(lab)+"/"+lab+".jpg";
      if (!FileExists(fname))
        fname = SolveObjectDirectory(lab)+"/"+lab+".gif";
    }
    if (fname.empty()) 
      return false;
    string nformat = libmagic_mime_type(fname).first;
    if (!nformat.empty()) {
      size_t spacepos = nformat.find(' ');
      if (spacepos != string::npos) 
        nformat = nformat.substr(0,spacepos);
      string ext = MIMEtypeToFileExtension(nformat);
      size_t dotpos = fname.rfind('.');
      string nname = fname.substr(0,dotpos)+ext;
      if (fname.substr(dotpos+1) != ext)
        Rename(fname, nname);
      target_type tt = MIMEtypeToTargetType(nformat);
      int nframes = 0;
      float framerate = 0.0;
      XmlDom xml;
      string sizestr((size_t)atoi(oi["size"].c_str()), '\0');
      if (!InsertOriginsInfo(i, false, nname, nname, nformat, oi["url"], "", 
                             sizestr, oioi, oi["date"], tt,
			     nframes, framerate, xml))
        ok = false;
    }
  }
  return ok;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::UpdateOriginsInfo(size_t /*idx*/,
				   const string& label, const string& line) {
    string hdr = "DataBase::UpdateOriginsInfo(label=<"+label+">) : "; 
    WriteLog(hdr+"called");
    string dir = SolveObjectDirectory(label);

    if (dir.empty()) 
      return ShowError(hdr, "directory not solved");

    stringstream ofilestr;
    ofilestr << dir << "/" << origins;
    string ofile = ofilestr.str();

    if (!FileExists(ofile)) {
      ofstream os(ofile.c_str());
      os << line << endl;
    } else {
      if (!RenameToOld(ofile))
	return ShowError(hdr, "could not move away old origins file: ", ofile);

      string ifile = ofile+".old";
      ifstream is(ifile.c_str());
      ofstream os(ofile.c_str());

      bool inserted = false;  
      string tmp;
      while (getline(is, tmp)) { 
	if (tmp.substr(0,label.size()) == label) {
	  inserted = true;
	  tmp = line;
	}
	os << tmp << endl;
      }
      if (!inserted)
	os << line << endl;
    }

    SetFilePermissions(ofile);
  
    WriteLog("Updated line \""+line+"\" in <"+ShortFileName(ofile)+">");

    ClearOriginsFileLineCache();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SlaveSendOriginsInfo(const map<string,string>& m,
				      XmlDom& xml) {
    string hdr = "DataBase::SlaveSendOriginsInfo() : "; 

    XmlDom objinfo = xml.Element("objectinfohash");

    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first!="indexz")
	objinfo.Prop(i->first, i->second);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::UpdateOriginsInfoSql(size_t idx,
				      const map<string,string>& min,
				      bool update) {
    // string hdr = "DataBase::UpdateOriginsInfoSql() : "; 

    return update ? UpdateOriginsInfoSqlUpdate(idx, min)
      : UpdateOriginsInfoSqlAddNew(idx, min);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::UpdateOriginsInfoSqlUpdate(size_t idx,
					    const map<string,string>& min) {
    string hdr = "DataBase::UpdateOriginsInfoSqlUpdate() : "; 

    bool debug = false;

    if (SqlMode()!=2)
      return ShowError(hdr+"SQL opened read only");

    if (min.size()==0)
      return false;

    if (debug)
      cout << hdr << "#" << idx;

    string cmd = "UPDATE "+SqlTableName("objects", SqlStyle())+" SET";
    
    bool add_comma = false;
    for (auto i=min.begin(); i!=min.end(); i++) {
      if (add_comma)
	cmd += ",";

      string key = i->first, val = i->second;

      if (key=="type" && val.find_first_not_of("0123456789")!=string::npos)
	val = ToStr(SolveTargetTypes(val));

      if (key=="date") {
	struct timespec ts = Picsom()->InverseOriginsDateString(val);
	val = string((char*)&ts, sizeof(ts));
	key = "insdate";  // not "moddate"
	add_comma = false;
	continue; // obs!
      }

      if (val.size()>1 && val.substr(val.size()-2)=="<>") {
	string prefix = val.substr(0, val.size()-2);

	map<string,string> m = ReadOriginsInfoSql(idx);
	if (m.find(key)==m.end())
	  return ShowError(hdr+"<"+Label(idx)+"> does not have <"+key+">");
	  
	string valx = m[key], valz = valx, ext;
	size_t p = valx.rfind('.');
	if (p!=string::npos) {
	  ext = valx.substr(p);
	  valx.erase(p);
	}
	string label = Label(idx);
	if (false && prefix.size()+valx.size()!=label.size())
	  return ShowError(hdr+"lengths <"+prefix+">+<"+valx+">=<"+label+
			   "> do not match");
	if (false && label.substr(label.size()-valx.size())!=valx)
	  return ShowError(hdr+"<"+label+"> is not base for <"+valz+">");

	if (key=="url") {
	  val = prefix+valz;

	} else {
	  // key=="label" || key=="name"
	  string method, body, frame, segm;
	  if (!SplitLabel(label, method, body, frame, segm))
	    return ShowError(hdr+"SplitLabel("+Label(idx)+") failed");
	  string newbody = key=="label" ? prefix+body : body;
	  string newval = method!="" ? method+":" : "";
	  newval += newbody;
	  if (frame!="")
	    newval += ":"+frame;
	  if (segm!="")
	    newval += "_"+segm;
	  newval += ext;
	  val = newval;
	}
      }

      /// there are also other cases...
      if (key=="name")
	key = "file";
      if (key=="format")
	key = "mime";
      
      cmd += " "+key+"=\""+val+"\"";
      add_comma = true;

      if (debug)
	cout << " " << key << "=\"" << val << "\"";
    }
    cmd += " WHERE indexz="+ToStr(idx)+";";

    if (debug)
      cout << endl;

    if (DebugSql())
      cout << TimeStamp() << hdr << cmd << endl;

    if (!SqlExec(cmd, true)) {
      ShowError(hdr+"SqlExec() failed with <"+cmd+">");
      SqlClose();
      return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::UpdateOriginsInfoSqlAddNew(size_t idx,
					    const map<string,string>& min) {

    string hdr = "DataBase::UpdateOriginsInfoSqlAddNew() : "; 

    map<string,string> m = min;

    struct timespec now = TimeNow();
    struct timespec mod = Picsom()->InverseOriginsDateString(m["date"]);
    string nowblob = SqlTimeBlob(now), modblob = SqlTimeBlob(mod);
    string user = Picsom()->UserName();

    target_type tt = SolveTargetTypes(m["type"]);
    string datastr = SqlBlob(m["data"]);

    string urlesc = m["url"];
    for (;;) {
      size_t p = urlesc.find_first_of("`'\"");
      if (p==string::npos)
	break;
      urlesc.erase(p, 1);
    }

    stringstream ss;
    ss << "INSERT INTO " << SqlTableName("objects", SqlStyle())
       << " VALUES("
       << idx         	 << ",'" // possibly also m["indexz"]
       << m["label"]  	 << "',"
       << int(tt)   	 << ",'"
       << m["auxid"]   	 << "','"
       << m["file"]   	 << "','"
       << urlesc    	 << "','"
       << m["page"]   	 << "','"
       << m["mime"]   	 << "','"
       << m["colors"] 	 << "',"
       << m["width"]     << ","
       << m["height"]    << ","
       << m["frames"]    << ","
       << m["framerate"] << ","
       << m["bytes"]     << ",'"
       << m["md5"]       << "',"
       << modblob        << ","
       << nowblob        << ",'"
       << user           << "',"
       << datastr
       << ");";
    string cmd = ss.str();

    if (DebugSql()) {
      string cmdtmp = cmd;
      size_t p = cmdtmp.find("x'");
      if (p!=string::npos) {
	size_t q = cmdtmp.find_first_not_of("x'0123456789ABCDEF", p);
	if (q!=string::npos) {
	  cmdtmp.erase(p, q-p);
	  cmdtmp.insert(p, "<"+ToStr((q-p-3)/2)+" bytes of BLOB>");
	}
      }
      cout << TimeStamp() << hdr << cmdtmp << endl;
    }

    if (!SqlExec(cmd, true)) {
      ShowError("SqlExec() failed with <"+cmd+">");
      SqlClose();
      return false;
    }

    if (DebugSql())
      cout << TimeStamp() << hdr << "insertion successful" << endl;

    return true;

#if 0
    bool ok = true;

    if (false && !m["data"].empty()) {
      sqlite3_int64 rowid = sqlite3_last_insert_rowid(sql3);
      if (DebugSql())
	cout << TimeStamp() << hdr << "rowid is " << rowid << endl;

      sqlite3_blob *blob = NULL;
      int r = sqlite3_blob_open(sql3, NULL, "objects", "data",
				rowid, 1, &blob);
      if (r!=SQLITE_OK || !blob)
	return ShowError(hdr+"sqlite3_blob_open() "+SqliteErrMsg());

      int size = sqlite3_blob_bytes(blob);
      if (DebugSql())
	cout << TimeStamp() << hdr << "BLOB size is " << size << endl;

      if ((size_t)size!=m["data"].size())
	ok = ShowError(hdr+"BLOB and data sizes differ");
      else {
	r = sqlite3_blob_write(blob, m["data"].c_str(), size, 0);
	if (r!=SQLITE_OK)
	  ok = ShowError(hdr+"sqlite3_blob_write() "+SqliteErrMsg());
      }

      r = sqlite3_blob_close(blob);
      if (r!=SQLITE_OK)
	return ShowError(hdr+"sqlite3_blob_close() "+SqliteErrMsg());

      if (DebugSql())
	cout << TimeStamp() << hdr << "BLOB written and closed" << endl;
    }

    return ok;
#endif // HAVE_SQLITE3_H
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::InsertOriginsInfo(size_t idx, bool update,
				   const string& truefile,
				   const string& dbname,
				   const string& typin,
				   const string& urlin,
				   const string& pagin, 
				   const string& datain,
				   const map<string,string>& oiin,
				   const string& date,
				   target_type& tt,
				   int& nframes, float& framerate,
				   XmlDom& xml) {
    string err = "DataBase::InsertOriginsInfo(...) : ";

    // WriteLog(err+"called");

    Tic("InsertOriginsInfo");

    string data  = datain, typ = typin;
    string label = LabelFromFileName(dbname);
    if (!IsLabel(label))
      return ShowError(err+" filename <"+dbname+"> could not be parsed to"
		       " proper label");

    string name(dbname);
    size_t slash = name.find_last_of('/');
    if (slash!=string::npos)
      name.erase(0, slash+1);

    nframes = 0;
    framerate = 0.0;

    string col = "-", dim = "0x0", csm = "-", aux;
    map<string,string> oi = oiin;
    if (oi["colors"]!="")     col = oi["colors"];
    if (oi["dimensions"]!="") dim = oi["dimensions"];
    if (oi["checksum"]!="")   csm = oi["checksum"];
    if (oi["auxid"]!="")      aux = oi["auxid"];

    bool angry = !update;
    if ((tt & (target_image|target_imageset|target_video)) &&
	(col=="-" || dim=="0x0" || csm=="-")) {
      SolveMissingOriginsInfo(truefile, tt, col, dim, csm,
			      nframes, framerate, typ, angry);
      if (typ.find("image/")!=0 && typ.find("video/")!=0)
	tt = target_no_target;
    }

    size_t len = data.size();
    if (!len) {
      struct stat st;
      if (!stat(truefile.c_str(), &st))
	len = st.st_size;
      if (len && SqlObjects())
	data = FileToString(truefile);
    }

    if (tt==target_imagefile) { // multiframe TIFFs or GIFs !!!
      size_t x = dim.find('x');
      if (x!=string::npos && dim.find('x', x+1)!=string::npos)
	tt = target_imagesetfile;
    }

    Tac("InsertOriginsInfo");  // a bit premature...

    int width = 0, height = 0, frames = 1;
    sscanf(dim.c_str(), "%dx%dx%d@%f", &width, &height, &frames, &framerate);
    map<string,string> m;
    m["indexz"]    = ToStr(idx);
    m["label"]     = label;
    m["type"]      = TargetTypeString(tt);
    m["file"]      = name;
    m["url"]       = urlin;
    m["page"]      = pagin;
    m["mime"]      = typ;
    m["colors"]    = col;
    m["width"]     = ToStr(width);
    m["height"]    = ToStr(height);
    m["frames"]    = ToStr(nframes); // used to be frames
    m["framerate"] = ToStr(framerate);
    m["bytes"]     = ToStr(len);
    m["auxid"]     = aux;
    m["md5"]       = csm;
    m["date"]      = date==""?"-":date;
    m["data"]      = data;

    bool do_pipe = Picsom()->IsSlave() &&
      Picsom()->SlavePipe().find("objectinfo")!=string::npos;

    if (do_pipe)
      return SlaveSendOriginsInfo(m, xml);
    
    if (UseSql())
      return UpdateOriginsInfoSql(idx, m, update);

    origins_entry_cache_map[idx] = m;
    
    string url = urlin, pag = pagin;
    EscapeWhiteSpace(url);
    EscapeWhiteSpace(pag);

    stringstream ss; 
    ss << name << "\t"
       << (url!=""?url:"-") << "\t"
       << (pag!=""?pag:"-") << "\t"
       << (typ!=""?typ:"-") << "\t"
       << col << "\t"
       << dim << "\t"
       << len << "\t"
       << csm << "\t"
       << (date!=""?date:"-");
 
    string line = ss.str();

    return UpdateOriginsInfo(idx, label, line);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SolveMissingOriginsInfo(const string& fname,
					 target_type tt,
					 string& col,
					 string& dim, string& chk,
					 int& nfr, float& frr, string& mime,
					 bool angry) {
    string msg = "SolveMissingOriginsInfo("+ShortFileName(fname)+") : ";
    // WriteLog(msg+"called");

    bool calculate_checksum = true;

    Tic("SolveMissingOriginsInfo");

    nfr = 0;
    frr = 0.0;

    bool ok = true;

    if (tt&target_image || tt&target_imageset) {
      if (insertobjectsrealinfo) {
	try {
	  const size_t climit = 256;

	  imagefile file(fname);
	  mime = file.format();
	  ok = file.nframes()>0;
	
	  if (ok) {
	    imagedata img = file.frame(0);
	  
	    stringstream tmp1;
	    tmp1 << img.width() << "x" << img.height();
	    if (file.nframes()>1)
	      tmp1 << "x" << file.nframes() << "@" << file.video_fps();
	    dim = tmp1.str();
	    nfr = file.nframes();
	    frr = file.video_fps();
	  
	    bool over = false;
	    set<vector<float> > sv;
	    for (size_t x=0; !over && x<img.width(); x++)
	      for (size_t y=0; !over && y<img.height(); y++) {
		sv.insert(img.get_float(x, y));
		if (sv.size()>climit)
		  over = true;
	      }
	  
	    stringstream tmp2;
	    if (over)
	      tmp2 << '>' << climit;
	    else
	      tmp2 << sv.size();
	    col = tmp2.str();
	  }
	}
	catch (const string& err) {
	  if (angry)
	    ok = ShowError(msg+"failed: "+err);
	  else
	    WriteLog(msg+err);
	}
	catch (const std::exception& err) {
	  ok = ShowError(msg+"failed <"+err.what()+">");
	}
	catch (...) {
	  ok = ShowError(msg+"failed");
	}
      } else {
	col = ">256";
	dim = "0x0";  // obs! was "123x456"
      }

    } else if (tt&target_video) {
      col = ">256";
      if (insertobjectsrealinfo) {
	videofile fv(fname);
	int h = fv.get_height(), w = fv.get_width();
	float a = fv.get_aspect();
	if (a)
	  w = (int)floor(fv.get_height()*a+0.5);
	
	frr = fv.get_frame_rate();
	nfr = fv.get_num_frames();
	
	if (DebugOrigins())
	  cout << "videofile::get_num_frames() returns " << nfr
	       << " and videofile::get_frame_rate() returns " << frr
	       << endl;

	if (!nfr && fv.get_demuxer()=="mpegts")
	  nfr = -1;      
	
	dim = ToStr(w)+"x"+ToStr(h);
	if (nfr!=-1)
	  dim += "x"+ToStr(nfr);
	dim += "@"+ToStr(frr);

      } else {
	dim = "0x0x0@0";
	nfr = 0;
	frr = 0;
      }
    }

    if (calculate_checksum && insertobjectsrealinfo) {
      chk.clear();
#ifdef HAVE_GCRYPT_H
      string data = FileToString(fname);
      NGram::NGramData::shadigest_vect_t hash
	= NGram::NGramData::hashsum(data, GCRY_MD_MD5);
      for (size_t i=0; i<hash.size(); i++) {
	unsigned char c = hash[i];
	for (size_t j=0; j<2; j++) {
	  unsigned char d = j==0 ? c/16 : c%16;
	  d += d>9 ? '@'-9+32 : '0';
	  chk += ToStr(d);	  
	}
      }
#endif // HAVE_GCRYPT_H
    }

    Tac("SolveMissingOriginsInfo");

    // WriteLog(msg+"with "+(insertobjectsrealinfo?"real":"fake")+
    //	     " data, returning ok="+ToStr(ok));

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> DataBase::ThumbnailPath(size_t idx, bool create) {
    string hdr = "DataBase::ThumbnailPath("+ToStr(idx)+") : ";

    string opath = SolveObjectPath(Label(idx));

    // this is _not_ useful if idx is a videosegment...
    // string opath = ObjectPathEvenExtract(idx);

    vector<string> tnd = FindThumbnailSizesList();
    for (auto i=tnd.begin(); i!=tnd.end(); i++)
      *i = "tn-"+*i+"/";
    if (tnd.empty())
      tnd.push_back("vc-");

    string zpath = opath;
    size_t zp = zpath.rfind('/');
    if (zp!=string::npos && zp>=2 && zpath.substr(zp-2, 2)==".d") {
      size_t q = zpath.rfind('/', zp-1);
      if (q!=string::npos) {
	zpath.erase(q, zp-q);
	zp = q;
      }
    }

    string dest;
    string otyp = thumbnailmime!="" ? thumbnailmime : "image/unknown";

    for (size_t i=0; i<2; i++)
      for (auto j=tnd.begin(); j!=tnd.end(); j++) {
	string path = zpath;
	size_t p = zp;
	path.insert(p+1, *j);

	if (i==0) {
	  p = path.rfind('/');
	  path.erase(p+1);
	  path += Label(idx);
	}

	p = path.rfind('.');
	if (p!=string::npos)
	  path.erase(p);
	
	if (thumbnailmime=="image/jpeg")
	  path += ".jpg";
	else if (thumbnailmime=="image/png")
	  path += ".png";
	
	if (dest=="")
	  dest = path;

	if (FileExists(path)) {
	  if (debug_tn)
	    cout << hdr << "returns <" << ShortFileName(path) << "> and <"
		 << otyp << ">" << endl;
	  
	  return make_pair(path, otyp);
	}
      }
    
    if (!create || dest=="")
      return make_pair("", "");

    string tdir = TempDir("");
    if (dest.find(tdir)==0) {
      dest.replace(0, tdir.size(), Path());
    }

    return make_pair(dest, otyp);
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> DataBase::VirtualThumbnailPath(int idx) {
    string hdr = "DataBase::VirtualThumbnailPath("+ToStr(idx)+") : ";

    for (auto tn=virtualthumbnailsize.begin();
	 tn!=virtualthumbnailsize.end(); tn++) {
      ground_truth gt = GroundTruthExpression(tn->first);
      if (gt[idx]==1) {
	if (tn->second=="lfw") {
	  imagedata fimg = ExtractLfwFace(idx); 
	  string ffname = TempDir("tn-lfw")+"/"+Label(idx)+".png";
	  try {
	    imagefile::write(fimg, ffname);
	  } catch (const string& e) {
	    ShowError(hdr+"imagefile::write("+ffname+") failed: "+e);
	    break;
	  }
	  
	  return make_pair(ffname, "image/png");
	}
	else
	  ShowError(hdr+"could not process class=<"+tn->first+"> rule=<"+
		    tn->second+">");

	break;
      }
    }
    
    return make_pair("", "");;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::InsertImageThumbnails(const string& fname, bool to_root) {
  Tic("InsertImageThumbnails");

  if (debug_tn)
    cout << "Now in DataBase::InsertImageThumbnails(" << fname << ","
	 << to_root << ")" << endl;

  bool ok = true;
  string tns = FindThumbnailSizes() + ',';
  while (tns.size()>1 && ok) {
    size_t comma = tns.find(',');
    stringstream ss;
    ss << tns.substr(0, comma);
    tns.erase(0, comma+1);
    int w = 0, h = 0;
    char x;
    ss >> w >> x >> h;
    if (x!='x') {
      ok = ShowError("DataBase::InsertImageThumbnails() no x");
      break;
    }
    if (w==0 || h==0) {
      ok = false;
      break;
    }

    ok = InsertImageThumbnail(fname, w, h, to_root);
  }

  Tac("InsertImageThumbnails");

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::InsertImageThumbnail(const string& fname,
				    int w, int h, bool /*to_root*/) {
  imagedata imgdata;
  try {
    imagefile imgfile(fname);
    imagedata piece = imgfile.frame(0);

    float orat = float(piece.width())/piece.height();
    float trat = float(w)/h;

    int wr = w, hr = h;
    if (orat<trat)
      wr = int(w*orat/trat);
    else
      hr = int(h*trat/orat);

    picsom::scalinginfo sinfo(piece.width(), piece.height(), wr, hr);
    int intrp = 1;
    piece.rescale(sinfo, intrp);

    imgdata = imagedata(w, h, piece.count(), piece.type());
    imgdata.copyAsSubimage(piece, (w-wr)/2, (h-hr)/2);
  }
  catch (const string& e) {
    return ShowError("DataBase::InsertImageThumbnail() : ", e);
  }

  stringstream tndir;
  tndir << "tn-" << w << "x" << h;

  const char *ext = ".gif";
  string label = LabelFromFileName(fname);
  string opath = SolveObjectPath(label, tndir.str(), "", true);
  if (opath.empty())
    return ShowError("DataBase::InsertImageThumbnail() : ",
		     "SolveObjectPath() failed (label="+label
		     +", tndir="+tndir.str()+ ", ext="+ext+")");

  opath += ext;

  if (!Picsom()->MakeDirectory(opath, true))
    return ShowError("DataBase::InsertImageThumbnail() : ",
		     "MakeDirectory(", opath, ") failed");

  imagefile::write(imgdata, opath, "image/gif");

  WriteLog("Wrote thumbnail image <", ShortFileName(opath), ">");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::InsertImageText(const string& label,
                                       const string& txt) {
  if (txt.empty())
    return true;

  string copy = txt;
  if (copy[copy.length()-1]!='\n')
    copy += '\n';

  bool ok = InsertObjectData(label, "texts", ".txt", copy, "")!="";

  return ok;
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CalculateFeatures(int idx, const list<string>& feats) {
    bool debug = debug_features;

    string msg = "DataBase::CalculateFeatures("+ToStr(idx)+" <"+Label(idx)
      +">,"+ToStr(feats.size())+ " feats) : ";

    size_t nf = 0;

    if (debug) {
      cout << TimeStamp() << msg << "starting with";
      if (feats.size())
	for (list<string>::const_iterator i=feats.begin(); i!=feats.end(); i++)
	  cout << " " << *i;
      else
	cout << " EMPTY";
      cout << endl;
    }

    Tic("CalculateFeatures");
    FindAllIndices();

    bool ok = true;
    set<string> segments;

    bool extract_all = feats.size()==1 && *feats.begin()=="*";
    set<string> featset;
    if (!extract_all)
      featset.insert(feats.begin(), feats.end());
    set<string> featsetchk = featset;

    target_type ott = ObjectsTargetType(idx);

    if (debug)
      cout << TimeStamp() << msg << "extract_all="+ToStr(extract_all)+
	" featset.size()="+ToStr(featset.size())+
	" target_type=" << TargetTypeString(ott) << endl;

    for (size_t i=0; ok && i<Nindices(); i++) {
      target_type itt = GetIndex(i)->FeatureTarget();
      if (itt==target_no_target) {
	itt = target_no_target;
      }

      bool comptt = GetIndex(i)->CompatibleTarget(ott);
      bool found  = featset.find(GetIndex(i)->Name())!=featset.end();
      bool useit  = extract_all || found;

      if (found && comptt && useit)
	featsetchk.erase(GetIndex(i)->Name());

      if (debug)
	cout << TimeStamp() << msg << GetIndex(i)->Name()
	     << " methodname=" <<  GetIndex(i)->MethodName()
	     << " target_type=" <<  TargetTypeString(itt)
	     << " compatible=" << comptt
	     << " found=" << found << " use=" << useit
	     << (found?" <<====!!":"")
	     << endl;

      if (comptt && useit) {
	Index *ip = GetIndex(i);
	if (ip->IsDummy())
	  ip = FindIndex(GetIndex(i)->Name(), "", true, false);
	XmlDom xml;
	bool ok1 = ip->CalculateFeatures(idx, segments, xml);
	if (!ok1)
	  ok = false;
	nf += ok1;
      }
    }

    if (featsetchk.size() && debug) {
      cout << TimeStamp() << msg
	   << "following specified features were not extracted:";
      for (set<string>::const_iterator i=featsetchk.begin();
	   i!=featsetchk.end(); i++)
	cout << " " << *i;
      cout << endl;
    }

    Tac("CalculateFeatures");

    if (debug)
      cout << TimeStamp() << msg
	   << "ending with "+ToStr(nf)+" features extracted" << endl;

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool
  DataBase::CalculateConceptFeatures(const Index *ip, 
				     const vector<size_t>& idxs,
				     const vector<string>& files,
				     const list<incore_feature_t>& incore,
				     feature_result *result) {
    string msg = "DataBase::CalculateConceptFeatures("+ip->Name()+") : ";
    cout << msg << "idxs =";
    for (auto i : idxs)
      cout << (i!=idxs.front()?",":"") << " " << i;
    cout << endl;
    cout << msg << "files =";
    for (auto i : files)
      cout << (i!=files.front()?",":"") << " " << i;
    cout << endl;
    cout << msg << "incore =";
    for (auto i : incore)
      cout << (i!=incore.front()?",":"") << " " << i.first.first;
    cout << endl;

    Index *fdescr = FindIndex(ip->Name());
    if (!fdescr)
      return ShowError(msg+"feature not known");

    string detname = fdescr->GetProperty("detections");

    list<string> feats { fdescr->GetProperty("features") };
    if (feats.empty() || feats.front()=="")
      return ShowError(msg+"<feature features=\"...\"> not specified");

    auto ddescr = described_detections.find(detname);
    if (ddescr==described_detections.end())
      return ShowError(msg+"detections <"+detname+"> not known");
    map<string,string> dmap(ddescr->second.begin(), ddescr->second.end());

    auto descr = *ddescr;
    descr.second.push_back({"incore", "true"});

    list<string> clslist = SplitClassNames(dmap["class"]);
    for (auto c : clslist) {
      string msgc = msg+"class=<"+c+"> ";
      bool force = false, tolerate_missing = false;
      string instance, augm;
      XmlDom xml;
      PicSOM::detection_stat_t dstat;
      if (!DoOneDetectionForAll(force, idxs, descr, c, instance, feats, 
				augm, tolerate_missing, NULL, xml, dstat))
	return ShowError(msgc+"DoOneDetectionForAll() failed");
    }

    for (auto i : idxs) {
      vector<float> featvec;
      for (auto c : clslist) {
	string det = svms.begin()->second->Identifier(dmap["svm_library"], 
						      feats.front(), 
						      dmap["svm_kernel_type"],
						      dmap["extra"], c, "");
	bool angry = false, dummy = true;
	map<string,vector<float> > dets
	  = RetrieveDetectionData(i, det, angry, dummy);
	if (dets.size()!=1)
	  return ShowError(msg+"RetrieveDetectionData() #"+ToStr(i)+
			   " <"+det+"> failed");
	featvec.push_back(dets.begin()->second[0]);
      }
      result->data.push_back(make_pair(featvec, Label(i)));
    }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool DataBase::LoadAndMatchFeatures(int idx, const list<string>& feats,
                                    bool may_add, bool take_all) {
  RwLockWrite("LoadAndMatchFeatures");
  Tic("LoadAndMatchFeatures");

  bool dontstop = true;

  bool match_all = feats.size()==1 && *feats.begin()=="*";
  set<string> featset;
  if (!match_all)
    featset.insert(feats.begin(), feats.end());

  bool ok = true;
  for (size_t i=0; (ok || dontstop) && i<Nindices(); i++) {
    /*
    cout << "i=" << i << " " << IndexMethodName(i)
         << " " << IndexName(i) << " index=["
         << GetIndex(i)->FeatureTargetString() << "] object=["
         << TargetTypeString(ObjectsTargetType(idx)) << "] "
         << TsSomDeprecated(i).CompatibleTarget(ObjectsTargetType(idx))
         << " " << (featset.find(GetIndex(i)->Name())!=featset.end())
         << endl;
    */
    if (IndexIs(i, "tssom") && TsSomDeprecated(i).Nlevels() &&
	TsSomDeprecated(i).CompatibleTarget(ObjectsTargetType(idx)) &&
        (match_all || featset.find(GetIndex(i)->Name())!=featset.end()))
      if (!TsSomDeprecated(i).LoadAndMatchFeatures(idx, may_add, take_all))
        ok = false;
  }

  Tac("LoadAndMatchFeatures");
  RwUnlockWrite("LoadAndMatchFeatures");
  
  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool DataBase::LoadAndMatchFeatures(const string& cls) {
  bool strict = false;

  list<string> feat;
  for (size_t i=0; i<Nindices(); i++) {
    feat.push_back(IndexName(i));
    if (IndexIs(i, "tssom"))
      TsSomDeprecated(i).ReadFiles(false, false, false, true, true,
                                   false, false, false);
  }

  bool ok = true;
  ground_truth gt = GroundTruthExpression(cls);
  for (size_t i=0; ok && i<gt.size(); i++)
    if (gt[i]==1)
      ok = LoadAndMatchFeatures(i, feat, false, false) || !strict;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void DataBase::InitializeDefaultAspects() {
  // default default aspects, that should be overriden if the aspects are 
  // really used

  // just something for coordinate aspect testing:
  /*
  aspects a;
  a.AspectRelevance("C", 0.0, aspect_data::aspect_coordinates);
  a.AspectRelevance("B", 0.0, aspect_data::aspect_default);
  DefaultAspects(target_video, a);
  */

  defaultaspects.clear();
  defaultaspects_cache.clear();

  // any_target matches with every object..
  aspects b;
  b.AspectRelevance("", 0.0);
  DefaultAspects(target_any_target, b);
}

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::RemoveCreated(const string& ext) {
    bool really_remove = true;
    bool ok = true;

    set<string> exts;
    if (ext!="") {
      vector<string> extv = SplitInCommas(ext);
      exts.insert(extv.begin(), extv.end());
    }

    set<string> removed;
  
    for (auto it=created_files.begin(); it!=created_files.end();) {
      const string& fn = *it;
      if (removed.find(fn)!=removed.end()) {
	it++;
	continue;
      }

      if (exts.size()) {
	string fnext = fn;
	size_t p = fnext.rfind(".");
	if (p!=string::npos)
	  fnext.erase(0, p);
	if (exts.find(fnext)==exts.end()) {
	  it++;
	  continue;
	}
      }

      WriteLog("DataBase::RemoveCreated() : Removing <"+ShortFileName(fn)+">");

      if (really_remove)
	ok = Unlink(fn);

      removed.insert(fn);

      it = created_files.erase(it);
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  imagedata DataBase::ImageData(size_t idx, bool angry, string *fmt) {
    string msg = "DataBase::ImageData("+ToStr(idx)+") : ";
    imagedata img;

    const string& lab = Label(idx);

    if (SqlObjects()) {
      string data = SqlObjectData(idx);
      string tmpf = TempFile("sqltmpzz/"+lab);
      StringToFile(data, tmpf);
      img = imagefile(tmpf).frame(0);
      Unlink(tmpf);
      return img;
    }

    string path = isdummydb ? Picsom()->RootDir("icons")+"/picsom.jpg" :
      ObjectPathEvenExtract(idx);

    if (path=="") {
      ShowError(msg+"path of <"+lab+"> not solved");
      return img;
    }

    try {
      imagefile ifile(path);
      img = ifile.frame(0);
      if (fmt)
	*fmt = ifile.format();
    } catch (...) { }

    if (img.isempty()) { // iszero
      if (angry)
	ShowError(msg+"reading <"+path+"> failed");
      else
	WriteLog(msg+"<"+path+"> was a broken or non-image file");
    }
    
    return img;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlStoreSegmentationFile(size_t idx, const string& sname,
					  const string& file) const {
    string hdr = "DataBase::SqlStoreSegmentationFile() : ";

    if (!UseSql())
      return ShowError(hdr+"not using SQL");

    if (SqlMode()!=2)
      return ShowError(hdr+"SQL opened read only");

    if (!FileExists(file))
      return ShowError(hdr+"file <"+file+"> does not exist");

    string data = FileToString(file);
    string blob = SqlBlob(data);
    string nowblob = SqlTimeBlob(TimeNow());
    string modblob = SqlTimeBlob(FileModTime(file));
    string user = Picsom()->UserName();
    string cmd = "INSERT INTO ";
    cmd += SqlTableName("segmentations", SqlStyle())
      +" VALUES("+ToStr(idx)+",'"+sname+"',"+modblob+","+nowblob+",'"+user
      +"',"+blob+");";
    if (DebugSql())
      cout << TimeStamp() << hdr << cmd << endl;
    if (!SqlExec(cmd, true))
      return false;    

    WriteLog("Stored segmentation <"+ShortFileName(file)+"> ("+
	     ToStr(data.size())+" bytes) in SQL");
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlRetrieveSegmentationFile(size_t idx, const string& sname,
					     const string& file) const {
    string hdr = "DataBase::SqlRetrieveSegmentationFile() : ";

    if (!UseSql())
      return ShowError(hdr+"not using SQL");

    string cmd = "SELECT data FROM segmentations WHERE indexz='"+
      ToStr(idx)+"' AND method='"+sname+"';";
    if (DebugSql())
      WriteLog(hdr+cmd);

    size_t n = 0;
    string data;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return ShowError(hdr+"segmentation <"+sname+"> for #"+ToStr(idx)+
			 " not found in SQL");

      for (;; n++) {
	int rrr = sqlite3_step(stmt);
	
	if (rrr!=SQLITE_ROW)
	  break;
	
	if (DebugSql())
	  SqliteShowRow(stmt);

	data = SqliteString(stmt, 0);
      }
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return ShowError(hdr+"segmentation <"+sname+"> for #"+ToStr(idx)+
			 " not found in SQL");

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
	
      size_t len = 10*1024*1024;
      char *buffer = new char[len];
      unsigned long length = 0;
      my_bool is_null = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = buffer;
      bind[0].buffer_length = len;
      bind[0].is_null       = &is_null;
      bind[0].length        = &length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      for (;; n++) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(hdr+"data truncated");
	  else
	    ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	data = string(buffer, length);
      }
      MysqlStmtClose(stmt);

      delete buffer;
    }
#endif // HAVE_MYSQL_H

    if (n!=1)
      return ShowError(hdr+"SQL contained "+ToStr(n)+"!=1 segmentations <"+
		       sname+"> for #"+ToStr(idx));

    if (!StringToFile(data, file))
      return ShowError(hdr+"failed to write in <"+file+">");

    WriteLog("Retrieved segmentation <"+ShortFileName(file)+"> from SQL");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlStoreFileFromFile(const string& name,
				      const string& file) const {
    string hdr = "DataBase::SqlStoreFileFromFile() : ";

    if (!FileExists(file))
      return ShowError(hdr+"file <"+file+"> does not exist");

    string data = FileToString(file);

    return SqlStoreFile(name, data, FileModTime(file));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlStoreFile(const string& name, const string& data,
			      const struct timespec& modtime) const {
    string hdr = "DataBase::SqlStoreFile() : ";
    
    if (!UseSql())
      return ShowError(hdr+"not using SQL");

    if (SqlMode()!=2)
      return ShowError(hdr+"SQL opened read only");

    struct timespec nowtime = TimeNow();
    string nowblob = SqlTimeBlob(nowtime);
    string modblob = SqlTimeBlob(modtime);
    string user = Picsom()->UserName();

    string blob = SqlBlob(data);
    string cmd =  SqliteMode()?"INSERT OR REPLACE INTO ":"REPLACE ";
    cmd += SqlTableName("files", SqlStyle())
      + " VALUES('"+name+"',"+modblob+","+nowblob+",'"+user+"',"+blob+");";

    if (DebugSql())
      cout << TimeStamp() << hdr << cmd << endl;

    if (!SqlExec(cmd, true))
      return false;    

    WriteLog("Stored <"+name+"> ("+ToStr(data.size())+" bytes) in SQL");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlRetrieveFileToFile(const string& name,
				       const string& file) const {
    string err = " DataBase::SqlRetrieveFileToFile() : ";
    
    string data;
    if (!SqlRetrieveFile(name, data))
      return ShowError(err+"SqlRetrieveFile() failed");

    if (!StringToFile(data, file))
      return ShowError(err+"failed to write "+ToStr(data.size())+
		       " bytes in <"+file+">");
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlRetrieveFile(const string& name, string& data,
				 bool silent) const {
    string hdr = "DataBase::SqlRetrieveFile() : ";
    
    if (!UseSql())
      return ShowError(hdr+"not using SQL");

    string cmd = "SELECT content FROM files WHERE name='"+name+"';";
    if (DebugSql())
      WriteLog(hdr+cmd);

    int n = 0;
      
#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return ShowError(hdr+"file <"+name+"> not found in SQL");

      for (;; n++) {
	int rrr = sqlite3_step(stmt);
	
	if (rrr!=SQLITE_ROW)
	  break;
	
	if (DebugSql())
	  SqliteShowRow(stmt);

	data = SqliteString(stmt, 0);
      }
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return ShowError(hdr+"file <"+name+"> not found in SQL");

      // int nc = mysql_stmt_field_count(stmt);
      // cout << "nc=" << nc << endl;

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
	
      size_t len = 10*1024*1024;
      char *buffer = new char[len];
      unsigned long length = 0;
      my_bool is_null = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = buffer;
      bind[0].buffer_length = len;
      bind[0].is_null       = &is_null;
      bind[0].length        = &length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      for (;; n++) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(hdr+"data truncated");
	  else
	    ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	data = string(buffer, length);
      }
      MysqlStmtClose(stmt);

      delete buffer;
    }
#endif // HAVE_MYSQL_H

    if (n!=1)
      return ShowError(hdr+"SQL contained "+ToStr(n)+"!=1 files <"+name+">");
    
    if (!silent)
      WriteLog("Retrieved <"+name+"> ("+ToStr(data.size())+" bytes) from SQL");
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDeleteFile(const string& name) const {
    string hdr = "DataBase::SqlDeleteFile() : ";
    
    if (!UseSql())
      return ShowError(hdr+"not using SQL");

    if (SqlMode()!=2)
      return ShowError(hdr+"SQL opened read only");

    string cmd = "DELETE FROM files WHERE name='"+name+"';";
    if (DebugSql())
      cout << TimeStamp() << hdr << cmd << endl;

    if (!SqlExec(cmd, true))
      return false;    

    WriteLog("Deleted <"+name+"> from SQL");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlObjectData(size_t idx) const {
    string hdr = "DataBase::SqlObjectData() : ";
    
    if (!SqlObjects())
      return ShowErrorS(hdr+"not using SQL data");

    string cmd = "SELECT data FROM objects WHERE indexz="+ToStr(idx)+";";
    if (DebugSql())
      cout << hdr << cmd << endl;

    string data;
    int n = 0;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, true);
      if (!stmt)
	return ShowErrorS(hdr+"SqlitePrepare() failed");

      for (;; n++) {
	int rrr = sqlite3_step(stmt);
      
	if (rrr!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	data = SqliteString(stmt, 0);
      }
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, true);
      if (!stmt)
	return ShowErrorS(hdr+"MysqlPrepare() failed");

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
	
      size_t len = 10*1024*1024;
      char *buffer = new char[len];
      unsigned long length = 0;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      bind[0].buffer_type   = MYSQL_TYPE_STRING;
      bind[0].buffer        = buffer;
      bind[0].buffer_length = len;
      bind[0].length        = &length;

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      for (;; n++) {
	int rrr = mysql_stmt_fetch(stmt);
	
	if (rrr) {
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED)
	    ShowError(hdr+"data truncated");
	  else
	    ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	data = string(buffer, length);
      }
      MysqlStmtClose(stmt);
      delete buffer;
    }
#endif // HAVE_MYSQL_H

    if (n!=1)
      return ShowErrorS(hdr+"n=="+ToStr(n)+"!=1");
    
    if (data.empty())
      return ShowErrorS(hdr+"empty data");
    
    return data;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlEscape(const string& f, bool fwd, bool new_alias) {
    string hdr = "DataBase::SqlEscape() : ";

    string fname = f;

    MysqlLock();

    if (fwd) {
      if (DebugSql())
	cout << hdr << "searching for SQL alias of <" << f << ">" << endl;

      map<string,string>::const_iterator i = sql_alias.begin();
      while (i!=sql_alias.end() && i->second!=f)
	i++;
      if (i!=sql_alias.end()) {
	if (DebugSql())
	  cout << "  found <" << i->first << ">" << endl;
	MysqlUnlock();

	return "__A"+i->first+"__";
      }
      if (DebugSql())
	cout << "  NOT found" << endl;

      while (1) {
	size_t p = fname.find_first_of(":-#.");
	if (p==string::npos)
	  break;
	if (fname[p]==':')
	  fname.replace(p, 1, "__C__");
	if (fname[p]=='-')
	  fname.replace(p, 1, "__M__");
	if (fname[p]=='#')
	  fname.replace(p, 1, "__S__");
	if (fname[p]=='.')
	  fname.replace(p, 1, "__D__");
      }
      while (1) {
	string s = "§";
	size_t p = fname.find(s);
	if (p==string::npos)
	  break;
	fname.replace(p, s.size(), "__P__");
      }

      if (fname.size()>50 && new_alias) {
	string a = ToStr(sql_alias.size());
	SqlAddAlias(a, f);
	sql_alias[a] = f;
	fname = "__A"+a+"__";
      }

      MysqlUnlock();

      return fname;
    }

    for (;;) {
      size_t p = fname.find("__C__");
      if (p==string::npos)
	break;
      fname.replace(p, 5, ":");
    }
    for (;;) {
      size_t p = fname.find("__M__");
      if (p==string::npos)
	break;
      fname.replace(p, 5, "-");
    }
    for (;;) {
      size_t p = fname.find("__S__");
      if (p==string::npos)
	break;
      fname.replace(p, 5, "#");
    }
    for (;;) {
      size_t p = fname.find("__D__");
      if (p==string::npos)
	break;
      fname.replace(p, 5, ".");
    }
    for (;;) {
      size_t p = fname.find("__P__");
      if (p==string::npos)
	break;
      fname.replace(p, 5, "§");
    }

    size_t p = fname.find("__A");
    if (p!=string::npos) {
      size_t q = fname.find("__", p+3);
      if (q!=string::npos && q!=p+3) {
	string a = fname.substr(p+3, q-p-3);
	map<string,string>::const_iterator i = sql_alias.find(a);
	if (i!=sql_alias.end())
	  fname.replace(p, q-p+2, i->second);
      }
    }

    MysqlUnlock();

    return fname;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlFeatureIsBlob(const string& f) {
    map<string,bool>::iterator i = sql_blob_info.find(f);
    if (i!=sql_blob_info.end())
      return i->second;

    bool is_blob = true;
    sql_blob_info[f] = is_blob;

    return is_blob;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> DataBase::SqlFeatureNames(const string& f, bool allow) {
    string fname = SqlEscape(f, true, allow), tname = "features_"+fname;

    return make_pair(tname, fname);
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t DataBase::SqlFeatureDataCount(const string& fin, bool angry) {
    string hdr = "DataBase::SqlFeatureDataCount("+fin+") : ";
    
    string f = SqlFeatureNames(fin, true).first;

    if (!SqlFeatures()) {
      ShowError(hdr+"not using SQL feature data");
      return 0;
    }

    string cmd = "SELECT indexz FROM "+f+";";
    if (DebugSql())
      cout << hdr << cmd << endl;

    size_t ret = 0;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, angry);
      if (stmt) {
	while (sqlite3_step(stmt)==SQLITE_ROW)
	  ret++;
	sqlite3_finalize(stmt);
      }
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, angry);
      if (stmt && !mysql_stmt_execute(stmt)) {
	int idx;
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));
	bind[0].buffer_type   = MYSQL_TYPE_LONG;
	bind[0].buffer        = &idx;
	bind[0].buffer_length = sizeof(idx);
	
	if (mysql_stmt_bind_result(stmt, bind)) {
	  ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
	}
	while (mysql_stmt_fetch(stmt)==0)
	  ret++;
	MysqlStmtClose(stmt);
      }
    }
#endif // HAVE_MYSQL_H

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVectorSet DataBase::SqlFeatureDataSet(const string& fin,
					     const string& spec,
					     bool angry) {
    string hdr = "DataBase::SqlFeatureDataSet("+fin+","+spec+") : ";
    
    FloatVectorSet ret;

    if (!SqlFeatures()) {
      ShowError(hdr+"not using SQL feature data");
      return ret;
    }

    string f = SqlFeatureNames(fin, true).first;
    string cmd = "SELECT * FROM "+SqlTableName(f);
    if (spec!="")
      cmd += " WHERE "+spec;
    cmd += ";";

    bool is_blob = ((DataBase*)this)->SqlFeatureIsBlob(f);

    if (DebugSql())
      cout << hdr << cmd << " is_blob=" << is_blob << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      sqlite3_stmt *stmt = SqlitePrepare(cmd, angry);
      if (!stmt)
	return ret;

      for (;;) {
	int rrr = sqlite3_step(stmt);
      
	if (rrr!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	FloatVector *v = SqliteFloatVector(stmt);
	ret.Append(v);
      }
      sqlite3_finalize(stmt);
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      MYSQL_STMT *stmt = MysqlPrepare(cmd, angry);
      if (!stmt)
	return ret;

      int nc = mysql_stmt_field_count(stmt);
      // cout << "nc=" << nc << endl;
      if (nc<2) {
	ShowError(hdr+"mysql_stmt_field_count()<2 : "+MysqlErrMsg(mysql));
	MysqlStmtClose(stmt);
	return ret;
      }

      if (is_blob && nc!=2) {
	ShowError(hdr+"mysql_stmt_field_count()!=2 with blob: "+
		  MysqlErrMsg(mysql));
	MysqlStmtClose(stmt);
	return ret;
      }

      if (mysql_stmt_execute(stmt)) {
	ShowError(hdr+"mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));
      }
      
      int idx = -1, veclen = nc-1;
      MYSQL_BIND *bind = new MYSQL_BIND[nc];
      memset(bind, 0, sizeof(MYSQL_BIND));
      bind[0].buffer_type   = MYSQL_TYPE_LONG;
      bind[0].buffer        = &idx;
      bind[0].buffer_length = sizeof(idx);

      long unsigned int blob_length = 0;
      float *fv = new float[nc-1], *vecf = fv, *blobf = NULL;

      if (is_blob) {
	// workaround for http://bugs.mysql.com/bug.php?id=33086 ???
	size_t blobfsize = 1000000; // OBS!
	vecf = blobf = new float[blobfsize];
	memset(bind+1, 0, sizeof(MYSQL_BIND));
	bind[1].buffer_type   = MYSQL_TYPE_LONG_BLOB;
	bind[1].buffer        = blobf;                   // NULL;
	bind[1].buffer_length = blobfsize*sizeof(float); // 0;
	bind[1].length        = &blob_length;

      } else
	for (int i=1; i<nc; i++) {
	  memset(bind+i, 0, sizeof(MYSQL_BIND));
	  bind[i].buffer_type   = MYSQL_TYPE_FLOAT;
	  bind[i].buffer        = &fv[i-1];
	  bind[i].buffer_length = sizeof(float);
	}	

      if (mysql_stmt_bind_result(stmt, bind)) {
	ShowError(hdr+"mysql_stmt_bind_result() failed: "+MysqlErrMsg(mysql));
      }

      for (;;) {
	int rrr = mysql_stmt_fetch(stmt);
      
	if (rrr) {
	  bool do_break = true;
	  if (rrr==MYSQL_NO_DATA)
	    ; // cout << "no data" << endl;
	  else if (rrr==MYSQL_DATA_TRUNCATED) {
	    if (is_blob && !blobf)
	      do_break = false;
	    else
	      ShowError(hdr+"data truncated");
	  } else
	    ShowError(hdr+"rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));

	  if (do_break)
	    break;
	}

	if (DebugSql())
	  MysqlShowRow(stmt);

	if (is_blob) {
	  if (!blob_length)
	    ShowError(hdr+"failed to get blob size");
	  if (blob_length%sizeof(float))
	    ShowError(hdr+"weird blob size "+ToStr(blob_length));
	  veclen = blob_length/sizeof(float);
	  if (DebugSql())
	    cout << hdr << "veclen=" << veclen << endl;
	  if (!blobf) {
	    vecf = blobf = new float[veclen];
	    memset(blobf, 0, veclen*sizeof(float));
	    memset(bind+1, 0, sizeof(MYSQL_BIND));
	    bind[1].buffer_type = MYSQL_TYPE_LONG_BLOB;
	    bind[1].buffer = blobf;
	    bind[1].buffer_length = blob_length;
	    if (mysql_stmt_bind_result(stmt, bind))
	      ShowError(hdr+"mysql_stmt_bind_result() failed B : "+
			MysqlErrMsg(mysql));

	    rrr = mysql_stmt_fetch_column(stmt, bind+1, 1, 0);
	    if (rrr)
	      ShowError(hdr+" X rrr="+ToStr(rrr)+" "+MysqlErrMsg(mysql));
	  }
	}
	
	FloatVector *v = new FloatVector(veclen, vecf, Label(idx).c_str());
	v->Number(idx);
	
	ret.Append(v);
      }
      MysqlStmtClose(stmt);

      delete [] blobf;
      delete [] bind;
      delete [] fv;
    }
#endif // HAVE_MYSQL_H

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::UploadBinFeatures(VectorIndex& vidx) {
    string hdr = "DataBase::BinFeatures("+vidx.Name()+") : ";

    string osdbname = Name();
    if (osdbname.find("os://")!=0)
      return ShowError(hdr+"database <"+Name()+"> is not in OpenStack");
    osdbname.erase(0,5);

    const string& fpath = vidx.BinInfoFileName();
    string fname = fpath;
    size_t p = fname.rfind('/');
    if (p!=string::npos)
      fname.erase(0, p+1);

    vector<string> cmd { "swift", "upload", osdbname, fpath,
	"--object-name", fname };

    int r = Picsom()->ExecuteSystem(cmd, true, true, true);

    if (r==0) {
      WriteLog("Successfully uploaded "+ToStr(FileSize(fpath))+" bytes of <"
	       +ShortFileName(fpath)+"> to OpenStack");
      return true;
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVector *DataBase::FeatureData(const string& fin, size_t idx,
				     bool angry) {
    string hdr = "DataBase::FeatureData("+fin+","+ToStr(idx)+") : ";

    auto p = feature_txt_files.find(fin);
    if (p!=feature_txt_files.end() && p->second.size())
      return FeatureDataCombined(fin, idx, angry);

    if (UseBinFeaturesRead()) {
      VectorIndex *vidx = dynamic_cast<VectorIndex*>(FindIndex(fin, "", true));
      if (!vidx) {
	if (angry)
	  ShowError(hdr, "feature not found");
	else
	  return NULL;
      }
      const string augm;
      if (!vidx->BinDataOpen(OpenReadWriteFea(), Size(), false, augm)) {
	ShowError(hdr, "opening binary features data file failed");
	return NULL;
      }
      FloatVector *v = vidx->BinDataFloatVector(idx);
      if (v)
	return v;

      v = FeatureDataNumPy(fin, idx, angry);
      if (v)
	return v;

      if (FeatureDataCombinedPrepare(fin, true))
	return FeatureDataCombined(fin, idx, angry);
    }

    if (SqlFeatures())
      return SqlFeatureData(fin, idx, angry);

    return NULL;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  FloatVector *DataBase::FeatureDataCombined(const string& fin, size_t idx,
					     bool angry) {
    string hdr = "DataBase::FeatureDataCombined("+fin+","+ToStr(idx)+") : ";

    if (feature_txt_files.find(fin)==feature_txt_files.end()) {
      ShowError(hdr+"called but doesn't exist");
      return NULL;
    }

    const vector<string>& vl = feature_txt_files.at(fin);
    if (vl.size()==0)
      return NULL;

    FloatVector *v_all = NULL;
    for (size_t i=0; i<vl.size(); i++) {
      FloatVector *v_one = FeatureData(vl[i], idx, angry);
      if (!v_one) {
	if (angry)
	  ShowError(hdr+"<"+vl[i]+"> i="+ToStr(i)+" returned NULL");
	delete v_all;
	return NULL;
      }
      if (i==0)
	v_all = new FloatVector(*v_one);
      else
	v_all->Append(*v_one);

      delete  v_one;
    }

    return v_all;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::FeatureDataCombinedPrepare(const string& fin, bool angry) {
    string hdr = "DataBase::FeatureDataCombinedPrepare("+fin+") : ";

    if (feature_txt_files.find(fin)!=feature_txt_files.end())
      return false;

    string fn = ExpandPath("features")+"/"+fin+".txt";
    if (!FileExists(fn)) {
      if (DebugFeatures())
	WriteLog(hdr+"file <"+fn+"> doesn't exist");
      feature_txt_files[fin] = vector<string>();
      return false;
    }
      
    string st = FileToString(fn);
    vector<string> l = SplitInSomething("\n", true, st);
    vector<string> a;
    for (auto i=l.begin(); i!=l.end(); i++)
      if (i->substr(0, 1)!="#" && i->find_first_not_of(" \t")!=string::npos) {
	string f = *i, ext;
	auto p = f.rfind('/');
	if (p!=string::npos)
	  f.erase(0, p+1);
	p = f.rfind('.');
	if (p!=string::npos) {
	  ext = f.substr(p+1);
	  if (ext=="bin" || ext=="npy" || ext=="txt")
	    a.push_back(f.substr(0, p));
	  else
	    a.push_back(f);
	} else
	    a.push_back(f);
      }
    feature_txt_files[fin] = a;

    if (a.size()==0 && angry)
      ShowError(hdr+"empty file <"+fn+">");

    if (DebugFeatures())
      WriteLog(hdr+"expands to ["+JoinWithString(a, "][")+"]");

    return a.size();
  }
    
  /////////////////////////////////////////////////////////////////////////////

  FloatVector *DataBase::FeatureDataNumPy(const string& fin, size_t idx,
					  bool angry) {
    string hdr = "DataBase::FeatureDataNumPy("+fin+","+ToStr(idx)+") : ";

    auto info = numpy_feature_info.find(fin);
    if (info!=numpy_feature_info.end() && info->second.first.first==NULL)
      return NULL;

    if (info==numpy_feature_info.end()) {
      string fn = ExpandPath("features")+"/"+fin+".npy";
      if (!FileExists(fn)) {
	numpy_feature_info[fin] = make_pair(make_pair((ifstream*)NULL, 0),
					    make_pair(0, 0));
	if (DebugFeatures())
	  WriteLog(hdr+"file <"+fn+"> doesn't exist");
	return NULL;
      }

      ifstream *in = new ifstream(fn);
      size_t o = 0, s = 0, r = 0, c = 0;
      if (!ReadNumPyHeader(*in, o, s, r, c)) {
	if (angry)
	  ShowError(hdr+"failed reading header <"+fn+">");
	return NULL;
      }

      numpy_feature_info[fin] = make_pair(make_pair(in, o), make_pair(r, c));
      info = numpy_feature_info.find(fin);
    }

    size_t o = info->second.first.second;
    size_t r = info->second.second.first;
    size_t c = info->second.second.second;
    size_t d = c;
    vector<float> nv = ReadNumPyVector(*info->second.first.first, o, 16, r, c,
				       idx, true);
    if (nv.size()!=d) {
      ShowError(hdr+"failed reading nympy vector");
      return NULL;
    }

    FloatVector *v = new FloatVector(d);
    for (size_t i=0; i<d; i++)
      v->Set(i, nv[i]);

    return v;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  FloatVector *DataBase::SqlFeatureData(const string& fin, size_t idx,
					bool angry) const {
    string hdr = "DataBase::SqlFeatureData("+fin+","+ToStr(idx)+") : ";
    
    string spec = "indexz="+ToStr(idx);

    Tic("SqlFeatureData");

    FloatVectorSet set = ((DataBase*)this)->SqlFeatureDataSet(fin, spec, angry);

    FloatVector *ret = NULL;

    size_t n = set.Nitems();
    if (n!=1) {
      if (n!=0 || angry)
	ShowError(hdr+"n=="+ToStr(n)+"!=1");
      set.Delete();

    } else {
      ret = set.Get(0);
      set.Relinquish(0);
    }

    Tac("SqlFeatureData");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t DataBase::SqlFeatureLength(const string& fin, bool angry) {
    string hdr = "DataBase::SqlFeatureLength("+fin+") : ";
    
    string f = SqlFeatureNames(fin, true).first;

    if (!SqlFeatures()) {
      ShowError(hdr+"not using SQL feature data");
      return 0;
    }

    for (size_t idx=0; idx<Size(); idx++) {
      string cmd = "SELECT * FROM "+f+" WHERE indexz="+ToStr(idx)+";";
      if (DebugSql())
	cout << hdr << cmd << endl;

#ifdef HAVE_SQLITE3_H
      if (SqliteMode()) {
	sqlite3_stmt *stmt = SqlitePrepare(cmd, angry);
	if (!stmt)
	  return 0;

	int r = sqlite3_step(stmt);

	if (r!=SQLITE_ROW) {
	  sqlite3_finalize(stmt);
	  continue;
	}

	if (DebugSql())
	  SqliteShowRow(stmt);

	FloatVector *v = SqliteFloatVector(stmt);
	sqlite3_finalize(stmt);
      
	size_t l = v->Length();
	delete v;

	return l;
      }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
      if (MysqlMode()) {
	MYSQL_STMT *stmt = MysqlPrepare(cmd, angry);
	if (!stmt)
	  return 0;

	int nc = mysql_stmt_field_count(stmt);

	MysqlStmtClose(stmt);

	return nc-1;
      }
#endif // HAVE_MYSQL_H

    }

    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlTableName(const string& t, const string& m) {
    if (m=="sqlite3")
      return "\""+t+"\"";

    if (m=="mysql")
      return t;

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlBeginTransaction() {
    string hdr = "DataBase::SqlBeginTransaction() : ";
    string cmd = SqlBeginTransactionCmd(SqlStyle());

    if (cmd!="" && !SqlExec(cmd, true)) {
      ShowError(hdr+"SqlExec() failed with <"+cmd+">");
      SqlClose();
      return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlEndTransaction() {
    string hdr = "DataBase::SqlEndTransaction() : ";
    string cmd = SqlEndTransactionCmd(SqlStyle());

    if (cmd!="" && !SqlExec(cmd, true)) {
      ShowError(hdr+"SqlExec() failed with <"+cmd+">");
      SqlClose();
      return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlBeginTransactionCmd(const string& m) {
    if (m=="sqlite3")
      return "BEGIN TRANSACTION;";

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlEndTransactionCmd(const string& m) {
    if (m=="sqlite3")
      return "COMMIT;";
    
    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  FloatVector *DataBase::SqliteFloatVector(sqlite3_stmt *stmt) const {
    int idx = SqliteInt(stmt, 0), vl = sqlite3_column_count(stmt)-1;
    float *fbuf = NULL;

    if (vl==1 && sqlite3_column_type(stmt, 1)==SQLITE_BLOB) {
      int nb = sqlite3_column_bytes(stmt, 1);
      if (nb%sizeof(float))
	ShowError("DataBase::SqliteFloatVector() : weird byte count in BLOB");
      vl = nb/sizeof(float);
      fbuf = new float[vl];
      const char *b = (const char*)sqlite3_column_blob(stmt, 1);
      memcpy(fbuf, b, vl*sizeof(float));
    }

    FloatVector *v = new FloatVector(vl, fbuf, Label(idx).c_str());
    v->Number(idx);
    if (!fbuf)
      for (int i=0; i<vl; i++) {
	double val = SqliteDouble(stmt, i+1);
	// cout << "n=" << n << " i=" << i << " val=" << val << endl;
	(*v)[i] = val;
      }

    delete [] fbuf;

    return v;
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDump(const string& sqllang, bool do_pipe,
			 const string& file, const string& spec,
			 const Query *q) {
    string err = "DataBase::SqlDump() : ";

    if (sqllang!="mysql" && sqllang!="sqlite3")
      return ShowError(err+"either mysql or sqlite3 dhould be specified");

    if (!do_pipe) {
      WriteLog("Dumping data to "+sqllang+" text file <"+file+">");

      ostream *osp = &cout;
      ofstream os;
      if (file!="-") {
	os.open(file.c_str());
	osp = &os;
      }
      return SqlDump(*osp, spec, q, sqllang, file);
    }

#ifdef HAVE_EXT_STDIO_FILEBUF_H
    Unlink(file);
    string cmd = sqllang;
    if (sqllang=="mysql")
      cmd += " --user=jorma --password=Awe20fB";
    if (sqllang=="sqlite3")
      cmd += " "+file;

    if (DebugSql())
      cout << err << "cmd=[" << cmd << "]" << endl;

    FILE *p = popen(cmd.c_str(), "w");
    __gnu_cxx::stdio_filebuf<char> sfb(p, ios::out);
    fstream pipe;
    pipe.ios::rdbuf(&sfb);
    bool r = SqlDump(pipe, spec, q, sqllang, file);
    pclose(p);
    return r;
#else
    return ShowError(err+"__gnu_cxx::stdio_filebuf<char> not available");
#endif // HAVE_EXT_STDIO_FILEBUF_H
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDump(ostream& os, const string& spec, const Query *q,
			 const string& m, const string& file) {
    bool all = spec=="", fea = true;

    if (m=="mysql") {
      string dbname = "picsom_"+file;
      os << "CREATE DATABASE " << dbname << ";" << endl
	 << "USE " << dbname << ";" << endl;
    }

    if (spec.find("emptyfeature")!=string::npos) {
      fea = false;
      if (!SqlDumpFeatures(os, m, q, false))
	return false;
    }

    if ((all || spec.find("version")!=string::npos) &&
	!SqlDumpVersion(os, m))
      return false;

    if ((all || spec.find("object")!=string::npos) &&
	!SqlDumpObjects(os, m))
      return false;

    if ((all || (fea && spec.find("feature")!=string::npos)) &&
	!SqlDumpFeatures(os, m, q, true))
      return false;

    if ((all || spec.find("class")!=string::npos) &&
	!SqlDumpClasses(os, m))
      return false;

     if ((all || spec.find("file")!=string::npos) &&
	!SqlDumpFiles(os, m))
      return false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDumpVersion(ostream& os, const string& m) {
    string spec;
    if (m=="sqlite3")
      spec = "keyz,value";
    if (m=="mysql")
      spec = "keyz VARCHAR(100),value VARCHAR(100)";

    if (m=="sqlite3")
      os << "BEGIN TRANSACTION;" << endl;
    os   << "CREATE TABLE version(" << spec << ");" << endl
	 << "INSERT INTO " << SqlTableName("version", m)
	 << " VALUES('DataBase.C','"
	 << PicSOM::ExtractVersion(DataBase_C_vcid) << "');" << endl;
    if (m=="sqlite3")
      os << "COMMIT;" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDumpObjects(ostream& os, const string& m) {
    // this should be harmonized with UpdateOriginsInfoSql()

    string specsqlite3 =
      "indexz    INTEGER PRIMARY KEY,"
      "label     UNIQUE NOT NULL,"
      "type      INTEGER,"
      "auxid     ,"
      "file      ,"
      "url       ,"
      "page      ,"
      "mime      ,"
      "colors    ,"
      "width     INTEGER,"
      "height    INTEGER,"
      "frames    INTEGER,"
      "framerate FLOAT,"
      "bytes     INTEGER,"
      "md5       ,"
      "moddate   BLOB(16),"
      "insdate   BLOB(16),"
      "user      ,"
      "data      BLOB";

    string specmysql =
      "indexz    INTEGER PRIMARY KEY,"
      "label     VARCHAR(20) NOT NULL,"
      "type      INTEGER,"
      "auxid     VARCHAR(20),"
      "file      VARCHAR(20),"
      "url       VARCHAR(100),"
      "page      VARCHAR(20),"
      "mime      VARCHAR(20),"
      "colors    VARCHAR(20),"
      "width     INTEGER,"
      "height    INTEGER,"
      "frames    INTEGER,"
      "framerate FLOAT,"
      "bytes     INTEGER,"
      "md5       VARCHAR(20),"
      "moddate   BINARY(16),"
      "insdate   BINARY(16),"
      "user      VARCHAR(8),"
      "data      LONGBLOB";

    // subrange ???

    if (m=="sqlite3")
      os << "BEGIN TRANSACTION;" << endl;
    os   << "CREATE TABLE objects("
	 << (m=="mysql"?specmysql:specsqlite3)
	 << ");" << endl;

    for (size_t i=0; i<Size(); i++) {
      bool prune = false, warn = false;
      const string &lab = Label(i);
      map<string,string> hash = ReadOriginsInfo(i, prune, warn);

      int w = 0, h = 0, f = 1, s = 0;
      float r = 0.0;
      const char *d = hash["dimensions"].c_str();
      sscanf(d, "%dx%dx%d@%f", &w, &h, &f, &r);

      if (hash["size"]!="")
        s = atoi(hash["size"].c_str());

      string url = hash["url"];
      for (size_t j=0; j<url.size(); j++)
	if (url[j]=='\'')
	  url.insert(j++, "\'");

      string rstr = ToStr(r);
      if (rstr.find('.')==string::npos)
	rstr += ".0";
      
      struct timespec now = TimeNow();
      struct timespec mod = Picsom()->InverseOriginsDateString(hash["date"]);
      string nowblob = SqlTimeBlob(now), modblob = SqlTimeBlob(mod);
      string user = Picsom()->UserName();

      stringstream ss;
      ss << i << ",'"
         << lab << "',"
         << (int)ObjectsTargetType(i) << ",'"
         << hash["name"] << "','"
         << url << "','"
         << hash["pageurl"] << "','"
         << hash["format"] << "','"
         << hash["colors"] << "',"
         << w << ","
         << h << ","
         << f << ","
         << rstr << ","
         << s << ",'"
         << hash["checksum"] << "',"
         << modblob << ","
         << nowblob << ",'"
         << user << "'";
      if (true || m=="sqlite3") {
	string blob = SqlObjectData(i);
	ss << "," << SqlBlob(blob);
      }

      string str = ss.str();
      os << "INSERT INTO " << SqlTableName("objects", m)
	 << " VALUES(" << str << ");" << endl;
    }

    if (m=="sqlite3")
      os << "COMMIT;" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDumpFeatures(ostream& os, const string& m,
				 const Query *q, bool full) {
    ((Query*)q)->ReadFiles(false);

    for (size_t i=0; i<q->Nindices(); i++) {
      const Index *index = &q->IndexStaticData(NULL, i);
      if (index->IsDummy()) {
	cout << "SqlDumpFeatures() : [" << index->FeatureFileName()
	     << "] is dummyindex" << endl;
	continue;
      }

      VectorIndex *vidx = (VectorIndex*)index;
      if (full)
	vidx->ReadDataFile();

      // TSSOM *tsidx = (TSSOM*)vidx;
      // cout << "XXX " << vidx->VectorLength()
      //      << " " << vidx->GuessVectorLength()
      //      << " " << tsidx->GuessVectorLength()
      //      << endl;

      vidx->SqlDumpData(os, full, m);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDumpClasses(ostream&, const string&) {
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SqlDumpFiles(ostream& os, const string& m) {
    string hdr = "DataBase::SqlDumpFiles() : ";

    string csqlite = "name UNIQUE,moddate BLOB(16),insdate BLOB(16),"
      "user,content BLOB";
    string cmysql  = "name VARCHAR(50) UNIQUE,"
      "moddate BINARY(16),insdate BINARY(16),user VARCHAR(8),"
      "content LONGBLOB";

    os << "CREATE TABLE files(" << (m=="mysql"?cmysql:csqlite) << ");" << endl;

#ifdef HAVE_SQLITE3_H
    if (SqliteMode()) {
      string cmd = "SELECT name,moddate,insdate,user FROM files;";
      if (DebugSql())
	cout << hdr << cmd << endl;

      sqlite3_stmt *stmt = SqlitePrepare(cmd, false);
      if (!stmt)
	return true;

      bool ok = true;

      for (; ok;) {
	int r = sqlite3_step(stmt);
      
	if (r!=SQLITE_ROW)
	  break;

	if (DebugSql())
	  SqliteShowRow(stmt);

	string name = SqliteString(stmt, 0);
	string modd = SqlBlob(SqliteString(stmt, 1));
	string insd = SqlBlob(SqliteString(stmt, 2));
	string user = SqliteString(stmt, 3);

	string data;
	ok = SqlRetrieveFile(name, data);
	if (!ok)
	  ShowError(hdr+"retrieving <"+name+"> failed");
	else {
	  string blob = SqlBlob(data);
	  os << "INSERT INTO " << SqlTableName("files", m)
	     << " VALUES('"+name+"',"+modd+","+insd+",'"+user+"',"
	    +blob+");" << endl;
	}
      }
      sqlite3_finalize(stmt);

      return ok;
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (MysqlMode()) {
      return true;
      return ShowError("not implemented") || true;
    }
#endif // HAVE_MYSQL_H

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlBlob(const char *s, size_t bs) {
    string str(2*bs, '0');
    const char *hd = "0123456789ABCDEF";
    for (size_t i=0; i<bs; i++) {
      unsigned char c = s[i];
      str[2*i]   = hd[c/16];
      str[2*i+1] = hd[c%16];
    }
    return "X'"+str+"'";
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqlTimeBlob(const struct timespec& t) {
    string s((char*)&t, sizeof(t));
    return SqlBlob(s);
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::SqliteDBFile(bool use_temp) {
    string fname = "sqlite3.db"; // , ext = ".sqlite3";

    if (path!=permanentpath) {
      if (!use_temp)
	return permanentpath; // this should end with ext...

      return TempDir("")+"/"+fname;
    }

    if (!use_temp)
      return ExpandPath(fname);

    return TempDir("")+"/"+fname;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  bool DataBase::SqliteOpen(const string& f, bool create, bool write) {
    if (!create && !FileExists(f)) {
      // WriteLog("sqlite3 database file <"+f+"> doesn't exist");
      // return false;
      return ShowError("sqlite3 database file <"+f+"> doesn't exist");
    }
    if (create && FileExists(f)) {
      // WriteLog("sqlite3 database file <"+f+"> already exists");
      // return false;
      return ShowError("sqlite3 database file <"+f+"> already exists");
    }

    Tic("SqliteOpen");

    bool ronly = false;

#ifdef SQLITE_OPEN_READONLY
    int flags = create ? SQLITE_OPEN_CREATE : 0;
    flags |= SQLITE_OPEN_READWRITE;
    if (!create && !write) {
      flags = SQLITE_OPEN_READONLY;
      ronly = true;
    }
    int rrr = sqlite3_open_v2(f.c_str(), &sql3, flags, NULL);
#else
    int a_w = ::access(f.c_str(), W_OK); 
    if (a_w) {
      if (write)
	return ShowError("sqlite3 db <"+f+"> has no write permissions");
      else
	ronly = true;
    }
    int rrr = sqlite3_open(f.c_str(), &sql3);
#endif // SQLITE_OPEN_READONLY

    if (rrr!=SQLITE_OK) {
      string why = SqliteErrMsg();
      ShowError("sqlite3_open() failed with <"+f+"> : "+why);
      SqliteClose();
      return false;
    }

    if (create)
      WriteLog("Sqlite DB <"+f+"> created");

    else {
      string cmd = "SELECT * FROM version;";
      if (!SqliteExec(cmd, true)) {
	ShowError("SqliteExec() failed with <"+f+">");
	SqliteClose();
	return false;
      }

      sqlite_mod_time = FileModTime(f);
      string mt = TimeString(sqlite_mod_time, false);
      
      WriteLog("Sqlite DB <"+ShortFileName(f)+"> "+ToStr(FileSize(f))
	       +" bytes stored ["+mt
	       +"] opened "+string(ronly?"read-only":"read-write"));
    }

    Tac("SqliteOpen"); // there are also 3x return false...

    return true;
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  bool DataBase::SqliteClose() {
    if (sql3) {
      int r = sqlite3_close(sql3);
      if (r!=SQLITE_OK) {
	string why = SqliteErrMsg();
	ShowError("SqliteClose() : sqlite3_close() failed with : "+why);
      }
      sql3 = NULL;
      string dbf = SqliteDBFile(Picsom()->TempSqliteDB());
      WriteLog("Closed sqlite3 access to <"+dbf+"> "+
	       ToStr(FileSize(dbf))+" bytes");
    }

    return true;
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  sqlite3_stmt *DataBase::SqlitePrepare(const string& cmd, bool warn) const {
    bool dotictac = true;

    if (!sql3) {
      ShowError("SqlitePrepare() : sql3==NULL");
      return NULL;
    }
      
    if (dotictac)
      Tic("SqlitePrepare");

    sqlite3_stmt *stmt = NULL;
    const char *tmp = NULL;
    // _v2
    int rrr = sqlite3_prepare(sql3, cmd.c_str(), cmd.size(), &stmt, &tmp);
    if (rrr!=SQLITE_OK || !stmt) {
      if (warn) {
	string why = SqliteErrMsg();
	ShowError("sqlite3_prepare() failed with ["+cmd+"] : "+why);
      }
      stmt = NULL;
    }

    if (dotictac)
      Tac("SqlitePrepare");

    return stmt;
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  bool DataBase::SqliteExec(const string& cmd, bool warn) const {
    bool dotictac = true;

    if (!sql3)
      return ShowError("SqliteExec() : sql3==NULL");
      
    if (dotictac)
      Tic("SqliteExec");

    char *errmsg = NULL;
    int rrr = sqlite3_exec(sql3, cmd.c_str(), NULL, NULL, &errmsg);

    if (dotictac)
      Tac("SqliteExec");

    if (rrr!=SQLITE_OK) {
      if (warn)
	ShowError("sqlite3_exec() failed with ["+cmd+"] : "+string(errmsg));
      return false;
    }

    return true;
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  string DataBase::SqliteErrMsg() const {
    if (!sql3) {
      ShowError("SqliteErrMsg() : sql3==NULL");
      return "";
    }
      
    return sqlite3_errmsg(sql3);
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SQLITE3_H
  void DataBase::SqliteShowRow(sqlite3_stmt *s) const {
    cout << Name() << " ";
    int c = sqlite3_data_count(s);
    for (int i=0; i<c; i++) {
      cout << sqlite3_column_name(s, i) << "=";
      int t = sqlite3_column_type(s, i);
      switch (t) {
      case SQLITE_INTEGER :
        cout << sqlite3_column_int(s, i) << " ";
        break;

      case SQLITE_FLOAT :
        cout << sqlite3_column_double(s, i) << " ";
        break;

      case SQLITE_TEXT :
        cout << "\"" << sqlite3_column_text(s, i) << "\" ";
        break;

      case SQLITE_BLOB :
        sqlite3_column_blob(s, i);
        cout << "[blob of " << sqlite3_column_bytes(s, i) << " bytes] ";
        break;

      case SQLITE_NULL :
      default :
        cout << "??? ";
      }
    }
    cout << endl;
  }
#endif // HAVE_SQLITE3_H

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MysqlThreadInit() {
#ifdef HAVE_MYSQL_H
    bool debug = false;
    if (mysql_library_initialized) {
      int r = mysql_thread_init();
      if (debug)
	cout << TimeStamp() << ThreadIdentifierUtil()
	     << " mysql_thread_init() returned " << r << endl;
      return r==0;
    }
#endif // HAVE_MYSQL_H
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MysqlThreadEnd() {
#ifdef HAVE_MYSQL_H
    bool debug = true;
    if (mysql_library_initialized) {
      mysql_thread_end();
      if (debug)
	cout << TimeStamp() << ThreadIdentifierUtil()
	     << " mysql_thread_end() called" << endl;
    }
#endif // HAVE_MYSQL_H
    return true;
  }
 
  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H

  MYSQL *DataBase::MysqlInit() {
    bool debug = false;
    bool ok = true;

    static RwLock lock;
    lock.LockWrite();
    if (!mysql_library_initialized) {
      ok = mysql_library_init(0, NULL, NULL)==0;
      mysql_library_initialized = true;
      if (debug)
	cout << TimeStamp() << ThreadIdentifierUtil()
	     << " mysql_library_init() "
	     << (ok?"executed successfully":"FAILED") << endl;

      if (mysql_thread_safe()==0)
	ShowError("mysql_thread_safe()==0 means "
		  "that MySQL is not thread safe...");
    }
    lock.UnlockWrite();

    if (!ok)
      return NULL;

    return mysql_init(NULL);
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> DataBase::MysqlUserPassword(const string&) {
    // sudo mysql
    // mysql> create user 'jorma'@'localhost' identified by 'Awe20fB';
    // mysql> grant all privileges on *.* to 'jorma'@'localhost';

    // sudo mysql
    // mysql> create database picsom_foo;
    // mysql> grant usage on *.* to 'jorma'@'localhost' identified by 'Awe20fB';
    // mysql> grant all privileges on  picsom_foo.* to 'jorma'@'localhost';

    return make_pair("jorma", "Awe20fB");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::MysqlDataBaseExists(const string& f, const string& s) {
    /// this is brutally copied from MysqlOpen() and should be corrected...

    string hdr = "DataBase::MysqlDataBaseExists("+f+","+s+") : ";
    
    string dbn = f;
    if (dbn.find("mysql:")==0)
      dbn.erase(0, 6);
    dbn = "picsom_"+dbn;
    size_t p = dbn.find('(');
    if (p!=string::npos)
      dbn.erase(p);

    if (DebugSql())
      cout << hdr << "testing existence of <" << dbn << "> in <"
	   << s << ">" << endl;

    MYSQL *mysqltmp = MysqlInit();
    if (!mysqltmp)
      return false;

    pair<string,string> up = MysqlUserPassword(s);
    const string& mysqlserver = s;

    if (!mysql_real_connect(mysqltmp, mysqlserver.c_str(), up.first.c_str(),
			    up.second.c_str(), NULL, 0, NULL, 0)) {
      mysql_close(mysqltmp);
      if (DebugSql())
	cout << hdr << "mysql_real_connect() A <"+MysqlErrMsg(mysqltmp)
	  +"> and return false" << endl;
      return false;
    }

    mysql_close(mysqltmp);
    mysqltmp = MysqlInit();

    if (!mysql_real_connect(mysqltmp, mysqlserver.c_str(), up.first.c_str(),
			    up.second.c_str(), dbn.c_str(), 0, NULL, 0)) {
      mysql_close(mysqltmp);
      if (DebugSql())
	cout << hdr << "mysql_real_connect() B <"+MysqlErrMsg(mysqltmp)
	  +"> and return false" << endl;
      return false;
    }

    mysql_close(mysqltmp);

    if (DebugSql())
      cout << hdr << "return true" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> DataBase::MysqlDataBaseList(PicSOM *ps) {
    string hdr = "DataBase::MysqlDataBaseList() : ";
    
    list<string> ret;

    if (DebugSql())
      cout << hdr << "finding all MySQL databases" << endl;

    MYSQL *mysqltmp = MysqlInit();
    if (!mysqltmp)
      return ret;

    string s = ps->SqlServer();
    pair<string,string> up = MysqlUserPassword(s);
    const string& mysqlserver = s;

    if (!mysql_real_connect(mysqltmp, mysqlserver.c_str(), up.first.c_str(),
			    up.second.c_str(), NULL, 0, NULL, 0)) {
      if (DebugSql())
	cout << hdr << "mysql_real_connect() A <"+MysqlErrMsg(mysqltmp)
	  +"> and return false" << endl;
      mysql_close(mysqltmp);
      return ret;
    }

    mysql_close(mysqltmp);
    mysqltmp = MysqlInit();

    string dbn = "INFORMATION_SCHEMA";
    if (!mysql_real_connect(mysqltmp, mysqlserver.c_str(), up.first.c_str(),
			    up.second.c_str(), dbn.c_str(), 0, NULL, 0)) {
      if (DebugSql())
	cout << hdr << "mysql_real_connect() B <"+MysqlErrMsg(mysqltmp)
	  +"> and return false" << endl;
      mysql_close(mysqltmp);
      return ret;
    }

    string cmd = "SELECT schema_name FROM INFORMATION_SCHEMA.SCHEMATA"
      " WHERE schema_name LIKE 'picsom_%';";
    if (DebugSql())
      cout << hdr << cmd << endl;

    MYSQL_STMT *stmt = mysql_stmt_init(mysqltmp);
    if (!stmt) {
      ShowError(hdr+"mysql_stmt_init() failed with ["+cmd+"]");
      return ret;
    }

    int rrr = mysql_stmt_prepare(stmt, cmd.c_str(), cmd.size());
    if (rrr) {
      ShowError("mysql_stmt_prepare() failed with ["+cmd+"]");
      return ret;
    }

    if (mysql_stmt_execute(stmt)) {
      ShowError(hdr+"mysql_stmt_execute() failed");
    }

    char dbname[1000] = "";
    unsigned long dbname_length = 0;
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = dbname;
    bind[0].buffer_length = sizeof(dbname);
    bind[0].length        = &dbname_length;

    if (mysql_stmt_bind_result(stmt, bind)) {
      ShowError(hdr+"mysql_stmt_bind_result() failed");
    }

    for (;;) {
      int rrr = mysql_stmt_fetch(stmt);
	
      if (rrr)
	break;

      string dn(dbname+7, dbname_length-7);
      ret.push_back(dn);
    }
    mysql_stmt_close(stmt);
    mysql_close(mysqltmp);

    return ret;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  bool DataBase::MysqlOpen(const string& f, bool create, bool /*write*/) {
    string hdr = "DataBase::MysqlOpen("+f+") : ";

    if (mysql)
      return ShowError(hdr+"already initialized");

    mysql = MysqlInit();
    if (!mysql)
      return ShowError(hdr+"MysqlInit() failed "+MysqlErrMsg(mysql));

    if (DebugSql())
      cout << hdr << "MySQL initalized" << endl;

    string dbn = f;
    if (dbn.find("mysql:")==0)
      dbn.erase(0, 6);
    dbn = "picsom_"+dbn;

    const string& mysqlserver = Picsom()->SqlServer();
    pair<string,string> up = MysqlUserPassword(mysqlserver);

    bool do_connect_b = true;
    if (create || dbn=="") {
      if (!mysql_real_connect(mysql, mysqlserver.c_str(), up.first.c_str(),
			      up.second.c_str(), NULL, 0, NULL, 0))
	return ShowError(hdr+"mysql_real_connect() A failed "+
			 MysqlErrMsg(mysql));

      if (DebugSql())
	cout << hdr << "MySQL real_connect()ed A" << endl;

      do_connect_b = false;
      if (create) {
	stringstream ss;
	ss << "CREATE DATABASE " << dbn.c_str() << ";";
	string ssstr = ss.str();

	if (DebugSql())
	  cout << hdr << ssstr << endl; 
      
	if (!MysqlQuery(ssstr, true))
	  return ShowError(hdr+"MysqlQuery(\"CREATE\") failed");

	WriteLog("Created MySQL db <"+dbn+">");

	MysqlClose();
	mysql = MysqlInit();
	do_connect_b = true;
      }
    }

    if (do_connect_b) {
      if (!mysql_real_connect(mysql, mysqlserver.c_str(),  up.first.c_str(),
			      up.second.c_str(), dbn.c_str(), 0, NULL, 0))
	return ShowError(hdr+"mysql_real_connect() B failed "+
			 MysqlErrMsg(mysql));

      if (DebugSql())
	cout << hdr << "MySQL real_connect()ed B" << endl;
    }

    string cmd = "USE "+dbn+";";

    if (DebugSql())
      cout << hdr << cmd << endl;

    if (!MysqlQuery(cmd, true))
      return ShowError(hdr+"MysqlQuery(\"USE\") failed");
   
    mysql_dbname = dbn;

//     if (!create && !FileExists(f)) {
//       WriteLog("sqlite3 database file <"+f+"> doesn't exist");
//       return false;
//     }
//     if (create && FileExists(f)) {
//       WriteLog("sqlite3 database file <"+f+"> already exists");
//       return false;
//     }

//     Tic("SqliteOpen");

//     bool ronly = false;

// #ifdef SQLITE_OPEN_READONLY
//     int flags = create ? SQLITE_OPEN_CREATE : 0;
//     flags |= SQLITE_OPEN_READWRITE;
//     if (!create && !write) {
//       flags = SQLITE_OPEN_READONLY;
//       ronly = true;
//     }
//     int rrr = sqlite3_open_v2(f.c_str(), &sql3, flags, NULL);
// #else
//     int a_w = ::access(f.c_str(), W_OK); 
//     if (a_w) {
//       if (write)
// 	return ShowError("sqlite3 db <"+f+"> has no write permissions");
//       else
// 	ronly = true;
//     }
//     int rrr = sqlite3_open(f.c_str(), &sql3);
// #endif // SQLITE_OPEN_READONLY

//     if (rrr!=SQLITE_OK) {
//       string why = SqliteErrMsg();
//       ShowError("sqlite3_open() failed with <"+f+"> : "+why);
//       SqliteClose();
//       return false;
//     }

//     if (create)
//       WriteLog("<"+ShortFileName(f)+"> created");

//     else {
//       string cmd = "SELECT * FROM version;";
//       if (!SqliteExec(cmd, true)) {
// 	ShowError("SqliteExec() failed with <"+f+">");
// 	SqliteClose();
// 	return false;
//       }

//       WriteLog("<"+ShortFileName(f)+"> opened "+
// 	       string(ronly?"read-only":"read-write"));
//     }

//     Tac("SqliteOpen"); // there are also 3x return false...

    return true;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  bool DataBase::MysqlClose(bool show) {
    if (mysql) {
      mysql_close(mysql);
      mysql = NULL;
      if (show)
	WriteLog("Closed mysql access");
    }
    return true;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  MYSQL_STMT *DataBase::MysqlPrepare(const string& cmd, bool warn) const {
    bool dotictac = true;

    if (!mysql) {
      ShowError("MysqlPrepare() : mysql==NULL");
      return NULL;
    }
    
    if (dotictac)
      Tic("MysqlPrepare");

    MysqlLock();

    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
      if (warn) {
	string why = MysqlErrMsg(mysql);
	ShowError("mysql_stmt_init() failed with ["+cmd+"] : "+why);
      }

    } else {
      int rrr = mysql_stmt_prepare(stmt, cmd.c_str(), cmd.size());
      if (rrr) {
	if (warn) {
	  string why = MysqlErrMsg(mysql);
	  ShowError("mysql_stmt_prepare() failed with ["+cmd+"] : "+why);
	}
	MysqlStmtClose(stmt);
	stmt = NULL;
      }
    }

    if (dotictac)
      Tac("MysqlPrepare");

    return stmt;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  bool DataBase::MysqlExecute(MYSQL_STMT *stmt, bool warn) const {
    if (!stmt) {
      ShowError("MysqlExecute() : stmt==NULL");
      return false;
    }

    int r = mysql_stmt_execute(stmt);
    
    if (warn && r) 
      ShowError("mysql_stmt_execute() failed: "+MysqlErrMsg(mysql));

    return r==0;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  bool DataBase::MysqlStmtClose(MYSQL_STMT *stmt) const {
    bool ok = mysql_stmt_close(stmt)==0;
    MysqlUnlock();

    return ok;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  bool DataBase::MysqlQuery(const string& cmdin, bool warn,
			    MYSQL *mysqlp, const DataBase *db) {
    bool dotictac = true;

    if (!mysqlp)
      return ShowError("MysqlQuery() : mysql==NULL");

    if (dotictac && db)
      db->Tic("MysqlQuery");

    if (db)
      db->MysqlLock();

    string tmp = cmdin, skip = "; \t\n";
    bool ret = true;

    for (;;) {
      size_t q = tmp.find(';');
      if (q==string::npos)
	q = tmp.size()-1;
      string cmd = tmp.substr(0, q+1);
      if (mysql_query(mysqlp, cmd.c_str())) {
	if (warn)
	  ShowError("mysql_query() failed with ["+cmd+"] : "+
		    MysqlErrMsg(mysqlp));
	ret = false;
	break;
      }
      tmp.erase(0, q+1);
      while (skip.find(tmp[0])!=string::npos)
	tmp.erase(0, 1);
      if (tmp=="")
	break;
    }

    if (ret) {
      MYSQL_RES *res = mysql_store_result(mysqlp);
      if (res)
	mysql_free_result(res);
    }

    if (db)
      db->MysqlUnlock();

    if (dotictac && db)
      db->Tac("MysqlQuery");

    return ret;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  string DataBase::MysqlErrMsg(MYSQL *mysqlp) {
    if (!mysqlp)
      return ShowErrorS("MysqlErrMsg() : mysql==NULL");

    string m = "\""+string(mysql_error(mysqlp))+"\" ("
      +ToStr(mysql_errno(mysqlp))+")";
      
    return m;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  void DataBase::MysqlShowRow(MYSQL_STMT*) const {
    if (!mysql) {
      ShowError("MysqlShowRow() : mysql==NULL");
    }
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  string DataBase::MysqlFindTableColumn(const string& t, const string& c) {
    if (!mysql)
      return ShowErrorS("MysqlFindTableColumn() : mysql==NULL");
    
    string tx = t;
    int n = 0;
    for (;;) {
      string cmd = "SELECT "+c+" FROM "+tx+";";
      if (MysqlQuery(cmd, false))
	return tx;
      cmd = "SELECT indexz FROM "+tx+";";
      if (!MysqlQuery(cmd, false))
	break;
      tx = t+ToStr(n++);
    }

    return "";
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_MYSQL_H
  string DataBase::MysqlAddTableColumn(const string& t, const string& c,
				       const string& f) {
    if (!mysql)
      return ShowErrorS("MysqlAddTableColumn() : mysql==NULL");
    
    // cmd = "SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS WHERE "
    // 	  "table_schema='"+mysql_dbname+"' AND table_name='"+tname+"';";

    string cmd = "ALTER TABLE "+t+" ADD COLUMN "+c+" "+f+";";
    if (!SqlExec(cmd, true))
      return ShowErrorS("MysqlAddTableColumn() : failed with <"+cmd+">");

    return t;
  }
#endif // HAVE_MYSQL_H

  /////////////////////////////////////////////////////////////////////////////

  const script_exp_t& DataBase::SetupScript(const string& s) {
    string hdr = "DataBase::SetupScript("+s+") : ";
    map<string,script_exp_t>::const_iterator i = setup.find(s);
    if (i==setup.end()) {
      script_exp_t script, src;
      string f = ExpandPath("setups", s);
      if (!FileExists(f))
        ShowError(hdr+"file <"+f+"> does not exist");
      else {
        Analysis a(Picsom(), NULL, NULL, vector<string>());
        script = a.ScriptExpandFile(f, src);
        script.insert(script.begin(), make_pair("setupname", s));
      }
      setup[s] = script;
      i = setup.find(s);
    }

    return i->second;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::TempDir(const string& d, bool m) {
    string msg = "DataBase::TempDir("+d+") : ";

    bool debug = PicSOM::DebugTempFiles();

    int mode = 02775;

    string dir, mkdir;

    if (tempdirs.empty()) {
      string n = Name();
      size_t p = n.find("://");
      if (p!=string::npos)
	n.replace(p, 3, "___");
      mkdir = dir = Picsom()->TempDirPersonal()+"/"+n;
      tempdirs.push_back(dir);
    }

    dir = *tempdirs.begin();

    if (d!="") {
      dir += "/"+d;
      if (!DirectoryExists(dir))
	mkdir = dir;
    }

    if (mkdir!="") {
      if (!Picsom()->MkDirHier(mkdir, mode))
	return ShowErrorS(msg+"MkDirHier("+mkdir+") failed");

      if (debug)
	cout << TimeStamp() << msg << "created temp dir <" << mkdir << ">"
	     << endl;

      if (m)
	tempdirs.push_back(mkdir);
    }

    return dir;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::TempFile(const string& f, bool m) {
    string msg = "DataBase::TempFile("+f+") : ";

    bool debug = PicSOM::DebugTempFiles();

    string p = TempDir("", true);

    size_t x = f.rfind('/');
    if (x!=string::npos) {
      int mode = 02775;

      string d = p+"/"+f.substr(0, x);

      if (!Picsom()->MkDirHier(d, mode))
	return ShowErrorS(msg+"MkDirHier("+d+") failed");
     
      if (debug)
	cout << TimeStamp() << msg << "created temp dir <" << d << ">" << endl;

      if (m)
	tempdirs.push_back(d);
    }

    p += "/"+f;

    if (debug)
      cout << TimeStamp() << msg << "returning <" << p << ">" << endl;

    if (m)
      tempfiles.push_back(p);

    return p;
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::UnlinkTempFiles() {
    string msg = "DataBase::UnlinkTempFiles() : ";

    bool debug = PicSOM::DebugTempFiles();

    for (list<string>::const_iterator i=tempfiles.begin();
	 i!=tempfiles.end(); i++) {
      if (debug)
	cout << TimeStamp() << msg << "unlinking <" << *i << ">" << endl;
      Unlink(*i);
    }

    tempfiles.clear();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CreateWordnetOntology(const vector<pair<string,float> >& h,
				       const string& n) {
    const bool debug = true;
    const string msg = "DataBase::CreateWordnetOntology() : ";

    ontology& o = Ontology(n);

    map<string,string> pps;
    for (auto i=h.begin(); i!=h.end(); i++) {
      vector<string> pp = ontology::wordnet_path(i->first);
      if (false && debug)
	cout << CommaJoin(pp) << endl;
      // pps[i->first] = CommaJoin(pp);
      for (size_t j=0; j<pp.size(); j++) {
	vector<string> t;
	for (size_t k=0; k<=j; k++)
	  t.push_back(pp[k]);
	pps[pp[j]] = CommaJoin(t);
      }

      auto p = pp.begin(), q = p;
      if (q!=pp.end()) {
	o.add_is_a(*q, "NOUN-ROOT");
	q++;
      }
      for (size_t l = 1; q!=pp.end(); p++, q++, l++) {
	ontology::entry_t& e = o.add_is_a(*q, *p);
	e.level = l;
	if (l==pp.size()-1)
	  e.own = i->second;
      }
    }
    
    size_t max_l = 0;
    for (auto i=o.IsA().begin(); i!=o.IsA().end(); i++)
      if (i->level>max_l)
	max_l = i->level;

    if (debug)
      cout << "max_l=" << max_l << endl;
    for (size_t l=max_l;; l--) {
      if (debug)
	cout << "LEVEL " << l << endl;
      for (auto i=o.IsA().begin(); i!=o.IsA().end(); i++)
	if (i->level==l) {
	  if (debug)
	    cout << "WNO" << (i->own!=0?"x":" ")
		 << " " << i->own+i->children << " = "
		 << i->own << " + " << i->children << " " << i->first 
		 << " " << l << " : " << pps[i->first] << endl;
	  if (i->second.find("ROOT")==string::npos) {
	    ontology::entry_t& r = o.add_is_a(i->second, "*");
	    r.children += i->children + i->own;
	  }
	}
      if (l==0)
	break;
    }
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::PopulateLscomMap(const PicSOM *ps, const string& mapfile) {
    const bool debug = false;
    const string msg = "DataBase::PopulateLscomMap() : ";
    const string lscommap = "/lscom/"+mapfile+".map";
    string lscom_map_filename = ps->RootDir("share")+lscommap;
    if (!FileExists(lscom_map_filename))
      lscom_map_filename = ps->RootDir("share", true)+lscommap;

    if (!FileExists(lscom_map_filename) && ps->NoRootDir())
      ps->DownloadFile(lscom_map_filename);

    ifstream fp(lscom_map_filename.c_str());
    if (!fp)
      return ShowError(msg, "could not open file: "+lscom_map_filename);

    string pref = mapfile+"/";
    vector<string> header;
    size_t linecount = 0;
    bool first = true;

    while (true) {
      string line;
      getline(fp, line);
      if (!fp)
	break;

      linecount++;

      if (line[0] == '#')
        continue;

      vector<string> parts = SplitInSpaces(line);

      if (first) {
        first = false;
        header = parts;
        //insert(header.begin(), parts.begin(), parts.end());

        for (size_t i=0; i<header.size(); i++)
          lscom_map[pref+header[i]] = list<pair<string,string> > ();

        continue;
      }

      if (parts.size() != header.size())
        return ShowError(msg, "Columns on row "+ToStr(linecount)+" mismatch! "+
                         ToStr(parts.size())+" != "+ToStr(header.size()));

      for (size_t i=1; i<parts.size(); i++) {
        lscom_map[pref+header[i]].push_back(make_pair(parts[0], parts[i]));
        if (debug)
          cout << TimeStamp() << msg << "Inserted (" << parts[0]
	       << ", " << parts[i] << ")" << endl;
      }
    }

    ps->WriteLog("Read "+ToStr(linecount)+" lines from "+lscom_map_filename);

    const string relationsmap = "/lscom/tv11.sin.relations.txt";
    string relations_map_filename = ps->RootDir("share")+relationsmap;
    if (!FileExists(relations_map_filename))
      relations_map_filename = ps->RootDir("share", true)+relationsmap;

    if (!FileExists(relations_map_filename) && ps->NoRootDir())
      ps->DownloadFile(relations_map_filename);

    ifstream fpr(relations_map_filename.c_str());
    if (!fpr)
      return ShowError(msg, "could not open file: "+relations_map_filename);

    size_t nimp = 0, nexl = 0;
    for (;;) {
      string line;
      getline(fpr, line);
      if (!fpr)
	break;

      size_t p = line.find('\r');
      if (p!=string::npos)
	line.erase(p);

      vector<string> parts = SplitInSpaces(line);
      if (parts.size()!=3)
	return ShowError(msg, "failed to split line ["+line+"]");

      if (parts[1]=="implies") {
	lscom_ontology.add_is_a(parts[0], parts[2]);
	nimp++;

      } else if (parts[1]=="excludes") {
	lscom_ontology.add_excludes(parts[0], parts[2]);
	nexl++;

      } else
	return ShowError(msg, "relation unknown in ["+line+"]");
    }
    
    ps->WriteLog("Read "+ToStr(nimp)+" implications and "+ToStr(nexl)
		 +" exclusions from "+relations_map_filename);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& DataBase::LscomName(const PicSOM* ps, const string& con,
				    bool dir, const string& mapin) {
    static const string error = "";
    const string msg = "DataBase::LscomName("+con+","+(dir?"true":"false")+","
      +mapin+") : ";

    string cmap = mapin!="" ? mapin : "lscom/trecvid2011";
    string mapfile = cmap;
    size_t p = mapfile.find('/');
    if (p!=string::npos)
      mapfile.erase(p);
    else
      mapfile = "lscom";

    static pthread_rwlock_t lscom_rwlock = PTHREAD_RWLOCK_INITIALIZER;
    pthread_rwlock_wrlock(&lscom_rwlock);
    if (lscom_map.empty())
      PopulateLscomMap(ps, mapfile);
    pthread_rwlock_unlock(&lscom_rwlock);

    lscom_map_t::const_iterator map_it = lscom_map.find(cmap);
    if (map_it == lscom_map.end()) {
      ShowError(msg, "Requested database not found in lscom map.");
      return error;
    }

    static map<string,string> pairlscomnum {
      { "lscom366+lscom184",  "|911" },
      { "lscom172+lscom183",  "|912" },
      { "lscom306+lscom233",  "|913" },
      { "lscom233+lscom260",  "|914" },
      { "lscom2535+lscom156", "|915" },
      { "lscom347+lscom227",  "|916" },
      { "lscom3153+lscom313", "|917" },
      { "lscom307+lscom201",  "|918" },
      { "lscom203+lscom028",  "|919" },
      { "lscom219+lscom306",  "|920" }
    };

    if (dir && con.find('+')!=string::npos) {
      auto p = pairlscomnum.find(con);
      if (p==pairlscomnum.end()) {
	ShowError(msg, "<"+con+"> not found in the pair map");
	return error;
      }
      return p->second;
    }

    string lcon = con;
    // As a courtesy we accept lscom id:s both as 003 and as lscom003.
    if (dir && lcon.find("lscom") == 0)
      lcon.erase(0, 5);
    int lcon_num = atoi(lcon.c_str());

    vector<const string*> ret;
    const lscom_list_t& l = map_it->second;
    for (lscom_list_t::const_iterator it = l.begin(); it != l.end(); it++) {
      if (dir && it->first == lcon)
        ret.push_back(&it->second);

      if (!dir) {
        pair<string,string> parts = SplitLscomName(it->second);
	int tv_num   = atoi(parts.second.c_str());

        if (lcon == parts.first || lcon == parts.second ||
	    (lcon_num>1000 && lcon_num==1000+tv_num))
          ret.push_back(&it->first);
      }
    }

    if (ret.empty()) {
      ShowError(msg, "Requested LSCOM name not found.");
      return error;
    }

    if (ret.size()!=1) {
      vector<string> xret;
      for (auto i=ret.begin(); i!=ret.end(); i++)
	xret.push_back(**i);
      ShowError(msg, "Multiple matching LSCOM names found: "+
		JoinWithString(xret, " "));
      return error;
    }

    return *ret[0];
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> DataBase::SplitLscomName(const string& s) {
    vector<string> a = SplitInSomething("|", false, s);
    return a.size() ? make_pair(a[0], a.size()>1?a[1]:"")
      : pair<string,string>();
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::LscomNameToTrecvidId(const string& cname, bool add1000) {
    string lscom_str = LscomName(cname, true, "lscom/trecvid2011");
    pair<string,string> lscom_pair = SplitLscomName(lscom_str);
    string fnum = lscom_pair.second;

    if (!add1000)
      return fnum;

    string flab = ToStr(1000+atoi(fnum.c_str()));

    return flab;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::PopulateImageNetMap(const PicSOM *ps) {
    const bool debug = false;
    const string msg = "DataBase::PopulateImageNetMap(): ";

    imagenet_map.clear();

    const string imagenetmap = "/imagenet/imagenet.map";
    string imagenet_map_filename = ps->RootDir("share")+imagenetmap;
    if (!FileExists(imagenet_map_filename))
      imagenet_map_filename = ps->RootDir("share", true)+imagenetmap;

    ifstream fp(imagenet_map_filename.c_str());
    if (!fp)
      return ShowError(msg, "could not open file: "+imagenet_map_filename);

    size_t linecount = 0;

    while (true) {
      string line;
      getline(fp, line);
      if (!fp)
	break;

      linecount++;

      if (line[0] == '#')
        continue;

      vector<string> parts = SplitInSomething(";", false, line);
      if (parts.size()!=3)
	return ShowError(msg, "split failed for <"+line+">");

      imagenet_map[parts[1]] =  parts[2];

      if (debug)
	cout << TimeStamp() << msg << "Inserted (" << parts[1] << ", " << parts[2] << ")"
	     << endl;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& DataBase::ImageNetName(const PicSOM* ps,
				       const string& con, bool dir) {
    static const string error = "";
    const string msg = "DataBase::ImageNetName(" + con + ", " +
      (dir?"true":"false") + ") : ";
    
    if (imagenet_map.empty())
      PopulateImageNetMap(ps);

    if (dir) {
      auto i = imagenet_map.find(con);
      if (i==imagenet_map.end()) {
	ShowError(msg, "Requested IMAGENET name not found.");
	return error;
      }
      return  i->second;
    }

    for (auto it=imagenet_map.begin(); it!=imagenet_map.end(); it++) {
      vector<string> parts = SplitImageNetName(it->second);
      if (con==parts[0])
	return it->first;
    }

    ShowError(msg, "Requested IMAGENET name not found.");

    return error;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<string> DataBase::SplitImageNetName(const string& s) {
    vector<string> a = SplitInSomething(",", false, s);
    return a;
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::JoinAsyncAnalysesDeprecated() {
#ifdef PICSOM_USE_PTHREADS
    if (analysis_async_deprecated) {
      list<Analysis::analyse_result> result;
      if (!analysis_async_deprecated->AnalysePthreadJoin(result, true))
	ShowError("AnalysePthreadJoin() failed in DataBase destructor");
      delete analysis_async_deprecated;
    }
    analysis_async_deprecated = NULL;
#endif // PICSOM_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

  int DataBase::FrameNumber(const string& l) {
    auto p = l.find(':');
    if (p==string::npos)
      return -1;
    string s = l.substr(p+1);

    p = s.find('_');
    if (p!=string::npos)
      s.erase(p);

    return atoi(s.c_str());      
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<size_t,size_t> DataBase::FrameNumberRange(size_t idx, const
						 ground_truth *gt) const {
    const object_info *oi = FindObject(idx);

    if (!gt) {
      int a = FrameNumber(oi->children.front());
      int b = FrameNumber(oi->children.back());
      return (a>=0 && b>=a) ? make_pair(a, b+1) : make_pair(0,0);
    }
    
    int a = -1, b = -1;
    for (auto i=oi->children.begin(); i<oi->children.end(); i++) {
      if (a==-1 && (*gt)[*i]==1)
	a = *i;
      if (a!=-1 && b==-1 && (*gt)[*i]==-1)
	b = *i;
    }
    if (b==-1 && a!=-1)
      b = oi->children.back()+1;

    if (a==-1 && b==-1)
      return make_pair(0, 0);

    int fa = FrameNumber(a), fb = FrameNumber(b);

    return make_pair(fa, fb);
  }

  /////////////////////////////////////////////////////////////////////////////

  list<video_frange> 
  DataBase::SolveVideoFranges(const ground_truth& gt) const {
    list<video_frange> ret;

    set<size_t> pset;
    for (size_t i=0; i<Size(); i++)
      if (gt[i]==1) {
	int p = ParentObject(i, false);
	if (p>=0)
	  pset.insert((size_t)p);
      }

    for (auto i=pset.begin(); i!=pset.end(); i++) {
      video_frange fr = FrameNumberFrange(*i, &gt);
      ret.push_back(fr);
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  video_frange_match
  DataBase::DTWmatch(const string& gts, const string& a, const string& b,
		     const dtw_spec& dtw) {
    const string msg = "DataBase::DTWmatch() : ";

    size_t debug = 1;
    bool do_visualise = false;

    if (dtw.dtwparam.find('v')!=string::npos) {
      do_visualise=true;
      cout << "visualising" << endl;
    }

    size_t nshow = debug ? 10 : 0;

    // maito,109500:35-54,109501:57-64

    if (debug)
      cout << TimeStamp() << msg << " dtwparam=" << dtw.dtwparam << endl;

    dtw_spec::cache_t cache, *cachep = &cache;
    //ClearDataVectorByLabelCache();

    string osstr;

    ground_truth gt = GroundTruthExpression(gts);
    
    string aa = a, bb = b, ars, brs;
    size_t p = aa.find(':');
    if (p!=string::npos) {
      ars = aa.substr(p+1);
      aa.erase(p);
    }
    p = bb.find(':');
    if (p!=string::npos) {
      brs = bb.substr(p+1);
      bb.erase(p);
    }

    size_t ida = LabelIndex(aa), idb = LabelIndex(bb);

    video_frange arr, brr;
    if (ars=="")
      arr = FrameNumberFrange(ida, &gt);
    else
      arr = FrameNumberFrange(ida, ars);
    if (brs=="")
      brr = FrameNumberFrange(idb, &gt);
    else
      brr = FrameNumberFrange(idb, brs);

    if (debug)
      cout << TimeStamp() << msg << gts << " " << arr.dashstr() << " "
	   << brr.dashstr() << endl;

    size_t z = FrameNumber(FindObject(idb)->children.back())+1;

    size_t l = arr.nframes();

    size_t gtlen = brr.nframes(); // for visualisation

    multimap<double,video_frange_match> res;

    float maxf = numeric_limits<float>::max()/10;
    float mindist = maxf, maxdist=0;

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
    cv::Mat visval, gtmask;
    if (do_visualise) {
      visval=cv::Mat(z+1,z+1, cv::DataType<float>::type,cv::Scalar::all(0));
      gtmask=cv::Mat(z+1,z+1, CV_8UC1,cv::Scalar::all(0));
    }
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

    for (size_t x=0; x<z; x++)
      for (size_t y=x+1; y<=z; y++) {

	video_frange zrr(this, idb, x, y);
	video_frange ints = zrr.get_intersection(brr);
	video_frange unio = zrr.get_union(brr);
	double m = double(ints.nframes())/unio.nframes();

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
	if (do_visualise && m>=0.5)
	  gtmask.at<uchar>(y,x)=255;
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

	size_t minlen=fmax(5,0.4*l);
	size_t maxlen=1.3*l;

// 	if (y-x>l)  // obs! should it be like y-x>1.2*l ???
// 	  continue;
	
// 	if (y-x<0.5*l)  // obs! hard-coded
// 	  continue;

 	if (y-x>maxlen)
 	  continue;
	
 	if (y-x<minlen) 
 	  continue;

	double lenweight=fabs((log(y-x)-log(0.5*l))/log(4))+2;
	// double lenweight=1;
	lenweight /= sqrt(y-x);

	// cout << "l=" << l << " x="<<x<<" y="<<y<<" w="<<lenweight<<endl;
	
	double d = DTWdistance(arr, zrr, dtw, cachep)*lenweight;

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
	if (do_visualise) {
	  mindist=fmin(mindist,d);
	  maxdist=fmax(maxdist,d);
	  visval.at<float>(y,x)=d;
	}
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

	video_frange_match match(arr, zrr);
	match.set_gt_b(brr);
	match.set_value("dist", d);
	string c = match.str();

	match.set_value("m", m);
	double f1 = ints.nframes()/sqrt(zrr.nframes()*brr.nframes());
	match.set_value("f1", f1);

	if (debug>1)
	  cout << TimeStamp() << msg << gts
	       << " " << c << " = " << d << " m=" << m << " f1=" << f1 << endl;

	res.insert(make_pair(d, match));
      }

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
    if (do_visualise) {
      cv::Mat vismat(z,z,CV_8UC3,cv::Scalar::all(50));

      for (size_t x=0; x<z; x++)
	for (size_t y=0; y<z; y++) {
	  if (gtmask.at<uchar>(y,x))
	    vismat.at<cv::Vec3b>(y,x)[0]=200;
	}

      if (maxdist-mindist>0) {
	for (size_t x=0; x<z; x++)
	  for (size_t y=x+1; y<z; y++) {
	    float v = (visval.at<float>(y,x)-mindist)/(maxdist-mindist);

	    if (y-x+1==gtlen) {
	      vismat.at<cv::Vec3b>(y,x)[2]=100;
	    }

	    if (v<0)
	      continue;

	    if (v<=0.5) {
	      vismat.at<cv::Vec3b>(y,x)[1]=255*2*(0.5-v);
	    } else {
	      vismat.at<cv::Vec3b>(y,x)[2]=255*2*(v-0.5);
	    }
	  }
      }
      
      vismat.at<cv::Vec3b>(res.begin()->second.b.end,
			   res.begin()->second.b.begin)=cv::Vec3b(255,255,255);

      string fn("dtwmat.");
      fn += dtw.str();
      fn += "_";
      fn += brr.dashstr().substr(0,6);
      fn += ".png";

      boost::replace_all(fn, ",", "_");
      boost::replace_all(fn, "/", "_P_");

      cout << "forming image " << fn << endl;

      vector<int> writeparam;

      writeparam.push_back(CV_IMWRITE_PNG_COMPRESSION);
      writeparam.push_back(0);

      cv::imwrite(fn,vismat,writeparam);	      
    }
#else
    do_visualise = do_visualise&&(gtlen+mindist>maxdist);
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

    size_t n = 0;
    for (auto i=res.begin(); i!=res.end() && n<nshow; i++, n++)
      cout << n << " " << osstr << " " << i->first << " : "
	   << i->second.str() << endl;

    return res.begin()->second;
  }

  /////////////////////////////////////////////////////////////////////////////

  video_frange_match
  DataBase::DTWmatch(const string& s, const string& a, const string& b,
		     const set<size_t>& comp, const map<size_t,float>& w,
		     bool sephands, double mul) {
    const string msg = "DataBase::DTWmatch() : ";

    size_t debug = 0;

    size_t nshow = debug ? 10 : 0;

    // maito,109500:35-54,109501:57-64

    ostringstream os;
    os << "mul=" << mul << " sephands=" << sephands << " comp=";
    for (auto j=comp.begin(); j!=comp.end(); j++)
      os << (j==comp.begin()?"":",") << *j;
    string osstr = os.str();

    if (debug)
      cout << TimeStamp() << msg << endl;

    ground_truth gt = GroundTruthExpression(s);
    
    string aa = a, bb = b, ars, brs;
    size_t p = aa.find(':');
    if (p!=string::npos) {
      ars = aa.substr(p+1);
      aa.erase(p);
    }
    p = bb.find(':');
    if (p!=string::npos) {
      brs = bb.substr(p+1);
      bb.erase(p);
    }

    size_t ida = LabelIndex(aa), idb = LabelIndex(bb);

    video_frange arr, brr;
    if (ars=="")
      arr = FrameNumberFrange(ida, &gt);
    else
      arr = FrameNumberFrange(ida, ars);
    if (brs=="")
      brr = FrameNumberFrange(idb, &gt);
    else
      brr = FrameNumberFrange(idb, brs);

    if (debug)
      cout << TimeStamp() << msg << s << " " << arr.dashstr() << " "
	   << brr.dashstr() << endl;

    size_t z = FrameNumber(FindObject(idb)->children.back())+1;

    size_t l = arr.nframes();

    multimap<double,video_frange_match> res;
    for (size_t x=0; x<z; x++)
      for (size_t y=x+1; y<=z; y++) {
	if (y-x>l)  // obs! should it be like y-x>1.2*l ???
	  continue;
	
	if (y-x<0.5*l)  // obs! hard-coded
	  continue;
	
	video_frange zrr(this, idb, x, y);
	double d = DTWdistance(arr, zrr, comp, w, sephands, mul);
	video_frange_match match(arr, zrr);
	match.set_gt_b(brr);
	match.set_value("dist", d);
	string c = match.str();

	video_frange ints = zrr.get_intersection(brr);
	video_frange unio = zrr.get_union(brr);
	double m = double(ints.nframes())/unio.nframes();
	match.set_value("m", m);
	double f1 = ints.nframes()/sqrt(zrr.nframes()*brr.nframes());
	match.set_value("f1", f1);

	if (debug>1)
	  cout << TimeStamp() << msg << s
	       << " " << c << " = " << d << " m=" << m << " f1=" << f1 << endl;

	res.insert(make_pair(d, match));
      }

    size_t n = 0;
    for (auto i=res.begin(); i!=res.end() && n<nshow; i++, n++)
      cout << n << " " << osstr << " " << i->first << " : "
	   << i->second.str() << endl;

    return res.begin()->second;
  }

  /////////////////////////////////////////////////////////////////////////////

  double DataBase::DTWdistance(const video_frange& fr1,
			       const video_frange& fr2,
			       const dtw_spec& s,
			       dtw_spec::cache_t *cachep) {
    size_t idx1 = fr1.idx, a1 = fr1.begin, b1 = fr1.end;
    size_t idx2 = fr2.idx, a2 = fr2.begin, b2 = fr2.end;

    const string msg = "DataBase::DTWdistance(" +ToStr(idx1)+","+
      ToStr(a1)+","+ToStr(b1)+","+ToStr(idx2)+","+ToStr(a2)+","+ToStr(b2)+
      ") : ";

    bool debug = false;

    if (debug)
      cout << TimeStamp() << msg << endl;

    float sum = 0;
    for (auto i=s.s.begin(); i!=s.s.end(); i++) {
      float d = DTWdistance(fr1, fr2, *i, s.wind, cachep);
      sum += d;
    }

    return sum;
  }

  /////////////////////////////////////////////////////////////////////////////

  double DataBase::DTWdistance(const video_frange& fr1,
			       const video_frange& fr2,
			       const dtw_spec::flist_t& s,
			       const pair<vector<float>,vector<float> >& w,
			       dtw_spec::cache_t *cachep) {
    size_t idx1 = fr1.idx, a1 = fr1.begin, b1 = fr1.end;
    size_t idx2 = fr2.idx, a2 = fr2.begin, b2 = fr2.end;

    const string msg = "DataBase::DTWdistance(" +ToStr(idx1)+","+
      ToStr(a1)+","+ToStr(b1)+","+ToStr(idx2)+","+ToStr(a2)+","+ToStr(b2)+
      ") : ";

    bool debug = false;

    vector<bool> relative; 
    vector<float> floatarg(s.size(),0);

    double maxf = numeric_limits<double>::max()/10;

    if (debug)
      cout << TimeStamp() << msg << endl;

    vector<FloatVectorSet*> fset;
    int compnr=0;
    for (auto f=s.begin(); f!=s.end(); f++) {
      Index *idx = FindIndex(f->name, "", true);
      VectorIndex *vidx = dynamic_cast<VectorIndex*>(idx);
      if (!vidx) {
	ShowError(msg+"dynamic_cast() failed");
	return -maxf;
      }
      vidx->ReadDataFile(false, false);
      fset.push_back(&vidx->Data());

      if(f->dist.find("rel")!=string::npos){
	relative.push_back(true);
	cachep=NULL; // disable caching if even one of the distance measures
	// uses translation invariance
      }
      else
	relative.push_back(false);

      size_t b=f->dist.find("(");
      if(b!=string::npos){
	float val;
	sscanf(f->dist.substr(b+1).c_str(),"%f",&val);
	floatarg[compnr]=val;
      }

      compnr++;

    }

    string cachekey = dtw_spec::str(s);
    map<size_t,map<size_t,float> > *cachemap = NULL;
    if (cachep)
      cachemap = &(*cachep)[cachekey];

    size_t n1 = b1-a1, n2 = b2-a2;

    vector<FloatVector> ave1(relative.size()),ave2(relative.size());

    size_t fi = 0;
    for (auto f=s.begin(); f!=s.end(); f++, fi++) {
      if(relative[fi]){
	
	for (size_t c=0; c<n2; c++) {
	  string lc = Label(idx2)+":"+ToStr(a2+c);
	  const FloatVector& fc = GetDataVectorByLabelCached(fset[fi], lc);
	  if(c==0) ave2[fi]=fc;
	  else
	    ave2[fi] += fc;
	}

	for (size_t r=0; r<n1; r++) {
	  string lr = Label(idx1)+":"+ToStr(a1+r);
	  const FloatVector& fr = GetDataVectorByLabelCached(fset[fi], lr);
	  if(r==0)
	    ave1[fi]=fr;
	  else
	    ave1[fi]+=fr;
	}

	ave1[fi] /= n1+1;
	ave2[fi] /= n2+1;
      }
    }

    simple::FloatMatrix m(n1, n2);
    for (size_t c=0; c<n2; c++) {
      string lc = Label(idx2)+":"+ToStr(a2+c);

      map<size_t,float> *colcache = NULL;
      if (cachemap)
	colcache = &(*cachemap)[a2+c];

      for (size_t r=0; r<n1; r++) {
	string lr = Label(idx1)+":"+ToStr(a1+r);

	double d = 0;
	bool found = false;
	if (colcache && colcache->find(a1+r)!=colcache->end()) {
	  d = (*colcache)[a1+r];
	  found = true;
	  if (debug)
	    cout << "cache: (" << r << "," << c << ") = ("
		 << a1+r << "," << a2+c << ")"
		 << " " << lr << " " << lc
		 << " " << d
		 << endl;
	}

	if (!found) {
	  size_t fi = 0;
	  for (auto f=s.begin(); f!=s.end(); f++, fi++) {
	    const FloatVector& fc = GetDataVectorByLabelCached(fset[fi], lc);
	    const FloatVector& fr = GetDataVectorByLabelCached(fset[fi], lr);

	    FloatVector ffr=fr;

	    if(relative[fi]){
	      ffr -= ave1[fi];
	      ffr += ave2[fi];
	    }

	    double dx = 0;
	    if (f->dist.find("euc")==0)
	      dx = DTWdistance_euc(fc, ffr, f->comp);
	    else if (f->dist.find("chisqr")==0)
	      dx = DTWdistance_chisqr(fc, ffr, f->comp);
	    else if (f->dist.find("hamming")==0)
	      dx = DTWdistance_hamming(fc, ffr, floatarg[fi]);
	    else if (f->dist.find("emd")==0)
	      dx = DTWdistance_emd(fc, ffr, f->comp, f->dist.substr(3));
	    else {
	      ShowError(msg+"distance type \""+f->dist+"\" unknown");
	      return -maxf;
	    }

	    if (debug)
	      cout << "[" << f->name << "][" << f->dist << "]"
		   << "(" << r << "," << c << ") = ("
		   << a1+r << "," << a2+c << ")"
		   << " " << lr << " " << lc
		   << " f->wght=" << f->wght
		   << " " << dx
		   << endl;
	  
	    d += dx*f->wght;
	  }
	}

	if (colcache && !found)
	  (*colcache)[a1+r] = d;

	float wr = 1, wc = 1;
	size_t ra = n1-1-r, ca = n2-1-c;
	if (r<ra)
	  ra = r;
	if (c<ca)
	  ca = c;
	if (ra<w.first.size())
	  wr = w.first[ra];
	if (ca<w.second.size())
	  wc = w.second[ca];

	if (debug && (wr!=1 || wc!=1))
	  cout << "r=" << r << " c=" << c << " wr=" << wr << " wc=" << wc
	       << endl;

	d *= wr*wc;
	
	double min = c==0 && r==0 ? 0 : maxf;
	if (r && m(r-1, c)<min)
	  min = m(r-1, c);
	if (c && m(r, c-1)<min)
	  min = m(r, c-1);
	if (r && c && m(r-1, c-1)<min)
	  min = m(r-1, c-1);
	m.Set(r, c, min+d);

	if (debug)
	  cout << "m(" << a1+r << "," << a2+c << ")=" << m(r, c)
	       << endl;
      }
    }

    return m(n1-1, n2-1);
  }

  /////////////////////////////////////////////////////////////////////////////

  double DataBase::DTWdistance_euc(const FloatVector& fc,
				   const FloatVector& fr,
				   const map<size_t,float>& comp) const {
    double dx = 0;

    if (comp.size())
      for (auto i=comp.begin(); i!=comp.end(); i++) {
	double dd = fc[i->first]-fr[i->first];
	dx += dd*dd*i->second;
      }
    else
      for (int i=0; i<fc.Length(); i++) {
	double dd = fc[i]-fr[i];
	dx += dd*dd;
      }

    dx = sqrt(dx);

    return dx;
  }

  /////////////////////////////////////////////////////////////////////////////

  double DataBase::DTWdistance_chisqr(const FloatVector& fc,
				   const FloatVector& fr,
				   const map<size_t,float>& comp) const {
    double dx = 0;

    if (comp.size())
      for (auto i=comp.begin(); i!=comp.end(); i++) {
	double dd = fc[i->first]-fr[i->first];
	double ds = fc[i->first]+fr[i->first];
	if(ds>0)
	  dx += dd*dd*i->second/ds;
      }
    else
      for (int i=0; i<fc.Length(); i++) {
	double dd = fc[i]-fr[i];
	double ds = fc[i]+fr[i];
	if(ds>0)
	  dx += dd*dd/ds;
      }

    return dx;
  }

 /////////////////////////////////////////////////////////////////////////////

  double DataBase::DTWdistance_hamming(const FloatVector& fc,
				   const FloatVector& fr,
				   const float threshold) const {
    double dx = 0;
    for (int i=0; i<fc.Length(); i++) {
      bool a=fc[i]>threshold;
      bool b=fr[i]>threshold;

      if(a!=b) dx++;
    }

    return dx;
  }


  /////////////////////////////////////////////////////////////////////////////

  double DataBase::DTWdistance_emd(const FloatVector& fc,
				   const FloatVector& fr,
				   const map<size_t,float>& comp,
				   const string& spec) const {
    const string msg = "DataBase::DTWdistance_emd() : ";

    int h = 0, w = 0;
    if (sscanf(spec.c_str(), "%dx%d", &h, &w)!=2) {
      ShowError(msg+"spec \""+spec+"\" is not %dx%d");
      return 0;
    }

    int nc = comp.size() ? comp.size() : fc.Length();

    if (h*w!=nc) {
      ShowError(msg+"dimensionality mismatch \""+spec+"\" vs "+ToStr(nc));
      return 0;
    }
    
#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
    cv::Mat sig1(nc, 3, CV_32FC1), sig2(nc, 3, CV_32FC1);
    double fcsum = 0, frsum = 0;
    int vi = 0;
    auto ci = comp.begin();
    for (int wi=0; wi<w; wi++)
      for (int hi=0; hi<h; hi++) {
	float fcval = 0, frval = 0;
	if (!comp.size()) {
	  fcval = fc[vi];
	  frval = fr[vi];
	} else {
	  fcval = fc[ci->first]*ci->second;
	  frval = fr[ci->first]*ci->second;
	  ci++;
	}
	fcsum += fcval;
	frsum += frval;
	sig1.at<float>(vi, 0) = fcval;
	sig2.at<float>(vi, 0) = frval;
	sig1.at<float>(vi, 1) = sig2.at<float>(vi, 1) = wi;
	sig1.at<float>(vi, 2) = sig2.at<float>(vi, 2) = hi;
	vi++;
      }

    for (int i=0; i<nc; i++) {
      if (fcsum)
	sig1.at<float>(i, 0) /= fcsum;
      else
	sig1.at<float>(i, 0) = 1.0/nc;
      if (frsum)
	sig2.at<float>(i, 0) /= frsum;
      else
	sig2.at<float>(i, 0) = 1.0/nc;
    }

    float d = cv::EMD(sig1, sig2, CV_DIST_L2);

    return d;
#else
    return 0*fr.Length();
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV
  }

  /////////////////////////////////////////////////////////////////////////////

  double DataBase::DTWdistance(const video_frange& fr1,
			       const video_frange& fr2,
			       const set<size_t>& slmotion_comp,
			       const map<size_t,float>& sh_weight,
			       bool sephands, double mul) {
    size_t idx1 = fr1.idx, a1 = fr1.begin, b1 = fr1.end;
    size_t idx2 = fr2.idx, a2 = fr2.begin, b2 = fr2.end;

    const string msg = "DataBase::DTWdistance(" +ToStr(idx1)+","+
      ToStr(a1)+","+ToStr(b1)+","+ToStr(idx2)+","+ToStr(a2)+","+ToStr(b2)+
      ") : ";

    bool debug = true;

    // 109500,35,55,109501,57,65

    if (debug)
      cout << TimeStamp() << msg << endl;

    if (sephands) {
      set<size_t> comp1, comp2;
      for (auto cc = slmotion_comp.begin(); cc!=slmotion_comp.end(); cc++)
	(comp1.size()<slmotion_comp.size()/2?comp1:comp2).insert(*cc);
      video_frange r1(this, idx1, a1, b1);
      video_frange r2(this, idx2, a2, b2);
      double d1 = DTWdistance(r1, r2, comp1, sh_weight, false, mul);
      double d2 = DTWdistance(r1, r2, comp2, sh_weight, false, mul);
      return d1+d2;
    }

    Index *slmotionp = FindIndex("slmotion", "", true);
    VectorIndex *slmotionv = dynamic_cast<VectorIndex*>(slmotionp);
    if (!slmotionv)
      return ShowError(msg+"dynamic_cast() failed");
    slmotionv->ReadDataFile(false, false);
    FloatVectorSet& slmotion = slmotionv->Data();

    Index *contourfdp = FindIndex("contourfd", "", true);
    VectorIndex *contourfdv = dynamic_cast<VectorIndex*>(contourfdp);
    if (!contourfdv)
      return ShowError(msg+"dynamic_cast() failed");
    contourfdv->ReadDataFile(false, false);
    FloatVectorSet& contourfd = contourfdv->Data();

    Index *shp = FindIndex("sh", "", true);
    VectorIndex *shv = dynamic_cast<VectorIndex*>(shp);
    if (!shv)
      return ShowError(msg+"dynamic_cast() failed");
    shv->ReadDataFile(false, false);
    FloatVectorSet& sh = shv->Data();

    string seg = "_lh";
    if (slmotion_comp.size()==2 && slmotion_comp.find(61)!=slmotion_comp.end())
      seg = "_rh";

    int n1 = b1-a1, n2 = b2-a2;

    double maxf = numeric_limits<double>::max()/10;

    simple::FloatMatrix m(n1, n2);
    for (int c=0; c<n2; c++) {
      // const FloatVector& slmotionc =
      //   *slmotion.GetByLabel(Label(idx2)+":"+ToStr(a2+c));
      const FloatVector& slmotionc =
	GetDataVectorByLabelCached(&slmotion, Label(idx2)+":"+ToStr(a2+c));

      const FloatVector& shc =
	GetDataVectorByLabelCached(&sh, Label(idx2)+":"+ToStr(a2+c));

      FloatVector contourfdc;
      if (mul)
	contourfdc = InterpolatedVector(contourfd, idx2, a2+c, seg);

      for (int r=0; r<n1; r++) {
	// const FloatVector& slmotionr =
	//   *slmotion.GetByLabel(Label(idx1)+":"+ToStr(a1+r));
	const FloatVector& slmotionr =
	  GetDataVectorByLabelCached(&slmotion, Label(idx1)+":"+ToStr(a1+r));

	const FloatVector& shr =
	  GetDataVectorByLabelCached(&sh, Label(idx1)+":"+ToStr(a1+r));

	FloatVector contourfdr;
	if (mul)
	  contourfdr = InterpolatedVector(contourfd, idx1, a1+c, seg);

	double dslmotion = 0.0, dsh = 0.0, dcontourfd = 0.0;
	for (auto cc=slmotion_comp.begin(); cc!=slmotion_comp.end(); cc++) {
	  double dd = slmotionc[*cc]-slmotionr[*cc];
	  dslmotion += dd*dd;
	}
	for (auto cc=sh_weight.begin(); cc!=sh_weight.end(); cc++) {
	  double dd = shc[cc->first]-shr[cc->first];
	  dsh += dd*dd*cc->second;
	}
	if (mul)
	  for (int cc=0; cc<contourfdc.Length(); cc++) {
	    double dd = contourfdc[cc]-contourfdr[cc];
	    dcontourfd += dd*dd;
	  }
	dslmotion  = sqrt(dslmotion);
	dsh        = sqrt(dsh);
	dcontourfd = sqrt(dcontourfd)*mul;
	double d   = dslmotion+dsh+dcontourfd;
	
	double min = c==0 && r==0 ? 0 : maxf;
	if (r && m(r-1, c)<min)
	  min = m(r-1, c);
	if (c && m(r, c-1)<min)
	  min = m(r, c-1);
	if (r && c && m(r-1, c-1)<min)
	  min = m(r-1, c-1);
	m.Set(r, c, min+d);

	if (debug)
	  cout << "m(" << a1+r << "," << a2+c << ")=" << m(r, c)
	       << " dslmotion=" << dslmotion
	       << " dsh=" << dsh
	       << " dcontourfd=" << dcontourfd
	       << endl;
      }
    }

    return m(n1-1, n2-1);
  }

  /////////////////////////////////////////////////////////////////////////////

  const FloatVector& 
  DataBase::GetDataVectorByLabelCached(const FloatVectorSet* set,
				       const string& lab) {
    static const FloatVector dummy;

    auto i = data_vector_cache[set].find(lab);
    if (i==data_vector_cache[set].end()) {
      const FloatVector *v = set->GetByLabel(lab);
      data_vector_cache[set][lab] = v;
    }

    return *data_vector_cache[set][lab];
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVector DataBase::InterpolatedVector(const FloatVectorSet& data,
					   int idx, int fr,
					   const string& seg) {
    const string msg = "DataBase::InterpolateVector() : ";

    bool debug = false;

    if (debug)
      cout << TimeStamp() << msg << data.FileName() << endl;

    string key = data.FileName();
    string lab = "seg:"+Label(idx)+":", tolab = lab+ToStr(fr)+seg;

    auto w = interpolated_vectors[key].find(tolab);
    if (w!=interpolated_vectors[key].end())
      return w->second;

    FloatVector *v = data.GetByLabel(lab+ToStr(fr)+seg);
    if (v) {
      interpolated_vectors[key][tolab] = *v;
      return *v;
    }

    const object_info *oi = FindObject(idx);
    int last = -1;
    int lastidx = oi && oi->children.size() ? oi->children.back() : -1;
    if (lastidx!=-1) {
      const string& ll = Label(lastidx);
      size_t p = ll.find(':');
      if (p!=string::npos)
	last = atoi(ll.substr(p+1).c_str());
    }

    FloatVector *va = NULL, *vb = NULL;
    int a = fr-1, b = fr+1;

    for (; !va && a>=0; a--) {
      v = data.GetByLabel(lab+ToStr(a)+seg);
      if (v)
	va = v;
    }

    for (; !vb && b<=last; b++) {
      v = data.GetByLabel(lab+ToStr(b)+seg);
      if (v)
	vb = v;
    }

    if (!va && !vb) {
      FloatVector r(data.VectorLength());
      interpolated_vectors[key][tolab] = r;
      return r;
    }

    if (va && !vb) {
      interpolated_vectors[key][tolab] = *va;
      return *va;
    }

    if (!va && vb) {
      interpolated_vectors[key][tolab] = *vb;
      return *vb;
    }

    FloatVector r = (*va*(b-fr)+*vb*(fr-a))/(b-a);
    interpolated_vectors[key][tolab] = r;
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SetFeatureAlias(const xmlNodePtr &n) {
    return SetFeatureAlias(GetProperty(n, "name"), GetProperty(n, "value"), 
			   GetProperty(n, "text"));
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SetFeatureAugmentation(const xmlNodePtr &n) {
    return SetFeatureAugmentation(GetProperty(n, "name"),
				  GetProperty(n, "options"), 
				  GetProperty(n, "text"));
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SetGroundTruth(const xmlNodePtr &n) {
    return SetGroundTruth(GetProperty(n, "name"), GetProperty(n, "value"), 
			  GetProperty(n, "text"));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::SetNamedCmdLine(const xmlNodePtr &n) {
    return SetNamedCmdLine(GetProperty(n, "name"), GetProperty(n, "value"), 
			   GetProperty(n, "text"));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::OpenReadWrite(const string& m) {
    if (m=="rw")
      return ShowError("-rw=XXX and dbopenmode=XXX should be XXX=sql/fea/det");

    if (m.find("sql")!=string::npos)
      OpenReadWriteSql(true);

    if (m.find("fea")!=string::npos)
      OpenReadWriteFea(true);

    if (m.find("det")!=string::npos)
      OpenReadWriteDet(true);

    if (m.find("txt")!=string::npos)
      OpenReadWriteTxt(true);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
  ///
  bool DataBase::CreateLevelDB(const string& /*dir*/, const string& /*fea*/,
			       const ground_truth& /*gt*/) {

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  ///
  list<vector<float> > DataBase::RunCaffe(const DataBase *db,
					  const string& name,
					  const vector<size_t>& ivec) {
    string msg = "DataBase::RunCaffe(vector<size_t>) : ";
    list<string> flist;
    for (auto i=ivec.begin(); i!=ivec.end(); i++)
      flist.push_back(ObjectPathEvenExtract(*i));

    return RunCaffe(db, name, flist);
  }

  /////////////////////////////////////////////////////////////////////////////

  ///
  list<vector<float> > DataBase::RunCaffe(const DataBase *db,
					  const string& name,
					  const list<string>& flist) {
    string msg = "DataBase::RunCaffe(list<string>) : ";
    list<imagedata> ilist;
    for (auto i=flist.begin(); i!=flist.end(); i++)
      ilist.push_back(imagefile(*i).frame(0));

    return RunCaffe(db, name, ilist);
  }

  /////////////////////////////////////////////////////////////////////////////

  ///
  list<vector<float> > DataBase::RunCaffe(const DataBase *db,
					  const string& name,
					  const list<imagedata>& ilist) {
    return RunCaffeOld(db, name, ilist);
  }

  /////////////////////////////////////////////////////////////////////////////

  ///
  list<vector<float> > DataBase::RunCaffeFeature(const string& /*name*/,
						 const list<imagedata>&
						 /*ilist*/) {
    string msg = "DataBase::RunCaffeFeature(list<imagedata>) : ";
    ShowError(msg+"not implemented yet");
    return list<vector<float> >();
  }

  /////////////////////////////////////////////////////////////////////////////

  ///
  list<vector<float> > DataBase::RunCaffeOld(const DataBase *db,
					     const string& name,
					     const list<imagedata>& ilist) {
    string msg = "DataBase::RunCaffe(list<imagedata>) : ";

    using caffe::Net;
    using caffe::Blob;
    using caffe::Caffe;

    Tic("RunCaffe()");

    bool debug3 = debug_detections>2;
    bool debug4 = debug_detections>3;
    bool mean_is_rgb = false;

    int gpudevice = Picsom()->GpuDeviceId();
    bool use_gpu = gpudevice>=0;
    if (use_gpu) {
      Caffe::SetDevice(gpudevice);
      Caffe::set_mode(Caffe::GPU);
    } else
      Caffe::set_mode(Caffe::CPU);

    // caffe/python/caffe/pycaffe.cpp

    string namewdb = name;
    if (db!=this)
      namewdb += "@"+db->Name();

    auto cmi = caffemap.find(namewdb);
    if (cmi==caffemap.end()) {
      Tic("RunCaffe() construct");

      string prototxt = db->ExpandPath("caffe/models")+"/"+name+"/prototxt";
      string model    = db->ExpandPath("caffe/models")+"/"+name+"/model";
      string mean     = db->ExpandPath("caffe/models")+"/"+name+"/mean.png";
      string map      = db->ExpandPath("caffe/models")+"/"+name+"/idxmap.txt";
      
      db->RootlessDownloadFile(prototxt, true);
      db->RootlessDownloadFile(model,    true);
      db->RootlessDownloadFile(mean,     true);
      db->RootlessDownloadFile(map,      false);

      string logtxt = "Loading caffe ["+ShortFileName(prototxt)+"] and [model] "
	"and [mean.png]";

      vector<size_t> caffe2dbmap;
      if (FileExists(map)) {
	logtxt += " and [idxmap.txt]";
	string ss = FileToString(map);
	vector<string> in = SplitInSomething("\n", false, ss);
	for (auto i=in.begin(); i!=in.end(); i++)
	  if ((*i)[0]>='0' && (*i)[0]<='9')
	    caffe2dbmap.push_back(atoi(i->c_str()));
      }

      string fusion = caffefusion;
      if (fusion=="")
	fusion = "amean";

      logtxt += " with fusion=\""+fusion+"\"";

      WriteLog(logtxt);

      if (debug3)
	FLAGS_logtostderr = 1;

#ifdef HAS_CAFFE_COLORCONVERT
      Caffe::set_phase(Caffe::TEST);
      Net<float> *netp = new Net<float>(prototxt);
#else
      Net<float> *netp = new Net<float>(prototxt, caffe::TEST);
#endif

      imagedata mimg = imagefile(mean).frame(0);
      caffe_t caffe(namewdb, prototxt, model, mean,
      		    netp, mimg, caffe2dbmap, fusion);
      caffemap[namewdb] = caffe;
      cmi = caffemap.find(namewdb);
      Net<float>& cnet = *cmi->second.net;
      cnet.CopyTrainedLayersFrom(model);

      Tac("RunCaffe() construct");
    }
    Net<float>& cnet = *cmi->second.net;

    list<vector<float> > emptyres, res;

    for (auto ilp = ilist.begin(); ilp!=ilist.end(); ) {
      const vector<Blob<float>*>& input_blobs = cnet.input_blobs();
      if (debug4)
	cout << "input_blobs.size()=" << input_blobs.size() << endl;
      if (input_blobs.size()!=1) {
	ShowError(msg+"input_blobs.size()!=1");
	return emptyres;
      }

      size_t ss = 227, ds = 256-ss;
      vector<float> dataten(10*3*ss*ss);
      if (input_blobs[0]->count()%dataten.size()) {
	ShowError(msg+"input_blobs[0]->count() mod dataten.size() != 0 : "+
		  ToStr(input_blobs[0]->count())+" vs "+
		  ToStr(dataten.size()));
	return emptyres;
      }

      size_t n_ten_blocks = input_blobs[0]->count()/dataten.size();

      if (debug4)
	cout << " input_blobs[0]->count()=" << input_blobs[0]->count()
	     << " input_blobs[0]->num()=" << input_blobs[0]->num()
	     << " input_blobs[0]->channels()=" << input_blobs[0]->channels()
	     << " input_blobs[0]->width()=" << input_blobs[0]->width()
	     << " input_blobs[0]->height()=" << input_blobs[0]->height()
	     << " n_ten_blocks=" << n_ten_blocks
	     << endl;
      
      vector<float> dataall(n_ten_blocks*dataten.size());

      for (size_t iii=0; iii<n_ten_blocks && ilp!=ilist.end(); iii++, ilp++) {

	// center square...
	// size_t iw = ilp->width(), ih = ilp->height();
	// size_t wh = iw<ih ? iw : ih, x = 0, y = 0;
	// if (iw>ih)
	// 	x = (iw-ih)/2;
	// else
	// 	y = (ih-iw)/2;
	// imagedata img = ilp->subimage(x, y, x+wh-1, y+wh-1);
	// scalinginfo si(wh, wh, 256, 256);

	Tic("RunCaffe() imagedata");

	// aspect ratio loss...
	imagedata img = *ilp;
	scalinginfo si(img.width(), img.height(), 256, 256);

	img.rescale(si, 1);

	float *ip = (float*)img.raw_float(256*256*3);
	const float *mp = cmi->second.mimg.raw_float(256*256*3);
	if (mean_is_rgb)
	  for (size_t z=0; z<256*256*3; z++)
	    ip[z] = 255*(ip[z]-mp[z]);
	else // gbr
	  for (size_t z=0; z<256*256*3; z+=3) {
	    ip[z  ] = 255*(ip[z  ]-mp[z+2]); // R
	    ip[z+1] = 255*(ip[z+1]-mp[z+1]); // G
	    ip[z+2] = 255*(ip[z+2]-mp[z  ]); // B
	  }

	float *p = &dataten[0];
	for (size_t j=0; j<5; j++) {
	  size_t dx = (j==0||j==2) ? 0 : ds;
	  size_t dy = (j==0||j==1) ? 0 : ds;
	  if (j==4)
	    dx = dy = ds/2;
	  if (debug4)
	    cout << dx << " " << dy << " " << dx+ss-1 << " " << dy+ss-1 << endl;
	  imagedata cut = img.subimage(dx, dy, dx+ss-1, dy+ss-1);

	  float *q = p+3*ss*ss;
	  const float *cutp = cut.raw_float(ss*ss*3);
	  for (size_t y=0, pxy=0; y<ss; y++) {
	    size_t qxy = (y+1)*ss-1;
	    for (size_t x=0; x<ss; x++, pxy++, qxy--) {
	      int cuti =  3*(cut.width()*y+x); // cut.to_index_w_count(x, y);
	      p[pxy]         = q[qxy]         = cutp[cuti+2]; // B
	      p[pxy+ss*ss]   = q[qxy+ss*ss]   = cutp[cuti+1]; // G
	      p[pxy+ss*ss*2] = q[qxy+ss*ss*2] = cutp[cuti  ]; // R
	    }
	  }
	  p += 2*3*ss*ss;
	}
	Tac("RunCaffe() imagedata");

	memcpy(&dataall[iii*dataten.size()], &dataten[0], dataten.size()*sizeof(float));
      }

      const float *arr = &dataall[0];

      /*
	cout << "top left B first rows left" << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[3*227+q];
	cout << endl;

	cout << "top left G first rows left" << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+2*227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+3*227+q];
	cout << endl;

	cout << "top left R last row right" << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+2*227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+3*227+q];
	cout << endl;
      */

      Tic("RunCaffe() outcopy");

      switch (Caffe::mode()) {
      case Caffe::CPU:
	memcpy(input_blobs[0]->mutable_cpu_data(), arr,
	       sizeof(float) * input_blobs[0]->count());
	break;
      case Caffe::GPU:
	cudaMemcpy(input_blobs[0]->mutable_gpu_data(), arr,
		   sizeof(float) * input_blobs[0]->count(),
		   cudaMemcpyHostToDevice);
	break;
      default:
	LOG(FATAL) << "Unknown Caffe mode.";
      }  // switch (Caffe::mode())

      Tac("RunCaffe() outcopy");

      Tic("RunCaffe() forward");
      //LOG(INFO) << "Start";
      const vector<Blob<float>*>& output_blobs = cnet.ForwardPrefilled();
      //LOG(INFO) << "End";
      Tac("RunCaffe() forward");

      if (output_blobs.size()!=1) {
	ShowError(msg+"output_blobs.size()!=1");
	return emptyres;
      }

      if (output_blobs[0]->num()!=10*(int)n_ten_blocks) {
	ShowError(msg+"output_blobs[0]->num()!=10*n_ten_blocks");
	return emptyres;
      }

      if (debug4)
	cout << " output_blobs[0]->count()=" << output_blobs[0]->count()
	     << " output_blobs[0]->num()=" << output_blobs[0]->num()
	     << " output_blobs[0]->channels()=" << output_blobs[0]->channels()
	     << " output_blobs[0]->width()=" << output_blobs[0]->width()
	     << " output_blobs[0]->height()=" << output_blobs[0]->height()
	     << endl;

      float *outarr = new float[output_blobs[0]->count()];

      Tic("RunCaffe() incopy");

      switch (Caffe::mode()) {
      case Caffe::CPU:
	memcpy(outarr, output_blobs[0]->cpu_data(),
	       sizeof(float) * output_blobs[0]->count());
	break;
      case Caffe::GPU:
	cudaMemcpy(outarr, output_blobs[0]->gpu_data(),
		   sizeof(float) * output_blobs[0]->count(),
		   cudaMemcpyDeviceToHost);
	break;
      default:
	LOG(FATAL) << "Unknown Caffe mode.";
      }  // switch (Caffe::mode())
      
      Tac("RunCaffe() incopy");

      Tic("RunCaffe() postprocess");

      for (size_t jj=0; jj<n_ten_blocks && res.size()<ilist.size(); jj++) {
	const string& fusion = cmi->second.fusion;
	vector<float> resv(output_blobs[0]->channels());
	for (int ii=0; ii<output_blobs[0]->channels(); ii++) {
	  if (debug4)
	    cout << "i=" << ii;
	  float sum = 0.0, max = 0.0;
	  for (int j=0; j<output_blobs[0]->num(); j++) {
	    size_t ij = jj*1000*10 + j*1000+ii;
	    if (debug4)
	      cout << " " << outarr[ij];
	    sum += outarr[ij];
	    if (outarr[ij]>max)
	      max = outarr[ij];
	  }
	  sum /= output_blobs[0]->num();
	  if (debug4)
	    cout << "  " << sum << endl;
	  resv[ii] = fusion=="amean" ? sum : fusion=="max" ? max : 0.0;
	}
	
	if (cmi->second.map.size()!=0) {
	  if (cmi->second.map.size()!=resv.size()) {
	    ShowError(msg+"dimensionality error in indexmap A");
	    // return;
	  }
	  
	  vector<float> tmp(output_blobs[0]->channels());
	  for (size_t ti=0; ti<resv.size(); ti++) {
	    size_t dst = cmi->second.map[ti];
	    if (dst>=tmp.size()) {
	      ShowError(msg+"dimensionality error in indexmap B");
	      // return;
	    }
	    tmp[dst] = resv[ti];
	  }
	  resv = tmp;
	}

	if (debug4) {
	  const string output_prefix = "proto";
	  const vector<string>& blob_names = cnet.blob_names();
	  const vector<boost::shared_ptr<Blob<float> > >& blobs = cnet.blobs();
	  for (int blobid = 0; blobid < (int)cnet.blobs().size(); ++blobid) {
	    // Serialize blob
	    cout << "Dumping " << blob_names[blobid] << " "
		 << blobs[blobid]->channels() << "," << blobs[blobid]->num() 
		 << endl;
	    caffe::BlobProto output_blob_proto;
	    blobs[blobid]->ToProto(&output_blob_proto);
	    WriteProtoToBinaryFile(output_blob_proto, 
				   output_prefix + "-binary-" + blob_names[blobid]);
	    //WriteProtoToTextFile(output_blob_proto, 
	    //			 output_prefix + "-text-" + blob_names[blobid]);
	  }
	}

	res.push_back(resv);
      }

      Tac("RunCaffe() postprocess");

      delete outarr;
    }

    Tac("RunCaffe()");

    return res;
  }
#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE

  /////////////////////////////////////////////////////////////////////////////

  list<vector<string> > DataBase::LinkedDataQuery(const string& t,
						  const string& sin,
						  const vector<string>& a) {
    string msg = "DataBase::LinkedDataQuery("+t+","+sin+","+ToStr(a)+") : ";
    if (Query::DebugInfo())
      WriteLog(msg+"starting");

    string s = sin;

    list<vector<string> > ret;
    
    if (t=="sparql") {
      string k = t+"/"+s;
      if (Query::DebugInfo())
	WriteLog(msg+"sparql k=\""+k+"\"");
      auto l = linked_data_queries.find(k);
      if (l==linked_data_queries.end()) {
	string ss = ExpandPath("sparql")+"/"+s+".rq";
	linked_data_queries[k] = linked_data_query_t(t, k, ss);
	l = linked_data_queries.find(k);
      }

      const linked_data_query_t& q = l->second;

      if (Query::DebugInfo()) {
	WriteLog(msg+q.str());
	WriteLog(msg+"before q.expand(a) : ["+JoinWithString(a, "][")+"]");
      }
      string query = q.expand(a);
      if (Query::DebugInfo())
	WriteLog(msg+"after  q.expand(a) : ["+JoinWithString(a, "][")+"]");

      Connection c(Picsom());
      ret = c.SparqlQuery(q.url, query, true);

      return ret;
    }

    if (t=="rdf") {
      string k = t+"/"+s;
      auto l = linked_data_queries.find(k);
      if (l==linked_data_queries.end()) {
	linked_data_queries[k] = linked_data_query_t(t, k, s);
	l = linked_data_queries.find(k);
      }

      const linked_data_query_t& q = l->second;

      Connection c(Picsom());
      ret = c.RdfQuery(q.url);

      return ret;
    }

    ShowError(msg+"only RDF and SPARQL queries supported");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  DataBase::linked_data_query_t::linked_data_query_t(const string& t,
						     const string& k,
						     const string& s) :
    type(t), key(k), queryfile(s),  nargs(0) {

    string msg = "linked_data_query_t("+t+","+k+","+s+") : ";
    if (Query::DebugInfo())
      cout << msg+"starting" << endl;
    
    if (type=="rdf")
      url = s;

    if (type=="sparql") {
      query = FileToString(queryfile);

      size_t p = query.find("# ENDPOINT ");
      if (p!=string::npos) {
	url = query.substr(p+11);
	p = url.find_first_of(" \t\n\r");
	if (p!=string::npos)
	  url.erase(p);

	 if (Query::DebugInfo())
	   cout << msg+"url=["+url+"]" << endl;
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::linked_data_query_t::expand(const vector<string>& a) const {
    string s = query;

    for (size_t p=0; p<s.size();) {
      size_t x = s.find("<:", p);
      if (x==string::npos)
	break;

      size_t y = s.find_first_not_of("0123456789", x+2);
      if (y==string::npos)
	break;

      if (s.substr(y, 2)!=":>")
	break;

      size_t i = atoi(s.substr(x+2, y-x-2).c_str());
      if (i>=a.size())
	break;

      s.replace(x, y-x+2, a[i]);

      p = x+a[i].size();
    }

    return s;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  vector<pair<size_t,double> > 
  DataBase::TrecvidSIN(const vector<size_t>& idxs,
		       const vector<pair<string,double> >& dw,
		       size_t nres, bool tolerate_missing, bool to_parent) {
    string msg = "DataBase::TrecvidSIN() : ";
    
    // size_t nres = 2000;
    // bool tolerate_missing = true;

    vector<pair<bin_data*,double> > dpw;
    for (auto i=dw.begin(); i!=dw.end(); i++)
      if (i->second!=0.0) {
	bool exists = false;
	auto l = BinRetrieveDetectionData(0, i->first, false, exists);
	if (!exists) {
	  ShowError(msg+"bin detection <"+i->first+"> not found");
	  continue;
	}
	dpw.push_back(make_pair(&bindetections[i->first], i->second));
      }

    if (idxs.size()==0) {
      ShowError(msg+"empty index set");
      return vector<pair<size_t,double> >();
    }

    target_type tt = ObjectsTargetType(idxs[0]);
      
    if (PicSOM::TargetTypeFileFullMasked(tt)==target_videosegment)
      return TrecvidSINtopDown(idxs, dpw, nres, tolerate_missing);

    if (PicSOM::TargetTypeFileFullMasked(tt)==target_image)
      return TrecvidSINbottomUp(idxs, dpw, nres, tolerate_missing, to_parent);

    ShowError(msg+"target type <"+TargetTypeString(tt)+
	      "> is neither videosegment nor image");
    return vector<pair<size_t,double> >();
  }

  /////////////////////////////////////////////////////////////////////////////
  
  vector<pair<size_t,double> >
  DataBase::TrecvidSINtopDown(const vector<size_t>& idxs,
			      const vector<pair<bin_data*,double> >& dpw,
			      size_t nres, bool tolerate_missing) {
    string msg = "DataBase::TrecvidSINtopDown() : ";
    
    multimap<double,size_t> res;
    for (size_t i=0; i<idxs.size(); i++) {
      size_t idx = idxs[i];

      target_type tt = ObjectsTargetType(idx);
      if (PicSOM::TargetTypeFileFullMasked(tt)!=target_videosegment) {
	stringstream ss;
	FindObject(idx)->dump_nonl(ss);
	ShowError(msg+ss.str()+" wrong target type");
	continue;
      }

      double max = 0;
      object_info *oi = FindObject(idx);
      for (size_t j=0; j<oi->children.size(); j++) {
	size_t cidx = oi->children[j];
	
	double sum = 0, wsum = 0;
	for (auto d=dpw.begin(); d!=dpw.end(); d++) {
	  vector<float> v = d->first->get_float(cidx);
	  if (v.size()!=1) {
	    if (tolerate_missing)
	      continue;

	    stringstream ss;
	    FindObject(cidx)->dump_nonl(ss);
	    ShowError(msg+ss.str()+" dimensionality error "+ToStr(v.size())+
                      " <"+d->first->filename()+"> "+d->first->str());
	    continue;
	  }
	  sum  += d->second * v[0];
	  wsum += d->second;
	}
	if (wsum)
	  sum /= wsum;

	if (sum>max)
	  max = sum;
      }

      res.insert(make_pair(max, idx));
    }
    
    vector<pair<size_t,double> > ret;
    size_t n = 0;
    for (auto i=res.rbegin(); i!=res.rend() && n<nres; i++, n++)
      ret.push_back(make_pair(i->second, i->first));

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  vector<pair<size_t,double> >
  DataBase::TrecvidSINbottomUp(const vector<size_t>& idxs,
			       const vector<pair<bin_data*,double> >& dpw,
			       size_t nres, bool tolerate_missing,
			       bool to_parent) {
    string msg = "DataBase::TrecvidSINbottomUp() : ";
    
    bool use_max = false;

    bool debug = false;

    map<size_t,double> val;
    for (size_t i=0; i<idxs.size(); i++) {
      size_t idxo = idxs[i];

      target_type tt = ObjectsTargetType(idxo);
      if (PicSOM::TargetTypeFileFullMasked(tt)!=target_image) {
	stringstream ss;
	FindObject(idxo)->dump_nonl(ss);
	ShowError(msg+ss.str()+" wrong target type");
	continue;
      }

      size_t dest = idxo;
      if (to_parent) {
	int p = ParentObject(idxo);
	if (p<0) {
	  ShowError(msg+"no parent");
	  return vector<pair<size_t,double> >();
	}
	dest = p;
      }
      
      double sum = 0, wsum = 0, max = 0;
      for (auto d=dpw.begin(); d!=dpw.end(); d++) {
	vector<float> v = d->first->get_float(idxo);
	if (v.size()!=1) {
	  if (tolerate_missing)
	    continue;
	  
	  stringstream ss;
	  FindObject(idxo)->dump_nonl(ss);
	  ShowError(msg+ss.str()+" dimensionality error "+ToStr(v.size())+
		    " <"+d->first->filename()+"> "+d->first->str());
	  continue;
	}
	sum  += d->second * v[0];
	wsum += d->second;
	if (v[0]>max)
	  max = v[0];

	if (debug)
	  cout << "image : #" << dest << " #" << idxo << " " << v[0] << " * "
	       << d->second << " = " << d->second * v[0] << endl;
      }
      if (wsum)
	sum /= wsum;

      if (val.find(dest)==val.end()) {
	if (debug)
	  cout << "shot : #" << dest << " #" << idxo << " avg = " << sum
	       << " max = " << max << endl;
	val[dest] = use_max ? max : sum;

      } else if ((use_max ? max : sum)>val[dest]) {
	if (debug)
	  cout << "shot : #" << dest << " #" << idxo << " avg = " << sum
	       << " max = " << max << " > " << val[dest] << endl;

	val[dest] = use_max ? max : sum;

      } else {
	if (debug)
	  cout << "shot : #" << dest << " #" << idxo << " avg = " << sum
	       << " max = " << max << " < " << val[dest] << endl;
      }
    }

    multimap<double,size_t> res;
    for (auto i=val.begin(); i!=val.end(); i++)
      res.insert(make_pair(i->second, i->first));

    vector<pair<size_t,double> > ret;
    size_t n = 0;
    for (auto i=res.rbegin(); i!=res.rend() && n<nres; i++, n++)
      ret.push_back(make_pair(i->second, i->first));

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  const DataBase::trecvid_med_event& DataBase::TrecvidMedEvent(const string&
							       eid) {
    static trecvid_med_event dummy;

    auto p = trecvid_med_event_map.find(eid);
    if (p==trecvid_med_event_map.end()) {
      trecvid_med_event e;
      e.event_id = eid;

      string fn = ExpandPath("MEDDataDivisions/MEDDATA/doc/eventtexts/"+
			     eid+".txt");
      string txt = FileToString(fn);
      vector<string> a = SplitInSomething("\n", true, txt);

      if (a[0].find("Event name: ")!=0)
	return dummy;
      e.event_name = a[0].substr(12);

      // definition 
      if (a[1].find("Definition: ")!=0)
	return dummy;
      e.definition = a[1].substr(12);
      int tmp_num = 0;
      for (int i = 2; a[i].find( "Explication:") == string::npos ; i++){
        e.definition += " " + a[i];
	tmp_num = i;
	} 
      

	// explication
      string tmp_exp;	
      for ( int i = tmp_num+1; a[i].find( "description:") == string::npos ; i++){
	  
	     if (a[i].find( "Explication:") != string::npos){
			//e.explication.push_back(a[i].substr(13));
		tmp_exp += " " +a[i].substr(13);
		
		}
	      else{
         		//e.explication.push_back( a[i]);
         		tmp_exp += " " +a[i];
		}

	      if (a[i] == "" ){
			e.explication.push_back(tmp_exp);
			tmp_exp ="";

		}
	tmp_num = i;
         
        }
	e.explication.push_back( tmp_exp);
	//scene
	string tmp_scene;
	for (int i = tmp_num+1; a[i].find( "objects/people:") == string::npos ; i++){
		if (a[i].find( "description:") != string::npos){
                                continue;

                        }
                else if (a[i].find( "scene:") != string::npos){
			 tmp_scene += " " + a[i].substr(8);
		}
 
		else{
		tmp_scene += " " + a[i];
		}
		tmp_num = i;
	}
	//replace should be subrutine also?
	string::size_type Pos(tmp_scene.find("("));
	while( Pos != string::npos){
		tmp_scene.replace(Pos, 1, ",");
		Pos = tmp_scene.find("(", Pos+1);  
	
	}
	Pos=tmp_scene.find(")");
        while( Pos != string::npos){
                tmp_scene.replace(Pos, 1, ",");
                Pos = tmp_scene.find(")", Pos+1);

        }	
		
	
	if (tmp_scene.find(";") != string::npos){
		e.scene = SplitInSomething(";", true , tmp_scene);
	}
	else {
		 e.scene = SplitInSomething(",", true , tmp_scene);
	}
	// object
	string tmp_obj;
        for (int i = tmp_num+1; a[i].find( "activities:") == string::npos ; i++){
                
                if (a[i].find( "objects/people:") != string::npos){
                         tmp_obj += " " + a[i].substr(17);
                }
 
                else{
                tmp_obj += " " + a[i];
                }
                tmp_num = i;

        }
        if (tmp_obj.find(";") != string::npos){
                e.objects = SplitInSomething(";", true , tmp_obj);
        }
        else {
                 e.objects = SplitInSomething(",", true , tmp_obj);

        }
	//activities
	string tmp_act;
        for (int i = tmp_num+1; a[i].find( "audio:") == string::npos ; i++){
                
                if (a[i].find( "activities:") != string::npos){
                         tmp_act += " " + a[i].substr(13);
                }
 
                else{
                tmp_act += " " + a[i];
                }
                tmp_num = i;
        }
        if (tmp_act.find(";") != string::npos){
                e.activities = SplitInSomething(";", true , tmp_act);
        }
        else {
                 e.activities = SplitInSomething(",", true , tmp_act);

        }
	//audio
	 string tmp_aud;
        for (size_t i = tmp_num+1;  i < a.size()  ; i++){

                if (a[i].find( "audio:") != string::npos){
                         tmp_aud += " " + a[i].substr(8);
                }

                else{
                tmp_aud += " " + a[i];
                }
                tmp_num = i;
        }
        if (tmp_aud.find(";") != string::npos){
                e.audio = SplitInSomething(";", true , tmp_aud);
        }
        else {
                 e.audio = SplitInSomething(",", true , tmp_aud);
        }

      trecvid_med_event_map[eid] = e;
      p = trecvid_med_event_map.find(eid);
    }

    return p->second;
  }

  /////////////////////////////////////////////////////////////////////////////

  void DataBase::trecvid_med_event::show(ostream& os) const {
    os << "event_id=\"" << event_id << "\"" << endl;
    os << "event_name=\"" << event_name << "\"" << endl;
    os << "definition=\"" << definition << "\"" << endl;

    os << "explication=";
    if (explication.empty())
      os << endl;
    for (size_t i=0; i<explication.size(); i++)
      os << (i?"            ":"") << "\"" << explication[i] << "\"" << endl;

    os << "scene=";
    if (scene.empty())
      os << endl;
    for (size_t i=0; i<scene.size(); i++)
      os << (i?"      ":"") << "\"" << scene[i] << "\"" << endl;

    os << "objects=";
    if (objects.empty())
      os << endl;
    for (size_t i=0; i<objects.size(); i++)
      os << (i?"        ":"") << "\"" << objects[i] << "\"" << endl;

    os << "activities=";
    if (activities.empty())
      os << endl;
    for (size_t i=0; i<activities.size(); i++)
      os << (i?"           ":"") << "\"" << activities[i] << "\"" << endl;

    os << "audio=";
    if (audio.empty())
      os << endl;
    for (size_t i=0; i<audio.size(); i++)
      os << (i?"      ":"") << "\"" << audio[i] << "\"" << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadExternalMetaData(const string& type,
				      const map<string,string>& param) {
    const string msg = "ReadExternalMetaData("+type+") : ";

    if (type=="COCO") {
      auto param_x = param;
      if (frag!="")
	param_x["frag"] = frag;
      return ReadCOCO(param_x);
    }

    if (type=="visualgenome") {
      postponed.push_back(make_pair("*visualgenome*", ""));
      externalmetadata_param = param;
      return true;
    }

    return ShowError(msg+"type not understood");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadCOCO(const map<string,string>& param) {
    const string msg = "ReadCOCO() : ";

    auto ti = param.find("train_instances");
    if (ti==param.end())
      return ShowError(msg+"train_instances not defined");

    auto tc = param.find("train_captions");
    if (tc==param.end())
      return ShowError(msg+"train_captions not defined");

    auto vi = param.find("val_instances");
    if (vi==param.end())
      return ShowError(msg+"val_instances not defined");

    auto vc = param.find("val_captions");
    if (vc==param.end())
      return ShowError(msg+"val_captions not defined");

    auto xc = param.find("test_captions");
    if (xc==param.end())
      return ShowError(msg+"test_captions not defined");

    auto s = param.find("splits");
    if (s==param.end())
      return ShowError(msg+"splits not defined");

    string cocofrag = "all";
    if (param.find("frag")!=param.end())
      cocofrag = param.at("frag");

#if defined(HAVE_JSON_JSON_H)
    return ReadCOCOjsoncpp(cocofrag, ti->second, tc->second, vi->second, 
			   vc->second, xc->second, s->second);
#elif defined(PICSOM_USE_JAULA) 
    return ReadCOCOjaula(tcocofrag, i->second, tc->second, vi->second, 
			 vc->second, xc->second, s->second);
#else
    return ShowError(msg+"neither HAVE_JSON_JSON_H nor PICSOM_USE_JAULA");
#endif
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_JSON_JSON_H
  bool 
  DataBase::ReadCOCOjsoncpp(const string& cocofrag,
			    const string& train_inst, const string& train_capt,
			    const string& val_inst,   const string& val_capt,
			    const string& test_capt, const string& splits) {
    const string msg = "ReadCOCOjsoncpp() : ";

    bool keep_coco_json = true;

    size_t show_every = 1000, show_samples = 0;

    bool cook = false;
    string captions = "gt-"+string(cook?"precooked":"raw");
    bool lucene_exists = false;
    string lucenedir = ExpandPath("lucene/"+captions);
    if (DirectoryExists(lucenedir)) {
      WriteLog("Using existing lucene index in <"+ShortFileName(lucenedir)
	       +">");
      lucene_exists = true;
    }

    vector<string> infilesx {
      val_capt, val_inst, train_capt, train_inst, test_capt, splits };
    //   0         1           2           3          4        5

    string remove_chars  = "'.";
    string spacify_chars = "\"?!.,-':;#$()/\t\n";

    int noldstart = Size();
    size_t ncapt_val = 0, ncapt_train = 0;
    size_t nimag_val = 0, nimag_train = 0, nimag_test = 0;
    map<string,set<size_t> > cls;
    set<string> all_classes;
    for (size_t ss=0; ss<6; ss++) {
      if (cocofrag=="min")
	continue;

      vector<string> infilevec;
      if (ss==4) {
	infilevec = SplitInCommas(infilesx[ss]);
	for (size_t ifv=0; ifv<infilevec.size(); ifv++) {
	  size_t p = infilevec[ifv].find_first_not_of(" \t\n");
	  infilevec[ifv].erase(0, p);
	  p = infilevec[ifv].find_last_not_of(" \t\n");
	  if (p!=string::npos)
	    infilevec[ifv].erase(p+1);
	}
      } else
	infilevec.push_back(infilesx[ss]);

      for (size_t ifv=0; ifv<infilevec.size(); ifv++) {
	size_t ndup = 0;
	string indatax = infilevec[ifv], indata = indatax, file_path = indatax;
	if (indata[0]!='/')
	  indata.insert(0, Path()+"/");
	if (!FileExists(indata))
	  return ShowError(msg+"file <"+indatax+"> => <"+indata+
			   "> doesn't exist");
      
	if (ss<5) {
	  size_t p = file_path.rfind('.');
	  if (p==string::npos)
	    return ShowError(msg+"dot not found in <"+file_path+">");
	  file_path.erase(p);
	  p = file_path.rfind('_');
	  if (p==string::npos)
	    return ShowError(msg+"underscore not found in <"+file_path+">");
	  file_path.erase(0, p+1);
	} else
	  file_path = "splits";

	WriteLog("Starting to read <"+ShortFileName(indata)+">");
	ifstream istr(indata);
	Json::Value  doctmp;
	Json::Value& doc = keep_coco_json ? coco_json[ss] : doctmp;
	istr >> doc;

	WriteLog("Read for <"+file_path+"> -- starting to parse.");

	string type = ss<4 ? doc.get("type", "").asString() : "";

	if (ss==0 || ss==2 || ss==4) {
	  if (false && ss!=4 && type!="captions")
	    return ShowError(msg+"type=\""+type+"\" should be \"captions\"");

	  int nold = Size();

	  const Json::Value& px = doc["images"];
	  size_t tot = px.size(), nadd = 0;
	  for (size_t k=0; k<px.size(); k++) {
	    const Json::Value& img = px[(int)k];
	    int id = img.get("id", "-1").asInt();
	    if (id>999999 || id<0)
	      return ShowError(msg+"2");
	    char tmp[20];
	    sprintf(tmp, "%06d", id);
	    string label = tmp;
	    int idx = LabelIndexGentle(label, false);
	    bool added = false;
	    if (idx!=-1 && (ss!=4 || ifv==0))
	      return ShowError(msg+"label <"+label+"> exists already");
	    if (idx==-1) {
	      idx = AddOneLabel(label, target_image, false, true);
	      added = true;
	    } else
	      ndup++;

	    string file_name = img.get("file_name", "").asString();
	    
	    if (!Picsom()->Quiet() && nadd<show_samples)
	      cout << "  " << label << " " << id << " "
		   << file_path << " " << file_name << endl;

	    if (added) {
	      map<string,string> hash;
	      hash["url"]    = file_path+".zip["+file_name+"]";
	      hash["file"]   = label+".jpeg";
	      hash["auxid"]  = ToStr(id);
	      hash["width"]  = img.get("width",  "0").asString();
	      hash["height"] = img.get("height", "0").asString();
	      origins_entry_cache_map[Size()-1] = hash;
	    }

	    cls[file_path].insert(idx);

	    (ss==0 ? nimag_val : ss==2 ? nimag_train : nimag_test)++;

	    if (!Picsom()->Quiet() && show_every && ++nadd%show_every==0)
	      cout << " ... added " << nadd << "/" << tot << "\r" << flush;
	  }
	  WriteLog("Read metadata for "+ToStr(Size()-nold)+" images from <"+
		   ShortFileName(indata)+"> + "+ToStr(ndup)+" duplicates");
	}

	if (ss==1 || ss==3) {
	  if (false && type!="instances")
	    return ShowError(msg+"type=\""+type+"\" should be \"instances\"");

	  map<size_t,string> category_id_to_name;
	  const Json::Value& pxc = doc["categories"];
	  for (size_t k=0; k<pxc.size(); k++) {
	    const Json::Value& img = pxc[(int)k];
	    size_t id   = img.get("id", "-1").asInt();
	    string name = img.get("name", "").asString();
	    for (;;) {
	      size_t p = name.find(' ');
	      if (p==string::npos)
		break;
	      name.replace(p, 1, "_");
	    }
	    if (!Picsom()->Quiet() && category_id_to_name.size()<show_samples)
	      cout << id << " " << name << endl;
	    category_id_to_name[id] = name;
	    if (keep_coco_json)
	      coco_category[id] = name;
	  }

	  map<size_t,vector<size_t> > class_member;
	  size_t ntot = 0;
	  const Json::Value& pxa = doc["annotations"];
	  for (size_t k=0; k<pxa.size(); k++) {
	    const Json::Value& img = pxa[(int)k];
	    size_t image_id    = img.get("image_id",    "-1").asInt();
	    size_t category_id = img.get("category_id", "-1").asInt();
	    if (!Picsom()->Quiet() && ntot<show_samples)
	      cout << image_id << " " << category_id << endl;
	    class_member[category_id].push_back(image_id);
	    ntot++;
	    string label = "00000"+ToStr(image_id);
	    label.erase(0, label.size()-6);
	    size_t idx = LabelIndex(label);
	    string cname = category_id_to_name[category_id];
	    cls[cname].insert(idx);
	    all_classes.insert(cname);
	  }
	  WriteLog("Read category data for <"+file_path+"> in "
		   +ToStr(category_id_to_name.size())
		   +" classes, "+ToStr(ntot)+" total members in "
		   +ToStr(class_member.size())+" classes");
	}

	if (ss==0 || ss==2) {
	  if (false && type!="captions")
	    return ShowError(msg+"type=\""+type+"\" should be \"captions\"");

	  if (lucene_exists)
	    continue;

	  map<size_t,string> concat;

	  const Json::Value& px = doc["annotations"];
	  for (size_t k=0; k<px.size(); k++) {
	    const Json::Value& ann = px[(int)k];
	    size_t id       = ann.get("id",       "-1").asInt();
	    size_t image_id = ann.get("image_id", "-1").asInt();
	    string caption  = ann.get("caption",    "").asString();
	    string captionorig = caption;

	    if (cook) {
	      caption = LowerCase(caption);
	      if (remove_chars!="")
		for (;;) {
		  size_t p = caption.find_first_of(remove_chars);
		  if (p==string::npos)
		    break;
		  caption.erase(p, 1);
		}
	      if (spacify_chars!="")
		for (;;) {
		  size_t p = caption.find_first_of(spacify_chars);
		  if (p==string::npos)
		    break;
		  caption[p] = ' ';
		}
	      for (;;) {
		size_t p = caption.find("  ");
		if (p==string::npos)
		  break;
		caption.erase(p, 1);
	      }
	      while (caption[0]==' ')
		caption.erase(0, 1);
	      while (caption[caption.size()-1]==' ')
		caption.erase(caption.size()-1);
	    } else {
	      for (;;) {
		size_t p = caption.find_first_of("\n\r");
		if (p==string::npos)
		  break;
		caption[p] = ' ';
	      }
	      for (;;) {
		size_t p = caption.find(" # ");
		if (p==string::npos)
		  break;
		caption.replace(p, 3, " ## ");
	      }
	      if (caption.find("# ")==0)
		caption.insert(0, "#");
	      if (caption.rfind(" #")==caption.size()-2)
		caption += "#";
	    }

	    char tmp[20];
	    sprintf(tmp, "%06d", (int)image_id);
	    string label = tmp;
	    size_t idx = LabelIndex(label);

	    if (!Picsom()->Quiet())
	      cout << "[" << id << "][" << image_id << "] <" << label
		   << "> #" << idx << " ["
		   << captionorig << "] -> [" << caption << "]"
		   << endl;

	    if (concat.find(idx)!=concat.end())
	      concat[idx] += " # ";
	    concat[idx] += caption;

	    (ss==0 ? ncapt_val : ncapt_train)++;
	  }

	  for (auto i=concat.begin(); i!=concat.end(); i++) {
	    vector<string> pp = SplitOnWord(" # ", i->second);
	    if (pp.size()<5)
	      /*return*/ ShowError(msg+"#"+ToStr(i->first)+" n of parts "
				   +ToStr(pp.size())+"<5");

	    if (!Picsom()->Quiet())
	      cout << i->first << " : " << i->second << endl;

	    TextIndexStoreLine(captions+" add-attribute text "+i->second);
	    TextIndexStoreLine(captions+" add-document "+Label(i->first));
	  }
	}

	if (ss==5) { // splits
	  const Json::Value& px = doc["images"];
	  for (size_t k=0; k<px.size(); k++) {
	    const Json::Value& img = px[(int)k];
	    string filename = img.get("filename", "").asString();
	    string split    = img.get("split",    "").asString();
	    string label    = filename;

	    size_t p = label.rfind('.');
	    if (p==string::npos)
	      return ShowError(msg+"<"+label+"> no dot");
	    label.erase(p);
	    if (label.size()<6)
	      return ShowError(msg+"<"+label+"> too short");
	    label.erase(0, label.size()-6);
	    size_t idx = LabelIndex(label);
	    cls[split+"split"].insert(idx);
	  }
	}
      }
    }

    ostringstream clsss;
    for (auto i=cls.begin(); i!=cls.end(); i++) {
      Picsom()->MkDirHier(ExpandPath("classes"), 02775);
      string cname = i->first, cfile = ExpandPath("classes", cname);
      if (FileSize(cfile)<1) {
	ground_truth gt(cname, Size());
	for (auto j=i->second.begin(); j!=i->second.end(); j++)
	  gt[*j] = 1;
	WriteClassFile(cfile, gt);
	ContentsInsert(gt);
      }
      clsss << " " << i->first << "=" << i->second.size();
    }

    vector<string> vec_all_classes(all_classes.begin(), all_classes.end());
    string file_all_classes = ExpandPath("classes")+"/all_classes";
    if (!FileExists(file_all_classes))
      WriteClassFile(file_all_classes, vec_all_classes,
		     "", ground_truth(), "", false, true);

    WriteLog("Read metadata for "+ToStr(Size()-noldstart)+" images ("
	     +ToStr(nimag_val)+" validation + "+ToStr(nimag_train)+" train + "
	     +ToStr(nimag_test)+" test) and "
	     +ToStr(ncapt_val+ncapt_train)+" captions ("
	     +ToStr(ncapt_val)+" validation + "+ToStr(ncapt_train)+" train)");
    WriteLog("Classes contain images:"+clsss.str());
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,list<vector<pair<float,float> > > > >
  DataBase::COCOmasks(size_t idx) {
    list<pair<string,list<vector<pair<float,float> > > > > ret;

    bool debug = false;

    if (coco_json.empty())
      return ret;

    if (debug)
      cout << "#" << idx << " <" << Label(idx) << ">" << endl;

    const string& label = Label(idx);
    size_t lint = atoi(label.c_str());

    for (size_t s=0; s<2; s++) {
      if (debug)
	cout << "s=" << s << endl;
      const Json::Value& doc = coco_json[s==0?1:3];
      const Json::Value& annolist = doc["annotations"];
      if (debug)
	cout << "annolist.size()=" << annolist.size() << endl;
      for (size_t k=0; k<annolist.size(); k++) {
	if (debug)
	  cout << "k=" << k << endl;
	const Json::Value& anno = annolist[(int)k];
	size_t image_id    = anno.get("image_id", "-1").asUInt64();;
	size_t id = anno.get("id", "0").asUInt64();;
	if (debug)
	  cout << "id=" << id << " image_id=" << image_id << endl;
	if (image_id==lint) {
	  size_t category_id = anno.get("category_id", "-1").asUInt64();;
	  string category = coco_category[category_id];
	  const Json::Value& segm = anno["segmentation"];
	  bool isarray = segm.isArray();
	  if (debug)
	    cout << "annotation id=" << id
		 << " isarray=" << (isarray?1:0)
		 << " segm.size()=" << segm.size() << endl;
	  if (isarray) {
	    list<vector<pair<float,float> > > sl;
	    for (size_t j=0; j<segm.size(); j++) {
	      if (debug)
		cout << "j=" << j << endl;
	      const Json::Value& segmin = segm[(int)j];
	      if (debug)
		cout << "segmin.size()=" << segmin.size() << endl;
	      vector<pair<float,float> > sv;
	      for (size_t i=0; i<segmin.size(); i+=2) {
		float x = segmin[(int)i  ].asFloat();
		float y = segmin[(int)i+1].asFloat();
		if (debug)
		  cout << x << "," << y << " ";
		sv.push_back(make_pair(x, y));
	      }
	      if (debug && j+1<segm.size())
		cout << " --- ";
	      sl.push_back(sv);
	    }
	    if (debug)
	      cout << endl;
	    ret.push_back(make_pair(category, sl));

	  } else if (debug)
	    cout << "segmentation is not an array" << endl;
	}
      }
    }

    return ret;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_JAULA
  bool 
  DataBase::ReadCOCOjaula(const string& /*cocofrac*/,
			  const string& train_inst, const string& train_capt,
			  const string& val_inst,   const string& val_capt,
			  const string& test_capt, const string& splits) {
    const string msg = "ReadCOCOjaula() : ";

    size_t show_every = 1000, show_samples = 5;

    bool lucene_exists = false;
    string lucenedir = ExpandPath("lucene/captions");
    if (DirectoryExists(lucenedir)) {
      WriteLog("Using existing lucene index in <"+ShortFileName(lucenedir)
	       +">");
      lucene_exists = true;
    }

    vector<string> infiles {
      val_capt, val_inst, train_capt, train_inst, test_capt, splits };
    //   0         1           2           3          4        5

    string remove_chars  = "'.";
    string spacify_chars = "\"?!.,-':;#$()/\t\n";

    int noldstart = Size();
    size_t ncapt_val = 0, ncapt_train = 0;
    size_t nimag_val = 0, nimag_train = 0, nimag_test = 0;
    map<string,set<size_t> > cls;
    set<string> all_classes;
    for (size_t ss=0; ss<6; ss++) {
      string indatax = infiles[ss], indata = indatax, file_path = indatax;
      if (indata[0]!='/')
	indata.insert(0, Path()+"/");
      if (!FileExists(indata))
	return ShowError(msg+"file <"+indatax+"> => <"+indata+
			 "> doesn't exist");
      
      if (ss<5) {
	size_t p = file_path.rfind('.');
	if (p==string::npos)
	  return ShowError(msg+"dot not found in <"+file_path+">");
	file_path.erase(p);
	p = file_path.rfind('_');
	if (p==string::npos)
	  return ShowError(msg+"underscore not found in <"+file_path+">");
	file_path.erase(0, p+1);
      } else
	file_path = "splits";

      WriteLog("Starting to read <"+ShortFileName(indata)+">");
      ifstream istr(indata);
      JAULA::Value_Complex *doc = JAULA::Parser::parseStream(istr);
      if (doc->getType()!=JAULA::Value::TYPE_OBJECT)
	return ShowError(msg+"1");

      WriteLog("Read for <"+file_path+"> -- starting to parse.");

      const JAULA::Value_Object& o = *dynamic_cast<JAULA::Value_Object*>(doc);
      auto& m = o.getData();
      string type;
      if (ss<4)
	type = JAULA_string(o, "type");

      if (ss==0 || ss==2 || ss==4) {
	if (ss!=4 && type!="captions")
	  return ShowError(msg+"type=\""+type+"\" should be \"captions\"");

	int nold = Size();

	auto p = m.find("images");
	if (p!=m.end()) {
	  const JAULA::Value_Array& a
	    = *dynamic_cast<JAULA::Value_Array*>(p->second);
	  auto& l = a.getData();
	  size_t tot = l.size(), nadd = 0;
	  for (auto k=l.begin(); k!=l.end(); k++) {
	    const JAULA::Value_Object& img
	      = *dynamic_cast<JAULA::Value_Object*>(*k);
	    int id = JAULA_int(img, "id");
	    if (id>999999)
	      return ShowError(msg+"2");
	    char tmp[20];
	    sprintf(tmp, "%06d", id);
	    string label = tmp;
	    int idx = AddOneLabel(label, target_image, false, true);
	    
	    //string file_path = JAULA_string(img, "file_path");
	    string file_name = JAULA_string(img, "file_name");
	    
	    if (!Picsom()->Quiet() && nadd<show_samples)
	      cout << "  " << label << " " << id << " "
		   << file_path << " " << file_name << endl;

	    map<string,string> hash;
	    //hash["url"] = file_path+".tar["+file_name+"]";
	    hash["url"]   = file_path+".zip["+file_name+"]";
	    hash["file"]  = label+".jpeg";
	    hash["auxid"] = ToStr(id);
	    origins_entry_cache_map[Size()-1] = hash;

	    cls[file_path].insert(idx);

	    (ss==0 ? nimag_val : ss==2 ? nimag_train : nimag_test)++;

	    if (!Picsom()->Quiet() && show_every && ++nadd%show_every==0)
	      cout << " ... added " << nadd << "/" << tot << "\r" << flush;
	  }
	}
	WriteLog("Read metadata for "+ToStr(Size()-nold)+" images from <"+
		 ShortFileName(indata)+">");
      }

      if (ss==1 || ss==3) {
	if (type!="instances")
	  return ShowError(msg+"type=\""+type+"\" should be \"instances\"");

	map<size_t,string> category_id_to_name;

	auto p = m.find("categories");
	if (p!=m.end()) {
	  const JAULA::Value_Array& a
	    = *dynamic_cast<JAULA::Value_Array*>(p->second);
	  auto& l = a.getData();
	  for (auto k=l.begin(); k!=l.end(); k++) {
	    const JAULA::Value_Object& img
	      = *dynamic_cast<JAULA::Value_Object*>(*k);
	    size_t id   = JAULA_int(img,    "id");
	    string name = JAULA_string(img, "name");
	    for (;;) {
	      size_t p = name.find(' ');
	      if (p==string::npos)
		break;
	      name.replace(p, 1, "_");
	    }
	    if (!Picsom()->Quiet() && category_id_to_name.size()<show_samples)
	      cout << id << " " << name << endl;
	    category_id_to_name[id] = name;
	  }
	}
	map<size_t,vector<size_t> > class_member;
	size_t ntot = 0;
	p = m.find("annotations");
	if (p!=m.end()) {
	  const JAULA::Value_Array& a
	    = *dynamic_cast<JAULA::Value_Array*>(p->second);
	  auto& l = a.getData();
	  for (auto k=l.begin(); k!=l.end(); k++) {
	    const JAULA::Value_Object& img
	      = *dynamic_cast<JAULA::Value_Object*>(*k);
	    size_t image_id    = JAULA_int(img, "image_id");
	    size_t category_id = JAULA_int(img, "category_id");
	    if (!Picsom()->Quiet() && ntot<show_samples)
	      cout << image_id << " " << category_id << endl;
	    class_member[category_id].push_back(image_id);
	    ntot++;
	    string label = "00000"+ToStr(image_id);
	    label.erase(0, label.size()-6);
	    size_t idx = LabelIndex(label);
	    string cname = category_id_to_name[category_id];
	    cls[cname].insert(idx);
	    all_classes.insert(cname);
	  }
	}
	WriteLog("Read category data for <"+file_path+"> in "
		 +ToStr(category_id_to_name.size())
		 +" classes, "+ToStr(ntot)+" total members in "
		 +ToStr(class_member.size())+" classes");
      }

      if (ss==0 || ss==2) {
	if (type!="captions")
	  return ShowError(msg+"type=\""+type+"\" should be \"captions\"");

	if (lucene_exists)
	  continue;

	map<size_t,string> concat;

	auto p = m.find("annotations");
	if (p!=m.end()) {
	  const JAULA::Value_Array& a
	    = *dynamic_cast<JAULA::Value_Array*>(p->second);
	  auto& l = a.getData();
	  for (auto k=l.begin(); k!=l.end(); k++) {
	    const JAULA::Value_Object& snt
	      = *dynamic_cast<JAULA::Value_Object*>(*k);
	    int id          = JAULA_int(   snt, "id");
	    int image_id    = JAULA_int(   snt, "image_id");
	    string caption = JAULA_string(snt, "caption");
	    string captionorig = caption;

	    caption = LowerCase(caption);
	    if (remove_chars!="")
	      for (;;) {
		size_t p = caption.find_first_of(remove_chars);
		if (p==string::npos)
		  break;
		caption.erase(p, 1);
	      }
	    if (spacify_chars!="")
	      for (;;) {
		size_t p = caption.find_first_of(spacify_chars);
		if (p==string::npos)
		  break;
		caption[p] = ' ';
	      }
	    for (;;) {
	      size_t p = caption.find("  ");
	      if (p==string::npos)
		break;
	      caption.erase(p, 1);
	    }
	    while (caption[0]==' ')
	      caption.erase(0, 1);
	    while (caption[caption.size()-1]==' ')
	      caption.erase(caption.size()-1);

	    char tmp[20];
	    sprintf(tmp, "%06d", image_id);
	    string label = tmp;
	    size_t idx = LabelIndex(label);

	    if (!Picsom()->Quiet())
	      cout << "[" << id << "][" << image_id << "] <" << label
		   << "> #" << idx << " ["
		   << captionorig << "] -> [" << caption << "]"
		   << endl;

	    if (concat.find(idx)!=concat.end())
	      concat[idx] += " # ";
	    concat[idx] += caption;

	    (ss==0 ? ncapt_val : ncapt_train)++;
	  }

	  for (auto i=concat.begin(); i!=concat.end(); i++) {
	    vector<string> pp = SplitOnWord(" # ", i->second);
	    if (pp.size()<5)
	      /*return*/ ShowError(msg+"#"+ToStr(i->first)+" n of parts "
				   +ToStr(pp.size())+"<5");

	    if (!Picsom()->Quiet())
	      cout << i->first << " : " << i->second << endl;

	    TextIndexStoreLine("captions add-attribute text "+i->second);
	    TextIndexStoreLine("captions add-document "+Label(i->first));
	  }
	}
      }

      if (ss==5) { // splits
        auto p = m.find("images");
	if (p!=m.end()) {
	  const JAULA::Value_Array& a
	    = *dynamic_cast<JAULA::Value_Array*>(p->second);
	  auto& l = a.getData();
	  for (auto k=l.begin(); k!=l.end(); k++) {
	    const JAULA::Value_Object& img
	      = *dynamic_cast<JAULA::Value_Object*>(*k);
	    string filename = JAULA_string(img, "filename");
	    string split    = JAULA_string(img, "split");
	    string label = filename;
	    size_t p = label.rfind('.');
	    if (p==string::npos)
	      return ShowError(msg+"<"+label+"> no dot");
	    label.erase(p);
	    if (label.size()<6)
	      return ShowError(msg+"<"+label+"> too short");
	    label.erase(0, label.size()-6);
	    size_t idx = LabelIndex(label);
	    cls[split+"split"].insert(idx);
	  }
	}
      }
    }

    ostringstream clsss;
    for (auto i=cls.begin(); i!=cls.end(); i++) {
      Picsom()->MkDirHier(ExpandPath("classes"), 02775);
      string cname = i->first, cfile = ExpandPath("classes", cname);
      if (FileSize(cfile)<1) {
	ground_truth gt(cname, Size());
	for (auto j=i->second.begin(); j!=i->second.end(); j++)
	  gt[*j] = 1;
	WriteClassFile(cfile, gt);
	ContentsInsert(gt);
      }
      clsss << " " << i->first << "=" << i->second.size();
    }

    vector<string> vec_all_classes(all_classes.begin(), all_classes.end());
    string file_all_classes = ExpandPath("classes")+"/all_classes";
    if (!FileExists(file_all_classes))
      WriteClassFile(file_all_classes, vec_all_classes,
		     "", ground_truth(), "", false, true);

    WriteLog("Read metadata for "+ToStr(Size()-noldstart)+" images ("
	     +ToStr(nimag_val)+" validation + "+ToStr(nimag_train)+" train + "
	     +ToStr(nimag_test)+" test) and "
	     +ToStr(ncapt_val+ncapt_train)+" captions ("
	     +ToStr(ncapt_val)+" validation + "+ToStr(ncapt_train)+" train)");
    WriteLog("Classes contain images:"+clsss.str());
    
    return true;
  }
#endif // PICSOM_USE_JAULA

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::ReadVisualGenome(const map<string,string>& param) {
    const string msg = "ReadVisualGenome() : ";
    stringstream ss;
    for (auto i=param.begin(); i!=param.end(); i++)
      ss << i->first << "=" << i->second << " ";
    WriteLog(msg+ss.str()+"starting");
    struct timespec start = TimeNow();

    string r = "";
    if (param.find("frag")!=param.end())
      r = param.at("frag");

    bool read_i = r.find('i')!=string::npos;
    bool read_o = r.find('o')!=string::npos;
    bool read_r = r.find('r')!=string::npos;
    bool read_a = r.find('a')!=string::npos;
    bool read_d = r.find('d')!=string::npos;
    bool read_g = r.find('g')!=string::npos;
    bool read_s = r.find('s')!=string::npos;
    bool read_y = r.find('y')!=string::npos;

#if defined(HAVE_JSON_JSON_H)
    bool dumpspec  = r.find('S')!=string::npos;
    bool showstats = r.find('X')!=string::npos;
    bool debug     = r.find('D')!=string::npos;
    bool debug_mem = r.find('M')!=string::npos;
    bool small     = r.find('T')!=string::npos;
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_i) {
      string f = "download/1.2/image_data.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& img = doc[(int)k];
	size_t idx = LabelIndex(ZeroPadToLabel(img["image_id"].asString()));
	vg_image_data[idx] = VG_image_data(img, this);
	if (debug)
	  cout << "image_data " << k << " : " << vg_image_data[idx].str() 
	       << endl;
      }
      WriteLog(msg+"done with <"+f+">");
    }
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_o) {
      string f = "download/1.4/objects.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& obj = doc[(int)k];
	size_t idx = LabelIndex(ZeroPadToLabel(obj["image_id"].asString()));
	vg_objects[idx] = VG_objects(obj, this);
	if (debug)
	  cout << "objects " << k << " : " << vg_objects[idx].str() << endl;
	const VG_objects& o = vg_objects[idx];
	if (dumpspec)
	  for (size_t i=0; i<o.objects.size(); i++) {
	    string lo = Label(o.idx)+"_o"+ToStr(o.objects[i].object_id);
	    cout << "**object** " << lo << " image+segment " << lo << ".jpg "
		 << Label(o.idx) << "(" << o.objects[i].x << "," << 
	      o.objects[i].y
		 << "," << o.objects[i].x+o.objects[i].w-1
		 << "," << o.objects[i].y+o.objects[i].h-1
		 << ") - image/jpeg - - 0 0 -" << endl
		 << "**subobjects** " << Label(o.idx) << " + " << lo << endl;
	  }
      }
      WriteLog(msg+"done with <"+f+">");
      if (showstats) {
	auto h = VG_stats("objects", true);
	CreateWordnetOntology(h, "");
      }
    }
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_r) {
      string f = "download/1.4/relationships.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& rel = doc[(int)k];
	size_t idx = LabelIndex(ZeroPadToLabel(rel["image_id"].asString()));
	vg_relationships[idx] = VG_relationships(rel, this);
	if (debug)
	  cout << "relationships " << k << " : " 
	       << vg_relationships[idx].str() << endl;
	const VG_relationships& r = vg_relationships[idx];
	for (auto i=r.relationships.begin(); i!=r.relationships.end(); i++) {
	  //vg_relationship_id_to_idx[i->relationship_id] = idx;
	  //vg_subject_id_to_relationship_id...
	}
      }
      WriteLog(msg+"done with <"+f+">");
    }
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_a) {
      string f = "download/1.2/attributes.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& att = doc[(int)k];
	size_t idx = LabelIndex(ZeroPadToLabel(att["image_id"].asString()));
	vg_attributes[idx] = VG_attributes(att, this);
	if (debug)
	  cout << "attributes " << k << " : " 
	       << vg_attributes[idx].str() << endl;
      }
      WriteLog(msg+"done with <"+f+">");
    }
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_d) {
      string f = "download/1.2/region_descriptions.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& rde = doc[(int)k];
	size_t idx = LabelIndex(ZeroPadToLabel(rde["regions"][0]
					       ["image_id"].asString()));
	vg_region_descriptions[idx] = VG_region_descriptions(rde, this);
	if (debug)
	  cout << "region_descriptions " << k << " : " 
	       << vg_region_descriptions[idx].str() << endl;
	const VG_region_descriptions& r = vg_region_descriptions[idx];
	if (dumpspec)
	  for (size_t i=0; i<r.regions.size(); i++) {
	    string lo = Label(r.idx)+"_r"+ToStr(r.regions[i].region_id);
	    cout << "**object** " << lo << " image+segment " << lo << ".jpg "
		 << Label(r.idx) << "(" << r.regions[i].x << "," 
		 << r.regions[i].y
		 << "," << r.regions[i].x+r.regions[i].width-1
		 << "," << r.regions[i].y+r.regions[i].height-1
		 << ") - image/jpeg - - 0 0 -" << endl
		 << "**subobjects** " << Label(r.idx) << " + " << lo << endl;
	  }
      }
      WriteLog(msg+"done with <"+f+">");
    }
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_g) {
      string f = "download/1.2/region_graphs.json.zip";
      if (small)
	f = "download/1.2/region_graphs_1000.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& rgr = doc[(int)k];
	size_t idx = LabelIndex(ZeroPadToLabel(rgr["image_id"].asString()));
	vg_region_graphs[idx] = VG_region_graphs(rgr, this);
	if (debug)
	  cout << "region_graphs " << k << " : " 
	       << vg_region_graphs[idx].str() << endl;
      }
      WriteLog(msg+"done with <"+f+">");
    }
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_s) {
      string f = "download/1.2/scene_graphs.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& sgr = doc[(int)k];
	size_t idx = LabelIndex(ZeroPadToLabel(sgr["image_id"].asString()));
	vg_scene_graphs[idx] = VG_scene_graphs(sgr, this);
	if (debug)
	  cout << "scene_graphs " << k << " : " 
	       << vg_scene_graphs[idx].str() << endl;
      }
      WriteLog(msg+"done with <"+f+">");
    }
    
    if (debug_mem)
      DumpMemoryUsage();
    
    if (read_y) {
      string f = "download/1.2/synsets.json.zip";
      WriteLog(msg+"reading <"+f+">");
      Json::Value doc;
      ifstream(Picsom()->UnZip(ExpandPath(f))) >> doc;
      if (debug_mem)
        DumpMemoryUsage();
      WriteLog(msg+"parsing <"+f+">");
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& syn = doc[(int)k];
	vg_synsets[k] = VG_synsets(syn, this);
	if (debug)
	  cout << "synsets " << k << " : " << vg_synsets[k].str() << endl;
      }
      WriteLog(msg+"done with <"+f+">");
    }
    
    float secf = TimeDiff(TimeNow(), start);
    WriteLog(msg+"took "+ToStr(secf)+" s");

    return true;

#else
    return ShowError(msg+"No JSON support compiled");
#endif // HAVE_JSON_JSON_H
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  static size_t asIntNull(const Json::Value& v) {
    return v.isNull() ? -1 : v.asInt64();
  }

  DataBase::VG_image_data::VG_image_data(const Json::Value& v, DataBase *p) {
    image_id  = v["image_id"].asInt64();
    coco_id   = asIntNull(v["coco_id"]);
    flickr_id = asIntNull(v["flickr_id"]);
    width     = v["width"].asInt64();
    height    = v["height"].asInt64();
    url       = v["url"].asString();

    if (p) {
      string l  = p->ZeroPadToLabel(v["image_id"].asString());
      idx = p->LabelIndex(l);
    } else
      idx = -1;

    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_image_data::str() const {
    stringstream ss;
    ss << "idx=" << (int)idx << " image_id=" << image_id
       << " coco_id=" << coco_id << " flickr_id=" << flickr_id
       << " width=" << width << " height=" << height << " url=" << url;
    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  DataBase::VG_objects::VG_objects(const Json::Value& v, DataBase *p) {
    image_id  = v["image_id"].asInt64();
    image_url = v["image_url"].asString();

    const Json::Value& ol = v["objects"];
    for (size_t k=0; k<ol.size(); k++) {
      const Json::Value& o = ol[(int)k];
      object oo;
      oo.object_id = o["object_id"].asInt64();
      oo.x = o["x"].asInt64();
      oo.y = o["y"].asInt64();
      oo.w = o["w"].asInt64();
      oo.h = o["h"].asInt64();

      const Json::Value& sv = o["synsets"];
      for (auto i=sv.begin(); i!=sv.end(); i++)
	oo.synsets.push_back(i->asString());

      const Json::Value& nv = o["names"];
      for (auto i=nv.begin(); i!=nv.end(); i++)
	oo.names.push_back(i->asString());

      const Json::Value& mv = o["merged_object_ids"];
      for (auto i=mv.begin(); i!=mv.end(); i++)
	oo.merged_object_ids.push_back(i->asInt());

      objects.push_back(oo);
    }

    if (p)
      idx = p->LabelIndex(p->ZeroPadToLabel(v["image_id"].asString()));
    else
      idx = -1;

    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_objects::str() const {
    stringstream ss;
    ss << "idx=" << (int)idx << " image_id=" << image_id
       << " image_url=" << image_url << " objects=[";
    for (auto i=objects.begin(); i!=objects.end(); i++) {
      ss << "(object_id=" << i->object_id << " x=" << i->x << " y=" << i->y
	 << " w=" << i->w << " h=" << i->h << " synsets=";
      for (auto j=i->synsets.begin(); j!=i->synsets.end(); j++)
	ss << (j==i->synsets.begin()?"":",") << *j;
      ss << " names=";
      for (auto j=i->names.begin(); j!=i->names.end(); j++)
	ss << (j==i->names.begin()?"":",") << *j;
      ss << " merged_object_ids=";
      for (auto j=i->merged_object_ids.begin(); j!=i->merged_object_ids.end();
	   j++)
	ss << (j==i->merged_object_ids.begin()?"":",") << *j;
      ss << ")";
    }
    ss << "]";

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  DataBase::VG_relationships::object_t::object_t(const Json::Value& o) {
    object_id = o["object_id"].asInt64();
    x = o["x"].asInt64();
    y = o["y"].asInt64();
    w = o["w"].asInt64();
    h = o["h"].asInt64();
    
    if (o.isMember("synsets")) {
      const Json::Value& sv = o["synsets"];
      for (auto i=sv.begin(); i!=sv.end(); i++)
	synsets.push_back(i->asString());
    }
    
    if (o.isMember("synset"))
      synsets.push_back(o["synset"].asString());

    if (o.isMember("names")) {
      const Json::Value& nv = o["names"];
      for (auto i=nv.begin(); i!=nv.end(); i++)
	names.push_back(i->asString());
    }

    if (o.isMember("name"))
      names.push_back(o["name"].asString());
    
    if (o.isMember("merged_object_ids")) {
      const Json::Value& mv = o["merged_object_ids"];
      for (auto i=mv.begin(); i!=mv.end(); i++)
	merged_object_ids.push_back(i->asInt());
    }

    if (false && names.size()!=synsets.size()) {
      cout << "object_id=" << object_id << "  names=" << CommaJoin(names)
	   << "  synsets=" << CommaJoin(synsets) << endl;
    }
  }
  
  /////////////////////////////////////////////////////////////////////////////

  DataBase::VG_relationships::VG_relationships(const Json::Value& v, 
					       DataBase *p) {
    image_id  = v["image_id"].asInt64();

    const Json::Value& rl = v["relationships"];
    for (size_t k=0; k<rl.size(); k++) {
      const Json::Value& r = rl[(int)k];
      relationship rr;
      rr.relationship_id = r["relationship_id"].asInt64();
      rr.predicate       = r["predicate"].asString();
      const Json::Value& sv = r["synsets"];
      for (auto i=sv.begin(); i!=sv.end(); i++)
	rr.synsets.push_back(i->asString());

      rr.subject = object_t(r["subject"]);
      rr.object  = object_t(r["object"]);

      relationships.push_back(rr);
    }

    if (p)
      idx = p->LabelIndex(p->ZeroPadToLabel(v["image_id"].asString()));
    else
      idx = -1;

    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_relationships::object_t::str() const {
    stringstream ss;
    ss << "object_id=" << object_id 
       << " merged_object_ids=" << ToStr(merged_object_ids, ",")
       << " names=" << CommaJoin(names)
       << " synsets=" << CommaJoin(synsets)
       << " x=" << x << " y=" << y << " w=" << w << " h=" << h;

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_relationships::relationship::str() const {
    stringstream ss;
    ss << "relationship_id=" << relationship_id
       << " predicate=" << predicate
       << " synsets=" << CommaJoin(synsets)
       << " subject=(" << subject.str()
       << ") object=(" << object.str() << ")";

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_relationships::str() const {
    stringstream ss;
    ss << "idx=" << (int)idx << " image_id=" << image_id
       << " relationships=[";
    for (auto i=relationships.begin(); i!=relationships.end(); i++)
      ss << "(" << i->str() << ")";
    ss << "]";

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  DataBase::VG_attributes::VG_attributes(const Json::Value& v, DataBase *p) {
    image_id  = v["image_id"].asInt64();

    const Json::Value& al = v["attributes"];
    for (size_t k=0; k<al.size(); k++) {
      const Json::Value& r = al[(int)k];
      attribute aa;
      aa.object_id  = r["object_id"].asInt64();
      // if (VG_object_to_idx(aa.object_id)==-1)
      //   continue;
      for (auto k=r["attributes"].begin(); k!=r["attributes"].end(); k++)
	aa.attributes.push_back(k->asString()); //obs! clean spaces
      attributes.push_back(aa);
    }

    if (p)
      idx = p->LabelIndex(p->ZeroPadToLabel(v["image_id"].asString()));
    else
      idx = -1;

    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_attributes::str() const {
    stringstream ss;
    ss << "idx=" << (int)idx << " image_id=" << image_id
       << " attributes=[";
    for (auto i=attributes.begin(); i!=attributes.end(); i++) {
      ss << "(object_id=" << i->object_id << " attributes=";
      for (auto j=i->attributes.begin(); j!=i->attributes.end(); j++)
	ss << (j==i->attributes.begin()?"":",") << *j;
      ss << ")";
    }
    ss << "]";

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  DataBase::VG_region_descriptions::
  VG_region_descriptions(const Json::Value& v, DataBase *p) {
    const Json::Value& rl = v["regions"];
    for (size_t k=0; k<rl.size(); k++) {
      const Json::Value& r = rl[(int)k];
      region rr;
      rr.region_id = r["region_id"].asInt64();
      rr.image_id  = r["image_id" ].asInt64();
      rr.x = r["x"].asInt64();
      rr.y = r["y"].asInt64();
      rr.width  = r["width" ].asInt64();
      rr.height = r["height"].asInt64();
      rr.phrase = r["phrase"].asString(); //obs! clean spaces
      regions.push_back(rr);
    }

    image_id = regions.front().image_id;
    
    if (p)
      idx = p->LabelIndex(p->ZeroPadToLabel(ToStr(image_id)));
    else
      idx = -1;

    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_region_descriptions::str() const {
    stringstream ss;
    ss << "idx=" << (int)idx << " image_id=" << image_id << " regions=[";
    for (auto i=regions.begin(); i!=regions.end(); i++) {
      ss << "(region_id=" << i->region_id << " image_id=" << i->image_id
	 << " x=" << i->x << " y=" << i->y
	 << " width=" << i->width << " height=" << i->height
	 << " phrase=" << i->phrase << ")";
    }
    ss << "]";

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  DataBase::VG_region_graphs::VG_region_graphs(const Json::Value& v,
					       DataBase *p) {
    image_id = v["image_id"].asInt64();
      
    const Json::Value& rl = v["regions"];
    for (size_t k=0; k<rl.size(); k++) {
      const Json::Value& r = rl[(int)k];
      region rr;
      rr.region_id = r["region_id"].asInt64();
      rr.image_id  = r["image_id" ].asInt64();
      rr.x = r["x"].asInt64();
      rr.y = r["y"].asInt64();
      rr.width  = r["width" ].asInt64();
      rr.height = r["height"].asInt64();
      rr.phrase = r["phrase"].asString(); //obs! clean spaces

      const Json::Value& ol = r["objects"];
      for (size_t i=0; i<ol.size(); i++) {
	const Json::Value& o = ol[(int)i];
	object oo;
	oo.object_id = o["object_id"].asInt64();
	oo.x = o["x"].asInt64();
	oo.y = o["y"].asInt64();
	oo.w = o["w"].asInt64();
	oo.h = o["h"].asInt64();
	oo.name = o["name"].asString();
	for (auto j=o["synsets"].begin(); j!=o["synsets"].end(); j++)
	  oo.synsets.push_back(j->asString()); //obs! clean spaces
	rr.objects.push_back(oo);
      }
      
      const Json::Value& sl = r["synsets"];
      for (size_t i=0; i<sl.size(); i++) {
	const Json::Value& s = sl[(int)i];
	synset ss;
	ss.entity_idx_start = s["entity_idx_start"].asInt64();
	ss.entity_idx_end   = s["entity_idx_end"  ].asInt64();
	ss.entity_name = s["entity_name"].asString();
	ss.synset_name = s["synset_name"].asString();
	rr.synsets.push_back(ss);
      }
      
      const Json::Value& rl = r["relationships"];
      for (size_t i=0; i<rl.size(); i++) {
	const Json::Value& r = rl[(int)i];
	relationship rs;
	rs.relationship_id = r["relationship_id"].asInt64();
	rs.subject_id      = r["subject_id"     ].asInt64();
	rs.object_id       = r["object_id"      ].asInt64();
	rs.predicate       = r["predicate"].asString();
	for (auto j=r["synsets"].begin(); j!=r["synsets"].end(); j++)
	  rs.synsets.push_back(j->asString()); //obs! clean spaces
	rr.relationships.push_back(rs);
      }
      
      regions.push_back(rr);
    }

    image_id = regions.front().image_id;
    
    if (p)
      idx = p->LabelIndex(p->ZeroPadToLabel(ToStr(image_id)));
    else
      idx = -1;

    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_region_graphs::str() const {
    stringstream ss;
    ss << "idx=" << (int)idx << " image_id=" << image_id << " regions=[";
    for (auto i=regions.begin(); i!=regions.end(); i++) {
      ss << "(region_id=" << i->region_id << " image_id=" << i->image_id
	 << " x=" << i->x << " y=" << i->y
	 << " width=" << i->width << " height=" << i->height
	 << " phrase=" << i->phrase << " objects=[";
      for (auto j=i->objects.begin(); j!=i->objects.end(); j++)
	ss << "(object_id=" << j->object_id << " name=" << j->name 
	   << " synsets=" << CommaJoin(j->synsets)
	   << " x=" << j->x << " y=" << j->y
	   << " w=" << j->w << " h=" << j->h << ")";
      ss << "] synsets=[";
      for (auto j=i->synsets.begin(); j!=i->synsets.end(); j++)
	ss << "(entity_name=" << j->entity_name 
	   << " synset_name=" << j->synset_name 
	   << " entity_idx_start=" << j->entity_idx_start 
	   << " entity_idx_end=" << j->entity_idx_end << ")";
      ss << "] relationships=[";
      for (auto j=i->relationships.begin(); j!=i->relationships.end(); j++)
	ss << "(relationship_id=" << j->relationship_id
	   << " subject_id=" << j->subject_id 
	   << " object_id=" << j->object_id 
	   << " predicate=" << j->predicate 
	   << " synsets=" << CommaJoin(j->synsets) << ")";
      ss << "])";
    }
    ss << "]";

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  DataBase::VG_scene_graphs::VG_scene_graphs(const Json::Value& v,
					     DataBase *p) {
    image_id = v["image_id"].asInt64();
      
    const Json::Value& ol = v["objects"];
    for (size_t i=0; i<ol.size(); i++) {
      const Json::Value& o = ol[(int)i];
      object oo;
      oo.object_id = o["object_id"].asInt64();
      oo.x = o["x"].asInt64();
      oo.y = o["y"].asInt64();
      oo.w = o["w"].asInt64();
      oo.h = o["h"].asInt64();
      for (auto j=o["names"].begin(); j!=o["names"].end(); j++)
	oo.names.push_back(j->asString()); //obs! clean spaces
      for (auto j=o["synsets"].begin(); j!=o["synsets"].end(); j++)
	oo.synsets.push_back(j->asString()); //obs! clean spaces
      for (auto j=o["attributes"].begin(); j!=o["attributes"].end(); j++)
	oo.attributes.push_back(j->asString()); //obs! clean spaces
      objects.push_back(oo);
    }
      
    const Json::Value& rl = v["relationships"];
    for (size_t i=0; i<rl.size(); i++) {
      const Json::Value& r = rl[(int)i];
      relationship rs;
      rs.relationship_id = r["relationship_id"].asInt64();
      rs.subject_id      = r["subject_id"     ].asInt64();
      rs.object_id       = r["object_id"      ].asInt64();
      rs.predicate       = r["predicate"].asString();
      for (auto j=r["synsets"].begin(); j!=r["synsets"].end(); j++)
	rs.synsets.push_back(j->asString()); //obs! clean spaces
      relationships.push_back(rs);
    }
    
    if (p)
      idx = p->LabelIndex(p->ZeroPadToLabel(ToStr(image_id)));
    else
      idx = -1;

    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_scene_graphs::str() const {
    stringstream ss;
    ss << "idx=" << (int)idx << " image_id=" << image_id << " objects=[";
    for (auto j=objects.begin(); j!=objects.end(); j++)
      ss << "(object_id=" << j->object_id << " names=" << CommaJoin(j->names) 
	 << " synsets=" << CommaJoin(j->synsets)
	 << " attributes=" << CommaJoin(j->attributes)
	 << " x=" << j->x << " y=" << j->y
	 << " w=" << j->w << " h=" << j->h << ")";
    ss << "] relationships=[";
    for (auto j=relationships.begin(); j!=relationships.end(); j++)
      ss << "(relationship_id=" << j->relationship_id
	 << " subject_id=" << j->subject_id 
	 << " object_id=" << j->object_id 
	 << " predicate=" << j->predicate 
	 << " synsets=" << CommaJoin(j->synsets) << ")";
    ss << "])";
    
    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

#if defined(HAVE_JSON_JSON_H)
  DataBase::VG_synsets::VG_synsets(const Json::Value& v, DataBase *p) {
    synset_name       = v["synset_name"].asString();
    synset_definition = v["synset_definition"].asString();
    db  = p;
  }
#endif // HAVE_JSON_JSON_H

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::VG_synsets::str() const {
    stringstream ss;
    ss << "synset_name=" << synset_name
       << " synset_definition=" << synset_definition;
    
    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  const DataBase::VG_attributes::attribute& 
  DataBase::VG_get_attribute(size_t id) { 
    for (auto i=vg_attributes.begin(); i!=vg_attributes.end(); i++) {
      const VG_attributes& r = i->second;
      for (auto j=r.attributes.begin(); j!=r.attributes.end(); j++)
	if (j->object_id==id)
	  return *j;
    }

    static const VG_attributes::attribute empty;

    return empty;
  }

  /////////////////////////////////////////////////////////////////////////////

  const DataBase::VG_relationships::relationship&
  DataBase::VG_get_relationship(size_t id) {
    string msg = "DataBase::VG_get_relationship("+ToStr(id)+") : ";
    // WriteLog(msg+"starting");

    for (auto i=vg_relationships.begin(); i!=vg_relationships.end(); i++) {
      const VG_relationships& r = i->second;
      for (auto j=r.relationships.begin(); j!=r.relationships.end(); j++)
	if (j->relationship_id==id)
	  return *j;
    }

    static const VG_relationships::relationship empty;

    return empty;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<pair<string,float> > 
  DataBase::VG_stats(const string& a, bool show) {
    string msg = "DataBase::VG_stats("+a+","+ToStr(show)+") : ";

    vector<pair<string,float> > ret;
    
    if (a=="objects") {
      size_t n = 0;
      map<string,size_t> o;
      for (auto i=vg_objects.begin(); i!=vg_objects.end(); i++)
	for (auto j=i->second.objects.begin(); j!=i->second.objects.end(); j++)
	  for (auto k=j->synsets.begin(); k!=j->synsets.end(); k++) {
	    o[*k]++;
	    n++;
	  }
      if (n) {
	multimap<size_t,string> c;
	for (auto i=o.begin(); i!=o.end(); i++)
	  c.insert(make_pair(i->second, i->first));
	for (auto i=c.rbegin(); i!=c.rend(); i++) {
	  ret.push_back(make_pair(i->second, float(i->first)/n));
	  if (show)
	    cout << i->second << " " << i->first << " " << float(i->first)/n
		 << endl;
	}
      }
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<size_t> 
  DataBase::VG_find(const string& a, const string& b,
		    const string& c, const string& din, 
		    const map<string,pair<string,float> >& /*attr*/) {
    string msg = "DataBase::VG_find("+a+","+b+","+c+","+din+") : ";
    // WriteLog(msg+"starting");

    vector<size_t> res;

    vector<string> dd = SplitInSomething("|", true, din);

    if (a=="relationships") {
      for (auto i=vg_relationships.begin(); i!=vg_relationships.end(); i++) {
	const VG_relationships& r = i->second;
	for (auto j=r.relationships.begin(); j!=r.relationships.end(); j++) {
	  if (b=="object") {
	    const VG_relationships::object_t *o = NULL;
	    if (c=="subject")
	      o = &j->subject;
	    if (c=="object")
	      o = &j->object;
	    if (o) {
	      for (auto d=dd.begin(); d!=dd.end(); d++) {
		auto p = find(o->synsets.begin(), o->synsets.end(), *d);
		if (p!=o->synsets.end())
		  res.push_back(j->relationship_id);
	      }
	    }
	  }
	  if (b=="predicate") {
	    for (auto d=dd.begin(); d!=dd.end(); d++) {
	      auto p = find(j->synsets.begin(), j->synsets.end(), *d);
	      if (p!=j->synsets.end())
		res.push_back(j->relationship_id);
	    }
	  }
	}
      }
    }
    
    WriteLog(msg+"ending with "+ToStr(res.size())+" relations found");

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::VG_attrib_match(const string& t, 
				 const VG_relationships::relationship& r,
				 const string& so,
				 const pair<string,pair<string,float> >& g) {
    if (t!="attributes")
      return false;
    
    const VG_relationships::object_t *o = NULL;
    if (so=="subject")
      o = &r.subject;
    if (so=="object")
      o = &r.object;
    if (!o)
      return false;
    
    size_t oid = o->object_id;
    const VG_attributes::attribute& aa = VG_get_attribute(oid);
    for (auto a=aa.attributes.begin(); a!=aa.attributes.end(); a++)
      if (*a==g.second.first)
	return true;

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<pair<size_t,double> > 
  DataBase::FindByGraph(const string& /*otype*/, const graph& g, size_t /*n*/) {
    string msg = "DataBase::FindByGraph(...) : ";
    WriteLog(msg+"starting");

    bool show_all = false;

    vector<pair<size_t,double> > res;

    vector<vector<size_t> > m;
    for (size_t i=0; i<g.size(); i++) {
      const graph_item& gi = g[i];
      m.push_back(vector<size_t>());
      if (gi.type==graph_item::node)
	m[i] = VG_find("relationships", "object", gi.name, gi.content, gi.attrib);
      if (gi.type==graph_item::edge)
	m[i] = VG_find("relationships", "predicate", gi.name, gi.content, gi.attrib);

      cout << gi.name << " " << gi.content << " " << m[i].size() << " found"
	// << ToStr(m[i], " ")
	   << endl;
      if (show_all)
	for (size_t j=0; j<m[i].size(); j++) {
	  const VG_relationships::relationship& r =
	    VG_get_relationship(m[i][j]);
	  cout << r.str() << endl;
	}
    }

    if (false) {
      set<size_t> o(m[0].begin(), m[0].end());
      for (size_t i=1; i<m.size(); i++) {
	set<size_t> n;
	for (auto j=m[i].begin(); j!=m[i].end(); j++)
	  if (o.find(*j)!=o.end())
	    n.insert(*j);
	o = n;
      }
      
      cout << "intersection size " << o.size() << endl;
      for (auto i=o.begin(); i!=o.end(); i++)
	cout << VG_get_relationship(*i).str() << endl;
    }

    map<size_t,float> val;
    for (size_t i=0; i<m.size(); i++)
      for (auto j=m[i].begin(); j!=m[i].end(); j++) {
	val[*j] += g[i].weight;
	auto r = VG_get_relationship(*j);
	for (auto k=g[i].attrib.begin(); k!=g[i].attrib.end(); k++)
	  if (VG_attrib_match("attributes", r, g[i].name, *k)) {
	    cout << "ATTRIB " << k->first << " " << k->second.first
		 << " " << k->second.second << endl;
	    val[*j] += k->second.second;
	  }
      }

    multimap<float,size_t> ord;
    for (auto i=val.begin(); i!=val.end(); i++)
      ord.insert(make_pair(i->second, i->first));

    map<float,size_t> cnt;
    for (auto i=ord.begin(); i!=ord.end(); i++)
      cnt[i->first]++;

    for (auto i=cnt.rbegin(); i!=cnt.rend(); i++)
      cout << i->first << " x " << i->second << " times" << endl;

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  DataBase::graph
  DataBase::ReadGraph(const string& fname) {
    string msg = "DataBase::FindByGraph("+fname+") : ";
    WriteLog(msg+"starting");

    graph res;
    size_t idx = 0;

    string fs = FileToString(fname);
    vector<string> ll = SplitInSomething("\n", true, fs);
    for (auto l=ll.begin(); l!=ll.end(); l++) {
      if (*l=="" || (*l)[0]=='#')
	continue;

      graph_item item;
      item.type   = graph_item::undef;
      item.idx    = idx++;
      item.weight = 1.0;

      string t = *l;
      StripWhiteSpaces(t);
      vector<string> tt = SplitInSpaces(t);
      for (auto ti=tt.begin(); ti!=tt.end(); ti++)
	if (*ti=="node")
	  item.type = graph_item::node;
	else if (*ti=="edge")
	  item.type = graph_item::edge;
	else if (ti->find('=')!=string::npos) {
	  auto kv = SplitKeyEqualValueNew(*ti);
	  if (kv.first=="name")
	    item.name = kv.second;
	  else if (kv.first=="content")
	    item.content = kv.second;
	  else if (kv.first=="weight")
	    item.weight = atof(kv.second.c_str());
	  else if (kv.first=="attrib") {
	    size_t p = kv.second.find(':');
	    if (p==string::npos) {
	      ShowError(msg+"no : found in <"+*ti+">");
	      return graph();
	    }
	    string a = kv.second.substr(0, p);
	    string b = kv.second.substr(p+1);
	    float  w = 1.0;
	    p = b.find(',');
	    if (p!=string::npos) {
	      w = atof(b.substr(p+1).c_str());
	      b.erase(p);
	    }
	    item.attrib[a] = make_pair(b, w);
	  } else if (kv.first=="links") {
	    vector<string> vs = SplitInCommas(kv.second);
	    for (auto i=vs.begin(); i!=vs.end(); i++)
	      item.links.push_back(atoi(i->c_str()));
	  } else {
	    ShowError(msg+"failed to process <"+kv.first+"> in line <"+t+">");
	    return graph();
	  }
	} else {
	  ShowError(msg+"failed to process <"+*ti+"> in line <"+t+">");
	  return graph();
	}
      
      res.push_back(item);
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,imagedata> > DataBase::ObjectMasksCombined(size_t idx) {
    list<pair<string,imagedata> > ret, sep = ObjectMasks(idx);

    for (auto i=sep.begin(); i!=sep.end(); i++) {
      imagedata *img = NULL;
      for (auto j=ret.begin(); !img && j!=ret.end(); j++)
	if (j->first==i->first)
	  img = &j->second;
      if (!img) {
	ret.push_back(*i);
	continue;
      }
      for (size_t x=0; x<img->width(); x++)
	for (size_t y=0; y<img->height(); y++)
	  if (i->second.get_one_uchar(x, y))
	    img->set(x, y, (unsigned char)255);
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,imagedata> > DataBase::ObjectMasks(size_t idx) {
    list<pair<string,imagedata> > ret;

#if defined(HAVE_JSON_JSON_H) && defined(PICSOM_USE_OPENCV)
    map<string,string> info = ReadOriginsInfo(idx, false, true);
    size_t w = atoi(info["width"].c_str()), h = atoi(info["height"].c_str());
    imagedata empty(w, h, 1, imagedata::pixeldata_uchar);
    
    auto r = COCOmasks(idx);
    for (auto i=r.begin(); i!=r.end(); i++) {
      for (auto j=i->second.begin(); j!=i->second.end(); j++) {
	//for (auto k=j->begin(); k!=j->end(); k++) {
	  vector<cv::Point> contour;
	  for (auto l=j->begin(); l!=j->end(); l++)
	    contour.push_back(cv::Point((int)floor(l->first+ 0.5),
					(int)floor(l->second+0.5)));
	  imagedata img = empty;
	  for (size_t x=0; x<w; x++)
	    for (size_t y=0; y<h; y++)
	      if (cv::pointPolygonTest(contour, cv::Point2f(x, y), false)>=0)
		img.set(x, y, (unsigned char)255);
	  ret.push_back(make_pair(i->first, img));
	}
    }

#else
    idx = idx+1;
    ShowError("DataBase::ObjectMasks() : HAVE_JSON_JSON_H undef");
#endif // HAVE_JSON_JSON_H && PICSOM_USE_OPENCV

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::GenerateSubtitles(size_t idx, const string& type,
				   const string& spec, const string& file) {
    string err = " DataBase::GenerateSubtitles("+ToStr(idx)+","+type+","
      +spec+","+file+") : ";

    if (type!="ass" && type!="vtt")
      return ShowError(err+"susbtitle type \""+type+"\" not understood");

    bool debug1 = true, debug2 = true, debug3 = false;

    bool show_segm_id = true, show_prob = false;
    bool use_sec = false; // obs!
    float fps = VideoFrameRate(idx);
    //if (!fps)
    //  return ShowError(err+"zero frame rate");

    string vlab = Label(idx);

    float thr = 0;
    vector<string> detections, sspecs;
    string lscom;

    vector<string> param = SplitInSomething(";", false, spec);
    for (auto i=param.begin(); i!=param.end(); i++)
      if (i->find('=')) {
	pair<string,string> kv = SplitKeyEqualValueNew(*i);
	if (kv.first=="segm")
	  sspecs = SplitInCommas(kv.second);
	else if (kv.first=="detections")
	  detections = SplitInCommas(kv.second);
	else if (kv.first=="thr")
	  thr = atof(kv.second.c_str());
	else if (kv.first=="showsegm")
	  show_segm_id = IsAffirmative(kv.second);
	else if (kv.first=="showprob")
	  show_prob = IsAffirmative(kv.second);
	else if (kv.first=="lscom")
	  lscom = kv.second;
	else
	  return ShowError(err+"\""+*i+"\" not understood");
      }

    string vfile = ObjectTypeAndPath(vlab, target_video);
    if (vfile=="")
      return ShowError(err+"label <"+vlab+"> is not an existing video file");

    string vfile_x = vfile;
    vfile = "";
      
    const object_info *oi = FindObject(vlab);
    oi->dump();

    // commented out 2015-4-14
    // WriteLog("Object <"+vlab+"> has "+
    // 	     ToStr(oi->children.size())+" children (before)");
    // MakeSubObjectIndex();
    // WriteLog("Object <"+vlab+"> has "+
    // 	     ToStr(oi->children.size())+" children (after)");

    float fontsize = 80;
    ofstream ox(file.c_str());
    if (type=="ass") {
    ox << "[Script Info]" << endl
       << "Title:" << endl
       << "Original Script:" << endl
       << "Original Translation:" << endl
       << "Original Editing:" << endl
       << "Original Timing:" << endl
       << "Original Script Checking:" << endl
       << "ScriptType: v4.00+" << endl
       << "Collisions: Normal" << endl
       << "PlayResY: 1024" << endl
       << "PlayDepth: 0" << endl
       << "Timer: 100,0000" << endl
       << "[V4+ Styles]" << endl
       << "Format:"
       << " Name, Fontname, Fontsize, PrimaryColour, SecondaryColour,"
       << " OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut,"
       << " ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow,"
       << " Alignment, MarginL, MarginR, MarginV, Encoding" << endl;

    string s1 = ",DejaVu Sans,"+ToStr(fontsize)+",";
    string s2 = ",&H00B4FCFC,&H00000000,&H00000000,"
      "0,0,0,0,100,100,0.00,0.00,1,2.50,0.00,";
    string s4 = ",&H00B4FCFC,&H000000FF,&H00000000,"
      "0,0,0,0,100,100,0.00,0.00,1,2.50,0.00,";
    string s3 = ",50,50,50,0";
    ox << "Style: subtitle" << s1 << "&H00FFFFFF" << s2 << 2 << s3 << endl
       << "Style: concepts" << s1 << "&H0000C000" << s2 << 7 << s3 << endl
       << "Style: personte" << s1 << "&H00C00000" << s2 << 6 << s3 << endl
       << "Style: personfa" << s1 << "&H00C000C0" << s2 << 6 << s3 << endl
       << "Style: personvo" << s1 << "&H00C000C0" << s2 << 4 << s3 << endl
       << "Style: facebox"  << s1 << "&HFF000000" << s4 << 7 << s3 << endl
       << "Style: link"     << s1 << "&H000000FF" << s2 << 4 << s3 << endl
       << "[Events]" << endl
       << "Format: Layer, Start, End, Style, Name,"
      " MarginL, MarginR, MarginV,Effect, Text" << endl;
    }

    if (type=="vtt")
      ox << "WEBVTT" << endl << endl
	 << "NOTE this is a comment..." << endl << endl;

    for (size_t si=0; si<sspecs.size(); si++) {
      string segmentspec = sspecs[si];

      if (segmentspec=="links") {
	float duration = 5;

	string lfile = SolveObjectPath(vlab);
	size_t p = lfile.rfind('.');
	if (p==string::npos)
	  lfile = "";
	else {
	  lfile.erase(p);
	  p = lfile.rfind('/');
	  if (p==string::npos)
	    lfile = "";
	  else {
	    string s = lfile.substr(p);
	    lfile += ".d"+s+".links";
	  }
	}
	if (FileExists(lfile)) {
	  WriteLog("Processing LINKS file <"+ShortFileName(lfile)+">");
	  list<textline_t> aline = ReadOSRS(lfile);
	  for (auto i=aline.begin(); i!=aline.end(); i++) {
	    string txt = i->txt_val[0].first;

	    if (type=="ass")
	      ox << "Dialogue: 0," << DataBase::ASStimeStr(i->start) << ","
		 << DataBase::ASStimeStr(i->start+duration)
		 << ",link,foo,000,000,000,," << txt << endl;

	    if (type=="vtt")
	      ox << "NOTE links not implemented..." << endl << endl;
	  }
	}
	continue;
      }

      string metaclassname = segmentspec, metaclasssegm;
      size_t p = metaclassname.find(':');
      if (p!=string::npos) {
	metaclasssegm = metaclassname.substr(0, p);
	metaclassname.erase(0, p+1);
      }

      if (IsMetaClassFile(metaclassname)) {
	WriteLog("Processing METACLASSFILE file <"+
		 ShortFileName(metaclassname)+"> with segments <"+
		 metaclasssegm+">");
	  
	list<textline_t> asrcooked;
	ground_truth_set clist;
	if (detections.size()==0)
	  clist = GroundTruthExpressionListNew(metaclassname);
	else {
	  list<string> ls = SplitClassNames(metaclassname);
	  for (auto i=ls.begin(); i!=ls.end(); i++) {
	    ground_truth gt;
	    gt.label(*i);
	    clist.insert(gt);
	  }
	}

	list<pair<size_t,pair<double,double> > > idxtpdur;

	if (metaclasssegm!="") {
	  // obs! this is replicated some lines down...
	  string re = "$re(^"+metaclasssegm+":"+vlab+":)";
	  ground_truth gt = GroundTruthFunction(re, target_videosegment);
	  vector<size_t> idxs = gt.indices(1);
	  for (size_t i=0; i<idxs.size(); i++) {
	    auto psd = ParentStartDuration(idxs[i], target_videofile);
	    if (psd.first!=idx)
	      return ShowError(err+"parent problem");
	    double tp = psd.second.first, dur = psd.second.second;
	    idxtpdur.push_back(make_pair(idxs[i], make_pair(tp, dur)));
	  }

	} else
	  for (size_t f_or_sec=0; f_or_sec<1000000; f_or_sec++) { // obs!
	    string label = vlab+":"+(use_sec?"s":"")+ToStr(f_or_sec);
	    int idx = LabelIndexGentle(label, false);
	    if (debug1)
	      cout << label << " #" << idx;

	    if (idx<0) {
	      if (debug1)
		cout << endl;
	      break; // was continue;
	    }
	    
	    double tp  = f_or_sec / (use_sec ? 1 : fps);
	    double dur = use_sec ? 1 : 1/fps;
	    idxtpdur.push_back(make_pair(idx, make_pair(tp, dur)));
	  }

	for (auto si=idxtpdur.begin(); si!=idxtpdur.end(); si++) {
	  string txt;
	  if (show_segm_id)
	    txt = Label(si->first)+" :";

	  map<float,string> valtxt;
	  for (size_t a=0; a<clist.size(); a++) {
	    if (clist[a].index_ok(idx) && clist[a][idx]==1)
	      txt += (txt==""?"":" ")+clist[a].label();

	    if (detections.size()==1) {
	      string detname = detections.front();
	      detname += "#"+clist[a].label();

	      bool angry = false, dummy = true;
	      map<string,vector<float> > dets
		= RetrieveDetectionData(si->first, detname, angry, dummy);

	      if (dets.size()==1) {
		if (dets.begin()->second.size()!=1)
		  return ShowError(err+"dets.begin()->second.size()!=1");
		    
		string labo = clist[a].label(), lab = labo;

		float val = dets.begin()->second[0];
		if (debug3)
		  cout << " " << labo << "=" << val;

		if (val>thr) {
		  if (lab.substr(0, 5)=="lscom") {
		    lab = LscomName(lab, true, lscom);
		    if (lscom=="" || lscom.find("trecvid2011")!=string::npos) {
		      pair<string,string> lp = DataBase::SplitLscomName(lab);
		      lab = lp.first;
		    }
		  }
		  if (lab.size()==9 && lab[0]=='n' &&
		      lab.find_first_not_of("0123456789", 1)==string::npos) {
		    lab = ImageNetName(lab, true);
		    vector<string> labv = SplitImageNetName(lab);
		    lab = labv[0];
		  }
		
		  if (debug2)
		    cout << " *" << labo << "/" << lab << "=" << val << "*";

		  string txt = lab;
		  if (show_prob) {
		    char valbuf[100];
		    sprintf(valbuf, " (%1.3f)", val);
		    txt += valbuf;
		  }
		  valtxt.insert(make_pair(val, txt));
		}
	      }
	    }
	  }

	  for (auto i=valtxt.rbegin(); i!=valtxt.rend(); i++)
	    txt += (txt==""?"":" ")+i->second;

	  if (txt!="") {
	    textline_t aline;
	    aline.start = si->second.first;
	    aline.end   = aline.start+si->second.second;
	    aline.add(txt, 0);
	    asrcooked.push_back(aline);

	    if (debug1 && !debug2)
	      cout << " " << txt.substr(0, 50);
	  }

	  if (debug1)
	    cout << endl;
	}

	if (asrcooked.empty()) {
	  textline_t aline;
	  aline.start = 0;
	  aline.end   = 10;
	  aline.set("no child frames nor segments", 0);
	  asrcooked.push_back(aline);
	}

	for (auto i=asrcooked.begin(); i!=asrcooked.end(); i++) {
	  string txt = i->get_text();

	  if (type=="ass")
	    ox << "Dialogue: 0," << DataBase::ASStimeStr(i->start) << ","
	       << DataBase::ASStimeStr(i->end)
	       << ",concepts,foo,000,000,000,," << txt << endl;
	      
	  if (type=="vtt")
	    ox << VTTtimeStr(i->start) << " --> " << VTTtimeStr(i->end)
	       << endl
	       << txt << endl << endl;
	}
	continue;
      } // IsMetaClassFile(metaclassname) ends... 

      // this should be harmonized with AnalyseElanize() if implemented there
      if (segmentspec.find("text|")==0 || segmentspec.find("text:")==0) {
	vector<string> sp = SplitInSomething("|", false, segmentspec);
	if (sp.size()!=3)
	  return ShowError(err+"sp.size()!=3 for <"+segmentspec+">");
	string txtposition = "concepts";
	size_t p = sp[0].find(':');
	if (p!=string::npos)
	  txtposition = sp[0].substr(p+1);
	if (txtposition!="concepts" && txtposition!="subtitle" &&
	    txtposition!="personte" && txtposition!="personfa" &&
	    txtposition!="personvo" && txtposition!="facebox" &&
	    txtposition!="link")
	  return ShowError(err+"txtposition <"+txtposition+"> unknown");

	string textindex = sp[1], field = "text", segm = sp[2];
	p = textindex.find(':');
	if (p!=string::npos) {
	  field = textindex.substr(p+1);
	  textindex.erase(p);
	}

	cout << "[" << segmentspec << "] => [" << textindex << "]["
	     << field << "][" << segm << "] -> [" << txtposition << "]"
	     << endl; 

	list<textline_t> txtlines;

	if (HasTextIndex(textindex)) {	
	  string asegm = segm;
	  if (asegm[0]=='*')
	    asegm.erase(0, 1);
	  // obs! this was replicated some lines up...
	  // string re = "$re(^"+asegm+":"+vlab+":)";
	  string prefix_col, col_suffix = ":";
	  if (asegm[0]==':')
	    col_suffix = asegm;
	  else
	    prefix_col = asegm;
	  if (prefix_col!="")
	    prefix_col += ":";
	  string re = "$re(^"+prefix_col+vlab+col_suffix+")";

	  ground_truth gt = GroundTruthFunction(re, target_videosegment);
	  vector<size_t> idxs = gt.indices(1);
	  for (size_t i=0; i<idxs.size(); i++) {
	    auto psd = ParentStartDuration(idxs[i], target_videofile);
	    if (psd.first!=idx)
	      return ShowError(err+"parent problem");
	    double tp = psd.second.first, dur = psd.second.second;
	    //idxtpdur.push_back(make_pair(idxs[i], make_pair(tp, dur)));

	    size_t sidx = idxs[i];
	    if (segm[0]=='*')
	      sidx = VideoOrSegmentMiddleFrame(sidx).first;
	    list<pair<string,string> > td = TextIndexRetrieve(sidx,
							      textindex);
	    for (auto ti=td.begin(); ti!=td.end(); ti++)
	      if (ti->first==field) {
		string txt = ti->second;
		size_t p = txt.find(" (");
		if (p!=string::npos)
		  txt.erase(p);
		textline_t aline;
		aline.start = tp;
		aline.end   = tp+dur;
		aline.set(txt, 0);
		txtlines.push_back(aline);
	      }
	  }

	} else if (segm=="") {  // texts are in 00.d/00-textindex.txt
	  bool raw = textindex=="raw";
	  string tsext = raw ? field : textindex;
	  string tf = ObjectTextFileSubdirPath(idx, "", tsext);
	  if (!FileExists(tf))
	    return ShowError(err+"<"+sp[1]+"> is neither a textindex nor "
			     "a timestamped line file, file <"+tf
			     +"> inexistent");
	  string asrfile;
	  txtlines = raw ? OSRScooked(tf, asrfile) : ReadOSRS(tf);

  	} else { // texts are in segm:tN-segm.d/segm:00:tN-segm.txt
	  // obs! this is replicated some lines up...
	  string re = "$re(^"+segm+":"+vlab+":)";
	  ground_truth gt = GroundTruthFunction(re, target_videosegment);
	  vector<size_t> idxs = gt.indices(1);
	  for (size_t i=0; i<idxs.size(); i++) {
	    string tf = ObjectTextFileSubdirPath(idxs[i], "", textindex);
	    textline_t tl = ObjectTextLineRetrieve(idxs[i], "",
						   textindex, true);
	    // textline_t tlmod = tl;
	    // if (!tlmod.empty())
	    //   tlmod.txt = tlmod.txt_val[0].first;
	    txtlines.push_back(tl);
	  }
	}

	for (auto ai=txtlines.begin(); ai!=txtlines.end(); ai++) {
	  // should this be inside ReadOSRS()?
	  string text = ai->get_text();
	  if (!show_prob) {
	    size_t p = text.rfind(" (");
	    if (p!=string::npos) {
	      size_t q = text.find_first_not_of("0123456789.+-e", p+2);
	      if (q==text.size()-1 && text[q]==')')
		text.erase(p);
	    }
	  }

	  float aistart = ai->start, aiend = ai->end;
	  if (aistart==0 && aiend==0)
	    aiend = VideoDuration(idx);

	  if (type=="ass")
	    ox << "Dialogue: 0," << DataBase::ASStimeStr(aistart) << ","
	       << DataBase::ASStimeStr(aiend)
	       << ","+txtposition+",foo,000,000,000,," << text << endl;
	      
	  if (type=="vtt")
	    ox << VTTtimeStr(aistart) << " --> " << VTTtimeStr(aiend) << endl
	       << text << endl << endl;
	}
	
	continue;
      }

      // if (segmentspec=="dia") {
      // 	string diafile = vfile_x;
      // 	size_t p = diafile.rfind('.');
      // 	size_t q = diafile.rfind('/', p);
      // 	diafile.erase(p+1);
      // 	diafile += "d"+diafile.substr(q, p-q+1)+"dia";
      // 	list<textline_t> dia = ReadOSRS(diafile);
      // 	WriteLog("Processing DIA file <"+ShortFileName(diafile)+">");
      // 	for (auto i=dia.begin(); i!=dia.end(); i++) {
      // 	  string txt = i->txt; // , t = txt.substr(0, 1);
      // 	  // if (t[0]>='a' && t[0]<='z')
      // 	  //   txt[0] = t[0]-32;
      // 	  // txt += ".";
	
      // 	  if (type=="ass")
      // 	    ox << "Dialogue: 0," << DataBase::ASStimeStr(i->start) << ","
      // 	       << DataBase::ASStimeStr(i->end)
      // 	       << ",personvo,foo,000,000,000,," << txt << endl;

      // 	  if (type=="vtt")
      // 	    ox << "NOTE dia not implemented yet..." << endl << endl;
      // 	}
      // 	continue;
      // }

      // if (segmentspec=="asr") {
      // 	string asrfile;
      // 	list<textline_t> asrcooked = OSRScooked(vfile_x, asrfile);
      // 	if (asrfile!="") {
      // 	  WriteLog("Processing OSRS file <"+ShortFileName(asrfile)+">");
      // 	  for (auto i=asrcooked.begin(); i!=asrcooked.end(); i++) {
      // 	    string txt = i->txt; // , t = txt.substr(0, 1);
      // 	    // if (t[0]>='a' && t[0]<='z')
      // 	    //   txt[0] = t[0]-32;
      // 	    // txt += ".";

      // 	    if (type=="ass")
      // 	      ox << "Dialogue: 0," << DataBase::ASStimeStr(i->start) << ","
      // 		 << DataBase::ASStimeStr(i->end)
      // 		 << ",subtitle,foo,000,000,000,," << txt << endl;

      // 	    if (type=="vtt")
      // 	      ox << VTTtimeStr(i->start) << " --> " << VTTtimeStr(i->end)
      // 		 << endl
      // 		 << txt << endl << endl;
      // 	  }
      // 	}
      // 	continue;
      // }

      int idx = LabelIndex(vlab);
      string sfile = SolveSegmentFilePath(idx, segmentspec, true);
      if (sfile=="")
	return ShowError(err+"segmentfile ["+segmentspec+"] for <"+
			 vlab+"> not found");
	
      WriteLog("Opening segmentfile <"+ShortFileName(sfile)+">");

      segmentfile segm(vfile, sfile, NULL, NULL, NULL, true, true, false);

      size_t nframes = segm.getNumFrames();
      SegmentationResultList *results = segm.readFileResultsFromXML("","");
      set<int> flist = segm.listProcessedFrames();

      WriteLog("Video file <"+ShortFileName(vfile_x)+"> contains "+
	       ToStr(nframes)+" frames of which "+ToStr(flist.size())+
	       " have been processed with <"+segmentspec+">");

      for (set<int>::const_iterator it=flist.begin(); it!=flist.end(); it++) {
	int f = *it;
	string fs = segm.FrameSpec(f);
	if (debug1)
	  cout << "FRAME [" << f << "] \"" << fs << "\" RESULTS of <"
	       << segmentspec << "> :" << endl;
	results = segm.readFrameResultsFromXML(f, "", "");

	string cons;
	for (SegmentationResultList::const_iterator it=results->begin();
	     it!=results->end(); it++) {
	  if (debug1) {
	    cout << "  NAME:  "   << it->name 	<< endl;
	    cout << "  TYPE:  "   << it->type 	<< endl;
	    cout << "  VALUE: "   << it->value	<< endl;
	    cout << "  METHODID:" << it->methodid << endl;
	    cout << "  RESULTID:" << it->resultid << endl << endl;
	  }

	  if (it->type=="detection") {
	    cons += (cons!=""?" ":"")+it->value;
	    continue;
	  }

	  if (it->type!="box" || it->name.substr(0, 4)!="face" ||
	      fs.substr(0, 2)!=":s")
	    continue;

	  double sec0 = atof(fs.substr(2).c_str()), sec1 = sec0+0.99;

	  float mul = 1024.0/segm.getHeight();
	  vector<string> vs = SplitInSpaces(it->value);
	  vector<int> v(vs.size());
	  for (size_t z=0; z<v.size(); z++)
	    v[z] = (int)floor(mul*atoi(vs[z].c_str())+0.5);

	  // http://en.wikipedia.org/wiki/SubStation_Alpha
	  // http://aegisub.cellosoft.com/docs/ASS_Tagsnmen
	
	  if (type=="ass") {
	    ox << "Dialogue: 1," << DataBase::ASStimeStr(sec0) << ","
	       << DataBase::ASStimeStr(sec1) << ",facebox,foo,000,000,000,,"
	       << "{\\pos(0,0)\\p1}m " << v[0] << " " << v[1]
	       << " l " << v[2] << " " << v[1] << " " << v[2] << " " << v[3]
	       << " " << v[0] << " " << v[3] << "{\\p0}" << endl;

	    if (debug2) {
	      ox << "Dialogue: 1," << DataBase::ASStimeStr(sec0) << ","
		 << DataBase::ASStimeStr(sec1) << ",facebox,foo,000,000,000,,"
		 << "{\\pos(0,0)\\p1}m 0 0 l 100 0 100 100 0 100{\\p0}" 
		 << endl;
	      ox << "Dialogue: 1," << DataBase::ASStimeStr(sec0) << ","
		 << DataBase::ASStimeStr(sec1) << ",facebox,foo,000,000,000,,"
		 << "{\\pos(0,0)\\p1}m 200 200 l 300 200 300 300 200 300{\\p0}"
		 << endl;
	    }

	  if (type=="vtt")
	    ox << "NOTE facebox not implemented yet..." << endl << endl;
	  }
	}

	if (cons!="" && fs.substr(0, 2)==":s") {
	  if (type=="ass") {
	    double sec0 = atof(fs.substr(2).c_str()), sec1 = sec0+0.99;
	    ox << "Dialogue: 1," << DataBase::ASStimeStr(sec0) << ","
	       << DataBase::ASStimeStr(sec1) << ",concepts,foo,000,000,000,,"
	       << "Shot x: " << cons
	       << endl;
	  }

	  if (type=="vtt")
	    ox << "NOTE cons not implemented yet..." << endl << endl;
	}
      }
    }

    WriteLog("Generated "+type+" subtitles in <"+file+"> with \""+
	     spec+"\"");
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<pair<size_t,float> >
  DataBase::FeatureFrameDifference(size_t idx, const string& fean,
				   size_t len, size_t skip) {
    string msg = "DataBase::FeatureFrameDifference("+ToStr(idx)+","+fean
      +","+ToStr(len)+","+ToStr(skip)+") : ";

    cout << msg << "STARTING" << endl;

    const object_info& oi = *FindObject(idx);
    oi.dump();

    vector<pair<size_t,float> > empty, r;
    map<size_t,size_t> imap;

    size_t fmin = 10000*10000, fmax = 0;
    for (size_t i=0; i<oi.children.size(); i++) {
      size_t c = oi.children[i];
      const string& l = Label(c);
      size_t p1 = l.find(Label(idx));
      if (p1==string::npos) {
	ShowError(msg+"fail 1");
	return empty;
      }
      size_t p = l.find(':', p1);
      if (p==string::npos)
	continue;
      if (l[p+1]<'0' ||l[p+1]>'9')
	continue;
      size_t f = atoi(l.substr(p+1).c_str());
      if (imap.find(f)!=imap.end()) {
	ShowError(msg+"fail 2");
	return empty;
      }
      if (f>fmax)
	fmax = f;
      if (f<fmin)
	fmin = f;

      imap[f] = c;
    }

    if (fmin!=0 || fmax!=imap.size()-1) {
      ShowError(msg+"fail 3");
      return empty;
    }

    vector<double> dvprev;
    vector<vector<double> > cf;
    for (size_t i=fmin; i<=fmax; i++) {
      if (imap.find(i)==imap.end()) {
	ShowError(msg+"fail 4");
	return empty;
      }

      FloatVector *v = FeatureData(fean, imap[i], false);
      vector<double> dv;
      if (v)
	dv = vector<double>((float*)*v, (float*)*v+v->Length());
      else { // obs! this will fail if the first frame is missing...
	ShowError(msg+"feature data missing for frame "+Label(idx)+":"+ToStr(i)
		  +" object #"+ToStr(imap[i]));
	if (dvprev.size()==0) {
	  ShowError(msg+"cannot continue...");
	  return empty;
	}
	dv = dvprev;
      }
      delete v;
      for (size_t j=0; j<dv.size(); j++) {
	if (cf.size())
	  dv[j] += (*cf.rbegin())[j];
	int cfy = fpclassify(dv[j]);
	if (cfy==FP_NAN || cfy==FP_INFINITE)
	  ShowError(msg+"value is nan or infinite");
      }
      cf.push_back(dv);
      dvprev = dv;
    }

    for (size_t i=0; i<cf.size(); i++) {
      float v = 0;
      if (i>=len+skip && i<=cf.size()-len-skip) {
	for (size_t j=0; j<cf[0].size(); j++) {
	  float a = cf[i+skip+len-1][j]-cf[i+skip-1][j]-cf[i-1-skip][j];
	  if (i>=1+skip+len)
	    a += cf[i-1-skip-len][j];
	  v += fabs(a);
	}
      }
      int cfy = fpclassify(v);
      if (cfy==FP_NAN || cfy==FP_INFINITE)
	ShowError(msg+"value is nan or infinite");
      r.push_back(make_pair(imap[i], v));
    }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  const vector<pair<size_t,size_t> >&
  DataBase::VideoOrSegmentFramesOrdered(size_t idx, bool only_exist) {
    string msg = "DataBase::VideoOrSegmentFramesOrdered("+ToStr(idx)+") ["
      +Label(idx)+"] : ";

    if (segment_frames.find(idx)!=segment_frames.end())
      return only_exist ? segment_frames[idx].first :
	segment_frames[idx].second;

    size_t pidx = idx;
    double tp = 0, dur = 10.0*365*24*3600, fps = 1;

    if (ObjectsTargetTypeContains(idx, target_videosegment)) {
      auto psd = ParentStartDuration(idx, target_videofile);
      pidx = psd.first;
      tp = psd.second.first;
      dur = psd.second.second;

      fps = VideoFrameRate(pidx);
      if (!fps) {
	static const vector<pair<size_t,size_t> > empty;
	ShowError(msg+"zero frame rate");
	return empty;
      }
    }

    map<size_t,size_t> f2idx;

    // const string& plabel = Label(pidx);
    // size_t l = plabel.size();
    const vector<int>& c = FindObject(pidx)->children;
    for (size_t i=0; i<c.size(); i++) {
      // const string& lab = Label(c[i]);
      // if (lab.find(plabel)!=0)
      // 	continue;
      // if (lab[l]!=':')
      // 	continue;
      // if (lab.find_first_not_of("0123456789", l+1)
      // 	  !=string::npos)
      // 	continue;
      //
      // size_t f = atoi(lab.substr(l+1).c_str());

      size_t f = FindObject(c[i])->frame;
      double t = f/fps;
      if (t>=tp && t<tp+dur)
	f2idx[f] = c[i];
    }

    vector<pair<size_t,size_t> > r, v;

    size_t ff = floor(tp*fps), lf = ceil((tp+dur)*fps), n_idx = 0;
    for (size_t fno=ff; fno<lf; fno++) {
      size_t idx = -1;
      if (f2idx.find(fno)!=f2idx.end()) {
	idx = f2idx.at(fno);
	n_idx++;
      }
      v.push_back(make_pair(idx, fno));
      if (n_idx==f2idx.size())
	break;
    }

    for (auto i=f2idx.begin(); i!=f2idx.end(); i++)
      r.push_back(make_pair(i->second, i->first));

    segment_frames[idx] = make_pair(r, v);

    return only_exist ? segment_frames[idx].first :
      segment_frames[idx].second;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<pair<size_t,size_t>,pair<size_t,size_t> >
  DataBase::VideoOrSegmentFirstAndLastFrame(size_t idx, bool only_exist) {
    string msg = "DataBase::VideoOrSegmentFirstAndLastFrame("+ToStr(idx)+") ["
      +Label(idx)+"] : ";

    const auto& v = VideoOrSegmentFramesOrdered(idx, only_exist);
    if (v.size()==0)
      return make_pair(make_pair(-1, -1), make_pair(-1, -1));

    return make_pair(v.front(), v.back());
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<size_t,size_t>
  DataBase::VideoOrSegmentMiddleFrame(size_t idx, bool only_exist) {
    string msg = "DataBase::VideoOrSegmentFirstAndLastFrame("+ToStr(idx)+") ["
      +Label(idx)+"] : ";

    pair<size_t,size_t> r = make_pair(-1, -1);

    const auto& v = VideoOrSegmentFramesOrdered(idx, false);
    if (v.size()==0)
      return r;

    size_t m = (v.front().second+v.back().second)/2;

    if (!only_exist) {
      for (size_t i=0; i<v.size(); i++)
	if (v[i].second==m)
	  return v[i];
      ShowError(msg+"failed 1");
    }

    size_t dmin = -1;
    const auto& a = VideoOrSegmentFramesOrdered(idx, true);
    for (size_t i=0; i<a.size(); i++) {
      size_t d = abs(int(a[i].second)-int(m));
      if (d<dmin) {
	dmin = d;
	r = a[i];
      }
    }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t DataBase::MiddleFrameTrick(size_t i, bool angry) {
    string hdr = "DataBase::MiddleFrameTrick("+ToStr(i)+") : ";

    if (segment_to_middleframe.size()==1 &&
	segment_to_middleframe.find(-1)==segment_to_middleframe.begin()) {
      if (angry)
	ShowError(hdr+"failing A");
      return i;
    }

    if (segment_to_middleframe.size()==0)
      for (size_t j=0; j<Size(); j++)
	if (ObjectsTargetTypeContains(j, target_video)) {
	  size_t m = VideoOrSegmentMiddleFrame(j, true).first;
	  segment_to_middleframe[j] = m;
	  middleframe_to_segment[m] = j; // obs! might not be unique...
	}
    
    auto p = segment_to_middleframe.find(i);
    if (p!=segment_to_middleframe.end())
      return p->second;

    p = middleframe_to_segment.find(i);
    if (p!=middleframe_to_segment.end())
      return p->second;

    if (angry)
      ShowError(hdr+"failing B");

    return i;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<size_t,pair<double,double> >
  DataBase::ParentStartDuration(size_t idx, target_type tt) {
    string msg = "DataBase::ParentStartDuration("+ToStr(idx)+","
      +TargetTypeString(tt)+") : ";

    const string& label = Label(idx);
    int pidx = GetParentVideo(label, tt);
    if (pidx<0) {
      ShowError(msg+"parent not found");
      return pair<size_t,pair<double,double> >();
    }

    string parobj, timepoint, duration;
    if (!SolveObjectSubrange(label, tt, parobj, timepoint, duration))
      ShowError(msg+"SolveObjectSubrange() failed");

    double tp = 0.0, dur = 0.0;
    if (timepoint.find("T")==0 && duration.find("PT")==0) {
      tp  = GetMpeg7TimePoint(timepoint);
      dur = GetMpeg7Duration(duration);
    } else {
      tp  = atof(timepoint.c_str());
      dur = atof(duration.c_str());
    }

    return make_pair(pidx, make_pair(tp, dur));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoAllCaptionings(bool force, const vector<size_t>& idxs,
				  const vector<string>& caps,
				  bool use_txti, XmlDom& xml) {
    string msg = "DataBase::DoAllCaptionings() : ";

    if (DebugCaptionings())
      WriteLog(msg+"starting with idxs.size()="+ToStr(idxs.size())+
	       " caps.size()="+ToStr(caps.size()));

    for (auto i=caps.begin(); i!=caps.end(); i++)
      if (!DoOneCaptioning(force, idxs, *i, use_txti, xml))
	return ShowError(msg+"failed with <"+*i+">");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::DoOneCaptioning(bool force, const vector<size_t>& idxsin,
				 const string& cap, bool use_txti,
				 XmlDom& xml) {
    string msg = "DataBase::DoOneCaptioning("+cap+") : ";

    if (DebugCaptionings())
      WriteLog(msg+"starting");

    string namebyti;
    if (captioning_textindex2name.find(cap)!=captioning_textindex2name.end()) {
      namebyti = captioning_textindex2name.at(cap);
      if (DebugCaptionings())
	WriteLog(msg+"textindex matches name <"+namebyti+">");
    }

    bool method_found = false, missing_found = false;
    string textindex;
    map<size_t,textline_t> res;
    for (auto i=described_captionings.begin();
	 i!=described_captionings.end(); i++) {
      if (DebugCaptionings())
	WriteLog(msg+"is it "+i->first+" ?");

      if (i->first==cap || i->first==namebyti) {
	map<string,string> spec(i->second.begin(), i->second.end());
	const string& type = spec["type"];
	auto ti = spec.find("textindex");
	if (ti!=spec.end()) {
	  textindex = ti->second;
	  if ((!Picsom()->IsSlave() || !Picsom()->IsSlavePiping("captions"))
	      && !OpenReadWriteTxt())
	    return ShowError(msg+"textindices opened read-only, "
			     "try -rw=...txt...");
	}

	vector<size_t> idxs;
	if (textindex=="" || Picsom()->IsSlavePiping("captions"))
	  idxs = idxsin;
	else 
	  for (auto j=idxsin.begin(); j!=idxsin.end(); j++) {
	    if (force || TextIndexRetrieve(*j, textindex).empty()) {
	      missing_found = true;
	      idxs.push_back(*j);
	      WriteLog(msg+"caption <"+textindex+"> for #"+ToStr(*j)+
		       " <"+Label(*j)+"> will be generated");
	    } else if (DebugCaptionings())
	      WriteLog(msg+"caption <"+textindex+"> for #"+ToStr(*j)+
		       " <"+Label(*j)+"> already exists");
	  }
	
	// obs! we should test which captions already exist...
	// idxs = idxsin;

	if (type=="neuraltalk") {
	  method_found = true;
	  res = GenerateSentencesNeuralTalk(idxs, spec);
	} else
	  ShowError(msg+"type="+type+" unknown");
	break;
      }
    }

    if (!method_found)
      return ShowError(msg+"method name="+cap+" not found");

    if (missing_found && res.empty())
      return ShowError(msg+"no results");

    WriteLog(msg+cap+" returned "+ToStr(res.size())+" results");
    size_t j = 0;
    for (auto i=res.begin(); i!=res.end(); i++, j++) {
      if (DebugCaptionings())
	WriteLog("     #"+ToStr(i->first)+" "+i->second.txt_display(true));

      bool do_flush = j==res.size()-1;
      if (!StoreCaptioningResult(i->first, cap, i->second, use_txti, 
				 do_flush, xml))
	return ShowError(msg+"StoreCaptioningResult() failed");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::StoreCaptioningResult(size_t idx, const string& capname,
				       const textline_t& val,
				       bool use_textindex, 
				       bool do_flush, XmlDom& xml) {
    string msg = "DataBase::StoreCaptioningResult("+ToStr(idx)+","+
      capname+") : ";

    Tic("StoreCaptioningResult");

    if (Picsom()->IsSlave() &&
	Picsom()->SlavePipe().find("captions")!=string::npos) {
      if (!xml)
	return ShowError(msg+"slave/pipe/xml mismatch");
      
      string str = val.txt_encode();
      XmlDom fe = xml.Element("caption", str);
      fe.Prop("database", Name());
      fe.Prop("name",     capname);
      fe.Prop("label",    Label(idx));

      Tac("StoreCaptioningResult");

      return true;
    }

    string textindex;
    auto i = described_captionings.find(capname);
    if (i==described_captionings.end() &&
	captioning_textindex2name.find(capname)!=
	captioning_textindex2name.end())
      i = described_captionings.find(captioning_textindex2name[capname]);

    if (i!=described_captionings.end()) {
      map<string,string> spec(i->second.begin(), i->second.end());
      if (spec.find("textindex")!=spec.end())
	textindex = spec.at("textindex");
    }

    if (textindex=="" && use_textindex)
      return ShowError(msg+"use_textindex=true but textindex not specified");

    if (textindex!="" && use_textindex) {
      list<string> f = TextIndexFields(textindex);
      if (f.size()!=1)
	return ShowError(msg+"TextIndexFields() returned != 1");
      const string& field = *f.begin();
    
      if (DebugCaptionings())
	WriteLog("     #"+ToStr(idx)+" "+val.txt_display(true));
    
      list<string> ti_input;
      string txt1 = textindex+" add-attribute "+field+" "+val.txt_encode();
      ti_input.push_back(txt1);
      string txt2 = textindex+" add-document "+Label(idx);
      ti_input.push_back(txt2);
      if (do_flush)
	ti_input.push_back(textindex+" flush-index");

      if (!TextIndexInput(ti_input))
	return ShowError(msg+"TextIndexInput() failed");
      else
	WriteLog(msg+capname+" stored in <"+textindex+"> results");
    
      Tac("StoreCaptioningResult");

      return true;
    }

    if (textindex=="")
      textindex = "text";

    cout << "**textentry** " << Label(idx) << " " << textindex << " " 
	 << val.txt_encode() << endl;
 
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_PYTHON
  PyObject *DataBase::PythonFeatureVector(const string& fname, size_t idx) {
    string msg = "DataBase::PythonFeatureVector("+fname+") : ";

    FloatVector *fv = FeatureData(fname, idx, true);
    if (!fv) {
      ShowError(msg+"#"+ToStr(idx)+" FeatureData() returned NULL");
      return NULL;
    }

    size_t l = fv->Length();
    PyObject *v = PyList_New(l);
    for (size_t i=0; i<l; i++)
      PyList_SetItem(v, i, PyFloat_FromDouble(fv->Get(i)));

    delete fv;

    return v;
  }
#endif // PICSOM_USE_PYTHON

  /////////////////////////////////////////////////////////////////////////////

  map<size_t,textline_t>
  DataBase::GenerateSentencesNeuralTalk(const vector<size_t>& idxs,
					const map<string,string>& spec) {
    string msg = "DataBase::GenerateSentencesNeuralTalk() : ";

    map<size_t,textline_t> empty, ret;

    string debugtype = "";
    //string debugtype = "raw_outputs";
    //string debugtype = "raw_beam_matrix";

    if (DebugCaptionings()) {
      stringstream specss;
      for (auto i=spec.begin(); i!=spec.end(); i++)
	specss << " " << i->first << "=" << i->second;
      WriteLog(msg+"starting with idxs.size()="+ToStr(idxs.size())+
	       " spec:"+specss.str());
    }

    if (idxs.size()==0) {
      WriteLog(msg+"empty image list given...");
      return empty;
    }

#ifdef PICSOM_USE_PYTHON
    bool middle_frame_trick = true;
    bool exp_feat = true;

    string name = "NONAME";
    if (spec.find("name")!=spec.end())
      name = spec.at("name");
    if (spec.find("model")==spec.end()) {
      ShowError(msg+"model undefined for neuraltalk <"+name+">");
      return empty;
    }
    const string& model = spec.at("model");

    if (spec.find("init")==spec.end() || spec.find("persist")==spec.end()) {
      ShowError(msg+"init or persist undefined for neuraltalk <"+name+
		"> in settings.xml");
      return empty;
    }
    const string& init    = spec.at("init");
    const string& persist = spec.at("persist");

    size_t beam_size = 1;
    if (spec.find("beam_size")!=spec.end())
      beam_size = atoi(spec.at("beam_size").c_str());

    if (DebugCaptionings())
      WriteLog(msg+"model=<"+model+"> init=<"+init+"> persist=<"+persist+
	       "> beam_size="+ToStr(beam_size));

    // setenv("HDF5_DISABLE_VERSION_CHECK", "2", 1);

    PyGILState_STATE gstate = PyGILState_Ensure();

    static bool path_set = false;
    if (!path_set) {
      string addpath1 = Picsom()->RootDir("python/neuraltalkTheano");
      if (!DirectoryExists(addpath1)) {
	ShowError(msg+"directory <"+addpath1+"> does not exist");
	return empty;  // obs!
      }
      string addpath2 = Picsom()->RootDir("python");
      if (DebugCaptionings())
	WriteLog(msg+"addind <"+addpath1+"> and <"+addpath2+"> in python path");
      PyObject *path = PySys_GetObject((char*)"path"); // borrowed
      PyObject *dir1 = PyString_FromString_x(addpath1.c_str());
      PyObject *dir2 = PyString_FromString_x(addpath2.c_str());
      if (PyList_Insert(path, 0, dir2)) {
	ShowError(msg+"PyList_Insert() <"+addpath2+"> failed");
	return empty;  // obs!
      }
      if (PyList_Insert(path, 0, dir1)) {
	ShowError(msg+"PyList_Insert() <"+addpath1+"> failed");
	return empty;  // obs!
      }
      Py_DECREF(dir1);
      Py_DECREF(dir2);
      path_set = true;
    }

    PyObject *caption_generator = NULL;
    auto mp = neuraltalk_models.find(model);
    if (mp!=neuraltalk_models.end())
      caption_generator = mp->second;

    if (!caption_generator) {
      if (DebugCaptionings())
	WriteLog(msg+"constructing model <"+model+">");

      PyObject *pName   = PyString_FromString_x("caption_generator");
      PyObject *pModule = PyImport_Import(pName);
      Py_DECREF(pName);
      if (!pModule) {
	PyErr_Print();
	ShowError(msg+"PyImport_Import() failed");
	return empty;  // obs!
      }
      PyObject *pDict   = PyModule_GetDict(pModule); // borrowed
      Py_DECREF(pModule);
      PyObject *pClass  = PyDict_GetItemString(pDict, "caption_generator"); // borrowed
    
      if (!PyCallable_Check(pClass)) {
	PyErr_Print();
	return empty; // obs!
      }

      string dbpath = Path();
      if (spec.find("database")!=spec.end())
	dbpath = Picsom()->ExpandDataBasePath(spec.at("database"));
      string mpath = dbpath+"/neuraltalk/models/"+model;

      PyObject *pParams = PyDict_New(), *p = NULL;
      PyDict_SetItemString(pParams, "multi_model",    	     p=PyInt_FromLong_x(0));
      Py_DECREF(p);
      PyDict_SetItemString(pParams, "use_label_file", 	     p=PyInt_FromLong_x(0));
      Py_DECREF(p);
      PyDict_SetItemString(pParams, "softmax_propogate",     p=PyInt_FromLong_x(0)); //obs! spelling
      Py_DECREF(p);
      PyDict_SetItemString(pParams, "beam_size",             p=PyInt_FromLong_x(beam_size));
      Py_DECREF(p);
      PyDict_SetItemString(pParams, "checkpoint_path",       p=PyString_FromString_x(mpath.c_str()));
      Py_DECREF(p);
      PyDict_SetItemString(pParams, "softmax_smooth_factor", p=PyFloat_FromDouble(1.0));
      Py_DECREF(p);
      if (!exp_feat) {
	PyDict_SetItemString(pParams, "en_aux_inp",          p=PyInt_FromLong_x(1));
	Py_DECREF(p);
      }

      PyObject *pArgs = PyTuple_New(1);
      PyTuple_SetItem(pArgs, 0, pParams); // steals

      caption_generator = PyObject_CallObject(pClass, pArgs);
      PyErr_Print();
      Py_DECREF(pArgs);

      neuraltalk_models[model] = caption_generator;

      if (DebugCaptionings())
	WriteLog(msg+"successfully constructed model <"+model+">");

    } else {
      if (DebugCaptionings())
	WriteLog(msg+"reusing stored model <"+model+">");
    }

    string ilist = Path()+"/neuraltalk/ilist.txt";
    string feat1 = Path()+"/features/frcnn80detP3+3SpatGaussScaleP6grRBFsun397.txt";
    string feat2 = Path()+"/features/c_in14_gr_pool5_d_aA3_ca3.txt";

    PyObject *pParams = PyDict_New(), *p = NULL;
    PyDict_SetItemString(pParams, "multi_model",  p=PyInt_FromLong_x(0));
    Py_DECREF(p);
    PyDict_SetItemString(pParams, "rescoreByLen", p=PyInt_FromLong_x(0));
    Py_DECREF(p);
    PyDict_SetItemString(pParams, "root_path", 	  p=PyString_FromString_x("rp"));
    Py_DECREF(p);
    PyDict_SetItemString(pParams, "fname_append", p=PyString_FromString_x("fna"));
    Py_DECREF(p);

    if (exp_feat) {
      PyObject *expfeat = PyList_New(idxs.size());
      PyObject *dbgfile = NULL;
      if (debugtype!="")
	dbgfile = PyList_New(idxs.size());
      size_t j = 0;
      for (auto i=idxs.begin(); i!=idxs.end(); i++, j++) {
	size_t vidxi = *i, vidxp = *i;
	if (FeatureData(init, vidxi, false)==NULL && middle_frame_trick)
	  vidxi = MiddleFrameTrick(vidxi, true);
	if (FeatureData(persist, vidxp, false)==NULL && middle_frame_trick)
	  vidxp = MiddleFrameTrick(vidxp, true);

      	PyObject *f1 = PythonFeatureVector(init,    vidxi);
      	PyObject *f2 = PythonFeatureVector(persist, vidxp);
      	PyObject *ff = PyList_New(2);
      	PyList_SetItem(ff, 0, f1); // steals
      	PyList_SetItem(ff, 1, f2); // steals
      	PyList_SetItem(expfeat, j, ff); // steals

	if (dbgfile) {
	  string idxs = ToStr(*i);
	  while (idxs.size()<10)
	    idxs = "0"+idxs;
	  string dbgf = "idx-"+idxs+".txt";
	  PyObject *p = PyString_FromString_x(dbgf.c_str());
	  PyList_SetItem(dbgfile, j, p); // steals
	}
      }
      PyDict_SetItemString(pParams, "explicit_feat", expfeat);
      Py_DECREF(expfeat);

      if (dbgfile) {
	PyDict_SetItemString(pParams, "debugfiles", dbgfile);
	Py_DECREF(dbgfile);
	PyObject *p = PyString_FromString_x(debugtype.c_str());
	PyDict_SetItemString(pParams, "debugtype", p);
	Py_DECREF(p);
      }

    } else {
      PyDict_SetItemString(pParams, "feat_file",    p=PyString_FromString_x(feat1.c_str()));
      Py_DECREF(p);
      PyDict_SetItemString(pParams, "aux_inp_file", p=PyString_FromString_x(feat2.c_str()));
      Py_DECREF(p);
      PyDict_SetItemString(pParams, "imgList",      p=PyString_FromString_x(ilist.c_str()));
      Py_DECREF(p);
    }

    PyObject *generate = PyString_FromString_x("generate");
    PyObject *pValue = PyObject_CallMethodObjArgs(caption_generator, generate, pParams, NULL);
    Py_DECREF(generate);
    Py_DECREF(pParams);
    if (!pValue || !PyDict_Check(pValue)) {
      PyErr_Print();
      ShowError(msg+"Z");
      return empty; // obs!
    }

    PyObject *blobs = PyDict_GetItemString(pValue, "imgblobs"); // borrowed
    if (!PyList_Check(blobs)) {
      PyErr_Print();
      ShowError(msg+"X");
      return empty; // obs!
    }

    size_t ls = PyList_Size(blobs);
    if (ls!=idxs.size()) {
      ShowError(msg+"input and output sizes differ: "+ToStr(idxs.size())+
		"!="+ToStr(ls));
      return empty;  // obs!
    }

    for (size_t li=0; li<ls; li++) {
      size_t idx = idxs[li];

      PyObject *blob = PyList_GetItem(blobs, li); // borrowed
      if (!PyDict_Check(blob)) {
	PyErr_Print();
	ShowError(msg+"W");
	return empty; // obs!
      }

      textline_t textl(this, idx);

      PyObject *candidate = PyDict_GetItemString(blob, "candidate"); // borrowed
      NeuralTalkAddCandidate(textl, candidate);
 
      PyObject *clist = PyDict_GetItemString(blob, "candidatelist"); // borrowed
      if (clist && PyList_Check(clist)) {
	size_t cs = PyList_Size(clist);
	for (size_t ci=0; ci<cs; ci++) {
	  PyObject *c = PyList_GetItem(clist, ci); // borrowed
	  NeuralTalkAddCandidate(textl, c);
	}
      }
 
      ret[idx] = textl;
    }

    Py_DECREF(pValue);

    PyGILState_Release(gstate);

#endif // PICSOM_USE_PYTHON

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_PYTHON
  bool DataBase::NeuralTalkAddCandidate(textline_t& textl, PyObject *cand) {
    string msg = "DataBase::NeuralTalkAddCandidate() : ";

    if (!cand) {
      PyErr_Print();
      return ShowError(msg+"NULL pointer");
    }
      
    if (!PyDict_Check(cand)) {
      PyErr_Print();
      return ShowError(msg+"PyDict_Check() failed");
    }

    PyObject *text = PyDict_GetItemString(cand, "text");
    // if (!PyUnicode_Check(text)) {
    //   PyErr_Print();
    //   return ShowError(msg+"XX1");
    // }

    PyObject *logprob = PyDict_GetItemString(cand, "logprob");
    if (!PyFloat_Check(logprob)) {
      PyErr_Print();
      return ShowError(msg+"XX2");
    }

    string caption;
    if (PyUnicode_Check(text)) {
      PyObject *utf8 = PyUnicode_AsUTF8String(text);
      caption = PyString_AsString_x(utf8);
      Py_DECREF(utf8);
    } else
      caption = PyString_AsString_x(text);

    double logp = PyFloat_AsDouble(logprob);
    if (DebugCaptionings())
      WriteLog("#"+ToStr(textl.idx)+" text: <"+caption+
	       "> logprob: "+ToStr(logp));

    textl.add(caption, logp);

    return true;
  }
#endif // PICSOM_USE_PYTHON

  /////////////////////////////////////////////////////////////////////////////

  map<size_t,textline_t>
  DataBase::ReadSentencesCOCOsubmission(const list<string>& args) {
    string msg = "DataBase::ReadSentencesCOCOsubmission(list) : ";

    map<size_t,textline_t> ret;

#if defined(HAVE_JSON_JSON_H)
    if (args.size()!=1)
      ShowError(msg+"args.size()!=1");

    else {
      string infile = *args.begin();
      WriteLog("Starting to read <"+ShortFileName(infile)+">");
      ifstream istr(infile);
      Json::Value doc;
      istr >> doc;
     
      size_t nsent = 0;
      for (size_t k=0; k<doc.size(); k++) {
	const Json::Value& img_cap = doc[(int)k];
	int    id  = img_cap.get("image_id", "-1").asInt();
	string cap = img_cap.get("caption", "").asString();

	// cout << id << " : " << cap << endl;

	string label = ToStr(id);
	while (label.length()<LabelLength())
	  label = "0"+label;

	int labelidx = LabelIndex(label);
	if (labelidx<0)
	  ShowError(msg+"labelidx<0");

	if (ret.find(labelidx)==ret.end())
	  ret[labelidx] = textline_t(this, labelidx);

	textline_t& tl = ret.at(labelidx);
	tl.add(cap, 0);

	nsent++;
      }

      WriteLog(ToStr(nsent)+" sentences for "+ToStr(ret.size())
	       +" objects read from "+ToStr(args.size())+" files");
    }

#else // HAVE_JSON_JSON_H
    ShowError(msg+"HAVE_JSON_JSON_H undef "+ToStr(args.size()));
#endif 

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  map<size_t,textline_t>
  DataBase::ReadSentencesNeuralTalk(const list<string>& args) {
    string msg = "DataBase::ReadSentencesNeuralTalk(list) : ";

    size_t nfiles = 0, nsent = 0;
    map<size_t,textline_t> ss;
    for (auto i=args.begin(); i!=args.end(); i++) {
      map<size_t,textline_t> l = ReadSentencesNeuralTalk(*i);
      ss.insert(l.begin(), l.end());
      for (auto j=l.begin(); j!=l.end(); j++)
	nsent += j->second.txt_val.size();

      nfiles++;
    }

    WriteLog(ToStr(nsent)+" sentences for "+ToStr(ss.size())
	     +" objects read from "+ToStr(nfiles)+" files");

    return ss;
  }

  /////////////////////////////////////////////////////////////////////////////

  map<size_t,textline_t> DataBase::ReadSentencesNeuralTalk(const string& f) {
    string msg = "DataBase::ReadSentencesNeuralTalk("+f+") : ";

    map<size_t,textline_t> ret;

#ifdef PICSOM_USE_JAULA
    JAULA::Value_Complex *doc = NULL;
    try {
      ifstream istr(f);
      doc = JAULA::Parser::parseStream(istr);
    } catch (const JAULA::Exception& e) {
      stringstream ss;
      e.display(ss);
      ShowError(msg+"JAULA::Parser::parseStream() failed: "+ss.str());
      return ret;
    }

    if (doc->getType()!=JAULA::Value::TYPE_OBJECT) {
      ShowError(msg+"1");
      delete doc;
      return ret;
    }

    const JAULA::Value_Object& o
      = *dynamic_cast<JAULA::Value_Object*>(doc);
    auto& m = o.getData();
    auto p = m.find("imgblobs");
    if (p!=m.end()) {
      const JAULA::Value_Array& a
	= *dynamic_cast<JAULA::Value_Array*>(p->second);
      auto& l = a.getData();
      for (auto k=l.begin(); k!=l.end(); k++) {
	const JAULA::Value_Object& img
	  = *dynamic_cast<JAULA::Value_Object*>(*k);
	string img_path = JAULA_string(img, "img_path");
	// cout << img_path << endl;
	string label = img_path;
	size_t p = label.find('.');
	if (p!=string::npos)
	  label.erase(p);

	const JAULA::Value_Object& candidate
	  = *dynamic_cast<JAULA::Value_Object*>(img.getData().at("candidate"));
	  
	string text    = JAULA_string(candidate, "text");
	double logprob = JAULA_double(candidate, "logprob");

	int labelidx = LabelIndex(label);
	if (labelidx<0)
	  ShowError(msg+"labelidx<0");
	
	if (ret.find(labelidx)==ret.end())
	  ret[labelidx] = textline_t(this, labelidx);

	textline_t& tl = ret.at(labelidx);
	tl.add(text, logprob);

	if (img.getData().find("candidatelist")!=img.getData().end()) {
	  const JAULA::Value_Array& cl
	    = *dynamic_cast<JAULA::Value_Array*>(img.getData().
						 find("candidatelist")
						 ->second);
	  for (auto c=cl.getData().begin(); c!=cl.getData().end(); c++) {
	    const JAULA::Value_Object& cand
	      = *dynamic_cast<JAULA::Value_Object*>(*c);
	    text    = JAULA_string(cand, "text");
	    logprob = JAULA_double(cand, "logprob");
	    tl.add(text, logprob);
	  }
	}
      }
    }
    delete doc;

#else
    ShowError(msg+"no JAULA");
#endif // PICSOM_USE_JAULA

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool
  DataBase::StoreSentences(const string& sspec, const string& type,
			   const vector<size_t>& idxsin,
			   const map<size_t,textline_t>& ssin,
			   const string& odir) {
    string msg = "DataBase::StoreSentences("+sspec+","+type+") : ";

    string typesspec = sspec, videolabel;
    if (type!="")
      typesspec = type+"-"+typesspec;

    bool is_video = false;
    vector<size_t> idxs = idxsin, missing;
    if (idxs.size() && ObjectsTargetTypeContains(idxs[0], target_video)) { 
      is_video = true;
      vector<size_t> idxs_new;
      for (auto i=idxs.begin(); i!=idxs.end(); i++) {
	if (!ObjectsTargetTypeContains(*i, target_video))
	  return ShowError(msg+"videos and non-videos mixed");

	auto p = ssin.find(*i);
	if (p!=ssin.end() && !p->second.has_time_set()) {
	  idxs_new.push_back(*i);
	  continue;
	}

	if (videolabel!="")
	  return ShowError(msg+"videolabel already set");
	videolabel = Label(*i);

	const object_info *oi = FindObject(*i);
	const vector<int>& c = oi->children;
	for (auto j=c.begin(); j!=c.end(); j++)
	  idxs_new.push_back(*j);
      }
      idxs = idxs_new;
    }

    for (auto i=idxs.begin(); i!=idxs.end(); i++)
      if (ssin.find(*i)==ssin.end())
	missing.push_back(*i);

    set<size_t> idxset(idxs.begin(), idxs.end());
    map<size_t,textline_t> ss;
    for (auto i=ssin.begin(); i!=ssin.end(); i++)
      if (idxset.find(i->first)!=idxset.end())
	ss.insert(*i);

    WriteLog("Sentences for "+ToStr(ssin.size())+" objects, "
	     "for "+ToStr(missing.size())+
	     " objects the sentence is missing, "+ToStr(ss.size())+
	     " objects will get sentence(s)");    

    ofstream os;
    if (is_video && videolabel!="") {
      string lab = videolabel;  // obs! what if there are more?
      string ofile = lab+"-"+typesspec+".txt";
      if (odir!="")
	ofile = odir+ofile;
      else {
	ofile = ObjectTextFileSubdirPath(LabelIndex(lab), "", typesspec);
	size_t p = ofile.rfind('/');
	Picsom()->MkDirHier(ofile.substr(0, p), 02775);
      }
      os.open(ofile);
    }

    for (auto i=ss.begin(); i!=ss.end(); i++) {
      // too slow...
      // auto psd = ParentStartDuration(i->idx, target_video);
      const string& label = Label(i->first);
      size_t p = label.rfind(':');
      if (is_video && p!=string::npos) {
	size_t f = atol(label.substr(p+1).c_str());
	float fps = 25;  // obs! hard-coded
	WarnOnce("Hard-coded fps=25");
	float dur = 1.0/fps;
	double tp = dur*f;
	// obs! since 2015-05-15 this probably doesn't work anymore
	textline_t tl(this, i->first);
	tl.start = tp;
	tl.end   = tp+dur;
	tl.txt_val = i->second.txt_val;
	os << tl.txt_display() << endl;

      } else {
	string ofile;
	if (odir!="")
	  ofile = odir+label+"-"+typesspec+".txt";
	else {
	  ofile = ObjectTextFileSubdirPath(i->first, "", typesspec);
	  size_t p = ofile.rfind('/');
	  Picsom()->MkDirHier(ofile.substr(0, p), 02775);
    	}
	os.open(ofile);
	os << i->second.txt_display() << endl;
	os.close();

	if (debug_text)
	  WriteLog("  stored "+ToStr(i->second.txt_val.size())+
		   " text entries of type \""+typesspec+"\" for <"+
		   Label(i->first)+"> in ["+ShortFileName(ofile)+"]");
      }
    }	

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<float> DataBase::TfIdf(const string& /*s*/, size_t /*n*/) {
    vector<float> v;

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  map<string,size_t> DataBase::NgramCounts(const string& s, size_t n) {
    map<string,size_t> v;

    vector<string> w = SplitInSpaces(s);

    for (size_t i=0; i<w.size()+1-n; i++) {
      string gram;
      bool skip = false;
      for (size_t j=i; !skip && j<i+n; j++)
	if (w[j]=="#")
	  skip = true;
	else
	  gram += (j==i?"":"_")+w[j];
      if (!skip)
	v[gram]++;
    }

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t DataBase::NgramIndex(const string& s, size_t n) {
    map<string,pair<size_t,double> >& m = idf[n];
    for (auto i=m.begin(); i!=m.end(); i++)
      if (i->first==s)
	return i->second.first;
    return -1;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool DataBase::CreateIdfs(const string& text,
			    const vector<size_t>& idxs,
			    const vector<size_t>& ns) {
    string msg = "DataBase::CreateIdfs("+text+") : ";

    bool debug = false;

    for (size_t ni=0; ni<ns.size(); ni++)
      idf[ns[ni]].clear();

    for (size_t i=0; i<idxs.size(); i++) {
      size_t idx = idxs[i];
      list<pair<string,string> > ls = TextIndexRetrieve(idx, text);
      map<string,string> sm(ls.begin(), ls.end());
      string s = sm["text"];
      if (debug)
	cout << s << endl;
      for (size_t ni=0; ni<ns.size(); ni++) {
	size_t n = ns[ni];
	map<string,size_t> ngc = NgramCounts(s, n);
	if (debug)
	  cout << n << " :";
	for (auto g=ngc.begin(); g!=ngc.end(); g++) {
	  if (debug)
	    cout << " " << g->first << "=" << g->second;
	  
	  map<string,pair<size_t,double> >& m = idf[n];
	  pair<size_t,double>& p = m[g->first];
	  p.second++;
	}
	if (debug)
	  cout << endl;
      }
    }

    for (size_t ni=0; ni<ns.size(); ni++) {
      map<string,pair<size_t,double> >& m = idf[ns[ni]];
      size_t j = 0;
      for (auto i=m.begin(); i!=m.end(); i++, j++) {
	i->second.first = j;
	i->second.second /= idxs.size();
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string DataBase::ShowTfIdf(const vector<float>& v, size_t n) {
    stringstream ss;

    bool started = false;
    map<string,pair<size_t,double> >& m = idf[n];
    for (auto i=m.begin(); i!=m.end(); i++)
      if (v[i->second.first]) {
	ss << (started?" ":"") << i->first << "(" << i->second.first << ")="
	   << i->second.second;
	started = true;
      }

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#include <Source.C>
#include <List.C>
template class simple::ListOf<picsom::DataBase>;
#endif // __GNUC__

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

