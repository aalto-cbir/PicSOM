// -*- C++ -*-  $Id: AudioBased.h,v 1.13 2014/01/23 13:03:15 jorma Exp $
// 
// Copyright 1998-2014 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file AudioBased.h

   \brief Declarations and definitions of class AudioBased
   
   AudioBased.h contains declarations and definitions of the class
   AudioBased, which is a base class for feature calculations that
   calculate features from audio objects.
  
   \author Hannes Muurinen <hannes.muurinen@hut.fi>
   $Revision: 1.13 $
   $Date: 2014/01/23 13:03:15 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

// compile only if we want to include audio libraries
//#ifdef USE_AUDIO

#ifndef _AudioBased_h_
#define _AudioBased_h_

#include <math.h>
#include <Feature.h>
#include "soundfile.h"
#include "audiodata.h"

namespace picsom {

  /// The default slice duration in milliseconds. Negative values means that
  /// we use the duration of the current audio file (that is, each file is 
  /// divided into excactly one slice).
#define DEFAULT_SLICE_DURATION 5000

  /// Base class for text features
  class AudioBased : public Feature {
  public:

    // -----------------------------------------------------------

    /// Returns the target type ("sound")
    virtual string TargetType() const { return "sound"; }

    // -----------------------------------------------------------

    virtual bool Incremental() const { return true; }

    // this used to return false until 2014-01-23, why???
    virtual bool IsAudioFeature() const { return true; }

    // -----------------------------------------------------------

    virtual featureVector CalculatePerFrame(int f, int ff, bool print) {
      bool ok = CalculateOneFrame(f);
      if (print && ok)
	DoPrinting(-1, ff);

      return AggregatePerFrameVector();
      //return featureVector();
    }

    // -----------------------------------------------------------

    virtual bool CalculateOneFrame(int f) {
      return CalculateCommon(f, true);
    }

    // -----------------------------------------------------------

    virtual bool CalculateOneLabel(int f, int l) {
      return CalculateCommon(f, false, l); 
    }

    // -----------------------------------------------------------

    virtual treat_type DefaultWithinFrameTreatment() const {
      return treat_concatenate; 
    }

    // -----------------------------------------------------------

    virtual treat_type DefaultBetweenFrameTreatment() const {
      return treat_separate; 
    }

    // -----------------------------------------------------------

    virtual treat_type DefaultBetweenSliceTreatment() const {
      return treat_separate; 
    }

    // -----------------------------------------------------------

    virtual pixeltype DefaultPixelType() const { return pixel_undef; }

    // -----------------------------------------------------------

    virtual featureVector CalculateRegion(const Region &/*r*/){
      AudioBasedData *d=
	(AudioBasedData*)CreateData(PixelType(),ConcatCountPerPixel(),0);
    
      cout << "AudioBased::CalculateRegion not implemented!!" << endl;

      bool allow_zero_count = true;
      return d->Result(allow_zero_count);
    }

    // -----------------------------------------------------------

    /** Loads the data.
	\param datafile the name of the object file
	\param segmentfile the name of the segmentation file (default "")
	\return true if data was successfully loaded, otherwise false.
    */
    virtual bool LoadObjectData(const string& datafile, 
				const string& segmentfile = "") { 
      return LoadAudioData(datafile, segmentfile);
    }

    // -----------------------------------------------------------

    /** This actually loads the data, called by the above function.
	\param datafile the name of the object file
	\param segmentfile the name of the segmentation file (default "")
	\return true if data was successfully loaded, otherwise false.
    */
    bool LoadAudioData(const string& datafile, 
		       const string& /*segmentfile*/) { 
      originalfilename = datafile;
      string f = datafile, mime = FileExtensionToMIMEtype(datafile);
      if (mime=="video/mp4" || mime=="video/x-msvideo") {
	videofile vf(datafile);
	tempaudio = f = vf.extract_audio();

	if (FileVerbose())
	  cout << "Extracted audio from <" << datafile << "> to <"
	       << tempaudio << ">" << endl;
      }
      auddat->open(f);

      return true;
    }

    // -----------------------------------------------------------

    /// all classes derived from this class aren't image features
    virtual bool IsImageFeature() const { return false; } 

    // -----------------------------------------------------------

    /// Pure virtual method -- each subclass must return its own vector size.
    virtual int FeatureVectorSize() const = 0;

    // -----------------------------------------------------------

    /// An audiofile contains Nframes() audio "frames" (or slices)
    virtual int Nframes() const { return AudioIntervalCount(); }

    // -----------------------------------------------------------

    /// Returns the file name of the audio file.
    virtual string ObjectDataFileName() const {
      return originalfilename;
      // return auddat->filename();
    }

    // -----------------------------------------------------------

    /// Returns the soundfile object of the currently loaded audio file.
    virtual soundfile *AudioFile() { return auddat; }
    virtual soundfile *AudioFile() const { return auddat; }

    // -----------------------------------------------------------

    /// Prints the command line options provided by the AudioBased class.
    virtual int printMethodOptions(ostream& = cout) const;

    // -----------------------------------------------------------
    // -----------------------------------------------------------
    // -----------------------------------------------------------

    /// Data class for audio based data
    class AudioBasedData : public Data {
    public:

      // -----------------------------------------------------------

      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      AudioBasedData(pixeltype t, int n, const Feature *p, soundfile *aud) 
	: Data(t, n, p) { 
	auddatptr = aud;
	result = featureVector();
      }

      // -----------------------------------------------------------

      /// Defined because we have virtual member functions...
      virtual ~AudioBasedData() {}

      // -----------------------------------------------------------

      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      virtual int Length() const { return result.size(); }

      // -----------------------------------------------------------

      /// Adds given float to the end of the result vector
      virtual void AddData(const float c ) {
	result.push_back(c);
      }

      // -----------------------------------------------------------

      /// Clears the result vector
      virtual void ClearData() {
	result.clear();
      }

      // -----------------------------------------------------------

      /** Returns the result of the feature extraction
	  \return feature result vector
      */
      virtual featureVector Result(bool) const {
	return result;
      }

      // -----------------------------------------------------------

      /// Returns the madaudiodata object of the currently loaded file
      soundfile *AudioData() {
	return auddatptr;
      }

      // -----------------------------------------------------------
    
      virtual int DataUnitSize() const { return exts; } 

      // -----------------------------------------------------------

      /// The name of the class
      virtual string Name() const { return "AudioBasedData";}

      // -----------------------------------------------------------

    protected:
      /// pointer to the soundfile object
      soundfile *auddatptr;

      /// the result vector
      featureVector result;

    }; // class AudioBasedData
  

    // -----------------------------------------------------------
    // -----------------------------------------------------------
    // -----------------------------------------------------------

    ///The protected stuff of the AudioBased class
  protected:

    // -----------------------------------------------------------

    /// This constructor is called by inherited class constructors.
    AudioBased() { 
      // sliceDuration = DEFAULT_SLICE_DURATION;
      sliceDuration = -1; // == infinite length
      auddat = new soundfile();
    } 

    // -----------------------------------------------------------

    /// This constructor doesn't add an entry in the methods list.
    AudioBased(bool) : Feature(false) { 
      /*sliceDuration = DEFAULT_SLICE_DURATION;
	auddat = new madaudiofile(); */
    }

    // -----------------------------------------------------------

    /// Destructor
    virtual ~AudioBased() {
      // obs! For some reason the destructor segfaults. Commented out for now:
      //delete auddat;
      if (tempaudio!="") {
	if (FileVerbose())
	  cout << "~AudioBased() unlinking <" << tempaudio << ">" << endl;
	Unlink(tempaudio);
      }
    }

    // -----------------------------------------------------------

    /// Processes the class specific command line arguments.
    virtual bool ProcessOptionsAndRemove(list<string>&);

    // -----------------------------------------------------------

    /// The duration of one audio slice in milliseconds.
    /// Returns the duration of the currently loaded audio file if duration <= 0
    virtual int IntervalDuration() {
      return sliceDuration > 0 ? sliceDuration : (int) auddat->getDuration(); 
    }
    virtual int IntervalDuration() const {
      return sliceDuration > 0 ? sliceDuration : (int) auddat->getDuration(); 
    }

    // -----------------------------------------------------------

    /// Returns the number of audio frames (slices) in the currently loaded
    /// file. If sliceDuration < 0, returns 1.
    int unsigned AudioIntervalCount() {
      if (sliceDuration <= 0)
	return 1;
      return (int unsigned) ceil((double)auddat->getDuration() / 
				 IntervalDuration());
    }
    int unsigned AudioIntervalCount() const {
      if (sliceDuration <= 0)
	return 1;
      return (int unsigned) ceil((double)auddat->getDuration() / 
				 IntervalDuration());
    }

    // -----------------------------------------------------------

    // f frame, all all segments, l segment data index
    virtual bool CalculateCommon(int f, bool all, int l = -1) {
      AudioPreProcess(f,all,l);

      if (FrameVerbose())
	cout << "AudioBased::CalculateCommon(" << f << "," << all << "," << l 
	     << ")" << endl;

      int s = 1;
      int j = -1;
      
      try {
	j = DataIndex(s);
      }
      catch (const string& str) {
	if (str.find(") not found") != string::npos) {}
      }

      if (j>=0)
	if (all || s==l)
	  CalculateOneInterval(0, (AudioBasedData*)GetData(j));

      AudioPostProcess(f,all,l);
      return true;
    }

    // -----------------------------------------------------------

    /// Pre-processing function for the audio data (subclasses can override)
    virtual bool AudioPreProcess(int /*f*/, bool /*all*/, int /*l*/) 
    { return true; }

    // -----------------------------------------------------------

    /// Post-processing functions for the audio data (subclasses can override)
    virtual bool AudioPostProcess(int /*f*/, bool /*all*/, int /*l*/) 
    { return true; }

    // -----------------------------------------------------------

    /// Calculates value for one audio interval of the audio file. Subclasses
    /// must override.
    virtual bool CalculateOneInterval(int i, AudioBasedData *d) = 0;

    // -----------------------------------------------------------

    /// Purely virtual function for data creation.
    virtual Feature::Data *CreateData(pixeltype t, int n, int) const = 0;

    // -----------------------------------------------------------

    /// Returns the data labels
    virtual vector<string> UsedDataLabels() const {
      vector<string> ret;

      char tmp[100];
      sprintf(tmp, "%d", 1);
      ret.push_back(tmp);

      if (LabelVerbose())
	ShowDataLabels("AudioBased::UsedDataLabels", ret, cout);
    
      return ret;
    }

    // -----------------------------------------------------------

  private:
    /// The audio file handling object
    soundfile *auddat;

    /// The duration of audio slices in milliseconds. Negative duration
    /// means that the whole audio clip is used.
    int sliceDuration; 

    ///
    string tempaudio;

    ///
    string originalfilename;

  }; // class AudioBased

} // namespace picsom

#endif // _AudioBased_h_

//#endif // USE_AUDIO

// Local Variables:
// mode: font-lock
// End:
