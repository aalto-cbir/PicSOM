// -*- C++ -*-  $Id: GenerateHeatmap.h,v 1.1 2008/11/04 11:50:09 vvi Exp $
// 
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _GENERATEHEATMAP_H_
#define _GENERATEHEATMAP_H_

#include <Segmentation.h>
#include <videofile.h>
#include <stdio.h>

namespace picsom {

  class GenerateHeatmap : public Segmentation {
  public:

    /// 
    GenerateHeatmap();
    
    /// 
    GenerateHeatmap(bool b) : Segmentation(b) {}
    
    /// 
    virtual ~GenerateHeatmap();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new GenerateHeatmap(); }  
    
    ///
    const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"GenerateHeatmap":"gh"; }
    
    ///
    virtual bool Process();
    
    ///
    virtual int ProcessOptions(int, char**);
    
    virtual const char *Description() const { 
      return 
	"Generates a hetmap from a given segmentation and associated score file";
    };
    
    ///
    virtual bool PreventOutput() const { return true; }
    
    ///
    virtual bool OpenExtraFiles();

    ///
    virtual bool CloseExtraFiles();

    ///
    void AddResultFrame(imagedata& d);



  protected:
    
    ///
    void SetResultName(const char *n){result_pattern=n;}
    
    ///
    std::string FormResultName();
    
    ///

    string result_pattern;
    
    ///
    string result_filename;
    
    imagedata *resultframe;
    imagefile *resultfile;

    std::string scorefilename;

    bool no_interpolation;

    
  };
}

#endif // _GENERATEHEATMAP_H_

// Local Variables:
// mode: font-lock
// End:
