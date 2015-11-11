// -*- C++ -*- $Id: External.h,v 1.51 2014/02/03 09:21:16 jorma Exp $

/**
   \file External.h

   \brief Declarations and definitions of class External
   
   External.h contains declarations and definitions of class the
   External, which is a base class for feature calculations that
   use an external program to perform the feature extraction.
  
   \author Mats Sjöberg <mats.sjoberg@hut.fi>
   $Revision: 1.51 $
   $Date: 2014/02/03 09:21:16 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _External_h_
#define _External_h_

#include <Feature.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif // HAVE_SYS_RESOURCE_H

namespace picsom {
#if defined(__alpha)
extern "C" int setenv(const char*, const char*, int);
#endif

#if defined(sgi)
static int setenv(const char *n, const char *v, int w) {
  std::string l = std::string(n) + "=" + v;
  if (w || !getenv(n))
    return putenv(strdup(l.c_str()));
  return 0;
}
#endif

/// Base class for external features
class External : public Feature {
public:
  ///
  typedef vector<string> execargv_t;

  ///
  typedef map<string,string> execenv_t;

  ///
  class execspec_t {
  public:
    execspec_t(const execargv_t& a, const execenv_t& e,
	       const string& l, const string& d) :
      argv(a), env(e), link_x(l), wd(d) {}
    
    execargv_t argv;
    execenv_t  env;
    string     link_x;
    string     wd;
  } ;

  ///
  typedef list<execspec_t> execchain_t;

  /// definition for the current data type
  typedef float datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  virtual bool Incremental() const { return true; }

  virtual bool CalculateOneFrame(int f);
 
  virtual bool CalculateOneLabel(int f, int l) {
    return CalculateCommon(f, false, l); 
  }
  
  virtual treat_type DefaultWithinFrameTreatment() const {
    return treat_concatenate; 
  }

  virtual treat_type DefaultBetweenFrameTreatment() const {
    return treat_separate; 
  }

  virtual treat_type DefaultBetweenSliceTreatment() const {
    return treat_separate; 
  }

  virtual featureVector CalculateRegion(const Region &/*r*/){
    ExternalData *d=
      (ExternalData*)CreateData(PixelType(),ConcatCountPerPixel(),0);
    
    cout << "External::CalculateRegion not implemented!!" << endl;

    bool allow_zero_count = true;

    featureVector ret=d->Result(allow_zero_count);
    deleteData(d);
    return ret;
  }

  ///
  string SolveArchitecture() const;

  ///
  string FindExecutable(const string&) const;

  ///
  string RemoteShell() const;

  ///
  virtual vector<string> RemoteExecution() const { return vector<string>(); }

  ///
  vector<string> RemoteExecutionLinux() const {
    vector<string> ret;
    string arch = SolveArchitecture();
    if (arch.substr(0,5)!="linux") {
      ret.push_back(RemoteShell());
      ret.push_back("samwise");
    }
    return ret;
  }

  /// 
  vector<string> RemoteExecutionNoBinary(const string& hostname="samwise") const
  {
    vector<string> ret;
    if (binary_not_found) {
      ret.push_back(RemoteShell());
      ret.push_back(hostname);
    }
    return ret;
  }

  ///
  vector<string> RemoteExecutionLinux64() const {
    vector<string> ret;
    string arch = SolveArchitecture();
    if (arch!="linux64") {
      ret.push_back(RemoteShell());
      ret.push_back("stimulus");
    }
    return ret;
  }

  ///
  vector<string> LocalLinuxOrRemoteLinux64() const {
    return SolveArchitecture()=="linux" ?
      vector<string>() : RemoteExecutionLinux64();
  }

  /// Opens dynamic library
  void *OpenDL(const string& name) const;
  
  string DLError() const;

  /// Data class for pixel based data
  class ExternalData : public Data {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    ExternalData(pixeltype t, int n, const Feature *p) : Data(t, n,p) { 
      datavec = vectortype(DataUnitSize());
    }

    /// Defined because we have virtual member functions...
    virtual ~ExternalData() {}

    /// Zeros the internal storage
    virtual void Zero() { 
      Data::Zero();
      datavec = vectortype(datavec.size());
    }

    virtual void ZeroToSize(int n) {
      datavec = vectortype(n);
      for (int i=0;i<n;i++)
	datavec[i] = 0;
    }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return datavec.size(); }

    /// += operator (not used jl?)
    virtual Data& operator+=(const Data &) {
      throw "ExternalData::operator+=() not defined"; }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool) const {
      return datavec;
    }

    virtual void SetVector(const vector<float>& v) {
      if (v.size()!=datavec.size()) {
	stringstream ss;
	ss << "ExternalData::SetVector(vector<float>) size mismatch : "
	   << "v.size()=" << v.size() << " datavec.size()=" << datavec.size();
	  throw ss.str();
      }
      datavec.assign(v.begin(), v.end());
    }

    virtual void SetVector(const vector<int>& v) {
      if (v.size()!=datavec.size())
	throw "ExternalData::SetVector(vector<int>) size mismatch";
      datavec.assign(v.begin(), v.end());
    }

  protected:
    /// internal storage of the datavector
    vectortype datavec;

  }; // class ExternalData

  // virtual vector<string> UsedDataLabels() const;
  
  vector<string> UsedDataLabelsBasic() const;

protected:
  /// This constructor is called by inherited class constructors.
  External() : binary_not_found(false) {}
  // automatic default constructor should do...

  /// This constructor doesn't add an entry in the methods list.
  /// !! maybe it shouldn't but it does !!
  External(bool) : Feature(false), binary_not_found(false) {}

  ///
  virtual bool ProcessOptionsAndRemove(list<string>&);

  void SetInput(const string& s) { input = s; }

  virtual string GetInput() const { return input; }

  virtual bool SaveOutput() const { return false; }

  virtual void StoreOutput(const string& buf) { output = buf; }

  virtual string GetOutput() const { return output; }

  virtual bool SaveError() const { return false; }

  virtual void StoreError(const string& buf) { error_output = buf; }

  virtual string GetError() const { return error_output; }

  virtual bool NeedsConversion() const { return false; }

  virtual pair<size_t,size_t> MaximumSize() const { return make_pair(0, 0); }

  virtual bool PreProcess(const string&, imagedata&, const pp_chain_t&,
			  const string&, int) const;

  virtual bool CalculateCommon(int f, bool all, int l = -1);

  virtual bool ExecPreProcess(int /*f*/, bool /*all*/, int /*l*/) {
    return true;
  }

  virtual bool ExecPostProcess(int /*f*/, bool /*all*/, int /*l*/) {
    return true;
  }

  virtual bool ProcessIntermediateFiles(const list<string>&,
					int /*f*/, bool /*all*/, int /*l*/) {
    return false;
  }

  string ReadBytes(int out, fd_set *fdsp, const string&);

  bool ExecRun (int f, bool all, int l);

  void ShowChildStatus(const string&, pid_t, int) const;

  ///
  virtual execchain_t GetExecChain(int f, bool all, int l) = 0;

  virtual void SetEnsurePost() {};
  virtual void SetEnsurePre() {};

  int FileSize(string filename) const;
  bool WaitForFile(string filename) const;

  void PostEnsure(string s) { post_ensure_files.push_back(s); }
  void PreEnsure(string s) { pre_ensure_files.push_back(s); }

  bool binary_not_found;

private:

  void RunExecChain(int in, int out, int err, const execchain_t&,
		    const vector<string>&);

  void RunExecProcess(int in, int out, int err, const execspec_t&,
		      const vector<string>&);

  bool SetChildLimits();

  /// ensure that important remote files do exist with non-zero size
  bool EnsureFilesPost() const { return EnsureFileList(post_ensure_files); }
  bool EnsureFilesPre() const { return EnsureFileList(pre_ensure_files); }
  bool EnsureFileList(vector<string>) const;

  /// list of files to ensure if executing remotely
  vector<string> pre_ensure_files, post_ensure_files;

#ifndef __MINGW32__
  static rlim_t rlimit_core, rlimit_cpu, rlimit_as;
#endif // __MINGW32__

  string input, output, error_output;
};
}
#endif // _External_h_

// Local Variables:
// mode: font-lock
// End:
