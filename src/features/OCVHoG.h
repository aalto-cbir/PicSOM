// -*- C++ -*- 	$Id: OCVHoG.h,v 1.1 2013/09/24 13:09:14 vvi Exp $
/**
   \file OCVHoG.h

   \brief Declarations and definitions of class OCVHoG
   
   OCVHoG.h contains declarations and definitions of class the OCVHoG, which
   is a class that performs MPEG7-like edge histogram extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.1 $
   $Date: 2013/09/24 13:09:14 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _OCVHoG_
#define _OCVHoG_

#include "PseudoMPEG7.h"

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif // HAVE_OPENCV2_CORE_CORE_HPP


namespace picsom {
/// A class that performs the average colour value  extraction.
class OCVHoG : public PseudoMPEG7 {

 public:

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  OCVHoG(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  OCVHoG(const string& img, const string& seg,
	      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  OCVHoG(bool) : PseudoMPEG7(false) {}

  // ~OCVHoG();

  virtual Feature *Create(const string& s) const {
    return (new OCVHoG())->SetModel(s);
  }

 
  // copied from PixelBased

  virtual string Name()          const { return "ocvhog"; }

  virtual string LongName()      const { return "OCV HoG"; }

  virtual string ShortText()     const {
    return "Variant of HoG calculated using OCV"; }


  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const{

    int subdivcount=0;

    set<int>::iterator it;
    for(it=pyramidlevels.begin();it!=pyramidlevels.end();it++)
      subdivcount += pow(2,*it * 2);


    return vector<float>(subdivcount*(2*blocksH-1)*(2*blocksV-1)*9*4);
  }

  virtual int printMethodOptions(ostream&) const;

  virtual bool ProcessOptionsAndRemove(list<string>&); 

protected:

 
  virtual featureVector CalculateMask(char *mask);


  int blocksH;
  int blocksV;

  set<int> pyramidlevels;

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    
    
  // here some real data of ocv types

  cv::HOGDescriptor hog;

#endif // HAVE_OPENCV2_CORE_CORE_HPP


};
}
#endif

// Local Variables:
// mode: font-lock
// End:
