// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: Point.C,v 1.3 2009/11/20 20:48:15 jorma Exp $

#ifndef _POINT_C_
#define _POINT_C_

#ifdef PRAGMA_INTERFACE
#pragma interface
#endif // PRAGMA_INTERFACE

#include <Point.h>
#include <Planar.h>

namespace simple {

template <class Type>
PointOf<Type> PointOf<Type>::Maximum() { SimpleAbort(); return PointOf<Type>(); }

template <class Type>
PointOf<Type> PointOf<Type>::Minimum() { SimpleAbort(); return PointOf<Type>(); }

template <class Type>
const char *PointOf<Type>::Format() { SimpleAbort(); return NULL; }

template <>
IntPoint IntPoint::Maximum() { return IntPoint(-MAXINT,  MAXINT); }

template <>
IntPoint IntPoint::Minimum() { return IntPoint(-MAXINT, -MAXINT); }

template <>
FloatPoint FloatPoint::Maximum() { return FloatPoint(-MAXFLOAT,  MAXFLOAT); }

template <>
FloatPoint FloatPoint::Minimum() { return FloatPoint(-MAXFLOAT, -MAXFLOAT); }

template <>
const char *IntPoint::Format() { return "%i%i"; }

template <>
const char *FloatPoint::Format() { return "%f%f"; }

#if SIMPLE_PLANAR

// #define __OLD_GNUG__

#ifndef __OLD_GNUG__
#include <Planar.h>
#endif // __OLD_GNUG__

template <class Type> 
/*inline*/ int PointOf<Type>::ToIndex() const {
  if (!planar)
    abort();
#ifdef __OLD_GNUG__
  return intpoint_planar_to_index(this, planar);
#else //  __OLD_GNUG__
  return planar->ToIndex((int)x, (int)y);
#endif //  __OLD_GNUG__
}


template <class Type> 
/*inline*/ PointOf<Type>& PointOf<Type>::operator++() {
  if (!planar)
    abort();
  if (IsDefined()) {
#ifdef __OLD_GNUG__
    intpoint_planar_increment(this, planar);
#else //  __OLD_GNUG__
    x++;
    if (x>=planar->Width()) {
      x = 0;
      y++;
      if (y>=planar->Height())
	SetUndefined();
    }
#endif //  __OLD_GNUG__
  }
  return *this;
}
#endif // SIMPLE_PLANAR 


} // namespace simple

#endif // _POINT_C_

