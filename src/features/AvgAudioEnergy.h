// -*- C++ -*-  $Id: AvgAudioEnergy.h,v 1.9 2014/01/23 13:03:44 jorma Exp $
// 
// Copyright 1998-2014 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file AvgAudioEnergy.h

   \brief Declarations and definitions of class AvgAudioEnergy
   
   AvgAudioEnergy.h contains declarations and definitions of class the 
   AvgAudioEnergy, which is a class that performs the average signal
   energy feature extraction from audio files.

  
   \author Hannes Muurinen <hannes.muurinen@hut.fi>
   $Revision: 1.9 $
   $Date: 2014/01/23 13:03:44 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo Perhaps something?
*/

// compile only if we want to include audio libraries
//#ifdef USE_AUDIO

#ifndef _AvgAudioEnergy_
#define _AvgAudioEnergy_

#include "AudioBased.h"

namespace picsom {
/// A class that performs the audio energy feature extraction.
class AvgAudioEnergy : public AudioBased {
 public:


  // -----------------------------------------------------------


  /// definition for the current data type
  typedef float datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;



  // -----------------------------------------------------------


  /** Constructor
      \param obj initialise to this object
      \param opt list of options to initialise to
  */
  AvgAudioEnergy(const string& obj = "", 
		 const list<string>& opt = (list<string>)0) {
    Initialize(obj, "", NULL, opt);
  }



  // -----------------------------------------------------------


  /// This constructor doesn't add an entry in the methods list.
  AvgAudioEnergy(bool) : AudioBased(false) { 
  }




  // -----------------------------------------------------------


  virtual Feature *Create(const string& s) const {
    return (new AvgAudioEnergy())->SetModel(s);
  }



  // -----------------------------------------------------------

  /// Creates the data object
  virtual Feature::Data *CreateData(pixeltype, int n, int /*i*/) const {
    return new AvgAudioEnergyData(DefaultPixelType(), n, this, 
				  AudioFile());
  }



  // -----------------------------------------------------------

  /// deletes the given data object
  virtual void deleteData(void *p){
    delete (AvgAudioEnergyData*)p;
  }




  // -----------------------------------------------------------

  /// Feature name
  virtual string Name()          const { return "avgaudioenergy"; }



  // -----------------------------------------------------------

  /// Feature name in longer format
  virtual string LongName()      const { return "Average audio energy"; }



  // -----------------------------------------------------------

  /// Feature description
  virtual string ShortText()     const {
    return 
      "An audio feature calculated from the energy of the audio signal.";}



  // -----------------------------------------------------------

  /// Version of the feature file
  virtual string Version() const;



  // -----------------------------------------------------------

  
  virtual featureVector getRandomFeatureVector() const { return featureVector(); }



  // -----------------------------------------------------------


  virtual string OptionSuffix() const {
    return "";
  }



  // -----------------------------------------------------------

  /// Prints the class-specific command line arguments
  virtual int printMethodOptions(ostream&) const;



  // -----------------------------------------------------------

  /// Returns the feature vector length
  virtual int FeatureVectorSize() const {
    return 1;
  }



  // -----------------------------------------------------------


protected:  



  // -----------------------------------------------------------


  /// Parses the class-specific command line arguments
  virtual bool ProcessOptionsAndRemove(list<string>&);




  // -----------------------------------------------------------
  // -----------------------------------------------------------
  // -----------------------------------------------------------


  /// The NGramData objects store the data needed for the feature calculation
  class AvgAudioEnergyData : public AudioBasedData {
  public:

    // -----------------------------------------------------------


    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    AvgAudioEnergyData(pixeltype t, int n,const Feature *p,
		       soundfile *aud)
      : AudioBasedData(t, n, p, aud) {
      energy = 0;
    }



    // -----------------------------------------------------------


    /// Defined because we have virtual member functions...
    ~AvgAudioEnergyData() {}



    // -----------------------------------------------------------


    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "AvgAudioEnergyData"; }



    // -----------------------------------------------------------


    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return 1; }



    // -----------------------------------------------------------


    /// += operator 
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "AvgAudioEnergyData::operator+=() not defined"; }



    // -----------------------------------------------------------


    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool) const {
      featureVector res(1);
      res[0] = energy;
      return res;
    }



    // -----------------------------------------------------------

    /// Sets the energy value
    virtual void AddData(const float val) {
      energy = val;

      if (PixelVerbose())
	cout << "AvgAudioEnergy::AddData(" << val << ")" << endl;
    }



    // -----------------------------------------------------------


    virtual void Zero() {}    

  private:
    /// The energy of the signal
    float energy;
  }; // class AvgAudioEnergyData




  // -----------------------------------------------------------
  // -----------------------------------------------------------
  // -----------------------------------------------------------

  /// Calculates the feature vector value for one audio frame (slice).
  virtual bool CalculateOneInterval(int /*i*/, AudioBasedData *d) {
    audiodata ad = AudioFile()->getNextAudioSlice( IntervalDuration() );

    short *data = ad.getData_short();
    int unsigned datalength = ad.getDataLength();
    float energy = 0;

    for(unsigned int j=0; j<datalength; j++) {
      float d = data[j]/(32*1024.0);
      // cout << j << " " << d << endl;
      energy += d*d;
    }

    // normalize with the sample duration:
    // energy = energy / datalength * ad.getSamplerate(); 
    energy = energy / datalength;

    d->AddData(energy);
    return true;
  }

private:

}; // class AvgAudioEnergy
}
#endif

//#endif //USE_AUDIO

// Local Variables:
// mode: font-lock
// End:
