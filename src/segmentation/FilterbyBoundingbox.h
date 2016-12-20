// -*- C++ -*-	$Id: FilterbyBoundingbox.h,v 1.1 2005/11/17 09:43:22 vvi Exp $

#ifndef _FILTERBYBOUNDINGBOX_H_
#define _FILTERBYBOUNDINGBOX_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class FilterbyBoundingbox : public Segmentation {

  public:
    /// 
    FilterbyBoundingbox();

    /// 
    FilterbyBoundingbox(bool b) : Segmentation(b) {}

    /// 
    virtual ~FilterbyBoundingbox();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new FilterbyBoundingbox(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"FilterbyBoundingbox":"fb"; }

    ///
    virtual const char *Description() const { return "Filter segmentation file by bounding boxes specified in another segmentation file."; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

    ///
    // virtual bool PreventOutput() const;
    
    ///
    // virtual bool OpenExtraFiles();

    ///
    //    virtual bool IncludeImagesInXML() const {return includeImages;}



  protected:
    std::string input_pattern;
    bool byHierarchy;
  };
} // namespace picsom
#endif // _FILTERBYBOUNDINGBOX_H_

// Local Variables:
// mode: font-lock
// End:
