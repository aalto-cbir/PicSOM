// -*- C++ -*-  $Id: Util.h,v 2.107 2021/05/11 14:46:57 jormal Exp $
// 
// Copyright 1998-2021 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_UTIL_H_
#define _PICSOM_UTIL_H_

//#include <missing-c-utils.h>
#include <TimeUtil.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif // HAVE_SYS_RESOURCE_H

#include <sys/stat.h>
#include <sys/types.h>

#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <iconv.h>
#include <regex.h>

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif // HAVE_WINSOCK2_H

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif // HAVE_NETDB_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

#ifdef HAVE_MAGIC_H
#include <magic.h>
#endif // HAVE_MAGIC_H

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif // HAVE_SYS_SYSCALL_H

#ifdef HAVE_JAULA_H
#include <jaula.h>
#endif // HAVE_JAULA_H

#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <set>
#include <vector>
#include <stdexcept>
#include <list>
#include <iostream>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif // HAVE_PWD_H

#ifdef __linux__
#include <execinfo.h>
#include <cxxabi.h>
#endif // __linux__

using namespace std;

#define swap_int(x,y) { int _swap_int_tmp=y;y=x;x=_swap_int_tmp; }

#define DATA_PATH_DEBUG_ENVVAR "DATA_PATH_DEBUG"
#define DATA_PATH_ENVVAR  "DATA_PATH"
#define DATA_PATH_DEFAULT ".:~/data:/share/local/data//:/usr/local/data//"

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

namespace picsom {
  static const string Util_h_vcid =
    "@(#)$Id: Util.h,v 2.107 2021/05/11 14:46:57 jormal Exp $";

  extern bool trap_after_error;
  // extern bool jam_after_error;
  extern bool backtrace_before_trap;

  extern string _picsom_util_temp_dir;

  ///
  inline void set_temp_dir(const string& d) { _picsom_util_temp_dir = d; }

  ///
  string get_temp_file_name(const string& = "");
  
  ///
  inline pid_t gettid() {
#ifdef __linux__
    return syscall(SYS_gettid);
#else
    return getpid();
#endif // __linux__
  }

  ///
  inline void NanoSleep(time_t sec, long nsec) {
    struct timespec ts = { sec, nsec };
    nanosleep(&ts, NULL);
  }

  ///
  inline void NanoSleep(double dsec) {
    int sec = floor(dsec);
    long nsec = (dsec-sec)*1000000000;
    NanoSleep(sec, nsec);
  }

  ///
  inline const string& HomeDir() {
    static string udir;
    if (udir=="") {
      passwd *pw = getpwuid(getuid());
      udir = pw ? pw->pw_dir : "";
    }
    return udir;
  }
  
  /// Process current working directory.
  inline string Cwd() {
    char cwdbuf[2048];
    const char *wd = getcwd(cwdbuf, sizeof cwdbuf);
    return wd ? wd : "";
  }

  /// Reads a file in a string.
  inline string FileToString(const string& fname) {
    string err = "FileToString("+fname+") : ";
    ifstream in(fname.c_str());
    string data;
    for (;;)
      try {
	char tmp[1024*1024];
	in.read(tmp, sizeof tmp);
	streamsize n = in.gcount();
	data.append(tmp, n);
	if (!n || !in)
	  break;
      }
      catch (const ifstream::failure &e) {
	cerr << err+"ifstream "+to_string(data.size())+" failure "+e.what()
	     << endl;
	return "";
      }
      catch (const exception &e) {
	cerr << err+to_string(data.size())+" exception "+e.what()
	     << endl;
	return "";
      }
    return data;
  }

  /// Wrtites a string in a file.
  inline bool StringToFile(const string& s, const string& fname) {
    ofstream out(fname.c_str());
    out << s;
    out.close();
    return out.good();
  }

  /// Copies one file to another.
  inline bool CopyFile(const string& src, const string& dst) {
    if (src=="" || dst=="")
      return false;
    
    ifstream  in(src);
    ofstream out(dst);
    if (!in || !out)
      return false;
    size_t bufsize = 64*1024*1024;
    char *buf = new char[bufsize];
    for (;;) {
      in.read(buf, bufsize);
      ssize_t n = in.gcount();
      if (n<1)
	break;
      out.write(buf, n);
    }
    delete buf;
    return out.good();
  }

  /**
     DOCUMENTATION MISSING
  */

  class RegExpMatch {
  public:
    RegExpMatch() : _start(-1), _length(-1) {}

    RegExpMatch(const string& str, const ssize_t start=-1, 
		const ssize_t length=-1) : 
      _str(str), _start(start), _length(length) {}
    
    const string& str() const { return _str; }
    const char* c_str() const { return _str.c_str(); }
    ssize_t start() const { return _start; }
    ssize_t length() const { return _length; }
    ssize_t end() const { return success() ? _start+_length : -1; }

    bool success() const { return _start!=-1 && _length!=-1; }

  private:
    string _str;
    ssize_t _start;
    ssize_t _length;
  };

  class RegExp {
  public:
    ///
    RegExp(const string& pattern = "", bool sub = true) :
      rep(NULL), errcode(REG_BADPAT) {
      if (pattern!="")
	comp(pattern, sub);
    }

    ~RegExp() { 
      if (rep)
	regfree(rep);
      delete rep;
    }

    void comp(const string& pattern, bool sub=true) {
      if (rep)
	regfree(rep);
      else
	rep = new regex_t;

      errcode = regcomp(rep, pattern.c_str(), 
			REG_EXTENDED + (sub?0:REG_NOSUB));
    }

    bool ok() const { return rep && errcode==0; }

    string error(int code=-1) const;

    bool match(const string& str);

    vector<RegExpMatch> match(const string& str, const int subs);

  protected:
    regex_t *rep;
    int errcode;
  };

  void ProgressText(const char* =NULL,const char* =NULL,
                    const char* =NULL, const char* =NULL,
                    const char* =NULL,const char* =NULL);

  /// Utterly useless but fun progress bar :)
  class ProgressBar {
  public:
    ProgressBar(int max, int w=78, const string& defmsg = "");

    void Update(int p, const string& msg = ""); 

    void Inc(const string& msg = "") { Update(oldp+1, msg); }

    int Max() { return pmax; }
 
  protected:
    bool disabled;
    int width;
    string pstr;
    int cc;
    int pmax;
    int oldp;
    string defmsg;
  };

  /// Make conversion from something to string.
  template <class T> inline string ToStr(const T& v) {
    stringstream ss;
    ss << v;
    return ss.str();
  }
  
  /// Make conversion from container<something> to string
  template <class I> inline string ToStr(I b, I e,
					 const string& separator = " ",
					 bool surround = false) {
    stringstream ss;
    for (auto i=b; i!=e; i++) {
      if (i!=b || surround)
	ss << separator;
      ss << *i;
      if (surround)
	ss << separator << " ";
    }
    return ss.str();
  }

  /// Make conversion from list<something> to string
  template <class T> inline string ToStr(const list<T>& v, 
					 const string& separator = " ",
					 bool surround = false) {
    return ToStr(v.begin(), v.end(), separator, surround);
  }

  /// Make conversion from vector<something> to string
  template <class T> inline string ToStr(const vector<T>& v, 
					 const string& separator = " ",
					 bool surround = false) {
    return ToStr(v.begin(), v.end(), separator, surround);
  }

  /// A string of fixed lenght with initial part given.
  inline string StrFieldL(size_t n, const string& s = "") {
    string r(n, ' ');
    if (s!="") {
      size_t m = n<s.size() ? n : s.size();
      r.replace(0, m, s, 0, m);
    }      
    return r;
  }

  /// A string of fixed lenght with last part given.
  inline string StrFieldR(size_t n, const string& s = "") {
    string r(n, ' ');
    if (s!="") {
      size_t m = n<s.size() ? n : s.size();
      r.replace(r.size()-m, m, s, 0, m);
    }      
    return r;
  }

  /// Returns 1024 for 1k etc.
  inline unsigned long FromSuffixStr(const string& s) {
    if (s=="unlimited")
#ifdef HAVE_SYS_RESOURCE_H
     return RLIM_INFINITY;
#else
    return (unsigned long)-1;
#endif // HAVE_SYS_RESOURCE_H

    long v = atol(s.c_str());
    if (s.size()) {
      if (s[s.size()-1]=='k')
	v *= 1024;
      if (s[s.size()-1]=='M')
	v *= 1024*1024;
      if (s[s.size()-1]=='G')
	v *= 1024L*1024*1024;;
    }
    return v;
  }

  /// Converts int to xxkB / xxMB / xxGB
  inline string HumanReadableBytes(size_t b) {
    for (size_t i=0; i<4; i++) {
      size_t d = 1;
      for (size_t j=i; j<3; j++)
	d *= 1024;
      if (i<3 && b<d)
	continue;

      char buf[100];
      float v = float(b)/d;
      if (v>=100 || i==3)
	sprintf(buf, "%d", (int)floor(v+0.5));
      else if (v>=10)
	sprintf(buf, "%4.1f", v);
      else
	sprintf(buf, "%4.2f", v);
      
      return string(buf)+string(i==0?"G":i==1?"M":i==2?"k":"")+"B";
    }
    return "???";
  }

  ///
  inline string LowerCase(const string& str) {
    string r = str;
    for (size_t i=0; i<r.size(); i++)
      if (r[i]>='A' && r[i]<='Z')
	r[i] += 32;
    return r;
  }

  /// Used eg. for ground truth argument lists.
  string JoinWithString(const vector<string>& a, const string& join,
                        bool force = false);

  /// Used eg. for ground truth argument lists.
  string JoinWithString(const list<string>& a, const string& join,
                        bool force = false);

  /// Used eg. for ground truth argument lists.
  inline string CommaJoin(const vector<string>& a, bool force = false) {
    return JoinWithString(a, ",", force);
  }

  /// Used eg. for ground truth argument lists.
  inline string CommaJoin(const list<string>& a, bool force = false) {
    return JoinWithString(a, ",", force);
  }

  ///
  list<string> BraceCommaExpand(const string&);

  /// Produces a vector of separated words split on one of the characters given.
  vector<string> SplitInSomething(const char*, bool, const string&);

  /// Produces a vector of separated words split on the word given.
  vector<string> SplitOnWord(const string&, const string&);

  /// Produces a vector of separated words.
  inline vector<string> SplitInSpaces(const string& s) {
    // \n\r was added 2014-12-15
    return SplitInSomething(" \t\n\r", true, s);
  }

  /// Produces a vector of separated words.
  inline vector<string> SplitInSpacesObeyQuotes(const string& s) {
    vector<string> lpraw = SplitInSomething(" ", true, s), lp;
    bool combine = false;
    for (size_t i=0; i<lpraw.size(); i++) {
      string& a = lpraw[i];
      bool combnext = combine;
      if (a[0]=='"') {
	combnext = true;
	a.erase(0, 1);
      }
      if (a.size() && a[a.size()-1]=='"') {
	combnext = false;
	a.erase(a.size()-1);
      }
      if (combine)
	lp[lp.size()-1] += " "+a;
      else
	lp.push_back(a);
      
      combine = combnext;
    }
    return lp;
  }

  /// Produces a vector of separated words.
  inline vector<string> SplitInCommasVector(const string& s) {
    return SplitInSomething(",", false, s);
  }

  /// Produces a list of separated words.
  inline list<string> SplitInCommasList(const string& s) {
    auto v = SplitInCommasVector(s);
    return {v.begin(), v.end()};
  }

  /// Produces a vector of separated words. (Should be deprecated.)
  inline vector<string> SplitInCommas(const string& s) {
    return SplitInCommasVector(s);
  }

  /// Produces a vector of separated words.
  vector<string> SplitInCommasObeyParentheses(const string& str);

  ///
  map<string,string> SplitCommaKeyValueMap(const string&);
  
  ///
  list<pair<string,string> > SplitCommaKeyValueList(const string&);
  
  /// Extracts f and a from "f(a)".
  bool SplitParentheses(const string& in, string& f, string& a, bool = false);

  /// Splits proto :// host [:port] [/path]
  bool SplitURL(const string&, string&, string&, string&, string&);

  /* Copied from Simple.h [start] */

  const char *Bold(const char *txt);

  const char *TimeStampP();

  void BackTraceMeHere(ostream& os);

  inline void AbortOnNewError() { /*set_new_handler(abort);*/ }
  void ConditionalAbort(bool b);
  bool ConditionalAbort(const char *f, int l);

  inline bool TrapAfterError() { return trap_after_error; }
  inline void TrapAfterError(bool t) { trap_after_error = t; }

//   bool JamAfterError() { return jam_after_error; }
//   void JamAfterError(bool t) { jam_after_error = t; }

  inline bool BackTraceBeforeTrap() { return backtrace_before_trap; }
  inline void BackTraceBeforeTrap(bool t) { backtrace_before_trap = t; }

  inline void TrapMeHere() {
    if (backtrace_before_trap)
      BackTraceMeHere(cerr);
#ifdef SIGTRAP
    kill(getpid(), SIGTRAP);
#else
    kill(getpid(), SIGABRT);
#endif // SIGTRAP
  }

  void LocateFileDebug(int d);

  const char *LocateFileRecursion(const char *dir, const char *name);

  const char *LocateFile(const char *name);

  ///
  bool FileExists(const char *name, bool locate = false);

  ///
  bool DirectoryExists(const string& name);

  /// Returns false if filename is empty or file doesn't exist.
  inline bool FileExists(const string& s, bool l = false) {
    return !s.empty() && FileExists(s.c_str(), l);
  }

  /* Copied from Simple.h [end] */

  /// 
  bool FileNameExtension(const string&, const string&, string&);

#ifndef NO_PTHREADS
  /// Human-readable version of pthread's identity.
  string ThreadIdentifierUtil(pthread_t, pid_t);
#endif // NO_PTHREADS

  /// Human-readable version of pthread's identity.
  inline string ThreadIdentifierUtil() {
#ifndef NO_PTHREADS
    return ThreadIdentifierUtil(pthread_self(), gettid());
#else
    return "no-threads";
#endif // NO_PTHREADS
  }

  inline bool ShowError(const string& a = "", const string& b = "", 
			const string& c = "", const string& d = "",
			const string& e = "", const string& f = "") {
    cerr << endl << TimeStamp()
	 << ThreadIdentifierUtil() << " ERROR: "
	 << a << b << c << d << e << f << endl << endl;
    if (TrapAfterError())
      TrapMeHere();
    
    return false;
  }

  inline string ShowErrorS(const string& a, const string& b="",
			   const string& c="", const string& d="",
			   const string& e="", const string& f="") {
    ShowError(a, b, c, d, e, f);
    return "";
  }

  ///
  bool IsAffirmative(const char *s);

  ///
  inline bool IsAffirmative(const string& s) {
    return IsAffirmative(s.c_str());
  }

  ///
  bool IsJson(const string&);
  
  ///
  bool IsXml(const string&);
  
  /// Issue once message.
  bool WarnOnce(const string& f);

  /// Message.
  bool Obsoleted(const string& f, bool trap = false);

  /// Removes whitespace from the beginning and end of a string.
  void StripWhiteSpaces(string& s);
  
  /// Replace spaces and tabs with %20 and %9.
  void EscapeWhiteSpace(string& s);

  /// Strip last word separated by specified character
  void StripLastWord(string& s, char sep='-');

  /// Return base name of file path (part after last '/')
  string BaseName(const string& fn);

  /// Returns true if string as the given ending.
  inline bool StringEnds(const string& str, const string& ext) {
    size_t sl = str.size(), el = ext.size();
    return sl>=el && str.rfind(ext)==sl-el;
  }

  /// Utility to show possibly binary strings.
  string HideBinaryData(const string&);
  
  /// Checks if line contains key=value and splits.  Delete results!
  bool SplitKeyEqualValueOld(const char*, const char*&, const char*&);

  /// Checks if line contains key=value and splits.
  bool SplitKeyEqualValue(const string& s, string& ks, string& vs);

  /// Checks if line contains key=value and splits.
  pair<string,string> SplitKeyEqualValueOld(const string& s);

  /// Checks if line contains key=value and splits.
  pair<string,string> SplitKeyEqualValueNew(const string& s) 
    /*throw(logic_error)*/;

  inline const char *CopyString(const char *s) {
    return s ? strcpy(new char[strlen(s)+1], s) : (char*)NULL; }

  const char *CopyString(const char*& d, const char *s);

  const char *CopyString(char*& d, const char *s);

  inline void CopyString(const char*& d, const string& s) {
    CopyString(d, s.c_str());
  }

  const char *CopyStringOldVersion(char*& d, const char *s);

  ///
  string StrError(int e = 0);
  
  /// Returns size of the file or -1 if inexistent.
  inline off_t FileSize(const string& n) {
    struct stat sbuf;
    return stat(n.c_str(), &sbuf) ? -1 : sbuf.st_size;
  }

  /// Returns human-readable size of the file or "-1" if inexistent.
  inline string FileSizeHumanReadable(const string& n) {
    off_t s = FileSize(n);
    return s>=0 ? HumanReadableBytes(s) : "-1";
  }

  /// Renames given file.
  inline bool Rename(const string& src, const string& dst) {
    return !rename(src.c_str(), dst.c_str());
  }

  /// Renames given file xxx.yyy to xxx.yyyext
  inline bool RenameToExt(const string& n, const string& ext) {
    return FileSize(n)==-1 ? true : Rename(n, n+ext);
  }

  /// Renames given file xxx.yyy to xxx.yyy.old.
  inline bool RenameToOld(const string& n) {
    //    return FileSize(n)==-1 ? true : Rename(n, n+".old");
    return RenameToExt(n,".old");
  }

  /// Links given file.
  inline bool Link(const string& src, const string& dst) {
    return !link(src.c_str(), dst.c_str());
  }

  /// Symlinks given file.
  inline bool Symlink(const string& src, const string& dst) {
    return !symlink(src.c_str(), dst.c_str());
  }

  /// Removes given file.
  inline bool Unlink(const string& n) {
    return !unlink(n.c_str());
  }

  /// Removes given directory.
  inline bool RmDir(const string& n) {
    return !rmdir(n.c_str());
  }

  /// Removes given directory.
  bool RmDirRecursive(const string& n);

  /// Converts cwd relative paths to absolute ones.
  string FullPath(const string& p);

  /// Converts absolute path to be relative with another one.
  string RelativePath(const string&, const string&);
  
  /// The work horse of the above two.
  const char *ExtensionVersusMIMEP(const string& ttt, bool to_mime);
  
  /// MIME Content-type from file extension.
  inline const char *FileExtensionToMIMEtypeP_xx(const string& e) {
    return ExtensionVersusMIMEP(e, true);
  }

  /// MIME Content-type from file extension.
  inline string FileExtensionToMIMEtype(const string& e) {
    const char *p = FileExtensionToMIMEtypeP_xx(e);
    return p ? p : "";
  }

  /// File extension from MIME Content-type.
  inline const char *MIMEtypeToFileExtensionP_xx(const string& m) {
    return ExtensionVersusMIMEP(m, false);
  }

  /// File extension from MIME Content-type.
  inline string MIMEtypeToFileExtension(const string& m) {
    const char *p = MIMEtypeToFileExtensionP_xx(m);
    return p ? p : "";
  }

  /// Converts eg. "JFIF" to "image/jpeg".
  inline void EnsureMIMEtype(string& t) {
    if (t=="JFIF")
      t = "image/jpeg";
  }

  ///
  map<string,string> ReadNumPyHeader(istream&, size_t&, bool = false);

  ///
  bool ReadNumPyHeader(istream&, size_t&, size_t&, size_t&, size_t&);

  ///
  bool ReadNumPyHeader(istream&, size_t&, size_t&, vector<size_t>&);

  ///
  vector<float> ReadNumPyVector(istream&, size_t, size_t, size_t, size_t,
				size_t, bool);

  ///
  bool WriteNumPyData(ostream&, const vector<size_t>&, const list<vector<float> >&);
  
  /// float16 to float32 conversion
  float FromFloat16(uint16_t);
  
  /// An utility to avoi utf probles for now...
  void Replace(string& s, char ch_old, const string& ch_new);

  /// An utility to avoi utf probles for now...
  inline void Replace(string& s, char ch_old, unsigned short v) {
    string vs = "  ";
    vs[0] = v/256;
    vs[1] = v%256;
    Replace(s, ch_old, vs);
  }

  /// An utility to avoi utf probles for now...
  string Latin1ToUTF(const string& s);

  ///
  size_t utf8length(const string& s);
  
  // Copied from Features
  bool GetMemoryUsage(int& pagesize, int& size, int& resident,
		      int& share, int& trs, int& drs, int& lrs,
		      int& dt);

  // Copied from Features
  void DumpMemoryUsage(ostream& os, bool with_diff);

  // Check mime type of file using libmagic
  pair<string,string> libmagic_mime_type(const string& fname);

  //
  const struct hostent* GetHostByName(const string& addr, 
                                      struct hostent& myhost);

  ///
  bool ReadEquationFile(const string&, map<string,string>&,
			map<string,double>&, map<string,double>&);
  
  ///
  bool WriteEquationFile(const string&, const map<string,string>&,
			 const map<string,double>&, const map<string,double>&);

  ///
  double SolveMath(const string&, const map<string,string>&, bool);

  ///
  vector<double> SolveMath(const vector<string>&, const map<string,string>&,
			   bool);

#ifdef HAVE_JAULA_H
  ///
  inline string JAULA_string(const JAULA::Value_Object& o, const string& f) {
    const JAULA::Value_String& s
      = *dynamic_cast<JAULA::Value_String*>(o.getData().at(f));
    return s.getData();
  }

  ///
  inline int JAULA_int(const JAULA::Value_Object& o, const string& f) {
    const JAULA::Value_Number_Int& s
      = *dynamic_cast<JAULA::Value_Number_Int*>(o.getData().at(f));
    return s.getData();
  }

  ///
  inline double JAULA_double(const JAULA::Value_Object& o, const string& f) {
    const JAULA::Value_Number& s
      = *dynamic_cast<JAULA::Value_Number*>(o.getData().at(f));
    return s.getData();
  }

#endif // HAVE_JAULA_H

  ///
  string OpenBlasVersion();

  ///
  double NiceUpperLimit(double v, size_t m = 0);

  ///
  inline double NiceUpperLimit(const vector<float>& v, size_t m = 0) {
    float max = 0;
    for (auto i=v.begin(); i!=v.end(); i++)
      if (*i>max)
	max = *i;
    return NiceUpperLimit(max, m);
  }

  ///
  string WashString(const string&, const string&);

  ///
  list<vector<pair<string,string> > >
  SparqlQuery(const string&, const string&, bool = false);
  
  ///
  list<vector<pair<string,string> > >
  SparqlQueryJena(const string&, const string&, bool = false);
  
  ///
  list<vector<pair<string,string> > >
  SparqlQueryRasqal(const string&, const string&, bool = false);
  
  ///
  string RdfToString(const list<pair<string,string> >&,
		     const list<vector<string> >&,
		     const string&);
  
} // namespace picsom

#endif // _PICSOM_UTIL_H_

