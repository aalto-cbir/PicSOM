// -*- C++ -*-	$Id: TestSegment.h,v 1.6 2002/01/03 07:50:27 jorma Exp $

#ifndef _TESTSEGMENT_H_
#define _TESTSEGMENT_H_

#include <PrePostProcessor.h>

class TestSegment : public PrePostProcessor{
public:
  /// 
  TestSegment() : limit(0) {}

  TestSegment(bool b) : PrePostProcessor(b) {}

  /// 
  virtual ~TestSegment(){}

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new TestSegment(); }

  ///
  virtual const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  virtual const char *MethodName(bool l = false) const {
    return l ? "TestSegment" : "ts"; }

  ///
  virtual bool DoProcess();

  ///
  virtual const char *Description() const {return "Testbed for segmentation";};

  ///
  virtual int ProcessOptions(int, char**);

  ///
  void GeneratePattern(int ind);
  void SetSegmentationRegion(const coord &nw, const coord &se,int label);

protected:
  ///
  float limit;

private:
};

#endif // _TESTSEGMENT_H_

// Local Variables:
// mode: font-lock
// End:
