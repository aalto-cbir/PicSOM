// -*- C++ -*-	$Id: RegionMerge.h,v 1.6 2006/06/16 12:31:44 vvi Exp $

#ifndef _REGIONMERGE_H_
#define _REGIONMERGE_H_

#include <Segmentation.h>
#include <Feature.h>
#include <SegmentationTree.h>
#include <RegionMerger.h>
#include <queue>
#include <set>
#include <algorithm>
#include <list>

namespace picsom{

  class RegionMerge : public Segmentation {
  public:

    /// 
    RegionMerge();

    RegionMerge(bool b) :Segmentation(b) {}
    
    /// 
    virtual ~RegionMerge();
    
    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new RegionMerge(); }
    
    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "RegionMerge" : "rm"; }
    
    ///
    // virtual bool Process(); inherited from parents
    
    virtual int ProcessOptions(int, char**);
    
    const char *Description() const 
    {return "Merging of adjacent regions.";};

    virtual bool Process();
    
  protected:

    // std::list<string> divideOptions(std::string options);


    int numberOfRegions;
    int sizeThreshold;
    int pixelsToUse;
    bool outputTree;
    bool treeLeavesAsBitmap;
    
    int regionCount;
    int currentTime;

    bool useAveraging;
    bool initialApproximate;

    float edgeWeight;

    int weightDistances;
    float weightBias;
    float saturationLimit;

    bool fourNeighbours;
 
    //segmentationTree hierarchy;
    RegionMerger *merger;

    imagedata edgeimg;

  public:

    typedef set<coord> coordSet;

    class Region{
    public:
      int joinedTo;
      int currentLabel; // for recording the segmentation tree info
      set<int> nList;
      int closestNeighbour;
      map<int,coordSet> borderInfo;
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

      bool operator<(const regionDiff &o) const
      {
	return dist<o.dist; 
      }

      std::string print(){
	std::stringstream ss;
	ss << "r1="<<r1<<" r2="<<r2<<" dist="<<dist<<" timestamp="
	   << timestamp;
	return ss.str();
      }

    };
    
    struct cmpdiff
    {
      bool operator()(const regionDiff &r1, const regionDiff&r2) const
      {
	return r1.dist<r2.dist; 
      }
    };
   
   

  protected:
    
    void checkQueue();
    int countRegions();

    vector<Region> regionList;
    //    std::priority_queue<regionDiff,vector<regionDiff>,cmpdiff> queue;
    vector<regionDiff> queue_vector;

  };

} // namespace picsom 

#endif // _REGIONMERGE_H_

// Local Variables:
// mode: font-lock
// End:
