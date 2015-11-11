// -*- C++ -*-  $Id: Point.h,v 1.5 2009/11/20 20:48:15 jorma Exp $
// 
// Copyright 1994-2008 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _POINT_H_
#define _POINT_H_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <Simple.h>

#include <math.h>

#if SIMPLE_PLANAR
namespace simple {
class Planar;

#ifdef __GNUG__
extern int  intpoint_planar_to_index(const void*, const void*);
extern void intpoint_planar_increment(void*, const void*);
#endif //  __GNUG__
#endif // SIMPLE_PLANAR

#define UNDEF_COORD MAXINT

//- Point
template <class Type> class PointOf : public Simple {
public:
  PointOf() { x = UNDEF_COORD; y = UNDEF_COORD; planar = NULL; }
  PointOf(Type xx, Type yy) { x = xx; y = yy; planar = NULL; }
  // automatic // PointOf(const PointOf& p) { *this = p; }
  PointOf(const char *str) { sscanf(str, Format(), &x, &y); planar = NULL; }
  PointOf(int xx) { x = xx; y = 0; planar = NULL; }
  PointOf(double xx) { x = Type(xx); y = 0; planar = NULL; }

  virtual ~PointOf() {}

#if SIMPLE_PLANAR
  PointOf(const Planar& p) { planar = &p; x = y = 0; }
  /*inline*/ int ToIndex() const;
  /*inline*/ PointOf& operator++();
#endif // SIMPLE_PLANAR

  virtual const char **SimpleClassNames() const {
    static const char *n[] = { "Point", NULL }; return n; }

  virtual void Dump(DumpMode = DumpDefault, ostream& os = cout) const {
    os << "Point " << (void*)this << " x=" << x << " y=" << y << endl;
  }

  // virtual Simple *Copy() const { return new PointOf(X(), Y()); }

  void DumpInLine(ostream& os = cout) const {
    os << "(" << x << "," << y << ")";
  }

  static /*inline*/ PointOf Maximum();
  static /*inline*/ PointOf Minimum();

  Type X() const { return x; }
  Type Y() const { return y; }
  PointOf& X(Type xx) { x = xx; return *this; }
  PointOf& Y(Type yy) { y = yy; return *this; }

  PointOf& Set(Type xx, Type yy) { X(xx); Y(yy); return *this; }

  int IsDefined() const { return x!=UNDEF_COORD || y!=UNDEF_COORD; }
  PointOf& SetUndefined() { Set(UNDEF_COORD, UNDEF_COORD); return *this; }

  PointOf operator+(const PointOf& p) const {
    return PointOf(X()+p.X(), Y()+p.Y()); }

  PointOf operator-(const PointOf& p) const {
    return PointOf(X()-p.X(), Y()-p.Y()); }

  PointOf operator-() const {
    return PointOf(-X(), -Y()); }

  PointOf operator*(Type d) const {
    return PointOf(X()*d, Y()*d);
  }

  PointOf& operator*=(Type d) {
    x *= d;
    y *= d;
    return *this;
  }

  operator int() const { return IsDefined(); }
  int operator !() const { return !IsDefined(); }

  PointOf operator/(double d) const {
//  this is stupiiiid defaulting of Type==int
    return PointOf((Type)floor(X()/d+0.5), (Type)floor(Y()/d+0.5));
  }

  PointOf operator*(double d) const {
//  this is stupiiiid defaulting of Type==int
    return PointOf((Type)floor(X()*d+0.5), (Type)floor(Y()*d+0.5));
  }

  PointOf& operator*=(double d) {
//  this is stupiiiid defaulting of Type==int
    return *this = *this*d;
  }

  PointOf& operator+=(const PointOf& p) {
    return Set(X()+p.X(), Y()+p.Y());
  }

  PointOf& operator-=(const PointOf& p) {
    return Set(X()-p.X(), Y()-p.Y());
  }

  PointOf& operator*=(const PointOf&) {
    cerr << "PointOf operator *= not implemented." << endl;
    return *this;
  }

  PointOf& operator/=(const PointOf&) {
    cerr << "PointOf operator /= not implemented." << endl;
    return *this;
  }

  PointOf operator*(const PointOf&) const {
    cerr << "PointOf operator * not implemented." << endl;
    return *this;
  }

  PointOf operator/(const PointOf&) const {
    cerr << "PointOf operator / not implemented." << endl;
    return *this;
  }

//   operator double() const {
//     cerr << "PointOf operator double not implemented." << endl;
//     return 0;
//   }

  int operator==(const PointOf& p) const { return X()==p.X() && Y()==p.Y(); }
  int operator!=(const PointOf& p) const { return X()!=p.X() || Y()!=p.Y(); }

  int operator<(const PointOf& p) const { return Sqr()<p.Sqr(); }
  int operator>(const PointOf& p) const { return Sqr()>p.Sqr(); }
  int operator<=(const PointOf& p) const { return Sqr()<=p.Sqr(); }
  int operator>=(const PointOf& p) const { return Sqr()>=p.Sqr(); }

  int operator<(int i)    const { return X()+Y()<i; }
  int operator<(double d) const { return X()+Y()<d; }

  // moved metric to Simple.h 3.10.2000 jl
  // enum Metric { NoMetric, FourNeigh, EightNeigh, Euclidean };

  double DistanceSquared(const PointOf& p) const {
    double tmpx = X()-p.X(), tmpy = Y()-p.Y();
    return tmpx*tmpx+tmpy*tmpy;
  }

  double Distance(const PointOf& p, Metric m = Euclidean) const {
    switch (m) {
    case Euclidean:
      return sqrt(DistanceSquared(p));
    case FourNeigh:
      return fabs(double(X()-p.X()))+fabs(double(Y()-p.Y()));
    case EightNeigh: {
      double a = fabs(double(X()-p.X())), b = fabs(double(Y()-p.Y()));
      return a>b ? a : b; }
    default:;
    }
    return -1;
  }

  static int MetricNNearests(Metric m) {
    switch (m) {
    case FourNeigh:
      return 4;
    case EightNeigh:
      return 8;
    default:;
    }
    return 0;
  }


  Type Sqr() const { return X()*X()+Y()*Y(); }

  static const char *Verbose(Metric m) {
    switch (m) {
    case Euclidean:  return "Euclidean";
    case NoMetric:   return "NoMetric";
    case FourNeigh:  return "FourNeigh";
    case EightNeigh: return "EightNeigh";
    }
    return NULL;
  }

  //  friend ostream& operator<<(ostream& os, const PointOf<Type>& p);

  friend ostream& operator<<(ostream& oss, const PointOf<Type>& pp) {
    //p.DumpInLine(oss);
    oss << pp.X() << " " << pp.Y();
    return oss;
  }

  friend istream& operator>>(istream& is, PointOf<Type>& p) {
    is >> p.x >> p.y;
    return is;
  }

protected:
  Type x, y;
#if SIMPLE_PLANAR
  const Planar *planar;
#else
  void *planar;
#endif // SIMPLE_PLANAR 

  inline static const char *Format();
};

  /*
#if SIMPLE_PLANAR

// #define __OLD_GNUG__

#ifndef __OLD_GNUG__
#include <Planar.h>
#endif // __OLD_GNUG__

template <class Type> 
inline int PointOf<Type>::ToIndex() const {
  if (!planar)
    abort();
#ifdef __OLD_GNUG__
  return intpoint_planar_to_index(this, planar);
#else //  __OLD_GNUG__
  return planar->ToIndex((int)x, (int)y);
#endif //  __OLD_GNUG__
}


template <class Type> 
inline PointOf<Type>& PointOf<Type>::operator++() {
  if (!planar)
    abort();
  if (IsDefined()) {
#ifdef __OLD_GNUG__
    intpoint_planar_increment(this, planar);
#else //  __OLD_GNUG__
    x++;
    if (x>=planar->Width()) {
      x = 0;
      y++;
      if (y>=planar->Height())
	SetUndefined();
    }
#endif //  __OLD_GNUG__
  }
  return *this;
}
#endif // SIMPLE_PLANAR 
  */

// template <class Type> 
// inline int PointOf<Type>::ToIndex() const {
//   if (!planar)
//     abort();
//   return planar->ToIndex((int)x, (int)y);
// }

// template <class Type> 
// inline PointOf<Type>& PointOf<Type>::operator++() {
//   if (!planar)
//     abort();
//   if (IsDefined()) {
//     x++;
//     if (x>=planar->Width()) {
//       x = 0;
//       y++;
//       if (y>=planar->Height())
// 	SetUndefined();
//     }
//   }
//   return *this;
// }

template <class Type>
ostream& operator<<(ostream& os, const PointOf<Type>& p) {
  if (p.IsDefined())
    p.DumpInLine(os);
  else
    os << "(UNDEF)";

  return os;
}

typedef PointOf<int>   IntPoint;
typedef PointOf<float> FloatPoint;

template <>
IntPoint IntPoint::Maximum();

template <>
IntPoint IntPoint::Minimum();

template <>
FloatPoint FloatPoint::Maximum();

template <>
FloatPoint FloatPoint::Minimum();

template <>
const char *IntPoint::Format();

template <>
const char *FloatPoint::Format();

// template <class Type> inline PointOf<Type>& PointOf<Type>::operator++(int) {
//   if (!planar)
//     abort();
//   if (IsDefined()) {
//     x++;
//     if (x>=planar->Width()) {
//       x = 0;
//       y++;
//       if (y>=planar->Height())
// 	SetUndefined();
//     }
//   }
//   return *this;
// }

} // namespace simple
#endif // _POINT_H_

