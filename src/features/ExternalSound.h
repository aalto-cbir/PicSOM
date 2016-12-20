// -*- C++ -*-  $Id: ExternalSound.h,v 1.6 2013/02/25 11:02:40 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file ExternalSound.h

   \brief Declarations and definitions of class ExternalSound
   
   ExternalSound.h contains declarations and definitions of the class
   ExternalSound, which is a class that calculates some audio features
   (e.g the mel-cepstrum feature) using an external command

   \author Hannes Muurinen <hannes.muurinen@tkk.fi>
   $Revision: 1.6 $
   $Date: 2013/02/25 11:02:40 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _ExternalSound_
#define _ExternalSound_

#include <External.h>

namespace picsom {
/// A class that ...
class ExternalSound : public External {
public:
  /// definition for the current data type
  typedef float datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  ExternalSound(const string& img = "", const list<string>& opt = (list<string>)0) {
    tempfilename = "";
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  ExternalSound(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    tempfilename = "";
    Initialize(img, seg, NULL, opt); 
  }

  /// This constructor doesn't add an entry in the methods list.
  /// !! maybe it shouldn't but it does !!
  ExternalSound(bool) : External(false) { tempfilename = ""; }

  ~ExternalSound() {
    if (tempfilename != "") {
      //cout << "unlinking " << tempfilename << endl;
      Unlink(tempfilename);
      size_t wp = tempfilename.find(".wav");
      if (wp != string::npos) {
	tempfilename = tempfilename.substr(0,wp);
	//cout << "unlinking " << tempfilename << endl;
	Unlink(tempfilename);
      }
      tempfilename = "";
    }
  }

  virtual Feature *Create(const string& s) const {
    return (new ExternalSound())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new ExternalSoundData(t, n, this);
  }

  virtual void deleteData(void *p){
    delete (ExternalSoundData*)p;
  }

  /** Overrided to provide support for videos, which are in fact
   *  images, but they cannot be read in with our current library functions.
   *  \return true if the input files are readable as images, false otherwise
   */
  virtual bool ImageReadable() const {
    return false;
  } 

  /// Is the feature an image feature? Override with a false-returning 
  /// function if not.
  virtual bool IsImageFeature() const { return false; }



  virtual bool LoadObjectData(const string& datafile, 
			      const string& /*segmentfile*/ = ""); 


  virtual featureVector CalculatePerFrame(int, int, bool);


  /** Returns the number of image frames
      \return number of frames
  */
  virtual int Nframes() const { return videoframecount; }

  virtual string ObjectDataFileName() const {
    return datafilename;
  }

  virtual vector<string> RemoteExecution() const { 
    return RemoteExecutionLinux64();
  }

  virtual string Name() const {
    return "externalsound";
  }

  virtual string LongName() const { 
    return "external feacat sound features";
  }

  virtual string ShortText() const {
    return "computes sound features using the external feacat program.";
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

  virtual bool CalculateCommon(int f, bool all, int l = -1) {
    bool ok = true;
    ExternalSoundData *data_dst = (ExternalSoundData *)GetData(l);
    if (!data_dst->FrameComputed(f)) 
      return External::CalculateCommon(f,all,l);
    return ok;
  }

  virtual string TargetType() const { return "sound"; }

  virtual pixeltype DefaultPixelType() const { return pixel_undef; }

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const;
  
  virtual int printMethodOptions(ostream&) const;

  virtual execchain_t GetExecChain(int f, bool all, int l);

  virtual bool ExecPostProcess(int /*f*/, bool /*all*/, int l);

  virtual bool CalculateOneFrame(int f);

  virtual bool SaveOutput() const { return true; }

  virtual featureVector AggregatePerFrameVector(size_t) const;

  // this function was overriden because we want to pass the frame number
  // to ResultVector() and AggregatePerFrameVector() functions, which isn't
  // done in the original DoPrinting() function.
  virtual bool DoPrinting(int s, int f);

  featureVector ResultVector(int idx, bool allow_zero_count, size_t f) const {
    ExternalSoundData *d = (ExternalSoundData*) GetData(idx);
    featureVector res = d->Result(allow_zero_count,f);
    if (!res.size())
      throw "Feature::ResultVector() : empty feature data element";
    
    return res;
  }
  
 
protected:  
  /** The ExternalSoundData objects stores the data needed for the 
      feature calculation */
  class ExternalSoundData : public ExternalData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    ExternalSoundData(pixeltype t, int n, const ExternalSound *p) : ExternalData(t, n, p) {
      //result = vectortype(Length());
      result.clear();
    }

    /// Defined because we have virtual member functions...
    virtual ~ExternalSoundData() {}

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "ExternalSoundData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    // this should be read from feacat output
    virtual int Length() const { return 13; } 

    /// Zeros the internal storage
    /*virtual void Zero() { 
      datavec = vectortype(datavec.size());
      result.clear();
      }*/

    virtual void AddValue(const vector<float>& v) {
      if ((int)v.size() % Length() != 0)
	throw "ExternalSoundData::AddValue(vector<float>) size mismatch";
      /*      transform(result[f].begin(), result[f].end(), v.begin(), 
	      result[f].begin(), plus<datatype>());*/
      vector<float>::const_iterator i;
      //cout << result.size() << endl;
      for (i=v.begin(); i!=v.end(); i+=Length()) {
	vectortype rv = vectortype(i,i+Length());
	//cout << result.size() << " " << rv.size() << " " << v.size() << endl;
	result.push_back(rv);
      }
    }

    virtual size_t ResultCount() {
      return result.size();
    }

    virtual void FillResults(size_t num) {
      vectortype v = result[result.size()-1];
      while(result.size() < num)
	result.push_back(v);
    }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool /*azc*/, size_t f) const {
      featureVector v(Length());

      if (FrameComputed(f))
	for (size_t i=0; i<v.size(); i++)
	  v[i] = result[f][i];
      //cout << "v: " << v.size() << endl;
      return v;
    }

    virtual bool FrameComputed(size_t f) const {
      //cout << result.size() << ", " << f << endl;
      return result.size() > f;
    }

    
    
  protected:
    vector<vectortype> result;

    //vectortype result;

  };

  string tempfilename;
  string datafilename;
  size_t videoframecount;
};
}
#endif

// Local Variables:
// mode: font-lock
// End:
