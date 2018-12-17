// -*- C++ -*-  $Id: DataBase.h,v 2.557 2018/12/15 23:07:51 jormal Exp $
// 
// Copyright 1998-2018 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_DATABASE_H_
#define _PICSOM_DATABASE_H_

#include <PicSOM.h>
#include <TSSOM.h>

#include <ground_truth.h>
#include <object_set.h>
#include <aspects.h>
#include <videofile.h>

#ifdef HAVE_SQLITE3_H
#include <sqlite3.h>
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
#include <mysql.h>
#endif // HAVE_MYSQL_H

#ifdef PICSOM_USE_CONTEXTSTATE
#include <ContextState.h>
#endif // PICSOM_USE_CONTEXTSTATE

#ifdef PICSOM_USE_OD
#include <ObjectDetection.h>
#endif // PICSOM_USE_OD

#ifdef HAVE_CUDA_RUNTIME_H
#include <cuda_runtime.h>
#endif // HAVE_CUDA_RUNTIME_H

#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
#include <cstring>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <caffe/caffe.hpp>
#pragma GCC diagnostic pop
#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE

#if defined(HAVE_CAFFE2_CORE_MACROS_H) && defined(PICSOM_USE_CAFFE2)
#include <caffe2/core/macros.h>
#endif // HAVE_CAFFE2_CORE_MACROS_H && PICSOM_USE_CAFFE2

#if defined(HAVE_THC_H)
#include <THC.h>
#endif // defined(HAVE_THC_H)

#ifdef HAVE_JSON_JSON_H
#include <json/json.h>
#endif // HAVE_JSON_JSON_H

#ifdef HAVE_ZIP_H
#include <zip.h>
#if (LIBZIP_VERSION_MAJOR==0)
typedef struct zip zip_t;
typedef struct zip_file zip_file_t;
typedef struct zip_stat zip_stat_t;
#endif // LIBZIP_VERSION_MAJOR==0
#endif // HAVE_ZIP_H

#ifdef PICSOM_USE_PYTHON
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include <Python.h>
#include <marshal.h>
#endif // PICSOM_USE_PYTHON

#include <ontology.h>

namespace picsom {
  static string DataBase_h_vcid =
    "@(#)$Id: DataBase.h,v 2.557 2018/12/15 23:07:51 jormal Exp $";

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
  typedef pair<int,int> xymap_e;
  typedef vector<xymap_e> xymap_ve;
  typedef map<int,xymap_ve> xymap_t;
  
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  typedef vector<pair<string,float> > keyword_list;

  ///
  typedef struct {
    string name; ///
    string schema; ///
    vector<pair<string,string> > spec;
    size_t rows;
    bool exists;
  } sql_table_info;

  ///
  typedef struct {
    string name; ///
    size_t size;
    struct timespec moddate;
    struct timespec insdate;
    string user;
    bool exists;
  } sql_file_info;

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  ///
  class SVM;
  class CbirAlgorithm;

  /**
     Database handling.
  */
  class DataBase : public Simple {
  public:
    /// The constructor.
    DataBase(PicSOM*, const string&, const string& = "", const string& = "",
	     bool = false, bool = false);

    /// poison
    DataBase() 
#ifdef PICSOM_USE_CONTEXTSTATE
      : context(NULL, NULL)
#endif // PICSOM_USE_CONTEXTSTATE
    { SimpleAbort(); }

    /// poison
    DataBase(const DataBase& db) : Simple(db)
#ifdef PICSOM_USE_CONTEXTSTATE
                                 , context(NULL, NULL)
#endif // PICSOM_USE_CONTEXTSTATE
    { SimpleAbort(); }

    /// The destructor.
    virtual ~DataBase();

    /// Dump is required in all descendants of Simple.
    virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;
    /// SimpleClassNames is required in all descendants of Simple.
    virtual const char **SimpleClassNames() const { static const char *n[] = {
      "DataBase", NULL }; return n; }

    /// Prefix to start databases to keep in user's local directory.
    static const string& UserDbPrefix() {
      static string s = "@";
      return s;
    }

    /// Separator between database name and view name.
    static const string& ViewPartSeparator() {
      static string s = "@";
      return s;
    }

    /// Returns a unique temporary directory name ready for writing.
    string TempDir(const string& = "", bool = true);

    /// Returns a unique temporary file name ready for writing.
    string TempFile(const string& = "", bool = true);

    ///
    void UnlinkTempFile(const string&);

    ///
    void UnlinkTempFiles();

    /// 
    void RemoveBrokenDataFiles(bool b) { remove_broken_datafiles = b; }

    /// 
    bool RemoveBrokenDataFiles() { return remove_broken_datafiles; }

    ///
    void Visible(bool v) { visible = v; }

    ///
    bool Visible() const { return visible; }

    /// Returns true if database name tells it is user's local db.
    bool IsUserDb() const {
      return Name()=="" || Name().find(UserDbPrefix())==0;
    }

    /// Returns true if database name tells it is user's local db.
    bool IsLocalDiskDb() const {
      return Path().find(Picsom()->TempDirRoot())==0;
    }

    ///
    XmlDom FeaturesExpandedXML() const {
      return features_expanded_xml.Root();
    }

    /// Access to in-core settings.xml.
    const XmlDom& SettingsXML() const { return settingsxml; }

    /// Initialization of basic information + description + access.
    bool ApplySettingsXML(bool);

    /// A helper routine for the one above.
    bool InterpretOrPostpone(const string&, const string&, bool);

    /// Creates the needed files and directories.
    bool CreateFromScratch(bool, bool, bool, bool, bool);

    /// Creates a new settings.xml file.
    bool CopyOrCreateSettingsXml(const string&);

    /// Creates a new settings.xml file.
    bool CreateNewSettingsXml(const string&);

    /// Sets group write permissions to files in global area.
    void SetFilePermissions(const string& f) const {
      if (!IsUserDb()) {
	int mode = 0664;
	chmod(f.c_str(), mode);
      }
    }

    /// Adds <extraction> in settings.xml file.
    bool AddExtraction(XmlDom&);

    /// Checks the mode and applies pic_som->MainLoopArgs().
    bool ApplyMainLoopArgs();

    /// Interprets key=value.
    bool Interpret(const string&, const string&, int&);

    /// Interprets key=value.
    bool Interpret(const string& k, const string& v);

    /// Interprets key=value.
    bool Interpret(const pair<string,string>& kv) {
      return Interpret(kv.first, kv.second);
    }

    ///
    bool InterpretDefaults();

    /// Returns file or directory name expanded from path or seconadrypath.
    /// (if not forbidden with bool argument)
    /// Delete after use!
    //const char *ExpandPathM(const char*, const char* = NULL,bool=false) const;

    /// Returns file or directory name expanded from path or seconadrypath.
    /// (if not forbidden with bool argument)
    string ExpandPath(const string& d, const string& l = "",
                      bool primonly = false) const;
    /*
    string ExpandPath(const string& d, const string& l = "",
                      bool primonly = false) const {
      const char *p = ExpandPathM(d.c_str(), l!="" ? l.c_str() : NULL,
                                  primonly);
      string ret = p ? p : "";
      delete [] p;
      return ret;
    }
    */

    /// Returns file or directory name expanded from path/views.
    string ExpandViewPath(const string&, const string& = "") const;

    /// Returns canonical name of the data base.
    const string& Name() const { return name; }

    const string& BaseName() const {
      return ParentView()?ParentView()->BaseName():Name();
    }

    /// Returns the long name of the database.
    const string& LongName() const { return longname; }

    /// Returns the short description text of the database.
    const string& ShortText() const { return shorttext; }

    /// Warns once if "WRITE HERE" texts occur in settings.xml.
    void CheckEmptyText(const string&);

    /// Utility for shortening displayed file names.
    string ShortFileName(const string& n) const {
      return n.find(Path()+"/") ? n : n.substr(Path().size()+1);
    }

    /// Returns the root directory name for the objects: "images" or "objects"
    const string& ObjectRoot() const { return objdir; }
    
    /// Finds all views and puts them in view list.
    void FindAllViews();

    /// Finds all views and puts them in view list.
    DataBase *FindView(const string&);

    /// Returns list of form "120x90,100x100,75x100"
    string FindThumbnailSizes();

    /// Returns list of thumbnail sizes.
    vector<string> FindThumbnailSizesList() {
      return SplitInCommas(FindThumbnailSizes());
    }

    /// Returns the last resort thumbnail size.
    static const string& LastResortThumbnailSize() {
      static string s = "120x90";
      return s;
    }

    /// Returns type of thumbnails.
    const string& ThumbnailType() const { return tn_type; }

    /// Sets type of thumbnails.
    void ThumbnailType(const string& s) { tn_type = s; }

    /// Gives path to the thumbnail and its MIME type.
    pair<string,string> ThumbnailPath(size_t, bool = false);

    ///
    pair<string,string> VirtualThumbnailPath(int);

    ///
    const script_exp_t& SetupScript(const string&);

    /// Locates and creates the Index for one feature type.
    Index *FindIndex(const string&, const string& params = "", 
                     bool = false, bool = false);

    /// locates the Index for one feature type if it exists.
    Index *FindIndexInternal(const string&, bool);

    /// Gives pointer to Index by number.
    Index *GetIndex(size_t i) const { return index[i]; }

    /// Method name of the index, such as "tssom".
    const string& IndexMethodName(size_t i) const {
      return GetIndex(i)->MethodName();
    }

    /// Test of index's method.
    bool IndexIs(size_t i, const string& n) const {
      return GetIndex(i)->MethodName()==n;
    }

    /// Returns true if argument specifies a valid raw feature name.
    // bool IsRawFeatureName(const string&) const;

    /// Needed by FindFeature().
    bool AnyLeafDirectoryFileExists(const string& s, const string& f) const {
      list<string> l = LeafDirectoryFileList(s, f, true, true);
      return !l.empty();
    }
    
    /// Return given directory if exists, otherwise returns empty string 
    bool DirectoryOK(const string& dir, bool printdebug=false) const {
      bool exists = DirectoryExists(dir);
      if (printdebug)
	cout << " <" << dir << ">\t" << (exists?"true":"false") << endl;
      return exists;
    }

    /// Return given filename if exists, otherwise returns empty string 
    string FileIfOK(const string& f, const string& dir="", 
		    bool printdebug=false, bool requiresize=false) const {
      string fname = f;
      if (!dir.empty())
	fname = dir+"/"+fname;
      string res = FileExists(fname)&&(!requiresize&&FileSize(fname)>0)
	? fname : "";
      if (printdebug)
	cout << " <" << fname << ">\t" << (res.empty()?"false":"true") << endl;
      return res;
    }

    /** Return given filename if exists, otherwise checks the same
	with .gz ending otherwise returns empty string */
    string FileIfOKgz(const string& fname, const string& dir="", 
		      bool printdebug=false, bool requiresize=false) const {
      string ret = FileIfOK(fname, dir, printdebug, requiresize);
      if (ret.empty()) 
	ret = FileIfOK(fname+".gz", dir, printdebug, requiresize);
      return ret;
    }

    //
    string ObjectTextFileSubdirPath(size_t, const string&, const string&);

    //
    textline_t ObjectTextLineRetrieve(size_t, const string&, const string&, bool);

    //
    bool ObjectTextLineStore(const textline_t&, const string&, const string&);

    // Checks whether object file with given index is of given type and availabe.
    string ObjectTypeAndPath(const string&, target_type) /*const*/;
    
    // 
    string MostAuthoritativeDataFile(int idx, Index* ts, bool most=true);

    //
    string LeastAuthoritativeDataFile(int idx, Index* ts) {
      return MostAuthoritativeDataFile(idx,ts,false);
    }

    // Returns true if filename looks like an orphan file.
    bool IsOrphanFileName(const string&) const;

    // Converts orphan filename to leaf directory filename.
    string OrphanToLeafFileName(const string&) const;

    /** Generates two bool vectors for the first and second directory
	levels in the database from a given ground_truth object.  The
	bool is set to true for a directory that contains an object in
	the gt object.
     */
    void CreateRestrictionVectorsOld(const ground_truth& restr, 
				     vector<bool>& d1_rest,
				     vector<bool>& d2_rest) const;

    /** Generates bool vectors for the first and second directory
	levels in the database from a given ground_truth object.  The
	bool is set to true for a directory that contains an object in
	the gt object.
     */
    void CreateRestrictionVectors(const ground_truth& restr, 
				  vector<vector<bool> >& dir_rest) const;

    /// Creates a list of files found in leaf directories.
    list<string> LeafDirectoryFileList(const string&, const string&,
				       bool = false, bool = false) const;

    /// A helper for the one above.
    bool LeafDirectoryFileListInner(list<string>&,
				    const vector<size_t>&,
				    const string&, const string&,
				    bool, bool, const ground_truth&,
				    const vector<vector<bool> >&) const;

    /// A helper for the one above.
    string LeafDirectory(const vector<size_t>&) const;
 
    /// Kind of an inverse of the one above.
    vector<size_t> ObjectDirectoryComponents(const string&);

    /// Creates a list of files found in leaf directories.
    list<string> LeafDirectoryFileListOld(const string&, const string&,
					  bool = false, bool = false) const;

    /// A helper for the one above.
    string LeafDirectoryOld(int d1 = -1, int d2 = -1, int d3 = -1) const {
      stringstream ret;
      ret << path << "/" << objdir;
      if (d1>=0) {
	ret << "/" << setw(2) << setfill('0') << d1 << setw(0);
	if (d2>=0) {
	  ret << "/" << setw(2) << setfill('0') << d2 << setw(0);
	  if (d3>=0)
	    ret << "/" << setw(2) << setfill('0') << d3 << setw(0);
	}
      }
      return ret.str();
    }

    string ObjectPathPart(string path) {
      string objpath = path + "/" + objdir;
      if (path.substr(0,objpath.size()) == objpath) 
	return path.substr(objpath.size()+1);
      return path;
    }

    /// Which objects should be in this directory?
    vector<int> AllObjectsIn(const vector<size_t>&) const;

    /// A helper for the above one
    bool AllObjectsInInner(int, const string&, vector<int>&) const;

    /// Which objects should be in this directory?
    vector<int> AllObjectsIn(size_t) const;

    /// Which objects should be in this directory?
    vector<int> AllObjectsInOld(int, int, int) const;

    /// Which objects should be in this directory?
    vector<int> AllObjectsInOld(int d) const {
      int d3 = d%100;
      d /= 100;
      int d2 = d%100;
      return AllObjectsInOld(d/100, d2, d3);
    }

    /// Are there any files of form .*12345678.*-fn.dat.* in dir?
    bool HasAnyOrphans(const string& dir, const string& fn) const;

    /// Returns true if target_segment is a modifier (as should in modern dbs).
    bool SegmentIsModifier() const { return segment_is_modifier; }

    /// The number of images in db when Indexs were created.
    int OriginalSize() const { return original_size; }

    /// The number of images in db when started.
    int StartUpSize() const { return startup_size; }

    /// Removes Restriction.
    void RemoveRestrictionInner(ground_truth& gt, const string& n) { 
      if (!gt.empty())
	WriteLog("Restriction "+(n!=""?n+" ":"")+"removed");
      gt.clear();
    }

    /// Removes Restriction.
    void RemoveRestriction() { 
      RemoveRestrictionInner(restriction, "");
    }

    /// The number of images in restriction, if known, or 0.
    size_t RestrictionSize(target_type tt) const {
      if (tt==target_any_target)
	return restriction.empty() ? Size() : restriction.positives();

      size_t n = 0;
      for (size_t i=0; i<Size(); i++)
	if ((restriction.empty() || restriction[i]==1) &&
	    ObjectsTargetTypeContains(i, tt))
	  n++;

      return n;
    }
    
    /// Alternative way of deducing the number of allowed objects.
    size_t GuessRestrictionSize(target_type tt) {
      size_t s = RestrictionSize(tt);
      return s ? s : Size();  // obs! this may be wrong...
    } 
    
    /// Class-wise allowed object restriction if any.
    const string& RestrictionName() const { return restriction.Label(); }

    /// Sets class-wise allowed objesct restriction.
    void RestrictionInner(ground_truth& gt, const string& n,
			  const string& s, bool verbose,
			  target_type tt, int other, bool expand);

    /// Sets class-wise allowed objesct restriction.
    void Restriction(const string& s, bool verbose = true);

    /// True if no restriction or object is included in it.
    bool OkWithRestriction(size_t i) const {
      return restriction.empty() || restriction.get(i)==1;
    }

    ///
    ground_truth RestrictionGT() const {
      if (restriction.Length())
	return restriction;
      ground_truth tmp("*", Size());
      tmp.Set(1);
      tmp.expandable(false);
      return tmp;
    }

    /// Gives object count numbers form tt_stat.
    int NumberObjects(target_type t) const {
      map<target_type,int>::const_iterator i = tt_stat.find(t);
      return i==tt_stat.end() ? 0 : i->second;
    }

    /// Tests all labels of the database.
    bool LabelTest();

    /// True if argument is valid object index in the database.
    bool IndexOK(size_t i) const { return _objects.index_ok(i); }

    /// Size of the database.
    size_t Size() const { return _objects.size(); }

    /// Clears all info about objects.
    void ClearObjects();

    /// Adds one new object in the database.
    object_info& AddObject(const string& l, target_type t);

    /// Shows them all, one per line.
    void DumpObjects() const { _objects.dump(cout); }

    /// Returns one object's full info.
    string ObjectDump(size_t i) const {
      stringstream ss;
      if (!IndexOK(i))
	ss << "*** object index " << i << " is invalid ***";
      else
	FindObject(i)->dump_nonl(ss);
      return ss.str();
    }

    /// Write access to object_info by index.
    object_info *FindObject(size_t i) { 
      static object_info dummy(this, 0, "00000000", target_imagefile);
      if (isdummydb)
	return &dummy;

      try {
	return &_objects.find(i);
      }
      catch (...) {
	ShowError("FindObject(", ToStr(i), ") failing");
	return NULL;
      }
    }

    /// Read-only access to object_info by index.
    const object_info *FindObject(size_t i) const { 
      return ((DataBase*)this)->FindObject(i);
    }

    /// Read-only access to object_info by label.
    const object_info *FindObject(const string& l) const { 
      return FindObject(LabelIndex(l));
    }

    /// Write access to object_info by label.
    object_info *FindObject(const string& l) { 
      return FindObject(LabelIndex(l));
    }

    /// Access to an object that is not really anything...
    static const object_info *DummyObject() { return &object_info::dummy(); }

    /// Returns label of i'th object in the database.
    // const char *LabelP(int i) const { return Label(i).c_str(); }

    /// Returns label of i'th object in the database.
    const string& Label(int i) const {
      try {
	return _objects.label(i);
      }
      catch (const out_of_range&) {
	ShowError("DataBase::Label() index ", ToStr(i), " not OK, Size()="
		  +ToStr(_objects.size()));
	static const string empty;
	return empty;
      }
    }

    ///
    string ZeroPadToLabel(const string& l) const {
      return string(LabelLength()-l.length(), '0')+l;
    }

    /// This fills _objects and tt_stat.
    bool ReadLabels(bool = false);

    /// This fills _objects and tt_stat.
    bool ReadLabelsClassical(bool = false);

    /// This fills _objects and tt_stat.
    bool ReadLabelsSql(bool = false);

    /// A helper for ones above.
    target_type TargetTypeTricks(target_type) const;

    /// A helper for ones above.
    void IncrementTargetTypeStatistics(target_type);

    ///
    bool ReadSubobjects() { 
      return ReadSimpleList(simplelist_subobjects); 
    }

    ///
    bool ReadDuplicates() {
      return ReadSimpleList(simplelist_duplicates); 
    }

    ///
    string SimpleListFilename(simplelist_type type) {
      switch (type) {
      case simplelist_duplicates:
	return "duplicates";
      case simplelist_subobjects:
	return "subobjects";
      case simplelist_proper_persons:
	return "person.dict";
      case simplelist_proper_locations:
	return "location.dict";
      case simplelist_commons:
	return "commons.dict";
      case simplelist_classfeatures:
	return "classfeatures";
      }
      return "";
    }
    
    string SimpleListFilename(simplelist_type type, const string& name) {
      string sl_name = name;
      if (sl_name.empty())
        sl_name = SimpleListFilename(type);
      return sl_name;
    }
  
    class SimpleList {
    public:
      SimpleList(bool is_set=true) {
	isset = is_set;
      }
      
      void insert(const pair<string,string>& ss) {
        sl_map.insert(ss);
      }

      void insert(const string& str) {
        sl_set.insert(str);
      }

      bool contains(const string& str) const {
	return sl_set.find(str)!=sl_set.end();
      }

      void clear() {
        sl_map.clear();
        sl_set.clear();
      }
      
      size_t size() const {
        return sl_set.size()+sl_map.size();
      }

      const set<string>& getSet() const {
	return sl_set;
      }

      const multimap<string,string>& getMap() const {
	return sl_map;
      }
      
    private:
      multimap<string,string> sl_map;
      set<string> sl_set;
      bool isset;
    }; // class SimpleList
    
    ///
    bool ReadSimpleListFile(const string& fname, 
			    list< list<string> >& ll,
			    bool words=false);

    ///
    bool ReadSimpleList(simplelist_type type, const string& name="");

    ///
    bool AppendSimpleListFile(const string& fname, const string& l);

    ///
    bool AppendSubobjectsFile(size_t, const vector<size_t>&);

    ///
    bool AppendSubobjectsFileSql(size_t, const vector<size_t>&);

    ///
    bool AppendSubobjectsFileClassical(size_t, const vector<size_t>&);

    /// May return target_type other than no_target, typically target_image
    /// for old datanbases that don't hold that information in .cod file
    target_type DefaultFeatureTarget() const { return default_feature_target; }

    ///
    target_type SolveTargetTypeFromLabel(const string& l,
					 bool segm_is_mod);

    /// Conditionally sets and returns defaultquery's query_target.
    target_type SolveDefaultTargetType();

    /// Looks object's target_type in its object_info.
    target_type ObjectsTargetType(size_t i) const {
      try {
	return _objects.find(i).type;
      }
      catch (...) {
	ShowError("DataBase::ObjectsTargetType() : illegal index (",
		  ToStr(i), ")");
	return target_no_target;
      }
    }

    /// Looks object's target_type in its object_info and returns string.
    string ObjectsTargetTypeString(size_t i) const {
      return TargetTypeString(ObjectsTargetType(i));
    }

    /// Checks if given set of bits are set in object's target_type.
    bool ObjectsTargetTypeContains(size_t i, target_type tt) const {
      return PicSOM::TargetTypeContains(ObjectsTargetType(i), tt);
    }

    /// This shows contents of tt_stat.
    void ShowTargetTypeStatistics() const;

    /// This makes a list<string> of tt_stat or such.
    list<string> TargetTypeStatisticsLong(const tt_stat_t&) const;

    /// This makes a list<string> of tt_stat.
    list<string> TargetTypeStatisticsLong() const {
      return TargetTypeStatisticsLong(tt_stat);
    }

    /// This makes a string of tt_stat or such.
    string TargetTypeStatistics(const tt_stat_t&) const;

    /// This fills parent/children subobject info in object_set.
    /// @param start_idx The info is calculated for objects start_idx..Size()-1
    bool MakeSubObjectIndex(int start_idx = 0);

    bool MakeSubObjectIndexSql(int start_idx);

    bool MakeSubObjectIndexClassical(int start_idx);

    /// Returns a vector of subobject indices.
    const vector<int>& SubObjects(int idx) const {
      try {
	return _objects.find(idx).children;
      }
      catch (...) {
	ShowError("DataBase::SubObjects(", ToStr(idx), ") failed");
	static const vector<int> empty;
	return empty;
      }
    }
    
    ///
    const vector<pair<size_t,size_t> >& 
    VideoOrSegmentFramesOrdered(size_t, bool = true);

    ///
    pair<pair<size_t,size_t>,pair<size_t,size_t> > 
    VideoOrSegmentFirstAndLastFrame(size_t, bool = true);

    ///
    pair<size_t,size_t> VideoOrSegmentMiddleFrame(size_t, bool = true);

    ///
    size_t MiddleFrameTrick(size_t, bool);

    ///
    string ParentObjectStringByPruning(const string& l) const;

    ///
    int ParentObjectByPruning(const string& l) const {
      string str = ParentObjectStringByPruning(l);
      int pidx = LabelIndexGentle(str);
      if (pidx<0)
	ShowError("DataBase::ParentObjectByPruning(", l, ") failed"
		  " with missing <", str, ">");

      return pidx;
    }

    // int ParentObjectByPruningOld(const string& l) const {
    //   string pname = LabelFileNamePart(l);
    //   int pidx = LabelIndex(pname);
    //   if (pidx<0)
    // 	ShowError("DataBase::ParentObjectByPruning(", l, ") failed");
    //   return pidx;
    // }

    int ParentObject(int i, bool warn = true) const {
      try {
	return _objects.find(i).default_parent();
      }
      catch (...) {
	if (warn)
	  ShowError("DataBase::ParentObject(", ToStr(i), ") failed");
	return -1;
      }
    }

    ///
    size_t RootParent(size_t idx) const {
      int p = ParentObject(idx, false);
      return p==-1 ? idx : RootParent(p);
    }

    ///
    pair<size_t,pair<double,double> > ParentStartDuration(size_t,
							  target_type);

    /// Checks object's parent and its children for matching types.
    bool ObjectHasLinkToTargetType(int, target_type) const;

    /// Returns list of labels by indices.
    vector<string> Labels(const IntVector& v) const {
      vector<string> ret;
      for (int i=0; i<v.Length(); i++) {
	ret.push_back(Label(v[i]));
	// cout << "#### " << i << " " << v[i] << " " << Label(v[i]) << endl;
      }
      return ret;
    }

    /// Returns list of labels by indices.
    vector<string> Labels(const vector<size_t>& v) const {
      vector<string> ret(v.size());
      for (size_t i=0; i<v.size(); i++)
	ret[i] = Label(v[i]);
      return ret;
    }

    /// Returns the index of data item carrying specified label.
    int LabelIndex(const string& l) const;

    /// Returns the index of data item carrying specified label.
    int LabelIndexGentle(const string& l, bool warn = true) const {
      //Tic("LabelIndexGentle");
      int r = -1;
      try {
	r = _objects.index_gentle(l);
      }
      catch (...) {
	if (warn)
	  ShowError("DataBase <"+name+"> LabelIndexGentle(", l, ") failed");
      }
      //Tac("LabelIndexGentle");
      return r; 
    }

    /// Returns the index of data item carrying specified label or #index.
    int ToIndex(const string& l) const {
      if (l[0]=='#' && l.find_first_not_of("0123456789", 1)==string::npos) {
	int idx = atoi(l.substr(1).c_str());
	return IndexOK(idx) ? idx : -1;
      }
      return LabelIndexGentle(l);
    }

    /// Same as the one above with provisional x,y after label.
    int LabelIndexGentleWithXY(const string& l, xymap_ve&) const;

    /// Inserts x,y coordinates in given class for given image.
    void InsertImageXYpoints(const string& cls, int idx, const xymap_ve& v) {
      all_xymaps[cls][idx] = v;
    }

    /// Access to x,y coordinates.
    bool HasXYmap(const string& cls) const {
      return all_xymaps.find(cls)!=all_xymaps.end();
    }

    /// Access to x,y coordinates.
    xymap_t& XYmap(const string& cls) {
      return all_xymaps[cls];
    }

    /// Access to x,y coordinates.
    const xymap_t& XYmap(const string& cls) const {
      static const xymap_t empty;
      map<string,xymap_t>::const_iterator i = all_xymaps.find(cls);
      return i==all_xymaps.end() ? empty : i->second;
    }

    /// Combines two x,y coordinate lists.
    bool CombineImageXYpoints(const ground_truth& gtl, const ground_truth& gtr,
			      const string& d) {
      if (gtl.Label()=="")
	return ShowError("Empty class label gtl in CombineImageXYpoints()");

      if (gtr.Label()=="")
	return ShowError("Empty class label gtr in CombineImageXYpoints()");

      const string left = gtl.Label(), right = gtr.Label();

      if (XYmap(left).empty() && XYmap(right).empty())
	return true;

      if (!XYmap(left).empty() && !XYmap(right).empty())
	return ShowError("Unable to combine two non-empty x,y maps");

      string cls = left+d+right;
      if (debug_gt)
	cout << "   * combining x,y lists to " << cls << endl;
      all_xymaps[cls] = XYmap(XYmap(left).empty() ? right : left);

      return true;
    }

    /// Checks the modification time and reads if necessary.
    ground_truth ConditionallyReadClassFile(const string&, bool, bool = false);

    /// Returns zero/one class membership vector from file.
    ground_truth ReallyReadClassFile(const string&, bool);

    /// Writes a "compiled" or "cooked" version of ground_truth.
    bool CompileClassFile(const ground_truth&, const string&);

    /// Reads in a file written by the above method.
    bool ReadCompiledClassFile(ground_truth&, const string&) const;

    /// Reads in a list of #deps in file written by the above method.
    list<string> CompiledClassFileDepends(const string&) const;

    /// A helper for the one above.
    pair<string,string> SolveClassFileNames(const string&) const;

    /// A helper for ReallyReadClassFile().
    bool MarkIndexRange(const string& first, const string& last,
			ground_truth& correct, int val) const;

    /// A helper for ReallyReadClassFile().
    bool MarkLabelRange(const string& first, const string& last,
			ground_truth& correct, int val) const;

    /// A helper for ReallyReadClassFile().
    bool IsClassDescription(const string& linestr) const {
      const char *line = linestr.c_str();
      return ((line[0]=='#' && line[1]==' ' && line[2]!='[' &&
	       strspn(line+1, " \t")!=strlen(line+1) &&
	       strncmp(line+2, "sock ", 5) &&
	       strncmp(line+2, "Subset of contents selection: [", 31) &&
	       strncmp(line+2, "Originally selected set was:  [", 31)));
    }

    /// Returns description of a class.
    string ReadClassDescription(const string&) const;
  
    ///
    list<pair<size_t,string> > ReadClassWithExtra(const string&) const;

    ///
    list<pair<size_t,pair<vector<size_t>,float> > > ReadClassWithBoxesValues(const string&) const;

    /// Writes class membership vector to file.
    bool WriteClassFile(const string& file, const ground_truth& set,
			const string& commstr = "",
			bool append = false) const {
      return WriteClassFile(file, vector<string>(), "", set, commstr, append, 
                            false);
    }

    /// Writes class membership vector to file.
    bool WriteClassFile(const string&, const vector<string>&, 
                        const string&, const ground_truth&,
			const string& = "",
                        bool = false, bool = false) const;

    /// Writes class membership vector to file.
    bool ConditionallyWriteClassFile(bool, const string&, const ground_truth&,
				     const string& = "", bool = false,
				     bool = false) const;

    /// Writes ordered class membership vector to file.
    bool WriteOrderedClassFile(const list<pair<size_t,double> >&, 
			       const string&, const string&,
			       bool, bool) const;

    /// Reads ordered class membership vector to file.
    vector<size_t> ReadOrderedClassFile(const string&, int = -1,
					bool = true) const;

    /// Sets the contentsname member.
    void ContentsName(const string& n) { contentsname = n; }

    /// Returns the value of the contentsname member.
    const string& ContentsName() const { return contentsname; }

    /// Reads the contents member from file specified by contentsname.
    bool ReadContents(bool, bool);

    /// Read access to content class.
    const ground_truth& Contents(size_t c) const { return contents[c]; }

    /// Returns zero/one if r'th item belongs to c'th content class.
    int Contents(size_t r, size_t c) const {
      if (contents.size()<=c || contents[c].size()<=r)
        return ShowError("DataBase::Contents(int,int) failing");

      return contents[c][r];
    }

    /// Returns the number of content classes.
    size_t ContentsClasses() const { return contents.size(); }

    /// Returns zero/one class memberships of r'th item.
    IntVector ContentsRow(int r) const { 
      IntVector ret(ContentsClasses());
      for (int c=0; c<ret.Length(); c++)
	ret[c] = Contents(r, c);

      return ret;
    }

    /// Adds a copy of ground truth and returns pointer.
    ground_truth *ContentsInsert(const ground_truth& g) {
      RwLockWrite("ContentsInsert");
      contents.insert(g);
      if (debug_gt)
	cout << "Inserted in contents [" << g.label() << "], now "
	     << contents.size() << " items" << endl;
      ground_truth *p = contents.find_ptr(g.label());
      RwUnlockWrite("ContentsInsert");

      return p;
    }

    /// Updates existing or adds new ground truth object and returns pointer.
    ground_truth *ContentsUpdateOrInsert(const ground_truth& g) {
      ground_truth *gt = ContentsFind(g.label());
      if (gt)
	*gt = g;
      else
	gt = ContentsInsert(g);

      return gt;
    }

    /// Resizes all ground truth vedtors to match the size of the database.
    void ContentsResize() {
      size_t s = Size();
      bool has_zeros = false;  // obs!?
      signed char val = has_zeros ? 0 : -1;
      contents.resize(s, val);
    }

    /// Returns name of i'th content class.
    const string& ContentsClassLabel(size_t i) const {
      return contents[i].Label();
    }

    /// Read access to ground truth data stored in contents.
    const ground_truth *ContentsFind(const string& s) const {
      return ((DataBase*)this)->ContentsFind(s);
    }

    /// Modify access to ground truth data stored in contents.
    ground_truth *ContentsFind(const string& s) {
      ground_truth *ret = contents.find_ptr(s);
      if (debug_gt)
	cout << "contents.find_ptr(" << s << ") "
	     << (ret?"FOUND":"NOT found") << " among "
	     << contents.size() << " items" << endl;
      return ret;
    }

    size_t ContentsErase(const string& s) {
      RwLockWrite("ContentsErase");
      size_t ret=0;
      for (size_t i=0; i<contents.size(); i++) {
	if (s.empty() || contents[i].label().find(s,0)==0) {
	  contents.erase(i);
	  ret++;
	}
      }
      RwUnlockWrite("ContentsErase");
      return ret;
    }

    /// This is called from Interpret with gt(xxx)=yyy lines.
    bool SetGroundTruth(const string& l, const string& e, 
			const string &t = "") {
      if (debug_gt)
	cout << "SetGroundTruth() : [" << l << "]=[" << e << "]" << endl;
      int o = -1;
      target_type tt = target_any_target;
      bool expand = true;
      ground_truth gt = GroundTruthExpression(e, tt, o, expand);
      gt.Label(l);
      gt.text(t);
      ContentsInsert(gt);
      string f = ExpandPath("classes", l);
      classfiles_t::iterator i = AddClassFileEntry(f);
      i->second.WasRead();
      return true;
    }

    /** Extracts the ground truth information from the xml node and calls the
     *  other SetGroundTruth() function.
     */
    bool SetGroundTruth(const xmlNodePtr &n);

    ///
    typedef map<string,pair<string,list<string> > > featurealias_t;

    ///
    typedef map<string,pair<string,list<string> > > featureaugmentation_t;

    /// Interprets, expands and saves the given feature alias expression
    bool SetFeatureAlias(const string &n, const string &e, 
			 const string &t = "");

    /// Extracts the feature alias info from the node and calls SetFeatureAlias
    bool SetFeatureAlias(const xmlNodePtr &n);

    /// Interprets, expands and saves the given feature augmentation expression
    bool SetFeatureAugmentation(const string &n, const string &e, 
			 const string &t = "");

    /// Extracts the feature augmentation info from the node and calls SetFeatureAugmentation
    bool SetFeatureAugmentation(const xmlNodePtr &n);

    ///
    bool PossiblyImplementFeatureAugmentation(const string&);

    ///
    const list<string>& FeatureRegExpList(const string&);

    ///
    bool SetTextQueryOption(const xmlNodePtr &n);
    
    ///
    bool SetClassAugment(const xmlNodePtr &n);

    /// Set text query processing log
    void TextQueryLog(const string& log) { textquerylog = log; }

    /// Get reference to text query processing log
    const string& TextQueryLog() { return textquerylog; }

    /// Expands if the argument is featurealias.
    list<string> ExpandFeatureAlias(const string& n);

    /// Dumps the feature aliases
    void DumpFeatureAlias() const {
      for (featurealias_t::const_iterator i=
	    featurealias.begin(); i!= featurealias.end(); i++) {
	cout << "Featurealias: " << i->first << endl 
	     << " Description:" << endl 
	     << "  " << i->second.first << endl 
	     << " Features:" << endl;
	for (list<string>::const_iterator j=i->second.second.begin(); 
	    j!=i->second.second.end(); j++) 
	  cout << "  " << *j << endl;
      }
    }

    ///
    void DumpIndexList(ostream&) const;

    ///
    typedef map<string,pair<string,list<string> > > namedcmdline_t;

    /// Interprets and saves the given named command line expression
    bool SetNamedCmdLine(const string &n, const string &e, 
			 const string &t = "") {
      vector<string> param = SplitInSpaces(e);
      list<string> l;
      for(vector<string>::iterator i=param.begin(); i!=param.end(); i++)
	l.push_back(*i);

      namedcmdline[n] = pair<string,list<string> >(t,l);
      return true;
    }

    /// Extracts the namedcmdline info from the node and calls SetNamedCmdLine
    bool SetNamedCmdLine(const xmlNodePtr &n);

    /// Dumps the namedcmdline structure
    void DumpNamedCmdLine() {
      for(namedcmdline_t::iterator i=
	    namedcmdline.begin(); i!= namedcmdline.end(); i++) {
	cout << "NamedCmdLine: " << i->first << endl 
	     << " Description:" << endl 
	     << "  " << i->second.first << endl 
	     << " Options:" << endl;
	for(list<string>::iterator j=i->second.second.begin(); 
	    j!=i->second.second.end(); j++) 
	  cout << "  " << *j << endl;
      }
    }


    /// Returns zero/one vector describing class membership.
    /// The string argument can be either class name or class membership file.
    ground_truth GroundTruth(const string&, target_type, int, bool);

    /// $func(arg1,arg2,...) type ground_truth expressions.
    ground_truth GroundTruthFunction(const string&, target_type);

    /// A helper function for the ones below.
    ground_truth GroundTruthCommons(const ground_truth& gt, target_type tt,
				    const string& f, const string& e,
				    const vector<string>& a) {
      ground_truth gtx = GroundTruthApplyOperations(tt, gt, a);
      GroundTruthSetLabel(gtx, f, e, a);
      gtx.expandable(false);

      if (debug_gt)
	GroundTruthSummaryTable(gtx);

      return gtx;
    }

    /// A helper function for the one above.
    void GroundTruthSetLabel(ground_truth& gtx, const string& f,
			     const string& e, const vector<string>& a) {
      gtx.Label("$"+f+"("+e+CommaJoin(a, true)+")");
    }

    /// Sets ground truth according to given expression.
    ground_truth GroundTruthEye(target_type, const string&, const string&,
				const vector<string>&);

    /// Expands ground truth to subobjects.
    ground_truth GroundTruthExp(target_type, const string&, const string&,
				const vector<string>&);

    /// Set ground truth on target_type.
    ground_truth GroundTruthType(target_type, const string&, const string&,
				 const vector<string>&);

    /// Ground truth by regular expressions on labels.
    ground_truth GroundTruthRegExp(target_type, const string&, const string&,
				   const vector<string>&);

    /// Ground truth by parent/child/sibling relations.
    ground_truth GroundTruthParents(target_type, const string&, const string&,
				   const vector<string>&);
    
    /// Ground truth by parent/child/sibling relations.
    ground_truth GroundTruthChildren(target_type, const string&, const string&,
				     const vector<string>&);
    
    /// Ground truth by parent/child/sibling relations.
    ground_truth GroundTruthSiblings(target_type, const string&, const string&,
				     const vector<string>&);
    
    /// Ground truth by parent/child/sibling relations.
    ground_truth GroundTruthNchildren(target_type, const string&, const string&,
				      const string&, const vector<string>&);
    
    /// Ground truth by master/duplicates relations.
    ground_truth GroundTruthDuplicates(target_type,const string&,const string&,
				       const vector<string>&);
    
    /// Ground truth by master/duplicates relations.
    ground_truth GroundTruthMasters(target_type, const string&, const string&,
				    const vector<string>&);
    
    /// Ground truth by data vector values.
    ground_truth GroundTruthData(target_type, const string&, const string&,
				 const vector<string>&);

    /// Sets ground truth according to given date expression.
    ground_truth GroundTruthDate(target_type, const string&, const string&);

    /// Sets ground truth by splitting the given ground truth.
    ground_truth GroundTruthSplit(target_type, const string&, const string&,
				  const string&, size_t,size_t, const string&);

    /// Sets ground truth by counting number of keywords found for each object.
    ground_truth GroundTruthNkeys(target_type, const string&, const string&,
				  const string&, const vector<string>&);

    /// Sets ground truth by subtracting all other class from the given one.
    ground_truth GroundTruthOnly(target_type, const string&, const string&,
				 const string&, const vector<string>&);

    /// Sets ground truth by selecting that class which first matches.
    ground_truth GroundTruthSwitch(target_type, const string&, const string&,
				   const string&, const vector<string>&);

    /// Sets ground truth by reding first lines from ordered class file.
    ground_truth GroundTruthHead(target_type, const string&, const string&,
				 const string&, const vector<string>&);

    /// Sets ground truth by taking a smaller random sample from the given one.
    ground_truth GroundTruthRandcut(target_type, const string&, const string&,
                                    const string&, const string&,
                                    const vector<string>&);

    /// Inner function for doing Randcut
    ground_truth GTRandcut(const ground_truth&, int, int=42);

    /// Sets ground truth by text index.
    ground_truth GroundTruthText(target_type, const string&, const string&,
				 const vector<string>&);

    /// Sets ground truth by SQL field values.
    ground_truth GroundTruthSql(target_type, const string&, const string&,
				 const vector<string>&);

    /// Sets ground truth by a label range.
    ground_truth GroundTruthLabelRange(target_type, const string&,
				       const string&, const string&,
				       const vector<string>&);

    /// Sets ground truth by a index range.
    ground_truth GroundTruthIndexRange(target_type, const string&,
				       const string&, const string&,
				       const vector<string>&);

    /// Maps lscomXXX to TRECVID's textual class names
    ground_truth GroundTruthLscom2Trecvid(target_type, const string&,
					  const string&, const string&,
					  const string&, const vector<string>&);

    /// Returns indices oof video (segment) middle frames.
    ground_truth GroundTruthMiddleFrame(target_type, const string&,
					const string&, const vector<string>&);

    /// Returns objects by their auxid.
    ground_truth GroundTruthAuxid(target_type, const string&, const string&,
				  const string&, const vector<string>&);

    /// Performs some straightforward expansions, etc.
    ground_truth GroundTruthApplyOperations(target_type, const ground_truth&,
					    const vector<string>&);

    /// Expands GroundTruth to subobjects.
    ground_truth GroundTruthExpandNew(const ground_truth&, 
                                      const vector<string>&) const;
                                      
    ///                                      
    ground_truth GroundTruthExpandNew(const ground_truth& gt) const {
      vector<string> a;
      return GroundTruthExpandNew(gt,a);
    }

    ///
    set<int> NumberRangesToSet(const vector<string>& a) const;

    ///
    vector<int> ExpandDownOneLevel(ground_truth& ret, const vector<int> oldidx, 
				   int nval, bool keep_parent, 
				   bool set_child) const;

    /// Expands GroundTruth to subobjects.
    bool GroundTruthExpandOld(ground_truth&) const;

    /// Expands GroundTruth to subobjects.
    bool GroundTruthExpandOld_slow(ground_truth&) const;

    /// Expands GroundTruth to subobjects.
    bool GroundTruthExpandOld_fast(ground_truth&) const;

    /// This also handles set operators like | & ^ ! \ ~ ().
    ground_truth GroundTruthExpression(const string&, target_type, int, bool);

    /// New interface to the abobe one.
    ground_truth GroundTruthExpression(const string& s) {
      return GroundTruthExpression(s, target_any_target, -1, false);
    }

    /// All object labels on one line ala WORDINVERSEFILE.
    ground_truth GroundTruthLine(const string&);

    /// Comma-separated string to a vector ground_truths.
    ground_truth_set GroundTruthExpressionListNew(const string&);

    /// Comma-separated string to vector set.
    /// could this and SelectionList() be obsoleted ?
    /// could we use SplitClassNames() somehow instead ?
    ground_truth_set GroundTruthExpressionListOld(const string&, bool);

    /// Returns cardinalities of "+1" and "+/-1" sets obtained
    /// as intersections of the given ground_truth and object_type.
    pair<size_t,size_t>GroundTruthTypeCount(const ground_truth&,
					    target_type) const;

    /// Useful for debugging purposes.
    string GroundTruthSummaryStr(const ground_truth&, bool, bool, bool,
				 float = numeric_limits<float>::max()) const;

    /// Useful for debugging purposes.
    void GroundTruthSummary(const ground_truth& g, ostream& s = cout) const {
      GroundTruthSummaryTable(g, false, false, s);
    }

    /// Useful for debugging purposes.
    void GroundTruthSummaryLine(const ground_truth& g, bool t = false,
				bool d = false, ostream& s = cout) const {
      s << "ground_truth: label=" << GroundTruthSummaryStr(g, false, t, d)
	<< flush;
    }

    /// Useful for debugging purposes.
    void GroundTruthSummaryTable(const ground_truth& g, bool t = false,
				bool d = false, ostream& s = cout) const {
      s << "ground_truth: label=" << GroundTruthSummaryStr(g, true, t, d)
	<< flush;
    }

    /// Useful for debugging purposes.
    string GroundTruthSummaryStr(const aspect_map_t&, bool, bool, bool) const;

    /// Useful for debugging purposes.
    void GroundTruthSummary(const aspect_map_t& g, ostream& s = cout) const {
      GroundTruthSummaryTable(g, false, false, s);
    }

    /// Useful for debugging purposes.
    void GroundTruthSummaryLine(const aspect_map_t& g, bool t = false,
				bool d = false, ostream& s = cout) const {
      s << GroundTruthSummaryStr(g, false, t, d) << flush;
    }

    /// Useful for debugging purposes.
    void GroundTruthSummaryTable(const aspect_map_t& g, bool t = false,
				bool d = false, ostream& s = cout) const {
      s << GroundTruthSummaryStr(g, true, t, d)	<< flush;
    }

    ///
    bool ReadURLBlacklist();

    ///
    bool MapReverseURLs(const ground_truth&, bool);

    ///
    int ReverseURL(const string& url) const {
      map<string,int>::const_iterator i = url_to_index.find(url);
      return i!=url_to_index.end() ? i->second : -1;
    }
    
    ///
    bool URLIsBlacklisted(const string& url) const;

    /// Writes strings to log.
    void WriteLog(const string& m1, const string& m2 = "",
		  const string& m3 = "", const string& m4 = "", 
		  const string& m5 = "") const {
      pic_som->WriteLogStr(LogIdentity(), m1, m2, m3, m4, m5); }

    /// Writes contents of a stream to log.
    void WriteLog(ostringstream& os) const {
      pic_som->WriteLogStr(LogIdentity(), os); }

    /// Gives log form of DataBase name.
    string LogIdentity() const {
      ostringstream s;
      s // << (void*)this << " "
	<< RwLockStatusChar() << '*' << name << '*';
      return s.str();
    }

    /// Up reference.
    PicSOM *Picsom() const { return pic_som; }

    /// Throws away unneeded data.
    void ReduceMemoryUse() {
      for (size_t i=0; i<Nindices(); i++)
	GetIndex(i)->ReduceMemoryUse(); 
    }

    /// Finds all features and returns the number.
    size_t FindAllIndices(bool = true);

    /// Finds all features and returns the number.
    size_t FindAllIndicesClassical(bool);

    /// Finds all features and returns the number.
    size_t FindAllIndicesSql();

    /// A helper for the one above.
    bool SqlFindIndex(const string&);

    ///
    vector<pair<string,string> > SqlParseSpec(const string&) const;

    /// Data source mappings for url://xxx rules
    bool SolveUrlRules();

    ///
    void TextIndexStoreLine(const string& l) {
      textindices_stored_lines.push_back(l);
    }

    /// Finds information about text indices.
    bool SolveTextIndices();

    /// Finds information about one text index.
    bool DescribedTextIndex(const XmlDom&);

    /// Connection to a picsom-lucene text index.
    Connection *LuceneTextIndex(const string&);

    /// Closel all textindices.
    bool CloseTextIndices();

    /// Performs one query operation with picsom-lucene text index.
    pair<bool,list<string> > LuceneOperation(const string&, const list<string>&,
					     bool = true);

    /// given "3" returns ["--lucene" "3.6.1" "--root" "..."] or something
    vector<string> SolvePicSOMLuceneArguments(const string&);

    /// Performs one query operation with picsom-lucene text index.
    string LuceneVersion(const string&);

    ///
    static bool ForceLuceneUnlock() { return force_lucene_unlock; }

    ///
    static void ForceLuceneUnlock(bool f) { force_lucene_unlock = f; }

    ///
    static bool ParseExternalMetadata() { return parse_external_metadata; }

    ///
    static void ParseExternalMetadata(bool v) { parse_external_metadata = v; }

    /// Finds features which don't have maps but are specified in settings.xml.
    bool SolveExtractions();

    /// Creates a feature extraction as specified in settings.xml.
    bool DescribedIndex(const XmlDom&);

    /// Creates a feature extraction as specified in settings.xml.
    bool DescribedIndexComplex(const XmlDom&);

    /// Adds /path/features/ in feature xetraction options read from setting2.
    string PathifyFeatureOption(const string&, const string&) const;

    /// Gaze coordinate normaliztion routines.
    double NormalizeGazeCoordInCollage(double, const string&);

    /// Logistic regression or something on gaze features.
    double GazeRelevance(const vector<float>&, const string&);

    /// Logistic regression or something on pointer features.
    double PointerRelevance(const vector<float>&, const string&);

    ///
    bool ReadGazeRelevanceGroupData();

    ///
    float GazeRelevanceGroupValue(simple::RandVar&, bool, size_t);

    /// Creates segmentation command.
    bool DescribedSegmentation(const XmlDom&);

    /// Creates object insertion method.
    bool DescribedObjectInsertion(const XmlDom&);

    /// Creates detection method.
    bool DescribedDetection(const XmlDom&);

    ///
    bool ExpandAndStoreDescribedDetections(const string&,
					   const list<pair<string,string> >&);

    ///
    list<pair<string,list<pair<string,string> > > >
    ExpandDescription(const string&, const list<pair<string,string> >&,
		      bool);

    ///
    list<pair<string,list<pair<string,string> > > >
    ExpandDescriptionInner(const string&, const list<pair<string,string> >&,
			   bool);

    ///
    list<pair<string,string> >
    ExpandDescriptionVariables(const list<pair<string,string> >&, bool, bool);

    /// Creates captioning method.
    bool DescribedCaptioning(const XmlDom&);

    /// Creates media extraction method.
    bool DescribedMedia(const XmlDom&);

    /// Returns a feature extraction programs output file name from map name.
    string FeatureOutputName(const string& mn) const {
      map<string,string>::const_iterator i = index_to_featurefile.find(mn);
      return i==index_to_featurefile.end() ? "" : i->second;
    }

    /// Returns a feture extraction method name from output file name.
    string FeatureMethodName(const string& mn) const {
      map<string,string>::const_iterator i = featurefile_to_method.find(mn);
      return i==featurefile_to_method.end() ? "" : i->second;
    }

    /// Finds names of all classes.
    list<string> FindAllClassNames(bool force, bool also_contents,
				   bool also_meta);

    /// Finds all classes and stores them in contents.
    bool FindAllClasses(bool, bool);

    /// Returns true if argument names a METACLASSFILE.
    bool IsMetaClassFile(const string&) const;

    /// Splits classname by commas.
    list<string> SplitClassNames(const string&) /*const*/;

    /// Returns names of all known classes.
    list<string> KnownClassNames() const;

    /// Process one line of access file.
    bool ProcessAccessEntry(const char*);

    /// Creates and returns a new RemoteHost object 
    /// (Memory allocated with new -- remember to delete!)
    RemoteHost* GetNewRemoteHost(bool,const string,const string,
					 const char*, map<string,string>*);

    /// Adds a new RemoteHost object to the linked list pointed to by
    /// the given RemoteHost pointers (first and last elements of the list,
    /// both NULL if list is empty).
    /// Returns true if successful
    bool AddNewRemoteHost(bool,const string,const string,
			  const char*, map<string,string>*,
			  RemoteHost**, RemoteHost**);

    /// Parses the content of a given <right> node, and returns a
    /// key-value pair containing the right name and it's optional parameter.
    pair<string,string> ParseRightString(const char* rightstring);

    /// Process one <rule> of access file.
    bool ProcessAccessEntryXML(xmlNodePtr);

    /// Process access rules fromXML.
    bool ApplyAccessRulesXML(xmlNodePtr);

    /// Process aspect rules fromXML.
    bool ApplyDefaultAspectsXML(xmlNodePtr);

    /// Process one <aspects> node in the aspect list
    bool ProcessAspectsEntryXML(xmlNodePtr);

    /// Returns true if client of query is allowed to use this.
    bool CheckPermissions(const Connection&) const;

    /// Returns true if the client of query has permission to do 
    /// the given thing
    bool IsAllowed(const Connection*, const string&) const;

    /// Sends any object to connection.
    bool SendObject(Connection&, object_type, const char*, const char* = NULL);

    /// Sends any object to connection.
    bool SendObject(Connection&, const char*, const char*,
		    const char*, const char*);

    ///
    bool SendVirtualObject(Connection&, object_type, const char*, const char*);

    ///
    bool CreateVirtualImage(ostream&, const string&, const string&) const;

    ///
    imagedata CreateTimelineImage(size_t, const string&);

    ///
    imagedata CreateImage(const string&, const string&) const;

    ///
    imagedata CreateVirtualImage(size_t idx, const string& spec) const {
      return CreateVirtualImage(Label(idx), spec);
    }

    ///
    imagedata CreateVirtualImage(const string&, const string&,
				 const string& = "") const;

    ///
    imagedata CreateVirtualImage8plus8(const string&, const string&) const;

    /// Returns a list of keywords, ie. classes where the object belongs.
    keyword_list FindKeywords(size_t, bool, bool);

    /// Gives path.
    const string& Path() const { return path; }

    /// Gives secondary path.
    const string& SecondaryPath() const { return secondarypath; }

    /// Sets secondary path.
    void SecondaryPath(const string& p) { secondarypath = p; }

    /// Lengths of the subdirectory parts of the labels, trad is 2+2+2+2.
    bool LabelFormat(const string&);

    /// Lengths of the subdirectory parts of the labels, trad is 2+2+2+2.
    const vector<size_t>& LabelFormat() const { return labelformat; }

     /// Lengths of the subdirectory parts of the labels, trad is 2+2+2+2.
    string LabelFormatStr(bool tot) const { 
      string labfmt;
      for (size_t i=0; i<labelformat.size(); i++)
	labfmt += string(i?"+":"")+ToStr(labelformat[i]);
      return tot ? labfmt+"="+ToStr(LabelLength()) : labfmt;
    }

    ///
    size_t LabelLength() const {
      size_t l = 0;
      for (size_t i=0; i<labelformat.size(); i++)
	l += labelformat[i];
      return l;
    }

    ///
    size_t ObjectDirectoryWidth(size_t i, bool total) const {
      if (!total) {
	if (i>=labelformat.size())
	  return 0;
	else {
	  size_t m = 1;
	  for (size_t j=0; j<labelformat[i]; j++)
	    m *= 10;
	  return m;
	}
      }

      size_t m = 1;
      for (size_t j=0; j<=i; j++)
	m *= ObjectDirectoryWidth(j, false);
      return m;
    }

    /// Solves directory path to an object.
    string SolveDirectory(const string& h, const string& l) const;

    /// Solves directory path to an object inside images/objects tree.
    string SolveObjectDirectory(const string& l) const {
      return SolveDirectory(objdir, l);
    }

    /// Solves the full path of an object or its parent.
    string SolveObjectPath(const string&, const string& = "",
			   const string& = "", bool = false,
			   bool* = NULL, bool = false,
			   bool = true) const;

    /// Solves the full path of an object or its parent.
    string SolveObjectPathSql(const string&, const string& = "",
                              const string& = "", bool = false,
                              bool* = NULL, bool = false,
                              bool = true) const;

    /// Solves the full path of an object or its parent.
    string SolveObjectPathClassical(const string&, const string& = "",
                                    const string& = "", bool = false,
                                    bool* = NULL, bool = false,
                                    bool = true) const;

    /// Solves the full path of an object.
    string SolveObjectPathSubDir(const string&, const string&,
				 bool* = NULL) const;

    /// Solves the full path of an object or its parent.
    string SolveObjectPathOrigins(const string&, const string&,
				  const string&, bool) const;

    /// A short-cut for CalculateFeatures() stuff.
    string CalculatedFeaturePath(const string& label, bool isnew) const {
      return SolveObjectPath(label, "features", "", isnew);
    }

    /// A short-cut for CalculateFeatures() stuff.
    string InsertedObjectPath(int idx, bool isnew, bool may_fail,
			      bool must_exist) const {
      return SolveObjectPath(Label(idx), "", "", isnew, NULL,
			     may_fail, must_exist);
    }

    ///
    string SqlTempFileName(int);

    ///
    bool RootlessDownloadObject(const string&, const string&) const;

    ///
    bool RootlessDownloadClassFile(const string&, bool) const;

    ///
    bool RootlessDownloadFile(const string&, bool) const;

    ///
    bool PossiblyDownloadFeatureData(const string&);

    /// A short-cut for CalculateFeatures() stuff.
    string ObjectPathEvenExtract(size_t, bool = true) /*const*/;

    /// A short-cut for CalculateFeatures() stuff.
    string ObjectPathEvenExtractOld(size_t, bool = true) /*const*/;

    /// A short-cut for CalculateFeatures() stuff.
    list<string> ObjectPathsEvenExtract(const vector<size_t>&, bool = true);

    ///
    string SolveSegmentFilePath(int, const string&, bool) const;

    /// OCR results for one video, possibly combining from children.
    list<pair<double,string> > OCRlinesOrCombine(size_t idx, const string&,
						 bool combine);
 
    /// ASR results for one video, possibly combining from children.
    list<textline_t> ASRlinesOrCombine(size_t idx, bool combine);

    ///
    float VideoFrameRate(size_t);

    ///
    float ParentRelativeStartTime(size_t);

    ///
    float VideoDuration(size_t);

    /// Converts onject path to OSRS file path in /00000000.d/
    string OSRSfileName(const string&);

    ///
    list<textline_t> OSRSraw(const string&, string&);

    ///
    list<textline_t> OSRScooked(const string&, string&);

    /// Reads speech recognition output.
    list<textline_t> ReadOSRS(const string&);

    /// Combines and splits ASRlines into proper chunks.
    list<textline_t> SplitASR(const list<textline_t>&);

    /// Processes one sentence of ASR output.
    list<textline_t> ProcessASRrange(list<textline_t>::const_iterator,
				  list<textline_t>::const_iterator);

    /// Used for ASS subtitles.
    static string ASStimeStr(double);

    /// Used for VTT subtitles.
    static string VTTtimeStr(double);

    /// Returns image data for given index.
    imagedata ImageData(size_t, bool = false, string* = NULL);

#ifdef USE_MRML
    /// HASH table that maps http_path (original file path+name) into picsom
    /// filename
    typedef map<string,string> originalfname_type;

    /// Reads original file locations into hashtable
    bool ReadOriginsIntoHash();
    
    /// Finds element from (originalpath, filename) hash table
    // const char *FindFnameFromHash__(const char *httppath) {
    //   const char *tmp = NULL;
    // 	tmp = originalfname[httppath].c_str();
    //   return tmp;
    // }

    /// Finds element from (originalpath, filename) hash table
    const string& FindFnameFromHash(const string& httppath) const {
      static const string empty;
      originalfname_type::const_iterator i = originalfname.find(httppath);
      return i!=originalfname.end() ? i->second : empty;
    }

    /// Retrieves full HTTP-path of image by its label
    string ImgHTTPpath(const string& imglab) {
      string path = SolveObjectPath(imglab);
      if (path=="") {
	ShowError("DataBase::ImageHTTPpath: Could not find path for ", imglab);
	return "";
      }      
      return string("file:")+path;
    }    
    
#endif // USE_MRML

    /// Sets force compile
    static void ForceCompile(bool d) { force_compile = d; }

    ///
    bool NoFeatureData() const { return nofeaturedata; }
    
    ///
    bool UseBinFeaturesRead() const { return use_bin_features_read; }

    ///
    bool UseBinFeaturesWrite() const { return use_bin_features_write; }

    ///
    bool UploadBinFeatures(VectorIndex&);

    ///
    static bool OpenReadWrite(const string&);

    ///
    static void OpenReadWriteSql(bool m) { open_read_write_sql = m; }

    ///
    static bool OpenReadWriteSql() { return open_read_write_sql; }

    ///
    static void OpenReadWriteFea(bool m) { open_read_write_fea = m; }

    ///
    static bool OpenReadWriteFea() { return open_read_write_fea; }

    ///
    static void OpenReadWriteDet(bool m) { open_read_write_det = m; }

    ///
    static bool OpenReadWriteDet() { return open_read_write_det; }

    ///
    static void OpenReadWriteTxt(bool m) { open_read_write_txt = m; }

    ///
    static bool OpenReadWriteTxt() { return open_read_write_txt; }

    /// Sets debugging of the use of mutex locks.
    static void DebugLocks(bool d) { debug_locks = d; }

    /// Sets debugging in GroundTruth*() creation.
    static void DebugGroundTruth(bool d) { debug_gt = d; }

    /// Checks debugging in GroundTruth*() creation.
    static bool DebugGroundTruth() { return debug_gt; }

    /// Sets trapping in GroundTruthExpand().
    static void DebugGroundTruthExpand(bool d) { debug_gt_exp = d; }

    /// Sets debugging in all virtual image creation.
    static void DebugVirtualObjects(bool d) { debug_vobj = d; }

    /// Checks debugging in all virtual image creation.
    static bool DebugVirtualObjects() { return debug_vobj; }

    /// Sets debugging in all thumbnail operations.
    static void DebugThumbnails(bool d) { debug_tn = d; }

    /// Sets debugging in access to origins files.
    static void DebugOrigins(bool d) { debug_origins = d; }

    /// Returns state of debugging in access to origins files.
    static bool DebugOrigins() { return debug_origins; }

    /// Sets debugging in access to SQL.
    static void DebugSql(bool d) { debug_sql = d; }

    /// Returns state of debugging in access to SQL.
    static bool DebugSql() { return debug_sql; }

    /// Sets debugging in access to TEXT.
    static void DebugText(bool d) { debug_text = d; }

    /// Returns state of debugging in access to TEXT.
    static bool DebugText() { return debug_text; }

    /// Sets debugging in access to origins files.
    static void DebugObjectPath(bool d) { debug_opath = d; }

    /// Sets debugging in access to leaf directory file searches.
    static void DebugLeafDir(size_t d) { debug_leafdir = d; }

    /// Checks debugging in access to leaf directory file searches.
    static size_t DebugLeafDir() { return debug_leafdir; }

    /// Sets debugging in ReadLabels/MakeSubObjectIndex().
    static void DebugDumpObjs(bool d) { debug_dumpobjs = d; }

    /// Sets debugging in *Detect*().
    static size_t DebugDetections() { return debug_detections; }

    /// Sets debugging in *Detection*().
    static void DebugDetections(size_t d) { debug_detections = d; }

    /// Sets debugging in *Detect*().
    static size_t DebugCaptionings() { return debug_captionings; }

    /// Sets debugging in *Captioning*().
    static void DebugCaptionings(size_t d) { debug_captionings = d; }

    /// Sets debugging of read/write of class files.
    static void DebugClassFiles(bool d) { debug_classfiles = d; }

    /// Debugging levels for features program etc.
    static size_t DebugFeatures() { return debug_features; }

    /// Debugging levels for features program etc.
    static void DebugFeatures(size_t v) { debug_features = v; }

    /// Debugging levels for segmentation program etc.
    static size_t DebugSegmentation() { return debug_segmentation; }

    /// Debugging levels for segmentation program etc.
    static void DebugSegmentation(size_t v) { debug_segmentation = v; }

    /// Debugging levels for images program etc.
    static size_t DebugImages() { return debug_images; }

    /// Debugging levels for images program etc.
    static void DebugImages(size_t v) { debug_images = v; }

    /// Whether to use mplayer.
    static bool InsertMplayer() { return insert_mplayer; }

    /// Whether to use mplayer.
    static void InsertMplayer(bool v) { insert_mplayer = v; }

    /// Gives access to defaultquery.
    Query& DefaultQuery() const { return *defaultquery; }

    /// Starts ticking.
    void Tic(const char *f) const { pic_som->Tic(f); }

    /// Stops tacking.
    void Tac(const char *f = NULL) const { pic_som->Tac(f); }

    /// Adds database object to the XML document.
    bool AddToXML(XmlDom& dom, object_type ot, const string& a, 
		  const string& b, Query *q, const Connection *c);

    /// Adds database object to the XML document.
    bool AddToXMLdata(XmlDom&, object_type, const string&, const string&,
                      bool, bool);

    /// Adds a virtual database object to the XML document.
    bool AddToXMLvirtual(XmlDom&, object_type, const string&, const string&);

    /// Adds a thumbnail object to the XML document.
    bool AddToXMLthumbnail(XmlDom&, const string&, const string&);

    /// Helper routine for the one above.
    bool TryToAddToXMLthumbnail(XmlDom&, const string&, const string&);

    /// Adds a .eml object to the XML document
    bool AddToXMLmessage(XmlDom&, const string&, const string&);

    /// Adds a .txt object to the XML document (BASE64-encoded)
    bool AddToXMLplaintext(XmlDom&, const string&, const string&);

    /// Adds an object (local copy of a full object) to the XML document.
    bool AddToXMLobject(XmlDom&, const string&, const string&);

    /// Adds databaseinfo object to the XML document.
    bool AddToXMLdatabaseinfo(XmlDom&, const string&, const Connection*,
                              bool, bool);

    /// Adds featurelist object to the XML document.
    bool AddToXMLfeaturelist(XmlDom&, const Query*) const;

    /// Adds algorithmlist object to the XML document.
    bool AddToXMLalgorithmlist(XmlDom&, const Query*);

    /// Adds the info about the arrived objects (using
    /// addToXMLobjectinfo) to the XML document
    bool AddToXMLarrivedobjectinfo(XmlDom&, /*const*/ Query*,
                                   const Connection*);

    /// Adds text to the XML document for a text object.
    bool AddToXMLtextinfo(XmlDom&, const string&) const;

    /// Adds objectinfo object to the XML document.
    bool AddToXMLobjectinfo(XmlDom&, const string&, Query*,
			    bool, bool, bool, bool,
                            XmlDom *extra, const string&) const;

    /// Adds imageinfo object to the XML document if image is virtual.
    bool AddToXMLimageinfovirtual(XmlDom&, const string&, Query*,
				  XmlDom*) const;

    /// Adds imageinfo object to the XML document if image is of type
    /// 8d+8d
    bool AddToXMLimageinfo8plus8(XmlDom&, const string&, Query*, XmlDom*) const;

    ///
    bool AddToXMLframeinfo(XmlDom&, const string&, const string&) const;

    /// 
    bool AddToXMLsegmentinfo(XmlDom&, const string&, const string&) const;

    /// Adds extra XML info to objectinfo object.
    bool AddToXMLextraobjectinfo(XmlDom&, const string&) const;

    /// Adds message text (label.d/message.txt) to objectinfo object.
    bool AddToXMLobjecttext(XmlDom&, const string&) const;

    /// Adds texts keywords etc from available textindices eg. lucene
    bool AddToXMLtextindextexts(XmlDom&, size_t) const;

    /// Adds <subobjects> list to <objectinfo> object and adds 
    /// an <objectinfo> for each subobject to the <subobjects> list.
    bool AddToXMLsubobjects(XmlDom&, const string&, Query*, bool, bool,
			    const string&) const;

    /// Adds origins file information to objectinfo object.
    bool AddToXMLoriginsobjectfileinfo(XmlDom&, XmlDom&,
				       size_t, string&, Query*) const;

    /// Adds information about availability of the object.
    bool AddToXMLavailabilityinfo(XmlDom&, const string&, Query*) const;

    /// Adds contentitems object to the XML document.
    bool AddToXMLcontentitemlist(XmlDom&);

    /// Adds contentitems object to the XML document.
    bool AddToXMLcontentitemlist_random(xmlNodePtr, xmlNsPtr);

    /// Adds contentitems object to the XML document.
    bool AddToXMLcontentitem(xmlNodePtr, xmlNsPtr, const string&,
			     int, int, int, const string&);

    /// Adds contentitems object to the XML document.
    bool AddToXMLcontentitem(xmlNodePtr p, xmlNsPtr n, const ground_truth& g) {
      return AddToXMLcontentitem(p, n, g.Label(),
				 g.NumberOfEqual(1), g.NumberOfEqual(-1),
				 Size(), g.text());
    }

    /// Adds segmentationinfo object to the XML document.
    bool AddToXMLsegmentationinfo(XmlDom&, const string&, const string&) const;

    /// Adds segmentationinfolist object to the XML document.
    bool AddToXMLsegmentationinfolist(XmlDom&, const string&) const;

    /// Adds pgm image of segmentation to the XML document.
    bool AddToXMLsegmentimg(XmlDom&, const string&, const string&) const;

    /// Workhorse for the one above.
    imagedata CreateSegmentImage(const string&, const string&) const;

    /// Workhorse for the one above.
    imagedata CreateSegmentImage(const string&, const string&, 
				 const vector<string>&, char) const;

    /// Workhorse for the one above.
    imagedata CreateSegmentImage(const string&, const string&,
				 const vector<int>&, char,
                                 const string&) const;

#ifdef PICSOM_USE_CONTEXTSTATE
    ///
    bool ConditionallyAddToContext(const upload_object_data& info) {
      return !use_context || context.Add(info);
    }

    ///
    bool AddToContext(int);

#endif // PICSOM_USE_CONTEXTSTATE

    ///
    bool AddToXMLcontextstate(XmlDom&, const string&, const string&) const;

    ///
    pair<string,string> SqlFeatureNames(const string&, bool);

    ///
    string SqlEscape(const string&, bool, bool);

    ///
    size_t SqliteMode() const { return sqlite_mode; }

    ///
    size_t MysqlMode() const { return mysql_mode; }

    ///
    size_t SqlMode() const {
      if (sqlite_mode && mysql_mode)
	SimpleAbort();

      return sqlite_mode ? sqlite_mode : mysql_mode;
    }

    ///
    bool UseSql() const { return SqlMode(); }

    ///
    const string& SqlStyle() const {
      static string mysql = "mysql", sqlite3 = "sqlite3", none = "none";
      return SqlMode() ? sqlite_mode ? sqlite3 : mysql : none;
    }

    ///
    bool SqlObjects() const { return UseSql() && sqlobjects; }

    ///
    bool SqlFeatures() const { return UseSql() && sqlfeatures; }

    ///
    bool SqlClasses() const { return UseSql() && sqlclasses; }

    ///
    bool SqlIndices() const { return UseSql() && sqlindices; }

    ///
    bool SqlSettings() const { return UseSql() && sqlsettings; }

    ///
    bool SqlAll() const { return UseSql() && sqlall; }

    ///
    sql_table_info SqlTableInfo(const string&) const;

    ///
    list<sql_table_info> SqlTableInfoList(const string&) const;

    ///
    bool SqlTableExists(const string&) const;

    ///
    bool SqlCreateTable(const string&, const string&) const;

    ///
    bool SqlReadAliases();

    ///
    bool SqlAddAlias(const string&, const string&);

    ///
    const map<string,string>& SqlAliases() const { return sql_alias; }

    ///
    bool SqlStoreFileFromFile(const string&, const string&) const;

    ///
    bool SqlStoreFile(const string&, const string&, const struct timespec&) const;

    ///
    bool SqlRetrieveFileToFile(const string&, const string&) const;

    ///
    bool SqlRetrieveFile(const string&, string&, bool = false) const;

    ///
    bool SqlDeleteFile(const string&) const;

    ///
    bool SqlCatFile(const string&) const;

    ///
    bool SqlFileExists(const string&) const;

    ///
    sql_file_info SqlFileInfo(const string&) const;

    ///
    list<sql_file_info> SqlFileInfoList(const string&) const;

    ///
    string SqlObjectData(size_t) const;

    ///
    bool SqlStoreSegmentationFile(size_t, const string&, const string&) const;

    ///
    bool SqlRetrieveSegmentationFile(size_t, const string&,
				     const string&) const;

    ///
    bool SqlFeatureIsBlob(const string&);

    ///
    size_t SqlFeatureDataCount(const string&, bool);

    ///
    FloatVectorSet SqlFeatureDataSet(const string&, const string&, bool);

    ///
    FloatVector *FeatureData(const string&, size_t, bool);

    ///
    FloatVector *FeatureDataCombined(const string&, size_t, bool);

    ///
    bool FeatureDataCombinedPrepare(const string&, bool);

    ///
    FloatVector *FeatureDataNumPy(const string&, size_t, bool);

    ///
    FloatVector *SqlFeatureData(const string&, size_t, bool) const;

    ///
    size_t SqlFeatureLength(const string&, bool);

    ///
    bool SqlInsertDetectionData(size_t, const string&, double);

    ///
    map<string,float> SqlRetrieveDetectionData(size_t, const string& = "");

    ///
    bool SqlExec(const string&, bool) const;

    ///
    bool SqlClose();

    ///
    static bool SqlDeleteDataBase(const string&, PicSOM*);

    /// A helper to create "x'00112233'".
    static string SqlBlob(const char *s, size_t bs);

    /// A helper to create "x'00112233'".
    static string SqlBlob(const string& s) {
      return SqlBlob(s.c_str(), s.size());
    }

    /// A helper to create "x'00112233...'".
    static string SqlTimeBlob(const struct timespec&);

    ///
    string SqliteDBFile(bool);

    ///
    string SqliteDBstr(const string&);

#ifdef HAVE_SQLITE3_H
    /// Creates the needed sqlite3 tables for a new database.
    bool CreateFromScratchSqlite(bool, bool, bool, bool, bool);

    ///
    bool SqliteOpen(const string&, bool, bool);

    ///
    bool SqliteClose();

    ///
    sqlite3_stmt *SqlitePrepare(const string&, bool = true) const;

    ///
    bool SqliteExec(const string&, bool = true) const;

    ///
    string SqliteErrMsg() const;

    ///
    void SqliteShowRow(sqlite3_stmt*) const;

    ///
    static int SqliteInt(sqlite3_stmt *s, int i) {
      // test something first ?
      return sqlite3_column_int(s, i);
    }

    ///
    static double SqliteDouble(sqlite3_stmt *s, int i) {
      // test something first ?
      return sqlite3_column_double(s, i);
    }

    ///
    static string SqliteString(sqlite3_stmt *s, int i) {
      // test something first ?
      const char *p = (const char*)sqlite3_column_blob(s, i);
      int n = sqlite3_column_bytes(s, i);
      return string(p, n);
    }

    ///
    FloatVector *SqliteFloatVector(sqlite3_stmt*) const;

    ///
    static bool HasSqlite() { return true; }

#else

    ///
    static bool HasSqlite() { return false; }

#endif // HAVE_SQLITE3_H

    ///
    static bool MysqlThreadInit();

    ///
    static bool MysqlThreadEnd();

#ifdef HAVE_MYSQL_H
    ///
    static MYSQL *MysqlInit();

    ///
    static pair<string,string> MysqlUserPassword(const string&);

    ///
    static bool MysqlDataBaseExists(const string&, const string&);

    ///
    static list<string> MysqlDataBaseList(PicSOM*);

    /// Creates the needed sqlite3 tables for a new database.
    bool CreateFromScratchMysql(bool, bool, bool, bool, bool);

    ///
    bool MysqlOpen(const string&, bool, bool);

    ///
    bool MysqlClose(bool = true);

    ///
    MYSQL_STMT *MysqlPrepare(const string&, bool = true) const;

    ///
    bool MysqlExecute(MYSQL_STMT*, bool = true) const;

    ///
    bool MysqlStmtClose(MYSQL_STMT*) const;

    ///
    static bool MysqlQuery(const string&, bool, MYSQL*,
			   const DataBase* = NULL);

    ///
    bool MysqlQuery(const string& q, bool w = true) const {
      return MysqlQuery(q, w, mysql, this);
    }

    ///
    static string MysqlErrMsg(MYSQL* = NULL);

    ///
    void MysqlShowRow(MYSQL_STMT*) const;

    ///
    void MysqlLock() const { //fake const...
      ((DataBase*)this)->mysql_lock.LockWrite();
    }

    ///
    void MysqlUnlock() const { //fake const...
      ((DataBase*)this)->mysql_lock.UnlockWrite();
    }

    ///
    // static int MysqlInt(sqlite3_stmt *s, int i) {
    //   // test something first ?
    //   return sqlite3_column_int(s, i);
    // }

    ///
    // static double MysqlDouble(sqlite3_stmt *s, int i) {
    //   // test something first ?
    //   return sqlite3_column_double(s, i);
    // }

    ///
    static string MysqlString(MYSQL_STMT *s, int i) {
      // test something first ?
      string ret;
      MYSQL_BIND bind[1];
      memset(bind, 0, sizeof(bind));
      if (!mysql_stmt_fetch_column(s, bind, i, 0)) {
      }
      return ret;
    }

    ///
    string MysqlFindTableColumn(const string&, const string&);

    ///
    string MysqlAddTableColumn(const string&, const string&, const string&);

    ///
    static bool HasMysql() { return true; }

#else

    ///
    static bool HasMysql() { return false; }

    ///
    static void MysqlLock() {}

    ///
    static void MysqlUnlock() {}

#endif // HAVE_MYSQL_H

    /// Used for different types of collages.
    imagedata CreateImageCollage(const list<string>&, const string&,
				 size_t, int, int, const imagedata&,
                                 float c=1.0);

    /// Used for different types of collages.
    static imagedata CreateImageCollage(const XmlDom&, DataBase*, Query*);

    /// Adds keywordlist.
    bool AddToXMLkeywordlist(XmlDom&, int, bool, bool, size_t,
			     const string&, const string&, const Query*) const;

    /// Inner loop of AddToXMLkeywordlist()
    bool AddToXMLkeywordlistInner(XmlDom&, const keyword_list&, string, 
				  float, const string&,
				  const set<string>*, bool) const;

    /// Adds temporarily extracted media clip
    bool AddToXMLmediaclip(xmlNodePtr, xmlNsPtr, const string&,
			   target_type);

    string Mpeg7MCXML(int pidx) const {
      string parent_label = Label(pidx);
      string videofile = SolveObjectPath(parent_label, "", "",
					 false, NULL, true);
      string objbase = videofile.substr(0,videofile.rfind('.'));
      return objbase+".d/mpeg7mc-"+parent_label+".xml";
    } 

    /// Adds featurealiases
    bool AddToXMLfeaturealiases(XmlDom&) const;

    /// Adds named command lines
    bool AddToXMLnamedcmdlines(xmlNodePtr, xmlNsPtr);

    /// Creates the default feature aliases if they do not exist already
    void CreateDefaultFeatureAliases(bool, bool);

    ///
    xmlNodePtr XMLSearchNode(xmlNodePtr node, const string& name,
			     const string& pname = "",
			     const string& pval = "");

    ///
    double GetMpeg7TimePoint(const string& str) {
      return GetMpeg7TimeValue(str, "T([0-9]+):([0-9]+):([0-9]+):"
			       "([0-9]+)F([0-9]+)");
    }

    ///
    double GetMpeg7Duration(const string& str) {
      return GetMpeg7TimeValue(str, "PT([0-9]+H)?([0-9]+M)?([0-9]+)S"
			       "([0-9]+)N([0-9]+)F");
    }

    ///
    double GetMpeg7TimeValue(const string& str, string regex_pattern);

    ///
    string RemoveZeros(const string& str) {
      char tmp[100];
      sprintf(tmp,"%d",atoi(str.c_str()));
      return tmp;
    }    

    ///
    string LabelSubDir(const string& label) const {
      string method, lname, lframe, segment;
      SplitLabel(label, method, lname, lframe, segment);
      
      return (lframe.size() ? lname+".d" : "");
    }

    /// A multipurpose wrapper around Extract*Clip().
    bool ExtractObject(size_t, bool = true);

    /// A multipurpose wrapper around Extract*Clip().
    bool ExtractObjects(const vector<size_t>&, bool);

    ///
    int PossiblyInsertFaceObjectAndExtractFeatures(int idx,
						   const segmentfile& segf,
						   int,
						   const string& name,
						   const list<string>& f);

    ///
    imagedata ExtractLfwFace(const imagedata&, const segmentfile&, int,
			     const string&);
    
    ///
    imagedata ExtractLfwFace(int);
    
    ///
    string RecognizeFace(const list<pair<string,FloatVector*>>&);

    ///
    int GetParentVideo(const string& label, const target_type tt);

    ///
    static string MediaExt(target_type tt) {
      return tt & target_sound ? ".wav" : tt & target_video ? ".mpg" : ".jpeg";
    }

    ///
    bool HasExtractedFiles() const { return extracted_files; }

    ///
    string ConvertGlobalToLocalDiskName(const string&);

    ///
    string CopyFileFromGlobalToLocalDisk(const string&);

    /// A multipurpose wrapper around IsMissing*Clip().
    bool IsMissingObject(size_t) const;

    ///
    bool IsImageFromContainer(size_t) const;

    ///
    bool IsMissingImageFromContainer(size_t) const;

    ///
    bool IsMissingObjectInSql(size_t) const;

    /// Returns true if AddToXMLmediaclip() needs to be called.
    bool IsMissingVideoClip(size_t idx) const {
      return IsMissingMediaClip(idx, target_video);
    }

    /// Returns true if AddToXMLmediaclip() needs to be called.
    bool IsMissingAudioClip(size_t idx) const {
      return IsMissingMediaClip(idx, target_sound);
    }

    /// Returns true if AddToXMLmediaclip() needs to be called.
    bool IsMissingVideoFrame(size_t idx) const {
      return IsMissingMediaClip(idx, target_image);
    }

    /// Returns true if AddToXMLmediaclip() needs to be called.
    bool IsMissingImageSegment(size_t idx) const {
      return IsMissingMediaClip(idx, target_imagesegment);
    }

    ///
    bool IsMissingMediaClip(size_t, target_type) const;

    /// Returns true if videoclip can be created.
    bool IsVideoClip(size_t idx) const {
      return IsMediaClip(idx, target_video, target_video);
    }

    /// Returns true if audioclip can be created.
    bool IsAudioClip(size_t idx) const {
      return IsMediaClip(idx, target_sound, target_video);
    }

    /// Returns true if image can be created.
    bool IsVideoFrame(size_t idx) const {
      return IsMediaClip(idx, target_image, target_video);
    }

    /// Returns true if image segment can be created.
    bool IsImageSegment_new(size_t idx) const {
      return IsMediaClip(idx, target_imagesegment, target_image);
    }

    ///
    bool IsMediaClip(size_t, target_type, target_type) const;

    ///
    bool IsImageSegment_old(size_t idx) const {
      return ObjectsTargetTypeContains(idx, target_image) &&
	ObjectsTargetTypeContains(idx, target_segment);
    }

    ///
    bool ExtractImageFromContainer(size_t);

    ///
    bool ExtractImagesFromContainer(const vector<size_t>&);

    ///
    bool ExtractFromContainerCommon(size_t, string&, string&, string&, string&);

    ///
    bool ExtractMediaClip(size_t idx, const target_type tt, bool);

    ///
    bool ExtractImageSegment(size_t idx, bool subd = true) {
      return ExtractMediaClip(idx, target_imagesegment, subd);
    }

    ///
    bool ExtractVideoClip(size_t idx, bool subd = true) {
      return ExtractMediaClip(idx, target_video, subd);
    }

    ///
    bool ExtractAudioClip(size_t idx, bool subd = true) {
      return ExtractMediaClip(idx, target_sound, subd);
    }

    ///
    bool ExtractVideoFrame(size_t idx, bool use_subdir) {
      return ExtractMediaClip(idx, target_image, use_subdir);
    }
        
    ///
    bool ExtractVideoFrames(const vector<size_t>&);

    /// This should be removed.  Was needed for trecvid2013med.
    void FpsCache(size_t i, float v) { fps_cache[i] = v; }
        
    /// This should be removed.  Was needed for trecvid2013med.
    float FpsCache(size_t i) { return fps_cache[i]; }
        
    /// Helper for the one above.
    bool SolveObjectSubrange(const string&, target_type tt, string&,
			     string&, string&);

    /// Helper for the one above.
    bool SolveObjectSubrangeOrigins(const string&, string&, string&, string&);

    /// Helper for the one above.
    bool SolveObjectSubrangeXML(const string&, int, string&, string&);

    /// Checks if new labels have been inserted. If there are new
    /// labels, load them and append the indices to the arrivedobjects
    /// vector
    bool LookForArrivedObjects();

    /// Count of views.
    size_t Nviews() const { return view.Nitems(); }

    /// Access to views.
    DataBase& View(size_t i) { return view[i]; }

    /// Append to list of views.
    void AppendView(DataBase *v) { view.Append(v); }

    /// Access to view's parent.
    DataBase *ParentView() const { return parent_view; }

    /// Tries to find existing cookie by user:password.
    const char *FindCookieP(const char*) const;

    /// Gives the total number of indexing methods.
    size_t Nindices() const { return index.size(); }

    /// Name of the indexed Index.
    const string& IndexName(size_t i) const {
      return GetIndex(i)->IndexName();
    }

    /// A helper.
    string MethodAndIndexName(size_t i) const {
      return "["+GetIndex(i)->MethodName()+"] <"+IndexName(i)+">";
    }

    /// Gives reference to any TSSOM map. Deprecated!
    TSSOM& TsSomDeprecated(size_t i) { 
      TSSOM *p = dynamic_cast<TSSOM*>(GetIndex(i));
      if (!p)
	ShowError("DataBase::TsSomDeprecated() failing with "+
                  MethodAndIndexName(i));

      return *p;
    }

    /// Gives reference to any TSSOM map. Deprecated!
    const TSSOM& TsSomDeprecated(size_t i) const {
      return ((DataBase*)this)->TsSomDeprecated(i);
    }

    /// "Canonical name" of a given feature map and its layer.
    string MapName(bool db_too, int m, int l = -1, int k = -1) const {
      return TsSomDeprecated(m).MapName(db_too, l, k);
    }

    /// Returns value of no_statistics.
    bool NoStatistics() const { return no_statistics; }

    /// Sets value of no_statistics.
    void NoStatistics(bool n) { no_statistics = n; }

    /// Extracts label part from a filename.
    static string LabelFromFileName(const string& f) {
      string ret = f;
      size_t slash = ret.find_last_of('/');
      if (slash!=string::npos)
	ret.erase(0, slash+1);

      size_t dot = ret.find_last_of('.');
      if (dot!=string::npos) {
	string lext = ret.substr(dot);
	if (lext==".bz2" || lext==".gz") {
	  ret.erase(dot);
	  dot = ret.find_last_of('.');
	}
	if (dot!=string::npos)
	  ret.erase(dot);
      }

      return ret;
    }

    /// Extracts the substring that starts at "start" and ends at "stop" 
    /// from string and places result in "result". Returns false if start 
    /// or stop is -1
    static bool GetSubstring(const string& str, string& result, 
			     int start, int stop);

    ///
    bool SetLabelRegExps();

    /// Splits object label of type segmethod:xxxxxxxx:ff_seg
    /// into method, imagename, frame and segment parts.
    /// Returns true if successfull, false otherwise
    bool SplitLabel(const string& label, 
			   string& segmethod, string& imagename, 
			   string& frame, string& segment,
			   bool multiframe) const;

    ///
    bool SplitLabel(const string& label, 
                    string& segmethod, string& imagename, 
                    string& frame, string& segment) const {
      return 
        SplitLabel(label, segmethod, imagename, frame, segment, false) ||
        SplitLabel(label, segmethod, imagename, frame, segment, true);
    }


    /// Returns the filename part of the object label
    string LabelFileNamePart(const string& label) const;

    /// Constructs label from method,body,frame,segment
    static string BuildLabel(const string& method, const string& body,
			     const string& frame, const string& segme) {
      string ret = method!="" ? method+":" : "";
      ret += body;
      ret += frame!="" ? ":"+frame : "";
      ret += segme!="" ? "_"+segme : "";
      return ret;
    }

    /// Returns true if "label" is a proper object label.
    bool IsLabel(const string& label) const;

    /// a non-static IsVirtualImage function that checks for target types also
    bool IsVirtualImage(const string& name) const {
      if (Is8plus8(name))
	return true;

      int lbindex = LabelIndexGentle(name);
      if (lbindex==-1)
	return true;

      target_type ttype = ObjectsTargetType(lbindex);
      return ttype==target_segment;
    }

    // Checks if label is of type 8d+8d
    static bool Is8plus8(const string& label) {
      if (label.size()!=17 || label[8]!='+')
	return false;
      
      const char *d = "0123456789";
      return label.find_first_not_of(d)==8 && label.find_last_not_of(d)==8;
    }

    /// Inserts a new image in the database.
    bool InsertObjects(list<upload_object_data>&, const list<string>&,
                       const list<string>&, const list<string>&, bool, bool,
		       const vector<string>&, XmlDom&, const string&);
    
    /// Inserts a new image in the database.
    bool InsertObjectsProcessing(list<upload_object_data>&, const list<string>&,
				 const list<string>&, const list<string>&,
				 bool, bool, const vector<string>&, XmlDom&,
				 const string&, int);
    
    /// Inserts a new segmentation image to the databse (called from
    /// InsertObjects). Returns false if an error occured.
    bool InsertSegmentationObject(const upload_object_data&);

    /// Inserts a new "regular" object to the database (not a
    /// segmentation object). Returns false if an error occured.
    /// Otherwise, argument is updated with the newly assigned indices.
    bool InsertRegularObject(upload_object_data&, XmlDom&);

    ///
    bool InsertContainerFile(const string&, bool);
    
    ///
    bool InsertContainerObjectsAndFile(upload_object_data&, XmlDom&);

    ///
    bool InsertContainerObjects(upload_object_data&, XmlDom&, string&);

    ///
    bool ContainsImages(const string&);

    ///
    bool TarToZip(const string&, const string&);
    
    ///
    bool InsertVideoSubObjects(size_t index, target_type,
			       upload_object_data& info,
			       int, float, bool, XmlDom&);

    ///
    bool KeywordsToClasses(int, const upload_object_data& info);

    ///
    bool PossiblyDisplay(const upload_object_data& info, target_type);

    /// Appends outputline to metaclassfile, if it is not already there.
    bool AppendToMetaClassFile(const string &metaclassfile, 
			       const string &outputline, bool reload=false);
    
    /// Reads the eml-file specified in arguments, extracts the
    /// attachments and inserts the message parts to the database.
    bool InsertMessageParts(upload_object_data&, string&, string&);

    /// Inserts one message part. The label of the parent message and
    /// the char string containing the part label, file type, file
    /// name etc must be given
    bool InsertMessagePart(upload_object_data&, string&, char*);

    /// Replaces the first occurance of space character with a '\0' in
    /// the given string. Returns a pointer to the position after the
    /// replaced character (returns NULL on error)
    char *GetNextSpaceSeparatedString(char*);

    /// Inserts a new image in the database.
    const char *InsertImage(int&, const char*, int, const char*,
			    const char* = NULL, const char* = NULL,
			    const char* = NULL, const char* = NULL,
			    const char* = NULL);

    /// Inserts a new object in the database, object file data.
    string InsertObjectFile(const string&, const string&, const string&,
			    const string&, const string&);

    /// A common helper function.
    string InsertObjectData(const string&, const string&, const string&,
			    const string&, const string&);

    /// Writes the given data to a file with the given name. 
    /// Returns true on success
    bool WriteDataToFile(const string&, const string&);

    /// Returns reformatted label and increments it.
    string ImportedLabel(const string&, string&);

    /// Returns next free label and increments it.
    string NextFreeLabel();

    /// Sets next free label or returns false if failure.
    bool NextFreeLabel(int i) {
      stringstream ss;
      ss << setw(LabelLength()) << setfill('0') << i;
      return NextFreeLabel(ss.str());
    }

    /// Sets next free label or returns false if failure.
    bool NextFreeLabel(long int li) {
      stringstream ss;
      ss << setw(LabelLength()) << setfill('0') << li;
      return NextFreeLabel(ss.str());
    }

    /// Sets next free label or returns false if failure.
    bool NextFreeLabel(const string&);

    /// Opens labels file for appending if not already open.
    bool OpenLabelsFile(bool);

    /// Closes labels file.
    bool CloseLabelsFile(bool);

    /// Appends given label to the labels file.  Returns index or -1 if fails.
    int AddOneLabel(const string&, target_type, bool, bool);

    /// Appends given label.  Missing parents are appended first.
    /// Returns true on success.
    bool AddLabelAndParents(const string&, target_type, bool);

    /// Adds missing origins info for all file objects.
    bool FillOrigins(const ground_truth*);

    /// Fix origins info for given index if it is unknown
    bool FixUnknownOrigins(size_t i);

    /// Update specific label in origins file, removing old line if it exists
    bool UpdateOriginsInfo(size_t, const string&, const string&);

    /// Update specific label in origins file, removing old line if it exists
    bool UpdateOriginsInfoSql(size_t, const map<string,string>&, bool);

    /// Update specific label in origins file, removing old line if it exists
    bool UpdateOriginsInfoSqlUpdate(size_t, const map<string,string>&);

    /// Update specific label in origins file, removing old line if it exists
    bool UpdateOriginsInfoSqlAddNew(size_t, const map<string,string>&);

    /// Slave version.
    bool SlaveSendOriginsInfo(const map<string,string>&, XmlDom&);

    /// Inserts a new image in the database, image file data.
    /// If it finds that target type is file+image and the object is multiframe
    /// image file, it changes target_image to target_imageset in last arg.
    bool InsertOriginsInfo(size_t, bool, 
			   const string&, const string&, const string&,
			   const string&, const string&, const string&,
			   const map<string,string>&,
			   const string&, target_type&, int&, float&,
			   XmlDom&);

    /// Extract image statistics from the image file.
    bool SolveMissingOriginsInfo(const string&, target_type,
				 string&, string&, string&,
				 int&, float&, string&, bool = true);

    /// Inserts a new image in the database, image file data.
    bool InsertImageText(const string&, const string&);

    /// Inserts a new image's all thumbnails in the database. Boolean
    /// argument determines to_root value given to SolveObjectPath()
    bool InsertImageThumbnails(const string&, bool = true);

    /// Inserts a new image's thumbnail of given size in the
    /// database. Boolean argument determines to_root value given to
    /// SolveObjectPath()
    bool InsertImageThumbnail(const string&, int, int, bool = true);

    /// Returns true if features should be calculated.
    bool OughtToCalculateFeatures() const { return allowownimage; }

    /// Returns true if features really can be calculated.
    bool CanCalculateFeatures() const {
      if (!Nindices())  // as of 2012-02-08 I don't understand this logic...
	return true;

      for (size_t i=0; i<Nindices(); i++)
	if (GetIndex(i)->CanCalculateFeatures())
	  return true;

      return false;
    }

    /// Calculates feature vectors for the new image.
    bool CalculateFeatures(int, const list<string>&);

    ///
    bool CalculateConceptFeatures(const Index*, const vector<size_t>&, 
				  const vector<string>&, 
				  const list<incore_feature_t>&,
				  feature_result*);

    /// Loads features from files and updates div and backref data.
    bool LoadAndMatchFeatures(int, const list<string>&, bool, bool);

    /// Loads features from files and updates div and backref data.
    bool LoadAndMatchFeatures(const string&);

    /// Rewrites any div files whose corresponding division member are changed.
    bool ReWriteChangedDivisionFiles(bool cwd, bool zipped);

    /// Returns true if detections have been defined.
    bool CanExtractMedia() const {
      return described_medias.size();
    }

    /// Returns true if detections have been defined.
    bool CanDoSegmentation() const {
      return described_segmentations.size();
    }

    /// Returns true if detections have been defined.
    bool CanDoDetections() const {
      return described_detections.size();
    }

    ///
    map<string,string> DescribedDetection(const string& d) const {
      map<string,string> r;
      if (described_detections.find(d)!=described_detections.end())
	r.insert(described_detections.at(d).begin(),
		 described_detections.at(d).end());
      return r;
    }

    /// Returns true if text searches have been defined.
    bool HasTextSearches() const {
      return described_textsearches.size();
    }

    /// Returns true if named text search has been defined.
    bool HasTextSearch(const string& n) const {
      return described_textsearches.find(n)!=described_textsearches.end();
    }

    const textsearchspec_t& TextSearch(const string& n) const {
      static textsearchspec_t empty;
      auto i = described_textsearches.find(n);
      return i!=described_textsearches.end() ? i->second : empty;
    }

    /// Returns true if text indices have been defined.
    bool HasTextIndices() const {
      return described_textindices.size();
    }

    /// Returns true if named text index has been defined.
    bool HasTextIndex(const string& n) const {
      return described_textindices.find(n)!=described_textindices.end();
    }

    /// Returns true if named text index has been defined.
    const string& DefaultTextIndex() const {
      static const string empty;
      return described_textindices.size() ?
	described_textindices.begin()->first : empty;	
    }

    /// Returns list of defined text indices.
    list<string> TextIndices() const {
      list<string> l;
      for (auto i=described_textindices.begin();
	   i!=described_textindices.end(); i++)
	l.push_back(i->first);
      return l;
    }

    /// Returns the description from settings.xml as a list.
    const list<pair<string,string> >&
    TextIndexDescription(const string& n) const {
      static list<pair<string,string> > empty;
      auto p = described_textindices.find(n);
      return p==described_textindices.end() ? empty : p->second;
    }

    ///
    const string& TextIndexDescriptionValue(const string&, const string&);

    ///
    list<string> TextIndexFields(const string&);

    ///
    bool TextIndexInput(const list<string>&);

    ///
    bool TextIndexUpdate(size_t, const string&,
			 const list<pair<string,string> >&);

    ///
    bool TextIndexUpdate(const string&, const string&,
			 const list<pair<string,string> >&);

    ///
    list<pair<string,string> > TextIndexRetrieve(size_t, const string&);

    ///
    list<pair<string,string> > TextIndexRetrieve(const string&, const string&);

    ///
    list<pair<string,string> > TextIndexRetrieveFake(size_t, const string&);

    ///
    list<textline_t> TextIndexAllLines( const string&, size_t);
    
    ///
    textline_t TextIndexLine(const string&, const string&, size_t);

    ///
    textline_t TextIndexLineOld(const string&, const string&, size_t);

    ///
    list<textline_t> TextIndexLines(const string&, const string&, 
				    const vector<size_t>&);

    ///
    list<textline_t> TextIndexLines(const string&, const string&, 
				    const ground_truth&);

    ///
    string TextIndexSearchByLabel(size_t, const string&, const string&);

    ///
    vector<textsearchresult_t> TextIndexSearch(const string&,
					       const string& = "",
					       const string& = "");

    ///
    list<string> TextIndexApplyRules(const string&,
				     const list<pair<string,string> >&);

    ///
    list<string> TextIndexRules(const string&);

    ///
    string TextIndex_aspell_common(const string&, const vector<string>&, bool);

    ///
    string TextIndex_aspell(const string&, const vector<string>&);

    ///
    string TextIndex_aspell_keep(const string&, const vector<string>&);

    ///
    vector<float> TfIdf(const string&, size_t);

    ///
    map<string,size_t> NgramCounts(const string&, size_t);

    ///
    size_t NgramIndex(const string&, size_t);

    ///
    bool CreateIdfs(const string&,
		    const vector<size_t>&, const vector<size_t>&);

    ///
    string ShowTfIdf(const vector<float>&, size_t);

    /// Executes all <extraction><detection> instructions.
    bool DoAllMediaExtractions(bool, const vector<size_t>&,
			       const vector<string>&,
			       const string&, Segmentation*);

    /// Executes all <extraction><segmentation> instructions.
    bool DoAllSegmentations(bool, const vector<size_t>&, const vector<string>&);

    /// Executes all <extraction><segmentation> instructions.
    bool DoAllSegmentations(bool, const list<upload_object_data>& objs,
			    const list<string>&);

    /// Executes one <extraction><segmentation> instruction.
    bool DoOneSegmentationForAll(bool, const vector<size_t>&, const string&);

    /// Executes one <extraction><segmentation> instruction.
    bool DoOneSegmentationForAll(bool, const vector<size_t>&, const
				 pair<string,list<pair<string,string> > >&);

    /// Executes one <extraction><segmentation> instruction.
    bool DoOneSegmentation(bool, size_t,
			   const pair<string,list<pair<string,string> > >&);

    ///
    string DetectionName(const list<pair<string,string> >&,
			 const list<string>&, const string&, const string&,
			 const string&, bool) const;

    ///
    string DetectionName(const string&, const list<string>&, const string&,
			 const string&, const string&, bool) const;
    
    ///
    bool OpenBinDetection(const string&, const string&, size_t);

    ///
    list<string> FindAllBinDetectionNames(bool);

    ///
    list<string> FindMatchingBinDetections(const string&, bool = false);

    /// Executes all <extraction><detection> instructions.
    bool DoAllDetections(bool, const vector<size_t>&, const vector<string>&,
			 const string&, const string&, const list<string>&,
			 const string&, bool,
			 Segmentation*, XmlDom&, PicSOM::detection_stat_t&);

    /// Executes all <extraction><detection> instructions.
    bool DoAllDetections(bool, const list<upload_object_data>& objs,
			 const list<string>&, const list<string>&,
			 bool, Segmentation*);

    /// Executes one <extraction><detection> instruction.
    bool DoOneDetectionForAll(bool, const vector<size_t>&, const string&,
			      const string&, const string&, const list<string>&,
			      const string&, bool, Segmentation*, XmlDom&,
			      PicSOM::detection_stat_t&);

    /// Executes one <extraction><detection> instruction.
    bool DoOneDetectionForAll(bool, const vector<size_t>&,
			      const pair<string,list<pair<string,string> > >&,
			      const string&, const string&, const list<string>&,
			      const string&, bool, Segmentation*, XmlDom&,
			      PicSOM::detection_stat_t&);

    /// Executes one <extraction><detection> instruction.
    bool DoOneDetection(bool, size_t,
			const pair<string,list<pair<string,string> > >&,
			const string&, const string&, const list<string>&,
			const string&, Segmentation*, bool, bool&, XmlDom&);

    /// Calls RunCaffe().
    bool DoOneCaffeDetection(size_t, const list<pair<string,string> >&);

    /// Calls ObjectDetection class.
    bool DoOneObjectDetection(size_t, const list<pair<string,string> >&);

    /// Executes one <extraction><detection> instruction if svmpred.
    bool SvmPredDetection(size_t, const list<pair<string,string> >&,
			  const string&, const list<string>&, Segmentation*);

    /// Executes one <extraction><detection> instruction if svmpred.
    bool SvmPredDetectionSingle(size_t, const map<string,string>&,
				const list<pair<string,string> >&,
				DataBase*, const list<string>&, const string&,
				Segmentation*, double*, string*, bool, bool&);

     /// Executes one <extraction><detection> instruction if svmpred.
    bool SvmPredDetectionSingleBatch(size_t, const map<string,string>&, 
				     const list<pair<string,string> >&,
				     DataBase*, const list<string>&, const string&,
				     Segmentation*, double*, string*, bool, bool&);

    /// Executes one <extraction><detection> instruction if svmpred.
    bool SvmPredDetectionSingleSelf(size_t, const map<string,string>&,
				    const list<pair<string,string> >&,
				    DataBase*, const list<string>&, const string&,
				    Segmentation*, double*, string*, bool, bool&);

    /// A helper for the above method.
    bool SvmCommon(const list<pair<string,string> >&);

    /// Executes one <extraction><detection> instruction if lsc.
    bool LscDetectionSingle(size_t, const map<string,string>&,
			    const list<pair<string,string> >&,
			    DataBase*, const list<string>&, const string&,
			    Segmentation*, double*, string*);

    /// Executes one <extraction><detection> instruction if random.
    bool RandomDetectionSingle(size_t, const map<string,string>&,
			       const list<pair<string,string> >&,
			       DataBase*, const list<string>&, const string&,
			       Segmentation*, double*, string*);

    /// Executes one <extraction><detection> instruction if caffe.
    bool CaffeDetectionSingle(size_t, const map<string,string>&,
			      const list<pair<string,string> >&,
			      DataBase*, const list<string>&, const string&,
			      Segmentation*, double*, string*,
			      XmlDom&);

    ///
    static bool IsKnownFusion(const string&);

    ///
    float FusionResult(const vector<double>&, const string&,
		       const vector<string>&, const vector<double>&,
		       size_t);

    ///
    float RankBasedValue(const string&, const string&, float, size_t&);

    ///
    pair<size_t,size_t> Rank(const string&, float);

    ///
    bool FusionDetectionSingle(size_t, const map<string,string>&,
			       const list<pair<string,string> >&,
			       DataBase*, const list<string>&, const string&,
			       Segmentation*);

    ///
    bool ChildrenDetectionSingle(size_t, const map<string,string>&,
				 const list<pair<string,string> >&,
				 DataBase*, const list<string>&, const string&,
				 Segmentation*);

    ///
    bool SuperClassDetectionSingle(size_t, const map<string,string>&,
				   const list<pair<string,string> >&,
				   DataBase*, const list<string>&, const string&,
				   Segmentation*);

    ///
    bool TimeFusionDetectionSingle(size_t, const map<string,string>&,
				   const list<pair<string,string> >&,
				   DataBase*, const list<string>&, const string&,
				   Segmentation*);

    ///
    bool TimeThresholdDetectionSingle(size_t, const map<string,string>&,
				      const list<pair<string,string> >&,
				      DataBase*, const list<string>&, const string&,
				      Segmentation*);

    ///
    bool SentenceSelectionDetectionSingle(size_t, const map<string,string>&,
					  const list<pair<string,string> >&,
					  DataBase*, const list<string>&, const string&,
					  Segmentation*);

    ///
    bool CombineDetectionSingle(size_t, const map<string,string>&,
				const list<pair<string,string> >&,
				DataBase*, const list<string>&, const string&,
				Segmentation*);

    ///
    bool StoreDetectionResult(size_t, const string&, float, bool);

    ///
    bool StoreDetectionResult(size_t, const string&, const vector<float>&,
			      XmlDom&, bool);

    ///
    bool BinInsertDetectionDataOld(size_t, const string&, float);

    ///
    bool BinInsertDetectionData(size_t, const string&, const vector<float>&,
				bool);

    ///
    map<string,vector<float> > BinRetrieveDetectionData(size_t, const string&,
							bool, bool&);

    ///
    map<string,vector<float> > RetrieveDetectionData(size_t, const string&,
						     bool, bool&);

    ///
    map<string,vector<float> > RetrieveOrProduceDetectionData(size_t,
							      const string&,
							      const string&,
							      bool, bool&,
							      bool);

    ///
    map<string,vector<float> > ProduceDetectionData(size_t, const string&,
						    const string&, bool);

    ///
    bool CommonDetection(const string&, size_t, bool,
			 const list<pair<string,string> >&,
			 const string&, const list<string>&, const string&,
			 Segmentation*, bool, bool&, XmlDom&);

    ///
    bool CommonDetectionSingle(const string&, size_t,
			       const list<pair<string,string> >&,
			       const list<string>&, const string&,
			       Segmentation*, double*, string*, bool, bool&,
			       XmlDom&);
    ///
    float ThresholdValue(const string&);
    
    ///
    bool DoAllCaptionings(bool, const vector<size_t>&, const vector<string>&,
			  bool, XmlDom&);

    ///
    bool DoOneCaptioning(bool, const vector<size_t>&, const string&,
			 bool, XmlDom&);

    ///
    bool StoreCaptioningResult(size_t, const string&, const textline_t&,
			       bool, bool, XmlDom&);

    /// OSRS speech recognition.
    bool OSRSspeechRecognition(size_t, const list<pair<string,string> >&);

    ///
    bool TesseractOCR(size_t, const list<pair<string,string> >&);

    ///
    string ExtractFinnishNames(const string&);

    ///
    bool LaunchAsyncAnalysisDeprecated(const script_list_t &);

    /// Creates an Analysis object for detections etc.
    bool RunAnalysis(const list<pair<string,string> >&,
		     const vector<string>&);

    /// Creates an Analysis object for detections etc.
    bool RunAnalysis(const list<string>&, const vector<string>&);

    /// Creates an Analysis object for detections etc.
    bool RunAnalysisSelf(const list<string>&, const vector<string>&);

    /// Creates an Analysis object for detections etc.
    bool RunAnalysisBatch(const list<string>&, const vector<string>&);

    ///
    string SVMfeaturename(const string&);

    ///
    string SVMoptions(const string&);

    ///
    string SVMextra(const string&);

    ///
    string SVMclass(const string&);

    /// Checks if an SVM exists.
    bool HasSVM(const string&, const string&, const string&,
		const string&, const string&, const string&) const;

    /// Creates or reuses an SVM.
    SVM *GetSVM(const string&, const string&);

    /// Creates or reuses an SVM.
    SVM *GetSVM(const string&, const string&, const string&, const string&,
		const string&, const string&, const string&);

    /// Returns true if access rules are being debugged.
    bool DebugAccess() const { return debug_access; }

    ///
    void ConditionallyLogLock(const char *a, const char *b) {
      if (debug_locks)
	WriteLog(a, b?" ":NULL, b);
    }

    /// Locks for reading/writing.
    void RwLockEither(int l, const char *txt = NULL) {
      if (!l)
	RwLockRead(txt);
      else
	RwLockWrite(txt);
    }

    /// Locks for reading.
    void RwLockRead(const char *txt = NULL) {
#ifdef PICSOM_USE_PTHREADS
      ConditionallyLogLock("RwLockRead()   entering", txt);
      rwlock.LockRead();
      ConditionallyLogLock("RwLockRead()   exiting ", txt);
#else
      txt++;
#endif // PICSOM_USE_PTHREADS
    }

    /// Locks for writing.
    void RwLockWrite(const char *txt = NULL) {
#ifdef PICSOM_USE_PTHREADS
      ConditionallyLogLock("RwLockWrite()  entering", txt);
      rwlock.LockWrite();
      ConditionallyLogLock("RwLockWrite()  exiting ", txt);
#else
      txt++;
#endif // PICSOM_USE_PTHREADS
    }

    /// Unlocks after reading/writing.
    void RwUnlockEither(int l, const char *txt = NULL) {
      if (!l)
	RwUnlockRead(txt);
      else
	RwUnlockWrite(txt);
    }

    /// Unlocks.
    void RwUnlockRead(const char *txt = NULL) {
#ifdef PICSOM_USE_PTHREADS
      ConditionallyLogLock("RwLockUnlockRead()     ", txt);
      rwlock.UnlockRead();
#else
      txt++;
#endif // PICSOM_USE_PTHREADS
    }

    /// Unlocks.
    void RwUnlockWrite(const char *txt = NULL) {
#ifdef PICSOM_USE_PTHREADS
      ConditionallyLogLock("RwLockUnlockWrite()    ", txt);
      rwlock.UnlockWrite();
#else
      txt++;
#endif // PICSOM_USE_PTHREADS
    }

    /// Pointer to "" or "W " or "R ".
    const char *RwLockStatusChar() const {
#ifdef PICSOM_USE_PTHREADS
      return rwlock.StatusChar();
#else
      return "";
#endif // PICSOM_USE_PTHREADS
    }

    /// True if locked for long-taking writing.
    bool RwLockBounce() const {
#ifdef PICSOM_USE_PTHREADS
      return rwlock.Bounce();
#else
      return false;
#endif // PICSOM_USE_PTHREADS
    }

    ///
    bool ThisThreadLocks() const { return rwlock.ThisThreadLocks(); }

    /// Access to classificationset.
    // const string& ClassificationSet() const { return classificationset; }

    /// Empties all stored class models.
    void DeleteClassModels();

    /// Deletes a class model. May be unsafe. At least in threads...
    bool DeleteClassModel(const string&);

    /// Creates a set of class models.
    bool PrepareClassification(const string&, bool, const vector<string>&,
                               bool, bool);

    /// Creates a set of class models.
    bool PrepareClassification(const list<string>&, bool, const vector<string>&,
                               bool, bool);

    /// Adds new class model by given name, optional bool argument turns 
    /// the class negative.
    bool AddClassModel(const string&, const vector<string>&,
                       bool = false, bool = false, bool = false);

    /// Adds new class model by given name, optional bool argument turns 
    /// the class negative.
    bool AddClassModel(const string& c, const string& f,
                       bool b = false, bool v = false) {
      vector<string> fv;
      fv.push_back(f);
      return AddClassModel(c, fv, b, v);
    }

    /// Returns the number of stored class models.
    size_t NclassModelsAll() const { return class_models_new.size(); }

    /// Access to stored class models by index.
    Query *ClassModel(size_t i) const { return class_models_new[i]; }

    /// Access to stored class models by class name.
    Query *ClassModel(const string& n) const;

    /// Picks feature names from defaultquery or returns "*".
    string DefaultIndices() const;

    /// Returns true if it is sensible to prune given label for info.
    bool IsPrunable(const string&) const;

    /// Reads object's info from the origins file.
    map<string,string> ReadOriginsInfo(size_t idx, bool p, bool w) const;

    /// Solves origins file's name and reads object's info from there.
    map<string,string> ReadOriginsInfoSql(size_t) const;

    /// Solves origins file's name and reads object's info from there.
    map<string,string> ReadOriginsInfoClassical(const string&,
						bool, bool) const;

    /// Reads object's info from the origins file.
    map<string,string> ReadOriginsInfoInner(const string&, const string&,bool);

    ///
    const list<string>& ReadOriginsInfoInnerCache(const string&);

    ///
    void ClearOriginsFileLineCache() {
      origins_file_line_cache_map.clear();
    }

    /// Reads dimensions field in the origins file.
    string SolveImageDimensions(const string& l, bool prune) const {
      size_t idx = LabelIndex(l);
      map<string,string> info = ReadOriginsInfo(idx, prune, true);
      map<string,string>::const_iterator i = info.find("dimensions");
      return i!=info.end() ? i->second : "";
    }

    /// Gives the length of a video in seconds.
    float SolveVideoLength(size_t) const;

    /// If !=1, specifies that only every n'th frame of a video is used.
    size_t FrameStep() const { return framestep; }

    /// If !=1, specifies that only every n'th frame of a video is used.
    void FrameStep(size_t s) { framestep = s; }

    /// If !=0, specifies that margins should be applied in video features.
    size_t Margin() const { return margin; }

    /// If !=0, specifies that margins should be applied in video features.
    void Margin(size_t s) { margin = s; }

    /// True if errtype B allowed in TSSOM::BackReferenceSanityCheck().
    bool EmptyLabelsAllowed() const { return empty_labels_allowed; }
    
    /// True if we don't want to see same vectors from top and leaf dirs.
    bool IgnoreLeafFeatures() const { return ignore_leaf_features; }
    
    /// Sets the default aspects for the given target_type
    void DefaultAspects(target_type t, const aspects& a) {
      defaultaspects[t] = a;
    }
    
    /// Returns the default aspects for a given target_type
    aspects DefaultAspects(target_type t) const;

    /// Caches result of the above.
    void CacheDefaultAspects(target_type t, const aspects& a) {
      defaultaspects_cache[t] = a;
    }

    /// Sets the default default aspects
    void InitializeDefaultAspects();

    /// Type of classfiles.
    typedef map<string,ModifiableFile> classfiles_t;

    ///
    classfiles_t::iterator AddClassFileEntry(const string& f) {
      pair<classfiles_t::iterator,bool> j
	= classfiles.insert(classfiles_t::value_type(f, ModifiableFile()));
      if (debug_gt)
	cout << "Added classfile entry [" << f << "]" << endl;

      return j.first;
    }
    
    const SimpleList& GetSimpleList(simplelist_type type,
                                    const string& name="") {
      string sl_name = SimpleListFilename(type, name);
      ReadSimpleList(type,sl_name);
      return simplelist_map[sl_name];
    }
      
    const set<string>& GetProperPersons(const string& name="") {
      return GetSimpleList(simplelist_proper_persons,name).getSet(); 
    }

    const set<string>& GetProperLocations(const string& name="") { 
      return GetSimpleList(simplelist_proper_locations,name).getSet(); 
    }

    const multimap<string,string>& GetCommons(const string& name="") {
      return GetSimpleList(simplelist_commons,name).getMap(); 
    }
    
    const multimap<string,string>& GetClassFeatures(const string& name="") {
      return GetSimpleList(simplelist_classfeatures,name).getMap(); 
    }
    
    const list<string> CreatedFiles() { return created_files; }

    void AddCreatedFile(const string& fn) { created_files.push_back(fn); }

    bool RemoveCreated(const string& = "");

/*
    bool SetCommonsFile(const string& val) {
      commonsfile = val;
      return ReadCommons();
    }

    bool SetClassFeaturesFile(const string& val) {
      classfeaturesfile = val;
      return ReadClassFeatures();
    }
 */

    /// Access to CbirAlgorithms available for the database.
    const vector<CbirAlgorithm*>& Algorithms() const { return algorithms; }

    /// Access to CbirAlgorithms available for the database.
    vector<CbirAlgorithm*> Algorithms(const string&) const;

    /// Checks first algorithms, then possibly calls CreateAlgorithm().
    CbirAlgorithm *FindOrCreateAlgorithm(const string&);

    /// Calls CbirAlgorithm::Create().
    CbirAlgorithm *CreateAlgorithm(const string&); 

    /// For input 'foo' outputs either 'foo' or '"foo"'.
    static string SqlTableName(const string&, const string&);

    /// For input 'foo' outputs either 'foo' or '"foo"'.
    string SqlTableName(const string& t) const {
      return SqlTableName(t, SqlStyle());
    }

    /// 
    static string SqlBeginTransactionCmd(const string&);

    /// 
    bool SqlBeginTransaction();

    /// 
    static string SqlEndTransactionCmd(const string&);

    /// 
    bool SqlEndTransaction();

    /// Lots of output possibly.
    bool SqlDump(const string&, bool, const string&,
		 const string&, const Query*);

    /// Lots of output possibly.
    bool SqlDump(ostream&, const string&, const Query*, const string&,
		 const string&);

    /// Lots of output possibly.
    bool SqlDumpVersion(ostream&, const string&);

    /// Lots of output possibly.
    bool SqlDumpObjects(ostream&, const string&);

    /// Lots of output possibly.
    bool SqlDumpFeatures(ostream&, const string&, const Query*, bool);

    /// Lots of output possibly.
    bool SqlDumpClasses(ostream&, const string&);

    /// Lots of output possibly.
    bool SqlDumpFiles(ostream&, const string&);

    ///
    bool SqlUpdateFileList();

    ///
    bool HasExtraFeaturesDir() const { return !extra_features_dir.empty(); }

    ///
    const string& ExtraFeaturesDir() const { return extra_features_dir; }

    ///
    static const string& LscomName(const PicSOM*, const string&, bool,
				   const string& = "");

    ///
    const string& LscomName(const string& c, bool dir, const string& map = "") {
      return LscomName(Picsom(), c, dir, map);
    }

    ///
    static pair<string,string> SplitLscomName(const string&);

    ///
    string LscomNameToTrecvidId(const string&, bool = true);

    ///
    string PossiblyDeLscom(const string& str) {
      if (str.find("lscom")!=0)
	return str;
      
      string l_str = LscomName(str, true, "lscom/trecvid2011");
      pair<string, string> l_pair = SplitLscomName(l_str);
      return l_pair.first;
    }

    ///
    static const string& ImageNetName(const PicSOM*, const string&, bool);

    ///
    const string& ImageNetName(const string& c, bool dir ) {
      return ImageNetName(Picsom(), c, dir);
    }

    ///
    static vector<string> SplitImageNetName(const string&);

    ///
    const ontology& LscomOntology() const { return lscom_ontology; }

    ///
    const ontology& Ontology(const string& n = "") const { 
      return ((DataBase*)this)->ontology_map[n]; 
    }

    ///
    ontology& Ontology(const string& n = "") { return ontology_map[n]; }

    ///
    bool CreateWordnetOntology(const vector<pair<string,float> >&,
			       const string& = "");
    
    ///
    map<size_t,textline_t> 
    GenerateSentencesNeuralTalk(const vector<size_t>&,
				const map<string,string>&);

#ifdef PICSOM_USE_PYTHON
    /// helper
    bool NeuralTalkAddCandidate(textline_t&, PyObject*);
#endif // PICSOM_USE_PYTHON

    ///
    map<size_t,textline_t> ReadSentencesNeuralTalk(const string&);

    ///
    map<size_t,textline_t> ReadSentencesNeuralTalk(const list<string>&);

    ///
    map<size_t,textline_t> ReadSentencesCOCOsubmission(const list<string>&);

    ///
    bool StoreSentences(const string&, const string&, const vector<size_t>&,
			const map<size_t,textline_t>&, const string&);

  private:
    typedef list<pair<string,string> > lscom_list_t;
    typedef map<string,lscom_list_t> lscom_map_t;
    static lscom_map_t lscom_map;
    static map<string,string> imagenet_map;
    static bool PopulateLscomMap(const PicSOM*, const string&);
    static bool PopulateImageNetMap(const PicSOM*);

    static ontology lscom_ontology;

    ///
    map<string,ontology> ontology_map;
    
  public:
    class trecvid_med_event {
    public:
      ///
      void show(ostream& = cout) const;

      ///
      string event_id;

      ///
      string event_name;

      ///
      string definition;

      ///
      vector<string> explication;

      ///
      vector<string> scene;

      ///
      vector<string> objects;

      ///
      vector<string> activities;

      ///
      vector<string> audio;
    }; // class trecvid_med_event

    ///
    const string& TrecvidMedEventName(const string& e) {
      return TrecvidMedEvent(e).event_name;
    }

    ///
    const trecvid_med_event& TrecvidMedEvent(const string&);

    ///
    vector<pair<size_t,double> > TrecvidSIN(const vector<size_t>&,
					    const vector<pair<string,double> >&,
					    size_t, bool, bool);

    ///
    vector<pair<size_t,double> > TrecvidSINtopDown(const vector<size_t>&,
						   const vector<pair<bin_data*,double> >&,
						   size_t, bool);

    ///
    vector<pair<size_t,double> > TrecvidSINbottomUp(const vector<size_t>&,
						    const vector<pair<bin_data*,double> >&,
						    size_t, bool, bool);
    
#ifdef PICSOM_USE_OD
    ///
    void OdSetDebug(int v) { odone.SetDebug(v); }

    ///
    bool OdLoadDbs(string& d, string& i) { return odone.LoadDbs(d, i); }

    ///
    bool OdProcessImage(string& f, savetype& s, dbitem& b) {
      return odone.ProcessImage(f, s, b);
    }

#endif // PICSOM_USE_OD

    ///
    static void UsePthreadsFeatures(bool v) { use_pthreads_features = v; }

    ///
    static bool UsePthreadsFeatures() { return use_pthreads_features; }
    
    ///
    static void UsePthreadsDetection(bool v) { use_pthreads_detection = v; }

    ///
    static bool UsePthreadsDetection() { return use_pthreads_detection; }
    
    ///
    void JoinAsyncAnalysesDeprecated();

    ///
    int FrameNumber(size_t i) const { return FrameNumber(Label(i)); }

    ///
    static int FrameNumber(const string&);

    ///
    pair<size_t,size_t> FrameNumberRange(size_t, const
					 ground_truth* = NULL) const;

    ///
    video_frange FrameNumberFrange(size_t idx, const
				   ground_truth *gt = NULL) const {
      pair<size_t,size_t> p = FrameNumberRange(idx, gt);
      return video_frange(this, idx, p);
    }

    ///
    video_frange FrameNumberFrange(size_t idx, const string& s) const {
      size_t b = 0, e = 0;
      if (s!="" && s!="empty") {
	vector<string> be = SplitInSomething("-", false, s);
	if (be.size()!=2)
	  ShowError("FrameNumberFrange("+ToStr(idx)+","+s+") : split failed");
	b = atoi(be[0].c_str());
	e = atoi(be[1].c_str())+1;
      }

      return video_frange(this, idx, make_pair(b, e));
    }

    ///
    list<video_frange> SolveVideoFranges(const ground_truth&) const;

    ///
    video_frange_match DTWmatch(const string&, const string&, const string&,
				const dtw_spec&);

    ///
    video_frange_match DTWmatch(const string&, const string&, const string&,
				const set<size_t>&, const map<size_t,float>&,
				bool, double);

    ///
    double DTWdistance(const video_frange&, const video_frange&,
		       const dtw_spec&, dtw_spec::cache_t*);

    ///
    double DTWdistance(const video_frange&, const video_frange&,
		       const dtw_spec::flist_t&,
		       const pair<vector<float>,vector<float> >&,
		       dtw_spec::cache_t*);

    ///
    double DTWdistance_euc(const FloatVector&, const FloatVector&,
			   const map<size_t,float>&) const;

    ///
    double DTWdistance_chisqr(const FloatVector&, const FloatVector&,
			   const map<size_t,float>&) const;

    ///
    double DTWdistance_hamming(const FloatVector& fc,
			       const FloatVector& fr,
			       const float threshold) const;

    ///
    double DTWdistance_emd(const FloatVector&, const FloatVector&,
			   const map<size_t,float>&, const string&) const;

    ///
    double DTWdistance(const video_frange&, const video_frange&,
		       const set<size_t>&, const map<size_t,float>&,
		       bool, double);

    ///
    FloatVector InterpolatedVector(const FloatVectorSet&, int, int,
				   const string&);
    ///
    const map<string,string>& Macro() const { return macro; }

    ///
    const FloatVector& GetDataVectorByLabelCached(const FloatVectorSet*,
						  const string&);

    ///
    void ClearDataVectorByLabelCache() {
      data_vector_cache.clear();
    }

#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
    ///
    bool CreateLevelDB(const string&, const string&, const ground_truth&);

    ///
    list<vector<float> > RunCaffe(const DataBase *db, const string& name,
				  const vector<size_t>& img);

    ///
    list<vector<float> > RunCaffe(const DataBase *db, const string& name,
				  const list<string>& img);

    ///
    list<vector<float> > RunCaffe(const DataBase *db, const string& name,
				  const list<imagedata>& img);
    ///
    list<vector<float> > RunCaffeFeature(const string& name,
					 const list<imagedata>& img);
    ///
    list<vector<float> > RunCaffeOld(const DataBase *db, const string& name,
				     const list<imagedata>& img);
#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE

    ///
    bool ReadExternalMetaData(const string&, const map<string,string>&);

    ///
    bool ReadCOCO(const map<string,string>&);

#ifdef PICSOM_USE_JAULA
    ///
    bool ReadCOCOjaula(const string&, const string&, const string&, const string&,
		       const string&, const string&, const string&);
#endif // PICSOM_USE_JAULA

#ifdef HAVE_JSON_JSON_H
    ///
    bool ReadCOCOjsoncpp(const string&, const string&, const string&, const string&,
			 const string&, const string&, const string&);

    ///
    list<pair<string,list<vector<pair<float,float> > > > > COCOmasks(size_t);
#endif // HAVE_JSON_JSON_H

    ///
    bool ReadVisualGenome(const map<string,string>&);

    ///
    list<pair<string,imagedata> > ObjectMasks(size_t);

    ///
    list<pair<string,imagedata> > ObjectMasksCombined(size_t);

    ///
    list<imagedata> ObjectMasks(size_t, const string&);

    ///
    imagedata ObjectMasksCombined(size_t, const string&);

    ///
    map<string,list<pair<vector<float>,string> > > &
    OdWordBox(const string& s) { return odset_word_box[s]; }

    ///
    list<XmlDom> FindDetectedObjectInfo(const string&, const string&);

    ///
    bool GenerateSubtitles(size_t idx, const string& type,
			   const string& spec, const string& file);

    ///
    const list<string>& ErfDetectionImages() const {
      return erf_detection_images;
    }
    
    ///
    const list<map<string,string> >&
    LinkedDataMappings(const string& s) const {
      static const list<map<string,string> > empty;
      if (linked_data_mappings.find(s)==linked_data_mappings.end())
	return empty;
      else
	return linked_data_mappings.at(s);
    }

    ///
    class linked_data_query_t {
    public:
      /// is this really needed?
      linked_data_query_t() : nargs(0) {}

      ///
      linked_data_query_t(const string&, const string&, const string&);

      ///
      string expand(const vector<string>&) const;

      ///
      string str() const {
	return string("type=")+type+" key="+key+" url="+url+
	  " queryfile="+queryfile+" query="+query;
      }

      ///
      string type;

      ///
      string key;

      ///
      string url;

      ///
      string queryfile;

      ///
      string query;

      ///
      size_t nargs;

    }; // class linked_data_query_t

    ///
    list<vector<string> > LinkedDataQuery(const string& t, const string& s,
					  const vector<string>& a
					  = vector<string>());
    ///
    vector<pair<size_t,float> > 
    FeatureFrameDifference(size_t, const string&, size_t, size_t);

    ///
    bool ExtractFullTars() const { return extractfulltars; }

    ///
    void ExtractFullTars(bool v) { extractfulltars = v; }

    ///
    bool ExtractFullZips() const { return extractfullzips; }

    ///
    void ExtractFullZips(bool v) { extractfullzips = v; }

    ///
    void Concepts(const string& s) { concepts = s; }

    ///
    const string& Concepts() const { return concepts; }
    
#ifdef PICSOM_USE_PYTHON
    ///
    PyObject *PythonFeatureVector(const string&, size_t);
#endif // PICSOM_USE_PYTHON

    ///
    class VG_image_data {
      public:
      ///
      VG_image_data() : db(NULL), idx(-1), image_id(-1), coco_id(-1),
			flickr_id(-1), width(-1), height(-1) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_image_data(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      size_t idx, image_id, coco_id, flickr_id, width, height;

      ///
      string url;
    };  // class VG_image_data

    ///
    class VG_objects {
      public:

      class object {
      public:
	///
	vector<string> synsets, names;
	
	///
	vector<size_t> merged_object_ids;

	///
	size_t object_id, x, y, w, h;
      };

      ///
      VG_objects() : db(NULL), idx(-1), image_id(-1) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_objects(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      size_t idx, image_id;

      ///
      vector<object> objects;

      ///
      string image_url;
    };  // class VG_objects

    ///
    class VG_relationships {
      public:

      class object_t {
      public:
	///
	object_t() : object_id(-1), x(-1), y(-1), w(-1), h(-1) {}

#if defined(HAVE_JSON_JSON_H)
	///
	object_t(const Json::Value&);
#endif // HAVE_JSON_JSON_H

	///
	vector<string> names, synsets;
	
	///
	vector<size_t> merged_object_ids;
	
	///
	size_t object_id, x, y, w, h;

	///
	string str() const;
      };

      class relationship {
      public:
	///
	relationship() : relationship_id(-1) {}

	///
	size_t relationship_id;

	///
	string predicate;

	///
	vector<string> synsets;

	///
	object_t subject, object;

	///
	string str() const;
      };

      ///
      VG_relationships() : db(NULL), idx(-1), image_id(-1) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_relationships(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      size_t idx, image_id;

      ///
      vector<relationship> relationships;
    };  // class VG_relationships

    ///
    class VG_attributes {
      public:

      class attribute {
      public:
	///
	attribute() : object_id(-1) {}

	///
	vector<string> attributes;
	
	///
	size_t object_id;
      };

      ///
      VG_attributes() : db(NULL), idx(-1), image_id(-1) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_attributes(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      size_t idx, image_id;

      ///
      vector<attribute> attributes;
    };  // class VG_attributes

    ///
    class VG_region_descriptions {
      public:

      class region {
      public:
	///
	string phrase;
	///
	size_t region_id, image_id, x, y, width, height;
      };

      ///
      VG_region_descriptions() : db(NULL), idx(-1), image_id(-1) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_region_descriptions(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      size_t idx, image_id;

      ///
      vector<region> regions;
    };  // class VG_region_descriptions

    ///
    class VG_region_graphs {
      public:

      class object {
      public:
	///
	string name;

	///
	vector<string> synsets;

	///
	size_t object_id, x, y, w, h;
      };
      
      class synset {
      public:
	///
	string entity_name, synset_name;

	///
	size_t entity_idx_start, entity_idx_end;
      };

      class relationship {
      public:
	///
	string predicate;

	///
	vector<string> synsets;

	///
	size_t relationship_id, subject_id, object_id;
      };
      
      class region {
      public:
	///
	vector<object> objects;
	
	///
	vector<synset> synsets;

	///
	vector<relationship> relationships;
	
	///
	string phrase;
	
	///
	size_t region_id, image_id, x, y, width, height;
      };

      ///
      VG_region_graphs() : db(NULL), idx(-1), image_id(-1) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_region_graphs(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      size_t idx, image_id;

      ///
      vector<region> regions;
    };  // class VG_region_graphs

    ///
    class VG_scene_graphs {
      public:

      class object {
      public:
	///
	vector<string> synsets, names, attributes;
	
	///
	size_t object_id, x, y, w, h;
      };

      class relationship {
      public:
	///
	string predicate;

	///
	vector<string> synsets;

	///
	size_t relationship_id, subject_id, object_id, x, y, w, h;
      };

      class scene {
      public:
	///
	vector<object> objects;
	
	///
	vector<relationship> relationships;
	
	///
	size_t image_id;
      };	
      
      ///
      VG_scene_graphs() : db(NULL), idx(-1), image_id(-1) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_scene_graphs(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      size_t idx, image_id;

      ///
      vector<object> objects;

      ///
      vector<relationship> relationships;
    };  // class VG_scene_graphs

    ///
    class VG_synsets {
      public:

      ///
      VG_synsets() : db(NULL) {}

#if defined(HAVE_JSON_JSON_H)
      ///
      VG_synsets(const Json::Value&, DataBase*);
#endif // HAVE_JSON_JSON_H

      ///
      string str() const;

      ///
      DataBase *db;

      ///
      string synset_name, synset_definition;
    };  // class VG_synsets

    ///
    const VG_attributes::attribute& VG_get_attribute(size_t); 

    ///
    const VG_relationships::relationship& VG_get_relationship(size_t);

    ///
    vector<pair<string,float> > VG_stats(const string&, bool);

    ///
    vector<size_t> VG_find(const string&, const string&, 
			   const string&, const string&,
			   const map<string,pair<string,float> >&);
    ///
    bool VG_attrib_match(const string&, const VG_relationships::relationship&,
			 const string&, const pair<string,pair<string,float> >&);

    ///
    class graph_item {
    public:
      ///
      enum type_t { undef, node, edge };

      ///
      type_t type;

      ///
      size_t idx; // per line in textual representation

      ///
      string id; // symbolic replacement of idx

      ///
      string name;

      ///
      string content;

      ///
      float weight;

      ///
      map<string,pair<string,float> > attrib;

      ///
      vector<size_t> links;
    }; // class graph_item

    ///
    typedef vector<graph_item> graph;

    ///
    graph ReadGraph(const string&);

    ///
    vector<pair<size_t,double> > FindByGraph(const string&, const graph&,
					     size_t);

    ///
    class cat2wn {
    public:
      ///
      bool read(const string&);

      /// not implemented yet
      vector<string> category_to_wordnet(const string&) const;

      /// not implemented yet
      vector<string> categories_by_synset(const string&) const;

      /// not implemented yet
      vector<string> category_list() const;

      /// not implemented yet
      vector<string> synset_list() const;

      ///
      multimap<string,string> cmap;
    }; // class cat2wn

    ///
    bool ReadCat2Wn(const string&);

    /// not implemented yet
    vector<string> CategoryList(const string& = "");

    /// not implemented yet
    vector<string> SynsetList(const string& = "");

