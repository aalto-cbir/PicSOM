// -*- C++ -*-	$Id: EndBlocker.h,v 1.1 2000/10/27 10:00:18 vvi Exp $

#ifndef _ENDBLOCKER_H_
#define _ENDBLOCKER_H_

#include <Segmentation.h>
#include <stdio.h>

class EndBlocker : public Segmentation {
public:
  /// 
  EndBlocker();

  /// 
  EndBlocker(bool b) : Segmentation(b) {}

  /// 
  virtual ~EndBlocker();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new EndBlocker(); }  

  ///
  const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"EndBlocker":"eb"; }

  ///
  virtual bool Process();

  ///
  virtual int ProcessOptions(int, char**);

  virtual const char *Description() const {return "Prevents .seg file from being written if last method";};



protected:

  virtual bool CreateRealOutFile(const char * /*n*/, int w, int h);


};

#endif // _ENDBLOCKER_H_

// Local Variables:
// mode: font-lock
// End:
