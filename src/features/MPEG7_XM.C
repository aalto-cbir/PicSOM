// -*- C++ -*-  $Id: MPEG7_XM.C,v 1.88 2014/02/03 09:21:16 jorma Exp $
// 
// Copyright 1998-2014 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <MPEG7_XM.h>
#include <fcntl.h>

namespace picsom {
  static const char *vcid =
    "$Id: MPEG7_XM.C,v 1.88 2014/02/03 09:21:16 jorma Exp $";

  static MPEG7_XM list_entry(true);
  static MPEG7_XM::XMLScalableColorHandler list_sch(true);
  static MPEG7_XM::XMLColorLayoutHandler list_clh(true);
  static MPEG7_XM::XMLContourShapeHandler list_cshh(true);
  static MPEG7_XM::XMLColorStructureHandler list_csth(true);
  static MPEG7_XM::XMLEdgeHistogramHandler list_ehh(true);
  static MPEG7_XM::XMLRegionShapeHandler list_rsh(true);
  static MPEG7_XM::XMLDominantColorHandler list_dch(true);
  static MPEG7_XM::XMLHomogeneousTextureHandler list_hth(true);
  static MPEG7_XM::XMLFaceRecognitionHandler list_frh(true);
  static MPEG7_XM::XMLAdvancedFaceRecognitionHandler list_afrh(true);
  static MPEG7_XM::XMLMotionActivityHandler list_mah(true);
  static MPEG7_XM::XMLGoFGoPColorHandler list_ggc(true);

  const MPEG7_XM::XMLDescriptorHandler
  *MPEG7_XM::XMLDescriptorHandler::list_of_methods;

  // Prefix to use for feature names
  string MPEG7_XM::PREFIX="MPEG7-";

  //===========================================================================

  string MPEG7_XM::Version() const {
    return vcid;
  }

  //===========================================================================

  Feature::featureVector MPEG7_XM::getRandomFeatureVector() const{
    featureVector ret;
    return ret;
  }

  //===========================================================================

  int MPEG7_XM::printMethodOptions(ostream&) const {
    return 0;
  }

  //===========================================================================

  bool MPEG7_XM::CalculateOneFrame(int f) {
    char msg[100];
    sprintf(msg, "MPEG7_XM::CalculateOneFrame(%d)", f);
  
    bool ok = true;
  
    int no = DataElementCount();  // this is 0 with FaceRecognition
  
    if (FrameVerbose())
      cout << msg << " : starting with DataElementCount()=" << no << endl;

    int n = no ? no : 1;

    for (int i=0; i<n; i++) {
      tempname.push_back("");
      templog.push_back("");
      temppar.push_back("");
      tempxml.push_back("");
      tempimage.push_back("");
      tempmask.push_back("");
    }

    for (int i=0; i<n; i++)
      if (!SegmentPreProcess(f, i))
	ok = false;

    if (no==0) {
      n = DataElementCount();
      if (FrameVerbose()) {
	if (n)
	  cout << msg << " : DataElementCount() changed, setting now n="
	       << n << endl;
	else
	  cout << msg << " : DataElementCount() still 0, setting now n=0"
	       << endl;
      }
      AddDataElements(true);
    }

    if (n && !CalculateOneLabel(f, 0))
      ok = false;
  
    for (int i=0; i<n; i++)
      if (!SegmentPostProcess(f, i))
	ok = false;

    if (ok) {
      for (int i=0; i<n; i++)
	SegmentRemoveTempFiles(i);
    }

    if (FrameVerbose())
      cout << msg << " : ending" << endl;
  
    return ok;
  }

  //===========================================================================

  //   virtual bool CalculateCommon(int f, bool all, int l = -1) {
  //     ExecPreProcess(f,all,l);
  //     ExecRun(f,all,l);
  //     ExecPostProcess(f,all,l);
  //     return true;
  //   }

  //===========================================================================

  bool MPEG7_XM::SegmentPreProcess(int f, int l) {
    string msg = "MPEG7_XM::SegmentPreProcess("+ToStr(f)+", "+ToStr(l)+") : ";

    handler->SegmentPrePreProcess();

    int this_seg = InverseDataIndex(l, true);

    if (FrameVerbose()) 
      cout << msg << "this_seg=" << this_seg << endl;

    // create dummy file (reserve namespace... sort of...)
    pair<bool,string> tmpfn = TempFile("feature_mpeg7xm_");
    tempname[l] = tmpfn.second;
  
    if (!tmpfn.first)
      cerr  << msg << "could not create temporary file "
	    << tempname[l] << endl;
    else if (FrameVerbose()) 
      cout << msg << "created tempfile " << tempname[l] << endl;

    int mode = 0666;
    chmod(tempname[l].c_str(), mode);

    // get names of image and segment files
    string imagepath = CurrentFullDataFileName();
    string segmpath  = SegmentData()->getSegmentFileName();
  
    bool converted = NeedsConversion();

    if (FrameVerbose()) 
      cout << msg << "imagepath=" << imagepath << " converted=" << converted
	   << endl;
  
    // Make symlink from actual image to temp directory
    string ext = "";
    string::size_type dotpos = imagepath.find_last_of(".");
    if (dotpos!=string::npos)
      ext = imagepath.substr(dotpos);
    tempimage[l] = tempname[l]+ext;
    
    if (FrameVerbose())
      cout << msg << "making symlink " << tempimage[l]
	   << " -> " << imagepath << endl;
    
    if (symlink(imagepath.c_str(),tempimage[l].c_str()) == -1) {
      cerr << msg << "error! could not create symlink " << 
	tempimage[l] << " -> " << imagepath << endl;
      return false;
    }

    // Convert seg to pbm
    imagedata mask(Width(), Height(), 1, imagedata::pixeldata_uchar);

    bool not_all_pixels = false;
    for (int i=0; i<Width(); i++) {
      for (int j=0; j<Height(); j++) {
	unsigned char sval = 0;
	vector<int> svec = SegmentVector(f, i, j);
	for (vector<int>::const_iterator si=svec.begin(); si!=svec.end(); si++)
	  if (*si==this_seg)
	    sval = 255;

	mask.set(i, j, sval);

	if (sval==0)
	  not_all_pixels = true;
      }
    }

    if (not_all_pixels) {
      tempmask[l] = tempname[l]+"_a.pbm";
      imagefile::write(mask, tempmask[l], "image/pbm");
    }

    // If we are calculating a segmented description,
    // use the mask image as input image for RegionShape and ContourShape,
    // which would normally look for shapes inside segments.
    const char *imagename = tempimage[l].c_str();
    bool use_mask_as_image = not_all_pixels && handler->IsShapeDescriptor();
    if (use_mask_as_image)
      imagename = tempmask[l].c_str();

    // put image file name in dummy file 
    FILE *tmpf = fopen(tempname[0].c_str(), "a");
    if (tmpf != NULL) {
      //fputs(tempimage[l].c_str(),tmpf);
      fputs(imagename, tmpf);
      if (f && !converted) {
	char frame[100];
	sprintf(frame, "[%d]", f);
	fputs(frame, tmpf);
      }
      fputs("\n",tmpf);
      fclose(tmpf);

    } else
      cerr << msg << "could not open temporary file "
	   << tempname[l] << endl;

    temppar[l] = tempname[l]+string(".par");

    FILE *tmpout = fopen(temppar[l].c_str(), "w");
  
    param_t::iterator it;
    param_t p = handler->GetParameters();

    for(it=p.begin(); it != p.end(); it++) {
      if (FrameVerbose())
	cout << msg << "setting parameter " << it->first 
	     << " to " << it->second << endl; 
      fputs((it->first+" "+it->second+"\n").c_str(),tmpout);       
    }

    fclose(tmpout);
    chmod(temppar[l].c_str(), mode);

    return handler->SegmentPreProcess(f, l);
  }

  //===========================================================================

  External::execchain_t MPEG7_XM::GetExecChain(int /*f*/, bool /*all*/, int l) {
    string msg = "MPEG7_XM::GetExecChain() : ";

    if (FrameVerbose())
      cout << msg << "starting" << endl;

    size_t n = size_t(l);
    if (l<0 || tempname.size()<=n || tempxml.size()<=n || templog.size()<=n )
      throw string("MPEG7_XM::GetExecChain() : ")+ToStr(l)+" out of bounds";

    tempxml[l] = tempname[l]+".xml";
    templog[l] = tempname[l]+".log";

    passwd *pw = getpwuid(getuid());
    string uhdir = pw&&pw->pw_dir ? pw->pw_dir : "";
    string xmdir = uhdir+"/picsom/bin";

    const string
      mats55_exe = "/share/imagedb/mats/XM-5.5/XMMain_linux_fast.exe",
      mats_exe = "/share/imagedb/mats/XM-6.0/bin/XMMain_linux6.0.exe",
      mats61_exe = "/share/imagedb/mats/XM-6.1/bin/XMMain_linux6.1.exe",
      //mats_exe = "/share/imagedb/mats/XM-6.0-new/bin/XMMain_linux_dbg6.0.exe",
      hossi_exe  = "/share/imagedb/hossi/XM/bin/XMMain_linux_dbg6.0.exe",
      own_exe    = xmdir+"/XMMain_linux6.1.exe";

    //bool hossi = false;

    string xm_version = handler->GetXMVersion(), cmd;
  
    if (FrameVerbose())
      cout << msg << "A" << endl;

    if (xm_version == "6.0 hossi") {
      cmd = hossi_exe;
      //handler->ForceXMVersion("6.0");
    }
    else if (xm_version == "6.1")
      cmd = mats61_exe;
    else if (xm_version == "6.0")
      //cmd = hossi ? hossi_exe : mats_exe;
      cmd = mats_exe;
    else 
      cmd = mats55_exe;

    bool triton = false;
    if (!FileExists(cmd)) {
      string c = cmd;
      size_t p = c.find("/bin/");
      if (p!=string::npos)
	c.erase(0, p);
      c = PicSOMroot()+"/"+SolveArchitecture()+c;
	
      if (FileExists(c)) {
	triton = true;
	cmd = c;
      }
    }

    if (FrameVerbose())
      cout << msg << "B" << endl;

    bool use_own = false;
    if (xm_version == "6.1" && FileExists(own_exe)) {
      cmd = own_exe;
      use_own = true;
    }

    execargv_t execargv;
    execargv.push_back(cmd);
    execargv.push_back("-p"+temppar[l]);
    execargv.push_back("-a"+handler->Name()+"Server");
    execargv.push_back("-l"+tempname[l]);
    execargv.push_back("-r"+templog[l]);
    execargv.push_back("-b"+tempxml[l]);

    execenv_t execenv;
    const char *pathp = getenv("PATH");
    string path = pathp ? pathp : "";
    string newpath = string("/share/imagedb/hossi/XM/bin")
      +(path==""?"":":")+path;
    execenv["PATH"] = newpath;
    if (use_own)
      execenv["PATH"] = xmdir+(path==""?"":":")+path;

    if (FrameVerbose())
      cout << msg << "C" << endl;

    string llp = "/share/imagedb/mats/XM/lib";
    if (xm_version == "5.5")
      llp = "/share/imagedb/markus/xerces-c-src1_6_0/lib";
    if (triton)
      llp = PicSOMroot()+"/"+SolveArchitecture()+"/lib32";
    if (use_own)
      llp = xmdir;

    const char *envllp = getenv("LD_LIBRARY32_PATH");
    if (envllp)
      llp += ":"+string(envllp);    

    execenv["LD_LIBRARY32_PATH"] = llp;

    if (FrameVerbose())
      cout << msg << "D" << endl;

    string wd = TempDir()+"/mpeg7_xm";
    if (!DirectoryExists(wd)) {
      int mode = 02775;
      int r = mkdir(wd.c_str(), mode);
      if (r) 
	throw "MPEG7_XM::GetExecChain() : mkdir("+wd+") failed";
    }

    execspec_t execspec(execargv, execenv, "", wd);

    if (FrameVerbose())
      cout << msg << "ending" << endl;

    return execchain_t(1, execspec);
  }

  //===========================================================================

  bool MPEG7_XM::SegmentPostProcess(int /*f*/, int l) {
    // convert xml -> dat
    MPEG7_XMData *data_dst = (MPEG7_XMData *)GetData(l);
    int maxc = DataElementCount();
    return SetFromXML(data_dst, tempxml[0],l,maxc);
  }

  //===========================================================================

  bool MPEG7_XM::SegmentRemoveTempFiles(int l) {
    // remove temporary files
    if (!KeepTmp()) {
      if (FrameVerbose())
	cout << "MPEG7_XM::SegmentRemoveTempFiles() Removing temporary files." 
	     << endl;
      Unlink(tempimage[l]);
      Unlink(tempmask[l]);
      Unlink(tempxml[l]);
      Unlink(temppar[l]);
      Unlink(templog[l]);
      Unlink(tempname[l]);
      if (tempimage[l].find('/')==0)
	RmDirRecursive(tempimage[l]+".dir");
    }
    return true;
  }

  //===========================================================================

  bool MPEG7_XM::ExecPostProcess(int /*f*/, bool /*all*/, int /*l*/) {
    //  return SegmentPostProcess(l);
    return true;
  }

  //===========================================================================

  bool MPEG7_XM::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "MPEG7_XM::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="forcexmversion") {
	if (value=="5.5" || value=="6.0" || value=="6.1")
	  ForceXMVersion(value);
	else
	  throw msg + " : bad version number, only 5.5, 6.0 and 6.1"
	    "are supported.";
      
	i = l.erase(i);
	continue;
      }

      if (key=="normalize") {
	string tmp = "", opts = "";
	if (value.find("(") != string::npos) {
	  SplitParentheses(value, tmp, opts);
	  SetNormalizationOpts(opts);
	  value = tmp;
	}

	if (value == "face4")
	  SetNormalization(n_face4);
	else if (value == "face" || value == "face1")
	  SetNormalization(n_face1);
	else
	  throw msg + " : unknown normalization method [" + value +
	    "], only face1 and face4 are currently supported.";
      
	i = l.erase(i);
	continue;
      }

      i++;
    }

    return External::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  void MPEG7_XM::SetNormalizationOpts(string &o) {
    string msg = "MPEG7_XM::SetNormalizationOpts() : ";
    cout << msg << "starting : " << o << endl;
  
    vector<string> ov = picsom::SplitInCommasObeyParentheses(o);
  
    for (size_t i=0; i<ov.size(); i++) {
      if (ov[i] == "savegamma")
	nopts.savegamma = true;
      else if (ov[i] == "ellipse")
	nopts.ellipse = true;
      else
	cout << msg << "unrecognized option: " << ov[i] << endl;
    }
  }

  //===========================================================================

  xmlNodePtr MPEG7_XM::SearchNode(xmlNodePtr node, 
				  const xmlChar *name) {
    xmlNodePtr cur = node->xmlChildrenNode;
    while (cur != NULL) {
      if (!xmlStrcmp(cur->name,name)) return cur;
      else {
	xmlNodePtr temp = SearchNode(cur, name);
	if (temp != NULL) return temp;
      }
      cur = cur->next;
    }
    return NULL;
  }

  //===========================================================================

  xmlNodePtr MPEG7_XM::SearchNextNode(xmlNodePtr cur,
				      const xmlChar *name)
  {
    while (cur != NULL) {
      if (!xmlStrcmp(cur->name,name)) return cur;
      cur = cur->next;
    }
    return NULL;
  }

  //===========================================================================

  bool MPEG7_XM::SetFromXML(MPEG7_XMData *d, string xmlfile, int l, int maxc) {
    struct stat st;
    if (stat(xmlfile.c_str(), &st) || !st.st_size) {
      if (FileVerbose())
	cout << "MPEG7_XMData::SetFromXML() nonexistent or empty <"
	     << xmlfile << ">" << endl;
      return false;
    }

    bool ok = false;
    xmlDocPtr doc = xmlParseFile(xmlfile.c_str());
    xmlNodePtr root = doc ? xmlDocGetRootElement(doc) : NULL;

    /*
      Add functionality here to find l:th Descriptor-block from XML.
      Current code find only the first one... 
    */
    xmlNodePtr node = NULL, find;

    int c = 0;
    if (root != NULL && !xmlStrcmp(root->name, (const xmlChar *) "Mpeg7")) {
      node = root->children;

      find = SearchNode(node,(const xmlChar *) "Descriptor");
      if (find == NULL) 
	find = SearchNode(node,(const xmlChar *) "VisualDescriptor");

      if (find != NULL) {
	if (l > 0) {
	  c = 1;
	  while (!ok && c<maxc) {
	    find = find->next;
	    find = SearchNextNode(find,(const xmlChar *) "Descriptor");
	    if (find == NULL) 
	      find = SearchNextNode(find,(const xmlChar *) "VisualDescriptor");
	  
	    if (find != NULL) {
	      if (c==l) {
		node = find;
		ok = true;
	      } else {
		c++;
	      }
	    } else {
	      c = maxc;
	    }
	  }
	} else { // if l=0
	  node = find;
	  ok = true;
	}
      }
    }
    if (!ok || !node) {
      cerr << "MPEG7_XM::MPEG7_XMData::SetFromXML: Error: bad XML in <"
	   << xmlfile << ">" << endl;
      xmlFreeDoc(doc);
      return false;
    }

    // get type
    xmlChar *type = xmlGetProp(node,(const xmlChar *)"type");
    if (type != NULL) {
      // call correct XMLDescriptorHandler class accoring to the value of type
      XMLDescriptorHandler *handler;
      string ttype = PREFIX+string((char *)type);

      // check if the type ends with the word "Type", and remove it if present
      if (ttype.substr(ttype.length()-4) != "Type") {
	if (FileVerbose())
	  cerr << "MPEG7_XM::MPEG7_XMData::SetFromXML: Warning: type field '" 
	       << ttype << "' in XML doesn't end with "  << "'Type' in <"
	       << xmlfile << ">" << endl;
      } else
	ttype = ttype.substr(0,ttype.length()-4);

      handler = XMLDescriptorHandler::FindMethod(ttype,this);
      if (handler != NULL) {
	handler->HandleXML(doc, node);
	d->SetVector(handler->GetData());
      } else 
	cerr << "MPEG7_XM::MPEG7_XMData::SetFromXML: Unknown descriptor: '" 
	     << type << "' in <" << xmlfile << ">" << endl;
      xmlFree(type);
    }

    // Strangely the xmlFree-functions cause lock-ups... 
    //
    //   if (doc != NULL) {
    //     xmlFreeDoc(doc); 
    //     if (root != NULL) {
    //       xmlFree(root);
    //       if (node != NULL) xmlFree(node); 
    //     }
    //   }

    return true;
  }

  //===========================================================================

  string MPEG7_XM::XMLDescriptorHandler::GetXMVersion() {
    if (parent != NULL) {
      string forceversion = parent->ForceXMVersion();
      if (forceversion != "")
	return forceversion;
    } else
      cerr << "MPEG7_XM::XMLDescriptorHandler::GetXMVersion: "
	   << "Warning: parent NULL" << endl;
    
    return GetPreferredXMVersion();
  }

  //===========================================================================

  bool MPEG7_XM::XMLDescriptorHandler::HandleXML(xmlDocPtr doc,
						 xmlNodePtr node) {
    xmlNodePtr cur = node->xmlChildrenNode;
    while (cur != NULL) {
      HandleNode(doc, cur);
      cur = cur->next;
    }
    return true;
  }

  //===========================================================================

  bool MPEG7_XM::XMLDescriptorHandler::HandleNode(xmlDocPtr doc,
						  xmlNodePtr cur) {
    xmlChar *cdata;
  
    cdata = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
    if (cdata != NULL)
      HandleContent(cur->name,cdata);
    if (cur->xmlChildrenNode != NULL) 
      HandleXML(doc, cur);  
    xmlFree(cdata);
    return true;
  }

  //===========================================================================

  bool MPEG7_XM::XMLDescriptorHandler::InDatanames(const xmlChar *name) {
    bool found = false;
    vector<string>::iterator it;
    string sname = string((char *)name);
  
    if (!datanames.empty()) {
      for (it = datanames.begin(); it != datanames.end(); it++)
	if (*it == sname)	found=true;
    }
    return found;
  }

  //===========================================================================

  bool MPEG7_XM::XMLDescriptorHandler::AddContentToData(xmlChar *content) {

    try {
      AddVectorToData(toVectorType(content));
      return true;
    } 
    catch (const string& err) {
      cerr << "MPEG7_XM::XMLDescriptorHandler::AddContentToData: Error: " << 
	err << endl;
      return false;
    }
  }

  //===========================================================================

  bool MPEG7_XM::XMLDescriptorHandler::HandleContent(const xmlChar *name, 
						     xmlChar *content) {
    if (FileVerbose())
      cout << "MPEG7_XM::XMLDescriptorHandler::HandleContent: " << name << ", "
	   << content << endl;
  
    if (/*datanames.empty() || */InDatanames(name)) {
      AddContentToData(content);
    }
    return true;
  }

  //===========================================================================

  External::vectortype MPEG7_XM::XMLDescriptorHandler::toVectorType(xmlChar
								    *content) {
    vectortype temp;
    string str = string((char *)content);

    RegExp re("^[ \t]*([-]?[0-9]+)");
    vector<RegExpMatch> v = re.match(str,1);

    while (re.ok() && !v.empty()) {
      temp.push_back(atof(v[0].c_str()));
      const size_t next=v[0].end()+1;
      if (next >= str.size()) break;
      str = str.substr(next);
      v = re.match(str,1);
    }

    return temp;
  }

  //===========================================================================

  multimap<int,External::vectortype>::iterator 
  MPEG7_XM::XMLDominantColorHandler::GetLargestInHash() {

    multimap<int,vectortype>::iterator it, p;
    it = hash.begin();
    p = it; 
    it++;
    for (; it != hash.end(); it++) {
      if (it->first >= p->first) 
	p = it;
    }
    return p;
  }

  //===========================================================================

  bool MPEG7_XM::XMLDominantColorHandler::HandleXML(xmlDocPtr doc, 
						    xmlNodePtr node) 
  {
    bool result = XMLDescriptorHandler::HandleXML(doc,node);
    multimap<int,vectortype>::iterator p;

    if (hash.size() > 0) {
      p = GetLargestInHash();
      AddVectorToData(p->second);

      if (hash.size() > 1) {
	hash.erase(p);
	p = GetLargestInHash();
      }
      AddVectorToData(p->second);
    }

    return result;
  }

  //===========================================================================

  bool MPEG7_XM::XMLDominantColorHandler::HandleNode(xmlDocPtr doc, 
						     xmlNodePtr cur) 
  {
    xmlChar *cdata;
    xmlNodePtr cnode;
    int per = -1;
    vectortype cvi;
    string value_str, percentage_str, index_str;
  
    if (GetXMVersion() == "5.5") {
      value_str = "Values";
      percentage_str = "Percentage";
      index_str = "ColorValueIndex";
    } else {
      value_str = "Value";
      percentage_str = "Percentage";
      index_str = "Index";
    }

    if (!xmlStrcmp(cur->name,(const xmlChar *)value_str.c_str())) {
      cnode = cur->xmlChildrenNode;
      while (cnode != NULL) {
	cdata = xmlNodeListGetString(doc, cnode->xmlChildrenNode, 1);
	if (cdata != NULL) {
	  if (!xmlStrcmp(cnode->name,(const xmlChar *)percentage_str.c_str())) {
	    per = atoi((char *)cdata);
	  }
	  if (!xmlStrcmp(cnode->name,(const xmlChar *)index_str.c_str())) {
	    cvi = toVectorType(cdata);
	  }
	}
	cnode = cnode->next;
	xmlFree(cdata);
      }
      hash.insert(pair<int,vectortype>(per,cvi));
    }
    return true;
  }

  //===========================================================================

  bool MPEG7_XM::XMLContourShapeHandler::HandleXML(xmlDocPtr doc, 
						   xmlNodePtr node) {
    bool result = XMLDescriptorHandler::HandleXML(doc,node);

    if (GetXMVersion() == "6.0") {
      AddVectorToData(peakX);
      AddVectorToData(peakY);
    }

    return result;
  }

  //===========================================================================

  bool MPEG7_XM::XMLContourShapeHandler::HandleNode(xmlDocPtr doc, 
						    xmlNodePtr cur) {
    if ((GetXMVersion() == "6.0") && 
	!xmlStrcmp(cur->name,(const xmlChar *)"Peak")) 
      {
	xmlChar *xprop, *yprop;
	xprop = xmlGetProp(cur,(const xmlChar *)"peakX");
	yprop = xmlGetProp(cur,(const xmlChar *)"peakY");
	try {
	  vectortype x = toVectorType(xprop);      
	  vectortype y = toVectorType(yprop);
      
	  AddVectorToX(x);
	  AddVectorToY(y);
	  return true;
	} 
	catch (const string& err) {
	  cerr << "MPEG7_XM::XMLContourShapeHandler::HandleNode: Error: " << 
	    err << endl;
	  return false;
	}
        
      } else 
      return MPEG7_XM::XMLDescriptorHandler::HandleNode(doc,cur);
  }

  //===========================================================================

  bool MPEG7_XM::XMLMotionActivityHandler::HandleContent(const xmlChar *name, 
							 xmlChar *content) {
    string sname = string((char *)name);
    if (sname != "DominantDirection")
      return XMLDescriptorHandler::HandleContent(name, content);

    if (FileVerbose())
      cout << "MPEG7_XM::XMLMotionActivityHandler::HandleContent: "
	   << name << ", " << content << endl;

    // Calculate value
    int val = atoi((char *)content);
    
    // There are 8 sectors, from 0...7, change this to a vector
    // to get better euclidean distances between sectors.
    // I'm using the smallest possible integers I found that
    // satisfied this criteria. The range of numbers is 0...6 
    // where (3,3) is origo (no direction)
    const char* mat_x[] = {"6","5","3","1","0","1","3","5"};
    const char* mat_y[] = {"3","1","0","1","3","5","6","5"};

    // AddContentToData does not accept const char*
    // so lets do a cast and hope for the best
    if(val >= 0 && val < 8) { 
      AddContentToData((xmlChar*)mat_x[val]);
      AddContentToData((xmlChar*)mat_y[val]);
    }else{
      AddContentToData((xmlChar*)mat_y[0]); // Origo is (3,3)
      AddContentToData((xmlChar*)mat_y[0]);
    }

    return true;
  }

  //===========================================================================

  static string comma_to_space(const string& s) {
    string r(s);
    for (size_t p = r.find(','); p!=string::npos; p = r.find(','))
      r.replace(p, 1, " ");
    return r;
  }

  //===========================================================================

  static pair<int,int> solve_point(segmentfile *s,
				   const string& n, const string& m) {
    int x, y, w, h;
    float a, x0, y0;

    SegmentationResultList *srl;
    srl = s->readFrameResultsFromXML(n, "point");
    if (srl->begin()==srl->end()) {
      delete srl;
      srl = s->readFileResultsFromXML(n, "point");
    }
    if (srl->begin()!=srl->end()) {
      string tmp = comma_to_space(srl->begin()->value);
      delete srl;
      istringstream osr(tmp);
      osr >> x >> y;
      return pair<int,int>(x, y);
    }
    delete srl;

    srl = s->readFrameResultsFromXML(n, "rotated-box");
    if (srl->begin()==srl->end()) {
      delete srl;
      srl = s->readFileResultsFromXML(n, "rotated-box");
    }
    if (srl->begin()!=srl->end()) {
      string tmp = comma_to_space(srl->begin()->value);
      delete srl;
      istringstream osr(tmp);
      osr >> x >> y >> w >> h >> a >> x0 >> y0;

      scalinginfo si(a, x0, y0);
      float x1, y1, x2, y2;
      si.rotate_dst_xy_c(x, y, x1, y1);
      si.rotate_src_xy_c(x2, y2, x1+w/2, y1+h/2);

      return pair<int,int>(int(floor(x2+0.5)), int(floor(y2+0.5)));
    }
    delete srl;

    throw m+"no "+n+" point/rotated-box";
  }

  //===========================================================================

  bool MPEG7_XM::XMLFaceRecognitionHandler::SegmentPreProcess(int f, int l) {
    stringstream hss;
    hss << f << "," << l;
    string h = "MPEG7_XM::XMLFaceRecognitionHandler::SegmentPreProcess("
      + hss.str() + ")";

    if (FrameVerbose())
      cout << h << endl;

    h += " : ";

    if (parent->DataLabels().size())
      throw h+"data_labels not empty";

    // if (f || l)
    //   throw h+"f!=0 || l!=0";

    if (l)
      throw h+"l!=0";

    segmentfile *s = parent->SegmentData();
    if (!s)
      throw h+"no segmentation data";

    s->setCurrentFrame(f);

    stringstream facestxt;
    string err;

    size_t nface = 0;
    try {
      pair<int,int> re = solve_point(s, "r-eye", h);
      pair<int,int> le = solve_point(s, "l-eye", h);

      if (FrameVerbose())
	cout << h << "from r-eye&l-eye: "  << re.first << "," << re.second
	     << " " << le.first << "," << le.second << endl;

      facestxt << 1 << endl
	       << re.first << " " << re.second << " "
	       << le.first << " " << le.second << endl;

      // There's only one face if eye locations are specified so no need 
      // for "_face0" suffix in label? -Markus
      parent->AddOneDataLabel("");
      nface++;
      //parent->AddOneDataLabel("face"+ToStr(nface++));

    } catch (const string& e) {
      err += e;
    }

    if (!nface) {
      SegmentationResultList *srl;
      srl = s->readFrameResultsFromXML("face", "box", true);
      if (srl->begin()==srl->end()) {
	delete srl;
	srl = s->readFileResultsFromXML("face", "box", true);
      }
      nface = srl->size();
      for (SegmentationResultList::const_iterator i=srl->begin();
	   i!=srl->end(); i++) {
	string tmp = comma_to_space(i->value);
	if (FrameVerbose())
	  cout << i->name << " : " << tmp << endl;

	istringstream osr(tmp);
	int tlx, tly, brx, bry;
	osr >> tlx >> tly >> brx >> bry;
	int ey  = tly+int(0.39*(bry-tly));
	int ex1 = tlx+int(0.31*(brx-tlx));
	int ex2 = tlx+int(0.69*(brx-tlx));

	if (i==srl->begin())
	  facestxt << nface << endl;

	facestxt << ex1 << " " << ey << " " << ex2 << " " << ey << endl;
	parent->AddOneDataLabel("face"+ToStr(nface++));
      }
      delete srl;
    }

    string dir = parent->GetTempImage(l)+".dir";
    int dmode = 0777, fmode = 0666;
    mkdir(dir.c_str(), dmode);
    chmod(dir.c_str(), dmode);

    if (FrameVerbose())
      cout << h << "created tempdir  " << dir << endl;

    string txt = dir+"/faces.txt";
    ofstream ftxt(txt.c_str());
    ftxt << facestxt.str() << flush;

    chmod(txt.c_str(), fmode);

    if (FrameVerbose()) 
      cout << h << "created tempfile " << txt << ":" << endl
	   << facestxt.str() << flush;

    return true;
  }

  //===========================================================================

  bool MPEG7_XM::XMLFaceRecognitionHandler::SegmentPrePreProcess() {
    if (!parent->UseNormalization())
      return true;

    string msg =
      "MPEG7_XM::XMLFaceRecognitionHandler::SegmentPrePreProcess() : ";
  
#ifdef HAVE_OPENCV2_CORE_CORE_HPP
  
    if (FileVerbose()) {      
      cout << msg << "Normalization turned on." << endl; 
      cout << msg << "Input: " << parent->ObjectFullDataFileName() << endl;
    }
  
    IplImage* image  = cvLoadImage(parent->ObjectFullDataFileName().c_str(), 
				   CV_LOAD_IMAGE_GRAYSCALE);
    if (!image) {
      cerr << msg << "ERROR: failed to load input image, normalization disabled." 
	   << endl; 
      return false;
    }

    IplImage* meanimage = cvLoadImage("/share/imagedb/picsom/share/MPEG7_XM/meanface.bmp", 
				      CV_LOAD_IMAGE_GRAYSCALE);
    if (!meanimage) {
      cerr << msg << "ERROR: failed to load mean image, normalization disabled." 
	   << endl; 
      return false;
    }

    if (meanimage->width != image->width || meanimage->height != image->height) {
      cerr << msg << "ERROR: mean and input image sizes do not match, you "
	   << "probably need to create a new mean image. Current sizes: input="
	   << image->width << "x" << image->height << ", mean=" << meanimage->width
	   << "x" << meanimage->height
	   << endl; 
      return false;
    }

    IplImage* gammaimage = cvCreateImage(cvGetSize(meanimage), IPL_DEPTH_8U, 1);
  
    if (parent->GetNormalization() == n_face4) {

      NormalizeRegion(image, gammaimage, meanimage, img_tl);
      NormalizeRegion(image, gammaimage, meanimage, img_tr);
      NormalizeRegion(image, gammaimage, meanimage, img_bl);
      NormalizeRegion(image, gammaimage, meanimage, img_br);

    } else if (parent->GetNormalization() == n_face1) {

      NormalizeRegion(image, gammaimage, meanimage, img_all);

    } else {
      cerr << msg << "ERROR: Unknown normalization method!" 
	   << endl;
      return false;
    }
    /*
      cvNamedWindow( "image", 1 );
      cvShowImage( "image", image);
      cvNamedWindow( "meanimage", 1 );
      cvShowImage( "meanimage", meanimage);
      cvNamedWindow( "gammaimage", 1 );
      cvShowImage( "gammaimage", gammaimage);
      cvWaitKey(0);
    */
  
    //MPEG7_XM* unconst_parent = (MPEG7_XM*) parent;
  
    pair<bool,string> tmpfn = parent->TempFile();
    string tmpbase = tmpfn.second;
  
    if (!tmpfn.first) {
      cerr << msg << "ERROR: Failed to create temporary file <"+tmpbase+">" 
	   << endl;
      return false;
    }
  
    string imagepath = tmpbase+".bmp";
    cvSaveImage(imagepath.c_str(), gammaimage);
    parent->SetIntermediateFilename(imagepath);

    if (FileVerbose()) {      
      cout << msg << "Output: " << parent->CurrentFullDataFileName() 
	   << endl;
    }

    if (parent->nopts.savegamma) {
      string gammafn = parent->ObjectDataFileName();
      if (gammafn.length()>11)
	gammafn.erase(8,4);
      if (parent->GetNormalization() == n_face4)
	gammafn += "-face4";
      else if (parent->GetNormalization() == n_face1)
	gammafn += "-face1";    
      gammafn += "-gamma.bmp";
      cvSaveImage(gammafn.c_str(), gammaimage);
      if (FileVerbose()) {      
	cout << msg << "Wrote gamma image : " << gammafn
	     << endl;
      }
    }

    cvReleaseImage(&image);
    cvReleaseImage(&meanimage);

#else
    cerr << endl << msg << "ERROR: OpenCV not found. Normalization disabled!" 
	 << endl << endl; 
#endif // HAVE_OPENCV2_CORE_CORE_HPP
  
    return true;
  }

  //===========================================================================

#ifdef HAVE_OPENCV2_CORE_CORE_HPP
    
  void MPEG7_XM::XMLFaceRecognitionHandler::ApplyGamma(IplImage* src,
						       IplImage* dst, 
						       double g) {  
    IplImage* temp = cvCreateImage(cvGetSize(src), 
				   IPL_DEPTH_32F, src->nChannels);  
    cvConvertScale(src, temp, 1.0/255);
    for (int y=0; y<temp->height; y++) {
      uchar* ptr = (uchar*) (temp->imageData + y*temp->widthStep);
      for (int x=0; x<temp->width; x++) {
	float* fp = (float*) &ptr[4*x];	  
	*fp = pow(double(*fp), 1./g);
      }
    }
    cvConvertScale(temp, dst, 255.0);
    cvReleaseImage(&temp);
  }

  // ---------------------------------------------------------------------

  void MPEG7_XM::XMLFaceRecognitionHandler::NormalizeRegion(IplImage* src, 
							    IplImage* dst, 
							    IplImage* mean_img, 
							    img_region reg) {
    string msg = "MPEG7_XM::XMLFaceRecognitionHandler::NormalizeRegion() : ";

    int width = src->width;
    int height = src->width; // !!! note this !!!

    int whalf = width/2;
    int hhalf = height/2;

    CvRect r;
    switch (reg) {
    case img_all:
      r = cvRect(1, 1, width, height);
      break;
    case img_tl:
      r = cvRect(1, 1, whalf, hhalf);
      break;
    case img_tr:
      r = cvRect(1+whalf, 1, whalf, hhalf);
      break;
    case img_bl:
      r = cvRect(1, 1+hhalf, whalf, hhalf);
      break;
    case img_br: 
      r = cvRect(1+whalf, 1+hhalf, whalf, hhalf);
      break;
    default:
      cerr << msg << "Unknown image region!" << endl;
    }

    IplImage *src_part   = cvCreateImage(cvSize(r.width,r.height),
					 IPL_DEPTH_8U, 1);
    IplImage *his_part   = cvCreateImage(cvSize(r.width,r.height),
					 IPL_DEPTH_8U, 1);
    IplImage *dst_part   = cvCreateImage(cvSize(r.width,r.height),
					 IPL_DEPTH_8U, 1);
    IplImage *mean_part  = cvCreateImage(cvSize(r.width,r.height),
					 IPL_DEPTH_8U, 1);
    IplImage* diffim     = cvCreateImage(cvSize(r.width,r.height),
					 IPL_DEPTH_8U, 1);
  
    cvSetImageROI(src, r);
    cvResize(src, src_part);
    cvResetImageROI(src);

    cvEqualizeHist(src_part, his_part);

    cvSetImageROI(mean_img, r);
    cvResize(mean_img, mean_part);
    cvResetImageROI(mean_img);

    IplImage* ellim      = cvCreateImage(cvSize(width, height),
					 IPL_DEPTH_8U, 1);
    IplImage* ell_part     = cvCreateImage(cvSize(r.width,r.height),
					   IPL_DEPTH_8U, 1);
    CvPoint ellcenter = cvPoint(whalf, hhalf); 
    CvSize ellaxes = cvSize((int)whalf*0.6, (int)hhalf*0.8); 
    cvZero(ellim);
    cvEllipse(ellim, ellcenter, ellaxes, 0, 0, 360, cvScalar(255), CV_FILLED);
    //string ellfn = "ellipse.bmp";
    //cvSaveImage(ellfn.c_str(), ellim);

    cvSetImageROI(ellim, r);
    cvResize(ellim, ell_part);
    cvResetImageROI(ellim);
  
    double bestgamma = 0.0;
    int mindiff = 9999999;
    for (double gamma = 0.01; gamma<0.99; gamma+=0.01) {
    
      ApplyGamma(his_part, dst_part, gamma);

      cvAbsDiff(dst_part, mean_part, diffim);

      CvScalar d;

      if (parent->nopts.ellipse) {
	//string ellpfn = "ellipse-part.bmp";
	//cvSaveImage(ellpfn.c_str(), ell_part);
	d = cvAvg(diffim, ell_part);
      } else
	d = cvAvg(diffim);
    
      if (d.val[0] < mindiff) {
	mindiff   = int(d.val[0]);
	bestgamma = gamma;
      }
    }

    if (FileVerbose())
      cout << msg << "Smallest distance: " << mindiff << 
	" with gamma: " << bestgamma << endl;

    ApplyGamma(his_part, dst_part, bestgamma);
  
    cvSetImageROI(dst, r);
    cvResize(dst_part, dst);
    cvResetImageROI(dst);

    cvReleaseImage(&src_part); 
    cvReleaseImage(&his_part); 
    cvReleaseImage(&dst_part); 
    cvReleaseImage(&mean_part);
    cvReleaseImage(&diffim);
    cvReleaseImage(&ellim);
    cvReleaseImage(&ell_part);
  }

#endif

} // namespace picsom

//=============================================================================

// Local Variables:
// mode: font-lock
// End:
