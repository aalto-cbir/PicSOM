// -*- C++ -*-  $Id: VectorSource.C,v 1.19 2012/08/20 08:41:25 jorma Exp $
// 
// Copyright 1994-2012 Jorma Laaksonen <jorma.laaksonen@aalto.fi>
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _VECTORSOURCE_C_
#define _VECTORSOURCE_C_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <VectorSource.h>
#include <Matrix.h>
#include <XMLutil.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#include <ctype.h>

#ifdef HAVE_WINSOCK2_H
#undef GetProp
#endif // HAVE_WINSOCK2_H

namespace simple {

#ifdef sgi
#pragma can_instantiate bool VectorSourceOf<StatQuad>::Write(const char*)
#endif // sgi

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// VectorSourceOf<Type>::~VectorSourceOf() {
//   delete select_label;
//   delete description;
//   ClosePipe();
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSourceOf<Type>::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&&Simple::DumpRecursive)
    SourceOf< VectorOf<Type> >::Dump(type, os);

  if (type&Simple::DumpShort || type&Simple::DumpLong) {
    os << " VectorSource "     	<< (void*)this
       << " vectorlength="     	<< vectorlength << "(" << VectorLength() << ")"
       << " select_label="     	<< ShowString(select_label)
     //<< " preprocess="       	<< (void*)preprocess
       << " description="      	<< ShowString(description)
       << " nodataread="       	<< nodataread
       << " binary=" 	       	<< binary
       << " zipped=" 	       	<< zipped
       << " sparse=" 	       	<< sparse
       << " write_labels_only=" << write_labels_only
       << " record_seek_index=" << record_seek_index
       << " cv_begin="         	<< cv_begin
       << " cv_end="           	<< cv_end
       << " cv_test="          	<< cv_test
       << " transform="        	<< (void*)transform
       << endl;

#if SIMPLE_FEAT
    if (transform)
      transform->Dump(type, os);
#endif // SIMPLE_FEAT
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::GetLine(istream *isp, gzFile gz, char *line,
				   int len, bool& toolong) {
  if (bool(isp)==bool(gz))
    return false;
  
  if (isp) {
    isp->getline(line, len);
    streamsize gcount = isp->gcount();
    toolong = gcount==len;

    return *isp && gcount;
  }

  if (!gzgets(gz, line, len))
    return false;

  size_t l = strlen(line);
  if (/*gzeof(gz) ||*/ !l)
    return false;

  toolong = false;
  if (line[l-1]=='\n')
    line[l-1] = 0;
  else
    toolong = true;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::GetLine(istream *isp, gzFile gz, 
				   const char*& line) {
  int len = 1024*1024;
  char *tmp = new char[len];

  line = NULL;
  for (;;) {
    bool toolong;
    if (!GetLine(isp, gz, tmp, len, toolong)) {
      delete [] tmp;
      delete [] line;
      line = NULL;
      return false;
    }

    AppendString(line, tmp);
    if (!toolong) {
      delete [] tmp;
      return true;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::PutLine(ostream *osp, gzFile gz, 
				   const char *line) {
  if (!PutString(osp, gz, line))
    return false;

  if (osp) {
    // *osp << endl;
    *osp << "\n";
    return osp->good();
  }

  if (gzputc(gz, '\n')==-1)
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::PutString(ostream *osp, gzFile gz, 
				     const char *line) {
  if (bool(osp)==bool(gz))
    return ShowError("VectorSource::PutString() bool(osp)==bool(gz)");

  if (osp) {
    *osp << line;
    return osp->good();
  }

  if (gzputs(gz, line)==-1)
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::PutChar(ostream *osp, gzFile gz, char ch) {
  if (bool(osp)==bool(gz)) {
    ShowError("VectorSource::PutChar() bool(osp)==bool(gz)");
    return false;
  }

  if (osp) {
    *osp << ch;
    return osp->good();
  }

  return gzputc(gz, ch)!=-1;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
xmlDocPtr VectorSourceOf<Type>::CopyXMLDescription(xmlDocPtr xml) {
  if (xmldoc)
    xmlFreeDoc(xmldoc);

  xmldoc = xml ? xmlCopyDoc(xml, 1) : NULL;

  return xmldoc;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::WriteDescription(const char *d, ostream *osp,
					    gzFile gz, xmlDocPtr xml) {
  int s = 1;

  for (const char *ptr=d; ptr && *ptr; ptr++) {
    if (s) {
      PutChar(osp, gz, '#');
      PutChar(osp, gz, ' ');
      s = 0;
    }
    PutChar(osp, gz, *ptr);
    if (*ptr=='\n')
      s = 1;
  }
  if (!s)
    PutChar(osp, gz, '\n');

  if (xml) {
    int tree_was = xmlIndentTreeOutput;
    xmlIndentTreeOutput = true;
    xmlChar *mem;
    int size;
    xmlDocDumpFormatMemory(xml, &mem, &size, true);
    xmlIndentTreeOutput = tree_was;
    WriteDescription((char*)mem, osp, gz, NULL);
    delete mem;
  }

  return IsGood(osp, gz);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const char *VectorSourceOf<Type>::ReadDescription(char*& d, istream *isp,
						  gzFile gz, xmlDocPtr *xml,
						  const char *fname,
						  bool permissive) {
  const char *ret = NULL;
  const int linesize = 20000000;
  char *line = new char[linesize];

  for (int n=0; true; n++) {
    bool toolong, ok = GetLine(isp, gz, line, linesize, toolong);
    if (!ok) {
      if (n!=0 || !permissive)
	ShowError("VectorSource::ReadDescription() GetLine() failed for <",
		  fname?fname:"", ">");
      delete [] line;
      return NULL;
    }
    if (toolong) {
      ShowError("VectorSource::ReadDescription() line too long for <",
                fname?fname:"", ">");
      delete [] line;
      return NULL;
    }
    
    if (line[0]=='#' && (!line[1] || line[1]==' ')) {
      if (line[1]==' ')
	AppendString(d, line+2);
      AppendString(d, "\n");

    } else {
      ret = CopyString(line);
      break;
    }
  }
  delete [] line;

  if (xml) {
    char *ptr = d;
    while (ptr && isspace(*ptr))
      ptr++;

    // added 4.11.2007
    if (ptr && !strncmp(ptr, "This is a TreeSOM file", 22)) {
      char *xptr = strchr(ptr, '<');
      if (xptr)
	ptr = xptr;
    }

    if (IsXMLHeader(ptr)) {
      char rootelement[1000] = "</";
      const char *lt = ptr+1;
      while ((lt = strchr(lt, '<')))
	if (isalpha(lt[1])) {
	  const char *end = strpbrk(lt, " \t\n>");
	  if (end && end-lt+3<(int)sizeof rootelement) {
	    strncpy(rootelement+2, lt+1, end-lt-1);
	    strcpy(rootelement+(end-lt+1), ">");
	  }
	  break;
	} else
	  lt++;

      if (strlen(rootelement)==2) {
	ShowError("VectorSource::ReadDescription() cound not locate XML root");
	return NULL;
      }

      // cout << rootelement << endl;

      char *eroot = strstr(ptr, rootelement);
      eroot = eroot ? strchr(eroot, '\n') : NULL;
      if (!eroot) {
	ShowError("VectorSource::ReadDescription() cound not locate XML /root");
	return NULL;
      }
      
      size_t len = eroot-ptr+1;

      if (*xml)
	xmlFreeDoc(*xml);

      char *xmlbuf = (char*)CopyString(ptr);
      xmlbuf[len] = 0;
      *xml = xmlParseMemory(xmlbuf, (int)len);
      delete [] xmlbuf;
    }
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSourceOf<Type>::ReadDescriptionOld(char*& d, istream *isp,
					     gzFile /*gz*/, xmlDocPtr *xml) {

  istream& is = *isp;

  char line[10240];

  for (;;) {
    int c = is.get();
    if (c!='#') {
      is.putback((char)c);
      break;
    }    
    c = is.get();
    if (c!=' ' && c!= '\n') {
      is.putback('#');
      is.putback((char)c);
      break;
    }

    if (c!='\n') {
      is.getline(line, sizeof line);
      if (is.gcount()==sizeof(line)-1) {
	ShowError("VectorSourceOf::ReadDescription line was too long.");
	break;
      }
      AppendString(d, line);
    }
    AppendString(d, "\n");
  }

  if (xml) {
    char *ptr = d;
    while (ptr && isspace(*ptr))
      ptr++;

    if (IsXMLHeader(ptr)) {
      size_t len = strlen(ptr);

      const char *nl = ptr;
      while ((nl = strchr(nl, '\n')))
	if (!strncmp(nl+1, "</", 2))
	  break;
	else
	  nl++;

      if (nl) {
	nl = strchr(nl+1, '\n');
	len -= strlen(nl)-1;
      }

      if (*xml)
	xmlFreeDoc(*xml);

      char *xmlbuf = (char*)CopyString(ptr);
      xmlbuf[len] = 0;
      *xml = xmlParseMemory(xmlbuf, (int)len);
      delete xmlbuf;
    }
  }

  return is.good();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::OpenForWrite(const char *name, ostream*& os,
					gzFile& gz) {
  if (Zipped()==-1)
    Zipped(false);

  if (zipped) {
    gz = gzopen(name, "w");
    os = NULL;
    return gz;
  }

  gz = NULL;
  os = new ofstream(name);
  return os && *os;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::CloseAfterWrite(ostream *os, gzFile gz) {
  if (bool(os)==bool(gz)) {
    ShowError("VectorSource::CloseAfterWrite() : bool(os)==bool(gz)");
    return false;
  }

  if (os)
    delete os;
  else
    gzclose(gz);

  return true;
}
///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::Write(const char *name) {
//   ostream *fs = OpenWriteIOstream(name);
//   int r = Write(fs, NULL);
//   delete fs;

  ostream *os;
  gzFile gz;
  bool o = OpenForWrite(name, os, gz);
  bool w = Write(os, gz);
  bool c = CloseAfterWrite(os, gz);

  return o&&w&&c;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::Write(ostream *osp, gzFile gz) {
  if (!Nitems() || !WriteHead(osp, gz))
    return false;

  const VectorOf<Type> *vec;
  First();
  while ((vec = Next()))
    if (!WriteOne(*vec, osp, gz))
      return false;

  return IsGood(osp, gz);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::WriteHead(ostream *osp, gzFile gz) {
  if (!WriteDescription(description, osp, gz))
    return false;
 
  char line[100];
  sprintf(line, "%d%s%s", VectorLength(), sparse?" sparse":"",
	  binary?" bin":"");

  bool ok = PutLine(osp, gz, line);

  Flush(osp, gz);

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::Flush(ostream *osp, gzFile /*gz*/) {
  if (osp)
    osp->flush();
  else {
    // gzflush(gz, ???);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::WriteOne(const VectorOf<Type>& vec, ostream *osp,
				    gzFile gz) {
  if (binary && !osp) {
    ShowError("VectorSource::WriteOne() Vector::WriteBinary x gzFile");
    return false;
  }

  if (binary)
    return vec.WriteBinary(*osp);

  const VectorOf<Type> *v = &vec;
  VectorOf<Type> copy;
  if (write_labels_only) {
    copy = vec;
    copy.Zero();
    v = &copy;
  }
  const char *line = v->WriteToString(sparse||write_labels_only, Precision());
  if (!line)
    return false;

  bool r = PutLine(osp, gz, line);
  delete [] line;

  return r;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSourceOf<Type>::WriteGnuPlot(ostream& os, int sparse) {
  First();
  const VectorOf<Type> *vec;
  while ((vec = Next())) {
    vec->Write(os);
    if (sparse)
      os << endl;
  }
    
  return os.good();
}
  
///////////////////////////////////////////////////////////////////////////////

template <class Type>
istream *VectorSourceOf<Type>::OpenReadIOstream(const char *name) {
  ShowError("VectorSource::OpenReadIOstream() should be obsoleted");

  if (Zipped()==-1) {
    ifstream is(name);
    char magic[3] = "XX";
    const char *zipmagic = "\037\213"; // oli \037\236
    if (!is.good())
      ShowError("VectorSourceOf<Type>::OpenReadIOstream(", name, ") failed");
    else
      is.read(magic, (int)strlen(magic));
    Zipped(is && !strcmp(magic, zipmagic));
  }

#ifdef HAS_ZFSTREAM
  return new gzifstream(name);

#else
  if (Zipped()) {
    cerr << endl << "ZIPPED files not supported"
	 << endl << endl;
    SimpleAbort();
  }
  return new ifstream(name);
#endif // HAS_ZFSTREAM
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
ostream *VectorSourceOf<Type>::OpenWriteIOstream(const char *name) {
  ShowError("VectorSource::OpenWriteIOstream() should be obsoleted");

  if (Zipped()==-1)
    Zipped(false);

#ifdef HAS_ZFSTREAM
  if (Zipped())
    return new gzofstream(name);
  else
    return new ofstream(name);

#else
  if (Zipped()) {
    cerr << endl << "ZIPPED files not supported"
	 << endl << endl;
    SimpleAbort();
  }
  return new ofstream(name);
#endif // HAS_ZFSTREAM
}

///////////////////////////////////////////////////////////////////////////////
/*
template <class Type>
iostream *VectorSourceOf<Type>::OpenPipe(const char *name, int wr) {
  static bool first = true;
  if (first) {
    ShowError("USE of VectorSource::OpenPipe() is DISCOURAGED !!!");
    // TrapMeHere();
    first = false;
  }

  if (!wr && AllowLocateFile()) 
    name = LocateFile(name);
  
  ClosePipe();

  char line[1024] = "";
  if (wr) {
    if (Zipped()==-1)
      Zipped(FALSE);

    if (Zipped())
      sprintf(line, "gzip > %s", name);

  } else {
    if (zipped==-1) {
//       sprintf(line, "gzip -t %s 2> /dev/null", name);
//       Zipped(system(line)==0);
      ifstream is(name);
      char magic[] = "XX";
      const char *zipmagic = "\037\213"; // oli \037\236
      is.read(magic, strlen(magic));
      Zipped(is && !strcmp(magic, zipmagic));
    }
    if (Zipped())
      sprintf(line, "gunzip -cf %s", name);
    else 
      *line = 0;
  }

  fstream *fs = new fstream;
  iostream *ios = fs;

  if (*line) {
    // #ifndef __GNUC__
    do {
      gzip_pipe = popen(line, wr ? "w" : "r");
      if (!gzip_pipe) {
	cerr << "VectorSource::OpenPipe failed: " << line
	     << " trying it over later..." << endl;
	sleep(10);
      }
    } while (!gzip_pipe);

    fs->attach(fileno(gzip_pipe));

  } else
    fs->open(name, wr ? ios::out : ios::in);

  return ios;
}
*/
///////////////////////////////////////////////////////////////////////////////
/*
template <class Type>
void VectorSourceOf<Type>::ClosePipe() {
  static bool first = true;
  if (first) {
    ShowError("USE of VectorSource::ClosePipe() is DISCOURAGED !!!");
    first = false;
    // TrapMeHere();
  }

  if (gzip_pipe)
    pclose(gzip_pipe);
  gzip_pipe = NULL;
}
*/
///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSourceOf<Type>::ReadHead(istream *isp, gzFile gz, const char **l,
				   bool permissive) {
  const char *line = ReadDescription(isp, gz, FileName(), permissive);
  if (!line) {
    if (!permissive)
      ShowError("VectorSourceOf::ReadDescription failed, filename=[",
		FileName(), "]");
    return 0;
  }

  if (l) {
    *l = line;
    return 1;
  }

  binary = sparse = false;

  int len = 0;
  sscanf(line, "%d", &len);
  if (len) {
    binary = strstr(line, " bin")!=NULL;    
    sparse = strstr(line, " sparse")!=NULL;    
  }

  delete [] line;

  return len;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> *VectorSourceOf<Type>::ReadOne(istream *isp, gzFile gz,
					      int len, bool record) {
  if (binary && !isp) {
    ShowError("VectorSource::ReadOne() Vector::ReadBinary x gzFile");
    return NULL;
  }

  VectorOf<Type> *vec = new VectorOf<Type>(len ? len : vectorlength,
					   NULL, NULL, nodataread);

  // cout << "ReadOne ";
  if (record && !PossiblyRecordSeekIndex(next, isp, gz)) {
    // perhaps next is never ++'ed
    ShowError("VectorSource::ReadOne() : PossiblyRecordSeekIndex() failed");
    return NULL;
  }

  if (binary) {
    if (!isp) {
      ShowError("VectorSource::ReadOne() Vector::ReadBinary x gzFile");
      delete vec;
      return NULL;
    }
    if (vec->ReadBinary(*isp)) {
      delete vec;
      return NULL;
    }
    return vec;
  }

  const char *line;
  if (!GetLine(isp, gz, line)) {
    // ShowError("VectorSource::ReadOne() GetLine() failed");
    delete vec;
    return NULL;
  }

  if (!vec->ReadFromString(line, sparse)) {
    delete vec;
    vec = NULL;
  }
  delete [] line;
  
  return vec;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> *VectorSourceOf<Type>::SeekAndReadOne(int idx,
						     istream *isp,
						     gzFile gz) const {

  // cout << "??? " << (void*)this << " " << idx << endl;

  if (!RecordSeekIndex() || !seek_index.IndexOK(idx)) {
    ShowError("VectorSource::SeekAndReadOne() no index");
    return NULL;
  }

  int pos = seek_index[idx];
  if (pos<0 || !SeekG(isp, gz, pos)) {
    ShowError("VectorSource::SeekAndReadOne() cannot seek");
    return NULL;
  }

  VectorSourceOf<Type> *foo = (VectorSourceOf<Type>*)this;

  bool nodata_was = foo->NoDataRead();
  foo->NoDataRead(false);
  VectorOf<Type> *v = foo->ReadOne(isp, gz, 0, false);
  foo->NoDataRead(nodata_was);

  return v;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::PossiblyRecordSeekIndex(int i,
						   istream *isp, gzFile gz) {
  if (!record_seek_index)
    return true;

  if (i<0)
    return false;

  long pos = Gpos(isp, gz);
  if (pos<0)
    return false;
    
  if (!seek_index.IndexOK(i)) {
    IntVector empty(i-seek_index.Length()+1000);
    empty.Set(-1);
    seek_index.Append(empty);
  }
    
  if (seek_index[i]!=-1)
    return false;

  seek_index[i] = pos;      

  // cout << "*** " << (void*)this << " " << i << " " << pos << endl;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSourceOf<Type>::Sum() {
  VectorOf<Type> sum(VectorLength());

  First();
  for (const VectorOf<Type> *v; (v = Next());)
    sum += *v;

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSourceOf<Type>::Mean() {
  VectorOf<Type> mean(VectorLength());

  int n = 0;
  First();
  for (const VectorOf<Type> *v; (v = Next()); n++)
    mean += *v;

  if (n)
    mean.Divide(n);

  return mean;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSourceOf<Type>::GeomMean() {
  VectorOf<Type> mean(VectorLength());
  mean.One();

  int n = 0;
  First();
  for (const VectorOf<Type> *v; (v = Next()); n++)
    for (int i=0; i<mean.Length(); i++)
      mean[i] *= v->Get(i);

  for (int ii=0; ii<mean.Length(); ii++)
    mean[ii] = double(mean[ii])>0?(Type)pow((double)mean[ii], 1.0/n):(Type)0;

  return mean;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSourceOf<Type>::Variance(VectorOf<Type> *mean) {
  VectorOf<Type> sum(VectorLength()), sqr(VectorLength());

  if (mean)
    mean->Length(VectorLength());

  int n = 0;
  First();
  for (const VectorOf<Type> *v; (v = Next()); n++) {
    sum += *v;
    sqr += v->Sqr();
  }
  if (n>1) 
    for (int i=0; i<sum.Length(); i++) {
      if (mean)
	(*mean)[i] = Type(double(sum[i])/n);

      Type q = Type((double(sqr[i])-double(sum[i]*sum[i])/n)/(n-1));
      // cout << q << " ";
      sum[i]= double(q)>0 ? q : (Type)0;
    }
  else {
    if (mean)
      *mean = sum;
    sum.Zero();
  }
  
  // cout << endl << "Variance... n=" << n << endl;

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> VectorSourceOf<Type>::RawAutoCov(int& i, VectorOf<Type> *m) {
  int l = VectorLength();

  VectorOf<Type> mean(l);
  MatrixOf<Type> corr = RawAutoCorr(i, &mean);

  corr.AddOuterProduct(mean, -1.0/i);

  if (m)
    *m = mean;

  return corr;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> VectorSourceOf<Type>::AutoCov(VectorOf<Type> *m, bool unb) {
  VectorOf<Type> mean(VectorLength());
  MatrixOf<Type> corr = AutoCorr(&mean, unb);
  // mean.Dump(DumpLong);

  corr.AddOuterProduct(mean, -1);

  if (m)
    *m = mean;

  // corr.Dump(DumpLong);

  return corr;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> VectorSourceOf<Type>::RawAutoCorr(int& i, VectorOf<Type> *m) {
  int l = VectorLength();
  MatrixOf<Type> corr(l, l);

  if (m)
    m->Lengthen(l);

  First();
  for (i=0;; i++) {
    const VectorOf<Type> *vec = Next();
    if (!vec)
      break;

    // vec->Dump(DumpLong);

    Type *v = (Type*)*vec;

    for (int j=0; j<l; j++) {
      Type *col = (Type*)*corr.GetColumn(j);
      for (int k=j; k<l; k++)
	col[k] += v[j]*v[k];
    }

    if (m)
      *m += *vec;
  }

  for (int j=0; j<l; j++) 
    for (int k=j+1; k<l; k++)
      corr.Set(j, k, corr.Get(k, j));

  // corr.Dump(DumpLong);
  
  return corr;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> VectorSourceOf<Type>::AutoCorr(VectorOf<Type> *m, bool unb) {
  int i;
  MatrixOf<Type> corr = RawAutoCorr(i, m);

  if (i) {
    if (unb)
      if (i==1)
	ShowError("VectorSource::AutoCorr() unbiased and i==1");
      else
	corr.Divide(i-1);
    else
      corr.Divide(i);

    if (m)
      m->Divide(i);
  }

  // corr.Dump(DumpLong);

  return corr;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<Type> VectorSourceOf<Type>::AutoCorrRel(VectorOf<Type>& m,
					      VectorOf<Type>* c) {
  int l = VectorLength();
  VectorOf<Type> mean(l);
  MatrixOf<Type> corr = AutoCorr(&mean);

  for (int j=0; j<l; j++) 
    for (int k=j; k<l; k++) {
      Type v = corr.Get(j, k)-mean[j]*m[k]-m[j]*mean[k]+m[j]*m[k];
      corr.Set(j, k, v).Set(k, j, v);
    }

  if (c)
    *c = mean;

  return corr;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<MatrixOf<Type> >
VectorSourceOf<Type>::AutoCorrList(VectorSetOf<Type> *m) {
  IntVector counts;
  ListOf<MatrixOf<Type> > ret = RawAutoCorrList(counts, m);
  for (int i=0; i<ret.Nitems(); i++) {
    ret[i] /= counts[i];
    if (m)
      (*m)[i] /= counts[i];
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<MatrixOf<Type> >
VectorSourceOf<Type>::AutoCovList(VectorSetOf<Type> *m) {
  IntVector counts;
  ListOf<MatrixOf<Type> > ret = RawAutoCorrList(counts, m);
  for (int i=0; i<ret.Nitems(); i++) {
    ret[i] /= counts[i];
    if (m)
      (*m)[i] /= counts[i];
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<MatrixOf<Type> >
VectorSourceOf<Type>::RawAutoCorrList(IntVector& counts, VectorSetOf<Type> *m){
  int l = VectorLength(), p;
  ListOf<MatrixOf<Type> > corrlist;

  if (m)
    m->Delete();

  counts.Length(0);

  First();
  for (;;) {
    const VectorOf<Type> *vec = Next();
    if (!vec)
      break;

    for (p=0; p<corrlist.Nitems(); p++)
      if (vec->LabelsMatch(corrlist[p].Label()))
	break;
    if (p==corrlist.Nitems()) {
      corrlist.Append(new MatrixOf<Type>(l, l));
      corrlist[p].Label(vec->Label());
      if (m)
	m->Append(new VectorOf<Type>(l));
      counts.Lengthen(p+1);
    }
    
    counts[p]++;

    Type *v = (Type*)*vec;

    for (int j=0; j<l; j++) {
      Type *col = (Type*)*corrlist[p].GetColumn(j);
      for (int k=j; k<l; k++)
	col[k] += v[j]*v[k];
    }

    if (m)
      (*m)[p] += *vec;
  }
  
  for (p=0; p<corrlist.Nitems(); p++)
    for (int j=0; j<l; j++) 
      for (int k=j+1; k<l; k++)
	corrlist[p].Set(j, k, corrlist[p].Get(k, j));
  
  return corrlist;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<MatrixOf<Type> >
VectorSourceOf<Type>::RawAutoCovList(IntVector& counts, VectorSetOf<Type> *m){
  VectorSetOf<Type> tmpm;
  ListOf<MatrixOf<Type> > corr = RawAutoCorrList(counts, m ? m : &tmpm);

  for (int i=0; i<corr.Nitems(); i++)
    if (m)
      corr[i].AddOuterProduct(*m->Get(i), -1.0/counts[i]);
    else
      corr[i].AddOuterProduct(tmpm[i], -1.0/counts[i]);

  return corr;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSourceOf<Type>& VectorSourceOf<Type>::SetValue(int l, int i, double v) {
  while (l>=Nvalues())
    AddValue();

  while (value[l].Length()<=i)
    value[l].Lengthen(value[l].Length()+1000);

  value[l].Set(i, v);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const char *VectorSourceOf<Type>::AdditionalPrintText() const {
  static char range[1000];

  if (cv_begin<cv_end)
    sprintf(range, "(%s: %d %d) ", CrossValidationTest()?"test":"train",
	    CrossValidationBegin(), CrossValidationEnd());
  else
    *range = 0;

  return range;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSourceOf<Type>::ClassMeans(VectorOf<Type> *all) {
  VectorSetOf<Type> means;

  if (all)
    all->Length(VectorLength());

  First();
  const VectorOf<Type> *vec;
  VectorOf<int> n;

  for (;(vec = Next());) {
    int j;
    for (j=0; j<means.Nitems(); j++)
      if (means[j].LabelsMatch(*vec))
	break;

    if (j==means.Nitems()) {
      means.Append(new VectorOf<Type>(*vec));
      n.Push(0);
    } else
      means[j] += *vec;

    n[j]++;

    if (all)
      *all += *vec;
  }

  for (int i=0; i<means.Nitems(); i++)
    means[i].Divide(n[i]);

  if (all)
    all->Divide(n.Sum());

  return means;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorSourceOf<Type>::MeanSqrDistance(const VectorOf<Type>& m) {
  First();
  const VectorOf<Type> *vec;
  int n = 0;
  double sum = 0;
  for (;(vec = Next()); n++)
    sum += m.DistanceSquaredOld(*vec);

  return sum/(float)n;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<float> VectorSourceOf<Type>::IntraAndInterClassMeanSqrDistances(int
									 nrm) {
  VectorOf<Type> mean;
  VectorSetOf<Type> classmean = ClassMeans(nrm ? &mean : NULL);

  MatrixOf<float> dist(classmean.Nitems(), classmean.Nitems());

  for (int i=1; i<classmean.Nitems(); i++)
    for (int j=0; j<i; j++)
      dist[j][i] = dist[i][j] = classmean[i].DistanceSquaredOld(classmean[j]);

  FloatVector intra = IntraClassMeanSqrDistances(classmean, nrm?&mean:NULL);

  for (int k=0; k<intra.Length(); k++)
    dist[k][k] = intra[k];

  if (nrm)
    dist.Multiply(1.0/mean[0]);

  return dist;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
FloatVector
VectorSourceOf<Type>::IntraClassMeanSqrDistances(const VectorSetOf<Type>& cm, 
						 VectorOf<Type>* all) {
  FloatVector sum(cm.Nitems());
  IntVector n(cm.Nitems());

  First();
  const VectorOf<Type> *vec;

  double allsum = 0;

  for (;(vec = Next());) {
    VectorOf<Type> *meanvec = cm.GetByLabel(vec->Label());
    int idx = cm.Find(meanvec);
    
    sum[idx] += meanvec->DistanceSquaredOld(*vec);
    n[idx]++;

    if (all)
      allsum += all->DistanceSquaredOld(*vec);
  }

  for (int i=0; i<sum.Length(); i++)
    sum[i] /= n[i];

  if (all)
    all->Length(1)[0] = (Type)(allsum/n.Sum());

  return sum;  
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const char *VectorSourceOf<Type>::GetDescription(const char *txt) const {
  static char buf[10240];
  char *ptr = description;
  size_t l = strlen(txt);

  for (;;) {
    if (!ptr || strlen(ptr)<l)
      return NULL;

    if (!strncmp(ptr, txt, l)) {
      if (!ptr[l] || ptr[l]=='\n')
	return "";

      else if (ptr[l]=='=') {
	strcpy(buf, ptr+l+1);
	ptr = strpbrk(buf, ",\n");
	if (ptr)
	  *ptr = 0;
	return buf;
      }
    }

    ptr = strpbrk(ptr, ",\n");
    if (ptr)
      ptr++;
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSourceOf<Type>::Transform(const VectorOf<Type>& v) const {
  static bool first = true;
  if (first)
    ShowError("VectorSource::Transform() of type ", v.TypeName(),
	      " not implemented");
  first = false;
  return v;
}

//----------------------------------------------------------------------

template <>
FloatVector FloatVectorSource::Transform(const FloatVector& v) const {
#if SIMPLE_FEAT
  if (transform)
    return transform->Apply(v);
#endif // SIMPLE_FEAT
  return v; 
  //  return transform ? transform->Apply(v) : v; 
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSourceOf<Type>::InterpretXML(xmlDocPtr xml) {
  using namespace picsom;  // bad, bad, bad

  // cout << "InterpretXML() root=<" << NodeName(xml->children) << ">" << endl;

  xmlNodePtr feat = xml->children;

  if (NodeName(feat)!="feature")
    feat = FindXMLchildElem(feat, "feature");

  if (!feat)
    return false;

  xmlNodePtr comp = FindXMLchildElem(feat, "components");
  if (comp) {
    // cout << "  <components> found" << endl;
    components.clear();
    for (xmlNodePtr c = comp->children; c; c = c->next) {
      if (NodeName(c) == "component")
	components.push_back(VectorComponent(c));
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
xmlNodePtr VectorSourceOf<Type>::FindOrAddFeatureComponents() {
  using namespace picsom;  // bad, bad, bad

  if (!xmldoc) {
    xmldoc = xmlNewDoc((XMLS)"1.0");
    xmlNsPtr ns = NULL;
    xmlNodePtr feat = xmlNewDocNode(xmldoc, ns, (XMLS)"feature", NULL);
    xmlDocSetRootElement(xmldoc, feat);
  }
  return FindOrAddFeatureComponents(xmldoc);
}  

///////////////////////////////////////////////////////////////////////////////

template <class Type>
xmlNodePtr VectorSourceOf<Type>::FindOrAddFeatureComponents(xmlDocPtr xml) {
  using namespace picsom;  // bad, bad, bad

  xmlNodePtr feat = NULL, ncomp = NULL;
  if (xml)
    feat = xml->children;
  if (feat && NodeName(feat)!="feature")
    feat = FindXMLchildElem(feat, "feature");
  if (feat) {
    xmlNodePtr comp = FindXMLchildElem(feat, "components");
    if (!comp)
      comp = xmlNewTextChild(feat, NULL, (XMLS)"components", NULL);
    else
      ncomp = FindXMLchildElem(comp, "component");

    if (!ncomp)
      for (int i=0; i<VectorLength(); i++) {
	xmlNodePtr a = xmlNewTextChild(comp, NULL, (XMLS)"component", NULL);
	SetProperty(a, "index", ToStr(i));
	if (!i)
	  ncomp = a;
      }
  }

  return ncomp;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
xmlNodePtr VectorSourceOf<Type>::SetFeatureComponentMeanStDev(xmlNodePtr ncomp,
							      double m,
							      double q) {
  using namespace picsom;  // bad, bad, bad

  if (ncomp) {
    SetProperty(ncomp, "mean",  ToStr(m));
    SetProperty(ncomp, "stdev", ToStr(q));

    while (ncomp) {
      ncomp = ncomp->next;
      if (ncomp && NodeName(ncomp)=="component")
	break;
    }
  }

  return ncomp;
}

///////////////////////////////////////////////////////////////////////////////

VectorComponent::VectorComponent(xmlNodePtr xml) {
    using picsom::GetProperty;

    bool debug = false;
    index = atoi(GetProperty(xml,"index").c_str());
    name = GetProperty(xml,"name");
    min_label = GetProperty(xml,"min-label");
    max_label = GetProperty(xml,"max-label");
    value     = atof(GetProperty(xml,"value").c_str());
    max_value = atof(GetProperty(xml,"max-value").c_str());
    min_value = atof(GetProperty(xml,"min-value").c_str());
    mean      = atof(GetProperty(xml,"mean").c_str());
    stdev     = atof(GetProperty(xml,"stdev").c_str());
    if (debug)
      cout << "Component:" 
	   << " index=" << index
	   << " name="  << name
	   << " value=" << value
	   << " mean="  << mean
	   << " stdev=" << stdev
	   << " min-value=" <<  min_value
	   << " min-label=" <<  min_label
	   << " max-value=" <<  max_value
	   << " max-label=" <<  max_label
	   << endl;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void VectorSourceOf<Type>::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void VectorSourceOf<Type>::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _VECTORSOURCE_C_

