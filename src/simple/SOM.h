// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: SOM.h,v 1.14 2012/02/01 14:09:36 mats Exp $

// -*- C++ -*-

#ifndef _SOM_H_
#define _SOM_H_

#include <Classifier.h>
#include <Planar.h>
#include <Point.h>

#include <vector>
using namespace std;

#if !SIMPLE_CLASS_SOM
#error SIMPLE_CLASS_SOM not defined
#endif

#define DEFAULT_ALPHA 0.2
#define DEFAULT_ALPHA_DECAY   0.9999
#define DEFAULT_RADIUS_DECAY 0.9999
#define DEFAULT_RADIUS_BASE  1

namespace simple {

typedef void (Simple::*SomProcessFunction)(FloatVector*, const FloatVector*, double&);

class SOM : public Classifier, public Planar {
public:
  enum NeighbourhoodKernel {n_k_rectangle, n_k_triangle};
  enum VotingMethod { NoVoting, MajorityVoting, NearestVoting,
		      VotingDuringTraining=4 };

  class Vote : public Simple {
  public:
    Vote(int c, const char *l) {
      count.Length(c).Label(l);
      min_dist.Length(c).Set(MAXFLOAT);
    }
    
    /// Poison.
    Vote() { SimpleAbort(); }

    virtual ~Vote() {}

    virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const {
      SimpleAbort(); }
    virtual const char **SimpleClassNames() const { static const char *n[] = {
    "Vote", NULL }; return n; }

    const char *Label() const { return count.Label(); }
    bool LabelsMatch(const char *l) const { return count.LabelsMatch(l); }

    void Empty() { count.Zero(); min_dist.Set(MAXFLOAT); }
    void Reduce(double mul) { count.Multiply(mul); }
    void Add(int i, int a, double d) {
      count.Add(i, a );
      if (min_dist[i]>d)
	min_dist[i] = d;
    }
    double GetDist(int i) const { return min_dist.Get(i); }
    int GetCount(int i) const { return count.Get(i); }
    int Sum() const { return count.Sum(); }
  protected:
    IntVector   count;
    FloatVector min_dist;
  };

  SOM(int, int, int,   TopologyType=Rectangular,
      double = DEFAULT_ALPHA, double = MAXFLOAT);
  SOM(int, int, const FloatVector&,
      TopologyType = Rectangular, 
      double = DEFAULT_ALPHA, double = MAXFLOAT);
  SOM(istream&);
  SOM(const char*);
  SOM(const SOM&);
  ~SOM();

  void Initialize(const FloatVector&,
		  double = DEFAULT_ALPHA, double = MAXFLOAT);

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

  virtual const char **Names() const { static const char *n[] = {
    "SOM", "Classifier", "VectorSet", NULL }; return n; }

// these are inherited from Classifier:

  virtual int Configuration(const char*, const char** = NULL);
  virtual const char *Configuration() const;
  virtual const char *Identity(const char* = NULL);
  virtual const char *ClassifyVector(const FloatVector&, float* = NULL);
  virtual bool AdaptVector(const FloatVector&);
  virtual void AdaptEpochDone(FloatVectorSource&);

  virtual bool BatchTrainEpochDone(const FloatVectorSet&,
				   const ListOf<IntVector>&);
  
  bool DoNeighborhoodAveraging(const FloatVectorSet&,
			       const IntVector&, double);

#ifdef CLASSIFIER_USE_PTHREADS
  struct neigh_aver_pthread_data {
    pthread_t pthread;          // thread id
    bool pthread_set;
    SOM *som;                   // this becomes this
    int i;                      // args of SetToNeighborhoodAverage()
    const FloatVectorSet *mean;
    const IntVector *count;
    double r;
  };

  bool DoNeighborhoodAveragingPthread(const FloatVectorSet&,
				      const IntVector&, double);

  static void *SetToNeighborhoodAverage(void *ptr) {
    neigh_aver_pthread_data& a = *(neigh_aver_pthread_data*)ptr;
    a.som->SetToNeighborhoodAverage(a.i, *a.mean, *a.count, a.r);
    return ptr; }
#endif // CLASSIFIER_USE_PTHREADS

  bool SetToNeighborhoodAverage(int, const FloatVectorSet&,
				const IntVector&, double r);

  virtual bool AdaptVote(const IntVector&, const FloatVector&,
			 const FloatVectorSet&);

  int Units() const { return width*height; }

  FloatVector *Unit(int i) const { return Get(i); }
  FloatVector *Unit(int x, int y) const {
    return IsInside(x, y) ? Unit(ToIndex(x, y)) : NULL; }
  FloatVector *Unit(const IntPoint& p) const { return Unit(p.X(), p.Y()); }
 
  int BestMatchingUnit(const FloatVector&, float* = NULL) const;
  IntPoint BestMatchingPoint(const FloatVector& v, float *d = NULL) const {
    return ToPoint(BestMatchingUnit(v, d)); }
  IntPointVector BestMatchingPoints(const FloatVectorSet& v) const;

  IntVector SolveNeighborhood(int i, double r) const;

  double NeighbourhoodFunction(int i, int p, double r) const;

  void DoAdapt(int, const FloatVector&,
	       double = MAXFLOAT, double = MAXFLOAT);

  void DoAdaptOld(int, const FloatVector&,
	       double = MAXFLOAT, double = MAXFLOAT);

  double Distance(const SOM& som) const { return sqrt(DistanceSquared(som));}
  double DistanceSquared(const SOM&) const;

  double Distance(int x, int y) const { return sqrt(DistanceSquared(x, y)); }

  double DistanceSquared(register int i, register int j) const {
    if (i<0 || j<0 || i>=Units() || j>=Units()) return MAXINT;
    return DistanceSquaredFast(i, j); }

  double DistanceSquaredFast(register int i, register int j) const {
    register double dx = XCoord(i)-XCoord(j), dy = YCoord(i)-YCoord(j);
    return dx*dx+dy*dy; }

  double VectorDistance(int x, int y) const {
    return IndexOK(x)&&IndexOK(y) ?
      Unit(x)->DistanceXX(*Unit(y), Metric()) : MAXFLOAT;
  }

  double VectorDistance(int x1, int y1, int x2, int y2) const {
    return VectorDistance(ToIndex(x1, y1), ToIndex(x2, y2)); }

  int  Recognize(const FloatVector&, float* = NULL);
  void Learn(const FloatVector&, int);
  void Learn(const FloatVector&);

  VotingMethod Voting() const { return voting_method; }
  NeighbourhoodKernel NKernel() const { return neighbourhood_kernel; }
  void  NKernel(NeighbourhoodKernel k) { neighbourhood_kernel = k; }
  void  Voting(VotingMethod i)  { voting_method = i;    }
  void  DoVoting(int, const FloatVector&, double);
  
  void  EmptyVotes();
  void  ReduceVotes(double);
  Vote *AddVote(int, const char*, double = 0);
  Vote *GetVotes(const char *l) const { return VotedLabel(GetVoteIndex(l)); }
  int   GetVoteIndex(const char*) const;
  Vote *Majority(int) const;
  void  ConditionallyMajorityVoteLabels(int = FALSE);
  void  DumpVotes(ostream&) const;
  void  DumpVotesByUnit(ostream&) const;
  void  DumpVotesOfUnit(int, ostream&) const;
  void  DumpVotesOfUnit(const IntPoint& p, ostream& os) const {
    DumpVotesOfUnit(ToIndex(p), os); }

  int   NvotedLabels()    const { return votelist.Nitems(); }
  Vote *VotedLabel(int i) const { return votelist.Get(i);   }

  Vote *AddLabel(const char *l) {
    votelist.Append(new Vote(Units(), l));
    return votelist.Peek();
  }

  void DoLabeling(FloatVectorSource&);
  void ClearLabels();
  
  void AddEitherVote(int, const char*, double);
  void AddNearestVote(int, const char*, double);

  void DisplayStatistics(ostream& = cout) const;
  void DisplayConfusion( ostream& = cout) const;

  virtual void PreProcess(FloatVector  *u, const FloatVector  *v,
			  double& a) {
    if (prep_o && prep_f) (prep_o->*prep_f)(u, v, a);  } 

  virtual void PostProcess(FloatVector  *u, const FloatVector  *v,
			   double& a) {
    if (postp_o && postp_f) (postp_o->*postp_f)(u, v, a);  } 

  void SetPreProcess(Simple* o = NULL, SomProcessFunction f = NULL) {
    prep_o = o; prep_f = f; }
  
  void SetPostProcess(Simple* o = NULL, SomProcessFunction f = NULL) {
    postp_o = o; postp_f = f; }

  SomProcessFunction GetPreProcess()  const { return prep_f; }
  SomProcessFunction GetPostProcess() const { return postp_f; }

  void DecayRadius() { 
    if (DebugAdapt())
      cout << "SOM::DecayRadius() : " << radius;
    radius.Decay(); 
    if (DebugAdapt())
      cout << " -> " << radius << endl;
  }

  void IncrementRound() { round++; }
  double Alpha()  const { return alpha; }
  double Radius() const { return radius; }
  int    Round()  const { return round;  }

  DecayVar& Alpha(double a)  { return  alpha.Keep(a); }
  DecayVar& Radius(double r) { return radius.Keep(r); }

  void SetNoNeighborhood() { radius.SetConstant(0).SetBase(0).Keep(); }

  FloatVectorSet Neighborhood(int, double,
			      Simple::Metric = Simple::Euclidean) const;

  Matchmethod GetMatchMethod() const      { return matchmethod; }
  void SetMatchMethod(Matchmethod m)      { matchmethod = m; }
  void SetMatchMethodEuclidean()          { matchmethod = euclidean; }
  void SetMatchMethodDotProduct()         { matchmethod = dotproduct; }
  void SetMatchMethodAbsoluteDotProduct() { matchmethod = absolutedotproduct; }

  bool ReadCommon(istream*, gzFile);

  int Read(istream& is) { return ReadCommon(&is, NULL); }
  int Read_Old(istream&);
  int Read(const char *n)  { ifstream is(n); return Read(is); }
  bool Write(ostream*, gzFile);
  bool Write(const char *n)  { ofstream os(n); return Write(&os, NULL); }
  bool Write() { return Write(FileName()); }

  int  Monitoring()      { return monitoring; }
  void Monitoring(int m) { monitoring = m;    }
  virtual void DoMonitoring(int = FALSE);

  virtual void DoStatistics(double);

  void Display(ostream& = cout) const;
  void DisplayHits(const FloatVectorSet&,
		   const IntPointVector&, int = FALSE) const;

  void DisplayComponent(ostream&, int, const char*) const;

  FloatVector WithinClassDistances(const FloatVectorSet&,
				   const IntPointVector&) const;
  FloatVector BetweenClassDistances(const FloatVectorSet&,
				   const IntPointVector&) const;

  void ForceDiverge(const FloatVector&, const FloatVectorSet&,
		    double, double, double, double);

  void SetDefaultRadius();

  int WriteGnuPlot(const char *n, int g = TRUE) const {
    ofstream os(n);
    return WriteGnuPlot(os, g ); }
  int WriteGnuPlot(ostream&, int = TRUE) const;

  void CountHitTypes() { hit_types[0] = hit_types[1] = hit_types[2] = 0; }
  void ReportHitTypes(ostream& os = cout) const {
    float t4 = (Width()-2)*(Height()-2);
    float t3 = (Width()+Height()-4)*2;

    os << "4-neighbors " << hit_types[0] << " / " << t4 << " "
       << hit_types[0]/t4 << endl;
    os << "3-neighbors " << hit_types[1] << " / " << t3 << " "
       << hit_types[1]/t3 << endl;
    os << "2-neighbors " << hit_types[2] << " / " << 4 << " "
       << hit_types[2]/4 << endl;
  }

  void ShowProgress(const FloatVector&, const char* = NULL,
		    const DecayVar* = NULL, const DecayVar* = NULL);
  void ProgressText(const char *txt) { CopyString(progress_text, txt); } 
  void ProgressInterval(int i) { progress_interval = i; }
  int  ProgressInterval() const { return progress_interval; }

  bool HasRawUmatrix() const { return HasUmatrix(0); }

  bool HasUnitMeanUmatrix() const { return HasUmatrix(1); }

  bool HasUmatrix(int l) const { return umatrix.IndexOK(l); }

  bool ExtractUmatrix(const char*);

  bool CalculateRawUmatrix();

  bool CalculateUnitMeanUmatrix();

  const FloatVector& UmatrixValues(int l) const {
    if (!umatrix.IndexOK(l)) {
      static FloatVector empty;
      ShowError("SOM::UmatrixValues() error");
      return empty;
    }
    return umatrix[l];
  }

  const FloatVector& RawUmatrixValues() const {
    return UmatrixValues(0); }

  const FloatVector& UnitMeanUmatrixValues() const {
    return UmatrixValues(1); }

  float& Umatrix(int x, int y, bool h, int l) const {
    int i = UmatrixIndex(x, y, h);
    if (!umatrix.IndexOK(l) || !umatrix[l].IndexOK(i)) {
      char tmp[100];
      sprintf(tmp, "l=%d/%d i=%d/%d x=%d y=%d h=%d", l, umatrix.Nitems(), i,
	      umatrix.IndexOK(l)?umatrix[l].Length():0, x, y, h);
      ShowError("SOM::Umatrix() error : ", tmp);
      static float err = -1;
      return err;
    }
    return umatrix[l][i];
  }

  typedef float& (SOM::*UmatrixFunc)(int,int,bool) const;

  float& OneUmatrix(int x, int y, bool h) const {
    int i = UmatrixIndex(x, y, h);
    if (i==-1)
      SimpleAbort();
    static float one = 1.0;
    return one;
  }

  float& RawUmatrix(int x, int y, bool h) const { return Umatrix(x, y, h, 0); }

  float& UnitMeanUmatrix(int x, int y, bool h) const {
    return Umatrix(x, y, h, 1); }

  int UmatrixLength() const {
    int n =  2*Units();
    return Topology()==Torus ? n : n-Width()-Height();
  }

  bool UmatrixIndexOK(int x, int y, bool h) const {
    if (x<0 || y<0)
      return false;

    if (Topology()!=Torus)
      if (h)
	return x<Width()-1 && y<Height();
      else
	return x<Width() && y<Height()-1;
    else
      return x<Width() && y<Height();
  }

  int UmatrixIndex(int x, int y, bool h) const {
    if (!UmatrixIndexOK(x, y, h))
      return -1;

    if (Topology()!=Torus)
      if (h)
	return Width()*(Height()-1)+x*Height()+y;
      else
	return x*(Height()-1)+y;
    else
      if (h)
	return Width()*Height()+x*Height()+y;
      else
	return x*Height()+y;
  }
  
  class umatrix_dist_t {
  public:
    umatrix_dist_t(int u, double d) : unit(u), dist(d) {}

    int Unit() const { return unit; }
    double Distance() const { return dist; }

  protected:
    int unit;
    float dist;
  };

  bool SolveUmatrixDistances(double dlim, UmatrixFunc);
  bool SolveUmatrixDistances(int x0, int y0, double dlim, UmatrixFunc);

  void DumpUmatrixDistances() const;
  double FindUmatrixDistance(int, int) const;

  bool CheckUmatrixDistances() const;

  const vector<umatrix_dist_t>& GetUmatrixDistances(int x, int y) {
    return umatrix_dist[ToIndex(x, y)];
  }

  double UmatrixDistanceLimit() const {
    return umatrix_dist_limit;
  }

  int UmatrixDistanceCount() const {
    int l = 0;
    for (size_t i=0; i<umatrix_dist.size(); i++)
      l += umatrix_dist[i].size();
    return l;
  }

  void DeleteUmatrix() { umatrix.Delete(); }

  void SaveUmatrix(bool s) { save_umatrix = s; }

  virtual double AverageQuantisationError(bool);

  float RadiusBase() const {
    return radius_base;
  }

  void RadiusBase(float r) { 
//     cout << "SOM::RadiusBase(" << r <<  ") radius_base=" 
//          << radius_base; 
    radius_base = r;
    radius.SetBase(radius_base);
//     cout << "; after SetBase(radius_base) radius_base=" << radius_base 
//          << " r=" << r << endl;
//     radius.Dump();
  }

  double MeanDistance() const { return mean_distance; }

  void MeanDistance(double v) { mean_distance = v; }

  double RmsDistance() const { return rms_distance; }

  void RmsDistance(double v) { rms_distance = v; }

protected:
  virtual Planar& Resize(int, int);
  DecayVar radius;

  int      round;
  float errsum;

  Matchmethod matchmethod;  

  Simple *prep_o, *postp_o;
  SomProcessFunction prep_f, postp_f;

  VotingMethod voting_method;
  ListOf<Vote> votelist;
  FloatVectorSet nearest_votes;

  int monitoring;
  int hit_types[3];

  int progress_interval;
  char *progress_text;

  FloatVectorSet umatrix;
  bool save_umatrix;
  float umatrix_dist_limit;
  vector<vector<umatrix_dist_t> > umatrix_dist;

  double average_quantisation_error;
  NeighbourhoodKernel neighbourhood_kernel;

  float radius_base;

  double mean_distance, rms_distance;
};

// typedef float& (SOM::*SOM_UmatrixFunc)(int,int,bool) const;

} // namespace simple
#endif // _SOM_H_

