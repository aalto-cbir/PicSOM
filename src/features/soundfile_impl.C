// -*- C++ -*-  $Id: soundfile_impl.C,v 1.3 2013/02/25 11:04:08 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

//#ifdef USE_AUDIO

#include <soundfile_impl.h>

namespace picsom {
  using namespace std;
  
  soundfile_impl::soundfile_impl() {
    reachedEOF = true;
    initialized = false;
    current_pos = 0;
    frame_count = 0;
    outputLength = 0;
    outputPosition = 0;
    samplerate = 44100;
  }

// ----------------------------------------------------------------------

  size_t soundfile_impl::readData(void* buffer, size_t elementSize, 
                  size_t desiredElements, FILE* f) {
    size_t desiredBytes = elementSize*desiredElements;
    if(f == NULL || buffer == NULL || reachedEOF || desiredBytes < 1) {
      return 0;
    }
    size_t aquiredBytes = fread(buffer, 1, desiredBytes, f);
    if(aquiredBytes != desiredBytes) {
      if(feof(f))
        reachedEOF = true;
      else
        printf("soundfile_impl: error reading the file\n");
    }
    return aquiredBytes;
  }

// ----------------------------------------------------------------------

} // namespace picsom

//#endif // USE_AUDIO
