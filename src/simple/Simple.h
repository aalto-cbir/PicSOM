// -*- C++ -*-  $Id: Simple.h,v 1.30 2017/04/28 07:48:34 jormal Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _SIMPLE_H_
#define _SIMPLE_H_

//#include <missing-c-utils.h>
#include <picsom-config.h>
#include <simple_defines.h>

#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <new>
#include <cfloat>
using namespace std;

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif // HAVE_STRINGS_H

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif // HAVE_LIMITS_H

#ifdef HAVE_BASETSD_H
#include <basetsd.h>
#endif // HAVE_BASETSD_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <libxml/parser.h>

#ifndef NO_PTHREADS
#include <pthread.h>
#else
#ifndef _BITS_PTHREADTYPES_H
typedef int pthread_t;
#endif // _BITS_PTHREADTYPES_H
#define pthread_self() ((pthread_t)0)
#endif // NO_PTHREADS

extern "C" typedef void (*CFP_atexit)(); 
extern "C" typedef void (*CFP_signal)(int); 
extern "C" typedef void* (*CFP_pthread)(void*); 
extern "C" typedef int (*CFP_qsort)(const void*,const void*);

// existence of external libraries

#ifdef sgi
#define HAS_SGIMATH
#define IRIX_TELLG_CAST (long long)
#else
#define IRIX_TELLG_CAST
#endif // sgi

#ifdef sgi
#ifndef __GNUC__
#define HAS_IFL
#endif // !__GNUC__
#define HAS_SGI
#define HAS_TIFF
#define HAS_NISTDATA
#  if _MIPS_SZPTR!=64
// #define HAS_DAFSLIB
#  endif // _MIPS_SZPTR!=64
#endif // sgi

// modifications to code

#define HAS_SSC

#ifdef __GNUC__
// #define EXPLICIT_INCLUDE_CXX
// #define PRAGMA_INTERFACE
#endif // __GNUC__

#ifdef __alpha
#undef HAS_SSC
#endif // __alpha

#if USE_DBMALLOC
#define DBMALLOC_SKIP_MACROS
#include <dbmalloc.h>
#endif

//#define MEMCPY_DEBUG

#ifdef MEMCPY_DEBUG
#define Memcpy(x,y,z) \
cerr << "memcpy(" << z << ") " << __FILE__ << " " << __LINE__ << endl, \
memcpy(x,y,z)
#else
#define Memcpy(x,y,z) memcpy(x,y,z)
#endif

#define SimpleAbort() Simple::ConditionalAbort(__FILE__,__LINE__)
#define NonAmbiguousAbort(x) x ## ::ConditionalAbort(__FILE__,__LINE__)

#define FunctionNotImplemented(f) ShowError("function not implemented ", f)

#if defined(sgi) && defined(_OPENMP)
#include <mpc.h>
#endif // sgi && _OPENMP

#ifndef MAXDOUBLE
#define MAXDOUBLE DBL_MAX
#endif // MAXDOUBLE

#ifndef MAXFLOAT
#define MAXFLOAT FLT_MAX
#endif // MAXFLOAT

#ifndef MAXINT
#define MAXINT INT_MAX
#endif // MAXINT

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#define DATA_PATH_DEBUG_ENVVAR "DATA_PATH_DEBUG"
#define DATA_PATH_ENVVAR  "DATA_PATH"
#define DATA_PATH_DEFAULT ".:~/data:/share/local/data//:/usr/local/data//"

//- Simple
namespace simple {
class Simple {
  //. Simple object is the mother of all mothers

public:
  //
  virtual ~Simple() {}

  enum DumpMode {
    DumpNone  = 0,
    DumpShort = 1,
    DumpLong  = 2,
    DumpRecursive = 4,
    DumpRecursiveShort = DumpRecursive+DumpShort,
    DumpRecursiveLong  = DumpRecursive+DumpLong,
    DumpShortRecursive = DumpRecursive+DumpShort,
    DumpLongRecursive  = DumpRecursive+DumpLong,
    DumpDefault        = DumpRecursiveShort
  };

  enum Metric { NoMetric, FourNeigh, EightNeigh, Euclidean };

  static void SetOption(  int& var, int op) { var|=(1<<op); }
  static void UnsetOption(int& var, int op) { var&=(~(1<<op)); }
  static bool CheckOption(int& var, int op) { return ((1<<op)==(var&(1<<op)));}

  virtual void Dump(DumpMode = DumpDefault, ostream& = cout) const = 0;
  static void DumpSimple(DumpMode = DumpDefault, ostream& = cout);

  virtual const char **SimpleClassNames() const = 0;
  const char *SimpleClassName() const { return *SimpleClassNames(); }
  int Is(char*, int = FALSE) const;
  void Check(char*) const;

  virtual Simple *Descendant(int) const { return NULL; }

  static const char *SetString(const char*& d, const char *s) {
    delete d;
    d = s;
    return s;
  }

  static const char *CopyString(const char *s) {
    return s ? strcpy(new char[strlen(s)+1], s) : (char*)NULL; }

  static void CopyString(const char*& d, const string& s) {
    CopyString(d, s.c_str());
  }

  static const char *CopyString(const char*& d, const char *s) {
    char*& dd = *(char**)&d;
    return CopyString(dd, s);
  }

  static const char *CopyString(char*& d, const char *s) {
    if (s!=d) {
      if (s==NULL || d==NULL || strlen(s)!=strlen(d))
	return CopyStringOldVersion(d, s);
      else
	strcpy(d, s);
    }
    return d;
  }

  static const char *CopyStringOldVersion(char*& d, const char *s) {
    char *old = d;
    d = (char*)CopyString(s);
    delete old;
    return d;
  }

  static void AppendString(const char*& d, const char *s, bool f = false) {
    char*& dd = *(char**)&d;
    AppendString(dd, s, f);
  }

  static void AppendString(char*&, const char*, bool f = false);

  static const char *ShowString(const char*);

  static const char *ShowString(const string&);

  static string ShowStringStr(const char*);

  static string ShowStringStr(const string& s) {
    return string("\"")+s+"\"";
  }

  static string MakeMatlabCompliant(const string&);

  static int StringsMatch(const char *u, const char *v) {
    return u && v && !strcmp(u, v); }

  static bool GetMemoryUsage(int& pagesize, int& size, int& resident,
			     int& share, int& trs, int& drs,
			     int& lrs, int& dt);

  static void DumpMemoryUsage(ostream& = cout, bool = true);

  static void MallocOrigin();
  static void MallocList();
  static int  MallocDiffers();
  static int  MallocCheck(bool = false);

  static int MallocCKDATA(bool);

  static const char *LocateFile(const char*);
  static const char *LocateFileRecursion(const char*, const char*);

  static void LocateFileDebug(int d) { locate_file_debug = d; }

  static DumpMode dump_mode;

  static void AbortOnNewError() { /*set_new_handler(abort);*/ }
  static void ConditionalAbort(bool b) { do_abort = b; }
  static bool ConditionalAbort(const char *f, int l) {
    cerr << endl << Bold("ConditionalAbort() hit ") << f << " " << l
	 << endl << endl;
    
    if (backtrace_before_trap)
      BackTraceMeHere(cerr);

    if (do_abort)
      abort();
    return false;
  }

  static bool TrapAfterError() { return trap_after_error; }
  static void TrapAfterError(bool t) { trap_after_error = t; }

  static bool JamAfterError() { return jam_after_error; }
  static void JamAfterError(bool t) { jam_after_error = t; }

  static bool BackTraceBeforeTrap() { return backtrace_before_trap; }
  static void BackTraceBeforeTrap(bool t) { backtrace_before_trap = t; }

  static void ErrorExtraText(const string& s) { error_extra_text = s; }

  static bool ShowError(const char* =NULL,const char* =NULL,const char* =NULL,
			const char* =NULL,const char* =NULL,const char* =NULL);

  static bool ShowError(const string& a, const string& b="",
			const string& c="", const string& d="",
			const string& e="", const string& f="") {
    return ShowError(a.c_str(), b.c_str(), c.c_str(),
		     d.c_str(), e.c_str(), f.c_str());
  }

  static string ShowErrorS(const string& a, const string& b="",
			const string& c="", const string& d="",
			const string& e="", const string& f="") {
    ShowError(a, b, c, d, e, f);
    return "";
  }

  static const char *Bold(const char *);

  static int HasLinpack() {
#ifdef HAS_LINPACK
    return TRUE;
#else
    return FALSE;
#endif // HAS_LINPACK
  }

  static bool UseLinpack() { return HasLinpack() ? use_linpack : false; }

  static void UseLinpack(bool u) {
    use_linpack = u;
    if (!HasLinpack() && u)
      ShowError("UseLinpack(TRUE) void when there is no Linpack available.");
  }

  static int IsOpen(ofstream& os) { return os.rdbuf()->is_open(); }

  static int IsOpen(ifstream& os) { return os.rdbuf()->is_open(); }

  ///
  static bool Exists(const char*, bool = false);

  ///
  static bool Exists(const string& s, bool l = false) {
    return Exists(s.c_str(), l);
  }

  ///
  static bool FileExists(const char*, bool = false);

  /// Rerturns false if filename is empty or file doesn't exist.
  static bool FileExists(const string& s, bool l = false) {
    return !s.empty() && FileExists(s.c_str(), l);
  }

  ///
  static bool FileExistsPossiblyZipped(const string& s) {
    return FileExists(s) || FileExists(s+".gz");
  }

  ///
  static bool DirectoryExists(const char*, bool = false);

  ///
  static bool DirectoryExists(const string& s, bool l = false) {
    return DirectoryExists(s.c_str(), l);
  }

  ///
  static const char *UniqueFileName(const char*, bool = false);

  ///
  static string UniqueFileNameStr(const string& b, bool l = false) {
    const char *p = UniqueFileName(b.c_str(), l);
    string ret = p ? p : "";
    delete p;
    return ret;
  }

  static bool IsXMLHeader(const char *s) {
    return s && !strncmp(s, "<?xml version=\"1.0\"?>", 21); }

//   static xmlNodePtr FindXMLchildElem(xmlNodePtr ptr, const char *name) {
//     if (ptr) {
//       ptr = ptr->children;
//       while (ptr)
// 	if (ptr->type==XML_ELEMENT_NODE &&
// 	    !xmlStrcmp(ptr->name, (const xmlChar *)name))
// 	  return ptr;
// 	else
// 	  ptr = ptr->next;
//     }
//     return NULL; }

  ///
  static bool IsAffirmative(const char *s) {
    if (!s || !*s) return false;
    if (strlen(s)==1 && strchr("YyTt1", *s)) return true;
    if (!strcasecmp(s, "yes") || !strcasecmp(s, "true") ||
	!strcasecmp(s, "on")) return true;
    return false; }

  ///
  static bool IsAffirmative(const string& s) {
    return IsAffirmative(s.c_str());
  }

  /*
  static void MPsetLock() {
#if defined(sgi) && defined(_OPENMP)
    mp_setlock();
#endif // defined(sgi) && defined(_OPENMP)
  }
  static void MPunsetLock() {
#if defined(sgi) && defined(_OPENMP)
    mp_unsetlock();
#endif // defined(sgi) && defined(_OPENMP)
  }
  */

  static void TrapMeHere() {
    if (backtrace_before_trap)
      BackTraceMeHere(cerr);
#ifdef SIGTRAP
    kill(getpid(), SIGTRAP);
#else
    kill(getpid(), SIGABRT);
#endif // SIGTRAP
  }

  static void BackTraceMeHere(ostream& = cout);

  /// THE FOLLOWING METHODS USE Standard Template Library (STL)

  /// Returns sorted list of entries in directory of given path.
  static list<string> SortedDir(const string& path,
				const string& pattern = "",
				bool only_files = false,
				bool only_dirs = false);
  /// Clears timespec.
  static void ZeroTime(struct timespec& t) { memset(&t, 0, sizeof t); }

  /// Sets timespec.
  static void SetTimeNow(struct timespec& t) {
    // #ifdef sgi
    clock_gettime(CLOCK_REALTIME, &t);
    // #else
    // t.tv_sec = time(NULL); t.tv_nsec = 0;
    // #endif // sgi
  }

  static const char *TimeStampP() {
    struct timespec tt;
    SetTimeNow(tt);
    tm mytime;
    tm *time = localtime_r(&tt.tv_sec, &mytime);

    static char buf[100];
    // in POSIX 1003.1-2001 %T==%H:%M:%S 
    // but version 3.2 "g++ -pedantic -Wformat" (correctly) doesn't allow it
    strftime(buf, sizeof buf, "[%d/%b/%Y %H:%M:%S", time); 
    sprintf(buf+strlen(buf), ".%03d] ", int(tt.tv_nsec/1000000));

    return buf;
  }

  static string PthreadIdentity() { return PthreadIdentity(pthread_self()); }

  static string PthreadIdentity(pthread_t t) {
    char tmp[100];
    PthreadIdentity(tmp, t);
    return tmp;
  }

  static bool PthreadIdentity(char *buf) {
    return PthreadIdentity(buf, pthread_self());
  }

  static bool PthreadIdentity(char *buf, pthread_t t);

protected:
  static bool use_linpack;
  static bool do_abort;
  static bool trap_after_error;
  static bool jam_after_error;
  static bool backtrace_before_trap;

  static string error_extra_text;

  static int locate_file_debug;

  static unsigned long dbmalloc_id, dbmalloc_size;

}; // class Simple

// some usefull classes

// class Numbered : public Simple {
// public:
//   int  IsNumberSet() const { return number!=-1; }
//   int  Number() const { return number; }
//   void Number(int n)  { number = n; }
// protected:
//   int number;
// }

// class Labeled : public Simple {
// public:
//   void  Label(const char *l) { CopyString(label, l); }
//   char *Label()      const       { return label;                       }
//   int   EmptyLabel() const       { return !label || !*label;           }
//   int   HasLabel() const         { return !EmptyLabel();               }
//   int   LabelsMatch(const char *str) const {
//     return !label && !str || StringsMatch(label, str); }
//   int   LabelsMatch(const VectorOf& v) const { return LabelsMatch(v.label); }
// protected:
//   char *label;
// }

// some classless stuff:

extern void dump(Simple*);
extern void names(Simple*);

inline int my_atoi(register const char *str) {
  register const char *p = str;
  register int s = 0, ten = 10;
  int minus = 0;

  if (*p=='-') {
    p++;
    minus = 1;

  } else if (*p=='+')
    p++;

  for (;;) {
    register int i = *p;
    if (!i)
      break;

    p++;

    if (i>='0' && i<='9')
      s = ten*s+i-'0';
    else {
      cerr << "my_atoi failed with " << str << endl;
      return atoi(str);
    }
  }

  return minus ? -s : s;
}

inline double my_atof(register const char *str) {
  register const char *p = str;
  register double s = 0, ten = 10;
  int minus = 0;

  if (*p=='-') {
    p++;
    minus = 1;
  }

  for (;;) {
    register int i = *p;
    if (!i)
      break;

    p++;

    if (i>='0' && i<='9')
      s = ten*s+i-'0';
    else if (i=='.')
      break;
    else {
      p--;
      break;
    }
  }

  register double m = 0.1;

  for (;;) {
    register int i = *p;
    if (!i)
      break;

    p++;

    if (i>='0' && i<='9')
      s += (i-'0')*m;
    else if (i=='e') {
      i = my_atoi(p);
      if (i<0)
	while (i++)
	  s /= ten;
      else if (i>0)
	while (i--)
	  s *= ten;
      break;
    } else {
      char buf[100];
      sprintf(buf,"my_atof failed with %s (%f) :%d",str,(minus ? -s : s),i);
      
      Simple::ShowError(buf);
      return atof(str);
    }
    m /= ten;
  }

  return minus ? -s : s;
}

} // namespace simple
#endif // _SIMPLE_H_

