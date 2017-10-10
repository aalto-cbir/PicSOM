// -*- C++ -*-    $Id: textdata.h,v 1.18 2017/04/28 07:48:19 jormal Exp $

/**
   \file textdata.h
   \brief Declarations and definitions of class picsom::textdata.
  
   textdata.h contains declarations and definitions of
   class picsom::textdata, which is a format and library
   independent storage for text data. The text data is stored
   in a vector of wchar_t elements.
   
   \author Hannes Muurinen <hannes.muurinen@hut.fi>
   $Revision: 1.18 $
   $Date: 2017/04/28 07:48:19 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _textdata_h_
#define _textdata_h_

//#include <missing-c-utils.h>
#include <picsom-config.h>

#include <sstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#define WORD_SEPARATORS " \t\n\r"
#define LINE_SEPARATORS "\n\r"

#ifdef __MINGW32__
#define LWORD_SEPARATORS WORD_SEPARATORS
#define LLINE_SEPARATORS LINE_SEPARATORS
#else
#define LWORD_SEPARATORS L" \t\n\r"
#define LLINE_SEPARATORS L"\n\r"
#endif // __MINGW32__
/**
  \brief namespace picsom wraps (eventually) all classes.
  
  textdata is defined within namespace picsom.  All other classes
  of the project either are there already or will be moved in there. 
*/

namespace picsom {
  /// namespace std is part of the C++ Standard.
  using namespace std;

#ifdef __MINGW32__
  typedef string wstring;
#endif // __MINGW32__

  typedef wchar_t textdata_char_t;
  typedef wstring textdata_string_t;
  typedef vector<textdata_char_t> textdata_char_vect_t;
  typedef vector< vector<string> > textlines_t;

  class textdata {
  public:
    enum unit {
      unit_char,
      unit_word,
      unit_line
    };

    /// Version control identifier of the imagedata.h file.
    static const string& version() {
      static const string v =
	"$Id: textdata.h,v 1.18 2017/04/28 07:48:19 jormal Exp $";
      return v;
    }

    /// The constructor initializes the vector used to store the data
    textdata(int l = 0, bool only_orig = false)
      throw(string) {
	if (l <0)
	  throw "textdata : illegal argument";
	_vec_wchar = textdata_char_vect_t(l, (textdata_char_t)0);
	_only_orig = only_orig;
      }
    
    virtual ~textdata(){};

    /// Does the object have any data?
    bool has_data() const throw() { return length()>0; }

    /// Number of characters stored in the object.
    unsigned int length() const { return _only_orig ?
				    _original.size() : _vec_wchar.size(); }

    /// Is the given index inside the data vector boundaries?
    bool index_ok(int i) const {
      return i>=0 && (unsigned)i<length();
    }

    /** Returns an error message string used for throwing exceptions
	\param f the name of the function
	\param m the message
    */
    static string ensure_msg(const string& f, const string& m) {
      return f+" : "+m;
    }

    /** Throws an exception
	\param f the name of the function
	\param m the message
    */
    static void ensure_do_throw(const string& f, const string& m) {
      throw ensure_msg(f, m);
    }

    /// Throws an error message unless the index is ok.
    void ensure_index(const char *f, int i) const {
      if(!index_ok(i)) {
	stringstream msg;
	msg << "index " << i << " not ok in text with length " << length();
	ensure_do_throw(f, msg.str());
      }
    }

    /// get-function for single characters:
    textdata_char_t get(int i) const {
      ensure_index("get_wchar()", i);
      return _only_orig ? (textdata_char_t) _original.at(i) : _vec_wchar.at(i);
    }

    /// get-function for entire data vector:
    textdata_char_vect_t get() const {
      return _vec_wchar;
    }

    /// converts the data to string
    string as_string() {
      if (_only_orig)
	return _original;

      string ret;
      for(textdata_char_vect_t::iterator i = _vec_wchar.begin();
	  i != _vec_wchar.end(); i++)
	ret += (unsigned char) *i;
      return ret;
    }

    /// converts the data to wstring
    textdata_string_t as_wstring() {
      textdata_string_t ret;
      for(textdata_char_vect_t::iterator i = _vec_wchar.begin();
	  i != _vec_wchar.end(); i++)
	ret += (textdata_char_t) *i;
      return ret;
    }

    /// Splits a given string into parts by separating at given characters
    static vector<string> split_string(const string& s, 
				       const string& split_chars) 
    {
      vector<string> ret;
      string::size_type start_idx = 0;

      string::size_type pos = s.find_first_of(split_chars, start_idx);

      while (pos != string::npos) {
	string sub = s.substr(start_idx, pos-start_idx);
	
	if (sub.length() && (sub.find_first_of(split_chars) != 0))
	  ret.push_back(sub);
	start_idx = pos+1;
	pos = s.find_first_of(split_chars, start_idx);
      }
      
      // add the last word if there is a word in the end of the text that 
      // doesn't end in a space/newline/tab
      if (start_idx < s.length()) {
	string sub = s.substr(start_idx);
	if (sub.length() && (sub.find_first_of(split_chars) != 0))
	  ret.push_back(sub);
      }

      return ret;    
    }

    /// Splits a given string into parts by separating at given characters
    static vector<wstring> split_wstring(const wstring& s, 
					 const wstring& split_chars) {
      vector<wstring> ret;
      wstring::size_type start_idx = 0;

      wstring::size_type pos = s.find_first_of(split_chars, start_idx);

      while (pos != wstring::npos) {
	wstring sub = s.substr(start_idx, pos-start_idx);
	if(sub.length() && (sub.find_first_of(split_chars) != 0))
	  ret.push_back(sub);
	start_idx = pos+1;
	pos = s.find_first_of(split_chars, start_idx);
      }
      
      // add the last word if there is a word in the end of the text that 
      // doesn't end in a space/newline/tab
      if (start_idx < s.length()) {
	wstring sub = s.substr(start_idx);
	if (sub.length() && (sub.find_first_of(split_chars) != 0))
	  ret.push_back(sub);
      }

      return ret;
    }

    /// returns the separate words of the text
    vector<string> get_words_as_string() {
      return split_string(as_string(), WORD_SEPARATORS);
    }

    /// returns the separate words of the text
    vector<wstring> get_words_as_wstring() {
      return split_wstring(as_wstring(), LWORD_SEPARATORS);
    }

    vector<string> get_lines_as_string() {
      return split_string(as_string(), LINE_SEPARATORS);
    }

    vector<wstring> get_lines_as_wstring() {
      return split_wstring(as_wstring(), LLINE_SEPARATORS);
    }

    textlines_t get_lines_as_words() {
      textlines_t ret;
      vector<string> lines = get_lines_as_string();
      for (vector<string>::const_iterator it = lines.begin(); it != lines.end();
	   it++)
	ret.push_back(split_string((string)*it,WORD_SEPARATORS));
      return ret;
    }

    string getWord(int unsigned i) {
      if(words.size() == 0)
	words = get_words_as_string();
      return words.at(i);
    }

    size_t wordCount() {
      if(words.size() == 0)
	words = get_words_as_string();
      return words.size();
    }

    string getLine(int unsigned i) {
      if(lines.size() == 0)
	lines = get_lines_as_string();
      return lines.at(i);
    }

    size_t lineCount() {
      if(lines.size() == 0)
	lines = get_lines_as_string();
      return lines.size();
    }


//     /// returns the separate words of the text
//     vector<string> get_words_as_string() {
//       vector<string> ret;
//       string s = as_string();
//       string::size_type start_idx = 0;

//       string::size_type pos = s.find_first_of(" \n\t", start_idx);

//       while(pos != string::npos) {
// 	string sub = s.substr(start_idx, pos-start_idx);
// 	if(sub.length() && sub != " " && sub != "\n" && sub != "\t")
// 	  ret.push_back(sub);
// 	start_idx = pos+1;
// 	pos = s.find_first_of(" \n\t", start_idx);
//       }
      
//       // add the last word if there is a word in the end of the text that 
//       // doesn't end in a space/newline/tab
//       if(start_idx < s.length()) {
// 	string sub = s.substr(start_idx);
// 	if(sub.length() && sub != " " && sub != "\n" && sub != "\t")
// 	  ret.push_back(sub);
//       }

//       return ret;
//     }

//     /// returns the separate words of the text
//     vector<wstring> get_words_as_wstring() {
//       vector<wstring> ret;
//       wstring s = as_wstring();
//       wstring::size_type start_idx = 0;

//       wstring::size_type pos = s.find_first_of(L" \n\t", start_idx);

//       while(pos != wstring::npos) {
// 	wstring sub = s.substr(start_idx, pos-start_idx);
// 	if(sub.length() && sub != L" " && sub != L"\n" && sub != L"\t")
// 	  ret.push_back(sub);
// 	start_idx = pos+1;
// 	pos = s.find_first_of(L" \n\t", start_idx);
//       }
      
//       // add the last word if there is a word in the end of the text that 
//       // doesn't end in a space/newline/tab
//       if(start_idx < s.length()) {
// 	wstring sub = s.substr(start_idx);
// 	if(sub.length() && sub != L" " && sub != L"\n" && sub != L"\t")
// 	  ret.push_back(sub);
//       }

//       return ret;
//     }

    /// Sets the original string value
    void original_string(string o) { _original = o; }

    /// Returns the original string value
    string original_string() { return _original; }

    /// if only original string is stored
    bool only_original() { return _only_orig; }
    
    /// set only original bool
    void only_original(bool o) { _only_orig = o; }

    /// set-function for single characters:
    void set(int i, const textdata_char_t d) { 
      ensure_index("set(wchar)", i);
      _vec_wchar[i] = d;
      if(words.size() > 0)
	words.clear();
      if(lines.size() > 0)
	lines.clear();
    }

    /// set-function for vectors:
    void set(const textdata_char_vect_t& d) {
      // ensure_something?
      _vec_wchar = d;
      if(words.size() > 0)
	words.clear();
      if(lines.size() > 0)
	lines.clear();
    }

    /// push-function adds the given character to the end of the data vector.
    void push(textdata_char_t d) {
      _vec_wchar.push_back(d);
      if(words.size() > 0)
	words.clear();
      if(lines.size() > 0)
	lines.clear();
    }

    /// concatenates the given data to the end of this instance.
    bool append(const textdata& d) {
      _original  += d._original;
      _vec_wchar.insert(_vec_wchar.end(),
			d._vec_wchar.begin(), d._vec_wchar.end());
      words.insert(words.end(), d.words.begin(), d.words.end());
      lines.insert(lines.end(), d.lines.begin(), d.lines.end());
      return true;
    }

  private:

    ///////////////////////////////////////////////////////////////////////

    /// Container for the text data
    textdata_char_vect_t _vec_wchar;
    
    /// The original string as read from the text file (without character 
    /// set conversion)
    string _original;

    /// The text as a vector of strings
    vector<string> words;

    /// The lines of the text as separate strings
    vector<string> lines;

    /// Do we store the text data only as in its original string form, or
    /// do we also make the character set conversion and store it as wchars?
    bool _only_orig;
  };

} // namespace picsom

#endif // !_textdata_h_

