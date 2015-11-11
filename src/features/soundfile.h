// -*- C++ -*-  $Id: soundfile.h,v 1.6 2014/01/23 13:02:02 jorma Exp $
// 
// Copyright 1998-2014 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

// compile only if we want to include audio libraries
//#ifdef USE_AUDIO

#ifndef _SOUNDFILE_H
#define _SOUNDFILE_H

#include "sfaudiofile.h"
#include "madaudiofile.h"
#include "soundfile_impl.h"

namespace picsom {
  using namespace std;

  class soundfile {
  protected:
    soundfile_impl* sip;
    string fname;
    string impl;

  public:

    soundfile() {
    }

    soundfile(const string& filename) {
      open(filename);
    }

    virtual ~soundfile() {
      if (sip)
        delete sip;
    }

    bool open(const string& filename) {
      string ext = "";
      size_t dotpos = filename.find_last_of('.');
      if (dotpos != string::npos)
        ext = filename.substr(dotpos+1);

      impl = "none";
      if (ext=="wav") {
        impl = "libsndfile";
        sip = new sfaudiofile();
      } else if (ext=="mp3" /*|| ext=="mp4" || ext=="avi"*/) {
        impl = "madaudio";
        sip = new madaudiofile();
      } else {
        cerr << "Unknown sound extension: " << ext << endl;
        return false;
      }
      sip->open(filename);

      if (sip->hasAudio()) {
        fname = filename;
        return true;
      }

      return false;
    }

    const string& filename() { return fname; }
    
    const string& implementation() { return impl; }

    audiodata getNextAudioSlice(int duration, bool enablefft = true) {
      return sip->getNextAudioSlice(duration, enablefft);
    }
  
    long getDuration() { return sip ? sip->getDuration() : 0; }

    int unsigned getSampleRate() { return sip->getSampleRate(); }

    virtual int getNChannels() { return sip->getNChannels(); }
  
    bool hasAudio() { return sip->hasAudio(); }    

  }; // class soundfile

} // namespace picsom

#endif // _SOUNDFILE_H

//#endif // USE_AUDIO
