#include <RegionMerger.h>
#include <math.h>

//static const string vcid =
//"@(#)$Id: RegionMerger.C,v 1.4 2009/04/30 10:09:26 vvi Exp $";

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::set;
using std::make_heap;
using std::push_heap;
using std::pop_heap;
using std::list;

namespace picsom {


#define WEIGHTBIAS 0.002

  ///////////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::mergeBb(const PredefinedTypes::Box *b,int desiredCount,bool joinSmall){

    if(Verbose()>1) cout << "in RegionMerger::mergeBb" << endl;

    cout << "verbose level: " << Verbose() << endl;

    if(desiredCount<0) desiredCount=numberOfRegions;

    listRegions(b);

    if(Verbose()>1) cout << "regions listed" << endl;

       if(joinSmall)
      joinSmallRegions();
    if(Verbose()>1) cout << "small regions joined" << endl;
       if(initialApproximate){
      initialApproximateMerge();
      if(Verbose()>1) 
	cout << "initial approximate merging completed" << endl;    
       }
       //dumpRegionList(true);
       fillPriorityQueue();
       //dumpRegionList(true);
       if(Verbose()>1) cout << "priority queue filled" << endl;
    mergeQueuedRegions(desiredCount);
    mapListToBitmap();
  }

  ///////////////////////////////////////////////////////////////////////////

  bool RegionMerger::listRegions(const PredefinedTypes::Box *b)
  {

    PredefinedTypes::Box bb;

    if(b)
      bb=*b;
    else{
      bb.ulx=bb.uly=0;
      bb.brx=img->getWidth()-1;
      bb.bry=img->getHeight()-1;
    }

    // enumerate dirs
    
    // 0 - E
    // 1 - SW
    // 2 - S
    // 3 - SE
    
    int incr[4];

    int w=bb.width(),h=bb.height();

    int l=w*h,i;
    unsigned char *isNeighbour= new unsigned char[l];
    
    incr[0]=1;
    incr[1]=w-1;
    incr[2]=w;
    incr[3]=w+1;
    
    const int xincr[]={1,-1,0,1};
    const int yincr[]={0,1,1,1};

    for(i=0;i<l;i++){
      isNeighbour[i]=15;
    }
    for(i=0;i<w;i++){
      isNeighbour[l-i-1] = 1; // lower neighbours away
    }
    
    for(i=0;i<l;i+=w){
      isNeighbour[i] &= 13; // SW neighbours away 
      isNeighbour[i+w-1] &= 6; // right neighbours away
    }
    
    int x,y,dir;

    int *lbl=img->GetSeparateLabelingInt(NULL,fourNeighbours,false,&bb);

    if(Verbose()>1)
    cout << "separate labeling formed" << endl;
    
    regionList.clear();
    orgRegions.clear();
    
    map<pair<int,int>,coordSet> borderPixels;

    for(i=0,x=bb.ulx,y=bb.uly;i<l;i++,x++){
      if(x>bb.brx){ x=bb.ulx;y++;}
      
      // cout<<"i= "<<i<<" (x,y)="<<x<<","<<y<<")"<<endl;

      //regionList[lbl[i]].joinedTo=lbl[i];
      orgRegions[lbl[i]].push_back(coord(x,y));
      //regionList[lbl[i]].l.push_back(coord(x,y));

      for(dir=0;dir<4;dir++){
	if((isNeighbour[i]&(1<<dir)) && ((fourNeighbours==false) || (dir&1)==0 ))
	  if(lbl[i]!=lbl[i+incr[dir]]){
	    
	    int r1=lbl[i], r2=lbl[i+incr[dir]];

	    regionList[r1].nList.insert(r2);
	    regionList[r2].nList.insert(r1);

	    if(edgeWeight != 0.0){
	      // this pixel pair is also inserted in the list of border pixels between the
	      borderPixels[pair<int,int>(r1,r2)].
		insert(coord(x+xincr[dir],y+yincr[dir]));
	      borderPixels[pair<int,int>(r2,r1)].
		insert(coord(x,y));
	    }
	  }
      }
	  
    }
    
    if(Verbose()>1)
      cout << "regionlist collected w/ " << regionList.size() << " regions" << endl;

    if(edgeWeight != 0){
      // calculate the edge magnitude image
      
      if(Verbose()>1) cout << "detecting edges"<< endl;
      detectEdges();
      if(Verbose()>1) cout << "edges detected" << endl;
    }

    map<pair<int,int>,coordSet>::iterator bit;
    for(bit=borderPixels.begin(); bit != borderPixels.end(); bit++){
      const int r1=bit->first.first, r2=bit->first.second;
      if(r1<r2){

	regionList[r1].bI[r2].strength= regionList[r2].bI[r1].strength
	  =borderStrength(bit->second,borderPixels[pair<int,int>(r2,r1)]);

	regionList[r1].bI[r2].count= regionList[r2].bI[r1].count=
	  bit->second.size()+borderPixels[pair<int,int>(r2,r1)].size();
      }
    }

    cout << "border strengths determined" << endl;
    
    map<int,Region>::iterator rit;

    for(rit=regionList.begin();rit!=regionList.end();rit++){
      //cout<<"region["<<rit->first<<"]: size="<<rit->second.l.size()<<endl;
      
      rit->second.rl.push_back(rit->first);
      rit->second.count=orgRegions[rit->first].size();

      if(outputTree)
	rit->second.currentLabel=
	  hierarchy.addStartingRegion(orgRegions[rit->first]);

    }


    if (Verbose()>1)
      cout << "Found " << regionList.size() << " separate regions." << endl;
    	    
    delete[] lbl;
    delete[] isNeighbour;

    // calculate feature vectors for all regions

    for(rit=regionList.begin();rit!=regionList.end();rit++)
      updateFeatureValues(rit->first);

    if (Verbose()>1) {
      cout << "Feature values calculated." << endl;
      
      int dim=regionList.begin()->second.fV.size();
      // dim should be got from feature spec.
      vector<float> sum(dim,0.0);
      vector<float> sqrsum(dim,0.0);

      for(rit=regionList.begin();rit!=regionList.end();rit++)
	for (int d=0; d<dim; d++){
	  sum[d]    += rit->second.fV[d];
	  sqrsum[d] += rit->second.fV[d]*rit->second.fV[d];
	}

      for(int d=0;d<dim;d++){
	sum[d] /= regionList.size();
	sqrsum[d] /= regionList.size();
	
	cout << "Feature component "<<d<<": mean="<<sum[d]<<" stddev="<<
	  sqrt(sqrsum[d]-sum[d]*sum[d]) << endl;	
      }

	
	
    }	

    return true;

  }
    

  ///////////////////////////////////////////////////////////////////////////
  
  bool RegionMerger::joinSmallRegions()
  {
    
    // first look for small regions that have only one neighbour
    int nregions=0;
    // dumpRegionList();
    map<int,Region>::iterator rit=regionList.begin();



    bool first=true;

    while(rit!=regionList.end()&&(int)regionList.size()>numberOfRegions){
      if((int)rit->second.count<=sizeThreshold ){
	nregions++;
	if((int)rit->second.nList.size()==1){
	  
	  // some tricks to avoid iterators going invalid
	  
	  if(first){
	    mergeRegions(rit->first,*rit->second.nList.begin(),no_update);
	    rit=regionList.begin();
	    continue;
	  }
	  else{
	    const map<int,Region>::iterator ritwas=rit;
	    rit--;
	    mergeRegions(ritwas->first,
			 *ritwas->second.nList.begin(),no_update);
	  }
	}
      }
      rit++;
      first=false;
    } // while    

    if(Verbose()>1){
      cout << nregions <<" small regions." << endl;
      cout << "regions w/ one neighbour joined." << endl;
      cout << "region count now " << regionList.size() << endl;
    }

    // then merge all small regions to their closest neighbours
  
      int smallmerges=0;

      bool approx_update=!useAveraging;

      bool fromFirst=false;

      rit=regionList.begin();
      while(rit!=regionList.end()&&(int)regionList.size()>numberOfRegions){
	  while((int)rit->second.count<=sizeThreshold){

	    const int n=findClosest(rit->first);
	    bool isInvalidated= rit->first > n;

	    fromFirst= isInvalidated && (rit==regionList.begin());
	    const map<int,Region>::iterator ritwas=rit;
	    if(isInvalidated) rit--;
	    mergeRegions(ritwas->first,n,
			 approx_update?choose_larger:full_update);
	    smallmerges++;
	    if(Verbose()>1 && smallmerges%100==0)
	      cout << smallmerges <<" small regions merged." << endl;
	    if(isInvalidated) break;
	  } // while

	if(fromFirst){
	  rit=regionList.begin();
	  fromFirst=false;
	}
	else{
	  rit++;
	}

      } // while
	
      if(approx_update){
	// all merges done, update remaining feature vectors
	for(rit=regionList.begin();rit!=regionList.end();rit++)
	    updateFeatureValues(rit->first);
      }

      // cout << "small regions joined" << endl;

      return true;
    
    }

  ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::mergeQueuedRegions(int desiredCount){

    while((int)regionList.size()>desiredCount){
      if(Verbose()>1 && (regionList.size()<100 || currentTime%100==0))
 	cout <<"region count:" << regionList.size() 
	      	     << " queue size: " << queue_vector.size() << endl; 

      if(clearThreshold>0 && queue_vector.size() > 
	 (size_t)clearThreshold*regionList.size())
	fillPriorityQueue();

      mergeTopRegions();


      
    }
  }
    
  
  ///////////////////////////////////////////////////////////////////////////
  
  void RegionMerger::mergeRegions(int i1, int i2, 
				 feature_update_t updateFeatures,
				 bool showDebug)
  {


    if(i1>i2){
      int tmp=i1;
      i1=i2;
      i2=tmp;
    }

    Region *r1=&(regionList[i1]),*r2=&(regionList[i2]);

    if(showDebug){
    cout << "mergeRegions(): Merging regions " << i1 << " and " << i2 << endl;
    cout << "neighbourlists:" << endl;
    cout << "counts: "<<r1->count<<" and "<<r2->count<<endl;
    }
    


    if(showDebug){
      cout << formVirtualSegmentLabel(r1->nList) << endl;
      cout << formVirtualSegmentLabel(r2->nList) << endl;
    }

    bool r2Larger=r1->count<r2->count;

    //  cout << "Before merging sizes of coordinate lists: " <<endl;
    //     cout << "r1: " << regionList[i1].l.size() << endl;
    //     cout << "r2: " << regionList[i2].l.size() << endl;
    
    set<int>::iterator it;

    for(it=r2->nList.begin(); it != r2->nList.end(); it++){
      if(*it == i1) continue;
      
      r1->nList.insert(*it);
      Region *r=&(regionList[*it]);

      if(edgeWeight != 0){
	// update borderInfo of neighbours
	
	r->bI[i1].add(r->bI[i2]);
	r->bI.erase(i2);

	r1->bI[*it].add(r2->bI[*it]);
      }

      r->nList.erase(i2);
      r->nList.insert(i1);
    }
    
    r1->nList.erase(i2);

    if(updateFeatures==full_update && useAveraging){
      int i;
      for(i=0;i<(int)r1->fV.size();i++){
	r1->fV[i] = (r1->count*r1->fV[i]+r2->count*r2->fV[i])/
	  (r1->count+r2->count);
      }
    }


    int i;
    for(i=0;i<(int)r2->rl.size();i++)
      r1->rl.push_back(r2->rl[i]);

    r1->count += r2->count;
    
       
    if(outputTree){
      r1->currentLabel=hierarchy.addRegionMerge(r1->currentLabel,
						r2->currentLabel);
    }

    

    if(updateFeatures==full_update){
      if(useAveraging==false)
	updateFeatureValues(i1); // inefficient
    }
    else if(updateFeatures==choose_larger){
      if(r2Larger)
	r1->fV=r2->fV;
    }
  //   cout << "After merging sizes of coordinate lists: " <<endl;
//     cout << "r1: " << regionList[i1].count << endl;
//     cout << "r2: " << regionList[i2].count << endl;


    regionList.erase(i2);

    if(showDebug){
      cout << "after merge nlist:" << endl;
      cout << formVirtualSegmentLabel(r1->nList) << endl;
    }

  }

  ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::fillPriorityQueue(){
    currentTime=0;

    queue_vector.clear();

    Region *r;

    map<int,Region>::iterator rit;
    for(rit=regionList.begin();rit!=regionList.end();rit++){

      r=&(rit->second);
	r->timestamp=0;
	// updateFeatureValues(i);

// 	set<int>::iterator it;
// 	for(it=r->nList.begin(); it != r->nList.end(); it++)
// 	  if(*it > i){
// 	    regionDiff d;
// 	    d.timestamp=0;
// 	    d.r1=i;
// 	    d.r2=*it;
// 	    d.dist=-regionDist(i,*it);
// 	    queue.push(d);
// 	  }

	r->closestNeighbour=findClosest(rit->first);

	regionDiff d;
	d.timestamp=0;
	d.r1=rit->first;
	d.r2=r->closestNeighbour;
	d.dist=-regionDist(rit->first,r->closestNeighbour);
	//queue.push(d);

	//	cout << "Initial merge: " << d.print() << endl;

	queue_vector.push_back(d);

    }
    make_heap(queue_vector.begin(),queue_vector.end());
  }
  ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::mergeTopRegions(){
    
    //regionDiff d=queue.top();
    //queue.pop();

    pop_heap(queue_vector.begin(),queue_vector.end());
    regionDiff d=queue_vector.back();
    queue_vector.pop_back();

    if(regionList.count(d.r1)==0 ||regionList.count(d.r2)==0){
      // cout << "popped inexistent merge" << endl;
      return;
    }

    if(d.r1>d.r2){
      int tmp=d.r1;
      d.r1=d.r2;
      d.r2=tmp;
    }
    
    Region *r1=&(regionList[d.r1]);
    Region *r2=&(regionList[d.r2]);

    if(d.timestamp < r1->timestamp || d.timestamp < r2->timestamp) return;

    currentTime++;

    int i=d.r1,i2=d.r2;

    //     cout << "Merging regions " << d.r1 <<" and " << d.r2 
    //       << " w\\ d=" << d.dist << endl;

//     cout << "Feature vectors:" << endl << " r1: ";
//     Feature::printFeatureVector(cout,r1->fV);
//     cout << endl << " r2: ";
//     Feature::printFeatureVector(cout,r2->fV);


//     cout << "Neighbours of r1: " << endl;
    set<int>::iterator it;

  //   for(it=r1->nList.begin(); it != r1->nList.end(); it++){
//       if(regionList[*it].count){
// 	cout << "     #"<<*it<<" (size "<<regionList[*it].count<<"):";
//       Feature::printFeatureVector(cout,regionList[*it].fV);
//       cout << endl;
//       }
//     }

//     cout << "Neighbours of r2: " << endl;
//     for(it=r2->nList.begin(); it != r2->nList.end(); it++){
//       if(regionList[*it].count){
// 	cout << "     #"<<*it<<" (size "<<regionList[*it].count<<"):";
// 	Feature::printFeatureVector(cout,regionList[*it].fV);
// 	cout << endl;
//       }
//     }

    mergeRegions(d.r1,d.r2,full_update);

    // now pointer r2 becomes invalid

    for(it=r1->nList.begin(); it != r1->nList.end(); it++){
      Region *r=&(regionList[*it]);
      bool otherBefore = (r->closestNeighbour != i && r->closestNeighbour != i2); 
      r->closestNeighbour=findClosest(*it);
      if(r->closestNeighbour != i && otherBefore) continue; 
      // other relevant merges remain in the queue

      regionDiff d;
      d.timestamp=currentTime;
      d.r1=*it;
      d.r2=r->closestNeighbour;
      d.dist = -regionDist(d.r1,d.r2);
      //queue.push(d);
      //    cout << "pushed merge: " << d.print() << endl;
      queue_vector.push_back(d);
      push_heap(queue_vector.begin(),queue_vector.end());

    }
    if(r1->nList.size()){

      r1->closestNeighbour=findClosest(i);
    
      regionDiff dn;
      dn.timestamp=currentTime;
      dn.r1=i;
      dn.r2=r1->closestNeighbour;
      dn.dist = -regionDist(dn.r1,dn.r2);
      //queue.push(d);
      queue_vector.push_back(dn);
      //cout << "pushed merge: " << dn.print() << endl;
      push_heap(queue_vector.begin(),queue_vector.end());
      
      r1->timestamp=currentTime;
      //r2->timestamp=currentTime;
    }
    // dumpRegionList();
    
    
  }

  ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::initialApproximateMerge(){

    // const int batchSize=40;
    const int initialStop=400;


    int batchnr=0;

    while((int)regionList.size() > initialStop){

//         cout << "beginning batch of initialApproximateMerge" << endl;
//         cout <<"regionCount="<<regionList.size() << endl;
      
      set<int> region;
      set<int> neighbours;
      set<regionDiff> joins;
      
      int batchSize=(regionList.size()-numberOfRegions)/2;
      //int batchSize=500;

      const int l=regionList.size();
	vector<int> labelsInRL;
	map<int,Region>::iterator rit;

	for(rit=regionList.begin();rit!=regionList.end();rit++)
	  labelsInRL.push_back(rit->first);

 // 	cout << labelsInRL.size() << " labels collected:( l=" << l<<")"<<endl;

//  	for(size_t i=0;i<labelsInRL.size();i++)
//  	  cout<<labelsInRL[i]<<endl;
	  

	
      for(int i=0;i<batchSize;i++){
	int j;

	int r=random(l-1);

	//cout << "r="<<r<<endl;

	j=labelsInRL[r];
	if(region.count(j)||neighbours.count(j)) continue;

	//cout << "region set: "<<formVirtualSegmentLabel(region)<<endl;
	//cout << "neighbours: "<<formVirtualSegmentLabel(neighbours)<<endl;

	region.insert(j);

	//char jstr[80];
	//sprintf(jstr,"%d",j);

	//if(regionList.count(j)==0) 
	//  throw string("accepted region ")+jstr+ " not in regionlist";

	const set<int> &nbr=regionList[j].nList;

	//cout << "accepted region "<<j<<" w/ neighbours " << endl
	//     <<"  "<< formVirtualSegmentLabel(nbr)<<endl;

	neighbours.insert(nbr.begin(),nbr.end());
	regionDiff tmp;
	tmp.r1=j;
	tmp.r2=findClosest(j);

	//cout << "closest region: "<<tmp.r2<<endl;
	tmp.dist=regionList[j].count*regionDist(tmp.r1,tmp.r2);
	tmp.dist=regionDist(tmp.r1,tmp.r2);
	joins.insert(tmp);
      }
      
      // cout <<"joins.size()="<<joins.size()<<endl;

      // sort(joins.begin(),joins.end());
      int i=0;
      map<int,int> joinedTo;
      int njoins=joins.size()/2; // arbitrary choice, quite aggressive
      if(njoins<1) njoins=1;

      for(set<regionDiff>::iterator it=joins.begin();i<njoins;
	  i++,it++){
	regionDiff d=*it;
	//cout << "i= "<<i<<" dist="<<it->dist<<endl;
	while(joinedTo.count(d.r1)) d.r1=joinedTo[d.r1];
	while(joinedTo.count(d.r2)) d.r2=joinedTo[d.r2];

	if(d.r1==d.r2) continue;

	mergeRegions(d.r1,d.r2,full_update);
	//	if(regionList.size() != countRegions()){
	//  cout << "batch nr " << batchnr << endl;
	//  cout << "joined regions "<<d.r1<<" and "<<d.r2<<endl;
	//  cout << "regionList.size()="<<regionList.size()<<" (real="<<countRegions()
	//     << ") -> batchSize="<<batchSize<<endl;
	//}

	if(d.r1<d.r2) joinedTo[d.r2]=d.r1;
	else joinedTo[d.r1]=d.r2;
      }  
      batchnr++;
    }

  }

  //////////////////////////////////////////////////////////////////////////
  int random(int m){
  int r;
  
  do{
    r=rand();
  } while(r==RAND_MAX);

  float f=((float)r)/((float)RAND_MAX);

  //  cout << "f=" << f<<endl;
  //cout << "m=" << m << endl;

  return (int)(f*(m+1));
}

  ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::dumpRegionList(bool labelsonly){

    cout << "Dumping region list:" << endl;
    map<int,Region>::iterator rit;
    for(rit=regionList.begin();rit!=regionList.end();rit++){
      if(labelsonly)
	cout << rit->first<<endl;
      else{
	Region *r = &(rit->second);
	cout << "regionList[" <<rit->first<<"]: count: " 
	     << r->count << endl;
      
	cout << endl << "nList: ";
	set<int>::iterator it;
	for(it=r->nList.begin();it != r->nList.end();it++)
	  cout << " " << *it;
	
	cout << endl;
	cout <<"fV: ";
	for(size_t j=0;j<r->fV.size();j++)
	  cout <<" "<<r->fV[j];
	cout << endl;
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::updateFeatureValues(int i)
  {
    if(regionList.count(i)==0) return;

    Region *r = &(regionList[i]);
    if(r->count==0) return;


      

    vector<coordList *> v;
    for(size_t j=0;j<r->rl.size();j++)
      v.push_back(&(orgRegions[r->rl[j]]));

    featureInterface.calculateFeatures(v,r->fV,pixelsToUse);
  }

  ///////////////////////////////////////////////////////////////////////////
  int RegionMerger::findClosest(int i){

    if(regionList.count(i)==0) 
      throw string("RegionMerger::findClosests(): index not in regionlist.");

    Region *r1=&(regionList[i]);

    //cout << "findClosest: region " <<i<<" has "<<r1->nList.size()
    //	 <<" neighbours." << endl; 

    int minind=-1;
    float mindist=9999999999999999.0;
    
    set<int>::iterator it;
    for(it=r1->nList.begin(); it != r1->nList.end(); it++){
      //Region *r = &(regionList[*it]);
      float dist=regionDist(i,*it);
	//Feature::VectorSqrDistance(r->fV,r1->fV);
      if(dist < mindist){
	mindist=dist;
	minind=*it;
      }
    }
    
    return minind;

  }
  ///////////////////////////////////////////////////////////////////////////

 float RegionMerger::sizeWeighting(float s1,float s2){

   float s;

   switch(weightDistances){
   case 1:
     s = (s1<s2)?s1:s2;
     s /= img->getHeight();
     s /= img->getWidth();
     if(s>saturationLimit) s=saturationLimit;
     return weightBias + sqrt(s);      

   case 2:
     s=s1+s2;
     s /= img->getHeight();
     s /= img->getWidth();
     if(s>saturationLimit) s=saturationLimit;
     return weightBias + sqrt(s);      

   case 3:
     s = (s1<s2)?s1:s2;
     s /= img->getHeight();
     s /= img->getWidth();
     if(s>saturationLimit) s=saturationLimit;
     return weightBias + log(s);      

   case 4:
     s = (s1<s2)?s1:s2;
     s /= img->getHeight();
     s /= img->getWidth();
     if(s>saturationLimit) s=saturationLimit;
     return weightBias + s;      

   default:
     throw string("RegionMerger::sizeWeighting(): unknown weighting method.");

   }
 }
 ///////////////////////////////////////////////////////////////////////////

  float RegionMerger::regionDist(int i1,int i2){

  char i1str[80],i2str[80];

     sprintf(i1str,"%d",i1);
     sprintf(i2str,"%d",i2);

     if(regionList.count(i1)==0)
       throw string("RegionMerger::regionDist(")+i1str+","+i2str+
 	"): inexistent region "+i1str;

     if(regionList.count(i2)==0)
       throw string("RegionMerger::regionDist(")+i1str+","+i2str+
 	"): inexistent region "+i2str;

    Region *r1 = &(regionList[i1]);
    Region *r2 = &(regionList[i2]);

    float ret=0.0;

    ret=sqrt(Feature::VectorSqrDistance(r1->fV,r2->fV));
    if(edgeWeight != 0){
      if(Verbose()>2)
	cout << "feature distance: " << ret << endl; 
      ret += edgeWeight * borderStrength(i1,i2);
      if(Verbose()>2)
	cout << " w/ edge contribution: " << ret << endl; 
    }

    ret *= sizeWeighting(r1->count,r2->count);

    return ret;

  }

 ///////////////////////////////////////////////////////////////////////////

  float RegionMerger::borderStrength(int i1,int i2){
    // returns the total border strength (in edgeimg) on
    // the border between regions i1 and i2
    // normalises with of (sublinear) function of # of border pixels

    Region *r1 = &(regionList[i1]);
    Region *r2 = &(regionList[i2]);

    if(!r1->bI.count(i2)) return 0;

    float s=r1->bI[i2].strength+r2->bI[i1].strength;
    
    return s / 
      borderLengthNorm(r1->bI[i2].count);
  }

 ///////////////////////////////////////////////////////////////////////////

  float RegionMerger::borderStrength(const coordSet &s1,const coordSet &s2){

    set<coord>::iterator it;
    float s=0.0;

    for(it=s1.begin();it!=s1.end();it++)
      s += edgeimg.get_one_float(it->x,it->y);

    for(it=s2.begin();it!=s2.end();it++)
      s += edgeimg.get_one_float(it->x,it->y);

    return s;
    
  }

 ///////////////////////////////////////////////////////////////////////////

  float RegionMerger::borderLengthNorm(int n){
    const float b=9;

    return n+b;
  }
 ///////////////////////////////////////////////////////////////////////////
  void RegionMerger::detectEdges(){

    // multi-scale edge detection, sum the responses

    int s;

    vector<float> h(3), v(3, 1.0);
    h[0] = -(h[2]=1);
    h[1]=0;
    edgeimg = img->accessFrame()->absolute_gradient(h, v);
    edgeimg.force_one_channel();

    vector<float> edata=edgeimg.get_float(); 

    for(s=1;s<=16;s*=2){
      
      if(Verbose()>1)
	cout << "processing scale " << s << endl;

      vector<float> blurflt(2*s+1,1.0/(2*s+1)); 

      imagedata blurimg=img->accessFrame()->convolve(blurflt,blurflt,1.0,true);
      
      char fn[80];
      sprintf(fn,"blur-%d.tiff",s);
      imagefile::write(blurimg,fn);

      // get gradient magnitude
      vector<float> h(3), v(3, 1.0);
      h[0] = -(h[2]=1);
      imagedata gradimg = blurimg.absolute_gradient(h, v);
      gradimg.force_one_channel();
      
      sprintf(fn,"grad-%d.tiff",s);
      imagefile::write(gradimg,fn);

      vector<float> gdata=gradimg.get_float();

      if (edata.size() != gdata.size())
	throw string("RegionMerger::detectEdges(): size mismatch");      

      for (size_t i=0; i<gdata.size(); i++)
	edata[i] += gdata[i];
    }

    if(Verbose()>1) 
      cout << "multi-scale finished" << endl;

    edgeimg.set(edata);

    // filter edgeimg with box filter
    const int boxD=5;
      
    vector<float> boxMask(boxD,1.0);
    edgeimg=edgeimg.convolve(boxMask,boxMask);
    
    // normalise the edge image to max=1
    
    vector<float> imgd=edgeimg.get_float();
    edgeimg=edgeimg.multiply(1.0/ *max_element(imgd.begin(),imgd.end()));	
    

    //cout << "writing edge image to file edgeimg.tiff" << endl;
    //imagefile::write(edgeimg,"edgeimg.tiff");
  }
  
  ///////////////////////////////////////////////////////////////////////////


 void RegionMerger::checkQueue(){

   set<int> valid; // these regions have currently valid merge in the queue

   for(vector<regionDiff>::iterator it=queue_vector.begin();
       it != queue_vector.end(); it++){
     regionDiff d=*it;

     Region *r1=&(regionList[d.r1]);
     Region *r2=&(regionList[d.r2]);

     if(d.timestamp < r1->timestamp || d.timestamp < r2->timestamp) continue;

     valid.insert(d.r1);
     valid.insert(d.r2);
   }

   for(size_t i=0; i<regionList.size();i++){
       if(valid.count(i)==0)
	 cout << "No valid merge for region "<< i << endl;
   }

 }

///////////////////////////////////////////////////////////////////////////

 int RegionMerger::countRegions(){

   int realcount=0;

   for(size_t i=0;i<regionList.size(); i++)
       realcount++;

   return realcount;

 }
  ///////////////////////////////////////////////////////////////////////////

  void RegionMerger::mapListToBitmap(){
    int label=0;
    map<int,Region>::iterator rit;
    for(rit=regionList.begin();	rit!=regionList.end();rit++){
	Region *r = &(rit->second);
	int j;
	for(j=0;(unsigned)j<r->rl.size();j++)
	  img->translateLabels(label,orgRegions[r->rl[j]]);
	label++;
    }
  }

  ///////////////////////////////////////////////////////////////////////////

} // namespace picsom

 
 ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
