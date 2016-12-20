// -*- C++ -*-	$Id: ChainCode.h,v 1.9 2001/06/05 13:32:55 vvi Exp $

#ifndef _CHAINCODE_H_
#define _CHAINCODE_H_

#include <TestSegment.h>

class ChainCode : public TestSegment {
public:
  ///
  ChainCode();

  /// 
  ChainCode(bool b) : TestSegment(b) {}

  /// 
  virtual ~ChainCode();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new ChainCode(); }  

  ///
  virtual const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"ChainCode":"cc"; }

  ///
  virtual bool DoProcess();

  virtual const char *Description() const 
  {return "Enumeration of pixels on segment borders";};
  

};

#endif // _CHAINCODE_H_

// Local Variables:
// mode: font-lock
// End:
