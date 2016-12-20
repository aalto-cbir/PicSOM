// -*- C++ -*-  $Id: videofile.h,v 1.29 2016/05/03 10:55:58 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/*!\file videofile.h

  \brief Declarations and definitions of class picsom::videofile.

  videofile.h contains methods for handling video files.
  Currently calls mplayer/mencoder externally, but in the future
  one might use e.g. the libavcodec library 
  (http://ffmpeg.mplayerhq.hu/).

  \author Mats Sjoberg <mats.sjoberg@tkk.fi>
  $Revision: 1.29 $
  $Date: 2016/05/03 10:55:58 $
  \bug May be some out there hiding.
  \warning Be warned against all odds!
  \todo So many things, so little time...
*/

#ifndef _videofile_h_
#define _videofile_h_

#include <imagefile.h>
#include <string>
#include <map>
#include <iostream>
#include <math.h>
#include <stdlib.h>

#define VF_MENCODER 0
#define VF_FFMPEG   1

namespace picsom {
  using namespace std;

  enum video_codec {
    vcodec_null,
    vcodec_copy,
    vcodec_mpeg1,
    vcodec_mpeg2,
    vcodec_mpeg4
  };

  enum audio_codec {
    acodec_null,
    acodec_copy,
    acodec_mp2,
    acodec_mp3
  };

  class videofile {
  public:
    /// Returns version of videofile class ie. version of videofile.h.
    static const string& version() {
      static string v =
	"$Id: videofile.h,v 1.29 2016/05/03 10:55:58 jorma Exp $";
      return v;
    }

    /// Default constructor
    videofile() {
      frame_num = 0;
      mplayer = NULL;
      mplayer_broken = false;
    }

    ///
    videofile(const string& filename, bool writing=false, const string& tempd="");

    /// Destructor.
    ~videofile();

    ///
    bool open(const string& f, bool wr = false);

    ///
    bool close();

    ///
    bool is_open() const { // obs!
      return _filename!="" || mplayer;
    }

    ///
    const string& filename() const { return _filename; }

    ///
    void add_frame(const imagedata& d);

    ///
    void write(double fps, 
	       const string& filename="",
	       video_codec vcodec=vcodec_mpeg2,
	       audio_codec acodec=acodec_null,
	       int program=VF_MENCODER);

    /// Extracts given time segment to a temporary filename which is returned.
    string extract_video_segment_to_tmp(const string& ext,
                                        double timepoint, double duration,
                                        video_codec vcodec=vcodec_copy,
                                        audio_codec acodec=acodec_copy,
                                        int program=VF_MENCODER,
                                        const string& extra_opts="",
					const string& tmpdir="");

    /// Extracts given time segment to given filename.
    void extract_video_segment(const string& filename,
			       double timepoint, double duration,
			       video_codec output_codec=vcodec_copy,
			       audio_codec acodec=acodec_copy,
			       int program=VF_MENCODER,
                               const string& extra_opts="");

    ///
    string codecs_to_options(int program, 
			     video_codec vcodec,
			     audio_codec acodec,
			     double video_br);

    ///
    string extract_audio(string fn="", int samplerate=0);

    ///
    int get_num_frames() { 
      return _writing ? frame_num : 
	(int)floor(get_frame_rate()*get_length()+0.5);
    }

    ///
    double get_frame_rate() { return atof(id["VIDEO_FPS"].c_str()); }

    ///
    int get_width() { return atoi(id["VIDEO_WIDTH"].c_str()); }

    ///
    int get_height() { return atoi(id["VIDEO_HEIGHT"].c_str()); }

    ///
    double get_length() { return atof(id["LENGTH"].c_str()); }

    ///
    double get_aspect() { return atof(id["VIDEO_ASPECT"].c_str()); }

    ///
    string get_demuxer() { return id["DEMUXER"]; }

    /** Returns true if video has been identified, false usually means
	that the file exists but is without a video stream.
    */
    bool has_video() { return _has_video; }

    ///
    static void local_bin(const string& p) { _local_bin = p; }

    ///
    static const string& local_bin() { return _local_bin; }

    /// Returns current debug mode.
    static int debug() { return _debug; }

    /// Sets debug mode.
    static void debug(int d) { _debug = d; }

    /// Returns current value of the temporary file preservation mode
    static bool keep_tmp_files() { return _keep_tmp_files; }

    /// Sets the temporary file preservation mode
    static void keep_tmp_files(bool d) { _keep_tmp_files = d; }
    
    /// Returns true if mplayer pipe is broken
    bool is_broken() { return mplayer_broken; }
   
  protected:
    ///
    string frame_outname(const string f_tempname, int f_num=-1);
      
    ///
    bool identify(const string& filename="");

    ///
    bool identify_mplayer_identify(const string& filename);

    ///
    bool identify_avprobe(const string& filename);

    ///
    static const string& avconvname();

    /// Generates temporary filename.
    static string temp_filename(const string& = "");

    /// Return directory for temporary files.
    static string temp_dir();

    /** Creates an error string from the given parameters (max 7)
        \return the resulting error string
    */
    static string error(const string& t, const string& u = "",
			const string& v = "", const string& w = "",
			const string& x = "", const string& y = "",
			const string& z = "") {
      static string head = "picsom::videofile::";
      return head+t+u+v+w+x+y+z;
    }

    ///
    static int execute_system(const string& cmd) {
      if (debug())
	cout << "Executing system(" << cmd << ")" << endl;
      return system(cmd.c_str());
    }

    ///
    inline bool unlink_file(const string& n) {
      return !unlink(n.c_str());
    }
    
    ///
    bool open_mplayer(const string&);

    ///
    bool feed_mplayer(const string&);

    ///
    bool close_mplayer();

  private:
    /// name of video file
    string _filename;

    /// writing
    bool _writing;

    ///
    bool _has_video;

    ///
    map<string, string> id;

    ///
    string frame_tempname;

    ///
    size_t frame_num;
    
    ///
    FILE *mplayer;

    ///
    static string _local_bin;

    /// debug flag
    static int _debug;

    /// debug flag
    static bool _keep_tmp_files;

    ///
    static string _temp_dir;
    
    ///
    bool mplayer_broken;

    ///
    string _tempdir;

  }; // class videofile
} // namespace picsom

#endif // !_videofile_h_

///////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: lazy-lock
// End:
