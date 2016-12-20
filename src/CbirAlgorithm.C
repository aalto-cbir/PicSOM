// -*- C++ -*-  $Id: CbirAlgorithm.C,v 2.40 2016/10/25 08:06:21 jorma Exp $
// 
// Copyright 1998-2016 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <CbirAlgorithm.h>
#include <Query.h>

namespace picsom {
  ///
  static const string CbirAlgorithm_C_vcid =
    "@(#)$Id: CbirAlgorithm.C,v 2.40 2016/10/25 08:06:21 jorma Exp $";

  /// Head of the list of "factory" instances.
  CbirAlgorithm *CbirAlgorithm::list_of_algorithms;

  /// Set to true if corresponding debugging output is desired.
  bool CbirAlgorithm::debug_stages     = false;
  bool CbirAlgorithm::debug_subobjects = false;
  bool CbirAlgorithm::debug_aspects    = false;
  bool CbirAlgorithm::debug_selective  = false;
  bool CbirAlgorithm::debug_weights    = false;
  bool CbirAlgorithm::debug_classify   = false;
  bool CbirAlgorithm::debug_placeseen  = false;
  int  CbirAlgorithm::debug_lists      = 0;

  ///
  bool CbirAlgorithm::relevance_to_siblings = true;

  struct relop_stage_type CbirAlgorithm::expand_operation = 
    { relop_none, relop_copy };

  struct relop_stage_type CbirAlgorithm::exchange_operation = 
    { relop_none, relop_none };

  struct relop_stage_type CbirAlgorithm::converge_operation = 
    { relop_sum, relop_none };

  float CbirAlgorithm::comb_factor = 1.0;

  //===========================================================================
  
  /// This is the only usable constructor.
  CbirAlgorithm::CbirAlgorithm(DataBase *db, const string& args) {
    const string hdr = "CbirAlgorithm::CbirAlgorithm() : ";

    if (debug_stages)
      cout << TimeStamp() << hdr+"constructing an instance with database="
	   << (db?db->Name():"NULL") << " args=\"" << args << "\"" << endl;

    database = db;
  }

  //===========================================================================

  CbirAlgorithm *CbirAlgorithm::FindAndCreateAlgorithm(DataBase *db,
						       const string& n)
    throw (string) {

    string args;
    const CbirAlgorithm *p = FindAlgorithm(n, args);
    if (!p)
      throw string("CBIR algorithm \"")+n+"\" not found";
    
    return p->Create(db, args);
  }

  //===========================================================================

  bool CbirAlgorithm::Initialize(const Query *q, const string& args,
				 QueryData*& qd) const {
    const string hdr = "CbirAlgorithm::Initialize() : ";

    if (debug_stages)
      cout << TimeStamp() << hdr << "args=\"" << args << "\"" << endl;

    qd = CreateQueryData(this, q); 
    qd->ZeroRound();

    const vector<string> ss = SplitInCommasObeyParentheses(args);
    for (vector<string>::const_iterator si = ss.begin();
 	 si != ss.end(); si++) {
      if (si->find('=')==string::npos)
	continue;
      const pair<string,string> kv = SplitKeyEqualValueNew(*si);
      int res = 0;
      if (!Interpret(qd, kv.first, kv.second, res) || !res)
	return ShowError(hdr+"failed to process "+kv.first+"="+kv.second);
    }
    return true; 
  }
  
  //===========================================================================

  bool CbirAlgorithm::AddIndex(QueryData *qd, Index::State *f) const {
    const string hdr = "CbirAlgorithm::AddIndex() : ";
    if (debug_stages)      
      cout << TimeStamp() << hdr+"adding \"" << (f?f->fullname:"") << "\""
	   << endl;	
    
    return qd->AddIndex(f);
  }

  //===========================================================================

  bool CbirAlgorithm::RemoveIndex(QueryData *qd, size_t i) const {
    const string hdr = "CbirAlgorithm::RemoveIndex() : ";
    if (debug_stages)      
      cout << TimeStamp() << hdr+"i="+ToStr(i)+" \"" << IndexFullName(qd, i)
	   << "\"" << endl;
    
    return qd->RemoveIndex(i);
  }

  //===========================================================================
  
  bool CbirAlgorithm::Interpret(QueryData *qd, const string& keyin,
				const string& valstr, int& res) const {
    const string hdr = "CbirAlgorithm::Interpret() : ";

    if (debug_stages)      
      cout << TimeStamp() << hdr+"key=\"" << keyin << "\" value=\""
	   << valstr << "\"" << endl;
    res = 1;

    int   intval   = atoi(valstr.c_str());
    float floatval = atof(valstr.c_str());

    if (keyin=="maxquestions") {
      qd->SetMaxQuestions(intval);
      return true;
    }

    if (keyin=="classaugment") {
      qd->SetClassAugmentValue(floatval);
      return true;
    }

    return false;
  }

  //===========================================================================
  
  ObjectList CbirAlgorithm::CbirRound(QueryData*, const ObjectList& /*seen*/,
				      size_t /*n*/) const {
    const string hdr = "CbirAlgorithm::CbirRound() : ";

    if (debug_stages)
      cout << TimeStamp() << hdr+"starting" << endl;	

    // seenobjects = seen;

    return ObjectList();
  }

  //===========================================================================

  void CbirAlgorithm::PrintAlgorithmList(ostream& os, bool wide) {
    bool first = true;
    int col = 0;
    for (const CbirAlgorithm *p = list_of_algorithms; p;
	 p=p->next_of_algorithms) {

      string n = p->BaseName();
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

  //===========================================================================

  map<string,string> CbirAlgorithm::SplitArgumentString(const string& s) {
    string hdr = "CbirAlgorithm::SplitArgumentString("+s+") : ";
    map<string,string> a;
    vector<string> v = SplitInSomething("_", false, s);
    for (size_t i=0; i<v.size(); i++) {
      pair<string,string> kv;
      try {
	if (v[i].find('=')!=string::npos)
	  kv = SplitKeyEqualValueNew(v[i]);
	else
	  kv.first = v[i];
      } catch (logic_error e) {
	ShowError(hdr+e.what());
      }
      if (a.find(kv.first)!=a.end())
	ShowError(hdr+"key ["+kv.first+"] set twice");
      a[kv.first] = kv.second;
    }

    return a;
  }

  //===========================================================================

  string CbirAlgorithm::LogIdentity(CbirAlgorithm::QueryData* qd) const {
    char txt[1000];
    const string identity_str = qd->GetQuery()->Identity();
    sprintf(txt, "<%s %s>", identity_str!=""?identity_str.c_str():"-",
	    /*connection?connection->Client().IsSet()?
	      ClientAddress():"-":"*",*/ // OBS! Query/Connection
	    database->Name().c_str());
    return txt;
  }

  //===========================================================================

  bool CbirAlgorithm::RelevanceUp(CbirAlgorithm::QueryData* qd,
				  const string& arg, struct relop_stage_type& rs, 
				  ObjectList& il, bool divide, bool seenimg) const {
    qd->GetQuery()->Tic("RelevanceUp");

    bool discard_children=arg.find("discard_children")!=string::npos;

    if (debug_subobjects && discard_children)
      cout << TimeStamp() << "RelevanceUp: discard_children=true" << endl;

    size_t nl = il.size();

    ZeroRelevanceCounters(il);

    if (seenimg) 
      AllSeenReally(il);

    map<int,int> idx;
    for (size_t i=0; i<nl; i++)  // only temporary solution, should use ivp 
      idx[il[i].Index()]=i;

    for (size_t i=0; i<nl; i++) {
      if (!il[i].Retained())
	continue;

      if (il[i].hasParents()) {
	double val = il[i].Value();
	PushRelevanceUp(qd, i, rs, il, val, idx, divide, seenimg);
	if (discard_children)
	  il[i].Discard();
      }
    }

    SumRelevanceCounters(il/*,seenimg*/);
  
    qd->GetQuery()->Tac("RelevanceUp");

    return true;
  }

  //===========================================================================

  bool CbirAlgorithm::RelevanceDown(CbirAlgorithm::QueryData* qd,
				    const string&, struct relop_stage_type& rs, 
				    ObjectList& il, bool divide, bool seenimg) const {
    if (!divide)
      divide = (rs.down == relop_divide);

    size_t nl = il.size();

    map<int,int> idx;
    for (size_t i=0; i<nl; i++)  // only temporary solution, should use ivp 
      idx[il[i].Index()]=i;

    if (seenimg)
      AllSeenReally(il);

    ZeroRelevanceCounters(il);

    for (size_t i=0; i<nl; i++) {  
      if(!il[i].Retained()) continue;

      double val = il[i].Value();
   
      if (il[i].hasChildren())
	PushRelevanceDown(qd, i, rs, il, val, idx, divide, seenimg, -1);
    }
		
    SumRelevanceCounters(il/*,seenimg*/);
  
    return true;
  }

  //===========================================================================

  bool CbirAlgorithm::RelevanceUpDown(CbirAlgorithm::QueryData* qd,
				      const string&, struct relop_stage_type& rs,
				      ObjectList& il, bool divide, bool seenimg) const {
    size_t nl = il.size();

    map<int,int> idx;
    for (size_t i=0; i<nl; i++)  // only temporary solution, should use ivp 
      idx[il[i].Index()]=i;

    if (seenimg)
      AllSeenReally(il);

    ZeroRelevanceCounters(il);

    for (size_t i=0; i<nl; i++) {
      if (il[i].hasChildren())
	PushRelevanceDown(qd, i, rs, il, il[i].Value(),idx,
			  divide, seenimg, -1);
    }
  
    for (size_t i=0; i<nl; i++)
      if (il[i].hasParents())
	PushRelevanceUp(qd, i, rs, il, il[i].Value(),idx, divide, seenimg);
  
    SumRelevanceCounters(il/*,seenimg*/);

    return true;
  }

  //===========================================================================

  void CbirAlgorithm::PushRelevanceUp(CbirAlgorithm::QueryData* qd,
				      size_t i, struct relop_stage_type& rs, 
				      ObjectList& il, double val, map<int,int> &idx,
				      bool divide, bool seenimg) const {

    bool divup = (rs.up == relop_avg);
    bool max = (rs.up == relop_max);

    if (debug_subobjects&&true) {
      cout << TimeStamp() << "PushRelevanceUp(): divide=" << (divide?"true":"false") 
	   << " divup=" << (divup?"true":"false")
	   << " max=" << (max?"true":"false") << endl;
    }

//    if (!val) return;

    //size_t nl = il.size();

    vector<int> parents = il[i].Parents();
    if (parents.empty())
      return;

    for (size_t pari=0; pari<parents.size(); pari++) {
      int pidx = parents[pari];

      //  // search for parent in il list...
      //     bool found = false;
      //     for (size_t j=0; j<nl && !found; j++) {
      //       if (il[j].Index()==pidx) {
      // 	found = true;

      // 	int ch_num = il[j].Children().size();
      // 	double upval = val;
      // 	if (divup && divide) upval /= ch_num;

      // 	il[j].BelowValue(upval);
      // 	if (debug_subobjects)
      // 	  cout << TimeStamp() << "PushRelevanceUp   [" << upval << "]: " << il[j].DumpStr()
      // 	       << " <+ " << il[i].DumpStr() << endl;
	
      // 	PushRelevanceUp(j, il, ivp, upval);

      // 	if (relevance_to_siblings && il[j].Children().size() > 1) 
      // 	  PushRelevanceDown(j, il, ivp, upval, divide, i);
      //       }

      //     }

      // search for parent in il list...
      bool found = idx.count(pidx)>0;
      if(found){
	size_t j=idx[pidx];
	//     for (size_t j=0; j<nl && !found; j++) {
	//       if (il[j].Index()==pidx) {
	// 	found = true;

	int ch_num = il[j].Children().size();
	double upval = val;
	if (divup && divide) upval /= ch_num;

	il[j].BelowValue(upval,max);
	if (debug_subobjects)
	  cout << TimeStamp() << "PushRelevanceUp   [" << upval << "]: " << il[j].DumpStr()
	       << " <+ " << il[i].DumpStr() << endl;
      
	PushRelevanceUp(qd, j, rs, il, upval, idx,
			true /*divide???*/, seenimg);
      
	if (relevance_to_siblings && il[j].Children().size() > 1) 
	  PushRelevanceDown(qd, j, rs, il, upval, idx, divide, seenimg, i);
      }

      if (!seenimg && !CanBeShownRestricted(qd, pidx, false))
	continue;

      if (!found) {
	select_type st = il[i].SelectType();
	if (st==select_show)
	  st = select_question;
	// obs! this doesn't yet handle aspects via AspectsFromIndices();
	Object *obj = new Object(database, pidx, st, 0, il[i].Extra());

	int ch_num = obj->Children().size();
	double upval = val; 
	if (divup && divide) upval /= ch_num;

	obj->CopyStageInfo(il[i]);
	obj->BelowValue(upval,max);
	obj->GenuineRelevance(false);
      
	if (seenimg && qd->GetQuery()->Target() == obj->TargetType())
	  obj->not_really_seen = true;

	/*      if (seenimg) {
		AddToSeen(obj);
		} else {*/
	il.push_back(*obj);
	//if (ivp!=NULL) (*ivp)[obj->Index()] = 1;
	//}
	idx[obj->Index()]=il.size()-1;

	if (debug_subobjects) 
	  cout << TimeStamp() <<  "PushRelevanceUp   [" << upval << "]: " 
	       << obj->DumpStr() << " <= " 
	       << il[i].DumpStr() << endl;
      
	if (relevance_to_siblings && obj->Children().size() > 1) 
	  PushRelevanceDown(qd, il.size()-1, rs, il, upval, idx,
			    divide, seenimg, i);

        delete obj;
      }
    } // for (...parents...)
  }

  //===========================================================================

  void CbirAlgorithm::PushRelevanceDown(CbirAlgorithm::QueryData* qd,
					size_t i, struct relop_stage_type& rs,
					ObjectList& il, double val, map<int,int> &idx, 
					bool divide, bool seenimg, int sibling_i) const {
    //    if (!val) return;

    //size_t nl = il.size();

    const vector<int>& chldrn = il[i].Children();
    size_t num_chldrn = chldrn.size();

    if (!num_chldrn)
      return;

/*    if (debug_subobjects)
      cout << TimeStamp() << "PushRelevanceDown(): divide=" << (divide?"true":"false") << endl;*/

    if (divide)
      val = comb_factor*val/num_chldrn;
    else
      val = comb_factor*val;

    for (size_t subi=0; subi<num_chldrn; subi++) {
      int cidx = chldrn[subi];

      // search for child in il list...
      bool found = idx.count(cidx) > 0;

      //for (size_t j=0; j<nl && !found; j++) {
      //  if (il[j].Index()==cidx) {
      //	found = true;
      if(found){
	size_t j=idx[cidx];
	if (sibling_i != -1) {
	  if (sibling_i != int(j))
	    il[j].SiblingValue(val);
	} else
	  il[j].AboveValue(val);
	  
	if (sibling_i == -1 || sibling_i != int(j)) {
	  if (debug_subobjects)
	    cout << TimeStamp() << "PushRelevanceDown [" << val << "]: " << il[i].DumpStr()
		 << " +> " << il[j].DumpStr() << endl;
	  
	  PushRelevanceDown(qd, j, rs, il, val, idx,
			    true /*divide???*/, seenimg, -1);
	}
      }
    

      // NOTE: we skip this check in case of expand rel. stage
      if (!seenimg && !CanBeShownRestricted(qd, cidx, false))
	continue;

      if (!found) {
	select_type st = il[i].SelectType();
	if (st==select_show)
	  st = select_question;
	// obs! this doesn't yet handle aspects via AspectsFromIndices();
	Object *obj = new Object(database, cidx, st, 0, il[i].Extra());
	obj->CopyStageInfo(il[i]);
	obj->Aspects(il[i].Aspects());

	if (sibling_i != -1) 
	  obj->SiblingValue(val);
	else
	  obj->AboveValue(val);

	obj->GenuineRelevance(false);

	if (seenimg && qd->GetQuery()->Target() == obj->TargetType())
	  obj->not_really_seen = true;
      
	/*if (seenimg) {
	  AddToSeen(obj);
	  } else {*/

	il.push_back(*obj);
	//if (ivp!=NULL) (*ivp)[obj->Index()] = 1;
	idx[obj->Index()]=il.size()-1;
	//}

	if (debug_subobjects)
	  cout << TimeStamp() <<  "PushRelevanceDown [" << val << "]: " << il[i].DumpStr()
	       << " => " << obj->DumpStr() << endl;
        delete obj;
      }
    } // for (...chldrn...)
  } 

  //===========================================================================

  bool CbirAlgorithm::CanBeShownRestricted(CbirAlgorithm::QueryData* qd,
					   int idx, bool checkparents) const {
    // the optimal order of operations might be different ?
    return qd->GetQuery()->CanBeShown(idx, checkparents) &&
      qd->GetQuery()->OkWithDataBaseRestriction(idx) && 
      qd->GetQuery()->OkWithTemporalRestriction(idx);
  }

  //===========================================================================

  map<string,aspect_data>
  CbirAlgorithm::AspectsFromIndices(CbirAlgorithm::QueryData* qd) const {
    map<string,aspect_data> a;
    set<target_type> tts;

    for (size_t i=0; i<qd->Nindices(); i++) {
      const vector<string>& fa = qd->IndexData(i).aspects;
      target_type tt = qd->IndexData(i).StaticPart()->FeatureTarget();
      tts.insert(tt);

      for (size_t j=0; j<fa.size(); j++)
	if (a.find(fa[j])==a.end()) {
	  a[fa[j]] = GetDataBase()->DefaultAspects(tt).AspectData(fa[j]);
	  if (debug_aspects)
	    cout << TimeStamp() << "ASPECTS: found [" << fa[j] << "] in <"
		 << qd->IndexData(i).fullname << "> tt=" << TargetTypeString(tt)
		 << " [" << a[fa[j]].str() << "]" << endl;

	}
    }

    aspects datmp = GetDataBase()->DefaultAspects(target_any_target);
    const map<string,aspect_data>& da = datmp.Aspects();

    for (map<string,aspect_data>::const_iterator i=da.begin();
	 i!=da.end(); i++)
      if (i->second.sticky) {
	a[i->first] = i->second;
	if (debug_aspects)
	  cout << TimeStamp() << "ASPECTS: added [" << i->first << "] ["
	       << a[i->first].str() << "]" << endl;
      }

    return a;
  }

  //===========================================================================

  string CbirAlgorithm::QueryData::ListHead(const string& txt, const string& fns,
					     int m, int l) const {
    char line[1000];
    strcpy(line, txt.c_str());
    if (m>=0 && l>=0) 
      sprintf(line+strlen(line), "(%d,%d):", m, l);
    
    sprintf(line+strlen(line), " query_target=%s",
	    TargetTypeString(GetQuery()->Target()).c_str());
    sprintf(line+strlen(line), " round=%d", round);
    
    if (fns!="")
      sprintf(line+strlen(line), " %s[%d]", fns.c_str(), l);
    if (m>=0) {
      string fts = IndexStaticData(m).FeatureTargetString();
      sprintf(line+strlen(line), " feature_target=%s", fts.c_str());
    }
    
    if (m>=0 && l>=0 && IsTsSomAndLevelOK(m, l)) {
      const TSSOM::State& fd = IndexDataTSSOM(m);
      
      if (fd.aspects.size())
	sprintf(line+strlen(line), " aspects=[%s]",
		CommaJoin(fd.aspects).c_str());      
      
      size_t ls = l;
      const float nan = Index::State::nan();
      if (fd.tssom_level_weight[l]!=nan)
	sprintf(line+strlen(line), " weight=%f", fd.tssom_level_weight[l]);
      else
	strcat(line, " weight=NA");
      strcat(line, " entropy=[");
      if (fd.entropy.size()>ls && fd.entropy[l]!=nan)
	sprintf(line+strlen(line), "%f", fd.entropy[l]);
      strcat(line, "/");
      if (fd.entropy_r.size()>ls && fd.entropy_r[l]!=nan)
	sprintf(line+strlen(line), "%f", fd.entropy_r[l]);
      strcat(line, "/");
      if (fd.entropy_p.size()>ls && fd.entropy_p[l]!=nan)
	sprintf(line+strlen(line), "%f", fd.entropy_p[l]);
      strcat(line, "/");
      if (fd.entropy_n.size()>ls && fd.entropy_n[l]!=nan)
	sprintf(line+strlen(line), "%f", fd.entropy_n[l]);
      strcat(line, "]");
    }
    
    strcat(line, " ---");
    
    return line;
  }

  //===========================================================================

  const string& CbirAlgorithm::QueryData::Label(size_t i) const {
    return query->Label(i);
  }

  //===========================================================================

  bool CbirAlgorithm::QueryData::RemoveIndex(size_t i) {
    if (i>=features.size())
      return false;

    if (DebugStages())
      cout << TimeStamp() << "CbirAlgorithm::RemoveIndex(" << i
	   << ") : about to delete "
	   << features[i]->StaticPart()->MethodName() << " \""
	   << features[i]->StaticPart()->IndexName() << "\" "
	   << (void*)features[i] << " from "
	   << GetQuery()->Identity() << endl;

    delete features[i];
    features.erase(features.begin()+i);

    return true;
  }

  //===========================================================================
  //===========================================================================
  //===========================================================================

} // namespace picsom
