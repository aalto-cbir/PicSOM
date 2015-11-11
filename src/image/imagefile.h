// -*- C++ -*-  $Id: imagefile.h,v 1.51 2012/11/05 16:01:40 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

/*!\file imagefile.h

  \brief Declarations and definitions of class picsom::imagefile.

  imagefile.h contains declarations and definitions of
  class picsom::imagefile which is used as bridge between
  class picsom::imagedata and some third-party implementation
  of physical image format libraries.

  \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
  $Revision: 1.51 $
  $Date: 2012/11/05 16:01:40 $
  \bug May be some out there hiding.
  \warning Be warned against all odds!
  \todo So many things, so little time...
*/

#ifndef _imagefile_h_
#define _imagefile_h_

#include <imagedata.h>

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <cerrno>

#include <stdlib.h>

#ifdef HAVE_IO_H
#include <io.h>
#endif // HAVE_IO_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

extern "C" typedef void (*CFP_atexit)(); 
	
namespace picsom {
  using namespace std;

  /** Class imagefile is a wrapper around image file format libraries.
      All things that are specific to any file format library implementation
      used, are only defined here and implemented in files such as
      imagefile-magick.C.  The interface towards the implementing methods
      contains only functions of the form static T impl_*(...) or
      T impl_*(...) const.
  */
  class imagefile {
  public:
    /// Returns version of imagefile class ie. version of imagefile.h.
    static const string& version() {
      static string v =
	"$Id: imagefile.h,v 1.51 2012/11/05 16:01:40 jorma Exp $";
      return v;
    }

    /// Returns version of imagefile implementation.
    static const string& impl_version();

    /// Returns version of underlying image format library.
    static const string& impl_library_version();

    /// jl
    typedef pair<string,string> version_entry;

    /// jl
    typedef vector<version_entry> version_array;

    /// Returns versions of underlying image format libraries.
    static const version_array& impl_used_library_versions();

    /// Default constructor needed by The Standard C++ Library.
    imagefile() : _writing(false), _dispose(false),
		  _default_pixel(imagedata::pixeldata_float),
		  _nframes(0), _curr_frame(-1), _use_cacheing(true) {
      create_impl();
    }

    /// Constructor that opens for reading with given format.
    imagefile(const string& n, const string& f = "") :
      _dispose(false), _default_pixel(imagedata::pixeldata_float), 
      _use_cacheing(true) {
      open(n, false, f);
    }

    /// Constructor that opens for reading with given pixeldatatype and format.
    imagefile(const string& n, imagedata::pixeldatatype t,
	      const string& f = "") : _dispose(false), _default_pixel(t),
				      _use_cacheing(true) {
      open(n, false, f);
    }

    /// Constructor that opens for reading/writing with given format.
    imagefile(const string& n, bool wr, const string& f = "") :
      _dispose(false), _default_pixel(imagedata::pixeldata_float), 
      _use_cacheing(true) {
      open(n, wr, f);
    }

    /// Poisoned because of _impl_data.
    imagefile(const imagefile&) { abort(); }
    
    /// Destructor.  Eg. Disposal of a temporal file.
    ~imagefile() {
      if (_dispose)
	add_disposed(_filename);
      destruct_impl();
    }

    /// Poisoned because of _impl_data.
    imagefile& operator=(const imagefile&) { abort(); }
    
    /** Dumps some basic information about the object.
	\return string of information.
    */
    string info() const {
      stringstream s;
      s << "imagefile <" << _filename << "> " << _format
        << " " << _nframes << " frame" << (_nframes==1?"":"s")
        << ", current=" << _curr_frame
        << " \"" << _description << "\" @" << (void*)this;
      return s.str();
    }

    //@{
    /// Three methods for accessing the filename.
    const string& filename() const { return _filename; }
    string& filename()             { return _filename; }
    void filename(const string& n) { _filename = n; }
    //@}

    //@{
    /// Three methods for accessing the format.
    const string& format() const { return _format; }
    string& format()             { return _format; }
    void format(const string& f) { _format = f; }
    //@}

    //@{
    /// Three methods for accessing the description.
    const string& description() const { return _description; }
    string& description()             { return _description; }
    void description(const string& d) { _description = d; }
    //@}

    //@{
    /// Three methods for accessing the default pixel type.
    imagedata::pixeldatatype  default_pixel() const { return _default_pixel; }
    imagedata::pixeldatatype& default_pixel() 	    { return _default_pixel; }
    void default_pixel(imagedata::pixeldatatype p)  { _default_pixel = p; }
    //@}

    /// Read-only access to _writing.
    bool writing() const { return _writing; }

    /// Returns true if associated with a filename that contains frames.
    bool is_open() const { return !_filename.empty() && _nframes; }

    /// Read-only access to _nframes.
    int nframes() const { return _nframes; }

    /// The samplerate of the audio track for videos (0 for images)
    int sound_samplerate() const { return sound_samplerate_impl(); }

    /// The fps value for videos (0 for images)
    double video_fps() const { return video_fps_impl(); }

    /// Returns true if a frame with given index exists.
    bool frame_index_ok(int f) const { return f>=0 && f<_nframes; }

    /// enable/disable frame cacheing
    void frame_cacheing(bool c) { _use_cacheing = c; }

    /// secret access to the hidden data
    void *impl_data() const { return _impl_data; }

    /// Opens an image file. 
    /// \param n  name of file.
    /// \param wr true if writing, false if reading.
    /// \param f  format if default not preferred.
    void open(const string& n, bool wr = false,
	      const string& f = "") {
      create_impl();
      _filename    = n;
      _writing     = wr;
      _format      = f;
      _nframes     = 0;
      _curr_frame  = -1;
      _description = "";

      try {
	if (wr)
	  open_write_impl(_filename, _format);
	else {
	  _nframes = open_read_impl(_filename, _format, _description);
	  if (_format.empty())
	    throw errtxt("open(", n, ", ", tos(wr), ", ", f,
			 ") failed: format not set");
	}
      }
      catch (const string& e) {
	_nframes = 0;
	throw errtxt("open(", n, ", ", tos(wr), ", ", f, ") failed: " + e);
      }
    }

    /** Writes an image file
       \param fname name of file
       \param fmt format if default not preferred
     */
    void write(const string& fname = "",
	       const string& fmt = "") const throw(string) {
      if (!_nframes)
	throw errtxt("write(", fname, ", ", fmt, ") failed: _nframes==0");

      const string& name = fname.empty() ? _filename : fname;
      const string& format = fmt.empty() ? _format : fmt;
      try {
	write_impl(name, format);
      }
      catch (const string& e) {
	throw errtxt("write(", fname, ", ", fmt, ") failed: " + e);
      }
    }

    /** Writes image data to a file
	\param d image data to write
	\param fn name of file
	\param fmt format if default not preferred
    */
    static void write(const imagedata& d, const string& fn,
                      const string& fmt = "") throw(string) {
      imagefile file(fn, true, fmt);
      file.add_frame(d);
      file.write();
    }

    /** Class containing display settings such as minimum and maximum
	with and height, aspect-ratio etc...
    */
    class displaysettings {
    public:
      /// Constructor. Sets the default value to all internal parameters
      displaysettings() :
	smooth(true),
	colorize(false),
	fork(false),
	keep(false),
	resize(true),
	aspect(true),
	min_width(400),
	min_height(400),
	max_width(900),
	max_height(900)
      { }

      /// File format if using a temporary file
      string format;

      /// Title to write on the image
      string title;
      
      /// Should the image be smoothed when scaled?
      bool smooth;

      /// jl
      bool colorize;

      /// Should we fork a background process to open the file?
      bool fork;

      /// Keep the temporary file if set to true.
      bool keep;

      /// Whether to rescale according to min/max_width/height.
      bool resize;

      /// Whether to retain the ascpect ratio if rescaling.
      bool aspect;

      /// Minimum width if rescaling.
      int min_width;

      /// Minimum height if rescaling.
      int min_height;

      /// Maximum width if rescaling.
      int max_width;

      /// Maximum height if rescaling.
      int max_height;
    };


    /** Display an image with default display settings
	\param d the image data to display
    */
    static void display(const imagedata& d) {
      display(d, displaysettings());
    }
      
    /** Display an image with given display settings
	\param d the image data to display
	\param fmt display settings to use
    */	
    static void display(const imagedata& d, const displaysettings& fmt) {
      try {
	display_impl(d, fmt);
      }
      catch (const string& e) {
	throw errtxt("display(-imagedata-, -format-) failed: " + e);
      }
    }

    /** Makes a temporary file and uses supplied external command to
	display it 
	\param d image data to display 
	\param fmt write format 
	\param cmd command line to run to display the image, %s is
	substituted for the temporary file name
    */
    static void display_tmpfile(const imagedata& d, const string& fmt,
				const string& cmd) {
      string tmpfname = make_tmpfile();

      try {
	write(d, tmpfname, fmt);
      }
      catch (const string& e) {
	throw errtxt("display_tmpfile(-imagedata-, " + fmt + ", " + cmd +
		     ") write(" + tmpfname + ", " + fmt + ") failed: " + e);
      }

      string cmdline = cmd;
      for (;;) {
	string::size_type s = cmdline.find("%s");
	if (s==string::npos)
	  break;
	cmdline.replace(s, 2, tmpfname);
      }

      // cout << "[" << cmdline << "]" << endl;

      int r = system(cmdline.c_str());
      if (r)
        cout << "display_tmpfile(-imagedata-, " + fmt + ", " + cmd +
          ") system(" + cmdline + ") failed" << endl;
    }

    /** Converts the image data into a string
	\param fmt format to use
    */
    string stringify(const string& fmt = "") const {
      if (!_nframes)
	throw errtxt("stringify(", fmt, ") failed: _nframes==0");

      const string& format = fmt.empty() ? _format : fmt;
      try {
	return stringify_impl(format);
      }
      catch (const string& e) {
	throw errtxt("stringify(", fmt, ") failed: " + e);
      }
    }

    /** Converts the given image data into a string
	\param d image data
	\param fmt format to use
    */
    static string stringify(const imagedata& d,
			    const string& fmt = "") {
      imagefile file("", true, fmt);
      file.add_frame(d);
      return file.stringify();
    }

    /** Converts the image data into a string using a temporary file
	\param fmt format to use
    */  
    string stringify_tmpfile(const string& fmt = "",
			     const string& ext = "") const {
      string tmpfname = make_tmpfile()+ext;

      try {
	write(tmpfname, fmt);
      }
      catch (const string& e) {
	throw errtxt("stringify_tmpfile(", fmt, ") write(", tmpfname, ", ",
		     fmt, ") failed: " + e);
      }

      ifstream img(tmpfname.c_str());
      if (!img.good())
	throw errtxt("stringify_tmpfile(", fmt, ") : failed to open ",
		     tmpfname);

      string ret;
      size_t bufsize = 1024*1024;
      char *buf = new char[bufsize];

      while (img)
	ret += string(buf, img.read(buf, bufsize).gcount());

      delete buf;

      if (ret.empty())
	throw errtxt("stringify_tmpfile(", fmt, ") : read empty string from ",
		     tmpfname);

      if (!_keep_tmp_files && unlink(tmpfname.c_str()))
	throw errtxt("stringify_tmpfile(", fmt, ") : unlink(", tmpfname,
		     ") failed");
      return ret;
    }

    /** Converts the given string to image file data
	\param data image data
	\param fmt format of data
	\return created image file data
    */
    static imagefile unstringify(const string& data,
				 const string& fmt = "") {
      try {
	return unstringify_impl(data, fmt);
      }
      catch (const string& e) {
	throw errtxt("unstringify(...,", fmt, ") failed: " + e);
      }
    }

    /** Converts the image data from a string using a temporary file
	\param data image file data
	\param fmt format to use
	\return created imagefile
    */  
    static imagefile unstringify_tmpfile(const string& data,
					 const string& fmt = "") {
      string tmpfname = make_tmpfile();

      try {
	ofstream os(tmpfname.c_str());
	os.write(data.c_str(), data.size());
      }
      catch (const string& e) {
	throw errtxt("unstringify_tmpfile(", fmt, ") ofstream::write(",
		     tmpfname, ") failed: " + e);
      }

      try {
	imagefile img(tmpfname, fmt);
	img.dispose(true);
	return img;
      }
      catch (const string& e) {
	throw errtxt("unstringify_tmpfile(", fmt, ") imagefile(",
		     tmpfname, ",", fmt, ") failed: " + e);
      }
    }

    /** Returns a specific frame
	\param f number of the fram to return
	\return image data of the frame
     */
    const imagedata& frame(int f) const {
      if (!frame_index_ok(f))
	throw errtxtfname("frame(", tos(f), ") index out of bounds");

      _frametype::const_iterator i = _frame.find(f);
      if (i==_frame.end())
	throw errtxtfname("frame(", tos(f), ") const could not fetch frame");

      return i->second;
    }

    /** Returns a specific frame
	\param f number of the frame to return
	\param s anticipated stepping of frames in future calls, default
	         zero inhibits the feature
	\return image data of the frame
     */
    imagedata& frame(int f, int s = 0) {
      if (!frame_index_ok(f))
	throw errtxtfname("frame(int) index ", tos(f), " out of bounds");

      _frametype::iterator i = _frame.find(f);
      if (i!=_frame.end())
	return i->second;

      if(!_use_cacheing)
	_frame.clear();

      try {
	imagedata fdat = read_frame_impl(_filename, f, s, _default_pixel);
	_frame.insert(_frametype::value_type(f, fdat));
	return _frame.find(f)->second;
      }
      catch (const string& e) {
	throw errtxtfname("frame(", tos(f), ") failed: " + e);
      }
    }
    /// Returns the image data of the current frame
    const imagedata& current_frame() const { return frame(_curr_frame); }

    /// Returns the image data of the current frame
    imagedata& current_frame() { return frame(_curr_frame); }

    /** Returns the values of one pixel
	\param f the frame
	\param x x coordinate of the pixel
	\param y y coordinate of the pixel
	\return a vector with the pixel values
    */
    vector<float> get_float(int f, int x, int y) {
      return frame(f).get_float(x, y);
    }

    /** Adds a frame 
	\param d image data of the frame
     */
    void add_frame(const imagedata& d) {
      if (!_writing)
	throw errtxt("add_frame() failed: not writing");
      _frame.insert(_frametype::value_type(_nframes++, d));
    }

    /** Removes a frame from memory
	Current frame cannot be removed.
	Removal is not allowed in writing mode.
	\param i index of the frame
    */
    void remove_frame(int i) {
      if (_writing)
	throw errtxt("remove_frame() failed: writing");
      if (i==_curr_frame)
	throw errtxt("remove_frame() failed: current frame cannot be removed");

      _frametype::iterator p =_frame.find(i);
      if (p!=_frame.end()) {
	_frame.erase(p);
	if (debug_impl())
	  cout << "now in imagefile::remove_frame(" << i << ") : success "
	       << endl;
      }
    }

    /// Returns current value of the debugging mode
    static int debug_impl()	   { return _debug_impl; }

    /// Sets the debugging mode
    static void debug_impl(int d) { _debug_impl = d; }

    /// Returns current value of the temporary file preservation mode
    static bool keep_tmp_files() { return _keep_tmp_files; }

    /// Sets the temporary file preservation mode
    static void keep_tmp_files(bool d) { _keep_tmp_files = d; }

  protected:

    /** Converts an integer to a string object
	\param i the integer
	\return a string
     */
    static string tos(int i) {
      stringstream ss;
      ss << i;
      return ss.str();
    }

    /** Creates an error string from the given parameters (max 7)
	\return the resulting error string
    */
    static string errtxt(const string& t, const string& u = "",
			 const string& v = "", const string& w = "",
			 const string& x = "", const string& y = "",
			 const string& z = "") {
      static string head = "picsom::imagefile::";
      return head+t+u+v+w+x+y+z;
    }

    /** Creates an error string from the filename and given parameters (max 7)
	\return the resulting error string
    */
    string errtxtfname(const string& t, const string& u = "",
		       const string& v = "", const string& w = "",
		       const string& x = "", const string& y = "",
		       const string& z = "") const {
      string head = "picsom::imagefile["+_filename+"]::";
      return head+t+u+v+w+x+y+z;
    }

  public:
    /** Creates a temporary file
	\return the name of the temporary file
     */
    static string make_tmpfile() {
      // perhaps this should obey TMPDIR ?
      char tmpfname[] = "/var/tmp/imagefileXXXXXX";
      int fd = mkstemp(tmpfname);
      if (fd<0) {
	stringstream err;
	err << errno << " " << strerror(errno);
	throw errtxt("make_tmpfile() failed with mkstemp(", tmpfname, ") : ",
		     err.str());
      }
      close(fd);
      return tmpfname;
    }

    /** documentation missing
     */
    static imagedata render_text(const string& t, float psize = 20.0) {
      try {
	return render_text_impl(t, psize);
      }
      catch (const string& e) {
	throw errtxt("render_text(", t, ") failed: " + e);
      }
    }

  private:
    /** Creation of implementation-specific data
     */
    bool create_impl();

    /** Destruction of implementation-specific data
     */
    bool destruct_impl();

    /** The low level implementation of the open function for reading a file
	\param fname file name
	\param fmt format
	\parma descr description
	\return jl?
     */
    int open_read_impl(const string&, string&, string&) const;

    /** The low level implementation of the open function for writing to file.
	It just prepares the opening; checks the given format and the file name 
	that they are ok.
	\param fname file name
	\param fmt format
     */
    void open_write_impl(const string&, const string&) const;

    /** The actual low level implementation that reads the data from one
	frame.
	\param f file name
	\param c frame number
	\param s anticipated stepping or zero to inhibit
	\param t pixel data type
	\return the data from specified frame
     */
    imagedata read_frame_impl(const string&, int, int,
			      imagedata::pixeldatatype) const;

    /** The actual low level implementation of the write function
	\param fname file name
	\param fmt format       
     */
    void write_impl(const string&, const string&) const;

    /** The actual low level implementation of the display function
	\param dd image data to display
	\param fmt display settings
    */
    static void display_impl(const imagedata&, const displaysettings&);

    /** The actual low level implementation of the stringify function
	\param fmt file format
    */
    string stringify_impl(const string&) const;

    /** The actual low level implementation of the unstringify function
	\param data image file data format
	\param fmt file format
	\return imagefile created from the data
    */
    static imagefile unstringify_impl(const string&,
				      const string& = "");

    /**  The samplerate of video audio tracks (0 for images)
    */
    int sound_samplerate_impl() const;

    /**  The fps value for videos (0 for images)
    */
    double video_fps_impl() const;

    /** 
    */
    static imagedata render_text_impl(const string&, float);

    /** Sets a flag to indicate whether the file should be disposed after use.
	\param d true if to be disposed
    */
    void dispose(bool d) { _dispose = d; }

    /** documentation missing
     */
    static void add_disposed(const string& n) { do_dispose(false, n); }

    /** documentation missing
     */
    static void dispose_all() { do_dispose(true, ""); }

    /** documentation missing
     */
    static void do_dispose(bool do_unlink, const string& n) {
      static list<string> files;

      if (!do_unlink) {
	if (files.empty())
	  atexit((CFP_atexit) dispose_all);

	files.push_back(n);

	return;
      }

      for (list<string>::const_iterator i=files.begin(); i!=files.end(); i++)
	unlink(i->c_str());

      files.clear();
    }

  private:
    /// debug flag
    static int _debug_impl;

    /// debug flag
    static bool _keep_tmp_files;

    /// file name
    string _filename;

    /// file format
    string _format;

    /// file description
    string _description;

    /// in core data
    string _cache;

    /// true if writing instead of reading
    bool _writing;
    
    /// true if temporary file that need to be removed after use
    bool _dispose;

    /// the default pixel data type
    imagedata::pixeldatatype _default_pixel;
    
    /// number of frames
    int _nframes;

    /// current frame number
    int _curr_frame;

    /// cacheing on/off
    bool _use_cacheing;

    /// mapping from frame number to corresponding image data
    typedef map<int,imagedata> _frametype;

    /// the frames
    _frametype _frame;

  public:
    /// implementation-dependent data
    void *_impl_data;
  };
}

#endif // !_imagefile_h_

