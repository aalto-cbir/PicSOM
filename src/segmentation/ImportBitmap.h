// -*- C++ -*-	$Id: ImportBitmap.h,v 1.4 2003/06/03 12:45:32 vvi Exp $

#ifndef _IMPORTBITMAP_H_
#define _IMPORTBITMAP_H_

#include <Segmentation.h>

namespace picsom{
  class ImportBitmap : public Segmentation {
  public:
    /// 
    ImportBitmap();
    
    /// 
    ImportBitmap(bool b) : Segmentation(b) {}
    
    /// 
    virtual ~ImportBitmap();
    
    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new ImportBitmap(); }  
    
    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"ImportBitmap":"ib"; }
    
    ///
    virtual const char *Description() const { return "A method for importing segmentations from bitmap files."; }
    
    ///
    virtual bool Process();
    
    ///
    virtual int ProcessOptions(int, char**);
    
    ///
    virtual bool ReadInputDescription(){ return false;}
    
    ///
    // virtual bool PreventOutput() const;
    
    ///
    // virtual bool OpenExtraFiles();
    
  protected:
    bool binarise;
    int threshold;
  };
} // namespace picsom
  
#endif // _IMPORTBITMAP_H_
  
  // Local Variables:
  // mode: font-lock
  // End:
