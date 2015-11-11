// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: TreeSOM.h,v 1.17 2010/07/22 21:43:15 jorma Exp $

// -*- C++ -*-

#ifndef _TREESOM_H_
#define _TREESOM_H_

#include <SOM.h>

#if !SIMPLE_CLASS_TREESOM
#error SIMPLE_CLASS_TREESOM not defined
#endif

#define TREESOMHEADER "# This is a TreeSOM file, nlevels="

namespace simple {
class TreeSOM : public SOM {
public:
  TreeSOM(int, int, int, TopologyType=Rectangular, double = DEFAULT_ALPHA, double = MAXFLOAT);
  TreeSOM(const char* = NULL);
  virtual ~TreeSOM();

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "TreeSOM", NULL }; return n; }

  int Nlevels() const {
    return (down?down->Nlevels():0)+1 ; }

  bool LevelOK(int l) const { return l>=0 && l<Nlevels(); }

  FloatVectorSet& Set(int);

  bool ReadCommon(istream*, gzFile);

  bool Read(gzFile gz) { return ReadCommon(NULL, gz); }

  bool Read(const char *n) { return ReadNew(n) /*ReadOld(n)*/ ; }

  bool Read(const string& s) { return Read(s.c_str()); }

  bool ReadNew(const char *n)  {
    gzFile gz = gzopen(n, "r");
    bool r = Read(gz);  gzclose(gz);  return r; }

  bool Read(istream& is) { return ReadCommon(&is, NULL); }
  bool ReadOld(istream&);
  bool ReadOld(const char *n)  {
    istream *fs = OpenReadIOstream(n);
    bool r = ReadOld(*fs);  delete fs;  return r; }

  bool Write(ostream*, gzFile);
  bool WriteOld(ostream&);
  bool Write()  { return Write(FileName()); }
  bool Write(const char *n)  {
    ostream *os;
    gzFile gz;
    bool o = OpenForWrite(n, os, gz);
    bool w = Write(os, gz);
    bool c = CloseAfterWrite(os, gz);
    return o&&w&&c;

    // ostream *fs = OpenWriteIOstream(n);
    // bool r = Write(*fs);  delete fs;  return r;
  }

  virtual bool AdaptWrite();

  static string DivDescriptionLine(bool, const string&, int);

  void SetDivDescription(IntVectorSet&, int) const;

  bool IsTopLevel() const { return !up; }
  bool IsMidLevel() const { return up&&down; }
  bool IsBottomLevel() const { return !down; }

  int Level() const { return IsTopLevel() ? 0 : 1+up->Level(); }

  TreeSOM *Root() { return IsTopLevel() ? this : up->Root(); }

  const TreeSOM *Root() const { return ((TreeSOM*)this)->Root(); }

  void SetLevels(int i) {
    if (i<1) {
      ShowError("TreeSOM::SetLevels less than one");
      SimpleAbort();
    }
    delete down;
    while(--i>0)
      AddLevel();
  }

  void AddLevel(TreeSOM *l = NULL) {
    if (!l) {
      l = new TreeSOM();
      l->NoDataRead(NoDataRead());
      l->RecordSeekIndex(RecordSeekIndex());
    }
    
    if (down)
      down->AddLevel(l);
    else {
      down = l;
      down->up = this;

      PropagateSome();
    }
  }

  void AddLevel(double xm, double ym) {
    const TreeSOM& bl = BottomLevel();
    TreeSOM *nl = new TreeSOM((int)floor(xm*bl.Width()+0.5),
			      (int)floor(ym*bl.Height()+0.5),
			      VectorLength());
    AddLevel(nl);    
  }

  void PropagateSome() {
    if (down) {
      down->TrainingSet(TrainingSet());
      down->Silent(Silent());
      down->Voting(voting_method);
      down->Metric(Metric());
      down->Topology(Topology());
      down->DistanceComparisonInterval(DistanceComparisonInterval());
      down->SkipInTraining(skiplabelsset);
      //cout << "Propagating neighkernel=" << NKernel()
      //     << " radius_base=" << RadiusBase() << endl;
      down->NKernel(NKernel());
      down->RadiusBase(RadiusBase());
      //cout << "Verify: radius_base=" << RadiusBase() << endl;
      down->TicTacSet(*this);

      down->PropagateSome();
    }
  }

  TreeSOM& BottomLevel() { return down ? down->BottomLevel() : *this; }
  const TreeSOM& BottomLevel() const {
    return down ? down->BottomLevel() : *this; }

  TreeSOM& TopLevel() { return up ? up->TopLevel() : *this; }
  const TreeSOM& TopLevel() const {
    return up ? up->TopLevel() : *this; }

  TreeSOM& Level(int i) {
    if (i<0 || (i>0 && !down)) SimpleAbort();
    return i ? down->Level(i-1) : *this; }

  const TreeSOM& Level(int i) const {
    if (i<0 || (i>0 && !down)) SimpleAbort();
    return i ? down->Level(i-1) : *this; }

  bool DivideXX(const FloatVectorSet&, const char* = NULL, bool = false);

  void Voting(VotingMethod, int = -1);
  void Silent(int, int = -1);
  int Silent() { return SOM::Silent(); }

  // bool Train(int = -1, bool = false);

  bool Train(int, bool, bool);
    
  void OldRadiusAndAlpha(int, int);

  void NewRadiusAndAlpha(int, int,int, double, double);

  void ReLabel(int = -1);

  virtual int GetNitems() const { return Level(adaptive_level).Nitems(); }

  virtual int FindBestMatch(const FloatVector& vec, float *dist,
			    IntVector *bmuvec) const {
    return LevelBestMatchingUnit(adaptive_level, vec, dist, bmuvec); }

  virtual bool AdaptSetDivide(const IntVectorSet& bmu, const FloatVector& d) {
    return Level(adaptive_level).Classifier::AdaptSetDivide(bmu, d); }

  virtual bool AdaptVote(const IntVector& bmu, const FloatVector& d,
			 const FloatVectorSet& lab) {
    return Level(adaptive_level).SOM::AdaptVote(bmu, d, lab); }

  virtual bool AdaptWriteDivide(bool lastiter=false) const;

  virtual bool AdaptVector(const FloatVector&);
  virtual void AdaptEpochDone(FloatVectorSource&);
  virtual bool BatchTrainEpochDone(const FloatVectorSet& sum,
				   const ListOf<IntVector>& r) {
    return Level(adaptive_level).SOM::BatchTrainEpochDone(sum, r); }

  int RestrictedSearch(int, const FloatVector&, float* = NULL,
		       gzFile = NULL, IntVector* = NULL) const;

  int RestrictedSearchOld(int, const FloatVector&, float* = NULL) const;

  int LevelBestMatchingUnit(int, const FloatVector&, float* = NULL,
			    IntVector* = NULL) const;

  void DoVoteLabels(int, FloatVectorSource&);

  bool InitializeFromAbove();

  bool CorrectFromLevelBelow();

  TreeSOM& TrainCount(const IntVector& tc) {
    traincount = tc; return *this; }

  TreeSOM& BmuDivDepthVector(const IntVector& dd) {
    IntVector d = dd;
    while (d.Length()<Nlevels())
      d.Prepend(1);
    for (int i=0; i<Nlevels(); i++)
      Level(i).BmuDivDepth(d[i]);

    return *this; 
  }

  TreeSOM& BmuDivDepthVector(const vector<int>& v) {
    IntVector dd(v.size());
    for (size_t i=0; i<v.size(); i++) dd[i]=v[i];
    return BmuDivDepthVector(dd);
  }

  TreeSOM& BatchTrainPeriod(const IntVector& btp) {
    batchtrainperiod = btp; return *this; }

  void AddDescriptionString(const string& k, const string& v) {
    description_extra[k] = v;
  }

  const char *StructureM() const;

  string Structure() const {
    const char *s = StructureM();
    string ret = s ? s : "";
    delete [] s;
    return ret;
  }

  IntVector FindBestMatches(const FloatVector& v,  bool use_depth=true);

  double SolveDistanceSquared(int, int, const FloatVector&,
			      const FloatVector::DistanceType*,
			      double, int, gzFile) const;

  const char *FileName() const { return up ? up->FileName() : filename; }

  void FileName(const char *name) { SOM::FileName(name); }
  void FileName(const string& n)  { FileName(n.c_str()); }

  void DeleteUmatrix() { 
    SOM::DeleteUmatrix();
    if (down)
      down->DeleteUmatrix();
  }

  void SaveUmatrix(bool s) { 
    SOM::SaveUmatrix(s);
    if (down)
      down->SaveUmatrix(s);
  }

  bool MultiResolutionCorrection() const {
    return multi_resolution_correction;
  }

  void MultiResolutionCorrection(bool m) { multi_resolution_correction = m; }

  bool ShowAQE() const {
    return show_aqe;
  }

  void ShowAQE(bool a) { show_aqe = a; }

  virtual double AverageQuantisationError(bool);
  
  bool WriteXmlDivisionFile(bool, const IntVectorSet&) const;
  
  void DoWriteDivXML(bool b) { divxml = b; }

protected:
  int adaptive_level;
  TreeSOM *up, *down;  

  IntVector traincount;

  IntVector batchtrainperiod;

  map<string,string> description_extra;

  bool multi_resolution_correction;

  bool show_aqe;
  
  bool divxml;
};

} // namespace simple
#endif // _TREESOM_H_

