// -*- C++ -*-  $Id: CbirExample.h,v 2.24 2010/03/16 23:27:59 jorma Exp $
// 
// Copyright 1998-2010 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _CBIREXAMPLE_H_
#define _CBIREXAMPLE_H_

#include <CbirAlgorithm.h>

using namespace std;

namespace picsom {
  ///
  static const string CbirExample_h_vcid =
    "@(#)$Id: CbirExample.h,v 2.24 2010/03/16 23:27:59 jorma Exp $";

  /**
     An example of CBIR algorithm implementation derived from the abstract
     CbirAlgorithm class.  Copy CbirExample.h to CbirYourMethod.h and 
     CbirExample.C to CbirYourMethod.C and edit them as you wish.
  */
  class CbirExample : public CbirAlgorithm {

  public:
  public:
    /**

    */
    class QueryData : public CbirAlgorithm::QueryData {
      ///
      friend class CbirExample;

    public:
      /// poison
      QueryData() { abort(); }

      ///
      QueryData(const CbirAlgorithm *a, const Query *q) :
	CbirAlgorithm::QueryData(a, q), seed(123) {}

      ///
      virtual CbirAlgorithm::QueryData *Copy(const Query *q) const {
	QueryData *copy = new QueryData(*this);
	copy->SetQuery(q);
	copy->CopyIndexData();

	return copy;
      }

    protected:
      ///
      int seed;

    }; // class CbirExample::QueryData

    /// A utility routine for the virtual methods.
    static CbirExample::QueryData *CastData(CbirAlgorithm::QueryData *qd) {
      CbirExample::QueryData *qde = static_cast<CbirExample::QueryData*>(qd);
      if (!qde) {
	ShowError("CbirExample::QueryData pointer conversion failed");
	abort();
      }
      return qde;
    }

    /**

    */
    class IndexData : public CbirAlgorithm::IndexData {

    }; // classCbirExample::IndexData

    /// Poisoned constructor.
    CbirExample() : CbirAlgorithm() {}

    /// This does not need to do anything else but call the base class
    /// constructor.  If the class has any non-trivial data members
    /// then they might be initialized here.
    CbirExample(DataBase*, const string&);

    /// This constructor just creates the "factory" instance of the class.
    CbirExample(bool) : CbirAlgorithm(false) {}

    /// Does not need to be defined.
    virtual ~CbirExample() {}

    /// This method creates the true usable instance.
    virtual CbirAlgorithm *Create(DataBase *db, const string& s) const {
      return new CbirExample(db, s);
    }

    /// This method creates the query-specific data structures.
    virtual CbirAlgorithm::QueryData *CreateQueryData(const CbirAlgorithm *a,
						      const Query *q) const {
      return new QueryData(a, q);
    }
    
    /// Name of the algorithm.  Needs to be defined.
    virtual string BaseName() const { return "example"; }

    /// A bit longer name.  Needs to be defined.
    virtual string LongName() const { return "example algorithm"; }

    /// Short description  of the algorithm.  Needs to be defined.
    virtual string Description() const {
      return "This is an example algorithm.";
    }
    
    /// If we had some specifiers they could be appended to BaseName()...
    // virtual string FullName() const { return BaseName(); }

    /** Virtual method that returns a list of parameter specifications.
	\return list of parameter specifications.
    */
    virtual const list<parameter>& Parameters() const;

    /// Initialization with query instance and arguments from
    /// algorithm=example(arguments) .  Does not need to be defined.
    virtual bool Initialize(const Query*, const string&,
			    CbirAlgorithm::QueryData*&) const;

    /// Something can be done when features have been specified.  Does
    /// not need to be defined.
    virtual bool AddIndex(CbirAlgorithm::QueryData*, Index::State*) const;

    /// Does not need to be defined if the algorithm does not have any
    /// tunable parameters of its own.
    virtual bool Interpret(CbirAlgorithm::QueryData *qd, const string& key,
			   const string& value, int& res) const;

    /// This method implements the actual CBIR algorithm.  Needs to be
    /// defined.
    virtual ObjectList CbirRound(CbirAlgorithm::QueryData*, const ObjectList&,
				 size_t) const;

  protected:
    /// Here would come the query-independent data if such existed.

  }; // class CbirExample

} // namespace picsom

#endif // _CBIREXAMPLE_H_
