// $Id: ExtBatch.C,v 1.5 2020/03/30 13:40:27 jormal Exp $	

// features.linux64.debug -d7 extbatch -o dir=/u/18/jormal/unix/picsom/python/AudioTagger/tagger -o venv=/u/18/jormal/unix/picsom/python/AudioTagger/venv -o cmd="python audio_tagging.py --fname /u/18/jormal/unix/t19/objects/006790.mp4 --model model_27000[0.344128].h5" 006790.mp4 

// features.linux64.debug -d7 extbatch -o dir=/u/18/jormal/unix/picsom/python/AudioTagger/tagger -o venv=/u/18/jormal/unix/picsom/python/AudioTagger/venv -o cmd="/bin/cat /etc/hosts" 006790.mp4 

#include <iterator>

#include <ExtBatch.h>
#include <fcntl.h>

namespace picsom {
  static const char *vcid =
    "$Id: ExtBatch.C,v 1.5 2020/03/30 13:40:27 jormal Exp $";

  static ExtBatch list_entry(true);

  //===========================================================================

  string ExtBatch::Version() const {
    return vcid;
  }

  //===========================================================================

  bool ExtBatch::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "ExtBatch::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
        cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="chunk_size") {
        chunk_size = atoi(value.c_str());
        i = l.erase(i);
        continue;
      }

      if (key=="dir") {
        dir = value;
        i = l.erase(i);
        continue;
      }

      if (key=="venv") {
        venv = value;
        i = l.erase(i);
        continue;
      }

      if (key=="llp") {
        llp = value;
        i = l.erase(i);
        continue;
      }

      if (key=="cmd") {
        cmd = SplitInSpaces(value);
        i = l.erase(i);
        continue;
      }

      // if (key=="env") {
      //   env = value;
      //   i = l.erase(i);
      //   continue;
      // }

      i++;
    }

    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  Feature::featureVector ExtBatch::getRandomFeatureVector() const {
    featureVector ret;
    return ret;
  }

  //===========================================================================

  int ExtBatch::printMethodOptions(ostream&) const {
    return 0;
  }

  //===========================================================================

  External::execchain_t ExtBatch::GetExecChain(int, bool, int) {
    string msg = "ExtBatch::GetExecChain() : ";
    if (FileVerbose())
      cout << TimeStamp() << msg << "called with dir=[" << dir << "] venv=["
	   << venv << "] cmd=[" << JoinWithString(cmd, " ") << "] llp=["
	   << llp << "] env=[" << /*ToStr(env)*/ "**not  printed**"
	   << "] " << files.size() << " files=[" << JoinWithString(files, " ")
	   << "]" << endl;

    if (cmd.empty()) {
      cerr << TimeStamp() << msg+"cmd is empty, failing" << endl;
      return {};
    }
     
    if (files.empty()) {
      cerr << TimeStamp() << msg+"files is empty, failing" << endl;
      return {};
    }
     
    // llp could be split into pieces and tested which ones exists...
    // if (DirectoryExists(lp1))
    //  lp = lp1;
    string lp = llp;

    if (lp!="") {    
      const char *l = getenv("LD_LIBRARY_PATH");
      if (l)
	lp += string(":")+string(l);
    }

    execenv_t  extfeaenv;
    if (lp!="")
      extfeaenv.insert({ "LD_LIBRARY_PATH", lp });
    for (const auto& i : env)
      extfeaenv.insert(i);

    vector<string> cmdx;
    for (const auto& i : cmd)
      if (i=="%one") {
	if (files.size()!=1) {
	  cerr << TimeStamp() << msg+"files.size()!=1" << endl;
	  return {};
	}
	cmdx.push_back(files[0]);
      } else if (i=="%commalist")
	cmdx.push_back(CommaJoin(files));
      else if (i=="%list")
	cmdx.insert(cmdx.end(), files.begin(), files.end());
      else
	cmdx.push_back(i);
    
    execargv_t extfeaargv;
    if (venv!="") {
      auto t = TempFile("extbatch_script_");
      if (!t.first) {
	cerr << TimeStamp() << msg+"TempFile() <"+t.second+"> failed" << endl;
	return {};
      }
      ofstream script(t.second);
      script << "#! /bin/sh" << endl
	     << ". " << venv << "/bin/activate" << endl
	     << ToStr(cmdx) << endl;
      script.close();
      if (FileVerbose())
	cout << TimeStamp() << msg << t.second << " = [" << FileToString(t.second)
	     << "]" << endl;

      extfeaargv = { "/bin/sh", t.second };

    } else 
      extfeaargv = cmdx;

    string eff_dir = dir;
    if (eff_dir=="%tmp") {
      eff_dir = tmpdir = get_temp_file_name("extbatch-");
      if (!EnsureDirectoryExists(eff_dir+"/foo")) {
	cerr << TimeStamp() << msg+"EnsureDirectoryExists() <"+
	  eff_dir+"> failed" << endl;
	return {};
      }
      if (FileVerbose())
	cout << TimeStamp() << msg << "created tempdir <" << eff_dir << ">"
	     << endl;
    }
    
    return { { extfeaargv, extfeaenv, "", eff_dir } };    
  }

  //===========================================================================

  bool ExtBatch::ProcessBatch() {
    string msg = "ExtBatch::ProcessBatch() : ";
    size_t nchunks = chunk_size ? (batch.size()+chunk_size-1)/chunk_size : 1;
    if (FileVerbose())
      cout << TimeStamp() << msg << batch.size()
	   << " entries to process in " << nchunks << " chunks of "
	   << (chunk_size?ToStr(chunk_size):"infinite") << endl;

    auto i = batch.begin();
    for (size_t c=0; c<nchunks; c++) {
      if (FileVerbose())
	cout << TimeStamp() << msg << "starting chunk " << c << "/" << nchunks
	     << endl;
      files.clear();
      auto i0 = i;
      for (size_t j=0; (!chunk_size||j<chunk_size)&&i!=batch.end(); j++,i++) {
	string ffname = i->fname;
	if (ffname[0]!='/')
	  ffname = Cwd()+"/"+ffname;
	if (FileVerbose())
	  cout << TimeStamp() << msg << "pre " << i->fname
	       << " -> " << ffname << endl;
	
	files.push_back(ffname);
      }

      if (FileVerbose())
	cout << TimeStamp() << msg << "CalculateCommon() called" << endl;

      bool ok = CalculateCommon(0, true, -1);

      vector<string> lines = SplitInSomething("\n", false, GetOutput());
      if (FileVerbose() || !ok)
	cout << TimeStamp() << msg << "CalculateCommon() finished with OK="
	     << (ok?"true":"false") << " and " << GetOutput().size() 
	     << " bytes of output in " << lines.size() << " lines" << endl;

      if (!ok) {
	cerr << TimeStamp() << msg+"CalculateCommon() failed" << endl;
	return false;
      }
      
      i = i0;
      size_t l = 0;
      for (size_t j=0; (!chunk_size||j<chunk_size)&&i!=batch.end(); j++,i++) {
	if (FileVerbose())
	  cout << TimeStamp() << msg << "post " << i->fname << endl;
	string label = i->fname;
	size_t p = label.rfind('/');
	if (p!=string::npos)
	  label.erase(0, p+1);
	p = label.rfind('.');
	if (p!=string::npos)
	  label.erase(p);

	string outstr, hdr = "PICSOMFEATURE ";
	for (; l<lines.size()&&outstr==""; l++)
	  if (lines[l].substr(0, hdr.size())==hdr)
	    outstr = lines[l].substr(hdr.size());
	if (outstr=="") {
	  cerr << TimeStamp() << msg+"\""+hdr+"\" not found "
	       << "when expecting label=<" <<label << ">" << endl;
	  return false;
	}
	
	vector<float> vec;
	vector<string> vs = SplitInSpaces(outstr);
	if (vs.size()<2) {
	  cerr << TimeStamp() << msg+"\""+hdr+"\" line is short" << endl;
	  return false;
	}
	string labelx = vs.back();
	vs.pop_back();
	for (const auto &s : vs)
	  vec.push_back(atof(s.c_str()));
	if (FileVerbose())
	  cout << TimeStamp() << msg << "C vec dim="<< vs.size() << " -> "
	       << vec.size() << " label=<" << label << "> labelx=<"
	       << labelx << ">" << endl;

	if (FileVerbose()) {
	  cout << TimeStamp() << msg;
	  for (size_t ii=0; ii<5; ii++)
	    cout << " " << vec[ii];
	  cout << endl;
	}

	if (label!=labelx) {
	  cerr << TimeStamp() << msg+" mismatch label=<" << label
	       << "> labelx=<"<< labelx << ">" << endl;
	  return false;
	}
	
	if (i->result) {
	  labeled_float_vector lv { vec, label };
	  i->result->append(lv);
	} else {
	  if (DataCount()==0) {
	    if (false && !fout_xx)
	      fout_xx = &cout;
	  
	    AddOneDataLabel("");
	    SetVectorLengthFake(vec.size());
	    AddDataElements(false);
	    ((VectorData*)GetData(0))->setLength(vec.size());
	  }
	  if (fout_xx)
	    Print(*fout_xx, vec, label);
	}
      }

      if (!KeepTmp()) {
	if (tmpdir!="") {
	  RmDirRecursive(tmpdir);
	  tmpdir = "";
	}
      }
    }
    
    batch.clear();
    
    return true;
  }

} // namespace picsom

//=============================================================================

// Local Variables:
// End:
