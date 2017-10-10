// -*- C++ -*-  $Id: OCVKeyPoint.C,v 1.4 2017/03/10 09:10:33 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)

#include <OCVKeyPoint.h>

#include <opencv2/features2d/features2d.hpp>

namespace picsom {
  static const string vcid =
    "$Id: OCVKeyPoint.C,v 1.4 2017/03/10 09:10:33 jormal Exp $";

  static OCVKeyPoint list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  string OCVKeyPoint::Version() const {
    return vcid;
  }

  /////////////////////////////////////////////////////////////////////////////

  int OCVKeyPoint::printMethodOptions(ostream &str) const {
    int ret = Feature::printMethodOptions(str);

    str << "detector=            "
        << "[Grid|Pyramid|Dynamic]FAST|STAR|SIFT|SURF|MSER|GFTT|HARRIS" << endl;
    str << "descriptor=          "
        << "[Opponent]SIFT|SURF|BRIEF" << endl;

    return ret+2;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVKeyPoint::ProcessOptionsAndRemove(list<string>& opts) {
    for (list<string>::iterator i = opts.begin(); i!=opts.end();) {
      string key, value;
      SplitOptionString(*i, key, value);

      if (key=="detector")
        detectorName = value;
      else if (key == "descriptor")
        descriptorName = value;
      else { // else continue without erasing option from list
        i++;
        continue;
      }

      i = opts.erase(i);
    }

    return Feature::ProcessOptionsAndRemove(opts);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVKeyPoint::CalculateOpenCV(int /*f*/, int l) {
    static const string msg = "OCVKeyPoint::CalculateOpenCV(): ";
    featureVector fv;

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    
    using namespace cv;

    // Detect keypoints
    Ptr<FeatureDetector> fd = FeatureDetector::create(detectorName);
    if (!fd) {
      cerr << msg << "Invalid feature detector: " << detectorName << endl;
      return false;
    }

    vector<KeyPoint> keypoints;
    fd->detect(CurrentFrameOCV(), keypoints);
    
    // Extract features
    Ptr<DescriptorExtractor> de = DescriptorExtractor::create(descriptorName);
    if (!de) {
      cerr << msg << "Invalid feature descriptor: " << descriptorName << endl;
      return false;
    }

    Mat descriptors;
    de->compute(CurrentFrameOCV(), keypoints, descriptors);

    if (descriptors.empty()) {
      cerr << msg << "No descriptors calculated for image" << BaseFileName()
           << endl;
      return false;
    }

    // FIXME: currently puts only first keypoint into data
    int dim = descriptors.cols;

    for (int i=0; i<dim; i++)
      fv.push_back(descriptors.at<double>(0,i));
#endif // HAVE_OPENCV2_CORE_CORE_HPP
    
    SetVectorData(l, fv);
    return true;
  }
    
}

#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

