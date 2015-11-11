// -*- C++ -*-  $Id: Classifier.h,v 1.20 2014/07/01 13:40:56 jorma Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _CLASSIFIER_H_
#define _CLASSIFIER_H_

#include <Vector.h>
#include <VectorSet.h>
#include <ConfusionMatrix.h>
#include <StatQuad.h>
#include <GnuPlot.h>
#include <OptVar.h>
#include <TicTac.h>

#if SIMPLE_FEAT
#include <Transform.h>
#endif

#if SIMPLE_MISC_DECAY
#include <DecayVar.h>
#endif // SIMPLE_MISC_DECAY

#include <regex.h>
#include <vector>
#include <string>
#include <map>
#include <set>

using namespace std;

enum TestMode {
  TrainError = 1, TestError = 2,
  TrainTestError = TrainError+TestError,
  Reject = 4,
  TrainRejectError = Reject+TrainError,
  TestRejectError = Reject+TestError,
  TrainTestRejectError = Reject+TrainTestError,
  FinalOnly = 8,
  FinalOnlyTrainError = FinalOnly+TrainError,
  FinalOnlyTestError = FinalOnly+TestError,
  FinalOnlyTrainTestError = FinalOnly+TrainTestError,
  FinalOnlyTrainRejectError = FinalOnly+TrainRejectError,
  FinalOnlyTestRejectError = FinalOnly+TestRejectError,
  FinalOnlyTrainTestRejectError = FinalOnly+TrainTestRejectError,
  InitialToo = 16,
  InitialTooTrainError = InitialToo+TrainError,
  InitialTooTestError = InitialToo+TestError,
  InitialTooTrainTestError = InitialToo+TrainTestError,
  InitialTooTrainRejectError = InitialToo+TrainRejectError,
  InitialTooTestRejectError = InitialToo+TestRejectError,
  InitialTooTrainTestRejectError = InitialToo+TrainTestRejectError
};

namespace simple {
class Classifier : protected FloatVectorSet {
public:
  Classifier(const char *n = NULL, int i = -1) : FloatVectorSet(n, i) {
    Init(NULL, FALSE); }
  Classifier(const Classifier& cls) : SourceOf<FloatVector>(cls),
				      FloatVectorSet(cls)  {
    Init(NULL, FALSE); }
  Classifier(const FloatVectorSet& set/*, int own = TRUE*/)
    : FloatVectorSet(set/*, own*/) { Init(NULL, FALSE); }

  virtual ~Classifier();

  Classifier& Init(const Classifier*, int);

  Classifier& CopySome(const Classifier& c, int cv_too) {
    FloatVectorSet::operator=(c); return Init(&c, cv_too); }

  Classifier& operator=(const Classifier& c) { return CopySome(c, TRUE); }

  virtual void Dump(Simple::DumpMode dm=DumpDefault, ostream& os=cout) const;
  void DumpCrossValidation(Simple::DumpMode, ostream&) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "Classifier", "VectorSet", "List", NULL }; return n; }

  FloatVectorSet& VectorSetPart() { return *this; }

  static Classifier *CreateFromName(const char*);
       
  virtual int Configuration(const char*, const char** = NULL);
  virtual const char *Configuration() const;
  virtual const char *Identity(const char* = NULL);

  void PreAccuracyText(const char *t) { CopyString(pre_accuracy_text, t); }
  const char *PreAccuracyText() const { return pre_accuracy_text; }

  int IsCorrectlyClassified(const FloatVector& vec) {
    return vec.LabelsMatch(ClassifyVector(vec)); }

  FloatVectorSet ClassifiedTo(FloatVectorSource&, const char*);
  FloatVectorSet ClassifiedWrongly(FloatVectorSource&);

  const FloatVector *RawVector() const { return raw_vector; }

  virtual const char *ClassifyRawVector(const FloatVector&, float* = NULL);

  virtual const char *ClassifyVector(const FloatVector&, float* = NULL);
  StatVar Classify(FloatVectorSource&, const char* = NULL);
  StatVar Classify();

  int DistanceComparisonInterval() const {
    return FloatVectorSet::DistanceComparisonInterval(); }

  Classifier& DistanceComparisonInterval(int u) {
    FloatVectorSet::DistanceComparisonInterval(u);
    for (int i=0; i<NCrossValidation(); i++)
      cross_val[i].DistanceComparisonInterval(u);
    return *this;
  }

  FloatVectorSource *CurrentSource() const { return current_source; }
  void CurrentSource(FloatVectorSource *s) { current_source = s; }

  void DoOldCrossValidation(FloatVectorSource&);

  virtual void InitializeParameters() {}

  virtual int GetNitems() const { return Nitems(); }
  virtual int FindBestMatch(const FloatVector&, float* = NULL,
			    IntVector* = NULL) const;

  virtual bool AdaptVector(const FloatVector&);
  virtual void AdaptEpochDone(FloatVectorSource&) {}
  virtual bool BatchTrainEpochDone(const FloatVectorSet&,
				   const ListOf<IntVector>&);

  void BatchTrainNoThreads(IntVector&, FloatVectorSet&, FloatVector*,
			   FloatVectorSet*, int, int&);

  bool CreateBackReferences(int, int, const IntVector&, ListOf<IntVector>&);

#ifdef CLASSIFIER_USE_PTHREADS
  int BatchTrainThreads() const { return batchtrain_threads; }

  void BatchTrainThreads(int n) { batchtrain_threads = n; }

  struct batch_train_pthread_data {
    pthread_t pthread; // thread id
    bool pthread_set;
    Classifier *cfr;   // this becomes this
    FloatVector vec;   // the best of this will be found
    int vecidx;        // index of vector in data set
    int bmu;           // return value;
    float dist;        // return value;
  };

  void BatchTrainPthread(IntVector&, FloatVectorSet&, FloatVector*,
			 FloatVectorSet*, int, int&);

  static void *BatchTrainPthread(void *data) {
    batch_train_pthread_data *p = (batch_train_pthread_data*)data;
    p->bmu = p->cfr->BatchTrainPthread(p->vec, &p->dist);
    return data; }

  int BatchTrainPthread(const FloatVector& v, float *d) {
    return FindBestMatch(v, d); }    
#endif // CLASSIFIER_USE_PTHREADS

  bool AdaptNew(bool, bool, bool, bool, bool, double&, double&, 
                bool lastiter=false);

  bool AdaptTraining();

  bool AdaptDivide(IntVectorSet&, FloatVector&, FloatVectorSet&,
		   double&, double&);

  virtual bool AdaptSetDivide(const IntVectorSet&, const FloatVector&);

  virtual bool AdaptWriteDivide(bool lastiter=false) const;

  virtual bool AdaptVote(const IntVector&, const FloatVector&,
			 const FloatVectorSet&);

  virtual bool AdaptWrite();

  bool BatchTrain(IntVector*, FloatVector*, FloatVectorSet*,
		  double&, double&);
  bool PerVectorTrain(IntVectorSet*, FloatVector*, FloatVectorSet*,
		      double&, double&);

  bool SkipLabel(const char *l) const {
    return l && skiplabelsset.find(l)!=skiplabelsset.end();
//     for (size_t j=0; j<skiplabels.size(); j++)
//       if (skiplabels[j]==l)
// 	return true;
//     return false;
  }

  int CountNotSkipped(FloatVectorSource& s) const {
    if (skiplabelsset.empty())
      return s.Nitems();

    s.First();
    int n = 0;
    for (const FloatVector *v; (v=s.Next());)
      if (!SkipLabel(v->Label()))
	n++;

    return n;
  }

  void Adapt();
  void Adapt(FloatVectorSource&);

  void Adapt1(const FloatVector&, int, double);
  void Adapt2(const FloatVector&, int, double);
  void Adapt3(const FloatVector&, int, double);
  void Adapt4(const FloatVector&, int, double);

  const ConfusionMatrix& GetConfusionMatrix() const { 
    return cfmtrxs[current_cfmtrx]; }
  ConfusionMatrix& GetConfusionMatrix() { return cfmtrxs[current_cfmtrx]; }

  void ClearConfusionMatrix() { GetConfusionMatrix().Clear(); }
  void SortConfusionMatrix() const { GetConfusionMatrix().Sort(); }

  void AddArbitraryToConfusionMatrix() { GetConfusionMatrix().AddArbitrary(); }

  void AddToConfusionMatrix(const FloatVector& v, const char *d = NULL) {
    AddToConfusionMatrix(v.Label(), d); }

  void AddToConfusionMatrix(const char *s, const char *d) {
    GetConfusionMatrix().Add(s, d); }

  void AddToConfusionMatrix(const ConfusionMatrix& m) {
    GetConfusionMatrix().Add(m); }

  void DisplayConfusionMatrix(ostream& os = cout,
			      const char** text = NULL) const {
    GetConfusionMatrix().Display(os, text?text:cfmtrx_text); }

  void DisplayConfusionAccuracy(int tail = 0, ostream& os = cout) const {
    GetConfusionMatrix().DisplayAccuracy(tail, os, cost); }

  void SetConfusionMatrixTexts(const char **text) { cfmtrx_text = text; }

  void CompressConfusionMatrix(double c) {
    for (int i=0; i<(signed)sizeof(cfmtrxs)/(signed)sizeof(*cfmtrxs); i++)
      cfmtrxs[i].Compress(c);
  }

  double CostRatio() const {
    if (cost)
      return GetConfusionMatrix().CostRatio(*cost);
    else
      return PureErrorRatio(); }

  double ErrorRatio() const {
    return CostRatio(); }

  double PureErrorRatio() const {
    return GetConfusionMatrix().ErrorRatio(); }

  double RejectionRatio() const {
    return GetConfusionMatrix().RejectionRatio(); }

  void ShowResultsHeadNoK(ostream&) const;
  virtual void ShowResultsHead(ostream&) const;
  virtual void ShowResults(int = -1, int = -1, ostream& = cout) const;

  int HasLabel(const char *lab) const {
    return GetConfusionMatrix().HasLabel(lab); }

  int FindLabel(const char *lab) const {
    return GetConfusionMatrix().FindIndex(lab); }

  // int HasPreClassify() const { return preclassify!=NULL; }
  // void PreClassify(void (*ptr)(const FloatVector&, FloatVector&)) {
  //    preclassify = ptr; }
  // void PreClassify(const FloatVector& src, FloatVector& dst) const {
  //   if (preclassify)
  //     preclassify(src, dst);
  //   else
  //     dst.Length(0);
  // }

  virtual void Lengthen(int len) {
    FloatVectorSet::Lengthen(len);

    for (int i=0; i<NCrossValidation(); i++)
      cross_val[i].Lengthen(len);

    if (TrainingSet() && !IsTrainingSetSelf())
      TrainingSet()->Lengthen(len);

    if (TestingSet() && !IsTestingSetSelf())
      TestingSet()->Lengthen(len);
  }    

  //   FloatVectorSet::VectorLength;
  virtual int VectorLength() const {
    if (!FloatVectorSet::VectorLength() && TrainingSet())
      return TrainingSet()->VectorLength();
    else
      return FloatVectorSet::VectorLength();
  }
  virtual void VectorLength(int i) { FloatVectorSet::VectorLength(i); }

  using FloatVectorSet::ShowError;
  using FloatVectorSet::Get; //g++
  using FloatVectorSet::ConditionalAbort; //g++
  using FloatVectorSet::Delete; //g++
  using FloatVectorSet::Relinquish; //g++
  using FloatVectorSet::ReadHead; //g++
  using FloatVectorSet::Is;
  using FloatVectorSet::CopyString;
  using FloatVectorSet::AppendString;
  using FloatVectorSet::ShowString;
  using FloatVectorSet::StringsMatch;
  using FloatVectorSet::IndexOK;
  using FloatVectorSet::Statistics;
  using FloatVectorSet::SetMatchMethod;
  using FloatVectorSet::GetMatchMethod;
  using FloatVectorSet::Read;
  using FloatVectorSet::Write;
  using FloatVectorSet::FileName;
  using FloatVectorSet::PrintNumber;
  using FloatVectorSet::SetPrintText;
  using FloatVectorSet::SetMetrics;
  using FloatVectorSet::Nearests;
  using FloatVectorSet::CountNumberOfLabels;
  using FloatVectorSet::NumberOfLabels;
  using FloatVectorSet::LabelByNumber;
  using FloatVectorSet::NumberOfUnitsWithLabelNumber;
  using FloatVectorSet::Nitems;
  using FloatVectorSet::NoDataRead;
  using FloatVectorSet::Description;
  using FloatVectorSet::WriteDescription;
  using FloatVectorSet::XMLDescription;
  using FloatVectorSet::Zipped;
  using FloatVectorSet::Sparse;
  using FloatVectorSet::WriteLabelsOnly;
  using FloatVectorSet::Metric;
  using FloatVectorSet::SetCovInvInMetric;
  using FloatVectorSet::RecordSeekIndex;
  using FloatVectorSet::ComponentDescriptions;
  using FloatVectorSet::GetComponentIndex;

  int RandomInit(int i);

  int EvenInit(int i, FloatVectorSet& s, int w = TRUE) {
    FileName(s.FileName());
    return FloatVectorSet::EvenInit(i, s, w); }
  int PropInit(int i, FloatVectorSet& s, int w = TRUE) {
    FileName(s.FileName());
    return FloatVectorSet::PropInit(i, s, w); }

  int EvenInit(FloatVectorSet& s, int w = TRUE) {
    return EvenInit(default_units, s, w); }

#define classifier_to_cross_vals( var , val ) \
  var = val; for (int i=0; i<NCrossValidation(); i++) cross_val[i]. var = val

  void DefaultUnits(int k) { classifier_to_cross_vals(default_units, k); }
  void DefaultK(int k)     { classifier_to_cross_vals(default_k,     k); }
  void RejectK(int k)      { classifier_to_cross_vals(reject_k,      k); }
  void AddK(int k)         { classifier_to_cross_vals(add_k,         k); }

  int DefaultUnits() const { return default_units; }
  int DefaultK() const { return default_k; }
  int RejectK()  const { return reject_k ; }
  int AddK()  const { return add_k ; }

  static int TestModeLength(TestMode, int);
  static int TestModeLength(TestMode m) {
    return TestModeLength(m, 1); }

  void SetPerformances(TestMode, FloatVectorSet&, FloatVectorSet&,
		       StatQuad&);

  void PerformanceTest(TestMode, FloatVectorSet&, FloatVectorSet&,
		       StatQuadVector&, int = FALSE, int = -1, int = 0);
  void PerformanceTest(TestMode, FloatVectorSet&, int,
		       StatQuadVector&, int = FALSE, int = -1, int = 0);

  void PerformanceTestHead(TestMode, StatQuadVector&, int, int&, int&);
  void PerformanceTestLoop(TestMode, FloatVectorSet&,
			   FloatVectorSet&, StatQuadVector&, int);

  int PerformanceSetBlockSize() const { return perf_block_size; }
  void PerformanceSetBlockSize(int i) { perf_block_size = i; }

  void ResultFileName(const char *str) { CopyString(result_file, str); }
  const char *ResultFileName(const char*, const char*, int = FALSE);
  const char *ResultFileName() { return ResultFileName(NULL, NULL); }

  void PlotAll() {
    Plot(TrainRejectError);
    Plot(TrainError);
    Plot(TrainTestError);
  }

  void Plot(TestMode m) { Plot(m, &results); }
  void Plot(TestMode, StatQuadVector*);
  void Plot();

  int AdaptRule() const { return adapt_rule; }
  void AdaptRule(int i) {
    if (i<1 || i>4)
      cerr << "Classifier::AdaptRule(" << i << ") not implemented." << endl;
    else
      adapt_rule = i;
  }
  void NoAdapt() { adapt_rule = 0; }

  int DefaultPerformanceIterations() const { return default_iter; }
  void DefaultPerformanceIterations(int i) { default_iter = i; }

  int DefaultPerformanceRepetitions() const { return default_rep; }
  void DefaultPerformanceRepetitions(int i) { default_rep = i; }

  int Silent() const { return silent; }
  void Silent(int i) { silent = i; }

  void SetPrintTexts(const char *txt = "") const {
    if (TrainingSet()) {
      TrainingSet()->PrintNumber(!silent);
      TrainingSet()->SetPrintText(txt);
    }
    if (TestingSet()) {
      TestingSet()->PrintNumber(!silent);
      TestingSet()->SetPrintText(txt);
    }
  }

  int CrossValidationSilent() const { return cross_validation_silent; }
  void CrossValidationSilent(int i) { cross_validation_silent = i; }

  int InsiderInit() const { return insider_init; }
  void InsiderInit(int i) { insider_init = i; }

#if SIMPLE_MISC_DECAY
  double Alpha() const { return alpha; }
  void DecayAlpha() { 
    if (DebugAdapt())
      cout << "Classifier::DecayAlpha() : " << alpha;
    alpha.Decay(); 
    if (DebugAdapt())
      cout << " -> " << alpha << endl;
  }
#endif // SIMPLE_MISC_DECAY

  int OldCrossValidation() const { return old_cross_validation; }
  void OldCrossValidation(int c) { old_cross_validation = c; }

  void CopyCvSetsFrom(const Classifier&);

  int CreateCrossValidation(int);
  int DeleteCrossValidation();

  StatVar CrossValidationTraining();
  StatVar CrossValidationTesting();
  
  void Intermediates(const StatQuadVector&, VectorSetOf<StatQuad>&, OptVar&);
  void Intermediates(const StatQuadVector&, OptVar* = NULL);

  StatQuad UpToThreeTests();
  const StatQuadVector& Results() const { return results; }
  StatQuadVector& Results() { return results; }

  StatQuad ResultByParameter(double) const;

  int ForceTestingSetOptimization() const {
    return TestingSet() && force_test_optimization; }

  void ForceTestingSetOptimization(int t) {
    force_test_optimization = t; }
  
  FloatVector TrainErrors() const { return StatQuad::TrainError(results); }
  FloatVector TestErrors()  const { return StatQuad::TestError(results);  }
  FloatVector CrossValidationErrors() const {
    return StatQuad::CrossValidationError(results); }

  FloatVector OptimizationError(const StatQuadVector& sqv) const {
    return
      ForceTestingSetOptimization() ? StatQuad::TestError(sqv) :
      DoCrossValidationTesting()    ? StatQuad::CrossValidationError(sqv) :
      DoTrainingSetTesting()        ? StatQuad::TrainError(sqv) :
      DoTestingSetTesting()         ? StatQuad::TestError(sqv) :
      FloatVector(); }

  FloatVector OptimizationError() const {
    return OptimizationError(results); }

  double OptimizationError(const StatQuad& sq) const {
    return
      ForceTestingSetOptimization() ? (double)sq.TestError() :
      DoCrossValidationTesting()    ? (double)sq.CrossValidationError() :
      DoTrainingSetTesting()        ? (double)sq.TrainError() :
      DoTestingSetTesting()         ? (double)sq.TestError() :
      0.0; }

  ConfusionMatrix& OptimizationConfusionMatrix() {
    // static ConfusionMatrix empty;
    return
      ForceTestingSetOptimization() ? cfmtrxs[1] :
      DoCrossValidationTesting()    ? cfmtrxs[2] :
      DoTrainingSetTesting()        ? cfmtrxs[0] :
      DoTestingSetTesting()         ? cfmtrxs[1] :
      *(ConfusionMatrix*)NULL; }

  virtual Classifier *CopyForCrossValidation(int) const;
  virtual int Create();

  int Create(FloatVectorSource& s) {
    TrainingSet(&s);
    return Create();
  }

  int NCrossValidation() const { return cross_val.Nitems(); }
  Classifier *CrossValidation(int i) const { return cross_val.Get(i); }

  FloatVectorSource *TrainingSet() const { return train_set; }
  FloatVectorSource *TestingSet()  const { return test_set;  }

  Classifier& TrainingSet(FloatVectorSource *s) {
    train_set = s; return *this; }
  Classifier& TestingSet(FloatVectorSource *s) {
    test_set  = s; return *this; }

  Classifier& TrainingSet(FloatVectorSource& s) {
    train_set = &s; return *this; }
  Classifier& TestingSet(FloatVectorSource& s) {
    test_set  = &s; return *this; }

  void TrainingSetSelf() { TrainingSet(this); }
  void TestingSetSelf()  { TestingSet(this); }
  int  IsTrainingSetSelf() { return TrainingSet()==this; }
  int  IsTestingSetSelf()  { return TestingSet()==this; }

  virtual void BeforeClassifyTrainingSet() {}
  virtual void AfterClassifyTrainingSet()  {}

  Classifier& Optimize(const OptVar& v) {
    OptVar::Append(optvar, new OptVar(v));
    return *this;
  }

  OptVar Optimize() { return Optimize(optvar); }
  OptVar Optimize(const OptVar*);
  virtual OptVar DoOptimize(const OptVar&);
  virtual OptVar SortOptVars(const OptVar&);

  OptVar DoOptimizeVectorLength(const OptVar&);
  OptVar DoOptimizeK(const OptVar&);

  virtual void VectorLengthChanged() {}
  int OptimalVectorLength(int = 1, int = MAXINT);

  int DoTestingSetTesting()  const { return TestingSet()!=NULL; }
  int DoTrainingSetTesting() const {
    return !no_train_testing && TrainingSet(); }
  int DoCrossValidationTesting() const {
    return TrainingSet() && NCrossValidation(); }
  int DoCrossValidationTraining() const {
    return DoCrossValidationTesting() && cross_val_train; }

  void DoCrossValidationTraining(int i) { cross_val_train = i; }
  void DoTrainingSetTesting(int i) { no_train_testing = !i; }

  void UseTrainingSetConfusionMatrix()     { current_cfmtrx = 0; }
  void UseTestingSetConfusionMatrix()      { current_cfmtrx = 1; }
  void UseCrossValidationConfusionMatrix() { current_cfmtrx = 2; }

  int IsTrainingSetConfusionMatrix() const     { return current_cfmtrx == 0; }
  int IsTestingSetConfusionMatrix() const      { return current_cfmtrx == 1; }
  int IsCrossValidationConfusionMatrix() const { return current_cfmtrx == 2; }

  int TrackErrors() const { return track_errors; }
  void TrackErrors(int t) { track_errors = t; }
  void ConditionallyTrackError(const FloatVector&, const char*);

  IntVector& Errors() { return errors; }

  int HasFeatureExtraction() const { return feat_ext!=NULL; }

#if SIMPLE_FEAT
  VectorTransform *FeatureExtraction() { return feat_ext; }
  void FeatureExtraction(VectorTransform *fe) { feat_ext = fe; }

  void AppendTransform(VectorTransform *fe) {
    VectorTransform::Append(feat_ext, fe);
  }

  void PrependTransform(VectorTransform *fe) {
    VectorTransform::Prepend(feat_ext, fe);
  }

  void RemoveLastTransform() {
    VectorTransform::RemoveLast(feat_ext);
  }

  void RemoveFirstTransform() {
    VectorTransform::RemoveFirst(feat_ext);
  }

  FloatVector PossiblyFeatureExtract(const FloatVector& v) const {
    if (HasFeatureExtraction())
      return Extract(v);
    else
      return v;
  }

  FloatVector Extract(const FloatVector& v) const {
    if (!feat_ext)
      abort();
    return feat_ext->Apply(v); }

  FloatVector RevertFloatVector(const FloatVector& v) const {
    if (!feat_ext)
      abort();
    return feat_ext->RevertFloatVector(v); }

  FloatVector ExtractAndRevert(const FloatVector& v) const {
    if (!feat_ext)
      abort();
    return feat_ext->ApplyAndRevert(v); }

  FloatVector ExtractClassifyAndRevert(const FloatVector& v) {
    if (!feat_ext)
      abort();
    return feat_ext->RevertFloatVector(Reproduce(feat_ext->Apply(v))); }

#if SIMPLE_IMAGE

  GreyImage ExtractClassifyAndRevert(const GreyImage& i) {
    if (!feat_ext)
      abort();
    return feat_ext->RevertGreyImage(Reproduce(feat_ext->Apply(i))); }

  GreyImage RevertGreyImage(const FloatVector& v) const {
    if (!feat_ext)
      abort();
    return feat_ext->RevertGreyImage(v); }

  GreyImage ExtractAndRevert(const GreyImage& i) const {
    if (!feat_ext)
      abort();
    return feat_ext->ApplyAndRevert(i); }
  
  FloatVector Extract(const GreyImage& i) const {
    if (!feat_ext)
      abort();
    return feat_ext->Apply(i); }

  virtual const char *ClassifyRawGreyImage(const GreyImage&, float* = NULL);

#endif // SIMPLE_IMAGE

#endif // SIMPLE_FEAT

  virtual FloatVector Reproduce(const FloatVector&) {
    abort(); return FloatVector(); }

  void ErrorLog(const char *name) {
    errlog.close(); errlog.clear(); errlog.open(name); }
  void ReportError(const FloatVector&, const char*);

  void ComponentEqualization(int);
  void ClassSpecificMetrics(int);

//   const MatrixOf<Type>& ClassMetrics(const char *lab) const {
//     return class_metrics[FindLabel(lab)]; }

  FloatVector FeatureWeighting(int, double, int = FALSE) const;

  ConfusionMatrix *CostMatrix() const { return cost; }
  Classifier& CostMatrix(ConfusionMatrix *m) {
    cost = m; return *this; }

  void WilsonEdit(int = 1);
  void Condensing();

  static IntVector DistanceClusters(const FloatMatrix&, int, double=1,
				    ListOf<IntVector>* = NULL);
  static IntVector DistClustSplit(const FloatMatrix&,const IntVector&);
  static IntVector DistClustDivide(const FloatMatrix&,const IntVector&);
  static IntVector DistClustCenters(const FloatMatrix&,const IntVector&,
				    double=1);
  
  bool HasAcceptLabels() const { return accept_labels.re_nsub!=MAXINT; }
  void AcceptLabels(const char*);
  bool IsLabelAcceptable(const char*) const;
  bool IsLabelAcceptable(const FloatVector& v) const {
    return IsLabelAcceptable(v.Label()); }

  IntVectorSet ListAcceptableLabels() const;

  static void kNNinitialize(FloatVectorSet&, int);
  static void kNNinsert(FloatVectorSet&, const FloatVector&, double);
  static const char *kNNresult(const FloatVectorSet&, int = 0);
  static double kNNlimit(const FloatVectorSet&);
  static void kNNend(FloatVectorSet&);

  // IntVector TrainingSetErrors() const { return train_errors; }
  // IntVector TestingSetErrors() const  { return test_errors; }
  const IntVector& TrainingSetErrors() const { return cfmtrxs[0].Impulses(); }
  const IntVector& TestingSetErrors()  const { return cfmtrxs[1].Impulses(); }

  void ErrorTailLength(int l ) { tail_length = l; }

  void RemoveNearest(const FloatVector& vec) {
    const FloatVector& nn = NearestTo(vec);
    Remove(&nn);
  }

  void Tic(const char *t) const { if (tictacptr) tictacptr->Tic(t); }
  void Tac(const char *t) const { if (tictacptr) tictacptr->Tac(t); }

  void TicTacStart() { if (tictacptr) tictacptr->Start(); }
  void TicTacSummary(bool cum = true, bool diff = true, ostream& os = cout) {
    if (tictacptr && tictacptr->Active()) tictacptr->Summary(cum, diff, os); }

  void TicTacSet(TicTac *t) { tictacptr = t; }

  void TicTacSet(const Classifier& c) { TicTacSet(c.tictacptr); }

  void NoTicTac() { TicTacSet(NULL); }

  void TicTacSetName(const char *n) { tictac.SetName(n); }

  void BatchTrainSettings(bool b, int p, int t) {
    batchtrain = b;
    batchtrainperiod = p;
    batchtrain_threads = t;
  }
  void BatchTrainSettings(const Classifier& c) {
    BatchTrainSettings(c.batchtrain, c.batchtrainperiod, c.batchtrain_threads);
  }

  bool BatchTrain() const { return batchtrain; }

  void ConfusionMatrixSorting(bool s) { sort_cfm = s; }

  const char *Normalization() const { return normalization; }

  void Normalization(const char *n) { CopyString(normalization, n); }

  void SkipInTraining(const set<string>& l) { skiplabelsset = l; }

  void SkipInTraining(const vector<string>& l) {
    // skiplabels = l;
    skiplabelsset.clear();
    for (vector<string>::const_iterator i=l.begin(); i!=l.end(); i++)
      skiplabelsset.insert(*i);
  }

  static void DebugAdapt(bool d) { debug_adapt = d; }
  static bool DebugAdapt() { return debug_adapt; }

  static void DebugSkips(bool d) { debug_skips = d; }
  static bool DebugSkips() { return debug_skips; }

  void DivideByNumbers(bool d) { divide_by_numbers = d; }
  bool DivideByNumbers() const { return divide_by_numbers; }

  void BmuDivDepth(int i) { bmu_div_depth = i; }
  int BmuDivDepth() const { return bmu_div_depth; }
  
  void RemoveExtraVariables() {
    codefileextravars.clear();
  }

  void AddExtraVariable(const string& key, const string& val) {
    codefileextravars[key] = val;
  }

  void UseTimeStamp(bool u) { use_timestamp = u; }

  bool UseTimeStamp() const { return use_timestamp; }

  string PossiblyTimeStamp() const {return use_timestamp ? TimeStampP() : "";}

  const map<int,string>& ClassificationsByNumber() const {
    return classifications;
  }
  
  int TrainDistLength() const { return train_dist.Length(); }

  void TrainDistSetLength(int i) {
    train_dist.Lengthen(i);
    train_dist.Set(-1.0);
  }

  void TrainDist(int i, double v) {
    if (train_dist.Length()<=i) {
      FloatVector tmp(i+1-train_dist.Length());
      tmp.Set(-1.0);
      train_dist.Append(tmp);
    }
    train_dist[i]=v;
  }
  
  double TrainDist(int i) const {
    return train_dist[i];
  }

protected:
  ConfusionMatrix cfmtrxs[3];
  int current_cfmtrx;
  const char **cfmtrx_text;

  // void (*preclassify)(const FloatVector&, FloatVector&);

  int default_units;
  int default_k;
  int reject_k;
  int add_k;
  int default_iter, default_rep;

  ListOf<GnuPlot> plot;
  StatQuadVector results;
  char *result_file;

  int perf_block_size;
  int insider_init;
  int do_eveninit;

  const char *identity;
  int adapt_rule;
  int silent, cross_validation_silent;
  int force_test_optimization, no_train_testing, cross_val_train;

  int bmu_div_depth;

  int old_cross_validation;

#if SIMPLE_MISC_DECAY
  DecayVar alpha;
#endif // SIMPLE_MISC_DECAY

  ListOf<Classifier> cross_val;
  FloatVectorSource *train_set, *test_set, *current_source;

  int track_errors;
  IntVector errors;

#if SIMPLE_FEAT
  VectorTransform *feat_ext;
#else 
  void *feat_ext;
#endif // SIMPLE_FEAT
  FloatVector *raw_vector;
  int do_comp_equal, do_class_metrics;

  ofstream errlog;

  OptVar *optvar;

  ConfusionMatrix *cost;

  regex_t accept_labels;
  char *accept_labels_pattern;

  int tail_length;

  TicTac tictac, *tictacptr;

  bool batchtrain;

  int batchtrainperiod;

  int batchtrain_threads;

  IntVectorSet train_bmu_set;
  FloatVector train_dist;
  ListOf<IntVector> train_backref;

  const char *pre_accuracy_text;
  bool sort_cfm;

  const char *normalization;

  static bool debug_adapt;
  static bool debug_skips;
  set<string> skiplabelsset;

  bool divide_by_numbers;

  bool use_timestamp;

  map<string,string> codefileextravars;

  map<int,string> classifications;
};

} // namespace simple
#endif // _CLASSIFIER_H_

