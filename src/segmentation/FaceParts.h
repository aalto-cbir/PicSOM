// -*- C++ -*-	$Id: FaceParts.h,v 1.9 2007/05/10 11:09:23 rozyang Exp $

#ifndef _FACEPARTS_H_
#define _FACEPARTS_H_

#include <PixelTemplate.h>
#include <SpringModel.h>
#include <stdio.h>
#include <imagefile.h>

#include <LBModel.h>

namespace picsom {

class FaceParts : public SpringModel {
public:
  /// 
  FaceParts();

  /// 
  FaceParts(bool b) : SpringModel(b) {}

  /// 
  virtual ~FaceParts();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new FaceParts(); }  

  ///
  virtual const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"FaceParts":"fp"; }

  ///
  virtual const char *Description() const { return "faceparts"; }

  ///
  virtual bool Process_new();

  ///
  virtual bool Process();
  
  ///
  virtual bool ProcessRoz();

  ///
  virtual bool ProcessRoz2();

  ///
  virtual bool ProcessSingleFace(SegmentationResultList *resultlist, size_t fi);

  ///
  virtual bool DetectFacePart(string partname, const imagedata& imgface,
			      size_t x0, size_t y0, float theta,
			      float xc, float yc,
			      float detect_theta, float detect_tau);
  /// 
  virtual float Gradient_x( PixelTemplate *src, int x, int y );

  /// 
  virtual float Gradient_y( PixelTemplate *src, int x, int y );

  /// 
  virtual float E_img(PixelTemplate *s1, PixelTemplate *s2, 
		      PixelTemplate *s3, PixelTemplate *s4 );

  ///
  virtual float E_struct(PixelTemplate *re, PixelTemplate *le, 
			 PixelTemplate *no, PixelTemplate *mo);

  ///
  virtual float* Gradient_E_struct(PixelTemplate *re, PixelTemplate *le, 
				   PixelTemplate *no, PixelTemplate *mo);

  ///
  virtual float* Gradient_E_img(PixelTemplate *s1, PixelTemplate *s2, 
				PixelTemplate *s3, PixelTemplate *s4);

  ///
  virtual int ProcessOptions(int, char**);

  ///
  // virtual bool PreventOutput() const;

  ///
  // virtual bool OpenExtraFiles();

protected:
  size_t FACE_WIDTH; // WIDTH to be used in operations, initialized at constructor
  size_t FACE_HEIGHT; // HEIGHT to be used in operations, initialized at constructor
  bool DEBUG;     // print debugging info or not
  bool Iterate;   // use gradient-based optimization or not
  bool NotFound;   // all failure in case no facial part found
  char Fname_R_EYE[20];   // filenames of facial parts
  char Fname_L_EYE[20];
  char Fname_NOSE[20];
  char Fname_MOUTH[20];

  string _flmodel, _frmodel, _fnmodel, _fmmodel;
  string _fnonlmodel, _fnonrmodel, _fnonnmodel, _fnonmmodel;

  float theta_l, theta_r, theta_n, theta_m;
  float tau_l, tau_r, tau_n, tau_m;

  float scale_min, scale_augment, scale_max;
};

}

#endif // _FACEPARTS_H_

// Local Variables:
// mode: font-lock
// End:
