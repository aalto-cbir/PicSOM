// -*- C++ -*-  $Id: External.C,v 1.46 2020/03/06 13:18:46 jormal Exp $
// 
// Copyright 1998-2020 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <External.h>
#include <sys/resource.h>
#include <sys/utsname.h>

#include <dlfcn.h>

//#include <imagescaling.h>

namespace picsom {
  // default rlimit sizes, RLIM_INFINITY is interpreted as "leave as is"
  rlim_t External::rlimit_core = 0;
  rlim_t External::rlimit_cpu  = RLIM_INFINITY;
  rlim_t External::rlimit_as   = RLIM_INFINITY;

//=============================================================================

  void *External::OpenDL(const string& name) const {
    // fixme: determine path in some smarter way?
    string arch = SolveArchitecture();
    string so = "/share/imagedb/picsom/"+arch+"/lib/"+name+".so";
    
    return dlopen(so.c_str(), RTLD_LAZY);
  }

//=============================================================================
  
  string External::DLError() const {
    char s[100];
    sprintf(s, "%s", dlerror());
    return string(s);
  }
 
//=============================================================================

  bool External::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "External::ProcessOptionsAndRemove() ";
  
    if (FileVerbose())
      cout << TimeStamp() << msg << "name=" << Name() << endl;
  
    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << TimeStamp() << "  <" << *i << ">" << endl;
    
      string key, value;
      SplitOptionString(*i, key, value); 
    
      if (key=="coresize") {
	rlimit_core = atoi(value.c_str());
	i = l.erase(i);
	continue;
      }

      if (key=="cputime") {
	rlimit_cpu = atoi(value.c_str());
	i = l.erase(i);
	continue;
      }

      if (key=="memsize") {
	rlimit_as = atoi(value.c_str());
	i = l.erase(i);
	continue;
      }

      i++;
    }
    bool ok = Feature::ProcessOptionsAndRemove(l);
    return ok;
  }

  //===========================================================================

  vector<string> External::UsedDataLabelsBasic() const {
    vector<pair<int,string> > labs;
    vector<pair<int,string> >::iterator i;

    if (IsImageFeature())
      labs = SegmentData()->getUsedLabelsWithText();
    else 
      labs.push_back(make_pair(0,""));
    
    if (LabelVerbose()) {
      cout << TimeStamp() << "External::UsedDataLabelsBasic() : " << endl;
      for (i=labs.begin(); i<labs.end(); i++)
        cout << TimeStamp() << "  " << i->first << " : \"" << i->second << "\"" << endl;
    }
    
    if (!UseBackground()) {
      for (i=labs.begin(); i<labs.end(); )
        if (i->second=="background")
          labs.erase(i);
        else
          i++;
    }
    
    vector<string> ret;
    for (i=labs.begin(); i<labs.end(); i++) {
      char tmp[100];
      sprintf(tmp, "%d", i->first);
      ret.push_back(tmp);
    }
    
    if (LabelVerbose())
      ShowDataLabels("External::UsedDataLabelsBasic", ret, cout);
    
    return ret;
  }

//=============================================================================

  bool External::CalculateOneFrame(int f) {
    char msg[100];
    sprintf(msg, "External::CalculateOneFrame(%d)", f);
  
    if (FrameVerbose())
      cout << TimeStamp() << msg << " : starting" << endl;
  
    bool ok = true;
  
    int n = DataElementCount();
    for (int i=0; ok && i<n; i++)
      if (!CalculateOneLabel(f, i))
	ok = false;
  
    if (FrameVerbose())
      cout << TimeStamp() << msg << " : ending" << endl;
  
    return ok;
  }  

///////////////////////////////////////////////////////////////////////////////

  string External::SolveArchitecture() const {
    struct utsname uts;
    if (uname(&uts) == -1) {
      cerr << TimeStamp() << "External::SolveArchitecture() : uname() call failed" << endl;
      return ""; 
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
    return uts.sysname;
  }

  /////////////////////////////////////////////////////////////////////////////

  string External::FindExecutable(const string& n) const {
    passwd *pw  = getpwuid(getuid());
    string home = pw ? pw->pw_dir : "";
    string homex = home+"/picsom/bin/"+n;
    if (home!="" && FileExists(homex))
      return homex;

    homex = home+"/picsom/"+n;
    if (home!="" && FileExists(homex))
      return homex;
    
    return "/share/imagedb/picsom/linux/bin/"+n;
  }

  //===========================================================================

  string External::RemoteShell() const {
    bool use_ssh = true;
    string arch = SolveArchitecture();

    if (arch == "irix")
      return use_ssh ? "/bin/ssh" : "/usr/bsd/rsh";
    if (arch == "linux" || arch == "linux64")
      return use_ssh ? "/usr/bin/ssh" : "/usr/bin/rsh";
    if (arch == "alpha")
      return use_ssh ? "/bin/ssh" : "/bin/rsh";

    // fallback
    return "/usr/bin/rsh";
  }

  //===========================================================================

  bool External::EnsureFileList(vector<string> files) const {
    for (vector<string>::const_iterator f=files.begin(); f!=files.end(); f++) 
      if (!WaitForFile(*f))
	return false;
    return true;
  }

  //===========================================================================

  string External::ReadBytes(int out, fd_set *fdsp, const string& name) {
    string msg = "External::ReadBytes() : ";

    string ret;
    struct stat statbuf;
    int statres = fstat(out, &statbuf);

    if (FD_ISSET(out, fdsp) && !statres) {
      const size_t bufsize = 1024*1024*4;
      char *buf = new char[bufsize];
      for (;;) {
        int read_bytes = read(out, buf, bufsize);
        ret += string(buf, read_bytes);
        
        if (read_bytes!=(int)bufsize)
          break;
      }
      delete [] buf;
    }

    if (PixelVerbose()) 
      cout << TimeStamp() << msg << name << " read " << ret.size()
	   << " bytes: \"" << ret << "\"" << endl;

    return ret;
  }

  //===========================================================================

  bool External::PreProcess(const string& ot, imagedata& imgd,
			    const pp_chain_t& ppm,
			    const string& ppname, int f) const {
    if (!Feature::PreProcess(ot, imgd, ppm, ppname, f))
      return false;

    if (ppname!="")
      ShowError("External::PreProcess() might not work...");
      
    bool needs = NeedsConversion();

    if (FrameVerbose())
      cout << TimeStamp() << "External::PreProcess(" << f << ") needs conversion = "
	   << (needs?"true":"false") << endl;

    if (needs) {
      pair<bool,string> tmpfn = ((External*)this)->TempFile();
      string tmpbase = tmpfn.second;

      if (!tmpfn.first) {
        cerr << TimeStamp() << "ERROR: failed to create temporary file <"+tmpbase+">" 
             << endl;
        return false;
      }

      string imagepath = tmpbase+".png";
      ((External*)this)->SetIntermediateFilename(imagepath);

      EnsureImage();
      segmentfile *sf = SegmentData();
      sf->setCurrentFrame(f);

      pair<size_t,size_t> maxs = MaximumSize();
      int maxw = maxs.first, maxh = maxs.second;
      int w0 = sf->getWidth(), w1 = w0, h0 =  sf->getHeight(), h1 = h0;

      if (maxw==0 || maxh==0 || (w0<=maxw && h0<=maxh))
	sf->writeImageFrame(imagepath.c_str(), f, "image/png");
      else {
	imagedata img = *sf->accessFrame();
	float dw = float(w0)/maxw, dh = float(h0)/maxh;
	float d = dw>dh ? dw : dh;
	int di = (int)ceil(d);
	scalinginfo sca(w0, h0, w0/di, h0/di);
	img.rescale(sca);
	imagefile::write(img, imagepath, "image/png");
	w1 = img.width();
	h1 = img.height();
      }

      ((External*)this)->Unlink(tmpbase);
      
      if (FrameVerbose())
        cout << TimeStamp() << "External::PreProcess() : "
             << "converted <" << ObjectDataFileName() << "> frame "
	     << f << " (" << w0 << "x" << h0 << ") to <" << imagepath
	     << "> (" << w1 << "x" << h1 << ") CurrentFullDataFileName()="
	     <<  ((External*)this)->CurrentFullDataFileName() << endl;
    }

    return true;
  }

  //===========================================================================

  bool External::CalculateCommon(int f, bool all, int l) {
    list<string> interm = IntermediateFiles(f);
    if (!interm.empty())
      return ProcessIntermediateFiles(interm, f, all, l);

    bool ok = true;
    SetChildLimits();
    ExecPreProcess(f, all, l);
    ok = ExecRun(f, all, l);
    ExecPostProcess(f, all, l);

    return ok;
  }

  //===========================================================================

  bool External::ExecRun(int f, bool all, int l = -1) {
    bool show_waiting = false;

    string msg = "External::ExecRun() : ";

    execchain_t execchain = GetExecChain(f, all, l);
    if (execchain.empty())
      return false;

    vector<string> remo = RemoteExecution();
    if (!remo.empty() && FrameVerbose()) {
      cout << TimeStamp() << msg+"remote execution:";
      for (vector<string>::const_iterator i=remo.begin(); i!=remo.end(); i++)
        cout << " " << *i;
      cout << endl;
    }
    
    int in[2], out[2], err[2];

    if (pipe(in) || pipe(out) || pipe(err)) {
      cerr << TimeStamp() << msg+"ERROR failed with pipe()." << endl;
      return false;
    }

    // sync();

    pid_t child = fork();

    if (child && FrameVerbose())
      cout << TimeStamp() << msg+"fork() returned child pid " << child
	   << " this parent pid is " << getpid() << endl;

    if (!remo.empty()) {
      SetEnsurePre();
      SetEnsurePost();
      if (!EnsureFilesPre()) {
        cerr << TimeStamp() << msg+"ERROR some remote files were not updated." << endl;
        return false;
      }
    }

    if (!child) {
      RunExecChain(in[0], out[1], err[1], execchain, remo);
      cerr << TimeStamp() << msg+"returned from RunExecChain() though should not" << endl;
      abort();
    }

    bool ok = true, exited = false;

    string instr = GetInput();
    if (instr!="") {
      int nw = write(in[1], instr.c_str(), instr.size());
      if (nw<1) {
        cout << TimeStamp() << msg+"ERROR in write()" << endl;
        ok = false;
      }
      else if (FrameVerbose())
        cout << TimeStamp() << msg+"wrote " << nw << " bytes in stream" << endl;
    }

    struct timeval no_delay = { 0, 0 }, short_delay = { 0, 50000 };

    string out_str, err_str;
    int maxfd = out[0]>err[0] ? out[0] : err[0]; //largest fd

    while (ok) {
      fd_set rfds, efds;

      // Listen 'out' and 'err' pipes for additional input
      FD_ZERO(&rfds);
      FD_SET(out[0], &rfds);
      FD_SET(err[0], &rfds);

      // CHeck for errors in them too!
      FD_ZERO(&efds);
      FD_SET(out[0], &efds);
      FD_SET(err[0], &efds);
       
      // Wait up to one/ten seconds
      struct timeval tv = exited ? no_delay : short_delay;
      int retval = select(maxfd+1, &rfds, NULL, &efds, &tv);
    
      // Check for waiting data
      if (retval > 0) {
        out_str += ReadBytes(out[0], &rfds, "stdout");
        err_str += ReadBytes(err[0], &rfds, "stderr");
      
        if (FD_ISSET(out[0], &efds) || FD_ISSET(err[0], &efds)) {
          if (FrameVerbose())
            cout << TimeStamp() << msg+"ERROR occurred in child: [" << strerror(errno) << "]";
          break;
        }

      } else if (FrameVerbose()) {
        if (retval < 0)
          cout << TimeStamp() << msg+"select(): error.";
        else if (show_waiting && retval == 0)
          cout << TimeStamp() << msg+"select(): waiting...";
      }

      if (exited)
        break;

      // Wait (but don't block!) for the child to exit
      int status;
      pid_t pid = waitpid(child, &status, WNOHANG); // WUNTRACED = if stopped
    
      ShowChildStatus(msg, pid, status);

      if (pid && WIFEXITED(status))
        exited = true;

      if (pid && WIFSIGNALED(status))
        ok = false;

    } // while
  
    close(in[0]);
    close(in[1]);
    close(out[0]);
    close(out[1]);
    close(err[0]);
    close(err[1]);

    if (!EnsureFilesPost()) {
      cerr << TimeStamp()
	   << msg+"ERROR some remote files were not updated." << endl;
      return false;
    }
  
    if (out_str!="") {
      if (SaveOutput())
	StoreOutput(out_str);
      if (FrameVerbose() || !ok)
        cout << TimeStamp()
	     << msg+"read " << out_str.size() << " bytes of output: [" 
	     << out_str << "]" << endl;
    }

    if (err_str!="") {
      if (SaveError())
	StoreError(err_str);
      if (FrameVerbose() || !ok)
        cout << TimeStamp()
	     << msg+"read " << err_str.size() << " bytes of error output: ["
             << err_str << "]" << endl;
    }

    return ok;
  }

  //===========================================================================

  void External::ShowChildStatus(const string& msg,
                                 pid_t pid, int status) const {
    if (pid && WIFEXITED(status) && FrameVerbose())
      cout << TimeStamp() << msg+"waitpid() on child " << pid
           << " returned " << status << ", exit code was "
           << WEXITSTATUS(status) << endl;

    if (pid && WIFSIGNALED(status))
      cerr << TimeStamp() << msg+"waitpid() on child " << pid
           << " ended due child terminated by signal "
           << WTERMSIG(status) << endl;
  }

  //===========================================================================

  void External::RunExecChain(int in, int out, int err, 
                              const execchain_t& execchain,
                              const vector<string>& remote) {
    string msg = "External::RunExecChain() : ";

    if (execchain.empty()) {
      cerr << TimeStamp() << msg+"Error: No exec chain is empty!!" << endl;
      return;
    }
    if (FrameVerbose())
      cout << TimeStamp() << msg+"child running " << getpid() << endl;

    if (execchain.size()==1) {
      RunExecProcess(in, out, err, *execchain.begin(), remote);
      cerr << TimeStamp() << msg+"Error: RunExecProcess() returned A" << endl;
      return;
    }

    for (execchain_t::const_iterator i=execchain.begin();
         i!=execchain.end(); i++) {
      pid_t child = fork();

      if (child && FrameVerbose())
        cout << TimeStamp() << msg+"fork() returned " << child << endl;

      if (!child) {
        RunExecProcess(in, out, err, *i, remote);
        cerr << TimeStamp() << msg+"Error: RunExecProcess() returned B" << endl;
        return;
      }

      int status;
      pid_t pid = waitpid(child, &status, 0);

      string msg2 = msg + "<"+Label(-1, -1, "")+"> ";
      for (vector<string>::const_iterator ai=i->argv.begin();
           ai!=i->argv.end(); ai++)
        msg2 += "["+*ai+"]";

      ShowChildStatus(msg2+" : ", pid, status);
    }

    // this is a tric to prevent PicSOM's "process called exit()" messages
    const char *bintrue = "/bin/true";
    execl(bintrue, bintrue, NULL);
    // exit(0);
  }

  //===========================================================================

  void External::RunExecProcess(int in, int out, int err, 
                                const execspec_t& execspec,
                                const vector<string>& remote) {
    string msg = "External::RunExecProcess() : ";

    vector<string> argv = execspec.argv;

    if (argv.empty()) {
      cerr << TimeStamp() << msg+"Error: argv is empty!!" << endl;
      return;
    }
    if (FrameVerbose())
      cout << TimeStamp() << msg+"child running " << getpid() << endl;

    string allarg;

    if (!remote.empty()) {
      if (!execspec.env.empty()) {
        allarg = "env ";
        for (execenv_t::const_reverse_iterator i=execspec.env.rbegin();
             i!=execspec.env.rend(); i++) {
          string kv = i->first+"="+i->second;
          argv.insert(argv.begin(), kv);
          allarg += kv;
        }
        argv.insert(argv.begin(), "env");
      }
      argv.insert(argv.begin(), remote.begin(), remote.end());

    } else {
      if (!execspec.env.empty())
        allarg = "env ";
      for (execenv_t::const_iterator i = execspec.env.begin();
           i!=execspec.env.end(); i++) {
        setenv(i->first.c_str(), i->second.c_str(), 1);
        string kv = i->first+"="+i->second;
        allarg += kv+" ";
        if (FrameVerbose())
          cout << TimeStamp() << msg+"execenv: " << kv << endl;
      }
    }

    typedef char *charptr;
    charptr *a = new charptr[argv.size()+1];
    a[argv.size()] = NULL;
    for (size_t i=0; i<argv.size(); i++) {
      a[i] = (charptr)argv[i].c_str();
      if (FrameVerbose()) {
        cout << TimeStamp() << msg+"execparam[" << i << "]: [" << a[i] << "]" << endl;
        allarg += string(allarg.empty()?"":" ")+argv[i];
      }
    }
    if (FrameVerbose())
      cout << TimeStamp() << msg+"execline: "+allarg << endl;
    
    if (execspec.wd!="") {
      int r = chdir(execspec.wd.c_str());
      if (r)
	 cerr << TimeStamp() << msg+"Error: chdir to <"+execspec.wd+"> failed" << endl;
      else if (FrameVerbose())
	cout << TimeStamp() << msg+"Changed to working directory <"+execspec.wd+">"
	     << endl;
    }

    dup2(in,  0);
    dup2(out, 1);
    dup2(err, 2);

    execv(a[0], a);
  } 

  //===========================================================================

  bool External::SetChildLimits() {
    //  rlim_t mega = 1024L*1024L;
    // maximum core size in bytes
    //  rlim_t rlimit_core = coresize, rlimit_cpu = 3600, rlimit_as = 2*1024*mega*/;
    rlimit r, m;
  
    char tmp[300];
    string msg = string("External::SetChildLimits: Restricted child process to ");

    errno = 0;

    if (rlimit_core != RLIM_INFINITY) {
      r.rlim_cur = r.rlim_max = rlimit_core;
      if (setrlimit(RLIMIT_CORE, &r)) {
	int err = errno;
	getrlimit(RLIMIT_CORE, &m);
	sprintf(tmp, "%s %ld > %ld", strerror(err), r.rlim_cur, m.rlim_max);
	cerr << TimeStamp() << "External::SetChildLimits() failed with RLIMIT_CORE " << tmp << endl;
	return false;
      }
      if (FrameVerbose()) {
	sprintf(tmp,"core=%lu bytes",rlimit_core);
	cout << TimeStamp() << msg << tmp << endl;
      }
    }

    if (rlimit_cpu != RLIM_INFINITY) {
      r.rlim_cur = r.rlim_max = rlimit_cpu;
      if (setrlimit(RLIMIT_CPU, &r)) {
	int err = errno;
	getrlimit(RLIMIT_CPU, &m);
	sprintf(tmp, "%s %ld > %ld", strerror(err), r.rlim_cur, m.rlim_max);
	cerr << TimeStamp() << "External::SetChildLimits() failed with RLIMIT_CPU " << tmp << endl;
	return false;
      }
      if (FrameVerbose()) {
	sprintf(tmp,"cpu=%lu s",rlimit_cpu);
	cout << TimeStamp() << msg << tmp << endl;
      }
    }

  
    if (rlimit_as != RLIM_INFINITY) {
      r.rlim_cur = r.rlim_max = rlimit_as;
      if (setrlimit(RLIMIT_AS, &r)) {
	int err = errno;
	getrlimit(RLIMIT_AS, &m);
	sprintf(tmp, "%s %ld > %ld", strerror(err), r.rlim_cur, m.rlim_max);
	cerr << TimeStamp() << "External::SetChildLimits() failed with RLIMIT_AS " <<  tmp << endl;
	return false;
      }
      if (FrameVerbose()) {
	sprintf(tmp,"as=%lu bytes",rlimit_as);
	cout << TimeStamp() << msg << tmp << endl;
      }
    }

    /*  if (FrameVerbose()) {
	sprintf(tmp, "core = %lu bytes, cpu = %lu s, as = %lu bytes",
	rlimit_core, rlimit_cpu, rlimit_as);
	cout << TimeStamp() << "External::SetChildLimits: Restricted child process to " << tmp 
	<< endl; 
	}*/

    return true;
  }

  //===========================================================================

  int External::FileSize(const string& filename) const {
    int size = -1;
    struct stat buf;

    if (stat(filename.c_str(),&buf) == 0) 
      size = (int)buf.st_size;

    if (FrameVerbose())
      cout << TimeStamp() << "External::FileSize() returns " << size << " bytes" << endl;

    return size;
  }

  //===========================================================================

  bool External::WaitForFile(const string& filename) const {
    if (FrameVerbose())
      cout << TimeStamp() << "External::WaitForFile: ensuring file '" << filename << "'."<<endl;
    int max_wait=10;
    for (int i=0; i<max_wait; i++) {
      if (FileSize(filename) > 0)
	return true;
      if (FrameVerbose())
	cout << TimeStamp() << "External::WaitForFile: Waiting...[" << filename  << "]" << endl;
      sleep(1);
    }
    return false;
  }

} // namespace picsom

