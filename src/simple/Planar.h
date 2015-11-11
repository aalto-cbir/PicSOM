// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: Planar.h,v 1.3 2009/11/20 20:48:15 jorma Exp $

// -*- C++ -*-

#ifndef _PLANAR_H_
#define _PLANAR_H_

#include <Point.h>

#if !SIMPLE_PLANAR
#error SIMPLE_PLANAR not defined
#endif

namespace simple {
class Planar {

 public:
  enum TopologyType { Rectangular, Torus };
  static std::string TopologyName(TopologyType t){
    switch(t){
    case Rectangular: return "rectangular";
    case Torus:       return "torus";
    }
    return "unknown";
  }
 
protected:
  Planar(int w = 0, int h = 0,TopologyType t=Rectangular) { 
    width = w; 
    height = h; 
    topology=t;
  }

  Planar(const Planar& p) : width(p.width), height(p.height), 
    topology(p.topology) {
  }
    
  virtual ~Planar() {}

  Planar& operator=(const Planar& p) { // Size is _not_ called
    // cout << "Now in Planar::operator=" << endl;
    width = p.width; height = p.height; topology = p.topology;
    return *this; }

  int operator==(const Planar& p) const {
    return width==p.width && height==p.height && topology==p.topology; }
  int operator!=(const Planar& p) const { return !operator==(p); }

//   virtual void Dump(DumpMode = DumpRecursiveShort, ostream& = cout) const;
//   virtual const char **Names() const { static const char *n[] = {
//     "Planar", "Vector", NULL }; return n; }

public:
  bool PlanarIndexOK(int i) const { return i>=0 && i<width*height; }

  int IsInside(int x, int y) const {
    return x>=0 && x<width && y>=0 && y<height; }
  int IsInside(const PointOf<int>& p) const { return IsInside(p.X(), p.Y()); }

  int ToIndex(int x, int y) const { return x+y*width; }
  int ToIndex(const PointOf<int>& p) const { return ToIndex(p.X(), p.Y()); }

  PointOf<int> ToPoint(int i) const {
    return i>=0 && i<width*height ? PointOf<int>(i%width, i/width) : PointOf<int>(); }

  int Nearest(int x, int y) const {
    if (x<0) x = 0;
    else if (x>=width) x = width-1;
    if (y<0) y = 0;
    else if (y>=height) y = height-1;
    return ToIndex(x, y);
  }
  int Nearest(const PointOf<int>& p) const { return Nearest(p.X(), p.Y()); }

  int XCoord(int i) const { return i%width; }
  int YCoord(int i) const { return i/width; }
  PointOf<int> XYCoord(int i) const { return PointOf<int>(XCoord(i), YCoord(i)); }

  int Width()     const { return width; }
  int Height()    const { return height; }
  PointOf<int> Size() const { return PointOf<int>(width, height); }

  Planar& Size(int w, int h) {
    int ow = width, oh = height;
    width = w;  height = h; return Resize(ow, oh);  
  }
  Planar& Width( int x) { return Size(x, height); }
  Planar& Height(int y) { return Size(width,  y); }
  Planar& Size(const PointOf<int>& s)  { return Size(s.X(), s.Y()); }

  double AspectRatio() const { return double(width)/height; }

  int PointType(int x, int y) const {
    if (x>0 && x<width-1 && y>0 && y<height-1)
      return 0;
    if ((x==0 || x==width-1) && (y==0 || y==height-1))
      return 2;
    return 1;
  }
  int PointType(const PointOf<int>& p) const {
    return PointType(p.X(), p.Y()); }
  int PointType(int u) const { return PointType(ToPoint(u)); }

  TopologyType Topology() const { return topology;}
  void Topology(TopologyType t){ topology=t;}

protected:
  virtual Planar& Resize(int, int) = 0;

  int width, height;
  TopologyType topology;
};

} // namespace simple
#endif // _PLANAR_H_

