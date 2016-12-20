// -*- C++ -*-  $Id: Util.C,v 2.64 2016/12/05 17:43:14 jorma Exp $
// 
// Copyright 1998-2016 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <Util.h>
#include <TimeUtil.h>

#include <stdlib.h>

#include <cmath>
#include <limits>

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

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif // MAXPATHLEN

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string Util_C_vcid =
  "@(#)$Id: Util.C,v 2.64 2016/12/05 17:43:14 jorma Exp $";

  static bool do_abort = false;
  static int locate_file_debug = 0;
  
  bool trap_after_error      = false;
  bool backtrace_before_trap = false;

  /////////////////////////////////////////////////////////////////////////////

  string RegExp::error(int code) const {
    if (code==-1)
      code = errcode;
    if (code==0) 
      return "Success!";

    if (!rep)
      return "No regexp";

    char buf[100];
    regerror(code, rep, buf, sizeof buf);

    return buf;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool RegExp::match(const string& str) {
    if (!rep)
      return false;

    int ret = regexec(rep, str.c_str(), 0, NULL, 0);
    if (ret==0)
      return true;
    if (ret!=REG_NOMATCH)
      errcode = ret;

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<RegExpMatch> RegExp::match(const string& str, const int subs) {
    const bool debug = false;
    stringstream msg;
    msg << "RegExp::match(" << str << ", " << subs << ") " << endl;

    vector<RegExpMatch> v;
    if (!ok())
      return v;
    
    regmatch_t* pm = new regmatch_t[subs+1];
    
    int ret = regexec(rep, str.c_str(), (size_t) subs+1, pm, 0);
    
    if (ret == 0)
      for (int i=1; i<subs+1; i++) {
	ssize_t start = pm[i].rm_so;
	ssize_t length =  pm[i].rm_eo-pm[i].rm_so;
	
	string res = "";
	if (start != -1 && length != -1)
	  res = str.substr(start, length);
	v.push_back(RegExpMatch(res,start,length));

	if (debug)
	  cout << msg.str() << "Found " << res << ", start=" << start
	       << " len=" << length << endl;

      }
    else if (ret != REG_NOMATCH) errcode = ret;

    if (ret != 0 && debug)
      cout << msg.str() <<  "Error " << ret << " (" << error(ret) << ")"
	   << endl;

    delete [] pm;

    return v;
  }

  ///////////////////////////////////////////////////////////////////////////

  void ProgressText(const char *t1, const char *t2,const char *t3,
                            const char *t4, const char *t5,const char *t6) {
#ifdef cray
    const char *t[6];
    t[0]= t1; t[1] = t2; t[2] = t3; t[3] = t4; t[4] = t5; t[5] = t6;
#else
    const char *t[6] = { t1, t2, t3, t4, t5, t6 };
#endif // cray

    size_t l = 0;
    for (int i=0; i<6; i++)
      if (t[i]) {
        l += strlen(t[i]);
        cout << t[i];
      }

    while (l--)
      cout << '\b';

    cout << flush;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool HasTerminalOutput() {
    return isatty(STDOUT_FILENO);
  }

  /////////////////////////////////////////////////////////////////////////////

  ProgressBar::ProgressBar(int max, int w, const string& msg) {
    disabled = !HasTerminalOutput();
    pmax = max;
    width = w;
    pstr = string(width+2, ' ');
    pstr[0]='['; pstr[width+1]=']';
    cc=0;
    oldp=0;
    defmsg = msg;
  }
    
  /////////////////////////////////////////////////////////////////////////////

  void ProgressBar::Update(int p, const string& msg) {
    if (disabled)
      return;

    static string chr = "-\\|/";
    string m = msg.empty() ? defmsg : msg;
    
    int k = ((width*p)/pmax) % width;
    for (int i=1; i<k+1; i++)
      pstr[i]='=';
    
    if (pmax < 10000 || p%10==0)
      cc = (cc+1)%4;
    pstr[k+1]=chr[cc];
    
    ProgressText(m.c_str(), pstr.c_str());
    
    oldp=p;
  }

  /////////////////////////////////////////////////////////////////////////////

  string JoinWithString(const vector<string>& a, const string& join,
			bool force) {
    string s;
    for (size_t i=0; i<a.size(); i++)
      s += (force||i ? join : "")+a[i];
    return s;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> BraceCommaExpand(const string& s) {
    string msg = "BraceCommaExpand("+s+") : ";

    size_t p = s.find('{');
    if (p==string::npos) 
      return list<string> { s };

    list<string> r;
    size_t l = 0, q = p+1;
    bool found = false;
    vector<size_t> a { p };
    while (q<s.length() && !found) {
      if (s[q]=='{')
	l++;
      if (s[q]=='}') {
	if (l==0) {
	  found = true;
	  a.push_back(q);
	} else
	  l--;
      }
      if (s[q]==',' && l==0)
	a.push_back(q);
      q++;
    }
    if (!found) {
      ShowError(msg+"failed A");
      return r;
    }
    for (size_t i=0; i<a.size()-1; i++) {
      string t = s.substr(0, p)+s.substr(a[i]+1, a[i+1]-a[i]-1)+
	s.substr(q);
      list<string> b = BraceCommaExpand(t);
      r.insert(r.end(), b.begin(), b.end());
    }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  string JoinWithString(const list<string>& a, const string& join, bool force) {
    string s;
    for (list<string>::const_iterator i=a.begin(); i!=a.end(); i++)
      s += (force||i!=a.begin() ? join : "")+*i;
    return s;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  vector<string> SplitInSomething(const char *sep, bool multi,
                                  const string& line) {
    vector<string> ret;

    if (line=="")
      return ret;

    const char *ptr = line.c_str();

    for (;;) {
      const char *nptr = strpbrk(ptr, sep);
      if (nptr==NULL)
        break;

      ret.push_back(string(ptr, nptr-ptr));

      ptr = multi ? nptr+strspn(nptr, sep) : nptr+1;
    }

    ret.push_back(ptr);

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<string> SplitOnWord(const string& sep, const string& line) {
    vector<string> ret;
    
    if (line.empty())
      return ret;
  
    size_t pt = 0;
    while (true) {
      size_t npt = line.find(sep, pt);
    
      string word = line.substr(pt, npt==string::npos ? string::npos : npt-pt);
      ret.push_back(word);

      if (npt == string::npos)
        return ret;

      pt = npt+sep.size();
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<string> SplitInCommasObeyParentheses(const string& str) {
    bool debug = false;

    vector<string> v;

    for (size_t b = 0; ; ) {
      size_t comma = 0;
      size_t pos = b;
      int pcount = 0;

      // Find next comma that is not within parentheses
      while (1) {
        pos = str.find_first_of("(),", pos);

        // End of string, comma is set to last char+1
        if (pos == string::npos) {
          comma = str.size();
          break;
        }

        const char& ch = str[pos];

        // Break if it is a comma and we are not within parentheses
        if (ch == ',' && pcount==0) {
          comma = pos;
          break;
        }
        
        // Count parentheses
        if (ch == '(') pcount++;
        if (ch == ')') pcount--;

        pos++;
      }
        
      string c = str.substr(b, comma-b);

      if (debug)
        cout << "<" << c << ">" << endl;

      v.push_back(c);
      
      if (comma==str.size())
	break;
      
      b = comma+1;
    }

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SplitParentheses(const string& in, string& f, string& a, 
			bool allow_empty_f) {
    bool debug=false;
    if (in=="" || in.substr(in.length()-1)!=")")
      return false;

    if (debug)
      cout << "SplitParentheses(): in="<< in << endl;
    int pcount=0;
    size_t i = in.length();
    do {
      i = in.find_last_of("()", i-1);
      if (i!=string::npos) {
	if (in[i] == ')') pcount++;
	if (in[i] == '(') pcount--;
      }
      if (debug)
	cout << "SplitParentheses(): i=" << i << ", pcount=" << pcount << endl;
    } while (i!=string::npos && pcount);

    if (debug)
      cout << "SplitParentheses(): end of do-while" << endl;

    if (i==string::npos || (i==0&&!allow_empty_f) || i>in.length()-3)
      return false;

    f = in.substr(0, i);
    a = in.substr(i+1, in.length()-i-2);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  /* Copied from Simple.h [start] */

  const char *Bold(const char *txt) {
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

  /////////////////////////////////////////////////////////////////////////////

  bool SplitURL(const string& urlin, string& proto, string& host, string& port,
                string& path) {
    proto = host = port = path = "";

    string url = urlin;

    size_t h = url.find("://");
    if (h!=string::npos) {
      proto = url.substr(0, h);
      url.erase(0, h+3);
    }

    size_t s = url.find('/');
    if (s!=string::npos)
      path = url.substr(s);

    size_t c = url.find(':');
    if (c<s) {
      host = url.substr(0, c);
      port = url.substr(c+1, s-c-1);

    } else
      host = url.substr(0, s);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool FileNameExtension(const string& in, const string& ext, string& out) {
    size_t dot = in.find(ext);
    if (dot==string::npos)
      return false;
    
    string s = in.substr(dot+ext.size());
    if (s!="" && s!=".gz")
      return false;

    out = in.substr(0, dot);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *TimeStampP() {
    timespec_t tt;
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

  /////////////////////////////////////////////////////////////////////////////

  void BackTraceMeHere(ostream& os) {
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
      free(name);
    }
    os << TimeStampP() << " *** stack backtrace ends ***" << endl << endl;
#else
    os << TimeStampP() << " *** stack backtrace not available ***" <<endl<<endl;
#endif // __linux__
  }

  /////////////////////////////////////////////////////////////////////////////

  void ConditionalAbort(bool b) { 
    do_abort = b; 
  }

  /////////////////////////////////////////////////////////////////////////////

  bool ConditionalAbort(const char *f, int l) {
    cerr << endl << Bold("ConditionalAbort() hit ") << f << " " << l
	 << endl << endl;
    
    if (backtrace_before_trap)
      BackTraceMeHere(cerr);

    if (do_abort)
      abort();
    return false;
  }
 
  /////////////////////////////////////////////////////////////////////////////

  void LocateFileDebug(int d) { 
    locate_file_debug = d; 
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *LocateFileRecursion(const char *dir, const char *name) {
#ifdef HAVE_DIRENT_H
    const char *ret = NULL;

    static char file[MAXPATHLEN];
    strcpy(file, dir);
    strcat(file, name);

    if (locate_file_debug>1)
      cout << "LocateFileRecursion stat()ing " << file << endl;

    struct stat st;
    if (!stat(file, &st) && S_ISREG(st.st_mode))
      ret = file;

    if (!ret && strlen(dir)>1 && file[strlen(dir)-2]=='/') {
      struct dirent *direntp;
      DIR *dirp = opendir(dir);
      if (!dirp) {
      
	//       ShowError("LocateFileRecursion(", dir, ", ", name,
	// 		") no such directory");
      } else {
	while (!ret && (direntp = readdir(dirp))) {
	  if (!strcmp(direntp->d_name, ".") || !strcmp(direntp->d_name, ".."))
	    continue;
	
	  strcpy(file, dir);
	  strcat(file, direntp->d_name);
	
	  if (locate_file_debug>1)
	    cout << "LocateFileRecursion stat()ing " << file << endl;
      
	  if (stat(file, &st)) {
	    cerr << "LocateFileRecursion stat failed: " << file << endl;
	    break;
	  }

	  if (st.st_mode&S_IFDIR) {
	    if (locate_file_debug>1)
	      cout << "LocateFileRecursion descending " << file << endl;
	    char copy[MAXPATHLEN];
	    strcpy(copy, file);
	    strcat(copy, "//");
	
	    ret = LocateFileRecursion(copy, name);
	  }
	}
	closedir(dirp);
      }
    }

    if (locate_file_debug>1)
      cout << "LocateFileRecursion returning " << ret << endl;

    return ret;
#else
    return NULL;
#endif // HAVE_DIRENT_H
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *LocateFile(const char *name) {
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
      cout << "LocateFile path=" << path << endl;

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
	    cerr << "LocateFile unresolved ~" << user << endl;
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
	  cerr << "LocateFile unresolved :: in " << path << endl;
	  break;
	}
      }
  
    delete [] tmp;

    if (locate_file_debug)
      cout << "LocateFile returns " << name << endl;

    return name;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool FileExists(const char *name, bool locate) {
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

  /* Copied from Simple.h [end] */

  /////////////////////////////////////////////////////////////////////////////

  bool DirectoryExists(const string& name) {
    struct stat sbuf;
    if (stat(name.c_str(), &sbuf)==0 && sbuf.st_mode&S_IFDIR)
      return true;
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool IsAffirmative(const char *s) {
    if (!s || !*s) return false;
    if (strlen(s)==1 && strchr("YyTt1", *s)) return true;
    if (!strcasecmp(s, "yes") || !strcasecmp(s, "true") ||
	!strcasecmp(s, "on")) return true;
    return false; 
  }

  /////////////////////////////////////////////////////////////////////////////

  bool WarnOnce(const string& f) {
    static set<string> done;
    if (done.find(f)==done.end()) {
      ShowError(f);
      done.insert(f);
      return true;
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Obsoleted(const string& f, bool trap) {
    if (WarnOnce(string("Function ")+f+" called though OBSOLETED!")) {
      BackTraceMeHere(cerr);
      if (trap) {
	BackTraceBeforeTrap(false);
	TrapMeHere();
      }
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  void StripWhiteSpaces(string& s) {
    // now as 11.6.2004 contains also \n !!!
    const char *white = " \t\n";
    s.erase(0, s.find_first_not_of(white));
    size_t p = s.find_last_not_of(white);
    if (p!=string::npos)
      s.erase(p+1);
  }

  /////////////////////////////////////////////////////////////////////////////

  void EscapeWhiteSpace(string& s) {
    for (size_t p; (p = s.find(' '))!=string::npos; )
      s.replace(p, 1, "%20");
    for (size_t p; (p = s.find('\t'))!=string::npos; )
      s.replace(p, 1, "%9");
  }

  /////////////////////////////////////////////////////////////////////////////

  void StripLastWord(string& s, char sep) {
    size_t sep_pos = s.rfind(sep);
    if (sep_pos != string::npos)
      s.erase(sep_pos);
  }

  /////////////////////////////////////////////////////////////////////////////
  
  string BaseName(const string& fn) {
    string ret = fn;
    size_t sep_pos = ret.rfind('/');
    if (sep_pos != string::npos)
      ret.erase(0, sep_pos+1);
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SplitKeyEqualValue(const string& s, string& ks, string& vs) {
    const char *k = NULL, *v = NULL;
    bool ok = SplitKeyEqualValueOld(s.c_str(), k, v);
    if (ok) {
      ks = k;
      vs = v;
    }
    delete [] k;
    delete [] v;
    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> SplitKeyEqualValueOld(const string& s) {
    string ks, vs;
    SplitKeyEqualValue(s, ks, vs);
    return pair<string,string>(ks, vs);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool SplitKeyEqualValueOld(const char *line,
			     const char* &key, const char* &value) {
    key = value = NULL;

    //  char *ptr = strchr(line, '=');

    int nestedvalue=0;
    const char *ptr = line;
    while ((ptr = strpbrk(ptr, "=()"))) {
      switch (*ptr) {
      case '(':
	{ nestedvalue++; break; }
      case ')':
	{ nestedvalue--; break; }
      case '=':
	{ if (!nestedvalue) goto ready; }      
      }
      ptr++;
    }
  
  ready:
    if (!ptr)
      return false;

    char *mykey = (char*)CopyString(line);
    mykey[ptr-line] = 0;
    while (strlen(mykey) && isspace(mykey[strlen(mykey)-1]))
      mykey[strlen(mykey)-1] = 0;

    if (strpbrk(mykey, " \t")) {
      delete mykey;
      return false;
    }

    while (isspace(*++ptr)) {}

    key = mykey;
    value = CopyString(ptr);

    if (value) {
      for (char *ptr2=(char*)value+strlen(value)-1; ptr2>=value; ptr2--)
	if (*ptr2!=' ' && *ptr2!='\t')
	  break;
	else
	  *ptr2 = 0;
    }
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> SplitKeyEqualValueNew(const string& s)
    throw(logic_error) {
    const string msg = "equal sign not found in ["+s+"] ";
    size_t p = 0, nested = 0;
    if (s[0]=='(' && s[s.length()-1]==')') {
      string new_s = s.substr(1,s.length()-2);
      return SplitKeyEqualValueNew(new_s);
    }
    while (true) {
      p = s.find_first_of("=()", p);
      if (p==string::npos)
	throw logic_error(msg+"A");
      if (s[p]=='=' && !nested)
	break;
      if (s[p]=='(')
	nested++;
      if (s[p]==')') {
	if (nested<1)
	  throw logic_error(msg+"B");
	nested--;
      }
      p++;
    }
    string k = s.substr(0, p), v = s.substr(p+1);
    StripWhiteSpaces(k);
    StripWhiteSpaces(v);
    return pair<string,string>(k, v);
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *CopyString(const char*& d, const char *s) {
    char*& dd = *(char**)&d;
    return CopyString(dd, s);
  }
  
  /////////////////////////////////////////////////////////////////////////////

  const char *CopyString(char*& d, const char *s) {
    if (s!=d) {
      if (s==NULL || d==NULL || strlen(s)!=strlen(d))
	return CopyStringOldVersion(d, s);
      else
	strcpy(d, s);
    }
    return d; 
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *CopyStringOldVersion(char*& d, const char *s) {
    char *old = d;
    d = (char*)CopyString(s);
    delete old;
    return d; 
  }

  /////////////////////////////////////////////////////////////////////////////

  bool RmDirRecursive(const string& p) {
    string cmd = "/bin/rm -rf "+p+" 1>/dev/null 2>&1";
    int r = system(cmd.c_str());
    return r==0;
  }

  /////////////////////////////////////////////////////////////////////////////

  string FullPath(const string& p) {
    static map<string,string> replace;
    if (replace.empty()) {
      replace["/m/fs/project0/imagedb/"] = "/share/imagedb/";
      replace["/m/fs/project/imagedb/"]  = "/share/imagedb/";
      replace["/m/fs/home/"]             = "/home/";
    }

    if (p.substr(0, 1)=="/")
      return p;

    char tmp[1024];
    string ret = getcwd(tmp, sizeof tmp);
    if (ret.substr(ret.size()-1, 1)!="/")
      ret += "/";
    ret += p;

    for (map<string,string>::const_iterator i = replace.begin();
	 i!=replace.end(); i++)
      if (ret.substr(0, i->first.size())==i->first)
	return i->second+ret.substr(i->first.size());

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *ExtensionVersusMIMEP(const string& ttt, bool to_mime) {
    static struct type_extensions {
      const char *mime_type, *ext_lc, *ext_uc;
    } table[] = {
      { "image/tiff",               ".tiff",".TIFF"},
      { "image/tiff",               ".tif", ".TIF" },
      { "image/gif",                ".gif", ".GIF" },
      { "image/jpeg",               ".jpeg", ".JPEG" },
      { "image/jpeg",               ".jpg", ".JPG" },
      { "image/jpg",                ".jpg", ".JPG" },  // this should be removed...
      { "image/png",                ".png", ".PNG" },
      { "image/x-ms-bmp",           ".bmp", ".BMP" }, // should this be removed?
      { "image/bmp",                ".bmp", ".BMP" },
      { "image/x-portable-bitmap",  ".pbm", ".PBM" },
      { "image/x-portable-greymap", ".pgm", ".PGM" },
      { "image/x-portable-pixmap",  ".ppm", ".PPM" },
      { "video/mpeg",               ".mpg", ".MPG" },
      { "video/mpeg",               ".m2v", ".m2v" },
      { "video/mpeg",               ".m1v", ".m1v" },
      { "video/mpeg",               ".mpeg",".MPEG"},
      { "video/mp4",                ".mp4", ".MP4" },
      { "video/mp2t",               ".mp2t",".mp2t"},
      { "video/quicktime",          ".mov", ".MOV" },
      { "video/x-msvideo",          ".avi", ".AVI" },
      { "video/webm",               ".webm", ".WEBM" },
      { "video/x-matroska",         ".mkv",  ".mkv" },
      { "video/x-flv",              ".flv",  ".flv" },
      { "audio/wav",                ".wav", ".WAV" },
      { "message/rfc822",           ".eml", ".EML" },
      { "text/plain",               ".txt", ".TXT" },
      { "text/html",                ".html",".HTML"},
      { "text/html",                ".htm", ".HTM" },
      { "application/json",         ".json",".JSON"},
      { "application/x-cogain",     ".cogain",".COGAIN"},
      { NULL, NULL, NULL }
    };

    if (ttt.empty())
      return NULL;
    
    const char *t = ttt.c_str();
    
    if (to_mime) {
      size_t dot = ttt.rfind('.');
      if (dot==string::npos)
	return NULL;
      t += dot;
    }
    
    for (const type_extensions *p = table; p->mime_type; p++)
      if (!to_mime && !strcmp(t, p->mime_type))
	return p->ext_lc;
      else if (to_mime && (!strcmp(t, p->ext_lc) || !strcmp(t, p->ext_uc)))
	return p->mime_type;
    
    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool ReadNumPyHeader(istream& in, size_t& o, size_t& fs,
		       size_t& r, size_t& c) {
    map<string,string> hdrdict = ReadNumPyHeader(in, o);
    fs = r = c = 0;
    string dtype = hdrdict["descr"], fortran_order = hdrdict["fortran_order"];
    if (dtype!="'<f2'")
      return false;
    string s = hdrdict["shape"];
    size_t p = s.find(',');
    if (s.size() && s[0]=='(' && s[s.size()-1]==')' && p!=string::npos) {
      r = atoi(s.substr(1).c_str());
      c = atoi(s.substr(p+1).c_str());
      fs = 16;
    }
    // cout << r << " " << c << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  // https://docs.scipy.org/doc/numpy/neps/npy-format.html

  map<string,string> ReadNumPyHeader(istream& in, size_t& o) {
    bool debug = false;
    
    struct numpy_header {
      char magic[6];
      unsigned char major, minor;
      uint16_t header_len;
    };
    numpy_header hdr;

    in.read((char*)&hdr, sizeof(hdr));
    string hdrstr(hdr.header_len, '\0');
    in.read((char*)hdrstr.c_str(), hdrstr.size());
    if (debug)
      cout << string(hdr.magic+1, 5) << " " << (int)hdr.major << "."
	   << (int)hdr.minor << " " << hdr.header_len << " [" << hdrstr << "]"
	   << endl;
    
    map<string,string> hdrdict;
    if (string(hdr.magic+1, 5)!="NUMPY" ||
	(int)hdr.major!=1 || (int)hdr.minor!=0)
      return hdrdict;

    o = sizeof(hdr)+hdr.header_len;

    for (size_t i=0; i<hdrstr.size();) {
      if (hdrstr[i]=='{' || hdrstr[i]==',' || hdrstr[i]==' ') {
	i++;
	continue;
      }
      if (hdrstr[i]=='\'') {
	string key = hdrstr.substr(i+1);
	size_t p = key.find('\'');
	if (p!=string::npos) {
	  string val = key.substr(p+1);
	  key.erase(p);
	  if (val.find(": ")==0) {
	    val.erase(0, 2);
	    size_t d = 0;
	    for (size_t j=0; j<val.size(); j++) {
	      if (val[j]=='(')
		d++;
	      if (val[j]==')')
		d--;
	      if (d==0 && val[j]==',') {
		p = j;
		break;
	      }
	    }
	    if (p!=0) {
	      val.erase(p);
	      hdrdict[key] = val;
	      if (debug)
		cout << "a [" << key << "]=[" << val << "]" << endl; 
	      i += key.size()+p+4;
	      continue;
	    }
	  }
	}
      }
      // hdrdict.clear();
      break;
    }

    //  for (auto i=hdrdict.begin(); i!=hdrdict.end(); i++)
    //    cout << "b [" << i->first << "]=[" << i->second << "]" << endl;

    return hdrdict;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<float> ReadNumPyVector(istream& in, size_t o, size_t s,
				size_t r, size_t c, size_t idx, bool rowwise) {

    vector<float> v(rowwise?c:r);
    for (size_t i=0; i<v.size(); i++) {
      size_t p = o+s/8*(rowwise?idx*c+i:idx+i*c);
      in.seekg(p);
      char buf[8];
      memset(buf, 0, 8);
      in.read(buf, s/8);
      if (s==16)
	v[i] = FromFloat16(*(uint16_t*)buf);
      else if (s==32)
	v[i] = *(float*)buf;
      else
	v[i] = numeric_limits<float>::quiet_NaN();
    }

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  // ftp://ftp.fox-toolkit.org/pub/fasthalffloatconversion.pdf

  float FromFloat16(uint16_t v) {
    unsigned int sign = v >> 15;
    unsigned int expo = (v >> 10) & 31;
    unsigned int mant = v & 1023;
    float f = 0;
    if (expo>0 && expo<31)
      f = (-2*sign+1)*double(1L<<expo)/double(1L<<15)*(1+mant/1024.0);
    else if (expo==0 && mant)
      f = (-2*sign+1)/double(1L<<14)*(mant/1024.0);
    else if (expo==31)
      f = mant ? numeric_limits<float>::quiet_NaN() :
	numeric_limits<float>::infinity();

    //cout << "{" << sign << " " << expo << " " << mant << "}=" << f << " ";

    return f;
  }

  /////////////////////////////////////////////////////////////////////////////

  void Replace(string& s, char ch_old, const string& ch_new) {
    for (size_t pos = 0;;) {
      size_t p = s.find(ch_old, pos);
      if (p==string::npos)
	break;
      s.replace(p, 1, ch_new);
      pos = p+ch_new.size();
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  string Latin1ToUTF(const string& s) {
    //#ifdef __GNUC__
    const char *tocode   = "UTF-8";
    const char *fromcode = "ISO8859-1";
    //#endif // __GNUC__

    string err = "Latin1ToUTF("+s+") failed: ";

    static iconv_t cd = iconv_open(tocode, fromcode);
    if (cd==iconv_t(-1)) {
      ShowError(err+"cd==-1");
      return s;
    }

    iconv(cd, NULL, NULL, NULL, NULL);

    size_t inbytesleft = s.size();
    size_t outbytesleft = 2*inbytesleft, obl_was = outbytesleft;

    char *ibuf = strdup(s.c_str()), *inbuf  = ibuf;
    char *obuf = new char[obl_was], *outbuf = obuf;

    size_t n = iconv(cd, (ICONV_CONST char**)&inbuf, &inbytesleft,
		     &outbuf, &outbytesleft);

    string r(obuf, obl_was-outbytesleft);

    delete [] obuf;
    free(ibuf);
  
    if (n!=0) {
      ShowError(err+"returned "+ToStr(n));
      return s;
    }

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool GetMemoryUsage(int& pagesize, int& size, int& resident,
		      int& share, int& trs, int& drs, int& lrs,
		      int& dt) {
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

  /////////////////////////////////////////////////////////////////////////////

  void DumpMemoryUsage(ostream& os, bool with_diff) {
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

  /////////////////////////////////////////////////////////////////////////////

  /// Solves the mime type of given file using libmagic
  pair<string,string> libmagic_mime_type(const string& fname) {
#ifdef HAVE_MAGIC_H
    const bool debug = false;
    const char* mime = NULL;
    magic_t m = magic_open( MAGIC_SYMLINK | MAGIC_MIME | MAGIC_COMPRESS );
    if (m && !magic_load(m, NULL))
      mime = magic_file(m, fname.c_str());

    string mt = mime ? mime : "";

    if (m)
      magic_close(m);

    if (debug)
      cout << "libmagic_mime_type(" << fname <<") :" 
           << " magic_file() returned <" << mt << ">" << endl;

    string compress;
    size_t s = mt.find(";"), ce = mt.find("compressed-encoding=");
    if (s!=string::npos && ce!=string::npos && ce>s) {
      compress = mt.substr(ce+20);
      s = compress.find(";");
      if (s!=string::npos)
	compress.erase(s);
    }

    s = mt.find(" (");
    if (s!=string::npos)
      mt.erase(s);

    s = mt.find("; ");
    if (s!=string::npos)
      mt.erase(s);

    // custom hack: for some reason libmagic returns the mime type 
    // "audio/X-HX-AAC-ADTS" with some garbage(?) in front of the string for
    // some mpeg videos as it's first match. If the search is continued 
    // (libmagic stops at first match unless MAGIC_CONTINUE is specified), the 
    // type video/mpeg is found as well.
    // AAC audio is one audio type that can be used in mpeg streams, so this 
    // probably is not a bug in libmagic, but there really is an AAC stream 
    // embedded in the file.

#ifndef NO_PTHREADS
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
#endif // NO_PTHREADS

    static multimap<string,string> prefer;
    if (prefer.empty()) {
      prefer.insert(make_pair("audio/X-HX-AAC-ADTS", "video/mpeg"));
      prefer.insert(make_pair("audio/X-HX-AAC-ADTS", "video/mp2p"));
      prefer.insert(make_pair("audio/X-HX-AAC-ADTS", "video/mpv"));
      prefer.insert(make_pair("image/x-3ds",         "image/tiff"));
      prefer.insert(make_pair("image/x-3ds",         "image/gif" ));
    }

#ifndef NO_PTHREADS
    pthread_mutex_unlock(&mutex);
#endif // NO_PTHREADS

    for (map<string,string>::const_iterator i=prefer.begin();
	 i!=prefer.end(); i++) 
      if (mt.find(i->first) != string::npos) {
	if (debug)
	  cout << "   <" << mt << "> contained <" << i->first << ">" << endl;

	// reopen with option MAGIC_CONTINUE
	m = magic_open( MAGIC_SYMLINK | MAGIC_MIME | MAGIC_COMPRESS | 
			MAGIC_CONTINUE );
	if ( m && !magic_load(m, NULL)) {
	  mime = magic_file(m, fname.c_str());

	  string newmt = mime;
	  if (debug)
	    cout << " magic_file() now returned <" << newmt << ">" << endl;

	  if (newmt.find(i->second) != string::npos) {
	    if (debug)
	      cout << "      <" << newmt << "> contained <" << i->second
		   << ">" << endl;
	    mt = i->second;
	  }
	}

	if (m)
	  magic_close(m);
      }

    if (mt=="application/octet-stream") {
      unsigned char  mpg[4] = { 0x00, 0x00, 0x01, 0xba };
      unsigned char  mkv[4] = { 0x1a, 0x45, 0xdf, 0xa3 };
      unsigned char jpeg[2] = { 0xff, 0xd8             };

      string own_mt;
      ifstream fs(fname.c_str());
      char buf[4];
      memset(buf, 0, sizeof buf);
      fs.read(buf, sizeof buf);
      if (fs) {
	if (!strncmp(buf, (char*)mpg, sizeof mpg))
	  own_mt = "video/mpeg";

	else if (!strncmp(buf, (char*)mkv, sizeof mkv))
	  own_mt = "video/x-matroska";

	else if (!strncmp(buf, (char*)jpeg, sizeof jpeg))
	  own_mt = "image/jpeg";
      }
      if (own_mt!="") {
	mt = own_mt;
	if (debug)
	  cout << "   changed type by own check to <" << mt << ">" << endl;
      }
    }

    if (debug)
      cout << "   returning <" << mt << ">" << endl;

    return make_pair(mt, compress);
#else
    WarnOnce("libmagic is not availbale, some functionality is missing.");
    string tmp = fname;
    return make_pair("", "");
#endif // HAVE_MAGIC_H
  }

  /////////////////////////////////////////////////////////////////////////////

  const struct hostent* GetHostByName(const string& addr, 
                                      struct hostent& myhost) {
    struct hostent *host = NULL;
#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
    char mybuf[10000];
    int myerrno = 0;
    gethostbyname_r(addr.c_str(), &myhost, mybuf, sizeof(mybuf),
                    &host, &myerrno);
#else
    host = gethostbyname(addr.c_str());
    if (host) {
      memcpy(&myhost, host, sizeof(myhost));
      host = &myhost;
    }
#endif // _BSD_SOURCE || _SVID_SOURCE

    if (false && host)
      cout << (int)(unsigned char)myhost.h_addr_list[0][0] << "."
	   << (int)(unsigned char)myhost.h_addr_list[0][1] << "."
	   << (int)(unsigned char)myhost.h_addr_list[0][2] << "."
	   << (int)(unsigned char)myhost.h_addr_list[0][3] << endl;
    
    return host;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifndef NO_PTHREADS
  string ThreadIdentifierUtil(pthread_t thread, pid_t tid) {
    stringstream ss;

#if defined(__linux__)
    ss << tid;
    thread++;
#elif defined(_MSC_VER)
    ss << "threadXX";
    tid++;
#else
    ss << thread;
    tid++;
#endif // __linux__

    return ss.str();
  }
#endif // NO_PTHREADS

  /////////////////////////////////////////////////////////////////////////////

  bool WriteEquationFile(const string& f, const map<string,string>& eq,
			 const map<string,double>& var,
			 const map<string,double>& cst) {
    string msg = "WriteEquationFile("+f+") : ";

    ofstream os(f);

    os << "# equations:" << endl;
    for (auto i=eq.begin(); i!=eq.end(); i++) {
      string s = i->first+" = "+i->second;
      for (size_t p=0;;) {
	p = s.find('*', p);
	if (p==string::npos)
	  break;
	s.replace(p, 1, " * ");
	p += 3;
      }
      os << s << endl;
    }

    os << "# variables:" << endl;
    for (auto i=var.begin(); i!=var.end(); i++) {
      string s = i->first+" = "+ToStr(i->second);
      os << s << endl;
    }

    os << "# constants:" << endl;
    for (auto i=cst.begin(); i!=cst.end(); i++) {
      string s = "const "+i->first+" = "+ToStr(i->second);
      os << s << endl;
    }

    return os.good();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool ReadEquationFile(const string& f, map<string,string>& eq,
			map<string,double>& var, map<string,double>& cst) {
    string msg = "ReadEquationFile("+f+") : ";

    string ss = FileToString(f);
    if (ss=="")
      return ShowError(msg+"file not found or empty");

    vector<string> a = SplitInSomething("\n", false, ss);
    
    for (auto i=a.begin(); i!=a.end(); i++) {
      string s = *i;
      bool is_const = s.find("const ")==0;
      if (is_const)
	s.erase(0, 6);

      for (;;) {
	size_t p = s.find_first_of(" \t");
	if (p==string::npos)
	  break;
	s.erase(p, 1);
      }

      if (s=="" || s[0]=='#')
	continue;

      pair<string,string> kv = SplitKeyEqualValueNew(s);
      if ((kv.second[0]>='0' && kv.second[0]<='9') ||
	  kv.second[0]=='-' || kv.second[0]=='+') {
	map<string,double>& m = is_const?cst:var;
	if (m.find(kv.first)!=m.end())
	  return ShowError(msg+"Variable/Const <"+kv.first+
			   "> multiply defined");

	if (kv.second.find_first_not_of("0123456789.e-+")!=string::npos)
	  return ShowError(msg+"<"+kv.second+"> doesn't look like a number");

	m[kv.first] = atof(kv.second.c_str());

      } else {
	if (eq.find(kv.first)!=eq.end())
	  return ShowError(msg+"Equation <"+kv.first+"> multiply defined");
	eq[kv.first] = kv.second;
      }
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<double> SolveMath(const vector<string>& v,
			   const map<string,string>& e, bool angry) {
    vector<double> r;
    for (auto i=v.begin(); i!=v.end(); i++)
      r.push_back(SolveMath(*i, e, angry));
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  double SolveMath(const string& v, const map<string,string>& ein,
		   bool angry) {
    string msg = "SolveMath("+v+","+ToStr(ein.size())+" equations) : ";

    map<string,string> e = ein;
    double nan = std::nan("");
    string s = v;

    for (;;) {
      for (;;) {
	size_t p = s.find_first_of(" \t");
	if (p==string::npos)
	  break;
	s.erase(p, 1);
      }
      if (s=="") {
	if (angry)
	  ShowError(msg+"dependent variable is empty string");
	return nan;
      }

      vector<string> a = SplitInSomething("*", false, s);
      bool changed = false;
      for (auto i=a.begin(); i!=a.end(); i++)
	if (i->size()==0) {
	  if (angry)
	    ShowError(msg+"expression has zero number of terms");
	  return nan;

	} else if (((*i)[0]>='A' && (*i)[0]<='Z') ||
		 ((*i)[0]>='a' && (*i)[0]<='z')) {
	  if (e.find(*i)==e.end()) {
	    if (angry)
	      ShowError(msg+"substitution for <"+*i+"> not found");
	    return nan;
	  }
	  string o = *i;
	  *i = e[*i];
	  if (*i==o) {
	    if (angry)
	      ShowError(msg+"substitution for <"+*i+"> is a loop");
	    return nan;
	  }
	  changed = true;
	}
      s = JoinWithString(a, "*", false);
      
      if (!changed) {
	double val = 1;
	for (auto i=a.begin(); i!=a.end(); i++)
	  val *= atof(i->c_str());
	return val;
      }
    }

    if (angry)
      ShowError(msg+"impossible escape from a closed loop");

    return nan;
  }

} // namespace picsom

