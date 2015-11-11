// -*- C++ -*-  $Id: WordInverse.C,v 2.12 2012/11/28 11:38:41 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <WordInverse.h>
#include <DataBase.h>
#include <WordHist.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string WordInverse_C_vcid =
    "@(#)$Id: WordInverse.C,v 2.12 2012/11/28 11:38:41 jorma Exp $";

  static WordInverse list_entry(true);

  bool WordInverse::debug_binary        = false;

  /////////////////////////////////////////////////////////////////////////////

  WordInverse::WordInverse(DataBase *d, const string& n, const string& f, 
			   const string& p, const string& params,
			   const Index* src) : 
    InvertedIndex(d, n, f, p, params, src) {

    binary_classindices = false;
    binary_scaling      = true;

    if (Name()=="")  // e.g. called from Analysis::Analysis().
      return;
    
    SetFeatureNames();

    metfile_str = FindFile(Name(), ".met");

    if (IsBinaryFeatureName(Name()))
      BuildBinary();
    
    else if (IsBinaryFeatureFile(metfile_str))
      BuildBinaryFromFile(params);
  }

  /////////////////////////////////////////////////////////////////////////////

  WordInverse::~WordInverse() {
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WordInverse::State::Initialize() {
    //weight    = vector<float>(1, Index::State::nan());
    bin_feat    = vector<float>();
    binary_data = WordInverse::BinaryFeaturesData();
    objlist     = vector<ObjectList>(1);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WordInverse::BuildBinary() {
    string msg = "WordInverse::BuildBinary() : ";

    static_binary_feature_data.clear();

    string f, keyval;
    if (!SplitParentheses(Name(), f, keyval) || f!="binary")
      return ShowError(msg+"failed 1");

    string metfile, target;
    vector<string> aa = SplitInCommas(keyval);
    for (size_t i=0; i<aa.size(); i++) {
      string key, val;
      if (!SplitKeyEqualValue(aa[i], key, val))
	return ShowError(msg+"failed 2");

      if (key=="metfile" && metfile=="")
	metfile = val;
      else if (key=="target" && target=="")
	target = val;
      else
	return ShowError(msg+"failed 3");
    }

    if (metfile=="")
      return ShowError(msg+"failed 4");

    target_type tt = target_no_target;
    if (target!="") {
      tt = SolveTargetTypes(target);
      if (tt==target_no_target)
	return ShowError(msg+"unable to recognize ["+
			 target+"]");
    }

    if (metfile.length()>4 && metfile.substr(metfile.length()-4)==".met")
      metfile.erase(metfile.length()-4);

    metfile_str = FindFile(metfile, ".met");

    if (metfile_str=="")
      return ShowError(msg+"metfile <", metfile+".met", "> not found");

    // WriteLog("Reading in <", metfile_str(), ">");
  
    ReadMetaDataFile(false, true, false);
    if (tt!=target_no_target && tt!=feature_target)
      FeatureTarget(tt);
    if (feature_target==target_no_target)
      return ShowError(msg+"feature_target==target_no_target");  

    bool expand = true;
    for (meta_data_fields_t::const_iterator i = meta_data_fields.begin();
	 i!=meta_data_fields.end(); i++)
      static_binary_feature_data
	.add(db->GroundTruthExpression(i->first, feature_target, -1, expand));
  
    if (!static_binary_feature_data.ClassCountXXX())
      return ShowError(msg+"no binary classes were found by <",
		       metfile_str, ">");

    stringstream str;
    str << metfile << ".met yielded "
	<< static_binary_feature_data.ClassCountXXX() << " binary features";
    WriteLog(str.str());

    if (binary_classindices)
      static_binary_feature_data.CreateClassIndices(DataSetSize());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WordInverse::BinaryFeaturesData::CreateClassIndices(int size) {
    int n = 0;
    binary_features.resize(size);
    for (size_t i=0; i<binary_features.size(); i++) {
      binary_features[i].clear();
      for (size_t j=0; j<binary_features_inverse.size(); j++)
	if (binary_features_inverse[j][i]==1) {
	  binary_features[i].push_back(j);
	  n++;
	}
    }

    //   stringstream str;
    //   str << "Binary calss indices built for " << binary_features.size()
    //       << " classes with " << n << " entries";
    //   WriteLog(str.str());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WordInverse::CheckBinaryParams(const string& params) {
    vector<string> dv = SplitInCommasObeyParentheses(params);
    for (size_t i=0; i<dv.size(); i++) {
      const string& d = dv[i];
      if (d.find('=')!=string::npos) {
	pair<string,string> kv = SplitKeyEqualValueNew(d);
	if (kv.first=="n") {
	  BinaryInverseFileN(atoi(kv.second.c_str()));
	} else if (kv.first=="weight") {
	  DefaultWeight(atof(kv.second.c_str()));
	} else
	  return ShowError("WordInverse::CheckBinaryParams() "
			   "unknown parameter <", d, ">");
      } else
	return ShowError("WordInverse::CheckBinaryParams() = not found in <"+
			 d+">");
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WordInverse::BuildBinaryFromFile(const string& params) {
    string msg = "WordInverse::BuildBinaryFromFile() : ";

    static_binary_feature_data.clear();
    binary_streampos.clear();

    FeatureTarget(target_text);

    binary_file.close();
    binary_file.clear();
    binary_file.open(metfile_str.c_str());

    if (!binary_file)
      return ShowError(msg+"open() failed");

    Tic("BuildBinaryFromFile");

    for (;;) {
      streampos pos = binary_file.tellg();
      string line;
      getline(binary_file, line);
      if (!binary_file)
	break;
    
      size_t w = line.find_first_not_of(" \t");
      if (w==string::npos)
	continue;

      if (line[0]=='#') {
	vector<string> aa = 
	  SplitInSpaces(line.substr(line.find_first_not_of(" \t",1)));
        
	if (aa[0] == "WORDINVERSEFILE" || aa[0] == "INVERSEFILE") { 
	  for (size_t i=1; i<aa.size(); i++) {
	    string key, val;
	    if (!SplitKeyEqualValue(aa[i], key, val))
	      key = aa[i];
	    if (key.empty()) continue;
	    if (key=="indices")
	      binary_classindices = true;
	    else if (key=="stem")
	      binary_stem = val;
	    else if (key=="noscaling")
	      binary_scaling = false;
	    else if (key=="metaclassfile")
	      binary_metaclassfile = db->ExpandPath("classes",val.empty()?
						    Name():val);
	    else if (key=="classes")
	      binary_metaclassfile = metfile_str;
	    else if (key=="weight")
	      DefaultWeight(atof(val.c_str()));
	    else if (key=="n")
	      binary_inversefile_n = atoi(val.c_str());
	    else if (key=="target")
	      FeatureTarget(val);
	    else 
	      ShowError(msg+" Unknown key: "+key+" - "+val);
	  }
	}
	if (!binary_metaclassfile.empty()) {
	  if (!params.empty()) 
	    CheckBinaryParams(params);
  
	  ReadMetaDataFile(false,false,false,
			   binary_metaclassfile,
			   binary_inversefile_n);

	  for (meta_data_fields_t::const_iterator i = meta_data_fields.begin();
	       i!=meta_data_fields.end(); i++) {
	    Tic("GroundTruthExpression");
	    bool expand = false; // was true (and much slower) until 5.4.2006
	    static_binary_feature_data.add(db->GroundTruthExpression(i->first,
							feature_target, 
							-1, expand));
	    Tac("GroundTruthExpression");
	  }	

	  if (!static_binary_feature_data.ClassCountXXX())
	    return ShowError(msg+"no binary classes were found by",
			     " <", binary_metaclassfile, ">");
	
	  if (binary_classindices) {
	    Tic("CreateClassIndices");
	    static_binary_feature_data.CreateClassIndices(DataSetSize());
	    Tac("CreateClassIndices");
	  }

	  break;
	}
	continue;
      }

      size_t s = line.find_first_of(" \t", w);
      if (s==string::npos) {
	ShowError(msg+"bare word?");
	s = line.size();
      }
    
      string word = line.substr(w, s-w);

      if (binary_streampos.find(word)!=binary_streampos.end())
	ShowError(msg+"duplicate word <"+word+">");

      binary_streampos[word] = pos;

      if (debug_binary)
	cout << "BINFEAT [" << Name() << "] : " << word << " " << pos << endl;
    }

    Tac("BuildBinaryFromFile");

    Picsom()->PossiblyShowDebugInformation(msg+"ending");

    if (binary_streampos.empty() && binary_metaclassfile.empty())
      return ShowError(msg+"failed to create streampos data");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WordInverse::UpdateDynamicBinaryTextFeature(const string& txt,
				         WordInverse::BinaryFeaturesData& fd) {
    
    string msg
      = "WordInverse::UpdateDynamicBinaryTextFeature() ["+Name()+"] : ";

    if (!IsDynamicBinaryTextFeature())
      return ShowError(msg+"not such a feature");

    if (debug_binary)
      cout << "BINFEAT " << msg << "called with [" << txt << "]" << endl;

    fd.clear();
    binary_file.clear();

    vector<string> words = SplitInSpaces(txt);
    for (vector<string>::const_iterator k=words.begin(); k!=words.end(); k++) {
      string word = TextBasedWordPreProcess(*k);
      int gt_index = static_binary_feature_data.ClassIndex(word);

      if (gt_index == -1) {

	if (debug_binary)
	  cout << "BINFEAT preprocess: <" << *k << "> -> <" << word << ">"
	       << endl;
	if (word=="")
	  continue;

	map<string,streampos>::const_iterator p = binary_streampos.find(word);
	if (debug_binary)
	  cout << "BINFEAT " << msg << "<" << word << "> "
	       << (p==binary_streampos.end()?"NOT FOUND":"found at "
		   +ToStr(p->second)) << endl;
	if (p==binary_streampos.end())
	  continue;
					     
	binary_file.seekg(p->second);
	if (!binary_file)
	  return ShowError(msg+"seekg() failed");

	string line;
	getline(binary_file, line);
	if (!binary_file)
	  return ShowError(msg+"getline() failed");

	size_t w = line.find_first_not_of(" \t");
	if (line.find(word, w)!=w)
	  return ShowError(msg+"line corrupt A ???");
    
	w = line.find_first_of(" \t", w);
	if (w==string::npos)
	  return ShowError(msg+"line corrupt B ???");

	w = line.find_first_not_of(" \t", w);
	if (w==string::npos)
	  return ShowError(msg+"line corrupt C ???");

	line.erase(0, w);

	static_binary_feature_data.add(db->GroundTruthLine(line));
	static_binary_feature_data.last().label(word);
	gt_index = static_binary_feature_data.ClassCountXXX()-1;
      }
      fd.add(static_binary_feature_data.GroundTruth(gt_index));
      fd.last().label(word);

      if (debug_binary)
	cout << "BINFEAT " << msg << "<" << word << "> contains "
	     << static_binary_feature_data.last().NumberOfEqual(1)
	     << " objects" << endl;
    }

    if (binary_classindices)
      static_binary_feature_data.CreateClassIndices(DataSetSize());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string WordInverse::TextBasedWordPreProcess(const string& s) const {
    bool stem = true;
    if (binary_stem!="") {
      if (binary_stem=="porter")
	stem = true;
      else
	WarnOnce("WordInverse::TextBasedWordPreProcess() stem!=porter");
    }

    WordHist tb;
    tb.SetDoStem(stem);
    return tb.WordPreProcess(s);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WordInverse::ReadDataFile(bool force, bool nodata, bool wrlocked) {
    if (metfile_str.size())
      return ReadMetaDataFile(force, nodata, wrlocked);
    cout << endl;
    cout << "NO METAFILE!!" << endl;
    cout << endl;
    return false;
  }

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

