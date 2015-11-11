// -*- C++ -*-  $Id: Source.h,v 1.3 2009/11/20 20:48:16 jorma Exp $
// 
// Copyright 1994-2008 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _SOURCE_H_
#define _SOURCE_H_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <Simple.h>

namespace simple {

//- Source
template <class Type> class SourceOf : public Simple {
protected:
  SourceOf() {
    next = print_number = nitems = 0;
    filename = print_text = NULL;
  }

  SourceOf(const SourceOf<Type>& s) :
    Simple(s),
    nitems(s.nitems), next(s.next), print_number(s.print_number),
    print_text((char*)CopyString(s.print_text)),
    filename((char*)CopyString(s.filename)) {
    // Don't know about this. Added it anyway 7.11.2005...
  }

  virtual ~SourceOf() { delete filename; delete print_text; }

public:
  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "Source", NULL }; return n; }

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

  const char *FileName() const { return filename; }
  void FileName(const char *name) { CopyString(filename, name); }
  void FileName(const string& s) { FileName(s.c_str()); }

  int Current() const { return next-1; }
  virtual int   Nitems() const { return nitems; }
  virtual void  First() { next = 0; }
  virtual void  Skip(int) { abort(); }
  virtual void  DoneWith(Type*) const {}

  virtual const Type *Next()  {
    PreNext(); PrintTextAndNumber(); return GetNext(); }
  virtual void  PreNext() {}
  virtual Type *GetNext() { abort(); return NULL; }

  virtual void PrintTextAndNumber() const { PrintText(); PrintNumber(); }

  virtual void PrintNumber() const {
    if (print_number) cout << next << "          \r" << flush; }
				
  void PrintNumber(int i) { print_number = i; }
  int GetPrintNumber() const { return print_number; }

  virtual void PrintText() const {
    if (print_text) cout << print_text << flush; }
				
  void PrintText(const char *txt) { CopyString(print_text, txt); }
  int HasPrintText() const { return print_text!=NULL; }
  const char *GetPrintText() const { return print_text; }

  void SetPrintText(const char*);
  void ConditionallySetPrintText(const char*);

  virtual const char *AdditionalPrintText() const { return ""; }

  virtual void Append( Type*, int = TRUE) { abort(); }
  virtual void Prepend(Type*, int = TRUE) { abort(); }

  int NextIndex() const { return next; }
  void NextIndex(int n) { next = n; }
  int NextIndexWithPostInc() { return next++; }
  void AddToNextIndex(int n) { next += n; }

protected:
  int    nitems;
  int    next;
  int    print_number;
  char  *print_text;
  char  *filename;
};

///////////////////////////////////////////////////////////////////////////

// template <class Type>
// inline void SourceOf<Type>::Dump(Simple::DumpMode type, ostream& os) const {
//   if (type&DumpShort || type&DumpLong)
//     os << Bold("Source ")  << (void*)this 
//        << " filename="     << ShowString(filename)
//        << " print_text="   << ShowString(print_text)
//        << " print_number=" << print_number
//        << " next="         << next
//        << " nitems="       << nitems
//        << endl;
// }

///////////////////////////////////////////////////////////////////////////

// template <class Type>
// /*inline*/ void SourceOf<Type>::SetPrintText(const char *str) {
//   if (str) {
//     char text[10000];
//     sprintf(text, "%s %s %s", str, filename, AdditionalPrintText());
//     PrintText(text);
//   } else
//     PrintText(NULL);
// }

// ///////////////////////////////////////////////////////////////////////////

// template <class Type>
// /*inline*/ void SourceOf<Type>::ConditionallySetPrintText(const char *str) {
//   if (HasPrintText())
//     SetPrintText(str);
// }

///////////////////////////////////////////////////////////////////////////

#ifdef EXPLICIT_INCLUDE_CXX
#include <Source.C>
#endif // EXPLICIT_INCLUDE_CXX

} // namespace simple
#endif // _SOURCE_H_

