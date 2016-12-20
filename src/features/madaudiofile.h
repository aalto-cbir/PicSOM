// -*- C++ -*-  $Id: madaudiofile.h,v 1.12 2016/06/23 10:49:26 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _MADAUDIOFILE_H
#define _MADAUDIOFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <errno.h>

#include <soundfile_impl.h>
#include <audiodata.h>

#ifdef HAVE_MAD_H
#include <mad.h>
#endif // HAVE_MAD_H

#define INPUT_BUFFER_SIZE (5*8192)
#define OUTPUT_BUFFER_SIZE 8192
#define OUTPUT_BUFFER_SAFETY 8192

namespace picsom {
  using namespace std;

  class madaudiofile : public soundfile_impl {
    
  protected:
    /// The currently opened file
    FILE *file;

#ifdef HAVE_MAD_H
    /// The MAD audio file structures used to handle the audio stream
    struct mad_stream *Stream;
    struct mad_frame *Frame;
    struct mad_synth *Synth;
    mad_timer_t *Timer;
#endif // HAVE_MAD_H

    /// Output buffer for MAD
    u_int8_t *OutputBuffer; //[OUTPUT_BUFFER_SIZE];

    /// Pointer to the end of the output buffer
    u_int8_t *OutputBufferEnd;

    /// Input buffer for MAD
    u_int8_t *InputBuffer; //[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];

  public:
    /// default constructor:
    madaudiofile();

    /// default destructor:
    ~madaudiofile();

    virtual bool eof() {
      return reachedEOF && outputLength==0;
    }

    /// loads a given file
    virtual bool open(const string& filename);

    /// returns an audio slice of given duration
    audiodata getNextAudioSlice(int duration, bool enablefft = true);

    /// calculates the duration of the given audio file from the stream 
    ///headers
    long update_duration(FILE* f);

    int getNChannels() {
#ifdef HAVE_MAD_H
      return MAD_NCHANNELS(&Frame->header);
#else
      return 0;
#endif // HAVE_MAD_H
    }

    /// The following function is from the madlld package example code 
    /// by Bertrand Petit.
    /// Converts the MAD audio data numbers to signed short.
#ifdef HAVE_MAD_H
    static signed short MadFixedToSshort(mad_fixed_t Fixed);
#endif // HAVE_MAD_H

    /// Uses MAD to grab next audio frame.
    /// The function is loosely based on the example code in madlld package
    /// by Bertrand Petit.
    /// Returns true if successfull.
    bool getNextAudioFrame();

  }; // class audiofile

} // namespace picsom

#endif //_MADAUDIOFILE_H

