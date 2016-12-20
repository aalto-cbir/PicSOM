// -*- C++ -*-	$Id: SampleRandom.h,v 1.1 2006/09/19 08:08:49 vvi Exp $

#ifndef _SAMPLERANDOM_H_
#define _SAMPLERANDOM_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class SampleRandom : public Segmentation {
  public:
    /// 
    SampleRandom();

    /// 
    SampleRandom(bool b) : Segmentation(b) {}

    /// 
    virtual ~SampleRandom();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new SampleRandom(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"SampleRandom":"sr"; }

    ///
    virtual const char *Description() const { return "Creates specified number on randomly placed rectangular segments"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

  protected:
    int numberOfRegions;
    int xSize;
    int ySize;
    float xFrac;
    float yFrac;
  };
} // namespace picsom
#endif // _SAMPLERANDOM_H_

// Local Variables:
// mode: font-lock
// End:
