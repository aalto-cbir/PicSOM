// $Id: Colour.C,v 1.7 2013/06/26 14:06:54 mats Exp $	

#include <Colour.h>

namespace picsom {
  static const char *vcid =
    "$Id: Colour.C,v 1.7 2013/06/26 14:06:54 mats Exp $";

static Colour list_entry(true);

//=============================================================================

string Colour::Version() const {
  return vcid;
}
//=============================================================================

bool Colour::ProcessOptionsAndRemove(list<string>&l){

  for (list<string>::iterator i = l.begin(); i!=l.end();) {
   
    string key, value;
    SplitOptionString(*i, key, value);
    
    if (key=="colourspace") {
      colourConversion=value;
      i = l.erase(i);
      continue;
    }
    i++;
  }

  return PixelBased::ProcessOptionsAndRemove(l);

}


//=============================================================================

int Colour::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  str << "colourspace=rgb|hsvcone|hsvcyl|hsvpolar|hsvpolar_shift|mtm|cielab     specifies colour space" 
      << endl;
  return ret+1;
}

//=============================================================================
Feature *Colour::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  colourConversion="none";
  return PixelBased::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================


//=============================================================================
void Colour::ColourConversion(vector<float> &v) const{
  if(colourConversion=="rgb"||colourConversion=="none"){
    //do nothing
  }
  else if(colourConversion=="hsvcone"){
    imagedata::rgb_to_conical_hsv(v[0],v[1],v[2]);
  }
  else if(colourConversion=="hsvcyl"){
    imagedata::rgb_to_cylindrical_hsv(v[0],v[1],v[2]);
  }
  else if(colourConversion=="hsvpolar"){
    imagedata::rgb_to_hsv(v[0],v[1],v[2]);
  }
  else if(colourConversion=="hsvpolar_shift"){
    imagedata::rgb_to_hsv(v[0],v[1],v[2]);
    v[0] += 1.0/12.0;
    if (v[0] > 1.0)
      v[0] -= 1.0;
    if (v[1] == 0.0)
      v[0] = rand()/(float)RAND_MAX;
  }
  else if(colourConversion=="mtm"){
    imagedata::rgb_to_munsell(v[0],v[1],v[2]);
  }
  else if(colourConversion=="cielab"){
    imagedata::rgb_to_cielab(v[0],v[1],v[2]);
  }
  else if(colourConversion=="yiq"){
    imagedata::rgb_to_yiq(v[0],v[1],v[2]);
  }
  else
    throw string("Colour::ColourConversion:: unknown conversion ")+colourConversion;
}
//=============================================================================

Feature::featureVector Colour::getRandomFeatureVector() const {
  featureVector ret;

  ret.push_back(rand()/(float)RAND_MAX); 
  ret.push_back(rand()/(float)RAND_MAX); 
  ret.push_back(rand()/(float)RAND_MAX); 

  return ret;
}
}

// Local Variables:
// mode: font-lock
// End:

