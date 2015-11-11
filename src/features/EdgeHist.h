// -*- C++ -*- 	$Id: EdgeHist.h,v 1.7 2008/10/31 09:44:26 jorma Exp $
/**
   \file EdgeHist.h

   \brief Declarations and definitions of class EdgeHist
   
   EdgeHist.h contains declarations and definitions of class the EdgeHist, which
   is a class that performs sobel edge histogram extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.7 $
   $Date: 2008/10/31 09:44:26 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _EdgeHist_
#define _EdgeHist_

#include "PixelBased.h"

namespace picsom {
/// A class that performs the average colour value  extraction.
class EdgeHist : public PixelBased {



 public:
  /// definition for the current data type
  typedef int datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  EdgeHist(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  EdgeHist(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  EdgeHist(bool) : PixelBased(false) {}

  // ~EdgeHist();

  virtual Feature *Create(const string& s) const {
    return (new EdgeHist())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new EdgeHistData(t, n, this);
  }


  virtual void deleteData(void *p){
    delete (EdgeHistData*)p;
  }


  virtual string Name()          const { return "edgehist"; }

  virtual string LongName()      const { return "edge histogram"; }

  virtual string ShortText()     const {
    return "Histogram of four Sobel edge directions."; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5");
    return centerdiag; 
  }

  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const{
    return vector<float>(4);
  }

  /*
  virtual bool probeTileFeatures(int x1,int y1, int x2, int y2,
			 featureVector &v,int probelabel=-1,
				 bool saveLabeling=false);
  */

  virtual int printMethodOptions(ostream&) const;

  protected:

  virtual bool ProcessOptionsAndRemove(list<string>&); 

  /// The EdgeHistData objects stores the data needed for the feature calculation
  class EdgeHistData : public PixelBasedData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    EdgeHistData(pixeltype t, int n,const Feature *p) : PixelBasedData(t, n,p) {
      sum = vectortype(4);
    }

    /// Defined because we have virtual member functions...
    virtual ~EdgeHistData() {}

    /// Zeroes the internal storage
    virtual void Zero() {
      PixelBasedData::Zero();
      sum = vectortype(4,0);
    }

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "EdgeHistData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return sum.size(); }

    /** Adds data from one pixel to the internal storage
	\param v the data vector to add
    */
    virtual void AddPixel(const vector<int>& v, int x, int y) {
      transform(sum.begin(), sum.end(), v.begin(), sum.begin(),
		plus<datatype>());
      PixelBasedData::AddPixel(v, x, y);
    }
    
    virtual void AddPixel(const vector<float>&, int, int) {
      throw "EdgeHistData::AddPixel(vector<float>) not implemented";
    }

    /** Removes data from one pixel to the internal storage
	\param v the data vector to remove
    */
    virtual void RemovePixel(const vector<int>& v) {
      transform(sum.begin(), sum.end(), v.begin(), sum.begin(),
		minus<datatype>());
      PixelBasedData::RemovePixel(v);
    }	

    virtual void RemovePixel(const vector<float>&) {
      throw "EdgeHistData::RemovePixel(vector<float>) not implemented";
    }

    /// += operator 
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "EdgeHistData::operator+=() not defined"; }

 /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool azc) const {
      if (!azc && !Count())
	throw "EdgeHistData::Result() : count==0 => division by zero";

      if (Feature::LabelVerbose())
	cout << "EdgeHistData::Result(), count=" << Count();

      double mul = Count() ? 1.0/Count() : 1.0;
      featureVector v(sum.size());
      for (size_t i=0; i<v.size(); i++) {
	v[i] = sum[i]*mul;
	if (Feature::LabelVerbose())
	  cout << " " << sum[i] << "->" << v[i];
      }

      if (Feature::LabelVerbose())
	cout << endl;

      return v;
    }

  protected:

   

    /// internal storage of the total sum of pixel values
    vector<int> sum;
  };

 virtual bool CalculateOneFrame(int f) { 

    // OBS! Side effect: discards the preprocessing result "sobelthresh" 

    bool ret= CalculateCommon(f, true);
    SegmentData()->discardPreprocessResult(f,"sobelthresh");
    return ret;
 }

  virtual bool CalculateCommon(int f, bool all, int l = -1) {
    EnsureImage();
    rptr=(preprocessResult_SobelThresh*)
      SegmentData()->accessPreprocessResult(f,"sobelthresh");
    return PixelBased::CalculateCommon(f,all,l);
  }
  
  virtual void CalculateOnePixel(int x, int y, PixelBasedData *d) {
    const int ind=x+y*Width();
    vector<int> v(4);
    v[0]=rptr->d0[ind];
    v[1]=rptr->d45[ind];
    v[2]=rptr->d90[ind];
    v[3]=rptr->d135[ind];
    
    d->AddPixel(v, x, y);
  }

  virtual void UnCalculateOnePixel(int x, int y, PixelBasedData *d) {
    const int ind=x+y*Width();
    vector<int> v(4);
    v[0]=rptr->d0[ind];
    v[1]=rptr->d45[ind];
    v[2]=rptr->d90[ind];
    v[3]=rptr->d135[ind];
    
    d->RemovePixel(v);
  }
  
  preprocessResult_SobelThresh *rptr;
  
public:
  virtual featureVector CalculateRegion(const Region& r) {

    EnsureImage();
    rptr=(preprocessResult_SobelThresh*)
      SegmentData()->accessPreprocessResult("sobelthresh");
    return PixelBased::CalculateRegion(r);
  }

  virtual bool moveRegion(const Region &r, int from, int to) {

    EnsureImage();
    rptr=(preprocessResult_SobelThresh*)
      SegmentData()->accessPreprocessResult("sobelthresh");
    return PixelBased::moveRegion(r,from,to);
  }

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
