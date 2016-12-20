// -*- C++ -*-	$Id: FilterbyColor.h,v 1.4 2007/05/30 07:28:46 jorma Exp $

#ifndef _FILTERBYCOLOR_H_
#define _FILTERBYCOLOR_H_

#include <Segmentation.h>
#include <stdio.h>

namespace picsom {

  class FilterbyColor : public Segmentation {

  public:
    /// 
    FilterbyColor();

    /// 
    FilterbyColor(bool b) : Segmentation(b) {}

    /// 
    virtual ~FilterbyColor();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new FilterbyColor(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"FilterbyColor":"cf"; }

    ///
    virtual const char *Description() const {
      return "Filter segmentation file by classification in a color space.";
    }
    
    ///
    virtual int ProcessOptions(int, char**);

    ///
    virtual bool Process();

    ///
    bool ProcessBox(const PredefinedTypes::Box&, int, const string&,
		    bool, size_t&, size_t&);

  protected:

    ///
    enum colorspace {
      cs_rgb,      
      cs_ycbcr
    };

    const string ColorSpaceName(colorspace c) const {
      switch (c) {
      case cs_rgb:   return "RGB";
      case cs_ycbcr: return "YCbCr";
      default: throw "OCVface:ColorSpaceName() unknown colorspace";
      }
    }

    colorspace used_colorspace;

    enum primitive {
      pr_box,      
      pr_circle
    };

    primitive used_primitive;

    vector<float> primitiveValues;

    ///
    bool IsInsidePrimitive(double, double, double);

    /// Parses the "-p" option, supports currently only "-p box(a,b,c,d,e,f)" 
    void ParsePrimitive(const string &p);

    // threshold (proportion of pixels needed) for accepting the segmentation
    double threshold;
    
    // whether to write the original and mask images (showing pixels that are inside the primitive)
    bool writemask;

    //
    string maskprefix;

  };
} // namespace picsom
#endif // _FILTERBYCOLOR_H_

// Local Variables:
// mode: font-lock
// End:
