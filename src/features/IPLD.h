// -*- C++ -*- 	$Id: IPLD.h,v 1.11 2009/09/08 08:31:04 jorma Exp $
/**
   \file IPLD.h

   \brief Declarations and definitions of class IPLD
   
   IPLD.h contains declarations and definitions of the class
   IPLD, which is a class that calculates the mel-ipld feature
   using an external command

   \author Mats Sjoberg <mats.sjoberg@hut.fi>
   $Revision: 1.11 $
   $Date: 2009/09/08 08:31:04 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _IPLD_
#define _IPLD_

#include "External.h"
#include <regex.h>

namespace picsom {
  /// A class that ...
  class IPLD : public External {
  public:
    /// definition for the current data type
    typedef float datatype;  // this should be selectable in run time...
  
    /// definition for the current data vector type
    typedef vector<datatype> vectortype;

    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    IPLD(const string& img = "", const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt);
      use_har = true;
      use_kplabels = use_dog = use_unnorm = false;
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    IPLD(const string& img, const string& seg,
	 const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt); 
      use_har = true;
      use_kplabels = use_dog = use_unnorm = false;
    }

    /// This constructor doesn't add an entry in the methods list.
    /// !! maybe it shouldn't but it does !!
    IPLD(bool) : External(false) {}

    // ~IPLD();

    virtual bool ProcessOptionsAndRemove(list<string>&);
    
    virtual Feature *Create(const string& s) const {
      return (new IPLD())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
      return new IPLDData(t, n, this);
    }

    virtual void deleteData(void *p) {
      delete (IPLDData*)p;
    }

    virtual vector<string> RemoteExecution() const { 
      return LocalLinuxOrRemoteLinux64();
    }

    virtual string Name() const {
      return "ipld";
    }

    virtual string LongName() const { 
      return "ipld features";
    }

    virtual string ShortText() const {
      return "128-dimensional ipld features.";
    }

    virtual const string& DefaultZoningText() const {
      static string flat("1");
      return flat; 
    }

    virtual treat_type DefaultWithinFrameTreatment() const {
      return use_kplabels ? treat_separate : treat_concatenate;
    }

    virtual string TargetType() const { return "image"; }

    virtual pixeltype DefaultPixelType() const { return pixel_grey; }

    virtual string GetSegmentationMethodName() const {
      string m = External::GetSegmentationMethodName();
      if (use_har)
	m += string(m==""?"":"+")+"har";
      if (use_dog)
	m += string(m==""?"":"+")+"dog";
      if (use_unnorm)
	m += string(m==""?"":"+")+"unnorm";
      return m;
    }

    virtual string Version() const;

    virtual featureVector getRandomFeatureVector() const;
  
    virtual int printMethodOptions(ostream&) const;

    virtual execchain_t GetExecChain(int f, bool all, int l);

    virtual bool ExecPreProcess(int f, bool all, int l);

    virtual bool ExecPostProcess(int f, bool all, int l);

    virtual bool ProcessIntermediateFiles(const list<string>&,
					  int f, bool all, int l);

    bool ExecPostProcessInner(const string&, int f, bool all, int l);

    virtual bool CalculateOneFrame(int f);

    virtual bool SaveOutput() const { return true; }

    virtual vector<string> UsedDataLabels() const {
      return use_kplabels ? KeyPointLabels() : UsedDataLabelsFromSegmentData();
    }

    vector<string> KeyPointLabels() const;

    virtual bool HasRawOutMode() const { return true; }

    bool RemoveTempFiles();
 
  protected:  
    /** The IPLDData objects stores the data needed for the 
	feature calculation */
    class IPLDData : public ExternalData {
    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      IPLDData(pixeltype t, int n, const IPLD *p) : ExternalData(t, n, p) {
	result = vectortype(Length());
      }

      /// Defined because we have virtual member functions...
      virtual ~IPLDData() {}

      /** Returns the name of the feature
	  \return feature name
      */    
      virtual string Name() const { return "IPLDData"; }

      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      virtual int Length() const { return 128; }

      virtual void AddValue(const vector<float>& v, int x, int y) {
	if ((int)v.size()!=Length())
	  throw "IPLDData::AddValue(vector<float>) size mismatch";
	transform(result.begin(), result.end(), v.begin(), result.begin(),
		  plus<datatype>());

	addToHistogram(v);
	Parent()->ProcessRawOutVector(v, x, y);
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

    }; // class IPLDData

    //
    typedef struct {
      float x, y, s, a;
      vector<float> v;
    } keypoint;

    //
    typedef list<keypoint> kp_list_t;

    //
    kp_list_t kp_list;

    //
    bool use_kplabels;

    //
    bool use_har;

    //
    bool use_dog;

    //
    bool use_unnorm;

    //
    map<string,string> tempfiles;

  }; // class IPLD

} // namespace picsom
#endif

// Local Variables:
// mode: font-lock
// End:
