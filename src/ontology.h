// -*- C++ -*-  $Id: ontology.h,v 2.1 2015/03/17 11:22:36 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_ONTOLOGY_H_
#define _PICSOM_ONTOLOGY_H_

#include <vector>
#include <list>
#include <string>

using namespace std;

namespace picsom {
  static string ontology_h_vcid =
    "@(#)$Id: ontology.h,v 2.1 2015/03/17 11:22:36 jorma Exp $";

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /**
     ontology
  */
  class ontology {
  public:
    ///
    typedef list<pair<string,pair<string,float> > > relation_t;

    ///
    typedef vector<pair<string,float> > rellist_t;

    /// The constructor.
    /// ontology();

    ///
    void add_is_a(const string& a, const string& b, float v = 1) {
      add(_is_a, a, b, v);
    }

    ///
    void add_is_part(const string& a, const string& b, float v = 1) {
      add(_is_part, a, b, v);
    }

    ///
    void add_excludes(const string& a, const string& b, float v = 1) {
      add(_excludes, a, b, v);
    }

    ///
    rellist_t is_a(const string& s, bool r = true) const {
      return pick(_is_a, s, r, true);
    }

    ///
    rellist_t is_part(const string& s, bool r = true) const {
      return pick(_is_part, s, r, true);
    }

    ///
    rellist_t excludes(const string& s, bool r = true) const {
      return pick(_excludes, s, r, true);
    }

    ///
    rellist_t reverse_is_a(const string& s, bool r = true) const {
      return pick(_is_a, s, r, false);
    }

    ///
    rellist_t reverse_is_part(const string& s, bool r = true) const {
      return pick(_is_part, s, r, false);
    }

    ///
    rellist_t reverse_excludes(const string& s, bool r = true) const {
      return pick(_excludes, s, r, false);
    }

    ///
    void add(relation_t&, const string&, const string&, float);

    ///
    rellist_t pick(const relation_t&, const string&, bool, bool) const;

  private:
    ///
    relation_t _is_a, _is_part, _excludes;

  };  // class DataBase

} // namespace picsom

#endif // _PICSOM_ONTOLOGY_H_

