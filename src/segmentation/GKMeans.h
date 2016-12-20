// -*- C++ -*-	$Id: GKMeans.h,v 1.1 2003/07/03 13:22:06 vvi Exp $

#ifndef _GKMEANS_H_
#define _GKMEANS_H_

#include <Segmentation.h>
#include <FeatureInterface.h>

namespace picsom{
  class GKMeans : public Segmentation {
  public:
    /// 
    GKMeans();

    GKMeans(bool b) :Segmentation(b) {}

    /// 
    virtual ~GKMeans();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new GKMeans(); }

    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "GKMeans" : "gk"; }
    
    virtual bool Process(); 
    
    virtual int ProcessOptions(int, char**);
    
    const char *Description() const 
    {return "Generalised k means clustering.";};
    
  protected:
    virtual bool CalculateMeans();
    virtual int AssignLabels();
    
    void divideOptions(string options,int &argc, char**&argv, char *&str);
    void calculateFeatures(int x, int y, Feature::featureVector &v);
    void recalculateFeatures(int x, int y, Feature::featureVector &v);
    
    coord dim;
    
    vector<Feature::featureVector> mean;
    vector<Feature::featureVector> variance;
    
    vector<int> weight;
    
    bool useWeights;
    bool estimateVariance;
    
    bool useMRFRelaxation;
    bool refineResult;
    
    int xTileSize;
    int yTileSize;
    
    int numberOfMeans;
    int partToUse;
    
    int pixelCount;
    
    fInterface featureInterface;
    
    char *lbl;
    Feature::featureVector *fVal;
    
    char *lbl_part;
    Feature::featureVector *fVal_part;
    
    Feature::featureVector *rVal;
    
  };
} // namespace picsom

#endif // _GKMEANS_H_

// Local Variables:
// mode: font-lock
// End:
