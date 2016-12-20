// -*- C++ -*- $Id: pdffile.h,v 1.1 2006/09/19 10:29:34 jorma Exp $	

#ifndef _PDFFILE_H
#define _PDFFILE_H

#include <list>
#include <string>
using namespace std;

namespace picsom {
  ///
  class pdffile {
  public:

    ///
    class object_hierarchy {
    public:
      
      ///
      string name;

      ///
      string type;

      ///
      list<object_hierarchy> subobjects;

    }; // class pdffile::object_hierarchy

    ///
    pdffile(const string&);

    ///
    ~pdffile();

    ///
    bool read(const string&);

    ///
    list<pair<string,string> > objects() const;  

    ///
    object_hierarchy hierarchy() const;  

    ///
    string extract(const string&) const;

  protected:
    ///
    string basename;

  }; // class pdffile

} // namespace picsom

#endif // _PDFFILE_H

