// -*- C++ -*-  $Id: OCVMlhmsLbp.h,v 1.1 2013/04/30 11:47:55 markus Exp $
/**
   \file OCVMlhmsLbp.h

   \brief Declarations and definitions of class OCVMlhmsLbp
   
   OCVMlhmsLbp calculates multi-level histograms of color multi-scale
   local binary patterns with spatial pyramids. It is adapted from 
   the Matlab MEX implementation in the file "mlhmslbp_spyr.c" in the 
   "Scenes/Objects classification toolbox [1] by

   Author : Sébastien PARIS : sebastien.paris@lsis.org
   -------  Date : 09/06/2010

   [1] http://www.mathworks.com/matlabcentral/fileexchange/29800-scenesobjects-classification-toolbox

   \author Mats Sjöberg <mats.sjoberg@aalto.fi>
   $Revision: 1.1 $
   $Date: 2013/04/30 11:47:55 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _OCVMlhmsLbp_
#define _OCVMlhmsLbp_

#define tiny 1e-8
#define verytiny 1e-15
#define sqrt3 1.73205080756888
#define invsqrt2 0.707106781186547
#define invsqrt3 0.577350269189626
#define invsqrt6 0.408248290463863

#define mxMalloc malloc
#define mxFree delete

#include <OCVFeature.h>

namespace picsom {

  struct mlhmslbp_opts {
    double *scale;
    int nscale;
    double  *spyr;
    int     nspyr;
    double *kernelx;
    int    kyx;
    int    kxx;
    double *kernely;
    int    kyy;
    int    kxy;
    int    color;
    int    maptable;
    int    useFF;
    int    improvedLBP;
    int    rmextremebins;
    double *norm;
    double clamp;
  };

  class OCVMlhmsLbp : public OCVFeature {
  public:

    OCVMlhmsLbp(const string& img = "",
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, "", NULL, opt); 
      InitializeMlhmsLbp();
    }

    OCVMlhmsLbp(const string& img, const string& seg,
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, seg, NULL, opt); 
      InitializeMlhmsLbp();
    }

    OCVMlhmsLbp(bool) : OCVFeature(false) {}

    ~OCVMlhmsLbp() {
      mxFree(options.spyr);
      mxFree(options.scale);
      mxFree(options.kernelx);
      mxFree(options.kernely);
      mxFree(options.norm);
    }

    virtual Feature *Create(const string& s) const
    { return (new OCVMlhmsLbp())->SetModel(s); }

    virtual string Name() const { return "OCVMlhmsLbp"; }
    virtual string LongName() const { 
      return "Multi-Level Histograms of Color Multi-Scale Local Binary Pattern with Spatial Pyramid"; 
    }
    virtual string ShortText() const { return LongName(); }
    virtual string TargetType() const { return "image"; }

    virtual string Version() const;

    int printMethodOptions(ostream&) const;

  protected:

    bool InitializeMlhmsLbp();

    virtual bool ProcessOptionsAndRemove(list<string>&);

    virtual bool CalculateOpenCV(int f, int l);

#ifdef HAVE_OPENCV2_CORE_CORE_HPP    
    void myCvtColor(const cv::Mat& in, cv::Mat& out);
#endif

    int	number_histo_lbp(double * , int);
    void rgb2gray(unsigned char * , int , int , double *);
    void rgb2nrgb(unsigned char * , int , int , double *);
    void rgb2opponent(unsigned char * , int , int , double *);
    void rgb2nopponent(unsigned char * , int , int , double *);
    void rgb2hue(unsigned char * , int , int , double *);
    
    void MakeIntegralImage(double *, double *, int , int , double *);
    double Area(double * , int , int , int , int , int );
    void conv2(double *, double *, int  , int , int  , int , int , int , 
	       double * , double * );
    void compute_hmblbp(double *, int , int , int , int , int  , int  , 
			double * , int , double , double * , int , int* , 
			struct mlhmslbp_opts );
    void compute_hmblbpff(double *, int , int , int , int , int  , int  , 
			  double * , int , double , double * , int , int* , 
			  fftw_complex *, fftw_plan , double * , 
			  struct mlhmslbp_opts  );
    void mlhmslbp_spyr(double * , double * , int  , int   , int , int , 
		       struct mlhmslbp_opts  );

    mlhmslbp_opts options;
    
    vector<string> scalevals;
    vector<string> spyrvals;

    static int table_normal[256];
    static int table_u2[256];
    static int table_ri[256];
    static int table_riu2[256];

  };
}

#endif
