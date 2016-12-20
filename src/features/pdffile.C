//

#include <pdffile.h>

namespace picsom {

  /////////////////////////////////////////////////////////////////////////////

  pdffile::pdffile(const string& name) {
    basename = name; // poista .ext
    // lue sis‰‰n ...
    read(name);
  }
   
  /////////////////////////////////////////////////////////////////////////////

  pdffile::~pdffile() {
  }
   
  /////////////////////////////////////////////////////////////////////////////

  bool pdffile::read(const string& name) {
    return true;
  }
   
  /////////////////////////////////////////////////////////////////////////////

  list<pair<string,string> > pdffile::objects() const {
    list<pair<string,string> > ret;
    ret.push_back(make_pair(basename,         "file"));
    ret.push_back(make_pair(basename+":text", "text"));

    /// oikeastaan pit‰isi kutsua hierarchy()-metodia...

    return ret;
  }
     
  /////////////////////////////////////////////////////////////////////////////

  pdffile::object_hierarchy pdffile::hierarchy() const {
    object_hierarchy ret;
    ret.name = basename;
    ret.type = "file";

    object_hierarchy sub;
    sub.name = basename+":text";
    sub.type = "text";
    ret.subobjects.push_back(sub);

    return ret;
  }
   
  /////////////////////////////////////////////////////////////////////////////

  string pdffile::extract(const string&) const {
    return "content of the requested subobject";
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom
