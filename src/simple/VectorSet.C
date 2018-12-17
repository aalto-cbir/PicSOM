// 
// Copyright 1994-2017 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2017 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: VectorSet.C,v 1.18 2018/03/16 10:27:41 jormal Exp $

#ifndef _VECTORSET_C_
#define _VECTORSET_C_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <VectorSet.h>
#include <Matrix.h>
#include <RandVar.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

namespace simple {

#ifdef sgi
#pragma can_instantiate void VectorSetOf<StatQuad>::Initialize()
#endif // sgi

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::Initialize() {
  matchmethod   = euclidean;
  //  use_self_dots = FALSE;
  dist_comp_ival = 0;
  scalar_quantized = NULL;
  metric = NULL;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type>& VectorSetOf<Type>::CopyFrom(const VectorSetOf<Type>& l,
					       int o) {
  ListOf<VectorOf<Type> >::CopyFrom(l, o);
  
  Binary(l.Binary());
  Zipped(l.Zipped());
  FileName(l.FileName());

  vectorlength  = l.vectorlength; // was: VectorLength(l.VectorLength());
  //  use_self_dots = l.use_self_dots;
  dist_comp_ival = l.dist_comp_ival;

  metric = l.metric;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::SharedCopy() const {
  VectorSetOf<Type> ret;
  
  return ret.SharedCopyFrom(*this);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::Dump(Simple::DumpMode type, ostream& os) const {
  if (int(type)&int(Simple::DumpRecursive)) {
    ListOf< VectorOf<Type> >::Dump(type, os);
    VectorSourceOf<Type>::Dump(Simple::DumpMode(type&~Simple::DumpRecursive),
			       os);
  }

  if (int(type)&int(Simple::DumpShort) || int(type)&int(Simple::DumpLong)) {
    os << Simple::Bold("  VectorSet ") << (void*)this
       << " matchmethod="      << matchmethod
      // << " use_self_dots="  << use_self_dots
       << " dist_comp_ival="   << dist_comp_ival
       << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const VectorOf<Type>& VectorSetOf<Type>::NearestTo(const VectorOf<Type>& v)
  const {
  IntVector nn(1);
  FloatVector ddd;
  NearestNeighbors(v, nn, ddd);

  return *Get(nn[0]);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::NearestNeighbors(const VectorOf<Type>& v,
					IntVector& res,
					FloatVector& d,
					const typename VectorOf<Type>::DistanceType
					*disttype) const {
  int l = res.Length();
  if (!l)
    return TRUE;

  d.Length(l).Set(MAXFLOAT);
  res.One();
  res.Multiply(-1);

  int dci = DistanceComparisonInterval();
  int k = 0;
  for (int i=0; i<Nitems(); i++) {
    VectorOf<Type> *vec = Get(i);
    if (vec==&v)
      continue;

    double testdist = d[k], dist = MAXFLOAT;

    //dist = v.DistanceSquared(*vec, UseIfNearer() ? testdist : MAXFLOAT);
    dist = v.DistanceSquaredXX(*vec, disttype?disttype:metric,
			       dci?testdist:MAXFLOAT, dci);

    if (dist>=testdist)
      continue;

    int j = 0, p = 0;
    
    if (k!=0 && dist>((float*)d)[0]) {
      j = k;
      while (p<j-1) {
	int n = (p+j)/2;
	testdist = ((float*)d)[n];
	if (dist==testdist) {
	  j = n;
	  break;
	}
	if (dist<testdist)
	  j = n;
	else
	  p = n;
      }
    }
    
    d.Insert(j, dist, k);
    res.Insert(j, i, k);

    if (k<l-1)
      k++;
  }

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const char *VectorSetOf<Type>::NearestNeighborsClass(const VectorOf<Type>& v,
						     int k, int rej,
						     float *dist, int *arb) {
  IntVector idx(k+1);
  FloatVector ddd;
  NearestNeighbors(v, idx, ddd);

  if (arb)
    *arb = (ddd.Length()>1 && ddd.Peek()==ddd.PeekPeek());

//   static bool first = true;
//   if (dist && first) {
//     ShowError("VectorSet::NearestNeighborsClass() dist not supported.");
//     first = false;
//     Abort();
//   }

  if (dist)
    *dist = ddd[0];

  const char *ret = NearestNeighborsClassIdx(idx, k, rej);

  // Statistics(v, idx, k);  // commented out 28.9.2001

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const char *VectorSetOf<Type>::NearestNeighborsClassIdx(const IntVector& idx,
							int k, int rej) {
  if (!k)
    k = idx.Length();

  int *count = new int[k], i, j = -1, max = 0;

  for (int kk=k; kk>0; kk--) {
    memset(count, 0, kk*sizeof(int));

    for (i=0; i<kk && i<idx.Length(); i++) {
      if (idx[i]==-1)
	continue;
      
      if (!Get(idx[i])->Label()) {
	cerr << "No label in VectorSetOf::NearestNeighborsClass" << endl;
	Get(idx[i])->Dump(Simple::DumpLong);
	continue;
      }

      for (j=0; j<i; j++)
	if (idx[j]==-1)
	  continue;
	else if (!strcmp(Get(idx[i])->Label(), Get(idx[j])->Label()))
	  break;

      count[j]++;
    }
    
    for (max = 0, i = 0, j = -1; i<kk; i++) {
      if (count[i]>max)
	max = count[j=i];
      else if (count[i]==max)
	j = -1;
    }
    if (max==0)
      break;

    if (j!=-1)
      break;
  }
  delete count;
  
  return j==-1 || (rej && max<rej) ? NULL : Get(idx[j])->Label();
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// const char *VectorSetOf<Type>::NearestNeighborsClass(int i, int k) {
//   return NearestNeighborsClass(*Get(i), k+1, i);
// }

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void VectorSetOf<Type>::DoNearestNeighborClassification(int k) {
//   SetupConfusionMatrix();
// 
//   First();
//   for (int i=0;; i++) {
//     VectorOf<Type> *vec = Next();
//     if (!vec)
//       break;
// 
//     NearestNeighborsClass(*vec, k+1, i);
//   }
// }

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void VectorSetOf<Type>::DoNearestNeighborClassification(int k,
// 							VectorSetOf<Type>* s) {
//   if (!s)
//     s = this;

//   //SetupConfusionMatrix();

//   s->First();
//   for (VectorOf<Type> *vec; (vec = s->Next());) {
//     // cerr << "VectorSet::DoNearestNeighborClassification" << endl;
//     NearestNeighborsClass(*vec, k);
//   }
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::Read(const char *name, int ptn, bool permissive) {
  if (ptn==FALSE || ptn==TRUE) { // default=-1
    PrintNumber(ptn);
    SetPrintText(ptn ? "Reading " : NULL);
  }

  /*
    istream *fs = OpenReadIOstream(name);
    int r = Read(*fs);
    delete fs;
  */

  gzFile gz = gzopen(name, "r");
  int r = Read(NULL, gz, permissive);
  gzclose(gz);  // added 18.8.2004

  return r;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::Read(istream *isp, gzFile gz, bool permissive) {
  Delete(); // added 8.12.2000.  does it break something somewhere ???

  int next_was = NextIndex();  // is this really needed somewhere ???
  int len = ReadHead(isp, gz, NULL, permissive);

  if (!len && !permissive)
    ShowError("VectorSet::Read vectorlength==0 in <",
	      FileName()?FileName():"", ">");

  ReadAll(isp, gz, len);

  NextIndex(next_was); //next = next_was; //is this really needed somewhere ???

  return Nitems()!=0;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type>& VectorSetOf<Type>::ReadAll(istream* isp, gzFile gz,
					      int len) {
  VectorOf<Type> *vec;

  while ((vec = ReadOne(isp, gz, len))) {
    Append(vec);
    PrintTextAndNumber();
    NextIndexWithPostInc(); // next++;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorSetOf<Type>::DistanceSquared(const VectorSetOf<Type>& set) const {
  if (Nitems() != set.Nitems())
    return MAXFLOAT;

  float sum = 0;

  for (int i=0; i<Nitems(); i++)
    sum += Get(i)->DistanceSquaredOld(set[i]);

  return sum;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> *VectorSetOf<Type>::GetByLabel(const char *l, int n) const {
  for (int i=0; i<Nitems(); i++)
    if (Get(i)->Label() && !strcmp(Get(i)->Label(), l) && !n--)
      return Get(i);

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
map<string,size_t> VectorSetOf<Type>::LabelToIndexMap() const {
  map<string,size_t> m;

  for (int i=0; i<Nitems(); i++)
    m[Get(i)->Label()] = i;

  return m;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::GetAllByLabel(const char *l,
						   IntVector *idx) const {
  VectorSetOf<Type> set;
  if (idx)
    idx->Length(Nitems());

  for (int i=0; i<Nitems(); i++)
    if (Get(i)->Label() && !strcmp(Get(i)->Label(), l)) {
      set.Append(Get(i), FALSE);
      if (idx)
	idx->Set(i, 1);
    }

  return set;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::AppendLabelList(VectorOf<Type> *vec, 
					ListOf< VectorSetOf<Type> >& sets, bool adpt) {
  int j;
  for (j=0; j<sets.Nitems(); j++)
    if (!strcmp(vec->Label(), sets[j].Get(0)->Label()))
      break;
  if (j==sets.Nitems())
    sets.Append(new VectorSetOf<Type>);
  sets[j].Append(vec, adpt);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf< VectorSetOf<Type> > VectorSetOf<Type>::SeparateLabels(bool adpt) {
  ListOf< VectorSetOf<Type> > sets;

  for (int i=0; i<Nitems(); i++)
    if (Get(i)->Label()) {
      AppendLabelList(Get(i), sets, adpt);
      if (adpt)
	Relinquish(i);
    }
  
  return sets;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf< VectorSetOf<Type> >
VectorSetOf<Type>::SeparateByIndex(const IntVector& idx, bool adpt) {
  ListOf< VectorSetOf<Type> > sets;

  if (idx.Length()!=Nitems()) {
    cerr << "VectorSet::SeparateByIndex failed 1" << endl;
    return sets;
  }

  for (int i=0; i<Nitems(); i++) {
    while (sets.Nitems()<=idx[i])
      sets.Append(new VectorSetOf<Type>);

    sets[idx[i]].Append(Get(i), adpt);
    if (adpt)
      Relinquish(i);
  }

  return sets;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::IsIncluded(const VectorOf<Type>& s) const {
  for (int i=0; i<Nitems(); i++)
    if (*Get(i)==s)
      return TRUE;

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::EvenInit(int n, VectorSetOf<Type>& set, int insiders) {
  Delete();
  set.CountNumberOfLabels();

  if (!set.Nitems())
    return FALSE;

  double frac = n/float(set.NumberOfLabels()), cum = 0;
  
  for (int i=0; i<set.NumberOfLabels(); i++) {
    VectorSetOf<Type> subset = set.LabelSubset(set.LabelByNumber(i));

    cum += frac;
    int nn = 0, ins = insiders, err = FALSE;
    for (int j=0; cum-0.5>Nitems(); j++) {
      VectorOf<Type> *vec = subset.Get(j);

      if (ins) {
	if (!vec) {
	  if (!nn)
	    ins = 0;
	  if (!err) {
	    cerr << "VectorSet::EvenInit found only " << nn << " insiders "
		 << "of " << int(frac) << " needed.  Label "
		 << set.LabelByNumber(i) << endl;
	    err = TRUE;
	  }
	  j = -1;
	  continue;
	}
	else if (!set.IsClassInsider(*vec, 3))
	  continue;
      }

      if (!vec) {
	if (j) {
	  j = -1;
	  continue;
	} else {
	  cerr << "VectorSet::EvenInit no exemplars of label "
	       << set.LabelByNumber(i) << " found" << endl;
	  break;
	}
      }

      Append(new VectorOf<Type>(*vec));
      nn++;
    }
  }

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::PropInit(int /*n*/, VectorSetOf<Type>& /*set*/,
				int /*insiders*/) {
  Delete();

  cout << "VectorSet::PropInit is yet not working as it should..." << endl;

//   if (set.Nitems()) {
//     set.CountNumberOfLabels();
//     // set.SetupConfusionMatrix();
// 
//     float frac = 0, cum = 0;
//     for (int i=0; set.LabelIndexOK(i); i++)
//       frac += set.TotalUnits(i);
// 
//     if (frac) {
//       int b = set.NumberOfLabels()<=n;
//       int m = b ? n-set.NumberOfLabels() : n;
// 	
//       frac = m/frac;
//       for (i=0; i<set.NumberOfLabels(); i++) {
// 	cum += set.TotalUnits(i)*frac+b;
// 	int j = 0;
// 	while (cum-0.5>Nitems())
// 	  Append(set.GetByLabel(set.Label(i), j++%set.TotalUnits(i))->Copy());
//       }
//     }
//   }

  return Nitems()>0;
}

// ///////////////////////////////////////////////////////////////////////////
// 
// template <class Type>
// void VectorSetOf<Type>::Adapt1(const VectorOf<Type>& v, int k, double a) {
//   IntVector idx(k);
//   NearestNeighbors(v, idx);
//   Statistics(v, idx, k, "before");
// 
//   for (int i=0; i<k && idx[i]>=0; i++) {
//     VectorOf<Type> *vec = Get(idx[i]);
// 
//     if (v.LabelsMatch(*vec))
//       vec->MoveTowards(v, a);
//     else
//       vec->MoveTowards(v, -a);
//   }
// 
//   NearestNeighbors(v, idx);
//   Statistics(v, idx, k, "after rule1");
// }
// 
// ///////////////////////////////////////////////////////////////////////////
// 
// template <class Type>
// void VectorSetOf<Type>::Adapt2(const VectorOf<Type>& v, int k, double a) {
//   IntVector idx1(k+1), idx2, idx3(k+1);
//   NearestNeighbors(v, idx1);
// 
//   const char *lab1 = NearestNeighborsClass(idx1, k);
//   idx2 = idx1;
//   idx2.Swap(k-1, k);
//   const char *lab2 = NearestNeighborsClass(idx2, k);
// 
//   if (v.LabelsMatch(lab2) && !v.LabelsMatch(lab1)) {
//     Statistics(v, idx1, k, "before");
//     
//     for (int i=k-1; i<k+1 && idx2[i]>=0; i++) {
//       VectorOf<Type> *vec = Get(idx2[i]);
// 
//       if (v.LabelsMatch(*vec))
// 	vec->MoveTowards(v, a);
//       else
// 	vec->MoveTowards(v, -a);
//     }
//     NearestNeighbors(v, idx3);
//     const char *lab3 = NearestNeighborsClass(idx3, k);
//     Statistics(v, idx3, k, "after");
// 
//   } else
//     Statistics(v, idx1, k, "no change");
// }
// 
// ///////////////////////////////////////////////////////////////////////////
// 
// template <class Type>
// void VectorSetOf<Type>::Adapt3(const VectorOf<Type>& v, int k, double a) {
//   IntVector idx(k+1);
//   NearestNeighbors(v, idx);
//   Statistics(v, idx, k, "before");
// 
//   for (int i=0; i<k+1 && idx[i]>=0; i++) {
//     VectorOf<Type> *vec = Get(idx[i]);
// 
//     if (v.LabelsMatch(*vec))
//       vec->MoveTowards(v, a);
//     else
//       vec->MoveTowards(v, -a);
//   }
// 
//   NearestNeighbors(v, idx);
//   Statistics(v, idx, k, "after rule3");
// }
// 
///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::Statistics(const VectorOf<Type>& v,
				   const IntVector& ix, int k,
				   const char *text) {
  const char *lab = NearestNeighborsClassIdx(ix, k);
  statlog << v.Label() << " " << lab;

  for (int i=0; i<ix.Length() && ix[i]>=0; i++) {
    VectorOf<Type> *vec = Get(ix[i]);
    statlog << "   " << vec->Label() << " " << vec->DistanceOld(v); 
  }

  if (!v.LabelsMatch(lab))
    statlog << " *";
  if (text)
    statlog << " " << text;

  statlog << endl;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::CountNumberOfLabels() {
  labelnumbers.Delete();
  
  for (int i=0; i<Nitems(); i++) {
    int j = labelnumbers.Nitems();

    if (Get(i)->Label() && strlen(Get(i)->Label())) {
      for (j=0; j<labelnumbers.Nitems(); j++)
	if (!strcmp(Get(i)->Label(), labelnumbers.Get(j)->Label())) {
	  labelnumbers[j].Add(0, 1);
	  break;
	}
    }
      
    if (j==labelnumbers.Nitems() && Get(i)->Label()) {
      int one = 1;
      labelnumbers.Append(new IntVector(1, &one, Get(i)->Label()));
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::LabelSubset(const char *label) const {
  VectorSetOf<Type> tmp;

  for (int i=0; i<Nitems(); i++)
    if (Get(i)->LabelsMatch(label))
      tmp.Append(Get(i), FALSE);

  return tmp;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::InsiderSubset(int k,
						   VectorSetOf<Type> *set) {
  if (!set)
    set = this;

  VectorSetOf<Type> tmp;
  IntVector idx(k);

  for (int i=0; i<Nitems(); i++) {
    VectorOf<Type> *vec = Get(i);
    FloatVector dummy;
    set->NearestNeighbors(*vec, idx, dummy);

    if (vec->LabelsMatch(set->NearestNeighborsClassIdx(idx, k, k)))
      tmp.Append(vec, FALSE);
  }

  return tmp;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::IsClassInsider(const VectorOf<Type>& vec, int k) {
  IntVector idx(k);
  FloatVector ddd;
  NearestNeighbors(vec, idx, ddd);

  return vec.LabelsMatch(NearestNeighborsClassIdx(idx, k, k));
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::ResetMinMaxNumber() {
  InitializeMinMaxNumber();

  for (int i=0; i<Nitems(); i++)
    CheckMinMaxNumber(Get(i)->Number());
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::DumpNumbers(ostream& os) const {
  for (int i=0; i<Nitems(); i++)
    os << Get(i)->Number() << " ";
  os << endl;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::SetNumbers() const {
  for (int i=0; i<Nitems(); i++)
    Get(i)->Number(i);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorSetOf<Type>::Minimum() const {
  Type r = VectorOf<Type>::LargestPositive();
  for (int i=0; i<Nitems(); i++) {
    Type n = Get(i)->Minimum();
    if (n<r)
      r = n;
  }

  return r;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
Type VectorSetOf<Type>::Maximum() const {
  Type r = VectorOf<Type>::LargestNegative();
  for (int i=0; i<Nitems(); i++) {
    Type n = Get(i)->Maximum();
    if (n>r)
      r = n;
  }

  return r;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorSetOf<Type>::MinimumDistance(const VectorOf<Type>& vec,
					  int own, int oth, int *idx) const {
  double min = MAXFLOAT;
  
  int dci = DistanceComparisonInterval();
  for (int i=0; i<Nitems(); i++) {
    const VectorOf<Type>& v = *Get(i);
    if (!own && vec.LabelsMatch(v))
      continue;

    if (!oth && !vec.LabelsMatch(v))
      continue;

    double d = vec.DistanceSquaredOld(v, dci ? min : MAXFLOAT);
    if (d<min) {
      min = d;
      if (idx)
	*idx = i;
    }
  }

  return min!=MAXFLOAT ? sqrt(min) : MAXFLOAT;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorSetOf<Type>::MaximumDistance(const VectorOf<Type>& vec,
					  int own, int oth, int *idx) const {
  double max = 0;
  for (int i=0; i<Nitems(); i++) {
    if (!own && vec.LabelsMatch(*Get(i)))
      continue;

    if (!oth && !vec.LabelsMatch(*Get(i)))
      continue;

    double d = vec.DistanceSquaredOld(*Get(i));
    if (d>max) {
      max = d;
      if (idx)
	*idx = i;
    }
  }

  return sqrt(max);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSourceOf<Type> *VectorSetOf<Type>::MakeSharedCopy() const {
  VectorSetOf<Type> *ret = new VectorSetOf<Type>;

  for (int i=0; i<Nitems(); i++)
    ret->Append(Get(i), FALSE);

  ret->CopyCrossValidationInfo(*this);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::ClassNeighborHistograms(const char *fn) {
  VectorSetOf<int> hist;

  int dk = 10;
  IntVector nn(dk);

  First();
  const VectorOf<Type> *vec;
  for (;(vec = Next());)
    for (int ok = FALSE; !ok;) {
      FloatVector dummy;
      NearestNeighbors(*vec, nn, dummy);
      for (int i=0; i<nn.Length(); i++)
	if (nn[i]<0)
	  break;

	else if (!Get(nn[i])->LabelsMatch(*vec)) {
	  int idx = hist.Find(hist.GetByLabel(vec->Label()));
	  if (idx<0) {
	    hist.Append(new IntVector(hist.VectorLength()));
	    idx = hist.Nitems()-1;
	    hist[idx].Label(vec->Label());
	  }

	  if (hist[idx].Length()<i+1)
	    hist.Lengthen(i+1);

	  hist[idx][i]++;
	  
	  ok = TRUE;
	  break;
	}

      if (!ok)
	nn.Lengthen(nn.Length()+dk);
    }	  

  hist.Write(fn);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSetOf<Type>::Component(int j) const {
  VectorOf<Type> ret(Nitems());

  for (int i=0; i<ret.Length(); i++)
    ret[i] = (*this)[i][j];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSetOf<Type>::Concat(int start, int len) const {
  VectorOf<Type> ret;

  for (int i=start; i<Nitems() && len>0; i++, len-=VectorLength())
    ret.Append(len>=VectorLength() ?
	       *Get(i) : VectorOf<Type>(len, *Get(i), NULL));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::Split(const VectorOf<Type>& v) {
  for (int i=0; i<v.Length(); i+=VectorLength()) {
    int l = v.Length()-i>=VectorLength() ? VectorLength() : v.Length()-i;
    VectorOf<Type> *p = new VectorOf<Type>(l, (Type*)v+i, v.Label());
    p->Lengthen(VectorLength());
    Append(p);
  }
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorSetOf<Type>::LindeBuzoGray(VectorSourceOf<Type>& set, float *dd) {
  if (dd)
    *dd = 0;

  VectorSetOf<Type> v, w;
  for (int i=0; i<Nitems(); i++) {
    v.Append(new VectorOf<Type>(VectorLength()));
    w.Append(new VectorOf<Type>(VectorLength()));
  }

  IntVector n(Nitems());

  set.First();
  const VectorOf<Type> *vec;
  int k = 0;
  while ((vec = set.Next())) {
    IntVector nn(1);
    FloatVector ddd;
    NearestNeighbors(*vec, nn, ddd);
    v[nn[0]] += *vec;
    n[nn[0]]++;
    w[k++%Nitems()] += *vec;
    if (dd)
      *dd += ddd[0];
  }
  if (dd && k)
    *dd /= k;

  // VectorOf<Type> mean = v.Sum()/n.Sum();

  float d = 0;
  for (int j=0; j<Nitems(); j++) {
    if (n[j])
      v[j] /= n[j];
    else
      v[j] = w[j]/(k/Nitems()+(j<k%Nitems()));

    d += v[j].DistanceSquaredOld((*this)[j]);
    (*this)[j] = v[j];
  }
  
  return sqrt(d);
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorSetOf<Type>::MeanSquaredDistance(const VectorSetOf<Type>& set)
  const {
  double d = 0;
  
  for (int i=0; i<set.Nitems(); i++)
    d += set.Get(i)->DistanceSquaredOld(NearestTo(*set.Get(i)));

  return d/set.Nitems();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
double VectorSetOf<Type>::MeanSquaredDistance(const VectorOf<Type>& vec)
  const {
  double d = 0;
  
  for (int i=0; i<Nitems(); i++)
    d += Get(i)->DistanceSquaredXX(vec);
  // was         DistanceSquaredOld until 11.06.2002

  return d/Nitems();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
IntVector VectorSetOf<Type>::NearestIndices(VectorSourceOf<Type>& set) const {
  IntVector ret;

  ShowError("VectorSet::NearestIndices() may not work properly...");
  // It should not be assumed that Number()==index !!!

  set.First();
  for (const VectorOf<Type> *vec; (vec = set.Next()); )
    ret.Push(NearestTo(*vec).Number());

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
FloatVector VectorSetOf<Type>::NormSquared(const VectorOf<Type> *m) const {
  FloatVector res(Nitems());

  for (int i=0; i<Nitems(); i++)
    if (!m)
      res[i] = Get(i)->NormSquared();
    else {
      VectorOf<Type> vec = *Get(i)-*m;
      res[i] = vec.NormSquared();
    }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
FloatVector VectorSetOf<Type>::Norm(const VectorOf<Type> *m) const {
  FloatVector res(Nitems());

  for (int i=0; i<Nitems(); i++)
    if (!m)
      res[i] = Get(i)->Norm();
    else {
      VectorOf<Type> vec = *Get(i)-*m;
      res[i] = vec.Norm();
    }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSetOf<Type>::LinearCombination(const VectorOf<Type>& vec,
						    VectorOf<Type> *c) const {
  if (!Nitems()) {
    if (c)
      c->Length(0);
    return VectorOf<Type>();
  }

  MatrixOf<Type> base;
  for (int i=1; i<Nitems(); i++)
    base.AppendColumn(new VectorOf<Type>((*this)[i]-(*this)[0]));
  base.Orthonormalize();

  for (int j=0; j<base.Nitems(); j++)
    if (base[j].NormSquared()==0)
      base.Remove(j--);

  if (!base.Nitems()) {
    if (c)
      c->Length(1)[0] = 1;
    return (*this)[0];
  }

  VectorOf<Type> v = vec-(*this)[0];
  VectorOf<Type> m = base.PreTransMultiply(v);
  VectorOf<Type> r = base.PreMultiply(m)+(*this)[0];

  if (c) {
    VectorOf<Type> ret(Nitems());
    MatrixOf<Type> mat;
    for (int k=0, l; k<Nitems(); k++) {
      for (l=0; l<mat.Columns(); l++)
	if ((*this)[k].ContentEqual(*mat.GetColumn(l)))
	  break;
      
      if (l==mat.Columns())
	mat.AppendColumn(Get(k), FALSE);
      else
	ret[k] = -1;
    }
    VectorOf<Type> cc = mat.Inverse().PreMultiply(r);
    
    for (int s=0, t=0; s<Nitems(); s++)
      if (int(ret[s])==-1)
	ret[s] = 0;
      else
	ret[s] = cc[t++];

    *c = ret;
  }

  return r;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type>
VectorSetOf<Type>::LinearCombinationByIteration(const VectorOf<Type>& vec,
						VectorOf<Type> *c) const {
  VectorOf<Type> coeff(Nitems()), r;
  if (Nitems()) {
    r = (*this)[0];
    coeff[0] = 1;
    ListOf<VectorOf<Type> > hist;

    for (int t=1;; t++) {
      int i = t%Nitems();
      if (t>Nitems())
	if ((hist[i]-r).Norm()/(hist[i]+r).Norm()<0.001)
	  break;
	else
	  hist[i] = r;
      else
	hist.AppendCopy(r);

      // r.Dump(DumpLong); coeff.Dump(DumpLong);

      VectorOf<Type> d = (*this)[i]-r;
      double b = d.DotProduct(vec-r)/d.NormSquared();
      r = r*(1-b)+(*this)[i]*b;
      coeff *= (1-b);
      coeff[i] += (Type)b;
    }
  }

  if (c)
    *c = coeff;

  return r;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::SubsetByIndex(const IntVector& idx)
  const {
  VectorSetOf<Type> ret;

  for (int i=0; i<idx.Length(); i++)
    ret.Append(Get(idx[i]), FALSE);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::SubsetByIndex(const std::vector<size_t>&
						   idx) const {
  VectorSetOf<Type> ret;

  for (size_t i=0; i<idx.size(); i++)
    ret.Append(Get((int)idx[i]), FALSE);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::SubsetByValueEquals(const IntVector& v,
							 int val)
  const {
  VectorSetOf<Type> ret;
  if (Nitems()!=v.Length()) {
    char tmp[100];
    sprintf(tmp, "Nitems()=%d v.Length()=%d", Nitems(), v.Length());
    ShowError("VectorSet::SubsetByValueEquals() mismatch: ", tmp);
  }

  for (int i=0; i<v.Length() && i<Nitems(); i++)
    if (v[i]==val)
      ret.Append(Get(i), FALSE);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
const MatrixOf<Type> *VectorSetOf<Type>::FindMetrics(const char * /*label*/) 
const {
  ShowError("VectorSetOf<Type>::FindMetrics is deimplemented");

//   for (int i=0; i<new_metrics.Nitems(); i++)
//     if (new_metrics[i].Label()==NULL || new_metrics[i].LabelsMatch(label))
//       return new_metrics.Get(i);

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::Nearests(const VectorOf<Type>& vec,
				 VectorOf<Type>*& o, VectorOf<Type>*& r)const {
  double od = MAXFLOAT, rd = MAXFLOAT;
  o = r = NULL;

  for (int i=0; i<Nitems(); i++) {
    if (Get(i)==&vec)
      continue;

    double d = (*Get(i)-vec).NormSquared();

    if (vec.LabelsMatch(*Get(i))) {
      if (d<od) {
	od = d;
	o = Get(i);
      }
    } else if (d<rd) {
      rd = d;
      r = Get(i);
    }
  } 
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
void VectorSetOf<Type>::CalculateSelfDots() {
  // UseSelfDots(TRUE);
  for (int i=0; i<Nitems(); i++)
    Get(i)->CalculateSelfDot();
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// double VectorSetOf<Type>::DistanceSquared(const VectorOf<Type>& v) const {
//   if (v.HasSelfDot() && UseSelfDots()) {
//     register double d = -1;
// #ifdef HAS_LINPACK
//     if (UseLinpack());
// #endif HAS_LINPACK
    
//     return d!=-1 ? d : ;

//   }
// }

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::ClassBorderIndex(const VectorOf<Type>& vec) const {
  FloatVector down;
  double dother = MAXFLOAT;

  for (int i=0; i<Nitems(); i++) {
    if (Get(i)==&vec)
      continue;

    double d = Get(i)->DistanceSquaredOld(vec, dother);
    if (d>dother)
      continue;

    if (Get(i)->LabelsMatch(vec))
      down.InsertSortIncreasingly(d);
				    
    else {
      while (down.Length() && down.Peek()>d)
	down.Pop();

      dother = d;
    }
  }

  return down.Length();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
IntVector VectorSetOf<Type>::ClassBorderIndices(const VectorSetOf<Type>& set)
  const {
  IntVector ret;
  for (int i=0; i<set.Nitems(); i++)
    ret.Push(ClassBorderIndex(set[i]));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
FloatVector VectorSetOf<Type>::SimilarityValues(const VectorOf<Type>& v) const{
  FloatVector sim(Nitems());
  for (int i=0; i<Nitems(); i++) {
    double nom = 0, den = 0; 
    for (int j=0; j<VectorLength(); j++) {
      nom += (*this)[i][j]*v[j];
      den += (*this)[i][j];
    }
    sim[i] = den ? nom/den : 0;
  }

  return sim;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSetOf<Type>::MinimumComponents() const {
  VectorOf<Type> ret(VectorLength());
  if (!Nitems())
    return ret.Length(0);

  ret = *Get(0);
  for (int i=1; i<Nitems(); i++)
    for (int j=0; j<VectorLength(); j++)
      if (ret[j]>(*this)[i][j])
	ret[j] = (*this)[i][j];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSetOf<Type>::MaximumComponents() const {
  VectorOf<Type> ret(VectorLength());
  if (!Nitems())
    return ret.Length(0);

  ret = *Get(0);
  for (int i=1; i<Nitems(); i++)
    for (int j=0; j<VectorLength(); j++)
      if (ret[j]<(*this)[i][j])
	ret[j] = (*this)[i][j];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
MatrixOf<float> VectorSetOf<Type>::MutualDistances(int mx) const {
  MatrixOf<float> ret(Nitems(), Nitems());
  
  for (int i=0; i<Nitems(); i++)
    for (int j=i+1; j<Nitems(); j++) {
      double d = (*Get(i)-*Get(j)).Norm(mx);
      ret.Set(i, j, d).Set(j, i, d);
    }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::KMeans(int k, int maxn,
					    IntVector *iv) const {
  bool old_init = false;

  VectorSetOf<Type> ret;
  int i;
  for (i=0; i<k; i++)
    ret.Append(new VectorOf<Type>(VectorLength()));

  RandVar rnd(0);

  IntVector indexold(Nitems());

  for (i=0; i<Nitems(); i++) {
    if (old_init) {
      int ri = rnd.RandomInt(k);
      indexold[i] = ri;
    }
    if (Get(i)->Number()!=i)
      ShowError("VectorSet::KMeans() will not work properly...");
    // It should not be assumed that Number()==index !!!
  }

  for (int r=0;; r++) {
    cout << "KMeans() starting round " << r << endl;

    if (r || old_init) {
      for (i=0; i<k; i++)
	ret[i].Zero();
 
      IntVector n(k);
      for (i=0; i<Nitems(); i++) {
	ret[indexold[i]] += *Get(i);
	n[indexold[i]]++;
      }

      for (i=0; i<k; i++)
	if (n[i])
	  ret[i].Divide(n[i]);

    } else {
      for (i=0; i<k; i++) {
	ret[i] = *Get(rnd.RandomInt(Nitems()));
	ret[i].Number(i);
      }
    }

    IntVector index(Nitems());
    for (i=0; i<Nitems(); i++) {
      const VectorOf<Type>& nn = ret.NearestTo(*Get(i));
      index[i] = nn.Number();
      // cout << index[i] << " ";
    }
    // cout << endl;

    if (index.ContentEqual(indexold) || --maxn<=0) {
      if (iv)
	*iv = index;
      break;
    }

    indexold = index;
  }  

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

template <>
FloatVectorSet FloatVectorSet::OrderComponentValues() const {
  FloatVectorSet ret;
  for (int s=0; s<Nitems(); s++)
    ret.Append(new FloatVector(VectorLength()));

  for (int comp=0; comp<VectorLength(); comp++) {
    FloatVector sort(Nitems());
    for (int i=0; i<Nitems(); i++)
      sort[i] = (*Get(i))[comp];

    sort.QuickSortIncreasingly();

    for (int j=0; j<Nitems(); j++)
      (*ret.Get(j))[comp] = sort[j];
  }

  return ret;
}

template <class Type>
VectorSetOf<Type> VectorSetOf<Type>::OrderComponentValues() const {
  FunctionNotImplemented("VectorSetOf<Type>::OrderComponentValues");
  return VectorSetOf<Type>();
}

///////////////////////////////////////////////////////////////////////////////

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate int VectorSetOf<float>::ScalarQuantize(double,const FloatVector &)
#endif // sgi || __alpha

template <>
int FloatVectorSet::ScalarQuantize(double v, const FloatVector &lim) {
  for (int i=0; i<lim.Length(); i++)
    if (v<lim[i])
      return i;

  return lim.Length();
}

template <class Type>
int VectorSetOf<Type>::ScalarQuantize(double, const FloatVector&) {
  FunctionNotImplemented("VectorSetOf<Type>::ScalarQuantize");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate IntVector VectorSetOf<float>::ScalarQuantize(const FloatVector&) const
#endif // sgi

template <>
IntVector FloatVectorSet::ScalarQuantize(const FloatVector &vec) const {
  IntVector ret(vec.Length(), NULL, vec.Label());

  for (int i=0; i<vec.Length(); i++)
    ret[i] = ScalarQuantize(vec[i], scalar_quantization_values[i]);

  return ret;
}

template <class Type>
IntVector VectorSetOf<Type>::ScalarQuantize(const FloatVector&) const {
  FunctionNotImplemented("VectorSetOf<Type>::ScalarQuantize");
  return IntVector();
}

///////////////////////////////////////////////////////////////////////////////

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate void VectorSetOf<float>::ScalarQuantize()
#endif // sgi || __alpha

template <>
void FloatVectorSet::ScalarQuantize() {
  delete scalar_quantized;
  scalar_quantized = new IntVectorSet();

  for (int i=0; i<Nitems(); i++)
    scalar_quantized->AppendCopy(ScalarQuantize(*Get(i)));
}

template <class Type>
void VectorSetOf<Type>::ScalarQuantize() {
  FunctionNotImplemented("VectorSetOf<Type>::ScalarQuantize");
}

///////////////////////////////////////////////////////////////////////////////

template <>
void FloatVectorSet::MakeScalarQuantBackRefs() {
  scalar_quantization_back_ref.Delete();

  for (int c=0; c<VectorLength(); c++) {
    scalar_quantization_back_ref.Append(new ListOf<IntVector>);
    for (int i=0; i<scalar_quantization_values[c].Length()+1; i++)
      scalar_quantization_back_ref[c].Append(new IntVector);
  }

  for (int i=0; i<Nitems(); i++)
    for (int c=0; c<VectorLength(); c++) {
      scalar_quantization_back_ref[c][ScalarQuantized(i)[c]].Append(i);
//       cout << "i=" << i << " c=" << c << " q=" << ScalarQuantized(i)[c]
// 	   << endl;
    }
}

template <class Type>
void VectorSetOf<Type>::MakeScalarQuantBackRefs() {
  FunctionNotImplemented("VectorSetOf<Type>::MakeScalarQuantBackRefs");
}

///////////////////////////////////////////////////////////////////////////////

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate VectorSetOf<float>::EqualizedScalarQuantizationPick
#endif // sgi

template <>
FloatVector FloatVectorSet::EqualizedScalarQuantizationPick(int n, const
							    FloatVector& sort){
  FloatVector lim(n-1);
  for (int k=0; k<n-1; k++) {
    int l = (int)ceil((float)sort.Length()*(k+1)/n);
    int m = l ? l-1 : 0;
    lim[k] = (sort[l]+sort[m])/2;
  }

  return lim;
}

template <class Type>
FloatVector VectorSetOf<Type>::EqualizedScalarQuantizationPick(int, const
							       FloatVector&) {
  FunctionNotImplemented("VectorSetOf<Type>::EqualizedScalarQuantizationPick");
  return FloatVector();
}

///////////////////////////////////////////////////////////////////////////////

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate FloatVector VectorSetOf<float>::CreateEqualizedScalarQuantization(int,int)
#endif // sgi

template <>
FloatVector FloatVectorSet::CreateEqualizedScalarQuantization(int n, int comp){
  FloatVector sort = Component(comp);
  sort.QuickSortIncreasingly();

  return EqualizedScalarQuantizationPick(n, sort);
}

template <class Type>
FloatVector VectorSetOf<Type>::CreateEqualizedScalarQuantization(int, int) {
  FunctionNotImplemented("VectorSetOf<Type>::CreateEqualizedScalarQuantization");
  return FloatVector();
}

///////////////////////////////////////////////////////////////////////////////

template <>
ListOf<FloatVector> 
FloatVectorSet::CreateEqualizedScalarQuantization(int n,
						  const FloatVectorSet& ord) {
  ListOf<FloatVector> ret;

  for (int i=0; i<VectorLength(); i++) {
    FloatVector sort = ord.Component(i);
    ret.AppendCopy(EqualizedScalarQuantizationPick(n, sort));
  }

  return ret;
}

template <class Type>
ListOf<FloatVector> VectorSetOf<Type>::CreateEqualizedScalarQuantization(
  int, const VectorSetOf<float>&) {
  FunctionNotImplemented("VectorSetOf<Type>::CreateEqualizedScalarQuantization");
  return ListOf<FloatVector>();
}

///////////////////////////////////////////////////////////////////////////////

template <>
ListOf<FloatVector> FloatVectorSet::CreateEqualizedScalarQuantization(int n) {
  ListOf<FloatVector> ret;

  for (int i=0; i<VectorLength(); i++)
    ret.AppendCopy(CreateEqualizedScalarQuantization(n, i));

  return ret;
}

template <class Type>
ListOf<FloatVector>  VectorSetOf<Type>::CreateEqualizedScalarQuantization(
  int) {
  FunctionNotImplemented("VectorSetOf<Type>::CreateEqualizedScalarQuantization");
  return ListOf<FloatVector>();
}

///////////////////////////////////////////////////////////////////////////////

template <>
IntVector FloatVectorSet::ScalarQuantizationPairDistanceHistogram() const {
  IntVector ret;

  for (int i=0; i<Nitems(); i++)
    for (int j=i+1; j<Nitems(); j++) {
      int d = ScalarQuantizationDistance(i, j);

      if (ret.Length()<d+1)
	ret.Lengthen(d+1);

      ret[d]++;
    }

  return ret;
}

template <class Type>
IntVector VectorSetOf<Type>::ScalarQuantizationPairDistanceHistogram() const {
  FunctionNotImplemented("VectorSetOf<Type>::ScalarQuantizationPairDistanceHistogram");
  return IntVector();
}

///////////////////////////////////////////////////////////////////////////////

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate IntVector VectorSetOf<float>::ScalarQuantizationEquals(const IntVector&,int) const
#endif // sgi || __alpha

template <>
IntVector FloatVectorSet::ScalarQuantizationEquals(const IntVector& v,
						   int d) const {
  if (!VectorLength()) {
    ShowError("FloatVectorSet::ScalarQuantizationEquals(): VectorLength()==0");
    return IntVector();
  }

  if (scalar_quantization_back_ref.Nitems()!=VectorLength()) {
    ShowError("FloatVectorSet::ScalarQuantizationEquals(): no back refs");
    return IntVector();
  }

  if (v.Length()!=VectorLength()) {
    ShowError("FloatVectorSet::ScalarQuantizationEquals():"
	      " v.Length()!=VectorLength()");
    v.Dump(Simple::DumpLong);
    return IntVector();
  }

  if (!scalar_quantization_back_ref[0].IndexOK(v[0])) {
    char tmp[100];
    sprintf(tmp, "index [%d][%d] not OK", 0, v[0]);
    ShowError("FloatVectorSet::ScalarQuantizationEquals(): ", tmp);
    return IntVector();
  }

  if (d==0) {
    IntVector set = scalar_quantization_back_ref[0][v[0]];
    for (int i=1; i<VectorLength(); i++)
      if (scalar_quantization_back_ref[i].IndexOK(v[i]))
	set.IntersectOrdered(scalar_quantization_back_ref[i][v[i]]);

      else {
	char tmp[100];
	sprintf(tmp, "index [%d][%d] not OK", i, v[i]);
	ShowError("FloatVectorSet::ScalarQuantizationEquals(): ", tmp);
	return IntVector();
      }

    return set;
  }

  IntVector set;
  for (int i=0; i<Nitems(); i++)
    if ((ScalarQuantized(i)-v).Norm(1)==d)
      set.Push(i);
  return set;
}

template <class Type>
IntVector VectorSetOf<Type>::ScalarQuantizationEquals(const IntVector&,
						      int) const {
  FunctionNotImplemented("VectorSetOf<Type>::ScalarQuantizationEquals");
  return IntVector();
}

///////////////////////////////////////////////////////////////////////////////

template <>
IntVector FloatVectorSet::ScalarQuantizationHitCountHistogram() const {
  IntVector ret;
  IntVector done(Nitems());

  for (int i=0; i<Nitems(); i++)
    if (!done[i]) {
      IntVector idx = ScalarQuantizationEquals(i);
      if (!idx.Length()) {
	ShowError("ScalarQuantizationHitCountHistogram() 0");
	continue;
      }

      if (ret.Length()<idx.Length())
	ret.Lengthen(idx.Length());

      ret[idx.Length()-1] += idx.Length();
		     
      for (int j=0; j<idx.Length(); j++)
	if (done[idx[j]]) {
	  ShowError("ScalarQuantizationHitCountHistogram() 1");
	  cout << "j=" << j << " idx[j]=" << idx[j] << endl;
	}
	else
	  done[idx[j]] = 1;
    }
    
  if (!done.Add(-1).IsZero())
    ShowError("ScalarQuantizationHitCountHistogram() 2");

  return ret;
}

template <class Type>
IntVector VectorSetOf<Type>::ScalarQuantizationHitCountHistogram() const {
  FunctionNotImplemented("VectorSetOf<Type>::ScalarQuantizationHitCountHistogram");
  return IntVector();
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
VectorOf<Type> VectorSetOf<Type>::Mean(const IntVector *sel) const {
  VectorOf<Type> sum(vectorlength);
  int n = sel ? sel->Length() : nitems;
  for (int i=0; i<n; i++)
    sum += *Get(sel ? sel->Get(i) : i);

  return sum/(double)(n?n:1);  
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSetOf<Type>::SetCovInvInMetric() /*const*/ {
  if (!metric ||
      (metric->GetType()!=VectorOf<Type>::DistanceType::mahalanobis &&
       metric->GetType()!=VectorOf<Type>::DistanceType::mahalanobis_bias)) {
    ShowError("VectorSet::SetCovInvInMetric() not mahalanobis metric");
    return false;
  }

  bool unbiased = metric->GetType()==VectorOf<Type>::DistanceType::mahalanobis;
  MatrixOf<Type> cov = AutoCov(NULL, unbiased);
  MatrixOf<Type> inv = cov.Inverse();
  // inv.Dump(DumpLong);
  if (!inv.Rows() || !inv.Columns()) {
    ShowError("VectorSet::SetCovInvInMetric() failed to invert matrix");
    return false;
  }

  metric->SetMatrix(new MatrixOf<Type>(inv));

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSetOf<Type>::Normalize(bool debug, xmlDocPtr xml) {
  xmlNodePtr ncomp = FindOrAddFeatureComponents(xml);

  StatVarVector sv(VectorLength());
  for (int i=0; i<Nitems(); i++)
    for (int j=0; j<VectorLength(); j++)
      sv[j] += (*Get(i))[j];

  for (int j=0; j<VectorLength(); j++) {
    double m = sv[j].Mean(), q = sv[j].StandDev();
    ComponentAddAndMultiply(j, -m, q?1/q:0);

    ncomp = SetFeatureComponentMeanStDev(ncomp, m, q);

    if (debug)
      cout << "Normalize() j=" << j << " n=" << sv[j].N()
	   << " mean=" << m << " stdev=" << q
	   << " min=" << sv[j].Min() << " max=" << sv[j].Max()
	   << " range=" <<  sv[j].Max()-sv[j].Min()
	   << endl;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSetOf<Type>::RemoveZeroVectors() {
  for (int i=Nitems()-1; i>=0; i--)
    if (Get(i)->IsZero())
      Remove(i);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
bool VectorSetOf<Type>::MapNegativeZeroPositive(Type n, Type z, Type p) {
  for (int i=0; i<Nitems(); i++)
    Get(i)->MapNegativeZeroPositive(n, z, p);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class Type>
int VectorSetOf<Type>::PolarizeComponents() {
  VectorOf<Type> min = MinimumComponents();
  VectorOf<Type> max = MaximumComponents();

  // min.Dump(DumpLong); max.Dump(DumpLong);

  int l = VectorLength();

  IntVector polar(l);
  for (int i=0; i<polar.Length(); i++)
    if (double(min[i])>=0 && double(max[i])<=1)
      polar[i] = 1;

  int n = polar.Sum();
  if (!n)
    return 0;
    
  Lengthen(l+n);
  for (int j=0; j<Nitems(); j++) {
    VectorOf<Type> *v = Get(j);
    VectorOf<Type> m(VectorLength());
    for (int i=0, d=0; i<polar.Length(); i++, d++)
      if (!polar[i])
	m[d] = (*v)[i];
      else {
	m[d]   = (Type)cos(double((*v)[i])*M_PI*2);
	m[++d] = (Type)sin(double((*v)[i])*M_PI*2);
      }
    v->CopyValue(m);
  }

  return n;
}

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void VectorSetOf<Type>::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// template <class Type>
// void VectorSetOf<Type>::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _VECTORSET_C_

