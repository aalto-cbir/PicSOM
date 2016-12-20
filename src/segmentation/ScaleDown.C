#include <ScaleDown.h>

static const string vcid="@(#)$Id: ScaleDown.C,v 1.4 2009/04/30 10:13:50 vvi Exp $";

namespace picsom {

  static ScaleDown list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  ScaleDown::ScaleDown() {
    desiredWidth=150;
    saveOriginal=false;
  }

  ///////////////////////////////////////////////////////////////////////////

  ScaleDown::~ScaleDown() {
    
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *ScaleDown::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void ScaleDown::UsageInfo(ostream& os) const { 
    os << "ScaleDown :" << endl
       << "  options : " << endl
       << "    -w <desired width>"
       << "    -s save the original image" << endl;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  int ScaleDown::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'w':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -w");
	  break;
	}
	
	option.option = "-w";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(ScaleDown::desiredWidth)) != 1)
	  throw string("switch -w requires an integer specifier.");
	
	argc--;
	argv++;
	break;

      case 's':
	option.option = "-s";
	saveOriginal=true;
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

  bool ScaleDown::Process() {

    if(saveOriginal){

      char nrstr[15];
      SegmentationResult r;
      int id=getImg()->storeImage(*(getImg()->imageFrame(getImg()->getCurrentFrame())));

      r.name = "ScaleDown:originalImageId";
      r.type = "int";
      sprintf(nrstr,"%d",id);
      r.value = nrstr;
      getImg()->writeFrameResultToXML(this,r);

    }
    else{
      char nrstr[15];
      SegmentationResult r;


      r.name = "ScaleDown:originalWidth";
      r.type = "int";
      sprintf(nrstr,"%d",Width());
      r.value = nrstr;
      getImg()->writeFrameResultToXML(this,r);

      r.name = "ScaleDown:originalHeight";
      r.type = "int";
      sprintf(nrstr,"%d",Height());
      r.value = nrstr;
      getImg()->writeFrameResultToXML(this,r);
    }

    if(desiredWidth>Width()){ // no scaling in this case

      // throw string("ScaleDown cannot scale up image."); 
    }
    else{


      int new_width = desiredWidth;
      int new_height =(int)( ((float)desiredWidth)/Width()*Height());

      if(Verbose()>1)
	cout << "scaling down " << Width()<<"x"<<Height()<<" -> " <<
	  new_width << "x"<<new_height<<endl;
      
      getImg()->rescaleFrame(scalinginfo(Width(),Height(),new_width,new_height));
    }

    return ProcessNextMethod();
  }

} // namespace picsom
 
///////////////////////////////////////////////////////////////////////////


// Local Variables:
// mode: font-lock
// End:
