// -*- C++ -*-  $Id: ValuedList.C,v 2.3 2011/03/31 09:36:32 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _PICSOM_VALUEDLIST_C_
#define _PICSOM_VALUEDLIST_C_

#include <ValuedList.h>

namespace picsom {
  ///
  static const string ValuedList_C_vcid =
    "@(#)$Id: ValuedList.C,v 2.3 2011/03/31 09:36:32 jorma Exp $";

  //===========================================================================

  template <class O> 
  void ValuedList<O>::move(size_t a, size_t b) {
    typename container::iterator ai = vec.begin()+a;
    typename container::iterator bi = vec.begin()+b;

    O *tmp = *ai;

    while (a<b) {
      // typename vector<O>::iterator i = ai+1;
      typename container::iterator i = ai;
      i++;
      *ai = *i;
      ai = i;
      a++;
    }
    while (a>b) {
      // typename vector<O>::iterator i = ai-1;
      typename container::iterator i = ai;
      i--;
      *ai = *i;
      ai = i;
      a--;
    }
    *bi = tmp;
  }

  //===========================================================================

  template <class O>
  bool ValuedList<O>::will_be_added(size_t llin, double v) const { 
    size_t ll = llin;
    if (ll==0)
      ll = numeric_limits<size_t>::max();

    if (size()>=ll && value(vec.rbegin())>=v)
      return false;

    return true;
  }

  //===========================================================================

  template <class O>
  void ValuedList<O>::add_and_sort(size_t llin, const O& o) { 
    // cout << "add_and_sort() adding : " << o.Index() << " " << o.Value() 
    //   << endl;

    size_t i = 0, ll = llin;
    if (ll==0)
      ll = numeric_limits<size_t>::max();

    if (size()>=ll && value(vec.rbegin())>=o.Value())
      return;

    typename container::iterator p = vec.begin();
    for (; i<size() && p!=vec.end(); p++, i++)
      if (o.Value()>value(p))
        break;
    if (i<ll) {
      p = vec_insert(p, o);
      contains(o.Index(), true);
    }
    if (size()>ll) {
      while (i++<ll)
        p++;
      for (typename container::iterator q=p; q!=vec.end(); q++) {
        contains(index(q), false, false);
        // cout << "add_and_sort() deleting : " << (*q)->Index()
        //      << " " << (*q)->Value() << endl;
        delete *q;
      }
      vec.erase(p, vec.end());
    }
  }

  //===========================================================================

  template <class O>
  bool ValuedList<O>::check(bool do_abort) const {
    string msg = "ValuedList::check() : ";
    vector<int> tmp = included;
    for (typename container::const_iterator i=vec.begin();
         i!=vec.end(); i++) {
      int intidx = index(i);
      if (intidx<0) {
        cout << msg << "idx<0" << endl;
        goto ret_false;
      }
      size_t idx = (size_t)intidx;
      if (included.size()<=idx) {
        cout << msg << "included.size()=" << included.size() << " idx="
             << idx << endl;
        goto ret_false;
      }
      if (!included[idx]) {
        cout << msg << "included[" << idx << "]=" << included[idx] << endl;
        goto ret_false;
      }
      if (!tmp[idx]) {
        cout << msg << "tmp[" << idx << "]=" << tmp[idx] << endl;
        goto ret_false;
      }
      tmp[idx] = 0;
    }
    for (size_t idx=0; idx<tmp.size(); idx++)
      if (tmp[idx]) {
        cout << msg << "tmp[" << idx << "]=" << tmp[idx] << endl;
        goto ret_false;
      }

    return true;

  ret_false:
    dump();
    if (do_abort)
      abort();
    return false;
  }

  //===========================================================================

  template <class O>
  void ValuedList<O>::dump() const {
    cout << "ValuedList() : " << endl;
    for (typename container::const_iterator i=vec.begin();
         i!=vec.end(); i++) {
      int intidx = index(i);
      cout << intidx << " =";
      size_t idx = (size_t)intidx;
      if (included.size()<=idx)
        cout << "?";
      else
        cout << included[idx];
      cout << endl;
    }
    for (size_t idx=0; idx<included.size(); idx++)
      if (included[idx])
        cout << "included[" << idx << "]=" << included[idx]
             << " " << (locate(idx, false)?"yes":"NO") << endl;
  }

} // namespace picsom

#endif // _PICSOM_VALUEDLIST_C_

//=============================================================================
//=============================================================================

// Local Variables:
// mode: lazy-lock
// End:

