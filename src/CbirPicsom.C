// -*- C++ -*-  $Id: CbirPicsom.C,v 2.43 2011/03/31 09:36:32 jorma Exp $
// 
// Copyright 1998-2011 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <CbirPicsom.h>
#include <Valued.h>
#include <Query.h>

namespace picsom {
  ///
  static const string CbirPicsom_C_vcid =
    "@(#)$Id: CbirPicsom.C,v 2.43 2011/03/31 09:36:32 jorma Exp $";

  /// This is the "factory" instance of the class.
  static CbirPicsom list_entry(true);

  bool CbirPicsom::QueryData::zero_matrices_allowed = true;

  bool CbirPicsom::QueryData::store_selfinfluence = false;

  float CbirPicsom::QueryData::comb_factor = 1.0;
  float CbirPicsom::QueryData::hit_comb_factor=5.0;

  IntVector CbirPicsom::QueryData::watchset;

  hitstype_t CbirPicsom::QueryData::hitstype = trad;

  //===========================================================================

  /// This is the principal constructor.
  CbirPicsom::CbirPicsom(DataBase *db, const string& args)
    : CbirStageBased(db, args) {

    string hdr = "CbirPicsom::CbirPicsom() : ";
    if (debug_stages)
      cout << hdr+"constructing an instance with arguments \""
	   << args << "\"" << endl;

    bottom = weighted = false;

    if (args=="bottom")
      bottom = true;
    if (args=="bottom_weighted")
      bottom = weighted = true;

    if (debug_stages)
      cout << hdr+"contructed instance with FullName()=\"" << FullName()
	   << "\"" << endl;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  const list<CbirAlgorithm::parameter>& CbirPicsom::Parameters() const {
    static list<parameter> l;
    if (l.empty()) {
      parameter bot = { "bottom",   "xsd:boolean", "", "true",
                        "use bottom layer only" };
      parameter wei = { "weighted", "xsd:boolean", "", "false",
                        "use explicit weighting" };
      l.push_back(bot);
      l.push_back(wei);
    }

    return l;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirPicsom::Initialize(const Query *q, const string& s,
			       CbirAlgorithm::QueryData*& qd) const {
    string hdr = "CbirPicsom::Initialize() : ";
    if (!CbirStageBased::Initialize(q, s, qd))
      return ShowError(hdr+"CbirStageBased::Initialize() failed");

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirPicsom::AddIndex(CbirAlgorithm::QueryData *qd,
                            Index::State *f) const {
    string hdr = "CbirPicsom::AddIndex() : ";
    if (!CbirStageBased::AddIndex(qd, f))
      return ShowError(hdr+"CbirStageBased::AddIndex() failed");

    TSSOM::State *p = dynamic_cast<TSSOM::State*>(f);
    if (!p) {
      // return ShowError(hdr+"<"+f->fullname+"> is not TSSOM");
      return true;
    }

    // p->TsSom().ReadFiles(false, false, false, true, true, false, false, false);

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first checks for its own parameters and then calls the
  /// corresponding method of the base class.
  bool CbirPicsom::Interpret(CbirAlgorithm::QueryData *qd, const string& k,
			     const string& v, int& r) const {
    string hdr = "CbirPicsom::Interpret() : ";

    if (debug_stages)      
      cout << hdr+"key=\""+k+"\" value=\""+v+"\"" << endl;

    CbirPicsom::QueryData *qde = CastData(qd);

    r = 1;

    int     intval = atoi(v.c_str());
    float floatval = atof(v.c_str());

    if (k=="seed") {
      qde->seed = intval;
      return true;
    }

    if (k=="permapnewimages" || k=="permapnewobjects") {
      qde->_sa_permapnewobjects = intval;
      return true;
    }

    if (k=="negative_weight") {
      qde->negative_weight = floatval;
      return true;
    }

    return CbirStageBased::Interpret(qd, k, v, r);
  }

  //===========================================================================

  /// This virtual method really needs to be overwritten in a derived
  /// class.
  ObjectList CbirPicsom::CbirRound(CbirAlgorithm::QueryData *qd,
				    const ObjectList& seen,
				    size_t maxq) const {
    string hdr = "CbirPicsom::CbirRound() : ";

    CbirPicsom::QueryData *qde = CastData(qd);

    if (qde->_sa_default_convtype=="")
      qde->_sa_default_convtype = GetQuery(qd)->ConvType(-1);

    if (qde->_sa_permapnewobjects==0)
      qde->_sa_permapnewobjects = GetQuery(qd)->PerMapObjects();

    bool nodata = true;

    for (size_t i=0; i<qde->Nindices(); i++)
      if (qde->IsTsSom(i))
	qde->TsSom(i).ReadFiles(false, nodata, false, true, true,
				false, false, false);

    ObjectList ol = CbirStageBased::CbirRound(qd, seen, maxq);

    if (debug_lists) {
      bool sseen = debug_lists, sother = debug_lists>2;
      qde->ShowUnitAndObjectLists(sseen, sother, false, sother, sother,
                                  sother, false, sother, false);
    }

    return ol;
  }

  //===========================================================================

  cbir_function CbirPicsom::StageDefault(cbir_stage stage, bool warn) const {
    switch (stage) {
    case stage_restrict:                 return func_true;
    case stage_enter:                    return func_default;
    case stage_initial_set:              return func_top_levels_by_entropy;
    case stage_check_image_count:        return func_random_unseen_images;
    case stage_no_positives:             return func_goto_has_positives;
    case stage_has_positives:            return func_goto_expand_relevance;
    case stage_expand_relevance:         return func_expand_down;
    case stage_run_per_map:              return bottom ? func_per_map_picsom_bottom : 
                                                         func_per_map_picsom_all;
    case stage_set_map_weights:          return weighted ? func_entropy_n_polynomial :
                                                  bottom ? func_one_for_bottom :
                                                           func_one_for_all;
    case stage_combine_unit_lists:       return func_default;
    case stage_extract_images:           return bottom ? func_extract_images_picsom_bottom :
                                                         func_extract_images_picsom_all;
    case stage_combine_image_lists:      return func_default;
    case stage_remove_duplicates:        return func_remove_duplicates_picsom;
    case stage_exchange_relevance:       return func_true;
    case stage_select_images_to_process: return func_default;
    case stage_process_images:           return bottom ? func_process_images_picsom_bottom : 
                                                         func_true;
    case stage_converge_relevance:       return func_converge_up;
    case stage_final_select:             return func_default;

    default:
      if (warn)
	ShowError("Query::StageDefaultPicSOM()All stage <",
		  StageName(stage), "> unknown");
      return func_unknown;
    }
  }

  //===========================================================================

  cbir_stage CbirPicsom::StageInitialSet(CbirStageBased::QueryData *qds,
					 cbir_function func,
					 const string& args,
					 size_t maxq) const {
    switch (func) {
    case func_top_levels_by_entropy:
      TopLevelsByEntropy();
      return stage_check_image_count;

    default:
      ;
    }      
    return CbirStageBased::StageInitialSet(qds, func, args, maxq);
  }

  //===========================================================================

  cbir_stage CbirPicsom::StageRunPerMap(CbirStageBased::QueryData *qds,
					cbir_function func,
					const string& args, size_t maxq) const {
    CbirPicsom::QueryData *qde = CastData(qds);    
    bool ok = false;
    switch (func) {
    case func_per_map_picsom_all:
      ok = ForAllMapLevels(qde, &CbirPicsom::PerMap, args, false);
      break;

    case func_per_map_picsom_bottom:
      ok = ForAllMapLevels(qde, &CbirPicsom::PerMap, args, true);
      break;
    
    default:
      return CbirStageBased::StageRunPerMap(qds, func, args, maxq);
    }

    return ok ? stage_set_map_weights : stage_error;
  }

  //===========================================================================

  cbir_stage CbirPicsom::StageSetMapWeights(CbirStageBased::QueryData *qds,
					cbir_function func,
					const string& args, size_t maxq) const {

    CbirPicsom::QueryData *qde = CastData(qds);    

    if (qde->CalculateEntropy() && !qde->DoEntropyCalculation()) {
      ShowError("CbirPicsom::StageSetMapWeights(): DoEntropyCalculation() failed");
      return stage_error;
    }

    bool ok = false;
    switch (func) {
    case func_one_for_all:
      ok = qde->ValueOrNaN(1, true);
      break;

    case func_one_for_bottom:
      ok = qde->ValueOrNaN(1, false);
      break;

    case func_const_for_bottom:
      ok = qde->TsSomWeightValue(false);
      break;

    case func_distances_for_bottom:
      ok = qde->TsSomWeightValueAndDistances(false);
      break;

    case func_entropy_n_polynomial:
      ok = qde->EntropyNpolynomial(args);
      break;

    case func_entropy_n_leave_out_worst:
      ok = qde->EntropyNleaveOutWorst(args);
      break;
    
    default:
      return CbirStageBased::StageSetMapWeights(qds, func, args, maxq);
    }

    if (ok && debug_weights)
      qde->SetMapWeightsDebugInfo();
  
    return ok ? stage_combine_unit_lists : stage_error;
  }

  //===========================================================================

  cbir_stage CbirPicsom::StageCombineUnitLists(CbirStageBased::QueryData* qds,
					       cbir_function func,
					       const string& args,
					       size_t maxq) const {
    if (func!=func_default)
      return  CbirStageBased::StageCombineUnitLists(qds, func, args, maxq);

    bool debug = false;

    bool has_binary = false, has_non_binary = false; // term "binary" here includes
                                                     // precalculated features

    CbirPicsom::QueryData *qde = CastData(qds);

    for (size_t i=0; i<qds->Nindices(); i++)
      if (!qde->IsTsSom(i))
	has_binary = true;
      else
	has_non_binary = true;

    int n = 0, nn = 0;
    for (size_t ii=0; ii<qds->Nindices(); ii++)
      if (qde->IsTsSom(ii))
	for (size_t jj=0; jj<qde->TsSom(ii).Nlevels(); jj++, nn++)
	  n += qde->PerMapVQUnit(ii, jj).size();

    if (debug)
      cout << "CbirPicsom::StageCombineUnitLists(): " << n
	   << " units in total, VQUnitsSize=" << nn << endl;

    // If limit_total_units really would be non-zero we should rethink
    // the way TSSOM weighting is implemented.  Now it is in
    // SelectImagesFromVQUnits().
    
    int ltu = qde->_sa_limit_total_units;
    if (ltu && ltu<n) {
      FloatVector values;
      for (size_t i=0; i<qds->Nindices(); i++)
	if (qde->IsTsSom(i))
	  for (size_t l=0; l<qde->TsSom(i).Nlevels(); l++) {
	    const VQUnitList& ul = qde->PerMapVQUnit(i, l);
	    FloatVector add(ul.size());
	    for (size_t j=0; j<ul.size(); j++)
	      add[j] = ul[j].Value();
	    values.Append(add);
	  }

      values.QuickSortDecreasingly();
      float ref = values[ltu-1];
      n = 0;
      for (size_t k=0; k<qds->Nindices(); k++)
	if (qde->IsTsSom(k))
	  for (size_t l=0; l<qde->TsSom(k).Nlevels(); l++) {
	    VQUnitList& ul = qde->PerMapVQUnit(k, l);
	    for (size_t j=0; j<ul.size(); j++)
	      if (n>=ltu || ul[j].Value()<ref)
		ul[j].Discard();
	      else
		n++;
	  }
    }
  
    if (has_non_binary)
      return n ? stage_extract_images : stage_error;

    return has_binary ? stage_extract_images : stage_error;
  }

  //===========================================================================

  cbir_stage CbirPicsom::StageExtractObjects(CbirStageBased::QueryData* qds,
                                             cbir_function func,
                                             const string& args,
                                             size_t maxq) const {
    bool ok = false;
    CbirPicsom::QueryData *qde = CastData(qds);    
    switch (func) {
    case func_extract_images_picsom_all:
      ok = ForAllMapLevels(qde, &CbirPicsom::ForUnits, args, false);
      break;

    case func_extract_images_picsom_bottom:
      ok = ForAllMapLevels(qde, &CbirPicsom::ForUnits, args, true);
      break;
   
    default:
      return CbirStageBased::StageExtractObjects(qds, func, args, maxq);
    }

    return ok ? stage_combine_image_lists : stage_error;
  }

  //===========================================================================

  cbir_stage CbirPicsom::StageRemoveDuplicates(CbirStageBased::QueryData* qds,
					       cbir_function func,
					       const string& args, size_t maxq) const {
    CbirPicsom::QueryData *qde = CastData(qds);
    bool ok = false;
    switch (func) {
    case func_remove_duplicates_picsom:
      ok = qde->RemoveDuplicates();
      break;

    default:
      return CbirStageBased::StageRemoveDuplicates(qds, func, args, maxq);
    }

    return ok ? stage_exchange_relevance : stage_error;
  }

  //===========================================================================

  cbir_stage CbirPicsom::StageProcessObjects(CbirStageBased::QueryData* qds,
                                             cbir_function func,
                                             const string& args,
                                             size_t maxq) const {
    CbirPicsom::QueryData *qde = CastData(qds);
    bool ok = false;
    switch (func) {
    case func_process_images_picsom_bottom:
      ok = ProcessObjects(qde, args);
      break;
    
    default:
      return CbirStageBased::StageProcessObjects(qds, func, args, maxq);
    }

    return ok ? stage_converge_relevance : stage_error;
  }

  //===========================================================================
  //===========================================================================

  bool CbirPicsom::ForAllMapLevels(CbirPicsom::QueryData* qde,
				   bool (CbirPicsom::*f)(CbirPicsom::QueryData*,
							 const string&,
							 int,int,bool) const,
				   const string& args, bool botonly) const {
#if 0 // QUERY_USE_PTHREADS
    if (picsom->PthreadsTssom())
      return ForAllMapLevelsPthread(f, args, botonly);
#endif // QUERY_USE_PTHREADS

    qde->GetQuery()->Tic("ForAllMapLevels");

    bool ok = true;
    int n = qde->Nindices();
    if (!n)
      ShowError("CbirPicsom::ForAllMapLevels(...) : Nindices()==0");

#ifdef sgi
#pragma parallel
#pragma pfor
#endif // sgi

    for (int i=0; i<n; i++) {
      if (!qde->IsTsSom(i))
        continue;

      int l = qde->NlevelsEvenAlien(i);
      for (int j=0; j<l; j++) {
	if (!(botonly && j<l-1)) {
	  PicSOM::ThreadTestCancel();
	  ok = ok && (this->*f)(qde, args, i, j, botonly);
	}
      }
    }

    qde->GetQuery()->Tac("ForAllMapLevels");

    return ok;
  }

  //===========================================================================

  bool CbirPicsom::PerMap(CbirPicsom::QueryData *qde,
			  const string& args, int i, int j, bool) const {
    string hdr = "CbirPicsom::PerMap() : ";

    CreateAllClassModels(qde);

    if (qde->IsInverseIndex(i) && j==0)
      return SimpleBinaryFeatureScores(i);

    if (qde->IsPreCalculatedIndex(i) && j==0)
      return SimplePreCalculatedScores(i);

    if (qde->IsSvm(i) && j==0)
      return SimpleSvmScores(i);

    if (qde->GetMatrixCount()!=1 && qde->GetMatrixCount()<3)
      return ShowError(hdr, "matrixcount != 1 and <3");

#if 0 //ifdef QUERY_USE_PTHREADS
    char txt[1000];
    sprintf(txt, "CbirPicsom::PerMap(%d, %d)", i, j);
    picsom->ConditionallyAnnounceThread(txt, query_main_thread);
    bool do_tictac = !picsom->PthreadsTssom();
#else
    bool do_tictac = true;
#endif // QUERY_USE_PTHREADS

    bool ok = true;

    if (do_tictac)
      qde->GetQuery()->Tic("PerMapPicSOM");

    if (!qde->PlaceSeenOnMap(i, j, true, qde->limitrelevance)) {
      char tmp[100];
      sprintf(tmp, "(%d,%d)", i, j);
      ShowError(hdr, tmp, ": PlaceSeenOnMap() failed");
      ok = false;
    }
    
    if (ok) {
      if (do_tictac)
	qde->GetQuery()->Tic("Convolve");

      if (qde->hitstype==bernoullimap) {
	if (qde->GetMatrixCount(i)<3) {
	  ShowError(hdr, "hitstype bernoullimap"
		      " requires matrixcount>=3");
	}
	qde->BernoulliMAP(i,j);

      } else 
	PerMapNonBernoulli(qde, args, i, j, ok);
 
      bool bottom_only = true;
      if (!bottom_only || qde->Map(i, j).IsBottomLevel())
	qde->MapSnapshots(i, j);

      if (do_tictac) {
	qde->GetQuery()->Tac("Convolve");
	qde->GetQuery()->Tic("BestVQUnits");
      }

      if (ok)
	qde->PerMapVQUnit(i, j) = BestVQUnits(qde, i, j);

      if (do_tictac)
	qde->GetQuery()->Tac("BestVQUnits");
    }

    if (do_tictac)
      qde->GetQuery()->Tac("PerMapPicSOM");
  
    return ok;
  }

  //===========================================================================

  bool CbirPicsom::DoClassModelAugmentation(CbirPicsom::QueryData *qde,
					    int ii, int j, int k) const {
    const string& mapname = qde->IndexShortName(ii);

    set<string> classes = GetAugmentClasses(qde, mapname);
    
    for (set<string>::iterator it=classes.begin(); it!=classes.end(); it++) {
      const string& classname = *it;

      // MATS: why can empty string come here?
      if (classname=="")
	continue;

      // float my_value = GetAugmentValue(classname, mapname);
      // float default_value = DefaultQuery()->GetAugmentValue(classname,mapname);
      float value = GetClassAugmentValue(qde, classname, mapname);

      if (debug_classify)
	cout << " * " << (value?"A":"Not a") << "ugmenting map " << mapname 
	     << " with class [" << classname << "] with value " << value 
	     << endl;

      if (value != 0) {
	// shouldn't do anything if class model and/or map already exists
	// the_db->AddClassModel(classname,mapname);
	Query *q = GetDataBase()->ClassModel(classname);
	if (!q)
	  return ShowError("DoClassModelAugmentation("+ToStr(ii)+",...) failed"
			   " : no model for classname=["+classname+"]");
	qde->Convolved(ii, j, k) += q->Convolved(mapname, j, k)*value;
      }
    }

    return true;
  }

  //===========================================================================

  set<string> CbirPicsom::GetAugmentClasses(CbirPicsom::QueryData *qde,
					    const string& mapname) const {
    set<string> classes;
    for (classaugment_iter_t it=qde->classaugment.begin(); 
	 it != qde->classaugment.end(); it++) {
      const string& featn = it->first.second;
      if (featn.empty() || featn==mapname)
	classes.insert(it->first.first);
    }
    return classes;
  }

  //===========================================================================

  float CbirPicsom::GetClassAugmentValue(CbirPicsom::QueryData *qde,
				    const string& classname, const string& mapname) const {
    pair<string,string> aug_pair(classname,mapname);

    // check with class,map
    classaugment_iter_t it = qde->classaugment.find(aug_pair);
    float val1 = it==qde->classaugment.end() ? 1.0 : it->second;

    // check with e.g. class,""
    aug_pair.second = "";
    it = qde->classaugment.find(aug_pair);
    float val2 = it==qde->classaugment.end() ? 1.0 : it->second;

    // check with "", ""
    aug_pair.first = "";
    it = qde->classaugment.find(aug_pair);
    float val3 = it==qde->classaugment.end() ? 1.0 : it->second;

    if (debug_classify)
      cout << "GetClassAugmentValue(" << classname << "," << mapname << ") = "
	   << val1 << " * " 
	   << val2 << " * " 
	   << val3 << endl;
    
    // if nothing found return zero
    return val1*val2*val3;
  }

  //===========================================================================

  void CbirPicsom::CreateAllClassModels(CbirPicsom::QueryData *qde) const {
    for (classaugment_iter_t it=qde->classaugment.begin(); 
	 it != qde->classaugment.end(); it++) {
      const string& classn = it->first.first;
      const string& featn  = it->first.second;
      if (classn.empty())
        continue;
      if (!featn.empty())
	GetDataBase()->AddClassModel(classn, featn);
      else 
	for (size_t m=0; m<qde->Nindices(); m++) 
	  GetDataBase()->AddClassModel(classn, qde->IndexShortName(m));
    }
  }

  //===========================================================================

  void CbirPicsom::PerMapNonBernoulli(CbirPicsom::QueryData *qde,
				      const string& args, int i, int j, bool &ok) const {
    string hdr = "CbirPicsom::PerMapNonBernoulli() : ";

    bool trad = false, zero = false, rho = false;
    if (qde->GetMatrixCount()>=3) {
      if (args=="")
	trad = true;
      else if (args.find("zero")!=string::npos)
	zero = true;
      else if (args.find("rho")!=string::npos)
	rho = true;
      else {
	ShowError(hdr, "StageArgs <", args,"> not understood");
	return;
      }      
    }

    bool si=qde->store_selfinfluence && qde->Map(i, j).IsBottomLevel();

    if (qde->GetMatrixCount(i)==1) {
      ok = qde->Convolve(i, j, 0);
      DoClassModelAugmentation(qde, i, j, 0);
      
    } else {                  // matrixcount>=3
      
      ok = qde->Convolve(i, j, 1) && qde->Convolve(i, j, 2);
      
      if (qde->hitstype==smoothed_fraction)
	ok = ok && qde->Convolve(i, j, 0);
      
      if (qde->GetMatrixCount(i)>=4)
	ok = ok && qde->Convolve(i, j, 3);
      
      if (trad) {
	if (qde->hitstype != smoothed_fraction) {
	  qde->Convolved(i, j, 0) = qde->Convolved(i,j,1) + qde->Convolved(i,j,2);
	  DoClassModelAugmentation(qde, i, j, 0);
	}
	
	ok = true;
	
      } else {
	if (si)
	  ShowError(hdr, "for matrixcount != 1 only"
		    " traditional combining implemented");
	double tmp = qde->Convolved(i, j, 1).Sum();
	if (tmp!=0 && tmp!=1) qde->Convolved(i, j, 1) /= tmp;
	tmp = qde->Convolved(i, j, 2).Sum();
	if (tmp!=0 && tmp!=-1) qde->Convolved(i, j, 2) /= -tmp;
	double mul1 = 1, mul2 = 1;
	if (zero)
	  mul2 = 0;
	
	else if (rho==true) { // this is always true...
	  // obs! NpositiveSeenImages() may not return what we expect...
	  double tot = qde->NseenObjects();
	  mul1 = qde->NpositiveSeenObjects()/tot;
	  mul2 = qde->NnegativeSeenObjects()/tot;
	}
	qde->Convolved(i, j, 0) = qde->Convolved(i,j,1)*mul1 + qde->Convolved(i,j,2)*mul2;
      }
    }
    
    if (si)
      PerMapSelfInfluence(qde, i, j);
  }

  //===========================================================================

  void CbirPicsom::PerMapSelfInfluence(CbirPicsom::QueryData *qde,
				       int i, int j) const {
    string hdr = "CbirPicsom::PerMapSelfInfluence() : ";
    bool debug=false;
	  
    for (size_t ii=0; ii<qde->NseenObjects(); ii++) {
      int idx = qde->seenobject[ii].Index();
      if(idx<0) continue;
      int mapidx = qde->Division(i, j)[idx];
      if(mapidx>=0 ){
	if(!qde->selfinfluence.count(idx))
	  ShowError(hdr, "selfinfluence not found for"
		    " seen image", qde->seenobject[ii].Label());
	const TreeSOM& ts = qde->MapEvenAlien(i, j);
	IntPoint p = ts.ToPoint(mapidx);
	
	if(debug)
	  cout << "Multiplying self-influence on map "<<i
	       <<" level="<<j<<endl;
	
	char locstring[80];
	sprintf(locstring,"(%d,%d)",p.X(),p.Y());
	if((int)qde->location_selfinfluence.size()<=i ||
	   (int)qde->location_selfinfluence[i].size()<=p.X() ||
	   (int)qde->location_selfinfluence[i][p.X()].size()<=p.Y())
	  ShowError(hdr, "location_selfinfluence "
		    " not found for point ",locstring,".");
	
	if(debug)
	  cout << "Self-influence of " << qde->seenobject[ii].Label()
	       << " on map "<<i<<" "<<locstring<<": "
	       << qde->selfinfluence[idx][i] << " * " 
	       << qde->location_selfinfluence[i][p.X()][p.Y()] << " = ";
	
	qde->selfinfluence[idx][i] *= qde->location_selfinfluence[i][p.X()][p.Y()];
	
	if(debug)
	  cout << qde->selfinfluence[idx][i] <<endl;
      }
    }
  }

  //===========================================================================

  VQUnitList CbirPicsom::BestVQUnits(CbirPicsom::QueryData *qde,
				     int ii, int jj) const {
    string hdr = "CbirPicsom::BestVQUnits() : ";
    char ticmsg[100];
    sprintf(ticmsg, "BestVQUnits(%d, %d)", ii, jj);
    // Tic(ticmsg);
    bool seenparents = true;
    bool allow_empty_labels = GetDataBase()->EmptyLabelsAllowed();
    
    const simple::FloatMatrix& cnv = qde->Convolved(ii, jj, 0);

    //int showable_units=0,img_tot=0;
    VQUnitList unitret;

    if (qde->IndexIndexOK(ii) && qde->TsSom(ii).LevelOKEvenAlien(jj) && cnv.Size()) {
      int pmni = qde->PerMapObjects();
      bool non_zero_found = false;

      const TreeSOM& map = qde->Map(ii, jj); // was EvenAlien
      for (IntPoint p=map; p; ++p) {
	int mapidx = map.ToIndex(p);
	int labidx = map.Unit(mapidx)->LabelIndex();
	const IntVector& brf = qde->BackReference(ii, jj, mapidx);
	const char *lab = map.Unit(mapidx)->Label();
	if (!lab) {
	  //
	  // we might also set lab and labidx on-fly from brf!
	  //

	  if (!allow_empty_labels) {
	    if (brf.Length())
	      ShowError(hdr+"label=NULL, backrefs exist "
			"consider setting empty_labels_allowed=true");
	    continue;
	  }
	
	  if (labidx!=-1) {
	    ShowError(hdr+"label=NULL, labidx!=-1");
	    continue;
	  }

	} else if (labidx<0) {
	  ShowError(hdr+"labidx<0");
	  continue;
	}

	//img_tot += BackReference(ii, jj, mapidx).Length();

	if (labidx==-1 || !CanBeShownRestricted(qde, labidx, seenparents)) {
	  if (!map.IsBottomLevel())
	    continue;
	  else {
	    bool found = false;
	    for (int i=0; !found && i<brf.Length(); i++)
	      if (CanBeShownRestricted(qde, brf[i], seenparents))
		found = true;

	    if (!found) {
		//cout << "CbirPicsom::QueryData::BestVQUnits(): rejected unit with brf count"
		//     << brf.Length() << endl;
	      continue;
	    }
	  }
        }

	float val = cnv.Get(p.Y(), p.X());
	if (val!=0.0)
	  non_zero_found = true;

	VQUnit candidate(mapidx, val, ii, jj);
	//showable_units++;
	unitret.add_and_sort((size_t)pmni, candidate);
      }

      if (!non_zero_found && !qde->zero_matrices_allowed)
	ShowError(hdr+"only zero values in "+string(ticmsg));
    }

    // Tac(ticmsg);

    //   cout << "BestVQUnits: "<<showable_units<<" showable units." << endl;
    //cout << "Total images in units: "<<img_tot<<endl;
    return unitret;
  }

  //===========================================================================

  bool CbirPicsom::ForUnits(CbirPicsom::QueryData* qde, const string& /*args*/,
			    int i, int j, bool botonly) const {
#if 0 //ifdef QUERY_USE_PTHREADS
    char txt[1000];
    sprintf(txt, "CbirPicsom::ForUnits(%d, %d)", i, j);
    picsom->ConditionallyAnnounceThread(txt, query_main_thread);
    bool do_tictac = !picsom->PthreadsTssom();
#else
    bool do_tictac = true;
#endif // QUERY_USE_PTHREADS

    bool ret = true;

    if (do_tictac)
      qde->GetQuery()->Tic("ForUnitsPicSOM");

    int idx = botonly ? i                : qde->ScoreValueIndex(i, j);
    int len = botonly ? qde->Nindices() : qde->NumberOfScoreValues();

    //    cout << "ForUnitsPicSOM() : i=" << i << " idx=" << idx << " len=" << len << endl;

    if (qde->IsInverseIndex(i) && j==0) 
      qde->PerMapNewObject(i, j) = SelectObjectsByBinaryFeatureScores(i, idx, len);

    else if (qde->IsPreCalculatedIndex(i) && j==0)
      qde->PerMapNewObject(i, j) = SelectObjectsByPreCalculatedScores(i, idx, len);
    
    else if (qde->IsSvm(i) && j==0)
      qde->PerMapNewObject(i, j) = SelectObjectsBySvmScores(i, idx, len);
    
    {
      bool aliendata = qde->TsSom(i).Type()==TSSOM::tssom_alien_data;

      const TSSOM::State& fd = qde->IndexDataTSSOM(i);
      float weight = fd.tssom_level_weight[j];
      if (weight==Index::State::nan()) {
	WarnOnce("CbirPicsom::ForUnits() : "
		 "feature weight == NaN detected for "+
		 qde->TsSom(i).MapName(false, j));
	weight = 0.0;
      }

      qde->PerMapNewObject(i, j) = SelectObjectsFromVQUnits(qde, weight,
                                                            qde->PerMapVQUnit(i, j),
                                                            qde->Map(i, j), //was EvenAlien
                                                            aliendata, idx, len);
    }

    if (do_tictac)
      qde->GetQuery()->Tac("ForUnitsPicSOM");
  
    return ret;
  }

  //===========================================================================

  ObjectList CbirPicsom::SelectObjectsFromVQUnits(CbirPicsom::QueryData* qde,
                                                  double weight,
                                                  const VQUnitList& list,
                                                  const TreeSOM& map,
                                                  bool aliendata,
                                                  int midx, int nmat) const {
    ObjectList imageret;
    
    bool seenparents = true;
    bool allow_empty_labels = GetDataBase()->EmptyLabelsAllowed();

    // Tic("SelectImagesFromVQUnits");

    int pmni = qde->PerMapObjects();

    DataBase *db = GetDataBase();
    
    FloatVector stvec(nmat+1);
    stvec.Set(MAXFLOAT);
    
    for (size_t j=0; j<list.size(); j++) {
      select_type q = select_question;
      int idx = list[j].Index();
      double valw = list[j].Value();
      
      valw *= weight;
      if (debug_weights)
	cout << "1st stage weighting: " << weight << " * " << list[j].Value()
	     << " = " << valw << endl;
      
      int maplabidx = -1;
      if (!aliendata) {
	maplabidx = map.Unit(idx)->LabelIndex();
	if (maplabidx<0 && !allow_empty_labels)
	  ShowError("CbirPicsom::SelectImagesFromVQUnits() empty label problem");
	
	stvec.Last() = stvec[midx] = valw;
	
	if (maplabidx>=0 && CanBeShownRestricted(qde, maplabidx, seenparents)) {
	  Object candidate(db, maplabidx, q, valw, "", 0, &stvec);
	  imageret.add_and_sort((size_t)pmni, candidate);
	}
      }
      
      if (map.IsBottomLevel()) {
	int ii = list[j].MapNumber();
	int jj = list[j].MapLevel();
	const IntVector& brf = qde->BackReference(ii, jj, idx);
	
	for (int i=0; i<brf.Length(); i++) {
	  int labidx = brf[i];
	  if (labidx!=maplabidx &&
	      CanBeShownRestricted(qde, labidx, seenparents)) {
	    if (imageret.will_be_added((size_t)pmni, valw)) {
	      Object candidate(db, labidx, q, valw, "", 0, &stvec);
	      imageret.add_and_sort((size_t)pmni, candidate);
	    }
	  }
	}
      }
    }
    
    // Tac("SelectImagesFromVQUnits");

    return imageret;
  }

  //===========================================================================

  bool CbirPicsom::ProcessObjects(CbirPicsom::QueryData* qde,
                                  const string& arg) const {
    string argcopy=arg;
    
    size_t proppos=argcopy.find("propagate_by_segment_hierarchy");
    bool propagate_by_segment_hierarchy=(proppos!=string::npos);

    if(proppos!=string::npos){
      // remove from string to prevent error message & trapping

      size_t eraselen=strlen("propagate_by_segment_hierarchy");
      if(proppos>0&&argcopy[proppos-1]==','){
	proppos--;
	eraselen++;
      }

      argcopy.erase(proppos,eraselen);

    }

    size_t mlpos=argcopy.find("mlweighting");
    bool mlweighting=(mlpos!=string::npos);

    if(mlpos!=string::npos){
      // remove from string to prevent error message & trapping

      size_t eraselen=strlen("mlweighting");
      if(mlpos>0&&argcopy[mlpos-1]==','){
	mlpos--;
	eraselen++;
      }

      argcopy.erase(mlpos,eraselen);

    }


    size_t mlest=argcopy.find("mlestimate");
    bool mlestimate=(mlest!=string::npos);

    if(mlest!=string::npos){
      // remove from string to prevent error message & trapping

      size_t eraselen=strlen("mlestimate");
      if(mlest>0&&argcopy[mlest-1]==','){
	mlest--;
	eraselen++;
      }

      argcopy.erase(mlest,eraselen);

    }

    size_t afterpos=argcopy.find("argafter");
    if(afterpos!=string::npos){
      int after;
      const char *cptr=argcopy.c_str()+afterpos;
      while(*cptr && *cptr !='=') cptr++;

      if(sscanf(cptr,"=%d",&after)==1){
	if(qde->Round()<=after){
	  argcopy="";
	  cout << "Clearing args to ProcessImagesPicSOMBottom() on round"
	       << qde->Round() << endl; 
	}
      }
      else
	return ShowError("Query::ProcessImagesPicSOMBottom() : StageArgs <",
			 argcopy, "> could not be understood");
    }
    picsom_bottom_score_options_t opt=ParseBottomOptions(qde, argcopy);

    qde->GetQuery()->Tic("ProcessImages");

    bool ok=true;
    
    scorefile_pointers fp;
    OpenScoreFiles(qde, fp);
      
    vector<float> cvals;
    vector<float> posnegvals;

    double *theta=NULL;

    if (mlweighting||mlestimate) {

      vector<vector<float> > scores;
      vector<vector<float> > pos_scores;
      
      cout << "collecting scores for ml weighting" << endl;
      size_t poscount=0,negcount=0;
      for (size_t i=0; i<qde->NseenObjects(); i++) {
	Object& img = qde->seenobject[i];
	if(!(img.TargetType()&qde->GetQuery()->Target())){
	  //cout << "rejected "<<img.Label()<<" bc of targettype."<<endl;
	  continue;
	}
	if(img.Value()!=0){
	  int idx = img.Index();

	  if (idx<0) 
	    ShowError("CbirPicsom::ProcessImages(): Index not solved for ",
		      img.Label());
	  
	  FloatVector stvec(qde->Nindices()+1);
	  stvec.Set(MAXFLOAT);

	  float val=PicSOMBottomScore(qde, idx,opt,&stvec,&img,&cvals,NULL);

	  if(mlestimate) cvals=vector<float>(1,val); // one map model
	  
	  //	  cout << "Score components for "<<((img.Value()>0)?"positive":"negative")<<" example image " << img.Label()<<":"<<endl;
	  //for(size_t c=0;c<cvals.size();c++)
	  //  cout<< " "<<cvals[c];
	  //cout << endl;

	  if(img.Value()>0){
	    poscount++;
	    pos_scores.push_back(cvals);
	  }
	  else{
	    negcount++;
	    scores.push_back(cvals);
	  }
	}
      }

      cout << "collected "<<poscount<<" positive and "<<negcount<<" negative scores."<<endl;
      
      for(size_t i=0;i<pos_scores.size();i++)
	scores.push_back(pos_scores[i]);

      theta=MLEstimateMapWeights(scores,negcount);

      cout << "Dumping train set probs.:" << endl;

      size_t total=scores.size(); 

      double *h=new double[total];

      ml_calculate_h(scores,theta,h);

      cout << "negatives: " << endl;
      size_t i;
      for(i=0;i<negcount;i++){
	cout << h[i] << " (";
	for(size_t j=0;j<scores[i].size();j++)
	  cout << " " << scores[i][j];
	cout <<")"<<endl;
      }
      
      cout << "positives: " << endl;
      
      for(i=negcount;i<total;i++){
	cout << h[i] // << endl;
	     << " (";
	for(size_t j=0;j<scores[i].size();j++)
	  cout << " " << scores[i][j];
	cout <<")"<<endl;
      }
      
      delete[] h;
    }

    for (size_t i=0; i<qde->NnewObjects(); i++) {
      Object& img = qde->newobject[i];
      int idx = img.Index();

      // cout << "processing object "<<i<<": " << img.Label()<<endl;

      if (idx<0) 
	ShowError("CbirPicsom::ProcessImages(): Index not solved for ",
		  img.Label());
      
      FloatVector stvec(qde->Nindices()+1);
      stvec.Set(MAXFLOAT);

      img.Value(0.0);
      float val=PicSOMBottomScore(qde, idx,opt,&stvec,&img,
				  &cvals,&posnegvals);

      if(qde->ScoreFile()!="" && (opt.filedump_ttypecheck==false || 
			   img.TargetType()&qde->GetQuery()->Target()))
	WriteScoreFiles(fp,img.Label(),idx,
			val,cvals,posnegvals);

      if(mlweighting)
	val=ml_calculate_h(cvals,theta);
      else if(mlestimate){
	cvals=vector<float>(1,val);
	val=ml_calculate_h(cvals,theta);
      }

      img.Value(val); 

      stvec.Last() = img.Value();
      img.HandleStageInfo(1, &stvec);
    }

    fp.close();

    for(int i=0;i<qde->watchset.Length();i++){
      bool seen = qde->FindInSeen(qde->watchset[i]) != NULL;
      bool innew = qde->IsInNewObjects(qde->watchset[i]);
      cout << "Status of watch set image " << GetDataBase()->Label(qde->watchset[i]) << ":";
      if(seen) cout << " seen earlier";
      else if(innew) cout <<" among considered images" ;
      else cout << " not shown";
      
      cout << endl;

      float score=PicSOMBottomScore(qde, qde->watchset[i], opt, NULL);
      cout << "Score for watch set image " << GetDataBase()->Label((qde->watchset)[i])
	   << " is " << score << endl;
    }

    if(propagate_by_segment_hierarchy){
      PropagateBySegmentHierarchy();
      if(qde->ScoreFile()!=""){
	string prop_fn=qde->ScoreFile()+"_prop";
	
	FILE *ffp;
	if((ffp=fopen(prop_fn.c_str(),"w"))==NULL)
	  ShowError("CbirPicsom::OpenScoreFiles(): "
		    "failed to open score file ",prop_fn);
	for (size_t i=0; i<qde->NnewObjects(); i++) {
	  Object& img = qde->newobject[i];
	  float val=img.Value();
	  fprintf(ffp,"%s %f\n",img.Label().c_str(),val);
	}

	fclose(ffp);
      }
    }

    //cout << "ProcessImagesPicsomBottom finishing with new list" << endl;
    //for (int i=0; i<NnewImages(); i++) {
    //  Object& img = newimage[i];
    //  cout << "object" << img.Label()<<": "<<img.Value()<<endl;      
    //}

    delete[] theta;
      
    qde->GetQuery()->Tac("ProcessImagesPicSOMBottom");
    
    return ok;
  }

  //===========================================================================

  CbirPicsom::picsom_bottom_score_options_t 
  CbirPicsom::ParseBottomOptions(CbirPicsom::QueryData* qde,
				 const string &arg) const {

      picsom_bottom_score_options_t opt;

      if(arg=="") opt.add=true;

      list<pair<string,string> > pairList=SplitArgs(arg);
      pair<string,string> argpair;
      
      while(pairList.size()>0){
	argpair=pairList.front();
	pairList.pop_front();
	if(argpair.first=="add"){
	  opt.add = true;
	} else if(argpair.first=="mul"){
	  opt.mul = true;
	} else if(argpair.first=="zero"){
	  opt.zero =true;
	} else if(argpair.first=="rho"){
	  opt.rho  = true;
	} else if(argpair.first=="ratioscore"){
	  opt.ratioscore = true;
	} else if(argpair.first=="smoothed_fraction"){
	  opt.smoothed_fraction = true;
	} else if(argpair.first=="relevancemodulation"){
	  opt.relevancemodulation = true;
	}	else if(argpair.first=="subcombination"){
	  if(argpair.second=="maximum"){
	    opt.maxofsubs = true;
	  } else if(argpair.second=="sqrsum"){
	    opt.geomsumsubs = true;
	  } else if(argpair.second=="sublinearrank"){
	    opt.sublinearrank = true;
	  }
	  else
	    return ShowError("CbirPicsom::ParseBottomOptions() : unknown"
			     "subcombination type ",argpair.second);
	}
	else if(argpair.first=="storesubscores"){
	  opt.storesubscores = true;
	}	else if(argpair.first=="posonly"){
	  opt.posonly = true;
	}	else if(argpair.first=="no_propagation"){
	  opt.no_propagation = true;
	}	else if(argpair.first=="filedump_ttypecheck"){
	  opt.filedump_ttypecheck=true;
	}	else if(argpair.first=="equalise_subobject_count"){
	  opt.equalise_subobject_count=true;
	}else{
	  ShowError("CbirPicsom::ParseBottomOptions() : couldn't "
			   "process argument pair (",argpair.first,",",
			   argpair.second,")");
	}
      }

      if (opt.mul==opt.add)
	return ShowError("CbirPicsom::ParseBottomOptions() : mul==add");

      if (opt.zero && opt.rho)
	return ShowError("CbirPicsom::ParseBottomOptions() : zero && rho");

      if (!opt.zero && !opt.rho && !opt.add && !opt.mul)
	return ShowError("CbirPicsom::ParseBottomOptions() : StageArgs <",
		       arg, "> could not be understood");

      if (opt.mul && qde->GetMatrixCount()<3)
	return ShowError("CbirPicsom::ParseBottomOptions() : "
			 "mul && matrixcount<3");
      

      if( (opt.maxofsubs ? 1 : 0) + (opt.geomsumsubs ? 1 : 0) +
	  (opt.sublinearrank ? 1 : 0) > 1)
	return ShowError("CbirPicsom::ParseBottomOptions() : "
		       "several methods for combining subobject scores given");

      if (opt.ratioscore && qde->GetMatrixCount()<3)
	return ShowError("CbirPicsom::ParseBottomOptions() : "
			 "ratioscore && matrixcount<3");
      if (opt.smoothed_fraction && qde->GetMatrixCount()<3)
	return ShowError("CbirPicsom::ParseBottomOptions() : "
			 "smoothed_fraction && matrixcount<3");
      if (opt.relevancemodulation && qde->GetMatrixCount()<3)
	return ShowError("CbirPicsom::ParseBottomOptions() : "
			 "relevancemodulation && matrixcount<3");
      if (opt.posonly && qde->GetMatrixCount()<3)
	return ShowError("CbirPicsom::ParseBottomOptions() : "
			 "posonly && matrixcount<3");

      return opt;
  }

  //===========================================================================

  list<pair<string,string> > CbirPicsom::SplitArgs(const string &arg) {

    string strleft=arg;
    list<pair<string,string> > ret;
    pair<string,string> tmppair;

    while(strleft!=""){
      string pairStr;
      size_t commapos=strleft.find(',');
      if(commapos==string::npos){ //last argument
	pairStr=strleft;
	strleft="";
      }
      else{
	pairStr=strleft.substr(0,commapos);
	strleft=strleft.substr(commapos+1);
      }
      
      size_t eqpos=pairStr.find('=');
      if(eqpos==string::npos){
	tmppair.first=pairStr;
	tmppair.second="";
      } else{
	tmppair.first=pairStr.substr(0,eqpos);
	tmppair.second=pairStr.substr(eqpos+1);
      }
      ret.push_back(tmppair);
    }
    return ret;
  }

  //===========================================================================

  bool CbirPicsom::OpenScoreFiles(CbirPicsom::QueryData* qde,
				  scorefile_pointers& fp) const {
    string scorefile = qde->ScoreFile();
    if (scorefile!="") {
      //cout << "opening scorefiles w/ prefix "<<scorefile<<endl;
      string component_fn=scorefile+"_comp", combined_fn=scorefile, 
	//reduced_fn=scorefile+"_red", 
	posneg_fn=scorefile+"_posneg"; 
	//posneg_red_fn=scorefile+"_posneg_red",
	//sum_red_fn=scorefile+"_sum_red";
   
      if ((fp.fp=fopen(combined_fn.c_str(),"w"))==NULL)
	ShowError("CbirPicsom::OpenScoreFiles(): "
		  "failed to open score file ",combined_fn);

      if ((fp.fp_comp=fopen(component_fn.c_str(),"w"))==NULL)
	ShowError("CbirPicsom::OpenScoreFiles(): "
		  "failed to open score file ",component_fn);
      
      if (qde->GetMatrixCount()>=3){
	if((fp.fp_posneg=fopen(posneg_fn.c_str(),"w"))==NULL)
	  ShowError("CbirPicsom::OpenScoreFiles(): "
		    "failed to open score file ",posneg_fn);
      }
    }

    return true;
  }

  //===========================================================================
  
  bool CbirPicsom::WriteScoreFiles(const scorefile_pointers &fp, const string &lbl,
				   int /*idx*/,
				   float val, const vector<float> &cvals,
				   const vector<float> &posnegvals) const {
    if(fp.fp){
      fprintf(fp.fp,"%s %f\n",lbl.c_str(),val);
    }

    if(fp.fp_comp){
      fprintf(fp.fp_comp,"%s",lbl.c_str());
      for(size_t i=0;i<cvals.size();i++)
        fprintf(fp.fp_comp," %f",cvals[i]);
      fprintf(fp.fp_comp,"\n");
    }

    if(fp.fp_posneg){
      fprintf(fp.fp_posneg,"%s",lbl.c_str());
      for(size_t i=0;i<posnegvals.size();i++)
        fprintf(fp.fp_posneg," %f",posnegvals[i]);
      fprintf(fp.fp_posneg,"\n");
    }
    
    return true;
  }

  //===========================================================================

  float CbirPicsom::PicSOMBottomScore(CbirPicsom::QueryData *qde, int idx,
				 const picsom_bottom_score_options_t &opt,
				 FloatVector *stvec, Object *img,
				 vector<float> *cvals,
				 vector<float> *posnegvals) const {
    float ret;
    vector<float> rootvals;
    vector<float> subvals;
    const vector<int> sub= GetDataBase()->SubObjects(idx);
    const string &lbl = GetDataBase()->Label(idx);
  
    bool store_posneg= posnegvals && qde->GetMatrixCount()>=3;

    bool si_stored = qde->store_selfinfluence && qde->selfinfluence.count(idx)>0 && 
      qde->selfinfluence[idx].size()==qde->Nindices();

    if(opt.equalise_subobject_count && (opt.add==false||opt.ratioscore)){
      ShowError("CbirPicsom::PicSOMBottomScore() : equalise_subobject_count"
		" currently implemented only for additive scoring");
      return 0;
    }

    if(debug_selective){
      cout << "Calculating score for <" << lbl <<">"<< endl;  
    }

    float classw = qde->ClassWeight(idx);
    if (debug_weights)
      cout << "Class weight = " << classw << " for:"
	   << GetDataBase()->ObjectDump(idx) << endl;

    if(opt.ratioscore)
      subvals=vector<float>(sub.size(),1.0);
    else
      subvals=vector<float>(sub.size(),0.0);

    double pprod = 1, nprod = 1;
    double rscore=1;
    
    if(cvals){
      cvals->clear();
      cvals->resize(qde->Nindices(),0.0);
    }
    if(store_posneg){
      posnegvals->clear();
      posnegvals->resize(2*qde->Nindices(),0.0);
    } else if(posnegvals)
      posnegvals->resize(0);

    for (size_t j=0; j<qde->Nindices(); j++) {
      double v = 0;

      if (qde->IsInverseIndex(j))
	v = BinaryFeatureScoreForObject(j, idx);

      else if (qde->IsPreCalculatedIndex(j))
	v = PreCalcultedScoreForObject(j, idx);

      else if (qde->IsTsSom(j)) {
	int bot = qde->NlevelsEvenAlien(j)-1;
	const TreeSOM &ts = qde->MapEvenAlien(j, bot);
	const IntVector& div = qde->Division(j, bot);

	const TSSOM::State& fd = qde->IndexDataTSSOM(j);
	
	float weight = fd.tssom_level_weight[bot];
	
	// OBS! this is a hack
	
	weight *= qde->Weight(j);
	
	if (weight==Index::State::nan()) {
	  WarnOnce("CbirPicsom::PicSOMBottomScore() : "
		   "feature weight == NaN detected for "+
		   qde->TsSom(j).MapName(false, bot));
	  weight = 0.0;
	}

	int positives = 0, negatives = 0;         
	double bias = DetermineCountBias();
	if(opt.ratioscore||opt.relevancemodulation||opt.smoothed_fraction)
	  qde->CountPositivesAndNegatives(div, positives, negatives, true);
	
	int mapidx = div.IndexOK(idx) ? div[idx] : -1;
	
	if(mapidx>=0){
	  
	  IntPoint p = ts.ToPoint(mapidx);
	  
	  float sij=0;
	  if(si_stored){
	    sij=qde->selfinfluence[idx][j];
	    //	      cout << "Self-influence for root object "
	    //		   << the_db->Label(idx)
	    //		   << ": "<<si<<endl; 
	  }
	  
	  v = opt.add ? (qde->Convolved(j, bot, 0).Get(p.Y(), p.X())-sij) : 0;

	  if(opt.smoothed_fraction){
	    const float bbias=0.05;
	    double pos = positives*qde->Convolved(j, bot, 1).Get(p.Y(), p.X());
	    double neg = -negatives*qde->Convolved(j, bot, 2).Get(p.Y(), p.X());
	    v=(pos+bbias)/(pos+neg+2*bbias);
	  }

	  if(store_posneg){
	    
	    (*posnegvals)[2*j]=qde->Convolved(j, bot, 1).Get(p.Y(), p.X());
	    (*posnegvals)[2*j+1]=qde->Convolved(j, bot, 2).Get(p.Y(), p.X());
	    if(sij>0)
	      (*posnegvals)[2*j] -= sij;
	    else
	      (*posnegvals)[2*j+1] -= sij;
	  }

	  if(debug_selective)
	    cout << "root object score: " << v <<endl; 

	  if(opt.ratioscore||opt.relevancemodulation){

	    double pos = positives*qde->Convolved(j, bot, 1).Get(p.Y(), p.X());
	    double neg = -negatives*qde->Convolved(j, bot, 2).Get(p.Y(), p.X());
	    
	    if(opt.ratioscore){
	      rscore *= (pos+bias)/(neg+bias);
	      if(cvals) (*cvals)[j]= (pos+bias)/(neg+bias);
	    }
	    if(opt.relevancemodulation){
	      v *= (pos+bias)/(neg+bias);	
	      if(debug_selective)
		cout << " * relevance multiplier " << (pos+bias)/(neg+bias) 
		     << " ->"<<v <<endl; 
	    }
	  }
	
	  if (opt.mul) {
	    pprod *=  qde->Convolved(j, bot, 1).Get(p.Y(), p.X());
	    nprod *= -qde->Convolved(j, bot, 2).Get(p.Y(), p.X());
	  }

	} else v=0; // mapidx >= 0

	if (opt.no_propagation==false /*use_subobjects*/) {

	  int subcount=0;

	  if(opt.equalise_subobject_count) // count subobjects on this map
	    for (size_t subi=0; subi<sub.size(); subi++) 
	      if(div.IndexOK(sub[subi])&&div[sub[subi]]>=0) subcount++;

	  for (size_t subi=0; subi<sub.size(); subi++) {
	    
	    int mapidx_sub = div.IndexOK(sub[subi]) ? div[sub[subi]] : -1;
	    if(mapidx_sub>=0){
	      IntPoint p = ts.ToPoint(mapidx_sub);
	      
	      float ssi=0;
	      bool ssi_stored = qde->store_selfinfluence &&
		qde->selfinfluence.count(sub[subi])>0 && 
		qde->selfinfluence[sub[subi]].size()==qde->Nindices();
	      
	      if(ssi_stored){
		ssi=qde->selfinfluence[sub[subi]][j];
	      }

	      if (opt.add){
		float multiplier=1.0*weight;
		if(opt.ratioscore||opt.relevancemodulation){
		  double pos = positives*qde->Convolved(j, bot, 1).Get(p.Y(), p.X());
		  double neg = -negatives*qde->Convolved(j, bot, 2).Get(p.Y(), p.X());
		  if(opt.ratioscore){
		    subvals[subi] *= (pos+bias)/(neg+bias);
		    if(cvals) (*cvals)[j]*= (pos+bias)/(neg+bias);
		  }

		  if(opt.relevancemodulation)
		    multiplier = (pos+bias)/(neg+bias);
		}
		
		if(debug_selective){
		  cout << "   subobject " << GetDataBase()->Label(sub[subi])
		       << " ->("<<p.X()<<","<<p.Y()<<") on ("<<j<<","
		       <<bot<<"):"<< endl;
		  cout << "    " << multiplier <<"*"
		       << qde->Convolved(j, bot, 0).Get(p.Y(), p.X()) <<"="
		       << multiplier*qde->Convolved(j, bot, 0).Get(p.Y(), p.X())
		       << endl;
		}
	      
		float subscore=multiplier*
		  qde->Convolved(j, bot, opt.posonly ? 1 :0).Get(p.Y(), p.X());
		
		if(opt.posonly==false || ssi>0)
		  subscore -= multiplier*ssi;
		
		if(opt.smoothed_fraction){
		  const float bbias=0.05;
		  double pos = positives*qde->Convolved(j, bot, 1).Get(p.Y(), p.X());
		  double neg = -negatives*qde->Convolved(j, bot, 2).Get(p.Y(), p.X());
		  subscore=(pos+bbias)/(pos+neg+2*bbias);
		}
		
		if(opt.equalise_subobject_count){
		  subscore /= subcount;
		  if(debug_selective)
		    cout << "Map "<<IndexFullName(qde, j)<<":"
		      "dividing by count "<<subcount<<endl; 
		}
		
		subvals[subi] += subscore;
		if(cvals) (*cvals)[j] += subscore;
		if(store_posneg){
		  (*posnegvals)[2*j]  += qde->Convolved(j, bot, 1).Get(p.Y(), p.X());
		  (*posnegvals)[2*j+1]+= qde->Convolved(j, bot, 2).Get(p.Y(), p.X());
		  if(ssi>0)
		    (*posnegvals)[2*j] -= ssi;
		  else
		  (*posnegvals)[2*j+1] -= ssi;
		}
	      }
	      if (opt.mul)
		ShowError("CbirPicsom::PicSOMBottomScore() : cannot handle ",
			  "multiplication of subobjects");

	    } // mapidx_sub >= 0

	    
	  }
	} // if use_subobjects
	float vwas = v;
	v *= (weight*classw);
	if (debug_weights)
	  cout << "2nd stage weighting: " << weight << " * " << classw
	       << " * " << vwas << " = " << v << endl;
	}
      
      rootvals.push_back(v); // this is senseless if add==false
      if(opt.add && cvals) (*cvals)[j]+=v;

      if(stvec)
	(*stvec)[j] = v;
    }
  
    if(img && opt.storesubscores){
      img->rel_distribution.subind=sub;
      img->rel_distribution.subval=subvals;
    }
  
    if(subvals.size()==0)
      {
	if(debug_selective)
	  cout << "no subobjects, returning root score" << endl;
	ret =accumulate(rootvals.begin(),rootvals.end(),0.0); 
      }
    else{
      if(opt.maxofsubs){
	ret =accumulate(rootvals.begin(),rootvals.end(),0.0)+ 
	  *max_element(subvals.begin(),subvals.end());
      }
      else if(opt.geomsumsubs){
	for(size_t ind=0;ind<subvals.size();ind++)
	  if(subvals[ind]>0)
	    subvals[ind] *= subvals[ind];
	  else
	    subvals[ind]=0;
	ret =accumulate(rootvals.begin(),rootvals.end(),0.0)+ 
	  sqrt(accumulate(subvals.begin(),subvals.end(),0.0));
      }
      else if(opt.sublinearrank){
	sort(subvals.begin(),subvals.end());

	float subscore=0;
	for (size_t rank=1; rank<=subvals.size(); rank++)
	  subscore += subvals[subvals.size()-rank]/rank;
    
	ret = accumulate(rootvals.begin(),rootvals.end(),0.0)+subscore; 
      }
      else // default is to sum the contributions of subobjects
	ret = accumulate(subvals.begin(),subvals.end(),0.0)+
	  accumulate(rootvals.begin(),rootvals.end(),0.0);
    }

    if (opt.mul && opt.zero)
      ret=pprod;
  
    if (opt.mul && opt.rho) {
      // obs! NpositiveSeenObjects() may not return what we expect...
      double tot = qde->NseenObjects();
      double mul1= qde->NpositiveSeenObjects()/tot;
      double mul2= qde->NnegativeSeenObjects()/tot;
      ret=mul1*pprod-mul2*nprod;
    }

    if (opt.mul && !opt.zero && !opt.rho)
      ret=pprod-nprod;
  
    if(debug_selective){
      cout << "Total score returned is: " << ret << endl;
    }

    return ret;
    //if(ratioscore){
    //  double maxval=*max_element(subvals.begin(),subvals.end());
    //  double average = accumulate(subvals.begin(),subvals.end(),0.0);
    //  average /= sub.size();
    //  img.Value(rscore+maxval+average);
    //}
  }

  //===========================================================================

  void CbirPicsom::ml_calculate_h(const vector<vector<float> > &scores,
				  const double *theta, double *tgt) const {
    //cout <<"calculate_h()..."<<endl;
    
    const size_t n=scores.size();
    const size_t nmaps=scores[0].size();
    
    for(size_t i=0;i<n;i++){
      double u=theta[0];
      for(size_t k=0;k<nmaps;k++){
	u +=scores[i][k]*theta[k+1];
      }
      tgt[i]=1/(1+exp(-u));
    }    
  }
  
  ///////////////////////////////////////////////////////////////////////////

  float CbirPicsom::ml_calculate_h(const vector<float> &score,
				   const double *theta) const {
    //cout <<"calculate_h()..."<<endl;
    
    const size_t nmaps=score.size();
    
    double u=theta[0];
    for(size_t k=0;k<nmaps;k++){
      u +=score[k]*theta[k+1];
    }
    
    return 1.0/(1+exp(-u));    
  }

  //===========================================================================
  //===========================================================================
  //===========================================================================
  //===========================================================================
  //===========================================================================
  //===========================================================================

  void CbirPicsom::QueryData::ShowUnitAndObjectLists(bool seen, bool pmu,
                                                     bool pmur, bool pmi,
                                                     bool comb, bool uni,
                                                     bool unir, bool nwi,
                                                     bool nwir) const {
    if (seen)
      DumpSeen();
    
    for (size_t i=0; i<Nindices(); i++)
      for (int j=0; j<Nlevels(i); j++) {
	if (pmu && IsTsSom(i))
	  DumpVQUnits(i, j);
	if (pmur && IsTsSom(i))
	  DumpVQUnits(i, j, true);
	if (pmi && IsTsSom(i))
	  DumpNew(i, j);
      }
    
    if (comb)
      DumpCombined();
    
    if (uni)
      DumpUnique();
    
    if (unir)
      DumpUnique(true);
    
    if (nwi)
      DumpNew();
    
    if (nwir)
      DumpNew(true);
    
    cout << "random_image_count = " << random_image_count << endl;
    cout << "-------------" << endl;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::RemoveDuplicates() {
    GetQuery()->Tic("RemoveDuplicates");
    
    bool debug = false;
    int na = 0, nc = 0;
    
    DeleteUnique();
    
    map<int,set<int> > possibly_in_unique; 	
    
    for (size_t i=0; i<combinedimage.size(); i++) {
      const Object& img = combinedimage[i];
      
      //    for (j=0; j<ListSize(uniqueimage); j++)
      //      if (uniqueimage.FastGet(j)->Match(img))
      //	break;
      // if (uniqueimage.FastGet(j)->LabelsMatch(img))
      //   break;
      
      //    if (j==ListSize(uniqueimage)) {
      int j=-1;
      if(possibly_in_unique[img.Index()].size()>0){
	set<int> &s=possibly_in_unique[img.Index()];
	set<int>::iterator it;
	for(it=s.begin();it!=s.end();it++)
	  if (uniqueimage[*it].Match(img)){
	    j=*it;
	    break;	
	  }
      }
      if(j<0){	
	na++;
	uniqueimage.push_back(img);
	possibly_in_unique[img.Index()].insert(uniqueimage.size()-1);
      } else {
	nc++;
	uniqueimage[j].Combine(img, true);//was (img, false)
      }
    }
  
    if (debug)
      cout << "CbirPicsom::QueryData::RemoveDuplicates() : na=" << na << " nc=" << nc
	   << " combined images:" << combinedimage.size()
	   << " uniqueimages:" << uniqueimage.size() << endl;

    GetQuery()->Tac("RemoveDuplicates");

    return uniqueimage.size();
  }

  //===========================================================================

  bool CbirPicsom::QueryData::ValueOrNaN(float v, bool all) {
    const float nan = Index::State::nan();
    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
	TSSOM::State& fd = IndexDataTSSOM(i);
	for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
	  fd.tssom_level_weight[j] =
	    all||j==fd.tssom_level_weight.size()-1 ? v : nan;
      }
    
    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::TsSomWeightValue(bool all) {
    const float nan = Index::State::nan();
    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
	float v = Weight(i);
	TSSOM::State& fd = IndexDataTSSOM(i);
	for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
	  fd.tssom_level_weight[j] =
	    all||j==fd.tssom_level_weight.size()-1 ? v : nan;
      }

    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::TsSomWeightValueAndDistances(bool all) {
    const float nan = Index::State::nan();
    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
        float v = Weight(i);
        TSSOM::State& fd = IndexDataTSSOM(i);

        const TreeSOM& ts = MapEvenAlien(i, fd.tssom_level_weight.size()-1);

        double meandist = ts.MeanDistance();
        // double rmsdist = ts.RmsDistance();
        
        double dist = 0.0;
        for (size_t i=0; i<NseenObjects(); i++) {
          const Object& obj = seenobject[i];
          dist += ts.TrainDist(obj.Index());
        }
        dist /= NseenObjects();
        
//        double dist_weight = dist > meandist ? meandist/dist : 1.0;
        double dist_weight = meandist/(meandist+dist);
        
//        cout << endl << "USING WEIGHT: " << dist_weight << endl << endl;

       for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
         fd.tssom_level_weight[j] =
           all||j==fd.tssom_level_weight.size()-1 ? v*dist_weight : nan;
      }

    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::EntropyNpolynomial(const string& arg) {
    const string defa = "0.33333333,1.0";
    const string& a = arg!="" ? arg : defa;

    vector<string> val = SplitInCommas(a);
    if (val.size()!=2)
      return ShowError("CbirPicsom::QueryData::EntropyNpolynomial(", a, ") failed");
  
    double min = atof(val[0].c_str()), ex = atof(val[1].c_str());
    const float nan = Index::State::nan();

    if (debug_weights)
      cout << "EntropyNpolynomial(" << a << ") -> " << min << " " << ex << endl;

    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
	TSSOM::State& fd = IndexDataTSSOM(i);
	for (size_t j=0; j<fd.tssom_level_weight.size(); j++) {
	  fd.tssom_level_weight[j] = fd.entropy_n[j]==nan ? nan :
	    pow(1+(min-1)*fd.entropy_n[j], ex);
	  if (debug_weights)
	    cout << "  " << MapName(false, i, j) << " : " << ToStr(fd.entropy_n[j])
		 << " -> " << ToStr(fd.tssom_level_weight[j]) << endl;
	}
      }

    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::EntropyNleaveOutWorst(const string&) {
    typedef multimap<float,int> order_t;
    order_t order;

    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i)) {
	TSSOM::State& fd = IndexDataTSSOM(i);
	if (fd.entropy_n.size())
	  order.insert(pair<float,int>(fd.entropy_n.back(), i));
      }

    int worst = -1;

    if (order.size()>1) {
      while (order.size()>2)
	order.erase(order.begin());
      if (order.begin()->first!=order.rbegin()->first)
	worst = order.rbegin()->second;
    }

    const float nan = Index::State::nan();
    for (int i=0; i<(int)Nindices(); i++)
      if (IsTsSom(i)) {
	TSSOM::State& fd = IndexDataTSSOM(i);
	for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
	  if (fd.entropy_n[j]==nan)
	    fd.tssom_level_weight[j] = nan;
	  else
	    fd.tssom_level_weight[j] = i==worst ? 0.0 : 1.0;
      }

    return true;
  }

  //===========================================================================

  void CbirPicsom::QueryData::SetMapWeightsDebugInfo() {
    cout << "weights: ";
    for (size_t i=0; i<Nindices(); i++) {
      cout << IndexFullName(i) << "=[";
      const TSSOM::State& fd = IndexDataTSSOM(i);
      for (size_t j=0; j<fd.tssom_level_weight.size(); j++)
	if (fd.tssom_level_weight[j]==Index::State::nan())
	  cout << (j?" ":"") << "NA";
	else
	  cout << (j?" ":"") << fd.tssom_level_weight[j];
      cout << "] ";
    }
    cout << endl;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::PlaceSeenOnMap(int ii, int jj, bool norm, bool limit) {

    if (!IsTsSomAndLevelOK(ii, jj))
      return ShowError("CbirPicsom::QueryData::PlaceSeenOnMap(): not TSSOM or invalid level");
    
    TsSom(ii).ReadDivisionFile();

    GetQuery()->Tic("PlaceSeenOnMap");
    bool ret = false;
    if (GetMatrixCount()==1)
      ret = PlaceSeenOnMapOneMap(ii, jj, norm);
    else if (GetMatrixCount()==3)
      ret = PlaceSeenOnMapThreeMaps(ii, jj, norm, limit);
    else {
      ShowError("CbirPicsom::QueryData::PlaceSeenOnMapPicSOM(): matrixcount!=1|3");
      ret = false;
    }
    GetQuery()->Tac("PlaceSeenOnMap");

    return ret;
  }

  //===========================================================================

  const vector<float>& CbirPicsom::QueryData::DivMultiplierVector(int /*m*/, int /*l*/) {
    static vector<float> v1, v3, v10;
    if (!v1.size())
      v1.push_back(1.0);
      
    if (!v3.size()) {
      v3.push_back(1.0);
      v3.push_back(0.75);
      v3.push_back(0.25);
    }

    if (!v10.size()) {
      v10.push_back(1.0);
      v10.push_back(0.9);
      v10.push_back(0.8);
      v10.push_back(0.7);
      v10.push_back(0.6);
      v10.push_back(0.5);
      v10.push_back(0.4);
      v10.push_back(0.3);
      v10.push_back(0.2);
      v10.push_back(0.1);
    }
  
    return v1;

    // return DivisionDepth(m, l)>=10 ? v10 : v1;
  }
    
  //===========================================================================

  const vector<float>& CbirPicsom::QueryData::DivHistoryVector(int /*m*/, int /*l*/) {
    static vector<float> v1, v5;
    if (!v1.size())
      v1.push_back(1.0);

    if (!v5.size()) {
      v5.push_back(1.0);
      v5.push_back(0.8);
      v5.push_back(0.6);
      v5.push_back(0.4);
      v5.push_back(0.2);
    }
  
    return v5;
  }
    
  //===========================================================================

  bool CbirPicsom::QueryData::BernoulliMAP(int ii, int jj) {
    MapSurfaceMAPOpt opt;
    
    char msgtmp[1000];
    sprintf(msgtmp, "BernoulliMAP(%s, %d)", TsSom(ii).Name().c_str(), jj);
    // Tic(msgtmp);
    //cout << msgtmp << endl;
    
    const simple::FloatMatrix& pos = Hits(ii, jj,1);
    const simple::FloatMatrix& neg = Hits(ii, jj,2);
    
    simple::FloatMatrix& cnv = Convolved(ii, jj, 0);
    cnv.SizeOrZero(pos);
    
    const int r = pos.Rows(), c = pos.Columns();
    
    opt.w=c;
    opt.h=r;
    
    opt.ConstructInverseCovariance();
    
    
    int *posdata=new int[r*c];
    int *negdata=new int[r*c];
    
    double positives = 0, negatives = 0;
    SumPositivesAndNegatives(Division(ii, jj), IndexData(ii),
			     positives, negatives, true);
    
    int x, y;
    
    for(y=0;y<r;y++) 
      for (x=0; x<c; x++){
	int ind=x+c*y;
	posdata[ind]=(int)(pos.FastGet(y,x)*positives+0.5);
	negdata[ind]=(int)(neg.FastGet(y,x)*negatives+0.5);
      }

    opt.SetHitCounts(posdata,negdata);

    // construct initial guess
    vector<double> theta(r*c,0.0);

    theta=opt.Optimise(theta);
    opt.ParamToProb(theta);

    for(y=0;y<r;y++) 
      for (x=0; x<c; x++){
	int ind=x+y*c;
	cnv.FastSet(y, x, theta[ind]);
      }

    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::PlaceSeenOnMapOneMap(int ii, int jj, bool norm) {
    bool use_aspect_weights = true;

    string msg = "CbirPicsom::QueryData::PlaceSeenOnMapOneMap() : ";

    if (GetMatrixCount()!=1)
      return ShowError(msg+"matrixcount!=1");

    if (debug_placeseen)
      cout << "PlaceSeenOnMapOneMap(" << ii << "," << jj << "," << norm
	   << ") : " << MapName(false, ii, jj)
	   << " aspects=[" << IndexAspects(ii) << "]";

    const TreeSOM& ts = MapEvenAlien(ii, jj);
    simple::FloatMatrix&  hts = Hits(ii, jj, 0);
    SetSizeOrZero(hts, ts);

    bool si=store_selfinfluence && Map(ii, jj).IsBottomLevel();

    double positives = 0, negatives = 0, pos_sum = 0, neg_sum = 0;         
    double res_sum_neg = 0, res_sum_pos = 0;
    if (norm)
      SumPositivesAndNegatives(Division(ii, jj), IndexData(ii),
			       positives, negatives, true);
    else
      positives = negatives = 1;

    if (debug_placeseen)
      cout << " total=" << NseenObjects() << " positives=" << positives
	   << " negatives=" << negatives << endl;

    const vector<float>& divmul = DivMultiplierVector(ii, jj);

    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& obj = seenobject[i];
      
      int idx = obj.Index();
      if (idx==-1) {
	ShowError(msg+"index not solved for ", obj.Label());
	continue;
      }
    
      double pos = 0.0, neg = 0.0;
      bool match = AspectsMatch(IndexData(ii), obj, pos, neg);
      double objval = obj.Value();
      double sumval = pos+neg;

      if (debug_placeseen)
	cout << "  i=" << i << " " << GetDataBase()->ObjectDump(idx)
	     << " aspects=[" << obj.AspectNamesValues()
	     << "] match=" << match << " objval=" << objval
	     << " pos=" << pos << " neg=" << neg
	     << " sumval=" << sumval << endl;

      if (!match)
	continue;

      for (size_t d=0; d<divmul.size(); d++) {
	const IntVector& divvec = Division(ii, jj, d);
	double mul = divmul[d];
	int mapidx = divvec.IndexOK(idx) ? divvec[idx] : -1;

	if (debug_placeseen)
	  cout << "    d=" << d << " mul=" << mul << " mapidx=" << mapidx;

	if (mapidx!=-1) {
	  IntPoint p = ts.ToPoint(mapidx);

	  if (debug_placeseen)
	    cout << " (" << p.Y() << "," << p.X() << ")";

	  double val = 0.0;

	  if (pos>0) {
	    if (!positives)
	      return ShowError(msg+"division by zero: positives==0");

	    double pval = pos/positives;
	    if (use_aspect_weights)
	      pval *= pos;

	    val += pval;
	    if (!d)
	      pos_sum += pos;
	    res_sum_pos += pval;
	  }

	  if (neg<0) {
	    if (!negatives)
	      return ShowError(msg+"division by zero: negatives==0");
	    
	    double nval = neg/negatives*negative_weight;
	    if (use_aspect_weights)
	      nval *= -neg;

	    val += nval;
	    if (!d)
	      neg_sum -= neg;
	    res_sum_neg += nval;
	  }

	  double res = mul*val;
	  hts.Add(p.Y(), p.X(), res);

	  if (debug_placeseen)
	    cout << " res=" << res << " => " << hts.Get(p.Y(), p.X());

	  if(si){
	    if(d>0) continue; // currently only the first influence is taken
	                      // into account

	    if(selfinfluence.count(idx)==0)
	      selfinfluence[idx]=vector<float>(Nindices(),0.0);
	    selfinfluence[idx][ii]=+res;
	  }
	}

	if (debug_placeseen)
	  cout << endl;
      }
    }

    if (norm && (positives!=pos_sum || negatives!=neg_sum)) {
      stringstream ss;
      ss << "positives="  << positives << " pos_sum=" << pos_sum
	 << " negatives=" << negatives << " neg_sum=" << neg_sum;
      return ShowError(msg+"check sum pos_sum or neg_sum mismatches: ",
		       ss.str());
    }

    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::PlaceSeenOnMapThreeMaps(int ii, int jj, bool norm,
					    bool limit) {
    bool use_aspect_weights = true;

    string msg = "CbirPicsom::QueryData::PlaceSeenOnMapThreeMaps() : ";

    if (GetMatrixCount()!=3)
      return ShowError(msg+"matrixcount!=3");

    if (debug_placeseen)
      cout << "PlaceSeenOnMapThreeMaps(" << ii << "," << jj << ","
	   << norm << "," << limit << ") : " << MapName(false, ii, jj) 
	   << " aspects=[" << IndexAspects(ii) << "]";

    const TreeSOM& ts = Map(ii, jj);
    simple::FloatMatrix&  hts_pos = Hits(ii, jj, 1);
    simple::FloatMatrix&  hts_neg = Hits(ii, jj, 2);
    SetSizeOrZero(hts_pos, ts);
    SetSizeOrZero(hts_neg, ts);

    bool si=store_selfinfluence && Map(ii, jj).IsBottomLevel();

    const IntVector& div = Division(ii, jj);

    double positives = 0, negatives = 0, pos_sum = 0, neg_sum = 0;
    double res_sum_neg = 0, res_sum_pos = 0;
    if (norm)
      SumPositivesAndNegatives(div, IndexData(ii), positives,negatives,true);
    else
      positives = negatives = 1;

    if (debug_placeseen)
      cout << " total=" << NseenObjects() << " positives=" << positives
	   << " negatives=" << negatives << endl;

    if (debug_selective)
      cout << "Normalising w/ positives=" << positives
	   << " negatives=" << negatives << endl;

    const vector<float>& divmul = DivMultiplierVector(ii, jj);

    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& obj = seenobject[i];

      int idx = obj.Index();
      if (idx==-1) {
	ShowError(msg+"index not solved for ", obj.Label());
	continue;
      }

      double pos = 0.0, neg = 0.0;
      bool match  = AspectsMatch(IndexData(ii), obj, pos, neg);
      double objval = obj.Value();
      double sumval = pos+neg;
   
      if (debug_placeseen)
	cout << "  i=" << i << " " << GetDataBase()->ObjectDump(idx)
	     << " aspects=[" << obj.AspectNamesValues()
	     << "] match=" << match << " objval=" << objval
	     << " pos=" << pos << " neg=" << neg
	     << " sumval=" << sumval << endl;

      if (!match)
	continue;

      for (size_t d=0; d<divmul.size(); d++) {
	const IntVector& divvec = Division(ii, jj, d);
	double mul = divmul[d];
	int mapidx = divvec.IndexOK(idx) ? divvec[idx] : -1;

	if (debug_placeseen)
	  cout << "    d=" << d << " mul=" << mul << " mapidx=" << mapidx;

	double res = 0.0;

	if (mapidx!=-1) {
	  IntPoint p = ts.ToPoint(mapidx);

	  if (debug_placeseen)
	    cout << " (" << p.Y() << "," << p.X() << ")";

	  if (pos>0) {
	    if (!positives)
	      return ShowError(msg+"division by zero: positives==0");
	   
	    double pval = mul*pos/positives;
	    if (use_aspect_weights)
	      pval *= pos;

	    hts_pos.Add(p.Y(), p.X(), pval);
	    if (!d)
	      pos_sum += pos;
	    res_sum_pos += pval;
	    res += pval;
	    
	    if (debug_placeseen)
	      cout << " pval=" << pval << " => " << hts_pos.Get(p.Y(), p.X());
	  }

	  if (neg<0) {
	    if (!negatives)
	      return ShowError(msg+"division by zero: negatives==0");

	    double nval = mul*neg/negatives*negative_weight;
	    if (use_aspect_weights)
	      nval *= -neg;

	    hts_neg.Add(p.Y(), p.X(), nval);
	    if (!d)
	      neg_sum -= neg;
	    res_sum_neg += nval;
	    res += nval;

	    if (debug_placeseen)
	      cout << " nval=" << nval << " => " << hts_neg.Get(p.Y(), p.X());;
	  }

	  if(si){
	    if(d>0) continue; // currently only the first influence is taken
	                      // into account

	    if(selfinfluence.count(idx)==0)
	      selfinfluence[idx]=vector<float>(Nindices(),0.0);
	    selfinfluence[idx][ii]=+res;
	  }

	  if (debug_placeseen)
	    cout << endl;
	}
      }
    }

    if (debug_placeseen)
      cout << "PlaceSeenOnMapThreeMaps(" << ii << "," << jj
	   << ",...) ending: res_sum_pos=" << res_sum_pos
	   << " res_sum_neg=" << res_sum_neg << endl;

    if (norm && (positives!=pos_sum || negatives!=neg_sum)) {
      stringstream ss;
      ss << "positives="  << positives << " pos_sum=" << pos_sum
	 << " negatives=" << negatives << " neg_sum=" << neg_sum;
      return ShowError(msg+"check sum pos_sum or neg_sum mismatches: ",
		       ss.str());
    }
  
    if (limit) {
      cout << "PlaceSeenOnMapThreeMaps() limiting relevance" << endl;

      int r=hts_pos.Rows();
      int c=hts_pos.Columns();

      double discount_pos=0.0, discount_neg=0.0;

      for(int x=0;x<c;x++) for(int y=0;y<r;y++)
	{
	  double before;
	
	  before=hts_pos(y,x);
	  if(before && before > 1.0/positives){
	    double newval= (2-2*pow(0.5,before*positives))/positives;
	    discount_pos += before-newval;
	    hts_pos.Set(y,x,newval);
	  }

	  before=hts_neg(y,x);
	  if(before && before > 1.0/negatives){
	    double newval= (2-2*pow(0.5,before*negatives))/negatives;
	    discount_neg += before-newval;
	    hts_neg.Set(y,x,newval);
	  }

	}

      // here we could have renormalisation based on 
      // discount_[pos,neg]
    }

    if (hitstype==smoothed_log) {
      int r=hts_pos.Rows();
      int c=hts_pos.Columns();

      double bias=0.05;

      for(int x=0;x<c;x++) for(int y=0;y<r;y++)
	{
	  double before;
	
	  before=hts_pos(y,x)*positives+bias;
	  hts_pos.Set(y,x,log(before));

	  before=-hts_neg(y,x)*negatives+bias;
	  hts_neg.Set(y,x,-log(before));
	}
    }
    else if (hitstype==posneg_lc) {
      int r=hts_pos.Rows();
      int c=hts_pos.Columns();

      for(int x=0;x<c;x++) for(int y=0;y<r;y++)
	{
	  double before;
	  float coeff=hit_comb_factor;
     
	  before=hts_pos(y,x)*positives*coeff;
	  hts_pos.Set(y,x,before);

	  before=-hts_neg(y,x)*negatives;
	  hts_neg.Set(y,x,-before);
	}
    }

    if(hitstype==smoothed_fraction)
      return PlaceSeenOnMapOneMapSmoothedFraction(ii,jj);
    else{
      // cout << "Summing hit matrices" << endl;
      Hits(ii, jj, 0) = hts_pos;
      Hits(ii, jj, 0)+= hts_neg;
    }
    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::PlaceSeenOnMapOneMapSmoothedFraction(int ii, int jj) {
    string msg = "CbirPicsom::QueryData::PlaceSeenOnMapOneMapSmoothedFraction() : ";
    if (GetMatrixCount()<1)
      return ShowError(msg+"matrixcount<1");

    cout << "entering " << msg << endl;

    if (UsingAspects())
      return ShowError(msg+"incompatible with aspects");

    const TreeSOM& ts = MapEvenAlien(ii, jj);
    simple::FloatMatrix&  hts = Hits(ii, jj, 0);
    SetSizeOrZero(hts, ts);

    const int r = hts.Rows(), c = hts.Columns();
    float *tmppos,*tmpneg;

    tmppos=new float[r*c];
    tmpneg=new float[r*c];

    for(int i=0;i<r*c;i++)
      tmppos[i]=tmpneg[i]=0;

    const vector<float>& divmul = DivMultiplierVector(ii, jj);

    for (size_t i=0; i<seenobject.size(); i++) {
      int idx = seenobject[i].Index();
      if (idx==-1) {
	ShowError(msg+"index not solved for ", seenobject[i].Label());
	continue;
      }
    
      for (size_t d=0; d<divmul.size(); d++) {
	const IntVector& divvec = Division(ii, jj, d);
	double mul = divmul[d];
	int mapidx = divvec.IndexOK(idx) ? divvec[idx] : -1;
	
	if (mapidx!=-1) {
	  IntPoint p = ts.ToPoint(mapidx);
	  double val = seenobject[i].Value();            
	  if(val>0)
	    tmppos[p.X()+c*p.Y()] += mul;
	  else
	    tmpneg[p.X()+c*p.Y()] += mul;
	}
      }
    }
    
    bool si=store_selfinfluence && Map(ii, jj).IsBottomLevel();

    double bias=0.1;

    for(int x=0;x<c;x++) for(int y=0;y<r;y++){
      int idx=x+y*c;
      float val=(tmppos[idx]+bias)/(tmppos[idx]+tmpneg[idx]+2*bias);
      hts.FastSet(y,x,val);
      if(si){
	if(selfinfluence.count(idx)==0)
	  selfinfluence[idx]=vector<float>(Nindices(),0.0);
	selfinfluence[idx][ii]=val;
      }
    }
     return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::CountPositivesAndNegatives(const IntVector& div,
					 int& pos, int& neg,
					 bool nonzero,
					 bool divcheck) const {
    pos = neg = 0;

    bool ok = true;
    for (size_t i=0; i<NseenObjects(); i++)
      if (divcheck==false ||
	  (div.IndexOK(seenobject[i].Index()) && div[seenobject[i].Index()]!=-1)) {
	float val = seenobject[i].Value();

	if (val!=-1 && val!=1 && val!=0)
	  ok = ShowError("CbirPicsom::QueryData::CountPositivesAndNegatives() : val!=+/-1/0");
	if (val>0)
	  pos++;    
	else if (val<0)
	  neg++;                   
      }

    if (nonzero) {
      if (pos == 0) pos = 1;    
      if (neg == 0) neg = 1;    
    }

    return ok;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::SumPositivesAndNegatives(const IntVector& div,
				       const Index::State& fd,
				       double& pos, double& neg,
				       bool divcheck) const {

    bool use_val = false;  // used to be like true until 14.8.2006

    pos = neg = 0;
    
    bool ok = true;
    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& obj = seenobject[i];
      int idx = obj.Index();
      if (divcheck==false || (div.IndexOK(idx) && div[idx]!=-1)) {
	double val = obj.Value(), mul = 0.0, pp = 0.0, nn = 0.0;
	if (!AspectsMatch(fd, obj, pp, nn))
	  continue;

	if (use_val) {
	  if (val>0)
	    pos += val*mul;
	  else if (val<0)
	    neg -= val*mul;
	} else {
	  pos += pp;
	  neg -= nn;
	}
      }
    }

    return ok;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::AspectsMatch(const Index::State& fd, const Object& o,
					   double& pos, double& neg) const {
    bool debug = false;

    const vector<string>&          fda = fd.aspects;
    const map<string,aspect_data>& oba = o.Aspects();

    bool fda_star = fda.size()==1 && fda[0]=="*";
    set<string> fds;
    if (!fda_star)
      fds.insert(fda.begin(), fda.end());
    
    pos = neg = 0.0;

    for (map<string,aspect_data>::const_iterator j = oba.begin();
	 j!=oba.end(); j++) {
      if (debug)
	cout << "AspectsMatch() : " << o.DumpStr() << " \"" << j->first
	     << "\"=" << j->second.value;
      if (j->second.value!=0.0 &&
	  (fda_star || fds.find(j->first)!=fds.end())) {
	if (j->second.value>0)
	  pos += j->second.value;
	else
	  neg += j->second.value;
	if (debug)
	  cout << " pos=" << pos << " neg=" << neg;
      }
      if (debug)
	cout << endl;
    }

    return pos!=0.0 || neg!=0.0;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::UsingAspects() const {
    if (positive.size()>1 ||
	(positive.size()==1 && positive.begin()->first!=""))
      return true;
    
    if (negative.size()>1 ||
	(negative.size()==1 && negative.begin()->first!=""))
      return true;
    
    for (size_t i=0; i<Nindices(); i++)
      if (IndexData(i).aspects.empty() || IndexData(i).aspects[0]!="*")
	return true;
    return false;
  }

  //===========================================================================

  const FloatVector *CbirPicsom::QueryData::ConvolutionVector(const string& str, int l) {
    static FloatVectorSet vset;
    
    float len, par;
    string t, vname = ConvolutionVectorName(str, l, t, len, par);
    if (!vname.length())
      return NULL;
    
    stringstream ss;
    ss << vname << " [" << t << " " << len << " " << par << "]";
    
#ifdef PICSOM_USE_PTHREADS
#ifdef PTHREAD_RWLOCK_INITIALIZER
    static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
#else
    static bool first=true;
    static pthread_rwlock_t rwlock;
    if (first) 
      pthread_rwlock_init(&rwlock, NULL);
    first=false;
#endif // PTHREAD_RWLOCK_INITIALIZER
    pthread_rwlock_rdlock(&rwlock);
#endif // PICSOM_USE_PTHREADS
    
    FloatVector *vec = vset.GetByLabel(vname.c_str());
    
#ifdef PICSOM_USE_PTHREADS
    pthread_rwlock_unlock(&rwlock);
#endif // PICSOM_USE_PTHREADS
    
    if (vec) {
      // cout << "ConvolutionVector() reused " << ss.str() << endl;
      goto destroy;
    }
    
#ifdef PICSOM_USE_PTHREADS
    pthread_rwlock_wrlock(&rwlock);
#endif // PICSOM_USE_PTHREADS
    
    vec = vset.GetByLabel(vname.c_str());
    if (!vec) {
      vec = CreateConvolutionVector(t, len, par);
      if (vec) {
	vec->Label(vname);
	vset.Append(vec);
	ss << " len=" << vec->Length() << (vec->Length() ? " :" : "");
	for (int i=0; i<vec->Length() && i<3; i++)
	  ss << " " << vec->Get(i) << (vec->Length()>i+1 ? "," : "");
	if (vec->Length()>3)
	  ss << " ...";
	GetQuery()->WriteLog("Created convolution vector ", ss.str());
      }
    }
    
#ifdef PICSOM_USE_PTHREADS
    pthread_rwlock_unlock(&rwlock);
  destroy:
    pthread_rwlock_destroy(&rwlock);
#else 
  destroy:
#endif // PICSOM_USE_PTHREADS
    
    return vec;
  }
  
  //===========================================================================

  bool CbirPicsom::QueryData::ConvolutionVectorNameAndFunc(const string& str, string& t,
					   ConvolutionFunction& f
					   ) {
    static const struct conv_name_and_func_entry {
      /*const*/ string name;
      ConvolutionFunction func;
    } conv_name_and_func[] = {
      { "point",       PointVectorValue  },
      { "triangle",    TriangleVectorValue  },
      { "gaussian",    GaussianVectorValue  },
      { "dog",         DoGVectorValue },
      { "rectangle",   RectangleVectorValue },
      { "exponential", ExponentialVectorValue },
      //{ "cosine",      CosineVectorValue },
      { "hann",        HannVectorValue },
      { "", NULL }
    };
    for (const conv_name_and_func_entry *e = conv_name_and_func; e->func; e++)
      if (e->name == str.substr(0, e->name.length())) {
	t = e->name;
	f = e->func;
	return true;
      }
    
    t = "";
    f = NULL;
    return false;
  }

  //===========================================================================

  string CbirPicsom::QueryData::ConvolutionVectorName(const string& str, int l,
				      string& t, float& len, float& par) {
    ConvolutionFunction f;
    
    if (!ConvolutionVectorNameAndFunc(str, t, f))
      return "";
    
    const char *ps = str.length()>t.length() ? str.c_str()+t.length()+1 : NULL;
    
    string param = ConvolutionVectorParam(ps, l, len, par);
    
    return param.length() ? (t + "-" + param) : t;
  }
  
  //===========================================================================
  
  ConvolutionFunction CbirPicsom::QueryData::ConvolutionVectorFunc(const string& str,
						   int l, float& len,
						   float& par) {
    ConvolutionFunction f;
    
    string t;
    if (!ConvolutionVectorNameAndFunc(str, t, f))
      return NULL;
    
    const char *ps = str.length()>t.length() ? str.c_str()+t.length()+1 : NULL;
    
    ConvolutionVectorParam(ps, l, len, par);
    
    return f;
  }

  //===========================================================================

  string CbirPicsom::QueryData::ConvolutionVectorParam(const char *ptr, int l,
				       float& len, float& par) {
    if (ptr) 
      while (l--) {
	const char *dash = strchr(ptr, '-');
	if (dash)
	  ptr = dash+1;
	else
	  break;
      }
    
    len = 1;
    par = 0;
    
    string param = ptr ? string(ptr, strcspn(ptr, "-")) : "";
    float p, q;
    int n = sscanf(param.c_str(), "%f:%f", &p, &q);
    if (n==1)
      len = p;
    else if (n==2) {
      len = q;
      par = p;
    }
    
    return param;
  }

  //===========================================================================
  
  FloatVector *CbirPicsom::QueryData::CreateConvolutionVector(const string& str,
					      double len, double par) {
    string t;
    ConvolutionFunction f;
    if (!ConvolutionVectorNameAndFunc(str, t, f))
      return NULL;
    
    FloatVector *vec = new FloatVector(1000);
    for (int i=0; i<vec->Length(); i++) {
      double val = (*f)(i, len, par);
      if (val)
	vec->Set(i, val);
      else {
	vec->Lengthen(i);
	break;
      }
    }
    
    return vec;
  }

  //===========================================================================

  double CbirPicsom::QueryData::PointVectorValue(double i, double, double) {
    return i==0 ? 1 : 0;
  }

  //===========================================================================
  
  double CbirPicsom::QueryData::TriangleVectorValue(double i, double len, double) {
    return i>len ? 0 : (len-i)/len;
  }
  
  //===========================================================================
  
  double CbirPicsom::QueryData::GaussianVectorValue(double i, double len, double par) {
    if (i>len)
      return 0;
    
    static const double m = sqrt(::log(2.0));
    double v = i*m/par;
    
    return exp(-v*v);
  }

  //===========================================================================
  
  double CbirPicsom::QueryData::DoGVectorValue(double i, double len, double par) {
    if (i>len)
      return 0;
    
    static const double m = sqrt(2.0/::log(2.0));
    
    return GaussianVectorValue(i,len,m*par)-GaussianVectorValue(i,len,m*1.6*par)/1.6;
  }
  
  //===========================================================================
  
  double CbirPicsom::QueryData::RectangleVectorValue(double i, double len, double) {
    return i>len ? 0 : 1;
  }
  
  //===========================================================================
  
  double CbirPicsom::QueryData::ExponentialVectorValue(double i, double len, double par) {
    return i>len ? 0 : exp(-i*::log(2.0)/par);
  }
  
  //===========================================================================
  
  double CbirPicsom::QueryData::HannVectorValue(double i, double len, double) {
    return i>len ? 0 : 0.5*(1+cos(6.28322*i/(2*(len)+1)));
  }
  
  //===========================================================================

  bool CbirPicsom::QueryData::Convolve() {
    GetQuery()->Tic("Convolve");

    bool ok = true;

    for (size_t i=0; i<Nindices(); i++)
      if (IsTsSom(i))
	for (size_t j=0; j<TsSom(i).Nlevels(); j++)
	  for (size_t k=0; k<GetMatrixCount(i); k++)
	    ok = ok && Convolve(i, j, k);

    GetQuery()->Tac("Convolve");

    return ok;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::Convolve(int i, int j, int k) {
    const string& ct = ConvType(i);

    if (ct.empty() || ct=="one-dim-mask")
      return ConvolveOneDimMask(i, j, k, TsSom(i).CreateCVector(j));

    if (ct.substr(0, 8)=="umatrix-")
      return ConvolveUmatrix(ct.substr(8), i, j, k);
    
    if (ct.substr(0,8)=="nbrrank-")
      return ConvolveNbrRank(ct.substr(8),i, j, k);

    if(ct.substr(0,10)=="ratioconv-")
      return ConvolveRatio(ct.substr(10),i,j,k);

    const FloatVector *cvec = ConvolutionVector(ct, j);
    if (!cvec)
      return ShowError("CbirPicsom::QueryData::Convolve() : convtype==[", ct,
		       "] unknown or failed");

    return ConvolveOneDimMask(i, j, k, *cvec);
  }

  //===========================================================================

  bool CbirPicsom::QueryData::ConvolveOneDimMask(int ii, int jj, int kk,
						 const FloatVector& v) {
    if (!v)
      return false;

    char msgtmp[1000];
    sprintf(msgtmp, "ConvolveOneDimMask(%s, %d, %d)", TsSom(ii).Name().c_str(), jj, kk);

    // Tic(msgtmp);

    const simple::FloatMatrix& hits = Hits(ii, jj, kk);
    simple::FloatMatrix& cnv = Convolved(ii, jj, kk);
    cnv.SizeOrZero(hits);

    const int r = hits.Rows(), c = hits.Columns(), l = v.Length();
    register int x, y;

    simple::FloatMatrix tmp(r, c);

    const TreeSOM& map = Map(ii, jj); // was EvenAlien

    bool store_si=store_selfinfluence && map.IsBottomLevel();
    if (store_si) {
      vector<float> colvec(r,0.0);
      location_selfinfluence[ii]=vector<vector<float> >(c,colvec);
    }

    double masksum = 2*v.Sum()-v.Get(0);

    bool circular = (TsSom(ii).Map(jj).Topology()==simple::Planar::Torus);
    if (circular==false) {
      bool non_zero_found = false;
      for (x=0; x<c; x++) {
	const int imax = l<c-x ? l : c-x, imin = l<=x ? l : x+1;
	for (y=0; y<r; y++) {
	  register double sum = 0, div = 0;
	  register int i;

	  for (i=0; i<imax; i++) {
	    register double t = v.FastGet(i);
	    sum += t*hits.FastGet(y, x+i); 
	    div += t;
	  }
	  for (i=1; i<imin; i++) {
	    register double t = v.FastGet(i);
	    sum += t*hits.FastGet(y, x-i); 
	    div += t;
	  }
	  if (bordercompensation==false)
	    div = masksum;
	  tmp.FastSet(y, x, sum/div);
	  if (store_si)
	    location_selfinfluence[ii][x][y]=1/div; 
	  if (sum!=0.0)
	    non_zero_found = true;
	}
      }
      if (!non_zero_found && !zero_matrices_allowed)
	ShowError("only zero values found in "+string(msgtmp));

    } else { // circular
      for (x=0; x<c; x++) {
	for (y=0; y<r; y++) {
	  register double sum = 0, div = 0;
	  register int i;
	  for (i=0; i<l; i++) {
	    register double t = v.FastGet(i);
	    sum += t*hits.FastGet(y, (x+i)%c); 
	    div += t;
	  }
	  for (i=1; i<l; i++) {
	    register double t = v.FastGet(i);
	    sum += t*hits.FastGet(y, (x-i+c)%c); 
	    div += t;
	  }
	  tmp.FastSet(y, x, sum/div);
	  if (store_si)
	    location_selfinfluence[ii][x][y]=1/div; 
	}
      }
    }

    if (circular==false) {
      for (y=0; y<r; y++) {
	const int imax = l<r-y ? l : r-y, imin = l<=y ? l : y+1;
	for (x=0; x<c; x++) {
	  register double sum = 0, div = 0;
	  register int i;
	
	  for (i=0; i<imax; i++) {
	    register double t = v.FastGet(i);
	    sum += t*tmp.FastGet(y+i, x); 
	    div += t;
	  }
	  for (i=1; i<imin; i++) {
	    register double t = v.FastGet(i);
	    sum += t*tmp.FastGet(y-i, x); 
	    div += t;
	  }
	  if (bordercompensation==false)
	    div = masksum;
	  cnv.FastSet(y, x, sum/div);
	  if (store_si)
	    location_selfinfluence[ii][x][y]*=1/div; 
	}
      }

    } else { // circular
      for (y=0; y<r; y++) {
	for (x=0; x<c; x++) {
	  register double sum = 0, div = 0;
	  register int i;
	
	  for (i=0; i<l; i++) {
	    register double t = v.FastGet(i);
	    sum += t*tmp.FastGet((y+i)%r, x); 
	    div += t;
	  }
	  for (i=1; i<l; i++) {
	    register double t = v.FastGet(i);
	    sum += t*tmp.FastGet((y-i+r)%r, x); 
	    div += t;
	  }
	  cnv.FastSet(y, x, sum/div);
	  if(store_si)
	    location_selfinfluence[ii][x][y]*=1/div; 
	}
      }
    } // circular 
    // Tac(msgtmp);

    return true;
  }

  //===========================================================================

  double CbirPicsom::MaximumEntropy(const simple::FloatMatrix& m) {
    simple::FloatMatrix em(m.Rows(), m.Columns());
    em.Add(1);
    return em.Entropy();
  }

  //===========================================================================

  double CbirPicsom::ConvolutionEntropy(const FloatVector *v) {
    if (!v || !v->Length())
      return -1;

    if (v->Length()==1)
      return 0;

    double div = 2*v->Sum()-v->Get(0);
    double sum = 0;
    for (int i=0; i<v->Length(); i++) {
      double p = v->Get(i)/div;
      sum += (i?2:1)*p*::log(p);
    }

    return -2*sum/::log(2.0);
  }

  //===========================================================================

  float CbirPicsom::QueryData::ClassWeight(int idx) const {
    if (classweight.empty())
      return 1;
    
    for (list<pair<ground_truth,float> >::const_iterator i=classweight.begin();
	 i!=classweight.end(); i++)
      if (i->first[idx]==1)
	return i->second;
    
    return 0;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::MapSnapshots(int i, int j, const string &xs) const {
    bool do_hits = true, do_conv = true;

    string n;

    for (size_t k=0; k<GetMatrixCount(i); k++) {
           bool scale_to_red = k>0 ? false: true;
      // bool scale_to_red = false;

      stringstream ss;
      ss << GetQuery()->Identity() << "_"
	 << IndexShortName(i) << "_" << j;
      if (GetMatrixCount(i)!=1)
	ss << "_" << k;
      ss << xs;

      string hname = ss.str(), cname = hname+"_conv", ext = ".png";
      
      if (mapimagename!="") {
	if (do_hits) {
	  n =  mapimagename+hname+ext;
	  string argstr;
	  // if(scale_to_red)
	  //  argstr="redonly";
	  imagedata hdata = GetQuery()->CreateMapImage(i, j, k, argstr);
	  
	  imagefile::write(hdata, n);
	  
	  GetQuery()->WriteLog("Wrote map hit image <", n, ">");
	}
	if (do_conv) {
	  n = mapimagename+cname+ext;
	  string argstr="convolved";

	  if(scale_to_red)
	    argstr+=",redonly";
	  imagedata cdata = GetQuery()->CreateMapImage(i, j, k, argstr);
	  imagefile::write(cdata, n);
	  GetQuery()->WriteLog("Wrote map convolution image <", n, ">");
	}
      }

      if (mapmatlabname!="" || mapdatname!="") {
	float *ch = Hits(i, j, k).TransposeConcatenate();
	FloatVector vh(Hits(i, j, k).Size(), ch);
	delete ch;
	float *cc = Convolved(i, j, k).TransposeConcatenate();
	FloatVector vc(Convolved(i, j, k).Size(), cc);
	delete cc;
      
	if (mapmatlabname!="") {
	  if (do_hits) {
	    n = Simple::MakeMatlabCompliant(mapmatlabname+hname);
	    vh.Label(n).WriteMatlab(n+".m");
	    GetQuery()->WriteLog("Wrote map hit matlab vector file <", n, ".m>");
	    n += "_m";
	    simple::FloatMatrix mm = Hits(i, j, k);
	    mm.Label(n).WriteMatlab(n+".m");
	    GetQuery()->WriteLog("Wrote map hit matlab matrix file <", n, ".m>");
	  }
	  if (do_conv) {
	    n = Simple::MakeMatlabCompliant(mapmatlabname+cname);
	    vc.Label(n).WriteMatlab(n+".m");
	    GetQuery()->WriteLog("Wrote map convolution matlab vector file <", n, ".m>");
	    n += "_m";
	    simple::FloatMatrix mm = Convolved(i, j, k);
	    mm.Label(n).WriteMatlab(n+".m");
	    GetQuery()->WriteLog("Wrote map convolution matlab matrix file <", n, ".m>");
	  }
	}
	if (mapdatname!="") {
	  if (do_hits) {
	    n = mapdatname+hname;
	    vh.Label(n).Write(n+".dat");
	    GetQuery()->WriteLog("Wrote map hits in <", n, ".dat>");
	  }
	  if (do_conv) {
	    n = mapdatname+cname;
	    vc.Label(n).Write(n+".dat");
	    GetQuery()->WriteLog("Wrote map convolution in <", n, ".dat>");
	  }
	}
      }
    }
  
    return true;
  }

  //===========================================================================

  bool CbirPicsom::QueryData::DoEntropyCalculation(int i, int j, int k) {
    if (!IsTsSom(i)) 
      return true;

    const float nan = Index::State::nan();
    const simple::FloatMatrix& mm = Convolved(i, j, k);

    bool check = k==1;
    float entropy = mm.EntropyWithCheck(check), e0 = nan, entropy_n = nan;
    float maxentr = MaximumEntropy(mm);
  
    if (maxentr==0.0) // zero-sized map, perhaps non-bottom level
      return true;

    TSSOM::State& fd = IndexDataTSSOM(i);
    fd.entropy[j]   = entropy;
    fd.entropy_r[j] = entropy/maxentr;

    const FloatVector *cvec = ConvolutionVector(ConvType(i), j);
    if (cvec) {
      e0 = ConvolutionEntropy(cvec);
      if (e0>=0.0) {
	fd.entropy_p[j] = entropy-e0;

	// obs! NpositiveSeenObjects() may not return what we expect...
	int npos = NpositiveSeenObjects();
	if (npos>1) {
	  float e_div = (e0>0 && npos*e0<maxentr ? npos*e0 : maxentr) - e0;
	  if (e_div)
	    entropy_n = (entropy-e0)/e_div;
	} else
	  entropy_n = 0.0;

	fd.entropy_n[j] = entropy>e0 ? entropy_n : 0.0;
      }
    }

    if (debug_weights)
      cout << "entropies " << MapName(false, i, j, k)
	   << " H=" << entropy << " Hmax=" << maxentr
	   << " Hr=" << fd.entropy_r[j] << " e0=" << e0
	   << " Hp=" << fd.entropy_p[j] << " Hnraw=" << entropy_n
	   << " Hn=" << fd.entropy_n[j] << endl;

    return true;
  }

  //===========================================================================
  //===========================================================================
  //===========================================================================
  //===========================================================================
  //===========================================================================

} // namespace picsom

