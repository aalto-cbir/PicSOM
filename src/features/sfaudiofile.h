// -*- C++ -*-  $Id: sfaudiofile.h,v 1.4 2016/06/23 10:49:26 jorma Exp $
// 
// Copyright 1998-2016 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _SFAUDIOFILE_H
#define _SFAUDIOFILE_H

// #define SF_DEBUG 1

#include <string>
#include <iostream>

#include <soundfile_impl.h>
#include <audiodata.h>

#ifdef HAVE_SNDFILE_H
#include <sndfile.h>
#else
typedef void SNDFILE;
typedef size_t sf_count_t;
#endif // HAVE_SNDFILE_H

namespace picsom {
  using namespace std;

  class sfaudiofile : public soundfile_impl {
    
  protected:
    SNDFILE *sfile;
    sf_count_t samples;

  public:
    /// default constructor:
    sfaudiofile() {}

    /// constructor with filename
    sfaudiofile(const string& filename);

    /// default destructor:
    ~sfaudiofile();

    /// loads a given file
    virtual bool open(const string& filename);

    virtual audiodata getNextAudioSlice(int duration, bool enablefft=true);

  }; // class audiofile

} // namespace picsom

#endif // _SFAUDIOFILE_H

