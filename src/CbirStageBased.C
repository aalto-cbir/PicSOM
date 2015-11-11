// -*- C++ -*-  $Id: CbirStageBased.C,v 2.33 2011/03/31 09:36:32 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <CbirStageBased.h>
#include <Valued.h>
#include <Query.h>

namespace picsom {
  ///
  static const string CbirStageBased_C_vcid =
    "@(#)$Id: CbirStageBased.C,v 2.33 2011/03/31 09:36:32 jorma Exp $";

  bool CbirStageBased::debug_check_lists = false;

  //===========================================================================

  /// This is the principal constructor.
  CbirStageBased::CbirStageBased(DataBase *db, const string& args)
    : CbirAlgorithm(db, args) {

    string hdr = "CbirStageBased::CbirStageBased() : ";
    if (debug_stages)
      cout << TimeStamp() << hdr+"constructing an instance" << endl;

    for (int f=stage_first; f<=stage_last; f++) {
      StageFunc((cbir_stage)f, func_default);
      StageArgs((cbir_stage)f, "");
    }
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirStageBased::Initialize(const Query *q, const string& s,
			       CbirAlgorithm::QueryData*& qd) const {
    string hdr = "CbirStageBased::Initialize() : ";
    if (!CbirAlgorithm::Initialize(q, s, qd))
      return ShowError(hdr+"CbirAlgorithm::Initialize() failed");

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirStageBased::AddIndex(CbirAlgorithm::QueryData *qd,
                                Index::State *f) const {
    string hdr = "CbirStageBased::AddIndex() : ";
    if (!CbirAlgorithm::AddIndex(qd, f))
      return ShowError(hdr+"CbirAlgorithm::AddIndex() failed");

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first checks for its own parameters and then calls the
  /// corresponding method of the base class.
  bool CbirStageBased::Interpret(CbirAlgorithm::QueryData *qd, const string& k,
				 const string& v, int& r) const {
    string hdr = "CbirStageBased::Interpret() : ";

    if (debug_stages)      
      cout << TimeStamp() << hdr+"key=\""+k+"\" value=\""+v+"\"" << endl;

    CbirStageBased::QueryData *qde = CastData(qd);

    r = 1;

    int intval = atoi(v.c_str());

    if (k=="seed") {
      qde->seed = intval;
      return true;
    }

    return CbirAlgorithm::Interpret(qd, k, v, r);
  }

  //===========================================================================

  bool CbirStageBased::FunctionNames(int i, cbir_function& f, const char*& t) {
    struct function_entry {
      cbir_function func;
      const char *name;
    };

    static function_entry list[] = {
#define make_func_entry(e) { func_ ## e , # e }
      make_func_entry(unknown),
      make_func_entry(error),
      make_func_entry(true),
      make_func_entry(default),
      make_func_entry(restriction_spec),
      make_func_entry(only_forced_images),
      make_func_entry(only_forced_images_user_test),
      make_func_entry(top_levels_by_entropy),
      make_func_entry(random_unseen_images),
      make_func_entry(random_positive_image),    
      make_func_entry(goto_initial_set),
      make_func_entry(goto_has_positives),
      make_func_entry(goto_expand_relevance),
      make_func_entry(goto_run_per_map),
      make_func_entry(goto_special_first_round),
      make_func_entry(all_unseen_images),
      make_func_entry(limit_images_with_vq),
      make_func_entry(expand),
      make_func_entry(expand_up),
      make_func_entry(expand_down),
      make_func_entry(expand_up_down),
      make_func_entry(expand_down_up),
      make_func_entry(expand_selective),
      make_func_entry(exchange),
      make_func_entry(exchange_up),
      make_func_entry(exchange_down),
      make_func_entry(exchange_up_down),
      make_func_entry(exchange_down_up),
      make_func_entry(converge),
      make_func_entry(converge_up),
      make_func_entry(converge_down),
      make_func_entry(converge_up_down),
      make_func_entry(converge_down_up),
      make_func_entry(per_map_picsom_all),
      make_func_entry(per_map_picsom_bottom),
      make_func_entry(per_map_vq),
      make_func_entry(goto_extract_images),
      make_func_entry(one_for_all),
      make_func_entry(one_for_bottom),
      make_func_entry(const_for_bottom),
      make_func_entry(entropy_n_polynomial),
      make_func_entry(entropy_n_leave_out_worst),
      make_func_entry(offline_shortcut),
      make_func_entry(extract_images_picsom_all),
      make_func_entry(extract_images_picsom_bottom),
      make_func_entry(extract_images_vq),
      make_func_entry(extract_images_sq),
      make_func_entry(remove_duplicates_picsom),
      make_func_entry(remove_duplicates_vq),
      make_func_entry(process_images_picsom_bottom),
      make_func_entry(process_images_vq),
      make_func_entry(process_images_vqw)
#undef make_func_entry
    };

    int n = sizeof(list)/sizeof(function_entry);
    if (i<0 || i>=n) {
      f = func_unknown;
      t = NULL;
      return false;
    }
  
    f = list[i].func;
    t = list[i].name;
    return true;
  }

  //===========================================================================

  const char* CbirStageBased::FunctionName(cbir_function x) {

    for (int i=0;; i++) {
      cbir_function f;
      const char *name;
      if (!FunctionNames(i, f, name))
	break;

      if (x==f)
	return name;
    }
    static char msg[1000];
    sprintf(msg, "--unknown (%d)--", (int)x);

    return msg;

  }

  //===========================================================================

  const CbirStageBased::stage_name_map_t& CbirStageBased::StageNameMap() {
    static stage_name_map_t nmap;
    
    if (nmap.empty()) {
#define make_stage_entry(e) nmap[stage_ ## e] = # e
      make_stage_entry(restrict);
      make_stage_entry(enter);
      make_stage_entry(initial_set);
      make_stage_entry(check_image_count);
      make_stage_entry(no_positives);
      make_stage_entry(has_positives);
      make_stage_entry(special_first_round);
      make_stage_entry(expand_relevance);
      make_stage_entry(run_per_map);
      make_stage_entry(set_map_weights);
      make_stage_entry(combine_unit_lists);
      make_stage_entry(extract_images);
      make_stage_entry(combine_image_lists);
      make_stage_entry(remove_duplicates);
      make_stage_entry(exchange_relevance);
      make_stage_entry(select_images_to_process);
      make_stage_entry(process_images);
      make_stage_entry(converge_relevance);
      make_stage_entry(final_select);
      make_stage_entry(ready);
      make_stage_entry(error);
#undef make_stage_entry
    }

    return nmap;
  }

  //===========================================================================

  cbir_function CbirStageBased::StageFunction(cbir_stage x, bool def,
					      bool show_def,
					      bool warn) const {
    cbir_function ff, err = func_unknown;

    try {
      ff = StageFunc(x);
    }
    catch (...) {
      return err;
    }

    if ((ff!=func_default || def) && !show_def)
      return ff;

    return StageDefault(x, warn);
  }

  //===========================================================================

  /// This virtual method really needs to be overwritten in a derived
  /// class.
  ObjectList CbirStageBased::CbirRound(CbirAlgorithm::QueryData *qd,
				       const ObjectList& seen,
				       size_t maxqin) const {
    string hdr = "CbirStageBased::CbirRound() : ";

    CbirStageBased::QueryData *qds = CastData(qd);

    // This is called only for CbirAlgorithm::CbirRound()'s logging ability
    // it will return an empty list which is NOT an indication of a failure
    CbirAlgorithm::CbirRound(qds, seen, maxqin);

    size_t maxq = qd->GetMaxQuestions(maxqin);

    for (size_t f=0; f<Nindices(qds); f++) {
      int matrixcount = 1;   // Query::GetMatrixCount();
      bool centropy = true; // false; // CbirPicsom::CalculateEntropy();
      ((TSSOM::State*)&qds->IndexData(f))->Initialize(false, matrixcount,
						      centropy);
    }

    qds->seenobject = seen;

    cbir_stage stage = stage_first;

    bool ok = true;
    bool sloppy = true;

    for (; ok; ) {
      PicSOM::ThreadTestCancel();

      if (debug_stages)
	cout << TimeStamp() << hdr <<" stage: " << StageName(stage) << " = "
	     << FunctionName(StageFunction(stage, false, false, false))
	     << " approaching switch () " << endl;

      if (stage==stage_ready)
	break;

      cbir_stage old_stage = stage;
      cbir_function func = StageFunction(stage);
      const string& args = StageArgs(stage);
  
      string msg = string("stage=<")+StageName(stage)+"> func=<"
	+FunctionName(func)+">";

      if (func==func_error)
	ok = ShowError(hdr+"func==func_error in "+msg);

      if (func==func_unknown)
	ok = ShowError(hdr+"func==func_unknown in "+msg);

      if (ok)
	switch (stage) {
	case stage_restrict:
	  stage = StageRestrict(qds, func, args, maxq);
	  break;

	case stage_enter:
	  stage = StageEnter(qds, func, args, maxq);
	  break;

	case stage_initial_set:
	  stage = StageInitialSet(qds, func, args, maxq);
	  break;

	case stage_check_image_count:
	  stage = StageCheckNimages(qds, func, args, maxq);  // obs! naming differs...
	  break;

	case stage_no_positives:
	  stage = StageNoPositives(qds, func, args, maxq);
	  break;

	case stage_has_positives:
	  stage = StageHasPositives(qds, func, args, maxq);
	  break;

	case stage_special_first_round:
	  stage = StageSpecialFirstRound(qds, func, args, maxq);
	  break;

	case stage_expand_relevance:
	  stage = StageExpandRelevance(qds, func, args, maxq);
	  break;

	case stage_run_per_map:
	  stage = StageRunPerMap(qds, func, args, maxq);
	  break;

	case stage_set_map_weights:
	  stage = StageSetMapWeights(qds, func, args, maxq);
	  break;

	case stage_combine_unit_lists:
	  stage = StageCombineUnitLists(qds, func, args, maxq);
	  break;

	case stage_extract_images:
	  stage = StageExtractObjects(qds, func, args, maxq);
	  break;

	case stage_combine_image_lists:
	  stage = StageCombineObjectLists(qds, func, args, maxq);
	  break;

	case stage_remove_duplicates:
	  stage = StageRemoveDuplicates(qds, func, args, maxq);
	  break;

	case stage_exchange_relevance:
	  stage = StageExchangeRelevance(qds, func, args, maxq);
	  break;

	case stage_select_images_to_process:
	  stage = StageSelectObjectsToProcess(qds, func, args, maxq);
	  break;

	case stage_process_images:
	  stage = StageProcessObjects(qds, func, args, maxq);
	  break;

	case stage_converge_relevance:
	  stage = StageConvergeRelevance(qds, func, args, maxq);
	  break;

	case stage_final_select:
	  stage = StageFinalSelect(qds, func, args, maxq);
	  break;

	case stage_error:
	  ok = ShowError(hdr+"stage_error in switch () after "+msg);
	  break;

	default:
	  ok = ShowError(hdr+"default in switch () after "+msg);
	}

      if (debug_stages) {
	stringstream ss;
	ss << " seen: " << qds->NseenObjects() << " comb: "
	   << qds->combinedimage.size()
	   << " uniq: " << qds->uniqueimage.size() << "/"
	   << qds->NuniqueRetained()
	   << " new: "  << qds->NnewObjects() << "/"
	   << qds->NnewRetained() << "/"
	   << qds->NnewObjectsRetainedNonShow();
	string listinfo = ss.str();
	cout << TimeStamp() << "CbirRound() : " << StageName(old_stage) << " = "
	     << FunctionName(StageFunction(old_stage, false, false, false))
	     << " exited switch () with ok=" << ok
	     << " next stage=" << StageName(stage) << listinfo << endl;

	if (false)
	  Picsom()->PossiblyShowDebugInformation(string("In CbirStages() ")+
						 StageName(old_stage)+" ended");
      }

      if (ok && stage==stage_error)
	ok = ShowError(hdr+"stage_error resulted after "+msg);

      if (ok && stage==old_stage)
	ok = ShowError(hdr+"stage==old_stage after "+msg);

      if (ok && !ConditionallyCheckObjectIndices(qds, sloppy))
	ok = ShowError(hdr+"object list disintegrity after "+msg);
    }

      /*
    if (matlabdump!="")
      DoDump(0, matlabdump);

    if (datdump!="")
      DoDump(1, datdump);

    if (debug_selective) {
      cout << TimeStamp() << "dumping newimage.rel_distribution";

      for(int i=0; i<NnewImages(qd); i++)
	if (newimage[i].Retained())
	  newimage[i].rel_distribution.dump();
    }
    */

    Picsom()->PossiblyShowDebugInformation("CbirStages() ending");

    return qds->newobject;
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageRestrict(CbirStageBased::QueryData * /*qds*/,
					   cbir_function func,
					   const string& /*args*/, size_t /*maxq*/) const {
    bool ok = false;

    switch (func) {
    case func_true:
      /*
	RemoveTemporalRestriction();
	restriction = CheckDB()->GroundTruthExpression("gender_Male",
	target_image, -1);
      */
      ok = true;
      break;

    case func_restriction_spec:
      ok = true; // RestrictionSpec(args);
      break;

    default:
      ShowError("CbirStageBased::StageRestrict(): operation for <",
		FunctionName(func), "> undefined");
    }

    return ok ? stage_enter : stage_error;
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageEnter(CbirStageBased::QueryData *qds,
					cbir_function func,
					const string& /*args*/, size_t /*maxq*/) const {
    if (func==func_goto_has_positives)
      return stage_has_positives;

    if (func!=func_default) {
      ShowError("CbirStageBased::StageEnter(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
    
    if (!qds->NseenObjects())
      return stage_initial_set;

    if (!qds->NpositiveSeenObjects())
      return stage_no_positives;

    target_type tt = target_any_target; // obs! added when type was required...
    if (qds->NseenObjects()<GetQuery(qds)->DataBaseRestrictionSize(tt)+
	GetQuery(qds)->RestrictedObjectsSeen() ||
	GetQuery(qds)->CanShowSeen())
      return stage_has_positives;

    return stage_ready;
  }
    
  //===========================================================================

  cbir_stage CbirStageBased::StageInitialSet(CbirStageBased::QueryData *qds,
					     cbir_function func,
					     const string& args,
					     size_t maxq) const {
    switch (func) {
    case func_only_forced_images:
    case func_only_forced_images_user_test:
      return OnlyForcedObjects(args) ? stage_ready : stage_error;
    
    case func_random_unseen_images:
      RandomUnseenObjects(qds, maxq, true, false);
      return stage_ready;

    case func_random_positive_image:
      RandomUnseenObjects(qds, maxq, true, true);
      return stage_expand_relevance;
    
    default:
      ShowError("CbirStageBased::StageInitialSet(): operation for <",
 		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  bool CbirStageBased::RandomUnseenObjects(CbirStageBased::QueryData *qd,
					  size_t maxq,
					  bool deletenewimages,
					  bool justonepositive) const {
    if (!DataBaseSize())
      return ShowError("Query::RandomUnseenObjects() : DataBaseSize()==0");

    Tic("RandomUnseenObjects");
    if (deletenewimages) {
      qd->DeleteNew();
      qd->random_image_count = 0;
    }

    int initialsize = qd->NnewObjects();
    target_type tt = target_any_target; // obs! added when type was required...
    size_t available = DataBaseRestrictionSize(tt)-qd->NseenObjects();
    size_t maxq_eff = qd->GetMaxQuestions(maxq);
    size_t lll = justonepositive ? 1 : maxq_eff<available ? maxq_eff : available;

    map<string,aspect_data> am = AspectsFromIndices(qd);
    map<string,aspect_data> *ap = &am; // NULL;

    IntVector idx;
    idx.SetIndices(DataBaseSize());

    RandVar rnd(qd->seed);
    int r = -1;
    while (qd->NnewObjects()<lll && idx.Length()) {
      int s = rnd.RandomInt(idx.Length());
      r = idx[s];
      int p = idx.Pop();
      if (s!=idx.Length())
	idx[s] = p;

      if (!CanBeShownRestricted(qd, r, false))
	continue;

      if (!database->ObjectsTargetTypeContains(r, qd->GetQuery()->Target()))
	continue;

      bool found = false;
      for (int i=0; !found && i<initialsize; i++)
	if (r==qd->NewObject(i).Index())
	  found = true;
      if (found)
	continue;

      Object *img = new Object(database, r, select_question, 1, "", -1, NULL, ap);

      qd->AppendNewObject(img, img->Index());

      qd->random_image_count++;
    }

    if (justonepositive) {
      return ShowError("query object is const...");
      // qd->GetQuery()->MarkAsSeenNoAspects(r, +1.0);
    }

    Tac("RandomUnseenObjects");

    return true;
  } 

  //===========================================================================
  
  cbir_stage CbirStageBased::StageCheckNimages(CbirStageBased::QueryData* qds,
					       cbir_function func,
					       const string& /*args*/,
					       size_t maxq) const {
    if (qds->NnewObjects()==qds->GetMaxQuestions(maxq))
      return stage_ready;

    switch (func) {
    case func_top_levels_by_entropy:    
      ShowError("CbirStageBased::StageCheckNimages(): operation for <",
		FunctionName(func), "> not yet defined");
      return stage_error;
      //entropy_level++;
      //TopLevelsByEntropy();
      //return stage_check_nimages;
    
    case func_random_unseen_images:
      RandomUnseenObjects(qds, maxq, false, false); // first false disables DeleteNew();
      return stage_ready;
    
    default:
      ShowError("CbirStageBased::StageCheckNimages(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageNoPositives(CbirStageBased::QueryData* /*qds*/,
					      cbir_function func, 
					      const string& /*args*/, size_t /*maxq*/) const {
    switch (func) {
    case func_goto_has_positives:
      return stage_has_positives;
    
    case func_goto_initial_set:
      return stage_initial_set;

    default:
      ShowError("CbirStageBased::StageNoPositives(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageHasPositives(CbirStageBased::QueryData* /*qds*/,
					       cbir_function func,
					       const string& /*args*/, size_t /*maxq*/) const {
    switch (func) {
    case func_goto_initial_set:
      return stage_initial_set;
      
    case func_goto_expand_relevance:
      return stage_expand_relevance;
      
    case func_goto_special_first_round:
      return stage_special_first_round;
      
    default:
      ShowError("CbirStageBased::StageHasPositives(): operation for <", 
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageExpandRelevance(CbirStageBased::QueryData* qds,
						  cbir_function func,
						  const string& args, size_t /*maxq*/) const {
    bool ok = false;

    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_expand:
      ok = Expand(qds, args);
      break;

    case func_expand_up:
      ok = Expand(qds, "sum,none");
      break;

    case func_expand_down:
      ok = Expand(qds, "none,copy");
      break;

    case func_expand_up_down:
      ok = Expand(qds, "sum,copy");
      break;

    case func_expand_down_up:
      ShowError("ExpandDownUp() Obsoleted");
      ok = false;
      break;
    
    case func_expand_selective:
      ok = ExpandSelective(qds, args);
      break;

    default:
      ShowError("CbirStageBased::StageExpandRelevance(): operation for <",
		FunctionName(func), "> undefined");
    }

    return ok ? stage_run_per_map : stage_error;
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageRunPerMap(CbirStageBased::QueryData* /*qds*/,
					    cbir_function func,
					    const string& /*args*/, size_t /*maxq*/) const {
    switch (func) {
    case func_goto_extract_images:
      return stage_extract_images;
    
    default:
      ShowError("CbirStageBased::StageRunPerMap(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageSetMapWeights(CbirStageBased::QueryData* /*qds*/,
						cbir_function func,
						const string& /*args*/, size_t /*maxq*/) const {
    switch (func) {   

    default:
      ShowError("CbirStageBased::StageSetMapWeights(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageCombineUnitLists(CbirStageBased::QueryData* /*qds*/,
						   cbir_function func,
						   const string& /*args*/,
                                                   size_t /*maxq*/) const {
    switch (func) {   
    case func_offline_shortcut:
      ShowError("CbirStageBased::StageCombineUnitLists(): operation for <",
		FunctionName(func), "> not implemented");
      return stage_error;	   
      //return OfflineShortcut();

    default:
      ShowError("CbirStageBased::StageCombineUnitLists(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageExtractObjects(CbirStageBased::QueryData* /*qds*/,
                                                 cbir_function func,
                                                 const string& /*args*/,
                                                 size_t /*maxq*/) const {
    switch (func) {   

    default:
      ShowError("CbirStageBased::StageExtractImages(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageCombineObjectLists(CbirStageBased::QueryData* qds,
                                                     cbir_function func,
                                                     const string& /*args*/,
                                                     size_t /*maxq*/) const {
    if (func!=func_default)
      ShowError("CbirStageBased::StageCombineObjectLists() : func!=func_default");
    
    qds->DeleteCombined();
    for (int i=0; i<qds->PerMapObjectsSize(); i++)
      qds->combinedimage.append(qds->PerMapNewObject(i));

    return qds->combinedimage.size() ? stage_remove_duplicates : stage_error;
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageRemoveDuplicates(CbirStageBased::QueryData* /*qds*/,
						   cbir_function func,
						   const string& /*args*/,
                                                   size_t /*maxq*/) const {
    switch (func) {

    default:
      ShowError("CbirStageBased::StageRemoveDuplicates(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageExchangeRelevance(CbirStageBased::QueryData* qds,
						    cbir_function func,
						    const string& args, size_t /*maxq*/) const {
    bool ok = false;
    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_exchange:
      ok = Exchange(qds, args);
      break;
    
    case func_exchange_up:
      ok = Exchange(qds, "sum,none");
      break;
    
    case func_exchange_down:
      ok = Exchange(qds, "none,div");
      break;
    
    case func_exchange_up_down:
      ok = Exchange(qds, "sum,div");
      break;
    
    case func_exchange_down_up:
      ShowError("ExchangeDownUp() Obsoleted");
      ok = false;
      break;
    
    default:
      ShowError("CbirStageBased::StageExchangeRelevance(): operation for <",
		FunctionName(func), "> undefined");
    }

    return ok ? stage_select_images_to_process : stage_error;
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageSelectObjectsToProcess(CbirStageBased::QueryData* qds,
                                                         cbir_function func,
                                                         const string& args,
                                                         size_t /*maxq*/) const {
    if (func!=func_default)
      ShowError("CbirStageBased::StageSelectImagesToProcess() : func!=func_default");

    int n = qds->uniqueimage.size();

    int limit_total_images = args.size() ? atoi(args.c_str()) : 0;
  
    if (limit_total_images && limit_total_images<n) {

      FloatVector values(qds->uniqueimage.size());
      for (int i=0; i<values.Length(); i++)
	values[i] = qds->uniqueimage[i].Value();
      values.QuickSortDecreasingly();

      float ref = values[limit_total_images-1];
      n = 0;
      for (size_t k=0; k<qds->uniqueimage.size(); k++)
	if (n>=limit_total_images || qds->uniqueimage[k].Value()<ref)
	  qds->uniqueimage[k].Discard();
	else
	  n++;
    }

    qds->DeleteNew();
    qds->random_image_count = 0;

    map<string,aspect_data> am = AspectsFromIndices(qds);

    for (size_t i=0; i<qds->uniqueimage.size(); i++)
      if (qds->uniqueimage[i].Retained()) {
	Object o(qds->uniqueimage[i]);
	o.Aspects(am);
	qds->AppendNewObject(o, o.Index());
      }

    return qds->NnewObjects() ? stage_process_images : stage_error;
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageProcessObjects(CbirStageBased::QueryData* /*qds*/,
                                                 cbir_function func,
                                                 const string& /*args*/,
                                                 size_t /*maxq*/) const {
    switch (func) {
    case func_true:
      return stage_converge_relevance;

    default:
      ShowError("CbirStageBased::StageProcessImages(): operation for <",
		FunctionName(func), "> undefined");
      return stage_error;
    }
  }

  //===========================================================================

  cbir_stage CbirStageBased::StageConvergeRelevance(CbirStageBased::QueryData* qds,
						    cbir_function func,
						    const string& args, size_t /*maxq*/) const {
    bool ok = false;
    switch (func) {
    case func_true:
      ok = true;
      break;

    case func_converge:
      ok = Converge(qds, args);
      break;

    case func_converge_up:
      ok = Converge(qds, "sum,none");
      break;

    case func_converge_down:
      ok = Converge(qds, "none,div");
      break;

    case func_converge_up_down:
      ok = Converge(qds, "sum,div");
      break;

    case func_converge_down_up:
      ShowError("ConvergeDownUp() Obsoleted");
      ok = false;
      break;

    default:
      ShowError("CbirStageBased::StageConvergeRelevance(): operation for <",
		FunctionName(func), "> undefined");
    }

    return ok ? stage_final_select : stage_error;

  }

  //===========================================================================

  cbir_stage CbirStageBased::StageFinalSelect(CbirStageBased::QueryData* qds,
					      cbir_function func,
					      const string& args, size_t maxq) const {

    if (func==func_true) { // bypass this stage
      for (size_t i=0; i<qds->NnewObjects(); i++) {
	qds->NewObject(i).Discard();
	qds->OldSetNewObjectIdx(qds->NewObject(i).Index(), 0);
      }
      qds->PossiblyRelativizeScores();

      return stage_ready;
    }

    if (func!=func_default)
      ShowError("CbirStageBased::StageFinalSelect() : func!=func_default");

    string filterclass;

    size_t filterpos = args.find("filter=");
    if (filterpos!=string::npos){
      const char *str = args.substr(filterpos+7).c_str();
      int incr = 0;

      while (str[incr] && str[incr]!=',' && !isspace(str[incr]) && 
	    str[incr] != ')')
	incr++;

      filterclass = args.substr(filterpos+7,incr);
      cout << TimeStamp() << "Parsed final filter "<<filterclass<<endl;
    }

    //bool store_relevance= arg.find("store_relevance")!=string::npos;

    //if(store_relevance && matrixcount < 3)
    //  ShowError("StageFinalSelect() : store_relevance && matrixcount < 3");

//     bool productboost = args.find("productboost")!=string::npos;

//     if (productboost && GetMatrixCount()<3)
//       ShowError("CbirStageBased::StageFinalSelect() : productboost && matrixcount < 3");

    qds->GetQuery()->Tic("StageFinalSelect");

    //  if (productboost) {

    //     const int largeval = 10;

    //     StoreSubobjectContribution("");
    //     for (int i=0; i<NnewImages(); i++) {
    //       const vector<float> &vals=newimage[i].rel_distribution.subval;

    //       bool reaches_unity=false;
    //       for (size_t n=0; n<vals.size(); n++)
    // 	if (vals[n]>= 1.0) {
    // 	  reaches_unity=true;
    // 	  break;
    // 	}

    //       if (reaches_unity) {
    // 	newimage[i].AddValue(largeval);
    // 	if (debug_selective)
    // 	  cout << TimeStamp() << "Added value to label " << newimage[i].Label() <<
    // 	    " val now " << newimage[i].Value() << endl;
    //       }
    //     }
    //   }


    target_type qt = qds->GetQuery()->Target();
    
    if (!filterclass.empty()){
      // if (tt==target_no_target) // commented out 2.5.2006 by jl
      //   tt = target_image;  // this might be redundant...
      
      bool expand = true;
      ground_truth ffilter = GetDataBase()->GroundTruthExpression(filterclass, qt,
								  -1, expand);

      for (size_t i=0; i<qds->NnewObjects(); i++) {
	if (!qds->NewObject(i).Retained())
	  continue;
	int idx=qds->NewObject(i).Index();

	bool infilter = ffilter.IndexOK(idx) && ffilter[idx]==1;

	if (!infilter) {
	  // cout << TimeStamp() << "finalfilter: discarding " << NewImage(i).Label()<<endl;
	  qds->NewObject(i).Discard();	
	  qds->OldSetNewObjectIdx(qds->NewObject(i).Index(), 0);
	}
	//else
	//  cout << TimeStamp() << "finalfilter: letting through " 
	//       << NewImage(i).Label()<<endl;
      }
    }
 
    //Object::SortList(newimage);

    vector<double> vals;	
    size_t n = 0;

    for (size_t i=0; i<qds->NnewObjects(); i++) {
      if (!qds->NewObject(i).Retained())
	continue;
      target_type tt = qds->NewObject(i).TargetType();
      // bool type_ok = tt&query_target;
      bool type_ok = PicSOM::TargetTypeContains(tt, qt);
      if (type_ok) {
	vals.push_back(qds->NewObject(i).Value());
	n++;
      } else {
	qds->NewObject(i).Discard();	
	qds->OldSetNewObjectIdx(qds->NewObject(i).Index(), 0);
      }	
    }

    // cout << TimeStamp() <<"n="<<n<<" maxquestions="<<MaxQuestions()<<endl;

    double lastval = 0.0;
    int count = 0;
    if (n>maxq) {
      sort(vals.begin(),vals.end());
      lastval = vals[n-maxq];

      for (size_t i=0; i<qds->NnewObjects(); i++) {
	if (!qds->NewObject(i).Retained())
	  continue;
	if (qds->NewObject(i).Value()<lastval) {
	  // this is a bit sloppy, see also CheckObjectIdx()
	  qds->NewObject(i).Discard();
	  qds->OldSetNewObjectIdx(qds->NewObject(i).Index(), 0);
	} else
	  count++;
      }
    }

    //int retainedcount=0;
    
//     for (int i=0; i<NnewImages(); i++){
//       if (NewImage(i).Retained())
// 	{
// 	retainedcount++;
// 	//	cout << TimeStamp() << i << "retained." << endl;
// 	}}


//     cout << TimeStamp() << "NnewImages()="<<NnewImages()<<" ("<<retainedcount<<" retained)"<<endl;

//     // move discarded items to the end of the newimage

//     int lastretained=NnewImages()-1;
//     while(lastretained >=0 && !NewImage(lastretained).Retained()) lastretained--;

//     for(int i=0;i<lastretained;i++){
//       if(NewImage(i).Retained()) continue;
//       newimage.Swap(i,lastretained);
//       while(lastretained >=0 && !NewImage(lastretained).Retained()) lastretained--;
//     }

    qds->GetQuery()->Tic("sort");
    if (args.find("nosort")==string::npos)
      Object::SortListByValue(qds->NewObject());
    qds->GetQuery()->Tac("sort");

    // this is needed if many objects in the end of the list  have the same value
    while (count-->(int)maxq) {
      qds->NewObject(count).Discard();
      qds->OldSetNewObjectIdx(qds->NewObject(count).Index(), 0);
    }

    // if (store_relevance && !productboost) {
    //   StoreSubobjectContribution("");
    // }

    if (debug_selective)
      cout << TimeStamp() << "StageFinalSelect(): score threshold " << lastval << endl;

    qds->GetQuery()->Tac("StageFinalSelect");

    // return NnewImages()&&NewImage(0).Retained() ? stage_ready : stage_error; 
    qds->PossiblyRelativizeScores();

    return n ? stage_ready : stage_error;
  }

  //===========================================================================
  //===========================================================================

  bool CbirStageBased::StrVsRelop(string& op, enum relop_type& r, bool setrelop) const {
    static struct string_to_relop {
      string long_str, short_str;
      enum relop_type relop;
    } table[] = {
      { "none",    "-",   relop_none },
      { "default", ".",   relop_default },
      { "sum",     "+",   relop_sum },
      { "maximum", "max", relop_max },
      { "average", "avg", relop_avg },
      { "copy",    "=",   relop_copy },
      { "divide",  "div", relop_divide },
      { "divide",  "/",   relop_divide },
      { "",        "",    relop_none }
    };
  
    for (const struct string_to_relop *p = table; p->long_str.size(); p++) {
      if (setrelop) {
	if (p->long_str == op || p->short_str == op) {
	  r = p->relop;
	  return true;
	}
      } else {
	if (p->relop == r) {
	  op = p->long_str;
	  return true;
	}
      }
    }

    return false;
  }

  //===========================================================================

  bool CbirStageBased::StrToRelopStage(const string& s, 
			      struct relop_stage_type& rp) const {
    string format_msg = "Arguments should be in the format: "
      "(none|sum|max|avg, none|copy|divide) "
      "where the first parameter is the relevance up operation "
      "and the last is the down operation.";

    if (!s.size())
      return ShowError(format_msg);

    string::size_type commapos = s.find(',');
    if (commapos==string::npos)
      return ShowError(format_msg);
    string op1 = s.substr(0,commapos);
    string op2 = s.substr(commapos+1);
      
    relop_type up = StrToRelop(op1); 
    relop_type down = StrToRelop(op2);

    if (up == relop_default) up = rp.up;
    if (down == relop_default) down = rp.down;

    if (up!=relop_sum && up!=relop_max && up!=relop_avg && up!=relop_none)
      return ShowError("Relevance up operation must be "
		       "sum, max, avg or none.");
    if (down!=relop_copy && down!=relop_divide && down!=relop_none)
      return ShowError("Relevance down operation must be "
		       "copy, divide or none.");

    rp.up = up;
    rp.down = down;
    return true;
  }

  //===========================================================================
  //===========================================================================

  bool CbirStageBased::Expand(CbirStageBased::QueryData* qds, const string& args) const {
    static string oldargs="";
    if (args != oldargs) {
      if (!StrToRelopStage(args,expand_operation)) return false;
      oldargs = args;
      LogRelopStage(qds, "Expand relevance  ",expand_operation);
    }

    bool res = true;
    switch (RelopStageToChoice(expand_operation)) {
    case RELEVANCE_SKIP:
      return res;
    case RELEVANCE_UP:
      res = RelevanceUp(qds, args, expand_operation, qds->seenobject, false, true);
      SortSeen();
      return res;
    case RELEVANCE_DOWN:
      res = RelevanceDown(qds, args, expand_operation, qds->seenobject, false, true);
      SortSeen();
      return res;
    }
    res = RelevanceUpDown(qds, args, expand_operation, qds->seenobject, false, true);
    SortSeen();
    return res;
  }

  //===========================================================================

  bool CbirStageBased::ExpandSelective(CbirStageBased::QueryData* /*qds*/, const string& /*arg*/) const {
    ShowError("CbirStageBased::ExpandSelective() not implemented yet!");
    return false;
  }

  //===========================================================================

  bool CbirStageBased::Exchange(CbirStageBased::QueryData* qds, const string& args) const {
    static string oldargs="";
    if (args != oldargs) {
      if (!StrToRelopStage(args,exchange_operation)) return false;
      oldargs = args;
      LogRelopStage(qds, "Exchange relevance",exchange_operation);
    }

    switch (RelopStageToChoice(exchange_operation)) {
    case RELEVANCE_SKIP:
      return true;
    case RELEVANCE_UP:
      return RelevanceUp(qds, args, exchange_operation, qds->uniqueimage, true, false);
    case RELEVANCE_DOWN:
      return RelevanceDown(qds, args, exchange_operation, qds->uniqueimage, true,false);
    }
    return RelevanceUpDown(qds, args, exchange_operation, qds->uniqueimage, true, false);
  }

  //===========================================================================

  bool CbirStageBased::Converge(CbirStageBased::QueryData* qds, const string& args) const {
    static string oldargs="";
    if (args != oldargs) {
      if (!StrToRelopStage(args,converge_operation)) return false;
      oldargs = args;
      LogRelopStage(qds, "Converge relevance",converge_operation);
    }

    switch (RelopStageToChoice(converge_operation)) {
    case RELEVANCE_SKIP:
      return true;
    case RELEVANCE_UP:
      return RelevanceUp(qds, args, converge_operation, qds->newobject, true, false);
    case RELEVANCE_DOWN:
      return RelevanceDown(qds, args, converge_operation, qds->newobject, true, false);
    }
    return RelevanceUpDown(qds, args, converge_operation, qds->newobject, true,false);
  }

  //===========================================================================
  //===========================================================================

  size_t CbirStageBased::QueryData::NseenObjects(int sign, target_type tt) const {
    if (tt==target_no_target)
      return ShowError("NseenObjects() : target_type==target_no_target");

    size_t ret = 0;
    for (size_t i=0; i<NseenObjects(); i++)
      if (tt==target_any_target ||
	  PicSOM::TargetTypeContains(seenobject[i].TargetType(), tt))
	if ((sign<0  && seenobject[i].Value()<0)  ||
	    (sign==0 && seenobject[i].Value()==0) ||
            (sign>0  && seenobject[i].Value()>0))
	  ret++;

    return ret;
  }

  //===========================================================================

  size_t CbirStageBased::QueryData:: NnewObjectsRetainedNonShow() const {
    size_t n = 0;
    for (size_t i=0; i<NnewObjects(); i++) {
      const Object& obj = NewObject(i);
      if (obj.Retained() && obj.SelectType()!=select_show)
	n++;
    }
    return n;
  }

  //===========================================================================

  int CbirStageBased::QueryData::PlaceInSeen_idx(size_t idx, bool& exists, int r) const { 
    // Tic("PlaceInSeen");

    bool pre2122bug = false;

    int ret = 0, i, l=0, intidx = idx;
    exists = false;
    if (!NseenObjects())
      goto do_tac;

    if (r==MAXINT)
      r = NseenObjects()-1;

    if (!pre2122bug && intidx>seenobject[r].Index()) {
      ret = r+1;
      goto do_tac;
    }

    if (intidx>seenobject[r].Index()) 
      i=l=r;
    else{
      do {
	i = (l+r)/2;
	if (intidx<seenobject[i].Index()) 
	  r=i-1;
	else
	  l=i+1;
      } while (intidx!=seenobject[i].Index() && l<=r);
    }

    if (intidx==seenobject[i].Index()) {
      exists = true;
      ret = i;
    } else 
      ret = l;

  do_tac:
    // Tac("PlaceInSeen");

    return ret;
  }  

  //===========================================================================

  bool CbirStageBased::QueryData::PossiblyRelativizeScores() {
    if (!relativescores)
      return true;

    float div = 0.0;
    for (size_t i=0; div==0.0 && i<NnewObjects(); i++)
      if (NewObject(i).Retained())
	div = NewObject(i).Value();
    
    for (size_t i=0; div>0 && i<NnewObjects(); i++) {
      NewObject(i).Value(NewObject(i).Value()/div);
      NewObject(i).MultiplyStageInfo(1/div);
    }
    
    return true;
  }

  //===========================================================================

  bool CbirStageBased::QueryData::CheckObjectIdx(const ObjectList& l,
						 const string& n,
						 bool /*sloppy*/) const {
    // USED TO BE: sloppy = true; // see also StageFinalSelect()
    bool ok = true; 

    for (size_t i=0; i<l.size(); i++)
      for (size_t j=i+1; j<l.size(); j++)
	if (l[i].Index()==l[j].Index())
	  ok = ShowError(n+" error C i="+ToStr(i)+" j="+ToStr(j)+ " #",
			 ToStr(l[i].Index())+" <"+l[i].Label()+">");

    bool r = l.check(false);
    if (!r)
      ok = ShowError(n+" ObjectList::check() failed");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool CbirStageBased::QueryData::CrossCheckSeenObjectIdx() const {
    bool ok = true;
    for (size_t i=0; i<NseenObjects(); i++) {
      const Object& o = seenobject[i];
      if (FindInObjectIndex(o.Index(), uniqueimage))
	ok = ShowError("seenobject crosscheck with uniqueimage : #",
		       ToStr(o.Index()), " <", o.Label(), ">");

      if (FindInObjectIndex(o.Index(), newobject))
	ok = ShowError("seenobject crosscheck with newobject : #",
		       ToStr(o.Index()), " <", o.Label(), ">");
    }
    return ok;
  }

  //===========================================================================
  //===========================================================================
  //===========================================================================

} // namespace picsom

