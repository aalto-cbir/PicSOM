// -*- C++ -*-	$Id: MergeColM.h,v 1.2 2003/08/11 08:20:41 vvi Exp $

#ifndef _MERGECOLM_H_
#define _MERGECOLM_H_

#include <Segmentation.h>
#include <Feature.h>
#include <FeatureInterface.h>
#include <queue>
#include <set>

namespace picsom{

  class MergeColM : public Segmentation {
  public:
    /// 
    MergeColM();

    MergeColM(bool b) :Segmentation(b) {}
    
    /// 
    virtual ~MergeColM();
    
    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new MergeColM(); }
    
    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "MergeColM" : "mc"; }
    
    ///
    // virtual bool Process(); inherited from parents
    
    virtual int ProcessOptions(int, char**);
    
    const char *Description() const 
    {return "Merging of adjacent regions.";};
    
  protected:
    
    virtual bool Process();
    
    bool listRegions();
    bool joinSmallRegions();
    void mergeRegions(int i1, int i2, bool updateFeatures);
    void fillPriorityQueue();
    void mergeTopRegions();
    void updateFeatureValues(int i);
    int findClosest(int i);
    std::list<string> divideOptions(std::string options);
   
    
    void dumpRegionList();

    int numberOfRegions;
    int sizeThreshold;
    int pixelsToUse;
    
    int regionCount;
    int currentTime;

    bool useAveraging;
    int weightDistances;
    float weightBias;
    float varBias;

 
    fInterface featureInterface; 
    int *lbl;

  public:
    class Region{
    public:
      int joinedTo;
      set<int> nList;
      coordList l;
      Feature::featureVector fV;
      int timestamp;
    };

    class regionDiff
    {
    public:
      int r1;
      int r2;
      float dist;
      int timestamp;
    };
    
    struct cmpdiff
    {
      bool operator()(const regionDiff &r1, const regionDiff&r2) const
      {
	return r1.dist<r2.dist; 
      }
    };
   
  protected:
    vector<Region> regionList;
    std::priority_queue<regionDiff,vector<regionDiff>,cmpdiff> queue;

  };

} // namespace picsom 

#endif // _MERGECOLM_H_

// Local Variables:
// mode: font-lock
// End:
