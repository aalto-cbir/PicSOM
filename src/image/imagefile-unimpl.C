// -*- C++ -*-  $Id: imagefile-unimpl.C,v 1.10 2018/03/23 11:37:01 jormal Exp $
// 
// Copyright 1998-2018 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <imagefile.h>
#include <imagedata.h>

#include <iostream>

namespace picsom {
  int imagefile::_debug_impl      = 0;
  bool imagefile::_keep_tmp_files = false;
  string imagefile::_tmp_dir      = "/var/tmp";

  static bool do_throw = true;
  static const string implhead = "imagefile-unimpl";

  // map<imagedata::pixeldatatype,string> imagedata::pixeldatatype_map;

  using namespace std;

  ///--------------------------------------------------------------------------

  const string& imagefile::impl_version() {
    static string v =
      "$Id: imagefile-unimpl.C,v 1.10 2018/03/23 11:37:01 jormal Exp $";
    return v;
  }

  ///--------------------------------------------------------------------------

  const string& imagefile::impl_library_version() { 
    static string v = "none";
    return v;
  }

  ///--------------------------------------------------------------------------

  const imagefile::version_array& imagefile::impl_used_library_versions() { 
    static const version_array arr;
    return arr;
  }

  ///--------------------------------------------------------------------------

  static void unimplemented(const char *f) {
    string msg = implhead + "::"+f+" unimplemented";
    if (do_throw)
      throw msg;
    else
      cerr << msg << endl;
  }

  ///--------------------------------------------------------------------------

  int imagefile::open_read_impl(const string& f, string&, string&) const {
    if (debug_impl())
      cout << "now in " << implhead << "::open_read_impl(" << f << ")" << endl;

    unimplemented("open_read_impl(const string&, string&, string&)");

    return 0;
  }

  ///--------------------------------------------------------------------------

  void imagefile::open_write_impl(const string& f, const string& g) const {
    if (debug_impl())
      cout << "now in " << implhead << "::open_write_impl(" << f
	   << ", " << g << ")" <<endl;

    unimplemented("open_write_impl(const string&, const string&)");
  }

  ///--------------------------------------------------------------------------

  imagedata imagefile::read_frame_impl(const string& f, int c, int, 
				       imagedata::pixeldatatype) const {
    if (debug_impl())
      cout << "now in " << implhead << "::read_frame_impl(" << f
	   << ", " << tos(c) << ")" << endl;

    unimplemented("read_frame_impl(const string&, int, pixeldatatype)");

    return imagedata(0, 0);
  }

  ///--------------------------------------------------------------------------

  void imagefile::write_impl(const string& f, const string& g) const {
    if (debug_impl())
      cout << "now in " << implhead << "::write_impl(" << f
	   << ", " << g << ")" << endl;

    unimplemented("write_impl(const string&, const string&)");
  }

  ///--------------------------------------------------------------------------

  string imagefile::stringify_impl(const string& fmt) const {
    if (debug_impl())
      cout << "now in " << implhead << "::stringify_impl(" << fmt
	   << ")" << endl;
    unimplemented("stringify_impl(const string&)");
    return "";
  }

  ///--------------------------------------------------------------------------

  imagefile imagefile::unstringify_impl(const string&, const string& fmt) {
    if (debug_impl())
      cout << "now in " << implhead << "::unstringify_impl(" << fmt
	   << ")" << endl;

    unimplemented("unstringify_impl(const string&, const string&)");
    return imagefile();
  }

  ///--------------------------------------------------------------------------

  void imagefile::display_impl(const imagedata&, const displaysettings&,
			       char*) {}

  ///--------------------------------------------------------------------------

  bool imagefile::create_impl() { return true; }

  ///--------------------------------------------------------------------------

  bool imagefile::destruct_impl() { return true; }

  ///--------------------------------------------------------------------------

  double imagefile::video_fps_impl() const {
    return 0.0;
  }

  ///--------------------------------------------------------------------------

  imagedata imagefile::render_text_impl(const string&, float) {
    return imagedata(1, 1, 3, imagedata::pixeldata_uchar);
  }

  ///--------------------------------------------------------------------------

} // namespace picsom

