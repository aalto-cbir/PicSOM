// -*- C++ -*-  $Id: ontology.C,v 2.4 2018/03/23 07:44:29 jormal Exp $
// 
// Copyright 1998-2018 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <ontology.h>
#include <Util.h>

#ifdef HAVE_WN_H
#include <wn.h>
#endif // HAVE_WN_H

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string ontology_C_vcid =
    "@(#)$Id: ontology.C,v 2.4 2018/03/23 07:44:29 jormal Exp $";


  /////////////////////////////////////////////////////////////////////////////

  vector<ontology::entry_t> ontology::pick(const relation_t& rel,
					   const string& s,
					   bool rec, bool d)  const {
    string msg = "ontology::pick() : ";

    vector<entry_t> ret;
    for (auto i=rel.begin(); i!=rel.end(); i++)
      if ((d && i->first==s) || (!d && i->second==s))
	ret.push_back(*i);

    if (!rec)
      return ret;

    vector<entry_t> retrec;
    for (auto i=ret.begin(); i!=ret.end(); i++) {
      retrec.push_back(*i);
      auto l = pick(rel, i->first, true, d);
      retrec.insert(retrec.end(), l.begin(), l.end());
    }
    
    return retrec;
  }

  /////////////////////////////////////////////////////////////////////////////

  ontology::entry_t& ontology::add(relation_t& r, const string& a, 
				   const string& b, float v1, float v2)  {
    string msg = "ontology::add(...,"+a+","+b+") : ";

    for (auto i=r.begin(); i!=r.end(); i++)
      if (i->first==a && (i->second==b || b=="*"))
	return *i;

    if (b=="*")
      cerr << msg << "not found" << endl;

    r.push_back(entry_t(a, b, v1, v2));

    return r.back();
  }

  /////////////////////////////////////////////////////////////////////////////

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

  bool ontology::wordnet_split(const string& s, string& w, int& p, int& e)  {
    string msg = "ontology::wordnet_split("+s+") : ";

#ifdef HAVE_WN_H
    size_t q = s.find('.');
    w = s.substr(0, q);

    p = s[q+1]=='n' ? NOUN : s[q+1]=='v' ? VERB : s[q+1]=='a' ? ADJ : ADV;

    e = atoi(s.substr(q+3).c_str());

    return true;
#else
    return false;
#endif // HAVE_WN_H
  }
#pragma GCC diagnostic pop
  
  /////////////////////////////////////////////////////////////////////////////

  vector<string> ontology::wordnet_path(const string& s)  {
    string msg = "ontology::wordnet_path("+s+") : ";

    vector<string> ret;

#ifdef HAVE_WN_H
    string t;
    int pos = 0, sense = 0;
    wordnet_split(s, t, pos, sense);
    for (;;) {
      string sensestr = ToStr(sense);
      if (sensestr.length()==1)
	sensestr = "0"+sensestr;
      stringstream tx;
      tx << t << "." << (pos==NOUN?'n':pos==VERB?'v':pos==ADJ?'a':'r')
	 << "." << sensestr;
      ret.insert(ret.begin(), tx.str());
      SynsetPtr ss = findtheinfo_ds((char*)t.c_str(),
				    pos, HYPERPTR, sense);
      if (!ss) {
	cerr << "<" << t << "> not found in wordnet" << endl;
	return vector<string>();
      }
      SynsetPtr ssx = NULL;
      if (ss->ptrlist)
	ssx = ss->ptrlist;
      if (!ssx && ss->ptrtyp[0]==INSTANCE)
	ssx = read_synset(ss->ppos[0], ss->ptroff[0], (char*)"");
      if (!ssx)
	break;
      t = ssx->words[0];
      sense = ssx->wnsns[0];
    }
#endif // HAVE_WN_H

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<string> ontology::wordnet_children(const string& s, bool r)  {
    string msg = "ontology::wordnet_children("+s+","+ToStr(r)+") : ";

    vector<string> ret;

#ifdef HAVE_WN_H

#endif // HAVE_WN_H

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  // bool ontology::()  {
  //   string msg = "ontology::() : ";
  // }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
