#include <FilterbyColor.h>

#define EPS 0.0000000001

using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::cerr;
using std::endl;
using std::set;
using std::vector;
using std::pair;

namespace picsom {
  static const string vcid =
    "@(#)$Id: FilterbyColor.C,v 1.7 2009/04/30 08:30:23 vvi Exp $";

  static FilterbyColor list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  FilterbyColor::FilterbyColor() {
    used_colorspace = cs_ycbcr;
    threshold = 0.5;
    writemask = false;
    maskprefix = "";

    // Simple skin color detector in YCbCr colorspace. 
    // From Chai & Ngan: Face Segmentation Using Skin-Color Map. IEEE Tran. on 
    // Circuits and Systems for Video Tech. 9(4): 551-564. June 1999. 
    // primitiveValues.push_back(0.0);    primitiveValues.push_back(1.0);
    // primitiveValues.push_back(-0.198); primitiveValues.push_back(-0.002);
    // primitiveValues.push_back(0.022);  primitiveValues.push_back(0.178);

    ParsePrimitive("box(0.0,1.0, -0.198,-0.002, 0.022,0.178)");

    // "box(0.0,1.0, -0.148,-0.052, 0.032,0.168)"
  }

  /////////////////////////////////////////////////////////////////////////////
  
  FilterbyColor::~FilterbyColor() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *FilterbyColor::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void FilterbyColor::UsageInfo(ostream& os) const { 
    os << "FilterbyColor :" << endl
       << "  options : " << endl
       << "  -s space     : used colorspace (currently supported: rgb, ycbcr)" << endl
       << "  -p primitive : used primitive (currently supported: \"box(rb,re,gb,ge,bb,be)\")" << endl
       << "  -t threshold : pixel proportion threshold (0<t<=1)" << endl
       << "  -a           : use average value instead (option not implemented)" << endl
       << "  -r           : reject segmentations inside the primitive, "
                           "accept others (option not implemented)" << endl
       << "  -i           : write original and mask images" << endl
       << "  -I prefix    : write original and mask images with name prefix" << endl;

  }

  /////////////////////////////////////////////////////////////////////////////

  int FilterbyColor::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {

      case 's':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -s");
	break;
      }

      option.option = "-s";
      option.addArgument(argv[1]);

      if (!strcmp(argv[1], "rgb"))
	used_colorspace = cs_rgb;
      else if (!strcmp(argv[1], "ycbcr"))
	used_colorspace = cs_ycbcr;
      else 
	EchoError(string("Unrecognized colorspace: ") + argv[1]);

      argc--;
      argv++;
      break;

      case 'p':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -p");
	break;
      }

      option.option = "-p";
      option.addArgument(argv[1]);
      ParsePrimitive(argv[1]);

      argc--;
      argv++;
      break;

      case 't':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -t");
	break;
      }

      option.option = "-t";
      option.addArgument(argv[1]);

      threshold = atof(argv[1]);
      if (threshold < EPS || threshold > 1.0) 
	EchoError(string("Threshold value out of bounds: ") + argv[1]);

      argc--;
      argv++;
      break;
	
      case 'i':
	option.option = "-i";
	writemask = true;
	break;

      case 'I':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -I");
	break;
      }

      option.option = "-I";
      option.addArgument(argv[1]);

      writemask = true;
      maskprefix = string(argv[1]);

      argc--;
      argv++;
      break;

      default:
	EchoError(string("unknown option ")+argv[0]);
      } // switch

      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
    return argc_old-argc;
    
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool FilterbyColor::Process() {
    SegmentationResultList *result=getImg()->readFrameResultsFromXML("","box");

    if (Verbose()>1) {
      ShowLinks();
      cout << "Found "<<result->size()<<" bounding boxes" << endl;
      cout << "Using colorspace " << ColorSpaceName(used_colorspace) << endl;
    }

    if (result->size()==0) {
      PredefinedTypes::Box bb(0, 0,
			      getImg()->getWidth(), getImg()->getHeight());

      size_t nin, nout;
      ProcessBox(bb, 0, "fullimage", true, nin, nout);
    }

    int nr=0;

    SegmentationResultList::const_iterator it;
    for (it=result->begin(); it!=result->end(); it++,nr++) {
      if (Verbose()>2)
	cout << "Found result name=" << it->name
	     << " type="<< it->type
	     << " value=\""<< it->value << "\""<<endl;

      PredefinedTypes::Box bb(it->value);

      size_t nin, nout;
      ProcessBox(bb, nr, it->name, false, nin, nout);

      double score = ((double)nin / (nin+nout));
      bool decision = (score > threshold);
      if (Verbose()>1)
	cout << "Inside primitive: " << nin << ", outside: " << nout 
	     << " pixels, score: " << score << ", threshold: " << threshold 
	     << ". Decision: " << (decision ? "pass" : "fail") << endl;

      if (decision) {
	SegmentationResult myresult;
	myresult.name  = it->name;
	myresult.type  = it->type;
	myresult.value = it->value;

	if (Verbose()>1)
	  cout << myresult.name << " = " << myresult.type << " [ "
	       << myresult.value << " ]" << endl;
      
	// getImg()->writeFrameResultToXML(this, myresult);

      } else {
	getImg()->invalidateFrameResults(this, it->name, it->type);
      }
    }

    delete result;

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool FilterbyColor::ProcessBox(const PredefinedTypes::Box& bb, int nr,
				 const string& itname, bool set,
				 size_t& nin, size_t& nout) {
    if (Verbose()>1) {
      string s;
      bb.write(s);
      cout << "Processing bounding box " << nr << ": " << s << endl;
    }

    coord ul(bb.ulx,bb.uly), br(bb.brx,bb.bry);

    if(ul.x<0) ul.x=0;
    if(ul.y<0) ul.y=0;
    if(br.x>getImg()->getWidth()-1) br.x=getImg()->getWidth()-1;
    if(br.y>getImg()->getHeight()-1) br.y=getImg()->getHeight()-1;

    float c1, c2, c3; // dimensions of the used colorspace (rgb initially)
    bool ok;

    nin = nout = 0;

    picsom::imagedata faceimg(br.x-ul.x, br.y-ul.y, 3,
			      picsom::imagedata::pixeldata_float);

    picsom::imagedata maskimg(br.x-ul.x, br.y-ul.y, 1,
			      picsom::imagedata::pixeldata_uchar);

    for (int yy=ul.y; yy<br.y; yy++)
      for (int xx=ul.x; xx<br.x; xx++) {

	ok = GetPixelRGB(xx, yy, c1, c2, c3);
	if (!ok) 
	  cerr << "Unable to read pixel (x=" << xx << ",y=" << yy << ")"
	       << endl;  

	if (writemask) {
	  vector<float> rgb;
	  rgb.push_back(c1); rgb.push_back(c2); rgb.push_back(c3);
	  faceimg.set(xx-ul.x,yy-ul.y,rgb);
	}

	if (Verbose()>3)
	  cout << "UL: x=" << xx << " y=" << yy 
	       << " RGB: " << c1 << ", " << c2 << ", " << c3 << endl;

	if (used_colorspace == cs_ycbcr) {
	  imagedata::rgb_to_YCbCr(c1, c2, c3);
	  if (Verbose()>3)
	    cout << "UL: x=" << xx << " y=" << yy 
		 << " YCbCr: " << c1 << ", " << c2 << ", " << c3 << endl;
	}

	bool in = IsInsidePrimitive(c1, c2, c3);
	if (in)
	  nin++;
	else
	  nout++;
	
	if (set)
	  SetPixelSegment(xx, yy, in);

	if (writemask)
	  maskimg.set(xx-ul.x, yy-ul.y, (unsigned char)(in?255:0));
      }

    if (Verbose()>1)
      cout << "pixel counts: in=" << nin << " out=" << nout << endl;

    if (writemask) {
      string orig, mask;
      orig = maskprefix + itname + "-orig.png";
      mask = maskprefix + itname + "-mask.png";
	
      picsom::imagefile::write(faceimg, orig.c_str());
      if (Verbose()>1)
	cout << "Wrote " << orig << endl;

      picsom::imagefile::write(maskimg, mask.c_str());
      if (Verbose()>1)
	cout << "Wrote " << mask << endl;
    }

    return true;
  }
      
  /////////////////////////////////////////////////////////////////////////////

  bool FilterbyColor::IsInsidePrimitive(double c1, double c2, double c3) {

    if (c1<primitiveValues.at(0) || c1>primitiveValues.at(1))
      return false;
    if (c2<primitiveValues.at(2) || c2>primitiveValues.at(3))
      return false;
    if (c3<primitiveValues.at(4) || c3>primitiveValues.at(5))
      return false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void FilterbyColor::ParsePrimitive(const string &p) {

    size_t pbeg = p.find('('), pend = p.find(')');    
    string pritype, pinside;
    bool error = false;
    primitiveValues.clear();

    if (pbeg == string::npos || pend == string::npos) 
      error = true;
    if (!error) {
      pritype = p.substr(0,pbeg);
      if (pritype == "box")
	used_primitive = pr_box;
      else 
	error = true;
    }
    if (!error) 
      pinside = p.substr(pbeg+1,pend-pbeg-1);

//     cout << "ParsePrimitive: pbeg=" << pbeg << ",pend=" << pend 
// 	 << ",pritype=" << pritype << ",pinside=" << pinside << endl;
    
    while (1) {
      size_t c = pinside.find(',');
      if (c == string::npos)
	break;
      primitiveValues.push_back(atof(pinside.substr(0,c).c_str()));
      pinside = pinside.substr(c+1);
    }
    primitiveValues.push_back(atof(pinside.c_str()));

    if (error)
      cerr << "ParsePrimitive: Parsing of [" << p << "] failed" << endl;

//     cout << "primitiveValues:" 
// 	 << " 0=" << primitiveValues[0] << ",1=" << primitiveValues[1] 
// 	 << ",2=" << primitiveValues[2] << ",3=" << primitiveValues[3] 
// 	 << ",4=" << primitiveValues[4] << ",5=" << primitiveValues[5] 
// 	 << endl;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
