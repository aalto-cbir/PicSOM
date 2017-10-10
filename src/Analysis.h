// -*- C++ -*-  $Id: Analysis.h,v 2.490 2017/05/09 10:19:10 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_ANALYSIS_H_
#define _PICSOM_ANALYSIS_H_

#include <Query.h>

#include <DataSet.h>
#include <ConfusionMatrix.h>

#include <cox/knn>
#include <cox/matrix>

#include <map>
#include <iostream>
using namespace std;

#ifdef USE_MRML
#include <netdb.h>
#endif // USE_MRML

#ifdef SIMPLE_USE_PTHREADS
#define ANALYSIS_USE_PTHREADS
#endif // SIMPLE_USE_PTHREADS

#include <AddDescription.h>
//?? #include <sstream>

namespace picsom {
  using simple::Classifier;
  using simple::ConfusionMatrix;
  using simple::FloatPointVector;
  using simple::GnuPlot;
  using simple::GnuPlotData;
  using simple::DataSet;

  static const string Analysis_h_vcid =
    "@(#)$Id: Analysis.h,v 2.490 2017/05/09 10:19:10 jormal Exp $";

  /////////////////////////////////////////////////////////////////////////////

  template <class T> 
  static vector<T> multiply(const vector<T>& in, T v) {
    vector<T> out(in.size());
    vector<T> vvv(in.size(), v);
    transform(in.begin(), in.end(), vvv.begin(),
	      out.begin(), multiplies<float>());
    return out;
  }

  /////////////////////////////////////////////////////////////////////////////

  template <class T> 
  static bool write_matlab(const vector<T>& v, ostream& os, const string& l) {
    if (l!="")
      os << l << " = ";
    os << "[";
    for (size_t i=0; i<v.size(); i++)
      os << v[i] << endl;
    os << "];" << endl;
  
    return os.good();
  }

  /////////////////////////////////////////////////////////////////////////////

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /** Main work horse for all analysis of results, implementation of 
      different algorithms, etc...
  */

  class Analysis {
  public:
    /// Primary data structure after parsing output of CbirStages().
    typedef list<pair<size_t,double> > objectlist_t;

    /// Results returned by Analyse*() methods.
    class analyse_result {
    public:
      /// Default constructor.
      analyse_result(double c = 0.0, double t = 0.0, double b = 0.0) :
	corr(c), tot(t), bias(b) { 
	isok = true;
	rescount = rounds = seen = found = firstpos = batchloop = 0;
	tau = tauord = birds = firsthitadv = firsthitadv_geo = 0.0;
	firsthit = roc_auc = 0.0;
	roc_eer = 1.0;
	mean_inv_rank = 0.0;
	ranking = old_ranking = appoint = -1;
	avgprec = old_avgprec = inferredap = -1.0;
	avgprecbl = avgprecadv_a = avgprecadv_g = 0.0;
	optval = sortval = 0.0;
	time_real = time_user = time_sys = time_cpu = 0.0;
      }

      /// Constructor that simply indicates correct/erroneus result.
      analyse_result(bool r) {
	clear();
	isok = r;
      }

      /// Construction from WHAT vector.
      analyse_result(const vector<float>& v);

      /// Read-from-file constructor.
      analyse_result(const string& s) {
	analyse_result tmp;
	if (tmp.read(s))
	  *this = tmp;
	else {
	  clear();
	  ShowError("analyse_result("+s+") failed");
	}
      }

      ///
      bool operator==(const analyse_result&) const;

      ///
      void clear() { *this = analyse_result(); }

      ///
      bool empty() const { return rescount==0; }

      ///
      void error() { isok = false; }

      ///
      bool errored() const { return !isok; }

      ///
      bool ok() const { return isok && rescount; }

      ///
      bool add_sub_result(const analyse_result& c) {
	sub_result.push_back(c);
	return true;
      }
      
      /// 
      void clear_sub_results() {
	sub_result_type = "";
	sub_result.clear();
      }

      ///
      bool read(const string& s);

      ///
      bool from_xml(XmlDom&);

      ///
      bool write(const string& s) const;

      ///
      bool to_xml(XmlDom&, bool) const;

      ///
      bool to_xml(XmlDom&, const string&, const vector<float>&, size_t) const;

      ///
      bool to_xml(XmlDom&, const string&, const list<pair<size_t, double> >&,
		  size_t) const;
      
      ///
      bool dump(ostream&, const string&, bool, bool) const;

      ///
      void set_times(const TicTac& t) {
	t.Times(time_real, time_user, time_sys);
	time_cpu = time_user+time_sys;
      }

      ///
      void set_sizes(double c, double t, double b) {
	corr = c; tot = t; bias = b;
      }

      ///
      void setvecs(double r, double p, double a, double i, double m, double l,
		   double fp, double tp){
	// cout << "setvecs() appending : " << r << " " << p << " "
	//      << a << " " << i << endl;
	recall.       push_back(r);
	precision.    push_back(p);
	avgprecision. push_back(a);
	instprecision.push_back(i);
	maxpr.        push_back(m);
	apriori.      push_back(l);
	false_pos.    push_back(fp);
	true_pos.     push_back(tp);
      }

      ///
      void set_roc_from_hits(const vector<float>&, double, double, size_t);

      ///
      void set_roc_from_recall(const vector<double>&, double, double);

      ///
      void set_roc_from_recall(const vector<float>& v, double s, double t) {
	set_roc_from_recall(vector<double>(v.begin(), v.end()), s, t);
      }

      ///
      void set_roc_eer();

      ///
      void set_roc_auc();

      ///
      size_t roc_size() const { return true_pos.size(); }

      ///
      string roc_range() const {
	stringstream ss;
	ss << "[";
	if (false_pos.size()>0)
	  ss << false_pos[0];
	if (false_pos.size()>1)
	  ss << ".." << false_pos.back();
	ss << "][";
	if (true_pos.size()>0)
	  ss << true_pos[0];
	if (true_pos.size()>1)
	  ss << ".." << true_pos.back();
	ss << "]";
	return ss.str();
      }

      ///
      double avgprecadv_g_value(double n) const {
        return avgprecadv_g>0 && n>0 ?
	  pow(double(avgprecadv_g), double(1.0/n)) : 0.0;
      }

      /// data members from here on:

      bool isok;
      float corr, tot, bias;
      int rescount;
      int rounds;
      int seen;
      int found;
      int firstpos;

      int old_ranking;
      float old_avgprec;

      int ranking;
      float avgprec;
      float avgprecbl;
      float avgprecadv_a;
      float avgprecadv_g;
      float inferredap;
      int appoint;

      float tau;
      float tauord;   
      float birds;
      float firsthit;
      float firsthitadv;
      float firsthitadv_geo;

      float roc_auc;
      float roc_eer;

      float mean_inv_rank;

      float time_real, time_user, time_sys, time_cpu;

      float optval;   // this is set afterwards in DoAnalyse().
      float sortval;
      int batchloop;  // this is set afterwards in DoAnalyse().
      string params;  // this is set afterwards in DoAnalyse().

      vector<float> recall;
      vector<float> precision;
      vector<float> avgprecision;
      vector<float> instprecision;
      vector<float> maxpr;
      vector<float> apriori;
      
      vector<float> false_pos;
      vector<float> true_pos;

      vector<float> threshold;
      vector<float> minaccrec;

      map<string,simple::FloatMatrix> conv_matrix;

      objectlist_t best;

      string sub_result_type;

      list<analyse_result> sub_result;

    };  // class analyse_result

    /// Creation from command line and directive files.
    Analysis(PicSOM*, const Analysis*, Query*, const vector<string>&);

    /// Poison.
    Analysis() { SimpleAbort(); }

    /// Poison.
    Analysis(const Analysis& /*a*/) { SimpleAbort(); }

    /// Destructor.
    virtual ~Analysis();

    /// Poison.
    Analysis& operator=(const Analysis&) { SimpleAbort(); return *this; }

    /// Dump is required in all descendants of Simple.
    virtual void Dump(Simple::DumpMode = Simple::DumpDefault,
		      ostream& = cout) const;
    
    /// Link to the surroundings.
    PicSOM *Picsom() { return picsom; }

    /// Link to the surroundings.
    const PicSOM *Picsom() const { return picsom; }

    /// Read access to name.
    const string& IdName() const { return idname!="" ? idname : method; }

    /// Set access to name.
    void IdName(const string& n) { idname = n; }

    /// Pointer to parent.
    const Analysis *ParentOrThis() const {
      return parent ? parent->ParentOrThis() : this;
    }

    /// Access to query.
    Query *GetQuery() const { return query; }

    /// Creates a new query.
    Query *CreateQuery() {
      //
      // old behavior created always a non-owned instance:
      // query = Picsom()->NewQuery();
      //
      DeleteQuery();
      query = new Query(Picsom());
      query_own = true;
      return query;
    }

    /// Retrieves an old query.
    Query *FindQuery(const string& q) {
      DeleteQuery();
      Connection *conn = NULL;
      query = Picsom()->FindQuery(q, conn, this);
      query_own = false;
      return query;
    }

    /// Deletes query if not owned bu picsom.
    void DeleteQuery() {
      if (query_own)
	delete query;

      query = NULL;
    }

    /// work horse
    static bool DoAnalyse(PicSOM*, const vector<string>&);

    ///
    int Verbose() const { return verbose; }
    
    ///
    void Verbose(int v) { verbose = v; }
    
    /// Returns true if cin can be read.
    bool HasCin() const { return has_cin; }

    /// Returns true if cin can be read.
    void HasCin(bool h) { has_cin = h; }
    
    /// Returns true if running in CBIR server.
    bool InServer() const { return Picsom()->IsServer(); }

    /// A placeholder...
    string Identity() const { return "A:"+TaskStack(); }

    /// Writes strings to log.
    void WriteLog(const string& m1, const string& m2 = "",
		  const string& m3 = "", const string& m4= "", 
		  const string& m5 = "") const {
      picsom->WriteLogStr(LogIdentity(), m1, m2, m3, m4, m5); }

    /// Writes contents of a stream to log.
    void WriteLog(ostringstream& os) const {
      picsom->WriteLogStr(LogIdentity(), os); }

    /// Gives log form of Analysis name, quite simple.
    string LogIdentity() const { return "=="; }

    /// Logs both in visual and matlab format.
    void DoubleLog(string& mat, const string& m1, const string& m2 = "",
		   const string& m3 = "", const string& m4= "", 
		   const string& m5 = "") const {
      WriteLog(m1, m2, m3, m4, m5);
      mat += "% " + m1 + m2 + m3 + m4 + m5 + "\n";
    }

    /// Writes a log separator both in visual and matlab format.
    void DoubleLogSeparator(string& mat) const {
      DoubleLog(mat, string(64, '='));
    }

    /// Processes given command line arguments.
    bool ProcessArguments(vector<string>&);

    /// Reproduces last command line arguments from rest_argv_str.
    vector<string> DashedExtraArguments() const {
      vector<string> a;
      if (rest_argv_str.size()>0) {
	if (rest_argv_str[0][0]!='-')
	  a.push_back("-");
	a.insert(a.end(), rest_argv_str.begin(), rest_argv_str.end());
      }
      return a;
    }

    ///
    bool ScriptSplitBlocks(const list<string>&, string&, list<string>&,
			   list<string>&, list<string>&, list<string>&) const;

    ///
    bool ScriptHasChoices(const list<string>& s) const {
      list<string> a, b, c, o;
      string n;
      return ScriptSplitBlocks(s, n, o, a, b, c) && !b.empty();
    }

    ///
    bool ScriptHasChoices() const { return ScriptHasChoices(script); }

    ///
    list<string> ScriptJoinBlocks(const list<string>& b, const string& l,
				  const list<string>& a) const {
      list<string> ret = b;
      if (!l.empty())
	ret.push_back(l);
      ret.insert(ret.end(), a.begin(), a.end());
      return ret;
    }

    bool Script(const list<string>& s) { 
      script = s;
      if (debug_script)
	ScriptDump(script, true);
      return !script.empty();
    }

    ///
    bool ScriptDump(const list<string>&, bool, bool = true) const;

    ///
    bool ScriptExecuteAndShow();

    ///
    bool ScriptShow();

    ///
    bool ScriptExecute() { return ScriptExecute(script); }

    ///
    bool ScriptExecute(const list<string>&);

    ///
    bool ScriptExecute(const script_exp_t&);

    /// 
    script_exp_t ScriptExpand(const list<string>&, bool) const;

    /// 
    script_exp_t ScriptExpandFile(const string&, const script_exp_t&) const;

    ///
    list<string> ScriptSolveScriptPath(const script_exp_t&) const;

    /// 
    script_exp_t ScriptDefaultMacros() const;

    /// 
    script_exp_t ScriptExpandMacros(const script_exp_t&, bool) const;

    /// 
    string ScriptExpandString(const string&,
			      const script_exp_t&) const;

    ///
    list<string> ScriptExpandBlock(const list<string>&, DataBase*) const;

    ///
    list<string> ScriptRead(const string&, const script_exp_t&) const;

    ///
    list<string> ScriptUnExpand(const script_exp_t& s) const {
      list<string> ret;
      for (script_exp_t::const_iterator i = s.begin(); i!=s.end(); i++)
	ret.push_back(i->first+"="+i->second);
      return ret;
    }

    ///
    script_exp_t::iterator ScriptFind(script_exp_t& s, const string& k) const {
      script_exp_t::iterator i = s.begin();
      for (; i!=s.end() && i->first!=k; i++) ;
      return i;
    }

    ///
    bool ScriptReplace(script_exp_t& s, const string& k, const string& v,
		       bool a, bool e) const;

    ///
    list<string> ScriptMakeChoices(const script_exp_t& s, const string& k,
				   const string& v, const list<string>&) const;

    ///
    bool SplitSemiColonsAndEqualSigns(script_exp_t& plist,
				      const string& a) const
      throw(logic_error) {
      vector<string> aa = SplitInSomething(";", false, a);
      for (size_t i=0; i<aa.size(); i++)
	plist.push_back(SplitKeyEqualValueNew(aa[i]));
      return true;
    }

    ///
    string JoinSemiColonsAndEqualSigns(const script_exp_t& plist) 
      const {
      string ret;
      for (list< pair<string,string> >::const_iterator i=plist.begin(); 
	   i!=plist.end(); i++) {
	if (!ret.empty()) ret += "; ";
	ret += i->first+"="+i->second;
      }
      return ret;
    }

    /// A helper routine for ProcessArguments().
    static bool IsBinaryDataContent(const string& s) {
      for (size_t i=0; i<s.size(); i++) {
	unsigned char ch = s[i];
	if (ch!='\t' && ch<' ')
	  return true;
	if (ch>126 && ch<128+' ')
	  return true;
      }
      return false;
    }

    /// Strores key=value pairs in commands.
    void RecordKeyEqualValue(const string& k, const string& v) {
      commands_str += (commands_str=="" ? "" : " ") + k + "=" + v;
    }

    /// Interpretation of key=value pairs.
    bool Interpret(const string&, const string&, int&);

    /// Execution of the create task.
    bool Create(const vector<string>&);

    /// Execution of the insert task.
    bool Insert();

    /// This is called by Insert().
    bool InsertObjects(const vector<string>&, vector<size_t>*);

    ///
    bool InsertObjectsOperation(const string&, const vector<string>&);

    ///
    bool AddTextLineData(const string&, size_t, const string&);

    ///
    bool WriteOutTextLineData();

    ///
    bool FetchImageNet(const string&, const string&, vector<string>&);

    ///
    bool InsertImageNetSynset(const string&, XmlDom&);

    /// This is called by InsertObjects().
    bool InsertDirectory(const string&, XmlDom&);

    /// This is called by InsertObjects() and InsertDirectory().
    bool InsertOneFile(const string&, XmlDom&, vector<size_t>*, const string&,
		       const string&, bool);

    /// Serial implementation of InsertOneFile().
    bool InsertOneFileSerial(const string&, XmlDom&, vector<size_t>*,
			     const string&, const string&, const string&, bool);

    /// Threaded implementation of InsertOneFile().
    bool InsertOneFileParallel(const string&, XmlDom&, vector<size_t>*,
			       const string&, const string&, const string&, bool);

    /// A synchronization point for InsertOneFileParallel().
    bool InsertOneFileSynchronize(vector<size_t>*);

    ///
    static void *InsertOneFilePthreadCall(void*);

    ///
    bool InsertOneFilePthreadCallInner(const string& filect,
				       XmlDom& xml, 
				       vector<size_t>* idxs);
    ///
    analyse_result AnalyseInsertSubObjects(const vector<string>&);

    ///
    FloatVector::DistanceType* MakeMetric(int vl);

    /// Sees if metric is set and acts accordingly.
    bool SetMetric(Classifier&, string&);

    /// Divides a database when either data or map is alien.
    bool HandleAlienDataBase(Query&);

    /// Divides an alien database's features with this databases's TS-SOMs.
    bool DivideAlienData(TSSOM&, TSSOM&, bool, const string&);


    /// Parses string like "0-9:1 10-15:0.5" to FloatVector
    static FloatVector* ParseSparse(const string& str, size_t len);

    /// Creates a new TSSOM and writes it and its .div to disk.
    bool CreateSOM(Query&);

    /// Creates a new TSSOM with libsvm.
    bool CreateSVM(const Query&);

    bool SVMPredTrain(const string& svmprefix, const string& featname,
		      const string& svmopts, const string& extra,
		      const string& svmname, const string& predict_str);

    bool PredictSVM(SVM* svmp, const ground_truth& rst, int n,
                    const string& fnpre, const string& predict_str = "yes",
                    bool do_cont=false, set<string> skip_set = set<string>());

    bool PredictSVMalt(SVM* svmp, const ground_truth& rst, int n,
		       const string& fnpre, const string& predict_str = "yes",
		       bool do_cont=false, set<string> skip_set = set<string>());

    ///
    bool SVMsetParam(const string& key, const string& val) {
      if (!SVM::allowedParam(key))
        return ShowError("SVMsetParam(): \""+key+"\" is not an allowed "
                         "parameter according to SVM::allowedParam() !");

      svm_params[key] = val;

      return true;
    }
    
    ///
    string SVMgetParam(const string& key, const string& def="") const {
      map<string, string>::const_iterator it = svm_params.find(key);
      return it==svm_params.end() ? def : it->second;
    }

    ///
    bool SVMparamGiven(const string& key) const {
      map<string, string>::const_iterator it = svm_params.find(key);
      return it!=svm_params.end();
    }
    
    ///
    int SVMgetParamInt(const string& key, int def=-1) const {
      string ret = SVMgetParam(key,"");

      const char* nptr = ret.c_str();
      char* endptr;
      errno = 0;
      int n = strtol(ret.c_str(), &endptr, 10);

      if (errno != 0 || endptr == nptr)
        return def;
      return n;
    }

    ///
    float SVMgetParamFloat(const string& key, float def=-1.0) const {
      string ret = SVMgetParam(key,"");

      const char* nptr = ret.c_str();
      char* endptr;
      errno = 0;
      float n = strtof(ret.c_str(), &endptr);

      if (errno != 0 || endptr == nptr)
        return def;
      return n;
    }

    ///
    bool SVMgetParamBool(const string& key, bool def=false) const {
      string ret = SVMgetParam(key, def ? "true" : "false");
      return IsAffirmative(ret);
    }

    ///
    string SVMparams() const {
      string s;
      for (map<string,string>::const_iterator i=svm_params.begin();
	   i!=svm_params.end(); i++)
	s += (s==""?"":",")+i->first+"="+i->second;
      return s;
    }

  private:
    bool ClassMax(ground_truth& gtx, int max, int rand_seed=42);
    void SVMPosNegMax(ground_truth& pos, ground_truth& neg);

  public:

    /// Creates a new ELM.
    bool CreateELM(const Query&);

    /// Creates a new vector quantization and writes it and its .div to disk.
    bool CreateLBG(Query&);

    /// Creates a file containing data values ordered by component.
    bool CreateOrd(Query&);

    /// Calculates average distances between data vectors.
    bool CreateMeanDist(Query&);

    /// Creates a new ground truth class.
    bool CreateGroundTruth();

    /// Creates a new ground truth class.
    ground_truth CreateGroundTruthQuery();

    /// Creates a new ground truth class.
    ground_truth CreateGroundTruthOrigins();

    /// Returns ground_truth from testset or argument corr if testset not def
    ground_truth TestSet(const ground_truth& corr,
			 target_type tt, int o) const {
      if (testsetname!="") {
	bool do_and = testsetname.find("*&")==0;
	string tset = do_and ? testsetname.substr(2) : testsetname;
	ground_truth tested = GroundTruthExpression(tset, tt, o, expand);
	if (tested.NumberOfEqual(1))
	  return do_and ? corr.TernaryAND(tested) : tested;

	ShowError("TestSet(): Found no objects [", TargetTypeString(tt),
		  "] of testset <", testsetname, ">");
	return ground_truth();
      }
      return corr;
    }

    ///
    bool DoDetections();

    ///
    bool DoSegments();

    ///
    bool ExtractMedia();

    /// 
    bool ExtractFeatures(Query&, bool = false,
			 const vector<string>& = vector<string>());

    /// 
    bool ExtractFeaturesListed(const vector<string>&);

    ///
    bool CombineOrphans(Query&);

    /// 
    bool CombineOrphansOld(Query&);

    /// 
    bool RemoveCreated(Query&) {
      return CheckDB()->RemoveCreated();
    }

    ///
    bool UpdateDiv(Query&);

    /// Saves maps without data.
    bool StripVectors(Query&);

    /// Performs random projection.
    bool RandProj(Query&);

    /// concatenates feature vectors
    bool ConcatFeat(Query&);

    ///
    analyse_result ConcatMapCoord(const vector<string>&);

    ///
    analyse_result CreateSegmentHistogram(const vector<string>&);

    ///
    analyse_result DivStats(const vector<string>&);

    ///
    analyse_result Optimize();

    ///
    analyse_result CrossValidateNew();

    /// A helper for the one above.
    bool CrossValScriptCorrect(script_exp_t&, const aspect_map_t&,
			       const string&, const string&);

    ///
    analyse_result CrossValidateOld();

    ///
    analyse_result MultiFoldFeatSel(const string& optarg, const string& rest);

    ///
    analyse_result MultipleFeatSel(const string& optarg, const string& rest);

    ///
    analyse_result AnalyseFeatSel(const vector<string>&);

    /// Expands a comma-separated string of feature aliases to list of
    /// real features
    list<string> ExpandFeaturesString(const string& fstr) const {
      list<string> feats;
      vector<string> l = SplitInCommasObeyParentheses(fstr);
      for (vector<string>::const_iterator lit=l.begin();lit!=l.end();lit++) {
	list<string> fl = GetDataBase()->ExpandFeatureAlias(*lit);
	feats.insert(feats.end(),fl.begin(),fl.end());
      }
      return feats;
    }

    ///
    string JoinFeaturesString(const list<string>& a, 
			      const string& b) const {
      list<string> c = a;
      c.push_back(b);
      return JoinFeaturesString(c);
    }
    
    ///
    list<string> RemoveListIntersection(const list<string>& a, 
					const list<string>& b) const {
      list<string> ret;
      
      if (!b.size())
	return a;

      for (list<string>::const_iterator i=a.begin(); i!=a.end(); i++) {
	list<string>::const_iterator pos = find(b.begin(), b.end(), *i);
	// add to ret if element not found in b
	if (pos==b.end()) 
	  ret.push_back(*i);
      }
      return ret;
    }

    ///
    list<string> ListIntersection(const list<string>& a,
				  const list<string>& b) const {
      list<string> ret;

      if (!b.size())
	return ret;

      for (list<string>::const_iterator i=a.begin(); i!=a.end(); i++) {
	list<string>::const_iterator pos = find(b.begin(), b.end(), *i);
	// add to ret if element not found in b
	if (pos!=b.end())
	  ret.push_back(*i);
      }
      return ret;
    }

    ///
    string JoinFeaturesString(const list<string>& a) const {
      string ret;
      for (list<string>::const_iterator i=a.begin(); i!=a.end(); i++)
	ret += (ret.empty()?"":",")+*i;
      return ret;
    }

    void DisplayScript(const list<string> &l){
      for (list<string>::const_iterator i=l.begin(); i!=l.end(); i++) 
	cout << *i << endl;
    }
	
    /// This calls the actual analysis methods upon the value of method.
    analyse_result Analyse();
    
    /// This calls the actual analysis methods upon the value of method.
    list<analyse_result> Analyse(script_list_t&, bool, bool);
    
    /// This calls the actual analysis methods upon the value of method.
    list<analyse_result> AnalyseSlaves(script_list_t&, bool);

    /// An utility for the above one.
    bool AllowManySlaveThreads(const script_list_t&) const;
    
    ///
    bool ProcessSlaveXmlResult(const XmlDom&, const PicSOM::slave_info_t&);

    ///
    bool ProcessSlaveXmlResultFeatureVectorList(const XmlDom&, bool,
						string&, size_t&);

    ///
    bool ProcessSlaveXmlResultFeatureVector(const XmlDom&, bool, string&);

    ///
    bool ProcessSlaveXmlResultDetectionVectorList(const XmlDom&, bool,
						  string&, size_t&);

    ///
    bool ProcessSlaveXmlResultDetectionVector(const XmlDom&, bool, string&);

    ///
    bool ProcessSlaveXmlResultCaptionList(const XmlDom&, bool,
					  string&, size_t&);

    ///
    bool ProcessSlaveXmlResultCaption(const XmlDom&, bool, string&);

    ///
    bool ProcessSlaveXmlResultObjectInfoHashList(const XmlDom&, size_t&);

    ///
    bool ProcessSlaveXmlResultObjectInfoHash(const XmlDom&);

    ///
    bool ProcessSlaveXmlResultObjectChildInfoList(const XmlDom&,
						  size_t&, size_t&);

    ///
    bool ProcessSlaveXmlResultObjectChildInfo(const XmlDom&, size_t&);

    /// This calls the actual analysis methods upon the value of method.
    list<analyse_result> AnalyseSerial(const script_list_t&);
    
    /// This calls the actual analysis methods upon the value of method.
    analyse_result Analyse(const script_list_e&);
    
#ifdef ANALYSIS_USE_PTHREADS
    ///
    typedef struct {
      pthread_t thread_id;
      pid_t tid;
      pthread_t parent_thread_id;
      pid_t parent_tid;
      bool thread_set;
      Analysis *analysis;
      script_list_e param;
      bool running;
      bool ready;
      bool joined;
      bool collected;
      analyse_result result;
      vector<size_t> indices;
    } analyse_pthread_data_t;

    ///
    static string ThreadIdentifier(const analyse_pthread_data_t& d) {
      return ThreadIdentifierUtil(d.thread_id, d.tid);
    }

    ///
    static string ThreadParentIdentifier(const analyse_pthread_data_t& d) {
      return ThreadIdentifierUtil(d.parent_thread_id, d.parent_tid);
    }

    /// Pthreaded layer above Analyse().
    list<analyse_result> AnalysePthread(const script_list_t&, bool);

    /// Pthreaded layer above Analyse().
    analyse_pthread_data_t *AnalysePthread(const script_list_e&);

    /// Wrapper around Analyse(script_list_e).
    static void *AnalysePthreadCall(void*);

    ///
    bool AnalysePthreadJoin(list<analyse_result>&, bool);

    ///
    bool AnalysePthreadJoin(analyse_pthread_data_t&);

    ///
    string ThreadDataInfo(const analyse_pthread_data_t&) const;

    ///
    void ShowThreadDataInfo() const;

    /// Registers a new thread.
    string RegisterThread(pthread_t p, pid_t tid, const string& t,
			  analyse_pthread_data_t *e) {
      return Picsom()->RegisterThread(p, tid, "analysis", t, this, e);
    }
    
    /// Registers a new thread.
    bool UnregisterThread(pthread_t p) {
      return Picsom()->UnregisterThread(p);
    }

    /// Registers a new thread.
    bool UnregisterThread(const string& t) {
      return Picsom()->UnregisterThread(t);
    }
    
    /// State of the thread.
    const string& ThreadState(analyse_pthread_data_t*) const;
    
    /// State of the thread.
    const string& ThreadState(void *p) const {
      return ThreadState((analyse_pthread_data_t*)p);
    }
    
    /// Result of the thread.
    const analyse_result& ThreadResult(analyse_pthread_data_t*) const;

    /// Result of the thread.
    const analyse_result& ThreadResult(void *p) const {
      return ThreadResult((analyse_pthread_data_t*)p);
    }

#endif // ANALYSIS_USE_PTHREADS

    /// 
    bool ShowAnalyseUsage(const string&);

    ///
    string ResultName(bool&, int, int, int, const string&) const;

    ///
    bool DisplayOrWrite(const imagedata&, bool, const string&,
			const string&, const string&) const;

    /// dummy analysis for testing
    analyse_result AnalyseDummy(const vector<string>&);

    /// dummy analysis for sleeping
    analyse_result AnalyseSleep(const vector<string>&);

    /// just dumps the execution environment
    analyse_result AnalyseEnv(const vector<string>&);

    ///
    analyse_result AnalyseCaffeConvertImageset(const vector<string>&);

    //
    analyse_result AnalyseTranslateTest(const vector<string>&);

    ///
    analyse_result AnalyseTfIdfTest(const vector<string>&);
    
    ///
    analyse_result AnalyseWord2VecTest(const vector<string>&);
    
    ///
    analyse_result AnalyseWordNetTest(const vector<string>&);
    
    ///
    analyse_result AnalyseSparqlTest(const vector<string>&);
    
    ///
    analyse_result AnalyseRdfTest(const vector<string>&);
    
    ///
    analyse_result AnalyseOpenCvTest(const vector<string>&);
    
    ///
    analyse_result AnalyseCudaTest(const vector<string>&);
    
    ///
    analyse_result AnalysePythonTest(const vector<string>&);
    
    ///
    analyse_result AnalyseTensorFlowTest(const vector<string>&);
    
    ///
    analyse_result AnalyseJaulaTest(const vector<string>&);
    
    ///
    analyse_result AnalyseGT(const vector<string>&);

    ///
    analyse_result AnalyseCreateClassFile(const vector<string>&);

    ///
    analyse_result AnalyseMetaClassFile(const vector<string>&);

    ///
    analyse_result AnalyseDTW(const vector<string>&);

    ///
    analyse_result AnalyseSuviVk(const vector<string>&);

    ///
    analyse_result AnalyseSuviBenchmark(const vector<string>&);

    ///
    analyse_result AnalyseLscom(const vector<string>&);

    ///
    analyse_result AnalyseOntology(const vector<string>&);

    ///
    analyse_result AnalyseDefineClass(const vector<string>&);

    ///
    analyse_result AnalyseErfData(const vector<string>&);

    ///
    analyse_result AnalyseObjectInfo(const vector<string>&);

    ///
    analyse_result AnalyseNextFreeLabel(const vector<string>&);

    ///
    analyse_result AnalyseCheckSums(const vector<string>&);

    ///
    analyse_result AnalyseSlaveTest(const vector<string>&);

    ///
    analyse_result AnalyseGvtTest(const vector<string>&);

    ///
    analyse_result AnalyseOdTest(const vector<string>&);

    ///
    analyse_result AnalyseObjectSetTest(const vector<string>&);

    ///
    analyse_result AnalyseTypes(const vector<string>&);

    ///
    analyse_result AnalyseLabelHash(const vector<string>&);

    ///
    analyse_result AnalyseRgb(const vector<string>&);

    ///
    analyse_result AnalyseConv(const vector<string>&);

    ///
    analyse_result AnalyseMI(const vector<string>&);

    ///
    analyse_result AnalyseDiv(const vector<string>&);

    ///
    analyse_result AnalyseDivOld(const vector<string>&);

    ///
    analyse_result AnalyseSomValues(const vector<string>&);

    ///
    analyse_result AnalyseDisplay(const vector<string>&);

    ///
    analyse_result AnalyseExtractMedia(const vector<string>&);

    ///
    analyse_result AnalyseExtractVideoThumbnails(const vector<string>&);

    ///
    analyse_result AnalyseExtractLfwFaces(const vector<string>&);

    ///
    analyse_result AnalyseFinlandiaKatsaus(const vector<string>&);

    ///
    float TimeSegmentationScore(const vector<size_t>&, size_t, size_t,
				const vector<float>&, float,
				float, float, float,
				const vector<vector<float> >& fea,
				float);

    ///
    analyse_result AnalyseShotBoundaryThreshold(const vector<string>&);

    ///
    analyse_result AnalyseShotBoundary(const vector<string>&);

    ///
    analyse_result AnalyseShotBoundaryWithSummary(const vector<string>&);

    ///
    analyse_result AnalyseShotBoundaryWithFeatures(const vector<string>&);

    ///
    analyse_result AnalyseShotBoundaryFromElan(const vector<string>&);

    ///
    analyse_result AnalyseShotBoundaryTrecvid(const vector<string>&);

    ///
    analyse_result AnalyseShotBoundaryParams(const vector<string>&);

    ///
    analyse_result AnalyseFillZeroFeatureData(const vector<string>&);

    ///
    analyse_result AnalyseFeatureData(const vector<string>&);

    ///
    analyse_result AnalyseDetectionData(const vector<string>&);

    ///
    analyse_result AnalyseDetectionStats(const vector<string>&);

    ///
    analyse_result AnalyseDetectionsToFeatures(const vector<string>&);

    ///
    analyse_result AnalyseDetectorDifference(const vector<string>&);

    ///
    analyse_result AnalyseDetectorPerformance(const vector<string>&);

    ///
    void ShowExamples(const multimap<float,size_t>&, bool, size_t,
		      DataBase*, const string&, bool);

    ///
    bool WriteHistogramImage(const vector<size_t>&, size_t, bool,
			     const string&);
    ///
    analyse_result AnalyseNearest(const vector<string>&);

    ///
    analyse_result AnalyseValue(const vector<string>&);

    ///
    analyse_result AnalyseEigDim(const vector<string>&);

    ///
    analyse_result AnalyseSiblings(const vector<string>&);

    ///
    analyse_result AnalyseBMUs(const vector<string>&);

    ///
    analyse_result AnalyseFocus(const vector<string>&);

    ///
    analyse_result AnalyseViewXY(const vector<string>&);

    /// Helper function for the two below.
    analyse_result RunAnalyseDivEachClass(const vector<string>&,
                                          const list<string>&);

    // typedef vector< pair<string,float> > classprobs_t;
    // typedef multimap<float,string> classprobs_t;
    typedef map<string,map<string,float> > classprobs_t;
    
    ///
    void InsertClassProb(classprobs_t& probs, size_t idx, 
                         const string& classn, float val) {
      classprobs_t::iterator it = probs.find(classn);
      if (it == probs.end())
        it = probs.insert(probs.end(), make_pair(classn,map<string,float>()));
      it->second.insert(make_pair(Label(idx),val));
    }
    
    ///
    bool CalculateMapClassProbabilities(size_t, const string&,
                                        classprobs_t&, float,
                                        bool=false);

    ///
    bool CalculateSVMClassProbabilities(size_t, const string&,
                                        classprobs_t&, float,
                                        bool=false,
                                        int depth=0);
  private:
    float (*imagenet_norm_func)(float, float, float);
    float (*imagenet_mode_func)(float, float);
  public:

    ///
    analyse_result AnalyseImageFile(const vector<string>&);

    ///
    analyse_result AnalyseImageNet(const vector<string>&);

    ///
    analyse_result AnalyseMapClassProbability(const vector<string>&);

    ///
    analyse_result AnalyseMapArea(const vector<string>&);

    ///
    analyse_result AnalyseColorNames(const vector<string>&);

    ///
    analyse_result AnalyseMapCollage(const vector<string>&);

    ///
    analyse_result AnalyseRandomCollage(const vector<string>&);

    ///
    analyse_result AnalyseCreateCollage(const vector<string>&);

    ///
    analyse_result AnalyseCreateVideoCollage(const vector<string>&);

    /// 
    analyse_result AnalyseExtractRawFeatures(const vector<string>&);

    ///
    analyse_result AnalyseGazeTrace(const vector<string>&);

    ///
    analyse_result AnalyseGazeTrace(const XmlDom&, imagedata&);

    ///
    bool GazeTraceDraw(imagedata&, XmlDom&, const string&, const string&,
		       const string&, const string&, const string&,
		       const string&);

    ///
    analyse_result AnalyseGazeRelevance(const vector<string>&);

    ///
    typedef list<cox::labeled<vector<float> > > gaze_data_t;

    ///
    vector<size_t> GazeDataRunAllUsers(const map<string,gaze_data_t>&,
				       const vector<float>&,
				       bool, size_t);

    ///
    vector<size_t> GazeDataRun(const map<string,gaze_data_t>&,
			       const list<pair<string,bool> >& ,
			       const list<pair<string,bool> >&,
			       const vector<float>&, bool, size_t);

    ///
    gaze_data_t GazeData(const map<string,gaze_data_t>&,
			 const list<pair<string,bool> > &);

    ///
    void GazeDataNormalize(gaze_data_t&, const vector<float>&,
			   const vector<float>&);

    ///
    vector<size_t> GazeDataClassify(const gaze_data_t&,
				    const gaze_data_t&, size_t);

    /// Used by AnalyseSiblings() and AnalyseBMUs().
    bool SiblingBMUcommon(map<float,float>&, float,
			  map<float,float>&, const string&);

    /// Used in AnalyseShotBoundary().
    bool OpenAddDescription(AddDescription*&, segmentfile*&,
			    const string&, const string& = "",
			    const string& = "") const;

    /// Used in AnalyseShotBoundary().
    bool CloseSegmentation(Segmentation*, segmentfile*) const;

    ///
    typedef multimap<pair<int,int>,float> sparse_pointset_t;

    ///
    static simple::FloatMatrix FillSparseValueMatrix(int rows, int cols,
						     const sparse_pointset_t&,
						     simple::FloatMatrix&);

    ///
    static int MapUsage(const simple::FloatMatrix&); 

  private:
    static double identity_kernel(const double x) { return x; }

    static double gaussian_kernel_d;
    
    static double gaussian_kernel(const double x) {
      return exp(-x*x/(2*gaussian_kernel_d*gaussian_kernel_d));
    }

  public:

    ///
    static double AveragePairDistance(const simple::FloatMatrix& m) {
      return AveragePairDistanceKernel(m,"identity");
    } 

    ///     
    static double AveragePairDistanceKernel(const simple::FloatMatrix& m,
					    const string& funcname);
                  
    ///
    static int Fragmentation(const simple::FloatMatrix&); 

    ///
    static double Purity(const simple::FloatMatrix&, const IntVectorSet&); 

    ///
    void AddToPoint(simple::FloatMatrix&, int, const TreeSOM&,
		    const string&, int, const string&, int) const;

    /** Solves the trajectory of the subobjects of the given object on the 
	selected feature maps */
    vector<vector<IntPoint> > SolveSubObjectTrajectories(int);

    vector<vector<IntPoint> > SolveSubObjectTrajectoriesDiv(int);

    string SBDParamString(vector<float> &);

    bool SBDHasBeenComputed(map<string,pair<double,double> >&, vector<float>&);

    ///
    bool AnalyseShotBoundaryInit(string imsg, ground_truth&, XmlDom&);

    /// Returns the frame numbers and durations of transitions
    vector<pair<size_t,size_t> > 
    SolveShotBoundaries(vector<vector<IntPoint> >&);

    /// returns the estimated probability of transition in given frame
    float InShotBoundaryTransition(vector<vector<pair<size_t,size_t> > >&,
				   size_t, float);

    /// solve boundaries from a single trajectory (significant jumps)
    vector<pair<size_t,size_t> > SolveTrajectoryBoundaries(vector<IntPoint>&,
							   size_t);

    /// Combines boundaries that are too close to each other
    vector<pair<size_t,size_t> > 
    CombineBoundaries(vector<pair<size_t,size_t> >&);

    /// true if areas spanned by two ranges in the trajectory overlap
    bool TrajectorySpannedAreasOverlap(vector<IntPoint>&, size_t, size_t, 
				       size_t, size_t, size_t);

    /// returns the average map coordinates of the given range on trajectory
    FloatPoint TrajectoryAvgPoint(vector<IntPoint>&, size_t, size_t);

    /// the max distance between the range on trajectory and the given point
    float TrajectoryMaxDistance(vector<IntPoint>&,size_t,size_t,FloatPoint);


    /** Compares the shot boundary data with ground truth data. Notice that 
     *  the given vector<pair<size_t,size_t> > is modified during the 
     *  process. */
    void CompareShotBoundaryResults(vector<pair<size_t,size_t> >&,XmlDom&,
				    size_t&, size_t&, size_t&,
				    size_t&, size_t&, size_t&);

    ///
    bool CompareShotBoundaryResultsCommon(XmlDom&, 
					  vector<pair<size_t,size_t> >&, 
					  string&, size_t&, size_t&, 
					  size_t&, size_t&, size_t&, size_t&);

    ///
    bool CompareShotBoundaryResultsSBD(XmlDom&, vector<pair<size_t,size_t> >&,
				       string&, size_t&, size_t&, size_t&,
				       size_t&, size_t&, size_t&);

    ///
    bool CompareShotBoundaryResultsSBDP(size_t&, size_t&, size_t&, size_t&,
					size_t&, size_t&, 
					map<string, pair<double,double> >&);

    ///
    bool GetNewShotBoundaryParams(map<string,pair<double,double> >&, 
				  vector< vector<float> >&);

    double HarmonicMean(double v1, double v2, double weight1 = 0.5) {
      if(v1==0.0 || v2==0.0)
	return 0.0;
      if(weight1 > 1.0 || weight1 < 0.0)
	weight1 = 0.5;
      double denom = weight1/v1 + (1-weight1)/v2;
      if(denom == 0.0)
	return 0.0;
      return 1/denom;
    }

    ///
    static simple::FloatMatrix Logarithmize(const simple::FloatMatrix&);

    ///
    analyse_result AnalyseKMeans(const vector<string>&);

    ///
    analyse_result AnalyseVectorQuantize(const vector<string>&);

    ///
    analyse_result AnalyseCreateHistograms(const vector<string>&);

    ///
    analyse_result AnalyseFindHistogramMatches(const vector<string>&);

    ///
    analyse_result AnalyseKMeansHistograms(const vector<string>&);

    ///
    analyse_result AnalysePairDistances(const vector<string>&);

    ///
    analyse_result AnalyseCluster(const vector<string>&);

    ///
    analyse_result AnalyseSpread(const vector<string>&);

    ///
    analyse_result AnalyseVote(const vector<string>&);

    ///
    analyse_result AnalyseClassSim(const vector<string>&);

    ///
    bool ClassSimMatrix(const map<pair<string,string>,float>&);

    /// 
    void ClassSimMatrixShow(const vector<string>&, const cox::matrix<float>&);

    ///
    analyse_result AnalyseClassification(const vector<string>&);

    ///
    analyse_result AnalyseClassify(const vector<string>&);

    /// Classify objects based on feature vector similarities
    analyse_result AnalyseFeatureClassify(const vector<string>&);

    ///
    analyse_result AnalyseKnnOld(const vector<string>&);

    ///
    analyse_result AnalyseTwoClassOld(const vector<string>&);

    ///
    analyse_result AnalyseTwoClassPerClassOld(const vector<string>&);

    ///
    list<cox::knn::lvector_t> PrepareDataSetOld(size_t, const ground_truth&,
						const string&, const string&);

    ///
    analyse_result AnalyseTwoClass(const vector<string>&);

    ///
    analyse_result AnalyseTwoClassPerClass(const vector<string>&);

    ///
    analyse_result AnalyseCommonDiscriminantOld(bool, const list<string>&,
						const vector<string>&);

    ///
    analyse_result AnalyseCommonDiscriminantNew(bool, const list<string>&,
						const vector<string>&);

    ///
    list<multimap<double,string> >
    ClassDistances(const list<cox::knn::lvector_t>&, const list<string>&);

    ///
    bool OneDiscriminant(const list<cox::knn::lvector_t>&,
			 const list<string>&, bool, bool, float&,
                         ConfusionMatrix&);

    ///
    list<cox::knn::lvector_t> PrepareDataSet(size_t, const ground_truth&,
					     const string&);

    ///
    bool DataPreProcessing(FloatVectorSet&, string&, xmlDocPtr = NULL) const;

    /// A convenience routine for DataPreProcessing().
    bool CheckSwitch(const string&, string&, string&, string&,
		     string* = NULL) const;

    ///
    bool PerformPCA(FloatVectorSet&, int) const; 

    ///
    bool AddGaussianNoise(FloatVectorSet&, float) const;

    ///
    static float GaussianNoise(float, float);

    ///
    bool ComponentOrder(FloatVectorSet&, string&) const;

    ///
    bool ComponentOrderDispersal(FloatVectorSet&, const string&, bool) const;

    ///
    FloatVector DispersalCalculus(const FloatVectorSet&, const char*,
				  int, int) const;

    ///
    FloatVector Dispersal(const FloatVectorSet&, const IntVector&) const;

    ///
    ground_truth_set SelectionList(const string&) const;

    ///
    bool IsInSelectionList(const ground_truth_set&, int, string&) const;

    ///
    bool MakeImage(int b, int c, int x, const string& d, const string& e,
		   const string& f1, const string& f2, const IntMatrix& g,
		   const SOM *h = NULL,
		   float& (SOM::*i)(int,int,bool) const = NULL,
		   double j=0) const {
      return MakeImage(b, c, x, d, e, f1, f2, &g, NULL, h, i, j); }

    ///
    bool MakeImage(int b, int c, int x, const string& d, const string& e,
		   const string& f1, const string& f2, const simple::FloatMatrix& g,
		   const SOM *h = NULL,
		   float& (SOM::*i)(int,int,bool) const = NULL,
		   double j=0) const {
      return MakeImage(b, c, x, d, e, f1, f2, NULL, &g, h, i, j); }

    ///
    bool MakeImage(int, int, int, const string&, const string&, const string&,
		   const string&, const IntMatrix*, const simple::FloatMatrix*,
		   const SOM*, float& (SOM::*)(int,int,bool) const,
		   double) const;

    ///
    bool MakeUmatrixBars(const SOM*, float& (SOM::*)(int,int,bool) const,
			 double, IntMatrix*&, IntMatrix*&) const;

    /// Add results from a round of analysis to final analyse_results 
    void AddToResult(const analyse_result&, analyse_result&, bool = true);

    /// helper function to the previous
    void SumRocs(const vector<float> &fp1,const vector<float> &tp1,
		 const vector<float> &fp2,const vector<float> &tp2,
		 vector<float> &res_fp,vector<float> &res_tp);

    ///
    analyse_result AnalyseQuery(const vector<string>&);

    ///
    analyse_result AnalyseObsProb(const vector<string>&);

    ///
    analyse_result AnalyseObsProbPerClass(int, const vector<string>&);

    ///
    analyse_result AnalyseRetrieve(const vector<string>&);

    ///
    analyse_result AnalyseBest(const vector<string>&);

    ///
    analyse_result AnalyseBestPerClass(const vector<string>&,
				       vector<float>* = NULL);

    ///
    analyse_result AnalyseBestFinalPart(const aspect_map_t&,
					const aspect_map_t&,
					const ground_truth&,
					const vector<string>&,
					vector<float>* = NULL);

    ///
    analyse_result AnalyseBestCrossVal(const vector<string>&);

    ///
    bool AnalyseBestWriteHeader(const aspect_map_t&, const aspect_map_t&,
				const ground_truth&);

    ///
    analyse_result AnalyseBestResults(const vector<string>&,
				      const ground_truth&,
				      const aspect_map_t&,
				      const aspect_map_t&,
				      const string&, bool);

    /// A subroutine for AnalyseBestResults().
    bool AnalyseBestResultsMaps(const string&);

    /// A subroutine for AnalyseBestResults().
    bool AnalyseBestResultsThreeLists(const objectlist_t&,
				      const ground_truth&);

    ///
    analyse_result AnalyseResultsCommon(const objectlist_t&,
					const ground_truth&, string&);

    ///
    void AnalyseBestResultsHTML();

    ///
    void AnalyseBestResultsTrecvid(Query*, const objectlist_t&);

    ///
    void AnalyseBestResultsTrecEval(Query*, const objectlist_t&);

    ///
    string TrecvidShotId(const string&) const;

    ///
    string TrecvidShotIdFromOrigins(const string&);

    ///
    analyse_result AnalyseTrecvidSINdevel(const vector<string>&);

    ///
    analyse_result AnalyseTrecvidSINweights(const vector<string>&);

    ///
    bool 
    TrecvidSINweightsRandomWalk(const vector<size_t>&,
				const vector<pair<string,double> >&,
				const map<string,double>&,
				const map<string,double>&,
				const map<string,string>&,
				const string&, const string&,
				const string&);

    ///
    bool 
    TrecvidSINweightsGradient(const vector<size_t>&,
			      const vector<pair<string,double> >&,
			      const map<string,double>&,
			      const map<string,double>&,
			      const map<string,string>&,
			      const string&, const string&,
			      const string&);

    ///
    bool TrecvidSINweightsEval(const list<string>&, const string&,
			       const string&, const string&, const string&,
			       const string&, const string&,
			       size_t, bool, bool,
			       vector<pair<size_t,double> >&);

    ///
    bool DetectionsWithClasswiseWeights(const vector<string>&,
					const string&, const string&,
					vector<pair<string,double> >&,
					map<string,double>&,
					map<string,double>&,
					map<string,string>&);

    ///
    vector<pair<string,double> > 
    TrecvidSINweightSolveMath(const vector<pair<string,double> >&,
			      const map<string,double>&,
			      const map<string,double>&,
			      const map<string,string>&);

    ///
    vector<double> 
    TrecvidSINweightSolveMath(const vector<pair<string,double> >&,
			      const vector<string>&,
			      const vector<double>&,			      
			      const map<string,double>&,
			      const map<string,string>&);

    ///
    double TrecvidSINevaluate(const vector<size_t>&,
			      const string&,
			      const vector<pair<string,double> >&,
			      const string& cname, const string&,
			      const string&, size_t, bool, bool);

    ///
    analyse_result AnalyseTrecEval(const vector<string>&);

    /// a helper for the two above methods
    double TrecvidInfAP(const string&,
			const map<string,vector<size_t> >&, size_t,
			const string&, const string&, bool) const;

    ///
    analyse_result AnalyseTrecvidIframes(const vector<string>&);

    ///
    analyse_result AnalyseTrecvidMedOutput2013(const vector<string>&);

    ///
    analyse_result AnalyseTrecvidMedOutput(const vector<string>&);

    ///
    analyse_result AnalyseTrecvidMedOutputSQ(const vector<string>&);

    ///
    analyse_result AnalyseTrecvidMedOutputEQ(const vector<string>&);

    ///
    analyse_result AnalyseTrecvidMedOutputES(const vector<string>&);

    ///
    typedef vector<vector<float> > trecvid_med_event_scores;

    ///
    bool AnalyseTrecvidMedXML(const string&, const string&,
			      const DataBase::trecvid_med_event&,
			      const trecvid_med_event_scores&,
			      XmlDom&);

    ///
    static string ThreeDigits(float);

    ///
    bool TrecvidSinOutputInner(const string&, const string&, const string&,
			       const string&, const string&,
			       const list<pair<string,vector<size_t> > >&);

    ///
    analyse_result AnalyseTrecvidSinOutput(const vector<string>&);

    ///
    analyse_result AnalyseTrecvidLocOutput(const vector<string>&);

    ///
    analyse_result AnalyseCOCOoutput(const vector<string>&);

    ///
    analyse_result AnalyseLSMDCoutput(const vector<string>&);

    ///
    analyse_result AnalyseImageNetOutput(const vector<string>&);

    ///
    analyse_result AnalyseVocOutput(const vector<string>&);

    ///
    analyse_result AnalyseYleJsonOutput(const vector<string>&);

    ///
    analyse_result AnalyseYleJsonOutputV1(const vector<string>&);

    ///
    analyse_result AnalyseEvaluateDetections(const vector<string>&);

    ///
    analyse_result AnalyseEvaluateDetectionsWithThresholds(const vector<string>&);

    ///
    analyse_result AnalyseShowTopDetections(const vector<string>&);

    ///
    analyse_result AnalyseDumpDetections(const vector<string>&);

    ///
    analyse_result AnalyseSelectThresholds(const vector<string>&);

    ///
    float SelectThreshold_value(const multimap<float,bool>&,
				const vector<string>&);

    ///
    float SelectThreshold_count(const multimap<float,bool>&,
				const vector<string>&);

    ///
    float SelectThreshold_maxr0(const multimap<float,bool>&,
				const vector<string>&);

    ///
    float SelectThreshold_optr0(const multimap<float,bool>&,
				const vector<string>&);

    ///
    float SelectThreshold_r0_common(const multimap<float,bool>&, bool);

    ///
    bool WriteOrderedClassFile(const list<pair<size_t,double> >&, 
			       const string&, const string&, bool, bool);

    ///
    analyse_result AnalyseFill(const vector<string>&);

    ///
    analyse_result AnalyseInteractive(const vector<string>&);

    ///
    analyse_result AnalyseSearch(const vector<string>&);

    ///
    analyse_result AnalyseSearchVideoTopic(const XmlDom&,
					   const vector<string>&);
					   
    ///
    void SplitSecondsTime(const double secs, int& hour, int& min, int& sec, 
                          int& msec);
    
    ///
    float TimeStringToSeconds(const string& tstr) {
      int min,sec,subsec;
      sscanf(tstr.c_str(),"%dm%d.%ds",&min,&sec,&subsec);
      return min*60+sec+(float)subsec/1000.0;
    }

    ///
    string SecondsToMPEG7MediaTimePoint(const double secs);

    ///
    string SecondsToMPEG7MediaDuration(const double secs);

    ///
    analyse_result AnalyseSearchBest(const string&, const vector<string>&);

    ///
    bool AnalyseSearchAppendTopicList(const XmlDom&);

    ///
    analyse_result AnalyseClass(const vector<string>&);

    ///
    analyse_result AnalyseConcatText(const vector<string>&);

    /// Main function of the tau analyse method
    analyse_result AnalyseSimulate(const vector<string>&);

    /// Main function of the tau analyse method
    analyse_result AnalyseSimulatePerClass(const vector<string>&);

    /// One round of tau analysis
    analyse_result AnalyseSimulateOneRun(const ground_truth&, int,
					 bool, int, bool,
					 const vector<size_t>&,
					 const aspect_map_t&);

    /// Helper for AnalyseSimulateOneRun()
    vector<int> AnalyseSimulateClickedExample(int, const string&);

    /// Helper for AnalyseSimulateOneRun()
    bool AnalyseSimulateMarkSegments(Query*, int, const string&, double, bool);

    /// Helper for AnalyseSimulateClickedExample() and *MarkSegments().
    vector<int> AnalyseSimulateReferenceSegments(int, const string&);

    /// Helper for AnalyseSimulateClickedExample() and *MarkSegments().
    vector<int> AnalyseSimulateExpandSegmentTree(int);

    /// Noise in binary/ternary relevance values.
    class ground_truth_noise {
    public:
      ///
      ground_truth_noise() :
	gt_pn(0), gt_np(0), gt_p0(0), gt_0p(0), gt_0n(0), gt_n0(0) {}

      ///
      string str() const;

      ///
      float gt_pn, gt_np, gt_p0, gt_0p, gt_0n, gt_n0;

    }; // class ground_truth_noise

    ///
    bool SetNoiseValue(const string&, float);

    /// Adds noise in binary/ternary relevance feed values.
    double NoisyRFBvalue(double, double, double, double, double,
                         double, double);

    /// Adds noise in binary/ternary relevance feed values.
    double NoisyRFBvalue(double v) {
      // return NoisyRFBvalue(v, gt_np, gt_pn, gt_0p, gt_0n, gt_p0, gt_n0);
      return NoisyRFBvalue(v, gt_noise);
    }

    /// Adds noise in binary/ternary relevance feed values.
    double NoisyRFBvalue(double, const ground_truth_noise&);

    /// Adds noise in binary/ternary relevance feed values.
    double NoisyRFBvalue(double, double, double, double, double);

    /// Creates vectors from BMU coordinates
    analyse_result AnalyseBMUcoord(const vector<string>&);

    /// Creates an annotated video
    analyse_result AnalyseVideoAnnotate(const vector<string>&);
  
    /// Creates an annotated video
    analyse_result AnalyseVideoTimeline(const vector<string>&);
  
    ///
    analyse_result AnalyseSubtitlesOld(const vector<string>&);

    ///
    analyse_result AnalyseSubtitles(const vector<string>&);

    ///
    analyse_result AnalyseElanize(const vector<string>&);

    ///
    bool CreateElanCsvConfPfsx(size_t, float, const vector<string>&,
			       const vector<pair<string,string> >&,
			       const string&, const string&, const string&);

    ///
    void ElanPfsxAdd(XmlDom&, const string&,
		     const string&, const string&);

    /// 
    analyse_result AnalyseFixOrigins(const vector<string>&);

    /// Creates a video summary
    analyse_result AnalyseVideoSummary(const vector<string>&);

    ///
    analyse_result AnalyseVideoShotSequence(const vector<string>&);

    /// Runs video copy detection
    analyse_result AnalyseVideoCopyDetection(const vector<string>&);

    /// Video file name from label
    string GetVideoFilename(const string&);

    ///
    FloatVectorSet CreateFrameSignatures(const string&, size_t, size_t);

    ///
    void FrameSignatureDetection(const FloatVectorSet&, 
				 const FloatVectorSet&, 
				 simple::FloatMatrix&, size_t);

    /// Calculate distance for frame overlaps using concatenated signatures.
    void FrameSignatureDistanceConcat(const FloatVectorSet&,
				      const FloatVectorSet&, 
				      // FloatVector&,
				      const size_t, const size_t, 
				      float&, size_t&);

    /// Calculate minimum of feature-wise distances for frame overlaps.
    void FrameSignatureDistanceMin(const FloatVectorSet&,
				   const FloatVectorSet&, 
				   FloatVector&,
				   const size_t, const size_t,
				   float&, size_t&);

    /// Extracts a key frame
    analyse_result AnalyseExtractKeyFrames(const vector<string>&);

    /// Inserts all videoframes as objects
    analyse_result AnalyseInsertFrames(const vector<string>&);

    /// Used in for video segments.
    //string SolveSegmentfilePath(const string&) const;

    /// Index of a single frame based on the video object's label
    int SolveFrameIndex(const string&, const size_t, const bool=false) const;

    /// Used in for video segments.
    // using PicSOM::video_frange;
    typedef list<video_frange> video_frange_list_t;

    ///
    video_frange_list_t ReadRangeSegments(const segmentfile&,
					  const string&) const;

    ///
    video_frange_list_t ReadKeyFrames(const segmentfile&) const;

    ///
    bool WriteFrameRanges(const video_frange_list_t&, const string&) const;

    ///
    FloatVector FrameRangeSignature(const video_frange&, const string&,
				    size_t);

    ///
    video_frange_list_t SelectFrameRanges(const video_frange_list_t&,
					  const FloatVectorSet&, 
					  const vector<int>&,
					  const vector<float>&,
					  size_t) const;

    ///
    analyse_result AnalyseVideoStream(const vector<string>&);

    ///
    analyse_result AnalyseConvertDiv(const vector<string>&);

    /// Post processing of pre-calculated results
    analyse_result AnalysePost(const vector<string>&);

    /// Shows the result values.
    bool Show(const analyse_result&, const string& = "", const string& = "");

    /// Plotting utilities
    bool Plot(const analyse_result&, const string& = "", const string& = "");

    /// Plotting utilities, just show values
    bool PlotShow(const analyse_result&,
		  const string& = "", const string& = "");

    /// Plotting utilities, just show some values
    bool PlotShowBySpec(const analyse_result&, const string&);

    /// Plotting utilities, ROC curve
    bool PlotRoc(const analyse_result&, const string& = "",const string& = "");

    /// Plotting utilities, ROC curve
    FloatPointVector PlotRocVector(const analyse_result& r) const {
      return PlotVector(r.false_pos, r.true_pos, 1.0/r.rescount);
    }

    /// Plotting utilities, ROC curve
    bool PlotMinAccRec(const analyse_result&,
		       const string& = "", const string& = "");

    /// Plotting utilities, ROC curve
    FloatPointVector PlotMinAccRecVector(const analyse_result& r) const {
      return PlotVector(r.threshold, r.minaccrec, 1.0/r.rescount);
    }

    /// Plotting utilities, Recall-Precision curve
    bool PlotRP(const analyse_result&, const string& = "", const string& = "");

    /// Plotting utilities, Recall-Precision curve
    FloatPointVector PlotRPVector(const analyse_result& r) const {
      return PlotVector(r.recall, r.precision, 1.0/r.rescount);
    }

    /// Plotting utilities, Recall-Precision curve
    FloatPointVector PlotRPfullVector(const analyse_result& r) const;

    /// Plotting utilities, common to many
    list<string> PlotCommandCommon1(float, float) const;

    /// Plotting utilities, common to many
    bool PlotCommandCommon2(GnuPlot&, float, float, float, float) const;

    /// Plotting utilities, common to many
    bool PlotAdd(GnuPlot&, const analyse_result&, const string&, const string&,
		 const string&);
    
    /// Plotting utilities, common to many
    bool PlotAddInner(GnuPlot&, const analyse_result&, const string&,
		      const string&);
    
    /// Plotting utilities, common to many
    FloatPointVector PlotVector(const vector<float>&, const vector<float>&,
				double) const;

    /// A wrapper for processing of AJAX data from a SOAP connection.
    analyse_result AnalyseAjax(const vector<string>&);

    /// Various tests for SQL timing etc.
    analyse_result AnalyseSqlTest(const vector<string>&);

    /// Various tests for neuraltalk.
    analyse_result AnalyseNeuraltalkTest(const vector<string>&);

    /// A call to DataBase::SqlDump.
    analyse_result AnalyseSqlDump(const vector<string>&);

    /// A call to DataBase::Sql*File*.
    analyse_result AnalyseSqlFile(const vector<string>&);

    /// A call to DataBase::SqlTables.
    analyse_result AnalyseSqlTables(const vector<string>&);

    /// A call to DataBase::UpdateOriginsInfoSqlUpdate.
    analyse_result AnalyseSqlUpdate(const vector<string>&);

    /// inner loop of above one
    bool SqlUpdateOneSet(const vector<string>&);

    ///
    analyse_result AnalyseUpdateVideoInfo(const vector<string>&);

    ///
    analyse_result AnalyseBinDataTest(const vector<string>&);

    ///
    analyse_result AnalyseBinData(const vector<string>&);

    /// Dumps features in raw binary files.
    analyse_result AnalyseBinDump(const vector<string>&);

    ///
    analyse_result AnalyseBinDumpTest(const vector<string>&);

    ///
    analyse_result AnalyseCOCOmasks(const vector<string>&);

    ///
    analyse_result AnalyseMaskedDetection(const vector<string>&);

    ///
    analyse_result AnalyseFeatureTest(const vector<string>&);

    ///
    analyse_result AnalyseEraseFeatures(const vector<string>&);

    ///
    analyse_result AnalyseImportFeatures(const vector<string>&);

    ///
    analyse_result AnalyseImportDatFeatures(const vector<string>&);

    ///
    analyse_result AnalyseImportExternalFeatures(const vector<string>&);

    ///
    analyse_result AnalyseCreateDummyFeatures(const vector<string>&);

    ///
    analyse_result AnalyseEraseDetections(const vector<string>&);

    ///
    analyse_result AnalyseImportDetections(const vector<string>&);

    ///
    analyse_result AnalyseImportExternalDetections(const vector<string>&);

    ///
    analyse_result AnalyseImportBinData(const vector<string>&);

    ///
    bool libsvmdump;
    bool LibsvmDump();

    ///
    analyse_result AnalyseTest(const vector<string>&);

    ///
    analyse_result AnalyseMathTest(const vector<string>&);

    /// Dumps TSSOMs with only labels and as sparse.
    analyse_result AnalyseRewriteMapsLabelsOnly(const vector<string>&);

    ///
    analyse_result AnalysePredictSVM(const vector<string>&);

    ///
    analyse_result AnalyseDetect(const vector<string>& a) {
      return AnalyseSegmentDetectCommon(a, false, true);
    }

    ///
    analyse_result AnalyseSegment(const vector<string>& a) {
      return AnalyseSegmentDetectCommon(a, true, false);
    }

    ///
    analyse_result AnalyseSegmentDetect(const vector<string>& a) {
      return AnalyseSegmentDetectCommon(a, true, true);
    }

    ///
    analyse_result AnalyseSegmentDetectCommon(const vector<string>& a,
					      bool, bool);

    ///
    analyse_result AnalyseSegmentDetectCommonSlaves(const vector<string>& a,
						    bool, bool);

    ///
    analyse_result AnalysePDF(const vector<string>&);

    ///
    analyse_result AnalyseCaptioning(const vector<string>&);

    ///
    analyse_result AnalyseSentenceCandidates(const vector<string>&);

    ///
    analyse_result AnalyseInsertSentences(const vector<string>&);

    ///
    analyse_result AnalyseEvaluateSentences(const vector<string>&);

    ///
    analyse_result AnalyseBLEUdeprecated(const vector<string>&);

    ///
    static vector<float> BLEUvalue(const vector<string>&,
				   const vector<vector<string> >&);

    ///
    static float BLEUvalue(const vector<string>&,
			   const vector<vector<string> >&, size_t);

    ///
    analyse_result AnalyseTextQuery(const vector<string>&);

    ///
    analyse_result AnalyseTextQueryInner(int, const string&, 
					 const textsearchspec_t&,
					 size_t&);
    ///
    string CleanTextQuery(const string&);

    ///
    analyse_result AnalyseDumpFakeTextIndex(const vector<string>&);
    ///
    bool RunAnalyseBestAndCombine(int, vector<textsearchresult_t>&);

    ///
    analyse_result AnalyseTextRetrieve(const vector<string>&);

    ///
    analyse_result AnalyseTextAddField(const vector<string>&);

    ///
    analyse_result AnalyseGoogleCustomSearch(const vector<string>&);

    ///
    analyse_result AnalyseCreateTars(const vector<string>&);

    /// Sets values needed for calculating a priori prob in analyse_result.
    static void AnalyseResultSetSizes(analyse_result& r, double c, double t,
				      double b) {
      r.set_sizes(c, t, b);
    }

    ///
    analyse_result AnalyseTrangeOverlaps(const vector<string>&);

    /// Prints out results of tau/best/etc. analyses
    bool WriteAnalyseResults(const string&, const analyse_result&,
			     const string&, bool, bool, bool = false);
    
    /// Calculates first avgprec and ranking and then calls the above one.
    bool SolveAndWriteAnalyseResults(const string&, analyse_result&,
				     const string&, bool, bool);
    
    ///
    static double TauEstimateMax(const IntVector&, const IntVector&, 
				 int, int, int);  

    ///
    static double TauEstimateMin(const IntVector&, const IntVector&, 
				 int, int, int);  

    ///
    static double TauEstimateMean(const IntVector&, const IntVector&, 
				  int, int, int);  

    ///
    static double TauEstimateFit(const IntVector&, const IntVector&, 
				 int, int, int);  

    ///
    static double TauEstimateTrad(const IntVector&, const IntVector&);

    ///
    static double CalculateTau(const IntVector&, const IntVector&);

    ///
    static double CalculateTau(const IntVector&, const DoubleVector&);

    ///
    static double TauEstimateOrder(const IntVector&, int, int);

    /// Estimate the BIRDS-I performance metric 
    static double TauEstimateBIRDS(const IntVector&, int);

    ///
    static bool ExpFitAndFill(DoubleVector&, int, int);

    ///
    static double MaxPR(const vector<int>&, int);

    ///
    static double Precision(const IntVector&, int = 0, vector<float>* = NULL);

    ///
    static double Recall(const IntVector&, int, int = 0,vector<float>* = NULL);

    ///
    static double AveragePrecision(const IntVector&, int, int = 0,
				   vector<float>* = NULL);

    ///
    static double AveragePrecision(const DoubleVector&, int, int = 0,
				   vector<float>* = NULL);

    ///
    static double InferredAveragePrecision(const DoubleVector&, int c);

    ///
    static double AveragePrecisionEstimate(const IntVector&, int, int, int =0);

    ///
    static double FirstHitAdv(double i, double corr, double tot) {
      return (corr+1)*(i+1)/(tot+1);
    }

    /// A helper routine for the ones below.
    Query *CheckQuery() const { if (!query) SimpleAbort(); return query; }

    ///
    DataBase *GetDataBase() const {
      return query ? query->GetDataBase() : NULL;
    }

    ///
    DataBase *CheckDB() const { return CheckQuery()->CheckDB(); }

    ///
    string ShortFileName(const string& s) const {
      return CheckDB()->ShortFileName(s);
    }

    ///
    target_type Target() const { return CheckQuery()->Target(); }

    ///
    ground_truth& TargetTypeGT() {
      if (q_targettype.empty()) {
	q_targettype = CheckDB()->GroundTruthFunction("$type(qt)", Target());
	// q_targettype.label("xxx");
      }
      return q_targettype;
    }

    ///
    ground_truth& PositiveGT() {
      if (q_positive.empty())
	q_positive = GTExpr(CheckQuery()->Positive());
      return q_positive;
    }

    ///
    ground_truth& NegativeGT() {
      if (q_negative.empty())
	q_negative = GTExpr(CheckQuery()->Negative());
      if (q_negative.empty())
	q_negative = ground_truth(DataBaseSize(), -1);
      return q_negative;
    }

    ///
    ground_truth& ViewClassGT() {
      if (q_viewclass.empty())
	q_viewclass = GTExpr(CheckQuery()->ViewClass());
      return q_viewclass;
    }

    ///
    ground_truth& DataBaseRestrictionGT() {
      if (db_restriction.empty())
	db_restriction = CheckDB()->RestrictionGT();
      return db_restriction;
    }

    ///
    ground_truth& QueryRestrictionGT(bool reset = false) {
      if (q_restriction.empty() || reset)
	q_restriction = CheckQuery()->RestrictionGT(reset);
      return q_restriction;
    }

    /// Just a shorthand for the one below...
    ground_truth GTExpr(const string& c) const {
      return c!="" ? GroundTruthExpression(c) : ground_truth();
    }

    ///
    ground_truth GroundTruthExpression(const string& c) const {
      if (!GetDataBase())
	return ground_truth();
      return GetDataBase()->GroundTruthExpression(c);
    }

    ///
    ground_truth GroundTruthExpression(const string& c, target_type t,
				       int other, bool exp) const {
      if (!GetDataBase())
	return ground_truth();
      return GetDataBase()->GroundTruthExpression(c, t, other, exp);
    }

    ///
    ground_truth_set GroundTruthExpressionListOld(const string& c,
						  bool exp) const {
      if (!GetDataBase())
	return ground_truth_set();
      return GetDataBase()->GroundTruthExpressionListOld(c, exp);
    }

    ///
    bool ReadFiles(bool lo = true) const { return CheckQuery()->ReadFiles(lo);}

    ///
    size_t Nindices() const { return CheckQuery()->Nindices(); }

    ///
    size_t Nfeatures() const { return CheckQuery()->Nfeatures(); }

    ///
    TSSOM& TsSom(size_t i) const { return CheckQuery()->TsSom(i); }

    /// Slightly ugly hack, returns Nlevels if it's a TSSOM, 0 otherwise
    int IndexNlevels(size_t i) const { 
      return CheckQuery()->IsTsSom(i) ? CheckQuery()->TsSom(i).Nlevels() : 1;
    }

    ///
    TreeSOM& Map(int i, int j) const { return CheckQuery()->Map(i, j); }

    ///
    const string& IndexFullName(size_t i) const {
      return CheckQuery()->IndexFullName(i);
    }

    ///
    int RndSeed() const { return CheckQuery()->RndSeed(); }

    ///
    void CbirStages() const { CheckQuery()->CbirStages(); }

    ///
    void CreateIndexData(bool keep) const { CheckQuery()->CreateIndexData(keep); }

    ///
    simple::FloatMatrix& Hits(int i, int j, int k) const {
      return CheckQuery()->Hits(i, j, k); }

    ///
    simple::FloatMatrix& Convolved(int i, int j, int k) const {
      return CheckQuery()->Convolved(i, j, k); }

    ///
    int LabelIndex(const string& l) const {
      return CheckQuery()->LabelIndex(l);
    }

    ///
    const string& Label(int i) const { return CheckQuery()->Label(i); }

    ///
    const char *LabelP(int i) const { return Label(i).c_str(); }

    ///
    const string& DataBaseName() const { return CheckQuery()->DataBaseName(); }

    ///
    size_t DataBaseSize() { return CheckQuery()->DataBaseSize(); }

    ///
    const string& ConvType(int m) { return CheckQuery()->ConvType(m); }

    /// Return true if string can be used as a Matlab variable and filename.
    static bool MatlabNameOK(const string& n) {
      if (!n.size() || !isalpha(n[0]))
	return false;
      for (size_t i=1; i<n.size(); i++)
	if (!isalnum(n[i]) && n[i]!='_')
	  return false;
      return true;
    }

    /// Setting of matlabname.
    void MatlabName(const string& n) {  matlabname_x = n; }

    /// Unexpanded matlabname.
    const string& MatlabName() const { return matlabname_x; }

    /// Expanded matlabname.
    string ExpandedMatlabName(const string& fn = "") const {
      return ExpandFileName("matlab", MatlabName(), fn);
    }

    /// Expanded resultname.
    string ExpandedResultName(const string& fn = "") const {
      return ExpandFileName("result", resultname, fn);
    }

    /// Expands speciers like %f in the argument string.
    string ExpandFileName(const string&, const string&,
			  const string& = "") const;

    /// Creates a file name that can be a suffixed to Matlab variable names.
    string MatlabFileName(int, int = -1, const string& = "") const;

    /// name of output file for some modes
    const string& FileName() const { return filename; }

    /// name of output directory for some modes
    string OutDirEvenDot() const {
      if (outdir=="")
	return "./";

      return *outdir.rbegin()=='/' ? outdir : outdir+'/';
    }

    /// Adds textual output to be passed to the browser.
    bool AddToXML(XmlDom&) const;

    /// Adds textual output to be passed to the browser.
    bool AddToXMLvariables(XmlDom&) const;

    /// Adds textual output to be passed to the browser.
    bool AddToXMLsearchtasks(XmlDom&) const;

    /// Adds textual output to be passed to the browser.
    bool AddToXMLxhtml(XmlDom&) const;

    /// Adds textual output to be passed to the browser.
    bool AddToXMLanalyse_result(XmlDom&, const analyse_result&) const;

    /// Adds textual output to be passed to the browser.
    bool AddToXMLxml_result(XmlDom&) const;

    /// Initializtion of xml_result.
    bool InitializeXmlResult(bool warn);

    /// Access to xml_result.
    XmlDom XmlResult() { return xml_result; }

    ///
    bool HasXmlResult() const { return xml_result.DocOK(); }

    /// Adds <error> element in xml_result.
    XmlDom SetXmlResultError(const string& msg = "");

    /// Adds <status> element in xml_result if empty.
    XmlDom SetXmlResultOK();

    /// Checks xml_result for <error>
    bool XmlResultIsError() const {
      return xml_result.Root().FirstElementChild().NodeName()=="error";
    }

    /// Checks xml_result for <error>
    string XmlResultErrorText() const {
      return XmlResultIsError() ?
        xml_result.Root().FirstElementChild().FirstChildContent() : "";
    }

    ///
    bool CopyXmlResultTo(Analysis*) const;

    /// Access to xslname.
    const string& XslName() { return xslname; }
    
    /// Creates new XHTML/HTML content node.
    void CreateXHTML() {
      if (xhtml)
	xmlFreeNode(xhtml);
      xhtml = xmlNewNode(NULL, (XMLS)"xhtml");
    }

    /// Returns name of the optimization scalar value.
    const string& OptimizeVariable() const { return optimize; }

    const string& FeatSelArgument() const { return featselarg; }

    /// Returns optimization scalar value and value proper for minimization.
    pair<float,float> OptimizeValue(const analyse_result& r);

    string AveragePrecisionMode() const {
      if (optimize.find("avgprec")!=0)
	return "";
      
      string mode = optimize.substr(AveragePrecisionInferred() ? 8 : 7);
      return mode.substr(0, mode.find('('));
    }

    bool AveragePrecisionInferred() const {
      return optimize.length()>7 && optimize.substr(7,1) == "i";
    }

    int AveragePrecisionDivider(int csum, int rounds) const {
      const string mode = AveragePrecisionMode();

      if (mode.empty() || (csum==rounds))
	return csum;
      else if (mode == "*")
	return rounds;
      else if (mode == "*?" || mode == "?*")
	return rounds<csum ? rounds : csum;
      else 
	ShowError("Bad average precision mode: "+mode);
      return csum;
    }    

    /// Reads average precision from the analyse_result.
    double AvgPrecision(const analyse_result& r, int i) const {
      if (!r.ok() || r.empty()) {
	ShowError("Average precision not readable from result");
	return -1;
      }
      // NOTE THE INDEXING!
      return AveragePrecisionInferred() ? r.inferredap/r.rescount :
 	i>0 && i<=int(r.avgprecision.size()) ?
 	r.avgprecision[i-1]/r.rescount : -1;
    }
    
    ///
    bool ReadResultTrecvid(const string&, const string&, const string&); 

    ///
    bool ReadStatsTrecvid(const string&, const string&, const string&); 
    
    ///
    bool ReadRankTrecvid(const vector<string>& args) {
      int argc = args.size();
      if (argc < 2 || argc > 4)
	return ShowError("Usage: trecvid[200?](run label, topic-number "
			 "[, results-file, stats-file])");
      const string& runlab = args[0];
      const string& topicnum = args[1];
      const string& results_file = (argc>2?args[2]:"search.results.table.web");
      const string& stats_file = (argc>3?args[3]:"search.stats.web");

      DataBase *db = CheckDB();
      string results_path = db->ExpandPath("results", results_file);
      string stats_path = db->ExpandPath("results", stats_file);
    
      bool ok = ReadResultTrecvid(results_path, runlab, topicnum) &
	ReadStatsTrecvid(stats_path, runlab, topicnum);
      if (ok) rank_mode = "trecvid";

      return ok;
    }

    bool ReadRankVoc2005(const vector<string>& /*args*/) {
      return true;
    }

    float RankRound(float f) { 
      return floor((f*1000)+0.5)/1000; 
    }

    int GetRanking(float f) {
      // round to three decimal places
      float fr = RankRound(f);
      int rank=rank_stats.size();
      for (multiset<float>::iterator mit = rank_stats.begin(); 
	   mit != rank_stats.end(); mit++) {
	if (fr <= *mit) return rank;
	rank--;
      }	
      return rank;
    }

    /// Sets invocation specific parameters in batch loop.
    void BatchParameters(const string& l, const Analysis* a = NULL) {
      string p = a ? a->BatchParameters() : "";
      batchparameters = p+(p==""?"":" ; ")+l;
    }

    /// Read access to invocation specific parameters in batch loop.
    const string& BatchParameters() const { return batchparameters; }

    /// An utility useful together with Query::WriteAnalyseVariablesNew().
    void AddExtraVariableInfo(list<string>& e,
			      const string& k, const string& v) const {
      if (v!="")
	e.push_back(k+"="+v);
    }

    /// An utility useful together with Query::WriteAnalyseVariablesNew().
    static void AddGroundTruthInfo(ground_truth_list& gtl, const string& name,
				   const ground_truth& gt,
				   float v = numeric_limits<float>::max()) {
      gtl.push_back(make_pair(name, make_pair(gt, v)));
    }

    /// An utility useful together with Query::WriteAnalyseVariablesNew().
    static void AddGroundTruthInfoFirst(ground_truth_list& gtl,
					const string& name,
					const ground_truth& gt,
					float v =
					numeric_limits<float>::max()) {
      gtl.push_front(make_pair(name, make_pair(gt, v)));
    }

    /// An utility useful together with Query::WriteAnalyseVariablesNew().
    static string FormatAspectName(const string& name, const string& a) {
      return name+(a==""?"":"."+a);
    }

    /// An utility useful together with Query::WriteAnalyseVariablesNew().
    static void AddAspectInfo(ground_truth_list& gtl, const string& name,
			      const aspect_map_t& aspect) {
      for (aspect_map_t::const_iterator a=aspect.begin(); a!=aspect.end(); a++)
	AddGroundTruthInfo(gtl, FormatAspectName(name, a->first),
			   a->second.first, a->second.second);
    }

    /// comma-separated list of aspect names.
    static string AspectNames(const aspect_map_t& a) {
      list<string> s;
      for (aspect_map_t::const_iterator i=a.begin(); i!=a.end(); i++)
	s.push_back(i->second.first.label());
      return CommaJoin(s);
    }

    /// Returns true for results of AnalyseBest().
    bool SkipObjectLists() const { return skipobjectlists; }

    /// Called in the end of Query::CbirStages().
    bool CbirStagesEnded(const Query*);

    ///
    static pair<string,string> ElapsedTime(const timespec&, const timespec&);

    /// Sets elapsed_time.
    bool SetElapsedTime(const timespec&, const timespec&);

    /// Sets elapsed_time.
    bool SetElapsedTime(const Query*);

    /// Sets elapsed_time.
    bool SetElapsedTime(const Query *a, const Query *b, bool save) {
      return SetElapsedTime(a->StartTime(), save ? b->SavedTime()
			    : b->LastModificationTime());
    }

    /// Returns depth of the instance in the analysis stack.
    size_t Depth() const { return parent ? 1+parent->Depth() : 0; }

    /// Shows a trace of the analysis stack.
    string TaskStack() const {
      string ret = parent ? parent->TaskStack()+"+" : "";
      if (batchblock!="")
	ret += "<"+batchblock+">";
      if (crossval!="")
	ret += "{"+crossval+"}";
      ret += method;
      if (batchparameters!="")
	ret += "["+batchparameters+"]";
      return ret;
    }

    /// Shows depth and trace of the analysis stack.
    string DepthAndTaskStack() const { return ToStr(Depth())+" "+TaskStack(); }

    ///
    void ConditionallyWriteResult(const analyse_result& r) {
      if (resultname=="")
	return;

      string resname = ExpandedResultName()+".xml";
      r.write(resname);
      WriteLog("Wrote analysis results in <"+resname+">");
    }

    /// Sets to use pthreads.
    static void UsePthreads(int n) {
#ifndef ANALYSIS_USE_PTHREADS
      if (n)
	ShowError("UsePthreads(int) called while not ANALYSIS_USE_PTHREADS");
#endif
      use_pthreads = n;
    }

    /// Whether to use pthreads.
    static int UsePthreads() {
      return use_pthreads;
    }

    /// Sets to use pthreads in insert.
    static void UsePthreadsInsert(bool u) {
#ifndef ANALYSIS_USE_PTHREADS
      if (u)
	ShowError("UsePthreadsInsert(true) "
		  "called while not ANALYSIS_USE_PTHREADS");
#endif
      use_pthreads_insert = u;
    }

    /// Whether to use pthreads in insert.
    static bool UsePthreadsInsert() {
      return use_pthreads_insert;
    }

    /// Sets debugging of script related operations.
    static void DebugScript(bool d) { debug_script = d; }

    /// Expands $$ in script to specied string value.
    string ExpandDollars(const string& in, const string& r) const {
      string out = in;
      for (;;) {
	size_t i = out.find("$$");
	if (i==string::npos)
	  return out;
	out.replace(i, 2, r);
      }
    }

    ///
    bool StoreMatrices() const { return store_matrices; }

    ///
    void StoreMatrices(bool s) { store_matrices = s; }

    ///
    bool StoreMatrices(analyse_result&) const;

    ///
    bool PrepareRawData(TSSOM&, TSSOM::raw_data_spec_t&, DataSet&);

    ///
    bool SetRawPreProcess(TSSOM::raw_data_spec_t&, DataSet&);

    ///
    const string& Method() const { return method; }

    ///
    bool IsInsert() const { return method=="insert" || method=="sqltest"; }

    ///
    bool IsCreate() const;

    ///
    bool ProcessObjectRequest(object_type, const string&, const string&, int&);

    /// Starts cpu timing for given function.
    void Tic(const char *f) { picsom->Tic(f); }

    /// Stops cpu timing for given function.
    void Tac(const char *f = NULL) { picsom->Tac(f); }

    ///
    bool StoreErfRelevances(const Query*);

    ///
    bool ErfRelevancesToROC();

    ///
    bool DoCallback(const string&);

    ///
    float Threshold() const { return threshold; }

#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
    ///
    analyse_result AnalyseCreateLevelDB(const vector<string>&);

    ///
    analyse_result AnalyseCaffeTest(const vector<string>&);
#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE

#if defined(HAVE_THC_H) && defined(PICSOM_USE_TORCH)
    ///
    analyse_result AnalyseTorchTest(const vector<string>&);
#endif // HAVE_THC_H && PICSOM_USE_TORCH

    ///
    const PicSOM::detection_stat_t& DetectionStat() const {
      return detection_stat;
    }

    ///
    const string& Classname() const { return classname; }
    
    ///
    vector<string> Detections() const { return detections; }
    
    ///
    vector<string> ExpandDetectionsWithClasses(const vector<string>&,
					       const list<string>&,
					       bool = true);

    ///
    vector<string> ExpandDetectionsWithClasses(const vector<string>&,
					       const string&);

  protected:
    /// Link to the outside universe.
    PicSOM *picsom;

    /// Link to up in analysis stack.
    const Analysis *parent;

    /// Most of the data is in Query class object.
    Query *query;

    /// False if PicSOM class owns the Query object.
    bool query_own;

    /// True if cin can be read.
    bool has_cin;

    /// This is used to collect options in ProcessArguments().
    DataBase *dummy_db;
    
    /// This is used to collect options in ProcessArguments().
    TSSOM *dummy_tssom;
    
    /// Analyse method's name.
    string method;

    /// Analysis' name identifier
    string idname;

    /// Result of the analysis.
    analyse_result result;

    /// Output should be written to *aout instead cout...
    ostream *aout;

#ifdef USE_MRML
    /// !! for MRML implementation of Analysis class !!
    /// Pointer to connection class (MRML uplink)
    Connection *mrml_connection;

    /// !! for MRML implementation of Analysis class !!
    /// MRML server address in form "hostname:port"
    string mrmlserver;

#endif //USE_MRML

    /// Last arguments that were not processed prior Analyse().
    vector<string> rest_argv_str;

    /// Commands as reads line-per-line from script files.
    list<string> script;

    /// Commands stored on one line.
    string commands_str;

    /// Verbosity level, 1 is default.
    int verbose;

    /// 0, 1, 2 or 0-0, 0-1, 0-2, ... identifier for loops
    string loopid;

    /// Needed in SOM training.
    string treesomstruct;

    /// Needed in SOM training.
    bool multirescorrect;

    /// Needed in SOM training.
    IntVector bmudivdepth;

    /// Needed in SOM training.
    IntVector traincount;

    /// Needed in SOM training.
    string rawdata;

    /// Calculates and dispalys AQE
    bool avgquanterror;

    /// Selects neighbourhood kernel for SOM training
    int neighkernel;

    /// Selects radius base for SOM training
    float radiusbase;

    /// Number of vq units for lbg.
    int units;

    /// Number of Vectors after random projection (0 if not used).
    int randproj;

    /// If non-empty, Create() will concatenate features
    string concatfeat;

    /// If non-empty, Create() will call CreateELM().
    string elm;

    /// If non-empty, Create() will call CreateSVM().
    string svm;

    /// Parameters beginning with svm_ given to SVM::Create()
    map<string,string> svm_params;

    ///
    string instance;

    /// If true, data generated from metadata is written to .dat file.
    bool writedata;    

    /// If set, Create() will perform feature extraction.
    bool extractfeatures;

    /// Force it.
    bool recalculatefeatures;

    /// Number of right-most label digits in each feature extraction batch.
    int featextbatchsize;

    ///
    bool dodetections;

    ///
    bool dosegments;

    ///
    bool extractmedia;
 
    /// If set, Create() will ...
    bool combineorphans;

    /// If set, Create() will ...
    bool removecreated;

    /// If set, Create() will ...
    bool updatediv;

    ///
    vector<string> odlist;

    /// True if database is to be updated by its origins data.
    bool fillorigins;

    /// True if only reads maps and then writes labeless cod files.
    bool stripvectors;

    /// True if only mean distance between vectors should be solved.
    bool meandist;

    /// True if Linde-Buzo-Grey quantization should be created.
    bool lbg;

    /// True if scalar quantization ord file should be created.
    bool ord;

    /// Name of alien database to use for division file creation.
    string alien_data;

    /// Name of alien database to use for division file creation.
    string alien_map;

    /// True if batch training is requested for.
    bool batchtrain;

    /// Batch training details.
    string batchtrainperiod;

    /// True if new labels can be appended in labels.
    bool addlabels;

    /// True if write zipped files.
    bool zipped;
    
    ///
    bool divxml;

    /// True if write only labels of vectors. Default is false.
    bool labelsonly;

    /// True if write also U-matrices of the SOMs. Default is true.
    bool saveumatrix;

    /// GroundTruthExpression()ed list of images not use in SOM training.
    string skiplabels;

    /// The class that is analyzed;
    string classname;
    
    /// Used in stripping the prefix.
    string classprefix;

    /// Threshold file name pattern for detections.
    string thresholds;

    /// The set used in intializing queries in tau analysis.
    string testsetname;

    /// Image set whose scores are dumped every round during AnalyseSimulate
    string watchclassname; 

    /// Spoofing method various Analyse*() methods
    string simulate;

    /// Description of the cross validation scheme.
    string crossval;

    /// For SVM::Create()
    string featureaugmentation;

    /// The level of maps to be analyzed, -1 for all.
    int level;

    /// Count of cbir rounds before stop.
    int rounds;

    /// Count of individual runs.
    int classrounds;

    /// Count of pseudo relevance feedback rounds.
    int prfrounds;

    /// Count of objects marked positive in pseudo relevance feedback.
    int prfobjects;

    /// Should we force hit by rndseed.
    bool forcehit;

    /// Should we force hit by rndseed.
    int firstforcedidx;

    /// Should we remove bias caused by forcing initial images. 
    bool usebias;

    /// Should hits be convolved.
    bool convolved;

    /// Should we logarihmize responses.
    bool logarithm;

    /// Threshold value for umatrix images (and also for other things).
    float threshold;

    /// Fraction of the original video length preserved in a summary.
    float summarylength;

    /// Length of clips included in a summary, in seconds.
    float summarycliplength;

    /// Integer magnifier for image size in MakeImage().
    int magnify;

    /// The count of histogram bins in AnalyseDiv().
    int histbins;

    /// The count of segments in keyword focusing of AnalyseFocus().
    int segments;

    /// Name of the optimization variable.
    string optimize;

    ///
    string featselarg;

    ///
    string segmentspec;

    ///
    string segmentexpand;

    /// Type of block if executing one.
    string batchblock;

    /// Parameter values specific for this invocation in batch.
    string batchparameters;

    /// Should we write image to file.
    string imagefilename;

    /// Additional image format specifiers.
    string imagespec;

    /// Size and border specifiers for collagemap thumbnails
    string maptnspec;

    /// The list of overlaps tested in video copy detection
    string vcd_overlaps;

    /// The fusion method used in video copy detection
    string vcd_fusionmethod;

    /// Start and stop for argument list in video copy detection
    int vcd_argstart;
    int vcd_argstop;

    /// Should we fork to display if imagefile=="-".
    bool forkdisplay;

    /// Should we write results in HTML file in AnalyseBest().
    string htmlfile;

    /// Should we link or copy images contained in the HTML file above.
    /// 0=no, 1=link, 2=copy
    int htmlfile_linkimages;

    /// filename for output files, used at least by concatfeat, 
    /// concatmapcoord, createsegmenthistogram
    string filename;

    /// name of output directory in CreateSOM() et al.
    string outdir;

    /// Prefix of matlab file name to write the results.
    string matlabname_x;

    /// Prefix of result files in AnalyseBest().
    string resultname;

    /// Used in AnalyseImportFeatures().
    string sourcedatabase;

    /// Used in AnalyseFillZeroFeatureData().
    string sourcefeature;

    /// Limits on click counts in AnalyseSimulate().
    size_t clicks_min, clicks_max;

    ///
    float bgcolor;

    /// Prefix of trecvid XML result file name to write the results.
    string trecvid;

    ///
    string trecvid_priority;

    ///
    string trecvid_runid;

    /// Whether to process {image,video,audio}Examples from TRECVID topics
    bool trecvid_noexamples;

    /// Whether to use aspects ("example","concept") in TRECVID search
    bool trecvid_useaspects;

    /// 
    bool trecvid_noprogressbar;

    ///
    string treceval;

    ///
    string imagenet_normalize;

    ///
    string imagenet_func;

    ///
    string imagenet_specialpre;

    ///
    string imagenet_inputprefix;

    ///
    string imagenet_rootclass;

    ///
    string imagenet_allcl;

    /// Name of trecvid's XML file containing <videoTopic> elements.
    string topics;

    /// 
    string metric_weights;

    /// Metric used in training the SOMs.
    string metric;

    /// Suffix for save name.
    string namesuffix;

    /// Use of reference comparison in DistanceSquaredXX.
    int distcomp;

    /// Compression value for ConfusionMatrix.
    float compress;

    /// Number of top candidates in classification.
    size_t ntop;

    /// Classifier specifier, eg knn<k> where <k> is k value of k-NN.
    string classifier;
    
    /// reject-k value of k-NN.
    string knn_reject;

    /// Different vector lengths to test classifiers like k-NN with.
    string vectorlength;

    /// Preprocessing and normalizations to the data.
    string preprocess;

    /// Procedure for vector component ordering.
    string componentorder;

    /// The field name in OriginsInfo used in CreateGroundTrurh().
    string field;

    /// The field value expression used in CreateGroundTrurh().
    string value;
    
    /// Used for checking the debug information before actual storage.
    bool dryrun;

    /// True if AnalyseSimulate() can truncate queries.
    bool truncate;

    /// How many best of all images to show in AnalyseBest().
    int bestall;

    /// How many best of positive images to show in AnalyseBest().
    int bestpositive;

    /// How many best of non-positive images to show in AnalyseBest().
    int bestother;

    /// Number of object/value pairs to include in AnalyseResultsCommon()
    int resultcount;

    /// Name of ordered set that is used as the first best objects in 
    /// AnalyseBest().
    string forcedbest;

    /// Set to true if AnalyseBest should return only obejcts defined by 
    /// forcedbest.
    bool forcedonly;

    /// true if less than maxquestions images can be 
    /// accepted as retrieval result
    bool relaxcounttest;

    /// true if AnalyseBest results are not needed
    bool skipresults;

    /// number of objects to be inserted if >=0
    int insertcount;

    /// number of objects really inserted
    int count_inserted;

    /// true if InsertDirectory() should be recursive
    bool recursive;

    /// true if InsertImageNetSynset() should skip already existing synsets
    string skipexisting;

    ///
    bool skipnonexistent;

    /// comma-separated list of directories to run insert in
    string insertdirs;

    /// Results in a form formatted for XHTML browsers.
    xmlNodePtr xhtml;

    /// true if old GroundTruthExpand() is desired.
    bool expand;

    /// true if AnalyseGT() should keep objects in the order they are listed
    /// in the class file.
    bool keeporder;

    /// true if AnalyseGT() should plot directory-wise info on lines.
    bool dirplot;

    /// search topics a la trecvid
    list<pair<string,string> > searchtasks;

    /// set to true if Query::AddToXMLobjects() should not show objectlists.
    bool skipobjectlists;

    /// used in CreateGroundTruthQuery() and AnalyseVideoCopyDetection()
    string queryname;

    /// used in CreateGroundTruthQuery().
    bool unseenzero;

    /// Time when interactive analysis was started.
    // timespec start_time;

    /// String of time in minutes.
    string elapsed_time;

    ///
    multiset<float> rank_stats;
    
    ///
    float old_rank_value;

    ///
    string rank_mode;

    ///
    list<string> featurechoices;

    ///
    bool check_first_feature_only;

    ///
    ground_truth q_targettype;

    ///
    ground_truth q_positive;

    ///
    ground_truth q_negative;

    ///
    ground_truth q_viewclass;

    ///
    ground_truth q_restriction;

    ///
    ground_truth db_restriction;

    ///
    ground_truth gt_trainset;

    ///
    ground_truth gt_testset;

    /// shot boundary determination parameters
    struct sbdparam_t {
      float votepercent;
      size_t nframes;
      size_t nframes_next;
      size_t framegap;
      size_t mindist;
      vector<float> boundaryexp;
      string gtfile;
      double gradualoptpercent; //gradual vs. abrupt cut weight in optimization
      vector<float> stepsize;
      vector<float> feature_weight;
      double precision_weight; //precision vs. recall weight in optimization
    };
    sbdparam_t sbdparams;

    ///
    string plot;

    ///
    vector<string> gnuplot_extra;
    
    ///
    string scoredump;

    ///
    bool scoredumponly;

    ///
    string scoredat;

#ifdef ANALYSIS_USE_PTHREADS
    ///
    typedef list<analyse_pthread_data_t> thread_data_t;

    ///
    thread_data_t thread_data;
#endif // ANALYSIS_USE_PTHREADS

    /// To use or not to use or how many pthreads.
    static int use_pthreads;

    /// To use or not to use pthreads in insert.
    static bool use_pthreads_insert;

    /// Debugging script operations?
    static bool debug_script;    

    ///
    bool store_matrices;

    ///
    float classmargin;

    ///
    size_t columns;

    ///
    XmlDom xml_result;

    ///
    string xslname;
    
    ///
    string pairdistancekernel;

    ///
    ground_truth_noise gt_noise;

    ///
    ground_truth_noise gt_gaze_noise;

    ///
    map<string,map<size_t,float> > erf_rel;

    /// Noise to be added in NoisyRFBvalue().
    RandVar rfbnoise;    

    ///
    vector<string> detections;

    ///
    vector<string> captionings;

    ///
    // vector<string> evaluations;

    ///
    vector<string> segmentations;

    ///
    string callback;

    ///
    bool elanconvert;

    ///
    bool elanoverwrite;

    ///
    float alpha, beta;

    ///
    size_t slave_features_missing;

    ///
    size_t slave_features_requested;

    ///
    size_t slave_detections_stored;

    ///
    size_t slave_captions_stored;

    ///
    size_t slave_features_stored;

    ///
    size_t slave_objectinfos_stored;

    ///
    bool optimize_expand_line;

    ///
    size_t hard_negative_mining;    

    ///
    PicSOM::detection_stat_t detection_stat;

    ///
    string lscommap;

    ///
    bool keeptmp;

    ///
    bool tolerate_missing_features;

    ///
    string mapgrid;

    ///
    map<string,map<size_t,textline_t> > insert_objects_textline;

  };  // class Analysis


  // an evaluator object to evaluate the performance
  // of a script for different weightings of the last feature
  // in the script

  class optEvaluator:public floatEvaluator{
  public:
    optEvaluator(Analysis *a, std::list<std::string> *b,
		 std::list<std::string> *f,
		 std::string *nf, std::string *ws){
      analysis=a;
      before=b;
      feats=f;
      new_feat=nf;
      weightstring=ws;
      firstresult=true;
    }

    virtual ~optEvaluator(){}


    virtual float eval(float x){
      cout << "optEvaluator::eval("<<x<<")=";
      string newbase=new_feat->substr(0,new_feat->find('('));
      list<string> a;
      string w; 
      char weightstr[80]; 
      sprintf(weightstr,"; weight(%s)=%f",newbase.c_str(),x);
      w=weightstr; 
      a.push_back("features="+analysis->JoinFeaturesString(*feats)+","+
		  newbase+*weightstring+w);
    
      list<string> script=analysis->ScriptJoinBlocks(*before,"",a);
 
      //cout << "optEvaluator::eval() Executing script " << endl; 
      //analysis->DisplayScript(script);
      analysis->Script(script);
      Analysis::analyse_result res = analysis->Analyse(); 
    
      if (!res.ok())
	return /*analysis->*/ ShowError("optEvaluator : Analyse() failed");
      pair<float,float> opt = analysis->OptimizeValue(res);      
      if(firstresult){
	bestresult=res;
	firstresult=false;
      } else{
	pair<float,float> o_opt = analysis->OptimizeValue(bestresult);
	if(opt.first>o_opt.first)
	  bestresult=res;
      }
      cout << -opt.first << endl;
      return -opt.first;
    }
  
    Analysis::analyse_result bestresult;
    bool firstresult;

  protected:
    Analysis *analysis;
    std::list<std::string> *before;
    std::list<std::string> *feats;
    std::string *new_feat;
    std::string *weightstring;
  }; // class optEvaluator
  
  
} // namespace picsom

#endif // _PICSOM_H_

