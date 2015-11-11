// $Id: IPLD.C,v 1.15 2014/02/03 09:21:16 jorma Exp $	

#include <IPLD.h>
#include <fcntl.h>

namespace picsom {
  static string vcid = "$Id: IPLD.C,v 1.15 2014/02/03 09:21:16 jorma Exp $";

  static IPLD list_entry(true);

  //===========================================================================

  string IPLD::Version() const {
    return vcid;
  }

  //===========================================================================

  Feature::featureVector IPLD::getRandomFeatureVector() const {
    featureVector ret;
    return ret;
  }
  
  //===========================================================================
  
  int IPLD::printMethodOptions(ostream&) const {
    return 0;
  }

  //===========================================================================

  bool IPLD::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "IPLD::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="kplabels") {
	use_kplabels = IsAffirmative(value);
	i = l.erase(i);
	continue;
      }

      if (key=="har") {
	use_har = IsAffirmative(value);
	i = l.erase(i);
	continue;
      }

      if (key=="dog") {
	use_dog = IsAffirmative(value);
	i = l.erase(i);
	continue;
      }

      if (key=="unnorm") {
	use_unnorm = IsAffirmative(value);
	i = l.erase(i);
	continue;
      }

      i++;
    }

    return External::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  bool IPLD::CalculateOneFrame(int f) {
    stringstream msgss;
    msgss << "IPLD::CalculateOneFrame(" << f << ") : ";
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

    int nd = DataCount(), l = 0;

    // if (!use_kplabels) 
    //   for(l=0;l<nd;l++){
    //     ((IPLDData *)GetData(l))->initHistogram();
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
	      cout << "IPLD: x=" << x << " y=" << y << " s=" << s << " l=" << l
		   << endl;

	    if(l>=0){

	      IPLDData *data_dst = (IPLDData *)GetData(l);
	      data_dst->AddValue(res, x, y);
	    }
	  }
	  continue;

	} else {
	  int s = SegmentScalar(f, x, y);
	  l = DataIndex(s, true);

	  if (LabelVerbose())
	    cout << "IPLD: x=" << x << " y=" << y << " s=" << s << " l=" << l
		 << endl;
	}
      }

      IPLDData *data_dst = (IPLDData *)GetData(l);
      data_dst->AddValue(res, x, y);

      if (use_kplabels)
	l++;
    }
  
    if (FrameVerbose())
      cout << msg << "ending in " << (ok?"success":"failure") << endl;
  
    return ok;
  }

  //===========================================================================

  vector<string> IPLD::KeyPointLabels() const {
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

  External::execchain_t IPLD::GetExecChain(int /*f*/, bool /*all*/, 
					    int /*l*/) {
    string detect   = FindExecutable("Detect");
    string compdesc = FindExecutable("ComputeDescriptor");
    string corners  = FindExecutable("corners2text");

    execchain_t execchain;

    if (use_har) {
      execargv_t detectargv, cmpdscargv, cornerargv;
      tempfiles["har"]      = tempfiles["base"]+".har";
      tempfiles["har_sift"] = tempfiles["base"]+".har_sift";
      tempfiles["har_out"]  = tempfiles["base"]+".har_out";

      detectargv.push_back(detect);
      detectargv.push_back("-dt");
      detectargv.push_back("har");
      detectargv.push_back(tempfiles["pgm"]);
      detectargv.push_back(tempfiles["har"]);

      cmpdscargv.push_back(compdesc);
      if (use_unnorm)
	cmpdscargv.push_back("-unnormalized");
      cmpdscargv.push_back(tempfiles["pgm"]);
      cmpdscargv.push_back(tempfiles["har"]);
      cmpdscargv.push_back(tempfiles["har_sift"]);

      cornerargv.push_back(corners);
      cornerargv.push_back(tempfiles["har_sift"]);
      cornerargv.push_back("-adddesc");
      cornerargv.push_back(tempfiles["har_out"]);

      execchain.push_back(execspec_t(detectargv, execenv_t(), "&&", ""));
      execchain.push_back(execspec_t(cmpdscargv, execenv_t(), "&&", ""));
      execchain.push_back(execspec_t(cornerargv, execenv_t(), "",   ""));
    }

    if (use_dog) {
      execargv_t detectargv, cmpdscargv, cornerargv;
      tempfiles["dog"]      = tempfiles["base"]+".dog";
      tempfiles["dog_sift"] = tempfiles["base"]+".dog_sift";
      tempfiles["dog_out"]  = tempfiles["base"]+".dog_out";

      detectargv.push_back(detect);
      detectargv.push_back("-dt");
      detectargv.push_back("dog");
      detectargv.push_back(tempfiles["pgm"]);
      detectargv.push_back(tempfiles["dog"]);

      cmpdscargv.push_back(compdesc);
      if (use_unnorm)
	cmpdscargv.push_back("-unnormalized");
      cmpdscargv.push_back(tempfiles["pgm"]);
      cmpdscargv.push_back(tempfiles["dog"]);
      cmpdscargv.push_back(tempfiles["dog_sift"]);
      
      cornerargv.push_back(corners);
      cornerargv.push_back(tempfiles["dog_sift"]);
      cornerargv.push_back("-adddesc");
      cornerargv.push_back(tempfiles["dog_out"]);

      execchain.push_back(execspec_t(detectargv, execenv_t(), "&&", ""));
      execchain.push_back(execspec_t(cmpdscargv, execenv_t(), "&&", ""));
      execchain.push_back(execspec_t(cornerargv, execenv_t(), "",   ""));
    }

    return execchain;
  }
  
  //===========================================================================

  bool IPLD::ExecPreProcess(int /*f*/, bool /*all*/, int /*l*/) {
    string msg = "ERROR: IPLD::ExecPreProcess() : ";

    pair<bool,string> tmpfn = TempFile();
    string tmpbase = tmpfn.second;

    if (!tmpfn.first) {
      cerr << msg+"failed to create temporary file <"+tmpbase+">" << endl;
      return false;
    }

    tempfiles["base"] = tmpbase;

    string tmppgm = tmpbase+".pgm";
    bool ok = ImageToPGM(tmppgm);
    if (!ok) {
      cerr << msg+": ImageToPGM("+tmppgm+") failed" <<endl;
      return false;
    }

    tempfiles["pgm"] = tmppgm;

    return true;
  }

  //===========================================================================
  
  bool IPLD::ExecPostProcess(int f, bool all, int l) {
    string str;
    if (use_har)
      str += ReadFile(tempfiles["har_out"]);
    if (use_dog)
      str += ReadFile(tempfiles["dog_out"]);

    RemoveTempFiles();

    // list<string> interm(1, str);
    // SaveIntermediate(f, interm);

    return ExecPostProcessInner(str, f, all, l);
  }

  //===========================================================================
  
  bool IPLD::ProcessIntermediateFiles(const list<string>& files,
				      int f, bool all, int l) {
    list<string> interm = ReadIntermediateInner(files);
    if (interm.size()!=1)
      return false;

    return ExecPostProcessInner(*interm.begin(), f, all, l);
  }

  //===========================================================================
  
  bool IPLD::ExecPostProcessInner(const string& str,
				  int /*f*/, bool /*all*/, int /*l*/) {
    string msg = "IPLD::ExecPostProcessInner() : ";
    kp_list.clear();

    istringstream ss(str);
    size_t d = 128;

    if (FrameVerbose())
      cout << msg+"d=" << d << " str.size()=" << str.size() << endl;

    for (size_t i=0;; i++) {
      float x, y, s, a;
      ss >> x >> y >> s >> a;
      
      if (!ss)
	break;

      vector<float> v(d);
      for (size_t j=0; j<d; j++) 
	ss >> v[j];

      if (PixelVerbose()) {
	cout << " " << x << "," << y << " " << s << " " << a << " ";
	for (size_t j=0; j<d && j<5; j++) 
	  cout << " " << v[j];
	cout << endl;
      }

      keypoint kp = { x, y, s, a, v };
      kp_list.push_back(kp); 
    }

    // First comes the second dirty trick:
    if (kp_list.size()==0) {
      vector<float> v(d);
      keypoint kp = { 0, 0, 0, 0, v };
      kp_list.push_back(kp); 
    }

    // And here comes the dirty trick:
    if (kp_list.size()==1)
      kp_list.push_back(*kp_list.begin());

    if (FrameVerbose())
      cout << msg+"stored " << kp_list.size() << " keypoints" << endl;

    return true;
  }

  //===========================================================================

  bool IPLD::RemoveTempFiles() {
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
