#include <ScaleSal.h>
//#include <sys/types.h>
//#include <sys/times.h>

using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;
using std::sort;

namespace picsom {
  static const string vcid="@(#)$Id: ScaleSal.C,v 1.5 2009/04/30 10:14:30 vvi Exp $";

  static ScaleSal list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  ScaleSal::ScaleSal() {
    maxscale=100;
    createDeltaLists(2,maxscale);
    globalThreshold=0.5;
    varThreshold = 5;
    numberOfMeans=8;
    skipClustering=false;
    
    clustersToKeep = -1;
    
    //iterations=1000000;
    //fftN=256;
    //fftK=8;

    quantised_image=NULL;
    saliencyAcc=clusteredSal=NULL;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  ScaleSal::~ScaleSal() {
    delete quantised_image;
    delete saliencyAcc;
    delete clusteredSal;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *ScaleSal::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void ScaleSal::UsageInfo(ostream& os) const { 
    os << "ScaleSal :" << endl
       << "  options : " << endl
       << "  -t <minimum saliency kept> (in fractions of the image max., def. 0.5)"
       << endl
       << "  -m entropy maximum detection only, no clustering" << endl
       << "  -k <# of merged neighbours in clustering> (def.8)" << endl
       << "  -v <variance threshold in clustering> (def. 5)" << endl
       << "  -e <# of most salient points to be preserved>" << endl
       << "  -E same as -e <default> (6)" << endl;

  }

  /////////////////////////////////////////////////////////////////////////////

  int ScaleSal::ProcessOptions(int argc, char** argv) {

 Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {

      case 't':
	if (argc<3){
	  throw string("Not enough command line arguments for switch -t");
	  break;
	}
	
	option.option = "-t";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%f",&globalThreshold) != 1)
	  throw string("switch -t requires a floating point argument.");
	
	argc--;
	argv++;
	break;


      case 'k':
	if (argc<3){
	  throw string("Not enough command line arguments for switch -k");
	  break;
	}
	
	option.option = "-k";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&numberOfMeans) != 1)
	  throw string("switch -k requires an integer specifier.");
	
	argc--;
	argv++;
	break;

      case 'e':
	if (argc<3){
	  throw string("Not enough command line arguments for switch -e");
	  break;
	}
	
	option.option = "-e";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&clustersToKeep) != 1)
	  throw string("switch -e requires an integer specifier.");
	
	argc--;
	argv++;
	break;

//       case 'i':
// 	if (argc<3){
//	  throw string("Not enough command line arguments for switch -k");
//	  break;
//	}
	
//	option.option = "-i";
//	option.addArgument(argv[1]);
	
//	if(sscanf(argv[1],"%d",&iterations) != 1)
//	  throw string("switch -i requires an integer specifier.");
	
//	argc--;
//	argv++;
//	break;

//      case 'N':
//	if (argc<3){
//	  throw string("Not enough command line arguments for switch -N");
//	  break;
//	}
	
//	option.option = "-N";
//	option.addArgument(argv[1]);
	
//	if(sscanf(argv[1],"%d",&fftN) != 1)
//	  throw string("switch -N requires an integer specifier.");
	
//	argc--;
//	argv++;
//	break;

//      case 'K':
//	if (argc<3){
//	  throw string("Not enough command line arguments for switch -K");
//	  break;
//	}
	
//	option.option = "-K";
//	option.addArgument(argv[1]);
	
//	if(sscanf(argv[1],"%d",&fftK) != 1)
//	  throw string("switch -K requires an integer specifier.");
//	
//	argc--;
//	argv++;
//	break;

      case 'v':
	if (argc<3){
	  throw string("Not enough command line arguments for switch -v");
	  break;
	}
	
	option.option = "-v";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%f",&globalThreshold) != 1)
	  throw string("switch -v requires a floating point argument.");
	
	argc--;
	argv++;
	break;
	
      case 'm':
	option.option = "-m";
	skipClustering=true;
	break;

      case 'E':
	option.option = "-E";
	clustersToKeep=6;
	break;

      default:
	throw string("unknown option ")+argv[0];
      } // switch

      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
    
    return argc_old-argc;
    
  }
  /////////////////////////////////////////////////////////////////////////////

  void ScaleSal::createDeltaLists(int startScale,int stopScale)
  {
    deltaList=vector<coordList>(stopScale+1);

    int dx,dy;

    for(dx=1;dx<=stopScale;dx++)
      for(dy=dx;dy<=stopScale;dy++){
	int ind = (int)ceil(sqrt((double)(dx*dx+dy*dy)));
	if(ind >= startScale && ind<=stopScale)
	  deltaList[ind].push_back(coord(dx,dy));
      }
  }

  /////////////////////////////////////////////////////////////////////////////
  void ScaleSal::quantise()
  {
    delete quantised_image;

    int w=getImg()->getWidth();
    int h=getImg()->getHeight();

    quantised_image=new int[w*h];
    int x,y;
    
    vector<float> v;

    for(y=0;y<h;y++) for(x=0;x<w;x++){
      getImg()->get_pixel_rgb(x,y,v);

      // now quantise v

      // for now using straightforward scheme:
      // y channel 5 bits
      // i channel 2 bits
      // q channel 2 bits

      float Y= 0.299*v[0]+0.587*v[1]+0.144*v[2]; // [0;1.030]
      float I= 0.596*v[0]-0.275*v[1]-0.321*v[2]; // [-0.596;0.596]
      float Q= 0.212*v[0]-0.528*v[1]+0.311*v[2]; // [-0.528;0.523]

      Y /= 1.03; // [0,1]

      I += 0.596;
      I /= 2*0.596; // [0,1]

      Q += 0.528;
      Q /= 0.528 + 0.523; // [0,1]

      int y_q = (int)(32*Y);
      if(y_q==32) y_q=31;

      int i_q = (int)(4*I);
      if(i_q==4) i_q=3;

      int q_q = (int)(4*Q);
      if(q_q==4) q_q=3;

      quantised_image[x+y*w]= (y_q << 4) + (i_q << 2) + q_q;

    }
   
  }

  /////////////////////////////////////////////////////////////////////////////

  void ScaleSal::collectSalientPoints(int x,int y,int minscale,int maxscale)
  {

    //    cout << "collectSalientPoints(" << x<<","<<y<<")" << endl;

    Histogram hist,oldHist;
    hist.reset(qsteps());
    oldHist.reset(qsteps());

    // special case s==1

      addPoint(hist,x,y);

      int sc;

      for(sc=2;sc<minscale-1;sc++){

	coordList::const_iterator it;
	for(it=deltaList[sc].begin(); it != deltaList[sc].end(); it++){

	  // 8-fold symmetry is implied
	  
	  addPoint(hist,x+(*it).x,y+(*it).y);
	  addPoint(hist,x+(*it).x,y-(*it).y);
	  addPoint(hist,x-(*it).x,y+(*it).y);
	  addPoint(hist,x-(*it).x,y-(*it).y);

	  if((*it).x !=(*it).y){
	    addPoint(hist,x+(*it).y,y+(*it).x);
	    addPoint(hist,x+(*it).y,y-(*it).x);
	    addPoint(hist,x-(*it).y,y+(*it).x);
	    addPoint(hist,x-(*it).y,y-(*it).x);
	  }
	}      
	addPoint(hist,x,y+sc-1); 
	addPoint(hist,x,y-sc+1);
	addPoint(hist,x+sc-1,y);
	addPoint(hist,x-sc+1,y);
      }

      oldHist=hist;
      sc=minscale-1;
      coordList::const_iterator it;
      for(it=deltaList[sc].begin(); it != deltaList[sc].end(); it++){
      
      // 8-fold symmetry is implied
      
	addPoint(hist,x+(*it).x,y+(*it).y);
	addPoint(hist,x+(*it).x,y-(*it).y);
	addPoint(hist,x-(*it).x,y+(*it).y);
	addPoint(hist,x-(*it).x,y-(*it).y);
      
	if((*it).x !=(*it).y){
	  addPoint(hist,x+(*it).y,y+(*it).x);
	  addPoint(hist,x+(*it).y,y-(*it).x);
	  addPoint(hist,x-(*it).y,y+(*it).x);
	  addPoint(hist,x-(*it).y,y-(*it).x);
	}
      }      
      addPoint(hist,x,y+sc-1); 
      addPoint(hist,x,y-sc+1);
      addPoint(hist,x+sc-1,y);
      addPoint(hist,x-sc+1,y);
  
  
      //  cout << "points added to histograms" << endl;
  
      float entropy=hist.calcEntropy();
      //cout << "entropy of the new hist is " << entropy;
      float weight=hist.calcDifference(oldHist);
      //cout << "the weight is " << weight << endl;

      // comparison to entropy of previous scale
      bool canBeMax=true;
  
      // all other scales
  
      for(sc=minscale;sc<=maxscale;sc++){
      
	//      cout << "processing scale " << sc << endl;
	oldHist=hist;
	// hist.reset(qsteps());
	coordList::const_iterator it;
	
	//cout << "deltaList has size" << deltaList[sc].size() << endl; 
	
	for(it=deltaList[sc].begin(); it != deltaList[sc].end(); it++){
	  // cout << "delta item: ("<< (*it).x<<","<<(*it).y<<")" << endl;
	  
	  // 8-fold symmetry is implied
	  
	  addPoint(hist,x+(*it).x,y+(*it).y);
	  addPoint(hist,x+(*it).x,y-(*it).y);
	  addPoint(hist,x-(*it).x,y+(*it).y);
	  addPoint(hist,x-(*it).x,y-(*it).y);
	  
	  if((*it).x !=(*it).y){
	    addPoint(hist,x+(*it).y,y+(*it).x);
	    addPoint(hist,x+(*it).y,y-(*it).x);
	    addPoint(hist,x-(*it).y,y+(*it).x);
	    addPoint(hist,x-(*it).y,y-(*it).x);
	  }
	}      
	
	// special treatment for points on coordinate axes
	
	addPoint(hist,x,y+sc-1); // we define ball(sc) to include points
	// x^2 + y^2 < sc^2 (smoother image than 
	// with equality)
	addPoint(hist,x,y-sc+1);
	addPoint(hist,x+sc-1,y);
	addPoint(hist,x-sc+1,y);

	//cout << "sc="<<sc<<", hist grew "<<oldHist.totalCount <<
	//  " ->" << hist.totalCount << endl; 
	
	float newH = hist.calcEntropy();
	//cout << "calculated new entropy " << newH <<" , old is " << entropy << endl;
	
	if(newH <= entropy){
	  if(canBeMax){
	    //  cout << "old was maximum, maxThusFar=" << maxThusFar << endl;
	    float val=sc*sc*weight*entropy/(2*sc-1);
	    if(val>globalThreshold * maxThusFar){
	      // cout << "Adding val "<< val <<" maxThusFar="<<maxThusFar<<endl;
	      //	      (*saliencyAcc)[XYS(x,y,sc-1)]=val;
	      saliencyAcc->push_back(salCluster(x,y,sc-1,val));
	      if(val>maxThusFar) maxThusFar=val;
	    }
	  }
	  canBeMax =false;
	} else{
	  canBeMax=true;
	  //cout << "calculating weight... " << endl;
	  weight=hist.calcDifference(oldHist);
	//	  cout << "weight=" << weight << endl;
	}
	entropy=newH;
      }

      // the maximum scale is inserted if it has larger entropy than the
      // previous scale
      
      if(canBeMax){
	float val=sc*sc*weight*entropy/(2*sc-1);
	if(val>globalThreshold * maxThusFar){
	  saliencyAcc->push_back(salCluster(x,y,sc-1,val));
	//	  (*saliencyAcc)[XYS(x,y,sc-1)]=val;
	  if(val>maxThusFar) maxThusFar=val;
	}
      }
  }

  /////////////////////////////////////////////////////////////////////////////
    
  void ScaleSal::kadirClustering(){

    if(Verbose()>1) cout << "Clustering" << endl;

      // sort the list of clusters according to saliency
      
    //cout << "list contains " << sorted.size() << " entries " << endl;

    if(skipClustering){
      clusteredSal=saliencyAcc;
      saliencyAcc=NULL;
      return;
    }
    
    sort(saliencyAcc->begin(),saliencyAcc->end(),salPoint::gtSal);
    //    cout << "sal[0]=" << sorted[0].sal<<endl; 
    //cout << "sal[1]=" << sorted[2].sal<<endl; 


    int K=numberOfMeans;

    vector<salCluster> *knearest=new vector<salCluster>;
    
    while(!saliencyAcc->empty()){

      //      cout << "Clustering : " << saliencyAcc->size() 
      //	   << " points left." << endl; 

      knearest->clear();
      findKNearest(&saliencyAcc,&knearest,K);

      // estimate means  and variances of K nearest points 

      //      cout << knearest->size() << " nearest neighbours found" << endl;

      float sx,sy,ss,sv,sqx,sqy,sqs,sqv;
      
      sx=sy=ss=sv=sqx=sqy=sqs=sqv=0;
      
      int j,n=knearest->size();
      for(j=0;j<n;j++){
	sx += (*knearest)[j].x;
	sy += (*knearest)[j].y;
	ss += (*knearest)[j].s;
	sv += (*knearest)[j].sal;
	
	sqx += (*knearest)[j].x*(*knearest)[j].x;
	sqy += (*knearest)[j].y*(*knearest)[j].y;
	sqs += (*knearest)[j].s*(*knearest)[j].s;
	sqv += (*knearest)[j].sal*(*knearest)[j].sal;
      }
      
      sx /= n; 
      sy /= n; 
      ss /= n; 
      sv /= n; 
      
      sqx /= n; 
      sqy /= n; 
      sqs /= n; 
      sqv /= n; 
       
      // variances estimated using N (instead of unbiased N-1)
      // to avoid special treatment when N==1
       
       
      sqx -= sx*sx;
      sqy -= sx*sy;
      sqs -= sx*ss;
      sqv -= sx*sv;

      float maxvar=max(sqx,max(sqy,sqs));

      //cout << "candidate cluster: (sx,sy,ss,sal)=("<<sx<<","<<sy<<","<<ss<<
      //	","<<sv<<")"<<endl;

      if(maxvar < varThreshold){
	
	// here some rounding error 
	XYS ref((int)(sx+0.5),(int)(sy+0.5),(int)(ss+0.5));
        size_t i;
	 
	float mindist=99999999999.0;
	
	for(i=0;i<clusteredSal->size();i++){
	  float d=(*clusteredSal)[i].calcSqrDistTo(ref);	   
	  if(d<mindist){mindist=d;}
	}

	// cout << "mindist="<<mindist<<endl;
       
	if(mindist>ss*ss){
	  clusteredSal->push_back(salCluster((int)(sx+0.5),(int)(sy+0.5),
					     (int)(ss+0.5),sv));
	}
      }
    }
     
    // simple bypass for the moment
      
    //clusteredSal = saliencyAcc;
    
    // cout << "after clustering # of points: " << clusteredSal.size() <<  endl;
    
  }

  /////////////////////////////////////////////////////////////////////////////

  void ScaleSal::findKNearest(vector<salCluster> **sList, 
			      vector<salCluster> **result, int K){

    if(!sList || !result) 
      throw string("ScaleSal::findKnearest(): NULL pointer");

    vector<salCluster> *tmpPtr;

    if((*sList)->size() <= (size_t)K){ 
      //  cout << "short slist" << endl;
      tmpPtr=*result; 
      *result=*sList; 
      *sList=tmpPtr;
      (*sList)->clear();
      return;
    }
    
    salCluster tgt = (**sList)[0];
    
    vector<salCluster>::iterator it;
    int ind=0;
    for(it=(*sList)->begin();it!=(*sList)->end();it++){
	it->ind = ind++;
	it->calcSqrDistTo(tgt);
    }

    //    cout << "distances to target calculated" << endl;
    
    vector<salCluster>
      cmpList = **sList; // sort this list according to distance to tgt

    sort(cmpList.begin(),cmpList.end(),salCluster::clusterFurther); 

    //    cout << "First distances:" <<cmpList[0].dist<<","<<cmpList[1].dist<<endl;

    // closest clusters end in the end of the list
    
    //    cout << "cmpList sorted" << endl;

    int i;
    set<int> rInd;
    int cind=cmpList.size()-1;
    
    for(i=0;i<K;i++){
      (*result)->push_back(cmpList[cind]);
      rInd.insert(cmpList[cind].ind);
      cind--;
    }
    
    // cout << "result list formed w/ " << result.size() << " entries." << endl;

    vector<salCluster> *oList=*sList;
    *sList=new vector<salCluster>;

    for(i=0;i<(int)oList->size();i++){
      if(!rInd.count(i)) 
	(*sList)->push_back((*oList)[i]);
    }

    //    cout << "list sizes: (r,s)=" << (*result)->size() 
    // << "," << (*sList)->size() << ")" << endl;

    delete oList;

  }
  
  /////////////////////////////////////////////////////////////////////////////
  void ScaleSal::labelRegions(){
    
    sort(clusteredSal->begin(),clusteredSal->end(),salPoint::ltSal);
    
    if(Verbose()>1)
      cout << "Labeling " << clusteredSal->size() <<" regions" << endl;
    
    // cout << "after sorting " << clusteredSal.size() << " entries " << endl;
    
    for(int x=0;x<Width();x++) for(int y=0;y<Height();y++)
      getImg()->set_pixel_segment(x,y,0);

    size_t i;
    int lbl=1;

    int start = 0;

    if(clustersToKeep>0) start=clusteredSal->size()-1-clustersToKeep;
    
    for(i=start;i<clusteredSal->size();i++){
      int dx,dy;
      int sx=(*clusteredSal)[i].x;
      int sy=(*clusteredSal)[i].y;
      int ss=(*clusteredSal)[i].s;
      //            float sal=(*clusteredSal)[i].sal;

	//      cout << "marking region (" << sx<<","<<sy<<","<<ss<<") w/ sal "<<
	    //	sal<<" & w/ lbl "<< i+1 <<
	    //	endl;
      
      for(dx=-ss;dx<=ss;dx++) for(dy=-ss;dy<=ss;dy++)
	if(dx*dx+dy*dy <= ss*ss && 
	   getImg()->coordinates_ok(sx+dx,sy+dy))
	  getImg()->set_pixel_segment(sx+dx,sy+dy,lbl);
      lbl++;
    }
      
      
    
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool ScaleSal::Process() {

    //  fftw_test();
    // return ProcessNextMethod();

    int x,y;
    int w=getImg()->getWidth();
    int h=getImg()->getHeight();

    maxscale=min(100,w/5);
    minscale=min(maxscale,7);

    maxThusFar=0;

    if(saliencyAcc)
      saliencyAcc->clear();
    else 
      saliencyAcc=new vector<salCluster>;

    if(clusteredSal)
      clusteredSal->clear();
    else 
      clusteredSal=new vector<salCluster>;

    quantise();

    // cout << "quantised image" << endl;

    for(y=0;y<h;y++) for(x=0;x<w;x++)
      collectSalientPoints(x,y,minscale,maxscale);

    // //   cout << "Testing delta-lists:" << endl;
    // //     {
  //     int s,x=30,y=30;
    //       coordList::const_iterator it;

    //       for(s=1;s<=27;s++){
    // 	getImg()->set_pixel_segment(x+s-1,y,s);
    //getImg()->set_pixel_segment(x-s+1,y,s);
    //	getImg()->set_pixel_segment(x,y+s-1,s);
    //getImg()->set_pixel_segment(x,y-s+1,s);

    //for(it=deltaList[s].begin(); it != deltaList[s].end(); it++){
    //  getImg()->set_pixel_segment(x+(*it).x,y+(*it).y,s);
    //  getImg()->set_pixel_segment(x+(*it).x,y-(*it).y,s);
    //  getImg()->set_pixel_segment(x-(*it).x,y+(*it).y,s);
    //  getImg()->set_pixel_segment(x-(*it).x,y-(*it).y,s);

    //  if((*it).x !=(*it).y){
    //    getImg()->set_pixel_segment(x+(*it).y,y+(*it).x,s);
    //    getImg()->set_pixel_segment(x+(*it).y,y-(*it).x,s);
    //    getImg()->set_pixel_segment(x-(*it).y,y+(*it).x,s);
    //    getImg()->set_pixel_segment(x-(*it).y,y-(*it).x,s);
    //  }

    //}
    //}
    //}

    //return ProcessNextMethod();

    //cout << "salient points collected" << endl;

    if(Verbose()>1) 
      cout << "# of salient points: " << saliencyAcc->size()
	   << endl;

    vector<salCluster>::iterator sit;
    vector<salCluster> *uList=saliencyAcc;
    
    saliencyAcc=new vector<salCluster>;

    // filter away small accumulator values

    for(sit=uList->begin(); sit!=uList->end(); sit++){
      if(sit->sal >= globalThreshold*maxThusFar){
	saliencyAcc->push_back(*sit);
      }
    }

    delete uList;

    if(Verbose()>1)
      cout << "after filtering # of salient points: " << saliencyAcc->size()
	   << endl;

    kadirClustering();

    if(Verbose()>1)
      cout << "after filtering # of salient points: " << clusteredSal->size()
	   << endl;

    labelRegions();

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

  /*  void ScaleSal::fftw_test(){

  cout << "convolution profiling: iterations=" << iterations << endl;
  cout << "map width = "<<fftN<<" kernel width = "<<fftK << endl;
    
    // convolution with FFT

    fftw_complex *out,*mask;
   double *in;
   struct tms start_time, stop_time;

    // we pretend mask to be pre-transformed

   int N=fftN;  // determine size of transform and convolution mask
   int K=fftK;

   fftw_plan p,bp;
    

    //    int s=2*N; // zero padding

   int ks;
   for(ks=1;ks<2*K+1;ks <<=1);

   ks=max(ks,N>>3);

   cout << "using ks="<<ks<<endl;

   int s=N+ks;

   int resultsize = s/2 +1; // conjugate symmetry in transform plane

   in = (double *)fftw_malloc(sizeof(double) * s);
   out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * resultsize);
   mask = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * resultsize);


   cout << "Start planning" << endl;

   p = fftw_plan_dft_r2c_1d(s,in,out,FFTW_PATIENT);
   bp = fftw_plan_dft_c2r_1d(s,out,in,FFTW_PATIENT);

   cout << "Planning done" << endl;

   int i;
   
   for(i=0;i<resultsize;i++)
   mask[i][0]=mask[i][1]=1.0/s;

   for(i=0;i<s;i++)
   in[i]=0.3;
   
   // begin measurement
   
   times(&start_time); 
   
   
   for(i=0;i<iterations;i++){
   
   int k;
   for(k=0;k<s;k++)
   in[k]=0.3;
   
   //           cout << "dump of input" << endl;
   //       int j;
   //    for(j=0;j<s;j++)
   //cout << "in["<<j<<"]:"<<in[j]<<endl;
   
   //cout << "forward transform" << endl;
   fftw_execute(p);
  //cout << "dump of ouput" << endl;
  
  //for(j=0;j<resultsize;j++)
  //	cout << "out["<<j<<"]:"<<out[j][0]<<"+"<<out[j][1]<<"i"<<endl;
  
      //cout << "complex multiply" << endl;
  multiplyComplexVector(out,mask,resultsize);
      //cout << "backward transform" << endl;
      fftw_execute(bp);
    }

    times(&stop_time);

    fftw_destroy_plan(p);
    fftw_destroy_plan(bp);


    cout << " fft: start_user: " << start_time.tms_utime
	 << " stop_user: " << stop_time.tms_utime 
	 << " difference: " << stop_time.tms_utime-start_time.tms_utime
	 << endl;

   
    // convolution using direct calculation
 
    int l=K+1;
    double *dmask = new double[l];
    for(i=0;i<l;i++)
      dmask[i]=0.3;

    // begin measurement

    times(&start_time); 
    
    int x,j;

    for(j=0;j<iterations;j++){
      //      cout << "round " <<j<<endl;
      int k;
      for(k=0;k<N;k++)
       in[k]=0.3;

      for (x=0; x<N; x++) {


	const int imax = l<N-x ? l : N-x, imin = l<=x ? l : x+1;
	register double sum = 0;
	register int i;



	//	cout << "s=" <<s<<" l="<<l<<" x="<<x<<" imax="<<imax<<" imin="<<imin<<endl;

	for (i=0; i<imax; i++) {
	  sum += dmask[i]*in[x+i]; 
	}
	for (i=1; i<imin; i++) {
	  sum += dmask[i]*in[x-i]; 
	}
	in[x]=sum;
      }
    }

    times(&stop_time);

    cout << " direct: start_user: " << start_time.tms_utime
	 << " stop_user: " << stop_time.tms_utime 
	 << " difference: " << stop_time.tms_utime-start_time.tms_utime
	 << endl;



    fftw_free(in); fftw_free(out); fftw_free(mask);    
    exit(1);
    
  }*/

  /*  void ScaleSal::multiplyComplexVector(fftw_complex *v1, 
				       const fftw_complex *v2, int len)
  {
    double ad,bc,s;
    int i;

    // element-wise multiply of two complex vectors, result is placed 
    // in the first one

    for(i=0;i<len;i++){
      ad = v1[i][0]*v2[i][1];
      bc = v1[i][1]*v2[i][0];
      s = (v1[i][0]-v1[i][1])*(v2[i][0]+v2[i][1]);
      v1[i][0]=s-ad+bc;
      v1[i][1]=ad+bc;
    }

    }*/


} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
