// -*- C++ -*-	$Id: SeparateMultipart.h,v 1.1 2005/02/04 14:13:37 vvi Exp $

#ifndef _SEPARATEMULTIPART_H_
#define _SEPARATEMULTIPART_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class SeparateMultipart : public Segmentation {
  public:
    /// 
    SeparateMultipart();

    /// 
    SeparateMultipart(bool b) : Segmentation(b) {}

    /// 
    virtual ~SeparateMultipart();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new SeparateMultipart(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"SeparateMultipart":"sm"; }

    ///
    virtual const char *Description() const { return "SeparateMultipart"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

  


  protected:
   
  };
} // namespace picsom
#endif // _SEPARATEMULTIPART_H_

// Local Variables:
// mode: font-lock
// End:
