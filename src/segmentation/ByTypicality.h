// -*- C++ -*-	$Id: ByTypicality.h,v 1.1 2006/06/16 12:34:42 vvi Exp $

#ifndef _BYTYPICALITY_H_
#define _BYTYPICALITY_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class ByTypicality : public Segmentation {
  public:

    /// 
    ByTypicality();
    
    /// 
    ByTypicality(bool b) : Segmentation(b) {}
    
    /// 
    virtual ~ByTypicality();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new ByTypicality(); }  
    
    ///
    const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"ByTypicality":"bt"; }
    
    ///
    virtual bool Process();
    
    ///
    virtual int ProcessOptions(int, char**);
    
    virtual const char *Description() const { 
      return 
	"Re-segments image based on the given typicality scores";
    };
    
    ///
    virtual bool PreventOutput() const { return true; }
    
    ///
    virtual bool OpenExtraFiles();

    ///
    virtual bool CloseExtraFiles();

  protected:
    
    ///
    void SetResultName(const char *n){result_pattern=n;}
    
    ///
    void SetStandardResultNaming(){ SetResultName("%i.res");}
    //"segments/%m:%i.res");}
    
    ///
    std::string FormResultName();

    ///
    imagedata *showTypicality();
    
    ///
    void readScoreFile(string filename);

    ///
    string result_pattern;
    
    ///
    string result_filename;
    
    ///
    bool primionly;

    ///
    bool doScaling;

    ///
    bool modulateIntensity;

    map<string,map<set<int>,float> > scoremap;
    
    imagedata *resultframe;
    imagefile *resultfile;
    
  };
}

#endif // _BYTYPICALITY_H_

// Local Variables:
// mode: font-lock
// End:
