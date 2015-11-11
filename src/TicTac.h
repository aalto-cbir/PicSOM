// -*- C++ -*-  $Id: TicTac.h,v 2.9 2015/10/01 10:02:21 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _TICTAC_H_
#define _TICTAC_H_

#include <missing-c-utils.h>

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif //HAVE_SYS_TIMES_H

#include <fstream>
#include <string>
#include <list>
#include <cmath>
using namespace std;

#if defined(sgi) && defined(_OPENMP)
#include <mpc.h>
#else 
#define mp_setlock()
#define mp_unsetlock()
#endif // defined(sgi) && defined(_OPENMP)

class TicTac {
public:
  /// Constructor with optional name.
  TicTac(const char *n = NULL) :
    myname(""), verbose(0), active(false), stopped(false), log(&cerr),
    level(0), separator("/"), error_count(0), longtictactxt(false),
    last_tac(NULL) {
    if (n)
      SetName(n);
    SetStartTime(); SetLastTime();SetStopTime(); }

  /// Sets myname after construction.
  void SetName(const char *n) { myname = n; }

  /// Sets verbosity level
  void Verbose(int v) {
    int x = v+1;
    if (verbose>0 || v>0) x = 0;
    verbose = v;
    Log(x, "Set verbosity level", String(v)); }

  /// Checks if verbosity is equal or exceeds the value.
  bool IsVerbose(int v) const { return verbose>=v; }

  /// Returns value of active.
  bool Active() const { return active; }

  /// Sets the activation state and initial time.
  void Start() {
    active = true;
    SetStartTime();
    Log(1, "Starting"); }

  /// Resets the activation state and initial time.
  void Stop() {
    stopped = true;
    active = false;
    SetStopTime();
    Log(1, "Stopping"); }

  /// Sets the logging stream.
  void Log(ostream& os) { log = &os; }

  /// Prints the summary.
  void Summary(bool cum = true, bool diff = true, ostream& os = cout) {
    tms now_cpu;
    timespec_t now_real;
    SetTimes(now_real, now_cpu);

    tm *time = localtime(&now_real.tv_sec);
    char timestr[1000];
    // in POSIX 1003.1-2001 %T==%H:%M:%S 
    // but version 3.2 "g++ -pedantic -Wformat" (correctly) doesn't allow it
    strftime(timestr, sizeof timestr, "[%d/%b/%Y %H:%M:%S] ", time);
    if (diff) {
      time = localtime(&last_real.tv_sec);
      strftime(timestr+strlen(timestr), sizeof(timestr)-strlen(timestr),
	       "Last readout [%d/%b/%Y %H:%M:%S]", time);
    }
    if (last_tac)
      sprintf(timestr+strlen(timestr), " Last Tac() from %s", last_tac);

    for (int i=0; i<79; i++)
      os << "*";
    os << endl
       << "TicTac summary of <" << myname << "> " << timestr << " :" << endl;

    if (error_count)
      os << "**" << endl << "** There have been " << error_count << " errors"
	 << endl << "**" << endl;

    int maxtictacs, usersum, systsum, ttw;
    timespec_t rsum;
    CalculateSums(true, maxtictacs, rsum, usersum, systsum, ttw);
    os << "  ";
    for (int j=ttw-1; j>=0; j--)
      os << (j>3?' ':"tics"[(ttw>3?3:ttw-1)-j]);

    os << "       real      user     sys    cpu     exe     own " << endl;

    if (diff) {
      DifferenceSummary(os);
      DifferenceReset();
    }

    if (cum)
      CumulativeSummary(os);
  }

  /// Prints cumulative time usage summary.
  void CumulativeSummary(ostream& os = cout) {
    CommonSummary(true, os);
  }

  /// Prints time usage summary relative to last display.
  void DifferenceSummary(ostream& os = cout) {
    CommonSummary(false, os);
  }

  /// Prints time usage summary.
  void CommonSummary(bool cum, ostream& os = cout) {
    tms now_cpu;
    timespec_t now_real;
    SetTimes(now_real, now_cpu);

    tms& cpunow     = stopped&&cum ? stop_cpu  : now_cpu;
    timespec_t& realnow = stopped&&cum ? stop_real : now_real;

    tms& cputhen     = cum ? start_cpu  : last_cpu;
    timespec_t& realthen = cum ? start_real : last_real;

    int maxtictacs, usersum, systsum, ttw;
    timespec_t rsum;
    CalculateSums(true, maxtictacs, rsum, usersum, systsum, ttw);
    
    if (!cum) {
      int foo, bar;
      CalculateSums(false, foo, rsum, usersum, systsum, bar);
    }

    int udiff   = cpunow.tms_utime-cputhen.tms_utime;
    int sdiff   = cpunow.tms_stime-cputhen.tms_stime;
    int div     = udiff+sdiff;
    timespec_t rdiff = { realnow.tv_sec-realthen.tv_sec,
			 realnow.tv_nsec-realthen.tv_nsec };

    char cd = cum?'C':'D';

    int lines = 0;
    list<ttentry>::iterator i;
    for (i=entries.begin(); i!=entries.end(); i++) {
      const ttentry::entryinfo& info = cum ? i->cum : i->dif;
      int ch_user, ch_sys;
      CalculateChildTimes(*i, cum, ch_user, ch_sys);
      lines += PrintLine(cd, true, os, ttw, info.tics, info.tacs,
			 info.real, info.user, info.system,
			 ch_user, ch_sys, div, i->name.c_str());
    }

    if (lines) {
      for (int l=0; l<ttw; l++)
	os << (longtictactxt?"--":"-");
      os << "----------------------------------------------------------"
	 << (longtictactxt?"-":"") << endl;
    }
    const char *sum1 = "-- 1st level sum --";
    const char *sum2 = "-- overall total --";
    if (lines)
      PrintLine(cd, false, os, ttw, -1, 0, rsum,  usersum, systsum,
		-1, -1, div, sum1);
    PrintLine(  cd, false, os, ttw, -1, 0, rdiff, udiff,   sdiff,
		-1, -1, div, sum2);
    os << "**" << endl;
  }

  /// Calculates sums and maximums.
  void CalculateSums(bool cum, int& maxtictacs, timespec_t& realsum,
		     int& usersum, int& systsum, int& ttw) {
    realsum.tv_nsec = realsum.tv_sec = 0;
    usersum = systsum = 0;
    maxtictacs = 1;
    list<ttentry>::iterator i;
    for (i=entries.begin(); i!=entries.end(); i++) {
      const ttentry::entryinfo& info = cum ? i->cum : i->dif;
      if (info.tics>maxtictacs) maxtictacs = info.tics;
      if (info.tacs>maxtictacs) maxtictacs = info.tacs;
      if (Depth(*i)==0) {
	realsum.tv_sec  += info.real.tv_sec;
	realsum.tv_nsec += info.real.tv_nsec;
	usersum += info.user;
	systsum += info.system;
      }
    }
    ttw = int(floor(log10((double)maxtictacs))+1);
  }

  /// Resets difference values after display.
  void DifferenceReset() {
    SetLastTime();
    list<ttentry>::iterator i;
    for (i=entries.begin(); i!=entries.end(); i++)
      i->reset_difference();
  }

  void Dump(ostream& os = cout) {
    os << "********" << endl;
    os << "level=" << level << endl;
    os << "*" << endl;
    Summary(true, true, os);
    os << "********" << endl;
  }

  /// Starts time counting for function.
  void Tic(const char *f) {
    if (!active)
      return;
    
    mp_setlock();

    Log(2, "Entered Tic(", f, ")");
    ttentry *l = FindLast();
    if (IsVerbose(4)) {
      if (l)
	Log(4, " - FindLast() returned", l->name.c_str(), String(l));
      else 
	Log(4, " - FindLast() returned NULL");
    }

    string lf(CreateName(l, f));
    Log(3, " ** created name<", lf.c_str(), ">");

    ttentry& e = Find(lf.c_str());
    e.increment_tics();
    e.level = ++level;

    e.set_tic_times_now();

    mp_unsetlock();
  }

  /// Stops time counting for function of last Tic() call.
  void Tac(const char *f = NULL) {
    if (!active)
      return;

    last_tac = f; // hopefully only static arguments for Tac()...

    mp_setlock();

    Log(2, "Entered Tac(", f, ")");
    ttentry *l = FindLast();
    if (!l) {
      ShowError("Tac(", f, ") not found");
      mp_unsetlock();
      return;
    }
    if (IsVerbose(4))
      Log(4, " - FindLast() returned", l->name.c_str(), String(l));

    if (f && !l->lastpartmatch(f))
      ShowError("Tac(", f, ") does not match <", l->name.c_str(), ">");

    l->increment_tacs();
    l->level = 0;
    --level;

    l->increment_times();

    mp_unsetlock();
  }

  /// Returns differences of stop_real/cpu and start_real/cpu.
  void Times(float& real, float& user, float& sys) const {
    double tic = TickSize();
    int udiff   = stop_cpu.tms_utime-start_cpu.tms_utime;
    int sdiff   = stop_cpu.tms_stime-start_cpu.tms_stime;
    user = udiff/tic;
    sys  = sdiff/tic;

    timespec_t rdiff = { stop_real.tv_sec -start_real.tv_sec,
			 stop_real.tv_nsec-start_real.tv_nsec };
    ttentry::normalize(rdiff);
    real = rdiff.tv_sec+rdiff.tv_nsec/1000000000.0;
  }
    
  /// Sets a pair of times.
  static void SetTimes(timespec_t& real, tms& cpu) {
    times(&cpu);
    clock_gettime(CLOCK_REALTIME, &real);
  }

  ///
  static double TickSize() {
#if defined(HAVE_SYSCONF) && defined(_SC_CLK_TCK)
    return (double)sysconf(_SC_CLK_TCK);
#elif defined(CLK_TCK)
    return (double)CLK_TCK;
#else
    return 1.0;
#endif // defined(HAVE_SYSCONF)
  }

protected:
  /// The data is kept here:
  class ttentry {
  public: 
    /// This structure collects the values.
    class entryinfo {
    public:
      entryinfo() : tics(0), tacs(0), user(0), system(0) {
	real.tv_nsec = real.tv_sec = 0; }

      /// Number of Tic() calls.
      int tics;
      
      /// Number of Tac() calls.
      int tacs;

      /// Cumulative wall clock time.
      timespec_t real;

      /// Cumulative user mode CPU usage.
      int user;

      /// Cumulative system mode CPU usage.
      int system;

    };  /// class entryinfo ends here

    /// Constructor.
    ttentry(TicTac *t, const char *n) : tictac(t), name(n), level(0) { 
      tictac->Log(5, " ** construcing ttentry **");
    } 
    
    /// Destructor.
    ~ttentry() { tictac->Log(5, " ** destructing ttentry **"); }

    /// Sets the timestamp.
    void set_tic_times_now() {
      SetTimes(tic_real, tic_cpu);

      const char vl = 6;
      if (tictac->IsVerbose(vl)) {
	char tmp[1000];
	sprintf(tmp, "times=%d.%d utime=%d stime=%d",
		(int)tic_real.tv_sec, (int)tic_real.tv_nsec,
		(int)tic_cpu.tms_utime, (int)tic_cpu.tms_stime);
	tictac->Log(vl, " ** setting timestamp    ", tmp);
      }
    }

    /// Increments the tics counts.
    void increment_tics() {
      cum.tics++;
      dif.tics++;
    }

    /// Increments the tacs counts.
    void increment_tacs() {
      cum.tacs++;
      dif.tacs++;
    }

    /// Increments the cumulative times relative to previously-set timestamp.
    void increment_times() {
      timespec_t tac_real;
      tms tac_cpu;
      SetTimes(tac_real, tac_cpu);

      const char vl = 6;
      if (tictac->IsVerbose(vl))
	showtimes();

      int reals = tac_real.tv_sec -tic_real.tv_sec;
      int realn = (int)(tac_real.tv_nsec-tic_real.tv_nsec);
      int user  = tac_cpu.tms_utime-tic_cpu.tms_utime;
      int syst  = tac_cpu.tms_stime-tic_cpu.tms_stime;

      cum.real.tv_sec  += reals;
      cum.real.tv_nsec += realn;
      cum.user   += user;
      cum.system += syst;

      dif.real.tv_sec  += reals;
      dif.real.tv_nsec += realn;
      dif.user   += user;
      dif.system += syst;

      normalize(cum.real);
      normalize(dif.real);

      if (tictac->IsVerbose(vl)) {
	char tmp[1000];
	sprintf(tmp, "times=%d.%d utime=%d stime=%d",
		(int)tac_real.tv_sec, (int)tac_real.tv_nsec,
		(int)tac_cpu.tms_utime, (int)tac_cpu.tms_stime);
	tictac->Log(vl, " ** subtracting timestamp", tmp);
	showtimes();
      }
    }

    /// Normalizes timespec_t.
    static void normalize(timespec_t& t) {
      const long nano = 1000000000;
      while (t.tv_nsec<0) {
	t.tv_nsec += nano;
	t.tv_sec--;
      }
      while (t.tv_nsec>=nano) {
	t.tv_nsec -= nano;
	t.tv_sec++;
      }
    }

    /// Shows cumulative times.
    void showtimes() const {
      char tmp[1000];
      sprintf(tmp, "real=%d.%d user=%d system=%d",
	      (int)cum.real.tv_sec, (int)cum.real.tv_nsec,
	      cum.user, cum.system);
      tictac->Log(6, " ** cumulative times are:", tmp);

      sprintf(tmp, "real=%d.%d user=%d system=%d",
	      (int)dif.real.tv_sec, (int)dif.real.tv_nsec,
	      dif.user, dif.system);
      tictac->Log(6, " ** difference times are:", tmp);
    }

    /// Resets difference values.
    void reset_difference() {
      dif = entryinfo();
    }

    /// Checks if the string matches.
    bool lastpartmatch(const char *f) const {
      string::size_type idx = name.rfind(f);  // was int
      return idx+strlen(f)==name.size();
    }

    /// Checks the names.
    bool is_child(const ttentry& e) const {
      const string& sep = tictac->separator;
      size_t s = name.size();

      //       cout << "is_child: " << name << " " << e.name << " "
      // 	   << s << " " << sep << endl;

      if (e.name.size()<s+sep.size()+1)
	return false;
      if (e.name.substr(0, s)!=name)
	return false;
      if (e.name.substr(s, sep.size())!=sep)
	return false;
      
      return e.name.find(sep, s+sep.size())==string::npos;
    }

    /// Pointer upwadrs.
    TicTac *tictac;
    
    /// Name of the function or such.
    string name;

    /// Level of this entry in stack.
    int level;

    /// Wall clock time of last Tic().
    timespec_t tic_real;

    /// CPU usage of last Tic().
    tms tic_cpu;

    /// Values cumulated from beginning.
    entryinfo cum;

    /// Values cumulated from last display.
    entryinfo dif;

  }; /// class ttentry ends here

  friend class TicTac::ttentry;

  /// Finds existing or creates new entry for named function.
  ttentry& Find(const char *f) {
    Log(3, "  Entered Find(", f, ")");

    list<ttentry>::iterator i;
    for (i=entries.begin(); i!=entries.end(); i++) {
      if (i->name==f) {
	if (IsVerbose(4))
	  Log(4, "    ", f, "matches", i->name.c_str(), String(&*i));
	break;

      } else
	if (IsVerbose(4))
	  Log(4, "    ", f, "doesn't match", i->name.c_str(), String(&*i));
    }
    if (i==entries.end()) {
      entries.push_back(ttentry(this, f));
      if (IsVerbose(4))
	Log(4, "   - adding   ", f, String(&entries.back()));
      i = entries.end();
      i--;
    }

    if (IsVerbose(4))
      Log(4, "   - returning", i->name.c_str(), String(&*i));

    return *i;
  }

  /// Returns the entry of last Tic().
  ttentry *FindLast() {
    if (IsVerbose(3)) {
      char lev[100];
      sprintf(lev, "level=%d", level);
      Log(3, "  Entered FindLast()", lev);
    }
    if (!level) {
      Log(4, "   - level zero: no entries");
      return NULL;
    }

    list<ttentry>::iterator i;
    for (i=entries.begin(); i!=entries.end(); i++)
      if (i->level==level) {
	if (IsVerbose(4))
	  Log(4, "   - returning", i->name.c_str(), String(&*i));

	return &*i;

      } else if (i->level>level)
	ShowError("!! inconsistency !!");

    Log(3, "   - NO ENTRY FOUND !!!");

    return NULL;
  }

  /// Calculates time usage of children.
  void CalculateChildTimes(const TicTac::ttentry& e, bool cum,
			   int& user, int& sys) const {
    user = sys = 0;
    list<ttentry>::const_iterator i;
    for (i=entries.begin(); i!=entries.end(); i++) {
      // cout << "CalculateChildTimes(): " << e.name << " " << e.level
      //	   << " " << i->name << " " << i->level << endl;

      if (/*i->level==e.level+1 &&*/ e.is_child(*i)) {
	user += (cum ? i->cum : i->dif).user;
	sys  += (cum ? i->cum : i->dif).system;
	// cout << "added " << (cum ? i->cum : i->dif).user
	//      << " and "  << (cum ? i->cum : i->dif).system << endl;
      }
    }
  }

  /// Conditionally prints to logging stream.
  void Log(int l, const char *a, const char *b = NULL, const char *c = NULL, 
	   const char *d = NULL, const char *e = NULL, const char *f = NULL) {
    if (IsVerbose(l))
      *log << "TicTac: "
	   << a << " " << b << " " << c << " "
	   << d << " " << e << " " << f
	   << endl;
  }

  /// Displays error message.
  void ShowError(const char *a, const char *b = NULL, const char *c = NULL, 
	   const char *d = NULL, const char *e = NULL, const char *f = NULL) {
    cerr << "TicTac ERROR: "
	 << a << " " << b << " " << c << " " << d << " " << e << " " << f
	 << endl;
    error_count++;
  }

  /// Prints one line of summary.
  bool PrintLine(int ch, bool skipz, ostream& os, int ttw, int tics, int tacs,
		 const timespec_t& real, int user, int system,
		 int ch_user, int ch_system, int div, const char *n) {
    if (skipz && !tics)
      return false;

    if (!div)
      div = 1;

    char tictactxt[100];
    if (tics>=0)
      if (longtictactxt)
	sprintf(tictactxt, "%*d/%*d", ttw, tics, ttw, tacs);
      else
	sprintf(tictactxt, "%*d%c", ttw, tics, tics==tacs?' ':'*');
    else {
      int il = longtictactxt ? 2*ttw+1 : ttw+1;
      for (int i=0; i<il; i++)
	tictactxt[i] = ' ';
      tictactxt[il] = 0;
    }

    double tck = TickSize();
    double nano = 1e-9;
    double rtime = (double)real.tv_sec+(double)real.tv_nsec*nano;
    double portion = 100.0*(user+system)/div;
    double ownportion = portion-100.0*(ch_user+ch_system)/div;

    char frac[20] = "        ";
    if (rtime)
      sprintf(frac, "(%5.1f%%)", 100.0*(user+system)/(tck*rtime));

    char own[20] = "        ";
    if (ch_user>=0 && ch_system>=0)
      sprintf(own, "[%5.1f%%]", ownportion);

    char line[1000];
    sprintf(line,
	    "%c %s %8.1fs %8.1fs %6.1fs %s %5.1f%% %s %s",
	    ch, tictactxt, rtime, user/tck, system/tck, frac, portion,
	    own, n);

    os << line << endl;

    return true;
  }

  /// Sets start times.
  void SetStartTime() { SetTimes(start_real, start_cpu); }

  /// Sets last display times.
  void SetLastTime() { SetTimes(last_real, last_cpu); }

  /// Sets stop times.
  void SetStopTime() { SetTimes(stop_real, stop_cpu); }

  /// Formats argument for printing.
  static const char *String(const void *p) {
    static char str[100];
    sprintf(str, "0x%p", p);
    return str; }

  /// Formats argument for printing.
  static const char *String(int i) {
    static char str[100];
    sprintf(str, "%d", i);
    return str; }

  /// Creates new entry name.
  string CreateName(const ttentry *l, const char *f) {
    string lf;
    if (l)
      lf += l->name + separator;
    lf += f;
    
    return lf; }

  /// Returns depth of entry in call stack.
  int Depth(const ttentry& e) {
    int n = 0;
    for (string::size_type i=0; i<e.name.size();) {
	string::size_type j = e.name.find(separator, i);
	if (j==string::npos)
	  break;
	n++;
	i = j+1;
    }
    
    return n;
  }

  /// Optional name of instance.
  string myname;

  /// Verbosity level.
  int verbose;

  /// True if active.
  bool active;

  /// True if stop times have been set.
  bool stopped;

  /// Logging stream.
  ostream *log;

  /// The Data!
  list<ttentry> entries;

  /// Depth of call stack.
  int level;

  /// Start CPU time.
  tms start_cpu;

  /// Last display CPU time.
  tms last_cpu;

  /// Stop CPU time.
  tms stop_cpu;

  /// Start wall clock.
  timespec_t start_real;

  /// Last display wall clock.
  timespec_t last_real;

  /// Stop wall clock.
  timespec_t stop_real;

  /// Separator string between function names.
  string separator;

  /// The number of errors detected.
  int error_count;

  /// True if bots tics and tacs should be shown.
  bool longtictactxt;

  /// Pointer to the argument of the last Tac().
  const char *last_tac;
};

#endif // _TICTAC_H_

