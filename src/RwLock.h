// -*- C++ -*-  $Id: RwLock.h,v 2.9 2015/05/07 07:33:47 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_RWLOCK_H_
#define _PICSOM_RWLOCK_H_

#include <Simple.h>
using simple::Simple;

#include <string>
using namespace std;

static const string RwLock_h_vcid =
  "@(#)$Id: RwLock.h,v 2.9 2015/05/07 07:33:47 jorma Exp $";

#ifdef SIMPLE_USE_PTHREADS
#define PICSOM_USE_PTHREADS
#endif // SIMPLE_USE_PTHREADS

namespace picsom {

#ifdef PICSOM_USE_PTHREADS
  /// 

  /**
     DOCUMENTATION MISSING
  */

  class RwLock {
  public:
    /// 
    RwLock() : wr_locker_set(false), wr_depth(0), bounce(false),
	       when(RwLock::emptytime()) {
      // cout << "RwLock::RwLock()" << endl;
      pthread_rwlock_init(&rwlock, NULL);
    }

    // let's just hope that this works...
    /// Poison...
    /// RwLock(const RwLock&) { abort(); } 
    
    ~RwLock() {
      // cout << "RwLock::~RwLock()" << endl;
      pthread_rwlock_destroy(&rwlock);
    }
    
    /// Poison...
    RwLock& operator=(const RwLock&) { abort(); return *this; } 

    /// should be called if object has been copied and both
    /// objects should have a working lock
    bool ReInit() {
      pthread_rwlock_init(&rwlock, NULL);
      return true;
    }

    ///
    bool Locked() const { return wr_locker_set; }

    ///
    pthread_t Locker() const { return wr_locker; }

    ///
    string LockerStr() const {
      if (!Locked())
	return "none";
      if (ThisThreadLocks())
	return "this thread";
      return "thread "+ThreadIdentifierUtil(wr_locker, -1);
    }

    ///
    bool ThisThreadLocks() const {
      return wr_locker_set && pthread_equal(wr_locker, pthread_self());
    }

    ///
    void LockRead() {
      if (!ThisThreadLocks())
	pthread_rwlock_rdlock(&rwlock);
    }

    /// A trick
    void LockRead() const {
      ((RwLock*)this)->LockRead();
    }

    ///
    void LockWrite() {
      if (!ThisThreadLocks()) {
	pthread_rwlock_wrlock(&rwlock);
	wr_locker = pthread_self();
	wr_locker_set = true;
      }
      wr_depth++;
    }

    ///
    void UnlockRead() {
      if (!ThisThreadLocks())
	pthread_rwlock_unlock(&rwlock);
    }

    /// A trick
    void UnlockRead() const {
      ((RwLock*)this)->UnlockRead();
    }

    ///
    void UnlockWrite() {
      // Simple is not used in segmentation directory...
      // if (!ThisThreadLocks())
      //   Simple::ShowError("UnlockWrite() failing badly...");

      wr_depth--;
      if (!wr_depth) {
	wr_locker_set = false;
	pthread_rwlock_unlock(&rwlock);
      }
    }

    ///
    const char *StatusChar() const {
      // We have to do a dirty trick on const...
      pthread_rwlock_t *lock = (pthread_rwlock_t*)&rwlock;
      int rd = pthread_rwlock_tryrdlock(lock);
      if (rd)
	return ThisThreadLocks() ? "W " : "w ";
      pthread_rwlock_unlock(lock);
      int wr = pthread_rwlock_trywrlock(lock);
      if (wr)
	return "R ";
      pthread_rwlock_unlock(lock);

      return "  ";
    }
	
    /// True if doing something that may take long.
    bool Bounce() const {
      if (!bounce)
	return false;

      // We have to do a dirty trick on const...
      pthread_rwlock_t *lock = (pthread_rwlock_t*)&rwlock;
      int rd = pthread_rwlock_tryrdlock(lock);
      if (rd)
	return true;

      pthread_rwlock_unlock(lock);
      return false;
    }

  protected:
    /// 
    static const timespec_t& emptytime() {
      static timespec_t e;
      Simple::ZeroTime(e);
      return e;
    }

    /// 
    pthread_rwlock_t rwlock;

    ///
    bool wr_locker_set;

    ///
    pthread_t wr_locker;

    ///
    int wr_depth;

    ///
    bool bounce;

    ///
    string msg;

    ///
    timespec_t when;

  };  // class RwLock
#endif // PICSOM_USE_PTHREADS

} // namespace picsom

#endif // _PICSOM_RWLOCK_H_

// Local Variables:
// mode: lazy-lock
// compile-command: "ssh itl-cl10 cd picsom/c++\\; make debug"
// End:

