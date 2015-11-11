// -*- C++ -*-  $Id: Texture.C,v 1.16 2008/09/29 10:22:40 markus Exp $
// 
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <Texture.h>

namespace picsom {
  static const char *vcid =
    "$Id: Texture.C,v 1.16 2008/09/29 10:22:40 markus Exp $";

  static Texture list_entry(true);

  //===========================================================================

  string Texture::Version() const {
    return vcid;
  }

  //===========================================================================

  bool Texture::MakeYArray() {
    EnsureSegmentation();
    int w = Width(), h = Height();
    int imageSize = w*h;
  
    int x=0, y=0, count=0;

    while(count<imageSize) {
      // float r,g,b;
      // segmfile.get_pixel_rgb(x,y,r,g,b);
      // yArray.push_back(0.299*r + 0.587*g + 0.114*b);

      vector<float> v = GetPixelFloats(x, y, pixel_rgb);
      if (v.size()!=3)
	throw "Texture::MakeYArray() v.size()!=3";

      yArray.push_back(0.299*v[0] + 0.587*v[1] + 0.114*v[2]);

      count++;

      if (++x==w) {
	y++;
	x=0;
      }
    }
    return true;
  }

  //===========================================================================

  /*
    bool Texture::probeTileFeatures(int x1,int y1, int x2, int y2,
    featureVector &v,int,bool) {
    return true;
    }
  */

  //===========================================================================
    
  Feature::featureVector Texture::TextureData::Result(bool azc) const {
    if (!azc && !Count())
      throw "TextureData::Result() : count==0 => division by zero";

    if (Feature::LabelVerbose())
      cout << "TextureData::Result(), count=" << Count();
        
    double mul = Count() ? 1.0/Count() : 1.0;
    featureVector v(temp_num);
    for (size_t i=0; i<v.size(); i++) {
      v[i] = texsum[i]*mul;
      if (Feature::LabelVerbose())
	cout << " " << texsum[i] << "->" << v[i];
    }
    if (Feature::LabelVerbose())
      cout << endl;

    return v;
  }
  
  //===========================================================================
   
  float Texture::ProcessPoints(int x, int y, int nx, int ny,
			       TemplateBasedData* /*d*/, int /*i*/) {
    // ??? TextureData *td = (TextureData *)d;
    const int w = Width();
  
    if (yArray[y*w+x] > yArray[ny*w+nx])
      return 1;
  
    return 0;
  }

  //===========================================================================
   
} // namespace picsom

//=============================================================================
   
// Local Variables:
// mode: font-lock
// End:
