// -*- C++ -*-	$Id: KMeans.h,v 1.7 2004/08/03 14:50:00 vvi Exp $

#ifndef _KMEANS_H_
#define _KMEANS_H_

#include <Segmentation.h>

namespace picsom {
  class KMeans : public Segmentation {
  public:
  /// 
    KMeans();

    KMeans(bool b) : Segmentation(b) {}

    /// 
    virtual ~KMeans();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new KMeans(); }
    
    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "KMeans" : "km"; }

 
    virtual bool Process(); 

    virtual int ProcessOptions(int, char**);

    const char *Description() const 
    {return "Basic K means clustering based on RGB values.";};
    
  protected:
    virtual bool CalculateMeans(bool randomise=false);
    virtual bool CalculateGradient();
    virtual int AssignLabels();

  
    float *mean;
    float *variance;
    float *gr;
    RGBTriple *means;
  
    float *grad;
    float max_grad;

    unsigned int *lbl_part;
    float *fVal_part;
  
    int number_of_means;
    bool filter_line_edges;

    int partToUse;
    int pixelCount;

    bool filter_by_rgb;
    float filter_tolerance;

    bool randomizeUnused;
  
  };
}

#endif // _KMEANS_H_

// Local Variables:
// mode: font-lock
// End:
