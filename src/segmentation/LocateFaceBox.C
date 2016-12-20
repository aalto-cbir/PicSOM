
#include <LocateFaceBox.h>

#include <cmath>

#include <imagecolor.h>

#include <string.h>
#include <stdio.h>

#define round(x) (int)floor((x)+0.5)
#define WIDTH_BOX_FACIALPARTS 16
#define HEIGHT_BOX_FACIALPARTS WIDTH_BOX_FACIALPARTS
#define PI 3.1415926535897932384626433832795

namespace picsom {

  static const string vcid="$Id: LocateFaceBox.C,v 1.3 2009/04/30 09:54:07 vvi Exp $";

  static LocateFaceBox list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  const char *LocateFaceBox::Version() const { 
    return vcid.c_str();
  }

  /////////////////////////////////////////////////////////////////////////////

  void LocateFaceBox::UsageInfo(ostream& os) const { 
    os << "LocateFaceBox :" << endl
       << "  options : " << endl
       << "    -t <theta>                   Set the threshold for face score (default=1000)." << endl
       << "    -T <tau>                     Set the difference threshold between face score and nonface score (default=0)." << endl
       << "    -s <min,augment,max>         Scaling range specificaiton, default & e.g. 20,1.1,100" << endl
       << "    -r <min,step,max>            Face angle range specification, default & e.g. -20,5,20" << endl
       << "    -f <face model filename>     Specifies the face model file (default=face_model.txt)." << endl
       << "    -n <nonface model filename>  Specifies the nonface model file (default=nonface_model.txt)." << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  int LocateFaceBox::ProcessOptions(int argc, char** argv) {
    int n;
    int argc_old=argc;
  
    while (*argv[0]=='-' && argc>1) {
      switch (argv[0][1]) {


    case 't':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -t");
	break;
      }
      sscanf(argv[1], "%f", &_theta);

      argc--;
      argv++;
      break;
      
    case 'T':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -T");
	break;
      }
      sscanf(argv[1], "%f", &_tau);

      argc--;
      argv++;
      break;

    case 's':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -s");
	break;
      }
      
      n = sscanf(argv[1], "%f,%f,%f", &_scale_min, &_scale_augment, &_scale_max);
      if (n!=3 || _scale_min<WIDTH_BOX_FACIALPARTS || _scale_max<_scale_min || _scale_augment<=1) {
	EchoError("incorrect scale specification");
	break;      	
      }

      argc--;
      argv++;
      break;

    case 'r':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -r");
	break;
      }
      
      float face_angle_min, face_angle_max;
      n = sscanf(argv[1], "%f,%f,%f", &face_angle_min, &_rotate_step, &face_angle_max);
      if (n!=3 || face_angle_max<face_angle_min || _rotate_step<=0) {
	EchoError("incorrect rotation specification");
	break;      	
      }
      /// note the angle to be rotated is negative to the face angle
      _rotate_min = -face_angle_max;
      _rotate_max = -face_angle_min;

      argc--;
      argv++;
      break;
      
    case 'f':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -f");
	break;
      }
      _ffacemodel = string(argv[1]);

      argc--;
      argv++;
      break;
      
    case 'n':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -n");
	break;
      }
      _fnonfacemodel = string(argv[1]);

      argc--;
      argv++;
      break;
      
      default:
	EchoError(string("unknown option ")+argv[0]);
      } // switch

    argc--;
    argv++; 
  } // while
  
  return argc_old-argc;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool LocateFaceBox::Process() {
    _pFace_model = LBModel::ReadModel(_ffacemodel);
    _pNonface_model = LBModel::ReadModel(_fnonfacemodel);
    
    list<LBBox> candidates;
    
    DetectFaces(candidates);
    
    if (candidates.empty()) {
      cout << "!!! No qualified face found !!!" << endl;
      return false;
    }
    
    LBBox fa;
    float dmax=0.0;
    int i=0;
    for (list<LBBox>::iterator ci=candidates.begin();ci!=candidates.end();ci++) {
      float d = ci->deltan - ci->deltaf;

      if (i==0 || d>dmax) {
      	dmax = d;
      	fa = *ci;
      }      
      i++;
    }
    
    if (Verbose()>2)
    cout << "fa.l=" << fa.l << " "
	 << "fa.t=" << fa.t << " "
	 << "fa.w=" << fa.w << " "
	 << "fa.degree=" << fa.degree << " "
	 << "fa.deltaf=" << fa.deltaf << " "
	 << "fa.deltan=" << fa.deltan << endl;

    imagedata* pImg = getImg()->accessFrame();
    int w0 = pImg->width();
    int h0 = pImg->height();
    float xc0 = 0.5*(w0-1);
    float yc0 = 0.5*(h0-1);
    float scale = (float)w0/fa.w;
    float xc = 0.5*(fa.w-1);
    float yc = 0.5*(h0/scale-1);
    float theta = PI * fa.degree / 180;    

    scalinginfo si(theta, xc, yc);
    float ls,ts;
    si.rotate_dst_xy_c(fa.l, fa.t, ls, ts);
    int l = round(scale*ls);
    int t = round(scale*ts);
    int w = round(scale*WIDTH_BOX_FACIALPARTS);
    int h = round(scale*HEIGHT_BOX_FACIALPARTS);
    float irxc, iryc;
    si.rotate_dst_xy_c((fa.l+0.5*WIDTH_BOX_FACIALPARTS),
		       (fa.t+0.5*HEIGHT_BOX_FACIALPARTS),
		       irxc, iryc);
    irxc = scale * irxc;
    iryc = scale * iryc;

    stringstream faceinnerbox;
    faceinnerbox << l << " " << t << " "
		 << w << " " << h << " "
		 << theta << " " << irxc << " " << iryc;

    SegmentationResult faceinnerboxres;
    faceinnerboxres.name="faceinnerbox";
    faceinnerboxres.type="rotated-box";
    faceinnerboxres.value=faceinnerbox.str();
    AddResultToXML(faceinnerboxres);

    float W = 30.0; // standard width of face inner box
    float HFORHEAD = 15.0; // standard forhead height
    float WF = 46.0;
    float HF = 56.0;
    float s = w / W; // scale between real face inner box and standard
                     // face inner box
    float wf = WF * s;
    float hf = HF * s;
    float lr, tr;

    scalinginfo si0(theta, xc0, yc0);

    si0.rotate_src_xy_c(lr, tr, l, t);
    float flfr = lr - 0.5*(WF-W)*s;
    float ftfr = tr - HFORHEAD*s;

    float flf, ftf;
    si0.rotate_dst_xy_c(flfr, ftfr, flf, ftf);

    float frxc, fryc;
    si0.rotate_dst_xy_c(flfr+0.5*wf, ftfr+0.5*hf, frxc, fryc);
  	
    stringstream facebox;
    facebox << round(flf) << " " << round(ftf) << " "
	    << round(wf) << " " << round(hf) << " "
            << theta << " " << frxc << " " << fryc;
    
    SegmentationResult faceboxres;
    faceboxres.name="face";
    faceboxres.type="rotated-box";
    faceboxres.value=facebox.str();
    AddResultToXML(faceboxres);

    float wh = hf;
    float WH = HF;
    float hlfr = lr - 0.5*(WH-W)*s;
    float htfr = ftfr;

    float hlf, htf;
    si0.rotate_dst_xy_c(hlfr, htfr, hlf, htf);

    float hrxc, hryc;
    si0.rotate_dst_xy_c(hlfr+0.5*wh, htfr+0.5*wh, hrxc, hryc);

    stringstream headbox;
    headbox << round(hlf) << " " << round(htf) << " "
	    << round(wh) << " " << round(wh) << " "
            << theta << " " << hrxc << " " << hryc;
    
    SegmentationResult headboxres;
    headboxres.name="head";
    headboxres.type="rotated-box";
    headboxres.value=headbox.str();
    AddResultToXML(headboxres);
    
    if (Verbose()>2)
      ShowFaceBox(fa);

    return ProcessNextMethod();
  }
 
  /////////////////////////////////////////////////////////////////////////////

  void LocateFaceBox::DetectFaces(list<LBBox>& faces) {
    imagedata img0 = *getImg()->accessFrame();
    /*    
    imagefile::displaysettings stgs;
    stgs.title = "img0";
    imagefile::display(img0, stgs);
    */
    
    img0.force_one_channel();
    
    int m = WIDTH_BOX_FACIALPARTS;
    int n = HEIGHT_BOX_FACIALPARTS;
    int w0 = img0.width();
    int h0 = img0.height();
    int w = round(_scale_min);
    int h = round(1.0 * h0 * w / w0);
    
    int fi=0;
    while(w<=_scale_max) {
      imagedata img = img0;
      scalinginfo scale_si(w0, h0, w, h);
      img.rescale(scale_si);
      
      if (Verbose()>2)
	cout << "w=" << w << endl;
      
      for (float degree=_rotate_min;degree<=_rotate_max;degree+=_rotate_step) {
      	imagedata rotated = img;
      	scalinginfo rotate_si(degree*PI/180, 0.5*(w-1), 0.5*(h-1));
      	rotated.rotate(rotate_si);
      	
	if (Verbose()>2)
	  cout << "degree=" << degree << endl;      	

      	for (int x=0;x<w-m+1;x++)
      	  for (int y=0;y<h-n+1;y++) {      	    	
      	    imagedata block = rotated.subimage(x,y,x+m-1,y+n-1);
      	    
      	    vector<float> Y = LBModel::PrepareVector(block);      	    
      	    float deltaf = _pFace_model->Score(Y);      	    
      	    float deltan = _pNonface_model->Score(Y);      	    

	    if (Verbose()>3)
	      cout << "w=" << w << " "
		   << "x=" << x << " "
		   << "y=" << y << " "
		   << "deltaf=" << deltaf << " "
		   << "deltan=" << deltan << endl;
	    
      	    if (deltaf<_theta && deltaf+_tau<deltan) {
      	      LBBox fa;
      	      fa.l = x;
      	      fa.t = y;
      	      fa.w = w;
      	      fa.degree = -degree;
      	      fa.deltaf = deltaf;
      	      fa.deltan = deltan;

	      if (Verbose()>2)
	      cout << "fa.l=" << fa.l << " "
		   << "fa.t=" << fa.t << " "
		   << "fa.w=" << fa.w << " "
		   << "fa.degree=" << fa.degree << " "
		   << "fa.deltaf=" << fa.deltaf << " "
		   << "fa.deltan=" << fa.deltan << endl;
      	      
      	      faces.push_back(fa);

	      if (Verbose()>2)
		cout << "face no. " << ++fi << " found" << endl;
      	    }
      	  }
      }
      w = round(w*_scale_augment);
      h = round(1.0 * h0 * w / w0);
    }   
  }
  
  /////////////////////////////////////////////////////////////////////////////

   void LocateFaceBox::ShowFaceBox(LBBox& fa) {
    imagedata img = *getImg()->accessFrame();
    
    int m = WIDTH_BOX_FACIALPARTS;
    int n = HEIGHT_BOX_FACIALPARTS;
    int l = fa.l;
    int t = fa.t;
    int r = fa.l + m - 1;
    int b = fa.t + n - 1;
    
    int w0 = img.width();
    int h0 = img.height();
    int w = fa.w;
    int h = round(1.0 * h0 * w / w0);

    scalinginfo scale_si(w0, h0, w, h);
    img.rescale(scale_si);
    
    float xc=0.5*(w-1), yc=0.5*(h-1);
    scalinginfo rotate_si(fa.degree*PI/180, xc, yc);
    
    float xlt, ylt, xlb, ylb, xrt, yrt, xrb, yrb;
    rotate_si.rotate_dst_xy(l-xc,t-yc,xlt,ylt);
    rotate_si.rotate_dst_xy(l-xc,b-yc,xlb,ylb);
    rotate_si.rotate_dst_xy(r-xc,t-yc,xrt,yrt);
    rotate_si.rotate_dst_xy(r-xc,b-yc,xrb,yrb);
    
    int nxlt = max(0,round(xlt+xc));
    int nylt = max(0,round(ylt+yc));
    int nxlb = max(0,round(xlb+xc));
    int nylb = min(h-1,round(ylb+yc));
    int nxrt = min(w-1,round(xrt+xc));
    int nyrt = max(0,round(yrt+yc));
    int nxrb = min(w-1,round(xrb+xc));
    int nyrb = min(h-1,round(yrb+yc));  

    img.force_three_channel();
    
    DrawLine(img, nxlt, nylt, nxlb, nylb);
    DrawLine(img, nxlb, nylb, nxrb, nyrb);
    DrawLine(img, nxrb, nyrb, nxrt, nyrt);
    DrawLine(img, nxrt, nyrt, nxlt, nylt);

    imagefile::displaysettings stgs;
    stgs.title = "FaceBox";
    imagefile::display(img, stgs);
   }  

   void LocateFaceBox::DrawLine(imagedata& img, int x1, int y1, int x2, int y2)  {

    std::vector<coord_float> list;
    Draw::LinePixelsToList(coord(x1,y1),coord(x2,y2),list);

    for (size_t i=0; i<list.size(); i++)
    	img.set(round(list[i].x), round(list[i].y), imagecolor<float>("red"));
	      
  }
  
} // namespace picsom
