// -*- C++ -*-  $Id: Query.h,v 2.337 2019/03/20 13:26:05 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_QUERY_H_
#define _PICSOM_QUERY_H_

#include <PicSOM.h>
#include <Connection.h>
#include <Valued.h>
#include <DataBase.h>
#include <CbirAlgorithm.h>

#include <WordInverse.h>
#include <PreCalculatedIndex.h>
#include <SVM.h>

#include <videofile.h>
//#include <uiart-state.h>

#include <RandVar.h>

#ifdef SIMPLE_USE_PTHREADS
#define QUERY_USE_PTHREADS
#endif // SIMPLE_USE_PTHREADS

#define RELEVANCE_SKIP   0
#define RELEVANCE_UP     1
#define RELEVANCE_DOWN   2

namespace picsom {
  using simple::FloatVectorSet;
  using simple::DoubleVector;
  using simple::SOM;
  using simple::IntMatrix;
  using simple::RandVar;

  static const string Query_h_vcid =
    "@(#)$Id: Query.h,v 2.337 2019/03/20 13:26:05 jormal Exp $";

  /**
     documentation missing
  */
  typedef double (*ConvolutionFunction)(double, double, double);

  /**
     documentation missing
  */
  typedef pair<string,pair<ground_truth,float> > ground_truth_list_e;
  typedef list<ground_truth_list_e> ground_truth_list;

  typedef vector<pair<string,string> > response_list;

  /////////////////////////////////////////////////////////////////////////////

  /**
     documentation missing
  */
  enum hitstype_t {
    unknown,
    trad,
    smoothed_fraction,
    priori_compensated_fraction,
    smoothed_log,
    posneg_lc,
    bernoullimap
  };

/**
   The Query class contains the query processing functionality for the 
   PicSOM image retrieval system. 

   Command line arguments:
   @verbinclude cmdline-io.dox
   
   @short A class implementing query processing in the PicSOM engine. 

   @version $Id: Query.h,v 2.337 2019/03/20 13:26:05 jormal Exp $

*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  class Query {
  public:
    /// This reads from a file or just initializes.
    Query(PicSOM*, const string& = "", const Connection* = NULL,
          Analysis* = NULL);

    /// This constructor is poisoned.
    Query(const Query&) { SimpleAbort(); }
    
    /// More poison.
    Query() { SimpleAbort(); }

    /// The destructor.
    virtual ~Query();

    /// More poison.
    Query& operator=(const Query&) { SimpleAbort(); return *this; }

    /// Dump is required in all descendants of Simple.
    virtual void Dump(Simple::DumpMode = Simple::DumpDefault,
                      ostream& = cout) const;

    /// Makes first things.
    void Initialize(PicSOM*);

    /// Gives the "unique" identifier of this object.
    const string& Identity() const { return identity; }

    /// Generates and returns the server-specific part of the identity string.
    string IdentityServerPart();

    /// Sets the identity to unique value.
    void SetIdentity(const string& s = "");

    /// Sets the identity to dummy value.
    void SetDefaultQueryIdentity() { SetIdentity("* default query *"); }

    /// Link to the surroundings.
    PicSOM *Picsom() const { return picsom; }

    /// Returns true if used by analysis with analyse=create.
    bool IsInsert() const;

    /// Returns true if used by analysis with analyse=insert.
    bool IsCreate() const;

    /// Returns true if used by analysis, other than create or insert.
    bool IsAnalyse() const;

    /// Returns true if this equals to db's defaultquery.
    bool IsDefaultQuery() const {
      return the_db && &the_db->DefaultQuery()==this;
    }

    Query *DefaultQuery() const { return &the_db->DefaultQuery(); }

    /// Pointer to Analysis if exists.
    Analysis *GetAnalysis() const { return analysis; }

    /// True if Analysis exists.
    bool HasAnalysis() const { return GetAnalysis(); }

    /// Setting of Analysis.
    void SetAnalysis(Analysis *a) { analysis = a; }

    /// Pointer to Analysis in recursive parents if exists.
    Analysis *GetParentAnalysis() const {
      return analysis ? analysis :
        parent ? parent->GetParentAnalysis() : NULL;
    }

    /// True if Analysis exists in any parent.
    bool HasParentAnalysis() const { return GetParentAnalysis(); }

    /// Returns true if inside RunPartThree() in some thread.
    bool InRun() const { return inrun; }

    /// Waits.
    bool WaitUntilReady() const;

    /// Returns true if query has sane identity.
    bool SaneIdentity() const { return SaneIdentity(Identity()); }

    /// Returns true if argument possibly is a valid identity.
    static bool SaneIdentity(const string&);

    /// Removes %3A -type escapes from identity string.
    static string UnEscapeIdentity(const string&);

    /// Used in Connection::HttpServerSolveRedirection() and
    /// Query::CreateChildWith_gotopage().
    Query *CreateChild(Connection*);

    /// Sets link to parent query and vice versa.
    void Parent(Query *p) {
      if (parent)
        ShowError("Query::Parent() : parent already exists");
      parent = p;
      if (p)
        p->AppendChild(this);
    }

    /// Link to parent.
    Query *Parent() const { return parent; }

    /// True if this is a root in a query tree.
    bool IsRoot() const { return !parent; }

    /// Link to root, ie. grand-grand-grand-...-parent.
    Query *Root() const {
      for (Query *r = (Query*)this;; r=r->parent)
        if (!r->parent)
          return r;
    }

    /// Root of identity.
    static string RootIdentity(const string&);

    /// Links to children.
    Query *Child(size_t i) const { return child_new[i]; }

    /// Number of children.
    size_t Nchildren() const { return child_new.size(); }

    /// Adds a new-born child.
    void AppendChild(Query *c) { child_new.push_back(c); }

    /// Prunes a child away.
    bool DeleteChild(Query*);

    /// Returns true if query is allowed to continue from this.
    bool CheckPermissions(const Query&, const Connection&) const;

    /// Returns true if connection is allowed to continue from this.
    bool CheckPermissions(const Connection&) const;

    /// Finds itself of a child by identity.
    Query *Find(const string&, bool);

    /// Finds the last child in the chain of first children.
    Query *FindLastChild() {
      Query *q = this;
      while (q->Nchildren())
        q = q->Child(0);
      return q;
    }

    /// Copies some variables from an earlier query instance.
    void CopySome(const Query&, bool, bool, bool);

    /// Sets the (human-readable) name of the query.
    void SetName(const string& n) { name = n; }

    /// Gets the (human-readable) name of the query.
    const string& GetName() const { return name; }

    /// True if first client's address has been set.
    bool FirstClientSet() const { return first_client.IsSet(); }

    /// Sets the address of the first client.
    void FirstClientAddress(const string& a) { first_client.Address(a); }

    /// Address of the first client.
    const string& FirstClientAddress() const { return first_client.Address(); }

    /// Reference to the first client.
    const RemoteHost& FirstClient() const { return first_client; }

    /// Reference to the first client.
    RemoteHost& FirstClient() { return first_client; }

    /// All command interpretation is performed here.
    bool ProcessControlLine(const char*, const Connection*, Analysis*, bool);

    ///  
    void ParseResponseList(xmlNodePtr, const string&, const string&, 
                           response_list&);

    ///
    bool ProcessResponse(const string&, const string&);

    /// Locates <imageresponselist> and calls ProcessObjectInfo()
    bool ProcessObjectResponseList(xmlNodePtr node);

    /// Command interpretation when command start with +, - or 0.
    void ProcessObjectInfo(const string&, const string&, bool);

    ///
    bool ProcessTextQueryOptions(xmlNodePtr node);

    ///
    bool ProcessClassStates(xmlNodePtr node);
        
    ///
    void ProcessClassState(const string&, const string&);

    /** Locates <selectedaspects> and calls ProcessAspectInfo() for each
     * <aspect>. ProcessObjectResponseList() must be called before this
     * function to ensure that the list of seen objects is up to date.
    */
    bool ProcessSelectedAspects(xmlNodePtr);

    /// Sets the given aspect as relevant for the given object.
    bool ProcessAspectInfo(const string&, int, double, const string& = "",
                           const vector<string>& = vector<string>());

    /// Clears the aspect relevance for all the seen objects
    void ClearAspectRelevance();

    /// Sets aspect weights from aspects=foo(weight=0.5),bar(weight=1.5)
    bool SetAspectWeights(const string&);

    /// Returns true if aspect weights have been set.
    bool HasAspectWeights() const { return aspect_weight.Size()!=0; }

    /// Returns aspect's weight. 
    /// 1 if none specified, 0 if the one not specified
    double GetAspectWeight(const string& a) const {
      if (!HasAspectWeights())
	return 1.0;

      if (aspect_weight.HasAspectRelevance(a))
	return aspect_weight.AspectRelevance(a);
      
      return 0.0;
    }

    /// Runs a Google Custom Search
    bool DoGoogleImageSearch(const string&, size_t);

    /// Runs a Google Custom Search
    list<string> GoogleCustomSearch(const string&, const string&, size_t);

    /// Process text query string
    bool ProcessTextQuery();

    /// Called by ProcessTextQuery() if add_classfeatures
    bool AddClassFeatures(const string&, vector<string>&);

    /** Joins a vector of words into one string separated by spaces
     *  @param words vector of words
     *  @param preprocess set to true if words should be preprocessed
     */
    string JoinWords(vector<string>& words, bool preprocess=false);

    /** Check if given string is a proper name
     *  @param proper_string string to check
     *  @param is_person will be set to true if person name
     *  @param shortest_dist will be set to word distance to correction, 
     *         -1 if not found
     *  @return the corrected word if correction needed
     */
    string CheckProperName(const string& proper_string,
                           bool& is_person, double& shortest_dist);

    /** Find closest word in wordlist.
     *  @param wordlist list of words to search through
     *  @param word word to search for
     *  @param dist maximum distance, will be set to distance of closest word
     *  @return the closest word, empty string if nothing found
     */
    string ClosestWord(const set<string>& wordlist, const string& word,
                       double& dist);

    /** Check is given word is in commons file
     *  @param cword word to check
     *  @param found_classes will be set to set of found classes
     *  @return true if successfull
     */
    bool CheckCommons(string cword, set< pair<string,string> >& found_classes);

    ///
    bool CheckConcepts(const string&, const string&,
		       set<pair<string,string> >&);

    /** Calculates the number of modifications that are needed to transform
     *  a into b.
     */
    double LevenshteinDistance(const string&, const string&);

    /// Returns the distance between a and b. Currently uses Levenshtein dist.
    double CalculateWordDistance(const string &a, const string &b) {
      return LevenshteinDistance(a,b);
    }
    
    /// A caching interface to DataBase::ReadOriginsInfo().
    const map<string,string>& OriginsInfo(size_t, bool);

    /// This tries to solve a key=value pair in query.
    /// returns true if key was recognized, false otherwise.
    /// third argument is 1 for OK, 0 for ERROR, -1 for undefined.
    bool Interpret(const string&, const string&, int&, const Connection*);

    /// A helper for Interpret().
    static bool InterpretIndex(const string& s, size_t& i);

    ///
    bool InterpretDefaults(int);

    /// Adds query information to the XML document.
    bool AddToXML(XmlDom& dom, object_type ot, const string& a, 
                  const string& b, const Connection *c);

    /// Adds queryinfo to the XML document.
    bool AddToXMLqueryinfo(XmlDom&, bool, bool, bool, bool,
                           const Connection*) /*const*/;

    /// Adds variables part to the XML document.
    bool AddToXMLvariables(XmlDom&, const Connection*) const;

    /// Adds query-dependent feature/TS-SOM information to the XML document.
    bool AddToXMLfeatureinfo_dynamic(XmlDom&, const Index::State*) const;

    /// Adds query-dependent feature information to the XML document.
    bool AddToXMLfeatureinfo(XmlDom&, size_t) const;

    /// Adds query-dependent object information to the XML document.
    bool AddToXMLobjectinfo_dynamic(XmlDom&, int);

    ///
    bool AddToXMLclassaugment(XmlDom&) const;
    
    /// Adds ground truth based data to the XML document.
    bool AddToXMLselection(xmlNodePtr parent, xmlNsPtr ns,
                           const string&, const string&) const;

    /// Adds statistics part to the XML document.
    bool AddToXMLstatistics(XmlDom&) const;

    /// Adds childlist part to the XML document.
    bool AddToXMLchildlist(XmlDom&, bool, bool, bool, bool,
                           const Connection*) const;

    /// Adds imagelist part to the XML document.
    bool AddToXMLobjectlist(XmlDom&, const Connection*, bool, bool) /*const*/;

    /// Adds ajax_data_all to the XML document.
    bool AddToXMLajaxdata(XmlDom&);

    /// Adds ajaxresponse part to the XML document.
    bool AddToXMLajaxresponse(XmlDom&, const Connection*);

    ///
    bool AddToXMLerfdata(XmlDom&);

    /// Adds images part to the XML document.
    bool AddToXMLobjects(XmlDom&, const Connection*, bool, bool) /*const*/;

    /// Adds guessed or deduced keyword to the XML document.
    bool AddToXMLguessedkeywords(XmlDom&) const;

    /// Adds mapimage part to the XML document.
    bool AddToXMLmapimage(XmlDom&, const string&, const string&) const;

    /// Helper for the above one.
    string MapImageString(const string&, const string&) const;

    /// Adds mapvalues part to the XML document.
    bool AddToXMLmapvalues(XmlDom&);

    /// Adds query visualizations.
    bool AddToXMLqueryimage(XmlDom&, const string&, const string&) const;

    /// Adds infolinks.
    bool AddToXMLinfolinklist(XmlDom&, const string&, const string&) const;

    /// Adds infolinks.
    bool AddToXMLinfolinklist_detectedobject(XmlDom&, const string&,
					     const string&) const;

    /// Adds infolinks.
    bool AddToXMLinfolinklist_detectedobject_xml(XmlDom&, const string&,
						 const string&) const;

    /// Adds infolinks.
    bool AddToXMLinfolinklist_detectedobject_sparql(XmlDom&, const string&,
						    const string&) const;

    /// Helper for the above one.
    static string AddToXMLinfolinklist_macro(const map<string,string>&, const string&);

    /// Adds visited links.
    bool AddToXMLvisitedlinklist(XmlDom&, const string&, const string&) const;

    ///
    bool AddVisitedLink(const string& u, const string& m, size_t s,
			const struct timespec& t) {
      visited_links.push_back(visited_link_t(u, m, s, t));
      return true;
    }

    /// Stores a temporal key=value pair.
    bool AddOtherKeyValue(const string&, const string&);

    ///
    void RemoveOtherKeyValue(const string& k) { keyvaluelist.erase(k); }

    /// Dumps all stored key=value pairs.
    void DumpKeyValues() const;

    /** Returns (and generates if NULL) the list of all the selected objects 
     *  in the query tree.
     */
    set<string> *QueryTreeFormerSelections();
    
    /// Adds the object to the list of all the selected objs in the query tree
    void AddToQueryTreeFormerSelections(int);

    /// Simple notation converter.
    static double MarkCharToValue(char ch) {
      return ch=='+' ? 1.0 : ch=='-' ? -1.0 : 0.0;
    }

    /// Marks one image with given value.
    bool MarkAsSeenEither(int, double, bool, DataBase* = NULL);

    /// Marks one image with given value.
    bool MarkAsSeenNoAspects(int i, double v, DataBase *d = NULL) {
      return MarkAsSeenEither(i, v, false, d);
    }

    /// Marks positives and negatives from ternary class membership vectors.
    bool MarkAsSeenNoAspects(const ground_truth*, const ground_truth*,
                             DataBase*);

    /// Marks positives and negatives from ternary class membership vectors.
    bool MarkAsSeenAspects(const aspect_map_t&, const aspect_map_t&,DataBase*);

    /// 
    bool MarkAsSeenEmptyAspect(const ground_truth *p,
                               const ground_truth *n,
                               DataBase *db);

    /// Helper for the one above.
    bool MarkAspectRelevance(const string&, const ground_truth&, double);

    /// Helper for the one above.
    void RemoveEmptyAspectFromSeen();

    /// Creates a list of all aspects in selected features.
    map<string,aspect_data> AspectsFromIndices() const;

    /// Removes one object from new list.
    bool RemoveFromNew(int);

    /// Removes one image from seen list.
    bool RemoveFromSeen(int);

    /// Marks all unmarked new images with +/-/0.
    bool MarkAllNewObjectsAsSeen(char, bool);

    /// Marks all unmarked new images with value and aspect.
    bool MarkAllNewObjectsAsSeenAspect(float, const string&);

    /// Changes all seen images from one type to another.
    bool ChangeAllSeenObjects(char, char, bool);

    /// Removes images with non-genuine relevance from seenobject
    void RemoveNongenuineFromSeen();

    void AllSeenReally() {
      for (size_t i=0; i<NseenObjects(); i++) 
        seenobject[i].not_really_seen = false;
    }

    /// Solves the index where label should be or is placed.
    int PlaceInSeen_idx(size_t, bool&, int = MAXINT) const;

    /// Solves the index where label should be or is placed.
    int PlaceInSeen_lab(const char*, bool&, int = MAXINT) const;

    /// Solves the index where label should be or is placed.
    int PlaceInSeenReal(const char*, bool&, int = MAXINT) const;

    /// Sorst the seenobject array.
    void SortSeen();

    /// Adds new object to seenobject.
    Object *AddToSeen(Object*);

    /// Adds new object to seenobjectreal.
    Object *AddToSeenReal(Object*);

    /// Returns the number of seen images marked with given sign (-/0/+).
    size_t NSeenObjects(int, target_type) const;

    /// Returns the number of positive-marked seen images.
    int NpositiveSeenObjects() const {
      return NSeenObjects(1, target_any_target);
    }

    /// Returns the number of zero-marked seen images.
    int NzeroSeenObjects() const {
      return NSeenObjects(0, target_any_target);
    }

    /// Returns the number of negative-marked seen images.
    int NnegativeSeenObjects() const {
      return NSeenObjects(-1, target_any_target);
    }

    /// Returns the number of positive-marked seen query_target type objects.
    int NpositiveSeenTargetObjects() const {
      return NSeenObjects(1, Target());
    }

    class algorithm_data {
    public:
      ///
      size_t index;

      ///
      string fullname;

      ///
      const CbirAlgorithm *algorithm;

      ///
      CbirAlgorithm::QueryData *data;

      ///
      ObjectList result;
    }; // class algorithm_data

    ///
    size_t NlowerAlgorithms(const CbirAlgorithm::QueryData*) const;

    ///
    const algorithm_data* 
    LowerAlgorithm(const CbirAlgorithm::QueryData*, size_t, bool = true) const;

    ///
    const vector<algorithm_data>& Algorithms() const {
      return algorithms;
    }

    ///
    vector<algorithm_data>& Algorithms() {
      return algorithms;
    }

    ///
    bool HasAlgorithm() const {
      return algorithm!=cbir_no_algorithm || !algorithms.empty();
    }

    ///
    bool HasAlgorithmAndFeatures() const {
      if (algorithm!=cbir_no_algorithm)
        return Nfeatures();

      if (algorithms.empty())
        return false;

      return algorithms[0].algorithm->Nindices(algorithms[0].data);
    }

    /// Whether the current page should skip the standard CBIR query processing.
    bool SkipOptionPage() const { return skipoptionpage; } 

    ///
    void SkipOptionPage(bool sop) { skipoptionpage = sop; } 

    /// Unselects all features.
    void UnselectIndices(algorithm_data*, bool silent = false);

    /// Selects features by comma-separated list.
    bool SelectIndices(algorithm_data *a, const string& s,
                       bool create = false, bool unselect = true) {
      vector<string> l = SplitInCommasObeyParentheses(s);
      return SelectIndices(a, l, create, unselect);
    }

    /// Selects features by given vector of strings.
    bool SelectIndices(algorithm_data*, const vector<string>&, 
                       bool create = false, bool unselect = true);

    /// Selects features by given list of strings.
    bool SelectIndices(algorithm_data *a, const list<string>& l,
                       bool create = false, bool unselect = true) {
      return SelectIndices(a, vector<string>(l.begin(), l.end()),
                           create, unselect);
    }

    /// Selects one feature.  If name is preceded by dash, just finds it.
    bool SelectIndex(algorithm_data*, const string&,
		     bool create = false);

    /// Gives a vector of index/feature names. 
    vector<string> SelectedIndices(algorithm_data*);
    
    ///
    void ShowFeatures() /*const*/;

    /// Pointer do DataBase.
    DataBase *CheckDB() const { if (!the_db) SimpleAbort(); return the_db; }

    /// Pointer do DataBase.
    DataBase *GetDataBase() const { return the_db; }

    /// Sets the pointer to database.
    DataBase *SetDataBase(const string& dbname) {
      DataBase *db = picsom->FindDataBase(dbname);
      SetDataBase(db);
      return db;
    }

    /// Sets the pointer to database.
    void SetDataBase(DataBase *db) { the_db = db; }

    /// Lengthens division vectors if they aren't long enough
    void ConditionallyLengthenVectors() {
      // obs! Should the metadatas be updated such as in ReadMetaDataFile() ???

      int l = (int)DataBaseSize();

      for (size_t i=0; i<Nindices(); i++)
        if (IsTsSom(i)) {
          for (int j=0; j<NlevelsEvenAlien(i); j++) {
            IntVector& divvec = Division(i, j);
            while (divvec.Length()<l)
              divvec.Push(-1);
          }
        }
    }

    /// Checks and possibly reloads the division files
    void CheckModifiedDivFiles() {
      for (size_t i=0; i<Nindices(); i++)
        TsSom(i).CheckModifiedFiles();
    }

    /// DataBase's name.
    const string& DataBaseName() const { 
      static const string empty;
      return the_db?the_db->Name():empty;
    }

    /// Number of objects in database.
    size_t DataBaseSize() const { return the_db?the_db->Size():0; }

    /// Number of objects in database, possibly restricted with selection.
    size_t DataBaseRestrictionSize(target_type tt) const {
      return the_db ? the_db->RestrictionSize(tt) : 0;
    }

    /// Number of objects in seen which are not allowed by restriction.
    int RestrictedObjectsSeen() const { return restricted_objects_seen; }

    /// Number of objects in seen which are not allowed by restriction.
    void RestrictedObjectsSeen(int s) { restricted_objects_seen = s; }

    /// Returns query_target.
    target_type Target() const { return query_target_; }

    /// Sets query_target.
    void Target(target_type tt) {
      bool debug = false;
      if (debug)
	cout << "query_target set from " << TargetTypeString(Target())
	     << " to " << TargetTypeString(tt) << endl;
      query_target_ = tt;
    }

    /// Number of objects of target_type query_target in the database.
    int NumberTargetTypeObjects() const {
      return the_db?the_db->NumberObjects(Target()):0;
    }
    
    /// Returns a label from the database by index.
    const string& Label(int i) const {
      static const string empty;
      return the_db?the_db->Label(i):empty;
    }

    /// Returns a label from the database by index.
    const char *LabelP(int i) const { return Label(i).c_str(); }

    /// Returns a label from the database by index.
    int LabelIndex(const string& l) const {
      return the_db?the_db->LabelIndex(l):-1;
    }

    /// Resolves what files need to be read and then does it.
    bool ReadFiles(bool lo = true, bool create = false);

    ///
    bool ReadDataFiles(bool force = false, bool nodata = false) {
      bool ret = true;
      for (size_t i=0; i<Nindices(); i++)
        ret = ret && TsSom(i).ReadDataFile(force, nodata);
      return ret;
    }

    ///
    bool SelectTopLevelsOld(bool);

    /// Selects all forced initial images given as argument.
    bool OnlyForcedObjects(const string&);

    /// Selects initial images among images on top levels.
    bool TopLevelsByEntropy();

    /// Calculates convolution and its entropy.
    double Entropy(int, int, int);

    /// Calculates convolutions and entropies and returns minimum's index.
    int MinimumEntropyMap(int, int);

    /// Calculates convolutions and entropies and returns the sum.
    double TotalEntropy(int, int);

    /// Selects initial images randomly.
    bool RandomUnseenObjects(bool deletenewobjects=true,
                             bool justonepositive=false);

    ///
    bool SelectFromPre();

    /// Selects all unseen images.
    bool AllUnseenObjects(bool deletenewobjects=true);

    /// Counts the number of seenobjects on a map.
    int SeenObjectCount(int, int) const;

    /// Gives index to the map which has least seen images.
    int LeastSeenMap(int) const;

    /// Makes a new Object instance for SelectTopLevels().
    Object *MakeNewObject(int, bool);

    /// Gives the total number of features.
    size_t NindicesNew() const {
      return Nindices(FirstAlgorithm());
    }

    /// Gives the total number of features.
    size_t Nindices() const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      return Nindices(NULL);
    }

    /// Gives the total number of features.
    size_t Nindices(const algorithm_data *a) const {
      return a ? a->algorithm->Nindices(a->data) : index_data.size();
    }

    /// Gives the total number of features.
    size_t Nfeatures() const { return Nindices(); }

    /// Returns true if any feature is DynamicBinaryTextFeature.
    bool HasDynamicBinaryTextFeatures() const {
      for (size_t i=0; i<Nindices(); i++)
        if (IsWordInverse(i) && wordInverse(i).IsDynamicBinaryTextFeature())
          return true;
      return false;
    }

    /// Gives the number of levels in a map.
    int Nlevels(int m) const {
      return algorithm==cbir_sq || !IsTsSom(m) ? 1 : TsSom(m).Nlevels();
    }

    /// Gives the number of levels in a map.
    int NlevelsEvenAlien(int m) const {
      return algorithm==cbir_sq || !IsTsSom(m) ? 1 :
        TsSom(m).NlevelsEvenAlien();
    }

    /// Gives numbers of all map levels in one vector.
    IntVector Nlevels() const {
      IntVector ret(Nindices());
      for (size_t i=0; i<Nindices(); i++)
        ret[i] = Nlevels(i);
      return ret; 
    }

    /// Finds index of named feature in tssom or -1 if not used.
    int IndexShortNameIndex(const string& n) const {
      for (size_t i=0; i<Nindices(); i++)
        if (n==IndexShortName(i))
          return i;
      return -1;
    }

    /// Finds index of named feature in tssom or -1 if not used.
    int IndexFullNameIndex(algorithm_data *a,
                             const string& n) const {
      if (!a)
        for (size_t i=0; i<Nindices(); i++)
          if (n==IndexFullName(i))
            return i;
      if (a)
        return a->algorithm->IndexFullNameIndex(a->data, n);
      return -1;
    }

    ///
    vector<size_t> IndexShortNameIndexList(const string& n) const {
      vector<size_t> idx;
      for (size_t i=0; i<Nindices(); i++)
        if (n==IndexShortName(i))
          idx.push_back(i);
      return idx;
    }

    /// Name of feature by index.
    const string& IndexShortName(size_t i) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      if (!MapIndexOK(i)) {
        static const string qms = "???";
        ShowError("Query::IndexShortName(int) index failure");
        return qms;
      }
      return IndexStaticData(NULL, i).Name();
    }

    /// Name of feature by index.
    const string& IndexFullName(size_t i) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      if (!IndexIndexOK(i)) {
        stringstream ss;
        ss << "i=" << i << " Nindices()=" << Nindices();
        ShowError("Query::IndexFullName(size_t) index failure ", ss.str());
        static const string qms = "???";
        return qms;
      }
      return IndexData(NULL, i).fullname;
    }

    list<string> IndexFullNames() const {
      list<string> l;
      for (size_t f=0; f<Nindices(); f++)
	l.push_back(IndexFullName(f));
      return l;
    }

    list<string> IndexFullNamesNew() const {
      const algorithm_data *a = FirstAlgorithm();
      list<string> l;
      for (size_t f=0; f<Nindices(a); f++)
	l.push_back(a ? a->data->IndexFullName(f) : IndexFullName(f));
      return l;
    }

    /// Weight of the feature, level (-1=bottom), default.
    float FeatureWeight_x(size_t i, int l = -1, float def = 1.0) const {
      const float nan = Index::State::nan();
      if (!IndexIndexOK(i)) {
        ShowError("Query::FeatureWeight(size_t) index failure A");
        return nan;
      }
      const vector<float>& w = IndexDataTSSOM(i).tssom_level_weight;
      if (l<-1 || (l==-1 && w.size()==0) || l>=(int)w.size()) {
        ShowError("Query::FeatureWeight(size_t) index failure B");
        return nan;
      }
      float v = l==-1 ? w.back() : w[size_t(l)];
      return v==nan ? def : v;
    }

    /// Returns true if if valid index to index_data.
    bool IndexIndexOK(size_t i) const { return i<Nindices(); }

    /// Returns true if if valid index to tssoms.
    bool MapIndexOK(size_t i) const { return IndexIndexOK(i); }

    /// Gives reference to any TSSOM map.
    TSSOM& TsSom(size_t i) { return IndexDataTSSOM(i).TsSom(); }

    /// Gives reference to any TSSOM map.
    const TSSOM& TsSom(size_t i) const { return IndexDataTSSOM(i).TsSom(); }

    /// Gives reference to VectorIndex.
    VectorIndex& vectorIndex(size_t i) {
      return IndexDataVectorIndex(i).Vectorindex();
    }

    /// Gives reference to any WordInverse object.
    WordInverse& wordInverse(size_t i) { 
      // obs! HAS_CBIRALGORITHM_NEW problem
      Index *ind = &IndexStaticData(NULL, i);
      WordInverse *w = dynamic_cast<WordInverse*>(ind);
      if (!w)
        ShowError("Query::wordInverse() failing with i="+ToStr(i)+
                  " fullname=<"+ind->Name()+">");
      return *w;
    }

    /// Gives reference to any WordInverse object.
    const WordInverse& wordInverse(size_t i) const { 
      return ((Query*)this)->wordInverse(i);
    }

    /// Gives reference to any PreCalculatedIndex object.
    PreCalculatedIndex& preCalculated(size_t i) { 
      // obs! HAS_CBIRALGORITHM_NEW problem
      Index *ind = &IndexStaticData(NULL, i);
      PreCalculatedIndex *w = dynamic_cast<PreCalculatedIndex*>(ind);
      if (!w)
        ShowError("Query::preCalculated() failing with i="+ToStr(i)+
                  " fullname=<"+ind->Name()+">");
      return *w;
    }

    /// Gives reference to any PreCalculatedIndex object.
    const PreCalculatedIndex& preCalculated(size_t i) const { 
      return ((Query*)this)->preCalculated(i);
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

    /// Returns reference to the data set.
    const FloatVectorSet& Data(int i) const { return TsSom(i).Data(); }

    /// Returns reference to the data set.
    FloatVectorSet& Data(int i) { return TsSom(i).Data(); }

    /// A convenience routine...
    static int ListSize(const ObjectListList& l) {
      return l.size();
    }

    /// A convenience routine...
    static int ListSize(const VQUnitListList& l) {
      return l.size();
    }

    ///
    size_t NseenObjects() const { return seenobject.size(); }

    ///
    const Object& SeenObject(size_t i) const { return seenobject[i]; }

    ///
    Object& SeenObject(size_t i) { return seenobject[i]; }

    ///
    size_t NnewObjects() const { return newobject.size(); }

    ///
    ObjectList& NewObject() { return newobject; }

    ///
    const Object& NewObject(size_t i) const { return newobject[i]; }

    ///
    Object& NewObject(size_t i) { return newobject[i]; }

    /// should be deprecated...
    void AppendNewObject(Object *o) { newobject.push_back(*o); delete o; }

    ///
    void AppendNewObject(const Object& o) { newobject.push_back(o); }

    /// Combines newobject structure from another Query.
    bool SetOrSumNewObjects(const Query&);

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
                       n,mapw);}

    /// Dumps seenobject.
    void DumpSeen(bool n = false) const { DumpList(seenobject, "SEEN", n); }

    /// Dumps vqunit list
    void DumpVQUnits(int m, int l, bool n = false) const {
      if (IsTsSomAndLevelOK(m, l))
        DumpList(PerMapVQUnit(m, l), "VQUNITS", n, IndexFullName(m), m, l,
                 MapEvenAlien(m,l).Width());
      else
        cout << "VQUNITS (" << m << "," << l << ") not available" << endl;
    }
    
    /// Dumps combinedimage.
    void DumpCombined() const { DumpList(combinedimage, "COMBINED"); }

    /// Dumps uniqueimage.
    void DumpUnique(bool n = false) const { DumpList(uniqueimage,"UNIQUE",n);}

    /// Dumps newobject.
    void DumpNew(bool n = false) const { DumpList(newobject, "NEW", n); }

    /// Dumps permapnewobject.
    void DumpNew(int m, int l, bool n = false) const {
      DumpList(PerMapNewObject(m, l), "NEW", n, IndexFullName(m), m, l); }
    
    ///
    void ShowObjectListsRecursively() const;

    /// Shows selected vqunit/image lists.
    void ShowUnitAndObjectLists(bool = true,  bool = true, bool = false,
                                bool = true,  bool = true, bool = true,
                                bool = false, bool = true, bool = false) const;

    /// Returns the retained size of the list.
    static size_t Nretained(const ObjectList& l) {
      size_t n = 0;
      for (size_t i=0; i<l.size(); i++)
        if (l[i].Retained())
          n++;
      return n;
    }

    ///
    size_t NuniqueRetained() const { return Nretained(uniqueimage); }

    ///
    size_t NnewRetained() const { return Nretained(newobject); }

    ///
    void DeleteSeen() { seenobject.clear(); }

    ///
    void DeleteCombined() { combinedimage.clear(); }

    ///
    void DeleteUnique() { uniqueimage.clear(); }

    ///
    void DeleteNew()  { newobject.clear(); }

    ///
    void FindMissingIndices();

    ///
    bool AddIndex(algorithm_data *a, Index *t, const string& fulln) {
      if (!a) {
        index_data.push_back(t->CreateInstance(fulln));
        return true;
      }
      return a->algorithm->AddIndex(a->data, t->CreateInstance(fulln));
    }

    ///
    bool RemoveIndex(algorithm_data *a, size_t i) {
      if (!a) {
        if (i>=Nindices())
          return ShowError("Query::RemoveIndex("+ToStr(i)+") : index error");

        delete index_data[i];
        index_data.erase(index_data.begin()+i);

        return true;
      }
      return a->algorithm->RemoveIndex(a->data, i);
    }

    /// Used to store xxx from features=zzz(xxx).
    bool SetIndexParameters(algorithm_data*, size_t, const string&);

    /// Called by SetIndexParameters().
    bool SetIndexWeight(Index::State&, const string&);

    /// Called by SetIndexParameters().
    bool SetIndexAspects(Index::State&, const string&);

    /// Used in Interpret().
    bool SetAspectInfo(map<string,pair<string,float> >&,
                       const string&,  const string&);

    /// Returns true if Index::State aspects intersect with Object aspects.
    bool AspectsMatch(const Index::State&, const Object&,
                      double&, double&) const;

    /// Returns true if aspects are used.
    bool UsingAspects() const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      if (positive.size()>1 ||
          (positive.size()==1 && positive.begin()->first!=""))
        return true;

      if (negative.size()>1 ||
          (negative.size()==1 && negative.begin()->first!=""))
        return true;

      for (size_t i=0; i<Nindices(); i++)
        if (IndexData(NULL, i).aspects.empty() ||
	    IndexData(NULL, i).aspects[0]!="*")
          return true;
      return false;
    }

    /// Returns comma-seprated list of aspects for given feature.
    string IndexAspects(const algorithm_data *a, size_t i) const {
      list<string> s;
      for (vector<string>::const_iterator j=IndexData(a, i).aspects.begin();
           j!=IndexData(a, i).aspects.end(); j++)
        s.push_back("\""+*j+"\"");
      return CommaJoin(s);
    }

    ///
    bool IndexDataExists() const { return Nindices(); }

    ///
    bool CreateIndexData(bool);

    ///
    bool CopyBasicIndexData(const vector<Index::State*>&);

    /// Marks objects from seenobjects to all ts-soms and levels.
    bool PlaceSeenOnMap(bool norm=true,bool limit=false) {
      for (size_t m=0; m<Nindices(); m++)
        for (int l=0; l<Nlevels(m); l++) 
          PlaceSeenOnMap(m, l,norm,limit);
      return true;
    }

    /// Marks objects from seenobjects to map of given ts-som and level.
    bool PlaceSeenOnMap(int, int, bool=true,bool=false);

    /// Returns true if divmul-weights must be checked per object
    bool DivMulPerObject(int m) { return TsSom(m).DivMulWeightsGiven(); }

    /// Gives the weighting vector for BMU div levels.
    vector<float> DivMultiplierVector(int, int, int=-1);

    /// Gives the weighting vector for BMU div levels.
    const vector<float>& DivHistoryVector(int, int);

    /// Marks objects from seenobjects to map of given ts-som and level.
    bool PlaceSeenOnMapPicSOM(int, int, bool=true,bool=false);

    /// Marks objects from seenobjects to map of given ts-som and level.
    bool PlaceSeenOnMapPicSOMoneMap(int, int, bool=true);

    /// Marks objects from seenobjects to map of given ts-som and level.
    bool PlaceSeenOnMapPicSOMoneMapSmoothedFraction(int, int);

    /// Marks objects from seenobjects to map of given ts-som and level.
    bool PlaceSeenOnMapPicSOMthreeMaps(int, int, bool=true,bool=false);

    /// Marks objects from seenobjects to map of given ts-som and level.
    bool PlaceSeenOnMapPicSOMfourMaps(int, int, bool=true);

    /// A helper routine for the three above.
    bool CountPositivesAndNegatives(const IntVector&, int&, int&, bool,
                                    bool=true) const;

    /// A helper routine for the three above.
    bool SumPositivesAndNegatives(const IntVector&, const Index::State&,
                                  double&, double&, bool) const;

    /// Marks objects from seenobjects to map of given ts-som and level.
    bool PlaceSeenOnMapVQ(int, int);

    /// A helper utlity for the PlaceSeenOnMap*() methods.
    void SetSizeOrZero(simple::FloatMatrix& hts, const TreeSOM& ts) {
      hts.SizeOrZero(ts.Height(), ts.Width());
      if (!hts.Size())
        ShowError("Matrix size set to 0x0");
    }

    /// Returns true if given object index is found in seenobject.
    bool IsSeen(int idx) const {
      return seenobject.contains((size_t)idx);
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

    /// Locates the object in seenobject[].
    const Object *FindInObjectIndex(int idx, const ObjectList& l) const {
      size_t n = l.size();
      for (size_t i=0; i<n; i++)
        if (l[i].Index()==idx)
          return &l[i];

      return NULL;
    }

    /// Locates the object in newobject[].
    Object *FindInNew(int idx) {
      for (size_t i=0; i<NnewObjects(); i++)
        if (NewObject(i).Index()==idx)
          return &NewObject(i);

      return NULL;
    }

    /// Checks the integrity of seenobject and newobject.
    bool ConditionallyCheckObjectIndices(bool sloppy) {
      return !debug_check_lists ||
        (CheckSeenObjectIdx(false) && CheckUniqueObjectIdx(false) &&
         CheckNewObjectIdx(sloppy) &&
	 (can_show_seen||CrossCheckSeenObjectIdx()));
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

    ///
    bool ObjectSelectable(size_t idx) const {
      return algorithms.size()==0 ||
	algorithms[0].algorithm->ObjectSelectable(idx);
    }

    /// Returns true if given object index can be returned as new object.
    bool CanBeShown(size_t, bool) const;

    /// Checks whether image index is allowed by database-wide restriction.
    bool OkWithDataBaseRestriction(size_t i) const {
      return the_db && the_db->OkWithRestriction(i);
    }

    /// returns intersection of the database-wide and query-wide restrictions.
    ground_truth RestrictionGT(bool reset = false) const {
      if (the_db)
	the_db->RwLockWrite("Query::RestrictionGT");
      if (restriction.size() && reset)
	((Query*)this)->SetTemporalRestriction(restriction.label(),
					       query_target_, -1);
      ground_truth r = the_db ? the_db->RestrictionGT() : ground_truth();
      ground_truth ret = restriction.size() ? r.TernaryAND(restriction) : r;
      if (the_db)
	the_db->RwUnlockWrite("Query::RestrictionGT");
      return ret;
    }

    /// Removes temporal restriction.
    void RemoveTemporalRestriction() {
      if (debug_restriction)
        cout << "Removing temporal restriction" << endl;
      restriction.clear();
    }

    /// Sets temporal restriction.
    void SetTemporalRestriction(const string& gt, target_type tt, int o) {
      if (!the_db) {
        ShowError("SetTemporalRestriction() : the_db==NULL");
        return;
      }
      struct timespec now = TimeNow();
      bool expand = true;
      restriction = the_db->GroundTruthExpression(gt, tt, o, expand);
      struct timespec end = TimeNow();
      float secf = TimeDiff(end, now);
      if (debug_restriction || secf>5.0) {
	char sec[100];
	sprintf(sec, "%.3f", secf);
        WriteLog("Set temporal restriction ["+gt+"] "+
		 TargetTypeString(tt)+" "+ToStr(o)+" in "+ToStr(sec)+" seconds");
	if (debug_restriction)
	  the_db->GroundTruthSummary(restriction);
      }
    }

    /// Name stored for temporal restriction.
    const string& TemporalRestrictionName() const {
      return restriction.label();
    }

    /// Checks whether image index is allowed by query-specific restriction.
    bool OkWithTemporalRestriction(size_t i) const {
      return !restriction.size() || restriction.get(i)==1;
    }

    /// Combines the three checks above.
    bool CanBeShownRestricted(size_t idx, bool checkparents) const {
      // the optimal order of operations might be different ?
      return CanBeShown(idx, checkparents) &&
        OkWithDataBaseRestriction(idx) && OkWithTemporalRestriction(idx);
    }

    /// Access to can_show_seen.
    bool CanShowSeen() const { return can_show_seen; }

    /// Sets can_show_seen.
    void CanShowSeen(bool s) { can_show_seen = s; }

    /// Return the number of BMU's available.
    int DivisionDepth(int i, int j) const { return TsSom(i).DivisionDepth(j); }

    ///
    IntVector& Division(int i, int j, int d = 0) {
      return TsSom(i).Division(j, d);
    }

    ///
    const IntVector& Division(int i, int j, int d = 0) const {
      return TsSom(i).Division(j, d);
    }

    /// Access to value of matrixcount.
    void DefaultMatrixCount(size_t c) { matrixcount_xx = c; }

    /// Access to value of matrixcount.
    size_t DefaultMatrixCount() const { return matrixcount_xx; }

    /// Access to value of matrixcount.
    size_t GetMatrixCount(size_t i) const {
      return IsTsSom(i) ? IndexDataTSSOM(i).MatrixCount() : 0;
    }

    ///
    size_t NumberOfScoreValues() const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      size_t s = 0;
      for (size_t i=0; i<Nindices(); i++)
        s += IndexStaticData(NULL, i).NumberOfScoreValues();
      return s;
    }

    ///
    int ScoreValueIndex(size_t ii, size_t jj) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      size_t s = 0;
      for (size_t i=0; i<Nindices(); i++) {
        size_t j = IndexStaticData(NULL, i).NumberOfScoreValues();
        if (i==ii)
          return jj>=j ? -1 : int(s+jj);
        s += j;
      }
      return -1;
    }

    ///
    simple::FloatMatrix& Hits(size_t i, size_t j, size_t k) {
      return *IndexDataTSSOM(i).GetHits(j, k);
    }

    ///
    const simple::FloatMatrix& Hits(size_t i, size_t j, size_t k) const {
      return ((Query*)this)->Hits(i, j, k);
    }

    ///
    simple::FloatMatrix& Convolved(size_t i, size_t j, size_t k) {
      return *IndexDataTSSOM(i).GetConvolved(j, k);
    }

    ///
    const simple::FloatMatrix& Convolved(size_t i, size_t j, size_t k) const {
      return ((Query*)this)->Convolved(i, j, k);
    }

    ///
    const simple::FloatMatrix& Convolved(const string& n, size_t j,
					 size_t k) const {
      int i = IndexShortNameIndex(n);
      if (i<0)
        ShowError("Query::Convolved() : IndexShortNameIndex("+n+") failed");
      return Convolved(i, j, k);
    }

    /// Gives hits matrix even from parents if available.
    const simple::FloatMatrix *GetHits(int, int, int) const;

    /// Gives convolved matrix even from parents if available.
    const simple::FloatMatrix *GetConvolved(int, int, int) const;

    /// Access to TSSOM::State::vqulist.
    VQUnitList& PerMapVQUnit(int i, int j) {
      return IndexDataTSSOM(i).vqulist[j];
    }

    /// const-const version of the one above.
    const VQUnitList& PerMapVQUnit(int i, int j) const {
      return ((Query*)this)->PerMapVQUnit(i, j);
    }

    /// Returns size of vqunits_xyz.
    size_t PerMapObjectsSize() const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      size_t s = 0;
      for (size_t i=0; i<Nindices(); i++)
        s += IndexData(NULL, i).objlist.size();
      return s;
    }

    /// One argument interface to permapnewobject.
    ObjectList& PerMapNewObject(int ii) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      // this might be slow...
      size_t i = (size_t)ii;
      for (size_t m=0; m<Nindices(); m++)
        if (i<IndexData(NULL, m).objlist.size())
          return IndexData(NULL, m).objlist[i];
        else
          i -= IndexData(NULL, m).objlist.size();
      static ObjectList empty;
      return empty;
    }

    /// const-const version of the one above.
    const ObjectList& PerMapNewObject(int i) const {
      return ((Query*)this)->PerMapNewObject(i);
    }

    ///
    ObjectList& PerMapNewObject(int i, int j) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      return IndexData(NULL, i).objlist[j];
    }

    ///
    const ObjectList& PerMapNewObject(int i, int j) const {
      return ((Query*)this)->PerMapNewObject(i, j);
    }

    ///
    const IntVector& BackReference(int i, int l, int j) const {
      if (!MapIndexOK(i)) {
        ShowError("Query::BackReference(i, l, j): i not OK");
        const IntVector *emptyptr = NULL;
        return *emptyptr;
      }
      return TsSom(i).BackReference(l, j); 
    }

    /// True if images will be selected by ground truth.
    bool HasViewClass() const { return !viewclass.empty(); }

    /// Objects selected by ground truth.
    const string& ViewClass() const { return viewclass; }

    /// Sets images selected by ground truth.
    void SetViewClass(const string& c) { viewclass = c; }

    /// Objects selected by ground truth or set to positive.
    const string& ViewClassOrPositive() const {
      return HasViewClass() ? ViewClass() : Positive();
    }

    /// True if images will be marked positive by ground truth.
    bool HasPositive(const string& a = "") const {
      return Positive(a)!="";
    }

    /// True if positives of any aspect have been defined.
    bool HasAnyPositive() const { return !positive.empty(); }

    /// Objects marked positive by ground truth.
    const string& Positive(const string& a = "") const {
      static const string empty;
      map<string,pair<string,float> >::const_iterator i = positive.find(a);
      return i!=positive.end() ? i->second.first : empty;
    }

    /// Sets images marked positive by ground truth.
    void SetPositive(const string& c, const string& a = "", double v = 1.0) {
      positive[a] = make_pair(c, v);
    }

    /// Returns vector of aspects for which positive has been defined.
    vector<string> PositiveAspects() const {
      vector<string> ret(positive.size());
      size_t j = 0;
      for (map<string,pair<string,float> >::const_iterator i=positive.begin();
           i!=positive.end(); i++)
        ret[j++] = i->first;
      return ret;
    }

    /// Returns an aspect_map_t of all positive aspects.
    aspect_map_t AspectsMapCommon(const map<string,pair<string,float> >& a,
                                  target_type t, int o, bool e) const {
      aspect_map_t m;
      for (map<string,pair<string,float> >::const_iterator i=a.begin(); i!=a.end(); i++)
        m[i->first] = make_pair(the_db->GroundTruthExpression(i->second.first, t, o, e),
                                i->second.second);

      return m;
    }

    /// Returns an aspect_map_t of all positive aspects.
    aspect_map_t PositiveAspectsMap(target_type t, int o, bool e) const {
      return AspectsMapCommon(positive, t, o, e);
    }

    /// True if images will be marked negative by ground truth.
    bool HasNegative(const string& a = "") const {
      return Negative(a)!="";
    }

    /// True if negatives of any aspect have been defined.
    bool HasAnyNegative() const { return !negative.empty(); }

    /// Objects marked negative by ground truth.
    const string& Negative(const string& a = "") const {
      static const string empty;
      map<string,pair<string,float> >::const_iterator i = negative.find(a);
      return i!=negative.end() ? i->second.first : empty;
    }

    /// Sets images marked negative by ground truth.
    void SetNegative(const string& c, const string& a = "", double v = 1.0) {
      negative[a] = make_pair(c, v);
    }

    /// Returns vector of aspects for which negative has been defined.
    vector<string> NegativeAspects() const {
      vector<string> ret(negative.size());
      size_t j = 0;
      for (map<string,pair<string,float> >::const_iterator i=negative.begin();
           i!=negative.end(); i++)
        ret[j++] = i->first;
      return ret;
    }

    /// Returns an aspect_map_t of all negative aspects.
    aspect_map_t NegativeAspectsMap(target_type t, int o, bool e) const {
      return AspectsMapCommon(negative, t, o, e);
    }

    /// Whether this query just got back from the settings page.
    bool BackFromSettings() const { return backfromsettings; }
    
    /// Marks that this query just got back from the settings page.
    void SetBackFromSettings() { backfromsettings = true; }

    ///
    void DeleteSomeVariables(bool = true);

    ///
    bool IsInNewObjects(size_t) const;

    /// Gives acceesss to pointed map level.
    const TreeSOM& PointedMapLevel() const {
      return Map(pointed_map, pointed_level);}

    ///
    bool IsMapPointSet() const { 
      return pointed_map>=0 && pointed_level>=0 && mappoint.IsDefined(); }

    /// Rounds mappoint to nearest map unit center coordinates.
    void RoundMapPoint();

    /// Moves mappoint one step to "nsweudlr".
    bool RelativeMapPointMovement(int);

    ///
    bool IsInZoomArea(int m, int l, int x, int y) const {
      if (!IsMapPointSet() || m!=pointed_map || l!=pointed_level)
        return false;
      return x>=maparea_ul.X() && x<=maparea_lr.X() &&
        y>=maparea_ul.Y() && y<=maparea_lr.Y();
    }
  
    ///
    bool IsEmptyMappoint(int, int, int, int) const;  

    /// Returns a combination of "nsweudlr".
    const char *AllowedMapPointMovements() const;

    /// Fills newobject list with images around mappoint.
    void SelectAroundMapPoint();

    /// Fills newobject list with images according to contents.
    void SelectByContents();

    /// Returns the number of requested objects.
    size_t MaxQuestions() const { return maxquestions; }

    /// Sets maxquestions.
    void MaxQuestions(size_t maxq) { maxquestions = maxq; }

    /// Returns the set of best vq units from a map level.
    /// Used by the PicSOM algorithm.
    VQUnitList BestVQUnits(int, int) const;

    /// Returns the set of best vq units from a map level.
    /// Used by VQ/VQW algorithms.
    VQUnitList BestVQUnitsVQ(int, int) const;

    /// Returns the set of potential new question images from a map level.
    /// Used by the PicSOM algorithm.
    ObjectList SelectObjectsFromVQUnits(double, const VQUnitList&,
                                       const TreeSOM&, bool, int, int) const;

    /// Returns the set of potential new question images from a vq unit list.
    /// Used by VQ/VQW algorithms.
    ObjectList SelectObjectsFromVQUnitsVQ(const VQUnitList&) const;

    /// Returns the set of potential new question images from a map level.
    /// Used by the PicSOM algorithm. 
    // ObjectList NewObjects(int, int) const;

    /// Returns the set of potential new question images from a map level.
    /// Used by VQ/VQW algorithms.
    ObjectList NewObjectsVQ(int, int) const;

    /// Returns the set of potential new question images from a map level.
    /// Used by VQ/VQW algorithms.
    // ObjectList NewObjectsFromUnseenUnitsVQ(int, int) const;

    /// Sorts the potential new images according to their distance to 
    /// positive images and returns MaxQuestions() best ones.
    /// Used by VQ/VQW algorithms.
    void ResolveNearestNewObjectsVQ(bool dowrite=TRUE);

    ///
    // simple::FloatMatrix CreateContentMatrix(int m, int l, const char* content);

    /// This is the place where responses get processed...
    bool SinglePass();

    ///
    void PerMapObjects(int i) { _sa_permapobjects = i; }

    ///
    int PerMapObjects() const { return _sa_permapobjects; }

#ifdef USE_MRML

    bool MRMLSinglePass(Connection *c);

    bool MRMLOpenSession(Connection *c);

    bool MRMLGetCollections(Connection *c);

    bool MRMLGetAlgorithms(Connection *c);

    bool MRMLConfigureSession(Connection *c);

    bool MRMLQueryStep(Connection *c);

    // for Analysis MRML implementation
    // sets collection_id after querying it for later use
    void setCollectionID(const char *cid) { collection_id = (string)cid; }

    // gets collection_id
    XMLS getCollectionID() { return (XMLS)collection_id.c_str(); }

    // for Analysis MRML implementation
    // sets algorithm_id after querying it for later use
    void setAlgorithmID(const char *aid) { algorithm_id = (string)aid; }

    // gets algorithm_id
    XMLS getAlgorithmID() { return (XMLS)algorithm_id.c_str(); }

#endif // USE_MRML

    /// Processes the CBIR state machine.
    bool CbirStages();

    /// Algorithms for setting quewry-specific restrictions.
    cbir_stage StageRestrict(cbir_function, const string&);

    /// Algorithms for selecting the initial direction based on seenobject.
    cbir_stage StageEnter(cbir_function, const string&);

    /// Algorithms for initial set selection.
    cbir_stage StageInitialSet(cbir_function, const string&);

    /// Algorithms for checking if stage_initial_set returned enough images.
    cbir_stage StageCheckNimages(cbir_function, const string&);

    /// Algorithms for continued initial selection.
    cbir_stage StageNoPositives(cbir_function, const string&);

    /// Algorithms for continuing set selection.
    cbir_stage StageHasPositives(cbir_function, const string&);

    /// Algorithms for handling first query round in a special manner.
    cbir_stage StageSpecialFirstRound(cbir_function, const string&);

    /// Algorithms for expanding relevance values to related objects.
    cbir_stage StageExpandRelevance(cbir_function, const string&);

    /// Common per-feature iterator.
    bool ForAllIndices(bool (Query::*foo)(int));

    /// Common per-map-level iterator.
    bool ForAllMapLevels(bool (Query::*foo)(const string&,int,int,bool),
                         const string&, bool botonly=false);

#ifdef QUERY_USE_PTHREADS
    struct map_level_pthread_data {
      pthread_t pthread;
      bool thread_set;
      Query *query;
      bool (Query::*f)(const string&,int,int,bool);
      const string *args;
      int map;
      int level;
      bool botonly;
    };

    /// Common per-map-level iterator.
    bool ForAllMapLevelsPthread(bool (Query::*foo)(const string&,int,int,bool),
                                const string&, bool);

    ///
    static void *ForAllMapLevelsPthread(void*);

#endif // QUERY_USE_PTHREADS

    /// Algorithms for running for each map.
    cbir_stage StageRunPerMap(cbir_function, const string&);

    /// Algorithms for setring per-TSSOM weights.
    cbir_stage StageSetMapWeights(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageCombineUnitLists(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageExtractObjects(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageCombineObjectLists(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageRemoveDuplicates(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageExchangeRelevance(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageSelectObjectsToProcess(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageProcessObjects(cbir_function, const string&);

    /// Algorithms for XXX.
    cbir_stage StageConvergeRelevance(cbir_function, const string&);

    /// Algorithms for doing the last selection.
    cbir_stage StageFinalSelect(cbir_function, const string&);

    /// Used in StageFinalSelect().
    bool PossiblyRelativizeScores();

    /// Sets temporary restriction by specification like 1-10:foo,11-15:bar
    bool RestrictionSpec(const string&);

    void ZeroRelevanceCounters(ObjectList& il) {
      for (size_t i=0; i<il.size(); i++) 
        il[i].ZeroRelevanceCounters();
    }

    void SumRelevanceCounters(ObjectList& il, bool set_to_one=false) {
      for (size_t i=0; i<il.size(); i++) 
        il[i].SumRelevanceCounters(set_to_one);
    }

    /// Combines values from subobject back to their parent.
    bool RelevanceUp(const string &, struct relop_stage_type&, 
                     ObjectList&, bool divide, bool seenimg);

    /// Combines values from parents to subobjects.
    bool RelevanceDown(const string&, struct relop_stage_type& , 
                       ObjectList&, bool divide, bool seenimg);

    /// Combines values from subobjects up to parents and then down again
    /// to the other subobjects.
    bool RelevanceUpDown(const string& /*s*/, struct relop_stage_type&, 
                         ObjectList& il, bool divide, bool seenimg);

    void PushRelevanceDown(size_t i, struct relop_stage_type&,
                           ObjectList& il, double val, map<int,int> &idx,
                           bool divide, bool seenimg, int sibling_i);

    void PushRelevanceUp(size_t i, struct relop_stage_type&,
                         ObjectList& il, double val,map<int,int> &idx, 
                         bool divide, bool seenimg);

    /// Access to list of stage names, the enum and pointers to functions.
    static bool StageNames(int i, cbir_stage&, const char*&);

    /// Returns string form for enum.
    const char *StageNameP(cbir_stage) const;

    /// Returns enum from string.
    cbir_stage StageName(const string&) const;

    /// Tries to set stage function to given value.
    bool SetStageFunction(const string&, const string&, bool = false);

    /// Function to run at that stage.
    cbir_function StageFunction(cbir_stage, bool = false, bool = false,
                                bool = true) const;

    /// Default functions to run for random algorithm.
    cbir_function StageDefaultRandom(cbir_stage, bool) const;

    /// Default functions to run for PicSOM algorithm (all TS-SOM levels).
    cbir_function StageDefaultPicSOMAll(cbir_stage, bool) const;

    /// Default functions to run for PicSOM algorithm (bottom level only).
    cbir_function StageDefaultPicSOMBottom(cbir_stage, bool) const;

    /// Default functions to run for PicSOM algorithm (bottom level only).
    cbir_function StageDefaultPicSOMBottomWeighted(cbir_stage, bool) const;

    /// Default functions to run for VQ algorithm.
    cbir_function StageDefaultVQ(cbir_stage, bool) const;

    /// Default functions to run for VQW algorithm.
    cbir_function StageDefaultVQW(cbir_stage, bool) const;

    /// Default functions to run for SQ algorithm.
    cbir_function StageDefaultSQ(cbir_stage, bool) const;

    /// Default functions to run for External/XXX algorithms.
    cbir_function StageDefaultExternal(cbir_stage, bool) const;

    /// Access to list of function names and the enum.
    static bool FunctionNames(int i, cbir_function&, const char*&);

    /// Returns string for enum.
    static const char *FunctionNameP(cbir_function);

    /// Returns enum for string.
    static cbir_function FunctionName(const string&);

    /// a new stage function...
    bool RemoveDuplicatesPicSOM();

    /// a new stage function...
    bool RemoveDuplicatesVQ();

    /// a new stage function...
    bool ProcessObjectsPicSOMBottom(const string&);

    ///
    class scorefile_pointers {
    public:
      scorefile_pointers() {
        fp = fp_comp = fp_posneg = NULL;
      }

      bool close() {
        if (fp)            fclose(fp);
        if (fp_comp)            fclose(fp_comp);
        if (fp_posneg)     fclose(fp_posneg);

        return true;
      }

      FILE *fp;
      FILE *fp_comp;
      FILE *fp_posneg;
    }; // class scorefile_pointers

    bool OpenScoreFiles(scorefile_pointers &fp);

    bool WriteScoreFiles(const scorefile_pointers &fp,const string &lbl,
                         int idx,
                         float val, const vector<float> &cvals,
                         const vector<float> &posnegvals);

    ///
    class picsom_bottom_score_options_t {
    public:

      picsom_bottom_score_options_t(){
        mul=add=zero=rho=ratioscore=smoothed_fraction=relevancemodulation
          = maxofsubs=geomsumsubs=sublinearrank=storesubscores=posonly=
          no_propagation=filedump_ttypecheck=equalise_subobject_count=false;
      }

      picsom_bottom_score_options_t(bool){
        picsom_bottom_score_options_t();
      }

      bool mul;
      bool add;
      bool zero;
      bool rho;
      bool ratioscore;
      bool smoothed_fraction;
      bool relevancemodulation;
      bool maxofsubs;
      bool geomsumsubs;
      bool sublinearrank;
      bool storesubscores;
      bool posonly;
      bool no_propagation;
      bool filedump_ttypecheck;
      bool equalise_subobject_count;
    }; // class picsom_bottom_score_options_t

    ///
    picsom_bottom_score_options_t ParseBottomOptions(const string &arg);

    /// actual score calculating function for the above stage fcn
    float PicSOMBottomScore(int ,const picsom_bottom_score_options_t &,
                            FloatVector *,Object *img=NULL, 
                            vector<float> *cvals=NULL,
                            vector<float> *posnegvals=NULL);

    double *MLEstimateMapWeights(const vector<vector<float> > &scores,
                                 size_t negatives);

    void ml_initTheta(const vector<vector<float> > &scores, double *tgt);

    void ml_calculate_h(const vector<vector<float> > &scores,
                        const double *theta, double *tgt);

    float ml_calculate_h(const vector<float> &score,
                        const double *theta);
    
    void ml_calculate_gradient(const vector<vector<float> > &scores, 
                               size_t negcount, const double *hval,
                               double *tgt);

    void ml_calculate_hessian(const vector<vector<float> > &scores,
                              const double *hval,
                              double **tgt);

    double ml_calculate_fval(const double *hval,size_t negcount,size_t total);

    bool PropagateBySegmentHierarchy();

    /// a new stage function...
    bool ProcessObjectsVQ(const string&, bool w=false);

    /// a new stage function...
    bool ProcessObjectsVQW(const string& a) { return ProcessObjectsVQ(a, true); }

    /// function called by StageFinalSelect

    bool StoreSubobjectContribution(const string&);
    
    /// function actually implementing the above function
    bool StoreSubobjectContribution(Object &img,const string&);

    /// helper function for above
    double DetermineCountBias() const;

    bool StrVsRelop(string& op, relop_type& r, bool setrelop=true);

    bool StrToRelopStage(const string&, struct relop_stage_type&);

    enum relop_type StrToRelop(const string& op) {
      enum relop_type r;
      string str = op;
      if (!StrVsRelop(str,r)) {
        ShowError("Bad operator \"", op, "\" changed to \"none\"");
        return relop_none;
      } 
      return r;
    }    

    string RelopToStr(const enum relop_type relop) {
      enum relop_type r=relop;
      string str;
      if (!StrVsRelop(str,r,false)) 
        return "[error]";
      return str;
    }   
 
    int RelopStageToChoice(const struct relop_stage_type rs) const {
      return (rs.up==relop_none?0:RELEVANCE_UP) + 
        (rs.down==relop_none?0:RELEVANCE_DOWN);
    }

    void LogRelopStage(const string& msg, const relop_stage_type& rs) {
      string mat;
      WriteLog(msg," (UP: "+RelopToStr(rs.up),
               ", DOWN: "+RelopToStr(rs.down)+")");
    }

    bool Expand(const string&);

    /// Puts children of seen objects to the end of the seenobject list
    /// (retain=false)
    bool ExpandDown(const string&);

    /// Puts parents of seen objects to the end of the seenobject list
    /// (retain=false)
    bool ExpandUp(const string&);

    /// Expands down and then up
    bool ExpandDownUp(const string& args) {
      return ExpandDown(args) && ExpandUp(args); 
    }

    /// Expands up and then down
    bool ExpandUpDown(const string& args);


    /// Puts some children of seen objects to the end of the seenobject list 

    bool ExpandSelective(const string&);

    bool Exchange(const string&);

    ///  
    bool ExchangeUp(const string&);

    /// 
    bool ExchangeDown(const string&);

    ///
    bool ExchangeUpDown(const string& s);

    ///
    bool ExchangeDownUp(const string& s) {
      return ExchangeDown(s) && ExchangeUp(s);
    }

    bool Converge(const string&);

    /// 
    bool ConvergeUp(const string&);

    /// 
    bool ConvergeDown(const string&);

    ///
    bool ConvergeUpDown(const string& s);

    ///
    bool ConvergeDownUp(const string& s) {
      return ConvergeDown(s) && ConvergeUp(s);
    }

    ///
    cbir_stage OfflineShortcut();

    /// Part of PicSOM that is run for each separate map.
    bool PerMapPicSOM(const string&, int, int, bool);

    /// Part of PicSOM that is run after best units have been found.
    bool ForUnitsPicSOM(const string&, int, int, bool);

    /// Part of PicSOM that is run after individual maps.
    bool CombineMapsPicSOM();

    /// Part of PicSOM that is run last.
    bool FinalSelectPicSOM();

    /// Part of VQ/VQW that is run for each separate map.
    bool PerMapVQ(int);

    /// Part of VQ/VQW that is run after best units have been found.
    bool ForUnitsVQ(int);

    /// Part of VQ that is run after individual maps.
    bool CombineMapsVQ();

    /// Part of VQ/VQW that is run last.
    bool FinalSelectVQ();

    /// Part of VQW that is run after individual maps.
    bool CombineMapsVQW();

    /// Gathers new images from all the maps and removes any duplicates.
    /// Used by VQ/VQW algorithms.
    bool GatherNewObjects();

    /// Assigns a score value for each new image.
    /// Used by VQ/VQW algorithms, weighted is true with VQW.
    bool NewObjectValues(bool weighted = false);

    /// Returns the indices of seen images.
    /// @param only_positives whether to process only positive images
    IntVector SeenObjectIndices(bool only_positives=FALSE) const;
    
    /// Returns the used map level in the VQ/VQW algorithms.
    int VQlevel() { return _sa_vqlevel; }

    /// Sets the used map level in the VQ/VQW algorithms.
    void VQlevel(int vql) { _sa_vqlevel = vql; }

    /// Calculates the squared distances of new images to previously seen
    /// positive images. Used by VQ/VQW algorithms.
    DoubleVector DistancesToPositives(const FloatVectorSet&, int);

    /// Calculates a score for map units. Used by VQ/VQW algorithms.
    FloatVector MapUnitScore(int, int) const; 

    /// Object extraction stage specific to scalar quantization.
    bool PerFeatureSQ(int);

    /// Object extraction stage specific to scalar quantization.
    bool PerFeatureSQold(int);

    /// Does scalar quantization of data in all maps.
    bool ScalarQuantize(bool = false);

    /// Does scalar quantization of data in one map.
    bool ScalarQuantize(int, bool = false);

    /// Calculate binary features scores.
    bool SimpleBinaryFeatureScores(int);

    /// Use BinaryClass() to calculate binary features scores.
    bool SimpleBinaryFeatureScoresViaBinaryClassNew(int, bool);

    /// Use BinaryClass() to calculate binary features scores.
    bool SimpleBinaryFeatureScoresViaBinaryClassOld(int, bool);

    /// Use BinaryFeature() to calculate binary features scores.
    bool SimpleBinaryFeatureScoresViaBinaryFeature(int, bool);

    /// Calculate non-latent binary features scores.
    bool SimpleBinaryFeatureScoresNonLatent(int);

    /// Calculates the binary feature score of image idx.
    double BinaryFeatureScoreForObject(int map, int idx) const;

    /// Selects the candidate images based on binary classes/features
    /// in the ForUnitsPicSOM() stage.
    ObjectList SelectObjectsByBinaryFeatureScores(int, int, int);

    /// Selects the candidate images based on precalculated scores
    /// in the ForUnitsPicSOM() stage.
    ObjectList SelectObjectsByPreCalculatedScores(int, int, int);

    /// Selects the candidate images based on SVM scores
    /// in the ForUnitsPicSOM() stage.
    ObjectList SelectObjectsBySvmScores(int, int, int);

    /// Returns precalculated scores.
    bool SimplePreCalculatedScores(int);

    /// Returns SVM scores.
    bool SimpleSvmScores(int);

  private:
    float (*prescore_function)(float);

  public:
    /// Returns a precalculated score of image idx.
    double PreCalculatedScoreForObject(int map, int idx) const;

    /// This is executed when everything has been set up ready.
    bool Run();

    /// This is executed when everything has been set up ready.
    bool RunXML(xmlDocPtr, Connection*);

    /// This is executed last...
    bool RunPartThree();

#ifdef QUERY_USE_PTHREADS
    /// Launch a pthread for running RunPartThree().
    static void *ExecuteRunPartThreeThread(void*);

    /// Launch a pthread for running RunPartThree().
    void *ExecuteRunPartThreeThread();
#endif // QUERY_USE_PTHREADS

    /// Wrapper around the above and direct call to RunPartThree().
    /// Possibly creates a new thread to free the calling Connection's thread.
    bool ExecuteRunPartThree();

  /// Calculates the theoretical maximum entropy for given matrix size.
  static double MaximumEntropy(const simple::FloatMatrix&);

  /// Calculates the theoretical entropy increase due to the convolution;
  static double ConvolutionEntropy(const FloatVector*);

  /// Returns a convolution mask vector.
  const FloatVector *ConvolutionVector(const string&, int);

  /// Creates a brand new convolution mask vector.
  static FloatVector *CreateConvolutionVector(const string&, double, double);

  /// Searches for ConvolutionVector name and pointer.
  static bool ConvolutionVectorNameAndFunc(const string&, string&,
                                           //double (*&)(double,double,double)
                                           ConvolutionFunction&);

  /// Splits and joins ConvolutionVector name parts.
  static string ConvolutionVectorName(const string&, int,
                                      string&, float&, float&);

  /// Splits ConvolutionVector name parts.
  static ConvolutionFunction ConvolutionVectorFunc(const string&, int,
                                                   float&, float&);

  /// Extracts ConvolutionVector parameters.
  static string ConvolutionVectorParam(const char*, int, float&, float&);

  /// Prepares things for convolution mask creation.
  static bool ConvolutionPrepare(const string&, float&, float&);

  /// Generates point response value.
  static double PointVectorValue(double, double, double);

  /// Generates values for half of a 1D triangle mask.
  static double TriangleVectorValue(double, double, double);

  /// Generates values for 1/rank mask.
  static double OnePerRankVectorValue(double, double, double);

  /// Generates values for half of a 1D Gaussian mask.
  static double GaussianVectorValue(double, double, double);
        
  /// Generates values for half of a 1D difference-of-gaussians mask.
  static double DoGVectorValue(double, double, double); 

  /// Generates values for half of a 1D rectangular mask.
  static double RectangleVectorValue(double, double, double);

  /// Generates values for half of a 1D rectangular mask.
  static double ExponentialVectorValue(double, double, double);

  /// Generates values for half of a 1D Hann mask.
  static double HannVectorValue(double, double, double);

    ///
    bool Convolve();
    
    ///
    bool Convolve(int, int, int);  

    ///
    bool ConvolveOneDimMask(int, int, int, const FloatVector&);  

    ///
    bool ConvolveNbrRank(const string &,int, int, int);  

    ///
    bool ConvolveRatio(const string &,int, int, int);  

    ///
    bool ConvolveUmatrix(const string&, int, int, int);  

    ///
    bool ConvolveUmatrixSum(const string&, SOM::UmatrixFunc, int, int, int);  

    ///
    bool ConvolveUmatrixSum(string&,
                            const SOM*, SOM::UmatrixFunc,
                            ConvolutionFunction, double, double,
                            const simple::FloatMatrix&,
                            simple::FloatMatrix&, simple::FloatMatrix&);

    ///
    bool ConvolveUmatrixSum(const SOM*, SOM::UmatrixFunc,
                            ConvolutionFunction, double, double,
                            const simple::FloatMatrix&, simple::FloatMatrix&, bool);
    
    ///
    bool ConvolveUmatrixPath(const string&, SOM::UmatrixFunc, int, int, int);  

    ///
    bool BernoulliMAP(int, int);  

    /// Writes strings to log.
    void WriteLog(const string& m1, const string& m2 = "",
                  const string& m3 = "", const string& m4 = "", 
                  const string& m5 = "") const {
      picsom->WriteLogStr(LogIdentity(), m1, m2, m3, m4, m5); }

    /// Writes contents of a stream to log.
    void WriteLog(ostringstream& os) const {
      picsom->WriteLogStr(LogIdentity(), os); }

    /// Gives log form of Query identity.
    string LogIdentity() const {
      stringstream ss;
      ss << "<" << (identity!=""?identity:"-") <<
        (the_db?" ":"") << DataBaseName() << ">";
      return ss.str();

      /*connection?connection->Client().IsSet()?
        ClientAddress():"-":"*",*/ // OBS! Query/Connection
    }

    /// Logs both in visual and matlab format.
    void DoubleLog(string& mat, const string& m1, const string& m2 = "",
                   const string& m3 = "", const string& m4= "", 
                   const string& m5 = "") const {
      WriteLog(m1, m2, m3, m4, m5);
      string tmp = m1 + m2 + m3 + m4 + m5;
      for (size_t b = 0;;) {
        size_t n = tmp.find('\n', b);
        if (n==string::npos)
          break;
        tmp.insert(n+1, "% ");
        b = n+1;
      }
      mat += "% "+tmp+"\n";
    }

    /// Writes a log separator both in visual and matlab format.
    void DoubleLogSeparator(string& mat) const {
      DoubleLog(mat, string(64, '='));
    }

    /// All info in one string.
    const string DisplayString(bool a = false) const {
      const char *txt = DisplayStringM(a);
      string ret = txt ? txt : "";
      delete txt;
      return ret;
    }

    /// All info in one string. Delete after use!
    const char *DisplayStringM(bool = false) const;

    /// Writes the 2-dimensional convolution mask out as a Matlab file.
    void ConvolutionTest(int, int);

    /// Returns index to TS-SOM defined by name.
    int FindTsSom(const string&, int* = NULL,
                  IntPoint* = NULL, FloatPoint* = NULL) const;

    /// Sends any object to connection.
    bool SendObject(Connection&, object_type, const char*, const char* = NULL);

    /// Writes positive-marked seenobjects to class file.
    void WriteSelectionsToFile() const;

    /// Writes label from "classdef::" to class file.
    void WriteClassdefToFile(const string& cls, const string& lbl, int val) 
      const;

    /// Was the SinglePass() / AnalysisXXX() operation successful?
    bool OkStatus() const { return okstatus; }

    /// Creates map image.
    imagedata CreateMapImage(int, int, int, const string&) const;

    /// Creates map image.
    imagedata CreateMapImage(const simple::FloatMatrix&,
                             const imagedata* = NULL) const;

    /// Helper routine to give color index.
    static int ValueToColorIndex(double v, double maxg, double ma, double mi) {
      int r = v>0 ? (int)floor(maxg/2+v*ma+0.5) : (int)floor(maxg/2+v*mi+0.5);
      if (r<0)
        return 0;
      if (r>maxg)
        return int(maxg);
      return r;
    }

    /// Paints a blue/wite/red map and combines it with background.
    imagedata BlueWhiteRedMap(const simple::FloatMatrix&,
                              const IntMatrix* = NULL,
                              const IntMatrix* = NULL,
                              const IntMatrix* = NULL,
                              const IntMatrix* = NULL, int = 1,
                              const imagedata* = NULL) const;

    /// Paints a blue/wite/red map.
    imagedata BlueWhiteRedMapInner(const simple::FloatMatrix&, const IntMatrix*,
                                   const IntMatrix*, const IntMatrix*,
                                   const IntMatrix*, int) const;

    /// Makes a color index map.
    imagedata ColorIndexMap(const IntMatrix&, const IntMatrix*,
                                    const IntMatrix*, int) const;

    /// Makes a color index map. (old version)
    // bool ColorIndexMap(ostream& os, const IntMatrix&,
    //               const IntMatrix*, const IntMatrix*, int) const;

    ///
    bool ShowMapPoints(const string&);

    /// Returns cbir algorithm.
    cbir_algorithm Algorithm() const { return algorithm; }

    /// Sets cbir algorithm.
    void Algorithm(cbir_algorithm a) { algorithm = a; }

    /// Starts ticking.
    void Tic(const char *f) const { picsom->Tic(f); }

    /// Stops tacking.
    void Tac(const char *f = NULL) const { picsom->Tac(f); }

    ///
    static void DebugInfo(bool d) { debug_info = d; }

    ///
    static bool DebugInfo() { return debug_info; }

    /// Sets debug_all_keys used in AddOtherKeyValue().
    static void DebugAllKeys(bool d) { debug_all_keys = d; }

    /// Sets debug_erf_features.
    static void DebugErfFeatures(int d) { debug_erf_features = d; }

    /// Gets debug_erf_features.
    static int DebugErfFeatures() { return debug_erf_features; }

    /// Sets debug_erf_relevance.
    static void DebugErfRelevance(int d) { debug_erf_relevance = d; }

    /// Gets debug_erf_relevance.
    static int DebugErfRelevance() { return debug_erf_relevance; }

    /// Sets debug_ajax.
    static void DebugAjax(int d) { debug_ajax = d; }

    /// Gets debug_ajax.
    static int DebugAjax() { return debug_ajax; }

    /// Sets debug_stages used in SelectIndex(s)().
    static void DebugRestriction(bool d) { debug_restriction = d; }

    /// Sets debug_stages used in SelectIndex(s)().
    static void DebugFeatSel(bool d) { debug_featsel = d; }

    /// Returns debug_stages used in SelectIndex(s)().
    static bool DebugFeatSel() { return debug_featsel; }

    /// Sets debug_aspects used in some *Aspect*().
    static void DebugAspects(bool d) { debug_aspects = d; }

    /// Returns debug_aspects used in some *Aspect*().
    static bool DebugAspects() { return debug_aspects; }

    /// Sets debug_phases used in some *Phase*().
    static void DebugPhases(bool d) { debug_phases = d; }

    /// Sets debug_placeseen used in some PlaceSeenOnMap*().
    static void DebugPlaceSeen(bool d) { debug_placeseen = d; }

    /// Sets debug_algorithms.
    static void DebugAlgorithms(bool d) { debug_algorithms = d; }

    /// Value of debug_algorithms.
    static bool DebugAlgorithms() { return debug_algorithms; }

    /// Sets debug_stages used in CbirStages().
    static void DebugStages(bool d) { debug_stages = d; }

    /// Sets debug_lists used in CbirStages().
    static void DebugLists(int d) { debug_lists = d; }

    /// Returna debug_lists used in CbirStages().
    static int DebugLists() { return debug_lists; }

    /// Sets debug_check_lists used in CbirStages().
    static void DebugCheckLists(bool d) { debug_check_lists = d; }

    /// Sets debug_weights used in CbirStages().
    static void DebugWeights(bool d) { debug_weights = d; }

    /// Sets debug_parameters used in CbirStages().
    static void DebugParameters(bool d) { debug_parameters = d; }

    /// Sets debug_subobjects used eg. in MarkAsSeen().
    static void DebugSubobjects(bool d) { debug_subobjects = d; }

    static bool DebugSubobjects() { return debug_subobjects; }

    /// Sets debug_can_be_shown used in CanBeShown().
    static void DebugCanBeShown(bool d) { debug_can_be_shown = d; }

    /// Sets debug_class used in DoClassification();
    static void DebugClassify(bool d) { debug_classify = d; }

    /// Returns debug_class used in DoClassification();
    static bool DebugClassify() { return debug_classify; }

    /// Sets debug_prf used in ExpansionByPRF();
    static void DebugPRF(bool d) { debug_prf = d; }

    /// Returns debug_prf used in ExpansionByPRF();
    static bool DebugPRF() { return debug_prf; }

    /// Sets debug_selective used in ProcessObjectsPicSOMBottom()
    /// and ExpandSelective.
    static void DebugSelective(bool d) { debug_selective = d; }

    ///
    static bool DebugSelective() { return debug_selective; }

    /// Bug compatibility mode.
    static void Pre2122Bug(bool m) { pre2122bug = m; }

    /// Bug compatibility mode.
    static bool Pre2122Bug() { return pre2122bug; }

    /// Sets if relevance should be immediately shared with sibling objects
    static void RelevanceToSiblings(bool d) { relevance_to_siblings = d; }

    /// Returns true if relevance is immediately shared with sibling objects
    static bool RelevanceToSiblings() { return relevance_to_siblings; }

   ///
    static void CombFactor(float v) { comb_factor = v; }

    /// Saves in a file.
    bool Save(const string&, bool = false) /*const*/;

    /// Retrieves from a file.
    bool Retrieve(const string&, bool /*= false*/, const Connection*);

    /// Retrieves from an XML tree.
    bool Retrieve(const XmlDom&, const Connection*);

    /// Retrieves from an XML chain.
    bool InterpretXML(const XmlDom&, bool, const Connection*);

    /// Retrieves objectinfo from an XML chain.
    bool InterpretXMLobjects(const XmlDom&);

    /// Retrieves erfdata from an XML chain.
    bool InterpretXMLerfdata(const XmlDom&, const string&);

    /// Parses comma-separated featurelist out of XML.  Delete after use!
    static const char *XMLextractFeatureNamesM(const xmlNodePtr);

    /// Retrieves stream.start data from an XML chain.
    bool InterpretXMLajaxdata(const XmlDom&, bool);

    /// Ensures that ajax_data_all has been initialized.
    bool Ensure_ajaxdata();

    /// Copies XML data to ajaxdata.
    bool AddDataTo_ajaxdata(const XmlDom&, const XmlDom&);

    /// Returns save path of the query.  Delete after use!
    string SavePath(bool r) const {
      return picsom->SavePath(Identity(), r);
    }

    /// Returns last access time, possibly recursively.
    struct timespec LastModificationTime(bool = false) const;

    /// Returns last access time, possibly recursively.
    struct timespec LastAccessTime(bool = false) const;

    /// Sets last modification time.
    void SetLastModificationTimeNow() { SetTimeNow(last_modification); }

    /// Sets last access time.
    void SetLastAccessTimeNow() { SetTimeNow(last_access); }

    /// Gives saved_time.
    struct timespec SavedTime() const { return saved_time; }

    /// Returns true if saved_time is more recent than given time.
    bool SavedAfter(const struct timespec& t) const {
      return MoreRecent(saved_time, t);
    }

    /// Returns true if last modification is more recent than saved_time.
    bool NeedsSave() const {
      return !MoreRecent(saved_time, LastModificationTime(true));
    }

    /// When a query has been retrieved from file, some things are missing...
    bool ConditionallyRecalculate();

    /// Counts the number of nodes in query tree.
    size_t Nodes() const {
      size_t n = 1; 
      for (size_t i=0; i<Nchildren(); i++) 
        n += Child(i)->Nodes();
      return n; 
    }

    /// Counts the number of leaves in query tree.
    size_t Leaves() const {
      size_t n = 0; 
      for (size_t i=0; i<Nchildren(); i++)
        n += Child(i)->Leaves();
      return n+(Nchildren()==0);
    }

    /// Counts the number of (new) images in query tree.
    size_t Objects() const {
      size_t n = NnewObjects();
      for (size_t i=0; i<Nchildren(); i++) 
        n += Child(i)->Objects();
      return n; 
    }

    /// Dumps image names with + prepended if positive.
    bool DumpObjectList() const;

    /// Restrict access.
    void SingleWriteMultiReadLock(bool);

    /// Release for others to write.
    void SingleWriteMultiReadUnlock();

#ifdef QUERY_USE_PTHREADS
    ///
    bool PthreadLock(bool w) { 
      if (!picsom->PthreadsConnection())
        return true;

      if (Connection::DebugLocks())
        WriteLog("PthreadLock(", w?"write":"read", ") entering");
      bool r = !(w?pthread_rwlock_wrlock(&rwlock):
                 pthread_rwlock_rdlock(&rwlock));
      if (Connection::DebugLocks())
        WriteLog("PthreadLock(", w?"write":"read", ") exited");
      return r;
    }

    ///
    bool PthreadUnlock() {
      if (!picsom->PthreadsConnection())
        return true;

      if (Connection::DebugLocks())
        WriteLog("PthreadUnlock()");
      bool r = !pthread_rwlock_unlock(&rwlock);
      return r;
    }

#endif // QUERY_USE_PTHREADS

    /// Prints out variables used in Analysis::Analyse*().
    bool WriteAnalyseVariablesOld(const string&, int, const ground_truth*,
                                  const ground_truth*, int, bool, string&);

    /// Prints out variables used in tau Analysis::Analyse*().
    bool WriteAnalyseVariablesNew(const string&, string&,
                                  const ground_truth_list&,
                                  const list<string>& = list<string>());

    /// The common tail part of the two abobe.
    bool WriteAnalyseVariablesCommon(const string&, string&, int, int, bool);

    /// Tells seed value of random number generator.
    int RndSeed() const { return rndseed_value; }

    /// Tells seed value of random number generator or "random".
    string RndSeedStr() const {
      return (rndseed_random?"random->":"")+ToStr(RndSeed());
    }

    /// Selects the effective random value for rndseed.
    int RndSeedRandomize() {
      rndseed_random = true;
      rndseed_value  = RandVar().Seed();
      WriteLog("rndseed randomized to "+ToStr(rndseed_value));
      return RndSeed();
    }

    /// Increments the rndseed value if it really was random or forced.
    int RndSeedStep(bool force) {
      if (rndseed_random || force) {
        int old = rndseed_value;
        rndseed_value = RandVar().Seed();
        WriteLog("rndseed stepped from "+ToStr(old)+" to "+ToStr(rndseed_value));
      }
      return RndSeed();
    }

    /// Page to show after an experiment.
    const string& EndPage() const { return endpage; }

    /// Page to show after an experiment.
    string EndPageUrl() const {
      string ep = EndPage();
      if (ep.find("http://")==0)
	return ep;
      if (ep[0]=='/')
	ep.erase(0, 1);
      return Picsom()->HttpListenConnection()+"/"+ep;
    }

    /// Returns maximum number rounds.
    size_t MaxRounds() const { return maxrounds; }

    /// Returns round number.
    int Round() const { return round; }

    /// Sets round counter to zero.
    void ZeroRound() { round = 0; }

    /// Increments round counter by one.
    void IncrementRound() { round++; }

    /// Sets upload_label.
    void UploadLabel(const string& l) { upload_label = l; }

    /// Sets convtype.
    void ConvType(int m, const string& t) {
      if (m==-1)
        _sa_default_convtype = t;

      else
        _sa_tssom_convtype[m] = t;
    }

    /// Returns convtype.
    const string& ConvType(int m) const { 
      if (m==-1)
        return _sa_default_convtype;

      map<int,string>::const_iterator i = _sa_tssom_convtype.find(m);
      return i!=_sa_tssom_convtype.end() ? i->second : _sa_default_convtype;
    }

    /// Sets bmuconvtype.
    void BmuConvType(int m, const string& t) {
      if (m==-1)
        _sa_default_bmuconvtype = t;
      else
        _sa_tssom_bmuconvtype[m] = t;
    }

    /// Returns bmuconvtype.
    string BmuConvType(int);

    /// Sets weight.
    void Weight(int m, float v) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      //      wordInverse(m).BinaryWeight(v);
      IndexData(NULL, m).Weight(v);
    }

    /// Returns weight.
    float Weight(int m) const { 
      // obs! HAS_CBIRALGORITHM_NEW problem
      //      return IsInverseIndex(m)?wordInverse(m).BinaryWeight():1.0;
      return IndexData(NULL, m).Weight();
    }

    /// Checks the validity of stage.
    static bool StageOK(cbir_stage s) {
      return s>=stage_first && s<=stage_last;
    }

    /// Get access to stage_map.
    const cbir_function& StageFunc(cbir_stage s) const throw(logic_error) {
      stage_map_t::const_iterator i = stage_map.find(s);
      if (i==stage_map.end())
        throw logic_error("unknown cbir_stage");
      return i->second;
    }

    /// Set access to stage_map.
    void StageFunc(cbir_stage s, cbir_function f) throw(logic_error) {
      if (!StageOK(s))
        throw logic_error("unknown cbir_stage");

      stage_map[s] = f;
    }

    /// Get access to stage_arg_map.
    const string& StageArgs(cbir_stage s) const throw(logic_error) {
      stage_arg_map_t::const_iterator i = stage_arg_map.find(s);
      if (i==stage_arg_map.end())
        throw logic_error("unknown cbir_stage");
      return i->second;
    }

    /// Set access to stage_arg_map.
    void StageArgs(cbir_stage s, const string& a) throw(logic_error) {
      if (!StageOK(s))
        throw logic_error("unknown cbir_stage");

      stage_arg_map[s] = a;
    }

    /// parses comma-separated argument list to a vector of key-value
    /// pairs

    static list<pair<string,string> > SplitArgs(const string &arg);

    /// Return a comma-separated list of selected features.
    string BriefIndexList() const {
      string ret;
      for (size_t i=0; i<Nindices(); i++)
        ret += (ret!=""?",":"")+IndexFullName(i);
      return ret;
    }
          
    /// Dumps out feature names.
    void ListIndices(ostream& os = cout) const {
      for (size_t i=0; i<Nindices(); i++)
        os << i << " <" << IndexFullName(i) << "> "
           << TsSom(i).DivisionLengths_x() << endl;
    }

    ///
    void DumpSeenRelevance(){
      for (size_t ind=0; ind<NseenObjects(); ind++){
        cout << "End of pass, dumping SeenObject("<<ind
             <<").rel_distribution" << endl;
        SeenObject(ind).rel_distribution.dump();
      }
    }

    /// True if CbirStages() should end with "classification" of positives.
    bool Classify() { return classify>0; }

    /// The true integer value of classify.
    int ClassifyInt() { return classify; }

    /// Call with non-zero if you want "classification" of positives.
    void Classify(int c) { classify = c; }

    /// Sets modifiable parameters for DoClassification();
    void SetClassifyParams(const string& m, int nc, const string& cs) {
      classifymethod    = m;
      classifynclasses  = nc;
      classificationset = cs;
    }

    ///
    size_t NclassModels() const { return classmodel.size(); }

    ///
    const string& ClassModelName(size_t i) const { return classmodel[i]; }

    ///
    void RemoveClassModels() { classmodel.clear(); }

    ///
    void AddClassModelName(const string& c) { classmodel.push_back(c); }

    /// 
    void ClassifyMethod(const string& m) { classifymethod = m; }

    /// 
    void ClassifyNClasses(const int n) { classifynclasses = n; }

    /// This does the "classification".
    bool DoClassification();

    /// This does the "classification".
    bool DoClassificationTsSom(Query*, const string&, double);

    /// This does the "classification".
    bool DoClassificationKnn(size_t);

    /// This does the "classification".
    bool DoClassificationSvm();

    /// 
    void ClearClassAugmentations() { 
      if (debug_classify)
        cout << "Clearing all class augmentations" << endl;
      classaugment.clear(); 
    }

    /// 
    set<string> GetAugmentClasses(const string& mapname) const {
      set<string> classes;
      for (auto i=classaugment.begin(); i!=classaugment.end(); i++) {
        const string& featn = i->first.second;
        if (featn.empty() || featn==mapname)
          classes.insert(i->first.first);
      }
      return classes;
    }

    /// 
    float ClassWeight(int) const;

    /// Calling this with true forces enytropy calculations.
    void CalculateEntropy(bool f) {
      calculate_entropy_force = f;
      if (f && DefaultMatrixCount()==1) {
        WarnOnce("Setting calculate_entropy_force implies matrixcount=3");
        DefaultMatrixCount(3);
      }
    }

    /// True if forced or set_map_weights stage requires it.
    bool CalculateEntropy() const {
      if (calculate_entropy_force)
        return true;

      if (algorithm<cbir_picsom || algorithm>cbir_picsom_bottom_weighted)
	return false;

      cbir_function f = StageFunction(stage_set_map_weights);

      return f==func_entropy_n_polynomial || f==func_entropy_n_leave_out_worst;
    }

    /// Actual routine to call the different types of entropy.
    bool DoEntropyCalculation() {
      size_t kdefault = DefaultMatrixCount()==3 ? 1 : 0;
      size_t k = entropymatrix==-1 ? kdefault : entropymatrix;

      if (k>=DefaultMatrixCount())
        k = kdefault;

      for (size_t m=0; m<Nindices(); m++)
        for (int l=0; l<Nlevels(m); l++)
          if (!DoEntropyCalculation(m, l, k))
            return false;
      
      return true;
    }

    /// Actual routine to call the different types of entropy.
    bool DoEntropyCalculation(int, int, int);

    /// A simple TSSOM weighting implementation. 
    bool ValueOrNaN(float, bool);

    /// A simple TSSOM weighting implementation. 
    bool TsSomWeightValue(bool);
    
    /// 
    bool TsSomWeightValueAndDistances(bool);

    /// A simple TSSOM weighting implementation. 
    bool EntropyNpolynomial(const string&);

    /// A simple TSSOM weighting implementation. 
    bool EntropyNleaveOutWorst(const string&);

    /// Sets file name for object lists in Matlab format.
    static void MatlabDump(const string& f) { matlabdump = f; }

    /// Returns file name for object lists in Matlab format.
    static const string& MatlabDump() { return matlabdump; }

    /// Dumps out object lists in Matlab format.
    bool DoMatlabDump() const { return DoDump(0, matlabdump); }

    /// Sets file name for object lists in dat format.
    static void DatDump(const string& f) { datdump = f; }

    /// Returns file name for object lists in dat format.
    static const string& DatDump() { return datdump; }

    /// Dumps out object lists in dat format.
    bool DoDatDump() const { return DoDump(1, datdump); }

    /// Dumps out object lists in Matlab or dat format.
    bool DoDump(int, const string&) const;

    /// write access to watch set
    static void Watchset(const IntVector& w) { watchset = w;}

    /// write access to watch set
    static void Watchset(const vector<size_t>& w) { 
      watchset.Length(w.size());
      for (size_t i = 0; i<w.size(); i++)
        watchset[i] = w[i];
    }

    /// Sets hits&convolved matrices dump file name prefix for images.
    void MapImageName(const string& n) { mapimagename = n; }

    /// gets the above mentioned prefix
    const string &MapImageName() const {return mapimagename;}

    /// Sets hits&convolved matrices dump file name prefix for matlab.
    void MapMatlabName(const string& n) { mapmatlabname = n; }

    /// Sets hits&convolved matrices dump file name prefix for dat files.
    void MapDatName(const string& n) { mapdatname = n; }

    /// Gets file name prefix for score dumps.
    string ScoreFile() { return scorefile; }

    /// Sets file name prefix for score dumps.
    void ScoreFile(const string& n) { scorefile = n; }

    /// Copies scorevalues in a long vector.
    bool StoreScores(vector<float>&) const;

    /// Dumps out hits&convolved matrices in various forms.
    bool MapSnapshots(int, int, const string &xs=string("")) const;

    /// Sets setname_comment_str variable.
    void SetnameComment(const string& t) { setname_comment = t; }

    /// Access to setname_comment_str variable.
    const string& SetnameComment() const { return setname_comment; }

    /// Collects recursively all seen object indices and responses.
    list<pair<int,float> > DepthFirstSeenObjects() const;

    /// Collects recursively all seen object indices and responses.
    Query *DepthFirstRelevantObjects(target_type) const;

    /// Returns number of retained non-show newobjects.
    size_t NnewObjectsRetainedNonShow() const;

    /// Traverses children and returns pointer to one with most objects.
    Query *FindLargestChild();

    /// Sets textquery.
    void TextQuery(const string& st) { textquery = st; }

    /// Access to textquery.
    const string& TextQuery() const { return textquery; }

    /// Sets implicit_textquery.
    void ImplicitTextQuery(const string& st) { implicit_textquery = st; }

    /// Access to implicit_textquery.
    const string& ImplicitTextQuery() const { return implicit_textquery; }

    /// Sets textquery_processed.
    void TextQueryProcessed(const string& st) { textquery_processed = st; }

    /// Access to textquery_processed.
    const string& TextQueryProcessed() const { return textquery_processed; }

    /// Solves text search parameters.
    textsearchspec_t TextSearchParams(const string& = "") const;

    /// Access to start time.
    const timespec& StartTime() const { return start; }

    ///
    bool DoClassModelAugmentation(int, int, int);

    typedef map<pair<string,string>,float> classaugment_t;
    typedef classaugment_t::iterator classaugment_iter_t;
    typedef classaugment_t::const_iterator classaugment_citer_t;

    ///
    void SetAugmentValue(const float value, const string& classname="", 
                         const string& mapname="") {
      if (debug_classify)
        cout << "Setting classaugment(" << classname 
             << (mapname.empty()?"":",") << mapname << ") = " << value
             << endl;
      classaugment[pair<string,string>(classname,mapname)] = value;
    }

    ///
    void SetAugmentValueBasedOnAspect(const float value,
                                      const string& classname="") {
      // obs! HAS_CBIRALGORITHM_NEW problem
      for (size_t m=0; m<Nindices(); m++) 
        for (vector<string>::const_iterator
	       j=IndexData(NULL, m).aspects.begin();
             j!=IndexData(NULL, m).aspects.end(); j++)
          if (*j == "concept") {
            SetAugmentValue(value, classname, IndexShortName(m));
            break;
          }
    }

    ///
    float GetAugmentValue(const string&, const string&);

    ///
    void CreateAllClassModels();

    ///
    typedef multimap<double,string> cls_dist_map;

    ///
    typedef list<cls_dist_map> cls_dist_map_list;

    ///
    typedef struct {
      cox::knn::lvector_t vector;
      TSSOM *tssom;
      TSSOM::classifier_data *cfier;
    } vector_cfier;

    ///
    typedef list<vector_cfier> vector_cfier_list;

    ///
    cls_dist_map_list ClassDistances(const vector_cfier_list&);

    ///
    list<string> Classify(const vector_cfier_list&);

    ///
    algorithm_data *FirstAlgorithm() {
      return algorithms.empty() ? NULL : &algorithms[0];
    }

    ///
    const algorithm_data *FirstAlgorithm() const {
      return algorithms.empty() ? NULL : &algorithms[0];
    }

    ///
    Index::State& IndexData(size_t i) {
      return IndexData(FirstAlgorithm(), i);
    }

    ///
    Index::State& IndexData(algorithm_data *a, size_t i) {
      if (a)
	return a->data->IndexData(i);

      if (!IndexIndexOK(i)) {
        stringstream s;
        s << "i=" << i << " Nindices()=" << Nindices();
        ShowError("Query::IndexData(size_t) index error! : ", s.str());
        TrapMeHere();
      }
      return *index_data[i];
    }

    ///
    const Index::State& IndexData(const algorithm_data *a, size_t i) const {
      return ((Query*)this)->IndexData((algorithm_data*)a, i);
    }

    ///
    const Index& IndexStaticData(const algorithm_data *a, size_t i) const {
      return *IndexData(a, i).StaticPart();
    }

    /// fixme: ideally this should not be needed...
    Index& IndexStaticData(algorithm_data *a, size_t i) {
      return *IndexData(a, i).StaticPart();
    }

    ///
    bool IsVectorIndex(size_t i) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      return IndexIndexOK(i) && IndexData(NULL, i).Is("vectorindex");
    }

    ///
    bool IsSvm(size_t i) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      return IndexIndexOK(i) && IndexData(NULL, i).Is("svm");
    }

    ///
    bool IsWordInverse(size_t i) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      return IndexIndexOK(i) && IndexData(NULL, i).Is("wordinverse");
    }

    ///
    bool IsInverseIndex(size_t i) const { 
      // obs! HAS_CBIRALGORITHM_NEW problem
      return IsWordInverse(i); 
    }

    ///
    bool IsPreCalculatedIndex(size_t i) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      return IndexIndexOK(i) && IndexData(NULL, i).Is("precalculated"); 
    }

    ///
    bool IsTsSom(size_t i) const {
      // obs! HAS_CBIRALGORITHM_NEW problem
      return IndexIndexOK(i) && IndexData(NULL, i).Is("tssom");
    }

    ///
    bool IsTsSomAndLevelOK(size_t i, size_t j) const {
      return IsTsSom(i) && TsSom(i).LevelOK(j);
    }

    ///
    VectorIndex::State& IndexDataVectorIndex(size_t i) {
      // obs! HAS_CBIRALGORITHM_NEW problem still?
      Index::State *s = &IndexData(FirstAlgorithm(), i);
      VectorIndex::State *p = dynamic_cast<VectorIndex::State*>(s);
      if (!p)
        ShowError("Query::IndexDataVectorIndex() failing with i="+ToStr(i)+
                  " fullname=<"+s->fullname+">");
      return *p;
    }

    ///
    const VectorIndex::State& IndexDataVectorIndex(size_t i) const {
      return ((Query*)this)->IndexDataVectorIndex(i);
    }

    ///
    TSSOM::State& IndexDataTSSOM(size_t i) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      Index::State *s = &IndexData(NULL, i);
      TSSOM::State *p = dynamic_cast<TSSOM::State*>(s);
      if (!p)
        ShowError("Query::IndexDataTSSOM() failing with i="+ToStr(i)+
                  " fullname=<"+s->fullname+"> because it is a "+
		  s->StaticPart()->MethodName());
      return *p;
    }

    ///
    const TSSOM::State& IndexDataTSSOM(size_t i) const {
      return ((Query*)this)->IndexDataTSSOM(i);
    }

    ///
    WordInverse::State& IndexDataWordInverse(size_t i) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      Index::State *s = &IndexData(NULL, i);
      WordInverse::State *p = dynamic_cast<WordInverse::State*>(s);
      if (!p)
        ShowError("Query::IndexDataWordInverse() failing with i="+ToStr(i)+
                  " fullname=<"+s->fullname+">");
      return *p;
    }

    ///
    const WordInverse::State& IndexDataWordInverse(size_t i) const {
      return ((Query*)this)->IndexDataWordInverse(i);
    }

    ///
    WordInverse::BinaryFeaturesData& BinaryFeatureData(size_t i) {
      return wordInverse(i).IsDynamicBinaryTextFeature() ?
        IndexDataWordInverse(i).binary_data : 
        wordInverse(i).StaticBinaryFeatureData();
    }

    ///
    const WordInverse::BinaryFeaturesData& BinaryFeatureData(size_t i) const {
      return ((Query*)this)->BinaryFeatureData(i);
    }

    ///
    PreCalculatedIndex::State& IndexDataPreCalculated(size_t i) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      Index::State *s = &IndexData(NULL, i);
      PreCalculatedIndex::State *p = 
	dynamic_cast<PreCalculatedIndex::State*>(s);
      if (!p)
        ShowError("Query::IndexDataPreCalculated() failing with i="+ToStr(i)+
                  " fullname=<"+s->fullname+">");
      return *p;
    }

    ///
    const PreCalculatedIndex::State& IndexDataPreCalculated(size_t i) const {
      return ((Query*)this)->IndexDataPreCalculated(i);
    }

    ///
    SVM::State& IndexDataSVM(size_t i) {
      // obs! HAS_CBIRALGORITHM_NEW problem
      Index::State *s = &IndexData(NULL, i);
      SVM::State *p = dynamic_cast<SVM::State*>(s);
      if (!p)
        ShowError("Query::IndexDataSVM() failing with i="+ToStr(i)+
                  " fullname=<"+s->fullname+">");
      return *p;
    }

    ///
    const SVM::State& IndexDataSVM(size_t i) const {
      return ((Query*)this)->IndexDataSVM(i);
    }

    /// Expands a query by running nrounds of pseudo relevance feedback. 
    /// Marks nobjects best-scoring objects as positive each round.
    bool ExpansionByPRF(int nrounds, int nobjects); 
    
    ///
    class classification_data_t {
    public:
      ///
      classification_data_t(const string& n = "", float v = 0.0) :
        classname(n), extrastring(""), extrafloat(-1.0), value(v) { }

      ///
      string classname;
      
      ///
      string extrastring;

      ///
      float extrafloat;

      ///
      float value;

    }; // class Query::classification_data_t

    ///
    const vector<classification_data_t>& ClassificationData() const {
      return classification_data;
    }
    
    ///
    class phase_rule_t {
    public:
      ///
      phase_rule_t(const string& a, const string& f, const string& t) :
        aspect(a), from(f), to(t) {}

      ///
      string str() const {
        stringstream ss;
        ss << "aspect=[" << aspect << "] from=\"" << from
           << "\" to=\"" << to << "\"";
        return ss.str();
      }

      ///
      string aspect;

      ///
      string from;

      ///
      string to;
    };  // class phase_rule_t

    ///
    bool AddPhaseRule(const string& a, const string& f, const string& t) {
      phase_rule.push_back(phase_rule_t(a, f, t));
      if (debug_phases)
        cout << "PHASE: added rule [" << phase_rule.back().str() << "]"
             << endl;
      return true;
    }

    ///
    bool PhaseChangeByAspect(const set<string>&);

    ///
    bool ChangePhase(const string&);

    ///
    void ErfPolicy(const string& s) { erfpolicy = s; }

    ///
    const string& ErfPolicy() const { return erfpolicy; }

    ///
    class html_image_data {
    public:
      ///
      html_image_data() : top(-1), left(-1), width(-1), height(-1),
                          x(-1), y(-1), square(false) {}  

      ///
      html_image_data(int t, int l, int w, int h, int xx, int yy,
                      bool q, const string& ty, const string& s) :
        top(t), left(l), width(w), height(h), x(xx), y(yy),
        square(q), type(ty), src(s) {}  

      int top;     ///
      int left;    ///
      int width;   ///
      int height;  ///
      int x;       ///
      int y;       ///
      bool square; ///
      string type; ///
      string src;  ///
      pair<size_t,size_t> collage_position, collage_size;

      string collage_spec() const {
        ostringstream ss;
        ss << "(" << collage_position.first << ","
           << collage_position.second << ")/"
           << collage_size.first << "x" << collage_size.second;
        return ss.str();
      }
    };

    ///
    string HTMLimgURLtoLabel(const string&) const;

    ///
    class erf_image_data {
    public:
      ///
      erf_image_data() : db(NULL), index(-1), keyclicks(0),
                         original_width(0), original_height(0) {}

      ///
      string keystring() const;
      
      ///
      string tagstring() const;
      
      ///
      string speechstring(const string&) const;
      
      ///
      double minimum_time_distance(const uiart::TimeSpec&,
				   const string&, uiart::TimeSpec&) const;

      ///
      double minimum_time_distance(const uiart::TimeSpec&,
				   const vector<uiart::GazeData>&,
				   uiart::TimeSpec&) const;

      ///
      string url;

      ///
      const DataBase *db;

      ///
      string label;

      ///
      int index;

      ///
      string key;

      ///
      size_t keyclicks;

      ///
      html_image_data position;
     
      ///
      size_t original_width, original_height;

      ///
      imagedata original;

      ///
      imagedata scaled;

      ///
      imagedata cumulative;

      ///
      vector<uiart::GazeData> gaze_sample_left;

      ///
      vector<uiart::GazeData> gaze_sample_right;

      ///
      vector<uiart::GazeData> gaze_sample_center;

      ///
      vector<uiart::GazeData> gaze_fixation_left;

      ///
      vector<uiart::GazeData> gaze_fixation_right;
      
      ///
      vector<uiart::GazeData> gaze_fixation_center;
      
      ///
      vector<uiart::GazeData> pointer_sample;

      ///
      vector<uiart::GazeData> pointer_click;

      ///
      typedef vector<pair<string,uiart::TimeSpec> > textdata_t;

      ///
      textdata_t keydata;

      ///
      textdata_t tagdata;

      ///
      textdata_t speechdata;

      ///
      map<string,vector<float> > features;

      ///
      map<string,float> relevance;

      ///
      list<map<string,string> > detections;

      ///
      vector<uiart::GazeData>& coorddata(bool pointer, int eye,
					 bool click_fixation) {
	if (pointer)
	  return click_fixation ? pointer_click : pointer_sample;
	
	if (eye==0)
	  return click_fixation ? gaze_fixation_left : gaze_sample_left;

	if (eye==1)
	  return click_fixation ? gaze_fixation_right : gaze_sample_right;

	if (eye==2)
	  return click_fixation ? gaze_fixation_center : gaze_sample_center;

	if (db)
	  db->ShowError("erf_image_data::coorddata() failed");

	static vector<uiart::GazeData> error;
	return error;
      }

      ///
      const vector<uiart::GazeData>& coorddata(bool pointer, int eye,
                                               bool click_fixation) const {
	return ((erf_image_data*)this)->coorddata(pointer, eye, click_fixation);
      }

    }; // class erf_image_data

    ///
    class seen_object_t {
    public:
      ///
      seen_object_t() {
	start = end = gaze_first = gaze_last = TimeZero();
      }

      ///
      seen_object_t(const string& o, const struct timespec& s, const struct timespec& e) :
	object(o), start(s), end(e) { }

      ///
      string str() const {
	ostringstream os;
	os << "\"" << object << "\" " << TimeString(start) << " -- "
	   << TimeString(end) << " gaze: " << TimeString(gaze_first) << " -- "
	   << TimeString(gaze_last);
	return os.str();
      }

      ///
      string object;

      ///
      struct timespec start, end, gaze_first, gaze_last;

    }; // class seen_object_t 

    ///
    class seen_word_t {
    public:
      ///
      seen_word_t(const string& w, const struct timespec& s, const struct timespec& e,
		  const string& i, float x, float y) :
	word(w), start(s), end(e), image(i), image_x(x), image_y(y) { }

      ///
      string str() const {
	ostringstream os;
	os << "\"" << word << "\" " << TimeString(start) << " -- "
	   << TimeString(end) << " [" << image << "] (" << image_x
	   << "," << image_y << ")";
	return os.str();
      }

      ///
      string word;

      ///
      struct timespec start, end;

      ///
      string image;

      ///
      float image_x, image_y;

    }; // class seen_word_t 

    ///
    class visited_link_t {
    public:
      ///
      visited_link_t(const string& u, const string& m, size_t s,
		     const struct timespec& t) :
	url(u), type(m), size(s), time(t) { }

      ///
      string str() const {
	ostringstream os;
	os << url << " " << type << " " << size << TimeString(time);
	return os.str();
      }

      ///
      string url;

      ///
      string type;

      ///
      size_t size;

      ///
      struct timespec time;

    }; // class visited_link_t 

    ///
    bool ProcessSpeechRecognizerResult(const string&, const uiart::TimeSpec&);

    ///
    list<string> ErfImageListLabels() const;

    ///
    list<string> ErfImageListSrcs() const;

    ///
    bool ErfImageDataExists(const string&, bool) const;

    ///
    erf_image_data& ErfImageDataByLabel(const string&);

    ///
    erf_image_data *ErfImageDataByTime(const uiart::TimeSpec&, const string&);

    ///
    erf_image_data& ErfImageData(const string&);

    ///
    const erf_image_data& ErfImageData(const string& l) const {
      return ((Query*)this)->ErfImageData(l);
    }

    ///
    const erf_image_data *ErfImageDataFixation(const uiart::TimeSpec&) const;

    ///
    erf_image_data *ErfImageDataFixation(const uiart::TimeSpec& t) {
      return (erf_image_data*)((const Query*)this)->ErfImageDataFixation(t);
    }

    ///
    list<string> ErfImageList() const {
      list<string> ret;

      for (map<string,erf_image_data>::const_iterator
             i=erf_image_data_cache.begin();
           i!=erf_image_data_cache.end(); i++)
        ret.push_back(i->first);

      return ret;
    }

    ///
    bool HasErfImageData() { return erf_image_data_cache.size(); }

    ///
    void ErfImageDataClear() { erf_image_data_cache.clear(); }

    ///
    string LabelInCollage(size_t, size_t) const;

    ///
    const string& ErfImageColor(const string&) const;

    ///
    const string& ErfImageColor(bool, bool) const;

    ///
    bool ConditionallyCalculateErfFeatures(int);

    ///
    bool ConditionallyCalculateErfFeatures(erf_image_data&,
                                           const erf_image_data&,
                                           int, XmlDom&, double);

    ///
    bool SolveGazeCenter(erf_image_data&);
    
    ///
    vector<float> CalculateGazeFeatures(const erf_image_data&,
                                        const erf_image_data&, int, int,
					double);
    ///
    double DetectGazeSamplingPeriod(const erf_image_data&);

    ///
    vector<float> CalculatePointerFeatures(const erf_image_data&,
                                           const erf_image_data&);

    ///
    vector<float> FeatureVector(const string& method,
				const string& label, bool recurse) const;

    ///
    float ThresholdGazeRelevance(float) const;

    ///
    bool ShowErfRelevances() const;

    ///
    bool ErfCheckAndSet_setaspect();

    ///
    bool ErfCheckAndSet_changetext();

    ///
    bool ErfCheck_gotopage();

    ///
    Query *CreateChildWith_gotopage();

    ///
    bool PrepareAfter_gotopage();

    ///
    bool MoveErfRelevancesToAspects();

    ///
    uiart::TimeSpec AjaxTimeSpec(const string&);

    ///
    bool ProcessAjaxRequest(const XmlDom&, bool, bool);

    ///
    bool ProcessAjaxDataPointer(const XmlDom&, bool);

    ///
    bool ProcessAjaxDataKeyboard(const XmlDom&);

    ///
    bool ProcessAjaxDataAudio(const XmlDom&);

    ///
    bool ProcessAjaxDataGaze(const XmlDom&);
    
    ///
    bool ProcessAjaxDataObjectTag(const XmlDom&);
    
    ///
    bool ProcessAjaxImageList(const XmlDom&);

    ///
    bool ProcessAjaxDataObjectDetection(const XmlDom&);
    
    ///
    XmlDom AjaxImageListToCollage(const XmlDom&);

    ///
    imagedata CombineImages(const imagedata&, const imagedata&);

    ///
    string AjaxSaveFileName(size_t i, bool cwd) const {
      char number[20];
      sprintf(number, "%05d", (int)i);
      string bname = Identity()+"_ajax_"+string(number)+".xml";

      return cwd ? bname : AjaxSaveFileName(bname);
    }

    ///
    string AjaxSaveFileName(const string& fn) const {
      string qname, sn = SavePath(false);
      size_t p = sn.rfind('/');
      if (p!=string::npos)
	qname = sn.substr(0, p+1)+fn;
      
      return qname;
    }

    ///
    static int AjaxSave() { return ajax_save; }

    ///
    static void AjaxSave(int v) { ajax_save = v; }

    ///
    static bool AjaxSaveAudio() { return ajax_save_audio; }

    ///
    static void AjaxSaveAudio(bool v) { ajax_save_audio = v; }

    ///
    static bool AjaxPlayAudio() { return ajax_play_audio; }

    ///
    static void AjaxPlayAudio(bool v) { ajax_play_audio = v; }

    ///
    static bool AjaxMplayer() { return ajax_mplayer; }

    ///
    static void AjaxMplayer(bool v) { ajax_mplayer = v; }

    ///
    static bool AjaxMplayerFile() { return ajax_mplayer_file; }

    ///
    static void AjaxMplayerFile(bool v) { ajax_mplayer_file = v; }

    /// A helper for the above ones.
    map<string,string> ParseXMLKeyValues(const XmlDom&, bool = false) const;

    /// A routine common to pointer and gaze data.
    bool ProcessAjaxImages(const XmlDom&, const uiart::TimeSpec&, float, float,
                           bool, int, bool, bool);


    /// A routine for gaze data.
    bool ProcessAjaxImagesCenter(const XmlDom&, const XmlDom&,
                                 const uiart::TimeSpec&, float, float,
                                 float, bool);

    /// A helper for the above ones.
    list<html_image_data> ParseXMLimgData(const XmlDom&, string&) const;

    /// A helper for ParseXMLimgData().
    bool SkipXMLimage(const string&, const string&) const;

    /// A helper for ProcessXMLimgData().
    bool XMLimgSquareCorrection(const erf_image_data&, bool,
                                int&, int&, int&, int&) const;

    ///
    bool SetCollageData(list<html_image_data>&) const;

    /// A helper for SetCollageData()
    pair<size_t,size_t> SolveCollagePosition(size_t, size_t) const;

    ///
    bool SolveRanges(vector<pair<size_t,size_t> >&, size_t, size_t) const;

    ///
    erf_image_data *ProcessXMLimgData(const html_image_data&,
                                      const uiart::TimeSpec&, float, float,
                                      bool, int, bool, bool, float, float);

    ///
    bool ErfInitializeImageData(erf_image_data&, const string&, bool);

    ///
    bool ProcessXMLcollageData(int, int, bool, bool, bool);

    ///
    bool CloseVideoOutput();

    ///
    const list<string>& Xsl() const { return xsl; }

    ///
    list<pair<string,string> > XsltParam() const;

    ///
    static const string& ErfKeyMapAlias(const string&);

    ///
    bool ErfStoreFeatures(XmlDom&, const string&, const string&,
			  const string&, float);

    ///
    bool ErfShowDataAndRecurse();

    ///
    imagedata ErfDetectionsImage(const string&, const string&);

    ///
    static string StringTailDotted(const string& s, size_t l) {
      return s.size()<=l ? s+string(l-s.size(), ' ') :
	"..."+s.substr(s.size()-l+3);
    }

    ///
    const map<string,string>& ErfDataSpeechString() const {
      return erfdata_speechstring;
    }

    ///
    const map<string,string>& ErfDataTagString() const {
      return erfdata_tagstring;
    }

    ///
    const string& TextIndex() const { return textindex; }

    ///
    const string& TextField() const { return textfield; }

    ///
    const string& QueryField() const { return queryfield; }

    ///
    string FindTextSnippet(size_t) const;

    ///
    string MakeTextSnippet(size_t);

    ///
    bool IncludeMinimal() const { return include_minimal; }

    ///
    const vector<string>& QueryDetections() const { 
      return querydetections; 
    }
    
  protected:
    ///
    vector<Index::State*> index_data;

    ///
    vector<classification_data_t> classification_data;
    
    /// Backreference.
    PicSOM *picsom;

    /// My unique name.
    string identity;

    /// Type of objects that are being searched for or otherwise processed.
    target_type query_target_;

    /// If this Query object is used for analysis then here it is.
    Analysis *analysis;

    /// My predecessor.
    Query *parent;

    /// My successors.
    vector<Query*> child_new;

    /// Algorithm used in CBIR.
    cbir_algorithm algorithm;

    /// Name of algorithm if external.
    string external_algorithm;

    ///
    vector<algorithm_data> algorithms;

    /// Type of stage_map.
    typedef map<cbir_stage,cbir_function> stage_map_t;

    /// Functions to call at each stage.
    stage_map_t stage_map;

    /// Type of stage_arg_map.
    typedef map<cbir_stage,string> stage_arg_map_t;

    /// Arguments to the stage functions.
    stage_arg_map_t stage_arg_map;

    /// Here comes a set of cbir-stage-wise parameters which
    /// are meaningful only to a set of stage functions.
    /// These *SHOULD* be passed as stage arguments...    

    /// Convolution type.
    string _sa_default_convtype;

    /// Convolution type.
    map<int,string> _sa_tssom_convtype;

    /// BMU Convolution type.
    string _sa_default_bmuconvtype;

    /// BMU Convolution type.
    map<int,string> _sa_tssom_bmuconvtype;

    /// New CBIR stage limit for XXX.
    int _sa_limit_per_map_units;
    
    /// New CBIR stage limit for XXX.
    int _sa_limit_total_units;
    
    /// New CBIR stage limit for XXX.
    int _sa_limit_per_map_images;
    
    /// Maximum number of yet unseen objects to be found on each map.
    int _sa_permapobjects;

    /// The used map level in the VQ/VQW algorithms.
    int _sa_vqlevel;

    /// Maximum number of question images to be returned.
    size_t maxquestions;

    /// Maximum number of iterative rounds.
    size_t maxrounds;

    // Page to show after an experiment
    string endpage;

    /// Status of SinglePass() / AnalysisXXX() between response and request.
    bool okstatus;

    /// True if response action is being run.
    bool inrun;

    /// Information on the first accessing client. 
    RemoteHost first_client;

    /// True if just logging the output of WriteControl().
    bool write_to_log;

    /// Each real query should have exactly one.
    DataBase *the_db;

    /// Database dependent setup.
    string setupname;

    /// Input data.
    ObjectList seenobject;

    /// This list saves the actual seen images if
    /// selective relevance distribution is in use
    ObjectList seenobjectreal;

    /// Intermediate list.
    ObjectList combinedimage;

    /// Intermediate list.
    ObjectList uniqueimage;

    /// Output data.
    ObjectList newobject;

    /** The set of the labels of all the objects that have been even once 
     *  selected in any node of this query tree.
     */
    set<string> *querytreeformerselections;

    /// Count of random images in newobject.
    int random_image_count;

    /// Pointed map index in 0,1,...
    int pointed_map;

    /// Pointed level index in 0,1,...
    int pointed_level;

    /// Point coordinates in [0,1]x[0,1].
    FloatPoint mappoint;

    /// These are resolved from the above by SelectAroundMapPoint().
    IntPoint maparea_ul, maparea_lr;

    /// Number of images to be shown per row in map zooms.
    int imgcols;

    /// Whether to show query statistics.
    int showstats;

    /// Human readable name of a query (or session in MRML)
    string name;
    
    /// Comment from the user.
    string comment;

    /// Name of class file to store the plus part of seenset.
    string setname;

    /// Comment text to be included in the above.
    string setname_comment;

    /// Last inserted image's label.
    string upload_label;

    /// Set of images to be show by ground truth.
    string viewclass;

    /// Set of images to be marked positive by ground truth.
    map<string,pair<string,float> > positive;

    /// Set of images to be marked negative by ground truth.
    map<string,pair<string,float> > negative;

    /// Restriction of an object subset in query-specific manner.
    ground_truth restriction;

    /// Creation time.
    struct timespec start;

    /// Last access time.
    struct timespec last_access;

    /// Last modification time.
    struct timespec last_modification;

    /// Time when written to disk.
    struct timespec saved_time;

    /// True if read from disk.
    bool from_disk;

    /// True when running AnalyseBest(). Affects CanBeShown().
    bool can_show_seen;

    /// 0 by defult, can be set to value 1 in AnalyseSimulate()..
    int restricted_objects_seen;

    /// Should segment index zero be neglected eg. as background?
    bool usezero;

    /// causes CanBeShown() to use logic can_show_child = 
    /// (can_show_child && can_show_parent)
    bool can_be_shown_and;

    /// causes PlaceSeenOnMapPicSOMthreeMaps() to limit the 
    /// relevance values originating from a single map unit
    bool limitrelevance;

    /// Turns off border compensation in ConvolveOneDimMask()
    /// if false
    bool bordercompensation;

    /// if true, forces the reading of .nbr files
    bool readnbr;

    /// True if one wants to see entropy values besides TS-SOMs on the browser.
    bool calculate_entropy_force;

    /// Number of matrices created for each TS-SOM level. 1 or 3 is typical.
    size_t matrixcount_xx;

    /// Index of matrix that is used for entropy calculation. -1 is default.
    int entropymatrix;

    /// The round number of this query.
    int round;

    /// Seed value for random number generator.
    int rndseed_value;

    /// Set to true if rndseed was specified as random.
    bool rndseed_random;

    /// Set to true if rndseed was specified as random each query round.
    /// (So that "restart query" produces new random images.)
    bool rndseed_random_each_round;

    /// Name prefix for writing convolved map s.
    string mapimagename;

    /// Name prefix for writing convolved map s.
    string mapmatlabname;

    /// Name prefix for writing convolved map s.
    string mapdatname;

    /// Name prefix for writing score components
    string scorefile;

    /// Set to true if the scores should be scaled down from one.
    bool relativescores;

    /// Set to false if we wish to skip normalizing hits over map.
    bool mapnormalize;

    /// Miscellaneous key=value pairs are sored here.
    map<string,string> keyvaluelist;

    /// True if you want to see all keys in AddOtherKeyValue().
    static bool debug_all_keys;

    /// Debugging of AJAX operations.
    static int debug_ajax;

    /// Debugging of CalculateGazeFeatures().
    static int debug_erf_features;

    /// 
    static int debug_erf_relevance;

    /// True if you want to see how temporal restriction is working.
    static bool debug_restriction;

    /// True if you want to see how SelectIndex/ices() is working.
    static bool debug_featsel;

    /// True if you want to see how *Aspect*() is working.
    static bool debug_aspects;

    /// True if you want to see how *Phase*() is working.
    static bool debug_phases;

    /// True if you want to see how PlaceSeenOnMap() is working.
    static bool debug_placeseen;

    /// True if you want to see how algorithm selection works.
    static bool debug_algorithms;

    /// True if you want to see how cbir_stages are changing.
    static bool debug_stages;

    /// True if you want to see vqunit and image lists.
    static int debug_lists;

    /// True if you want to see vqunit and image lists.
    static bool debug_check_lists;

    /// Set to true if zero values after convolutions are not problem.
    static bool zero_matrices_allowed;

    /// Filename for dumping object lists in the end of CbirStages().
    static string matlabdump;

    /// Filename for dumping object lists in the end of CbirStages().
    static string datdump;

    ///
    static IntVector watchset;

    ///
    static bool debug_info;

    /// True if you want to see how per TSSOM object weighting is done.
    static bool debug_weights;

    /// True if you want to see query parameters in CbirStages().
    static bool debug_parameters;

    /// True if you want to see how subobjects are treated.
    static bool debug_subobjects;

    /// True if you want to see how CanBeShown() works.
    static bool debug_can_be_shown;

    /// True if you want to see how DoClassification() works.
    static bool debug_classify;

    /// True if you want to see how ExpansionByPRF() works.
    static bool debug_prf;

    /// True if you want to see how relevance assesment of
    /// image segments works.
    static bool debug_selective;

    /// Combination factor for relevance combination between objects
    static float comb_factor;

    /// Combination factor for positive/negative relevance in Hits()
    /// used only if hitstype==posneg_lc
    static float hit_comb_factor;

    /// Positive if one wants to see "classification" of positives:
    /// value 1 represents classification within normal operation,
    /// value 2 represents analysis mode (AnalyseClass)
    int classify;

    /// The used method in DoClassification().
    string classifymethod;

    /// The number of proposed classes in DoClassification().
    int classifynclasses;

    /// The name of class METACLASSFILE to use in classification.
    string classificationset;

    /// List of actual class names to use in classification.
    vector<string> classmodel;

    /// Method-wise classification results.
    map<string,multimap<double,string> > classresult;

    static bool relevance_to_siblings;
    static struct relop_stage_type expand_operation;
    static struct relop_stage_type exchange_operation;
    static struct relop_stage_type converge_operation;

    static bool store_selfinfluence;

    static hitstype_t string2hitstype_t(const string &s){
      if(s=="trad")
        return trad;
      else if(s=="smoothed_fraction")
        return smoothed_fraction;
      else if(s=="priori_compensated_fraction")
        return priori_compensated_fraction;
      else if(s=="smoothed_log")
        return smoothed_log;
      else if(s=="posneg_lc")
        return posneg_lc;
      else if(s=="bernoullimap")
        return bernoullimap;
      else
        return unknown;
    }

    ///
    static hitstype_t hitstype;

    ///
    bool backfromsettings;

    ///
    bool skipoptionpage;

    // 1 if greymaps with grey zero and 2 if greymaps with white zero.
    int greymaps;

    // If non-MAXFLOAT, maximum value used in mapping values to colors.
    float maxposvalue;

    // If non-MAXFLOAT, maximum value used in mapping values to colors.
    float maxnegvalue;

    // Relevance predicted from gaze patterns is set to this value if image is not seen.
    float default_gaze_relevance;

    // Set to "eng" or "fin" if speech recognition is desired.
    string speechrecognizer;

    // Either "pointer", "gaze" or "gaze-left|right|center"
    string speechsync, keysync;

#ifdef USE_MRML
    /// !! for MRML implementation of Analysis class !!
    /// indicates whether MRML server functions PicSOM or GIFT style
    /// (i.e. does server return user-relevant elements (seen images) or not)
    bool mrmlserver_repeats_oldimages;

    /// used by Analysis class, stores collection-id temporarily here for 
    /// connecting back & configuration using collection-id, which also 
    /// could be known in advance..
    string collection_id;

    /// used by Analysis class, stores algorithm-id temporarily here for 
    /// connecting back & configuration using algorithm-id which also
    /// could be known in advance..
    string algorithm_id; 

    /// used by Analysis class, stores featurelist temporarily here from mytau
    string listoffeatures; 

#endif //USE_MRML

#ifdef QUERY_USE_PTHREADS
    /// If running parallel pthread for TSSOM's this is the initial one.
    pthread_t query_main_thread;

    ///
    bool main_thread_set;

    /// Read/write lock for making requests wait for response to finish.
    pthread_rwlock_t rwlock;
#endif // QUERY_USE_PTHREADS

    /// Used in caching by OriginsInfo().
    typedef map<string,map<string,string> > origins_info_t;

    /// Cache of calls to DataBase::ReadOriginsInfo().
    origins_info_t origins_info;

    /// Textual query.
    string textquery;

    /// Textual query.
    string implicit_textquery;

    ///
    string textquery_processed;

    /// Bug compatibility mode.
    static bool pre2122bug;

    ///
    static int ajax_save;

    ///
    static bool ajax_save_audio;

    ///
    static bool ajax_play_audio;

    ///
    static bool ajax_mplayer;

    ///
    static bool ajax_mplayer_file;

  public:
    ///
    bool textquery_correct_person;

    ///
    bool textquery_correct_location;

    ///
    bool check_commons_classes;

    /// whether to require the aspect "concept" for features to create
    /// a class model
    bool commons_classes_useaspects;

    ///
    bool add_classfeatures;

  protected:
    ///
    string commonsfile;
                         
    ///
    string classfeaturesfile;                     

    ///
    classaugment_t classaugment;

    ///
    list<pair<ground_truth,float> > classweight;

    /// storage for self-influence values of seen images
    map<int,vector<float> > selfinfluence;

    ///
    vector<vector<vector<float> > > location_selfinfluence;

    ///
    map<string,list<pair<string,string> > > phase_script;

    ///
    list<phase_rule_t> phase_rule;

    ///
    string phase_current;

    /// ajax data piece by piece
    // list<XmlDom> ajax_data_pieces;

    size_t ajax_data_piece_count;

    /// ajax data all in one
    XmlDom ajax_data_all;

    ///
    videofile ajax_image_mplayer;

    ///
    map<string,erf_image_data> erf_image_data_cache;

    ///
    int ajax_image_x, ajax_image_y;

    ///
    erf_image_data *ajax_image_p;

    ///
    XmlDom ajax_collage_xml;

    ///
    map<string,string> collage_position;

    ///
    vector<pair<size_t,size_t> > collage_ranges_x;

    ///
    vector<pair<size_t,size_t> > collage_ranges_y;

    ///
    map<string,string> erf_image_lab2src;

    ///
    imagedata ajax_collage_image;

    ///
    imagedata ajax_collage_image_cumulative;

    ///
    videofile ajax_collage_mplayer;

    ///
    int erf_features_when;

    ///
    string erfpolicy;

    ///
    string erfkeymap;

    ///
    bool erfsplitdata;

    ///
    bool erfptrstogaze;

    ///
    static ofstream erfdatafile;

    ///
    float negative_weight;

    ///
    multimap<string,string> ajaxresponse;

    ///
    bool ajax_gotopage_sent;

    ///
    bool forcecbirstages;

    ///
    list<string> xsl;

    ///
    string tnsize;

    ///
    size_t tncolumns;

    ///
    aspects aspect_weight;

    ///
    map<string,string> erfdata_speechstring;

    ///
    map<string,string> erfdata_tagstring;

    ///
    string initialset_pre;

    ///
    string textsearch;

    ///
    string textindex;

    ///
    string textfield;

    ///
    string queryfield;

    ///
    vector<textsearchresult_t> textsearchresult;

    ///
    map<size_t,string> textsnippetcache;

    ///
    bool use_google_image_search;

    /// These control amount of information stored in AddtoXML*().
    bool include_minimal;

    ///
    bool include_snippet;

    ///
    bool include_origins_info;

    ///
    map<string,seen_object_t> seen_objects;

    ///
    list<seen_word_t> seen_words;

    ///
    list<visited_link_t> visited_links;

    ///
    map<string,list<vector<string> > > sparql_query_cache;

    ///
    vector<string> querydetections;
    
  };  // class Query

  class NewtonOpt {
  public:
    virtual ~NewtonOpt() {}

    vector<double> Optimise(const vector<double> &initial);
    
    int nparam;

  protected:

    virtual double CalculateFval(double *w)=0;
    virtual void CalculateGradient()=0;
    virtual void CalculateHessian()=0;
    virtual void NewWPrecalculate(){};
    virtual void InitOpt(){}; // place for memory allocation etc.
    virtual void EndOpt(){}; // place for freeing memory etc.

    void InvertHessian();

    double *current;
    double *newtheta;
    double *gradient;
    double **hessian;
    double *hdata;
  };

  class MapSurfaceMAPOpt : public NewtonOpt{
  public:
    int w;
    int h;
    MapSurfaceMAPOpt(){
      w=h=0;
      cdata=NULL; 
      c_inv=NULL;
      uval=NULL;
    };
    virtual ~MapSurfaceMAPOpt(){
      delete[] c_inv;
      delete[] cdata;
    };
     
    void ConstructInverseCovariance();
    void SetHitCounts(const int *p, const int *n)
    {pos=p; neg=n;}

    static double ParamToProb(double d);
    static void ParamToProb(vector<double> &v);

  protected:

    virtual double CalculateFval(double *w);
    virtual void CalculateGradient();
    virtual void CalculateHessian();
    virtual void NewWPrecalculate();

    virtual void InitOpt(); // place for memory allocation etc.
    virtual void EndOpt(); // place for freeing memory etc.

    double **c_inv;
    double *cdata;
    double *uval;
    const int *pos;
    const int *neg;
  };


  void InvertMatrix(double **a, int n);
  std::vector<float> MSEFitParabola(const std::vector<float>&,
                                    const std::vector<float>&);

  class floatEvaluator{
  public:
    virtual float eval(float)=0;
    virtual ~floatEvaluator(){}
  };

  void mnbrak(float *ax, float *bx, float *cx, 
              float *fa, float *fb, float *fc,
              floatEvaluator *func);

  float golden(float ax, float bx, float cx, floatEvaluator *f, float tol,
               float *xmin, float fb);

  float brent(float ax, float bx, float cx, floatEvaluator *f, float tol,
              float *xmin, float fb);

} // namespace picsom

#endif // _PICSOM_QUERY_H_

