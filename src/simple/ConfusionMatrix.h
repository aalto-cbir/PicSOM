// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: ConfusionMatrix.h,v 1.2 2009/11/20 20:48:15 jorma Exp $

// -*- C++ -*-

#ifndef _CONFUSIONMATRIX_H_
#define _CONFUSIONMATRIX_H_

#include <Vector.h>
#include <Matrix.h>

#include <string>

namespace simple {
class ConfusionMatrix : public FloatMatrix {
public:
  ConfusionMatrix() { arbitrary = 0; compress = 0.0; }
  ConfusionMatrix(const ConfusionMatrix& s) : SourceOf<FloatVector>(s),
					      FloatMatrix(s) {
    arbitrary  = s.arbitrary;
    compress = s.compress;
  }
  // virtual ~ConfusionMatrix();

  ConfusionMatrix& operator=(const ConfusionMatrix&);

  virtual void Dump(Simple::DumpMode dm = DumpDefault,
		    ostream& os = cout) const {
    if (dm&DumpRecursive)
      FloatMatrix::Dump(dm, os);
    os << Bold("    ConfusionMatrix ") << (void*)this
       << " arbitrary="  << arbitrary
       << " compress=" << compress
       << endl;
  }

  virtual const char **Names() const { static const char *n[] = {
    "ConfusionMatrix", "Matrix", "VectorSet", "List", NULL }; return n; }

  virtual int AllowLocateFile() { return TRUE; }

  void Clear();
  void Sort() const;

  void Add(const ConfusionMatrix&);

  void Add(const char*, const char*, double = 1);

  void Add(const std::string& a, const std::string& b, double v = 1) {
    Add(a.c_str(), b.c_str(), v);
  }

  void AddLabel(const char*);
  int  HasLabel(const char *lab)  const { return FindIndex(lab)!=-1; }
  int  FindIndex(const char *lab) const;

  void AddArbitrary()    { arbitrary++; }
  void ZeroArbitrary()   { arbitrary = 0; }
  int  Arbitrary() const { return arbitrary; }
  void Arbitrary(int a)  { arbitrary = a; }

  double Compress() const { return compress; }
  void Compress(double c) { compress = c; }

  double Get(const char *src, const char *dst) const {
    int s = FindIndex(src), d = FindIndex(dst);
    return (s>=0 && d>=0) ? FloatMatrix::Get(s, d) : 0;
  }

  void Set(const char *src, const char *dst, double v) {
    double r = Get(src, dst);
    Add(src, dst, v-r);
  }

  const char *Label(int i) const { return GetColumn(i)->Label(); }
  int Labels() const { return Columns(); }

  void Display(ostream& os = cout, const char** = NULL) const;
  void DisplayWide(ostream& os = cout, const char** = NULL) const;
  void DisplayCompress(ostream& os = cout, const char** = NULL) const;
  void DisplayAccuracy(int = 0, ostream& os = cout,
		       const ConfusionMatrix* = NULL) const;

  double WrongDestination(int) const;
  double TotalDestination(int) const;
  double WrongSource(int) const;
  double TotalSource(int) const;

  FloatVector WrongDestination() const;
  FloatVector TotalDestination() const;
  FloatVector WrongSource() const;
  FloatVector TotalSource() const;

  double Total() const { return Sum(); }
  double Correct() const;
  double Erroneous() const;
  double Rejected() const;
  double ErrorRatio() const {
    return Erroneous() ? 100*Erroneous()/(Total()-Rejected()) : 0; }

  double CostRatio(const ConfusionMatrix& cost) const {
    double ret = 0;
    for (int i=0; i<Columns(); i++)
      for (int j=0; j<Rows(); j++)
	ret += FloatMatrix::Get(i, j)*cost.Get(Label(i), Label(j));

    return Total() ? ret/Total() : 0;
  }

  double RejectionRatio()    const { return 100* Rejected()/Total(); }
  double RecognitionRatio()  const { return 100*  Correct()/Total(); }
  double SubstitutionRatio() const { return 100*Erroneous()/Total(); }
  double ReliabilityRatio()  const {
    return Correct()+Erroneous() ? 100*Correct()/(Correct()+Erroneous()) : 0; }

  int HasRejection() const { return FindIndex(NULL)>=0; }

  const IntVector& Impulses() const { return impulses; }

protected:
  int   arbitrary;
  float compress;
  IntVector impulses;
};

} // namespace simple
#endif // _CONFUSIONMATRIX_H_

