// -*- C++ -*-  $Id: OCVFeature.C,v 1.8 2017/03/10 09:11:39 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)

#include <OCVFeature.h>

namespace picsom {

  /////////////////////////////////////////////////////////////////////////////

  bool OCVFeature::CalculateOneFrame(int f) {
    bool ok = true;
    
    int n = DataElementCount();
    
    for (int i=0; ok && i<n; i++)
      if (!CalculateOneLabel(f, i))
        ok = false;
    
    return ok;
  }  


  /////////////////////////////////////////////////////////////////////////////

  bool OCVFeature::CalculateOneLabel(int f, int l) {
    static const string msg = "OCVFeature::CalculateOneLabel() : ";

    if (LabelVerbose()) {
      cout << msg << "starting f=" << f << " l=" << l << endl;
      cout << msg << "  RegionDescriptorCount()=" <<
	RegionDescriptorCount();
      if ((size_t)l<RegionDescriptorCount())
	cout << " where " << l << " -> " << RegionDescriptor(l).first;
      cout << endl;
    }

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    
    string imagepath = CurrentFullDataFileName();

    try {
      if (1) {
	ocv_image_ = CurrentFrameOCV(); // cv::imread(imagepath);
      } else {
	cv::Mat temp = CurrentFrameOCV(); // cv::imread(imagepath);
	cv::Mat M = (cv::Mat_<double>(2,3) << 
		     1, 0, 10, 
		     0, 1, 0);
	cv::warpAffine(temp, ocv_image_, M, temp.size());
      }
    } catch (const string& excp) {
      cerr << msg << "OpenCV imread() caught exception [" << excp
           << "] on " << imagepath << endl;
      return false;
    }

    if (ocv_image_.empty()) {
      cerr << msg << "OpenCV imread() unable to open " << imagepath << endl;
      return false;
    }

    if (!ocv_image_.data) {
      cerr << msg << "Image data not loaded properly from " << imagepath
           << endl;
      return false;
    }

    if ((size_t)l<RegionDescriptorCount()) {
      imagedata rimg = RegionImage(l);
      if (LabelVerbose())
	cout << msg << "  RegionImage() returned " << rimg.info() << endl;
      ocv_image_ = Convert2cvMat(rimg);
    }

    bool r = CalculateOpenCV(f, l);

    if (LabelVerbose())
      cout << msg << "ending" << endl;

    return r;

#else
    return false;
#endif // HAVE_OPENCV2_CORE_CORE_HPP
  }

  /////////////////////////////////////////////////////////////////////////////

  void OCVFeature::SetVectorData(int i, const featureVector& fv) {
    string msg = "OCVFeature::SetVectorData("+ToStr(i)+",fv.size="+
      ToStr(fv.size())+") : ";

    if (FrameVerbose())
      cout << msg << endl;

    VectorData* vd = dynamic_cast<VectorData*>(GetData(i));
    if (!vd) {
      cerr << "OCVFeature::SetVectorData(" << i << "): no VectorData found.";
      return;
    }
    
    vd->setVector(fv);
  }      

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV
