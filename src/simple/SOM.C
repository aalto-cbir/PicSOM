// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: SOM.C,v 1.19 2010/07/22 21:43:14 jorma Exp $

#ifndef _SOM_C_
#define _SOM_C_

#include <SOM.h>

#include <sstream>
#include <set>

namespace simple {

///////////////////////////////////////////////////////////////////////////////

SOM::SOM(int w, int h, int l, TopologyType t, double a, double r) : 
  Planar(w, h, t) {
  FloatVector v(l);
  Initialize(v, a, r);
}

///////////////////////////////////////////////////////////////////////////////

SOM::SOM(int w, int h,const FloatVector& v, TopologyType t,double a, double r)
  : Planar(w, h, t) {
  Initialize(v, a, r);
}

///////////////////////////////////////////////////////////////////////////////

SOM::SOM(istream& is) : Planar(0, 0) {
  FloatVector vec;
  Initialize(vec);
  Read(is);
}

///////////////////////////////////////////////////////////////////////////////

SOM::SOM(const char* name) : Planar(0, 0) {
  FloatVector vec;
  Initialize(vec);
  if (name)
    Read(name);
}

///////////////////////////////////////////////////////////////////////////////

SOM::SOM(const SOM& som) : SourceOf<FloatVector>(som),
			   Classifier(som),
			   Planar(som) {
  FloatVector tmp;
  const FloatVector& v = som.Units() ? *som.Unit(0) : tmp;

  Size(som.Size());

  Initialize(v, som.alpha, som.radius);
  round = som.round;

  // voting etc. should be handled, too (but is not needed, yet)

  SetMatchMethod(som.GetMatchMethod());

  alpha  = som.alpha;
  radius = som.radius;

//   SetPreProcess( this, som.GetPreProcess());
//   SetPostProcess(this, som.GetPostProcess());

  for (int i=0; i<Units(); i++)
    *Unit(i) = *som.Unit(i);
}

///////////////////////////////////////////////////////////////////////////////

SOM::~SOM() {
  delete progress_text;
  //  votelist.Delete();

//   if (monitor_filename)
//     unlink(monitor_filename);
//   delete monitor_filename;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::Initialize(const FloatVector& v, double a, double r) {
  radius_base = DEFAULT_RADIUS_BASE;
  do_eveninit = FALSE;
  round  = 0;

  mean_distance = rms_distance = -1;

  Voting(MajorityVoting);
  Monitoring(FALSE);  
//  monitor_filename = NULL;

  errsum = 0;

  alpha.SetExponential(DEFAULT_ALPHA_DECAY).Keep(a);

  SetMatchMethodEuclidean();

  // SetPreProcess();
  // SetPostProcess();

  FloatVectorSet::Append(new FloatVector(v));

  radius = r; // SetDefaultRadius(); // done by Resize();

  Resize(0, 0);

  hit_types[0] = hit_types[1] = hit_types[2] = -1;

  progress_interval = 0;
  progress_text = NULL;

  umatrix_dist_limit = -1;
  save_umatrix = true;

  average_quantisation_error = -1;
  NKernel(n_k_rectangle);
}

///////////////////////////////////////////////////////////////////////////////

void SOM::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&DumpShort || type&DumpLong)
    os << Bold("SOM ") 		<< (void*)this
       << " width="    		<< width
       << " height="   		<< height
       << " topology="          << TopologyName(Topology()) 
       << " alpha="    		<< alpha 
       << " radius="   		<< radius
       << " round="    		<< round
       << " voting="   		<< (int)Voting()
       << " progress_text="     << ShowString(progress_text)
       << " progress_interval=" << progress_interval
       << " hit_types[]="
       << hit_types[0] << "," << hit_types[1] << "," << hit_types[2]
       << endl;

  if (type&DumpLong) {
    for (int i=0; i<Units(); i++)
      Unit(i)->Dump(DumpRecursiveLong, os);
    DumpVotes(os);
  }
}

///////////////////////////////////////////////////////////////////////////////

const char *SOM::Identity(const char*) {
  char tmp[1000];
  sprintf(tmp, "som-%s-%s", FileName(), Configuration());
  CopyString(identity, tmp);

  return identity;
}

///////////////////////////////////////////////////////////////////////////////

int SOM::Configuration(const char *str, const char**) {
  int found = FALSE;
  const char *base = str;

  for (; *str; str++) {
    if (*str==' ' || *str=='\t')
      continue;
    
    int n, d1, d2;

    if (sscanf(str, "size%dx%d%n", &d1, &d2, &n)==2) {
      Size(d1, d2);
      str += n-1;
      found = TRUE;

    } else {
      const char *retstr;
      if (!Classifier::Configuration(str, &retstr)) {
	cerr << "SOM::Configuration(" << base << ") error in: "
	     << str << endl;
	return FALSE;
      }
      str = retstr;
      found = TRUE;
      if (!str)
	break;
    }
  }

  return found;
}

///////////////////////////////////////////////////////////////////////////////

const char *SOM::Configuration() const {
  static char conf[1000];
  *conf = 0;

  sprintf(conf+strlen(conf), "size%dx%d", Width(), Height());
  sprintf(conf+strlen(conf), "iter%d", DefaultPerformanceIterations());
  sprintf(conf+strlen(conf), "rep%d", DefaultPerformanceRepetitions());
  sprintf(conf+strlen(conf), "silent%d", Silent());

  return conf;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::SetDefaultRadius() {
  if (double(radius)==MAXFLOAT) {
    float r = (width>height ? width : height)/2;

    if (r)
      radius.SetExponential(DEFAULT_RADIUS_DECAY).SetBase(radius_base).Keep(r);
      //radius.SetExponential(DEFAULT_RADIUS_DECAY).SetBase(DEFAULT_RADIUS_BASE).
	//Keep(r);
    // cout << "radius_base=" << radius_base << " ";
    // radius.Dump();
    // SetRadius(r);
    // SetRadiusDecay(DEFAULT_RADIUS_DECAY);
    // SetRadiusBase( DEFAULT_RADIUS_BASE);
  }
}

///////////////////////////////////////////////////////////////////////////////

const char *SOM::ClassifyVector(const FloatVector& vec, float *d) {
  int u = Recognize(vec, d);
  if (!IndexOK(u)) {
    cerr << "SOM::ClassifyVector: Recognize failed" << endl;
    return NULL;
  }

  if (hit_types[0]>=0)
    hit_types[PointType(u)]++;

  return Get(u)->Label();
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::AdaptVector(const FloatVector& vec) {
  Learn(vec);
  return true;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::AdaptEpochDone(FloatVectorSource& set) {
  // cout << "SOM::AdaptEpochDone" << endl;
  DoLabeling(set);

  cout << "SOM::AdaptEpochDone " << Silent() << endl;

  if (Silent()<2) {
    Display();
    Dump(DumpRecursive);
  }
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DoLabeling(FloatVectorSource& set) {
  ClearLabels();
  EmptyVotes();
  set.First();

  VotingMethod method = VotingMethod(voting_method&~VotingDuringTraining);
  if (method==NoVoting)
    return;

  for (const FloatVector *vec; (vec = set.Next());) {
    float d;
    int u = BestMatchingUnit(*vec, &d);
    AddEitherVote(u, vec->Label(), d);
  }
  
  ConditionallyMajorityVoteLabels();
}

///////////////////////////////////////////////////////////////////////////////

int SOM::BestMatchingUnit(const FloatVector& v, float *err) const {
  cout << "SOM::BestMatchingUnit()" << endl;

  IntVector nn(1);
  FloatVector ddd;
  NearestNeighbors(v, nn, ddd);

  static int first = TRUE;
  if (err && first) {
    ShowError("SOM::BestMatchingUnit err not implemented.");
    first = FALSE;
  }

  return nn[0];

//   switch (matchmethod) {
//   case euclidean:
//     return v->MinimumDistance(*(FloatVectorSet*)this, err);
// 
//   case dotproduct:
//     return v->MaximumDotProduct(*(FloatVectorSet*)this, err, FALSE);
// 
//   case absolutedotproduct:
//     return v->MaximumDotProduct(*(FloatVectorSet*)this, err, TRUE);
//   }
// 
//   return -1;
}

///////////////////////////////////////////////////////////////////////////////

IntPointVector SOM::BestMatchingPoints(const FloatVectorSet& set) const {
  IntPointVector res(set.Nitems());
  
  for (int i=0; i<set.Nitems(); i++)
    res[i] = XYCoord(BestMatchingUnit(set[i]));
  
  return res;
}

///////////////////////////////////////////////////////////////////////////////

IntVector SOM::SolveNeighborhood(int i, double r) const {
#ifdef CLASSIFIER_USE_PTHREADS
  bool do_tictac = !batchtrain || !BatchTrainThreads();
#else
  bool do_tictac = true;
#endif // CLASSIFIER_USE_PTHREADS

  if (do_tictac)
    Tic("SOM::SolveNeighborhood()");

  if (r==MAXFLOAT) r = Radius();

  const int w1 = Width()-1, h1 = Height()-1;
  const int x0 = XCoord(i), y0 = YCoord(i), x1 = (int)(x0+r), y1 = (int)(y0+r);
  const int xs = x0>r ? (int)(x0-r) : 0, ys = y0>r ? (int)(y0-r) : 0;
  const int xe = x1<w1 ? x1 : w1, ye = y1<h1 ? y1 : h1;

  const double r2 = r*r;

  const int l = (Topology()==Torus) ? (2*(int)r+1)*(2*(int)r+1) : (xe-xs+1)*(ye-ys+1);
  
  IntVector idx(l);

  switch(Topology()){
  case Rectangular:
    {
      //      cout << "SOM::SolveNeighborhood(), topol=Rectangular" << endl;
      //cout << "(x0,y0) = (" <<x0<<","<<y0<<")" << endl;

      int ii = 0;
      for (int x=xs; x<=xe; x++)
	for (int y=ys; y<=ye; y++) {
	  const double dx = x-x0, dy = y-y0;
	  if (dx*dx+dy*dy <= r2)
	    idx[ii++] = ToIndex(x, y);
	  //idx.Push(ToIndex(x, y));
	}
      idx.Lengthen(ii);
    }
    break;

  case Torus:
    {

      //cout << "SOM::SolveNeighborhood(), topol=Torus r="<<r<< endl;
      //cout << "(x0,y0) = (" <<x0<<","<<y0<<")" << endl;

      set<int> nbr;

      const int w=Width(),h=Height(); 
      for (int x=0;x<=r;x++)
	for (int y=0; y<=r; y++) {
	  if (x*x+y*y <= r2){
	    nbr.insert(ToIndex((x0+x+w)%w, (y0+y+h)%h));
	    nbr.insert(ToIndex((x0+x+w)%w, (y0-y+h)%h));
	    nbr.insert(ToIndex((x0-x+w)%w, (y0+y+h)%h));
	    nbr.insert(ToIndex((x0-x+w)%w, (y0-y+h)%h));
	  }

	  //idx.Push(ToIndex(x, y));
	}
      idx.Lengthen(nbr.size());
      int ii=0;
      for(set<int>::const_iterator it=nbr.begin(); it != nbr.end(); it++)
	idx[ii++]=*it;

      //cout << "Neighborhood: " << endl;
      //for(int iii=0;iii<ii;iii++){
      //	cout << " (" <<XCoord(idx[iii])<<","<<YCoord(idx[iii]);
	
      //      }
      //cout << endl;

    }
    break;
    
  default:
    ShowError("SOM::SolveNeighborhood: unknown topology");
  }

  if (do_tictac)
    Tac("SOM::SolveNeighborhood()");

  return idx;
}

///////////////////////////////////////////////////////////////////////////////
// By Philip Prentis

double SOM::NeighbourhoodFunction(int i, int p, double r) const {
#ifdef CLASSIFIER_USE_PTHREADS
  bool do_tictac = !batchtrain || !BatchTrainThreads();
#else
  bool do_tictac = true;
#endif // CLASSIFIER_USE_PTHREADS

  if (do_tictac)
    Tic("SOM::NeighbourhoodFunction()");

  if (r==MAXFLOAT) r = Radius();

  double dist = 0;
  double val = 0;

  const int w1 = Width(), h1 = Height();

  double x1,x2,x3,xdist;
  double y1,y2,y3,ydist;

  switch(Topology()){
  case Rectangular:
    xdist = XCoord(i)-XCoord(p); ydist=YCoord(i)-YCoord(p);
    dist = sqrt((xdist*xdist)+(ydist*ydist));
    break;
  case Torus:
    x1 = XCoord(i)-XCoord(p); x2 = w1+XCoord(i)-XCoord(p); x3 = w1-XCoord(i)+XCoord(p);
    x1 *= x1; x2 *= x2; x3 *= x3;
    xdist = x1<x2 ? (x1<x3 ? x1 : x3) : (x2<x3 ? x2 : x3);
    y1 = YCoord(i)-YCoord(p); y2 = h1+YCoord(i)-YCoord(p); y3 = h1-YCoord(i)+YCoord(p);
    y1 *= y1; y2 *= y2; y3 *= y3;
    ydist = y1<y2 ? (y1<y3 ? y1 : y3) : (y2<y3 ? y2 : y3);
    dist = sqrt(xdist+ydist);
    break;
  default:
    ShowError("SOM::NeighbourhoodFunction: unknown topology");
  }


  switch (NKernel()) {
  case n_k_rectangle:
    val = dist<=r ? 1 : 0;
    break;
  case n_k_triangle:
    val = r==-1 ? 0 : dist<=r ? 1-(dist/(r+1)) : 0;
    break;
  default:
    ShowError("SOM::NeighbourhoodFunction: unknown neighbourhood kernel");
  }

  if (do_tictac)
    Tac("SOM::NeighbourhoodFunction()");

  return val;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::AdaptVote(const IntVector& bmu, const FloatVector& d,
		    const FloatVectorSet& labset) {
  int nn = TrainingSet() ? TrainingSet()->Nitems() : 0;
  if (!nn)
    return ShowError("SOM::AdaptVote() training set size unknown");

  int nb = bmu.Length();
  if (nn>nb || d.Length()!=nb || labset.Nitems()!=nb) {
    char tmp[1000];
    sprintf(tmp, "nn=%d bmu.Length()=%d d.Length()=%d labset.Nitems()=%d",
	    nn, bmu.Length(), d.Length(), labset.Nitems());
    return ShowError("SOM::AdaptVote() vector size mismatch : ", tmp);
  }

  Tic("SOM::AdaptVote()");

  cout << PossiblyTimeStamp() << "SOM::AdaptVote(), nn=" << nn
       << ", nb=" << nb << endl;

  ClearLabels();  // OBS! THIS IS THE PLACE WHERE THE BUG WAS!!!
  EmptyVotes();
  for (int i=0; i<nb; i++) 
    if (bmu[i]!=-1)
      AddEitherVote(bmu[i], labset[i].Label(), d[i]);

  if (voting_method&NearestVoting) {
    int sum = 0;
    for (int u=0; u<nearest_votes.Nitems(); u++)
      sum += (int)nearest_votes[u][1];
    cout << PossiblyTimeStamp()
	 << "SOM::AdaptVote(), #nearest_votes="<< sum << endl;
  }

  ConditionallyMajorityVoteLabels();
  if (Silent()<2) {
    if (Width()<=20)
      Display();
    Dump(DumpRecursiveShort);
  }

  Tac("SOM::AdaptVote()");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DoAdapt(int i, const FloatVector& v, double a, double r) {
  if (r==MAXFLOAT) r = Radius();

  IntVector idx = SolveNeighborhood(i, r);

  Tic("SOM::DoAdapt()");

  if (a==MAXFLOAT) a = Alpha();

  if (DebugAdapt())
    cout << "SOM::DoAdapt() adapting a=" << a << " r=" << r << endl;

  for (int j=0; j<idx.Length(); j++) {
    int p = idx[j];
    FloatVector *u = Unit(p);
    double tmpa;
    if (NKernel() == n_k_rectangle)
      tmpa = a;
    else
      tmpa = a * NeighbourhoodFunction(i, p, r);
    u->MoveTowards(v, tmpa);

    if (DebugAdapt())
      cout << "  p=" << p << " tmpa=" << tmpa << endl;
  }

  Tac("SOM::DoAdapt()");
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DoAdaptOld(int i, const FloatVector& v, double a, double r) {
  Tic("SOM::DoAdapt()");

  if (a==MAXFLOAT) a = Alpha();
  if (r==MAXFLOAT) r = Radius();

  int w1 = Width()-1, h1 = Height()-1;
  int x0 = XCoord(i), y0 = YCoord(i), x1 = (int)(x0+r), y1 = (int)(y0+r);
  int xs = x0>r ? (int)(x0-r) : 0, ys = y0>r ? (int)(y0-r) : 0;
  int xe = x1<w1 ? x1 : w1, ye = y1<h1 ? y1 : h1;

  double r2 = r*r;

  for (int x=xs; x<=xe; x++)
    for (int y=ys; y<=ye; y++) {
      double dx = x-x0, dy = y-y0;

      if (dx*dx+dy*dy <= r2) { 
	// cout << "SOM::DoAdapt " << i << " " << j << endl;

	FloatVector *u = Unit(ToIndex(x, y));
	double tmpa = a;
	//      PreProcess( u,  v, tmpa);
	//Tic("MoveTowards()");
	u->MoveTowards(v, tmpa);
	// Tac("MoveTowards()");
	//      PostProcess(u,  v, tmpa);
      }
    }

  Tac("SOM::DoAdapt()");
}

///////////////////////////////////////////////////////////////////////////////

FloatVectorSet SOM::Neighborhood(int i, double d, IntPoint::Metric m) const {
  FloatVectorSet set;
  IntPoint ip = ToPoint(i);

  for (int j=0; j<Units(); j++){
    IntPoint jp=ToPoint(j);

    if(Topology()==Torus){
      // here we assume that dist is 
      // monotonically increasing fcn of |x_i-x_j|
      // and |y_i - y_j|

      int dx=ip.X()-jp.X();
      int w=Width(),h=Height();

      if(dx>w/2)
	jp += IntPoint(w,0);
      else if(dx<-w/2)
	jp -= IntPoint(w,0);
      
      int dy=ip.Y()-jp.Y();
      if(dy>h/2)
	jp += IntPoint(0,h);
      else if(dy<-h/2)
	jp -= IntPoint(0,h);
    }

    if (ip.Distance(jp, m)<=d)
      set.Append(Unit(j), FALSE);
  }

  return set;
}

///////////////////////////////////////////////////////////////////////////////

double SOM::DistanceSquared(const SOM& som) const {
  if (Units() != som.Units())
    return MAXFLOAT;

  float sum = 0;

  for (int i=0; i<Units(); i++)
    sum += Unit(i)->DistanceSquaredXX(*som.Unit(i));

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

int SOM::Recognize(const FloatVector& v, float *d) {
  int u = BestMatchingUnit(v, d);

  if (u==-1) {
    cerr << "ERROR in SOM::Recognize:" << endl;
    v.Dump(DumpRecursiveLong, cerr);
    Dump(DumpRecursiveLong, cerr);
    return -1;
  }

  DoVoting(u, v, *d);
  // DoStatistics(err);

  return u;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::Learn(const FloatVector& v) {
  int u = Recognize(v);

  if (u!=-1)
    Learn(v, u);
}

///////////////////////////////////////////////////////////////////////////////

void SOM::ShowProgress(const FloatVector& v, const char *txt,
		       const DecayVar *a, const DecayVar *r) {
  if (silent>1 || !progress_interval || v.Number()%progress_interval)
    return;

  char line[10240];
  sprintf(line, "%s %d", txt?txt:"", v.Number());

  if (a) {
    double adbl = *a;
    const char *atxt = (const char*)*a;
    sprintf(line+strlen(line), " alpha=%g (%s)", adbl, atxt);
  }

  if (r) {
    double rdbl = *r;
    const char *rtxt = (const char*)*r;
    sprintf(line+strlen(line), " radius=%g (%s)", rdbl, rtxt);
  }

  size_t i = strlen(line);  // was int
  while (i<100)
    line[i++] = ' ';
  line[i] = 0;

  if (silent==0)
    cout << line << endl;
  else
    cout << line << "\r" << flush;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::Learn(const FloatVector& v, int u) {
  Tic("SOM::Learn():");
  ShowProgress(v, progress_text, &alpha, &radius);
  DoAdapt(u, v);
  DoMonitoring();
  DecayAlpha();
  DecayRadius();
  IncrementRound();
  Tac("SOM::Learn():");
}

///////////////////////////////////////////////////////////////////////////////

void SOM::EmptyVotes() {
  for (int i=0; i<NvotedLabels(); i++)
    VotedLabel(i)->Empty();
  nearest_votes.Delete();
}

///////////////////////////////////////////////////////////////////////////////

void SOM::ReduceVotes(double mul) {
  for (int i=0; i<NvotedLabels(); i++)
    VotedLabel(i)->Reduce(mul);
}

///////////////////////////////////////////////////////////////////////////////

SOM::Vote *SOM::AddVote(int i, const char *l, double d) {
  // Tic("SOM::AddVote()");

  // cout << "SOM::AddVote()" << endl;

  Vote *ptr = GetVotes(l);
  if (!ptr)
    ptr = AddLabel(l);

  ptr->Add(i, 1, d);

  // Tac("SOM::AddVote()");

  return ptr;
}

///////////////////////////////////////////////////////////////////////////////

int SOM::GetVoteIndex(const char *l) const {
  for (int i=0; i<NvotedLabels(); i++)
    if (VotedLabel(i)->LabelsMatch(l))
      return i;

  return -1;
}

///////////////////////////////////////////////////////////////////////////////

SOM::Vote *SOM::Majority(int u) const {
  int j=-1, max = 0;
  float mind = MAXFLOAT;

  for (int i=0; i<NvotedLabels(); i++) {
    const Vote& vote = *VotedLabel(i);
    if (vote.GetCount(u)>max) {
      max = vote.GetCount(u);
      j = i;
      mind = vote.GetDist(u);

    } else if (vote.GetCount(u)==max && vote.GetDist(u)<mind) {
      j = i;
      mind = vote.GetDist(u);
    }
  }

  return j!=-1 ? VotedLabel(j) : NULL;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::ClearLabels() {
  for (int i=0; i<Units(); i++)
    Unit(i)->Label(NULL);
}

///////////////////////////////////////////////////////////////////////////////

void SOM::ConditionallyMajorityVoteLabels(int preserve) {
  if (!(voting_method&MajorityVoting))
    return;

  if (NvotedLabels()==0) {
    cerr << "SOM::ConditionallyMajorityVoteLabels no labels" << endl;
    Dump(DumpLong);
  }

  for (int i=0; i<Units(); i++) {
    Vote *v = Majority(i);
    Unit(i)->Label(v ? v->Label() : preserve ? Unit(i)->Label() : NULL);
  }
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DumpVotes(ostream& os) const {

  for (int i=0; i<NvotedLabels(); i++)
    VotedLabel(i)->Dump(DumpDefault, os);
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DumpVotesByUnit(ostream& os) const {
  for (int u=0; u<Units(); u++) {
    char tmp[128];
    sprintf(tmp, "%3d: ", u);
    os << tmp;
    
    DumpVotesOfUnit(u, os);
    os << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DumpVotesOfUnit(int u, ostream& os) const {
  Vote *maj = Majority(u);
  if (!maj)
    return;

  for (int v = maj->GetCount(u); v>0; v--)
    for (int l=0; l<NvotedLabels(); l++)
      if (VotedLabel(l)->GetCount(u)==v) {
	char tmp[128];
	sprintf(tmp, "%3u %s ", v, VotedLabel(l)->Label());
	os << tmp;
      }
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DisplayStatistics(ostream& os) const {
  int tot = 0, cor = 0, err = 0, empty = 0, u;

  for (u=0; u<Units(); u++)
    for (int l=0; l<NvotedLabels(); l++)
      if (VotedLabel(l)->LabelsMatch(Unit(u)->Label()))
	cor += VotedLabel(l)->GetCount(u);
      else
	err += VotedLabel(l)->GetCount(u);

  for (int i=0; i<NvotedLabels(); i++)
    tot += VotedLabel(i)->Sum();

  os << "Total="      << tot
       << " Correct="   << cor
       << " Erroneous=" << err;
  if (tot) {
    streamsize prec = os.precision(2);
    os.setf(ios::fixed, ios::floatfield);
    os << " (" << 100.0*err/tot << " %)";
    os.precision(prec);
  }
  os << endl;    

  for (u=0; u<Units(); u++)
    if (Unit(u)->EmptyLabel())
      empty++;

  os << "There are "  << Units()
       << " units and " << NvotedLabels()
       << " labels.  "  << empty
       << " units don't have a label." << endl;    
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DisplayConfusion(ostream& os) const {
  int n = NvotedLabels();
  if (!n)
    return;

  int *conf = new int[n*(n+1)], maxval = 1, maxlen = 0, src;
  memset(conf, 0, (n+1)*n*sizeof *conf);

  for (src=0; src<n; src++) {
    Vote *vsrc = VotedLabel(src);
    if ((int)strlen(vsrc->Label())>maxlen)
      maxlen = (int)strlen(vsrc->Label());

    for (int u=0; u<Units(); u++) {
      int dst = n;

      if (!Unit(u)->EmptyLabel()) {
	dst = GetVoteIndex(Unit(u)->Label());
	if (dst<0) {
	  cerr << "GetVoteIndex returned lt zero" << endl;
	  continue;
	}
      }

      int val = conf[src*(n+1)+dst] += vsrc->GetCount(u);
      if (val>maxval)
	maxval = val;
    }
  }

  int yllen = 1+(int)floor(log10((double)maxval));

  char ylformat[1024];
  sprintf(ylformat, "%%%ds ", maxlen);

  char nformat[1024];
  sprintf(nformat, "%%%dd ", yllen);

  char xlformat[1024];
  sprintf(xlformat, "%%%ds ", yllen);

  int i = 1; // isatty(os.something);
  const char *bold = i?"\033[1m":"", *strike_out = i?"\033[2m":"",
    *normal = i?"\033[0m":"";

  char tmp[1024];

  for (src=-1; src<n; src++) {
    sprintf(tmp, ylformat, src==-1 ? "" : VotedLabel(src)->Label());
    os << bold << tmp << normal;

    for (int dst=0; dst<n+1; dst++) {
      if (src==-1)
	sprintf(tmp, xlformat, dst<n ? VotedLabel(dst)->Label() : "?");
      else {
	sprintf(tmp, nformat, conf[(n+1)*src+dst]);
	if (!conf[(n+1)*src+dst] && src!=dst)
	  tmp[yllen-1] = ' ';
      }

      tmp[yllen]   = ' ';
      tmp[yllen+1] = 0;
 
      if (dst!=src)
	os <<  (src==-1?bold:strike_out) << tmp << normal;
      else
	os << tmp;
    }

    os << endl;
  }

  delete conf;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::ReadCommon(istream *isp, gzFile gz) {
  if (bool(isp)==bool(gz))
    return false;

  // istream& is = *isp;

  const char *eh = "SOM::ReadCommon() : ";

  Delete();

  const char *line;
  if (!ReadHead(isp, gz, &line))
    return ShowError(eh, "ReadHead() failed");

  int len = atoi(line);
  if (!len)
    return ShowError(eh, "len==0");

  VectorLength(len); // vectorlength = len; // added 13.12.01

  const char *ptr = line+strspn(line, "0123456789 \t");

  if (!strncmp(ptr, "sparse", 6)) {
    Sparse(true);
    ptr += 6;
    ptr += strspn(ptr, " \t");
  }

  if (strncmp(ptr, "rect", 4) && strncmp(ptr, "hexa", 4))
    return ShowError(eh, "failed to find rect/hexa");

  ptr += 4+strspn(ptr+4, " \t");

  width = atoi(ptr);

  ptr += strspn(ptr, "0123456789");
  ptr += strspn(ptr, " \t");

  height = atoi(ptr);

  ptr += strspn(ptr, "0123456789");
  ptr += strspn(ptr, " \t");

  if (!strncmp(ptr, "bubble", 6)) {
    ptr += 6;
    // ptr += strspn(ptr, "0123456789");
    ptr += strspn(ptr, " \t");
  }

  if (!strncmp(ptr, "torus", 5)) {
    Topology(Torus);
  }

  delete [] line;

//  cout << "len=" << len << " w=" << w << " h=" << h << endl;

  // cout << "NoDataRead()=" << NoDataRead() << endl;

  FloatVector vec(len, NULL, NULL, NoDataRead());
  Initialize(vec);

  if (!Units())
    return ShowError(eh, "failed with Units()==0");

  for (int i=0; i<Units(); i++) {
    char no[15];
    sprintf(no, "%d", i);

    const char *vecstr = NULL;
    for (;;) {
      // cout << "SOM::ReadCommon ";
      if (!PossiblyRecordSeekIndex(i, isp, gz))
	return ShowError(eh, "PossiblyRecordSeekIndex() failed unit ", no);

      Tic("FloatVectorSource::GetLine");
      if (!FloatVectorSource::GetLine(isp, gz, vecstr) || !*vecstr)
	return ShowError(eh, "GetLine() failed with unit ", no);
      Tac("FloatVectorSource::GetLine");

      if (*vecstr=='#')
	delete [] vecstr;
      else
	break;
    }

    // added 14.09.2005 to avoid random content of sparse vectors!
    Unit(i)->Zero(); 

    Tic("FloatVector::ReadFromString");
    if (!Unit(i)->ReadFromString(vecstr, Sparse())) {
      ShowError(eh, "failed with unit ", no);
      Unit(i)->Dump(DumpRecursiveLong);
      delete [] vecstr;
      return false;
    }
    Tac("FloatVector::ReadFromString");
    delete [] vecstr;
  }

  ExtractUmatrix(Description());

  return true;
}

///////////////////////////////////////////////////////////////////////////////

/*
int SOM::Read_Old(istream& is) {
  Delete();
  //  ReadHead(is, true);  

  int len = 0; // , w = 0, h= 0;
  char topo[100], neig[100], ch;

  for (;;) {
    do {
      ch = is.get();
    } while (ch=='\n');

    if (ch=='#')
      is.ignore(MAXINT, '\n');
    else {
      is.putback(ch);
      break;
    }
  }

  is >> len >> topo >> width >> height >> neig;

//  cout << "len=" << len << " w=" << w << " h=" << h << endl;

  // cout << "NoDataRead()=" << NoDataRead() << endl;

  FloatVector vec(len, NULL, NULL, NoDataRead());
  Initialize(vec);

  for (int i=0; i<Units(); i++)
    if (!Unit(i)->Read(is)) {
      cout << "ERROR in SOM::Read" << endl;
      Unit(i)->Dump(DumpRecursiveLong);
      cout << is.tellg() << endl;
      break;
    }

  return is.good();
}
*/

///////////////////////////////////////////////////////////////////////////////

bool SOM::Write(ostream *osp, gzFile gz) {
  bool issparse = Sparse() || WriteLabelsOnly();

  PutLine(osp, gz, "#");
  WriteDescription(osp, gz);
  
  Flush(osp, gz);

  char comment[1000];
  PutLine(osp, gz, "#");
  sprintf(comment, "# alpha:  %s", alpha.ToLongString());
  PutLine(osp, gz, comment);
  sprintf(comment, "# radius: %s", radius.ToLongString());
  PutLine(osp, gz, comment);
  if (umatrix.IndexOK(0) && save_umatrix) {
    PutString(osp, gz, "# umatrix: ");
    const char *umx = umatrix[0].WriteToString();
    PutLine(osp, gz, umx);
    delete umx;
  }
  PutLine(osp, gz, "#");

  Flush(osp, gz);

  sprintf(comment, "%d%s rect %d %d bubble", Unit(0)->Length(),
	  issparse?" sparse":"", width, height);

  if (Topology()==Torus)
    strcat(comment, " torus");

  PutLine(osp, gz, comment);

  Flush(osp, gz);

  for (int u=0; u<Units(); u++)
    WriteOne(*Unit(u), osp, gz); // Unit(u)->Write(os);

  Flush(osp, gz);

  return IsGood(osp, gz);
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DoMonitoring(int force) {
  if (!monitoring)
    return;

  if (Round()%monitoring && !force)
    return;

  char line[1024];
  sprintf(line, "%d %f", Round(), errsum);

//  gnuplot.AddLine(line).Refresh();

  errsum = 0;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DoVoting(int u, const FloatVector& v, double d) {
  if (voting_method&VotingDuringTraining)
    AddEitherVote(u, v.Label(),d);
}

///////////////////////////////////////////////////////////////////////////////

void SOM::AddEitherVote(int u, const char *l, double d) {
  if (!IndexOK(u)) {
    ShowError("SOM::AddEitherVote() !IndexOK(u)");
    return;
  }
  if (!l) {
    ShowError("SOM::AddEitherVote() !l");
    return;
  }
  if (d==MAXFLOAT) {
    ShowError("SOM::AddEitherVote() d==MAXFLOAT");
    return;
  }

  // cout << "SOM::AddEitherVote() " << (void*)this
  // << " " << int(voting_method) << endl;

  if (SkipLabel(l)) {
    if (DebugSkips())
      cout << "SOM::AddEitherVote() skipping [" << l << "]" << endl;
    return;
  }

  if (voting_method&NearestVoting)
    AddNearestVote(u, l, d);
  if (voting_method&MajorityVoting)
    AddVote(u, l, d);
}

///////////////////////////////////////////////////////////////////////////////

void SOM::AddNearestVote(int u, const char *l, double d) {
  //  Tic("SOM::AddNearestVote()");

  // cout << "SOM::AddNearestVote()" << endl;

  while (nearest_votes.Nitems()<=u)
    nearest_votes.Append(new FloatVector(2));

  nearest_votes[u][1]++;

  if (!nearest_votes[u].Label() || nearest_votes[u][0]>d) {
    nearest_votes[u][0] = d;
    nearest_votes[u].Label(l);
    Unit(u)->Label(l);

//     char tmp[1024];
//     sprintf(tmp, "%d %s %g", u, l, d);
//     cout << "AddNearestVote() added " << tmp << endl;
  }

  // Tac("SOM::AddNearestVote()");
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DoStatistics(double err) {
  errsum += err;
}

///////////////////////////////////////////////////////////////////////////////

Planar& SOM::Resize(int ow, int oh) {
  FloatVector tmp(Classifier::VectorLength()), *old = NULL;

//   cout << "SOM::Resize nitems=" << Nitems() << " units=" << Units();
//   if (Nitems())
//     cout << " veclen=" << Unit(0)->Length();
//   cout << endl;

  if (Nitems() && !Relinquish(old = Unit(0))) {
    cerr << ":SOM::Resize(): Relinquish failed." << endl;
    old = NULL;
  }

  Delete();

  for (int i=0; i<Units(); i++) {
    Append(new FloatVector(old ? *old : tmp));

    Unit(i)->Number(i);

    if (Unit(i)->IsAllocated()) {
      Unit(i)->Randomize();
      Unit(i)->Normalize();
    }
  }

  delete old;

  if (!ow && !oh)
    SetDefaultRadius();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::Display(ostream& os) const {
  size_t l = 0;
  for (int i=0; i<Units(); i++)
    if (Unit(i)->Label() && strlen(Unit(i)->Label())>l)
      l = strlen(Unit(i)->Label());

  char empty[1000];

  for (size_t j=0; j<l; j++)
    empty[j] = ' ';
  empty[l] = 0;
  empty[l/2] = '.';

  for (int y=0; y<Height(); y++) {
    for (int x=0; x<Width(); x++) {
      if (!Unit(x, y)->Label())
	os << empty;
      else {
	char tmp[1000];
	sprintf(tmp, "%*s", (int)l, Unit(x, y)->Label());
	os << tmp;
      }
      if (x+1<Width())
	os << " ";
    }
    os << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DisplayComponent(ostream& os, int i, const char *fmt) const {
  for (int y=0; y<Height(); y++) {
    for (int x=0; x<Width(); x++) {
      char tmp[1000];
      sprintf(tmp, fmt, Unit(x, y)->Get(i));
      os << tmp;
    }
    os << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DisplayHits(const FloatVectorSet& set,
			      const IntPointVector& ip, int much) const {

  const char *bold = "\033[1m", *strike_out = "\033[2m", *normal = "\033[0m";

  for (int y=0; y<Height(); y++) {
    for (int x=0, i; x<Width(); x++) {
      const char *lab = Unit(ToIndex(x, y))->Label();
      if (much)
	cout << (lab ? lab : ".");

      for (i=0; i<ip.Length(); i++)
	if (ip[i].X()==x && ip[i].Y()==y) {
	  cout << (much ? set[i].LabelsMatch(lab) ? bold : strike_out : "")
	       << (set[i].Label() ? set[i].Label() : "*")
	       << (much ? normal : "");
	  break;
	}

      if (i==ip.Length())
	cout << (much ? " " : ".");
      if (!much)
	cout << " ";

    }
    cout << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

FloatVector SOM::WithinClassDistances(const FloatVectorSet& set,
					      const IntPointVector& ip) const {
  FloatVector ret(ip.Length());
	
  for (int i=0; i<ip.Length(); i++) {
    float max = 0;
    for (int j=0; j<ip.Length(); j++) {
      if (!set[i].LabelsMatch(set[j]) || i==j)
	continue;
      
      float d = ip[i].Distance(ip[j]);
      if (d>max)
	max = d;
    }
    ret[i] = max;
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////

FloatVector SOM::BetweenClassDistances(const FloatVectorSet& set,
					       const IntPointVector& ip)const {
  FloatVector ret(ip.Length());
	
  for (int i=0; i<ip.Length(); i++) {
    float min = MAXFLOAT;
    for (int j=0; j<ip.Length(); j++) {
      if (set[i].LabelsMatch(set[j]) || i==j)
	continue;
      
      float d = ip[i].Distance(ip[j]);
      if (d<min)
	min = d;
    }
    ret[i] = min;
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::ForceDiverge(const FloatVector& v, const FloatVectorSet& set,
			       double min, double max, double aw, double ab) {

  for (int i=0; i<set.Nitems(); i++) {
    IntPoint vp = XYCoord(BestMatchingUnit(v));
    int dir = v.LabelsMatch(set[i]) ? 1 : -1;

    IntPoint ip = XYCoord(BestMatchingUnit(set[i]));
    double d = vp.Distance(ip);
    if (d<min || d>max)
      continue;

    double a = dir==1 ? aw : ab;

    double at = atan2(double(ip.Y()-vp.Y()), double(ip.X()-vp.X()));
    double st = sin(at), ct = cos(at);
    IntPoint dp((int)floor(ct+0.5), (int)floor(st+0.5));

    IntPoint vpt(vp);

    // cout << vpt << "->" << ToPoint(Nearest(vpt+dp*dir)) << v.Label() << " ";

    vpt = ToPoint(Nearest(vpt+dp*dir));
    
    DoAdapt(ToIndex(vpt), v, a, 1.5);

    // cout << ip<< "->"<< ToPoint(Nearest(ip-dp*dir))<< set[i].Label() <<endl;

    ip = ToPoint(Nearest(ip-dp*dir));
    
    DoAdapt(ToIndex(ip), set[i], a, 1.5);
  }
}

///////////////////////////////////////////////////////////////////////////////

int SOM::WriteGnuPlot(ostream& os, int grid) const {
  int x, y;

  for (y=0; y<Height(); y++)
    for (x=0; x<Width(); x++) {
      Unit(x, y)->Write(os);
      if (!grid || x==Width()-1)
	os << endl;
    }

  if (grid)
    for (x=0; x<Width(); x++)
      for (y=0; y<Height(); y++) {
	Unit(x, y)->Write(os);
	if (y==Height()-1)
	  os << endl;
      }

  return os.good();
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::BatchTrainEpochDone(const FloatVectorSet& sum,
			      const ListOf<IntVector>& backref) {
  Classifier::BatchTrainEpochDone(sum, backref);

  Tic("SOM::BatchTrainEpochDone()");

  double r = Radius();
  IntVector counts(backref.Nitems());
  int csum = 0;
  for (int k=0; k<backref.Nitems(); k++)
    csum += counts[k] = backref[k].Length();

  for (int j=0; j<csum; j++)
    DecayRadius();

  cout << PossiblyTimeStamp()
       << "SOM::BatchTrainEpochDone(), nitems=" << Nitems()
       << " r=" << r << endl;
  
  FloatVectorSet oldset = VectorSetPart();  // this makes a copy of the old

  DoNeighborhoodAveraging(oldset, counts, r);

  Tac("SOM::BatchTrainEpochDone()");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::DoNeighborhoodAveraging(const FloatVectorSet& mean,
				  const IntVector& count, double r) {
#ifdef CLASSIFIER_USE_PTHREADS
  if (BatchTrainThreads())
    return DoNeighborhoodAveragingPthread(mean, count, r);
#endif // CLASSIFIER_USE_PTHREADS

  Tic("SOM::DoNeighborhoodAveraging()");

  for (int i=0; i<Units(); i++)
    SetToNeighborhoodAverage(i, mean, count, r);

  Tac("SOM::DoNeighborhoodAveraging()");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef CLASSIFIER_USE_PTHREADS
bool SOM::DoNeighborhoodAveragingPthread(const FloatVectorSet& mean,
					 const IntVector& count, double r) {
 
  Tic("SOM::DoNeighborhoodAveragingPthread()");

  const char *errhead = "SOM::DoNeighborhoodAveragingPthread() ";

  int n = batchtrain_threads>=0 ? batchtrain_threads : 16;
  int con = pthread_getconcurrency();
  if (pthread_setconcurrency(n))
    ShowError(errhead, "pthread_setconcurrency() failed");

  neigh_aver_pthread_data *data = new neigh_aver_pthread_data[n];
  for (int k=0; k<n; k++)
    data[k].pthread_set = false;

  int loopsleft = n+1;
  bool ready = false;
  for (int i=0; loopsleft; i++) {
    int j = i%n;
    if (data[j].pthread_set) {
      void *void_ret = NULL;
      if (pthread_join(data[j].pthread, &void_ret)) 
	ShowError(errhead, "pthread_join() failed");

      data[j].pthread_set = false;
      loopsleft = n+1;
    } else
      loopsleft--;

    if (ready)
      continue;

    if (i<Units()) {
      data[j].pthread_set = false;
      data[j].som     = this;
      data[j].i       = i;
      data[j].mean    = &mean;
      data[j].count   = &count;
      data[j].r       = r;

      //#ifdef __alpha
      void *(*xxx)(void*) = SetToNeighborhoodAverage;
      //if (pthread_create(&data[j].pthread, NULL, (Clinkfptype)xxx, data+j))
      //#else
      if (pthread_create(&data[j].pthread, NULL,
			 (CFP_pthread)xxx, data+j))
	//#endif // __alpha
	ShowError(errhead, "pthread_create() failed");
      else
	data[j].pthread_set = true;

    } else
      ready = true;
  }

  delete [] data;

  if (pthread_setconcurrency(con))
    ShowError(errhead, "pthread_setconcurrency() failed");

  Tac("SOM::DoNeighborhoodAveragingPthread()");

  return true;
}
#endif // CLASSIFIER_USE_PTHREADS

///////////////////////////////////////////////////////////////////////////////

bool SOM::SetToNeighborhoodAverage(int i, const FloatVectorSet& mean,
				   const IntVector& count, double r) {
  IntVector idx = SolveNeighborhood(i, r);

  if (!idx.Length())
    return true;

  FloatVector sum(VectorLength());
  double mulsum = 0;

  for (int j=0; j<idx.Length(); j++) {
    // cout << "i=" << i << " idx[j]=" << idx[j] << endl;
    const int ii = idx[j];
    const double m = count[ii];
    sum.AddWithMultiply(mean[ii], m);
    mulsum += m;
  }

  if (mulsum) {
    sum.Divide(mulsum);
    VectorSetPart()[i].CopyValue(sum);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::CalculateRawUmatrix() {
  // cout << "SOM::CalculateRawUmatrix() called" << endl;

  umatrix.Delete();
  umatrix.Append(new FloatVector(UmatrixLength(), NULL, "raw"));

  bool is_torus = Topology()==Torus;

  for (int x=0; x<Width(); x++)
    for (int y=0; y<Height(); y++) {
      int x2 = (x+1)%Width(), y2 = (y+1)%Height();

      if (is_torus || y<Height()-1)
	RawUmatrix(x, y, false) = VectorDistance(x, y, x, y2);
      if (is_torus || x<Width()-1)
	RawUmatrix(x, y, true)  = VectorDistance(x, y, x2, y);
    }

  // umatrix[0].Dump(DumpLong);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::CalculateUnitMeanUmatrix() {
  if (!umatrix.IndexOK(0)) {
    ShowError("SOM::CalculateUnitMeanUmatrix(): raw U-matrix missing");
    return false;
  }
  FloatVector *v = umatrix.GetEvenNew(1);
  *v = umatrix[0];
  v->Label("unit-mean");
  v->Divide(v->Mean());

  // umatrix[1].Dump(DumpLong);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::ExtractUmatrix(const char *txt) {
  umatrix.Delete();

  while (txt && *txt) {
    if (!strncmp(txt, "umatrix: ", 9)) {
      const char *end = strchr(txt, '\n');
      if (!end)
	break;
      
      FloatVector v(UmatrixLength());
      if (v.ReadFromString(txt+9))
	umatrix.AppendCopy(v);
      else
	ShowError("SOM::ExtractUmatrix() ValueFromString() failed");

      break;
    }
    txt = strchr(txt, '\n');
    if (txt)
      txt++;
  }

  //   if (umatrix.IndexOK(0))
  //     umatrix[0].Dump(DumpLong);
  
  return umatrix.IndexOK(0);
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::SolveUmatrixDistances(double dlim, SOM::UmatrixFunc ufunc) {
  if (dlim==umatrix_dist_limit)
    return true;

  umatrix_dist_limit = -1;
  umatrix_dist.clear();

  for (IntPoint p=*this; p; ++p)
    if (!SolveUmatrixDistances(p.X(), p.Y(), dlim, ufunc)) {
      umatrix_dist.clear();
      return false;
    }

  umatrix_dist_limit = dlim;

  // DumpUmatrixDistances();

  CheckUmatrixDistances();

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::SolveUmatrixDistances(int x0, int y0, double dlim,
				SOM::UmatrixFunc ufunc) {
  int o = ToIndex(x0, y0);
  umatrix_dist.push_back(vector<umatrix_dist_t>(1, umatrix_dist_t(o, 0)));

  bool found = true;
  for (int r=1; found; r++) {
    found = false;
    int x = x0+r, y = y0;
    for (int m=0; m<4; m++) {
      int dx = (m/2)*2-1, dy = (((m+1)/2)%2)*2-1;
      for (int i=0; i<r; i++, x += dx, y += dy) {
	if (!IsInside(x, y))  // what about toroid ???
	  continue;

	// DumpUmatrixDistances();

	int x1 = x+dy, y1 = y, x2 = x, y2 = y-dx;
	bool h = true, v = false;

	if (i==0) {
	  if (m==0 || m==2) {
	    x2 = x1;
	    y2 = y1;
	    v = true;  // ie. only horizontal direction is effective
	  } else {
	    x1 = x2;
	    y1 = y2;
	    h = false;  // ie. only vertical direction is effective
	  }
        }

	float s1 = FindUmatrixDistance(o, ToIndex(x1, y1));
	float s2 = FindUmatrixDistance(o, ToIndex(x2, y2));

	// if (s1==MAXFLOAT || s2==MAXFLOAT)
	//   return ShowError("SOM::SolveUmatrixDistances() s1/2 == MAXFLOAT");

	int ux1 = x<x1 ? x : x1, uy1 = y<y1 ? y : y1;
	int ux2 = x<x2 ? x : x2, uy2 = y<y2 ? y : y2;

	/*
	cout << "x0=" << x0 << " y0=" << y0 << " r=" << r << " m=" << m
	     << " dx=" << dx << " dy=" << dy
	     << " i=" << i
	     << " x=" << x << " y=" << y
	     << " x1=" << x1 << " y1=" << y1
	     << " x2=" << x2 << " y2=" << y2
	     << " ux1=" << ux1 << " uy1=" << uy1 << " h=" << h
	     << " ux2=" << ux2 << " uy2=" << uy2 << " v=" << v
	     << endl;
	*/

	float u1 = (this->*ufunc)(ux1, uy1, h);
	float u2 = (this->*ufunc)(ux2, uy2, v);

	if (u1==-1 || u2==-1)
	  return ShowError("SOM::SolveUmatrixDistances() u1/2 == -1");

	float d1 = s1 + u1;
	float d2 = s2 + u2;

	float dd = d1<d2 ? d1 : d2;

	if (dd<=dlim) {
	  found = true;
	  umatrix_dist.back().push_back(umatrix_dist_t(ToIndex(x, y), dd));
	}
      }
    }
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

void SOM::DumpUmatrixDistances() const {
  cout << "U-matrix distance dump, " << umatrix_dist.size()
       << " units" << endl;
  for (size_t u=0; u<umatrix_dist.size(); u++) {
    if (u)
      cout << "---------" << endl;
    const vector<umatrix_dist_t>& d = umatrix_dist[u];
    for (size_t i=0; i<d.size(); i++)
      cout << u << "->" <<  d[i].Unit() << " : " << d[i].Distance() << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

double SOM::FindUmatrixDistance(int from, int to) const {
  // cout << "SOM::FindUmatrixDistance(" << from << "," << to << ")" << endl;

  if (from<0|| size_t(from)>=umatrix_dist.size()) {
    ShowError("SOM::FindUmatrixDistance() failed with from");
    return MAXFLOAT;
  }

  const vector<umatrix_dist_t>& d = umatrix_dist[from];
  for (size_t i=0; i<d.size(); i++) {
    // cout << "i=" << i << " unit=" << d[i].Unit()
    // 	 << " dist=" << d[i].Distance() << endl;
    if (d[i].Unit()==to)
      return d[i].Distance();
  }

  return MAXFLOAT;
}

///////////////////////////////////////////////////////////////////////////////

bool SOM::CheckUmatrixDistances() const {
  for (size_t u=0; u<umatrix_dist.size(); u++) {
    const vector<umatrix_dist_t>& d = umatrix_dist[u];
    for (size_t i=0; i<d.size(); i++) {
      size_t v = d[i].Unit();
      float  l = d[i].Distance();
      const float eps = 0.00001;

      if (l<0 || l>umatrix_dist_limit)
	return ShowError("SOM::CheckUmatrixDistances() failed with index l");

      if (v==u && l!=0) {
	stringstream s;
	s << "v=" << v << " u=" << u << " l=" << l;
	return ShowError("SOM::CheckUmatrixDistances() non-zero distance : ",
			 s.str());
      }

      if (v!=u && l==0) {
	stringstream s;
	s << "v=" << v << " u=" << u << " l=" << l;
	return ShowError("SOM::CheckUmatrixDistances() zero distance : ",
			 s.str());
      }

      if (fabs(FindUmatrixDistance(u, v)-l)>eps*l ||
	  fabs(FindUmatrixDistance(v, u)-l)>eps*l) {
	char tmp[1000];
	sprintf(tmp, "u=%d i=%d v=%d l=%f f1=%f f2=%f d1=%f d2=%f",
		(int)u, (int)i, (int)v, l,
		FindUmatrixDistance(u, v), FindUmatrixDistance(v, u),
		fabs(FindUmatrixDistance(u, v)-l),
		fabs(FindUmatrixDistance(v, u)-l));
	return ShowError("SOM::CheckUmatrixDistances() failed 3 : ", tmp);
      }
      /*
      for (int j=0; j<i; j++)
	if (d[j].Distance()==l)
	  / *return * / ShowError("SOM::CheckUmatrixDistances() failed 4");
      */
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// By Philip Prentis

double SOM::AverageQuantisationError(bool recalculate) {
  cout << PossiblyTimeStamp()
       << "SOM::AverageQuantisationError(" << recalculate << "), ";
  if (!recalculate && (average_quantisation_error >= 0)) {
    cout << "average_quantisation_error == " << average_quantisation_error 
         << endl;
    return average_quantisation_error;
  }
  else {
    average_quantisation_error = 0;
    FloatVectorSource& set = *TrainingSet();
    set.First();
    int count = 0;
    for (const FloatVector *vec; (vec = set.Next());) {
      //float d;
      //BestMatchingUnit(*vec, &d);

      //int SOM::BestMatchingUnit(const FloatVector& v, float *err) const {
      //    cout << "SOM::BestMatchingUnit()" << endl;

      IntVector nn(1);
      FloatVector ddd;
      NearestNeighbors(*vec, nn, ddd);
      average_quantisation_error += double(((float*)ddd)[0]);
      count++;
    }
    average_quantisation_error = count ? average_quantisation_error/float(count)
                                       : -1;
    cout << "average_quantisation_error == " << average_quantisation_error; 
    cout << ", count == " << count << endl;
    return average_quantisation_error;
  }
}

///////////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _SOM_C_

