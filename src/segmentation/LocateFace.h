// -*- C++ -*-  $Id: LocateFace.h,v 1.10 2004/02/17 11:59:32 rozyang Exp $

#ifndef _LOCATEFACE_H_
#define _LOCATEFACE_H_

#include <Segmentation.h>
#include <valarray>
#include <complex>

namespace picsom {

  class LocateFace : public Segmentation {
  public:
    typedef double cmplx_t;

    /// 
    LocateFace() {bRaw=false;}

    /// 
    LocateFace(bool b) : Segmentation(b) {bRaw=false;}

    /// 
    virtual ~LocateFace() {}

    /// Here are the pure virtuals overloaded:
    
    ///
    virtual Segmentation *Create() const { return new LocateFace(); }  

    ///
    const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"LocateFace":"lf"; }

    ///
    virtual bool Process();

    ///
    virtual bool ProcessJpakkane();

    ///
    virtual bool ProcessRoz();

    ///
    virtual bool ProcessNew();

    ///
    virtual int ProcessOptions(int, char**);

    virtual const char *Description() const {return "locateface";};

  protected:
    ///
    void ShowPoints(const vector<complex<cmplx_t> >&, const string&) const;

    /** Calculates gradient and stores it in gradimg.
	\post absolute value sum gradient image \e gradimg is made.
    */
    void CalculateGradient();

    /** Searches for the best position for horizon symmetry axis in.
	\param  img image to process.
	\return horizontal coordinate.
     */
    int HorizontalSymmetryAxis(const imagedata& img) const;

    /** Searches for the best location for upper half circle in an edge image.
	\param  img image to process.
	\param  x horizontal coordinate.
	\return vertical coordinate.
     */
    int MatchUpperHalfCircles(const imagedata& img, unsigned int x) const;

    /** Creates image in polar coordinates and stores it in sweepimg.
	\post gradient image in polar coordinates \e sweepimg is made.
    */
    void SweepImage(int rows, int x0, int y0);

    void Foud(vector<complex<cmplx_t> >&, const int level = 10);

    void FindMaxOnLines(vector<complex<cmplx_t> >& coordxy, int x0, int y0, double maxside, double b, double ampl);			
    void FindMaxOnLines0(vector<complex<cmplx_t> >& coordxy, const int x0, const int y0, const int maxside, const double b);
    
    void RemoveEdgeErrors(vector<complex<cmplx_t> >&, const int x0);

    bool IsInside(coord p, coord *polygon, const int N);

    bool LocateRoughEllipse(vector<complex<cmplx_t> >& coordxy, int angle1, int angle2,  vector<double>& coef);
    bool ConvertEllipseParameters(vector<double>& coef, double& x0, double& y0, double& ld, double& sd);

    // jl
    static float max_value_index(const valarray<float>&a, int& j);

    // jl
    imagedata gradimg;

    // jl
    imagedata sweepimg;
    
    // jl
    imagedata maskimg;
    
    // bRaw==true i.e. the source images are already face images. Directly save the image
    //                 as head box to xml result file
    // bRaw==false i.e. the LocateFace algorithm should be performed.
    bool bRaw;
 };

} // namespace picsom

#endif // _LOCATEFACE_H_

// Local Variables:
// mode: font-lock
// End:
