// $Id: CVC.C,v 1.3 2014/06/18 14:48:19 jorma Exp $	

#include <CVC.h>
#include <cvc-descriptors.h>

namespace picsom {
  static const string vcid =
    "$Id: CVC.C,v 1.3 2014/06/18 14:48:19 jorma Exp $";

  static CVC list_entry(true);

  //===========================================================================

  string CVC::Version() const {
    return vcid;
  }

  //===========================================================================

  bool CVC::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "CVC::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="descriptor") {
	descriptor = descriptor_string2int(value);
    	i = l.erase(i);
      	continue;
      }

      if (key=="grid") {
	grid = atoi(value.c_str());
    	i = l.erase(i);
      	continue;
      }

      if (key=="radius") {
	radius = atof(value.c_str());
    	i = l.erase(i);
      	continue;
      }

      if (key=="blocks") {
	blocks = SplitInCommas(value);
    	i = l.erase(i);
      	continue;
      }

      // if (key=="") {
      // 	zzz = value;
      
      // 	i = l.erase(i);
      // 	continue;
      // }

      i++;
    }

    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  int CVC::descriptor_string2int(const string& s) {
    if (s=="cn")
      return 1;
    if (s=="hue")
      return 2;
    if (s=="dcd")
      return 3;
    if (s=="josa")
      return 4;
    if (s=="opp")
      return 5;

    return 0;
  }

  //===========================================================================

  const string& CVC::descriptor_int2string(int i) {
    static vector<string> v { "cn", "hue", "dcd", "josa", "opp" };
    if (i<1 || i>5) {
      throw "bye!";
    }
    return v[i-1];
  }

  //===========================================================================

  int CVC::printMethodOptions(ostream &str) const {
    /// obs! dunno yet
    return Feature::printMethodOptions(str);
  }

  //===========================================================================

  // Feature::comp_descr_t CVC::ComponentDescriptions() const {
  //   comp_descr_t d(PixelTypeSize());
  //   return d;
  // }

  //===========================================================================

  Feature::featureVector CVC::getRandomFeatureVector() const {
    featureVector ret;

    ret.push_back(rand()/(float)RAND_MAX); 
    ret.push_back(rand()/(float)RAND_MAX); 
    ret.push_back(rand()/(float)RAND_MAX); 

    return ret;
  }

  //===========================================================================

  bool CVC::CalculateOneFrame(int /*f*/) {
    string fname = CurrentFullDataFileName();
    char *imageFile = (char*)fname.c_str();
    char *detectorFile = (char*)DetectorFile().c_str();
    int patch_size = 21, scalef = 5, num_bins = 36, smooth_flag = 0;
    double sigma = 21/4.0;
    bool fast = true;
    int probs = 25;
    stringstream out;

    SetDataLabelsAndIndices(true);
    AddDataElements(true);

    if (FileVerbose())
      cout << "CVC::CalculateOneFrame() : starting with descriptor="
	   << descriptor << " (" << descriptor_int2string(descriptor)
	   << ") grid=" << grid << " radius=" << radius << endl; 

    setCVCdir(PicSOMroot()+"/share/cvc");

    switch (descriptor) {
    case 1: descriptorCN(imageFile,detectorFile,patch_size,scalef,sigma,0,out);
      break;
    case 2: descriptorHUE(imageFile,detectorFile,patch_size,scalef,sigma,num_bins,smooth_flag,out);
      break;
    case 3: descriptorDCD(imageFile,detectorFile,patch_size,scalef,sigma,fast,probs,out);
      break;
    case 4: descriptorCN(imageFile,detectorFile,patch_size,scalef,sigma,1,out);
      break;
    case 5: descriptorOPP(imageFile,detectorFile,patch_size,scalef,sigma,num_bins,smooth_flag,out);
      break;
    default:
      cerr << "descriptor not among 1..5" << endl;
      return false;
    }

    // cout << out.str() << endl;

    size_t dim = 0, nl = 0;
    list<cvc_descriptor> l;
    for (size_t lno=0;; lno++) {
      string line;
      getline(out, line);
      if (!out)
	break;

      if (lno==0) {
	if (line!="MARC1")
	  break;
	else
	  continue;
      }

      if (lno==1) {
	dim = atoi(line.c_str());
	continue;
      }

      if (lno==2) {
	nl = atoi(line.c_str());
	continue;
      }

      size_t p = line.find(">;");
      if (line[0]!='<' || p==string::npos || line[line.size()-1]!=';') {
	break;
      }
      vector<string> vs = SplitInSpaces(line.substr(1, p-1));
      if (vs.size()!=6)
	break;

      cvc_descriptor d;
      d.type = vs[0];
      d.x = atoi(vs[1].c_str());
      d.y = atoi(vs[2].c_str());
      d.r = atoi(vs[3].c_str());
      d.a = atoi(vs[4].c_str());
      d.b = atoi(vs[5].c_str());

      string ss = line.substr(p+2);
      while (ss[0]==' ')
	ss.erase(0, 1);
      ss.erase(ss.size()-1);
      vs = SplitInSpaces(ss);
      if (vs.size()!=dim)
	break;

      for (size_t i=0; i<vs.size(); i++)
	d.v.push_back(atof(vs[i].c_str()));

      l.push_back(d);
    }

    if (!nl || l.size()!=nl) {
      cerr << "????" << endl;
      return false;
    }

    vector<pair<string, vector<size_t> > > reg;
    for (size_t i=0; i<blocks.size(); i++) {
      string s = blocks[i];
      size_t p = s.find('x');
      if (p!=string::npos) {
	size_t w = atoi(s.c_str());
	size_t h = atoi(s.substr(p+1).c_str());
	for (size_t j=0; j<h; j++)
	  for (size_t k=0; k<w; k++) {
	    string lab = s+"_"+ToStr(k)+"_"+ToStr(j);
	    size_t x0 = k    *Width()/w;
	    size_t x1 = (k+1)*Width()/w;
	    size_t y0 = j    *Height()/h;
	    size_t y1 = (j+1)*Height()/h;
	    if (LabelVerbose())
	      cout << lab << " " << x0 << "," << y0 << " - " << x1 << "," << y1
		   << endl;
	    vector<size_t> v { x0, y0, x1, y1 };
	    reg.push_back(make_pair(lab, v));
	  }
      }
    }

    for (auto i=l.begin(); i!=l.end(); i++)
      for (size_t l=0; l<reg.size(); l++)
	if (i->x>=reg[l].second[0] && i->x<reg[l].second[2] &&
	    i->y>=reg[l].second[1] && i->y<reg[l].second[3]) {
	  if (KeyPointVerbose())
	    cout << l << " " << reg[l].first << " " << i->str() << " " << endl;
	  
	  ((CVCData*)GetData(l))->AddValue(i->v, i->x, i->y);
	}

    return true;
  }

  //===========================================================================

  const string& CVC::DetectorFile() {
    const string fn = CurrentFullDataFileName();
    auto d = detectorfile_map.find(fn);
    if (d==detectorfile_map.end()) {
      while (detectorfile_map.size()) {
	Unlink(detectorfile_map.begin()->second);
	detectorfile_map.erase(detectorfile_map.begin());
      }

      pair<bool,string> tmp = TempFile("cvc_detector_");
      string dn = tmp.second+BaseFileName()+".txt";

      if (FileVerbose())
	cout << "SOON creating <" << dn << ">" << endl;

      size_t g = grid;
      size_t y0 = g-1, x0 = g-1, y1 = Height()-g, x1 = Width()-g;
      size_t dy = g, dx = g;
      if (!g) {
	y0 = Height()/2;
	x0 = Width()/2;
	y1 = y0+1;
	x1 = x0+1;
	dx = dy = Width()+Height();
      }

      list<pair<size_t,size_t> > l;
      for (size_t y=y0; y<y1; y+=dy)
	for (size_t x=x0; x<x1; x+=dx)
	  l.push_back(make_pair(x, y));

      ofstream os(dn);
      os << "KOEN1" << endl
	 << "0" << endl
	 << l.size() << endl;
      for (auto i=l.begin(); i!=l.end(); i++)
	os << "<CIRCLE " << i->first << " " << i->second
	   << " " << radius << " 0 0>;;" << endl;

      detectorfile_map[fn] = dn;

      d = detectorfile_map.find(fn);
    }

    return d->second;
  }
  
  //===========================================================================


  //===========================================================================

} // namespace picsom

//=============================================================================

// Local Variables:
// mode: font-lock
// End:

