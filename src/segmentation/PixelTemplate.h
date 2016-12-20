// -*- C++ -*-	$Id: PixelTemplate.h,v 1.9 2009/08/19 13:08:14 jorma Exp $

#ifndef _PIXELTEMPLATE_H_
#define _PIXELTEMPLATE_H_

#include <imagedata.h>
#include <stdio.h>

namespace picsom {

  

class PixelTemplate : public imagedata {
  /// a class for storing one-channel float pixel templates
public:
  /// 
  PixelTemplate(): imagedata(){};

  ///
  PixelTemplate(int w, int h) : imagedata(w,h,1,imagedata::pixeldata_float)
  {}

  ///
  PixelTemplate(const imagedata &i):imagedata(i){
    /// after copying data members 
    /// ensure suitable data format
    convert(pixeldata_float);
    force_one_channel();
  }

  /// constructor for scaled version of a tile of source image
  PixelTemplate(const imagedata&, int, int, int, int, int, int);


  /// 
  //virtual ~PixelTemplate();

  ///
  virtual const char *Version() const;

  ///
  bool Verbose() const {return false;};

  ///
  // bool InsertScaledImage(const imagedata&, int, int, int, int, int, int);

  // functionality now implemented in constructor

  // Calculates the correlation between template tmpl and picture *this
  // within the area bounded by (tlx,tly) -> (brx,bry) 
  // calculation will happen for each pixel if dx & dy ==1
  // otherwise will skip dx pixels horizontally and dy pixels vertically
  PixelTemplate *Correlate(const /*Segmentation*/ PixelTemplate& tmpl,
			   int tlx, int tly,
			   int brx, int bry, int dx, int dy) const;
			   
  float Corr2(PixelTemplate* pTmpl);
  
  /// simple version (no error checking, limits must be set 
  /// from calling function so that (brx,bry) will *never* exceed
  /// *src image dimensions and *tgt must have big enough size!
  bool Normalize(const PixelTemplate *src,
		  PixelTemplate *tgt, int tlx,
		 int tly, int brx, int bry) const;

  // returns result of max correlation (x-coord)
  int GetCorrMax_x() {
      return Corr_Max_x;
  }
  // returns result of max correlation (y-coord)
  int GetCorrMax_y() {
      return Corr_Max_y;
  }
  // sets max correlation (x-coord)
  // move under "protected"
  void SetCorrMax_x(int x) {
      Corr_Max_x = x;
  }
  // sets max correlation (y-coord)
  // move under "protected"
  void SetCorrMax_y(int y) {
      Corr_Max_y = y;
  }
  // returns the maximum correlation result
  float GetCorrMax(){
    return Corr_Max;
  }
  // sets the maximum correlation result
  void SetCorrMax(float max) {
    Corr_Max = max;
  }
  // returns result of max correlation (x-coord)
  int GetOptCorrMax_x() {
      return opt_Corr_Max_x;
  }
  // returns result of max correlation (y-coord)
  int GetOptCorrMax_y() {
      return opt_Corr_Max_y;
  }
  // sets max correlation (x-coord)
  // move under "protected"
  void SetOptCorrMax_x(int x) {
      opt_Corr_Max_x = x;
  }
  // sets max correlation (y-coord)
  // move under "protected"
  void SetOptCorrMax_y(int y) {
      opt_Corr_Max_y = y;
  }
  // returns the maximum correlation result
  float GetOptCorrMax(){
    return opt_Corr_Max;
  }
  // sets the maximum correlation result
  void SetOptCorrMax(float max) {
    opt_Corr_Max = max;
  }

protected:
  // maximum correlation coordinates
  int Corr_Max_x, Corr_Max_y;
  float Corr_Max;

  // maximum correlation coords after optimization
  int opt_Corr_Max_x, opt_Corr_Max_y;
  float opt_Corr_Max;

};

}

#endif // _PIXELTEMPLATE_H_

// Local Variables:
// mode: font-lock
// End:
