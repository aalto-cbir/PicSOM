// -*- C++ -*- 	$Id: ColorLayout.h,v 1.2 2008/10/31 09:44:26 jorma Exp $
/**
   \file ColorLayout.h

   \brief Declarations and definitions of class ColorLayout
   
   ColorLayout.h contains declarations and definitions of class the ColorLayout, which
   is a class that performs sobel edge histogram extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.2 $
   $Date: 2008/10/31 09:44:26 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _ColorLayout_
#define _ColorLayout_

#include "PseudoMPEG7.h"

namespace picsom {
/// A class that performs the average colour value  extraction.
class ColorLayout : public PseudoMPEG7 {

 public:

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  ColorLayout(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  ColorLayout(const string& img, const string& seg,
	      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  ColorLayout(bool) : PseudoMPEG7(false) {}

  // ~ColorLayout();

  virtual Feature *Create(const string& s) const {
    return (new ColorLayout())->SetModel(s);
  }

 
  // copied from PixelBased

  virtual string Name()          const { return "colorlayout"; }

  virtual string LongName()      const { return "Colour layout"; }

  virtual string ShortText()     const {
    return "DCT coefficients of average colour in 20x20 grid."; }


  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const{
    return vector<float>(12);
  }

  virtual int printMethodOptions(ostream&) const;

  virtual bool ProcessOptionsAndRemove(list<string>&); 

protected:

  // quantisation functions defined in the MPEG-7 standard
 
  float trunc(float x){ return (x>0)?floor(x):ceil(x); }

  int quant_Y_DC(int i) {
    int j;
    i = i/8;
    if(i>192) j=112+(i-192)/4;
    else if(i>160) j=96+(i-160)/2;
    else if(i>96) j=32+i-96;
    else if(i>64) j=16+(i-64)/2;
    else j=i/4;
    return j>>1;
  }

  int quant_CbCr_DC(int i) {
    int j;
    i = i/8;
    if(i>191) j=63;
    else if(i>160) j=56+(i-160)/4;
    else if(i>144) j=48+(i-144)/2;
    else if(i>112) j=16+i-112;
    else if(i>96) j=8+(i-96)/2;
    else if(i>64) j=(i-64)/4;
    else j=0;
    return j;
  }

  int quant_Y_AC(int i) {
    int j;
    i = i/2;
    if(i>255) i=255;
    if(i<-256) i=-256;
    if(abs(i)>127) j=64+abs(i)/4;
    else if(abs(i)>63) j=32+abs(i)/2;
    else j=abs(i);
    j = (i<0)?-j:j;
    return (int)trunc(((double)j+128.0)/8.0+0.5);
  }

  int quant_CbCr_AC(int i) {
    int j;
    if(i>255) i=255;
    if(i<-256) i=-256;
    if(abs(i)>127) j=64+abs(i)/4;
    else if(abs(i)>63) j=32+abs(i)/2;
    else j=abs(i);
    j = (i<0)?-j:j;
    return (int)trunc(((double)j+128.0)/8.0+0.5);
  }

  virtual featureVector CalculateMask(char *mask);

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
