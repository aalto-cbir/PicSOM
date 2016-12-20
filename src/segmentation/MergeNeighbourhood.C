#include <MergeNeighbourhood.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::map;

static const string vcid =
"@(#)$Id: MergeNeighbourhood.C,v 1.2 2009/04/30 09:57:55 vvi Exp $";
namespace picsom {
  static MergeNeighbourhood list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  MergeNeighbourhood::MergeNeighbourhood() {
    nbr_radius=2;
    colour_tolerance=0.05;
    size_threshold=-1;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  MergeNeighbourhood::~MergeNeighbourhood() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *MergeNeighbourhood::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void MergeNeighbourhood::UsageInfo(ostream& os) const { 
    os << "MergeNeighbourhood :" << endl
       << "  options : " << endl
       << "    -r <neighbourhood radius>   (default: 2)" << endl
       << "    -c <colour tolerance>       (deafult: 0.05)" << endl
       << "    -s <size threshold>         (deafult: 3*radius)" << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int MergeNeighbourhood::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'r':
	if (argc<3){
	  throw string("MergeNeighbourhood: Not enough commandline arguments for switch -r");
	  break;
	}
	
	option.option="-r";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(MergeNeighbourhood::nbr_radius)) != 1)
	  throw string("MergeNeighbourhood: switch -r requires an integer specifier.");
	
	argc--;
	argv++;
	break;

      case 'c':
	if (argc<3){
	  throw string("MergeNeighbourhood: Not enough commandline arguments for switch -c");
	  break;
	}
	
	option.option="-c";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%f",&(MergeNeighbourhood::colour_tolerance)) != 1)
	  throw string("MergeNeighbourhood: switch -c requires an integer specifier.");
	
	argc--;
	argv++;
	break;

      case 's':
	if (argc<3){
	  throw string("MergeNeighbourhood: Not enough commandline arguments for switch -s");
	  break;
	}
	
	option.option="-s";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(MergeNeighbourhood::size_threshold)) != 1)
	  throw string("MergeNeighbourhood: switch -s requires an integer specifier.");
	
	argc--;
	argv++;
	break;



default:
	throw string("MergeNeighbourhood::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);
    
      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;
  
  }
  ///////////////////////////////////////////////////////////////////////////

  bool MergeNeighbourhood::Process(){

    int s;
    map<int,int> accu,newlabel;
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

      cout << "avg[" << rgbIt->first << "] : ("
	   << rgbIt->second.r() << " "
	   << rgbIt->second.g() << " "
	   << rgbIt->second.b() << ")" << endl;
    }

    if(size_threshold < 0) size_threshold = 3 * nbr_radius;

    for (int y=0; y<sf->getHeight(); y+=2*nbr_radius)
      for(int x=0; x<sf->getWidth(); x+=2*nbr_radius) {
	accu.clear();
	for(int dx=-nbr_radius;dx<=nbr_radius;dx++)
	  for(int dy=-nbr_radius;dy<=nbr_radius;dy++)
	    if(sf->coordinates_ok(x+dx,y+dy)){
	      sf->get_pixel_segment(x,y,s);
	      accu[s]++;
	    }

	newlabel.clear();
	map<int,int>::const_iterator it;
	for(it=accu.begin();it!=accu.end();it++)
	  if(it->second <= size_threshold){ 
	    map<int,int>::const_iterator it2;
	    for(it2=accu.begin();it2!=accu.end();it2++)
	      if(it!=it2 && it2->second > size_threshold &&
		 avgs[it->first].SqrDist(avgs[it2->first]) < colour_tolerance){
		  newlabel[it->first] = it2->first;
		  break;
		}
	  }
    
	for(int dx=-nbr_radius;dx<=nbr_radius;dx++)
	  for(int dy=-nbr_radius;dy<=nbr_radius;dy++)
	    if(sf->coordinates_ok(x+dx,y+dy)){
	      sf->get_pixel_segment(x+dx,y+dy,s);
	      if(newlabel.count(s))
		sf->set_pixel_segment(x+dx,y+dy,newlabel[s]);
	    }
	
      }

    return ProcessNextMethod();

  }  
       
  ///////////////////////////////////////////////////////////////////////////
}

// Local Variables:
// mode: font-lock
// End:



