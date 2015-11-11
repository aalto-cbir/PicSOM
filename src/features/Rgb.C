// $Id: Rgb.C,v 1.8 2006/11/17 14:18:38 jorma Exp $	

#include <Rgb.h>

namespace picsom {
  static const string vcid =
    "$Id: Rgb.C,v 1.8 2006/11/17 14:18:38 jorma Exp $";

  static Rgb list_entry(true);

  //===========================================================================

  string Rgb::Version() const {
    return vcid;
  }

  //===========================================================================

  int Rgb::printMethodOptions(ostream &str) const {
    /// obs! dunno yet
    return Feature::printMethodOptions(str);
  }

  //===========================================================================

  Feature::comp_descr_t Rgb::ComponentDescriptions() const {
    
    if(histogramMode.empty()){

      comp_descr_t d(PixelTypeSize());
      
      for (size_t i=0; i<(size_t)PixelTypeSize(); i++) {
	typedef pair<string,string> P;
	
	d[i].push_back(P("name", PixelChannelName(i)));
	d[i].push_back(P("min-value", "0"));
	d[i].push_back(P("max-value", "1"));
	
	// d[i].push_back(P("min-label", ""));
	// d[i].push_back(P("max-label", ""));
      }
      
      return d;
    }else{
      comp_descr_t d(0);
      return d;
    }
  }

  //===========================================================================

  Feature::featureVector Rgb::getRandomFeatureVector() const {
    featureVector ret;

    ret.push_back(rand()/(float)RAND_MAX); 
    ret.push_back(rand()/(float)RAND_MAX); 
    ret.push_back(rand()/(float)RAND_MAX); 

    return ret;
  }

  //===========================================================================

} // namespace picsom

//=============================================================================

// Local Variables:
// mode: font-lock
// End:

