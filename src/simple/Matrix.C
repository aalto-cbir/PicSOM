// -*- C++ -*-  $Id: Matrix.C,v 1.12 2020/04/03 12:29:32 jormal Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2020 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _MATRIX_C_
#define _MATRIX_C_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <Matrix.h>

namespace simple {

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>::MatrixOf(int d, int n, const Type *m) {
  for (int i=0; i<n; i++) {
    Append(new VectorOf<Type>(d, m));
    if (m)
      m += d;
  }

  label = NULL;
  transposed = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>::MatrixOf(const MatrixOf<Type>& m) :
  SourceOf<VectorOf<Type> >(), // obs! this really (), not (m) ! 
  VectorSetOf<Type>(m) {
  label = NULL;
  *this = m;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>::MatrixOf(const VectorSetOf<Type>& s) : VectorSetOf<Type>(s) {
  label = NULL;
  transposed = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>::MatrixOf(const char *name) : VectorSetOf<Type>(name) {
  label = NULL;
  transposed = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>::~MatrixOf() {
  delete label;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void MatrixOf<Type>::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&Simple::DumpRecursive)
    VectorSetOf<Type>::Dump(type, os);

  if (type&Simple::DumpShort || type&Simple::DumpLong) {
    os << Simple::Bold("   Matrix ") << (void*)this
       << " (rows="      << Rows()
       << " columns="    << Columns() << ")"
       << " transposed=" << transposed
       << " label="      << ShowString(label)
       << endl;
  }

  if (type&Simple::DumpLong)
    for (int i=0; i<Rows(); i++) {
      for (int j=0; j<Columns(); j++)
	os << Get(i, j) << (j<Columns()-1 ? " " : "");
      os << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::operator=(const MatrixOf<Type>& m) {
  *(VectorSetOf<Type>*)this = m;

  transposed = m.transposed;
  Label(m.Label());

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::operator=(const Type* m) {
  for (int i=0; i<Columns(); i++) {
    *GetColumn(i) = m;
    m += Rows();
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::operator+=(const MatrixOf<Type>& m) {
  if (Columns()!=m.Columns())
    cerr << "Matrix::operator+= disparity" << endl;

  else
    for (int i=0; i<Columns(); i++)
      *GetColumn(i) += *m.GetColumn(i);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::operator-=(const MatrixOf<Type>& m) {
  if (Columns()!=m.Columns())
    cerr << "Matrix::operator+= disparity" << endl;

  else
    for (int i=0; i<Columns(); i++)
      *GetColumn(i) -= *m.GetColumn(i);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type *MatrixOf<Type>::Concatenate() const {
  Type *ret = new Type[Size()], *ptr = ret;
  for (int i=0; i<Columns(); i++) {
    Memcpy(ptr, (Type*)*GetColumn(i), Rows()*sizeof(Type));
    ptr += Rows();
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type *MatrixOf<Type>::TransposeConcatenate() const {
  Type *ret = new Type[Size()], *ptr = ret;
  for (int j=0; j<Rows(); j++)
    for (int i=0; i<Columns(); i++)
      *(ptr++) = FastGet(j, i);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Randomize() {
  for (int i=0; i<Nitems(); i++)
    GetColumn(i)->Randomize();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Normalize() {
  for (int i=0; i<Nitems(); i++)
    GetColumn(i)->Normalize();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Orthogonalize() {
  cerr << "Matrix::Orthogonalize not yet implemented" << endl;

  // code to be inserted
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double MatrixOf<Type>::GeneralizedDotProduct(const VectorOf<Type>& l,
					     const VectorOf<Type>& r) const {
  double sum = 0;
  for (int i=0; i<l.Length(); i++)
    sum += GetColumn(i)->DotProduct(l)*r[i];

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> MatrixOf<Type>::PreTransMultiply(const VectorOf<Type>& v,
						int dim) const {
  if (dim<0)
    dim = Columns();

  VectorOf<Type> res(dim, NULL, v.Label());
  const int l = res.Length();
  for (int i=0; i<l; i++)
    res.FastSet(i, GetColumn(i)->DotProduct(v));

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> MatrixOf<Type>::PreMultiply(const VectorOf<Type>& v,
					   int dim) const {
  if (dim<0)
    dim = Columns();

  if (v.Length()<dim) {
    cerr << "MatrixOf::PreMultiply dimension mismatch: "
	 << dim << " " << v.Length() << endl;
    return VectorOf<Type>();
  }

  VectorOf<Type> res(VectorLength(), NULL, v.Label());
  const int l = res.Length(), m = dim;
  for (int i=0; i<l; i++) {
    register double sum = 0.0;
    for (int j=0; j<m; j++)
      sum += v[j]*FastGet(i, j);
      // sum += v[j]*GetColumn(j)->Get(i);

    res.FastSet(i, (Type)sum);
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> MatrixOf<Type>::PreMultiplyFloat(const FloatVector& v) const {
  if (v.Length()!=Nitems()) {
    cerr << "MatrixOf::PreMultiplyFloat dimension mismatch" << endl;
    VectorOf<Type> res;
    return res;
  }

  VectorOf<Type> res(VectorLength(), NULL, v.Label());
  for (int i=0; i<res.Length(); i++) {
    register Type sum = 0; // was 0.0;
    for (int j=0; j<v.Length(); j++)
      sum += Type(GetColumn(j)->Get(i)*(double)v[j]);

    res.Set(i, sum);
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::PreMultiply(const MatrixOf<Type>& m)const{
  if (m.Rows()!=Columns()) {
    cerr << "MatrixOf::TransMultiply dimension mismatch" << endl;
    return MatrixOf<Type>();
  }

  MatrixOf<Type> ret;

  for (int i=0; i<m.Columns(); i++)
    ret.AppendColumn(new VectorOf<Type>(PreMultiply(*m.GetColumn(i))));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::PreTransMultiply(const MatrixOf<Type>& m)const{
  if (m.Rows()!=Rows()) {
    cerr << "MatrixOf::PreTransMultiply dimension mismatch" << endl;
    return MatrixOf<Type>();
  }

  MatrixOf<Type> ret;

  for (int i=0; i<m.Columns(); i++)
    ret.AppendColumn(new VectorOf<Type>(PreTransMultiply(*m.GetColumn(i))));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::PostTransMultiply(const MatrixOf<Type>& m)const{
  if (m.Columns()!=Columns()) {
    cerr << "MatrixOf::PostTransMultiply dimension mismatch" << endl;
    return MatrixOf<Type>();
  }

  MatrixOf<Type> ret;

  for (int i=0; i<Rows(); i++)
    ret.AppendColumn(new VectorOf<Type>(m.PreMultiply(Row(i))));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool MatrixOf<Type>::IsSymmetric(bool verbose) const {
  if (!IsSquare())
    return false;

  for (int i=0; i<Columns(); i++)
    for (int j=i+1; j<Columns(); j++)
      if (Get(i, j) != Get(j, i)) {
	if (verbose)
	  cerr << "MatrixOf::IsSymmetric: not symmetric due to (" << i
	       << "," << j << ")=" << Get(i, j) << " but (" << j
	       << "," << i << ")=" << Get(j, i) << " difference="
	       << Get(i, j)-Get(j, i) << endl;
	return false;
      }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::EigenVectors(VectorOf<Type> *v, int) const {
  static bool first = true;
  if (first)
    ShowError("Matrix::EigenVectors() of type ", v->TypeName(),
	      " not implemented");
  first = false;
  return MatrixOf<Type>();
}

//----------------------------------------------------------------------

extern "C" {
#ifdef HAS_LINPACK
  int ilaenv_(int*, char*, char*, int*, int*, int*, int*);
  void ssyevx_(char*, char*, char*, int*, float*, int*, float*,
   	       float*, int*, int*, float*, int*, float*, float*,
   	       int*, float*, int*, int*, int*, int*);
  void sgetrf_(int*, int*, float*, int*, int*, int*);
  void sgetri_(int*, float*, int*, int*, float*, int*, int*);
  
#else
  int ilaenv_(int*, char*, char*, int*, int*, int*, int*) {
    Simple::ShowError("ilaenv not implemented."); return 0; }
  void ssyevx_(char*, char*, char*, int*, float*, int*, float*,
	       float*, int*, int*, float*, int*, float*, float*,
	       int*, float*, int*, int*, int*, int*) {
    Simple::ShowError("ssyevx not implemented."); }
  void sgetrf_(int*, int*, float*, int*, int*, int*) {
    Simple::ShowError("sgetrf not implemented."); }
  void sgetri_(int*, float*, int*, int*, float*, int*, int*) {
    Simple::ShowError("sgetri not implemented."); }
#endif // HAS_LINPACK
}

#ifdef HAS_SGIMATH
extern "C" {
  void sgeco_(float*, int&, int&, int*, float&, float*);
  void sgedi_(float*, int&, int&, int*, float*, float*, int&);
  void svd_(int&, int&, int&, float*, float*,
	    int&, float*, int&, float*, int&, float*);
}
#else
static void sgeco_(float*, int&, int&, int*, float&, float*) {
  Simple::ShowError("sgeco not implemented (only in SGIMATH)."); }
static void sgedi_(float*, int&, int&, int*, float*, float*, int&) {
  Simple::ShowError("sgedi not implemented (only in SGIMATH)."); }
static void svd_(int&, int&, int&, float*, float*,
		 int&, float*, int&, float*, int&, float*) {
  Simple::ShowError("svd not implemented (only in SGIMATH)."); }
#endif // HAS_SGIMATH

//----------------------------------------------------------------------

template <>
FloatMatrix FloatMatrix::EigenVectors(FloatVector* val, int n) const {
  if (!n)
    n = Columns();

  if (val)
    val->Zero();

  if (!IsSymmetric()) {
    cerr << "Cannot yet eigenvectorize unsymmetric matrices." << endl;
    return *this;
  }

  char ssytrd[100] = "SSYTRD", empty[100] = "";

  int one = 1, zero = 0, col = Columns();
  int nb = ilaenv_(&one, ssytrd, empty, &zero, &zero, &zero, &zero);
  int lwork = nb+3>8 ? (nb+3)*col : 8*col;
  
  int first = col-n+1, m = 0, info = 0;
  int *iwork = new int[5*col], *ifail = new int[col];
  float *z = new float[n*col], *work = new float[lwork];
  float *v = new float[Columns()];
  float *a = Concatenate();
  float fzero = 0;

  char V[] = "V", I[] = "I", U[] = "U";

  ssyevx_(V, I, U, &col, (float*)(void*)a, &col,
	  &fzero, &fzero, &first, &col, &fzero, &m, 
	  v, (float*)(void*)z, &col, (float*)(void*)work, &lwork,
	  iwork, ifail, &info);

  if (info<0)
    cerr << "Matrix::EigenVectors() ssyevx argument failure" << endl;
  if (info>0)
    cerr << "Matrix::EigenVectors() ssyevx convergation failure" << endl;

  FloatMatrix ret(Rows(), n);
  ret = z;

  if (val)
    val->Lengthen(n) = v;

  delete a;
  delete v;
  delete work;
  delete z;
  delete ifail;
  delete iwork;
  
  for (int i=0; i<ret.Columns()/2; i++) {
    ret.Swap(i, ret.Columns()-1-i);
    if (val) {
      float tmp = val->Get(i);
      val->Set(i, val->Get(ret.Columns()-1-i));
      val->Set(ret.Columns()-1-i, tmp);
    }
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> MatrixOf<Type>::EigenValues() const {
  VectorOf<Type> val(Rows());
  val.Zero();

  return val;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> MatrixOf<Type>::SVD(MatrixOf<Type>*, MatrixOf<Type>*) const {
  VectorOf<Type> v;
  static bool first = true;
  if (first)
    ShowError("Matrix::SVD() of type ", v.TypeName(), " not implemented");
  first = false;
  return v;
}

//----------------------------------------------------------------------

template <>
FloatVector FloatMatrix::SVD(FloatMatrix *l, FloatMatrix *r) const{
  if (!IsSquare()) {
    cerr << "Cannot yet svd non-square matrices." << endl;
    return FloatVector();
  }

  FloatVector w(Rows());

  float *a = Concatenate();
  int matu = l!=NULL, matv = r!=NULL, ierr = -1, nm = Rows();
  float *u = new float[Size()];
  float *v = new float[Size()];
  float *rv1 = new float[nm];

  svd_(nm, nm, nm, a, w, matu, u, matv, v, ierr, rv1); 

  if (l) 
    l->Size(*this) = u;

  if (r)
    r->Size(*this) = v;

  delete rv1;
  delete u;
  delete v;
  delete a;

  if (ierr)
    cerr << "Matrix::SVD ierr=" << ierr << endl;

  w.SortDecreasingly();

  return w;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> MatrixOf<Type>::Diagonal() const {
  if (!IsSquare()) {
    cerr << "Cannot yet give diagonal of a non-square matrix." << endl;
    return VectorOf<Type>();
  }

  VectorOf<Type> ret(Rows());
  for (int i=0; i<ret.Length(); i++)
    ret[i] = Get(i, i);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Diagonal(const VectorOf<Type>& d) {
  Size(d.Length(), d.Length());
  for (int i=0; i<d.Length(); i++)
    Set(i, i, d[i]);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::Transpose(int hard) {
  if (!hard)
    transposed = TRUE;

  else if (IsSquare())
    for (int i=0; i<Columns(); i++)
      for (int j=i+1; j<Columns(); j++) {
	Type tmp = Get(i, j);
	Set(i, j, Get(j, i));
	Set(j, i, tmp);
      }

  else {
    VectorSetOf<Type> s;
    for (int i=0; i<Rows(); i++) {
      VectorOf<Type> col(Columns());
      for (int j=0; j<Columns(); j++)
	col[j] = Get(i, j);
      s.AppendCopy(col);
    }
    Delete();
    for (int k=0; k<s.Nitems(); k++) {
      s.Relinquish(k);
      AppendColumn(s.Get(k));
    }
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int MatrixOf<Type>::WriteGnuPlot(ostream& os) const {
  for (int i=0; i<Rows(); i++) {
    for (int j=0; j<Columns(); j++)
      os << Get(i, j) << " ";
    os << endl;
  }

  return os.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int MatrixOf<Type>::ReadGnuPlot(istream& is) const {

  return is.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int MatrixOf<Type>::WriteMatlab(ostream& os) const {
  if (!EmptyLabel())
    os << Label() << " = ";
  os << "[";

  for (int i=0; i<Rows(); i++) {
    for (int j=0; j<Columns(); j++)
      os << Get(i, j) << " ";
    os << ";" << endl;
  }
  os << "];" << endl << endl;

  return os.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int MatrixOf<Type>::ReadMatlab(istream& is) const {

  return is.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Zero() {
  for (int i=0; i<Columns(); i++)
    GetColumn(i)->Zero();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::One() {
  if (!IsSquare())
    cerr << "Not square matrix in MatrixOf::One" << endl;
  else {
    Zero();
    for (int i=0; i<Columns(); i++)
      GetColumn(i)->Set(i, 1);
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Multiply(double a) {
  for (int i=0; i<Columns(); i++)
    GetColumn(i)->Multiply(a);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Orthonormalize() {
  // OrthonormalityError();

  int n = Rows(), i;
  VectorOf<Type> tmpvec(n);
  Type *w = new Type[n*Columns()];

  for (i=0; i<Columns(); i++) {
    for (register int t=0; t<n; t++) {
      register Type sum = Get(t, i), *ptr1 = w;
      for (register int j=0; j<i; j++) {
	register Type ssum = 0, *ptr2 = (Type*)*GetColumn(i);
	register int p = n;
	while (p--)
	  ssum += *ptr1++ * *ptr2++;
	
	sum -= w[j*n+t]*ssum;
      }
      w[i*n+t] = sum;
      // cout << "." << flush;
    }
    // cout << endl;
    tmpvec.Share(w+i*n).Normalize();
  }

  for (i=0; i<Columns(); i++)
    *GetColumn(i) = w+i*n;

  delete w;

  // OrthonormalityError();
  
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void MatrixOf<Type>::OrthonormalityError(float *e1, float *e2, int v) const {
  float maxd = 0, maxn = 0;
  
  for (int i=0; i<Columns(); i++) {
    VectorOf<Type> r = PreTransMultiply(*GetColumn(i));
    for (int j=0; j<Columns(); j++) {
      float a = fabs(double((i==j)-r[j]));
      if (i==j && a>maxd)
	maxd = a;
      if (i!=j && a>maxn)
	maxn = a;
    }
  }

  if (e1)
    *e1 = maxd;

  if (e2)
    *e2 = maxn;

  if (v)
    cout << "Matrix::OrthonormalityError diagonal: "
	 << maxd << "  other: " << maxn << endl;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::AppendColumn(VectorOf<Type> *v, int o) {
  if (v && Rows() && v->Length()!=Rows())
    cerr << "Matrix::AppendColumn dimensions don't match "
	 << v->Length() << " " << Rows() << endl;
  
  else
    if (v)
      Append(v, o);
    else
      Append(new VectorOf<Type>(Rows()));

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::AppendRow(const VectorOf<Type> *vec) {
  if (vec && Columns() && vec->Length()!=Columns())
    cerr << "Matrix::AppendRow dimensions don't match "
	 << vec->Length() << " " << Columns() << endl;
  
  else {
    int i;
    if (!Columns() && vec)
      for (i=0; i<vec->Length(); i++)
	AppendColumn();

    int r = Rows();
    for (i=0; i<Columns(); i++) {
      GetColumn(i)->Lengthen(r+1);
      if (vec)
	Set(r, i, vec->Get(i));
    }
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int MatrixOf<Type>::RemoveRow(int row) {
  if (row>=Rows() || row<0)
    return 0;
  
  for (int col=0; col<Columns(); col++)
    GetColumn(col)->Remove(row);

  return 1;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const MatrixOf<Type>& MatrixOf<Type>::SwapColumns(int i, int j) const {
  if (i<0 || j<0 || i>=Columns() || j>=Columns())
    cerr << "Matrix::SwapColumns invalid index: " << i << " " << j << endl;

  else {
    for (int k=0; k<Rows(); k++) {
      Type tmp = Get(k, i);
      Set(k, i, Get(k, j));
      Set(k, j, tmp);
    }
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const MatrixOf<Type>& MatrixOf<Type>::SwapRows(int i, int j) const {
  if (i<0 || j<0 || i>=Rows() || j>=Rows())
    cerr << "Matrix::SwapRows invalid index: " << i << " " << j << endl;

  else {
    for (int k=0; k<Columns(); k++) {
      Type tmp = Get(i, k);
      Set(i, k, Get(j, k));
      Set(j, k, tmp);
    }
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::Size(int y, int x) {
  Delete();

  if (y<1 || x<1) {
    cerr << "Matrix::Size(" << y << "," << x << ") illegal" << endl;
    abort();
  }

  while (x--)
    AppendColumn(new VectorOf<Type>(y));

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type>& MatrixOf<Type>::AddOuterProduct(const VectorOf<Type>& vec,
						register double mul) {
  register Type *v = (Type*)vec;
  register int l = vec.Length();

  for (register int j=0; j<l; j++) {
    register Type *col = (Type*)*GetColumn(j);
    for (register int k=0; k<l; k++)
      col[k] += (Type)(v[j]*v[k]*mul);
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type MatrixOf<Type>::Determinant() const {
  cerr << "Matrix::Determinant non-existent" << endl;
  return 1;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::Inverse(float *determinant) const {
  if (IsSquare()) {
    Type *a = Concatenate();
    int col = Columns(), *ipiv = new int[Columns()], info = -1;

    sgetrf_(&col, &col, (float*)(void*)a, &col, ipiv, &info);
    if (info) {
      ShowError("Matrix::Inverse() sgetrf_() failed.");
      return MatrixOf();
    }
    info = -1;
    int lwork = col*col;
    float *work = new float[lwork];

    sgetri_(&col, (float*)(void*)a, &col, ipiv, work, &lwork, &info);
    if (info) {
      ShowError("Matrix::Inverse() sgetri_() failed.");
      return MatrixOf();
    }

    MatrixOf ret(col, col, a);

    delete [] a;
    delete [] work;
    delete [] ipiv;

    if (determinant) {
      ShowError("Matrix::Inverse() doesn't calculate determinant");
      *determinant = 0;
    }

    return ret.Label(Label());

  } else {

    MatrixOf<Type> mat, inv, ret;

    if (Rows()<Columns())
      mat = SelfOuterProduct();
    else
      mat = SelfInnerProduct();
    
    inv = mat.Inverse();

    if (Rows()<Columns())
      ret = PreTransMultiply(inv);
    else
      ret = PostTransMultiply(inv);
    
    if (determinant)
      *determinant = 0; // obs?

    return ret.Label(Label());
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::InverseSgiMath(float *determinant) const {
  if (IsSquare()) {
    Type *a = Concatenate();
    int job = 11, col = Columns(), *ipvt = new int[Columns()];
    float det[2] = {0,0};
    float rcond, *work = new float[col*col];

    sgeco_((float*)(void*)a, col, col, ipvt, rcond, work);
    sgedi_((float*)(void*)a, col, col, ipvt, det, work, job);

    MatrixOf<Type> ret(col, col, a);

    delete a;
    delete work;
    delete ipvt;

    if (det[0]==0) {
      ShowError("Matrix::Inverse() failed");
      return MatrixOf();
    }

    if (determinant)
      *determinant = det[0]*pow(double(10), double(det[1]));

    return ret.Label(Label());

  } else {

    MatrixOf<Type> mat, inv, ret;

    if (Rows()<Columns())
      mat = SelfOuterProduct();
    else
      mat = SelfInnerProduct();
    
    inv = mat.Inverse();

    if (Rows()<Columns())
      ret = PreTransMultiply(inv);
    else
      ret = PostTransMultiply(inv);
    
    if (determinant)
      *determinant = 0; // obs?

    return ret.Label(Label());
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type MatrixOf<Type>::GeneralizedInnerProduct(const VectorOf<Type>& v) const {
  Type sum = 0;
  for (int i=0; i<Columns(); i++)
    sum += v[i]*GetColumn(i)->DotProduct(v);

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> MatrixOf<Type>::Row(int j) const {
  VectorOf<Type> v(Columns());

  for (int i=0; i<v.Length(); i++)
    v[i] = (*this)(j, i);

  return v;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> MatrixOf<Type>::SemiOrthonormalBasis(int d, int n,
						    int r, int s, float *err) {
  MatrixOf<Type> ret;
  float emax = MAXFLOAT;
  VectorOf<Type> v(d);

  for (int k=0; k<r; k++) {
    //cout << "k=" << k << endl;
    MatrixOf<Type> m;
    float e = 0;
    for (int l=0; l<n; l++) {
      //cout << "l=" << l << endl;
      float verr = 0;
      for (int o=0; o<s; o++) {
	v.Randomize().Normalize();
	//cout << "now new vector v" << endl;
	verr = 0;
	bool ok = true;
	for (int p=0; ok && p<m.Columns(); p++) {
	  float q = fabs(double(m.GetColumn(p)->DotProduct(v)));
	  //cout << "q=" << q << " verr=" << verr << " e=" << e << endl;
	  if (q>verr) verr = q;
	  if (q>emax) ok = false;
	}
	if (ok) {
	  m.AppendColumn(new VectorOf<Type>(v));
	  //cout << "added vector, ncolumns=" << m.Columns() << endl;
	  break;
	}
      }
      if (m.Columns()!=l+1)
	break;
      if (verr>e) e = verr;
    }
    if (m.Columns()==n && e<emax) {
      ret = m;
      emax = e;
    }
  }
  if (err) *err = emax;

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double MatrixOf<Type>::Entropy(double b) const {
  if (Minimum()<0)
    return MAXFLOAT;

  float sum = (float)Sum(), e = 0;
  if (!sum)
    return 0;

  for (int i=0; i<Rows(); i++)
    for (int j=0; j<Columns(); j++)
      if (Get(i,j))
	e += Get(i,j)*log(Get(i,j)/sum);

  return -e/(sum*log(b));
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double MatrixOf<Type>::EntropyWithCheck(bool warn, double b) const {
  bool found = false;

  float sum = 0, e = 0;
  for (int i=0; i<Rows(); i++)
    for (int j=0; j<Columns(); j++)
      if (Get(i,j)>0)
	sum += Get(i,j);
      else if (Get(i,j)<0)
	found = true;

  if (found && warn)
    ShowError("Matrix::EntropyWithCheck() : negative value(s) found");
    
  if (!sum)
    return 0;

  for (int i=0; i<Rows(); i++)
    for (int j=0; j<Columns(); j++)
      if (Get(i,j)>0)
	e += Get(i,j)*log(Get(i,j)/sum);

  return -e/(sum*log(b));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <>
bool MatrixOf<float>::BlasTestInverse(int v) {
  if (v)
    cout << "FloatMatrix::BlasTestInverse() starting." << endl;

  bool ok = true;

  float m1[] = {    1, -1,  2, -3,
		   3, -2, -1,  1,
		   1,  2, -2,  2,
		   -2, 2,  3,  1 };
  float i1[] = { 0.2667,    0.2000,    0.2667,    0.0667,
		0.2222,   -0.1667,    0.3889,    0.0556,
		0.1111,    0.1667,   -0.0556,    0.2778,
		-0.2444,    0.2333,   -0.0778,    0.1889 };

  ok &= MatrixOf(4, 4, m1).TestInverse(MatrixOf(4, 4, i1), 1, v);

  if (v>1 || (v<-1 && !ok))
    cout << " FloatMatrix::BlasTestInverse() ended "
	 << (ok?"SUCCESSFULLY":"in FAILURE") << endl;

  return ok;
}

template <class Type>
bool MatrixOf<Type>::BlasTestInverse(int) {
  FunctionNotImplemented("MatrixOf<Type>::BlasTestInverse(bool)");
  return false;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool MatrixOf<Type>::TestInverse(const MatrixOf<Type>& corr,
				 int num, int verbose) const {
  bool ok = true, shown = false, corr_shown = false;
  for (int round=0; round<2; round++) {
    std::string func = "Inverse";
    MatrixOf<Type> inv;
    if (round==0)
      inv = Inverse();
    else {
#ifdef HAS_SGIMATH
      func = "InverseSgiMath";
      inv = InverseSgiMath();
#else
      break;
#endif // HAS_SGIMATH
    }
    float max = (inv-corr).MaximumAbsolute();
    const float lim = 1e-4;
    bool err  = max>lim;
    if (err)
      ok = false;

    if (verbose>2 || (err && verbose<-2)) {
      if (!shown) {
	cout << "TestInverse experiment #" << num
	     << ": Calculating inverse of" << endl;
	Dump(Simple::DumpLong);
	cout << endl;
	shown = true;
      }
      cout << "Result from " << func << "() is " << endl;
      inv.Dump(Simple::DumpLong);
      cout << endl;
      if (err && !corr_shown) {
	cout << "Correct result would be" << endl;
	corr.Dump(Simple::DumpLong);
	cout << endl;
	corr_shown = true;
      }
      cout << endl << "Maximum absolute component difference = " << max
	   << endl << endl;
    }
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool MatrixOf<Type>::BlasTestMahalanobis(int v) {
  if (v)
    cout << "FloatMatrix::BlasTestMahalanobis() starting." << endl;

  bool ok = true;

  Type d[] = { 1,1, 2,1, 2,2, 3,2, 4,2, 4,3, 5,3 }; 
  VectorSetOf<Type> set(2);
  set.Split(VectorOf<Type>(14, d));
  // set.Dump(DumpLong);

  set.Metric(VectorOf<Type>::MakeDistance("mahalanobis"));

  if (!set.SetCovInvInMetric()) {
    ShowError("Matrix::BlasTestMahalanobis() : SetCovInvInMetric() 1 failed");
    ok = false;
  }

  Type d33[] = { 3,3 }, d34[] = { 3,4 }, d43[] = { 4,3 }, d44[] = { 4,4 };
  VectorOf<Type> v33(2, d33), v34(2, d34), v43(2, d43), v44(2, d44);
    
  if (ok) {
    ok &= TestMahalanobis(set, v33, v34, 5.25, 1, v);
    ok &= TestMahalanobis(set, v33, v43, 2   , 2, v);
    ok &= TestMahalanobis(set, v33, v44, 1.25, 3, v);
  }

  set.DeleteMetric();
  set.Metric(VectorOf<Type>::MakeDistance("mahalanobis-bias"));
  if (!set.SetCovInvInMetric()) {
    ShowError("Matrix::BlasTestMahalanobis() : SetCovInvInMetric() 2 failed");
    ok = false;
  }

  if (ok) {
    ok &= TestMahalanobis(set, v33, v34, 7      , 4, v);
    ok &= TestMahalanobis(set, v33, v43, 2+1.0/3, 5, v);
    ok &= TestMahalanobis(set, v33, v44, 2+1.0/3, 6, v);
  }

  if (v>1 || (v<-1 && !ok))
    cout << " FloatMatrix::BlasTestMahalanobis() ended "
	 << (ok?"SUCCESSFULLY":"in FAILURE") << endl;

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool MatrixOf<Type>::TestMahalanobis(const VectorSetOf<Type>& set,
				     const VectorOf<Type>& v1,
				     const VectorOf<Type>& v2,
				     double corr, int num, int verbose) {
  double d12 = v1.DistanceSquaredMahalanobis(v2, set.Metric()->GetMatrix());
  double d21 = v2.DistanceSquaredMahalanobis(v1, set.Metric()->GetMatrix());
					      
  const double lim = 1e-4;
  bool err = fabs(d12-d21)>lim || fabs(d12-corr)>lim || fabs(d21-corr)>lim;
  if (verbose>2 || (err && verbose<-2)) {
    cout << "TestMahalanobis experiment #" << num << endl;
    v1.Dump(Simple::DumpLong);
    cout << endl;
    v2.Dump(Simple::DumpLong);
    cout << endl;

    cout << "d(a,b) = " << d12 << "  d(b,a) = " << d21
	 << "  correct = " << corr << endl;
  }
  return !err;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool MatrixOf<Type>::BlasTestMatrixVectorProduct(int verbose) {
  if (verbose)
    cout << "FloatMatrix::BlasTestMatrixVectorProduct() starting." << endl;

  int d = 80000, k = 1000;
  FloatVector v(d);
//  FloatMatrix m(k, d);
//
//   m.PreMultiply(v);
//   if (verbose) {
//     cout << "finished performing matrix(" << k << "x" << d << ") vector("
// 	 << d << ") multiplication" << endl;
//   }


  FloatMatrix n(d, k);
  n.PreTransMultiply(v);
  if (verbose) {
    cout << " finished performing matrix(" << k << "x" << d << ") vector("
	 << d << ") (transposed) multiplication" << endl;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool MatrixOf<Type>::BlasTestDotProduct(int verbose) {
  if (verbose)
    cout << "FloatMatrix::BlasTestDotProduct() starting." << endl;

  int d = 10;
  FloatVector a(d), b(d);

  for (int i=0; i<d; i++) {
    a[i] = i+1;
    b[i] = i-d/2;
  }

  double p = a.DotProduct(b), corr = 55.0;

  if (verbose)
    cout << " finished performing vector(" << d << ") dot product with result "
	 << p << " (correct==" << corr << ")" << endl;

  return p==corr;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool MatrixOf<Type>::BlasTestEigenVectors(int verbose) {
  cout << "FloatMatrix::BlasTestEigenVectors(" << verbose << ")" << endl;

  int N=3;

  float aval[]={1,0,1,0,1,0,1,0,1};

  FloatMatrix A(N,N,aval);
  FloatVector eval;

  cout << "input matrix " << endl;
  for(int i=0;i<N;i++){
    for(int j=0;j<N;j++)
      cout << A.FastGet(i,j) << " ";
    cout << endl;
  }

  FloatMatrix evec=A.EigenVectors(&eval);

  cout << "eigenvalues: " << endl;

  for(int i=0;i<N;i++)
    cout << eval.Get(i) << " ";
  cout << endl;

  cout << "eigenvectors: " << endl;
  for (int i=0; i<N; i++) {
    for(int j=0; j<N; j++)
      cout << evec.FastGet(i,j) << " ";
    cout << endl;
  }
  cout << endl;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

#if defined(sgi)
#pragma do_not_instantiate bool MatrixOf<float>::BlasTest(int)
#endif // sgi

#if defined(__alpha)
#pragma do_not_instantiate MatrixOf<float>::BlasTest
#endif //  __alpha

template <>
bool MatrixOf<float>::BlasTest(int verbose) {
  if (verbose)
    cout << "FloatMatrix::BlasTest() starting. HasLinpack()="
	 << Simple::HasLinpack() << " UseLinpack()="
	 << Simple::UseLinpack() << endl
	 << " fundamental object sizes: short=" << sizeof(short)
	 << " int=" << sizeof(int) << " long=" << sizeof(long)
	 << " float=" << sizeof(float) << " double=" << sizeof(double)
	 << endl;

  bool ok = true;

  ok &= BlasTestDotProduct(verbose);
  ok &= BlasTestMatrixVectorProduct(verbose);
  ok &= BlasTestInverse(verbose);
  ok &= BlasTestMahalanobis(verbose);

  // atlas error in ilaenv_()!
  // ok &= BlasTestEigenVectors(verbose);

  if (verbose>0 || (verbose<0 && !ok))
    cout << "FloatMatrix::BlasTest() ended "
	 << (ok?"SUCCESSFULLY":"in FAILURE") << endl;

  return ok;
}

template <class Type>
bool MatrixOf<Type>::BlasTest(int) {
  FunctionNotImplemented("MatrixOf<Type>::BlasTest(bool)");
  return false;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void MatrixOf<Type>::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void MatrixOf<Type>::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// FloatVector matrix_inverse_multiply(const float *mat, int d, int n,
// 				    const float *vec, int l) {

//   FloatMatrix m(d, n, mat), invm = m.Inverse();
//   FloatVector v(l, vec);
//   return invm.PreMultiply(v);
// }

///////////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _MATRIX_C_
