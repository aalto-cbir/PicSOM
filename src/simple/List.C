// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: List.C,v 1.9 2017/05/09 10:16:59 jormal Exp $

#ifndef _LIST_C_
#define _LIST_C_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <string.h>

#include <List.h>
#include <Vector.h>

#include <RandVar.h>
// class RandVar;

// extern VectorOf<int> RandVar::Permutation(int, int);

namespace simple {

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>::~ListOf() {
  FullDelete();
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::FullDelete() {
  Delete();
  delete [] list;
  delete [] own;
  list = NULL;
  own = NULL;
  listsize = 0;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::Delete() {
  while (nitems)
    if (own[--nitems])
      delete list[nitems];
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>& ListOf<Type>::CopyFrom(const ListOf<Type>& l, int o) {
  Delete();
  delete [] list; // these two were missing
  delete [] own;  // up to 26.02.2003 !!!

  nitems   = l.nitems;
  listsize = l.listsize;
  next     = l.next;

  if (nitems>listsize)
    listsize = nitems;
  list = new Type*[listsize];
  own  = new   int[listsize];

  for (int i=0; i<nitems; i++) {
    own[i]  = (o==0 && l.own[i]) || o==1;
    list[i] = own[i] ? new Type(l[i]) : l.Get(i);
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&Simple::DumpRecursive)
    SourceOf<Type>::Dump(type, os);

  if (type&Simple::DumpShort || type&Simple::DumpLong)
    os << Bold(" List ") << (void*)this 
       << " listsize="   << listsize
       << " list="       << (void*)list
       << " own="        << (void*)own;

  if (type&Simple::DumpLong) {
    int i;
    for (i=0; i<Nitems(); i++)
      os << " list[" << i << (own[i]?"o":"") << "]=" << list[i];
    for (; i<listsize; i++)
      os << " list*[" << i << (own[i]?"o":"") << "]";
  }
  os << endl;

#ifndef __INTEL_COMPILER
  // is erroneous when Type==char :
  if ((type&Simple::DumpRecursiveLong)==Simple::DumpRecursiveLong)
    for (int i=0; i<Nitems(); i++)
      list[i]->Dump(type, os);
#endif // __INTEL_COMPILER
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::Append(const ListOf<Type>& l, bool chown, bool o) {
  for (int i=0; i<l.Nitems(); i++)
    Append(l.Get(i), chown ? o : l.own[i]);
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::Append(Type *i, int o) {
  if (listsize<=nitems) {
    Type **oldlist = list;
    list = new Type*[NewListsize()];
    Memcpy(list, oldlist, nitems*sizeof *list);
    delete [] oldlist;

    int *oldown = own;
    own = new int[NewListsize()];
    Memcpy(own, oldown, nitems*sizeof *own);
    delete [] oldown;
  }

  own[nitems]    = o;
  list[nitems++] = i;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::Prepend(Type *i, int o) {
  if (listsize<=nitems) {
    Type **oldlist = list;
    list = new Type*[NewListsize()];
    Memcpy(list+1, oldlist, nitems*sizeof *list);
    delete [] oldlist;

    int *oldown = own;
    own = new int[NewListsize()];
    Memcpy(own+1, oldown, nitems*sizeof *own);
    delete [] oldown;

  } else {
    memmove(list+1, list, nitems*sizeof *list);
    memmove(own+1,  own,  nitems*sizeof *own);
  }

  nitems++;
  *own  = o;
  *list = i;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>& ListOf<Type>::Insert(int i, Type *x, int o) {
  if (i!=nitems && !IndexOK(i)) {
    SimpleAbort();
    return *this;
  }

  Append(x, o);

  int tmpo = own[nitems-1];
  Type *tmpp = list[nitems-1];

  for (int j=nitems-1; j>i; j--) {
    list[j] = list[j-1];
    own[j]  = own[j-1];
  }

  own[i] = tmpo;
  list[i] = tmpp;

  return *this;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
bool ListOf<Type>::Remove(int i) {
  if (!IndexOK(i))
    return false;

  if (own[i])
    delete list[i];

  for (; i<Nitems()-1; i++) {
    list[i] = list[i+1];
    own[i]  = own[i+1];
  }

  nitems--;

  return true;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
int ListOf<Type>::Relinquish(int i) {
  if (!IndexOK(i) || !own[i])
    return FALSE;

  own[i] = FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
int ListOf<Type>::Adopt(int i) {
  if (!IndexOK(i) || own[i])
    return FALSE;

  own[i] = TRUE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
int ListOf<Type>::Find(const Type *u) const {
  for (int i=0; i<Nitems(); i++)
    if (Get(i)==u)
      return i;

  return -1;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
int ListOf<Type>::NumberOfOwns() const {
  int j = 0;

  for (int i=0; i<Nitems(); i++)
    if (own[i])
      j++;

  return j;
}

///////////////////////////////////////////////////////////////////////////

// template <class Type>
// int ListOf<Type>::Read(istream& /*is*/) {
//   return 1;
// }

///////////////////////////////////////////////////////////////////////////

// template <class Type>
// int ListOf<Type>::Write(ostream& os) {
//   for (int i=0; i<Nitems(); i++)
//     Get(i)->Write(os);

//   return 1;
// }

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>& ListOf<Type>::RandomlyReorder(int p) {
  VectorOf<int> perm = RandVar::Permutation(Nitems(), p);
  Type **newlist = new Type*[Nitems()];
  int *newown = new int[Nitems()];

  for (int i=0; i<Nitems(); i++) {
    newlist[i] = list[perm[i]];
    newown[i]  = own[perm[i]];
  }

  Memcpy(list, newlist, Nitems()*sizeof *list);
  Memcpy(own,  newown,  Nitems()*sizeof *own);

  delete newlist;
  delete newown;

  return *this;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>& ListOf<Type>::RandomlySplitTo(ListOf<Type>& dest, int z, int p) {
  int n = (Nitems()+z-1)/z;
  VectorOf<int> perm = RandVar::Permutation(n, p);

  int s = 0, j = 0, i;
  for (i=0; i<Nitems(); i++) {
    if (i%z==0)
      s = perm[j++]&1;

    if (s) {
      dest.Append(list[i], own[i]);
      list[i] = NULL;
      own[i] = FALSE;
    }
  }

//   for (i=0; i<Nitems(); i++)
//     if (!list[i])
//       Remove(i--);

  for (i=Nitems()-1; i>=0; i--)
    if (!list[i])
      Remove(i);

  return *this;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::LeaveSomeOut(int s, int n,
				ListOf<Type>& in, ListOf<Type>& out) const {
  in.Delete();
  out.Delete();
  for (int i=0; i<Nitems(); i++)
    (i>=s && i<s+n ? out : in).Append(Get(i), FALSE);
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type> ListOf<Type>::SharedCopy() const {
  ListOf<Type> ret;

  for (int i=0; i<Nitems(); i++)
    ret.Append(Get(i), FALSE);

  return ret;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
const ListOf<Type>& ListOf<Type>::SharedCopyTo(ListOf<Type>& dst) const {
  for (int i=0; i<Nitems(); i++)
    dst.Append(Get(i), FALSE);

  return *this;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>& ListOf<Type>::Sort(int (*cmp)(const Type&, const Type&)) {
  for (int i=1; i<Nitems(); i++) {
    for (int j=i; j>0; j--)
      if (cmp(*Get(j-1), *Get(j))>0)
	Swap(j-1, j);
      else
	break;
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>& ListOf<Type>::Arrange(const IntVector& ord) {
  Type **newlist = new Type*[listsize];
  int   *newown  = new int[listsize];

  for (int i=0; i<Nitems(); i++) {
    if (!IndexOK(ord[i]))
      SimpleAbort();

    newlist[i] = list[ord[i]];
    newown[i]  = own[ord[i]];
  }

  delete list;
  delete own;
  list = newlist;
  own = newown;

  return *this;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::SaveOwn(int*& save) const {
  save = new int[Nitems()];
  Memcpy(save, own, Nitems()*sizeof *save);
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::RestoreOwn(int* save) {
  Memcpy(own, save, Nitems()*sizeof *save);
  delete save;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::SetOwn(int o) {
  for (int i=0; i<Nitems(); i++)
    own[i] = o; 
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
void ListOf<Type>::RemoveNotOwns() {
  for (int i=Nitems()-1; i>=0; i--)
    if (!own[i])
      Remove(i);
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
ListOf<Type>& ListOf<Type>::Move(int i, int j) {
  if (IndexOK(i)&&IndexOK(j)&&i!=j) {
    Type *tmp = Get(i);
    int tmpi = own[i];
    
    if (i<j)
      for (int k=i; k<j; k++) {
	list[k] = list[k+1];
	own[k]  = own[k+1];
      }
    else
      for (int k=i; k>j; k--) {
	list[k] = list[k-1];
	own[k]  = own[k-1];
      }
    Set(j, tmp);
    own[j] = tmpi;
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _LIST_C_

