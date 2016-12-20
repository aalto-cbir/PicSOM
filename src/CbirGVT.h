// -*- C++ -*-  $Id: CbirGVT.h,v 2.6 2011/03/23 08:43:03 jorma Exp $
// 
// Copyright 1998-2011 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _CBIRGVT_H_
#define _CBIRGVT_H_

#include <CbirAlgorithm.h>

using namespace std;

namespace picsom {
  ///
  static const string CbirGVT_h_vcid =
    "@(#)$Id: CbirGVT.h,v 2.6 2011/03/23 08:43:03 jorma Exp $";

  /**
  */
  class CbirGVT : public CbirAlgorithm {
    ///
  public:
    /**

    */
    class QueryData : public CbirAlgorithm::QueryData {
      ///
      friend class CbirGVT;

    public:
      /// poison
      QueryData() { abort(); }

      ///
      QueryData(const CbirAlgorithm *a, const Query *q) :
	CbirAlgorithm::QueryData(a, q) {}

      ///
      virtual CbirAlgorithm::QueryData *Copy(const Query *q) const {
	QueryData *copy = new QueryData(*this);
	copy->SetQuery(q);
	copy->CopyIndexData();

	return copy;
      }

    protected:

    }; // class CbirGVT::QueryData

    /// A utility routine for the virtual methods.
    static CbirGVT::QueryData *CastData(CbirAlgorithm::QueryData *qd) {
      CbirGVT::QueryData *qde = static_cast<CbirGVT::QueryData*>(qd);
      if (!qde) {
	ShowError("CbirGVT::QueryData pointer conversion failed");
	abort();
      }
      return qde;
    }

    /**

    */
    class IndexData : public CbirAlgorithm::IndexData {

    }; // classCbirGVT::IndexData

    /// Poisoned constructor.
    CbirGVT() : CbirAlgorithm() {}

    /// This does not need to do anything else but call the base class
    /// constructor.  If the class has any non-trivial data members
    /// then they might be initialized here.
    CbirGVT(DataBase*, const string&);

    /// This constructor just creates the "factory" instance of the class.
    CbirGVT(bool) : CbirAlgorithm(false) {}

    /// Does not need to be defined.
    virtual ~CbirGVT() {}

    /// This method creates the true usable instance.
    virtual CbirAlgorithm *Create(DataBase *db, const string& s) const {
      return new CbirGVT(db, s);
    }

    /// This method creates the query-specific data structures.
    virtual CbirAlgorithm::QueryData *CreateQueryData(const CbirAlgorithm *a,
						      const Query *q) const {
      const CbirGVT *pa = static_cast<const CbirGVT*>(a);
      if (!pa)
        return NULL;

      QueryData *qd = new QueryData(a, q);

      return qd;
    }
    
    /// Name of the algorithm.  Needs to be defined.
    virtual string BaseName() const { return "gvt"; }

    /// A bit longer name.  Needs to be defined.
    virtual string LongName() const { return "GVT algorithm"; }

    /// Short description  of the algorithm.  Needs to be defined.
    virtual string Description() const {
      return "This is the GVT algorithm.";
    }
    
    /// If we had some specifiers they could be appended to BaseName()...
    virtual string FullName() const {
      // currently the parameters need to be in alphabetical order!
      string fn = BaseName();

      return fn;
    }

    /** Virtual method that returns a list of parameter specifications.
	\return list of parameter specifications.
    */
    virtual const list<parameter>& Parameters() const;

    /// Initialization with query instance and arguments from
    /// algorithm=gvt(arguments) .  Does not need to be defined.
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

    /// Returns true if argument is valid value for linrel.
    static bool Valid_linrel(const string& s) {
      return s=="AUCB" || s=="1UCB" || s=="AExp" || s=="Comb";
    }

    /// Returns true if argument is valid value for kerneltype.
    static bool Valid_kerneltype(const string& s) {
      return s=="linr" || s=="gaus";
    }
     /// Returns true if argument is valid value for mkl.
    static bool Valid_mkl(const string& s) {
      return s=="none" || s=="rr" || s=="1c" || s=="2c";
    }

    /// Returns true if argument is valid value for feat.
    static bool Valid_feat(const string& s) {
      return s=="raw" || s=="fsh" || s=="som";
    }

    ///
    ground_truth GetGvtImages(const ground_truth& gt, size_t nn,
			      size_t nobj, const ground_truth& al) {
      vector<size_t> idx = gt.indices(1), nnvec(idx.size(), nn);
      return GetGvtImages(idx, nnvec, nobj, al);
    }

    ///
    ground_truth GetGvtImages(const vector<size_t>&, const vector<size_t>&,
			      size_t, const ground_truth&);

    ///
    void *GetGvtImagesCommon(string&) const;

    ///
    ground_truth GetGvtImagesOneByOne(size_t, size_t);

    ///
    ground_truth GetGvtImagesMany(const vector<size_t>&,
				  const vector<size_t>&,
				  const ground_truth&);

    ///
    ground_truth GetGvtImagesManyNew(const vector<size_t>&,
				     const vector<size_t>&,
				     size_t, const ground_truth&);

    ///
    void AddCache(size_t, size_t, size_t);

    ///
    vector<size_t> FindCache(size_t, size_t, bool&);

  protected:
    ///
    size_t debug;

    ///
    bool lastroundonly;

    ///
    typedef multimap<size_t,pair<size_t,vector<size_t> > > cache_t;

    ///
    typedef cache_t::value_type cache_e;

    ///
    cache_t cache;
    
  }; // class CbirGVT

} // namespace picsom

#endif // _CBIRGVT_H_
