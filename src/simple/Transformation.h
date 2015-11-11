// -*- C++ -*-  $Id: Transformation.h,v 1.3 2009/11/20 20:48:16 jorma Exp $
// 
// Copyright 1994-2008 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _TRANSFORMATION_H_
#define _TRANSFORMATION_H_

//- Transformation
namespace simple {
class Transformation : public Simple {
public:
  Transformation(double xx = 0, double yy = 0, double ss = 1) {
    x = xx; y = yy; s = ss; }

  int operator==(const Transformation& t) const {
    return x==t.x && y==t.y && s ==t.s; }
  int operator!=(const Transformation& t) const {
    return x!=t.x || y!=t.y || s !=t.s; }

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "Transformation", NULL }; return n; }

  virtual void Dump(DumpMode = DumpDefault, ostream& os = cout) const {
    os << "Transformation " << (void*)this
       << " x=" << x << " y=" << y << " s=" << s << endl;
  }

  virtual void DumpInLine(ostream& os = cout) const {
    os << "[" << x << "," << y << "," << s << "]";
  }

  void Initialize() { Set(0, 0, 1); }

  double X() const { return x; }
  double Y() const { return y; }
  double Scale() const { return s; }
  Transformation& X(double xx) {     x = xx; return *this; }
  Transformation& Y(double yy) {     y = yy; return *this; }
  Transformation& Scale(double ss) { s = ss; return *this; }

  void Set(double xx, double yy, double ss = 1) {
    x = xx; y = yy; s = ss; }

  void Apply(double& xx, double& yy) const {
    xx = s*xx+x; yy = s*yy+y;
  }

  void PreMultiply(const Transformation& t) {
    x += s*t.x;
    y += s*t.y;
    s *= t.s;
  }

  void PostMultiply(const Transformation& t) {
    x = t.s*x + t.x;
    y = t.s*y + t.y;
    s *= t.s;
  }

//   int IsDefined() { return p1.IsDefined() && p2.IsDefined(); }
//   void SetUndefined() { p1.SetUndefined(); p2.SetUndefined(); }

  friend ostream& operator<<(ostream& os, Transformation& t) {
    t.DumpInLine(os);
    return os;
  }
//   friend istream& operator>>(istream& is, Transformation& t) {
//     is >> t.x >> t.y >> t.s;
//     return is;
//   }

protected:
  float x, y, s;
};

} // namespace simple
#endif // _TRANSFORMATION_H_

