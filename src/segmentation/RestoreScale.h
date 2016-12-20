// -*- C++ -*-	$Id: RestoreScale.h,v 1.2 2004/08/03 15:01:34 vvi Exp $

#ifndef _RESTORESCALE_H_
#define _RESTORESCALE_H_

#include <Segmentation.h>

namespace picsom{

  class RestoreScale : public Segmentation {
  public:
    /// 
    RestoreScale();

    /// 
    RestoreScale(bool b) : Segmentation(b) {}

    /// 
    virtual ~RestoreScale();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new RestoreScale(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"RestoreScale":"rs"; }

    ///
    virtual const char *Description() const { 
      return "Restore image to scale prior to ScaleDown"; }
    
    ///
    virtual bool Process();
    
    ///
    virtual int ProcessOptions(int, char**);

    ///
    virtual bool Cleanup();

    ///
    // virtual bool PreventOutput() const;
    
    ///
    // virtual bool OpenExtraFiles();

  protected:
    bool discardImage;

  };
} // namespace picsom

#endif // _RESTORESCALE_H_

// Local Variables:
// mode: font-lock
// End:
