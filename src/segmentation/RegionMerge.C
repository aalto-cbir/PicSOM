#include <RegionMerge.h>
#include <math.h>

static const string vcid =
"@(#)$Id: RegionMerge.C,v 1.18 2009/04/30 10:08:28 vvi Exp $";

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::set;
using std::make_heap;
using std::push_heap;
using std::pop_heap;
using std::list;

namespace picsom {

  static RegionMerge list_entry(true);


  ///////////////////////////////////////////////////////////////////////////

  RegionMerge::RegionMerge() {    

    merger=new RegionMerger();

    numberOfRegions=5;
    sizeThreshold=600;
    useAveraging=false;
    weightDistances=0;
    pixelsToUse=-1;
    weightBias=WEIGHTBIAS;
    fourNeighbours=false;
    saturationLimit=10; // larger that 2* (area of whole image)
    edgeWeight=0;

    outputTree=false;
    treeLeavesAsBitmap=false;
    initialApproximate=false;
  }

  ///////////////////////////////////////////////////////////////////////////

  RegionMerge::~RegionMerge() {
    delete merger;
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *RegionMerge::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void RegionMerge::UsageInfo(ostream& os) const { 
    os << "RegionMerge :" << endl
       << "  options : " << endl
       << "    -f <feature> [-o opt1=str1 opt2=...] c1:c2:...:cn w1:w2:...:wn" << endl
       << "    -r <desired # of regions> (default: 5)" << endl
       << "    -s <region area threshold> (in pixels, default: 600) " << endl
       << "    -a combine feature vectors for two regions by averaging " << endl
       << "    -p <# of pixels to use in feature calculation>" << endl
       << "    -w1 weight distances according to size of smaller region (sqrt)" << endl
       << "    -w2 weight distances according to sum of region sizes (sqrt)" << endl
       << "    -w3 weight distances according to smaller region size (log)" << endl
       << "    -w4 weight distances according to smaller region size (linear)" << endl

       << "    -b  bias term used in weighting" << endl
       << "    -l  saturating limit for -wX (as fraction of image area)" << endl
       << "    -c4 use four-connectivity in neighbour region determination" << endl
       << "    -e weighting of edge magnitude between regions (def. 0)" << endl
       << "    -t output segmentation hierarchy, leaf regions in bitmap" << endl
       << "    -T output segmentation hierarchy, leaf regions in XML" << endl
       << "    -i use approximation in initial merging" << endl;
    

  }

  ///////////////////////////////////////////////////////////////////////////

  int RegionMerge::ProcessOptions(int argc, char** argv)
  {
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

      if(sscanf(argv[1],"%d",&(RegionMerge::numberOfRegions)) != 1)
	EchoError("switch -r requires an integer specifier.");

      argc--;
      argv++;
      break;

    case 's':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -s");
	break;
      }

      option.option = "-s";
      option.addArgument(argv[1]);

      if(sscanf(argv[1],"%d",&(RegionMerge::sizeThreshold)) != 1)
	EchoError("switch -s requires an integer specifier.");

      argc--;
      argv++;
      break;


    case 'f':
      if (argc<5){
	EchoError("Not enough commandline arguments for switch -f");
	break;
      }
      option.option = "-f";
      {
	int nr_opts=merger->featureInterface.parseFeatureSpec(argv+1);
	int nrargs=3+2*nr_opts;
	
	int j;

	for(j=1;j<=nrargs;j++)
	  option.addArgument(argv[j]);
	
	argc -= nrargs;
	argv += nrargs;
      }
      break;

      case 'a':
	option.option = "-a";
	useAveraging=true;
	break;

      case 'p':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -p");
	  break;
	}

	option.option = "-p";
	option.addArgument(argv[1]);

	if(sscanf(argv[1],"%d",&(RegionMerge::pixelsToUse)) != 1)
	  EchoError("switch -p requires an integer specifier.");

	argc--;
	argv++;
	break;

      case 'w':
	
	if(argv[0][2]=='1'){
	  option.option = "-w1";
	  weightDistances=1;
	  break;
	}
	else if(argv[0][2]=='2'){
	  option.option = "-w2";
	  weightDistances=2;
	  break;
	}
	else if(argv[0][2]=='3'){
	  option.option = "-w3";
	  weightDistances=3;
	  break;
	}
	else if(argv[0][2]=='4'){
	  option.option = "-w4";
	  weightDistances=4 ;
	  break;
	}
	else
	  throw string("RegionMerge::processOptions(): unknown option ")+argv[0];

	break;

      case 'c':
	
	if(argv[0][2]=='4'){
	  option.option = "-c4";
	  fourNeighbours=true;
	  break;
	}
     

	else
	  throw string("RegionMerge::processOptions(): unknown option ")+argv[0];

	break;


      case 'b':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -b");
	  break;
	}

	option.option = "-b";
	option.addArgument(argv[1]);
      
	if(sscanf(argv[1],"%f",&(RegionMerge::weightBias)) != 1)
	  EchoError("switch -b requires a float specifier.");
	
	argc--;
	argv++;
	break;

      case 'l':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -l");
	  break;
	}

	option.option = "-l";
	option.addArgument(argv[1]);
      
	if(sscanf(argv[1],"%f",&(RegionMerge::saturationLimit)) != 1)
	  EchoError("switch -l requires a float specifier.");
	
	argc--;
	argv++;
	break;

      case 'e':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -e");
	  break;
	}

	option.option = "-e";
	option.addArgument(argv[1]);
      
	if(sscanf(argv[1],"%f",&(RegionMerge::edgeWeight)) != 1)
	  EchoError("switch -e requires a float specifier.");
	
	argc--;
	argv++;
	break;

      case 't':
	option.option = "-t";
	outputTree = true;
	treeLeavesAsBitmap=true;
	break;

      case 'T':
	option.option = "-T";
	outputTree = true;
	treeLeavesAsBitmap=false;
	break;

      case 'i':
	option.option = "-i";
	initialApproximate=true;
	break;


      default:
	throw string("RegionMerge::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
  
    // pass parameter values to RegionMerger object

    merger->numberOfRegions=numberOfRegions;
    merger->sizeThreshold=sizeThreshold;
    merger->useAveraging=useAveraging;
    merger->weightDistances=weightDistances;
    merger->pixelsToUse=pixelsToUse;
    merger->weightBias=weightBias;
    merger->fourNeighbours=fourNeighbours;
    merger->saturationLimit=saturationLimit;
    merger->edgeWeight=edgeWeight;

    merger->outputTree=outputTree;
    merger->treeLeavesAsBitmap=treeLeavesAsBitmap;
    merger->initialApproximate=initialApproximate;

    return argc_old-argc;

  }


  ///////////////////////////////////////////////////////////////////////////

  bool RegionMerge::Process()
  {

    int i;

    merger->setImage(getImg());
    merger->Verbose(Verbose());

    merger->featureInterface.createMethods(getImg());

    merger->clear();
    merger->hierarchy.reset();

    if(Verbose() > 1){
      // dump info about the parametres from the command line
      cout << "number of feature classes: " << merger->featureInterface.featureClass.size() << endl;
      for(i=0;(unsigned)i<merger->featureInterface.featureClass.size();i++){
	cout << "feature class " << i << endl;
	cout << "name: " << merger->featureInterface.featureClass[i].name << endl;
	cout << "version: " << merger->featureInterface.featureClass[i].ptr->Version() << endl;

	cout << "short text: " << merger->featureInterface.featureClass[i].ptr->ShortText() << endl;

	cout << "options: " ;
	list<string>::const_iterator it;
	for(it=merger->featureInterface.featureClass[i].options.begin();
	    it!=merger->featureInterface.featureClass[i].options.end(); it++)
	  cout << *it << " ";
	cout << endl;

      }

      cout << "number of feature specifications: " << merger->featureInterface.featureSpec.size() << endl;
      
      for(i=0;(unsigned)i<merger->featureInterface.featureSpec.size();i++){
	cout << "feature specification " << i << endl;
	cout << "class: " << merger->featureInterface.featureSpec[i].c << endl;
	cout << "index: " << merger->featureInterface.featureSpec[i].index << endl;
	cout << "weight: " << merger->featureInterface.featureSpec[i].weight << endl;
      }
    

  }

    merger->mergeBb(NULL,numberOfRegions);

    if(outputTree){

      SegmentationResult res;
      res.name="regionhierarchy";
      res.type="regionhierarchy";
      res.value="";

      xmlNodePtr resNode=getImg()->writeFrameResultToXML(this,res);

      
      xmlNodePtr hNode=xmlNewTextChild(resNode,NULL,
				       (xmlChar*)"regionhierarchy",
				       NULL);

      xmlNodePtr lNode=xmlNewTextChild(hNode,NULL,
				       (xmlChar*)"leafregions",
				       NULL);
      
      if(treeLeavesAsBitmap)
	xmlSetProp(lNode,(xmlChar*)"inbitmap",(xmlChar*)"true");

      if(Verbose()>1)
	cout << merger->hierarchy.startingRegions.size() << " leaf regions and " <<
	  merger->hierarchy.regionMerges.size() << "region merges.";


      if(treeLeavesAsBitmap){
	int w=Width(),h=Height();
	int x,y;
	for(y=0;y<h;y++) for(x=0;x<w;x++)
	  getImg()->set_pixel_segment(x,y,0);
      }
	// read leaf regions from lists
	
      for(int i=0;(unsigned)i<merger->hierarchy.startingRegions.size();i++){
	const coordList &l=merger->hierarchy.startingRegions[i];	  
	if(treeLeavesAsBitmap){
	  int j;
	  for(j=0;(unsigned)j<l.size();j++)
	    getImg()->set_pixel_segment(l[j].x,l[j].y,i);
	}
	else{
	  PredefinedTypes::Region r;
	  r.id="list";
	  ostringstream ss;
	  ss << l;
	  r.value=ss.str();
	  string s;
	  r.write(s);
	  xmlNodePtr rNode=xmlNewTextChild(lNode,NULL,
					   (xmlChar*)"region",
					   (xmlChar*)s.c_str());
	  
	  char labelstr[30];
	  sprintf(labelstr,"%d",i); // labeling starts from 1	  
	  xmlSetProp(rNode,(xmlChar*)"label",(xmlChar*)labelstr);
	  
	}
	
      } 
      
      // output region merges
      
      xmlNodePtr mlistNode=xmlNewTextChild(hNode,NULL,
					   (xmlChar*)"mergelist",
					   NULL);
      int to=merger->hierarchy.startingRegions.size();

      for(size_t i=0;i<merger->hierarchy.regionMerges.size();i++){
	const pair<int,int> &p=merger->hierarchy.regionMerges[i];
	if(Verbose()>1)
	  cout << "merge: " << p.first << " " << p.second << endl; 
	
	xmlNodePtr mNode=xmlNewTextChild(mlistNode,NULL,
					   (xmlChar*)"merge",
					   NULL);

	char labelstr[30];
	sprintf(labelstr,"%d",p.first);
	xmlSetProp(mNode,(xmlChar*)"from1",(xmlChar*)labelstr);
	sprintf(labelstr,"%d",p.second);
	xmlSetProp(mNode,(xmlChar*)"from2",(xmlChar*)labelstr);
	sprintf(labelstr,"%d",to++);
	xmlSetProp(mNode,(xmlChar*)"to",(xmlChar*)labelstr);
      }	
    }

    merger->featureInterface.freeClasses();

    return ProcessNextMethod();

  }

  ///////////////////////////////////////////////////////////////////////////
  
  // list<string> RegionMerge::divideOptions(string options)
  //{
  //  
  //// primitive option string splitter
  //  // better one should be used
  //
  //int pos=0;
  //char *str;
    
  //list<string> ret;

  //str=new char[options.size()+1];
  //strcpy(str,options.c_str());
    
  //int startind;

  //for(;;){
  //  while(isspace(str[pos]));
  //  if(!str[pos]) break;
  //  startind=pos;
  //  while(str[pos] && !isspace(str[pos]));
  //  if(!str[pos]) break;
  //  str[pos]=0;
  //  ret.push_back(string(str+startind));
  //}

  //return ret;
  //}
  
} // namespace picsom

 
 ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
