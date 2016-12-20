// -*- C++ -*-	$Id: PickLargest.h,v 1.5 2002/09/24 11:01:23 vvi Exp $

#ifndef _PICKLARGEST_H_
#define _PICKLARGEST_H_

#include <Segmentation.h>
namespace picsom{

class PickLargest : public Segmentation {
public:
  /// 
  PickLargest();

  /// 
  PickLargest(bool b) : Segmentation(b) {}

  /// 
  virtual ~PickLargest();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new PickLargest(); }  

  ///
  const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"PickLargest":"pl"; }

  ///
  virtual bool Process();

  ///
  virtual int ProcessOptions(int, char**);

  virtual const char *Description() const 
  {return "Removal of all but largest foreground region.";};

protected:
};
}
#endif // _PICKLARGEST_H_

// Local Variables:
// mode: font-lock
// End:
