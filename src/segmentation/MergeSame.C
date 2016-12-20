#include <MergeSame.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::map;

static const string vcid =
"@(#)$Id: MergeSame.C,v 1.2 2009/04/30 09:58:26 vvi Exp $";
namespace picsom {
  static MergeSame list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  MergeSame::MergeSame() {
    colour_tolerance=0.05;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  MergeSame::~MergeSame() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *MergeSame::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void MergeSame::UsageInfo(ostream& os) const { 
    os << "MergeSame :" << endl
       << "  options : " << endl
       << "    -c <colour tolerance>       (deafult: 0.05)" << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int MergeSame::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'c':
	if (argc<3){
	  throw string("MergeSame: Not enough commandline arguments for switch -c");
	  break;
	}
	
	option.option="-c";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%f",&(MergeSame::colour_tolerance)) != 1)
	  throw string("MergeSame: switch -c requires an integer specifier.");
	
	argc--;
	argv++;
	break;

default:
	throw string("MergeSame::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);
    
      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;
  
  }
  ///////////////////////////////////////////////////////////////////////////

  bool MergeSame::Process(){

    int s;
    map<int,int> newlabel;
    map<int,RGBTriple> avgs;
    map<int,int> counts;

    vector<float> rgb;

    segmentfile *sf=getImg();

    for (int y=0; y<sf->getHeight(); y++)
      for(int x=0; x<sf->getWidth(); x++) {
	sf->get_pixel_segment(x,y,s);
	sf->get_pixel_rgb(x,y,rgb);

	counts[s]++;
	avgs[s].r(avgs[s].r()+rgb[0]);
	avgs[s].g(avgs[s].g()+rgb[1]);
	avgs[s].b(avgs[s].b()+rgb[2]);
      }

    map<int,RGBTriple>::iterator rgbIt;
    for(rgbIt=avgs.begin();rgbIt!=avgs.end();rgbIt++){
      rgbIt->second.r(rgbIt->second.r()/counts[rgbIt->first]);
      rgbIt->second.g(rgbIt->second.g()/counts[rgbIt->first]);
      rgbIt->second.b(rgbIt->second.b()/counts[rgbIt->first]);

      //      cout << "avg[" << rgbIt->first << "] : ("
      //	   << rgbIt->second.r() << " "
      //	   << rgbIt->second.g() << " "
      //	   << rgbIt->second.b() << ")" << endl;
    }

    for(rgbIt=avgs.begin();rgbIt!=avgs.end();rgbIt++){
      map<int,RGBTriple>::iterator rgbIt2=rgbIt;
      for(rgbIt2++;rgbIt2!=avgs.end();rgbIt2++)
	if(newlabel.count(rgbIt2->first)==0 &&
	   rgbIt->second.SqrDist(rgbIt2->second) < colour_tolerance)
	  newlabel[rgbIt2->first]=rgbIt->first;
    }
	  
    for (int y=0; y<sf->getHeight(); y++)
      for(int x=0; x<sf->getWidth(); x++) {
	sf->get_pixel_segment(x,y,s);
	if(newlabel.count(s))
	  sf->set_pixel_segment(x,y,newlabel[s]);
      }

    return ProcessNextMethod();

  }  
       
  ///////////////////////////////////////////////////////////////////////////
}

// Local Variables:
// mode: font-lock
// End:



