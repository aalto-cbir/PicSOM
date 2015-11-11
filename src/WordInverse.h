// -*- C++ -*-  $Id: WordInverse.h,v 2.15 2011/12/14 13:41:18 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 
// -*- C++ -*-	$Id: WordInverse.h,v 2.15 2011/12/14 13:41:18 jorma Exp $

#ifndef _PICSOM_WORDINVERSE_H_
#define _PICSOM_WORDINVERSE_H_

#include <InvertedIndex.h>

namespace picsom {
  /**
     Inverse index for words aka inverted file
  */
  class WordInverse : public InvertedIndex {
  public:
    /**
       The constructor for the WordInverse class.
       @param db     the database where features have been calculated
       @param name   canonical name of feature set
       @param path   full path to directory containing the files
    */
    WordInverse(DataBase *db, const string& name, const string& feat,
		const string& path, const string& params, const Index* src);

    ///
    WordInverse(bool) : InvertedIndex(false) {}

    ///
    WordInverse() { abort(); }

    ///
    WordInverse(const Index&) { abort(); }

    ///
    virtual ~WordInverse();

    class State;
    class BinaryFeaturesData;

    ///
    virtual const string& MethodName() const {
      static string wordinverse = "wordinverse";
      return wordinverse;
    }

    ///
    virtual Index* Make(DataBase *db, const string& name, const string& feat,
			const string& path, const string& params,
			const Index *src) const {
      return new WordInverse(db, name, feat, path, params, src);
    }

    ///
    virtual bool IsMakeable(DataBase* /*db*/, const string& fn, 
			    const string& dirstr, bool) const {
      string fname = dirstr+"/"+fn, metn = fname+".met";
      return IsBinaryFeatureName(fn) || IsBinaryFeatureFile(metn);
    }

    ///
    virtual bool ReadDataFile(bool force, bool nodata, bool wrlocked);

    /// Returns true if string names a binary feature.
    static bool IsBinaryFeatureName(const string& fn) {
      string f, a;
      return SplitParentheses(fn, f, a) && f=="binary";
    }

    /// Returns true if string names a file containing a binary feature.
    static bool IsBinaryFeatureFile(const string& fn) {
      if (fn=="")
	return false;

      ifstream ifs(fn.c_str());
      string line;
      getline(ifs, line);
      return line.find("# WORDINVERSEFILE")==0||line.find("# INVERSEFILE")==0;
    }

    ///
    virtual Index::State *CreateInstance(const string& n) {
      return new State(this,n);
    }

    /// Returns true if this feature can appear in the user interface.
    virtual bool IsSelectable() /*const*/ {
      return true;//GuessVectorLength() || IsBinaryFeature();
    }

    /// Sets debug_binary that affects *Binary*().
    static void DebugBinary(bool c) { debug_binary = c; }

    /// Returns debug_binary that affects *Binary*() also in Query.
    static bool DebugBinary() { return debug_binary; }

    bool IsBinaryFeature_deprecated() const {
      return static_binary_feature_data.ClassCountXXX() ||
	binary_streampos.size();
      // return true;
    }

    /// The "old" binary features return true, "textquery" features false.
    bool IsLatentBinaryFeature() const {
      return IsBinaryFeature_deprecated() && !IsDynamicBinaryTextFeature();
    }

    /// Returns true if is a dynamic binary feature suitable for text queries.
    bool IsDynamicBinaryTextFeature() const {
      return binary_streampos.size();
    }

    /// Called when new querytext has been set.
    bool UpdateDynamicBinaryTextFeature(const string&, BinaryFeaturesData&);

    /// Calls TextBase::WordPreProcess().
    string TextBasedWordPreProcess(const string&) const;

    /// Returns true if memory-consuming but fast indices are desired.
    bool UseBinaryClassIndices() const { return binary_classindices; }

    /// Returns true if memory-consuming but fast indices are desired.
    bool UseBinaryScaling() const { return binary_scaling; }

    /// Builds binary_features and binary_features_inverse.
    bool BuildBinary();

    ///
    bool CheckBinaryParams(const string& params);

    /// Builds binary_features and binary_features_inverse.
    bool BuildBinaryFromFile(const string& params="");

    ///
    bool ReopenBinaryFile(string filename) {
      binary_file.close();
      binary_file.clear();
      binary_file.open(filename.c_str());
      return binary_file.good();
    }

    /// Set number of lines to read from binary inverse file
    void BinaryInverseFileN(const size_t v) { binary_inversefile_n=v; }
    
    ///
    BinaryFeaturesData& StaticBinaryFeatureData() {
      return static_binary_feature_data;
    }

  public:
    class BinaryFeaturesData {
    public:
      void clear() {
	binary_features.clear();
	binary_features_inverse.clear();
      }

      ground_truth& last() { return binary_features_inverse.back(); }

      void add(ground_truth gt) { binary_features_inverse.push_back(gt); }

      /// Returns the total number of binary classes in use.
      int ClassCountXXX() const { return binary_features_inverse.size(); }
	
      /// Returns the name (label) of ith binary class.
      const string& ClassName(int i) const {
	return binary_features_inverse[i].Label();
      }
      
      /// Returns the binary classes in which the ith object belongs to.
      const vector<int>& ClassIndices(size_t i) const {
	if (i<binary_features.size())
	  return binary_features[i];
	
	ShowError("BinaryFeaturesData::ClassIndices()!!!");
	
	static const vector<int> empty;
	return empty;
      }
      
      /// 
      bool CreateClassIndices(int);

      /// Returns an ground_truth of objects belonging to ith binary class.
      const ground_truth& GroundTruth(size_t i) const {
	return binary_features_inverse[i];
      }

      /// Returns index of named binary class, -1 if not found
      int ClassIndex(const string& name) const {
	for (size_t i=0; i<binary_features_inverse.size(); i++)
	  if (binary_features_inverse[i].Label() == name) return i;
	return -1;
      }

    protected:
      /// Binary class indices for objects.
      vector<vector<int> > binary_features;
      
      /// Pointers to ternary membership vectors with class names as labels.
      vector<ground_truth> binary_features_inverse;

    }; // WordInverse::BinaryFeaturesData

  protected:

    /// True if binary features be traced.
    static bool debug_binary;

    /// Open stream for reading per-word object sets.
    ifstream binary_file;

    /// Map from words to streampos' in binary_file.
    map<string,streampos> binary_streampos;

    /// True if BinaryClassIndices() should work.
//     bool binary_classindices;

    /// Equals "porter" if TextBased::PreProcess() is used for stemming.
    string binary_stem;

    /// True if binary class weights are to be scaled on 
    bool binary_scaling;
    
    ///
    size_t binary_inversefile_n;
    
    ///
    string binary_metaclassfile;

    ///
    BinaryFeaturesData static_binary_feature_data;

  public:
    ///
    class State : public InvertedIndex::State {
    public:
      ///
      State(Index *t, const string& n = "") : InvertedIndex::State(t, n) {}
      
      ///
      virtual ~State() {}
      
      ///
      virtual State *MakeCopy(bool full) const {
	State *s = new State(*this);
 	if (!full)
 	  s->clear();
	return s;
      }

      ///
      virtual float ScoreValue(int) const { return 0; }

      ///
      bool Initialize();

      ///
      const WordInverse& wordInverse() const { 
	return *(WordInverse*)StaticPart(); 
      }

      /// deprecated...
      WordInverse& wordInverse() { return *(WordInverse*)StaticPart(); }

      ///
      void clear() {
	objlist.clear();
      }

      ///
      vector<float> bin_feat;

//       /// fixme: just one list should be enough...
//       vector<ObjectList> objlist;

      ///
      WordInverse::BinaryFeaturesData binary_data;

    };  // class WordInverse::State

  };  // class WordInverse

} // namespace picsom

#endif // _PICSOM_WORDINVERSE_H_

// Local Variables:
// mode: lazy-lock
// End:

