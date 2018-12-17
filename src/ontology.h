// -*- C++ -*-  $Id: ontology.h,v 2.3 2018/02/14 09:18:14 jormal Exp $
// 
// Copyright 1998-2018 PicSOM Development Group <picsom@ics.aalto.fi>
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
    "@(#)$Id: ontology.h,v 2.3 2018/02/14 09:18:14 jormal Exp $";

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /**
     ontology
  */
  class ontology {
  public:
    ///
    class entry_t {
    public:
      ///
      entry_t() : own(0), children(0) {}

      ///
      entry_t(const string& a, const string& b, float v1, float v2) :
	first(a), second(b), own(v1), children(v2), level(0) {}
      
      ///
      string first, second;

      ///
      float own, children;

      ///
      size_t level;
    };

    typedef list<entry_t> relation_t;

    ///
    typedef vector<entry_t> rellist_t;

    /// The constructor.
    /// ontology();

    ///
    entry_t&  add_is_a(const string& a, const string& b) {
      return add(_is_a, a, b);
    }

    ///
    entry_t& add_is_part(const string& a, const string& b) {
      return add(_is_part, a, b);
    }

    ///
    entry_t& add_excludes(const string& a, const string& b) {
      return add(_excludes, a, b);
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
    entry_t& add(relation_t&, const string&, const string&, 
		 float = 0, float = 0);

    ///
    rellist_t pick(const relation_t&, const string&, bool, bool) const;

    ///
    static bool wordnet_split(const string&, string&, int&, int&);
    
    ///
    static vector<string> wordnet_path(const string&);
    
    ///
    static vector<string> wordnet_children(const string&, bool = false);
    
    ///
    relation_t& IsA() { return _is_a; }

    ///
    relation_t& IsPart() { return _is_part; }

    ///
    relation_t& Excludes() { return _excludes; }

  private:
    ///
    relation_t _is_a, _is_part, _excludes;

  };  // class ontology

} // namespace picsom

#endif // _PICSOM_ONTOLOGY_H_

