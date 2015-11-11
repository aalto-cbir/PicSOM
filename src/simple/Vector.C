// -*- C++ -*-  $Id: Vector.C,v 1.24 2014/02/03 09:22:15 jorma Exp $
// 
// Copyright 1994-2013 Jorma Laaksonen <jorma.laaksonen@aalto.fi>
// Copyright 1998-2013 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _VECTOR_C_
#define _VECTOR_C_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <Vector.h>
#include <Matrix.h>
#include <Histogram.h>

#ifdef sgi
#if !_SGIAPI
extern "C" int snprintf(char *, size_t, const char *, ...); 
#endif // _SGIAPI
#endif // sgi

namespace simple {

class Histogram;

// extern FloatVector matrix_inverse_multiply(const float*, int, int,
// 					   const float*, int);

///////////////////////////////////////////////////////////////////////////////

template <class Type> int VectorOf<Type>::dist_comp_ival = 8;
template <class Type> int VectorOf<Type>::dump_length_limit = 100;
//template <class Type> size_t VectorOf<Type>::n_created = 0;
//template <class Type> size_t VectorOf<Type>::n_destructed = 0;

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>::VectorOf(int len, const Type *v,
				  const char *lab, bool nodata) {
  Initialize();
  length = len;
  if (!nodata)
    Allocate(v);
  Label(lab);
  // n_created++;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::Initialize() {
  length = 0;
  vector = NULL;
  label  = NULL;
  own    = FALSE;
  number = -1;
  classification = NULL;
  CancelSelfDot();
  ios_width = 0;
  label_index = -1;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>::~VectorOf() {
  DeleteVectorIfOwn();
  delete [] label;
  delete classification;
  // n_destructed++;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::DeleteVectorIfOwn() {
  if (own)
    delete [] vector;
  vector = NULL;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::CopyValue(const VectorOf<Type>& v) {
  if (length!=v.length) {
    DeleteVectorIfOwn();

    length = v.length;
    if (v.vector)
      Allocate(v.vector);
    else
      vector = NULL;

  } else
    if (v.vector)
      CopyValues(v.vector, v.Length());
    else {
      delete [] vector;  // [] added 25.9.2000
      vector = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::operator=(const VectorOf<Type>& v) {
  CopyValue(v);

  self_dot = v.self_dot;

  Label(v.label);
  Number(v.number);
  Classification(v.classification);
  LabelIndex(v.label_index);

//   if (*this!=v) {
//     cerr << "Vector::operator= failed" << endl;
//     cerr << "source: "; v.Dump(DumpRecursiveLong);
//     cerr << "destination: "; Dump(DumpRecursiveLong);
//     if (*this!=v)
//       Abort();
//   }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::operator+=(const VectorOf<Type>& v) {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator+="))
    return *this;
  
  const int l = Length();
  for (register int i=0; i<l; i++)
    vector[i] += v.vector[i];

  CancelSelfDot();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::AddWithMultiply(const VectorOf<Type>& v,
						double m) {
  if (!CheckDisparity(v, TRUE, "VectorOf::AddWithMultipply"))
    return *this;
  
  const int l = Length();
  for (register int i=0; i<l; i++)
    vector[i] += (Type)(m*v.vector[i]);

  CancelSelfDot();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::operator-=(const VectorOf<Type>& v) {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator-="))
    return *this;
  
  register Type *p1 = vector;
  register Type *p2 = v.vector;
  register int i = length;

  while (i--) {
    *p1 -= *p2;
    p1++;
    p2++;
  }

  CancelSelfDot();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::operator*=(const VectorOf<Type>& v) {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator*="))
    return *this;
  
  for (register int i=0; i<Length(); i++)
    vector[i] *= v.vector[i];

  CancelSelfDot();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::operator/=(const VectorOf<Type>& v) {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator/="))
    return *this;
  
  for (register int i=0; i<Length(); i++)
    vector[i] /= v.vector[i];

  CancelSelfDot();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator+(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator+"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = vector[i]+v[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator-(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator-"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = vector[i]-v[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator*(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator*"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = vector[i]*v[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator/(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator/"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = vector[i]/v[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator*(double m) const {
  VectorOf<Type> res(*this);
  res.Multiply(m);

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator/(double m) const {
  VectorOf<Type> res(*this);
  res.Multiply(1.0/m);

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::operator=(const Type* v) {
  DeleteVectorIfOwn();

  Allocate(v);
  
  CancelSelfDot();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::operator==(const VectorOf<Type>& v) const {
  if (!ContentEqual(v))
    return FALSE;

  if (/*own!=v.own ||*/ !LabelsMatch(v) || number!=v.number)
    return FALSE;

  // nothing is done with 'classification'

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorOf<Type>::ContentEqual(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator=="))
    return false;

  for (int i=0; i<length; i++)
    if (vector[i]!=v.vector[i])
      return false;
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator~() const {
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = ~(int)vector[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernaryNOT() const {
  char *newlabel = label ? new char[strlen(label)+2] : NULL;
  if (newlabel)
    sprintf(newlabel, "~%s", label);

  VectorOf<Type> res(Length(), NULL, newlabel);
  delete newlabel;

  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = -(int)vector[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator|(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator|"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = int(vector[i])|int(v[i]);

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernaryOR(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::TernaryOR"))
    return *this;

  char *newlabel = label&&v.Label() ?
    new char[strlen(label)+strlen(v.Label())+2] : NULL;
  if (newlabel)
    sprintf(newlabel, "%s|%s", label, v.Label());
  
  VectorOf<Type> res(Length(), NULL, newlabel);
  delete newlabel;

  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = vector[i]>v[i] ? vector[i] : v[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator&(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator&"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);;
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = int(vector[i])&int(v[i]);

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernaryAND(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::TernaryAND"))
    return *this;
  
  char *newlabel = label&&v.Label() ?
    new char[strlen(label)+strlen(v.Label())+2] : NULL;
  if (newlabel)
    sprintf(newlabel, "%s&%s", label, v.Label());

  VectorOf<Type> res(Length(), NULL, newlabel);
  delete newlabel;

  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = vector[i]<v[i] ? vector[i] : v[i];

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::operator^(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::operator^"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = int(vector[i])^int(v[i]);

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernaryXOR(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::TernaryXOR"))
    return *this;
  
  VectorOf<Type> a = TernarySetMinus(v), b = v.TernarySetMinus(*this);
  VectorOf<Type> ret = a.TernaryOR(b);

  if (label&&v.Label()) {
    char *newlabel = new char[strlen(label)+strlen(v.Label())+2];
    sprintf(newlabel, "%s^%s", label, v.Label());
    ret.Label(newlabel);
    delete newlabel;
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::SetMinus(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::SetMinus"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  for (int i=0; i<Length(); i++)
    res[i] = int(vector[i])&~int(v[i]);

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernarySetMinus(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::TernarySetMinus"))
    return *this;
  
  VectorOf<Type> ret = TernaryAND(v.TernaryNOT());
  if (label&&v.Label()) {
    char *newlabel = new char[strlen(label)+strlen(v.Label())+2];
    sprintf(newlabel, "%s\\%s", label, v.Label());
    ret.Label(newlabel);
    delete newlabel;
  }

  return ret;
}
///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernaryMask(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::TernaryMask"))
    return *this;
  
  VectorOf<Type> res(Length(), NULL, label);
  res.Number(Number());
  res.LabelIndex(LabelIndex());

  Type zero = Type(0);

  for (int i=0; i<Length(); i++)
    res[i] = v[i]>zero ? vector[i] : zero;

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernaryExclusive() const {
  VectorOf<Type> ret(*this);
  ret.Change(0, -1);

  if (label) {
    char *newlabel = new char[strlen(label)+2];
    sprintf(newlabel, "!%s", label);
    ret.Label(newlabel);
    delete newlabel;
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::TernaryDefined() const {
  VectorOf<Type> ret(*this);
  ret.Change(-1, 1);

  if (label) {
    char *newlabel = new char[strlen(label)+2];
    sprintf(newlabel, "%%%s", label);
    ret.Label(newlabel);
    delete newlabel;
  }

  return ret;
}

template <class Type>
void VectorOf<Type>::TernaryTruthTables(ostream& os) {
  os << "TernaryNOT : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernaryNOT, os);

  os << endl << "TernaryExclusive : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernaryExclusive, os);

  os << endl << "TernaryDefined : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernaryDefined, os);

  os << endl << "TernaryAND : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernaryAND, os);

  os << endl << "TernaryOR : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernaryOR, os);

  os << endl << "TernaryXOR : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernaryXOR, os);

  os << endl << "TernarySetMinus : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernarySetMinus, os);

  os << endl << "TernaryMask : " << endl;
  TernaryTruthTable(&VectorOf<Type>::TernaryMask, os);
  os << endl;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::TernaryTruthTable(VectorOf<Type> (VectorOf<Type>::*f)()
				       const, ostream& os) {
  VectorOf<Type> v(3);
  v[0] = (Type)-1;
  v[1] = (Type) 0;
  v[2] = (Type)+1;
  VectorOf<Type> r = (v.*f)();
  for (int i=0; i<3; i++) {
    char line[100];
    sprintf(line, "%+2d -> %+2d", int(v[i]), int(r[i]));
    os << line << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::TernaryTruthTable(VectorOf<Type>
				       (VectorOf<Type>::*f)(const 
							    VectorOf<Type>& v)
				       const, ostream& os) {
  VectorOf<Type> v(3);
  v[0] = (Type)-1;
  v[1] = (Type) 0;
  v[2] = (Type)+1;

  for (int i=-1; i<=1; i++) {
    VectorOf<Type> u(3);
    u[0] = u[1] = u[2] = (Type)i;
    VectorOf<Type> r = (u.*f)(v);
    char line[100];
    sprintf(line, "%+2d -> %+2d %+2d %+2d", i, int(r[0]),int(r[1]),int(r[2]));
    os << line << endl;
  }
}


///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&DumpShort || type&DumpLong) {
    os << Bold(TypeName()) << Bold("Vector ") << (void*)this
       << " length="         << length
       << " own="            << own
       << " label="          << ShowString(label)
       << " label_index="    << label_index
       << " number="         << number
       << " classification=" << classification
       << " self_dot="       << self_dot
       << " vector="         << (void*)vector << endl;
  }

  if (type&DumpLong && vector) {
    DisplayAll(os);
    os << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::DisplayOne(Type f, ostream& os) const {
  os << setw(ios_width);
  if (IsLargestPositive(f))
    os << "+++ ";
  else if (IsLargestNegative(f))
    os << "--- ";
  else
    os << f << " ";
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::Allocate(const Type *v, int n) {
  vector = length>0 ? new Type[length] : NULL;
  if ((!vector && length>0) || length<0) {
    cerr << "Vector::Allocate memory request failed, length=" << length
	 << endl;
    DumpMemoryUsage(cerr);
    SimpleAbort();
    //return;
  }

  if (!vector) {
    own = 0;
    return;
  }

  // static int i;
  // cout << "Vector::Allocate() " << i++ << endl;

  own = 1;
  if (v)
    CopyValues(v, n<length ? n : length);
  else
    Zero();

  CancelSelfDot();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Lengthen(int l) {
  if (l!=length) {
    Type *oldvector = vector, zero = 0;
    int   oldlength = length;
    int   oldown    = own;
    length = l>0 ? l : 0;

    Allocate(oldvector, oldlength);

    for (int i=oldlength; i<length; i++)
      vector[i] = zero;

    if (oldown)
      delete [] oldvector;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Append(const VectorOf<Type>& vec) {
  int oldlength = length;
  Lengthen(length+vec.Length());

  for (int i=0; i<vec.Length(); i++)
    vector[oldlength+i] = vec[i];

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Prepend(const VectorOf<Type>& vec) {
  int oldlength = length;
  Lengthen(length+vec.Length());

  for (int i=oldlength-1; i>=0; i--)
    vector[vec.Length()+i] = vector[i];

  for (int j=0; j<vec.Length(); j++)
    vector[j] = vec[j];

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Prepend(Type v) {
  Lengthen(length+1);

  for (int i=Length()-2; i>=0; i--)
    vector[i+1] = vector[i];

  vector[0] = v;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Remove(int i) {
  if (IndexOK(i)) {
    for (; i<length-1; i++)
      (*this)[i] = (*this)[i+1];

    Lengthen(length-1);
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Insert(int i, Type v, int keep) {
  if (i>=0) {
    if (keep==-1) {
      Lengthen(i>length ? i+1 : length+1);
      keep = length-1;
    }

    memmove(vector+i+1, vector+i, (keep-i)*sizeof(Type));

//     for (int k=keep; k>i; k--)
//       vector[k] = vector[k-1];

    vector[i] = v;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Components(const IntVector& c) {
  Type *new_vector = new Type[c.Length()];
  bool ok = true;
  for (int i=0; ok && i<c.Length(); i++)
    if (IndexOK(c[i]))
      new_vector[i] = FastGet(c[i]);
    else
      ok = false;

  if (ok) {
    length = c.Length();
    delete vector;
    vector = new_vector;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::SlowCopyValues(register const Type *v, register int n) {
  register Type *ptr = vector;
  while (n--)
    *ptr++ = *v++;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::SlowZero() {
  if (!length || !vector)
    return;

  /*register*/ int n = length;
  /*register*/ Type *ptr = vector;
  Type zero = 0;

  while (n--)
    *ptr++ = zero;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::IsZero() const {
  register int n = length;
  register Type *ptr = vector;

  while (n--)
    if (*ptr++)
      return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::One() {
  for (int i=0; i<length; i++)
    vector[i] = 1;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// VectorOf<Type>& VectorOf<Type>::Index() {
//   for (int i=0; i<length; i++)
//     vector[i] = i;

//   return *this;
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::AnnounceDisparity(const VectorOf<Type>& v,
				       const char *msg) const {
  cerr << "this:  ";
  Dump(length<=dump_length_limit ? DumpRecursiveLong : DumpRecursiveShort,
       cerr);

  cerr << "other: ";
  v.Dump(v.Length()<=dump_length_limit?DumpRecursiveLong:DumpRecursiveShort,
	 cerr);

  ShowError("Vector::Disparity", msg?" in ":NULL, msg);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef HAS_LINPACK
extern "C" float sdot_(const int&, const float*, const int&,
		       const float*, const int&);
#else
static double sdot_(const int&, const float*, const int&,
		   const float*, const int&) {
  Simple::ShowError("sdot not implemented."); return -1; }
#endif // HAS_LINPACK

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate VectorOf<float>::DotProduct
#endif // sgi // __alpha

template <>
float VectorOf<float>::DotProduct(const VectorOf<float>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::DotProduct"))
    return 0;

  int one = 1;

  if (UseLinpack() && IsFloatVector())
    return sdot_(length, vector, one, v.vector, one);

  register float sum = 0, *ptr1 = vector, *ptr2=v.vector;
  register int i = length;
  while (i--)
    sum += *(ptr1++) * *(ptr2++);

  return sum;
}

template <class Type>
Type VectorOf<Type>::DotProduct(const VectorOf<Type>&) const {
  FunctionNotImplemented("VectorOf<Type>::DotProduct"); return (Type)0;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// int VectorOf<Type>::MaximumDotProduct(const ListOf< VectorOf<Type> >& vl,
// 				      Type *ret, int absval) const {
//   int j = -1;
//   Type max = absval ? Type(0) : LargestNegative();

//   for (int i=0; i<vl.Nitems(); i++) {
//     Type  prod = DotProduct(vl[i]);
//     Type cprod = absval && prod<0 ? -prod : prod;

//     if (cprod>=max) {
//       max = cprod;
//       j = i;
//       if (ret)
// 	*ret = prod;
//     }
//   }

//   return j;
// }

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// int VectorOf<Type>::MinimumDistance(const ListOf< VectorOf<Type> >& vl,
// 				    float *ret) const {
//   int j = -1;
//   float min = MAXFLOAT;

//   for (int i=0; i<vl.Nitems(); i++) {
//     float dist = DistanceSquared(vl[i]);

//     if (dist<min) {
//       min = dist;
//       j = i;
//     }
//   }

//   if (ret)
//     *ret = j!=-1 ? sqrt(min) : MAXFLOAT;

//   return j;
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorOf<Type>::Max(int *loc, int finite_only, int absolute) const {
  if (!vector || length==0) {
    if (loc)
      *loc = -1;
    return 0;
  }

  if (loc)
    *loc = 0;

  Type rv = LargestNegative(), max = 0;
  for (int i=0; i<length; i++) {
    Type v = absolute && vector[i]<Type(0) ? -vector[i] : vector[i];
    if (v>rv && (!finite_only || v!=LargestPositive())) {
      rv  = v;
      max = vector[i];
      if (loc)
	*loc = i;
    }
  }

  return max;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorOf<Type>::Min(int *loc, int finite_only, int absolute) const {
  if (!vector || length==0) {
    if (loc)
      *loc = -1;
    return 0;
  }

  if (loc)
    *loc = 0;

  Type rv = LargestPositive(), min = 0;
  for (int i=0; i<length; i++) {
    Type v = absolute && vector[i]<Type(0) ? -vector[i] : vector[i];
    if (v<rv && (!finite_only || v!=LargestNegative())) {
      rv  = v;
      min = vector[i];
      if (loc)
	*loc = i;
    }
  }

  return min;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorOf<Type>::Sum() const {
  Type sum = 0;
  for (int i=0; i<length; i++)
    sum += vector[i];

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Add(Type v) {
  for (int i=0; i<length; i++)
    vector[i] += v;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Multiply(register double v) {
//  register Type vv = (Type)v;
  register Type *ptr = vector;
  register int i = length;

  while (i--) {
//    *ptr *= vv;
//     *ptr *= v; 
    *ptr = Type(*ptr*v);
    ptr++;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::NormSquared(double d) const {
  register Type *ptr = vector;
  register double sum = 0;
  register int i = length;
  if (d==2)
    while (i--) {
      /*register*/ Type tmp = *(ptr++);
      sum += SelfProduct(tmp);
    }
  else if (d==1)
    while (i--) {
      /*register*/ Type tmp = *(ptr++);
      sum += tmp<Type(0) ? -tmp : tmp;
    }
  else
    while (i--) {
      /*register*/ Type tmp = *(ptr++);
      sum += pow(fabs(double(tmp)), d);  // was float(tmp)
    }

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Normalize(double d) {
  double sum = Norm(d);

  if (sum) {
    sum = 1/sum;
    register Type *ptr = vector;
    register int i = length;
    while (i--) {
      *ptr = Type(*ptr*sum);
      ptr++;
    }
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Randomize() {
  for (int i=0; i<length; i++)
    vector[i] = Type((rand()%32767-16383.0)/2);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::FirstEqual(Type t) const {
  for (int i=0; i<length; i++)
    if (Value(i)==t)
      return i;

  return -1;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::NumberOfEqual(Type t) const {
  int n = 0;
  for (int i=0; i<length; i++)
    if (Value(i)==t)
      n++;

  return n;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::NumberOfEqual(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::NumberOfEqual"))
    return 0;

  int n = 0;
  for (int i=0; i<length; i++)
    if (Value(i)==v.Value(i))
      n++;

  return n;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::Equals(Type v) const {
  VectorOf<Type> ret(length);

  // int n = 0;
  for (int i=0; i<length; i++)
    if (Value(i)==v)
      ret[i] = Type(1);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorOf<Type>::AreUnique() const {
  for (int i=0; i<length; i++)
    for (int j=i+1; j<length; j++)
      if (vector[i]==vector[j])
	return false;
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Sort(int dec, int *idx) {
  for (int i=1; i<Length(); i++)
    for (int j=i-1; j>=0; j--)
      if (dec ? Get(j)<Get(j+1) : Get(j)>Get(j+1)) {
	Swap(j, j+1);
	if (idx) {
	  int tmp  = idx[j+1];
	  idx[j+1] = idx[j];
	  idx[j]   = tmp;
	}
      } else
	break;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::InsertSort(int dec, Type v, int *idx, bool k) {
  int i;

  for (i=0; i<Length(); i++)
    if (dec ? Get(i)<v : Get(i)>v)
      break;

  if (idx)
    *idx = i;

  return Insert(i, v, k ? Length()-1 : -1);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::qsort_compar(const void *a, const void *b) {
  Type aa = *(Type*)a, bb = *(Type*)b;
  return aa==bb ? 0 : aa<bb ? -1 : 1;
}

template <class Type>
int VectorOf<Type>::qsort_compar_minus(const void *a, const void *b) {
  return -qsort_compar(a, b);
}

template <class Type>
VectorOf<Type>& VectorOf<Type>::QuickSort(bool dec) {
  // #ifdef __alpha
  // int (*xxx)(const void *, const void *) = dec?qsort_compar_minus:qsort_compar;
  // qsort(vector, length, sizeof(Type), (Clinkfptype3)xxx);
  // #else
  qsort(vector, length, sizeof(Type), (CFP_qsort)(dec?qsort_compar_minus:qsort_compar));
  // #endif // __alpha

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// VectorOf<Type>& VectorOf<Type>::Order(const int *idx) {
// //   if (idx.Length()!=Length()) {
// //     cerr << "Vector::Order length mismatch" << endl;
// //     return *this;
// //   }

//   VectorOf<Type> copy(*this);
//   for (int i=0; i<Length(); i++)
//     Set(i, copy[idx[i]]);

//   return *this;
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::Difference() const {
  VectorOf<Type> ret(Length()-1);

  for (int i=0; i<Length()-1; i++)
    ret.Set(i, Get(i+1)-Get(i));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::SetDifference() {
  if (!length)
    ShowError("VectorOf<Type>::SetDifference() length==0");

  else {
    for (int i=0; i<length-1; i++)
      vector[i] -= vector[i+1];

    length--;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorOf<Type>::Cumulate() const {
  VectorOf<Type> sum(length);

  for (int i=0; i<length; i++)
    sum[i] = sum.Get(i-1)+vector[i];

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::MoveTowards(const VectorOf<Type>& v,
					    double a) {
  register Type *src = v.vector, *dst = vector;
  register int i = length;
  while (i--) {
//    *dst += Type(a*(*src-*dst));
//    *dst += (*src-*dst)*a;
    *dst = Type(*dst+(*src-*dst)*a);
    dst++;
    src++;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorOf<Type>::Mean() const {
  Type sum = 0;
  for (int i=0; i<length; i++)
    sum += vector[i];

  return sum/Type(Length());
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorOf<Type>::Variance() const {
  Type sum = 0, sqr = 0;
  for (int i=0; i<length; i++) {
    sum += vector[i];
    sqr += vector[i]*vector[i];
  }
  Type q = Length()>1 ? Type((sqr-sum*sum/Type(Length()))/Type(Length()-1))
    : (Type)0;

  return double(q)>0 ? q : (Type)0;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorOf<Type>::Skewness() const {
  Type m = Mean(), d = StandardDeviation();
  Type sss = 0;
  for (int i=0; i<length; i++) {
    Type a = vector[i]-m;
    sss += a*a*a;
  }

  return sss/Type(Length())/(d*d*d);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorOf<Type>::Kurtosis() const {
  Type m = Mean(), v = Variance();
  Type ssss = 0;
  for (int i=0; i<length; i++) {
    Type a = vector[i]-m;
    ssss += a*a*a*a;
  }

  return (Type)(double(ssss)/Length()/(double(v)*double(v))-3);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const VectorOf<Type>& VectorOf<Type>::MeanVariance(Type& m, Type& v) const {
  Type sum = 0, sqr = 0;
  for (int i=0; i<length; i++) {
    sum += vector[i];
    sqr += vector[i]*vector[i];
  }
  
  m = Length() ? sum/Type(Length()) : (Type)0;
  Type q = Length()>1 ? Type(sqr-sum*m)/Type(Length()-1) : (Type)0;
  v = double(q)>0 ? q : (Type)0;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::RemoveMean() {
  Type sum = 0;
  for (int i=0; i<length; i++)
    sum += vector[i];

  if (sum!=Type(0)) {
    sum /= length;
    for (int i=0; i<length; i++)
      vector[i] -= (Type)sum;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::Classification(const VectorOf<Type> *cl) {
  delete classification;

  if (cl) {
    cerr << "Vector::Classification is obsoleted." << endl;
    // classification = new VectorOf(*cl);
  } else
    classification = NULL;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::ValueFromString(const char *ptr) {
  Lengthen(0);
  for (; ptr;) {
    int l, m, n;
    if (sscanf(ptr, "%d-%d%n", &l, &m, &n)==2) {
      while (l<=m)
	Push(l++);
    } else if (sscanf(ptr, "%d%n", &l, &n)==1)
      Push(l);
    else
      break;

    ptr += n;
    if (*ptr==',')
      ptr++;
  }
  return *this; 
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorOf<Type>::ReadFromString(const char *str, bool sparse) {
  bool delim_found = !sparse;
  char tmps[10240];

  for (register int j=0; j<Length()+sparse; j++) {
    *tmps = 0;
    for (;;) {
      size_t nn = strcspn(str, " \t\n");
      register char *ptr = tmps+strlen(tmps);
      strncpy(ptr, str, nn);
      ptr[nn] = 0;
      str += nn + strspn(str+nn, " \t\n");

      if (*tmps!='[' || tmps[strlen(tmps)-1]==']')
	break;
      strcat(tmps, " ");
    }

    if (!strlen(tmps)) {
      if (j) {
	char expl[1000];
	sprintf(expl, "sparse=%d j=%d length=%d label=%s",
		sparse, j, length, label);
	ShowError("Vector::Read() failed between components ", expl);
      }
      return false;
    }

    if (sparse && tmps[0]==':' && !tmps[1]) {
      delim_found = true;
      break;
    }

    if (vector) { // vector==NULL if VectorSource::nodataread==true
      if (!sparse)
	Set(j, From(tmps));
      
      else {
	double val = 1;
	char *col = strchr(tmps, ':');
	if (col) {
	  val = atof(col+1);
	  *col = 0;
	}
	Set(atoi(tmps), (Type)val);
      }
    }
  }

  if (!delim_found)
    ShowError("Vector::Read() did not find : deliminator");

  str += strspn(str, " \t\n");
  strcpy(tmps, str);
  char *ptr = strpbrk(tmps, " \t\n\r");
  if (ptr)
    *ptr = 0;

  Label(*tmps ? tmps : NULL);
  LabelIndex(-1);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::Read(istream& is, bool sparse) {
  ShowError("Vector::Read() should be obsoleted");
  // The real thing is now in ReadFromString().
  
  char tmps[10240];
  int tmpc;

  for (;;)
    if ((tmpc = is.get())=='#')
      is.ignore(MAXINT, '\n');
    else if (tmpc!='\n') {
      is.putback((char)tmpc);
      break;
    }
  
  bool delim_found = !sparse;

  for (register int j=0; j<Length()+sparse; j++) {
    *tmps = 0;
    for (;;) {
      register char *ptr = tmps+strlen(tmps);
      is >> ptr;
      if (*tmps!='[' || tmps[strlen(tmps)-1]==']')
	break;
      strcat(tmps, " ");
    }

    if (!strlen(tmps)) {
      if (j) {
	char expl[1000];
	sprintf(expl, "sparse=%d j=%d length=%d label=%s",
		sparse, j, length, label);
	ShowError("Vector::Read() failed between components ", expl);
      }
      return FALSE;
    }

    if (sparse && tmps[0]==':' && !tmps[1]) {
      delim_found = true;
      break;
    }

    if (vector) { // vector==NULL if VectorSource::nodataread==true
      if (!sparse)
	Set(j, From(tmps));
      
      else {
	double val = 1;
	char *col = strchr(tmps, ':');
	if (col) {
	  val = atof(col+1);
	  *col = 0;
	}
	Set(atoi(tmps), (Type)val);
      }
    }
  }

  if (!delim_found)
    ShowError("Vector::Read() did not find : deliminator");

  tmpc = is.get();

  while (tmpc==' ' || tmpc=='\t')
    tmpc = is.get();

  is.putback((char)tmpc);
  is.getline(tmps, sizeof tmps);

  register char *ptr = tmps;
  while (*ptr)
    if (*ptr==' ' || *ptr=='\t') 
      *ptr = 0;
    else
      ptr++;

  Label(*tmps ? tmps : NULL);
  LabelIndex(-1);

  return is.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::ReadBinary(istream& is) {
  // ShowError("Vector::ReadBinary() should be obsoleted");
  // The real thing is now in ReadFromString().

  char line[1024];
  
  is.read((char*)vector, length*(int)sizeof(Type));

  for (unsigned int i=0; is&&i<sizeof(line); i++) {
    line[i] = 0;
    is.get(line[i]);
    if (!line[i])
      break;
  }

  Label(*line ? line : NULL);
  LabelIndex(-1);

  return is.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const char *VectorOf<Type>::WriteToString(bool sparse, int prec) const {
  const int linesize = 40000000;
  // cout << "--- about to call new()" << endl;
  char *line = new char[linesize];
  // cout << "--- returned from new()" << endl;
  if (!line) {
    ShowError("Vector::WriteToString() failed to malloc");
    SimpleAbort();
  }

  *line = 0;
  int len = 0;
  char tmp[1000];

  for (int j=0; j<Length(); j++)
    if (!sparse || Get(j)) {
      bool nzero = sparse&&double(Get(j))!=0, wcol = nzero&&double(Get(j))!=1;
      int ll = nzero ? sprintf(tmp, "%d%s", j, wcol?":":"") : 0;
      if (!sparse || wcol) {
	int l = ToString(j, tmp+ll, (int)sizeof(tmp)-ll, prec);
	if (!l) {
	  ShowError("Vector::WriteToString() : ToString() failed");
	  return NULL;
	}
	ll += l;
      }
      if (!ll)
	continue;

      if (len+ll+2>=linesize) {
	sprintf(tmp, "length=%d j=%d len=%d ll=%d linesize=%d ",
		Length(), j, len, ll, linesize);
	ShowError("Vector::WriteToString() : line too long (1) ", tmp, line);
	return NULL;
      }

      strcpy(line+len, tmp);
      line[len+ll]   = ' ';
      line[len+ll+1] = 0;
      len += ll+1;
    }

  int ll = snprintf(tmp, sizeof tmp, "%s%s",
		    sparse?Label()?": ":":":"", Label()?Label():"");

  if (len+ll+1>=linesize) {
    ShowError("Vector::WriteToString() : line too long (2)");
    return NULL;
  }

  strcpy(line+len, tmp);
  line[len+ll] = 0;

  return line;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::Write(ostream& os, bool sparse) const {
  ShowError("Vector::Write() should be obsoleted");
  // The real thing is now in WriteToString().

  if (!sparse)
    for (int j=0; j<Length(); j++)
      os << Get(j) << " ";

  else
    for (int j=0; j<Length(); j++)
      if (Get(j)) {
	os << j;
	if (double(Get(j))!=1)
	  os << ":" << Get(j);
	os << " ";
      }

  if (sparse)
    os << ":";

  if (Label())
    os << (sparse?" ":"") << Label();

  os << endl;

  return os.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::WriteBinary(ostream& os) const {
  // ShowError("Vector::WriteBinary() should be obsoleted");
  // The real thing is now in WriteToString().

  os.write((char*)vector, length*(int)sizeof(Type));

  if (Label())
    os.write(Label(), (int)strlen(Label()));
  
  os.put((char)0);

  return os.good();
}

  ///////////////////////////////////////////////////////////////////////////////
  
  template <class Type>
  typename VectorOf<Type>::DistanceType*
  VectorOf<Type>::MakeDistance(const std::string& s) {
    if (s=="euclidean")
      return new DistanceType(DistanceType::euclidean);

    if (s=="weighted_euclidean")
      return new DistanceType(DistanceType::weighted_euclidean);

    if (s=="maximum")
      return new DistanceType(DistanceType::maximum);
    
    if (s=="mahalanobis")
      return new DistanceType(DistanceType::mahalanobis);

    if (s=="mahalanobis-bias")
      return new DistanceType(DistanceType::mahalanobis_bias);

    if (s=="dotdistance")
      return new DistanceType(DistanceType::dotdistance);

    if (s=="kullback")
      return new DistanceType(DistanceType::kullback);

    if (s=="kullback-inv")
      return new DistanceType(DistanceType::kullback_inv);

    if (s=="kullback-sym")
      return new DistanceType(DistanceType::kullback_sym);

    if (s=="chisquare")
      return new DistanceType(DistanceType::chisquare);

    if (s=="histogram_intersection")
      return new DistanceType(DistanceType::histogram_intersection);

    if (std::string(s, 0, 3)=="ell")
      return new DistanceType(DistanceType::ell,
			      atof(std::string(s, 3).c_str()));

    ShowError("Vector::MakeDistance(", s.c_str(), ") failed");
    return NULL;
  }

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// double VectorOf<Type>::DistanceSquaredIfNearer(const VectorOf<Type>& v,
// 					       double ref,
// 					       const /*VectorOf<float>*/ float *metr) const {
//   if (!CheckDisparity(v, TRUE, "VectorOf::DistanceSquared") 
//       /*|| metr && ( metr->Length()!=Length() ||
// 		   metr->CheckDisparity(*metr, TRUE,
// 					"VectorOf::DistanceSquared")) */)
//     return 0;

//   register Type *ptr1 = vector, *ptr2 = v.vector;
//   register double sum = 0;
//   register int i = length;
//   if (!metr)
//     while (i--) {
//       sum += DistanceSquared(*ptr1++, *ptr2++);
//       if (sum>ref)
// 	return MAXFLOAT;
//     }
//   else {
//     register const float *mptr = /* * */ metr;
//     while (i--) {
//       sum += DistanceSquared(*ptr1++, *ptr2++)**mptr++;
//       if (sum>ref)
// 	return MAXFLOAT;
//     }
//   }

//   return sum;
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DotProductDistanceSquared(const VectorOf<Type>& v)const{
  // cout << "VectorOf::DotProductDistanceSquared" << endl;

  if (!CheckDisparity(v, TRUE, "VectorOf::DotProductDistanceSquared"))
    return 0;

  if (!HasSelfDot() || !v.HasSelfDot()) {
    ShowError("Vector::DotProductDistanceSquared no selfdot");
    Dump(DumpLong);
    v.Dump(DumpLong);
    SimpleAbort();
    return 0;
  }

  return SelfDot() + v.SelfDot() - 2*DotProduct(v);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::CosineDistanceSquared(const VectorOf<Type>& v)const{
  // cout << "VectorOf::CosineDistanceSquared" << endl;

  if (!CheckDisparity(v, TRUE, "VectorOf::CosineDistanceSquared"))
    return 0;

  if (!HasSelfDot() || !v.HasSelfDot()) {
    ShowError("Vector::CosineDistanceSquared no selfdot");
    Dump(DumpLong);
    v.Dump(DumpLong);
    SimpleAbort();
    return 0;
  }

  double d = DotProduct(v);

  return 1-d*fabs(d)/(SelfDot()*v.SelfDot());
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceSquaredOld(const VectorOf<Type>& v, double ref,
					  const float *metr) const {
  static bool first = true;
  if (first) {
    first = false;
    ShowError("Vector::DistanceSquaredOld() called");
  }

  if (HasSelfDot() && v.HasSelfDot() && ref==MAXFLOAT && !metr)
    return DotProductDistanceSquared(v);

  if (!CheckDisparity(v, TRUE, "VectorOf::DistanceSquared"))
    return 0;

  register Type *ptr1 = vector, *ptr2 = v.vector;
  register double sum = 0;
  register int i = length, kkk = DistanceComparisonInterval(), kk, k;
  register const float *mptr = metr;

  if (ref==MAXFLOAT) {
    if (!metr)
      while (i--)
	sum += DistanceSquared(*ptr1++, *ptr2++);

    else
      while (i--)
	sum += DistanceSquared(*ptr1++, *ptr2++)**mptr++;

  } else {

    if (!metr) {
      while (i) {
	kk = (i<kkk ? i : kkk);
	k  = 0;

	if (sum>ref)
	  return MAXFLOAT;

	do {
	  sum += DistanceSquared(ptr1[k], ptr2[k]);
	  k++;
	} while (k<kk);

	i -= kk;
	ptr1 += kk;
	ptr2 += kk;
      }

    } else
      while (i--)
	if (sum>ref)
	  return MAXFLOAT;
	else
	  sum += DistanceSquared(*ptr1++, *ptr2++)**mptr++;
  }

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceSquaredEuclidean(const VectorOf<Type>& v,
						double ref, int dci,
						const FloatVector *w) const {
  if (w && w->Length() != v.Length()) {
    stringstream tmp;
    tmp << v.Length() << " != " << w->Length();
    ShowError("VectorOf<Type>::DistanceSquaredEuclidean(): bad weights! "+
              tmp.str());
  }

  if (HasSelfDot() && v.HasSelfDot() && ref==MAXFLOAT)
    return DotProductDistanceSquared(v);

  Type *ptr1 = vector, *ptr2 = v.vector;
  double sum = 0;
  int i = length, kkk = dci?dci:DistanceComparisonInterval();

  if (ref==MAXFLOAT || kkk>=length) {
    if (!w) {
      while (i--)
	sum += DistanceSquared(*ptr1++, *ptr2++);
    } else {
      float *wptr = *w;
      while (i--)
	sum += DistanceSquared(*ptr1++, *ptr2++)**(wptr++);
    }

  } else
    while (i) {
      if (sum>ref)
	return MAXFLOAT;

      int kk = (i<kkk ? i : kkk);
      i -= kk;
      if (!w) {
	while (kk--)
	  sum += DistanceSquared(*ptr1++, *ptr2++);
      } else {
	float *wptr = *w;
	while (kk--)
	  sum += DistanceSquared(*ptr1++, *ptr2++)**(wptr++);
      }
    }

  // Dump(DumpLong);  v.Dump(DumpLong);  cout << sum << endl;
  
  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceChiSquare(const VectorOf<Type>& v) const {

  Type *ptr1 = vector, *ptr2 = v.vector;
  double sum = 0.0;
  int i = length;

  while (i--) {
    double s = DistanceSquared(*ptr1, *ptr2), d = *ptr1+*ptr2;
    if (d)
      s /= d; // otherwise x[i]==y[i]==0
    ptr1++;
    ptr2++;
    sum += s;
  }
  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceHistogramIntersection(const VectorOf<Type>& v) 
  const {

  Type *ptr1 = vector, *ptr2 = v.vector;
  double nom = 0.0, den = 0.0;
  int i = length;

  while (i--) {
    nom += ((*ptr1)>(*ptr2)?(*ptr2):(*ptr1));
    den += *ptr1;
  }
  return nom/den;
}

///////////////////////////////////////////////////////////////////////////////


template <class Type>
double VectorOf<Type>::DistanceSquaredEll(const VectorOf<Type>& v, double ellv,
					  double ref, int dci) const {
  // cout << "Vector::DistanceSquaredEll()" << endl;

  Type *ptr1 = vector, *ptr2 = v.vector;
  double sum = 0;
  int i = length, kkk = dci?dci:DistanceComparisonInterval();

  if (ref==MAXFLOAT || kkk>=length)
    while (i--)
      sum += DistanceSquaredEll(*ptr1++, *ptr2++, ellv);

  else
    while (i) {
      if (sum>ref)
	return MAXFLOAT;

      int kk = (i<kkk ? i : kkk);
      i -= kk;
      while (kk--)
	sum += DistanceSquaredEll(*ptr1++, *ptr2++, ellv);
    }
  
  return pow(sum, 2.0/ellv);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceSquaredMax(const VectorOf<Type>& v,
					  double ref, int dci) const {
  // cout << "Vector::DistanceSquaredMax()" << endl;

  Type *ptr1 = vector, *ptr2 = v.vector;
  double max = 0;
  int i = length, kkk = dci?dci:DistanceComparisonInterval();

  if (ref==MAXFLOAT || kkk>=length)
    while (i--) {
      double d = DistanceSquared(*ptr1++, *ptr2++);
      if (d>max)
	max = d;
    }

  else
    while (i) {
      if (max>ref)
	return MAXFLOAT;

      int kk = (i<kkk ? i : kkk);
      i -= kk;
      while (kk--) {
	double d = DistanceSquared(*ptr1++, *ptr2++);
	if (d>max)
	  max = d;
      }
    }
  
  return max;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceSquaredMahalanobis(const VectorOf<Type>& v,
						  const MatrixOf<Type> *m,
						  double ref,
						  int /*dci*/) const {
  // cout << "Vector::DistanceSquaredMahalanobis()" << endl;

  if (!m) {
    ShowError("Vector::DistanceSquaredMahalanobis() no matrix");
    return MAXFLOAT;
  }
  if (m->Rows()!=Length() || m->Columns()!=Length()) {
    ShowError("Vector::DistanceSquaredMahalanobis() matrix size mismatch");
    return MAXFLOAT;
  }

  static bool first = true;
  if (first && ref!=MAXFLOAT) {
    first = false;
    ShowError("Vector::DistanceSquaredMahalanobis() ref!=MAXFLOAT "
	      "meaningless");
  }
  
  VectorOf<Type> d = *this-v;

  // double v0 = m->PreMultiply(d).DotProduct(d);
  // double v1 = m->PreTransMultiply(d).DotProduct(d);
  double v2 = m->GeneralizedDotProduct(d, d);
  // char tmp[100];
  // sprintf(tmp, "%f %f %f", v0, v1, v2);
  // ShowError("Vector::DistanceSquaredMahalanobis() : ", tmp);

//   static bool firstx = true;
//   if (firstx) {
//     cout << "DistanceSquaredMahalanobis()" << endl;
//     Dump(DumpLong);
//     v.Dump(DumpLong);
//     cout << v2 << endl;
//     firstx = false;
//   }

  return v2;

  /*
  if (ref==MAXFLOAT)
    return m->PreMultiply(d).DotProduct(d);

  double sum = 0;
  int i = length, j = 0, kkk = dci?dci:DistanceComparisonInterval();
  while (i) {
    if (sum>ref)
      return MAXFLOAT;

    int kk = (i<kkk ? i : kkk);
    i -= kk;
    while (kk--) {
      double tmp = d.DotProduct(*m->GetColumn(j))*d[j];
      if (tmp<0)
	ShowError("Vector::DistanceSquaredMahalanobis() tmp<0");
      sum += tmp;
      j++;
    }
  }
  
  return sum;
  */
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceSquaredKullback(const VectorOf<Type>& v,
					       int mode, double ref,
					       int dci) const {
  // cout << "Vector::DistanceSquaredKullback()" << endl;

  Type *ptr1 = vector, *ptr2 = v.vector;
  double sum = 0;
  int i = length, kkk = dci?dci:DistanceComparisonInterval();

  if (ref==MAXFLOAT || kkk>=length)
    while (i--)
      sum += DistanceKullbackTerm(*ptr1++, *ptr2++, mode);

  else
    while (i) {
      if (sum>ref)
	return MAXFLOAT;

      int kk = (i<kkk ? i : kkk);
      i -= kk;
      while (kk--)
	sum += DistanceKullbackTerm(*ptr1++, *ptr2++, mode);
    }
  
  return sum*sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::DistanceSquaredXX(const VectorOf<Type>& v, 
					 const typename VectorOf<Type>::DistanceType *metr,
					 double ref, int kkk) const {
  // cout << "Vector::DistanceSquaredXX()" << endl;

//   if (ref!=MAXFLOAT)
//     cout << "Vector::DistanceSquaredXX() ref IS SET" << endl;
  
  if (!CheckDisparity(v, TRUE, "VectorOf::DistanceSquaredXX"))
    return MAXFLOAT;

  if (!metr || metr->GetType()==DistanceType::euclidean)
    return DistanceSquaredEuclidean(v, ref, kkk);

  if (!metr || metr->GetType()==DistanceType::weighted_euclidean) {
    const FloatVector* w = metr->GetWeights();
    if (!w) {
      ShowError("No weights set with metric=weighted_euclidean!");
      return MAXFLOAT;
    }
    return DistanceSquaredEuclidean(v, ref, kkk, w);
  }

  if (metr && metr->GetType()==DistanceType::ell)
    return DistanceSquaredEll(v, metr->GetEll(), ref, kkk);

  if (metr && metr->GetType()==DistanceType::maximum)
    return DistanceSquaredMax(v, ref, kkk);

  if (metr && metr->GetType()==DistanceType::dotdistance)
    return DotProductDistanceSquared(v);

  if (metr && metr->GetType()==DistanceType::cosine)
    return CosineDistanceSquared(v);

  if (metr && (metr->GetType()==DistanceType::kullback ||
	       metr->GetType()==DistanceType::kullback_inv ||
	       metr->GetType()==DistanceType::kullback_sym))
    return DistanceSquaredKullback(v, metr->GetType()-DistanceType::kullback,
				   ref, kkk);

  if (metr && metr->GetType()==DistanceType::chisquare)
    return DistanceChiSquare(v);

  if (metr && metr->GetType()==DistanceType::histogram_intersection)
    return DistanceHistogramIntersection(v);

  char tmp[200];
  sprintf(tmp, "%d = %s", (int)metr->GetType(), metr->String().c_str());

  ShowError("Vector::DistanceSquaredXX() unhandled DistanceType ", tmp);

  return MAXFLOAT;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>::DistanceType::~DistanceType() {
  delete covinv;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::DistanceType::SetMatrix(const MatrixOf<Type> *m) {
  delete covinv;
  covinv = m;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::DistanceType::SetWeights(const FloatVector *m) {
  delete w;
  w = m;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
string VectorOf<Type>::DistanceType::String(bool lng) const {
  string ret;
  static char tmp[100];  // obs! static
  switch (type) {
  case undef:            return "undef";
  case euclidean:        return lng ? "euclidean" 	     : "euc";
  case weighted_euclidean: return lng ? "weighted_euclidean" : "weuc";
  case maximum:          return lng ? "maximum"   	     : "max";
  case mahalanobis:      return lng ? "mahalanobis"      : "mah";
  case mahalanobis_bias: return lng ? "mahalanobis-bias" : "mahB";
  case dotdistance:      return lng ? "dotdistance"      : "dot";
  case kullback:         return lng ? "kullback"         : "kl" ;
  case kullback_inv:     return lng ? "kullback-inv"     : "klI";
  case kullback_sym:     return lng ? "kullback-sym"     : "klS";
  case ell:	sprintf(tmp, "ell%f", ellval); return tmp;
  default: return "???";
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::WriteGnuPlot(ostream& os, int sparse) const {
  for (int i=0; i<Length(); i++)
    os << Get(i) << (sparse ? "\n" : "") << "\n";
  
  os << flush;

  return os.good();
}

///////////////////////////////////////////////////////////////////////////////

#ifndef __GNUC__
template <class Type>
double VectorOf<Type>::LillieforsTest(ostream& os, const char *plot) const {
  double mean = Mean();
  double stdv = StandardDeviation();

  VectorOf<Type> copy(*this);
  copy.SortIncreasingly().Sub(mean).Divide(stdv*sqrt(2.0));
  copy.SetErf().Divide(2).Add(0.5);
  VectorOf<Type> floor(*this);
  floor.SetIndices().Divide(Length());
  VectorOf<Type> ceil(floor);
  ceil.Add(1.0/Length());

  if (plot) {
    VectorOf<Type> vec(*this);
    vec.SortIncreasingly().Sub(mean).Divide(stdv);
    MatrixOf<Type> tmp;
    tmp.AppendCopy(vec);
    tmp.AppendCopy(copy);
    tmp.AppendCopy(floor*0.5+ceil*0.5);
    tmp.AppendCopy(tmp[2]-copy);
    tmp.AppendCopy(tmp[3].Abs());
    tmp.WriteGnuPlot(plot);
  }

  double maxfloor = (double)(copy-floor).Abs().Maximum();
  double maxceil  = (double)(ceil-copy).Abs().Maximum();
  double maxmax   = maxceil>maxfloor ? maxceil : maxfloor;

//   VectorOf<Type>(*this).SortIncreasingly().Dump(DumpLong);
//   VectorOf<Type>(*this).SortIncreasingly().Sub(mean).Divide(stdv).Dump(DumpLong);
//   copy.Dump(DumpLong);
//   floor.Dump(DumpLong);
//   ceil.Dump(DumpLong);
//   (copy-ceil).Abs().Dump(DumpLong);
//   (copy-floor).Abs().Dump(DumpLong);

  double s = 1/sqrt((double)Length());
  double d[4] = { 1.031*s, 0.886*s, 0.805*s, 0.735*s };
  static struct {
    double v;
    const char *t;
  } quant[] = { {1.031, "more than 99"}, {0.886, "95..99"}, {0.805, "90..95"},
		{0.768, "85..90"}, {0.736, "80..85"}, {-1, "less than 80"}};

  os << "*** LillieforsTest ***" << endl
     << "    n=" << Length() << " D=" << maxmax
     << " d[0.01]=" << d[0] << " d[0.05]=" << d[1]
     << " d[0.10]=" << d[2] << " d[0.20]=" << d[3]
     << endl << "    Normality can be rejected with ";

  for (int i=0; i<sizeof(quant)/sizeof(*quant); i++)
    if (maxmax*sqrt(double(Length()))>quant[i].v) {
      os << quant[i].t;
      break;
    }

  os << " % certainty."<< endl;

  return maxmax;
}

#else

template <class Type>
double VectorOf<Type>::LillieforsTest(ostream&, const char*) const {
  static bool first = true;
  if (first)
    ShowError("Vector::LillieforsTest not implemented.");
  first = false;
  return MAXFLOAT;
}
#endif // __GNUC__

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::ChiSquareTest(const VectorOf<float>&,
				     ostream&, const char*) const {
  ShowError("Vector::ChiSquareTest not implemented.");
  return 0;
}

#ifdef __GNUC__
template <>
double VectorOf<float>::ChiSquareTest(const VectorOf<float>&,
				     ostream&, const char *) const {
  ShowError("FloatVector::ChiSquareTest not implemented.");
  return 0;
}
#else
template <>
double VectorOf<float>::ChiSquareTest(const VectorOf<float>& inlim,
				     ostream& os, const char *plot) const {
  int n = inlim.Length()+1; 

  if (n<4) {
    ShowError("ChiSquareTest(n<4) is impossible.");
    return 0;
  }

  VectorOf<float> limits, expe;

  double mean = Mean(), stdv = StandardDeviation();

  if (inlim.IsZero())
    for (int i=1; i<n; i++)
      limits.Push(NormInv(i/float(n), mean, stdv));
  else
    limits = inlim;

  double prevcdf = 0;
  for (int j=0; j<limits.Length(); j++) {
    double cdf = NormCdf(limits[j], mean, stdv);
    expe.Push(cdf-prevcdf);
    prevcdf = cdf;
  }
  expe.Push(1-prevcdf).Multiply(Length());

  Histogram hg(*this, limits);
  hg.Normalized(FALSE);

  double sum = 0;
  for (int i=0; i<n; i++)
    sum += (hg[i]-expe[i])*(hg[i]-expe[i])/expe[i];

  if (plot) {
    char tmpplot[1000];
    sprintf(tmpplot, "%s-a", plot);
    hg.Scaled(TRUE).BarGraph(TRUE).WriteGnuPlot(tmpplot);

    sprintf(tmpplot, "%s-b", plot);
    VectorOf<float> range(200);
    range.SetRamp().Multiply(hg.LimitMax()-hg.LimitMin()).Add(hg.LimitMin());
    double mul = Length(); // *(hg.LimitMax()-hg.LimitMin())/hg.TotalBins();
    VectorOf<FloatPoint> val(range.Length());
    for (int q=0; q<val.Length(); q++)
      val[q] = FloatPoint(range[q], mul*NormPdf(range[q], mean, stdv));
    val.WriteGnuPlot(tmpplot);
  }

  double c = ChiSquareCdf(sum, n-3);

  os << "*** ChiSquareTest ***" << endl
     << "   n=" << Length() << " k=" << n << " d.o.f=" << (n-3)
     << " X=" << sum << endl << "    Normality can be rejected with "
     << (100*c) << " % certainty."<< endl;

//   hg.Dump(DumpLong);
//   expe.Dump(DumpLong);

  return c;
}

#endif // __GNUC__

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::CorrelationCoefficient(const VectorOf<Type>& v) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::CorrelationCoefficient"))
    return MAXFLOAT;

  if (Length()<2)
    return MAXFLOAT;

  Type xm, xs2, ym, ys2;
  MeanVariance(xm, xs2);
  v.MeanVariance(ym, ys2);
  
  Type xys = DotProduct(v)-(Type)(Length()*xm*ym);

  return xys/((Length()-1)*sqrt(double(xs2)*ys2));
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::CorrelationTest(const VectorOf<Type>& v,
				       ostream& os) const {

  double rho = CorrelationCoefficient(v);
  if (rho==MAXFLOAT) {
    ShowError("Vector::CorrelationTest failed.");
    return 0;
  }

  double t = fabs(rho)*sqrt((Length()-2)/(1-rho*rho));
  double p = 2*TCdf(t, Length()-2)-1;

  os << "*** CorrelationTest ***" << endl
     << "     n=" << Length() << " rho=" << rho << " t=" << t 
     << " d.o.f=" << (Length()-2) << endl
     << "     Uncorrelatedness can be rejected with " << 100*p
     << " % certainty."<< endl;

  return p;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::WriteMatlab(ostream& os, bool nl) const {
  if (!EmptyLabel())
    os << Label() << " = ";
  os << "[";

  for (int i=0; i<Length(); i++)
    os << Get(i) << (nl?"\n":" ");
  os << "];" << endl;

  return os.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::ReadMatlab(istream& is) const {
  ShowError("Vector::ReadMatlab not even started yet...");
  return is.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::TPdf(double x, int v) {
  if (v<=0) {
    ShowError("Vector::TPdf negative freedom ???");
    return 0;
  }

  double term = exp(LnGamma((v+1)/2.0)-LnGamma(v/2.0));
  return term/(sqrt(v*M_PI)*pow(1+x*x/v, (v+1)/2.0));
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::TCdf(double x1, int v) {
  if (v<=0) {
    ShowError("Vector::TCdf negative freedom ???");
    return 0;
  }

  if (v==1)
    return 0.5+atan(x1)/M_PI;

  // $MATLAB/toolbox/stats/tcdf.m

  double x2 = v/(v+x1*x1), a2 = v/2.0, b2 = 0.5;

  // $MATLAB/toolbox/matlab/specfun/betainc.m
  
  // cerr << "betainc(" << x2 << "," << a2 << "," << b2 <<")-";

  double bt = exp(LnGamma(a2+b2)-LnGamma(a2)-LnGamma(b2)
		  +a2*log(x2)+b2*log(1-x2));

  double x3 = x2<(a2+1)/(a2+b2+2) ? x2 : 1-x2;
  double a3 = x2<(a2+1)/(a2+b2+2) ? a2 : b2;
  double b3 = x2<(a2+1)/(a2+b2+2) ? b2 : a2;

  // $MATLAB/toolbox/matlab/specfun/betacore.m

  // cerr << "betacore(" << x3 << "," << a << "," << b <<")-";

  double qab = a3+b3, qap = a3+1, qam = a3-1, am = 1, bm = am, y = am;
  double bz = 1 - qab*x3/qap, yold = 0;
  int m = 1;

  while (fabs(y-yold) > 4*MatlabEps()*fabs(y)) {
    double tem = 2 * m, d  = m * (b3 - m) * x3 / ((qam + tem) * (a3 + tem));
    double ap = y + d * am, bp = bz + d * bm;
    d  = -(a3 + m) * (qab + m) * x3 / ((a3 + tem) * (qap + tem));
    double app = ap + d * y, bpp = bp + d * bz;
    yold = y;
    am = ap / bpp;
    bm = bp / bpp;
    y = app / bpp;
    if (m == 1)
      bz = 1;
    m++;
  }

  // cerr << y << endl;

  double betainc = x2<(a2+1)/(a2+b2+2) ? bt*y/a2 : 1-bt*y/b2;

  // cerr << betainc << endl;

  return x1<0 ? betainc/2 : 1-betainc/2;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::TInv(double /*x*/, int /*v*/) {
  ShowError("Vector::TInv not even started yet...");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::GammaPdfMLEFit(float&, float&) const {
  FunctionNotImplemented("VectorOf<Type>::GammaPdfMLEFit");
}

template <>
void VectorOf<float>::GammaPdfMLEFit(float& reta, float& retb) const {
  FloatVector c = Log();

  const double d = 0.0000001, da = 0.0001;
  const double y = c.Mean()-log(Mean());
  const double start = 2;

  double a = start, min = 0, max = -1; 

  do {
    if (min==0)
      a = a/2;
    else if (max==-1)
      a = 2*a;
    else
      a = (min+max)/2;

    const double g = Gamma(a-d), gd = Gamma(a+d), dgda = (gd-g)/(2*d);
    const double r = dgda/Gamma(a)-log(a);

    if (r<y)
      min = a;
    else if (r>y)
      max = a;
    else
      break;

  } while (max==-1 || max-min>da);

  reta = a;
  retb = Mean()/a;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::GammaPdfMomFit(float&, float&) const {
  FunctionNotImplemented("VectorOf<Type>::GammaPdfMomFit");
}

template <>
void VectorOf<float>::GammaPdfMomFit(float& reta, float& retb) const {
  float mean, var;
  MeanVariance(mean, var);

  reta = mean*mean/var;
  retb = var/mean;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::GammaPdfFit(const FloatVector&, float&,
				 float&, float&) const {
  FunctionNotImplemented("VectorOf<Type>::GammaPdfFit");
}

template <>
void VectorOf<float>::GammaPdfFit(const FloatVector& vec, float& reta,
				  float& retb, float& retc) const {

  if (!CheckDisparity(vec, TRUE, "VectorOf::GammaPdfFit (tail)"))
    return;
  
  FloatVector y = vec.Log(), x = Log();
  double p = Sum(), q = x.Sum(), r = x.Sqr().Sum(), s = DotProduct(x);
  double t = y.Sum(), u = y.DotProduct(x), v = DotProduct(y), w = Sqr().Sum();

//   cout << "p=" << p << endl;
//   cout << "q=" << q << endl;
//   cout << "r=" << r << endl;
//   cout << "s=" << s << endl;
//   cout << "t=" << t << endl;
//   cout << "u=" << u << endl;
//   cout << "v=" << v << endl;
//   cout << "w=" << w << endl;

  float mat[] = { (float)Length(), (float)q, (float)p, (float)q, (float)r,
		  (float)s, -(float)p, -(float)s, -(float)w },
    ve[] = { (float)t, (float)u, (float)v };

  //   FloatVector res = matrix_inverse_multiply(mat, 3, 3, ve, 3);

  MatrixOf<float> h(3, 3, mat), hinv = h.Inverse();
  FloatVector z(3, ve), res = hinv.PreMultiply(z);

  reta = exp(double(res[0]));
  retb = res[1];
  retc = res[2];
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// VectorOf<float> VectorOf<Type>::MatrixInversePreMultiply(const float *mf
// 							 int h, int w) const {
//   MatrixOf<float> mat;
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::ExpFit(const FloatVector&, float&, float&) const {
  FunctionNotImplemented("VectorOf<Type>::ExpFit");
}

template <>
void VectorOf<float>::ExpFit(const FloatVector& v,
			     float& reta, float& retb) const {

  if (!CheckDisparity(v, TRUE, "VectorOf::ExpFit"))
    return;
  
  FloatVector y = v.Log();
  double p = Sum(),       q = y.Sum();
  double r = Sqr().Sum(), s = DotProduct(y);
  double den = r*Length()-p*p;

  if (den==0) {
    ShowError("Vector::ExpFit failed.");
    reta = retb = 0;
    return;
  }

  reta = exp((q*r-p*s)/den);
  retb = (p*q-s*Length())/den;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::ReciprocFit(const FloatVector&, float&, float&) const {
  FunctionNotImplemented("VectorOf<Type>::ReciprocFit");
}

template <>
void VectorOf<float>::ReciprocFit(const FloatVector& v,
				  float& reta, float& retb) const {

  if (!CheckDisparity(v, TRUE, "VectorOf::ReciprocFit"))
    return;
  
  double y1 = v.Sum(),       y2 = v.DotProduct(v);
  double z1 = DotProduct(v), z2 = DotProduct(v.Sqr());
  double den = y1*y1-y2*Length();

  if (den==0) {
    ShowError("Vector::ReciprocFit failed.");
    reta = retb = 0;
    return;
  }

  reta = (z2*y1-z1*y2)/den;
  retb = (z1*y1-z2*Length())/den;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// double VectorOf<Type>::GammaPdfTail(Type x, double a, double b) {
//   double xx = x, m = a*b, d = m, diff = 0.00001;

// //   if (xx<m)
// //     cerr << "Vector::GammaPdfTail failing" << endl;

//   double sum = 0, g1 = 0, g2, g3, a1, a2;

//   for (;;) {
//     if (!g1)
//       g1 = GammaPdfAt(xx, a, b);
//     else 
//       g1 = g3;

//     g3 = GammaPdfAt(xx+d, a, b);
//     int divided = FALSE;

//     for (;;) {
//       g2 = GammaPdfAt(xx+d/2, a, b);

//       a1 = (g1+g3)*d/2;
//       a2 = (g1+2*g2+g3)*d/4;

//       double err = fabs(a1-a2);
//       if (err>diff) {
// 	d /= 2;
// 	diff /= 2;
// 	g3 = g2;
// 	divided = TRUE;
//       } else
// 	break;
//     }
//     sum += a2;
//     xx  += d;

//     if (a2<diff)
//       break;

//     if (!divided) {
//       d *= 2;
//       diff *= 2;
//     }
//   }

//   return sum;
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::GammaCdf(double x, double a, double b) {
  if (x<0 || a<=0 || b<=0) {
    cerr << "GammaCdf: argument failure" << endl;
    return -1;
  }

  x /= b; // $MATLAB/toolbox/matlab/specfun/gammainc.m

  if (x<a+1) {
    double ap = a, sum = 1/a, del = sum;
    while (del >= 10*MatlabEps()*sum) {
      ap++;
      del *= x/ap;
      sum += del;
    }

    return sum*exp(-x + a*log(x) - LnGamma(a));

  } else {
    double a0 = 1, a1 = x, b0 = 0, b1 = a0, fac = 1, n = 1, g = b1, gold = b0;

    while (fabs(g-gold) >= fabs(10*MatlabEps()*g)) {
      double ana = n-a, anf = n*fac;
      gold = g;
      a0 = (a1 + a0*ana) * fac;
      b0 = (b1 + b0*ana) * fac;
      a1 = x * a0 + anf * a1;
      b1 = x * b0 + anf * b1;
      fac = 1 / a1;
      g = b1 * fac;
      n++;
    }
    return 1 - exp(-x + a*log(x) - LnGamma(a)) * g;
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::GammaInv(double, double, double) {
  ShowError("Vector::GammaInv not implemented.");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::NormInv(double y, double m, double s) {
  if (y<=0 || y>=1 || s<=0) {
    cerr << "NormInv: argument failure" << endl;
    return 0;
  }

  static double a[] = { 0.886226899, -1.645349621, 0.914624893, -0.140543331 };
  static double b[] = { -2.118377725, 1.442710462, -0.329097515, 0.012229801 };
  static double c[] = { -1.970840454, -1.624906493, 3.429567803, 1.641345311 };
  static double d[] = { 3.543889200, 1.637067800 };

  y = 2*y-1; // $MATLAB/toolbox/matlab/specfun/erfinv.m
  double x = 0, sqrtpi = sqrt(M_PI);
  if (fabs(y)<=0.7) {
    double z = y*y;
    x=y*(((a[3]*z+a[2])*z+a[1])*z+a[0])/((((b[3]*z+b[2])*z+b[1])*z+b[0])*z+1);

  } else if (y>0.7) {
    double z = sqrt(-log((1-y)/2));
    x = (((c[3]*z+c[2])*z+c[1])*z+c[0])/((d[1]*z+d[0])*z+1);

  } else if (y<-0.7) {
    double z = sqrt(-log((1+y)/2));
    x = -(((c[3]*z+c[2])*z+c[1])*z+c[0])/((d[1]*z+d[0])*z+1);
  }

  x -= (erf(x) - y) / (2/sqrtpi * exp(-x*x));
  x -= (erf(x) - y) / (2/sqrtpi * exp(-x*x));

  return M_SQRT2*x*s + m;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::CalculateSelfDot() {
//   if (UseLinpack()) {
//     int one = 1;
//     SelfDot(sdot_(length, vector, one, vector, one));

//   } else {

//     register double dp = 0;
//     register int i = length;
//     register float *x = vector;
//     if (i)
//       do {
// 	i--;
// 	dp += *x**x;
// 	x++;
//       } while (i);
//     SelfDot(dp);
//   }

  SelfDot(DotProduct(*this));

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::NonZeros() const {
  int n = 0;
  for (int i=0; i<Length(); i++)
    if (vector[i])
      n++;

  return n;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<int> VectorOf<Type>::NonZeroIndices() const {
  VectorOf<int> ret;
  for (int i=0; i<Length(); i++)
    if (vector[i])
      ret.Push(i);

  return ret;
}

//////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<int> VectorOf<Type>::FindIndices() const {
  VectorOf<int> myindex(length);
  for (int i=0; i<length; i++) {
    int j = FirstEqual(i);
    myindex[i] = j;
    if (j<0)
      ShowError("Vector::FindIndices index not found");
  }
  return myindex;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Arrange(const VectorOf<int>& index) {
  if (length!=index.Length()) {
    ShowError("Vector::Arrange() dimension mismatch");
    SimpleAbort();
  }

  Type *newvector = new Type[length];
  for (int i=0; i<length; i++) {
    if (!IndexOK(index[i])) {
      ShowError("Vector::Arrange() index mismatch");
      SimpleAbort();
    }
    newvector[i] = vector[index[i]];
  }

  delete [] vector;   // [] added 25.9.2000
  vector = newvector;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>& VectorOf<Type>::Move(int i, int j) {
  if (IndexOK(i)&&IndexOK(j)&&i!=j) {
    Type tmp = Get(i);
    
    if (i<j)
      for (int k=i; k<j; k++)
	vector[k] = vector[k+1];

    else
      for (int k=i; k>j; k--)
	vector[k] = vector[k-1];

    Set(j, tmp);
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::IntersectOrdered(const VectorOf<Type>& set) {
  int n = set.Length()-1;
  for (int i=Length()-1; i>=0; i--)
    if (n<0 || vector[i]>set[n])
      Remove(i);
    else {
      if (vector[i]<set[n])
	i++;
      n--;
    }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorOf<Type>::ToString(int, char*, int, int) const {
  ShowError("VectorOf<Type>::ToString() not implemented");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

template <>
int VectorOf<float>::ToString(int i, char *buf, int len, int prec) const {
  if (!prec)
    return len>20 ? sprintf(buf, "%g", Get(i)) : 0;

  return len>20 ? sprintf(buf, "%.*g", prec, Get(i)) : 0;
}

///////////////////////////////////////////////////////////////////////////////

template <>
int VectorOf<int>::ToString(int i, char *buf, int len, int) const {
  return len>20 ? sprintf(buf, "%d", Get(i)) : 0;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorOf<Type>::MapNegativeZeroPositive(Type n, Type z, Type p) {
  Type zero = Type(0);

  for (int i=0; i<length; i++)
    if (FastGet(i)<zero)
      FastSet(i, n);
    else if (FastGet(i)>zero)
      FastSet(i, p);
    else
      FastSet(i, z);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::SymbolEntropy(double b) const {
  VectorOf<Type> s = *this;
  s.QuickSortIncreasingly();
    // s.Dump(DumpLong);

  double h = 0, n = 0, l = s.Length();
  
  for (int i=0; i<l; i++) {
    n++;
    if (i==l-1 || s[i+1]!=s[i]) {
      double p = n/l;
      h -= p*log(p);

      //cout << "i="<< i << " s[i]="<< s[i] << " n="<< n << " p="<< p << endl;

      n = 0;
    }
  }

  return h/log(b);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorOf<Type>::SymbolEquiVocation(const VectorOf<Type>& v,
					  double b) const {
  if (!CheckDisparity(v, TRUE, "VectorOf::SymbolEquiVocation"))
    return 0;

  VectorOf<Type> s = v;
  s.QuickSortIncreasingly();

  double h = 0;
  int n = 0, l = s.Length();

  for (int i=0; i<l; i++) {
    n++;
    if (i==l-1 || s[i+1]!=s[i]) {
      double p = double(n)/l;
      VectorOf<Type> t(n);
      for (int q=0, r=0; q<l; q++)
	if (v[q]==s[i])
	  t[r++] = Get(q);
      
      h += p*t.SymbolEntropy(b);

      n = 0;
    }
  }

  return h;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// VectorOf<Type>::() {

// }

///////////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _VECTOR_C_

