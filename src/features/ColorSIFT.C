// $Id: ColorSIFT.C,v 1.30 2015/10/01 10:07:49 jorma Exp $        

#include <ColorSIFT.h>
#include <fcntl.h>

namespace picsom {
  static string vcid =
    "$Id: ColorSIFT.C,v 1.30 2015/10/01 10:07:49 jorma Exp $";

  static ColorSIFT list_entry(true);

  //size_t ColorSIFT::max_width  = 400;
  //size_t ColorSIFT::max_height = 400;
  size_t ColorSIFT::max_width  = 1024;
  size_t ColorSIFT::max_height = 1024;

  //===========================================================================

  string ColorSIFT::Version() const {
    return vcid;
  }

  //===========================================================================

  Feature::featureVector ColorSIFT::getRandomFeatureVector() const {
    featureVector ret;
    return ret;
  }
  
  //===========================================================================
  
  int ColorSIFT::printMethodOptions(ostream& str) const {

    str << "Supported options: (used as -o opt=value)" << endl
	<< "  detector      : used detector in colorDescriptor" << endl
	<< "  descriptor    : used descriptor in colorDescriptor" << endl
	<< "  codebook      : codebook in cdb.tab format, produces histogram" 
        << endl
	<< "  codebookdim   : dimensionality of the codebook (default: 1000)" 
        << endl
	<< "  codebookidx   : codebook, produces clustering results, does not "
           "need codebookdim" << endl
        << "  softsigma     : use soft clustering with this sigma value, "
           "otherwise hard clustering" << endl
        << "  pointselector : for spatial pyramid use, e.g. pyramid-1x1-2x2"
        << endl
	<< "Note that \"-r\" is often useful without codebook and with "
           "codebookidx" << endl;
    
    return 0;
  }

  //===========================================================================

  bool ColorSIFT::NeedsConversion() const {
    string f = SegmentData()->format();
    if (f!="image/jpeg" && f!="image/png")
      return true;
    
    if (max_width==0 || max_height==0)
      return true;
    
    if (SegmentData()->getWidth()>(int)max_width ||
	SegmentData()->getHeight()>(int)max_height)
      return true;
    
    return false;
  }

  //===========================================================================

  bool ColorSIFT::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "ColorSIFT::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
        cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="detector") {
        cs_detector = value;
        i = l.erase(i);
        continue;
      }

      if (key=="descriptor") {
        cs_descriptor = value;
	if (cs_descriptor == "sift")
	  cs_descriptor_length = 128;
        i = l.erase(i);
        continue;
      }

      if (key=="codebook") {
	if (FileVerbose())
	  cout << msg << "Processing codebook" << endl;
        cs_codebook_path = value;
        i = l.erase(i);
	if (IsRawOutMode()) {
	  cs_codebookMode = "hardindex";
	  if (FileVerbose())
	    cout << msg << "Setting codebookMode=hardindex" << endl;

	}
        continue;
      }

      if (key=="codebookidx") {
	if (FileVerbose())
	  cout << msg << "Processing codebookidx" << endl;
        cs_codebook_path = value;
        i = l.erase(i);
	cs_codebookMode = "hardindex";
	cs_codebook_size = 1;
        continue;
      }

      if (key=="codebookdim") {
        cs_codebook_size = atoi(value.c_str());
        i = l.erase(i);
        continue;
      }

      if (key=="softsigma") {
        cs_softsigma = atof(value.c_str());
	if (FileVerbose())
	  cout << msg << "Setting soft clustering with sigma="
               << cs_softsigma << endl;
        i = l.erase(i);
        continue;
      }
      
      if (key=="pointselector") {
        cs_pointselector = value;
	if (FileVerbose())
	  cout << msg << "Setting pointselector=" << cs_pointselector << endl;
        i = l.erase(i);
        continue;
      }

      if (key=="version") {
        cs_version = value;
	if (FileVerbose())
	  cout << msg << "Setting cs_version=" << cs_version << endl;
        i = l.erase(i);
        continue;
      }

      if (key=="cs_binary") {
        cs_binary = value;
	if (FileVerbose())
	  cout << msg << "Setting cs_binary=" << cs_binary << endl;
        i = l.erase(i);
        continue;
      }

      if (key=="maxsize") {
	int w = 0, h = 0;
	if (sscanf(value.c_str(), "%dx%d", &w, &h)!=2 || w<1 || h<1)
	  cerr << msg << "failed to process maxsize=\"" << value << "\""
	       << endl;
	else {
	  max_width  = w;
	  max_height = h;
	  if (FileVerbose())
	    cout << msg << "Setting max_width=" << max_width
		 << " max_height=" << max_height << endl;
	}
	i = l.erase(i);
        continue;
      }

      i++;
    }

    return External::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  bool ColorSIFT::CalculateOneFrame(int f) {
    stringstream msgss;
    msgss << "ColorSIFT::CalculateOneFrame(" << f << ") : ";
    string msg = msgss.str();
    
    if (FrameVerbose())
      cout << msg << "starting" << endl;
    
    bool ok = true;
  
    if (!External::CalculateCommon(f, true, -1))
      ok = false;

    if (use_kplabels) {
      SetDataLabelsAndIndices(true);
      AddDataElements(true);

      if (kp_list.size()!=DataCount()) {
        cerr << msg << "unable to handle kp_list.size()!=DataCount() case yet"
             << " kp_list.size()=" << kp_list.size() << " DataCount()="
             << DataCount() << endl;
        ok = false;
      }
    }

    int nd = DataCount(), l=0;

    // if (!use_kplabels) 
    //   for(l=0;l<nd;l++){
    //     ColorSIFTData* data_dst = (ColorSIFTData *)GetData(l);
    //     data_dst->initHistogram();
    //     data_dst->Zero();
    //   }

    l=0;

    for (kp_list_t::const_iterator k=kp_list.begin();
         ok && k!=kp_list.end(); k++) {
      if (l>=nd) {
        cerr << msg << "l>=nd l==" << l << " nd=" << nd << endl;
        ok = false;
        break;
      }

      const vector<float>& res = k->v;
      int x = (int)floor(k->x+0.5), y = (int)floor(k->y+0.5);

      if (!use_kplabels) {
        bool segm_vect = true;

        if (segm_vect) {
          vector<int> sv = SegmentVector(f, x, y);
          for (size_t svi=0; svi<sv.size(); svi++) {
            int s = sv[svi];
            l = DataIndex(s, true);
            if (LabelVerbose())
              cout << "ColorSIFT: x=" << x << " y=" << y
		   << " s=" << s << " l=" << l << endl;

	    if(l>=0){
	      ColorSIFTData *data_dst = (ColorSIFTData *)GetData(l);
	      data_dst->AddValue(res, x, y);
	    }
          }
          continue;

        } else {
          int s = SegmentScalar(f, x, y);
          l = DataIndex(s, true);

          if (LabelVerbose())
            cout << "ColorSIFT: x=" << x << " y=" << y
		 << " s=" << s << " l=" << l << endl;
        }
      }

      ColorSIFTData *data_dst = (ColorSIFTData *)GetData(l);
      data_dst->AddValue(res, x, y);

      if (use_kplabels)
        l++;
    }
  
    if (FrameVerbose())
      cout << msg << "ending in " << (ok?"success":"failure") << endl;
  
    return ok;
  }

  //===========================================================================

  vector<string> ColorSIFT::KeyPointLabels() const {
    int n = kp_list.size()>1 ? kp_list.size() : 2;
    size_t l = (size_t)ceil(log10(n-1.0));

    vector<string> ret(kp_list.size());
    for (size_t i=0; i<kp_list.size(); i++) {
      stringstream ss;
      ss << "00000" << i;
      string s = ss.str();
      ret[i] = s.substr(s.size()-l);
    }
    return ret;
  }

  //===========================================================================

  External::execchain_t ColorSIFT::GetExecChain(int /*f*/, bool /*all*/, 
                                            int /*l*/) {
    execchain_t execchain;

    string cd_bin = cs_binary;

    // If cs_binary is not set try to resolve automatically
    if (cd_bin.empty()) {
      const string arch = SolveArchitecture();

      if (arch != "linux" && arch != "linux64" && arch != "darwin") {
        cerr << "ERROR: architecture is " << arch 
	     << ", should be linux, linux64, or darwin!!" << endl;
        return execchain; 
      }

      cd_bin = PicSOMroot()+"/"+arch+"/bin/colorDescriptor";
      if (!FileExists(cd_bin)) {
	cout << "colorDescriptor binary not found at " 
	     << cd_bin << endl
	     << "Using remote execution instead." << endl;
	binary_not_found = true;
      }

      // passwd *pw = getpwuid(getuid());
      // string uhdir = pw ? pw->pw_dir : "";
      // string cd_bin_own = uhdir+"/picsom/"+arch+"/bin/colorDescriptor";
      // if (!FileExists(cd_bin_own))
      // 	cd_bin_own = uhdir+"/picsom/bin/colorDescriptor";
      // if (FileExists(cd_bin_own))
      // 	cd_bin = cd_bin_own;
      // else
      // 	cd_bin = COLORDESCRIPTOR_BASE_PATH + arch + "/bin/" +
      // 	  "colorDescriptor" + cs_version;
    }

    if (MethodVerbose())
      cout << "ColorSIFT using binary: " << cd_bin << endl;

    execargv_t cd_argv;
    tempfiles["output"]      = tempfiles["base"]+".output";

    cd_argv.push_back(cd_bin);
    cd_argv.push_back(tempfiles["image"]);

    tempfiles["errorlog"] = tempfiles["base"]+"_error.log";
    cd_argv.push_back("--logFile");
    cd_argv.push_back(tempfiles["errorlog"]);
    
    cd_argv.push_back("--detector");
    cd_argv.push_back(cs_detector);
    
    cd_argv.push_back("--descriptor");
    cd_argv.push_back(cs_descriptor);
   
    if (!cs_codebook_path.empty()) {
      cd_argv.push_back("--codebook");
      cd_argv.push_back(cs_codebook_path);
    }

    if (!cs_codebookMode.empty()) {
      cd_argv.push_back("--codebookMode");
      cd_argv.push_back(cs_codebookMode);
    }
    
    if (cs_softsigma > 0.0) {
      cd_argv.push_back("--codebookMode");
      cd_argv.push_back("unc");
      cd_argv.push_back("--codebookSigma");
      cd_argv.push_back(ToStr(cs_softsigma));
    }

    if (!cs_pointselector.empty()) {
      cd_argv.push_back("--pointSelector");
      cd_argv.push_back(cs_pointselector);
    } 

    cd_argv.push_back("--output");
    cd_argv.push_back(tempfiles["output"]);

    execchain.push_back(execspec_t(cd_argv, execenv_t(), "", ""));

    return execchain;
  }
  
  //===========================================================================

  bool ColorSIFT::ExecPreProcess(int /*f*/, bool /*all*/, int /*l*/) {
    string msg = "ColorSIFT::ExecPreProcess() : ";

    pair<bool,string> tmpfn = TempFile();
    string tmpbase = tmpfn.second;

    if (!tmpfn.first) {
      cerr << msg+"ERROR: failed to create temporary file <"+tmpbase+">"
	   << endl;
      return false;
    }

    tempfiles["base"] = tmpbase;

    // Make symlink from actual image to temp directory
    string imagepath = CurrentFullDataFileName();
    string ext = "";
    string::size_type dotpos   = imagepath.find_last_of(".");
    if (dotpos!=string::npos) 
      ext = imagepath.substr(dotpos);

    // small hack because colorDescriptor doesn't understand the .jpeg extension
    if (ext == ".jpeg")
      ext = ".jpg";
    string tempimage = tmpbase+ext;

    stringstream sstr;
    sstr << "symlink: " << tempimage << " -> " << imagepath;
    if (FrameVerbose())
      cout << msg << "making " << sstr.str() << endl;
    if (symlink(imagepath.c_str(),tempimage.c_str()) == -1) {
      cerr << msg << "ERROR: could not create " << sstr.str() << endl;
      return false;
    }
    
    tempfiles["image"] = tempimage;

    return true;
  }

  //===========================================================================
  
  bool ColorSIFT::ExecPostProcess(int f, bool all, int l) {
    string str;
    bool ok = ExecPostProcessInner(str, f, all, l);
    RemoveTempFiles();
    return ok;
  }

  //===========================================================================
  
/*  bool ColorSIFT::ProcessIntermediateFiles(const list<string>& files,
                                      int f, bool all, int l) {
    list<string> interm = ReadIntermediateInner(files);
    if (interm.size()!=1)
      return false;

    return ExecPostProcessInner(*interm.begin(), f, all, l);
  }*/

  //===========================================================================
  
  bool ColorSIFT::ExecPostProcessInner(const string& /*str*/,
				       int /*f*/, bool /*all*/, int /*l*/) {
    string msg = "ColorSIFT::ExecPostProcessInner() <"+ObjectDataFileName()+
      "> : ";
    kp_list.clear();

    ifstream fp(tempfiles["output"].c_str());
    if (!fp) {
      cerr << msg << "could not open file: " << tempfiles["output"] << endl;
      return false;
    }
    
    string line;
    fp >> line;
    if (line != "KOEN1") {
      cerr << msg << "bad output file format, doesn't start with KOEN1: " 
           << tempfiles["output"] << endl;
      return false;
    }
    
    size_t d =  cs_codebook_path.empty() ?
      cs_descriptor_length : cs_codebook_size;

    fp >> line;
    size_t dd = atoi(line.c_str());    
    fp >> line;
    size_t nn = atoi(line.c_str());
 
    // desperate hack!
    bool concat=false;
    if (nn>1 && !cs_pointselector.empty()) {
      concat=true;
    } else {
      nn=1;
    }
    
    if (dd*nn != d) {
      cerr << msg << "incorrect dimensionality in output, "
           << line << "!=" << d << " in file: " << tempfiles["output"] << endl;
      if (!cs_codebook_path.empty())
	cerr << msg << "maybe you should set also codebookdim" << endl;
      return false;
    }

    // desperate hack, cont...
    vector<float> v(d);
    size_t voff = 0;

    while (!fp.eof()) {
      fp >> line;
      if (line == "<CIRCLE") {
        // <CIRCLE x y scale orientation cornerness>
        int x, y;
        double s, o;
        string c;
        fp >> x >> y >> s >> o >> c;
        
        for (size_t j=0; j<dd; j++) 
          fp >> v[j+voff];
        
        if (concat)
          voff += dd;

        if (PixelVerbose()) {
          cout << "[" << x << "," << y << " " << s << "]:";
          for (size_t j=0; j<dd; j++) 
            cout << " " << v[j];
          cout << endl;
        }
        
        if (!concat) {
          keypoint kp = { (float)x, (float)y, (float)s, (float)o, v };
          kp_list.push_back(kp); 
        }
      }
    }

    if (concat) {
      keypoint kp = { 0, 0, 0, 0, v };
      kp_list.push_back(kp);
    }

    // First comes the second dirty trick:
    if (kp_list.size()==0) {
      vector<float> v(d);
      keypoint kp = { 0, 0, 0, 0, v };
      kp_list.push_back(kp); 
    }

    // (Mats 25.11.2009) Commented away this "dirty trick". Seems to work
    // without it now, and it caused the histogram output (only one line, 
    // i.e. kp_list.size()==1) to be counted doubly, i.e. multiplied by 2.
    //
    // And here comes the dirty trick:
    //      if (kp_list.size()==1)
    //        kp_list.push_back(*kp_list.begin());

    if (FrameVerbose())
      cout << msg+"stored " << kp_list.size() << " keypoints" << endl;

    return true;
  }

  //===========================================================================

  bool ColorSIFT::RemoveTempFiles() {
    if (KeepTmp())
      return true;

    bool ok = true;

    for (map<string,string>::const_iterator i=tempfiles.begin();
         i!=tempfiles.end(); i++)
      if (!Unlink(i->second))
        ok = false;

    return ok;
  }
  
  //===========================================================================
  
  //===========================================================================
  
} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
