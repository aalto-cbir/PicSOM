// -*- C++ -*-	$Id: Binarise.h,v 1.2 2005/11/08 17:51:25 vvi Exp $

#ifndef _BINARISE_H_
#define _BINARISE_H_

#include <Segmentation.h>

namespace picsom {
  class Binarise : public Segmentation {
  public:
  /// 
    Binarise();

    Binarise(bool b) : Segmentation(b) {}

    /// 
    virtual ~Binarise();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new Binarise(); }
    
    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "Binarise" : "bi"; }

 
    virtual bool Process(); 

    virtual int ProcessOptions(int, char**);

    const char *Description() const 
    {return "Chooses one segment to be background (0) and aggregates others as foreground (1).";};

  protected:
    
    int backgroundLabel;
    
  };
}

#endif // _BINARISE_H_

// Local Variables:
// mode: font-lock
// End:
