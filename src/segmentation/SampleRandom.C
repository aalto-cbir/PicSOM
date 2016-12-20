#include <SampleRandom.h>
#include <time.h>

using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;

namespace picsom {
  static const string vcid="@(#)$Id: SampleRandom.C,v 1.2 2009/04/30 10:11:41 vvi Exp $";

  static SampleRandom list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  SampleRandom::SampleRandom() {
    numberOfRegions=5;
    xSize=ySize=10;
    xFrac=yFrac=0.0;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  SampleRandom::~SampleRandom() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *SampleRandom::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void SampleRandom::UsageInfo(ostream& os) const { 
    os << "SampleRandom :" << endl
       << "  options : " << endl
       << "    -r <desired # of regions> (default: 5)" << endl
       << "    -s <width>[:<height>] size of the regions (default: 10:width)"
       << "    -f <xfrac>[:<yfrac>] region size as fraction of <w,h> (default: 0:xfrc, 0=not used)"
       << endl;

  }

  /////////////////////////////////////////////////////////////////////////////

  int SampleRandom::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;


    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'r':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -r");
	  break;
	}

	option.option = "-r";
	option.addArgument(argv[1]);

	if(sscanf(argv[1],"%d",&numberOfRegions) != 1)
	  EchoError("switch -r requires an integer specifier.");

	argc--;
	argv++;
	break;

      case 's':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -s");
	  break;
	}
	option.option = "-s";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&xSize) != 1)
	  throw string("switch -s requires an integer specifier.");

	if(strchr(argv[1],':')){
	  if(sscanf(strchr(argv[1],':')+1,"%d",&ySize) != 1)
	    throw string("switch -s requires an integer specifier after :.");
	}
	else ySize=xSize;

      case 'f':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -f");
	  break;
	}
	option.option = "-s";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%f",&xFrac) != 1)
	  throw string("switch -f requires a float specifier.");

	if(strchr(argv[1],':')){
	  if(sscanf(strchr(argv[1],':')+1,"%f",&yFrac) != 1)
	    throw string("switch -f requires a float specifier after :.");
	}
	else yFrac=xFrac;
	
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
  
  bool SampleRandom::Process() {

    int xSz,ySz;

    if(xFrac>0 && yFrac>0 && xFrac <= 1 && yFrac <=1){
      xSz=(int)(xFrac*Width());
      ySz=(int)(yFrac*Height());
    }
    else{
      xSz=(xSize>Width())?Width():xSize;
      ySz=(ySize>Height())?Height():ySize;
    }

    char valstr[80],lblstr[80];

    clock_t c=clock();
    struct tm *t=gmtime(&c);

    srand(t->tm_sec+60*t->tm_min+3600*t->tm_hour);


    for(int i=0;i<numberOfRegions;i++){

      double r=rand();
	r /= RAND_MAX+1.0; // scale r to interval [0,1)
      r *= (Width()-xSz);

      int xStart= (int)r;

      r=rand();
      r /= RAND_MAX+1.0; // scale r to interval [0,1)
      r *= (Height()-ySz);

      int yStart= (int)r;

      sprintf(lblstr,"SampleRandom%d",i);
      sprintf(valstr,"rect:%d,%d,%d%d",xStart,yStart,
	      xStart+xSz-1,yStart+ySz-1);

      SegmentationResult segresult;
      segresult.name=lblstr;
      segresult.type="region";
      segresult.value=valstr;
    
      getImg()->writeFrameResultToXML(this,segresult);
    }

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
