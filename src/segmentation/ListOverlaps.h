// -*- C++ -*-	$Id: ListOverlaps.h,v 1.3 2008/11/04 11:48:20 vvi Exp $

#ifndef _LISTOVERLAPS_H_
#define _LISTOVERLAPS_H_

#include <Segmentation.h>
#include <Feature.h>
#include <stdio.h>

namespace picsom {

  class ListOverlaps : public Segmentation {

  public:
    /// 
    ListOverlaps();

    /// 
    ListOverlaps(bool b) : Segmentation(b) {}

    /// 
    virtual ~ListOverlaps();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new ListOverlaps(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"ListOverlaps":"lo"; }

    ///
    virtual const char *Description() const { return "List overlaps of all segments with a specified bounding box"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

   
    virtual bool PreventOutput() const {return true;}
    
    ///
    // virtual bool OpenExtraFiles();

    ///
    //    virtual bool IncludeImagesInXML() const {return includeImages;}



  protected:


    std::vector<PredefinedTypes::Box> bb;

    bool showboxes;

    bool segmentareafraction;


  };
} // namespace picsom
#endif // _LISTOVERLAPS_H_

// Local Variables:
// mode: font-lock
// End:
