// -*- C++ -*- 	$Id: ColM.h,v 1.18 2014/01/23 13:06:03 jorma Exp $
/**
   \file ColM.h

   \brief Declarations and definitions of class ColM
   
   ColM.h contains declarations and definitions of class the ColM, which
   is a class that performs the feature extraction using three first central
   moments of colour distribution in cartesian HSV cone.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.18 $
   $Date: 2014/01/23 13:06:03 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _ColM_
#define _ColM_

#include "PixelBased.h"

namespace picsom {
/// A class that performs the average RGB value feature extraction.
class ColM : public PixelBased {
 
 public:
  /// definition for the current data type
  typedef float datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  ColM(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  ColM(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

  Feature *Initialize(const string& img, const string& seg,
		    segmentfile *s, const list<string>& opt,
		    bool allocate_data=true);

  /// This constructor doesn't add an entry in the methods list.
  ColM(bool) : PixelBased(false) {}

  // ~Rgb();

  virtual Feature *Create(const string& s) const {
    return (new ColM())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new ColMData(t, n, this);
  }

  virtual void deleteData(void *p){
    delete (ColMData*)p;
  }
  
  virtual string Name()          const { return "colm"; }

  virtual string LongName() const {
    return "colour moments in "+colourConversion; }

  virtual string ShortText()     const {
    return "Values of three first central moments of color distribution in "+
      colourConversion; }

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

  ///
  static const string& DefaultColourConversion() {
    static const string d = "hsvcone";
    return d;
  }

protected:  

 virtual bool ProcessOptionsAndRemove(list<string>&);
  //  void ProcessOptionsLocal(const std::list<string>& l);

  /// The ColMData objects stores the data needed for the feature calculation
  class ColMData : public PixelBasedData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    ColMData(pixeltype t, int n, const Feature *p) : PixelBasedData(t, n, p) {
      sum = vectortype(3*DataUnitSize());
      Zero();
    }

    /// Defined because we have virtual member functions...
    virtual ~ColMData() {}

    /// Zeroes the internal storage
    virtual void Zero() {
      PixelBasedData::Zero();
      sum = vectortype(sum.size());

      hist_limits.clear();
      // vector<float> empty, lv;
      // int n = 5;
      // for (int i=1; i<n; i++)
      // 	lv.push_back(float(i)/n);
      // int dim = PixelTypeSize();
      // //cout << "dim=" << dim << endl;
      // if (dim==3) {
      // 	hist_limits.push_back(empty);
      // 	hist_limits.push_back(empty);
      // 	hist_limits.push_back(lv);
      // }

      const string& hfb = ((ColM*)Parent())->histfixedbins;
      size_t p = 0;
      while (hfb.length()>p) {
	size_t q = hfb.find(",", p);
	if (q==string::npos)
	  q = hfb.length();
	size_t d = atoi(hfb.substr(p, q-p).c_str());
	vector<float> lv;
	for (size_t i=1; i<d; i++)
	  lv.push_back(float(i)/d);
	hist_limits.push_back(lv);
	p = q+1;
      }
    }

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "ColMData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return sum.size(); }

    /** Adds data from one pixel to the interna0l storage
	\param v the data vector to add
    */
    virtual void AddPixel(const vector<int>&, int, int) {
      throw "ColMData::AddPixel(vector<int>) not implemented";
    }
    
    virtual void AddPixel(const vector<float>& v, int x, int y) {
      if (sum.size()!=3*v.size())
	throw "ColMData::AddPixel(vector<float>) size mismatch : ";

      size_t nch = v.size();

      vector<float> cv(v);
      ((ColM*)Parent())->ColourConversion(cv);
      
      if (isnan(cv[0]) || (nch>1 && isnan(cv[1])) || (nch>2 && isnan(cv[2])))
	throw string("colour conversion returned nan");

      vector<float> powers;
      if (nch>0) powers.push_back(cv[0]);
      if (nch>1) powers.push_back(cv[1]);
      if (nch>2) powers.push_back(cv[2]);

      if (nch>0) powers.push_back(cv[0]*cv[0]);
      if (nch>1) powers.push_back(cv[1]*cv[1]);
      if (nch>2) powers.push_back(cv[2]*cv[2]);

      if (nch>0) powers.push_back(powers[nch]  *cv[0]);
      if (nch>1) powers.push_back(powers[nch+1]*cv[1]);
      if (nch>2) powers.push_back(powers[nch+2]*cv[2]);

      transform(sum.begin(), sum.end(), powers.begin(), sum.begin(),
		plus<datatype>());
      if (PixelVerbose())
	cout << "ColMData::AddPixel(vector<float>)" << endl;
      PixelBasedData::AddPixel(v, x, y);
    }

    /** Removes data from one pixel to the internal storage
	\param v the data vector to remove
    */
    virtual void RemovePixel(const vector<int>& ) {
      throw "ColMData::RemovePixel(vector<int>) not implemented";
    }	

    virtual void RemovePixel(const vector<float>& v) {
      if (sum.size()!=3*v.size())
	throw "ColMData::RemovePixel(vector<float>) size mismatch";
      
      size_t nch = v.size();

      vector<float> cv(v);
      ((ColM*)Parent())->ColourConversion(cv);
      
      vector<float> powers;
      if (nch>0) powers.push_back(cv[0]);
      if (nch>1) powers.push_back(cv[1]);
      if (nch>2) powers.push_back(cv[2]);

      if (nch>0) powers.push_back(cv[0]*cv[0]);
      if (nch>1) powers.push_back(cv[1]*cv[1]);
      if (nch>2) powers.push_back(cv[2]*cv[2]);

      if (nch>0) powers.push_back(powers[3]*cv[0]);
      if (nch>1) powers.push_back(powers[4]*cv[1]);
      if (nch>2) powers.push_back(powers[5]*cv[2]);

      transform(sum.begin(), sum.end(), powers.begin(), sum.begin(),
		minus<datatype>());
      if (PixelVerbose())
	cout << "ColmData::RemovePixel(vector<float>)" << endl;
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
      // calculates first three central moments using
      // m1 = E[X]
      // m2 = E[X¨2] - E[X]¨2
      // m3 = E[X¨3] - 3E[X]E[X¨2]+2E[X]¨3

      // ordering of the compopnents:
      // m1_h
      // m1_s
      // m1_v
      // m2_h
      // h2_s
      // h2_v
      // m3_h
      // m3_s
      // m3_v

      if (!azc && !Count())
	throw "ColMData::Result() : count==0 => division by zero";

      if (sum.size()!=9 && sum.size()!=3)
	throw "ColMData::Result() : sum.size() != 9 | 3";

      if (Feature::LabelVerbose())
	cout << "ColMData::Result(), count=" << Count();

      size_t nch = sum.size()==9 ? 3 : 1;
      
      double mul = Count() ? 1.0/Count() : 1.0;
      featureVector v(sum.size());
      for (size_t i=0; i<nch; i++) {
	v[i] = sum[i]*mul;

	v[nch+i]= mul*(sum[nch+i]-mul*sum[i]*sum[i]);
	if (v[nch+i]<0) {
	  //cout << "fixing negative arg of sqrt(" << v[nch+i] <<")"<< endl;
	  v[nch+i]=0;
	}
	else
	  v[nch+i] = sqrt(v[nch+i]);

	v[2*nch+i]= mul*(sum[2*nch+i]-mul*(3*sum[i]*sum[nch+i]-
				   2*mul*sum[i]*sum[i]*sum[i]));
	if(v[2*nch+i]<0)
	  v[2*nch+i] = -pow((double)-v[2*nch+i],1.0/3.0);
	else
	  v[2*nch+i] = pow((double)v[2*nch+i],1.0/3.0);

	if (Feature::LabelVerbose()){
	  cout << " " << sum[i] << "->" << v[i];
	  cout << " " << sum[nch+i] << "->" << v[nch+i];
	  cout << " " << sum[2*nch+i] << "->" << v[2*nch+i];
	}
      }

      if (Feature::LabelVerbose())
	cout << endl;

      return v;
    }

  protected:
    /// internal storage of the total sum of pixel values
    vectortype sum;
  };

  ///
  string colourConversion;

  ///
  string histfixedbins;

 public:
  void ColourConversion(std::vector<float> &v) const;
};
}
#endif

// Local Variables:
// mode: font-lock
// End:
