// -*- C++ -*-  $Id: object_info.h,v 2.4 2008/10/08 09:42:32 jorma Exp $
// 
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _PICSOM_OBJECT_INFO_H_
#define _PICSOM_OBJECT_INFO_H_

#include <PicSOM.h>

#include <string>
using namespace std;

static const string object_info_h_vcid =
  "@(#)$Id: object_info.h,v 2.4 2008/10/08 09:42:32 jorma Exp $";

namespace picsom {
  class DataBase;

  /**
     Information per object in a database.
  */
  class object_info {
  public:
    /// 
    object_info(DataBase *b, int i, const string& l, target_type t) :
      db(b), index(i), label(l), type(t), master(-1) {}

    ///
    const string& db_name() const; // defined in DataBase.C !!!

    ///
    void dump_nonl(ostream& os = cout) const {
      os << db_name() << " #" << index << " <" << label << "> ["
	 << TargetTypeString(type) << "] M: " << master << " {";

      for (size_t i=0; i<duplicates.size(); i++)
	os << (i?",":"") << duplicates[i];

      os << "} P: ";
      for (size_t i=0; i<parents.size(); i++)
	os << (i?",":"") << parents[i];

      os << " {";

      for (size_t i=0; i<children.size(); i++)
	os << (i?",":"") << children[i];

      os << "}";
    }

    ///
    void dump(ostream& os = cout) const {
      dump_nonl(os);
      os << endl;
    }

    ///
    static const object_info& dummy() {
      static const object_info d(NULL, -1, "", target_no_target);
      return d;
    }

    int default_parent() const { 
      return (parents.size() ? parents[0] : -1);
    }

    /// 
    DataBase *db;
    
    ///
    int index;

    ///
    string label;

    ///
    target_type type;    

    ///
    vector<int> parents;

    ///
    vector<int> children;

    ///
    int master;

    ///
    vector<int> duplicates;    

  };  // class object_info

} // namespace picsom

#endif // _PICSOM_OBJECT_INFO_H_

// Local Variables:
// mode: lazy-lock
// compile-command: "ssh itl-cl10 cd picsom/c++\\; make debug"
// End:

