// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: DataSet.C,v 1.3 2009/11/20 20:48:15 jorma Exp $

#include <DataSet.h>

namespace simple {

///////////////////////////////////////////////////////////////////////////

DataSet::DataSet(const DataSet& s) : SourceOf<FloatVector>(s),
				     FloatVectorSource(s) {
  Initialize();
  FileName(s.FileName());
  writing = s.writing;
  counted_nitems = s.counted_nitems;
  forced_vectorlength = s.forced_vectorlength;
  
  if (FileName() && !writing)
    Rewind();
}

///////////////////////////////////////////////////////////////////////////

DataSet::DataSet(const char* n, int wr) {
  Initialize();
  FileName(n);
  writing = wr;

  if (FileName() && !writing)
    Rewind();
}

///////////////////////////////////////////////////////////////////////////

void DataSet::Initialize() {
  writing = FALSE;
  in_file = NULL;
  out_file = NULL;
  gz = NULL;
  counted_nitems = -1;
  forced_vectorlength = -1;
  call_func = NULL;
  call_data = NULL;
}

///////////////////////////////////////////////////////////////////////////

DataSet::~DataSet() {
  delete in_file;
  delete out_file;
  if (gz)
    gzclose(gz);
}

///////////////////////////////////////////////////////////////////////////

void DataSet::Dump(Simple::DumpMode dt, ostream& os) const {
  if (dt&&DumpRecursive)
    FloatVectorSource::Dump(dt, os);

  os << "  DataSet "            << (void*)this
     << " counted_nitems="      << counted_nitems
     << " forced_vectorlength=" << forced_vectorlength
     << " writing="             << writing
     << endl;
}

///////////////////////////////////////////////////////////////////////////

FloatVector* DataSet::Get(int n) {
  if (n==MAXINT)
    n = NextIndex();
  
  if (n<NextIndex())
    First();

  if (n>NextIndex())
    Skip(n-NextIndex());

  FloatVector *vec = NULL;
  
  if (!vec && (in_file || gz))
    vec = ReadOne(in_file, gz);

  if (!vec && call_func)
    vec = (FloatVector*)(*call_func)(call_data);

  if (!vec) {
    if (counted_nitems<0)
      counted_nitems = NextIndex();

    return NULL;
  }

  if (forced_vectorlength>=0)
    vec->Lengthen(forced_vectorlength);

  vector = *vec;
  delete vec;

  vector.Number(NextIndexWithPostInc());
  
  return &vector;
}

///////////////////////////////////////////////////////////////////////////

DataSet& DataSet::Rewind(int wr) {
  if (!wr && in_file)
    in_file->ignore(MAXINT);

  delete in_file;
  delete out_file;
  if (gz)
    gzclose(gz);

  in_file  = NULL;
  out_file = NULL;
  gz       = NULL;

  //file = OpenPipe(FileName(), wr);
  //file = new fstream(FileName(), ios::in);

  if (wr) {
    // cout << "Rewind() write " << endl;

    OpenForWrite(FileName(), out_file, gz);

    if (!out_file && !gz)
      ShowError("DataSet::Rewind() failed to open <", FileName(),
		"> for reading");
    else
      WriteHead(out_file, gz);

  } else if (!call_func) {
    // cout << "Rewind() read" << endl;

    VectorLength(0);
    gz = gzopen(FileName(), "r");
    if (!gz)
      ShowError("DataSet::Rewind() failed to open <", FileName(),
		"> for reading");
    VectorLength(ReadHead(in_file, gz));
  }

  if (!VectorLength())
    ShowError("DataSet::Rewind() vectorlength==0");

  NextIndex(0);

  return *this;
}

///////////////////////////////////////////////////////////////////////////

void DataSet::Append(FloatVector *vec, int del) {
  // cout << "Append() vectorlength=" << VectorLength() << endl;

  if (!VectorLength()) {
    VectorLength(vec->Length());
    Rewind(TRUE);
  }

  WriteOne(*vec, out_file, gz);
    
  if (del)
    delete vec;
}

///////////////////////////////////////////////////////////////////////////

FloatVectorSource *DataSet::MakeSharedCopy() const {
  DataSet *ret = new DataSet(*this);
  ret->CopyCrossValidationInfo(*this);

  return ret;
}

///////////////////////////////////////////////////////////////////////////

void DataSet::CountItems() {
  First();
  counted_nitems = 0;
  while (Get(counted_nitems))
    counted_nitems++;
}

///////////////////////////////////////////////////////////////////////////

// int DataSet::() {
// }

///////////////////////////////////////////////////////////////////////////

// int DataSet::() {
// }

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

} // namespace simple

