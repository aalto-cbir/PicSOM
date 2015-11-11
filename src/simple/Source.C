// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: Source.C,v 1.2 2009/11/20 20:48:16 jorma Exp $

#ifndef _SOURCE_C_
#define _SOURCE_C_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <Source.h>

namespace simple {

///////////////////////////////////////////////////////////////////////////

// template <class Type>
// SourceOf<Type>::~SourceOf() {
//   delete filename;
//   delete print_text;
// }

///////////////////////////////////////////////////////////////////////////

template <class Type>
void SourceOf<Type>::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&DumpShort || type&DumpLong)
    os << Bold("Source ")  << (void*)this 
       << " filename="     << ShowString(filename)
       << " print_text="   << ShowString(print_text)
       << " print_number=" << print_number
       << " next="         << next
       << " nitems="       << nitems
       << endl;
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
/*inline*/ void SourceOf<Type>::SetPrintText(const char *str) {
  if (str) {
    char text[10000];
    sprintf(text, "%s %s %s", str, filename, AdditionalPrintText());
    PrintText(text);
  } else
    PrintText(NULL);
}

///////////////////////////////////////////////////////////////////////////

template <class Type>
/*inline*/ void SourceOf<Type>::ConditionallySetPrintText(const char *str) {
  if (HasPrintText())
    SetPrintText(str);
}

///////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _SOURCE_C_

