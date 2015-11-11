// $Id: ColM.C,v 1.9 2014/01/23 13:06:24 jorma Exp $	

#include <ColM.h>

namespace picsom {
  static const char *vcid =
    "$Id: ColM.C,v 1.9 2014/01/23 13:06:24 jorma Exp $";

static ColM list_entry(true);

//=============================================================================

string ColM::Version() const {
  return vcid;
}

//=============================================================================

int ColM::printMethodOptions(ostream &str) const {
  /// obs! dunno yet
 int ret=Feature::printMethodOptions(str);
 str << "colourspace==rgb|hsvcone|hsvcyl|hsvpolar|mtm|cielab"
     << "     specifies colour space, default=" << DefaultColourConversion()
     << endl;
 return ret+1;

}
//=============================================================================

Feature *ColM::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  colourConversion = DefaultColourConversion();
  return PixelBased::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================



Feature::featureVector ColM::getRandomFeatureVector() const {
  featureVector ret;

  int i;
  for(i=0;i<9;i++)
    ret.push_back(rand()/(float)RAND_MAX); 
  
  return ret;
}
//=============================================================================

bool ColM::ProcessOptionsAndRemove(list<string>&l){

  for (list<string>::iterator i = l.begin(); i!=l.end();) {
   
    string key, value;
    SplitOptionString(*i, key, value);

    // these options should be somewere else...
    if (key=="colourspace") {
      colourConversion = value;
      i = l.erase(i);
      continue;
    }

    if (key=="histfixedbins") {
      histfixedbins = value;
      i = l.erase(i);
      continue;
    }



    i++;
  }

  return PixelBased::ProcessOptionsAndRemove(l);

}

//=============================================================================

void ColM::ColourConversion(vector<float>& v) const {
  if (v.size()<3 || colourConversion=="rgb" || colourConversion=="none")
    return;

  else if(colourConversion=="hsvcone")
    imagedata::rgb_to_conical_hsv(v[0],v[1],v[2]);
  
  else if(colourConversion=="hsvcyl")
    imagedata::rgb_to_cylindrical_hsv(v[0],v[1],v[2]);
  
  else if(colourConversion=="hsvpolar")
    imagedata::rgb_to_hsv(v[0],v[1],v[2]);
  
  else if(colourConversion=="mtm")
    imagedata::rgb_to_munsell(v[0],v[1],v[2]);
  
  else if(colourConversion=="cielab")
    imagedata::rgb_to_cielab(v[0],v[1],v[2]);
  
  else if(colourConversion=="yiq")
    imagedata::rgb_to_yiq(v[0],v[1],v[2]);
  
  else
    throw string("ColM::ColourConversion:: unknown conversion ")
      +colourConversion;
}
}
//=============================================================================


// Local Variables:
// mode: font-lock
// End:

