// -*- C++ -*-  $Id: Index.C,v 2.89 2018/12/15 23:07:51 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <Index.h>

#include <DataBase.h>
#include <Query.h>
#include <Connection.h>
#include <base64.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string Index_C_vcid =
    "@(#)$Id: Index.C,v 2.89 2018/12/15 23:07:51 jormal Exp $";

  ///
  Index *Index::list_of_indices;

  ///
  Index *Index::dummy_index;

  ///
  bool Index::debug_target_type = false;

  /////////////////////////////////////////////////////////////////////////////

  Index::Index(DataBase *d, const string& n, const string& f, const string& p,
	       const string& params, const Index* src) {
    feature_target = target_no_target;
    CopyFrom(src);
    db       	    = d;
    indexname 	    = n;
    featurefilename = f;
    path 	    = p;  
    next_of_indices = NULL;
    default_weight  = 1.0;

    if (params.find("target=")==0)
      feature_target = SolveTargetTypes(params.substr(7));
  }

  /////////////////////////////////////////////////////////////////////////////

  Index::~Index() {
  }

  /////////////////////////////////////////////////////////////////////////////

  void Index::ShowAllNames() const {
    cout << "FeatureMethodName()=" << FeatureMethodName() << endl;
    cout << "FeatureFileName()=" << FeatureFileName() << endl;
    cout << "MethodName()=" << MethodName() << endl;
    cout << "FeatureFileNameExtraSpec()=" << FeatureFileNameExtraSpec() << endl;
    cout << "FeatureFileNameWithExtra()=" << FeatureFileNameWithExtra() << endl;
    cout << "IndexName()=" << IndexName() << endl;
    cout << "InstanceSpec()=" << InstanceSpec() << endl;
    cout << "IndexNameWithInstanceSpec()=" << IndexNameWithInstanceSpec() << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::CompatibleTarget(target_type t) const {
    // bool t_is_imgseq = t==target_imagesetfile || t==target_videofile;
    bool t_is_imgseq = t==target_imagesetfile;

    if (feature_target==target_image && t_is_imgseq)
      return true;

    if (feature_target==target_segment && (t==target_imagefile||t_is_imgseq))
      return true;

    target_type ottmask = PicSOM::TargetTypeFileFullMasked(t);
    bool ret = (ottmask&feature_target)==ottmask;

    // now (2014-01-23) allows sound features to be extracted from videos if
    // feature_target = sound+video

    // was until 2014-01-23:
    // int a = int(t)&int(feature_target);
    // bool ret = a!=0 && a==int(feature_target);
      
    if (debug_target_type)
      cout << "Index::CompatibleTarget() " << indexname
	   << ": feature_target=<"
	   << TargetTypeString(feature_target) << "> t=<"
	   << TargetTypeString(t) << "> ottmask=<"
	   << TargetTypeString(ottmask) << "> ottmask&feature_target=<"
	   << TargetTypeString(target_type(ottmask&feature_target))
	   << "> feature_target&t=<"
	   << TargetTypeString(target_type(int(t)&int(feature_target)))
	   << "> ret=" << ret << endl;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::Interpret(const string& key, const string& val, int& res) {
    res = 1;

    if (key=="target" || key=="featuretarget") {  // "target" added 2009-12-09
      FeatureTarget(val);
      return true;
    }

    if (key=="longname") {
      longname = val;
      return true;
    }

    if (key=="shorttext") {
      shorttext = val;
      return true;
    }

    res = 0;

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::SetFeatureNames() {
    // string mm = MethodName()+MethodSeparator();
    // featurefilename = indexname;
    // if (featurefilename.find(mm)==0)
    //   featurefilename.erase(0, mm.size());

    // Strip off ending which is not part of feature name.

    // Mats commented away 22.4.2010 because "-" is bad as separator,
    // used e.g. in ColorSIFTds-soft90 which is the name of the full
    // .dat file.

    //    StripLastWord(featurefilename,'-');

    featuremethodname = db->FeatureMethodName(featurefilename);
    if (featuremethodname=="")
      featuremethodname = featurefilename;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Index::CopyFrom(const Index* src) {
    if (!src)
      return;

    db = src->db;
    indexname = src->indexname;
    longname = src->longname;
    shorttext = src->shorttext;
    path = src->path;
    subdir = src->subdir;
    features_command = src->features_command;
    segmentation_command = src->segmentation_command;
    feature_target = src->feature_target;
    tt_stat = src->tt_stat;
    property = src->property;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  void Index::ReorderMethods() {
    Index *tssom = NULL, *prev = NULL, *last = NULL;
    for (Index *m=list_of_indices; m; m=m->next_of_indices) {
      if (m->MethodName()=="tssom")
        tssom = m;
      if (tssom==NULL)
        prev = m;
      last = m;
    }
    if (tssom==NULL || tssom==last)
      return;

    if (prev==NULL)
      list_of_indices = tssom->next_of_indices;
    else
      prev->next_of_indices = tssom->next_of_indices;

    last->next_of_indices = tssom;
    tssom->next_of_indices = NULL;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  void Index::PrintMethodList(ostream& os, bool wide) {
    bool first = true;
    int col = 0;
    for (const Index *p = list_of_indices; p; p=p->next_of_indices) {
      string n = p->MethodName();
      if (wide) {
	os << n << endl;
	continue;
      }
	
      if (!first)
	os << ", ";      
      first = false;
      
      if (col+n.length() > 60) {
	os << endl << "\t";
	col = 0;
      }
      os << n;       
      col += n.length()+2;
    }
  
    if (!wide && col>0)
      os << endl;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  Index* Index::Find(DataBase* db, const string& fn, 
		     const string& dirstr, const string& params,
		     bool create, bool allow_dummy, const Index *src) {
    vector<const Index*> ip;
    
    for (const Index *p = list_of_indices; p; p=p->next_of_indices)
      if (p->IsMakeable(db, fn, dirstr, create))
	ip.push_back(p);

    if (ip.size()==2 && ip[1]->MethodName()=="tssom")
      ip.erase(ip.begin()+1);

    if (ip.size()==2 && ip[0]->MethodName()=="svm" &&
        ip[1]->MethodName()=="precalculated") {
      cout << "WARNING: Index::Find(" << fn 
           << "): found both svm and pre indices, picking the latter." << endl;
      ip.erase(ip.begin());
    }

    if (ip.size()==0 && allow_dummy && dummy_index)
      ip.push_back(dummy_index);

    if (ip.size()!=1) {
      // ShowError("Index::Find(", fn, ") failed! Found ", ToStr(ip.size()), 
      //           " indices.");
      return NULL;
    }

    Index *r = ip[0]->Make(db, fn, fn, dirstr, params, src);

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  Index* Index::Find(DataBase* db, const string& fn, 
                     const list<string>& dirs, const string& params,
                     bool create, bool allow_dummy, const Index* src) {
    Index* r = NULL;
    list<string>::const_iterator it = dirs.begin();
    for (; it != dirs.end() && !r; it++)
      r = Index::Find(db, fn, *it, params, create, allow_dummy, src);

    if (!r)
      ShowError("Index::Find(", fn, ") failed!");

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::CalculateFeatures(const vector<size_t>& idxvin,
				list<incore_feature_t>& incore,
				set<string>& done_segm,
				const XmlDom& xml, bin_data *raw) {
    string tname = "CalculateFeatures<"+Name()+">";

    if (idxvin.size() && incore.size())
      return ShowError(tname+"only one type of input allowed");

    bool is_concepts = featuremethodname=="concepts";
    bool extract_images = !is_concepts;

    set<size_t> idxset;
    vector<size_t> idxv;
    for (auto i=idxvin.begin(); i!=idxvin.end(); i++)
      if (idxset.find(*i)==idxset.end()) {
	idxv.push_back(*i);
	idxset.insert(*i);
      }

    // set to true if you want to remove features_command and
    // segmentation_command after one failure
    bool be_cruel = false;

    ///
    bool be_gentle = Picsom()->IsSlave();

    bool debug_storing = db->DebugFeatures();

    string hdr = "Index::CalculateFeatures() ["+Name()+"] : ";

    bool calculate_internal = true;

    // NOTE: here we assume that all objects in the vector have the same
    // properties as the first one, otherwise it doesn't make sense to
    // calculate them all at once... OBS: should we instead check this??
    int idxfirst = idxv.size() ? idxv.front() : -1;
    const string labelfirst = idxfirst>=0 ? Label(idxfirst) : "";

    if (!CanCalculateFeatures())
      return ShowError(hdr, "no features_command");

    target_type tt = target_no_target;
    stringstream msg;
    if (idxv.size()) {
      tt = db->ObjectsTargetType(idxfirst);

      if (idxv.size()==1) 
	msg << "object #" << idxfirst << " <" << labelfirst << "> type="
	    << TargetTypeString(tt);
      else
	msg << ToStr(idxv.size())+" objects #" << idxfirst << " <" << labelfirst
	    << "> to #" << idxv.back() << " <" << Label(idxv.back())
	    << "> type=" << TargetTypeString(tt);
      if (idxvin.size()!=idxv.size())
	msg << " (" << idxvin.size()-idxv.size() << " duplicates removed)";
      
      if (subdir!="")
	msg << " from subdir=" << subdir;
    }

    if (incore.size()) {
      tt = feature_target; // obs!
      msg << ToStr(incore.size()) << " INCORE objects";
    }

    if (!CompatibleTarget(tt))
      return ShowError(hdr, "target_type mismatch: ",
		       FeatureTargetString(), " vs. ", TargetTypeString(tt));

    WriteLog("Calculating feature vector(s) for ", msg.str()); 

    // FIXME: segm should be done for entire vector as well...

    // string img = db->ObjectPathEvenExtract(idx);
    // if (img=="")
    //   return ShowError(hdr, "ObjectPathEvenExtract() failed");

    list<string> imgs;
    if (extract_images) {
      imgs = db->ObjectPathsEvenExtract(idxv);
      if (idxv.size() && imgs.size()==0)
	return ShowError(hdr, "ObjectPathEvenExtract() failed");
    }

    bool allow_local_features = false;
    string outdir;
    if (!allow_local_features && db->HasExtractedFiles() && idxfirst>=0) {
      outdir = db->ObjectPathEvenExtract(idxfirst, false);
      size_t p = outdir.rfind('/');
      if (p!=string::npos)
	outdir.erase(p);
    }

    if (!segmentation_command.empty()) {
      // this should not happen anymore as segmentations should be
      // done explicitly.
      const string cmdstr = ToStr(segmentation_command);
      if (done_segm.find(cmdstr)==done_segm.end()) {
	CalculateSegmentation(idxv);
	done_segm.insert(cmdstr);
      } else
	WriteLog("Re-using existing segmentation");
    }
  
    Tic(tname.c_str());

    vector<string> cmd(features_command.begin(), features_command.end());

    if (idxv.size()>1) {
      vector<string>::iterator vit = find(cmd.begin(), cmd.end(), "-eOR");
      if (vit!=cmd.end())
	cmd.insert(cmd.erase(vit),"-eOr");
    }

    //   if (segmentation_command.empty()) /// shouln't this be !empty() ????
    //     HackSegmentationCmd(cmd, label);

    size_t fstep = db->FrameStep();
    if (fstep!=1 && feature_target==target_video)
      cmd.insert(cmd.begin()+1, "-s"+ToStr(fstep));

    SetFeaturesDebugging(cmd);

    size_t mgin = db->Margin();
    if (mgin!=0 && feature_target==target_video) {
      cmd.push_back("-o");
      cmd.push_back("margin="+ToStr(mgin));
    }

    bool no_dat_output = db->SqlFeatures() || db->UseBinFeaturesWrite();

    if (!no_dat_output && outdir!="") {
      cmd.insert(cmd.begin()+1, outdir);
      cmd.insert(cmd.begin()+1, "-od");
    }

    if (no_dat_output) {
      for (vector<string>::iterator a=cmd.begin(); a!=cmd.end(); a++) {
	if (*a=="-IB") {
	  a++;
	  if (a==cmd.end())
	    return ShowError("Error modifying feature extraction command");
	  continue;
	}

	if (a!=cmd.begin() && (*a)[0]!='-')
	  break;
	if (a->find("-O")==0) {
	  cmd.erase(a);
	  a = cmd.begin();
	}
	if (*a=="-o") {
	  a = cmd.erase(a);
	  if (a!=cmd.end())
	    cmd.erase(a);
	  a = cmd.begin();
	}
      }
      cmd.insert(cmd.begin()+1, "-Ox");
      if (raw)
	cmd.insert(cmd.begin()+1, "-r");
    }

    set<string> fnameset;
    set<string> tmpfnames;
    vector<string> files;

    // list<string> fnamelist = db->ObjectPathsEvenExtract(idxv);
    list<string> fnamelist = imgs;
    for (auto i=fnamelist.begin(); i!=fnamelist.end(); i++) {
      string fname = *i;

      if (db->SqlObjects())
	tmpfnames.insert(fname);

      if (subdir!="") {
	size_t p = fname.rfind('/');
	if (p!=string::npos) {
	  fname.insert(p+1, subdir+"/"); 
	}
      }

      string fname_extra = fname;

      // fname_extra += "<framerange=100-340>";

      if (fnameset.find(fname_extra)!=fnameset.end())
	continue;

      fnameset.insert(fname_extra);
      files.push_back(fname_extra);
    }

    if (fnameset.size()!=idxv.size())
      WriteLog("Removed "+ToStr(idxv.size()-fnameset.size())+
	       " duplicates from object filename list");

    string fname = CalculatedFeaturePath(labelfirst, false, idxv.size()>1);
    if (FileExists(fname)) {
      if (RenameToOld(fname))
	db->AddCreatedFile(fname+".old");
      else if (!Unlink(fname))
	WarnOnce("Error: unable to rename <"+fname+"> to .old or remove it");
    }

    bool all_vecs_needed = false;
    if (all_vecs_needed) {
      VectorIndex *vi = dynamic_cast<VectorIndex*>(this);
      if (vi)
	vi->ReadDataFile();
      else
	all_vecs_needed = false;
    }

    bool store_vector = all_vecs_needed || db->CanDoDetections();
    bool useinternal  = store_vector || no_dat_output;

    feature_result featres;
    feature_result *featresp = useinternal ? &featres : NULL;

    bool ok = true;
    int r = 1;

    bool threads = db->UsePthreadsFeatures() && files.size()>1;

    if (files.size())
      WriteLog("Extracting "+Name()+" features for "+ToStr(files.size())
	       +" files starting <"+files.front()+">"
	       +(threads?" in threads":""));
    else if (incore.size())
      WriteLog("Extracting "+Name()+" features for "+ToStr(incore.size())
	       +" INCORE objects starting <"+incore.front().first.first
	       +","+incore.front().first.second+">"
	       +(threads?" in threads":""));
    else if (is_concepts && idxv.size())
      WriteLog("Extracting "+Name()+" CONCEPT features for "+
	       ToStr(idxv.size())+" objects starting <"+
	       db->Label(idxv[0])+">"
	       +(threads?" in threads":""));
    else
      return ShowError(tname+" no objects left");

    vector<string> cmd_files = cmd;
    cmd_files.insert(cmd_files.end(), files.begin(), files.end());

    Feature::SetTempDir(db->TempDir());

    if (featuremethodname=="concepts") {
      r = db->CalculateConceptFeatures(this, idxv, files, incore, featresp) 
	? 0 : 1;

    } else {
      if (calculate_internal) {
	cmd_files[0] = "*picsom-features-internal*";
	
	if (threads)
	  r = CalculateFeaturesPthreads(cmd, files, *featresp);
	else
	  r = Feature::Main(cmd_files, incore, featresp);

      } else
	r = Picsom()->ExecuteSystem(cmd_files, true, false, false);
    }

    for (auto i=tmpfnames.begin(); i!=tmpfnames.end(); i++)
      if (!Unlink(*i))
	ShowError(hdr+"failed to unlink <"+*i+">");

    if (r) {
      if (be_cruel) {
	features_command.clear();
	segmentation_command.clear();
      }
      ShowError("Failed to calculate "+Name()+" features for some object"
		" in a list of "+ToStr(idxv.size())+" starting with <"+
		string(imgs.size()?imgs.front():"none")+">");
      ok = be_gentle;
    }

    if (useinternal) {  // obs! used to be if (ok && useinternal)...
      if (db->SqlFeatures() && db->SqlMode()!=2)
	return ShowError(hdr+"SQL database opened for read only");

      if (incore.size() && incore.size()!=featres.data.size())
	return ShowError(hdr+"incore and featres sizes differ");
      auto incore_i = incore.begin();

      VectorIndex *vidx = dynamic_cast<VectorIndex*>(this);
      if (!vidx)
	return ShowError(hdr+"index <"+Name()+"> is not VectorIndex but "
			 +MethodName());

      bool do_pipe = Picsom()->IsSlave()&&
	Picsom()->SlavePipe().find("features")!=string::npos;

      size_t rawidx = raw ? raw->nobjects() : 0;
      for (auto i=featres.data.begin(); i!=featres.data.end(); i++) {
	if (do_pipe) {
	  if (!xml)
	    return ShowError(hdr+"slave/pipe/xml mismatch");

	  /// someday, unify this with DataBase::StoreDetectionResult
	  string fdata((char*)&i->first[0], i->first.size()*sizeof(float));
	  istringstream b64in(fdata);
	  ostringstream b64out;
	  if (!b64::encode(b64in, b64out))
	    return ShowError(hdr+"b64::encode() failed");

	  XmlDom fe = xml.Element("featurevector", "\n"+b64out.str());
	  fe.Prop("database", db->Name());
	  fe.Prop("name", FeatureFileName());
	  fe.Prop("label", i->second);
	  fe.Prop("length", i->first.size());
	  fe.Prop("type", "float");

	} else {
	  if (incore.size()) {
	    incore_i->second = i->first;
	    incore_i++;

	  } else {
	    if (db->SqlFeatures())
	      vidx->SqlInsertFeatureData(*i);

	    if (db->UseBinFeaturesWrite()) {
	      if (!raw)
		vidx->BinDataStoreFeature(*i);
	      else {
		raw->resize(rawidx+1);
		if (!raw->set_float(rawidx++, i->first))
		  return ShowError(hdr+"failed setting "+ToStr(i->first.size())+
				   " dimensional feature vector as #"+
				   ToStr(rawidx-1)+" in "+raw->str());
	      }
	    }
	  }
	}

	if (store_vector) {
	  FloatVector vec(i->first.size(), &i->first[0], i->second.c_str());
	  int dbidx = db->ToIndex(i->second);
	  //cout << "  dbidx=" << dbidx << " label=" << i->second << endl;
	  vec.Number(dbidx);
	  FloatVector *vx = vidx->DataAppendCopy(vec, false);
	  
	  if (debug_storing) {
	    cout << "Storing ";
	    vx->Dump();
	    cout << "  vidx=" << (void*)vidx << " " << vidx->IndexName()
		 << " dbidx=" << dbidx << " NdataVectors=" 
		 << vidx->NdataVectors() << endl;
	  }
	}
      }

      if (raw)
	WriteLog("Extracted "+ToStr(rawidx)+" raw vectors");
    }

    Tac(tname.c_str());

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  struct features_pthread_data {
    ///
    PicSOM *picsom;

    ///
    pthread_t pid;

    ///
    vector<string> cmd;

    ///
    string file;

    ///
    feature_result res;
  };

  int Index::CalculateFeaturesPthreads(vector<string>& cmd,
				       vector<string>& files,
				       feature_result& res) {
    string msg = "Index::CalculateFeaturesPthreads() : ";

    bool debug = Picsom()->DebugThreads();

    size_t nparallel = PicSOM::CpuCount()+2;

    if (FeatureMethodName().find("SIFT")!=string::npos)
      nparallel = (size_t)ceil(nparallel/2.5);

    if (debug)
      cout << msg << "starting with nparallel=" << nparallel
	   << " files.size()=" << files.size() << endl;

    pthread_t nothread = (pthread_t)-1;

    list<features_pthread_data> ptdata;
    for (auto i=files.begin(); i!=files.end(); i++) {
      if (FileSize(*i)<1) {
	cout << msg+"file <"+*i+"> is inexistent/empty, skipping it" << endl;
	continue;
      }
      
      features_pthread_data d = { Picsom(), nothread, cmd,
				  *i, feature_result() };
      ptdata.push_back(d);
    }

    for (auto next = ptdata.begin();;) {
      size_t nrun = 0;
      for (auto i=ptdata.begin(); i!=ptdata.end(); i++)
	if (i->pid!=nothread) {
	  void *ret = NULL;
#ifdef PICSOM_USE_PTHREADS
	  int r = pthread_tryjoin_np(i->pid, &ret);
#else
	  int r = -1;
#endif // PICSOM_USE_PTHREADS
	  if (r==0) {
	    bool has_result = false;
	    if (i->res.dimension()) {
	      has_result = true;
	      res.append(i->res);
	    }
	    i->pid = nothread;
	    if (debug)
	      cout << msg+"Joined thread of <" << i->file << "> "
		   << (has_result?"WITH":"WITHOUT") << " result" << endl;

	  }
	  else
	    nrun++;
	}

      if (nrun==0 && next==ptdata.end())
	break;

      while (nrun<nparallel && next!=ptdata.end()) {
	if (debug)
	  cout << msg+"Starting thread for <" << next->file << ">" << endl;

#ifdef PICSOM_USE_PTHREADS
	int r = pthread_create(&next->pid, NULL,
			       CalculateFeaturesPthreadsCall,
			       &*next);
#else
	int r = -1;
#endif // PICSOM_USE_PTHREADS

	if (r) {
	  ShowError(msg+"pthread_create() failed");
	  return 1;
	}

	next++;
	nrun++;
      }
    }

    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  void *Index::CalculateFeaturesPthreadsCall(void *p) {
    string msg = "Index::CalculateFeaturesPthreadsCall() : ";

    features_pthread_data& d = *(features_pthread_data*)p;

    bool debug = d.picsom->DebugThreads();

    if (debug)
      cout << msg+"in thread... starting <"+d.file+">" << endl;

    vector<string> cmd_files = d.cmd;
    cmd_files.push_back(d.file);

    list<incore_feature_t> incore;
    int r = Feature::Main(cmd_files, incore, &d.res);

    if (debug)
      cout << msg+"in thread... finished <"+d.file+"> with " << r << endl;

    return r ? p : NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::CalculateSegmentation(const vector<size_t>& idxv) {
    bool ok = true;
    for (size_t i=0; ok && i<idxv.size(); i++) 
      ok = CalculateSegmentation(idxv[i], db->ObjectPathEvenExtract(idxv[i]));
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::CalculateSegmentation(size_t idx, const string& imgpath) {
    // This routine was earlier called only by Index::CalculateFeatures(int) 
    // so it is still _supposed_ that most of the sanity checks have 
    // already been made...

    string sname = segmentation_command.size() ?
      segmentation_command.back() : "";

    string tname = "CalculateSegmentation<"+sname+">";

    Tic(tname.c_str());

    string hdr = "Index::CalculateSegmentation() ["+Name()+"] : ";

    bool calculate_internal = true;

    stringstream ssstr;
    for (size_t i=0; i<segmentation_command.size(); i++)
      ssstr << (i?" ":"") << segmentation_command[i];
    string ss = ssstr.str();

    const string& label = Label(idx);
    target_type tt = db->ObjectsTargetType(idx);

    stringstream msg;
    msg << "object #" << idx << " <" << label << "> type="
	<< TargetTypeString(tt);
    WriteLog("Calculating segmentation <"+ss+"> for ", msg.str()); 

    vector<string> segm_command = segmentation_command;
    segm_command.push_back(imgpath);

    int r = 1;

    if (calculate_internal) {
      r = Segmentation::Main(segm_command);
    } else
      r = Picsom()->ExecuteSystem(segm_command, true, false, false);

    Tac(tname.c_str());

    if (r) {
      segmentation_command.clear();
      return ShowError(hdr+"Failed to calculate segmentation");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  /// Returns true if argument is valid object index in the data base.
  bool Index::IndexOK(int i) const { return CheckDB()->IndexOK(i); }

  /// Returns the label of i'th item in data base in a better way.
  const string& Index::Label(int i) const { return CheckDB()->Label(i); }
  
  /// Returns the label of i'th item in data base in a better way.
  const char *Index::LabelP(int i) const { return Label(i).c_str(); }
  
  /// Returns the index of a label or -1 if not known yet.
  int Index::LabelIndex(const string& s) const {
    return CheckDB()->LabelIndex(s);
  }

  /// Returns the target_type of i'th item in data base in a better way.
  target_type Index::ObjectsTargetType(int i) const {
    return CheckDB()->ObjectsTargetType(i);
  }

  /// Returns the number of objects in the data base.
  int Index::DataSetSize() const { return CheckDB()->Size(); }

  ///
  const string& Index::DataBaseName() const { return CheckDB()->Name(); }

  //
  string Index::TargetCounts() const {
    return CheckDB()->TargetTypeStatistics(tt_stat);
  }

  PicSOM *Index::Picsom() const { return db->Picsom(); }

  /// Starts ticking.
  void Index::Tic(const char *f) const { db->Tic(f); }

  /// Stops tacking.
  void Index::Tac(const char *f) const { db->Tac(f); }

  /// Locks for reading.
  void Index::RwLockRead(const char *txt) {   db->RwLockRead(txt); }

  /// Locks for writing.
  void Index::RwLockWrite(const char *txt) {  db->RwLockWrite(txt); }

  /// Unlocks.
  void Index::RwUnlockRead(const char *txt) {
    db->RwUnlockRead(txt); }

  /// Unlocks.
  void Index::RwUnlockWrite(const char *txt) {
    db->RwUnlockWrite(txt); }

  /// Pointer to "" or "W " or "R ".
  const char *Index::RwLockStatusChar() const {
    return db->RwLockStatusChar(); }

  string Index::ShortFileName(const string& n) const {
    return db ? db->ShortFileName(n) : n;
  }

  /////////////////////////////////////////////////////////////////////////////

  char *Index::FindFileM(const char *nn, const char *ext, bool strip) const {
    char n[2000];
    strcpy(n, nn);
    for (;;) {
      for (int i=0; i<3; i++) {
	if (i!=0 && db==NULL)
	  return NULL;

	const char *p = NULL;
	switch (i) {
	case 0: p = Path().c_str();              break;
	case 1: p = db->Path().c_str();          break;
	case 2: p = db->SecondaryPath().c_str();
	}
	if (!p || !*p)
	  continue;
      
	char fname[2048];
	sprintf(fname, "%s%s/%s%s", p, i?"/features":"", n, ext?ext:"");
	// cout << "Index::FindFile("<<fname << ") -> "<<FileExists(fname)<<endl;

	if (FileExists(fname))
	  return strdup(fname);

	strcat(fname, ".gz");
	// cout << "Index::FindFile("<<fname << ") -> "<<FileExists(fname)<<endl;

	if (FileExists(fname))
	  return strdup(fname);
      }
      if (!strip)
	break;
      char *dash = strrchr(n, '-');
      if (!dash)
	break;
      *dash = 0;
    }

    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::CheckPermissions(const Query& r) const {
    cout << "++++++ Index::CheckPermissions " 
	 << r.FirstClientAddress() << " wants to access " << Name() <<endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::SetFeaturesDebugging(vector<string>& cmd) const {
    if (DataBase::DebugFeatures())
      cmd.insert(cmd.begin()+1, "-d"+ToStr(DataBase::DebugFeatures()));
    if (DataBase::DebugSegmentation())
      cmd.insert(cmd.begin()+1, "-D"+ToStr(DataBase::DebugSegmentation()));
    if (DataBase::DebugImages())
      cmd.insert(cmd.begin()+1, "-m"+ToStr(DataBase::DebugImages()));

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Index::AddToXMLfeatureinfo(XmlDom& xml, const Query *q) const {
    XmlDom tree = xml.Element("feature");

    tree.Element("name",       Name());
    tree.Element("longname",   LongName());
    tree.Element("shorttext",  ShortText());
    tree.Element("longtext",   LongText());
    tree.Element("method",     MethodName());
    tree.Element("objecttype", TargetTypeString(feature_target));
    tree.Element("counts",     TargetCounts());

    bool ok = AddToXMLfeatureinfo_static(tree);

    if (ok&&q) {
      int idx = q->IndexShortNameIndex(Name());
      // obs! HAS_CBIRALGORITHM_NEW problem
      const State *s = idx==-1 ? NULL : &q->IndexData(NULL, idx);
      ok = q->AddToXMLfeatureinfo_dynamic(tree, s);
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  float Index::ValueToCDF(float v, int t) {
    const list<float>& cdf = t==0 ? cdf_test : t==1 ? cdf_devel : t==2 ?
      cdf_devel_pos : cdf_devel_neg;

    if (cdf.empty()) {
      ShowError("Index::ValueToCDF was called but no pre has been loaded!");
      return 0.0;
    }

    int i=0;
    list<float>::const_iterator it = cdf.begin();

    while (it != cdf.end() && *it < v) { i++; it++; }
    
    return i/(float)cdf.size();
  }

  /////////////////////////////////////////////////////////////////////////////

  float Index::PDFmodelRho() const {
    if (cdf_devel_pos.size()+cdf_devel_neg.size()==0)
      return 0.0;

    return cdf_devel_pos.size()/
      float(cdf_devel_pos.size()+cdf_devel_neg.size());
  }

  /////////////////////////////////////////////////////////////////////////////

  float Index::PDFmodelCorProb(float v, float a) const {
    if (a==0.0)
      a = PDFmodelRho();

    for (int i=pdf_low.Length()-1; i>=0; i--)
      if (pdf_low[i]<v) {
	float denom = (1-a)*pdf_neg[i]+a*pdf_pos[i];
	float p = denom ? a*pdf_pos[i]/denom : 0;
	return p;
      }
    return 0.0;
  }

  /////////////////////////////////////////////////////////////////////////////

  float Index::PDFmodelPrecision(float v, float a) const {
    if (a==0.0)
      a = PDFmodelRho();

    float possum = 0.0, negsum = 0.0;
    for (int i=pdf_low.Length()-1; i>=0; i--) {
      possum += pdf_pos[i];
      negsum += pdf_neg[i];
      if (pdf_low[i]<v)
	return a*possum/((1-a)*negsum+a*possum);
    }
    return 0.0;
  }

  /////////////////////////////////////////////////////////////////////////////

  float Index::PDFmodelRecall(float v) const {
    for (int i=pdf_low.Length()-1; i>=0; i--)
      if (v>=pdf_low[i])
	return pdf_rec[i];

    return 1.0;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Index::CreatePDFmodels(size_t nin, bool force, bool verbose) {
    if ((size_t)pdf_low.Length()==nin && !force)
      return;

    list<float> all(cdf_test.begin(), cdf_test.end());
    all.insert(all.end(), cdf_devel.begin(), cdf_devel.end());
    all.sort();
    if (verbose)
      cout << "Total " << all.size() << " values for PDF" << endl
	   << "  rho = " << PDFmodelRho() << endl;

    size_t n = all.size()>1 ? nin : 0;

    pdf_low = FloatVector(n, NULL, "low");
    pdf_all = FloatVector(n, NULL, "all");
    pdf_tes = FloatVector(n, NULL, "tes");
    pdf_tra = FloatVector(n, NULL, "tra");
    pdf_pos = FloatVector(n, NULL, "pos");
    pdf_neg = FloatVector(n, NULL, "neg");
    pdf_rec = FloatVector(n, NULL, "rec");

    if (!n)
      return;

    float min = all.front(), max = all.back();

    if (verbose)
      cout << "  min = " << min << endl
	   << "  max = " << max << endl;

    float d = (max-min)/n;
    for (size_t i=0; i<n; i++) {
      float a = min+i*d, b = a+d;
      bool bi = i<n-1;

      pdf_low[i] = a;
      pdf_all[i] = CountInRange(all,           a, true, b, bi)/
	float(all.size());
      pdf_tes[i] = CountInRange(cdf_test,      a, true, b, bi)/
	float(cdf_test.size());
      pdf_tra[i] = CountInRange(cdf_devel,     a, true, b, bi)/
	float(cdf_devel.size());
      pdf_pos[i] = CountInRange(cdf_devel_pos, a, true, b, bi)/
	float(cdf_devel_pos.size());
      pdf_neg[i] = CountInRange(cdf_devel_neg, a, true, b, bi)/
	float(cdf_devel_neg.size());
    }

    for (int i=n-1; i>=0; i--)
      pdf_rec[i] = pdf_pos[i]+(i<int(n)-1?pdf_rec[i+1]:0.0);
  }

  /////////////////////////////////////////////////////////////////////////////

  void Index::AnalysePDF(size_t n) {
    cout << endl << endl;
    CreatePDFmodels(n, true, true);

    float rho = PDFmodelRho();

    if (pdf_all.Length()<2)
      return;

    string fname = "svm_pdf_values.m";
    ofstream os(fname.c_str());

    pdf_all.WriteMatlab(os, true);
    pdf_tes.WriteMatlab(os, true);
    pdf_tra.WriteMatlab(os, true);
    pdf_pos.WriteMatlab(os, true);
    pdf_neg.WriteMatlab(os, true);
    pdf_rec.WriteMatlab(os, true);

    vector<float> ap;
    ap.push_back(0.5);
    ap.push_back(0.1);
    ap.push_back(0.01);
    ap.push_back(0.001);
    ap.push_back(0.0001);
    ap.push_back(rho);
    
    float d = pdf_low[1]-pdf_low[0];

    for (size_t ai=0; ai<ap.size(); ai++) {
      float a = ap[ai];
      string as = ToStr(a);
      size_t pp = as.find('.');
      if (pp!=string::npos)
	as.erase(pp, 1);
      if (a==rho)
	as = "";
      string clab = "cor"+as;
      FloatVector cor(n, NULL, clab.c_str());
      for (size_t i=0; i<n; i++)
	cor[i] = PDFmodelCorProb(pdf_low[i]+d/2, a);
      cor.WriteMatlab(os, true);

      string plab = "pre"+as;
      FloatVector pre(n, NULL, plab.c_str());
      for (int i=n-1; i>=0; i--)
	pre[i] = PDFmodelPrecision(pdf_low[i]+d/2, a);
      pre.WriteMatlab(os, true);
    }

    cout << "Created file <" << fname << ">" << endl << endl << endl;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  size_t Index::CountInRange(const list<float>& l, float a, bool ia,
			       float b, bool ib) {
    size_t n = 0;
    for (list<float>::const_iterator i=l.begin(); i!=l.end(); i++)
      if ((*i>a || (*i==a && ia)) && (*i<b || (*i==b && ib)))
	n++;

    return n;
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

