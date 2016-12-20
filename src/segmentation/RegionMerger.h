// -*- C++ -*-	$Id: RegionMerger.h,v 1.1 2006/06/16 12:36:23 vvi Exp $

#ifndef _REGIONMERGER_H_
#define _REGIONMERGER_H_

#include <segmentfile.h>
#include <Feature.h>
#include <stdio.h>
#include <FeatureInterface.h>
#include <SegmentationTree.h>
#include <queue>
#include <set>
#include <algorithm>
#include <list>

namespace picsom {

#define WEIGHTBIAS 0.002

  // void printSet(const std::set<int>&s,ostream &o){
  //  set<int>::const_iterator it;
  //  for(it=s.begin();it!=s.end();it++)
  //    o << *it << " ";
  //}

  int random(int m);

  enum feature_update_t {no_update,choose_larger,full_update};

  class RegionMerger {

  public:
    RegionMerger(segmentfile *i=NULL): img(i){      


      numberOfRegions=5;
      sizeThreshold=600;
      useAveraging=false;
      weightDistances=0;
      pixelsToUse=-1;
      weightBias=WEIGHTBIAS;
      fourNeighbours=false;
      saturationLimit=10; // larger that 2* (area of whole image)
      edgeWeight=0;
      
      outputTree=false;
      treeLeavesAsBitmap=false;
      initialApproximate=false;
      verbose=0;
      clearThreshold=10;
    }

    ~RegionMerger(){

    }
    

    void clear(){
      regionList.clear();
      queue_vector.clear();
    }

    typedef set<coord> coordSet;

    class borderInfo{
    public:

      borderInfo(){ reset();};

      void reset(){
	count=0;
	strength=0;
      };

      void add(const borderInfo &o){
	count += o.count;
	strength += o.strength;
      }

      int count;
      float strength;
    };


    class Region{
    public:
      //int joinedTo;

      Region(){
	count=0;
	timestamp=-1;
      }

      int currentLabel; // for recording the segmentation tree info
      set<int> nList;
      int closestNeighbour;
      map<int,borderInfo> bI;
      vector<int> rl;
      int count;
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

    int numberOfRegions;
    int sizeThreshold;
    int pixelsToUse;
    bool outputTree;
    bool treeLeavesAsBitmap;
    
    // int regionCount;
    int currentTime;

    bool useAveraging;
    bool initialApproximate;

    float edgeWeight;

    int weightDistances;
    float weightBias;
    float saturationLimit;

    bool fourNeighbours;
    int clearThreshold;

 
    fInterface featureInterface; 

    segmentationTree hierarchy;

    map<int,coordList> orgRegions;

    void mergeBb(const PredefinedTypes::Box *b=NULL,int desiredCount=-1,
		 bool joinSmall=true);

    bool listRegions(const PredefinedTypes::Box *b=NULL);
    bool joinSmallRegions();
    void mergeRegions(int i1, int i2, feature_update_t updateFeatures,
		      bool showDebug=false);
    void fillPriorityQueue();
    void mergeQueuedRegions(int desiredCount);
    void mergeTopRegions();
    void initialApproximateMerge();
    void mapListToBitmap();
    void updateFeatureValues(int i);
    int findClosest(int i);
    float sizeWeighting(float s1,float s2);   
    
    void dumpRegionList(bool labelsonly=false);

    std::map<int,Region> *getRegionList(){return &regionList;}

    segmentfile *getImage() const {return img;}
    void setImage(segmentfile *i){img=i;}


    int Verbose() const{ return verbose;}
    void Verbose(int v){verbose=v;}

  protected:

    segmentfile *img;
    imagedata edgeimg;

    int verbose;

    float regionDist(int i1, int i2);
    float borderStrength(int i1,int i2);
    float borderStrength(const coordSet &s1, 
			 const coordSet &s2);
    float borderLengthNorm(int n);
    void detectEdges();
    
    void checkQueue();
    int countRegions();

    std::map<int,Region> regionList;
    vector<regionDiff> queue_vector;

  };

} // namespace picsom
#endif // _REGIONMERGER_H_

// Local Variables:
// mode: font-lock
// End:
