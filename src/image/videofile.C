// -*- C++ -*-  $Id: videofile.C,v 1.54 2020/03/30 19:40:42 jormal Exp $
// 
// Copyright 1998-2020 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <videofile.h>

#include <vector>
#include <sstream>
#include <fstream>
#include <errno.h>

namespace picsom {
  using namespace std;

  int          videofile::_debug = 0;
  bool         videofile::_keep_tmp_files = false;
  string       videofile::_tmp_dir;
  list<string> videofile::bin_path = { "/usr/bin" };
  ///--------------------------------------------------------------------------
  
  videofile::videofile(const string& filename, bool writing,
		       const string& tempd) 
    : _filename(filename), _writing(writing), _has_video(false),
      frame_num(0), mplayer(NULL), mplayer_broken(false), _tempdir(tempd) {
    if (filename!="")
      open(filename, writing);
  }

  ///--------------------------------------------------------------------------

  videofile::~videofile() {
    close();
    
    if (_writing && !frame_tempname.empty() && frame_num > 0 &&
        !keep_tmp_files()) {
      for (size_t i=1; i<=frame_num; i++) {
        unlink_file(frame_outname(frame_tempname, i));
      }
      unlink_file(frame_tempname);
    }
  }

  ///--------------------------------------------------------------------------
  
  bool videofile::open(const string& filename, bool writing) {
    close();

    _filename  = filename;
    _writing   = writing;
    _has_video = !_writing && identify();
    frame_num  = 0;

    if (filename[0]=='-' && writing)
      open_mplayer(filename);

    return true;
  }

  ///--------------------------------------------------------------------------
  
  bool videofile::close() {
    close_mplayer();
    _filename = "";
    _writing = _has_video = false;
    frame_num = 0;

    return true;
  }

  ///--------------------------------------------------------------------------
  
  void videofile::add_frame(const imagedata& d) {
    const string msg = "add_frame(): ";
    if (!_writing)
      throw error(msg, "failed: not writing");

    if (!frame_num)
      frame_tempname = tmp_filename();
    frame_num++;

    const string outname = frame_outname(frame_tempname, (int)frame_num);

    try {
      imagefile::write(d, outname, "image/jpeg");
      if (debug())
        cout << msg << "wrote " << outname << endl;
    } catch (const string& e) {
      cerr << msg << "imagefile::write(..., " << outname
           << ", image/jpeg) failed : " << e << endl;
      return;
    }

    feed_mplayer(outname);
  }

  ///--------------------------------------------------------------------------

  string videofile::frame_outname(const string f_tempname, int f_num) {
    char buf[100] = "*";
    if (f_num != -1)
      sprintf(buf, "%08d", f_num);
    return f_tempname+"_frame_"+buf+".jpg";
  }

  ///--------------------------------------------------------------------------

  void videofile::write(double fps, const string& filename, 
                        video_codec vcodec, audio_codec acodec,
                        int program) {
    const string msg = "write(): ";
    if (!_writing)
      throw error(msg, "failed: not writing");
    if (!frame_num)
      throw error(msg, "no frames added");

    const string& fname = filename.empty()?_filename:filename;
    if (fname.empty())
      throw error(msg, "no filename given");
    
    /*
    char cmdtmp[100];
    sprintf(cmdtmp, "mencoder \"mf://%s\" -mf type=jpg:fps=%f "
            "-of mpeg -o %s",
            frame_outname(frame_tempname).c_str(),fps,fname.c_str());
    */

    stringstream ss;
    double vbr = 6000*1000.0;
    ss << "mencoder \"mf://" << frame_outname(frame_tempname)
       << "\" -mf type=jpg:fps=" << fps << " -of mpeg -o " << fname
       << codecs_to_options(program, vcodec, acodec, vbr);

    if (debug()<2)
      ss << " 1>/dev/null 2>&1"; 

    string cmd = ss.str();

    int r = execute_system(cmd);
    
    if (r) 
      throw error(msg, "Execution of command \""+cmd+"\" failed.");
  }

  ///--------------------------------------------------------------------------

  bool videofile::identify(const string& filename) {
    string fname = filename.empty() ? _filename : filename;

    list<string> prog { "ffprobe", "avprobe" };
    string probe;
    for (const auto& p : prog) {
      for (const auto& d : bin_path) {
	string f = d+"/"+p;
	if (exists(f)) {
	  probe = f;
	  break;
	}
      }
      if (probe!="")
	break;
    }

    if (probe=="")
      return identify_mplayer_identify(fname);

    bool r = false;
    try {
      r = identify_avprobe(fname, probe);
    } catch (const string& err) {
      cerr << "identify_avprobe() failed : " << err << endl;
    }

    return r;
  }

  ///--------------------------------------------------------------------------

  static string calc_frac(const string& expin) {
    string exp = expin;
    size_t s = exp.find('/');
    if (s!=string::npos)
      exp[s] = ':';
    s = exp.find(':');
    if (s==string::npos)
      return exp;

    string a = exp.substr(0, s), b = exp.substr(s+1);
    float r = atof(a.c_str())/atof(b.c_str());
    stringstream ss;
    ss << r;

    return ss.str();
  }
  
  bool videofile::identify_avprobe(const string& fname,
				   const string& prog) {
    const string msg = "identify_avprobe(): ";
  
    //bool debug = false;

    const string tempname = videofile::tmp_filename(_tempdir);
    string full_cmd = prog+" -show_format -show_streams "+fname+
      " 2>/dev/null </dev/null > "+tempname;
    int r = execute_system(full_cmd);
    if (r) {
      stringstream ss;
      ss << r;
      // unlink_file(tempname);
      throw error(msg, "Execution of ", full_cmd, " failed with return value "+
		  ss.str()+".");
    }
 
    ifstream tmpf(tempname.c_str());
    if (!tmpf) {
      unlink_file(tempname);
      throw error(msg, "Could not open ", tempname, " for reading.");
    }
    if (tmpf.bad()) {
      unlink_file(tempname);
      tmpf.close();
      throw error(msg, "File ", tempname, " is bad() while reading.");
    }

    string codec_type, r_frame_rate;
    double duration = 0.0, time_base = 0.0;
    size_t nb_frames = 0;
    while (true) {
      string line;
      getline(tmpf, line);
      if (!tmpf)
	break;

      if (debug())
	cout << line << endl;

      size_t e = line.find('=');
      if (e==string::npos) {
	codec_type = "";
	if (debug())
	  cout << "* codec_type=" << codec_type << endl;
	continue;
      }

      string key = line.substr(0, e), val = line.substr(e+1);
      
      if (key=="codec_type") {
	codec_type = val;
	if (debug())
	  cout << "* codec_type=" << codec_type << endl;
      }

      if (key=="avg_frame_rate" && codec_type!="audio" && val!="0/0") {
	id["VIDEO_FPS"] = calc_frac(val);
	if (debug())
	  cout << "* VIDEO_FPS=" << id["VIDEO_FPS"] << endl;
      }
      
      if (key=="r_frame_rate" && codec_type!="audio" && val!="0/0") {
	r_frame_rate = calc_frac(val);
	if (debug())
	  cout << "* r_frame_rate=" << r_frame_rate << endl;
      }
      
      if (key=="width") {
	id["VIDEO_WIDTH"] = val;
	if (debug())
	  cout << "* VIDEO_WIDTH=" << val << endl;
      }
	
      if (key=="height") {
	id["VIDEO_HEIGHT"] = val;
	if (debug())
	  cout << "* VIDEO_HEIGHT=" << val << endl;
      }
      
      if (key=="duration" && codec_type!="audio") {
	id["LENGTH"] = val;
	duration = atof(val.c_str());
	if (debug())
	  cout << "* LENGTH=" << val << " duration=" << duration << endl;
      }

      if (key=="nb_frames" && codec_type!="audio") {
	nb_frames = atoi(val.c_str());
	if (debug())
	  cout << "* nb_frames=" << nb_frames << endl;
      }
      
      if (key=="time_base" && codec_type!="audio") {
	size_t p = val.find('/');
	if (p!=string::npos) {
	  double a = atof(val.c_str()), b = atof(val.substr(p+1).c_str());
	  time_base = a/b;
	  if (debug())
	    cout << "* time_base=" << a/b << endl;
	}
      }

      if (key=="display_aspect_ratio") {
	string valx = val;
	if (valx=="N/A") // added 20200127
	  valx = "0/1";
	id["VIDEO_ASPECT"] = calc_frac(valx);
	if (debug())
	  cout << "* VIDEO_ASPECT=" << id["VIDEO_ASPECT"] << endl;
      }
      
      if (key=="format_long_name") {
	if (val=="MPEG-PS format")
	  id["DEMUXER"] = "mpegps";
	else
	  id["DEMUXER"] = "???";
	if (debug())
	  cout << "* DEMUXER=" << id["DEMUXER"] << endl;
      }
    }
    
    if (id.find("VIDEO_FPS")==id.end() && r_frame_rate!="") {
      id["VIDEO_FPS"] = r_frame_rate;
      if (debug())
        cout << "*a VIDEO_FPS=" << r_frame_rate << endl;
    }
    
    if (id.find("VIDEO_FPS")==id.end() && time_base) {
      char tmp[100];
      sprintf(tmp, "%f", 1/time_base);
      id["VIDEO_FPS"] = tmp;
      if (debug())
        cout << "*b VIDEO_FPS=" << tmp << endl;
    }

    if (id.find("VIDEO_FPS")==id.end() && duration && nb_frames) {
      char tmp[100];
      sprintf(tmp, "%f", nb_frames/duration);
      id["VIDEO_FPS"] = tmp;
      if (debug())
        cout << "*c VIDEO_FPS=" << tmp << endl;
    }

    if (id.find("VIDEO_FPS")==id.end()) {
      id["VIDEO_FPS"] = "";
      if (debug())
        cout << "*d VIDEO_FPS=" << endl;
    }
    
    unlink_file(tempname);

    return true;
  }

  ///--------------------------------------------------------------------------
  
  bool videofile::identify_mplayer_identify(const string& fname) {
    const string msg = "identify_mplayer_identify(): ";

    const string midentify_cmd = "/usr/bin/mplayer -vo null -ao null "
      "-frames 1 -identify ";
    const string identify_cmd = "/usr/bin/identify";

    const string tempname = videofile::tmp_filename();
    //int r = execute_system(midentify_cmd+" "+fname+" > "+tempname);
    int r = execute_system(midentify_cmd+" "+fname+
			   " 2>/dev/null </dev/null | grep ^ID_ > "+tempname);
    if (r) {
      unlink_file(tempname);
      throw error(msg, "Execution of <", midentify_cmd+" "+fname, "> failed.");
    }
 
    ifstream tmpf(tempname.c_str());
    if (!tmpf) {
      unlink_file(tempname);
      throw error(msg, "Could not open ", tempname, " for reading.");
    }
    if (tmpf.bad()) {
      unlink_file(tempname);
      tmpf.close();
      throw error(msg, "File ", tempname, " is bad() while reading.");
    }

    int i = 0;
    char line[1024];
    while (tmpf.getline(line, sizeof line), tmpf.good()) {
      while (*line && line[strlen(line)-1]=='\r')
        line[strlen(line)-1] = 0;
      
      string ll = line;
      size_t pos = ll.find_first_of("=");
      string id_name = ll.substr(0,pos);
      string id_value = ll.substr(pos+1);
      if (id_name.substr(0,3) != "ID_")
        throw error(msg, "Bad identify name: ", id_name);
      id_name = id_name.substr(3);
      id[id_name] = id_value;
      if (debug())
        cout << msg << " [" << fname << "] " 
             << id_name << " => " << id_value << endl;
      i++;
    }
    tmpf.close();

    bool is_mpegts = id.find("DEMUXER")!=id.end() && id["DEMUXER"]=="mpegts";

    double length = atof(id["LENGTH"].c_str());
    if (!is_mpegts && length==0.0) {
      if (debug())
        cout << msg << " midentify returned 0 length, trying identify..." 
             << endl;
      r = execute_system(identify_cmd+" "+fname+" | wc -l > "+tempname);
      if (r) {
        unlink_file(tempname);
        throw error(msg, "Execution of ", identify_cmd, " failed.");
      }
      tmpf.open(tempname.c_str());
      if (!tmpf || tmpf.bad()) {
        unlink_file(tempname);
        if (tmpf) tmpf.close();
        throw error(msg, "Error reading ", tempname, ".");
      }
      tmpf.getline(line, sizeof line);
      int frames = atoi(line);
      length = frames/get_frame_rate();
      stringstream ss;
      ss << length;
      id["LENGTH"] = ss.str();
      tmpf.close();
    }

    unlink_file(tempname);
      
    return i;
  }
  
  ///--------------------------------------------------------------------------
  
  string videofile::extract_video_segment_to_tmp(const string& ext,
						 double tp, double dur,
						 video_codec vcodec,
						 audio_codec acodec,
						 int program,
                                                 const string& extra_opts,
						 const string& tmpdir) {
    const string tempbase = tmp_filename(tmpdir);
    const string tempname = tempbase+ext;

    extract_video_segment(tempname, tp, dur, vcodec, acodec, program,
                          extra_opts);
    unlink_file(tempbase);

    return tempname;
  }

  ///--------------------------------------------------------------------------
  
  const string& videofile::avconvname() {
    static string conv;
    if (conv=="") {
      list<string> prog { "ffmpeg", "avconv" };
      for (const auto& p : prog) {
	for (const auto& d : bin_path) {
	  string f = d+"/"+p;
	  if (exists(f)) {
	    conv = f;
	    break;
	  }
	}
	if (conv!="")
	  break;
      }
      if (conv=="")
	conv = "/bin/false";
    }
    return conv;
  }

  ///--------------------------------------------------------------------------

  void videofile::extract_video_segment(const string& filename,
                                        double tp, double dur,
                                        video_codec vcodec,
                                        audio_codec acodec,
                                        int program,
                                        const string& extra_opts) {
    const string msg = "extract_video_segment(): ";

    string avconv = avconvname();
    double fps = get_frame_rate();
    
    int frames = (int)floor(dur*fps+0.5);
    if (frames<1)
      frames = 1;

    //hack needed in tv08...
    //if (frames < 40) frames=40;

    if (debug())
      cout << msg << "found timepoint=" << tp << "s, duration=" << dur 
           << "s" << endl;

    double vbr = 6000*1000.0;
    string cmds, opts = codecs_to_options(program, vcodec, acodec, vbr);
    
    if (!extra_opts.empty())
      opts += " " + extra_opts;

    char cmdtmp[1000];
    switch (program) {
    case VF_MENCODER: // mencoder
      sprintf(cmdtmp, "mencoder %s -ss %f -frames %d -of mpeg -o %s", 
              _filename.c_str(), tp, frames, filename.c_str());
      cmds = cmdtmp + opts;
      break;

    case VF_FFMPEG: // avconv actually... -same_q for ffmpeg
      if (frames==1) {
	double asp = get_aspect();
	int w = get_width(), h = get_height();
	if (asp)
	  w = int(floor(h*asp+0.5));
	stringstream ass;
	ass << "-s " << w << "x" << h;
	// string aspect = "-aspect "+ToStr(asp);
	string aspect = ass.str();
	sprintf(cmdtmp,
		// -same_quant removed 2014-10-21 when turning to libav-9.17
		"%s -nostdin -y -i %s -ss %f -vframes 1 %s -f image2 %s",
		avconv.c_str(),
		_filename.c_str(), tp, aspect.c_str(), filename.c_str());
      } else
	sprintf(cmdtmp, "%s -nostdin -y -i %s -ss %f -t %f %s %s", 
		avconv.c_str(),
		_filename.c_str(), tp, dur, opts.c_str(), filename.c_str());

      cmds = cmdtmp;
      break;

    default:
      throw error(msg, "Unknown program number!");
    }

    if (debug()<2)
      cmds += " 1>/dev/null 2>&1"; 

    int r = execute_system(cmds);

    if (r) 
      throw error(msg,"Execution of exctraction command failed.");
  }

  ///--------------------------------------------------------------------------
  
  string videofile::codecs_to_options(int program,
				      video_codec vcodec,
				      audio_codec acodec, 
				      double video_br) {
    const string msg = "codecs_to_options(): ";

    string cmds = "";

    if (program==VF_MENCODER) {
      vector<string> lavcopts;
      
      // Set audio codec
      if (acodec==acodec_null)
        cmds += " -nosound";
      else {
        cmds += " -oac ";
        switch (acodec) {
        case acodec_copy:
          cmds += "copy";
          break;
        case acodec_mp2:
          cmds += "lavc";
          lavcopts.push_back("acodec=mp2");
          break;
        case acodec_mp3:
          cmds += "lavc";
          lavcopts.push_back("acodec=mp3");
          break;
        default:
          throw error(msg,"Audio codec not implemented!");
        }
      }

      // Set video codec
      cmds += " -ovc ";
      switch (vcodec) {
      case vcodec_copy:
        cmds += "copy";
        break;
      case vcodec_mpeg1:
        cmds += "lavc";
        lavcopts.push_back("vcodec=mpeg1video");
        break;
      case vcodec_mpeg2:
        cmds += "lavc";
        lavcopts.push_back("vcodec=mpeg2video");
        break;
      case vcodec_mpeg4:
        cmds += "lavc";
        lavcopts.push_back("vcodec=mpeg4");
        break;
      default:
        throw error(msg,"Video codec not implemented!");
      }

      // Possibly add lavcopts 
      if (!lavcopts.empty()) {
        cmds += " -lavcopts ";
        for (size_t i=0; i<lavcopts.size(); i++) 
          cmds += (i?":":"")+lavcopts[i];
      }
    }
    
    if (program==VF_FFMPEG) {
      if (acodec==acodec_copy)
        cmds += " -acodec copy";
      else if (acodec==acodec_null)
        cmds += " -an";
      else
	throw error(msg, "Audio codec not implemented!");
  
      if (vcodec==vcodec_copy) 
	cmds += " -vcodec copy";
      else if (vcodec==vcodec_null) 
	cmds += " -vn";
      else if (vcodec==vcodec_mpeg1) 
	cmds += " -vcodec mpeg1video";
      else if (vcodec==vcodec_mpeg4) 
	cmds += " -vcodec mpeg4";
      else
	throw error(msg, "Video codec not implemented!");

      if (video_br) {
	stringstream tmp;
	tmp << (int)floor(video_br+0.5);
	cmds += " -b:v "+tmp.str();
      }

      if (get_frame_rate()>0) {
	stringstream tmp;
	tmp << get_frame_rate();
	string s = tmp.str();
	size_t p = s.find('.');
	if (p!=string::npos && s.length()>p+3)
	  s.erase(p+3);
	cmds += " -r "+s;
      }
    }
    
    return cmds;
  }

  ///--------------------------------------------------------------------------

  string videofile::extract_audio(string fn, int samplerate) {
    const string msg = "extract_segment_video() ";
    if (fn.empty()) fn=_filename;
    
    const string tempbase = videofile::tmp_filename();
    string wavname_tmp = tempbase+"_tmp.wav";
    string wavname = tempbase+".wav";
    
    string acmd = "mplayer -ao pcm:file="+wavname_tmp+
      // " -vc dummy"+ // doesn't seem to work or be needed in some versions(?)
      " -vo null "+fn;

    string avconv = avconvname();
    if (avconv!="/bin/false") {
      // string acodec = "pcm_alaw";
      string acodec = "pcm_s16le";
      acmd = avconv+" -y -i "+fn+" -vn -acodec "+acodec+" "+wavname_tmp;
    }

    if (debug()<2) {
      acmd += " 1>/dev/null"; // " 1>/dev/null 2>&1";
      if (debug()<1 && avconv!="/bin/false")
	acmd += " 2>&1";
    }

    int r = execute_system(acmd);
    if (r) {
      unlink_file(tempbase);
      throw error(msg, "audio extraction command failed");
    }

    stringstream mcmd;
    mcmd << "sox " << wavname_tmp << " -c 1 ";
    if (samplerate)
      mcmd << " -r " << samplerate << " ";
    mcmd << wavname;
    if (debug()<2)
      mcmd << " 1>/dev/null"; // " 1>/dev/null 2>&1";
    
    r = execute_system(mcmd.str());
    unlink_file(wavname_tmp);
    unlink_file(tempbase);

    if (r) 
      throw error(msg, "audio mono conversion command failed");
    
    return wavname;
  }

  ///--------------------------------------------------------------------------

  string videofile::tmp_filename(const string& tmpdir) {
    string tempname = tmpdir!="" ? tmpdir : tmp_dir();
    tempname += "/picsom_videofile_XXXXXX";
    char tempnamec[1000];
    strcpy(tempnamec, tempname.c_str());

    /* creates empty file tempnamec just to allocate the "namespace" in the
       filesystem */
    int res = mkstemp(tempnamec);

    if (res==-1) {
      stringstream err;
      err << errno << " " << strerror(errno);
      throw error("temp_filename() failed with mkstemp(", tempnamec, 
                  ") saying '", err.str(), "'");
    }
    ::close(res);

    return tempnamec;
  }

  ///--------------------------------------------------------------------------

  string videofile::tmp_dir() {
    if (_tmp_dir.empty()) {
      char *envdir = getenv("TMPDIR");
      _tmp_dir = (envdir && (strlen(envdir)>0) ? envdir : "/tmp");
    }
    return _tmp_dir;
  }

  ///--------------------------------------------------------------------------

  bool videofile::open_mplayer(const string& opt) {
    if (mplayer)
      close_mplayer();
  
    string cmd = "mplayer -vo x11 -nosound -fixed-vo -slave -idle";
    // cmd += " -framedrop";
    if (opt!="-")
      cmd += " "+opt;
    if (debug()<2)
      cmd += " 1>/dev/null 2>&1";

    mplayer = popen(cmd.c_str(), "w");
    mplayer_broken = false;
    
    if (debug())
      cout << "*** Opened new mplayer [" << mplayer << "] *** " << endl;

    return true;
  }

  ///--------------------------------------------------------------------------

  bool videofile::feed_mplayer(const string& f) {
    if (mplayer) {
      // timespec_t snap1 = { 0, 1000 };
      // nanosleep(&snap1, NULL); 
      struct stat sbuf;
      int n = stat(f.c_str(), &sbuf) ?  -1 : (int)sbuf.st_size;
      
      /*
      if (n>0) {
        char *buf = new char[n];
        int fd = ::open(f.c_str(), O_RDONLY);
        if (fd==-1)
          cout << "feed_mplayer() : open() failed" << endl;
        else {
          ssize_t nn = read(fd, buf, n);
          if (nn!=n)
            cout  << "feed_mplayer() : n=" << n << " != " << nn << "=nn" << endl;
        }
        if (fd!=-1)
          ::close(fd);

        delete buf;
      }
      */

      string cmd = "loadfile mf://"+f+"\n";
      if (debug())
        cout << "feed_mplayer() *** " << f << " " << n << " bytes" << endl;
      fprintf(mplayer, "%s", cmd.c_str());
      // timespec_t snap2 = { 0, 1000 };
      // nanosleep(&snap2, NULL); 
      int ret = fflush(mplayer);
      if (ret<0) {
        cout << endl << "*** Error writing to mplayer [" 
             << strerror(errno) << "] <" << f << "> " << n
             << " bytes ***" << endl << endl;
        mplayer_broken = true;
      }
    }

    return true;
  }

  ///--------------------------------------------------------------------------

  bool videofile::close_mplayer() {
    if (mplayer) {
      if (!mplayer_broken) {
        fprintf(mplayer, "quit\n");
        fflush(mplayer);
      }
      pclose(mplayer);
      if (debug())
        cout << "*** Closed mplayer" << (mplayer_broken?" (was BROKEN)":"")
             << endl;
    }
    mplayer_broken = false;
    mplayer = NULL;        

    return true;
  }

  ///--------------------------------------------------------------------------

  bool videofile::reencode(const string& dst, const string& spec) const {
    const string msg = "videofile::reencode("+dst+","+spec+") : ";
    if (debug())
      cout << msg << "starting" << endl;

    string cmd = "ffmpeg -i "+_filename+" "+dst;
    if (!debug())
      cmd += " 1>/dev/null 2>&1";

    int r = execute_system(cmd);

    if (r)
      cerr << msg << "failed to execute [" << cmd << "]" << endl;
    
    return r==0;
  }

  ///--------------------------------------------------------------------------

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
