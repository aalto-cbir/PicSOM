// -*- C++ -*-  $Id: sfaudiofile.C,v 1.8 2013/02/25 11:02:49 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

//#ifdef USE_AUDIO

//#define SF_DEBUG 1

#include <sfaudiofile.h>

namespace picsom {
  using namespace std;
  
  sfaudiofile::sfaudiofile(const string& filename) {
    open(filename);
  }

  // ----------------------------------------------------------------------
    
  sfaudiofile::~sfaudiofile() {
#ifdef HAVE_SNDFILE_H
    if (sfile)
      sf_close(sfile);
#endif // HAVE_SNDFILE_H
  }

  // ----------------------------------------------------------------------

  bool sfaudiofile::open(const string& filename) {
#ifdef HAVE_SNDFILE_H
    SF_INFO sfinfo;
    sfinfo.format=0;
    sfile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (!sfile)
      return false;

    if ((sfinfo.format & SF_FORMAT_WAV) == 0) {
      cerr << "File format is not WAV (" << sfinfo.format << ")" << endl;
      return false;
    }

    // if (sfinfo.channels != 1) {
    //   cerr << "Currently only mono is supported! (channels=" 
    //        << sfinfo.channels << ")" << endl;
    //   //return false;
    // }
      
    samplerate = sfinfo.samplerate;
    num_channels = sfinfo.channels;
    samples = sfinfo.frames;
    duration = (long)floor((samples*1000)/(double)samplerate);
    initialized = true;

#ifdef SF_DEBUG
    cout << "sfaudiofile::open(" << filename << ")"
         << " samplerate=" << samplerate
         << " channels=" << num_channels
         << " samples=" << samples
         << " duration=" << duration
         << " sections=" << sfinfo.sections
         << endl;
#endif // SF_DEBUG

    return true;

#else
    cerr << "libsndfile not available for " << filename << endl;
    return false;
#endif // HAVE_SNDFILE_H
  }

  // ----------------------------------------------------------------------

  audiodata sfaudiofile::getNextAudioSlice(int duration, bool enablefft) {
#ifdef HAVE_SNDFILE_H
    bool force_mono = true, mono_by_avg = true;
    size_t nch = getNChannels();
    if (force_mono && nch>1)
      nch = 1;

    int bitspersample = sizeof(short)*8;
    audiodata aud((double)duration, getSampleRate(), nch,
                  bitspersample, enablefft);

    size_t read_count = 
      (size_t)floor((double)duration*(double)getSampleRate()/1000.0*
                    (double)getNChannels());
    short *buf = new short[read_count];
    size_t samples_read = sf_read_short(sfile, buf, read_count);
    size_t samples = samples_read;

    if (force_mono && getNChannels()>1) {
      samples = samples_read/getNChannels();
      short *tmp = buf;
      size_t res_size = read_count/getNChannels();
      buf = new short[res_size];
      for (size_t i=0; i<res_size; i++)
	if (mono_by_avg) {
	  int sum = 0;
	  for (size_t j=0; j<(size_t)getNChannels(); j++)
	    sum += tmp[i*getNChannels()+j];
	  buf[i] = sum/getNChannels();

	} else
	  buf[i] = tmp[i*getNChannels()];

      delete tmp;
    }

    int currentDuration = aud.pushData(buf, samples);
    current_pos += currentDuration;

#ifdef SF_DEBUG
    cout << "sfaudiofile::getNextAudioSlice(" << duration 
         << ", " << (enablefft?"true":"false") << "): " 
         << samples_read << " samples read"
	 << ", samples=" << samples
	 << ", read_count=" << read_count
         << ", current_pos=" << current_pos 
         << endl;
#endif // SF_DEBUG

    delete buf;

    return aud;

#else
    cerr << "libsndfile not available for " << duration
	 << " " << enablefft << endl;
    return false;
#endif // HAVE_SNDFILE_H
  }   

  // ----------------------------------------------------------------------
} // namespace picsom

//#endif // USE_AUDIO
