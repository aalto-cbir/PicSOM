// -*- C++ -*-  $Id: AudioFreq.h,v 1.10 2013/02/25 11:06:14 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file AudioFreq.h

   \brief Declarations and definitions of class AudioFreq
   
   AudioFreq.h contains declarations and definitions of class the 
   AudioFreq, which is a class that calculates the audio frequency
   distribution feature for audio files. The distribution is a 
   crude distribution vector, which contains six values that
   represent different bands of the spectrum.

  
   \author Hannes Muurinen <hannes.muurinen@hut.fi>
   $Revision: 1.10 $
   $Date: 2013/02/25 11:06:14 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo Perhaps something?
*/

// compile only if we want to include audio libraries
//#ifdef USE_AUDIO

#ifndef _AudioFreq_
#define _AudioFreq_

#include "AudioBased.h"

// The feature vector contains energy of the audio signal in the following
// five bands:
// 0 .. F/16,
// F/16 .. F/8,
// F/8 .. F/4,
// F/4 .. F/2,
// F/2 .. F and
// F .. the rest of the data,
// where F is AUDIOFREQ_DEFAULT_NYQUIST_FREQ.

#define AUDIOFREQ_DEFAULT_NYQUIST_FREQ 22050

namespace picsom {
  /// A class that performs the audio energy feature extraction.
  class AudioFreq : public AudioBased {
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
    AudioFreq(const string& obj = "", 
	      const list<string>& opt = (list<string>)0) {
      Initialize(obj, "", NULL, opt);
    }

    // -----------------------------------------------------------

    /// This constructor doesn't add an entry in the methods list.
    AudioFreq(bool) : AudioBased(false) { 
    }

    // -----------------------------------------------------------

    virtual Feature *Create(const string& s) const {
      return (new AudioFreq())->SetModel(s);
    }

    // -----------------------------------------------------------

    /// Creates the data object
    virtual Feature::Data *CreateData(pixeltype, int n, int /*i*/) const {
      return new AudioFreqData(DefaultPixelType(), n, this, 
			       AudioFile());
    }

    // -----------------------------------------------------------

    /// Deletes the given data object
    virtual void deleteData(void *p){
      delete (AudioFreqData*)p;
    }

    // -----------------------------------------------------------

    /// The name of the feature
    virtual string Name()          const { return "audiofreq"; }

    // -----------------------------------------------------------

    /// Long name of the feature
    virtual string LongName()      const { 
      return "Audio signal frequency distribution"; 
    }

    // -----------------------------------------------------------

    /// Feature description
    virtual string ShortText()     const {
      return "An audio feature calculated from the FFT of the signal.";
    }

    // -----------------------------------------------------------

    /// Returns the class version
    virtual string Version() const;

    // -----------------------------------------------------------

  
    virtual featureVector getRandomFeatureVector() const {
      return featureVector();
    }

    // -----------------------------------------------------------

  
    virtual string OptionSuffix() const {
      return "";
    }

    // -----------------------------------------------------------

    /// Prints the class-specific command line arguments
    virtual int printMethodOptions(ostream&) const;

    // -----------------------------------------------------------

    /// The feature vector size
    virtual int FeatureVectorSize() const {
      return 6;
    }

  protected:  

    // -----------------------------------------------------------

    /// Parses the class-specific command line arguments
    virtual bool ProcessOptionsAndRemove(list<string>&);

    // -----------------------------------------------------------
    // -----------------------------------------------------------
    // -----------------------------------------------------------

    /// The NGramData objects store the data needed for the feature calculation
    class AudioFreqData : public AudioBasedData {
    public:

      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      AudioFreqData(pixeltype t, int n,const Feature *p,
		    soundfile *aud)
	: AudioBasedData(t, n, p, aud) {
      }

      // -----------------------------------------------------------

      /// Defined because we have virtual member functions...
      ~AudioFreqData() {}

      // -----------------------------------------------------------

      /** Returns the name of the feature
	  \return feature name
      */    
      virtual string Name() const { return "AudioFreqData"; }

      // -----------------------------------------------------------

      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      virtual int Length() const { return 6; }

      // -----------------------------------------------------------

      /// += operator 
      virtual Data& operator+=(const Data &) {
	// this should check that typeof(d) == typeof(*this) !!!
	throw "AudioFreqData::operator+=() not defined"; }

      // -----------------------------------------------------------

      virtual void Zero() {}

    }; // class AudioFreqData

    // -----------------------------------------------------------
    // -----------------------------------------------------------
    // -----------------------------------------------------------

    /// Calculates the 6-element feature vector for one audio interval.
    virtual bool CalculateOneInterval(int /*i*/, AudioBasedData *d) {
      bool log_value = true;

      audiodata ad = AudioFile()->getNextAudioSlice( IntervalDuration() );

      double freqmax = ad.getSamplerate() / 2.0; // nyquist frequency

      fftw_complex *fftdata = ad.FFT();
      int unsigned datalength = ad.getFFTLength();

      d->ClearData();

      vector<double> minv {
	0,
	  AUDIOFREQ_DEFAULT_NYQUIST_FREQ/16.0,
	  AUDIOFREQ_DEFAULT_NYQUIST_FREQ/8.0,
	  AUDIOFREQ_DEFAULT_NYQUIST_FREQ/4.0,
	  AUDIOFREQ_DEFAULT_NYQUIST_FREQ/2.0,
	  AUDIOFREQ_DEFAULT_NYQUIST_FREQ
	  };
	  
      vector<double> maxv(minv.begin()+1, minv.end());
      maxv.push_back(-1);

      for (size_t i=0; i<6; i++) {
	double min = minv[i], max = maxv[i];
	// cout << "min=" << min << " max=" << max << endl;
	double v = fftEnergyOnInterval(fftdata, datalength, freqmax, min, max);

	if (log_value && v>0)
	  v = log10(v);

	if (v<=0)
	  v = 0;

	d->AddData(v);
      }

      // d->AddData(fftEnergyOnInterval(fftdata, datalength, freqmax,
      // 		 		      0, AUDIOFREQ_DEFAULT_NYQUIST_FREQ/16.0));

      // d->AddData(fftEnergyOnInterval(fftdata, datalength, freqmax,
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ/16.0, 
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ/8.0));

      // d->AddData(fftEnergyOnInterval(fftdata, datalength, freqmax,
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ/8.0, 
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ/4.0));

      // d->AddData(fftEnergyOnInterval(fftdata, datalength, freqmax,
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ/4.0, 
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ/2.0));

      // d->AddData(fftEnergyOnInterval(fftdata, datalength, freqmax,
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ/2.0, 
      // 		 		      AUDIOFREQ_DEFAULT_NYQUIST_FREQ));

      // d->AddData(fftEnergyOnInterval(fftdata, datalength, freqmax,
      // 				      AUDIOFREQ_DEFAULT_NYQUIST_FREQ, -1));

      return true;
    }

    // -----------------------------------------------------------

  private:
  
    /// Calculates the average signal spectral energy on the given frequency 
    /// interval.
    /// If upperbound is negative, the frequency interval is from
    /// lowerbound to the end of the fft data.
    float fftEnergyOnInterval(fftw_complex *data, int unsigned datalength,
			      double maxfreq, 
			      double lowerbound, double upperbound) {
      float energy = 0;

      int unsigned idxmin = fftFrequencyToIndex(lowerbound,datalength,maxfreq);
      int unsigned idxmax = upperbound >= 0 ? 
	fftFrequencyToIndex(upperbound,datalength,maxfreq) : datalength;
      
      for (int unsigned i=idxmin; i<idxmax && i < datalength; i++) {
	energy += data[i][0]*data[i][0] + data[i][1]*data[i][1];
	//energy += sqrt(data[i][0]*data[i][0] + data[i][1]*data[i][1]);
      }

      if (upperbound < 0)
	upperbound = maxfreq;

      // average over the frequency interval and normalize with the sample 
      // duration:
      double divider = (upperbound - lowerbound) * IntervalDuration() / 1000; 
      if (divider != 0)
	energy /= divider; 

      return energy;
    }

    // -----------------------------------------------------------

    /// Converts the index of the fft array with given parameters to a frequency
    double fftIndexToFrequency(int unsigned idx, int unsigned fftlength, 
			       double maxfreq) {
      return (double)idx / (double)fftlength * maxfreq;
    }

    // -----------------------------------------------------------

    /// Converts the index of the fft array with given parameters to a frequency
    int unsigned fftFrequencyToIndex(double freq, int unsigned fftlength, 
				     double maxfreq) {
      return (int unsigned) floor( (freq / maxfreq * (double)fftlength) );
    }

  }; // class AudioFreq
}
#endif // _AudioFreq_

//#endif // USE_AUDIO
// Local Variables:
// mode: font-lock
// End:
