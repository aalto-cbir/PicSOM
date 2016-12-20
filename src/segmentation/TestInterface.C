#include <TestInterface.h>

static const string vcid=
"@(#)$Id: TestInterface.C,v 1.4 2009/04/30 10:23:16 vvi Exp $";

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::set;


namespace picsom {

  static TestInterface list_entry(true);

#define WEIGHTBIAS 0.002

  ///////////////////////////////////////////////////////////////////////////

  TestInterface::TestInterface() {   
  tileX1=tileX2=tileY1=tileY2=0;
  }

  ///////////////////////////////////////////////////////////////////////////

  TestInterface::~TestInterface() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *TestInterface::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void TestInterface::UsageInfo(ostream& os) const { 
    os << "TestInterface :" << endl
       << "  options : " << endl
       << "    -f <feature> [-o opt1=str1 opt2=...] c1:c2:...:cn w1:w2:...:wn" << endl
       << "    -t <x1> <y1> <x2> <y2> " << endl;
  }

  ///////////////////////////////////////////////////////////////////////////

  int TestInterface::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;

    while (argc>1 && *argv[0]=='-') {
      option.reset();
      switch (argv[0][1]) {
      case 't':
	if (argc<6){
	EchoError("Not enough commandline arguments for switch -t");
	break;
      }

      option.option = "-t";
      option.addArgument(argv[1]);
      option.addArgument(argv[2]);
      option.addArgument(argv[3]);
      option.addArgument(argv[4]);

      if(sscanf(argv[1],"%d",&tileX1) != 1)
	EchoError("switch -t requires an integer specifier.");

      if(sscanf(argv[2],"%d",&tileY1) != 1)
	EchoError("switch -t requires an integer specifier.");

      if(sscanf(argv[3],"%d",&tileX2) != 1)
	EchoError("switch -t requires an integer specifier.");

      if(sscanf(argv[4],"%d",&tileY2) != 1)
	EchoError("switch -t requires an integer specifier.");

      argc-=4;
      argv+=4;
      break;

    case 'f':
      if (argc<5){
	EchoError("Not enough commandline arguments for switch -f");
	break;
      }
      option.option = "-f";
      {
	int nr_opts=featureInterface.parseFeatureSpec(argv+1);
	int nrargs=3+2*nr_opts;
	
	int j;

	for(j=1;j<=nrargs;j++)
	  option.addArgument(argv[j]);
	
	argc -= nrargs;
	argv += nrargs;
      }
      break;

      default:
	throw string("TestInterface::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;

  }


  ///////////////////////////////////////////////////////////////////////////

  bool TestInterface::Process()
  {

    // preProcess
    
    int i;

    featureInterface.createMethods(getImg());

    // doProcess

    if(Verbose() > 1){

      // dump info about the parametres from the command line
      cout << "number of feature classes: " << featureInterface.featureClass.size() << endl;


      for(i=0;(unsigned)i<featureInterface.featureClass.size();i++){
	cout << "feature class " << i << endl;
	cout << "name: " << featureInterface.featureClass[i].name << endl;
	cout << "options: " ;
	list<string>::const_iterator it;
	for(it=featureInterface.featureClass[i].options.begin();
	    it!=featureInterface.featureClass[i].options.end(); it++)
	  cout << *it << " ";
	cout << endl;

      }

      cout << "number of feature specifications: " << featureInterface.featureSpec.size() << endl;
      
      for(i=0;(unsigned)i<featureInterface.featureSpec.size();i++){
	cout << "feature specification " << i << endl;
	cout << "class: " << featureInterface.featureSpec[i].c << endl;
	cout << "index: " << featureInterface.featureSpec[i].index << endl;
	cout << "weight: " << featureInterface.featureSpec[i].weight << endl;
      }
    

    }

    Feature::featureVector fV;

    coordList l;
    int x,y;
    for(x=tileX1;x<=tileX2;x++) for(y=tileY1;y<=tileY2;y++)
      l.push_back(coord(x,y));

    vector<coordList *> vc;
    vc.push_back(&l);

    featureInterface.calculateFeatures(vc,fV);
    Feature::printFeatureVector(cout,fV);
    cout << endl;
  
    featureInterface.freeClasses();
    
    return ProcessNextMethod();

  }
  
}
 
 ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
