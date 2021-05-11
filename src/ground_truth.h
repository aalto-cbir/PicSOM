// -*- C++ -*-  $Id: ground_truth.h,v 2.33 2021/05/11 14:46:57 jormal Exp $
// 
// Copyright 1998-2021 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_GROUND_TRUTH_H_
#define _PICSOM_GROUND_TRUTH_H_

#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <stdexcept>

namespace picsom {
  using namespace std;

  static const string ground_truth_h_vcid =
    "@(#)$Id: ground_truth.h,v 2.33 2021/05/11 14:46:57 jormal Exp $";

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  /** Structure for storing ground truth classes.
  */

  class ground_truth {
  public:
    ///
    typedef signed char value_type;

    ///
    explicit ground_truth(const string& l = "", size_t d = 0) :
      vec(d), implicit(d), lab(l), exp(true) {}

    ///
    explicit ground_truth(size_t d, value_type v = 0) : vec(d, v), implicit(d), 
							exp(true) {}

    ///
    ground_truth(const ground_truth&) = default;
    
    ///
    size_t size() const { return vec.size(); }

    ///
    bool index_ok(size_t i) { return i<size(); }

    ///
    void clear() { vec.clear(); implicit.clear(); lab.clear(); txt.clear(); }

    ///
    bool empty() const { return size()==0; }

    ///
    void resize(size_t l, value_type v) { vec.resize(l, v); implicit.resize(l,0); }

    ///
    value_type get(size_t i) const { return vec[i]; }

    ///
    const value_type& operator[](size_t i) const { return vec[i]; }

    ///
    value_type& operator[](size_t i) { return vec[i]; }

    ///
    ground_truth& operator=(const ground_truth& g) {
      vec   = g.vec;
      implicit = g.implicit;
      lab   = g.lab;
      txt   = g.txt;
      exp   = g.exp;
      dep   = g.dep;
      return *this;
    }

    ///
    bool operator==(const ground_truth& g) {
      return vec==g.vec && implicit==g.implicit && lab==g.lab && txt==g.txt && 
	exp==g.exp && dep==g.dep;
    }

    /// Compares only the data itself, label and text can differ.
    bool DataEquals(const ground_truth& g) {
      return vec==g.vec && exp==g.exp && dep==g.dep;
    }

    ///
    bool operator!=(const ground_truth& g) {
      return !((*this)==g);
    }

    ///
    ground_truth operator&(const ground_truth& g) const {
      return TernaryAND(g);
    }

    ///
    const string& label() const { return lab; }

    ///
    void label(const string& l) { lab = l; }

    ///
    const string& text() const { return txt; }

    ///
    void text(const string& t) { txt = t; }

    ///
    vector<size_t> indices(value_type v) const {
      vector<size_t> r;
      for (size_t i=0; i<size(); i++)
	if (get(i)==v)
	  r.push_back(i);
      return r;
    }

    ///
    size_t change(int s, int d) {
      size_t n = 0;
      for (size_t i=0; i<size(); i++)
	if (get(i)==s) {
	  (*this)[i] = d;
	  n++;
	}
      return n;
    }

    ///
    bool expandable() const { return exp; }

    ///
    void expandable(bool e) { exp = e; }

    ///
    const set<string>& depends() const { return dep; }

    ///
    void depends(const string& s) { dep.insert(s); }

    ///
    void depends(const ground_truth& g) {
      dep.insert(g.dep.begin(), g.dep.end());
    }

    ///
    bool has_minus() const { return has(-1); }

    ///
    bool has_zero()  const { return has(0); }

    ///
    bool has_plus()  const { return has(1); }

    ///
    bool has(int v) const {
      for (size_t i=0; i<size(); i++)
	if (vec[i]==v)
	  return true;
      return false;
    }

    ///
    size_t number_of_equal(int v) const { 
      size_t n = 0;
      for (size_t i=0; i<size(); i++)
	if (vec[i]==v)
	  n++;
      return n;
    }

    ///
    size_t negatives() const { return number_of_equal(-1); }

    ///
    size_t zeros() const     { return number_of_equal(0); }

    ///
    size_t positives() const { return number_of_equal(1); }

    /// old interface methods from here on...

    ///
    int Length() const { return (int)size(); }

    ///
    bool IndexOK(int i) const { return i>=0 && i<(int)size(); }

    ///
    int Get(int i) const { return (int)get((size_t)i); }

    ///
    ground_truth& Set(int v) { 
      for (size_t i=0; i<size(); i++)
	vec[i] = v;
      return *this;
    }

    ///
    ground_truth& Set(int i, int v) { vec[(size_t)i] = v; return *this; }

    ///
    void One() { Set(1); }

    ///
    size_t NumberOfEqual(int v) const { return number_of_equal(v); }

    ///
    const string& Label() const { return label(); }

    ///
    void Label(const string& l) { label(l); }

    ///
    void TernaryCounts(int& m, int& z, int& p) const {
      m = z = p = 0;
      for (size_t i=0; i<size(); i++)
	if (vec[i]==0)
	  z++;
	else if (vec[i]>0)
	  p++;
	else
	  m++;
    }

    ///
    ground_truth TernaryXOR(const ground_truth& g) const {
      check(g);
      ground_truth a = TernarySetMinus(g), b = g.TernarySetMinus(*this);
      ground_truth ret = a.TernaryOR(b);
      ret.lab = lab+"^"+g.lab;
      if (txt!="" && g.txt!="")
	ret.txt = txt+" XOR "+g.txt;
      return ret;
    }

    ///
    ground_truth TernaryOR(const ground_truth& g) const {
      check(g);
      ground_truth ret(*this);
      for (size_t i=0; i<size(); i++)
	if (vec[i]>0 || g.vec[i]>0)
	  ret.vec[i] = vec[i]>g.vec[i] ? vec[i] : g.vec[i];
	else if (vec[i]<0 || g.vec[i]<0)
	  ret.vec[i] = vec[i]<g.vec[i] ? vec[i] : g.vec[i];
      ret.lab = lab+"|"+g.lab;
      if (txt!="" && g.txt!="")
	ret.txt = txt+" OR "+g.txt;
      ret.depends(g);
      return ret;
    }

    ///
    ground_truth TernaryORmax(const ground_truth& g) const {
      check(g);
      ground_truth ret(*this);
      for (size_t i=0; i<size(); i++)
	ret.vec[i] = vec[i]>g.vec[i] ? vec[i] : g.vec[i];
      ret.lab = lab+"|"+g.lab;
      if (txt!="" && g.txt!="")
	ret.txt = txt+" OR "+g.txt;
      ret.depends(g);
      return ret;
    }

    ///
    ground_truth TernaryAND(const ground_truth& g) const {
      check(g);
      ground_truth ret(*this);
      for (size_t i=0; i<size(); i++)
	ret.vec[i] = vec[i]<g.vec[i] ? vec[i] : g.vec[i];
      ret.lab = lab+"&"+g.lab;
      if (ret.lab.find("*&")==0)
	ret.lab.erase(0, 2);
      if (txt!="" && g.txt!="")
	ret.txt = txt+" AND "+g.txt;
      ret.depends(g);
      return ret;
    }

    ///
    ground_truth TernaryNOT() const { 
      ground_truth ret(*this);
      for (size_t i=0; i<size(); i++)
	ret.vec[i] *= -1;
      ret.lab = "~"+lab;
      if (txt!="")
	ret.txt = "NOT "+txt;
      return ret;
    }

    ///
    ground_truth TernarySetMinus(const ground_truth& g) const {
      check(g);
      ground_truth ret = TernaryAND(g.TernaryNOT());
      ret.lab = lab+"\\"+g.lab;
      if (txt!="" && g.txt!="")
	ret.txt = txt+" MINUS "+g.txt;
      return ret;
    }

    ///
    ground_truth TernaryMask(const ground_truth& g) const {
      check(g);
      ground_truth ret(*this);
      for (size_t i=0; i<size(); i++)
	ret.vec[i] = g.vec[i]>0 ? vec[i] : 0;
      // ret.lab == lab; OBS!
      // if (txt!="" && g.txt!="")
      //   ret.txt = txt+" ??? "+g.txt; OBS!
      ret.depends(g);
      return ret;
    }

    ///
    ground_truth TernaryExclusive() const {
      ground_truth ret(*this);
      for (size_t i=0; i<size(); i++)
	if (ret.vec[i]==0)
	  ret.vec[i] = -1;
      ret.lab = "!"+lab;
      if (txt!="")
	ret.txt = "EXCL "+txt;
      return ret;
    }

    ///
    ground_truth TernaryDefined() const {
      ground_truth ret(*this);
      for (size_t i=0; i<size(); i++)
	if (ret.vec[i]==-1)
	  ret.vec[i] = 1;
      ret.lab = "%"+lab;
      if (txt!="")
	ret.txt = "DEF "+txt;
      return ret;
    }

    ///
    string pack() const {
      string ret(size(), 'x');
      for (size_t i=0; i<size(); i++)
	ret[i] = implicit[i] ? vec[i] ^ 128: vec[i];

      return ret;
    }

    ///
    void unpack(const string& s) {
      vec = vector<value_type>(s.size());
      implicit = vector<bool>(s.size());
      for (size_t i=0; i<size(); i++) {
	bool b1 = (s[i] >> 7) & 1;
	bool b2 = (s[i] >> 6) & 1;
	if (b1 != b2) {
	  vec[i] = s[i] ^ 128;
	  implicit[i] = true;
	} else
	  vec[i] = s[i];
      }
    }

    ///
    bool isImplicit(size_t i) const { return implicit[i]; }

    ///
    void setImplicit(size_t i, bool b=true) { implicit[i] = b; }

    ///
    size_t setImplicitEquals(int s) {
      size_t n = 0;
      for (size_t i=0; i<size(); i++)
	if (get(i)==s) {
	  setImplicit(i);
	  n++;
	}
      return n;
    }

    ///
    bool hasImplicit() const {
      for (size_t i=0; i<size(); i++)
	if (implicit[i])
	  return true;
      return false;
    }

    ///
    size_t countImplicit() const {
      size_t res = 0;
      for (size_t i=0; i<size(); i++)
	if (implicit[i])
	  res++;
      return res;
    }

  protected:
    ///
    void check(const ground_truth& g) const /*throw(logic_error)*/ {
      if (g.size()!=size()) {
	stringstream ss;
	ss << "sizes of ground_truths differ: " << size() << "!=" << g.size();
	throw logic_error(ss.str());
      }
    }

  private:
    ///
    vector<value_type> vec;

    ///
    vector<bool> implicit;

    ///
    string lab;

    ///
    string txt;

    //
    bool exp;

    //
    set<string> dep;

  };  // class ground_truth

  ///==========================================================================
  ///==========================================================================

  ///
  class ground_truth_set {
  public:
    ///
    ground_truth_set() {}

    ///
    void erase(size_t i) { vecset.erase(vecset.begin()+i); }

    ///
    size_t size() const { return vecset.size(); }

    ///
    bool index_ok(size_t i) { return i<size(); }

    ///
    size_t ground_truth_size() const { return size() ? vecset[0].size() : 0; }

    ///
    ground_truth& operator[](size_t i) { return vecset[i]; }

    ///
    const ground_truth& operator[](size_t i) const { return vecset[i]; }

    ///
    void insert(const ground_truth& g) { vecset.push_back(g); }

    ///
    size_t find(const string& l) const {
      size_t j = 0;
      for (vector<ground_truth>::const_iterator i=vecset.begin();
	   i!=vecset.end(); i++, j++)
	if (i->label()==l)
	  return j;
      return (size_t)-1;
    }

    ///
    ground_truth *find_ptr(const string& l) {
      size_t i = find(l);
      return index_ok(i) ? &*(vecset.begin()+i) : NULL;
    }

    ///
    const ground_truth *find_ptr(const string& l) const {
      return ((ground_truth_set*)this)->find_ptr(l);
    }

    ///
    void resize(size_t l, ground_truth::value_type v) { 
      for (vector<ground_truth>::iterator i=vecset.begin();
	   i!=vecset.end(); i++)
        i->resize(l, v);
    }

  private:
    //
    vector<ground_truth> vecset;

  };  // class ground_truth_set

  // typedef vector<ground_truth> ground_truth_set;

} // namespace picsom

#endif // _PICSOM_GROUND_TRUTH_H_

// Local Variables:
// mode: lazy-lock
// End:

