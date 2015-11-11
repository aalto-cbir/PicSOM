// -*- C++ -*-  $Id: GnuPlot.h,v 1.6 2009/11/20 20:48:15 jorma Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND

#ifndef _GNUPLOT_H_
#define _GNUPLOT_H_

#include <Vector.h>
#include <List.h>
#include <StatQuad.h>

#include <list>
#include <string>
#include <iostream>
using namespace std;

namespace simple {

typedef VectorOf<StatQuad> StatQuadVector;

#undef Data  // who .... emits this ?????

class GnuPlot;

class GnuPlotData : public Simple {
public:
  GnuPlotData() { SimpleAbort(); }
  GnuPlotData(GnuPlot*, int, const char* = NULL, const char* = NULL);
  GnuPlotData(const GnuPlotData& g) : Simple(g) {
    cerr << "GnuPlotData(const GnuPlotData&) not implemented." << endl;
  }

  virtual ~GnuPlotData() {
    if (!keep)
      unlink(name);
    delete name;
    delete text;
    delete style;
    delete ownivec;
    delete ownfvec;
    delete ownpvec;
  }

  int Write(ostream&) { return TRUE; }
//  GnuPlotData *Copy() const { return NULL; }
  const char **SimpleClassNames() const { return NULL; }
  void Dump(DumpMode, ostream&) const;

  int Hide() const { return hide; }
  void Hide(int i) { hide = i; }

  int Length() const {
    return length ? length : pvec ? pvec->Length() : fvec ? fvec->Length()
      : ivec ? ivec->Length() : 0; }

  void Set(int i, const FloatPoint& r) {
    if (ownpvec) {
      if (ownpvec->Length()<i+1) ownpvec->Lengthen(i+1);
      ownpvec->Set(i, r);
    }
    else
      cerr << "GnuPlotData::Set(..., FloatPoint) failed" << endl;
  }

  void Set(int i, double r) {
    if (ownfvec) {
      if (ownfvec->Length()<i+1) ownfvec->Lengthen(i+1);
      ownfvec->Set(i, r);
    }
    else
      cerr << "GnuPlotData::Set(..., double) failed" << endl;
  }

  void Set(int i, int r) {
    if (ownivec) {
      if (ownivec->Length()<i+1) ownivec->Lengthen(i+1);
      ownivec->Set(i, r);
    }
    else
      cerr << "GnuPlotData::Set(..., int) failed" << endl;
  }

  GnuPlotData& Refresh();

  GnuPlotData& IncLen() { length++; return *this; }

  GnuPlotData& AddLine(const char *l) { data << l << endl; return IncLen(); }
  GnuPlotData& AddValue(double v)     { data << v << endl; return IncLen(); }
  GnuPlotData& AddValue(int    v)     { data << v << endl; return IncLen(); }
  GnuPlotData& AddValue(const FloatPoint& p) {
    return AddValue(p.X(), p.Y()); }

  GnuPlotData& AddValue(double v, double w) { data << v << " " << w << endl;
					      return IncLen(); }
  GnuPlotData& AddValue(int    v, int    w) { data << v << " " << w << endl;
					      return IncLen(); }

//   void Style(const char *s) { CopyString(style, s); }
//   const char *Style() { return style ? style : ""; }

  static void Keep(bool k) { keep = k; }

  ofstream     data;
  const char        *name;
  const char        *text;
  const char        *style;
  GnuPlot     *gnuplot;
  const IntVector        *ivec;
  const FloatVector      *fvec;
  const FloatPointVector *pvec;
  IntVector        *ownivec;
  FloatVector      *ownfvec;
  FloatPointVector *ownpvec;
  int length, hide;

  static bool keep;
};

typedef ListOf<GnuPlotData> GnuPlotDataList;

//- GnuPlot
class GnuPlot : public Simple {
public:
  /// old constructor
  GnuPlot(const char *t = NULL) { 
    list<string> l;
    if (t)
      l.push_back(t);
    Initialize(l);
  }

  ///
  GnuPlot(const list<string>& l) { Initialize(l); }

  ///
  GnuPlot(const GnuPlot& gp);

  ///
  virtual ~GnuPlot();

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "GnuPlot", NULL }; return n; }

//  GnuPlot *Copy() const { return NULL; }
  int Write(ostream&) { return TRUE; }

  GnuPlotData& operator[](int i) const { return Data(i); }

  GnuPlotData& Data(int i) const {
    if (i<0 || i>=datalist.Nitems()) {
      cerr << "GnuPlot::Data(" << i << ") out of bounds" << endl;
      abort();
    }
    return datalist[i];
  }

  int Nitems() const { return datalist.Nitems(); }

  GnuPlot& Refresh();

  GnuPlotData *AddPlot(int, const char* = NULL, const char* = NULL);
  GnuPlotData *AddPlot(const IntVector&, const char* = NULL,
		       const char* = NULL);
  GnuPlotData *AddPlot(const FloatVector&, const char* = NULL,
		       const char* = NULL);
  GnuPlotData *AddPlot(const FloatPointVector&, const char* = NULL,
		       const char* = NULL);

  GnuPlotData *AddVector(const IntVector*, const char*, const char* = NULL);
  GnuPlotData *AddVector(const FloatVector*, const char*, const char* = NULL);
  GnuPlotData *AddVector(const FloatPointVector*, const char*, const char* = NULL);

  GnuPlotData *AddIntVector(const char*, const char* = NULL);
  GnuPlotData *AddFloatVector(const char*, const char* = NULL);
  GnuPlotData *AddFloatPointVector(const char*, const char* = NULL);

  GnuPlotData *AddFloatPointVector(const FloatVector&, const FloatVector&,
				   const char* = NULL, const char* = NULL);

  GnuPlot& Delete(GnuPlotData*);

  int CrossConnections() const { return cross_connections; }
  void CrossConnections(int c) { cross_connections = c; Refresh(); }

  void Plot(const StatQuadVector&);
  void PlotTrainTest(const StatQuadVector&);
  void PlotRejectError(const StatQuadVector&);

  void Plot(const StatQuadVector *s = NULL) {
    if (s) { sqv = s; }
    if (sqv) { Plot(*sqv); }
  }

  void PlotTrainTest(const StatQuadVector *s = NULL) {
    if (s) { sqv = s; }
    if (sqv) { PlotTrainTest(*sqv); }
  }

  void PlotRejectError(const StatQuadVector *s = NULL) {
    if (s) { sqv = s; }
    if (sqv) { PlotRejectError(*sqv); }
  }

  void RepeatPreviousPlot();

  void PlotOneVanilla(const StatQuadVector&, int, int);
  void Plot(const ListOf<StatQuadVector>&);

  GnuPlot& Range(const char *r) { CopyString(range, r); return *this; }
  const char *Range() const { return range; }

  static const char *LabelText(const char*, int, const char*, int);

//  GnuPlot& ();
//  GnuPlot& ();

  int number;
  int dnum;

  enum PlotType {
    NoPlot, Vanilla, TrainTest, RejectError
  };

  static int HasTrainError(const StatQuadVector&);
  static int HasTestError(const StatQuadVector&);
  static int HasCrossValidationError(const StatQuadVector&);
  static int HasReject(const StatQuadVector&);

  static void Debug(int r) { debug = r; }
  static void IgnorePipeSignal(int = 0, int = 0);

  void DisableRefresh()  { SetRefresh(FALSE); }
  void EnableRefresh()   { SetRefresh(TRUE);  }
  void SetRefresh(int r) { do_refresh = r;    }
  int  GetRefresh() const { return do_refresh; }

protected:
  GnuPlot& OpenPipe();
  void Initialize(const list<string>&);
  void MakeCrossConnections();

  static int tot_number;
  static int debug;

  // ofstream ctrl;
  FILE    *pipe;
  char    *range;
  GnuPlotDataList datalist;
  int cross_connections;
  ofstream ccos;
  char *ccname;

  list<string> cmds;
  // const char *cmd_line;

  const StatQuadVector *sqv;
  enum PlotType prev_plot;

  void ToCtrl(const char*, const char* = NULL, const char* = NULL);

  void ToCtrl(const string& s) { ToCtrl(s.c_str()); }

  int do_refresh;
};

} // namespace simple
#endif // _GNUPLOT_H_

