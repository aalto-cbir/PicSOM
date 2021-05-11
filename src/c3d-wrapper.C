// -*- C++ -*-  $Id: c3d-wrapper.C,v 1.4 2020/03/31 14:43:04 jormal Exp $
// 
// Copyright 1998-2020 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <imagefile.h>
#include <videofile.h>
#include <iostream>
#include <Util.h>
using namespace std;
using namespace picsom;

int main(int argc, char **argv) {
  bool images = true, reencode = false;
  
  string wrapper = argv[0];
  if (argc<7) {
    cerr << wrapper << " : missing arguments, usage:" << endl
	 << wrapper << " c3d.bin prototxt model layer pooling file ..." << endl;
      return 1;
  }
  string cmdbin = argv[1];
  string proto  = argv[2];
  string model  = argv[3];
  string layer  = argv[4];
  string pool   = argv[5];

  if (pool!="avg" && pool!="max") {
    cerr << wrapper << " : pool should be either avg or max" << endl;
    return 1;
  }

  string path = cmdbin;
  size_t p = path.find("/bin/");
  if (p!=string::npos)
    videofile::prepend_bin_path(path.substr(0, p+4));
  
  vector<string> layers = SplitInCommas(layer);

  vector<pair<string,string> > ll;
  map<string,list<string> > r;

  string listf = images ? "ucf101_video_frame.list" : "video.list";
  size_t n = 0;
  ofstream vl(listf), pl("prefix.list");
  for (int i=6; i<argc; i++) {
    string vfi = argv[i], vfo = vfi;

    if (reencode) {
      size_t p = vfo.rfind('/');
      if (p==string::npos) {
	cerr << wrapper << " cannot handle " << vfo << endl;
	return 4;
      }
      vfo.erase(0, p+1);
      videofile vid(vfi);
      vid.reencode(vfo, "");
    }
    
    imagefile v(vfo);
    int nft = v.nframes();
    if (nft<1) {
      cerr << wrapper << " no frames in " << vfo << endl;
      ll.push_back({"",""});
      continue;
    }
    if (images) {
      for (int f=0; f<v.nframes(); f++)
	v.frame(f);
      nft = v.nframes();
    }
    bool expand = nft<16;
    int nf = expand ? 16 : nft;
    
    string lab = vfo;
    size_t p = lab.rfind('/');
    if (p!=string::npos)
      lab.erase(0, p+1);
    p = lab.rfind('.');
    if (p!=string::npos)
      lab.erase(p);
    
    string vfol = images ? lab : vfo;
    if (images && mkdir(lab.c_str(), 0770)) {
      cerr << wrapper << " failed to mkdir(" << lab << ")" << endl;
      return 5;
    }

    string px = "out_"+ToStr(i-6);
    ll.push_back({lab, px});
    r[px] = {};
    for (int f=0; f<nf; f+=2) {
      if (images) {
	char tmp[100];
	sprintf(tmp, "%06d.jpg", f);
	string ffile = lab+"/"+string(tmp);
	imagedata img = v.frame(f>=nft ? nft-1 : f);
	try {
	  imagefile::write(img, ffile);
	} catch (const string&) {
	  cerr << wrapper << " failed write to <" << ffile << ">" << endl;
	  return 6;
	}
      }
      if (f%16)
	continue;

      if (f>=nf-15)
	break;
      
      string pz = px+"_"+ToStr(f);
      r[px].push_back(pz);
      pl << pz << endl;
      vl << vfol << " " << f << " 0" << endl;
      n++;
    }
  }
  vl.close();
  pl.close();

  size_t nmb = (n+29)/30;
  string cmd = cmdbin+" "+proto+" "+model+" -1 30 "+ToStr(nmb)+" prefix.list";
  for (const auto& l : layers)
    cmd += " "+l;

  //cout << "[" << cmd << "]" << endl;
  int s = system(cmd.c_str());
  if (s) {
    cerr << wrapper << " : system(" << cmd << ") failed" << endl;
    return 2;
  }

  for (const auto& i : ll) {
    list<vector<float> > vec;
    for (const auto& j : r[i.second]) {
      vector<float> v;
      for (const auto& k : layers) {
	string f = j+"."+k;
	int s = FileSize(f);
	if (s<20) {
	  cerr << wrapper << " : file <" << f << "> not found or small" 
	       << endl;
	  return 3;
	}
	vector<float> a((s-20)/sizeof(float));
	ifstream is(f);
	is.seekg(20);
	is.read((char*)&a[0], a.size()*sizeof(float));
	if (!is)  {
	  cerr << wrapper << " : reading file <" << f << "> failed" << endl;
	  return 4;
	}
	v.insert(v.end(), a.begin(), a.end());
      }
      vec.push_back(v);
    }

    vector<float> v(vec.front().size());

    if (pool=="avg")
      for (const auto& i : vec)
	for (size_t j=0; j<v.size(); j++)
	  v[j] += i[j]/vec.size();
	
    
    if (pool=="max") {
      bool first = true;
      for (const auto& i : vec)
	if (first) {
	  v = i;
	  first = false;
	} else
	  for (size_t j=0; j<v.size(); j++)
	    if (i[j]>v[j])
	      v[j] = i[j];
    }
    
    cout << "PICSOMFEATURE " << ToStr(v) << " " << i.first << endl;
  }
  
  return 0;
}


