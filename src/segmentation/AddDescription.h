// -*- C++ -*-  $Id: AddDescription.h,v 1.11 2012/06/05 14:00:07 jorma Exp $
// 
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _ADDDESCRIPTION_H_
#define _ADDDESCRIPTION_H_

#include <Segmentation.h>
//#include <stdio.h>

namespace picsom {

  class AddDescription : public Segmentation {
  public:
    /// 
    AddDescription();
    
    /// 
    AddDescription(bool b) : Segmentation(b) {
      methodname_long = "AddDescription";
      methodname_short = "ad";
    }
    
    /// 
    virtual ~AddDescription();
    
    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new AddDescription(); }  
    
    ///
    const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return (l ? methodname_long : methodname_short).c_str();
    }
    
    ///
    virtual bool Process();

    ///
    virtual bool ProcessGlobal();
    
    ///
    virtual int ProcessOptions(int, char**);
    
    ///
    virtual const char *Description() const {
      return "Add textual annotation from command line";
    }
    
    ///
    static bool CheckIfKnownType(const string& type);

    ///
    void SetMethodName(const string& s, const string& l) {
      methodname_short = s;
      methodname_long  = l; 
    }

    ///
    void addPendingAnnotation(const SegmentationResult& r,
			      int f = 0, bool allf = true, 
			      bool filer = true) {
      pendingAnnotation ann;
      ann.result = r;
      ann.frame = f;
      ann.allframes = allf;
      ann.fileresult = filer;

      pending.push_back(ann);
    }
    
    ///
    void addPendingAnnotation(const string& a, const string& b,
			      const string& c) {
      addPendingAnnotation(SegmentationResult(a, b, c));
    }

  protected:
    class pendingAnnotation {
    public:
      // 
      pendingAnnotation() : frame(-1), allframes(false), fileresult(false) {}

      // 
      string Str() const;

      // 
      SegmentationResult result;

      // 
      int frame;

      // 
      bool allframes;

      // 
      bool fileresult;

    }; // class pendingAnnotation
    
    //
    string methodname_long, methodname_short;

    // 
    std::vector<pendingAnnotation> pending; 

  }; // class AddDescription

} // namespace picsom

#endif // _ADDDESCRIPTION_H_

// Local Variables:
// mode: font-lock
// End:
