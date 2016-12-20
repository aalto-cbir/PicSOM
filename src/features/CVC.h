// -*- C++ -*- 	$Id: CVC.h,v 1.3 2014/06/18 14:48:07 jorma Exp $
/**
   \file CVC.h

   \brief Declarations and definitions of class CVC
   
   CVC.h contains declarations and definitions of class the CVC, which
   is a class that performs the average CVC value feature extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.3 $
   $Date: 2014/06/18 14:48:07 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _CVC_
#define _CVC_

#include "Feature.h"

namespace picsom {
  /// A class that performs the average CVC value feature extraction.
  class CVC : public Feature {
  public:
    /// definition for the current data type
    typedef float datatype;  // this should be selectable in run time...
  
    /// definition for the current data vector type
    typedef vector<datatype> vectortype;

    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    CVC(const string& img = "", const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt);
      descriptor = 0;
      grid = 0;
      radius = 10;
      blocks.push_back("1x1");
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    CVC(const string& img, const string& seg,
	const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt);
    }

    /// This constructor doesn't add an entry in the methods list.
    CVC(bool) : Feature(false) {}

    // ~CVC();
  
    //
    virtual bool ProcessOptionsAndRemove(list<string>&);

    virtual Feature *Create(const string& s) const {
      return (new CVC())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
      return new CVCData(t, n, this);
    }

    virtual void deleteData(void *p) {
      delete (CVCData*)p;
    }

    virtual string Name()          const { return "CVC"; }

    virtual string LongName()      const { return "CVC color"; }

    virtual string ShortText()     const {
      return "CVC color descriptors CN, HUE, DCD, OPP."; }

    virtual string Version() const;

    virtual featureVector getRandomFeatureVector() const;

    virtual int printMethodOptions(ostream&) const;

    // virtual comp_descr_t ComponentDescriptions() const;

    virtual string TargetType() const { return "image"; }

    virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

    virtual bool HasRawOutMode() const { return true; }

    virtual bool CalculateOneLabel(int, int) { return false; }

    virtual bool CalculateOneFrame(int);

    virtual featureVector CalculateRegion(const picsom::Region&) {
      return featureVector(); 
    }

    virtual treat_type DefaultWithinFrameTreatment()  const { return treat_concatenate; }
    virtual treat_type DefaultBetweenFrameTreatment() const { return treat_separate;    }
    virtual treat_type DefaultBetweenSliceTreatment() const { return treat_separate;    }

    virtual vector<string> UsedDataLabels() const {
      // return UsedDataLabelsFromSegmentData();
      vector<string> lab;
      //size_t l = 0;
      for (size_t i=0; i<blocks.size(); i++) {
	string s = blocks[i];
	size_t p = s.find('x');
	if (p!=string::npos) {
	  size_t w = atoi(s.c_str());
	  size_t h = atoi(s.substr(p+1).c_str());
	  for (size_t j=0; j<h; j++)
	    for (size_t k=0; k<w; k++)
	      lab.push_back(s+"_"+ToStr(k)+"_"+ToStr(j));
	}
      }
      return lab;
    }

    static int descriptor_string2int(const string&);
    static const string& descriptor_int2string(int);

    const string& DetectorFile();

  protected:  
    ///
    class cvc_descriptor {
    public:
      ///
      string str() const {
	ostringstream os;
	os << "<" << type << " " << x << " " << y << " " << r << " "
	   << a << " " << b << ">;";
	for (size_t i=0; i<v.size(); i++)
	  os << " " << v[i];
	os << ";";
	return os.str();
      }

      ///
      string type;

      ///
      size_t x, y, r, a, b;

      ///
      vector<float> v;
    }; // cvc_descriptor


    ///
    int descriptor;

    ///
    size_t grid;

    ///
    float radius;

    ///
    map<string,string> detectorfile_map;

    ///
    vector<string> blocks;

    /// The CVCData objects stores the data needed for the feature calculation
    class CVCData : public Data {
    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
    CVCData(pixeltype t, int n, const CVC *p) : Data(t, n, p) {
	cvc = p;
	count = 0;
	result = vectortype(DataUnitSize());
	Zero();
      }

      /// Defined because we have virtual member functions...
      virtual ~CVCData() {}

      /// Zeros the internal storage
      virtual void Zero() {
	result = vectortype(result.size());
      }

      /** Returns the name of the feature
	  \return feature name
      */    
      virtual string Name() const { return "CVCData"; }

      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      virtual int Length() const { return result.size(); }

      virtual int DataUnitSize() const {
	switch (cvc->descriptor) {
	case 1: return 11;
	case 3: return 25;
	case 4: return 11;
	}
	return -1;
      }

      /// += operator 
      virtual Data& operator+=(const Data &) {
      	// this should check that typeof(d) == typeof(*this) !!!
      	throw "CVCData::operator+=() not defined"; }

      virtual void AddValue(const vector<float>& v, int x, int y) {
        if ((int)v.size()!=Length())
          throw "CVCData::AddValue(vector<float>) size mismatch";
        transform(result.begin(), result.end(), v.begin(), result.begin(),
                  plus<datatype>());
	count++;

        addToHistogram(v);
        Parent()->ProcessRawOutVector(v, x, y);
      }

      /** Returns the result of the feature extraction
	  \return feature result vector
      */
      virtual featureVector Result(bool /*azc*/) const {
        featureVector v(result.size());

	float mul = count ? 1.0/count : 1.0;

        for (size_t i=0; i<v.size(); i++)
          v[i] = result[i]*mul;

        return v;
      }

    protected:
      ///
      const CVC *cvc;

      /// internal storage
      vectortype result;
      
      ///
      size_t count;

    };  // class CVCData

  };  // class CVC
}
#endif

// Local Variables:
// mode: font-lock
// End:
