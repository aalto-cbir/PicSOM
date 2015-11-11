// -*- C++ -*-  $Id: SVM.h,v 2.59 2015/04/11 19:18:51 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_SVM_H_
#define _PICSOM_SVM_H_

#include <fstream>

#include <VectorIndex.h>
#include <VectorSet.h>

#include <libsvm.h>
#include <liblinear.h>
#include <PmSVM.h>

#if defined(HAVE_VL_GENERIC_H) && !defined(HAVE_WINSOCK2_H)
#define USE_VLFEAT
#endif // HAVE_VL_GENERIC_H

#ifdef USE_VLFEAT
extern "C" {
#include <vl/generic.h>
#include <vl/homkermap.h>
}
#endif

namespace picsom {
  /**
     A wrapper around libsvm.
  */
  class SVM : public VectorIndex {
  public:
    /**
       The constructor for the SVM class.
       @param db     the database where features have been calculated
       @param name   canonical name of feature set (identifier inside PicSOM)
       @param feat   feature in use
       @param path   full path to directory containing the files
       @param params string with svm_foo=xxx parameters (with svm_ stripped)
    */
    SVM(DataBase *db, const string& name, const string& feat,
	const string& path, const string& params, const Index* src);

    ///
    SVM(bool) : VectorIndex(false) {}

    ///
    SVM() { abort(); }

    ///
    SVM(const Index&) { abort(); }

    ///
    virtual ~SVM();

    /// Sets debugging.
    static void DebugSVM(size_t d) { debug_svm = d; }

    /// Checks debugging.
    static size_t DebugSVM() { return debug_svm; }

  private:

    class kernel_t {
    public:
      kernel_t() : kernel_func(NO_KERNEL), use_exp(false) {}

      kernel_t(const string&);

      kernel_t(const kernel_t& other) {
        kernel_func = other.kernel_func;
        use_exp = other.use_exp;
      }

      /// enum for specifying SVM kernel implemented in PicSOM 
      typedef enum { 
        NO_KERNEL,
        EUCL,         // rbf (exponential euclidean)
        // EXP_CHI2,    // exponential chi^2
        INT,         // intersection
        CHI2,        // chi^2
        JS,          // Jensen-Shannon
      } kernel_func_t;

      static vector<string>& KernelTypeNames() {
        static vector<string> ktn = {
          "NO_KERNEL",
          "EUCL",
          // "EXP_CHI2",
          "INT",
          "CHI2",
          "JS"
        };
        return ktn;
      }

      string str() const;

      bool operator !=(const kernel_t& other) const {
        return
          kernel_func != other.kernel_func ||
          use_exp != other.use_exp;
      }

      // bool operator!=(kernel_t& other) {
      //   return 
      //     kernel_func == other.kernel_func &&
      //     use_exp == other.use_exp;

      // }

      kernel_func_t func() const { return kernel_func; }

      bool uses_exp() const { return use_exp; }

      bool ok() const { return kernel_func != NO_KERNEL; }
        
    private:
      kernel_func_t kernel_func;
      bool use_exp;
    };

  public:
    typedef enum {
      LIB_SVM,
      LIB_LINEAR,
      PM_SVM,
    } library_t;

    static const string& LibraryString(library_t lt) {
      static vector<string> library_names;
      if (library_names.empty()) {
        library_names.push_back("svm");
        library_names.push_back("linear");
        library_names.push_back("pmsvm");
      }
      return library_names[lt];
    }
    
    const string& LibraryString() const {
      return LibraryString(svm_library);
    }

    library_t GetLibrary() const { return svm_library; }

    /// forward declaration
    class State;

    ///
    virtual const string& MethodName() const {
      static string svm = "svm";
      return svm;
    }

    ///
    virtual Index* Make(DataBase *db, const string& name, const string& path,
			const string& params, const Index *src) const {
      string feat = name;
      size_t p = feat.find('#');
      if (p!=string::npos)
	feat.erase(p);
      return new SVM(db, name, feat, path, params, src);
    }

    ///
    virtual bool IsMakeable(DataBase* db, const string& fn, 
			    const string& dirstr, bool) const;

    ///
    virtual Index::State *CreateInstance(const string& n) {
      return new State(this,n);
    }

    /// Returns true if this feature can appear in the user interface.
    virtual bool IsSelectable() /*const*/ {
      return true;//GuessVectorLength() || IsBinaryFeature();
    }

    /// Return name of the index for internal use and filename.
    string Identifier(const string& pre, const string& f, const string& o,
		      const string& e, const string& c, const string& i) const {
      return pre+MethodSeparator()+f+"::"+o+"::"+e+"#"+c+
	(i==""?"":"§"+i);
    }

    /**
       This creates the SVM classifier, my reading model from file, or
       by performing parameter selection.
       @param svmname
       @param fname
       @param svmlib         what svm library we are using: svm, linear
       @param cls            set of ground truth classes for pos and neg
       @param readfrom_model
    */
    bool Create(const string& svmname, string& fname, const string& svmlib,
                const ground_truth_set& cls, const string& readfrom_model);

  private:
    typedef list<FloatVectorSet> setlist_t;

    bool InitProblem(setlist_t& setlist, const ground_truth_set& cls);
  public:

    ///
    static bool allowedParam(const string& key);

    ///
    void setParams(const map<string,string>& m) {
      non_static_params = m;
    }

  private:
    ///
    void setParam(const pair<string,string>& keyval) {
      non_static_params.insert(keyval);
    }

    ///
    void setParam(const string& key, const string& val) {
      setParam(make_pair(key, val));
    }

    ///
    void setParam(const string& keyval) {
      setParam(SplitKeyEqualValueNew(keyval));
    }

    ///
    string getParam(const string& key, const string& def="") const {
      map<string,string>::const_iterator it = non_static_params.find(key);
      return it==non_static_params.end() ? def : it->second;
    }

    ///
    bool paramGiven(const string& key) const {
      map<string, string>::const_iterator it = non_static_params.find(key);
      return it!=non_static_params.end();
    }
    
    ///
    int getParamInt(const string& key, int def=-1) const {
      string ret = getParam(key,"");

      const char* nptr = ret.c_str();
      char* endptr;
      errno = 0;
      int n = strtol(ret.c_str(), &endptr, 10);

      if (errno != 0 || endptr == nptr)
        return def;
      return n;
    }

    ///
    float getParamFloat(const string& key, float def=-1.0) const {
      string ret = getParam(key,"");

      const char* nptr = ret.c_str();
      char* endptr;
      errno = 0;
      float n = strtof(ret.c_str(), &endptr);

      if (errno != 0 || endptr == nptr)
        return def;
      return n;
    }

    ///
    bool getParamBool(const string& key, bool def=false) const {
      string ret = getParam(key, def ? "true" : "false");
      return IsAffirmative(ret);
    }

  public:
    ///
    bool LoadModel(const string&, const string&,
		   const string&, const string&);

    ///
    double PredictProb(const FloatVector& f, const string& type = "prob", 
                       int lab=0);
  private:
    ///
    vector<double> PredictProbMulti(const FloatVector& v,
				    const string& type = "prob");

  public:
    ///
    class State : public VectorIndex::State {
    public:
      ///
      State(Index *t, const string& n = "") : VectorIndex::State(t, n) {}

      ///
        virtual State *MakeCopy(bool /*full*/) const {
	State *s = new State(*this);
	return s;
      }

      ///
      virtual float ScoreValue(int) const { return 0; }

      ///
      bool Initialize();

    };  // class SVM::State

    static void DisableCache(bool b);

    bool IsReady() const { return is_ready; }

    bool IsFailed() const { return is_failed; }

  private:

    ///
    bool ReadPreToCDF(const string&, bool, const string&, const string&);

    ///
    bool SetLibrary(const string& svmlib);

    /// Write ce, ge to cache file if they are not already there
    void ParamselCacheWrite(double ce, double ge, double acc);

    /// Get value from cache, return negative if not present
    double ParamselCacheGet(double ce, double ge);

    /// Read the paramsel cache file to memory
    void ParamselCacheRead(const string& fname);

    typedef pair<double, double> paramsel_pair_t;
    map<paramsel_pair_t, double> paramsel_cache;
    string paramsel_cache_filename;

    library_t svm_library;

    class Parameter {
    public:
      Parameter();

      Parameter(const Parameter&);

      ~Parameter();

      void SetLibrary(SVM::library_t l) { svm_library = l; }

      bool SetKernel(SVM::kernel_t);

      void UseExp(bool exp);

      bool UseExp() const;

      void SetWeights(double cweight, int nr);

      void SetProbability(bool b) {
        sparam.probability = b;
      }

      bool GetProbability() const { return sparam.probability; }

      void SetEps(double d) {
        sparam.eps = lparam.eps = d;
      }

      void SetC(double d, bool power) {
        lparam.C = power ? pow(2,d) : d;
        sparam.C = lparam.C;
      }

      double GetC() const { return lparam.C; }

      double GetP() const { return pmsvm_p; }

      void SetP(double p) { pmsvm_p = p; }

      void SetGamma(double d, bool power) {
        sparam.gamma = power ? pow(2,d) : d;
      }

      double GetGamma() const { return sparam.gamma; }

      void SetSolver(int s) { lparam.solver_type = s; }

      int GetSolver() const { return lparam.solver_type; }

      kernel_t GetKernel() const { return kernel_type; }

      bool KernelHasGamma() const {
        return svm_library == LIB_SVM && GetKernel().uses_exp();
      }
      
      const struct parameter* GetLinear() const { return &lparam; }

      const struct svm_parameter* GetSVM() const { return &sparam; }

    protected:
      /// internal pointer to liblinear parameter
      struct parameter lparam;

      /// internal pointer to libsvm parameter
      struct svm_parameter sparam;

    private:
      ///
      void SetLinearXX(const string&);
      
      ///
      void SetSVMXX(const string&);

      double pmsvm_p;

      kernel_t kernel_type;
      library_t svm_library;
      bool use_exp;
    }; // class SVM::Parameter

    class Model;

    class Problem {
    public:

      /// Constructor that initializes from given file
      Problem(library_t, const string&);

      /**
         Constructor that initializes from given ground truth set.
         @param cls   ground_truth_set of positives and negatives
         @param model pointer to model in use (this is needed for scaling)
      */
      Problem(library_t, const setlist_t&);
      
      ~Problem();

    private:
      ///
      void InitXY();

      ///
      bool SetXY(int i, struct svm_node* xx, double yy);

      ///
      bool Read(const string& fname);

    public:
      ///
      bool Write(const string& fname); 

      bool CheckParameter(const Parameter& param);

    // private:
      ///
      // void Set(const FloatVectorSet& p, const FloatVectorSet& n,
      //          struct svm_node*& x_space);

      // void Set(const setlist_t& sl, struct svm_node*& x_space);

    // public:
      const svm_node* GetX(int i);

      double GetY(int i) const;

      int GetL() const { return sprob.l; }

      const struct problem* GetLinear() { return &lprob; }

      const struct svm_problem* GetSVM() { return &sprob; }

      PmSVM::problem* GetPmSVM() { return &pprob; }

      size_t NrClass() const { return nr_class; }

    private:
      /// 
      void InitMembers(library_t);

      size_t nr_class;

      library_t svm_library;

      void SetL(int l) {
        sprob.l = lprob.l = pprob.l = l;
      }

      svm_node* x_space;

      /// internal pointer to liblinear problem
      struct problem lprob;

      /// internal pointer to libsvm problem
      struct svm_problem sprob;

      PmSVM::problem pprob;
    }; // class SVM::Problem


    /**
       Model is a wrapper around the underlying model in the svm library used.
       E.g. the support vectors.
    */
    class Model {
    public:
      ///
      Model(SVM*);

      ///
      ~Model();

      ///
      void RangeFromSetList(const setlist_t&);

      ///
      bool HasModel() { 
        return
          (svm_library == LIB_LINEAR && lmodel) ||
          (svm_library == LIB_SVM && smodel) ||
          (svm_library == PM_SVM && pmodel);
      }

      ///
      bool Write(const string& svmname);

      ///
      void PredictValues(const struct svm_node *x, double* p);

      ///
      void PredictProbability(const struct svm_node *x, double* p);

      ///
      bool Load(const string&);

      ///
      void Train(SVM::Problem*);

      ///
      bool Scale(FloatVectorSet& s) {
        bool ok = true;
        for (int i=0; i<s.Nitems() && ok; i++)
          ok = Scale(*s.Get(i));
        return ok;
      } 

      ///
      bool Scale(FloatVector& v);

      ///
      void SetParameter(const Parameter& p);

      ///
      Parameter GetParameter() { return param; }

      ///
      bool CheckProbability() {
        if (svm_library == LIB_LINEAR)
          return check_probability_model(lmodel);
        else if (svm_library == LIB_SVM)
          return svm_check_probability_model(smodel);
        else if (svm_library == PM_SVM)
          return false; // FIXME 
        return false;
      }

      ///
      int GetClassNr() {
        if (svm_library == LIB_LINEAR)
          return get_nr_class(lmodel);
        else if (svm_library == LIB_SVM)
          return svm_get_nr_class(smodel);
        else if (svm_library == PM_SVM)
          return pmodel->nr_class;
        return 0;
      }

      ///
      void GetLabels(int* label) {
        if (svm_library == LIB_LINEAR)
          get_labels(lmodel, label);
        else if (svm_library == LIB_SVM)
          svm_get_labels(smodel, label);
        else if (svm_library == PM_SVM)
          label = pmodel->label; // FIXME ?
      }

      void ResetZeroedCounter() { scale_zeroed = 0; }
      
      int ZeroedCounter() const { return scale_zeroed; }

      int MinVectorLength();

      void DoScaling(bool b) { do_scaling = b; }

      bool DoScaling() const { return do_scaling; }

      void ScalingFrom(float s) { scaling_from = s; }
      void ScalingTo(float s) { scaling_to = s; }

    private:

      /// Copies values from the Parameter object to the internal
      /// param structure of lmodel/smodel.
      void SyncParameter();

      SVM* svm;

      ///
      list<pair<string,string> > ReadExtraParameters(const string&);

      ///
      library_t svm_library;

      ///
      int scale_zeroed;
      
      ///
      float scaling_from, scaling_to;
      
      ///
      FloatVector range_min;
      FloatVector range_max;

      ///
      bool do_scaling;

      ///
      int min_vec_len;

      ///
      Parameter param;

      /// internal pointer to liblinear model
      struct model* lmodel;

      /// internal pointer to libsvm model
      struct svm_model* smodel;

      PmSVM::model* pmodel;

    }; // class SVM::Model


    /**
       Target is a wrapper around the target values for cross
       validation.  The internal representation might be different for
       different svm libraries.
    */
    class Target {
    public:
      Target(int, library_t);

      ~Target();

      double Get(int i) const {
        return use_int ? (double)target_i[i] : target_d[i];
      }

      double* GetDouble() { return target_d; }
      int* GetInteger() { return target_i; }

    private:
      bool use_int;
      double* target_d;
      int* target_i;
    }; // class SVM::Target
    
    ///
    double GetCWeight(const ground_truth_set& cls);

    ///
    bool DoParamSel(Parameter& param, Problem* prob, const string& paramsel);

    ///
    bool SetParametersFromParamsel(Model* model, const string& pselname);

    ///
    // bool InitProblem(Problem* prob,// parameter& param,
    //                  svm_node*& x_space, const ground_truth_set& cls);

    ///
    struct svm_node* ConvertVector(const FloatVector& v);

    ///
    // static void ReadProblem(const string& fn) { readproblem = fn; }

    // ///
    // static string ReadProblem() { return readproblem; }

    ///
    bool AddModel(const string& n); //, struct model *p);

    ///
    bool DestroyModel(const string& n, bool angry);

    ///
    typedef vector<double> seq_t;
    typedef pair<double, double> param_pair_t;
    typedef vector<param_pair_t> seq_param_t;

    ///
    void CrossValidation(Problem* prob, Parameter& param, int fold,
                         Target& target, double** prob_estim);

    ///
    void GridSearch(Problem*, Parameter&, seq_t, seq_t, int, 
                    double&, double&, double&);

    Model* model;

    ///
    // static string readproblem;

    ///
    seq_t Sequence(double, double, double);

    ///
    seq_t PermuteSequence(seq_t);

    ///
    seq_param_t ParameterSequence(const seq_t&, const seq_t&);

    ///
    static bool SplitRangeString(const string& str, double& begin,
                                 double& step, double& end);

    ///
    map<string,string> non_static_params;

    ///
    bool is_ready;

    ///
    bool is_failed;

    ///
    void SetBaseFilename(const string& fname) {
      base_fname = fname;
    }

    ///
    string GetFilename(const string& ending) {
      return base_fname+ending;
    }

    ///
    string base_fname;

    ///
    typedef enum { NO_NORM, L1_NORM, L2_NORM } norm_t;

    norm_t normalization;

    void DoNormalize(FloatVector*);
    void DoNormalize(FloatVectorSet&);

    ///
    typedef enum { NO_PREPROCESS, SQRT } preprocess_t;

    preprocess_t preprocess;

    void DoPreprocess(FloatVector*);
    void DoPreprocess(FloatVectorSet&);

  public:
    norm_t Normalization() const { return normalization; }
    void Normalization(norm_t n)  { normalization = n; }

    preprocess_t Preprocessing() const { return preprocess; }
    void Preprocessing(preprocess_t n)  { preprocess = n; }

    string HomogeneousKernelMap() const { return hkm_name; }
  private: 

#ifdef USE_VLFEAT
    bool InitialiseHomogeneousKernelMap(const string&, int);
    void HomogeneousKernelMapEvaluate(FloatVector*);
    void HomogeneousKernelMapEvaluate(FloatVectorSet&);

    VlHomogeneousKernelMap* hkm;
    int hkm_order;
#endif

    ///
    string hkm_name;

    ///
    static size_t debug_svm;
  };  // class SVM

} // namespace picsom

#endif // _PICSOM_SVM_H_

