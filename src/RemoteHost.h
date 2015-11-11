// -*- C++ -*-	$Id: RemoteHost.h,v 2.12 2014/04/02 09:18:01 jorma Exp $

// 
// Copyright 1998-2007 PicSOM Development Group
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// picsom@cis.hut.fi
// 

#ifndef _PICSOM_REMOTEHOST_H_
#define _PICSOM_REMOTEHOST_H_

#include <missing-c-utils.h>

#include <Simple.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif // HAVE_SYS_SOCKET_H

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HAVE_NETINET_IN_H

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif // HAVE_ARPA_INET_H

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#include <ws2tcpip.h>
#endif // HAVE_WINSOCK2_H

#include <string>

namespace picsom {
  using namespace std;

  static const string RemoteHost_h_vcid =
    "@(#)$Id: RemoteHost.h,v 2.12 2014/04/02 09:18:01 jorma Exp $";

  class DataBase;

  class RemoteHost : public Simple {
  public:
    /// Poison constructor.
    RemoteHost() {
      SimpleAbort();
    }

    /// Constructor.
    RemoteHost(DataBase *d) { 
      db = d;
      memset(&addr, 0, sizeof addr);
      address_length = -1;
      next = NULL;
      servant = false;
      pid = 0;
    }
    
    /// The copy constructor.
    RemoteHost(const RemoteHost& rh) : Simple(rh) {
      *this = rh;
    }

    /// One more cup of coffee...
    RemoteHost(DataBase *d, const sockaddr_in& sa) {
      db = d;
      memcpy(&addr, &sa, sizeof addr);
      address_length = 32;
      SetAddress();
      next = NULL;
      servant = false;
      pid = 0;
    }      

    /// Destructor.
    virtual ~RemoteHost() {
      delete next;
    }

    /// Assignment operator
    RemoteHost& operator=(const RemoteHost& rh) {
      db = rh.db;
      memcpy(&addr, &rh.addr, sizeof addr);
      address_length = rh.address_length;
      SetAddress();
      next = NULL;
      servant = rh.servant;
      cookie_str = rh.cookie_str;
      userpassword_str = rh.userpassword_str;
      pid = rh.pid;
      return *this;
    }      

    /// Dump is required in all descendants of Simple.
    virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

    /// SimpleClassNames is required in all descendants of Simple.
    virtual const char **SimpleClassNames() const { static const char *n[] = {
      "RemoteHost", NULL }; return n; }

    /// Returns true if initialized.
    bool IsSet() const { return address_length>=0; }

    /// Sets address text internally.
    void SetAddress();

    /// Returns address in string form.
    const string& Address() const { return address_str; }

    /// Sets address in string form and its length.
    void Address(const string&, int = 32);

    /// Number of significant bits in address.
    int AddressLength() const { return address_length; }
    
    /// Number of significant bits in address.
    void AddressLength(int l) { address_length = l; }
    
    /// Returns port number in the other end.
    int Port() const { return address_length>=0 ? ntohs(addr.sin_port) : 0; }

    /// Returns process id of the opposite end.
    int Pid() const { return pid; }

    /// Sets record of process id of the opposite end.
    void Pid(int p) { pid = p; }

    /// Returns the cookie if set.
    const string& Cookie() const { return cookie_str; }

    /// Sets cookie.
    void Cookie(const string& c) { cookie_str = c; }

    /// Returns address or address:cookie.
    string AddressCookie() const {
      return Address() + (Cookie()!=""?":":"") + Cookie();
    }

    /// Returns user:password if set.
    const string& UserPassword() const { return userpassword_str; }

    /// Sets user:password.
    void UserPassword(const string& c) { userpassword_str = c; }

    /// Creates socket connection by accept().
    int Accept(int fd);

    /// True if specifies servant.
    bool IsServant() const { return IsSet() && servant; }

    /// Sets the servant variable.
    bool IsServant(bool b) { return servant = b; }

    /// True if specifies client.
    bool IsClient() const { return IsSet() && !servant; }

    /// Reads string and sets itself accordingly.
    bool Set(bool, const char*);

    /// Sets the next rule in the list. 
    bool SetNext(RemoteHost *r) { 
      if (next == NULL) {
	next = r; 
	return true;
      }
      return false;
    }

    void SetRights(map<string,string> *m) {
      allow.clear();
      allow.insert(m->begin(),m->end());
    }

    /// Returns true if the given right exists
    bool HasRight(string key) {
      map<string,string>::iterator i = allow.find(key);
      return i != allow.end();
    }

    /// Returns the value of the right. Gives valid return value only if
    /// the right exists ( HasRight(key)==true ), so thie should be checked
    /// before using this function..
    string GetRightValue(string key) {
      map<string,string>::iterator i = allow.find(key);
      if (i != allow.end()) 
	return i->second;
      else {
	ShowError("RemoteHost::GetRightValue() : key '", key ,"' is invalid");
	return "";
      }
    }

    /// Checks whether the requester matches the spec.
    bool Match(const RemoteHost&, const RemoteHost&, const string&, bool)const;

    /// Checks whether the requester matches the spec.
    bool Match(const RemoteHost&) const;

    /// Returns mask like 255.255.255.0 packed in 32 bits.
    unsigned int Mask() const;

    /// Tries to find existing cookie by user:password.
    const char *FindCookieP(const char*) const;

    /// Sets value of debug.
    static void Debug(bool d) { debug = d; }

    /// Returns value of debug.
    static bool Debug() { return debug; }

    /// Writes strings to log.
    void WriteLog(const char *m1, const char *m2 = NULL,
		  const char *m3 = NULL, const char *m4 = NULL, 
		  const char *m5 = NULL) const;

    /// Writes contents of a stream to log.
    void WriteLog(ostringstream& os) const;

    /// Gives log form of RemoteHost name.
    const char *LogString() const { return "$$$"; }

  protected:
    /// Upward link.
    DataBase *db;
 
    /// RemoteHost's address.
    sockaddr_in addr;

    /// Number of meaningful bits in address.
    int address_length;

    /// Address in string form.
    string address_str;

    /// Process id of the other end.
    int pid;

    /// True if specifies servant, false if client.
    bool servant;

    /// Next rule in list.
    RemoteHost *next;

    /// Cookie used in access rules.
    string cookie_str;

    /// User:password used in access rules.
    string userpassword_str;

    /// True if rule usage should be traced.
    static bool debug;

    /// Access rules for this connection
    map<string,string> allow;

  };  // class RemoteHost

} // namespace picsom

#endif // _PICSOM_REMOTEHOST_H_

// Local Variables:
// mode: lazy-lock
// compile-command: "ssh itl-cl10 cd picsom/c++\\; make debug"
// End:

