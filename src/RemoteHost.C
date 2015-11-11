// -*- C++ -*-  $Id: RemoteHost.C,v 2.17 2009/11/20 20:47:17 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <PicSOM.h>
#include <RemoteHost.h>
#include <DataBase.h>

#if defined(sgi) && !_SGIAPI
extern "C" int inet_aton(const char *, struct in_addr *); 
#endif // defined(sgi) && !_SGIAPI

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string RemoteHost_C_vcid =
    "@(#)$Id: RemoteHost.C,v 2.17 2009/11/20 20:47:17 jorma Exp $";

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool RemoteHost::debug = false;

void RemoteHost::Dump(Simple::DumpMode dt, ostream& os) const {
  os << Bold("RemoteHost ") << (void*)this
     << " db->Name()="   << db->Name()
     << " servant="      << servant
     << " address="      << ShowString(address_str)
     << " cookie="       << ShowString(cookie_str)
     << " userpassword=" << ShowString(userpassword_str)
     << " pid="          << pid
     << " debug="        << debug;

  if (next) {
    os << endl << "next=";
    next->Dump(dt, os);
  } else
    os << " next=NULL" << endl;  
}

///////////////////////////////////////////////////////////////////////////////

void RemoteHost::Address(const string& astr, int l) {
  const char *a = astr!="" ? astr.c_str() : NULL;

  if (a && l==32 && strchr(a, '/') && strlen(a)<100) {
    char tmp[100];
    strcpy(tmp, a);
    char *ptr = strchr(tmp, '/');
    *ptr = 0;
    Address(tmp, atoi(ptr+1));
    return;
  }

#ifndef PTW32_VERSION
  if (a && inet_aton(a, &addr.sin_addr))
    address_length = l;
  else
    address_length = -1;

  // cout << "RemoteHost::Address() : astr=[" << astr << "] address_length="
  //      << address_length << endl;

  if (address_length>=0 && address_length<=32)
    addr.sin_addr.s_addr &= Mask();
#endif // PTW32_VERSION

  SetAddress();
}

///////////////////////////////////////////////////////////////////////////////

unsigned int RemoteHost::Mask() const {
  if (address_length<0 || address_length>32)
    return 0;

//   if (sizeof(int)!=4)
//     Abort();

  unsigned int mask = 0xffffffff;
#ifdef sgi
  if (address_length<32)
    mask = (unsigned int)(((1L<<address_length)-1)<<(32-address_length));
#else
  if (address_length<32)
    mask = (unsigned int)((1L<<address_length)-1);
#endif //sgi

  return mask;
}

///////////////////////////////////////////////////////////////////////////////

void RemoteHost::SetAddress() {
#ifndef PTW32_VERSION
  if (IsSet())
    address_str = inet_ntoa(addr.sin_addr);

  else
#endif // PTW32_VERSION
    address_str = "--unset--";
}

///////////////////////////////////////////////////////////////////////////////

bool RemoteHost::Set(bool serv, const char *str) {
  bool debug = false;
  if (debug)
    cout << "RemoteHost::Set(" << serv << "," << str << ")" << endl;
  size_t n = strspn(str, "0123456789./");
  if (debug)
    cout << " n=" << n << endl;
  if (!n || n>18)
    return false;
  
  char addrstr[20];
  strncpy(addrstr, str, n);
  addrstr[n] = 0;
  Address(addrstr);
  if (debug)
    cout << " addrstr=[" << addrstr <<"] IsSet()=" << IsSet() << endl;
  if (!IsSet())
    return false;

  n += strspn(str+n, " \t");
  servant = serv;

  if (debug)
    cout << " n=" << n << " str[n]=<" << str[n] << "> IsClient()=" << IsClient()
	 << endl;

  if (isdigit(str[n])) {
    if (IsClient())
      return false;

    next = new RemoteHost(db);
    return next->Set(false, str+n);
  }

  switch (str[n]) {
  case 0:
  case '#':
    return IsClient();
    
  case ',':
    return (next = new RemoteHost(db))->Set(servant, str+n+1);

  case ':':
    if (IsServant() || strlen(str+n)>100)
      return false;
    else {
      char cook[1000];
      strcpy(cook, str+n+1);
      char *ptr = strpbrk(cook, " \t#");
      if (ptr)
	*ptr = 0;
      if (!strlen(cook))
	return false;

      ptr = strchr(cook, ':');
      if (ptr)
	*ptr = 0;
      Cookie(cook);
      UserPassword(ptr ? ptr+1: NULL);

      return true;
    }

  default:
    return false;
  }
}

///////////////////////////////////////////////////////////////////////////////

int RemoteHost::Accept(int fd) {
  //#ifndef PTW32_VERSION
#ifdef sgi
  int l = sizeof addr;
#else
  socklen_t l = sizeof addr;
#endif // sgi
  int s = accept(fd, (sockaddr*)&addr, &l);
  address_length = 32;
  SetAddress();
  return s;
  //#endif // PTW32_VERSION
}
  
///////////////////////////////////////////////////////////////////////////////

bool RemoteHost::Match(const RemoteHost& srv, const RemoteHost& cli,
			       const string& c, bool debugging) const {
  bool do_debug = debug || debugging;

  char tmptxt[1000];
  if (do_debug) {
    sprintf(tmptxt,"RemoteHost::Match(*%s*, server=%s, client==%s, cookie=%s)",
	    db->Name().c_str(), srv.Address().c_str(), cli.Address().c_str(),
	    c.c_str());
    WriteLog(tmptxt);
  }

  if (srv.Address()=="127.0.0.1") { // "localhost"
    if (do_debug)
      WriteLog(tmptxt, " localhost always ALLOWED.");
    return true;
  }

  bool srvfound = false;
  for (const RemoteHost *rule = this; rule; rule = rule->next) {
    if (do_debug) {
      sprintf(tmptxt, " %c %c %s/%d",
	      rule==this?'*':' ', rule->IsServant()?'S':'C',
	      rule->Address().c_str(), rule->AddressLength());
      if (rule->Cookie()!="")
	sprintf(tmptxt+strlen(tmptxt), ":%s", rule->Cookie().c_str());
      if (rule->UserPassword()!="")
	sprintf(tmptxt+strlen(tmptxt), ":%s", rule->UserPassword().c_str());
    }

    if (rule->IsServant() && !srvfound)
      srvfound = rule->Match(srv);

    if (rule->IsClient()) {
      if (!srvfound) {
	if (do_debug)
	  WriteLog(tmptxt, " Servant not found. FAIL");

	return false;

      } else if (rule->Match(cli)) {
	bool cookie_ok = false, cookie_seen = false;
	for (const RemoteHost *crule = rule; crule; crule = crule->next) {

	  if (crule->Cookie()!="")
	    cookie_seen = true;
	  if (c==crule->Cookie())
	    cookie_ok = true;
	}
	if (!cookie_seen || cookie_ok) {
	  if (do_debug)
	    WriteLog(tmptxt, " OK.");

	  return true;
	}
      }
    }

    if (do_debug) {
      if (srvfound)
	WriteLog(tmptxt, " server OK, client not OK. Taking next host...");
      else
	WriteLog(tmptxt, " not OK. Taking next host...");
    }
  }

  if (do_debug)
    WriteLog("   No more hosts in this rule. FAIL");

  return false;
}

///////////////////////////////////////////////////////////////////////////////

bool RemoteHost::Match(const RemoteHost& h) const {
  if (!IsSet())
    return false;

  unsigned int mine  =   addr.sin_addr.s_addr;
  unsigned int yours = h.addr.sin_addr.s_addr;
  unsigned int diff  = mine ^ yours;

  // cout << "RemoteHost::Match() address_length=" << address_length << endl;

  return !(diff & Mask());
}

///////////////////////////////////////////////////////////////////////////////

const char *RemoteHost::FindCookieP(const char *userpw) const {
  if (IsSet())
    for (const RemoteHost *rule = this; rule; rule = rule->next)
      if (rule->IsClient() && rule->UserPassword()==userpw) {
// 	cout << "RemoteHost::FindCookie(" << userpw << ") => ["
// 	     << rule->Cookie() << "]" << endl;
	return rule->Cookie().c_str();
      }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    /// Writes strings to log.
    void RemoteHost::WriteLog(const char *m1, const char *m2,
		  const char *m3, const char *m4, 
		  const char *m5) const {
      db->Picsom()->WriteLogCommon(LogString(), m1, m2, m3, m4, m5); }

    /// Writes contents of a stream to log.
    void RemoteHost::WriteLog(ostringstream& os) const {
      db->Picsom()->WriteLogCommon(LogString(), os); }


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// class RemoteHost ends here

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#include <Query.h>
#include <Source.C>
#include <List.C>
template class simple::SourceOf<picsom::RemoteHost>;
template class simple::ListOf<picsom::RemoteHost>;
#endif // __GNUC__

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

