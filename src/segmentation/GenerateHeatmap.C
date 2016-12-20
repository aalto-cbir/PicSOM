// -*- C++ -*-  $Id: GenerateHeatmap.C,v 1.3 2009/04/30 12:34:59 vvi Exp $
// 
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <GenerateHeatmap.h>
#include <math.h>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::list;

namespace picsom{
  static const string vcid =
    "@(#)$Id: GenerateHeatmap.C,v 1.3 2009/04/30 12:34:59 vvi Exp $";

  static GenerateHeatmap list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  GenerateHeatmap::GenerateHeatmap() {
    no_interpolation = false;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  GenerateHeatmap::~GenerateHeatmap() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *GenerateHeatmap::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void GenerateHeatmap::UsageInfo(ostream& os) const { 
    os << "GenerateHeatmap :" << endl
       << "  options : " << endl
       << "    -s <score file>   "   
       << endl
       << "    -n no interpolation   "   
       << endl
       << "    -o <filename>|<pattern>  Specifies output filename(s)." 
       << endl;
    
  }

  ///////////////////////////////////////////////////////////////////////////
  
  int GenerateHeatmap::ProcessOptions(int argc, char** argv) { 
    Option option;
    int argc_old=argc;
    string tmpstr;
  
    SegmentationResult result;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
	
      case 's':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -s");
	  break;
	}
	
	option.option = argv[0];
	option.addArgument(argv[1]);

	scorefilename=argv[1];

	argc--;
	argv++;
	break;
      
      case 'o':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -o");
	break;
      }

      option.option = "-o";
      option.addArgument(argv[1]);

      SetResultName(argv[1]);

      argc--;
      argv++;
      break;

      case 'n':

      option.option = "-n";
      no_interpolation=true;
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

///////////////////////////////////////////////////////////////////////////

  bool GenerateHeatmap::OpenExtraFiles() {
    result_filename = FormResultName();
    
    if (!result_filename.length())
      throw string("Empty result_filename in GenerateHeatmap::InitialiseDr().");
    resultfile = new imagefile(result_filename,true,"image/tiff");

    AddToResultFiles(result_filename);
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool GenerateHeatmap::CloseExtraFiles(){

    if (Verbose()>1)
      cout << "Writing "<<resultfile->nframes()<<" frames to file "
	   <<result_filename<<endl;
    
    if (!resultfile)
      return false;

    resultfile->write();
    delete resultfile;
    resultfile=NULL;

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void GenerateHeatmap::AddResultFrame(imagedata& d) {
    if (resultfile) 
      resultfile->add_frame(d);
  }

  ///////////////////////////////////////////////////////////////////////////

 
  bool GenerateHeatmap::Process() {

    resultframe=getImg()->getEmptyFrame();

    // attempt to read the score file

    map<int,float> tilescores;

    FILE *f=fopen(scorefilename.c_str(),"r");
    if(!f) throw string("Couldn't open score file ") + scorefilename +
      " for reading";

    char tile[200];
    float s;
    int idx;

    while(!feof(f)){
      if(fscanf(f,"%s %f",tile,&s)==2){
	size_t offset=0;
	while(tile[offset]&&tile[offset]!='_') offset++;
	if(tile[offset]=='_' && sscanf(tile+offset+1,"%d",&idx)==1){
	  tilescores[idx]=s;
	}
      }
    }

    fclose(f);

    cout << "read scores for " << tilescores.size() << " tiles." << endl;

    // currently works only for regions specified in XML

    bool regionsInXML=
      (getImg()->accessLastFrameResultNode("","region"))?true:false;

    if(regionsInXML==false)
      throw string("GenerateHeatmap currently implemented for only regions specified in xml");
    

    int w=getImg()->getWidth(), h=getImg()->getHeight();

    float *weight=new float[w*h];
    float *val=new float[w*h];

    for(size_t i=0;i<(size_t)w*h;i++)
      weight[i]=val[i]=0;


    SegmentationResultList *rl=
      getImg()->readFrameResultsFromXML("","region");

    size_t tilecount=rl->size();

    float a=0.6;

    float sigmasqr=a*(w*h);
    sigmasqr /= tilecount;
    float sigma=sqrt(sigmasqr);

    for(size_t r=0;r<rl->size();r++){
      Region *rptr = Region::createRegion((*rl)[r].value);
      if(rptr==NULL)
	throw string("parseAndCreate() region failed");
      coordList l=rptr->listCoordinates();

      if(no_interpolation==false){
	// find the centroid of the region

	float avex=0,avey=0;
	for(size_t i=0;i<l.size();i++){
	  avex += l[i].x;
	  avey += l[i].y;
	}

	avex /= l.size();
	avey /= l.size();

	cout << "tile " << r+1 << " score " << tilescores[r+1] << "avex="
	     <<avex<<" avey="<<avey<<endl;
	
	for(int x=(int)(avex-3*sigma);x<(int)(avex+3*sigma);x++)
	  for(int y=(int)(avey-3*sigma);y<(int)(avey+3*sigma);y++){
	    if(x<0 || y<0 || x>=w || y>= h) continue;
	    
	    float dx=x-avex;
	    float dy=y-avey;
	    float dsqr=dx*dx+dy*dy;
	    
	    if(dsqr>9*sigmasqr) continue;
	    
	    size_t idx=x+w*y;
	    
	    float we=exp(-0.5*dsqr/sigmasqr);
	    
	    val[idx] += we*fabs(tilescores[r+1]);
	    weight[idx] += we;
	  }
      } else {
	for(size_t i=0;i<l.size();i++){
	  if(l[i].x<0 || l[i].y<0 || l[i].x>=w || l[i].y>= h) continue;
	  size_t idx=l[i].x+w*l[i].y;
	  val[idx] += fabs(tilescores[r+1]);
	  weight[idx] += 1.0;
	}
      }
      delete rptr;
    }
    delete rl;

    // normalise scores by sum of weights

    float maxnorm=0;

    for(size_t i=0;i<(size_t)w*h;i++)
      if(weight[i]>0){
	val[i] /= weight[i];
	if(val[i]>maxnorm) maxnorm=val[i];
      } else
	val[i]=0;

    // normalise maximum score to 1.0

    for(size_t i=0;i<(size_t)w*h;i++)
      val[i] /= maxnorm;
    
    // transfer values to result frame

    for(int x=0;x<w;x++)
      for(int y=0;y<h;y++){
	float v=val[x+y*w];
	vector<float> vec(3,v);
	resultframe->set(x,y,vec);
      }
  
    AddResultFrame(*resultframe);
    delete resultframe;
    resultframe=NULL;

    return ProcessNextMethod();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  string GenerateHeatmap::FormResultName() {
    string pattern = result_pattern.length()?result_pattern:"heatmap-%i.tiff";
    string filename = *GetInputImageName();
    if(filename.empty())
      filename=*GetInputSegmentName();
    return getImg()->FormOutputFileName(pattern,filename,
					FirstMethod()->ChainedMethodName());
  }
  
  ///////////////////////////////////////////////////////////////////////////

   
 


}
