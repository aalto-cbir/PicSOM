// -*- C++ -*-  $Id: PreCalculatedIndex.h,v 2.4 2011/12/14 13:41:18 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _PICSOM_PRECALCULATEDINDEX_H_
#define _PICSOM_PRECALCULATEDINDEX_H_

#include <Index.h>
#include <ValuedList.h>

namespace picsom {
  /**
     Class for precalculated indices read from disk.
  */
  class PreCalculatedIndex : public Index {
  public:
    /**
       The constructor for the PreCalculatedIndex class.
       @param db     the database where features have been calculated
       @param name   canonical name of feature set
       @param path   full path to directory containing the files
    */
    PreCalculatedIndex(DataBase *db, const string& name, const string& feat,
		       const string& path,
		       const string& params, const Index* src);

    ///
    PreCalculatedIndex(bool) : Index(false) {}

    ///
    PreCalculatedIndex() { abort(); }

    ///
    PreCalculatedIndex(const Index&) { abort(); }

    ///
    virtual ~PreCalculatedIndex();

    class State;
    //    class PreCalculatedData;

    ///
    virtual const string& MethodName() const {
      static string precalculated = "precalculated";
      return precalculated;
    }

    ///
    virtual Index* Make(DataBase *db, const string& name, const string& feat,
			const string& path, const string& params,
			const Index *src) const {
      return new PreCalculatedIndex(db, name, feat, path, params, src);
    }

    ///
    virtual bool IsMakeable(DataBase* /*db*/, const string& fn, 
			    const string& dirstr, bool) const {
      return IsPreCalculatedFile(dirstr+"/"+fn+".pre");
    }
    
    /// Returns true if string names a file containing a precalculated feature.
    static bool IsPreCalculatedFile(const string& fn) {
      if (fn=="")
	return false;
      
      ifstream ifs(fn.c_str());
      string line;
      getline(ifs, line);
      return line.find("# PRECALCULATEDFILE")==0;
    }

    ///
    virtual Index::State *CreateInstance(const string& n) {
      return new State(this,n);
    }

    /// Returns true if this feature can appear in the user interface.
    virtual bool IsSelectable() /*const*/ {
      return true;//GuessVectorLength() || IsBinaryFeature();
    }

    /// Sets debug_precalculated that affects *PreCalculated*().
    static void DebugPreCalculated(bool c) { debug_precalculated = c; }

    /// Returns debug_precalculated that affects *PreCalculated*() also in Query.
    static bool DebugPreCalculated() { return debug_precalculated; }

    ///
    virtual bool ReadDataFile(bool force=false, bool nodata=false,
                              bool wrlocked=false);

    ///
    bool ReadPreCalculatedFile();

    /// 
    map<string,double> precalc;

    //PreCalculatedData& StaticPreCalculatedData()

    ///
    //    PreCalculatedData& StaticPreCalculatedData() {
    //      return precalculated_data;
    //    }
    
  public:
    ///
//     class PreCalculatedData {
//     public:
//       void clear() {
// 	precalculated.clear();
//       }

//     protected:
//       /// Precalculated values for objects.
//       vector<vector<float> > precalculated;

//     }; // class PreCalculatedIndex::PreCalculatedData

    class State : public Index::State {
    public:
      State(Index *t, const string& n = "") : Index::State(t, n) {}
      
      ///
      virtual State *MakeCopy(bool full) const {
	State *s = new State(*this);
 	if (!full)
 	  s->clear();
	return s;
      }

      ///
      bool Initialize() {
	objlist     = vector<ObjectList>(1);
	return true;
      }

      ///
      void clear() {
	objlist.clear();
      }

      ///
      virtual float ScoreValue(int) const { return 0; }

    protected:
      
    };  // class PreCalculatedIndex::State

  protected:

    /// True if precalculated features be traced.
    static bool debug_precalculated;

    /// Full path to precalculated file.
    string prefile_str;

    /// Full path to data file.
    string datfile_str;

    /// Index of data vectors read to precalc.
    int datindex;

    /// Component name of the data vector index to be read.
    string component;

    //    PreCalculatedData precalculated_data;

  };  // class PreCalculatedIndex

} // namespace picsom

#endif // _PICSOM_PRECALCULATEDINDEX_H_

// Local Variables:
// mode: lazy-lock
// End:

