// -*- C++ -*-  $Id: Simple.C,v 1.21 2016/10/25 08:16:55 jorma Exp $
// 
// Copyright 1994-2016 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2016 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <Simple.h>

#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif // HAVE_STROPTS_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#include <math.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif // HAVE_PWD_H

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif // HAVE_SYS_SYSCALL_H
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif // HAVE_SYS_PARAM_H
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef __linux__
#include <execinfo.h>
#include <cxxabi.h>
#endif // __linux__

#if defined(sgi) || defined(__alpha)
#include <sys/procfs.h>
#endif // defined(sgi) || defined(__alpha)

#ifdef __APPLE__
#include <mach/mach.h>
#endif // __APPLE__

#if !USE_DBMALLOC
static unsigned long malloc_inuse(unsigned long*) { return 0; }
static void malloc_list(int, unsigned long, unsigned long) {}
static int malloc_chain_check(int) { return 0; }
union dbmalloptarg { int i; char *str; }; 
static int dbmallopt(int, dbmalloptarg*) { return 0; }
#define MALLOC_CKDATA 0
#endif // USE_DBMALLOC

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif // PATH_MAX

#ifndef MAXPATHLEN
#define MAXPATHLEN PATH_MAX
#endif // MAXPATHLEN

#ifndef S_ISREG
#define S_ISREG(x) (1)
#endif // S_ISREG

namespace simple {

bool Simple::use_linpack      = true;
bool Simple::do_abort         = true;
bool Simple::trap_after_error = false;
bool Simple::jam_after_error = false;
bool Simple::backtrace_before_trap = false;
string Simple::error_extra_text;

int Simple::locate_file_debug = -1;
unsigned long Simple::dbmalloc_id;
unsigned long Simple::dbmalloc_size;
Simple::DumpMode Simple::dump_mode =  Simple::DumpDefault;

///////////////////////////////////////////////////////////////////////////

void dump(Simple *x) {
  x->Dump(Simple::dump_mode);
}

///////////////////////////////////////////////////////////////////////////

void dumpmode(int m) {
  Simple::dump_mode = (Simple::DumpMode)m;
}

///////////////////////////////////////////////////////////////////////////

void names(Simple *x) {
  for (const char **list = x->SimpleClassNames(); list && *list; list++)
    cout << *list << " ";
  cout << endl;
}

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

int Simple::Is(char *name, int descend) const {
  for (const char **list = SimpleClassNames(); *list; list++)
    if (!strcmp(*list, name)) {
      if (!descend)
	return 1;
      else for (int i=0;; i++) {
	Simple *tmp = Descendant(i);
	if (!tmp)
	  return 1;

	if (!tmp->Is(name, TRUE))
	  return 0;
      }
    }

  return 0;
}

///////////////////////////////////////////////////////////////////////////

void Simple::Check(char *type) const {
  // if (this==NULL) {
  //   cerr << "NULL pointer to Simple checked..." << endl;
  //   return;
  // }

  if (!Is(type)) {
    cerr << "Type of ";
    Dump(DumpRecursiveLong, cerr);
    cerr << "is not " << type << endl;
  }
}

///////////////////////////////////////////////////////////////////////////

void Simple::AppendString(char*& d, const char *s, bool f) {
  char *old = d;

  if (s) {
    int ld = old ? (int)strlen(old) : 0;
    int ls = (int)strlen(s);

    d = new char[ld+ls+1];
    if (old)
      Memcpy(d, old, ld);
    strcpy(d+ld, s);
  } else 
    d = NULL;

  delete [] old;
  if (f)
    delete [] s;
}

///////////////////////////////////////////////////////////////////////////

const char *Simple::ShowString(const char *s) {
  static char *str = NULL;

  char tmp[1000];
  sprintf(tmp, "0x%p", (void*)s);
  
  size_t l = strlen(tmp);
  if (s)
    l += 3+strlen(s);

  delete str;
  str = new char[l+1];

  strcpy(str, tmp);
  if (s) {
    strcat(str, "=\"");
    strcat(str, s);
    strcat(str, "\"");
  }

  return str;
}

  /////////////////////////////////////////////////////////////////////////

  string Simple::ShowStringStr(const char *s) {
    char tmp[1000];
    sprintf(tmp, "0x%p", (void*)s);
  
    string ret = tmp;
    if (s) 
      ret += string("=\"")+s+"\"";

    return ret;
  }

///////////////////////////////////////////////////////////////////////////

const char *Simple::ShowString(const std::string& s) {
  static char *str = NULL;
  delete str;
  str = new char[s.length()+3];
  sprintf(str, "\"%s\"", s.c_str());
  return str;
}

///////////////////////////////////////////////////////////////////////////

string Simple::MakeMatlabCompliant(const string& s) {
  string r(s);
  const char avoid[] = "-+[].:,%$()#{}=";
  for (size_t a=0; a<strlen(avoid); a++) {
    char tmp[2] = "x";
    *tmp = avoid[a];
    for (string::size_type i = 0; ; i++) {
      i = r.find(tmp, i);
      if (i!=string::npos)
	r.replace(i, 1, "_");
      else
	break;
    }
  }
  while (r[0]=='_')
    r.erase(0, 1);

  return r;
}

///////////////////////////////////////////////////////////////////////////

void Simple::DumpSimple(DumpMode, ostream& os) {
  os << Bold("Simple ")
     << " use_linpack="       	  << use_linpack
     << " do_abort="          	  << do_abort
     << " trap_after_error="  	  << trap_after_error
     << " jam_after_error="  	  << jam_after_error
     << " backtrace_before_trap=" << backtrace_before_trap
     << " locate_file_debug=" 	  << locate_file_debug
     << " dbmalloc_id="       	  << dbmalloc_id
     << " dbmalloc_size="     	  << dbmalloc_size
     << endl;
}

///////////////////////////////////////////////////////////////////////////

bool Simple::PthreadIdentity(char *buf, pthread_t t) {
  *buf = 0;

#if defined(sgi)
  if ((int)t!=-1) {
    sprintf(buf, "0x%lx", (long)t);
    return true;
  }

#elif defined(__alpha)
  if (t) {
    sprintf(buf, "%ld", (long)t->__sequence);
    return true;
  }

#elif defined(__linux__)
  if (t) {
    sprintf(buf, "%ld", (long)t);
    // sprintf(buf, "th#%d", (int)syscall(SYS_gettid));
    return true;
  }
#endif

  strcpy(buf, "?");

  return false;
}

///////////////////////////////////////////////////////////////////////////

bool Simple::GetMemoryUsage(int& pagesize, int& size, int& resident,
			    int& share, int& trs, int& drs, int& lrs, int& dt){
#ifndef __MINGW32__
  pagesize = getpagesize();
#endif // __MINGW32__

#ifdef __linux__
  ifstream is("/proc/self/statm");              // man 5 proc
  is >> size >> resident >> share >> trs >> drs >> lrs >> dt;
  return is.good();
#endif // __linux__

#if defined(sgi) || defined(__alpha)
  char pfile[256];

#ifdef sgi
  sprintf(pfile, "/proc/pinfo/%05d", getpid()); // man 4 proc
#else // __alpha
  sprintf(pfile, "/proc/%05d", getpid());       // man 4 proc
#endif // sgi

  int fd = open(pfile, O_RDONLY);
  if (fd<0) {
    cerr << "GetMemoryUsage(): open(" << pfile << ") failed: "
	 << strerror(errno) << endl;
    return false;
  }

  prpsinfo prps;
  if (ioctl(fd, PIOCPSINFO, &prps)<0) {
    cerr << "GetMemoryUsage(): ioctl(" << pfile << ") failed: "
	 << strerror(errno) << endl;
    close(fd);
    return 0;
  }
  close(fd);

  size     = prps.pr_size;
  resident = prps.pr_rssize;
  share = trs = drs = lrs = dt = 0;

  return true;
#endif // sgi || __alpha

#ifdef __APPLE__
    task_t targetTask = mach_task_self();
    struct task_basic_info ti;
    mach_msg_type_number_t count = TASK_BASIC_INFO_64_COUNT;
    
    kern_return_t kr = task_info(targetTask, TASK_BASIC_INFO_64,
				 (task_info_t) &ti, &count);
    if (kr != KERN_SUCCESS) {
      cerr << "GetMemoryUsage(): kernel returned an error" << endl;
      return false;
    }
    
    // On Mac OS X, the resident_size is in bytes, not pages!
    // (This differs from the GNU Mach kernel)
    resident = ti.resident_size / pagesize;

    return true;
#endif // __APPLE__

#if !defined(__linux__) && !defined(sgi) && !defined(__alpha) && !defined(__APPLE__)
    return false;
#endif // !__linux__ && !sgi && !__alpha && !__APPLE__
}

///////////////////////////////////////////////////////////////////////////

void Simple::DumpMemoryUsage(ostream& os, bool with_diff) {
  int pagesize, size, resident, share, trs, drs, lrs, dt;
  if (!GetMemoryUsage(pagesize, size, resident, share, trs, drs, lrs, dt)) {
    cerr << "DumpMemoryUsage() : GetMemoryUsage() failed" << endl;
    return;
  } 

  char size_mb[100], size_d[100] = "", size_d_mb[100] = "";
  char  rss_mb[100],  rss_d[100] = "",  rss_d_mb[100] = "";
  sprintf(size_mb, "%.1f", (double)size    *pagesize/(1024*1024.0));
  sprintf(rss_mb,  "%.1f", (double)resident*pagesize/(1024*1024.0));

  static int prev_size, prev_resident;
  if (with_diff) {
    int size_diff = size    -prev_size;
    int  rss_diff = resident-prev_resident;
    sprintf(size_d,  " (%+d)", size_diff);
    sprintf( rss_d,  " (%+d)",  rss_diff);
    sprintf(size_d_mb,  " (%+.2f)", (double)size_diff*pagesize/(1024*1024.0));
    sprintf( rss_d_mb,  " (%+.2f)",  (double)rss_diff*pagesize/(1024*1024.0));
  }

  // os << "pagesize=" << pagesize << endl;

  os << "Process size = " << size << size_d
     << " pages / " << size_mb << size_d_mb << " Mbytes   ";

  os << "Resident size = " << resident << rss_d
     << " pages / " << rss_mb << rss_d_mb << " Mbytes ";
    
  os << endl;

  prev_size     = size;
  prev_resident = resident;
}

///////////////////////////////////////////////////////////////////////////

void Simple::MallocOrigin() {
  dbmalloc_size = malloc_inuse(&dbmalloc_id);
}

///////////////////////////////////////////////////////////////////////////

void Simple::MallocList() {
  unsigned long id = 0;
  malloc_inuse(&id);
  malloc_list(2, dbmalloc_id, id);
}

///////////////////////////////////////////////////////////////////////////

int Simple::MallocDiffers() {
  unsigned long size = malloc_inuse(NULL);

  return size!=dbmalloc_size;
}

///////////////////////////////////////////////////////////////////////////

int Simple::MallocCheck(bool core) {
  cout << "MallocCheck() starting." << endl;
  int r = malloc_chain_check(core);
  cout << "MallocCheck() ready." << endl;
  return r;
}

///////////////////////////////////////////////////////////////////////////

int Simple::MallocCKDATA(bool onoff) {
  dbmalloptarg m;
  m.i = onoff;
  return dbmallopt(MALLOC_CKDATA, &m);
}

///////////////////////////////////////////////////////////////////////////

const char *Simple::LocateFile(const char *name) {
  char *tmp = NULL;
  const char *env = getenv(DATA_PATH_ENVVAR), *def = DATA_PATH_DEFAULT, *path;

  if (!env)
    path = def;
  else {
    const char *ptr = NULL;
    if (*env==':')
      ptr = env;
    else if (strstr(env, "::"))
      ptr = strstr(env, "::")+1;
    else if (env[strlen(env)-1]==':')
      ptr = env+strlen(env);

    if (ptr) {
      tmp = new char[strlen(env)+strlen(def)+1];
      strncpy(tmp, env, ptr-env);
      strcat(tmp+(ptr-env), def);
      strcat(tmp, ptr);
      path = tmp;

    } else
      path = env;
  }
  if (locate_file_debug==-1) {
    if (getenv(DATA_PATH_DEBUG_ENVVAR))
      locate_file_debug = atoi(getenv(DATA_PATH_DEBUG_ENVVAR));
    else
      locate_file_debug = FALSE;
  }

  if (locate_file_debug)
    cout << "Simple::LocateFile path=" << path << endl;

  if (name && *name!='/')
    for (const char *dir = path; *dir; dir++) {
      char file[MAXPATHLEN];
      const char *col = strchr(dir, ':');
      if (col) {
	strncpy(file, dir, col-dir);
	file[col-dir] = 0;
      } else
	strcpy(file, dir);
      
      if (file[strlen(file)-1]!='/')
	strcat(file, "/");
      
      if (*file=='~') {
	char user[MAXPATHLEN];
	strcpy(user, file+1);
	*strchr(user, '/') = 0;
	
	struct passwd *pwd = user[0] ? getpwnam(user) : getpwuid(getuid());
	if (!pwd) {
	  cerr << "Simple::LocateFile unresolved ~" << user << endl;
	  *file = 0;
	  
	} else {
	  memmove(file+strlen(pwd->pw_dir), file+1+strlen(user),
		  strlen(file)-strlen(user));
	  memcpy(file, pwd->pw_dir, strlen(pwd->pw_dir));
	}
      }

      if (*file) {
	const char *ret = LocateFileRecursion(file, name);
	if (ret) {
	  name = ret;
	  break;
	}
      }

      dir = col;
      if (!dir)
	break;
      if (*(dir+1)==':') {
	cerr << "Simple::LocateFile unresolved :: in " << path << endl;
	break;
      }
    }
  
  delete tmp;

  if (locate_file_debug)
    cout << "Simple::LocateFile returns " << name << endl;

  return name;
}

///////////////////////////////////////////////////////////////////////////

const char *Simple::LocateFileRecursion(const char *dir, const char *name) {
  const char *ret = NULL;

  static char file[MAXPATHLEN];
  strcpy(file, dir);
  strcat(file, name);

  if (locate_file_debug>1)
    cout << "Simple::LocateFileRecursion stat()ing " << file << endl;

  struct stat st;
  if (!stat(file, &st) && S_ISREG(st.st_mode))
    ret = file;

  if (!ret && strlen(dir)>1 && file[strlen(dir)-2]=='/') {
#ifdef HAVE_DIRENT_H
    struct dirent *direntp;
    DIR *dirp = opendir(dir);
    if (!dirp) {
      
//       ShowError("Simple::LocateFileRecursion(", dir, ", ", name,
// 		") no such directory");
    } else {
      while (!ret && (direntp = readdir(dirp))) {
	if (!strcmp(direntp->d_name, ".") || !strcmp(direntp->d_name, ".."))
	continue;
	
	strcpy(file, dir);
	strcat(file, direntp->d_name);
	
	if (locate_file_debug>1)
	  cout << "Simple::LocateFileRecursion stat()ing " << file << endl;
      
	if (stat(file, &st)) {
	  cerr << "Simple::LocateFileRecursion stat failed: " << file << endl;
	  break;
	}

	if (st.st_mode&S_IFDIR) {
	  if (locate_file_debug>1)
	    cout << "Simple::LocateFileRecursion descending " << file << endl;
	  char copy[MAXPATHLEN];
	  strcpy(copy, file);
	  strcat(copy, "//");
	
	  ret = LocateFileRecursion(copy, name);
	}
      }
      closedir(dirp);
    }
#endif // HAVE_DIRENT_H
  }

  if (locate_file_debug>1)
    cout << "Simple::LocateFileRecursion returning " << ret << endl;

  return ret;
}

///////////////////////////////////////////////////////////////////////////

bool Simple::ShowError(const char *t1, const char *t2,const char *t3,
		       const char *t4, const char *t5,const char *t6) {
#ifdef cray
  const char *t[6];
  t[0]= t1; t[1] = t2; t[2] = t3; t[3] = t4; t[4] = t5; t[5] = t6;
#else
  const char *t[6] = { t1, t2, t3, t4, t5, t6 };
#endif // cray

  string threadstr;
#ifndef NO_PTHREADS
  stringstream tss;
  tss << (int)syscall(SYS_gettid);
  threadstr = tss.str();
#endif // !NO_PTHREADS

  int j = 1; // isatty(os.something);
  const char *strike_out = j?"\033[2m":"", *normal = j?"\033[0m":"";

  cerr << endl << endl << strike_out << TimeStampP()
       << "[" << threadstr << "] "
       << (error_extra_text!=""?error_extra_text+" ":"") << "ERROR: ";

  for (int i=0; i<6; i++)
    if (t[i])
      cerr << t[i];

  cerr << normal << endl << endl;

  while (jam_after_error)
    sleep(10);

  if (trap_after_error)
    TrapMeHere();

  return false;
}

///////////////////////////////////////////////////////////////////////////

void Simple::BackTraceMeHere(ostream& os) {
#ifdef __linux__
  const bool use_malloc = true, do_demangle = true;
  const int maxn = 1000;
  void *array[maxn];
  int n = backtrace(array, maxn);

  os << endl << TimeStampP() << " *** stack backtrace begins ***" << endl;
  
  if (!use_malloc) 
    backtrace_symbols_fd(array, n, 1); // if malloc() doesn't work...

  else {
    char **name = backtrace_symbols(array, n);
    for (int i=0; i<n-1; i++) {
      char *p = name[i];
      char tmp[10000], buf[10000];
      char *pa = strchr(name[i], '(');
      if (do_demangle && pa && pa[1]=='_') {
	strcpy(tmp, pa+1);
	pa = strchr(tmp, '+');
	if (pa) {
	  *pa = 0;
	  size_t s = sizeof buf;
	  int st = 0;
	  __cxxabiv1::__cxa_demangle(tmp, buf, &s, &st);
	  if (!st) {
	    sprintf(buf+strlen(buf), "+%s", pa+1);
	    pa = strrchr(buf, ')');
	    if (pa) {
	      *pa = 0;
	      p = buf;
	    }
	  }
	}
      }
      os << i << ": " << p << endl;
    }
    delete name;
  }
  os << TimeStampP() << " *** stack backtrace ends ***" << endl << endl;
#else
  os << TimeStampP() << " *** stack backtrace not available ***" <<endl<<endl;
#endif // __linux__
}

///////////////////////////////////////////////////////////////////////////

const char *Simple::Bold(const char *txt) {
  return txt;

  static char *str = NULL;
  delete str;
  
  int j = 1; // isatty(os.something);
  const char *bold = j?"\033[1m":"", *normal = j?"\033[0m":"";

  str = new char[strlen(bold)+strlen(normal)+strlen(txt)+1];
  strcpy(str, bold);
  strcat(str, txt);
  strcat(str, normal);

  return str;
}

///////////////////////////////////////////////////////////////////////////

bool Simple::Exists(const char *name, bool locate) {
  if (locate)
    name = LocateFile(name);

  struct stat sbuf;
  return (stat(name, &sbuf)==0);
}

///////////////////////////////////////////////////////////////////////////

bool Simple::FileExists(const char *name, bool locate) {
  if (locate)
    name = LocateFile(name);

  struct stat sbuf;
  if (stat(name, &sbuf)==0) {
    // cout << "sbuf.st_mode=" << (int)sbuf.st_mode << endl;
    if (sbuf.st_mode&S_IFREG)
      return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////

bool Simple::DirectoryExists(const char *name, bool locate) {
  if (locate)
    name = LocateFile(name);

  struct stat sbuf;
  if (stat(name, &sbuf)==0) {
    if (sbuf.st_mode&S_IFDIR)
      return true;
  }
  
  return false;
}

///////////////////////////////////////////////////////////////////////////

const char *Simple::UniqueFileName(const char *name, bool locate) {
  if (locate)
    name = LocateFile(name);

  if (!FileExists(name))
    return CopyString(name);

  const char *tmp = NULL;
  for (int i=0; i<1000; i++) {
    char suffix[100];
    sprintf(suffix, ".%d", i);
    CopyString(tmp, name);
    AppendString(tmp, suffix);
    if (!FileExists(tmp))
      return tmp;
  }
  ShowError("Simple::UniqueFileName(", name, ") failed");
  CopyString(tmp, "/dev/null");
  
  return tmp;
}

///////////////////////////////////////////////////////////////////////////
/// THE FOLLOWING METHODS USE Standard Template Library (STL)
///////////////////////////////////////////////////////////////////////////

list<string> Simple::SortedDir(const string& path,
			       const string& /*pattern*/,
			       bool only_files,
			       bool only_dirs) {
  list<string> list;
  bool debug = false;

#ifdef HAVE_DIRENT_H
  DIR *dv = opendir(path.c_str());  
  for (dirent *dpv; dv && (dpv = readdir(dv)); )
    if (strcmp(dpv->d_name, ".") && strcmp(dpv->d_name, "..")) {
      string name = path;
      if (*name.rbegin()!='/')
	name += '/';
      name += dpv->d_name;

      if (only_files || only_dirs) {
	struct stat st;
	if (stat(name.c_str(), &st))
	  continue;
	
	if (only_files && !(st.st_mode&S_IFREG))
	  continue;

	if (only_dirs && !(st.st_mode&S_IFDIR))
	  continue;
      }

      list.push_back(dpv->d_name);

      if (debug)
	cout << list.size() << " " << list.back() << endl;
    }

  if (dv)
    closedir(dv);
#endif // HAVE_DIRENT_H
  
  list.sort();

  return list;
}

} // namespace simple

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

