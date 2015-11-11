// -*- C++ -*-  $Id: Histogram.C,v 1.5 2009/11/20 20:48:15 jorma Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <Histogram.h>

namespace simple {

///////////////////////////////////////////////////////////////////////////

Histogram::Histogram(const Histogram& hg) :  VectorOf<FloatPoint>(hg) {
  Initialize(0, NULL, NULL);
  *this = hg;
}

///////////////////////////////////////////////////////////////////////////

Histogram::Histogram(const FloatVector& v, const FloatVector& l) {
  Initialize(0, &v, NULL);
  LimitValues(l);
}

///////////////////////////////////////////////////////////////////////////

Histogram::Histogram(const IntVector& v, const FloatVector& l) {
  Initialize(0, NULL, &v);
  LimitValues(l);
}

///////////////////////////////////////////////////////////////////////////

Histogram::Histogram(const ListOf<FloatVector> *l, int b) {
  Initialize(b, NULL, NULL);

  if (l->Nitems() && l->Get(0)->Length()) {
    flist = l;

    int len = l->Get(0)->Length();
    for (int i=0; i<len; i++)
      transp.Append(new FloatVector());
    for (int j=0; j<l->Nitems(); j++)
      for (int k=0; k<len; k++)
	transp[k].Append(l->Get(j)->Get(k));

    fvector = transp.Get(0); // Use() causes refresh...
    Histogram *hg = this;
    for (int n=1; n<len; n++) {
      hg->next = new Histogram(transp.Get(n), b);
      hg = hg->next;
    }
    Refresh();

  }
  else
    ShowError("Histogram::Histogram(FloatVectorList) empty list.");
}

///////////////////////////////////////////////////////////////////////////

void Histogram::Initialize(int b, const FloatVector *fv, const IntVector *iv) {
  fvector    = fv;
  ivector    = iv;
  cumulative = FALSE;
  normalized = TRUE;
  scaled     = FALSE;
  zero_ends  = FALSE;
  bar_graph  = FALSE;
  lowlimit   = -MAXFLOAT;
  highlimit  =  MAXFLOAT;
  bins       = b;
  no_refresh = FALSE;

  next       = NULL;
  flist      = NULL;

  Refresh();
}

///////////////////////////////////////////////////////////////////////////

Histogram& Histogram::operator=(const Histogram& hg) {
  Initialize(hg.Bins(), hg.fvector, hg.ivector);
  cumulative = hg.cumulative;
  normalized = hg.normalized;
  scaled     = hg.scaled;
  zero_ends  = hg.zero_ends;
  bar_graph  = hg.bar_graph;
  lowlimit   = hg.lowlimit;
  highlimit  = hg.highlimit;
  no_refresh = hg.no_refresh;

  values = hg.values;
  lefts  = hg.lefts;
  rights = hg.rights;
  centers = hg.centers;
  limitvalues = hg.limitvalues;

  if (hg.next)
    ShowError("Histogram::operator= copy of next not implemented.");

  if (hg.flist)
    ShowError("Histogram::operator= copy of flist not implemented.");

  if (hg.hash.Nitems())
    ShowError("Histogram::operator= copy of hash not implemented.");

  if (hg.transp.Nitems())
    ShowError("Histogram::operator= copy of transp not implemented.");

  return Refresh();
}

///////////////////////////////////////////////////////////////////////////

void Histogram::Dump(Simple::DumpMode dt, ostream& os) const {
  VectorOf<PointOf<float> >::Dump(dt, os);

  os << Bold(" Histogram ") << (void*)this
     << " fvector="    	    << (void*)fvector
     << " ivector="    	    << (void*)ivector
     << " flist="    	    << (void*)flist
     << " next="       	    << (void*)next
     << " bins="       	    << bins
     << " no_refresh=" 	    << no_refresh
     << " cumulative=" 	    << cumulative
     << " scaled="     	    << scaled
     << " normalized=" 	    << normalized
     << " zero_ends="  	    << zero_ends
     << " bar_graph="  	    << bar_graph
     << " lowlimit=";
  DisplayOne( lowlimit, os);
  os << " highlimit=";
  DisplayOne(highlimit, os);
  os << " hash.Nitems()="   << hash.Nitems()
     << " transp.Nitems()=" << transp.Nitems()
     << endl;
  
  if (dt & DumpLong) {
    os << " values=";
    values.Dump(DumpLong, os);
  }
  if (dt & DumpLong) {
    os << " lefts=";
    lefts.Dump(DumpLong, os);
  }
  if (dt & DumpLong) {
    os << " centers=";
    centers.Dump(DumpLong, os);
  }
  if (dt & DumpLong) {
    os << " rights=";
    rights.Dump(DumpLong, os);
  }
  if (dt & DumpLong) {
    os << " limitvalues=";
    limitvalues.Dump(DumpLong, os);
  }

  if (dt & DumpRecursive && next)
    next->Dump(dt, os);

  if (hash.Nitems() & DumpRecursive && next)
    for (int p=0; p<hash.Nitems(); p++)
      cout << hash[p].Label() << '\t' << hash[p][0] << endl;
}

///////////////////////////////////////////////////////////////////////////

FloatVector Histogram::BarLimits() const {
  FloatVector lim;

  if (Bins()) {
    float min = Min(), max = Max();
    if (!limitvalues.Length())
      lim.Length(Bins()).SetIndices().Multiply((max-min)/Bins()).Add(min);
    else {
      lim = limitvalues;
      lim.Prepend(min);
    }
    lim.Append(max);
    lim[0] -= 0.00001*(lim[1]-lim[0]);

    if (zero_ends && lim.Length()>1) {
      lim.Prepend(2*lim[0]-lim[1]);
      if (!cumulative)
	lim.Append(2*lim.Peek()-lim.PeekPeek());
    }
  }

  return lim;
}

///////////////////////////////////////////////////////////////////////////

Histogram& Histogram::Refresh() {
  if (no_refresh)
    return *this;

  int n = fvector ? fvector->Length() : ivector ? ivector->Length() : 0;

  if (!n || !Bins())
    return *this;

  if (limitvalues.Length() && limitvalues.Length()!=Bins()-1) {
    ShowError("Histogram::Refresh limitvalues cancelled.");
    limitvalues.Length(0);
  }

  FloatVector barlimits = BarLimits(), limits(barlimits);
  limits.Pull();
  Length(limits.Length());
  values.Length(Length());

  for (int i=0, r; i<n; i++) {
    float v = fvector ? fvector->Get(i) : ivector->Get(i);
    for (r=0; r<limits.Length(); r++)
      if (v<=limits[r])
	break;
    values[r]++;
  }

  if (values.Sum()!=n) {
    ShowError("Histogram::Refresh failed.");
    cerr << "n=" << n << " values.Sum()=" << values.Sum() << endl;
  }

  if (normalized)
    values.Divide(n);

  if (scaled && !cumulative)
    for (int j=0; j<values.Length(); j++)
      values[j] /= (barlimits[j+1]-barlimits[j]);

  if (cumulative)
    for (int ii=1; ii<values.Length(); ii++)
      values.Add(ii, values.Get(ii-1));

  centers.Length(Length());
  lefts.Length(Length());
  rights.Length(Length());

  for (int j=0; j<values.Length(); j++) {
    lefts[j]   = barlimits[j];
    rights[j]  = barlimits[j+1];
    centers[j] = (lefts[j]+rights[j])/2;
    Set(j, FloatPoint(centers[j], values[j]));
  }

  if (bar_graph) {
    VectorOf<FloatPoint> v;
    double oldy = 0;
    for (int p=0; p<barlimits.Length(); p++) {
      double newy = p<values.Length() ? values[p] : 0;
      v.Push(FloatPoint(barlimits[p], oldy));
      if (newy!=oldy)
	v.Push(FloatPoint(barlimits[p], newy));
      oldy = newy;
    }
    this->FloatPointVector::operator=(v);
  }

  if (Next())
    Next()->Refresh();

  if (flist)
    MultiDimensionalRefresh();

  return *this;
}

///////////////////////////////////////////////////////////////////////////

void Histogram::Statistics(const FloatVector& vec, ostream& os,
			   const char *file, int n, double min, double max) {

  os << vec.Mean()     << " "
     << vec.Variance() << " "
     << vec.Skewness() << " "
     << vec.Kurtosis();
  
  // os << " | " << vec.TestNormality();

  if (file) {
    Histogram hist(&vec, n);
    hist.Limits(min, max);
    hist.WriteGnuPlot(file);
  }
}

///////////////////////////////////////////////////////////////////////////

// FloatVector Histogram::Centers() const {
//   float min = Min(), max = Max(), diff = (max-min)/Bins();
//   FloatVector r(Bins());
//   for (int i=0; i<r.Length(); i++)
//     r[i] = min+(i+0.5)*diff;
//   if (zero_ends)
//     r.Prepend(min-0.5*diff).Append(max+0.5*diff);  
//   return r;
// }

///////////////////////////////////////////////////////////////////////////

double Histogram::GetValue(double x) const {
//   int i, b = Bins();
//   for (i=0; i<b; i++)
//     if (vector[i].X()>=x)
//       break;
    
//   if (i==b)
//     return b==0 ? 0 : vector[b-1].Y();

//   if (!i)
//     return vector[0].Y();

//   return fabs(vector[i-1].X()-x)<fabs(vector[i].X()-x) ?
//     vector[i-1].Y() : vector[i-1].Y();

  int i = BinNumber(x);
  return values.IndexOK(i) ? values[i] : 0;
}

///////////////////////////////////////////////////////////////////////////

double Histogram::GetValue(double x, double y) const {
  FloatVector vec(2);
  vec.Set(0, x).Set(1, y);
  return GetValue(vec);
}

///////////////////////////////////////////////////////////////////////////

double Histogram::GetValue(const FloatVector& vec) const {
  IntVector idx(vec.Length());
  for (int i=0; i<vec.Length(); i++) {
    int bin = Level(i)->BinNumber(vec[i]);
    if (!Level(i)->values.IndexOK(bin))
      return 0;

    idx[i] = bin;
  }

  return GetValueByIndices(idx);
}

///////////////////////////////////////////////////////////////////////////

double Histogram::GetValue(const char *txt) const {
  for (int i=0; i<hash.Nitems(); i++)
    if (hash[i].LabelsMatch(txt))
      return hash[i][0];

  return 0;
}

///////////////////////////////////////////////////////////////////////////

double Histogram::GetValueByIndices(const IntVector& iv) const {
  char label[10000] = "";
  for (int i=0; i<iv.Length(); i++)
    sprintf(label+strlen(label), "%d%s", iv[i], i<iv.Length()-1?",":"");

  return GetValue(label);
}

///////////////////////////////////////////////////////////////////////////

Histogram& Histogram::Equalize() {
  RemoveLimitValues();

  int cum = Cumulative(), norm = Normalized(), bn = Bins(), sca = Scaled();
  Scaled(FALSE).Cumulative(TRUE).Normalized(TRUE).Bins(10*Bins());

  // Dump(DumpRecursiveLong);

  double frac = 1.0/bn;

  FloatVector lim;
  for (int i=1, j=0; i<bn; i++) {
    while (values[j]<i*frac)
      j++;
    lim.Append(rights[j]);
  }
  LimitValues(lim);

  // Dump(DumpRecursiveLong);

  if (Next())
    Next()->Equalize();

  Bins(bn).Cumulative(cum).Normalized(norm).Scaled(sca);

  return *this;
}

///////////////////////////////////////////////////////////////////////////

Histogram Histogram::Partial(double loval, double hival) const {
  Histogram ret(*this);
  int cum = ret.Cumulative(), norm = ret.Normalized();
  ret.Cumulative(TRUE).Normalized(TRUE);

  FloatVector l(ret.Lefts()), r(ret.Rights()), v(ret.Values());

  ret.Cumulative(cum).Normalized(norm);
  ret.NoRefresh(TRUE);

  int lidx, hidx;
  for (lidx=0; lidx<v.Length(); lidx++)
    if (v[lidx]>loval)
      break;
  if (lidx)
    lidx--;

  for (hidx=v.Length()-1; hidx>=0; hidx--)
    if (v[hidx]<hival)
      break;
  if (hidx<v.Length()-1)
    hidx++;

  ret.Bins(hidx-lidx+1);
  for (int i=v.Length()-1; i>hidx; i--) {
    ret.values.Pop();
    ret.lefts.Pop();
    ret.rights.Pop();
    ret.centers.Pop();
    ret.limitvalues.Pop();
  }

  for (int j=0; j<lidx; j++) {
    ret.values.Pull();
    ret.lefts.Pull();
    ret.rights.Pull();
    ret.centers.Pull();
    ret.limitvalues.Pull();
  }

  if (ret.values.Length()!=ret.Bins())
    ShowError("Histogram::Partial failed.");

  return ret;
}

///////////////////////////////////////////////////////////////////////////

int Histogram::GnuPlotGrid(ostream& os) const {
  if (VectorLength()!=2) {
    ShowError("Histogram::GnuPlotGrid vectorlength should be 2.");
    return FALSE;
  }
  float xl = LimitMin(), xh = LimitMax();
  float yl = Next()->LimitMin(), yh = Next()->LimitMax();

  for (int x=0; x<lefts.Length()+1; x++) {
    float xv =  x<lefts.Length() ? lefts[x] : rights[x-1];
    os << xv << " " << yl << endl << xv << " " << yh << endl << endl;
  }
  for (int y=0; y<Next()->lefts.Length()+1; y++) {
    float yv =  y<Next()->lefts.Length()?Next()->lefts[y]:Next()->rights[y-1];
    os << xl << " " << yv << endl << xh << " " << yv << endl << endl;
  }

  for (int t=0; t<hash.Nitems(); t++) {
    int xi, yi;
    if (sscanf(hash[t].Label(), "%d,%d", &xi, &yi)!=2)
      ShowError("Histogram::GnuPlotGrid hash interpretation failed 1.");
    if (!centers.IndexOK(xi) || !Next()->centers.IndexOK(yi))
      ShowError("Histogram::GnuPlotGrid hash interpretation failed 2.");
    float xc = centers[xi], yc = Next()->centers[yi];
    os << "set label \"" << hash[t][0] << "\" at "
       << xc << "," << yc << " center" << endl;
  }

  return os.good();
}

///////////////////////////////////////////////////////////////////////////

Histogram& Histogram::MultiDimensionalRefresh() {
  hash.Delete();

  if (!flist || !flist->Nitems() ||
      (flist->Nitems()&&!flist->Get(0)->Length())) {
    ShowError("Histogram::MultiDimensionalRefresh failed in the beginning.");
    return *this;
  }

  int len = flist->Get(0)->Length(), p;
  
  for (int j=0; j<flist->Nitems(); j++) {
    char label[10000] = "";
    for (int k=0; k<len; k++) {
      float v = flist->Get(j)->Get(k);
      Histogram *hg = Level(k);
      int n = hg->BinNumber(v);
      if (!hg->values.IndexOK(n))
	ShowError("Histogram::MultiDimensionalRefresh index not OK.");

      sprintf(label+strlen(label), "%d", n);
      if (k<len-1)
	strcat(label, ",");      
    }
    for (p=0; p<hash.Nitems(); p++)
      if (hash[p].LabelsMatch(label))
	break;
    if (p==hash.Nitems())
      hash.Append(new FloatVector(1, NULL, label));
    hash[p][0]++;
    if (p>0 && hash[p-1][0]<hash[p][0])
      hash.Swap(p-1, p);
  }

  int sum = 0;
  for (int r=0; r<hash.Nitems(); r++)
    sum += (int)hash[r][0];

  if (!sum || sum!=flist->Nitems())
    ShowError("Histogram::MultiDimensionalRefresh failed.");

  for (int t=0; t<hash.Nitems(); t++) {
    if (normalized)
      hash[t][0] /= sum;

    if (scaled)
      hash[t][0] /= Area(hash[t].Label());
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////

const Histogram *Histogram::Level(int i) const {
  // cout << "Histogram::Level(" << i << ") const" << endl;
  
  if (!i)
    return this;

  else if (!Next() || i<0) {
    ShowError("Histogram::Level failed.");
    SimpleAbort();
    return this;

  } else
    return Next()->Level(i-1);
}

///////////////////////////////////////////////////////////////////////////

Histogram *Histogram::Level(int i) {
  // cout << "Histogram::Level(" << i << ")" << endl;
  
  if (!i)
    return this;

  else if (!Next() || i<0) {
    ShowError("Histogram::Level failed.");
    SimpleAbort();
    return this;

  } else
    return Next()->Level(i-1);
}

///////////////////////////////////////////////////////////////////////////

int Histogram::BinNumber(double x) const {
  if (!lefts.Length() || x<lefts[0])
    return -MAXINT;

  for (int i=0; i<rights.Length(); i++)
    if (rights[i]>=x)
      return i;

  // ShowError("Histogram::BinNumber failed.");

  return MAXINT;
}

///////////////////////////////////////////////////////////////////////////

double Histogram::Area(const IntVector &iv) const {
  if (!iv.Length() || iv.Length()!=VectorLength()) {
    ShowError("Histogram::Area failed 1.");
    return 0;
  }

  double vol = 1;
  for (int i=0; i<iv.Length(); i++) {
    const Histogram *hg = Level(i);
    if (!hg->rights.IndexOK(iv[i])) {
      ShowError("Histogram::Area failed 2.");
      return 0;
    }
    vol *= hg->rights[iv[i]]-hg->lefts[iv[i]];
  }
  return vol;
}

///////////////////////////////////////////////////////////////////////////

double Histogram::Area(const char *str) const {
  IntVector ivec;
  while (str) {
    ivec.Append(atoi(str));
    str = strchr(str, ',') ? strchr(str, ',')+1 : NULL;
  }

  return Area(ivec);
}

///////////////////////////////////////////////////////////////////////////

// int Histogram::() {
// }

///////////////////////////////////////////////////////////////////////////

// int Histogram::() {
// }

///////////////////////////////////////////////////////////////////////////

// int Histogram::() {
// }

///////////////////////////////////////////////////////////////////////////

// int Histogram::() {
// }

///////////////////////////////////////////////////////////////////////////

// int Histogram::() {
// }

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

} // namespace simple

