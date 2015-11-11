// -*- C++ -*-  $Id: CbirPicsom.h,v 2.30 2009/12/09 11:55:41 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _CBIRPICSOM_H_
#define _CBIRPICSOM_H_

#include <CbirStageBased.h>

using namespace std;

namespace picsom {
  ///
  static const string CbirPicsom_h_vcid =
    "@(#)$Id: CbirPicsom.h,v 2.30 2009/12/09 11:55:41 jorma Exp $";

  typedef map<pair<string,string>,float> classaugment_t;
  typedef map<pair<string,string>,float>::iterator classaugment_iter_t;

  /**
     An picsom of CBIR algorithm implementation derived from the abstract
     CbirAlgorithm class.  Copy CbirPicsom.h to CbirYourMethod.h and 
     CbirPicsom.C to CbirYourMethod.C and edit them as you wish.
  */
  class CbirPicsom : public CbirStageBased {

  public:
    /**

    */
    class QueryData : public CbirStageBased::QueryData {
      ///
      friend class CbirPicsom;

    public:
      /// poison
      QueryData() { abort(); }

      ///
      QueryData(const CbirAlgorithm *a, const Query *q) :
	CbirStageBased::QueryData(a, q), seed(123),
	_sa_limit_per_map_units(0), _sa_limit_total_units(0),
	_sa_limit_per_map_images(0), _sa_permapnewobjects(0),
	bordercompensation(true), negative_weight(1.0), matrixcount_xx(1), 
	calculate_entropy_force(false), entropymatrix(-1),
        limitrelevance(false) {}

      ///
      virtual CbirAlgorithm::QueryData *Copy(const Query *q) const {
	QueryData *copy = new QueryData(*this);
	copy->SetQuery(q);
	copy->CopyIndexData();

	return copy;
      }

      /// Dumps seenobject.
      void DumpSeen(bool n = false) const { DumpList(seenobject, "SEEN", n); }
            
      /// Dumps combinedimage.
      void DumpCombined() const { DumpList(combinedimage, "COMBINED"); }
      
      /// Dumps uniqueimage.
      void DumpUnique(bool n = false) const { DumpList(uniqueimage,"UNIQUE",n);}
      
      /// Dumps newobject.
      void DumpNew(bool n = false) const { DumpList(newobject, "NEW", n); }
      
      /// Dumps permapnewobject.
      void DumpNew(int m, int l, bool n = false) const {
	DumpList(PerMapNewObject(m, l), "NEW", n, IndexFullName(m), m, l); }

      /// Dumps vqunit list
      void DumpVQUnits(int m, int l, bool n = false) const {
	if (IsTsSomAndLevelOK(m, l))
	  DumpList(PerMapVQUnit(m, l), "VQUNITS", n, IndexFullName(m), m, l,
		   MapEvenAlien(m,l).Width());
	else
	  cout << "VQUNITS (" << m << "," << l << ") not available" << endl;
      }

      /// Shows selected vqunit/image lists.
      void ShowUnitAndObjectLists(bool = true,  bool = true, bool = false,
                                  bool = true,  bool = true, bool = true,
                                  bool = false, bool = true, bool = false) const;
      
      ///
      simple::FloatMatrix& Hits(size_t i, size_t j, size_t k) {
	return *IndexDataTSSOM(i).GetHits(j, k);
      }

      ///
      const simple::FloatMatrix& Hits(size_t i, size_t j, size_t k) const {
	return ((CbirPicsom::QueryData*)this)->Hits(i, j, k);
      }

      ///
      simple::FloatMatrix& Convolved(size_t i, size_t j, size_t k) {
	return *IndexDataTSSOM(i).GetConvolved(j, k);
      }

      ///
      const simple::FloatMatrix& Convolved(size_t i, size_t j, size_t k) const {
	return ((CbirPicsom::QueryData*)this)->Convolved(i, j, k);
      }

      ///
      const simple::FloatMatrix& Convolved(const string& n, size_t j, size_t k) const {
	int i = IndexShortNameIndex(n);
	if (i<0)
	  ShowError("CbirPicsom::QueryData::Convolved() : IndexShortNameIndex("+
		    n+") failed");
	return Convolved(i, j, k);
      }

      /// Access to TSSOM::State::vqulist.
      VQUnitList& PerMapVQUnit(int i, int j) {
	return IndexDataTSSOM(i).vqulist[j];
      }

      /// const-const version of the one above.
      const VQUnitList& PerMapVQUnit(int i, int j) const {
	return ((QueryData*)this)->PerMapVQUnit(i, j);
      }

      /// Sets convtype.
      void ConvType(int m, const string& t) {
	if (m==-1)
	  _sa_default_convtype = t;
	
	else
	  _sa_tssom_convtype[m] = t;
      }
      
      /// Returns convtype.
      const string& ConvType(int m) const { 
	if (m==-1)
	  return _sa_default_convtype;
	
	map<int,string>::const_iterator i = _sa_tssom_convtype.find(m);
	return i!=_sa_tssom_convtype.end() ? i->second : _sa_default_convtype;
      }

      /// Sets weight.
      void Weight(int m, float v) {
	//      wordInverse(m).BinaryWeight(v);
	IndexData(m).Weight(v);
      }

      /// Returns weight.
      float Weight(int m) const { 
	//      return IsInverseIndex(m)?wordInverse(m).BinaryWeight():1.0;
	return IndexData(m).Weight();
      }

      ///
      virtual bool RemoveDuplicates();

      /// A simple TSSOM weighting implementation. 
      bool ValueOrNaN(float, bool);
      
      /// A simple TSSOM weighting implementation. 
      bool TsSomWeightValue(bool);

      /// 
      bool TsSomWeightValueAndDistances(bool);
      
      /// A simple TSSOM weighting implementation. 
      bool EntropyNpolynomial(const string&);
      
      /// A simple TSSOM weighting implementation. 
      bool EntropyNleaveOutWorst(const string&);

      ///
      void SetMapWeightsDebugInfo();

      ///
      IntVector& Division(int i, int j, int d = 0) {
	return TsSom(i).Division(j, d);
      }

      ///
      const IntVector& Division(int i, int j, int d = 0) const {
	return TsSom(i).Division(j, d);
      }

      /// Access to value of matrixcount.
      void SetMatrixCount(size_t c) { matrixcount_xx = c; }
      
      /// Access to value of matrixcount.
      size_t GetMatrixCount() const { return matrixcount_xx; }

      /// Access to value of matrixcount.
      size_t GetMatrixCount(size_t i) const {
	return IsTsSom(i) ? IndexDataTSSOM(i).MatrixCount() :0;
      }

      /// Marks objects from seenobjects to all ts-soms and levels.
      bool PlaceSeenOnMap(bool norm=true,bool limit=false) {
	for (size_t m=0; m<Nindices(); m++)
	  for (int l=0; l<Nlevels(m); l++) 
	    PlaceSeenOnMap(m, l, norm, limit);
	return true;
      }

      /// Marks objects from seenobjects to map of given ts-som and level.
      bool PlaceSeenOnMap(int, int, bool=true,bool=false);
      
      /// Gives the weighting vector for BMU div levels.
      const vector<float>& DivMultiplierVector(int, int);
      
      /// Gives the weighting vector for BMU div levels.
      const vector<float>& DivHistoryVector(int, int);
      
      ///
      bool BernoulliMAP(int, int);

      /// Marks objects from seenobjects to map of given ts-som and level.
      bool PlaceSeenOnMapOneMap(int, int, bool=true);
            
      /// Marks objects from seenobjects to map of given ts-som and level.
      bool PlaceSeenOnMapThreeMaps(int, int, bool=true,bool=false);
      
      /// Marks objects from seenobjects to map of given ts-som and level.
      bool PlaceSeenOnMapOneMapSmoothedFraction(int, int);

      /// A helper routine for the three above.
      bool CountPositivesAndNegatives(const IntVector&, int&, int&, bool,
				      bool=true) const;
      
      /// A helper routine for the three above.
      bool SumPositivesAndNegatives(const IntVector&, const Index::State&,
				    double&, double&, bool) const;
      
      /// A helper utlity for the PlaceSeenOnMap*() methods.
      void SetSizeOrZero(simple::FloatMatrix& hts, const TreeSOM& ts) {
	hts.SizeOrZero(ts.Height(), ts.Width());
	if (!hts.Size())
	  ShowError("Matrix size set to 0x0");
      }
      
      /// Returns true if Index::State aspects intersect with Object aspects.
      bool AspectsMatch(const Index::State&, const Object&,
			double&, double&) const;

      /// Returns true if aspects are used.
      bool UsingAspects() const;
      
      /// Returns comma-seprated list of aspects for given feature.
      string IndexAspects(size_t i) const {
	list<string> s;
	for (vector<string>::const_iterator j=IndexData(i).aspects.begin();
	     j!=IndexData(i).aspects.end(); j++)
	  s.push_back("\""+*j+"\"");
	return CommaJoin(s);
      }

      /// 
      virtual void SetClassAugmentValue(const float value,
					const string& classname="", 
					const string& mapname="") {
	if (debug_classify)
	  cout << "Running CbirPicsom::QueryData::SetClassAugmentValue() : "
	       << classname << (mapname.empty()?"":",") << mapname << ") = "
	       << value << endl;
	classaugment[pair<string,string>(classname,mapname)] = value;
      }

      /// Returns a convolution mask vector.
      const FloatVector *ConvolutionVector(const string&, int);
      
      /// Creates a brand new convolution mask vector.
      static FloatVector *CreateConvolutionVector(const string&, double, double);
      
      /// Searches for ConvolutionVector name and pointer.
      static bool ConvolutionVectorNameAndFunc(const string&, string&,
					       //double (*&)(double,double,double)
					       ConvolutionFunction&);
      
      /// Splits and joins ConvolutionVector name parts.
      static string ConvolutionVectorName(const string&, int,
					  string&, float&, float&);
      
      /// Splits ConvolutionVector name parts.
      static ConvolutionFunction ConvolutionVectorFunc(const string&, int,
						       float&, float&);
      
      /// Extracts ConvolutionVector parameters.
      static string ConvolutionVectorParam(const char*, int, float&, float&);
      
      /// Prepares things for convolution mask creation.
      static bool ConvolutionPrepare(const string&, float&, float&);
      
      /// Generates point response value.
      static double PointVectorValue(double, double, double);
      
      /// Generates values for half of a 1D triangle mask.
      static double TriangleVectorValue(double, double, double);
      
      /// Generates values for half of a 1D Gaussian mask.
      static double GaussianVectorValue(double, double, double);
      
      /// Generates values for half of a 1D difference-of-gaussians mask.
      static double DoGVectorValue(double, double, double); 
      
      /// Generates values for half of a 1D rectangular mask.
      static double RectangleVectorValue(double, double, double);
      
      /// Generates values for half of a 1D rectangular mask.
      static double ExponentialVectorValue(double, double, double);

      /// Generates values for half of a 1D Hann mask.
      static double HannVectorValue(double, double, double);

      ///
      bool Convolve();
      
      ///
      bool Convolve(int, int, int);  
      
      ///
      bool ConvolveOneDimMask(int, int, int, const FloatVector&);  
      
      ///
      bool ConvolveNbrRank(const string &,int, int, int) {
 	ShowError("ConvolveNbrRank() not implemented yet");
	return false;
      }
      
      ///
      bool ConvolveRatio(const string &,int, int, int) {
 	ShowError("ConvolveRatio() not implemented yet");
	return false;
      }
      
      ///
      bool ConvolveUmatrix(const string&, int, int, int) {
 	ShowError("ConvolveUmatrix() not implemented yet");
	return false;
      }

      ///
      size_t NumberOfScoreValues() const {
	size_t s = 0;
	for (size_t i=0; i<Nindices(); i++)
	  s += IndexStaticData(i).NumberOfScoreValues();
	return s;
      }

      ///
      int ScoreValueIndex(size_t ii, size_t jj) const {
	size_t s = 0;
	for (size_t i=0; i<Nindices(); i++) {
	  size_t j = IndexStaticData(i).NumberOfScoreValues();
	  if (i==ii)
	    return jj>=j ? -1 : int(s+jj);
	  s += j;
	}
	return -1;
      }

      /// One argument interface to permapnewobject.
      ObjectList& PerMapNewObject(int ii) {
	// this might be slow...
	size_t i = (size_t)ii;
	for (size_t m=0; m<Nindices(); m++)
	  if (i<IndexData(m).objlist.size())
	    return IndexData(m).objlist[i];
	  else
	    i -= IndexData(m).objlist.size();
	static ObjectList empty;
	return empty;
      }
      
      /// const-const version of the one above.
      const ObjectList& PerMapNewObject(int i) const {
	return ((QueryData*)this)->PerMapNewObject(i);
      }
      
      ///
      ObjectList& PerMapNewObject(int i, int j) {
	return IndexData(i).objlist[j];
      }
      
      ///
      const ObjectList& PerMapNewObject(int i, int j) const {
	return ((QueryData*)this)->PerMapNewObject(i, j);
      }

      ///
      const IntVector& BackReference(int i, int l, int j) const {
	if (!IndexIndexOK(i)) {
	  ShowError("CbirPicsom::QueryData::BackReference(i, l, j): i not OK");
	  const IntVector *emptyptr = NULL;
	  return *emptyptr;
	}
	return TsSom(i).BackReference(l, j); 
      }

      ///
      void PerMapObjects(int i) { _sa_permapnewobjects = i; }
      
      ///
      int PerMapObjects() const { return _sa_permapnewobjects; }

      /// Gets file name prefix for score dumps.
      string ScoreFile() { return scorefile; }
      
      /// Sets file name prefix for score dumps.
      void ScoreFile(const string& n) { scorefile = n; }

      /// write access to watch set
      static void Watchset(const IntVector& w) { watchset = w;}
      
      /// write access to watch set
      static void Watchset(const vector<size_t>& w) { 
	watchset.Length(w.size());
	for (size_t i = 0; i<w.size(); i++)
	  watchset[i] = w[i];
      }

      /// 
      float ClassWeight(int) const;

      /// Dumps out hits&convolved matrices in various forms.
      bool MapSnapshots(int, int, const string& xs=string("")) const;

      void MapImageName(const string& n) { mapimagename = n; }
      
      /// gets the above mentioned prefix
      const string &MapImageName() const {return mapimagename;}
      
      /// Sets hits&convolved matrices dump file name prefix for matlab.
      void MapMatlabName(const string& n) { mapmatlabname = n; }
      
      /// Sets hits&convolved matrices dump file name prefix for dat files.
      void MapDatName(const string& n) { mapdatname = n; }

      /// Calling this with true forces enytropy calculations.
      void CalculateEntropy(bool f) {
	calculate_entropy_force = f;
	if (f && GetMatrixCount()==1) {
	  WarnOnce("Setting calculate_entropy_force implies matrixcount=3");
	  SetMatrixCount(3);
	}
      }

      /// True if forced or set_map_weights stage requires it.
      bool CalculateEntropy() const {
	if (calculate_entropy_force)
	  return true;

	cbir_function f = StageFunction(stage_set_map_weights);

	return f==func_entropy_n_polynomial || f==func_entropy_n_leave_out_worst;
      }

      /// Actual routine to call the different types of entropy.
      bool DoEntropyCalculation() {
	size_t kdefault = GetMatrixCount()==3 ? 1 : 0;
	size_t k = entropymatrix==-1 ? kdefault : entropymatrix;

	if (k>=GetMatrixCount())
	  k = kdefault;

	for (size_t m=0; m<Nindices(); m++)
	  for (int l=0; l<Nlevels(m); l++)
	    if (!DoEntropyCalculation(m, l, k))
	      return false;
      
	return true;
      }

      /// Actual routine to call the different types of entropy.
      bool DoEntropyCalculation(int, int, int);

    protected:
      ///
      int seed;

      /// Convolution type.
      string _sa_default_convtype;
      
      /// Convolution type.
      map<int,string> _sa_tssom_convtype;

      /// New CBIR stage limit for XXX.
      int _sa_limit_per_map_units;
      
      /// New CBIR stage limit for XXX.
      int _sa_limit_total_units;
      
      /// New CBIR stage limit for XXX.
      int _sa_limit_per_map_images;

      /// Maximum number of images to be found on each map.
      int _sa_permapnewobjects;

      /// Turns off border compensation in ConvolveOneDimMask()
      /// if false
      bool bordercompensation;

      ///
      float negative_weight;

      ///
      classaugment_t classaugment;

      /// Number of matrices created for each TS-SOM level. 1 or 3 is typical.
      size_t matrixcount_xx;

      /// True if one wants to see entropy values besides TS-SOMs on the browser.
      bool calculate_entropy_force;

      /// Index of matrix that is used for entropy calculation. -1 is default.
      int entropymatrix;

      /// Set of images to be marked positive by ground truth.
      map<string,pair<string,float> > positive;

      /// Set of images to be marked negative by ground truth.
      map<string,pair<string,float> > negative;

      /// Set to true if zero values after convolutions are not problem.
      static bool zero_matrices_allowed;

      ///
      static hitstype_t hitstype;

      static bool store_selfinfluence;

      /// storage for self-influence values of seen images
      map<int,vector<float> > selfinfluence;
      
      ///
      vector<vector<vector<float> > > location_selfinfluence;

      /// causes PlaceSeenOnMapPicSOMthreeMaps() to limit the 
      /// relevance values originating from a single map unit
      bool limitrelevance;

      /// Combination factor for relevance combination between objects
      static float comb_factor;

      /// Combination factor for positive/negative relevance in Hits()
      /// used only if hitstype==posneg_lc
      static float hit_comb_factor;

      /// Name prefix for writing convolved map s.
      string mapimagename;
      
      /// Name prefix for writing convolved map s.
      string mapmatlabname;
      
      /// Name prefix for writing convolved map s.
      string mapdatname;

      /// Name prefix for writing score components
      string scorefile;

      static IntVector watchset;

      ///
      list<pair<ground_truth,float> > classweight;

    }; // class CbirPicsom::QueryData

    /// A utility routine for the virtual methods.
    static CbirPicsom::QueryData *CastData(CbirAlgorithm::QueryData *qd) {
      CbirPicsom::QueryData *qde = static_cast<CbirPicsom::QueryData*>(qd);
      if (!qde) {
	ShowError("CbirPicsom::QueryData pointer conversion failed");
	abort();
      }
      return qde;
    }

    ///
    virtual cbir_stage StageInitialSet(CbirStageBased::QueryData*,
				       cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageRunPerMap(CbirStageBased::QueryData*,
				      cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageSetMapWeights(CbirStageBased::QueryData*,
					  cbir_function, const string&, size_t) const;
    ///
    virtual cbir_stage StageCombineUnitLists(CbirStageBased::QueryData*,
					     cbir_function, const string&, size_t) const;
    ///
    virtual cbir_stage StageExtractObjects(CbirStageBased::QueryData*,
                                           cbir_function, const string&, size_t) const;
    ///
    virtual cbir_stage StageRemoveDuplicates(CbirStageBased::QueryData*,
					     cbir_function, const string&, size_t) const;
    ///
    virtual cbir_stage StageProcessObjects(CbirStageBased::QueryData*,
                                           cbir_function, const string&, size_t) const;

    /// Common per-map-level iterator.
    virtual bool ForAllMapLevels(CbirPicsom::QueryData*,
				 bool (CbirPicsom::*foo)(CbirPicsom::QueryData*,
							 const string&,int,int,bool) const,
				 const string&, bool botonly=false) const;


    /// Gives the number of levels in a map.
    int Nlevels(CbirPicsom::QueryData *qd, int m) const {
      return qd->IsTsSom(m) ? qd->Nlevels(m) : 1;
    }

    /// Gives the number of levels in a map.
    int NlevelsEvenAlien(CbirPicsom::QueryData *qd, int m) const {
      return qd->IsTsSom(m) ? qd->NlevelsEvenAlien(m) : 1;
    }

#if 0 // ifdef QUERY_USE_PTHREADS
    struct map_level_pthread_data {
      pthread_t pthread;
      bool thread_set;
      Query *query;
      bool (Query::*f)(const string&,int,int,bool);
      const string *args;
      int map;
      int level;
      bool botonly;
    };

    /// Common per-map-level iterator.
    bool ForAllMapLevelsPthread(bool (CbirPicsom::*foo)(const string&,int,int,bool),
				const string&, bool);

    ///
    static void *ForAllMapLevelsPthread(void*);

#endif // QUERY_USE_PTHREADS

    /// Part of PicSOM that is run for each separate map.
    virtual bool PerMap(CbirPicsom::QueryData*,
			const string&, int, int, bool) const;
    
    ///
    bool DoClassModelAugmentation(CbirPicsom::QueryData*,
					      int, int, int) const;

    /// 
    set<string> GetAugmentClasses(CbirPicsom::QueryData*, const string&) const;

    ///
    float GetClassAugmentValue(CbirPicsom::QueryData*, 
			       const string&, const string&) const;

    ///
    void CreateAllClassModels(CbirPicsom::QueryData*) const;

    ///
    void PerMapNonBernoulli(CbirPicsom::QueryData*, const string&,
			    int i, int j, bool &ok) const;

    ///
    void PerMapSelfInfluence(CbirPicsom::QueryData*,
			     int i, int j) const;

    /// Returns the set of best vq units from a map level.
    /// Used by the PicSOM algorithm.
    VQUnitList BestVQUnits(CbirPicsom::QueryData*, int, int) const;

    /// Calculate binary features scores.
    bool SimpleBinaryFeatureScores(int) const {
      ShowError("SimpleBinaryFeatureScores() not implemented yet");
      return false;
    }

    /// Returns precalculated scores.
    bool SimplePreCalculatedScores(int) const {
      ShowError("SimplePreCalculatedScores() not implemented yet");
      return false;
    }
    
    /// Returns SVM scores.
    bool SimpleSvmScores(int) const {
      ShowError("SimpleSvmScores() not implemented yet");
      return false;
    }
    
    /// Part of PicSOM that is run after best units have been found.
    virtual bool ForUnits(CbirPicsom::QueryData*, const string&, int, int, bool) const;

    /// Selects the candidate images based on binary classes/features
    /// in the ForUnitsPicSOM() stage.
    ObjectList SelectObjectsByBinaryFeatureScores(int, int, int) const {
      ShowError("SelectObjectsByBinaryFeatureScores() not implemented yet");
      static ObjectList empty;
      return empty;
    }
    
    /// Selects the candidate images based on precalculated scores
    /// in the ForUnitsPicSOM() stage.
    ObjectList SelectObjectsByPreCalculatedScores(int, int, int) const {
      ShowError("SelectObjectsByPreCalculatedScores() not implemented yet");
      static ObjectList empty;
      return empty;
    }

    /// Selects the candidate images based on SVM scores
    /// in the ForUnitsPicSOM() stage.
    ObjectList SelectObjectsBySvmScores(int, int, int) const {
      ShowError("SelectObjectsBySvmScores() not implemented yet");
      static ObjectList empty;
      return empty;
    }

    /// Returns the set of potential new question images from a map level.
    /// Used by the PicSOM algorithm.
    ObjectList SelectObjectsFromVQUnits(CbirPicsom::QueryData*,
                                        double, const VQUnitList&,
                                        const TreeSOM&, bool, int, int) const;
    
    /// was Query::ProcessObjectsPicSOMBottom()
    virtual bool ProcessObjects(CbirPicsom::QueryData*, const string&) const;

    /// Calculates the binary feature score of image idx.
    double BinaryFeatureScoreForObject(int /*map*/, int /*idx*/) const {
      ShowError("BinaryFeatureScoreForObject() not implemented yet");
      return -1.0;
    }

    /// Returns a precalculated score of image idx.
    double PreCalcultedScoreForObject(int /*map*/, int /*idx*/) const {
      ShowError("PreCalcultedScoreForObject() not implemented yet");
      return -1.0;
    }

    class scorefile_pointers {
    public:
      scorefile_pointers() {
	fp = fp_comp = fp_posneg = NULL;
      }

      bool close() {
	if (fp)            fclose(fp);
	if (fp_comp) 	   fclose(fp_comp);
	if (fp_posneg)     fclose(fp_posneg);

	return true;
      }

      FILE *fp;
      FILE *fp_comp;
      FILE *fp_posneg;
    };

    bool OpenScoreFiles(CbirPicsom::QueryData*, scorefile_pointers &fp) const;

    bool WriteScoreFiles(const scorefile_pointers &fp,const string &lbl,
			 int idx,
			 float val, const vector<float> &cvals,
			 const vector<float> &posnegvals) const;

    ///
    class picsom_bottom_score_options_t {
    public:

      picsom_bottom_score_options_t(){
	mul=add=zero=rho=ratioscore=smoothed_fraction=relevancemodulation
	  = maxofsubs=geomsumsubs=sublinearrank=storesubscores=posonly=
	  no_propagation=filedump_ttypecheck=equalise_subobject_count=false;
      }

      picsom_bottom_score_options_t(bool){
	picsom_bottom_score_options_t();
      }

      bool mul;
      bool add;
      bool zero;
      bool rho;
      bool ratioscore;
      bool smoothed_fraction;
      bool relevancemodulation;
      bool maxofsubs;
      bool geomsumsubs;
      bool sublinearrank;
      bool storesubscores;
      bool posonly;
      bool no_propagation;
      bool filedump_ttypecheck;
      bool equalise_subobject_count;
    };

    ///
    picsom_bottom_score_options_t ParseBottomOptions(CbirPicsom::QueryData*,
						     const string &arg) const;

    /// parses comma-separated argument list to a vector of key-value pairs
    static list<pair<string,string> > SplitArgs(const string &arg);

    /// actual score calculating function for the above stage fcn
    float PicSOMBottomScore(CbirPicsom::QueryData*,
			    int ,const picsom_bottom_score_options_t &,
			    FloatVector *,Object *img=NULL, 
			    vector<float> *cvals=NULL,
			    vector<float> *posnegvals=NULL) const;

    double DetermineCountBias() const {
      // determine the histogram count bias based on the 
      // concolution kernel
      
      // bias = 0.5*sum _{kernel} / |support_kernel|;  
      
      // currently a placeholder that always returns the value
      // for a triangle-8 convolution mask:
      // bias = [(K*K+3K+1)/((K+1)(2K+1))]^2 | K=8
      
      return 0.5*7921.0/23409.0;
    }

    double *MLEstimateMapWeights(const vector<vector<float> >& /*scores*/,
				 size_t /*negatives*/) const {
      ShowError("MLEstimateMapWeights() not implemented yet");
      return NULL;
    }

    void ml_calculate_h(const vector<vector<float> > &scores,
			const double *theta, double *tgt) const;

    float ml_calculate_h(const vector<float> &score,
			const double *theta) const;

    ///
    bool PropagateBySegmentHierarchy() const {
      ShowError("PropagateBySegmentHierarchy() not implemented yet");
      return false;
    }

    /// Calculates the theoretical maximum entropy for given matrix size.
    static double MaximumEntropy(const simple::FloatMatrix&);

    /// Calculates the theoretical entropy increase due to the convolution;
    static double ConvolutionEntropy(const FloatVector*);

    /**

    */
    class IndexData : public CbirStageBased::IndexData {

    }; // classCbirPicsom::IndexData

    /// Poisoned constructor.
    CbirPicsom() : CbirStageBased() {}

    /// This does not need to do anything else but call the base class
    /// constructor.  If the class has any non-trivial data members
    /// then they might be initialized here.
    CbirPicsom(DataBase*, const string&);

    /// This constructor just creates the "factory" instance of the class.
    CbirPicsom(bool) : CbirStageBased(false) {}

    /// Does not need to be defined.
    virtual ~CbirPicsom() {}

    /// This method creates the true usable instance.
    virtual CbirAlgorithm *Create(DataBase *db, const string& s) const {
      return new CbirPicsom(db, s);
    }

    /// This method creates the query-specific data structures.
    virtual CbirAlgorithm::QueryData *CreateQueryData(const CbirAlgorithm *a,
						      const Query *q) const {
      return new QueryData(a, q);
    }
    
    /// Name of the algorithm.  Needs to be defined.
    virtual string BaseName() const { return "picsomx"; }

    /// A bit longer name.  Needs to be defined.
    virtual string LongName() const { return "picsom algorithm"; }

    /// Short description  of the algorithm.  Needs to be defined.
    virtual string Description() const {
      return "This is a picsom algorithm.";
    }
    
    /// Query-independent specifiers are appended to BaseName().
    virtual string FullName() const { 
      return BaseName()+(bottom?"_bottom":"")+(weighted?"_weighted":"");
    }

    /// Virtual method that returns a list of parameter specifications.
    virtual const list<parameter>& Parameters() const;

    /// Initialization with query instance and arguments from
    /// algorithm=picsom(arguments) .  Does not need to be defined.
    virtual bool Initialize(const Query*, const string&,
			    CbirAlgorithm::QueryData*&) const;

    /// Something can be done when features have been specified.  Does
    /// not need to be defined.
    virtual bool AddIndex(CbirAlgorithm::QueryData*, Index::State*) const;

    /// Does not need to be defined if the algorithm does not have any
    /// tunable parameters of its own.
    virtual bool Interpret(CbirAlgorithm::QueryData *qd, const string& key,
			   const string& value, int& res) const;

    /// This method implements the actual CBIR algorithm.  Needs to be
    /// defined.
    virtual ObjectList CbirRound(CbirAlgorithm::QueryData*, const ObjectList&,
				 size_t) const;

    ///
    virtual cbir_function StageDefault(cbir_stage stage, bool warn) const;

    /// Selects initial images among images on top levels.
    bool TopLevelsByEntropy() const { return false; }

  protected:
    ///
    bool bottom;

    ///
    bool weighted;

  }; // class CbirPicsom

} // namespace picsom

#endif // _CBIRPICSOM_H_
