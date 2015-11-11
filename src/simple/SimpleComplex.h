// -*- C++ -*-  $Id: SimpleComplex.h,v 1.4 2009/11/20 20:48:16 jorma Exp $
// 
// Copyright 1994-2008 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _SIMPLECOMPLEX_H_
#define _SIMPLECOMPLEX_H_

#include <Point.h>

//- SimpleComplex
namespace simple {
class SimpleComplex : public FloatPoint {
public:
  SimpleComplex(float xx = 0, float yy = 0) : FloatPoint(xx, yy) {}
  SimpleComplex(const SimpleComplex& c) : FloatPoint(c.x, c.y) {}
  SimpleComplex(const char *str) : FloatPoint(str) {}
  SimpleComplex(int xx) : FloatPoint((float)xx, 0) {}
  SimpleComplex(double xx) : FloatPoint(xx, 0) {}

//  if used, Vector:DistanceSquared(SimpleComplex, SimpleComplex) should be defined
//  operator double() const { return Rho(); } 

//   SimpleComplex operator+(const SimpleComplex& p) const {
//     return SimpleComplex(X()+p.X(), Y()+p.Y()); }

  SimpleComplex operator-(const SimpleComplex& p) const {
    return SimpleComplex(X()-p.X(), Y()-p.Y()); }

  SimpleComplex operator-() const {
    return SimpleComplex(-X(), -Y()); }

//   SimpleComplex operator*(const SimpleComplex& p) const {
//     return SimpleComplex(X()*p.X()-Y()*p.Y(), X()*p.Y()+Y()*p.X()); }

  SimpleComplex operator/(const SimpleComplex& p) const {
    cerr << "Ternary SimpleComplex division unimplemented." << endl;
    return SimpleComplex(X(), p.Y()); }

  SimpleComplex operator/(double) const {
    cerr << "Ternary SimpleComplex double division unimplemented." << endl;
    return *this; }

  SimpleComplex operator/=(const SimpleComplex&) const {
    cerr << "Binary SimpleComplex division unimplemented." << endl;
    return *this; }

  SimpleComplex operator*(double d) const {
    return SimpleComplex(d*x, d*y);
  }

  SimpleComplex operator*=(double d) {
    x *= d;
    y *= d;
    return *this;
  }

  SimpleComplex operator/=(double d) {
    x /= d;
    y /= d;
    return *this;
  }

  SimpleComplex operator*(const SimpleComplex& c) const {
    return SimpleComplex(x*c.x-y*c.y, x*c.y+y*c.x);
  }

  SimpleComplex operator*=(const SimpleComplex& c) {
    SimpleComplex tmp(*this);
    x = tmp.x*c.x-tmp.y*c.y;
    y = tmp.x*c.y+tmp.y*c.x;
    return *this;
  }

  SimpleComplex operator+(const SimpleComplex& c) const {
    return SimpleComplex(x+c.x, y+c.y);
  }

  SimpleComplex operator+=(const SimpleComplex& c) {
    x += c.x;
    y += c.y;
    return *this;
  }

  SimpleComplex operator=(double r) {
    x = r;
    y = 0;
    return *this;
  }

  int operator<(int) const    { SimpleAbort(); return FALSE; }
  int operator<(double) const { SimpleAbort(); return FALSE; }

  double Real() const { return x; }
  double Imag() const { return y; }

  double Theta()      const { return atan2(y, x); }
  double Rho()        const { return sqrt(RhoSquared()); }
  double RhoSquared() const { return x*x+y*y; }       

  SimpleComplex Conjungate() const { return SimpleComplex(x, -y); }

  static SimpleComplex UnitDirection(double theta) {
    return SimpleComplex(cos(theta), sin(theta));
  }

  static SimpleComplex Zero() { return SimpleComplex(0, 0); }

  static inline SimpleComplex Maximum() {
    return SimpleComplex(-MAXFLOAT,  MAXFLOAT); }
  static inline SimpleComplex Minimum() {
    return SimpleComplex(-MAXFLOAT, -MAXFLOAT); }
};

} // namespace simple
#endif // _SIMPLECOMPLEX_H_

