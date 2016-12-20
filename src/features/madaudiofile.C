// -*- C++ -*-  $Id: madaudiofile.C,v 1.8 2016/06/23 10:49:26 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#define MADAUDIO_PRINT_DEBUG

#include <madaudiofile.h>

namespace picsom {
  using namespace std;
  
  madaudiofile::madaudiofile() {
#ifdef MADAUDIO_PRINT_DEBUG
    printf("Now in madaudiofile() default constructor\n");
#endif
    file = NULL;
    
    OutputBuffer = OutputBufferEnd = InputBuffer = NULL;
    reachedEOF = true;
    initialized = false;
    current_pos = 0;
    frame_count = 0;
    outputLength = 0;
    outputPosition = 0;
    samplerate = 44100;

#ifdef HAVE_MAD_H
    Stream = new struct mad_stream;
    Frame = new struct mad_frame;
    Synth = new struct mad_synth;
    Timer = new mad_timer_t;

    mad_stream_init(Stream);
    mad_frame_init(Frame);
    mad_synth_init(Synth);
    mad_timer_reset(Timer);

    OutputBuffer = new u_int8_t[OUTPUT_BUFFER_SIZE+OUTPUT_BUFFER_SAFETY];
    if (!OutputBuffer)
      throw "Madaudiofile: unrecoverable error: couldn't allocate memory";

    InputBuffer = new u_int8_t[INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];
    if (!InputBuffer)
      throw "Madaudiofile: unrecoverable error: couldn't allocate memory";
#endif // HAVE_MAD_H

    OutputBufferEnd = OutputBuffer+OUTPUT_BUFFER_SIZE;

#ifdef MADAUDIO_PRINT_DEBUG
    printf("New madaudiofile was created\n");
#endif
  }

  // ----------------------------------------------------------------------
    
  madaudiofile::~madaudiofile() {
    if (file)
      fclose(file);

    OutputBufferEnd = NULL;

#ifdef HAVE_MAD_H
    mad_synth_finish(Synth);
    mad_frame_finish(Frame);
    mad_stream_finish(Stream);

    delete Timer;
    delete Synth;
    delete Frame;
    delete Stream;
#endif // HAVE_MAD_H

    /*
      if(InputBuffer != NULL)
      delete [] InputBuffer;

      // for some reason this causes segfault:
      if(OutputBuffer != NULL)
      delete [] OutputBuffer; */
  }

  // ----------------------------------------------------------------------

  bool madaudiofile::open(const string& filename) {
#ifdef MADAUDIO_PRINT_DEBUG
    printf("madaudiofile: opening file %s\n",filename.c_str());
#endif
    if (file)
      fclose(file);

#ifdef HAVE_MAD_H
    mad_synth_finish(Synth);
    mad_frame_finish(Frame);
    mad_stream_finish(Stream);

    memset(OutputBuffer, 0, OUTPUT_BUFFER_SIZE + OUTPUT_BUFFER_SAFETY);
    memset(InputBuffer, 0, INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD);
      
    mad_stream_init(Stream);
    mad_frame_init(Frame);
    mad_synth_init(Synth);
    mad_timer_reset(Timer);
  
    Stream->error = (mad_error)0;
#endif // HAVE_MAD_H

    file = fopen(filename.c_str(),"r");

    if (file) {
      update_duration(file);
      initialized = true;
      reachedEOF = false;
      current_pos = 0;
      frame_count = 0;
      outputLength = 0;
      outputPosition = 0;
      samplerate = 44100;
      curr_filename = filename;
#ifdef MADAUDIO_PRINT_DEBUG
      printf("File %s opened, position = %i\n",filename.c_str(),
	     current_pos);
#endif
      return true;
    }
    else 
      throw(string("Could not open file <")+filename+">");

    return false;
  }
    
  // ----------------------------------------------------------------------

  /*    size_t madaudiofile::readData(void* buffer, size_t elementSize, 
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
	printf("madaudiofile.h: error reading the file\n");
	}
	return aquiredBytes;
	}*/

  // ----------------------------------------------------------------------

  audiodata madaudiofile::getNextAudioSlice(int duration, bool enablefft) {
    if (outputLength==0)
      getNextAudioFrame();

    audiodata aud((double)duration, samplerate, getNChannels(), 16, enablefft);
    int currentDuration = 0;

    /*      while(currentDuration < duration && outputLength != 0) {
    // Adds data until end_pos or until end of the frame has been reached.
    // Returns the last added pos+1*/
    currentDuration = aud.pushData(OutputBuffer, &outputPosition, 
				   outputLength, duration); 

    if (currentDuration < duration && outputPosition >= outputLength)
      getNextAudioFrame();

    //}
    current_pos += currentDuration;

    return aud;
  }

  // ----------------------------------------------------------------------

  long madaudiofile::update_duration(FILE *f) {
    bool oldEOF = reachedEOF;
    reachedEOF = false;
    current_pos = 0;
    frame_count = 0;
    outputLength = 0;
    outputPosition = 0;
    //      samplerate = 44100;
    samplerate = 16000;

    //bool firstRun = true;

    if (!f)
      throw "madaudiofile::update_duration(): file is not opened\n";

    fpos_t oldPosition;
    fgetpos(f,&oldPosition);
    fseek(f, 0, SEEK_SET);

    /*
      struct mad_header m_header;
      mad_timer_reset(Timer);

      while(!reachedEOF) {

      if(Stream->buffer==NULL || Stream->error==MAD_ERROR_BUFLEN || 
      firstRun) {
      firstRun = false;
      size_t ReadSize,Remaining;
      unsigned char *ReadStart;
      u_int8_t *GuardPtr = NULL;
      if(Stream->next_frame!=NULL) {
      Remaining=Stream->bufend-Stream->next_frame;
      memmove(InputBuffer,Stream->next_frame,Remaining);
      ReadStart=InputBuffer+Remaining;
      ReadSize=INPUT_BUFFER_SIZE-Remaining;
      }
      else {
      ReadSize=INPUT_BUFFER_SIZE;
      ReadStart=InputBuffer;
      Remaining=0;
      }

      ReadSize = readData(ReadStart,1,ReadSize,f);
      if(ReadSize <= 0)
      throw "Madaudiofile: could not read from file\n";

      if(reachedEOF) {
      GuardPtr=ReadStart+ReadSize;
      memset(GuardPtr,0,MAD_BUFFER_GUARD);
      ReadSize+=MAD_BUFFER_GUARD;
      }
      mad_stream_buffer(Stream,InputBuffer,ReadSize);
      }

      while(1) {
      if( mad_header_decode(&m_header, Stream) == -1 ) {
      if( MAD_RECOVERABLE(Stream->error) ) {
      continue;
      } else if( Stream->error == MAD_ERROR_BUFLEN ) {
      break; // EOF 
      } else {
      break; // BAD ERROR, oh well 
      }
      }
      mad_timer_add(Timer, m_header.duration);
      }
      }  
    */
    double dur = 0;
    /*while(!eof()) */{
      audiodata ad = getNextAudioSlice(1000, false);
      dur += ad.getDuration();
    }
    duration = (long int) dur;
            
    fsetpos(f,&oldPosition);


    /*
      duration = mad_timer_count(*Timer, MAD_UNITS_MILLISECONDS);
    
      mad_header_finish(&m_header);
    */

#ifdef HAVE_MAD_H
    mad_synth_finish(Synth);
    mad_frame_finish(Frame);
    mad_stream_finish(Stream);
      
    memset(OutputBuffer, 0, OUTPUT_BUFFER_SIZE);
    memset(InputBuffer, 0, INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD);
      
    mad_stream_init(Stream);
    mad_frame_init(Frame);
    mad_synth_init(Synth);
    mad_timer_reset(Timer);

    Stream->error=(mad_error)0;
#endif // HAVE_MAD_H

    reachedEOF = oldEOF;

#ifdef MADAUDIO_PRINT_DEBUG
    printf("Got duration %li\n",duration);
#endif
    return duration;
  }

  // ----------------------------------------------------------------------

#ifdef HAVE_MAD_H
  signed short madaudiofile::MadFixedToSshort(mad_fixed_t Fixed) {
    /* A fixed point number is formed of the following bit pattern:
     *
     * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
     * MSB                          LSB
     * S ==> Sign (0 is positive, 1 is negative)
     * W ==> Whole part bits
     * F ==> Fractional part bits
     *
     * This pattern contains MAD_F_FRACBITS fractional bits, one
     * should alway use this macro when working on the bits of a fixed
     * point number. It is not guaranteed to be constant over the
     * different platforms supported by libmad.
     *
     * The signed short value is formed, after clipping, by the least
     * significant whole part bit, followed by the 15 most significant
     * fractional part bits. Warning: this is a quick and dirty way to
     * compute the 16-bit number, madplay includes much better
     * algorithms.
     */
      
    /* Clipping */
    if(Fixed>=MAD_F_ONE)
      return(SHRT_MAX);
    if(Fixed<=-MAD_F_ONE)
      return(-SHRT_MAX);
      
    /* Conversion. */
    Fixed=Fixed>>(MAD_F_FRACBITS-15);
    return((signed short)Fixed);
  }
#endif // HAVE_MAD_H
    
  // ----------------------------------------------------------------------

  bool madaudiofile::getNextAudioFrame() {
#ifdef HAVE_MAD_H
#ifdef MADAUDIO_PRINT_DEBUG
    printf("Now in getNextAudioFrame()\n");
#endif      
    u_int8_t *OutputPtr = OutputBuffer;
    u_int8_t *GuardPtr = NULL;

    while(OutputPtr < OutputBufferEnd) {
      /* The input bucket must be filled if it becomes empty or if
       * it's the first execution of the loop.
       */
      if(Stream->buffer==NULL || Stream->error==MAD_ERROR_BUFLEN || 
	 frame_count == 0) {
	size_t ReadSize,Remaining;
	unsigned char *ReadStart;
          
	/* {2} libmad may not consume all bytes of the input
	 * buffer. If the last frame in the buffer is not wholly
	 * contained by it, then that frame's start is pointed by
	 * the next_frame member of the Stream structure. This
	 * common situation occurs when mad_frame_decode() fails,
	 * sets the stream error code to MAD_ERROR_BUFLEN, and
	 * sets the next_frame pointer to a non NULL value. (See
	 * also the comment marked {4} bellow.)
	 *
	 * When this occurs, the remaining unused bytes must be
	 * put back at the beginning of the buffer and taken in
	 * account before refilling the buffer. This means that
	 * the input buffer must be large enough to hold a whole
	 * frame at the highest observable bit-rate (currently 448
	 * kb/s). XXX=XXX Is 2016 bytes the size of the largest
	 * frame? (448000*(1152/32000))/8
	 */
	if(Stream->next_frame!=NULL) {
	  Remaining=Stream->bufend-Stream->next_frame;
	  memmove(InputBuffer,Stream->next_frame,Remaining);
	  ReadStart=InputBuffer+Remaining;
	  ReadSize=INPUT_BUFFER_SIZE-Remaining;
	}
	else
	  ReadSize=INPUT_BUFFER_SIZE,
	    ReadStart=InputBuffer,
	    Remaining=0;
          
	/* Fill-in the buffer. If an error occurs print a message
	 * and leave the decoding loop. If the end of stream is
	 * reached we also leave the loop but the return status is
	 * left untouched.
	 */
	ReadSize=readData(ReadStart,1,ReadSize,file);
	if(ReadSize<=0) {
	  break; // probably end of file reached and nothing was read
	}
          
	/* {3} When decoding the last frame of a file, it must be
	 * followed by MAD_BUFFER_GUARD zero bytes if one wants to
	 * decode that last frame. When the end of file is
	 * detected we append that quantity of bytes at the end of
	 * the available data. Note that the buffer can't overflow
	 * as the guard size was allocated but not used the the
	 * buffer management code. (See also the comment marked
	 * {1}.)
	 *
	 * In a message to the mad-dev mailing list on May 29th,
	 * 2001, Rob Leslie explains the guard zone as follows:
	 *
	 *    "The reason for MAD_BUFFER_GUARD has to do with the
	 *    way decoding is performed. In Layer III, Huffman
	 *    decoding may inadvertently read a few bytes beyond
	 *    the end of the buffer in the case of certain invalid
	 *    input. This is not detected until after the fact. To
	 *    prevent this from causing problems, and also to
	 *    ensure the next frame's main_data_begin pointer is
	 *    always accessible, MAD requires MAD_BUFFER_GUARD
	 *    (currently 8) bytes to be present in the buffer past
	 *    the end of the current frame in order to decode the
	 *    frame."
	 */
	if(reachedEOF) {
	  GuardPtr=ReadStart+ReadSize;
	  memset(GuardPtr,0,MAD_BUFFER_GUARD);
	  ReadSize+=MAD_BUFFER_GUARD;
	}
          
	/* Pipe the new buffer content to libmad's stream decoder
	 * facility.
	 */
	mad_stream_buffer(Stream,InputBuffer,ReadSize+Remaining);
	Stream->error=(mad_error)0;
      }
        
      /* Decode the next MPEG frame. The streams is read from the
       * buffer, its constituents are break down and stored the the
       * Frame structure, ready for examination/alteration or PCM
       * synthesis. Decoding options are carried in the Frame
       * structure from the Stream structure.
       *
       * Error handling: mad_frame_decode() returns a non zero value
       * when an error occurs. The error condition can be checked in
       * the error member of the Stream structure. A mad error is
       * recoverable or fatal, the error status is checked with the
       * MAD_RECOVERABLE macro.
       *
       * {4} When a fatal error is encountered all decoding
       * activities shall be stopped, except when a MAD_ERROR_BUFLEN
       * is signaled. This condition means that the
       * mad_frame_decode() function needs more input to complete
       * its work. One shall refill the buffer and repeat the
       * mad_frame_decode() call. Some bytes may be left unused at
       * the end of the buffer if those bytes forms an incomplete
       * frame. Before refilling, the remaining bytes must be moved
       * to the beginning of the buffer and used for input for the
       * next mad_frame_decode() invocation. (See the comments
       * marked {2} earlier for more details.)
       *
       * Recoverable errors are caused by malformed bit-streams, in
       * this case one can call again mad_frame_decode() in order to
       * skip the faulty part and re-sync to the next frame.
       */
      if(mad_frame_decode(Frame,Stream)) {
	if(MAD_RECOVERABLE(Stream->error)) {
	  /* Do not print a message if the error is a loss of
	   * synchronization and this loss is due to the end of
	   * stream guard bytes. (See the comments marked {3}
	   * supra for more informations about guard bytes.)
	   */
	  if(Stream->error!=MAD_ERROR_LOSTSYNC ||
	     Stream->this_frame!=GuardPtr) {
	  }
	  continue;
	}
	else
	  if(Stream->error==MAD_ERROR_BUFLEN)
	    continue; // end of buffer reached --> read more from the file
	  else {
	    fprintf(stderr,
		    "madaudiofile: unrecoverable frame level error (%i).\n",
		    Stream->error);
	    break;
	  }
      }

        
      if(frame_count == 0) {
	samplerate = Frame->header.samplerate;
#ifdef MADAUDIO_PRINT_DEBUG
	printf("Initially setting samplerate to %i\n",samplerate);
#endif
      }
#ifdef MADAUDIO_PRINT_DEBUG
      if(Frame->header.samplerate != samplerate)
	printf("Obs: samplerate changed to %i!\n",Frame->header.samplerate);
#endif        
      /* Accounting. The computed frame duration is in the frame
       * header structure. It is expressed as a fixed point number
       * whole data type is mad_timer_t. It is different from the
       * samples fixed point format and unlike it, it can't directly
       * be added or subtracted. The timer module provides several
       * functions to operate on such numbers. Be careful there, as
       * some functions of libmad's timer module receive some of
       * their mad_timer_t arguments by value!
       */
      frame_count++;
      mad_timer_add(Timer,Frame->header.duration);
        
      /* Once decoded the frame is synthesized to PCM samples. No errors
       * are reported by mad_synth_frame();
       */
      mad_synth_frame(Synth,Frame);
        
      /* Synthesized samples must be converted from libmad's fixed
       * point number to the consumer format. Here we use unsigned
       * 16 bit big endian integers on two channels. Integer samples
       * are temporarily stored in a buffer that is flushed when
       * full.
       */
      int unsigned sr = 0;
      for(int i=0;i<Synth->pcm.length;i++) {
	signed short    Sample;

	u_int8_t left[2];
	u_int8_t right[2];
	/* Left channel */
	Sample=MadFixedToSshort(Synth->pcm.samples[0][i]);
	left[0]=Sample>>8;
	left[1]=Sample&0xff;
            
	/* Right channel. If the decoded stream is monophonic then
	 * the right output channel is the same as the left one.
	 */
	if(MAD_NCHANNELS(&Frame->header)==2)
	  Sample=MadFixedToSshort(Synth->pcm.samples[1][i]);
	right[0]=Sample>>8;
	right[1]=Sample&0xff;

	while(sr < samplerate) {
	  /* Left channel */
	  *(OutputPtr++)=left[1];
	  *(OutputPtr++)=left[0];
            
	  /* Right channel. If the decoded stream is monophonic then
	   * the right output channel is the same as the left one.
	   */
	  *(OutputPtr++)=right[1];
	  *(OutputPtr++)=right[0];

	  sr += Synth->pcm.samplerate;
	}
	sr -= samplerate;
      }
        
      if (OutputPtr>=OutputBufferEnd+OUTPUT_BUFFER_SAFETY) {
	char cstr[200];
	sprintf(cstr,"getNextAudioFrame() writing outside of OutputBuffer:"
		" %p >= %p VERY BAD!!!\n", OutputPtr,
		OutputBufferEnd+OUTPUT_BUFFER_SAFETY);
	throw(string(cstr));
      }
        
    } // while(OutputPtr < OutputBufferEnd)
    outputLength = OutputPtr-OutputBuffer;
    outputPosition = 0;

    if (outputLength == 0)
      reachedEOF = true;

    // If we have breaked out of the loop, but it wasn't because of EOF,
    // return false.
    if (OutputPtr < OutputBufferEnd && !reachedEOF) 
      return false;

    return true;
#else
    cerr << "libmad not available" << endl;
    return false;
#endif // HAVE_MAD_H
  }

  // ----------------------------------------------------------------------
} // namespace picsom

