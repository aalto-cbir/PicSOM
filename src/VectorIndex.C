// -*- C++ -*-  $Id: VectorIndex.C,v 2.111 2020/10/21 08:22:08 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <VectorIndex.h>
#include <DataBase.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string VectorIndex_C_vcid =
    "@(#)$Id: VectorIndex.C,v 2.111 2020/10/21 08:22:08 jormal Exp $";

  bool VectorIndex::bin_data_full_test = false;
  bool VectorIndex::fast_bin_check     = false;
  bool VectorIndex::nan_inf_check      = false;

  /////////////////////////////////////////////////////////////////////////////

  VectorIndex::VectorIndex(DataBase *d, const string& n, const string& f, 
			   const string& p, const string& ps,
			   const Index* src) : 
    Index(d, n, f, p, ps, src) {

    SetFeatureNames();

    has_solved_datafile_names = data_numbers_set = false;

    string fname = FindFile(FeatureFileName(), ".bin", false);
    is_selectable = fname!="" && FileSize(fname)>0;
    // if (is_selectable)
    //   cout << FeatureFileName() << " is SELECTABLE" << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  VectorIndex::~VectorIndex() {
    BinDataUnmap();
    LmdbDataClose();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::AddToXMLfeatureinfo_static(XmlDom& tree) const {
    tree.Element("size", VectorLength());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<string> VectorIndex::SolveFeatureAugmentation(const string& fea,
						       const string& aug) {
    string msg = "VectorIndex::SolveFeatureAugmentation("+fea+","+aug+") : ";
    vector<string> ret;
    DataBase *db = CheckDB();
    list<string> ll = db->ExpandFeatureAlias(aug);
    vector<string> l(ll.begin(), ll.end());
    if (l.size()==0)
      l = SplitInCommas(aug);
    for (auto i=l.begin(); i!=l.end(); i++)
      ret.push_back(fea+*i);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  target_type VectorIndex::SolveDataTarget(const FloatVectorSet& dset) {
    xmlDocPtr doc = dset.XMLDescription();
    if (doc) {
      xmlNodePtr r = xmlDocGetRootElement(doc);
      if (r)
        r = FindXMLchildElem(r, "target");
      if (r)
        return TargetType(GetXMLchildContent(r));
    }
    return target_no_target;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t VectorIndex::GuessVectorLength() {
    if (VectorLength())
      return VectorLength();

    // this will now be false if dat files have not yet been read
    if (DataFileCount(false)) 
      return simple::FloatVectorSource::ReadVectorLength(DataFileName(0).
							 c_str());

    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t VectorIndex::DataFileCount(bool heavy) /*const*/ {
    if (db->SqlFeatures() || db->UseBinFeaturesRead())
      return 0;
    
    if (datfilelist.empty() && heavy)
      SolveDataFileNames();
    
    return datfilelist.size();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::ReadDataFile(bool force, bool nodata, bool wrlocked) {
    //    if (metfile_str!="")
    //      return ReadMetaDataFile(force, nodata, wrlocked);

    if (!wrlocked)                   // obs! this ckecking _might_ be
      RwLockRead("ReadDataFile()");  // unnecessary now...
    bool really = ReadDataFileReally(force, nodata);
    if (!wrlocked)
      RwUnlockRead("ReadDataFile()");

    if (!really)
      return true;

    bool ok = true;

    if (!wrlocked)
      RwLockWrite("ReadDataFile()");

    if (ReadDataFileReally(force, nodata)) {
      if (db->UseBinFeaturesRead())
	ok = ReadDataFileBin(nodata);
      else if (db->SqlFeatures())
	ok = ReadDataFileSql(nodata);
      else
	ok = ReadDataFileClassical(nodata);

      // Picsom()->PossiblyShowDebugInformation("ReadDataFile() ending");
    }

    if (!wrlocked)
      RwUnlockWrite("ReadDataFile()");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::ReadDataFileClassical(bool nodata) {
    bool ok = true;

    Tic("ReadDataFileClassical");
    // WriteLog("VectorIndex::ReadDataFile() ", nodata?"nodata ":"", datfile);
    bool nodatawas = data.NoDataRead(nodata);

    for (size_t i=0; i<DataFileCount() && ok; i++) {
      FloatVectorSet datatmp;
      FloatVectorSet& dataref = i ? datatmp : data;

      const string& datfile = DataFileName(i);
      dataref.FileName(datfile);
      dataref.NoDataRead(nodata);

      if (!dataref.Read(datfile)) {
	if (db->RemoveBrokenDataFiles()) {
	  WriteLog("VectorIndex::ReadDataFile() reading <"+datfile+
		   "> failed");
	  RenameToExt(datfile,".broken");
	  db->AddCreatedFile(datfile+".broken");
	} else {
	  ShowError("VectorIndex::ReadDataFile() reading <"+datfile+
		    "> failed");
	  ok = false;
	  // Dump();
	}
      }

      if (FeatureTarget()==target_no_target) {
	target_type tt = SolveDataTarget(dataref);
	if (tt!=target_no_target)
	  FeatureTarget(tt);
      }

      stringstream tmptxt;
      tmptxt << " (dim=" << dataref.VectorLength() << ", "
	     << dataref.Nitems() << " vectors "
	     << data.Nitems() << ".."
	     << data.Nitems()+dataref.Nitems()-1 << ")";
      WriteLog("Read data file ", nodata?"(labels only) ":"",
	       "<"+ShortFileName(datfile)+">", tmptxt.str());

      if (data.VectorLength()!=dataref.VectorLength())
	//        ok = ShowError("vector lengths differ!");
	WriteLog("vector lengths differ!");

      if (i && ok)
	data.AppendCopy(datatmp);
    }

    if (DataFileCount()>1) {
      data.FileName("");
      ostringstream tmptxt;
      tmptxt << "Read a total of " << DataFileCount() << " data files, "
	     << data.Nitems() << " vectors";
      WriteLog(tmptxt);
    }

    if (data.Nitems()==0 && SqlNdataVectors()>0) {
      // data = SqlDataVectors("");
      data = db->SqlFeatureDataSet(FeatureFileName(), "", true);
      ostringstream tmptxt;
      tmptxt << "Read " << data.Nitems() << " vectors from SQL";
      WriteLog(tmptxt);
    }

    data.NoDataRead(nodatawas);
    Tac("ReadDataFileClassical");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::ReadDataFileSql(bool /*nodata*/) {
    bool angry = false /*true*/;
    data.Delete();
    data = db->SqlFeatureDataSet(FeatureFileName(), "", angry);
    // data = SqlDataVectors("");
    return data.Nitems();
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVector *VectorIndex::BinDataFloatVector(size_t i) {
    string msg = "VectorIndex::BinDataFloatVector() : ";

    // if (!BinDataExists(i))
    //   return NULL;

    if (!BinInfoVectorLength()) {
      // ShowError(msg+"vdim=0");
      return NULL;
    }

    if (( bin_data_full_test && !BinDataFullTest(i)) ||
	(!bin_data_full_test && !BinDataExists(i)))
      return NULL;

    // FloatVector v(BinInfoVectorLength(), (float*)BinDataAddress(i));

    vector<float> vv = _bin_data_m.begin()->second.get_float(i);
    if (!vv.size())
      ShowError(msg+"empty vector returned");

    FloatVector v(BinInfoVectorLength(), &vv[0]);
    v.Number(i);
    v.Label(db->Label(i));
    
    return new FloatVector(v);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::ReadDataFileBin(bool /*nodata*/) {
    string msg = "VectorIndex::ReadDataFileBin() : ";

    bool rw = false; // db->OpenReadWriteFea()
    const string augm;
    if (!BinDataOpen(rw, db->Size(), false, 0.0, augm))
      return ShowError(msg+"BinDataOpen() failed");

    data.FileName(_bin_data_m.begin()->second.filename());

    for (size_t i=0; i<db->Size(); i++) {
      FloatVector *v = BinDataFloatVector(i);
      if (v)
	data.Append(v);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  // bool VectorIndex::ReadDataFileBinOld(bool nodata) {
  //   string msg = "VectorIndex::ReadDataFileBinOld() : ";

  //   bool ok = true;

  //   Tic("ReadDataFileBin");
  //   data.Delete();
  //   bool nodatawas = data.NoDataRead(nodata);

  //   ifstream in;
  //   string ns = FeatureFileName(), datfile;
  //   size_t vl = OpenBinDataOld(ns, datfile, in), nzero = 0, nnan = 0;

  //   if (!vl)
  //     ok = ShowError(msg+"OpenBinDataOld() failed");
  //   else
  //     WriteLog("Starting to read binary feature data <"+
  // 	       ShortFileName(datfile)+">");

  //   data.FileName(datfile);
  //   data.NoDataRead(nodata);

  //   for (size_t i=0; ok && i<db->Size(); i++) {
  //     FloatVector v(vl);
  //     in.read((char*)(float*)v, vl*sizeof(float));

  //     if (!db->ObjectsTargetTypeContains(i, FeatureTarget()))
  // 	continue;

  //     if (v.IsZero()) {
  // 	nzero++;
  // 	continue;
  //     }

  //     // was needed in image-net, temporal hack
  //     float nanvalue = numeric_limits<float>::quiet_NaN();
  //     if (!memcmp(&v[0], &nanvalue, sizeof(float))) {
  // 	nnan++;
  // 	continue;
  //     }

  //     if (in) {
  // 	v.Number(i);
  // 	v.Label(db->Label(i));
  // 	data.AppendCopy(v);

  //     } else
  // 	ok = ShowError(msg+"read() with <"+datfile+"> failed");
  //   }

  //   if (FeatureTarget()==target_no_target) {
  //     target_type tt = SolveDataTarget(data);
  //     if (tt!=target_no_target)
  // 	FeatureTarget(tt);
  //   }

  //   stringstream tmptxt;
  //   tmptxt << " (dim=" << data.VectorLength() << ", "
  // 	   << data.Nitems() << " vectors 0.."
  // 	   << data.Nitems()-1 << ") " << nzero
  // 	   << " zero and " << nnan << " nan vectors were skipped";
  //   WriteLog("Read data file ", nodata?"(labels only) ":"",
  // 	     "<"+ShortFileName(datfile)+">", tmptxt.str());

  //   data.NoDataRead(nodatawas);
  //   Tac("ReadDataFileClassical");

  //   return ok;
  // }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataOpen(bool rw, size_t n, bool create, float v,
				const string& augm) {
    string msg = "VectorIndex::BinDataOpen("+ToStr(rw)+","+ToStr(n)+
      ","+ToStr(create)+","+ToStr(v)+","+augm+") <"+BinInfoFileName()+"> : ";

    bool tolerate_large = true;

    // if (rw && VectorLength()==0)
    //   return ShowError(msg+"VectorLength()==0");

    if (BinInfoRawDataLength(augm)==0)
      BinDataOpenFile(rw, create, v, augm);

    if (VectorLength() && BinInfoVectorLength()!=VectorLength())
      return ShowError(msg+"vdim error VectorLength()="+
		       ToStr(VectorLength())+" BinInfoVectorLength()="+
		       ToStr(BinInfoVectorLength()));

    if (!tolerate_large &&
	BinInfoRawDataLength(augm)>BinInfoRawDataLength(n, augm))
      return ShowError(msg+"size mismatch: "+
		       ToStr(BinInfoRawDataLength(augm))+
		       ">"+ToStr(BinInfoRawDataLength(n, augm))+" "+
		       _bin_data_m.begin()->second.str());

    if (rw && create && v==1.0 &&
	BinInfoRawDataLength(augm)<BinInfoRawDataLength(n, augm))
      return BinDataExpand(n);

    if (!rw && !create && !VectorLength() && HasProperty("recipe"))
      return BinDataOpenVirtual(GetProperty("recipe"));

    return true;
  }
   
  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataOpenVirtual(const string& r) {
    string msg = "VectorIndex::BinDataOpenVirtual("+r+") : ";
    cout << msg << "STARTING" << endl;

    if (r.find("detections=")==0)
      return BinDataOpenVirtualDetections(r);

    if (r.find("transpose=")==0)
      return BinDataOpenVirtualTranspose(r);

    if (r.find("concatenate=")==0)
      return BinDataOpenVirtualConcatenate(r);

    return ShowError(msg+"recipe type not implemented");
  }
   
  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataOpenVirtualDetections(const string& r) {
    string msg = "VectorIndex::BinDataOpenVirtualDetections("+r+") : ";
    cout << msg << "STARTING" << endl;

    if (r.find("detections=")!=0)
      return ShowError(msg+"type not detections");

    string s = r.substr(11);
    size_t p = s.find('#');
    if (p==string::npos)
      return ShowError(msg+"detections recipe should contain '#'");

    string d = s.substr(0, p+1), c = s.substr(p+1);

    cout << msg << "[" << d << "][" << c << "]" << endl;

    DataBase *db = CheckDB();
    size_t n = db->Size();
    list<string> clx = db->SplitClassNames(c);
    vector<string> cl(clx.begin(), clx.end());

    _bin_data_m.begin()->second.open("/dev/null/"+r, true, 1.0,
				     bin_data::header::format_float,
				     0, cl.size());
    _bin_data_m.begin()->second.resize(n);

    for (size_t i=0; i<n; i++) {
      if (!db->ObjectsTargetTypeContains(i, feature_target))
	continue;

      vector<float> v(cl.size());
      for (size_t j=0; j<v.size(); j++) {
	string detname = d+cl[j];
	bool dummy = true;
	map<string,vector<float> > dets
	  = db->RetrieveDetectionData(i, detname, true, dummy);
	float val = 0;
	if (dets.size()!=1)
	  ShowError(msg+detname+" dets.size()!=1 when i="+ToStr(i));
	else if (dets.begin()->second.size()!=1)
	  ShowError(msg+detname+" dets.begin()->second.size()!=1 when i="+
		    ToStr(i));
	else
	  val = dets.begin()->second[0];

	v[j] = val;
      }
      if (!BinDataStoreFeature(i, v))
	return ShowError(msg+"BinDataStoreFeature() failed, i="+ToStr(i));
    }

    return true;
  }
   
  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataOpenVirtualTranspose(const string& r) {
    string msg = "VectorIndex::BinDataOpenVirtualTranspose("+r+") : ";
    cout << msg << "STARTING" << endl;

    if (r.find("transpose=")!=0)
      return ShowError(msg+"type not transpose");

    string s = r.substr(10);
    size_t p = s.find(',');
    if (p==string::npos)
      return ShowError(msg+"transpose recipe should contain ','");

    string f = s.substr(0, p);
    size_t c = atoi(s.substr(p+1).c_str()), z = 0;

    DataBase *db = CheckDB();
    size_t n = db->Size();

    for (size_t i=0; i<n && !z; i++)
      if (db->ObjectsTargetTypeContains(i, feature_target)) {
	FloatVector *v = db->FeatureData(f, i, false);
	if (v) {
	  z = v->Length();
	  delete v;
	  if (z && z%c)
	    return ShowError(msg+ToStr(z)+" is not divisible by "+ToStr(c));
	  z = z/c;
	}
      }

    cout << msg << "[" << f << "] " << c << " " << z << " " << n << endl;

    _bin_data_m.begin()->second.open("/dev/null/"+r, true, 1.0,
				     bin_data::header::format_float, 0, c*z);
    _bin_data_m.begin()->second.resize(n);

    for (size_t i=0; i<n; i++) {
      if (!db->ObjectsTargetTypeContains(i, feature_target))
	continue;

      FloatVector *vv = db->FeatureData(f, i, false);

      vector<float> v(c*z);
      for (size_t j=0, a=0; j<z; j++)
	for (size_t k=0; k<c; k++, a++)
	  v[a] = (*vv)[k*z+j];

      delete vv;

      if (!BinDataStoreFeature(i, v))
       	return ShowError(msg+"BinDataStoreFeature() failed, i="+ToStr(i));
    }

    return true;
  }
   
  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataOpenVirtualConcatenate(const string& r) {
    string msg = "VectorIndex::BinDataOpenVirtualConcatenate("+r+") : ";
    cout << msg << "STARTING" << endl;

    if (r.find("concatenate=")!=0)
      return ShowError(msg+"type not concatenate");

    string s = r.substr(12);
    vector<string> fea = SplitInCommasObeyParentheses(s);

    DataBase *db = CheckDB();
    size_t n = db->Size();

    size_t dim = 0;
    for (size_t i=0; i<n && !dim; i++)
      if (db->ObjectsTargetTypeContains(i, feature_target)) {
	bool first = true;
	for (auto f=fea.begin(); f!=fea.end(); f++) {
	  FloatVector *v = db->FeatureData(*f, i, false);
	  if (!v) {
	    if (first)
	      break;
	    return ShowError(msg+" vector for #"+ToStr(i)+" in <"
			     +*f+"> not found");
	  }
	  first = false;
	  size_t d = v->Length();
	  delete v;
	  
	  if (!d)
	    return ShowError(msg+" zero dim for #"+ToStr(i)+" in <"+*f+">");

	  dim += d;
	}
      }

    cout << msg << " dim=" << dim << endl;

    _bin_data_m.begin()->second.open("/dev/null/"+r, true, 1.0,
		   bin_data::header::format_float, 0, dim);
    _bin_data_m.begin()->second.resize(n);

    for (size_t i=0; i<n; i++) {
      if (!db->ObjectsTargetTypeContains(i, feature_target))
	continue;

      vector<float> cv(dim);
      size_t c = 0;
      for (auto f=fea.begin(); f!=fea.end(); f++) {
	FloatVector *v = db->FeatureData(*f, i, false);
	if (!v)
	  return ShowError(msg+" vector for #"+ToStr(i)+" in <"
			   +*f+"> not found");

	for (size_t a=0; a<(size_t)v->Length(); a++,c++)
	  cv[c] = (*v)[a];
	
	delete v;
      }
      if (!BinDataStoreFeature(i, cv))
	return ShowError(msg+"BinDataStoreFeature() failed, i="+ToStr(i));
    }
    
    return true;
  }
   
  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataOpenFile(bool rw, bool create, float v,
				    const string& augm) {
    string msg = "VectorIndex::BinDataOpenFile("+string(rw?"rw":"ro")
      +","+string(create?"1":"0")+","+ToStr(v)+","+augm+") : ";

    string fff = augm!="" ? augm : FeatureFileName();
    string key = fff==FeatureFileName() ? "" : fff;

    string fname = FindFile(fff, ".bin", false);

    if ((fname=="" || FileSize(fname)==0) && (!rw || !create))
      return true; 
    
    bool is_new = false;
    if (fname=="") {
      fname = Path()+"/"+fff+".bin";
      is_new = true;
    }

    string prodquant = GetProperty("prodquant");
    if (prodquant!="") {
      prodquant = FindFile(prodquant, "", false);
      // prodquant = Path()+"/"+prodquant;
    }

    long nb = is_new ? -1 : FileSize(fname);
    if (nb<1)
      is_new = true;

    msg += "<"+fname+"> : ";

    try {
      _bin_data_m[key].open(fname, rw, v, bin_data::header::format_float,
			    0, VectorLengthForced());

      if (prodquant!="")
	_bin_data_m[key].prodquant(prodquant);

    } catch (const string& s) {
      /*return*/ ShowError(msg+s);
      exit(1);
    }

    string fileinfo = "new";
    if (!is_new)
      fileinfo = "existing "+ToStr(nb)+" bytes ("+HumanReadableBytes(nb)+")";

    WriteLog(string("Opened ")+fileinfo+
	     " bin data file <"+db->ShortFileName(fname)+"> for "+
	     (rw?"modification":"reading")+"\n  "
	     +_bin_data_m[key].str());
    
    Picsom()->PossiblyShowDebugInformation("After opening bin data file");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataExpand(size_t n) {
    string msg = "VectorIndex::BinDataExpand("+ToStr(n)+") : ";

    size_t l = _bin_data_m.begin()->second.rawsize(n);

    WriteLog("Extending bin data file <"+
	     db->ShortFileName(_bin_data_m.begin()->second.filename())+
	     "> from "+ToStr(_bin_data_m.begin()->second.rawsize())+" to "+
	     ToStr(l)+" bytes ("+HumanReadableBytes(l)+")");

    try {
      _bin_data_m.begin()->second.resize(n, 255);
      WriteLog("  "+_bin_data_m.begin()->second.str());
    } catch (string& s) {
      return ShowError(msg+s);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataUnmap() {
    string msg = "VectorIndex::BinDataUnmap() : ";

    /// this is currently a no-op. munmap() is called in ~bin_data() though...

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataStoreFeature(const FloatVector& vec) {
    string msg = "VectorIndex::BinDataStoreFeature(FloatVector) : ";

    //bool debug = false;

    vector<float> v(vec.Length());
    memcpy(&v[0], (float*)vec, vec.Length()*sizeof(float));

    return BinDataStoreFeature(make_pair(v, vec.Label()));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataStoreFeature(const cox::labeled_float_vector& vec) {
    string msg = "VectorIndex::BinDataStoreFeature(labeled_float_vector) : ";

    bool debug = false;

    const vector<float>& fvec = vec.first;
    const string& label = vec.second;
    size_t idx = db->LabelIndex(label);

    if (debug)
      cout << msg << FeatureFileName() << " " << label
	   << " #" << idx << " " << fvec.size() << endl;
    
    return BinDataStoreFeature(idx, fvec);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::BinDataStoreFeature(size_t idx, const vector<float>& vec) {
    string msg = "VectorIndex::BinDataStoreFeature(size_t,vector<float>) : ";

    bool debug = false;

    if (!_bin_data_m.begin()->second.is_incore() && !db->OpenReadWriteFea())
      return ShowError(msg+"not opened with -rw=...fea...");

    if (idx>=db->Size())
      return ShowError(msg+"idx>=db->Size()");

    if (debug)
      cout << msg << FeatureFileName() << " " << db->Label(idx)
	   << " #" << idx << " " << vec.size() << endl;
    
    if (VectorLength()==0)
      data.VectorLength(vec.size());

    if (vec.size()!=VectorLength())
      return ShowError(msg+"vec.size()="+ToStr(vec.size())+
		       " != VectorLength()="+ToStr(VectorLength()));

    const string augm;
    if (!BinDataOpen(true, db->Size(), true, db->BinFeatureVersion(), augm))
      return ShowError(msg+"BinDataOpen(true) failed");

    if (debug) {
      for (size_t i=0; i<vec.size(); i++)
	cout << vec[i] << " ";
      cout << db->Label(idx) << endl;
    }

    if (!_bin_data_m.begin()->second.set_float(idx, vec, db->Size()))
      return ShowError(msg+"idx="+ToStr(idx)+" set_float() failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void VectorIndex::SolveDataFileNames(bool force) {
    if (!force && has_solved_datafile_names)
      return;

    string ns = FeatureFileName();

    WriteLog("Starting to solve datafile names <"+ns+">");

    // This is a cruel hack to prevent FindFile() from finding
    // the parent database's features subdirectory if this is a view db ...

    string secpath = db->SecondaryPath();
    db->SecondaryPath("");

    // The old behavior until 06.09.2005 was to use only the
    // xxx.dat file in the top-level feature directory if it existed.
    // Now, also the leaf directories are scanned unless the following
    // is set to false!
    // 04.0.2006: now controlled by settings.xml, defaults to leafs_too=true
    bool leafs_too = !db->IgnoreLeafFeatures();

    for (int i=0; i<2; i++) {
      string datfile = FindFile(ns, ".dat", true);

      if (datfile!="")
        datfilelist = ExpandIncludes(datfile);
      bool was_includes = !datfilelist.empty();

      if ((leafs_too || datfile=="") && !was_includes)
        // "stripping" a la VectorIndex::FindFile() is not yet implemented here.
        datfilelist = db->LeafDirectoryFileList("features", ns+".dat",
                                                false, true);
      if (datfile!="" && !was_includes)
        AddDataFileName(datfile);

      db->SecondaryPath(secpath); // so this is set when i==1 ...

      if (!datfilelist.empty() || secpath=="" || was_includes)
        break;
    }

    size_t n = datfilelist.size();
    WriteLog("Found "+ToStr(n)+" datafile"+(n!=1?"s":""));
    has_solved_datafile_names = true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::AddDataFileName(const string& datfile) {
    datfilelist.push_front(datfile);
    if (DataBase::DebugLeafDir())
      cout << "Prepended <" << datfile << "> in datfilelist" << endl;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> VectorIndex::ExpandIncludes(const string& master) {
    list<string> ret;

    ifstream dat(master.c_str());
    
    for (;;) {
      string line;
      getline(dat, line);
      if (!dat || line[0]!='#')
        break;

      if (line.find("# include-set ")==0) {
        string set = line.substr(14);
        size_t p = set.find_first_of(" \t");
        if (p!=string::npos)
          set.erase(p);
        TSSOM tmp(db, set, Path()); // obs! TSSOM!!!
        tmp.SolveDataFileNames();
        const list<string>& tmp_list = tmp.DataFileNameList();
        ret.insert(ret.end(), tmp_list.begin(), tmp_list.end());
      }
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::HasDataFileName(const string& n) {
    if (DataBase::DebugLeafDir())
      cout << "VectorIndex::HasDataFileName(" << n << ") : DataFileCount()="
           << DataFileCount() << endl;

    if (!DataFileCount() || n=="")
      return false;
    
    for (list<string>::const_iterator i=datfilelist.begin();
         i!=datfilelist.end(); i++) {
      if (DataBase::DebugLeafDir()>1)
        cout << "  <" << *i << "> : " << (*i==n) << endl;
      if (*i == n)
        return true;      
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVectorSet VectorIndex::DataByIndices(IntVector& idxvector) {
    ReadDataFile();
    SetDataSetNumbers(false, false);

    FloatVectorSet res; 

    if (!data.Nitems() || !data[0].IsAllocated()) {
      ShowError("VectorIndex::DataByIndices(): ",
                data.Nitems()?"!data[0].IsAllocated()":"!data.Nitems()");
      return res;
    }

    for (int i=0; i<idxvector.Length(); i++) {
      int idx = idxvector[i];
      if (idx < 0 || idx > data.Nitems()) {
        ShowError("VectorIndex::DataByIndices(): Index out of bounds: ",
                  ToStr(idx));
        SimpleAbort();
      }

      res.AppendCopy(data[idx]);
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVectorSet VectorIndex::DataByIndices(const vector<size_t>& v,
					    bool tolerate_missing) {
    const string augm;
    if (db->UseBinFeaturesRead())
      return DataByIndicesBin(v, augm, tolerate_missing);

    return db->SqlMode() ? DataByIndicesSql(v) : DataByIndicesClassical(v);
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVectorSet VectorIndex::DataByIndicesAugmented(const vector<size_t>& v,
						     const string& augm,
						     bool tolerate_missing) {
    if (db->UseBinFeaturesRead())
      return DataByIndicesBin(v, augm, tolerate_missing);

    return db->SqlMode() ? DataByIndicesSql(v) : DataByIndicesClassical(v);
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVectorSet VectorIndex::DataByIndicesClassical(const vector<size_t>&
						     idxvec) {
    ReadDataFile();
    SetDataSetNumbers(false, false);

    FloatVectorSet res; 

    for (size_t i=0; i<idxvec.size(); i++) {
      size_t idx = idxvec[i];
      const FloatVector *v = data.FindByNumber((int)idx);
      if (!v) {
        ShowError("VectorIndex::DataByIndices(): Vector not found: #", 
            ToStr(idx), " <"+CheckDB()->Label(idx)+">");
	SimpleAbort();
      } 

      res.AppendCopy(*v);
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVectorSet VectorIndex::DataByIndicesSql(const vector<size_t>& idxvec) {
    FloatVectorSet res; 

    string spec;
    for (size_t i=0; i<idxvec.size(); i++) {
      if (i)
	spec += " OR ";
      spec += "indexz="+ToStr(idxvec[i]);
    }

    return db->SqlFeatureDataSet(FeatureFileName(), spec, true);
    // return SqlDataVectors(spec);
  }

  /////////////////////////////////////////////////////////////////////////////

  // size_t VectorIndex::OpenBinDataOld(const string& ns,
  // 				  string& fname, ifstream& in) {
  //   string msg = "VectorIndex::OpenBinDataOld("+ns+") : ";

  //   Obsoleted(msg);

  //   fname = FindFile(ns, ".bin", true);

  //   if (fname=="")
  //     return ShowError(msg+".bin file not found for <"+ns+">");

  //   in.open(fname.c_str());
  //   if (!in)
  //     return ShowError(msg+"opening <"+fname+"> failed");

  //   float vlf = 0.0;
  //   in.read((char*)&vlf, sizeof(float));
  //   if (!in)
  //     return ShowError(msg+"reading dimensionality of <"+ns+"> failed");
    
  //   size_t vl = size_t(floor(vlf));
  //   if (vlf!=float(vl) || !vl)
  //     return ShowError(msg+"interpreting dimensionality "+ToStr(vlf)+ " of <"+
  // 		       ns+"> failed");

  //   if (!vl)
  //     ShowError(msg+"dimensionality is zero");

  //   return vl;
  // }

  /////////////////////////////////////////////////////////////////////////////

  FloatVectorSet VectorIndex::DataByIndicesBin(const vector<size_t>&
					       idxvec, const string& augm,
					       bool tolerate_missing) {
    string msg = "VectorIndex::DataByIndicesBin() : "+Name()+" ";

    bool verbose = false;

    if (verbose)
      cout << TimeStamp() << msg << "starting <" << FeatureFileName()
	   << "> n=" << idxvec.size() << " augm=" << augm << endl;

    FloatVectorSet empty, res;

    if (!BinDataOpen(false, db->Size(), false, 0.0, augm))
      return ShowError(msg+"BinDataOpen() failed");

    if (!_bin_data_m.begin()->second.is_ok()) {
      ShowError(msg+"bin data not ok");
      return empty;
    }

    // size_t vl = _bin_data_m.begin()->second.get_header_copy().vdim; // until 2013-10-25
    size_t vl = _bin_data_m.begin()->second.vdim();
    if (!vl)  {
      ShowError(msg+"bin data dimension is zero");
      return empty;
    }

    for (size_t i=0; i<idxvec.size(); i++) {
      size_t idx = idxvec[i];

      vector<float> bv = _bin_data_m.begin()->second.get_float(idx);
      if (bv.size()!=vl) {
	string err = msg+"idx="+ToStr(idx)+" vector dim is "+
	  ToStr(bv.size())+" as should be "+ToStr(vl);

	if (tolerate_missing) 
	  WriteLog(err+" continuing still");
	else {
	  ShowError(err+" interrupting");;
	  return empty;
	}
	continue;
      }

      FloatVector v(vl, &bv[0]);
      v.Number(idx);
      v.Label(db->Label(idx));
      res.AppendCopy(v);
    }

    if (verbose)
      cout << TimeStamp() << msg << "ending" << endl;

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////

  // FloatVectorSet VectorIndex::DataByIndicesBinOld(const vector<size_t>&
  // 						  idxvec) {
  //   string msg = "VectorIndex::DataByIndicesBinOld() : ";

  //   Obsoleted(msg);

  //   //bool use_cache = use_bin_cache;
  //   bool verbose   = false;

  //   if (verbose)
  //     cout << TimeStamp() << msg << "starting <" << FeatureFileName()
  // 	   << "> n=" << idxvec.size() << endl;

  //   FloatVectorSet empty, res;

  //   return empty;
  // }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::LmdbDataOpen(bool rw, size_t n, bool create,
				const string& /*augm*/) {
    string dname = BinInfoFileName();
    if (dname.substr(dname.size()-4)==".bin")
      dname.erase(dname.size()-4);
    dname += ".lmdb";
    
    string msg = "VectorIndex::LmdbDataOpen("+ToStr(rw)+","+ToStr(n)+
      ","+ToStr(create)+") <"+dname+"> : ";

#ifdef HAVE_LMDB_H
    if (_lmdb_data.env!=NULL)
      return true;
    
    WriteLog(msg+"starting");
    Picsom()->MkDir(dname, 02775);

    // size_t mapsize = 10485760;
    // mapsize *= 10;
    size_t mapsize = 1024L*1024*1024*1024;

    int r = mdb_env_create(&_lmdb_data.env);
    if (r)
      return ShowError(msg+"mdb_env_create() failed: "+
		       mdb_strerror(r));

    r = mdb_env_set_mapsize(_lmdb_data.env, mapsize);
    if (r)
      return ShowError(msg+"mdb_env_set_mapsize() failed: "+
		       mdb_strerror(r));

    r = mdb_env_open(_lmdb_data.env, dname.c_str(), 0, 02664);
    if (r)
      return ShowError(msg+"mdb_env_open() failed: "+
		       mdb_strerror(r));

    r = mdb_txn_begin(_lmdb_data.env, NULL, 0, &_lmdb_data.txn);
    if (r)
      return ShowError(msg+"mdb_txn_begin() failed: "+
		       mdb_strerror(r));

    r = mdb_dbi_open(_lmdb_data.txn, NULL, 0, &_lmdb_data.dbi);
    if (r)
      return ShowError(msg+"mdb_dbi_open() failed: "+
		       mdb_strerror(r));

    return true;

#else
    return ShowError(msg+"lmdb not supported");
#endif // HAVE_LMDB_H
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::LmdbDataClose() {
    string msg = "VectorIndex::LmdbDataClose() : ";

#ifdef HAVE_LMDB_H
    if (_lmdb_data.env)
      WriteLog("Closing lmdb database");

    if (_lmdb_data.dbi)
      mdb_dbi_close(_lmdb_data.env, _lmdb_data.dbi);
    _lmdb_data.dbi = 0;

    if (_lmdb_data.txn)
      if (mdb_txn_commit(_lmdb_data.txn))
	ShowError(msg+"mdb_txn_commit() failed");
    _lmdb_data.txn = NULL;
    
    if (_lmdb_data.env)
      mdb_env_close(_lmdb_data.env);
    _lmdb_data.env = NULL;
#endif // HAVE_LMDB_H

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::LmdbDataStoreFeature(const FloatVector& vec, 
					 const string& key) {
    string msg = "VectorIndex::LmdbDataStoreFeature(FloatVector) : ";

    vector<float> v(vec.Length());
    memcpy(&v[0], (float*)vec, vec.Length()*sizeof(float));

    return LmdbDataStoreFeature(make_pair(v, key));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::LmdbDataStoreFeature(size_t idx, const vector<float>& vec) {
    string msg = "VectorIndex::LmdbDataStoreFeature(size_t,vector<float>) : ";

    return LmdbDataStoreFeature(make_pair(vec, db->Label(idx)));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::LmdbDataStoreFeature(const cox::labeled_float_vector& v) {
    string msg = "VectorIndex::LmdbDataStoreFeature(labeled_float_vector) : ";

#ifdef HAVE_LMDB_H
    bool debug = false;

    const vector<float>& vec = v.first;
    const string& keytxt     = v.second;
    
    if (debug)
      cout << msg << FeatureFileName() << " " << keytxt
	   << " " << vec.size() << endl;
    
    if (!db->OpenReadWriteFea())
      return ShowError(msg+"not opened with -rw=...fea...");

    if (VectorLength()==0)
      data.VectorLength(vec.size());

    if (vec.size()!=VectorLength())
      return ShowError(msg+"vec.size()="+ToStr(vec.size())+
		       " != VectorLength()="+ToStr(VectorLength()));

    const string augm;
    if (!LmdbDataOpen(true, db->Size(), true, augm))
      return ShowError(msg+"LmdbDataOpen(true) failed");

    if (debug) {
      for (size_t i=0; i<vec.size(); i++)
	cout << vec[i] << " ";
      cout << keytxt << endl;
    }

    MDB_val key = { keytxt.size(), (void*)keytxt.c_str() };
    MDB_val val = { sizeof(float)*vec.size(), (void*)&vec[0] };

    int r = mdb_put(_lmdb_data.txn, _lmdb_data.dbi, &key, &val, 0);
    if (r)
      return ShowError(msg+"key=<"+keytxt+"> mdb_put() failed: "
		       +mdb_strerror(r));

    return true;
#else
    if (v.first.size()) {}
    return ShowError(msg+"lmdb not supported");
#endif // HAVE_LMDB_H
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::SetDataSetNumbers(bool force, bool may_add) {
    string msg = "VectorIndex::SetDataSetNumbers() ["+Name()+"] : ";

    if (!force && data_numbers_set)
      return true;

    Tic("SetDataSetNumbers");

    bool debug = false;
    int max_err = 100, n_err = 0;

    FloatVectorSet &set = Data();

    bool segm_is_mod = db->SegmentIsModifier();

    bool added = false;
    vector<bool> idx(db->Size(), false);

    size_t total = 0, changed = 0;
    int minn = set.MinNumber(), maxn = set.MaxNumber();
    for (int i=0; n_err<max_err && i<set.Nitems(); i++) {
      const char *l = set[i].Label();
      int j = db->LabelIndexGentle(l), n = set[i].Number();

      if (may_add && j==-1) {
        target_type tt = db->SolveTargetTypeFromLabel(l, segm_is_mod);
        j = db->AddLabelAndParents(l, tt, true, true);
        added = true;
        if (j==-1) {
          ShowError(msg+"failed to add label <"+l+">");
          n_err++;
        }
      }

      if (!db->IndexOK(j)) {
        ShowError(msg+"index for <", l, "> not found ");
        n_err++;
      }

      if (j>=(int)idx.size()) {
        if (!may_add) {
          ShowError(msg+"strange index "+ToStr(j)+", idx.size()="+
                    ToStr(idx.size()));
          n_err++;
        }
        size_t vl = j<1024 ? 1024 : j;
        vector<bool> tmp(vl, false); 
        idx.insert(idx.end(), tmp.begin(), tmp.end());
      }

      if (j>=0 && size_t(j)<idx.size()) {
        if (idx[j]) {
          ShowError(msg+"duplicate label <", l, ">");
          n_err++;
        }
        else
          idx[j] = true;
      }

      if (debug && j!=n)
        cout << msg+"changing " << l << "'s (index " 
             << i << ") number " << n << " -> " << j << endl;

      set[i].Number(j);
      total++;
      if (j!=n)
        changed++;
    }

    if (added)
      db->CloseLabelsFile(true);

    if (!n_err) {
      set.ResetMinMaxNumber();

      stringstream ss;
      ss << "[" << minn << ".." << maxn << "] -> ["
         << set.MinNumber() << ".." << set.MaxNumber() << "]";
      WriteLog("Remapped "+ToStr(changed)+"/"+ToStr(total)+
               " vector numbers ", ss.str());
    }

    Tac("SetDataSetNumbers");

    data_numbers_set = n_err==0;

    return data_numbers_set;
  }

  /////////////////////////////////////////////////////////////////////////////

  const FloatVector* VectorIndex::GetDataFromFile(size_t i,
                                                  FloatVectorSet*& data,
                                                  string& current_datfile,
                                                  bool check_most_auth) {
    string msg = "VectorIndex::GetDataFromFile("+ToStr(i)+") : ";

    DataBase *db = CheckDB();
    bool debug = db->DebugLeafDir()>2;

    const FloatVector *v = NULL;
    if (data && !check_most_auth) {
      // v = data->GetByLabel(Label(i));
      v = data->GetByNumber(i);
      if (v && v->LabelStr()!=Label(i)) {
	ShowError(msg+"label mismatch 1 #"+ToStr(i)+" <"+v->LabelStr()+
		  "> <"+Label(i)+">");
	return NULL;
      }
    }

    if (!v) {
      string datfile = db->MostAuthoritativeDataFile(i, this);
      if (datfile.empty()) {
	if (debug)
	  cout << msg << "WARNING: No datfile for " << Label(i) << endl;
        return NULL;
      }

      if (datfile != current_datfile) {
        current_datfile = datfile;

        delete data;
	bool permissive = true;
        data = new FloatVectorSet(datfile.c_str(), -1, permissive);
        if (debug)
          cout << msg << "Read [" << db->ShortFileName(datfile) << "]"
               << " " << Name() << " " << data->Nitems() << "x"
               << data->VectorLength() << endl;

	for (int i=0; i<data->Nitems(); i++)
	  data->Get(i)->Number(db->LabelIndexGentle(data->Get(i)->Label()));
      }
      
      // v = data->GetByLabel(Label(i));
      v = data->GetByNumber(i);
      if (v && v->LabelStr()!=Label(i)) {
	ShowError(msg+"label mismatch 2 #"+ToStr(i)+" <"+v->LabelStr()+
		  "> <"+Label(i)+">");
	return NULL;
      }
    }

    if (debug) {
      cout << msg << "Returning ";
      if (!v)
	cout << "NULL vector";
      else
	cout << "label=" << v->Label() << " number=" << v->Number()
	     << " length=" << v->Length();
      cout << endl;
    }

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> VectorIndex::SqlNames() const {
    string fname = FeatureFileName();
    return db->SqlFeatureNames(fname, true);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::SqlDumpData(ostream& os, bool full, const string& m) {
    //ReadData();
    SetDataSetNumbers(false, false);

    string tname;

    os << DataBase::SqlBeginTransactionCmd(m) << endl
       << SqlTableSchema(VectorLength(), tname) << ";" << endl;

    for (size_t i=0; full && i<(size_t)Data().Nitems(); i++) {
      const FloatVector& v = Data()[i];

      stringstream ss;
      ss << v.Number();

      for (size_t j=0; j<VectorLength(); j++)
	ss << "," << v[j];

      string str = ss.str();
      os << "INSERT INTO " << DataBase::SqlTableName(tname, m)
	 << " VALUES(" << str << ");" << endl;
    }

    os << DataBase::SqlEndTransactionCmd(m) << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string VectorIndex::SqlTableSchema(size_t vl, string& tname,
				     size_t typein) const {
    size_t type = typein;
    type = 2; // BLOB is now forced
    if (type>2) {
      ShowError("VectorIndex::SqlTableSchema() : type>2");
      return "";
    }
    if (type==0)
      type = vl<=50? 1 : 2;

    pair<string,string> sn = SqlNames();
    tname = sn.first;
    string fname = sn.second;
    string spec  = "indexz INTEGER PRIMARY KEY";

    if (type==1) {
      for (size_t i=0; i<vl; i++)
	spec += ","+fname+"_"+ToStr(i)+" FLOAT";
    } else if (vl>0)
      spec += ","+fname+"_0_"+ToStr(vl-1)+" LONGBLOB";

    return "CREATE TABLE "+tname+" ("+spec+")";
  }

  /////////////////////////////////////////////////////////////////////////////

  int VectorIndex::SqlNdataVectors() {
    if (!db->SqlFeatures())
      return -1;

    return db->SqlFeatureDataCount(FeatureFileName(), false);
  }

  /////////////////////////////////////////////////////////////////////////////

  /*
  FloatVectorSet VectorIndex::SqlDataVectors(const string& s) {
    string hdr = "VectorIndex::SqlDataVectors() : ";

    FloatVectorSet ret;
    if (!db->SqlFeatures())
      return ret;

    // this should possibly be in DataBase :

    pair<string,string> sn = SqlNames();
    string tname = sn.first, fname = sn.second;

    string spec = "*";
    // for (size_t i=0; i<VectorLength(); i++)
    //   spec += ","+fname+"_"+ToStr(i);

    string cmd = "SELECT "+spec+" FROM "+db->SqlTableName(tname);
    if (s!="")
      cmd += " WHERE "+s;
    cmd += ";";

    if (db->DebugSql())
      cout << hdr << cmd << endl;

#ifdef HAVE_SQLITE3_H
    if (db->SqliteMode()) {
      sqlite3_stmt *stmt = db->SqlitePrepare(cmd);
      if (!stmt) {
	ShowError(hdr+"failed with ["+cmd+"]");
	return ret;
      }

      int rrr = 0;

      size_t n = 0;
      for (;; n++) {
	rrr = sqlite3_step(stmt);
	if (rrr!=SQLITE_ROW)
	  break;
	
	// db->SqliteShowRow(stmt);

	FloatVector *v = db->DataBase::SqliteFloatVector(stmt);
	ret.Append(v);
      }
      
      rrr = sqlite3_finalize(stmt);

      return ret;
    }
#endif // HAVE_SQLITE3_H

#ifdef HAVE_MYSQL_H
    if (db->MysqlMode()) {
      MYSQL_STMT *stmt = db->MysqlPrepare(cmd);
      if (!stmt) {
	ShowError(hdr+"failed with ["+cmd+"]");
	return ret;
      }

      return ret;
    }
#endif // HAVE_MYSQL_H

    return ret;
  }
  */

  /////////////////////////////////////////////////////////////////////////////

  FloatVector VectorIndex::SqlDataVector(int /*idx*/) {
    FloatVector v(7);

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::SqlInsertFeatureData(const cox::labeled_float_vector& d) {
    string msg = "VectorIndex::SqlInsertFeatureData() : ";

    int idx = db->ToIndex(d.second);
    if (idx<0)
      return ShowError(msg+"no index for <"+d.second+">");

    string tnamex = SqlNames().first;
    if (!db->SqlTableExists(tnamex)) {
      string tname;
      string spec = SqlTableSchema(d.first.size(), tname);

      if (!db->SqlCreateTable("", spec))
	return ShowError(msg+"SqlCreateTable() failed");
    }

    bool is_blob = db->SqlFeatureIsBlob(tnamex);
 
    stringstream ss;
    ss << idx;
    
    if (is_blob && d.first.size()) {
      string s = DataBase::SqlBlob((const char*)&d.first.front(),
				   d.first.size()*sizeof(float));
      ss << "," << s;

    } else
      for (size_t j=0; j<d.first.size(); j++)
	ss << "," << d.first[j];
    
    string str = ss.str(), tname = SqlNames().first, m = db->SqlStyle();
    string cmd = "INSERT INTO "+db->SqlTableName(tname, m)
      +" VALUES ("+str+");";

    if (!db->SqlExec(cmd, true))
      return ShowError(msg+"SqlExec() failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  VectorIndex::classifier_data *VectorIndex::AddClassifier(const string& t,
                                               const string& n, bool angry) {
    classifier_data *c = FindClassifier(n);
    if (c) {
      if (angry) {
        ShowError("VectorIndex::AddClassifier("+t+","+n+") : already exists");
        return NULL;
      }
      return c;
    }

    string key = t+"::"+n;
    WriteLog("Creating ["+t+"] classifier with name ["+n+
             "] and key ["+key+"]");

    classifier_map[key] = classifier_data(t, n);
    
    return &classifier_map[key];
  }

  /////////////////////////////////////////////////////////////////////////////

  VectorIndex::classifier_data *VectorIndex::FindClassifier(const string& n) {
   classifier_map_t::iterator i = classifier_map.find(n);
    if (i!=classifier_map.end())
      return &i->second;

    for (i=classifier_map.begin(); i!=classifier_map.end(); i++)
      if (i->second.name==n)
        return &i->second;
 
    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::DestroyClassifiers() {
    const string  msg = "VectorIndex::DestroyClassifiers() : ";

    while (classifier_map.size()) {
      string name = classifier_map.begin()->first; 
      string type = classifier_map.begin()->second.type;

//       if (type=="svm") {
//         if (!SvmDestroyModel(name))
//           return false;
//         else
//           continue;
//       }

      if (type=="knn" || type=="lsc") {
        if (!DestroyClassifier(&classifier_map.begin()->second, true))
          return false;
        else
          continue;
      }

      return ShowError(msg+"type ["+type+"] unknown");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool VectorIndex::DestroyClassifier(classifier_data *c, bool verbose) {
    for (classifier_map_t::iterator i=classifier_map.begin();
         i!=classifier_map.end(); i++)
      if (c==&i->second) {
        if (verbose)
          WriteLog("Destroying ["+c->type+"] classifier named ["+c->name+"]");

        classifier_map.erase(i);
        return true;
      }

    return ShowError("VectorIndex::DestroyClassifier() failing");
  }

  /////////////////////////////////////////////////////////////////////////////

  VectorIndex::classifier_data::~classifier_data() {
  }

  /////////////////////////////////////////////////////////////////////////////

  string VectorIndex::classifier_data::classify(const cox::knn::lvector_t& v) {
    if (type=="knn")
      return knn.classify(v);
    
    return ShowErrorS("classifier_data::classify() with ["+type+
                      "] undefined");
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<pair<string,double> >
  VectorIndex::classifier_data::class_distances(const cox::knn::lvector_t& v) {
    if (type=="knn")
      return knn.class_distances(v);
    
    ShowError("classifier_data::class_distances with ["+type+"] undefined");

    vector<pair<string,double> > d;
    return d;
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
