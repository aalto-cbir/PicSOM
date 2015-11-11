// -*- C++ -*-  $Id: imagefile-opencv.C,v 1.19 2015/10/09 12:04:18 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@cis.hut.fi>
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

#include <tiffio.h>

namespace picsom {
  static const string implhead = "imagefile-opencv";
  int imagefile::_debug_impl = 0;
  bool imagefile::_keep_tmp_files = false;
  static bool do_throw = true;

  // map<imagedata::pixeldatatype,string> imagedata::pixeldatatype_map;

  using namespace std;

  ///--------------------------------------------------------------------------

  const string& imagefile::impl_version() {
    static string v =
      "$Id: imagefile-opencv.C,v 1.19 2015/10/09 12:04:18 jorma Exp $";
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
			  next_frame((size_t)-1),
			  last_successfully_read_frame((size_t)-1) {}

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
    size_t last_successfully_read_frame;

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

    data(this)->use_libtiff=false;
    data(this)->is_image = fmt.find("image/")==0;
    data(this)->is_video = fmt.find("video/")==0;

    if (!data(this)->is_image && !data(this)->is_video) {
	cerr << msg << "MIME type \"" << fmt
	     << "\" is not a supported image or video format" << endl;
	return 0;
    }

    string fname = fnamein;

    if (data(this)->is_image) {
      if (cmp=="application/x-gzip") {
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

      if (fmt=="image/tiff") {
	int n=0;

	TIFF *tif = TIFFOpen(fname.c_str(),"r");
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
      if (debug_impl())
	cout << "  opencv reports " << nframes << " frames" << endl;
      
      data(this)->filename   = fname;
      data(this)->next_frame = 0;
      
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

    data(this)->is_image = true;
    data(this)->is_video = false;  // there is now no way to set this to true
    data(this)->use_libtiff=false;
   
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

  imagedata imagefile::read_frame_impl(const string& f, int fc, int step, 
				       imagedata::pixeldatatype t) const {
    string msg = "imagefile-opencv::read_frame_impl() : ";

    bool truncate_on_failure = true;

    if (debug_impl())
      cout << "now in " << implhead << "::read_frame_impl(" << f
	   << ", "  << tos(fc) << ", " << tos(step) << ", "
	   << imagedata::datatypename(t) << ")"
	   << endl;

    if (!data(this)->is_image && !data(this)->is_video) {
      unimplemented("imagefile-opencv::read_frame_impl() "
		    "neither supported image nor video");
       
      return imagedata(0, 0);
    }

    if (data(this)->use_libtiff) {
      TIFF *tif=TIFFOpen(f.c_str(), "r");
      
      int frame=0;
      while(frame<fc){
	TIFFReadDirectory(tif);
	frame++;
      }

      // cout << "directory " << frame << endl;
      // TIFFPrintDirectory(tif, stdout, TIFFPRINT_NONE);

      uint32 w,h;

      TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);           // uint32 width;
      TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);        // uint32 height;

      uint32 npixels=w*h;

      imagedata img(w, h, 1, imagedata::pixeldata_uint16);

      uint16 *raster=(uint16 *) _TIFFmalloc(npixels *sizeof(uint16));

      // wastefully allocate room for the whole image
      // TIFFReadRGBAImage(tif, w, h, raster, 0); 

      uint32 nrchannels=1; 
      uint16 bps;
      
      TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nrchannels);
      TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
      
      // cv::Mat vismat(h,w,CV_8UC1,cv::Scalar::all(0));

      if (nrchannels!=1)
	unimplemented("imagefile-opencv::read_frame_impl(); tiff read w/ nrchannels !=1");

      if (bps!=16)
	unimplemented("imagefile-opencv::read_frame_impl(); tiff read w/ bps != 16");

      for(size_t y=0;y<h;y++){
	TIFFReadScanline(tif,raster,y);
	for(size_t x=0;x<w;x++){
	  //	    vismat.at<uchar>(y,x)=(raster[x])?255:0;
	  img.set(x,y,raster[x]);
	}
      }
 
      TIFFClose(tif);
      _TIFFfree(raster);

      // cv::imshow("frame",vismat);
      // cv::waitKey(0);
	
      img.convert(t);    
	
      return(img); 
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
	    cerr << msg << "imagefile-opencv::read_frame_impl() seek failed <"
		 << filename() << "> frame #" << fc
		 << " next_frame=" << data(this)->next_frame << endl;
	    return imagedata(0, 0);
	  }
	  data(this)->next_frame++;
	}
	fno2 = data(this)->next_frame;
      }

      /*
      if (!vc.grab()) {
	cerr << msg << "OpenCV VideoCapture::grab() failed" << endl;
	return imagedata(0, 0);
      }

      if (!vc.retrieve(ocv_image)) {
	cerr << msg << "OpenCV VideoCapture::retrieve() failed" << endl;
	return imagedata(0, 0);
      }
      */

      vc >> ocv_image;

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

 if(fmt=="image/tiff" && nframes()>1)
      data(this)->use_libtiff=true;

    if (data(this)->use_libtiff){


      TIFF *out= TIFFOpen(fname.c_str(), "w");

      for(int framenr=0;framenr<nframes();framenr++){

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

	if(framenr==0){
	  TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, description().c_str());
	}

	size_t linebytes = img.width()*sizeof(uint16);
	unsigned char *buf = NULL;
	if (TIFFScanlineSize(out)==(int)linebytes)
	  buf =(unsigned char *)_TIFFmalloc(linebytes);
	else
	  buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));

	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, img.width()*sizeof(uint16)));

	int l=img.width()*img.height();
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

  void imagefile::display_impl(const imagedata&, const displaysettings&) {}

  ///--------------------------------------------------------------------------

  double imagefile::video_fps_impl() const {
    return 0.0;
  }

  ///--------------------------------------------------------------------------

  imagedata imagefile::render_text_impl(const string& text, float fontScale) {
    fontScale /= 21;
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    int thickness = 1;
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(text, fontFace,
					fontScale, thickness, &baseline);
    cv::Point org(0, textSize.height);
    cv::Mat mat(textSize.height+1, textSize.width, CV_8UC3, cv::Scalar::all(0));
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

