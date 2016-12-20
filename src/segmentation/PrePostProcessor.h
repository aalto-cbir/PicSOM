// -*- C++ -*-	$Id: PrePostProcessor.h,v 1.1 2000/08/10 09:34:55 vvi Exp $

#ifndef _PREPOSTPROCESSOR_H_
#define _PREPOSTPROCESSOR_H_

#include <Segmentation.h>

class PrePostProcessor : public Segmentation {
public:
  /// 
  PrePostProcessor();

  /// 
  PrePostProcessor(bool b) : Segmentation(b) {}

  /// 
  virtual ~PrePostProcessor();

  /// This base class doesn't contain implementation for all pure virtual 
  /// methods
  
  ///
  virtual bool Process();

protected:

  // new virtual member
  virtual bool PreProcess(){return true;};

  // new pure virtual member
  virtual bool DoProcess()=0;

  // new virtual member
  virtual bool PostProcess(){return true;};
  
};

#endif // _PREPOSTPROCESSOR_H_

// Local Variables:
// mode: font-lock
// End:
