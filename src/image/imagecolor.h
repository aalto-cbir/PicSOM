// -*- C++ -*-  $Id: imagecolor.h,v 1.8 2008/10/31 11:04:49 jorma Exp $
// 
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

/**
   \file imagecolor.h
   \brief Declarations and definitions of class picsom::imagecolor.
  
   imagecolor.h contains declarations and definitions of
   class picsom::imagecolor, which is a format and library
   independent storage for pixel based images.
   
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.8 $
   $Date: 2008/10/31 11:04:49 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _imagecolor_h_
#define _imagecolor_h_

#include <vector>
#include <string>

/**
  \brief namespace picsom wraps (eventually) all classes.
  
  imagecolor is defined within namespace picsom.  All other classes
  of the project either are there already or will be moved in there. 
*/

namespace picsom {
  /// namespace std is part of the C++ Standard.
  using namespace std;

  /** The class imagecolor is a file format independent pixel data storage.
     The following guidelines have been followed:
     -# independency on image file formats
     -# independency on image library implementations 
     -# compatibility with C++ Standard vector<> classes
     -# datatypes float, double, unsigned char+short+long supported
     -# any number of samples per pixel
     -# no pseudocolor vs. directcolor distinction
  */

  ///
#ifdef __alpha
#pragma message save
#pragma message disable basclsnondto
#endif // __alpha
  template <class T> class imagecolor : public vector<T> {
#ifdef __alpha
#pragma message restore
#endif // __alpha

  public:
    /// Version control identifier of the imagecolor.h file.
    static const string& version() {
      static const string v =
	"$Id: imagecolor.h,v 1.8 2008/10/31 11:04:49 jorma Exp $";
      return v;
    }

    ///
    imagecolor(const string& s = "black") : vector<T>(3) {
      if (s=="white")
	(*this)[0] = (*this)[1] = (*this)[2] = multiplier();
      else if (s=="red")
	(*this)[0] = multiplier();
      else if (s=="green")
	(*this)[1] = multiplier();
      else if (s=="blue")
	(*this)[2] = multiplier();
      else if (s=="cyan")
	(*this)[1] = (*this)[2] = multiplier();
      else if (s=="magenta")
	(*this)[0] = (*this)[2] = multiplier();
      else if (s=="yellow")
	(*this)[0] = (*this)[1] = multiplier();
      else if (s.size()==7 && s[0]=='#') {
	for (size_t i=0; i<3; i++) {
	  unsigned int v = 0;
	  sscanf(s.substr(1+i*2, 2).c_str(), "%2x", &v);
	  // cout << "i=" << i << " v=" << v << endl;
	  (*this)[i] = T(float(v)/255*multiplier());
	}
      }
    }

    ///
    T multiplier() const {
      return T(max_value());
    }

    float max_value() const {
      if (typeid(T)==typeid(float))
	return 1.0;
      if (typeid(T)==typeid(unsigned char))
	return 255.0;
      return 0.0;
    }
  }; // class imagecolor

} // namespace picsom

#endif // !_imagecolor_h_

