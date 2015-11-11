// $Id: SampleColour.C,v 1.3 2008/09/29 10:07:38 jorma Exp $	

#include <SampleColour.h>

namespace picsom {
  static const char *vcid =
    "$Id: SampleColour.C,v 1.3 2008/09/29 10:07:38 jorma Exp $";

static SampleColour list_entry(true);

//=============================================================================

string SampleColour::Version() const {
  return vcid;
}

//=============================================================================

bool SampleColour::ProcessOptionsAndRemove(list<string>&l){

  for (list<string>::iterator i = l.begin(); i!=l.end();) {
   
    string key, value;
    SplitOptionString(*i, key, value);
    
    if (key=="samplecount") {
      if(sscanf(value.c_str(),"%d",&sampleCount) != 1)
	throw string("SampleColour::ProcessOptionsAndRemove: couldn't parse samplecount.");
      i = l.erase(i);
      continue;
    }
    i++;
  }

  return Colour::ProcessOptionsAndRemove(l);

}


//=============================================================================

int SampleColour::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Colour::printMethodOptions(str);
  str << "samplecount=<int>     number of samples/image (def. 1)" 
      << endl;
  return ret+1;
}

//=============================================================================
Feature *SampleColour::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  sampleCount=1;
  return Colour::Initialize(img,seg,s,opt,allocate_data);
}
}
//=============================================================================


