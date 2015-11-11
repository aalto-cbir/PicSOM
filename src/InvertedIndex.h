// -*- C++ -*-  $Id: InvertedIndex.h,v 2.5 2011/12/14 13:41:18 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _PICSOM_INVERTEDINDEX_H_
#define _PICSOM_INVERTEDINDEX_H_

#include <Index.h>

namespace picsom {
  /**
     Abstract base class for inverted indices
  */
  class InvertedIndex : public Index {
  protected:
    /**
       The constructor for the InvertedIndex class.
       @param db     the database where features have been calculated
       @param name   canonical name of feature set
       @param path   full path to directory containing the files
    */
    InvertedIndex(DataBase *db, const string& name, const string& feat,
		  const string& path,
		  const string& params, const Index* src);

    ///
    InvertedIndex(bool) : Index(false) {}

    ///
    InvertedIndex() { abort(); }

    ///
    InvertedIndex(const Index&) { abort(); }

    ///
    virtual ~InvertedIndex();
    
  public:
    /// Reads data file if force or not already read.
    /// fixme: should not be here
    virtual bool ReadDataFile(bool force = false, bool nodata = false,
			      bool rwlocked = false) = 0;

    /// Reads metadata file if force or not already read.
    /// fixme: should not be here
    virtual bool ReadMetaDataFile(bool force /*= false*/, 
				  bool nodata /*= false*/,
				  bool rwlocked /*= false*/, 
				  string filename="",
				  size_t maxn=0);

    /// Full-path filename of the data file.
    /// fixme: should not be here
    virtual const string& MetaDataFileName() const { return metfile_str; }

    ///
    class State : public Index::State {
    public:
      State(Index *t, const string& n = "") : Index::State(t, n) {}

    };  // class InvertedIndex::State

  protected:

    /// Full path to meta data file.
    /// fixme: should not be here
    string metfile_str;

    /// If the data is generated meta data, this contains the fields.
    /// fixme: should not be here
    typedef list<pair<string,int> > meta_data_fields_t;

    /// If the data is generated meta data, this contains the fields.
    /// fixme: should not be here
    meta_data_fields_t meta_data_fields;

    /// True if BinaryClassIndices() should work.
    /// fixme: should not be here
    bool binary_classindices;

    /// Data
    FloatVectorSet data;

  };  // class InvertedIndex

} // namespace picsom

#endif // _PICSOM_INVERTEDINDEX_H_

