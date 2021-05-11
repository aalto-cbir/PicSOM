// -*- C++ -*-         $Id: ColorSIFT.h,v 1.17 2013/03/21 09:13:39 jorma Exp $
/**
   \file ColorSIFT.h

   \brief Declarations and definitions of class ColorSIFT
   
   ColorSIFT.h contains declarations and definitions of the class
   ColorSIFT, which is a class that calculates the ColorSIFT feature
   using an external command

   \author Mats Sjoberg <mats.sjoberg@hut.fi>
   $Revision: 1.17 $
   $Date: 2013/03/21 09:13:39 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _ColorSIFT_
#define _ColorSIFT_

#include "External.h"
#include <regex.h>

namespace picsom {
  /// A class that ...
  class ColorSIFT : public External {
  public:
    /// definition for the current data type
    typedef float datatype;  // this should be selectable in run time...
  
    /// definition for the current data vector type
    typedef vector<datatype> vectortype;

    /** Constructor
        \param img initialise to this image
        \param opt list of options to initialise to
    */
    ColorSIFT(const string& img = "", 
        const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt);
      SetDefaults();
    }

    /** Constructor
        \param img initialise to this image
        \param seg initialise with this segmentation
        \param opt list of options to initialise to
    */
    ColorSIFT(const string& img, const string& seg,
         const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt); 
      SetDefaults();
    }

    ///
    void SetDefaults() {
      cs_detector = "harrislaplace";
      cs_descriptor = "opponentsift";
      cs_descriptor_length = 384;
      cs_codebook_path = "";
      cs_codebookMode = "";
      cs_codebook_size = 1000;
      cs_softsigma = -1.0;
      cs_pointselector = "";
      cs_version = "";
      cs_binary = "";

      use_kplabels = false; 
    }

    /// This constructor doesn't add an entry in the methods list.
    /// !! maybe it shouldn't but it does !!
    ColorSIFT(bool) : External(false) {}

    // ~ColorSIFT();

    virtual bool ProcessOptionsAndRemove(list<string>&);
    
    virtual Feature *Create(const string& s) const {
      return (new ColorSIFT())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
      return new ColorSIFTData(t, n, this);
    }

    virtual void deleteData(void *p) {
      delete (ColorSIFTData*)p;
    }

    virtual vector<string> RemoteExecution() const { 
      return RemoteExecutionNoBinary();
    }

    virtual string Name() const {
      return "ColorSIFT";
    }

    virtual string LongName() const { 
      return "ColorSIFT features";
    }

    virtual string ShortText() const {
      return "ColorSIFT features. Check http://www.science.uva.nl/~ksande/";
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

// fixme: here har, ds ??
/*    virtual string GetSegmentationMethodName() const {
      string m = External::GetSegmentationMethodName();
      if (use_har) 
        m += string(m==""?"":"+")+"har";
      if (use_dog)
        m += string(m==""?"":"+")+"dog";
      if (use_unnorm)
        m += string(m==""?"":"+")+"unnorm";
      return m;
    }*/

    virtual string Version() const;

    virtual featureVector getRandomFeatureVector() const;
  
    virtual int printMethodOptions(ostream&) const;

    virtual execchain_t GetExecChain(int f, bool all, int l);

    virtual bool ExecPreProcess(int f, bool all, int l);

    virtual bool ExecPostProcess(int f, bool all, int l);

/*    virtual bool ProcessIntermediateFiles(const list<string>&,
                                          int f, bool all, int l);*/

    bool ExecPostProcessInner(const string&, int f, bool all, int l);

    virtual bool CalculateOneFrame(int f);
  
    virtual bool NeedsConversion() const;

    virtual pair<size_t,size_t> MaximumSize() const {
      return make_pair(max_width, max_height);
    }

    virtual bool SaveOutput() const { return true; }

    virtual vector<string> UsedDataLabels() const {
      return use_kplabels ? KeyPointLabels() : UsedDataLabelsFromSegmentData();
    }

    vector<string> KeyPointLabels() const;

    virtual bool HasRawOutMode() const { return true; }

    bool RemoveTempFiles();
 
  protected:  
    /** The ColorSIFTData objects stores the data needed for the 
        feature calculation */
    class ColorSIFTData : public ExternalData {
    public:
      /** Constructor
          \param t sets the pixeltype of the data
          \param n sets the extension (jl?)
      */
      ColorSIFTData(pixeltype t, int n, const ColorSIFT *p) : 
        ExternalData(t, n, p) 
      {
        cs = p;
        result = vectortype(Length());
        // FIXME: we should probably be using ExternalData's datavec
        // here...
      }

      /// Defined because we have virtual member functions...
      virtual ~ColorSIFTData() {}

      /// Zeros the internal storage
      virtual void Zero() {
        ExternalData::Zero();
        result = vectortype(result.size(), 0.0);
      }

      /** Returns the name of the feature
          \return feature name
      */    
      virtual string Name() const { return "ColorSIFTData"; }

      /** Returns the lenght of the data contained in the object
          \return data length
      */
      virtual int Length() const { 
        return cs->cs_codebook_path.empty() ? 
	  cs->cs_descriptor_length : cs->cs_codebook_size;
      }

      virtual void AddValue(const vector<float>& v, int x, int y) {
        if ((int)v.size()!=Length())
          throw "ColorSIFTData::AddValue(vector<float>) size mismatch";
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
      const ColorSIFT *cs;

    }; // class ColorSIFTData

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
    string cs_detector;
    
    //
    string cs_descriptor;
    
    //
    string cs_codebook_path;

    //
    string cs_codebookMode;

    //
    int cs_descriptor_length;

    //
    int cs_codebook_size;

    //
    float cs_softsigma;

    //
    string cs_pointselector;

    //
    string cs_version;

    //
    string cs_binary;

    //
    map<string,string> tempfiles;

    //
    static size_t max_width, max_height;

  }; // class ColorSIFT

} // namespace picsom
#endif

// Local Variables:
// mode: font-lock
// End:
