// -*- C++ -*-  $Id: Index.h,v 2.57 2015/10/09 12:08:13 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_INDEX_H_
#define _PICSOM_INDEX_H_

#include <Feature.h>
#include <ValuedList.h>
#include <ground_truth.h>
#include <bin_data.h>

#include <iterator>
#include <limits>
using namespace std;

namespace picsom {
  /// 
  class DataBase;

  ///
  typedef map<target_type,int> tt_stat_t;

  /**
     Abstract base class for TSSOM etc objects.
  */
  class Index {
  protected:
    /**
       The constructor for the Index class.
       @param db     the database where features have been calculated
       @param name   canonical name of the index
       @param feat   canonical name of the used feature set
       @param path   full path to directory containing the files
    */
    Index(DataBase *db, const string& name, const string& feat,
	  const string& path, const string& params, const Index* src);

    /// Constructor that adds reference to that instance in list_of_indices.
    Index(bool is_dummy) {
      if (is_dummy)
	dummy_index = this;
      else {
	next_of_indices = list_of_indices;
	list_of_indices = this;
      }
    }

    ///
    Index() { abort(); }

    ///
    Index(const Index&) { abort(); }

    ///
    void CopyFrom(const Index*);

  public:
    ///
    virtual ~Index();

    ///
    virtual const string& MethodName() const = 0;

    ///
    virtual Index* Make(DataBase *db, const string& name, const string& feat,
			const string& path, const string& params,
			const Index *src) const = 0;
    
    ///
    static Index* Find(DataBase* db, const string& fn, 
		       const string& dirstr, const string& params,
		       bool create=false, bool allow_dummy=false,
		       const Index* = NULL);
    ///
    static Index* Find(DataBase* db, const string& fn, 
		       const list<string>& dirs, const string& params,
		       bool create=false, bool allow_dummy=false,
		       const Index* = NULL);
    ///
    virtual bool IsMakeable(DataBase* db, const string& fn, const string& dirstr,
			    bool create=false) const = 0;

    ///
    virtual bool IsDummy() const { return false; }

    ///
    static void ReorderMethods();

    /** Produces a list of the MethodName()s of all know indexing methods.
	\return list of index method names.
    */
    static list<string> MethodList() {
      list<string> l;
      for (const Index *p = list_of_indices; p;
	   p=p->next_of_indices)
	l.push_back(p->MethodName());
      return l;
    }

    /** Prints a list of the registered indexing methods
	\param os the output stream to print to (default cout).
	\param wide set to true if wide output is prefered.
    */
    static void PrintMethodList(ostream& os = cout, bool wide = false);

    ///
    class State;
    virtual State *CreateInstance(const string&) = 0;

    ///
    static void DebugTargetType(bool d) { debug_target_type = d; }

    ///
    const string& DataBaseName() const;

    /// Interprets key=value.
    bool Interpret(const pair<string,string>& kv, int& r) {
      return Interpret(kv.first, kv.second, r);
    }

    /// Interprets key=value.
    virtual bool Interpret(const string&, const string&, int&);

    /// Finds full file paths.  Delete after use!
    char *FindFileM(const char*, const char* = NULL, bool = false) const;

    /// Finds full file paths.
    string FindFile(const string& b, const string& e = "", bool l = false){
      char *tmp = FindFileM(b.c_str(), e.size()?e.c_str():NULL, l);
      string ret = tmp?tmp:"";
      free(tmp);
      return ret;
    }

    /// Returns the path of the index files.
    const string& Path() const { return path; }

    /// Returns the name of the feature, like "colm".
    const string& FeatureMethodName() const {
      return featuremethodname;
    }

    /// Returns the name of the feature, like "zo5:colm".
    const string& FeatureFileName() const {
      return featurefilename;
    }

    /// Returns the extra spec of the feature, like "-hkm-int".
    const string& FeatureFileNameExtraSpec() const {
      return featureextraspec;
    }

    /// Returns the name of the feature, like "zo5:colm-hkm-int".
    string FeatureFileNameWithExtra() const {
      return featurefilename+featureextraspec;
    }

    /// Returns "airplane";
    const string& ClassName() const { return classname; }

    /// Returns the name of the index, 
    ///   like "linear::zo5:colm::hkm-int::ZZ#airplane".
    const string& IndexName() const { return indexname; }

    /// Like "7"
    const string& InstanceSpec() const {
      return instancespec;
    }

    /// Like "svm::zo5:colm-hkm-int#airplane§7"
    string IndexNameWithInstanceSpec() const {
      string s = IndexName();
      if (InstanceSpec()!="")
	s += "§"+InstanceSpec();
      return s;
    }

    /// Returns the name of the feature. To be obsoleted.
    const string& Name() const { return IndexName(); }

    /// Returns the longname of the feature.
    const string& LongName() const {
      return longname.size()?longname:Name();
    }

    /// Returns the shorttext of the feature.
    const string& ShortText() const {
      return shorttext.size()?shorttext:LongName(); }

    /// Returns the longtext of the feature.
    const string& LongText() const { return ShortText(); }

    /// An utility.
    void ShowAllNames() const;

    /// A helper routine for the ones below.
    DataBase *CheckDB() const {
      if (!db) {
	ShowError("Index::CheckDB() aborting");
	SimpleAbort();
      }
      return db;
    }

    /// A helper routine to shorten filenames for display.
    string ShortFileName(const string& n) const;

    /// Separator between index method name and index name.
    static const string& MethodSeparator() {
      static string s = "::"; // "-" ":"
      return s;
    }

    /// This has to be called in all final constructors().
    bool SetFeatureNames();

    /// Checks if some files have changed and rereads them.
    virtual bool CheckModifiedFiles() { return true; }
    
    /// Returns true if argument is valid object index in the data base.
    bool IndexOK(int i) const; // { return CheckDB()->IndexOK(i); }

    /// Returns the label of i'th item in data base in a better way.
    const string& Label(int i) const; // { return CheckDB()->Label(i); }

    /// Returns the label of i'th item in data base in a better way.
    const char *LabelP(int i) const; // { return Label(i).c_str(); }

    /// Returns the index of a label or -1 if not known yet.
    int LabelIndex(const string& s) const; // {
      // return CheckDB()->LabelIndex(s);
    // }

    /// Returns the target_type of i'th item in data base in a better way.
    target_type ObjectsTargetType(int i) const; // {
      // return CheckDB()->ObjectsTargetType(i);
    // }

    /// Returns the number of objects in the data base.
    int DataSetSize() const; // { return CheckDB()->Size(); }

    /// True if feature_target is a "subset" of the proposed type.
    bool CompatibleTarget(target_type t) const;

    /// Read access to feature_target member.
    target_type FeatureTarget() const { return feature_target; }

    /// Stringified feature_target member.
    string FeatureTargetString() const {
      return TargetTypeString(feature_target);
    }

    /// Modify access to feature_target member.
    void FeatureTarget(target_type tt, bool guess = false) {
      target_type old_tt = feature_target;
      stringstream ss;
      ss << " [" << FeatureTargetString() << "]->[";
      feature_target = tt;
      ss << FeatureTargetString() << "]";
      if (guess || (old_tt!=tt && old_tt!=target_no_target))
	WriteLog("Target type ", guess ? "guessed" : "changed", ss.str());
    }

    /// Modify access to feature_target member.
    void FeatureTarget(const string& tts) {
      // some compatibility tricks...
      FeatureTarget(tts=="imagefile" ? target_image : SolveTargetTypes(tts));
    }

    /// Tries to solve feature_target by reading some file headers.
    virtual bool GuessFeatureTarget() { return false; }

    /// Reads all files if force or not already read. If nodata, skip vectors.
    // virtual bool ReadFiles(bool force, bool nodat) = 0;

    /// Returns true if this feature can appear in the user interface.
    virtual bool IsSelectable() = 0;

    /// Function that sets the contents of tt_stat.
    virtual void SetTargetCounts() { tt_stat.clear(); }

    /// Function that adds one entry in tt_stat.
    void AddOneTarget(target_type tt) { tt_stat[tt]++; }

    /// String form of target_type statistics.
    string TargetCounts() const;

    /// Writes strings to log.
    void WriteLog(const string& m1, const string& m2 = "",
		  const string& m3 = "", const string& m4 = "", 
		  const string& m5 = "") const {
      Picsom()->WriteLogStr(LogIdentity(), m1, m2, m3, m4, m5); }

    /// Writes contents of a stream to log.
    void WriteLog(ostringstream& os) const {
      Picsom()->WriteLogStr(LogIdentity(), os); }

    /// Gives log form of Index name.
    string LogIdentity() const {
      ostringstream s;
      s << RwLockStatusChar() << MethodName() << '{' << DataBaseName()
	<< '/' << Name() << " [" << FeatureTargetString() << "]}";
      return s.str();
    }

    ///
    PicSOM *Picsom() const; // { return db->Picsom(); }

    ///
    virtual void ReduceMemoryUse() {}

    /// Returns true if client of query is allowed to use this.
    bool CheckPermissions(const Query&) const;

    /// Starts ticking.
    void Tic(const char *f) const; // { db->Tic(f); }

    /// Stops tacking.
    void Tac(const char *f = NULL) const; // { db->Tac(f); }

    /// Adds TSSOM object information to the XML document.
    // virtual bool AddToXML(XmlDom&, object_type, const string&, 
    // 			  const string&, const Query*) = 0;

    ///
    bool AddToXMLfeatureinfo(XmlDom&, const Query*) const;

    /// Adds query-independent feature information to the XML document.
    virtual bool AddToXMLfeatureinfo_static(XmlDom&) const { return true; }

    /// Sets features_command;
    virtual bool SolveFeatureCalculationCommand(bool, bool) { return false; }

    /// Returns true if features can be calculated.
    bool CanCalculateFeatures() const { return !features_command.empty(); }

    /// Sets -d -D -m switches for features command.
    bool SetFeaturesDebugging(vector<string>&) const;

    /// Externally set the feature calculation command.
    void FeaturesCommand(const vector<string>& v, bool show) {
      features_command = v;
      if (show)
	WriteLog("Feature calculation command set to ["+ToStr(v)+"] target="+
		 FeatureTargetString()+" subdir="+subdir);
    }

    /// Externally set something eg. in DataBase::DescribedIndex().
    void SetProperty(const string& key, const string& val) {
      property[key] = val;
    }

    /// Read sometging set externally eg. in DataBase::DescribedIndex().
    string GetProperty(const string& key) const {
      return HasProperty(key) ? property.at(key) : "";
    }

    ///
    bool HasProperty(const string& key) const {
      return property.find(key)!=property.end();
    }

    /// Externally set the subdir.
    void Subdir(const string& s) { subdir = s; }

    /// Externally set the segmentation command.
    void SegmentationCommand(const string& c) {
      cout << "SegmentationCommand(" << c << endl;

      istringstream ss(c);
      vector<string> scmd;
      copy(istream_iterator<string>(ss), istream_iterator<string>(), 
           back_inserter(scmd));
      SegmentationCommand(scmd);
    }

    ///
    void SegmentationCommand(const vector<string>& c) {
      segmentation_command = c;
      WriteLog("Segmentation command set to [", ToStr(c), "] target=",
	       FeatureTargetString());
    }

    ///
    virtual bool CalculateFeatures(size_t idx, set<string>& done_segm,
				   XmlDom& xml) {
      vector<size_t> v;
      v.push_back(idx);
      return CalculateFeatures(v, done_segm, xml, NULL);
    }

    ///
    virtual bool CalculateFeatures(vector<size_t>&, set<string>&, XmlDom&,
				   bin_data*);

    ///
    int CalculateFeaturesPthreads(vector<string>&,
				  vector<string>&, feature_result&);
    
    ///
    static void *CalculateFeaturesPthreadsCall(void*);

    ///
    virtual string CalculatedFeaturePath(const string&, bool, bool) const {
      return "";
    }

    /// Calculates segmentation for the new object.
    /// Called by CalculateFeatures(int).
    bool CalculateSegmentation(size_t, const string&);

    ///
    bool CalculateSegmentation(const vector<size_t>& idxv);

    /// Locks for reading.
    void RwLockRead(const char *txt = NULL);// {   db->RwLockRead(txt); }

    /// Locks for writing.
    void RwLockWrite(const char *txt = NULL);// {  db->RwLockWrite(txt); }

    /// Unlocks.
    void RwUnlockRead(const char *txt = NULL);// {
      // db->RwUnlockRead(txt); }

    /// Unlocks.
    void RwUnlockWrite(const char *txt = NULL);// {
      // db->RwUnlockWrite(txt); }

    /// Pointer to "" or "W " or "R ".
    const char *RwLockStatusChar() const;// { return db->RwLockStatusChar(); }

    /// 
    virtual size_t NumberOfScoreValues() const { return 1; }

    /// Set default weight
    void DefaultWeight(float v) { default_weight = v; }
    
    /// Get default weight
    float DefaultWeight() const { return default_weight; }

    /// Mapping from score to cumulative distribution function index.
    /// t==0 => cdf_test
    /// t==1 => cdf_devel
    /// t==2 => cdf_devel_pos
    /// t==3 => cdf_devel_neg
    float ValueToCDF(float v, int t);

    ///
    void AnalysePDF(size_t);

    ///
    bool HasPDFmodel() const { return pdf_low.Length(); }

    ///
    void CreatePDFmodels(size_t, bool, bool);

    ///
    static size_t CountInRange(const list<float>&, float, bool, float, bool);
    
    ///
    float PDFmodelRho() const;

    ///
    float PDFmodelCorProb(float, float = 0.0) const;

    ///
    float PDFmodelPrecision(float, float = 0.0) const;

    ///
    float PDFmodelRecall(float) const;

  protected:
    /// Pointer to the data base.
    DataBase *db;

    /// Name of featurefiles.
    string featuremethodname;

    /// Name of featurefiles.
    string featurefilename;

    /// Spec of feature extra operations.
    string featureextraspec;

    /// Name of index, used to be name_str and used as feature name too.
    string indexname;

    /// Longer name.
    string longname;

    ///
    string classname;

    ///
    string instancespec;

    /// Short description text.
    string shorttext;

    /// Full path to directory.
    string path;

    /// Subdirectory where actual object files are, like "aligned" in database=lfw
    string subdir;

    /// Anything can be stored here in strings.
    map<string,string> property;

    /// Command to use when new features are calculated.
    vector<string> features_command;

    /// Command to use when new segmentations are calculated.
    vector<string> segmentation_command;

    /// Type of objects stored in this structure.
    target_type feature_target;

    /// Points to the head of a linked list of indices.
    static Index *list_of_indices;

    /// Forms the chain of available indices.
    Index *next_of_indices;

    ///
    static Index *dummy_index;

    /// Counts of objects mapped with this structure.
    tt_stat_t tt_stat;

    ///
    float default_weight;

    ///
    static bool debug_target_type;

    ///
    list<float> cdf_devel, cdf_devel_pos, cdf_devel_neg, cdf_test;

    ///
    FloatVector pdf_low, pdf_all, pdf_tes, pdf_tra, pdf_pos, pdf_neg, pdf_rec;

  public:
    ///
    class State {
    protected:
      ///
      State(Index *t, const string& n = "") : index_p(t), fullname(n) {
	aspects.push_back("*");
	feature_weight = t->DefaultWeight();
      }

    public:
      ///
      virtual ~State() {}

      ///
      virtual State *MakeCopy(bool) const = 0;

      ///
      virtual float ScoreValue(int) const = 0;

      ///
      Index *StaticPart() const { return index_p; }

      ///
      bool Is(const string& m) const { return StaticPart()->MethodName()==m; }

      ///
      bool IsComplex() const {
	return fullname!="" && (!index_p || fullname!=index_p->Name());
      }

      /// Set weight
      void Weight(float v) { feature_weight = v; }
      
      /// Get weight
      float Weight() const { return feature_weight; }

      ///
      static float nan() { return numeric_limits<float>::max(); }

    private:
      ///
      Index *index_p;

    public:
      ///
      string fullname;

      ///
      vector<string> aspects;
      
      /// 
      vector<ObjectList> objlist;

    protected:
      /// 
      double feature_weight;

    };  // class Index::State

  };  // class Index

} // namespace picsom

#endif // _PICSOM_INDEX_H_

// Local Variables:
// mode: lazy-lock
// End:

