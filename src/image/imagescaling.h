// -*- C++ -*-    $Id: imagescaling.h,v 1.10 2012/08/31 07:40:34 jorma Exp $

/*!\file imagescaling.h

  \brief Declarations and definitions of class picsom::scalinginfo.

  imagescaling.h contains declarations and definitions of
  class picsom::scalinginfo which is used to specify parameters
  needed by various methods of class picsom::imagedata when resacling
  images.

  \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
  $Revision: 1.10 $
  $Date: 2012/08/31 07:40:34 $
  \bug The naming as imagescaling.h vs. class picsom::scalinginfo is confusing.
  \warning Be warned against all odds!
  \todo The documentation is missing, not to name the implementation ...
*/

#ifndef _imagescaling_h_
#define _imagescaling_h_

#include <string>

namespace picsom {
  using namespace std;

  //! Class scalinginfo specifies parameters for image scaling.
  //! Class scalinginfo is used to abstract the parameters needed
  //! in specifying various image scaling operation performed by
  //! class picsom::imagedata.  (Which are yet to be implemented...)
  //
  class scalinginfo {
  public:
    //! not yet documented...
    scalinginfo() :
      _s_width(0), _s_height(0), _d_width(0), _d_height(0),
      _s_xoffs(0), _s_yoffs(0),  _d_xoffs(0), _d_yoffs(0),
      _rotate_theta(0), _rotate_center_x(0), _rotate_center_y(0),
      _stretch(false) {}

    //! not yet documented...
    scalinginfo(int a, int b, int c, int d,
		float e = 0, float f = 0, float g = 0, float h = 0) :
      _s_width(a), _s_height(b), _d_width(c), _d_height(d),
      _s_xoffs(e), _s_yoffs(f),  _d_xoffs(g), _d_yoffs(h),
      _rotate_theta(0), _rotate_center_x(0), _rotate_center_y(0),
      _stretch(false) {}
      
    //! not yet documented...
    scalinginfo(float a, float b, float c) :
      _s_width(0), _s_height(0), _d_width(0), _d_height(0),
      _s_xoffs(0), _s_yoffs(0),  _d_xoffs(0), _d_yoffs(0),
      _rotate_theta(a), _rotate_center_x(b), _rotate_center_y(c),
      _stretch(false) {}
      
    //! not yet documented...
    void rotate_dst_xy(float src_x, float src_y,
		       float& dst_x, float& dst_y) const throw(string) {
      dst_x = src_x * cos(_rotate_theta) - src_y * sin(_rotate_theta);
      dst_y = src_x * sin(_rotate_theta) + src_y * cos(_rotate_theta);
    }

    void rotate_dst_xy_c(float src_x, float src_y,
			 float& dst_x, float& dst_y) const throw(string) {
      rotate_dst_xy(src_x-_rotate_center_x, src_y-_rotate_center_y,
		    dst_x, dst_y);
      dst_x += _rotate_center_x;
      dst_y += _rotate_center_y;
    }
    
    void rotate_src_xy(float& src_x, float& src_y,
		       float dst_x, float dst_y) const throw(string) {
      src_x = dst_x * cos(-_rotate_theta) - dst_y * sin(-_rotate_theta);
      src_y = dst_x * sin(-_rotate_theta) + dst_y * cos(-_rotate_theta);
    }
    
    void rotate_src_xy_c(float& src_x, float& src_y,
			 float dst_x, float dst_y) const throw(string) {
      rotate_src_xy(src_x, src_y,
		    dst_x-_rotate_center_x, dst_y-_rotate_center_y);
      src_x += _rotate_center_x;
      src_y += _rotate_center_y;
    }

    void get_rotate_center(float& x, float& y) const throw(string) {
      x = _rotate_center_x;
      y = _rotate_center_y;
    }

    //! not yet documented...
    bool is_set() const {
      return _s_width && _s_height && _d_width && _d_height;
    }

    bool is_rotate_set() const {
      return _rotate_theta && _rotate_center_x && _rotate_center_y;
    }

    //! not yet documented...
    void ensure_is_set(const char *txt) const throw(string) {
      if (!is_set())
	throw string("scalinginfo::")+txt+"() called when !is_set()";
    }

    //! not yet documented...
    int dst_width() const throw(string) {
      ensure_is_set("dst_width");
      return _d_width;
    }

    //! not yet documented...
    int dst_height() const throw(string) {
      ensure_is_set("dst_height");
      return _d_height;
    }

    //! not yet documented...
    int dst_size() const throw(string) {
      ensure_is_set("dst_size");
      return _d_width*_d_height;
    }

    //! not yet documented...
    int src_x(int x) const throw(string) {
      ensure_is_set("src_x");
      return _src_x(x);
    }

    //! not yet documented...
    int src_y(int y) const throw(string) {
      ensure_is_set("src_y");
      return _src_y(y);
    }

    //! not yet documented...
    void forwards(int& x, int& y) const throw(string) {
      ensure_is_set("forwards");
      x = _dst_x(x);
      y = _dst_y(y);
    }

    //! not yet documented...
    void backwards(int& x, int& y) const throw(string) {
      ensure_is_set("backwards");
      x = _src_x(x);
      y = _src_y(y);
    }

    typedef vector<pair<int,float> > range_t;

    //! not yet documented...
    range_t src_x_range(int x) const throw(string) {
      ensure_is_set("src_x_range");
      return _src_x_range(x);
    }

    //! not yet documented...
    range_t src_y_range(int y) const throw(string) {
      ensure_is_set("src_y_range");
      return _src_y_range(y);
    }

    //! not yet documented...
    bool stretch() const { return _stretch; }

    //! not yet documented...
    void stretch(bool s) { _stretch = s; }

    //! not yet documented...
    string info() const {
      stringstream s;
      s << "scalinginfo source: " << _s_width << "x" << _s_height
	<< "+" << _s_xoffs << "+" << _s_yoffs
	<< " dest: " << _d_width << "x" << _d_height
	<< "+" << _d_xoffs << "+" << _d_yoffs
	<< " rot: "  << _rotate_theta
	<< "+" << _rotate_center_x << "+" << _rotate_center_y
	<< " stretch=" << (_stretch?1:0)
	<< " @" << (void*)this;
      return s.str();
    }

  private:
    //! not yet documented...
    int _dst_x(int x) const {
      return int(floor((x-_s_xoffs)*_d_width /_s_width +0.5+_d_xoffs)); }

    //! not yet documented...
    int _dst_y(int y) const {
      return int(floor((y-_s_yoffs)*_d_height/_s_height+0.5+_d_yoffs)); }

    //! not yet documented...
    int _src_x(int x) const {
      return int(floor((x-_d_xoffs)*_s_width /_d_width +0.5+_s_xoffs)); }

    //! not yet documented...
    int _src_y(int y) const {
      return int(floor((y-_d_yoffs)*_s_height/_d_height+0.5+_s_yoffs)); }
      
    //! not yet documented...
    range_t _src_x_range(int x) const {
      double m  = double(_s_width)/_d_width, mr = 1/m;
      double xs = (x-_d_xoffs)*m+_s_xoffs;
      int x0 = (int)floor(xs);
      int x1 = (int)floor(xs+m);
      range_t ret(x1-x0+1);
      for (size_t i=0, j=ret.size()-1; i<=j; i++) {
	double v = i==0 ? (1-xs+x0)*mr : i==j ? (xs+m-x1)*mr : mr;
	ret[i].first  = x0+i;
	ret[i].second = v;
      }
      return ret;
    }

    //! not yet documented...
    range_t _src_y_range(int y) const {
      double m  = double(_s_height)/_d_height, mr = 1/m;
      double ys = (y-_d_yoffs)*m+_s_yoffs;
      int y0 = (int)floor(ys);
      int y1 = (int)floor(ys+m);
      range_t ret(y1-y0+1);
      for (size_t i=0, j=ret.size()-1; i<=j; i++) {
	double v = i==0 ? (1-ys+y0)*mr : i==j ? (ys+m-y1)*mr : mr;
	ret[i].first  = y0+i;
	ret[i].second = v;
      }
      return ret;
    }

    //! not yet documented...
    int _s_width, _s_height, _d_width, _d_height;

    //! not yet documented...
    float _s_xoffs, _s_yoffs;

    //! not yet documented...
    float _d_xoffs, _d_yoffs;
    
    //! not yet documented...
    float _rotate_theta;
    
    //! not yet documented...
    float _rotate_center_x, _rotate_center_y;

    //! not yet documented...
    bool _stretch;
    
  }; // class scalinginfo

} // namespace picsom

#endif // !_imagescaling_h_

