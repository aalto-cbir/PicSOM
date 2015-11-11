// -*- C++ -*-  $Id: List.h,v 1.7 2014/07/01 13:40:56 jorma Exp $
// 
// Copyright 1994-2008 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _LIST_H_
#define _LIST_H_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <Source.h>

namespace simple {

template <class T> class VectorOf;

//- List
template <class Type> class ListOf : virtual public SourceOf<Type> {
public:
  using SourceOf<Type>::Nitems;
  using SourceOf<Type>::ConditionalAbort;
  //g++ SourceOf<Type>::DumpMode;
  using SourceOf<Type>::Bold;
  using SourceOf<Type>::nitems;
  using SourceOf<Type>::next;

public:
  ListOf() { Initialize();}

  ListOf(const ListOf& l) : SourceOf<Type>(l)
    /* Don't know about the above call. Added it anyway 7.11.2005...*/ {
    Initialize(); AsSuchCopyFrom(l); }

  virtual ~ListOf();

  ListOf& operator=(const ListOf& l) { return AsSuchCopyFrom(l); }

  ListOf& CopyFrom(const ListOf&, int);
  ListOf& AsSuchCopyFrom(const ListOf& l) { return CopyFrom(l,  0); }
  ListOf& CopiedCopyFrom(const ListOf& l) { return CopyFrom(l,  1); }
  ListOf& SharedCopyFrom(const ListOf& l) { return CopyFrom(l, -1); }

  int IndexOK(int i) const { return i>=0 && i<nitems; }

  Type& operator[](int i) const {
    if (!IndexOK(i)) {
      cerr << "Index " << i << " in List::operator[] out of bounds" << endl;
      Dump(Simple::DumpRecursiveLong, cerr);
      SimpleAbort();
    }
    return *Get(i); }

  virtual void Dump(Simple::DumpMode = Simple::DumpDefault,
		    ostream& = cout) const;

  virtual const char **NamesSimpleClass() const { static const char *n[] = {
    "List", NULL }; return n; }

  virtual void Append(const ListOf&, bool = false, bool = false);

  virtual void Append( Type*, int = TRUE);
  virtual void Prepend(Type*, int = TRUE);

  inline void AppendCopy(const Type& v); // { Append(new Type(v)); }

  void AppendCopy(const ListOf& src) {
    for (int i=0; i<src.Nitems(); i++)
      AppendCopy(src[i]); }

  int TakeAll(ListOf<Type>& src) {
    for (int i=0; i<src.Nitems(); i++)
      if (!src.Relinquish(i))
	return FALSE;
      else
	Append(src.Get(i));
    return TRUE;
  }

  bool Remove(int);
  bool Remove(const Type *u) { return Remove(Find(u)); }
  bool RemoveLast(int i = 1) {
    bool r = true; while (i-->0) r &= Remove(Nitems()-1); return r; }

  int Relinquish(int);
  int Relinquish(const Type *u) { return Relinquish(Find(u)); }

  int Adopt(int);
  int Adopt(const Type *u) { return Adopt(Find(u)); }

  int Find(const Type*) const;
  bool IsContained(const Type *u) const { return Find(u)!=-1; }

  bool IsOwned(int i) const {
    return IndexOK(i) && own[i]; }

  bool IsOwned(const Type *u) const {
    return IsOwned(Find(u)); }

  void RemoveNotOwns();

  int NumberOfOwns() const;

  ListOf& Swap(int i, int j) {
    if (IndexOK(i)&&IndexOK(j)) {
      Type *tmp = Get(i);
      Set(i, Get(j));
      Set(j, tmp);
      int tmpi = own[i];
      own[i] = own[j];
      own[j] = tmpi;
    }
    return *this;
  }

  ListOf& Move(int i, int j);

  void Empty() { nitems = 0; }
  void Delete();
  void FullDelete();

  void Set(int i, Type *v, int o = TRUE) {
    if (IndexOK(i)) { list[i] = v; own[i] = o; }
  }

  void Replace(int i, Type *v, int o = TRUE) {  // delete was added 25.6.98
    if (IndexOK(i)) {
      if (own[i])
	delete list[i];

      list[i] = v; own[i] = o;
    }
  }

  Type *Get(int i) const { return IndexOK(i) ? list[i] :  NULL; }

  Type *FastGet(int i) const { return list[i]; }

  Type *GetEvenNew(int i) {
    while (nitems<=i)
      Append(new Type());
    return Get(i); }

  void Push(Type *i) { Append(i); }
  Type *Pull()       { Type *p = Get(0); Relinquish(0); Remove(0); return p; }
  Type *Pop()        { Type *p = Peek(); if (nitems) nitems--; return p; }
//   Type *Peek()       { return Get(nitems-1); }
//   const Type *Peek() const { return Get(nitems-1); }
  Type *Peek() const { return Get(nitems-1); }

  ListOf& Insert(int, Type*, int = TRUE);

  ListOf& RandomlyReorder(int = MAXINT);
  ListOf& RandomlySplitTo(ListOf&, int, int = MAXINT);
  void LeaveSomeOut(int, int, ListOf&, ListOf&) const;

  ListOf SharedCopy() const;
  const ListOf& SharedCopyTo(ListOf&) const;

//   int Read(istream&);
//   int Read(const char *n)   { ifstream is(n); return Read(is); }
//   int Write(ostream&);
//   int Write(const char *n)  { ofstream os(n); return Write(os); }

  ListOf& Sort(int (*)(const Type&, const Type&));
  ListOf& Arrange(const VectorOf<int>&);

  void SaveOwn(int*&) const;
  void RestoreOwn(int*);
  void SetOwn(int);

protected:
  Type **list;
  int   *own;
  int    listsize;

  void Initialize() {
    nitems = 0; // should other SourceOf<> members be initialized too ?
    listsize = 0;
    list = NULL; own = NULL;
  }

  int NewListsize() {
    while (listsize<nitems+1)
      if (!listsize)
	listsize = 1;
      else listsize *= 2;

    return listsize;
  }
};

template <class Type>
void ListOf<Type>::AppendCopy(const Type& v) { Append(new Type(v)); }

#ifdef EXPLICIT_INCLUDE_CXX
#include <List.C>
#endif // EXPLICIT_INCLUDE_CXX

} // namespace simple
#endif // _LIST_H_

