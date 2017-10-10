// -*- C++ -*-  $Id: audiodata.h,v 1.15 2017/04/28 07:48:19 jormal Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

// compile only if we want to include audio libraries

#ifndef _AUDIODATA_H
#define _AUDIODATA_H

// #include <missing-c-utils.h>
#include <picsom-config.h>

#include <stdio.h>
#include <complex>
#include <iostream>
#include <string>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

#ifdef HAVE_AUDIOFILE_H
#include <audiofile.h>
#endif // HAVE_AUDIOFILE_H

#ifdef HAVE_FFTW3_H
#include <fftw3.h>
#else
static void fftw3_missing(const std::string& f) {
  std::cerr << f << " failing because of missing libfftw3" << std::endl;
}
typedef char fftw_plan;
typedef double* fftw_complex;
#endif // HAVE_FFTW3_H

#define MEM_ALLOCATION_STEP_SIZE_MSEC 1000.0

#define XINE_ENABLE_EXPERIMENTAL_FEATURES

/// additional audio buffer size beyond the requested audio length 
/// (just for testing purpose -- by default set to 0)
#define BUFFER_LENGTH_MSEC 0.0

// #define AUDIODATA_PRINT_DEBUG

namespace picsom {
  using namespace std;

  typedef uint8_t u_int8_t;

  class audiodata {
  protected:
    /// the audio slice duration
    double duration;

    /// the samplerate of the data
    int samplerate;

    /// number of audio channels (currently supported: 1 or 2)
    int channels;

    /// number of bits per sample (typically 8 or 16)
    int bitspersample;

    /// number of bytes per millisecond of audio data
    double bytespermillisec;

    /// the current position in the data
    long currentpos;

    /// pointer to the audio data
    u_int8_t *data;

    /// the data length in bytes
    long datalength;

    /// the true data length in 16-bit words
    long truedatalength;

    /// fft plan (the plan is executed for the data)
    fftw_plan fftplan;

    /// pointer to fft input data
    double *fftinput;

    /// pointer to fft output data
    fftw_complex *fftoutput;

    /// True if we want to use fft (the user can disable fft in the 
    /// constructor, which will slightly improve the performance of the
    /// program since no futile computations are made)
    bool fft_enabled;

    // ----------------------------------------------------------------------

    /// Allocates more memory
    void allocateMoreMemory() {
      long newdatalength = (long)((double)datalength + 
                                  MEM_ALLOCATION_STEP_SIZE_MSEC *
                                  bytespermillisec);
#ifdef AUDIODATA_PRINT_DEBUG
	cout << "audiodata::allocateMoreMemory() allocating "
	     << datalength << " + " << MEM_ALLOCATION_STEP_SIZE_MSEC
	     << " * " << bytespermillisec << " = " << newdatalength
	     << " bytes" << endl;
#endif // AUDIODATA_PRINT_DEBUG

      u_int8_t *newdata = new u_int8_t[newdatalength];
      memcpy(newdata, data, datalength);
      if (data)
        delete [] data;
      data = newdata;
      datalength = newdatalength;

      truedatalength = datalength*8/bitspersample/channels;

      if (fft_enabled) {
#ifdef HAVE_FFTW3_H
        fftw_destroy_plan(fftplan);

	delete [] fftinput;
	delete [] fftoutput;

        fftinput = new double[ truedatalength ];
        fftoutput = new fftw_complex[ truedatalength / 2 + 1 ];

        fftplan = fftw_plan_dft_r2c_1d((int)truedatalength, fftinput, 
                                       fftoutput, FFTW_ESTIMATE);
#else
	fftw3_missing("audiodata::allocateMoreMemory()");
#endif // HAVE_FFTW3_H
      }

#ifdef AUDIODATA_PRINT_DEBUG
      printf("audiodata::allocateMoreMemory() reallocated with %ld bytes\n",
	     newdatalength);
#endif
    }

  public:

    // ----------------------------------------------------------------------

    /** Constructor
        \param d duration in milliseconds.
        \param sr samplerate in Hz.
        \param c number of channels.
        \param bps bits per sample (typically 8 or 16).
        \param enablefft true if you want to use FFT (causes a minor hit in 
        performance time)
    */
    audiodata(double d = 0, int sr = 0, int c = 0, int bps = 0, 
              bool enablefft = true) {
      duration = 0;
      samplerate = sr;
      channels = c;
      bitspersample = bps;
      fft_enabled = enablefft;

      fftinput = NULL;
      fftoutput = NULL;

      if (channels > 2)
        throw("ERROR: Audiodata -- max 2 audio channels supported\n");

      currentpos = 0;

#ifdef AUDIODATA_PRINT_DEBUG
      printf("new audiodata with d=%f, sr=%i, c=%i, bps=%i\n",
             d,samplerate,channels,bitspersample);
#endif // AUDIODATA_PRINT_DEBUG

      if (samplerate == 0 || channels == 0 || bitspersample == 0) {
        datalength = -1;
        return;
      }
      
      bytespermillisec = (double)samplerate * (double)channels * 
        (double)bitspersample / 8.0 / 1000.0;

      datalength = (long int) ( (d + BUFFER_LENGTH_MSEC) * bytespermillisec);

      data = new u_int8_t[ datalength ];

#ifdef AUDIODATA_PRINT_DEBUG
      printf("Allocated %ld bytes.\n",datalength);
#endif // AUDIODATA_PRINT_DEBUG

      truedatalength = datalength*8/bitspersample/channels;

      if (fft_enabled) {
#ifdef HAVE_FFTW3_H
        fftinput = new double[ truedatalength ];
        fftoutput = new fftw_complex[ truedatalength / 2 + 1 ];

        fftplan = fftw_plan_dft_r2c_1d((int)truedatalength, fftinput, 
                                       fftoutput, FFTW_ESTIMATE);
#else
	fftw3_missing("audiodata::audiodata()");
#endif // HAVE_FFTW3_H
      }

      if (data == NULL)
        datalength = -1;
    }

    // ----------------------------------------------------------------------

    /// destructor
    ~audiodata() {
      if (datalength != -1)
        delete [] data;

#ifdef HAVE_FFTW3_H
      if (fftinput && fftoutput)
        fftw_destroy_plan(fftplan);
#else
      fftw3_missing("audiodata::~audiodata()");
#endif // HAVE_FFTW3_H

      delete [] fftinput;
      delete [] fftoutput;
    }

    // ----------------------------------------------------------------------

    /// Pushes given data into the buffer. Updates duration accordingly
    int pushData(u_int8_t *buffer, size_t *startIdx, size_t bufferSize, 
                 int desiredDuration) {
      int cpylength = (int) (((double) (desiredDuration))*bytespermillisec);
      if (cpylength > (int) (bufferSize - (*startIdx))) {
        cpylength = bufferSize - (*startIdx);
      }

      while(currentpos + cpylength > datalength)
        allocateMoreMemory();

#ifdef AUDIODATA_PRINT_DEBUG
      printf("pushData() %d bytes (1).\n", cpylength);
#endif // AUDIODATA_PRINT_DEBUG

      memcpy(data + currentpos, buffer + *startIdx, cpylength);

      currentpos += cpylength;
      *startIdx += cpylength;
      duration = currentpos / bytespermillisec;

      return (int) duration;
    }

    // ----------------------------------------------------------------------

    /// Pushes given data into the buffer.
    int pushData(short *buffer, size_t bufferSize) {
      int cpylength = bufferSize*sizeof(short)/sizeof(u_int8_t);

      while (currentpos + cpylength > datalength)
        allocateMoreMemory();

#ifdef AUDIODATA_PRINT_DEBUG
      printf("pushData() %d bytes (2).\n", cpylength);
#endif // AUDIODATA_PRINT_DEBUG

      memcpy(data + currentpos, (u_int8_t *)buffer, cpylength);

      currentpos += cpylength;

      duration = cpylength / bytespermillisec;

      return (int) duration;
    }

    // ----------------------------------------------------------------------

    /// Returns the current duration of the audio in milliseconds
    double getDuration() { return duration; }

    // -----------------------------------------------------------

    /// Returns the samplerate
    int getSamplerate() { return samplerate; }

    // ----------------------------------------------------------------------

    /// Saves the audio as a WAV file.
    void saveAsWav(char *filename) {
#ifdef AUDIODATA_PRINT_DEBUG
      printf("In saveAsWav(), bitspersample=%i samplerate=%i channels=%i\n",
             bitspersample, samplerate, channels);
#endif // AUDIODATA_PRINT_DEBUG
      
#ifdef HAVE_AUDIOFILE_H
      AFfilesetup fs = afNewFileSetup();

      afInitFileFormat(fs, AF_FILE_WAVE);
      afInitSampleFormat(fs, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 
                         bitspersample);
      afInitChannels(fs, AF_DEFAULT_TRACK, channels);
      afInitRate(fs, AF_DEFAULT_TRACK, samplerate);
      afInitCompression(fs, AF_DEFAULT_TRACK, AF_COMPRESSION_NONE);

      AFfilehandle fh = afOpenFile(filename, "w", fs);
      
      double num = (double)currentpos  // / (double)samplerate 
        / (double)channels * 8.0 / bitspersample;

      afWriteFrames(fh, AF_DEFAULT_TRACK, data, (int)num);

      afCloseFile(fh);
      afFreeFileSetup(fs);
#else
      cerr << "audiodata::saveAsWav(" << filename << ") failing"
	" due to missing libaudiofile" << endl;
#endif // HAVE_AUDIOFILE_H
    }

    // ----------------------------------------------------------------------

    /// Returns the data length in bytes
    long dataLength() {
      return datalength;
    }

    // ----------------------------------------------------------------------

    /// returns the (complex) fft of the current signal
    fftw_complex *FFT() {
      if (!fft_enabled)
        throw "audiodata: FFT was not enabled";

      /* changed 2013-01-10 by jorma
	int *truedata = (int*) data;
	for (int i=0; i<currentpos*8/bitspersample/channels; i++)
        fftinput[i] = (double) truedata[i];
	for (int i=currentpos; i<truedatalength; i++)
        fftinput[i] = 0;
      */

      if (bitspersample!=16) {
	cerr << "bitspersample!=16" << endl;
	exit(1);
      }
      if (sizeof(short)!=2) {
	cerr << "sizeof(short)!=2" << endl;
	exit(1);
      }

      int samples_available = currentpos*8/bitspersample/channels;
      int i = 0, chno = 0;

      short *truedata = (short*) data;
      for (; i<samples_available; i++)
        fftinput[i] = (double) truedata[channels*i+chno];
      for (; i<truedatalength; i++)
        fftinput[i] = 0;
      
#ifdef AUDIODATA_PRINT_DEBUG
      cout << "audiodata::FFT() currentpos=" << currentpos
	   << " bitspersample=" << bitspersample
	   << " channels=" << channels
	   << " chno=" << chno
	   << " samples_available=" << samples_available
	   << " truedatalength=" << truedatalength
	   << endl;
#endif // AUDIODATA_PRINT_DEBUG

#ifdef HAVE_FFTW3_H
      fftw_execute(fftplan);
#else
      fftw3_missing("audiodata::FFT()");
#endif // HAVE_FFTW3_H

      return fftoutput;
    }

    // -----------------------------------------------------------

    /// Returns the length of the fft data.
    size_t getFFTLength() {
      if (!fft_enabled)
        throw "audiodata: FFT was not enabled";

      return truedatalength / 2 + 1;
    }

    // ----------------------------------------------------------------------

    /// returns a short pointer to the data
    short *getData_short() {
      short *dataptr = (short*) data;
      return dataptr;
    }

    // -----------------------------------------------------------

    /// returns the data length of the data returned by getData()
    size_t getDataLength() {
      return currentpos*8/bitspersample/channels;
    }

    // ----------------------------------------------------------------------

    /// returns the data length in bytes
    size_t getDataLengthInBytes() {
      return currentpos;
    }

    // ----------------------------------------------------------------------

    /// saves the fft to the given file (as plain text)
    void saveFFT(char *filename) {
      if (!fft_enabled)
        throw "audiodata: FFT was not enabled";

      FILE *f = fopen(filename, "w");

      fftw_complex *four = FFT();

      for (int i=0; i<truedatalength/2+1; i++) {
        fprintf(f,"%f %f\n",four[i][0], four[i][1]);
      }

      fclose(f);
    }

    // ----------------------------------------------------------------------

    /// saves the fft to the given file (as plain text)
    void saveAsASCII(char *filename) {
      FILE *f = fopen(filename, "w");

      int *dataptr = (int*) data;

      for (int i=0; i<currentpos*8/bitspersample/channels; i++) {
        fprintf(f,"%i\n",dataptr[i]);
      }

      fclose(f);
    }

    // ----------------------------------------------------------------------

  }; // class audiodata

} // namespace picsom

#endif // _AUDIODATA_H

