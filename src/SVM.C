// -*- C++ -*-  $Id: SVM.C,v 2.103 2015/06/29 12:02:10 jorma Exp $
// 
// Copyright 1998-2014 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <SVM.h>

#include <DataBase.h>
#include <Analysis.h>

#include <cerrno>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define CACHE_DIFF 0.000001

void print_null(const char *) {}

namespace picsom {
  static const string SVM_C_vcid =
    "@(#)$Id: SVM.C,v 2.103 2015/06/29 12:02:10 jorma Exp $";

  static SVM list_entry(true);

  size_t SVM::debug_svm = 0;
  
  // string SVM::readproblem = "";

  /////////////////////////////////////////////////////////////////////////////

  /* examples:
     n: svm::ColorSIFT#$randcut(aeroplane&trainval,50,42)
     f: ColorSIFT
     p:
     ps: cweight=3,fold=3,params=kernel_type=CHISQUARED_NOEXP,\
         paramselect=grid+grid,scaling=false,use_cache=true
  */

  SVM::SVM(DataBase *d, const string& n, const string& f, const string& p, 
           const string& ps, const Index* src) : 
    VectorIndex(d, n, f, p, ps, src), svm_library(LIB_SVM),
    model(NULL), is_ready(false), is_failed(false) {
    static const string msg = "SVM::SVM() : ";

    if (ps=="") { 
      ShowError(msg+"empty parameter string");
      return; 
    }
    
    vector<string> pars = SplitInCommasObeyParentheses(ps);
    for (size_t i=0; i<pars.size(); i++)
      if (pars[i].find('=')==string::npos) {
	ShowError(msg+"no '=' in \""+pars[i]+"\"");
	return;
      }
      else
	setParam(pars[i]);

    svm_set_print_string_function(print_null); // for libsvm
    set_print_string_function(print_null);     // for liblinear

    SetFeatureNames();

#ifdef USE_VLFEAT
    hkm_order = 0;
    hkm = NULL;
    InitialiseHomogeneousKernelMap(getParam("homker", ""),
				   getParamInt("homkerorder", 3));
#endif

    string norm_str = getParam("normalize", "");
    if (norm_str.empty())
      normalization = NO_NORM;
    else if (norm_str == "L1")
      normalization = L1_NORM;
    else if (norm_str == "L2")
      normalization = L2_NORM;
    else
      ShowError(msg, "Unknown norm specification: "+norm_str);

    if (normalization != NO_NORM)
      WriteLog(msg, "Setting normalization to "+ToStr(normalization));

    string preprocess_str = getParam("preprocess_components", "");
    if (preprocess_str.empty())
      preprocess = NO_PREPROCESS;
    else if (preprocess_str == "sqrt")
      preprocess = SQRT;
    else
      ShowError(msg, "Unknown preprocessing mode: "+preprocess_str);

    DisableCache(!getParamBool("use_cache", false));

    string modelfile = getParam("modelfile");
    string library   = getParam("library");
    string positive  = getParam("positive");
    string negative  = getParam("negative");
    if (modelfile!="" && library!="")
      if (!LoadModel(modelfile, library, positive, negative)) {
	is_failed = true;
	return;
      }

    int pdfmodels = getParamInt("pdfmodels");
    if (pdfmodels>0)
      CreatePDFmodels(pdfmodels, false, false);
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::~SVM() {
    DestroyModel("", false);
#ifdef USE_VLFEAT
    if (hkm)
      vl_homogeneouskernelmap_delete(hkm);
#endif
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::IsMakeable(DataBase* /*db*/, const string& fn, 
                       const string& dirstr, bool create) const {
    if (!create) {
      string fname = dirstr+"/"+fn+".svm";
      return FileExists(fname);
    }
    string mm = MethodName()+MethodSeparator();
    return fn.find(mm)==0;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool SVM::SplitRangeString(const string& str, double& begin, double& step,
                             double& end) {
    string msg = "SVM::SplitRangeString(): ";
    
    vector<string> v = SplitInCommas(str);
    if (v.size() != 3)
      return ShowError(msg, "Range should have three parts: begin, step, end.");
    
    begin = atof(v[0].c_str());
    step = atof(v[1].c_str());
    end = atof(v[2].c_str());

    // some (non-exhaustive) sanity checks ...
    if ((end > begin && step < 0) ||
        (end < begin && step > 0) ||
        (end == begin && step!=0))
      return ShowError(msg, "Given range not sensible: ", str);
      
    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool SVM::allowedParam(const string& key) {
    static set<string> ps = {
      "C",
      "options",
      "extra",
      "log2c",
      "gamma",
      "log2g",
      "kernel_type",
      "linear_solver",
      "homker",
      "homkerorder",
      "predict_gt",
      "cweight",
      "fold",
      "max_negative",
      "max_positive",
      "normalize",
      "paramselect",
      "predict",
      "predtrain",
      "predcont",
      "readmodel",
      "scaling",
      "trainmodel",
      "use_cache",
      "load_pre",
      "preprocess_components",
      "use_bug_2_78",
      "allow_init_from_paramsel",
      "writeproblem",
      "readproblem",
      "posneg_ratio",
      "scaling_from",
      "scaling_to"
    };

    bool found = ps.find(key)!=ps.end();

    return found;
  }

  /////////////////////////////////////////////////////////////////////////////

  // cache for calculated values
  typedef pair<const svm_node*, const svm_node*> kernel_idx_t;
  static map<kernel_idx_t, double> kcache;

  typedef double (*kernel_diff_func_t)(kernel_idx_t);
  static kernel_diff_func_t kernel_diff_func = NULL;

  // whether the used kernel is "exponential type"
  static bool exp_type_kernel = true;

  static bool disable_cache = true;

  void SVM::DisableCache(bool b) { disable_cache = b; }


  static double dot(const svm_node *px, const svm_node *py) {
    double sum = 0;
    while (px->index != -1 && py->index != -1) {
      if (px->index == py->index) {
        sum += px->value * py->value;
        ++px;
        ++py;
      } else {
        if (px->index > py->index)
          ++py;
        else
          ++px;
      }			
    }
    return sum;
  }

  static double diff_eucl(kernel_idx_t pp) {
    const svm_node* px = pp.first;
    const svm_node* py = pp.second;
    
    return -(dot(px,px)+dot(py,py)-2*dot(px,py));
  }

  static double diff_chi2(kernel_idx_t pp) {
    const svm_node* px = pp.first;
    const svm_node* py = pp.second;

    double sum = 0;
    while (px->index != -1 && py->index != -1) {
      if (px->index == py->index) {
        double s=px->value + py->value;
        if (s!=0) {
          double d=px->value - py->value;
          sum += d*d/s;
        }
        ++px;
        ++py;
      } else {
        if (px->index > py->index) {	
          sum += py->value;
          ++py;
        } else {
          sum += px->value;
          ++px;
        }
      }			
    }

    while (px->index != -1) {
      sum += px->value;
      ++px;
    }
    
    while (py->index != -1) {
      sum += py->value;
      ++py;
    }

    return -sum;
  }

  // histogram intersection
  static double diff_int(kernel_idx_t pp) {
    const svm_node* px = pp.first;
    const svm_node* py = pp.second;

    double sum = 0;
    while (px->index != -1 && py->index != -1) {
      if (px->index == py->index) {
	sum += min (px->value, py->value);
	++px;
	++py;
      } else {
	if (px->index > py->index)
	  ++py;
	else
	  ++px;
      }		
    }
    return sum;
  }

  // chi-squared kernel alternative ~ 2xy/(x+y)
  // static double diff_chi2_alt(kernel_idx_t pp) {
  //   const svm_node* px = pp.first;
  //   const svm_node* py = pp.second;

  //   double sum = 0;
  //   while (px->index != -1 && py->index != -1) {
  //     if (px->index == py->index) {
  //       sum += 2*(px->value*py->value)/(px->value + py->value);
  //       ++px;
  //       ++py;
  //     } else {
  //       if (px->index > py->index)
  //         ++py;
  //       else
  //         ++px;
  //     }			
  //   }
  //   return sum;
  // }

  // Jensen-Shannon's Kernel
  static double diff_js(kernel_idx_t pp) {
    const svm_node* px = pp.first;
    const svm_node* py = pp.second;
    
    double sum = 0;
    while (px->index != -1 && py->index != -1) {
      if (px->index == py->index) {
	sum+= px->value/2*log((px->value+py->value)/px->value) +
          py->value/2*log((px->value+py->value)/py->value);
	++px;
	++py;
      }	else {
	if (px->index > py->index)
	  ++py;
	else
	  ++px;
      }			
    }
    return sum;
  }


#define CLEANUP   1000000
#define CACHEMAX 40000000

  static double myKernelCallback(double gamma, 
				 const svm_node *px,
				 const svm_node *py) {
    static bool error_shown_before = false;
    double res = 0.0;

    if (!kernel_diff_func)
      return ShowError("SVM: using callback without a function being set!");

    kernel_idx_t cidx = make_pair(px,py);
    map<kernel_idx_t, double>::iterator it = kcache.end();

    if (!disable_cache) {
      it = kcache.find(cidx);

      // assume symmetric
      if (it == kcache.end()) {
        cidx = make_pair(py,px);
        it = kcache.find(cidx);
      }
    }

    // if still not found in cache, we just have to calculate it...
    if (it == kcache.end()) {
      res = (*kernel_diff_func)(cidx);
      if (kcache.size() >= CACHEMAX) {
        cout << "[SVM] Kernel cache size " << kcache.size() 
             << " is at max. Removing some elements... ";
        for (long int i=0; i<CLEANUP; i++)
          kcache.erase(kcache.begin());
        cout << " done." << endl;
      }
      if (!disable_cache)
	kcache[cidx] = res;

    } else
      res = it->second;

    if (exp_type_kernel) {
      errno = 0;
      double oldres = res;
      res = exp(gamma*res);
      int e = errno;
      if (e) {
        if (!error_shown_before) {
          cout << "WARNING: myKernelCallback(): exp() gave a floating-point "
            "exception. (errno" << e << "). You might need to scale your "
            "features! gamma=" << gamma << " res=" << oldres << endl;
          error_shown_before = true;
        }
        if (res != 0)
          return MAXFLOAT;
      }
    }

    return res;
  }    


  /////////////////////////////////////////////////////////////////////////////

  // SVM::kernel_t SVM::KernelType(const string& name) {
  SVM::kernel_t::kernel_t(const string& name) :
    kernel_func(NO_KERNEL), use_exp(false) {

    string kname(name);

    if (kname == "RBF") {
      use_exp = true;
      kernel_func = EUCL;
      return;
    }
    
    if (kname.substr(0, 4) == "EXP_") {
      use_exp = true;
      kname.erase(0, 4);
    }

    const vector<string>& ktn = KernelTypeNames();

    for (size_t i=0; i<ktn.size(); i++)
      if (kname == ktn[i])
        kernel_func =  kernel_func_t(i);

    if (kname == "EUCL")
      kernel_func = NO_KERNEL;
  }

  /////////////////////////////////////////////////////////////////////////////

  // const string& SVM::KernelType(SVM::kernel_t kt) {
  string SVM::kernel_t::str() const {
    const vector<string>& ktn = KernelTypeNames();

    size_t i = (size_t)kernel_func;
    if (i >= ktn.size()) {
      ShowError("SVM::kernel_t::str(): "+ToStr(i)+" >= "+ToStr(ktn.size()));
      return "";
    }
    
    string ret = (use_exp ? "EXP_" : "") + ktn[i];
    if (ret == "EXP_EUCL")
      ret = "RBF";
      
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::SetLibrary(const string& svmlib) {
    const string msg = "SVM::SetLibrary(): ";

    if (svmlib == "svm") {
      svm_library = LIB_SVM;
      return true;
    } else if (svmlib == "linear") {
      svm_library = LIB_LINEAR;
      return true;
    } else if (svmlib == "pmsvm") {
      svm_library = PM_SVM;
      return true;
    }

    // if given library not found, report an error
    string error_msg = "No SVM library named \""+svmlib+"\"!";

    bool params_in_library = svmlib.find('=') != string::npos;
    if (IsAffirmative(svmlib) || params_in_library) {
      error_msg +=
        " Please use svm= only for setting library, e.g. svm=svm or svm=linear."
        " Please give library parameters with svm_params.";
      if (params_in_library)
        error_msg += " E.g. svm_params="+svmlib+".";
    }
        
    return ShowError(msg, error_msg);
  }

  /////////////////////////////////////////////////////////////////////////////

  double SVM::GetCWeight(const ground_truth_set& cls) {
    static const string msg = "SVM::GetCWeight() : ";
    
    string cwp = getParam("cweight","1.0");
    double cweight = atof(cwp.c_str());

    if (cwp == "auto" && cls.size()!=2) {
      ShowError(msg, "Doesn't make sense to use cweight=cauto with "
                "multiclass SVM.");
      return 1.0;
    }

    if (cwp == "auto")
      cweight = (double)cls[1].positives() / (double)cls[0].positives();
    if (cweight <= 0.0)
      cweight = 1.0;
    if (cweight != 1.0)
      WriteLog(msg, "Setting C penalty weight ", cwp=="auto" ? "[auto]" : "",
               " = "+ToStr(cweight));

    return cweight;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::SetParametersFromParamsel(Model* model, const string& pselname) {
    const string msg = "SVM::SetParametersFromParamsel: ";

    ParamselCacheRead(pselname);

    map<paramsel_pair_t, double>::const_iterator it;
    paramsel_pair_t best;
    double best_acc = -1.0;

    for (it = paramsel_cache.begin(); it != paramsel_cache.end() ; it++)
      if (it->second > best_acc) {
        best = it->first;
        best_acc = it->second;
      }

    if (best_acc < 0)
      return false;

    Parameter p;
    p.SetC(best.first, true);
    p.SetGamma(best.second, true);
    model->SetParameter(p);

    WriteLog(msg, "Selecting best parameters: log2(C)=" + ToStr(best.first) +
             ", log2(g)="+ToStr(best.second)+".");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Create(const string& svmname, string& fname,
                   const string& svmlib, const ground_truth_set& cls,
                   const string& readfrom_model) {
    string msg = "SVM::Create: ";
    if (!SetLibrary(svmlib))
      return false;

    WriteLog(msg, "[" + LibraryString() + "]");

    DestroyModel(svmname, false);

    model = new Model(this);
    // kernel_t kt = KernelType(getParam("kernel_type", "NO_KERNEL"));
    kernel_t kt(getParam("kernel_type", "NO_KERNEL"));

    if (fname.empty())
      fname = "./"+MethodName()+MethodSeparator()+IndexName()+"#"+svmname;
    SetBaseFilename(fname);

    string svm_fname = GetFilename(".svm");
    string paramsel = getParam("paramselect");

    // Possibly read model from model file.
    if (IsAffirmative(getParam("readmodel","no")) || !readfrom_model.empty()) {
      string readfrom = readfrom_model;
      if (readfrom.empty())
        readfrom = svm_fname;

      if (!FileExists(readfrom))
        readfrom = db->ExpandPath("svms", readfrom);
      
      if (!FileExists(readfrom)) {
        string pselname = GetFilename(".paramsel");
        if (FileExists(pselname) &&
            getParamBool("allow_init_from_paramsel", false)) {
          WriteLog(msg, "<"+readfrom+"> not found. Reading parameters from <"+
                   pselname+"> instead.");
          SetParametersFromParamsel(model, pselname);
        } else {
          return ShowError(msg, "Instructed to read model from <"+readfrom+">, "
                           "but file cannot be found!");
        }
      } else {
        WriteLog(msg, "Reading model from <"+readfrom+"> ...");
        model->Load(readfrom);

	if (paramsel!="" && getParam("c_range")=="" &&
	    getParam("g_range")=="") {
	  Parameter param = model->GetParameter();
	  bool use_gamma = param.KernelHasGamma();
	  float c = param.GetC(), g = use_gamma ? param.GetGamma() : 1;
	  float c_log2 = log2(c), g_log2 = log2(g);
	  size_t c_n = 5, g_n = 5;
	  float c_step = 0.4, g_step = 0.4;
	  float c_hrange = (c_n-1)/2*c_step, g_hrange = (g_n-1)/2*g_step;
	  float c_begin = c_log2-c_hrange, c_end = c_log2+c_hrange;
	  float g_begin = g_log2-g_hrange, g_end = g_log2+g_hrange;
	  string c_range = ToStr(c_begin)+","+ToStr(c_step)+","+ToStr(c_end);
	  string g_range = ToStr(g_begin)+","+ToStr(g_step)+","+ToStr(g_end);
	  if (!use_gamma)
	     g_range = ToStr(g_log2)+",0,"+ToStr(g_log2);

	  WriteLog(msg, "C="+ToStr(c)+" g="+ToStr(g)+" log2(C)="+ToStr(c_log2)+
		   " log2(g)="+ToStr(g_log2)+" setting c_range="+c_range+
		   " and g_range="+g_range);
	  setParam("c_range", c_range);
	  setParam("g_range", g_range);
	}
      }
    } else {
      // If we are not starting from a file, set reasonable default
      // for scaling.
      bool sc = false;
      if (paramGiven("scaling"))
        sc = getParamBool("scaling");
      else if (svm_library == LIB_SVM && kt.func() == kernel_t::EUCL)
        sc = true;
        
      model->DoScaling(sc);
    }

    // Copy current parameter settings from model and modify.
    Parameter param = model->GetParameter();
    param.SetProbability(true);
    param.SetWeights(GetCWeight(cls), cls.size());

    if (svm_library == PM_SVM) {
      if (kt.func() == kernel_t::CHI2)
        param.SetP(-1);
      else if (kt.func() == kernel_t::INT)
        param.SetP(-8);
      else
        return ShowError(msg, "PmSVM currently only supports"
			 " CHI2 and INT kernels.");
    }

    if (svm_library == LIB_SVM) {
      const kernel_t& pkt = param.GetKernel();

      // Error if given and file-read kernels differ
      if (kt.ok() && pkt.ok() && kt != pkt)
        return ShowError(msg, "Kernel in script differs from kernel in file! "+
                         kt.str() + " != " + pkt.str());
      
      // If no given kernel, pick from file
      if (!kt.ok())
        kt = pkt;

      // Set kernel
      if (!param.SetKernel(kt))
        return false;
    }

    // Check for manually given parameters.
    if (paramGiven("C")) 
      param.SetC(getParamFloat("C"), false);

    if (paramGiven("gamma"))
      param.SetGamma(getParamFloat("gamma"), false);
    
    if (paramGiven("log2c"))
      param.SetC(getParamFloat("log2c"), true);
    
    if (paramGiven("log2g"))
      param.SetC(getParamFloat("log2g"), true);
    
    if (paramGiven("linear_solver"))
      param.SetSolver(getParamInt("linear_solver", 0));

    if (paramGiven("scaling_from"))
      model->ScalingFrom(getParamFloat("scaling_from"));

    if (paramGiven("scaling_to"))
      model->ScalingTo(getParamFloat("scaling_to"));

    // Initialize problem, i.e. positive & negative examples to be
    // used for training.
    Problem* prob;
    setlist_t setlist;

    string readproblem = getParam("readproblem", "no");
    if (readproblem != "no") {
      prob = new Problem(svm_library, readproblem);
    } else {
      InitProblem(setlist, cls);
      prob = new Problem(svm_library, setlist);
    }

    if (IsAffirmative(getParam("writeproblem","no"))) {
      WriteLog(msg, "Writing problem to <", svm_fname+".dat", ">");
      prob->Write(svm_fname+".dat");
    }

    if (!prob->CheckParameter(param))
      return ShowError(msg, "Parameters not valid for problem.");

    // Do parameter selection if requested.
    if (!paramsel.empty())
      if (!DoParamSel(param, prob, paramsel))
        return ShowError(msg, "SVM::DoParamSel failed.");

    // Do training with given or selected parameters.
    WriteLog(msg, "Running svm_train with C=", ToStr(param.GetC()), 
             param.KernelHasGamma() ? " g="+ToStr(param.GetGamma()) : "");

    model->SetParameter(param);
    model->Train(prob);

    // Write model to file.
    model->Write(svm_fname);

    AddModel(svmname);

    Picsom()->PossiblyShowDebugInformation("After SVM creation");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::InitProblem(setlist_t& setlist, const ground_truth_set& cls) {
    string msg = "SVM::InitProblem() : ";

    size_t cs = cls.size();
    for (size_t i=0; i<cs; i++) {
      string classname = "class["+ToStr(i)+"]";
      if (cs==2)
        classname = i ? "negative" : "positive";
      cout << classname << "=" 
           << CheckDB()->GroundTruthSummaryStr(cls[i], true, false, false)
           << endl;
    }

    setlist.push_back(DataByIndices(cls[0].indices(1)));
    size_t nitems = setlist.back().Nitems();
    WriteLog(msg+ToStr(setlist.back().Nitems())+" vectors used for <"+
	     cls[0].label()+">");

    for (size_t i=1; i<cls.size(); i++) {
      setlist.push_back(DataByIndices(cls[i].indices(1)));
      nitems += setlist.back().Nitems();
      WriteLog(msg+ToStr(setlist.back().Nitems())+" vectors used for <"+
	       cls[i].label()+">");
    }

    string hkmmsg;
#ifdef USE_VLFEAT
    if (hkm) {
      setlist_t::iterator it = setlist.begin();
      for (; it != setlist.end(); it++)
        HomogeneousKernelMapEvaluate(*it);
      hkmmsg = "hkm="+hkm_name+",order="+ToStr(hkm_order);
    }
#endif
      
    model->RangeFromSetList(setlist);
    model->ResetZeroedCounter();

    setlist_t::iterator it = setlist.begin();
    for (; it != setlist.end(); it++)
      model->Scale(*it);

    if (preprocess != NO_PREPROCESS)
      for (setlist_t::iterator it = setlist.begin(); it != setlist.end();)
        DoPreprocess(*it++);

    if (normalization != NO_NORM)
      for (setlist_t::iterator it = setlist.begin(); it != setlist.end();)
        DoNormalize(*it++);

    size_t vl = setlist.back().VectorLength();

    int zz = model->ZeroedCounter();
    int zz_max = nitems*vl;
    if (zz) {
      double p = (double)zz/zz_max*100.0;
      stringstream ss;
      ss << msg << zz
         << " components were NaN, changed to 0!"
         << " This is " << p << "% of all components!";
      
      if (p >= 1.0)
        return ShowError(ss.str());
      else
        cout << "WARNING: " << ss.str() << endl;
    }

    WriteLog(msg+"initialised " + ToStr(nitems) + " " +
             (model->DoScaling() ? "scaled" : "unscaled") +
             (normalization == NO_NORM ? "" :
              (normalization == L1_NORM ? " L1" :
               normalization == L2_NORM ? " L2" :
               "") + string(" normalized")) + 
             (preprocess == NO_PREPROCESS ? "" :
              ", sqrt preprocessed") + 
             " vectors of length " + ToStr(vl)
	     + (hkmmsg!=""?", ":"") + hkmmsg
	     + ".");

    // cout << "Nitems() =";
    // it = setlist.begin();
    // for (; it != setlist.end(); it++)
    //   cout << " " << it->Nitems();
    // cout << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::DoParamSel(SVM::Parameter& param, SVM::Problem* prob,
                       const string& paramsel) {
    static const string msg = "SVM::DoParamSel() : ";
    
    bool use_gamma = param.KernelHasGamma();
    vector<string> ps_methods = SplitInSomething("+", false, paramsel);

    param_pair_t best; // first=C, second=gamma
    param_pair_t prev_best = best;
    double best_acc = -1.0;

    param.SetEps(0.1); // should be sufficient for parameter selection

    double c_begin, c_end, c_step;
    double g_begin, g_end, g_step;
    double cn=0, gn=0;
    int fold = atoi(getParam("fold","5").c_str());
    if (fold < 2)
      return ShowError(msg, "cross validation fold="+ToStr(fold)+
                       " not sensible.");

    srand(atoi(getParam("randseed","42").c_str()));

    ParamselCacheRead(GetFilename(".paramsel"));

    for (size_t m=0; m<ps_methods.size(); m++) {
      const string& method = ps_methods[m];

      if (method == "grid" || method == "line") {
        // simple grid search as in libsvm/tools/grid.py
        
        // set default values if first step, or previous was line search
        if (gn==0) {
          g_begin=3;  g_end=-15; g_step=-2;
        }
        
        if (cn==0) {
          c_begin=-5; c_end=15;  c_step=2;
          if (gn>0) { // this should be the case after a grid search
            gn=0;
            g_begin = best.second - g_step*3;
            g_end = best.second + g_step*3;
            if (g_begin > 3) {
                double diff = g_begin-3;
                g_begin = 3;
                g_end = g_end-diff;
            }
            if (g_end < -15) {
              double diff = -15-g_end;
              g_begin = g_begin+diff;
              g_end = -15;
            }              
          }
        }
        
        if (method == "line" && use_gamma) {
          if (m>0)
            return ShowError(msg, "line is a special case of grid search "
                             "which can only be as the first step!");
          c_begin=14; c_end=14; c_step=0;
        }
        
        if (m==0) {
          string cr_str = getParam("c_range");
          if (!cr_str.empty())
            SplitRangeString(cr_str, c_begin, c_step, c_end);
          
          string gr_str = getParam("g_range");
          if (!gr_str.empty())
            SplitRangeString(gr_str, g_begin, g_step, g_end);
        } else {
          if (cn > 0) {
            c_step = c_step/cn;
            c_begin = best.first - c_step*cn;
            c_end = best.first + c_step*cn;
          }
          
          if (gn > 0) {
            g_step = g_step/gn;
            g_begin = best.second - g_step*gn;
            g_end = best.second + g_step*gn;
          }
        }
        
        if (!use_gamma) {
          // dummy values for when kernel does not have gamma
          g_begin=1; g_end=1; g_step=0;
        }
        
        seq_t c_seq = Sequence(c_begin, c_end, c_step);
        seq_t g_seq = Sequence(g_begin, g_end, g_step);
        cn = floor((double)c_seq.size()/2.0);
        gn = floor((double)g_seq.size()/2.0);
        
        stringstream ss;
        ss.precision(5);
        ss << "Starting " << method << " search (fold=" << fold << ")";
        if (kernel_diff_func)
          ss << " [kernel_diff_func set]";
        if (!disable_cache)
          ss << " [cacheing set]";
        ss << ":" << endl
           << "  log2c=" << c_begin << "," << c_begin+c_step << ",...,"
           << c_end << " total " << c_seq.size() << endl
           << "  log2g=" << g_begin << "," << g_begin+g_step << ",...,"
           << g_end << " total " << g_seq.size() << endl;
        WriteLog(msg, ss.str());
        
        if (getParamBool("use_bug_2_78", false))
          best_acc = -1.0;

        prev_best=best;
        GridSearch(prob, param, c_seq, g_seq, fold,
		   best.first, best.second, best_acc);
        
        ss.clear();
        ss.str("");
        ss << "Selecting best parameters: log2c=" << best.first 
           << ", log2g=" << best.second;
        WriteLog(msg, ss.str());
      }
    }

    // Restore normal eps value for training.
    param.SetEps(1e-3);
    
    // Set the C, gamma to the best ones in parameter selection.
    param.SetC(best.first, true);
    param.SetGamma(best.second, true);

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void SVM::ParamselCacheRead(const string& fname) {
    const bool verbose = true;
    paramsel_cache_filename = fname;
    const string msg = "SVM::ParamselCacheRead("+paramsel_cache_filename+"): ";
    
    ifstream fp(paramsel_cache_filename.c_str());
    if (!fp)
      return;

    while (true) {
      double ce, ge, acc;
      fp >> ce;
      fp >> ge;
      fp >> acc;

      if (!fp)
        break;

      if (verbose)
        cout << msg << ce << "," << ge << " = " << acc << endl;

      if (ParamselCacheGet(ce, ge) >= 0)
        cout << "WARNING: " << paramsel_cache_filename
             << " has duplicate lines for " << ce << " " << ge << endl;

      paramsel_cache[make_pair(ce, ge)] = acc;
    }
    
    fp.close();
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::ParamselCacheWrite(double ce, double ge, double acc) {
    const string msg = "SVM::ParamselCacheWrite(): ";

    double cache_acc = ParamselCacheGet(ce, ge);

    if (cache_acc >= 0) {
      if (fabs(cache_acc-acc) > CACHE_DIFF) {
        ShowError(msg, "Cache contains old value "+ToStr(cache_acc)+" != "+
                  ToStr(acc)+"!!");
      }
      return;
    }

    if (paramsel_cache_filename.empty()) {
      ShowError(msg, "Paramsel cache file not set!");
      return;
    }

    ofstream fp(paramsel_cache_filename.c_str(), ios_base::app | ios_base::out);
    fp << ce << " " << ge << " " << acc << endl;
    fp.close();

    paramsel_cache[make_pair(ce, ge)] = acc;
  }

  /////////////////////////////////////////////////////////////////////////////

  double SVM::ParamselCacheGet(double ce, double ge) {
    map<paramsel_pair_t, double>::const_iterator it;

    for (it = paramsel_cache.begin(); it != paramsel_cache.end() ; it++) {
      const paramsel_pair_t& p = it->first;
      if (fabs(p.first - ce) < CACHE_DIFF && fabs(p.second - ge) < CACHE_DIFF)
        return it->second;
    }

    return -1.0;
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::GridSearch(Problem* prob, Parameter& param, 
                       seq_t c_seq, seq_t g_seq, int fold, 
                       double& best_c, double& best_g, double &best_acc) {
    Tic("GridSearch");
    string msg = "SVM::GridSearch(): ";

    int verbose = 1;
    c_seq = PermuteSequence(c_seq);
    g_seq = PermuteSequence(g_seq);

    seq_param_t sp = ParameterSequence(c_seq, g_seq);

    int l = prob->GetL();

    Target target(l, svm_library);

    int cn = prob->NrClass();
    double** prob_estim = new double*[l];
    for (int i=0; i<l; i++)
      prob_estim[i] = new double[cn];

    cout.precision(5);

    for (seq_param_t::const_iterator i=sp.begin(); i!=sp.end(); i++) {
      double ce = i->first;
      double ge = i->second;

      param.SetC(ce, true);
      param.SetGamma(ge, true);

      double acc = 1.0;

      bool using_cache = false;
      double cached_acc = ParamselCacheGet(ce, ge);

      if (cached_acc >= 0) {
        acc = cached_acc;
        using_cache = true;
      } else {
        if (param.GetProbability()) {
          CrossValidation(prob, param, fold, target, prob_estim); 

          int k = 0;
          int kmax = cn;

          if (cn == 2) {
            // FIXME: this is not very general way, but hard to get
            // labels from here...
            k = 0;
            kmax = 1;
          }
        
          acc = 0.0;
          for (; k<kmax; k++) {
            multimap<double,int> pp;
            for (int i=0; i<l; i++)
              pp.insert(make_pair(prob_estim[i][k],i));
        
            DoubleVector hits(l);
            int i=0;
            for (multimap<double,int>::const_reverse_iterator it=pp.rbegin();
                 it!=pp.rend() && i<l; it++) {
              bool true_pos = prob->GetY(it->second) == 1.0;
              hits[i++] = true_pos ? 1.0 : 0.0;
              if (verbose>=2 && i<10)
                cout << i << ": (" << it->first << ") " << (true_pos?"X":"")
                     << endl;
            }
        
            acc += Analysis::AveragePrecision(hits, l, 0, NULL);
          }
          if (kmax > 1)
            acc /= (double)kmax;
        } else {
          // Plain accuracy, when param.probability == 0
          CrossValidation(prob, param, fold, target, NULL);
        
          int total_correct = 0;
          for (int i=0; i<l; i++) 
            if (target.Get(i) == prob->GetY(i))
              total_correct++;
          acc = (double)total_correct/(double)l;
        }
      }
      
      ParamselCacheWrite(ce, ge, acc);

      if (verbose) {
        WriteLog("[GridSearch] log2c="+ToStr(ce)+", log2g="+ToStr(ge)+", ",
                 param.GetProbability() ? "average precision":"accuracy",
                 "="+ToStr(acc)+(using_cache ? " [cached]":""));
      }

      if (acc>best_acc || (acc==best_acc && ge==best_g && ce<best_c)) {
        best_acc = acc;
        best_c=ce;
        best_g=ge;
      }
    }

    for (int i=0; i<l; i++)
      delete [] prob_estim[i];
    delete [] prob_estim;

    Tac("GridSearch");
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::CrossValidation(Problem* prob, Parameter& param, int fold,
                            Target& target, double** prob_estim) {
    if (svm_library == LIB_LINEAR)
      cross_validation(prob->GetLinear(), param.GetLinear(), fold,
                       target.GetInteger(), prob_estim);
    else if (svm_library == LIB_SVM)
      svm_cross_validation(prob->GetSVM(), param.GetSVM(), fold,
                           target.GetDouble(), prob_estim);
    else if (svm_library == PM_SVM)
      prob->GetPmSVM()->CrossValidation(fold, param.GetC(), param.GetP()); //fixme
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::seq_t SVM::Sequence(double sbegin, double sendin, double sstep) {
    seq_t seq;

    double send = sendin+sstep/2; // added 2013-06-25

    double s = sbegin;
    while (true) {
      if (sstep > 0 && s > send) break; 
      if (sstep < 0 && s < send) break; 
      seq.push_back(s);
      if (sstep == 0) break;
      s += sstep;
    }
    return seq;
  }

  /////////////////////////////////////////////////////////////////////////////

// These can be used for debugging seq_t seq_param_t stuff...
//
//   void PrintSequence(seq_t seq) {
//     for (seq_t::const_iterator i = seq.begin(); i!= seq.end(); i++) {
//       cout << *i << " ";
//     }
//     cout << endl;
//   }
// 
   /////////////////////////////////////////////////////////////////////////////
// 
//   void PrintParameterSequence(seq_param_t sp, const string& msg="") {
//     if (!msg.empty())
//       cout << msg << " ";
//     for (seq_param_t::const_iterator i=sp.begin(); i!=sp.end(); i++) {
//       cout << "(" << i->first << ", " << i->second << "), ";
//     }
//     cout << endl;
//   }
// 
  /////////////////////////////////////////////////////////////////////////////

  SVM::seq_t SVM::PermuteSequence(seq_t seq) {
    seq_t::const_iterator a = seq.begin();
    seq_t::const_iterator b = seq.end();

    int n = distance(a,b);
    if (n <= 1) 
      return seq;

    int mid = (int)n/2;
    seq_t left = PermuteSequence(seq_t(a, a+mid));
    seq_t right = PermuteSequence(seq_t(a+mid+1, b));

    seq_t ret = seq_t(1, *(a+mid));
    seq_t::iterator l = left.begin();
    seq_t::iterator r = right.begin();
    while (l!=left.end() || r!=right.end()) {
      if (l!=left.end()) ret.push_back(*l++);
      if (r!=right.end()) ret.push_back(*r++);
    }
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::seq_param_t SVM::ParameterSequence(const seq_t& c_seq, 
                                          const seq_t& g_seq) {
    seq_param_t sp;

    size_t nr_c = c_seq.size();
    size_t nr_g = g_seq.size();

    size_t i=0, j=0;
    while (i < nr_c || j < nr_g) {
      if ((double)i/(double)nr_c < (double)j/(double)nr_g) {
        // increase c resolution
        for (size_t k=0; k<j; k++)
          sp.push_back(param_pair_t(c_seq[i], g_seq[k]));
        i++;
      } else {
        // increase g resolution
        for (size_t k=0; k<i; k++)
          sp.push_back(param_pair_t(c_seq[k], g_seq[j]));
        j++;
      }
    }
    return sp;
  }

#ifdef USE_VLFEAT

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::InitialiseHomogeneousKernelMap(const string& kname, int order) {
    const string msg = "SVM::InitialiseHomogeneousKernelMap("+kname+"): ";

    bool debug = DebugSVM();

    if (hkm) 
      vl_homogeneouskernelmap_delete(hkm);

    hkm = NULL;
    hkm_order = 0;

    if (kname.empty())
      return false;

    VlHomogeneousKernelType hkt;
    if (kname == "intersection")
      hkt = VlHomogeneousKernelIntersection;
    else if (kname == "chisquared" || kname == "chi2")
      hkt = VlHomogeneousKernelChi2;
    else if (kname == "js" || kname == "JS")
      hkt = VlHomogeneousKernelJS;
    else
      return ShowError(msg, "unknown kernel name!");

    hkm_name = kname;
    hkm_order = order; // hkm approximation order

#ifndef __MINGW32__
    hkm = vl_homogeneouskernelmap_new(hkt, 1, hkm_order, -1, 
                                      VlHomogeneousKernelMapWindowRectangular);
#endif // __MINGW32__

    if (debug)
      WriteLog(msg, "initialised");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::HomogeneousKernelMapEvaluate(FloatVector* fv) {
    size_t len = fv->Length();

    // Make a copy of old values
    FloatVector vold = *fv;

    int hkm_mult = 2*hkm_order+1;
    // Lengthen vector to fit new values
    fv->Lengthen(len*hkm_mult);

    // Get pointers to actual float data
    float* fp = (float*)vold;
    float* res = (float*)(*fv);

    // Just do it!
    for (size_t i=0; i<len; i++) {
      // Unfortunately we need to zero the result values, since
      // vl_homogeneouskernelmap_evaluate_f doesn't always fill it
      for (int j=0; j<hkm_mult; j++)
        res[j] = 0.0;

#ifndef __MINGW32__
      vl_homogeneouskernelmap_evaluate_f(hkm, res, 1, *fp++);
#endif // __MINGW32__
      res += hkm_mult;
    }
    
    // cout << endl << "SVM::HomogeneousKernelMapEvaluate() just changed " << endl;
    // vold.Dump(Simple::DumpLong);
    // cout << " into " << endl;
    // fv->Dump(Simple::DumpLong);
    // cout << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::HomogeneousKernelMapEvaluate(FloatVectorSet& fvs) {
    for (int i=0; i<fvs.Nitems(); i++)
      HomogeneousKernelMapEvaluate(fvs.Get(i));
  }

#endif

  /////////////////////////////////////////////////////////////////////////////

  void SVM::DoNormalize(FloatVector* fv) {
    if (normalization == L1_NORM)
      fv->Normalize(1);
    else if (normalization == L2_NORM)
      fv->Normalize(2);
  }

  /////////////////////////////////////////////////////////////////////////////
  
  void SVM::DoNormalize(FloatVectorSet& fvs) {
    for (int i=0; i<fvs.Nitems(); i++)
      DoNormalize(fvs.Get(i));
  }    

  /////////////////////////////////////////////////////////////////////////////
  
  void SVM::DoPreprocess(FloatVector* fv) {
    Tic("DoPreprocess");
    //    if (preprocess == SQRT) ...
    float* fp = *fv;
    for (int i=0; i<fv->Length(); i++) {
      float sign = 1.0;
      if (*fp < 0.0) 
	sign = -1.0;
      *fp = sign*sqrtf(fabs(*fp));
      
      // was this until 2013-10-10 - markus
      //*fp = sqrtf(*fp > 0.0 ? *fp : 0.0); 
      fp++;
    }       
    Tac("DoPreprocess");
  }

  /////////////////////////////////////////////////////////////////////////////
  
  void SVM::DoPreprocess(FloatVectorSet& fvs) {
    for (int i=0; i<fvs.Nitems(); i++)
      DoPreprocess(fvs.Get(i));
  }    

  /////////////////////////////////////////////////////////////////////////////

  double SVM::PredictProb(const FloatVector& v, const string& type, int lab) {
    const string msg = "SVM::PredictProb(): ";

    const int vl = model->MinVectorLength();
    int lcomp = v.Length();

#ifdef USE_VLFEAT
    int hkm_mult = 2*hkm_order+1;
    lcomp *= hkm_mult;
#endif

    if (svm_library!=LIB_SVM && lcomp!=vl) {
      ShowError(msg+"Length of vector to predict (" + ToStr(lcomp) + ") "
		"must equal length of model vectors: " + ToStr(vl) + ".");
      return 0.0;
    }

    if (lcomp < vl) {
      ShowError(msg+"Length of vector to predict (" + ToStr(lcomp) + ") "
		"must be >= largest index in first SVM: " + ToStr(vl) + ".");
      return 0.0;
    }    

    bool tictac_w_name = false;
    string tname = "PredictProb";
    if (tictac_w_name)
      tname += "<"+Name()+">";
    Tic(tname.c_str());

    vector<double> vd = PredictProbMulti(v, type);

    if (vd.size() == 2) {
      int label[2];
      model->GetLabels(&label[0]);
      lab = label[0]==1 ? 0 : 1;
    }

    Tac(tname.c_str());

    double ret = vd[lab];
    if (DebugSVM()>1) {
      size_t ml = 5;
      cout << msg << FeatureFileNameWithExtra() << " : #" << v.Number()
	   << " " << v.Label();
      for (size_t i=0; i<(size_t)v.Length()&&i<ml; i++)
	cout << " " << v[i];
      if ((size_t)v.Length()>ml)
	cout << " ...";
      cout << " = " << ret << endl;
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<double> SVM::PredictProbMulti(const FloatVector& vin,
				       const string& type) {
    string msg = "SVM::PredictProbMulti(): ";
    vector<double> ret;

    if (!model || !model->HasModel()) {
      ShowError(msg, "a model needs to be loaded first!");
      return ret;
    }
    
    bool values = type.substr(0,3) == "val";
    if (!values && !model->CheckProbability()) {
      ShowError(msg, "model does not contain required information to "
                "do probability estimates.");
      return ret;
    }

    FloatVector v(vin);

#ifdef USE_VLFEAT
    if (hkm)
      HomogeneousKernelMapEvaluate(&v);
#endif
   
    int cn = model->GetClassNr();

    double* prob_estimates = new double[cn];
    model->Scale(v);

    if (preprocess != NO_PREPROCESS)
      DoPreprocess(&v);

    if (normalization != NO_NORM)
      DoNormalize(&v);

    svm_node* x_space = ConvertVector(v);

    if (values)
      model->PredictValues(x_space, prob_estimates);
    else
      model->PredictProbability(x_space, prob_estimates);

    delete [] x_space;

    for (int i=0; i<cn; i++) {
      double val = prob_estimates[i];

      if (type == "valueslog" || type == "vallog")
        val = 1.0/(1.0+exp(-val));
      ret.push_back(val);
    }

    delete [] prob_estimates;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  struct svm_node* SVM::ConvertVector(const FloatVector& v) {
    int l = v.Length();
    struct svm_node* x_space = new svm_node[1+l];
    
    int i;
    for (i=0; i<l; i++) {
      x_space[i].index = i+1;
      x_space[i].value = v[i];
    }
    x_space[l].index = -1;
    return x_space;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::AddModel(const string& n) {
    DestroyModel(n, false);
    /*classifier_data *c = */AddClassifier("svm", n);
 
    WriteLog("Added SVM model ["+n+"]");
 
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::DestroyModel(const string& n, bool angry) {
    classifier_data *c = FindClassifier(n);
    if (!c) 
      return !angry ||
        ShowError("SVM::DestroyModel("+n+") : does not exist");

    delete model;
    model = NULL;

    DestroyClassifier(c);
    WriteLog("Destroyed SVM model ["+n+"]");

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool SVM::LoadModel(const string& mfilein, const string& svmlib,
                      const string& pos, const string& neg) {
    string mfile = mfilein, msg = "SVM::LoadModel("+mfile+","+svmlib+") : ";

    if (is_ready)
      SimpleAbort();

    if (!SetLibrary(svmlib))
      return false;

    DestroyModel("", false);

    if (!FileExists(mfile)) {
      string mf = db->ExpandPath("svms", mfile);

      if (!FileExists(mf) && db->IsLocalDiskDb())
	db->RootlessDownloadFile(mf, true);

      if (FileExists(mf))
	mfile = mf;
    }

    model = new Model(this);
    if (!model->Load(mfile)) {
      DestroyModel("", false);
      return ShowError(msg+"Load("+mfile+") failed.");
    }

    if (pos!="") {
      string filebase = mfile;

      size_t pp = filebase.find(".svm");
      if (pp != string::npos)
        filebase.erase(pp);

      if (FileExists(filebase+".pre"))
	ReadPreToCDF(filebase+".pre", false, "", "");
      if (FileExists(filebase+"-train.pre"))
	ReadPreToCDF(filebase+"-train.pre", true, pos, neg);
    }

    is_ready = true;

    Picsom()->PossiblyShowDebugInformation("After loading SVM model");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::ReadPreToCDF(const string& fname, bool isdevel, const string& pos,
			 const string& neg) {
    const string msg = "SVM::ReadPreToCDF("+fname+","
      +ToStr(isdevel)+") : ";

    ifstream pre(fname.c_str());
    if (!pre)
      return ShowError(msg, "could not open file!");

    Tic("ReadPreCalculatedFile");

    ground_truth dev_pos, dev_neg;
    if (isdevel) {
      dev_pos = db->GroundTruthExpression(pos);
      dev_neg = db->GroundTruthExpression(neg);
    }

    size_t linecount = 0;
    
    list<float>& cdfx = isdevel ? cdf_devel : cdf_test;

    for (;;) {
      string line;
      getline(pre, line);
      if (!pre)
	break;

      if (line[0]=='#')
        continue;

      vector<string> parts = SplitInSpaces(line);
      float v = atof(parts[0].c_str());
      cdfx.push_back(v);

      string label = parts[1];
      size_t cp = label.find(':');  // trecvid2011 -> svmdb hack
      if (cp!=string::npos)
	label.erase(cp);
      int idx = db->LabelIndex(label);
      if (idx<0)
	return ShowError(msg+"label <"+label+"> unknown");

      if (isdevel) {
	if (size_t(idx)>=dev_pos.size() || size_t(idx)>=dev_neg.size())
	  return ShowError(msg+"index "+ToStr(idx)+" unknown");

	if (dev_pos[idx]==1)
	  cdf_devel_pos.push_back(v);
	if (dev_neg[idx]==1)
	  cdf_devel_neg.push_back(v);
      }

      linecount++;
    }

    cdfx.sort();

    string addtxt;
    if (isdevel) {
      cdf_devel_pos.sort();
      cdf_devel_neg.sort();
      addtxt = ", "+ToStr(cdf_devel_pos.size())+" positive and "
	+ToStr(cdf_devel_neg.size())+" negative samples";
    }

    Tac("ReadPreCalculatedFile");

    WriteLog(msg, "Read precalculated file <"+ShortFileName(fname)+"> " +
	     ToStr(linecount) + " lines"+addtxt);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::State::Initialize() {
    objlist = vector<ObjectList>(1);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Model::Model(SVM* parent) {
    svm = parent;

    lmodel = NULL;
    smodel = NULL;
    pmodel = NULL;

    svm_library = svm->GetLibrary();
    param.SetLibrary(svm_library);
    min_vec_len = -1;
    ResetZeroedCounter();

    do_scaling = false;
    scaling_from = -1.0;
    scaling_to = 1.0;
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Model::~Model() {
    if (lmodel)
      free_and_destroy_model(&lmodel);
    if (smodel)
      svm_free_and_destroy_model(&smodel);
    if (pmodel)
      delete pmodel;

    lmodel = NULL;
    smodel = NULL;
    pmodel = NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  int svm_last_node_index(struct svm_model* m) {
    int i = 0;
    svm_node* x_space = m->SV[0];
    while (x_space[i].index != -1) { i++; }
    return x_space[i-1].index;
  }

  /////////////////////////////////////////////////////////////////////////////

  int SVM::Model::MinVectorLength() {
    if (min_vec_len == -1)
      min_vec_len = 
        svm_library == LIB_LINEAR ? lmodel->nr_feature :
        svm_library == PM_SVM ? pmodel->nr_feature :
        svm_last_node_index(smodel);
    return min_vec_len;
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Model::PredictValues(const struct svm_node *x, double* p) {
    if (svm_library == LIB_LINEAR)
      predict_values(lmodel, x, p);
    else if (svm_library == LIB_SVM)
      svm_predict_values(smodel, x, p); 
    else 
      ShowError("PredictValues not implemented for this library.");
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Model::PredictProbability(const struct svm_node *x, double* p) {
    if (svm_library == LIB_LINEAR) {
      svm->Tic("predict_probability");
      predict_probability(lmodel, x, p); 
      svm->Tac("predict_probability");
    } else if (svm_library == LIB_SVM) {
      svm->Tic("svm_predict_probability");
      svm_predict_probability(smodel, x, p);
      svm->Tac("svm_predict_probability");
    } else
      ShowError("PredictProbability not implemented for this library.");
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Model::RangeFromSetList(const setlist_t& setlist) {
    setlist_t::const_iterator it = setlist.begin();
    FloatVectorSet all = *it;

    while (++it != setlist.end())
      all.AppendCopy(*it);

    range_min = all.MinimumComponents();
    range_max = all.MaximumComponents();
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Model::Train(SVM::Problem* prob) {
    if (svm_library == LIB_LINEAR) {
      if (lmodel)
        free_and_destroy_model(&lmodel);
      lmodel = train(prob->GetLinear(), param.GetLinear());
    } else if (svm_library == LIB_SVM) {
      if (smodel)
        svm_free_and_destroy_model(&smodel);
      smodel = svm_train(prob->GetSVM(), param.GetSVM());
    } else if (svm_library == PM_SVM) {
      if (pmodel)
        delete pmodel;

      pmodel = new PmSVM::model;
      prob->GetPmSVM()->Train(*pmodel, param.GetC(), param.GetP());
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Model::SetParameter(const Parameter& p) {
    param = p;
    SyncParameter();
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Model::SyncParameter() {
    // Changes made in param need to be "synced" to the internal param
    // of the model...
    if (smodel && param.GetSVM())
      smodel->param = *(param.GetSVM());
    if (lmodel && param.GetLinear())
      lmodel->param = *(param.GetLinear());
  }    

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Model::Write(const string& mfname) {
    if (svm_library == LIB_LINEAR)
      save_model(mfname.c_str(), lmodel);
    else if (svm_library == LIB_SVM)
      svm_save_model(mfname.c_str(), smodel);
    else if (svm_library == PM_SVM)
      ShowError("Write not implemented yet for PmSVM."); //fixme

    ofstream mof(mfname.c_str(), ios::app);

    if (do_scaling) {
      mof << "#picsom svmscale=" << scaling_from << ":" << scaling_to;
      for (int i=0; i<range_min.Length(); i++)
        mof << "," << i+1 << ":" << range_min[i] << ":" << range_max[i];
      mof << endl;
    }

    if (param.KernelHasGamma())
      mof << "#picsom gamma="  << param.GetGamma() << endl;

    mof << "#picsom C="      << param.GetC() << endl;

    mof << "#picsom target=" << svm->FeatureTargetString() << endl;

    mof << "#picsom library=" << svm->LibraryString() << endl;

    if (svm_library == LIB_SVM)
      mof << "#picsom kernel_type="
          << param.GetKernel().str() << endl;

    if (svm_library == LIB_LINEAR)
      mof << "#picsom linear_solver=" << param.GetSolver() << endl;

    if (svm_library == PM_SVM)
      mof << "#picsom pmsvm_p=" << param.GetP() << endl;

    if (svm->Normalization() != NO_NORM)
      mof << "#picsom normalization=" << (int)svm->Normalization() << endl;

    if (svm->Preprocessing() != NO_PREPROCESS)
      mof << "#picsom preprocess_components=" << (int)svm->Preprocessing() << endl;

#ifdef USE_VLFEAT
    string kname = svm->HomogeneousKernelMap();
    if (!kname.empty()) {
      mof << "#picsom homker=" << kname << endl;
      mof << "#picsom homkerorder=" << svm->hkm_order << endl;
    }
#endif

    mof.close();
 
    svm->WriteLog("Created SVM classifier <"+mfname+">");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Model::Load(const string& mfile) {
    string msg = "SVM::Model::Load("+mfile+") : ";

    bool debug = DebugSVM();

    svm->Tic("SVM::Model::Load");

    list<pair<string,string> > extra = ReadExtraParameters(mfile);

    do_scaling = false;

    Parameter& p = param;

    string homker, homkerorder = "3";

    bool ok = true;
    for (list<pair<string,string> >::const_iterator i=extra.begin();
         i!=extra.end(); i++) {
      int r = 0;

      if (i->first == "svmscale") {
        do_scaling = true;
        vector<string> sp = SplitInCommas(i->second);
        vector<string> sr = SplitInSomething(":",false,sp[0]);
        scaling_from = atof(sr[0].c_str());
        scaling_to = atof(sr[1].c_str());

        range_min.Lengthen(sp.size()-1); 
        range_max.Lengthen(sp.size()-1); 
        range_min.Zero(); range_max.Zero();

        for (size_t k=1; k<sp.size(); k++) {
          sr = SplitInSomething(":",false,sp[k]);
          int idx = atoi(sr[0].c_str())-1;
          range_min[idx] = atof(sr[1].c_str());
          range_max[idx] = atof(sr[2].c_str());
        }
      } else if (i->first == "C") {
        p.SetC(atof(i->second.c_str()), false);
      } else if (i->first == "gamma") {
        p.SetGamma(atof(i->second.c_str()), false);
      } else if (i->first == "kernel_type") {
        p.SetKernel(i->second);
      } else if (i->first == "linear_solver") {
        p.SetSolver(atoi(i->second.c_str()));
      } else if (i->first == "normalization") {
        svm->Normalization((norm_t)atoi(i->second.c_str()));
      } else if (i->first == "preprocess_components") {
        svm->Preprocessing((preprocess_t)atoi(i->second.c_str()));
      } else if (i->first == "homker") {
        // svm->InitialiseHomogeneousKernelMap(i->second);  // was until 2013-10-21
	homker = i->second;
      } else if (i->first == "homkerorder") {
	homkerorder = i->second;
      } else if (i->first == "library") {
        if (i->second != svm->LibraryString())
          return ShowError(msg, "SVM library in file ["+i->second+"] differs "
                           "from library in class ["+svm->LibraryString()+"]");
      } else if (i->first == "linear") {
	if ((i->second == "yes" && svm->GetLibrary()!=LIB_LINEAR) ||
	    (i->second != "yes" && svm->GetLibrary()==LIB_LINEAR)) 
          return ShowError(msg, "interpreting linear=yes mismatch");

      } else if (!svm->Interpret(*i, r) || !r)
        ok = false;

      if (!ok) {
        ShowError(msg+"interpreting "+i->first+"="+i->second+" failed");
      } else if (debug) {
        size_t maxl = 80;
        string v = i->second.substr(0, maxl);
        if (i->second.length()>maxl)
          v += " ...";
	svm->WriteLog("  with "+i->first+" = "+v);
      }
    }
    
    if (!ok)
      return false;

    if (homker!="") {
#ifdef USE_VLFEAT
      svm->InitialiseHomogeneousKernelMap(homker, atoi(homkerorder.c_str()));
#else
      return ShowError(msg, "homker not available without vlfeat");
#endif // USE_VLFEAT
    }

    if (svm_library == LIB_LINEAR) {
      svm->Tic("load_model");
      lmodel = load_model(mfile.c_str());
      svm->Tac("load_model");
    } else if (svm_library == LIB_SVM) {
      svm->Tic("svm_load_model");
      smodel = svm_load_model(mfile.c_str());
      svm->Tac("svm_load_model");
    } else if (svm_library == PM_SVM) {
      svm->Tic("pmsvm_load_model");
      ShowError("pmsvm load model not implemented yet.");
      //      pmodel = ;
      svm->Tac("pmsvm_load_model");
    }

    if (svm_library == LIB_LINEAR && !lmodel)
      return ShowError(msg+"load_model() failed with <"+mfile+">");

    if (svm_library == LIB_SVM && !smodel)
      return ShowError(msg+"svm_load_model() failed <"+mfile+">");

    SyncParameter();

    stringstream ss;
    ss << "Loaded " << LibraryString(svm_library)
       << " model <"+svm->ShortFileName(mfile)+">";
    if (homker!="")
      ss << " with homker=" << homker
	 << " order=" << homkerorder;
    ss << " dimensionality=" << MinVectorLength();

    svm->WriteLog(ss.str());

    svm->Tac("SVM::Model::Load");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string> >
  SVM::Model::ReadExtraParameters(const string& mfile) {
    list<pair<string,string> > m;

    ifstream is(mfile.c_str());
    while (is) {
      string line;
      getline(is, line);
      if (line.find("#picsom ")==0)
        m.push_back(SplitKeyEqualValueNew(line.substr(8)));
    }

    return m;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool SVM::Model::Scale(FloatVector& v) {
    if (!do_scaling)
      return true;

    string msg = "SVM::Scale(): ";
    if (range_min.Length() == 0) 
      return ShowError(msg, "No scaling given!");

    int vl = v.Length();

    // sanity check
    if (vl != range_min.Length() || vl != range_max.Length())
      return ShowError(msg, "Lengths differ!");
    float diff = scaling_to-scaling_from;
    for (int i=0; i<vl; i++) {
      float range = range_max[i]-range_min[i];
      v[i] = diff*(v[i]-range_min[i])/range + scaling_from;
      if (isnan(v[i])) {
        v[i] = 0.0;
        scale_zeroed++;
      }
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Parameter::Parameter() {
    // kernel_type = kernel_t::NO_KERNEL;
    svm_library = LIB_SVM;
    
    // Set default values for linear SVM
    lparam.solver_type = L2R_LR;

    lparam.C = 1;
    lparam.eps = 1e-3; // 0.1 for paramsel ?

    lparam.nr_weight = 0;
    lparam.weight_label = NULL;
    lparam.weight = NULL;

    // Set default values for kernel SVM
    sparam.svm_type = C_SVC;
    sparam.kernel_type = KT_CALLBACK;
    sparam.callback = myKernelCallback;

    sparam.degree = 3;
    sparam.gamma = 0;        // 1/k
    sparam.coef0 = 0;
    sparam.nu = 0.5;
    sparam.cache_size = 100;
    sparam.C = 1;
    sparam.eps = 1e-3; // 0.1 for paramsel ?
    sparam.p = 0.1;
    sparam.shrinking = 1;
    sparam.probability = 0;
    sparam.nr_weight = 0;
    sparam.weight_label = NULL;
    sparam.weight = NULL;

    pmsvm_p = -1; // -1 = chi-square, -8 = hik
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Parameter::Parameter(const Parameter& p) {
    kernel_type = p.kernel_type;
    svm_library = p.svm_library;
    lparam = p.lparam;
    sparam = p.sparam;

    int nr = 0;
    if (svm_library == LIB_LINEAR)
      nr = lparam.nr_weight;
    else if (svm_library == LIB_SVM)
      nr = sparam.nr_weight;

    if (nr <= 0)
      return;

    int* wl = (int*)malloc(sizeof(int)*nr);
    double* w = (double*)malloc(sizeof(double)*nr);

    if (svm_library == LIB_LINEAR) {
      memcpy(wl, p.lparam.weight_label, sizeof(int)*nr);
      memcpy(w, p.lparam.weight, sizeof(double)*nr);

      lparam.nr_weight = nr;
      lparam.weight_label = wl;
      lparam.weight = w;
    } else {
      memcpy(wl, p.sparam.weight_label, sizeof(int)*nr);
      memcpy(w, p.sparam.weight, sizeof(double)*nr);
      
      sparam.nr_weight = nr;
      sparam.weight_label = wl;
      sparam.weight = w;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Parameter::~Parameter() {
    if (lparam.weight)
      destroy_param(&lparam);
    if (sparam.weight)
      svm_destroy_param(&sparam);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Parameter::SetKernel(SVM::kernel_t kt) {
    static const string msg = "SVM::Parameter::SetKernel(): ";

    kernel_t::kernel_func_t kf = kt.func();

    if (kf == kernel_t::EUCL)
      kernel_diff_func = diff_eucl;
    else if (kf == kernel_t::INT)
      kernel_diff_func = diff_int;
    else if (kf == kernel_t::CHI2)
      kernel_diff_func = diff_chi2;
    else if (kf == kernel_t::JS)
      kernel_diff_func = diff_js;
    else if (kf == kernel_t::NO_KERNEL)
      return ShowError(msg, "No kernel set for libsvm!");
    else
      return ShowError(msg, "Unsupported kernel type.");

    exp_type_kernel = kt.uses_exp();
    kernel_type = kt;

    if (DebugSVM())
      cout << TimeStamp() << msg << "Setting kernel to [" << kt.str() << "]"
	   << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Parameter::SetWeights(double cweight, int nr) {
    int* wl = (int*)malloc(sizeof(int)*nr);
    double* w = (double*)malloc(sizeof(double)*nr);

    if (nr == 2) {
      wl[0] = 1;  w[0] = cweight;
      wl[1] = -1; w[1] = 1.0;
    } else {
      // FIXME: cweight should be double[] ...
      for (int i=0; i<nr; i++) {
        wl[i] = i;
        w[i] = cweight;
      }
    }

    if (svm_library == LIB_LINEAR) {
      lparam.nr_weight = nr;
      lparam.weight_label = wl;
      lparam.weight = w;
    } else if (svm_library == LIB_SVM) {
      sparam.nr_weight = nr;
      sparam.weight_label = wl;
      sparam.weight = w;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void SVM::Problem::InitMembers(library_t lt) {
    svm_library = lt;
    lprob.y = NULL; lprob.x = NULL;
    sprob.y = NULL; sprob.x = NULL;
    x_space = NULL;
    pprob.y = NULL;
    pprob.indexes = NULL;
    pprob.values = NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Problem::Problem(library_t lt, const string& filename) {
    InitMembers(lt);
    Read(filename);
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Problem::Problem(library_t lt, const setlist_t& sl) {
    InitMembers(lt);

    nr_class = sl.size();

    size_t sumn = 0;
    setlist_t::const_iterator it = sl.begin();
    for (; it != sl.end(); it++)
      sumn += it->Nitems();

    SetL(sumn);
    int l = GetL();

    size_t vl = sl.back().VectorLength();

    InitXY();

    if (svm_library == PM_SVM) {
      double bias = 0;

      // count total number of components
      int allocated = 0;
      it = sl.begin();
      for (; it != sl.end(); it++)
        for (int m=0; m<it->Nitems(); m++) {
          const FloatVector& v = *(it->Get(m));
          for (int k=0; k<v.Length(); k++)
            if (v[k])
              allocated++;
          allocated++;
        }

      if (bias>0) allocated += l;
      pprob.allocated = allocated;
      
      pprob.index_buf = new int[allocated];
      pprob.value_buf = new double[allocated];
      pprob.y = new int[l];
      pprob.indexes = new int*[l];
      pprob.values = new double*[l];

      int i = 0; // index over all vectors
      int j = 0; // index into _buf
      int ll = 0; // label counter, 0 .. nr_class-1
      it = sl.begin();
      for (; it != sl.end(); it++) {
        for (int m=0; m<it->Nitems(); m++) {
          int label = ll;
          if (nr_class == 2)
            label = ll ? -1 : 1;

          //          SetXY(i++, &x_space[j], label);
          pprob.indexes[i] = &pprob.index_buf[j];
          pprob.values[i] = &pprob.value_buf[j];
          pprob.y[i] = label;
 
          const FloatVector& v = *(it->Get(m));
          for (int k=0; k<v.Length(); k++) {
            if (v[k]) {
              pprob.index_buf[j] = k+1 + (bias > 0);
              pprob.value_buf[j] = v[k];

              if (pprob.value_buf[j] < 0) 
                pprob.value_buf[j] = 0.001;
              
              // x_space[j].index = k+1;
              // x_space[j].value = v[k];
              j++;
            }
          }
          pprob.index_buf[j++] = -1;
          //          x_space[j++].index = -1;

          i++;
        }
        ll++;
      }
      return;
    }
 
    x_space = new svm_node[l*(1+vl)];
     
    int i = 0; // index over all vectors
    int j = 0; // index into x_space, i.e. each vector component 
    int ll = 0; // label counter, 0 .. nr_class-1

    it = sl.begin();
    for (; it != sl.end(); it++) {
      for (int m=0; m<it->Nitems(); m++) {
        int label = ll;
        if (nr_class == 2)
          label = ll ? -1 : 1;

        SetXY(i++, &x_space[j], label);
 
        const FloatVector& v = *(it->Get(m));
        for (int k=0; k<v.Length(); k++) {
          if (v[k]) {
            x_space[j].index = k+1;
            x_space[j].value = v[k];
            j++;
          }
        }
        x_space[j++].index = -1;
      }
      ll++;
    }

    if (svm_library == LIB_LINEAR) {
      // FIXME: always sets to -1 at the moment
      lprob.bias = -1;
      int max_index = vl;

      // copied from liblinear train.C
      if (lprob.bias >= 0) {
        lprob.n = max_index+1;
        for (int i=1; i<lprob.l; i++)
          (lprob.x[i]-2)->index = lprob.n; 
        x_space[j-2].index = lprob.n;
      } else
        lprob.n=max_index;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Problem::~Problem() {
    if (lprob.y)
      delete [] lprob.y;
    if (lprob.x)
      delete [] lprob.x;
    if (sprob.y)
      delete [] sprob.y;
    if (sprob.x)
      delete [] sprob.x;
  }

  /////////////////////////////////////////////////////////////////////////////

  const svm_node* SVM::Problem::GetX(int i) {
    if (svm_library == LIB_LINEAR)
      return lprob.x[i];
    else if (svm_library == LIB_SVM)
      return sprob.x[i];
    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  double SVM::Problem::GetY(int i) const {
    if (svm_library == LIB_LINEAR)
      return (double)lprob.y[i];
    else if (svm_library == LIB_SVM)
      return sprob.y[i];
    return 0.0;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  void SVM::Problem::InitXY() {
    int l = GetL();
    if (svm_library == LIB_LINEAR) {
      lprob.y = new int[l];
      lprob.x = new svm_node*[l];
    } else if (svm_library == LIB_SVM) {
      sprob.y = new double[l];
      sprob.x = new svm_node*[l];
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Problem::SetXY(int i, struct svm_node* xx, double yy) {
    if (i < 0 || i >= GetL())
      return false;

    if (svm_library == LIB_LINEAR) {
      lprob.x[i] = xx;
      lprob.y[i] = (int)floor(yy);
    } else if (svm_library == LIB_SVM) {
      sprob.x[i] = xx;
      sprob.y[i] = floor(yy);
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Problem::Write(const string& fname) {
    ofstream fp(fname.c_str());
    bool ok = fp.good();
    for (int i=0; ok && i<GetL(); i++) {
      fp << GetY(i);
      const svm_node* x_space = NULL;

      if (svm_library == LIB_LINEAR)
        x_space = lprob.x[i];
      else if (svm_library == LIB_SVM)
        x_space = sprob.x[i];

      for (int j=0; x_space[j].index != -1; j++) {
        fp << " " << x_space[j].index << ":" << x_space[j].value;
      }
      fp << endl;
    }
    fp.close();
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Problem::Read(const string& fname) {
    string msg = "SVM::Problem::Read("+fname+"): ";

    if (svm_library == PM_SVM)
      return ShowError(msg, "not implemented for PM_SVM!");

    int l = 0;
    int maxidx = 0;

    ifstream fp(fname.c_str());
    bool ok = fp.good();
  
    // first read-through: count lines and elements
    string line;
    while (getline(fp, line)) {
      istringstream ss(line);
      string s;
      ss >> s;
      int c = atoi(s.c_str());
      if (c != -1 && c != 1)
        return ShowError(msg, "implemented only for two classes.");
      while (ss >> s) {
        size_t cpos = s.find(':');
        int idx = atoi(s.substr(0,cpos).c_str());
        if (idx > maxidx)
          maxidx = idx;
      }
      l++;
    }
    cout << "fp.tellg()=" << fp.tellg() << endl;
    fp.clear();
    fp.seekg(0, ios::beg); // reset to reading from beginning
    cout << "fp.tellg()=" << fp.tellg() << endl;


    // WriteLog(msg, "read problem with "+ToStr(l)+" vectors of length "+
    //          ToStr(maxidx));

    InitXY();

    int maxl = l*(1+maxidx);
    x_space = new svm_node[maxl];

    int i=0, j=0;
    while (getline(fp, line)) {
      istringstream ss(line);
      string s;
      ss >> s;

      SetXY(i++, &x_space[j], atof(s.c_str()));

      while (ss >> s) {
        size_t cpos = s.find(':');
        x_space[j].index = atoi(s.substr(0,cpos).c_str());
        x_space[j++].value = atof(s.substr(cpos+1).c_str());
      }
      x_space[j++].index = -1;
      cout << endl;
      if (j > maxl)
        return ShowError(msg, "Read more vectors than what was allocated for!",
                         ToStr(j)+" > "+ToStr(maxl));
    }
    
    fp.close();
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SVM::Problem::CheckParameter(const Parameter& param) {
    const char *error_msg = NULL;

    if (svm_library == LIB_LINEAR)
      error_msg = check_parameter(&lprob, param.GetLinear());
    else if (svm_library == LIB_SVM)
      error_msg = svm_check_parameter(&sprob, param.GetSVM());

    if (error_msg) {
      string msg = error_msg;
      return ShowError("SVM::Parameter::Check() error: <"+msg+">");
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  SVM::Target::Target(int l, library_t lt) : target_d(NULL), target_i(NULL) {
    use_int = (lt == LIB_LINEAR);

    if (use_int)
      target_i = new int[l];
    else
      target_d = new double[l];
  }

  /////////////////////////////////////////////////////////////////////////////
  
  SVM::Target::~Target() {
    if (target_i)
      delete [] target_i;
    if (target_d)
      delete [] target_d;
  }

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
