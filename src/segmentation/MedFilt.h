// -*- C++ -*-	$Id: MedFilt.h,v 1.1 2000/10/02 13:02:12 vvi Exp $

#ifndef _MEDFILT_H_
#define _MEDFILT_H__

#include <PrePostProcessor.h>

class MedFilt : public PrePostProcessor {
public:
  /// 
  MedFilt();

  /// 
  MedFilt(bool b) : PrePostProcessor(b) {}

  /// 
  virtual ~MedFilt();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new MedFilt(); }  

  ///
  const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"MedFilt":"mf"; }

  ///
  virtual bool DoProcess();

  ///
  virtual int ProcessOptions(int, char**);

  virtual const char *Description() const {return "median filter";};

protected:
};

#endif // _MEDFILT_H_

// Local Variables:
// mode: font-lock
// End:
