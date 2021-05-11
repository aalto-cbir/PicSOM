// -*- C++ -*-  $Id: CbirStageBased.h,v 2.28 2021/05/11 14:46:57 jormal Exp $
// 
// Copyright 1998-2021 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _CBIRSTAGEBASED_H_
#define _CBIRSTAGEBASED_H_

#include <CbirAlgorithm.h>
#include <Query.h>

using namespace std;

namespace picsom {
  ///
  static const string CbirStageBased_h_vcid =
    "@(#)$Id: CbirStageBased.h,v 2.28 2021/05/11 14:46:57 jormal Exp $";

  /**
     An stagebased of CBIR algorithm implementation derived from the abstract
     CbirAlgorithm class.  Copy CbirStageBased.h to CbirYourMethod.h and 
     CbirStageBased.C to CbirYourMethod.C and edit them as you wish.
  */
  class CbirStageBased : public CbirAlgorithm {

  public:
    /**

    */
    class QueryData : public CbirAlgorithm::QueryData {
      ///
      friend class CbirStageBased;

    public:
      /// poison
      QueryData() { abort(); }

      ///
      QueryData(const CbirAlgorithm *a, const Query *q) :
	CbirAlgorithm::QueryData(a, q), seed(123), relativescores(false) {}

      ///
      virtual CbirAlgorithm::QueryData *Copy(const Query *q) const {
	QueryData *copy = new QueryData(*this);
	copy->SetQuery(q);

	return copy;
      }

    protected:
      ///
      int seed;

      /// Set to true if the scores should be scaled down from one.
      bool relativescores;

    public:
      ///
      /// Function to run at that stage.
      cbir_function StageFunction(cbir_stage x, bool def = false,
				  bool show_def = false, bool warn = true) const {
	return ((CbirStageBased*)GetAlgorithm())
	  ->StageFunction(x, def, show_def, warn);
      }

      ///
      size_t NseenObjects() const { //return GetQuery()->NseenObjects();
	return seenobject.size();
      }

      ///
      size_t NseenObjects(int, target_type) const;

      ///
      const Object& SeenObject(int i) const { return seenobject[i]; }

      ///
      Object& SeenObject(int i) { return seenobject[i]; }

      ///
      size_t NpositiveSeenObjects() const { return NseenObjects(1, target_any_target); }

      ///
      size_t NnegativeSeenObjects() const { return NseenObjects(-1, target_any_target); }
      
      ///
      size_t NuniqueRetained() const { return uniqueimage.size(); } // obs! Retained

      ///
      size_t NnewObjects() const { return newobject.size(); }
      
      ///
      size_t NnewRetained() const { return newobject.size(); } // obs! Retained

      ///
      size_t NnewObjectsRetainedNonShow() const;

      ///
      ObjectList& NewObject() { return newobject; }     

      ///
      const Object& NewObject(int i) const { return newobject[i]; }
      
      ///
      Object& NewObject(int i) { return newobject[i]; }
      
      ///
      void DeleteCombined() { 
	combinedimage.clear(); 
      }

      ///
      void DeleteUnique() { 
	uniqueimage.clear(); 
      }

      ///
      void DeleteNew()  {
	newobject.clear();  
      }

      void OldSetNewObjectIdx(int, int) {}

      ///
      void AppendNewObject(Object *o, int idx, bool check = false) {
        // should be deprecated
	newobject.push_back(*o);
        delete o;
	if (!check || idx>=0)
	  OldSetNewObjectIdx(idx, 1);
      }

      ///
      void AppendNewObject(const Object& o, int idx, bool check = false) {
	newobject.push_back(o);
	if (!check || idx>=0)
	  OldSetNewObjectIdx(idx, 1);
      }
                        
      /// Locates the object in seenobject[].
      Object *FindInSeen(int idx) {
	bool imgexists = false;
	// const char *label = LabelP(idx);
	// int imgplace = PlaceInSeen_lab(label, imgexists);
	int imgplace = PlaceInSeen_idx(idx, imgexists);
	return imgexists ? &SeenObject(imgplace) : NULL;
      }
      
      /// Locates the object in seenobject[].
      const Object *FindInSeen(int idx) const {
	return ((Query*)this)->FindInSeen(idx);
      }

      /// Solves the index where label should be or is placed.
      int PlaceInSeen_idx(size_t, bool&, int = MAXINT) const;

      ///
      bool IsInNewObjects(int idx) const {
	return newobject.contains((size_t)idx);
      }

      /// Used in StageFinalSelect().
      bool PossiblyRelativizeScores();

      /// Locates the object in seenobject[].
      const Object *FindInObjectIndex(int idx, const ObjectList& l) const {
	size_t n = l.size();
	for (size_t i=0; i<n; i++)
	  if (l[i].Index()==idx)
	    return &l[i];

	return NULL;
      }

      /// Checks the integrity of an objectlist and corresponding vector.
      bool CheckObjectIdx(const ObjectList&, const string&, bool) const;

      /// Checks the integrity of seenobject.
      bool CheckSeenObjectIdx(bool sloppy) const {
	return CheckObjectIdx(seenobject, "seenobject", sloppy);
      }

      /// Checks the integrity of uniqueimage.
      bool CheckUniqueObjectIdx(bool sloppy) const {
	return CheckObjectIdx(uniqueimage, "uniqueimage", sloppy);
      }

      /// Checks the integrity of newobject.
      bool CheckNewObjectIdx(bool sloppy) const {
	return CheckObjectIdx(newobject, "newobject", sloppy);
      }

      /// Checks the integrity of seenobject versus other indices.
      bool CrossCheckSeenObjectIdx() const;

    protected:
      ///
      ObjectList seenobject;

      ///
      ObjectList combinedimage;

      ///
      ObjectList uniqueimage;

      ///
      ObjectList newobject;

    }; // class CbirStageBased::QueryData

    //  public:
    /// Forward declaration.
                      // class CbirStageBased::QueryData;

    /// Poisoned constructor.
    CbirStageBased() : CbirAlgorithm() {}

    /// This does not need to do anything else but call the base class
    /// constructor.  If the class has any non-trivial data members
    /// then they might be initialized here.
    CbirStageBased(DataBase*, const string&);

    /// This constructor just creates the "factory" instance of the class.
    CbirStageBased(bool) : CbirAlgorithm(false) {}

    /// Does not need to be defined.
    virtual ~CbirStageBased() {}

    /// This method creates the query-specific data structures.
    virtual CbirAlgorithm::QueryData *CreateQueryData(const CbirAlgorithm *a,
						      const Query *q) const {
      return new QueryData(a, q);
    }
    
    /// Name of the algorithm.  Needs to be defined.
    virtual string Name() const { return "stagebased"; }

    /// A bit longer name.  Needs to be defined.
    virtual string LongName() const { return "stagebased algorithm"; }

    /// Short description  of the algorithm.  Needs to be defined.
    virtual string Description() const {
      return "This is a stage-based algorithm.";
    }
    
    /// Initialization with query instance and arguments from
    /// algorithm=stagebased(arguments) .  Does not need to be defined.
    virtual bool Initialize(const Query*, const string&,
			    CbirAlgorithm::QueryData*&) const;

    /// Something can be done when features have been specified.  Does
    /// not need to be defined.
    virtual bool AddIndex(CbirAlgorithm::QueryData*, Index::State*) const;

    /// Does not need to be defined if the algorithm does not have any
    /// tunable parameters of its own.
    virtual bool Interpret(CbirAlgorithm::QueryData *qd, const string& key,
			   const string& value, int& res) const;

    /** This a new virtual that makes CbirStageBased a pure virtual class.
	\param stage stage whose default function is requested
	\param warn true if unknow function should be warned of
	\return default function of that algorithm at that stage
    */
    virtual cbir_function StageDefault(cbir_stage stage, bool warn) const = 0;

    /// This method implements the actual CBIR algorithm.  Needs to be
    /// defined.
    virtual ObjectList CbirRound(CbirAlgorithm::QueryData*, const ObjectList&,
				 size_t) const;

    ///
    typedef map<enum cbir_stage,string> stage_name_map_t;

    /// Creates and returns association between cbir_stage and string.
    static const stage_name_map_t& StageNameMap();

    /// Returns enum from string.
    static cbir_stage StageName(const string& s) {
      const stage_name_map_t& m = StageNameMap();
      for (stage_name_map_t::const_iterator i = m.begin(); i!=m.end(); i++)
	if (i->second==s)
	  return i->first;
      return stage_unknown;
    }

    /// Returns string from enum.
    static string StageName(cbir_stage s) {
      const stage_name_map_t& m = StageNameMap();
      stage_name_map_t::const_iterator i = m.find(s);
      return i!=m.end() ? i->second : "unknown";
    }

    /// Checks the validity of stage.
    static bool StageOK(cbir_stage s) {
      return s>=stage_first && s<=stage_last;
    }

    /// Tries to set stage function to given value.
    bool SetStageFunction(const string&, const string&, bool = false);

    /// Function to run at that stage.
    cbir_function StageFunction(cbir_stage, bool = false, bool = false,
				bool = true) const;

    /// Access to list of function names and the enum.
    static bool FunctionNames(int i, cbir_function&, const char*&);

    /// Returns enum for string.
    static cbir_function FunctionName(const string&);

    /// Returns enum for string.
    static const char* FunctionName(cbir_function);

    /// Get access to stage_map.
    const cbir_function& StageFunc(cbir_stage s) const /*throw(logic_error)*/ {
      stage_map_t::const_iterator i = stage_map.find(s);
      if (i==stage_map.end())
	throw logic_error("unknown cbir_stage");
      return i->second;
    }

    /// Set access to stage_map.
    void StageFunc(cbir_stage s, cbir_function f) /*throw(logic_error)*/ {
      if (!StageOK(s))
	throw logic_error("unknown cbir_stage");

      stage_map[s] = f;
    }

    /// Get access to stage_arg_map.
    const string& StageArgs(cbir_stage s) const /*throw(logic_error)*/ {
      stage_arg_map_t::const_iterator i = stage_arg_map.find(s);
      if (i==stage_arg_map.end())
	throw logic_error("unknown cbir_stage");
      return i->second;
    }

    /// Set access to stage_arg_map.
    void StageArgs(cbir_stage s, const string& a) /*throw(logic_error)*/ {
      if (!StageOK(s))
	throw logic_error("unknown cbir_stage");

      stage_arg_map[s] = a;
    }

    /// 
    bool StrVsRelop(string& op, relop_type& r, bool setrelop=true) const;

    /// 
    bool StrToRelopStage(const string&, struct relop_stage_type&) const;

    /// 
    enum relop_type StrToRelop(const string& op) const {
      enum relop_type r;
      string str = op;
      if (!StrVsRelop(str,r)) {
	ShowError("Bad operator \"", op, "\" changed to \"none\"");
	return relop_none;
      } 
      return r;
    }    

    /// 
    string RelopToStr(const enum relop_type relop) const {
      enum relop_type r=relop;
      string str;
      if (!StrVsRelop(str,r,false)) 
	return "[error]";
      return str;
    }   
 
    /// 
    int RelopStageToChoice(const struct relop_stage_type rs) const {
      return (rs.up==relop_none?0:RELEVANCE_UP) + 
	(rs.down==relop_none?0:RELEVANCE_DOWN);
    }

    /// 
    void LogRelopStage(CbirStageBased::QueryData* qds,
		       const string& msg, const relop_stage_type& rs) const {
      string mat;
      WriteLog(qds, msg," (UP: "+RelopToStr(rs.up),
	       ", DOWN: "+RelopToStr(rs.down)+")");
    }

    /// Sets debug_check_lists
    static void DebugCheckLists(bool d) { debug_check_lists = d; }

  protected:
    /// Type of stage_map.
    typedef map<cbir_stage,cbir_function> stage_map_t;

    /// Functions to call at each stage.
    stage_map_t stage_map;

    /// Type of stage_arg_map.
    typedef map<cbir_stage,string> stage_arg_map_t;

    /// Arguments to the stage functions.
    stage_arg_map_t stage_arg_map;

    ///
    static bool debug_check_lists;

    /// A utility routine for the virtual methods.
    static CbirStageBased::QueryData *CastData(CbirAlgorithm::QueryData *qd) {
      CbirStageBased::QueryData *qde = static_cast<CbirStageBased::QueryData*>(qd);
      if (!qde) {
	ShowError("CbirStageBased::QueryData pointer conversion failed");
	abort();
      }
      return qde;
    }

    /// Selects initial images randomly.
    bool RandomUnseenObjects(CbirStageBased::QueryData*, size_t, bool, bool) const;

    ///
    virtual cbir_stage StageRestrict(CbirStageBased::QueryData*,
			     cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageEnter(CbirStageBased::QueryData*,
			  cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageInitialSet(CbirStageBased::QueryData*,
			       cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageCheckNimages(CbirStageBased::QueryData*,
					 cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageNoPositives(CbirStageBased::QueryData*,
				cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageHasPositives(CbirStageBased::QueryData*,
				 cbir_function, const string&, size_t) const;

    /// StageSpecialFirstRound() needed only for CbirVQ?
    virtual cbir_stage StageSpecialFirstRound(CbirStageBased::QueryData*,
					      cbir_function func, const string&, size_t) const {
      ShowError("CbirStageBased::StageSpecialFirstRound(): operation for <", 
		FunctionName(func), "> undefined");
      return stage_error; 
    }

    ///
    virtual cbir_stage StageExpandRelevance(CbirStageBased::QueryData*,
					    cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageRunPerMap(CbirStageBased::QueryData*,
				      cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageSetMapWeights(CbirStageBased::QueryData*,
					  cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageCombineUnitLists(CbirStageBased::QueryData*,
					     cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageExtractObjects(CbirStageBased::QueryData*,
                                           cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageCombineObjectLists(CbirStageBased::QueryData*,
					      cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageRemoveDuplicates(CbirStageBased::QueryData*,
					     cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageExchangeRelevance(CbirStageBased::QueryData*,
					      cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageSelectObjectsToProcess(CbirStageBased::QueryData*,
						  cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageProcessObjects(CbirStageBased::QueryData*,
					  cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageConvergeRelevance(CbirStageBased::QueryData*,
					      cbir_function, const string&, size_t) const;

    ///
    virtual cbir_stage StageFinalSelect(CbirStageBased::QueryData*,
					cbir_function, const string&, size_t) const;

    ///
    bool ConditionallyCheckObjectIndices(CbirStageBased::QueryData *qd,
					 bool sloppy) const {
      return !debug_check_lists ||
	(qd->CheckSeenObjectIdx(false) && qd->CheckUniqueObjectIdx(false) &&
         qd->CheckNewObjectIdx(sloppy) && 
         true); // (can_show_seen||qd->CrossCheckSeenObjectIdx());
    }

    ///
    bool Expand(CbirStageBased::QueryData*, const string&) const;
    
    /// Puts some children of seen objects to the end of the seenobject list 
    bool ExpandSelective(CbirStageBased::QueryData*, const string&) const;
    
    ///
    bool Exchange(CbirStageBased::QueryData*, const string&) const;
    
    ///
    bool Converge(CbirStageBased::QueryData*, const string&) const ;

    /**

    */
    class IndexData : public CbirAlgorithm::IndexData {

    }; // classCbirStageBased::IndexData

  }; // class CbirStageBased

} // namespace picsom

#endif // _CBIRSTAGEBASED_H_
