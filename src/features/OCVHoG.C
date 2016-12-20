// $Id: OCVHoG.C,v 1.2 2013/09/24 13:10:36 vvi Exp $	

#include <OCVHoG.h>

namespace picsom {
  static const char *vcid =
    "$Id: OCVHoG.C,v 1.2 2013/09/24 13:10:36 vvi Exp $";

static OCVHoG list_entry(true);

//=============================================================================

string OCVHoG::Version() const {
  return vcid;
}
//=============================================================================

bool OCVHoG::ProcessOptionsAndRemove(list<string>&l){

 for (list<string>::iterator i = l.begin(); i!=l.end();) {
   
    string key, value;
    SplitOptionString(*i, key, value);
    
    if (key=="numberofblocks") {
      //      zeropad=IsAffirmative(value);
      int v;
      if(sscanf(value.c_str(),"%d",&v)==1){
	 blocksH=blocksV=v;
	 size_t p=value.find(',');
	 if(p!=string::npos&&sscanf(value.c_str()+p+1,"%d",&v)==1)
	   blocksV=v;
      }
      i = l.erase(i);
      continue;
    }
    if (key=="pyramidlevels") {
      vector<string> levels = SplitInCommasObeyParentheses(value);

      int val;
      
      pyramidlevels.clear();

      for(size_t n=0;n<levels.size();n++)
	if(sscanf(levels[n].c_str(),"%d",&val)==1)
	  pyramidlevels.insert(val);

      i = l.erase(i);
      continue;
    }

    i++;
  } 

 return Feature::ProcessOptionsAndRemove(l);

}


//=============================================================================

int OCVHoG::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  str << "numberofblocks=X,Y (default=2,X)"
      << endl;
  str << "pyramidlevels=X[,Y]* (default=0)"
      << endl;

 return ret+2;
}

//=============================================================================
Feature *OCVHoG::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{

  blocksH=blocksV=2;
  pyramidlevels.clear();
  pyramidlevels.insert(0);

  return Feature::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================

  Feature::featureVector OCVHoG::CalculateMask(char *mask){

    // the mask specifies, which pixels to include in calculation  
    // nonzero -> include

    //cout << "calculating mask" << endl;

    int subdivcount=0;

    set<int>::iterator it;
    for(it=pyramidlevels.begin();it!=pyramidlevels.end();it++)
      subdivcount += pow(2,*it * 2);

    featureVector ret(subdivcount*(2*blocksH-1)*(2*blocksV-1)*9*4,0.0);

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    

    int w = Width(true), h = Height(true);

    // find the bounding box of the mask to claculate the dividing grid

 
    PredefinedTypes::Box maskbb;

    int ind=0;
    bool first=true;

    for(int y=0;y<h;y++)
      for(int x=0;x<w;x++)
	if(mask[ind++]){
	  if(first){
	    maskbb.ulx=maskbb.brx=x;
	    maskbb.uly=maskbb.bry=y;
	    first=false;
	  }else{
	    if(x<maskbb.ulx) maskbb.ulx=x;
	    if(x>maskbb.brx) maskbb.brx=x;
	    if(y<maskbb.uly) maskbb.uly=y;
	    if(y>maskbb.bry) maskbb.bry=y;
	  }
	}

    float maskbbw=maskbb.brx-maskbb.ulx+1,maskbbh=maskbb.bry-maskbb.uly+1;

    ret.clear();

    for(it=pyramidlevels.begin();it!=pyramidlevels.end();it++){

      int div=pow(2,*it);

      int xincr=maskbbw/div;
      int yincr=maskbbh/div;

      for(int yind=0;yind<div;yind++)
	for(int xind=0;xind<div;xind++){

	  PredefinedTypes::Box bb;
	  bb.ulx=maskbb.ulx+xind*xincr;
	  bb.uly=maskbb.uly+yind*yincr;

	  float bbw=xincr,bbh=yincr;

	  bb.brx=bb.ulx+bbw-1;
	  bb.bry=bb.uly+bbh-1;

	  int cellsize=ceil(0.5*fmax(ceil(bbw/blocksH),ceil(bbh/blocksV)));

	  if(cellsize<2){
	    for(int i=0;i<(2*blocksH-1)*(2*blocksV-1)*9*4;i++)
	      ret.push_back(0.0);
	  }
	  else{

	    //    cout << "cell size: " << cellsize << endl;

// 	    cout << "number of blocks: " << blocksH << "x" << blocksV << endl;
	    
// 	    cout << "pyramid levels:";

//     set<int>::iterator it;
//     for(it=pyramidlevels.begin();it!=pyramidlevels.end();it++)
//       cout << " " << *it;
//     cout << endl;


	    hog.cellSize=cv::Size(cellsize,cellsize);
	    hog.blockSize=cv::Size(2*cellsize,2*cellsize);
	    hog.blockStride=hog.cellSize;
	    hog.winSize=cv::Size(2*cellsize*blocksH,2*cellsize*blocksV);

	    cv::Mat m(hog.winSize,CV_8UC1,cv::Scalar::all(0));

	    vector<float> v; 

	    int padX=0.5*(hog.winSize.width-bbw);
	    int padY=0.5*(hog.winSize.height-bbh);

	    //     cout << "padX="<<padX<<endl;
	    //     cout << "padY="<<padY<<endl;

	    for(int x=bb.ulx;x<=bb.brx;x++)
	      for(int y=bb.uly;y<=bb.bry;y++)
		{
		  //	   cout <<"(x,y)=("<<x<<","<<y<<")"<<endl;
		  if(mask[y*w+x]){
		    SegmentData()->get_pixel_rgb(x, y,v);
		    float h=0.333333*(v[0]+v[1]+v[2]);
		    //	  cout << "h="<<h<<endl;
		    m.at<uchar>(y-bb.uly+padY,x-bb.ulx+padX)=255*h;

		  }
		}
// 	    cv::imshow("padded image",m);
// 	    cv::waitKey(0);
	    vector<float> des;
	    hog.compute (m,des);
// 	    cout << "got " << des.size() << " descriptors" << endl;

	    for(size_t i=0;i<des.size();i++)
	      ret.push_back(des[i]);
	  }
	}
    }


#endif // HAVE_OPENCV2_CORE_CORE_HPP

    return ret;

  }

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

