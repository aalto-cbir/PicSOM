// -*- C++ -*-  $Id: aspects.h,v 2.14 2010/04/27 11:58:02 jorma Exp $
// 
// Copyright 1998-2010 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_ASPECTS_H_
#define _PICSOM_ASPECTS_H_

#include <vector>
#include <map>
#include <string>
#include <iostream>
using namespace std;

namespace picsom {
  static const string aspects_h_vcid =
    "@(#)$Id: aspects.h,v 2.14 2010/04/27 11:58:02 jorma Exp $";

  ///
  class aspect_data {
  public:

    ///
    enum aspect_type { aspect_default, aspect_coordinates };

    ///
    aspect_data() : value(0), type(aspect_default), sticky(false) {}
    
    /// Copies the contents of this object to given object
    void copyTo(aspect_data& as) const {
      as = *this;

      // as.value = value;
      // as.type = type;
      // as.params.assign(/*as.*/params.begin(), /*as.*/params.end());
    }

    ///
    string typeStr() const {
      switch(type) {
      case aspect_coordinates:
	return "coordinates";
	break;
      default:
	return "default";
	break;	  
      }
    }
    
    ///
    void setType(const string& tstr) {
      if (tstr.compare("coordinates") == 0)
	type = aspect_coordinates;
      else
	type = aspect_default;
    }

    ///
    string str() const {
      stringstream ss;
      ss << "value=" << value << " type=" << typeStr();
      if (!params.empty()) {
	ss << " param=";
	for (size_t i=0; i<params.size(); i++)
	  ss << (i?",":"") << params[i];
      }
      if (sticky)
	ss << " sticky";
      return ss.str();
    }
    
    ///
    double value;
    
    ///
    enum aspect_type type;

    ///
    vector<string> params;

    ///
    bool sticky;
    
  }; // class aspect_data

  /// A container containing a map of aspect name -> aspect_data pairs
  class aspects {
  public:

    aspects() {
    }

    /// Copies the contents of this object to given object
    void copyTo(aspects& a) const {
      a.ClearAspects();
      for (map<string,aspect_data>::const_iterator i =
	     aspect_relevance.begin();
	  i != aspect_relevance.end(); i++) {
	i->second.copyTo(a.aspect_relevance[i->first]);
      }
    }

    /** Returnstrue if the named aspect exists. */
    bool HasAspectRelevance(const string& a) const {
      map<string,aspect_data>::const_iterator i = aspect_relevance.find(a);
      return i!=aspect_relevance.end();
    }

    /** Returns the relevance of the given aspect, or 0 if the aspect 
     *  doesn't exist (obs! should this be some other value?) */
    double AspectRelevance(const string& a) const {
      return AspectData(a).value;
    }
    
    /// Returns the aspect_data object of the given aspect
    aspect_data AspectData(const string& a) const {
      map<string,aspect_data>::const_iterator i = aspect_relevance.find(a);
      if (i!=aspect_relevance.end()) 
	return i->second;

      aspect_data tmp;
      return tmp;
    }

    /// Sets the relevance of the given aspect
    bool AspectRelevance(const string& a, double v, const string& t, 
			 const vector<string>& p = vector<string>(),
			 bool s = false) {
      // if (aspect_relevance.find(a)!=aspect_relevance.end())
      //   return false;

      if (aspect_relevance[a].value)
	return false;

      aspect_relevance[a].setType(t);
      aspect_relevance[a].value  = v;
      aspect_relevance[a].params = p;
      aspect_relevance[a].sticky = s;

      return true;
    }

    /// Sets the relevance of the given aspect
    void AspectRelevance(const string& a, double v,
			 aspect_data::aspect_type t = 
			 aspect_data::aspect_default, 
			 const vector<string>& p = vector<string>()) {
      aspect_relevance[a].value = v;
      aspect_relevance[a].type = t;
      aspect_relevance[a].params = p;
    }

    /// Sets the aspect_data of the given aspect
    void AspectRelevance(const string& a, const aspect_data& d) {
      pair<string,aspect_data> p(a,d);
      aspect_relevance.insert(p);
    }

    /// Returns the relevance of all the aspects
    map<string,aspect_data>& Aspects() {
      return aspect_relevance;
    }

    /// Returns the relevance of all the aspects
    const map<string,aspect_data>& Aspects() const {
      return aspect_relevance;
    }

    /// Sets the relevance of all the aspects.
    void Aspects(const map<string,aspect_data>& a) {
      aspect_relevance = a;
    }

    /// Pushes some aspects to the aspect map
    void PushAspects(const aspects& a) {
      for (map<string,aspect_data>::const_iterator
	     i = a.aspect_relevance.begin();
	   i != a.aspect_relevance.end(); i++) 
	i->second.copyTo(aspect_relevance[i->first]);
    }

    size_t Size() const {
      return aspect_relevance.size();
    }

    /// Sets the relevance of all aspects to zero (checkbox not selected)
    void ClearAspectRelevance() {
      map<string,aspect_data>::iterator i = aspect_relevance.begin();
      while(i!=aspect_relevance.end()) {
	aspect_relevance[i->first].value = 0.0;
	aspect_relevance[i->first].type = aspect_data::aspect_default;
	aspect_relevance[i->first].params.clear();
	i++;
      }
    }

    /// Removes all the aspects from the aspect relevance map
    void ClearAspects() {
      aspect_relevance.clear();
    }

    /// Dumps the contents of the aspect map to stdout
    void Dump() {
      cout << "Dumping aspect map with size " << aspect_relevance.size() 
	   << endl;
      for(map<string,aspect_data>::iterator i = aspect_relevance.begin();
	  i != aspect_relevance.end(); i++) {

	  cout << i->first << ":" << endl;
	cout << " Value: " << i->second.value << endl;
	cout << " Type: " << i->second.typeStr() << endl;
	cout << " Params: " << endl;
	for(vector<string>::iterator j = i->second.params.begin();
	    j != i->second.params.end(); j++) 
	  cout << "  " << *j << endl;

      }
    }

    string str() const {
      ostringstream ss;
      for (map<string,aspect_data>::const_iterator i=aspect_relevance.begin();
	   i!=aspect_relevance.end(); i++)
	ss << (i!=aspect_relevance.begin()? " ":"")
	   << "\"" << i->first << "\"=[" << i->second.str() << "]";

      return ss.str();
    }

  protected:

    /// The relevance information of the aspects of this object
    map<string,aspect_data> aspect_relevance;

  }; // class aspects

} // namespace picsom

#endif // _PICSOM_ASPECTS_H_

