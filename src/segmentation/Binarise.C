#include <Binarise.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::map;

static const string vcid =
"@(#)$Id: Binarise.C,v 1.8 2009/04/29 14:28:36 vvi Exp $";
namespace picsom {
  static Binarise list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  Binarise::Binarise() {
    backgroundLabel=-1;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  Binarise::~Binarise() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *Binarise::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void Binarise::UsageInfo(ostream& os) const { 
    os << "Binarise :" << endl
       << "  options : " << endl
       << "  -b <backgroundlabel>"<<endl
       << "  -B      same as -b 0"<<endl  
       << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int Binarise::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      default:
      case 'b':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -b");
	  break;
	}

	option.option = "-b";
	option.addArgument(argv[1]);

	if(sscanf(argv[1],"%d",&backgroundLabel) != 1)
	  EchoError("switch -b requires an integer specifier.");

	argc--;
	argv++;
	break;

      case 'B':
	option.option = "-B";
	backgroundLabel=0;

	break;

	throw string("Binarise::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);
    
      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;
  
  }
  ///////////////////////////////////////////////////////////////////////////

  bool Binarise::Process(){
    int v;
    map<int,int> accu;
    segmentfile *sf=getImg();

    int blabel=backgroundLabel;

    if(backgroundLabel<0){
      float cx=sf->getWidth()/2;
      float cy=sf->getHeight()/2;

      for (int y=0; y<sf->getHeight(); y++)
	for(int x=0; x<sf->getWidth(); x++) {
	  float d=(x-cx)*(x-cx)+(y-cy)*(y-cy);
	  sf->get_pixel_segment(x, y, v);
	  accu[v]+=(int)d;
	}
	
      float max=-1;
      int maxind=-1;
    
      map<int,int>::const_iterator it;

      for(it=accu.begin();it!=accu.end();it++)
	if(it->second > max){
	  max=it->second;
	  maxind=it->first;
	}
      blabel=maxind;
    }
      
  //   cout << "maxind=" << maxind << endl;
 //    sf->showHist();

    for (int y=0; y<sf->getHeight(); y++)
      for(int x=0; x<sf->getWidth(); x++) {
	sf->get_pixel_segment(x, y, v);
	sf->set_pixel_segment(x,y,(v==blabel)?0:1);
      }

//     sf->showHist();

    return ProcessNextMethod();

  }  
       
  ///////////////////////////////////////////////////////////////////////////
}

// Local Variables:
// mode: font-lock
// End:



