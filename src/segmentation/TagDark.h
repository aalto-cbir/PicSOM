// -*- C++ -*-	$Id: TagDark.h,v 1.1 2005/11/08 17:51:48 vvi Exp $

#ifndef _TAGDARK_H_
#define _TAGDARK_H_

#include <Segmentation.h>

namespace picsom {
  class TagDark : public Segmentation {
  public:
  /// 
    TagDark();

    TagDark(bool b) : Segmentation(b) {}

    /// 
    virtual ~TagDark();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new TagDark(); }
    
    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "TagDark" : "td"; }

 
    virtual bool Process(); 

    virtual int ProcessOptions(int, char**);

    const char *Description() const 
    {return "Tags dark segments as background";};
    
  };
}

#endif // _TAGDARK_H_

// Local Variables:
// mode: font-lock
// End:
