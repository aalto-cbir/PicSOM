// $Id: SIFT.C,v 1.14 2014/02/03 09:21:16 jorma Exp $	

#include <SIFT.h>
#include <fcntl.h>

namespace picsom {
  static string vcid = "$Id: SIFT.C,v 1.14 2014/02/03 09:21:16 jorma Exp $";

  static SIFT list_entry(true);

  //===========================================================================

  string SIFT::Version() const {
    return vcid;
  }

  //===========================================================================

  Feature::featureVector SIFT::getRandomFeatureVector() const {
    featureVector ret;
    return ret;
  }
  
  //===========================================================================
  
  int SIFT::printMethodOptions(ostream&) const {
    return 0;
  }

  //===========================================================================

  bool SIFT::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "SIFT::ProcessOptionsAndRemove() ";

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

      i++;
    }

    return External::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  bool SIFT::CalculateOneFrame(int f) {
    char msg[100];
    sprintf(msg, "SIFT::CalculateOneFrame(%d)", f);
    
    if (FrameVerbose())
      cout << msg << " : starting" << endl;
    
    bool ok = true;
  
    if (!External::CalculateCommon(f, true, -1))
      ok = false;

    if (use_kplabels) {
      SetDataLabelsAndIndices(true);
      AddDataElements(true);

      if (kp_list.size()!=DataCount()) {
	cerr << msg << " unable to handle kp_list.size()!=DataCount() case yet"
	     << " kp_list.size()=" << kp_list.size() << " DataCount()="
	     << DataCount() << endl;
	ok = false;
      }
    }

    int nd = DataCount(), l = 0;
    for (kp_list_t::const_iterator k=kp_list.begin();
	 ok && k!=kp_list.end(); k++) {
      if (l>=nd) {
	cerr << msg << "l>=nd l==" << l << " nd=" << nd << endl;
	ok = false;
	break;
      }

      vector<float> res(k->v.begin(), k->v.end());
      int x = (int)floor(k->x+0.5), y = (int)floor(k->y+0.5);

      if (!use_kplabels) {
	int s = SegmentScalar(f, x, y);
	l = DataIndex(s, true);

	if (LabelVerbose())
	  cout << "SIFT: x=" << x << " y=" << y << " s=" << s << " l=" << l
	       << endl;
      }

      if(l>=0){

	SIFTData *data_dst = (SIFTData*)GetData(l);
	data_dst->AddValue(res, x, y);
      }

      if (use_kplabels)
	l++;
    }

    if (FrameVerbose())
      cout << msg << "ending in " << (ok?"success":"failure") << endl;
    
    return ok;
  }

  //===========================================================================

  vector<string> SIFT::KeyPointLabels() const {
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

  External::execchain_t SIFT::GetExecChain(int /*f*/, bool /*all*/, 
					   int /*l*/) {
    const string exec = "/share/imagedb/picsom/linux/bin/sift";

    execargv_t execargv;
    execargv.push_back(exec);

    execspec_t execspec(execargv, execenv_t(), "", "");

    return execchain_t(1, execspec);
  }
  
  //===========================================================================

  bool SIFT::ExecPreProcess(int /*f*/, bool /*all*/, int /*l*/) {
    string pgm = ImageToPGM();
    if (pgm.empty())
      return false;

    SetInput(pgm);
    
    // cout << GetInput().substr(0, 30) << endl;

    return true;
  }

  //===========================================================================
  
  bool SIFT::ExecPostProcess(int f, bool all, int l) {
    string str = GetOutput();

    list<string> interm(1, str);
    SaveIntermediate(f, interm);

    return ExecPostProcessInner(str, f, all, l);
  }

  //===========================================================================
  
  bool SIFT::ProcessIntermediateFiles(const list<string>& files,
				      int f, bool all, int l) {
    list<string> interm = ReadIntermediateInner(files);
    if (interm.size()!=1)
      return false;

    return ExecPostProcessInner(*interm.begin(), f, all, l);
  }

  //===========================================================================
  
  bool SIFT::ExecPostProcessInner(const string& str,
				  int /*f*/, bool /*all*/, int /*l*/) {
    kp_list.clear();

    istringstream ss(str);
    size_t n = 0, d = 0;
    ss >> n >> d;

    if (FrameVerbose())
      cout << "SIFT::ExecPostProcess() : n=" << n << " d=" << d << endl;

    for (size_t i=0; i<n; i++) {
      float x, y, s, a;
      ss >> y >> x >> s >> a;

      vector<int> v(d);
      for (size_t j=0; j<d; j++) 
	ss >> v[j];

      if (PixelVerbose()) {
	cout << " " << x << "," << y << " " << s << " " << a << " ";
	for (size_t j=0; j<d && j<5; j++) 
	  cout << " " << v[j];
	cout << endl;
      }

      if (!ss) {
	cerr << "SIFT::ExecPostProcess() : ERROR, ended too early with "
	     << i << "/" << n << endl;
	break;
      }

      keypoint kp = { x, y, s, a, v };
      kp_list.push_back(kp); 
    }

    // And here comes the dirty trick:
    if (kp_list.size()==1)
      kp_list.push_back(*kp_list.begin());

    if (FrameVerbose())
      cout << "SIFT::ExecPostProcess() : stored " << kp_list.size()
	   << " keypoints" << endl;

    return true;
  }

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
