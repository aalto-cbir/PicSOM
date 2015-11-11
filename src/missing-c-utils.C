// -*- C++ -*-  $Id: missing-c-utils.C,v 2.15 2013/11/06 15:28:44 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <missing-c-utils.h>

#include <string>
#include <iostream>
#include <set>
using namespace std;

void _warn_missing_(const string& s) {
  static set<string> done;

  if (done.find(s)!=done.end())
    return;
      
  done.insert(s);

  cerr << endl << endl 
       << "missing-c-utils warning: " << s << " not implemented!" << endl
       << endl << endl;
}

#ifndef HAVE_FORK
#include <stdio.h>
pid_t fork() {
  _warn_missing_("fork()");
  return 1;
}
#endif // HAVE_FORK

#ifndef HAVE_KILL
#include <stdio.h>
int kill(pid_t pid,int sig) {
  _warn_missing_("kill()");
  return -1;
}
#endif // HAVE_KILL

#ifndef HAVE_SLEEP
#if HAVE_WINSOCK2_H
#include <winsock2.h>
unsigned int sleep(unsigned int seconds) {
  Sleep(seconds);
  return 0;
}
#elif HAVE__SLEEP
#include <stdlib.h>
unsigned int sleep(unsigned int seconds) {
  return _sleep(seconds);
}
#else
#include <stdio.h>
unsigned int sleep(unsigned int seconds) {
  _warn_missing_("sleep()");
  return seconds;
}
#endif
#endif // HAVE_SLEEP

#ifndef HAVE_GETUID
uid_t getuid() {
  // _warn_missing_("getuid()");
  return 42;
}
#endif // HAVE_GETUID

#ifndef HAVE_GETPWUID
struct passwd *getpwuid(uid_t uid) {
  static struct passwd p = {
    (char*)"Name",
    (char*)"Password",
    42,
    24,
    (char*)"Real Name",
    (char*)".",
    (char*)""
  };
  return &p;
}
#endif // HAVE_GETPWUID

#ifndef HAVE_GETPWNAM
struct passwd *getpwnam(const char *name) {
  static struct passwd p = {
    (char*)"Name",
    (char*)"Password",
    42,
    24,
    (char*)"Real Name",
    (char*)".",
    (char*)""
  };
  _warn_missing_("getpwnam()");
  return &p;
}
#endif // HAVE_GETPWNAM

#ifndef HAVE_INET_ATON
#if defined(HAVE_NETINET_IN_H) && defined(HAVE_ARPA_INET_H)
#include <netinet/in.h>
#include <arpa/inet.h>
int inet_aton(const char *name, struct in_addr *ad) {
  ad->S_un.S_addr = inet_addr(name);
  return 1;
}
#else
int inet_aton(const char *name, struct in_addr *ad) {
  return 0;
}
#endif // HAVE_NETINET_IN_H && HAVE_ARPA_INET_H
#endif // HAVE_INET_ATON

#ifndef HAVE_TIMES
clock_t times (tms_t *t) {
  return clock();
}
#endif // HAVE_TIMES

#ifndef HAVE_CLOCK_GETTIME
#include <sys/timeb.h>
#include <iostream>
using namespace std;
int clock_gettime(int, timespec_t *t) {
  struct timeb tb;
  ftime(&tb);
  // cout << "clock_gettime() : " << tb.time << "." << tb.millitm << endl;
  if (t) {
    t->tv_sec  = tb.time;
    t->tv_nsec = 1000000*tb.millitm;
    // cout << "  t->tv_sec=" << t->tv_sec << " t->tv_nsec=" << t->tv_nsec
    //      << endl;
  }
  return 0;
}
#endif // HAVE_CLOCK_GETTIME

#ifndef HAVE_LOCALTIME_R
tm *localtime_r(const time_t *t, tm *tt) {
  tm *ti = localtime(t);
  memcpy(tt, ti, sizeof(tm));

  // tt->tm_sec   = ti->tm_sec;
  // tt->tm_min   = ti->tm_min;
  // tt->tm_hour  = ti->tm_hour;
  // tt->tm_mday  = ti->tm_mday;
  // tt->tm_mon   = ti->tm_mon;
  // tt->tm_year  = ti->tm_year;
  // tt->tm_wday  = ti->tm_wday;
  // tt->tm_yday  = ti->tm_yday;
  // tt->tm_isdst = ti->tm_isdst;

  return tt;
}
#endif // HAVE_LOCALTIME_R

#ifndef HAVE_STRPTIME
#include <time.h>
char *strptime(const char *s, const char* /*format*/, struct tm* /*tm*/) {
  // not really implemented as you can see...
  return (char*)s;
}
#endif // HAVE_STRPTIME

#ifndef HAVE_NANOSLEEP
#include <time.h>
// extern "C" unsigned int sleep(unsigned int seconds);

int nanosleep(const struct timespec *req, struct timespec *rem) {
  int s = req->tv_sec;
  if (!s)
    s = 1;
  else if (req->tv_nsec>=500000000)
    s++;
  sleep(s);
  if (rem)
    rem->tv_sec = rem->tv_nsec = 0;

  return 0;
}
#endif // HAVE_NANOSLEEP

#ifndef HAVE_GETHOSTNAME
int gethostname(char *name, size_t len) {
  _warn_missing_("gethostname()");
  if (name)
    strncpy(name, "myhost", len);
  return 0;
}
#endif // HAVE_GETHOSTNAME

#ifndef HAVE_GETPAGESIZE
int getpagesize() {
  _warn_missing_("getpagesize()");
  return 1;
}
#endif // HAVE_GETPAGESIZE

#ifndef HAVE_GETLOADAVG
// possibly a bug in MinGW: the getpagesize function implementation is
// defined in the compiled library files, but the headers are missing.
int getloadavg(double*, int) {
  _warn_missing_("getloadavg()");
  return 0;
}
#endif // HAVE_GETLOADAVG

#ifndef HAVE_GETCWD
char *getcwd(char* /*buf*/, size_t /*len*/) {
  _warn_missing_("getcwd()");
  return NULL;
}
#endif // HAVE_GETCWD

#ifndef HAVE_GETPID
pid_t getpid() {
  _warn_missing_("getpid()");
  return 0;
}
#endif // HAVE_GETPID

#ifndef HAVE_LINK
int link(const char* /*oldpath*/, const char* /*newpath*/) {
  _warn_missing_("link()");
  return -1;
}
#endif // HAVE_LINK

#ifndef HAVE_SYMLINK
int symlink(const char* /*oldpath*/, const char* /*newpath*/) {
  _warn_missing_("symlink()");
  return -1;
}
#endif // HAVE_SYMLINK

#ifndef HAVE_STRCASECMP
#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H
int strcasecmp(const char *s1, const char *s2) {
  _warn_missing_("strcasecmp()");
  return strcmp(s1, s2);
}
#endif // HAVE_STRCASECMP

#ifndef HAVE_MKSTEMP
int mkstemp(char*) {
  _warn_missing_("mkstemp");
  return 0; 
}
#endif // HAVE_MKSTEMP

#ifndef HAVE_ERAND48
double erand48(unsigned short /*xsubi*/[3]) {
  _warn_missing_("erand48");
  return 0;
}
#endif // HAVE_ERAND48

#ifndef HAVE_NRAND48
long int nrand48(unsigned short /*xsubi*/[3]) {
  _warn_missing_("nrand48");
  return 0;
}
#endif // HAVE_NRAND48

#ifndef HAVE_RAND_R
int rand_r(unsigned int* /*seedp*/) {
  _warn_missing_("rand_r");
  return 0;
}
#endif // HAVE_RAND_R

#ifndef HAVE_POPEN
#include <io.h>
FILE *popen(const char* /*command*/, const char* /*type*/) {
  _warn_missing_("popen");
  return NULL;
}
#endif // HAVE_POPEN

#ifndef HAVE_PCLOSE
#include <io.h>
int pclose(FILE* /*stream*/) { 
  _warn_missing_("pclose");
  return -1; 
}
#endif // HAVE_PCLOSE

#ifndef HAVE_ERF
double erf(double) {
  _warn_missing_("erf");
  return 0; 
}
#endif // HAVE_ERF

#ifndef HAVE_GAMMA
double gamma(double) {
  _warn_missing_("gamma");
  return 0; 
}
#endif // HAVE_GAMMA

#ifndef HAVE_SNPRINTF
int snprintf(char *, size_t, const char *, ...) {
  _warn_missing_("snprintf");
  return 0; 
}
#endif // HAVE_SNPRINTF

#ifndef HAVE_FCNTL
int fcntl(int, int, ...) {
  _warn_missing_("fcntl");
  return 0; 
}
#endif // HAVE_FCNTL

// this is a bit tricky as we do have regex.h from glibc...
#ifndef HAVE_REGEX_H
#include <regex.h>
int regcomp(regex_t* /*preg*/, const char* /*regex*/, int /*cflags*/) {
  _warn_missing_("regcomp()");
  return 0;
}
int regexec(const  regex_t* /*preg*/, const char* /*string*/, 
            size_t /*nmatch*/, regmatch_t /*pmatch*/[], int /*eflags*/) {
  _warn_missing_("regexec()");
  return 0;
}
size_t regerror(int /*errcode*/, const regex_t* /*preg*/, char* /*errbuf*/, 
                size_t /*errbuf_size*/) {
  _warn_missing_("regerror()");
  return 0;
}      
void regfree(regex_t* /*preg*/) {
  _warn_missing_("regfree()");
}
#endif // HAVE_REGEX_H

