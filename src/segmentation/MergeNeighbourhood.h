// -*- C++ -*-	$Id: MergeNeighbourhood.h,v 1.1 2003/07/03 13:47:05 vvi Exp $

#ifndef _MERGENEIGHBOURHOOD_H_
#define _MERGENEIGHBOURHOOD_H_

#include <Segmentation.h>

namespace picsom {
  class MergeNeighbourhood : public Segmentation {
  public:
  /// 
    MergeNeighbourhood();

    MergeNeighbourhood(bool b) : Segmentation(b) {}

    /// 
    virtual ~MergeNeighbourhood();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new MergeNeighbourhood(); }
    
    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "MergeNeighbourhood" : "mn"; }

 
    virtual bool Process(); 

    virtual int ProcessOptions(int, char**);

    const char *Description() const 
    {return "Merges labels of similar colours in small neighbourhoods";};
    
  protected:
    int nbr_radius;
    float colour_tolerance;
    int size_threshold;

  };

}

#endif // _MERGENEIGHBOURHOOD_H_

// Local Variables:
// mode: font-lock
// End:
