// -*- C++ -*-  $Id: OCVKeyPoint.h,v 1.1 2011/09/23 13:46:54 mats Exp $
/**
   \file OCVKeyPoint.h

   \brief Declarations and definitions of class OCVKeyPoint
   
   OCVKeyPoint.h contains declarations and definitions of 
   class the OCVKeyPoint, which
   is a class that calculates some simple geometric descriptors.
  
   \author Mats Sjöberg <mats.sjoberg@aalto.fi>
   $Revision: 1.1 $
   $Date: 2011/09/23 13:46:54 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _OCVKeyPoint_
#define _OCVKeyPoint_

#include <OCVFeature.h>

namespace picsom {

  class OCVKeyPoint : public OCVFeature {
  public:

    OCVKeyPoint(const string& img = "",
                const list<string>& opt = (list<string>)0)
    { Initialize(img, "", NULL, opt); }

    OCVKeyPoint(const string& img, const string& seg,
                const list<string>& opt = (list<string>)0)
    { Initialize(img, seg, NULL, opt); }

    OCVKeyPoint(bool) : OCVFeature(false) {}

    virtual Feature *Create(const string& s) const
    { return (new OCVKeyPoint())->SetModel(s); }

    virtual string Name() const { return "OCVKeyPoint"; }
    virtual string LongName() const { return "OpenCV keypoint feature"; }
    virtual string ShortText() const { return LongName(); }
    virtual string TargetType() const { return "image"; }

    virtual string Version() const;

    int printMethodOptions(ostream&) const;

  protected:
    virtual bool ProcessOptionsAndRemove(list<string>&);

    virtual bool CalculateOpenCV(int f, int l);

    string detectorName;
    string descriptorName;
  };
}

#endif
