// -*- C++ -*-  $Id: missing-c-utils.h,v 2.24 2014/04/02 09:31:09 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

/** Implementations for C utils that are not supported by the architecture.
 *  The configure script checks if the functions and headers needed exists, 
 *  and generates config.h file accordingly. This header file and the 
 *  corresponding .c file defines the implementations for the missing 
 *  functions.
 */

#ifndef _MISSING_C_UTILS_H_
#define _MISSING_C_UTILS_H_

#ifdef __cplusplus
//extern "C" {
#endif // __cplusplus

#include <config.h>

// ugly hack

#ifdef __APPLE__
#undef  ICONV_CONST
#define ICONV_CONST
#endif // __APPLE__

#ifdef __MINGW32__
#ifndef ICONV_CONST
#define ICONV_CONST const
#endif // ICONV_CONST
#endif // __MINGW32__

// another one

#ifdef HAVE_ILAENV_
#define HAS_LINPACK
#endif // HAVE_ILAENV_

// and then the third one

#ifdef HAVE_WINSOCK2_H
#define HAVE_SOCKADDR_IN 1
#define HAVE_GETHOSTBYNAME 1
#define HAVE_GETHOSTNAME 1
#define HAVE_NTOHS 1
#undef PICSOM_USE_CSOAP
#undef HAVE_MYSQL
#undef HAVE_SLEEP
#undef FindExecutable
#undef max
#undef SetProp
#undef GetProp
#endif // HAVE_WINSOCK2_H

// TYPES

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifndef HAVE_TIME_T
typedef long time_t;
#endif // HAVE_TIME_T

#ifndef HAVE_STRUCT_TIMESPEC
#ifdef HAVE_TIME_H
#include <time.h>
#endif // HAVE_TIME_H
struct timespec {
  time_t tv_sec;
  time_t tv_nsec;
};
#define HAVE_STRUCT_TIMESPEC 1
#endif // HAVE_STRUCT_TIMESPEC

#ifndef HAVE_TIMESPEC_T
typedef struct timespec timespec_t;
#endif // HAVE_TIMESPEC_T

#ifndef HAVE_STRUCT_HOSTENT
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
};
#endif // HAVE_STRUCT_HOSTENT

#ifndef HAVE_SOCKADDR_IN
typedef struct {
  int sin_port;
  struct {
    unsigned int s_addr;
  } sin_addr;
} sockaddr_in;
#endif // HAVE_SOCKADDR_IN

#ifndef HAVE_SSIZE_T
typedef int ssize_t;
#endif // HAVE_SSIZE_T

#ifndef HAVE_UID_T
typedef int uid_t;
typedef int gid_t;
#endif // HAVE_UID_T

#ifndef HAVE_PID_T
typedef int pid_t;
#endif // HAVE_PID_T

#ifndef HAVE_MODE_T
typedef int mode_t;
#endif // HAVE_MODE_T

#if !HAVE_DECL_CLOCK_REALTIME
#define CLOCK_REALTIME 42
#endif // HAVE_DECL_CLOCK_REALTIME

#ifdef HAVE_MATH_H
// MSC needs this for M_PI etc.
#define _USE_MATH_DEFINES
#include <math.h>
#endif // HAVE_MATH_H

#ifndef isnan
#define isnan(x) (0)
#endif // isnan

#if !HAVE_DECL_M_PI
#define M_PI 3.14159265358979323846
#define M_SQRT2 1.41421356237309504880
#endif // HAVE_DECL_M_PI

#ifndef HAVE_INTTYPES_H
//ms visual studio 9.0
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#else
#include <inttypes.h>
#endif // HAVE_INTTYPES_H

#ifndef HAVE_SYS_TIMES_H
#include <time.h>
struct tms {
  clock_t tms_utime;
  clock_t tms_stime;
  clock_t tms_cutime;
  clock_t tms_cstime;
};
typedef struct tms tms_t;
#endif // HAVE_SYS_TIMES_H

#ifndef HAVE_PWD_H
struct passwd {
  char *pw_name;
  char *pw_passwd;
  uid_t pw_uid;
  gid_t pw_gid;
  char *pw_gecos;
  char *pw_dir;
  char *pw_shell;
};
#endif // HAVE_PWD_H

#if !HAVE_DECL_W_OK
#define W_OK 1234
#endif // HAVE_W_OK

// FUNCTIONS:

#ifndef HAVE_FORK
pid_t fork();
#endif // HAVE_FORK

#ifndef HAVE_KILL
int kill(pid_t,int);
#endif // HAVE_KILL

#ifndef HAVE_SLEEP
unsigned int sleep(unsigned int seconds);
#endif // HAVE_SLEEP

#ifndef HAVE_GETUID
uid_t getuid();
#endif // HAVE_GETUID

#ifndef HAVE_GETPWUID
struct passwd *getpwuid(uid_t uid);
#endif // HAVE_GETPWUID

#ifndef HAVE_GETPWNAM
struct passwd *getpwnam(const char *name);
#endif // HAVE_GETPWNAM

#ifndef HAVE_INET_ATON
int inet_aton(const char *cp, struct in_addr *inp);
#endif // HAVE_INET_ATON

#ifndef HAVE_NTOHS
uint16_t ntohs(uint16_t hostshort);
#endif // HAVE_NTOHS

#ifndef HAVE_TIMES
#ifdef HAVE_SYS_TIMES_H
typedef void tms_t;
#endif // HAVE_SYS_TIMES_H
typedef long long off64_t;
typedef long off_t;
clock_t times(tms_t*);
#endif // HAVE_TIMES

#ifndef HAVE_CLOCK_GETTIME
int clock_gettime(/*clockid_t*/ int, struct timespec *t);
#endif // HAVE_CLOCK_GETTIME

#ifndef HAVE_LOCALTIME_R
#include <time.h>
tm *localtime_r(const time_t *t, tm *tt);
#endif // HAVE_LOCALTIME_R

#ifndef HAVE_STRPTIME
#include <time.h>
char *strptime(const char *s, const char *format, struct tm *tm);
#endif // HAVE_STRPTIME

#ifndef HAVE_NANOSLEEP
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);
#endif // HAVE_NANOSLEEP

#ifndef HAVE_GETHOSTNAME
int gethostname(char *name, size_t len);
#endif // HAVE_GETHOSTNAME

#ifndef HAVE_GETHOSTBYNAME
struct hostent *gethostbyname(char *name);
#endif // HAVE_GETHOSTBYNAME

#ifndef HAVE_GETPAGESIZE
int getpagesize();
#endif // HAVE_GETPAGESIZE

#ifndef HAVE_GETLOADAVG
int getloadavg(double*, int);
#endif // HAVE_GETLOADAVG

#ifndef HAVE_GETCWD
char *getcwd(char* buf, size_t len);
#endif // HAVE_GETCWD

#ifndef HAVE_GETPID
pid_t getpid(void);
#endif // HAVE_GETPID

#ifndef HAVE_LINK
int link(const char *oldpath, const char *newpath);
#endif // HAVE_LINK

#ifndef HAVE_SYMLINK
int symlink(const char *oldpath, const char *newpath);
#endif // HAVE_SYMLINK

#ifndef HAVE_RMDIR
int rmdir(const char *pathname);
#endif // HAVE_RMDIR

#ifndef HAVE_ROUND
#define round(x) (floor(x+0.5))
#endif // HAVE_ROUND

#ifndef HAVE_STRCASECMP
extern "C" int strcasecmp(const char *s1, const char *s2);
#endif // HAVE_STRCASECMP

#ifndef HAVE_MKSTEMP
int mkstemp(char*);
#endif // HAVE_MKSTEMP

#ifndef HAVE_ERAND48
double erand48(unsigned short xsubi[3]);
#endif // HAVE_ERAND48

#ifndef HAVE_NRAND48
long int nrand48(unsigned short xsubi[3]);
#endif // HAVE_NRAND48

#ifndef HAVE_RAND_R
int rand_r(unsigned int *seedp);
#endif // HAVE_RAND_R

#ifndef HAVE_CHMOD
int chmod(const char *path, mode_t mode);
#endif // HAVE_CHMOD

#ifndef HAVE_MKDIR
int mkdir(const char *pathname, mode_t mode);
#endif // HAVE_MKDIR

#ifndef HAVE_ACCESS
int access(const char *pathname, int mode);
#endif // HAVE_ACCESS

#ifndef HAVE_STDXXX_FILENO
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif // HAVE_STDXXX_FILENO

#ifndef HAVE_POPEN
#include <stdio.h>
FILE *popen(const char *command, const char *type);
#endif // HAVE_POPEN

#ifndef HAVE_PCLOSE
int pclose(FILE *stream);
#endif // HAVE_PCLOSE

#ifndef HAVE_ERF
double erf(double x);
#endif // HAVE_ERF

#ifndef HAVE_GAMMA
double gamma(double x);
#define signgam 1
#endif // HAVE_GAMMA

#ifndef HAVE_SNPRINTF
int snprintf(char *str, size_t size, const char *format, ...);
#endif // HAVE_SNPRINTF

#ifndef HAVE_FCNTL
int fcntl(int, int, ...);
#define F_GETFL 0
#define F_SETFL 0
#define O_NONBLOCK 0
#endif // HAVE_FCNTL

#ifndef HAVE_UNLINK
int unlink(const char*);
#endif // HAVE_UNLINK

#ifdef __cplusplus
//}
#endif // __cplusplus

#endif // _MISSING_C_UTILS_H_
