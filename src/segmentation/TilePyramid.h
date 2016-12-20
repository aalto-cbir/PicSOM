// -*- C++ -*-	$Id: TilePyramid.h,v 1.2 2008/06/12 15:44:48 vvi Exp $

#ifndef _TILEPYRAMID_H_
#define _TILEPYRAMID_H_

#include <Segmentation.h>

namespace picsom{
  class TilePyramid : public Segmentation {
  public:
    /// 

    TilePyramid();

    TilePyramid(bool b) :Segmentation(b) {}

    /// 
    virtual ~TilePyramid();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new TilePyramid(); }

    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "TilePyramid" : "tp"; }
    
    virtual bool Process(); 
    
    virtual int ProcessOptions(int, char**);
    
    const char *Description() const 
    {return "Creates quadtree segmentation of the image";};
    
  protected:

    int levels;

  };
} // namespace picsom

#endif // _TILEPYRAMID_H_

// Local Variables:
// mode: font-lock
// End:
