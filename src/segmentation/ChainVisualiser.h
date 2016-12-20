// -*- C++ -*-	$Id: ChainVisualiser.h,v 1.2 2000/08/10 09:36:57 vvi Exp $

#ifndef _CHAINVISUALISER_H_
#define _CHAINVISUALISER_H_

#include <PrePostProcessor.h>

class ChainVisualiser : public PrePostProcessor {
public:
  /// 
  ChainVisualiser();

  /// 
  ChainVisualiser(bool b) : PrePostProcessor(b) {}

  /// 
  virtual ~ChainVisualiser();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new ChainVisualiser(); }  

  ///
  const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"ChainVisualiser":"cv"; }

  ///
  virtual bool DoProcess();

  ///
  virtual int ProcessOptions(int, char**);

  virtual const char *Description() const {return "Chain Code Visualiser";};

protected:
};

#endif // _CHAINVISUALISER_H_

// Local Variables:
// mode: font-lock
// End:
