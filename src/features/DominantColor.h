// -*- C++ -*- 	$Id: DominantColor.h,v 1.6 2010/08/17 11:25:45 vvi Exp $
/**
   \file DominantColor.h

   \brief Declarations and definitions of class DominantColor
   
   DominantColor.h contains declarations and definitions of class the DominantColor, which
   is a class that performs sobel edge histogram extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.6 $
   $Date: 2010/08/17 11:25:45 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _DominantColor_
#define _DominantColor_

#include "PseudoMPEG7.h"
#include <algorithm>

namespace picsom {

class DominantColor : public PseudoMPEG7 {

 public:

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  DominantColor(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  DominantColor(const string& img, const string& seg,
	      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  DominantColor(bool) : PseudoMPEG7(false) {}

  // ~DominantColor();

  virtual Feature *Create(const string& s) const {
    return (new DominantColor())->SetModel(s);
  }

 
  // copied from PixelBased

  virtual string Name()          const { return "dominantcolor"; }

  virtual string LongName()      const { return "Dominant Color"; }

  virtual string ShortText()     const {
    return "CIE Luv coordinates of two dominant color clusters*"; }

  // NB According to MPEG-7 standard, ought to use CIE Lab space
  // This erraneous behaviour was decided to be kept unchanged 
  // to avoid a change in feature vector semantics


  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const{
    return vector<float>(6);
  }

  virtual int printMethodOptions(ostream&) const;

  virtual bool ProcessOptionsAndRemove(list<string>&); 

protected:

  virtual featureVector CalculateMask(char *mask);

  vector<featureVector > 
  GLA(vector<Feature::featureVector> &data,
      int initBins, int finalBins);

  vector<featureVector > 
  GLAApproximate(vector<Feature::featureVector> &data,
		 int initBins, int finalBins,int samplecount);
    
  void update_centers(vector<featureVector> &data, vector<int> &labels, 
		      vector<featureVector> &centers, int number_of_means);
  
  int update_labels(vector<featureVector> &data, vector<int> &labels, 
		    vector<featureVector> &centers, int number_of_means);

  featureVector 
  determinePerturbation(vector<featureVector> &data,
			vector<int> &labels,int bin);

  float totalDistortion(vector<featureVector> &data,
				      vector<int> &labels,
				      vector<featureVector> &bins);


  int findMaxDistortion(vector<featureVector> &data,
			vector<int> &labels,
			vector<featureVector> &bins);


};

}
#endif

// Local Variables:
// mode: font-lock
// End:
