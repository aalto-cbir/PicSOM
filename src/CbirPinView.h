// -*- C++ -*-  $Id: CbirPinView.h,v 2.14 2010/12/15 10:25:18 jorma Exp $
// 
// Copyright 1998-2010 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _CBIRPINVIEW_H_
#define _CBIRPINVIEW_H_

#include <CbirAlgorithm.h>

using namespace std;

namespace picsom {
  ///
  static const string CbirPinView_h_vcid =
    "@(#)$Id: CbirPinView.h,v 2.14 2010/12/15 10:25:18 jorma Exp $";

  /**
     An example of CBIR algorithm implementation derived from the abstract
     CbirAlgorithm class.  Copy CbirPinView.h to CbirYourMethod.h and 
     CbirPinView.C to CbirYourMethod.C and edit them as you wish.
  */
  class CbirPinView : public CbirAlgorithm {
    ///
  public:
    /**

    */
    class QueryData : public CbirAlgorithm::QueryData {
      ///
      friend class CbirPinView;

    public:
      /// poison
      QueryData() { abort(); }

      ///
      QueryData(const CbirAlgorithm *a, const Query *q) :
	CbirAlgorithm::QueryData(a, q),linrel("AUCB"), mkl("none"),
        tensor(false), feat("raw"), ridge(0.1),kerneltype("linr") {}

      ///
      virtual CbirAlgorithm::QueryData *Copy(const Query *q) const {
	QueryData *copy = new QueryData(*this);
	copy->SetQuery(q);
	copy->CopyIndexData();

	return copy;
      }

      ///
      vector<size_t> indexvec;

    protected:
      ///
      string linrel;

      ///
      string mkl;

      ///
      bool tensor;

      ///
      string feat;

      ///
      float ridge;

      ///
      string kerneltype;

    }; // class CbirPinView::QueryData

    /// A utility routine for the virtual methods.
    static CbirPinView::QueryData *CastData(CbirAlgorithm::QueryData *qd) {
      CbirPinView::QueryData *qde = static_cast<CbirPinView::QueryData*>(qd);
      if (!qde) {
	ShowError("CbirPinView::QueryData pointer conversion failed");
	abort();
      }
      return qde;
    }

    /**

    */
    class IndexData : public CbirAlgorithm::IndexData {

    }; // classCbirPinView::IndexData

    /// Poisoned constructor.
    CbirPinView() : CbirAlgorithm() {}

    /// This does not need to do anything else but call the base class
    /// constructor.  If the class has any non-trivial data members
    /// then they might be initialized here.
    CbirPinView(DataBase*, const string&);

    /// This constructor just creates the "factory" instance of the class.
    CbirPinView(bool) : CbirAlgorithm(false) {}

    /// Does not need to be defined.
    virtual ~CbirPinView() {}

    /// This method creates the true usable instance.
    virtual CbirAlgorithm *Create(DataBase *db, const string& s) const {
      return new CbirPinView(db, s);
    }

    /// This method creates the query-specific data structures.
    virtual CbirAlgorithm::QueryData *CreateQueryData(const CbirAlgorithm *a,
						      const Query *q) const {
      const CbirPinView *pa = static_cast<const CbirPinView*>(a);
      if (!pa)
        return NULL;

      QueryData *qd = new QueryData(a, q);
      qd->kerneltype = pa->kerneltype;
      qd->linrel = pa->linrel;
      qd->mkl    = pa->mkl;
      qd->tensor = pa->tensor;
      qd->feat   = pa->feat;
      qd->ridge  = pa->ridge;

      return qd;
    }
    
    /// Name of the algorithm.  Needs to be defined.
    virtual string BaseName() const { return "pinview"; }

    /// A bit longer name.  Needs to be defined.
    virtual string LongName() const { return "pinview algorithm"; }

    /// Short description  of the algorithm.  Needs to be defined.
    virtual string Description() const {
      return "This is the PinView algorithm.";
    }
    
    /// If we had some specifiers they could be appended to BaseName()...
    virtual string FullName() const {
      // currently the parameters need to be in alphabetical order!
      string fn = BaseName()+"_feat="+feat+"_kerneltype="+kerneltype+"_linrel="+linrel;
      if (mkl=="rr"||mkl=="1c"||mkl=="2c")
        fn += "_mkl="+mkl;
      fn += tensor?"_tensor":"";
      //fn += "_ridge="+ridge; //correct format?
      // ridge is not here
      return fn;
    }

    /** Virtual method that returns a list of parameter specifications.
	\return list of parameter specifications.
    */
    virtual const list<parameter>& Parameters() const;

    /// Initialization with query instance and arguments from
    /// algorithm=pinview(arguments) .  Does not need to be defined.
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
    ground_truth GetGvtImages(const ground_truth& gt, size_t nn) {
      return GetGvtImages(gt.indices(1), nn);
    }

    ///
    ground_truth GetGvtImages(const vector<size_t>&, size_t);

    ///
    ground_truth GetGvtImages(size_t, size_t);

  protected:
    ///
    string linrel;

    ///
    string mkl;
    
    ///
    bool tensor;

    ///
    string feat;

    ///
    float ridge;

    string kerneltype;
    
  }; // class CbirPinView

} // namespace picsom

#endif // _CBIRPINVIEW_H_
