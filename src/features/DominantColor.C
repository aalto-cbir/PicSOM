// $Id: DominantColor.C,v 1.13 2010/08/17 11:26:28 vvi Exp $	

#include <DominantColor.h>

#include <limits>

namespace picsom {
  static const char *vcid =
    "$Id: DominantColor.C,v 1.13 2010/08/17 11:26:28 vvi Exp $";

static DominantColor list_entry(true);

//=============================================================================

string DominantColor::Version() const {
  return vcid;
}
//=============================================================================

bool DominantColor::ProcessOptionsAndRemove(list<string>&l){
  return Feature::ProcessOptionsAndRemove(l);
}

//=============================================================================

int DominantColor::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  return ret;
}

//=============================================================================
Feature *DominantColor::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  return Feature::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================

  Feature::featureVector DominantColor::CalculateMask(char *mask){

    // the mask specifies, which pixels to include in calculation  
    // nonzero -> include
    
    preprocessResult_Cieluv *rptr= (preprocessResult_Cieluv*)
      SegmentData()->accessPreprocessResult("cieluv");

// NB According to MPEG-7 standard, ought to use CIE Lab space
  // This erraneous behaviour was decided to be kept unchanged 
  // to avoid a change in feature vector semantics



    int w = Width(true), h = Height(true);
 
    int ind=0;

    // collect pixel values to an array

    featureVector vf(3);

    vector<featureVector > arr;

    for(int y=0;y<h;y++)
      for(int x=0;x<w;x++){
	if(mask[ind]){
	  vf[0]=rptr->l[ind];
	  vf[1]=rptr->u[ind];
	  vf[2]=rptr->v[ind];
	  arr.push_back(vf);
	}
	ind++;
      }

    if(arr.size()==0) // not a single pixel in the mask
      return Feature::featureVector(6,0.0);

    const int initBins=1;
    const int finalBins=8;


    // cout << "about to start GLA" << endl;

    const int samplecount=3000;

    vector<featureVector > bins=GLAApproximate(arr,initBins,finalBins,samplecount);

    // cout << "GLA ready , got clusters" << endl;

    //for(size_t i=0;i<bins.size();i++){
    //  cout <<" ("; printFeatureVector(cout,bins[i]); cout<<")"<<endl;
    //}

    // agglomerative merging

    vector<int> labels(arr.size());

    update_labels(arr,labels,bins,bins.size());
    int N=arr.size();
    vector<int> counts(finalBins,0);

    for(int i=0;i<N;i++){
      counts[labels[i]]++;
    }

    // cout << "counts collected" << endl;

    float *dist = new float[finalBins*finalBins];
    
    float mindist=numeric_limits<float>::max();
    pair<int,int> minpair(1,0);

    // initialize distance table
    
    for(int i=0;i<finalBins;i++){
      for(int j=0;j<i;j++){
	int idx=i+j*finalBins;
	float d=VectorSqrDistance(bins[i],bins[j]);
	dist[idx]=d;
	if (d<mindist){
	  mindist=d;
	  minpair.first=i;
	  minpair.second=j;
	}
      }
    }
    
    // cout << "distance table initialised" << endl;

    set<int> invalid;

    const float mergeThreshold=255.0;

    while(mindist>mergeThreshold && finalBins-invalid.size()>2){
      // minpair.first is merged to minpair.second

      const int i1=minpair.first;
      const int i2=minpair.second;

      //cout << "merging cluster "<<i1<<" to cluster " <<i2<<endl;  

      float c1=counts[i1],c2=counts[i2];

      MultiplyFeatureVector(bins[i1],c1/(c1+c2));
      MultiplyFeatureVector(bins[i2],c2/(c1+c2));

      AddToFeatureVector(bins[i2],bins[i1]);

      counts[i2]+=(int)c1;
      counts[i1]=0;

      for(int i=0;i<N;i++)
	if(labels[i]==i1) labels[i]=i2;

      invalid.insert(i1);
      
      // update distance table

      for(int j=0;j<finalBins;j++){
	if(j==i2) continue;
	if(invalid.count(j)) continue;
	int idx=(j<i2)?i2+j*finalBins:j+i2*finalBins;
	float d=VectorSqrDistance(bins[i2],bins[j]);
	dist[idx]=d;
      }

      // find the minimum

      bool first=true;

      for(int i=0;i<finalBins;i++){
	if(invalid.count(i)) continue;
	for(int j=0;j<i;j++){
	  if(invalid.count(j)) continue;
	  int idx=i+j*finalBins;
	  float d=dist[idx];
	  if(first){
	    mindist=d;
	    minpair.first=i;
	    minpair.second=j;
	    first=false;
	  } else if(d<mindist){
	    mindist=d;
	    minpair.first=i;
	    minpair.second=j;
	  }
	}
      }
    }

    //bins are NOT converted back to rgb but left as cieluv
    
    //cout << "merging ready , got clusters" << endl;

    //for(size_t i=0;i<bins.size();i++){
    //  imagedata::cieluv_to_rgb(bins[i][0],bins[i][1],bins[i][2]);
    //  cout <<" ("; printFeatureVector(cout,bins[i]); cout<<")"<<endl;
    //}

    // find largest counts
    
    featureVector ret;
    
    for(int i=0;i<2;i++){
      int maxind=0;
      int maxcnt=counts[0];
      for(int b=0;b<finalBins;b++)
	if(counts[b]>maxcnt){
	  maxind=b;
	  maxcnt=counts[b];
	}
      
      ret.push_back(bins[maxind][0]);
      ret.push_back(bins[maxind][1]);
      ret.push_back(bins[maxind][2]);

      counts[maxind]=0;

    }

    delete[] dist;

    return ret;
  }

//=============================================================================

  vector<Feature::featureVector > 
  DominantColor::GLA(vector<Feature::featureVector> &data,
		     int initBins, int finalBins){

    const float distThreshold=0.05;

    int N=data.size();

    if(N==0)
      return vector<featureVector >(0);

    vector<featureVector > bins;

    vector<int> labels(N);

    for(int i=0;i<N;i++){
      labels[i]=i%initBins;
    }

    update_centers(data,labels,bins,initBins);

    bool first=true;

    for(;;){

      if(!first || initBins==1){
	if(bins.size()==(unsigned)finalBins) break;

	// cout << "GLA splitting..." << endl;

	int split=findMaxDistortion(data,labels,bins);

	// cout << "...bin # "<<split<<endl;

	// split bin 
	
	featureVector pv=determinePerturbation(data,labels,split);
	
	bins.push_back(bins[split]);
	
	AddToFeatureVector(bins[split],pv);
	MultiplyFeatureVector(pv,-1);
	AddToFeatureVector(bins.back(),pv);
      }

      first=false;

      float distortion=totalDistortion(data,labels,bins),dist_old;
      
      do{
	// cout << "In GLA inner loop. numBins="<<bins.size()<<endl;

	update_labels(data,labels,bins,bins.size());
	update_centers(data,labels,bins,bins.size());
	dist_old=distortion;
	distortion=totalDistortion(data,labels,bins);
      } while(fabs(dist_old-distortion)> distThreshold*dist_old);
    }

    return bins;

  }



//=============================================================================

  void DominantColor::update_centers(vector<Feature::featureVector> &data,
				     vector<int> &labels, 
				     vector<Feature::featureVector> &centers,
				     int number_of_means){


    int N=data.size();
    if(N==0) return;

    size_t dim=data[0].size();

    featureVector zerovec(dim, 0.0);
    centers=vector<featureVector>(number_of_means,zerovec);
    vector<int> ctr(number_of_means,0);

    for(int i=0;i<N;i++){
      int c=labels[i];
      AddToFeatureVector(centers[c],data[i]);
      ctr[c]++;
    }

    for(int i=0;i<number_of_means;i++)
      if(ctr[i])
	DivideFeatureVector(centers[i],ctr[i]);


  }

//=============================================================================
  int DominantColor::update_labels(vector<featureVector> &data,
				     vector<int> &labels, 
				     vector<featureVector> &centers,
				     int number_of_means){

    int N=data.size();
    if(N==0) return 0;

    int changed=0;

    for(int i=0;i<N;i++){
      float mindist=VectorSqrDistance(centers[0],data[i]);
      
      int minind=0;
      for(int c=1;c<number_of_means;c++){

	float d=VectorSqrDistance(centers[c],data[i],0,-1,mindist);
	
	if(d<mindist){
	  minind=c;
	  mindist=d;
	}
      }

      if(labels[i]!=minind){
	labels[i]=minind;
	changed++;
      }
    }

    return changed;

  }

//=============================================================================
  Feature::featureVector 
  DominantColor::determinePerturbation(vector<featureVector> &data,
				       vector<int> &labels,int bin){
    
    int N=data.size();


    if(N==0) return featureVector(0);
    int ctr=0;
    size_t dim = data[0].size();

    // calculate standard deviation

    featureVector sum(dim, 0), sqrsum(dim, 0);

    for(int i=0;i<N;i++)
      if(labels[i]==bin){
	ctr++;
	AddToFeatureVector(sum,data[i]);
	for(size_t d=0;d<dim;d++)
	  sqrsum[d] += data[i][d]*data[i][d];
      }
    
    if(ctr==0)
      return featureVector(dim,0.0);

    DivideFeatureVector(sum,ctr);
    DivideFeatureVector(sqrsum,ctr);
    
    for (size_t d=0; d<dim; d++) {
      sqrsum[d] -= sum[d]*sum[d];
      sqrsum[d] = 0.1*sqrt(sqrsum[d]);
    }

    return sqrsum;
    
  }
//=============================================================================
  float DominantColor::totalDistortion(vector<featureVector> &data,
				       vector<int> &labels,
				       vector<featureVector> &bins){
    int N=data.size();
    float dist=0;
    for(int i=0;i<N;i++)
      dist += VectorSqrDistance(data[i],bins[labels[i]]);

    return dist;
  }
//=============================================================================
  int DominantColor::findMaxDistortion(vector<featureVector> &data,
				       vector<int> &labels,
				       vector<featureVector> &bins){
    int N=data.size();
    int numBins=bins.size();

    featureVector dist(numBins,0.0);
    vector<int> ctr(numBins,0);

    for(int i=0;i<N;i++){
      dist[labels[i]] += VectorSqrDistance(data[i],bins[labels[i]]);
      ctr[labels[i]]++;
    }

    int maxind=0;
    float maxdist=(ctr[0])?dist[0]/ctr[0]:0;

    for(int b=1;b<numBins;b++){

      // cout << "ctr["<<b<<"]="<<ctr[b]<<endl;

      if(ctr[b]==0) continue;
      float d=dist[b]/ctr[b];
      if(d>maxdist){
	maxdist=d;
	maxind=b;
      }
    }

    return maxind;
  }


//=============================================================================

  vector<Feature::featureVector > 
  DominantColor::GLAApproximate(vector<Feature::featureVector> &data,
				int initBins, int finalBins,int samplecount){

    int orgN=data.size();
    if(orgN<=samplecount) return GLA(data,initBins,finalBins);

    // sample the data with replacement (simpler)

    vector<featureVector> sampleData(samplecount);

    unsigned int seed = 1;

    for(int i=0;i<samplecount;i++){
      float frac=rand_r(&seed);
      frac /= RAND_MAX;
      int r=(int)(frac*orgN);
      
      if(r==orgN) r=0;
      sampleData[i]=data[r];
    }

    return GLA(sampleData,initBins,finalBins);

  }

//=============================================================================
} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

