// -*- C++ -*-  $Id: TimeUtil.h,v 2.21 2015/03/20 16:14:40 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_TIMEUTIL_H_
#define _PICSOM_TIMEUTIL_H_

#include <missing-c-utils.h>

#include <string>
#include <vector>
#include <sstream>
using namespace std;

#ifdef HAVE_TIME_H
#include <time.h>
#endif // HAVE_TIME_H

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif // HAVE_STDLIB_H

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

namespace picsom {
  static const string Time_h_vcid =
    "@(#)$Id: TimeUtil.h,v 2.21 2015/03/20 16:14:40 jorma Exp $";

  /**
     DOCUMENTATION MISSING
  */

  /// Clears timespec.
  inline void SetTimeZero(timespec_t& t) { memset(&t, 0, sizeof t); }

  /// Clears timespec.
  inline timespec_t TimeZero() { 
    timespec_t t;
    SetTimeZero(t);
    return t;
  }

  ///
  inline bool IsTimeZero(const timespec_t& t) {
    return t.tv_sec==0 && t.tv_nsec==0;
  }

  /// Sets timespec from file's modification time.
  inline void SetTime(timespec_t& t, const string& f) {
    SetTimeZero(t);
    struct stat st;
    if (!stat(f.c_str(), &st))
#ifdef sgi
      t = st.st_mtim;
#else // ie. __linux__ + __alpha
      t.tv_sec = st.st_mtime;
#  ifdef __alpha
      t.tv_nsec = st.st_umtime*1000; 
#  endif // __alpha
#endif // sgi
  }

  /// Sets timespec.
  inline void SetTimeNow(timespec_t& t) {
    // #ifdef sgi
    clock_gettime(CLOCK_REALTIME, &t);
    // #else
    // t.tv_sec = time(NULL); t.tv_nsec = 0;
    // #endif // sgi
  }

  /// Returns current time.
  inline timespec_t TimeNow() {
    timespec_t t;
    SetTimeNow(t);
    return t;
  }

  /// Replacement for Simple::TimeStampP().
  inline string TimeStamp() {
    timespec_t tt;
    SetTimeNow(tt);
    tm mytime;
    tm *time = localtime_r(&tt.tv_sec, &mytime);
      
    char buf[100];
    strftime(buf, sizeof buf, "[%d/%b/%Y %H:%M:%S", time); 
    sprintf(buf+strlen(buf), ".%03d] ", int(tt.tv_nsec/1000000));
    
    return buf;
  }

  /// Time in string. (Implementation in PicSOM.C)
  string TimeString(const timespec_t& t, bool = false);

  /// xs:dateTime. (Implementation in PicSOM.C)
  string XSdateTime(const timespec_t& t);

  /// Time in string.
  inline string TimeDotString(const timespec_t& ts) {
    ostringstream tmp;
    tmp << "00000000" << ts.tv_nsec;
    string nsstr = tmp.str();
    nsstr.erase(0, nsstr.size()-9);

    ostringstream ss;
    ss << ts.tv_sec << "." << nsstr;
    return ss.str();
  }

  /// Time from string.
  inline timespec_t TimeFromDotString(const string& s) {
    timespec_t ts = TimeZero();
    size_t p = s.find('.');
    if (p==string::npos)
      return ts;

    ts.tv_sec  = atol(s.substr(0, p).c_str());
    ts.tv_nsec = atol(s.substr(p+1).c_str());

    return ts;
  }

  ///
  inline void TimeNormalize(timespec_t& ts) {
    while (ts.tv_nsec>1000000000) {
      ts.tv_nsec -= 1000000000;
      ts.tv_sec += 1;
    }
    while (ts.tv_nsec<0) {
      ts.tv_nsec += 1000000000;
      ts.tv_sec -= 1;
    }
  }

  ///
  inline void TimeAdd(timespec_t& ts, double t) {
    long s  = long(floor(t));
    long ns = long(1000000000.0*(t-s));
    ts.tv_sec  += s;
    ts.tv_nsec += ns;
    TimeNormalize(ts);
  }

  /// Returns true if times are equal.
  inline bool TimesEqual(const timespec_t& f, const timespec_t& l) {
    return f.tv_sec==l.tv_sec && f.tv_nsec==l.tv_nsec;
  }

  /// Returns true if former time is more recent than latter.
  inline bool MoreRecent(const timespec_t& f, const timespec_t& l,
			 string *d = NULL) {
    bool r = f.tv_sec>l.tv_sec ||
      (f.tv_sec==l.tv_sec && f.tv_nsec>l.tv_nsec);
    if (d)
      *d = TimeString(f) + (r?">":"<=") + TimeString(l);
    
    return r;
  }

  /// Returns true if former time is more recent than latter.
  inline bool MoreRecent(const time_t& f, const timespec_t& l, string*) {
    return f>l.tv_sec || (f==l.tv_sec && l.tv_nsec);
  }

  /// Returns true if modified time in stat is more recent than given time.
  inline bool MoreRecent(const struct stat& s, const timespec_t& l,
			 string *d = NULL) {
#if defined(sgi)
    return MoreRecent(s.st_mtim, l, d); 
#elif defined(__alpha)
    timespec_t t = { s.st_mtime, 1000*s.st_umtime} ;
    return MoreRecent(t, l, d);
#else
    return MoreRecent(s.st_mtime, l, d);
#endif
  }

  /// Returns true if given time is more recent than given number of seconds.
  inline bool MoreRecent(const timespec_t& t, int s, string *d = NULL) {
    timespec_t now; 
    SetTimeNow(now); 
    now.tv_sec-=s;
    return MoreRecent(t, now, d);
  }

  /// Returns true if modified time of given file is more recent than given.
  inline bool MoreRecent(const string& fname, const timespec_t& l,
			 string *d = NULL) {
    if (d)
      *d = "";
    struct stat st;
    return fname!="" && !stat(fname.c_str(), &st) && MoreRecent(st, l, d);
  }

  /// Returns true if modified time of 1st file is more recent than 2nd's.
  inline bool MoreRecent(const string& f1, const string& f2,
			 string *d = NULL) {
    timespec_t t1, t2;
    SetTime(t1, f1);
    SetTime(t2, f2);
    return MoreRecent(t1, t2, d);
  }
  
  /// Returns difference of the two times in seconds.
  inline double TimeDiff(const timespec_t& a, const timespec_t& b) {
    return a.tv_sec-b.tv_sec+(a.tv_nsec-b.tv_nsec)*1.0e-9;
  }

  /// Return modification time of a file.
  inline timespec_t FileModTime(const string& f) {
    timespec_t ts;
    memset(&ts, 0, sizeof ts);

    struct stat st;
    if (!stat(f.c_str(), &st))
      ts.tv_sec = st.st_mtime;

    return ts;
  }

  ///
  inline void SecToMinHour(double& s, int& m, int& h) {
    m = (int)s/60;
    h = m/60;
    m %= 60;
    s -= (60*h+m)*60;
  }

  ///
  inline double TimeStrToSec(const string& s) {
    double sec = 0;
    size_t p = 0;
    for (;;) {
      size_t q = s.find(':', p);
      if (q==string::npos)
	break;
      sec *= 60;
      sec += atoi(s.substr(p, q-p).c_str());
      p = q+1;
    }
    sec *= 60;
    sec += atof(s.substr(p).c_str());
    return sec;
  }

  ///
  inline string SbatchTimeFromSec(size_t s) {
    size_t m = s/60, h = m/60, d = h/24;
    h %= 24;
    m %= 60;
    s %= 60;
    stringstream ss;
    if (d)
      ss << d << "-";
    if (d||h)
      ss << h << ":";
    ss << m << ":" << s;

    return ss.str();
  }


  ///
  inline size_t SbatchTimeToSec(const string& tin) {
    string t = tin;

    if (t.find_first_not_of("0123456789-:")!=string::npos)
      return 0;

    size_t d = t.find('-'), c = t.find(':');
    if (d!=string::npos && c!=string::npos && c<d)
      return 0;

    size_t s = 0;
    if (d!=string::npos) {
      s = atoi(t.substr(0, d).c_str())*24*3600;  // days-
      t.erase(0, d+1);
    }

    vector<int> v;
    for (size_t p=0; p<t.size();) {
      size_t q = t.find(':', p);
      if (q==string::npos) {
	v.push_back(atoi(t.substr(p).c_str()));
	break;
      }
      v.push_back(atoi(t.substr(p, q-p).c_str()));
      p = q+1;
    }

    if (v.size()<1 || v.size()>3)
      return 0;

    if (d!=string::npos) {
      size_t m = 3600;
      for (size_t i=0; i<v.size(); i++) {
	s += v[i]*m;
	m /= 60;
      }
    } else if (v.size()==1)
      s = v[0]*60;
    else if (v.size()==2)
      s = v[0]*60+v[1];
    else
      s = v[0]*3600+v[1]*60+v[2];

    return s;
  }

} // namespace picsom

#endif // _PICSOM_TIMEUTIL_H_

