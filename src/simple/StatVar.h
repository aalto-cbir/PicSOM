// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: StatVar.h,v 1.3 2009/11/20 20:48:16 jorma Exp $

// -*- C++ -*-

#ifndef _STATVAR_H_
#define _STATVAR_H_

#include <Vector.h>

//- StatVar
namespace simple {
class StatVar : public Simple {
public:
  StatVar() { Clear(); }
  StatVar(const char *s) { From(s); }
  inline StatVar(const VectorOf<float>& vec);
  StatVar(double r) { Clear(); Add(r); }
  StatVar(int i) { Clear(); if (i) NotImplementedAbort("StatVar(int)"); }

  virtual ~StatVar() {}

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& os = cout) const {
    os << Bold("StatVar ") << (void*)this
       << " n="   << n
       << " sum=" << sum
       << " sqr=" << sqr
       << " min=" << min
       << " max=" << max 
       << endl;
  }

  virtual const char **SimpleClassNames() const { static const char *nam[] = {
    "StatVar", NULL }; return nam; }

  operator double() const { return Mean(); }

  StatVar& operator+=(const StatVar& s) {
    n += s.n;
    sum += s.sum;
    sqr += s.sqr;
    if (s.min<min)
      min = s.min;
    if (s.max>max)
      max = s.max;

    return *this;
  }

  inline StatVar& operator=(const VectorOf<float>& vec);

  StatVar& operator=(double v) {
    Clear();
    Add(v);

    return *this;
  }

  StatVar& operator+=(double v) {
    return Add(v);
  }

  StatVar& operator-=(const StatVar&) {
    NotImplemented("operator-=(const StatVar&) const");
    return *this;
  }

  StatVar& operator*=(const StatVar&) {
    NotImplemented("operator*=(const StatVar&) const");
    return *this;
  }

  StatVar& operator/=(const StatVar&) {
    NotImplemented("operator/=(const StatVar&) const");
    return *this;
  }

  StatVar operator+(const StatVar& s) const {
    StatVar r(*this);
    r += s;

    return r;
  }

  StatVar operator-(const StatVar&) const {
    NotImplementedAbort("operator-(const StatVar&) const");
    return *this;
  }

  StatVar operator*(const StatVar&) const {
    NotImplementedAbort("operator*(const StatVar&) const");
    return *this;
  }

  StatVar operator/(const StatVar&) const {
    NotImplementedAbort("operator/(const StatVar&) const");
    return *this;
  }

  StatVar operator-() const {
    NotImplementedAbort("operator-() const");
    return *this;
  }

  StatVar operator*(double) const {
    NotImplementedAbort("operator*(double) const");
    return *this;
  }

  StatVar operator/(double) const {
    NotImplementedAbort("operator/(double) const");
    return *this;
  }

  StatVar& operator*=(double) {
    NotImplementedAbort("operator*=(double)");
    return *this;
  }
 
  int operator==(const StatVar& s) const {
    return n==s.n && sum==s.sum && sqr==s.sqr && min==s.min && max==s.max;
  }

  int operator!=(const StatVar& s) const {
    return !(*this==s);
  }

  int operator<(const StatVar&) const {
    NotImplemented("operator<(const StatVar&) const");
    return FALSE;
  }

  int operator<(double) const {
    NotImplemented("operator<(double) const");
    return FALSE;
  }

  int operator>(const StatVar&) const {
    NotImplemented("operator>(const StatVar&) const");
    return FALSE;
  }

  int operator>=(const StatVar&) const {
    NotImplemented("operator>=(const StatVar&) const");
    return FALSE;
  }

  static StatVar Maximum() {
    StatVar tmp;
    tmp.n = MAXINT;
    tmp.sum = tmp.sqr = tmp.min = tmp.max = MAXFLOAT;
    return tmp;
  }

  static StatVar Minimum() {
    StatVar tmp;
    tmp.n = -MAXINT;
    tmp.sum = tmp.sqr = tmp.min = tmp.max = -MAXFLOAT;
    return tmp;
  }

  StatVar& Clear() {
    n = 0;
    sum = sqr = min = max = 0;
    return *this;
  }

  StatVar& Add(double r) {
    n++;
    sum += r;
    sqr += r*r;
    if (r>max || n==1)
      max = r;
    if (r<min || n==1)
      min = r;

    return *this;
  }

  inline StatVar& Add(const VectorOf<float>& vec);

  int IsSet() const { return n>0; }

  double Min() const { return n ? min : 0; }
  double Max() const { return n ? max : 0; }
  double Mean() const { return n ? sum/n : 0; }
  int N() const { return n; }

  double Variance() const {
    return n>1 ? (sqr-sum*Mean())/(n-1) : 0;
  }

  double StandDev() const {
    double q = Variance();
    return sqrt(q>0 ? q : 0);
  }

  void Print(int verb = FALSE, ostream& os = cout, const char *t=NULL) const {
    if (!verb) 
      os << "[" << n << " " << sum << " " << sqr << " " << min << " " << max
	 << "]";

    else if (IsSet() || !t) {
      os << "n=" << n
	 << " sum=" << sum
	 << " sqr=" << sqr
	 << " min=" << min
	 << " max=" << max;

      if (n) {
	os << " ";

	os << Mean();
	if (n>1)
	  os << " " << StandDev();
      }
    }
    else
      os << t;
  }

  void From(const char *str) {
    if (sscanf(str, "[%d%lf%lf%lf%lf]", &n, &sum, &sqr, &min, &max)!=5) {
      cerr << "StatVar::From(" << str << ") failed" << endl;
      abort();
    }
  }

  double MeanStandDevNormalize(double d) const {
    double q = StandDev();
    return q ? (d-Mean())/q : d; }

  static FloatVector MeanStandDevNormalize(const VectorOf<StatVar>& svar,
					   const FloatVector& vec) {
    FloatVector ret(vec.Length(), NULL, vec.Label());
    for (int i=0; i<ret.Length(); i++)
      ret[i] = svar[i].MeanStandDevNormalize(vec[i]);
    return ret; }

  static FloatVector Variance(const VectorOf<StatVar>& svar) {
    FloatVector ret(svar.Length(), NULL, svar.Label());
    for (int i=0; i<ret.Length(); i++)
      ret[i] = svar[i].Variance();
    return ret; }

  static FloatVector StandDev(const VectorOf<StatVar>& svar) {
    FloatVector ret(svar.Length(), NULL, svar.Label());
    for (int i=0; i<ret.Length(); i++)
      ret[i] = svar[i].StandDev();
    return ret; }

  friend ostream& operator<<(ostream& os, const StatVar& p) {
    p.Print(FALSE, os); return os; }

protected:
  int n;
  double sum;
  double sqr;
  double min;
  double max;

  static void NotImplemented(const char *txt) {
    cerr << "Not implemented StatVar::" << txt << endl;
  }
  static void NotImplementedAbort(const char *txt) {
    NotImplemented(txt);
    abort();
  }
};

inline StatVar::StatVar(const VectorOf<float>& vec) { *this = vec; }

inline StatVar& StatVar::operator=(const VectorOf<float>& vec) {
  Clear();
  Add(vec);

  return *this;
}

inline StatVar& StatVar::Add(const VectorOf<float>& vec) {
  for (int i=0; i<vec.Length(); i++)
    Add(vec[i]);

  return *this;
}

typedef VectorOf<StatVar> StatVarVector;

template <>
inline const char *StatVarVector::TypeName() const { return "StatVar"; }

} // namespace simple
#endif // _STATVAR_H_

