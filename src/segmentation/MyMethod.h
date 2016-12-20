// -*- C++ -*-	$Id: MyMethod.h,v 1.9 2006/06/16 12:39:30 vvi Exp $

#ifndef _MYMETHOD_H_
#define _MYMETHOD_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class MyMethod : public Segmentation {
  public:
    /// 
    MyMethod();

    /// 
    MyMethod(bool b) : Segmentation(b) {}

    /// 
    virtual ~MyMethod();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new MyMethod(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"MyMethod":"mm"; }

    ///
    virtual const char *Description() const { return "mymethod"; }
    
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
#endif // _MYMETHOD_H_

// Local Variables:
// mode: font-lock
// End:
