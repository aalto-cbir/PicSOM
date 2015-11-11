// -*- C++ -*-  $Id: ontology.C,v 2.1 2015/03/17 11:22:36 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <ontology.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string ontology_C_vcid =
    "@(#)$Id: ontology.C,v 2.1 2015/03/17 11:22:36 jorma Exp $";


  /////////////////////////////////////////////////////////////////////////////

  vector<pair<string,float> > ontology::pick(const relation_t& rel,
					     const string& s,
					     bool rec, bool d)  const {
    string msg = "ontology::pick() : ";

    vector<pair<string,float> > ret;
    for (auto i=rel.begin(); i!=rel.end(); i++)
      if ((d && i->first==s) || (!d && i->second.first==s))
	ret.push_back(make_pair(d?i->second.first:i->first,
				i->second.second));

    if (!rec)
      return ret;

    vector<pair<string,float> > retrec;
    for (auto i=ret.begin(); i!=ret.end(); i++) {
      retrec.push_back(*i);
      auto l = pick(rel, i->first, true, d);
      retrec.insert(retrec.end(), l.begin(), l.end());
    }
    
    return retrec;
  }

  /////////////////////////////////////////////////////////////////////////////

  void ontology::add(relation_t& r, const string& a, const string& b,
		     float v)  {
    string msg = "ontology::add() : ";
    
    r.push_back(make_pair(a, make_pair(b, v)));
  }

  /////////////////////////////////////////////////////////////////////////////

  // bool ontology::()  {
  //   string msg = "ontology::() : ";
  // }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
