// -*- C++ -*-  $Id: OCVCentrist.C,v 1.19 2017/03/10 09:10:23 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)

#include <OCVCentrist.h>

#include <algorithm>
#include <iostream>
#include <fstream>

namespace picsom {
  static const string vcid =
    "$Id: OCVCentrist.C,v 1.19 2017/03/10 09:10:23 jormal Exp $";

  static OCVCentrist list_entry(true);
  
  /////////////////////////////////////////////////////////////////////////////

  bool OCVCentrist::useSobel = false;
  bool OCVCentrist::useBoth = false; // whether use both image and Sobel
  int OCVCentrist::sizeCV = 5;
  int OCVCentrist::extra = 2;
  //int OCVCentrist::splitlevel = 2; // replaced by options.levels
  int OCVCentrist::splitsize[4] = {1,6,31,144};
  double OCVCentrist::splitratio[3] = {1.0,1.0,1.35};

  Funnel *OCVCentrist::funnel = NULL;
  IlluminationNormalization *OCVCentrist::illnorm = NULL;

  /////////////////////////////////////////////////////////////////////////////

  string OCVCentrist::Version() const {
    return vcid;
  }

  /////////////////////////////////////////////////////////////////////////////

  int OCVCentrist::printMethodOptions(ostream &str) const {
    int ret = 0;

    str << "crop=X               "
        << "crop X pixels around the image (default: X=0)" << endl;
    str << "crop=X,Y             "
        << "crop X pixels from x-axis, Y from y-axis" << endl;
    str << "crop=X0,X1,Y0,Y1     "
        << "crop X0 and X1 pixels from x-axis (left and right), Y0 and Y1 from y-axis" 
	<< endl;
    str << "levels=X             "
        << "use X spatial pyramid levels (default: X=2)" << endl;
    str << "pcaout=1             "
	<< "write PCA matrix to: " << PicSOMroot() << "/" << CENTRIST_PCA_OUTPUT 
	<< endl;
    str << "funnel=modelfile     "
        << "perform funneling using model from modelfile"<< endl;
    str << "illnorm=X            "
        << "perform illumination normalization. X=1: crop before, X=2 crop after" 
	<< endl;
    str << "imgout=1             "
        << "write final, processed image to disk with an \"out-\" prefix" << endl;    

    return ret+6;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVCentrist::InitializeCentrist() {
    options.crop.clear();
    options.levels = 2;
    options.generate_pca = false;
    options.write_image = false;    

    string pca_load_path = PicSOMroot() + "/" + string(useSobel ? CENTRIST_PCA_SOBEL : CENTRIST_PCA_NORMAL); 
    
    if (lf.Load((pca_load_path.c_str()), 40, 256) == false) {
      cerr << "OCVCentrist(): PCA matrix loading from [" << pca_load_path
           << "] failed." << endl;
      return false;
    }
    NormalizeLoadFactors(lf);

    options.do_funneling = false;    
    funneling_modelfile = "";
    //funnel = NULL;

    options.do_illnorm = false;
    options.cropfirst = true;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVCentrist::ProcessOptionsAndRemove(list<string>& opts) {
    for (list<string>::iterator i = opts.begin(); i!=opts.end();) {
      string key, value;
      SplitOptionString(*i, key, value);

      if (key=="crop") {
	vector<string> cropvals = SplitInCommasObeyParentheses(value);
	switch (cropvals.size()) {
	case (1):
	  options.crop.set(atoi(cropvals[0].c_str()));
	  break;
	case (2):
	  options.crop.set(atoi(cropvals[0].c_str()), atoi(cropvals[1].c_str()));
	  break;
	case (4):
	  options.crop.set(atoi(cropvals[0].c_str()), atoi(cropvals[1].c_str()),
			   atoi(cropvals[2].c_str()), atoi(cropvals[3].c_str()));
	  break;
	default:
	  cerr << "OCVCentrist(): Parsing option \"crop\" failed: value=\"" 
	       << value << "\"" << endl;
	  return false;
	}

      } else if (key == "levels")
        options.levels = atoi(value.c_str());
      
      else if (key == "pcaout")
        options.generate_pca = true;

      else if (key == "imgout")
        options.write_image = true;

      else if (key == "funnel") {
        options.do_funneling = true;
	funneling_modelfile = value;
	if (!funnel)	
	  funnel = new Funnel();

      } else if (key == "illnorm") {
        options.do_illnorm = true;
	if (!illnorm)
	  illnorm = new IlluminationNormalization();
	if (atoi(value.c_str())==2)
	  options.cropfirst = false;

      } else { // else continue without erasing option from list
        i++;
        continue;
      }

      i = opts.erase(i);
    }

    return Feature::ProcessOptionsAndRemove(opts);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVCentrist::CalculateOpenCV(int /*f*/, int l) {
    static const string msg = "OCVCentrist::CalculateOpenCV(): ";
    featureVector fv;

    using namespace cv;

    IplImage *ipl_image = new IplImage(CurrentFrameOCV());
    IplImage *from_funnel, *from_illnorm;

    if (options.do_funneling) {
      if (!funnel) {
	cerr << msg << "Creation of funnel object failed." << endl;
	return false;
      }
      if (!funnel->ModelLoaded()) {
	if (!funnel->LoadModel(funneling_modelfile)) {
	  cerr << msg << "Loading of funneling model failed." << endl;
	  return false;
	}
      }
      if (!funnel->Process(&ipl_image)) {
	cerr << msg << "Funneling of input image failed." << endl;
	return false;
      }
      from_funnel = ipl_image;
    }

    if (options.do_illnorm) {
      if (!illnorm) {
	cerr << msg << "Creation of illnorm object failed." << endl;
	return false;
      }
      bool ok = true;
      if (options.cropfirst) {
	ok = illnorm->Process(&ipl_image, options.crop);
	options.crop.clear();
      } else {
	ok = illnorm->Process(&ipl_image);
      }
      if (!ok) {
	cerr << msg << "Illumination normalization of input image failed." 
	     << endl;
	return false;
      }	
      from_illnorm = ipl_image;
    }

    IntImage<double> im;
    if (!im.FromMat(ipl_image, options.crop, 
		    options.write_image ? BaseFileName() : "")) {
      cerr << msg << "IntImage creation failed." << endl;
      return false;
    }

    Array2d<double> features;
    size_t fsize = (lf.nrow+extra)*splitsize[options.levels]*(1+useBoth);
    features.Create(1, fsize);

    ofstream pcaout;
    if (options.generate_pca) {
      string pca_save_path = PicSOMroot() + "/" + CENTRIST_PCA_OUTPUT;
      pcaout.open(pca_save_path.c_str(), ios::out | ios::app);
    }
    
    GenerateFeatureForSingleChannelImage(im, &features.p[0][0],
					 (options.generate_pca?&pcaout:NULL));
    
    if (options.generate_pca) 
      pcaout.close();
    
    for (int j=0; j<features.ncol; j++)
      fv.push_back(features.p[0][j]);

    // funnel and illnorm create new IplImages and changes ipl_image to point 
    // to them, so these have to be released.
    if (options.do_funneling)
      cvReleaseImage(&from_funnel);
    if (options.do_illnorm)
      cvReleaseImage(&from_illnorm);
      
    SetVectorData(l, fv);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::NormalizeLoadFactors(Array2d<double>& lf)
  {// normalize such that the eigenvectors have 0 mean and 1 std
    for(int i=0;i<lf.nrow;i++)
      {
        double sum;
        sum = 0; for(int j=0;j<lf.ncol;j++) sum+=lf.p[i][j]; sum/=lf.ncol;
        for(int j=0;j<lf.ncol;j++) lf.p[i][j] -= sum;
        sum = 0; for(int j=0;j<lf.ncol;j++) sum += lf.p[i][j]*lf.p[i][j]; sum /= lf.ncol;
        // What we really need is a for a feature vector x and a load factor f, calculate their correlation coefficient
        // so we need to make f 0 mean and 1 norm -- instead of 1 sigma as the code here did
        // but the difference is only a constant, i.e. equivalent if no floating point roundoff errors are considered
        // and because I do not want to run the code again on all datasets, let just don't remove the 'sum /= lf.ncol' sentence. Prentend it's not there :-)
        sum = 1.0/sqrt(sum);
        for(int j=0;j<lf.ncol;j++) lf.p[i][j] *= sum;
      }
  }

  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::PrepareScaleRatio(Array2dC<double>& scaleratio)
  {//if options.levels==2, the level 0 (entire image) features had a different (higher) weight
    for(int i=0;i<scaleratio.ncol;i++) scaleratio.buf[i]=1.0;
    if(options.levels==2)
      {
        for(int i=0;i<scaleratio.ncol;i++)
	  {
            int index = i/(lf.nrow+extra);
            if(index<25)
	      scaleratio.buf[i] = splitratio[0];
            else if(index<30)
	      scaleratio.buf[i] = splitratio[1];
            else if(index<31)
	      scaleratio.buf[i] = splitratio[2];
            else if(index<56)
	      scaleratio.buf[i] = splitratio[0];
            else if(index<61)
	      scaleratio.buf[i] = splitratio[1];
            else
	      scaleratio.buf[i] = splitratio[2];
	  }
      }
  }

  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::FindMinMaxValue(Array2d<double>& features,Array2dC<bool>& train,Array2dC<double>& minmax)
  {   // find min/max feature values of the training set in order to normalize features to [-1 1]
    // minmax.p[0] -- min; minmax.p[1] -- max
    for(int i=0;i<features.ncol;i++) { minmax.p[0][i]= 1E20; minmax.p[1][i]=-1E20; }
    for(int i=0;i<features.nrow;i++)
      {
        if(train.buf[i]==false) continue;
        for(int j=0;j<features.ncol;j++) 
	  {
            if(features.p[i][j]<minmax.p[0][j]) minmax.p[0][j] = features.p[i][j];
            if(features.p[i][j]>minmax.p[1][j]) minmax.p[1][j] = features.p[i][j];
	  }
      }
    for(int i=0;i<features.ncol;i++)
      {
        if(minmax.p[0][i]>=minmax.p[1][i])
	  {
            //std::cerr<<"There exists features with equal min value and max value."<<std::endl;
            minmax.p[1][i] = 0.0;
	  }
        else
	  minmax.p[1][i] = 2.0/(minmax.p[1][i]-minmax.p[0][i]);
      }
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::ScaleFeatures(Array2d<double>& features,Array2d<double>& test)
  {  //similar to FindMinMaxValue, but here train/test set are provided separately
    assert(features.ncol == test.ncol); //train and test have same number of features
    for(int j=0;j<features.ncol;j++)
      {
        double fmin = 1E20, fmax = -fmin;
        for(int i=0;i<features.nrow;i++)
	  {
            if(features.p[i][j]>fmax) fmax = features.p[i][j];
            if(features.p[i][j]<fmin) fmin = features.p[i][j];
	  }
        double r;
        if(fmax-fmin<1e-10) r = 0; else r = 1.0/(fmax-fmin);
        for(int i=0;i<features.nrow;i++) features.p[i][j] = (features.p[i][j]-fmin)*r;
        if(features.p!=test.p) 
	  for(int i=0;i<test.nrow;i++) test.p[i][j] = (test.p[i][j]-fmin)*r;
      }
  }
  
  //!!!!!!!!!! PACT
  //!!!!!!!!!! comment out the 2 functions for orientation histogram below, and uncomment these 2 functions for PACT to use PACT as features
  void OCVCentrist::GenerateHistForOneRect(double** p,const int x1,const int x2,const int y1,const int y2,double* pointer,double* temp)
  {   // generate CT histogram for a given rectangle: pixels in [x1 y1]-(x2,y2) -- including (x1,y1) but excluding (y1,y2)

    for(int i=0;i<256;i++) temp[i]=0;
    double pixelSum = 0, pixelSq = 0;
    for(int x=x1;x<x2;x++)
      {
        for(int y=y1;y<y2;y++)
	  {
            pixelSum += p[x][y];
            pixelSq += p[x][y]*p[x][y];
            temp[CensusTransform(p,x,y)]++;
	  }
      }
    
    const double size = (x2-x1)*(y2-y1);
    if(extra)
      {
        pixelSum /= ( size*255.0 );
        pixelSq /= ( size*255.0*255.0 );
        pixelSq -= pixelSum*pixelSum; if(pixelSq<1E-6) pixelSq=0; else pixelSq = sqrt(pixelSq);
        pointer[0] = pixelSum;
        if(pixelSum<1E-6)
	  {
            if(extra>=2) pointer[1]=0;
        }
        else 
	  {
            if(extra>=2) pointer[1]=pixelSq/pixelSum;
	  }
      }
    // normalize histogram to have 0 mean, remove the first and last entry, and normalize to have unit norm
    double sum = 0; 
    for(int j=0;j<256;j++) sum += temp[j]; 
    sum/=256;
    for(int j=0;j<256;j++) temp[j] -= sum;
    temp[0] = temp[255] = 0;
    double sq = 0; 
    for(int j=0;j<256;j++) sq += temp[j]*temp[j]; 
    sq = (sq>100)?1.0/sqrt(sq):0; 
    for(int j=0;j<256;j++) temp[j] *= sq;
  }

  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::GenerateFeatureForOneRect(double** p,const int x1,const int x2,const int y1,const int y2,double* pointer,Array2d<double>& lf,std::ofstream* pcaout)
  {   // generate PCA of histogram
    double temp[256];
    
    GenerateHistForOneRect(p,x1,x2,y1,y2,pointer,temp);
    if(pcaout!=NULL) // save the histogram for generating PCA eigenvectors if 'pcaout' file handler is not NULL
      {
        for(int j=0;j<256;j++) (*pcaout)<<temp[j]<<" ";
        (*pcaout)<<std::endl;
      }

    for(int j=0;j<lf.nrow;j++) 
      { 
        double sum=0; 
        for(int k=0;k<256;k++) 
	  sum+=lf.p[j][k]*temp[k];
        pointer[j+extra] = sum; 
      }

  }
  
//!!!!!!!!! Orientation histogram
//!!!!!!!!! comment out the above 2 functions, and uncomment the below 2, to change from PACT to orientation histogram
// void GenerateHistForOneRect(double** p,const int x1,const int x2,const int y1,const int y2,double* pointer,double* temp)
// {   // generate CT histogram for a given rectangle: pixels in [x1 y1]-(x2,y2) -- including (x1,y1) but excluding (y1,y2)
//  for(int i=0;i<40;i++) temp[i]=0;
//  double pixelSum = 0, pixelSq = 0;
//  for(int x=x1;x<x2;x++)
//  {
//      for(int y=y1;y<y2;y++)
//      {
//          pixelSum += p[x][y];
//          pixelSq += p[x][y]*p[x][y];
//          temp[int((atan2(p[x][y]-p[x+1][y],p[x][y]-p[x][y+1])+PI)/(2.0*PI/39.9999))]++;
//      }
//  }

//  const double size = (x2-x1)*(y2-y1);
//  if(extra)
//  {
//      pixelSum /= ( size*255.0 );
//      pixelSq /= ( size*255.0*255.0 );
//      pixelSq -= pixelSum*pixelSum; if(pixelSq<1E-6) pixelSq=0; else pixelSq = sqrt(pixelSq);
//      pointer[0] = pixelSum;
//      if(pixelSum<1E-6)
//      {
//          if(extra>=2) pointer[1]=0;
//      }
//      else 
//      {
//          if(extra>=2) pointer[1]=pixelSq/pixelSum;
//      }
//  }
//  // normalize histogram to have 0 mean, and normalize to have unit norm
//  double sum = 0; 
//  for(int j=0;j<40;j++) sum += temp[j]; sum/=40;
//  for(int j=0;j<40;j++) temp[j] -= sum;
//  double sq = 0; 
//  for(int j=0;j<40;j++) sq += temp[j]*temp[j]; 
//  sq = (sq>100)?1.0/sqrt(sq):0; 
//  for(int j=0;j<40;j++) temp[j] *= sq;
// }

// void GenerateFeatureForOneRect(double** p,const int x1,const int x2,const int y1,const int y2,double* pointer,Array2d<double>& lf,std::ofstream* /*pcaout*/)
// {   // generate PCA of histogram
//  double temp[40];

//  GenerateHistForOneRect(p,x1,x2,y1,y2,pointer,temp);

//  for(int j=0;j<lf.nrow;j++) pointer[j+extra] = temp[j]; 
// }

  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::GenerateFeatureForOneSplit(IntImage<double>& im,const int splitX,const int splitY,double* pointer,Array2d<double>& lf,std::ofstream* pcaout)
  {   // generate features for a split level, which divide the image into splitX*splitY blocks
    for(int splitx=0;splitx<splitX;splitx++) 
      {
        for(int splity=0;splity<splitY;splity++) 
	  {
            int x1=1+im.nrow/splitX*splitx, x2=im.nrow/splitX*(splitx+1)-1;
            int y1=1+im.ncol/splitY*splity, y2=im.ncol/splitY*(splity+1)-1;
            if(useSobel)
	      {
                if(x1==1) x1++;
                if(x2==im.nrow-1) x2--; 
                if(y1==1) y1++;
                if(y2==im.ncol-1) y2--;
	      }
            GenerateFeatureForOneRect(im.p,x1,x2,y1,y2,
				      &pointer[(splitx*splitY+splity)*(lf.nrow+extra)],
				      lf,pcaout);
	  }
      }
    // we also generate features with shifted blocks (splitX-1)*(splitY-1)
    for(int splitx=0;splitx<splitX-1;splitx++) 
      { // all rectangles will not be at borders, so useSobel=true/false does not affect this part
        for(int splity=0;splity<splitY-1;splity++) 
	  {
            int x1=1+im.nrow/splitX*splitx+im.nrow/(2*splitX), x2=im.nrow/splitX*(splitx+1)-1+im.nrow/(2*splitX);
            int y1=1+im.ncol/splitY*splity+im.ncol/(2*splitY), y2=im.ncol/splitY*(splity+1)-1+im.ncol/(2*splitY);
            GenerateFeatureForOneRect(im.p,x1,x2,y1,y2,&pointer[(lf.nrow+extra)*(splitX*splitY+(splitx*(splitY-1)+splity))],lf,pcaout);
	  }
      }
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::GenerateFeatureForSingleChannelImage(IntImage<double>& im,double* pointer,std::ofstream* pcaout,Array2d<double>& lf)
  {   // single channel image (grey, or R, G, or B channel);
    IntImage<double> resized;
    if(options.levels==0)
      {
        if(useSobel)
	  {
            im.Sobel(resized,true,true);
            GenerateHistForOneRect(resized.p,2,resized.nrow-2,2,resized.ncol-2,pointer,pointer+extra);
	  }
        else
	  GenerateHistForOneRect(im.p,1,im.nrow-1,1,im.ncol-1,pointer,pointer+extra);
        return;
      }
    
    int baseoffset = 0;
    assert(options.levels>=1 && options.levels<=3);
    
    if(options.levels==3)
      {
        if(useSobel)
	  {
            im.Sobel(resized,true,true);
            GenerateFeatureForOneSplit(resized,8,8,pointer,lf,pcaout);
	  }
        else
	  GenerateFeatureForOneSplit(im,8,8,pointer,lf,pcaout);
        baseoffset = (lf.nrow+extra)*(8*8+7*7);
      }
    if(options.levels>=2)
      {
        if(useSobel)
	  {
            im.Sobel(resized,true,true);
            GenerateFeatureForOneSplit(resized,4,4,pointer+baseoffset,lf,pcaout);
	  }
        else
	  GenerateFeatureForOneSplit(im,4,4,pointer+baseoffset,lf,pcaout);
        im.Resize(resized,0.5); im = resized;
        baseoffset += (lf.nrow+extra)*(4*4+3*3);
      }
    if(options.levels>=1)
      {
        if(useSobel)
	  {
            im.Sobel(resized,true,true);
            GenerateFeatureForOneSplit(resized,2,2,pointer+baseoffset,lf,pcaout);
	  }
        else    
	  GenerateFeatureForOneSplit(im,2,2,pointer+baseoffset,lf,pcaout);
        im.Resize(resized,0.5); im = resized;
        baseoffset += (lf.nrow+extra)*(2*2+1*1);
      }
    if(useSobel)
      {
        im.Sobel(resized,true,true);
        GenerateFeatureForOneSplit(resized,1,1,pointer+baseoffset,lf,pcaout);
      }
    else
      GenerateFeatureForOneSplit(im,1,1,pointer+baseoffset,lf,pcaout);
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void OCVCentrist::GenerateFeatureForSingleChannelImage(IntImage<double>& im,double* pointer,std::ofstream* pcaout)
  {
    IntImage<double> im2;
    im2 = im;
    
    GenerateFeatureForSingleChannelImage(im2,pointer,pcaout,lf);
    if(useBoth)
    {
        bool oldSobel = useSobel;
        im2 = im;
        useSobel = true;
        GenerateFeatureForSingleChannelImage(im2,pointer+splitsize[options.levels]*(lf.nrow+extra),pcaout,lf2);
        useSobel = oldSobel;
    }
  }
  
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::IncreaseCapacity(const int newrow) {
    assert(newrow>nrow);
    T** newp = new T*[newrow]; 
    assert(newp!=NULL);
    memcpy(newp,p,sizeof(T*)*nrow);
    for (int i=nrow;i<newrow;i++) {
      newp[i] = new T[ncol]; 
      assert(newp[i]!=NULL);
    }
    delete[] p; p = NULL;
    p = newp; newp = NULL;
    nrow = newrow;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::DecreaseCapacity(const int newrow) {
    assert(newrow<nrow);    
    T** newp = new T*[newrow]; 
    assert(newp!=NULL);
    memcpy(newp,p,sizeof(T*)*newrow);
    for (int i=newrow;i<nrow;i++) {
      delete[] p[i];
      p[i] = NULL;
    }
    delete[] p; p = NULL;
    p = newp; newp = NULL;
    nrow = newrow;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::Create(const int _nrow,const int _ncol) {
    assert(_nrow>0 && _ncol>0);
    if(ncol==_ncol) return AdjustCapacity(_nrow);
    Clear();
    nrow = _nrow; ncol = _ncol;
    p = new T*[nrow]; assert(p!=NULL);
    for (int i=0;i<nrow;i++) {
      p[i] = new T[ncol]; assert(p[i]!=NULL);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::Swap(Array2d<T>& array2) {
    std::swap(nrow,array2.nrow);
    std::swap(ncol,array2.ncol);
    std::swap(p,array2.p);
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::Clear() {
    for (int i=0;i<nrow;i++) { 
      delete[] p[i]; 
      p[i] = NULL; 
    }
    delete[] p; p = NULL;
    nrow = ncol = 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::AdjustCapacity(const int newrow) {
    assert(newrow>0);
    if(newrow == nrow) 
      return;
    else if(newrow>nrow) 
      IncreaseCapacity(newrow);
    else // newrow < nrow
      DecreaseCapacity(newrow);
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> bool Array2d<T>::Load(const char* filename,const int height,
					  const int width) {
    if(!FileExists(filename)) return false;
    if(height<=0 || width<=0) return false;
    Create(height,width);
    if(p==NULL) return false;
    std::ifstream in(filename);
    if(!in.good()) return false;
    for(int i=0;i<height;i++)
      {
	for(int j=0;j<width;j++) in>>p[i][j];
	if(!in.good()) return false;
      }
    in.close();
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> bool Array2d<T>::Save(const char* filename) {
    std::ofstream out(filename);
    for(int i=0;i<nrow;i++)
      {
	for(int j=0;j<ncol;j++) out<<p[i][j]<<" ";
	out<<std::endl;
	if(!out.good()) return false;
      }
    out.close();
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::Print(std::ostream& of) const
  {
    of<<std::endl<<"-----------------------------------"<<std::endl;
    for(int i=0;i<nrow;i++)
      {
	for(int j=0;j<ncol;j++) of<<p[i][j]<<" ";
	of<<std::endl;
      }
    of<<"-----------------------------------"<<std::endl;
    of<<"\tHeight="<<nrow<<", Width="<<ncol<<std::endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::RowSum(Array2d<T>& rowsum) {
    rowsum.Create(1,nrow);
    for(int i=0;i<nrow;i++)
      {
	T sum = 0;
	for(int j=0;j<ncol;j++) sum += p[i][j];
	rowsum.p[0][i] = sum;
      }
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::RowAverage(Array2d<T>& rowavg) 
  // Notice that when T is an integer type, RowAverage will return an integer value
  {
    RowSum(rowavg);
    REAL t = 1.0/ncol;
    for(int i=0;i<nrow;i++) rowavg.p[0][i] *= t;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2d<T>::Multiply(const Array2d<T>& B,Array2d<T>& result)
  {
    assert(ncol==B.nrow);
    result.Create(nrow,B.ncol);
    T** bp = B.p;
    T** rp = result.p;
    for(int i=0;i<nrow;i++)
      for(int j=0;j<B.ncol;j++)
	{
	  REAL sum = 0;
	  for(int k=0;k<ncol;k++) sum += (p[i][k]*bp[k][j]);
	  rp[i][j] = sum;
	}
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2dC<T>::Create(const int _nrow,const int _ncol) {
    assert(_nrow>0 && _ncol>0);
    if(nrow==_nrow && ncol==_ncol) return;
    Clear();
    nrow = _nrow; ncol = _ncol;
    buf = new T[nrow*ncol]; assert(buf!=NULL);
    p = new T*[nrow]; assert(p!=NULL);
    for(int i=0;i<nrow;i++) p[i] = buf + i * ncol;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2dC<T>::Swap(Array2dC<T>& array2) {
    std::swap(nrow,array2.nrow);
    std::swap(ncol,array2.ncol);
    std::swap(p,array2.p);
    std::swap(buf,array2.buf);
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2dC<T>::Clear() {
    delete[] buf; buf = NULL;
    delete[] p; p = NULL;
    nrow = ncol = 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> bool Array2dC<T>::Load(const char* filename,const int height,
					   const int width) {
    if(!FileExists(filename)) return false;
    if(height<=0 || width<=0) return false;
    Create(height,width);
    if(p==NULL) return false;
    std::ifstream in(filename);
    if(!in.good()) return false;
    for(int i=0;i<height;i++)
      {
	for(int j=0;j<width;j++) in>>p[i][j];
	if(!in.good()) return false;
      }
    in.close();
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> bool Array2dC<T>::Save(const char* filename) {
    std::ofstream out(filename);
    for(int i=0;i<nrow;i++)
      {
	for(int j=0;j<ncol;j++) out<<p[i][j];
	if(!out.good()) return false;
      }
    out.close();
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2dC<T>::Print(std::ostream& of) const
  {
    of<<std::endl<<"-----------------------------------"<<std::endl;
    for(int i=0;i<nrow;i++)
      {
	for(int j=0;j<ncol;j++) of<<p[i][j]<<" ";
	of<<std::endl;
      }
    of<<"-----------------------------------"<<std::endl;
    of<<"\tHeight="<<nrow<<", Width="<<ncol<<std::endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2dC<T>::RowSum(Array2dC<T>& rowsum) {
    rowsum.Create(1,nrow);
    for(int i=0;i<nrow;i++)
      {
	T sum = 0;
	for(int j=0;j<ncol;j++) sum += p[i][j];
	rowsum.p[0][i] = sum;
      }
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2dC<T>::RowAverage(Array2dC<T>& rowavg) 
  // Notice that when T is an integer type, RowAverage will return an integer value
  {
    RowSum(rowavg);
    REAL t = 1.0/ncol;
    for(int i=0;i<nrow;i++) rowavg.p[0][i] *= t;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void Array2dC<T>::Multiply(const Array2dC<T>& B,Array2dC<T>& result)
  {
    assert(ncol==B.nrow);
    result.Create(nrow,B.ncol);
    T** bp = B.p;
    T** rp = result.p;
    for(int i=0;i<nrow;i++)
      for(int j=0;j<B.ncol;j++)
	{
	  REAL sum = 0;
	  for(int k=0;k<ncol;k++) sum += (p[i][k]*bp[k][j]);
	  rp[i][j] = sum;
	}
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template<class T> void IntImage<T>::Clear(void) {
    Array2dC<T>::Clear();
    variance = 0.0;
    label = -1;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  bool IntImage<T>::Load(const std::string& filename, size_t crop) {
    IplImage* img;
    img = cvLoadImage(filename.c_str());

    if(img==NULL) return false;

    if (crop) {
      IplImage* imgtemp;
      imgtemp = cvCreateImage(cvSize(img->width-2*crop,img->height-2*crop),
			      img->depth, img->nChannels);
      cvSetImageROI(img, cvRect(crop, crop, img->width-2*crop, img->height-2*crop));
      cvCopy(img, imgtemp);
      cvResetImageROI(img);
      cvReleaseImage(&img);
      img = imgtemp;
      imgtemp = NULL;
    }

    if(img->nChannels>1) {
      IplImage* imgtemp;
      imgtemp = cvCreateImage(cvSize(img->width,img->height),IPL_DEPTH_8U,1);
      cvCvtColor(img,imgtemp,CV_BGR2GRAY);
      cvReleaseImage(&img);
      img = imgtemp;
      imgtemp = NULL;
    }

    //cvSaveImage("foo.bmp", img);

    SetSize(img->height,img->width);
    for(int i=0,ih=img->height,iw=img->width;i<ih;i++) {
      T* pdata = p[i];
      unsigned char* pimg = reinterpret_cast<unsigned char*>(img->imageData+img->widthStep*i);
      for(int j=0;j<iw;j++) pdata[j] = pimg[j];
    }
    cvReleaseImage(&img);
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> bool IntImage<T>::FromMat(IplImage *input, const size_t crop) {
    return FromMat(input, crop, crop);
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> bool IntImage<T>::FromMat(IplImage *input, const size_t crop_x, 
					      const size_t crop_y) {
    crop_t crop;
    crop.x0 = crop.x1 = crop_x;
    crop.y0 = crop.y1 = crop_y;
    return FromMat(input, crop, "");
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> bool IntImage<T>::FromMat(IplImage *input, const crop_t &crop, 
					      string outimgfname) {
    IplImage *img;
    img = cvCreateImage(cvGetSize(input), input->depth, input->nChannels);
    cvCopy(input, img);

    if (img==NULL) {
      cout << "img is NULL" << endl;
      return false;
    }

    if (crop.x0 || crop.x1 || crop.y0 || crop.y1) {
      IplImage* imgtemp;
      imgtemp = cvCreateImage(cvSize(img->width - crop.x0 - crop.x1,
				     img->height - crop.y0 - crop.y1),
			      img->depth, img->nChannels);
      cvSetImageROI(img, cvRect(crop.x0, crop.y0, imgtemp->width, imgtemp->height));
      cvCopy(img, imgtemp);
      cvResetImageROI(img);
      cvReleaseImage(&img);
      img = imgtemp;
      imgtemp = NULL;
    }

    if (img->nChannels>1) {
      IplImage* imgtemp;    
      imgtemp = cvCreateImage(cvSize(img->width,img->height),IPL_DEPTH_8U,1);
      cvCvtColor(img,imgtemp,CV_BGR2GRAY);
      cvReleaseImage(&img);
      img = imgtemp;
      imgtemp = NULL;
    }

    if (!outimgfname.empty()) {
      string ofn = "out-"+outimgfname+".jpg";
      cvSaveImage(ofn.c_str(), img);
    }

    SetSize(img->height,img->width);
    for(int i=0,ih=img->height,iw=img->width;i<ih;i++) {
      T* pdata = p[i];
      unsigned char* pimg = reinterpret_cast<unsigned char*>(img->imageData+img->widthStep*i);
      for(int j=0;j<iw;j++) pdata[j] = pimg[j];
    }
    cvReleaseImage(&img);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  void IntImage<T>::Save(const std::string& filename) const {
    IplImage* img;
    
    img = cvCreateImage(cvSize(ncol,nrow),IPL_DEPTH_8U,1);
    for(int i=0,ih=img->height,iw=img->width;i<ih;i++)
    {
        T* pdata = p[i];
        unsigned char* pimg = reinterpret_cast<unsigned char*>(img->imageData+img->widthStep*i);
        for(int j=0;j<iw;j++) pimg[j] = (unsigned char)pdata[j];
    }
    cvSaveImage(filename.c_str(),img);
    cvReleaseImage(&img);
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  void IntImage<T>::SetSize(const int h,const int w)
  // 'size' is the new size of the image, if necessary, memory is reallocated
  // size.cx is the new height and size.cy is the new width
  {
    if((h == nrow) && (w == ncol)) return;
    
    Clear();
    Array2dC<T>::Create(h,w);
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  IntImage<T>& IntImage<T>::operator=(const IntImage<T>& source) {
    if(&source==this) return *this;
    SetSize(source.nrow,source.ncol);
    memcpy(buf,source.buf,sizeof(T)*nrow*ncol);
    label = source.label;
    variance = source.variance;
    
    return *this;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  void IntImage<T>::Print(std::ostream& of) const {
    Array2dC<T>::Print();
    of<<"\tvariance="<<variance<<",    label="<<label<<std::endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  void IntImage<T>::Resize(IntImage<T> &result,const REAL ratio) const {
    Resize(result,int(nrow*ratio),int(ncol*ratio));
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  void IntImage<T>::Resize(IntImage<T>& result,const int height,const int width) const
  {
    assert(height>0 && width>0);
    result.SetSize(height,width);
    REAL ixratio = nrow*1.0/height, iyratio = ncol*1.0/width;

    REAL* p_y = new REAL[result.ncol]; assert(p_y!=NULL);
    int* p_y0 = new int[result.ncol]; assert(p_y0!=NULL);
    for(int i=0;i<width;i++)
    {
        p_y[i] = i*iyratio;
        p_y0[i] = (int)p_y[i];
        if(p_y0[i]==ncol-1) p_y0[i]--;
        p_y[i] -= p_y0[i];
    }

    for(int i=0;i<height;i++)
    {
        int x0; REAL x;
        x = i*ixratio;
        x0 = (int)x;
        if(x0==nrow-1) x0--;
        x -= x0;
        for(int j=0;j<width;j++)
        {
            int y0=p_y0[j];
            REAL y=p_y[j],fx0,fx1;

            fx0 = REAL(p[x0][y0] + y*(p[x0][y0+1]-p[x0][y0]));
            fx1 = REAL(p[x0+1][y0] + y*(p[x0+1][y0+1]-p[x0+1][y0]));

            result.p[i][j] = T(fx0 + x*(fx1-fx0));
        }
    }

    delete[] p_y; p_y=NULL;
    delete[] p_y0; p_y0=NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void IntImage<T>::Swap(IntImage<T>& image2) {
    Array2dC<T>::Swap(image2);
    std::swap(variance,image2.variance);
    std::swap(label,image2.label);
}

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void IntImage<T>::operator-=(const IntImage<T>& img2) {
    assert(nrow==img2.nrow && ncol==img2.ncol);
    for(int i=0,size=nrow*ncol;i<size;i++) p[i] -= img2.p[i];
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void IntImage<T>::AbsoluteValue(void) {
    for(int i=0,size=nrow*ncol;i<size;i++)  if(buf[i]<0) buf[i] = -buf[i];
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T> void IntImage<T>::Thresh(const REAL thresh) {
    for(int i=0,size=nrow*ncol;i<size;i++) buf[i]=(buf[i]>thresh)?REAL(1.0):0;
  }

  /////////////////////////////////////////////////////////////////////////////

  template<class T>
  void IntImage<T>::Sobel(IntImage<REAL>& result,const bool useSqrt,const bool normalize)
  {// compute the Sobel gradient. For now, we just use the very inefficient way. Optimization can be done later
    // if useSqrt = true, we compute the real Sobel gradient; otherwise, the square of it
    // if normalize = true, the numbers are normalized to be in 0..255
    result.Create(nrow,ncol);
    for(int i=0;i<nrow;i++) result.p[i][0] = result.p[i][ncol-1] = 0;
    for(int i=0;i<ncol;i++) result.p[0][i] = result.p[nrow-1][i] = 0;
    for(int i=1;i<nrow-1;i++)
    {
        for(int j=1;j<ncol-1;j++)
        {
            REAL gx =     p[i-1][j-1] - p[i-1][j+1]
                     + 2*(p[i][j-1]   - p[i][j+1])
                     +    p[i+1][j-1] - p[i+1][j+1];
            REAL gy =     p[i-1][j-1] - p[i+1][j-1]
                     + 2*(p[i-1][j]   - p[i+1][j])
                     +    p[i-1][j+1] - p[i+1][j+1];
           if(useSqrt || normalize ) // if we want to normalize the result image, we'd better use the true Sobel gradient
                result.p[i][j] = sqrt(gx*gx+gy*gy);
           else
                result.p[i][j] = gx*gx+gy*gy;
        }
    }
    if(normalize)
    {
        REAL minv = 1e20, maxv = -minv;
        for(int i=1;i<nrow-1;i++)
        {
            for(int j=1;j<ncol-1;j++)
            {
                if(result.p[i][j]<minv)
                    minv = result.p[i][j];
                else if(result.p[i][j]>maxv)
                    maxv = result.p[i][j];
            }
        }
        for(int i=0;i<nrow;i++) result.p[i][0] = result.p[i][ncol-1] = minv;
        for(int i=0;i<ncol;i++) result.p[0][i] = result.p[nrow-1][i] = minv;
        REAL s = 255.0/(maxv-minv);
        for(int i=0;i<nrow*ncol;i++) result.buf[i] = (result.buf[i]-minv)*s;
    }
  }
   
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Funnel::Funnel() {
    model_loaded = false;
    numFeatureClusters = edgeDescDim = numRandPxls = -1;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  const int Funnel::NUM_PARAMS = 4;
  const int Funnel::WINDOW_SIZE = 4;
  const int Funnel::OUTERDIMW = 150; 
  const int Funnel::OUTERDIMH = 150;
  const int Funnel::INNERDIMW = 100;
  const int Funnel::INNERDIMH = 100;
  const int Funnel::SIFTHISTDIM = 4;
  const int Funnel::SIFTBUCKETSDIM = 8;

  /////////////////////////////////////////////////////////////////////////////
  
  bool Funnel::LoadModel(const string &modelfile) {
    ifstream mfstream(modelfile.c_str());
    if (!mfstream)
      return false;

    mfstream >> numFeatureClusters >> edgeDescDim;
    vector<float> cRow(edgeDescDim, 0);
    for(int i=0; i<numFeatureClusters; i++)
      centroids.push_back(cRow);
    sigmaSq.reserve(numFeatureClusters);

    for(int i=0; i<numFeatureClusters; i++) {
      for(int j=0; j<edgeDescDim; j++) {
	mfstream >> centroids[i][j];
      }
      mfstream >> sigmaSq[i];
    }

    mfstream >> numRandPxls;
    randPxls.reserve(numRandPxls);
    for(int j=0; j<numRandPxls; j++) {
      mfstream >> randPxls[j].first >> randPxls[j].second;
    }

    vector<float> dfCol(numFeatureClusters, 0);
    for(int i=0; i<numRandPxls; i++)
      logDistField.push_back(dfCol);

    int iteration;
    while(true) {
      mfstream >> iteration;
      if(mfstream.eof())
	break;
      
      for(int j=0; j<numRandPxls; j++) {
	for(int i=0; i<numFeatureClusters; i++) {
	  mfstream >> logDistField[j][i];
	}
      }
      logDFSeq.push_back(logDistField);
    }

    computeGaussian(Gaussian, WINDOW_SIZE);

    model_loaded = true;
    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  bool Funnel::Process(IplImage **image) {

    IplImage *originalImage;
    IplImage *baseImage;
    
    //IplImage *tmp = &IplImage(image);
    IplImage *tmp = *image;
    if(tmp == 0) {
      cerr << "Funnel::Process(): problem with input image." << endl;
      return false;
    }
	
    // resize to outerDim
    IplImage *tmp2 = cvCreateImage(cvSize(OUTERDIMW, OUTERDIMH), 
				   tmp->depth, tmp->nChannels);
    cvResize(tmp, tmp2);
    // change to floating point
    IplImage *tmp3 = cvCreateImage(cvSize(OUTERDIMW, OUTERDIMH), 
				   IPL_DEPTH_32F, tmp->nChannels);
    cvConvertScale(tmp2, tmp3);
    // convert to grayscale
    if(tmp3->nChannels > 1) {
      IplImage *tmp4 = cvCreateImage(cvSize(OUTERDIMW, OUTERDIMH), 
				     IPL_DEPTH_32F, 1);
      cvCvtColor(tmp3, tmp4, CV_BGR2GRAY);
      IplImage *tmp5 = tmp3;
      tmp3 = tmp4;
      cvReleaseImage(&tmp5);
    }
    
    originalImage = tmp;
    baseImage = tmp3;	
    cvReleaseImage(&tmp2);

    vector<float> vrow(NUM_PARAMS);
    vector<vector<float> > v(1, vrow);
    
    const int height = baseImage->height-2*WINDOW_SIZE;
    const int width = baseImage->width-2*WINDOW_SIZE;
    const int baseWidthStep = baseImage->widthStep / sizeof(float);
    
    vector<float> ofEntry(edgeDescDim, 0);
    vector<vector<float> > ofCol(width, ofEntry);
    vector<vector<vector<float> > > ofRow(height, ofCol);
    vector<vector<vector<vector<float> > > > originalFeatures(1, ofRow);
    vector<float> SiftDesc(edgeDescDim); 
    
    vector<float> mtRow(width+2*WINDOW_SIZE);
    vector<vector<float> > m(height+2*WINDOW_SIZE, mtRow);
    vector<vector<float> > theta(height+2*WINDOW_SIZE, mtRow);
    float dx, dy;
    
    for(int j=0; j<height+2*WINDOW_SIZE; j++) {
      for(int k=0; k<width+2*WINDOW_SIZE; k++) {

	if (j==0) {
	  dy = ((float*)baseImage->imageData)[(j+1)*baseWidthStep+k] - 
	    ((float*)baseImage->imageData)[j*baseWidthStep+k];
	} else {
	  if (j==height+2*WINDOW_SIZE-1)
	    dy = ((float*)baseImage->imageData)[j*baseWidthStep+k] - 
	      ((float*)baseImage->imageData)[(j-1)*baseWidthStep+k];
	  else
	    dy = ((float*)baseImage->imageData)[(j+1)*baseWidthStep+k] - 
	      ((float*)baseImage->imageData)[(j-1)*baseWidthStep+k];
	}

	if (k==0) {
	  dx = ((float*)baseImage->imageData)[j*baseWidthStep+(k+1)] - 
	    ((float*)baseImage->imageData)[j*baseWidthStep+k];
	} else {
	  if (k==width+2*WINDOW_SIZE-1)
	    dx = ((float*)baseImage->imageData)[j*baseWidthStep+k] - 
	      ((float*)baseImage->imageData)[j*baseWidthStep+(k-1)];
	  else
	    dx = ((float*)baseImage->imageData)[j*baseWidthStep+(k+1)] - 
	      ((float*)baseImage->imageData)[j*baseWidthStep+(k-1)];
	}
	
	m[j][k] = (float)sqrt(dx*dx+dy*dy);
	theta[j][k] = (float)atan2(dy,dx) * 180.0f/CV_PI;
	if(theta[j][k] < 0)
	  theta[j][k] += 360.0f;
      }
    }
    
    for(int j=0; j<height; j++) {
      for(int k=0; k<width; k++) {
	getSIFTdescripter(SiftDesc, m, theta, j+WINDOW_SIZE, k+WINDOW_SIZE, 
			  WINDOW_SIZE, SIFTHISTDIM, SIFTBUCKETSDIM, 
			  Gaussian);
	originalFeatures[0][j][k] = SiftDesc;
      }
    }  
    
    for(int j=0; j<height; j++) {
      for(int k=0; k<width; k++) {
	vector<float> distances(numFeatureClusters);
	float sum = 0;
	for(int ii=0; ii<numFeatureClusters; ii++) {
	  distances[ii] = 
	    exp(-dist(originalFeatures[0][j][k], 
		      centroids[ii])/(2*sigmaSq[ii]))/sqrt(sigmaSq[ii]);
	  sum += distances[ii];
	}
	for(int ii=0; ii<numFeatureClusters; ii++)
	  distances[ii] /= sum;
	originalFeatures[0][j][k] = distances;
      }
    }
    
    cvReleaseImage(&baseImage);

    vector<float> fidsEntry(numFeatureClusters, 0);
    vector<vector<float> > fidsRow(numRandPxls, fidsEntry);
    vector<vector<vector<float> > > featureIDs(1, fidsRow);
    
    vector<float> nfEntry(numFeatureClusters, 0);
    vector<vector<float> > newFIDs(numRandPxls, nfEntry);
    float centerX = width/2.0f, centerY = height/2.0f;
    
    float d[NUM_PARAMS] = {1.0f, 1.0f, CV_PI/180.0f, 0.02f};
    
    getNewFeatsInvT(featureIDs[0], originalFeatures[0], v[0], 
		    centerX, centerY, randPxls);
    
    for(int iter=0; iter<(int)logDFSeq.size(); iter++) {
      float oldL = computeLogLikelihood(logDFSeq[iter], featureIDs[0], 
					numFeatureClusters);
      for(int k=0; k<NUM_PARAMS; k++) {
	float dn = ((rand()%160)-80)/100.0f;
	if(k>1)
	  dn /= 100.0f;
	v[0][k] += (d[k] + dn);
	
	getNewFeatsInvT(newFIDs, originalFeatures[0], v[0], 
			centerX, centerY, randPxls);
	float newL = computeLogLikelihood(logDFSeq[iter], newFIDs, 
					  numFeatureClusters);
	
	if(newL > oldL) {
	  featureIDs[0] = newFIDs;
	  oldL = newL;

	 } else {
	  v[0][k] -= (2*(d[k] + dn));
	  getNewFeatsInvT(newFIDs, originalFeatures[0], v[0], 
			  centerX, centerY, randPxls);
	  newL = computeLogLikelihood(logDFSeq[iter], newFIDs, 
				      numFeatureClusters);
	  
	  if(newL > oldL) {
	    oldL = newL;
	    featureIDs[0] = newFIDs;

	  } else {
	    v[0][k] += (d[k]+dn);
	  }
	}
      }
    }
    
    float cropT1inv[2][3] = {{1,0,originalImage->width/2.0f}, 
			     {0,1,originalImage->height/2.0f}};
    float cropS1inv[3][3] = {{originalImage->width/(float)OUTERDIMW,0,0}, 
			     {0,originalImage->height/(float)OUTERDIMH,0}, 
			     {0,0,1}};		    
    float cropS2inv[3][3] = {{OUTERDIMW/(float)originalImage->width,0,0}, 
			     {0,OUTERDIMH/(float)originalImage->height,0}, 
			     {0,0,1}};
    float cropT2inv[3][3] = {{1,0,-originalImage->width/2.0f}, 
			     {0,1,-originalImage->height/2.0f}, {0,0,1}};
    
    float postM[3][3] = {{1,0,OUTERDIMW/2.0f}, {0,1,OUTERDIMH/2.0f}, {0,0,1}};
    float preM[3][3] = {{1,0,-OUTERDIMW/2.0f}, {0,1,-OUTERDIMH/2.0f}, {0,0,1}};
    
    float tM[3][3]  = {{1, 0, v[0][0]}, {0, 1, v[0][1]}, {0,0,1}};
    float rM[3][3]  = {{cos(v[0][2]), -sin(v[0][2]), 0}, 
		       {sin(v[0][2]), cos(v[0][2]), 0}, {0, 0, 1}};
    float sM[3][3]  = {{exp(v[0][3]), 0, 0}, {0, exp(v[0][3]), 0}, {0, 0, 1}};
    
    CvMat tCVM, rCVM, sCVM, *xform, postCVM, preCVM;
    CvMat cropT1invCVM, cropS1invCVM, cropS2invCVM, cropT2invCVM;
    
    tCVM  = cvMat(3, 3, CV_32FC1, tM);
    rCVM  = cvMat(3, 3, CV_32FC1, rM);
    sCVM  = cvMat(3, 3, CV_32FC1, sM);
    
    postCVM = cvMat(3, 3, CV_32FC1, postM);
    preCVM = cvMat(3, 3, CV_32FC1, preM);
    
    cropT1invCVM = cvMat(2,3,CV_32FC1, cropT1inv);
    cropS1invCVM = cvMat(3,3,CV_32FC1, cropS1inv);
    cropS2invCVM = cvMat(3,3,CV_32FC1, cropS2inv);
    cropT2invCVM = cvMat(3,3,CV_32FC1, cropT2inv);
    
    xform = cvCreateMat(2, 3, CV_32FC1);
    cvMatMul(&cropT1invCVM, &cropS1invCVM, xform);
    cvMatMul(xform, &tCVM, xform);
    cvMatMul(xform, &rCVM, xform);
    cvMatMul(xform, &sCVM, xform);
    cvMatMul(xform, &cropS2invCVM, xform);
    cvMatMul(xform, &cropT2invCVM, xform);     
      
    IplImage* dst = cvCreateImage(cvSize(originalImage->width,
					 originalImage->height), 
				  originalImage->depth, 3);
    cvWarpAffine(originalImage, dst, xform, 
		 CV_WARP_INVERSE_MAP + CV_WARP_FILL_OUTLIERS);
      
    //string outimg = "output.bmp";
    //cout << "Saving image " << outimg << endl;
    //cvSaveImage(outimg.c_str(), dst);
    
    cvReleaseMat(&xform);      
    *image = dst;
    //cvReleaseImage(&dst);

    //cvReleaseImage(&originalImage);
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Funnel::computeGaussian(vector<vector<float> > &Gaussian, int windowSize) {
    for (int i=0; i<2*windowSize; i++) {
      vector<float> grow(2*windowSize);
      for (int j=0; j<2*windowSize; j++) {
	float ii = i-(windowSize-0.5f), jj = j-(windowSize-0.5f);
	grow[j] = exp(-(ii*ii+jj*jj)/(2*windowSize*windowSize));
      }
      Gaussian.push_back(grow);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void Funnel::getSIFTdescripter(vector<float> &descripter, vector<vector<float> > &m, 
				 vector<vector<float> > &theta, int x, int y, 
				 int windowSize, int histDim, int bucketsDim, 
				 vector<vector<float> > &Gaussian) {
    for(int i=0; i<(signed)descripter.size(); i++)
      descripter[i]=0;
    
    int histDimWidth = 2*windowSize/histDim;
    float degPerBin = 360.0f/bucketsDim;
    
    // weight magnitudes by Gaussian with sigma equal to half window
    vector<float> mtimesGRow(2*windowSize);
    vector<vector<float> > mtimesG(2*windowSize, mtimesGRow);
    for(int i=0; i<2*windowSize; i++)
      {
	for(int j=0; j<2*windowSize; j++)
	  {
	    int xx = x+i-(windowSize-1), yy = y+j-(windowSize-1);
	    mtimesG[i][j] = m[xx][yy] * Gaussian[i][j];
	  }
      }
    
    // calculate descripter
    // using trilinear interpolation
    int histBin[2], histX[2], histY[2];
    float dX[2], dY[2], dBin[2];
    for(int i=0; i<2*windowSize; i++)
      {
	for(int j=0; j<2*windowSize; j++)
	  {
	    histX[0] = i/histDim; histX[1] = i/histDim;
	    histY[0] = j/histDim; histY[1] = j/histDim;
	    dX[1] = 0;
	    dY[1] = 0;
	    
	    int iModHD = i % histDim;
	    int jModHD = j % histDim;
	    int histDimD2 = histDim/2;
	    
	    if( iModHD >= histDimD2 && i < 2*windowSize - histDimD2 )
	      {
		histX[1] = histX[0] + 1;
		dX[1] = (iModHD + 0.5f - histDimD2) / histDim;
	      }
	    if( iModHD < histDimD2 && i >= histDimD2 )
	      {
		histX[1] = histX[0] - 1;
		dX[1] = (histDimD2 + 0.5f - iModHD) / histDim;
	      }
	    if( jModHD >= histDimD2 && j < 2*windowSize - histDimD2 )
	      {
		histY[1] = histY[0] + 1;
		dY[1] = (jModHD + 0.5f - histDimD2) / histDim;
	      }
	    if( jModHD < histDimD2 && j >= histDimD2)
	      {
		histY[1] = histY[0] - 1;
		dY[1] = (histDimD2 + 0.5f - jModHD) / histDim;
	      }
	    
	    dX[0] = 1.0f - dX[1];
	    dY[0] = 1.0f - dY[1];
	    
	    float histAngle = theta[x+i-(windowSize-1)][y+j-(windowSize-1)];
	    
	    histBin[0] = (int)(histAngle / degPerBin);
	    histBin[1] = (histBin[0]+1) % bucketsDim;
	    dBin[1] = (histAngle - histBin[0]*degPerBin) / degPerBin;
	    dBin[0] = 1.0f-dBin[1];
	    
	    for(int histBinIndex=0; histBinIndex<2; histBinIndex++)
	      {
		for(int histXIndex=0; histXIndex<2; histXIndex++)
		  {
		    for(int histYIndex=0; histYIndex<2; histYIndex++)
		      {
			int histNum = histX[histXIndex]*histDimWidth + histY[histYIndex];
			int bin = histBin[histBinIndex];
			descripter[histNum*bucketsDim + bin] += (mtimesG[i][j] * dX[histXIndex] * dY[histYIndex] * dBin[histBinIndex]);
		      }
		  }
	      }
	  }
      }
    
    // normalize
    // threshold values at .2, renormalize
    float sum = 0;
    for(int i=0; i<(signed)descripter.size(); i++)
      sum += descripter[i];
    
    if(sum < .0000001f)
      {
	//float dn = 1.0f / (signed)descripter.size();
	for(int i=0; i<(signed)descripter.size(); i++)
	  descripter[i] = 0;
	return;
      }
    
    for(int i=0; i<(signed)descripter.size(); i++)
      {
	descripter[i] /= sum;
	if(descripter[i] > .2f)
	  descripter[i] = .2f;
      }
    sum = 0;
    for(int i=0; i<(signed)descripter.size(); i++)
      sum += descripter[i];
    for(int i=0; i<(signed)descripter.size(); i++)
      descripter[i] /= sum;
  }

  /////////////////////////////////////////////////////////////////////////////

  float Funnel::dist(vector<float> &a, vector<float> &b) {
    float r=0;
    for(int i=0; i<(signed)a.size(); i++)
      r+=(a[i]-b[i])*(a[i]-b[i]);
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Funnel::getNewFeatsInvT(vector<vector<float> > &newFIDs, 
			       vector<vector<vector<float> > > &originalFeats, 
			       vector<float> &vparams, float centerX, float centerY, 
			       vector<pair<int, int> > &randPxls) {
  int numFeats = newFIDs[0].size();
  vector<float> uniformDist(numFeats, 1.0f/numFeats);
  
  float postM[2][3] = {{1,0,centerX}, {0,1,centerY}};
  float preM[3][3] = {{1,0,-centerX}, {0,1,-centerY}, {0,0,1}};

  float tM[3][3]  = {{1, 0, vparams[0]}, {0, 1, vparams[1]}, {0,0,1}};
  float rM[3][3]  = {{cos(vparams[2]), -sin(vparams[2]), 0}, {sin(vparams[2]), cos(vparams[2]), 0}, {0, 0, 1}};
  float sM[3][3]  = {{exp(vparams[3]), 0, 0}, {0, exp(vparams[3]), 0}, {0, 0, 1}};
  
  CvMat tCVM, rCVM, sCVM, *xform, postCVM, preCVM;
  //CvMat tCVM, rCVM, sCVM, hxCVM, hyCVM, *xform, postCVM, preCVM;
  tCVM  = cvMat(3, 3, CV_32FC1, tM);
  rCVM  = cvMat(3, 3, CV_32FC1, rM);
  sCVM  = cvMat(3, 3, CV_32FC1, sM);

  postCVM = cvMat(2, 3, CV_32FC1, postM);
  preCVM = cvMat(3, 3, CV_32FC1, preM);
  
  xform = cvCreateMat(2, 3, CV_32FC1);
  cvMatMul(&postCVM, &tCVM, xform);
  cvMatMul(xform, &rCVM, xform);
  cvMatMul(xform, &sCVM, xform);
  cvMatMul(xform, &preCVM, xform);

  int height = (signed)originalFeats.size();
  int width  = (signed)originalFeats[0].size();
  
  for(int i=0; i<(signed)newFIDs.size(); i++)
    {
      int j = randPxls[i].first, k = randPxls[i].second;
      int nx = (int)(xform->data.fl[0]*k + xform->data.fl[1]*j + xform->data.fl[2] + 0.5f);
      int ny = (int)(xform->data.fl[3]*k + xform->data.fl[4]*j + xform->data.fl[5] + 0.5f);
      if(!(ny >= 0 && ny < height && nx >= 0 && nx < width))
	newFIDs[i] = uniformDist;
      else
	newFIDs[i] = originalFeats[ny][nx];
    }
  
  cvReleaseMat(&xform);
  }

  /////////////////////////////////////////////////////////////////////////////

  float Funnel::computeLogLikelihood(vector<vector<float> > &logDistField, 
				     vector<vector<float> > &fids, 
				     int numFeatureClusters) {
    float l = 0;
    for (int j=0; j<(signed)fids.size(); j++) {
	for (int i=0; i<numFeatureClusters; i++)
	  l += fids[j][i] * logDistField[j][i];
    }
    return l;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  const double IlluminationNormalization::GAMMA = 0.2;
  const double IlluminationNormalization::SIGMA0 = 1.0;
  const double IlluminationNormalization::SIGMA1 = 2.0;
  const double IlluminationNormalization::DO_NORM = 10.0;

  /////////////////////////////////////////////////////////////////////////////

  bool IlluminationNormalization::Process(IplImage **image, const crop_t &crop) {

    IplImage *tmp = *image;
    if(tmp == 0) {
      cerr << "IlluminationNormalization::Process(): problem with input image." 
	   << endl;
      return false;
    }

    IplImage* grayimg;    
    grayimg = cvCreateImage(cvSize(tmp->width - crop.x0 - crop.x1,
				   tmp->height - crop.y0 - crop.y1)
			    ,IPL_DEPTH_8U,1);
    cvSetImageROI(tmp, cvRect(crop.x0, crop.y0, grayimg->width, grayimg->height));
    cvCvtColor(tmp,grayimg,CV_BGR2GRAY);
    cvResetImageROI(tmp);

    IplImage* floatimg = cvCreateImage(cvGetSize(grayimg), IPL_DEPTH_32F, 1);  
    cvConvertScale(grayimg, floatimg);
    cvReleaseImage(&grayimg);

    for (int y=0; y<floatimg->height; y++) {
      uchar* ptr = (uchar*) (floatimg->imageData + y*floatimg->widthStep);
      for (int x=0; x<floatimg->width; x++) {
     	float *fp = (float*) &ptr[4*x];	  
     	*fp = pow(double(*fp), GAMMA);
      }
    }

    IplImage* gaussimg = cvCreateImage(cvGetSize(floatimg), IPL_DEPTH_32F, 1);  
    cvSmooth(floatimg, gaussimg, CV_GAUSSIAN, 7, 7, SIGMA1, SIGMA1);
    cvSmooth(floatimg, floatimg, CV_GAUSSIAN, 7, 7, SIGMA0, SIGMA0);
    cvAddWeighted(floatimg, 1.0, gaussimg, -1.0, 0.0, floatimg);
    cvReleaseImage(&gaussimg);    

    double a = 0.1, meanabs = 0.0, meanminabs = 0.0;
    for (int y=0; y<floatimg->height; y++) {
      uchar* ptr = (uchar*) (floatimg->imageData + y*floatimg->widthStep);
      for (int x=0; x<floatimg->width; x++) {
     	float *fp = (float*) &ptr[4*x];	  
	meanabs += pow(double(abs(*fp)), a);
      }
    }
    meanabs /= (floatimg->width * floatimg->height);

    for (int y=0; y<floatimg->height; y++) {
      uchar* ptr = (uchar*) (floatimg->imageData + y*floatimg->widthStep);
      for (int x=0; x<floatimg->width; x++) {
     	float *fp = (float*) &ptr[4*x];	  
	*fp /= pow(meanabs, 1/a);
      }
    }

    for (int y=0; y<floatimg->height; y++) {
      uchar* ptr = (uchar*) (floatimg->imageData + y*floatimg->widthStep);
      for (int x=0; x<floatimg->width; x++) {
     	float *fp = (float*) &ptr[4*x];	  
	meanminabs += pow(MIN(DO_NORM, double(abs(*fp))), a);
      }
    }
    meanminabs /= (floatimg->width * floatimg->height);
    //cout << "meanabs=" << meanabs << ", meanminabs=" << meanminabs << endl;
    
    for (int y=0; y<floatimg->height; y++) {
      uchar* ptr = (uchar*) (floatimg->imageData + y*floatimg->widthStep);
      for (int x=0; x<floatimg->width; x++) {
     	float *fp = (float*) &ptr[4*x];	  
	*fp /= pow(meanminabs, 1/a);
	*fp = DO_NORM*tanh(double(*fp)/DO_NORM);
      }
    }

    float maxp = -1000000.0, minp = 1000000.0;
    for (int y=0; y<floatimg->height; y++) {
      uchar* ptr = (uchar*) (floatimg->imageData + y*floatimg->widthStep);
      for (int x=0; x<floatimg->width; x++) {
	float *fp = (float*) &ptr[4*x];	  
	if (*fp > maxp)
	  maxp = *fp;
	else if(*fp < minp)
	  minp = *fp;
      }
    }
    //cout << "maxp=" << maxp << ", minp=" << minp << ", tanh(1)=" << tanh(1) << endl;
    
    cvConvertScale(floatimg, floatimg, 1.0, -minp);
    cvConvertScale(floatimg, floatimg, 255.0/(maxp-minp));
    
    //string outimg = "illnorm.bmp";
    //cout << "Saving image " << outimg << endl;
    //cvSaveImage(outimg.c_str(), floatimg);

    IplImage* dstimg = cvCreateImage(cvGetSize(floatimg), IPL_DEPTH_8U, 1);  
    cvConvertScale(floatimg, dstimg);
    cvReleaseImage(&floatimg);

    *image = dstimg;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV


