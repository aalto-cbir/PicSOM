// -*- C++ -*-  $Id: DummyIndex.h,v 2.8 2011/12/14 13:41:17 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _PICSOM_DUMMYINDEX_H_
#define _PICSOM_DUMMYINDEX_H_

#include <Index.h>

namespace picsom {
  /**
     Dummy class for containing information before index creation
  */
  class DummyIndex : public Index {
  public:
    /**
       The constructor for the DummyIndex class.
       @param db     the database where features have been calculated
       @param name   canonical name of feature set
       @param path   full path to directory containing the files
    */
    DummyIndex(DataBase *db, const string& name, const string& feat,
	       const string& path,
               const string& params, const Index* src);

    ///
    DummyIndex(bool) : Index(true) {}

    ///
    DummyIndex() { abort(); }

    ///
    DummyIndex(const Index&) { abort(); }

    ///
    virtual ~DummyIndex();

    ///
    virtual const string& MethodName() const {
      static string dummyindex = "dummyindex";
      return dummyindex;
    }

    ///
    virtual Index* Make(DataBase *db, const string& name, const string& feat,
			const string& path, const string& params,
			const Index *src) const {
      return new DummyIndex(db, name, feat, path, params, src);
    }

    ///
    virtual bool IsMakeable(DataBase*, const string&, const string&, 
			    bool) const { return false; }

    ///
    virtual bool IsDummy() const { return true; }

    ///
    virtual State *CreateInstance(const string&n ) {
      return new State(this, n);
      // SimpleAbort();
      // return NULL;
    }

    /// Returns true if this feature can appear in the user interface.
    virtual bool IsSelectable() { return false; }

    /// Reads data file if force or not already read.
    virtual bool ReadDataFile(bool, bool, bool) {
      SimpleAbort();
      return false;
    }
    
    ///
    virtual size_t NumberOfScoreValues() const { return 0; } 

    ///
    class State : public Index::State {
    public:
      ///
      State(Index *t, const string& n = "") : Index::State(t, n) {}

      ///
      virtual State *MakeCopy(bool) const { return new State(*this); }

      ///
      virtual float ScoreValue(int) const { return 0; }

    }; // class DummyIndex::State

 };  // class DummyIndex

} // namespace picsom

#endif // _PICSOM_DUMMYINDEX_H_

