// $Id: ScalableColor.C,v 1.3 2008/09/29 10:22:40 markus Exp $	

#include <ScalableColor.h>

namespace picsom {
  static const char *vcid =
    "$Id: ScalableColor.C,v 1.3 2008/09/29 10:22:40 markus Exp $";

static ScalableColor list_entry(true);

//=============================================================================

string ScalableColor::Version() const {
  return vcid;
}
//=============================================================================

bool ScalableColor::ProcessOptionsAndRemove(list<string>&l){

  return Feature::ProcessOptionsAndRemove(l);

}


//=============================================================================

int ScalableColor::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  return ret;
}

//=============================================================================
Feature *ScalableColor::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  return Feature::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================

  Feature::featureVector ScalableColor::CalculateMask(char *mask){

    // the mask specifies, which pixels to include in calculation  
    // nonzero -> include


    int w = Width(true), h = Height(true);

    // find the bounding box of the mask to claculate the dividing grid


    vector<int> hist(256,0);
    vector<float> vf;

    int ind=0;

    for(int y=0;y<h;y++)
      for(int x=0;x<w;x++)
	if(mask[ind++]){
	  vf= GetPixelFloats(x, y, pixel_rgb);
	  imagedata::rgb_to_hsv(vf[0],vf[1],vf[2]);
	  //	  cout << "got hsv ("<<vf[0]<<" "<<vf[1]<<" "<<vf[2]<<")"<<endl;

	  int hidx=(int)(vf[0]*16);
	  if(hidx==16) hidx=15;
	  int sidx=(int)(vf[1]*4);
	  if(sidx==4) sidx=3;
	  int vidx=(int)(vf[2]*4);
	  if(vidx==4) vidx=3;

	  int idx=(hidx<<4) + (sidx<<2) + vidx;
	  
	  //cout << " index " << idx<<endl;

	  hist[idx]++;
	  
	}

    int maxval=0;

    int maxint=(1<<11);

    for(int i=0;i<256;i++){
      if(hist[i]>maxval) maxval=hist[i];
    }

    // cout << "maxval="<<maxval<<endl;
    
    vector<float> haar(256);

    for(int i=0;i<256;i++){
      hist[i] = hist[i]*maxint/maxval;
      if(hist[i]==maxint) hist[i]--;

      // quantise non-linearly to 4 bits

      haar[i]=histquant4(hist[i]);

    }

    // quantised histogram formed, perform the 
    // HAAR transform

    haar1d(haar);

    return haar;

  }

//=============================================================================
} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

