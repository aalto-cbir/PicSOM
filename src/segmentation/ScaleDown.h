// -*- C++ -*-	$Id: ScaleDown.h,v 1.1 2003/07/03 13:24:12 vvi Exp $

#ifndef _SCALEDOWN_H_
#define _SCALEDOWN_H_

#include <Segmentation.h>

namespace picsom{

  class ScaleDown : public Segmentation {
  public:
    /// 
    ScaleDown();

    /// 
    ScaleDown(bool b) : Segmentation(b) {}

    /// 
    virtual ~ScaleDown();
    
    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new ScaleDown(); }  
    
    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"ScaleDown":"sd"; }

    ///
    virtual const char *Description() const { return "scale down image"; }

    ///
    virtual bool Process();
    
    ///
    virtual int ProcessOptions(int, char**);

    ///
    // virtual bool PreventOutput() const;
    
    ///
    // virtual bool OpenExtraFiles();
    
  protected:


    int desiredWidth;
    
    bool saveOriginal;

  };
} // namespace picsom

#endif // _SCALEDOWN_H_

// Local Variables:
// mode: font-lock
// End:
