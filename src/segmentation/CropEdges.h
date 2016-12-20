// -*- C++ -*-  $Id: CropEdges.h,v 1.3 2003/10/29 12:31:08 jorma Exp $

#ifndef _CROPEDGES_H_
#define _CROPEDGES_H_

#include <Segmentation.h>

namespace picsom {
  class CropEdges : public Segmentation {
  public:
    /// 
    CropEdges() {}

    /// 
    CropEdges(bool b) : Segmentation(b) {}

    /// 
    virtual ~CropEdges() {}

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new CropEdges(); }  

    ///
    const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"CropEdges":"ce"; }

    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

    ///
    virtual const char *Description() const { return "cropedges"; }

  protected:

  };
}

#endif // _CROPEDGES_H_

// Local Variables:
// mode: font-lock
// End:
