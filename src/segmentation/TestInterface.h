// -*- C++ -*-	$Id: TestInterface.h,v 1.1 2004/05/21 13:32:36 vvi Exp $

#ifndef _TESTINTERFACE_H_
#define _TESTINTERFACE_H_

#include <Segmentation.h>
#include <Feature.h>
#include <FeatureInterface.h>


namespace picsom{

  class TestInterface : public Segmentation {
  public:
    /// 
    TestInterface();

    TestInterface(bool b) :Segmentation(b) {}
    
    /// 
    virtual ~TestInterface();
    
    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new TestInterface(); }
    
    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "TestInterface" : "in"; }
    
    ///
    // virtual bool Process(); inherited from parents
    
    virtual int ProcessOptions(int, char**);
    
    const char *Description() const 
    {return "Tests interface to feature calculation.";};

    virtual bool Process();
    
  protected:
 
    fInterface featureInterface; 
    int tileX1;
    int tileX2;
    int tileY1;
    int tileY2;
  };
} // namespace picsom 

#endif // _TESTINTERFACE_H_

// Local Variables:
// mode: font-lock
// End:
