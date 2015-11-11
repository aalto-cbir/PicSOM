// -*- C++ -*- 	$Id: EdgeHistogram.h,v 1.2 2008/10/31 09:44:26 jorma Exp $
/**
   \file EdgeHistogram.h

   \brief Declarations and definitions of class EdgeHistogram
   
   EdgeHistogram.h contains declarations and definitions of class the EdgeHistogram, which
   is a class that performs MPEG7-like edge histogram extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.2 $
   $Date: 2008/10/31 09:44:26 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _EdgeHistogram_
#define _EdgeHistogram_

#include "PseudoMPEG7.h"

namespace picsom {
/// A class that performs the average colour value  extraction.
class EdgeHistogram : public PseudoMPEG7 {

 public:

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  EdgeHistogram(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  EdgeHistogram(const string& img, const string& seg,
	      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  EdgeHistogram(bool) : PseudoMPEG7(false) {}

  // ~EdgeHistogram();

  virtual Feature *Create(const string& s) const {
    return (new EdgeHistogram())->SetModel(s);
  }

 
  // copied from PixelBased

  virtual string Name()          const { return "edgehistogram"; }

  virtual string LongName()      const { return "Edge Histogram"; }

  virtual string ShortText()     const {
    return "Histogram of five edge types in 4x4 subimages"; }


  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const{
    return vector<float>(80);
  }

  virtual int printMethodOptions(ostream&) const;

  virtual bool ProcessOptionsAndRemove(list<string>&); 

protected:

 
  virtual featureVector CalculateMask(char *mask);
  void accumulatePixel(int x,int y,float &sum, float &count,
		       char mask,float weight);

  bool zeropad;

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
