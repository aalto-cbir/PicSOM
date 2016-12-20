// -*- C++ -*-	$Id: MergeSame.h,v 1.1 2003/07/03 13:51:45 vvi Exp $

#ifndef _MERGESAME_H_
#define _MERGESAME_H_

#include <Segmentation.h>

namespace picsom {
  class MergeSame : public Segmentation {
  public:
  /// 
    MergeSame();

    MergeSame(bool b) : Segmentation(b) {}

    /// 
    virtual ~MergeSame();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new MergeSame(); }
    
    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "MergeSame" : "ms"; }

 
    virtual bool Process(); 

    virtual int ProcessOptions(int, char**);

    const char *Description() const 
    {return "Merges labels of similar colours";};
    
  protected:
    float colour_tolerance;

  };

}

#endif // _MERGESAME_H_

// Local Variables:
// mode: font-lock
// End:
