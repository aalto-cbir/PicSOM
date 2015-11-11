// -*- C++ -*- 	$Id: Colour.h,v 1.10 2013/06/26 13:50:39 mats Exp $
/**
   \file Colour.h

   \brief Declarations and definitions of class Colour
   
   Colour.h contains declarations and definitions of class the Colour, which
   is a class that performs the average colour value feature extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.10 $
   $Date: 2013/06/26 13:50:39 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _Colour_
#define _Colour_

#include "PixelBased.h"

namespace picsom {
/// A class that performs the average colour value  extraction.
class Colour : public PixelBased {



 public:
  /// definition for the current data type
  typedef float datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  Colour(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  Colour(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  Colour(bool) : PixelBased(false) {}

  // ~Colour();

  virtual Feature *Create(const string& s) const {
    return (new Colour())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new ColourData(t, n, this);
  }


  virtual void deleteData(void *p){
    delete (ColourData*)p;
  }


  virtual string Name()          const { return "colour"; }

  virtual string LongName()      const { return "average colour"; }

  virtual string ShortText()     const {
    return "Average values of components in specified colour space."; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5");
    return centerdiag; 
  }

  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const;
  /*
  virtual bool probeTileFeatures(int x1,int y1, int x2, int y2,
			 featureVector &v,int probelabel=-1,
				 bool saveLabeling=false);
  */

  virtual int printMethodOptions(ostream&) const;

  protected:

  virtual bool ProcessOptionsAndRemove(list<string>&); 

  /// The ColourData objects stores the data needed for the feature calculation
  class ColourData : public PixelBasedData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    ColourData(pixeltype t, int n,const Feature *p) : PixelBasedData(t, n,p) {
      sum = vectortype(DataUnitSize());
      Zero();
    }

    /// Defined because we have virtual member functions...
    virtual ~ColourData() {}

    /// Zeroes the internal storage
    virtual void Zero() {
      PixelBasedData::Zero();
      sum = vectortype(sum.size());

      // hack for hue histogram ...
      hist_limits.clear();
      vector<float> empty, lv;
      int n = 6;
      for (int i=1; i<n; i++)
	lv.push_back(float(i)/n);

      int dim = PixelTypeSize();
      // cout << "dim=" << dim << endl;
      if (dim==3) {
	hist_limits.push_back(lv);
	hist_limits.push_back(empty);
	hist_limits.push_back(empty);
      }
    }

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "ColourData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return sum.size(); }

    /** Adds data from one pixel to the internal storage
	\param v the data vector to add
    */
    virtual void AddPixel(const vector<int>&, int, int) {
      throw "ColourData::AddPixel(vector<int>) not implemented";
    }
    
    virtual void AddPixel(const vector<float>& v, int x, int y) {
      vector<float> cv(v);

      if (v.size()==3) {
	((Colour*)Parent())->ColourConversion(cv);
      
	if(isnan(cv[0]) || isnan(cv[1]) || isnan(cv[2]))
	  throw string("colour conversion returned nan");
      }

      transform(sum.begin(), sum.end(), cv.begin(), sum.begin(),
		plus<datatype>());
      if (PixelVerbose())
	cout << "ColourData::AddPixel(vector<float>)" << endl;
      PixelBasedData::AddPixel(cv, x, y);
    }

    /** Removes data from one pixel to the internal storage
	\param v the data vector to remove
    */
    virtual void RemovePixel(const vector<int>& ) {
      throw "ColourData::RemovePixel(vector<int>) not implemented";
    }	

    virtual void RemovePixel(const vector<float>& v) {
      if (v.size()!=3)
	throw "ColMData::RemovePixel(vector<int>) size mismatch";
      
      vector<float> cv(v);

      ((Colour*)Parent())->ColourConversion(cv);
   
      transform(sum.begin(), sum.end(), cv.begin(), sum.begin(),
		minus<datatype>());
      if (PixelVerbose())
	cout << "ColourData::RemovePixel(vector<int>)" << endl;
      PixelBasedData::RemovePixel(v);
    }

    /// += operator 
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "ColourData::operator+=() not defined"; }

 /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool azc) const {
      if (!azc && !Count())
	throw "ColourData::Result() : count==0 => division by zero";

      if (Feature::LabelVerbose())
	cout << "ColourData::Result(), count=" << Count();

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
  };

 public:
  void ColourConversion(std::vector<float> &v) const; 
  std::string colourConversion;

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
