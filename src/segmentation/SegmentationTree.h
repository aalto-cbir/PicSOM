// -*- C++ -*-	$Id: SegmentationTree.h,v 1.1 2006/09/19 08:08:49 vvi Exp $

#ifndef _SEGMENTATIONTREE_H_
#define _SEGMENTATIONTREE_H_

#include <Segmentation.h>
#include <Feature.h>
#include <FeatureInterface.h>
#include <queue>
#include <set>
#include <algorithm>
#include <list>

namespace picsom{

 class segmentationTree { 
    public:
      std::vector<coordList> startingRegions;
      std::vector<std::pair<int,int> > regionMerges; 
     
      void reset(){
	startingRegions.clear();
	regionMerges.clear();
	cout<<"reset: regionMerges.size() "<<regionMerges.size()<<endl;
      }

      int addStartingRegion(const coordList &l){ 
	// returns the label of newly added region (labeling starts from 0)

	if(!regionMerges.empty()){
	  cout << "size of Mergelist: "<<regionMerges.size() << endl;
	  throw std::string("segmentationTree:addStartingRegion: "
			    "list of merges not empty.");
	}
	startingRegions.push_back(l);
	return startingRegions.size()-1;
	
      }

      int addRegionMerge(int r1, int r2){ 
	// returns the label of the composite region

	cout << "Adding merge " << r1<<","<<r2<<endl;

	int nrStarts=startingRegions.size();
	regionMerges.push_back(std::pair<int,int>(r1,r2));

	return nrStarts+regionMerges.size()-1;
      }

 };

} // namespace picsom 

#endif // _SEGMENTATIONTREE_H_

// Local Variables:
// mode: font-lock
// End:
