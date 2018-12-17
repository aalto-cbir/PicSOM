// -*- C++ -*-	$Id: OCVface.C,v 1.24 2018/12/16 09:35:49 jormal Exp $

#include <Util.h>

#ifdef HAVE_OPENCV2_CORE_CORE_HPP
#include <OCVface.h>
#include <opencv2/core/core.hpp>

// needed for foo() :
#include <imagefile.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#ifdef _EiC
#define WIN32
#endif

using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;

// a piece that used to be in PicSOM.C:

static void foo() {
  picsom::imagefile imgf("/share/imagedb/picsom/databases/pointing04/originals/Personne01/personne01246+0+0.jpg");
  picsom::imagedata img = imgf.frame(0);
  cv::Mat inFrame = *(cv::Mat*)imgf.impl_data();
  cv::Mat greyscale;
  cv::cvtColor(inFrame, greyscale, CV_BGR2GRAY);
  
  string cascadeFilename = "/share/imagedb/picsom/linux64/share/OpenCV/haarcascades/haarcascade_frontalface_alt2.xml";
  cv::CascadeClassifier classifier = cv::CascadeClassifier(cascadeFilename);
  vector<cv::Rect> facelist;
  float scaleFactor = 1.2;
  int minNeighbours = 2;
  
  classifier.detectMultiScale(greyscale, facelist, scaleFactor,
			      minNeighbours, 
			      CV_HAAR_DO_CANNY_PRUNING, 
			      cv::Size(greyscale.size().width/16, 
				       greyscale.size().height/16));
  if (true==false)
    foo();
}

namespace picsom {
  static const string vcid =
    "@(#)$Id: OCVface.C,v 1.24 2018/12/16 09:35:49 jormal Exp $";

  static OCVface list_entry(true);

  static vector<CascadeClassifier> cascades;
  
  /////////////////////////////////////////////////////////////////////////////

  OCVface::OCVface() {
    includeImages = only_biggest = false;

    overlap_threshold = 0.5;
    scale = 1.3;

    face_scalefactor = 1.1;
    face_min_neighbors = 3;
    face_min_width = 20;
    face_min_height = 20;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  OCVface::~OCVface() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *OCVface::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void OCVface::UsageInfo(ostream& os) const { 
    os << "OCVface :" << endl
       << "  options : " << endl
       << "  -b          : Only the biggest face is detected" << endl
       << "  -c cascades : Used cascades (e.g. \"f\", \"p\", \"f+p\") " << endl
       << "  -s val      : Used scale (default val: 1.3)" << endl
       << "  -t val      : Overlap threshold (default val: 0.5)" << endl
       << "  -p A,B,C,D  : Face detector parameters as: "
       <<                  "A=scalefactor,B=min_neighbors,"
       <<                  "C=min_width,D=min_height." << endl 
       << "                Defaults values are: 1.1,3,20,20" << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  int OCVface::ProcessOptions(int argc, char** argv) {
    //
    // these options have been copied from MyMethod and are useless...
    //
 
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {

      case 'b':
	only_biggest = true;
	break;

      case 'c':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -c");
	  break;
	}

	option.option="-c";
	option.addArgument(argv[1]);

	while (strlen(argv[1])) {
	  switch (argv[1][0]) {
	  case 'p': 
	    used_cascades.push_back(cascade_profile);
	    break;
	  case 'f': 
	    used_cascades.push_back(cascade_frontal);
	    break;
	  default:
	    EchoError(string("unknown cascade ")+argv[1][0]);	  
	  }
	  argv[1]++;
	  if (strlen(argv[1]) && argv[1][0] == '+')
	    argv[1]++;
	}
		     
	argc--;
	argv++;
	break;

      case 's':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -s");
	  break;
	}
	
	option.option="-s";
	option.addArgument(argv[1]);

	scale = atof(argv[1]);

	if (Verbose()>1) 
	  cout << "Using scale " << scale << endl;
	
	argc--;
	argv++;
	break;
	
      case 't':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -t");
	  break;
	}
	
	option.option="-t";
	option.addArgument(argv[1]);

	overlap_threshold = atof(argv[1]);

	if (Verbose()>1) 
	  cout << "Using overlap threshold " << overlap_threshold << endl;
	
	argc--;
	argv++;
	break;

      case 'p':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -p");
	  break;
	}
	
