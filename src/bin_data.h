// -*- C++ -*-  $Id: bin_data.h,v 2.31 2021/05/11 14:46:57 jormal Exp $
// 
// Copyright 1998-2021 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_BIN_DATA_H_
#define _PICSOM_BIN_DATA_H_

#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <stdexcept>
using namespace std;

#define BIN_DATA_USE_UTIL_H 1
#ifdef BIN_DATA_USE_UTIL_H
#include <Util.h>
#else
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#define HAVE_SYS_MMAN_H 1
#define HAVE_STRING_H 1
#endif // BIN_DATA_USE_UTIL_H

#ifndef NO_PTHREADS
#define BIN_DATA_USE_PTHREADS 1
#endif // NO_PTHREADS

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

namespace picsom {
#ifndef BIN_DATA_USE_UTIL_H
  static string ToStr(float) { return ""; }
  static string HumanReadableBytes(size_t) { return ""; }
#endif // BIN_DATA_USE_UTIL_H
  
  ///
  class bin_data {
  public:
    ///
    class header {
    public:
      ///
      enum format_type {
	format_undef       =  0,
	format_float	   =  1,
	format_double	   =  2,
	format_fcomplex	   =  3,
	format_dcomplex	   =  4,
	format_bool 	   =  8,
	format_size_t	   =  9,
	format_string      = 16,
	format_var_length  = 1L<<16,
	format_expl_status = 1L<<24,
	format_expl_index  = 1LL<<32,
	format_file_dict   = 1LL<<40
      };

      ///
      header() {
	memcpy(magic, "PSBD", 4);
      }

      ///
      string str() const {
	stringstream ss;
	ss << magic[0] << magic[1] << magic[2] << magic[3]
	   << " v" << version << " [" << hsize << "] len=" << rlength
	   << " dim=" << vdim << " format=0x" << hex << format;
	return ss.str();
      }

      ///
      static format_type target_format(format_type f) {
	return format_type(f&255);
      }

      ///
      format_type target_format() const {
	return target_format((format_type)format);
      }

      ///
      const string& target_format_str() const;

      ///
      static bool is_supported_format(format_type);

      ///
      bool is_supported_format() const {
	return is_supported_format((format_type)format);
      }

      ///
      static bool is_var_length_format(format_type f) {
	return f&format_var_length;
      } 

      ///
      bool is_var_length_format() const {
	return is_var_length_format((format_type)format);
      } 

      ///
      static bool is_expl_status_format(format_type f) {
	return f&format_expl_status;
      } 

      ///
      bool is_expl_status_format() const {
	return is_expl_status_format((format_type)format);
      } 

      ///
      static bool is_expl_index_format(format_type f) {
	return f&format_expl_index;
      } 

      ///
      bool is_expl_index_format() const {
	return is_expl_index_format((format_type)format);
      }

      ///
      static bool is_file_dict_format(format_type f) {
	return f&format_file_dict;
      } 

      ///
      bool is_file_dict_format() const {
	return is_file_dict_format((format_type)format);
      }

      ///
      size_t statusoffset() const {
	return 8*is_expl_index_format();
      }

      ///
      size_t payloadoffset() const {
	return 8*is_expl_index_format()+4*is_expl_status_format();
      }
 
      ///
      size_t payloadsize() const {
	return is_var_length_format() ? 0 : rlength-payloadoffset();
      }

      ///
      char magic[4];      // offset=00 "PSBD" for "PicSOM binary data"
    
      ///
      float version;      // offset=04 1.0 or 1.1
      
      ///
      uint64_t hsize = sizeof(header); // offset=08 header size, always 64

      ///
      uint64_t rlength = 0;   // offset=16 size of a record, 4*vector dim
      
      ///
      uint64_t vdim = 0;      // offset=24 vector dimensionality
      
      ///
      uint64_t format = 0;    // offset=32 for float ==1
      
      ///
      uint64_t dummyx = 0;    // offset=40 filled with zeros
      
      ///
      uint64_t min = 0;       // offset=48 first index of first block
      
      ///
      uint64_t max = 0;       // offsets=56 last+1 index of first block
      
    }; // class bin_data::header

    ///
    bin_data(const string& fn = "", bool = false, float = 1.0,
	     bin_data::header::format_type = header::format_undef,
	     size_t = 0, size_t = 0);

    ///
    ~bin_data();

    ///
    const header& get_header_real() const /*throw(logic_error)*/ {
      string msg = "bin_data::get_header_real("+_filename+") : ";
      if (!is_ok())
	throw logic_error(msg+"bin_data not opened or too small");
      return *(header*)_ptr;
    }

    ///
    const header& get_header_copy() const /*throw(logic_error)*/ {
      string msg = "bin_data::get_header_copy("+_filename+") : ";
      if (!is_ok())
	throw logic_error(msg+"bin_data not opened or too small");
      return _header;
    }
    
    ///
    // size_t object_count() const {
    //   return (_size-_header.hsize)/_header.rlength;
    // }

    ///
    string str() const;

    ///
    const string& filename() const { return _filename; }

    ///
    bool is_open() const { return /*_fd>=0*/ _ptr; }
 
    ///
    bool is_incore() const { return _incore; }

    ///
    static bool is_version(float x, float v) { return fabs(x-v)<0.0001; }
      
    ///
    bool is_v10() const { return is_version(_header.version, 1.0); }
      
    ///
    bool is_v11() const { return is_version(_header.version, 1.1); }

    ///
    const list<size_t>& blocks() const { return _blocks; }

    ///
    list<vector<size_t> > block_summary() const;

    ///
    string dump_blocks() const;
    
    ///
    string dump_block_summary() const;
    
    ///
    size_t rawsize() const { return _size; }

    ///
    size_t rawsize(size_t n) const { 
      if (is_v10())
	return _header.hsize+n*_header.rlength; 
      else
	return -1;
    }

    ///
    size_t nobjects() const;

    ///
    size_t componentsize() const {
      header::format_type tf = _header.target_format();
      switch (tf) {
      case header::format_float:  return 32;
      case header::format_double: return 64;
      case header::format_bool:   return  1;
      case header::format_size_t:
	return 8*(_header.payloadsize())/_header.vdim;
      default: return 0;
      }
    }

    ///
    bool is_ok() const { return is_open() && rawsize()>=sizeof(header); }

    ///
    bool open(const string&, bool, float, header::format_type,
	      size_t, size_t) /*throw(string)*/;

    ///
    bool open_inner(const string&, bool, float, header::format_type,
		    size_t, size_t) /*throw(string)*/;

    ///
    bool resize(size_t, unsigned char = 255) /*throw(string)*/;

    ///
    bool resize_common(size_t, unsigned char = 255) /*throw(string)*/;

    ///
    bool resize_common_inner(size_t, unsigned char = 255) /*throw(string)*/;

    ///
    bool flush();

    ///
    bool close(bool) /*throw(string)*/;

    ///
    size_t vdim() const {
      if (!_prodquant)
	return is_open() ? _header.vdim : 0;

      return is_open() ? _header.vdim * _prodquant->vdim() : 0;
    }

    ///
    bool fulltest(size_t) const;

    ///
    bool exists(size_t) const;

    ///
    bool erase(size_t) const;

    ///
    void erase_all() const;

    ///
    vector<float> get_float(size_t) const;

    ///
    bool set_float(size_t, const vector<float>&, size_t = 0);

    ///
    vector<bool> get_bool(size_t) const;

    ///
    bool set_bool(size_t, const vector<bool>&);

    ///
    vector<size_t> get_size_t(size_t) const;

    ///
    bool set_size_t(size_t, const vector<size_t>&);

    ///
    string get_string(size_t) const;

    ///
    bool set_string(size_t, const string&);

    ///
    void *raw_address(size_t) const;

    ///
    void *raw_address_if_exists(size_t) const;

    ///
    void *vector_address(size_t) const;

    ///
    void *vector_address_if_exists(size_t) const;

    ///
    void check_optimize(void*) const; // fake-const...

    ///
    void optimize(void*) const;

    ///
    size_t rss_size() const;

    ///
    bool prodquant(const string&);

    ///
    const bin_data *prodquant() const { return _prodquant; }

    ///
    inline bool is_valid_8(size_t a) const {
      return a+7<_size;
    }
    
    ///
    inline bool is_valid_8(size_t p, size_t i) const {
      size_t a = p + 8*i;
      return a+7<_size;
    }
    
    ///
    inline size_t& st(size_t p, size_t i) /*throw(logic_error)*/ {
      size_t a = p + 8*i;
      if (!is_valid_8(a))
	throw logic_error("bin_data::st()");
      
      return *(size_t*)((char*)_ptr+a);
    }
    
    ///
    inline const size_t& st(size_t p, size_t i) const /*throw(logic_error)*/ {
      return ((bin_data*)this)->st(p, i);
    }

    ///
    inline bool block_type_is_2(size_t p) const {
      return is_valid_8(p, 3) && st(p, 0)==2;
    }

    ///
    const vector<bool>& all_set_float() const;

    ///
    bool pack_to_file(const string&, bool) const;
    
    ///
    bool pack_to_memory(bin_data&, bool) const;

    ///
    bool unpack_to_file(const string&, bool) const;
    
    ///
    bool unpack_to_memory(bin_data&, bool) const;

    ///
    size_t block_type(size_t) const;
    
    ///
    size_t block_size(size_t) const;
    
    ///
    size_t block_nobjects(size_t) const;
    
    ///
    pair<size_t,size_t> block_minmax(size_t) const;
    
    ///
    size_t block_capacity(size_t) const;
    
    ///
    void *block_raw_address(size_t, size_t) const;
    
    ///
    bool add_indexed_block(size_t, size_t, 
			   const list<pair<size_t,const float*> >&);

    ///
    bool add_indexed_float(size_t, size_t, const vector<float>&);

    ///
    bool add_direct_block(size_t, size_t, 
			  const list<pair<size_t,const float*> >&);

  protected:
    ///
    header _header;

    ///
    string _filename;

    ///
    int _fd;

    ///
    size_t _size;

    ///
    bool _rw;

    ///
    void *_ptr;

    ///
    bin_data *_prodquant;

    ///
    bool _incore;

#ifdef BIN_DATA_USE_PTHREADS
    ///
    pthread_rwlock_t _rwlock;
#endif // BIN_DATA_USE_PTHREADS

    ///
    size_t _n_access;

    ///
    size_t _opt_interval;

    ///
    size_t _opt_rss_target;

    ///
    static bool _close_handles;

    ///
    vector<bool> _set_float;

    ///
    list<size_t> _blocks;
    
  }; // class bin_data

} // namespace picsom

#endif // _PICSOM_BIN_DATA_H_

