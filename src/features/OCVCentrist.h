// -*- C++ -*-  $Id: OCVCentrist.h,v 1.16 2013/03/21 09:14:01 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file OCVCentrist.h

   \brief Declarations and definitions of class OCVCentrist and 
   
   OCVCentrist.h contains declarations and definitions of 
   class the OCVCentrist, which
   is a class that ...
  
   \author Markus Koskela <markus.koskela@aalto.fi>
   $Revision: 1.16 $
   $Date: 2013/03/21 09:14:01 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _OCVCentrist_
#define _OCVCentrist_

#include <OCVFeature.h>

#ifdef HAVE_OPENCV2_CORE_CORE_HPP

#include <opencv2/imgproc/imgproc_c.h>

#define USE_DOUBLE
#ifdef USE_DOUBLE
    typedef double REAL;
#else
    typedef float REAL;
#endif

#define CENTRIST_BASE_PATH "share/centrist/"
#define CENTRIST_PCA_NORMAL CENTRIST_BASE_PATH "lf_UIUC.txt"
#define CENTRIST_PCA_SOBEL  CENTRIST_BASE_PATH "lf_UIUC_Sobel.txt"
#define CENTRIST_PCA_OUTPUT CENTRIST_BASE_PATH "pcatrain.txt"

namespace picsom {

  struct crop_t {
    size_t x0;
    size_t x1;
    size_t y0;
    size_t y1;
    crop_t(): x0(0),x1(0),y0(0),y1(0) { }
    void clear() { x0 = x1 = y0 = y1 = 0; }
    void set(size_t val) { x0 = x1 = y0 = y1 = val; }
    void set(size_t xval, size_t yval) { x0 = x1 = xval; y0 = y1 = yval; }
    void set(size_t _x0, size_t _x1, size_t _y0, size_t _y1) { 
      x0 = _x0; x1 = _x1; y0 = _y0; y1 = _y1; 
    }
  };

  struct centrist_opts {
    crop_t crop;
    int levels;
    bool generate_pca;
    bool write_image;
    bool do_funneling;
    bool do_illnorm;
    bool cropfirst;
  };
  
// #include "mdarray.h"
  
  template<class T> class Array2d {
    
  private:
    void IncreaseCapacity(const int newrow);
    void DecreaseCapacity(const int newrow);

  public:
    int nrow;
    int ncol;
    T** p;
    
    Array2d():nrow(0),ncol(0),p(NULL) { }

    Array2d(const int nrow,const int ncol):nrow(0),ncol(0),p(NULL) { 
      Create(nrow,ncol); 
    }

    Array2d(const Array2d<T>& source):nrow(0),ncol(0),p(NULL) {
      if (source.p!=NULL) {
	Create(source.nrow,source.ncol);
	for (int i=0;i<nrow;i++) 
	  memcpy(p[i],source.p[i],sizeof(T)*ncol);
      }
    }

    Array2d<T>& operator=(const Array2d<T>& source) {
      if (source.p!=NULL) {
	Create(source.nrow,source.ncol);
	for (int i=0;i<nrow;i++) 
	  memcpy(p[i],source.p[i],sizeof(T)*ncol);
      } else
	Clear();
      return *this;
    }

    void Create(const int _nrow,const int _ncol);
    void Swap(Array2d<T>& array2);
    void Clear();
    void AdjustCapacity(const int newrow);
    bool Load(const char* filename,const int height,const int width);
    bool Save(const char* filename);
    void Print(std::ostream& of = std::cout) const;
    void RowSum(Array2d<T>& rowsum);
    void RowAverage(Array2d<T>& rowavg);
    void Multiply(const Array2d<T>& B,Array2d<T>& result);

    ~Array2d() { Clear(); }

  }; // class Array2d

  template<class T> class Array2dC {

  public:
    int nrow;
    int ncol;
    T** p;
    T* buf;

    Array2dC():nrow(0),ncol(0),p(NULL),buf(NULL) { }
    
    Array2dC(const int nrow,const int ncol):nrow(0),ncol(0),p(NULL),buf(NULL) {
      Create(nrow,ncol); 
    }

    Array2dC(const Array2dC<T>& source):nrow(0),ncol(0),p(NULL),buf(NULL) {
      if (source.buf!=NULL) {
	Create(source.nrow,source.ncol);
	memcpy(buf,source.buf,sizeof(T)*nrow*ncol);
      }
    }

    Array2dC<T>& operator=(const Array2dC<T>& source) {
      if (source.buf!=NULL) {
	Create(source.nrow,source.ncol);
	memcpy(buf,source.buf,sizeof(T)*nrow*ncol);
      }
      else
	Clear();
      return *this;
    }

    void Create(const int _nrow,const int _ncol);
    void Swap(Array2dC<T>& array2);
    void Clear();
    bool Load(const char* filename,const int height,const int width);
    bool Save(const char* filename);
    void Print(std::ostream& of = std::cout) const;
    void RowSum(Array2dC<T>& rowsum);
    void RowAverage(Array2dC<T>& rowavg);
    void Multiply(const Array2dC<T>& B,Array2dC<T>& result);

    ~Array2dC() { Clear(); }

  }; // class Array2dC

  // #include "IntImage.h"

  const REAL PI = (REAL)3.14159265358979323846;
  const REAL PI4 = (REAL)(PI/4);

  template<class T> class IntImage:public Array2dC<T> {
    
  private:
    IntImage(const IntImage<T> &source) { } // prohibit copy constructor

  public:
    IntImage():variance(0.0),label(-1) { }
    ~IntImage() { Clear(); }
    
    inline void Clear(void);
    inline void SetSize(const int h, const int w);
    void Print(std::ostream& of = std::cout) const;
    bool Load(const std::string& filename, size_t crop=0);
    bool FromMat(IplImage *input, const size_t crop=0);
    bool FromMat(IplImage *input, const size_t cropx, const size_t cropy);
    bool FromMat(IplImage *input, const crop_t &crop, string outimgfname);
    void Save(const std::string& filename) const;
    void Swap(IntImage<T>& image2);

    void Resize(IntImage<T> &result,const REAL ratio) const;
    void Resize(IntImage<T>& result,const int height,const int width) const;

	IntImage<T>& operator=(const IntImage<T>& source);
    void operator-=(const IntImage<T>& img2);
    void AbsoluteValue(void);
    void Thresh(const REAL thresh);

    void Sobel(IntImage<REAL>& result,const bool useSqrt,const bool normalize);

  public:
    using Array2dC<T>::nrow;
    using Array2dC<T>::ncol;
    using Array2dC<T>::buf;
    using Array2dC<T>::p;
    REAL variance;
    int label;

  }; // class IntImage

  class Funnel {

  public: 

    Funnel();
    bool ModelLoaded() { return model_loaded; }
    bool LoadModel(const string &modelfile);
    bool Process(IplImage **image);
    
  protected:

    void computeGaussian(vector<vector<float> > &Gaussian, int windowSize);
    float dist(vector<float> &a, vector<float> &b);
    void getSIFTdescripter(vector<float> &descripter, vector<vector<float> > &m,
			   vector<vector<float> > &theta, int x, int y, 
			   int windowSize, int histDim, int bucketsDim, 
			   vector<vector<float> > &Gaussian);
    float computeLogLikelihood(vector<vector<float> > &logDistField, 
			       vector<vector<float> > &fids, 
			       int numFeatureClusters);
    void getNewFeatsInvT(vector<vector<float> > &newFIDs, 
			 vector<vector<vector<float> > > &originalFeats,
			 vector<float> &vparams, float centerX, float centerY,
			 vector<pair<int, int> > &randPxls);

    // similarity transforms - x translation, y translation, rotation, uniform scaling:
    static const int NUM_PARAMS;
    static const int WINDOW_SIZE;
    static const int OUTERDIMW; 
    static const int OUTERDIMH;
    static const int INNERDIMW;
    static const int INNERDIMH;
    static const int SIFTHISTDIM;
    static const int SIFTBUCKETSDIM;

    bool model_loaded;

    int numFeatureClusters, edgeDescDim;
    vector<vector<float> > centroids;
    vector<float> sigmaSq;

    int numRandPxls;
    vector<pair<int, int> > randPxls;

    vector<vector<float> > logDistField;
    vector<vector<vector<float> > > logDFSeq;

    vector<vector<float> > Gaussian;

  }; // class Funnel

  class IlluminationNormalization {

  public: 

    IlluminationNormalization() { }
    bool Process(IplImage **image, const crop_t &crop = crop_t());
    
  protected:

    static const double GAMMA;
    static const double SIGMA0; // inner Gaussian
    static const double SIGMA1; // outer Gaussian
    static const double DO_NORM; // contrast normalization

  }; // class IlluminationNormalization

  class OCVCentrist : public OCVFeature {
  public:

    OCVCentrist(const string& img = "",
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, "", NULL, opt); 
      InitializeCentrist();
    }

    OCVCentrist(const string& img, const string& seg,
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, seg, NULL, opt); 
      InitializeCentrist();
    }

    OCVCentrist(bool) : OCVFeature(false) { }

    ~OCVCentrist() { 
      //if (funnel) delete funnel; 
      //if (illnorm) delete illnorm; 
    }

    virtual Feature *Create(const string& s) const
    { return (new OCVCentrist())->SetModel(s); }

    virtual string Name() const { return "OCVCentrist"; }
    virtual string LongName() const { return "OpenCV centrist feature"; }
    virtual string ShortText() const { return LongName(); }
    virtual string TargetType() const { return "image"; }

    virtual string Version() const;

    int printMethodOptions(ostream&) const;

  protected:

    bool InitializeCentrist();

    virtual bool ProcessOptionsAndRemove(list<string>&);

    virtual bool CalculateOpenCV(int f, int l);

    inline int CensusTransform(double** p,const int x,const int y) {
      int index = 0;
      if(p[x][y]<=p[x-1][y-1]) index |= 0x80;
      if(p[x][y]<=p[x-1][y]) index |= 0x40;
      if(p[x][y]<=p[x-1][y+1]) index |= 0x20;
      if(p[x][y]<=p[x][y-1]) index |= 0x10;
      if(p[x][y]<=p[x][y+1]) index |= 0x08;
      if(p[x][y]<=p[x+1][y-1]) index |= 0x04;
      if(p[x][y]<=p[x+1][y]) index |= 0x02;
      if(p[x][y]<=p[x+1][y+1]) index |= 0x01;
      return index;
    }

    void GenerateFeatureForSingleChannelImage(IntImage<double>& im, 
					      double* pointer,
					      std::ofstream* pcaout);
    void GenerateFeatureForSingleChannelImage(IntImage<double>& im,
					      double* pointer,
					      std::ofstream* pcaout,
					      Array2d<double>& lf);
    void PrepareScaleRatio(Array2dC<double>& scaleratio);
    void NormalizeLoadFactors(Array2d<double>& lf);
    void FindMinMaxValue(Array2d<double>& features,Array2dC<bool>& train,
			 Array2dC<double>& minmax);
    void ScaleFeatures(Array2d<double>& features,Array2d<double>& test);
    void GenerateFeatureForOneRect(double** p,const int x1,const int x2,
				   const int y1,const int y2,double* pointer,
				   Array2d<double>& lf,std::ofstream* pcaout);
    void GenerateHistForOneRect(double** p,const int x1,const int x2,
				const int y1,const int y2,double* pointer,
				double* temp);
    void GenerateFeatureForOneSplit(IntImage<double>& im,const int splitX,
				    const int splitY,double* pointer,
				    Array2d<double>& lf,std::ofstream* pcaout);

    centrist_opts options;

    Array2d<double> lf,lf2;

    static bool useSobel;
    static bool useBoth;
    static int sizeCV;
    static int extra;
    //static int splitlevel; // replaced by options.levels
    static int splitsize[4];
    static double splitratio[3];

    string detectorName;
    string descriptorName;

    string funneling_modelfile;
    static Funnel *funnel;
    static IlluminationNormalization *illnorm;    

  }; // class OCVCentrist

} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP

#endif // _OCVCentrist_
