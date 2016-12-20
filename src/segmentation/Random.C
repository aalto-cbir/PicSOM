#include <Random.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::list;

static const string vcid =
"@(#)$Id: Random.C,v 1.2 2009/04/30 10:04:48 vvi Exp $";

namespace picsom{
  static Random list_entry(true);

  ///////////////////////////////////////////////////////////////////////////
  
  Random::Random() {
    numberOfMeans=5;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  Random::~Random() {
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  const char *Random::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void Random::UsageInfo(ostream& os) const { 
    os << "Random :" << endl
       << "  options : " << endl
       << "    -k <# of segments> (default: 5)" << endl
       << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int Random::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'k':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -k");
	  break;
	}
	
	option.option = "-k";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(Random::numberOfMeans)) != 1)
	  throw string("switch -k requires an integer specifier.");
	
	argc--;
	argv++;
	break;
	
      default:
	throw string("unknown option ")+argv[0];
      } // switch

      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
    
    return argc_old-argc;
    
  }
  
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool Random::Process()
  {
    
    // assign labels randomly

    int x,y;

    segmentfile *s = getImg();
    int h=Height();
    int w=Width();
    
    for(y=0;y<h;y++)
      for(x=0;x<w;x++){
	s->set_pixel_segment(x,y,rand() % numberOfMeans);
      }
    
    return ProcessNextMethod();

  }
}
  ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
  
