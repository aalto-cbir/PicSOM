// 
// Copyright 1994-2007 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2007 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: VectorSet.h,v 1.19 2014/07/01 13:40:56 jorma Exp $

// -*- C++ -*-

#ifndef _VECTORSET_H_
#define _VECTORSET_H_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <VectorSource.h>

#ifdef EXPLICIT_INCLUDE_CXX
#include <List.h>
#endif // EXPLICIT_INCLUDE_CXX

#include <vector>

namespace simple {

enum Matchmethod {  // couldn't this be nested ??
  euclidean,
  dotproduct,
  absolutedotproduct
};

template <class Type> class MatrixOf;

//- VectorSet
template <class Type> class VectorSetOf : public ListOf<VectorOf<Type> >,
public VectorSourceOf<Type> {
public:
  using VectorSourceOf<Type>::ShowError;

  using VectorSourceOf<Type>::vectorlength;
  using VectorSourceOf<Type>::FileName;
  //g++ VectorSourceOf<Type>::next;
  using VectorSourceOf<Type>::nitems;
  using VectorSourceOf<Type>::Nitems;
  using VectorSourceOf<Type>::CopyString;
  using VectorSourceOf<Type>::description;
  using VectorSourceOf<Type>::PrintNumber;
  using VectorSourceOf<Type>::SetPrintText;
  using VectorSourceOf<Type>::ReadHead;
  using VectorSourceOf<Type>::ReadOne;
  using VectorSourceOf<Type>::PrintTextAndNumber;
  using VectorSourceOf<Type>::First;
  using VectorSourceOf<Type>::Next;
  using VectorSourceOf<Type>::AutoCov;
  using VectorSourceOf<Type>::Binary;
  using VectorSourceOf<Type>::Zipped;
  using VectorSourceOf<Type>::Sum; //alpha
  using VectorSourceOf<Type>::NextIndex; //alpha
  using VectorSourceOf<Type>::NextIndexWithPostInc; //alpha
  using VectorSourceOf<Type>::AddToNextIndex; //alpha
  using VectorSourceOf<Type>::InitializeMinMaxNumber; //alpha
  using VectorSourceOf<Type>::CheckMinMaxNumber; //alpha
  // VectorSourceOf<Type>::ShowError; //alpha
  using VectorSourceOf<Type>::StringsMatch; //alpha

  using VectorSourceOf<Type>::FindOrAddFeatureComponents;
  using VectorSourceOf<Type>::SetFeatureComponentMeanStDev;

  using ListOf<VectorOf<Type> >::GetEvenNew;
  using ListOf<VectorOf<Type> >::Get;
  using ListOf<VectorOf<Type> >::FastGet;
  using ListOf<VectorOf<Type> >::ConditionalAbort;
  using ListOf<VectorOf<Type> >::IndexOK;
  //g++ ListOf<VectorOf<Type> >::ShowError;
  using ListOf<VectorOf<Type> >::Remove;
  using ListOf<VectorOf<Type> >::Delete;
  using ListOf<VectorOf<Type> >::Relinquish;
  // ListOf<VectorOf<Type> >::;

public: 
  VectorSetOf(int l = 0) { Initialize(); vectorlength = l; }

  VectorSetOf(int l, int n) {
    Initialize(); vectorlength = l; GetEvenNew(n-1); }

  VectorSetOf(const VectorSetOf& s) : 
    SourceOf<VectorOf<Type> >(s),   // this was (probably?) added 28.6.2004
    ListOf<VectorOf<Type> >(/*s*/), // this was (s) until 1.7.2004
    VectorSourceOf<Type>(s) {
    Initialize(); AsSuchCopyFrom(s); }

    VectorSetOf(const char *name, int i = -1, bool permissive = false) {
      Initialize(); 
      FileName(name); 
      if (name) 
	Read(name, i, permissive);
    }

    VectorSetOf(const VectorOf<Type>& v, int l = 0) {
    Initialize(); vectorlength = l; Split(v); }

  virtual ~VectorSetOf() { delete scalar_quantized; }

  VectorSetOf& operator=(const VectorSetOf& s) { return AsSuchCopyFrom(s); }
  VectorSetOf& CopyFrom(const VectorSetOf&, int);
  VectorSetOf& AsSuchCopyFrom(const VectorSetOf& l) { return CopyFrom(l,  0); }
  VectorSetOf& CopiedCopyFrom(const VectorSetOf& l) { return CopyFrom(l,  1); }
  VectorSetOf& SharedCopyFrom(const VectorSetOf& l) { return CopyFrom(l, -1); }

  VectorSetOf SharedCopy() const;

  void AppendSharedRange(const VectorSetOf& l, int beg, int end) {
    for (int i=beg; i<=end; i++)
      Append(l.Get(i), false); }
    
  void AppendShared(const VectorSetOf& l) {
    AppendSharedRange(l, 0, l.Nitems()-1); }
    
  virtual void Dump(Simple::DumpMode = Simple::DumpDefault,
		    ostream& = cout) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "VectorSet", NULL }; return n; }

  virtual VectorOf<Type> *GetNext() { return Get(NextIndexWithPostInc()); }
  virtual void Skip(int n) { AddToNextIndex(n); }

  virtual VectorSourceOf<Type> *MakeSharedCopy() const;

  int Read(istream*, gzFile, bool = false);
  int Read(const char *n, int = -1, bool = false);
  int Read(const string& s, int l = -1) { return Read(s.c_str(), l); }

  VectorSetOf& ReadAll(istream*, gzFile, int = 0);

  int IsIncluded(const VectorOf<Type>&) const;
  int IsClassInsider(const VectorOf<Type>&, int);

  virtual int  VectorLength() const {
    return Nitems() ? Get(0)->Length() : vectorlength; }

  virtual void VectorLength(int i) {
    VectorSourceOf<Type>::VectorLength(i); }

  virtual void Lengthen(int l) {
    vectorlength = l;
    for (int i=0; i<Nitems(); i++)
      Get(i)->Lengthen(l);
  }

  virtual void Append(const ListOf<VectorOf<Type> > &, bool, bool) {
    SimpleAbort(); }

  virtual void Append(VectorOf<Type> *v, int d = TRUE) {
    if (v && !v->IsNumberSet()) {
      int n = VectorSetOf<Type>::Nitems();
      v->Number(n); }
    ListOf<VectorOf<Type> >::Append(v, d);
    CheckMinMaxNumber(v->Number());
  }

  VectorOf<Type> *AppendCopy(const VectorOf<Type>& v) {
    VectorOf<Type> *c = new VectorOf<Type>(v);
    Append(c);
    return c;
  }

  void AppendCopy(const VectorSetOf<Type>& s) {
    for (int i=0; i<s.Nitems(); i++)
      AppendCopy(s[i]); }

  IntVector NearestIndices(VectorSourceOf<Type>&) const;

  const VectorOf<Type>& NearestTo(const VectorOf<Type>&) const;

  const VectorOf<Type>& NearestToMean() { return NearestTo(Mean()); }

  void Nearests(const VectorOf<Type>&, VectorOf<Type>*&,VectorOf<Type>*&)const;

  double MeanSquaredDistance(const VectorOf<Type>&) const;

  double MeanSquaredDistance(const VectorSetOf<Type>&) const;

  int NearestNeighbors(const VectorOf<Type>&, IntVector&, FloatVector&,
		       const typename VectorOf<Type>::DistanceType* = NULL) const;

  const char *NearestNeighborsClass(const VectorOf<Type>&, int, int = 0,
				    float* = NULL, int* = NULL);

  const char *NearestNeighborsClassIdx(const IntVector&, int = 0, int = 0);

  int ClassBorderIndex(const VectorOf<Type>&) const;

  IntVector ClassBorderIndices(const VectorSetOf<Type>&) const;

  const char *Description() const { return description;         }

  VectorSetOf& Description(const char *d) {
    CopyString(description, d); return *this; }        

  Matchmethod GetMatchMethod() const      { return matchmethod; }
  void SetMatchMethod(Matchmethod m)      { matchmethod = m; }
  void SetMatchMethodEuclidean()          { matchmethod = euclidean; }
  void SetMatchMethodDotProduct()         { matchmethod = dotproduct; }
  void SetMatchMethodAbsoluteDotProduct() { matchmethod = absolutedotproduct; }

  double Distance(const VectorSetOf& set) const {
    return sqrt(DistanceSquared(set)); }
  double DistanceSquared(const VectorSetOf&) const;

  // double DistanceSquared() const;
  void CalculateSelfDots();
//   int UseSelfDots() const { return use_self_dots; }
//   VectorSetOf<Type>& UseSelfDots(int u) { use_self_dots = u; return *this; }

  int DistanceComparisonInterval() const { return dist_comp_ival; }
  VectorSetOf<Type>& DistanceComparisonInterval(int u) {
    dist_comp_ival = u; return *this; }

  void SetLabels(const char *l) {
    for (int i=0; i<Nitems(); i++)
      Get(i)->Label(l); }

  VectorOf<Type> *GetByLabel(const char*, int = 0) const;
  VectorOf<Type> *GetByLabel(const string& s, int i = 0) const {
    return GetByLabel(s.c_str(), i);
  }

  const VectorOf<Type> *GetByNumber(int n) const {
    static int previ=0, prevn=0;
    
    // hack for loops, when next item is probably close by :)
    int testi = previ+n-prevn;
    VectorOf<Type>* pt = Get(testi);
    if (pt && pt->Number()==n) {
      previ=testi; prevn=n;
      return pt;
    }
    
    // if not, then do it the slow way...
    for (int i=0; i<nitems; i++)
      if (FastGet(i)->Number()==n) {
	previ=i; prevn=n;
	return FastGet(i);
      }
    return NULL;
  }

  const VectorOf<Type> *FindByNumber(int n) const { return GetByNumber(n); }

  VectorSetOf<Type> GetAllByLabel(const char*, IntVector* = NULL) const;
  static void AppendLabelList(VectorOf<Type>*, ListOf< VectorSetOf<Type> >&,
			      bool = false);
  ListOf<VectorSetOf<Type> > SeparateLabels(bool = false);
  ListOf<VectorSetOf<Type> > SeparateByIndex(const IntVector&, bool = false);

  VectorOf<Type> Component(int) const;

  bool Components(const IntVector& c) {
    if (Nitems()==0) {
      vectorlength = c.Length();
      return true;
    }

    for (int i=0; i<c.Length(); i++)
      if (!Get(0)->IndexOK(c[i]))
	return false;
      
    for (int i=0; i<Nitems(); i++)
      Get(i)->Components(c);

    return true;
  }

  void ArrangeComponents(const IntVector& idx) {
    for (int i=0; i<Nitems(); i++)
      Get(i)->Arrange(idx);
  }

  bool Normalize(bool = false, xmlDocPtr = NULL);

  bool ComponentAddAndMultiply(int i, double a, double m) {
    if (!IndexOK(i))
      return false;
    for (int j=0; j<Nitems(); j++) {
      Type& v = (*FastGet(j))[i];
      v = Type((double(v)+a)*m);
    }
    return true; }

  void RemoveComponent(int j) {
    for (int i=0; i<Nitems(); i++)
      Get(i)->Remove(j);
  }

  bool RemoveComponentsSumBelow(Type v, IntVector& idx) {
    idx.Length(0);
    VectorOf<Type> sum = Sum();
    for (int i=sum.Length()-1; i>=0; i--)
      if (sum[i]<v) {
	idx.Prepend(i);
	RemoveComponent(i);
      }
    return true;
  }

  bool RemoveComponentsSumAbove(Type v, IntVector& idx) {
    idx.Length(0);
    VectorOf<Type> sum = Sum();
    for (int i=sum.Length()-1; i>=0; i--)
      if (sum[i]>v) {
	idx.Prepend(i);
	RemoveComponent(i);
      }
    return true;
  }

  FloatVector NormSquared(const VectorOf<Type>* = NULL) const;
  FloatVector Norm(const VectorOf<Type>* = NULL) const;

  void RejectLog(const char *name, int l = 2) {
    rejectlog.close(); rejectlog.clear(); rejectlog.open(name);
    rejectlog << l << endl;
  }
  // void ReportReject(const VectorOf<Type>&, const char*);

  int EvenInit(int, VectorSetOf<Type>&, int = TRUE);
  int PropInit(int, VectorSetOf<Type>&, int = TRUE);

  void Statistics(const char *name) {
    statlog.close(); statlog.clear(); statlog.open(name);
  }
  void Statistics(const VectorOf<Type>&, const IntVector&, int,
		  const char* = NULL);

  void CountNumberOfLabels();
  int NumberOfLabels() const { return labelnumbers.Nitems(); }
  const char *LabelByNumber(int i) const {
    return i<labelnumbers.Nitems() && i>=0 ? labelnumbers[i].Label() : NULL; }
  int NumberOfUnitsWithLabelNumber(int i) const {
    return i<labelnumbers.Nitems() && i>=0 ? labelnumbers[i][0] : 0; }
  int LabelListIndex(const char *lab) const {
    for (int i=0; i<NumberOfLabels(); i++)
      if (StringsMatch(LabelByNumber(i), lab))
	return i;
    return -1;
  }

  VectorSetOf LabelSubset(const char*) const;
  VectorSetOf InsiderSubset(int, VectorSetOf<Type>* = NULL);
  VectorSetOf SubsetByIndex(const VectorOf<int>&) const;
  VectorSetOf SubsetByIndex(const std::vector<size_t>&) const;
  VectorSetOf SubsetByValueEquals(const VectorOf<int>&, int) const;

  void ResetMinMaxNumber();
  void DumpNumbers(ostream& = cout) const;
  void SetNumbers() const;

  Type Minimum() const;
  Type Maximum() const;

  VectorOf<Type> MinimumComponents() const;
  VectorOf<Type> MaximumComponents() const;

  double MinimumDistance(const VectorOf<Type>&, int = TRUE, int = TRUE, int* = NULL) const;
  double MaximumDistance(const VectorOf<Type>&, int = TRUE, int = TRUE, int* = NULL) const;

  double MinimumExternalDistance(const VectorOf<Type>& v, int *i =NULL) const {
    return MinimumDistance(v, FALSE, TRUE, i); }
  double MinimumInternalDistance(const VectorOf<Type>& v, int *i =NULL) const {
    return MinimumDistance(v, TRUE, FALSE, i); }
  double MaximumInternalDistance(const VectorOf<Type>& v, int *i =NULL) const {
    return MaximumDistance(v, TRUE, FALSE, i); }

  void ClassNeighborHistograms(const char*);

  VectorOf<Type> Concat(int = 0, int = MAXINT) const;
  void Split(const VectorOf<Type>&);

  double LindeBuzoGray(VectorSourceOf<Type>&, float* = NULL);

  VectorOf<Type> LinearCombination(const VectorOf<Type>&,
				   VectorOf<Type>* = NULL) const;
  VectorOf<Type> LinearCombinationByIteration(const VectorOf<Type>&,
					      VectorOf<Type>* = NULL) const;

  const MatrixOf<Type> *FindMetrics(const char* = NULL) const;
//   void SetMetrics(const  ListOf<MatrixOf<Type> >& m) {
//     new_metrics = m; }
  void SetMetrics(const  ListOf<MatrixOf<Type> >&) {
    ShowError("VectorSetOf<Type>::SetMetrics is deimplemented"); }

  FloatVector SimilarityValues(const VectorOf<Type>&) const;

  MatrixOf<float> MutualDistances(int = 2) const;

  VectorSetOf<Type> KMeans(int, int = 100, IntVector* = NULL) const;

  VectorSetOf<Type> OrderComponentValues() const;

  bool HasScalarQuantization() const {
    return scalar_quantized && scalar_quantized->Nitems()==Nitems(); }

  bool HasScalarQuantizationBackRefs() const {
    return scalar_quantization_back_ref.Nitems(); }

  void ScalarQuantize();

  IntVector ScalarQuantize(const FloatVector&) const;

  static int ScalarQuantize(double, const FloatVector&);

  void ScalarQuantize(const ListOf<FloatVector>& l) {
    ScalarQuantizationValues(l);
    ScalarQuantize(); }

  void ScalarQuantizationValues(const ListOf<FloatVector>& l) {
    if (l.Nitems()!=VectorLength())
      ShowError("Classifier::ScalarQuantizationValues(): "
		"vector length mismatch");

    scalar_quantization_values = l; }

  IntVector ScalarQuantizationBins() const {
    IntVector b(VectorLength());
    for (int i=0; i<VectorLength(); i++) b[i] = ScalarQuantizationBins(i);
    return b; }

  int ScalarQuantizationBins(int i) const {
    return scalar_quantization_values[i].Length()+1; }

  const IntVector& ScalarQuantized(int i) const {
    return (*scalar_quantized)[i]; }

  void MakeScalarQuantBackRefs();

  const IntVector& ScalarQuantBackRefs(int i, int j) const {
    return scalar_quantization_back_ref[i][j]; }

  int ScalarQuantizationHits(int i, int j) const {
    return scalar_quantization_back_ref[i][j].Length(); }

  IntVector ScalarQuantizationPairDistanceHistogram() const;

  IntVector ScalarQuantizationHitCountHistogram() const;

  IntVector ScalarQuantizationEquals(int i, int d = 0) const {
    if (!scalar_quantized->IndexOK(i)) {
      ShowError("ScalarQuantizationEquals() index not OK");
      return IntVector();
    }
    return ScalarQuantizationEquals(ScalarQuantized(i), d); }

  IntVector ScalarQuantizationEquals(const IntVector&, int = 0) const;

  int ScalarQuantizationDistance(int i, int j) const {
    return ScalarQuantizationDistance(ScalarQuantized(i), ScalarQuantized(j));}

  static int ScalarQuantizationDistance(const IntVector& i,
					const IntVector& j) {
    return (int) (i-j).Norm(1); }

  ListOf<FloatVector> CreateEqualizedScalarQuantization(int);

  ListOf<FloatVector> CreateEqualizedScalarQuantization(int,
							const
							VectorSetOf<float>&);

  FloatVector CreateEqualizedScalarQuantization(int, int);

  FloatVector EqualizedScalarQuantizationPick(int, const FloatVector&);

  VectorOf<Type> Mean(const IntVector* = NULL) const;

  void Metric(typename VectorOf<Type>::DistanceType *m) { metric = m; } 

  void DeleteMetric() { delete metric; metric = NULL; }

  typename VectorOf<Type>::DistanceType *Metric() const { return metric;}

  bool SetCovInvInMetric() /*const*/;
  
  bool RemoveZeroVectors();

  bool MapNegativeZeroPositive(Type, Type, Type);

  int PolarizeComponents();

protected:
  void Initialize();

  ListOf<IntVector> labelnumbers;
  // ListOf<MatrixOf<Type> > new_metrics;
  Matchmethod matchmethod;

  ofstream statlog;
  ofstream rejectlog;

  // int use_self_dots;
  int dist_comp_ival;

  VectorSetOf<int>        *scalar_quantized;

  ListOf<FloatVector> scalar_quantization_values;

  ListOf<ListOf<IntVector> > scalar_quantization_back_ref;

  typename VectorOf<Type>::DistanceType *metric;
};

typedef VectorSetOf<float> FloatVectorSet;
typedef VectorSetOf<int> IntVectorSet;

template <>
FloatVectorSet FloatVectorSet::OrderComponentValues() const;

template <>
void FloatVectorSet::MakeScalarQuantBackRefs();

template <>
ListOf<FloatVector> FloatVectorSet::CreateEqualizedScalarQuantization(int);

template <>
ListOf<FloatVector> FloatVectorSet::CreateEqualizedScalarQuantization(int,
						      const FloatVectorSet&);

#ifdef EXPLICIT_INCLUDE_CXX
#include <VectorSet.C>
#endif // EXPLICIT_INCLUDE_CXX

} // namespace simple
#endif // _VECTORSET_H_

