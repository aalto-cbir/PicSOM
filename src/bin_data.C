// -*- C++ -*-  $Id: bin_data.C,v 2.27 2015/03/04 14:21:53 jorma Exp $
// 
// Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <bin_data.h>

//#include <Util.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif // HAVE_SYS_MMAN_H

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif // HAVE_FCNTL_H

#include <cerrno>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string bin_data_C_vcid =
    "@(#)$Id: bin_data.C,v 2.27 2015/03/04 14:21:53 jorma Exp $";

  bool bin_data::close_handles = true;

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::header::is_supported_format(format_type fmt) {
    // string msg = "bin_data::is_supported_format() : ";
    if (is_var_length_format(fmt) || is_expl_index_format(fmt) ||
	is_file_dict_format(fmt))
      return false;

    format_type tf = target_format(fmt);
    return tf==format_float || tf==format_size_t;
  }

  /////////////////////////////////////////////////////////////////////////////

  const string& bin_data::header::target_format_str() const {
    // string msg = "bin_data::target_format_str() : ";
    static string f = "float", i = "size_t", b = "bool", s = "string",
      u = "undef", x = "unknown";

    format_type ft = target_format();
    switch(ft) {
    case format_undef:  return u;
    case format_float:  return f;
    case format_size_t: return i;
    case format_bool:   return b;
    case format_string: return s;
    default:            return x;
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  bin_data::bin_data(const string& f, bool rw,
		     bin_data::header::format_type fmt, size_t b, size_t l) :
    _fd(-1), _size(0), _rw(rw), _ptr(NULL), _prodquant(NULL), _incore(false),
    _n_access(0), _opt_interval(0), _opt_rss_target(0) {

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_init(&rwlock, NULL);
#endif // BIN_DATA_USE_PTHREADS

    if (f!="")
      open(f, rw, fmt, b, l);
  }

  /////////////////////////////////////////////////////////////////////////////

  bin_data::~bin_data()  {
    close(false);
#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_destroy(&rwlock);
#endif // BIN_DATA_USE_PTHREADS
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::close(bool do_throw) throw(string) {
    string msg = "bin_data::close() : ", err;

    bool ok = true;

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_wrlock(&rwlock);
#endif // BIN_DATA_USE_PTHREADS

#ifdef HAVE_SYS_MMAN_H
    if (_ptr && munmap(_ptr, _size)) {
      ok = false;
      err = "munmap() failed";
    }
#endif // HAVE_SYS_MMAN_H

    if (ok && _fd>=0 && ::close(_fd)) {
      ok = false;
      err = "close() failed "+string(strerror(errno));
    }

    delete _prodquant;

    _ptr = NULL;
    _fd  = -1;
    _filename = "";
    _size = 0;
    _prodquant = NULL;

    memset(&_header, 0, sizeof(header));

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_unlock(&rwlock);
#endif // BIN_DATA_USE_PTHREADS

    if (do_throw)
      throw msg+err;

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::flush() {
    //string msg = "bin_data::flush() : ";
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::fulltest(size_t i) const {
    // string msg = "bin_data::fulltest() : ";
    unsigned char *a = (unsigned char*)raw_address(i);
    if (!a)
      return false;

    if (_header.is_expl_status_format()) {
      a += _header.statusoffset();
      return a[2]!=255 || a[3]!=255;
    }

    // obs! this is only valid for format==1
    a += _header.payloadoffset();
    size_t l = _header.payloadsize();
    for (size_t i=0; i<l; i+=sizeof(float))
      if (a[i+2]==255 && a[i+3]==255)
	return false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::exists(size_t i) const {
    // string msg = "bin_data::exists() : ";
    unsigned char *a = (unsigned char*)raw_address(i);
    if (!a)
      return false;

    if (_header.is_expl_status_format()) {
      a += _header.statusoffset();
      return a[2]!=255 || a[3]!=255;
    }

    // obs! this is only valid for format==1
    a += _header.payloadoffset();
    return a[2]!=255 || a[3]!=255;
  }

  /////////////////////////////////////////////////////////////////////////////

  void *bin_data::raw_address(size_t i) const {
    // string msg = "bin_data::raw_address() : ";
    size_t l = _header.hsize+i*_header.rlength_x;
    return _ptr && l+_header.rlength_x<=_size ? (char*)_ptr+l : NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  void *bin_data::raw_address_if_exists(size_t i) const {
    // string msg = "bin_data::raw_address_if_exists() : ";
    unsigned char *a = (unsigned char*)raw_address(i), *b = a;
    if (!a)
      return NULL;
    b += _header.statusoffset();
    return (b[2]!=255 || b[3]!=255) ? (void*)a : NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  void *bin_data::vector_address(size_t i) const {
    // string msg = "bin_data::vector_address() : ";
    unsigned char *a = (unsigned char*)raw_address(i);
    return a ? a+_header.payloadoffset() : NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  void *bin_data::vector_address_if_exists(size_t i) const {
    // string msg = "bin_data::vector_address_if_exists() : ";
    unsigned char *a = (unsigned char*)raw_address_if_exists(i);
    return a ? a+_header.payloadoffset() : NULL;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::erase(size_t i) const {
    // string msg = "bin_data::erase() : ";
    void *a = raw_address(i);
    if (!a)
      return false;

    memset((char*)a, 255, _header.rlength_x);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::open(const string& fn, bool rw,
		      bin_data::header::format_type fmt,
		      size_t bits, size_t vl) throw(string) {
    // string msg = "bin_data::open("+fn+") : ";

    bool ok = false;
    string err;

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_wrlock(&rwlock);
#endif // BIN_DATA_USE_PTHREADS

    try {
      ok = open_inner(fn, rw, fmt, bits, vl);
    } catch (const string& e) {
      err = e;
    }

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_unlock(&rwlock);
#endif // BIN_DATA_USE_PTHREADS

    if (!ok)
      throw err;

    _opt_interval   = 1000;
    _opt_rss_target = 1024L*1024*1024;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::open_inner(const string& fn, bool rw,
			    bin_data::header::format_type fmt,
			    size_t bits, size_t vl) throw(string) {
    string msg = "bin_data::open_inner("+fn+") : ";

    _incore = fn.find("/dev/null")==0;

    if (is_open() && fn==_filename) // parallel threads can cause this...
      return true;

    if (is_open())
      throw(msg+"already open");

    if (!_incore) {
      int oflags = rw ? O_RDWR|O_CREAT : O_RDONLY;
      _fd = ::open(fn.c_str(), oflags, 0664);
      if (_fd<0)
	throw(msg+"open() error");
    }

    _filename = fn;

    struct stat st;
    st.st_size = 0;
    if (!_incore && fstat(_fd, &st))
      throw(msg+"fstat() error A");

    if (st.st_size==0) {
      if (!vl)
	throw(msg+"vectorlength==0");

      if (!header::is_supported_format(fmt))
	throw(msg+"not supported format");

      header::format_type ft = header::target_format(fmt);

      _header = header();
      _header.format_x  = fmt;
      _header.vdim    = vl;
      _header.rlength_x = (ft==header::format_float?4*vl:bits)+
	_header.payloadoffset();

      if (!_incore) {
	//WriteLog("Initializing bin data file <"+db->ShortFileName(fname)+"> "+
	//	       _header.str());
	if (write(_fd, &_header, _header.hsize)!=(ssize_t)_header.hsize)
	  throw(msg+"write() error");
	if (fstat(_fd, &st))
	  throw(msg+"fstat() error B");

      } else
	st.st_size = sizeof(_header);

    } else if ((size_t)st.st_size<sizeof(_header))
      throw(msg+"file size "/*+ToStr(st.st_size)+*/" too small");
    
#ifdef HAVE_SYS_MMAN_H
    int prot = rw ? PROT_WRITE|PROT_READ : PROT_READ;
    int flags = _incore ? (MAP_PRIVATE | MAP_ANONYMOUS) : MAP_SHARED;
    off_t offset = 0;
    void *addr = NULL;

    _ptr = mmap(addr, st.st_size, prot, flags, _fd, offset);
    if (_ptr==MAP_FAILED) {
      _ptr = NULL;
      throw(msg+"mmap() error "+string(strerror(errno)));
    }
    if (_incore)
      memcpy(_ptr, &_header, sizeof(_header));

#endif // HAVE_SYS_MMAN_H
    
    if (!_incore && close_handles) {
      if (::close(_fd)) {
	_ptr = NULL;
	throw(msg+"close() error "+string(strerror(errno)));
      }
      _fd = -1;
    }

#ifdef HAVE_SYS_MMAN_H
    if (madvise(_ptr, st.st_size, MADV_SEQUENTIAL)) {
      _ptr = NULL;
      throw(msg+"madvise() error "+string(strerror(errno)));
    }
#endif // HAVE_SYS_MMAN_H

    size_t hso = ((header*)_ptr)->hsize, hse = hso;
    if (hse<4)
      hse = 4;
    if (hse>sizeof(header))
      hse = sizeof(header);
    memcpy(&_header, _ptr, hse);

    string magic(_header.magic, 4);
    if (magic!="PSBD") {
      float fdim = 0;
      memcpy(&fdim, _header.magic, 4);
      int idim = (int)floor(fdim);
      if (idim==fdim && idim>0) {
	_header = header();
	_header.version = 0.0;
	_header.hsize   = 4;
	_header.rlength_x = 4*idim;
	_header.vdim    = idim;
	_header.format_x  = header::format_float;

      } else
	throw(msg+"magic error");
    }

    if (vl && _header.vdim!=vl)
      throw(msg+"vdim error");

    if ((st.st_size-_header.hsize)%_header.rlength_x)
      throw(msg+"rlength error");

    if (!_header.is_supported_format())
      throw(msg+"format error");

    if (_header.rlength_x!=(componentsize()*_header.vdim)/8+
	_header.payloadoffset())
      throw(msg+"rlength vs. vdim mismatch");

    _rw = rw;
    _size = st.st_size;  // now is_ok()==true

    /*
    string nistr = ToStr((_size-_header.hsize)/
			 _header.rlength)+" items";

    WriteLog("Opened bin data file <"+db->ShortFileName(fname)+"> for "+
	     (rw?"modification":"reading"));

    WriteLog("  "+_header.str()+" "+ToStr(_size)+
	     " bytes "+nistr+" (fd="+ToStr(_fd)+", hs="+
	     ToStr(hso)+"/"+ToStr(hse)+")");
    */

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::resize(size_t n, unsigned char x) throw(string) {
    // string msg = "bin_data::resize() : ";

    bool ok = false;
    string err;

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_wrlock(&rwlock);
#endif // BIN_DATA_USE_PTHREADS

    try {
      ok = resize_inner(n, x);
    } catch (const string& e) {
      err = e;
    }

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_unlock(&rwlock);
#endif // BIN_DATA_USE_PTHREADS

    if (!ok)
      throw err;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::resize_inner(size_t n, unsigned char x) throw(string) {
    string msg = "bin_data::resize_inner() : ";

    if (close_handles && _fd<0 && !_incore) {
      int oflags = _rw ? O_RDWR|O_CREAT : O_RDONLY;
      _fd = ::open(_filename.c_str(), oflags, 0664);
      if (_fd<0)
	throw(msg+"open() error");
    }

    size_t l = rawsize(n);

    if (!_incore && ftruncate(_fd, l))
      throw msg+"ftruncate() error "+string(strerror(errno));

#ifdef HAVE_MREMAP
    void *p = mremap(_ptr, _size, l, MREMAP_MAYMOVE);
    if (p==MAP_FAILED)
      throw msg+"mremap() error "+string(strerror(errno));
#else
#ifdef HAVE_SYS_MMAN_H
    int prot = PROT_WRITE|PROT_READ;
    int flags = MAP_SHARED;
    void *p = mmap(NULL, l, prot, flags, _fd, 0);
    if (p==MAP_FAILED)
      throw msg+"mmap() error "+string(strerror(errno));

    if (munmap(_ptr, _size))
      throw msg+"munmap() error "+string(strerror(errno));
#else
    void *p = NULL;
#endif // HAVE_SYS_MMAN_H
#endif // HAVE_MREMAP

    if (close_handles && !_incore) {
      if (::close(_fd))
	throw msg+"close() error "+string(strerror(errno));
      _fd = -1;
    }

    memset((char*)p+_size, x, l-_size);
   
    _ptr  = p;
    _size = l;
    
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t bin_data::nobjects() const {
    size_t ret = 0;

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_rdlock((pthread_rwlock_t*)&rwlock);
#endif // BIN_DATA_USE_PTHREADS

    if (is_open()) {
      if (!is_ok())
	ret = (size_t)-1;

      else {
	// obs only for fixed-size objects...
	size_t n = (_size-sizeof(header))/_header.rlength_x;
	ret = _size==sizeof(header)+n*_header.rlength_x ? n : (size_t)-1;
      }
    }

#ifdef BIN_DATA_USE_PTHREADS
    pthread_rwlock_unlock((pthread_rwlock_t*)&rwlock);
#endif // BIN_DATA_USE_PTHREADS

    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<float> bin_data::get_float(size_t i) const {
    string msg = "bin_data::get_float() : ";

    if (!is_ok() || !_header.is_supported_format())
      return vector<float>();

    if (_prodquant) {
      if (_prodquant->nobjects()%_header.vdim) {
	cerr << msg << "size error" << endl;
	return vector<float>();
      }
      bin_data *p = _prodquant;
      ((bin_data*)this)->_prodquant = NULL;
      vector<size_t> q = get_size_t(i);
      ((bin_data*)this)->_prodquant = p;
      float *b = (float*)_prodquant->vector_address_if_exists(0);
      size_t d = _prodquant->vdim(), n = _prodquant->nobjects()/_header.vdim;
      vector<float> v(vdim());
      for (size_t jq=0, jv=0; jq<q.size(); jq++, jv+=d) {
	if (q[jq]>=n) {
	  cerr << msg << "index error" << endl;
	  return vector<float>();
	}
	memcpy(&v[jv], b+q[jq]*d, d*sizeof(float));
	b += n*d;
      }
      return v;
    }

    float *pos = (float*)vector_address_if_exists(i);
    if (!pos)
      return vector<float>();

    // vector<float> v(_header.vdim);
    // memcpy(&v[0], pos, _header.vdim*sizeof(float));

    vector<float> v(pos, pos+_header.vdim);

    check_optimize(pos);

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::set_float(size_t i, const vector<float>& v)  {
    // string msg = "bin_data::set_float() : ";

    if (!is_ok() || !_rw || v.size()!=_header.vdim ||
	_header.target_format()!=header::format_float)
      return false;

    float *pos = (float*)vector_address(i);
    if (!pos)
      return false;

    memcpy(pos, &v[0], v.size()*sizeof(float));

    check_optimize(pos);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<bool> bin_data::get_bool(size_t i) const {
    // string msg = "bin_data::get_bool() : ";

    if (!is_ok() || !_header.is_supported_format())
      return vector<bool>();

    unsigned char *pos = (unsigned char*)vector_address_if_exists(i), *pp = pos;
    if (!pos)
      return vector<bool>();

    size_t n = _header.payloadsize();
    vector<bool> v(8*n);
    for (size_t j=0, l=0; j<n; j++) {
      unsigned char x = *pp++;
      for (size_t j=0; j<8; j++, l++) {
	bool b = x&0x80;
	v[l] = b;
	x *= 2;
      }
    }

    check_optimize(pos);

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::set_bool(size_t /*i*/, const vector<bool>& /*v*/)  {
    // string msg = "bin_data::set_bool() : ";

    // if (!is_ok() || !_rw || v.size()!=_header.vdim || _header.format!=1)
    //   return false;

    // bool *pos = (bool*)address(i);
    // if (!pos)
    //   return false;

    // memcpy(pos, &v[0], v.size()*sizeof(bool));

    // check_optimize(pos);

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  vector<size_t> bin_data::get_size_t(size_t i) const {
    // string msg = "bin_data::get_size_t() : ";

    vector<size_t> v;

    if (!is_ok() || !_header.is_supported_format())
      return v;

    unsigned char *pos = (unsigned char*)vector_address_if_exists(i), *pp = pos;
    if (!pos)
      return v;

    size_t osize = componentsize();
    if (osize!=8 && osize!=16 && osize!=32 && osize!=64)
      return v;

    size_t vl = vdim();
    v.resize(vl);
    for (size_t j=0; j<vl; j++) {
      switch (osize) {
      case 8:
	v[j] = *pp++;
	break;
      case 16:
	v[j] = *(uint16_t*)pp;
	pp += 2;
	break;
      case 32:
	v[j] = *(uint32_t*)pp;
	pp += 4;
	break;
      case 64:  // could use memcpy() but is rare...
	v[j] = *(uint64_t*)pp;
	pp += 8;
	break;
      }
    }

    check_optimize(pos);

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::set_size_t(size_t i, const vector<size_t>& v)  {
    // string msg = "bin_data::set_size_t() : ";

    if (!is_ok() || !_rw || v.size()!=_header.vdim ||
	_header.target_format()!=header::format_size_t)
      return false;

    size_t osize = componentsize();
    if (osize!=8 && osize!=16 && osize!=32 && osize!=64)
      return false;

    unsigned char *pos = (unsigned char*)vector_address(i), *pp = pos;
    if (!pos)
      return false;

    size_t vl = v.size();
    for (size_t j=0; j<vl; j++) {
      switch (osize) {
      case 8:
	*pp++ = v[j];
	break;
      case 16:
	*(uint16_t*)pp = v[j];
	pp += 2;
	break;
      case 32:
	*(uint32_t*)pp = v[j];
	pp += 4;
	break;
      case 64:  // could use memcpy() but is rare...
	*(uint64_t*)pp = v[j];
	pp += 8;
	break;
      }
    }

    check_optimize(pos);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  string bin_data::get_string(size_t i) const {
    // string msg = "bin_data::get_string() : ";

    if (!is_ok() || !_header.is_supported_format())
      return "";

    char *pos = (char*)vector_address_if_exists(i);
    if (!pos)
      return "";

    string v(pos, pos+_header.rlength_x);

    check_optimize(pos);

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::set_string(size_t /*i*/, const string& /*v*/)  {
    // string msg = "bin_data::set_string() : ";

    // if (!is_ok() || !_rw || v.size()!=_header.vdim || _header.format!=1)
    //   return false;

    // string *pos = (string*)address(i);
    // if (!pos)
    //   return false;

    // memcpy(pos, &v[0], v.size()*sizeof(string));

    // check_optimize(pos);

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  void bin_data::check_optimize(void *p) const {
    // string msg = "bin_data::check_optimize() : ";

    if (_opt_interval && _opt_rss_target && !is_incore() &&
	(_n_access%_opt_interval==0))
      optimize(p);

    ((bin_data*)this)->_n_access++;
  }

  /////////////////////////////////////////////////////////////////////////////

  void bin_data::optimize(void *p) const {
#ifdef HAVE_SYS_MMAN_H
    // string msg = "bin_data::optimize() : ";

    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t l = ((char*)p-(char*)_ptr)/pagesize;

    if (madvise(_ptr, l*pagesize, MADV_DONTNEED))
      {} // cerr << "madvise() failed 1" << endl;
    
    if (madvise((char*)_ptr+l*pagesize, _size-l*pagesize, MADV_SEQUENTIAL))
      {} // cerr << "madvise() failed 2" << endl;
    
    if (false) {
    size_t rss = rss_size();
    // cout << "RSS: " << _n_access << " : " << rss
    //      << " (" << HumanReadableBytes(rss) << ")" << endl;

    // MADV_DONTFORK

    int advise = rss<=_opt_rss_target ? MADV_SEQUENTIAL :  MADV_DONTNEED;
    if (madvise(_ptr, _size, advise))
      {} // cerr << "madvise() failed" << endl;
    }
#endif // HAVE_SYS_MMAN_H
  }

  /////////////////////////////////////////////////////////////////////////////

  size_t bin_data::rss_size() const {
#ifdef HAVE_SYS_MMAN_H
    // string msg = "bin_data::rss_size() : ";

    if (!is_ok())
      return 0;

    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t l = (_size+pagesize-1)/pagesize, s = 0;
    unsigned char *vec = new unsigned char[l];

    int r = mincore(_ptr, _size, vec);

    if (r==0)
      for (size_t i=0; i<l; i++)
	if (vec[i])
	  s++;

    delete vec;

    return s*pagesize;
#endif // HAVE_SYS_MMAN_H
    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool bin_data::prodquant(const string& f)  {
    // string msg = "bin_data::prodquant() : ";
    
    delete _prodquant;
    _prodquant = new bin_data(f);
    // cout << _prodquant->str() << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /*
  bool bin_data::()  {
    // string msg = "bin_data::() : ";

  }
  */
  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
