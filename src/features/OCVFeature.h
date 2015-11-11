// -*- C++ -*-  $Id: OCVFeature.h,v 1.5 2013/02/25 13:50:22 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file OCVFeature.h

   \brief Declarations and definitions of class OCVFeature
   
   OCVFeature.h contains declarations and definitions of class the
   OCVFeature, which is a base class for feature calculations that use
   OpenCV to perform the feature extraction.
  
   \author Mats Sjöberg <mats.sjoberg@aalto.fi>
   $Revision: 1.5 $
   $Date: 2013/02/25 13:50:22 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _OCVFeature_h_
#define _OCVFeature_h_

#include <Feature.h>

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif // HAVE_OPENCV2_CORE_CORE_HPP

namespace picsom {

  /// Base class for external features
  class OCVFeature : public Feature {
  public:

    virtual vector<string> UsedDataLabels() const
    { return UsedDataLabelsFromSegmentData(); }

    virtual bool CalculateOneFrame(int f);
 
    virtual bool CalculateOneLabel(int f, int l);

    virtual featureVector CalculateRegion(const Region&)
    { throw "OCVFeature::CalculateRegion() not implemented."; }
  
    virtual treat_type DefaultWithinFrameTreatment() const
    { return treat_concatenate; }

    virtual treat_type DefaultBetweenFrameTreatment() const
    { return treat_separate; }

    virtual treat_type DefaultBetweenSliceTreatment() const
    { return treat_separate; }

    virtual pixeltype DefaultPixelType() const { return pixel_undef; }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const
    { return new VectorData(t, n, this); }
  
    virtual void deleteData(void *p)
    { delete (VectorData*)p; }

    void SetVectorData(int, const featureVector&);

    virtual featureVector getRandomFeatureVector() const
    { return featureVector(); }

  protected:
    /// This constructor is called by inherited class constructors.
    OCVFeature() {}

    /// This constructor doesn't add an entry in the methods list.
    OCVFeature(bool) : Feature(false) {}

    virtual bool CalculateOpenCV(int f, int l) = 0;

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    
    cv::Mat ocv_image_;
#endif // HAVE_OPENCV2_CORE_CORE_HPP
  };

}
#endif // _OCVFeature_h_

// Local Variables:
// mode: font-lock
// End:
