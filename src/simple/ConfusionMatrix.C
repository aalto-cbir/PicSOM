// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: ConfusionMatrix.C,v 1.3 2009/11/20 20:48:15 jorma Exp $

#include <ConfusionMatrix.h>

namespace simple {

///////////////////////////////////////////////////////////////////////////

// ConfusionMatrix::ConfusionMatrix(const ConfusionMatrix&) {
// 
// }

///////////////////////////////////////////////////////////////////////////

// ConfusionMatrix::~ConfusionMatrix() {
// 
// }

///////////////////////////////////////////////////////////////////////////

ConfusionMatrix& ConfusionMatrix::operator=(const ConfusionMatrix& c) {
  FloatMatrix::operator=(c);

  arbitrary = c.arbitrary;
  compress  = c.compress;

  return *this;
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::Add(const ConfusionMatrix& m) {
  for (int si=0; si<m.Rows(); si++)
    for (int di=0; di<m.Columns(); di++)
      Add(m.GetColumn(si)->Label(), m.GetColumn(di)->Label(),
	  m.FloatMatrix::Get(si, di));
  arbitrary += m.arbitrary;
  impulses.Append(m.impulses);
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::Add(const char *src, const char *dst, double n) {
  if (!HasLabel(src))
    AddLabel(src);

  if (!HasLabel(dst))
    AddLabel(dst);

  int si = FindIndex(src), di = FindIndex(dst);

  FloatMatrix::Add(si, di, n);

  impulses.Push(!StringsMatch(src, dst));
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::AddLabel(const char *lab) {
  if (HasLabel(lab)) {
    cerr << "ConfusionMatrix::AddLabel already has label " << lab << endl;
    return;
  }

  AppendColumn();
  AppendRow();
  GetColumn(Columns()-1)->Label(lab);

  if (Columns()>1 && GetColumn(Columns()-2)->Label()==NULL) {
    SwapColumns(Columns()-2, Columns()-1);
    SwapRows(Rows()-2, Rows()-1);
    GetColumn(Columns()-2)->Label(GetColumn(Columns()-1)->Label());
    GetColumn(Columns()-1)->Label(NULL);
  }
}

///////////////////////////////////////////////////////////////////////////

int ConfusionMatrix::FindIndex(const char *lab) const {
  for (int i=0; i<Columns(); i++)
    if (GetColumn(i)->LabelsMatch(lab))
      return i;

  return -1;
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::Display(ostream& os, const char **text) const {
  if (!Columns() || !Rows())
    return;

  if (compress==0.0)
    DisplayWide(os, text);
  else
    DisplayCompress(os, text);
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::DisplayCompress(ostream& os, const char**) const {
  char tmp[10000], labfmt[10000], line[10000];

  int maxval = 1, i;
  for (i=0; i<Columns(); i++) {
    double m = TotalDestination(i);
    if (m>maxval)
      maxval = (int)m;
  }
  unsigned int yllen = 1+(int)floor(log10((double)maxval));

  unsigned int maxlen = 1;
  for (i=0; i<Columns(); i++)
    if (GetColumn(i)->Label() && strlen(GetColumn(i)->Label())>maxlen)
      maxlen = (int)strlen(GetColumn(i)->Label());

  sprintf(labfmt, "%%-%ds -> %%%ds  %%%dd/%%%dd", 
	  maxlen, maxlen, yllen, yllen);
  
  double err, cumerr, errat = ErrorRatio();
  FloatVector wsrc = WrongSource();
  for (i=0, cumerr=0; i<Rows(); i++) {
    int src = wsrc.ArgMax();
    if (!wsrc[src])
      break;

    err = 100.0*wsrc[src]/Total();
    strcat(strcpy(tmp, labfmt), " %6.2f %6.2f %6.2f");
    sprintf(line, tmp, GetColumn(src)->Label(), "", (int)wsrc[src],
	    (int)TotalSource(src), 100.0*wsrc[src]/TotalSource(src), 
	    err, cumerr+=err);
    os << line << endl;

    wsrc[src] = 0;
    if (cumerr >= compress*errat && wsrc.NonZeros()>1) {
      os << "..." << endl;
      break;
    }
  }
  os << endl;

  FloatVector wdst = WrongDestination();
  for (i=0, cumerr=0; i<Rows(); i++) {
    int dst = wdst.ArgMax();
    if (!wdst[dst])
      break;

    err = 100.0*wdst[dst]/Total();
    strcat(strcpy(tmp, labfmt), " %6.2f %6.2f %6.2f");
    sprintf(line, tmp, "", GetColumn(dst)->Label(), (int)wdst[dst],
	    (int)TotalDestination(dst), 100.0*wdst[dst]/TotalDestination(dst), 
	    err, cumerr+=err);
    os << line << endl;

    wdst[dst] = 0;
    if (cumerr >= compress*errat && wdst.NonZeros()>1) {
      os << "..." << endl;
      break;
    }
  }
  os << endl;

  float *concat = Concatenate();
  FloatVector wrng(Rows()*Columns(), concat);
  delete concat;

  for (i=0; i<Rows(); i++) {
    wrng[i*Rows()+i] = 0;
    if (HasRejection())
      wrng[wrng.Length()-1-i] = 0;
  }
  for (cumerr=0;;) {
    int j = wrng.ArgMax(), src = j%Rows(), dst = j/Rows();
    if (!wrng[j])
      break;
    
    err = 100.0*wrng[j]/Total();
    strcat(strcpy(tmp, labfmt), " %6.2f %6.2f %6.2f");
    sprintf(line, tmp, GetColumn(src)->Label(), GetColumn(dst)->Label(), 
	    (int)wrng[j], (int)TotalSource(src), 
	    100.0*wrng[j]/TotalSource(src), err, cumerr+=err);
    os << line << endl;

    wrng[j] = 0;
    if (cumerr >= compress*errat && wrng.NonZeros()>1) {
      os << "..." << endl;
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::DisplayWide(ostream& os, const char **text) const {
  int maxval = 1, i;
  for (i=0; i<Columns(); i++) {
    double m = TotalDestination(i);
    if (m>maxval)
      maxval = (int)m;
  }

  unsigned int maxlen = 1;
  for (i=0; i<Columns(); i++)
    if (GetColumn(i)->Label() && strlen(GetColumn(i)->Label())>maxlen)
      maxlen = (int)strlen(GetColumn(i)->Label());

  unsigned int yllen = 1+(int)floor(log10((double)maxval));
  if (yllen<maxlen)
    yllen = maxlen;

  char ylformat[1024];
  sprintf(ylformat, "%%%ds ", maxlen);

  char nformat[1024];
  sprintf(nformat, "%%%dd ", yllen);

  char xlformat[1024];
  sprintf(xlformat, "%%%ds ", yllen);

  i = 1; // isatty(os.something);
  const char *bold = i?"\033[1m":"", *strike_out = i?"\033[2m":"",
    *normal = i?"\033[0m":"";

  char tmp[1024];

  for (int src=-1; src<Rows()+2; src++) {
    const char *lab = src>=0 && src<Rows() ? GetColumn(src)->Label() : "";

    if (src>=0 && TotalSource(src)==0)
      continue;

    sprintf(tmp, ylformat, lab ? lab : "?");
    os << bold << tmp << normal;

    for (int dst=0; dst<Columns()+2; dst++) {
      if (src>=Rows() && dst>=Columns())
	continue;

      if (src==-1) {
	lab = dst>=0 && dst<Columns() ? GetColumn(dst)->Label() : "";
	sprintf(tmp, xlformat, lab ? lab : "?");
	os << bold << tmp << normal;
	continue;
      }

      float v;
      if (src<Rows() && dst<Columns())
	v = FloatMatrix::Get(src, dst);
      else if (dst<Columns())
	v = src==Rows() ? WrongDestination(dst) : TotalDestination(dst);
      else
	v = dst==Columns() ? WrongSource(src) : TotalSource(src);

      sprintf(tmp, nformat, (int)v);

      if (!v && src!=dst)
	tmp[yllen-1] = ' ';

      if (src<Rows() && dst<Columns() && src!=dst)
	os << strike_out << tmp << normal;
      else
	os << tmp;
    }

    if (src>=0 && src<Rows()) {
      if (TotalSource(src)) {
	sprintf(tmp, " %6.2f", 100.0*WrongSource(src)/TotalSource(src));
	os << tmp;
      } else
	os << "       ";

      if (text) {
	if (text[src])
	  os << " " << text[src];
	else
	  text = NULL;
      }
    }
    os << endl;
  }

}

///////////////////////////////////////////////////////////////////////////

double ConfusionMatrix::Correct() const {
  double sum = 0;
  for (int i=0; i<Columns(); i++)
    if (GetColumn(i)->Label()!=NULL)
      sum += FloatMatrix::Get(i, i);

  return sum;
}

///////////////////////////////////////////////////////////////////////////

double ConfusionMatrix::Erroneous() const {
  double sum = 0;
  for (int i=0; i<Columns(); i++)
    if (GetColumn(i)->Label()!=NULL)
      sum += GetColumn(i)->Sum()-FloatMatrix::Get(i, i);

  return sum;
}

///////////////////////////////////////////////////////////////////////////

double ConfusionMatrix::Rejected() const {
  for (int i=0; i<Columns(); i++)
    if (GetColumn(i)->Label()==NULL)
      return GetColumn(i)->Sum();

  return 0;
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::DisplayAccuracy(int tail, ostream& os,
				      const ConfusionMatrix *cost) const{
  streamsize prec = os.precision(2);
#if !defined(__GNUC__) || __GNUC__<3
  int setf = (int)os.setf(ios::fixed, ios::floatfield);
#endif // __GNUC__

  os << "Total="      << (int)Total()
     << " Correct="   << (int)Correct()
     << " Erroneous=" << (int)Erroneous();

  if (arbitrary)
    os << " Arbitrary=" << arbitrary;

  if (Total())
    os << " (" << ErrorRatio() << " %)";

  if (Rejected())
    os << " Rejected=" << (int)Rejected()
       << " (" << RejectionRatio() << " %)";

  if (cost)
    os << " Cost=" << CostRatio(*cost);

  if (tail) {
    int sum = 0, beg = tail<impulses.Length()? impulses.Length()-tail : 0;
    for (int i=beg; i<impulses.Length(); i++)
      sum += impulses[i];
    int div = impulses.Length()-beg;

    os << " TailError=" << sum << "/" << div
       << " (" << (div?100.0*sum/div:0.0) << " %)";
  }

  os << endl;    

#if !defined(__GNUC__) || __GNUC__<3
  os.setf(setf, ios::floatfield);
#endif // __GNUC__
  os.precision(prec);
}

///////////////////////////////////////////////////////////////////////////

double ConfusionMatrix::WrongDestination(int col) const {
  return TotalDestination(col)-FloatMatrix::Get(col, col);
}

///////////////////////////////////////////////////////////////////////////

double ConfusionMatrix::TotalDestination(int col) const {
  return GetColumn(col)->Sum();
}

///////////////////////////////////////////////////////////////////////////

double ConfusionMatrix::WrongSource(int row) const {
  return TotalSource(row)-FloatMatrix::Get(row, row);
}

///////////////////////////////////////////////////////////////////////////

double ConfusionMatrix::TotalSource(int row) const {
  double sum = 0;

  for (int i=0; i<Columns(); i++)
    sum += FloatMatrix::Get(row, i);

  return sum;
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::Clear() {
  while (Columns())
    RemoveColumn(0);
}

///////////////////////////////////////////////////////////////////////////

void ConfusionMatrix::Sort() const {
  for (int i=1; i<Columns(); i++)
    for (int j=i; j>0; j--)
      if (GetColumn(j)->Label() && GetColumn(j-1)->Label() &&
	  strcmp(GetColumn(j)->Label(), GetColumn(j-1)->Label())<0) {
	SwapColumnsAndRows(j, j-1);

	const char *tmp = CopyString(GetColumn(j)->Label());
	GetColumn(j)->Label(GetColumn(j-1)->Label());
	GetColumn(j-1)->Label(tmp);
	delete tmp;

      } else
	break;
}

///////////////////////////////////////////////////////////////////////////

FloatVector ConfusionMatrix::WrongDestination() const {
  FloatVector res(Labels());
  for (int i=0; i<res.Length(); i++)
    res[i] = WrongDestination(i);

  return res;
}

///////////////////////////////////////////////////////////////////////////

FloatVector ConfusionMatrix::TotalDestination() const {
  FloatVector res(Labels());
  for (int i=0; i<res.Length(); i++)
    res[i] = TotalDestination(i);

  return res;
}

///////////////////////////////////////////////////////////////////////////

FloatVector ConfusionMatrix::WrongSource() const {
  FloatVector res(Labels());
  for (int i=0; i<res.Length(); i++)
    res[i] = WrongSource(i);

  return res;
}

///////////////////////////////////////////////////////////////////////////

FloatVector ConfusionMatrix::TotalSource() const {
  FloatVector res(Labels());
  for (int i=0; i<res.Length(); i++)
    res[i] = TotalSource(i);

  return res;
}

///////////////////////////////////////////////////////////////////////////

// void ConfusionMatrix::() {
// 
// }

///////////////////////////////////////////////////////////////////////////

// void ConfusionMatrix::() {
// 
// }

///////////////////////////////////////////////////////////////////////////

} // namespace simple

