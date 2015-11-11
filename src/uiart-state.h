// -*- C++ -*-  $Id: uiart-state.h,v 1.19 2010/05/26 11:56:14 jorma Exp $
// 
// Copyright 2008-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Copyright 2008-2009 UI-ART project group
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _uiart_state_h_
#define _uiart_state_h_

#include <time.h>
#include <math.h>

#include <string>
#include <vector>
#include <sstream>
using namespace std;

namespace uiart {

  /** All timing information is stored in this format which is actually
      only a wrapper around struct timespec.
   */
  class TimeSpec {
  public:
    ///
    TimeSpec(time_t s, long ns = 0) {
      ts.tv_sec  = s;
      ts.tv_nsec = ns;
      Normalize();
    }

    ///
    string Str() const {
      ostringstream tmp;
      tmp << "00000000" << ts.tv_nsec;
      string nsstr = tmp.str();
      nsstr.erase(0, nsstr.size()-9);

      ostringstream ss;
      ss << ts.tv_sec << "." << nsstr;
      return ss.str();
    }
 
    ///
    bool operator==(const TimeSpec& t) const {
      return (ts.tv_sec==t.ts.tv_sec && ts.tv_nsec==t.ts.tv_nsec);
    }

    ///
    bool operator>(const TimeSpec& t) const {
      return ts.tv_sec>t.ts.tv_sec ||
        (ts.tv_sec==t.ts.tv_sec && ts.tv_nsec>t.ts.tv_nsec);
    }
         
    ///
    bool operator<(const TimeSpec& t) const {
      return ts.tv_sec<t.ts.tv_sec ||
        (ts.tv_sec==t.ts.tv_sec && ts.tv_nsec<t.ts.tv_nsec);
    }
    
    ///
    bool operator>=(const TimeSpec& t) const {
      return ts.tv_sec>t.ts.tv_sec ||
        (ts.tv_sec==t.ts.tv_sec && ts.tv_nsec>=t.ts.tv_nsec);
    }

    ///
    TimeSpec operator+(double t) const {
      long s  = long(floor(t));
      long ns = long(1000000000.0*(t-s));
      s  += ts.tv_sec;
      ns += ts.tv_nsec;
      return TimeSpec(s, ns);
    }
  
    ///
    TimeSpec operator-(const TimeSpec& t) const {
      long s = ts.tv_sec - t.ts.tv_sec;
      long ns = ts.tv_nsec - t.ts.tv_nsec;
      return TimeSpec(s,ns);
    }
    
    ///
    operator double() const {
      return (double)ts.tv_sec + (double)ts.tv_nsec/1000000000.0;
    }

    /// Access to timespec_t.
    struct timespec& Ts() { return ts; }
 
  protected:
    ///
    TimeSpec Normalize() {
      while (ts.tv_nsec>1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec += 1;
      }
      while (ts.tv_nsec<0) {
        ts.tv_nsec += 1000000000;
        ts.tv_sec -= 1;
      }
      return *this;
    }

    ///
    struct timespec ts;
  };

  class CoordinateSystem {
  };

  class RGBvalue {
    float r, g, b;
  };

  class Source {
  public:
    //
    typedef enum { video, audio, gaze, object, ar_content } source_type;

    //
    Source(source_type t, const string& n) : type(t), name(n) {}

    //
    source_type type;

    //
    string name;
  };

  class Data {
  protected:
    ///
    Data(const TimeSpec& s, const TimeSpec& e, const TimeSpec& r) :
      start_time(s), end_time(e), record_time(r) {}

    ///
    string Str() const {
      ostringstream ss;
      ss << "start_time=" << start_time.Str()
         << " end_time=" << end_time.Str()
         << " record_time=" << record_time.Str();
      return ss.str();
    }

  public:
    TimeSpec start_time;
    TimeSpec end_time;
    TimeSpec record_time;
  };

  class Position {
  public:
    const CoordinateSystem *ref;
    float x;
    float y;
    float z;
    float alpha;
    float beta;
    float gamma;
  };

  class VideoSource : public Source { // name = "front"/"back"/...
  public:
    ///
    VideoSource(const string& n);

    ///
    float  frame_rate;

    ///
    size_t width, height;
  };

  class VideoData : public Data {
  public:
    ///
    VideoData(const VideoSource*);

    ///
    const VideoSource *ref;

    ///
    vector<RGBvalue>  pixels;

    ///
    string filename;

    ///
    string mimetype;
  };

  class AudioSource :  public Source { // name = "left"/"right"/...
  public:
    ///
    AudioSource(const string& n) : Source(audio, n) {}

    float sample_rate;
  };

  class AudioData :  public Data {
  public:
    ///
    AudioData(const TimeSpec& s, const TimeSpec& e, const TimeSpec& r) :
      Data(s, e, r) {}

    const AudioSource *ref;
    vector<float>     waveform;
  };

  class GazeSource : public Source { // name = "left"/"right"/...
  public:
    ///
    GazeSource(const string& n) : Source(gaze, n) {}

    float sample_rate;
    size_t width, height;
  };

  class GazeData : public Data {
  public:
    ///
    GazeData(float _x, float _y, const TimeSpec& _ts,
             float _ps = 0.0, float _t = 0.0) :
      Data(_ts, 0, 0), ref(NULL), x(_x), y(_y), pupil_size(_ps),  
      fixation_time(_t) { }

    ///
    string Str() const {
      ostringstream ss;
      ss << "x=" << x << " y=" << y << " pupil_size=" << pupil_size
         << " fixation_time=" << fixation_time << " ";
      return ss.str()+Data::Str();
    }

    ///
    bool IsInsideFixation(const TimeSpec& t) const {
      return t>=start_time && t<start_time+fixation_time/1000.0;
    }

    const GazeSource  *ref;
    float             x, y, pupil_size, fixation_time;
  
  };

  class ObjectSource :  public Source {
    // name = "head"/"marker"/"GPS"/"compass"/...
  public:  
    ObjectSource(const string& n) : Source(object, n) {}

  };

  class ObjectData :  public Data {
  public:
    ///
    ObjectData(const TimeSpec& s, const TimeSpec& e, const TimeSpec& r) :
      Data(s, e, r) {}

    const ObjectSource *ref;
    Position           position;
    string             identity;  // "marker-id-123456"/"John Doe"
  };

} // namespace uiart

#endif // _uiart_state_h_

