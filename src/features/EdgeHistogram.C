// $Id: EdgeHistogram.C,v 1.4 2008/09/29 10:07:38 jorma Exp $	

#include <EdgeHistogram.h>

namespace picsom {
  static const char *vcid =
    "$Id: EdgeHistogram.C,v 1.4 2008/09/29 10:07:38 jorma Exp $";

static EdgeHistogram list_entry(true);

//=============================================================================

string EdgeHistogram::Version() const {
  return vcid;
}
//=============================================================================

bool EdgeHistogram::ProcessOptionsAndRemove(list<string>&l){

 for (list<string>::iterator i = l.begin(); i!=l.end();) {
   
    string key, value;
    SplitOptionString(*i, key, value);
    
    if (key=="padwithzeros") {
      zeropad=IsAffirmative(value);
      i = l.erase(i);
      continue;
    }
    i++;
  } 

 return Feature::ProcessOptionsAndRemove(l);

}


//=============================================================================

int EdgeHistogram::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  str << "padwithzeros=true|false (default=true)"
      << endl;
 return ret+1;
}

//=============================================================================
Feature *EdgeHistogram::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  zeropad=true;
  return Feature::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================

  Feature::featureVector EdgeHistogram::CalculateMask(char *mask){

    // the mask specifies, which pixels to include in calculation  
    // nonzero -> include

    //cout << "calculating mask" << endl;


    int w = Width(true), h = Height(true);

    // find the bounding box of the mask to claculate the dividing grid

 
    PredefinedTypes::Box bb;

    int ind=0;
    bool first=true;

    for(int y=0;y<h;y++)
      for(int x=0;x<w;x++)
	if(mask[ind++]){
	  if(first){
	    bb.ulx=bb.brx=x;
	    bb.uly=bb.bry=y;
	    first=false;
	  }else{
	    if(x<bb.ulx) bb.ulx=x;
	    if(x>bb.brx) bb.brx=x;
	    if(y<bb.uly) bb.uly=y;
	    if(y>bb.bry) bb.bry=y;
	  }
	}

    //string bbstr; bb.write(bbstr);
    //cout << "bounding box" << bbstr << endl;

    if(first || bb.area()<64) // region too small
      return featureVector(80, 0.0);

    featureVector ret(80,0.0);

    const float desiredNumberOfBlocks=1100;
    float blockdim=sqrt((bb.width()*bb.height())/desiredNumberOfBlocks);

    float subw=bb.width()/4.0,subh=bb.height()/4.0;

    // process each subimage 
    for(int i=0;i<4;i++) for(int j=0;j<4;j++){
      

      float ulx=bb.ulx+j*subw;
      //float brx=bb.ulx+(j+1)*subw; // open interval

      float uly=bb.uly+i*subh;
      //float bry=bb.uly+(i+1)*subh; // open interval

      //cout << "processing subimage ("<<i<<","<<j<<") w/ ulx="<<ulx
      //	   << " uly="<<uly<<endl;

      int xcount=(int)(subw/blockdim),ycount=(int)(subh/blockdim);
      float blockw=subw/xcount,blockh=subh/ycount;

      // for each block

      float sum[2][2],count[2][2];
      vector<float> localHist(5,0.0);

      for(int bi=0;bi<xcount;bi++) for(int bj=0;bj<ycount;bj++){

	//cout << "processing block ("<<bi<<","<<bj<<")"<<endl;

	// for each of the four macropixels

	for(int pi=0;pi<2;pi++) for(int pj=0;pj<2;pj++){

	  //	  cout << "processing macropixel ("<<bi<<","<<bj<<"): ";

	  sum[pi][pj]=0; count[pi][pj]=0;
	  float x1=ulx+(bi+0.5*pi)*blockw;
	  float x2=x1+0.5*blockw;

	  float y1=uly+(bj+0.5*pj)*blockh;
	  float y2=y1+0.5*blockh;

	  int xi1=(int)ceil(x1),xi2=(int)floor(x2)-1; // closed interval
	  int yi1=(int)ceil(y1),yi2=(int)floor(y2)-1; // closed interval

	  //	  cout << "("<<x1<<","<<y1<<")-("<<x2<<","<<y2<<") -> ("
	  //     <<xi1<<","<<yi1<<")-("<<xi2<<","<<yi2<<")"<<endl;

	  // determine if there are partial pixels 
	  // on left, right, top or bottom

	  const float epsilon=0.01;

	  bool partialLeft=(fabs(x1-xi1)>epsilon);
	  bool partialRight=(fabs(x2-1-xi2)>epsilon);
	  bool partialTop=(fabs(y1-yi1)>epsilon);
	  bool partialBottom=(fabs(y2-1- yi2)>epsilon);

	  //cout << "partialleft: " << partialLeft
	  //     << " partialRight: " << partialRight << "(delta="<<x2-1-xi2<<")"
	  //     << " partialTop: " << partialTop 
	  //     << " partialBottom: " << partialBottom << endl;
	    

	  // inner region

	  for(int x=xi1;x<=xi2;x++)
	    for(int y=yi1;y<=yi2;y++)
	      accumulatePixel(x,y,sum[pi][pj],count[pi][pj],mask[y*w+x],1);
	  
	  // left and right partial borders (no corners)

	  // cout << "inner region accumulated" << endl;
	  
	  if(partialLeft){
	    float weight=xi1-x1;
	    int x=xi1-1;
	    for(int y=yi1;y<=yi2;y++)
	      accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			      mask[y*w+x],weight);
	  }

	  if(partialRight){
	    float weight=x2-xi2;
	    int x=xi2+1;
	    for(int y=yi1;y<=yi2;y++)
	      accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			      mask[y*w+x],weight);
	  }
	  
	  if(partialTop){
	    float weight=yi1-y1;
	    int y=yi1-1;
	    for(int x=xi1;x<=xi2;x++)
	      accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			      mask[y*w+x],weight);
	  }

	  if(partialBottom){
	    float weight=y2-1-yi2;
	    int y=yi2+1;
	    for(int x=xi1;x<=xi2;x++)
	      accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			      mask[y*w+x],weight);
	  }

	  // cout << "partial sides accumulated" << endl;

	  // partial corners


	  if(partialLeft && partialTop){
	    int x=xi1-1,y=yi1-1;
	    float weight=(yi1-y1)*(xi1-x1);
	    accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			    mask[y*w+x],weight);
	  }

	  if(partialLeft && partialBottom){
	    int x=xi1-1,y=yi2+1;
	    float weight=(y2-yi2)*(xi1-x1);
	    accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			    mask[y*w+x],weight);
	  }

	  if(partialRight && partialTop){
	    int x=xi2+1,y=yi1-1;
	    float weight=(yi1-y1)*(x2-xi2);
	    accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			    mask[y*w+x],weight);
	  }

	  if(partialRight && partialBottom){
	    int x=xi2+1,y=yi2+1;
	    float weight=(y2-yi2)*(x2-xi2);
	    accumulatePixel(x,y,sum[pi][pj],count[pi][pj],
			    mask[y*w+x],weight);
	  }

	} // for macropixels

	// sums and counts now collected, evaluate filter responses
	
	if(count[0][0]&&count[0][1]&&count[1][0]&&count[1][1]){
	  float pixval[2][2];
	  for(int pi=0;pi<2;pi++) for(int pj=0;pj<2;pj++)
	    pixval[pi][pj]=sum[pi][pj]/count[pi][pj];

	  int maxtype=0;
	  float maxval;

	  const float sqrt2=sqrt(2.0);

	  float vert = abs(pixval[0][0]+pixval[0][1]-
			   pixval[1][0]-pixval[1][1]);
	  maxval=vert;

	  float hor = abs(pixval[0][0]-pixval[0][1]+
			   pixval[1][0]-pixval[1][1]);
	  if(hor>maxval) {maxtype=1; maxval=hor;}

	  float dia45 = sqrt2*abs(pixval[0][0]-pixval[1][1]);
	  if(dia45>maxval) {maxtype=2; maxval=dia45;}

	  float dia135 = sqrt2*abs(pixval[1][0]-pixval[0][1]);
	  if(dia135>maxval) {maxtype=3; maxval=dia135;}

	  float nondir = 2*abs(pixval[0][0]-pixval[0][1]-
			   pixval[1][0]+pixval[1][1]);
	  if(nondir>maxval) {maxtype=4; maxval=nondir;}

	  
	  const float edgeThreshold=0.1;

	  if(maxval>edgeThreshold){
	    localHist[maxtype]++;
	  }
	}


      } // for blocks

      for(int k=0;k<5;k++)
	ret[5*(i*4+j)+k]= localHist[k] / (xcount*ycount);
    }

    //cout << "mask calculated" << endl;

    return ret;

  }
//=============================================================================
  void EdgeHistogram::accumulatePixel(int x,int y,float &sum, float &count,
				      char mask,float weight){
    vector<float> v; 
    float h;

    if(!mask){
      if(!zeropad) return;
      h=0.0;
    } else{
      SegmentData()->get_pixel_rgb(x, y,v);
      h=0.333333*(v[0]+v[1]+v[2]);
    }
    
    sum += weight*h;
    count += weight;
  }

//=============================================================================
} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

