// -*- C++ -*-    $Id: textfile.h,v 1.14 2008/10/31 09:44:28 jorma Exp $

/*!\file textfile.h

\brief Declarations and definitions of class picsom::textfile.

textfile.h contains declarations and definitions of
class picsom::textfile which is used as bridge between
class picsom::textdata and some third-party implementation
of physical text format libraries. The class can be used to
read the contents of a text file to a textdata object.

\author Hannes Muurinen <hannes.muurinen@hut.fi>
$Revision: 1.14 $
$Date: 2008/10/31 09:44:28 $
\bug May be some out there hiding.
\warning Be warned against all odds!
\todo So many things, so little time...
*/

#ifndef _textfile_h_
#define _textfile_h_

#include "textdata.h"

#include <fstream>
#include <iostream>
#include <iomanip>

namespace picsom {
	using namespace std;

  class textfile {
  public:

    enum file_type {
      file_text,
      file_html,
      file_xml
    };

    /// Returns version of textfile class ie. version of textfile.h.
    static const std::string& version() {
      static std::string v =
	"$Id: textfile.h,v 1.14 2008/10/31 09:44:28 jorma Exp $";
      return v;
    }

    /// Default constructor
    textfile() { 
      _filename = "";
      _data = textdata();
    }

    /// Constructor that reads the text file data for later use
    textfile(const std::string& n, bool only_original = false, 
	     file_type ftype = file_text) 
    {
      readfile(n,only_original,ftype);
    }

    /// Destructor.
    ~textfile() {

    }

    //@{
    /// Two methods for accessing the filename.
    const std::string& filename() const { return _filename; }
    std::string& filename()             { return _filename; }
    //@}

    /// Returns true if associated with a filename that contains text.
    bool has_data() const { return !_filename.empty() && _data.length(); }

    void readfile(const std::string& f, bool only_original = false,
		  file_type ftype = file_text);

    /// Prints the data to standard output
    void printdata() {
#ifndef __MINGW32__
      for (size_t i = 0; i < _data.length(); i++) {
	wchar_t c = _data.get(i);
	//cerr << "(" << (char) c << ")";
	wcout << c;
      }
#endif // __MINGW32__
    }

    /// Returns the textdata object
    textdata& Data() { return _data; }

    /// Returns the textdata object
    const textdata& Data() const { return _data; }

    /// Concatenates data from given textfile to end of this one.
    bool append(const textfile& f) {
      _filename += (_filename!="" ? "," : "")+f._filename;
      return _data.append(f._data);
    }

  protected:

// 		void get_text(xmlNodePtr node,xmlChar *rstr);
// 		char *remove_xml(const char *cstr);
    bool remove_html(string& str);

  private:

    //////////////////////////////////////////////////////////////////////////

    /// file name
    std::string _filename;

    /// text data
    textdata _data;

  };
} // namespace

#endif // !_textfile_h_

