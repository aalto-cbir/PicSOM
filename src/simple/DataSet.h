// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: DataSet.h,v 1.3 2009/11/20 20:48:15 jorma Exp $

// -*- C++ -*-

#ifndef _DATASET_H_
#define _DATASET_H_

#include <Matrix.h>

//- DataSet
namespace simple {
class DataSet : public FloatVectorSource {
public:
  DataSet() { Initialize(); }
  DataSet(const DataSet&);
  DataSet(const char*, int = FALSE);

  virtual ~DataSet();

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

  virtual const char **Names() const { static const char *n[] = {
    "DataSet", "Source", NULL }; return n; }

  void CountItems();

  virtual int Nitems() const {
    if (counted_nitems<0) {
      cerr << "DataSet::Nitems() called" << endl;
      return 0;
    }
    return counted_nitems;
  }

  void SetNitems(int n) { counted_nitems = n; }

  virtual void         First() { if (NextIndex()) Rewind(); }
  virtual void Append(FloatVector*, int = TRUE);
  virtual void Prepend(FloatVector*, int = TRUE) {
    cerr << "DataSet::Prepend() called" << endl; }

  virtual FloatVector *GetNext()  { return Get(MAXINT); }

  virtual void Skip(int n) {
    while (n-->0) {
      FloatVector *vec = ReadOne(in_file, gz);
      if (!vec)
	break;
      delete vec;
      AddToNextIndex(1);
    } }
  
  virtual int  VectorLength() const {
    return forced_vectorlength>=0 ? forced_vectorlength :
      FloatVectorSource::VectorLength(); }
  virtual void VectorLength(int i)  { FloatVectorSource::VectorLength(i); }

  virtual void Lengthen(int l) { forced_vectorlength = l; }

  virtual FloatVectorSource *MakeSharedCopy() const;

  virtual void DoneWith(FloatVector *i) const { 
    cout << "DoneWith" << endl; delete i; }

//   FloatVectorSet::Description;
//   FloatVectorSet::Mean;
//   FloatVectorSet::AutoCov;
//   FloatVectorSet::RawAutoCorr;
//   FloatVectorSet::RawAutoCorrList;
//   FloatVectorSet::AutoCorr;
//   FloatVectorSet::AutoCorrRel;
//   FloatVectorSet::Binary;
//   FloatVectorSet::Zipped;

  FloatVector *Get(int /* = MAXINT */);
  DataSet& Rewind(int = FALSE);

  void SetCallback(CFP_pthread f, void *d) {
    call_func = f;
    call_data = d;
  }

protected:
  void Initialize();

  int writing, counted_nitems, forced_vectorlength;

  istream *in_file;

  ostream *out_file;

  gzFile gz;

  FloatVector vector;

  CFP_pthread call_func;
  void       *call_data;
};

} // namespace simple
#endif // _DATASET_H_

