// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: GnuPlot.C,v 1.13 2009/11/20 20:48:15 jorma Exp $

#include <GnuPlot.h>

#include <signal.h>

#include <map>

namespace simple {

int  GnuPlot::debug = FALSE;
int  GnuPlot::tot_number = 0;
bool GnuPlotData::keep = false;

///////////////////////////////////////////////////////////////////////////

GnuPlotData::GnuPlotData(GnuPlot *gp, int open,
			 const char *t, const char *s) {
  char tmp[100];
  long pid = getpid();
  sprintf(tmp, "/var/tmp/gnuplot%ld.%d.%d", pid, gp->number, gp->dnum++);
  name = CopyString(tmp);
  text  = CopyString(t);
  style = CopyString(s);
  gnuplot = gp;
  ivec = NULL;
  fvec = NULL;
  pvec = NULL;
  ownivec = NULL;
  ownfvec = NULL;
  ownpvec = NULL;
  hide = FALSE;
  length = 0;

  if (open)
    data.open(name);
}

///////////////////////////////////////////////////////////////////////////

void GnuPlotData::Dump(Simple::DumpMode /*dt*/, ostream& os) const {
  os << Bold("GnuPlotData ") << (void*)this
     << " gnuplot="          << (void*)gnuplot
     << " length="           << length
     << " hide="             << hide
     << " name="             << ShowString(name)
     << " text="             << ShowString(text)
     << " style="            << ShowString(style)
     << " ivec="             << (void*)ivec
     << " fvec="             << (void*)fvec
     << " pvec="             << (void*)pvec
     << " ownivec="          << (void*)ownivec
     << " ownfvec="          << (void*)ownfvec
     << " ownpvec="          << (void*)ownpvec
     << endl;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData& GnuPlotData::Refresh() {
  if (!ivec && !fvec && !pvec)
    return *this;

  data.close();
  data.clear();
  if (!keep)
    unlink(name);

  length = 0;
  int n = Length();
  if (n) {
    data.open(name);

    for (int i=0; i<n; i++)
      if (pvec)
	AddValue((*pvec)[i]);
      else if (fvec)
	AddValue((*fvec)[i]);
      else
	AddValue((*ivec)[i]);
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////

GnuPlot::GnuPlot(const GnuPlot& gp) : Simple(gp) {
  Initialize(gp.cmds);
  Range(gp.range);

  cross_connections = gp.cross_connections;
  ccname            = gp.ccname;
  dnum              = gp.dnum;
  number            = gp.number;
  do_refresh        = gp.do_refresh;
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::Initialize(const list<string>& l) {
  if (!tot_number)
    IgnorePipeSignal();

  pipe  = NULL;
  range = NULL;
  cmds = l;
  
  cross_connections = FALSE;
  ccname = NULL;
  dnum = 0;
  number = tot_number++;

  sqv = NULL;
  prev_plot = NoPlot;

  do_refresh = TRUE;
}

///////////////////////////////////////////////////////////////////////////

GnuPlot::~GnuPlot() {
  delete range;

  if (pipe)
    pclose(pipe);

  if (ccname)
    unlink(ccname);

  delete ccname;
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::Dump(Simple::DumpMode dt, ostream& os) const {
  os << Bold("GnuPlot ")      << (void*)this
     << " debug="             << debug
     << " tot_number="        << tot_number
     << " pipe="              << (void*)pipe
     << " sqv="               << (void*)sqv
     << " prev_plot="         << (int)prev_plot
     << " do_refresh="        << do_refresh
     << " cross_connections=" << cross_connections
     << " ccname="            << ShowString(ccname)
     << " range="             << ShowString(range)
     << " datalist.Nitems()=" << datalist.Nitems()
     << endl;

  if (dt&DumpRecursive)
    for (int i=0; i<datalist.Nitems(); i++) {
      os << "datalist[" << i << "]=";
      datalist[i].Dump(dt, os);
    }
}

///////////////////////////////////////////////////////////////////////////

GnuPlot& GnuPlot::OpenPipe() {
  typedef map<string,pair<string,string> > ttype_t;
  ttype_t ttype;
  if (ttype.empty()) {
    ttype["postscript"] = make_pair<string,string>("eps", "eps");
    ttype["eepic"]      = make_pair<string,string>("eepic", "");
    ttype["pdf"]        = make_pair<string,string>("pdf", "");
    ttype["png"]        = make_pair<string,string>("png", "");
  }

  stringstream cmd, script;
  cmd << "gnuplot ";

  for (list<string>::const_iterator i=cmds.begin(); i!=cmds.end(); i++) {
    ttype_t::const_iterator term = ttype.end();
    stringstream output;
    for (ttype_t::const_iterator j=ttype.begin();
	 term==ttype.end() && j!=ttype.end(); j++)
      if (*i==j->first) {
	term = j;
	output << "gnuplot" << getpid() << "-" << number << "."
	       << j->second.first;

      } else if (i->find(j->first)==0 && i->size()>j->first.size()+1 &&
		 (*i)[j->first.size()]=='=') {
	term = j;
	output << i->substr(j->first.size()+1);
      }

    if (term!=ttype.end()) {
      script << "set term " << term->first << " " << term->second.second
	     << endl
	     << "set output '" << output.str() << "'" << endl;
      continue;
    }

    if ((*i)[0]=='-') {
      cmd << *i << " ";
      continue;
    }

    script << *i << endl;
  }

  cmd << " 2> /dev/null";
  string command = cmd.str();
  if (debug)
    cerr << "GnuPlot #" << number << ": " << command << endl;
  
  pipe = popen(command.c_str(), "w");
  if (!pipe)
    cerr << "GnuPlot::OpenPipe() popen failed" << endl;
  else {
    // ctrl.rdbuf()->attach(fileno(pipe));
    if (cmd)
      ToCtrl(script.str());
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////

GnuPlot& GnuPlot::Refresh() {
  if (!GetRefresh())
    return *this;

  int i;

  for (i=0; i<datalist.Nitems(); i++)
    if (!datalist[i].Hide() && datalist[i].Length())
      break;

  if (i<datalist.Nitems()) {
    if (!pipe)
      OpenPipe();

    int first = TRUE;
    
    for (i=0; i<datalist.Nitems(); i++) {
      if (datalist[i].Hide() || !datalist[i].Length())
	continue;

      if (first) {
	first = FALSE;
	ToCtrl("plot ");
	if (range)
	  ToCtrl(range, " ");

      } else
	ToCtrl("replot ");

      ToCtrl("'", datalist[i].name, "'");
      if (datalist[i].text)
	ToCtrl(" title '", datalist[i].text, "'");
      
      string style = "linespoints";
      if (datalist[i].style)
	style = datalist[i].style;
      style += "\n";

      ToCtrl(" w ", style.c_str());
      datalist[i].Refresh();
    }

    if (cross_connections) {
      MakeCrossConnections();
      ToCtrl("replot '", ccname, "' title ''\n");
    }
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddPlot(int open, const char *txt, const char *sty) {
  GnuPlotData *ptr = new GnuPlotData(this, open, txt, sty);
  datalist.Append(ptr);
  return ptr;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddPlot(const FloatVector& vec, const char *txt,
			      const char *sty) {
  GnuPlotData *ptr = AddPlot(TRUE, txt, sty);

  for (int i=0; i<vec.Length(); i++)
    ptr->AddValue(vec[i]);

  Refresh();

  return ptr;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddPlot(const IntVector& vec, const char *txt,
			      const char *sty) {
  GnuPlotData *ptr = AddPlot(TRUE, txt, sty);

  for (int i=0; i<vec.Length(); i++)
    ptr->AddValue(vec[i]);

  Refresh();

  return ptr;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddPlot(const FloatPointVector& vec, const char *txt,
			      const char *sty) {
  GnuPlotData *ptr = AddPlot(TRUE, txt, sty);

  for (int i=0; i<vec.Length(); i++)
    ptr->AddValue(vec[i]);

  Refresh();

  return ptr;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddVector(const FloatPointVector *vec, const char *txt,
				const char *sty) {
  GnuPlotData *ptr = AddPlot(FALSE, txt, sty);

  if (!vec)
    vec = ptr->ownpvec = new FloatPointVector;

  ptr->pvec = vec;
  Refresh();

  return ptr;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddVector(const FloatVector *vec, const char *txt,
				const char *sty) {
  GnuPlotData *ptr = AddPlot(FALSE, txt, sty);

  if (!vec)
    vec = ptr->ownfvec = new FloatVector;

  ptr->fvec = vec;
  Refresh();

  return ptr;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddVector(const IntVector *vec, const char *txt,
				const char *sty) {
  GnuPlotData *ptr = AddPlot(FALSE, txt, sty);

  if (!vec)
    vec = ptr->ownivec = new IntVector;

  ptr->ivec = vec;
  Refresh();

  return ptr;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddFloatPointVector(const FloatVector& xvec, 
					  const FloatVector& yvec,
					  const char *txt, const char *sty) {
  GnuPlotData *gpd = AddFloatPointVector(txt, sty);
  
  if (!xvec.CheckDisparity(yvec, TRUE, "GnuPlot::AddFloatPointVector"))
    return gpd;
  
  for (int i=0; i<xvec.Length(); i++)
    gpd->Set(i, FloatPoint(xvec[i], yvec[i]));
  
  Refresh();

  return gpd;
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddFloatPointVector(const char *txt, const char *sty) {
  return AddVector((FloatPointVector*)NULL, txt, sty);
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddFloatVector(const char *txt, const char *sty) {
  return AddVector((FloatVector*)NULL, txt, sty);
}

///////////////////////////////////////////////////////////////////////////

GnuPlotData *GnuPlot::AddIntVector(const char *txt, const char *sty) {
  return AddVector((IntVector*)NULL, txt, sty);
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::MakeCrossConnections() {
  if (ccname)
    unlink(ccname);

  char tmp[100];
  sprintf(tmp, "/var/tmp/gnuplot%ld.%d", (long)getpid(), number);
  CopyString(ccname, tmp);

  ccos.close();
  ccos.clear();
  ccos.open(ccname);

  for (int i=0; ; i++) {
    int found = FALSE;

    for (int j=0; j<datalist.Nitems(); j++) {
      GnuPlotData *data = datalist.Get(j);
      if (data->Hide())
	continue;

      float x = i, y;

      if (data->pvec && i<data->pvec->Length()) {
	x = (*data->pvec)[i].X();
	y = (*data->pvec)[i].Y();
      } else if (data->fvec && i<data->fvec->Length())
	y = (*data->fvec)[i];
      else if (data->ivec && i<data->ivec->Length())
	y = (*data->ivec)[i];
      else continue;

      ccos << x << " " << y << endl;
      found = TRUE;
    }
    if (!found)
      break;

    ccos << endl;
  }
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::PlotOneVanilla(const StatQuadVector& q, int many, int b) {
  if (Nitems()<=b) {
    AddFloatPointVector(LabelText("train reject", many, q.Label(), b/6));
    AddFloatPointVector(LabelText("train error",  many, q.Label(), b/6));
    AddFloatPointVector(LabelText("test reject",  many, q.Label(), b/6));
    AddFloatPointVector(LabelText("test error",   many, q.Label(), b/6));
    AddFloatPointVector(LabelText("cv reject",    many, q.Label(), b/6));
    AddFloatPointVector(LabelText("cv error",     many, q.Label(), b/6));
  }

  for (int ii=b; ii<b+6; ii++)
    Data(ii).ownpvec->Length(0);

  for (int j=0, k=0; j<q.Length(); j++)
    if (q[j].IsPartiallySet()) {
      for (int i=b; i<b+6; i++)
	Data(i).Set(k, FloatPoint(StatQuad::HasX(q)?q[j].X():k, q[j][i-b]));
      k++;
    }

  int r = HasReject(q),    t = HasTrainError(q);
  int e = HasTestError(q), c = HasCrossValidationError(q);

  Data(b  ).Hide(!(t&&r));
  Data(b+1).Hide(!t);
  Data(b+2).Hide(!(e&&r));
  Data(b+3).Hide(!e);
  Data(b+4).Hide(!(c&&r));
  Data(b+5).Hide(!c);
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::Plot(const StatQuadVector& q) {
  PlotOneVanilla(q, FALSE, 0);

  if (Nitems()!=6) {
    cerr << "GnuPlot::Plot # of plots should be 6 " << Nitems() << endl;
  }

  Refresh();

  prev_plot = Vanilla;
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::Plot(const ListOf<StatQuadVector>& ql) {
  int r = GetRefresh();
  DisableRefresh();

  for (int e=0; e<ql.Nitems(); e++)
    PlotOneVanilla(ql[e], TRUE, 6*e);

  if (Nitems()!=6*ql.Nitems()) {
    cerr << "GnuPlot::Plot # of plots should be " << 6*ql.Nitems() << " "
	 << Nitems() << endl;
  }

  SetRefresh(r);

  Refresh();

  prev_plot = Vanilla;
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::PlotTrainTest(const StatQuadVector& q) {
  if (!Nitems()) {
    AddFloatPointVector("test(train) reject");
    AddFloatPointVector("test(train) error");
    AddFloatPointVector("test(cv) reject");
    AddFloatPointVector("test(cv) error");
    CrossConnections(TRUE);
  }
  if (Nitems()!=4) {
    cerr << "GnuPlot::PlotTrainTest # of plots should be 4 "
	 << Nitems() << endl;
  }

  for (int ii=0; ii<4; ii++)
    Data(ii).ownpvec->Length(0);

  for (int j=0, k=0; j<q.Length(); j++)
    if (q[j].IsPartiallySet()) {
      Data(0).Set(k, q[j].TrainTestReject());
      Data(1).Set(k, q[j].TrainTestError());
      Data(2).Set(k, q[j].CrossValidationTestReject());
      Data(3).Set(k, q[j].CrossValidationTestError());
      k++;
    }

  int r = HasReject(q),    t = HasTrainError(q);
  int e = HasTestError(q), c = HasCrossValidationError(q);

  Data(0).Hide(!(t&&e&&r));
  Data(1).Hide(!(t&&e));
  Data(2).Hide(!(c&&e&&r));
  Data(3).Hide(!(c&&e));

  Refresh();

  prev_plot = TrainTest;
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::PlotRejectError(const StatQuadVector& q) {
  if (!Nitems()) {
    AddFloatPointVector("train");
    AddFloatPointVector("test");
    AddFloatPointVector("cv");
    CrossConnections(TRUE);
  }
  if (Nitems()!=3) {
    cerr << "GnuPlot::PloRejectError # of plots should be 2 "
	 << Nitems() << endl;
  }

  for (int ii=0; ii<3; ii++)
    Data(ii).ownpvec->Length(0);

  for (int j=0, k=0; j<q.Length(); j++)
    if (!q[j].IsPartiallySet()) {
      Data(0).Set(k, q[j].TrainRejectError());
      Data(1).Set(k, q[j].TestRejectError());
      Data(2).Set(k, q[j].CrossValidationRejectError());
      k++;
    }

  int r = HasReject(q),    t = HasTrainError(q);
  int e = HasTestError(q), c = HasCrossValidationError(q);

  Data(0).Hide(!(t&&r));
  Data(1).Hide(!(e&&r));
  Data(2).Hide(!(c&&r));

  Refresh();

  prev_plot = RejectError;
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::RepeatPreviousPlot() {
  switch (prev_plot) {
  case Vanilla:
    Plot();
    break;

  case TrainTest:
    PlotTrainTest();
    break;

  case RejectError:
    PlotRejectError();
    break;

  case NoPlot:
  default:
    break;
  }
}

///////////////////////////////////////////////////////////////////////////

int GnuPlot::HasTrainError(const StatQuadVector& q) {
  for (int j=0; j<q.Length(); j++)
    if (q[j].TrainError()) return TRUE;

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////

int GnuPlot::HasTestError(const StatQuadVector& q) {
  for (int j=0; j<q.Length(); j++)
    if (q[j].TestError()) return TRUE;

  return FALSE;    
}

///////////////////////////////////////////////////////////////////////////

int GnuPlot::HasCrossValidationError(const StatQuadVector& q) {
  for (int j=0; j<q.Length(); j++)
    if (q[j].CrossValidationError()) return TRUE;

  return FALSE;    
}

///////////////////////////////////////////////////////////////////////////

int GnuPlot::HasReject(const StatQuadVector& q) {
  for (int j=0; j<q.Length(); j++)
    if (q[j].TrainReject() || q[j].TestReject() ||
	q[j].CrossValidationReject()) return TRUE;
    
  return FALSE;    
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::ToCtrl(const char *s1, const char *s2, const char *s3) {
  static int endl_done = TRUE, last_number = MAXINT;

  if (debug && (endl_done || number!=last_number)) {
    cerr << "GnuPlot #" << number << ": ";
    endl_done = FALSE;
  }

  stringstream ctrl;

  ctrl << s1;
  if (debug)
    cerr << s1;

  if (s2) {
    ctrl << s2;
    if (debug)
      cerr << s2;
  }

  if (s3) {
    ctrl << s3;
    if (debug)
      cerr << s3;
  }

  ctrl << flush;

  if (debug && (s1[strlen(s1)-1]=='\n' || (s2 && s2[strlen(s2)-1]=='\n') ||
		(s3 && s3[strlen(s3)-1]=='\n')))
    endl_done = TRUE;

  last_number = number;

  string str = ctrl.str();

  int r = fwrite(str.c_str(), str.size(), 1, pipe)!=str.size();
  r += 5;
    
  fflush(pipe);
}

///////////////////////////////////////////////////////////////////////////

const char *GnuPlot::LabelText(const char *sf, int t, const char *pr, int b) {
  static char lab[1000];
  if (t)
    if (pr)
      sprintf(lab, "%s : ", pr);
    else
      sprintf(lab, "%d : ", b);
  else
    *lab = 0;

  strcpy(lab+strlen(lab), sf);

  return lab;
}

///////////////////////////////////////////////////////////////////////////

void GnuPlot::IgnorePipeSignal(int, int) {
  #ifdef SIGPIPE
  signal(SIGPIPE, (CFP_signal)IgnorePipeSignal);
  #endif // SIGPIPE
}

///////////////////////////////////////////////////////////////////////////

// int GnuPlot::() {
// }

///////////////////////////////////////////////////////////////////////////

// int GnuPlot::() {
// }

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

} // namespace simple