	option.option="-p";
	option.addArgument(argv[1]);

	{
	  vector<string> pv = SplitInCommasObeyParentheses(argv[1]);
	  if (pv.size()!=4) {
	    EchoError("Cannot parse parameters for switch -p");
	    break;
	  }
	  face_scalefactor   = atof(pv[0].c_str());
	  face_min_neighbors = atoi(pv[1].c_str());
	  face_min_width     = atoi(pv[2].c_str());
	  face_min_height    = atoi(pv[3].c_str());
	}

	if (Verbose()>1)
	  cout << "Using face detector parameters:" << endl
	       << "face_scalefactor=" << face_scalefactor
	       << ", face_min_neighbors=" << face_min_neighbors
	       << ", face_min_width=" << face_min_width
	       << ", face_min_height=" << face_min_height << endl;

	argc--;
	argv++;
	break;

      default:
	EchoError(string("OCVface: unknown option ")+argv[0]);
      } // switch

      if (!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while

    return argc_old-argc;
  }

  /////////////////////////////////////////////////////////////////////////////

  IplImage *OCVface::CreateIplImage() const {
    imagedata tmp = *getImg()->accessFrame();
    tmp.convert(imagedata::pixeldata_uchar);

    if (Verbose()>3) {
      imagefile::display(tmp);
    }

    // tmp.force_one_channel();

    IplImage *img = cvCreateImage(cvSize(tmp.width(), tmp.height()),
				  8, tmp.count());

    vector<unsigned char> vec = tmp.get_uchar();
    if (tmp.count()==3)
      for (size_t i=0; i<vec.size(); i+=3) {
	unsigned char v = vec[i];
	vec[i]   = vec[i+2];
	vec[i+2] = v;
      }

    if (Verbose()>2) {
      cout << tmp.count() << " " << tmp.width() << " " << img->width
	   << " " << img->widthStep << endl;
    }

    for (size_t y=0; y<tmp.height(); y++)
      memcpy(img->imageData+y*img->widthStep,
	     &vec[y*tmp.width()*tmp.count()],
	     tmp.width()*tmp.count());

    return img;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  bool OCVface::Process() {
    bool has_show_image = false;

    if (used_cascades.size() == 0)
      used_cascades.push_back(cascade_frontal);

    if (Verbose()>2) {
      ShowLinks();
      for (unsigned int i=0; i<used_cascades.size(); i++)
	cout << "Using cascade " << i << ": " << CascadeName(used_cascades[i]) 
	     << endl;
    }

    if (cascades.size() == 0)       
      for (unsigned int i=0; i<used_cascades.size(); i++) {
	string cascade_name = CascadePath() + "/" +
	  CascadeName(used_cascades[i]);
	if (Verbose()>2)
	  cout << "Loading cascade " << cascade_name << endl;

	CascadeClassifier cc;
	if (!cc.load(cascade_name)) {
	    cerr << "OCVface : opening <" << cascade_name << "> failed" << endl;
	    return false;
	  }
	  
	cascades.push_back(cc);	
      }

    string filename = *GetInputImageName();

    // IplImage *img = cvLoadImage( filename.c_str(), 1 );
    IplImage *img = CreateIplImage();

    if (!img)  {
      cerr << "OCVface : opening <" << filename << "> failed" << endl;
      return false;
    }

    if (has_show_image) {
      if (Verbose()>2)
	cvNamedWindow("result", 1);

      if (Verbose()>3) {
	cvShowImage("result", img);
	cvWaitKey(0);
      }
    }

    // Mat imgMat(img);
    Mat imgMat = cvarrToMat(img);
    
    Mat gray, small_img(cvRound(imgMat.rows/scale), cvRound(imgMat.cols/scale), 
			CV_8UC1);
    cvtColor(imgMat, gray, CV_BGR2GRAY);
    resize(gray, small_img, small_img.size(), 0, 0, INTER_LINEAR);
    equalizeHist(small_img, small_img);

    vector<Rect> allfaces;

    int flags = CV_HAAR_SCALE_IMAGE;
    if (only_biggest)
      flags += CV_HAAR_FIND_BIGGEST_OBJECT;

    //CV_HAAR_DO_ROUGH_SEARCH

    for (unsigned int c=0; c<cascades.size(); c++) {
      vector<Rect> faces;
      if (Verbose()>1)
	cout << "Running cascade " << c << endl;

      double t = (double)cvGetTickCount();
      cascades[c].detectMultiScale(small_img, faces, face_scalefactor,
				   face_min_neighbors, flags,
				   Size(face_min_width, face_min_height));
      t = (double)cvGetTickCount() - t;
      if (Verbose()>1)
	printf ("Found %d faces, detection time = %g ms\n", (int)faces.size(), 
		t/((double)cvGetTickFrequency()*1000.));

      vector<Rect>::const_iterator r;
      for (r = faces.begin(); r != faces.end(); r++) {

	int tlx = int(r->x*scale);
	int tly = int(r->y*scale);
	int brx = int((r->x+r->width)*scale);
	int bry = int((r->y+r->height)*scale);

	if (MaxOverlap(*r, allfaces) > overlap_threshold) {
	  if (Verbose()>1)
	    cout << "Overlapping face found with cascade "
		 << CascadeName(used_cascades[c])
		 << " (" << tlx << " " << tly << " "
		 << brx << " " << bry << ")"
		 << endl;

	} else {
	  stringstream ss1, ss2;
	  ss1 << ResultName(used_cascades[c]) << allfaces.size();
	  ss2 << tlx << " " << tly << " " << brx << " " << bry;
	  	  
	  SegmentationResult myresult;
	  myresult.name  = ss1.str();
	  myresult.type  = "box";
	  myresult.value = ss2.str();
	  
	  if (Verbose()>1)
	    cout << "  " << myresult.Str() << endl;

	  if (Verbose()>2)
	    rectangle(imgMat, Rect(tlx,tly,brx-tlx,bry-tly), 
		      CV_RGB(0,0,255), 3, 8, 0);

	  getImg()->writeFrameResultToXML(this, myresult);

	  allfaces.push_back(*r);
	}
      }
    }    

    if (has_show_image) {
      if (Verbose()>2) {    
	cvShowImage("result", img);
	cvWaitKey(0);
	cvDestroyWindow("result"); 
      }
    }

    cvReleaseImage(&img);

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

  double OCVface::MaxOverlap(const Rect &r, const vector<Rect> &all) {
    double max_overlap = 0.0;
    for (unsigned int i=0; i<all.size(); i++) {
      double o = Overlap(r, all[i]);
      if (o > max_overlap) 
	max_overlap = o;
    }
    return max_overlap;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  double OCVface::Overlap(const Rect &r1, const Rect &r2) {
    int r1ax = r1.x; int r1bx = r1.x+r1.width; 
    int r1ay = r1.y; int r1by = r1.y+r1.height;
    int r2ax = r2.x; int r2bx = r2.x+r2.width; 
    int r2ay = r2.y; int r2by = r2.y+r2.height;

    int r1size = (r1bx-r1ax)*(r1by-r1ay); 
    int r2size = (r2bx-r2ax)*(r2by-r2ay);

    int rax = MAX(r1ax,r2ax); int ray = MAX(r1ay,r2ay);
    int rbx = MIN(r1bx,r2bx); int rby = MIN(r1by,r2by);

    int rsize = 0;
    if (rbx>rax && rby-ray)
      rsize = (rbx-rax)*(rby-ray);

    double res = (double)rsize / MAX(r1size,r2size);

    if (Verbose()>2) {
      cout << "Calculating overlap of (scaled images) [" 
	   << r1ax << " " << r1ay << " " << r1bx << " " << r1by << "] and [" 
	   << r2ax << " " << r2ay << " " << r2bx << " " << r2by << "]" << endl
	   << " Result: " << res << endl;       
    }
    return res;
  }

  /////////////////////////////////////////////////////////////////////////////


} // namespace picsom

// static CvMemStorage* storage = 0;
// static CvHaarClassifierCascade* cascade = 0;

const char* cascade_name =
    "haarcascade_frontalface_alt.xml";
/*    "haarcascade_profileface.xml";*/

#if 0
int ocv_main( int argc, char** argv )
{
    CvCapture* capture = 0;
    IplImage *frame, *frame_copy = 0;
    int optlen = strlen("--cascade=");
    const char* input_name;

    if( argc > 1 && strncmp( argv[1], "--cascade=", optlen ) == 0 )
    {
        cascade_name = argv[1] + optlen;
        input_name = argc > 2 ? argv[2] : 0;
    }
    else
    {
        cascade_name = "../../data/haarcascades/haarcascade_frontalface_alt2.xml";
        input_name = argc > 1 ? argv[1] : 0;
    }

    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    
    if( !cascade )
    {
        fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
        fprintf( stderr,
        "Usage: facedetect --cascade=\"<cascade_path>\" [filename|camera_index]\n" );
        return -1;
    }
    storage = cvCreateMemStorage(0);
    
    if( !input_name || (isdigit(input_name[0]) && input_name[1] == '\0') )
        capture = cvCaptureFromCAM( !input_name ? 0 : input_name[0] - '0' );
    else
        capture = cvCaptureFromAVI( input_name ); 

    cvNamedWindow( "result", 1 );

    if( capture )
    {
        for(;;)
        {
            if( !cvGrabFrame( capture ))
                break;
            frame = cvRetrieveFrame( capture );
            if( !frame )
                break;
            if( !frame_copy )
                frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
                                            IPL_DEPTH_8U, frame->nChannels );
            if( frame->origin == IPL_ORIGIN_TL )
                cvCopy( frame, frame_copy, 0 );
            else
                cvFlip( frame, frame_copy, 0 );
            
            detect_and_draw( frame_copy );

            if( cvWaitKey( 10 ) >= 0 )
                break;
        }

        cvReleaseImage( &frame_copy );
        cvReleaseCapture( &capture );
    }
    else
    {
        const char* filename = input_name ? input_name : (char*)"lena.jpg";
        IplImage* image = cvLoadImage( filename, 1 );

        if( image )
        {
            detect_and_draw( image );
            cvWaitKey(0);
            cvReleaseImage( &image );
        }
        else
        {
            /* assume it is a text file containing the
               list of the image filenames to be processed - one per line */
            FILE* f = fopen( filename, "rt" );
            if( f )
            {
                char buf[1000+1];
                while( fgets( buf, 1000, f ) )
                {
                    int len = (int)strlen(buf);
                    while( len > 0 && isspace(buf[len-1]) )
                        len--;
                    buf[len] = '\0';
                    image = cvLoadImage( buf, 1 );
                    if( image )
                    {
                        detect_and_draw( image );
                        cvWaitKey(0);
                        cvReleaseImage( &image );
                    }
                }
                fclose(f);
            }
        }

    }
    
    cvDestroyWindow("result");

    return 0;
}

void detect_and_draw( IplImage* img,  CvHaarClassifierCascade *cascade,
		      CvMemStorage *storage)
{
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    double scale = 1.3;
    IplImage* gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
    IplImage* small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
                         cvRound (img->height/scale)),
                     8, 1 );
    int i;

    cvCvtColor( img, gray, CV_BGR2GRAY );
    cvResize( gray, small_img, CV_INTER_LINEAR );
    cvEqualizeHist( small_img, small_img );
    cvClearMemStorage( storage );

    if( cascade )
    {
        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
                                            1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                            cvSize(30, 30) );
        t = (double)cvGetTickCount() - t;
        printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );
        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
            cvCircle( img, center, radius, colors[i%8], 3, 8, 0 );
        }
    }

    cvShowImage( "result", img );
    cvReleaseImage( &gray );
    cvReleaseImage( &small_img );
}
#endif // 0

#endif // HAVE_OPENCV2_CORE_CORE_HPP

///////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: font-lock
// End:
