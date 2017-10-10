// -*- C++ -*-  $Id: VectorIndex.h,v 2.50 2017/05/09 10:19:50 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_VECTORINDEX_H_
#define _PICSOM_VECTORINDEX_H_

#include <Index.h>
#include <bin_data.h>

#include <cox/knn>
#include <cox/lsc>

#include <fstream>

namespace picsom {
  /**
     Abstract base class for TSSOM etc. objects.
  */
  class VectorIndex : public Index {
    //  protected:
  public:
    /**
       The constructor for the VectorIndex class.
       @param db     the database where features have been calculated
       @param name   canonical name of feature set
       @param path   full path to directory containing the files
    */
    VectorIndex(DataBase *db, const string& name, const string& feat,
		const string& path, const string& params, const Index* src);

    ///
    VectorIndex(bool) : Index(false) {}

    ///
    VectorIndex() { abort(); }

    ///
    VectorIndex(const VectorIndex& i) : Index(i) { abort(); }

    ///
    VectorIndex(const Index&) { abort(); }

    ///
    virtual ~VectorIndex();
    
  public:

    ///
    virtual const string& MethodName() const { 
      static string vectorindex = "vectorindex";
      return vectorindex;
    }

    ///
    virtual Index* Make(DataBase *db, const string& name, const string& feat,
			const string& path,
			const string& params, const Index *src) const {
      return new VectorIndex(db, name, feat, path, params, src);
    }

    ///
    virtual bool IsMakeable(DataBase* /*db*/, const string& /*fn*/,
                            const string& /*dirstr*/,
			    bool /*create*/=false) const {
      return false;
    }

    /// Reads data file if force or not already read.
    bool ReadDataFile(bool force = false, bool nodata = false,
                      bool rwlocked = false);

    /// True if data file really has to be read from disk.
    bool ReadDataFileReally(bool force, bool nodata) {
      return !HasData() || force || (!nodata && !HasDataVectors());
    }

    ///
    bool ReadDataFileClassical(bool);

    ///
    bool ReadDataFileSql(bool);

    ///
    bool ReadDataFileBinOld(bool);

    ///
    bool ReadDataFileBin(bool);

    ///
    bool BinDataFullTest(size_t i) const {
      return _bin_data_m.begin()->second.fulltest(i); 
    }

    ///
    bool BinDataExists(size_t i) const {
      return _bin_data_m.begin()->second.exists(i); 
    }

    ///
    bool BinDataErase(size_t i) const {
      return _bin_data_m.begin()->second.erase(i); 
    }

    ///
    void BinDataEraseAll() const {
      if (_bin_data_m.size())
	_bin_data_m.begin()->second.erase_all();
    }

    ///
    FloatVector *BinDataFloatVector(size_t);

    ///
    static void BinDataFullTest(bool v) { bin_data_full_test = v; }

    ///
    static bool BinDataFullTest() { return bin_data_full_test; }

    /// Returns true if data set is not empty.
    size_t NdataVectors() const { return data.Nitems(); }

    /// Returns true if data set is not empty.
    bool HasData() const { return NdataVectors(); }

    /// Returns true if data set vectors are not zero-sized.
    bool HasDataVectors() const {
      return HasData() && data[0].IsAllocated();
    }

    /// Appends one data vector.
    FloatVector *DataAppendCopy(const FloatVector& v, bool set_number) {
      FloatVector *r = data.AppendCopy(v);
      if (set_number && data_numbers_set)
	r->Number(data.Nitems()-1);
      return r;
    }

    /// Returns vector dimensionality, zero if not known.
    virtual size_t VectorLength() const {
      return (size_t) data.VectorLength();
    }

     /// Returns vector dimensionality, zero if not known.
    virtual size_t GuessVectorLength();

    /// This is now done only when really needed...
    void SolveDataFileNames(bool force=false);

    /// This is called by SolveDataFileNames().
    list<string> ExpandIncludes(const string&);

    /// The number of data files.
    size_t DataFileCount(bool heavy = true) /*const*/;

    /// Full-path filename of the data file.
    const string& DataFileName(size_t i) /*const*/ { 
      if (i<DataFileCount()) {
        int j = i;
        for (list<string>::const_iterator p=datfilelist.begin();
             p!=datfilelist.end(); p++, j--)
          if (!j)
            return *p;
      }

      ShowError("TSSOM::DataFileName(int) erroneous argument ",
                      ToStr(i));

      static const string sink = "/dev/null";
      return sink;
    }

    /// Prepends a file name in data file list
    bool AddDataFileName(const string& n);

    /// Returns true if given name exists in data file list
    bool HasDataFileName(const string& n);

    /// Access to full datfilelist.
    const list<string>& DataFileNameList() const { return datfilelist; }

    /// Feature augmentation alternatives.
    vector<string> SolveFeatureAugmentation(const string&, const string&);

    /// Table name and column prefix of SQL data.
    pair<string,string> SqlNames() const;

    /// Access to data stored in SQL.
    int SqlNdataVectors();

    /// Access to data stored in SQL.
    // FloatVectorSet SqlDataVectors(const string&);

    /// Access to data stored in SQL.
    FloatVector SqlDataVector(int);

    /// Checks for <target> in data's description.
    static target_type SolveDataTarget(const FloatVectorSet&);

    /// Checks for <target> in data's description.
    target_type SolveDataTarget() const { return SolveDataTarget(Data()); }

    /// Returns reference to the data set.
    const FloatVectorSet& Data() const { return data; }

    /// Returns reference to the data set.
    FloatVectorSet& Data() { return data; }

    /// Returns pointer to a particular data vector by DataBase wide index.
    const FloatVector *DataVector(int idx) const {
      return data.GetByNumber(idx);
    }

    /// Returns data (feature) vectors pointed by indices.
    FloatVectorSet DataByIndices(IntVector&);

    /// Returns data (feature) vectors pointed by indices.
    FloatVectorSet DataByIndices(const vector<size_t>&, bool);

    /// Returns data (feature) vectors pointed by indices.
    FloatVectorSet DataByIndicesAugmented(const vector<size_t>&, const string&,
					  bool);

    /// Returns data (feature) vectors pointed by indices.
    FloatVectorSet DataByIndicesClassical(const vector<size_t>&);

    /// Returns data (feature) vectors pointed by indices.
    FloatVectorSet DataByIndicesSql(const vector<size_t>&);

    /// Returns data (feature) vectors pointed by indices.
    FloatVectorSet DataByIndicesBin(const vector<size_t>&, const string&, bool);

    /// Returns data (feature) vectors pointed by indices.
    FloatVectorSet DataByIndicesBinOld(const vector<size_t>&);

    ///
    bool BinDataOpen(bool, size_t, bool, const string&);

    ///
    bool BinDataOpenVirtual(const string&);

    ///
    bool BinDataOpenVirtualDetections(const string&);

    ///
    bool BinDataOpenVirtualTranspose(const string&);

    ///
    bool BinDataOpenVirtualConcatenate(const string&);

    ///
    bool BinDataOpenFile(bool, bool, const string&);

    ///
    bool BinDataExpand(size_t);

    ///
    bool BinDataFlush() { return _bin_data_m.begin()->second.flush(); }

    ///
    bool BinDataUnmap();

    /// Stores a feature vector for insertion in binary data file.
    bool BinDataStoreFeature(const FloatVector&);

    /// Stores a feature vector for insertion in binary data file.
    bool BinDataStoreFeature(const cox::labeled_float_vector&);

    /// Stores a feature vector for insertion in binary data file.
    bool BinDataStoreFeature(size_t, const vector<float>&);

    ///
    size_t OpenBinDataOld(const string&, string&, ifstream&);

    ///
    bool OpenBinData();

    ///
    bool BinInfoSet() const { return BinInfoVectorLength(); }

    ///
    const string& BinInfoFileName() const {
      static const string empty;
      return _bin_data_m.size() ?
	_bin_data_m.begin()->second.filename() : empty;
    }

    ///
    size_t BinInfoVectorLength() const {
      return _bin_data_m.begin()->second.is_ok() ?
	_bin_data_m.begin()->second.get_header_copy().vdim : 0;
    }

    ///
    size_t BinInfoRawDataLength(const string& augm) /*const*/ {
      string key = augm==FeatureFileName() ? "" : augm;
      return _bin_data_m[key].rawsize();
    }

    ///
    size_t BinInfoRawDataLength(size_t n, const string& augm) /*const*/ {
      string key = augm==FeatureFileName() ? "" : augm;
      return _bin_data_m[key].rawsize(n);
    }

    /// This is needed to reset numbers in data vectors.
    bool SetDataSetNumbers(bool force, bool may_add);

    ///
    const FloatVector* GetDataFromFile(size_t, FloatVectorSet*&, string&,
                                       bool = false);


    /// Adds query-independent VectorIndex information to the XML document.
    virtual bool AddToXMLfeatureinfo_static(XmlDom&) const;

    /// Calculates self dot product of each vector in data set.
    void CalculateDataBaseSelfDots() { data.CalculateSelfDots(); }

    /// Dumps data in Sql format.
    bool SqlDumpData(ostream&, bool, const string&);
    
    /// Forms a string spec for SQL table creation.
    /// type==0 : let the routine decide columns/blob
    /// type==1 : force columns
    /// type==2 : force blob
    string SqlTableSchema(size_t, string&, size_t type = 0) const;

    /// Inserts a row in the features_XXX table.
    bool SqlInsertFeatureData(const cox::labeled_float_vector&);

    /// true if fast_bin_check=true
    static bool FastBinCheck() { return fast_bin_check; }

    ///
    static void FastBinCheck(bool s) { fast_bin_check = s; }

    /// true if fast_bin_check=true
    static bool NanInfCheck() { return nan_inf_check; }

    ///
    static void NanInfCheck(bool s) { nan_inf_check = s; }

    ///
    class classifier_data {
    public:
      ///
      classifier_data() {}

      ///
      classifier_data(const string& t, const string& n) :
        type(t), name(n) {}

      ///
      ~classifier_data();

      ///
      string classify(const cox::knn::lvector_t&);

      ///
      vector<pair<string,double> > class_distances(const cox::knn::lvector_t&);

      ///
      string type;

      ///
      string name;

      ///
      cox::knn knn;

      ///
      cox::lsc lsc;
      
    }; // class classifier_data

    ///
    classifier_data *AddClassifier(const string&, const string&, bool = true);

    ///
    classifier_data *FindClassifier(const string&);

    ///
    bool DestroyClassifier(classifier_data*, bool = false);
    
    ///
    bool DestroyClassifiers();

    virtual bool IsSelectable() { return is_selectable; }

  protected:
    /// Vectorial data is stored here.
    FloatVectorSet data;

    /// Full path to data file.
    list<string> datfilelist;

    /// Set to true when SolveDataFileNames() has been run
    bool has_solved_datafile_names;

    ///
    bool data_numbers_set;

    ///
    map<string,bin_data> _bin_data_m;

    ///
    static bool bin_data_full_test, fast_bin_check, nan_inf_check;

    /// knn, lsc etc. stuff
    typedef map<string,classifier_data> classifier_map_t;

    /// knn, lsc etc. stuff
    classifier_map_t classifier_map;

    ///
    bool is_selectable;

  public:
    ///
    class State : public Index::State {
    public:
      ///
      State(Index *t, const string& n = "") : Index::State(t, n) {}

      ///
      const VectorIndex& Vectorindex() const {
        return *(VectorIndex*)StaticPart();
      }

      ///
      VectorIndex& Vectorindex() { return *(VectorIndex*)StaticPart(); }

      ///
      virtual State *MakeCopy(bool) const {
	State *s = new State(*this);
	return s;
      }

      ///
      virtual float ScoreValue(int) const { return 0; }

      ///
      const FloatVectorSet& Data() const { return Vectorindex().Data(); }

      ///
      FloatVectorSet& Data() { return Vectorindex().Data(); }

      ///
      void AddSample(const FloatVector& v) { sample.AppendCopy(v); }

      ///
      FloatVectorSet sample;

    };  // class VectorIndex::State

    virtual Index::State *CreateInstance(const string& n) {
      return new State(this,n);
    }

  protected:

  };  // class VectorIndex

} // namespace picsom

#endif // _PICSOM_VECTORINDEX_H_

