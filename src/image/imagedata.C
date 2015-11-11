// -*- C++ -*-    $Id: imagedata.C,v 1.3 2015/11/10 18:20:00 jorma Exp $

/**
   \file imagedata.C
   \brief Declarations and definitions of class picsom::imagedata.
  
   imagedata.h contains declarations and definitions of
   class picsom::imagedata, which is a format and library
   independent storage for pixel based images.
   
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.3 $
   $Date: 2015/11/10 18:20:00 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

/**\mainpage PicSOM/image directory

   \section history Background
   Classes picsom::imagedata and picsom::imagefile were written because, ...

   \section philos Philosophy
   The guidelines in the design of picsom::imagedata ...
   
   Similarly, picsom::imagefile ...

   \section requires Requirements
   There are no special requirements ...
*/   

#ifndef _imagedata_C_
#define _imagedata_C_

#include <imagedata.h>

/**
  \brief namespace picsom wraps (eventually) all classes.
  
  imagedata is defined within namespace picsom.  All other classes
  of the project either are there already or will be moved in there. 
*/

namespace picsom {
  /// namespace std is part of the C++ Standard.
  using namespace std;

#define __id__get_one_T(T, n) \
  template <> T imagedata::get_one<T>(size_t x, size_t y) const {   \
    ensure_xy("get_one<T>(int,int)", pixeldata_##n, x, y, 1); \
    return _vec_##n[to_index(x, y)]; }

  __id__get_one_T(unsigned char, uchar)
  __id__get_one_T(uint16_t, uint16)
  __id__get_one_T(uint32_t, uint32)
  __id__get_one_T(float, float)
  __id__get_one_T(double, double)
  __id__get_one_T(complex<float>, scmplx)
  __id__get_one_T(complex<double>, dcmplx)

} // namespace picsom

#endif // !_imagedata_C_

