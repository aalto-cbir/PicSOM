#include <CreateThumbnail.h>
#include <cmath>

#include <sys/stat.h>
#include <sys/types.h>

namespace picsom {

  static const string vcid=
    "$Id: CreateThumbnail.C,v 1.12 2009/04/30 12:56:24 vvi Exp $";

  static CreateThumbnail list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  CreateThumbnail::CreateThumbnail() : tn_dir_output(false) {

  }

  /////////////////////////////////////////////////////////////////////////////

  CreateThumbnail::~CreateThumbnail() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *CreateThumbnail::Version() const { 
    return vcid.c_str();
  }

  /////////////////////////////////////////////////////////////////////////////

  void CreateThumbnail::UsageInfo(ostream& os) const { 
    os << "CreateThumbnail :" << endl
       << "  options : " << endl
       << "    -O  use standard output naming pattern tn-100x100/01234567.png"
       << endl;
      // << "Reads rescaling information from segmented image." << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  int CreateThumbnail::ProcessOptions(int argc, char **argv) { 
    int argc_old = argc;

    if (argc>1) {
      string argstr = argv[0];
      if (argstr=="-O") {
	tn_dir_output = true;
	Option option;
	option.option="-O";
	addToSwitches(option);
	argc--;
      }
      else
	throw string("unknown option ")+argstr;
    }

    return argc_old-argc;    
  }

  /////////////////////////////////////////////////////////////////////////////

  bool CreateThumbnail::Process() {
    if (Verbose()>1)
      ShowLinks();

    int minx, miny, maxx, maxy;
    string data;
    int l, t, w, h;
    float theta, xc, yc;

    // Getting data required to make thumbnail.
    SegmentationResultList *resultlist = ReadResultsFromXML("head");
    if (!resultlist || resultlist->size()!=1 ||
	(((*resultlist)[0].type!="rotated-box")&&((*resultlist)[0].type!="box"))) {
      cerr << "Cannot get head data in CreateThumbnail::Process!"
	   << endl;
      return false;
    }

    data = (*resultlist)[0].value;
    if ((*resultlist)[0].type=="rotated-box") {
      if (sscanf(data.c_str(), "%d %d %d %d %f %f %f",
		 &l, &t, &w, &h, &theta, &xc, &yc) != 7) {
      cerr << "Cannot read head rotated box data in CreateThumbnail::Process"
	   << endl;
      delete resultlist;
      return false;
      }
      /*
      cout << "l=" << l << " "
	   << "t=" << t << " "
	   << "w=" << w << " "
	   << "h=" << h << " "
	   << "theta=" << theta << " "
	   << "xc=" << xc << " "
	   << "yc=" << yc << endl;
      */
      imagedata *pImg = getImg()->accessFrame();
      /*
    imagefile::displaysettings stgs;
    stgs.title = "before rotatation";
    imagefile::display(*pImg, stgs);
      */
      scalinginfo si(-theta, xc, yc);
      float luf, tuf;
      si.rotate_dst_xy_c((float)l, (float)t, luf, tuf);
      minx = (int)floor(luf+0.5);
      miny = (int)floor(tuf+0.5);
      maxx = minx + w - 1;
      maxy = miny + h - 1;
      /*
      cout << "minx=" << minx << " "
	   << "miny=" << miny << " "
	   << "maxx=" << maxx << " "
	   << "maxy=" << maxy << endl;
      */
      pImg->rotate(si);
      /*
    stgs.title = "after rotatation";
    imagefile::display(*pImg, stgs);
      */
    }
    else
    if (sscanf(data.c_str(), "%d %d %d %d", &minx, &miny, &maxx, &maxy) != 4) {
      cerr << "Cannot read head bounding box data in CreateThumbnail::Process"
	   << endl;
      delete resultlist;
      return false;
    }

    delete resultlist;

    float scale = 100.0/(maxx-minx+1);

    //     data = (*GetDescriptionFields())["Scale"];
    //     if (sscanf(data.c_str(), "%f", &scale) != 1) {
    //       scale = 100.0/(maxx-minx); // Defaults to 100 pixels wide.
    //     }
    //     data = (*GetDescriptionFields())["Normalizer"];

    // Forming filename for thumbnail.
    // If inputname is picture.tif, thumbname will be picture_t.png.
    const string& inputname = getImg()->getImageFileName();

    const string thumbname = CreateOutputFileName(inputname);

    if (Verbose()>2)
      cout << "CreateThumbnail : minx=" << minx << " miny=" << miny
	   << " maxx=" << maxx << " maxy=" << maxy << " scale=" << scale
	   << " inputname=<" << inputname << "> thumbname=<" << thumbname
	   << ">" << endl;
  
    return WriteThumbnail(thumbname, minx, miny, maxx, maxy, 100, 100, true)
      && ProcessNextMethod();

    
    // return WriteThumbnailOld(thumbname, minx, miny, maxx, maxy, scale,
    //		  true, 256) && ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool CreateThumbnail::WriteThumbnail(const string& filename,
				       const int x0, const int y0,
				       const int x1, const int y1,
				       const int t_width, const int t_height,
				       const bool coloroutput) {

    try {
      imagedata img = *getImg()->accessFrame();
      scalinginfo si(x1-x0+1, y1-y0+1, t_width, t_height, x0, y0);
      img.rescale(si, 1);

      if (!coloroutput)
	img.force_one_channel();

      size_t slash = filename.rfind('/');
      if (slash!=string::npos) {
	string dir = filename.substr(0, slash);
	struct stat sbuf;
	if (stat(dir.c_str(), &sbuf)!=0 || !(sbuf.st_mode&S_IFDIR)) {
	  if (Verbose()>2)
	    cout << "CreateThumbnail : mkdir(" << dir << ")" << endl;
	  int res = mkdir(dir.c_str(), 0775);
	  if (res) {
	    cerr << "Cannot mkdir() thumbnail directory <" << dir << ">"
		 << endl;
	    return false;
	  }
	  if (Verbose()>2)
	    cout << "CreateThumbnail : chmod(" << dir << ")" << endl;
	  res = chmod(dir.c_str(), 0775);
	  if (res) {
	    cerr << "Cannot chmod() thumbnail directory <" << dir << ">"
		 << endl;
	    return false;
	  }
	}
      }

      imagefile::write(img, filename);

      return true;
    }
    catch (const string& errstr) {
      cerr << "CreateThumbnail : catched <" << errstr << ">" << endl;
      return false;
    }
    catch (...) {
      cerr << "CreateThumbnail : catched" << endl;
      return false;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool CreateThumbnail::WriteThumbnailOld(const string& filename,
					  const int x0, const int y0,
					  const int x1, const int y1,
					  const double scale,
					  const bool coloroutput,
					  const float normalizer) {
    int t_width  = int(floor(0.5 + scale * (x1-x0)));
    int t_height = int(floor(0.5 + scale * (y1-y0)));
    unsigned char *thumbdata;
    // ilStatus s;

    // Creating thumbnail.
    size_t datasize;
    thumbdata = Rescale(x0, y0, x1, y1, scale, normalizer, datasize);
    if (!thumbdata) {
      cerr << "Cannot create thumbnail image (Out of memory?).\n";
      return false;
    }

    imagedata img(t_width, t_height, 3, imagedata::pixeldata_uchar);
    vector<unsigned char> thumbdata_vec(thumbdata, thumbdata+datasize);
    img.set(thumbdata_vec);

    if (!coloroutput)
      img.force_one_channel();

    size_t slash = filename.rfind('/');
    if (slash!=string::npos) {
      string dir = filename.substr(0, slash);
      struct stat sbuf;
      if (stat(dir.c_str(), &sbuf)!=0 || !(sbuf.st_mode&S_IFDIR)) {
	if (Verbose()>2)
	  cout << "CreateThumbnail : mkdir(" << dir << ")" << endl;
	int res = mkdir(dir.c_str(), 0775);
	if (res) {
	  cerr << "Cannot create thumbnail directory <" << dir << ">" << endl;
	  return false;
	}
      }
    }

    imagefile::write(img, filename);

    return true;

    /*
    // Saving the created thumbnail.
    ilFileImg thumbfile;
    iflSize *size;
    iflFileConfig cfg;
    ilConfig *thumbconfig;

    if(coloroutput){
      size = new iflSize(t_width, t_height, 3);
      thumbconfig = new ilConfig(iflUChar, iflInterleaved, 3);
      cfg.setColorModel(iflRGB);
    }
    else { // Grayscale output requested, converting data to grayscale.
      size = new iflSize(t_width, t_height, 1);
      thumbconfig = new ilConfig(iflUChar, iflInterleaved, 1);
      cfg.setColorModel(iflLuminance);
      unsigned char *temp = new unsigned char[t_width*t_height];
      if (!temp) {
	cerr << "Out of memory allocating temporary thumbnail data.\n";
	return false;
      }
      for (int x=0; x<t_width; x++)
	for(int y=0; y<t_height; y++) {
	  int ii = t_width*y+x, ix = 3*ii;
	  temp[ii] = (thumbdata[ix] + thumbdata[ix+1] + thumbdata[ix+2])/3;
	}
    
      delete thumbdata;
      thumbdata = temp;
      temp = NULL;
    }

    cfg.setDimensions(*size);
    cfg.setPageDimensions(*size);
    cfg.setDataType(iflUChar);
    cfg.setCompression(iflCompression(0));
    iflFormat *fmt = iflFormat::findByFormatName("PNG"); // Do NOT delete this
    // or IL crashes.
    s = thumbfile.createFile(filename, NULL, &cfg, fmt);
    if (s)
      return ShowStatus(s, "CreateThumbnail");
    s = thumbfile.setTile(0, 0, t_width, t_height, thumbdata, thumbconfig);
    if (s)
      return ShowStatus(s, "CreateThumbnail");
    s = thumbfile.closeFile();
    if (s)
      return ShowStatus(s, "CreateThumbnail");

    delete thumbdata, size, thumbconfig;
    return true;
    */
  }

  /////////////////////////////////////////////////////////////////////////////

  /*
    This function returns the rescaled image as unsigned char. If your
    data is, for example, between 0 and 1, give normalizer the value 256,
    so your data is scaled appropriately. Normalizer's default value is 1.
  */

  unsigned char* CreateThumbnail::Rescale(int x0, int y0, int x1, int y1,
					  double scale, float normalizer,
					  size_t& retsize) {
    int t_width  = (int)floor(0.5 + scale*(x1-x0));
    int t_height = (int)floor(0.5 + scale*(y1-y0));
    retsize = 3*t_width*t_height;
    unsigned char *thumb = new unsigned char[retsize];
    if (!thumb) {
      retsize = 0;
      return NULL;
    }

    double x_incr = (double)(x1-x0)/t_width;
    double y_incr = (double)(y1-y0)/t_height;
    for (int y=0, idx=0; y<t_height; y++)
      for (int x=0; x<t_width; x++) {
	float R = 0.0, G = 0.0, B = 0.0;
	int inx = int(x0 + x*x_incr), iny = int(y0 + y*y_incr);
	GetPixelRGB(inx, iny, R, G, B);

	thumb[idx++] = (unsigned char)(R*normalizer);
	thumb[idx++] = (unsigned char)(G*normalizer);
	thumb[idx++] = (unsigned char)(B*normalizer);

	if (Verbose()>3)
	  cout << "CreateThumbnail::Rescale : y=" << y << " x=" << x
	       << " iny=" << iny << " inx=" << inx
	       << " " << R << "," << G << "," << B
	       << " -> " << (int)thumb[idx-3] << "," << (int)thumb[idx-2]
	       << "," << (int)thumb[idx-1] << endl;
      }

    return thumb;
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

