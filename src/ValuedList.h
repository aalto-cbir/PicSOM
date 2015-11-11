// -*- C++ -*-  $Id: ValuedList.h,v 2.4 2011/03/31 09:36:32 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _PICSOM_VALUEDLIST_H_
#define _PICSOM_VALUEDLIST_H_

#include <Valued.h>

#include <string>
#include <vector>
#include <map>
using namespace std;

static const string ValuedList_h_vcid =
  "@(#)$Id: ValuedList.h,v 2.4 2011/03/31 09:36:32 jorma Exp $";

namespace picsom {
  /**
     DOCUMENTATION MISSING
  */

  template <class O> class ValuedList {
    ///
    typedef vector<O*> container;

  public:
    ///
    ValuedList() {}

    ///
    ValuedList(const ValuedList& l) : vec(l.vec), included(l.included) {
      for (typename container::iterator i=vec.begin(); i!=vec.end(); i++)
	*i = new O(**i);
    }

    ///
    ~ValuedList() {
      clear();
    }

    ///
    ValuedList& operator=(const ValuedList& l) {
      clear();
      included = l.included;
      vec = l.vec;
      for (typename container::iterator i=vec.begin(); i!=vec.end(); i++)
	*i = new O(**i);

      return *this;
    }

    ///
    bool empty() const { return vec.empty(); }

    ///
    size_t size() const { return vec.size(); }

    ///
    O& operator[](size_t i) {
      return *vec[i];
    }

    ///
    const O& operator[](size_t i) const {
      return (*(ValuedList<O>*)this)[i];
    }

    ///
    typename ValuedList<O>::container::iterator begin() { return vec.begin(); }

    ///
    typename ValuedList<O>::container::iterator end() { return vec.end(); }

    ///
    typename ValuedList<O>::container::const_iterator begin() const {
      return vec.begin();
    }

    ///
    typename ValuedList<O>::container::const_iterator end() const {
      return vec.end();
    }

    ///
    typename ValuedList<O>::container::reverse_iterator rbegin() {
      return vec.rbegin();
    }

    ///
    typename ValuedList<O>::container::reverse_iterator rend() {
      return vec.rend();
    }

    ///
    typename ValuedList<O>::container::const_reverse_iterator rbegin() const {
      return vec.rbegin();
    }

    ///
    typename ValuedList<O>::container::const_reverse_iterator rend() const {
      return vec.rend();
    }

    ///
    void push_back(const O& o) { 
      vec.push_back(new O(o));
      contains(o.Index(), true);
    }

    ///
    void clear() {
      for (typename container::iterator i=vec.begin(); i!=vec.end(); i++)
	delete *i;
      vec.clear();
      included = vector<int>(included.size(), 0);
    }

    ///
    void erase(size_t i) { 
      typename container::iterator p = vec.begin()+i;

      size_t idx = (size_t)index(p);
      delete *p;
      vec.erase(p);
      contains(idx, false);
    }

    ///
    typename ValuedList<O>::container::iterator find(size_t idx) {
      if (!contains(idx))
	return vec.end();

      return find_inner(idx);
    }

    ///
    typename ValuedList<O>::container::const_iterator find(size_t idx) const {
      return ((ValuedList<O>*)this)->find(idx);
    }

    ///
    typename ValuedList<O>::container::iterator
    find_inner(size_t idx) {
      typename container::iterator p = vec.begin();
      for (; p!=vec.end(); p++)
	if (index(p)==(int)idx)
	  break;

      return p;
    }

    ///
    O *locate(size_t idx, bool fast = true) {
      if (fast && !contains(idx))
	return NULL;

      typename container::const_iterator p = find_inner(idx);

      return p!=vec.end() ? *p : NULL;
    }

    ///
    const O *locate(size_t idx, bool fast = true) const {
      return ((ValuedList<O>*)this)->locate(idx, fast);
    }

    ///
    template <class ITER> double value(ITER p) const {
      return (*p)->Value();
    }

    ///
    template <class ITER> int index(ITER p) const {
      return (*p)->Index();
    }

    ///
    bool contains(size_t idx) const {
      return idx<included.size() && included[idx];
    }

    ///
    void contains(size_t idx, bool v, bool may_check = true) {
      bool do_check = false;
      if (idx>=included.size())
	lengthen(2*idx+1024);
      included[idx] = v;
      if (may_check && do_check)
	check();
    }

    ///
    void move(size_t a, size_t b);

    ///
    bool will_be_added(size_t ll, double v) const;
    
    ///
    void add_and_sort(size_t ll, const O& o);
    
    ///
    bool check(bool do_abort = true) const;

    ///
    void dump() const;

    /// 
    bool insert(size_t p, const O *o) {
      if (p>size())
        return false;

      typename container::iterator i = vec.begin()+p;
      vec_insert(i, o);
      contains(o->Index(), true);

      return true;
    }

    ///
    void append(const ValuedList<O>& l) {
      for (size_t i = 0; i<l.size(); i++)
        push_back(l[i]);
    }

    ///
    bool swap_objects(size_t i, size_t j) {
      if (i>=size() || j>=size())
        return false;

      O tmp = (*this)[i]; 
      (*this)[i] = (*this)[j];
      (*this)[j] = tmp;
      
      return true;
    }

  private:
    ///
    typename container::iterator vec_insert(typename container::iterator i,
					    const O *o) {
      return vec.insert(i, (O*)o);
    }

    ///
    typename container::iterator vec_insert(typename container::iterator i,
					    const O& o) {
      return vec.insert(i, new O(o));
    }

    ///
    void lengthen(size_t l) {
      included.insert(included.end(), l-included.size(), 0);
    }

  protected:
    ///
    container vec;

    ///
    vector<int> included;

  }; // class ValuedList

  typedef vector<VQUnitList> VQUnitListList;
  typedef vector<ObjectList> ObjectListList;

} // namespace picsom

#endif // _PICSOM_VALUEDLIST_H_

// Local Variables:
// mode: lazy-lock
// End:

