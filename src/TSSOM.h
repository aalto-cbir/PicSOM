// -*- C++ -*-  $Id: TSSOM.h,v 2.117 2015/10/09 12:07:52 jorma Exp $
// 
// Copyright 1998-2010 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_TSSOM_H_
#define _PICSOM_TSSOM_H_

#include <VectorIndex.h>

#include <ModifiableFile.h>

#include <TreeSOM.h>
#include <RandVar.h>

//struct svm_node;
//struct svm_parameter;
//struct svm_problem;

namespace picsom {
  using simple::TreeSOM;
  using simple::FloatPoint;
  using simple::IntPoint;

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  typedef map<target_type,int> tt_stat_t;

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /**
     TS-SOM and feature-related functions and members.

     There exists one TSSOM object for each triplet of data/map/division
  */

  class TSSOM : public VectorIndex {
  public:
    enum tssom_type {
      tssom_undef = -1,
      tssom_normal,
      tssom_alien_data,
      tssom_alien_map,
      tssom_alien_data_and_map
    };

    /**
       The constructor for the TSSOM class.
       @param db     the database where features have been calculated
       @param name   canonical name of feature set
       @param path   full path to directory containing the files
    */
    TSSOM(DataBase *db, const string& name, const string& path,
          const string& params="", const Index* src=NULL);

    ///
    TSSOM(bool) : VectorIndex(false) {}

    /// Poison.
    TSSOM() { abort(); }

    /// Poison.
    TSSOM(const TSSOM& t) : VectorIndex(t) { abort(); }

    /// The destructor.
    virtual ~TSSOM();

    ///
    virtual const string& MethodName() const {
      static string tssom = "tssom";
      return tssom;
    }

    ///
    virtual Index* Make(DataBase *db, const string& name,
                        const string& /*feat*/,
                        const string& path,
                        const string& params, const Index *src) const {
      return new TSSOM(db, name, path, params, src);
    }

    ///
    virtual Index::State *CreateInstance(const string& n) {
      return new State(this, n);
    }
 
    ///
    virtual bool IsMakeable(DataBase *db, const string& fn, 
                            const string& dirstr, bool create) const;
    ///
    bool IsRawFeatureName(const string& fn) const;

    /// Returns type of the structure.
    tssom_type Type() const { return type; }

    /// Returns true if type of the structure is plain old normal.
    bool TypeIsNormal() const { return type==tssom_normal; }

    /// Returns string form of type.
    static const string& TypeName(tssom_type t) {
      typedef map<tssom_type,string> m_type; 
      static m_type m;
      if (m.empty()) {
        // m[(tssom_type)-1] = "???";
        m[tssom_undef]              = "undef";
        m[tssom_normal]             = "normal";
        m[tssom_alien_data]         = "alien_data";
        m[tssom_alien_map]          = "alien_map";
        m[tssom_alien_data_and_map] = "alien_data_and_map";
      }
      m_type::const_iterator i = m.find(t);
      return (i!=m.end() ? i : m.find((tssom_type)-1))->second;
    }

    ///
    const string& TypeName() const { return TypeName(type); }

    ///
    virtual size_t NumberOfScoreValues() const { return Nlevels(); }    

    /// "Canonical name" of the feature map and its layer.
    string MapName(bool db_too, int l = -1, int k = -1) const {
      stringstream ss;
      if (db_too)
        ss << DataBaseName() << "/";
      ss << Name();
      if (l>=0)
        ss << "[" << l << "]";
      if (k>=0)
        ss << ":" << k;

      return ss.str();
    }

    /// "Canonical name" of the feature map and a set of its layer.
    string MapName(bool db_too, const vector<int>& l, int k = -1) const {
      stringstream ss;
      if (db_too)
        ss << DataBaseName() << "/";
      ss << Name();
      if (l.size()>0) {
        ss << "[";
        for (size_t i=0; i<l.size(); i++)
          ss << (i?",":"") << l[i];
        ss << "]";
      }
      if (k>=0)
        ss << ":" << k;

      return ss.str();
    }

    ///
    static const string& AlienPartSeparator() {
      static string s = "#";
      return s;
    }

    /// Extracts the xxx part of {#}xxx#yyy.
    string AlienDataBaseName() const {
      size_t f = Name().substr(0, 1) == AlienPartSeparator() ? 1 : 0;
      size_t c = Name().find(AlienPartSeparator(), f);
      return c==string::npos ? "" : Name().substr(f, c-f);
    }

    /// Extracts the yyy part of {#}xxx#yyy.
    string AlienFeatureName() const {
      size_t f = Name().substr(0, 1) == AlienPartSeparator() ? 1 : 0;
      size_t c = Name().find(AlienPartSeparator(), f);
      return c==string::npos ? "" :
        Name().substr(c+AlienPartSeparator().size());
    }

    /// Returns alien feature name made up of db and feature parts.
    static string AlienFeatureName(const string& db, const string& f,
                                   tssom_type t) {
      if (t==tssom_alien_data)
        return AlienPartSeparator()+db+AlienPartSeparator()+f;
      if (t==tssom_alien_map)
        return db+AlienPartSeparator()+f;

      ShowError("TSSOM type not understood in TSSOM::AlienFeatureName()");
      return "???";
    }

    /// Returns alien feature name made up of db and feature parts.
    string AlienFeatureName(const TSSOM& a, tssom_type t) const {
      return AlienFeatureName(a.DataBaseName(), Name(), t);
    }

    /// Resolves tssom_type from feature's name.
    static tssom_type TypeFromName(const string& f) {
      const string& s = AlienPartSeparator();
      size_t p = f.find(s);
      if (p==0 && f.substr(s.size()).find(s)!=string::npos)
        return tssom_alien_data;

      if (p!=string::npos)
        return p!=0 ? tssom_alien_map : tssom_undef;

      return tssom_normal;
    }

    /// Resolves tssom_type from TSSOM's name.
    tssom_type TypeFromName() const {
      return TypeFromName(Name());
    }

    /// Returns true if argument looks like alien feature name.
    static bool IsAlienFeature(const string& f) {
      tssom_type t = TypeFromName(f);
      return t==tssom_alien_data || t==tssom_alien_map;
    }

    /// Interprets key=value.
    virtual bool Interpret(const string&, const string&, int&);

    /// Returns the number of objects in the alien data base.
    int AlienDataBaseSize() const; // { return AlienDataBase()->Size(); }

    /// Returns the number of objects in the data base or the alien db.
    int DataSetSizeEvenAlien() const {
      switch (type) {
      case tssom_normal:     return DataSetSize();
      case tssom_alien_data: return AlienDataBaseSize();
      case tssom_alien_map:  return DataSetSize(); // ???
      default:
        return ShowError("TSSOM::DataSetSizeEvenAlien() : type unknown");
      }
    }

    /// Tries to solve feature_target by reading some file headers.
    virtual bool GuessFeatureTarget();

    /// Reads all files if force or not already read. If nodata, skip vectors.
    /// target_type is used by ReadMetaDataFile() if that is called.
    bool ReadFiles(bool force, bool nodat, bool dat,
                   bool cod, bool div, bool cnv, bool ord, bool nbr);

    /// Reads map file if force or not already read.
    bool ReadMapFile(bool force = false, bool nodata = false);

    /// True if map file really has to be read from disk.
    bool ReadMapFileReally(bool force, bool nodata) {
      return !HasTreeSOM() || force || (!nodata && !HasTreeSOMvectors());
    }

    /// Returns true if TreeSOM is not empty.
    bool HasTreeSOM() const { return treesom.Units(); }

    /// Returns true if TreeSOM vectors are not zero-sized.
    bool HasTreeSOMvectors() const {
      return HasTreeSOM() && treesom.Unit(0)->IsAllocated();
    }

    /// Returns true if TreeSOM vectors are not zero-sized.
    bool HasNonZeroTreeSOMvectors() const {
      return HasTreeSOMvectors() && !treesom.Unit(0)->IsZero();
    }

    /// Returns true if TreeSOM contains U-matrix.
    bool HasUmatrix() const { return treesom.HasRawUmatrix(); }

    /// Sets meandistance when reading map file (ReadMapFile()).
    bool SetMeanDist(const string&);
    
    /// Splits comma-separated argument and reads images in.
    bool SetBackGround(const string&);

    /// Access to background imagedata, to be deprecated later...
    const map<string,imagedata>& BackGround() const { return background; }
    
    /// Reads division file if force or not already read.
    /// As a side effect creates back reference vectors.
    bool ReadDivisionFile(bool force = false);

          /// Reads the new XML division file format    
    bool ReadDivisionFileXml(const string& divname);

    /// True if division file really has to be read from disk.
    bool ReadDivisionFileReally(bool force) {
      return force || !DivisionFileRead(); 
    }
    
    /// True if division file has been read.
    bool DivisionFileRead() const { return division_xx.Nitems(); }

    /// Writes division file back on disk after changes.
    bool ReWriteDivisionFile(bool cwd, bool zipped);
    
    ///
    bool WriteXmlDivisionFile(bool zipped);

    /// Reads convolution file if force or not already read.
    bool ReadConvolutionFile(bool force = false);
  
    /// True if convolutionfile really has to be read from disk.
    bool ReadConvolutionFileReally(bool force) {
      return force || convolutions_str.size()==0;
    }

    /// Reads convolution file if force or not already read.
    bool ReadOrdFile(bool force = false);

    /// Reads nearest neighbour file if force or not already read.
    bool ReadNbrFile(bool force = false);
  
    /// Solves label indices for map labels on all levels.
    bool FindLabelIndices(bool = false);
    
    /// Solves label indices for map labels on one level.
    bool FindLabelIndices(int, bool = false);
    
    /// Returns reference to the TS-SOM.
    const TreeSOM &Map(int l = 0) const { return treesom.Level(l); }

    /// Returns reference to the TS-SOM.
    TreeSOM &Map(int l = 0) { return treesom.Level(l); }

    /// Returns reference to the TS-SOM.
    const TreeSOM &MapEvenAlien(int l = 0) const { 
      static TreeSOM dummy;
      switch (type) {
      case tssom_normal:     return Map(l);
      case tssom_alien_data: return AlienDataTssom()->Map(l);
      case tssom_alien_map:  return AlienMapTssom()->Map(l); // ???
      default:
        ShowError("TSSOM::MapEvenAlien() : type unknown");
        return dummy;
      }
    }

    /// Number of levels in TreeSOM.
    size_t Nlevels() const {
      return treesom.Nlevels() && treesom.Level(0).Units() ?
        treesom.Nlevels() : 0; }

    /// Returns true of TreeSOM level exists.
    bool LevelOK(size_t l) const { return treesom.LevelOK(l); }

    /// Returns true of TreeSOM level exists.
    bool LevelOKEvenAlien(size_t l) const {
      switch (type) {
      case tssom_normal:     return LevelOK(l);
      case tssom_alien_data: return AlienDataTssom()->LevelOK(l);
      case tssom_alien_map:  return AlienMapTssom()->LevelOK(l);
      default:
        return ShowError("TSSOM::LevelOKEvenAlien() : type unknown");
      }
    }

    /// Number of levels in TreeSOM may use other maps.
    size_t NlevelsEvenAlien() const {
      switch (type) {
      case tssom_normal:     return Nlevels();
      case tssom_alien_data: return AlienDataTssom()->Nlevels();
      case tssom_alien_map:  return AlienMapTssom()->Nlevels();
      default:
        return ShowError("TSSOM::NlevelsEvenAlien() : type unknown");
      }
    }

    /// Utility to give native map similar to the one used by alien data.
    const TSSOM *AlienDataTssom() const;

    ///  Utility to give alien map used by native data.
    const TSSOM *AlienMapTssom() const;

    ///  Utility to give alien db used by native data or vice versa.
    DataBase *AlienDataBase() const {
      DataBase *d = Picsom()->FindDataBase(AlienDataBaseName());
      if (!d) {
        ShowError("TSSOM::AlienDataBase() : FindDataBase() failed");
        SimpleAbort();
      }
      return d;
    }

    /// Returns true if this feature can appear in the user interface.
    virtual bool IsSelectable() /*const*/ {
      return VectorIndex::IsSelectable() || GuessVectorLength();
    }

    /// Lenght of feature vectors.
    virtual size_t VectorLength() const {
      return treesom.VectorLength() ?
        treesom.VectorLength() : VectorIndex::VectorLength();
    }

    ///
    void SetMetric(FloatVector::DistanceType*);

    /// Another way to get the lenght of feature vectors.
    virtual size_t GuessVectorLength() /*const*/; 

    /// Returns true if all bmu_depths are equal to one.
    bool DivisionIndexOldStyle() const {
      if (Nlevels()!=bmu_depths.size())
        return false;
      for (size_t i=0; i<bmu_depths.size(); i++)
        if (bmu_depths[i]!=1)
          return false;
      return true;
    }

    /// Returns the number of available BMUs on given level.
    int DivisionDepth(int l) const {
      if (l<0 || l>=(int)bmu_depths.size()) {
        ShowError("DivisionDepth() failed name="+Name()+" l="+ToStr(l)+
                  " bmu_depths.size()="+ToStr(bmu_depths.size()));
        return 0;
      }
      return bmu_depths[l];
    }

    /// Mapper from division vector level and depth to index.
    int DivisionVectorIndex(int l, int d, bool fail_ok = false) const {
      if (d<0 || d>=DivisionDepth(l)) {
        if (!fail_ok) {
          stringstream tmp;
          tmp << Name() << " l=" << l << " d=" << d
              << " depth=" <<DivisionDepth(l);
          ShowError("DivisionVectorIndex() "+tmp.str()+" failed A");
        }
        return -1;
      }

      for (int k=0; k<l; k++)
        d += bmu_depths[k];
      if (division_xx.IndexOK(d))
        return d;

      ShowError("DivisionVectorIndex() failed B");
      return -1;
    }

    /// Returns reference to the d'th division vector of level l.
    IntVector& Division(int l, int d) { 
      static IntVector dummy;
//       if (IsBinaryFeature()) {
//         // WarnOnce("TSSOM::Division(int) called for a binary feature");
//         return dummy;
//       }
      int i = DivisionVectorIndex(l, d);
      if (!division_xx.IndexOK(i)) {
        // WarnOnce("TSSOM::Division(" + ToStr(i) + ") called but"
        //          " division.Nitems()=" + ToStr(division_xx.Nitems()));
        return dummy;
      }
      return division_xx[i];
    }

    /// Returns reference to the d'th division vector of level l.
    const IntVector& Division(int l, int d) const {
      int i = DivisionVectorIndex(l, d);
      return division_xx[i];
    }

    /// String form information on division vector lengths.
    string DivisionLengths_x() const {
      stringstream ss;
      for (int i=0; i<division_xx.Nitems(); i++)
        ss << (i?" ":"") << division_xx[i].Length();
      return ss.str();
    }

    /// Function that sets the contents of tt_stat.
    virtual void SetTargetCounts() {
      tt_stat.clear();
      if (division_xx.Nitems()) {
        const IntVector& d = *division_xx.Peek();
        for (int i=0; i<d.Length(); i++)
          if (d[i]>=0)
            AddOneTarget(ObjectsTargetType(i));
      }
    }

    /// Returns reference to the component ordered data set.
    const FloatVectorSet& Ord() const { return ord; }

    /// Returns reference to the back reference vector of i'th item
    /// on l'th level.
    const IntVector& BackReference(int l, int i) const {
      //static IntVector empty;
      const IntVector *emptyptr = NULL;
      const IntVector& empty = *emptyptr;
      if (!backreflev.IndexOK(l)) {
        ShowError("TSSOM::BackReference(l, i): l not OK"); return empty; }
      if (!backreflev[l].IndexOK(i)) {
        ShowError("TSSOM::BackReference(l, i): i not OK"); return empty; }
      return backreflev[l][i];
    }

    /// Returns reference to the back reference vectors on l'th level.
    const IntVectorSet& BackReference(int l) const {
      const IntVectorSet *emptyptr = NULL;
      const IntVectorSet& empty = *emptyptr;
      if (!backreflev.IndexOK(l)) {
        ShowError("TSSOM::BackReference(l): l not OK"); return empty; }
      return backreflev[l];
    }

    ///
    virtual void ReduceMemoryUse();

    /// Full-path filename of the convolution mask file.
    const string& ConvolutionFileName() const { return cnvfile_str; }

