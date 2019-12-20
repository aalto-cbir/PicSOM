// -*- C++ -*-  $Id: object_set.h,v 2.6 2019/09/24 12:32:35 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_OBJECT_SET_H_
#define _PICSOM_OBJECT_SET_H_

#include <object_info.h>

#include <string>
#include <deque>
#include <unordered_map>

namespace picsom {
  using namespace std;

  static const string object_set_h_vcid =
    "@(#)$Id: object_set.h,v 2.6 2019/09/24 12:32:35 jormal Exp $";

  class DataBase;

  /**
     A container of objects.
  */
  class object_set {
  public:
    /// 
    object_info& add(DataBase *b, const string& l, target_type t) 
      throw(logic_error) {
      _set.push_back(object_info(b, size(), l, t));
      object_info& o = _set.back();
      auto const r = _label.insert( { l, &o } );
      if (!r.second)
	throw logic_error("label already exists");
      return o;
    }

    ///
    bool index_ok(size_t i) const { return i<size(); }

    /// 
    size_t size() const { return _set.size(); }

    ///
    void clear() { _set.clear(); _label.clear(); }

    ///
    const string& label(size_t i) const throw(out_of_range) {
      return find(i).label;
    }

    ///
    int index(const string& l) const throw(out_of_range) {
      return find(l).index;
    }

    ///
    int index_gentle(const string& l) const {
      label_map_type::const_iterator p = _label.find(l);
      return p==_label.end() ? -1 : p->second->index;
    }

    ///
    object_info& find(size_t i) throw(out_of_range,logic_error) {
      try {
	object_info& ret = _set.at(i);
	if (ret.index!=(int)i)
	  throw logic_error("index doesn't match index");
	return ret;
      }
      catch (...) { // at() should throw only out_of_ranges, but ...
	throw out_of_range("index out of range");
      }
    }

    ///
    const object_info& find(size_t i) const throw(out_of_range,logic_error) {
      return ((object_set*)this)->find(i);
    }

    ///
    object_info& find(const string& l) throw(out_of_range) {
      label_map_type::const_iterator p = _label.find(l);
      if (p==_label.end())
	throw out_of_range("label not found");
      return *p->second;
    }

    ///
    const object_info& find(const string& l) const throw(out_of_range) {
      label_map_type::const_iterator p = _label.find(l);
      if (p==_label.end())
	throw out_of_range("label not found");
      return *p->second;
    }

    ///
    void dump(ostream& os = cout) const {
      size_t i = 0;
      for (set_type::const_iterator p = _set.begin(); p!=_set.end(); p++) {
	os << i++ << ": ";
	p->dump(os);
      }
    }

  protected:
    /// 
    typedef deque<object_info> set_type;

    /// 
    set_type _set;

    /// 
    // typedef map<string,object_info*> label_map_type;

    /// 
    typedef unordered_map<string,object_info*> label_map_type;

    ///
    label_map_type _label;

  };  // class object_set

} // namespace picsom

#endif // _PICSOM_OBJECT_SET_H_

// Local Variables:
// mode: lazy-lock
// compile-command: "ssh itl-cl10 cd picsom/c++\\; make debug"
// End:

