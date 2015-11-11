// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: StatQuad.h,v 1.2 2009/11/20 20:48:16 jorma Exp $

// -*- C++ -*-

#ifndef _STATQUAD_H_
#define _STATQUAD_H_

#include <StatVar.h>

#undef Type

//- StatQuad
namespace simple {
class StatQuad : public Simple {
public:
  StatQuad() { x = 0; }
  StatQuad(const char *s) { From(s); }
  StatQuad(double) { NotImplementedAbort("StatQuad(double)"); }
  StatQuad(int i) { x = 0; if (i) NotImplementedAbort("StatQuad(int)"); }
  virtual ~StatQuad() {}

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& os = cout) const {
    os << Bold("StatQuad ") << (void*)this
       << " var=" << (void*)&var[0] << " ";
    Print(TRUE, os); os << endl; }

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "StatQuad", NULL }; return n; }

  StatVar& operator[](int i) /*const*/ {
    if (i<0 || i>5) {
      cerr << "StatQuad::operator[](int i) index " << i << " out of bounds"
	   << endl;
      abort();
    }
    return var[i];
  }

  operator double() const {
    NotImplemented("operator StatQuad::double() const");
    return 0;
  }

  double& X() { return x; }
  double  X() const { return x; }
  StatQuad& X(double xx) { x = xx; return *this; }

  StatQuad& operator+=(const StatQuad& s) {
    for (int i=0; i<6; i++)
      var[i] += s.var[i];

    return *this;
  }

  StatQuad& operator-=(const StatQuad&) {
    NotImplemented("operator-=(const StatQuad&) const");
    return *this;
  }

  StatQuad& operator*=(const StatQuad&) {
    NotImplemented("operator*=(const StatQuad&) const");
    return *this;
  }

  StatQuad& operator/=(const StatQuad&) {
    NotImplemented("operator/=(const StatQuad&) const");
    return *this;
  }

  StatQuad operator+(const StatQuad& s) const {
    StatQuad r(*this);
    r += s;

    return r;
  }

  StatQuad operator-(const StatQuad&) const {
    NotImplementedAbort("operator-(const StatQuad&) const");
    return *this;
  }

  StatQuad operator*(const StatQuad&) const {
    NotImplementedAbort("operator*(const StatQuad&) const");
    return *this;
  }

  StatQuad operator/(const StatQuad&) const {
    NotImplementedAbort("operator/(const StatQuad&) const");
    return *this;
  }

  StatQuad operator-() const {
    NotImplementedAbort("operator-() const");
    return *this;
  }

  StatQuad operator*(double) const {
    NotImplementedAbort("operator*(double) const");
    return *this;
  }

  StatQuad operator/(double) const {
    NotImplementedAbort("operator/(double) const");
    return *this;
  }

  StatQuad& operator*=(double) {
    NotImplementedAbort("operator*=(double)");
    return *this;
  }
 
  int operator==(const StatQuad& s) const {
    for (int i=0; i<6; i++)
      if (var[i]!=s.var[i])
	return FALSE;
    return TRUE;
  }

  int operator!=(const StatQuad& s) const {
    return !(*this==s);
  }

  int operator<(const StatQuad&) const {
    NotImplemented("operator<(const StatQuad&) const");
    return FALSE;
  }

  int operator<(double) const {
    NotImplemented("operator<(double) const");
    return FALSE;
  }

  int operator>(const StatQuad&) const {
    NotImplemented("operator>(const StatQuad&) const");
    return FALSE;
  }

  int operator>=(const StatQuad&) const {
    NotImplemented("operator>=(const StatQuad&) const");
    return FALSE;
  }

  static StatQuad Maximum() {
    StatQuad tmp;
    tmp[0] = tmp[1] = tmp[2] = tmp[3] = StatVar::Maximum();
    return tmp;
  }

  static StatQuad Minimum() {
    StatQuad tmp;
    tmp[0] = tmp[1] = tmp[2] = tmp[3] = StatVar::Minimum();
    return tmp;
  }

  void Clear() {
    for (int i=0; i<6; i++)
      var[i].Clear();
  }

//   int IsSet() const {
//     for (int i=0; i<4; i++)
//       if (!var[i].IsSet())
// 	return FALSE;
//     return TRUE;
//   }

  int IsFullySet() const {
    for (int i=0; i<6; i++)
      if (!var[i].IsSet())
	return FALSE;
    return TRUE;
  }

  int IsPartiallySet() const {
    for (int i=0; i<6; i++)
      if (var[i].IsSet())
	return TRUE;
    return FALSE;
  }

  int HasReject() const {
    return float(TrainReject())!=0 || float(TestReject())!=0 ||
      float(CrossValidationReject())!=0; }

  void Print(int verb = FALSE, ostream& os = cout) const {
    if (!verb)
      os << "[";

    for (int i=0; i<6; i++) {
      var[i].Print(verb, os, verb ? "[]" : NULL);
      if (i<5 && verb)
	os << " ";
    }
    if (verb)
      os << " x=" << x;
    else
      os << x << "]";
  }

  StatQuad& ZeroTrain() {
    TrainError(StatVar()); TrainReject(StatVar()); return *this; }
  StatQuad& ZeroTest() {
    TestError(StatVar()); TestReject(StatVar()); return *this; }
  StatQuad& ZeroCrossValidation() {
    CrossValidationError(StatVar());
    CrossValidationReject(StatVar()); return *this; }
  StatQuad& ZeroError() {
    TrainError(StatVar()); TestError(StatVar());
    CrossValidationError(StatVar()); return *this; }
  StatQuad& ZeroReject() {
    TrainReject(StatVar()); TestReject(StatVar());
    CrossValidationReject(StatVar()); return *this; }

  StatQuad& TrainReject(const StatVar& x) { var[0] = x; return *this; }
  StatQuad& TrainError( const StatVar& x) { var[1] = x; return *this; }
  StatQuad& TestReject( const StatVar& x) { var[2] = x; return *this; }
  StatQuad& TestError(  const StatVar& x) { var[3] = x; return *this; }
  StatQuad& CrossValidationReject(const StatVar& x) { var[4]=x; return *this;}
  StatQuad& CrossValidationError( const StatVar& x) { var[5]=x; return *this;}

  StatQuad& TrainReject(double x) { var[0].Add(x); return *this; }
  StatQuad& TrainError( double x) { var[1].Add(x); return *this; }
  StatQuad& TestReject( double x) { var[2].Add(x); return *this; }
  StatQuad& TestError(  double x) { var[3].Add(x); return *this; }
  StatQuad& CrossValidationReject( double x) { var[4].Add(x); return *this; }
  StatQuad& CrossValidationError(  double x) { var[5].Add(x); return *this; }

  StatVar& TrainReject() 	   { return var[0]; }
  StatVar& TrainError()  	   { return var[1]; }
  StatVar& TestReject()  	   { return var[2]; }
  StatVar& TestError()   	   { return var[3]; }
  StatVar& CrossValidationReject() { return var[4]; }
  StatVar& CrossValidationError()  { return var[5]; }

  StatVar TrainReject() 	   const { return var[0]; }
  StatVar TrainError()  	   const { return var[1]; }
  StatVar TestReject()  	   const { return var[2]; }
  StatVar TestError()   	   const { return var[3]; }
  StatVar CrossValidationReject()  const { return var[4]; }
  StatVar CrossValidationError()   const { return var[5]; }

  FloatPoint TrainTestReject() const {
    return FloatPoint(TrainReject(), TestReject()); }

  FloatPoint TrainTestError() const {
    return FloatPoint(TrainError(), TestError()); }

  FloatPoint CrossValidationTestReject() const {
    return FloatPoint(CrossValidationReject(), TestReject()); }

  FloatPoint CrossValidationTestError() const {
    return FloatPoint(CrossValidationError(), TestError()); }

  FloatPoint TrainRejectError() const {
    return FloatPoint(TrainReject(), TrainError()); }

  FloatPoint TestRejectError() const {
    return FloatPoint(TestReject(), TestError()); }

  FloatPoint CrossValidationRejectError() const {
    return FloatPoint(CrossValidationReject(), CrossValidationError()); }

  static VectorOf<float> TrainReject(const VectorOf<StatQuad>& v) {
    return Component(v, 0); }
  static VectorOf<float> TrainError(const VectorOf<StatQuad>& v) {
    return Component(v, 1); }
  static VectorOf<float> TestReject(const VectorOf<StatQuad>& v) {
    return Component(v, 2); }
  static VectorOf<float> TestError(const VectorOf<StatQuad>& v) {
    return Component(v, 3); }
  static VectorOf<float> CrossValidationReject(const VectorOf<StatQuad>& v) {
    return Component(v, 4); }
  static VectorOf<float> CrossValidationError(const VectorOf<StatQuad>& v) {
    return Component(v, 5); }

  inline static VectorOf<float> Component(const VectorOf<StatQuad>&, int);


  static VectorOf<StatQuad>& RealSize(VectorOf<StatQuad>& v) {
//     for (int i=v.Length()-1; i>=0; i--)
//       if (!v[i].IsPartiallySet())
// 	v.Lengthen(i);

    while (v.Length() && !v.Last().IsPartiallySet())
      v.Pop();

    while (v.Length() && !v[0].IsPartiallySet())
      v.Pull();
    
    return v;
  }

  static int HasX(const VectorOf<StatQuad>& v) {
    for (int i=0; i<v.Length(); i++)
      if (v[i].X())
	return TRUE;
    return FALSE;
  }

  void Display(ostream& os = cout) const {
    streamsize prec = os.precision(2);
#if !defined(__GNUC__) || __GNUC__<3
    int setf = (int)os.setf(ios::fixed, ios::floatfield);
} // namespace simple
#endif // __GNUC__

    os << " errors:  "
       << (float)TestError() << " "
       << (float)CrossValidationError() << " "
       << (float)TrainError() << "  (test cross-v train)" << endl;

    if (HasReject())
      os << " rejects: "
       << (float)TestReject() << " "
       << (float)CrossValidationReject() << " "
       << (float)TrainReject() << endl;

#if !defined(__GNUC__) || __GNUC__<3
    os.setf(setf, ios::floatfield);
} // namespace simple
#endif // __GNUC__
    os.precision(prec);
  }

  void From(const char *str) {
    x = 0;

    char tmp[1000];
    const char *ptr = NULL, *orig = str;
    int bd = 0, i = 0;
    for (; *str; str++) {
      if (*str=='[') {
	bd++;
	if (bd==2)
	  ptr = str;
      }

      if (*str==']') {
	bd--;
	if (bd==1) {
	  memcpy(tmp, ptr, str-ptr+1);
	  tmp[str-ptr+1] = 0;
	  ptr = NULL;
	  if (i>5) goto err;
	  var[i++].From(tmp);
	  if (i==6)
	    x = atof(str+1);
	}
      }

      if (bd<0 || bd>2) goto err;
    }

    if (i!=4 && i!=6) {
    err:
      cerr << "StatQuad::From(" << orig << ") failed: bd=" << bd
	   << " i=" << i << endl;
      abort();
    }
  }

  friend ostream& operator<<(ostream& os, const StatQuad& p)
    // #ifndef __DECCXX
  { p.Print(FALSE, os); return os; }
  // #else
  // ;
  // #endif // __DECCXX

protected:
  StatVar var[6];
  double x;

  static void NotImplemented(const char *txt) {
    ShowError("Not implemented StatQuad::", txt);
  }
  static void NotImplementedAbort(const char *txt) {
    NotImplemented(txt);
    abort();
  }
};

typedef VectorOf<StatQuad> StatQuadVector;

template <>
inline const char *VectorOf<StatQuad>::TypeName() const { return "StatQuad"; }

inline VectorOf<float> StatQuad::Component(const VectorOf<StatQuad>& v, int i){
  VectorOf<float> ret(v.Length());
  for (int j=0; j<ret.Length(); j++)
    ret[j] = v[j].var[i];

  return ret;
}

} // namespace simple
#endif // _STATQUAD_H_

