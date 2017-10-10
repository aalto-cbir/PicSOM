// -*- C++ -*-    $Id: imagedata.h,v 1.111 2017/04/28 07:49:29 jormal Exp $

/**
   \file imagedata.h
   \brief Declarations and definitions of class picsom::imagedata.
  
   imagedata.h contains declarations and definitions of
   class picsom::imagedata, which is a format and library
   independent storage for pixel based images.
   
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.111 $
   $Date: 2017/04/28 07:49:29 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _imagedata_h_
#define _imagedata_h_

//#include <missing-c-utils.h>
#include <picsom-config.h>

#include <vector>
#include <map>
#include <valarray>
#include <string>
#include <complex>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <cmath>
#include <cstdio>

#include <math.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif // HAVE_INTTYPES_H

#include <imagescaling.h>
#include <imagecolor.h>

#if !defined(PICSOM_IMAGEDATA_FAST)
  #define PICSOM_IMAGEDATA_ENSURE_XY
#endif //PICSOM_IMAGEDATA_FAST

/**
  \brief namespace picsom wraps (eventually) all classes.
  
  imagedata is defined within namespace picsom.  All other classes
  of the project either are there already or will be moved in there. 
*/

namespace picsom {
  /// namespace std is part of the C++ Standard.
  using namespace std;

  /** The class imagedata is a file format independent pixel data storage.
      The following guidelines have been followed:
      -# independency on image file formats
      -# independency on image library implementations 
      -# compatibility with C++ Standard vector<> classes
      -# datatypes float, double, unsigned char+short+long supported
      -# any number of samples per pixel
      -# no pseudocolor vs. directcolor distinction
  */

  class imagedata {
  public:
    /// Version control identifier of the imagedata.h file.
    static const string& version() {
      static const string v =
	"$Id: imagedata.h,v 1.111 2017/04/28 07:49:29 jormal Exp $";
      return v;
    }

    /** Type of the stored data.
	The pixel data is always stored in one of the following
	formats.  It is possible to change between the types, but when
	accessing the data, its format has to be known in advance. \n
	The floating types are assumed to lie within range [0,1] when
	the data is read from or written to external files.  Otherwise,
	the interpretation of the floating values is unlimited.
    */
    enum pixeldatatype {
      pixeldata_undef,   //!< When the type has not been specified yet.
      pixeldata_float,   //!< A float in range [0,1] or other.
      pixeldata_double,  //!< A double in range [0,1] or other.
      pixeldata_uchar,   //!< An unsigned char in range 0..2^8-1=255.
      pixeldata_uint16,  //!< An unsigned short in range 0..2^16-1=65535.
      pixeldata_uint32,  //!< An unsigned long in range 0..2^32-1=4294967295.
      pixeldata_scmplx,  //!< A single-precision complex ([0,1],0).
      pixeldata_dcmplx   //!< A double-precision complex ([0,1],0).
    };

    /** Resolves pixeldatatype from typeid.
	\param  t typeid.
	\throw  std::string if \e t is not produce a valid enum value.
	\return pixeldatatype
    */
    static pixeldatatype datatype(const type_info& t) {
      if (t==typeid(float))                return pixeldata_float;
      if (t==typeid(double))               return pixeldata_double;
      if (t==typeid(unsigned char))        return pixeldata_uchar;
      if (t==typeid(uint16_t))             return pixeldata_uint16;
      if (t==typeid(uint32_t))             return pixeldata_uint32;
      if (t==typeid(std::complex<float>))  return pixeldata_scmplx;
      if (t==typeid(std::complex<double>)) return pixeldata_dcmplx;
      return pixeldata_undef;
    }

    /** Gives pixeldatatype in string form.
	\param  t datatype specifier to be stringified.
	\throw  std::string if \e t is not a valid enum value.
	\return const std::string& like "undef", "float", "uchar" or "uint16".
    */
    static const string& datatypename(pixeldatatype t) throw(string) {
      static map<pixeldatatype,string> pixeldatatype_map;
      if (pixeldatatype_map.empty()) {
	pixeldatatype_map[pixeldata_undef]  = "undef";
	pixeldatatype_map[pixeldata_float]  = "float";
	pixeldatatype_map[pixeldata_double] = "double";
	pixeldatatype_map[pixeldata_uchar]  = "uchar";
	pixeldatatype_map[pixeldata_uint16] = "uint16";
	pixeldatatype_map[pixeldata_uint32] = "uint32";
	pixeldatatype_map[pixeldata_scmplx] = "scmplx";
	pixeldatatype_map[pixeldata_dcmplx] = "dcmplx";
      }

      map<pixeldatatype,string>::const_iterator i = pixeldatatype_map.find(t);
      if (i==pixeldatatype_map.end())
	throw msg("datatypename("+tos(t)+") failed");

      return i->second;
    }

    /** Gives integral pixeldatatype of specified bit count.
	\param  n any bit count of 8, 16 or 32.
	\throw  std::string if \e n is not any of allowed values.
	\return Corresponding datatype.
    */
    static pixeldatatype intdatatype(int n) throw(string) {
      switch (n) {
      case  8: return pixeldata_uchar;
      case 16: return pixeldata_uint16;
      case 32: return pixeldata_uint32;
      }
      throw msg("intdatatype("+tos(n)+") : illegal size value");
    }

    /** Returns true if argument type is defined.
	\param  t queried datatype.
	\return true if \e t is a defined type. 
    */
    static bool is_defined(pixeldatatype t) throw() {
      return is_floating_real(t) || is_floating_cmplx(t) || is_integral(t);
    }

    /** Returns true if argument type is floating real, ie. float or double.
	\param  t queried datatype.
	\return true if \e t is a floating real type. 
    */
    static bool is_floating_real(pixeldatatype t) throw() {
      return t==pixeldata_float || t==pixeldata_double; }

    /** Returns true if argument type is floating complex, ie. scmplx/dcmplx.
	\param  t queried datatype.
	\return true if \e t is a floating complex type. 
    */
    static bool is_floating_cmplx(pixeldatatype t) throw() {
      return t==pixeldata_scmplx || t==pixeldata_dcmplx; }

    /** Returns true if argument type is integral, ie. char, short or long.
	\param  t queried datatype.
	\return true if \e t is an integral type. 
    */
    static bool is_integral(pixeldatatype t) throw() {
      return t==pixeldata_uchar || t==pixeldata_uint16 || t==pixeldata_uint32;}

    //
    // static methods related to pixeldatatype end and imagedata methods start
    // 

    /** The default constructor.
	I believe the default constructor has to be defined if any
	other constructor is defined. Also I believe the C++ Standard
	containers require the existence of the default constructor.\n
	Puts the object in an uninitialized state.
    */
    imagedata() : _type(pixeldata_undef), _count(0), _width(0), _height(0) {}

    /** The "standard" constructor.
	\param w width of the image.
	\param h height of the image.
	\param c channel count of the image.
	\param t datatype of the image.
	\throw std::string if non-positive or senseless arguments.
	\post  Internal data container is allocated.
    */
    imagedata(size_t w, size_t h, size_t c = 3,
	      pixeldatatype t = pixeldata_float) 
      throw(string) : _type(t), _count(c), _width(w), _height(h) {

	if (!is_defined(t))
	  throw msg("imagedata("+tos(w)+", "+tos(h)+", "+tos(c)+", "+tos(t)
		    +") : illegal argument");

	size_t n = _count*_width*_height;
	alloc_vector(_type, n);
      }

    /** A constructor that solves the pixeldatatype from type_info.
	\param w width of the image.
	\param h height of the image.
	\param c channel count of the image.
	\param t type_info of the datatype of the image.
	\throw std::string if non-positive or senseless arguments.
	\post  Internal data container is allocated.
    */
    imagedata(size_t w, size_t h, size_t c , const type_info& t) throw(string) {
      *this = imagedata(w, h, c, datatype(t));
    }

    virtual ~imagedata() {};

    /** Changes the size of the image.
	\param w new width of the image.
	\param h new height of the image.
    */
    void resize(size_t w, size_t h) {
      resize_vector(w*h*_count);
      _width=w;
      _height=h;
    }

    /** Returns the used datatype.
	\return Datatype.
    */
    pixeldatatype type() const { return _type; }

    /** Returns the used datatype in string form.
	\see    datatypename(pixeldatatype)
	\throw  std::string may be thrown by datatypename(pixeldatatype).
	\return Datatype stringified like "undef", "float", "uchar" or "int16".
    */
    const string& datatypename() const throw(string) {
      return datatypename(_type); }

    /** Returns true if used datatype is defined.
	\see    is_defined(pixeldatatype)
	\return true if used datatype really is defined.
    */
    bool is_defined() const throw() { return is_defined(_type); }

    /** Returns true if used datatype is real floating, ie\. float or double.
	\see    is_floating_real(pixeldatatype)
	\return true if used datatype really is floating real.
    */
    bool is_floating_real() const throw() { return is_floating_real(_type); }

    /** Returns true if used datatype is floating complex, ie\. scmplx/dcmplx.
	\see    is_floating_cmplx(pixeldatatype)
	\return true if used datatype really is floating complex.
    */
    bool is_floating_cmplx() const throw() { return is_floating_cmplx(_type); }

    /** Returns true if used datatype is integral, ie char, short or long.
	\see    is_integral(pixeldatatype)
	\return true if used datatype really is integral.
    */
    bool is_integral() const throw() { return is_integral(_type); }

    /** Returns the channel count of the image.
	\return Count of channels in the image.
    */
    size_t count() const { return _count; }

    /** Returns the width of the image.
	\return Width of the image. 
    */
    size_t width() const { return _width; }

    /** Returns the height of the image.
	\return Height of the image. 
    */
    size_t height() const { return _height; }

    /** Dumps some basic information about the object.
	\return string of information.
    */
    string info() const {
      stringstream s;
      s << "imagedata " << _width << "x" << _height << " x " << _count
	<< "x" << datatypename() << " @" << (void*)this;
      return s.str();
    }

    /** Returns true if the image is uninitialized.
        \return true for empty image.
    */
    bool isempty() const {
      return _type==pixeldata_undef || !_width || !_height || !_count;
    }

    /** Returns true if the image consist of zero-valued pixels.
        \return true for zero-valued image.
    */

    bool iszero() const {
      switch (_type) {
      case pixeldata_float:   return _iszero_vec(_vec_float);
      case pixeldata_double:  return _iszero_vec(_vec_double);
      case pixeldata_uchar:   return _iszero_vec(_vec_uchar);
      case pixeldata_uint16:  return _iszero_vec(_vec_uint16);
      case pixeldata_uint32:  return _iszero_vec(_vec_uint32);
      case pixeldata_scmplx:  return _iszero_vec(_vec_scmplx);
      case pixeldata_dcmplx:  return _iszero_vec(_vec_dcmplx);
      default:
	throw msg("iszero() : switch ("+datatypename(_type)+") failed");
      }
    }

    /** Returns true if the image size is zero.
        \return true for zero-sized image.
    */
    bool empty() const { return _width==0 || _height==0; }

    /** Checks the validity of xy coordinates.
	\param x coordinate along "width" axis.
	\param y coordinate along "height" axis.
	\return true if (x,y) lies within the image.
    */
    bool coordinates_ok(size_t x, size_t y) const {
      return x<_width && y<_height;
    }

    //
    // The primary methods to access the image data are named get() and set().
    // 
    // The get() and set() functions come in three flavors:
    // 
    // 1) These methods access the whole data array as one:
    //    vector<T> get_T() const;
    //    vector<T> get<T>() const;
    //    void      set(const vector<T>&);
    // 
    //    vector<T> get_ordered_T(const string&) const;
    //    vector<T> get_ordered<T>(const string&) const;
    // 
    // 2) These methods access one pixel as a vector:
    //    vector<T> get_T(size_t, size_t) const;
    //    vector<T> get<T>(size_t, size_t) const;
    //    void      set(size_t, size_t, const vector<T>&);
    //
    // 3) These methods access one pixel as a scalar provided it is such:
    //    T    get_one_T(size_t, size_t) const;
    //    T    get_one<T>(size_t, size_t) const;
    //    void set(size_t, size_t, T);
    //
    // As you can see, all set() functions have the same name, but the
    // get() functions have two times count of T different names...

    /// Implements "vector<T> get_T() const" functions which make a
    /// copy of it all:
#define __id__get_all(n)			\
    ensure_type("get_" #n "()", pixeldata_##n); \
	return _vec_##n;
    
    //@{
    /** Copies out the whole image as one possibly longish vector.
	\throw  std::string if datatype does not match.
	\return Copied pixels in vector.
    */
    vector<float>             get_float()  const { __id__get_all(float);  }
    vector<double>            get_double() const { __id__get_all(double); }
    vector<unsigned char>     get_uchar()  const { __id__get_all(uchar);  }
    vector<uint16_t>          get_uint16() const { __id__get_all(uint16); }
    vector<uint32_t>          get_uint32() const { __id__get_all(uint32); }
    vector<complex<float> >   get_scmplx() const { __id__get_all(scmplx); }
    vector<complex<double> >  get_dcmplx() const { __id__get_all(dcmplx); }
    //@}

    vector<float> get_ordered_float(const string& s,
				    vector<size_t>& size,
				    vector<size_t>& stride) const {
      ensure_type("get_ordered_float", pixeldata_float);
      if (s=="" || s=="yxc") {
	size   = vector<size_t> { _height, _width, _count };
	stride = vector<size_t> { _width*_count, _count, 1 };
	return _vec_float;
      }
      if (s=="cyx") {
	const float *d = _vec_float.data();
	vector<float> v(_vec_float.size());
	size_t i=0;
	for (size_t c=0; c<_count; c++)
	  for (size_t y=0; y<_height; y++)
	    for (size_t x=0; x<_width; x++, i++)
	      v[i] = d[to_index(x, y, c)];
	size   = vector<size_t> { _count, _height, _width };
	stride = vector<size_t> { _height*_width, _width, 1 };
	return v;
      }
      size.clear();
      stride.clear();
      return vector<float>();
    }

    /// Implements "set(const vector<T>&)" functions which set it all
    /// in one call
#define __id__set_all(n, d)						\
    ensure_size("set(const vector<" #n ">&)", pixeldata_##n, d.size());	\
	_vec_##n = d;
    
    //@{
    /** Copies in the whole image from one possibly longish vector.
	\throw std::string if datatype or vector size does not match. 
    */
    void set(const vector<float>& d)          	 { __id__set_all(float,  d); }
    void set(const vector<double>& d)         	 { __id__set_all(double, d); }
    void set(const vector<unsigned char>& d)  	 { __id__set_all(uchar,  d); }
    void set(const vector<uint16_t>& d) 	 { __id__set_all(uint16, d); }
    void set(const vector<uint32_t>& d)  	 { __id__set_all(uint32, d); }
    void set(const vector<complex<float> >& d)   { __id__set_all(scmplx, d); }
    void set(const vector<complex<double> >& d)  { __id__set_all(dcmplx, d); }
    //@}

    /// Implements "get_T(size_t,size_t)" functions which get a copy of
    /// values for one pixel.
#define __id__get_vec(x, y, t, n)				\
    ensure_xy("get_" #n "(size_t,size_t)", pixeldata_##n, x, y, -1);	\
	vector<t>::const_iterator				\
	i = _vec_##n.begin()+to_index_w_count(x, y);		\
	return vector<t>(i, i+_count);

    //@{
    /** Copies out values from one pixel.
	\param x coordinate along "width" axis.
	\param y coordinate along "height" axis.
	\throw std::string if datatype doesn't match or coordinates not valid.
	\return Copied vector of values.
    */
    vector<float> get_float(size_t x, size_t y) const {
      __id__get_vec(x, y, float, float); }

    vector<double> get_double(size_t x, size_t y) const {
      __id__get_vec(x, y, double, double); }

    vector<unsigned char> get_uchar(size_t x, size_t y) const {
      __id__get_vec(x, y, unsigned char, uchar); }

    vector<uint16_t> get_uint16(size_t x, size_t y) const {
      __id__get_vec(x, y, uint16_t, uint16); }

    vector<uint32_t> get_uint32(size_t x, size_t y) const {
      __id__get_vec(x, y, uint32_t, uint32); }

    vector<complex<float> > get_scmplx(size_t x, size_t y) const {
      __id__get_vec(x, y, complex<float>, scmplx); }

    vector<complex<double> > get_dcmplx(size_t x, size_t y) const {
      __id__get_vec(x, y, complex<double>, dcmplx); }
    //@}

    /// Implements "set(size_t,size_t,const vector<T>&)" functions which set
    /// the values for one pixel.
#define __id__set_vec(x, y, n, d)					\
    ensure_xy("set(size_t, size_t, const vector<" #n ">&)",		\
	      pixeldata_##n, x, y, d.size());				\
	size_t b = to_index_w_count(x, y);				\
	for (size_t i=0; i<_count; i++) _vec_##n[b++] = d[i];

    //@{
    /** Sets values for the specified pixel.
	\param x coordinate along "width" axis.
	\param y coordinate along "height" axis.
	\param d pixel data in a vector.
	\throw std::string if datatype or length of \e d does not match or
	coordinates are invalid
    */
    void set(size_t x, size_t y, const vector<float>& d) {
      __id__set_vec(x, y, float, d); }

    void set(size_t x, size_t y, const vector<double>& d) {
      __id__set_vec(x, y, double, d); }

    void set(size_t x, size_t y, const vector<unsigned char>& d) {
      __id__set_vec(x, y, uchar, d); }

    void set(size_t x, size_t y, const vector<uint16_t>& d) {
      __id__set_vec(x, y, uint16, d); }

    void set(size_t x, size_t y, const vector<uint32_t>& d) {
      __id__set_vec(x, y, uint32, d); }

    void set(size_t x, size_t y, const vector<complex<float> >& d) {
      __id__set_vec(x, y, scmplx, d); }

    void set(size_t x, size_t y, const vector<complex<double> >& d) {
      __id__set_vec(x, y, dcmplx, d); }
    //@}

    /// Implements "get_one_T(size_t,size_t)" functions which get the scalar
    /// value of one pixel.
#define __id__get_one(x, y, n)						\
    ensure_xy("get_one_" #n "(size_t,size_t)", pixeldata_##n, x, y, 1);	\
	return _vec_##n[to_index(x, y)];
    
    //@{
    /** Gets scalar value of the specified pixel.
	\param x coordinate along "width" axis.
	\param y coordinate along "height" axis.
	\throw std::string if datatype or length of \e d does not match or
	coordinates are invalid.
	\return The value of the scalar pixel.
    */
    float get_one_float(size_t x, size_t y) const {
      __id__get_one(x, y, float); }

    double get_one_double(size_t x, size_t y) const {
      __id__get_one(x, y, double); }

    unsigned char get_one_uchar(size_t x, size_t y) const {
      __id__get_one(x, y, uchar); }

    uint16_t get_one_uint16(size_t x, size_t y) const {
      __id__get_one(x, y, uint16); }

    uint32_t get_one_uint32(size_t x, size_t y) const {
      __id__get_one(x, y, uint32); }

    complex<float> get_one_scmplx(size_t x, size_t y) const {
      __id__get_one(x, y, scmplx); }

    complex<double> get_one_dcmplx(size_t x, size_t y) const {
      __id__get_one(x, y, dcmplx); }

    template <class T> T get_one(size_t x, size_t y) const;
    //@}


    /// Implements "set(size_t,size_t,T)" functions which set the scalar
    /// value of one pixel.
#define __id__set_one(x, y, n, v)				\
    ensure_xy("set(size_t,size_t," #n ")", pixeldata_##n, x, y, 1);	\
	_vec_##n[to_index(x, y)] = v;

    //@{
    /** Sets value of specified pixel to given float value.
	\throw std::string if invalid coordinates or datatype mismatch. 
    */
    void set(size_t x, size_t y, float v)           {__id__set_one(x, y, float,  v);}
    void set(size_t x, size_t y, double v)          {__id__set_one(x, y, double, v);}
    void set(size_t x, size_t y, unsigned char v)   {__id__set_one(x, y, uchar,  v);}
    void set(size_t x, size_t y, uint16_t v)  {__id__set_one(x, y, uint16, v);}
    void set(size_t x, size_t y, uint32_t v)   {__id__set_one(x, y, uint32, v);}
    void set(size_t x, size_t y, complex<float> v)  {__id__set_one(x, y, scmplx, v);}
    void set(size_t x, size_t y, complex<double> v) {__id__set_one(x, y, dcmplx, v);}
    //@}

    // 
    // There also exist secondary methods to access the data named raw().
    // The raw() functions have now been implemented only for reading.
    // For a tiny security measure, calls to raw() need to guess the size.
    // 

    /// Implements "const T *raw_T() const" functions which return it all.
#define __id__raw__(n, s) ensure_size("raw_" #n "(size_t)", pixeldata_##n, s); \
	return &*_vec_##n.begin();

    //@{
    /** Gives access to the internal data in plain C pointer form.
	\param s size of the buffer to show that the caller knows it.
	\throw std::string if \e s is incorrect or datatype mismatches.
	\return Pointer to raw data.
    */
    const float            *raw_float(size_t s) const { __id__raw__(float,  s); }
    const double          *raw_double(size_t s) const { __id__raw__(double, s); }
    const unsigned char    *raw_uchar(size_t s) const { __id__raw__(uchar,  s); }
    const uint16_t  *raw_uint16(size_t s) const { __id__raw__(uint16, s); }
    const uint32_t   *raw_uint32(size_t s) const { __id__raw__(uint32, s); }
    const complex<float>  *raw_scmplx(size_t s) const { __id__raw__(scmplx, s); }
    const complex<double> *raw_dcmplx(size_t s) const { __id__raw__(dcmplx, s); }
    //@}


    /** Gives access to the internal data in plain C pointer form.
	Calls the appropriate raw_XXX function depending on the current
	internal pixel data type.
	\param   n size of the buffer to show that the caller knows it.
	\throw   std::string if not of correct type.
	\return  const void* for any kind of read-only access.
	\warning This may cause trouble if not used with caution...
    */
    const void *raw(size_t n) const throw(string) {
      switch (_type) {
      case pixeldata_float:  return raw_float(n);
      case pixeldata_double: return raw_double(n);
      case pixeldata_uchar:  return raw_uchar(n);
      case pixeldata_uint16: return raw_uint16(n);
      case pixeldata_uint32: return raw_uint32(n);
      case pixeldata_scmplx: return raw_scmplx(n);
      case pixeldata_dcmplx: return raw_dcmplx(n);
      default:
	throw msg("raw() of type "+datatypename()+" not implemented");
      }
    }

    // for setting of raw data, no security checks
    void set_raw_from_uint16(uint16_t val, size_t ind) {
      decanonize_uint16(val, _type, ind);
    }

    // for setting of raw data, no security checks
    void set_raw_from_uchar(unsigned char val, size_t ind) {
      decanonize_uchar(val, _type, ind);
    }

#define __id__fill__(x, y, w, h, n, d)					\
    ensure_type("fillTile(const vector<" #n ">&)", pixeldata_##n);	\
	ensure_count("fillTile(const vector<" #n ">&)", d.size());	\
	for (size_t yoff=0; yoff<h; yoff++)                             \
	  for (size_t xoff=0; xoff<w; xoff++)	                        \
	    set(x+xoff, y+yoff, d);
    
    void fillTile(size_t x, size_t y, size_t w, size_t h, const vector<float> &v){
      __id__fill__(x, y, w, h, float,  v);}
    void fillTile(size_t x, size_t y, size_t w, size_t h, const vector<double> &v){
      __id__fill__(x, y, w, h,double, v);}
    void fillTile(size_t x, size_t y, size_t w, size_t h, const vector<unsigned char> &v){
      __id__fill__(x, y, w, h,uchar,  v);}
    void fillTile(size_t x, size_t y, size_t w, size_t h, const vector<uint16_t> &v){
      __id__fill__(x, y, w, h,uint16, v);}
    void fillTile(size_t x, size_t y, size_t w, size_t h, const vector<uint32_t> &v){
      __id__fill__(x, y, w, h,uint32, v);}
    void fillTile(size_t x, size_t y, size_t w, size_t h, const vector<complex<float> > &v){
      __id__fill__(x, y, w, h,scmplx, v);}
    void fillTile(size_t x, size_t y, size_t w, size_t h, const vector<complex<double> > &v){
      __id__fill__(x, y, w, h,dcmplx, v);}


#define __id__copysub__(dtype,n)				     \
        size_t w = src.width(), ws = w, h = src.height(), srcy = 0;  \
        const dtype *srcptr = (const dtype*)src.raw(w*h*count());    \
        if (x<0) { srcptr -= x*count(); ws += x; x = 0; }            \
        if (y<0) { srcptr -= y*w*count(); srcy = -y; y = 0; }        \
	dtype *dstptr = &*_vec_##n.begin();			     \
	dstptr += count()*(width()*y+x);			     \
        size_t wm = width()-x;                                       \
        if (ws<wm) wm = ws;                                          \
	for (size_t dsty=y; srcy<h&&dsty<height(); srcy++, dsty++) { \
	  memcpy(dstptr, srcptr, wm*count()*sizeof(dtype));	     \
	  dstptr += width()*count();				     \
	  srcptr += w*count();} 


    void copyAsSubimage(const imagedata &src, int x, int y) {
      ensure_type("copyAsSubimage()",src.type());
      ensure_count("copyAsSubimage()",src.count());

      switch (_type) {
      case pixeldata_float:
	{
	  __id__copysub__(float,float);
	}
	break;
      case pixeldata_double:
	{
	  __id__copysub__(double,double);
	}
	break;
      case pixeldata_uchar:
	{
	  __id__copysub__(unsigned char,uchar);
	}
	break;
      case pixeldata_uint16:
	{
	  __id__copysub__(uint16_t,uint16);
	}
	break;
      case pixeldata_uint32:
	{
	  __id__copysub__(uint32_t,uint32);
	}
	break;
      case pixeldata_scmplx:
	{
	  __id__copysub__(complex<float>,scmplx);
	}
	break;
      case pixeldata_dcmplx:
	{
	  __id__copysub__(complex<double>,dcmplx);
	}
	break;
      default:
	throw msg("copyAsSubimage() of type "+datatypename()+
		  " not implemented");
      }
    }

    void copyAsSubimage(const imagedata &src, const vector<int>& c) {
      if (c.size()!=8)
	throw string("copyAsSubimage() c.size()!=8");

      float dx1 = c[0]-c[6], dy1 = c[1]-c[7];
      float l1  = sqrt(dx1*dx1+dy1*dy1);
      float dx2 = c[2]-c[4], dy2 = c[3]-c[5];
      float l2  = sqrt(dx2*dx2+dy2*dy2);

      float maxl = l2>l1?l2:l1;
      float dy = 1/maxl;
      
      for (float fy=0; fy<=1; fy+=dy) {
	size_t sy = floor(fy*(src.height()-1));

	float xa = c[0]+fy*(c[6]-c[0]);
	float ya = c[1]+fy*(c[7]-c[1]);
	float xb = c[2]+fy*(c[4]-c[2]);
	float yb = c[3]+fy*(c[5]-c[3]);
	float l =  sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb));
	float dx = 1/l;

	for (float fx=0; fx<=1; fx+=dx) {
	  size_t sx = floor(fx*(src.width()-1));
	  
	  float xx = xa+fx*(xb-xa);
	  float yy = ya+fx*(yb-ya);
	  size_t ixx = (size_t)floor(xx+0.5);
	  size_t iyy = (size_t)floor(yy+0.5);

	  if (coordinates_ok(ixx, iyy))
	    set(ixx, iyy, src.get_uchar(sx, sy));
	}
      }
    }

    void copyAsSubimageInv(const imagedata &src, const vector<int>& c) {
      if (c.size()!=8)
	throw string("copyAsSubimageInv() c.size()!=8");

      for (size_t iyy=0; iyy<height(); iyy++) {
	float fy = iyy/(height()-1.0);
	float xa = c[0]+fy*(c[6]-c[0]);
	float ya = c[1]+fy*(c[7]-c[1]);
	float xb = c[2]+fy*(c[4]-c[2]);
	float yb = c[3]+fy*(c[5]-c[3]);

	for (size_t ixx=0; ixx<width(); ixx++) {
	  float fx = ixx/(width()-1.0);
	  float xx = xa+fx*(xb-xa);
	  float yy = ya+fx*(yb-ya);
	  size_t sxx = (size_t)floor(xx+0.5);
	  size_t syy = (size_t)floor(yy+0.5);

	  set(ixx, iyy, src.get_uchar(sxx, syy));
	}
      }
    }

   /** Changes the internal pixel data type
	\param t the new pixel data type
	\return the old pixel data type
    */
    pixeldatatype convert(pixeldatatype t) throw(string) {
      if (!is_defined(t))
	throw msg("convert() : strange destination type");

      pixeldatatype old_type = _type;
      if (t!=_type) {
	const size_t n = vector_size();
	
	alloc_vector(t, n);

	for (size_t i=0; i<n; i++) {
	  _canontype v = canonize(i);
	  decanonize(v, t, i);
	}

	clear_vector();
	_type = t;
      }
      return old_type;
    }

    /// OBS: add comment!
    bool conversion_test(pixeldatatype t = pixeldata_double) {
      imagedata copy = *this;
      copy.convert(t);
      copy.convert(_type);
      cout << "imagedata::conversion_test()";
      if (vector_size()!=copy.vector_size()) {
	cout << " vector sizes differ" << endl;
	return false;
      }
      bool fail = false;
      if (_vec_float!=copy._vec_float) {
	fail = true; cout << " float vectors differ"; }
      if (_vec_double!=copy._vec_double) {
	fail = true; cout << " double vectors differ"; }
      if (_vec_uchar!=copy._vec_uchar) {
	fail = true; cout << " uchar vectors differ"; }
      if (_vec_uint16!=copy._vec_uint16) {
	fail = true; cout << " uint16 vectors differ"; }
      if (_vec_uint32!=copy._vec_uint32) {
	fail = true; cout << " uint32 vectors differ"; }
      if (_vec_scmplx!=copy._vec_scmplx) {
	fail = true; cout << " scmplx vectors differ"; }
      if (_vec_dcmplx!=copy._vec_dcmplx) {
	fail = true; cout << " dcmplx vectors differ"; }

      cout << (fail?"": " OK") << endl;
      
      return !fail;
    }

    /// Forces the data into using three channels.
    void force_three_channel() {
      if (_count!=3) {
	if (_count!=1) {
	  char str[80];
	  sprintf(str, "don't know how to force three channels out of %d",
		  (int)_count);
	  throw msg(str);
	}

	size_t n =_width*_height;
	switch (_type) {
	case pixeldata_float:
	  {
	    vector<float> tmp = _vec_float;
	    _vec_float.resize(3*n);
	    size_t i,j;
	    for(i=0,j=0;i<n;i++){
	      _vec_float[j++]=tmp[i];
	      _vec_float[j++]=tmp[i];
	      _vec_float[j++]=tmp[i];
	    }
	  }
	  break;

	case pixeldata_double:
	  {
	    vector<double> tmp = _vec_double;
	    _vec_double.resize(3*n);
	    size_t i,j;
	    for(i=0,j=0;i<n;i++){
	      _vec_double[j++]=tmp[i];
	      _vec_double[j++]=tmp[i];
	      _vec_double[j++]=tmp[i];
	    }
	  }
	  break;

	case pixeldata_uchar:
	  {
	    vector<unsigned char> tmp = _vec_uchar;
	    _vec_uchar.resize(3*n);
	    size_t i,j;
	    for(i=0,j=0;i<n;i++){
	      _vec_uchar[j++]=tmp[i];
	      _vec_uchar[j++]=tmp[i];
	      _vec_uchar[j++]=tmp[i];
	    }
	  }
	  break;


	case pixeldata_uint16:
	  {
	    vector<uint16_t> tmp = _vec_uint16;
	    _vec_uint16.resize(3*n);
	    size_t i,j;
	    for(i=0,j=0;i<n;i++){
	      _vec_uint16[j++]=tmp[i];
	      _vec_uint16[j++]=tmp[i];
	      _vec_uint16[j++]=tmp[i];
	    }
	  }
	  break;

	case pixeldata_uint32:
	  {
	    vector<uint32_t> tmp = _vec_uint32;
	    _vec_uint32.resize(3*n);
	    size_t i,j;
	    for(i=0,j=0;i<n;i++){
	      _vec_uint32[j++]=tmp[i];
	      _vec_uint32[j++]=tmp[i];
	      _vec_uint32[j++]=tmp[i];
	    }
	  }
	  break;
	  
	  // obs! complex types missing

	default:
	  throw msg("unsupported pixeldata type");
	}
	
	_count = 3;

      }
    }

    /// Forces the data into using one channel by averaging over three channels.
    void force_one_channel(const vector<float>& min = vector<float>()) {
      if (_count!=1) {
	if (_count!=3) {
	  char str[80];
	  sprintf(str, "don't know how to force three channels out of %d",
		  (int)_count);
	  throw msg(str);
	}

	vector<float> m = min;
	if (m.size()==0) {
	  m.push_back(1/3.0);
	  m.push_back(1/3.0);
	  m.push_back(1/3.0);
	}	  

	size_t n =_width*_height;
	switch (_type) {
	case pixeldata_float:
	  {
	    vector<float> tmp = _vec_float;
	    _vec_float.resize(n);
	    for (size_t i=0, j=0; j<n; j++, i+=3)
	      _vec_float[j] = m[0]*tmp[i]+m[1]*tmp[i+1]+m[2]*tmp[i+2];
	  }
	  break;

	case pixeldata_double:
	  {
	    vector<double> tmp = _vec_double;
	    _vec_double.resize(n);
	    for (size_t i=0, j=0; j<n; j++, i+=3)
	      _vec_double[j] = m[0]*tmp[i]+m[1]*tmp[i+1]+m[2]*tmp[i+2];
	  }
	  break;

	case pixeldata_uchar:
	  {
	    vector<unsigned char> tmp = _vec_uchar;
	    _vec_uchar.resize(n);
	    for (size_t i=0, j=0; j<n; j++, i+=3)
	      _vec_uchar[j] = (unsigned char)
		floor(m[0]*tmp[i]+m[1]*tmp[i+1]+m[2]*tmp[i+2]+0.5);
	  }
	  break;


	case pixeldata_uint16:
	  {
	    vector<uint16_t> tmp = _vec_uint16;
	    _vec_uint16.resize(n);
	    for (size_t i=0, j=0; j<n; j++, i+=3)
	      _vec_uint16[j] = (uint16_t)
		floor(m[0]*tmp[i]+m[1]*tmp[i+1]+m[2]*tmp[i+2]+0.5);
	  }
	  break;

	case pixeldata_uint32:
	  {
	    vector<uint32_t> tmp = _vec_uint32;
	    _vec_uint32.resize(n);
	    for (size_t i=0, j=0; j<n; j++, i+=3)
	      _vec_uint32[j] = (uint32_t)
		floor(m[0]*tmp[i]+m[1]*tmp[i+1]+m[2]*tmp[i+2]+0.5);
	  }
	  break;
	  
	  // obs! complex types missing

	default:
	  throw msg("unsupported pixeldata type");
	}
	
	_count = 1;
      }
    }

    /** Extracts a subimage.
	\param 	ulx upper left x-coordinate of the subimage.
	\param 	uly upper left y-coordinate of the subimage.
	\param 	lrx lower right x-coordinate of the subimage.
	\param 	lry lower right y-coordinate of the subimage.
	\throw 	std::string if something goes wrong.
	\return a subimage with otherwise similar properties.
    */
    imagedata subimage(size_t ulx, size_t uly,
		       size_t lrx, size_t lry) const throw(string) {
      if (!coordinates_ok(ulx, uly) || !coordinates_ok(lrx, lry) ||
	  lrx<=ulx || lry<=uly) {
	stringstream txt;
	txt << " ulx=" << ulx << " uly=" << uly
	    << " lrx=" << lrx << " lry=" << lry
	    << " with " << info();
	throw msg("subimage(...) coordinate error:"+txt.str());
      }

      imagedata ret(lrx-ulx+1, lry-uly+1, _count, _type);
      for (size_t x=ulx; x<=lrx; x++)
	for (size_t y=uly; y<=lry; y++)
	  ret.set_canon(x-ulx, y-uly, get_canon(x, y));
      return ret;
    }

    /** Changes the scale of the image data.
	\param si information for the scaling
    */
    void rescale(const scalinginfo& si, size_t interp = 0) throw(string) {
      /// this should work for other types too...
      /// perhaps through canonization...
      
      switch (_type) {
      case pixeldata_float:
	_rescale<float>(si, interp);
	break;
      case pixeldata_double:
	_rescale<double>(si, interp);
	break;
      case pixeldata_uchar:
	_rescale<unsigned char>(si, interp);
	break;
      case pixeldata_uint16:
	_rescale<uint16_t>(si, interp);
	break;
      case pixeldata_uint32:
	_rescale<uint32_t>(si, interp);
	break;

      default:
	throw msg("rescale() : unsupported pixeldata type");
      }
    }

    /** Rotate the image data.
	\param si information for the rotation
    */
    void rotate(const scalinginfo& si, int interp = 0,
		int bbox = 0) throw(string) {
      /// this should work for other types too...
      /// perhaps through canonization...
      
      switch (_type) {
      case pixeldata_float:
	_rotate<float>(si, interp, bbox);
	break;
      case pixeldata_double:
	_rotate<double>(si, interp, bbox);
	break;
      case pixeldata_uchar:
	_rotate<unsigned char>(si, interp, bbox);
	break;
      case pixeldata_uint16:
	_rotate<uint16_t>(si, interp, bbox);
	break;
      case pixeldata_uint32:
	_rotate<uint32_t>(si, interp, bbox);
	break;

      default:
	throw msg("rotate() : unsupported pixeldata type");
      }
    }
    
    /** Reverses the image pixel values.
     */
    void invert() throw(string) {
      size_t i = 0, n = _width*_height*_count;
      switch (_type) {
      case pixeldata_float:
	for (; i<n; i++)
	  _vec_float[i] = 1-_vec_float[i];
	break;

      case pixeldata_double:
	for (; i<n; i++)
	  _vec_double[i] = 1-_vec_double[i];
	break;

      case pixeldata_uchar:
	for (; i<n; i++)
	  _vec_uchar[i] = 255-_vec_uchar[i];
	break;

      default:
	throw msg("invert() : unsupported pixeldata type");
      }
    }

    /** Calculates histogram.
     */
    vector<size_t> histogram(int x0=-1, int y0=-1,
			  int x1=-1, int y1=-1) const throw(string) {
      if (_type!=pixeldata_uchar || _count!=1)
	throw msg("histogram() : only implemented for 1 x uchar images");

      vector<size_t> ret(256);

      if (y1<0 || x1<0 || y0<0 || y1<0) {
	const size_t n = _width*_height;
	for (size_t i=0; i<n; i++)
	  ret[_vec_uchar[i]]++;

      } else {
	for (size_t x=(size_t)x0; x<=(size_t)x1; x++)
	  for (size_t y=(size_t)y0; y<=(size_t)y1; y++)
	    ret[_vec_uchar[to_index_w_count(x, y)]]++;
      }

      return ret;
    }

    map<int,size_t> histogram_uint16() throw(string) {
      if (_type!=pixeldata_uint16 || _count!=1)
	throw msg("histogram_uint16() : "
		  "only implemented for 1 x uint16 images");

      map<int,size_t> ret;

      const size_t n = _width*_height;
      for (size_t i=0; i<n; i++)
	ret[_vec_uint16[i]]++;

      return ret;
    }

    /// Colorizes the image data.
    void colorize() {
      if (_count!=1 || _type!=pixeldata_uchar)
	throw msg("colorize() not a proper type");

      force_three_channel();
      for (size_t y=0; y<height(); y++)
	for (size_t x=0; x<width(); x++) {
	  int v = get_uchar(x, y)[0];
	  // cout << "colorize(): " << x << "," << y << " : " << v << endl;
	  set(x, y, color_wbrgbcmy(v));
	}
    }

    /// Contains the current palette type.
    typedef vector<unsigned char> _palettetype;

    /// jl
    static const _palettetype& color_common(_palettetype (*f)(size_t),
					    unsigned char i,
					    vector<_palettetype>& c) {
      if (i>=c.size())
	for (size_t j=c.size(); j<=i; j++)
	  c.push_back((*f)(j));
      return c[i];
    }

    /// jl
    static const _palettetype& color_wbrgbcmy(unsigned char v) {
      static vector<_palettetype> c;
      return color_common(make_wbrgbcmy, v, c);
    }

    /// jl
    static _palettetype make_wbrgbcmy(size_t ii) {
      _palettetype v(3);

      if (ii==0)
	v = _palettetype(3, (unsigned char)255);

      else if (ii!=1) {
	size_t i = ii;
	i -= 2;
	int j = i/6;
	int jj = 0;
	if (j==1)
	  jj = 64;
	else if (j==2)
	  jj = 32;
	else if (j==3)
	  jj = 96;

	i = i%6;
	bool cmy = i>2;
	i = i%3;
	for (size_t k=0; k<3; k++) {
	  unsigned char a = 255-jj;
	  v[(k+i)%3] = ((!k)!=cmy) ? a : 255-a;
	}
      }

      cout << "make_wbrgbcmy(" << ii << "): "
	   << (int)v[0] << " " << (int)v[1] << " " << (int)v[2] << endl;

      return v;
    }

    // ------------------------------------------------------------------------

    /** Creates complex object from two real ones.
	\param  x real part.
	\param  y imaginary part.
	\throw  std::string if something goes wrong.
	\return complex image with _type==pixeldata_scmplx.
    */
    static imagedata make_cmplx(const imagedata& x, const imagedata& y)
      throw(string) {
      if ((!x.is_floating_real()&&!x.is_integral()) ||
	  (!y.is_floating_real()&&!y.is_integral()))
	throw msg("make_cmplx() failed with types");

      if (x.width()!=y.width() || x.height()!=y.height() ||
	  x.count()!=y.count())
	throw msg("make_cmplx() failed with sizes");
      
      imagedata ret(x);
      ret.convert(pixeldata_scmplx);
      const size_t n = ret.vector_size();
      complex<float> *raw = (complex<float>*) ret.raw_scmplx(n);

      for (size_t i=0; i<n; i++)
	raw[i] += complex<float>(0, y.canonize(i));

      return ret;
    }

    // ------------------------------------------------------------------------

    /** Multiplies image with constant.
	\param  m multiplier.
	\return multiplied image.
    */
    imagedata multiply(double m) const throw(string) {
      if (m==1.0)
	return *this;

      imagedata ret(*this);
      const size_t n = vector_size();
      for (size_t i=0; i<n; i++) {
	_canontype v = canonize(i);
	ret.decanonize(m*v, _type, i);
      }

      return ret;
    }

    // ------------------------------------------------------------------------

    /** Convolves image with two one-dimensional filters.
	\param  h horizontal 1-d filter.
	\param  v vertical 1-d filter.
	\param  m optional multiplier factor.
	\param  zeropad ; if true, calculates the convolution also
                on borders of the img w/ assumption that outside the 
		image area the values are zero
	\param circular; if true calculates circular convolution 	

	\return filtered image.
    */
    imagedata convolve(const vector<float>& h, const vector<float>& v,
		       double m = 1.0, bool zeropad=false,
		       bool circular=false) const throw(string) {
      if (h.empty() && v.empty())
	return m==1.0 ? *this : multiply(m);

      if (h.empty())
	return convolve_vertical(v, m,zeropad,circular);
	  
      return convolve_horizontal(h,1.0,zeropad,circular).
	convolve_vertical(v, m,zeropad,circular);
    }

    // ------------------------------------------------------------------------

    /** Convolves image horizontally with a one-dimensional filter.
	\param  h horizontal 1-d filter.
	\param  m optional multiplier factor.
	\param  zeropad ; if true, calculates the convolution also
                on borders of the img w/ assumption that outside the 
		image area the values are zero
	\param circular; if true calculates circular convolution 	

	\return filtered image.
    */
    imagedata convolve_horizontal(const vector<float>& h,
				  double m = 1.0,bool zeropad=false,
				  bool circular=false)
      const throw(string) {
      if(zeropad && circular)
	throw std::string("imagedata::convolve_horizontal(): zeropad && circular");

      if (h.empty())
	return m==1.0 ? *this : multiply(m);

      imagedata ret(*this);
	  
      const unsigned int xb = (h.size()-1)/2, xe = _width-h.size()/2;
      const unsigned int dx = xb*_count; 

      //       cout << xb << " " << xe << " " << dx << endl;

      unsigned int i=0;
      for (unsigned int y=0; y<_height; y++) {
	unsigned int x = 0;

	if(circular){
	  for (; x<_width; x++)
	    for (unsigned int j=0; j<_count; j++, i++) {
	      _canontype s = 0;
	      for (unsigned int t=0; t<h.size(); t++) {
		s += h[t]*canonize(i+((x-xb+t+_width)%_width-x)*_count);
	      }
	      ret.decanonize(m*s, _type, i);
	    }
	} else{
	  for (; x<xb; x++)
	    for (unsigned int j=0; j<_count; j++, i++)
	      if(zeropad==false)
		ret.decanonize(0.0, _type, i);
	      else{
		_canontype s = 0;
		for (unsigned int t=xb-x; t<h.size(); t++) {
		  s += h[t]*canonize(i-dx+t*_count);
		}
		ret.decanonize(m*s, _type, i);
	      }	   
	  
	  for (; x<xe; x++)
	    for (unsigned int j=0; j<_count; j++, i++) {
	      _canontype s = 0;
	      for (unsigned int t=0; t<h.size(); t++) {
		s += h[t]*canonize(i-dx+t*_count);
		// 	      _canontype v = canonize(i-dx+t*_count);
		// 	      cout << y << " " << x << " " << j << " " << i << " " << t
		// 		   << " " << (i-dx+t*_count) << " " << v << " " << h[t]
		// 		   << " " << h[t]*v << " " << s
		// 		   << endl;
	      }
	      ret.decanonize(m*s, _type, i);
	    }
	  
	for (; x<_width; x++)
	  for (unsigned int j=0; j<_count; j++, i++)
	    if(zeropad==false)
	      ret.decanonize(0.0, _type, i);
	    else{
	      _canontype s = 0;
	      for (unsigned int t=0; t<h.size()-(x-xe+1); t++) {
		s += h[t]*canonize(i-dx+t*_count);
	      }
	      ret.decanonize(m*s, _type, i);
	    }
	} // !circular
      } 
      if (i!=vector_size())
	throw msg("convolve_horizontal() failed with i");

      return ret;
      }

    // ------------------------------------------------------------------------

    /** Convolves image vertically with a one-dimensional filter.
	\param  v vertical 1-d filter.
	\param  m optional multiplier factor.
	\param  zeropad ; if true, calculates the convolution also
                on borders of the img w/ assumption that outside the 
		image area the values are zero
	\param circular; if true calculates circular convolution 	

	\return filtered image.
    */
    imagedata convolve_vertical(const vector<float>& v,
				double m = 1.0,bool zeropad=false,
				bool circular=false) 
      const throw(string) {

      if(zeropad && circular)
	throw std::string("imagedata::convolve_vertical(): zeropad && circular");

      if (v.empty())
	return m==1.0 ? *this : multiply(m);

      imagedata ret(*this);
	  
      const unsigned int yb = (v.size()-1)/2, ye = _height-v.size()/2;
      const unsigned int dy = yb*_count*_width; 

      unsigned int i=0, y=0;

      if(circular){
	for (; y<_height; y++)
	  for (unsigned int x=0; x<_width; x++)
	    for (unsigned int j=0; j<_count; j++, i++) {
	      _canontype s = 0;
	      for (unsigned int t=0; t<v.size(); t++) {
		s += v[t]*canonize(i+((y-yb+t+_height)%_height-y)*_count*_width);
	      }
	      ret.decanonize(m*s, _type, i);
	    }
      } else{
	
	for (; y<yb; y++)
	  for (unsigned int x=0; x<_width; x++)
	    for (unsigned int j=0; j<_count; j++, i++)
	      if(zeropad==false)
		ret.decanonize(0.0, _type, i);
	      else{
		_canontype s = 0;
		for (unsigned int t=yb-y; t<v.size(); t++) {
		  s += v[t]*canonize(i-dy+t*_count*_width);
		}
		ret.decanonize(m*s, _type, i);
	      }	   
	
	
	for (; y<ye; y++)
	  for (unsigned int x=0; x<_width; x++)
	    for (unsigned int j=0; j<_count; j++, i++) {
	      _canontype s = 0;
	      for (unsigned int t=0; t<v.size(); t++) {
		s += v[t]*canonize(i-dy+t*_count*_width);
		// 	      _canontype w = canonize(i-dy+t*_count*_width);
		// 	      cout << y << " " << x << " " << j << " " << i << " " << t
		// 		   << " " << (i-dy+t*_count*_width) << " " << w << " " << h[t]
		// 		   << " " << v[t]*w << " " << s
		// 		   << endl;
	      }
	      ret.decanonize(m*s, _type, i);
	    }
	
	for (; y<_height; y++)
	  for (unsigned int x=0; x<_width; x++)
	    for (unsigned int j=0; j<_count; j++, i++)
	      if(zeropad==false)
		ret.decanonize(0.0, _type, i);
	      else{
		_canontype s = 0;
		for (unsigned int t=0; t<v.size()-(y-ye+1); t++) {
		  s += v[t]*canonize(i-dy+t*_count*_width);
		}
		ret.decanonize(m*s, _type, i);
	      }
      } // !circular
      if (i!=vector_size())
	throw msg("convolve_vertical() failed with i");
      
      return ret;
    }

    // ------------------------------------------------------------------------

    /** Calculates vector form gradient given two one-dimensional masks.
	\param  h 1-dimensional filter, horizontal for first argument.
	\param  v 1-dimensional filter, vertical for first argument.
	\param  m optional multiplier.
	\throw  std::string if something goes wrong.
	\return vector image with _type==pixeldata_scmplx.
    */
    imagedata vector_gradient(const vector<float>& h, const vector<float>& v,
			      double m = 1.0) const throw(string) {
      imagedata x = convolve(h, v, m);
      imagedata y = convolve(v, h, m);

      return make_cmplx(x, y);
    }

    // ------------------------------------------------------------------------

    /** Calculates absolute sum gradient given two one-dimensional masks.
	\param  h 1-dimensional filter, horizontal for first argument.
	\param  v 1-dimensional filter, vertical for first argument.
	\param  m optional multiplier.
	\throw  std::string if something goes wrong.
	\return magnitude image with _type==pixeldata_float.
    */
    imagedata absolute_gradient(const vector<float>& h, const vector<float>& v,
				double m = 1.0) const throw(string) {
      imagedata x = convolve(h, v, m);
      imagedata y = convolve(v, h, m);

      imagedata ret(width(), height(), count(), pixeldata_float);
      const int n = ret.vector_size();
      for (int i=0; i<n; i++) {
	_canontype vx = fabs(x.canonize(i));
	_canontype vy = fabs(y.canonize(i));
	ret.decanonize(vx+vy, pixeldata_float, i);
      }

      return ret;
    }

    // ------------------------------------------------------------------------

    /** Calculates Euclidean sum gradient given two one-dimensional masks.
	\param  h 1-dimensional filter, horizontal for first argument.
	\param  v 1-dimensional filter, vertical for first argument.
	\param  m optional multiplier.
	\throw  std::string if something goes wrong.
	\return magnitude image with _type==pixeldata_float.
    */
    imagedata euclidean_gradient(const vector<float>& h,const vector<float>& v,
				 double m = 1.0) const throw(string) {
      imagedata x = convolve(h, v, m);
      imagedata y = convolve(v, h, m);

      imagedata ret(width(), height(), count(), pixeldata_float);
      const int n = ret.vector_size();
      for (int i=0; i<n; i++) {
	_canontype vx = x.canonize(i);
	_canontype vy = y.canonize(i);
	_canontype vv = sqrt(vx*vx+vy*vy);
	ret.decanonize(vv, pixeldata_float, i);
      }

      return ret;
    }

    // ------------------------------------------------------------------------

    /** Low pass filtering with Gaussian
	\param  dev standard deviation of the gaussian
	\param  w   radius of the filtering window (def. 2*dev) 
	\param  m   optional multiplier
	\param  zeropad see convolve()
	\param circular; if true calculates circular convolution 	

	\throw  std::string if something goes wrong.


	\return filtered image
    */

    imagedata gaussian_filter(float dev,int w=-1,float m=1.0,
			      bool zeropad=true, bool circular=false)
      const throw (string) {

      if(zeropad && circular)
	throw std::string("imagedata::gaussianfilter(): zeropad && circular");

      if(w<0) w=(int)(2*dev);
      if(dev==0) return multiply(m);

      if(dev<0) throw string("imagedata::gaussian_filter(): dev < 0");

      vector<float> mask(2*w+1);
      mask[w]=1.0; 
      float sum=1.0;
      float var2=2*dev*dev;

      for(int d=1;d<=w;d++){
	mask[w-d]=mask[w+d]=exp(-d*d/var2);
	sum += 2*mask[w-d];
      }

      return convolve(mask,mask,m/(sum*sum),zeropad,circular);
    }

    // ------------------------------------------------------------------------

    /** Converts the image to polar coordinates.
	\param  rho vector of radii.
	\param  phi vector of angles.
	\param  x0 center of transformation.
	\param  y0 center of transformation.
	\throw  std::string if something goes wrong.
	\return image in polar coordinates (radius,angle).
    */
    imagedata to_polar(const vector<float>& rho, const vector<float>& phi,
		       float x0, float y0)
      const throw(string) {

      imagedata ret(rho.size(), phi.size(), count(), _type);
      for (unsigned int p=0; p<phi.size(); p++) {
	double sv = sin(phi[p]), cv = cos(phi[p]);
	for (unsigned int r=0; r<rho.size(); r++) {
	  int x = (int)floor(x0+rho[r]*cv+0.5);
	  int y = (int)floor(y0+rho[r]*sv+0.5);
	  if (coordinates_ok(x, y)) {
	    _canonvector v = get_canon(x, y);
	    ret.set_canon(r, p, v);
	  }
	}
      }
      return ret;
    }

    // ------------------------------------------------------------------------

    /** Converts the image to polar coordinates.
	\param  lrho number of different radii.
	\param  lphi number of different angles.
	\param  x0 center of transformation.
	\param  y0 center of transformation.
	\throw  std::string if something goes wrong.
	\return image in polar coordinates (radius,angle).
    */
    imagedata to_polar(int lrho, int lphi, float x0, float y0)
      const throw(string) {
      
      vector<float> rho, phi;
      polar_transform_vectors(lrho, lphi, rho, phi);
      return to_polar(rho, phi, x0, y0);
    }
      
    // ------------------------------------------------------------------------

    /** Calculates radius and angle vectors neede in polar transforms.
	\param lrho number of different radii.
	\param lphi number of different angles.
	\param rho vector of radii.
	\param phi vector of angles.
	\post  rho and phi are set to zero-starting increasing values.
    */

    static void polar_transform_vectors(int lrho, int lphi, vector<float>& rho,
					vector<float>& phi) {
      rho = vector<float>(lrho);
      for (unsigned int i=0; i<rho.size(); i++)
	rho[i] = i;

      phi = vector<float>(lphi);
      for (unsigned int i=0; i<phi.size(); i++)
	phi[i] = 2*M_PI*i/phi.size();
    }

    // ------------------------------------------------------------------------

    /** Converts the image from polar to Cartesian coordinates.
	\param  w width of the image.
	\param  h height of the image.
	\param  rho vector of radii.
	\param  phi vector of angles.
	\param  x0 center of transformation.
	\param  y0 center of transformation.
	\throw  std::string if something goes wrong.
	\return image in polar coordinates (radius,angle).
    */
    imagedata from_polar(int w, int h,
			 const vector<float>& rho, const vector<float>& phi,
			 float x0, float y0)
      const throw(string) {

      if (phi.size()!=height() || rho.size()!=width())
	throw "from_polar() data size problem";

      imagedata ret(w, h, count(), _type);
      for (unsigned int p=0; p<phi.size(); p++) {
	double sv = sin(phi[p]), cv = cos(phi[p]);
	for (unsigned int r=0; r<rho.size(); r++) {
	  int x = (int)floor(x0+rho[r]*cv+0.5);
	  int y = (int)floor(y0+rho[r]*sv+0.5);
	  if (ret.coordinates_ok(x, y)) {
	    _canonvector v = get_canon(r, p);
	    ret.set_canon(x, y, v);
	  }
	}
      }
      return ret;
    }

    // ------------------------------------------------------------------------

    /** Splits given integer image with 2N bits to two images with
	N bits each 
        \return images vector of result images, less significant first
    */

    std::vector<imagedata> split_to_2() const {
      switch (_type) {
      case pixeldata_uint16:
	{
	  // we produce two uchar images out of the uint16 image
	  std::vector<imagedata> ret(2,imagedata(width(),height(),count(),
						 pixeldata_uchar));
	  int l=_vec_uint16.size();
	  std::vector<unsigned char> ms(l), ls(l);
	  
	  for(int i=0;i<l;i++){
	    ls[i]=_vec_uint16[i] & 255;
	    ms[i]=_vec_uint16[i] >> 8;
	  }
	  
	  ret[0].set(ls);
	  ret[1].set(ms);
	  
	  return ret;
	}
	break;
      case pixeldata_uint32:
	{
	  // we produce two uint16 images out of the uint32 image
	  std::vector<imagedata> ret(2,imagedata(width(),height(),count(),
						 pixeldata_uint16));
	  int l=_vec_uint32.size();
	  std::vector<uint16_t> ms(l), ls(l);
	  
	  for(int i=0;i<l;i++){
	    ls[i]=_vec_uint32[i] & 65535;
	    ms[i]=_vec_uint32[i] >> 16;
	  }

	  ret[0].set(ls);
	  ret[1].set(ms);
	  
	  return ret;
	}
	break;


      default:
	throw msg("unsupported pixeldata type");
      }
    }


    // ------------------------------------------------------------------------

    /** Splits given integer image with 4N bits to four images with
	N bits each
        \return images vector of result images, least significant first
    */

    std::vector<imagedata> split_to_4() const {
      switch (_type) {
      case pixeldata_uint32:
	{
	  // we produce four uchar images out of the uint32 image
	  std::vector<imagedata> ret(4,imagedata(width(),height(),count(),
						 pixeldata_uchar));
	  int l=_vec_uint32.size();
	  std::vector<unsigned char> mmms(l),mms(l),ms(l), ls(l);

	  for(int i=0;i<l;i++){
	    uint32_t val=_vec_uint32[i];
	    
	    ls[i]=val & 255;
	    val >>= 8; ms[i]=val & 255;
	    val >>= 8; mms[i]=val & 255;
	    mmms[i]=val >> 8;
	  }
	  
	  ret[0].set(ls);
	  ret[1].set(ms);
	  ret[2].set(mms);
	  ret[3].set(mmms);
	  
	  return ret;
	}
	break;


      default:
	throw msg("unsupported pixeldata type");
      }
    }

    // ------------------------------------------------------------------------

    /** Combines K N bit images to one KN bit image. Possible combinations:
	K=2; N=8
	K=2; N=16
	K=4; N=8

        \param  imgs vector of images to be combined, least significant image
	first
    */
    static imagedata combine_bits(const std::vector<imagedata> &imgs){
      if( imgs.size()==2 && 
	  imgs[0].type()==pixeldata_uchar && imgs[1].type()==pixeldata_uchar)
	{
	  // uchar -> uint16
	  imagedata ret(imgs[0].width(),imgs[0].height(),imgs[0].count(),
			pixeldata_uint16);
	  std::vector<unsigned char> v0,v1;
	  v0=imgs[0].get_uchar();
	  v1=imgs[1].get_uchar();
	  
	  unsigned int l=v0.size();
	  if (l!=v1.size())
	    throw msg("Image size mismatch.");

	  std::vector<uint16_t> v(l);
	  for (unsigned int i=0;i<l;i++)
	    v[i]=v0[i]+(v1[i]<<8);
	  
	  ret.set(v);

	  return ret;
	}
      else if(imgs.size()==2 && imgs[0].type()==pixeldata_uint16 &&
	      imgs[1].type()==pixeldata_uint16)
	{
	  // uint16 -> uint32

	  //cout << "combining 2x16 -> 32 bits" << endl;

	  imagedata ret(imgs[0].width(),imgs[0].height(),imgs[0].count(),
			pixeldata_uint32);
	  std::vector<uint16_t> v0,v1;
	  v0=imgs[0].get_uint16();
	  v1=imgs[1].get_uint16();
	  
	  unsigned int l=v0.size();
	  if (l!=v1.size())
	    throw msg("Image size mismatch.");

	  //cout << "got data vectors of size " << l << endl;

	  std::vector<uint32_t> v(l);
	  for (unsigned int i=0;i<l;i++)
	    v[i]=v0[i]+(v1[i]<<16);

	  ret.set(v);

	  return ret;
	}
      else if( imgs.size()==4 && 
	       imgs[0].type()==pixeldata_uchar && 
	       imgs[1].type()==pixeldata_uchar &&
	       imgs[2].type()==pixeldata_uchar &&
	       imgs[3].type()==pixeldata_uchar)
	{
	  // uchar -> uint32
	  imagedata ret(imgs[0].width(),imgs[0].height(),imgs[0].count(),
			pixeldata_uint32);
	  std::vector<unsigned char> v0,v1,v2,v3;
	  v0=imgs[0].get_uchar();
	  v1=imgs[1].get_uchar();
	  v2=imgs[2].get_uchar();
	  v3=imgs[3].get_uchar();
	  
	  unsigned int l=v0.size();
	  if (l!=v1.size() || l!=v2.size() || l!=v3.size())
	    throw msg("Image size mismatch.");

	  std::vector<uint32_t> v(l);
	  for (unsigned int i=0;i<l;i++)
	    v[i]=v0[i]+(v1[i]<<8)+(v2[i]<<16)+(v3[i]<<24);

	  ret.set(v);

	  return ret;
	}
      else 
	throw msg("unsupported image types");
    }

    // ------------------------------------------------------------------------

    /** Converts the image from polar to Cartesian coordinates.
	\param  w width of the image.
	\param  h height of the image.
	\param  lrho number of different radii.
	\param  lphi number of different angles.
	\param  x0 center of transformation.
	\param  y0 center of transformation.
	\throw  std::string if something goes wrong.
	\return image in polar coordinates (radius,angle).
    */
    imagedata from_polar(int w, int h, int lrho, int lphi, float x0, float y0)
      const throw(string) {

      vector<float> rho, phi;
      polar_transform_vectors(lrho, lphi, rho, phi);
      return from_polar(w, h, rho, phi, x0, y0);
    }

    // ------------------------------------------------------------------------

    /** Returns the minimum and maximum values over all pixels.
	\return a pair of minimum,maximum value vectors.
    */
    pair<vector<float>,vector<float> > min_max_float() const {
      // obs! this should be implemented for all types...
      pair<_canonvector,_canonvector> ext = min_max_canon();
      vector<float> min(ext.first.begin(),  ext.first.end());
      vector<float> max(ext.second.begin(), ext.second.end());

      return pair<vector<float>,vector<float> >(min, max);
    }

    // ------------------------------------------------------------------------

    /** Returns the average and standard deviation over all pixels.
	\return a pair of average,standard deviation vectors.
    */
    pair<vector<float>,vector<float> > avg_std_float() const {
      // obs! this should be implemented for all types...
      pair<_canonvector,_canonvector> ext = avg_std_canon();
      vector<float> avg(ext.first.begin(),  ext.first.end());
      vector<float> std(ext.second.begin(), ext.second.end());

      return pair<vector<float>,vector<float> >(avg, std);
    }

    // ------------------------------------------------------------------------

    /** Shifts and scales the pixel values by scalars.
	\param add value to be added.
	\param mul multiplier.
    */
    void add_multiply(float add, float mul) {
      vector<float> addv(_count, add), mulv(_count, mul);
      add_multiply(addv, mulv);
    }

    /** Shifts and scales the pixel values by vectors.
	\param add vector of values to be added.
	\param mul vector of multipliers.
    */
    void add_multiply(const vector<float>& add, const vector<float>& mul) {
      for (unsigned int x=0; x<_width; x++)
	for (unsigned int y=0; y<_height; y++) {
	  _canonvector v = get_canon(x, y);
	  for (unsigned int i=0; i<v.size(); i++)
	    v[i] = (v[i]+add[i])*mul[i];
	  set_canon(x, y, v);
	}
    }

    /** Normalizes (only float type) image between 0,1.
     */
    void normalize_01() {
      if (vector_size()==0)
	return;
      pair<_canonvector,_canonvector> ext = min_max_canon();
      vector<float> add(ext.first.size());
      for (unsigned int i=0; i<add.size(); i++)
	add[i] = -ext.first[i];
      vector<float> mul(ext.second.size());
      for (unsigned int i=0; i<mul.size(); i++)
	mul[i] = 1/(ext.second[i]-ext.first[i]);
      add_multiply(add, mul);
    }
    
    /** Projects data to horizontal axis by adding values vertically.
	\return array of column-wise sums.
    */

    valarray<float> vertical_sum() const throw(string) {
      if (_type!=pixeldata_float)
	throw msg("vertical_sum() not float");
      if (_count!=1)
	throw msg("vertical_sum() not one channel");

      valarray<float> ret(_width);
      for (unsigned x=0; x<_width; x++) {
	float sum = 0;
	for (unsigned y=0; y<_height; y++)
	  sum += get_one_float(x, y);
	ret[x] = sum;
      }
      return ret;
    }

    // ------------------------------------------------------------------------

    static void rgb_to_hsv(float &r, float &g, float &b) {
      // note that hue coordinate represents angle in
      // hsv cone. discontinuity artifact at h=1/0

      float h,s,v;

      float cmax = max(r,max(g,b));
      float cmin = min(r,min(g,b));

      v = cmax;
      if (v == 0.0) {
	s = h = 0.0;   // pixel is black
      }
      else {
	s = (cmax-cmin) / cmax;

	if(s==0) h=0;
	else{
	  float cdelta = cmax - cmin;
	  float rc = (cmax-r) / cdelta;
	  float gc = (cmax-g) / cdelta;
	  float bc = (cmax-b) / cdelta;
	  if (r == cmax)
	    h = bc-gc;
	  else
	    if (g == cmax)
	      h = 2.0 + rc-bc;
	    else
	      h = 4.0 + gc-rc;
	  h /= 6.0;
	  if (h < 0.0)
	    h += 1.0;
	}
      }
  
      r=h; g=s; b=v;
    }

    // ------------------------------------------------------------------------

    static void rgb_to_conical_hsv(float &r, float &g, float &b) {
      float x,y;
      const float pi = 3.141592654;

      rgb_to_hsv(r,g,b);
      // v gives directly the z-coordinate;
      x = g*b*cos(2*pi*r); // x = v*s*cos(2*pi*h);
      y = g*b*sin(2*pi*r); // y = v*s*sin(2*pi*h);

      r=x; g=y;
    }

    // ------------------------------------------------------------------------

    static void rgb_to_cylindrical_hsv(float &r, float &g, float &b) {
      float x,y;
      const float pi = 3.141592654;

      rgb_to_hsv(r,g,b);
      // v gives directly the z-coordinate;
      x = g*cos(2*pi*r); // x = s*cos(2*pi*h);
      y = g*sin(2*pi*r); // y = s*sin(2*pi*h);

      r=x; g=y;
    }

    // ------------------------------------------------------------------------

    static void rgb_to_munsell(float &r, float &g, float &b){
      // using MTM as given in  Miyahara&Yoshida (1988)

      // obs! scaling may be incorrect
      // the last phase is taken to be non-iterative

      float x,y,z;
      x = 1.020*(0.608*r+0.174*g+0.200*b);
      y = 0.299*r+0.587*g+0.144*b;
      z = 0.847*(0.066*g+1.112*b);

      float h1=mtm_nonlinearity(x)-mtm_nonlinearity(y);
      float h2=mtm_nonlinearity(z)-mtm_nonlinearity(y);
      float h3=mtm_nonlinearity(y);

      float m1=h1;
      float m2=0.4*h2;
      float m3=0.23*h3;

      const float A=8.880, B=0.966, C=8.025, D=2.558;

      float invr;
      if(m1||m2)
	invr = 1.0/sqrt(m1*m1+m2*m2);
      else 
	invr=0;

      float s1=(A+B*m1*invr)*m1;
      float s2=(C+D*m2*invr)*m2;

      r=s1;
      g=s2;
      b=m3;
    }

    static float mtm_nonlinearity(const float &a){
      return 11.6*pow((double)a,(double)0.333333)-1.6;
    }

    // ------------------------------------------------------------------------

    static void rgb_to_hueminmax(float &r, float &g, float &b) {
      float h,s;

      float cmax = max(r,max(g,b));
      float cmin = min(r,min(g,b));

      if (cmax == 0.0) {
	s = h = 0.0;   // pixel is black
      }
      else {
	s = (cmax-cmin) / cmax;

	if(s==0) h=0;
	else{
	  float cdelta = cmax - cmin;
	  float rc = (cmax-r) / cdelta;
	  float gc = (cmax-g) / cdelta;
	  float bc = (cmax-b) / cdelta;
	  if (r == cmax)
	    h = bc-gc;
	  else
	    if (g == cmax)
	      h = 2.0 + rc-bc;
	    else
	      h = 4.0 + gc-rc;
	  h /= 6.0;
	  if (h < 0.0)
	    h += 1.0;
	}
      }
  
      r=h; g=cmin; b=cmax;
    }

    // ------------------------------------------------------------------------

    static void rgb_to_huediffsum(float &r, float &g, float &b) {
      float h,s;

      float cmax = max(r,max(g,b));
      float cmin = min(r,min(g,b));

      if (cmax == 0.0) {
	s = h = 0.0;   // pixel is black
      }
      else {
	s = (cmax-cmin) / cmax;

	if(s==0) h=0;
	else{
	  float cdelta = cmax - cmin;
	  float rc = (cmax-r) / cdelta;
	  float gc = (cmax-g) / cdelta;
	  float bc = (cmax-b) / cdelta;
	  if (r == cmax)
	    h = bc-gc;
	  else
	    if (g == cmax)
	      h = 2.0 + rc-bc;
	    else
	      h = 4.0 + gc-rc;
	  h /= 6.0;
	  if (h < 0.0)
	    h += 1.0;
	}
      }
  
      r=h; g=cmax-cmin; b=0.5*(cmax+cmin);
    }

    // ------------------------------------------------------------------------

    // nonlinear quantisation specified in MPEG-7 standard

    static int quantise_huediffsum_32(float /*h*/,float /*d*/,float /*s*/){
      throw string("Not yet implemented."); 
    }

    static int quantise_huediffsum_64(float /*h*/,float /*d*/,float /*s*/){
      throw string("Not yet implemented."); 
    }

    int quantise_huediffsum_128(float /*h*/,float /*d*/,float /*s*/){
      throw string("Not yet implemented."); 
    }

    static int quantise_huediffsum_256(float h,float d,float s) {
      d *= 255;

      if(d<6){
	// subspace=0;

	int si=(int)(32*s);
	if(si==32) si=31;
	return si;

      }else if(d<20){
	// subspace=1;
	int si=(int)(8*s);
	if(si==8) si=7;
	int hi=(int)(4*h);
	if(hi==4) hi=3;

	return 32 + 8*hi + si;

      }
      else if(d<60){
	// subspace=2;
	int si=(int)(4*s);
	if(si==4) si=3;
	int hi=(int)(16*h);
	if(hi==16) hi=15;

	return 64 + 4*hi + si;
      } else if(d<110) {
	// subspace=3;
	  int si=(int)(4*s);
	  if(si==4) si=3;
	  int hi=(int)(16*h);
	  if(hi==16) hi=15;

	  return 128 + 4*hi + si;
      } else {
	// subspace=4;
	int si=(int)(4*s);
	if(si==4) si=3;
	int hi=(int)(16*h);
	if(hi==16) hi=15;

	return 192 + 4*hi + si;
      }
    }

    // ------------------------------------------------------------------------

    static void rgb_to_cielab(float &r, float &g, float &b) {
      float X,Y,Z; 
      // american TV style conversion
      X = 0.608*r+0.174*g+0.200*b;
      Y = 0.299*r+0.587*g+0.144*b;
      Z = 0.066*g+1.112*b;

      float Yn=Y/1.030;
      float Xn=X/ 0.982;
      float Zn=Z/ 1.178;

      float L;

      if(Yn>0.008856) L=116*pow((double)Yn,(double)1.0/3.0)-16;
      else L=903.3*Yn;
      
      r = L; // L*, note scaling 0..100
      
      g=500*(lab_nonlinearity(Xn)-lab_nonlinearity(Yn)); // a*
      b=200*(lab_nonlinearity(Yn)-lab_nonlinearity(Zn)); // b*
    }

    static float lab_nonlinearity(const float &a) {
      return (a>0.008856)? pow((double)a,(double)0.333333) :
	(7.787*a+16.0/116.0); 
    }

    // ------------------------------------------------------------------------

    static void cielab_to_rgb(float &l, float &a, float &b) {
      float yn,xn,zn;

      if(l<903.3*0.008856) yn=l/903.3;
      else yn=pow((l+16)/(double)116.0,(double)3.0);

      xn=inv_lab_nonlinearity(a/500.0+lab_nonlinearity(yn));
      zn=inv_lab_nonlinearity(lab_nonlinearity(yn)-b/200.0);

      float Y=yn*1.030;
      float X=xn* 0.982;
      float Z=zn* 1.178;

      l = 1.9077*X-0.5347*Y-0.2739*Z; // r
      a = -0.9861*X+2.0051*Y-0.0823*Z; // g
      b = 0.0585*X-0.1190*Y+0.9042*Z; // b

      if (l>1.0) l=1.0; 
      if (l<0.0) l=0.0;  
      if (a>1.0) a=1.0; 
      if (a<0.0) a=0.0;  
      if (b>1.0) b=1.0;
      if (b<0.0) b=0.0;  
    }

    static float inv_lab_nonlinearity(const float a){
      if(a*a*a > 0.008856)
	return a*a*a;
      else return (a-16.0/116.0)/7.787;
    }

 // ------------------------------------------------------------------------

    static void rgb_to_cieluv(float &r, float &g, float &b) {
      float X,Y,Z; 
      // american TV style conversion
      X = 0.608*r+0.174*g+0.200*b;
      Y = 0.299*r+0.587*g+0.144*b;
      Z = 0.066*g+1.112*b;

      const float Yw=1.030;
      const float Xw=0.982;
      const float Zw=1.178;

      float Yn=Y/1.030;

      float L;

      if(Yn>0.008856) L=116*pow((double)Yn,(double)1.0/3.0)-16;
      else L=903.3*Yn;
      
      r = L; // L*, note scaling 0..100
      
      float div=X+15*Y+3*Z;
      const float divw=Xw+15*Yw+3*Zw;

      float up=4*X/div;
      float vp=9*Y/div;

      const float upw=4*Xw/divw;
      const float vpw=9*Yw/divw;

      g=13*L*(up-upw);
      b=13*L*(vp-vpw);
    }

    // ------------------------------------------------------------------------

    static void cieluv_to_rgb(float &l, float &u, float &v) {
      float yn,X,Z;

      if(l<903.3*0.008856) yn=l/903.3;
      else yn=pow((l+16)/(double)116.0,(double)3.0);

      const float Yw=1.030;
      const float Xw=0.982;
      const float Zw=1.178;

      const float divw=Xw+15*Yw+3*Zw;

      const float upw=4*Xw/divw;
      const float vpw=9*Yw/divw;

      float Y=yn*Yw;

      float ldiv=1/(13*l);
      float up=u*ldiv+upw;
      float vp=v*ldiv+vpw;

      X=2.25*Y*(up/vp);
      Z=(1.3333/up-1)*X-5*Y;

      l = 1.9077*X-0.5347*Y-0.2739*Z; // r
      u = -0.9861*X+2.0051*Y-0.0823*Z; // g
      v = 0.0585*X-0.1190*Y+0.9042*Z; // b

      if (l>1.0) l=1.0; 
      if (l<0.0) l=0.0;  
      if (u>1.0) u=1.0; 
      if (u<0.0) u=0.0;  
      if (v>1.0) v=1.0; 
      if (v<0.0) v=0.0;  
    }

    // ------------------------------------------------------------------------

    static void rgb_to_yiq(float &r, float &g, float &b) {
      float y,i,q; 
      // NTSC encoding

      y = 0.299*r+0.587*g+0.144*b;
      i = 0.596*r-0.275*g-0.321*b;
      q = 0.212*r-0.528*g+0.311*b;

      r=y; g=i; b=q;
    }

    // ------------------------------------------------------------------------

    static void rgb_to_YCbCr(float &r, float &g, float &b) {
      float y,cb,cr; 
      // used in MPEG-7

      y = 0.299*r+0.587*g+0.144*b;
      cb = -0.169*r-0.331*g+0.500*b;
      cr = 0.500*r-0.419*g-0.081*b;

      r=y; g=cb; b=cr;
    }

    // ------------------------------------------------------------------------

    vector<unsigned char> create_yuv420() {
      convert(imagedata::pixeldata_uchar);
      force_three_channel();

      vector<unsigned char> rgb = get_uchar();

      // reduce if odd sized, this is how Muvis wants it
      size_t width = _width - (_width%2);
      size_t height = _height - (_height%2);

      size_t l = width*height;

      size_t Uind = l, Vind = Uind+(l>>2);

      // l bytes of y, l/4 bytes of u and l/4 of v
      vector<unsigned char> buf(l+(l>>1),0);

      for (size_t j=0; j<height; j++) {
	for (size_t i=0; i<width; i++) {
	  size_t ind = j*width+i;
	  size_t ind2 = (j>>1)*(width>>1) + (i>>1);
	  float y=rgb[ind], u=rgb[ind+1], v=rgb[ind+2];
	  rgb_to_YCbCr(y,u,v);
	  
	  buf[ind] = (unsigned char)round(y);
	  buf[Uind+ind2] += (unsigned char)round(u);
	  buf[Vind+ind2] += (unsigned char)round(v);
	}
      }

      // normalise U and V
      for (size_t i=Uind; i<buf.size(); i++)
	buf[i] = buf[i]>>2;
      
      return buf;
    }

    // ------------------------------------------------------------------------

    void line(int x0, int y0, int x1, int y1, size_t w,
	      const string& c) throw(string) {
      imagecolor<unsigned char> v(c);
      line(x0, y0, x1, y1, w, v);
    }

    void line(int x0, int y0, int x1, int y1, size_t w,
	      const vector<unsigned char>& v) throw(string) {
      float dx = x1-x0, dy = y1-y0;
      float l = dx*dx+dy*dy;
      if (l>0) {
	l = sqrt(l);
        float rx = dy/l, ry = -dx/l;
	for (float p=0; p<l+1; p++) {
	  float r  = p/l;
          float xs = r*x0+(1-r)*x1;
          float ys = r*y0+(1-r)*y1;
          for (float a=-(w-1.0)/2; a<=(w-1.0)/2; a++) {
            int x = int(a*rx+xs);
            int y = int(a*ry+ys);
            /*
            cout << x0 << "," << y0 << "-" << x1 << "," << y1 << " " << w
                 << " l=" << l << " p=" << p
                 << " rx=" << rx << " ry=" << ry
                 << " xs=" << xs << " ys=" << ys
                 << " a=" << a << " a*rx=" << a*rx << " a*ry=" << a*ry
                 << " x=" << x << " y=" << y
                 << endl;
            */
            if (coordinates_ok(x, y))
              set(x, y, v);
          }
	}
      }
    }

    // ------------------------------------------------------------------------

    void polygon(const vector<int>& p, size_t w,
		 const string& c) throw(string) {
      for (size_t a=0; a<p.size()/2; a++) {
	size_t b = (a+1)%(p.size()/2);
	line(p[2*a], p[2*a+1], p[2*b], p[2*b+1], w, c);
      }
    }

    // ------------------------------------------------------------------------

    void circle(int x0, int y0, float r1, float r0,
		const string& c) throw(string) {
      imagecolor<unsigned char> v(c);
      circle(x0, y0, r1, r0, v);
    }

    void circle(int x0, int y0, float r1, float r0,
		const vector<unsigned char>& v) throw(string) {
      for (int x=int(x0-r1); x<=x0+r1; x++)
	for (int y=int(y0-r1); y<=y0+r1; y++) {
	  float d = (x-x0)*(x-x0)+(y-y0)*(y-y0);
	  if (d<=r1*r1 && d>=r0*r0 && coordinates_ok(x, y))
	    set(x, y, v);
	}
    }

    // ------------------------------------------------------------------------

  protected:
    /** A simple utility function to convert integer to string.
	\param  i integer.
	\return \e i stringified.
    */
    static string tos(int i) {
      stringstream ss;
      ss << i;
      return ss.str();
    }

    /// Returns a message appended to the name header "picsom::imagedata::"
    static string msg(const string& s) { return name_head()+s; }

    /** Returns an error message string used for throwing exceptions
	\param f the name of the function
	\param m the message
    */
    static string ensure_msg(const string& f, const string& m) {
      return f+" : "+m;
    }

    /** Throws an exception
	\param f the name of the function
	\param m the message
    */
    static void ensure_do_throw(const string& f, const string& m) {
      throw ensure_msg(f, m);
    }

    /** Ensures that the type is correct, if not throws an exception.
	\param f the function name
	\param t the pixel data type to check for
    */
    void ensure_type(const char *f, pixeldatatype t) const {
      if (_type!=t && t!=pixeldata_undef)
	ensure_do_throw(f, string("type error, is ")+datatypename(_type)+
			" while expecting "+datatypename(t));
    }

    /** Ensures that the given coordinates are ok, if not throws an exception.
	\param f the function name
	\param x,y the coordinates to check
    */
    void ensure_coordinates(const char *f, int x, int y) const {
      if (!coordinates_ok(x, y)) {
	stringstream msg;
	msg << "coordinates (" << x << "," << y << ") not ok in "
	    << _width << "x" << _height << " image";
	ensure_do_throw(f, msg.str());
      }
    }

    /** Ensures that the given channel count of the image equals internal.
	\param f the function name.
	\param c the channel count to check
    */
    void ensure_count(const char *f, unsigned int c) const {
      if (c!=_count) {
	stringstream msg;
	msg << "size==" << c << " and _count==" << _count << " differ";
	ensure_do_throw(f, msg.str());
      }
    }

    /** Ensure that the two given sizes are equal.
	\param f the function name
	\param r,s the two sizes to compare
    */
    static void ensure_sizediff(const char *f, int r, int s) {
      if (r!=s) {
	string msg = "stored and requested sizes differ, ";
	ensure_do_throw(f, msg+tos(r)+"!="+tos(s));
      }
    }

    /** Ensures that the given type, coordinates and channel counts are ok.
	\param f the function name
	\param t the pixel data type to check
	\param x,y the coordinates to check
	\param s the channel count to check 
    */
#ifdef PICSOM_IMAGEDATA_ENSURE_XY
    void ensure_xy(const char *f, pixeldatatype t, int x, int y, int s) const {
      ensure_type(f, t);
      ensure_coordinates(f, x, y);
      if (s!=-1)
	ensure_count(f, s);
    };
#else
    void ensure_xy(const char*, pixeldatatype, int, int, int) const {}
#endif // PICSOM_IMAGEDATA_ENSURE_XY

    /** Ensures that the given pixel data type and vector size is ok.
	\param f the function name
	\param t the pixel data type to check
	\param s the vector size to check
    */
    void ensure_size(const char *f, pixeldatatype t, int s) const {
      ensure_type(f, t);
      ensure_sizediff(f, vector_size(), s);
    };

    /** Returns the current vector size.
	\throw  std::string if not of defined type
	\return the size of the current internal data storage vector
    */
    unsigned int vector_size() const throw(string) {
      switch (_type) {
      case pixeldata_float:   return _vec_float .size();
      case pixeldata_double:  return _vec_double.size();
      case pixeldata_uchar:   return _vec_uchar .size();
      case pixeldata_uint16:  return _vec_uint16.size();
      case pixeldata_uint32:  return _vec_uint32.size();
      case pixeldata_scmplx:  return _vec_scmplx.size();
      case pixeldata_dcmplx:  return _vec_dcmplx.size();
      default:
	throw msg("vector_size() : switch ("+datatypename(_type)+") failed");
      }
    }

    /** Allocates memory for a vector of type t and size n.
	\param t the pixel data type
	\param n the size of the vector
	\throw  std::string if not of defined type
    */
    void alloc_vector(pixeldatatype t, int n) throw(string) {
      switch (t) {
      case pixeldata_float:
	_vec_float = vector<float>(n, 0.0); break;
      case pixeldata_double:
	_vec_double = vector<double>(n, 0.0); break;
      case pixeldata_uchar:
	_vec_uchar = vector<unsigned char>(n, (unsigned char)0); break;
      case pixeldata_uint16:
	_vec_uint16 = vector<uint16_t>(n, (unsigned int)0); break;
      case pixeldata_uint32:
	_vec_uint32 = vector<uint32_t>(n, (uint32_t)0); break;
      case pixeldata_scmplx:
	_vec_scmplx = vector<complex<float> >(n, complex<float>()); break;
      case pixeldata_dcmplx:
	_vec_dcmplx = vector<complex<double> >(n, complex<double>()); break;
      default:
	throw msg("alloc_vector() : switch ("+datatypename(t)+") failed");
      }
    }

    /** Resizes the internal data vector to size n.
	\param n the size of the vector
	\throw  std::string if not of defined type
    */
    void resize_vector(int n) throw(string) {
      switch (_type) {
      case pixeldata_float:
	_vec_float.resize(n); break;
      case pixeldata_double:
	_vec_double.resize(n); break;
      case pixeldata_uchar:
	_vec_uchar.resize(n); break;
      case pixeldata_uint16:	_vec_uint16.resize(n); break;
      case pixeldata_uint32:
	_vec_uint32.resize(n); break;
      case pixeldata_scmplx:
	_vec_scmplx.resize(n); break;
      case pixeldata_dcmplx:
	_vec_dcmplx.resize(n); break;
      default:
	throw msg("resize_vector() : switch ("+datatypename(_type)+") failed");
      }
    }

    /** Clears the internal vector.
	\throw  std::string if not of defined type
    */
    void clear_vector() throw(string) {
      switch (_type) {
      case pixeldata_float:   _vec_float .clear(); break;
      case pixeldata_double:  _vec_double.clear(); break;
      case pixeldata_uchar:   _vec_uchar .clear(); break;
      case pixeldata_uint16:  _vec_uint16.clear(); break;
      case pixeldata_uint32:  _vec_uint32.clear(); break;
      case pixeldata_scmplx:  _vec_scmplx.clear(); break;
      case pixeldata_dcmplx:  _vec_dcmplx.clear(); break;
      default:
	throw msg("clear_vector() : switch ("+datatypename(_type)+") failed");
      }
      _type = pixeldata_undef;
    }
    
    /** Tests if the given vector contains all zeros
	\param   v the vector to be tested
	\return  true if v contains only zeros
    */  
       
    template <class T>
    bool _iszero_vec(const vector<T> &v) const{
      //vector<T>::const_iterator it;
      int s=v.size();
      T z =(T)0; 
      for(int i=0;i<s;i++)
	if(v[i]!=z) return false;
      return true;
    }

  private:
    /** An utility function to return "picsom::imagedata::".
	\return "picsom::imagedata::" 
    */
    static const string& name_head() { 
      static const string head = "picsom::imagedata::";
      return head;
    }
    
    /** Converts the given coordinates to a one-dimensional index using width w.
	\param x,y coordinates
	\param w width
	\return one-dimensional index
    */
    int to_index_w_width(int x, int y, int w) const { return y*w+x; }

    /** Converts the given coordinates to a one-dimensional index.
	\param x,y the coordinates to convert
	\returns a one-dimensional index
    */
    int to_index(int x, int y) const { return to_index_w_width(x, y, _width); }

    /** Converts the given coordinates to an index with pixel count accounted.
	\param x,y the coordinates to convert
	\returns a one-dimensional index
    */
    int to_index_w_count(int x, int y) const {
      return _count*to_index_w_width(x, y, _width);
    }

    /** Converts the given coordinates to an index with pixel count accounted.
	\param x,y,c the coordinates and channel to convert
	\returns a one-dimensional index
    */
    int to_index(int x, int y, int c) const {
      return to_index_w_count(x, y)+c;
    }

    /// A canonical datatype through which some transformations are performed.
    typedef double _canontype;

    /// A canonical datatype vectorized.
    typedef vector<_canontype> _canonvector;

    /** Canonizes (converts to double) the value at given index.
	A shell to hide the fact that \e _canontype is double.
	Calls \ref canonize_double(int).
    */
    _canontype canonize(int i) const { return canonize_double(i); }

    /** Converts the value at given index to double.
	\param  i index to vector<>. Validity not checked.
	\throw  std::string if datatype is invalid.
	\return Value converted to a double in [0,1]. 
    */
    double canonize_double(int i) const throw(string) {
      const double dc = 17.0*15, ds = dc*(dc+2), dl = ds*(ds+2);
      switch (_type) {
      case pixeldata_float:   return _vec_float[i];
      case pixeldata_double:  return _vec_double[i];
      case pixeldata_uchar:   return _vec_uchar[i]/dc;
      case pixeldata_uint16:  return _vec_uint16[i]/ds;
      case pixeldata_uint32:  return _vec_uint32[i]/dl;
      case pixeldata_scmplx:  return _vec_scmplx[i].real();
      case pixeldata_dcmplx:  return _vec_dcmplx[i].real();

      default:
	throw msg("canonize_double() : switch ("+datatypename(_type)+
		  ") failed");
      }
    }

    /** Decanonizes (converts from double) the value to the given index.
	A shell to hide the fact that \e _canontype is double.
	Calls \ref decanonize_double(_canontype, pixeldatatype, int).
    */
    void decanonize(_canontype v, pixeldatatype t, int i) {
      decanonize_double(v, t, i);
    }

    /** Converts a double and stores it in the given index.
	\param  v canonized value to be stored.
	\param  t datatype to convert to.  Specifies which vector<> is changed.
	\param  i index to vector<>. Validity not checked.
	\throw  std::string if datatype is invalid.
    */
    void decanonize_double(double v, pixeldatatype t, int i) throw(string) {
      const double mc = 17.0*15, ms = mc*(mc+2), ml = ms*(ms+2);
      switch (t) {
      case pixeldata_float:  _vec_float[i] = v; break;
      case pixeldata_double: _vec_double[i]= v; break;
      case pixeldata_uchar:  _vec_uchar[i] = (unsigned char)(v*mc+0.5); break;
      case pixeldata_uint16: _vec_uint16[i]=(uint16_t)(v*ms+0.5); break;
      case pixeldata_uint32: _vec_uint32[i]= (uint32_t)(v*ml+0.5); break;
      case pixeldata_scmplx: _vec_scmplx[i]= v; break;
      case pixeldata_dcmplx: _vec_dcmplx[i]= v; break;

      default:
	throw msg("decanonize_double() : switch ("+datatypename(t)+") failed");
      }
    }

    /** Converts uint16 and stores it in the given index.
	\param  v uint16 value to be stored.
	\param  t datatype to convert to.  Specifies which vector<> is changed.
	\param  i index to vector<>. Validity not checked.
	\throw  std::string if datatype is invalid.
    */
    void decanonize_uint16(uint16_t v, pixeldatatype t, int i) throw(string) {
      // const double mc = 17.0*15, ms = mc*(mc+2);
      // const double one_per_ms = 1.0/ms;

      switch (t) {
      case pixeldata_float:  _vec_float[i] = v/65535.0; break;
      case pixeldata_double: _vec_double[i]= v/65535.0; break;
      case pixeldata_uchar:  _vec_uchar[i] = v >> 8; break; //
	//(unsigned char)(v*mc+0.5); break;
      case pixeldata_uint16: _vec_uint16[i]= v; break;
	//(uint16_t)(v*ms+0.5); break;
      case pixeldata_uint32: _vec_uint32[i]= v << 16; break; 
	//			       (uint32_t)(v*ml+0.5); break;

      case pixeldata_scmplx: _vec_scmplx[i]= v/65535.0; break;
      case pixeldata_dcmplx: _vec_dcmplx[i]= v/65535.0; break;

      default:
	throw msg("decanonize_uint16() : switch ("+datatypename(t)+") failed");
      }
    }

    /** Converts uchar and stores it in the given index.
	\param  v uchar value to be stored.
	\param  t datatype to convert to.  Specifies which vector<> is changed.
	\param  i index to vector<>. Validity not checked.
	\throw  std::string if datatype is invalid.
    */

    void decanonize_uchar(unsigned char v, pixeldatatype t,
			  int i) throw(string) {
      switch (t) {
      case pixeldata_float:  _vec_float[i]  = v/255.0; break;
      case pixeldata_double: _vec_double[i] = v/255.0; break;
      case pixeldata_uchar:  _vec_uchar[i]  = v; break; //
      case pixeldata_uint16: _vec_uint16[i] = v << 8; break;
      case pixeldata_uint32: _vec_uint32[i] = v << 24; break; 
      case pixeldata_scmplx: _vec_scmplx[i] = v/255.0; break;
      case pixeldata_dcmplx: _vec_dcmplx[i] = v/255.0; break;

      default:
	throw msg("decanonize_uchar() : switch ("+datatypename(t)+") failed");
      }
    }

    /** Gets canonical vector at point (x,y).
     */
    _canonvector get_canon(int x, int y) const {
      _canonvector ret(_count);
      int p = to_index_w_count(x, y);
      for (unsigned int i=0; i<_count; i++, p++)
	ret[i] = canonize(p);
      return ret;
    }

    /** Sets canonical vector at point (x,y).
     */
    void set_canon(int x, int y, const _canonvector& v) {
      int p = to_index_w_count(x, y);
      for (unsigned int i=0; i<_count; i++, p++)
	decanonize(v[i], _type, p);
    }

#ifdef __alpha
#pragma message save
#pragma message disable notusetmpfunprm
#endif // __alpha

    /** documentation missing ??
     */
    template <typename T> 
    void _rescale(const scalinginfo& si, size_t interp) throw(string) {
      /// call this only when you are sure that type T matches _type
      /// and interp is valid
      
      switch (interp) {
      case 0:
	_rescale_0<T>(si);
	break;

      case 1:
	_rescale_1<T>(si);
	break;

      default:
	throw msg("rescale() : interpolation not implemeted");
      }
    }

    /** documentation missing ??
     */
    template <typename T> 
    void _rescale_0(const scalinginfo& si) throw(string) {
      /// call this only when you are sure that type T matches _type
      bool debug = false;

      int w = si.dst_width(), h = si.dst_height();
      int newsize = _count*si.dst_size();
      vector<T> newvt(newsize);
      
      for (int y=0, ij=0; y<h; y++) {
	int yii = si.src_y(y), yi = yii;
	if (si.stretch()) {
	  if (yi<0)
	    yi = 0;
	  if (yi>=(int)_height)
	    yi = _height-1;
	}
	for (int x=0; x<w; x++, ij++) {
	  int xii = si.src_x(x), xi = xii;
	  if (si.stretch()) {
	    if (xi<0)
	      xi = 0;
	    if (xi>=(int)_width)
	      xi = _width-1;
	  }
	  if (debug)
	    cout << x << "," << y << " <- " << xii << "," << yii
		 << " <- " << xi << "," << yi << endl;
	  if (!coordinates_ok(xi, yi)) {
	    stringstream ss;
	    ss << "(" << xi << "," << yi << ") <- (" << x << "," << y << ")";
	    throw msg("_rescale_0() : !coordinates_ok"+ss.str()+" in "+
		      info()+ " with "+si.info());
	  }
	  T *v = (T*)get_data_ptr(xi, yi);
	  int b = _count*to_index_w_width(x, y, w);
	  for (size_t ii=0; ii<_count; ii++)
	    newvt[b++] = *(v++);
	}
      }
      set_data(&newvt);
      _width = w;
      _height = h;
    }

    /** documentation missing ??
     */
    template <typename T> 
    void _rescale_1(const scalinginfo& si) throw(string) {
      /// call this only when you are sure that type T matches _type
      
      bool debug = false;

      int w = si.dst_width(), h = si.dst_height();
      int newsize = _count*si.dst_size();
      vector<T> newvt(newsize);

      vector<scalinginfo::range_t> xr(w);
      
      for (int y=0, ij=0; y<h; y++) {
	scalinginfo::range_t yrange = si.src_y_range(y);

	for (int x=0; x<w; x++, ij++) {
	  if (y==0)
	    xr[x] = si.src_x_range(x);
	  const scalinginfo::range_t& xrange = xr[x];
	
	  vector<double> pixel(_count);
	  double mulsum = 0.0;

	  for (size_t yp=0; yp<yrange.size(); yp++) {
	    int yi = yrange[yp].first;
	    for (size_t xp=0; xp<xrange.size(); xp++) {
	      int xi = xrange[xp].first;

	      if (debug)
		cout << "x=" << x << " y=" << y
		     << " w=" << xrange.size() << " h=" << yrange.size()
		     << " xp=" << xp << " yp=" << yp
		     << " xi=" << xi << " yi=" << yi
		     << " xm=" << xrange[xp].second
		     << " ym=" << yrange[yp].second << endl;

	      if (coordinates_ok(xi, yi)) {
		T *v = (T*)get_data_ptr(xi, yi);
		double mul = yrange[yp].second*xrange[xp].second;
		mulsum += mul;
		for (size_t i=0; i<_count; i++)
		  pixel[i] += *(v++)*mul;
	      }
	      // else
	      //   throw msg("_rescale_1() : !coordinates_ok");
	    }
	  }

	  int b = _count*to_index_w_width(x, y, w);
	  for (size_t ii=0; ii<_count; ii++)
	    newvt[b++] = T(pixel[ii]/mulsum);
	}
      }
      set_data(&newvt);
      _width = w;
      _height = h;
    }

    /** Rotate image
    
        bbox (0) crop: Output image includes only the central portion
        around the rotating center and the rotated image and is the 
        same size as input image. 
        
        bbox (1) loose: Output image includes the whole rotated image 
        and is generally larger than the input image.
     */
    template <typename T> 
    void _rotate(const scalinginfo& si, int interp, int bbox) throw(string) {
      /// call this only when you are sure that type T matches _type
      /// and interp & bbox are valid
      
      switch (interp) {
      case 0:
	_rotate_0<T>(si, bbox);
	break;

      default:
	throw msg("rotate() : interpolation not implemeted");
      }
    }

    /** Rotate image using nearest interpolation    
        
        Pixels in areas outside the original image are set to zero.
     */
    template <typename T> 
    void _rotate_0(const scalinginfo& si, int bbox) throw(string) {
      /// call this only when you are sure that type T matches _type
      /// and bbox is valid

      float xs0, ys0;
      si.get_rotate_center(xs0, ys0);
      int ws = width();
      int hs = height();
      /*
      cout << "xs0=" << xs0 << " "
	   << "ys0=" << ys0 << " "
           << "ws=" << ws << " "
           << "hs=" << hs << endl;
      */
      int wd, hd;
      float xd0, yd0;
      float xdt[4], ydt[4];
      float xmin, ymin, xmax, ymax;
      switch (bbox) {      
      case 0:
        wd = width();
        hd = height();
        xd0 = xs0;
        yd0 = ys0;
        break;
      case 1:
        si.rotate_dst_xy((float)0-xs0,    (float)0-ys0,    xdt[0], ydt[0]);
        si.rotate_dst_xy((float)0-xs0,    (float)hs-1-ys0, xdt[1], ydt[1]);
        si.rotate_dst_xy((float)ws-1-xs0, (float)0-ys0,    xdt[2], ydt[2]);
        si.rotate_dst_xy((float)ws-1-xs0, (float)hs-1-ys0, xdt[3], ydt[3]);
        /*
        cout << "xdt=(" << xdt[0] << "," << xdt[1] << "," << xdt[2] << "," << xdt[3] << ")" << endl;
        cout << "ydt=(" << ydt[0] << "," << ydt[1] << "," << ydt[2] << "," << ydt[3] << ")" << endl;
        */
        xmin = min(min(xdt[0], xdt[1]), min(xdt[2], xdt[3]));
        ymin = min(min(ydt[0], ydt[1]), min(ydt[2], ydt[3]));
        xmax = max(max(xdt[0], xdt[1]), max(xdt[2], xdt[3]));
        ymax = max(max(ydt[0], ydt[1]), max(ydt[2], ydt[3]));
	/*
	cout << "xmin=" << xmin << " "
	     << "ymin=" << ymin << " "
	     << "xmax=" << xmax << " "
	     << "ymax=" << ymax << endl;
	*/
        wd = (int)floor(xmax - xmin + 1 + 0.5);
        hd = (int)floor(ymax - ymin + 1 + 0.5);
        xd0 = -xmin;
        yd0 = -ymin;
        break;
      default:
        throw msg("rotate() : bbox not implemeted");
      }
      int newsize = _count * wd * hd;
      vector<T> newvt(newsize);
      /*
      cout << "wd=" << wd << " " << "hd=" << hd << " " << "xd0=" << xd0 << " " << "yd0=" << yd0 << endl;
      cout << "*****************************************" << endl;
      */
      for (int y=0; y<hd; y++) {
      	float yd = y - yd0;      	
      	for (int x=0; x<wd; x++) {
      	  float xd = x - xd0;
      	  float xs, ys;
      	  si.rotate_src_xy(xs, ys, xd, yd);
      	  int xi = (int)floor(xs+xs0+0.5);
      	  int yi = (int)floor(ys+ys0+0.5);
	  /*  
      	  cout << "x=" << x << " "
      	       << "y=" << y << " "
      	       << "xd=" << xd << " "
      	       << "yd=" << yd << " "
      	       << "xs=" << xs << " "
      	       << "ys=" << ys << " "
      	       << "xi=" << xi << " "
      	       << "yi=" << yi << endl;
	  */     
   	  int b = _count*to_index_w_width(x, y, wd);
      	  if (xi<0 || yi<0 || xi>=ws || yi>=hs) {
	    for (size_t i=0; i<_count; i++)
	      newvt[b++] = 0;      	    
      	  }
      	  else {
      	    T *v = (T*)get_data_ptr(xi, yi);
	    for (size_t i=0; i<_count; i++)
	      newvt[b++] = *(v++);
      	  }      	 
      	}
      }
      set_data(&newvt);
      _width = wd;
      _height = hd;      
    }
    

#ifdef __alpha
#pragma message restore
#endif // __alpha

    /** documentation missing ??
     */
    void *get_data_ptr(int x, int y) {
      int offs = to_index_w_count(x, y);

      switch (_type) {
      case pixeldata_float:
	return (void*)&_vec_float[offs];
  	
      case pixeldata_double:  
	return (void*)&_vec_double[offs];
	
      case pixeldata_uchar:   
	return (void*)&_vec_uchar[offs];
	
      case pixeldata_uint16:  
	return (void*)&_vec_uint16[offs];
	
      case pixeldata_uint32:  
	return (void*)&_vec_uint32[offs];
	
      default:
	throw msg("get_data_ptr() : switch ("+datatypename(_type)+
		  ") failed");
      } 
    } 

    /** documentation missing ??
     */
    void set_data(void *v){
      // assumes that v points to a vector of data 
      // compatible with current _type

      switch (_type) {
      case pixeldata_float:
	_vec_float=*(vector<float>*)v;
  	break;
      case pixeldata_double:  
	_vec_double=*(vector<double>*)v;
	break;
      case pixeldata_uchar:   
	_vec_uchar=*(vector<unsigned char>*)v;
	break;
      case pixeldata_uint16:  
	_vec_uint16=*(vector<uint16_t>*)v;
	break;
      case pixeldata_uint32:  
	_vec_uint32=*(vector<uint32_t>*)v;
	break;
      default:
	throw msg("set_data() : switch ("+datatypename(_type)+
		  ") failed");
      } 
    }

    /** Returns the minimum and maximum values over all pixels.
	\return a pair of minimum,maximum value canonical vectors.
    */
    pair<_canonvector,_canonvector> min_max_canon() const {
      _canonvector min = get_canon(0, 0), max = min;

      for (unsigned int x=0; x<_width; x++)
	for (unsigned int y=0; y<_height; y++) {
	  const _canonvector v = get_canon(x, y);
	  for (unsigned int i=0; i<v.size(); i++)
	    if (v[i]<min[i])
	      min[i] = v[i];
	    else if (v[i]>max[i])
	      max[i] = v[i];
	}

      return pair<_canonvector,_canonvector>(min, max);
    }

    /** Returns the average and standard deviation over all pixels.
	\return a pair of average,standard deviation canonical vectors.
    */
    pair<_canonvector,_canonvector> avg_std_canon() const {
      _canonvector sum1 = _canonvector(_count), sum2 = sum1;

      for (unsigned int x=0; x<_width; x++)
	for (unsigned int y=0; y<_height; y++) {
	  const _canonvector v = get_canon(x, y);
	  for (unsigned int i=0; i<_count; i++) {
	    sum1[i] += v[i];
	    sum2[i] += v[i]*v[i];
	  }
	}

      _canonvector avg(_count), std(_count);

      _canontype n = _width*_height, div = n ? n-1 : 1;
      for (unsigned int i=0; i<_count; i++) {
	avg[i] = sum1[i]/n;
	_canontype var = (sum2[i]-sum1[i]*avg[i])/div;
	std[i] = var>=0 && n>1 ? sqrt(var) : 0;
      }

      return pair<_canonvector,_canonvector>(avg, std);
    }

    ///////////////////////////////////////////////////////////////////////////

    /// Datatype of the image.
    pixeldatatype _type;

    /// Channel count of the image.
    size_t _count;

    /// Width of the image in pixels.
    size_t _width;

    /// Height of the image in pixels.
    size_t _height;

    /// Container for float data.
    vector<float> _vec_float;

    /// Container for double data.
    vector<double> _vec_double;

    /// Container for unsigned char data.
    vector<unsigned char> _vec_uchar;

    /// Container for unsigned short data.
    vector<uint16_t> _vec_uint16;
    
    /// Container for unsigned long data.
    vector<uint32_t> _vec_uint32;

    /// Container for complex<float>.
    vector<complex<float> > _vec_scmplx;

    /// Container for complex<double>.
    vector<complex<double> > _vec_dcmplx;

    /// Used by pixeldatatype().
    // static map<pixeldatatype,string> pixeldatatype_map;

  }; // class imagedata

#if defined(sgi) || defined(__alpha)
#pragma do_not_instantiate float imagedata::get_one<float>(size_t,size_t) const
#pragma do_not_instantiate double imagedata::get_one<double>(size_t,size_t) const
#pragma do_not_instantiate unsigned char imagedata::get_one<unsigned char>(size_t,size_t) const
#pragma do_not_instantiate uint16_t imagedata::get_one<uint16_t>(size_t,size_t) const
#pragma do_not_instantiate uint32_t imagedata::get_one<uint32_t>(size_t,size_t) const
#pragma do_not_instantiate complex<float> imagedata::get_one<complex<float> >(size_t,size_t) const
#pragma do_not_instantiate complex<double> imagedata::get_one<complex<double> >(size_t,size_t) const
#endif // sgi || __alpha

} // namespace picsom

#endif // !_imagedata_h_

