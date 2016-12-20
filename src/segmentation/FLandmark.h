// -*- C++ -*-	$Id: FLandmark.h,v 1.1 2012/11/07 10:51:43 jorma Exp $

#ifndef _FLANDMARK_H_
#define _FLANDMARK_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class FLandmark : public Segmentation {
  public:
    /// 
    FLandmark();

    /// 
    FLandmark(bool b) : Segmentation(b) {}

    /// 
    virtual ~FLandmark();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new FLandmark(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"FLandmark":"fl"; }

    ///
    virtual const char *Description() const { return "flandmark"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

    ///
    // virtual bool PreventOutput() const;
    
    ///
    // virtual bool OpenExtraFiles();

    ///
    virtual bool IncludeImagesInXML() const {return includeImages;}


    bool includeImages;


  protected:
    bool showseparate;
  };
} // namespace picsom
#endif // _FLANDMARK_H_

// Local Variables:
// mode: font-lock
// End:
