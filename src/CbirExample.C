// -*- C++ -*-  $Id: CbirExample.C,v 2.26 2010/03/16 23:28:42 jorma Exp $
// 
// Copyright 1998-2010 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <CbirExample.h>
#include <Valued.h>
#include <Query.h>

namespace picsom {
  ///
  static const string CbirExample_C_vcid =
    "@(#)$Id: CbirExample.C,v 2.26 2010/03/16 23:28:42 jorma Exp $";

  /// This is the "factory" instance of the class.
  static CbirExample list_entry(true);

  //===========================================================================

  /// This is the principal constructor.
  CbirExample::CbirExample(DataBase *db, const string& args)
    : CbirAlgorithm(db, args) {

    string hdr = "CbirExample::CbirExample() : ";
    if (debug_stages)
      cout << hdr+"constructing an instance" << endl;

    // After calling CbirAlgorithm::CbirAlgorithm(DataBase*) we should
    // have a valid DataBase pointer named database:

    cout << hdr << "we are using database \"" << database->Name()
	 << "\" that contains " << database->Size() << " objects" << endl;

    // Through the database pointer we have also indirect access to
    // the main PicSOM object.  The PicSOM object pointed to by
    // Picsom() has lots of useful information, for example:
    cout << hdr << "UserName()=\"" << Picsom()->UserName() << "\"" << endl;
    cout << hdr << "HostName()=\"" << Picsom()->HostName() << "\"" << endl;
    cout << hdr << "Cwd()=\""      << Picsom()->Cwd()      << "\"" << endl;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  const list<CbirAlgorithm::parameter>& CbirExample::Parameters() const {
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
  bool CbirExample::Initialize(const Query *q, const string& s,
			       CbirAlgorithm::QueryData*& qd) const {
    string hdr = "CbirExample::Initialize() : ";
    if (!CbirAlgorithm::Initialize(q, s, qd))
      return ShowError(hdr+"CbirAlgorithm::Initialize() failed");

    // After calling CbirAlgorithm::Initialize() we should have a valid Query
    // pointer named qd->query:

    cout << hdr << "query's identity is \"" << GetQuery(qd)->Identity()
	 << "\"" << endl;
    cout << hdr << "query's target type is \""
	 << TargetTypeString(GetQuery(qd)->Target()) << "\"" << endl;

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirExample::AddIndex(CbirAlgorithm::QueryData *qd,
			       Index::State *f) const {
    string hdr = "CbirExample::AddIndex() : ";
    if (!CbirAlgorithm::AddIndex(qd, f))
      return ShowError(hdr+"CbirAlgorithm::AddIndex() failed");

    // After calling CbirAlgorithm::AddIndex() we should have a list
    // of feature names in variable named features:

    cout << hdr << "the following features have been selected: ";
    for (size_t i=0; i<Nindices(qd); i++)
      cout << (i?", ":"") << IndexFullName(qd, i);
    cout << endl;

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first checks for its own parameters and then calls the
  /// corresponding method of the base class.
  bool CbirExample::Interpret(CbirAlgorithm::QueryData *qd, const string& k,
			      const string& v, int& r) const {
    string hdr = "CbirExample::Interpret() : ";

    if (debug_stages)      
      cout << hdr+"key=\""+k+" value=\""+v+"\"" << endl;

    CbirExample::QueryData *qde = CastData(qd);

    r = 1;

    int intval = atoi(v.c_str());

    if (k=="seed") {
      qde->seed = intval;
      return true;
    }

    return CbirAlgorithm::Interpret(qd, k, v, r);
  }

  //===========================================================================

  /// This virtual method really needs to be overwritten in a derived
  /// class.
  ObjectList CbirExample::CbirRound(CbirAlgorithm::QueryData *qd,
				    const ObjectList& seen,
				    size_t maxq) const {
    string hdr = "CbirExample::CbirRound() : ";

    CbirExample::QueryData *qde = CastData(qd);

    // This is called only for CbirAlgorithm::CbirRound()'s logging ability
    // it will return an empty list which is NOT an indication of a failure
    CbirAlgorithm::CbirRound(qde, seen, maxq);

    cout << hdr << "starting with " << seen.size() << " object already seen"
	 << endl;

    // We can access the results of the other algorithms.
    cout << hdr << "we have " << GetQuery(qde)->NlowerAlgorithms(qde)
	 << " \"lower\" CBIR algorithms ready with their results" << endl;
    for (size_t i=0; i<GetQuery(qde)->NlowerAlgorithms(qde); i++) {
      const Query::algorithm_data& a
	= *GetQuery(qde)->LowerAlgorithm(qde, i);
      cout << " index="    << a.index         << endl;
      cout << " fullname=" << a.fullname      << endl;
      cout << " objects="  << a.result.size() << endl;
    }

    size_t nobj = qde->GetMaxQuestions(maxq);

    // This simple algorithm just returns nobj number of yet unseen
    // objects.  The objects need to be of the same target_type as
    // requested by the Query object.

    vector<size_t> idx;
    for (size_t i=0; i<database->Size(); i++)
      if (database->ObjectsTargetTypeContains(i, GetQuery(qde)->Target()) &&
	  GetQuery(qde)->CanBeShownRestricted(i, true))
	idx.push_back(i);

    cout << hdr << "selecting " << nobj << " objects of type \""
	 << TargetTypeString(GetQuery(qde)->Target()) << "\" out of "
	 << idx.size() << " unseen ones with seed " << qde->seed << endl;

    RandVar rndvar(qde->seed);
    ObjectList list;
    while (!idx.empty() && (nobj==0 || list.size()<nobj)) {
      size_t rnd = (size_t)rndvar.RandomInt(idx.size());
      list.push_back(Object(database, idx[rnd], select_question));
      idx.erase(idx.begin()+rnd);
    }

    cout << hdr << "returning " << list.size() << " objects" << endl;
      
    return list;
  }

  //===========================================================================

} // namespace picsom

