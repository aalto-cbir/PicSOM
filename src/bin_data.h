// -*- C++ -*-  $Id: bin_data.h,v 2.22 2017/05/09 10:16:17 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
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

#include <Util.h>

//#include <missing-c-utils.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

#ifndef NO_PTHREADS
#define BIN_DATA_USE_PTHREADS 1
#endif // NO_PTHREADS

namespace picsom {
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
      header() : version(1.0), hsize(sizeof(header)),
		 rlength_x(0), vdim(0), format_x(0) {
	memcpy(magic, "PSBD", 4);
	memset(dummy, 0, sizeof dummy);
      }

      ///
      string str() const {
	stringstream ss;
	ss << magic[0] << magic[1] << magic[2] << magic[3]
	   << " v" << version << " [" << hsize << "] len=" << rlength_x
	   << " dim=" << vdim << " format=0x" << hex << format_x;
	return ss.str();
      }

      ///
      static format_type target_format(format_type f) {
	return format_type(f&255);
      }

      ///
      format_type target_format() const {
	return target_format((format_type)format_x);
      }

      ///
      const string& target_format_str() const;

      ///
      static bool is_supported_format(format_type);

      ///
      bool is_supported_format() const {
	return is_supported_format((format_type)format_x);
      }

      ///
      static bool is_var_length_format(format_type f) {
	return f&format_var_length;
      } 

      ///
      bool is_var_length_format() const {
	return is_var_length_format((format_type)format_x);
      } 

      ///
      static bool is_expl_status_format(format_type f) {
	return f&format_expl_status;
      } 

      ///
      bool is_expl_status_format() const {
	return is_expl_status_format((format_type)format_x);
      } 

      ///
      static bool is_expl_index_format(format_type f) {
	return f&format_expl_index;
      } 

      ///
      bool is_expl_index_format() const {
	return is_expl_index_format((format_type)format_x);
      }

      ///
      static bool is_file_dict_format(format_type f) {
	return f&format_file_dict;
      } 

      ///
      bool is_file_dict_format() const {
	return is_file_dict_format((format_type)format_x);
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
	return is_var_length_format() ? 0 : rlength_x-payloadoffset();
      }

      ///
      char magic[4];      // offset=00 "PSBD" for "PicSOM binary data"
    
      ///
      float version;      // offset=04 currently always 1.0
      
      ///
      uint64_t hsize;     // offset=08 header size, currently always 64

      ///
      uint64_t rlength_x; // offset=16 size of a record, 4*vector dimensionality
      
      ///
      uint64_t vdim;      // offset=24 vector dimensionality
      
      ///
      uint64_t format_x;  // offset=32 for float ==1
      
      ///
      uint64_t dummy[3];  // offsets=40,48,56, filled with zeros
      
    }; // class bin_data::header

    ///
    bin_data(const string& fn = "", bool = false,
	     bin_data::header::format_type = header::format_undef,
	     size_t = 0, size_t = 0);

    ///
    ~bin_data();

    ///
    const header& get_header_real() const throw(logic_error) {
      string msg = "bin_data::get_header_real("+_filename+") : ";
      if (!is_ok())
	throw logic_error(msg+"bin_data not opened or too small");
      return *(header*)_ptr;
    }

    ///
    const header& get_header_copy() const throw(logic_error) {
      string msg = "bin_data::get_header_copy("+_filename+") : ";
      if (!is_ok())
	throw logic_error(msg+"bin_data not opened or too small");
      return _header;
    }

    ///
    string str() const {
      stringstream ss;
      if (is_ok()) {
	ss << get_header_copy().str();
	if (_fd>=0)
	  ss << " fd=" << _fd;
	if (_size) {
	  size_t b = componentsize();
	  long n = (_size-_header.hsize)/_header.rlength_x;
	  long m = (_size-_header.hsize)%_header.rlength_x;
	  ss << " size=" << _size << " (" << HumanReadableBytes(_size)
	     << ") [" << _header.target_format_str()
	     << ", " << n << " objects";
	  if (m)
	    ss << " + " << m << " extra bytes";
	  ss << ", " << b << " bit" << (b>1?"s":"") << "/comp";
	  if (_header.is_var_length_format())
	    ss << ", var_length";
	  if (_header.is_expl_status_format())
	    ss << ", expl_status";
	  if (_header.is_expl_index_format())
	    ss << ", expl_index";
	  if (_header.is_file_dict_format())
	    ss << ", file_dict";
	  if (_prodquant)
	    ss << ", prodquant vdim=" << vdim();
	  ss << "] " << (_rw?"rw":"ro");
	}
      }
      return ss.str();
    }

    ///
    const string& filename() const { return _filename; }

    ///
    bool is_open() const { return /*_fd>=0*/ _ptr; }
 
    ///
    bool is_incore() const { return _incore; }

   ///
    size_t rawsize() const { return _size; }

    ///
    size_t rawsize(size_t n) const { return _header.hsize+n*_header.rlength_x; }

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
    bool open(const string&, bool, header::format_type,
	      size_t, size_t) throw(string);

    ///
    bool open_inner(const string&, bool, header::format_type,
		    size_t, size_t) throw(string);

    ///
    bool resize(size_t, unsigned char = 255) throw(string);

    ///
    bool resize_inner(size_t, unsigned char = 255) throw(string);

    ///
    bool flush();

    ///
    bool close(bool) throw(string);

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
    bool set_float(size_t, const vector<float>&);

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
    pthread_rwlock_t rwlock;
#endif // BIN_DATA_USE_PTHREADS

    ///
    size_t _n_access;

    ///
    size_t _opt_interval;

    ///
    size_t _opt_rss_target;

    ///
    static bool close_handles;

  }; // class bin_data

} // namespace picsom

#endif // _PICSOM_BIN_DATA_H_

