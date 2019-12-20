// -*- C++ -*-  $Id: PicSOM.C,v 2.618 2019/11/19 12:26:43 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#define __STDC_CONSTANT_MACROS
#include <stdint.h>

// this affects inclusion of <string.h> to declare strsignal().
#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // !_GNU_SOURCE 
#endif // __linux__

#include <PicSOM.h>
#include <DataBase.h>
#include <Query.h>
#include <Analysis.h>
#include <TSSOM.h>
#include <Connection.h>
#include <RemoteHost.h>
#include <CbirStageBased.h>
#include <Feature.h>
#include <RandVar.h>
#include <SVM.h>

#include <map>

#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <iconv.h>

#include <sys/utsname.h>
#include <sys/types.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif // HAVE_NETDB_H

#ifdef HAVE_SYS_LOADAVG_H
#include <sys/loadavg.h>
#endif // HAVE_SYS_LOADAVG_H

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif // HAVE_SYS_UTSNAME_H

#if defined(PICSOM_USE_OPENCV)
#  if defined(HAVE_OPENCV2_CORE_CORE_HPP)
#  include <opencv2/core/core.hpp>
#  endif // HAVE_OPENCV2_CORE_CORE_HPP
#  if defined(HAVE_OPENCV2_CORE_CUDA_HPP)
#  include <opencv2/core/cuda.hpp>
#  endif // HAVE_OPENCV2_CORE_CUDA_HPP
#  if defined(HAVE_OPENCV2_CORE_GPUMAT_HPP)
#  include <opencv2/core/gpumat.hpp>
#  endif // HAVE_OPENCV2_CORE_GPUMAT_HPP
#endif // PICSOM_USE_OPENCV

#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/opensslv.h>
#endif // HAVE_OPENSSL_SSL_H

#ifdef HAVE_WN_H
#include <wn.h>
static string foo = string(license)+string(dblicense);
#endif // HAVE_WN_H

#ifdef HAVE_ASPELL_H
#include <aspell.h>
#endif // HAVE_ASPELL_H

#ifdef HAVE_FLANDMARK_DETECTOR_H
#include <flandmark_detector.h>
#endif // HAVE_FLANDMARK_DETECTOR_H

#ifdef HAVE_MKL_H
#include <mkl.h>
#endif // HAVE_MKL_H

#ifdef HAVE_MAD_H
#include <mad.h>
#endif // HAVE_MAD_H

#ifdef HAVE_SNDFILE_H
#include <sndfile.h>
#endif // HAVE_SNDFILE_H

#ifdef HAVE_AUDIOFILE_H
#include <audiofile.h>
#endif // HAVE_AUDIOFILE_H

#ifdef HAVE_GLOG_LOGGING_H
#include <glog/logging.h>
#endif // HAVE_GLOG_LOGGING_H

#ifdef HAVE_JSON_JSON_H
#include <json/json.h>
#endif // HAVE_JSON_JSON_H

#ifdef HAVE_THRIFT_THRIFT_H
//#include <thrift/version.h>
#endif // HAVE_THRIFT_THRIFT_H

#if defined(HAVE_CAFFE2_CORE_MACROS_H) && defined(PICSOM_USE_CAFFE2)
#include <caffe2/core/init.h>
#endif // HAVE_CAFFE2_CORE_MACROS_H && PICSOM_USE_CAFFE2

#if defined(PICSOM_HAVE_OCTAVE) && defined(PICSOM_USE_OCTAVE)
// config.h conflicts:
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
// /usr/include/mad.h conflict:
#undef SIZEOF_LONG
#include <octave/oct.h>
#include <octave/octave.h>
#include <octave/parse.h>
#endif // PICSOM_HAVE_OCTAVE && PICSOM_USE_OCTAVE

extern "C" {
#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#include <libavcodec/avcodec.h>
#endif // HAVE_LIBAVCODEC_AVCODEC_H

#ifdef HAVE_LIBAVDEVICE_AVDEVICE_H
#include <libavdevice/avdevice.h>
#endif // HAVE_LIBAVDEVICE_AVDEVICE_H

#ifdef HAVE_LIBAVFILTER_AVFILTER_H
#include <libavfilter/avfilter.h>
#endif // HAVE_LIBAVFILTER_AVFILTER_H

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#endif // HAVE_LIBAVFORMAT_AVFORMAT_H

#ifdef HAVE_LIBAVUTIL_AVUTIL_H
#include <libavutil/avutil.h>
#endif // HAVE_LIBAVUTIL_AVUTIL_H

#ifdef HAVE_LIBSWSCALE_SWSCALE_H
#include <libswscale/swscale.h>
#endif // HAVE_LIBSWSCALE_SWSCALE_H
}

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef PICSOM_USE_SLMOTION
#include <../slmotion/slmotion.hpp>
#endif // PICSOM_USE_SLMOTION

#ifdef PICSOM_USE_TENSORFLOW
#include <tensorflow/core/public/version.h>
#endif // PICSOM_USE_TENSORFLOW

#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
#endif // HAVE_TIFFIO_H

#ifdef HAVE_GIF_LIB_H
#include <gif_lib.h>
#endif // HAVE_GIF_LIB_H

#ifdef HAVE_H5CPP_H
#include <H5Cpp.h>
#endif // HAVE_H5CPP_H

#ifdef PICSOM_USE_PYTHON
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include <Python.h>
#endif // PICSOM_USE_PYTHON

#if defined(HAVE_THC_H) && defined(PICSOM_USE_TORCH)
extern "C" {
#include <luajit.h>
}
#endif // HAVE_THC_H && PICSOM_USE_TORC

#ifndef CODEFLAGS
#define CODEFLAGS "--unknown--"
#endif // CODEFLAGS

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  const string PicSOM_C_vcid =
    "@(#)$Id: PicSOM.C,v 2.618 2019/11/19 12:26:43 jormal Exp $";

  int PicSOM::debug_times  = 0;
  int PicSOM::debug_mem    = 0;
  int PicSOM::debug_octave = 0;
  int PicSOM::debug_osrs   = 0;
  int PicSOM::debug_tmps   = 0;

  bool PicSOM::debug_interpret = false;
  bool PicSOM::debug_slaves    = false;
  bool PicSOM::keep_temp       = false;

  bool PicSOM::force_fast_slave = true;

  string PicSOM::forcedhostname;

  bool ModifiableFile::debug_zerotimes = false;
  bool ModifiableFile::debug_modified  = false;

#ifdef PICSOM_USE_PTHREADS
  bool PicSOM::debug_threads = false;

  string PicSOM::launch_browser_str;
  string PicSOM::browser;
#endif // PICSOM_USE_PTHREADS

  void (PicSOM::*PicSOM::signal_function[128])(int);
  PicSOM *PicSOM::signal_instance[128];
  PicSOM *PicSOM::atexit_instance = NULL;

  PicSOM::version_data_type PicSOM::version_data;

  bool PicSOM::has_features_internal     = true;
  bool PicSOM::has_segmentation_internal = true;
  bool PicSOM::has_imgrobot_internal     = false;

#if defined(PICSOM_USE_CAFFE)||defined(PICSOM_USE_CAFFE2)
  static bool caffe2_initialized = false;
#endif // PICSOM_USE_CAFFE||PICSOM_USE_CAFFE2

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define _stringify_(x) _stringify__(x) 
#define _stringify__(x) #x 

  PicSOM::PicSOM(const vector<string>& arg) : tics("PicSOM") {
#ifdef PICSOM_USE_PTHREADS
    main_thread = pthread_self();
    listen_thread_set = mpi_listen_thread_set = false;
    slavestatus_thread_set = analysis_thread_set = false;
    threads = -1;
    pthreads_connection = pthreads_tssom = false;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex,     &attr);
    pthread_mutex_init(&log_mutex, NULL);
#endif // PICSOM_USE_PTHREADS

    SetTimeNow(start_time);

#ifdef PICSOM_USE_PTHREADS
    xmlInitParser();
#endif // PICSOM_USE_PTHREADS

#ifdef HAVE_WN_H
    wninit();
#endif // HAVE_WN_H

    // Initial values for variables:
    xmlIndentTreeOutput = true;
    trace_other_keys    = true;
    quiet               = false;
    has_cin             = true;

    myname_str = mybinary_str = arg.size() ? arg[0] : "";

#if defined(HAVE_GLOG_LOGGING_H) && (defined(PICSOM_USE_CAFFE)||defined(PICSOM_USE_CAFFE2))
    // should test something like google::IsGoogleLoggingInitialized()
    if (!Feature::GlogInitDone() && !caffe2_initialized)
      google::InitGoogleLogging(myname_str.c_str());
    FLAGS_logtostderr = 0;
    Feature::GlogInitDone(true);
#endif // HAVE_GLOG_LOGGING_H && PICSOM_USE_CAFFE||CAFFE2

    listen_connection = mpi_listen_connection =
      soap_server = speech_recognizer = NULL;
    uselocalhost = false;

#ifdef __linux__
    char exebuf[1000];
    int exebuf_read = readlink("/proc/self/exe", exebuf, sizeof exebuf);
    if (exebuf_read>=0)
      mybinary_str = string(exebuf, exebuf_read);
#endif // __linux__

    CreateEnvironment();

    Index::ReorderMethods();

    // TempDirRoot();                        // to initialize static variables

    sqlserver    = "localhost";
    tempsqlitedb = true;

    gpupolicy = 0;
    gpudevice = -1;
    nvidia_smi_ok = true;

    development    = true;
    defaultdirpath = true;
    norootdir      = false;

    string fallback_path = "/usr/local/lib/picsom";
    path_str = fallback_path;
#ifdef PICSOM_PATH
    path_str = _stringify_(PICSOM_PATH);
    if (path_str[0]!='/')
      path_str = UserHomeDir()+"/"+path_str;
#endif // PICSOM_PATH
    if (!DirectoryExists(path_str))
      path_str = "";

#ifdef HAVE_WINREG_H
    path_str = "something from the windows registry";
#endif // HAVE_WINREG_H

    quit = false;
    start_online = start_analyse = start_version_only = false;

    no_save       = false;
    expunge_time  = 600;

    allow_fork = true;
    fork_faked = false;

    streambuf_cout = streambuf_cerr = streambuf_clog = NULL;

    ZeroTime(speech_zero_time);
    ZeroTime(speech_recognizer_start_time);

    if (!atexit_instance) {
      AtExitInstance(this);
      atexit((CFP_atexit) PicSOM::WriteLogExit);
    }

    // SolveVersionData();  // moved downwards to obey -nolinpack
    sysarchitecture = SolveArchitecture();
    if (sysarchitecture.find("linux")==0)
      linuxdistr = SolveLinuxDistribution();

    use_signals = childlimits = true;
    guarding = use_nosignals = is_slave = expect_slaves = false;
    guarded_pid = -1;

    LoggingModeTimestampIdentity();

#ifdef PICSOM_USE_PYTHON
    // setenv("PYTHONPATH", ".", 1);
    Py_Initialize();
    PyEval_InitThreads();
    PyEval_SaveThread(); // obs!
    string pyv = "2";
#ifdef PICSOM_USE_PYTHON3
    pyv = "3";
#endif // PICSOM_USE_PYTHON3
    // this should obey quiet switch -q ...
    // WriteLog("Python"+pyv+" initialized");
    // TestPyGILState("PicSOM::PicSOM()");
#endif // PICSOM_USE_PYTHON

    if (env.find("_CONDOR_JOB_AD")==env.end()) {
      gpudevice = SolveGpuDevice();
      if (gpudevice==-2) {
	nvidia_smi_ok = false;
	gpudevice = -1;
      }
    }
    Feature::GpuDeviceId(gpudevice);

    int mypid = getpid();

    soap_port = 0;
    ListenPort(0, 0);
    listen_mpi = false;
#ifdef PICSOM_USE_MPI
    use_mpi_slaves = true;
#else
    use_mpi_slaves = false;
#endif // PICSOM_USE_MPI
    use_slaves = true;

    CleanUpBogusDirs();

    for (size_t ai=0; ai<arg.size(); ai++) {
      cmdline_str += (ai?" ":"");
      if (arg[ai].find_first_of(" \t\n")!=string::npos)
	cmdline_str += string("\"") + arg[ai] + "\"";
      else
	cmdline_str += arg[ai];
    }

    vector<string> aa;
    for (size_t ai=1; ai<arg.size(); ai++)
      aa.push_back(arg[ai]);
    bool args_ok = CommandLineParsing(aa);
    bool runs_in_bg = mypid!=getpid() || fork_faked;

    // if (guarding && !runs_in_bg)
    //   args_ok = false;

    if (!args_ok) {
      ShowUsage(true);
      exit(1);
    }

    if (env.find("ROOT")!=env.end()) {
      path_str = env["ROOT"];

    } else {
      if (!DirectoryExists(path_str)) {
	path_str = RootDir();
	defaultdirpath = false;
      }
    }

    // this not yet fully thought...
    if (norootdir)
      path_str = env["ROOT"] = "/NO/PICSOM/ROOT";

    Segmentation::PicSOMroot(path_str);
    Feature::PicSOMroot(path_str);
    videofile::local_bin(Path()+"/"+SolveArchitecture()+"/bin");

    if (guarding) {
      if (!DoGuarding())
	exit(1);
      if (childlimits && !SetChildLimits())
	exit(1);
    }

    if (!guarding)
      StorePidToFile();

    bool create_terminal = false;

    if (!start_online && !start_analyse)
      SolveStartUpMode();

    if (!start_analyse && (listen_porta||create_terminal||soap_port))
      start_online = true;

    StartTimes();        // shouldn't this be called earlier ?
    SolveVersionData();  // this used to come earlier...

    // Some initial logging:
    WriteLogVersion();
    WriteLogProcessInfo();

    // Force initialization of function static variables...
    SVM::allowedParam("");

#ifdef PICSOM_USE_PTHREADS
    RegisterThread(main_thread, gettid(), "picsom", "main", NULL, NULL);
#endif // PICSOM_USE_PTHREADS

    ReadQueries();
    DumpQueries();

    if (start_online && create_terminal && HasTerminalInput() && !runs_in_bg)
      CreateTerminalConnection();

    if (HandleSignals() && !start_version_only)
      SetSignalHandlers(runs_in_bg);

    if (soap_port)
      soap_server = CreateSoapServerConnection(soap_port);

    if (listen_porta)
      SetUpListenConnection(false /*expect_slaves*/,
			    false /*expect_slaves*/, "/");

    ReadSecrets();

    PossiblyShowDebugInformation("After PicSOM construction");
  }

  /////////////////////////////////////////////////////////////////////////////

  PicSOM::~PicSOM() {
    size_t ssub = 0, ssta = 0, ster = 0, stas = 0, sres = 0;
    vector<string> limbos;

    CancelSlaveStatusThread();
    
    RwLockWriteSlaveList();

    size_t j = 0;
    for (auto& i : slave_list) {
      if (debug_slaves)
	cout << SlaveInfoString(i, false) << endl;
      if (i.hostname=="" && i.status.find("_wait")!=string::npos)
	ssub += i.n_slaves_started;
      if (!IsTimeZero(i.started))
	ssta++;
      if (i.status=="terminated")
	ster++;
      if (i.status=="limbo")
	limbos.push_back(i.hostname);

      stas += i.n_tasks_tot;
      sres += i.n_tasks_fin;

      UpdateSlaveInfo(true);

      bool skip = i.Locked() && !i.ThisThreadLocks();
      cout << "TerminateSlave(" << i.n_slaves_started << "/"
	   << i.hostname << ") " << j++ << "/" << slave_list.size()
	   << " skip=" << (skip?"true":"false") << endl;

      if (!skip)
	TerminateSlave(i, "it's the time to go", 0);
    }

    RwUnlockWriteSlaveList();

    if (ssub) {
      ostringstream ss;
      ss << "Submitted " << ssub << " slaves, " << ssta << " started, "
	 << ster << " terminated, " << limbos.size() << " in limbo "
	 << stas << " tasks started, " << sres << " finished";
      WriteLog(ss);
      if (limbos.size())
	WriteLog("  in limbo: "+ToStr(limbos));
    }

    ostringstream msg;
    msg << "Destructing " << analysis.size() << " analyses "
	<< Connections(false, false) << " connections "
	<< Nqueries() << " queries "
	<< database.Nitems() << " databases";
    WriteLog(msg.str());

    for (size_t i=0; i<analysis.size(); i++)
      delete analysis[i];

    for (int i=0; i<database.Nitems(); i++) {
      GetDataBase(i)->JoinAsyncAnalysesDeprecated();
      GetDataBase(i)->CloseTextIndices();
    }

#ifdef PICSOM_USE_PTHREADS
    if (HasAnalysisThread() && !IsAnalysisThread()) {
      if (pthread_cancel(analysis_thread)) //obs! should we wait?
	ShowError("cancelling analysis_thread failed");
      analysis_thread_set = false;
    }
#endif // PICSOM_USE_PTHREADS

    for (size_t i=0; i<Connections(false, false); i++)
      delete GetConnection(i);

    RwLockWriteQueryList();
    for (size_t i=0; i<Nqueries(); i++)
      delete GetQuery(i);
    RwUnlockWriteQueryList();

    database.FullDelete();

    WriteLog("All databases closed");
    
    if (temp_dir_personal!="" && !keep_temp) {
      WriteLog("Removing "+TempDirPersonal());
      RmDirRecursive(TempDirPersonal());
    }

    //quiet = false;
    PossiblyShowDebugInformation("Exiting", true);

    if (atexit_instance==this)
      AtExitInstance(NULL);

    if (streambuf_cout)
      cout.rdbuf(streambuf_cout);

    if (streambuf_cerr)
      cerr.rdbuf(streambuf_cerr);

    if (streambuf_clog)
      clog.rdbuf(streambuf_clog);

    xmlCleanupParser();

    WriteLog("Bye");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CleanUpBogusDirs() {
    bool wait_rm = false;

    list<string> dd { "/ramdisk", "/tmp", "/local", "/l", "/users" };

    list<string> dirs;

#ifdef HAVE_DIRENT_H
    // This is not effective anymore...
    string slurmdir = "/local";
    DIR *d = opendir(slurmdir.c_str());

    for (dirent *dp; d && (dp = readdir(d)); ) {
      string name = dp->d_name;
      if (name.find("slurm-")!=0)
	continue;

      string n = slurmdir+"/"+name;
      struct stat sbuf;
      if (stat(n.c_str(), &sbuf))
	continue;

      if (sbuf.st_uid!=getuid())
	continue;      

      dd.push_back(n);
    }
    if (d)
      closedir(d);

    for (auto dname=dd.begin(); dname!=dd.end(); dname++) {
      DIR *d = opendir(dname->c_str());

      for (dirent *dp; d && (dp = readdir(d)); ) {
	string name = dp->d_name;
	vector<string> p = SplitInSomething("-", false, name);
	if (p.size()!=3 || p[0]!=UserName() || p[1]!="picsom")
	  continue;

	pid_t pid = atoi(p[2].c_str());
	if (pid==getpid())
	  continue;

	string t = "/proc/"+p[2]+"/limits";
	string s = FileToString(t);
	if (s!="")
	  continue;

	dirs.push_back(*dname+"/"+name);
      }
      if (d)
	closedir(d);
    }
#endif // HAVE_DIRENT_H

    bool ok = true;
    if (wait_rm) {
      if (dirs.size()) {
	WriteLog("Cleaning up bogus "+ToStr(dirs));
	for (auto d=dirs.begin(); d!=dirs.end(); d++)
	  if (!RmDirRecursive(*d))
	    ok = ShowError("Failed to remove directory <"+*d+">");
      }

    } else {
      if (dirs.size()) {
	vector<string> cmd;
	cmd.push_back("/bin/rm");
	cmd.push_back("-rf");
	cmd.insert(cmd.end(), dirs.begin(), dirs.end());
	cmd.push_back("1>/dev/null");
	cmd.push_back("2>&1");
	cmd.push_back("&");
	WriteLog("Cleaning up in background bogus "+ToStr(dirs));
	ExecuteSystem(cmd, true, false, false);
      }
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::Main(int argc, const char **argv, PicSOM **pp) {
    vector<string> args;
    for (int i=0; i<argc; i++)
      args.push_back(argv[i]);

    return Main(args, pp);
  }
  
  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::Main(const vector<string>& args, PicSOM **pp) {
    if (pp)
      *pp = NULL;
    // vector<string> aa; 
    // PicSOM picsomx(aa);
    // Analysis ana(&picsomx, NULL, NULL, vector<string>());
    // ana.AnalyseEnv(vector<string>());

#ifdef HAVE_WINSOCK2_H
    WSADATA lpWSAData;
    int r = WSAStartup(MAKEWORD(2, 0), &lpWSAData);
    if (Connection::DebugSockets())  // this is seldom true...
      cout << "PicSOM::Main() : WSAStartup() returned r=" << r << endl;
    if (r) {
      cerr << "WSAStartup() returned " << r << endl;
      exit(1);
    }
#endif // HAVE_WINSOCK2_H

#ifdef PICSOM_USE_MPI
    setenv("OMPI_MCA_mpi_warn_on_fork", "0", 1);
    MPI::Init();
#endif // PICSOM_USE_MPI

#if defined(HAVE_CAFFE2_CORE_MACROS_H) && defined(PICSOM_USE_CAFFE2)
    if (true) {
      int argc = 1;
      char** argv = new char*[2];
      argv[0] = new char[10];
      strcpy(argv[0], "hello");
      argv[1] = NULL;
      caffe2::GlobalInit(&argc, &argv);
      caffe2_initialized = true;
    }
#endif // HAVE_CAFFE2_CORE_MACROS_H && PICSOM_USE_CAFFE2

    Connection::InitializeSSL();

    ostream& errout = cerr;
    // errout << "STARTING"<< endl;
    // Simple::TrapAfterError(true);

    try {
      vector<string> psargs { args[0] };
      size_t ai = 1;
      string mode;
      
      if (args.size()>1) {
	const string& a = args[1];
      
	if (a[0]=='-' && a.size()==2)
	  mode = a.substr(1);
      }
      
      if (mode=="f" || mode=="s" || mode=="r" || mode=="p")
	ai = 2;	// remove the mode parameter

      for (; ai<args.size(); ai++)
	psargs.push_back(args[ai]);

      if (mode=="f") {
	// this sets videofile::_local_bin and Feature::picsomroot
	vector<string> aa { "*dummy*executable*", "-q" }; 
	PicSOM picsomx(aa);
	return Feature::Main(psargs);
      }
	    
      if (mode=="s")
	return Segmentation::Main(psargs);
      
      if (mode=="r")
	return Usage(args[0], "imgrobot not included");
      
      if (mode=="h" || mode=="H" || mode=="?")
	return Usage(args[0]);

      if (mode!="p" && mode!="c" && mode!="i" && mode!="a" && 
	  mode!="v" && mode!="q" && mode!="")
	return Usage(args[0], "Unknown PicSOM Mode");
      
      PicSOM *p = new PicSOM(psargs); 
      if (pp) {
	*pp = p;
	return 0;
      }

      int r = p->Execute(0, NULL);
      delete p;

      Connection::ShutdownSSL();

      return r;
    }
    catch (const string& s) {
      errout << "PicSOM::Main(): ERROR <" << s << ">" << endl;
      return 31;
    }
    catch (const char* s) {
      errout << "PicSOM::Main(): ERROR <" << s << ">" << endl;
      return 34;
    }
    catch (const std::exception& ex) {
      errout << "PicSOM::Main(): ERROR [" << ex.what() << "]" << endl;
      return 32;
    }
    catch (...) {
      errout << "PicSOM::Main(): ERROR ..." << endl;
      return 33;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::Usage(const string& prog, const string& msg) {
    if (msg.length() > 0)
      cerr << "ERROR: " << msg << endl;
    cerr << "USAGE: " << prog << " [<-mode>]" << endl;
    cerr << "Where <-mode> is:" << endl;
    cerr << "  -p or no mode switch : normal picsom operation" << endl;
    cerr << "  -f : feature extraction mode" << endl;
    cerr << "  -s : segmentation mode" << endl;
    cerr << "  -h : show this help text" << endl;
    return 2;
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::CreateEnvironment() {
#if HAVE_DECL_ENVIRON
    char **e = environ;
#else
    char **e = NULL; // SunOS/MacOS...
#endif //  HAVE_DECL_ENVIRON

    set<string> vars;
    vars.insert("PATH");
    vars.insert("LD_LIBRARY_PATH");
    vars.insert("LD_PRELOAD");
    vars.insert("PYTHONPATH");
    vars.insert("LOADEDMODULES");
    vars.insert("CUDA_VISIBLE_DEVICES");
    vars.insert("JOB_ID");
    vars.insert("JOB_NAME");
    vars.insert("QUEUE");
    vars.insert("REQUEST");
    // vars.insert("SLURM_JOB_ID");
    // vars.insert("SLURM_JOB_NAME");
    // vars.insert("SLURM_MEM_PER_CPU");
    vars.insert("_CONDOR_SCRATCH_DIR");
    vars.insert("_CONDOR_JOB_AD");
    vars.insert("_CONDOR_MACHINE_AD");
    vars.insert("_CONDOR_JOB_ID"); // home made

    env.clear();
    while (e && *e) {
      string v = *(e++);
      try {
        pair<string,string> kv = SplitKeyEqualValueNew(v);
        string key = kv.first;
        if (key.find("PICSOM_")==0 || key.find("SLURM_")==0 ||
	    vars.find(key)!=vars.end()) {
          if (key.find("PICSOM_")==0)
            key.erase(0, 7);
          env[key] = kv.second;
        }
      } catch (...) {}
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::ListenPort(const string& n, bool only_one) {
    int a = PICSOM_DEFAULT_PORT, b = a+10;
#ifdef PICSOM_DEVELOPMENT
    bool r = UserPorts(n, a, b);
#else
    bool r = n!="*dummy*";
#endif // PICSOM_DEVELOPMENT
    if (r)
      ListenPort(a, only_one ? a : b);

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::StorePidToFile() const {
    if (pidfile!="") {
      ofstream pidf(pidfile);
      pidf << getpid() << endl;

      return pidf.good();
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::DoGuarding() {
    if (!listen_porta || listen_porta!=listen_portb) {
      ShowError("DoGuarding() can guard only one port");
      return false;
    }

    StorePidToFile();

    char msghead[1000], msgtail[] = "\n\n";
    sprintf(msghead, "\n+++GUARD+++ process pid %d ", getpid());  
    WriteLog(msghead, "starting", msgtail);

    SetSignalHandler(SIGINT,  &PicSOM::LogSignalAndContinue);
    SetSignalHandler(SIGTERM, &PicSOM::LogSignalAndContinue);
    SetSignalHandler(SIGABRT, &PicSOM::LogSignalAndContinue);
    SetSignalHandler(SIGFPE,  &PicSOM::LogSignalAndContinue);
    SetSignalHandler(SIGILL,  &PicSOM::LogSignalAndContinue);
    SetSignalHandler(SIGSEGV, &PicSOM::LogSignalAndContinue);

#ifdef SIGXCPU
    SetSignalHandler(SIGXCPU, &PicSOM::LogSignalAndContinue);
#endif // SIGXCPU
#ifdef SIGPIPE
    SetSignalHandler(SIGPIPE, &PicSOM::LogSignalAndContinue);
#endif // SIGPIPE
#ifdef SIGHUP
    SetSignalHandler(SIGHUP,  &PicSOM::LogSignalAndContinue);
#endif // SIGHUP
#ifdef SIGUSR1
    SetSignalHandler(SIGUSR1, &PicSOM::LogSignalAndContinue);
#endif // SIGUSR1
#ifdef SIGUSR2
    SetSignalHandler(SIGUSR2, &PicSOM::LogSignalAndContinue);
#endif // SIGUSR2
#ifdef SIGBUS
    SetSignalHandler(SIGBUS,  &PicSOM::LogSignalAndContinue);
#endif // SIGBUS
#ifdef SIGEMT
    SetSignalHandler(SIGEMT,  &PicSOM::LogSignalAndContinue);
#endif // SIGEMT

    const int max_forks = 20;
    int fork_count = 0;

    bool debug = false;
    const int mini_sleep = 1, short_sleep = 15, med_sleep = 60, long_sleep = 300;
    int next_sleep = mini_sleep;

    for (;;) {
      if (fork_count>=max_forks) {
	WriteLog(msghead, "stopped by exceeded fork count", msgtail);
	return false;
      }

      if (sleep(next_sleep)) {
	WriteLog(msghead, "being stopped by external signal", msgtail);
	if (guarded_pid!=-1) {
	  int last_sleep = 5;
	  WriteLog(msghead, "killing guarded process "+ToStr(guarded_pid));
	  kill(guarded_pid, SIGTERM);
	  WriteLog(msghead, "sleeping "+ToStr(last_sleep)+" seconds...");
	  sleep(last_sleep);
	}
	return false;
      }

      next_sleep = short_sleep;

      char cmd[1000];
      sprintf(cmd, "netstat -a | grep \"[\\\\.:]%d \"", listen_porta);

      bool found = false;

      FILE *netstat = popen(cmd, "r");
      if (!netstat) {
	next_sleep = med_sleep;
	continue;
      }

      if (debug)
	cout << "DoGuarding() opened popen(" << cmd << ")" << endl;

      while (!feof(netstat)) {
	char line[10000];
	if (fgets(line, sizeof line, netstat)) {
	  found = true;
	  if (debug)
	    cout << "DoGuarding() read line : " << line << flush;
	}
      }

      if (pclose(netstat)==-1) {
	next_sleep = med_sleep;
	continue;
      }
    
      if (debug)
	cout << "DoGuarding() closed pipe " << endl;

      if (found)
	continue;

      if (debug)
	cout << "DoGuarding() now fork()ing ..." << endl;

      pid_t pid = fork();
      fork_count++;

      if (pid==-1) {
	ShowError("DoGuarding() fork() failed");
	next_sleep = med_sleep;
	continue;
      }

      if (pid==0) {
	sleep(mini_sleep);
	return true;
      }

      char tmp[100];
      sprintf(tmp, "spawned child pid %d, fork #%d", pid, fork_count);
      WriteLog(msghead, tmp, msgtail);

      guarded_pid = pid;

      next_sleep = long_sleep;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetChildLimits() {
#ifdef HAVE_SYS_RESOURCE_H
    rlim_t mega = 1024L*1024L, giga = 1024L*mega, hour = 3600;
    rlim_t rlimit_core = 0, rlimit_cpu = 100*hour, rlimit_as = 8*giga;
    rlimit r, m;
  
    char tmp[300];

    errno = 0;
    r.rlim_cur = r.rlim_max = rlimit_core;
    if (setrlimit(RLIMIT_CORE, &r)) {
      int err = errno;
      getrlimit(RLIMIT_CORE, &m);
      sprintf(tmp, "%s %ld > %ld", strerror(err), (long)r.rlim_cur,
	      (long)m.rlim_max);
      return ShowError("SetChildLimits() failed with RLIMIT_CORE ", tmp);
    }

    r.rlim_cur = r.rlim_max = rlimit_cpu;
    if (setrlimit(RLIMIT_CPU, &r)) {
      int err = errno;
      getrlimit(RLIMIT_CPU, &m);
      sprintf(tmp, "%s %ld > %ld", strerror(err), (long)r.rlim_cur,
	      (long)m.rlim_max);
      return ShowError("SetChildLimits() failed with RLIMIT_CPU ", tmp);
    }

    r.rlim_cur = r.rlim_max = rlimit_as;
    if (setrlimit(RLIMIT_AS, &r)) {
      int err = errno;
      getrlimit(RLIMIT_AS, &m);
      sprintf(tmp, "%s %ld > %ld", strerror(err), (long)r.rlim_cur,
	      (long)m.rlim_max);
      return ShowError("SetChildLimits() failed with RLIMIT_AS ", tmp);
    }

    sprintf(tmp, "core = %lu bytes, cpu = %lu seconds, as = %lu bytes",
	    (long unsigned)rlimit_core, (long unsigned)rlimit_cpu,
	    (long unsigned)rlimit_as);
    WriteLog("Restricted server process to ", tmp);
#endif // HAVE_SYS_RESOURCE_H

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetUnLimited() {
    SetLimit("as",     "unlimited");
    SetLimit("nofile", "unlimited");
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetLimit(const string& n, const string& v) {
#ifdef HAVE_SYS_RESOURCE_H
    string msg = "SetLimit("+n+"="+v+") : ";

    rlim_t val  = 0;

    int resource = 0;

    bool ok = true;
    if (n=="as") {
      resource = RLIMIT_AS;
      val = FromSuffixStr(v);
      if (val<(rlim_t)FromSuffixStr("100M"))
	return ShowError(msg+ToStr(val)+" is too small");

    } else if (n=="core") {
      resource = RLIMIT_CORE;
      val = FromSuffixStr(v);
  
    } else if (n=="nofile") {
      resource = RLIMIT_NOFILE;
      val = FromSuffixStr(v);
  
    } else if (n=="cpu") {
      resource = RLIMIT_CPU;
      val = atol(v.c_str());
      if (v.size()) {
	if (v[v.size()-1]=='m')
	  val *= 60;
	if (v[v.size()-1]=='h')
	  val *= 3600;
	if (v[v.size()-1]=='d')
	  val *= 24L*3600;
      }
 
    } else if (n=="nproc") {
      resource = RLIMIT_NPROC;
      val = FromSuffixStr(v);

    } else 
      ok = false;

    if (!ok)
      return ShowError(msg+"resource name not understood");

    rlimit r, m;
    getrlimit(resource, &m);
    if (m.rlim_max<val)
      val = m.rlim_max;
    r.rlim_max = r.rlim_cur = val;
    errno = 0;

    if (setrlimit(resource, &r)) {
      char tmp[300];
      int err = errno;
      getrlimit(resource, &m);
      sprintf(tmp, "%s %ld > %ld", strerror(err), (long)r.rlim_cur,
	      (long)m.rlim_max);
      return ShowError(msg+"failed with "+v+"="+ToStr(val)+" : "+tmp);
    }

    ostringstream ss;
    ss << "Limited resource \""+n+"\" to "+v;
    if (val!=RLIM_INFINITY)
      ss << "=" << val;

    WriteLog(ss);
#endif // HAVE_SYS_RESOURCE_H

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::DoSshForwarding(int p) {
// triton proxy: ssh -N -L 5612:triton.aalto.fi:5610 jormal@triton.aalto.fi

  if (ssh_forwarding.empty())
    return true;

  string::size_type co = ssh_forwarding.find(':');
  string::size_type at = ssh_forwarding.find('@');

  if (at==string::npos)
    at = co;

  if (co==string::npos || at<co || co==0 || at==ssh_forwarding.size()-1)
    return ShowError("SSH port forwarding should look like 'port:user@host'"
		     " or 'port:host'");

  string port = ssh_forwarding.substr(0, co);
  string user = ssh_forwarding.substr(co+1, at>co?at-co-1:0);
  string host = ssh_forwarding.substr(at+1);

  stringstream pstr;
  pstr << p;

  // commercial-SSH : -f
  // openSSH        : -f -N

  string allowed_hosts = "localhost"; // "*"
  vector<string> cmd;
  cmd.push_back("ssh");
  cmd.push_back("-f");
  cmd.push_back("-R "+port+":"+allowed_hosts+":"+pstr.str());

  cmd.push_back((user.length()?user+"@":"")+host);

  WriteLog("Preparing SSH port forwarding[", ToStr(cmd), "]");

  int s = ExecuteSystem(cmd, false, false, false);
  if (s==-1)
    return ShowError("SSH port forwarding [", ssh_forwarding, "] failed");

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::IsRootlessSlave() const {
    // return MasterAddress()!="";
    return false; // ugly hack for the time being...
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::HasFreeSlaves() const {
    RwLockReadSlaveList();

    for (auto i=slave_list.begin(); i!=slave_list.end(); i++) {
      cout << SlaveInfoString(*i, true) << endl;
      string s = i->hostspec;
      size_t p0 = s.find('(');
      if (p0!=string::npos) {
	size_t p1 = s.find(')', p0);
	if (p1!=string::npos) {
	  vector<string> a
	    = SplitInCommasObeyParentheses(s.substr(p0+1, p1-p0-1));
	  for (size_t i=0; i<a.size(); i++)
	    if (a[i]=="free") {
	      RwUnlockReadSlaveList();
	      return true;
	    }
	}
      }
    }

    RwUnlockReadSlaveList();

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::PrepareSlave(const string& spec) {
    // DebugInterpret(true);
    // TrapAfterError(true);
    // BackTraceBeforeTrap(true);
    
    bool default_quiet = true;

    is_slave   = true;
    use_slaves = false;
    string slave_key;
    string addr;

    if (spec.empty()) {    
      quiet = default_quiet;
    } else {
      vector<string> specs = SplitInCommas(spec);
      vector<string>::const_iterator i=specs.begin();
      addr = *i;
      for (i++; i!=specs.end(); i++)
	if (*i=="jam")
	  Simple::JamAfterError(true);

	else if (i->find("slavekey=",0) == 0) {
	  quiet = default_quiet;
	  slave_key = i->substr(9);
	  
	} else if (*i=="noroot") {
	  norootdir = true;
	  
	} else if (*i=="debug") {
	  quiet = false;
	  Connection::DebugHTTP(4);
	  Connection::DebugReads(true);
	  Connection::DebugWrites(1);

	} else
	  return ShowError("PrepareSlave("+spec+"): unknown specs: "+*i);
    }
    Simple::ErrorExtraText("=slave="+HostName()+"=");

    // LoggingMode();
    // SetLogging();
    
    start_online = true;
    
    Connection *conn = NULL;
    if (addr=="")
      conn = CreateTerminalConnection();
    else {
      if (addr.find("%3Btcp://")!=string::npos) {
	string a = Connection::UnEscapeMPIport(addr);
	conn = CreateMPIConnection(a, true);
      } else {
	conn = CreateUplinkConnection(addr, true, true);
	// we should check that pids match or something and die gracefully...
      }
    }

    if (!conn)
      return ShowError("PrepareSlave("+spec+") failed");

    if (spec=="") {
      conn->XML(true);
      conn->SendAcknowledgment(map<string,string>());

    } else {
      master_address = "http://"+addr;
      master_auth_token = "hattifnattar";

      string jobid = "???";
      if (env.find("SLURM_JOB_ID")!=env.end())
	jobid = env["SLURM_JOB_ID"];
      if (env.find("_CONDOR_JOB_ID")!=env.end())
	jobid = env["_CONDOR_JOB_ID"];
      XmlDom xml = XmlDom::Doc("picsom:slaveoffer");
      XmlDom offer = xml.Root("slaveoffer");
      AddToXMLstatus(offer);
      offer.Element("slavekey", slave_key);
      offer.Element("jobid",    jobid);
      conn->XML(true);
      conn->WriteOutXml(xml);
//       bool dummy;
//       conn->ReadAndParseXML(dummy, false);
      // we should check something and die gracefully...
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::ShowSlaveInfoList() const {
    RwLockReadSlaveList();
    for (auto s=slave_list.begin(); s!=slave_list.end(); s++)
      cout << SlaveInfoString(*s, true) << endl;
    RwUnlockReadSlaveList();
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::SlaveInfoString(const slave_info_t& slave, bool thrs) const {
    stringstream ss, cmd, td1, td2, td3;

    for (size_t i=0; i<slave.command.size(); i++)
      cmd << "[" << slave.command[i] << "]";

    bool started   = !IsTimeZero(slave.started);
    struct timespec now = TimeNow();
    size_t s_used  = started ? TimeDiff(now, slave.started) : 0;
    int    s_left  = slave.max_time-s_used;
    int    s_leftx = s_left>0 ? s_left : -s_left;
    string maxtime = SbatchTimeFromSec(slave.max_time);
    string used    = SbatchTimeFromSec(s_used);
    string left    = (s_left<0?"-":"")+SbatchTimeFromSec(s_leftx);

    string ltime;
    if (!IsTimeZero(slave.task_started)) {
      if (!IsTimeZero(slave.task_finished))
	ltime = SbatchTimeFromSec((size_t)ceil(TimeDiff(slave.task_finished,
							slave.task_started)));
      else
	ltime = SbatchTimeFromSec((size_t)ceil(TimeDiff(now,
							slave.task_started)))
	  +"+";
    }

    td1 << " submitted=" << TimeString(slave.submitted) 
	<< " started=" << TimeString(slave.started) 
	<< " time max=" << maxtime
	<< " used=" << used;
    if (slave.max_time)
      td1 << " left=" << left;
    td2 << " last started=" << TimeString(slave.task_started)
	<< " finished=" << TimeString(slave.task_finished)
	<< " time=" << ltime;

    double diff_u = TimeDiff(slave.updated, now);
    double diff_s = TimeDiff(slave.spare,   now);
    td3.precision(4);
    td3 << " updated=" << TimeString(slave.updated) 
	<< " (now" << (diff_u>=0?"+":"") << diff_u << "s)"
	<< " spare=" << TimeString(slave.spare) 
	<< " (now" << (diff_s>=0?"+":"") << diff_s << "s)";

    string lockmsg = "not-locked";
    if (slave.Locked())
      lockmsg = "LOCKED by "+slave.LockerStr();

    ss << lockmsg << " hostspec=" << slave.hostspec
       << endl << "      "
       << " parent=" << (slave.parent?"yes":"no")
       << " status=" << slave.status
       << " keys=" << ToStr(slave.keys)
       << " jobids=" << ToStr(slave.jobids)
       << " hostname=" << slave.hostname
       << " pid=" << slave.pid
      // << " executable=" << slave.executable
      // << " command=" << cmd.str() 
       << " conn=" << (slave.conn?slave.conn->StateString():"nil")
       << " load=" << slave.load 
       << " cpus=" << slave.cpucount 
       << " usage=" << slave.cpuusage
       << " returned_empty=" << int(slave.returned_empty)
       << endl << "      "
       << " n_slaves_now=" << slave.n_slaves_now
       << " n_slaves_started=" << slave.n_slaves_started
       << " n_slaves_contacted=" << slave.n_slaves_contacted
       << " max_slaves_start=" << slave.max_slaves_start
       << " max_slaves_add=" << slave.max_slaves_add
       << endl << "      "
       << " n_tasks_par=" << slave.n_tasks_par
       << " n_tasks_tot=" << slave.n_tasks_tot
       << " n_tasks_fin=" << slave.n_tasks_fin
       << " max_tasks_par=" << slave.max_tasks_par
       << " max_tasks_tot=" << slave.max_tasks_tot
       << " inrun=" << slave.running_threads 
       << " pending=" << slave.pending_threads
       << endl << "      "
       << td1.str()
       << endl << "      "
       << td2.str()
       << endl << "      "
       << td3.str();
      // << " connection=" << (slave.conn?slave.conn->Identity():"NULL");

#ifdef PICSOM_USE_PTHREADS
    if (thrs) {
      if (slave.threads.size()==0)
	ss << " no threads";
      for (thread_list_t::const_iterator i=slave.threads.begin();
	   i!=slave.threads.end(); i++)
	ss << endl << "   " << ThreadInfoString(*i, true);
    }
#endif // PICSOM_USE_PTHREADS

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetSlaveInfo(const string& hosts) {
    if (!use_slaves) {
      WriteLog("Use of slaves prevented.");
      return true;
    }

    bool force_fast = ForceFastSlave();

    vector<string> l = SplitInCommasObeyParentheses(hosts);
    if (l.size()==1 && hosts.find(' ')!=string::npos)
      l = SplitInSpaces(hosts);

    const string& cmd = MyBinary();
    string ext = "debug";
    if (cmd.substr(cmd.size()-4) == "fast")
      ext = "fast";
    string exe = FindExecutable("c++", "picsom", ext); // obs! doesn't work
    if (exe=="")
      exe = cmd;

    if (force_fast) {
      size_t p = exe.find(".debug");
      if (p!=string::npos)
	exe.replace(p, 6, ".fast");
    }

    for (size_t i=0; i<l.size(); i++) {
      bool exists = false;
      size_t sno = 0;
      RwLockReadSlaveList();
      for (auto s=slave_list.begin(); s!=slave_list.end(); s++, sno++)
	if (s->hostspec==l[i]) {
	  exists = true;
	  break;
	}
      RwUnlockReadSlaveList();

      slave_info_t s(l[i], exe);

      if (debug_slaves)
	WriteLog("Slave #"+ToStr(sno)+": "+SlaveInfoString(s, true)
		 +(exists?" already EXISTS":" DEFINED"));
      if (!exists) {
	RwLockWriteSlaveList();
	slave_list.push_back(s);
	RwUnlockWriteSlaveList();
      }
    }

    return !slave_list.empty();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::StartSlaves(size_t nmax) {
    RwLockWriteSlaveList();

    for (auto i=slave_list.begin(); i!=slave_list.end(); i++)
      if (i->status=="uninit") {
	string sspec = ToStr(i->max_slaves_start)+"+"+ToStr(i->max_slaves_add);
	if (nmax) {
	  if (nmax<i->max_slaves_start) {
	    i->max_slaves_start = nmax;
	    i->max_slaves_add = 0;
	  } else {
	    size_t nmax_add = nmax-i->max_slaves_start;
	    if (nmax_add<i->max_slaves_add)
	      i->max_slaves_add = nmax_add;
	  }
	}
	sspec += " -> "+ToStr(i->max_slaves_start)+"+"+ToStr(i->max_slaves_add);
	WriteLog("Starting slaves max="+ToStr(nmax)+" "+sspec+" now running "+
		 ToStr(i->n_slaves_now)+" adding "+
		 ToStr(i->max_slaves_start-i->n_slaves_now));
	for (size_t j=i->n_slaves_now; j<i->max_slaves_start; j++) {
	  CreateSlavePartOne(*i, false, "", "");
	  RwUnlockWriteSlaveList();
	  struct timespec ts = { 0, 100000000 }; // 100 millisecond
	  nanosleep(&ts, NULL);
	  // sleep(1); // until 2014-07-26
	  RwLockWriteSlaveList();
	}
      }

    size_t n_ok = 0;
    for (auto i=slave_list.begin(); i!=slave_list.end(); i++)
      if (CreateSlavePartTwo(*i))
	n_ok++;

    if (!n_ok)
      ShowError("StartSlaves() failed to start any slave of "+
		ToStr(slave_list.size())+" choices");

    RwUnlockWriteSlaveList();

    return n_ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CreateSlave(slave_info_t& slave, bool do_submit,
			   const string& id, const string& cmdline) {
    return CreateSlavePartOne(slave, do_submit, id, cmdline)
      && CreateSlavePartTwo(slave);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CreateSlavePartOne(slave_info_t& slave, bool do_submit,
				  const string& /*id*/,
				  const string& cmdline) {
    string msg = "CreateSlavePartOne() : ";

    bool condor_fn01_to_pc45 = false;
    bool use_sbatch_exclude = true;

    string special, random_key;
    bool do_bg = false, free = false;
    bool keeptmp = true; // debug_slaves;
    bool debug_submit = false;
    bool trick_5h = false;  // since 2014-07-26

    slave.command.clear();
    
    // triton defaults are: 00:15:00       2000 (M)   smallmem
    set<string> sbatch_keys { "time", "mem-per-cpu", "partition", "qos",
	"N", "n", "exclusive", "constraint", "gres", "debug" };
    set<string> condor_keys { "request_memory" };
    list<pair<string,string>> sbatch_keyval;
    map<string,string> condor_keyval;
    condor_keyval["request_memory"] = "1024"; // 1 GB

    const string& hostspec = slave.hostspec;
    string hostpart = hostspec;
    if (hostspec!="" && hostspec!="-" && hostspec!="*") {
      string args;
      if (SplitParentheses(hostspec, hostpart, args)) {
	vector<string> argv = SplitInCommas(args);
	const string& a = argv[0];
	if (a=="qsub" || a=="back" || a=="condor" || a=="sbatch")
	  special = a;
	else
	  return ShowError(msg+"Unknown slave host argument: "+args);

	if (special!="") {
	  for (size_t i=1; i<argv.size(); i++) {
	    const string& keyval = argv[i];
	    if (keyval.find('=')!=string::npos) {
	      pair<string,string> kv = SplitKeyEqualValueNew(keyval);
	      const string& akey = kv.first;
	      const string& aval = kv.second;

	      if (akey=="debug") {
		debug_submit = true;

	      } else if (akey=="maxslaves") {
		int val = atoi(aval.c_str());
		if (val<1)
		  return ShowError(msg+"Bad value for argument key: " + akey
				   + "=" + aval);

		slave.max_slaves_start = (size_t)val;
		size_t add = 0, p = aval.find('+');
		if (p!=string::npos)
		  add = (size_t)atoi(aval.substr(p+1).c_str());
		slave.max_slaves_add = add;

		// WriteLog("Set max slaves to "+ToStr(val));

	      } else if (akey=="maxtaskspar") {
		int val = atoi(aval.c_str());
		if (val<0)
		  return ShowError(msg+"Bad value for argument key: " + akey
				   + "=" + aval);

		slave.max_tasks_par = (size_t)val;
		// WriteLog("Set max parallel tasks to "+ToStr(val));

	      } else if (akey=="maxtasks") {
		int val = atoi(aval.c_str());
		if (val<0)
		  return ShowError(msg+"Bad value for argument key: " + akey
				   + "=" + aval);

		slave.max_tasks_tot = (size_t)val;
		// WriteLog("Set max total tasks to "+ToStr(val));

	      } else if (special=="sbatch" &&
			 sbatch_keys.find(akey)!=sbatch_keys.end()) {
		if (akey=="time") {
		  slave.max_time = SbatchTimeToSec(aval);
		  if (slave.max_time==4*3600&&trick_5h)
		    slave.max_time = 5*3600;
		}

		sbatch_keyval.push_back(make_pair(akey, aval));

	      } else if (special=="condor" &&
			 condor_keys.find(akey)!=condor_keys.end()) {
		condor_keyval[akey] = aval;

	      } else
		return ShowError(msg+"Unknown argument key: " + akey);

	    } else if (keyval=="free") {
	      free = true;

	    } else
	      return ShowError(msg+"Unknown argument: " + keyval);
	  }

	  if (!free) {
	    // obs! this is a strange place to call these...
	    SetUpListenConnection(true, true);
	    if (use_mpi_slaves)
	      SetUpMPIListenConnection();
	  }
	}
      }
    }

    if (hostpart=="localhost") {
      if (special=="back")
	do_bg = true;
    } else {
      slave.command.push_back("/usr/bin/ssh");
      if (special=="back")
	slave.command.push_back("-f");
      slave.command.push_back(hostpart);
    }

    if (special!="")
      slave.status = special+(free?(do_submit?"_free":"_submit"):"_wait");

    RandVar r;
    char sk[10];
    sprintf(sk,"%06X", r.RandomInt(16777215));
    random_key = string(sk);

    vector<string> exec;
    //exec.push_back("/usr/bin/srun");
    //exec.push_back("-v");
    //exec.push_back("--slurmd-debug=4");
    exec.push_back(slave.executable);
    // exec.push_back("-Dinterpret");
    // exec.push_back("-Dreads");
    // exec.push_back("-Dwrites");
    // exec.push_back("-Dselects=2");

    if (!free) {
      // exec.push_back("-rw"); // obs! this might be needed sometime
      slave.keys.push_back(random_key);
      string slave_param = "-slave";
      if (special!="") {
	slave_param += "=";
	if (listen_mpi && mpi_listen_thread_set && mpi_listen_connection)
	  slave_param += mpi_listen_connection->EscapedMPIport();
	else
	  slave_param += HostName()+":"+ToStr(listen_connection->Port());
	slave_param += ",slavekey="+random_key;
	if (debug_slaves)
	  slave_param += ",debug";
      }
      exec.push_back(slave_param);
    }

    if (special!="") {
      string logfile = TempDirPersonal()+"/slave_submit_"+random_key+".log";
      string tempfile;
      ofstream tmpf;
      bool do_execute = false;

      if (special!="back" && (!free || do_submit)) {
	tempfile = TempDirPersonal()+"/picsom_slave_"+random_key;
	tmpf.open(tempfile.c_str());
	do_execute = true;
      }

      if (special=="back") {
	slave.command.insert(slave.command.end(), exec.begin(), exec.end());
	do_execute = true;
      }

      if (do_execute && special=="qsub") {
	tmpf << "#! /bin/sh" << endl;
	tmpf << "#$ -N picsom-slave-" << random_key << endl;
	tmpf << "#$ -cwd" << endl;
	for (vector<string>::const_iterator it=exec.begin();
	     it!=exec.end(); it++)
	  tmpf << *it << " ";
	tmpf << endl;

	slave.command.push_back("qsub");
	if (!debug_slaves) {
	  slave.command.push_back("-e /dev/null");
	  slave.command.push_back("-o /dev/null");
	}
	slave.command.push_back("<");
	slave.command.push_back(tempfile);
      }

      if (do_execute && special=="condor") {
	string outd = Cwd(); // UserHomeDir() || TempDirPersonal()
	string out = outd+"/picsom_slave_"+random_key;
	if (keeptmp)
	  WriteLog("Condor logs are written in "+out+".{log,out,err}");

	size_t request_memory = atoi(condor_keyval["request_memory"].c_str());
	tmpf << "requirements   = ICSLinux == 2014" << endl;
	tmpf << "notification   = Never" << endl;
	tmpf << "log            = " << (keeptmp ? out+".log" : "/dev/null") << endl;
	tmpf << "error          = " << (keeptmp ? out+".err" : "/dev/null") << endl;
	tmpf << "output         = " << (keeptmp ? out+".out" : "/dev/null") << endl;
	tmpf << "append_files   = " << (keeptmp ? out+".out" : "/dev/null") << endl;
	tmpf << "environment    = _CONDOR_JOB_ID=$(Cluster)" << endl;
	tmpf << "executable     = " << exec[0] << endl;
	tmpf << "image_size     = " << request_memory*1024 << endl;
	tmpf << "request_memory = " << request_memory << endl;
	tmpf << "arguments      =";
	if (!do_submit)
	  for (size_t it=1; it<exec.size(); it++)
	    tmpf << " " << exec[it];
	else
	  tmpf << " " << cmdline;
	tmpf << endl;
	tmpf << "queue" << endl;

	if (condor_fn01_to_pc45) {
	  size_t p = tempfile.rfind('/');
	  string remote_tempfile = "/tmp"+tempfile.substr(p);
	  vector<string> scmd {
	    "cat", tempfile, "|", "ssh", "triton", "ssh", "jorma@james",
	      "ssh", "pc45", "tee", remote_tempfile
	      };
	  scmd.push_back("1>/dev/null");
	  scmd.push_back("2>&1");
	  ExecuteSystem(scmd, true, true, true);
	  
	  slave.command.push_back("ssh");
	  slave.command.push_back("triton");
	  slave.command.push_back("ssh");
	  slave.command.push_back("jorma@james");
	  slave.command.push_back("ssh");
	  slave.command.push_back("pc45");
	  slave.command.push_back("condor_submit");
	  slave.command.push_back(remote_tempfile);
	  slave.command.push_back("1>/dev/null");
	  slave.command.push_back("2>&1");

	} else {
	  slave.command.push_back("condor_submit");
	  slave.command.push_back(tempfile);
	  slave.command.push_back("1>"+logfile);
	  slave.command.push_back("2>&1");
	}
      }
      
      if (do_execute && special=="sbatch") {
	ostream& tmpx = debug_submit ? cout : tmpf;

	string out = UserHomeDir()+"/picsom_slave_"+random_key;

	if (do_submit)
	  tmpx << "#! " << mybinary_str << endl << endl;
	else {
	  tmpx << "#!";
	  for (auto a=exec.begin(); a!=exec.end(); a++)
	    tmpx << " " << *a;
	  tmpx << endl;
	}

	if (use_sbatch_exclude) {
	  list<string> nodes = SbatchExclude();
	  if (nodes.size())
	    sbatch_keyval.push_back(make_pair("exclude", CommaJoin(nodes)));
	}

	for (auto kv=sbatch_keyval.begin(); kv!=sbatch_keyval.end(); kv++)
	  if (kv->first=="n" || kv->first=="N")
	    tmpx << "#SBATCH -" << kv->first << " " << kv->second << endl;
	  else if (kv->first=="exclusive")
	    tmpx << "#SBATCH --" << kv->first << endl;
	  else
	    tmpx << "#SBATCH --" << kv->first << "=" << kv->second << endl;
	tmpx << endl;

	if (do_submit) {
	  size_t p = 0;
	  while (true) {
	    size_t q = cmdline.find(' ', p);
	    if (q==string::npos)
	      break;
	    tmpx << cmdline.substr(p, q-p) << endl;
	    p = q+1;
	  }
	  tmpx << endl;

	} else
	  tmpx << "analyse=dummy" << endl;

 	slave.command.push_back("/usr/bin/sbatch");
 	slave.command.push_back(tempfile);
        slave.command.push_back("1>"+logfile);
        slave.command.push_back("2>&1");
      }

      tmpf.close();

      if (do_bg)
	slave.command.push_back("&");

      if (do_execute) {
	SetTimeNow(slave.submitted);
	if (!debug_submit)
	  ExecuteSystem(slave.command, true, false, true);
	else
	  cout << endl << ToStr(slave.command) << endl << endl;
	slave.n_slaves_now++;
	slave.n_slaves_started++;

	if (special=="condor" && !debug_submit) {
	  string logtxt = FileToString(logfile), jobid = "xxx";
	  if (logtxt.find("Submitting job(s).")==0) {
	    string pat = " submitted to cluster ";
	    size_t p = logtxt.find(pat);
	    if (p!=string::npos) {
	      jobid = logtxt.substr(p+pat.size());
	      size_t p = jobid.find_first_not_of("0123456789");
	      if (p!=string::npos)
		jobid.erase(p);
	      slave.jobids.push_back(jobid);
	    }
	  }
	  if (jobid=="xxx")
	    ShowError(msg+"condor_submit failed with result: "+logtxt);

	  // float sleeptime = 5;
	  // WriteLog("... condor_submit extra sleep "+ToStr(sleeptime)+" sec");
	  // NanoSleep(sleeptime);
	}
	if (special=="sbatch" && !debug_submit) {
	  string logtxt = FileToString(logfile);
	  if (logtxt.find("Submitted batch job ")==0) {
	    string jobid = logtxt.substr(20);
	    size_t p = jobid.find_first_of(" \t\n");
	    if (p!=string::npos)
	      jobid.erase(p);
	    slave.jobids.push_back(jobid);
	    WriteLog("Submitted sbatch job "+jobid);

	  } else
	    ShowError(msg+"sbatch failed with result: <"+logtxt+"> in <"+logfile+">");
	}
      }

      if (tempfile!="" && !keeptmp)
	Unlink(tempfile);

      if (logfile!="" && !keeptmp)
	Unlink(logfile);

    } else {
      slave.command.insert(slave.command.end(), exec.begin(), exec.end());
      slave.conn = CreatePipeConnection(slave.command);
      if (!slave.conn) {
	slave.status = "connect failed";
	return ShowError(msg+"CreateSlave() : creating pipe ["+
			 ToStr(slave.command)+"] failed"); 
      }
      slave.conn->XML(true);
      slave.status = "connected";
      slave.conn->CanPossiblyBeSelected(false);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CreateSlavePartTwo(slave_info_t& slave) {
    const string& hostspec = slave.hostspec;

    if (hostspec.find("(back",0)!=string::npos || 
	hostspec.find("(qsub",0)!=string::npos || 
	hostspec.find("(condor",0)!=string::npos || 
	hostspec.find("(sbatch",0)!=string::npos)
      return true;

    if (!slave.conn)
      return false;

    bool angry = false;

    string msg = "CreateSlavePartTwo() : ";

    if (!slave.conn->ReadAcknowledgment(false)) {
      slave.status = "read failed";
      string err = msg+SlaveInfoString(slave, true)+
	" ReadAcknowledgement() failed";
      if (angry)
	return ShowError(err);
      WriteLog(err);
      return false;
    }

    if (debug_slaves)
      cout << "Slave " << SlaveInfoString(slave, true) << " created" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::TakeSlave(Connection *conn, const string& slave_key,
			 const string& jobid) {
    RwLockWriteSlaveList();
    bool r = TakeSlaveInner(conn, slave_key, jobid);
    RwUnlockWriteSlaveList();
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::TakeSlaveInner(Connection *conn, const string& slave_key,
			      const string& jobid) {
    string msg = "PicSOM::TakeSlaveInner() : ";

    if (slave_key!="") {
      if (debug_slaves) {
	cout << "TakeSlaveInner(" << slave_key << "," << jobid
	     << ") starting with slaves:" << endl;
	ShowSlaveInfoList();
      }

      for (auto sp = slave_list.begin(); sp!=slave_list.end(); sp++) {
	set<string> keyset(sp->keys.begin(), sp->keys.end());
	bool key_found = keyset.find(slave_key)!=keyset.end();

	if (sp->status=="connected" && sp->conn && key_found) {
	  if (debug_slaves)
	    cout << "Slave " << SlaveInfoString(*sp, true) 
		 << " seems to match slavekey=" << slave_key << endl;
	  bool dummy, read_ok = sp->conn->ReadAndParseXML(dummy, true);
	  if (debug_slaves)
	    cout << "  read_ok=" << (int)read_ok << endl;
	  if (!read_ok) {
	    sp->status="condor_wait";
	    delete sp->conn;
	    sp->conn = NULL;
	  }
	}

	if ((sp->status=="back_wait" || sp->status=="qsub_wait" ||
	     sp->status=="condor_wait" || sp->status=="sbatch_wait") &&
	    !sp->conn && key_found) {

	  if (debug_slaves)
	    cout << "Slave " << SlaveInfoString(*sp, true) 
		 << " taken for slavekey=" << slave_key << endl;
	  
	  sp->n_slaves_contacted++;

	  // UpdateSlaveInfo(*sp);  // obs! Is this safe???

	  slave_info_t sp2(*sp);
	  sp2.parent = &*sp;
	  sp2.command.clear();
 	  sp2.keys = vector<string> { slave_key };
 	  sp2.jobids = vector<string> { jobid };
	  sp2.status = "connected";
	  sp2.n_slaves_now = 0;
	  sp2.n_slaves_started = 0;
	  sp2.n_slaves_contacted = 0;
	  sp2.max_slaves_start = 0;
	  sp2.max_slaves_add = 0;
	  sp2.conn = conn;
	  sp2.conn->CanPossiblyBeSelected(false);
	  sp2.lock.ReInit();

	  conn->Identity("slave["+slave_key+"]@"+conn->HostName());
	  conn->CanPossiblyBeSelected(false);

	  if (!use_mpi_slaves && !UpdateSlaveInfo(sp2))
	    return ShowError("UpdateSlaveInfo() failed");

	  slave_list.push_back(sp2);
	  return ConditionallyStartSlaveStatusThread();
	}
      }

      return ShowError(msg+"Slaveoffer with slavekey="+slave_key+
		       " could not be found!");
    }
    
    // if no initializing slave found, create a new one

    string hostname = conn->HostName();
    if (hostname=="")
      hostname = "noname";
    slave_info_t slave(hostname, conn);
    slave.n_slaves_contacted = 1;
    // if (!UpdateSlaveInfo(slave))
    //   return ShowError(msg+"UpdateSlaveInfo() failed");

    conn->Identity("slave@"+hostname);
    conn->CanPossiblyBeSelected(false);

    if (true || debug_slaves)
      WriteLog("Slave "+SlaveInfoString(slave, true)+" taken");

    slave_list.push_back(slave);

    return ConditionallyStartSlaveStatusThread();
  }

  /////////////////////////////////////////////////////////////////////////////

  static void *_slave_status_thread_main(void *p) {
    bool r = ((PicSOM*)p)->SlaveStatusThreadMain();
    return r ? p : NULL;
  }

  bool PicSOM::StartSlaveStatusThread() {
    string msg = "PicSOM::StartSlaveStatusThread() : ";
    
    WriteLog(msg+"starting");

    int r = pthread_create(&slavestatus_thread, NULL,
			   _slave_status_thread_main, this);

    if (r)
      return ShowError(msg+"pthread_create() failed");

    slavestatus_thread_set = true;
    
    //RegisterThread(ssthread, gettid(), "picsom", "slave-status", NULL, NULL);
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CancelSlaveStatusThread() {
#ifdef PICSOM_USE_PTHREADS
    if (HasSlaveStatusThread() && !IsSlaveStatusThread()) {
      WriteLog("Cancelling and joining SlaveStatusThread");
      if (pthread_cancel(slavestatus_thread))
	return ShowError("cancelling slavestatus_thread failed");
      
      void *retval = NULL;
      if (pthread_join(slavestatus_thread, &retval))
	return ShowError("joining slavestatus_thread failed");
      if (retval!=PTHREAD_CANCELED)
	return ShowError("joining slavestatus_thread returned strange");
      slavestatus_thread_set = false;
      return true;
    }
    return false;
#endif // PICSOM_USE_PTHREADS

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SlaveStatusThreadMain() {
    RegisterThread(pthread_self(), gettid(), "picsom", "slave-status",
		   NULL, NULL);

    string msg = "PicSOM::SlaveStatusThreadMain() : ";

    WriteLog(msg+"started");

    int old = 0;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
    for (;;) {
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
      UpdateSlaveInfo(false);
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
      sleep(1);
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  static void *_analysis_thread_main(void *p) {
    bool r = ((PicSOM*)p)->AnalysisThreadMain();
    return r ? p : NULL;
  }

  bool PicSOM::StartAnalysisThread() {
    string msg = "PicSOM::StartAnalysisThread() : ";
    
    WriteLog(msg+"starting");

    int r = pthread_create(&analysis_thread, NULL,
			   _analysis_thread_main, this);

    if (r)
      return ShowError(msg+"pthread_create() failed");

    analysis_thread_set = true;
    
    //RegisterThread(ssthread, gettid(), "picsom", "analysis", NULL, NULL);
  
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AnalysisThreadMain() {
    RegisterThread(pthread_self(), gettid(), "picsom", "analysis", NULL, NULL);

    string msg = "PicSOM::AnalysisThreadMain() : ";

    WriteLog(msg+"started");

    vector<string> args;
    Analysis *a = new Analysis(this, NULL, NULL, args);
    a->Analyse(analysis_script_list, false, true);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::RunInAnalysisThread(const script_list_t& l) {
    string msg = "PicSOM::RunInAnalysisThread() : ";
    WriteLog(msg+"started");

    ConditionallyStartAnalysisThread();
    
    RwLockWriteAnalysisScriptList();
    analysis_script_list.insert(analysis_script_list.end(),
				l.begin(), l.end());
    RwUnlockWriteAnalysisScriptList();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  PicSOM::slave_info_t *PicSOM::SelectSlave(bool many, double timeout,
					    const string& id,
					    const string& cmdline,
					    size_t nmax,
					    bool& alive) {
    alive = false;

    if (slave_list.empty())
      return NULL;

    if (slave_list.begin()->status=="uninit" && !StartSlaves(nmax))
      return NULL;

    RwLockReadSlaveList();
    PicSOM::slave_info_t *r = SelectSlaveInner(many, timeout, id, cmdline,
					       nmax, alive);
    RwUnlockReadSlaveList();
    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  PicSOM::slave_info_t *PicSOM::SelectSlaveInner(bool many, double /*timeout*/,
						 const string& id,
						 const string& cmdline,
						 size_t /*nmax*/,
						 bool& alive) {
    /// timeout is now always 0.0 ...

    int updatetime = 60;
    int submitinterval = 1;
    int defsparetime  = 10, effsparetime = defsparetime;
    double minproc = 12; // 1.25;
    bool is_submit = false;

    slave_info_t *p = NULL;

    set<string> waited_keys;

    // static bool first_call = true;
    static struct timespec prev_update = TimeZero();
    struct timespec now, when_next = prev_update;
    SetTimeNow(now);
    when_next.tv_sec += updatetime;
    bool updated = false;
    if (false && MoreRecent(now, when_next)) { // falsed 2014-12-04 (too late?)
      UpdateSlaveInfo(false);
      SetTimeNow(prev_update);
      updated = true;
    }

    size_t i = 0, sel_i = 0, n_ok = 0, n_not_spa = 0;
    for (auto j = slave_list.begin(); j!=slave_list.end(); j++, i++) {
      if (j->Locked())
	continue;

      j->RwLockIt();
      bool unlock = true;

      SetTimeNow(now);
      struct timespec old = now;
      old.tv_sec -= updatetime;
      bool ter = j->status=="terminated" || j->status=="limbo";
      bool spa = !ter && MoreRecent(j->spare, now);
      bool upd = !ter && !spa && (MoreRecent(old, j->updated) || j->load<0);
      bool ok = !ter;
      // 2014-07-27 UpdateSlaveInfo() is now called outside of this loop
      if (false && upd) { 
	ok = UpdateSlaveInfo(*j);
	ter = j->status=="terminated" || j->status=="limbo";
	if (ter)
	  ok = false;
      }
      if (j->status.find("_wait")!=string::npos)
	waited_keys.insert(j->keys.begin(), j->keys.end());

      if (ter && j->keys.size()) {
	auto ki = waited_keys.find(j->keys.front());
	if (ki!=waited_keys.end())
	  waited_keys.erase(ki);
      }	

      if (debug_slaves) {
	stringstream xx;
	if (!ok)
	  xx << (ter ? " TERMINATED" : " FAILED");
	else
	  xx << (upd?" UPDATED":"") << (spa?" SPARED":"");
	WriteLog("Slave #"+ToStr(i)+": "+SlaveInfoString(*j, true)+xx.str());
      }

      if (ok) {
	// load per processor, if cpucount==0 we "guess" that it is 2
	double loadproc = j->load / (j->cpucount ? j->cpucount : 2);
	bool is_submit_tmp = j->status.find("_submit")!=string::npos;
	if (is_submit_tmp)
	  loadproc = 0;

	if (!spa && loadproc>=0 && loadproc<=minproc &&
	    (many || (j->running_threads==0 && j->pending_threads==0)) &&
	    (j->max_tasks_tot==0 || j->n_tasks_tot<j->max_tasks_tot) &&
	    (j->max_tasks_par==0 || j->n_tasks_par<j->max_tasks_par)) {
	  minproc = loadproc;

	  if (p)
	    p->RwUnlockIt();

	  p = &*j;
	  sel_i = i;
	  unlock = false;

	  is_submit = is_submit_tmp;
	  effsparetime = is_submit ? submitinterval : defsparetime;

	  if (debug_slaves)
	    WriteLog("This slave is MIN candidate");
	}
	n_ok++;
	n_not_spa += !spa;
      }

      if (unlock)
	j->RwUnlockIt();
    }
    
    if (!n_ok || waited_keys.empty()) {
      if (debug_slaves)
	WriteLog("No alive ones in list of "+ToStr(slave_list.size())
		 +" slaves");
    } else
      alive = true;

    if (!p && updated)
      WriteLog("No available slave found among "+ToStr(slave_list.size())+
	       ", "+ToStr(n_ok)+" were ok, "+ToStr(n_not_spa)+
	       " were not spared");

    if (p) {
      if (debug_slaves)
	WriteLog("Selected slave #"+ToStr(sel_i)+": "+
		 SlaveInfoString(*p, true));
      SetTimeNow(p->spare);
      p->spare.tv_sec += effsparetime;

      Simple::ErrorExtraText("=master="+HostName()+"=");

      const string& hostspec = p->hostspec;
      // obs! as of 2013-05-05 all slaves should already be created... 
      if (false && (p->status=="uninit" ||
		    p->status.find("_wait")!=string::npos) &&
	  (hostspec.find("(qsub")!=string::npos ||
	   hostspec.find("(condor")!=string::npos ||
	   hostspec.find("(sbatch")!=string::npos)
	  /* && (p->max_slaves==0 || p->n_slaves<p->max_slaves)*/
	  ) {
	slave_info_t slave(hostspec, p->executable);
	p = NULL;
	if (CreateSlave(slave, is_submit, id, cmdline)) {
	  if (debug_slaves)
	    WriteLog("Created slave for further qsub/condor/sbatch runs: " 
		     +SlaveInfoString(slave, true));
	  slave_list.push_back(slave);
	  p = &slave_list.back();
	}

      } else {
	p->n_tasks_par++;
	p->n_tasks_tot++;
      }
    }

    return p;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::UpdateSlaveInfo(bool stats_only) {
    if (!stats_only && HasSlaveStatusThread() && !IsSlaveStatusThread())
      return true; //obs!  these cases should be eliminated...

    return UpdateSlaveInfoNewest(stats_only);
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool PicSOM::UpdateSlaveInfoNewest(bool stats_only) {
    string msg = ThreadIdentifierUtil()+" UpdateSlaveInfoNewest() : ";

    bool debug = debug_slaves;

    if (debug)
      WriteLog(msg+"locking");
    RwLockReadSlaveList();

    size_t n_locked_tot = 0, n_locked_other = 0;
    size_t n_keep_locked = 0, n_term = 0, n_limbo = 0;
    size_t no_conn = 0, n_close = 0, n_fail = 0, no_mpi = 0;
    size_t no_req = 0, n_sent = 0, n_rcvd = 0, n_norcvd = 0, n_wait = 0; 
    for (auto& i : slave_list) {
      if (i.Locked()) {
	n_locked_tot++;
	if (!i.ThisThreadLocks()) {
	  n_locked_other++;
	  continue;
	}
      }
      
      bool was_locked = i.ThisThreadLocks(), keep_locked = false;
      if (!was_locked)
	i.RwLockIt();

      if (i.status=="terminated")
	n_term++;

      else if (i.status=="limbo")
	n_limbo++;

      else if (!i.conn)
	no_conn++;

      else if (i.conn->IsClosed())
	n_close++;

      else if (i.conn->IsFailed())
	n_fail++;
      
      else if (!stats_only) {
	if (i.conn->Type()!=Connection::conn_mpi_down)
	  no_mpi++;

	if (i.status_requested==false) {
	  // now _should_ be was_locked==false
	  UpdateSlaveInfoWrite(i, false);
	  if (i.status_requested) {
	    keep_locked = true;
	    n_sent++;
	  } else
	    no_req++;

	} else {
	  // now _should_ be was_locked==true
	  if (i.conn->Available()) {
	    if (UpdateSlaveInfoRead(i, false))
	      n_rcvd++;
	    else {
	      n_norcvd++;
	      string sinfo = "job=["+(i.jobids.size()?i.jobids[0]:"")
		+"] in <"+i.hostname+"> ("+i.hostspec+")";
	      WriteLog(msg+sinfo+" failing");
	    }
	  } else {
	    keep_locked = true;
	    n_wait++;
	  }
	}
      }

      if (!keep_locked)
	i.RwUnlockIt();
      else
	n_keep_locked++;
    }

    stringstream ss;
    ss << "total="         << slave_list.size()
       << " locked_tot="   << n_locked_tot
       << " locked_other=" << n_locked_other
       << " keep_locked="  << n_keep_locked
       << " term="   	   << n_term
       << " limbo="  	   << n_limbo
       << " no_conn="	   << no_conn
       << " close="  	   << n_close
       << " fail="   	   << n_fail
       << " no_mpi=" 	   << no_mpi
       << " no_req=" 	   << no_req
       << " wait="   	   << n_wait
       << " rcvd="   	   << n_rcvd
       << " no_rcvd="	   << n_norcvd
       << " sent="   	   << n_sent;

    if (debug)
      WriteLog(msg+ss.str());

    RwUnlockReadSlaveList();
    if (debug)
      WriteLog(msg+"unlocked");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::UpdateSlaveInfo(slave_info_t& s) {
    if (HasSlaveStatusThread() && !IsSlaveStatusThread())
      return true; //obs!  these cases should be eliminated...

    string msg = "UpdateSlaveInfo() ["+s.hostspec+"] : ";

    if (debug_slaves)
      WriteLog("Slave "+SlaveInfoString(s, true)+" update starting");

    UpdateSlaveInfoWrite(s);

    struct timespec ts = { 0, 10000000 }; // 10 millisecond
    nanosleep(&ts, NULL);

    UpdateSlaveInfoRead(s);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::UpdateSlaveInfoWrite(slave_info_t& s, bool do_lock) {
    if (HasSlaveStatusThread() && !IsSlaveStatusThread())
      return true; //obs!  these cases should be eliminated...

    /// there should be some kinda timeout so that this wouldn't block...

    string msg = "UpdateSlaveInfoWrite() ["+s.hostspec+"] : ";

    if (debug_slaves)
      WriteLog("Slave "+SlaveInfoString(s, true)+" update starting to write");

    if (!s.conn ||
	s.status.find("_wait")!=string::npos   ||
	s.status.find("_submit")!=string::npos ||
	s.status.find("_free")!=string::npos   ||
	s.status=="terminated" ||
	s.status=="limbo")
      return true;

    XmlDom req = XmlDom::Doc("picsom:request");
    XmlDom roo = req.Root("request");
    roo.Element("status");

    if (do_lock)
      s.RwLockIt();

    bool ret = true;
    if (s.conn) {
      ret = s.conn->WriteOutXml(req);
      if (ret)
	s.status_requested = true;
      else
	SetSlaveToLimbo(s, "UpdateSlaveInfoWrite");
    }

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::UpdateSlaveInfoRead(slave_info_t& s, bool do_lock) {
    if (HasSlaveStatusThread() && !IsSlaveStatusThread())
      return true; //obs!  these cases should be eliminated...

    /// there should be some kinda timeout so that this wouldn't block...

    string msg = "UpdateSlaveInfoRead() ["+s.hostspec+"] : ";

    if (debug_slaves)
      WriteLog("Slave "+SlaveInfoString(s, true)+" update starting to read");

    // s.status_requested could be checked of course...
    if (!s.conn ||
	s.status.find("_wait")!=string::npos   ||
	s.status.find("_submit")!=string::npos ||
	s.status.find("_free")!=string::npos   ||
	s.status=="terminated" ||
	s.status=="limbo")
      return true;

    /// The slave _should_ be locked by this thread every time we come
    /// here, but that does _not_ seem to be the case. E.g. AnalyseSlaves()
    /// seems to be able to have taken the lock...
    /// A proper solution will be to decouple the slavestatus writes
    /// and reads (as already planned).
    if (!s.ThisThreadLocks() && do_lock)
      s.RwLockIt();

    if (!s.conn)
      return true;

    set<string> lnames;
    for (thread_list_t::const_iterator i = s.threads.begin();
	 i!=s.threads.end(); i++)
      if (lnames.find(i->name)==lnames.end())
	lnames.insert(i->name);
      else {
	s.status_requested = false;
	if (do_lock)
	  s.RwUnlockIt();
	return ShowError(msg+"duplicate thread name =\""+i->name+
			 "\" in master"); 
      }

    bool dummy = false;
    bool parseresult = s.conn->ReadAndParseXML(dummy, false);

    if (!parseresult) {
      s.status_requested = false;
      if (do_lock)
	s.RwUnlockIt();
      return false; // ShowError(msg+"ReadAndParseXML() failed");
    }

    XmlDom xml(s.conn->XMLdoc());
    if (!xml.DocOK()) {
      SetSlaveToLimbo(s, "UpdateSlaveInfoRead");
      if (do_lock)
	s.RwUnlockIt();
      return true;
    }

    string host = xml.FindContent("/result/status/hostname", true);
    if (host.find_first_not_of(" \t\n")==string::npos) {
      if (do_lock)
	s.RwUnlockIt();
      return ShowError(msg+"empty <hostname>");
    }
    s.hostname = host;

    string stti = xml.FindContent("/result/status/started", true);
    if (stti.find_first_not_of(" \t\n")==string::npos) {
      s.status_requested = false;
      if (do_lock)
	s.RwUnlockIt();
      return ShowError(msg+"empty <started>");
    }
    s.started = TimeFromDotString(stti);

    string pid = xml.FindContent("/result/status/pid", true);
    if (pid.find_first_not_of(" \t\n")==string::npos) {
      s.status_requested = false;
      if (do_lock)
	s.RwUnlockIt();
      return ShowError(msg+"empty <pid>");
    }
    s.pid = atoi(pid.c_str());

    string l = xml.FindContent("/result/status/load", true);
    if (l.find_first_not_of(" \t\n")==string::npos) {
      s.status_requested = false;
      if (do_lock)
	s.RwUnlockIt();
      return ShowError(msg+"empty <load>");
    }
    s.load = atof(l.c_str());

    string c = xml.FindContent("/result/status/cpucount", true);
    if (c.find_first_not_of(" \t\n")==string::npos) {
      s.status_requested = false;
      if (do_lock)
	s.RwUnlockIt();
      return ShowError(msg+"empty <cpucount>");
    }
    s.cpucount = atoi(c.c_str());

    string u = xml.FindContent("/result/status/cpuusage", true);
    if (u.find_first_not_of(" \t\n")==string::npos) {
      s.status_requested = false;
      if (do_lock)
	s.RwUnlockIt();
      return ShowError(msg+"empty <cpuusage>");
    }
    s.cpuusage = atof(u.c_str());

    s.running_threads = 0;
    s.pending_threads = 0;

    if (debug_slaves)
      WriteLog("  Starting to process threadlist data, earlier "+
	       ToStr(lnames.size())+" threads");

    XmlDom thr = xml.FindPath("/result/status/threadlist", false);
    thr = thr.FirstElementChild();
    for (; thr; thr = thr.NextElement()) {
      string nname = thr.NodeName();
      if (nname=="") {
	if (debug_slaves)
	  WriteLog("   nname==empty"); 
	break;
      }

      if (nname!="thread") {
	s.status_requested = false;
	if (do_lock)
	  s.RwUnlockIt();
	return ShowError(msg+"<"+nname+"> found inside <threadlist>");
      }

      string name = thr.FindContent("name", true);
      if (name=="") {
	s.status_requested = false;
	if (do_lock)
	  s.RwUnlockIt();
	return ShowError(msg+"no <name> in <thread>");
      }

      string type = thr.FindContent("type", true);
      if (type=="") {
	s.status_requested = false;
	if (do_lock)
	  s.RwUnlockIt();
	return ShowError(msg+"no <type> in <thread>");
      }

      if (type=="picsom") {
	if (debug_slaves)
	  WriteLog("   type=picsom"); 
	continue;
      }

      thread_list_t::iterator i = s.threads.begin();
      while (i!=s.threads.end() && i->name!=name)
	i++;

      if (i==s.threads.end()) {
	ShowError(msg+"thread name =\""+name+"\" of ["+
		  SlaveInfoString(s, true)+
		  "] not found in master, continuing...");
	continue;
      }

      if (lnames.find(name)==lnames.end()) {
	s.status_requested = false;
	if (do_lock)
	  s.RwUnlockIt();
	return ShowError(msg+"duplicate thread name =\""+name+"\" in slave");
      }

      lnames.erase(name);

      if (debug_slaves)
	WriteLog("   updating <"+name+">"); 

      UpdateSlaveThreadInfo(*i, thr);

      if (i->slave_state=="running")
	s.running_threads++;

      if (i->slave_state=="pending")
	s.pending_threads++;
    }

    if (lnames.size()) {
      s.status_requested = false;
      if (do_lock)
	s.RwUnlockIt();
      return ShowError(msg+"thread name =\""+*lnames.begin()
		       +"\" not found in slave");
    }

    SetTimeNow(s.updated);

    if (debug_slaves)
      WriteLog("Slave "+SlaveInfoString(s, true)+" update finished");

    s.status_requested = false;
    if (do_lock)
      s.RwUnlockIt();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetSlaveToLimbo(slave_info_t& s, const string& reason) {
    string msg = "PicSOM::SetSlaveToLimbo() : ";

    stringstream cdump;
    s.conn->Dump(cdump);
    string cstr = cdump.str();
    cstr.erase(cstr.size()-1);
    WriteLog(msg+"slave "+SlaveInfoString(s, true)+"\n   connection "+
	     cstr+" in limbo because failing in "+reason);
    s.conn->Close();
    s.conn = NULL;
    s.status = "limbo";
    if (s.parent)
      PossiblyDecreaseSlaveCount(*s.parent);
    s.status_requested = false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::UpdateSlaveThreadInfo(thread_info_t& e, const XmlDom& thr) {
    stringstream ss;
    ss << ThreadInfoString(e, true);

#ifndef PTW32_VERSION
    e.id          = (pthread_t)atoi(thr.FindContent("pthread", true).c_str());
#endif // PTW32_VERSION
    e.name        = thr.FindContent("name", true);
    e.phase       = thr.FindContent("phase", true);
    e.type        = thr.FindContent("type",  true);
    e.text        = thr.FindContent("text",  true);
    e.slave_state = thr.FindContent("state", true);

    ss << " => " << ThreadInfoString(e, true);
    if (debug_slaves)
      WriteLog("Updated slave thread info: "+ss.str());
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t PicSOM::NslaveHostsContacted() {
    RwLockReadSlaveList();
    size_t n = 0;
    for (auto i=slave_list.begin(); i!=slave_list.end(); i++)
      n += i->n_slaves_contacted;

    RwUnlockReadSlaveList();

    return n;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t PicSOM::NslavesCommon(void *obj, bool threads, const string& spec) {
    string msg = "PicSOM::NslavesCommon(...,"+ToStr(threads)+","+spec+") : ";
    bool debug = false;

    RwLockReadSlaveList();

    if (debug)
      cout << msg << "STARTING " << slave_list.size() << endl;

    size_t n = 0;
    for (auto i=slave_list.begin(); i!=slave_list.end(); i++) {
      // if (!UpdateSlaveInfo(*i))
      // 	continue;

      if (debug)
	cout << msg << i->status << " " << i->threads.size() << endl;

      if ((i->status=="limbo" || i->status=="terminated") &&
	  i->status!=spec)
	continue;

      for (thread_list_t::const_iterator j=i->threads.begin();
	   j!=i->threads.end(); j++) {
	if (debug)
	  cout << "   " << j->name << " " 
	       << j->object << " " << obj << " "
	       << int(j->object==obj) << " " << endl;
	if (j->object==obj) {
	  n++;
	  if (!threads)
	    break;
	}
      }
    }

    if (debug)
      cout << msg << "RETURNING " << n << endl;

    RwUnlockReadSlaveList();

    return n;
  }

  /////////////////////////////////////////////////////////////////////////////

  pair<PicSOM::slave_info_t*,PicSOM::thread_info_t*>
  PicSOM::SlaveThreadWithResults(void *obj) {
    pair<slave_info_t*,thread_info_t*> ret((slave_info_t*)NULL,
  					   (thread_info_t*)NULL);
    RwLockReadSlaveList();

    bool found = false;
    for (auto i=slave_list.begin(); !found && i!=slave_list.end(); i++)
      if (i->conn && i->status!="limbo" && !i->Locked()) {
	i->RwLockIt();
	if (!i->conn->IsClosed() && !i->conn->IsFailed())
	  for (thread_list_t::iterator j=i->threads.begin();
	       !found && j!=i->threads.end(); j++)
	    if (j->object==obj &&
		j->slave_state!="pending" && j->slave_state!="running" &&
		j->slave_state!="errored") {
	      ret = make_pair(&*i, &*j);
	      found = true;
	    }
	if (!found)
	  i->RwUnlockIt();
      }

    RwUnlockReadSlaveList();

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  // pair<PicSOM::slave_info_t*,PicSOM::thread_info_t*>
  // PicSOM::FirstNonPendingNonRunningNonErroredNonLimboSlaveThread(void *obj) {
  //   pair<slave_info_t*,thread_info_t*> ret((slave_info_t*)NULL,
  // 					   (thread_info_t*)NULL);
  //   RwLockReadSlaveList();

  //   for (auto i=slave_list.begin(); i!=slave_list.end(); i++)
  //     if (i->status!="limbo" && i->conn)
  // 	for (thread_list_t::iterator j=i->threads.begin();
  // 	     j!=i->threads.end(); j++)
  // 	  if (j->object==obj &&
  // 	      j->slave_state!="pending" && j->slave_state!="running" &&
  // 	      j->slave_state!="errored") {
  // 	    ret = make_pair(&*i, &*j);
  // 	    goto out;
  // 	  }
  // out:
  //   RwUnlockReadSlaveList();

  //   return ret;
  // }

  /////////////////////////////////////////////////////////////////////////////

  // pair<PicSOM::slave_info_t*,PicSOM::thread_info_t*>
  // PicSOM::FirstNonPendingNonRunningNonErroredSlaveThread(void *obj) {
  //   pair<slave_info_t*,thread_info_t*> ret((slave_info_t*)NULL,
  // 					   (thread_info_t*)NULL);
  //   RwLockReadSlaveList();

  //   for (auto i=slave_list.begin(); i!=slave_list.end(); i++)
  //     for (thread_list_t::iterator j=i->threads.begin();
  // 	   j!=i->threads.end(); j++)
  // 	if (j->object==obj &&
  // 	    j->slave_state!="pending" && j->slave_state!="running" &&
  // 	    j->slave_state!="errored") {
  // 	  ret = make_pair(&*i, &*j);
  // 	  goto out;
  // 	}
  // out:
  //   RwUnlockReadSlaveList();

  //   return ret;
  // }

  /////////////////////////////////////////////////////////////////////////////

  // pair<PicSOM::slave_info_t*,PicSOM::thread_info_t*>
  // PicSOM::FirstNonPendingNonRunningSlaveThread(void *obj) {
  //   pair<slave_info_t*,thread_info_t*> ret((slave_info_t*)NULL,
  // 					   (thread_info_t*)NULL);
  //   RwLockReadSlaveList();

  //   for (auto i=slave_list.begin(); i!=slave_list.end(); i++)
  //     for (thread_list_t::iterator j=i->threads.begin();
  // 	   j!=i->threads.end(); j++)
  // 	if (j->object==obj &&
  // 	    j->slave_state!="pending" && j->slave_state!="running") {
  // 	  ret = make_pair(&*i, &*j);
  // 	  goto out;
  // 	}

  // out:
  //   RwUnlockReadSlaveList();

  //   return ret;
  // }

  /////////////////////////////////////////////////////////////////////////////

  // pair<PicSOM::slave_info_t*,PicSOM::thread_info_t*>
  // PicSOM::FirstNonRunningSlaveThread(void *obj) {
  //   pair<slave_info_t*,thread_info_t*> ret((slave_info_t*)NULL,
  // 					   (thread_info_t*)NULL);
  //   RwLockReadSlaveList();

  //   for (auto i=slave_list.begin(); i!=slave_list.end(); i++)
  //     for (thread_list_t::iterator j=i->threads.begin();
  // 	   j!=i->threads.end(); j++)
  // 	if (j->object==obj && j->slave_state!="running") {
  // 	  ret = make_pair(&*i, &*j);
  // 	  goto out;
  // 	}

  // out:
  //   RwUnlockReadSlaveList();

  //   return ret;
  // }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::RemoveSlaveThread(const pair<slave_info_t*,
				 thread_info_t*>& si) {
    if (!si.first->ThisThreadLocks())
      si.first->RwLockIt();

    RwLockWriteSlaveList();
    bool r = RemoveSlaveThreadInner(si);
    RwUnlockWriteSlaveList();

    si.first->RwUnlockIt();

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::RemoveSlaveThreadInner(const pair<slave_info_t*,
				      thread_info_t*>& si) {
    string msg = "RemoveSlaveThread() : ";

    if (debug_slaves) {
      stringstream ss;
      ss << SlaveInfoString(*si.first, false) << "\n  "
	 << ThreadInfoString(*si.second, true);
      WriteLog("Removing slave thread "+ss.str());
    }
    
    string name = si.second->name;

    bool erased = false;

    for (auto i=slave_list.begin(); !erased && i!=slave_list.end(); i++)
      if (&*i==si.first) {
	for (thread_list_t::iterator j=i->threads.begin();
	     !erased && j!=i->threads.end(); j++)
	  if (&*j==si.second) {
	    // i->RwLockIt();
	    i->threads.erase(j);
	    // i->RwUnlockIt();
	    erased = true;
	  }
	
	if (!erased)
	  return ShowError(msg+"thread not found in master");
      }

    if (!erased)
      return ShowError(msg+"slave not found in master");

    if (!si.first->conn ||
	si.first->conn->IsClosed() || si.first->conn->IsFailed())
      return true;

    XmlDom req = XmlDom::Doc("picsom:request");
    XmlDom roo = req.Root("request");
    XmlDom thr = roo.Element("thread");
    thr.Prop("name",    name);
    thr.Prop("command", "terminate");
    si.first->conn->WriteOutXml(req);

    bool dummy = false;
    if (!si.first->conn->ReadAndParseXML(dummy, false))
      return ShowError(msg+"ReadAndParseXML() failed");
    
    XmlDom xml(si.first->conn->XMLdoc());
    if (xml.FindContent("/result/thread/phase", true)!="terminated")
      return ShowError(msg+"slave thread termination failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::PossiblyTerminateSlave(slave_info_t& slave) {
    string msg = "PossiblyTerminateSlave() : ";
    bool debug = false;

    float mul = 2.0;

    if (debug)
      WriteLog(msg+"slave.max_time="+ToStr(slave.max_time)
	       +" IsTimeZero(slave.started)="+
	       ToStr((int)IsTimeZero(slave.started))
	       +" IsTimeZero(slave.task_finished)="+
	       ToStr((int)IsTimeZero(slave.task_finished)));

    if (!slave.max_time || IsTimeZero(slave.started) ||
	IsTimeZero(slave.task_finished))
      return true;

    struct timespec now = TimeNow();
    double s_used  = TimeDiff(now, slave.started);
    double s_left  = slave.max_time-s_used;
    double s_last  = TimeDiff(slave.task_finished, slave.task_started);

    if (debug)
      WriteLog(msg+"s_used="+ToStr(s_used)+" s_left="+ToStr(s_left)+
	       " s_last="+ToStr(s_last));

    string why;
    if (s_left<mul*s_last)
      why = "because "+ToStr(s_left)+"<"+ToStr(mul*s_last);

    if (slave.returned_empty)
      why = "because returned empty";
    
    if (why!="")
      return TerminateSlave(slave, why, 3);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::TerminateSlave(slave_info_t& slave, const string& why,
			      int sleepsec) {
    slave.RwLockIt();

    // string msg = "TerminateSlave() : ";

    if (debug_slaves) {
      string out = "Terminating slave "+SlaveInfoString(slave, true);
      out += string(" [")+why+string("]");
      WriteLog(out);
    }
  
    if (slave.conn) {
      XmlDom req = XmlDom::Doc("picsom:command");
      XmlDom roo = req.Root("command");
      roo.Element("terminate");
      slave.conn->WriteOutXml(req);
      if (sleepsec>0)
	sleep(sleepsec);
      slave.conn->Close();
      slave.conn = NULL;
    }
    slave.status = "terminated";
    
    if (slave.parent) {
      PossiblyDecreaseSlaveCount(*slave.parent);
      // the following was commented out 2014-03-27
      // similar out-commenting had earlier been done in UpdateSlaveInfo()
      //   where status="limbo" is set
      // PossiblyStartNewSlaves(*slave.parent);
    }

    slave.RwUnlockIt();

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::PossiblyDecreaseSlaveCount(slave_info_t& s) {
    string msg = "PossiblyDecreaseSlaveCount() : ";
    
    if (s.n_slaves_started && s.n_slaves_now)
      s.n_slaves_now--;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CheckSlaveCount(slave_info_t& s) {
    string msg = "CheckSlaveCount() : ";

    if (s.hostspec.find("(sbatch,")!=string::npos)
      return CheckSlaveCountSlurm(s);

    if (s.hostspec.find("(condor,")!=string::npos)
      return CheckSlaveCountCondor(s);

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CheckSlaveCountSlurm(slave_info_t& s) {
    string msg = "CheckSlaveCountSlurm() : ";

    //string slurm = "/share/apps/bin/slurm";
    //string slurm = "/usr/bin/slurm";
    string slurm = "/usr/local/bin/slurm";
    size_t n_pend = 0;
    set<size_t> ids;
    stringstream ss;
    vector<string> cmd { "nohup", slurm, "q" };
    auto rr = ShellExecute(cmd, true, true);
    if (!rr.first)
      ss << "'slurm q' command failed";
    else {
      const vector<string>& l = rr.second;
      for (size_t i=1; i<l.size(); i++) {
	size_t jobid = atoi(l[i].c_str());
	if (!jobid) {
	  ss << "line '" << l[i] << "' not understood ";
	  continue;
	}
	ids.insert(jobid);
	if (l[i].find("PENDING")!=string::npos)
	  n_pend++;
      }
      if (ids.empty())
	ss << "no change due to no jobs found with slurm";
      else {
	size_t n_conn = 0;
	for (auto sp=slave_list.begin(); sp!=slave_list.end(); sp++)
	  if (sp->jobids.size()==1 && sp->jobids[0].size() &&
	      sp->jobids[0].find_first_not_of("0123456789")==string::npos) {
	    size_t jobid = atoi(sp->jobids[0].c_str());
	    if (ids.find(jobid)!=ids.end())
	      n_conn++;
	  }
	if (!n_conn)
	  ss << "no change due to no jobs found in PicSOM";
	else {
	  size_t n = n_conn + n_pend;
	  if (n>s.n_slaves_now)
	    n = s.n_slaves_now;
	  ss << s.n_slaves_now << " -> " << n << " (conn=" << n_conn
	     << ", pend=" << n_pend << ")";
	  s.n_slaves_now = n;
	}
      }
    }
    if (true || debug_slaves)
      WriteLog(msg+"updated n_slaves_now: "+ss.str());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CheckSlaveCountCondor(slave_info_t& s) {
    string msg = "CheckSlaveCountCondor() : ";

    string condor = "condor_q";
    size_t n_pend = 0;
    set<size_t> ids;
    stringstream ss;
    vector<string> cmd { condor };
    auto rr = ShellExecute(cmd, true, true);
    if (!rr.first)
      ss << "'condor_q' command failed";
    else {
      const vector<string>& l = rr.second;
      size_t i = 0;
      while (i<l.size()) {
	if (l[i].substr(0, 4)!=" ID ") {
	  i++;
	  continue;
	}
	i++;
	break;
      }
      for (; i<l.size(); i++) {
	if (l[i].find(" jobs; ")!=string::npos)
	  break;
	size_t jobid = atoi(l[i].c_str());
	if (!jobid) {
	  ss << "line '" << l[i] << "' not understood ";
	  continue;
	}
	ids.insert(jobid);
	if (l[i].find(" I ")!=string::npos)
	  n_pend++;
      }
      if (ids.empty())
	ss << "no change due to no jobs found with condor_q";
      else {
	size_t n_conn = 0;
	for (auto sp=slave_list.begin(); sp!=slave_list.end(); sp++)
	  if (sp->jobids.size()==1 && sp->jobids[0].size() &&
	      sp->jobids[0].find_first_not_of("0123456789")==string::npos) {
	    size_t jobid = atoi(sp->jobids[0].c_str());
	    if (ids.find(jobid)!=ids.end())
	      n_conn++;
	  }
	if (!n_conn)
	  ss << "no change due to no jobs found in PicSOM";
	else {
	  size_t n = n_conn + n_pend;
	  if (n>s.n_slaves_now)
	    n = s.n_slaves_now;
	  ss << s.n_slaves_now << " -> " << n << " (conn=" << n_conn
	     << ", pend=" << n_pend << ")";
	  s.n_slaves_now = n;
	}
      }
    }
    if (true || debug_slaves)
      WriteLog(msg+"updated n_slaves_now: "+ss.str());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::PossiblyStartNewSlaves(slave_info_t& s) {
    string msg = "PossiblyStartNewSlaves() : ";

    static size_t r = 0;
    r++;
    if (r%10==0)
      CheckSlaveCount(s);

    size_t start = 0, max_all = s.max_slaves_start+s.max_slaves_add;
    if (s.n_slaves_now<s.max_slaves_start && s.max_slaves_add && 
	s.n_slaves_started<max_all) {
      start = s.max_slaves_start-s.n_slaves_now;
      if (max_all-s.n_slaves_started<start)
	start = max_all-s.n_slaves_started;
    }

    if (true || debug_slaves)
      WriteLog(msg+"max_slaves_start="+ToStr(s.max_slaves_start)+
	       " max_slaves_add="+ToStr(s.max_slaves_add)+
	       " n_slaves_now="+ToStr(s.n_slaves_now)+
	       " n_slaves_started="+ToStr(s.n_slaves_started)+
	       " start="+ToStr(start));

    if (s.hostspec.find("(condor,")!=string::npos)
      start = 0;

    if (start)
      WriteLog("Starting "+ToStr(start)+" new slaves");

    for (size_t i=0; i<start; i++)
      CreateSlavePartOne(s, false, "", "");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<string> PicSOM::SbatchExclude() const {
    string msg = "SbatchExclude() : ";
    string exclude = UserHomeDir()+"/picsom/sbatch-exclude";
    string st = FileToString(exclude);
    vector<string> l = SplitInSomething("\n", true, st);
    list<string> ret;
    for (auto i=l.begin(); i!=l.end(); i++) {
      string s = *i;
      size_t p = s.find('#');
      if (p!=string::npos)
	s.erase(p);
      vector<string> v = SplitInSpaces(s);
      ret.insert(ret.end(), v.begin(), v.end());
    }

    WriteLog(msg+"<"+exclude+"> contained \""+CommaJoin(ret)+"\"");

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::ExtractVersion(const string& vcid) {
    char vers[1024] = "????";

    const char *ptr = strstr(vcid.c_str(), ",v ");
    if (ptr && strlen(ptr)>3) {
      ptr += 3;
      strcpy(vers, ptr);
      char *ptr2 = strchr(vers, ' ');
      if (ptr2)
	*ptr2 = 0;
    }

    return vers;
  }

  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::CpuCount() {
#if defined(__alpha) || defined(sgi)
    return (int)sysconf(_SC_NPROC_ONLN);
#elif defined(__linux__)
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 0; // else return 0 which means unknown
#endif
  }

  /////////////////////////////////////////////////////////////////////////////

  double PicSOM::CpuUsage() {
    static struct timespec time_was;
    static tms cpu_was;
    static double tck = TicTac::TickSize();
    static bool first = true;

    if (first) {
      first = false;
      TicTac::SetTimes(time_was, cpu_was);
      return 0;
    }

    double r = -1;
    struct timespec time_is;
    tms cpu_is;
    TicTac::SetTimes(time_is, cpu_is);

    double tdiff = time_is.tv_sec-time_was.tv_sec+
      1e-9*(time_is.tv_nsec-time_was.tv_nsec);

    if (tdiff) {
      int udiff   = cpu_is.tms_utime-cpu_was.tms_utime;
      int sdiff   = cpu_is.tms_stime-cpu_was.tms_stime;
      r = 100.0*(udiff+sdiff)/(tck*tdiff);
    }

    time_was = time_is;
    cpu_was = cpu_is;

    return r;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& PicSOM::HostName(bool dom) {
    const map<string,string> forced_name {
      make_pair("login2.triton.aalto.fi", "login2")
	};

    static string host, hostdom, numeric;
    if (host=="") {
      if (forcedhostname!="")
	host = hostdom = numeric = forcedhostname;
      else {
	char tmp[1000] = "";
	gethostname(tmp, sizeof tmp);
	hostdom = host = tmp;

	struct hostent myhost;
	const struct hostent* he = GetHostByName(tmp, myhost);

	bool found = false;
	auto p = forced_name.find(hostdom);
	if (p!=forced_name.end()) {
	  found = true;
	  host = hostdom = p->second;
	}

	if (!found && he) {
	  hostdom = he->h_name;
	  if (hostdom.find('.')!=string::npos &&
	      hostdom.find("localdomain")==string::npos)
	    found = true;

	  for (int j=0; !found && he->h_addr_list[j]; j++) {
	    stringstream ss;
	    for (int i=0; i<he->h_length; i++)
	      ss << (i?".":"") << (int)(unsigned char)he->h_addr_list[j][i];
	    numeric = ss.str();
	    if (numeric.find("127.0.")!=0)
	      found = true;
	  }
	}
	if (!found) {
	  struct utsname buf;
	  if (!uname(&buf)) {
	    if (strlen(buf.nodename)) {
	      hostdom = host = buf.nodename;
	      size_t p = host.find('.');
	      if (p!=string::npos)
		host.erase(p);
	      found = true;
	    }
	  }
	}
	if (!found)
	  numeric = HostAddr();
      }
    }
    return numeric!="" ? numeric : dom ? hostdom : host;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& PicSOM::HostAddr(const string& ifin) {
    static string numeric;
    if (numeric=="") {
      string ifname = ifin;
      if (ifname=="")
	ifname = "eth0";  // obs! ignored now...

      // string cmd = "/sbin/ifconfig "+ifname+
      //   " | grep \"inet addr:\" | grep Bcast:";

      string cmd = "/sbin/ip addr show | grep inet | grep global";
      FILE *ifconfig = popen(cmd.c_str(), "r");
      if (ifconfig) {
	char line[10000];
	if (fgets(line, sizeof line, ifconfig)) {
	  string tmp = line;
	  size_t p = tmp.find_first_of("0123456789");
	  if (p!=string::npos) {
	    tmp.erase(0, p);
	    p = tmp.find_first_not_of("0123456789.");
	    if (p!=string::npos)
	      tmp.erase(p);
	    if (tmp.size()>=7)
	      numeric = tmp;
	  }
	}
	pclose(ifconfig);
      }
    }
    return numeric;
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::OriginsDateString(const struct timespec& tt, bool nsec) {
    char tmp[1024] = "??????";
    tm *time = localtime(&tt.tv_sec);
    if (time) {
      strftime(tmp, sizeof tmp, "%Y%m%d%H%M%S", time);
      if (nsec)
	sprintf(tmp+strlen(tmp), ".%09ld", tt.tv_nsec);
    }
    return tmp;
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::OriginsDateStringNow(bool nsec) {
    struct timespec tt;
    SetTimeNow(tt);
    return OriginsDateString(tt, nsec);
  }

  /////////////////////////////////////////////////////////////////////////////

  struct timespec PicSOM::InverseOriginsDateString(const string& s) {
    struct timespec ts = { 0, 0 };

    string digit = "0123456789";
    bool is_short = s.size()==14 && s.find_first_not_of(digit)==string::npos;
    bool is_long  = s.size()==24 && s.find_first_not_of(digit)==14 &&
      s.find_first_not_of(digit, 15)==string::npos && s[14]=='.';

    if (!is_short && !is_long)
      return ts;

    const char *str = s.c_str();
    // str =~ yyyymmddHHMMSS.123456789
    //        012345678901234567890123

    tm date = {
      TwoDigitsToInt( str, 12),      // sec
      TwoDigitsToInt( str, 10),      // min
      TwoDigitsToInt( str,  8),      // hour
      TwoDigitsToInt( str,  6),      // mday
      TwoDigitsToInt( str,  4)-1,    // mon 
      FourDigitsToInt(str,  0)-1900, // year
      0,                             // wday
      0,                             // yday
      1                              // isdst
#ifdef __linux__
      , 0, NULL                      // gmtoff, zone
#endif // __linux__
    };

    ts.tv_sec = mktime(&date);

    if (is_long)
      ts.tv_nsec = atol(str+15);

    return ts;
  }

  /////////////////////////////////////////////////////////////////////////////

  string XSdateTime(const struct timespec& tt) {
    tm *time = localtime(&tt.tv_sec);
    if (!time)
      return "failed("+TimeDotString(tt)+")";

    char tmp[1024];
    strftime(tmp, sizeof tmp, "%Y-%m-%dT%H:%M:%S", time);
    
    return tmp;
  }

  /////////////////////////////////////////////////////////////////////////////

  string TimeString(const struct timespec& tt, bool msec) {
    tm *time = localtime(&tt.tv_sec);
    if (!time)
      return "failed("+TimeDotString(tt)+")";

    char tmp[1024];
    strftime(tmp, sizeof tmp, "%d.%m.%Y %H:%M:%S", time);  // was %y
    
    if (msec) {
      int ms = (int)floor(tt.tv_nsec/1000000.0);
      sprintf(tmp+strlen(tmp), ".%03d", ms);
    }

    return tmp;
  }

  /////////////////////////////////////////////////////////////////////////////

  struct timespec PicSOM::InverseTimeStringOld(const char *str) {
    tm date;

    if (strlen(str)==17) {
      // str =~ dd.mm.yy HH:MM:SS
      //        01234567890123456

      tm shortdate = {
	TwoDigitsToInt(str, 15),     // sec
	TwoDigitsToInt(str, 12),     // min
	TwoDigitsToInt(str,  9),     // hour
	TwoDigitsToInt(str,  0),     // mday
	TwoDigitsToInt(str,  3)-1,   // mon 
	TwoDigitsToInt(str,  6)+100, // year
	0,                           // wday
	0,                           // yday
	1                            // isdst
#ifdef __linux__
	, 0, NULL                    // gmtoff, zone
#endif // __linux__
      };
      memcpy(&date, &shortdate, sizeof date);

    } else if (strlen(str)==19) {
      // str =~ dd.mm.yyyy HH:MM:SS
      //        0123456789012345678

      tm longdate = {
	TwoDigitsToInt( str, 17),      // sec
	TwoDigitsToInt( str, 14),      // min
	TwoDigitsToInt( str, 11),      // hour
	TwoDigitsToInt( str,  0),      // mday
	TwoDigitsToInt( str,  3)-1,    // mon 
	FourDigitsToInt(str,  6)-1900, // year
	0,                             // wday
	0,                             // yday
	1                              // isdst
#ifdef __linux__
	, 0, NULL                      // gmtoff, zone
#endif // __linux__
      };
      memcpy(&date, &longdate, sizeof date);

    } else
      ShowError("PicSOM::InverseTimeString(", str, ") unknow format");

    struct timespec ts;
    ts.tv_sec = mktime(&date);
    ts.tv_nsec = 0;

    //   cout << "InverseTimeString(" << str << ") -> " << ts.tv_sec << " "
    //        << TimeString(ts) << endl;

    return ts;
  }

  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::TwoDigitsToInt(const char *str, int i) {
    return (str[i]-'0')*10+(str[i+1]-'0');
  }

  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::FourDigitsToInt(const char *str, int i) {
    return (str[i]-'0')*1000+(str[i+1]-'0')*100+TwoDigitsToInt(str, i+2);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CommandLineParsing(const vector<string>& args) {
    bool args_ok = true;

    string tmp1starg;
    size_t ai = 0; 
    for (; ai<args.size() && args[ai][0]=='-' && args_ok; ai++) {
      const string empty;
      const string& astr = args[ai];
      const string& nstr = ai<args.size()-1 ? args[ai+1] : empty;

      /*>> cmdline-io Input/output, logging and socket options.
	-port || -port=A || -port=A..B
	Tells that the server should listen to the default system socket
	port, given port A or the first free one in a range A..B of
	socket ports.
      */
      if (astr.find("-port")==0) {
#ifdef PICSOM_DEVELOPMENT
	development = false;
#endif // PICSOM_DEVELOPMENT
	if (astr.size()==5) {
	  ListenPort();
	  continue;
	}
	if (astr[5]=='=' && astr.size()>6) {
	  int p, q;
	  int n = sscanf(astr.c_str()+6, "%d..%d", &p, &q);
	  ListenPort(p, n==2?q:p);
	  continue;
	}
	ShowError("CommandLineParsing() -port failed");
      }

      /*>	-myport
	Tells that the server should listen to user-specific port used
	for development purposes.
      */
      if (astr=="-myport") {
	if (!ListenPort(UserName()))
	  ShowError("CommandLineParsing() -myport failed for ", UserName());
	continue;
      }

      /*>	-localhost
	Tells that the server should be contacted as localhost which
	is useful for SSH tunneling.
      */
      if (astr=="-localhost") {
	uselocalhost = true;
	continue;
      }

      /*>	-host=forcedname
      */
      if (astr.find("-host=")==0) {
	forcedhostname = astr.substr(6);
	continue;
      }

      /*>	-soap=port
	Tells that the server should listen to user-specific SOAP port.
      */
#ifdef PICSOM_USE_CSOAP
      if (astr=="-soap" || astr.substr(0, 6)=="-soap=") {
	soap_port = PICSOM_DEFAULT_SOAP_PORT;
	if (astr.size()>6) {
	  if (astr.find("single")!=string::npos)
	    Connection::SoapServerSingle(true);
	  else {
	    soap_port = atoi(astr.substr(6).c_str());
	    if (!soap_port)
	      ShowError("CommandLineParsing() -soap= failed");
	  }
	}
	continue;
      }
#endif // PICSOM_USE_CSOAP

      /*> -logm=[time|id|timeid|]
	Sets logging mode.
      */
      if (astr.find("-logm=")==0) {
	LoggingMode(astr.substr(6));
	continue;
      }

      /*>	-log
	Directs all logging output to its standard location in
	.../picsom/logs/server/xxx.
      */    
      if (astr=="-log") {
	SetLogging();
	continue;
      }

      /*>	-log=FILE
	Directs all logging output to FILE.
      */    
      if (astr.find("-log=")==0) {
	SetLogging(astr.c_str()+5);
	continue;
      }

      /*>	-nocin
	Tells that cin should not be read.
      */    
      if (astr=="-nocin") {
	HasCin(false);
	continue;
      }

      /*>	-X
	prevents access to root directory.
      */    
      if (astr=="-X") {
	norootdir = true;
	continue;
      }

      /*>	-root=PATH
	Sets root directory.
      */    
      if (astr.find("-root=")==0) {
	env["ROOT"] = astr.substr(6);
	continue;
      }

      /*>	-home=PATH
	Sets home directory.
      */    
      if (astr.find("-home=")==0) {
	env["HOME"] = astr.substr(6);
	continue;
      }

      /*>	-user=NAME
	Sets user name.
      */    
      if (astr.find("-user=")==0) {
	env["USER"] = astr.substr(6);
	continue;
      }

      /*>	-q
	Makes execution less noisy, ie. all logging is sinked.
      */
      if (astr=="-q") {
	Quiet(true);
	continue;
      }
    
      /*>	-Q
	Makes execution silent, ie. all output is sinked.
      */
      if (astr=="-Q") {
	SetLogging("/dev/null");
	continue;
      }

      /*> -gtcomp
	Forces compilation of classes file.
      */
      if (astr=="-gtcomp") {
	DataBase::ForceCompile(true);
	continue;
      }
    
      /*>> cmdline-other Other and miscellaneous switches.
	-db XX,YY
	Restricts access to only specied list of databases.
      */
      if (astr=="-db" && nstr!="") {
	SetAllowedDataBases(nstr);
	ai++;
	continue;
      }

      /*>
	-nometa
	Sets external metadata parsing to false
      */
      if (astr=="-nometa") {
	DataBase::ParseExternalMetadata(false);
	continue;
      }

      /*>
	-pidfile file
	Saves pid in given file
      */
      if (astr=="-pidfile" && nstr!="") {
	pidfile = nstr;
	ai++;
	continue;
      }

      /*>
	-classify
	Turns on the classification of selected objects (with default values). 
      */
      if (astr=="-classify") {
	WarnOnce("-classify is obsoleted, use Q::classify=1");
	// Query::Classify(1); // was Classify(true)
	continue;
      }

      /*>
	-classify=method,nclasses,classificationset
	Turns on the classification of selected objects. The used method and 
	number of proposed classes can be specified. 
      */
      if (astr.substr(0, 10)=="-classify=") {
	WarnOnce("-classify=a,b,c is obsoleted, use Q::classifyparams=a,b,c");
	// Query::Classify(1); // was Classify(true)
	// vector<string> vals = SplitInCommas(astr.substr(10));
	// Query::SetClassifyParams(vals[0], atoi(vals[1].c_str()), vals[2]);
	continue;
      }

      /*>
	-query XX
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-query" && nstr!="") {
	readquery.push_back(nstr);
	ai++;
	continue;
      }

      /*>
	-dumpquery
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-dumpquery" && nstr!="") {
	dumpquery.push_back(nstr);
	ai++;
	continue;
      }

      /*>
	-matlabdump
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-matlabdump" && nstr!="") {
	Query::MatlabDump(nstr);
	ai++;
	continue;
      }

      /*>
	-context
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-context" && nstr!="") {
	LoadContext(nstr);
	ai++;
	continue;
      }

      /*>
	-bg
        Immediately fork a new process where the execution is continued
	while the original parent process exits.
      */
      if (astr=="-bg") {
	RunInBackGround();
	continue;
      }

      /*>
	-nobg
        Set a flag that prevents forthcoming -bg switches to cause forking.
      */
      if (astr=="-nobg") {
	allow_fork = false;
	continue;
      }

      /*>
	-noslaves
        Set a flag that makes slave specificatio ineffective.
      */
      if (astr=="-noslaves") {
	use_slaves = false;
	continue;
      }

      /*>
	-demo
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-demo" || astr=="-democlean" ||
	  astr=="-demotest" || astr=="-demotesttest" ||
	  astr=="-forge" || astr=="-forgetest" || astr=="-forgeclean") {
#ifdef PICSOM_USE_PTHREADS
	pthreads_tssom = pthreads_connection = true;
#endif // PICSOM_USE_PTHREADS
	bool usemysql = Connection::RestCaMysql();
	if (astr=="-demo" || astr=="-democlean" ||
	    astr=="-forge" || astr=="-forgeclean") {
	  guarding = true;
	  RunInBackGround();
	  SetLogging();
	}
	development = false;
	Path(UserHomeDir()+"/picsom");
	if (astr=="-democlean" || astr=="-forgeclean")
	  Unlink(UserHomeDir()+"/picsom/databases/restca.sqlite3");
	string defdb  = "ftp.sunet.se", svmdb = "svmdb", restca = "restca";
	string facedb = "d2ifaces", ilsvrc2012 = "ilsvrc2012-ics";
	if (usemysql)
	  restca = "mysql://"+restca;
	if (astr=="-demotesttest")
	  defdb += "-www";
	DataBase::OpenReadWriteSql(true);
	if (astr.find("demo")!=string::npos) {
	  FindDataBase(defdb);
	  SetAllowedDataBases(defdb+","+svmdb+","+restca+","+facedb);
	  Connection::DefaultXSL("demo");
	} else { // forge
	  SetAllowedDataBases(restca+","+svmdb);
	  //SetAllowedDataBases(restca+","+svmdb+","+ilsvrc2012
	  //		      +",finlandia,d2i-demo");
	  expect_slaves = true;
	  // FindDataBase("d2i-demo");
	  if (astr.find("test")==string::npos) {
	    // DataBase *fin = FindDataBase("finlandia");
	    // fin->Visible(false);
	  }
	}
	ListenPort();
	continue;
      }

      /*>
	-server
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-server") {
#ifdef PICSOM_USE_PTHREADS
	pthreads_tssom = pthreads_connection = true;
#endif // PICSOM_USE_PTHREADS
	development = false;
	DataBase::OpenReadWrite("sql+fea+txt");
	RunInBackGround(); SetLogging(); ListenPort();
	continue;
      }

      /*>
	-myserver
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-myserver") {
#ifdef PICSOM_USE_PTHREADS
	pthreads_tssom = pthreads_connection = true;
#endif // PICSOM_USE_PTHREADS
	DataBase::OpenReadWrite("sql+fea+txt");
	RunInBackGround(); SetLogging(); ListenPort(UserName(), true);
	continue;
      }

      /*>
	-guard
        *DOCUMENTATION MISSING* 
	*/
      if (astr.find("-guard")==0) {
	guarding = true;
	if (astr.find("=nolimits")!= string::npos)
	  childlimits = false;
	continue;
      }

      /*>
	-master=foo.domain.net:port
        *DOCUMENTATION MISSING* 
	*/
      if (astr.find("-master=")==0) {
	master_address = astr.substr(8);
	norootdir = true;
	continue;
      }

      /*>
	-slave
        *DOCUMENTATION MISSING* 
	*/
      if (astr.find("-slave")==0 && (astr.size()==6 || astr[6]=='=')) {
	PrepareSlave(astr.size()==6?"":astr.substr(7));
	continue;
      }

      /*>
	-ssh
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-ssh" && nstr!="") {
	ssh_forwarding = nstr;
	ai++;
	continue;
      }

      /*>
	-unlimit
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-unlimit" || astr=="-unlimited") {
	SetUnLimited();
	continue;
      }

      /*>
	-development
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-development") {
	development = true;
	continue;
      }

      /*>
	-nodevelopment
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-nodevelopment") {
	development = false;
	continue;
      }

      /*>
	-signals
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-signals") {
	use_signals = true;
	continue;
      }

      /*>
	-nosignals
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-nosignals") {
	use_nosignals = true;
	continue;
      }

      /*>
	-nosave
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-nosave") {
	no_save = true;
	continue;
      }

      /*>
	-noexpunge
        *DOCUMENTATION MISSING* 
	*/
      if (astr=="-noexpunge") {
	expunge_time = -1;
	continue;
      }

      /*>
	-expungetime=S
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-expungetime=")==0) {
	expunge_time = atoi(astr.c_str()+13);
	continue;
      }

      /*>	-sqlite
       */
      if (astr.find("-sqlite=")==0) {
	return ShowError("switch -sqlite=rw replaced by -rw=sql");
      }

      /*> -rw
       */
      if (astr=="-rw") {
	return ShowError("-rw is replaced by -rw=sql/fea/det");
      }

      /*> -rw
       */
      if (astr.find("-rw=")==0) {
	DataBase::OpenReadWrite(astr.substr(4));
	continue;
      }

      /*>	-httpclienttest
       */
      if (astr=="-httpclienttest" && nstr!="") {
	HttpClientTest(nstr);
	ai++;
	continue;
      }

      /*>	-streamstest
	Tests that all output to C and C++ streams go to the
	desired location.
      */
      if (astr=="-streamstest") {
	StreamsTest();
	continue;
      }

      /*>	-octavetest
	Tests that all output to C and C++ streams go to the
	desired location.
      */
      if (astr=="-octavetest") {
	OctaveInializeOnce();
	continue;
      }

      /*>	-imagetest=file
	Tests that image files can be read and written.
      */
      if (astr.find("-imagetest=")==0) {
	ImageTest(astr.substr(11));
	continue;
      }

      /*>	-blastest | -blastest=N
	Executes FloatMatrix::BlasTest(1) or FloatMatrix::BlasTest(N)
	and shows the elapsed time. 
      */
      if (astr.substr(0, 9)=="-blastest") {
	DebugTimes(true);
	StartTimes();
	simple::FloatMatrix::BlasTest(DebugInteger(astr));
	ShowTimes(true, false);
	continue;
      }

      /*>	-labeltest DBNAME
	Executes PicSOM::LabelTest(DBNAME).
      */
      if (astr=="-labeltest" && nstr!="") {
	LabelTest(nstr);
	ai++;
	continue;
      }

      /*>	-3TTtest
	Executes IntVector::TernaryTruthTables(). 
      */
      if (astr=="-3TTtest") {
	IntVector::TernaryTruthTables();
	continue;
      }

      /*>	-exit
	Calls exit(42).  Useful after various tests. 
      */
      if (astr=="-exit") {
	exit(42);
	continue;
      }

      /*>	-jam
        Executes for (;;); . Useful after various tests.
      */
      if (astr=="-jam") {
	for (;;) {}
	continue;
      }

      /*>
	-nolinpack
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-nolinpack") {
	UseLinpack(false);
	continue;
      }

      /*>
	-pre707bug
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-pre707bug") {
	Valued::Pre707Bug(true);
	continue;
      }

      /*>
	-pre2122bug
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-pre2122bug") {
	Query::Pre2122Bug(true);
	continue;
      }

      /*>
	-Dinfo
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dinfo") {
	Query::DebugInfo(true);
	continue;
      }

      /*>
	-Dobjreqs
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dobjreqs") {
	Connection::DebugObjectRequests(true);
	continue;
      }

      /*>
	-Dtt
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dtt") {
	Index::DebugTargetType(true);
	continue;
      }

      /*>
	-Dtemp
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dtemp" || astr=="-Dtmp") {
	DebugTempFiles(true);
	continue;
      }

      /*>
	-Dkeep
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dkeep" || astr=="-Dkeep_temp") {
	KeepTemp(true);
	continue;
      }

      /*>
	-Dbounces
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dbounces") {
	Connection::DebugBounces(true);
	continue;
      }

      /*>
	-Dreads
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dreads") {
	Connection::DebugReads(true);
	continue;
      }

      /*>
	-Dwrites | -Dwrites=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 8)=="-Dwrites") {
	Connection::DebugWrites(DebugInteger(astr));
	continue;
      }

      /*>
	-Derf_features | -Derf_features=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 14)=="-Derf_features") {
	Query::DebugErfFeatures(DebugInteger(astr));
	continue;
      }

      /*>
	-Derf_relevance | -Derf_relevance=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 15)=="-Derf_relevance") {
	Query::DebugErfRelevance(DebugInteger(astr));
	continue;
      }

      /*>
	-Dajax | -Dajax=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 6)=="-Dajax") {
	Query::DebugAjax(DebugInteger(astr));
	continue;
      }

      /*>
	-Doctave | -Doctave=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 8)=="-Doctave") {
	DebugOctave(DebugInteger(astr));
	continue;
      }

      /*>
	-Dproxy | -Dproxy=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 7)=="-Dproxy") {
	Connection::DebugProxy(DebugInteger(astr));
	continue;
      }

      /*>
	-Dhttp | -Dhttp=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 6)=="-Dhttp") {
	Connection::DebugHTTP(DebugInteger(astr));
	continue;
      }

      /*>
	-Dsoap | -Dsoap=2
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 6)=="-Dsoap") {
	Connection::DebugSOAP(DebugInteger(astr));
	continue;
      }

      /*>
	-Dsockets
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dsockets") {
	Connection::DebugSockets(true);
	continue;
      }

      /*>
	-Dselects
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dselects")==0) {
	Connection::DebugSelects(DebugInteger(astr));
	continue;
      }

      /*>
	-Dslaves
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dslaves") {
	debug_slaves = true;
	continue;
      }

      /*>
	-Dlocks
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dlocks") {
	Connection::DebugLocks(true);
	DataBase::DebugLocks(true);
	continue;
      }

      /*>
	-Dtimeout=T
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dtimeout=")==0) {
	Connection::TimeOut(astr.c_str()+10);
	continue;
      }

      /*>
	-Dtrap
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dtrap") {
	Simple::TrapAfterError(true);
	picsom::TrapAfterError(true);
	continue;
      }

      /*>
	-Djam
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Djam") {
	Simple::JamAfterError(true);
	continue;
      }

      /*>
	-Dbacktrace
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dbacktrace") {
	Simple::BackTraceBeforeTrap(true);
	BackTraceBeforeTrap(true);
	continue;
      }

      /*>
	-Dtimes
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dtimes")==0) {
	DebugTimes(DebugInteger(astr));
	continue;
      }

      /*>
	-Dmem
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dmem")==0) {
	DebugMemoryUsage(DebugInteger(astr));
	PossiblyShowDebugInformation("After processing -Dmem switch");
	continue;
      }

      /*>
	-Dkeys
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dkeys") {
	Query::DebugAllKeys(true);
	continue;
      }

      /*>
	-Dinterpret
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dinterpret") {
	DebugInterpret(true);
	continue;
      }

      /*>
	-Dscript
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dscript") {
	Analysis::DebugScript(true);
	continue;
      }

      /*>
	-Daccess
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Daccess") {
	RemoteHost::Debug(true);
	continue;
      }

      /*>
	-Drestriction
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Drestriction") {
	Query::DebugRestriction(true);
	continue;
      }

      /*>
	-Dfeatsel
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dfeatsel") {
	Query::DebugFeatSel(true);
	continue;
      }

      /*>
	-Daspects
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Daspects") {
	Query::DebugAspects(true);
	CbirAlgorithm::DebugAspects(true);
	continue;
      }

      /*>
	-Dphases
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dphases") {
	Query::DebugPhases(true);
	continue;
      }

      /*>
	-Dplaceseen
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dplaceseen") {
	Query::DebugPlaceSeen(true);
	CbirAlgorithm::DebugPlaceSeen(true);
	continue;
      }

      /*>
	-Dalgorithms
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dalgorithms") {
	Query::DebugAlgorithms(true);
	// CbirAlgorithm::DebugAlgorithms(true);
	continue;
      }

      /*>
	-Dstages
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dstages") {
	Query::DebugStages(true);
	CbirAlgorithm::DebugStages(true);
	continue;
      }

      /*>
	-Dlists | -Dlists=n
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dlists")==0) {
	Query::DebugLists(DebugInteger(astr));
	CbirAlgorithm::DebugLists(DebugInteger(astr));
	continue;
      }

      /*>
	-Dchecklists
	*DOCUMENTATION MISSING*
	*/
      if (astr=="-Dchecklists") {
	Query::DebugCheckLists(true);
	CbirStageBased::DebugCheckLists(true);
	continue;
      }

      /*>
	-Dweights
	*DOCUMENTATION MISSING*
	*/
      if (astr=="-Dweights") {
	Query::DebugWeights(true);
	CbirAlgorithm::DebugWeights(true);
	continue;
      }

      /*>
	-Dparam
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dparam") {
	Query::DebugParameters(true);
	continue;
      }

      /*>
	-Dsubobjects
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dsubobjects") {
	Query::DebugSubobjects(true);
	CbirAlgorithm::DebugSubobjects(true);
	continue;
      }

      /*>
        -Dcomb_factor
	*DOCUMENTATION MISSING* */

      if (astr.find("-Dcomb_factor")==0) {
	Query::CombFactor(DebugInteger(astr));
	continue;
      }

      /*>
	-Dcbs
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dcbs") {
	Query::DebugCanBeShown(true);
	continue;
      }

      /*>
	-Dclassify
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dclassify") {
	Query::DebugClassify(true);
	CbirAlgorithm::DebugClassify(true);
	continue;
      }

      /*>
	-Dprf
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dprf") {
	Query::DebugPRF(true);
	continue;
      }

      /*>
	-Dselective
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dselective") {
	Query::DebugSelective(true);
	CbirAlgorithm::DebugSelective(true);
	continue;
      }

      /*>
	-Dbrefs
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dbrefs") {
	TSSOM::BackRefSanityChecks(true);
	continue;
      }

      /*>
	-Dbinary
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dbinary") {
	WordInverse::DebugBinary(true);
	continue;
      }

      /*>
	-Dprecalc
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dprecalc") {
	PreCalculatedIndex::DebugPreCalculated(true);
	continue;
      }

      /*>
	-Dmodified
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dmodified") {
	ModifiableFile::DebugModified(true);
	continue;
      }

      /*>
	-Dtictac=V
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dtictac=")==0) {
	tics.Verbose(atoi(astr.c_str()+9));
	continue;
      }

      /*>
	-Dgp
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dgp") {
	GnuPlot::Debug(99);
	continue;
      }

      /*>
	-Dgpkeep
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dgpkeep") {
	simple::GnuPlotData::Keep(true);
	continue;
      }

      /*>
	-Dimg | -Dimg=D
        *DOCUMENTATION MISSING*
	*/
      if (astr.substr(0, 5)=="-Dimg") {
	imagefile::debug_impl(DebugInteger(astr));
	continue;
      }

      /*>
	-Dgt
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dgt") {
	DataBase::DebugGroundTruth(true);
	continue;
      }

      /*>
	-Dgtepx
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dgtexp") {
	DataBase::DebugGroundTruthExpand(true);
	continue;
      }

      /*>
	-Dvobj || -Dvimg
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dvobj" || astr=="-Dvimg") {
	DataBase::DebugVirtualObjects(true);
	continue;
      }

      /*>
	-Dtn
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dtn") {
	DataBase::DebugThumbnails(true);
	continue;
      }

      /*>
	-Dsql
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dsql") {
	DataBase::DebugSql(true);
	continue;
      }

      /*>
	-Dtext
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dtext") {
	DataBase::DebugText(true);
	continue;
      }

      /*>
	-Dorigins
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dorigins") {
	DataBase::DebugOrigins(true);
	continue;
      }

      /*>
	-Dopath
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dopath") {
	DataBase::DebugObjectPath(true);
	continue;
      }

      /*>
	-Dleafdir
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dleafdir")==0) {
	DataBase::DebugLeafDir(DebugInteger(astr));
	continue;
      }

      /*>
	-Ddumpobjs
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Ddumpobjs") {
	DataBase::DebugDumpObjs(true);
	continue;
      }

      /*>
	-Dsvm
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dsvm")==0) {
	SVM::DebugSVM(DebugInteger(astr));
	continue;
      }

      /*>
	-Ddetections
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Ddetections")==0) {
	DataBase::DebugDetections(DebugInteger(astr));
	continue;
      }

      /*>
	-Dcaptioning
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dcaptionings")==0) {
	DataBase::DebugCaptionings(DebugInteger(astr));
	continue;
      }

      /*>
	-Dclassfiles
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dclassfiles") {
	DataBase::DebugClassFiles(true);
	continue;
      }

      /*>
	-Dadapt
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dadapt") {
	Classifier::DebugAdapt(true);
	continue;
      }

      /*>
	-Dskips
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dskips") {
	Classifier::DebugSkips(true);
	continue;
      }

      /*>
	-Dvideo
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dvideo")==0) {
	videofile::debug(DebugInteger(astr));
	continue;
      }

      /*>
	-Dfeatures
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dfeatures")==0) {
	DataBase::DebugFeatures(DebugInteger(astr));
	continue;
      }

      /*>
	-Dsegmentation
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dsegmentation")==0) {
	int i = DebugInteger(astr);
	DataBase::DebugSegmentation(i);
	Segmentation::Verbose(i);
	continue;
      }

      /*>
	-Dimages
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dimages")==0) {
	DataBase::DebugImages(DebugInteger(astr));
	continue;
      }

      /*>
	-Dexpire
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dexpire")==0) {
	Connection::DebugExpire(true);
	continue;
      }

      /*>
	-tmp=dir
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-tmp=")==0) {
	string s = astr.substr(5);
	if (!SetTempDirRoot(s))
	  return ShowError("failed to set temp dir <"+s+">");
	continue;
      }

#ifdef PICSOM_USE_PTHREADS

      /*>
	-threads=N
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-threads=")==0) {
	threads = atoi(astr.c_str()+9);
	continue;
      }

      /*>
	-Pconn
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Pconn")==0) {
	pthreads_connection = true;
	continue;
      }

      /*>
	-Ptssom
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Ptssom")==0) {
	pthreads_tssom = true;
	continue;
      }

      /*>
	-Panalysis
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Panalysis=")==0) {
	Analysis::UsePthreads(atoi(astr.c_str()+11));
	continue;
      }

      /*>
	-Dthreads
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-Dthreads") {
	debug_threads = true;
	continue;
      }
#endif // PICSOM_USE_PTHREADS

      // Show usage/version:
    
      /*>
	-help | -h
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-help" || astr=="-h") {
	ShowUsage(false);
	exit(0);
      }

      /*>
	-version | -v
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-version" || astr=="-v") {
	start_version_only = true;
	continue;
      }
    
      /*>
	-analyse | -a
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-analyse" || astr=="-a") {
	start_analyse = true;
	continue;
      }

      /*>
	-create | -c
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-create" || astr=="-c") {
	start_analyse = true;
	tmp1starg = "create";
	continue;
      }

      /*>
	-insert | -i
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-insert" || astr=="-i") {
	start_analyse = true;
	tmp1starg = "insert";
	continue;
      }

      /*>
	-Dosrs
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-Dosrs")==0) {
	DebugSpeechRecognizer(DebugInteger(astr));
	continue;
      }

      /*>
	-osrs
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-osrs" || astr.find("-osrs=")==0) {
	string languser = "fin";
	if (astr.find("-osrs=")==0)
	  languser = astr.substr(6);
	args_ok = StartSpeechRecognizer(languser);
	continue;
      }

      /*>
	-osrs
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-osrsfile=")==0) {
	args_ok = TestSpeechRecognizer(astr.substr(10));
	continue;
      }

      /*>
	-saveaudio
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-saveaudio") {
	Query::AjaxSaveAudio(true);
	continue;
      }

      /*>
	-playaudio
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-playaudio") {
	Query::AjaxPlayAudio(true);
	continue;
      }

      /*>
	-saveajax
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-saveajax=")==0) {
	Query::AjaxSave(DebugInteger(astr));
	continue;
      }

      /*>
	-mplayer
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-mplayer")==0) {
	Query::AjaxMplayer(true);
	DataBase::InsertMplayer(true);
	if (astr.find("file")!=string::npos)
	  Query::AjaxMplayerFile(true);
	continue;
      }

      /*>
	-erf=xxx
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-erf=")==0) {
	return ShowError("switch -erf=xxx replaced by Q::erfpolicy=xxx");
      }

#ifdef PICSOM_USE_PTHREADS

      /*>
	-ff
        *DOCUMENTATION MISSING*
	*/

      if (astr=="-ff" || astr.find("-ff=")==0) {
	launch_browser_str = astr=="-ff" ? "true" : astr.substr(4);
	browser = "firefox";
	continue;
      }

      /*>
	-gc
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-gc" || astr.find("-gc=")==0) {
	launch_browser_str = astr=="-gc" ? "true" : astr.substr(4);
	//browser = "google-chrome";
	browser = "chromium-browser";
	continue;
      }

      /*>
	-op
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-op" || astr.find("-op=")==0) {
	launch_browser_str = astr=="-op" ? "true" : astr.substr(4);
	browser = "opera";
	continue;
      }

      /*>
	-ko
        *DOCUMENTATION MISSING*
	*/
      if (astr=="-ko" || astr.find("-ko=")==0) {
	launch_browser_str = astr=="-ko" ? "true" : astr.substr(4);
	browser = "konqueror";
	continue;
      }

#endif // PICSOM_USE_PTHREADS

      /*>
	-xsl=xxx
        *DOCUMENTATION MISSING*
	*/
      if (astr.find("-xsl=")==0) {
	Connection::DefaultXSL(astr.substr(5));
	continue;
      }

      // Unrecognized option:
      args_ok = ShowError("Unrecognized command line option: ", astr);
      break;
  }

  if (args_ok) {
    vector<string> aa;
    for (vector<string>::const_iterator aai=args.begin()+ai;
         aai!=args.end(); aai++)
      if (aai->find("Q::")==0 || aai->find("DB::")==0 || aai->find("E::")==0)
        StoreDefaultKeyValue(*aai);
      else
        aa.push_back(*aai);

    CmdLineArgs(aa);
    if (start_analyse) {
      if (tmp1starg!="")
	cmdline_args.insert(cmdline_args.begin(), "analyse="+tmp1starg);
      else if (CmdLineArgs().size())
	cmdline_args[0].insert(0, "analyse=");

    } else
      if (false && !cmdline_args.empty())
        args_ok = ShowError("Extra key=value arguments on command line");
  }

  return args_ok;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::StoreDefaultKeyValue(const string& t, const string& kv) {
    string tcan;

    if (t=="Q" || t=="q" || t=="query" || t=="Query")
      tcan = "query";

    if (t=="DB" || t=="db" || t=="database" || t=="DataBase")
      tcan = "database";

    if (t=="E" || t=="e" || t=="env" || t=="Env")
      tcan = "env";

    if (tcan=="")
      return ShowError("StoreDefaultKeyValue("+t+","+kv+
		       ") failed due unrecognized \""+t+"\"");

    try {
      pair<string,string> kvp = SplitKeyEqualValueNew(kv);
      return StoreDefaultKeyValue(tcan, kvp.first, kvp.second);
    } catch (...) {
      return ShowError("StoreDefaultKeyValue("+t+","+kv+
		       ") key=value split failed");
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::SolveStartUpMode() {
    const vector<string>& args = CmdLineArgs();

    string analysis;
    bool first_file = true;
    for (size_t i = 0; i<args.size() && !start_online && !start_analyse; i++) {
      const string& a = args[i];

      if (a=="-")
	break;

      if (a.find('=')!=string::npos) {
	SolveStartUpModeInner(a);
	continue;
      }

      if (FileExists(a)) {
	if (first_file) {
	  first_file = false;
	  ifstream in(a.c_str());
	  while (in) {
	    string line;
	    getline(in, line);
	    if (line[0]=='#' || line.find_first_not_of(" \t")==string::npos)
	      continue;
	    if (line[0]=='*' && line[line.length()-1]=='*')
	      continue;
	    SolveStartUpModeInner(line);
	    if (start_online||start_analyse)
	      break;
	  }
	}
	else
	  break;
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::SolveStartUpModeInner(const string& line) {
    pair<string,string> kv;
    try {
      kv = SplitKeyEqualValueNew(line);
    } catch (...) {
      return "";
    }

    if (kv.first=="analyse") {
      start_analyse = true;
      return kv.second;
    }

    if (kv.first=="mode") {
      if (kv.second=="analyse" || kv.second=="create" || kv.second=="insert")
	start_analyse = true;
      else if (kv.second=="online")
	start_online = true;
    }

    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::ReadQueries() {
    for (size_t i=0; i<readquery.size(); i++) {
      Query *q = new Query(this, readquery[i]);
      AppendQuery(q, true);
      string oname = readquery[i]+".test-save";
      q->Save(oname, true);
      q->CloseVideoOutput();
    }
    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::DumpQueries() {
  for (size_t i=0; i<dumpquery.size(); i++) {
    Query *q = FindQuery(dumpquery[i], NULL);
    if (!q) {
      ShowError("Query [", dumpquery[i], "] not found.");
      continue;
    }
    cout << dumpquery[i] << " :" << endl;
    q->DumpObjectList();
  }

  if (dumpquery.size())
    exit(0);

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::OpenLog(const string& of) {
    CloseLog(); 
    log.clear(); 
#if defined(sgi) || defined(__linux__)
    Unlink(of);
    log.open(of, ios::app);
#else // __alpha
    log.open(of, ios::app|ios::trunc);
#endif // sgi || __linux__
    return log.good(); 
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetLogging(const string& ofin) {
    string of = ofin;
    if (of=="") {
      time_t currtime = time(NULL); 
      tm *time = localtime(&currtime);
      char tmp[1000];
      strftime(tmp, sizeof(tmp), "/%Y%m%d:", time);  // was %y

      of = RootDir("logs/server")+tmp+ToStr(getpid());
    }

    if (!MakeDirectory(of, true)) {
      ShowError("SetLogging() failed to make directory for <"+of+">");
      return false;
    }
    if (!OpenLog(of)) {
      ShowError("SetLogging() failed to open <"+of+"> for logging");
      return false;
    }

    ios::sync_with_stdio();

#ifdef __alpha
    cerr.tie(0);
#endif // __alpha

    streambuf_cout = cout.rdbuf(log.rdbuf());
    streambuf_cerr = cerr.rdbuf(log.rdbuf());
    streambuf_clog = clog.rdbuf(log.rdbuf());

    int fd = open(of.c_str(), O_WRONLY | O_APPEND, 0664);

    if (!dup2(fd, 1)) {
      ShowError("SetLogging() failed to set stdout");
      return false;
    }
    if (!dup2(fd, 2)) {
      ShowError("SetLogging() failed to set stderr");
      return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::StreamsTest() {
    WriteLog("This text is written with WriteLog()");
    WriteLog("The next lines are written in "
             "log, cout, cerr, clog, fildes(1), fildes(2), stdout, stderr");
  
    log  << "log"  << endl;
    cout << "cout" << endl;
    cerr << "cerr" << endl;
    clog << "clog" << endl;

    const char *fd1 = "fildes(1)\n", *fd2 = "fildes(2)\n";

    int r1 = write(1, fd1, strlen(fd1));
    int r2 = write(2, fd2, strlen(fd2));
    r2 += r1;

    printf("stdout\n");
    fflush(stdout);
    fprintf(stderr, "stderr\n");
    fflush(stderr);

    ShowError("This text is written with ShowError()");
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::ImageTest(const string& f) {
    WriteLog("Opening file <"+f+">");

    try {
      imagefile file(f);
      imagedata data = file.frame(0);
      WriteLog(" "+file.info());
      WriteLog(" "+data.info());

      string fout = f+".test";
      WriteLog("Writing file <"+fout+">");
      imagefile::write(data, fout);

    } catch(const string& err) {
      ShowError(err);
    }
  }

///////////////////////////////////////////////////////////////////////////////

int PicSOM::Execute(int, const char**) {
  if (start_version_only)
    return 0;

  bool ok = true;

  if (start_online) {
    ok = MainLoop();

#ifdef PICSOM_USE_PTHREADS
    JoinThreads(true);
#endif // PICSOM_USE_PTHREADS

    return ok ? 0 : 1;
  }

  else if (start_analyse) {
#ifdef PICSOM_USE_PTHREADS
    if (listen_porta)
      PthreadMainLoop();
#endif // PICSOM_USE_PTHREADS

    ok = Analysis::DoAnalyse(this, cmdline_args);

#ifdef PICSOM_USE_PTHREADS
    JoinThreads(true);
#endif // PICSOM_USE_PTHREADS
  }

  else 
    ok = ShowError("PicSOM::Execute() operation mode unspecified");

  return ok ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ShowUsage(bool error) {
  cerr << myname_str << ": " << (error ? "invocation error: " : "")
       << "USAGE:"                                      << endl
       << "   " <<myname_str<< " [-port[=XX]] [-myport] [-server] [-myserver]"
       << " [-guard]" << endl
    
       << "   " <<myname_str<< " -i|-insert database=xxx - image.jpg ...    "
       << "      (insert images in a new or existing database)" << endl

       << "   " <<myname_str<< " -c|-create ctrl-in                         "
       << "      (create new .cod and .div files)"       << endl

       << "   " <<myname_str<< " -a|-analyse tau|obsprob|div|spread|knn|vote"
       << " args (run analyses)"                         << endl
       << endl;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::MainLoop() {
    Connection *c = NULL;

    for (;;) {
      //MakeFakeRestCaCallbacks();

      int n = SelectableConnections()+MPIconnections();
      if (!n)
	break;

      SaveOldQueries();
      ExpungeOldQueries();

      if (c)
	c->DeleteQuery();

      if (quit)
	return true;

      c = SelectInput();
      if (!c) {
	if (n==1 && soap_server) {
	  struct timespec snap = { 1, 0 };
	  nanosleep(&snap, NULL);
	}
	continue;  // used to be return true
      }

      if (c->IsClosed()) // this might not happen anymore...
	continue; // as of 2015-11-30 this might happen again...

      if (c->IsFailed()) // to get rid of Connection::ReadAndParseXML() failures
	continue;

      if (c->XML() || c->MRML()) {
	c->ReadAndRunXMLOrMRML();
	continue;
      }
    
      if (c==speech_recognizer) {
	ProcessSpeechRecognizerOutput();
	continue;
      }

      if (true) // added 2015-05-21
	continue;

      c->Dump();
      cout << endl;

      return ShowError("PicSOM::MainLoop() failing");
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_PTHREADS
  bool PicSOM::PthreadMainLoop() {
    string msg = "PicSOM::PthreadMainLoop() : ";

    if (listen_thread_set)
      return ShowError(msg+"listen_thread already set");

    int r = pthread_create(&listen_thread, NULL,
			   (CFP_pthread) MainLoopPthreadCall, this);

    if (r)
      return ShowError(msg+"pthread_create() failed");

    listen_thread_set = true;

    //RegisterThread(listen_thread, gettid(), "picsom", "listen", NULL, NULL);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void *PicSOM::MainLoopPthreadCall(void *p) {
    PicSOM *pp = (PicSOM*)p;
    pp->RegisterThread(pthread_self(), gettid(), "picsom", "listen",
		       NULL, NULL);

    static bool tr = true, fl = false;
    return pp->MainLoop() ? &tr : &fl;
  }
#endif // PICSOM_USE_PTHREADS

  /////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_PTHREADS
  bool PicSOM::StartMPIListenThread() {
    string msg = "PicSOM::StartMPIListenThread() : ";

#ifdef PICSOM_USE_MPI    
    if (mpi_listen_thread_set)
      return ShowError(msg+"mpi_listen_thread already set");

    int r = pthread_create(&mpi_listen_thread, NULL,
			   (CFP_pthread)MPILoopPthreadCall, this);

    if (r)
      return ShowError(msg+"pthread_create() failed");

    mpi_listen_thread_set = true;

    //RegisterThread(listen_thread, gettid(), "picsom", "listen", NULL, NULL);

    return true;

#else

    return ShowError(msg+"PICSOM_USE_MPI not defined");

#endif // PICSOM_USE_MPI    
  }

  /////////////////////////////////////////////////////////////////////////////

  void *PicSOM::MPILoopPthreadCall(void *p) {
#ifdef PICSOM_USE_MPI    
    PicSOM *pp = (PicSOM*)p;
    pp->RegisterThread(pthread_self(), gettid(), "picsom", "mpi_listen",
		       NULL, NULL);

    bool ok = true;

    for (;;) {
      Connection *c = pp->mpi_listen_connection;
      Connection *x = Connection::CreateMPI(pp, c->mpi_port, false);

      x->mpi_inter = c->mpi_intra.Accept(c->mpi_port.c_str(),
					 c->mpi_info, c->mpi_root);
      pp->AppendConnection(x);
      
      cout << "Accepted..." << endl;
    }

    static bool tr = true, fl = false;

    return ok ? &tr : &fl;
#else
    return p ? NULL : p;
#endif // PICSOM_USE_MPI    
  }
#endif // PICSOM_USE_PTHREADS

#ifdef PICSOM_USE_MPI    
  static void* test_loop(void* p) {
    PicSOM *pp = (PicSOM*)p;
    pp->RegisterThread(pthread_self(), gettid(), "picsom", "mpi_listen_x",
		       NULL, NULL);

    static bool tr = true, fl = false;
    bool ok = true;

    for (;;) {
      for (size_t i=0; i<1000; i++) {
	Connection *z = pp->GetConnection(i);
	if (!z)
	  break;

	if (z->Type()!=Connection::conn_mpi_down)
	  continue;

	cout << i << " IS conn_mpi" << endl;
	if (z->MPIavailable()) {
	  cout << i << " HAS message waiting" << endl;
	  pair<string,string> tc = z->ReadInMPI();
	  if (tc.first=="")
	    cout << i << " HAS no message ???" << endl;
	  else {
	    cout << i << " message TYPE=" << tc.first << " "
		 << tc.second << endl;
	  }
	}
      }
      sleep(1);
    }

    return ok ? &tr : &fl;
  }
#endif // PICSOM_USE_MPI    

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ShowHelp(Connection& /*conn*/) const {
  /*
  conn.WriteLine("");
  conn.WriteLine("PicSOM version ");
  conn.WriteLine(PicSOM_C_vcid);
  conn.WriteLine("");
  conn.WriteLine("Available commands:\tArguments:");
  conn.WriteLine(" variable=value (set)");
  conn.WriteLine(" +|-|0 (mark)\t\timage");
  conn.WriteLine(" r (run)\t\tinputfile outputfile");
  conn.WriteLine(" q (query)");
  conn.WriteLine(" l (load)\t\tinputfile");
  conn.WriteLine(" p (path)");
  conn.WriteLine(" d (databases)");
  conn.WriteLine(" f (features)");
  conn.WriteLine(" v (variables)");
  conn.WriteLine(" s (statistics)");
  conn.WriteLine(" a (analyse)\t\ttau|obsprob|spread|div");
  conn.WriteLine(" z (restart)");
  conn.WriteLine(" c (command history)");
  conn.WriteLine(" x (exit)");
  conn.WriteLine(" h (help)\t\t[command]");
  conn.WriteLine("");
  */
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ShowHistory(Connection& /*conn*/) const {
#ifdef PICSOM_USE_READLINE
  HIST_ENTRY **list;
  register int i;
  
  list = history_list ();
  if (list /*&& the_connection.IsInteractive()*/) {
    cout << endl << "Command history:" << endl;
    for (i = 0; list[i]; i++)
      //os << i << ": " << list[i]->line << endl;
      conn.WriteLine(list[i]->line);
  }
#else
  // conn.WriteLine("No history.");
#endif // PICSOM_USE_READLINE
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ShowPath(Connection& /*conn*/) const {
  Obsoleted("ShowPath()");
  // conn.WriteLine(Path());
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::Dump(Simple::DumpMode dt, ostream& os) const {
  os << Bold("PicSOM ")   << (void*)this
     << " path="          << ShowString(path_str)
#ifdef PICSOM_USE_PTHREADS
     << " threads="       << threads
#endif // PICSOM_USE_PTHREADS
     << endl;

  if (dt&DumpRecursiveLong) {
//     os << " dataset=";
//     dataset_list.Dump(DumpLong, os);
    
//     os << " selected=";
//     selected.Dump(DumpLong, os);

//     os << " seen=";
//     seen.Dump(DumpLong, os);

//     treesom_list.Dump(dt, os);
  }
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::WriteLogCommon(const char *a, const char *b, 
			    const char *c, const char *d, const char *e,
			    const char *f) const {
  if (quiet)
    return;

  ostream *out = &cout;
  if (IsOpen(*(ofstream*)&log))
    out = (ofstream*)&log;
  
  struct timespec tt;
  SetTimeNow(tt);
  tm mytime;
  tm *time = localtime_r(&tt.tv_sec, &mytime);
  char head[1000] = "";
  if (log_timestamp) {
    // in POSIX 1003.1-2001 %T==%H:%M:%S 
    // but version 3.2 "g++ -pedantic -Wformat" (correctly) doesn't allow it
    strftime(head, sizeof head, "[%d/%b/%Y %H:%M:%S", time);
    sprintf(head+strlen(head), ".%03d] ", int(tt.tv_nsec/1000000));
  }

#ifdef PICSOM_USE_PTHREADS
  if (pthreads_connection||pthreads_tssom||Analysis::UsePthreads()||
      Analysis::UsePthreadsInsert()||DataBase::UsePthreadsDetection()||
      IsSlave()||HasSlaves()) {
    string pthid = ThreadIdentifierUtil()+" ";
    strcat(head, pthid.c_str());
  }
#endif // PICSOM_USE_PTHREADS

  if (log_identity)
    sprintf(head+strlen(head), "%s ", a?a:"<<>>");

  const char *list[] = { b, c, d, e, f };
  bool needs_head = true;

  LogMutexLock();

  for (size_t i=0; i<sizeof(list)/sizeof(*list); i++) {
    const char *str = list[i]?list[i]:"";
    char *cpy = new char[strlen(str)+1];
    char *ptr = strcpy(cpy, str);

    while (ptr && *ptr) {
      if (needs_head) {
	*out << head;
	needs_head = false;
      }

      char *nl = strchr(ptr, '\n');
      if (nl)
	*nl = 0;

      *out << ptr;

      if (nl) {
	*out << endl;
	needs_head = true;
	ptr = nl+1;
      } else
	ptr = NULL;
    }
    delete [] cpy;
  }
  if (!needs_head)
    *out << endl;

  LogMutexUnLock();
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::WriteLogCommon(const char *head, ostringstream& os) const {
  WriteLogCommon(head?head:"!!!", os.str().c_str());
}

  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::ExecuteSystem(const string& cmd, bool pre, bool ok, bool err) {
    if (pre)
      WriteLog("Executing system("+cmd+")");

    struct timespec start = TimeNow();
    int r = system(cmd.c_str());
    
    if (ok) {
      if (!r)
	WriteLog("Successfully executed system("+cmd+") in "+
		 ToStr(TimeDiff(TimeNow(), start))+" s");
      else if (!err)
	WriteLog("Failed to execute system("+cmd+") status=", ToStr(r));
    }
    
    if (err && r)
      ShowError("Failed to execute system("+cmd+") status=", ToStr(r));
    
    return r;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  pair<bool,vector<string> > 
  PicSOM::ShellExecute(const vector<string>& cmdin,
		       bool also_err, bool verbose) {
    pair<bool,vector<string> >  err = make_pair(false, vector<string>());

    static size_t n = 0;
    string tmp = TempDirPersonal()+"/shell_execute_"+ToStr(n++)+".out";
    vector<string> cmd = cmdin;
    bool inredir = false;
    for (size_t i=0; !inredir && i<cmd.size(); i++)
      if (cmd[i].size() && cmd[i][0]=='<')
	inredir = true;
    if (!inredir)
      cmd.push_back("</dev/null");
    cmd.push_back("1>"+tmp);
    if (also_err)
      cmd.push_back("2>&1");
    bool ok = ExecuteSystem(cmd, verbose, verbose, false)==0;

    string st = FileToString(tmp);
    if (!keep_temp)
      Unlink(tmp);

    vector<string> l = SplitInSomething("\n", true, st);

    return make_pair(ok, l);

    // old version:
    // Unlink(tmp);
    //
    // if (!ok)
    //   return err;
    //
    // vector<string> l = SplitInSomething("\n", true, st);
    //
    // return make_pair(true, l);
  }

  /////////////////////////////////////////////////////////////////////////////

  int PicSOM::SolveGpuDevice() {
    // obs!  should obey SLURM_JOB_GPUS GPU_DEVICE_ORDINAL CUDA_VISIBLE_DEVICES

    double utilization_limit = 50; // percent
    
    string smi = "/usr/bin/nvidia-smi";
    if (!FileExists(smi))
      return -1;

    bool verbose = true;
    vector<string> cmd { smi, "-q", "-d", "UTILIZATION"	/*, "2>&1"*/ };
    auto rr = ShellExecute(cmd, false, verbose);
    if (rr.first==false)
      return -2;

    vector<string> l = rr.second;

    vector<float> gpu;
    for (size_t i=0; i<l.size(); i++) {
      if (verbose)
	cout << l[i] << endl;
      if (l[i].substr(0, 4)=="GPU " && i+2<l.size() &&
	  l[i+2].find(" Gpu ")!=string::npos) {
	size_t p =  l[i+2].find(" : ");
	size_t q =  l[i+2].find(" %");
	if (p!=string::npos && q!=string::npos && p<q)
	  gpu.push_back(atof(l[i+2].substr(p+3, q-p-3).c_str()));
	else if (p!=string::npos)
	  gpu.push_back(-1);
      }
    }

    int id = -1;
    for (size_t i=0; id==-1 && i<gpu.size(); i++)
      if (gpu[i]<=utilization_limit)
	id = i;

    string res = ToStr(gpu.size())+" Nvidia GPUs found";
    if (gpu.size())
      res += ", utilization: "+ToStr(gpu);
    res += ", selected: "+ToStr(id);

    const map<string,string>& e = Environment();
    if (e.find("CUDA_VISIBLE_DEVICES")!=e.end()) {
      res += ", CUDA_VISIBLE_DEVICES="+e.at("CUDA_VISIBLE_DEVICES");
      id = 0;
    }

    WriteLog(res);

    return id;
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::SolveArchitecture() const {
#ifdef HAVE_SYS_UTSNAME_H
    bool debug = false;

    struct utsname uts;
    if (uname(&uts) == -1) {
      ShowError("PicSOM::SolveArchitecture() : uname() call failed");
      return ""; // should we provide some default architecture?
    }

    if (debug) {
      printf ("System name     : %s\n", uts.sysname);
      printf ("Node name       : %s\n", uts.nodename);
      printf ("Version         : %s\n", uts.version); 
      printf ("Release         : %s\n", uts.release);
      printf ("Machine         : %s\n", uts.machine);
    }

    string sysname = uts.sysname, machine = uts.machine;

    if (sysname=="IRIX64")
      return "irix";

    if (sysname=="OSF1")
      return "alpha";

    if (sysname=="Darwin")
      return "darwin";

    if (sysname=="Linux") {
      if (machine=="x86_64")
	return "linux64";
      else
	return "linux";
    }
    // fallback:
    return sysname;

#elif defined(__MINGW64__)
    return "mingw64";

#elif defined(__MINGW32__)
    return "mingw32";

#else
    return "noarch";
    // return "win32";
#endif // HAVE_SYS_UTSNAME_H
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::SolveLinuxDistribution() const {
    string f = "/usr/bin/lsb_release";
    if (FileExists(f)) {
      FILE *pipe = popen((f+" -d").c_str(), "r");
      if (!pipe)
	return "";

      char tmp[10000];
      char *r = fgets(tmp, sizeof tmp, pipe);
      pclose(pipe);
      if (!r)
	return "";

      string line = tmp;
      if (line.find("Description:")!=0)
	return "";
      size_t p = line.find_first_of(" \t");
      if (p==string::npos)
	return "";
      p = line.find_first_not_of(" \t", p);
      if (p==string::npos)
	return "";

      line.erase(0, p);
      p = line.find_first_of("\n\r");
      if (p!=string::npos)
	line.erase(p);

      return line;
    }

    f = "/etc/system-release";
    if (FileExists(f)) {
      string line = FileToString(f);
      size_t p = line.find_last_not_of(" \t\n\r");
      if (p!=string::npos)
	line.erase(p+1);
      return line;
    }
     
    return "";
  }

  /////////////////////////////////////////////////////////////////////////////

  static string av_version_str(unsigned int v) {
    unsigned int major = v>>16;
    unsigned int minor = (v>>8)%256;
    unsigned int micro = v%256;
    return ToStr(major)+"."+ToStr(minor)+"."+ToStr(micro);
  }

#define xstringize(x) #x
#define stringize(x) xstringize(x)

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::SolveVersionData() const {
    string dummy = av_version_str(0);

    string v = Release();
    if (v!="")
      SetVersionData("Release", v);

    SetVersionData("PicSOM.C", PicSOM_C_Version());
    SetVersionData("PicSOM.h", PicSOM_h_Version());
    //SetVersionData("picsom.C", picsom_C_Version());

    char compilerinfo[1000] = "-unknown-";
#ifdef __GNUC__
    sprintf(compilerinfo, "GCC %d.%d", __GNUC__, __GNUC_MINOR__);
#ifdef __GNUC_PATCHLEVEL__
    sprintf(compilerinfo+strlen(compilerinfo), ".%d", __GNUC_PATCHLEVEL__);
#endif // __GNUC_PATCHLEVEL__
#ifdef __INTEL_COMPILER
    sprintf(compilerinfo+strlen(compilerinfo), " INTEL COMPILER %d",
            __INTEL_COMPILER);
#endif // __INTEL_COMPILER
#elif sgi
    sprintf(compilerinfo, "MIPSpro %d", _COMPILER_VERSION);
#elif __alpha
    char vstr[100];
    int dv = __DECCXX_VER, dvt = (dv/10000)%10;
    sprintf(vstr, "%c%d.%d-%03d", dvt==6?'T':dvt==8?'S':dvt==9?'V':'?',
            dv/10000000, (dv%10000000)/100000, dv%10000);
    sprintf(compilerinfo, "Compaq C++ %s (%d)", vstr, dv);
#elif _MSC_VER
    sprintf(compilerinfo+strlen(compilerinfo), " MicroSoft COMPILER %d",
            _MSC_VER);
#endif // __GNUC__ / __INTEL_COMPILER / sgi / __alpha / _MSC_VER

    SetVersionData("compiler",  compilerinfo);
    if (string(CODEFLAGS)!="--unknown--")
      SetVersionData("CODEFLAGS", CODEFLAGS);

#ifdef _POSIX_C_SOURCE
    SetVersionData("_POSIX_C_SOURCE", ToStr(_POSIX_C_SOURCE));
#endif // _POSIX_C_SOURCE

    char datetime[100];
    sprintf(datetime, "%s %s", __DATE__, __TIME__);
    SetVersionData("compiletime", datetime);

    stringstream alist;
    CbirAlgorithm::PrintAlgorithmList(alist, false);
    SetVersionData("Algorithms", alist.str());

    stringstream mlist;
    Index::PrintMethodList(mlist, false);
    SetVersionData("Methods", mlist.str());

    stringstream flist;
    Feature::PrintFeatureList(flist, false);
    SetVersionData("Features", flist.str());

    stringstream slist;
    Segmentation::ListMethods(slist, true);
    SetVersionData("Segmentations", slist.str());

#ifdef PICSOM_USE_SLMOTION
    SetVersionData("slmotion", slmotion::SLMOTION_VCID);
#endif //  PICSOM_USE_SLMOTION

#ifdef PICSOM_USE_OD
    SetVersionData("ObjectDetection", ObjectDetection::Version());
#endif //  PICSOM_USE_OD

#if defined(CUDA_VERSION)
    int cdv = 0, crv = 0;
    cudaDriverGetVersion(&cdv);
    cudaRuntimeGetVersion(&crv);
    ostringstream cudastr;
    cudastr << CUDA_VERSION << " (" << cdv << "," << crv << ")";
    SetVersionData("CUDA", cudastr.str());
#endif // CUDA_VERSION

    vector<string> pieces;
#if defined(CAFFE_VERSION) && defined(PICSOM_USE_CAFFE)
    pieces.push_back("caffe_" CAFFE_VERSION);
#endif // CAFFE_VERSION && PICSOM_USE_CAFFE
#if defined(CAFFE2_VERSION) && defined(PICSOM_USE_CAFFE2)
    pieces.push_back("caffe2_"+ToStr(CAFFE2_VERSION));
#endif // CAFFE2_VERSION && PICSOM_USE_CAFFE2
#ifdef GOOGLE_PROTOBUF_VERSION
    pieces.push_back("protobuf_"+ToStr(GOOGLE_PROTOBUF_VERSION));
#endif // GOOGLE_PROTOBUF_VERSION
#if defined(TH_INDEX_BASE)
    pieces.push_back("torch");
#endif // defined(TH_INDEX_BASE)
#ifdef INTEL_MKL_VERSION
    char mklvers[100];
    MKL_Get_Version_String(mklvers, sizeof mklvers);
    pieces.push_back("intel_mkl_"+ToStr(INTEL_MKL_VERSION)+" ("+mklvers+")");
#endif // INTEL_MKL_VERSION
    string ob = OpenBlasVersion();
    if (ob!="")
      pieces.push_back("OpenBlas "+ob);
#ifdef PICSOM_USE_CSOAP
    pieces.push_back("csoap");
#endif // PICSOM_USE_CSOAP
#ifdef THRIFT_VERSION
    pieces.push_back("Thrift_" THRIFT_VERSION " ()");
#endif // THRIFT_VERSION
#ifdef MAD_VERSION
    pieces.push_back("mad_" MAD_VERSION " ("+string(mad_version)+")");
#endif // MAD_VERSION
#if defined(PICSOM_USE_AUDIO) && defined(SNDFILE_1)
    pieces.push_back("sndfile"
		     " ("+string(sf_version_string())+")");
#endif // PICSOM_USE_AUDIO && SNDFILE_1
#if defined(PICSOM_USE_AUDIO) && defined(HAVE_AUDIOFILE_H)
    pieces.push_back("audiodfile_"
      +ToStr(LIBAUDIOFILE_MAJOR_VERSION)+"."+ToStr(LIBAUDIOFILE_MINOR_VERSION)
		     +"."+ToStr(LIBAUDIOFILE_MICRO_VERSION));
#endif // PICSOM_USE_AUDIO && HAVE_AUDIOFILE_H
#ifdef HAVE_FFTW3_H
    pieces.push_back("fftw3 ("+ToStr(fftw_version)+")");
#endif // HAVE_FFTW3_H
#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(CV_VERSION) && defined(PICSOM_USE_OPENCV)
    pieces.push_back("opencv_" CV_VERSION);
#endif // HAVE_OPENCV2_CORE_CORE_HPP && CV_VERSION && PICSOM_USE_OPENCV
#ifdef VL_VERSION_STRING
    pieces.push_back("vlfeat_" VL_VERSION_STRING
      " ("+string(vl_get_version_string())+")");
#endif // VL_VERSION_STRING
#ifdef HAVE_FLANDMARK_DETECTOR_H
    pieces.push_back("flandmark");
#endif // HAVE_FLANDMARK_DETECTOR_H
#if defined(PICSOM_HAVE_OCTAVE) && defined(PICSOM_USE_OCTAVE)
    pieces.push_back("octave_" OCTAVE_VERSION);
#endif // PICSOM_HAVE_OCTAVE && PICSOM_USE_OCTAVE
#ifdef HAVE_MYSQL_H
    pieces.push_back("mysql_" MYSQL_SERVER_VERSION
      " ("+string(mysql_get_client_info())+")");
#endif // HAVE_MYSQL_H
#ifdef HAVE_SQLITE3_H
    pieces.push_back("sqlite3_" SQLITE_VERSION
      " ("+string(sqlite3_libversion())+")");
#endif // HAVE_SQLITE3_H
#ifdef HAVE_LMDB_H
    pieces.push_back(MDB_VERSION_STRING
		     " ("+string(mdb_version(NULL, NULL, NULL))+")");
#endif // HAVE_LMDB_H
#ifdef HAVE_MAGIC_H
    pieces.push_back("libmagic");
#endif // HAVE_MAGIC_H
#ifdef HAVE_OPENSSL_SSL_H
    pieces.push_back(OPENSSL_VERSION_TEXT);
#endif // HAVE_OPENSSL_SSL_H
#ifdef PICSOM_USE_MPI
    int mpiv = 0, mpisv = 0;
    MPI::Get_version(mpiv, mpisv);
    pieces.push_back("MPI ("+ToStr(mpiv)+"."+ToStr(mpisv)+")");
#endif // PICSOM_USE_MPI
#ifdef HAVE_WN_H
    pieces.push_back("WordNet"
      " ("+string(wnrelease)+")");
#endif // HAVE_WN_H
#ifdef HAVE_ASPELL_H
    pieces.push_back("Aspell"
      " (" /* +string(aspell_version_string())+ */ ")");
#endif // HAVE_ASPELL_H
    if (HasLinpack())
      pieces.push_back("linpack("+
        string(UseLinpack()?"":"not ")+"used)");

#if defined(HAVE_LIBAVCODEC_AVCODEC_H) && defined(HAVE_LIBAVCODEC)
    pieces.push_back("libavcodec_" stringize(LIBAVCODEC_VERSION) " ("+
    		     av_version_str(avcodec_version())+")");
#endif // HAVE_LIBAVCODEC_AVCODEC_H
#if defined(HAVE_LIBAVFORMAT_AVFORMAT_H) && defined(HAVE_LIBAVFORMAT)
    pieces.push_back("libavformat_" stringize(LIBAVFORMAT_VERSION) " ("+
    		     av_version_str(avformat_version())+")");
#endif // HAVE_LIBAVFORMAT_AVFORMAT_H
// #ifdef HAVE_LIBAVDEVICE_AVDEVICE_H
//     pieces.push_back("libavdevice_" stringize(LIBAVDEVICE_VERSION) " ("+
// 		     av_version_str(avdevice_version())+")");
// #endif // HAVE_LIBAVDEVICE_AVDEVICE_H
#if defined(HAVE_LIBAVUTIL_AVUTIL_H) && defined(HAVE_LIBAVUTIL)
    pieces.push_back("libavutil_" stringize(LIBAVUTIL_VERSION) " ("+
    		     av_version_str(avutil_version())+")");
#endif // HAVE_LIBAVUTIL_AVUTIL_H
// #ifdef HAVE_LIBAVFILTER_AVFILTER_H
//     pieces.push_back("libavfilter_" stringize(LIBAVFILTER_VERSION) " ("+
// 		     av_version_str(avfilter_version())+")");
// #endif // HAVE_LIBAVFILTER_AVFILTER_H
#if defined(HAVE_LIBSWSCALE_SWSCALE_H) && defined(HAVE_LIBSWSCALE)
    pieces.push_back("libswscale_" stringize(LIBSWSCALE_VERSION) " ("+
    		     av_version_str(swscale_version())+")");
#endif // HAVE_LIBSWSCALE_SWSCALE_H

#ifdef HAVE_JSON_JSON_H
    pieces.push_back("jsoncpp_"+string(JSONCPP_VERSION_STRING));
#endif // HAVE_JSON_JSON_H

#ifdef HAVE_ZIP_H
    pieces.push_back("libzip_" LIBZIP_VERSION);
#endif // HAVE_ZIP_H

#ifdef TIFFLIB_VERSION
    pieces.push_back("TIFF_" stringize(TIFFLIB_VERSION));
#endif // TIFFLIB_VERSION

#ifdef GIFLIB_MAJOR
    pieces.push_back("giflib_" stringize(GIFLIB_MAJOR) "."
		     stringize(GIFLIB_MINOR) "." stringize(GIFLIB_RELEASE));
#endif // GIFLIB_MAJOR     

#ifdef H5_VERSION
    unsigned int majnum = 0, minnum = 0, relnum = 0;
    H5get_libversion(&majnum, &minnum, &relnum);
    pieces.push_back("hdf5_" H5_VERSION " ("+ToStr(majnum)+"."+
		     ToStr(minnum)+"."+ToStr(relnum)+")");
#endif // H5_VERSION    

    string piecestr = JoinWithString(pieces, ", ");
    for (size_t split=60; piecestr.size()>split; split+=60) {
      size_t p = piecestr.rfind(", ", split);
      if (p!=string::npos) {
	piecestr.replace(p+1, 1, "\n\t");
	split = p;
      }
    }
    SetVersionData("Pieces", piecestr);

#ifdef PY_VERSION
    string pythonv = Py_GetVersion();
    for (;;) {
      size_t p = pythonv.find("\n");
      if (p==string::npos)
	break;
      pythonv[p] = ' ';
    }
    SetVersionData("Python", string(PY_VERSION)+" ("+pythonv+")");
#endif // PY_VERSION

#ifdef LUAJIT_VERSION_SYM
    SetVersionData("LuaJIT", string(stringize(LUAJIT_VERSION_SYM))+" ()");
#endif // LUAJIT_VERSION_SYM

#ifdef RAPTOR_VERSION_STRING
    SetVersionData("Raptor", string(RAPTOR_VERSION_STRING)+" ("+raptor_version_string+")");
#endif // RAPTOR_VERSION_STRING

#ifdef RASQAL_VERSION_STRING
    SetVersionData("Rasqal", string(RASQAL_VERSION_STRING)+" ("+rasqal_version_string+")");
#endif // RASQAL_VERSION_STRING

#ifdef TF_VERSION_STRING
    SetVersionData("TensorFlow", string(TF_VERSION_STRING)+" ("+")");
#endif // TF_VERSION_STRING

#ifdef USE_GVT
    SetVersionData("USE_GVT",  "*** defined ***");
#endif // USE_GVT

#ifdef USE_MRML
    SetVersionData("USE_MRML",  "*** defined ***");
#endif // USE_MRML

#ifdef PICSOM_USE_READLINE
    SetVersionData("PICSOM_USE_READLINE",  "*** defined ***");
#endif // PICSOM_USE_READLINE

#if defined(HAVE_OPENCV2_CORE_CUDA_HPP) && defined(PICSOM_USE_OPENCV)
#ifdef PICSOM_USE_TENSORFLOW
    bool show_gpu_mem = false;
#else
    bool show_gpu_mem = true;
#endif // PICSOM_USE_TENSORFLOW

    bool use_opencv_cuda = true;

    if (use_opencv_cuda && nvidia_smi_ok) {
      int ncuda = cv::cuda::getCudaEnabledDeviceCount();
      stringstream cmsg;
      cmsg << ncuda;
      if (ncuda==0)
	cmsg << " (no CUDA support in OpenCV)";
      if (ncuda==-1)
	cmsg << " (no devices)";
      SetVersionData("cv::cuda::CudaEnabledDeviceCount", cmsg.str());
      vector<pair<cv::cuda::FeatureSet,string> > fea {
	{ cv::cuda::FEATURE_SET_COMPUTE_10, "10" },
	{ cv::cuda::FEATURE_SET_COMPUTE_11, "11" },
	{ cv::cuda::FEATURE_SET_COMPUTE_12, "12" },
	{ cv::cuda::FEATURE_SET_COMPUTE_13, "13" },
	{ cv::cuda::FEATURE_SET_COMPUTE_20, "20" },
	{ cv::cuda::FEATURE_SET_COMPUTE_21, "21" },
	{ cv::cuda::FEATURE_SET_COMPUTE_30, "30" },
	{ cv::cuda::FEATURE_SET_COMPUTE_35, "35" },
	{ cv::cuda::GLOBAL_ATOMICS, 	    "global_atomics" },
	{ cv::cuda::SHARED_ATOMICS, 	    "shared_atomics" },
	{ cv::cuda::NATIVE_DOUBLE,  	    "native_double"  },
	{ cv::cuda::WARP_SHUFFLE_FUNCTIONS, "warp_shuffle_functions" },
	{ cv::cuda::DYNAMIC_PARALLELISM,    "dynamic_parallelism" }
      };

      if (ncuda!=0) {
	string cap_built;
	for (size_t i=0; i<fea.size(); i++) 
	  if (cv::cuda::TargetArchs::builtWith(fea[i].first))
	    cap_built += (cap_built!=""?" ":"")+fea[i].second;

	string cap_has, cap_ptx, cap_bin;
	for (int maj=1; maj<10; maj++)
	  for (int min=0; min<10; min++) {
	    if (cv::cuda::TargetArchs::has(maj, min))
	      cap_has += (cap_has!=""?" ":"")+ToStr(maj)+"."+ToStr(min);
	    if (cv::cuda::TargetArchs::hasPtx(maj, min))
	      cap_ptx += (cap_ptx!=""?" ":"")+ToStr(maj)+"."+ToStr(min);
	    if (cv::cuda::TargetArchs::hasBin(maj, min))
	      cap_bin += (cap_bin!=""?" ":"")+ToStr(maj)+"."+ToStr(min);
	  }
    	  
	SetVersionData("  compute capability built", cap_built);
	SetVersionData("  compute capability has", cap_has);
	SetVersionData("  compute capability ptx", cap_ptx);
	SetVersionData("  compute capability bin", cap_bin);
      }

      for (int j=0; j<ncuda; j++) {
	cv::cuda::DeviceInfo di(j);
	stringstream str;
	str << di.deviceID() << " \""
	    << di.name() << "\" "
	    << di.majorVersion() << "." << di.minorVersion() << " "
	    << "#multiproc=" << di.multiProcessorCount();

	if (show_gpu_mem)
	  str << " memory=" << di.freeMemory() << " / "<< di.totalMemory();
    
	str << " supports=[";
	bool firsts = true;
	for (size_t i=0; i<fea.size(); i++) 
	  if (di.supports(fea[i].first)) {
	    str << (firsts?"":" ") << fea[i].second;
	    firsts = false;
	  }

	str << "] compatible=" << (di.isCompatible()?"YES":"NO");
	SetVersionData("  #"+ToStr(j), str.str());
      }
    }
#endif // HAVE_OPENCV2_CORE_CUDA_HPP && PICSOM_USE_OPENCV

    SetVersionData("imagedata", imagedata::version());
    SetVersionData("imagefile", imagefile::version());
    SetVersionData("imagefile::impl", imagefile::impl_version());
    SetVersionData("imagefile::impl_library",
                   imagefile::impl_library_version());

    const imagefile::version_array& va=imagefile::impl_used_library_versions();
    for (imagefile::version_array::const_iterator vi=va.begin();
         vi!=va.end(); vi++)
      SetVersionData("imagefile::impl_library::"+vi->first, vi->second);

    char libxmlinfo[100];
    sprintf(libxmlinfo, "%s / %s", LIBXML_DOTTED_VERSION, xmlParserVersion);
    SetVersionData("xmlParserVersion", libxmlinfo);

    char zlibinfo[1000];
    sprintf(zlibinfo, "%s / %s", ZLIB_VERSION, zlibVersion());
    SetVersionData("ZLIB_VERSION", zlibinfo);
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::WriteLogVersion() {
    for (version_data_type::const_iterator i = version_data.begin();
         i!=version_data.end(); i++)
      WriteLog(i->first, " = ", i->second);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLversions(XmlDom& xml) const {
    XmlDom vers = xml.Element("versions");

    for (auto const& i : version_data) {
      XmlDom e = vers.Element("piece");
      e.Prop("name",    i.first);
      e.Prop("version", i.second);
    }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::AddToXMLconnectionlist(XmlDom& xml) const {
  XmlDom cl = xml.Element("connectionlist");

  bool ok = true;
  for (size_t i=0; ok && i<Connections(false, false); i++)
    ok = GetConnection(i)->AddToXMLconnection(cl);

  return ok;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLanalysislist(XmlDom& xml, const string& spec) const {
    XmlDom a = xml.Element("analysislist");

    bool ok = true;

    for (vector<Analysis*>::const_iterator i=analysis.begin();
	 ok && i!=analysis.end(); i++)
      ok = AddToXMLanalysis(a, *i, NULL, spec);
  
#ifdef PICSOM_USE_PTHREADS
    for (thread_list_t::const_iterator i=thread_list.begin();
	 ok && i!=thread_list.end(); i++)
      if (i->type=="analysis" || i->type=="soapexecute")
	ok = AddToXMLanalysis(a, NULL, &*i, spec);
#endif // PICSOM_USE_PTHREADS

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLanalysis(XmlDom& xml, const string& name,
				const string& spec) const {
    pid_t tid = 0;
    if (name.find_first_not_of("0123456789")==string::npos)
      tid = (pid_t)atoi(name.c_str());

    const thread_info_t *e = tid ? FindThread(tid) : FindThread(name);

    string err;
    if (!e)
      err = "thread \""+name+"\" inexistent";
    else if (e->type!="analysis")
      err = "thread \""+name+"\" not analysis";

    if (err!="")
      return ShowError("AddToXMLanalysis() : "+err);

    return AddToXMLanalysis(xml, (Analysis*)e->object, e, spec);
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::AddToXMLanalysis(XmlDom& xml, const Analysis *ana,
			      const thread_info_t *e,
			      const string& spec) const {
  if (!ana && e)
    ana = (Analysis*)e->object;

  if (!ana)
    return ShowError("AddToXMLanalysis() : no object");

  XmlDom a = xml.Element("analysis");

  a.Element("identity", ana->Identity());

  stringstream ss;
  ss << (void*)ana;
  a.Element("memory", ss.str());

  if (e)
    AddToXMLthread(a, e);

#ifdef PICSOM_USE_PTHREADS
  if (e && ThreadState(e)=="ready") {
    if (spec!="brief") {
      const Analysis::analyse_result& res = ana->ThreadResult(e->data);
      ana->AddToXMLanalyse_result(a, res);
    }
    if (spec.find("fullxmlresult")!=string::npos)
      ana->AddToXMLxml_result(a);
  }
#endif // PICSOM_USE_PTHREADS

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLstatus(XmlDom& xml) const {
    XmlDom s = xml.Element("status");

    s.Element("hostname", HostName(false));
    s.Element("pid",      ToStr(getpid()));
    s.Element("started",  TimeDotString(start_time));

    double load = -1.0;
    if (getloadavg(&load, 1)==1 || true) {
      stringstream ss;
      ss.precision(2);
      ss << load;
      s.Element("load", ss.str());
    }
  
    int cpucount = CpuCount();
    if (cpucount)
      s.Element("cpucount", cpucount);

    char tmp[100];
    sprintf(tmp, "%.2g", ((PicSOM*)this)->CpuUsage());
    string cpuusage = ToStr(tmp);
    s.Element("cpuusage", cpuusage);

    return AddToXMLanalysislist(s, "brief") && AddToXMLthreadlist(s);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLslavelist(XmlDom& xml) const {
    XmlDom l = xml.Element("slavelist");

    bool ok = true;

    RwLockReadSlaveList();

    for (auto i=slave_list.begin(); ok && i!=slave_list.end(); i++)
      ok = AddToXMLslave(l, &*i);

    RwUnlockReadSlaveList();

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLslave(XmlDom& xml, const slave_info_t *e) const {
    XmlDom s = xml.Element("slave");

    s.Element("hostspec",   e->hostspec);
    s.Element("hostname",   e->hostname);
    s.Element("executable", e->executable);
    //s.Element("command",  join(e->command));
    s.Element("status",     e->status);
    s.Element("load",    	  ToStr(e->load));
    s.Element("cpucount",   ToStr(e->cpucount));
    s.Element("cpuusage",   ToStr(e->cpuusage));
    s.Element("submitted",  TimeString(e->submitted));
    s.Element("started",    TimeString(e->started));
    s.Element("updated",    TimeString(e->updated));
    s.Element("spare",      TimeString(e->spare));
    if (e->conn)
      s.Element("conn",     e->conn->LogIdentity());

    if (e->threads.size()) {
      XmlDom th = s.Element("threadlist");
      for (thread_list_t::const_iterator i=e->threads.begin();
	   i!=e->threads.end(); i++)
	AddToXMLthread(th, &*i);
    }

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLthreadlist(XmlDom& xml) const {
#ifdef PICSOM_USE_PTHREADS
    if (!ThreadListSanityCheck())
      return false;
#endif // PICSOM_USE_PTHREADS

    XmlDom l = xml.Element("threadlist");

    bool ok = true;

#ifdef PICSOM_USE_PTHREADS
    for (thread_list_t::const_iterator i=thread_list.begin();
	 ok && i!=thread_list.end(); i++)
      ok = AddToXMLthread(l, &*i);
#endif // PICSOM_USE_PTHREADS

    return ok;
  }

  ///////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLthread(XmlDom& xml, const thread_info_t *e) const {
    XmlDom t = xml.Element("thread");

    t.Element("name",    	   e->name);
#ifdef PICSOM_USE_PTHREADS
    if (e->thread_set)
      t.Element("pthread",   ThreadIdentifier(*e));
#endif // PICSOM_USE_PTHREADS
    t.Element("phase",   	   e->phase);
    t.Element("type",    	   e->type);
    t.Element("text",    	   e->text);
    t.Element("state",   	   ThreadState(e));
    t.Element("slave_state", e->slave_state);
    t.Element("showname",    e->showname);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLthread_rip(XmlDom& xml, const string& n) const {
    XmlDom t = xml.Element("thread");

    t.Element("name",  n);
    t.Element("phase", "terminated");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::ThreadInfoString(const thread_info_t& e, bool slave) const {
    stringstream ss;
    ss << "name=" << e.name << " thread_set=" << (int)e.thread_set
#ifdef PICSOM_USE_PTHREADS
       << " id=" << ThreadIdentifier(e)
#endif // PICSOM_USE_PTHREADS
       << " phase=" << e.phase
       << " type=" << e.type << " text=\"" << e.text << "\"";

    if (e.type=="analyse" && !slave)       // e.data may point to different
      ss << " state=" << ThreadState(&e);  // types of objects, which is bad...

    ss << " slave_state=" << e.slave_state;

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& PicSOM::ThreadState(const thread_info_t *e) const {
    static string null = "null", unknown = "unknown";
    if (!e || !e->object || !e->data)
      return null;

    if (e->type!="analysis")
      return unknown;
#ifdef PICSOM_USE_PTHREADS
    return ((Analysis*)e->object)->ThreadState(e->data);
#else
    return unknown;
#endif // PICSOM_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::WriteLogProcessInfo() {
    char pthreads[100] = "";
#ifdef CLASSIFIER_USE_PTHREADS
    strcat(pthreads, "in Classifier.C");
#endif // CLASSIFIER_USE_PTHREADS
#ifdef PICSOM_USE_PTHREADS
    sprintf(pthreads+strlen(pthreads),
            "%s in PicSOM.C (threads=%d, conn=%d, tssom=%d, analysis=%d)",
            *pthreads?" and":"", threads, pthreads_connection, pthreads_tssom,
	    Analysis::UsePthreads());
#endif // PICSOM_USE_PTHREADS
    if (*pthreads)
      WriteLog("pthreads = ", pthreads);

    char tmp[1000], thrid[100] = "";
#ifdef PICSOM_USE_PTHREADS
    string th = "/"+ThreadIdentifierUtil();
    strcpy(thrid, th.c_str());
#endif // PICSOM_USE_PTHREADS
    sprintf(tmp,
	    "user=%s, host=%s, ip=%s, arch=%s, distr=%s, pid=%d%s,"
	    " gpudevice=%d, development=%d",
	    UserName().c_str(), HostName().c_str(), HostAddr().c_str(),
	    sysarchitecture.c_str(),
	    linuxdistr.c_str(), getpid(), thrid, gpudevice, Development());
    WriteLog(tmp);

#ifdef CUDA_VERSION
    stringstream ss;
    ss << "CUDA";
    int driverVersion = 0, runtimeVersion = 0, cc = 0;
    cudaDriverGetVersion(&driverVersion);
    cudaRuntimeGetVersion(&runtimeVersion);
    cudaGetDeviceCount(&cc);
    ss << " driver=" << driverVersion/1000 << "." << (driverVersion%100)/10
       << " runtime=" << runtimeVersion/1000 << "." << (runtimeVersion%100)/10
       << " devicecount=" << cc;

    WriteLog(ss.str());
    for (int ci=0; ci<cc; ci++) {
      struct cudaDeviceProp cprop;
      cudaGetDeviceProperties(&cprop, ci);
      ostringstream cstr;
      cstr << "\"" << cprop.name << "\""
	   << " mem=" << cprop.totalGlobalMem
	   << " cap=" << cprop.major << "." << cprop.minor
	   << " mpc=" << cprop.multiProcessorCount;
      WriteLog(" #"+ToStr(ci)+" : "+cstr.str());
    }
#endif // CUDA_VERSION
    
    string cwd = Cwd();
    WriteLog("cwd="+(cwd==""?"(null)":cwd));
    WriteLog("cmdline=", cmdline_str);
    WriteLog("binary=", mybinary_str);

    WriteLog("RootDir()=",        RootDir());
    WriteLog("Path()=",        Path());
    WriteLog("UserHomeDir()=", UserHomeDir());
    WriteLog("UserName()=",    UserName());
    WriteLog("SolveArchitecture()=", SolveArchitecture());

    set<string> adb = AllowedDataBases();
    if (adb.size()) {
      string astr;
      for (set<string>::const_iterator i=adb.begin(); i!=adb.end(); i++)
	astr += string(astr!=""?",":"")+"<"+*i+">";
      WriteLog("allowed databases=", astr);
    }

    // displays environment
    for (map<string,string>::const_iterator i=env.begin(); i!=env.end(); i++)
      WriteLog("${"+i->first+"}="+i->second);
  }

///////////////////////////////////////////////////////////////////////////////

void PicSOM::StartTimes() {
  if (DebugTimes())
    tics.Start();
}

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::CreateAnalysis(xmlNodePtr node, const string& aid) {
    // this is the place where slaves are enslaved...
    string msg = "CreateAnalysis() : ";

    if (NodeName(node)!="analysis")
      return ShowErrorS(msg+"not <analysis>");

    string batchline;

    xmlNodePtr c = node->children;
    while (c && c->type!=XML_ELEMENT_NODE)
      c = c->next;

    if (NodeName(c)!="batchline")
      return ShowErrorS(msg+"not <batchline>");
    batchline = NodeChildContent(c);

    c = c->next;
    while (c && c->type!=XML_ELEMENT_NODE)
      c = c->next;

    if (NodeName(c)!="script")
      return ShowErrorS(msg+"not <script>");

    string scr = NodeChildContent(c);
    if (scr=="")
      return ShowErrorS(msg+"empty <script>");

    vector<string> script_v = SplitInSomething("\n", true, scr);
    list<string> script(script_v.begin(), script_v.end());

    c = c->next;

    vector<string> argv;

    for (; c; c = c->next) {
      if (c->type!=XML_ELEMENT_NODE)
	continue;

      if (NodeName(c)!="arg")
	return ShowErrorS(msg+"not <arg>");
    
      string a = NodeChildContent(c);
      if (a=="")
	return ShowErrorS(msg+"empty <arg>");

      argv.push_back(a);
    }

    script_list_e scrargv;
    scrargv.first         = batchline;
    scrargv.second.first  = script;
    scrargv.second.second = argv;

    // static size_t n = 0;

    Analysis *a = new Analysis(this, NULL, NULL, argv);
    a->IdName(aid);
    AppendAnalysis(a);
    /// a is removed and deleted by ThreadExecuteCommand() w/"terminate"

#ifdef ANALYSIS_USE_PTHREADS
    if (IsSlave() && debug_slaves) {
      cout << "Slave starting to execute script <" << scrargv.first << ">" << endl;
      for (auto si=scrargv.second.first.begin(); si!=scrargv.second.first.end();
	   si++)
	cout << "[" << *si << "]" << endl;
      cout << "args:";
      for (auto si=scrargv.second.second.begin(); si!=scrargv.second.second.end();
	   si++)
	cout << " [" << *si << "]";
      cout << endl;
    }

    Analysis::analyse_pthread_data_t *p = a->AnalysePthread(scrargv);
    string id = p ? Analysis::ThreadIdentifier(*p) : "";

    return id!="" ? id : ShowErrorS(msg+"AnalysePthread() failed");
#else
    /// this really doesn't work...
    Analysis::analyse_result res = a->Analyse(scrargv);

    return res.errored() ? "ERROR" : "OK";
#endif // ANALYSIS_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::RootDir(const string& subd, bool fake) const {
    string p = Path();

    if (!norootdir && !fake && Development())
      p = UserHomeDir()+"/picsom";

    if (norootdir)
      p = ((PicSOM*)this)->TempDirPersonal();

    if (subd!="")
      p += "/"+subd;

    return p;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::DataBaseExists(const string& name) {
#ifdef HAVE_MYSQL_H
    if (name.find("mysql://")==0)
      return DataBase::MysqlDataBaseExists(name.substr(8), sqlserver);
#endif // HAVE_MYSQL_H

    string p = ExpandDataBasePath(name);
    // cout << "<" << p << ">" << endl;
    return FileExists(p+"/labels") || FileExists(p+"/settings.xml") ||
      FileExists(p+"/sqlite3.db") ||
      (FileExists(p) && p.substr(p.size()-8)==".sqlite3");  
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::ExpandDataBasePath(const string& nin, const string& q,
				    const string& r, bool user) const {
    // does not _yet_ handle multidirectory paths
    // does not _yet_ handle ~userid

    string d = RootDir("databases", true);

    string n = nin;
    if (n.find("mysql://")==0) {
      // was return ""; until 2013-04-15
      n.erase(0, 8);
    }

    if (n.find("os://")==0) {
      n.erase(0, 5);
      d = ((PicSOM*)this)->TempDirPersonal()+"/databases";
    }

    string ret = ".";
  
    // if (n=="") {} ???

    string p, a;
    if (!SplitParentheses(n, p, a))
      p = n;

    if (p!=".")
      ret = d + "/" + p;

    if ((!DirectoryExists(ret) && !FileExists(ret+".sqlite3")) || user) {
      string lp = DataBase::UserDbPrefix();
      string tmp = RootDir("databases", false)+"/";
      bool local = p.find(lp)==0;
      tmp += p.substr(local ? lp.size() : 0);
      if (DirectoryExists(tmp) || FileExists(tmp+".sqlite3") || user)
	ret = tmp;
    }

    if (!DirectoryExists(ret) && FileExists(ret+".sqlite3")) {
      if (q!="" || r!="")
	return ShowErrorS("ExpandDataBasePath() database.slite3 cannot"
			  " be combined with q or r");

      return ret+".sqlite3";
    }

    if (q!="")
      ret += string("/")+q;

    if (r!="")
      ret += string("/")+r;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  DataBase *PicSOM::FindDataBaseEvenNew(const string& name, bool insrt) {
    MutexLock();

    DataBase *db = name=="." ? NULL : FindDataBase(name, "", false, false);

    if ((!db && insrt) || name==".") {
      bool user = name.substr(0, 1)==DataBase::UserDbPrefix();
      /*
      bool do_settings = true;
      bool do_labels   = do_settings && true;
      bool do_features = do_settings && true;
      bool do_objects  = do_settings && true;
      bool do_classes  = do_settings && true;
      */

      string dbpath;
      if (true  /*do_settings*/)
	dbpath = ExpandDataBasePath(name, "", "", user);

      // if (dbpath!="")
      //   MakeDirectory(dbpath); // now in DataBase::CreateFromScratch()

      db = new DataBase(this, name, dbpath, "", true);

      /*
      db->CreateFromScratch(do_settings, do_labels, do_features,
			    do_objects, do_classes);
      */

      if (name==".") // obs! dunno about this... 
	db->ReadLabels();

      database.Append(db);
    }

    MutexUnlock();

    return db;
  }

  /////////////////////////////////////////////////////////////////////////////

  DataBase *PicSOM::FindDataBase(const string& name, const string& view,
				 bool all_views, bool warn) {
    string msg = "PicSOM::FindDataBase() : ";

    if (name=="") {
      ShowError(msg+"called with empty dbname");
      return NULL;
    }

    // added 2012-03-02: if database=foo(xxx=yyy) or foo@bar(xxx=yyy) 
    // is specified then we should search for foo.
    // we thus assume we would not have two instances of one database.
    string sname = name;
    size_t p = sname.find('(');
    if (p!=string::npos)
      sname.erase(p);

    if (!IsAllowedDataBase(sname))
      return NULL;
  
    const string& sep = DataBase::ViewPartSeparator();

    if (view=="") {
      size_t vps = name.find(sep, 1);
      if (vps!=string::npos)
	return FindDataBase(name.substr(0, vps),
			    name.substr(vps+sep.size()), false);
    }

    string nameview = name;
    if (view!="")
      nameview += sep+view;

    DataBase *basedb = NULL;

    //MutexLock();

    for (int i=0; i<database.Nitems(); i++)
      if (database[i].Name()==sname) {
	basedb = database.Get(i);
	DataBase *v = basedb->FindView(view);
	if (v)
	  return v;
	break;
      }

    if (!basedb) {
      string dbpath = ExpandDataBasePath(name);

      bool do_download = norootdir || name.find("os://")==0;

      if (do_download && !DownloadDataBase(name, dbpath)) {
	ShowError(msg+"download failed");
	return NULL;
      }

      if (!DataBaseExists(name)) {
	MutexUnlock();
	if (warn)
	  ShowError(msg+"database in directory [", dbpath,"] inexistent",
		    " (labels file missing?)");
	return NULL;
      }

      bool no_dir = !DirectoryExists(dbpath);
      basedb = new DataBase(this, name, dbpath, "", false, no_dir);
      basedb->InterpretDefaults(); // added 2010-03-01

      if (all_views)
	basedb->FindAllViews();

      database.Append(basedb);
    }

    //MutexUnlock();

    return basedb->FindView(view);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::DeleteDataBase(DataBase *db) {
    string msg = "PicSOM::DeleteDataBase(" + db->Name() + ") : ";

    WriteLog("Explicitly deleting database "+ db->Name() + " from memory");

    db->JoinAsyncAnalysesDeprecated();

    if (!database.Remove(db))
      return ShowError(msg+"Remove() failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::DownloadFile(const string& path) const {
    const string msg = "PicSOM::DownloadFile("+path+") : ";

    if (MasterAddress()=="")
      return ShowError(msg+"cannot download because master_address not set");

    string rpath = path, r = RootDir();
    if (rpath.find(r)!=0)
      return ShowError(msg+"path <"+path+"> does not start with <"+r+">");

    rpath.erase(0, r.size());
    string url = MasterAddress()+rpath, ctype;
    list<pair<string,string> > hdrs;
    hdrs.push_back(make_pair("X-Auth-Token", MasterAuthToken()));
    string tmp = Connection::DownloadFile((PicSOM*)this, url, hdrs, ctype);
    
    if (tmp=="")
      return ShowError(msg+"downloading <"+url+"> failed");

    string dir = path;
    size_t p = dir.rfind('/');
    if (p!=string::npos)
      dir.erase(p);

    int mode = 02775;
    if (!MkDirHier(dir, mode))
      return ShowError(msg+"creating directory <"+dir+"> failed");

    if (!Rename(tmp, path))
      return ShowError(msg+"moving <"+tmp+"> to <"+path+"> failed");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::DownloadDataBase(const string& name, const string& dir) {
    const string msg = "PicSOM::DownloadDataBase("+name+","+dir+") : ";

    if (false && !IsSlave())
      return ShowError(msg+"cannot download when not a slave");

    string url;
    list<pair<string,string> > hdrs;

    if (name.find("os://")==0) {
      string token = Connection::GetOpenStackAuthToken(this);

      if (token=="")
	return ShowError(msg+"getting OpenStack token for \""+name+"\" failed");

      url = Connection::GetOpenStackDownloadUrl(this);
      if (url=="")
	return ShowError(msg+"getting OpenStack dl url failed");

      url += "/"+name.substr(5)+"/sqlite3.db";

      hdrs.push_back(make_pair("X-Auth-Token", token));

    } else {
      if (master_address=="")
	return ShowError(msg+"cannot download because master_address not set");

      url = master_address+"/database/"+name+"/sqlite3.db";
      hdrs.push_back(make_pair("X-Auth-Token", MasterAuthToken()));
    }

    int timeout = 50;
    string ctype, dbdata = Connection::DownloadFile(this, url, hdrs, ctype,
						    timeout);
    
    if (dbdata=="")
      return ShowError(msg+"downloading <"+url+"> failed");

    int mode = 02775;
    if (!MkDirHier(dir, mode))
      return ShowError(msg+"creating directory <"+dir+"> failed");

    // this should not be here, but let it be
    if (!MkDirHier(dir+"/features", mode))
      return ShowError(msg+"creating directory <"+dir+"/features> failed");

    if (!Rename(dbdata, dir+"/sqlite3.db"))
      return ShowError(msg+"moving <"+dbdata+"> to <"+dir+
		       "/sqlite3.db> failed");

    // cout << "ctype=<" << ctype << ">" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetAllowedDataBases(const string& nstr) {
    allowed_databases.clear();
    vector<string> dbs = SplitInCommas(nstr);
    for (size_t i=0; i<dbs.size(); i++) {
      string dbn = dbs[i];
      if (dbn.find("mysql://")==0) {
	FindDataBase(dbn, "", false, true);
	dbn.erase(0, 8);
      }
      AddAllowedDataBase(dbn, true);
      size_t a = dbn.find(DataBase::UserDbPrefix());
      if (a!=string::npos)
	AddAllowedDataBase(dbn.substr(0, a), true);
    }
    return true;
  }

///////////////////////////////////////////////////////////////////////////////

void PicSOM::FindAllDataBases(bool all_views) {
  //
  // There should be a modification time check here
  //

  for (int i=0; i<2; i++) {
    if (i && (!Development() || !DefaultDirPath()))
      break;

    string dbpath = ExpandDataBasePath("", "", "", i);

#ifdef HAVE_DIRENT_H
    DIR *d = opendir(dbpath.c_str());

    for (dirent *dp; d && (dp = readdir(d)); ) {
      string name = dp->d_name;
      if (name[0]=='.')
	continue;

      if (i)
	name = DataBase::UserDbPrefix()+name;

      if (!IsAllowedDataBase(name))
	continue;

      string d1 = ExpandDataBasePath(name);
      string d2 = ExpandDataBasePath(name, "features");
      string f1 = ExpandDataBasePath(name, "labels");

      if (DirectoryExists(d1) && DirectoryExists(d2) && FileExists(f1))
	FindDataBase(name, "", all_views);
    }
  
    if (d)
      closedir(d);
#endif // HAVE_DIRENT_H
  }
}

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::DoInterpret(const string& key, const string& val,
			 PicSOM *p, Query *q,
			 const Connection *cc, Connection *mc,
			 Analysis *a, TSSOM *t, bool other, bool warn) {
  bool ready = false;
  int result = 0;

  //if (q) cout << "1 >>>> " << TargetTypeString(q->Target()) << endl;

  if (a)
    a->RecordKeyEqualValue(key, val);

  if (!ready && p) {
    ready = p->Interpret(key, val, result);
    TraceInterpret(ready, "PicSOM", key, val, result);
  }

  if (!ready && a) {
    ready = a->Interpret(key, val, result);
    TraceInterpret(ready, "  Analysis", key, val, result);
  }

  if (!ready && mc) {
    ready = mc->Interpret(key, val, result);
    TraceInterpret(ready, "  Connection", key, val, result);
  }
  
  if (!ready && q) {
    ready = q->Interpret(key, val, result, cc);
    TraceInterpret(ready, "  Query", key, val, result);
  }

  // earlier it was needed that q->QueryMode()==operation_create|insert
  if (!ready && q && q->GetDataBase()) {
    ready = q->GetDataBase()->Interpret(key, val, result);
    TraceInterpret(ready, "  DataBase", key, val, result);
  }
 
  if (!ready && t) {
    ready = t->Interpret(key, val, result);
    TraceInterpret(ready, "  TSSOM", key, val, result);
  }

  if (!ready && a) {
    object_type ot = ObjectType(key);
    if (int(ot)>=0) {
      string oname = val, ospec;
      size_t p = oname.find(' ');
      if (p!=string::npos) {
        ospec = oname.substr(p+1);
        oname.erase(p);
      }
      ready = a->ProcessObjectRequest(ot, oname, ospec, result);
    }
    TraceInterpret(ready, "  object", key, val, result);
  }

  if (!ready && q && other)
    ready = q->AddOtherKeyValue(key, val);
  
  //if (q) cout << "2 >>>> " << TargetTypeString(q->Target()) << endl;

  if (warn && (!ready || result!=1))
    ShowError("PicSOM::DoInterpret() <", key, ">=<", val, "> ",
	      !ready?"not recognized":result==-1?"resulted in -1":"failed");

  return result==1;
}

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::Interpret(const string& keystr, const string& valstr, int& res) {
  res = 1;

  int  intval  = atoi(valstr.c_str());
  bool boolval = IsAffirmative(valstr);

  /*>> interpret
      path
      *DOCUMENTATION MISSING* */
    
  if (keystr=="path") {
    Path(valstr);
    return true;
  }

  if (keystr=="tempdir") {
    if (!IsSlave() && !SetTempDirRoot(valstr))
      res = ShowError("failed to set temp dir <"+valstr+">");
    return true;
  }

  if (keystr=="slavetempdir") {
    if (IsSlave() && !SetTempDirRoot(valstr))
      res = ShowError("failed to set temp dir <"+valstr+">");
    return true;
  }

  /*>
      log
      *DOCUMENTATION MISSING* */
    
  if (keystr=="log") {
    OpenLog(valstr.c_str());
    WriteLogVersion(); 
    WriteLogProcessInfo();
    return true;
  }

  // ...
    
  if (keystr=="logmode") {
    LoggingMode(valstr);
    return true;
  }

  if (keystr=="sqlserver") {
    sqlserver = valstr;
    if (DataBase::DebugSql())
      cout << "sqlserver = " << sqlserver << endl;
    return true;
  }

  if (keystr=="tempsqlitedb") {
    if (!IsSlave())
      tempsqlitedb = boolval;
    return true;
  }

  if (keystr=="slavetempsqlitedb") {
    if (IsSlave())
      tempsqlitedb = boolval;
    return true;
  }

  if (keystr=="unlimited") {
    SetUnLimited();
    return true;
  }

  if (keystr=="childlimits") {
    childlimits = boolval; // this comes too late. see -guard=nolimits
    return true;
  }

  if (keystr=="uselocalhost") {
    uselocalhost = boolval;
    return true;
  }

  if (keystr=="quiet") {
    quiet = boolval;
    return true;
  }

  if (keystr=="debugtimes") {
    DebugTimes(intval);
    StartTimes();
    return true;
  }

  if (keystr=="debugmem") {
    DebugMemoryUsage(intval);
    PossiblyShowDebugInformation("After processing debugmem=x setting");
    return true;
  }

  if (keystr=="debuglocks") {
    Connection::DebugLocks(true);
    DataBase::DebugLocks(true);
    return true;
  }

  if (keystr=="debugkeys") {
    Query::DebugAllKeys(boolval);
    return true;
  }

  if (keystr=="debugtrap") {
    Simple::TrapAfterError(boolval);
    picsom::TrapAfterError(boolval);
    return true;
  }

  if (keystr=="debugbacktrace") {
    Simple::BackTraceBeforeTrap(boolval);
    BackTraceBeforeTrap(boolval);
    return true;
  }

  if (keystr=="debugoctave") {
    DebugOctave(intval);
    return true;
  }

  if (keystr=="debugorigins") {
    DataBase::DebugOrigins(boolval);
    return true;
  }

  if (keystr=="debugimg") {
    imagefile::debug_impl(intval);
    return true;
  }

  if (keystr=="debugvideo") {
    videofile::debug(intval);
    return true;
  }

  if (keystr=="debugimages") {
    DataBase::DebugImages(intval);
    return true;
  }

  if (keystr=="debugreads") {
    Connection::DebugReads(boolval);
    return true;
  }

  if (keystr=="debugwrites") {
    Connection::DebugWrites(intval);
    return true;
  }

  if (keystr=="debughttp") {
    Connection::DebugHTTP(intval);
    return true;
  }

  if (keystr=="debugajax") {
    Query::DebugAjax(intval);
    return true;
  }

  if (keystr=="debugaspects") {
    Query::DebugAspects(boolval);
    return true;
  }

  if (keystr=="debugplaceseen") {
    Query::DebugPlaceSeen(boolval);
    return true;
  }

  if (keystr=="debugstages") {
    Query::DebugStages(boolval);
    CbirAlgorithm::DebugStages(boolval);
    return true;
  }

  if (keystr=="debugclassify") {
    Query::DebugClassify(true);
    CbirAlgorithm::DebugClassify(true);
    return true;
  }

  if (keystr=="debugweights") {
    Query::DebugWeights(true);
    CbirAlgorithm::DebugWeights(true);
    return true;
  }

  if (keystr=="debuglists") {
    Query::DebugLists(intval);
    CbirAlgorithm::DebugLists(intval);
    return true;
  }

  if (keystr=="debugchecklists") {
    Query::DebugCheckLists(true);
    CbirStageBased::DebugCheckLists(true);
    return true;
  }

  if (keystr=="debuggt") {
    DataBase::DebugGroundTruth(true);
    return true;
  }

  if (keystr=="debuggtexp") {
    DataBase::DebugGroundTruthExpand(true);
    return true;
  }

  if (keystr=="debugsvm") {
    SVM::DebugSVM(intval);
    return true;
  }

  if (keystr=="debugdetections") {
    DataBase::DebugDetections(intval);
    return true;
  }

  if (keystr=="debugcaptionings") {
    DataBase::DebugCaptionings(intval);
    return true;
  }

  if (keystr=="debugfeatures") {
    DataBase::DebugFeatures(intval);
    return true;
  }

  if (keystr=="debugsegmentation") {
    DataBase::DebugSegmentation(intval);
    Segmentation::Verbose(intval);
    return true;
  }

  if (keystr=="debugsubobjects") {
    Query::DebugSubobjects(boolval);
    return true;
  }

  if (keystr=="debugslaves") {
    debug_slaves = boolval;
    return true;
  }

  if (keystr=="debugthreads") {
#ifdef PICSOM_USE_PTHREADS
    debug_threads = boolval;
#endif // PICSOM_USE_PTHREADS
    return true;
  }

  if (keystr=="debugscript") {
    Analysis::DebugScript(boolval);
    return true;
  }

  if (keystr=="debuginterpret") {
    debug_interpret = boolval;
    return true;
  }

  if (keystr=="debugsql") {
    DataBase::DebugSql(boolval);
    return true;
  }

  if (keystr=="debugtext") {
    DataBase::DebugText(boolval);
    return true;
  }

  if (keystr=="caffe_expand_task") {
#if defined(HAVE_CAFFE_CAFFE_HPP) && defined(PICSOM_USE_CAFFE)
    extern void caffe_expand_task(size_t);
    caffe_expand_task(intval);
#endif // HAVE_CAFFE_CAFFE_HPP && PICSOM_USE_CAFFE
    return true;
  }

  if (keystr=="bin_data_full_test") {
    VectorIndex::BinDataFullTest(boolval);
    return true;
  }

  if (keystr=="fast_bin_check") {
    VectorIndex::FastBinCheck(boolval);
    return true;
  }

  if (keystr=="nan_inf_check") {
    VectorIndex::NanInfCheck(boolval);
    return true;
  }

  if (keystr=="force_fast_slave") {
    ForceFastSlave(boolval);
    return true;
  }

  if (keystr.find("limit_")==0)
    return SetLimit(keystr.substr(6), valstr);

  if (keystr=="pre707bug") {
    Valued::Pre707Bug(boolval);
    return true;
  }

  if (keystr=="slaves") {
    SetSlaveInfo(valstr);
    return true;
  }

  if (keystr=="slavepipe") {
    slavepipe = valstr;
    return true;
  }

  if (keystr=="use_slaves") {
    use_slaves = boolval;
    return true;
  }

  if (keystr=="use_mpi_slaves") {
    use_mpi_slaves = boolval;
    return true;
  }

  if (keystr=="gpupolicy") {
    gpupolicy = intval;
    if (gpupolicy==2 && gpudevice<0)
      res = ShowError("gpupolicy=2 failed due to missing device");

    Feature::GpuDeviceId(GpuDeviceId());
    return true;
  }

  if (keystr=="gpudevice") {
    gpudevice = intval;
    Feature::GpuDeviceId(GpuDeviceId());
    return true;
  }

  if (keystr=="parse_external_metadata") {
    DataBase::ParseExternalMetadata(boolval);
    return true;
  }

  if (keystr=="alloweddbs") {
    SetAllowedDataBases(valstr);
    return true;
  }

  if (keystr=="dbopenmode") {
    if (!IsSlave())
      DataBase::OpenReadWrite(valstr);
    return true;
  }

  if (keystr=="slavedbopenmode") {
    if (IsSlave())
      DataBase::OpenReadWrite(valstr);
    return true;
  }

  if (keystr=="force_lucene_unlock") {
    // DataBase::ForceLuceneUnlock(boolval);
    DataBase::ForceLuceneUnlock(false); // this shouldn't be true...
    return true;
  }

  if (keystr=="listenport") {
    if (valstr=="my") {
      if (!ListenPort(UserName()))
	ShowError("listenport=my failed for user ", UserName());

    } else if (valstr=="yes") {
      ListenPort();

    } else {
      int p, q;
      int n = sscanf(valstr.c_str()+6, "%d..%d", &p, &q);
      ListenPort(p, n==2?q:p);
    }

    SetUpListenConnection(true, true, "/");

    return true;
  }

#ifdef PICSOM_USE_PTHREADS
  if (keystr=="threads") {
    threads = intval;
    return true;
  }

  if (keystr.find("pthreads_conn")==0) {
    pthreads_connection = boolval;
    return true;
  }

  if (keystr=="pthreads_tssom") {
    pthreads_tssom = boolval;
    return true;
  }

  if (keystr=="pthreads_analysis") {
    Analysis::UsePthreads(intval);
    return true;
  }

  if (keystr=="usethreads") {
    vector<string> v = SplitInSomething(",", false, valstr);
    if (v.size()==0)
      v.push_back("");
    for (size_t i=0; i<v.size(); i++)
      if (v[i]=="analysis")
	Analysis::UsePthreads(1000); // obs!
      else if (v[i]=="insert")
	Analysis::UsePthreadsInsert(true);
      else if (v[i]=="detection")
	DataBase::UsePthreadsDetection(true);
      else if (v[i]=="features")
	DataBase::UsePthreadsFeatures(true);
      else if (v[i]=="connection")
	pthreads_connection = true;
      else if (v[i]=="tssom")
	pthreads_tssom = true;
      else {
	res = 0;
	ShowError("PicSOM::Interpret() : failed to interpret usethreads="+
		  v[i]);
      }
    return true;
  }
#endif // PICSOM_USE_PTHREADS

  return false;
}

  /////////////////////////////////////////////////////////////////////////////

  size_t PicSOM::Connections(bool only_non_closed,
                             bool only_selectable) const {
    if (!only_non_closed && !only_selectable)
      return connection.size();

    size_t ret = 0;

    for (size_t i=0; i<connection.size(); i++) {
      if (Connection::DebugSelects() && only_selectable) {
	cout << " connection #" << i << " ";
	GetConnection(i)->Dump();
	cout << " closed=" << GetConnection(i)->IsClosed()
	     << " can_be_selected=" << GetConnection(i)->CanBeSelected()
	     << endl;
      }
      if ((!only_non_closed || !GetConnection(i)->IsClosed()) &&
	  (!only_selectable ||  GetConnection(i)->CanBeSelected()))
	ret++;
    }

    if (only_selectable && soap_server) {
      if (Connection::DebugSelects() && only_selectable)
	cout << " connection soap_server" << endl;
      ret++;
    }

    if (Connection::DebugSelects() && only_selectable)
      cout << "***** SelectableConnections() returns " << ret << endl;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t PicSOM::MPIconnections() const {
    size_t ret = 0;

    for (size_t i=0; i<connection.size(); i++)
      if (GetConnection(i)->Type()==Connection::conn_mpi_down)
	ret++;

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  Connection *PicSOM::SelectInput() {
#ifdef PICSOM_USE_PTHREADS
    if (!listen_thread_set) {
      listen_thread = main_thread;
      listen_thread_set = true;
    }

    if (!pthread_equal(listen_thread, pthread_self())) {
      ShowError("PicSOM::SelectInput() : called from another pthread");
      return NULL;
    }
#endif // PICSOM_USE_PTHREADS

    fd_set rset, eset; 
    int nb = -1;
    for (;;) {
#ifdef PICSOM_USE_MPI    
      for (size_t i=0; i<connection.size(); i++) {
	Connection *c = GetConnection(i);
	Connection::connection_type ct = c->Type();
	if ((ct==Connection::conn_mpi_down || ct==Connection::conn_mpi_up) &&
	    c->CanBeSelected() && !c->IsFailed() && c->MPIavailable())
	  return c;
      }
#endif // PICSOM_USE_MPI    

      if (Connection::DebugSelects()) {
	cout << "---- SelectInput() pthread=" << ThreadIdentifierUtil() << endl;
	ShowConnections();
	cout << "----" << endl;
      }

      FD_ZERO(&rset);
      FD_ZERO(&eset);

      bool set = false;
    
      size_t nconn = Connections(false, false);
      for (size_t i=0; i<nconn; i++) {
	GetConnection(i)->Active(false);

	if (GetConnection(i)->Rfd()==STDIN_FILENO && IsSlave() &&
	    !GetConnection(i)->IsClosed() && feof(stdin))
	  GetConnection(i)->Close();

	bool selectable = false;
	bool index_ok   = GetConnection(i)->Rfd()<FD_SETSIZE;
	if (index_ok && GetConnection(i)->CanBeSelected()) {
	  FD_SET(GetConnection(i)->Rfd(), &rset);
	  FD_SET(GetConnection(i)->Rfd(), &eset);
	  set = selectable = true;
	}
	if (Connection::DebugSelects())
	  cout << "  #" << i << " rfd=" << GetConnection(i)->Rfd()
	       << " is " << (selectable?"SELECTABLE":"NOT selectable")
	       << " index_ok=" << (index_ok?"yes":"no") << endl;

	if (GetConnection(i)->IsClosed() && GetConnection(i)->HasOwnThread())
	  GetConnection(i)->CancelThread();
      }
    
      if (Connection::DebugSelects())
	cout << "----" << endl;

      if (!set)
	return NULL;

      const string& timeout = Connection::TimeOut();
      bool use_timeout = timeout.size() || Connection::DebugSelects()>1;

      timeval wait, *timeout_ptr = use_timeout ? &wait : NULL;
      wait.tv_sec = 5; wait.tv_usec = 0;

      if (timeout.size()) {
	int sec = 0, usec = 0;
	sscanf(timeout.c_str(), "%d.%d", &sec, &usec);
	wait.tv_sec = sec;
	wait.tv_usec = usec;
      }

      nb = select(FD_SETSIZE, &rset, NULL, &eset, timeout_ptr);

      if (Connection::DebugSelects())
	cout << "***** select() returned nb=" << nb << endl;

      if (nb<=0) {
	if (timeout.size()) {
	  WriteLog("Terminating due to timeout in SelectInput().");
	  return NULL;
	} else
	  continue;
      }

      bool new_created = false;
      nconn = Connections(false, false);
      for (size_t j=0; j<nconn; j++) 
	if (GetConnection(j)->Type()==Connection::conn_listen &&
	    GetConnection(j)->Rfd()<FD_SETSIZE &&
	    FD_ISSET(GetConnection(j)->Rfd(), &rset)) {
	  RemoteHost host(NULL);
	  int ns = host.Accept(GetConnection(j)->Rfd());
	  if (ns==-1) {
	    perror("accept");
	    exit(1);
	  }
	
	  if (Connection::DebugSelects())
	    cout << "***** j=" << j << " ns=" << ns << endl;

	  GetConnection(j)->SetAccessTime();
	  GetConnection(j)->IncrementQueries();

	  new_created = CreateSocketConnection(ns, host);
	  nconn = Connections(false, false);

	  if (new_created && Connection::DebugSelects())
	    cout << "***** new SOCKET was created, StateString="
		 << GetConnection(j)->StateString() << " ******" << endl;

	  break;
	}

      if (!new_created)
	break;
    }

    if (Connection::DebugSelects())
      cout << "***** now continuing in SelectInput ****" << endl;

    size_t nconn = Connections(false, false);
    for (size_t j=0; j<nconn; j++)
      if (GetConnection(j)->Rfd()>=0 &&
	  GetConnection(j)->Rfd()<FD_SETSIZE &&
	  FD_ISSET(GetConnection(j)->Rfd(), &eset)
	  && GetConnection(j)->Identity()!="/dev/null") {
#ifndef __alpha
	cout << TimeStamp() << ThreadIdentifierUtil() << " EXCEPTION HERE: ";
	GetConnection(j)->Dump();
#endif // __alpha
      }
  
    for (size_t j=0; j<nconn; j++) {
      Connection *c = GetConnection(j);
      // c->ReadableBytes() added 2015-05-21 for allowing keep-alive
      if (c->CanBeSelected() && c->Rfd()<FD_SETSIZE &&
	  FD_ISSET(c->Rfd(), &rset) && c->Type()!=Connection::conn_listen) {
	if (c->ReadableBytes()) {
	  c->IncrementQueries();

	  if (Connection::DebugSelects()) {
	    cout << endl << "  #" << j << " ";
	    c->Dump();
	    cout << " bytes=" << c->ReadableBytes() << endl;
	  }

	  c->Active(true);
	  
	  return c;

	} else {
	  if (Connection::DebugSelects()) {
	    cout << "  about to read EOF from ";
	    c->Dump();
	    cout << endl;
	  }
	  if (!c->ReadEofAndClose())
	    ShowError("PicSOM::SelectInput() : ReadEofAndClose() failed");

	  return c;
	}
      }

      if (c->IsClosed()) { // added 20191119 because all were Keep-Open
	if (CloseConnection(c, true, false, true))
	  j--;
	nconn = Connections(false, false);
      }
    }
  
    return NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::ShowConnections(Connection *conn, ostream *os) const {
    // no, this is not obsoleted as long as needed by SelectInput()...
    // Obsoleted("PicSOM::ShowConnections()");

    for (size_t i=0; i<Connections(false, false); i++) {
      char tmp[1000];
      const char *ptr = GetConnection(i)->DisplayStringM();
      sprintf(tmp, "%c #%2d: %s", GetConnection(i)==conn?'*':' ', (int)i, ptr);
      // if (conn)
      //   conn->WriteLine(tmp);
      if (os)
	*os << tmp << endl;
      delete [] ptr;
    }
  }

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ShowQueries(Connection& /*conn*/) const {
  Obsoleted("PicSOM::ShowQueries()");
  for (size_t i=0; i<Nqueries(); i++) {
    const char *ptr = GetQuery(i)->DisplayStringM();
    // conn.WriteLine(ptr);
    delete [] ptr;
  }
}

  /////////////////////////////////////////////////////////////////////////////

  Query *PicSOM::FindQueryInMemory(const string& id) const {
    string msg = "FindQueryInMemory("+id+") : ";

    bool debug = false;

    if (debug)
      cout << msg << "starting" << endl;
    
    string ident = Query::UnEscapeIdentity(id);

    Query *q = NULL;

    RwLockReadQueryList();
    for (size_t i=0; !q && i<Nqueries(); i++)
      q = GetQuery(i)->Find(ident, debug);
    RwUnlockReadQueryList();

    if (q) {
      q->ConditionallyRecalculate();
      q->SetLastAccessTimeNow();
    }

    if (debug)
      cout << msg << "returning " << (q?"non-":"") << "NULL" << endl;
    
    return q;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query *PicSOM::FindNthQueryInMemory(const string& userpwd, size_t n) const {
    // the number of queries found with username userpwd
    size_t counter = 0;

    // which one of those queries was asked 
    int wanted = n;

    if (n==0) {
      ShowError("*PicSOM::FindNthQueryInMemory() search for zero'th query");
      return NULL;
    }

    Query *q = NULL;

    RwLockReadQueryList();
    for (size_t i=0; i<Nqueries(); i++) {
      const string& userpassword = GetQuery(i)->FirstClient().
	UserPassword().c_str();

      if (userpassword==userpwd) {
	counter++;
	q = GetQuery(i)->Root();
	wanted--;
      }
      if (wanted==0) break;
    }
    RwUnlockReadQueryList();

    // if we did not find so many queries as asked return null
    if (counter != n)
      return NULL;

    if (q) {
      q->ConditionallyRecalculate();
      q->SetLastAccessTimeNow();
    }

    return q;
  }

  /////////////////////////////////////////////////////////////////////////////

  Query *PicSOM::FindQuery(const string& idin, const Connection *conn,
                           Analysis *a) {
    string ident = Query::UnEscapeIdentity(idin), memident, p;

    if (ident=="Q:previous" && Nqueries())
      ident = GetQuery(Nqueries()-1)->Identity();

    Query *q = FindQueryInMemory(ident);
    if (q) {
      if (a)
        q->SetAnalysis(a);
      return q;
    }

    if (ident[0]=='/' || ident.substr(0, 2)=="./" ||
        ident.substr(0, 3)=="../")
      p = ident;

    else {
      string r = Query::RootIdentity(ident);
      if (r=="") {
        ShowError("Query::RootIdentity(", ident, ") failed");
        return NULL;
      }
      p = SavePath(r, true);
    }

    if (p=="")
      return NULL;
      
    q = new Query(this, p, conn, a);

    if (q->SaneIdentity()) {
      AppendQuery(q, true);
      memident = q->Identity();

    } else {
      delete q;
      return NULL;
    }

    return FindQueryInMemory(memident);
  }

///////////////////////////////////////////////////////////////////////////////

string PicSOM::SavePath(const string& ident, bool reuse) const {
  if (!Query::SaneIdentity(ident)) {
    ShowError("PicSOM::SavePath() : insane identity <", ident, ">");
    return "";
  }

  char subdir[1000];
  sprintf(subdir, "%c%c/%c%c/%c%c/%c%c/%c%c/%c%c",
	  ident[2], ident[3], ident[4], ident[5], ident[6], ident[7], 
	  ident[9], ident[10], ident[11], ident[12], ident[13], ident[14]);

  string rdt = RootDir("queries", true);
  char prim[1024];
  sprintf(prim, "%s/%s/%s", rdt.c_str(), subdir, ident.c_str());

  if (!Development() || (reuse&&FileExists(prim)))
    return prim;

  string rdf = RootDir("queries");
  char secn[1024];
  sprintf(secn, "%s/%s/%s", rdf.c_str(), subdir, ident.c_str());

  return secn;
}

///////////////////////////////////////////////////////////////////////////////

enum_info enum_object_type_info_xx[] = {
#define make_object_entry(e) { #e, ot_ ## e }
  make_object_entry(no_object),                // 1st picsom-type
  make_object_entry(cookie),
  make_object_entry(databaselist),
  make_object_entry(connectionlist),
  make_object_entry(connection),
  make_object_entry(querylist),
  make_object_entry(versions),
  make_object_entry(command),
  make_object_entry(threadlist),
  make_object_entry(thread),
  make_object_entry(analysislist),
  make_object_entry(analysis),
  make_object_entry(slavelist),
  make_object_entry(slave),
  make_object_entry(status),
  make_object_entry(databaseinfo),             // 1st db-type
  make_object_entry(featurelist),
  make_object_entry(object),
  make_object_entry(thumbnail),
  make_object_entry(message),
  make_object_entry(plaintext),
  make_object_entry(objectinfo),
  make_object_entry(objectfileinfo),
  make_object_entry(frameinfo),
  make_object_entry(segmentinfo),
  make_object_entry(objecttext),
  make_object_entry(featurevector),
  make_object_entry(contentitems),
  make_object_entry(segmentationinfo),
  make_object_entry(segmentationinfolist),
  make_object_entry(segmentimg),
  make_object_entry(contextstate),
  make_object_entry(timeline),
  make_object_entry(featureinfo),              // 1st tssom-type
  make_object_entry(mapunitinfo),
  make_object_entry(classplot),
  make_object_entry(mapcollage),
  make_object_entry(umatimage),
  make_object_entry(queryinfo),                // 1st query-type
  make_object_entry(newidentity),
  make_object_entry(variables),
  make_object_entry(objectlist),
  make_object_entry(ajaxresponse),
  make_object_entry(mapimage),
  make_object_entry(queryimage),
  { NULL, -1 }
#undef make_object_entry
};

enum_info *enum_object_type_info = enum_object_type_info_xx;

enum_info enum_target_type_info_xx[] = {
#define make_target_entry(e) { #e, target_ ## e }
  make_target_entry(no_target),
  make_target_entry(file),
  make_target_entry(segment),
  make_target_entry(full),
  make_target_entry(virtual),
  make_target_entry(image),
  make_target_entry(imageset),
  make_target_entry(collage),
  make_target_entry(gazetrack),
  make_target_entry(text),
  make_target_entry(html),
  make_target_entry(data),
  make_target_entry(message),
  make_target_entry(sound),
  make_target_entry(video),
  make_target_entry(link),
  make_target_entry(any_target),
  { NULL, -1 }
#undef make_target_entry
};

enum_info *enum_target_type_info = enum_target_type_info_xx;

enum_info enum_msg_type_info_xx[] = {
#define make_msg_type_entry(e) { #e, msg_ ## e }
  make_msg_type_entry(undef),
  make_msg_type_entry(acknowledgment),
  make_msg_type_entry(response),
  make_msg_type_entry(objectrequest),
  make_msg_type_entry(upload),
  make_msg_type_entry(slaveoffer),
  make_msg_type_entry(command),
  { NULL, -1 }
#undef make_query_type_entry
};

  enum_info *enum_msg_type_info = enum_msg_type_info_xx;

///////////////////////////////////////////////////////////////////////////////

int EnumInfo(enum_info *info, const char *name) {
  for (; info && info->str; info++)
    if (!strcmp(info->str, name))
      return info->type;

  return -1;
}

///////////////////////////////////////////////////////////////////////////////

const char *EnumInfoP(enum_info *info, int ot, bool wn) {
  static char ret[100];

  for (; info && info->str; info++)
    if (ot==info->type) {
      if (!wn)
	return info->str;

      sprintf(ret, "%s (%d)", info->str, ot);
      return ret;
    }

  if (wn) {
    sprintf(ret, "*unknown* (%d)", ot);
    return ret;
  }

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::SomeDataBaseBounces() const {
  for (int i=0; i<database.Nitems(); i++)
    if (database[i].RwLockBounce())
      return true;

  return false;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddObjectToXML(XmlDom& xml, Query *q, Connection *c,
			      object_type ot, const string& on,
			      const string& os) {
    string msg = "PicSOM::ObjectToXML() : ";

    if (IsPicSOM(ot))
      return AddToXML(xml, ot, on, os, c);

    if (!q && (IsQuery(ot)||IsDataBase(ot)||IsTSSOM(ot)))
      return ShowError(msg+"no query");

    if (IsQuery(ot))
      return q->AddToXML(xml, ot, on, os, c);

    if (IsDataBase(ot)) {
      DataBase *db = q->GetDataBase();
      if (!db)
	return ShowError(msg+"no database");

      return db->AddToXML(xml, ot, on, os, q, c);
    }

    if (IsTSSOM(ot)) {
      int m = q->FindTsSom(on);
      if (m<0)
	return ShowError(msg+"FindTsSom(", on, ") failed");

      return q->TsSom(m).AddToXML(xml, ot, on, os, q);
    }

    return ShowError(msg+"unknown objecttype: ", ObjectTypeP(ot, true));
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::AddToXML(XmlDom& xml, object_type ot, const string& namestr,
		      const string& specstr, Connection *conn) {
  xmlNodePtr parent = xml.node;
  xmlNsPtr   ns     = xml.ns;

  string msg = "AddToXML() : ";

  const char *name = namestr.c_str();
  const char *spec = specstr.c_str();

  bool ok = true;

  switch (ot) {
  case ot_cookie: {
    if (!conn)
      return ShowError(msg+"no connection");

    const char *cookie = BakeCookieP(name);

    char addr[1000], oldcookie[100] = "";
    strcpy(addr, conn->Addresses(true).c_str());
    char *ptr = strchr(addr, ':');
    if (ptr) {
      *ptr = 0;
      strcpy(oldcookie, ptr+1);
    }
    sprintf(addr+strlen(addr), ":%s", cookie);
    ostringstream cstr;
    cstr << "Sent a cookie [" << addr << "]";
    if (name)
      cstr << " for <" << name << ">";
    if (strlen(oldcookie))
      cstr << " old cookie was <" << oldcookie << ">";
    WriteLog(cstr);
    
    AddTag(parent, ns, "cookie", cookie);

    break;
  }

  case ot_databaselist: {
    if (conn && conn->BounceOK() && SomeDataBaseBounces()) {
      AddToXMLbounce(parent, ns);
      break;
    }

    FindAllDataBases(true);
    AddToXMLdatabaselist(xml, namestr, specstr, conn);
    break;
  }

  case ot_connectionlist: {
    ok = AddToXMLconnectionlist(xml);
    break;
  }

  case ot_querylist: {
    XmlDom ql = xml.Element("querylist");

    RwLockReadQueryList();
    for (size_t i=0; i<Nqueries(); i++)
      //if (GetQuery(i).CheckPermissions(*q))
      GetQuery(i)->AddToXMLqueryinfo(ql, false, false, false, false, conn);

    if (spec && !strcmp(spec, "full"))
      AddToXMLquerylistfull(ql.node, ql.ns);
    RwUnlockReadQueryList();

    break;
  }

  case ot_versions: {
    AddToXMLversions(xml);
    break;
  }

  case ot_command: {
    ok = ExecuteCommand(namestr, specstr, xml, conn);
    break;
  }

  case ot_slavelist: {
    ok = AddToXMLslavelist(xml);
    break;
  }

  case ot_threadlist: {
    ok = AddToXMLthreadlist(xml);
    break;
  }

  case ot_thread: {
#ifdef PICSOM_USE_PTHREADS
    thread_info_t *t = FindThread(namestr);
    if (!t)
      return ShowError(msg+"thread <"+namestr+"> not found");

    bool intact = true;
    if (!ThreadExecuteCommand(t, specstr, intact))
      return ShowError(msg+"ThreadExecuteCommand(..., "+specstr+") failed");

    ok = intact ? AddToXMLthread(xml, t) : AddToXMLthread_rip(xml, name);
#endif // PICSOM_USE_PTHREADS
    break;
  }

  case ot_analysislist: {
    ok = AddToXMLanalysislist(xml, specstr);
    break;
  }

  case ot_analysis: {
    ok = AddToXMLanalysis(xml, namestr, "fullxmlresult"/*specstr*/);
    break;
  }

  case ot_status: {
#ifdef PICSOM_USE_PTHREADS
    JoinThreads(false);
#endif // PICSOM_USE_PTHREADS
    AddToXMLstatus(xml);
    break;
  }

  default:
    ok = ShowError(msg+"unknown objecttype: "+ObjectTypeP(ot, true));
  }

  return ok;
}  

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::AddToXMLdatabaselist(XmlDom& xml, const string& /*name*/,
                                    const string& spec, Connection *conn) {
    XmlDom dbl = xml.Element("databaselist");
    // dbl.AddSOAP_ENC_arrayType(true);
    for (int i=0; i<database.Nitems(); i++)
      database[i].AddToXMLdatabaseinfo(dbl, spec, conn, true, false);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

bool PicSOM::AddToXMLbounce(xmlNodePtr parent, xmlNsPtr ns) {
  if (Connection::DebugBounces())
    cout << "************* now in PicSOM::AddToXMLbounce() *******" << endl;
  /*xmlNodePtr b = */ xmlNewTextChild(parent, ns, (XMLS)"bounce", NULL);
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::AddToXMLquerylistfull(xmlNodePtr parent, xmlNsPtr ns) const {
  bool ok = true;

  for (int i=0; i<2; i++) {
    if (i==0 && !Development())
      continue;

    if (Development()) {
      xmlNodePtr info = xmlNewTextChild(parent, ns, (XMLS)"queryinfo", NULL);
      xmlNewTextChild(info, ns, (XMLS)"identity",
		  (XMLS)(i?"--public--":"--private--"));
    }

    string rd = RootDir("queries", i);
    ok = ok && AddToXMLquerylistfullRecurse(parent,ns, rd.c_str());
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::AddToXMLquerylistfullRecurse(xmlNodePtr parent, xmlNsPtr ns,
					  const char *p) const {

  std::list<std::string> list = SortedDir(p);
  list.reverse();

  for (std::list<std::string>::iterator
	 i = list.begin(); i!=list.end(); i++) {
    const char *lab = (*i).c_str();
    char name[1000];
    sprintf(name, "%s/%s", p, lab);

    if (FileExists(name) && Query::SaneIdentity(lab) &&
	!FindQueryInMemory(lab)) {
      xmlNodePtr info = xmlNewTextChild(parent, ns, (XMLS)"queryinfo", NULL);
      xmlNewTextChild(info, ns, (XMLS)"identity",     (XMLS)lab);
      
    } else if (DirectoryExists(name))
      AddToXMLquerylistfullRecurse(parent, ns, name);
  }
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

const char *PicSOM::BakeCookieP(const char *userpw) const {
  if (userpw)
    for (int k=0; k<database.Nitems(); k++) {
      const char *ocookie = database[k].FindCookieP(userpw);
      if (ocookie)
	return ocookie;
    }

  static RandVar r;
  static char cookie[8];
  const char *spec = "tuttutu";
  const char *t = "rtpshjklvnm", *u = "aeiouy";
  size_t lt = strlen(t), lu = strlen(u), i;
  for (i=0; i<strlen(spec) && i<sizeof(cookie)-1; i++) {
    int j = r.RandomInt(int(spec[i]=='t' ? lt : lu));
    cookie[i] = spec[i]=='t' ? t[j] : u[j];
  }
  cookie[i] = 0;

  return cookie;    
}

///////////////////////////////////////////////////////////////////////////////

const char *PicSOM::CreateQueryListM(const Query& q) const {
  const char *ret = NULL;

  ShowError("const Connection *conn = NULL");
  const Connection *conn = NULL;

  for (size_t i=0; i<Nqueries(); i++)
    if (GetQuery(i)->CheckPermissions(q, *conn)) {
      if (!ret)
	CopyString(ret, "[\n");

      AppendString(ret, "{\n");
      AppendString(ret, GetQuery(i)->DisplayStringM(true), true);
      AppendString(ret, "}\n");
    }

  if (ret)
    AppendString(ret, "]\n");

//cout << "------------------" << endl << ret << "------------------" << endl;

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::DumpQueryList(ostream *os, bool recurse,
			   Query *q, int indent) const {
  if (q) {
    for (int i=0; i<indent; i++)
      *os << "  ";

    const char *str = q->DisplayStringM(false);
    *os << (void*)q << " " << str << endl;
    delete str;

    if (recurse)
      for (size_t j=0; j<q->Nchildren(); j++)
	DumpQueryList(os, true, q->Child(j), indent+1);

    return;
  }

  for (size_t i=0; i<Nqueries(); i++)
    DumpQueryList(os, recurse, GetQuery(i));
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::DumpDataBaseList(ostream *os) const {
  for (int i=0; i<database.Nitems(); i++) {
    *os << database[i].Name()
	<< " " << (void*)this
	<< endl;

    for (size_t k=0; k<database[i].Nindices(); k++)
      *os << "      " << database[i].IndexName(k)
	  << " " << (void*)database[i].GetIndex(k)
	  << endl; 

    for (size_t j=0; j<database[i].Nviews(); j++) {
      *os << "   " << database[i].View(j).Name()
	  << " " << (void*)&database[i].View(j)
	  << endl;

      for (size_t k=0; k<database[i].View(j).Nindices(); k++)
	*os << "      " << database[i].View(j).IndexName(k)
	    << " " << (void*)database[i].View(j).GetIndex(k)
	    << endl;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::WriteLogExit() {
  if (atexit_instance) {
    char pid[100];
    sprintf(pid, "%d", getpid());
    atexit_instance->WriteLog("Process ", pid,
			      " terminated by calling exit()");
  }
}

///////////////////////////////////////////////////////////////////////////////

const char *PicSOM::SignalNameP(int s) {
#if HAVE_STRSIGNAL
  return strsignal(s);
#else

  static const char *names[] = {
    "????",
#  if defined(sgi)
    /**/    "HUP" , "INT"    , "QUIT", "ILL"   , "TRAP", "ABRT", "EMT" ,
    "FPE" , "KILL", "BUS"    , "SEGV", "SYS"   , "PIPE", "ALRM", "TERM", 
    "USR1", "USR2", "CHLD"   , "PWR" , "WINCH" , "URG" , "IO"  , "STOP", 
    "TSTP", "CONT", "TTIN"   , "TTOU", "VTALRM", "PROF", "XCPU", "XFSZ",  
    "K32" , "CKPT", "RESTART", "UME"

#  elif defined(__alpha)
    /**/     "HUP",  "INT", "QUIT",  "ILL", "TRAP",  "ABRT",  "EMT",
     "FPE", "KILL",  "BUS", "SEGV",  "SYS", "PIPE",  "ALRM", "TERM", 
     "URG", "STOP", "TSTP", "CONT", "CHLD",  "TTIN", "TTOU",  "IO",
    "XCPU", "XFSZ","VTALRM","PROF","WINCH",  "INFO", "USR1", "USR2"

#  elif defined(__MINGW32__)
    /**/     "SIG1",  "INT",   "SIG3",  "ILL",   "SIG5",  "SIG6",  "SIG7",  
    "FPE" ,  "SIG9",  "SIG10", "SEGV",  "SIG12", "SIG13", "SIG14", "TERM", 
    "SIG16", "SIG17", "SIG18", "SIG19", "SIG20", "BREAK", "ABRT"
#  endif
  };

  return s>0 && s<sizeof(names)/sizeof(*names) ? names[s] : names[0];

#endif // HAVE_STRSIGNAL
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::WriteLogSignal(int s, const char *msg) {
  const char *name = SignalNameP(s);

  char *txt = new char[100+(msg?strlen(msg):0)];
  sprintf(txt, "Received in %d signal %s (%d) %s",
	  getpid(), name, s, msg?msg:"");
  WriteLog(txt);
  delete [] txt;
}

///////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetSignalHandlers(bool in_bg) {
    SetSignalHandler(SIGINT,  &PicSOM::SaveAllAndExitSignal);
    SetSignalHandler(SIGTERM, &PicSOM::SaveAllAndExitSignal);

#ifdef SIGXCPU
    // __alpha : SetSignalHandler(SIGXCPU, &PicSOM::KillParentAndSelf);
    SetSignalHandler(SIGXCPU, &PicSOM::SaveAllAndExitSignal);
#endif // SIGXCPU
#ifdef SIGPIPE
    SetSignalHandler(SIGPIPE, &PicSOM::LogSignalAndContinue);
#endif // SIGPIPE
#ifdef SIGHUP
    SetSignalHandler(SIGHUP,  &PicSOM::ShowConnectionsSignal);
#endif // SIGHUP
#ifdef SIGUSR1
    SetSignalHandler(SIGUSR1, &PicSOM::DumpQueryListSignal);
#endif // SIGUSR1
#ifdef SIGUSR2
    //SetSignalHandler(SIGUSR2, &PicSOM::ShowTimesSignal);
    SetSignalHandler(SIGUSR2, &PicSOM::DumpDataBaseListSignal);
#endif // SIGUSR2

    if (in_bg || use_signals) {
      SetSignalHandler(SIGABRT, &PicSOM::LogSignalAndExit);
      SetSignalHandler(SIGFPE,  &PicSOM::LogSignalAndExit);
      SetSignalHandler(SIGILL,  &PicSOM::LogSignalAndExit);
      SetSignalHandler(SIGSEGV, &PicSOM::LogSignalAndExit);
#ifdef SIGBUS
      SetSignalHandler(SIGBUS,  &PicSOM::LogSignalAndExit);
#endif // SIGBUS
#ifdef SIGEMT
      SetSignalHandler(SIGEMT,  &PicSOM::LogSignalAndExit);
#endif // SIGEMT
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::SetSignalHandler(int s, void (PicSOM::*f)(int)) {
    bool show = false;

    if (show) {
      char head[100];
      sprintf(head, "Setting handler in %d for signal ", getpid());
      WriteLog(head, ToStr(s), " = ", SignalNameP(s));
    }

    if (s>=0 && s<=127) {
      signal_instance[s] = this;
      signal_function[s] = f;
      signal(s, (CFP_signal) SignalHandler);
    }
  }

///////////////////////////////////////////////////////////////////////////////

void PicSOM::SignalHandler(int s) {
  // cout << "PicSOM::SignalHandler(" << s << ")" << endl;
  
  (signal_instance[s]->*signal_function[s])(s);  
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::KillParentAndSelf(int) {
  int signal = SIGTERM;

#ifdef HAVE_GETPPID
  if (getppid()!=1) {
    kill(getppid(), SIGINT);
    sleep(30);
  }
  if (getppid()!=1)
    kill(getppid(), SIGKILL);

  signal = SIGKILL;
#endif // HAVE_GETPPID

  kill(getpid(), signal);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::LogSignalAndExit(int s) {
  WriteLogSignal(s, "in LogSignalAndExit()");

  if (BackTraceBeforeTrap())
    BackTraceMeHere();
    
  exit(1);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::LogSignalAndContinue(int s) {
  WriteLogSignal(s, "in LogSignalAndContinue()");
  SetSignalHandler(s, &PicSOM::LogSignalAndContinue);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::DummySignal(int s) {
  WriteLogSignal(s, "in DummySignal()");
  SetSignalHandler(s, &PicSOM::DummySignal);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ShowConnectionsSignal(int s) {
  WriteLogSignal(s, "in ShowConnectionsSignal()");
  WriteLog("---");
  ostringstream cstr;
  ShowConnections(NULL, &cstr);
  WriteLog(cstr);
  WriteLog("---");
  SetSignalHandler(s, &PicSOM::ShowConnectionsSignal);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::DumpDataBaseListSignal(int s) {
  WriteLogSignal(s, "in DumpDataBasesSignal()");
  WriteLog("---");
  ostringstream cstr;
  DumpDataBaseList(&cstr);
  WriteLog(cstr);
  WriteLog("---");
  SetSignalHandler(s, &PicSOM::DumpDataBaseListSignal);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::DumpQueryListSignal(int s) {
  WriteLogSignal(s, "in DumpQueryListSignal()");
  WriteLog("---");
  ostringstream cstr;
  DumpQueryList(&cstr);
  WriteLog(cstr);
  WriteLog("---");
  SetSignalHandler(s, &PicSOM::DumpQueryListSignal);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::DumpMemoryUsageSignal(int s) {
  WriteLogSignal(s, "in DumpMemoryUsageSignal()");
  WriteLog("---");
  ostringstream cstr;
  DumpMemoryUsage(cstr);
  WriteLog(cstr);
  WriteLog("---");
  SetSignalHandler(s, &PicSOM::DumpMemoryUsageSignal);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ShowTimesSignal(int s) {
  WriteLogSignal(s, "in ShowTimesSignal()");
  ostringstream cstr;
  ShowTimes(true, true, cstr);
  WriteLog(cstr);
  SetSignalHandler(s, &PicSOM::ShowTimesSignal);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::SaveAllAndExitSignal(int s) {
  WriteLogSignal(s, "in SaveAllAndExitSignal()"); 
  SaveAllAndExit();
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::SaveAllAndExit() {
  for (int i=0; i<database.Nitems(); i++)
    GetDataBase(i)->CloseTextIndices();

  SaveOldQueries(true);
  CloseAllConnections();
#ifdef PICSOM_USE_PTHREADS
  CancelRunningQueries();
  RemoveFinishedQueries(true);
#endif // PICSOM_USE_PTHREADS
  PossiblyShowDebugInformation("SaveAllAndExit() before cleanup", true);

  int ndb = NdataBases();
  for (int i=0; i<ndb; i++)
    delete GetDataBase(i);

  PossiblyShowDebugInformation("SaveAllAndExit() after cleanup", true);
  WriteLog("All done, bye ;)");
  exit(1);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_PTHREADS

void PicSOM::BlockSignals() {
#ifdef HAVE_SIGSET_T
  if (HandleSignals()) {
    sigset_t sigset;
    sigfillset(&sigset);
    if (pthread_sigmask(SIG_SETMASK, &sigset, NULL))
      ShowError("BlockSignals(): pthread_sigmask()");
  }
#endif // HAVE_SIGSET_T
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::ConditionallyAnnounceThread(const string& txt, pthread_t r,
                                         pid_t t) {
  if (debug_threads && !pthread_equal(pthread_self(), r))
    WriteLog("Launched thread [", ThreadIdentifierUtil(r, t), "] ",
             txt!=""?txt:"");
}

#endif // PICSOM_USE_PTHREADS

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::MakeDirectory(const char *p, bool is_file) /*const*/ {
  if (is_file && !strchr(p, '/'))
    return true;

  if (!is_file && DirectoryExists(p))
    return true;

  int mode = 02775;

  if (!is_file && MkDir(p, mode))
    return true;

  char tmp[1024];
  strcpy(tmp, p);
  char *s = strrchr(tmp, '/');
  if (!s)
    return false;

  *s = 0;
  if (!MakeDirectory(tmp, false))
    return false;

  if (is_file || MkDir(p, mode))
    return true;

  return false;
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::MkDirHier(const string& dir, int mode) const {
    if (!dir.size())
      return ShowError("MkDirHier() with empty dir called");

    for (size_t p=1; p!=string::npos; ) {
      size_t s = dir.find('/', p);
      if (s==string::npos)
	s = dir.length();

      string d = dir.substr(0, s);
      if (!DirectoryExists(d) && !MkDir(d, mode))
	return ShowError("MkDir("+d+") failed");

      if (s==dir.length())
	break;

      p = s+1;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::MkDir(const string& dir, int mode) const {
#if MKDIR_ONE_ARG
    int r = mkdir(dir.c_str());
#else
    int r = mkdir(dir.c_str(), mode);
#endif // MKDIR_ONE_ARG
    if (!r) {
      chmod(dir.c_str(), mode);

      //     struct stat st;
      //     stat(dir, &st);
      //     char mod[100];
      //     sprintf(mod, " 0%05o 0x%04x", st.st_mode, st.st_mode);

      WriteLog("Created directory <"+dir+">");
    }

    return r==0;
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::SaveOldQueries(bool force) const {
  if (no_save)
    return true;

  const int save_after_seconds = force ? -1 : 5;
  // WriteLog("now in SaveOldQueries()");
  
  ((PicSOM*)this)->MutexLock();
  RwLockReadQueryList();
  for (size_t i=0; i<Nqueries(); i++)
    if (!MoreRecent(GetQuery(i)->LastModificationTime(true),
		    save_after_seconds) &&
	GetQuery(i)->NeedsSave()) {
      string sn = GetQuery(i)->SavePath(false);
      // WriteLog("calling Save() for ", GetQuery(i)->Identity());
      if (sn!="")
	GetQuery(i)->Save(sn, false);
    }
  RwUnlockReadQueryList();
  ((PicSOM*)this)->MutexUnlock();

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::ExpungeOldQueries() {
  if (expunge_time<0)
    return true;

  RwLockWriteQueryList();

  for (int i=Nqueries()-1; i>=0; i--) {
    struct timespec t = GetQuery(i)->LastAccessTime(true);
    if (!MoreRecent(t, expunge_time) && GetQuery(i)->SavedAfter(t)) {
      ostringstream msg;
      msg << "Expunged last access=" << TimeString(t)
	  << " saved=" << TimeString(GetQuery(i)->SavedTime())
	  << " querylist size then " << Nqueries()-1;
      GetQuery(i)->WriteLog(msg);
      
      DeleteQuery(i);
    }
  }

  RwUnlockWriteQueryList();

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::CloseAllConnections() {
  ostringstream msg;
  msg << "Closing " << Connections(false, false) << " connections";
  WriteLog(msg.str());

  bool ok = true;

  for (size_t i=0; i<Connections(false, false); i++)
    if (GetConnection(i)->Type()!=Connection::conn_terminal)
      ok = ok && GetConnection(i)->Close(false);

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

xmlDocPtr PicSOM::HTMLfile(xmlNodePtr *head, xmlNodePtr *body) {
  xmlDocPtr doc   = xmlNewDoc((XMLS)"1.0");
  xmlNsPtr   ns   = NULL;
  xmlNodePtr html = NewDocNode(doc, ns, "html");
  xmlDocSetRootElement(doc, html);
  
  if (head)
    *head = xmlNewTextChild(html, ns, (XMLS)"head", NULL);

  if (body)
    *body = xmlNewTextChild(html, ns, (XMLS)"body", NULL);

  return doc;
}

  ///////////////////////////////////////////////////////////////////////////////

  string PicSOM::HTMLlocalLinkImage(const string& src, const string& idir,
                                    bool copy) {
    const string dst = idir+"/"+BaseName(src);
    const string ret = BaseName(dst);

    const string msg = "HTMLlocalLinkImage(): "+src+" -> "+dst+": ";

    if (FileExists(dst))
      return ret;

    bool ok = copy ? CopyFile(src, dst) : Symlink(src, dst);

    if (!ok) {
      ShowError(msg, copy ? "copying" : "symlinking", " failed!");
      return "";
    }

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////////

  void PicSOM::HTMLlocalTnAndImage(xmlNodePtr p, const string& l,
                                   DataBase* db, const string& dir, int link) {
    const string msg = "HTMLlocalTnAndImage(): ";

    xmlNodePtr a = AddTag(p, NULL, "a");
    xmlNodePtr b = AddTag(a, NULL, "img");

    vector<string> tnss = db->FindThumbnailSizesList();
    string im = db->SolveObjectPath(l, "", "", false, NULL, true);
    if (im=="") // obs! trecvid2005 trick
      im = db->SolveObjectPath(l+":2", "", "", false, NULL, true);

    string tn;
    for (vector<string>::const_iterator i=tnss.begin();
         tn=="" && i!=tnss.end(); i++)
      tn = db->SolveObjectPath(l, "tn-"+*i);

    string file = "file://";

    if (link) {
      const string sdir = "images";
      const string idir = dir+"/"+sdir;
      if (!DirectoryExists(idir)) {
#if MKDIR_ONE_ARG
       int ret = mkdir(idir.c_str());
#else
       int ret = mkdir(idir.c_str(), 0755);
#endif // MKDIR_ONE_ARG
        if (ret == -1) {
          ShowError(msg, "Could not create directory ", idir, ": ",
                    strerror(errno));
          return;
        }
      }

      const bool copy = link == 2;

      if (!im.empty())
        im = sdir+"/"+HTMLlocalLinkImage(im, idir, copy);

      if (!tn.empty())
        tn = sdir+"/"+HTMLlocalLinkImage(tn, idir, copy);
    
      file = "";
    }

    if (im!="" && tn!="")
      SetProperty(a, "href", file+im);
    if (tn!="")
      SetProperty(b, "src",  file+tn);
    else
      SetProperty(b, "src",  file+im);

    SetProperty(b, "width", "100");
    SetProperty(b, "height", "100");
  }

  ///////////////////////////////////////////////////////////////////////////////

void PicSOM::HTMLserverTnAndImage(xmlNodePtr p, const string& l,
				  const DataBase* db, const Query *q,
				  const list<pair<string,string> >& extra) {

  xmlNodePtr img = AddTag(p, NULL, "::IMAGE::");
  SetProperty(img, "label",      l);
  SetProperty(img, "database",   db->Name());
  SetProperty(img, "query",      q->Identity());

  size_t idx = db->LabelIndex(l);
  map<string,string> info = db->ReadOriginsInfo(idx, true, true);
  for (map<string,string>::const_iterator i = info.begin();
       i!=info.end(); i++)
    SetProperty(img, i->first, i->second);

  //   string dim = db->SolveImageDimensions(l, true);
  //   if (dim=="")
  //     ShowError("HTMLserverTnAndImage(", l,
  // 	      ") failed in SolveImageDimensions()");
  //   else
  //     SetProperty(img, "dimensions", dim);

  for (list<pair<string,string> >::const_iterator i=extra.begin();
       i!=extra.end(); i++)
    SetProperty(img, i->first, i->second);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::HTMLlocalMap(xmlNodePtr p, const Query *q, int m, int l, int k,
			  const string& prefix) {
  stringstream ssn, ss;
  ss  << prefix << q->Identity() << "_";
  ssn << q->IndexShortName(m) << "_" << l;
  if (q->GetMatrixCount(m)!=1)
    ssn << "_" << k;
  
  string ssns=ssn.str(), hname=ss.str()+ssns, cname=hname+"_conv";
	      
  xmlNodePtr i = HTMLimg(p, cname+".png");
  SetProperty(i, "width", 100);
  SetProperty(i, "height", 100);
  SetProperty(i, "border", 1);
}

///////////////////////////////////////////////////////////////////////////////

void PicSOM::HTMLserverMap(xmlNodePtr p, const Query *q, int m, int l, int k) {

  xmlNodePtr i = AddTag(p, NULL, "::MAP::");
  SetProperty(i, "query",   q->Identity());
  SetProperty(i, "feature", q->IndexShortName(m));
  SetProperty(i, "level",   l);
  SetProperty(i, "matrix",  k);
}

///////////////////////////////////////////////////////////////////////////////

string PicSOM::FindExecutable(const string& dir, const string& name,
			      const string& ext) const {

  set<string> known { "tesseract" };

  if (dir=="" && ext=="") {
    string bindir = Path()+"/"+SolveArchitecture()+"/bin";

    if (known.find(name)!=known.end()) {
      string tmp = bindir+"/"+name;
      if (FileExists(tmp))
	return tmp;
    }

    if (name=="ffmpeg" || name=="avconv") {
      string tmp = bindir+"/avconv";
      if (FileExists(tmp))
	return tmp;
      tmp = bindir+"/ffmpeg";
      if (FileExists(tmp))
	return tmp;
      tmp ="/usr/bin/avconv";
      if (FileExists(tmp))
	return tmp;
      tmp ="/usr/bin/ffmpeg";
      if (FileExists(tmp))
	return tmp;
    }
    return "/bin/false";
  }

  string basename = RootDir(dir) + "/" + name;
  if (ext!="")
    basename += string(".") + sysarchitecture + ".";

  string filename = basename + ext;
  if (FileExists(filename))
    return filename;

  // if the file doesn't exist, try another extension.
  const string extensions[] = { "fast", "debug", "" };
  for (int i=0; i<3; i++) {
    if (extensions[i] == ext)
      continue;
    filename = basename + extensions[i];
    if (FileExists(filename))
      return filename;
  }

  return ""; // return empty string if not suitable executable was found
}

  /////////////////////////////////////////////////////////////////////////////

  target_type MIMEtypeToTargetType(const string& s) {
    if (s.find("picsom/")==0)
      return SolveTargetTypes(s.substr(7));

    if (s=="image/unknown") // fallback value used by Analysis::InsertObjects()
      return target_file;

    if (s=="text/html")
      return target_htmlfile;

    if (s=="text/xml" || s=="application/json")
      return target_datafile;

    if (s.find("text/")==0)
      return target_textfile;

    if (s.find("image/")==0)
      return target_imagefile;

    if (s.find("message/")==0)
      return target_messagefile;

    if (s.find("video/")==0)
      return target_videofile;

    if (s.find("audio/")==0)
      return target_soundfile;

    if (s.find("application/x-cogain")==0)
      return target_type(target_gazetrack+target_file);

    if (s=="application/x-empty")
      return target_no_target;

    if (s=="application/zip")
      return target_imageset;

    if (s=="application/x-tar")
      return target_imageset;

    Simple::ShowError("MIMEtypeToTargetType() : unable to convert <", s, ">"); 

    return target_error;
  }

  /////////////////////////////////////////////////////////////////////////////

  target_type SolveTargetTypes(const string& strin, bool warn) {
    typedef map<string,target_type> tmap_t;
    static tmap_t tmap;

    tmap_t::iterator i = tmap.find(strin);
    if (i==tmap.end()) {
      string str = strin;
      while (str.size() && str[str.size()-1]==' ')
	str.erase(str.size()-1);
      while (str[0]==' ')
	str.erase(0, 1);

      vector<string> a = SplitInSomething("+", false, str);
      int ttsum = 0;
      for (size_t j=0; j<a.size(); j++) {
	target_type tt = TargetType(a[j]);
	if (tt==target_error) {
	  if (warn)
	    Simple::ShowError("SolveTargetTypes() : unknown target type \""+
			      a[j]+"\" in \""+strin+"\"");
	  ttsum = target_no_target;
	  break;
	}
	ttsum += tt;
      }
      tmap[strin] = target_type(ttsum);
      i = tmap.find(strin);
    }
    return i->second;
  }

  /////////////////////////////////////////////////////////////////////////////

  /// Creates a new query and takes care of it.
  Query *PicSOM::NewQuery() {
    return AppendQuery(new Query(this), false);
  }

  /////////////////////////////////////////////////////////////////////////////

  /// Adds query to the list of queries.
  Query *PicSOM::AppendQuery(Query *q, bool check) {
    RwLockWriteQueryList();
    query.push_back(q);
    RwUnlockWriteQueryList();

    if (check && !q->SaneIdentity())
      ShowError("AppendQuery() : insane identity <", q->Identity(), ">");

    q->WriteLog("Appended, querylist size now ", ToStr(Nqueries()));

    PossiblyShowDebugInformation("After appending a new query");

    return q;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::DeleteQuery(Query *q) {
    RwLockWriteQueryList();

    bool found = false;
    for (size_t i=0; !found && i<Nqueries(); i++)
      if (GetQuery(i)==q) {
	delete q;
	query.erase(query.begin()+i, query.begin()+i+1);
	found = true;
      }

    RwUnlockWriteQueryList();

    return found || ShowError("PicSOM::DeleteQuery(Query*) failed");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::DeleteQuery(size_t i) {
    if (i>=Nqueries())
      return ShowError("PicSOM::DeleteQuery(size_t) failed");

    RwLockWriteQueryList();

    delete GetQuery(i);
    query.erase(query.begin()+i, query.begin()+i+1);

    RwUnlockWriteQueryList();

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::HasTerminalInput() {
#ifdef HAS_ISATTY
  return isatty(STDIN_FILENO);
#else
  return false;
#endif // HAS_ISATTY
}

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreateTerminalConnection() {
    return AppendConnection(Connection::CreateTerminal(this));
  }

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreateFileConnection(const char *n) {
    return AppendConnection(Connection::CreateFile(this, n));
  }

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreateStreamConnection(istream& s) {
    return AppendConnection(Connection::CreateStream(this, s));
  }

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreatePipeConnection(const vector<string>& v,
                                           bool errnull) {
    return AppendConnection(Connection::CreatePipe(this, v, errnull));
  }

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreateListenConnection(int a, int b) {
    return AppendConnection(Connection::CreateListen(this, a, b));
  }

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreateMPIListenConnection() {
    return AppendConnection(Connection::CreateMPIListen(this));
  }

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreateMPIConnection(const string& port, bool c) {
    return AppendConnection(Connection::CreateMPI(this, port, c));
  }

  /// Creates a connection which is appended to the list.
  Connection *PicSOM::CreateSocketConnection(int i, const RemoteHost& h) {
    Connection *c = Connection::CreateSocket(this, i, h);
    if (c->IsHttpServer() && c->IsClosed()) {
      if (Connection::DebugHTTP())
        c->Notify(false, "was already closed", true);
      delete c;
      return NULL;

    } else    
      return AppendConnection(c);
  }

  /// Creates an uplink which is appended to the list.
  Connection *PicSOM::CreateUplinkConnection(const string& address,
					     bool con, bool ack) {
    return AppendConnection(Connection::CreateUplink(this, address, con, ack));
  }

  /// Creates an HTTP client connection which is appended to the list.
  Connection *PicSOM::CreateHttpClientConnection(const string& u, bool s,
						 int p) {
    return AppendConnection(Connection::CreateHttpClient(this, u, s, p));
  }

  /// Creates an uplink which is appended to the list.
  Connection *PicSOM::CreateSoapServerConnection(int p) {
    return AppendConnection(Connection::CreateSoapServer(this, p));
  }

  /// Creates an uplink which is appended to the list.
  Connection *PicSOM::CreateSoapClientConnection(const string& url) {
    return AppendConnection(Connection::CreateSoapClient(this, url));
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::CloseConnection(Connection *c, bool dodel, bool angry,
				 bool notify) {
    if (!c)
      return false;

    bool ok = true;
    if (!c->Close(angry, notify))
      ok = false;
   
    vector<Connection*>::iterator i = connection.begin();
    while (i!=connection.end() && *i!=c)
      i++;

    if (i==connection.end())
      ok = false;
    else
      connection.erase(i);
    
    if (dodel)
      delete c;

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetUpMPIListenConnection() {
    string hdr = "SetUpMPIListenConnection() : ";

    if (mpi_listen_connection)
      return true;

    mpi_listen_connection = CreateMPIListenConnection();
    if (!mpi_listen_connection)
      return ShowError(hdr+"CreateMPIListenConnection() failed");

#ifdef PICSOM_USE_MPI
    WriteLog("Started MPI port \""+mpi_listen_connection->mpi_port+"\"");
#endif // PICSOM_USE_MPI    

#ifdef PICSOM_USE_PTHREADS
    return StartMPIListenThread();
#else
    retrun ShowError(hdr+"PICSOM_USE_PTHREADS undef");
#endif // PICSOM_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetUpListenConnection(bool lstn, bool thrd, const string& msg) {
    string hdr = "SetUpListenConnection() : ";

    if (listen_connection)
      return true;

    if (!listen_porta && !ListenPort(UserName()))
      return ShowError(hdr+"ListenPort("+UserName()+") failed");

    listen_connection = CreateListenConnection(listen_porta, listen_portb);
    if (!listen_connection)
      ShowError(hdr+"CreateListenConnection("+ToStr(listen_porta)+","+
		ToStr(listen_portb)+") failed");

    if (listen_connection) {
      string url = HttpListenConnection(), urldb = url;
      if (!allowed_databases.empty())
        urldb += "/database/"+*allowed_databases.rbegin();

      if (msg!="")
        cout << endl << "Locate your browser to " << url
             << msg << endl << endl;

#ifdef PICSOM_USE_PTHREADS
      if (launch_browser_str!="") {
        string ffurl = urldb;
        if (launch_browser_str!="true") {
          if (launch_browser_str[0]=='/')
            ffurl = url+launch_browser_str;
          else if (launch_browser_str!=".")
	    ffurl = urldb+"/"+launch_browser_str;
        }
        vector<string> cmd;
        cmd.push_back(browser);
        cmd.push_back(ffurl);
        cmd.push_back("1>/dev/null");
        cmd.push_back("2>&1");
        cmd.push_back("&");
        ExecuteSystem(cmd, false, true, true);
      }
#endif // PICSOM_USE_PTHREADS
    }

    if (!lstn)
      return true;

    if (thrd)
#ifdef PICSOM_USE_PTHREADS
      return PthreadMainLoop();
#else
      return ShowError(hdr+"cannot enter main loop without threading!");
#endif // PICSOM_USE_PTHREADS

    return MainLoop();
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::HttpListenConnection() const {
    if (!listen_connection)
      return "";

    string hname = uselocalhost ? "localhost" : HostName(true);

    string url = "http://"+hname+":"+ToStr(listen_connection->Port());

    return url;
  }

///////////////////////////////////////////////////////////////////////////////

#ifdef PICSOM_USE_PTHREADS
  /// Adds now pthread,Query pair in the list of running queries.
  void PicSOM::AppendRunningQuery(pthread_t p, Query *q) {
    RemoveFinishedQueries(false);

    query_thread.push_back(query_thread_e(p, q));

    if (DebugThreads()) {
      stringstream tmp;
      tmp << "/" << ThreadIdentifierUtil(p, gettid()) << " " << q->Identity();
      WriteLog("Appended running query pthread ", tmp.str(), " list size now ",
	       ToStr(NrunningQueries()));
    }
  }

  /// Removes any finished queries from the pthread,Query pair list.
  void PicSOM::RemoveFinishedQueries(bool force) {
    return; // OBS! OBS! OBS!

    if (query_thread.size()) {
      ostringstream msg;
      msg << "Joining " << query_thread.size() << " query threads";
      WriteLog(msg.str());
    }

    for (query_thread_t::iterator i = query_thread.begin();
	 i!=query_thread.end(); )
      if (force || !i->second->InRun()) {
	if (DebugThreads() || (force && i->second->InRun())) {
	  stringstream tmp;
	  tmp << "/" << ThreadIdentifierUtil(i->first, 0) // obs! wrong
	      << " " << i->second->Identity();
	  WriteLog("Joining running query pthread ", tmp.str(), " list size "
		   "then ", ToStr(NrunningQueries()-1));
	}
	pthread_join(i->first, NULL);
	i = query_thread.erase(i);
      } else
	i++;
  }
  /// Sends cancellation to all running queries.

  void PicSOM::CancelRunningQueries() {
    RemoveFinishedQueries(false);

    if (query_thread.size()) {
      ostringstream msg;
      msg << "Cancelling " << query_thread.size() << " running query threads";
      WriteLog(msg.str());
    }

    for (query_thread_t::iterator i = query_thread.begin();
	 i!=query_thread.end(); i++) {
      if (DebugThreads()) {
	stringstream tmp;
	tmp << "/" << ThreadIdentifierUtil(i->first, 0) // obs! wrong
            << " "<< i->second->Identity();
	WriteLog("Cancelling running query pthread ", tmp.str());
      }
      pthread_cancel(i->first);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::SleepIfNoThreadsAvailable() {
    bool debug = DebugThreads();
    
    while (!ThreadsAvailable()) {
      if (debug) {
	cout << "SleepIfNoThreadsAvailable() : "
	     << "waiting for an available thread: "
	     << ThreadsRunning() << " / "
	     << ThreadsTotal() << endl;
	sleep(1);
      }
      struct timespec ts = { 0, 1000000 }; // 1 millisecond
      nanosleep(&ts, NULL);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::RegisterThread(pthread_t p, pid_t tid, const string& t,
				const string& txt, void *o, void *d) {
    static RwLock lock;
    lock.LockWrite();

    ThreadListSanityCheck();

    static int count = 0;
    int mynum = count++;

    string n = t+"-"+ToStr(mynum), oname = o ? "???" : "NULL";
    if (o) {
      if (t=="analysis")
	oname = ((Analysis*)o)->Identity();

    } else
      if (t=="picsom")
	oname = "";

    thread_info_t e = { n, p, tid, true, "created", t, txt,
			"*thread*", o, d, oname };
    thread_list.push_back(e);

    ThreadListSanityCheck();

    string thnum = ToStr(ThreadsRunning())+"("+ToStr(ThreadsPending())+")";
    thnum += "/"+string(threads>=0?ToStr(threads):"unlimited");
    thnum += "/"+ToStr(ThreadsTotal());
    thnum += "("+ToStr(ThreadsUninit())+"+"+
      ToStr(ThreadsReady())+"+"+ToStr(Threads("collected"))+"/"+
      ToStr(ThreadsJoined())+")";

    WriteLog("Registered thread "+thnum+" \""+oname+"\" <"+n+"> "+t+" "+
	     ThreadIdentifier(e)+" "+ToStr(o)+" "+ToStr(d)+" : " + txt);

    lock.UnlockWrite();

    return n;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::UnregisterThread(const string& n) {
    ThreadListSanityCheck();

    for (thread_list_t::iterator i=thread_list.begin();
	 i!=thread_list.end(); i++)
      if (i->name==n) {
	string thnum = ToStr(ThreadsRunning())+"/"+
	  (threads>=0?ToStr(threads):"unlimited");
	thnum += "/"+ToStr(ThreadsTotal());
	thnum += "("+ToStr(ThreadsUninit())+"+"+
	  ToStr(ThreadsReady())+"+"+ToStr(Threads("collected"))+"/"+
	  ToStr(ThreadsJoined())+")";

	WriteLog("Unregistering thread "+ThreadIdentifier(*i)+" "+thnum+
		 " \""+i->showname+"\" <"+i->name+">");

	RwLockWriteThreadList();
	thread_list.erase(i);
	RwUnlockWriteThreadList();

	return ThreadListSanityCheck();
      }

    return ShowError("Thread \""+n+"\" inexistent");
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t PicSOM::Threads(const string& s, const string& t) const {
    bool debug = debug_threads;

    string msg = "PicSOM::Threads("+s+","+t+") : ";
    size_t n = 0;

    RwLockReadThreadList();

    for (thread_list_t::const_iterator i=thread_list.begin();
	 i!=thread_list.end(); i++) {
      const string& state = ThreadState(&*i);
      const string& type = i->type;
      if (debug)
	cout << msg << i->name << " / " << i->phase << " / "
	     << i->type << " / " << state << " : "
	     << ThreadInfoString(*i, false) << endl;
      if ((state==s || s=="") && (type==t || t==""))
	n++;
    }

    RwUnlockReadThreadList();

    if (debug)
      cout << msg << "returning " << n << endl;

    return n;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::JoinThreads(bool /*wait*/) {
    ThreadListSanityCheck();

    Threads("");

    for (auto& i : thread_list )
      if (i.phase=="created") {
	bool do_join = false;

	if (i.type=="picsom" && i.text=="listen") {
	  if (pthread_cancel(i.id)) {
	    i.phase="cancel failed";
	    return ShowError("JoinThreads() : pthread_cancel() failed");
	  }
	  do_join = true;
	}

	string state = ThreadState(&i);
      
	if (i.type=="analysis" && (state=="ready" || state=="errored"))
	  do_join = true;

	if (!do_join)
	  continue;

	void *p = NULL;
	if (pthread_join(i.id, &p)) {
	  i.phase="join failed";
	  return ShowError("JoinThreads() : pthread_join() failed");
	}

	i.phase="joined";
	WriteLog("Joined thread "+ThreadInfoString(i, false));
	// i.thread_set = false; // commented out 2012-05-16, may cause trouble?
      }

    return ThreadListSanityCheck();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::ThreadListSanityCheck() const {
    string msg = "ThreadListSanityCheck() : ";

#ifndef PTW32_VERSION
    set<pthread_t> p;
    set<string> n;

    for (const auto& i : thread_list)
      if (i.phase=="created") {
	if (p.find(i.id)!=p.end())
	  return ShowError(msg+"id duplicated : "+ThreadInfoString(i, false));

	if (n.find(i.name)!=n.end())
	  return ShowError(msg+"name duplicated : "+ThreadInfoString(i, false));

	if (!i.thread_set)
	  return ShowError(msg+"thread_set==false : "+ThreadInfoString(i, false));
			 
	p.insert(i.id);
	n.insert(i.name);

      } else if (i.phase=="joined") {
	if (i.thread_set && false) // test disabled 2012-05-16
	  return ShowError(msg+"thread_set==true : "+ThreadInfoString(i, false));

	if (n.find(i.name)!=n.end())
	  return ShowError(msg+"name duplicated : "+ThreadInfoString(i, false));

	n.insert(i.name);

      } else
	return ShowError(msg+"phase unknown : "+ThreadInfoString(i, false));
#endif // PTW32_VERSION

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::ThreadExecuteCommand(thread_info_t *t, const string& cmd,
				  bool& intact) {
  string msg = "ThreadExecuteCommand() : ";

  intact = true;

  thread_list_t::iterator i = thread_list.begin();
  while (i!=thread_list.end() && &*i!=t)
    i++;

  if (i==thread_list.end())
    return ShowError(msg+"thread not found");

  string name = i->name;

  if (cmd=="info")
    return true;

  if (cmd=="terminate") {
    Analysis *a = i->type=="analysis" ? (Analysis*)i->object : NULL;

    if (i->phase=="created") {
      if (pthread_cancel(i->id))
	return ShowError(msg+"pthread_cancel() failed");
      
      if (pthread_join(i->id, NULL))
	return ShowError(msg+"pthread_join() failed");

      // i->thread_set = false; ???

    } else if (i->phase!="joined")
      return ShowError(msg+"phase <"+t->phase+"> unknown");

    RwLockWriteThreadList();
    thread_list.erase(i);
    RwUnlockWriteThreadList();

    intact = false;

    WriteLog("Thread <"+name+"> terminated");

    if (a)
      RemoveAnalysis(a);    

    return true;
  }

  return ShowError(msg+"command <"+cmd+"> unknown");
}

#endif // PICSOM_USE_PTHREADS

///////////////////////////////////////////////////////////////////////////////

bool PicSOM::ExecuteCommand(const string& cmd, const string& /*spec*/,
			    XmlDom& xml, Connection *conn) {
  string msg = "ExecuteCommand() : ";

  if (cmd=="bye") {
    if (!conn)
      return ShowError(msg+"cmd==bye && conn==NULL");

    xml.Element("connection", "bye");

    WriteLog("Command <bye> issued by "+conn->LogIdentity());
    conn->Close();  // so nothing really gets written out there...
    return true;
  }

  if (cmd=="quit") {
    if (!conn)
      return ShowError(msg+"cmd==quit && conn==NULL");

    WriteLog("Command <quit> issued by "+conn->LogIdentity());
    quit = true;
    return true;
  }

  if (cmd=="exit") {
    if (!conn)
      return ShowError(msg+"cmd==quit && conn==NULL");

    WriteLog("Command <exit> issued by "+conn->LogIdentity());
    exit(1);
    return true;
  }

  return ShowError(msg+"command <"+cmd+"> not known");
}

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::LabelTest(const string& dbname) {
    StartTimes();

    DataBase *db = FindDataBase(dbname);
    bool ok = db->LabelTest();

    PossiblyShowDebugInformation("LabelTest()", true);

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::HttpClientTest(const string& url) {
    string msg = "PicSOM::HttpClientTest("+url+") : ";

    string proto, host, portstr, path;
    SplitURL(url, proto, host, portstr, path);
    int port = portstr!="" ? atoi(portstr.c_str()) : 0;
    bool ssl = false;
    if (proto=="https")
      ssl = true;

    if (proto!="" && proto!="http" && !ssl)
      return ShowError(msg+"protocol \""+proto+"\" unknown");

    Connection *x = CreateHttpClientConnection(host, ssl, port);
    list<pair<string,string> > hdrs;
    string data, ctype;
    x->HttpClientGetString("GET", path, hdrs, "", data, ctype);
    cout << "data.size()=" << data.size() << endl;
    delete x;

    StringToFile(data, "http_client.bin");

    if (false) {
      imagefile ifile = imagefile::unstringify(data); // this doesn't work
      imagedata idata = ifile.frame(0);
      cout << idata.info() << endl;
      imagefile::display(idata);
    }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

cbir_algorithm PicSOM::CbirAlgorithm(const string& name) {
  for (int a=0;; a++) {
    const char *str = CbirAlgorithmP((cbir_algorithm)a);
    if (!str)
      return cbir_no_algorithm;

    if (name==str)
      return (cbir_algorithm)a;
  }
}

///////////////////////////////////////////////////////////////////////////////

const char *PicSOM::CbirAlgorithmP(cbir_algorithm a) {
  struct typelist {
    const char *str;
    cbir_algorithm type;
  };

  static typelist list[] = {
#define make_algorithm_entry(e) { #e , cbir_ ## e }
    make_algorithm_entry(no_algorithm),
    make_algorithm_entry(random),
    make_algorithm_entry(picsom),
    make_algorithm_entry(picsom_bottom),
    make_algorithm_entry(picsom_bottom_weighted),
    make_algorithm_entry(vq),
    make_algorithm_entry(vqw),
    make_algorithm_entry(sq),
    make_algorithm_entry(external),
    { NULL, cbir_no_algorithm }
#undef  make_algorithm_entry
  };

  for (typelist *ptr = list; ptr->str; ptr++)
    if (ptr->type==a)
      return ptr->str;

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

  void PicSOM::AppendAnalysis(Analysis *a) {
    analysis.push_back(a);
    WriteLog("Analysis [", a->IdName(), "] appended, list size now ",
	     ToStr(Nanalyses()));
  }

  /////////////////////////////////////////////////////////////////////////////

  Analysis *PicSOM::FindAnalysis(const string& an) const {
    for (size_t i=0; i<Nanalyses(); i++)
      if (GetAnalysis(i)->IdName()==an)
	return GetAnalysis(i);
    return NULL;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::FindAnalysis(Analysis *a) const {
    for (size_t i=0; i<Nanalyses(); i++)
      if (GetAnalysis(i)==a)
	return true;
    return false;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::RemoveAnalysis(Analysis *a) {
    vector<Analysis*>::iterator i = analysis.begin();
    while (i!=analysis.end() && a!=*i)
      i++;
    if (i==analysis.end())
      return ShowError("RemoveAnalysis() : instance not found");
  
    analysis.erase(i);
    delete a;
    WriteLog("Analysis removed, list size now ", ToStr(Nanalyses()));
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::StartSpeechRecognizer(const string& languser) {
    string hdr = "PicSOM::StartSpeechRecognizer() : ";

    if (speech_recognizer)
      speech_recognizer->Close();
    delete speech_recognizer;

    ZeroTime(speech_zero_time);
    ZeroTime(speech_recognizer_start_time);

    string lang = languser, user;
    size_t p = lang.find('/');
    if (p!=string::npos) {
      user = lang.substr(p+1);
      lang.erase(p);
    }

    bool errnull = true;
    string cmd = UserHomeDir()+"/picsom/speech/pinview_rec.sh";

    vector<string> osrs_cmd;
    osrs_cmd.push_back(cmd);
    osrs_cmd.push_back(lang);
    if (user!="")
      osrs_cmd.push_back(user);

    speech_recognizer = CreatePipeConnection(osrs_cmd, errnull);
    if (!speech_recognizer)
      return ShowError("Invocation of the speech recognizer failed"); 

    int r = fcntl(speech_recognizer->Wfd(), F_GETFL);
    if (r==-1)
      return ShowError(hdr+"fcntl(F_GETFL) failed");

    r = r | O_NONBLOCK;

    r = fcntl(speech_recognizer->Wfd(), F_SETFL, r);
    if (r==-1)
      return ShowError(hdr+"fcntl(F_SETFL) failed");

    SetTimeNow(speech_recognizer_start_time);

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::TestSpeechRecognizer(const string& f) {
    string hdr = "PicSOM::TestSpeechRecognizer() : ";

    if (!speech_recognizer)
      return ShowError(hdr+"speech recognizer not running");

    size_t n = 0;
    while (!SpeechRecognizerAcceptsData()) {
      sleep(1);
      cout << TimeStamp() << hdr
	   << "waiting for the speech recognizer to be ready ["
	   << speech_recognizer->StateString() << "]"
	   << endl;
      n++;
      if (n>60)
	return ShowError(hdr+"waited too long");
    }

    // USAGE: play -r 16000 -2 -s f.raw

    ifstream in(f.c_str());
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
      catch (...) {
	return ShowError("Reading speech audio file <", f, "> failed");
      }

    uint32_t len = 6+data.size();

    char m_audio = 0; // M_AUDIO
    char urgent  = 0;
    string buf;
    buf.insert(0, (char*)&len, 4);
    buf.insert(4, 1, m_audio);
    buf.insert(5, 1, urgent);
    buf.insert(buf.size(), data);

    // sleep(10);

    float fsec = float(buf.size())/32000;
    //int  sec = (int)floor(1.0+fsec);

    struct timespec ts;
    SetTimeNow(ts);

    for (int i=0; i<6; i++) {
      struct timespec te = ts;
      TimeAdd(te, fsec);
      FeedSpeechRecognizer(buf, f+"-"+ToStr(i), ts, te);
      //cout << TimeStamp() << hdr << "starting to sleep for " << sec
      //     << " seconds" << endl;
      //sleep(sec);
      for (size_t l=0;; l++) {
	if (false&&l)
	  ProcessSpeechRecognizerOutput();
	cout << TimeStamp() << hdr << "recognizer state now ["
	     << speech_recognizer->StateString() << "]"
	     << endl;
	// bool b = !speech_recognizer->State(0);
	if (!speech_recognizer->State(0)) {
	  int ss = 5;
	  cout << TimeStamp() << hdr << "now sleeping for " << ss
	       << " seconds" << endl;
	  sleep(ss);
	  cout << TimeStamp() << hdr << "recognizer state now ["
	       << speech_recognizer->StateString() << "]"
	       << endl;
	}

	if (!speech_recognizer->State(0))
	  break;

	ProcessSpeechRecognizerOutput();
      }
      ts = te;

      cout << TimeStamp() << hdr << "output loop exited" << endl;
    }
    // sleep(10);
    // ProcessSpeechRecognizerOutput();

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SpeechRecognizerAcceptsData() const {
    int fixed_time = 0; // 20;  // seconds

    struct timespec t;
    ZeroTime(t);

    if (TimesEqual(t, speech_recognizer_start_time))
      return false;

    if (fixed_time) {
      SetTimeNow(t);
      TimeAdd(t, -fixed_time);
      return MoreRecent(t, speech_recognizer_start_time);
    }

    static bool was_ready = false;

    if (!was_ready)
      was_ready = speech_recognizer->State(0);

    return was_ready;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::FeedSpeechRecognizer(const string& data, const string& qid,
				    const struct timespec& stime,
				    const struct timespec& etime) {
    string hdr = "PicSOM::FeedSpeechRecognizer() : ";

    if (!speech_recognizer)
      return false;

    bool skip = !SpeechRecognizerAcceptsData(), zero = true;
    
    for (size_t i=0; zero && i<data.size(); i++)
      if (data[i]!='\0')
	zero = false;

    if (debug_osrs>1)
      cout << TimeStamp() << hdr << data.size() << " " << qid << " "
	   << TimeDotString(stime) << " - " << TimeDotString(etime)
	   << " [" << speech_recognizer->StateString() << "]"
	   << (zero?" ALL ZEROS":"")
	   << (skip?" SKIPPING":"")
	   << endl;

    if (skip)
      return true;

    uint32_t len = 6+data.size();
    char m_audio = 0; // M_AUDIO
    char urgent  = 0;
    string buf;
    buf.insert(0, (char*)&len, 4);
    buf.insert(4, 1, m_audio);
    buf.insert(5, 1, urgent);
    buf.insert(buf.size(), data);

    if (speech_zero_time.tv_sec==0 && speech_zero_time.tv_nsec==0)
      speech_zero_time = stime;

    speech_data_entry_t sd(qid, stime, etime);
    speech_data.push_back(sd);

    const char *out = buf.c_str();
    size_t dataleft = buf.size(), totalr = 0;

    while (dataleft) {
      size_t nwaits = 0;

      while (!speech_recognizer->State(1)) {
	if (nwaits==20)
	  return ShowError(hdr+"waited too long");

	cout << TimeStamp() << hdr << "waiting for pipe" << endl;
        struct timespec snap = { 0, 100*1000000 }; // 100 ms
	nanosleep(&snap, NULL);
 	nwaits++;
      }

      if (!speech_recognizer->State(1)) {
	ShowError(hdr+"recognizer ended prematurely");
	CloseConnection(speech_recognizer, true, true, true);
	speech_recognizer = NULL;
	return false;
      }

      int r = write(speech_recognizer->Wfd(), out, dataleft);
      if (r==-1) {
	ShowError(hdr+"write() errored");
	continue;
      }

      if (r!=(int)dataleft)
	cout << TimeStamp() << hdr << "write() processed partial input: "
	     << ToStr(r)+"<"+ToStr(dataleft) << endl;

      dataleft -= r;
      totalr +=r;
      out += r;
    }

    if (totalr==buf.size() && debug_osrs>2)
      cout << TimeStamp() << hdr << totalr << " bytes sent!" << endl;

    return totalr>0;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::ProcessSpeechRecognizerOutput() {
    string msg = "PicSOM::ProcessSpeechRecognizerOutput() : ";

    int debug = debug_osrs;
    
    if (!speech_recognizer || speech_data.empty())
      return false;

    if (debug)
      cout << TimeStamp() << msg << "entered" << endl;

    static map<int,string> mtype;
    if (mtype.empty()) {
      mtype[ 0] = "M_AUDIO";
      mtype[ 1] = "M_AUDIO_END";
      mtype[ 2] = "M_RESET";
      mtype[ 3] = "M_PROBS";
      mtype[ 4] = "M_PROBS_END";
      mtype[ 5] = "M_READY";
      mtype[ 6] = "M_RECOG";
      mtype[ 7] = "M_RECOG_END";
      mtype[ 8] = "M_DECODER_SETTING";
      mtype[ 9] = "M_DECODER_PAUSE";
      mtype[10] = "M_DECODER_UNPAUSE";
      mtype[11] = "M_ADAPT_ON";
      mtype[12] = "M_ADAPT_OFF";
      mtype[13] = "M_ADAPT_RESET";
      mtype[14] = "M_STATE_HISTORY";
      mtype[15] = "M_DEBUG";
      mtype[16] = "M_MESSAGE";
    }

    struct timespec ztime;
    ZeroTime(ztime);
    speech_data_entry_t new_result("", ztime, ztime);

    size_t bufsize = 1024*1024;
    char *bufall = new char[bufsize];
    ssize_t nn = read(speech_recognizer->Rfd(), bufall, bufsize);

    if (debug>2)
      cout << TimeStamp() << msg+"read " << nn << " bytes" << endl;
    if (nn<0)
      return ShowError(msg+"read() returned "+ToStr(nn));

    if (debug>4)
      for (ssize_t i=0; i<nn; i++) {
	unsigned int c = (unsigned int)bufall[i];
	cout << "  " << i << " " << c;
	if (c>=32 && c<128)
	  cout << " '" << bufall[i] << "'";
	cout << endl;
      }

    for (size_t p=0; p<(size_t)nn; ) {
      const char *buf = bufall+p;
      size_t n = nn-p;

      uint32_t len = 0;
      unsigned char type = 0;
      unsigned char urgent = 0;

      if (debug>3)
	cout << " pos=" << p << "/" << nn << " ";

      if (n>=4) {
	memcpy((char*)&len, buf, 4);
        if (debug>3)
          cout << " len=" << len;
      }
      if (n>=5) {
	type = buf[4];
	string typestr = "???";
	if (mtype.find(type)!=mtype.end())
	  typestr = mtype.find(type)->second;
        if (debug>3)
          cout << " type=" << (int)type << " " << typestr;
      }
      if (n>=6) {
	urgent = buf[5];
        if (debug>3)
          cout << " urgent=" << (int)urgent;
      }
      bool hit = false;
      if (type==6) { // M_RECOG
        string line(buf+6, len-6);
        if (debug>3 || (debug==3 && line!="")) {
          cout << " [" << line << "] ";
	  hit = true;
	}
	
	speech_str += line;
	while (speech_str[0]=='[') {
	  size_t e = speech_str.find("]");
	  if (e!=string::npos) {
	    string res = speech_str.substr(1, e-1);
	    speech_str.erase(0, e+1);
	    while (speech_str[0]==' ')
	      speech_str.erase(0, 1);

	    if (res!="") {
	      float tmul = 1.0/125;
	      istringstream ss(res);
	      int s, e;
	      string rec;
	      ss >> s >> e >> rec;
	      if (debug>2)
		cout << " <" << s << "><" << e << "><" << rec << ">";

	      struct timespec stime = speech_zero_time;
	      TimeAdd(stime, s*tmul);

	      struct timespec etime = stime;
	      if (e!=-1) {
		etime = speech_zero_time;
		TimeAdd(etime, e*tmul);
	      }

	      speech_result_entry_t sre = { s, e, stime, etime, rec, "" };
	      speech_data_entry_t *se = FindSpeechData(stime, false);
	      if (!se) {
		DumpSpeechData(NULL);
		ShowError(msg+"speech data not found "+TimeDotString(stime));

	      } else {
		if (debug>1)
                  DumpSpeechData(se);
		sre.qid = se->qid;
		se->result.push_back(sre);
		if (debug>2)
		  cout << "<" << se->qid << ">";
	      }
	      new_result.result.push_back(sre);
	    }
	  }
	}
      }
      if (debug>3 || hit)
        cout << endl;

      if (!len)
	break;

      p += len;
    }

    if (debug)
      cout << TimeStamp() << msg << "ready processing input" << endl;

    delete bufall;

    const list<speech_result_entry_t>& rl = new_result.result;

    size_t i = 0;
    for (list<speech_result_entry_t>::const_iterator p = rl.begin();
	 p!=rl.end(); p++, i++) {
	if (debug)
	  cout << "SPEECH: <" << p->qid << "> " 
	       << p->sframe << " " << p->eframe << " "
	       << TimeDotString(p->stime) << " "
	       << TimeDotString(p->etime) << " ["
	       << p->str << "]" << endl;

	Query *q = FindQueryInMemory(p->qid);
	if (q) {
	  uiart::TimeSpec stime(p->stime.tv_sec, p->stime.tv_nsec);
	  q->ProcessSpeechRecognizerResult(p->str, stime);
	}
    }

    if (debug)
      cout << TimeStamp() << msg << "exiting" << endl;

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  PicSOM::speech_data_entry_t *PicSOM::FindSpeechData(const struct timespec& t,
						      bool strict) {
    speech_data_entry_t *approx = NULL;
    double mind = numeric_limits<double>::max();
    for (list<speech_data_entry_t>::iterator i=speech_data.begin();
	 i!=speech_data.end(); i++) {
      if (TimesEqual(t, i->stime) || (MoreRecent(t, i->stime) &&
				      !MoreRecent(t, i->etime)))
	return &*i;

      double ds = fabs(TimeDiff(t, i->stime));
      double de = fabs(TimeDiff(t, i->etime));
      double d  = ds<de ? ds : de;
      if (d<mind) {
	mind = d;
	approx = &*i;
      }
    }
      
    return strict ? NULL : approx;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::DumpSpeechData(const PicSOM::speech_data_entry_t *e) {
    if (!e) {
      for (list<speech_data_entry_t>::iterator i=speech_data.begin();
	   i!=speech_data.end(); i++) {
	DumpSpeechData(&*i);
	cout << endl;
      }
      return;
    }

    cout << "(" << e->qid
	 << "," << TimeDotString(e->stime)
	 << "," << TimeDotString(e->etime)
	 << "," << e->result.size()
	 << ")";
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::ProcessSpeechRecognizerOutputV1() {
    string msg = "PicSOM::ProcessSpeechRecognizerOutput() : ";

    int debug = 1;
    
    if (!speech_recognizer)
      return false;

    static map<int,string> mtype;
    if (mtype.empty()) {
      mtype[ 0] = "M_AUDIO";
      mtype[ 1] = "M_AUDIO_END";
      mtype[ 2] = "M_RESET";
      mtype[ 3] = "M_PROBS";
      mtype[ 4] = "M_PROBS_END";
      mtype[ 5] = "M_READY";
      mtype[ 6] = "M_RECOG";
      mtype[ 7] = "M_RECOG_END";
      mtype[ 8] = "M_DECODER_SETTING";
      mtype[ 9] = "M_DECODER_PAUSE";
      mtype[10] = "M_DECODER_UNPAUSE";
      mtype[11] = "M_ADAPT_ON";
      mtype[12] = "M_ADAPT_OFF";
      mtype[13] = "M_ADAPT_RESET";
      mtype[14] = "M_STATE_HISTORY";
      mtype[15] = "M_DEBUG";
      mtype[16] = "M_MESSAGE";
    }

    size_t bufsize = 1024*1024;
    char *bufall = new char[bufsize];
    ssize_t nn = read(speech_recognizer->Rfd(), bufall, bufsize);

    if (debug)
      cout << msg+"read " << nn << " bytes" << endl;
    if (nn<0)
      return ShowError(msg+"read() returned "+ToStr(nn));

    if (debug>1)
      for (ssize_t i=0; i<nn; i++)
	cout << "  " << i << " " << (unsigned int)bufall[i] << endl;

    const char *buf = bufall;
    for (size_t p=0; p<(size_t)nn; ) {
      size_t n = nn-p;

      uint32_t len = 0;
      unsigned char type = 0;
      unsigned char urgent = 0;

      if (n>=4) {
	memcpy((char*)&len, buf, 4);
        if (debug)
          cout << " len=" << len;
      }
      if (n>=5) {
	type = buf[4];
	string typestr = "???";
	if (mtype.find(type)!=mtype.end())
	  typestr = mtype.find(type)->second;
        if (debug)
          cout << " type=" << (int)type << " " << typestr;
      }
      if (n>=6) {
	urgent = buf[5];
        if (debug)
          cout << " urgent=" << (int)urgent;
      }
      if (type==6) { // M_RECOG
        string line(buf+6, len-6);
        if (debug) 
          cout << " [" << line << "] ";

        if (line[0]!='*')
          cout << ">>> " << line << " <<<" << endl;
        
      }
      if (debug)
        cout << endl;

      if (!len)
	break;

      p   += len; 
      buf += len;
    }

    delete bufall;

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::LoadContext(const string& s) {
    string msg = string("LoadContext(")+s+") : ";

    size_t c = s.find(':');
    if (c==string::npos)
      return ShowError(msg+"':' not found");

    string dbn = s.substr(0, c), lab = s.substr(c+1);

    DataBase *db = FindDataBase(dbn);
    if (!db)
      return ShowError(msg+"database '"+dbn+"' not found");

    int idx = db->LabelIndex(lab);
    if (idx<0)
      return ShowError(msg+"label '"+lab+"' not found");

#ifdef PICSOM_USE_CONTEXTSTATE
    for (size_t i=(size_t)idx; i<db->Size(); i++)
      if (db->ObjectsTargetTypeContains(i, target_data))
        db->AddToContext(i);
#endif // PICSOM_USE_CONTEXTSTATE

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::OctaveInializeOnce() {
    static bool ret  = false;
#if defined(PICSOM_HAVE_OCTAVE) && defined(PICSOM_USE_OCTAVE)
    MutexLock();
    static bool done = false;
    if (!done) {
      static char *argv[3];
      argv[0] = strdup("picsom");
      argv[1] = strdup("-q");
      argv[2] = NULL;
      ret = octave_main(2, argv, 1);
      done = true;
      
      if (ret)
        WriteLog("Initialized octave version " OCTAVE_VERSION);

      octavebasepath.push_back("");

      string dir = Path()+"/octave";
      if (DirectoryExists(dir)) {
        octavebasepath.push_back(dir);
        OctaveAddPath(dir, true, true);
      }
      dir = UserHomeDir()+"/picsom/octave";
      if (DirectoryExists(dir)) {
        octavebasepath.push_back(dir);
        OctaveAddPath(dir, true, true);
      }
    }
    MutexUnlock();
#endif // PICSOM_HAVE_OCTAVE && PICSOM_USE_OCTAVE

    return ret ? true : ShowError("OctaveInializeOnce() failing");
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::OctaveAddPath(const string& path, bool exist, bool once) {
#if defined(PICSOM_HAVE_OCTAVE) && defined(PICSOM_USE_OCTAVE)
    for (list<string>::const_iterator i=octavebasepath.begin();
         i!=octavebasepath.end(); i++) {
      if ((*i)!="" && path.substr(0, 1)=="/")
        continue;

      string dir = (*i)+path;

      if ((!exist || DirectoryExists(dir)) &&
          (octavepath.find(dir)==octavepath.end() || !once)) {
        octave_value_list f_arg, f_ret;
        f_arg(0) = octave_value(dir);
        f_ret = feval("addpath", f_arg, 0);
        octavepath.insert(path);
        WriteLog("Added <"+path+"> in octave path");
      }
    }
    return true;
#else
    return (path.size()||exist||once)&&false;
#endif // PICSOM_HAVE_OCTAVE && PICSOM_USE_OCTAVE
  }
  
  /////////////////////////////////////////////////////////////////////////////

  size_t PicSOM::FileSystemFreeSpace(const string& f) {
    // vector<string> cmd { "/bin/df", "-B1", f, "2>&1" };
    // Connection *c = Connection::CreatePipe(this, cmd, true);
    // if (!c)
    //   return 0;

    // char buf[1024];
    // ssize_t n = 0;
    // if (c->State(0, 3) && !c->State(2)) 
    //   n = read(c->Rfd(), buf, sizeof buf);
    // delete c;

    // if (n<1)
    //   return 0;

    // buf[n] = 0;
    // string st(buf);
    // size_t p = st.find('\n');
    // if (p!=string::npos)
    //   st.erase(0, p+1);

    vector<string> cmd { "/bin/df", "-B1", f };
    auto r = ShellExecute(cmd, true, false);
    if (!r.first || r.second.size()<2)
      return 0;

    string st = r.second[1];
    vector<string> col = SplitInSpaces(st);
    if (col.size()!=6)
      return 0;

    size_t s = atol(col[3].c_str());

    return s;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::SetTempDirRoot(const string& p) {
    if (temp_dir_root_d!="") {
      // if (temp_dir_root_d!=temp_dir_root)
      //   return ShowError("Setting temporary storage too late after usage.");
      // else
	return true;
    }

    vector<string> pl = SplitInSomething(":", false, p);

    size_t min = 0;
    for (auto i=pl.begin(); i!=pl.end(); i++) {
      string s = *i;
      if (s[0]!='/') {
	min = atoi(s.c_str());
	size_t u = s.find_first_not_of("0123456789");
	if (u!=string::npos) {
	  char m = s[u];
	  if (m=='k') min *= 1024LL; 
	  if (m=='M') min *= 1024LL*1024; 
	  if (m=='G') min *= 1024LL*1024*1024;
	  if (m=='T') min *= 1024LL*1024*1024*1024;
	}
	continue;
      }
      size_t fs = 0;
      if (min) {
	fs = FileSystemFreeSpace(s);
	if (fs<min)
	  continue;
      }
      string t = s+"/"+UserName()+"-picsom-"+ToStr(getpid())
	+".test";
      bool fail = false;
      if (!StringToFile("hello world\n", t))
	fail = true;
      Unlink(t);
      if (fail)
	continue;

      string info;
      if (fs) {
	float fsf = float((10*fs)/(1024L*1024*1024))/10;
	info = " "+ToStr(fsf)+"GB";
      }
      temp_dir_root   = *i;
      temp_dir_root_d = "";
      WriteLog("Selected temporary storage <"+temp_dir_root+">"+info);

      if (temp_dir_personal!="") { // this forces to forget the original...
	if (RmDir(temp_dir_personal))
	  WriteLog("Successfully removed empty <"+temp_dir_personal+">");
	else
	  WriteLog("Something left in <"+temp_dir_personal+">?");
	temp_dir_personal = "";
      }

      return true;
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& PicSOM::TempDirRoot() {
    if (temp_dir_root_d=="") {
      if (temp_dir_root!="") {
	temp_dir_root_d = temp_dir_root;
	if (!DirectoryExists(temp_dir_root_d)) {
	  int mode = 02775;
	  MkDirHier(temp_dir_root_d, mode);
	}

      } else {
	char *envdir = getenv("TMPDIR");
	string d = (envdir && (strlen(envdir)>0) ? envdir : "/tmp");
	while (d.size() && d[d.size()-1]=='/')
	  d.erase(d.size()-1);
	temp_dir_root_d = d;
      }
    }
    return temp_dir_root_d;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::TempDirPersonal(const string& d) {
    int mode = 02775;
    if (temp_dir_personal=="") {
      temp_dir_personal
	= TempDirRoot()+"/"+UserName()+"-picsom-"+ToStr(getpid());
      string imgtmp = temp_dir_personal+"/tmp";
      MkDirHier(imgtmp, mode);
      imagefile::tmp_dir(imgtmp);
      videofile::tmp_dir(imgtmp);
    }
    if (d=="")
      return temp_dir_personal;

    string nd = temp_dir_personal+"/"+d;
    MkDirHier(nd, mode);
    
    return nd;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  list<string> PicSOM::Translate(const string& meth, const string& src,
				 const string& dst, const list<string>& txt) {
    if (meth=="yandex")
      return TranslateYandexV1(src, dst, txt);
      // ShowError("Yandex.Translate v1 discontinued");
    
    if (meth=="google")
      return TranslateGoogle(src, dst, txt);
    
    return list<string>();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  pair<string,string> PicSOM::GoogleCredentials(const string& a) {
    string msg = "PicSOM::GoogleCredentials("+a+") : ";

#if defined(HAVE_JSON_JSON_H)
    string project = GetSecret(a, true);
    if (project=="") {
      ShowError(msg+"\""+a+"\" not found");
      return {"", ""};
    }

    if (project.substr(0, 2)=="~/")
      project.replace(0, 1, UserHomeDir());

    if (!FileExists(project)) {
      ShowError(msg+"file <"+project+"> not found");
      return {"", ""};
    }
    
    Json::Value doc;
    ifstream(project) >> doc;
    string project_id = doc.get("project_id", "").asString();
    if (project_id=="") {
      ShowError(msg+"project_id not found");
      return {"", ""};
    }

    vector<string> cmd
      { "env", "GOOGLE_APPLICATION_CREDENTIALS="+project,
	"gcloud", "auth", "application-default", "print-access-token" };

    auto r = ShellExecute(cmd, false, true);
    if (!r.first || r.second.size()!=2) {
      ShowError(msg+"gcloud auth failed");
      return {"", ""};
    }

    return { project_id, r.second[0] };
    
#else
    ShowError(msg+"HAVE_JSON_JSON_H not defined");
    return {"", ""};
#endif // HAVE_JSON_JSON_H
  }
  
  /////////////////////////////////////////////////////////////////////////////

  list<string> PicSOM::TranslateGoogle(const string& src, const string& dst,
				       const list<string>& input) {
    string msg = "PicSOM::TranslateGoogle("+src+","+dst+","+ToStr(input)+") : ";

    list<string> ret;
    auto google = GoogleCredentials("google-translate-project");
    if (google.first=="") {
      ShowError(msg+"could not get google credentials");
      return ret;
    }
    
#if defined(HAVE_JSON_JSON_H)
    const string& project_id = google.first, key = google.second;

    list<pair<string,string> >
      hdrs { 
	    {"Content-Type",  "application/json; charset=utf-8"},
	    {"Authorization", "Bearer "+key},
	    {"Connection",    "close"} };

    Json::Value contents(Json::arrayValue);
    for (const auto& i : input)
      contents.append(i);
    Json::Value req;
    req["contents"] = contents;
    req["sourceLanguageCode"] = src;
    req["targetLanguageCode"] = dst;
    req["mimeType"] = "text/plain";
    
    string cntnt = Json::FastWriter().write(req);

    string url = "https://translation.googleapis.com/v3beta1/projects/"
      +project_id+"/locations/global:translateText", txt, ctype;

    if (!Connection::DownloadString(this, "POST", url, hdrs, cntnt,
				    txt, ctype)) {
      ShowError(msg+"POST <"+url+"> failed");
      return ret;
    }
    if (ctype!="application/json; charset=UTF-8") {
      ShowError(msg+"content-type <"+ctype+"> not handled");
      return ret;
    }

    // cout << txt << endl;
    Json::Value res;
    istringstream(txt) >> res;
    Json::Value translations = res["translations"];
    for (const auto& t : translations)
      ret.push_back(t["translatedText"].asString());
    
    return ret;

#else
    ShowError(msg+"HAVE_JSON_JSON_H not defined");
    return list<string>();
#endif // HAVE_JSON_JSON_H
  }
  
  /////////////////////////////////////////////////////////////////////////////

  list<string> PicSOM::TranslateYandexV1(const string& src, const string& dst,
				       const list<string>& input) {
    string msg = "PicSOM::TranslateYandexV1("+src+","+dst+") : ";

    bool debug = false;

    static set<string> dirset;

    list<string> ret, empty;
    string key = GetSecret("yandex_translate", true);
    if (key!="") {
      list<pair<string,string> > hdrs { 
	make_pair("Accept", "*/*"),
        make_pair("Content-Type", "application/x-www-form-urlencoded"),
	make_pair("Connection", "close") 
      };
      if (dirset.empty()) {
	string url = "https://translate.yandex.net/api/v1.5/tr/getLangs?key="+
	  key+"&ui=en", txt, ctype;
	if (!Connection::DownloadString(this, "POST", url, hdrs, "",
					txt, ctype)) {
	  ShowError(msg+"POST <"+url+"> failed A");
	  return empty;
	}
	if (ctype!="text/xml; charset=utf-8") {
	  ShowError(msg+"content-type <"+ctype+"> not handled A");
	  return empty;
	}
	XmlDom xml = XmlDom::FromString(txt);
	XmlDom Langs = xml.Root();
	XmlDom dirs  = Langs.FirstElementChild();
	XmlDom strng = dirs.FirstElementChild();
	while (strng) {
	  string frmto = strng.FirstChildContent();
	  if (debug)
	    cout << "Lang/dirs/string <" << frmto << ">" << endl;
	  if (frmto=="")
	    break;
	  dirset.insert(frmto);
	  strng = strng.NextElement();
	}
      }
      string dir = src+"-"+dst;
      if (dirset.find(dir)==dirset.end()) {
	ShowError(msg+"dir <"+dir+"> not handled");
	return empty;
      }
      string cntnt;
      for (auto i=input.begin(); i!=input.end(); i++)
	cntnt += string(cntnt==""?"":"&")+"text="+*i;
      string url = "https://translate.yandex.net/api/v1.5/tr/translate?key="+
      key+"&lang="+dir, txt, ctype;

      if (!Connection::DownloadString(this, "POST", url, hdrs, cntnt,
				      txt, ctype)) {
	ShowError(msg+"POST <"+url+"> failed B");
	return empty;
      }
      if (ctype!="text/xml; charset=utf-8") {
	ShowError(msg+"content-type <"+ctype+"> not handled B");
	return empty;
      }
      XmlDom xml = XmlDom::FromString(txt);
      XmlDom Translation = xml.Root(); // code="200" should be checked
      XmlDom text  = Translation.FirstElementChild();
      while (text) {
	string dsttxt = text.FirstChildContent();
	if (debug)
	  cout << "Translation/text <" << dsttxt << ">" << endl;
	if (dsttxt=="")
	  break;
	ret.push_back(dsttxt);
	text = text.NextElement();
      }
    }

    return ret;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool PicSOM::ReadSecrets() {
    bool debug = false;

    secrets.clear();
    string s = FileToString(Path()+"/secrets.txt")+"\n"+
      FileToString(UserHomeDir()+"/picsom/secrets.txt");

    vector<string> l =  SplitInSomething("\n", true, s);
    for (auto i=l.begin(); i!=l.end(); i++) {
      string s = *i;
      size_t p = s.find('#');
      if (p!=string::npos)
	s.erase(p);
      if (s.find('=')!=string::npos) {
	pair<string,string> kv = SplitKeyEqualValueNew(s);
	secrets[kv.first] = kv.second;
	if (debug)
	  cout << "Added secret <" << kv.first << ">=<" << kv.second << ">"
	       << endl;
      }
    }

    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::GetSecret(const string& k, bool angry) const {
    if (secrets.find(k)!=secrets.end())
      return secrets.at(k);

    if (angry)
      ShowError("Secret for key <"+k+"> not found");

    return "";
  }
  
  /////////////////////////////////////////////////////////////////////////////

  string PicSOM::UnZip(const string& z, const string& s, const string& d) {
    string msg = "Unzip("+z+","+s+","+d+") : ";
    
#ifdef HAVE_ZIP_H
    size_t bufsize = 100*1024*1024;
    string buf(bufsize, ' ');

    int flags = 0;
    int ierr = 0;
    zip_t *x = zip_open(z.c_str(), flags, &ierr);
    if (!x) {
      ShowError(msg+"zip_open() failed");
      return "";
    }
    string ss = s;
    if (ss=="") {
      const char *fn = zip_get_name(x, 0, 0);
      if (!fn) {
        ShowError(msg+"zip_get_name() failed");
        return "";
      }
      ss = fn;
    }

    static size_t nn = 0;
    string dd = d;
    if (dd=="")
      dd = TempDirPersonal()+"/unzip-files/"+ToStr(nn++)+"/"+ss;
    MakeDirectory(dd, true);

    WriteLog(msg+"extracting to <"+dd+">");

    ofstream of(dd);

    zip_file_t *f = zip_fopen(x, ss.c_str(), 0);

    for (;;) {
      int n = zip_fread(f, &buf[0], bufsize);
      if (n>0)
	of.write(&buf[0], n);
      if (n<=0)
	break;
    }

    zip_fclose(f);
    zip_close(x);

    return dd;

#else
    ShowError(msg+"ZIP not available");
    return "";
#endif // HAVE_ZIP_H
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void PicSOM::TestPyGILState(const string& m) const {
#ifdef PICSOM_USE_PYTHON
    WriteLog("PicSOM::TestPyGILState() "+m+" a");
    PyGILState_STATE gstate = PyGILState_Ensure();
    WriteLog("PicSOM::TestPyGILState() "+m+" b");
    PyGILState_Release(gstate);
    WriteLog("PicSOM::TestPyGILState() "+m+" c");
#else
    WriteLog("PicSOM::TestPyGILState() "+m+" PICSOM_USE_PYTHON not defined");
#endif // PICSOM_USE_PYTHON
  }
  
  /////////////////////////////////////////////////////////////////////////////

  // bool PicSOM::() {
  // 
  // }
  
// class PicSOM ends here

  /////////////////////////////////////////////////////////////////////////////

  string video_frange::value() const {
    stringstream ss;
    ss << (db?db->Label(idx):"nil-db#"+ToStr((int)idx))
       << " " << begin << " " << end << " " << begin_t << " " << end_t
       << (type!=""?" "+type:"");
    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string video_frange::dashstr() const {
    stringstream ss;
    if (end>begin)
      ss << db->Label(idx) << ":" << begin << "-" << end-1;
    else
      ss << db->Label(idx) << ":empty";
    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  dtw_spec dtw_spec::parse(const string& ssin) {
    string err = "dtw_spec::parse() : ", ssx = ssin;

    dtw_spec dtw, empty;
    vector<float> wind_r, wind_c;


    size_t p = ssx.find("!");
    if (p!=string::npos) {
      dtw.dtwparam=ssx.substr(p+1);
      ssx.erase(p);
    }


    p = ssx.find("||");
    if (p!=string::npos) {
      vector<string> vy = SplitInSomething("/", false, ssx.substr(p+2));
      for (auto k=vy.begin(); k!=vy.end(); k++) {
	vector<string> vs = SplitInCommas(*k);
	for (auto j=vs.begin(); j!=vs.end(); j++)
	  (k==vy.begin()?wind_r:wind_c).push_back(atof(j->c_str()));
      }
      ssx.erase(p);
    }

    dtw.wind = make_pair(wind_r, wind_c);

    vector<string> ssv = SplitInSomething("|", false, ssx);
    for (auto ia=ssv.begin(); ia!=ssv.end(); ia++) {
     dtw_spec::flist_t l;

     vector<string> ssz = SplitInSomething("/", false, *ia);
      
      for (auto ib=ssz.begin(); ib!=ssz.end(); ib++) {
	vector<string> vs = SplitInCommas(*ib);
	if (vs.size()!=4) {
	  cerr << err+"\""+*ib+"\" should have 4 components" << endl;
	  return empty;
	}

	map<size_t,float> m;
	vector<string> vc = SplitInSomething("+", false, vs[3]);
	for (auto j=vc.begin(); j!=vc.end(); j++) {
	  string s = *j;
	  size_t d = s.find('-');
	  size_t c = s.find(':');
	  if (d==string::npos || c==string::npos || c<d) {
	    cerr << err+"failed to process \""+s+"\"" << endl;
	    return empty;
	  }
	  int a = atoi(s.substr(0, d).c_str());
	  int b = atoi(s.substr(d+1, c-d-1).c_str());
	  float v = atof(s.substr(c+1).c_str());
	  for (size_t t=(size_t)a; t<=(size_t)b; t++)
	    m[t] = v;
	}

	dtw_spec::fea_t e(vs[0], atof(vs[1].c_str()), vs[2], m); 
	l.push_back(e);
      }

      dtw.s.push_back(l);
    }

    return dtw;
  }

  /////////////////////////////////////////////////////////////////////////////

  string dtw_spec::fea_t::str() const {
    stringstream css;
    css << name << "," << wght << "," << dist << ",";

    size_t p = string::npos, a = p;
    float nv = -12345, av = nv;
    bool first = true;
    for (auto k=comp.begin(); k!=comp.end(); k++) {
      if (k->first!=p+1 || k->second!=av) {
	if (av!=nv) {
	  css << (first?"":"+") << a << "-" << p << ":" << av;
	  first = false;
	}
	p = string::npos;
      }
      if (p==string::npos) {
	a  = k->first;
	av = k->second;
      }
      p = k->first;
    }
    if (p!=string::npos)
      css << a << "-" << p << ":" << av;

    return css.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string dtw_spec::str(const flist_t& e) {
    stringstream ss;
    for (auto j=e.begin(); j!=e.end(); j++)
      ss << (j!=e.begin()?"/":"") << j->str();

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string dtw_spec::str() const {
    stringstream ss;
    for (auto i=s.begin(); i!=s.end(); i++)
      ss << (i!=s.begin()?"|":"") << str(*i);

    if (wind.first.size()||wind.second.size()) {
      ss << "||";
      for (size_t i=0; i<wind.first.size(); i++)
	ss << (i?",":"") << wind.first[i];
      ss << "/";
      for (size_t i=0; i<wind.second.size(); i++)
	ss << (i?",":"") << wind.second[i];
    }

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  string textline_t::str_common_x(bool multi, bool full, bool time,
				  bool val, bool enc) const {
    stringstream ss;
    
    if (full) {
      ss << "#" << idx
	 << (db?" "+db->Label(idx):"")
	 << " " << (db?db->Name():"no db") << " ";

      if (index!="")
	ss << "index=[" << index << "] ";
      if (field!="")
	ss << "field=[" << field << "] ";
      if (generator!="")
	ss << "generator=[" << generator << "] ";
      if (evaluator!="")
	ss << "evaluator=[" << evaluator << "] ";
    }

    if (time && is_time_set())
      ss << start << " " << end << " ";

    size_t j = 0;
    for (auto& i : txt_val_box) {
      if (!multi && j)
	break;
      
      if (j++)
	ss << " # ";
      string t = i.t;
      if (enc) {
	size_t p = 0;
	for (;;) {
	  p = t.find('#', p);
	  if (p==string::npos)
	    break;
	  t.insert(p, "#");
	  p += 2;
	}
      }
      ss << t;
      if (val) {
	if (i.is_val_set())
	  ss << " (" << i.v << ")";
	if (i.is_box_set())
	  ss << " [" << i.x << " " << i.y << " " << i.h << " " << i.w << "]";
      }
    }
	  
    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool textline_t::txt_decode(const string& sin) {
    string msg = "textline_t::txt_decode("+sin+") : ", s = sin;

    *this = textline_t();

    size_t p = s.find('\n');
    if (p!=string::npos && p!=s.size()-1)
      return ShowError(msg+"only \\n not the last character");

    if (p!=string::npos)
      s.erase(p);

    p = s.find_first_not_of("0123456789.: \t");
    if (p!=0 && s.substr(0, 2)!=" (") {
      size_t q = s.find_first_of(" \t");
      if (q==0 || q==string::npos)
	return ShowError(msg+"error #1");
      if (q<=p) {
	size_t r = s.find_first_not_of(" \t", q);
	if (r==string::npos)
	  return ShowError(msg+"error #2");

	size_t t = s.find_first_of(" \t", r);
	if (t!=string::npos && t<=p) {      
	  start = TimeStrToSec(s.substr(0, q));
	  end   = TimeStrToSec(s.substr(r, t-r));
	  s.erase(0, p);
	}
      }
    }
    
    vector<string> m = SplitOnWord(" # ", s);
    for (auto i : m ) {
      double v = empty_v, x = 0, y = 0, w = 0, h = 0;
      string t = i, b = i;
      size_t p = t.find(" (");
      if (p!=string::npos) {
	char *endptr = NULL;
	errno = 0;
	string ss = t.substr(p+2);
	double d = strtod(ss.c_str(), &endptr);
	if (!errno && endptr && *endptr==')') {
	  v = d;
	  t.erase(p);
	}
      }
      p = b.find(" [");
      if (p!=string::npos && b[b.size()-1]==']') {
	int r = sscanf(b.substr(p+2).c_str(), "%lg %lg %lg %lg",
		       &x, &y, &w, &h);
	if (r!=4)
	  x = y = w = h = 0;
      }

      p = 0;
      for (;;) {
	p = t.find("##", p);
	if (p==string::npos)
	  break;
	t.erase(p, 1);
	p += 1;
      }
      
      add(t, v, x, y, w, h);
      // txt_val.push_back(make_pair(a, val));
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  boxdata_t::boxdata_t(const DataBase *d, const vector<string>& a) {
    string msg = "boxdata_t::() : ";
    db = d;
    if (a.size())
      idx = db->LabelIndexGentle(a[0], false);
    if (a.size()>1)
      segm = a[1];
    if (a.size()>2)
      recog = a[2];
    if (a.size()>6) {
      tl_x = atof(a[3].c_str());
      tl_y = atof(a[4].c_str());
      br_x = atof(a[5].c_str());
      br_y = atof(a[6].c_str());
    }
    if (a.size()>7)
      val = atof(a[7].c_str());
    if (a.size()>8)
      type = a[8];

    if (a.size()>9) {
      vector<string> b = a;
      for (size_t i=0; i<9; i++)
	b.erase(b.begin());
      txt = JoinWithString(b, " ");
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  string boxdata_t::str() const {
    stringstream ss;
    ss << "[" << db->Name() << " #" << idx << " <" << db->Label(idx)
       << "> " << segm << "/" << recog << " [" << tl_x << "," << tl_y
       << " " << br_x << "," << br_y << "] " << val << " (" << type
       << ") \"" << txt << "\"";
    return ss.str();
  }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
This is how to use malloc_ss in IRIX (man malloc_ss):

make MALLOCLIB=-lmalloc_ss debug

setenv _SPEEDSHOP_VERBOSE
setenv _SSMALLOC_FASTCHK
setenv _SSMALLOC_FULLWARN
setenv _SSMALLOC_CLEAR_FREE
setenv _SSMALLOC_CLEAR_MALLOC

**/

/**
This is how to use Third Degree in ALPHA (man third):

third [-invalid -uninit heap+stack -free 10000000000] [-pthread] ./picsom.alpha....

**/

/**
This is how to use Electric Fence in LINUX (man efence):

setenv LD_PRELOAD /share/imagedb/picsom/linux64/lib/libefence.so
setenv EF_PROTECT_BELOW 1
setenv EF_PROTECT_FREE 1
setenv EF_FILL 42
setenv EF_ALLOW_MALLOC_0 1

or in gdb

set env LD_PRELOAD /share/imagedb/picsom/linux64/lib/libefence.so
set env EF_PROTECT_BELOW 1
set env EF_PROTECT_FREE 1
set env EF_FILL 42
set env EF_ALLOW_MALLOC_0 1
*/

/**
This is how to use NJAMD in LINUX (man njamd):

setenv LD_PRELOAD libnjamd.so
setenv NJAMD_PROT strict
setenv NJAMD_CHK_FREE error
setenv NJAMD_DUMP_LEAKS_ON_EXIT num
setenv NJAMD_DUMP_STATS_ON_EXIT 1
setenv NJAMD_DUMP_CORE soft
setenv NJAMD_ALLOW_READ 1

or in gdb

set env LD_PRELOAD libnjamd.so
set env NJAMD_PROT strict
set env NJAMD_CHK_FREE error
set env NJAMD_DUMP_LEAKS_ON_EXIT num
set env NJAMD_DUMP_STATS_ON_EXIT 1
set env NJAMD_DUMP_CORE soft
set env NJAMD_ALLOW_READ 1

In 64-bin LINUX you'll also need (, but it doesn't work anyway...)
setenv  LD_LIBRARY_PATH /share/imagedb/picsom/linux64/lib
set env LD_LIBRARY_PATH /share/imagedb/picsom/linux64/lib

*/

/* and finally, some useful valgrind options:

%> valgrind -v --leak-check=full --show-reachable=yes picsom ...

*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
