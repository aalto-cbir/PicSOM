#include <RestoreScale.h>

static const string vcid="@(#)$Id: RestoreScale.C,v 1.6 2009/04/30 10:10:43 vvi Exp $";

namespace picsom{
  static RestoreScale list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  RestoreScale::RestoreScale() {
    discardImage=false;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  RestoreScale::~RestoreScale() {

  }

  ///////////////////////////////////////////////////////////////////////////

  const char *RestoreScale::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void RestoreScale::UsageInfo(ostream& os) const { 
    os << "RestoreScale :" << endl
       << "  options : " << endl
       << "    -c discard image" << endl;

  }

  ///////////////////////////////////////////////////////////////////////////

  int RestoreScale::ProcessOptions(int argc, char** argv) { 

    Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'c':
	option.option = "-c";
	discardImage=true;
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

  bool RestoreScale::Process() {

    int ow,oh;
    const imagedata *oi = NULL;

    SegmentationResult *saved=getImg()->readLastFrameResultFromXML
      ("ScaleDown:originalImageId");

    if(!saved){
      if(Verbose()>1){
	cout << "RestoreScale::Process : saved image not found." << endl;
	cout << "Trying to find size info" << endl;
      }

      SegmentationResult *owRes=getImg()->readLastFrameResultFromXML
	("ScaleDown:originalWidth");
      if(!owRes)
	throw string("RestoreScale::Process: ScaleDown:originalWidth not found");

      SegmentationResult *ohRes=getImg()->readLastFrameResultFromXML
	("ScaleDown:originalHeight");
      if(!ohRes){
	delete owRes;
	throw string("RestoreScale::Process: ScaleDown:originalHeight not found");
      }

      if(sscanf(owRes->value.c_str(),"%d",&ow)!=1){
	delete owRes; delete ohRes;
	throw string("RestoreScale::Process: couldn't parse original width.");
      }
      if(sscanf(ohRes->value.c_str(),"%d",&oh)!=1){
	delete owRes; delete ohRes;
	throw string("RestoreScale::Process: couldn't parse original height.");
      }

      delete owRes; delete ohRes;
    }
    else{

      int id;
      if(sscanf(saved->value.c_str(),"%d",&id)!=1){
	throw string("RestoreScale::Process: couldn't parse original id.");
      }

      if(Verbose()>1)
	cout << "original image w/ id " << id << " found." << endl;

      oi=getImg()->storedImage(id);
      if(!oi)
	throw string ("RestoreScale::Process :storedImage(") + saved->value
		      +") failed"; 

      else
	if(Verbose()>1)
	  cout << "accessed parsed result frame" << endl;

      ow = oi->width();
      oh = oi->height();
    }

    getImg()->rescaleFrame(scalinginfo(Width(),Height(),ow,oh),oi,NULL,discardImage);
    
    delete saved;

    return ProcessNextMethod();
  }

  ///////////////////////////////////////////////////////////////////////////


  bool RestoreScale::Cleanup() {

    for(int f=0;f<getImg()->getNumFrames();f++){

    SegmentationResult *saved=getImg()->readLastFrameResultFromXML
      ("ScaleDown:originalImageId");

    if(!saved){
      if(Verbose()>1) 
	cout << "ScaleDown::Cleanup : saved image sequence not found." << endl;
    }
    else{

      int id;
      if(sscanf(saved->value.c_str(),"%d",&id)!=1){
	throw string("RestoreScale::Cleanup(): couldn't parse original id.");
      }
      delete saved;

      if(Verbose()>1) cout << "Removing stored image " << id <<  endl;
      getImg()->removeImage(id);
    }
    }

    return CleanupPreviousMethod();
  }

  ///////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
