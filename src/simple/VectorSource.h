// 
// Copyright 1994-2007 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2007 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: VectorSource.h,v 1.17 2014/10/28 09:41:29 jorma Exp $

// -*- C++ -*-

#ifndef _VECTORSOURCE_H_
#define _VECTORSOURCE_H_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <vector>
using namespace std;

#include <Vector.h>
#include <List.h>
#include <StatQuad.h>

#if SIMPLE_FEAT
#include <Transform.h>
#endif

#include <zlib.h>

namespace simple {

template <class Type> class VectorSetOf;
template <class Type> class MatrixOf;

//
class VectorComponent {
  // <component index="0" name="age" min-value="-1" min-label="old"
  //  max-value="1" max-label="new" mean="0.2" stdev="0.3"/>
 public:
  ///
 VectorComponent() : value(0), min_value(0), max_value(0),
    mean(0), stdev(0), index(-1) {}

  ///
  VectorComponent(xmlNodePtr xml);

  float value, min_value, max_value, mean, stdev;
  string name, min_label, max_label;
  int index;
};  // class VectorComponent
  

//- VectorSource
template <class Type> class VectorSourceOf :
virtual public SourceOf<VectorOf<Type> > {
public:
  using SourceOf<VectorOf<Type> >::ShowError;

  using SourceOf<VectorOf<Type> >::CopyString;
  using SourceOf<VectorOf<Type> >::ShowString;
  //g++ SourceOf<VectorOf<Type> >::ShowError;
  using SourceOf<VectorOf<Type> >::AppendString;
  using SourceOf<VectorOf<Type> >::Current;
  using SourceOf<VectorOf<Type> >::Nitems;
  using SourceOf<VectorOf<Type> >::First;
  using SourceOf<VectorOf<Type> >::Skip;
  using SourceOf<VectorOf<Type> >::IsXMLHeader;
  using SourceOf<VectorOf<Type> >::ConditionalAbort;
  using SourceOf<VectorOf<Type> >::FileName;
  using SourceOf<VectorOf<Type> >::next;

protected:
public:          // added 16.2.2001
  VectorSourceOf(int l = 0) {
    vectorlength = l;
    select_label = NULL;
    // preprocess = NULL;
    binary = FALSE;
    zipped = -1;
    sparse = false;
    description = NULL;
    gzip_pipe = NULL;
    cv_begin = cv_end = 0;
    cv_test = FALSE;
    transform = NULL;
    nodataread = false;
    precision = 0;
    xmldoc = NULL;
    write_labels_only = false;
    record_seek_index = false;
    InitializeMinMaxNumber();
  }

  VectorSourceOf(const VectorSourceOf<Type>& s)
    : SourceOf<VectorOf<Type> >(s) {  // added 1.7.2004

    vectorlength = s.vectorlength;
    description = (char*)CopyString(s.description);
    cv_begin = s.cv_begin;
    cv_end   = s.cv_end;
    cv_test  = s.cv_test;
    binary = s.binary;
    zipped = s.zipped;
    sparse = s.sparse;
    nodataread = s.nodataread;
    value = s.value;
    write_labels_only = s.write_labels_only;
    record_seek_index = s.record_seek_index;
    seek_index = s.seek_index;
    max_number = s.max_number;
    min_number = s.min_number;
    precision  = s.precision;
    components = s.components;

    select_label = (char*)CopyString(s.select_label);

    gzip_pipe = NULL;
    transform = NULL;
    xmldoc    = NULL;
  }

public: 
  virtual ~VectorSourceOf() {
    delete [] select_label;
    delete [] description;
    // ClosePipe();
    if (xmldoc)
      xmlFreeDoc(xmldoc);
  }

  virtual void Dump(Simple::DumpMode = Simple::DumpDefault, ostream& = cout) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "VectorSource", NULL }; return n; }

  virtual int  VectorLength() const { return vectorlength; }
  virtual void VectorLength(int i)  { vectorlength = i;    }
  virtual void Lengthen(int) { abort(); }

  static int ReadVectorLength(const string& fname) {
    VectorSourceOf<Type> vs;
    return vs.ReadHead(fname.c_str());
  }

  const VectorOf<Type> *GetNumber(int i) {
    for (int nul = 0; nul<2;) {
      const VectorOf<Type> *vec = Next();
      if (!vec || vec->Number()>i) {
	nul++;
	First();
	continue;
      }
      if (vec->Number()==i)
	return vec;
    }
    return NULL;
  }

  virtual const VectorOf<Type> *Next()  {
    for (;;) {
      const VectorOf<Type> *vec = SourceOf<VectorOf<Type> >::Next();
      if (!vec)
	return NULL;
      if (SelectOK(*vec))
	return PreProcess(vec);
    }
  }
    
  virtual void PreNext() {
    if (!NextIsUsed()) {
      if (next>=cv_begin && next<cv_end)
	Skip(cv_end-next);
      else if (next<cv_begin)
	Skip(cv_begin-next);
      else if (next>=cv_end)
	Skip(Nitems()-next);
    }
  }

  int SelectOK(const VectorOf<Type>& v) {
    return !select_label || v.LabelsMatch(select_label); }

  const char *SelectLabel() const { return select_label; }
  void SelectLabel(const char *l) { CopyString(select_label, l); }

//   void PreProcess(const VectorOf<Type>*(*f)(const VectorOf<Type>*)) {
//     preprocess = f; }
//   const VectorOf<Type> *(*PreProcess())(const VectorOf<Type>*) {
//     return preprocess; }

  const VectorOf<Type> *PreProcess(const VectorOf<Type> *v) {
#if SIMPLE_FEAT
    if (transform) {
      static VectorOf<Type> ret;
      ret = Transform(*v);
      return &ret;
    }
#endif // SIMPLE_FEAT
    return v;
  }

  void SetDescription(const char *txt) { CopyString(description, txt); }
  void AppendDescription(const char *txt) { AppendString(description, txt); }
  const char *GetDescription() const { return description; }
  const char *GetDescription(const char *txt) const;

  virtual int AllowLocateFile() { return TRUE; }

//   int Read(istream&);
//   int Read(const char *n, int = -1);

  static bool GetLine(istream*, gzFile, char*, int, bool&);

  static bool GetLine(istream*, gzFile, const char*&);

  static bool PutLine(ostream*, gzFile, const char*);

  static bool PutString(ostream*, gzFile, const char*);

  static bool PutChar(ostream*, gzFile, char);

  static bool IsGood(istream *isp, gzFile gz) {
    if (bool(isp)==bool(gz))
      return false;
    return (isp&&isp->good()) || (gz&&!gzeof(gz)); }

  static bool IsGood(ostream *osp, gzFile gz) {
    if (bool(osp)==bool(gz))
      return false;
    return (osp&&osp->good()) || (gz&&!gzeof(gz)); }

  static long Gpos(istream *isp, gzFile gz) {
    if (bool(isp)==bool(gz))
      return -1;
    return isp ? (long) IRIX_TELLG_CAST isp->tellg() : (long)gztell(gz); }

  static bool SeekG(istream *isp, gzFile gz, long pos) {
    if (bool(isp)==bool(gz))
      return false;
    return isp ? isp->seekg(pos).good() : gzseek(gz, pos, SEEK_SET)!=-1; }

  int ReadHead(const char *name, const char **l = NULL) {
    gzFile gz = gzopen(name, "r");
    int len = ReadHead(NULL, gz, l);  gzclose(gz);  return len; }

  int ReadHeadOld(const char *name, const char **l = NULL) {
    istream *fs = OpenReadIOstream(name);
    int len = ReadHead(fs, NULL, l);  delete fs;  return len; }

  int ReadHead(istream*, gzFile = NULL, const char** = NULL, bool = false);

  VectorOf<Type> *ReadOne(istream*, gzFile, int = 0, bool = true);

  bool OpenForWrite(const char*, ostream*&, gzFile&);
  bool CloseAfterWrite(ostream*, gzFile);

  bool Write(ostream*, gzFile);
  bool Write(const char *n);

  bool Write(const string& s) { return Write(s.c_str()); }

  bool WriteHead(ostream*, gzFile);
  bool WriteOne(const VectorOf<Type>& vec, ostream*, gzFile);
  bool Flush(ostream*, gzFile);

  int WriteGnuPlot(const char* n, int spa = FALSE) {
    ofstream os(n);
    return WriteGnuPlot(os, spa); }
  int WriteGnuPlot(ostream&, int = FALSE);

  static const char *ReadDescription(char*&, istream*, gzFile,
				     xmlDocPtr* = NULL, const char* = NULL,
				     bool = false);
  static int ReadDescriptionOld(char*&, istream*, gzFile, xmlDocPtr* = NULL);
  static bool WriteDescription(const char*, ostream*, gzFile, xmlDocPtr =NULL);

  const char *ReadDescription(istream *isp, gzFile gz, const char *fn = NULL,
			      bool permissive = false) {
    delete description; description = NULL; 
    const char *ret = ReadDescription(description, isp, gz, &xmldoc, fn,
				      permissive);
    if (xmldoc)
      InterpretXML(xmldoc);
    return ret;
  }

  bool WriteDescription(ostream *osp, gzFile gz) {
    return WriteDescription(description, osp, gz);}
  
  int Binary() const            { return binary;            }
  VectorSourceOf& Binary(int b) { binary = b; return *this; }
  int Zipped() const            { return zipped;            }
  VectorSourceOf& Zipped(int z) { zipped = z; return *this; }

  /// Returns true if source is sparse coded.
  bool Sparse() const { return sparse; }

  /// Sets sparseness of the sourec.
  void Sparse(bool s) { sparse = s; }

  int Nvalues() const     { return value.Nitems(); }
  VectorSourceOf& AddValue() { value.Append(new FloatVector); return *this; }
  VectorSourceOf& ZeroValues() { value.Delete(); return *this; }

  VectorSourceOf& SetValue(int l, int i, double v);
  VectorSourceOf& SetValue(int l, double v) { return SetValue(l, Current(),v);}
  VectorSourceOf& SetValue(double v) { return SetValue(0, Current(), v); }

  double GetValue(int l, int i) const {
    return l>=0&&l<Nvalues()?value[l].Get(i):0; }
  double GetValue(int l) const { return GetValue(l, Current());  }
  double GetValue() const { return GetValue(0, Current()); }

  VectorOf<Type> Sum();
  VectorOf<Type> Mean();
  VectorOf<Type> GeomMean();
  VectorOf<Type> Variance(VectorOf<Type>* = NULL);
  VectorOf<Type> StandardDeviation(VectorOf<Type> *m = NULL) {
    return Variance(m).Sqrt();
  }

  MatrixOf<Type> AutoCov(VectorOf<Type>* = NULL, bool = false);
  MatrixOf<Type> AutoCorr(VectorOf<Type>* = NULL, bool = false);
  MatrixOf<Type> AutoCorrRel(VectorOf<Type>&, VectorOf<Type>* = NULL);

  MatrixOf<Type> RawAutoCov(int&, VectorOf<Type>* = NULL);
  MatrixOf<Type> RawAutoCorr(int&, VectorOf<Type>* = NULL);

  ListOf<MatrixOf<Type> > AutoCorrList(VectorSetOf<Type>* = NULL);
  ListOf<MatrixOf<Type> > AutoCovList(VectorSetOf<Type>* = NULL);

  ListOf<MatrixOf<Type> > RawAutoCorrList(IntVector&,
					  VectorSetOf<Type>* = NULL);
  ListOf<MatrixOf<Type> > RawAutoCovList(IntVector&,
					 VectorSetOf<Type>* = NULL);

  virtual const char *AdditionalPrintText() const;

  int CrossValidationBegin() const { return  cv_begin; }
  int CrossValidationEnd()   const { return  cv_end;   }
  int CrossValidationTest()  const { return  cv_test;  }
  int CrossValidationTrain() const { return !cv_test;  }

  void CrossValidationBegin(int s) { cv_begin = s; }
  void CrossValidationEnd(int s)   { cv_end   = s; }
  void CrossValidationTest(int s)  { cv_test =  s; }
  void CrossValidationTrain(int s) { cv_test = !s; }

  void CopyCrossValidationInfo(const VectorSourceOf<Type>& s) {
    CrossValidationBegin(s.CrossValidationBegin());
    CrossValidationEnd(s.CrossValidationEnd());   
    CrossValidationTest(s.CrossValidationTest());  
    CrossValidationTrain(s.CrossValidationTrain());
  }

  int NextIsUsed() const {
    return !cv_test ?
      next<cv_begin||next>=cv_end : next>=cv_begin&&next<cv_end; }

  virtual VectorSourceOf<Type> *MakeSharedCopy() const { abort(); return NULL;}

  VectorSetOf<Type> ClassMeans(VectorOf<Type>* = NULL);
  double MeanSqrDistance(const VectorOf<Type>&); 

  MatrixOf<float> IntraAndInterClassMeanSqrDistances(int = TRUE);
  FloatVector IntraClassMeanSqrDistances(const VectorSetOf<Type>&,
					 VectorOf<Type>* = NULL);

  VectorOf<Type> Transform(const VectorOf<Type>& v) const;

#if SIMPLE_FEAT
  void SetTransform(VectorTransform *vtf) { transform = vtf; }
  VectorTransform *GetTransform() { return transform; }

  void AppendTransform(VectorTransform *fe) {
    VectorTransform::Append(transform, fe);
  }

  void PrependTransform(VectorTransform *fe) {
    VectorTransform::Prepend(transform, fe);
  }

  void RemoveLastTransform() {
    VectorTransform::RemoveLast(transform);
  }

  void RemoveFirstTransform() {
    VectorTransform::RemoveFirst(transform);
  }
#endif // SIMPLE_FEAT

  bool NoDataRead() const { return nodataread; }
  bool NoDataRead(bool n) { bool t = nodataread; nodataread = n; return t; }

  xmlDocPtr XMLDescription() const { return xmldoc; }

  xmlDocPtr CopyXMLDescription(xmlDocPtr);

  xmlDocPtr CopyXMLDescription(const VectorSourceOf<Type>& s) {
    return CopyXMLDescription(s.XMLDescription());
  }

  /// Sets a flag that effects WriteOne().
  void WriteLabelsOnly(bool o) { write_labels_only = o; }

  ///
  bool WriteLabelsOnly() const { return write_labels_only; }

  ///
  bool RecordSeekIndex() const { return record_seek_index; }

  ///
  void RecordSeekIndex(bool r) { record_seek_index = r; }

  ///
  bool PossiblyRecordSeekIndex(int i, istream *isp, gzFile gz);

  ///
  VectorOf<Type> *SeekAndReadOne(int, istream*, gzFile) const;

  void InitializeMinMaxNumber() {
    max_number = -MAXINT;
    min_number = MAXINT;
  }

  ///
  int MaxNumber() { return max_number; }
  
  ///
  int MinNumber() { return min_number; }
  
  ///
  void CheckMinMaxNumber(int i) {
    if (i>max_number)
      max_number = i;
    if (i<min_number)
      min_number = i;
  }

  ///
  void Precision(int p) { precision = p; }

  ///
  int Precision() const { return precision; }

  ///
  bool InterpretXML(xmlDocPtr);

  ///
  const vector<VectorComponent>& ComponentDescriptions() const {
    return components;
  }

  ///
  void AppendComponentDescription(const VectorComponent& c) {
    components.push_back(c);
  }

  ///
  int GetComponentIndex(const string& name) const {
    for (vector<VectorComponent>::const_iterator
	   i=components.begin(); i!=components.end(); i++) {
      if (i->name == name)
	return i->index;
    }
    return -1;
  }

  //
  xmlNodePtr FindOrAddFeatureComponents();

  //
  xmlNodePtr FindOrAddFeatureComponents(xmlDocPtr);

  //
  static xmlNodePtr SetFeatureComponentMeanStDev(xmlNodePtr, double, double);

protected:
  /// Open possibly decompressing pipe for reading.
  istream *OpenReadIOstream(const char*);

  /// Open possibly compressing pipe for writing.
  ostream *OpenWriteIOstream(const char*);

  int vectorlength;
  char *description;

  int cv_begin, cv_end, cv_test;

  // iostream  *OpenPipe(const char*, int = FALSE);
  // void ClosePipe();
  int binary, zipped;
  bool sparse;
  bool nodataread;

  int precision;

  FILE *gzip_pipe;

  ListOf<FloatVector> value;

  char *select_label;
  //  const VectorOf<Type> *(*preprocess)(const VectorOf<Type>*);

#if SIMPLE_FEAT
  VectorTransform *transform;
#else
  void *transform;
#endif

  xmlDocPtr xmldoc;

  bool write_labels_only;

  bool record_seek_index;

  IntVector seek_index;

  int max_number, min_number;

  vector<VectorComponent> components;
};

typedef VectorSourceOf<int> IntVectorSource;
typedef VectorSourceOf<float> FloatVectorSource;
typedef VectorSourceOf<SimpleComplex> SimpleComplexVectorSource;
typedef VectorSourceOf<StatVar> StatVarVectorSource;
typedef VectorSourceOf<StatQuad> StatQuadVectorSource;

#ifdef EXPLICIT_INCLUDE_CXX
#include <VectorSource.C>
#endif // EXPLICIT_INCLUDE_CXX

} // namespace simple
#endif // _VECTORSOURCE_H_

