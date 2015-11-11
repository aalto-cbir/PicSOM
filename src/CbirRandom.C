// -*- C++ -*-  $Id: CbirRandom.C,v 2.5 2011/03/30 10:44:24 jorma Exp $
// 
// Copyright 1998-2010 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <CbirRandom.h>
#include <Valued.h>
#include <Query.h>

namespace picsom {
  ///
  static const string CbirRandom_C_vcid =
    "@(#)$Id: CbirRandom.C,v 2.5 2011/03/30 10:44:24 jorma Exp $";

  /// This is the "factory" instance of the class.
  static CbirRandom list_entry(true);

  //===========================================================================

  /// This is the principal constructor.
  CbirRandom::CbirRandom(DataBase *db, const string& args)
    : CbirAlgorithm(db, args) {

    string hdr = "CbirRandom::CbirRandom() : ";

    debug = false;

    if (debug_stages)
      cout << TimeStamp() << hdr << "we are using database \""
	   << database->Name() << "\" that contains " << database->Size()
	   << " objects" << endl;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  const list<CbirAlgorithm::parameter>& CbirRandom::Parameters() const {
    static list<parameter> l;
    if (l.empty()) {
      // parameter bot = { "bottom",   "xsd:boolean", "", "true",
      //                   "description" } ;
      // parameter wei = { "weighted", "xsd:boolean", "", "false",
      //                   "description" } ;
      // l.push_back(bot);
      // l.push_back(wei);
    }

    return l;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirRandom::Initialize(const Query *q, const string& s,
			       CbirAlgorithm::QueryData*& qd) const {
    string hdr = "CbirRandom::Initialize() : ";
    if (!CbirAlgorithm::Initialize(q, s, qd))
      return ShowError(hdr+"CbirAlgorithm::Initialize() failed");

    // After calling CbirAlgorithm::Initialize() we should have a valid Query
    // pointer named qd->query:

    cout << TimeStamp() << hdr << "query's identity is \""
	 << GetQuery(qd)->Identity() << "\"" << endl;
    cout << TimeStamp() << hdr << "query's target type is \""
	 << TargetTypeString(GetQuery(qd)->Target()) << "\"" << endl;

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirRandom::AddIndex(CbirAlgorithm::QueryData *qd,
			       Index::State *f) const {
    string hdr = "CbirRandom::AddIndex() : ";
    if (!CbirAlgorithm::AddIndex(qd, f))
      return ShowError(hdr+"CbirAlgorithm::AddIndex() failed");

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first checks for its own parameters and then calls the
  /// corresponding method of the base class.
  bool CbirRandom::Interpret(CbirAlgorithm::QueryData *qd, const string& k,
			      const string& v, int& r) const {
    string hdr = "CbirRandom::Interpret() : ";

    if (debug_stages)      
      cout << TimeStamp()
	   << hdr+"key=\""+k+"\" value=\""+v+"\"" << endl;

    CbirRandom::QueryData *qde = CastData(qd);

    r = 1;

    int intval = atoi(v.c_str());

    if (k=="debug") {
      ((CbirRandom*)this)->debug = IsAffirmative(v);
      return true;
    }

    if (k=="seed") {
      qde->seed = intval;
      return true;
    }

    return CbirAlgorithm::Interpret(qd, k, v, r);
  }

  //===========================================================================

  /// This virtual method really needs to be overwritten in a derived
  /// class.
  ObjectList CbirRandom::CbirRound(CbirAlgorithm::QueryData *qd,
				    const ObjectList& seen,
				    size_t maxq) const {
    string hdr = "CbirRandom::CbirRound() : ";

    bool do_debug = debug_stages || debug;

    CbirRandom::QueryData *qde = CastData(qd);

    // This is called only for CbirAlgorithm::CbirRound()'s logging ability
    // it will return an empty list which is NOT an indication of a failure
    CbirAlgorithm::CbirRound(qde, seen, maxq);

    if (do_debug)
      cout << TimeStamp()
	   << hdr << "starting with " << seen.size() << " object already seen"
	   << endl;

    const Query::algorithm_data *lower
      = GetQuery(qd)->LowerAlgorithm(qd, 0, false);

    set<size_t> lowerset;
    if (lower) {
      for (size_t i=0; i<lower->result.size(); i++)
	if (lower->result[i].Retained())
	  lowerset.insert(lower->result[i].Index());

      if (do_debug)
	cout << TimeStamp() << hdr << "lower algorithm [" << lower->fullname
	     << "] returns " << lowerset.size() << "/" << lower->result.size()
	     << " objects" << endl;
    }

    size_t nobj = qde->GetMaxQuestions(maxq), nobjeff = nobj-lowerset.size();

    if (nobj<lowerset.size()) {
      ShowError(hdr+"nobj<lowerset.size()");
      nobjeff = 0;
    }

    // This simple algorithm just returns nobj number of yet unseen
    // objects.  The objects need to be of the same target_type as
    // requested by the Query object.

    ObjectList list;
    if (lower)
      for (size_t i=0; i<lower->result.size(); i++)
	if (lower->result[i].Retained())
	  list.push_back(Object(database, lower->result[i].Index(),
				select_question));

    stringstream nobjstrss;
    nobjstrss << nobj << "-" << lowerset.size() << "=" << nobjeff;
    string nobjstr = nobjstrss.str();
      
    bool old_and_slow = database->Size()<4000000;

    if (old_and_slow) {
      vector<size_t> idx;
      for (size_t i=0; i<database->Size(); i++)
	if (database->ObjectsTargetTypeContains(i, GetQuery(qde)->Target()) &&
	    GetQuery(qde)->CanBeShownRestricted(i, true) &&
	    (lowerset.empty() || lowerset.find(i)==lowerset.end()))
	  idx.push_back(i);

      if (do_debug)
	cout << TimeStamp()
	     << hdr << "selecting " << nobjstr << " random objects of type \""
	     << TargetTypeString(GetQuery(qde)->Target()) << "\" out of "
	     << idx.size() << " unseen&unselected ones with seed "
	     << qde->seed << endl;

      RandVar rndvar(qde->seed);
      while (!idx.empty() && (nobj==0 || list.size()<nobj)) {
	size_t rnd = (size_t)rndvar.RandomInt(idx.size());
	list.push_back(Object(database, idx[rnd], select_question));
	idx.erase(idx.begin()+rnd);
      }

    } else {
      if (do_debug)
	cout << TimeStamp()
	     << hdr << "selecting " << nobjstr << " random objects of type \""
	     << TargetTypeString(GetQuery(qde)->Target()) << "\" out of "
	     << "UNKNOWN NUMBER OF" << " unseen&unselected ones with seed "
	     << qde->seed << endl;

       RandVar rndvar(qde->seed);
       while (list.size()<nobj) {
	 size_t i = (size_t)rndvar.RandomInt(database->Size());
	 if (database->ObjectsTargetTypeContains(i, GetQuery(qde)->Target()) &&
	     GetQuery(qde)->CanBeShownRestricted(i, true) &&
	     (lowerset.empty() || lowerset.find(i)==lowerset.end()))
	   list.push_back(Object(database, i, select_question));
       }
    }

    if (do_debug||debug_lists)
      cout << TimeStamp()
	   << hdr << "returning " << list.size() << " objects" << endl;

    return list;
  }

  //===========================================================================

} // namespace picsom