  protected:
    /// This points back to the system.
    PicSOM *pic_som;

    /// List of views.
    ListOf<DataBase> view;

    /// Parent of this view.
    DataBase *parent_view;

    /// Each index gets its structure here.
    vector<Index*> index;

    /// Canonical name.
    string name;

    /// Displayed name.
    string longname;

    /// Short description text.
    string shorttext;

    /// Full slash-starting path.
    string permanentpath;

    /// Full slash-starting path.
    string path;

    /// Full slash-starting secondarypath.
    string secondarypath;

    /// Full slash-starting path for creating new sqlite3 db.
    //string path_sqlite3;

    /// Specifiers for additional extractions to be added in settings.xml
    string extractionspec;

    /// True if this database does not really include anything.
    bool isdummydb;

    /// Lengths of the label subdirectory names, traditionally 2+2+2+2.
    vector<size_t> labelformat;

    /// How to convert original labels into PicSOM's system.
    string labelimport;
    
    ///
    RegExp label_re1, label_re2;

    /// File to hold labels of all objects, typically "labels".
    string labels;

    /// File to hold origins info of all objects, typically "origins".
    string origins;

    /// Filename of settings.xml if read from disc when creating a new db.
    string settings;

    /// Root directory name to hold the objects, "images" or "objects".
    string objdir;

    /// Path to the features directory.
    string features;

    /// Handle to the XML formed settings file.
    ModifiableFile settingsfile;

    /// Handle to the labels file.
    ModifiableFile labelsfile;

    /// Appending stream to labels file.
    ofstream labels_ofs;
    
    /// Files in classes subdirectory.
    classfiles_t classfiles;

    /// Path to the contents file.
    string contentsname;

    /// Long content description.
    xmlNodePtr longtext_xml;

    /// Settings file in XML.
    XmlDom settingsxml;

    /// Expanded feature extraction descriptions.
    XmlDom features_expanded_xml;

    /// Stored CbirAlgorithm factory instances.
    vector<CbirAlgorithm*> algorithms;

    /// "image/jpeg" or "image/png" etc.
    string thumbnailmime;

    /// Size "WxH,WxH,..." of thubnails.
    string thumbnailsize;

    /// Size "WxH" of virtual images to be generated.
    string virtualimagesize;

    /// Size "WxH,WxH,..." of virtual thubnails to be generated.
    list<pair<string,string> > virtualthumbnailsize;

    /// Type of thumbnails.
    string tn_type;

    /// Some combination of file+frames+seconds+minutes+shots 
    string videoinsert;

    /// Some combination of file+frames+seconds+minutes+shots 
    string videoextract;

    /// True if CreateFromScratch() has been called.
    bool created_from_scratch;

    /// Normally true, but false eg. for coil-100.
    bool has_origins;

    /// True eg. for corel and models.
    bool wxh_reversed_in_origins;

    /// Set this to true if .div files can map objects to unlabeled SOM nodes.
    bool empty_labels_allowed;

    /// Set this to true if xx/xx/features/*.dat have been collected.
    bool ignore_leaf_features;

    /// Set to true if main object should use child's text contents.
    bool use_subobject_text;

    /// Set to true if target_imagesegment has replaced target_segment.
    bool segment_is_modifier;

    /// Set to true if we should generate implicit subobject index
    bool use_implicit_subobjects;

    /// Alternative path to extract object to, for example with more disk space
    string extractobjectpath;

    /// Extra dir to search features from
    string extra_features_dir;

    /// Size when maps were trained.
    int original_size;

    /// Size when started.
    int startup_size;

    /// Default values for queries.
    Query *defaultquery;

    /// For old databases such as corel.
    target_type default_feature_target;

    /// Information per each object.
    object_set _objects;

    /// Count statistics of all target types in objects.
    tt_stat_t tt_stat;

    /// Class memberships.
    ground_truth_set contents;

    /// x,y coordinates given for ground_truth classes.
    map<string,xymap_t> all_xymaps;

    /// List of accessed servants and clients.
    ListOf<RemoteHost> access;

    /// Force compilation or not, default false
    static bool force_compile;

    /// True if locks are debugged.
    static bool debug_locks;

    /// True if GroundTruth*() are debugged.
    static bool debug_gt;

    /// True if GroundTruthExpand() is trapped.
    static bool debug_gt_exp;

    /// True if Create{Virtual,Segment}{Image,Object}() and 
    /// TSSOM::AddToXMLmapcollage() are debugged.
    static bool debug_vobj;

    /// All thumbnail operations are debugged.
    static bool debug_tn;

    /// True if ReadOriginsInfo() is traced.
    static bool debug_origins;

    /// True if SolveObjectPath*() are traced.
    static bool debug_opath;

    /// True if LeafDirectoryFileList() is traced.
    static size_t debug_leafdir;

    /// True if *Detetion*() are traced.
    static size_t debug_detections;

    /// True if *Captioning*() are traced.
    static size_t debug_captionings;

    /// True to force call to DumpObjects() in ReadLabels/MakeSubObjectIndex().
    static bool debug_dumpobjs;

    /// True to get messages like "Read/Wrote [compiled] class file <xxx>".
    static bool debug_classfiles;

    /// Debugging levels for features program etc.
    static size_t debug_features;

    /// Debugging levels for features program etc.
    static size_t debug_segmentation;

    /// Debugging levels for features program etc.
    static size_t debug_images;

    /// Whether to allow writing in SQL databases.
    static bool open_read_write_sql;

    /// Whether to allow writing in binary feature files.
    static bool open_read_write_fea;

    /// Whether to allow writing in binary detection files.
    static bool open_read_write_det;

    /// Whether to allow writing in (lucene) text files.
    static bool open_read_write_txt;

    ///
    static bool force_lucene_unlock; 

    ///
    static bool parse_external_metadata;

    /// Whether to use mplayer.
    static bool insert_mplayer;

    /// Whether sql operations are traced.
    static bool debug_sql;

    /// Whether text operations are traced.
    static bool debug_text;

    /// Whether access rules are traced.
    bool debug_access;

    /// Restriction of an image subset in database-wide manner.
    ground_truth restriction;

    /// Restriction that defines the subset of face images.
    ground_truth face_restriction;

    /// Restriction that defines the subset of object images.
    ground_truth object_restriction;

    /// Restriction that defines the subset of location images.
    ground_truth location_restriction;

    /// Temporary storage for some key=value specifications.
    vector<pair<string,string> > postponed;

    /// Time when access file was last read.
    struct timespec access_read_time;

    /// Time when sqlite3.db was modified if existed.
    struct timespec sqlite_mod_time;

    /// True if call to Query::AddToXMLstatistics() is too slow
    bool no_statistics;

    /// How uploads/insertions are performed?
    insertmode_t insertmode;

    /// false if SolveMissingOriginsInfo() should be just no-op.
    bool insertobjectsrealinfo;

    ///
    bool insertallow_no_target;
    
    /// Map of index name -> feature file name found in settings.xml.
    map<string,string> index_to_featurefile;

    /// Map of feature file name -> feature method name found in settings.xml.
    map<string,string> featurefile_to_method;

    /// List of segmentation names found in settings.xml.
    map<string,list<pair<string,string> > > described_segmentations;

    /// List of detection names found in settings.xml.
    map<string,list<pair<string,string> > > described_detections;

    /// List of captioning names found in settings.xml.
    map<string,list<pair<string,string> > > described_captionings;

    /// Mapping from captionings' textindices to names
    map<string,string> captioning_textindex2name;

    /// List of media extraction names found in settings.xml.
    map<string,list<pair<string,string> > >  described_medias;

    /// List of object insertion method names found in settings.xml.
    map<string,list<pair<string,string> > >  described_objectinsertions;

    /// List of text index searches found in settings.xml.
    map<string,textsearchspec_t> described_textsearches;

    /// List of text index names found in settings.xml.
    map<string,list<pair<string,string> > > described_textindices;

    /// List of text index names found in settings.xml.
    map<string,Connection*> textindices;

    ///
    list<string> textindices_stored_lines;

    /// Some combination of "seg", "sql", etc.
    string storedetections;

    /// The name of detections to show in the UI 
    string showdetections;

    /// Set true in SolveExtractions() to avoid multiple executions.
    bool extractions_solved;

    /// Set true in SolveTextIndices() to avoid multiple executions.
    bool textindices_solved;

    /// Set true in FindAllClasses() to avoid multiple executions.
    bool all_classes_found_plain, all_classes_found_contents;

    ///
    list<string> bindetectionnames;

    /// Cached return from FindAllClassNames() if not force=true;
    list<string> class_name_cache;

    /// Cached return from SplitClasNames().
    map<string,list<string> > split_class_names_cache;
    
    /// Set to false if InsertObjects() should not call 
    /// ReWriteChangedDivisionFiles().
    bool write_changed_divfiles;

    /// THE FOLLOWING bools SHOULD BE IN access :
    
    /// Whether to deliver local copies of images 
    /// (does not depend on whether the local copy actually exists).
    bool deliverlocalcopies;

    /// Set to false if not be seen in database list.
    bool visible;

    /// Set to true if small media clips can be generated from large ones.
    bool mediaclips;

    /// Whether user can select the algorithm.
    bool allowalgorithmselection;

    /// Whether user can select convolution type and size.
    bool allowconvolutionselection;

    /// Whether user can select images by ground truth classes.
    bool allowcontentselection;

    /// Whether new images can be added.
    bool allowinsertion;

    /// Whether query can be initiated with own image.
    bool allowownimage;

    /// Whether segmentation images can be inserted
    bool allowsegimage;

    /// Whether textual searches are allowed.
    bool allowtextquery;

    /// Whether user can define ground truth classes.
    bool allowclassdefinition;

    /// Whether user can store retrieval results as classes.
    bool allowstoreresults;

    /// Whether imageinfo should show existing segmentations.
    bool showsegmentations;

    /// Whether user can select the object types.
    bool allowobjecttypeselection;

    /// Whether user can select the display mode (images/texts/messages/...).
    bool allowdisplaymodeselection;

    /// Whether the server notifies about the labels inserted since
    /// last request
    bool allowarrivedobjectnotification;

    /// Whether scaling of images is allowed.
    bool allowimagescaling;

    /// Whether processes with development==false should load this.
    bool nondevelopmentuse;

    /// The name of class METACLASSFILE to use in classification.
    // string classificationset;

    /// The name of class METACLASSFILE used by FindAllClasses().
    string allcontentsclasses;

    /// The name of class METACLASSFILE used by FindAllBinDetections().
    string alldetections;

    /// Stored class models.
    vector<Query*> class_models_new;

    /// A vector containing the indices of the labels arrived during
    /// the server session
    vector<int> arrivedobjects;

    /// If !=1, specifies that only every n'th frame of a video is used.
    size_t framestep;

    /// If !=0, specifies that margins should be applied in video features.
    size_t margin;

    /// The target_type-specific default aspects
    map<target_type, aspects> defaultaspects;

    /// The target_type-specific default aspects cached
    map<target_type, aspects> defaultaspects_cache;

    /// Reverse mapping from URL field in origins file back to index.
    map<string,int> url_to_index;

    ///
    map<string,list<string> > origins_file_line_cache_map;

    ///
    map<size_t,map<string,string> > origins_entry_cache_map;

    /// 
    vector<string> url_blacklist;

    /** The feature aliases. The map key is the name of the alias, the
     *  first string of the pair is the description, and the list contains
     *  the names of the features this alias points into.
     */
    featurealias_t featurealias;

    featureaugmentation_t featureaugmentation;

    /** The named command lines. The map key is the name of the rule, the
     *  first string of the pair is the description, and the list contains
     *  the parameters of the rule.
     */
    namedcmdline_t namedcmdline;

    /// The link icon mode (text/image/none links under the query objects)
    string linkiconmode;

    ///
    typedef map<string,pair<double,double> > gaze_coord_norm_t;

    ///
    gaze_coord_norm_t gaze_coord_norm;

    ///
    typedef map<string, pair<string,string> > textqueryopt_t;

    /// text query options
    textqueryopt_t textqueryopt;
    
    /// text query processing log
    string textquerylog;

    ///
    static bool remove_broken_datafiles;

    /// list of files created on the fly, potentially to be removed
    list<string> created_files;
    
    ///
    map<string,SimpleList> simplelist_map;

#ifdef USE_MRML
    /// HASH table that maps http_path (original file path+name) into picsom
    /// filename
    originalfname_type  originalfname;
#endif // USE_MRML

#ifdef PICSOM_USE_PTHREADS
    ///
    RwLock rwlock;
#endif // PICSOM_USE_PTHREADS

    ///
    videofile input_image_mplayer_scene;

    ///
    videofile input_image_mplayer_face;

    ///
    bool use_context;

#ifdef PICSOM_USE_CONTEXTSTATE
    ///
    ContextState context;
#endif // PICSOM_USE_CONTEXTSTATE

#ifdef PICSOM_USE_OD
    ///
    ObjectDetection odone;

    ///
    map<string,ObjectDetection> odset;
#endif // PICSOM_USE_OD

    ///
    map<string,map<string,list<pair<vector<float>,string> > > > odset_word_box;

    /// 
    size_t sqlite_mode;

    /// 
    size_t mysql_mode;

    ///
    set<string> sql_files;

#ifdef HAVE_SQLITE3_H
    ///
    sqlite3* sql3;
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    ///
    MYSQL* mysql;

    ///
    RwLock mysql_lock;

    ///
    string mysql_dbname;
#endif // HAVE_MYSQL_H

    /// Used when UseSql();
    string next_free_label;

    /// true if objects are stored in SQL;
    bool sqlobjects;

    /// true if features are stored in SQL;
    bool sqlfeatures;

    /// true if classes are stored in SQL;
    bool sqlclasses;

    /// true if indices are stored in SQL;
    bool sqlindices;

    /// true if settings are stored in SQL;
    bool sqlsettings;

    /// true if all is stored in SQL;
    bool sqlall;

    ///
    map<string,bool> sql_blob_info;

    ///
    map<string,string> sql_alias;

    ///
    set<string> sql_table;

    ///
    bool nofeaturedata;

    ///
    bool use_bin_features_read;

    ///
    bool use_bin_features_write;

    ///
    bool tempmediafiles;

    ///
    bool tempimagefiles;

    ///
    map<string,script_exp_t> setup;

    ///
    map<size_t,vector<float> > gaze_relevance_group_data_pos;
    map<size_t,vector<float> > gaze_relevance_group_data_neg;

    ///
    list<string> tempdirs, tempfiles;

    ///
    map<string,SVM*> svms;

    ///
    Analysis *analysis_async_deprecated;

    ///
    static bool use_pthreads_features;

    ///
    static bool use_pthreads_detection;

    ///
    static bool mysql_library_initialized;

    ///
    map<string,map<string,FloatVector> > interpolated_vectors;

    ///
    map<string,list<pair<string,string> > > textindex_fields;

    ///
    map<string,string> macro;

    ///
    map<const FloatVectorSet*,map<string,const FloatVector*> >
       data_vector_cache;

    ///
    map<string,float> finnish_firstnames, finnish_lastnames;

    ///
    string facebox;

    ///
    set<string> tars_extracted;

    ///
    bool extracted_files;

    ///
    bool alwaysusetarfiles;

    ///
    bool extractfulltars;

    ///
    bool extractfullzips;

    ///
    bool uselocalzipfiles;

#ifdef HAVE_ZIP_H
    ///
    map<string,zip_t*> zipfiles;
#endif // HAVE_ZIP_H
    
    ///
    map<string,bin_data> bindetections;

    ///
    map<string,pair<size_t,map<float,size_t> > > ranks;

    ///
    map<string,pair<lsc,lsc> > lscset;

    ///
    map<size_t,float> fps_cache;

    ///
    bool overwritedetections;

    ///
    list<string> erf_detection_images;

#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
    ///
    class caffe_t {
    public:
      ///
      caffe_t() : net(NULL) {}

      ///
      caffe_t(const string& na, const string& pt, const string& mo,
	      const string& me, caffe::Net<float> *ne,
	      const imagedata& mi, const vector<size_t>& ma,
	      const string& f) :
	name(na), prototxt(pt), model(mo), mean(me), net(ne),
	mimg(mi), map(ma), fusion(f) {}

      ///
      ~caffe_t() { /*delete net;*/ }

      ///
      string name, prototxt, model, mean;

      ///
      caffe::Net<float> *net;

      ///
      imagedata mimg;

      ///
      vector<size_t> map;

      ///
      string fusion;

    }; // class caffe_t

    ///
    map<string,caffe_t> caffemap;

    ///
    string caffefusion;

#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE

    ///
    map<string,XmlDom> detected_object_info;

    ///
    map<string,linked_data_query_t> linked_data_queries;

    ///
    map<string,trecvid_med_event> trecvid_med_event_map;

    ///
    map<string,map<string,string> > url_rules;

    ///
    map<string,list<string> > feature_re_list;

    ///
    RwLock feature_re_list_lock;

    ///
    map<size_t,pair<vector<pair<size_t,size_t> >,
		    vector<pair<size_t,size_t> > > > segment_frames;

    ///
    map<size_t,size_t> segment_to_middleframe, middleframe_to_segment;

    ///
    map<size_t,map<string,pair<size_t,double> > > idf;

    ///
    map<string,vector<string> > feature_txt_files;

    ///
    map<string,pair<pair<istream*,size_t>,pair<size_t,size_t> > >
        numpy_feature_info;

    ///
    string concepts;
    
#ifdef HAVE_JSON_JSON_H
    ///
    map<size_t,Json::Value> coco_json;

    ///
    map<size_t,string> coco_category;
#endif // HAVE_JSON_JSON_H

    ///
    map<string,list<map<string,string> > > linked_data_mappings;

    ///
    string externalmetadatatype;

    ///
    map<string,string> externalmetadata_param;

    /// a typical value to override default externalmetadata_param
    string frag;

#ifdef PICSOM_USE_PYTHON
    ///
    map<string,PyObject*> neuraltalk_models;
#endif // PICSOM_USE_PYTHON

    map<size_t,VG_image_data>          vg_image_data;
    map<size_t,VG_objects>             vg_objects;
    map<size_t,VG_relationships>       vg_relationships;
    map<size_t,VG_attributes>          vg_attributes;
    map<size_t,VG_region_descriptions> vg_region_descriptions;
    map<size_t,VG_region_graphs>       vg_region_graphs;
    map<size_t,VG_scene_graphs>        vg_scene_graphs;
    map<size_t,VG_synsets>             vg_synsets;

    map<string,cat2wn> cat2wm_map;

  };  // class DataBase

#ifdef PY_VERSION

  inline PyObject *PyString_FromString_x(const char *s) {
#if PY_MAJOR_VERSION < 3
      return PyString_FromString(s);
#else
      return PyUnicode_FromString(s);
#endif // PY_MAJOR_VERSION
    }

  inline const char *PyString_AsString_x(PyObject *s) {
#if PY_MAJOR_VERSION < 3
      return PyString_AsString(s);
#else
      return _PyUnicode_AsString(s);
#endif // PY_MAJOR_VERSION
    }

  inline PyObject *PyInt_FromLong_x(long int s) {
#if PY_MAJOR_VERSION < 3
      return PyInt_FromLong(s);
#else
      return PyLong_FromLong(s);
#endif // PY_MAJOR_VERSION
    }

  inline long PyInt_AsLong_x(PyObject *s) {
#if PY_MAJOR_VERSION < 3
      return PyInt_AsLong(s);
#else
      return PyLong_AsLong(s);
#endif // PY_MAJOR_VERSION
    }

#endif // PY_VERSION
    
} // namespace picsom

#endif // _PICSOM_DATABASE_H_

