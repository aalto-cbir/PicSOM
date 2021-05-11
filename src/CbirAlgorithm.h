// -*- C++ -*-  $Id: CbirAlgorithm.h,v 2.60 2021/05/11 14:46:57 jormal Exp $
// 
// Copyright 1998-2021 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _CBIRALGORITHM_H_
#define _CBIRALGORITHM_H_

#define RELEVANCE_SKIP   0
#define RELEVANCE_UP     1
#define RELEVANCE_DOWN   2

#include <DataBase.h>
#include <ValuedList.h>

using namespace std;

namespace picsom {
  ///
  static const string CbirAlgorithm_h_vcid =
    "@(#)$Id: CbirAlgorithm.h,v 2.60 2021/05/11 14:46:57 jormal Exp $";

  /**
     Abstract base class for CBIR algorithms, one instance per each database.
     Contains functionality for:
     1) Creating instances of different CBIR algorithms.
     2) Convenient access to data structures needed by the algorithms. 
  */
  class CbirAlgorithm {
  public:
    class QueryData;
    struct parameter;

  protected:
    /** Default constructor. Should not be called. Poisoned.
    */
    CbirAlgorithm() { abort(); }

    /** Principal constructor.
	\param db pointer to the DataBase object, will be stored.
	\param args query-independent constant parameters of the algorithm.
    */
    CbirAlgorithm(DataBase *db, const string& args);

    /** Special constructor to add reference to that instance in
	list_of_algorithms. This will be used by exactly one static
	"factory" instance of each implemented algorithm.
	\param not used.
    */
    CbirAlgorithm(bool) {
      next_of_algorithms = list_of_algorithms;
      list_of_algorithms = this;
    }

  public:
    /** Destructor, doesn't really do anything, but needs to be declared as 
	virtual. 
    */
    virtual ~CbirAlgorithm() {}

    /** Pure virtual method that should call the actual principal 
	constructor.  Called by the "factory" instance of the class.
        Query-independent arguments can be given in a picsom script line 
	as algorithm=name_arguments or algorithm=namearguments.
	\param db pointer to the DataBase object, passed to the constructor.
	\param m query-independent constant parameters for the algorithm.
	\return pointer to created CBIR algorithm instance.
    */
    virtual CbirAlgorithm *Create(DataBase *db, const string& m) const = 0;

    /** Pure virtual method that should call the actual constructor for the
	algorithm- and query-dependent data.
	\param a pointer to the algorithm.
	\param q pointer to the query.
	\return pointer to created algorithm-specific QueryData instance.
    */
    virtual QueryData *CreateQueryData(const CbirAlgorithm *a,
				       const Query *q) const = 0;

    /** Pure virtual method.  Output of this method will be used when searching
        CBIR algorithms by name.
	\return name of the algorithm.
    */
    virtual string BaseName() const = 0;

    /** Pure virtual method.  This is a bit more descriptive version of the
        algorithm's name for human readers.
	\return a bit longer name of the algorithm.
    */
    virtual string LongName() const = 0;

    /** Pure virtual method.  This description should tell something about
        the algorithm so that users can select which algorithm to use.
	\return short description of the algorithm.
    */
    virtual string Description() const = 0;

    /** Virtual method to give exact name of the algorithm.  Typically combines
	BaseName() with some extra query-independent specifiers.
	\return name of the algorithm.
    */
    virtual string FullName() const { return BaseName(); }

    /** Virtual method that returns a list of parameter specifications.
	\return list of parameter specifications.
    */
    virtual const list<parameter>& Parameters() const {
      static list<parameter> empty;
      return empty;
    }

    /** Virtual method to be overwritten if the algorithm needs to interpret
        query-specific arguments given in a picsom script line as 
	algorithm=name(arguments).
	\param q pointer to the Query instance.
	\param args arguments given in parentheses when the algorithm has
	been specified in a picsom script.
	\param data destination for empty QueryData structure.
	\return true if successful.
    */
    virtual bool Initialize(const Query *q, const string& args,
			    QueryData*& data) const;

    /** Virtual method not necessary to be overwritten. Adds a feature
	to the list of used features. Called when the user selects
	features to be used or a picsom script line features=aa,bb,cc
	is processed.
	\param qd query-specific data.
	\param f pointer to Index::State that hold the dynamic part of 
	         the feature index.
	\return true if successful.
    */
    virtual bool AddIndex(QueryData *qd, Index::State *f) const;

    /** Virtual method not necessary to be overwritten. Removes a feature
	from the list of used features. 
	\param qd query-specific data.
	\param i the ordinal number of the feature to be removed
	\return true if successful.
    */
    virtual bool RemoveIndex(QueryData *qd , size_t i) const;

    /** Virtual method.  To be overwritten if the algorithm needs any
        parameters for tuning its operation.
	\param qd query-specific data.
	\param key name of the parameter to be set.
	\param value parameter's value.
	\param res to be set to 0 if key was recognized as algorithm's
	       parameter, but processing the directive failed eg due to
	       bad value
	\return true if  key was recognized as algorithm's parameter and 
	false if it wasn't.
    */
    virtual bool Interpret(QueryData *qd, const string& key,
			   const string& value, int& res) const;

    /** Virtual method that needs to be overwritten.
	\param qd query-specific data.
	\param seen list of objects whose relevance for the query are known.
	\param nobj number of objects to retrieve.
	\return list of objects found as the most relevant ones for the query.
    */
    virtual ObjectList CbirRound(QueryData *qd, const ObjectList& seen,
				 size_t nobj) const;

    /**
     */
    virtual bool ObjectSelectable(size_t /*idx*/) const {
      return true; // could check the target types etc...
    }

    /** Access to the main PicSOM object.  One may safely assume that
	the database pointer is valid and contains a valid pointer to
	the PicSOM object.
    */
    PicSOM *Picsom() const { return database->Picsom(); }

    /** Access to the Query object.
	\param qd query-specific data.
	\return pointer to Query.
    */
    const Query *GetQuery(const QueryData *qd) const { return qd->GetQuery(); }

    /// Pointer to DataBase.
    DataBase *GetDataBase() const { return database; }

    /**
       Returns the number of features in the list of used features.
       \param qd query-specific data.	
       \return the number of features used
    */
    size_t Nindices(const QueryData *qd) const {
      return qd->Nindices();
    }
    
    /**
       Returns the name of a feature in the list of used features.
       \param qd query-specific data.	
       \param i the ordinal number of the feature	      
       \return the name of the feature
    */
    string IndexFullName(const QueryData *qd, size_t i) const {
      return qd->IndexFullName(i);
    }

    /** Returns the ordinal number of a given feature in the list of
       used features, -1 if the given feature is not found in the
       list.
       \param qd query-specific data.	
       \param n name of the feature
       \return the ordinal number of the feature
    */
    int IndexFullNameIndex(const QueryData *qd, const string& n) const {
      return qd->IndexFullNameIndex(n);
    }

    /**
       .
    */

    /// Selects all forced initial images given as argument.
    bool OnlyForcedObjects(const string&) const { 
      return ShowError("CbirAlgorithm::OnlyForcedObjects() not implemented");
    }

    /// Writes strings to log.
    void WriteLog(CbirAlgorithm::QueryData* qd,
		  const string& m1, const string& m2 = "",
		  const string& m3 = "", const string& m4 = "", 
		  const string& m5 = "") const {
      Picsom()->WriteLogStr(LogIdentity(qd), m1, m2, m3, m4, m5); 
    }

    /// Writes contents of a stream to log.
    void WriteLog(CbirAlgorithm::QueryData* qd, ostringstream& os) const {
      Picsom()->WriteLogStr(LogIdentity(qd), os); 
    }

    /// Gives log form of Query identity.
    string LogIdentity(CbirAlgorithm::QueryData* qd) const;

    /** Sets debugging of CBIR algorithm internals on and off.
	\param d true if debugging is desired.
    */
    static void DebugStages(bool d) { debug_stages = d; }

    /** Tells whether debugging of CBIR algorithm internals is on or off.
	\return true if debugging is enabled.
    */
    static bool DebugStages() { return debug_stages; }

    /// Sets debug_subobjects
    static void DebugSubobjects(bool d) { debug_subobjects = d; }

    /// Sets debug_aspects
    static void DebugAspects(bool d) { debug_aspects = d; }

    /// Sets debug_selective
    static void DebugSelective(bool d) { debug_selective = d; }

    /// Sets debug_weights
    static void DebugWeights(bool d) { debug_weights = d; }

    /// Sets debug_classify
    static void DebugClassify(bool d) { debug_classify = d; }

    /// Sets debug_classify
    static void DebugPlaceSeen(bool d) { debug_placeseen = d; }

    /// Sets debug_lists
    static void DebugLists(int d) { debug_lists = d; }

    /// Splits "foo_gnat=gnu_bar" stringified argument string.
    static map<string,string> SplitArgumentString(const string&);

    /** Produces a list of the BaseName()s of all know algorithms.
	\return list of algorithm names.
    */
    static list<string> AlgorithmList() {
      list<string> l;
      for (const CbirAlgorithm *p = list_of_algorithms; p;
	   p=p->next_of_algorithms)
	l.push_back(p->BaseName());
      return l;
    }

    /** Prints a list of the registered algorithms
	\param os the output stream to print to (default cout).
	\param wide set to true if wide output is prefered.
    */
    static void PrintAlgorithmList(ostream& os = cout, bool wide = false);

    /** Finds a "factory" instance of a CBIR algorithm specified by name, which
	should be BaseName() of the algorithm appended with some extra 
	query-independent specifiers.
	\param name of the algorithm.
	\return pointer to the "factory" instance of the class.
    */
    static const CbirAlgorithm *FindAlgorithm(const string& n, string& a)
      /*throw(string)*/ {
      a = "";
      const CbirAlgorithm *found = NULL;
      for (const CbirAlgorithm *p = list_of_algorithms; p;
	   p = p->next_of_algorithms) {
	if (n.find(p->BaseName())==0) {
	  if (found)
	    throw string("more than one algorithm matches \"")+n+"\"";
	  else {
	    found = p;
	    a = n.substr(p->BaseName().size());
	    while (a[0]=='_')
	      a.erase(0, 1);
	  }
        }
      }
      return found;
    }

    /** A workhorse to first find the correct "factory" instance by name and
	then to use it to create a real functional instance of the class.
	\param db pointer to the DataBase object, passed to the constructor.
	\param n name and query-independent parameters of the algorithm.
	\return pointer to created CBIR algorithm instance.
    */
    static CbirAlgorithm *FindAndCreateAlgorithm(DataBase *db,
						 const string& n) /*throw(string)*/;

    /// Combines values from subobject back to their parent.
    bool RelevanceUp(CbirAlgorithm::QueryData*,
		     const string &, struct relop_stage_type&, 
		     ObjectList&, bool divide, bool seenimg) const;

    /// Combines values from parents to subobjects.
    bool RelevanceDown(CbirAlgorithm::QueryData*,
		       const string&, struct relop_stage_type& , 
		       ObjectList&, bool divide, bool seenimg) const;

    /// Combines values from subobjects up to parents and then down again
    /// to the other subobjects.
    bool RelevanceUpDown(CbirAlgorithm::QueryData*,
			 const string& /*s*/, struct relop_stage_type&, 
			 ObjectList& il, bool divide, bool seenimg) const;

    ///
    void PushRelevanceUp(CbirAlgorithm::QueryData*,
			 size_t i, struct relop_stage_type&,
			 ObjectList& il, double val,map<int,int> &idx, 
			 bool divide, bool seenimg) const;
    ///
    void PushRelevanceDown(CbirAlgorithm::QueryData*,
			   size_t i, struct relop_stage_type&,
			   ObjectList& il, double val, map<int,int> &idx,
			   bool divide, bool seenimg, int sibling_i) const;
    /// 
    bool CanBeShownRestricted(CbirAlgorithm::QueryData* qd, int idx,
			      bool checkparents) const; 

    /// Sorts the seenobject array.
    void SortSeen() const {}

    void ZeroRelevanceCounters(ObjectList& il) const {
      for (size_t i=0; i<il.size(); i++) 
	il[i].ZeroRelevanceCounters();
    }

    void SumRelevanceCounters(ObjectList& il, bool set_to_one=false) const {
      for (size_t i=0; i<il.size(); i++) 
	il[i].SumRelevanceCounters(set_to_one);
    }

    void AllSeenReally(ObjectList& il) const {
      for (size_t i=0; i<il.size(); i++) 
	il[i].not_really_seen = false;
    }
    
    /// Creates a list of all aspects in selected features.
    map<string,aspect_data> 
    AspectsFromIndices(CbirAlgorithm::QueryData*) const;

    ///
    size_t DataBaseSize() const { return database->Size(); }

    ///
    size_t DataBaseRestrictionSize(target_type tt) const {
      return database->RestrictionSize(tt);
    }

    ///
    void Tic(const string&) const {}

    ///
    void Tac(const string&) const {}

  protected:
    /// Pointer to the DataBase object that holds all data related to
    /// the objects of the collection.
    DataBase *database;

    /// Set to true if you want to see debug information. Defaults to false.
    static bool debug_stages;

    /// True if you want to see how subobjects are treated.
    static bool debug_subobjects;

    /// True if you want to see how aspects are treated.
    static bool debug_aspects;

    /// True if you want to see how ...
    static bool debug_selective;

    /// True if you want to see how ...
    static bool debug_weights;

    /// True if you want to see how ...
    static bool debug_classify;

    /// True if you want to see how ...
    static bool debug_placeseen;

    /// True if you want to see how ...
    static int debug_lists;

    static bool relevance_to_siblings;
    static struct relop_stage_type expand_operation;
    static struct relop_stage_type exchange_operation;
    static struct relop_stage_type converge_operation;

    /// Combination factor for relevance combination between objects,
    /// used in PushRelevanceDown().
    static float comb_factor;

  private:
    /// Points to the head of a linked list of "factory" instances.
    static CbirAlgorithm *list_of_algorithms;
    
    /// Forms the chain of "factory" instances of available methods.
    CbirAlgorithm *next_of_algorithms;

  public:
    /**
       As there exists one CbirAlgorithm instance per each database the
       query-specific data is stored in this object.
    */
    class QueryData {
      ///
      friend class CbirAlgorithm;

    public:
      /// poison
      QueryData() { abort(); }

      ///
      QueryData(const CbirAlgorithm *a, const Query *q) : 
	algorithm(a), query(q), maxquestions(-1), random_image_count(0) {}

      ///
      virtual ~QueryData() {
	while (features.size())
	  RemoveIndex(0);
      }

      ///
      virtual QueryData *Copy(const Query *q) const = 0;

      ///
      void CopyIndexData() {
	for (size_t i=0; i<features.size(); i++)
	  features[i] = features[i]->MakeCopy(false);
      }

      /// Access to the Query object.
      const Query *GetQuery() const { return query; }

      /// Access to the CbirAlgorithm object.
      const CbirAlgorithm *GetAlgorithm() const { return algorithm; }

      /// Access to the DataBase object.
      const DataBase *GetDataBase() const { return algorithm->GetDataBase(); }

      /// Access to object labels by index.
      const string& Label(size_t i) const;

      /**
	 .
      */
      bool AddIndex(Index::State *f) {
	features.push_back(f);
	return true;
      }

      /**
	 .
      */
      bool RemoveIndex(size_t);

      ///
      Index::State& IndexData(size_t i) {
	if (!IndexIndexOK(i)) {
	  stringstream s;
	  s << "i=" << i << " Nindices()=" << Nindices();
	  ShowError("CbirAlgorithm::IndexData(size_t) index error! : ",
		    s.str());
	  TrapMeHere();
	}
	return *features[i];
      }
      
      ///
      const Index::State& IndexData(size_t i) const {
	return ((QueryData*)this)->IndexData(i);
      }

      ///
      const Index& IndexStaticData(size_t i) const {
	return *IndexData(i).StaticPart();
      }

      /// fixme: ideally this should not be needed...
      Index& IndexStaticData(size_t i) {
	return *IndexData(i).StaticPart();
      }

      /// Returns true if if valid index to feature_data.
      bool IndexIndexOK(size_t i) const { return i<Nindices(); }

      /**
	 .
      */
      size_t Nindices() const {
	return features.size();
      }

      /// Returns size of vqunits_xyz.
      int PerMapObjectsSize() const {
	size_t s = 0;
	for (size_t i=0; i<Nindices(); i++)
	  s += IndexData(i).objlist.size();
	return (int)s;
      }

      /// One argument interface to permapnewobject.
      ObjectList& PerMapNewObject(int ii) {
	// this might be slow...
	size_t i = (size_t)ii;
	for (size_t m=0; m<Nindices(); m++)
	  if (i<IndexData(m).objlist.size())
	    return IndexData(m).objlist[i];
	  else
	    i -= IndexData(m).objlist.size();
	static ObjectList empty;
	return empty;
      }

      /// const-const version of the one above.
      const ObjectList& PerMapNewObject(int i) const {
	return ((QueryData*)this)->PerMapNewObject(i);
      }

      ///
      ObjectList& PerMapNewObject(int i, int j) {
	return IndexData(i).objlist[j];
      }

      ///
      const ObjectList& PerMapNewObject(int i, int j) const {
	return ((QueryData*)this)->PerMapNewObject(i, j);
      }
      
      /// Name of feature by index.
      const string& IndexShortName(size_t i) const {
	if (!IndexIndexOK(i)) {
	  static const string qms = "???";
	  ShowError("Query::IndexShortName(size_t) index failure");
	  return qms;
	}
	return IndexStaticData(i).Name();
      }

      /// Finds index of named feature in tssom or -1 if not used.
      int IndexShortNameIndex(const string& n) const {
	for (size_t i=0; i<Nindices(); i++)
	  if (n==IndexShortName(i))
	    return i;
	return -1;
      }

      /**
	 .
      */
      string IndexFullName(size_t i) const {
	return i<=features.size() ? features[i]->fullname : "";
      }

      /**
	 .
      */
      int IndexFullNameIndex(const string& n) const {
	for (size_t i=0; i<features.size(); i++)
	  if (n==features[i]->fullname)
	    return i;
	return -1;
      }

      /**
         .
      */
      list<string> IndexFullNameList() const {
        list<string> ret;
	for (size_t i=0; i<features.size(); i++)
	  ret.push_back(features[i]->fullname);
	return ret;
      }

      ///
      bool IsTsSom(size_t i) const {
	return IndexIndexOK(i) && IndexData(i).Is("tssom");
      }

      ///
      bool IsTsSomAndLevelOK(size_t i, size_t j) const {
	return IsTsSom(i) && TsSom(i).LevelOK(j);
      }

      ///
      bool IsSvm(size_t i) const {
        return IndexIndexOK(i) && IndexData(i).Is("svm");
      }

      ///
      bool IsWordInverse(size_t i) const {
	return IndexIndexOK(i) && IndexData(i).Is("wordinverse");
      }

      /// 
      bool IsInverseIndex(size_t i) const {
	return IsWordInverse(i);
      }

      /// 
      bool IsPreCalculatedIndex(size_t i) const {
	return IndexIndexOK(i) && IndexData(i).Is("precalculated");
      }

      ///
      TSSOM::State& IndexDataTSSOM(size_t i) {
	Index::State *s = &IndexData(i);
	TSSOM::State *p = dynamic_cast<TSSOM::State*>(s);
	if (!p)
	  ShowError("CbirPicsom::QueryData::IndexDataTSSOM() ",
		    "failing with i="+ToStr(i)+" fullname=<"+s->fullname+">");
	return *p;
      }

      ///
      const TSSOM::State& IndexDataTSSOM(size_t i) const {
	return ((QueryData*)this)->IndexDataTSSOM(i);
      }

      /// Gives reference to any TSSOM map.
      TSSOM& TsSom(size_t i) { 
	return IndexDataTSSOM(i).TsSom(); 
      }

      /// Gives reference to any TSSOM map.
      const TSSOM& TsSom(size_t i) const {
	return IndexDataTSSOM(i).TsSom();
      }

      /// Gives reference to any particular level of TreeSOM.
      TreeSOM& Map(int i, int j) { return TsSom(i).Map(j); }
      
      /// Gives reference to any particular level of TreeSOM.
      const TreeSOM& Map(int i, int j) const { return TsSom(i).Map(j); }
      
      /// Gives reference to any particular level of TreeSOM even alien data.
      const TreeSOM& MapEvenAlien(int i, int j) const {
	return TsSom(i).MapEvenAlien(j);
      }

      /// "Canonical name" of a given feature map and its layer.
      string MapName(bool db_too, int m, int l = -1, int k = -1) const {
	return TsSom(m).MapName(db_too, l, k);
      }

      ///
      int Nlevels(int m) const {
	return IsInverseIndex(m) || IsPreCalculatedIndex(m) ?
	  1 : TsSom(m).Nlevels();
      }

      ///
      int NlevelsEvenAlien(int m) const {
	return IsInverseIndex(m) || IsPreCalculatedIndex(m) ?
	  1 : TsSom(m).NlevelsEvenAlien();
      }

      /** Solves the number of objects the CBIR is requested to return.
	  If variable maxquestions has been modified from its default value -1,
	  then that value is returned, otherwise the argument value. 
	  Special value 0 means that all objects of the database should be 
	  returned in the order of decreasing relevance.
	  \return number of requested objects.
      */
      size_t GetMaxQuestions(size_t mq) const {
	return maxquestions==-1 ? mq : (size_t)maxquestions;
      }

      /** Sets the number of objects the user wants the CBIR algorithm 
          to return.
	  \param maxq number of objects. 
      */
      void SetMaxQuestions(int maxq) {
	maxquestions = maxq;
      }

      /// Returns round number.
      int Round() { return round; }
      
      /// Sets round counter to zero.
      void ZeroRound() { round = 0; }
      
      /// Increments round counter by one.
      void IncrementRound() { round++; }

      /// Produces header line for unit/image dumps.
      string ListHead(const string& txt, const string& fns, int m, int l) const;

      /// Common part to all object dumps.
      void DumpList(const ObjectList& list, const string& txt, bool n = false,
		    const string& fn = "", int m = -1, int l = -1) const {
	Object::DumpList(list, ListHead(txt, fn, m, l), "-------------", n);}
    
      /// Common part to all vqunit dumps.
      void DumpList(const VQUnitList& list, const string& txt, bool n = false,
		    const string& fn = "", int m = -1, int l = -1,
		    int mapw=-1) const {
	VQUnit::DumpList(list, ListHead(txt, fn, m, l), "-------------",
			 n,mapw);
      }

      /**This virtual function can be used to turn on class model augmentation 
	 for a certain concept and a certain feature. Does nothing in the base class. 
	  \param value of the class model (usually +1 or -1)
	  \param classname
	  \param featurename
      */
      virtual void SetClassAugmentValue(const float value,
					const string& classname="", 
					const string& featurename="") {
	if (debug_classify)
	  cout << "Running (empty) CbirAlgorithm::QueryData::SetClassAugmentValue() : "
	       << classname << (featurename.empty()?"":",")
	       << featurename << ") = " << value << endl;
      }
    
    private:
      ///
      const CbirAlgorithm *algorithm;

      ///
      const Query *query;

      ///
      vector<Index::State*> features;

    protected:
      ///
      void SetQuery(const Query *q) { query = q; }

      /// Stored list of seen objects with relevance values.
      // ObjectList seenobjects;

      /// Maximum number of question images to be returned.  Defaults to
      /// -1 which means that the number of objects specified by the second
      /// argument of the CbirRound() method call shall be obeyed.  Any other
      /// value than -1 will override the second argument of CbirRound().
      /// Special value 0 means that ALL objects of the database shall be
      /// ordered and returned.
      int maxquestions;

      /// Count of random images in newobject.
      int random_image_count;

      /// 
      int round;

    }; // class CbirAlgorithm::QueryData

    /**
       As there exists one CbirAlgorithm instance per each database the
       feature-specific data is stored in this object.
    */
    class IndexData {

    }; // class CbirAlgorithm::IndexData

    /**
       Description of specifiable parameters for the algorithm.
    */
    struct parameter {
      ///
      string name;

      ///
      string type;

      ///
      string range;

      ///
      string defval;

      ///
      string description;
    }; // struct parameter

  }; // class CbirAlgorithm

} // namespace picsom

#endif // _CBIRALGORITHM_H_
