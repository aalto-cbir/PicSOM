#ifndef _LOCATEFACEBOX_H_
#define _LOCATEFACEBOX_H_

#include <Segmentation.h>

#include <LBModel.h>

namespace picsom {
  class LocateFaceBox : public Segmentation {
  public:    
    /// 
    LocateFaceBox() {
    	_theta = 1000;
    	_tau = -400;
    	_scale_min = 20, _scale_augment = 1.05, _scale_max = 70;
    	_rotate_min = -20, _rotate_step = 5, _rotate_max = 20;
	_ffacemodel = "lbmodel/face_model.txt";
	_fnonfacemodel = "lbmodel/non_face_model.txt";
    	_pFace_model = NULL;
    	_pNonface_model = NULL;
    }

    /// 
    LocateFaceBox(bool b) : Segmentation(b) {}

    /// 
    virtual ~LocateFaceBox() {}

    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new LocateFaceBox(); }  

    ///
    const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"LocateFaceBox":"lb"; }

    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

    virtual const char *Description() const {return "locatefacebox";};

  protected:
    
    void DetectFaces(list<LBBox>& faces);
    
    void ShowFaceBox(LBBox& fa);
    void DrawLine(imagedata& img, int x1, int y1, int x2, int y2);
    
  
    float _theta;
    float _tau;
    float _scale_min, _scale_augment, _scale_max;
    float _rotate_min, _rotate_step, _rotate_max;
    
    string _ffacemodel;
    string _fnonfacemodel;
    
    LBModel* _pFace_model;
    LBModel* _pNonface_model;
 };

} // namespace picsom

#endif // _LOCATEFACEBOX_H_

// Local Variables:
// mode: font-lock
// End:
