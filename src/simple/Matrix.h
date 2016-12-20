// -*- C++ -*-  $Id: Matrix.h,v 1.11 2016/10/25 08:16:25 jorma Exp $
// 
// Copyright 1994-2016 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2016 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _MATRIX_H_
#define _MATRIX_H_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <VectorSet.h>

namespace simple {

template <class Type> class MatrixOf : public/*protected*/ VectorSetOf<Type> {
public:
  using VectorSetOf<Type>::Nitems;
  using VectorSetOf<Type>::Remove;
  using VectorSetOf<Type>::CopyString;
  using VectorSetOf<Type>::StringsMatch;
  using VectorSetOf<Type>::ShowError;
  using VectorSetOf<Type>::Append;
  using VectorSourceOf<Type>::ShowString;
  // VectorSetOf<Type>::;

public:
  MatrixOf(int = 0, int = 0, const Type* = NULL);
  MatrixOf(const MatrixOf& s);
  MatrixOf(const VectorSetOf<Type>&);
  MatrixOf(const char *name);
  virtual ~MatrixOf();

  MatrixOf& operator=(const MatrixOf&);
  MatrixOf& operator=(const Type*);

  MatrixOf& operator*=(double m) { return Multiply(m); }
  MatrixOf& operator/=(double m) { return Multiply(1/m); }

  MatrixOf operator*(double m) const {
    MatrixOf ret(*this);
    for (int i=0; i<Columns(); i++)
      *ret.GetColumn(i) *= m;
    return ret;
  }

  MatrixOf& operator+=(const MatrixOf&);
  MatrixOf& operator-=(const MatrixOf&);

  MatrixOf operator+(const MatrixOf& m) const {
    if (!SizesMatch(m))
      return MatrixOf();

    MatrixOf ret = *this;
    for (int i=0; i<Columns(); i++)
      *ret.GetColumn(i) += *m.GetColumn(i);
    
    return ret;
  }

  MatrixOf operator-(const MatrixOf& m) const {
    if (!SizesMatch(m))
      return MatrixOf();

    MatrixOf ret = *this;
    for (int i=0; i<Columns(); i++)
      *ret.GetColumn(i) -= *m.GetColumn(i);
    
    return ret;
  }

  Type operator()(int i, int j) const { return Get(i, j); }

  virtual void Dump(Simple::DumpMode = Simple::DumpDefault, ostream& = cout) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "Matrix", "VectorSet", "List", NULL }; return n; }

  bool SizesMatch(const MatrixOf& m) const {
    return Columns()==m.Columns() && Rows()==m.Rows(); }

  virtual int AllowLocateFile() { return FALSE; }

  inline bool IsFloatMatrix() const;
  inline bool IsDoubleMatrix() const;
  inline bool IsIntMatrix() const;

  using VectorSetOf<Type>::VectorLength;
  using VectorSetOf<Type>::Read;
  using VectorSetOf<Type>::Write;
  using VectorSetOf<Type>::Delete;
  using VectorSetOf<Type>::IsIncluded;

  bool IndexOK(int i, int j) const {
    return i>=0 && j>=0 && j<Columns() && i<Rows(); }

  VectorOf<Type> *GetColumn(int i) const { return VectorSetOf<Type>::Get(i); }

  VectorOf<Type> *Get(int) const { return NULL; }

  Type Get(int i, int j) const {
    return IndexOK(i, j)?FastGet(i, j):Type(0); }

  Type FastGet(int i, int j) const {
    // the following stupidity is for g++'s sake
    return (this->list)[j]->FastGet(i); }

  const MatrixOf& Set(int i, int j, Type v) const {
    if (IndexOK(i, j))
      FastSet(i, j, v);
    return *this; }

  const MatrixOf& FastSet(int i, int j, Type v) const {
    // the following stupidity is for g++'s sake
    (this->list)[j]->FastSet(i, v); 
    return *this; }

  int Size() const { return Rows()*Columns(); }
  MatrixOf& Size(int, int);
  MatrixOf& Size(const MatrixOf<Type>& m) {
    return Size(m.Rows(), m.Columns()); }

  void SizeOrZero(const MatrixOf<Type>& m) {
    if (IsSameSize(m))
      Zero();
    else
      Size(m);
  }

  void SizeOrZero(int r, int c) {
    if (IsSameSize(r, c))
      Zero();
    else
      Size(r, c);
  }

  bool IsSameSize(int r, int c) const {
    return Rows()==r && Columns()==c;
  }

  bool IsSameSize(const MatrixOf& m) const {
    return IsSameSize(m.Rows(), m.Columns());
  }

  int Rows()    const { return Nitems() ? GetColumn(0)->Length() : 0; }
  int Columns() const { return Nitems(); }

  int RemoveRow(int);
  int RemoveColumn(int i) { return Remove(i); }

  VectorOf<Type> Row(int) const;

  VectorOf<Type> Diagonal() const;
  MatrixOf<Type>& Diagonal(const VectorOf<Type>&);

  MatrixOf& AppendRow(const VectorOf<Type>* = NULL);
  MatrixOf& AppendColumn(VectorOf<Type> *c = NULL, int = TRUE);
  //MatrixOf& PrependRow(VectorOf<Type>*);      // not implemented
  //MatrixOf& PrependColumn(VectorOf<Type> *c); // not implemented

  const MatrixOf& SwapColumns(int, int) const;
  const MatrixOf& SwapRows(int, int) const;
  const MatrixOf& SwapColumnsAndRows(int i, int j) const {
    SwapColumns(i, j); return SwapRows(i, j); }

  MatrixOf& Zero();
  MatrixOf& One();

  MatrixOf& Multiply(double);
  MatrixOf& Divide(double r) { return Multiply(1/r); }

  MatrixOf& Add(Type v) {
    for (int i=0; i<Rows(); i++) for (int j=0; j<Columns(); j++) Add(i, j, v);
    return *this; }

  MatrixOf& Add(int i, int j, Type v) {
    if (IndexOK(i, j)) Set(i, j, Get(i, j)+v); 
    return *this; }

  MatrixOf& Multiply(int i, int j, double v) {
    if (IndexOK(i, j)) Set(i, j, Type(Get(i, j)*v)); 
    return *this; }

  MatrixOf& AddOuterProduct(const VectorOf<Type>&, double = 1.0);

  bool IsSymmetric(bool = true) const;
  bool IsSquare() const { return Columns() && Columns()==Rows(); }
  bool IsZero() const {
    for (int c=0; c<Columns(); c++)
      if (!GetColumn(c)->IsZero())
	return false;
    return true;
  }

  Type *Concatenate() const;

  Type *TransposeConcatenate() const;

  VectorOf<Type> PreMultiplyFloat(const FloatVector&) const;

  VectorOf<Type> PreMultiply(const VectorOf<Type>&, int = -1) const;
  VectorOf<Type> PreTransMultiply(const VectorOf<Type>&, int = -1) const;
  VectorOf<Type> PostMultiplyTrans(const VectorOf<Type>& v, int d = -1) const {
    return PreTransMultiply(v, d); }
  VectorOf<Type> PostTransMultiplyTrans(const VectorOf<Type>& v,
					int d = -1) const {
    return PreMultiply(v, d); }

  MatrixOf PreMultiply(const MatrixOf&) const;
  // MatrixOf PostMultiply(const MatrixOf&) const;
  MatrixOf PreTransMultiply(const MatrixOf&) const;
  MatrixOf PostTransMultiply(const MatrixOf&) const;

  double GeneralizedDotProduct(const VectorOf<Type>&,
			       const VectorOf<Type>&) const;

  MatrixOf& Randomize();
  MatrixOf& Normalize();
  MatrixOf& Orthogonalize();
  MatrixOf& Orthonormalize();

  void OrthonormalityError(float* = NULL, float* = NULL, int = TRUE) const;

  MatrixOf Inverse(float* = NULL) const;
  MatrixOf InverseSgiMath(float* = NULL) const;

  VectorOf<Type> SVD(MatrixOf<Type>* = NULL, MatrixOf<Type>* = NULL) const;

  Type Determinant() const;
  Type GeneralizedInnerProduct(const VectorOf<Type>&) const;

  MatrixOf SelfOuterProduct() const { return PostTransMultiply(*this); }
  MatrixOf SelfInnerProduct() const { return PreTransMultiply( *this); }

  MatrixOf EigenVectors(VectorOf<Type>* = NULL, int = 0) const;
  VectorOf<Type> EigenValues() const;

  MatrixOf Transpose(int = FALSE);

  int WriteGnuPlot(const char *n) const {
    ofstream os(n); return WriteGnuPlot(os); }
  int WriteGnuPlot(ostream&) const;
  int ReadGnuPlot(const char *n) const {
    ifstream is(n); return  ReadGnuPlot(is); }
  int ReadGnuPlot(istream&) const;

  int WriteMatlab(const string& n) const { return WriteMatlab(n.c_str()); }

  int WriteMatlab(const char *n) const {
    ofstream os(n); return WriteMatlab(os); }

  int WriteMatlab(ostream&) const;

  int ReadMatlab(const char *n) const {
    ifstream is(n); return  ReadMatlab(is); }

  int ReadMatlab(istream&) const;

  MatrixOf& Label(const char *l) { CopyString(label, l); return *this; }
  MatrixOf& Label(const std::string& s) { return Label(s.c_str()); }

  char *Label()      const       { return label;             }
  int   EmptyLabel() const       { return !label || !*label; }

  int LabelsMatch(const char *str) const {
    return (!label && !str) || StringsMatch(label, str);
  }
  int LabelsMatch(const MatrixOf& m) const { return LabelsMatch(m.label); }
  int LabelsMatch(const VectorOf<Type>& v) const {
    return LabelsMatch(v.Label());
  }

  Type Sum() const {
    Type sum = 0;
    for (int i=0; i<Columns(); i++)
      sum += GetColumn(i)->Sum();
    return sum;
  }

  Type Minimum() const {
    Type m = VectorOf<Type>::LargestPositive();
    for (int i=0; i<Columns(); i++) {
      Type t = GetColumn(i)->Minimum();
      if (t<m)
	m = t;
    }
    return m;
  }

  Type Maximum() const {
    Type m = VectorOf<Type>::LargestNegative();
    for (int i=0; i<Columns(); i++) {
      Type t = GetColumn(i)->Maximum();
      if (t>m)
	m = t;
    }
    return m;
  }

  Type MaximumAbsolute() const {
    Type m = VectorOf<Type>::LargestNegative();
    for (int i=0; i<Columns(); i++) {
      Type t = GetColumn(i)->MaximumAbsolute();
      if (t>m)
	m = t;
    }
    return m;
  }

  Type DotProduct(const MatrixOf<Type>& m) const {
    Type s = 0;
    if (Rows()!=m.Rows() || Columns()!=m.Columns())
      ShowError("Matrix::DotProduct(const Matrix&) dimensionality mismatch");
    else
      for (int c=0; c<Columns(); c++)
	s += GetColumn(c)->DotProduct(*m.GetColumn(c));

    return s;
  }

  Type AbsDiffSum(const MatrixOf<Type>& m) const {
    Type s = 0;
    if (Rows()!=m.Rows() || Columns()!=m.Columns())
      ShowError("Matrix::AbsDiffSum(const Matrix&) dimensionality mismatch");
    else
      for (int c=0; c<Columns(); c++)
	s += GetColumn(c)->AbsDiffSum(*m.GetColumn(c));

    return s;
  }

  static MatrixOf SemiOrthonormalBasis(int, int, int = 100, int = 100,
				       float* = NULL);

  double Entropy(double = 2) const;

  double EntropyWithCheck(bool, double = 2) const;

  static bool BlasTest(int = 1);

  static bool BlasTestInverse(int);
  bool TestInverse(const MatrixOf<Type>&, int, int) const;

  static bool BlasTestMahalanobis(int);
  static bool TestMahalanobis(const VectorSetOf<Type>&, const VectorOf<Type>&,
			      const VectorOf<Type>&, double, int, int);

  static bool BlasTestMatrixVectorProduct(int);
  
  static bool BlasTestDotProduct(int);

  static bool BlasTestEigenVectors(int);

protected:
  int transposed;
  char *label;

}; // template class MatrixOf

template <class Type>
inline bool MatrixOf<Type>::IsFloatMatrix() const { return false; }

template <class Type>
inline bool MatrixOf<Type>::IsDoubleMatrix() const { return false; }

template <class Type>
inline bool MatrixOf<Type>::IsIntMatrix() const { return false; }

template <>
inline bool MatrixOf<float>::IsFloatMatrix() const { return true; }

template <>
inline bool MatrixOf<double>::IsDoubleMatrix() const { return true; }

template <>
inline bool MatrixOf<int>::IsIntMatrix() const { return true; }

typedef MatrixOf<int> IntMatrix;
typedef MatrixOf<float> FloatMatrix;
typedef MatrixOf<double> DoubleMatrix;
typedef MatrixOf<SimpleComplex> SimpleComplexMatrix;

template <>
FloatMatrix FloatMatrix::EigenVectors(FloatVector*, int) const;

#ifdef EXPLICIT_INCLUDE_CXX
#include <Matrix.C>
#endif // EXPLICIT_INCLUDE_CXX

} // namespace simple
#endif // _MATRIX_H_

