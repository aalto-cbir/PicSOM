// -*- C++ -*-	$Id: RemoveHoles.h,v 1.3 2002/09/24 10:59:01 vvi Exp $

#ifndef _REMOVEHOLES_H_
#define _REMOVEHOLES_H_

#include <Segmentation.h>

namespace picsom {
  
  class RemoveHoles : public Segmentation {
  public:
    /// 
    RemoveHoles();
    
    /// 
    RemoveHoles(bool b) : Segmentation(b) {}
    
    /// 
    virtual ~RemoveHoles();
    
    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new RemoveHoles(); }  
    
    ///
    const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"RemoveHoles":"rh"; }

    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);
    
    virtual const char *Description() const {
      return "Fill all but largest background component";};
    
  protected:
  };
}
#endif // _REMOVEHOLES_H_

// Local Variables:
// mode: font-lock
// End:
