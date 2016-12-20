#include <SeparateMultipart.h>


using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;

namespace picsom {
  static const string vcid="@(#)$Id: SeparateMultipart.C,v 1.3 2009/04/30 10:18:47 vvi Exp $";

  static SeparateMultipart list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  SeparateMultipart::SeparateMultipart() {
  }

  /////////////////////////////////////////////////////////////////////////////
  
  SeparateMultipart::~SeparateMultipart() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *SeparateMultipart::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void SeparateMultipart::UsageInfo(ostream& os) const { 
    os << "SeparateMultipart :" << endl
       << "  options : none" << endl;

  }

  /////////////////////////////////////////////////////////////////////////////

  int SeparateMultipart::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;


    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
   
	
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
  
  bool SeparateMultipart::Process() {
    cout << " SeparateMultipart::Process()" << endl;
    int i=0;
    int *lbl = getImg()->GetSeparateLabelingInt(NULL,true);
    for (int y=0; y<getImg()->getHeight(); y++) 
      for (int x=0; x<getImg()->getWidth(); x++){
	SetPixelSegment(x,y,lbl[i++]);
      }

    delete lbl;

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
