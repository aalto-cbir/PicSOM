// -*- C++ -*-	$Id: ModifiableFile.h,v 2.4 2017/04/28 07:46:07 jormal Exp $

// 
// Copyright 1998-2007 PicSOM Development Group
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// picsom@cis.hut.fi
// 

#ifndef _PICSOM_MODIFIABLEFILE_H_
#define _PICSOM_MODIFIABLEFILE_H_

#include <TimeUtil.h>

#include <Simple.h>

static const string ModifiableFile_h_vcid =
  "@(#)$Id: ModifiableFile.h,v 2.4 2017/04/28 07:46:07 jormal Exp $";

namespace picsom {
  /**
     DOCUMENTATION MISSING
  */

  class ModifiableFile {
  public:
    /// Default constructor.
    ModifiableFile() {
      ZeroTimes();
    }

    /// Initializes time fields.
    ModifiableFile& ZeroTimes() {
      Simple::ZeroTime(rtime);
      if (debug_zerotimes)
	cout << TimeStamp() << "ModifiableFile::ZeroTimes() ["
	     << fname << "] called" << endl;
      return *this;
    }

    /// Sets name part.
    ModifiableFile& operator=(const string& n) { return Name(n); }

    /// Sets name part.
    ModifiableFile& Name(const string& n) { 
      string oldfname = fname; fname = n;
      return oldfname==fname ? *this : ZeroTimes(); }

    /// Gives name part.
    operator const string&() const { return fname; }

    /// Gives name part.
    const string& Name() const { return fname; }

    /// Gives name part.
    const char *NameP() const { return fname.c_str(); }

    /// Returns true if modified since last read.
    bool Modified() const {
      string d;                 // true if file doesn't exist
      bool r = MoreRecent(*this, rtime, debug_modified?&d:NULL);
      if (debug_modified)
	cout << TimeStamp() << "ModifiableFile::Modified() ["
	     << fname << "] MoreRecent() <" << d << "> returns " << r << endl;
      return r; }

    /// Sets read time.
    void WasRead() { Simple::SetTimeNow(rtime); }

    /// Sets static debug_modified to a value.
    static void DebugModified(bool d) { debug_modified = d; }

  protected:
    /// Name of the file.
    string fname;

    /// Read time.
    struct timespec rtime;

    /// True when calls to ZeroTimes() are traced.
    static bool debug_zerotimes;

    /// True when calls to Modified() are traced.
    static bool debug_modified;

  };  // class ModifiableFile

} // namespace picsom

#endif // _PICSOM_MODIFIABLEFILE_H_

// Local Variables:
// mode: lazy-lock
// compile-command: "ssh itl-cl10 cd picsom/c++\\; make debug"
// End:

