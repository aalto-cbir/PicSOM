// -*- C++ -*-	$Id: GradientMagn.h,v 1.1 2000/10/19 08:14:04 vvi Exp $

#ifndef _GRADIENTMAGN_H_
#define _GRADIENTMAGN_H_

#include <PrePostProcessor.h>

class GradientMagn : public PrePostProcessor{
public:
  /// 
  GradientMagn();

  /// 
  GradientMagn(bool b) : PrePostProcessor(b) {}

  /// 
  virtual ~GradientMagn();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new GradientMagn(); }  

  ///
  const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"GradientMagn":"gm"; }

  ///
  virtual bool DoProcess();

  ///
  virtual int ProcessOptions(int, char**);

  virtual const char *Description() const {return "mymethod";};

protected:
};

#endif // _GRADIENTMAGN_H_

// Local Variables:
// mode: font-lock
// End:
