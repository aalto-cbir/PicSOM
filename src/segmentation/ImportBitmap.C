#include <ImportBitmap.h>

static const string vcid="@(#)$Id: ImportBitmap.C,v 1.7 2009/04/30 09:48:17 vvi Exp $";

namespace picsom{
  static ImportBitmap list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  ImportBitmap::ImportBitmap() {
    binarise=false;
    threshold=0;
  }

  ///////////////////////////////////////////////////////////////////////////

  ImportBitmap::~ImportBitmap() {
    
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *ImportBitmap::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void ImportBitmap::UsageInfo(ostream& os) const { 
    os << "ImportBitmap :" << endl
       << "  options : " << endl
       << "    -b <threshold>  binarise bitmap with given upper" << endl 
       << "                    limit for background " << endl
       << "    -B              same as -b 0" << endl
       << endl;
  }

  ///////////////////////////////////////////////////////////////////////////

  int ImportBitmap::ProcessOptions(int argc, char** argv) { 

    Option option;
    int argc_old=argc;


    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'b':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -b");
	break;
      }

      option.option="-w";
      option.addArgument(argv[1]);


      if(sscanf(argv[1],"%d",&(ImportBitmap::threshold)) != 1)
	EchoError("switch -b requires an integer specifier.");

      binarise=true;

      argc--;
      argv++;
      break;

    case 'B':
      option.option="-B";
      binarise=true;
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
  bool ImportBitmap::Process(){ 

    // read segmentation from the input image
    
    int x,y,s;
    float r,g,b;

    for(y=0;y<Height();y++) for(x=0;x<Width();x++){
      getImg()->get_pixel_rgb(x,y,r,g,b);
      s=(int)(255*(r+g+b)/3);
      if(s<0) s=-1; 
      getImg()->set_pixel_segment(x,y,s);
    }

    if(binarise)
      for(y=0;y<Height();y++) for(x=0;x<Width();x++){
	getImg()->get_pixel_segment(x,y,s);
	getImg()->set_pixel_segment(x,y,s > threshold ? 1 : 0);
      }
    
    return ProcessNextMethod();
  }
} // namespace picsom

  ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
