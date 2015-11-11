// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: Histogram.h,v 1.4 2014/07/01 13:40:56 jorma Exp $

// -*- C++ -*-

#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#include <List.h>
#include <Vector.h>
#include <StatQuad.h>

//- Histogram
namespace simple {
class Histogram : protected VectorOf<PointOf<float> > {
public:
  Histogram(const Histogram&);

  Histogram(int b = 10) { Initialize(b, NULL, NULL); }
  Histogram(const VectorOf<float>* v, int n=10) { Initialize(n, v, NULL); }
  Histogram(const   VectorOf<int>* v, int n=10) { Initialize(n, NULL, v); }
  Histogram(const VectorOf<float>&, const VectorOf<float>&);
  Histogram(const   VectorOf<int>&, const VectorOf<float>&);

  Histogram(const VectorOf<StatQuad>&, const VectorOf<float>&) { SimpleAbort(); }
  Histogram(const VectorOf<PointOf<float> >&, const VectorOf<float>&) { SimpleAbort(); }
  Histogram(const VectorOf<PointOf<int> >&, const VectorOf<float>&) { SimpleAbort(); }
  
  Histogram(const ListOf<VectorOf<float> >*, int n=10);

  ~Histogram() { delete next; }

  Histogram& operator=(const Histogram&);
  double operator[](int i) const { return Get(i).Y(); }
  
  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

  virtual const char **Names() const { static const char *n[] = {
    "Histogram", NULL }; return n; }

  // operator const VectorOf<PointOf<float> >& () const { return *this; }

  const VectorOf<PointOf<float> >& GnuPlot() const { return *this; }

  PointOf<float> Get(int i) const { return VectorOf<PointOf<float> >::Get(i); }

  Histogram MakeCumulativePdf() const {
    Histogram ret(*this);
    ret.Cumulative(TRUE).Normalized(TRUE);
    return ret;
  }

  double GetValue(double) const;
  double GetValue(double, double) const;
  double GetValue(const FloatVector&) const;
  double GetValue(const char*) const;
  double GetValueByIndices(const IntVector&) const;
  
  double Area(const IntVector&) const;
  double Area(const char*) const;

  Histogram& Use(const   IntVector *i) { ivector = i; return Refresh(); }
  Histogram& Use(const FloatVector *f) { fvector = f; return Refresh(); }
  Histogram& Use(const ListOf<FloatVector> *l) { flist = l; return Refresh(); }
  
  FloatVector BarLimits() const;

  Histogram& Refresh();
  Histogram& MultiDimensionalRefresh();

  Histogram *Next() const { return next; }
  int VectorLength() const { return next ? 1+next->VectorLength() : 1; }
  const Histogram *Level(int) const;
  Histogram *Level(int);
  // Histogram *Level(int i) { return (Histogram*) Level(i); }

  double Min() const {
    return  lowlimit == -MAXFLOAT ? RealMin() : lowlimit; }

  double Max() const {
    return highlimit == MAXFLOAT ? RealMax() : highlimit; }

  double RealMin() const { return  fvector ? fvector->Minimum() :
    ivector ? ivector->Minimum() : -MAXFLOAT; }

  double RealMax() const { return fvector ? fvector->Maximum() :
    ivector ? ivector->Maximum() : MAXFLOAT; }

  double LimitMin() const { return  zero_ends ? BarLimits()[0] : Min(); }
  double LimitMax() const { return  zero_ends ? BarLimits().Peek() : Max(); }

  double  LowLimit() const { return  lowlimit; }
  double HighLimit() const { return highlimit; }

  Histogram& LowLimit(double l = -MAXFLOAT) { lowlimit = l; return Refresh(); }
  Histogram& HighLimit(double l = MAXFLOAT) {highlimit = l; return Refresh(); }

  Histogram& Limits(double l = -MAXFLOAT, double h = MAXFLOAT) {
    lowlimit = l; highlimit = h; return Refresh(); }

  Histogram& Expand(const FloatPoint& p) { return Expand(p.X(), p.Y()); }
  Histogram& Expand(double l, double h) {
    double mi = RealMin(), ma = RealMax();
    if (mi!=-MAXFLOAT && ma!=MAXFLOAT)
      Limits(mi-l*(ma-mi), ma+h*(ma-mi));
    else
      Limits();
    if (Next())
      Next()->Expand(l, h);
    return *this; }
  
  int TotalBins() const        { return Bins()+(zero_ends?2:0); }

  int Bins() const             { return bins; }
  Histogram& Bins(int b)       { bins = b; return Refresh(); }

  int Cumulative() const       { return cumulative; }
  Histogram& Cumulative(int c) { cumulative = c; return Refresh(); }

  int Normalized() const       { return normalized; }
  Histogram& Normalized(int c) { normalized = c; return Refresh(); }

  int Scaled() const           { return scaled; }
  Histogram& Scaled(int c)     { scaled = c; return Refresh(); }

  int BarGraph() const         { return bar_graph; }
  Histogram& BarGraph(int c)   { bar_graph = c; return Refresh(); }

  int ZeroEnds() const         { return zero_ends; }
  Histogram& ZeroEnds(int c)   { zero_ends = c; return Refresh(); }

  int NoRefresh() const        { return no_refresh; }
  Histogram& NoRefresh(int c)  { no_refresh = c; return Refresh(); }

  using VectorOf<PointOf<float> >::ShowError;
  using VectorOf<PointOf<float> >::Write;
  using VectorOf<PointOf<float> >::WriteGnuPlot;

  const FloatVector& Values()  const { return values; }
  const FloatVector& Centers() const { return centers; }
  const FloatVector& Lefts()   const { return lefts; }
  const FloatVector& Rights()  const { return rights; }
  FloatVector Values()  { return values; }
  FloatVector Centers() { return centers; }
  FloatVector Lefts()   { return lefts; }
  FloatVector Rights()  { return rights; }

  static void Statistics(const FloatVector&, ostream& = cout,
			 const char* = NULL, int = 10,
			 double = -MAXFLOAT, double = MAXFLOAT);

  Histogram& RemoveLimitValues() { limitvalues.Length(0); return Refresh(); }

  Histogram& LimitValues(const FloatVector& lv) {
    limitvalues = lv;
    return Bins(lv.Length()+1);
  }

  const FloatVector& LimitValues() const { return limitvalues; }
  FloatVector& LimitValues() { return limitvalues; }

  Histogram& Equalize();
  Histogram Partial(double, double) const;

  int GnuPlotGrid(ostream&) const;
  int GnuPlotGrid(const char *n) const {
    ofstream os(n);
    return GnuPlotGrid(os); }

  int BinNumber(double) const;

protected:
  void Initialize(int, const FloatVector*, const IntVector*);

  const FloatVector *fvector;
  const IntVector   *ivector;
  const ListOf<FloatVector> *flist;

  int bins;
  float lowlimit, highlimit;
  int no_refresh, cumulative, normalized, scaled, zero_ends, bar_graph;

  FloatVector values, lefts, rights, centers, limitvalues;
  Histogram *next;

  ListOf<FloatVector> hash, transp;
};

} // namespace simple
#endif // _HISTOGRAM_H_