    /// Full-path filename of the component ordered data file.
    const string& OrdFileName() const { return ordfile_str; }

    /// Full-path filename of the nearest neighbour file.
    const string& NbrFileName() const { return nbrfile_str; }

    /// All info in one string. Delete after use!
    // const char *DisplayStringM(bool = true, bool = false, bool = false);

    /// Forms 1-dimensional convolution kernel.
    FloatVector CreateCVector(int level); 

    /// This will be obsoleted when CreateImage() is ready.
    void PGMize(ostream&, const simple::FloatMatrix&) const;

    /// Returns upper left and lower right coordinates of zoom area.
    void ZoomArea(int, const FloatPoint&, int, IntPoint&, IntPoint&) const;
    
    /// Returns center coordinates of zoom area.
    IntPoint ZoomCenter(int, const FloatPoint&) const;

    /// Creates mapunitinfo string.
    const char *CreateMapUnitInfo(const char*) const;

    /// Creates mapunitinfo string.
    const char *CreateMapUnitInfo(int, int, int) const;

    /// interprets "[1][2,3]" or "[1](0.1,0.2)".
    bool SolveLevelAndUnit(const char*, int* = NULL,
                           IntPoint* = NULL, FloatPoint* = NULL) const;

    ///
    static void CollapseAndArrangeRows(simple::IntMatrix&);

    /// Creates back reference lists for all levels
    bool CreateBackReferences();

    /// Creates back reference lists for all units of one level.
    bool CreateBackReference(int);

    /// Checks that backreferences are sane.
    bool BackReferenceSanityCheck(int) const;

    /// Checks that backreferences are sane.
    bool DivisionVectorSanityCheck(const IntVector&) const;

    /// Adds TSSOM object information to the XML document.
    bool AddToXML(XmlDom&, object_type, const string&, 
                  const string&, const Query*);

    /// Adds query-independent TS-SOM information to the XML document.
    virtual bool AddToXMLfeatureinfo_static(XmlDom&) const;

    /// Adds mapunit information to the XML document.
    bool AddToXMLmapunitinfo(XmlDom&, const string&, const string&);

    /// Adds collage of images on a tree level to the XML document
    bool AddToXMLmapcollage(XmlDom&, const string&);

    ///
    typedef struct {
      size_t bw;
      size_t bh;
      size_t iw;
      size_t ih;
      bool keepaspect;
    } map_tn_spec_t;

    /** Solves collagemap thumbnail specs from given string, if string is empty
        or erroneous, default values (bw,bh,iw,ih,keepaspect) are used. 
    */
    static map_tn_spec_t SolveMapTnSpec(const string& spec_str,
                                        size_t bw=50, size_t bh=50,
                                        size_t iw=46, size_t ih=46, 
                                        bool keepaspect=false);

    ///
    imagedata CreateMapCollage(size_t level, const map_tn_spec_t& spec, 
                               const imagedata& b, float c = 1.0,
                               size_t tlx = 0, size_t tly = 0,
                               size_t brx = 0, size_t bry = 0,
                               const string& s_pre = "",
                               const string& s_pos = "") const {
      return CreateMapCollage(level,spec.bw,spec.bh,spec.iw,spec.ih,b,c,
                              spec.keepaspect,tlx,tly,brx,bry,s_pre,s_pos);
    };

    ///
    imagedata CreateMapCollage(size_t level, size_t cw, size_t ch,
                               size_t iw, size_t ih, const imagedata& b,
                               float c=1.0,
                               bool keepaspect=false,
                               size_t tlx = 0, size_t tly = 0,
                               size_t brx = 0, size_t bry = 0,
                               const string& s_pre = "",
                               const string& s_pos = "") const;

    /// Returns mean distance between vectors in data.
    double MeanDistance() const { return meandistance; }

    /// Sets backrefsanitychecks that affects BackReferenceSanityCheck().
    static void BackRefSanityChecks(bool c) { backrefsanitychecks = c; }

    /// Sets features_command;
    virtual bool SolveFeatureCalculationCommand(bool, bool);

    ///
    virtual bool CalculateFeatures(size_t idx, set<string>& done_segm,
				   XmlDom& xml) {
      return VectorIndex::CalculateFeatures(idx, done_segm, xml);
    }

    ///
    virtual bool CalculateFeatures(vector<size_t>&, set<string>&,
				   XmlDom& xml, bin_data*);

    /// A short-cut.
    virtual string CalculatedFeaturePath(const string& label, bool/*isnew*/,
                                         bool, bool gzip=false) const;

    /// Changes feature calculation command to use uploaded segmentation image
    void HackSegmentationCmd(vector<string>&, const string&);

    /// Loads features from a file and updates div and backref data.
    bool LoadAndMatchFeatures(int, bool, bool);

    /// Maps vectors on SOM and updates div and backref data.
    bool MatchFeatures(const FloatVector&);

    /// Sets the division data for the object with given index
    bool SetDivision(int, IntVector&);

    /// This checks that data vectors' target_type intersects with target_type.
    bool CheckDataSetTargetTypes() const;

    int CheckDataSet(const ground_truth&, int, bool,
		     vector<size_t>&, vector<size_t>&, vector<size_t>&, 
                     vector<size_t>&, bool, bool);

    /// This removes vectors from data set for training under restriction.
    bool ApplyRestriction(const char *r, target_type t) { 
      return ApplyRestriction(r ? string(r) : "", t);
    }

    /// This removes vectors from data set for training under restriction.
    bool ApplyRestriction(const string&, target_type);

    /// This a general interface for classifier creation.
    bool CreateClassifier(const string&, const string&,
                          const ground_truth_set&);

    /// This _should_ create an SOM...
    bool CreateSOM(const string&, const string&,
                   const ground_truth_set&);

    /// This _should_ create a k-NN...
    bool CreateKNN(const string&, const string&,
                   const ground_truth_set&);

    /// This _should_ create an LSC...
    bool CreateLSC(const string&, const string&,
                   const ground_truth_set&);

    /// Used by CreateXXX().
    typedef list<cox::knn::lvector_t> cox_knn_set_t;

    ///  Used by CreateXXX().
    cox_knn_set_t CoxVectors(const ground_truth&, const string&);

    ///
    cox_knn_set_t CoxVectors(const ground_truth_set&);

    /// Checks if some files have changed and rereads them.
    virtual bool CheckModifiedFiles();
    
    /// Accees to information changed division member.
    bool DivisionChanged() const { return division_changed; }

    /// Returns true if SOM unit vector values should be displayed.
    bool ShowMapValues() const { return show_map_values; }

    /// Returns true if data vector values should be displayed.
    bool ShowDataValues() const { return show_data_values; }

    ///
    const vector<simple::VectorComponent>& ComponentDescriptions() const {
      return treesom.ComponentDescriptions();
    }

    ///
    int GetComponentIndex(const string& name) const {
      return treesom.GetComponentIndex(name);
      // return Data().GetComponentIndex(name);
    }

    ///
    typedef cox::labeled_float_vector labeled_float_vector;

    ///
    typedef struct {
      TSSOM *tssom;
      size_t nobjects;
      size_t nsamples;
      size_t ntotal;
      size_t index;
      size_t number;
      size_t nfiles;
      vector<size_t> objects;
      list<labeled_float_vector>   input;
      vector<labeled_float_vector> vec;
      vector<float> mean;
      vector<float> stdev;
      simple::RandVar randvar;
    } raw_data_spec_t;

    /// Callback function for a DataSet object needing raw data.
    static void *RawDataSourceCallBack(void*);

    /// Callback function for a DataSet object needing raw data.
    FloatVector *RawDataSource(raw_data_spec_t&);

    /// Sets things ready for the above method and return vector length.
    int PrepareRawData(raw_data_spec_t&, bool);

    /// Subroutine for the above ones.
    labeled_float_vector NextFeatureVector(raw_data_spec_t&);

    /// Subroutine for the above ones.
    list<labeled_float_vector> NextFeatureFile(raw_data_spec_t&);

    /** Returns true if weights are given in div-file, i.e. must be checked
        per object.
    */
    bool DivMulWeightsGiven() { return divmul_weights.size(); }

    ///
    const vector<float>& DivMulWeights(int l, int ind) const {
      return divmul_weights[ind][l]; 
    }
    
    bool HasDiv() { return !divfile.Name().empty(); }
    
    void BmuDepths(const IntVector& v);

    void InitializeDivision(int ll=-1);
    
  protected:
    /// Type of the structure.
    tssom_type type;

    /// Full path to map file.
    string codfile_str;

    /// The division file might be externally modified and reread.
    ModifiableFile divfile;

    /// Full path to convolutions file.
    string cnvfile_str;

    /// Full path to component ordered data file.
    string ordfile_str;

    /// Full path to nearest neighbour file.
    string nbrfile_str;

    /// This describes the convolution masks.
    string convolutions_str; 

    /// Mean distance between vectors in data.
    double meandistance;

    /// The TS-SOM is stored here.
    TreeSOM treesom;

    /// The per TS-SOM level BMU depths in the division file.
    vector<int> bmu_depths;

    /// The division info is stored here.
    IntVectorSet division_xx;
    
    /// Set true if division changed after read from disk.
    bool division_changed;

    /// The ord file is dtored here.
    FloatVectorSet ord;

    /// 
    vector<vector<vector<float> > > divmul_weights;

  public: // quick hack, ought to be implemented through access methods
    /// .nbr file contents are stored here
    vector<vector<vector<int> > > nearest_neighbour;

  protected:
    /// Back references are solved and stored here.
    ListOf<IntVectorSet> backreflev;

    /// True if backrefs should be sanity checked.
    static bool backrefsanitychecks;

    /// Background images by their filename.
    map<string,imagedata> background;

    /// Set true if SOM unit vectors should be shown in HTML.
    bool show_map_values;

    /// Set true if data vectors should be shown in HTML.
    bool show_data_values;

  public:
    ///
    class State : public VectorIndex::State {
    public:
      ///
      State(Index *t, const string& n = "") : VectorIndex::State(t, n) {
        matrixcount = 0;
      }

      ///
      virtual State *MakeCopy(bool full) const { 
        State *s = new State(*this);
        if (!full)
          s->clear();

        return s;
      }

      ///
      virtual float ScoreValue(int idx) const {
        int bot = TsSom().Nlevels()-1;
        const IntVector& div = TsSom().Division(bot, 0);
        int mapidx = div.IndexOK(idx) ? div[idx] : -1;
        IntPoint p = TsSom().Map(bot).ToPoint(mapidx);
        return GetConvolved(bot, 0)->Get(p.Y(), p.X());
      }

      ///
      const TSSOM& TsSom() const { return *(TSSOM*)StaticPart(); }

      /// deprecated...
      TSSOM& TsSom() { return *(TSSOM*)StaticPart(); }

      ///
      void clear() {
        vqulist.clear();
        objlist.clear();
        hit.clear();
        cnv.clear();
        matrixcount = 0;
      }

      ///
      bool Initialize(bool keep, size_t mcount, bool do_ent);

      ///
      size_t MatrixCount() const { return matrixcount; }

      ///
      simple::FloatMatrix *GetHits(size_t l, size_t k) {
        return l<TsSom().Nlevels() && k<MatrixCount() ?
          &hit[l*MatrixCount()+k] : NULL;
      }

      ///
      const simple::FloatMatrix *GetHits(size_t l, size_t k) const {
        return ((TSSOM::State*)this)->GetHits(l, k);
      }

      ///
      simple::FloatMatrix *GetConvolved(size_t l, size_t k) {
        return l<TsSom().Nlevels() && k<MatrixCount() ?
          &cnv[l*MatrixCount()+k] : NULL;
      }

      ///
      const simple::FloatMatrix *GetConvolved(size_t l, size_t k) const {
        return ((TSSOM::State*)this)->GetConvolved(l, k);
      }

    public:
      ///
      vector<float> entropy;

      ///
      vector<float> entropy_r;

      ///
      vector<float> entropy_p;

      ///
      vector<float> entropy_n;

      ///
      size_t matrixcount;

      /// 
      vector<VQUnitList>  vqulist;

      /// 
      vector<simple::FloatMatrix> hit;

      /// 
      vector<simple::FloatMatrix> cnv;

      ///
      vector<float> tssom_level_weight;

    }; // class TSSOM::State

  };  // class TSSOM

} // namespace picsom

#endif // _PICSOM_TSSOM_H_

