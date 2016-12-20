#include <FaceParts.h>

static const string vcid="@(#)$Id: FaceParts.C,v 1.19 2009/04/30 08:12:00 vvi Exp $";

#define ROUND(x) floor((x)+0.5)
#define IROUND(x) (size_t)ROUND(x);
#define round(x) (size_t)floor((x)+0.5)

namespace picsom {

static FaceParts list_entry(true);

///////////////////////////////////////////////////////////////////////////

FaceParts::FaceParts() {
  // use gradient-based optimization
  Iterate = true;

  // allow failure in case no facial part found
  NotFound = false;

  // no debug prints,please
  DEBUG = 0;


  // set w,h for scaling down (according to the w,h of templates)
  FACE_WIDTH = 100;
  FACE_HEIGHT = 100;

  // init filenames
  strcpy(Fname_R_EYE,"r-eye.png");
  strcpy(Fname_L_EYE,"l-eye.png");
  strcpy(Fname_NOSE , "nose.png");
  strcpy(Fname_MOUTH, "mouth.png");

  _flmodel = "lbmodel/l-eye_model.txt";
  _frmodel = "lbmodel/r-eye_model.txt";
  _fnmodel = "lbmodel/nose_model.txt";
  _fmmodel = "lbmodel/mouth_model.txt";
  _fnonlmodel = "lbmodel/non_l-eye_model.txt";
  _fnonrmodel = "lbmodel/non_r-eye_model.txt";
  _fnonnmodel = "lbmodel/non_nose_model.txt";
  _fnonmmodel = "lbmodel/non_mouth_model.txt";

  theta_l = 1200; tau_l = -400;
  theta_r = 1200; tau_r = -400;
  theta_n = 1100; tau_n = -400;
  theta_m = 1000; tau_m = -300;

  scale_min = 50.0;
  scale_augment = 1.05;
  scale_max = 80.0;
}

///////////////////////////////////////////////////////////////////////////

FaceParts::~FaceParts() {

}

///////////////////////////////////////////////////////////////////////////

const char *FaceParts::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void FaceParts::UsageInfo(ostream& os) const { 
  os << "FaceParts :" << endl
     << "  options : " << endl
     << "    -noiter                    Will not run energy optimization at all." << endl
     << "    -notfound                  Allow finding failure in case no facial part found, " << endl
     << "                               otherwise will return a dummy result." << endl
     << "    -m <f_part>=<fname>        Specify custom input file for a template." << endl
     << "    -t <f_part>=<theta>        Specify custom theta value for detection." << endl
     << "    -T <f_part>=<tau>          Specify custom tau value for detection." << endl<<endl
     << "Legal names of f_part:         r-eye, l-eye, nose, mouth"<< endl
     << "Default filenames:             r-eye.png, l-eye.png, nose.png, mouth.png"<<
endl;
}

///////////////////////////////////////////////////////////////////////////

int FaceParts::ProcessOptions(int argc, char** argv) { 
  int argc_old=argc;
  int res;
  while(argv[0][0]== '-' && argc > 1) {

    // "-noiter"
    res = strcmp(&argv[0][1],"noiter");
    if(res == 0) {
      Iterate = false;
      argc--;
      argv++;
    }
    // "-notfound"
    res = strcmp(&argv[0][1],"notfound");
    if(res == 0) {
      NotFound = true;
      argc--;
      argv++;
    }
    // "-m"
    if( argv[0][1] == 'm' ) {
      argc--;
      argv++;

      res = strncmp(&argv[0][0],"r-eye=",6);
        // "-m r-eye=r_eye2.png"
        if(res == 0) {
          strcpy(Fname_R_EYE, &argv[0][6]);
          if(Verbose()) 
            cout <<"Using -m r-eye="<<Fname_R_EYE<<" as parameter."<<endl;

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"l-eye=",6);
        // "-m l-eye=l_eye2.png"
        if(res == 0) {
          strcpy(Fname_L_EYE, &argv[0][6]);
          if(Verbose()) 
            cout <<"Using -m l-eye="<<Fname_L_EYE<<" as parameter."<<endl;

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"nose=",5);
        // "-m nose=nose2.png"
        if(res == 0) {
          strcpy(Fname_NOSE, &argv[0][5]);
          if(Verbose()) 
            cout <<"Using -m nose="<<Fname_NOSE<<" as parameter."<<endl;

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"mouth=",6);
        // "-m mouth=mouth2.png"
        if(res == 0) {
          strcpy(Fname_MOUTH, &argv[0][6]);
          if(Verbose()) 
            cout <<"Using -m mouth="<<Fname_MOUTH<<" as parameter."<<endl;

          argc--;
          argv++;
        }
    }
    if( argv[0][1] == 't' ) {
      argc--;
      argv++;

      res = strncmp(&argv[0][0],"l-eye=",6);
        if(res == 0) {
          if (sscanf(&argv[0][6], "%f", &theta_l)!=1) {
	    cerr << "!!! l-eye theta specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"r-eye=",6);
        if(res == 0) {
          if (sscanf(&argv[0][6], "%f", &theta_r)!=1) {
	    cerr << "!!! r-eye theta specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"nose=",5);
        if(res == 0) {
          if (sscanf(&argv[0][5], "%f", &theta_n)!=1) {
	    cerr << "!!! nose theta specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"mouth=",6);
        if(res == 0) {
          if (sscanf(&argv[0][6], "%f", &theta_m)!=1) {
	    cerr << "!!! mouth theta specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }
    }

    if( argv[0][1] == 'T' ) {
      argc--;
      argv++;

      res = strncmp(&argv[0][0],"l-eye=",6);
        if(res == 0) {
          if (sscanf(&argv[0][6], "%f", &tau_l)!=1) {
	    cerr << "!!! l-eye tau specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"r-eye=",6);
        if(res == 0) {
          if (sscanf(&argv[0][6], "%f", &tau_r)!=1) {
	    cerr << "!!! r-eye tau specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"nose=",5);
        if(res == 0) {
          if (sscanf(&argv[0][5], "%f", &tau_n)!=1) {
	    cerr << "!!! nose tau specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"mouth=",6);
        if(res == 0) {
          if (sscanf(&argv[0][6], "%f", &tau_m)!=1) {
	    cerr << "!!! mouth tau specification error !!!" << endl;
	    return false;
	  }

          argc--;
          argv++;
        }
    }

    if( argv[0][1] == 's' ) {
      argc--;
      argv++;

      if (sscanf(&argv[0][0], "%f,%f,%f", 
		 &scale_min, 
		 &scale_augment,
		 &scale_max)!=3) {
	    cerr << "!!! scale specification error !!!" << endl;
	    return false;
      }

      argc--;
      argv++;
    }
  }
  return argc_old-argc;
}

///////////////////////////////////////////////////////////////////////////

bool FaceParts::Process_new() {
  PixelTemplate leye;
  PixelTemplate nose;
  int leye_i = AddPixelTemplate(leye);
  int nose_i = AddPixelTemplate(nose);

  state_t st = CreateStateVariable();

  spring_t leye_nose(&st, 55, 3, 2*leye_i, 2*leye_i+1, 2*nose_i, 2*nose_i+1);

  AddSpring(leye_nose);

  state_t low(st); // = ...
  state_t high(st); // = ...

  optimization = optimization_springsonly;
  state_t opt1 = Optimize(st, low, high);

  optimization = optimization_springsmoving;
  state_t opt2 = Optimize(opt1, low, high);

  optimization = optimization_springsandimages;
  state_t opt3 = Optimize(opt2, low, high);

  optimization = optimization_imageonly;
  /*state_t opt4 =*/ Optimize(opt3, low, high);

  return ProcessNextMethod();
}

///////////////////////////////////////////////////////////////////////////

bool FaceParts::ProcessRoz2() {
  bool wc = true;

  if (Verbose()) {
    cout << "processing frame " << getImg()->getCurrentFrame() << endl;
  }

  SegmentationResultList *resultlist = ReadResultsFromXML("face", "", wc);
  
  if (Verbose())
    cout << resultlist->size() << " face candidates" << endl;

  if (!resultlist ||
      (((*resultlist)[0].type!="box") &&
       ((*resultlist)[0].type!="rotated-box"))) {
      cerr << "Cannot get face data in FaceParts::ProcessRoz2!"
	   << " frame " << getImg()->getCurrentFrame()
	   << endl;
      return false;
  }

  size_t fi;  
  for (fi=0;fi<resultlist->size();fi++) {
    if (!ProcessSingleFace(resultlist, fi))
      return false;
  }

  return ProcessNextMethod();
}

///////////////////////////////////////////////////////////////////

bool FaceParts::ProcessSingleFace(SegmentationResultList* resultlist, size_t fi) {
  int minx, miny, maxx, maxy;
  int l, t, w, h;
  float theta, xc, yc;
  float luf, tuf;
  imagedata *pImg = getImg()->accessFrame();
 
  string data = (*resultlist)[fi].value;
  if ((*resultlist)[fi].type=="rotated-box") {
    if (sscanf(data.c_str(), "%d %d %d %d %f %f %f",
	       &l, &t, &w, &h, &theta, &xc, &yc) != 7) {
      cerr << "Cannot read head rotated box data in FaceParts::ProcessRoz2"
	   << endl;
      delete resultlist;
      return false;
    }

    if (Verbose()>2)
      cout << "rotated-box data read from segment file: " << endl
	   << "l=" << l << " "
	   << "t=" << t << " "
	   << "w=" << w << " "
	   << "h=" << h << " "
	   << "theta=" << theta << " "
	   << "xc=" << xc << " "
	   << "yc=" << yc << endl;

    scalinginfo si(-theta, xc, yc);    
    pImg->rotate(si);
    si.rotate_dst_xy_c((float)l,(float)t, luf, tuf);
    minx = round(luf);
    miny = round(tuf);
    maxx = minx + w - 1;
    maxy = miny + h - 1;
  }
  else { // i.e. the type is "box"
    if (sscanf(data.c_str(),"%d,%d,%d,%d",&minx,&miny,&maxx,&maxy)!=4 &&
	sscanf(data.c_str(),"%d%d%d%d",&minx,&miny,&maxx,&maxy)!=4) {
      cerr << "Cannot read face bounding box data in FaceParts::ProcessRoz2"
	   << endl;
      delete resultlist;
      return false;
    }

    if (Verbose()>2)
      cout << "box data read from segment file: " << endl
	   << "minx=" << minx << " "
	   << "miny=" << miny << " "
	   << "maxx=" << maxx << " "
	   << "maxy=" << maxy << endl;

    xc = 0.5 * pImg->width();
    yc = 0.5 * pImg->height();
    theta = 0.0;
  }

  imagedata img = *getImg()->accessFrame();
  img.force_one_channel();

  if (minx<0 || miny<0 || maxx<0 || maxy<0) {
    return true;
  }

  //  cout << "before MIN: maxx=" << maxx << "img.width()=" << img.width() << endl;

  maxx = MIN(maxx, (int)(img.width()-1));
  maxy = MIN(maxy, (int)(img.height()-1));

  //  cout << "after MIN: maxx=" << maxx << "img.width()=" << img.width() << endl;

  if (maxx>=(int)img.width())
    throw string("ProcessSingleFace: maxx>=img.width()");

  if (maxy>=(int)img.height())
    throw string("ProcessSingleFace: maxy>=img.height()");

  if (minx>=maxx)
    throw string("ProcessSingleFace: minx>maxx");

  if (miny>=maxy)
    throw string("ProcessSingleFace: miny>maxy");

  imagedata imgface = img.subimage(minx, miny, maxx, maxy);

  if (!DetectFacePart("l-eye", imgface, minx, miny,
		      theta, xc, yc, theta_l, tau_l))
    return false;

  if (!DetectFacePart("r-eye", imgface, minx, miny,
		      theta, xc, yc, theta_r, tau_r))
    return false;

  if (!DetectFacePart("nose",  imgface, minx, miny,
		      theta, xc, yc, theta_n, tau_n))
    return false;

  if (!DetectFacePart("mouth", imgface, minx, miny,
		      theta, xc, yc, theta_m, tau_m))
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////

 bool FaceParts::DetectFacePart(string partname, const imagedata& imgface,
				size_t x0, size_t y0, float theta,
				float xc, float yc, float detect_theta,
				float detect_tau) {
   if (Verbose()>2) {
     cout << "entering DetectFacePart..." << endl;
     cout << "partname=" << partname << " "
	  << "x0=" << x0 << " "
	  << "y0=" << y0 << " "
	  << "theta=" << theta << " "
	  << "xc=" << xc << " "
	  << "yc=" << yc << endl;
     cout << "detect_theta=" << detect_theta << " "
          << "detect_tau=" << detect_tau << endl;
   }

  LBModel *pModel=NULL, *pNonModel=NULL;
  int wpart=0, hpart=0;
  int xbegin=0, xend=0, ybegin=0, yend=0;

  size_t w0 = imgface.width();
  size_t h0 = imgface.height();

  switch (partname[0]) {
  case 'l':
    pModel = LBModel::ReadModel(_flmodel);
    pNonModel = LBModel::ReadModel(_fnonlmodel);
    wpart = 21;
    hpart = 12;
    break;
  case 'r':
    pModel = LBModel::ReadModel(_frmodel);
    pNonModel = LBModel::ReadModel(_fnonrmodel);
    wpart = 21;
    hpart = 12;
    break;
  case 'n':
    pModel = LBModel::ReadModel(_fnmodel);
    pNonModel = LBModel::ReadModel(_fnonnmodel);
    wpart = 16;
    hpart = 16;
    break;
  case 'm':
    pModel = LBModel::ReadModel(_fmmodel);
    pNonModel = LBModel::ReadModel(_fnonmmodel);
    wpart = 21;
    hpart = 12;
    break;
  default:
    cerr << "DetectFacePart(): face part not implemented." << endl;
  }

  size_t w = round(scale_min);
  size_t h = round(1.0 * h0 * w / w0);

  list<LBBox> candidates;
    
  int pno=0;
  while(w<=scale_max) {
    if (Verbose()>2)
      cout << "w=" << w << endl;

    imagedata scaled = imgface;

    scalinginfo scale_si(w0, h0, w, h);
    scaled.rescale(scale_si);
    
    switch (partname[0]) {
    case 'l':
      xbegin = round(0.5*w);
      xend   = round(0.9*w) - wpart + 1;
      ybegin = round(0.2*h);
      yend   = round(0.5*h) - hpart + 1;
    break;
    case 'r':
      xbegin = round(0.1*w);
      xend   = round(0.5*w) - wpart + 1;
      ybegin = round(0.2*h);
      yend   = round(0.5*h) - hpart + 1;
    break;
    case 'n':
      xbegin = round(0.3*w);
      xend   = round(0.7*w) - wpart + 1;
      ybegin = round(0.3*h);
      yend   = round(0.8*h) - hpart + 1;
      break;
    case 'm':
      xbegin = round(0.2*w);
      xend   = round(0.8*w) - wpart + 1;
      ybegin = round(0.5*h);
      yend   = h - hpart + 1;
      break;
    default:
      cerr << "DetectFacePart(): face part not implemented." << endl;
  }
    
    xbegin = MAX(xbegin, 0);
    ybegin = MAX(ybegin, 0);
    xend = MIN(xend, (int)(scaled.width()-wpart));
    yend = MIN(yend, (int)(scaled.height()-hpart));

    if (Verbose()>2)
      cout << "xbegin=" << xbegin << " "
	   << "xend=" << xend << " "
	   << "ybegin=" << ybegin << " "
	   << "yend=" << yend << endl;

    if (wpart<=0)
      throw string("DetectFacePart: wpart<=0");
    if (hpart<=0)
      throw string("DetectFacePart: hpart<=0");
    
    /*
    xbegin = 0;
    xend   = w - wpart;
    ybegin = 0;
    yend   = h - hpart;
    */
    for (int x=xbegin;x<xend;x++)
      for (int y=ybegin;y<yend;y++) {

	if (x+wpart-1>=(int)scaled.width())
	  throw string("DetectFacePart: x+wpart-1>=scaled.width()");

	if (y+hpart-1>=(int)scaled.height())
	  throw string("DetectFacePart: y+hpart-1>=scaled.height()");

	imagedata block = scaled.subimage(x,y,x+wpart-1,y+hpart-1);
	vector<float> Y = LBModel::PrepareVector(block);
	float deltaf = pModel->Score(Y);
	float deltan = pNonModel->Score(Y);

	if (Verbose()>2) {
	  if (isnan(deltaf) || isnan(deltan)) {
	    cout << "### Y=";
	    for (size_t i=0;i<Y.size();i++)
	      cout << Y[i] << " " ;
	    cout << endl;
	  }
	}
	
	if (Verbose()>3)
	  cout << "deltaf=" << deltaf << "deltan=" << deltan << endl;

	if (deltaf<detect_theta && deltaf+detect_tau<deltan) {
	  LBBox candidate;
	  candidate.l = x;
	  candidate.t = y;
	  candidate.w = w;
	  candidate.degree = 0;
	  candidate.deltaf = deltaf;
	  candidate.deltan = deltan;
      	      
	  candidates.push_back(candidate);
	  
	  if (Verbose()>2) {
	    cout << partname << " no. " << ++pno << " found. ";
	    cout << "l=" << candidate.l << " "
		 << "t=" << candidate.t << " "
		 << "w=" << candidate.w << " "
		 << "deltaf=" << candidate.deltaf << " " 
		 << "deltan=" << candidate.deltan << endl;
	  }
	}
      }
    w = round(w*scale_augment);
    h = round(1.0 * h0 * w / w0);

  }

  if (candidates.size()==0) {
    if (NotFound) {
      cout << "!!! " << partname << " not found !!!" << endl;
      return false;
    }
    else {
      LBBox candidate;
      candidate.l = 0;
      candidate.t = 0;
      candidate.w = round(scale_min);
      candidate.degree = 0;
      candidate.deltaf = 0;
      candidate.deltan = 0;
      candidates.push_back(candidate);
    }
  }

  LBBox part;
  float dmax=0.0;
  size_t i=0;
  for (list<LBBox>::iterator ci=candidates.begin();ci!=candidates.end();ci++) {
    float d = ci->deltan - ci->deltaf;
    
    if (i==0 || d>dmax) {
      dmax = d;
      part = *ci;
    }      
    i++;
  }

  if (Verbose()) {
    cout << "****************************************" << endl;
    cout << "best " << partname << ": "
	 << "l=" << part.l << " "
	 << "t=" << part.t << " "
	 << "w=" << part.w << " "
	 << "deltaf=" << part.deltaf << " " 
	 << "deltan=" << part.deltan << endl;
    cout << "****************************************" << endl;
  }
  
  float scale = (float)w0/part.w;
  float fl = part.l * scale + x0;
  float ft = part.t * scale + y0;
  scalinginfo si(-theta, xc, yc);
  float fls, fts;
  si.rotate_src_xy_c(fls, fts, fl, ft);
  size_t lres = round(fls);
  size_t tres = round(fts);
  size_t wres = round(scale*wpart);
  size_t hres = round(scale*hpart);

  float pxrc, pyrc;
  si.rotate_src_xy_c(pxrc, pyrc, fl+0.5*wpart*scale, ft+0.5*hpart*scale);

  stringstream partstr;
  partstr << lres << " " << tres << " "
	  << wres << " " << hres << " "
	  << theta << " " << pxrc << " " << pyrc;
  
  SegmentationResult partres;
  partres.name=partname;
  partres.type="rotated-box";
  partres.value=partstr.str();
  AddResultToXML(partres);

  return true;
}


///////////////////////////////////////////////////////////////////////////

bool FaceParts::ProcessRoz() {
  // -----------------------------------------------------
  int F_tlx=0,F_tly=0,F_brx=0,F_bry=0;
 
  // Reading face (coordinate) information from XML to F_xxx vars
  SegmentationResultList *resultlist = ReadResultsFromXML("face");
  if (resultlist!=NULL){
    for (size_t i=0;i<resultlist->size();i++) {
      const char *val = (*resultlist)[i].value.c_str();
      int x1,x2,y1,y2;
      if((*resultlist)[i].type=="box"){
        if (sscanf(val,"%d,%d,%d,%d",&x1,&y1,&x2,&y2)==4 ||
	    sscanf(val,"%d%d%d%d",&x1,&y1,&x2,&y2)==4)
	  F_tlx = x1; F_tly = y1;
          F_brx = x2; F_bry = y2;
      }
      else {
         cout << "Error: face coordinates not found!"<<endl;
         abort();
      }
    }
  delete resultlist;
  }
  // Checking boundaries and shrinking them if needed.
  if(F_tlx < 0) F_tlx = 0;
  if(F_tly < 0) F_tly = 0;
  if(F_brx > Width()-1) F_brx = Width()-1;
  if(F_bry > Height()-1) F_bry = Height()-1;

  // -----------------------------------------------------
  // these will be needed when scaling results up to match original image
  float S_X = (float)(F_brx-F_tlx+1)/(float)FACE_WIDTH;
  float S_Y = (float)(F_bry-F_tly+1)/(float)FACE_HEIGHT;


  if (Verbose()>1)
    cout << "Scaling Image.." << endl;
  // -----------------------------------------------------
  // Creating & Scaling down Face
  PixelTemplate *face = new PixelTemplate(*getImg()->accessFrame(), 
					  F_tlx, F_tly, 
					  F_brx, F_bry,
					  FACE_WIDTH, FACE_HEIGHT); 

  // -----------------------------------------------------
  // Defining some constants for PixelTemplate Calculations
  float centerx = face->width() / 2.0; 
  float centery = face->height() / 2.0;
  float mux[4] = {-18.2402, 19.2648, 0.6570, 1.1393};
  float muy[4] = {1.3664, 0.9204, 14.8706, 30.7077};
  float sigmax[4] = {2.4787, 2.4508, 2.6431, 2.8290};
  float sigmay[4] = {2.9525, 3.1033, 2.9633, 3.0483};
  
  cout << getImg()->getImageFileName() << endl;

  for (int partj=0;partj<4;partj++)
  {
  	float rw = sigmax[partj] * 1.65;
  	float rh = sigmay[partj] * 1.65;
  	
  	int l0 = IROUND(mux[partj] - rw + centerx);
  	int t0 = IROUND(muy[partj] - rh + centery);
  	int r0 = IROUND(mux[partj] + rw + centerx);
  	int b0 = IROUND(muy[partj] + rh + centery);
  	
//  	cout << "l0=" << l0 << "; " << "t0=" << t0 << "; " << "r0=" << r0 << "; " << "b0=" << b0 << endl;

  	char Fname_part[20];
  	switch (partj)
  	{
  		case 0:
  			strcpy(Fname_part, Fname_L_EYE);
  			break;
  		case 1:
  			strcpy(Fname_part, Fname_R_EYE);
  			break;
  		case 2:
  			strcpy(Fname_part, Fname_NOSE);
  			break;
  		case 3:
  			strcpy(Fname_part, Fname_MOUTH);
  			break;
  	}
  	imagefile fpart(Fname_part);
  	
  	int w = fpart.frame(0).width();
  	int h = fpart.frame(0).height();
  	int tw =  w / 2;
  	int th =  h / 2;
  	
//  	cout << "w=" << w << "; " << "h=" << h << "; " << "tw=" << tw << "; " << "th=" << th << endl;
  	
  	l0 = ( l0 - tw >= 0 ) ? l0 : tw;
  	t0 = ( t0 - th >= 0 ) ? t0 : th;
  	r0 = ( r0 - tw + w - 1 < (int)face->width() ) ? r0 : face->width() + tw - w + 1;
  	b0 = ( b0 - th + h - 1 < (int)face->height() ) ? b0 : face->height() + th - h + 1;
  	
//  	cout << "l0=" << l0 << "; " << "t0=" << t0 << "; " << "r0=" << r0 << "; " << "b0=" << b0 << endl;
  	
  	PixelTemplate *pTmpl0 = (PixelTemplate*)(&(fpart.frame(0)));
//  	cout << "before new" << endl;
  	PixelTemplate *pTmpl = new PixelTemplate(w, h);
//  	cout << "before Normalize" << endl;
  	pTmpl0->Normalize(pTmpl0, pTmpl, 0, 0, w-1, h-1);
  	
//  	cout << "before iteration" << endl;
  	
  	float max_corr2 = -1;
  	int max_corr2_x = -1;
  	int max_corr2_y = -1;
  	for (int x=l0;x<=r0;x++)
  		for (int y=t0;y<=b0;y++)
  		{
//  			cout << "x=" << x << "; " << "y=" << y << endl;
  			PixelTemplate* pPart = new PixelTemplate(w, h);
//  			cout << "before pPart normalization" << endl;
  			face->Normalize(face, pPart, x-tw, y-th, x-tw+w-1, y-th+h-1);
//  			cout << "before corr2" << endl;
  			float corr2 = pPart->Corr2(pTmpl);
  			if (max_corr2<corr2)
  			{
  				max_corr2 = corr2;
  				max_corr2_x = x;
  				max_corr2_y = y;  				
  			}
  			delete pPart;
  		}
  	
//  	PixelTemplate *part_corr = face->Correlate(fpart.frame(0), l0, t0, r0, b0, 1, 1);
  	
  	SegmentationResult myresult;
  	char corr_max[50]; // Should be enough to correlation coordinates
//  	sprintf(corr_max, "%d %d",
//  		(int)(part_corr->GetCorrMax_x()*S_X+F_tlx), 
//  		(int)(part_corr->GetCorrMax_y()*S_Y+F_tly));
	sprintf(corr_max, "%d %d",
		(int)(max_corr2_x*S_X+F_tlx),
		(int)(max_corr2_y*S_Y+F_tly));
 
   	switch (partj)
  	{
  		case 0:
  			myresult.name="l-eye";
  			break;
  		case 1:
  			myresult.name="r-eye";
  			break;
  		case 2:
  			myresult.name="nose";
  			break;
  		case 3:
  			myresult.name="mouth";
  			break;
  	}
  	myresult.type="point";
  	myresult.value=corr_max;
  	if(!AddResultToXML(myresult))
  		cout << "partj=" << partj << ",point," << myresult.value <<") failed" << endl;
  		
  	delete pTmpl;
  }

  return ProcessNextMethod();
}
///////////////////////////////////////////////////////////////////////////

bool FaceParts::Process() {
	return ProcessRoz2();
/*	
  if (Verbose()>1)
    ShowLinks();

  int l = (Width()+Height())/2;

  float avgr = 0, avgg = 0, avgb = 0;

  for (int x=0; x<Width(); x++)
    for (int y=0; y<Height(); y++) {
      int s = 1;
      if (y>x)
	s += 2;
      if (x+y>l)
	s += 1;
      //cout << x << "," << y << endl;
      float r, g, b;
      bool ok = GetPixelRGB(x, y, r, g, b);
      if (!ok) {
	cerr << "FaceParts::Process() failed in GetPixelRGB("
	     << x << "," << y << "," << s << ")" << endl;
	return false;
      }
      avgr += r;
      avgg += g;
      avgb += b;

      ok = SetPixelSegment(x, y, s);
      if (!ok) {
	cerr << "FaceParts::Process() failed in SetSegment("
	     << x << "," << y << "," << s << ")" << endl;
	return false;
      }
    }

  if (Verbose()>1) {
    float d = Width()*Height();
    cout << "avgr = " << (avgr/d) << endl;
    cout << "avgg = " << (avgg/d) << endl;
    cout << "avgb = " << (avgb/d) << endl;
  }

  // -----------------------------------------------------
  // Reading face (coordinate) information from XML to F_xxx vars
  int F_tlx,F_tly,F_brx,F_bry;
  SegmentationResultList *resultlist = ReadResultsFromXML("head");
  if (!resultlist)
    return false;

  for (size_t i=0;i<resultlist->size();i++) {
    const char *val = (*resultlist)[i].value.c_str();
    int x1,x2,y1,y2;
    if((*resultlist)[i].type=="box"){
      if (sscanf(val,"%d,%d,%d,%d",&x1,&y1,&x2,&y2)==4 ||
	  sscanf(val,"%d%d%d%d",&x1,&y1,&x2,&y2)==4)
	F_tlx = x1; F_tly = y1;
      F_brx = x2; F_bry = y2;
    }
    else {
      cout << "Error: face coordinates not found!"<<endl;
      abort();
    }
  }
  delete resultlist;
  // Checking boundaries and shrinking them if needed.
  if(F_tlx < 0) F_tlx = 0;
  if(F_tly < 0) F_tly = 0;
  if(F_brx > Width()-1) F_brx = Width()-1;
  if(F_bry > Height()-1) F_bry = Height()-1;

  // -----------------------------------------------------
  // these will be needed when scaling results up to match original image
  float S_X = (float)(F_brx-F_tlx+1)/(float)FACE_WIDTH;
  float S_Y = (float)(F_bry-F_tly+1)/(float)FACE_HEIGHT;


  if (Verbose()>1)
    cout << "Scaling Image.." << endl;
  // -----------------------------------------------------
  // Creating & Scaling down Face
  PixelTemplate *face = new PixelTemplate(*getImg()->accessFrame(), 
					  F_tlx, F_tly, 
					  F_brx, F_bry,
					  FACE_WIDTH, FACE_HEIGHT); 

  // -----------------------------------------------------
  // Defining some constants for PixelTemplate Calculations
  // define right eye search area (w:0->2/3, h:1/6->3/4)
  const int R_EYE_L = 0;		  // 
  const int R_EYE_R = face->width() /2; // 2/3
  const int R_EYE_T = 1*face->height()/4; // 	   
  const int R_EYE_B = 3*face->height()/4; //
  // define left eye search area (w:1/3->w-1,h:1/6->3/4)
  const int L_EYE_L = 1*face->width()/2; //1/3 
  const int L_EYE_R = face->width() -1;
  const int L_EYE_T = 1*face->height()/4;		   
  const int L_EYE_B = 3*face->height()/4;

  // define nose search area (w:0->w-1,h:1/4->h-1)
  const int NOSE_L  = 1*face->width()/4; //face->width()/3;  
  const int NOSE_R  = 3*face->width()/4; //3*face->width() /4;
  const int NOSE_T  = face->height()/4;  //quite tight
  const int NOSE_B  = face->height()-1;  // since some images have mouth missing
  // define mouth search area (w:0->w-1,h:2/3->h-1)
  const int MOUTH_L = face->width()/5;    // 
  const int MOUTH_R = 4*face->width()/5;  //
  const int MOUTH_T = 2*face->height()/3;
  const int MOUTH_B = face->height()-1;

  // -----------------------------------------------------
  // locating files that contain templates of faceparts
  imagefile   r_eye(Fname_R_EYE);
  imagefile   l_eye(Fname_L_EYE);
  imagefile   nose(Fname_NOSE);
  imagefile   mouth(Fname_MOUTH);


  if (Verbose()>1)
    cout << "Calculating Correlation.." << endl;

  // ----------------------------------------------------------
  // Calculating correlations for each faceparts
  PixelTemplate *r_eye_corr = face->Correlate(r_eye.frame(0),
					      R_EYE_L, R_EYE_T,
					    R_EYE_R, R_EYE_B, 1, 1);
  PixelTemplate *l_eye_corr = face->Correlate(l_eye.frame(0), 
					      L_EYE_L, L_EYE_T, 
					      L_EYE_R, L_EYE_B, 1, 1);
  PixelTemplate *nose_corr  = face->Correlate(nose.frame(0),
					      NOSE_L, NOSE_T, 
					      NOSE_R, NOSE_B, 1, 1);
  PixelTemplate *mouth_corr = face->Correlate(mouth.frame(0),  
					      MOUTH_L, MOUTH_T,
					      MOUTH_R, MOUTH_B, 1, 1);

  // ---------------------------- -----------------------------
  // THIS is the place to write alternative optimization method
  // if the system is modified later


  // Define some coefficients and constants for Energy calculations
  // K for scaling E_img to accomodate E_struct

    const float Ke = (float)1/(float)100;  //energy scaling factor
    const float Kg = (float)0.75;          // gradient scaling factor
    const float Eslim = 0.001;             // näillä
    const float Eilim = 0.25;              // arvoilla ja || ehdolla
    const float Etotlim = 0.0045; 

//    const float Ke = (float)1/(float)100;  //energy scaling factor
//    const float Kg = (float)0.75;             // gradient scaling factor
//    const float Eslim = 0.003; // näillä
//    const float Eilim = 0.29;    // arvoilla ja || ehdolla
//    const float Etotlim = 0.0045; 


  const int ITER_MAX    = 1000;	     // maximum number of iterations
  float gEtotxy[4]      = {0,0,0,0}; // array for absolute gradients of Etot  components
  int n = 0, iterations = 0;         // loop counter

  // Calculate Estruct,Eimg, Etot and the gradients initially

  float E_s = E_struct(r_eye_corr, l_eye_corr, nose_corr, mouth_corr);
  float E_i = E_img(r_eye_corr, l_eye_corr, nose_corr, mouth_corr);
  float Etot = (float)E_s + (float)Ke*(float)E_i;
  // grad_Es & grad_Ei are 8 component arrays
  float *grad_Es = Gradient_E_struct(r_eye_corr, l_eye_corr, nose_corr, mouth_corr);
  float *grad_Ei = Gradient_E_img(r_eye_corr, l_eye_corr, nose_corr, mouth_corr);
  float *grad_Etot = new float[8];
  for(n=0; n<8; n++) 
    grad_Etot[n] = grad_Es[n]+Kg*grad_Ei[n];

  if (DEBUG) {
    cout << "E_tot:" << Etot <<endl;
    cout << "Es: " << E_s << " Ei: "<<E_i <<endl;
    cout<<"R_eye: "<<r_eye_corr->GetOptCorrMax_x()<<" "<<r_eye_corr->GetOptCorrMax_y()<<endl;
    cout<<"L_eye: "<<l_eye_corr->GetOptCorrMax_x()<<" "<<l_eye_corr->GetOptCorrMax_y()<<endl;
    cout<<"Nose: "<<nose_corr->GetOptCorrMax_x()<<" "<<nose_corr->GetOptCorrMax_y()<<endl;
    cout<<"Mouth: "<<mouth_corr->GetOptCorrMax_x()<<" "<<mouth_corr->GetOptCorrMax_y()<<endl;
    for(n=0; n<8; n++) cout << grad_Es[n] <<" ";
    cout << " end of Gradient_E_struct"<<endl;
    for(n=0; n<8; n++) cout << grad_Ei[n] <<" ";
    cout << " end of Gradient_E_img"<<endl;
    for(n=0; n<8; n++) cout << grad_Etot[n] <<" ";
    cout << " end of Gradient_E_tot"<<endl;
    cout <<"gEtotxy: "<<gEtotxy[0]<<" " <<gEtotxy[1]<<" "<<gEtotxy[2]<<" "<<gEtotxy[3]<<endl;
  }

  if (Verbose()>1)
    cout << "Optimising result.." << endl;


  // ------------------------------- ------------------------------
  // Iterations begin (gradient-based minimum energy search)
  if( Iterate ) {
    for(iterations=0; iterations<ITER_MAX; iterations++) { 
    // Iterate until suitable ending condition is found
      if(  (E_s < Eslim && E_i < Eilim) || (Etot < Etotlim) ) {
        // cout << "Fine work" << Etot <<endl;
        break;
      }

      // gradient vectors are 8 dimensional, 
      // [0] & [1] == r_eye: delta( (x),(y) )
      // [2] & [3] == l_eye: delta( (x),(y) )
      // [4] & [5] == nose: delta ( (x),(y) )
      // [6] & [7] == mouth: delta ((x),(y))
     
      // calculate absolute gradient of Etot for comparison 
      gEtotxy[0] = grad_Etot[0]*grad_Etot[0] + grad_Etot[1]*grad_Etot[1];
      gEtotxy[1] = grad_Etot[2]*grad_Etot[2] + grad_Etot[3]*grad_Etot[3];
      gEtotxy[2] = grad_Etot[4]*grad_Etot[4] + grad_Etot[5]*grad_Etot[5];
      gEtotxy[3] = grad_Etot[6]*grad_Etot[6] + grad_Etot[7]*grad_Etot[7];

      // Update component which has greatest absolute gradient 
      if(gEtotxy[0] > gEtotxy[1] && gEtotxy[0] > gEtotxy[2] &&
         gEtotxy[0] > gEtotxy[3] ) {
         // Update only x or y component (which one is greater)
         if( (float)(grad_Etot[0]*grad_Etot[0]) > (float)(grad_Etot[1]*grad_Etot[1]) ) {
           ((float)-grad_Etot[0]>(float)0)? 
            r_eye_corr->SetOptCorrMax_x(r_eye_corr->GetOptCorrMax_x()+1):
            r_eye_corr->SetOptCorrMax_x(r_eye_corr->GetOptCorrMax_x()-1);
         }
         else {
           ((float)-grad_Etot[1]>(float)0)? 
            r_eye_corr->SetOptCorrMax_y(r_eye_corr->GetOptCorrMax_y()+1):
            r_eye_corr->SetOptCorrMax_y(r_eye_corr->GetOptCorrMax_y()-1);
         }
      }
      else if(gEtotxy[1] > gEtotxy[2]&& gEtotxy[1] >gEtotxy[3]) {
         // Update only x or y component (which one is greater)
        if( (float)(grad_Etot[2]*grad_Etot[2]) > (float)(grad_Etot[3]*grad_Etot[3]) ){
          ((float)-grad_Etot[2]>(float)0)? 
           l_eye_corr->SetOptCorrMax_x(l_eye_corr->GetOptCorrMax_x()+1):
           l_eye_corr->SetOptCorrMax_x(l_eye_corr->GetOptCorrMax_x()-1);
        }
        else {
          ((float)-grad_Etot[3]>(float)0)? 
           l_eye_corr->SetOptCorrMax_y(l_eye_corr->GetOptCorrMax_y()+1):
           l_eye_corr->SetOptCorrMax_y(l_eye_corr->GetOptCorrMax_y()-1);
        }
      }
      else if( gEtotxy[2] > gEtotxy[3] ) {
         // Update only x or y component (which one is greater)
        if( (float)(grad_Etot[4]*grad_Etot[4]) > (float)(grad_Etot[5]*grad_Etot[5]) ){
          ((float)-grad_Etot[4]>(float)0)? 
           nose_corr->SetOptCorrMax_x(nose_corr->GetOptCorrMax_x()+1):
           nose_corr->SetOptCorrMax_x(nose_corr->GetOptCorrMax_x()-1);
        }
        else {
          ((float)-grad_Etot[5]>(float)0)? 
           nose_corr->SetOptCorrMax_y(nose_corr->GetOptCorrMax_y()+1):
           nose_corr->SetOptCorrMax_y(nose_corr->GetOptCorrMax_y()-1);
        }
      }
      else { // last component is greatest
         // Update only x or y component (which one is greater)
        if( (float)(grad_Etot[6]*grad_Etot[6]) > (float)(grad_Etot[7]*grad_Etot[7]) ){
          ((float)-grad_Etot[6]>(float)0)? 
           mouth_corr->SetOptCorrMax_x(mouth_corr->GetOptCorrMax_x()+1):
           mouth_corr->SetOptCorrMax_x(mouth_corr->GetOptCorrMax_x()-1);
        }
        else {
          ((float)-grad_Etot[7]>(float)0)? 
           mouth_corr->SetOptCorrMax_y(mouth_corr->GetOptCorrMax_y()+1):
           mouth_corr->SetOptCorrMax_y(mouth_corr->GetOptCorrMax_y()-1);
        }
      }

      E_s = E_struct(r_eye_corr, l_eye_corr, nose_corr, mouth_corr);
      E_i = E_img(r_eye_corr, l_eye_corr, nose_corr, mouth_corr);
      Etot = (float)E_s + (float)Ke*(float)E_i;

      // remove pointers to arrays since they will get new arrays below
      delete[] grad_Es;
      delete[] grad_Ei;
      grad_Es = Gradient_E_struct(r_eye_corr, l_eye_corr, nose_corr,mouth_corr);
      grad_Ei = Gradient_E_img(r_eye_corr, l_eye_corr, nose_corr, mouth_corr);
      for(n=0; n<8; n++) 
        grad_Etot[n] = grad_Es[n] +Kg*grad_Ei[n];

      if(DEBUG) {
        // some debugging information
        for(n=0; n<8; n++) cout << grad_Es[n] <<" ";
        cout << " end of Gradient_E_struct"<<endl;
        for(n=0; n<8; n++) cout << grad_Ei[n] <<" ";
        cout << " end of Gradient_E_img"<<endl;
        for(n=0; n<8; n++) cout << grad_Etot[n] <<" ";
        cout << " end of Gradient_E_tot"<<endl;
        cout<<"gEtotxy: "<<gEtotxy[0]<<" " <<gEtotxy[1]<<" "<<gEtotxy[2]<<" "<<gEtotxy[3]<<endl;
        cout<<"R_eye: "<<r_eye_corr->GetOptCorrMax_x()<<" "<<r_eye_corr->GetOptCorrMax_y()<<endl;
        cout<<"L_eye: "<<l_eye_corr->GetOptCorrMax_x()<<" "<<l_eye_corr->GetOptCorrMax_y()<<endl;
        cout<<"Nose: "<<nose_corr->GetOptCorrMax_x()<<" "<<nose_corr->GetOptCorrMax_y()<<endl;
        cout<<"Mouth: "<<mouth_corr->GetOptCorrMax_x()<<" "<<mouth_corr->GetOptCorrMax_y()<<endl;
        cout << "Es: " << E_s << " Ei: "<<E_i <<" Etot: "<<Etot<<endl;
        cout << iterations <<":iterations"<<endl;
      }

    }  
  } // ---------------------- Iterations end

  if(DEBUG) {
  cout << "E_tot:" << Etot <<endl;
  cout << "Es: " << E_s << " Ei: "<<E_i <<endl;
  cout <<"R_eye: "<<r_eye_corr->GetOptCorrMax_x()<<" "<<r_eye_corr->GetOptCorrMax_y()<<endl;
  cout <<"L_eye: "<<l_eye_corr->GetOptCorrMax_x()<<" "<<l_eye_corr->GetOptCorrMax_y()<<endl;
  cout <<"Nose: "<<nose_corr->GetOptCorrMax_x()<<" "<<nose_corr->GetOptCorrMax_y()<<endl;
  cout <<"Mouth: "<<mouth_corr->GetOptCorrMax_x()<<" "<<mouth_corr->GetOptCorrMax_y()<<endl;
  for(n=0; n<8; n++) cout << grad_Es[n] <<" ";
  cout << " end of Gradient_E_struct"<<endl;
  for(n=0; n<8; n++) cout << grad_Ei[n] <<" ";
  cout << " end of Gradient_E_img"<<endl;
  for(n=0; n<8; n++) cout << grad_Etot[n] <<" ";
  cout << " end of Gradient_E_tot"<<endl;
  cout << " iter: "<<iterations<<endl;
  cout <<"gEtotxy: "<<gEtotxy[0]<<" " <<gEtotxy[1]<<" "<<gEtotxy[2]<<" "<<gEtotxy[3]<<endl;
  }

  //          Gradient -based optimization method ends
  // ---------------------------- -----------------------------



  // -----------------------------------------------------------
  // Saving correlation results (initial correlation coordinates) to XML
  if (Verbose()>1)
    cout << "Saving correlation results to XML" << endl;

  SegmentationResult myresult;
  char corr_max[50]; // Should be enough to correlation coordinates
  sprintf(corr_max, "%d %d",(int)(r_eye_corr->GetCorrMax_x()*S_X+F_tlx), 
                   (int)(r_eye_corr->GetCorrMax_y()*S_Y+F_tly));

  myresult.name="r-eye:0";
  myresult.type="point";
  myresult.value=corr_max;
  if(!AddResultToXML(myresult))
    cout << "writing (r-eye:0,point," << myresult.value <<") failed" << endl;

  sprintf(corr_max, "%d %d",(int)(l_eye_corr->GetCorrMax_x()*S_X+F_tlx), 
                   (int)(l_eye_corr->GetCorrMax_y()*S_Y+F_tly));
  myresult.name="l-eye:0";
  myresult.type="point";
  myresult.value=corr_max;
  AddResultToXML(myresult);

  sprintf(corr_max, "%d %d",(int)(nose_corr->GetCorrMax_x()*S_X+F_tlx), 
                           (int)(nose_corr->GetCorrMax_y()*S_Y+F_tly));

  myresult.name="nose:0";
  myresult.type="point";
  myresult.value=corr_max;
  AddResultToXML(myresult);

  sprintf(corr_max, "%d %d",(int)(mouth_corr->GetCorrMax_x()*S_X+F_tlx), 
                   (int)(mouth_corr->GetCorrMax_y()*S_Y+F_tly));

  myresult.name="mouth:0";
  myresult.type="point";
  myresult.value=corr_max;
  AddResultToXML(myresult);

  if(Iterate) {
  if (Verbose()>1)
    cout << "Saving optimisation results to XML.." << endl;
  // Saving optimisation results (coordinates) to XML
  sprintf(corr_max, "%d %d",
                   (int)(r_eye_corr->GetOptCorrMax_x()*S_X+F_tlx), 
                   (int)(r_eye_corr->GetOptCorrMax_y()*S_Y+F_tly));

  myresult.name="r-eye";
  myresult.type="point";
  myresult.value=corr_max;
  AddResultToXML(myresult);

  sprintf(corr_max, "%d %d",
                   (int)(l_eye_corr->GetOptCorrMax_x()*S_X+F_tlx), 
                   (int)(l_eye_corr->GetOptCorrMax_y()*S_Y+F_tly));
  myresult.name="l-eye";
  myresult.type="point";
  myresult.value=corr_max;
  AddResultToXML(myresult);

  sprintf(corr_max, "%d %d",(int)(nose_corr->GetOptCorrMax_x()*S_X+F_tlx), 
                           (int)(nose_corr->GetOptCorrMax_y()*S_Y+F_tly));

  myresult.name="nose";
  myresult.type="point";
  myresult.value=corr_max;
  AddResultToXML(myresult);

  sprintf(corr_max, "%d %d",
                   (int)(mouth_corr->GetOptCorrMax_x()*S_X+F_tlx), 
                   (int)(mouth_corr->GetOptCorrMax_y()*S_Y+F_tly));

  myresult.name="mouth";
  myresult.type="point";
  myresult.value=corr_max;
  AddResultToXML(myresult);
  }

  delete r_eye_corr;
  delete l_eye_corr;
  delete nose_corr;
  delete mouth_corr;
  delete grad_Ei;
  delete grad_Es;
  delete grad_Etot;

  return ProcessNextMethod();
  */
}

///////////////////////////////////////////////////////////////////////////

float FaceParts::Gradient_x( PixelTemplate *src, int x, int y ) {
// Mi = 0.5*(1-Corr(x,y))
  float M = 0, grad_x = 0;

  // define array for calculating Gradient X
  const int Gx[] = { -1,0,1 , -2,0,2 , -1,0,1 };

  for (int j = -1; j <= 1; j++) {
    for (int i = -1; i <= 1; i++) {
      int xi = x+i, yj = y+j;
      //doublecheck that this is inside limits
      if (xi < 0 || yj < 0 ||
	  xi > (int)src->width()-1 || yj > (int)src->height()-1)
	M = 0;

      else {
	if(src->coordinates_ok(xi,yj))
	  M = src->get_one_float( xi, yj);
	else
	  M=0;

    //     if(!ok) {
//           if(Verbose()) 
//             cout <<"Error: Grad_x GetPixel error: x,i,y,j:"<<x<<" "<<i<<" "<<y<<" "<<j<<endl;
//           M = 0;        // if getpixel exceeds image dimensions
//         }
      }
 
      M = (1-M)/2;          // normalize correlation to [0,1]
      // and calculate gradient_x
      grad_x += (float)M*(float)Gx[ (j+1)*3 +(i+1) ] ; 
    }
  }
  grad_x = (float)grad_x / (float)8;
  return grad_x;
}

///////////////////////////////////////////////////////////////////////////

float FaceParts::Gradient_y( PixelTemplate *src, int x, int y ) {
// Mi = 0.5*(1-Corr(x,y))
  float M = 0, grad_y = 0;

  // define array for calculating Gradient X
  const int Gy[] = { -1,-2,-1 , 0,0,0 , 1,2,1 };

  for (int j = -1; j <= 1; j++) {
    for (int i = -1; i <= 1; i++) {
      int xi = x+i, yj = y+j;
      //doublecheck that this is inside limits
      if (xi < 0 || yj < 0 || xi > (int)src->width()-1 ||
	  yj > (int)src->height()-1)
        M = 0;

      else {
	if(src->coordinates_ok(xi,yj))
	  M = src->get_one_float( xi, yj);
	else
	  M=0;
	   //        ok = src->GetPixelGrey( xi, yj, M );

     //    if(!ok) {
//           if(Verbose())
//             cout <<"Error: x,i,y,j:"<<x<<" "<<i<<" "<<y<<" "<<j<<endl;
//           M = 0;        // if getpixel exceeds image dimensions
//         }
      }

      M = (1-M)/2;          // normalize correlation to [0,1]
      // and calculate gradient_x
      grad_y += (float)M*(float)Gy[ (j+1)*3 +(i+1) ] ; 
    }
  }
  grad_y = (float)grad_y / (float)8;
  return grad_y;
}

///////////////////////////////////////////////////////////////////////////
// E_img calculates image component of total Energy
// will be between 0 .. 1
float FaceParts::E_img(PixelTemplate *s1, PixelTemplate *s2, PixelTemplate *s3,
		       PixelTemplate *s4) {
  float v1,v2,v3,v4;

  v1 = s1->get_one_float( s1->GetOptCorrMax_x(),s1->GetOptCorrMax_y());
  v1 = ((float)1-(float)v1)/(float)2;


  v2 = s2->get_one_float( s2->GetOptCorrMax_x(),s2->GetOptCorrMax_y());
  v2 = ((float)1-(float)v2)/(float)2;

  v3 = s3->get_one_float( s3->GetOptCorrMax_x(),s3->GetOptCorrMax_y());
  v3 = ((float)1-(float)v3)/(float)2;

  v4 = s4->get_one_float( s4->GetOptCorrMax_x(),s4->GetOptCorrMax_y());
  v4 = ((float)1-(float)v4)/(float)2;

  float Eimg = (v1+v2+v3+v4)/(float)4;

  return Eimg;
}
///////////////////////////////////////////////////////////////////////////
// input: 4 correlation images with all pixels between -1 .. +1
// parameters: *re = *r_eye, *le = *l_eye, *no = *nose, *mo = *mouth
float FaceParts::E_struct(PixelTemplate *re, PixelTemplate *le, 
                          PixelTemplate *no, PixelTemplate *mo) {
  int i;
  float xi[4];          // all xi:s (frm re,le,no,mo)
  float yi[4];          // all yi:s (frm re,le,no,mo)

  int DEBUG = 0;

  // define constants for face parts
  const int R_EYE = 0; 
  const int L_EYE = 1; 
  const int NOSE  = 2; 
  const int MOUTH = 3; 

  // define constants for strings
  const int STR1 = 0;
  const int STR2 = 1;
  const int STR3 = 2;
  const int STR4 = 3;
  const int STR5 = 4;
  const int STR6 = 5;

  if(DEBUG)
  if( re->width()!=le->width() || re->width()!=no->width() ||
      re->width()!=mo->width() || le->width()!=no->width() ||
      le->width()!=mo->width() || no->width()!=mo->width() ||
      re->height()!=le->height() || re->height()!=no->height() ||
      re->height()!=mo->height() || le->height()!=no->height() ||
      le->height()!=mo->height() || no->height()!=mo->height() ) {
    cout << "Error: Correlation image dimensions are not the same!"<<endl;
    abort();
  }

  // calculate relative maximum (x,y)i coordinates for each face part
  xi[R_EYE] = (float) re->GetOptCorrMax_x() / (float) re->width();
  xi[L_EYE] = (float) le->GetOptCorrMax_x() / (float) le->width();
  xi[NOSE]  = (float) no->GetOptCorrMax_x() / (float) no->width();
  xi[MOUTH] = (float) mo->GetOptCorrMax_x() / (float) mo->width();

  yi[R_EYE] = (float) re->GetOptCorrMax_y() / (float) re->height();
  yi[L_EYE] = (float) le->GetOptCorrMax_y() / (float) le->height();
  yi[NOSE]  = (float) no->GetOptCorrMax_y() / (float) no->height();
  yi[MOUTH] = (float) mo->GetOptCorrMax_y() / (float) mo->height();


  float *li = new float[6];
  // calculate distances between face parts in relative coordinate system
  // distance |s(i) - s(j)| == sqrt( (x(i)-x(j))^2 + (y(i)-y(j))^2 )
  // dist(re,le) = li[str1]  dist(re,no) = li[str2] dist(re,mo) = li[str3]
  // dist(le,no) = li[str4] dist(le,mo) = li[str5] dist(no,mo) = li[str6]
  li[STR1] = sqrt( (xi[L_EYE]-xi[R_EYE])*(xi[L_EYE]-xi[R_EYE]) 
                  +(yi[L_EYE]-yi[R_EYE])*(yi[L_EYE]-yi[R_EYE]) );
  li[STR2] = sqrt( (xi[NOSE] -xi[R_EYE])*(xi[NOSE] -xi[R_EYE]) 
                  + (yi[NOSE]-yi[R_EYE])*(yi[NOSE]-yi[R_EYE]) );
  li[STR3] = sqrt( (xi[MOUTH]-xi[R_EYE])*(xi[MOUTH]-xi[R_EYE]) 
                  + (yi[MOUTH]-yi[R_EYE])*(yi[MOUTH]-yi[R_EYE]) );
  li[STR4] = sqrt( (xi[NOSE] -xi[L_EYE])*(xi[NOSE] -xi[L_EYE]) 
                  + (yi[NOSE]-yi[L_EYE])*(yi[NOSE]-yi[L_EYE]) );
  li[STR5] = sqrt( (xi[MOUTH]-xi[L_EYE])*(xi[MOUTH]-xi[L_EYE]) 
                  + (yi[MOUTH]-yi[L_EYE])*(yi[MOUTH]-yi[L_EYE]) );
  li[STR6] = sqrt( (xi[MOUTH]-xi[NOSE]) *(xi[MOUTH]-xi[NOSE])  
                  + (yi[MOUTH]-yi[NOSE])*(yi[MOUTH]-yi[NOSE]) );

  // STRING CONSTANTS:
  // these were calculated by the mean of 6 images
  float l[6] = { 0.2776, 0.1839, 0.3129,  0.1948, 0.3258 , 0.1604};


  // Store energies of strings into separate array
  // Strings are named so that: 
  // r_eye <-> r_eye == 1 // position [0] in array
  // r_eye <-> nose  == 2 // pos [1] in array
  // r_eye <-> mouth == 3 // ...
  // l_eye <-> nose  == 4
  // l_eye <-> mouth == 5
  // nose  <-> mouth == 6
  float E_str[6];


  // Calculate energies of strings
  // first string energy
  E_str[STR1] = (li[STR1]-l[STR1]) * (li[STR1]-l[STR1]);
  // second string energy
  E_str[STR2] = (li[STR2]-l[STR2]) * (li[STR2]-l[STR2]);
  // 3rd string energy
  E_str[STR3] = (li[STR3]-l[STR3]) * (li[STR3]-l[STR3]);
  // 4th string energy
  E_str[STR4] = (li[STR4]-l[STR4]) * (li[STR4]-l[STR4]);
  // 5th string energy
  E_str[STR5] = (li[STR5]-l[STR5]) * (li[STR5]-l[STR5]);
  // 6th string energy
  E_str[STR6] = (li[STR6]-l[STR6]) * (li[STR6]-l[STR6]);


  // Finally, calculate E_struct
  float Estruct = 0;
  for(i=0; i<6; i++) {
    Estruct += E_str[i];
  }
  Estruct = (float)Estruct/(float)6;

  delete[] li;
  // now we have E_struct(t) calculated!

  return Estruct;
}

///////////////////////////////////////////////////////////////////////////

float* FaceParts::Gradient_E_struct(PixelTemplate *re, PixelTemplate *le, 
				    PixelTemplate *no, PixelTemplate *mo) {
  int i,j;
  float *xi = new float[4]; // all xi:s (frm re,le,no,mo)
  float *yi = new float[4]; // all yi:s (frm re,le,no,mo)

  int DEBUG = 0;
  // define constants for face parts
  const int R_EYE = 0; 
  const int L_EYE = 1; 
  const int NOSE  = 2; 
  const int MOUTH = 3; 

  // define constants for strings
  const int STR1 = 0;
  const int STR2 = 1;
  const int STR3 = 2;
  const int STR4 = 3;
  const int STR5 = 4;
  const int STR6 = 5;
  const int X = 0;
  const int Y = 1;

  if(DEBUG)
  if( re->width()!=le->width() || re->width()!=no->width() ||
      re->width()!=mo->width() || le->width()!=no->width() ||
      le->width()!=mo->width() || no->width()!=mo->width() ||
      re->height()!=le->height() || re->height()!=no->height() ||
      re->height()!=mo->height() || le->height()!=no->height() ||
      le->height()!=mo->height() || no->height()!=mo->height() ) {
    cout << "Error: Correlation image dimensions are not the same!"<<endl;
    abort();
  }

  // calculate relative maximum (x,y)i coordinates for each face part
  xi[R_EYE] = (float) re->GetOptCorrMax_x() / (float) re->width();
  xi[L_EYE] = (float) le->GetOptCorrMax_x() / (float) le->width();
  xi[NOSE]  = (float) no->GetOptCorrMax_x() / (float) no->width();
  xi[MOUTH] = (float) mo->GetOptCorrMax_x() / (float) mo->width();

  yi[R_EYE] = (float) re->GetOptCorrMax_y() / (float) re->height();
  yi[L_EYE] = (float) le->GetOptCorrMax_y() / (float) le->height();
  yi[NOSE]  = (float) no->GetOptCorrMax_y() / (float) no->height();
  yi[MOUTH] = (float) mo->GetOptCorrMax_y() / (float) mo->height();

  float *li = new float[6];
  // calculate distances between face parts in relative coordinate system
  // distance |s(i) - s(j)| == sqrt( (x(i)-x(j))^2 + (y(i)-y(j))^2 )
  // dist(re,le) = li[str1]  dist(re,no) = li[str2] dist(re,mo) = li[str3]
  // dist(le,no) = li[str4] dist(le,mo) = li[str5] dist(no,mo) = li[str6]
  li[STR1] = sqrt( (xi[L_EYE]-xi[R_EYE])*(xi[L_EYE]-xi[R_EYE]) 
                  + (yi[L_EYE]-yi[R_EYE])*(yi[L_EYE]-yi[R_EYE]) );
  li[STR2] = sqrt( (xi[NOSE] -xi[R_EYE])*(xi[NOSE] -xi[R_EYE]) 
                  + (yi[NOSE]-yi[R_EYE])*(yi[NOSE]-yi[R_EYE]) );
  li[STR3] = sqrt( (xi[MOUTH]-xi[R_EYE])*(xi[MOUTH]-xi[R_EYE]) 
                  + (yi[MOUTH]-yi[R_EYE])*(yi[MOUTH]-yi[R_EYE]) );
  li[STR4] = sqrt( (xi[NOSE] -xi[L_EYE])*(xi[NOSE] -xi[L_EYE]) 
                  + (yi[NOSE]-yi[L_EYE])*(yi[NOSE]-yi[L_EYE]) );
  li[STR5] = sqrt( (xi[MOUTH]-xi[L_EYE])*(xi[MOUTH]-xi[L_EYE]) 
                  + (yi[MOUTH]-yi[L_EYE])*(yi[MOUTH]-yi[L_EYE]) );
  li[STR6] = sqrt( (xi[MOUTH]-xi[NOSE]) *(xi[MOUTH]-xi[NOSE])  
                  + (yi[MOUTH]-yi[NOSE])*(yi[MOUTH]-yi[NOSE]) );

  // following values were roughly estimated from scaled version 
  // of 00000015.png
  // (i.e. 100x100 wide image)
  // l[i][j] == dist(i,j), where i,j E[R_EYE,L_EYE,NOSE,MOUTH]
  // float l[6] = { 0.24,0.16,0.28 , 0.16,0.28 ,0.16 };

  // Following values were roughly estimated from 6 images 
  // (mean calculated)
  // r_eye == 1, l_eye == 2. nose == 3, mouth == 4
  // 1->2 1->3 1->4 2->3 2->4 3->4
  float l[6] = { 0.2776,0.1839,0.3129 , 0.1948,0.3258 , 0.1604};
//  float l[6] = { 27.76,18.39,31.29 , 19.48,32.58 , 16.04};

  // Store energies of strings into separate array
  // Strings are named so that: 
  // r_eye <-> r_eye == 1 // position [0] in array
  // r_eye <-> nose  == 2 // pos [1] in array
  // r_eye <-> mouth == 3 // ...
  // l_eye <-> nose  == 4
  // l_eye <-> mouth == 5
  // nose  <-> mouth == 6
  float grad_li[8][6];

  // Calculate gradients of Li
  // grad Li(1->2) = ( grad_x Li, grad_y Li ) = 
  // ( (x2-x1)/|(x1,y1)-(x2,y2)|, (y2-y1)/|(x1,y1)-(x2,y2)| )

  // String 1
  grad_li[2*R_EYE+X][STR1] = (xi[R_EYE]-xi[L_EYE]) / li[STR1];
  grad_li[2*R_EYE+Y][STR1] = (yi[R_EYE]-yi[L_EYE]) / li[STR1];
  grad_li[2*L_EYE+X][STR1] = -(xi[R_EYE]-xi[L_EYE])/ li[STR1];
  grad_li[2*L_EYE+Y][STR1] = -(yi[R_EYE]-yi[L_EYE])/ li[STR1];
  grad_li[2*NOSE+X][STR1] = 0;
  grad_li[2*NOSE+Y][STR1] = 0;
  grad_li[2*MOUTH+X][STR1] = 0;
  grad_li[2*MOUTH+Y][STR1] = 0;

  // String 2
  grad_li[2*R_EYE+X][STR2] =  (xi[R_EYE]-xi[NOSE]) / li[STR2];
  grad_li[2*R_EYE+Y][STR2] =  (yi[R_EYE]-yi[NOSE]) / li[STR2];
  grad_li[2*L_EYE+X][STR2] = 0;
  grad_li[2*L_EYE+Y][STR2] = 0;
  grad_li[2*NOSE+X][STR2]  = -(xi[R_EYE]-xi[NOSE]) / li[STR2];
  grad_li[2*NOSE+Y][STR2]  = -(yi[R_EYE]-yi[NOSE]) / li[STR2];
  grad_li[2*MOUTH+X][STR2] = 0;
  grad_li[2*MOUTH+Y][STR2] = 0;

  // String 3
  grad_li[2*R_EYE+X][STR3] =  (xi[R_EYE]-xi[MOUTH])/ li[STR3];
  grad_li[2*R_EYE+Y][STR3] =  (yi[R_EYE]-yi[MOUTH])/ li[STR3];
  grad_li[2*L_EYE+X][STR3] = 0;
  grad_li[2*L_EYE+Y][STR3] = 0;
  grad_li[2*NOSE+X][STR3]  = 0;
  grad_li[2*NOSE+Y][STR3]  = 0;
  grad_li[2*MOUTH+X][STR3] = -(xi[R_EYE]-xi[MOUTH])/ li[STR3];
  grad_li[2*MOUTH+Y][STR3] = -(yi[R_EYE]-yi[MOUTH])/ li[STR3];

  // String 4
  grad_li[2*R_EYE+X][STR4] = 0;
  grad_li[2*R_EYE+Y][STR4] = 0;
  grad_li[2*L_EYE+X][STR4] =  (xi[L_EYE]-xi[NOSE])/ li[STR4];
  grad_li[2*L_EYE+Y][STR4] =  (yi[L_EYE]-yi[NOSE])/ li[STR4];
  grad_li[2*NOSE+X][STR4]  = -(xi[L_EYE]-xi[NOSE])/ li[STR4];
  grad_li[2*NOSE+Y][STR4]  = -(yi[L_EYE]-yi[NOSE])/ li[STR4];
  grad_li[2*MOUTH+X][STR4] = 0;
  grad_li[2*MOUTH+Y][STR4] = 0;

  // String 5
  grad_li[2*R_EYE+X][STR5] = 0;
  grad_li[2*R_EYE+Y][STR5] = 0;
  grad_li[2*L_EYE+X][STR5] =  (xi[L_EYE]-xi[MOUTH])/ li[STR5];
  grad_li[2*L_EYE+Y][STR5] =  (yi[L_EYE]-yi[MOUTH])/ li[STR5];
  grad_li[2*NOSE+X][STR5]  = 0;
  grad_li[2*NOSE+Y][STR5]  = 0;
  grad_li[2*MOUTH+X][STR5] = -(xi[L_EYE]-xi[MOUTH])/ li[STR5];
  grad_li[2*MOUTH+Y][STR5] = -(yi[L_EYE]-yi[MOUTH])/ li[STR5];


  // String 6
  grad_li[2*R_EYE+X][STR6] = 0;
  grad_li[2*R_EYE+Y][STR6] = 0;
  grad_li[2*L_EYE+X][STR6] = 0;
  grad_li[2*L_EYE+Y][STR6] = 0;
  grad_li[2*NOSE+X][STR6]  =  (xi[NOSE]-xi[MOUTH])/ li[STR6];
  grad_li[2*NOSE+Y][STR6]  =  (yi[NOSE]-yi[MOUTH])/ li[STR6];
  grad_li[2*MOUTH+X][STR6] = -(xi[NOSE]-xi[MOUTH])/ li[STR6];
  grad_li[2*MOUTH+Y][STR6] = -(yi[NOSE]-yi[MOUTH])/ li[STR6];

  float *grad_E_str = new float[8];
  for(i=0; i<8; i++) grad_E_str[i] = 0;

  // calculating gradient E_struct
  for( i = 0; i < 8; i++ ) {
    for( j = 0; j < 6; j++ ) {
      grad_E_str[i] += (float)2*(float)(li[j]-l[j])*(float)grad_li[i][j];
    }
  }

  // for convenience 1/6 * grad_E_struct is in its own loop in here
  for( i = 0; i < 8; i++) {
    grad_E_str[i] = (float)grad_E_str[i] / (float)6;
  }

  // now we have E_struct(t) calculated!

  delete[] li;
  delete[] xi;
  delete[] yi;

  // == Grad_E_struct
  return grad_E_str;
}

///////////////////////////////////////////////////////////////////////////

float* FaceParts::Gradient_E_img(PixelTemplate *s1, PixelTemplate *s2, 
				 PixelTemplate *s3, PixelTemplate *s4) {
  float *grad_M = new float[8];

  grad_M[0] = Gradient_x(s1, s1->GetOptCorrMax_x(),s1->GetOptCorrMax_y());
  grad_M[1] = Gradient_y(s1, s1->GetOptCorrMax_x(),s1->GetOptCorrMax_y());

  grad_M[2] = Gradient_x(s2, s2->GetOptCorrMax_x(),s2->GetOptCorrMax_y());
  grad_M[3] = Gradient_y(s2, s2->GetOptCorrMax_x(),s2->GetOptCorrMax_y());

  grad_M[4] = Gradient_x(s3, s3->GetOptCorrMax_x(),s3->GetOptCorrMax_y());
  grad_M[5] = Gradient_y(s3, s3->GetOptCorrMax_x(),s3->GetOptCorrMax_y());

  grad_M[6] = Gradient_x(s4, s4->GetOptCorrMax_x(),s4->GetOptCorrMax_y());
  grad_M[7] = Gradient_y(s4, s4->GetOptCorrMax_x(),s4->GetOptCorrMax_y());

  // (Grad)E_img = 1/4*(Grad)Mi(xi(t),yi(t))
  for( int i=0; i<8; i++)
    grad_M[i] = (float)grad_M[i] / (float)4;

  return grad_M;
}

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

}

// Local Variables:
// mode: font-lock
// End:
