// -*- C++ -*-  $Id: ExternalSound.C,v 1.13 2017/11/29 05:09:11 jormal Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

//#ifdef __linux__

#include <iterator>

#include <ExternalSound.h>

#include <fcntl.h>

#ifdef HAVE_AUDIOFILE_H
#include <audiofile.h>
#endif // HAVE_AUDIOFILE_H

namespace picsom {
  static const char *vcid =
    "$Id: ExternalSound.C,v 1.13 2017/11/29 05:09:11 jormal Exp $";

  static ExternalSound list_entry(true);

  //===========================================================================

  string ExternalSound::Version() const {
    return vcid;
  }

  //===========================================================================

  Feature::featureVector ExternalSound::getRandomFeatureVector() const {
    featureVector ret;
    return ret;
  }

  //===========================================================================

  int ExternalSound::printMethodOptions(ostream&) const {
    return 0;
  }

  //===========================================================================

  Feature::featureVector ExternalSound::CalculatePerFrame(int f, int ff, 
							  bool print) {
#ifndef __MINGW32__
    cox::tictac::func tt(tics, "Feature::CalculatePerFrame");
#endif

    stringstream msg;
    msg << "ExternalSound::CalculatePerFrame(" << f << "," << ff << ","
	<< print << ") : ";

    //PossiblySetZoning();  // added 13.9.2004

    if (f && WithinFrameTreatment()==treat_concatenate &&
	!DataLabels().empty()) {
      vector<string> labels = SolveUsedDataLabels();
      sort(labels.begin(), labels.end()); // later on, something smarter...
      if (labels!=DataLabels())
	throw msg.str()+"labels of existing image segments differ";
    }

    if (f && WithinFrameTreatment()==treat_separate) {
      SetDataLabelsAndIndices(true);
      AddDataElements(true);
    }

    bool ok = true;

    // imagedata dummyimg;
    // pp_chain_t dummyppm;
    // if (!FramePreProcess(dummyimg, f)) // obs! preprocess_in_use ???
    //   ok = false;

    if (ok && !CalculateOneFrame(f))
      ok = false;

    if (!ok) {
      cerr << msg.str() << "Warning: calculation failed, "
	"using zero vector." << endl;
      switch (WithinFrameTreatment()) {
      case treat_separate: {
	const vector<string>& l = DataLabels();
	int j=0;
	for (vector<string>::const_iterator i=l.begin(); i!=l.end(); i++)
	  GetData(j++)->ZeroToSize(FeatureVectorSize());
	break;
      }
      case treat_concatenate:
	GetData(f)->ZeroToSize(FeatureVectorSize());
	break;
      case treat_pixelconcat:
      case treat_average:
      case treat_join:
      default:
	throw msg.str()+"within frame treatment \""+
	  WithinFrameTreatmentName()+ "\" not implemented yet";
      }
      ok = true;
    }

    if (print && ok)
      DoPrinting(-1, ff);

    featureVector concat;

    if (!print)
      concat = AggregatePerFrameVector(f);

    if (FrameVerbose())
      cout << msg.str() << "returning vector of length " << concat.size()
	   << endl;

    return concat;
  }

  //===========================================================================

  bool ExternalSound::CalculateOneFrame(int f) {
    char msg[100];
    sprintf(msg, "ExternalSound::CalculateOneFrame(%d)", f);
  
    if (FrameVerbose())
      cout << msg << " : starting" << endl;
  
    bool ok = true;
  
    if (!CalculateOneLabel(f, 0))
      ok = false;
  
    if (FrameVerbose())
      cout << msg << " : ending" << endl;
  
    return ok;
  }

  //===========================================================================

  External::execchain_t ExternalSound::GetExecChain(int /*f*/, bool /*all*/, 
						    int /*l*/) {
    string exec = "/share/imagedb/picsom/external/linux64/feacat";
    string filep = FullFileName(tempfilename!="" ? tempfilename : datafilename);

    // a temporary config file should be created on the fly instead of a static
    // file
    string configfilepath = "/home/hmuurine/picsom/features/mfcc_p.feaconf";

    // stringstream startfss,endfss;
    // string startf,endf;
    // startfss << f;
    // endfss << f+1;
    // startfss >> startf;
    // endfss >> endf;

    execargv_t execargv;
    execargv.push_back(exec);
    execargv.push_back("-c");
    execargv.push_back(configfilepath);
    // execargv.push_back("-s");
    // execargv.push_back(startf);
    // execargv.push_back("-e");
    // execargv.push_back(endf);
    execargv.push_back(filep);

    execspec_t execspec(execargv, execenv_t(), "", "");

    return execchain_t(1, execspec);
  }

  //===========================================================================

  bool ExternalSound::ExecPostProcess(int /*f*/, bool /*all*/, int l) {
    string str = GetOutput();
    istringstream ss(str);
    vector<float> res;

    copy(istream_iterator<float>(ss), istream_iterator<float>(), 
	 back_inserter(res));
 
    ExternalSoundData *data_dst = (ExternalSoundData *)GetData(l);

    //if(BetweenFrameTreatment()==treat_separate)
    //  data_dst->Zero();

    data_dst->AddValue(res);
    //cout << videoframecount << endl;
    if (videoframecount > data_dst->ResultCount())
      data_dst->FillResults(videoframecount);
    //cout << videoframecount << endl;

    return true;
  }

  //===========================================================================

  Feature::featureVector
  ExternalSound::AggregatePerFrameVector(size_t f) const {
    EnsureSegmentation();
  
    const vector<string>& labels = DataLabels();

    if (!labels.size())
      throw "AggregatePerFrameVector() no labels in segmentation";

    if (FrameVerbose()) {
      cout << "Feature::AggregatePerFrameVector() labels.size()="
	   << labels.size() << " :";
      for (size_t i=0; i<labels.size(); i++)
	cout << (i==0?" ":", ") << labels[i];
      cout << endl;
    }

    featureVector aggregate;

    bool azc = true;  // "allow_zero_count"
    for (size_t i=0; i<labels.size(); i++) {
      const featureVector v = ResultVector(i, azc, f); // was labels[i]
    
      if (LabelVerbose())
	cout << "Feature::AggregatePerFrameVector() : " << i << "/"
	     << labels.size() << " size=" << v.size() << endl;

      aggregate.insert(aggregate.end(), v.begin(), v.end());
    }

    if (!aggregate.size())
      throw "empty AggregatePerFrameVector";

    return aggregate;
  }

  //===========================================================================

  bool ExternalSound::DoPrinting(int slice, int frame) {
    static const string errhead = "Feature::DoPrinting() : ";
    bool azc = true;
    string lbl;

    switch (WithinFrameTreatment()) {
    case treat_separate: {
      const vector<string>& l = DataLabels();
      int j=0;
      for (vector<string>::const_iterator i=l.begin(); i!=l.end(); i++)
	Print(ResultVector(j++, azc, frame), Label(slice, frame, *i));
      break;
    }

    case treat_concatenate:
      Print(AggregatePerFrameVector(frame), Label(slice, frame, ""));
      break;

    case treat_pixelconcat:
    case treat_average:
    case treat_join:
    default:
      throw errhead+"within frame treatment \""+
	WithinFrameTreatmentName()+ "\" not implemented yet";
    }

    return true;
  }

  //===========================================================================

  bool ExternalSound::LoadObjectData(const string& datafile, 
				     const string& /*segmentfile*/) { 

    datafilename = datafile;

    if (datafilename.find(".wav",datafilename.size()-4) != string::npos) {
      // obs! works only for audio files with .wav extension

      // use libaudiofile to determine audio length if we have a wav file,
      // and calculate the framecount from it:
    
      tempfilename = "";

#ifdef HAVE_AUDIOFILE_H
      AFfilehandle fh = afOpenFile(datafile.c_str(), "r", AF_NULL_FILESETUP);
    
      double samplerate = afGetRate(fh, AF_DEFAULT_TRACK);
      AFframecount framec = afGetFrameCount(fh, AF_DEFAULT_TRACK);
    
      afCloseFile(fh);
    
      const double fps = 29.97; // obs! hard-coded value is used unless we have
      // a proper video file
    
      videoframecount = (int) floor(framec*fps/samplerate);
    
#else
      cerr << "ExternalSound::LoadObjectData(" << datafile << ") failing"
	" due to missing libaudiofile" << endl;
      return false;
#endif // HAVE_AUDIOFILE_H

    } else if (datafilename.find(".mpg",datafilename.size()-4)!=string::npos) {
      // obs! works only for videos with .mpg extension

      // use imagefile/xine to determine the real video frame count if we 
      // have an mpg file. Also dump the audio stream into a temporary wav 
      // file:

      imagefile img = imagefile(datafilename);
      videoframecount = img.nframes();
    
      string tempname = "/tmp/picsom_feature_tmp_wav_XXXXXX";
      char tempnamec[100];
      strcpy(tempnamec,tempname.c_str());
    
      if (mkstemp(tempnamec) == -1)
	return false;
    
      tempname = string(tempnamec)+"_tmp.wav";
      tempfilename = string(tempnamec)+".wav";
    
      // extract stereo sound stream:
      string cmd = "mplayer -ao pcm:file="+tempname+" -vc dummy -vo null "
	+datafilename;
      //cout << "calling system("<<cmd<<")"<<endl;
      int error = system(cmd.c_str());
      if (error)
	return false;

      // convert to mono 32000Hz wave file:
      cmd = "sox "+tempname+" -c 1 -r 32000 "+tempfilename;
      //cout << "calling system("<<cmd<<")"<<endl;
      error = system(cmd.c_str());
      if (error)
	return false;
    
      //cout << "unlinking " << tempname << endl;
      Unlink(tempname);

    } else 
      return false; // return false if unknown file extension
  
    return true;
  }

  //===========================================================================

} // namespace picsom

//#endif // __linux__

// Local Variables:
// mode: font-lock
// End:
