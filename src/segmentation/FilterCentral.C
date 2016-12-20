#include <FilterCentral.h>


using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;

namespace picsom {
  static const string vcid="@(#)$Id: FilterCentral.C,v 1.3 2009/04/30 08:31:28 vvi Exp $";

  static FilterCentral list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  FilterCentral::FilterCentral() {

    output_destination=std_out;
    background_label=0;

    filter_criterion=touch;
    perimeterFrac=0.25;
    areaFrac=0.25;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  FilterCentral::~FilterCentral() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *FilterCentral::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void FilterCentral::UsageInfo(ostream& os) const { 
    os << "FilterCentral :" << endl
       << "  options : " << endl
       << "  -os          outputs to stdout (default)" << endl
       << "  -ox          outputs to xml" << endl
       << "  -ob <label>  outputs to bitmap" << endl
       << "  -oB          same as -ob 0" << endl
       << "  -t           filter out border touching segments (default)" 
       << endl
       << "  -p <frac>    filt. out segments w/ #(border_touch)/perimeter >"
      " <frac>" << endl
       << "  -P           same as -p 0.25" << endl

       << "  -a <frac>    filt. out segments w/ #(border_touch)/sqrt(area) >"
      " <frac>" << endl
       << "  -A           same as -a 0.25" << endl;

  }

  /////////////////////////////////////////////////////////////////////////////

  int FilterCentral::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;


    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'o':
	switch( argv[0][2]){
	case 's':
	  option.option="-os";
	  output_destination=std_out;
	  getImg()->SetNullOutput();
	  break;
	case 'x':
	  option.option="-ox";
	  output_destination=xml;
	  break;
	case 'b':
	  if (argc<3){
	    EchoError("Not enough commandline arguments for switch -ob");
	    break;
	  }

	  option.option = "-ob";
	  option.addArgument(argv[1]);

	  if(sscanf(argv[1],"%d",&background_label) != 1)
	    EchoError("switch -ob requires an integer specifier.");
	  output_destination=bitmap;
	  argc--;
	  argv++;
	  break;
	case 'B':
	  option.option="-oB";
	  background_label=0;
	  output_destination=bitmap;
	  break;
	  
	default:
	  goto unknown_option;
	//option.addArgument(argv[1]);
	}

	break;

      case 't':
	option.option="-t";
	filter_criterion=touch;
	break;

      case 'p':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -p");
	  break;
	}

	option.option = "-p";
	option.addArgument(argv[1]);
	filter_criterion=touch_per_perimeter;
      
	if(sscanf(argv[1],"%f",&perimeterFrac) != 1)
	  EchoError("switch -p requires a float specifier.");
	argc--;
	argv++;

	break;

      case 'P':
	option.option = "-P";
	filter_criterion=touch_per_perimeter;
	perimeterFrac=0.25;
	break;


      case 'a':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -a");
	  break;
	}

	option.option = "-a";
	option.addArgument(argv[1]);
	filter_criterion=touch_per_area;
      
	if(sscanf(argv[1],"%f",&areaFrac) != 1)
	  EchoError("switch -a requires a float specifier.");
	argc--;
	argv++;

	break;

      case 'A':
	option.option = "-A";
	filter_criterion=touch_per_area;
	areaFrac=0.25;
	break;
	
      default:
      unknown_option:
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
  
  bool FilterCentral::Process() {
   
    vector<int> *labels=getImg()->getUsedLabels();

    int n=labels->size();
    map<int,int> labelindex;

    for(int i=0;i<n;i++)
      labelindex[(*labels)[i]]=i;

    vector<int> borderCount(n,0), perimeterCount(n,0), areaCount(n,0);

    int w=getImg()->getWidth(), h=getImg()->getHeight();

    // collect necessary statistics

    for (int x=0; x<w; x++){
      for (int y=0; y<h; y++) {
	int s;
	
	getImg()->get_pixel_segment(x, y, s);
	
	areaCount[labelindex[s]]++;

	if(x==0 || x==w-1 || y==0 || y==h-1)
	  borderCount[labelindex[s]]++;
	
	if(getImg()->isSegmentBoundary(x,y))
	  perimeterCount[labelindex[s]]++;
      }
    }

    set<int> filterLabels;

    switch(filter_criterion){
    case touch:
      for(int i=0;i<n;i++)
	if(borderCount[i]) filterLabels.insert((*labels)[i]); 
      break;
    case touch_per_perimeter:
      for(int i=0;i<n;i++)
	if((float)borderCount[i]/perimeterCount[i] > perimeterFrac ) 
	  filterLabels.insert((*labels)[i]); 
      break;
    case touch_per_area:
      for(int i=0;i<n;i++)
	if((float)borderCount[i]/sqrt((float)areaCount[i]) > areaFrac ) 
	  filterLabels.insert((*labels)[i]);
      break;
    }

    switch(output_destination){
    case std_out:
      {
      string base=getImg()->getImageFileName();

      string::size_type i = base.rfind('/');
      if (i!=string::npos)
	base.erase(0, i+1);
  
      i = base.rfind('.');
      if (i!=string::npos)
	base.erase(i);

      stringstream ss;
      ss << base;

      if (getImg()->getNumFrames()>1)
	ss << ":" << getImg()->getCurrentFrame();

      base    = ss.str();
      string method  = getImg()->collectChainedName();
      // get rid of this last method's name

      //      cout << "Chained name:" << method<<endl;

      size_t ppos=method.rfind('+');
       if (ppos!=string::npos)
	method.erase(ppos);

       string pattern = "%m:%i";


       base = segmentfile::FormOutputFileName(pattern, base, method);

  

      
      for(int i=0;i<n;i++)
	if(!filterLabels.count((*labels)[i]))
	  cout << base << "_"<<(*labels)[i] << endl;
      }
      break;
    case xml:
      {
	SegmentationResult res;
	res.name="CentralSegments";
	res.type="list";
	res.value="";

	char labelstr[40];
	bool first=true;
	for(int i=0;i<n;i++)
	  if(!filterLabels.count((*labels)[i])){
	    sprintf(labelstr,"%d",(*labels)[i]);
	      if(first)
		first=false;
	      else
		res.value += ' ';
	      res.value += labelstr;
	    }
	
	getImg()->writeFrameResultToXML(this,res);
      }
      break;
    case bitmap:
      for (int x=0; x<w; x++)
	for (int y=0; y<h; y++) {
	  int s;
	  getImg()->get_pixel_segment(x, y, s);
	  if(filterLabels.count(s))
	    SetPixelSegment(x,y,background_label);	  
      }

      break;
    }

    delete labels;

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
