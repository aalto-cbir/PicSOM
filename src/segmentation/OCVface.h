// -*- C++ -*-	$Id: OCVface.h,v 1.15 2012/12/01 15:54:30 jorma Exp $

#ifndef _OCVFACE_H_
#define _OCVFACE_H_

#include <Segmentation.h>
// #include <stdio.h>

#ifdef HAVE_OPENCV2_CORE_CORE_HPP

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

using namespace cv;

namespace picsom {

  class OCVface : public Segmentation {
  public:
    /// 
    OCVface();

    /// 
    OCVface(bool b) : Segmentation(b) {}

    /// 
    virtual ~OCVface();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new OCVface(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"OCVface":"of"; }

    ///
    virtual const char *Description() const { return "ocvface"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

    ///
    IplImage *CreateIplImage() const;

    ///
    // virtual bool PreventOutput() const;
    
    ///
    // virtual bool OpenExtraFiles();

    ///
    virtual bool IncludeImagesInXML() const {return includeImages;}

    ///
    const string& CascadePath() const {
      static string ret;
      if (ret=="")
	ret = PicSOMroot()+"/linux64/share/OpenCV";
      return ret;
    }

    /// Supported cascades
    enum cascadetype {
      cascade_frontal,
      cascade_profile
    };

    /// 
    const string CascadeName(cascadetype c) const {
      switch (c) {
      case cascade_frontal: return "haarcascades/haarcascade_frontalface_alt2.xml";
      case cascade_profile: return "haarcascades/haarcascade_profileface.xml";
      default: throw "OCVface:CascadeName() unknown cascadetype";
      }
    }

    const string ResultName(cascadetype c) const {
      switch (c) {
      case cascade_frontal: return "face";
      case cascade_profile: return "facep";
      default: throw "OCVface:ResultName() unknown cascadetype";
      }
    }
    
    /// Returns maximum overlap of rectangle r to the rectangles listed in all.
    double MaxOverlap(const Rect &r, const vector<Rect> &all);

    /// Overlap of two rectangles.
    double Overlap(const Rect &r1, const Rect &r2);
    
    bool includeImages;

  protected:
    /// Contains the cascades to be used for face detection, in this order.
    vector<cascadetype> used_cascades;

    /// The scaling factor for the input image before face detection. Larger 
    /// values make detection faster but smaller faces will not be detected.
    double scale;

    /// Controls the overlap allowed until a face returned by a non-primary
    /// cascade is rejected.
    double overlap_threshold;

    /// Option for CascadeClassifier::detectMultiScale():
    /// Specifies how much the image size is reduced at each image scale.
    double face_scalefactor; 

    /// Option for CascadeClassifier::detectMultiScale():
    /// Specifies how many neighbors each candiate rectangle should have to 
    /// retain it.
    int face_min_neighbors;
    
    /// Option for CascadeClassifier::detectMultiScale():
    /// Minimum possible object width. Objects smaller than that are ignored.
    int face_min_width;
    
    /// Option for CascadeClassifier::detectMultiScale():
    /// Minimum possible object height. Objects smaller than that are ignored.
    int face_min_height;
    
    /// true if only the biggest face should be detected
    bool only_biggest;    
  };
} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP
#endif // _OCVFACE_H_

// Local Variables:
// mode: font-lock
// End:
