// -*- C++ -*-	$Id: FilterCentral.h,v 1.2 2005/02/16 10:29:21 vvi Exp $

#ifndef _FILTERCENTRAL_H_
#define _FILTERCENTRAL_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class FilterCentral : public Segmentation {

    enum dest_type{ std_out,xml,bitmap } ;
    enum criterion_type {touch, touch_per_perimeter, touch_per_area};

  public:
    /// 
    FilterCentral();

    /// 
    FilterCentral(bool b) : Segmentation(b) {}

    /// 
    virtual ~FilterCentral();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new FilterCentral(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"FilterCentral":"fc"; }

    ///
    virtual const char *Description() const 
    { return "Filter out some regions"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

    
    virtual bool PreventOutput() const{
      switch(output_destination){
      case xml:
      case bitmap:
	return false;
      default:
	return true;
      }
    }
    
   


  protected:
    dest_type output_destination;
    int background_label;

    criterion_type filter_criterion;
    float perimeterFrac;
    float areaFrac;
    
  };
} // namespace picsom
#endif // _FILTERCENTRAL_H_

// Local Variables:
// mode: font-lock
// End:
