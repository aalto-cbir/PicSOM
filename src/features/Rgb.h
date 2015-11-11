// -*- C++ -*- 	$Id: Rgb.h,v 1.28 2013/08/20 07:31:27 jorma Exp $
/**
   \file Rgb.h

   \brief Declarations and definitions of class Rgb
   
   Rgb.h contains declarations and definitions of class the Rgb, which
   is a class that performs the average RGB value feature extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.28 $
   $Date: 2013/08/20 07:31:27 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _Rgb_
#define _Rgb_

#include "PixelBased.h"

namespace picsom {
/// A class that performs the average RGB value feature extraction.
class Rgb : public PixelBased {
 public:
  /// definition for the current data type
  typedef float datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  Rgb(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  Rgb(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

  /// This constructor doesn't add an entry in the methods list.
  Rgb(bool) : PixelBased(false) {}

  // ~Rgb();

  virtual Feature *Create(const string& s) const {
    return (new Rgb())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new RgbData(t, n, this);
  }

  virtual void deleteData(void *p) {
    delete (RgbData*)p;
  }

  virtual string Name()          const { return "rgb"; }

  virtual string LongName()      const { return "average rgb color"; }

  virtual string ShortText()     const {
    return "Average values of Red, Green and Blue channels."; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5");
    return centerdiag; 
  }

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const;
  /*
  virtual bool probeTileFeatures(int x1,int y1, int x2, int y2,
			 featureVector &v,int probelabel=-1,
				 bool saveLabeling=false);
  */

  virtual int printMethodOptions(ostream&) const;

  virtual comp_descr_t ComponentDescriptions() const;

protected:  
  /// The RgbData objects stores the data needed for the feature calculation
  class RgbData : public PixelBasedData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    RgbData(pixeltype t, int n, const Feature *p) : PixelBasedData(t, n,p) {
      sum = vectortype(DataUnitSize());
      Zero();
    }

    /// Defined because we have virtual member functions...
    virtual ~RgbData() {}

    /// Zeros the internal storage
    virtual void Zero() {
      PixelBasedData::Zero();
      sum = vectortype(sum.size());

      /// set up the histogram bin fixed 3^dim histogram
      vector<float> lv(2);
      lv[0]=0.3333;
      lv[1]=0.6666;

      int dim = PixelTypeSize();
      hist_limits = vector<featureVector>(dim, lv); 
    }

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "RgbData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return sum.size(); }

    /** Adds data from one pixel to the internal storage
	\param v the data vector to add
    */
    virtual void AddPixel(const vector<int>& v, int x, int y) {
      if (v.size()!=sum.size())
	throw "RgbData::AddPixel(vector<int>) size mismatch";

      if (!Parent()->IsHistogramMode())
	transform(sum.begin(), sum.end(), v.begin(), sum.begin(),
		  plus<datatype>());
      else
	throw "RgbData::AddPixel(vector<int>) not implemented "
	  "for histogram mode";

      if (PixelVerbose())
	cout << "RgbData::AddPixel(vector<int>)" << endl;

      PixelBasedData::AddPixel(v, x, y);
    }

    /** Removes data from one pixel to the internal storage
	\param v the data vector to remove
    */
    virtual void RemovePixel(const vector<int>& v) {
      if (v.size()!=sum.size())
	throw "RgbData::RemovePixel(vector<int>) size mismatch";

      if (Parent()->IsHistogramMode())
	throw "RgbData::RemovePixel(vector<int>) not implemented "
	  "for histogram mode";

      transform(sum.begin(), sum.end(), v.begin(), sum.begin(),
		minus<datatype>());
      if (PixelVerbose())
	cout << "RgbData::RemovePixel(vector<int>)" << endl;

      PixelBasedData::RemovePixel(v);
    }

    /** Adds data from one pixel to the internal storage
	\param v the data vector to add
    */
    virtual void AddPixel(const vector<float>& v, int x, int y) {
      if (v.size()!=sum.size())
	throw "RgbData::AddPixel(vector<float>) size mismatch";

      transform(sum.begin(), sum.end(), v.begin(), sum.begin(),
		plus<datatype>());

      if (PixelVerbose())
	cout << "RgbData::AddPixel(vector<float>)" << endl;

      PixelBasedData::AddPixel(v, x, y);
    }

    /** Removes data from one pixel to the internal storage
	\param v the data vector to remove
    */
    virtual void RemovePixel(const vector<float>& v) {
      if (v.size()!=sum.size())
	throw "RgbData::RemovePixel(vector<float>) size mismatch";

      transform(sum.begin(), sum.end(), v.begin(), sum.begin(),
		minus<datatype>());

      if (PixelVerbose())
	cout << "RgbData::RemovePixel(vector<float>)" << endl;

      PixelBasedData::RemovePixel(v);
    }

    /// += operator 
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "RgbData::operator+=() not defined"; }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool azc) const {
      if (!azc && !Count())
	throw "RgbData::Result() : count==0 => division by zero";

      if (Feature::LabelVerbose())
	cout << "RgbData::Result(), count=" << Count();

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
    vectortype sum;

  };  // class RgbData

};  // class Rgb
}
#endif

// Local Variables:
// mode: font-lock
// End:
