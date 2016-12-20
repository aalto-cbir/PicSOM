// -*- C++ -*-	$Id: Random.h,v 1.1 2003/07/03 13:27:44 vvi Exp $

#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <Segmentation.h>

namespace picsom{
  class Random : public Segmentation {
  public:
    /// 
    Random();

    Random(bool b) :Segmentation(b) {}

    /// 
    virtual ~Random();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new Random(); }

    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "Random" : "ra"; }
    
    virtual bool Process(); 
    
    virtual int ProcessOptions(int, char**);
    
    const char *Description() const 
    {return "Random assignment of segment labels";};
    
  protected:

    int numberOfMeans;
    
  };
} // namespace picsom

#endif // _RANDOM_H_

// Local Variables:
// mode: font-lock
// End:
