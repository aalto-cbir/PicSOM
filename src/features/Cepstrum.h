// -*- C++ -*- 	$Id: Cepstrum.h,v 1.10 2008/10/31 09:44:26 jorma Exp $
/**
   \file Cepstrum.h

   \brief Declarations and definitions of class Cepstrum
   
   Cepstrum.h contains declarations and definitions of the class
   Cepstrum, which is a class that calculates the mel-cepstrum feature
   using an external command

   \author Mats Sjoberg <mats.sjoberg@hut.fi>
   $Revision: 1.10 $
   $Date: 2008/10/31 09:44:26 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _Cepstrum_
#define _Cepstrum_

#include "External.h"
#include <regex.h>

namespace picsom {
/// A class that ...
class Cepstrum : public External {
public:
  /// definition for the current data type
  typedef float datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  Cepstrum(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  Cepstrum(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt); 
  }

  /// This constructor doesn't add an entry in the methods list.
  /// !! maybe it shouldn't but it does !!
  Cepstrum(bool) : External(false) {}

  // ~Cepstrum();

  virtual Feature *Create(const string& s) const {
    return (new Cepstrum())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new CepstrumData(t, n, this);
  }

  virtual void deleteData(void *p){
    delete (CepstrumData*)p;
  }

  /** Overrided to provide support for videos, which are in fact
   *  images, but they cannot be read in with our current library functions.
   *  \return true if the input files are readable as images, false otherwise
   */
  virtual bool ImageReadable() const {
    return false;
  } 

  virtual vector<string> RemoteExecution() const {
    return RemoteExecutionLinux64();
  }

  virtual string Name() const {
    return "cepstr";
  }

  virtual string LongName() const { 
    return "cepstrum with mel-scale";
  }

  virtual string ShortText() const {
    return "12 cepstrum features (mel-scale), last value is power.";
  }

  virtual const string& DefaultZoningText() const {
    static string flat("1");
    return flat; 
  }

  virtual vector<string> UsedDataLabels() const {
    vector<string> ret;
    ret.push_back("1");
    return ret;

  }

  virtual string TargetType() const { return "sound"; }

  virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const;
  
  virtual int printMethodOptions(ostream&) const;

  virtual execchain_t GetExecChain(int f, bool all, int l);

  virtual bool ExecPostProcess(int /*f*/, bool /*all*/, int l);

  virtual bool CalculateOneFrame(int f);

  virtual bool SaveOutput() const { return true; }
 
protected:  
  /** The CepstrumData objects stores the data needed for the 
      feature calculation */
  class CepstrumData : public ExternalData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    CepstrumData(pixeltype t, int n, const Cepstrum *p) : ExternalData(t, n, p) {
      result = vectortype(Length());
    }

    /// Defined because we have virtual member functions...
    virtual ~CepstrumData() {}

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "CepstrumData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return 13; }

    virtual int DataUnitSize() const { return exts; } 

    virtual void AddValue(const vector<float>& v) {
      if ((int)v.size()!=Length())
	throw "CepstrumData::AddValue(vector<float>) size mismatch";
      transform(result.begin(), result.end(), v.begin(), result.begin(),
		plus<datatype>());
    }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool /*azc*/) const {
      featureVector v(result.size());

      for (size_t i=0; i<v.size(); i++)
	v[i] = result[i];

      return v;
    }
    
  protected:
    vectortype result;

  };

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
