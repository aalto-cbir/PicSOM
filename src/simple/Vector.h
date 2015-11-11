// -*- C++ -*-  $Id: Vector.h,v 1.20 2013/08/19 09:15:56 jorma Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND

#ifndef _VECTOR_H_
#define _VECTOR_H_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#ifdef __GNUG__
//#define NGinline
#define NGinline inline
#else // __GNUG__
#define NGinline inline
#endif // __GNUG__

#include <SimpleComplex.h>

namespace simple {

template <class Type> class MatrixOf;

//- Vector
template <class Type> class VectorOf : public Simple {
  //. Vector object is the mother of all other mathematical objects.

public:
  class Distance;

  explicit VectorOf(int = 0, const Type* = NULL,
	   const char* = NULL, bool = false);

  explicit VectorOf(const char *n) {
    Initialize();
    Read(n);
    // n_created++;
  }

  VectorOf(const VectorOf& v) : Simple(v) {
    Initialize();
    *this = v;
    // n_created++;
  }

  virtual ~VectorOf();

  VectorOf  operator+(const VectorOf&) const;
  VectorOf  operator-(const VectorOf&) const;
  VectorOf  operator*(const VectorOf&) const;
  VectorOf  operator/(const VectorOf&) const;

  VectorOf& operator=(const VectorOf&);
  VectorOf& operator+=(const VectorOf&);
  VectorOf& operator-=(const VectorOf&);
  VectorOf& operator*=(const VectorOf&);
  VectorOf& operator/=(const VectorOf&);

  VectorOf  operator*(double) const;
  VectorOf  operator/(double) const;

  VectorOf& operator*=(double m) { return Multiply(m);     }
  VectorOf& operator/=(double m) { return Multiply(1.0/m); }

  VectorOf& operator=(const Type*);

  VectorOf  operator-() const { return VectorOf<Type>(*this).Multiply(-1); }

  VectorOf  operator~() const;

  VectorOf  operator|(const VectorOf& v) const;

  VectorOf  operator&(const VectorOf& v) const;

  VectorOf  operator^(const VectorOf& v) const;

  VectorOf  SetMinus(const VectorOf& v) const;

  Type& operator[](int i) const {
    if (!IndexOK(i)) {
      cerr << "Index " << i << " in Vector::operator[] out of bounds" << endl;
      Dump(length<=dump_length_limit ? DumpRecursiveLong : DumpRecursiveShort,
	   cerr);
      SimpleAbort();
    }
    return vector[i];
  }

  VectorOf TernaryOR(const VectorOf& v) const;
  VectorOf TernaryXOR(const VectorOf& v) const;
  VectorOf TernaryAND(const VectorOf& v) const;
  VectorOf TernarySetMinus(const VectorOf& v) const;
  VectorOf TernaryMask(const VectorOf& v) const;
  VectorOf TernaryNOT() const;
  VectorOf TernaryExclusive() const;
  VectorOf TernaryDefined() const;

  bool TernaryCounts(int& n, int& z, int& p) const {
    n = z = p = 0;
    for (int i=0; i<length; i++)
      switch ((int)vector[i]) {
      case -1: n++; break;
      case  0: z++; break;
      case  1: p++; break;
      default: return ShowError("Vector::TernaryCounts() Non -1/0/+1 value");
      }
    return true;
  }


  //
  static void TernaryTruthTables(ostream& = cout);

  //
  static void TernaryTruthTable(VectorOf<Type> (VectorOf<Type>::*f)() const,
				ostream& = cout);

  //
  static void TernaryTruthTable(VectorOf<Type>
				(VectorOf<Type>::*f)(const VectorOf<Type>&)
				const, ostream& = cout);


  Type& Value(int i) const { return (*this)[i]; }
  Type& Last() { return (*this)[length-1]; }
  const Type& Last() const { return (*this)[length-1]; }

  operator Type*() const { return vector; }

  int operator==(const VectorOf&) const;
  int operator!=(const VectorOf& v) const { return !operator==(v); }

  virtual void Dump(DumpMode = DumpDefault, ostream& = cout) const;
  void DisplayOne(Type, ostream& = cout) const;
  void DisplayAll(ostream& os = cout) const {
    for (int i=0; i<Length(); i++) DisplayOne(Get(i), os); }

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "Vector", NULL }; return n; }

  bool IndexOK(int i) const { return i>=0 && i<length && vector; }
  bool IndexOK(const VectorOf<int>& idx) const { 
    for (int i=0; i<length; i++)
      if (!IndexOK(idx[i]))
	return false;
    return true;
  }

  bool ContentEqual(const VectorOf&) const;
  bool AreUnique() const;

  //- Share
  VectorOf& Share(Type *v) {
    DeleteVectorIfOwn();
    vector = v;
    own    = FALSE;
    return *this;
  }
  VectorOf& Share(const VectorOf& v) {
    Share(v.vector); length = v.length; return *this;}

  inline const char *TypeName() const;
  NGinline bool IsFloatVector() const;
  NGinline bool IsDoubleVector() const;
  NGinline bool IsIntVector() const;

  //- Zero
  inline VectorOf& Zero();
  int IsZero() const;
  //. Set all components of vector to zero.

  int NonZeros() const;

  VectorOf<int> NonZeroIndices() const;

  inline void CopyValues(const Type*, int);

  void CopyValue(const VectorOf&);

  VectorOf& One();

  /// vector's component 'idx' to be 'value'.
  const VectorOf& Set(int idx, Type v) const {
    if (IndexOK(idx)) FastSet(idx, v); return *this; }

  const VectorOf& FastSet(int idx, Type v) const {
    vector[idx] = v; return *this; }

  const VectorOf& Set(Type v) const {
    for (int i=0; i<length; i++) vector[i] = v; return *this; }
  //. Set vector's component 'idx' to be 'value'.

  const VectorOf& SetRange(int beg, int end, Type v) const {
    for (int i=beg; i<=end; i++) Set(i, v); return *this; }

  const VectorOf& Change(Type ov, Type nv) const {
    for (int i=0; i<length; i++)
      if (vector[i] == ov)
	vector[i] = nv;
    return *this; }

  //- Get
  Type Get(int idx) const {
    return IndexOK(idx) ? vector[idx] : (Type)0; }
  //. Get the value of vector's component 'idx'.

  Type FastGet(int idx) const { return vector[idx]; }

  VectorOf& SetIndices(int l = 0) {
    if (l) Length(l);
    for (int i=0; i<length; i++) vector[i] = i;
    return *this;
  }

  VectorOf& SetRamp(int l = 0) {
    SetIndices(l);
    return Multiply(1.0/(length-1));
  }

  VectorOf Polynomial(const VectorOf<Type>& pol) const {
    VectorOf<Type> res(Length());
    if (pol.Length())
      for (int i=0; i<Length(); i++) {
	int j = pol.Length()-1;
	Type sum = pol[j];
	while (j--)
	  sum = vector[i]*sum+pol[j];
	res[i] = sum;
      }
    return res;
  }

  ///
  Type AbsDiffSum(const VectorOf& v) const {
    float sum = 0.0;

    if (CheckDisparity(v))
      for (int i=0; i<length; i++)
	sum += fabs(float(vector[i]-v.vector[i]));

    return (Type)sum;
  }

  //- DotProduct
  Type DotProduct(const VectorOf& other) const;
  //. Calculate vector's inner alias dot product with vector 'other'.

  static double SelfProduct(double x) { return x*x; }
  static double SelfProduct(int x) { return x*x; }
  static double SelfProduct(const SimpleComplex& x) { return x.RhoSquared(); }
//   static double SelfProduct(const StatVar&) {
//     cerr << "Vector::SelfProduct(StatVar)" << endl;
//     return 0; }
//   static double SelfProduct(const StatQuad&) {
//     cerr << "Vector::SelfProduct(StatQuad)" << endl;
//     return 0; }

  double DotProductDistanceSquared(const VectorOf&) const;

  double CosineDistanceSquared(const VectorOf&) const;

  double DistanceSquaredOld(const VectorOf&, double = MAXFLOAT,
			 const float *metr = NULL) const;

  double DistanceOld(const VectorOf& other, double limit = MAXFLOAT,
		  const float *metr = NULL ) const {
    return sqrt(DistanceSquaredOld(other, limit, metr)); }

  class DistanceType {
  public:
    enum DistType {
      undef, euclidean, weighted_euclidean, maximum, ell, mahalanobis, 
      mahalanobis_bias, dotdistance, kullback, kullback_inv, kullback_sym, 
      cosine, chisquare, histogram_intersection 
    };

    DistanceType(DistType t = undef, double l = 0) :
      type(t), ellval(l), covinv(NULL), w(NULL) {}

    DistanceType(DistType t, const MatrixOf<Type> *m) :
      type(t), ellval(0), covinv(m), w(NULL) {}

    ~DistanceType(); // { delete covinv; }
    
    DistType GetType() const { return type; }

    double GetEll() const { return ellval; }

    void SetMatrix(const MatrixOf<Type> *m); // { delete covinv; covinv = m; }

    const MatrixOf<Type> *GetMatrix() const { return covinv; }

    void SetWeights(const VectorOf<float> *w);

    const VectorOf<float> *GetWeights() const { return w; }

    std::string String(bool lng = true) const;

  protected:
    DistType type;
    float ellval;
    const MatrixOf<Type> *covinv;
    const VectorOf<float> *w;
  };

  static DistanceType *MakeDistance(const std::string& s);

  double DistanceXX(const VectorOf& v,
                    const typename VectorOf::DistanceType *d = NULL) const {
    return sqrt(DistanceSquaredXX(v, d)); }

  double DistanceSquaredXX(const VectorOf& v, double r, int i = 0,
			   const typename VectorOf::DistanceType *d = NULL) const {
    return DistanceSquaredXX(v, d, r, i); }

  double DistanceSquaredXX(const VectorOf&,
			   const typename VectorOf::DistanceType* = NULL,
			   double = MAXFLOAT, int = 0) const;

//   double DistanceXX(const VectorOf& other, double limit = MAXFLOAT,
// 		    const VectorOf::DistanceType *metr = NULL ) const {
//     return sqrt(DistanceSquaredXX(other, limit, metr)); }

  double DistanceSquaredEuclidean(const VectorOf&,
				  double = MAXFLOAT, int = 0,
				  const VectorOf<float>* = NULL) const;

  double DistanceSquaredEll(const VectorOf&, double,
			    double = MAXFLOAT, int = 0) const;

  double DistanceSquaredMax(const VectorOf&, double = MAXFLOAT, int = 0) const;

  double DistanceSquaredMahalanobis(const VectorOf&, const MatrixOf<Type>*,
				    double = MAXFLOAT, int = 0) const;

  double DistanceSquaredKullback(const VectorOf&, int = 0,
				 double = MAXFLOAT, int = 0) const;

  double DistanceChiSquare(const VectorOf&) const;

  double DistanceHistogramIntersection(const VectorOf&) const;

  static double DistanceKullbackTerm(double p, double q, int mode = 0) {
    switch (mode) {
    case 0: return q>0 ? q*log(q/p)       : 0;
    case 1: return DistanceKullbackTerm(q,p);
    case 2: return (DistanceKullbackTerm(p,q)+DistanceKullbackTerm(q,p))/2;
    default: SimpleAbort(); return 0;
    } }

  static double DistanceSquared(register double x, register double y) {
    register double t = x-y; return t*t; }

  static double DistanceSquaredEll(double x, double y, double l) {
    return pow(fabs(x-y), l); }

//   static double DistanceSquared(const StatVar&, const StatVar&) {
//     cerr << "Vector::DistanceSquared(StatVar, StatVar)" << endl;
//     return 0;
//   }
//   static double DistanceSquared(const StatQuad&, const StatQuad&) {
//     cerr << "Vector::DistanceSquared(StatQuad, StatQuad)" << endl;
//     return 0;
//   }

  //- Length
  int Length() const { return length; }

  VectorOf& Length(int l) {
    if (l!=length || !own) {
      DeleteVectorIfOwn(); 
      length = l; 
      Allocate();
    } else {
      Zero();
      CancelSelfDot();
    }
    return *this;
  }

  VectorOf& Length_old(int l) {
    DeleteVectorIfOwn(); length = l; Allocate(); return *this; }

  VectorOf& Lengthen(int);
  VectorOf& Append(const VectorOf&);
  VectorOf& Prepend(const VectorOf&);

  VectorOf& Append(Type v) { return Push(v); }
  VectorOf& Prepend(Type v);

  VectorOf& Remove(int);
  VectorOf& Insert(int, Type, int = -1);

  VectorOf& Components(const VectorOf<int>&);

  VectorOf& Push(Type v) { Lengthen(length+1); Set(length-1, v); return *this;}
  Type Pop()  { Type r = Get(length-1); Lengthen(length-1); return r; }
  Type Peek() { return Get(length-1); }
  Type PeekPeek() { return Get(length-2); }
  Type Pull() {
    Type r = Get(0);
    for (int i=0; i<length-1; i++)
      Set(i, Get(i+1));
    Lengthen(length-1);
    return r;
  }

  bool CheckDisparity(register const VectorOf& v, register int a = FALSE,
		     register const char* msg = NULL) const {
    if (v.length != length || (length && (!vector || !v.vector))) {
      if (a)
	AnnounceDisparity(v, msg);
      return SimpleAbort();			 
    }
    return true;
  }

  void AnnounceDisparity(const VectorOf&, const char*) const;

  VectorOf& Multiply(double);
  //VectorOf& Multiply(int i, double m) {Set(i, Type(Get(i)*m));return *this; }

  VectorOf& Divide(double r) { return Multiply(1/r); }

  VectorOf& Add(Type);
  VectorOf& Add(int i, Type v) { Set(i, Get(i)+v); return *this; }
  VectorOf& Sub(Type v) { return Add(-v); }
  VectorOf& Sub(int i, Type v) { return Add(i, -v); }

  VectorOf& AddWithMultiply(const VectorOf&, double);

  VectorOf& Swap(int i, int j) {
    if (IndexOK(i)&&IndexOK(j)) {
      Type tmp = Get(i);
      Set(i, Get(j));
      Set(j, tmp);
    }
    return *this;
  }

  VectorOf& Move(int i, int j);

  bool Contains(Type v) const { return FirstEqual(v)!=-1; }
  int FirstEqual(Type) const;
  int NumberOfEqual(Type) const;
  int NumberOfEqual(const VectorOf&) const;

  VectorOf<int> FindIndices() const;

  VectorOf Equals(Type) const;

  VectorOf& Arrange(const VectorOf<int>&);

  VectorOf& Sort(int, int*);
  VectorOf& SortIncreasingly(int *v = NULL) { return Sort(FALSE, v);}
  VectorOf& SortDecreasingly(int *v = NULL) { return Sort(TRUE,  v);}

  VectorOf& InsertSort(int, Type, int* = NULL, bool = false);
  VectorOf& InsertSortIncreasingly(Type v) { return InsertSort(FALSE, v);}
  VectorOf& InsertSortDecreasingly(Type v) { return InsertSort(TRUE,  v);}

  VectorOf& InsertSortFixedIncreasingly(Type v, int *i = NULL) {
    return InsertSort(FALSE, v, i, true); }

  VectorOf& InsertSortFixedDecreasingly(Type v, int *i = NULL) {
    return InsertSort(TRUE,  v, i, true); }

  VectorOf& QuickSort(bool);
  VectorOf& QuickSortIncreasingly() { return QuickSort(false);}
  VectorOf& QuickSortDecreasingly() { return QuickSort(true); }

  static int qsort_compar(const void *a, const void *b);
  static int qsort_compar_minus(const void *a, const void *b);

  //  VectorOf& Order(const int*); -> Arrange()

  VectorOf Difference() const;

  VectorOf& SetDifference();

  VectorOf Cumulate() const;
  // VectorOf Squared() const; -> Sqr()
  // VectorOf Inverse() const; -> Inv()

  Type Max(int*, int, int = FALSE) const;
  Type Min(int*, int, int = FALSE) const;
  Type Maximum(int *i = NULL)         const { return Max(i, FALSE, FALSE); }
  Type Minimum(int *i = NULL)         const { return Min(i, FALSE, FALSE); }
  Type MaximumFinite(int *i = NULL)   const { return Max(i, TRUE,  FALSE); }
  Type MinimumFinite(int *i = NULL)   const { return Min(i, TRUE,  FALSE); }
  Type MaximumAbsolute(int *i = NULL) const { return Max(i, FALSE, TRUE);  }
  Type MinimumAbsolute(int *i = NULL) const { return Min(i, FALSE, TRUE);  }

  int ArgMax() const { int i; Maximum(&i); return i; }
  int ArgMin() const { int i; Minimum(&i); return i; }

  int ToString(int, char*, int, int) const;

  static NGinline Type From(const char*);

  static NGinline Type LargestPositive();
  static NGinline Type LargestNegative();
  static NGinline int IsLargestPositive(Type v);
  static NGinline int IsLargestNegative(Type v);

  Type Sum() const;

//   int MaximumDotProduct(const ListOf<VectorOf>&, Type* = NULL,
// 			int = FALSE) const;
//   int MinimumDistance(const ListOf<VectorOf>&, float* = NULL) const;

  double NormSquared(double = 2) const;
  double Norm(double d = 2) const {
    double tmp;
    return
      d==2        ? sqrt(NormSquared(2)) :
      d==1        ? NormSquared(1) :
      d==0        ? (tmp = MinimumAbsolute())>=0 ? tmp : -tmp :
      d==MAXFLOAT ? (tmp = MaximumAbsolute())>=0 ? tmp : -tmp :
      pow(NormSquared(d), 1/d); }

  VectorOf& Normalize(double = 2);
  VectorOf& DirectToPositive() { if (Sum()<0.0) Multiply(-1); return *this; }
  VectorOf& Randomize();
  VectorOf& MoveTowards(const VectorOf&, double = 1);
  VectorOf& RemoveMean();

  bool IsAllocated() { return vector!=NULL; }

  int       IsNumberSet() const { return number!=-1; }
  int       Number() const { return number; }
  VectorOf& Number(int n)  { number = n; return *this; }

  VectorOf& Label(const char *l) { CopyString(label, l); return *this; }
  VectorOf& Label(const std::string& l) { return Label(l.c_str()); }

  std::string LabelStr() const { return label ? label : ""; }

  char *Label()      const       { return label;                       }
  int   EmptyLabel() const       { return !label || !*label;           }
  int   HasLabel() const         { return !EmptyLabel();               }

  bool HasVector() const { return vector; }

  int LabelsMatch(const char *str) const {
    return (!label && !str) || StringsMatch(label, str);
  }

  int LabelsMatch(const VectorOf& v) const { return LabelsMatch(v.label); }

  void Classification(const VectorOf *c);
  VectorOf *Classification() const { return classification; }

  ///
  VectorOf& ValueFromString(const char *ptr);

  ///
  VectorOf& ValueFromString(const string& s) {
    return ValueFromString(s.c_str());
  }

  bool ReadFromString(const char*, bool = false);

  const char *WriteToString(bool = false, int = 0) const;

  int Read(istream&, bool = false);
  int ReadBinary(istream&);
  int Read(const char *n) {
    ifstream is(n); int l = 0; is >> l; Length(l); return Read(is); }

  int Write(ostream&, bool = false) const;
  int WriteBinary(ostream&) const;
  int Write(const char *n) const {
    const char *l = WriteToString();
    ofstream os(n);
    os << Length() << endl << l << endl;
    delete l;
    return os.good();
  }

  int Write(const string& n) const { return Write(n.c_str()); }

  int WriteMatlab(const string& n) const { return WriteMatlab(n.c_str()); }

  int WriteMatlab(const char *n) const {
    ofstream os(n); return WriteMatlab(os); }

  int WriteMatlab(ostream&, bool = true) const;

  int ReadMatlab(const char *n) const {
    ifstream is(n); return  ReadMatlab(is); }

  int ReadMatlab(istream&) const;

  static int DecreasingNumbers(const VectorOf& a, const VectorOf& b) {
    return b.number-a.number; }
  static int IncreasingNumbers(const VectorOf& a, const VectorOf& b) {
    return a.number-b.number; }

  int WriteGnuPlot(ostream&, int = FALSE) const;
  int WriteGnuPlot(const char *n, int s = FALSE) const {
    ofstream ofs(n);
    return WriteGnuPlot(ofs, s); }

  //double TestZeroMean() const;
  double LillieforsTest(ostream& = cout, const char* = NULL) const;
  double ChiSquareTest(const VectorOf<float>&, ostream& = cout,
		       const char* = NULL) const;
  double ChiSquareTest(int n, ostream& o = cout, const char* p = NULL) const {
    return ChiSquareTest(VectorOf<float>(n-1), o, p); }

  double CorrelationCoefficient(const VectorOf<Type>&) const;
  double CorrelationTest(const VectorOf<Type>&, ostream& = cout) const;

  double SymbolEntropy(double = 2) const;
  double SymbolEquiVocation(const VectorOf<Type>&, double = 2) const;
  double SymbolMutualInformation(const VectorOf<Type>& v, double b = 2) const {
    return SymbolEntropy(b)-SymbolEquiVocation(v, b);
  }

  Type Mean() const;
  Type Variance() const;
  Type StandardDeviation() const { return (Type)sqrt((double)Variance()); }
  Type Skewness() const;
  Type Kurtosis() const;
  const VectorOf<Type>& MeanVariance(Type&, Type&) const;
  const VectorOf<Type>& MeanStandardDeviation(Type& m, Type& d) const {
    MeanVariance(m, d); d = (Type)sqrt((double)d); return *this; }

  void GammaPdfMLEFit(float&, float&) const;
  void GammaPdfMomFit(float&, float&) const;
  void GammaPdfFit(const VectorOf<float>&, float&, float&, float&) const;
  void ReciprocFit(const VectorOf<float>&, float&, float&) const;
  void ExpFit(const VectorOf<float>&, float&, float&) const;

  static double NormPdf(double x, double m = 0, double s = 1) {
    return exp(-(x-m)*(x-m)/(2*s*s))/(s*sqrt(2*M_PI)); }
  static double NormCdf(double x, double m = 0, double s = 1) {
    return erf((x-m)/(s*M_SQRT2))/2+0.5; }
  static double NormInv(double x, double m = 0, double s = 1);

  static double TPdf(double, int);
  static double TCdf(double, int);
  static double TInv(double, int);

  static double Gamma(double x) {
    double lg = gamma(x);
    return signgam*exp(lg);
  }
  static double LnGamma(double x) { return gamma(x); }

  static double GammaPdf(double x, double a, double b) {
    if (x<=0)
      return 0;

    if (x<0.0 || a<0 || b<0) {
      // char args[100];
      // sprintf(args, "x=%g a=%g b=%g", x, a, b);
      // ShowError("Vector::GammaPdf negative argument(s): ", args);
      // Abort();
      return 0; }
    return pow(x, a-1)*exp(-x/b)/(pow(b, a)*Gamma(a));
  }
  static double GammaCdf(double, double, double = 1);
  static double GammaInv(double, double, double = 1);

  static double ChiSquarePdf(double x, double v) {
    return GammaPdf(x, v/2, 2); }
  static double ChiSquareCdf(double x, double v) {
    return GammaCdf(x, v/2, 2); }
  static double ChiSquareInv(double x, double v) {
    return GammaInv(x, v/2, 2); }

#define _set_to(f) \
for(int i=0;i<length;i++) vector[i]=(Type)f((double)vector[i]);return *this
#define _set_to_1(f,a) \
for(int i=0;i<length;i++) vector[i]=(Type)f((double)vector[i],a);return *this
#define _set_to_2(f,a,b) \
for(int i=0;i<length;i++) vector[i]=(Type)f((double)vector[i],a,b);return *this

#define _get_of_commom_ VectorOf<Type> w(length);for(int i=0;i<length;i++)
#define _get_of(f) \
_get_of_commom_ w[i]=(Type)f((double)vector[i]);return w
#define _get_of_1(f,a) \
_get_of_commom_ w[i]=(Type)f((double)vector[i],a);return w
#define _get_of_2(f,a,b) \
_get_of_commom_ w[i]=(Type)f((double)vector[i],a,b);return w

#define _inv_(x) (1.0/(x))
#define _sqr_(x) ((x)*(x))
#define _pos_(x) ((x)>Type(0.0)?Type(x):Type(0.0))

  VectorOf& SetInv()     { _set_to(_inv_);   }
  VectorOf& SetSqr()     { _set_to(_sqr_);   }
  VectorOf& SetPos()     { _set_to(_pos_);   }
  VectorOf& SetSqrt()    { _set_to(sqrt);    }
  VectorOf& SetAbs()     { _set_to(fabs);    }
  VectorOf& SetErf()     { _set_to(erf);     }
  VectorOf& SetExp()     { _set_to(exp);     }
  VectorOf& SetLog()     { _set_to(log);     }
  VectorOf& SetGamma()   { _set_to(Gamma);   }
  VectorOf& SetLnGamma() { _set_to(LnGamma); }

  VectorOf  Inv()     const { _get_of(_inv_);   }
  VectorOf  Sqr()     const { _get_of(_sqr_);   }
  VectorOf  Pos()     const { _get_of(_pos_);   }
  VectorOf  Sqrt()    const { _get_of(sqrt);    }
  VectorOf  Abs()     const { _get_of(fabs);    }
  VectorOf  Erf()     const { _get_of(erf);     }
  VectorOf  Exp()     const { _get_of(exp);  	}
  VectorOf  Log()     const { _get_of(log);  	}
  VectorOf  Gamma()   const { _get_of(Gamma);	}
  VectorOf  LnGamma() const { _get_of(LnGamma); }

  VectorOf& SetPow(double v)  { _set_to_1(pow,v); }

  VectorOf& SetTPdf(int v) { _set_to_1(TPdf,v); }
  VectorOf& SetTCdf(int v) { _set_to_1(TCdf,v); }
  VectorOf& SetTInv(int v) { _set_to_1(TInv,v); }

  VectorOf  Pow(double v)  const { _get_of_1(pow,v); }

  VectorOf  TPdf(int v) const { _get_of_1(TPdf,v); }
  VectorOf  TCdf(int v) const { _get_of_1(TCdf,v); }
  VectorOf  TInv(int v) const { _get_of_1(TInv,v); }

  VectorOf& SetChiSquarePdf(double v) { _set_to_1(ChiSquarePdf,v); }
  VectorOf& SetChiSquareCdf(double v) { _set_to_1(ChiSquareCdf,v); }
  VectorOf& SetChiSquareInv(double v) { _set_to_1(ChiSquareInv,v); }

  VectorOf  ChiSquarePdf(double v) const { _get_of_1(ChiSquarePdf,v); }
  VectorOf  ChiSquareCdf(double v) const { _get_of_1(ChiSquareCdf,v); }
  VectorOf  ChiSquareInv(double v) const { _get_of_1(ChiSquareInv,v); }

  VectorOf& SetNormPdf(double m = 0, double s = 1) { _set_to_2(NormPdf,m,s);}
  VectorOf& SetNormCdf(double m = 0, double s = 1) { _set_to_2(NormCdf,m,s);}
  VectorOf& SetNormInv(double m = 0, double s = 1) { _set_to_2(NormInv,m,s);}

  VectorOf  NormPdf(double m = 0, double s = 1) const {_get_of_2(NormPdf,m,s);}
  VectorOf  NormCdf(double m = 0, double s = 1) const {_get_of_2(NormCdf,m,s);}
  VectorOf  NormInv(double m = 0, double s = 1) const {_get_of_2(NormInv,m,s);}

  VectorOf& SetGammaPdf(double a, double b = 1) { _set_to_2(GammaPdf,a,b); }
  VectorOf& SetGammaCdf(double a, double b = 1) { _set_to_2(GammaCdf,a,b); }
  VectorOf& SetGammaInv(double a, double b = 1) { _set_to_2(GammaInv,a,b); }

  VectorOf  GammaPdf(double a, double b = 1) const { _get_of_2(GammaPdf,a,b); }
  VectorOf  GammaCdf(double a, double b = 1) const { _get_of_2(GammaCdf,a,b); }
  VectorOf  GammaInv(double a, double b = 1) const { _get_of_2(GammaInv,a,b); }

#undef _pos_
#undef _sqr_
#undef _inv_
#undef _set_to
#undef _set_to_1
#undef _set_to_2
#undef _get_of
#undef _get_of_1
#undef _get_of_2
#undef _get_of_commom_

  double SelfDot() const { return self_dot; }
  VectorOf& SelfDot(double s) { self_dot = s; return *this; }
  VectorOf& CancelSelfDot() { return SelfDot(-1); }
  VectorOf& CalculateSelfDot();
  int HasSelfDot() const { return self_dot!=-1; }

  static int DistanceComparisonInterval() { return dist_comp_ival; }
  static void DistanceComparisonInterval(int i) { dist_comp_ival = i; }

  VectorOf& StreamDisplayWidth(int w) { ios_width = w; return *this; }

  //  friend ostream& operator<< <> (ostream& os, const VectorOf<Type>& vec);

  void IntersectOrdered(const VectorOf&);

  int LabelIndex() const { return label_index; }

  VectorOf& LabelIndex(int i) { label_index = i; return *this; }

  void MapNegativeZeroPositive(Type, Type, Type);
						  
private:
  void Initialize();
  void DeleteVectorIfOwn();
  //  void DeleteVectorIfOwn() { if (own) delete [] vector; vector = NULL; }
  void Allocate(const Type* = NULL, int = MAXINT);
  void SlowZero();
  void FastZero()              { memset(vector, 0, length*sizeof(Type)); }
  void SlowCopyValues(const Type*, int);
  void FastCopyValues(const Type *v, int n) {
    Memcpy(vector, v, n*sizeof(Type)); }

  static double MatlabEps() { return 2.220446049250313e-16; }

//   static VectorOf<float> MatrixInversePreMultiply(const float*, int, int,
// 						  const VectorOf<float>&);

protected:
  Type     *vector;
  int       length;
  int       own;
  char     *label;
  VectorOf *classification;
  int       number;
  int       label_index;
  double    self_dot;
  static int dist_comp_ival;
  int       ios_width;
  static int dump_length_limit;

public:
  // static size_t n_created, n_destructed;
};

#ifdef __GNUC__
template <>
float VectorOf<float>::DotProduct(const VectorOf<float>& v) const;
#endif // __GNUC__

/*
template <class Type>
inline ostream& operator<<(ostream& os, const VectorOf<Type>& vec) {
  for (int i=0; i<vec.Length(); i++)
    vec.DisplayOne(vec.Get(i), os);
  return os;
}
*/

template <class Type>
NGinline bool VectorOf<Type>::IsFloatVector() const { return false; }

template <class Type>
inline bool VectorOf<Type>::IsDoubleVector() const { return false; }

template <class Type>
inline bool VectorOf<Type>::IsIntVector() const { return false; }

template <>
NGinline bool VectorOf<float>::IsFloatVector() const { return true; }

template <>
NGinline bool VectorOf<double>::IsDoubleVector() const { return true; }

template <>
NGinline bool VectorOf<int>::IsIntVector() const { return true; }

template <class Type>
NGinline VectorOf<Type>& VectorOf<Type>::Zero() { SlowZero(); return *this; }

template <class Type>
NGinline void VectorOf<Type>::CopyValues(const Type *v, int n) {
  SlowCopyValues(v, n); 
}

template <>
NGinline void VectorOf<float>::CopyValues(const float *v, int n) {
  FastCopyValues(v, n); 
}

template <>
NGinline void VectorOf<int>::CopyValues(const int *v, int n) {
  FastCopyValues(v, n); 
}

template <class Type>
NGinline int VectorOf<Type>::IsLargestPositive(Type v) {
  return v==LargestPositive(); 
}

template <class Type>
NGinline int VectorOf<Type>::IsLargestNegative(Type v) {
  return v==LargestNegative(); 
}

template <>
NGinline int VectorOf<int>::LargestPositive() { return  MAXINT; }

template <>
NGinline int VectorOf<int>::LargestNegative() { return -MAXINT; }

template <>
NGinline float VectorOf<float>::LargestPositive() { return  MAXFLOAT; }

template <>
NGinline float VectorOf<float>::LargestNegative() { return -MAXFLOAT; }

template <>
NGinline double VectorOf<double>::LargestPositive() { return  MAXDOUBLE; }

template <>
NGinline double VectorOf<double>::LargestNegative() { return -MAXDOUBLE; }

template <class Type>
NGinline Type VectorOf<Type>::LargestPositive() { return Type::Maximum(); }

template <class Type> 
NGinline Type VectorOf<Type>::LargestNegative() { return Type::Minimum(); }

template <>
NGinline int   VectorOf<int>::From(const char *s) { return    atoi(s); }

template <>
NGinline float VectorOf<float>::From(const char *s) { return my_atof(s); }

template <>
NGinline double VectorOf<double>::From(const char *s) { return atof(s); }

template <class Type> NGinline Type VectorOf<Type>::From(const char *s) {
  return s; }

#define define_vector_type_name(x, y) \
template <> inline const char *VectorOf< x >::TypeName() const { return #x; } \
typedef VectorOf< x > y

define_vector_type_name(int,        IntVector);
define_vector_type_name(float,      FloatVector);
define_vector_type_name(double,     DoubleVector);
define_vector_type_name(SimpleComplex,    SimpleComplexVector);
define_vector_type_name(IntPoint,   IntPointVector);
define_vector_type_name(FloatPoint, FloatPointVector);

template <>
void FloatVector::ExpFit(const VectorOf<float>&, float&, float&) const;

#if (!defined(__GNUC__) || __GNUC__==3)
template <>
double FloatVector::ChiSquareTest(const VectorOf<float>&, ostream&,
				  const char*) const;
#endif // !GNUC4

#ifdef EXPLICIT_INCLUDE_CXX
#include <Vector.C>
#endif // EXPLICIT_INCLUDE_CXX

} // namespace simple
#endif // _VECTOR_H_

