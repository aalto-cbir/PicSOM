// -*- C++ -*-	$Id: TemporalMorphology.h,v 1.2 2014/10/28 09:43:45 jorma Exp $

#ifndef _TEMPORALMORPHOLOGY_H_
#define _TEMPORALMORPHOLOGY_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class TemporalMorphology : public Segmentation {
  public:
    /// 
    TemporalMorphology();

    /// 
    TemporalMorphology(bool b) : Segmentation(b) {}

    /// 
    virtual ~TemporalMorphology();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new TemporalMorphology(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"TemporalMorphology":"tm"; }

    ///
    virtual const char *Description() const { return "temporalmorphology"; }
    
    ///
    virtual bool Process() { return true; }

    ///
    virtual bool ProcessGlobal();

    ///
    virtual int ProcessOptions(int, char**);

  protected:
    ///
    float dilate, erode;

  }; // class TemporalMorphology

} // namespace picsom

#endif // _TEMPORALMORPHOLOGY_H_

// Local Variables:
// mode: font-lock
// End:
