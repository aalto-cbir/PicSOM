// -*- C++ -*-	$Id: ListAdjacent.h,v 1.1 2005/11/17 09:43:22 vvi Exp $

#ifndef _LISTADJACENT_H_
#define _LISTADJACENT_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class ListAdjacent : public Segmentation {
  public:
    /// 
    ListAdjacent();

    /// 
    ListAdjacent(bool b) : Segmentation(b) {}

    /// 
    virtual ~ListAdjacent();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new ListAdjacent(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"ListAdjacent":"la"; }

    ///
    virtual const char *Description() const { return "List all combinations of adjacent regions"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

   
  protected:

  };
} // namespace picsom
#endif // _LISTADJACENT_H_

// Local Variables:
// mode: font-lock
// End:
