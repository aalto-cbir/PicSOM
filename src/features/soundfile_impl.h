// -*- C++ -*-  $Id: soundfile_impl.h,v 1.4 2016/06/23 10:49:26 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _SOUNDFILE_IMPL_H
#define _SOUNDFILE_IMPL_H

//#include <stdlib.h>
//#include <stdio.h>
//#include <string>
//#include <iostream>
//#include <fstream>
//#include <limits.h>
//#include <errno.h>
#include "audiodata.h"

namespace picsom {
  using namespace std;

  class soundfile_impl {
    
  protected:
    /// is the object ready to read audio data
    bool initialized;

    /// True if the currently opened file is finished
    bool reachedEOF;

    /// The number of channels in the audio stream
    int num_channels;
    
    /// The samplerate of the audio stream
    int unsigned samplerate;

    /// Current position in the audio stream
    int current_pos;

    /// number of audio frames read
    int frame_count;

    /// The duration of the audio stream in milliseconds
    long duration; // msec

    /// The name of the currently opened file
    string curr_filename;

    /// the amount of data currently in OuputBuffer
    size_t outputLength; // the amount of data currently in OutputBuffer

    /// the current position in OutputBuffer
    size_t outputPosition; // the next element to read at the OutputBuffer
    
  public:
    /// default constructor:
    soundfile_impl();

    /// default destructor:
    virtual ~soundfile_impl() {}

    /// returns true if stream is loaded and it has an audio stream
    bool hasAudio() {
      return initialized;
    }
    
    /// returns true if end of file has been reached
    virtual bool eof() {
      return reachedEOF;
    }

    /// loads a given file
    virtual bool open(const string& filename) = 0;

    /// Returns the file name of the currently opened audio file
    string filename() const {
      return curr_filename;
    }

    /// Reads a number of elements from the given file.
    /// Returns the true number of read bytes. 
    size_t readData(void* buffer, size_t elementSize, 
              	    size_t desiredElements, FILE* f);

    /// seeks to a given position in the audio stream
    bool audioSeekTo(int /*position*/) {
      printf("audioSeekTo() not yet implemented\n");
      return false;
    }

    /// returns an audio slice of given duration
    virtual audiodata getNextAudioSlice(int duration, 
                                        bool enablefft=true) = 0;

    /// returns an audio slice of given duration from the given offset onwards
    audiodata getAudioSlice(int position, int duration) {
      audioSeekTo(position);
      return getNextAudioSlice(duration);
    }

    /// Returns the duration of the audio file in milliseconds
    long getDuration() {
      return duration;
    }

    int unsigned getSampleRate() {
      return samplerate;
    }

    /// Returns the number of channels in the audio file
    virtual int getNChannels() {
      return num_channels;
    }

  }; // class soundfile_impl


} // namespace picsom

#endif // _SOUNDFILE_IMPL_H

