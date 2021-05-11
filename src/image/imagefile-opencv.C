// -*- C++ -*-  $Id: imagefile-opencv.C,v 1.41 2020/04/27 11:03:58 jormal Exp $
// 
// Copyright 1998-2020 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <Util.h>

#ifdef HAVE_OPENCV2_CORE_CORE_HPP

#include <imagefile.h>
#include <imagedata.h>

#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
#endif // HAVE_TIFFIO_H

#ifdef HAVE_GIF_LIB_H
#include <gif_lib.h>
#endif // HAVE_GIF_LIB_H

#if CV_VERSION_MAJOR>=4
#include <opencv2/imgproc/types_c.h>
#include <opencv2/videoio/legacy/constants_c.h>
#endif

namespace picsom {
  int    imagefile::_debug_impl     = 0;
  bool   imagefile::_keep_tmp_files = false;
  string imagefile::_tmp_dir        = "/var/tmp";

  static bool do_throw = true;
  static const string implhead = "imagefile-opencv";

#ifdef HAVE_GIF_LIB_H
  static bool   gif_convert = false;
  static string gif_convert_extension = "";
#else
  static bool   gif_convert = true;
  static string gif_convert_extension = ".png"; // ".tiff"
#endif // HAVE_GIF_LIB_H

  using namespace std;

  ///--------------------------------------------------------------------------

  const string& imagefile::impl_version() {
    static string v =
      "$Id: imagefile-opencv.C,v 1.41 2020/04/27 11:03:58 jormal Exp $";
    return v;
  }

  ///--------------------------------------------------------------------------

  const string& imagefile::impl_library_version() { 
    static string libvers;
    if (libvers=="") {
      libvers = "OpenCV "+string(CV_VERSION);
    }
    return libvers;
  }

  ///--------------------------------------------------------------------------

  const imagefile::version_array& imagefile::impl_used_library_versions() { 
    static version_array libvers;
    if (libvers.empty()) {
#ifdef TIFF_VERSION
      char tiffinfo[1000];
      sprintf(tiffinfo, "%d", TIFF_VERSION);
      libvers.push_back(version_entry("TIFF_VERSION", tiffinfo));
#endif // TIFF_VERSION
    }
    return libvers;
  }

  ///--------------------------------------------------------------------------

  class _opencv_impl_data {
  public:
    ///
    _opencv_impl_data() : is_image(false), is_video(false), use_libtiff(false),
			  next_frame((size_t)-1), fps(0.0),
			  last_successfully_read_frame((size_t)-1), gif(NULL) {}

    ///
    ~_opencv_impl_data() {
 #ifdef HAVE_GIF_LIB_H
      if (gif) {
#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR>=5
	int err = 0;
	DGifCloseFile(gif, &err);
#else
	DGifCloseFile(gif);
#endif // GIFLIB_MAJOR>=5
      }
#endif // HAVE_GIF_LIB_H
    }

    ///
    cv::Mat image;

    ///
    cv::VideoCapture vc;

    ///
    string filename;

    ///
    bool is_image;

    ///
    bool is_video;

    ///
    bool use_libtiff;

    ///
    size_t next_frame;

    ///
    double fps;
    
    ///
    size_t last_successfully_read_frame;

    ///
    cv::Mat last_successful_frame;
    
 #ifdef HAVE_GIF_LIB_H
    ///
    GifFileType *gif;

    ///
    list<imagedata> frames;
#else
    void *gif;
#endif // HAVE_GIF_LIB_H
  }; // class _opencv_impl_data

  ///--------------------------------------------------------------------------

  static _opencv_impl_data *data(const imagefile *f) {
    return (_opencv_impl_data*)f->_impl_data;
  }

  ///--------------------------------------------------------------------------

  bool imagefile::create_impl() {
    _impl_data = new _opencv_impl_data();
    return true;
  }

  ///--------------------------------------------------------------------------

  bool imagefile::destruct_impl() {
    delete data(this);
    return true;
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

  static string format2dotextension(const string& fmt) {
    if (fmt=="")
      return "";

    string ext = fmt;
    size_t p = ext.find('/');
    if (p!=string::npos)
      ext.erase(0, p+1);
    ext = "."+ext;

    return ext;
  }

  ///--------------------------------------------------------------------------

  int imagefile::open_read_impl(const string& fnamein, string& fmt,
				string& descr) const {
    if (debug_impl())
      cout << "now in " << implhead << "::open_read_impl(" << fnamein << ","
	   << fmt << ")" << endl;

    string msg = "imagefile-opencv::open_read_impl() : ", cmp;

    if (fmt=="") {
      pair<string,string> fmtcmp = libmagic_mime_type(fnamein);
      fmt = fmtcmp.first;
      cmp = fmtcmp.second;

      if (false && fmt=="application/octet-stream")
	fmt = "video/mp2t"; // "image/jpeg"

      if (debug_impl())
	cout << "  libmagic_mime_type(" << fnamein << ") returned fmt=\""
	     << fmt << "\" cmp=\"" << cmp << "\"" << endl;

      if (fmt.find("image/")!=0 && fmt.find("video/")!=0) {
	throw implhead + "::open_read_impl("+fnamein+") type not image but \""
	  +fmt+"\"";
      }
    }

//     if(fmt=="image/tiff"){
//       data(this)->is_tiff=true;
//       data(this)->is_image=data(this)->is_video=false;
//     } else{

    data(this)->use_libtiff = false;
    data(this)->is_image    = fmt.find("image/")==0;
    data(this)->is_video    = fmt.find("video/")==0;

    if (!data(this)->is_image && !data(this)->is_video) {
	cerr << msg << "MIME type \"" << fmt
	     << "\" is not a supported image or video format" << endl;
	return 0;
    }

    string fname = fnamein;

    if (data(this)->is_image) {
      if (cmp=="application/x-gzip" || cmp=="application/gzip") {
	// obs! unsafe things might happen...
	string tmpfile = make_tmpfile();
	CopyFile(fnamein, tmpfile+".gz");
	Unlink(tmpfile);
	string cmd = "gunzip "+tmpfile+".gz";
	if (system(cmd.c_str()))
	  cerr << msg << "system(gunzip ...) failed" << endl;
	fname = tmpfile;
	// obs! this should be unlinked...
      }

      if (fmt=="image/gif") {
	if (gif_convert) {
	  if (debug_impl())
	    cout << "  doing .gif to " << gif_convert_extension << " conversion"
		 << endl;
	  string tmpfile = make_tmpfile();
	  Unlink(tmpfile);
	  string cmd = "convert "+fname+" "+tmpfile+gif_convert_extension;
	  cout << "[" << cmd << "]" << endl;
	  if (system(cmd.c_str()))
	    cerr << msg << "system(convert ...) failed" << endl;
	  fname = tmpfile+gif_convert_extension;
	  // obs! this should be unlinked...
	  if (gif_convert_extension==".tiff")
	    fmt = "image/tiff";
	} else {
	  if (debug_impl())
	    cout << "  using libgif" << endl;
#ifdef HAVE_GIF_LIB_H
#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR>=5
	  int err = 0;
	  data(this)->gif = DGifOpenFileName(fname.c_str(), &err);
#else
	  data(this)->gif = DGifOpenFileName(fname.c_str());
#endif // GIFLIB_MAJOR>=5
 	  if (!data(this)->gif) {
	    cerr << msg << "EGifOpenFileName() failed" << endl;
	    return 0;
	  }
	  DGifSlurp(data(this)->gif);
	  if (debug_impl())
	    cout << "    libgif reported " << data(this)->gif->ImageCount << " frames" << endl;
	  if (data(this)->gif->ImageCount) {
	    SavedImage *i = data(this)->gif->SavedImages;
	    for (int e=0; e<i->ExtensionBlockCount; e++)
	      if (i->ExtensionBlocks[e].Function==GRAPHICS_EXT_FUNC_CODE) {
		unsigned char *ubb = (unsigned char*)i->ExtensionBlocks[e].Bytes;
		int delay_time = ubb[1]+256*ubb[2];
		data(this)->fps = delay_time ? 100.0/delay_time : 0;
	      }
	    if (debug_impl())
	      cout << "    libgif reported " << data(this)->fps << " fps" << endl;
	  }
	  return data(this)->gif->ImageCount;
#else
	  cerr << msg << "gif_convert==false but HAVE_GIF_LIB_H undef" << endl;
	  return 0;
#endif // HAVE_GIF_LIB_H
	}
      }

      if (fmt=="image/tiff") {
	int n = 0;

	TIFF *tif = TIFFOpen(fname.c_str(), "r");
	if (!tif) 
	  return n; // probably not a TIFF file

	char *d = NULL;
	int ret = TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &d);
	
	if (ret==1)
	  descr = string(d);

	// throw implhead + "::open_read_impl(" + fname + ", " + fmt +
	// ") TIFFOpen(" + fname + ") failed";

	int dircount = 0;
	uint16 pagenr = 0, totalpages = 0;

	do {
	  // cout << "directory " << dircount << endl;
	  // TIFFPrintDirectory(tif, stdout, TIFFPRINT_NONE);
	  dircount++;
	  if (TIFFGetField(tif, TIFFTAG_PAGENUMBER, &pagenr, &totalpages)==1) {
	    n = totalpages;
	    break;
	  }
	} while (TIFFReadDirectory(tif));
      
	if (!totalpages)
	  n = dircount;
      
	if (debug_impl())
	  cout << implhead+ "::open_read_impl() counted from tifflib n="
	       << n << endl; 
      
	TIFFClose(tif);
      
	if (n>1) {
	  data(this)->use_libtiff = true;
	  data(this)->filename    = fname;
	  return n;
	}
      }

      // tiff files with exactly one frame fall through
      // and are thus read via opencv in order to
      // enable multiple channels (~ordinary images)

      cv::Mat& ocv_image = data(this)->image;

      try {
	ocv_image = cv::imread(fname);
      } catch (const string& excp) {
	cerr << msg << "OpenCV imread() caught exception [" << excp
	     << "] on " << fname << endl;
	return 0;
      }
      
      if (ocv_image.empty()) {
	cerr << msg << "OpenCV imread() unable to open " << fname << endl;
	return 0;
      }

      if (!ocv_image.data) {
	cerr << msg << "Image data not loaded properly from " << fname
	     << endl;
	return 0;
      }

      data(this)->filename = fname;

      return 1;

    } else {
      cv::VideoCapture& vc = data(this)->vc;
      if (!vc.open(fname)) {
	cerr << msg << "OpenCV VideoCapture::open() failed on "
	     << fname << endl;
	return 0;
      }
      
      size_t nframes = (size_t)vc.get(CV_CAP_PROP_FRAME_COUNT);
      double fps     = (size_t)vc.get(CV_CAP_PROP_FPS);
      if (debug_impl())
	cout << "  opencv reports nframes=" << nframes
	     << " fps=" << fps << endl;
      
      data(this)->filename   = fname;
      data(this)->next_frame = 0;
      data(this)->fps        = fps;
      
      if (debug_impl()) {
	cout << "CV_CAP_PROP_POS_MSEC =      " << vc.get(CV_CAP_PROP_POS_MSEC) << endl;
	cout << "CV_CAP_PROP_POS_FRAMES =    " << vc.get(CV_CAP_PROP_POS_FRAMES) << endl;
	cout << "CV_CAP_PROP_POS_AVI_RATIO = " << vc.get(CV_CAP_PROP_POS_AVI_RATIO) << endl;
	cout << "CV_CAP_PROP_FRAME_WIDTH =   " << vc.get(CV_CAP_PROP_FRAME_WIDTH) << endl;
	cout << "CV_CAP_PROP_FRAME_HEIGHT =  " << vc.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;
	cout << "CV_CAP_PROP_FPS =           " << vc.get(CV_CAP_PROP_FPS) << endl;
	cout << "CV_CAP_PROP_FOURCC =        " << vc.get(CV_CAP_PROP_FOURCC) << endl;
	cout << "CV_CAP_PROP_FRAME_COUNT =   " << vc.get(CV_CAP_PROP_FRAME_COUNT) << endl;
	cout << "CV_CAP_PROP_FORMAT =        " << vc.get(CV_CAP_PROP_FORMAT) << endl;
	cout << "CV_CAP_PROP_MODE =          " << vc.get(CV_CAP_PROP_MODE) << endl;
	cout << "CV_CAP_PROP_CONVERT_RGB =   " << vc.get(CV_CAP_PROP_CONVERT_RGB) << endl;
	//cout << "CV_CAP_PROP_BUFFERSIZE  = "   << vc.get(CV_CAP_PROP_BUFFERSIZE)  << endl;
      }

      return nframes;
    }
  }

  ///--------------------------------------------------------------------------

  void imagefile::open_write_impl(const string& fname,
				  const string& fmt) const {
    if (debug_impl())
      cout << "now in " << implhead << "::open_write_impl(" << fname
	   << ", " << fmt << ")" <<endl;

    data(this)->is_image    = true;
    data(this)->is_video    = false;  // there is now no way to set this to true
    data(this)->use_libtiff = false;
   
    // obs: currently the comment field is written only for
    // multi-frame tiffs

    if (fname.empty())
      return;

    ofstream os(fname.c_str());
    if (!os)
      throw implhead + "::open_write_impl(" + fname + ", " + fmt + ") failed";

    unlink(fname.c_str());
  }

  ///--------------------------------------------------------------------------

  imagedata imagefile::read_frame_impl(const string& fin, int fc, int step, 
				       imagedata::pixeldatatype t) const {
    string msg = "imagefile-opencv::read_frame_impl() : ";

    bool truncate_on_failure = true;

    if (debug_impl())
      cout << "now in " << implhead << "::read_frame_impl(" << fin
	   << ", "  << tos(fc) << ", " << tos(step) << ", "
	   << imagedata::datatypename(t) << ")"
	   << endl;

    if (!data(this)->is_image && !data(this)->is_video) {
      unimplemented("imagefile-opencv::read_frame_impl() "
		    "neither supported image nor video");
       
      return imagedata(0, 0);
    }

    if (data(this)->gif) {
      // https://chromium.googlesource.com/webm/libwebp/+/0.3.0/examples/gif2webp.c
      if (debug_impl())
	cout << "now in " << implhead << "::read_frame_impl() with GIF" << endl;

  #ifdef HAVE_GIF_LIB_H
      if (data(this)->frames.size()>(size_t)fc)
	return *next(data(this)->frames.begin(), fc);
      
      GifFileType *g = data(this)->gif;
      
      ColorMapObject *gcm = g->SColorMap;
      if (debug_impl()) {
	cout << data(this)->filename << " : " << g->SWidth
	     << "x" << g->SHeight << " " << g->SColorResolution
	     << " " << g->SBackGroundColor << " ("
	     << g->Image.Left << " " << g->Image.Top << " "<< g->Image.Width 
	     << " " << g->Image.Height << ")" << endl;
	if (gcm) {
	  cout << "GLOBAL colormap" << endl;
	  if (debug_impl()>1)
	    for (int j=0; j<gcm->ColorCount; j++)
	      cout << j << " : " << (int)gcm->Colors[j].Red << " "
		   << (int)gcm->Colors[j].Green << " " << (int)gcm->Colors[j].Blue 
		   << endl;
	}
      }
      
      imagedata img(g->SWidth, g->SHeight, 3, imagedata::pixeldata_uchar);
      size_t n = img.width()*img.height(), l = img.count();
      unsigned char *vx = (unsigned char*)img.raw(n*l);

      size_t transparent = 100000;
      SavedImage *i = g->SavedImages;
      
      for (int f=0; f<g->ImageCount; f++, i++) {
	if (debug_impl())
	  cout << "STARTING frame " << f << "/" << g->ImageCount 
	       << " (" << i->ImageDesc.Left << " " << i->ImageDesc.Top
	       << " " << i->ImageDesc.Width << " " << i->ImageDesc.Height
	       << " " << i->ImageDesc.Interlace << ")" << endl;
	for (int e=0; e<i->ExtensionBlockCount; e++) {
	  if (debug_impl())
	    cout << "ext " << e << "/" << i->ExtensionBlockCount
		 << " " << i->ExtensionBlocks[e].Function
		 << " " << i->ExtensionBlocks[e].ByteCount;
	
	  char *sbb = (char*)i->ExtensionBlocks[e].Bytes;
	  unsigned char *ubb = (unsigned char*)i->ExtensionBlocks[e].Bytes;
	  if (debug_impl())
	    for (int j=0; j< i->ExtensionBlocks[e].ByteCount; j++)
	      cout << " [" << (int)ubb[j]  << "]";
	
	  if (i->ExtensionBlocks[e].Function==APPLICATION_EXT_FUNC_CODE) {
	    if (debug_impl())
	      cout << " = application " << string(sbb, i->ExtensionBlocks[e].ByteCount)
		   << endl;
	  
	  } else if (i->ExtensionBlocks[e].Function==GRAPHICS_EXT_FUNC_CODE) {
	    size_t delay_time = 0;
	    if (i->ExtensionBlocks[e].ByteCount>3) {
	      transparent = ubb[3];
	      delay_time = ubb[1]+256*ubb[2];
	    }
	    double fps = delay_time ? 100.0/delay_time : 0;
	    if (debug_impl())
	      cout << " = graphics transparent=" << transparent 
		   << " (delay_time=" << delay_time << " fps=" << fps << ")" << endl;
	  }
#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR>=5
	  else if (i->ExtensionBlocks[e].Function==CONTINUE_EXT_FUNC_CODE) {
	    if (debug_impl())
	      cout << " = continue " << endl;
	  } else if (debug_impl())
	    cout << " (something else) " << endl;
#else
	  else if (debug_impl())
	    cout << " (old giflib) " << endl;
#endif // GIFLIB_MAJOR>=5
	}

	ColorMapObject *lcm = i->ImageDesc.ColorMap;
	if (debug_impl() && lcm) {
	  cout << "LOCAL colormap" << endl;
	  if (debug_impl()>1)
	    for (int j=0; j<lcm->ColorCount; j++)
	      cout << j << " : " << (int)lcm->Colors[j].Red << " "
		   << (int)lcm->Colors[j].Green << " " << (int)lcm->Colors[j].Blue 
		   << endl;
	}
	ColorMapObject *c = lcm ? lcm : gcm;

	unsigned char *v = vx;
	size_t nx = n;
	
	imagedata subimg;
	if (i->ImageDesc.Width!=g->SWidth || i->ImageDesc.Height!=g->SHeight) {
	  subimg = img.subimage(i->ImageDesc.Left, i->ImageDesc.Top,
				i->ImageDesc.Left+i->ImageDesc.Width-1,
				i->ImageDesc.Top +i->ImageDesc.Height-1);
	  nx = subimg.width()*subimg.height();
	  v  = (unsigned char*)subimg.raw(nx*3);
	}
	
	int cc = c ? c->ColorCount : 0, bpp = c ? c->BitsPerPixel : 0;
	if (cc==256 && bpp==8) {
	  for (size_t p=0; p<nx; p++) {
	    size_t j = i->RasterBits[p];
	    if (false && p<1000)
	      cout << " " << (int)j;
	    if (false && p==1000)
	      cout << endl;
	    if (j!=transparent) {
	      v[p*l  ] = c->Colors[j].Red;
	      v[p*l+1] = c->Colors[j].Green;
	      v[p*l+2] = c->Colors[j].Blue;
	    }
	  }	
	}

	if (nx!=n) {
	  img.copyAsSubimage(subimg, i->ImageDesc.Left, i->ImageDesc.Top);
	}

	imagedata img2 = img;
	img2.convert(t);    
	data(this)->frames.push_back(img2);
      }

      return *next(data(this)->frames.begin(), fc);
#else
      return imagedata(0, 0);
#endif // HAVE_GIF_LIB_H
    }
      
    if (data(this)->use_libtiff) {
      string f = data(this)->filename;
      if (f=="")
	f = fin;
      
      TIFF *tif = TIFFOpen(f.c_str(), "r");
      
      int frame = 0;
      while (frame<fc) {
	TIFFReadDirectory(tif);
	frame++;
      }

      // cout << "directory " << frame << endl;
      // TIFFPrintDirectory(tif, stdout, TIFFPRINT_NONE);

      uint32 w = 0, h = 0;
      uint16 nrchannels = 0, bps = 0, sformat = 0, interp = 0;

      TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,      &w); // uint32 width;
      TIFFGetField(tif, TIFFTAG_IMAGELENGTH,     &h); // uint32 height;
      TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nrchannels);
      TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE,   &bps);
      TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT,    &sformat);
      TIFFGetField(tif, TIFFTAG_PHOTOMETRIC,     &interp);

      size_t npixels = w*h;

      if (interp==3) {
	if (bps!=8)
	  unimplemented("imagefile-opencv::read_frame_impl(); colormap tiff read w/ bps != 8");

	imagedata img(w, h, 3, imagedata::intdatatype(16));
	// cout << img.info() << endl;
	
	uint16 *red = NULL, *green = NULL, *blue = NULL;
	TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue);

	vector<unsigned char> samples(w*nrchannels);
	for (size_t y=0; y<h; y++) {
	  TIFFReadScanline(tif, &samples[0], y);
	  if (false && y==50) {
	    for (size_t i=0; i<samples.size(); i++)
	      cout << " " << (int)samples[i];
	    cout << endl;
	  }
	  for (size_t x=0; x<w; x++) {
	    // size_t i = samples[x*nrchannels]+256*samples[x*nrchannels+1];
	    size_t i = samples[x*nrchannels];
	    vector<uint16_t> v { red[i], green[i], blue[i] };
	    img.set(x, y, v);
	  }
	}

	img.convert(t);    

	return img;
	
      } else {
	if (bps!=8 && bps!=16)
	  unimplemented("imagefile-opencv::read_frame_impl(); tiff read w/ bps != 8 | 16");

	imagedata::pixeldatatype dtype = imagedata::intdatatype(bps);
	imagedata img(w, h, nrchannels, dtype);
	cout << img.info() << endl;

	size_t dsize = bps/8;
	vector<void*> rasters(nrchannels);
	for (size_t i=0; i<nrchannels; i++)
	  rasters[i] = _TIFFmalloc(npixels*dsize);

	for (size_t y=0; y<h; y++) {
	  for (size_t i=0; i<nrchannels; i++)
	    TIFFReadScanline(tif, rasters[i], y, i);
	
	  for (size_t x=0; x<w; x++) {
	    if (nrchannels==1) {
	      if (bps==8)
		img.set(x, y, ((const unsigned char*)rasters[0])[x]);

	      if (bps==16)
		img.set(x, y, ((const uint16_t*)rasters[0])[x]);

	    } else {
	      if (bps==8) {
		vector<unsigned char> zzz(nrchannels);
		for (size_t i=0; i<nrchannels; i++)
		  zzz[i] = ((const unsigned char*)rasters[i])[x];
		img.set(x, y, zzz);
	      }
	    
	      if (bps==16) {
		vector<uint16_t> zzz(nrchannels);
		for (size_t i=0; i<nrchannels; i++)
		  zzz[i] = ((const uint16_t*)rasters[i])[x];
		img.set(x, y, zzz);
	      }
	    }
	  }
	}
 
	TIFFClose(tif);
	for (size_t i=0; i<nrchannels; i++)
	  _TIFFfree(rasters[i]);

	img.convert(t);    
	
	return(img); 
      }
    }
    
    cv::Mat& ocv_image = data(this)->image;

    if (data(this)->is_image) {
      if (fc) {
	unimplemented("imagefile-opencv::read_frame_impl() with fc!=0");
	return imagedata(0, 0);
      }
    }

    size_t fno1 = 0, fno2 = 0;

    if (data(this)->is_video) {
      bool can_seek = false;

      cv::VideoCapture& vc = data(this)->vc;

      if (can_seek) {
	fno1 = (size_t)vc.get(CV_CAP_PROP_POS_FRAMES);
	vc.set(CV_CAP_PROP_POS_FRAMES, fc);

      } else {
	if (data(this)->next_frame>(size_t)fc) {
	  /// obs! it seems that this doesn't work for all videos!
	  // vc.set(CV_CAP_PROP_POS_FRAMES, 0);
	  vc.open(data(this)->filename);
	  data(this)->next_frame = 0;
	}
	fno1 = data(this)->next_frame;
	while (data(this)->next_frame<(size_t)fc) {
	  cv::Mat tmp;
	  vc >> tmp;

	  if (debug_impl())
	    cout << "now in " << implhead << "::read_frame_impl() "
		 << "next_frame=" << data(this)->next_frame
		 << " fc=" << fc
		 << " tmp=" << tmp.cols <<"x" << tmp.rows
		 << endl;

	  if (tmp.empty()) {
	    if (!_extend_last_frame) {
	      cerr << msg << "imagefile-opencv::read_frame_impl() seek failed <"
		   << filename() << "> frame #" << fc
		   << " next_frame=" << data(this)->next_frame << endl;
	      return imagedata(0, 0);
	    }
	    
	  } else
	    data(this)->last_successful_frame = tmp;

	  data(this)->next_frame++;
	}
	fno2 = data(this)->next_frame;
      }

      vc >> ocv_image;

      if (ocv_image.empty() && _extend_last_frame) {
	if (debug_impl())
	  cout << "using previously stored last frame" << endl;
	ocv_image = data(this)->last_successful_frame;
      }
      
      if (ocv_image.empty()) {
	cerr << msg << "OpenCV VideoCapture::retrieve() failed <"
	     << filename() << "> frame #" << fc << endl;

	if (truncate_on_failure) {
	  size_t nframes_was = _nframes;
	  ((imagefile*)this)->_nframes =
	    data(this)->last_successfully_read_frame+1;
	  cerr << msg << "truncating the number of frames from "
	       << nframes_was << " to " << _nframes << endl;
	}

	return imagedata(0, 0);
      }

      if (data(this)->last_successfully_read_frame==size_t(-1) ||
	  data(this)->last_successfully_read_frame<data(this)->next_frame)
	data(this)->last_successfully_read_frame = data(this)->next_frame;

      data(this)->next_frame++;

      if (can_seek)
	fno2 = (size_t)vc.get(CV_CAP_PROP_POS_FRAMES);
    }

    int cha = ocv_image.channels(), type = ocv_image.type();

    if (debug_impl())
      cout << "   " << ocv_image.cols << "x" << ocv_image.rows << " channels="
	   << cha << " frameno=" << fno1 << "->" << fno2
	   << " type=" << type << " CV_8UC3=" << CV_8UC3 << endl;

    if (cha!=3) {
      unimplemented("imagefile-opencv::read_frame_impl() with channels!=3");
      return imagedata(0, 0);
    }

    if (type!=CV_8UC3) {
      unimplemented("imagefile-opencv::read_frame_impl() with type!=CV_8UC3");
      return imagedata(0, 0);
    }

    cv::Mat rgb;
    cv::cvtColor(ocv_image, rgb, CV_BGR2RGB);

    imagedata img(rgb.cols, rgb.rows, 3, imagedata::pixeldata_uchar);
    unsigned char *d = (unsigned char*)rgb.data;
    img.set(vector<unsigned char>(d, d+3*rgb.cols*rgb.rows));
    img.convert(t);    

    return img;
  }

  ///--------------------------------------------------------------------------

  void imagefile::write_impl(const string& fname, const string& fmt) const {
    string msg = "imagefile-opencv::write_impl() : ";

    if (debug_impl())
      cout << "now in " << implhead << "::write_impl(" << fname
	   << ", " << fmt << ")" << endl;

    if (fmt=="image/tiff" && nframes()>1)
      data(this)->use_libtiff = true;

    if (data(this)->use_libtiff) {
      TIFF *out = TIFFOpen(fname.c_str(), "w");

      for(int framenr=0; framenr<nframes(); framenr++) {
	imagedata img = frame(framenr);

	if (img.type()!=imagedata::pixeldata_uint16)
	  unimplemented("write_impl() writing tiff with type!=uint16");
	
	if (img.count()!=1)
	  unimplemented("write_impl() writing tiff with nr. of channels != 1");

	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, img.width());  
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, img.height()); 
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 16);  
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);

	if (framenr==0) {
	  TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, description().c_str());
	}

	size_t linebytes = img.width()*sizeof(uint16);
	unsigned char *buf = NULL;
	if (TIFFScanlineSize(out)==(int)linebytes)
	  buf = (unsigned char*)_TIFFmalloc(linebytes);
	else
	  buf = (unsigned char*)_TIFFmalloc(TIFFScanlineSize(out));

	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, img.width()*sizeof(uint16)));

	int l = img.width()*img.height();
	const uint16 *iptr=img.raw_uint16(l);

	//Now writing image to the file one strip at a time
	for (uint32 row = 0; row < img.height(); row++){
	  memcpy(buf, iptr+row*img.width(), linebytes);    
	  if (TIFFWriteScanline(out, buf, row, 0) < 0)
	    break;
	}

	if (buf)
	  _TIFFfree(buf);

	if(TIFFWriteDirectory(out)!=1)
	  throw string("Writing TIFF directory failed.");

      }

      TIFFClose(out);
      return;

    } else if (data(this)->is_image) {
      if (nframes()!=1)
	unimplemented("write_impl() with nframes()!=1");

      imagedata img = frame(0);
      if (img.type()!=imagedata::pixeldata_uchar &&
	  img.type()!=imagedata::pixeldata_uint16)
	img.convert(imagedata::pixeldata_uchar); // obs!
      img.force_three_channel(); // obs!

      cv::Mat& ocv_image = data(this)->image;

      ocv_image = cv::Mat::zeros(img.height(), img.width(),
				 img.type()==imagedata::pixeldata_uchar?
				 CV_8UC3:CV_16UC3);
      for (size_t x=0; x<img.width(); x++)
	for (size_t y=0; y<img.height(); y++)
	  if (img.type()==imagedata::pixeldata_uchar) {
	    vector<unsigned char> vin = img.get_uchar(x, y);
	    cv::Vec3b vout(vin[2], vin[1], vin[0]);
	    ocv_image.at<cv::Vec3b>(y, x) = vout;
	  } else {
	    vector<uint16_t> vin = img.get_uint16(x, y);
	    cv::Vec3w vout(vin[2], vin[1], vin[0]);
	    ocv_image.at<cv::Vec3w>(y, x) = vout;
	  }

      try {
	string ext = format2dotextension(fmt), tmp = fname+ext;
	vector<int> params;
	bool ok = cv::imwrite(tmp, ocv_image, params);
	if (!ok)
	  cerr << msg << "OpenCV imwrite() failed with " << tmp << endl;
	else if (fname!=tmp && rename(tmp.c_str(), fname.c_str()))
	  cerr << msg << "rename("+tmp+","+fname+") failed" << endl;

      } catch (const string& excp) {
	cerr << msg << "OpenCV imwrite() caught exception [" << excp
	     << "] on " << fname << endl;
      }
      return;
    } 

    unimplemented("write_impl() with !is_image");
  }

  ///--------------------------------------------------------------------------

  string imagefile::stringify_impl(const string& fmt) const {
    if (debug_impl())
      cout << "now in " << implhead << "::stringify_impl(" << fmt
	   << ")" << endl;
    // this sure is implementable more efficiently...
    string ext = format2dotextension(fmt);
    return stringify_tmpfile(fmt, ext);
  }

  ///--------------------------------------------------------------------------

  imagefile imagefile::unstringify_impl(const string& d,
					const string& fmt) {
    if (debug_impl())
      cout << "now in " << implhead << "::unstringify_impl(" << fmt
	   << ")" << endl;

    return unstringify_tmpfile(d, fmt);
  }

  ///--------------------------------------------------------------------------

  static cv::Mat imagedata2cvmat(const imagedata& d, int t) {
    cv::Mat mat(d.height(), d.width(), t);
    if (t==CV_32FC3) {
      imagedata a = d;
      a.convert(imagedata::pixeldata_float);
      a.force_three_channel();
      for (size_t x=0; x<a.width(); x++)
	for (size_t y=0; y<a.height(); y++) {
	  vector<float> v = a.get_float(x, y);
	  float t = v[0];
	  v[0] = v[2];
	  v[2] = t;
	  a.set(x, y, v);
	}
      vector<float> dv = a.get_float();
      float *p = &mat.at<float>(0);
      memcpy(p, &dv[0], 4*dv.size());
      
    } else {
      cerr << "imagefile-opencv::imagedata2cvmat() : type=" << t
	   << " not implemented" << endl;
    }
    
    return mat;
  }

  ///--------------------------------------------------------------------------

  static void display_impl_int(const imagedata& d, 
			       const imagefile::displaysettings&,
			       char *k) {
    try {
      string wname = "picsom::imagefile";
      cv::namedWindow(wname, 1);
      cv::Mat mat = imagedata2cvmat(d, CV_32FC3);
      cv::imshow(wname, mat);
      char c = (char)cv::waitKey(0);
      if (k)
	*k = c;
      // cout << "Key <" << c << "> hit" << endl;
    } catch (const cv::Exception& e) {
      cerr << "imagefile-opencv::display_impl_int() " << d.info()
	   << " code=<" << e.code << "> err=<" << e.err << "> file=<"
	   << e.file << "> func=<" << e.func << "> line=<" << e.line
	   << "> msg=<" << e.msg << ">" << endl;
    }
  }

  ///--------------------------------------------------------------------------

  static void display_impl_ext(const imagedata& d, 
			       const imagefile::displaysettings&, char*) {
    string fmt = "image/png";
    string cmd = "eog %s";
    imagefile::display_tmpfile(d, fmt, cmd);
  }

  ///--------------------------------------------------------------------------

  void imagefile::display_impl(const imagedata& d, const displaysettings& s,
			       char *k) {
    bool use_int = true;
    if (use_int)
      display_impl_int(d, s, k);
    else
      display_impl_ext(d, s, k);
  }

  ///--------------------------------------------------------------------------

  double imagefile::video_fps_impl() const {
    return data(this)->fps;
  }

  ///--------------------------------------------------------------------------

  imagedata imagefile::render_text_impl(const string& text, float fontScale) {
    fontScale /= 21;
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    int thickness = 1;
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(text, fontFace,
					fontScale, thickness, &baseline);
    if (baseline>0) // ???
      baseline--;
    cv::Point org(0, textSize.height+1);
    cv::Mat mat(textSize.height+baseline+2, textSize.width,
		CV_8UC3, cv::Scalar::all(0));
    cv::putText(mat, text, org, fontFace, fontScale,
		cv::Scalar::all(255), thickness, 8);

    imagedata img(mat.cols, mat.rows, 3, imagedata::pixeldata_uchar);
    unsigned char *d = (unsigned char*)mat.data;
    img.set(vector<unsigned char>(d, d+3*mat.cols*mat.rows));
    img.invert();

    return img;
 }

  ///--------------------------------------------------------------------------

} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP

