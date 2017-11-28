// -*- C++ -*-  $Id: PicSOM.h,v 2.302 2017/08/16 07:18:08 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**\mainpage PicSOM content-based information retrieval system

   \section requires Requirements
   There are no special requirements except some ...

   \section installation Installation
   To appear...

   \section use Usage
   To appear...

*/   

#ifndef _PICSOM_H_
#define _PICSOM_H_

#include <Util.h>
#include <XMLutil.h>
#include <TimeUtil.h>
#include <RwLock.h>

#include <Segmentation.h>

#include <VectorSet.h>

#include <TicTac.h>
// #include <cox/tictac>

#include <uiart-state.h>

#include <limits>

#include <sys/types.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif // HAVE_PWD_H

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif // HAVE_SYS_SYSCALL_H

#undef PICSOM_DEVELOPMENT
#undef PICSOM_USE_CONTEXTSTATE

// #define PICSOM_USE_READLINE

#ifdef PICSOM_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif // PICSOM_USE_READLINE

#ifdef SIMPLE_USE_PTHREADS
#define PICSOM_USE_PTHREADS

// This is for Macs:
#ifndef PTHREAD_MUTEX_RECURSIVE_NP
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif // PTHREAD_MUTEX_RECURSIVE_NP

#endif // SIMPLE_USE_PTHREADS

#ifdef HAVE_WINSOCK2_H
#undef GetProp
#endif // HAVE_WINSOCK2_H

#if !SIMPLE_CLASS_PICSOM
#error SIMPLE_CLASS_PICSOM not defined
#endif

#define MAXARRAYSIZE 4096 
#define EVER ;;

// #define PICSOM_DEFAULT_DIRECTORY "/share/imagedb/picsom"
#define PICSOM_DEFAULT_PORT       5600
#define PICSOM_DEFAULT_SOAP_PORT 10000

#define RELEVANCE_SKIP   0
#define RELEVANCE_UP     1
#define RELEVANCE_DOWN   2

#undef DisplayString

#ifdef PICSOM_USE_READLINE
/**
   A global variable containing the command line history.
   Used by the GNU readline library.
*/
extern HIST_ENTRY **history_list ();
#endif // PICSOM_USE_READLINE

namespace picsom {
  using simple::Simple;
  using simple::IntVector;
  using simple::IntVectorSet;
  using simple::ListOf;

  static string PicSOM_h_vcid =
    "@(#)$Id: PicSOM.h,v 2.302 2017/08/16 07:18:08 jormal Exp $";

  extern const string PicSOM_C_vcid, picsom_C_vcid;

  class DataBase;
  class Connection;
  class Query;
  class Analysis;
  class TSSOM;
  class ground_truth;
  class RemoteHost;

  /////////////////////////////////////////////////////////////////////////////

  /// Structure for storing enum<->string conversion information, the old way.
  struct enum_info {
    const char *str;
    int type;
  };

  /// Structure for storing enum<->string conversion information, the new way.
  typedef map<int,pair<string,string> > enum_info_map;

  /// Structure for storing enum<->string conversion information, the new way.
  typedef pair<string,enum_info_map> enum_info_struct;

  /// Structure for storing for storing class names and ground truths.
  typedef map<string,pair<ground_truth,float> > aspect_map_t;

  /** documentation missing
   */ 
  enum object_type {
    ot_error = -1, ot_no_object,
    ot_cookie, ot_databaselist, ot_connectionlist, ot_connection,
    ot_querylist, ot_versions, ot_command, 
    ot_threadlist, ot_thread, ot_analysislist, ot_analysis,
    ot_slavelist, ot_slave, ot_status,                 ot_picsomlast=ot_status,

    ot_databaseinfo, ot_featurelist, ot_object, ot_thumbnail, ot_message,
    ot_plaintext, ot_objectinfo, ot_objectfileinfo, ot_frameinfo,
    ot_segmentinfo, ot_objecttext, ot_featurevector, ot_contentitems,
    ot_segmentationinfo, ot_segmentationinfolist, ot_segmentimg,
    ot_contextstate, ot_timeline,                   ot_dblast=ot_timeline,

    ot_featureinfo, ot_mapunitinfo, ot_classplot, ot_mapcollage,
    ot_umatimage,                                    ot_tssomlast=ot_umatimage,

    ot_queryinfo, ot_newidentity, ot_variables, ot_objectlist, 
    ot_ajaxresponse, ot_mapimage, ot_queryimage,    ot_querylast=ot_queryimage
  };

  /** documentation missing
   */ 
  enum target_type {
    target_error      = -1,
    target_no_target  = 0,
    target_file       = 1<<0, // modifier
    target_segment    = 1<<1, // modifier
    target_full       = 1<<2, // modifier
    target_virtual    = 1<<3, // modifier
    target_image      = 1<<4,
    target_imageset   = 1<<5,
    target_collage    = 1<<6, 
    target_gazetrack  = 1<<7,
    target_text       = 1<<8,
    target_html       = 1<<9,
    target_data       = 1<<10,
    target_message    = 1<<11,
    target_sound      = 1<<12,
    target_video      = 1<<13,
    target_link       = 1<<14,
    target_any_target = (1<<16)-1,
    //
    // then some combinations thereof:
    target_modifiers   = target_file|target_segment|target_full|target_virtual,
    target_real_types  = target_image|target_imageset|target_text|target_html|
    target_data|target_message|target_sound|target_video|
    target_link,
    //
    target_imagefile    = target_file|target_image,
    target_imagesetfile = target_file|target_imageset,
    target_textfile     = target_file|target_text,
    target_htmlfile     = target_file|target_html,
    target_datafile     = target_file|target_data,
    target_messagefile  = target_file|target_message,
    target_videofile    = target_file|target_video,
    target_soundfile    = target_file|target_sound,
    //
    // these are only partially supported for now...
    target_imagesegment    = target_segment|target_image,
    target_imagesetsegment = target_segment|target_imageset,
    target_textsegment     = target_segment|target_text,
    target_htmlsegment     = target_segment|target_html,
    target_messagesegment  = target_segment|target_message,
    target_videosegment    = target_segment|target_video,
    target_soundsegment    = target_segment|target_sound
  };

  /** documentation missing
   */ 
  enum select_type {
    select_undef,
    select_answer,
    select_question,
    select_show,
    select_map
  };

  /** documentation missing
   */ 
  enum msg_type {
    msg_undef,
    msg_acknowledgment,
    msg_response,
    msg_objectrequest,
    msg_upload,
    msg_slaveoffer,
    msg_command
  };
  
  /////////////////////////////////////////////////////////////////////////////

  /// Intermediate data structure used when uploading objects.
  class upload_object_data {
  public:
    upload_object_data() : nframes(0), framerate(0.0), fake(false),
			   nofile(false) {}
    string path;             //!< file name if the data not included in data.
    string data;             //!< binary content of the object.
    string use;              //!< "regular" or "segmentation"
    string ctype;            //!< MIME Content-Type like image/jpeg
    string auxid;            //!< auxiliary id for the object used somewehere
    string url;              //!< URL of the object itself
    string page;             //!< URL of the directory/page containg the object
    string date;             //!< date when inserted in origins file format
    string text;             //!< text to be insert in texts subdirectory
    string comprext;         //!< compression extension like ".gz" or ".bz2" 
    string dbname;           //!< file name after object has been added
    string colors;           //!< number of colors or >256
    string dimensions;       //!< WxH or WxHxnframes
    string checksum;         //!< MD5 checksum
    vector<string> keywords; //!< like "houses","cars","faces"
    string kw_comment;       //!< comment to added in the class files
    vector<size_t> indices;  //!< new indices for the extracted objects
    vector<string> labels;   //!< new labels for the extracted objects
    vector<size_t> featext;  //!< a subset of the one above to extract features
    size_t nframes;          //!< frame count in videos
    float framerate;         //!< frame rate in videos
    bool fake;               //!< set to true if just a placeholder
    bool nofile;             //!< set to true if target type should not contain file
    string forcedlabel;      //!< label to be used
  };

  /// Used when adding/inserting/uploading new objects ina DataBase.
  enum insertmode_t {
    insert_undef,        //<!
    insert_copy,         //<!
    insert_move,         //<!
    insert_relativelink, //<!
    insert_softlink,     //<!
    insert_hardlink      //<!
  };

  /** documentation missing
   */ 
  enum cbir_algorithm {
    cbir_no_algorithm,
    cbir_random,
    cbir_picsom,
    cbir_picsom_bottom,
    cbir_picsom_bottom_weighted,
    cbir_vq,
    cbir_vqw,
    cbir_sq,
    cbir_external
  };

  /** documentation missing
   */ 
  enum cbir_stage {
    stage_unknown = -1,
    stage_restrict,                       stage_first = stage_restrict,
    stage_enter,
    stage_initial_set,
    stage_check_image_count,
    stage_no_positives,
    stage_has_positives,
    stage_special_first_round,
    stage_expand_relevance,
    stage_run_per_map,
    stage_set_map_weights,
    stage_combine_unit_lists,
    stage_extract_images,
    stage_combine_image_lists,
    stage_remove_duplicates,
    stage_exchange_relevance,
    stage_select_images_to_process,
    stage_process_images,
    stage_converge_relevance,
    stage_final_select,
    stage_ready,
    stage_error,                           stage_last = stage_error
  };

  /** documentation missing
   */ 
  enum cbir_function {
    func_unknown = -1,                  // general
    func_error,
    func_true,
    func_default,

    func_restriction_spec,              // stage_restrict

    func_only_forced_images,            // stage_initial_set
    func_only_forced_images_user_test,
    func_top_levels_by_entropy,
    func_random_unseen_if_no_classmodels,
    func_random_unseen_images,
    func_random_positive_image,
    func_select_from_pre,

    func_goto_initial_set,              // stage_no_positives
    func_goto_has_positives,

    func_goto_expand_relevance,         // stage_has_positives 
    func_goto_run_per_map,              
    func_goto_special_first_round,

    func_all_unseen_images,             // stage_special_first_round
    func_limit_images_with_vq,

    func_expand,                        // stage_expand_relevance
    func_expand_up,   
    func_expand_down,
    func_expand_up_down,
    func_expand_down_up,
    func_expand_selective,
    
    func_exchange,                      // stage_exchange_relevance
    func_exchange_up,
    func_exchange_down,
    func_exchange_up_down,
    func_exchange_down_up,

    func_converge,                      // stage_converge_relevance
    func_converge_up,
    func_converge_down,
    func_converge_up_down,
    func_converge_down_up,

    func_per_map_picsom_all,            // stage_run_per_map 
    func_per_map_picsom_bottom,
    func_per_map_vq,
    func_goto_extract_images,

    func_one_for_all,                   // stage_set_map_weights
    func_one_for_bottom,
    func_const_for_bottom,
    func_distances_for_bottom,
    func_entropy_n_polynomial,
    func_entropy_n_leave_out_worst,

    func_offline_shortcut,               // stage_combine_unit_lists

    func_extract_images_picsom_all,     // stage_extract_images
    func_extract_images_picsom_bottom,
    func_extract_images_vq,
    func_extract_images_sq,

    func_remove_duplicates_picsom,      // stage_remove_duplicates
    func_remove_duplicates_vq,

    func_process_images_picsom_bottom,  // stage_process_images
    func_process_images_vq,             
    func_process_images_vqw
  };

  enum simplelist_type {
    simplelist_duplicates,
    simplelist_subobjects,
    simplelist_proper_persons,
    simplelist_proper_locations,
    simplelist_commons,
    simplelist_classfeatures
  };

  enum relop_type {
    relop_none,
    relop_default,

    // relevance up
    relop_sum,
    relop_max,
    relop_avg,

    // relevance down
    relop_copy,
    relop_divide
  };

  struct relop_stage_type {
    relop_type up;
    relop_type down;
  };

  /////////////////////////////////////////////////////////////////////////////

  /** documentation missing
   */ 
  extern enum_info *enum_object_type_info;

  /** documentation missing
   */ 
  extern enum_info *enum_target_type_info;

  /** documentation missing
   */ 
  extern enum_info *enum_select_type_info;

  /** documentation missing
   */ 
  extern enum_info *enum_msg_type_info;

  /////////////////////////////////////////////////////////////////////////////

  /// True if object_type is of PicSOM type.
  inline bool IsPicSOM(object_type o) {
    return o>ot_no_object && o<=ot_picsomlast; }

  /// True if object_type is of DataBase type.
  inline bool IsDataBase(object_type o) {
    return o>ot_picsomlast && o<=ot_dblast; }

  /// True if object_type is of TSSOM type.
  inline bool IsTSSOM(object_type o) {
    return o>ot_dblast && o<=ot_tssomlast; }

  /// True if object_type is of Query type.
  inline bool IsQuery(object_type o) {
    return o>ot_tssomlast && o<=ot_querylast; }

  /// Returns enum of string.
  int EnumInfo(enum_info*, const char*);

  /// Returns string of enum.
  const char *EnumInfoP(enum_info*, int, bool);

  /// Returns string of enum.
  inline string EnumInfo(enum_info *e, int o, bool w) {
    return EnumInfoP(e, o, w);
  }

  /// Returns object_type from string.
  inline object_type ObjectType(const char *s) {
    int r = EnumInfo(enum_object_type_info, s);
    return r==-1 ? ot_error : (object_type)r;
  }

  /// Returns object_type from string.
  inline object_type ObjectType(const string& s) {
    return ObjectType(s.c_str());
  }

  /// Returns string from object_type.
  inline const char *ObjectTypeP(object_type t, bool v = false) {
    return EnumInfoP(enum_object_type_info, t, v);
  }

  /// Returns string from object_type.
  inline string ObjectType(object_type t) {
    return EnumInfo(enum_object_type_info, t, true);
  }

  /// Converts eg. "image/jpeg" to target_type target_image+target_file.
  target_type MIMEtypeToTargetType(const string &s);

  /// Returns target_type from string.
  inline target_type TargetType(const char *s) {
    return (target_type)EnumInfo(enum_target_type_info, s);
  }

  /// Returns target_type from string.
  inline target_type TargetType(const string& s) {
    return TargetType(s.c_str());
  }

  /// Returns string from single-bit target_type.
  inline string TargetTypeBitName(target_type t) {
    const char *s = EnumInfoP(enum_target_type_info, t, false);
    return s ? s : "??tt??";
  }

  /// Returns msg_type from string.
  inline msg_type MsgType(const string& s) {
    return (msg_type)EnumInfo(enum_msg_type_info, s.c_str());
  }

  /// Returns string from msg_type.
  inline const char *MsgTypeP(msg_type t, bool v = false) {
    return EnumInfoP(enum_msg_type_info, t, v);
  }

  /// General string => enum conversion tool.
  inline int EnumInfoEnum(const enum_info_struct& eis,
			  const string& s) throw(invalid_argument) {
    const enum_info_map& map = eis.second;
    for (enum_info_map::const_iterator i = map.begin(); i!=map.end(); i++)
      if (i->second.first==s)
	return i->first;
    throw invalid_argument("enum string <"+s+"> not resolved in \""+
			   eis.first+"\"");
  }

  /// General enum => string conversion tool.
  inline const string& EnumInfoName(const enum_info_struct& eis,
				    int e) throw(invalid_argument) {
    const enum_info_map& map = eis.second;
    enum_info_map::const_iterator i = map.find(e);
    if (i!=map.end())
      return i->second.first;

    stringstream ss;
    ss << e;
    throw invalid_argument("enum int "+ss.str()+" not resolved in \""+
			   eis.first+"\"");
  }

  /// General enum => char conversion tool.
  inline const string& EnumInfoChar(const enum_info_struct& eis,
				    int e) throw(invalid_argument) {
    const enum_info_map& map = eis.second;
    enum_info_map::const_iterator i = map.find(e);
    if (i!=map.end())
      return i->second.second;

    stringstream ss;
    ss << e;
    throw invalid_argument("enum int "+ss.str()+" not resolved in \""+
			   eis.first+"\"");
  }

  /// For insertmode_t <=> string conversion.
  inline const enum_info_struct& InsertModeEnumInfo() {
    static enum_info_struct m;
    if (m.first=="") {
      m.first = "insertmode_enum_info";
      typedef pair<string,string> p;
      m.second[insert_undef]        = p("undef",    	"?");
      m.second[insert_copy]         = p("copy",     	"c");
      m.second[insert_move]         = p("move",     	"m");
      m.second[insert_relativelink] = p("relativelink", "r");
      m.second[insert_softlink]     = p("softlink",     "s");
      m.second[insert_hardlink]     = p("hardlink",     "h");
    }
    return m;
  }

  /// Conversion between string => insertmode_t
  inline insertmode_t InsertModeName(const string& m) {
    try {
      return (insertmode_t) EnumInfoEnum(InsertModeEnumInfo(), m);
    }
    catch (const invalid_argument& err) {
      Simple::ShowError("InsertModeName(const string&) : [", err.what(), "]");
      return insert_undef;
    }
  }

  /// Conversion between insertmode_t => string
  inline const string& InsertModeName(insertmode_t m) {
    try {
      return EnumInfoName(InsertModeEnumInfo(), m);
    }
    catch (const invalid_argument& err) {
      Simple::ShowError("InsertModeName(insertmode_t) : [", err.what(), "]");
      static const string dummy = "???";
      return dummy;
    }
  }

  /// For select_type <=> string conversion.
  inline const enum_info_struct& SelectTypeEnumInfo() {
    static enum_info_struct m;
    if (m.first=="") {
      m.first = "select_type_enum_info";
      typedef pair<string,string> p;
      m.second[select_undef]    = p("undef",    "?");
      m.second[select_answer]   = p("answer",   "A");
      m.second[select_question] = p("question", "Q");
      m.second[select_show]     = p("show",     "S");
      m.second[select_map]      = p("map",      "M");
    }
    return m;
  }

  /// Conversion between string => select_type
  inline select_type SelectTypeName(const string& m) {
    try {
      return (select_type) EnumInfoEnum(SelectTypeEnumInfo(), m);
    }
    catch (const invalid_argument& err) {
      Simple::ShowError("SelectTypeName(const string&) : [", err.what(), "]");
      return select_undef;
    }
  }

  /// Conversion between select_type => string
  inline const string& SelectTypeName(select_type m) {
    try {
      return EnumInfoName(SelectTypeEnumInfo(), m);
    }
    catch (const invalid_argument& err) {
      Simple::ShowError("SelectTypeName(select_type) : [", err.what(), "]");
      static const string dummy = "???";
      return dummy;
    }
  }

  /// Conversion between select_type => string
  inline const string& SelectTypeChar(select_type m) {
    try {
      return EnumInfoChar(SelectTypeEnumInfo(), m);
    }
    catch (const invalid_argument& err) {
      Simple::ShowError("SelectTypeChar(select_type) : [", err.what(), "]");
      static const string dummy = "???";
      return dummy;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////

  /// Solves all the '+' separated target types from the given string.
  target_type SolveTargetTypes(const string& s, bool warn = true);

  inline string TargetTypeString(target_type tt, bool esc = false) {
    if (int(tt)<0) {
      Simple::ShowError("TargetTypeString() : target_type < 0");
      return "";
    }

    if (tt==target_no_target)
      return "no_target";

    if (tt==target_any_target)
      return "any_target";

    string ret, sep = esc ? "_plus_" : "+";
    for (size_t i=0; i<8*sizeof(tt); i++) {
      int b = int(tt)&(1<<i);
      if (b)
	ret += (ret=="" ? "" : sep) + TargetTypeBitName(target_type(b));
    }
    return ret!="" ? ret : "?????";
  }

  /// True if ORed aka '+' separated type.
  inline bool IsCombined(target_type tt) {
    bool first = false;
    for (size_t i=0; i<8*sizeof(tt); i++)
      if (int(tt)&(1<<i)) {
	if (first)
	  return true;
	else
	  first = true;
      }

    return false;
  }

  /// All target_type:s separated.
  inline vector<target_type> TargetTypeSet(target_type tt) {
    vector<target_type> v;
    for (size_t i=0; i<8*sizeof(tt); i++) {
      int b = int(tt)&(1<<i);
      if (b)
	v.push_back(target_type(b));
    }
    return v;
  }

  ///
  typedef list<pair<string,string> > script_exp_t;
  
  ///
  typedef pair<string,pair<list<string>,vector<string> > > script_list_e;

  ///
  typedef list<script_list_e> script_list_t;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /**
     The PicSOM class contains the functionality for the PicSOM 
     image retrieval system. 

     The class is divided into several member classes which are responsible
     for individual tasks such as query processing, database management etc.

     Command line arguments:
     @verbinclude cmdline-io.dox
   
     @short A class implementing the PicSOM engine. 
     @version $Id: PicSOM.h,v 2.302 2017/08/16 07:18:08 jormal Exp $
  */
  class PicSOM : public Simple {
  public:
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    /**
       The constructor for the PicSOM class.
       @param arg
    */
    PicSOM(const vector<string>& arg);

    /// The destructor.
    virtual ~PicSOM();
  
    /// Dump is required in all descendants of Simple.
    virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;
  
    /// SimpleClassNames is required in all descendants of Simple.
    virtual const char **SimpleClassNames() const { static const char *n[] = {
	"PicSOM", NULL }; return n; }
  
    /// Simple interface to C main().
    static int Main(int argc, const char **argv, PicSOM** = NULL);

    /// Inner main().
    static int Main(const vector<string>&, PicSOM** = NULL);

    ///
    static int Usage(const string& prog, const string& msg="");

    /// Version string from vcid.
    static string PicSOM_C_Version() { return ExtractVersion(PicSOM_C_vcid); }

    /// Version string from vcid.
    static string PicSOM_h_Version() { return ExtractVersion(PicSOM_h_vcid); }

    /// Version string from vcid.
    //static string picsom_C_Version() { return ExtractVersion(picsom_C_vcid); }

    /// Version string from vcid. Delete after use!
    static string ExtractVersion(const string&);

    /// Filled in by release/build script.
    static string Release() { return "" "picsom-0.36" ; }

    ///
    static bool HasFeaturesInternal() { return has_features_internal; }

    ///
    static bool HasSegmentationInternal() { return has_segmentation_internal; }

    ///
    static bool HasImgRobotInternal() { return has_imgrobot_internal; }

    /// 0=0, 1=if, 2=yes
    size_t GpuPolicy() const { return gpupolicy; }

    ///
    void GpuPolicy(size_t p) { gpupolicy = p; }

    /// -1=none, 0,1,... =deviceid
    int GpuDeviceId() const { return gpupolicy ? gpudevice : -1; }

    ///
    int SolveGpuDevice();

    ///
    pair<bool,vector<string> > ShellExecute(const vector<string>&, bool, bool);

    /// Maps all getenv("PICSOM_XXX") to env["XXX"].
    void CreateEnvironment();

    /// Access to environment type variables.
    const map<string,string>& Environment() const { return env; }

    /// Name of the owner of the process.
    const string& UserName() const {
      static string uname;
      if (uname=="") {
	if (env.find("USER")!=env.end())
	  uname = env.find("USER")->second;
	else {
	  passwd *pw = getpwuid(getuid());
	  uname = pw ? pw->pw_name : "";
	}
      }
      return uname;
    }

    /// Process owner's home dir.
    const string& UserHomeDir() const {
      static string udir;
      if (udir=="") {
	if (env.find("HOME")!=env.end())
	  udir = env.find("HOME")->second;
	else {
	  passwd *pw = getpwuid(getuid());
	  udir = pw ? pw->pw_dir : "";
	}
      }
      return udir;
    }

    /// Process current working directory.
    static string Cwd() {
      char cwdbuf[2048];
      const char *wd = getcwd(cwdbuf, sizeof cwdbuf);
      return wd ? wd : "";
    }

    /// Name of the executing host.
    static const string& HostName(bool = false);

    /// IP address of the executing host.
    static const string& HostAddr(const string& = "");

    /// Number of CPUs.
    static int CpuCount();

    /// Estimates CPU usage percent
    double CpuUsage();

    /// Returns name by which the program was invoked.
    const string& MyName() const { return myname_str; }

    /// Returns name of the binary with which the program was invoked.
    const string& MyBinary() const { return mybinary_str; }

    /// Access to master_address, useful for rootless slaves.
    const string& MasterAddress() const { return master_address; }

    /// Access to master_auth_token, useful for rootless slaves.
    const string& MasterAuthToken() const { return master_auth_token; }

    /// Process switches from shell and sets rests in cmdline_args.
    bool CommandLineParsing(const vector<string>&);

    /// Parses arguments from cmdline_args or from file to set start_XXX.
    void SolveStartUpMode();

    /// A helper utility for the above one.
    string SolveStartUpModeInner(const string&);

    /// This gets called right after constructor.
    int Execute(int, const char**);

    /// This is it, cannot say more.
    bool MainLoop();

    ///
    void Quit() { quit = true; }

#ifdef PICSOM_USE_PTHREADS
    /// Called when in operation_analyse mode and has listening connection.
    bool PthreadMainLoop();

    /// Called by PthreadMainLoop().
    static void *MainLoopPthreadCall(void*);

    ///
    bool StartMPIListenThread();

    /// Called by PthreadMPILoop().
    static void *MPILoopPthreadCall(void*);

#endif // PICSOM_USE_PTHREADS

    ///
    bool OctaveInializeOnce();

    ///
    bool OctaveAddPath(const string&, bool, bool);

    ///
    const string& SqlServer() const { return sqlserver; }

    ///
    bool TempSqliteDB() const { return tempsqlitedb; }

    /// Hints to novices and otherwise innocent.
    void ShowUsage(bool = true);

    /// Returns true if cin can be read.
    bool HasCin() const { return has_cin; }

    /// Returns true if cin can be read.
    void HasCin(bool h) { has_cin = h; }

    ///
    static const char *SeenCharacterP(double v) {
      return v<0 ? "-" : v>0 ? "+" : "0";
    }

    ///
    void MarkSeen(char, const char*);

    ///
    IntVectorSet RandomUnseenLabeledMapIndices(int) const;

    ///
    IntVector AllPositiveConvolvedUnseenMapIndices(int, int) const;

    ///
    IntVector BestPositiveConvolvedUnseenMapIndices(int, int, int) const; 

    ///
    IntVectorSet BestPositiveConvolvedUnseenMapIndices(int, int = 0) const;

    ///
    IntVector AllPositiveSeenMapIndices(int, int) const;

    /// Forks and runs child in background.
    bool RunInBackGround() {
      if (!allow_fork) {
	fork_faked = true;
	return true;
      }
      int p = fork();
      if (p>0)
	_exit(0); // this should be the only place where is _exit()...
      return p==0;
    }

    /// Waits for the correct moment to fork new instances.
    bool DoGuarding();

    /// Restricts newly-forked server instance's resource limits.
    bool SetChildLimits();

    /// Restricts process resource ilmits.
    bool SetLimit(const string&, const string&);

    ///
    bool SetUnLimited();

    /// Forwards listening socket to a remote host.
    bool DoSshForwarding(int);

    /// Common data type for storing info on running pthreads.
    typedef struct {
      string    name;
      pthread_t id;
      pid_t     tid;
      bool      thread_set;
      string    phase;
      string    type;
      string    text;
      string    slave_state;
      void     *object;
      void     *data;
      string    showname;
    } thread_info_t;

    /// Info on running pthreads.
    typedef list<thread_info_t> thread_list_t;

    /// Slave stuff.
    class slave_info_t {
    public:
      ///
      slave_info_t(const string& h, const string& e) :
	parent(NULL),
	hostspec(h), executable(e), status("uninit"), pid(0),
	load(-1), cpucount(0),cpuusage(-1),
	n_slaves_now(0), n_slaves_started(0), n_slaves_contacted(0),
	max_slaves_start(1), max_slaves_add(0),
	max_tasks_par(0), max_tasks_tot(0),
	n_tasks_par(0), n_tasks_tot(0), n_tasks_fin(0),
	pending_threads(0), running_threads(0), max_time(0),
	conn(NULL), status_requested(false) {
	struct timespec ts;
	Simple::ZeroTime(ts);
	submitted = started = task_started = task_finished =
	  updated = spare = ts;
      }

      ///
      slave_info_t(const string& h, Connection *c) :
	parent(NULL),
	hostspec(h), status("connected"), pid(0),
	load(-1), cpucount(0), cpuusage(-1),
	n_slaves_now(0), n_slaves_started(0), n_slaves_contacted(0),
	max_slaves_start(0), max_slaves_add(0),
	max_tasks_par(0), max_tasks_tot(0),
	n_tasks_par(0), n_tasks_tot(0), n_tasks_fin(0),
	pending_threads(0), running_threads(0), max_time(0),
	conn(c), status_requested(false) {
	struct timespec ts;
	Simple::ZeroTime(ts);
	submitted = started = task_started = task_finished =
	  updated = spare = ts;
      }

      ///
      void RwLockIt()   { lock.LockWrite();   }

      ///
      void RwUnlockIt() { lock.UnlockWrite(); }

      ///
      bool Locked() const { return lock.Locked(); }

      ///
      bool ThisThreadLocks() const { return lock.ThisThreadLocks(); }

      ///
      pthread_t Locker() const { return lock.Locker(); }

      ///
      string LockerStr() const { return lock.LockerStr(); }

      ///
      RwLock lock;

      ///
      slave_info_t *parent;

      ///
      string hostspec;

      ///
      string hostname;

      ///
      string executable;

      ///
      vector<string> command;

      ///
      string status;

      ///
      vector<string> keys;

      ///
      vector<string> jobids;

      ///
      pid_t pid;

      ///
      double load;

      ///
      size_t cpucount;

      ///
      double cpuusage;

      ///
      size_t n_slaves_now;

      ///
      size_t n_slaves_started;

      ///
      size_t n_slaves_contacted;

      ///
      size_t max_slaves_start;

      ///
      size_t max_slaves_add;

      ///
      size_t max_tasks_par;

      ///
      size_t max_tasks_tot;

      ///
      size_t n_tasks_par;

      ///
      size_t n_tasks_tot;

      ///
      size_t n_tasks_fin;

      ///
      size_t pending_threads;

      ///
      size_t running_threads;

      ///
      size_t max_time;

      ///
      struct timespec submitted;

      ///
      struct timespec started;

      ///
      struct timespec task_started;

      ///
      struct timespec task_finished;

      ///
      struct timespec updated;

      ///
      struct timespec spare;

      ///
      Connection *conn;

      ///
      thread_list_t threads;

      ///
      bool status_requested;

    }; // class slave_info_t

    /// Slave stuff.
    typedef list<slave_info_t> slave_list_t;

    ///
    bool UseMpiSlaves() const { return use_mpi_slaves; }

    ///
    static void ForceFastSlave(bool f) { force_fast_slave = f; }

    ///
    static bool ForceFastSlave() { return force_fast_slave; }

    /// Sets all things ready for a slave process.
    bool PrepareSlave(const string&);

    /// Returns true if this is a server instance.
    bool IsServer() const { return start_online; }

    /// Returns true if this is a slave instance.
    bool IsSlave() const { return is_slave; }

    ///
    bool ExpectSlaves() const { return expect_slaves; }

    /// Returns true if this instance has slaves.
    bool HasSlaves() const { return slave_list.size(); }

    /// Returns true if this instance has free slaves.
    bool HasFreeSlaves() const;

    /// Setups slave information.
    bool SetSlaveInfo(const string&);

    /// A dumping utility.
    void ShowSlaveInfoList() const;

    /// Stringified contents of slave_info_t.
    string SlaveInfoString(const slave_info_t&, bool) const;

    /// Starts up all slaves.
    bool StartSlaves(size_t);

    /// Creates a slave instance.
    bool CreateSlave(slave_info_t&, bool, const string&, const string&);

    /// Creates a slave instance.
    bool CreateSlavePartOne(slave_info_t&, bool, const string&, const string&);

    /// Creates a slave instance.
    bool CreateSlavePartTwo(slave_info_t&);

    /// Called after <slaveoffer> message.
    bool TakeSlave(Connection*, const string&, const string&);

    /// Called after <slaveoffer> message.
    bool TakeSlaveInner(Connection*, const string&, const string&);

    /// Picks one slave for use.
    slave_info_t *SelectSlave(bool, double, const string&, const string&,
			      size_t, bool&);
  
    /// Picks one slave for use.
    slave_info_t *SelectSlaveInner(bool, double, const string&, const string&,
				   size_t, bool&);
  
    /// Updates all slaves' information.
    bool UpdateSlaveInfo();

    ///
    bool UpdateSlaveInfoNewest();

    /// Updates all slaves' information.
    bool UpdateSlaveInfoRatherOld();

    /// Updates all slaves' information.
    bool UpdateSlaveInfoOld();

    /// Updates slave's information.
    bool UpdateSlaveInfo(slave_info_t&);

    /// Updates slave's information.
    bool UpdateSlaveInfoWrite(slave_info_t&, bool = true);

    /// Updates slave's information.
    bool UpdateSlaveInfoRead(slave_info_t&, bool = true);

    /// Helper for the above one.
    bool UpdateSlaveThreadInfo(thread_info_t&, const XmlDom&);

    ///
    bool SetSlaveToLimbo(slave_info_t&, const string&);

    ///
    map<string,size_t> SlaveStatistics() const;

    /// Returns number of slave hosts where object* matches the argument.
    size_t NslaveHostsContacted();

    /// Returns number of slave hosts where object* matches the argument.
    size_t NslaveHostsAlive(void *p) {
      return NslavesCommon(p, false, "");
    }

    /// Returns number of slave hosts where object* matches the argument.
    size_t NslaveHostsAlsoLimbo(void *p) {
      return NslavesCommon(p, false, "limbo");
    }

    /// Returns number of slave hosts where object* matches the argument.
    size_t NslaveHostsAlsoTerminated(void *p) {
      return NslavesCommon(p, false, "terminated");
    }

    /// Returns number of slave threads where object* matches the argument.
    size_t NslaveThreadsAlive(void *p) {
      return NslavesCommon(p, true, "");
    }

    /// Returns number of slave threads where object* matches the argument.
    size_t NslaveThreadsAlsoLimbo(void *p) {
      return NslavesCommon(p, true, "limbo");
    }

    /// Returns number of slave threads where object* matches the argument.
    size_t NslaveThreadsAlsoTerminated(void *p) {
      return NslavesCommon(p, true, "terminated");
    }

    /// Returns number of slave hosts/threads where object* matches the argument.
    size_t NslavesCommon(void*, bool, const string&);

    ///
    const slave_info_t *SlaveInfo(size_t i) const {
      size_t k = 0;
      for (auto j=slave_list.begin(); k<=i && j!=slave_list.end(); j++, k++)
	if (k==i)
	  return &*j;
      return NULL;
    }

    /// Returns the first non-running thread where object* matches the argument.
    //pair<slave_info_t*,thread_info_t*> FirstNonRunningSlaveThread(void*);

    /// Returns the first non-running thread where object* matches the argument.
    //pair<slave_info_t*,thread_info_t*>
    //FirstNonPendingNonRunningSlaveThread(void*);

    /// Returns the first non-running thread where object* matches the argument.
    //pair<slave_info_t*,thread_info_t*>
    //FirstNonPendingNonRunningNonErroredSlaveThread(void*);

    /// Returns the first non-running thread where object* matches the argument.
    //pair<slave_info_t*,thread_info_t*>
    //FirstNonPendingNonRunningNonErroredNonLimboSlaveThread(void*);

    ///
    pair<slave_info_t*,thread_info_t*> SlaveThreadWithResults(void*);

    /// Notifies about limbo or terminated slaves.
    bool PossiblyDecreaseSlaveCount(slave_info_t&);

    /// Starts a new slave if parent spec allows.
    bool PossiblyStartNewSlaves(slave_info_t&);

    /// Helper for PossiblyStartNewSlaves()
    bool CheckSlaveCount(slave_info_t&);

    /// Helper for PossiblyStartNewSlaves()
    bool CheckSlaveCountSlurm(slave_info_t&);

    /// Helper for PossiblyStartNewSlaves()
    bool CheckSlaveCountCondor(slave_info_t&);

    /// Removes the given slave thread.
    bool RemoveSlaveThread(const pair<slave_info_t*,thread_info_t*>&);

    /// Removes the given slave thread.
    bool RemoveSlaveThreadInner(const pair<slave_info_t*,thread_info_t*>&);

    /// Terminates slave if it seems to running out of execution time.
    bool PossiblyTerminateSlave(slave_info_t&);

    /// Terminates slave if it seems to running out of execution time.
    bool TerminateSlave(slave_info_t&, const string&);

    ///
    const string& SlavePipe() const { return slavepipe; }

    ///
    bool IsSlavePiping(const string& s) const {
      return IsSlave() && slavepipe.find(s)!=string::npos;
    }

    ///
    list<string> SbatchExclude() const;

    ///
    void Quiet(bool q) { quiet = q; }
    
    /// Returns true if no output is desired.
    bool Quiet() const { return quiet; }

    /// Logging routine to be called from all classes.
    void WriteLogCommon(const char*, const char*, const char* = NULL,
			const char* = NULL, const char* = NULL,
			const char* = NULL) const;

    /// Logging routine to be called from all classes.
    void WriteLogCommon(const char*, ostringstream&) const;

    /// Logging routine to be called from all classes.
    void WriteLogStr(const string& s1, const string& s2,
		     const string& s3 = "", const string& s4 = "",
		     const string& s5 = "", const string& s6 = "") const {
      WriteLogCommon(s1.c_str(), s2.c_str(), s3.c_str(),
		     s4.c_str(), s5.c_str(), s6.c_str()); }

    /// Logging routine to be called from all classes.
    void WriteLogStr(const string& s, ostringstream& os) const {
      WriteLogStr(s.empty() ? "!!!" : s, os.str()); }

    /// Mutual exclusion from log stream.
    void LogMutexLock() const {
#ifdef PICSOM_USE_PTHREADS
      pthread_mutex_lock(&((PicSOM*)this)->log_mutex);
#endif // PICSOM_USE_PTHREADS
    }

    /// Clear mutual exclusion from log stream.
    void LogMutexUnLock() const {
#ifdef PICSOM_USE_PTHREADS
      pthread_mutex_unlock(&((PicSOM*)this)->log_mutex);
#endif // PICSOM_USE_PTHREADS
    }

    /// Mutual exclusion from all process-wide operations.
    void MutexLock() {
#ifdef PICSOM_USE_PTHREADS
      pthread_mutex_lock(&mutex);
#endif // PICSOM_USE_PTHREADS
    }

    /// Clear mutual exclusion from all process-wide operations.
    void MutexUnlock() {
#ifdef PICSOM_USE_PTHREADS
      pthread_mutex_unlock(&mutex);
#endif // PICSOM_USE_PTHREADS
    }

    /// Sets logging mode w/wo timestamp, w/wo identity part.
    void LoggingMode(const string& s) { 
      bool stamp = false, id = false;
      if (s.find("stamp")!=string::npos || s.find("time")!=string::npos)
	stamp = true;
      if (s.find("id")!=string::npos)
	id = true;
    
      LoggingMode(stamp, id);
    }

    /// Sets logging mode w/wo timestamp, w/wo identity part.
    void LoggingMode(bool t, bool i) { log_timestamp = t; log_identity = i; }

    /// Sets logging mode to WITH timestamp WITH identity.
    void LoggingModeTimestampIdentity() { LoggingMode(true, true); }

    /// Sets logging mode to WITHOUT timestamp WITHOUT identity.
    void LoggingModePlain() { LoggingMode(false, false); }

    /// Read access to the state of timestamping on/off.
    bool UseTimeStamp() const { return log_timestamp; }

    ///
    typedef struct {
      clock_t real;
      struct tms cpu;
    } TimeType;

    /// Starts cpu timing for given function.
    void Tic(const char *f) { tics.Tic(f); }

    /// Stops cpu timing for given function.
    void Tac(const char *f = NULL) { tics.Tac(f); }

    /// Gives reference to the TicTac object.
    TicTac *GetTicTac() { return &tics; }

    /// 
    void StartTimes();

    /// Lets tictac to show what it has gathered.
    void ShowTimes(bool cum = true, bool diff = true,
		   ostream& os = cout) { tics.Summary(cum, diff, os); }

    /// Shows debug information if we are tracing them.
    void PossiblyShowDebugInformation(const string& m = "", bool end = false) {
      if (quiet)
	return;
      PossiblyShowTimes__(m, end);
      PossiblyDumpMemoryUsage__(m, end);
    }

    /// Shows times if we are tracing them.
    void PossiblyShowTimes__(const string& m = "", bool end = false) {
      bool cum = end || (debug_times&1), diff = debug_times&2;
      if (diff || cum) {
	if (m!="")
	  cout << m << " ";
	ShowTimes(cum, diff);
      }
    }

    /// Shows memory usage if we are tracing it.
    void PossiblyDumpMemoryUsage__(const string& m = "", bool end = false) {
      if (end || debug_mem>1) {
	if (m!="")
	  cout << m << " ";
	DumpMemoryUsage();
      }
    }

    ///
    const string& Path() const { return path_str; }

    ///
    void Path(const string& p) {
      path_str = p;
      for (int i = path_str.size()-1; i>=0 && path_str[i]=='/'; i--)
	path_str.erase(i);
    }

    ///
    bool NoRootDir() const { return norootdir; }

    /// Expands to ".../databases/my_db/xxx/yyy".
    string ExpandDataBasePath(const string& db, const string& s = "",
			      const string& t = "", bool all = false) const;

    /// Returns true if such database exists.
    bool DataBaseExists(const string& name);

    /// Reads in database or creates it if not already read.
    DataBase *FindDataBaseEvenNew(const string&, bool);

    /// Reads in database if not already read.
    DataBase *FindDataBase(const string& n, const string& v = "",
			   bool all = false, bool warn = true);

    /// Download sufficient files to find a DataBase,
    bool DownloadDataBase(const string&, const string&);

    /// Downloads something like "share/lscom/lscom.map"
    bool DownloadFile(const string&) const;

    /// Reads in stubs for all databases.
    void FindAllDataBases(bool = false);

    /// Returns the number of databases.
    int NdataBases() const { return database.Nitems(); }

    /// Returns the databases based on number.
    const DataBase *GetDataBase(int i) const { return database.Get(i); }

    /// Returns the databases based on number.
    DataBase *GetDataBase(int i) { return database.Get(i); }

    /// Deletes a database from memory, not from disk.
    bool DeleteDataBase(DataBase*);  

    /// Seeks for views under database directory.
    //void FindDataBaseViews(const DataBase*);

    /// This is called by Analysis::ProcessArguments().
    template <class ITER>
    bool Interpret(ITER i, const ITER& e, Query *q, Analysis *a, TSSOM *t) {
      for (; i!=e; i++) {
	const string& k = i->first, v = i->second;
	bool res = DoInterpret(k, v, this, q, NULL, NULL, a, t, false, true);
	if (!res)
	  return ShowError("PicSOM::Interpret(", k, "=", v, ") failed");
      }
      return true;
    }

    /// This is the mother of all Interpret()s.
    static bool DoInterpret(const string&, const string&,
			    PicSOM*, Query*, const Connection*, Connection*,
			    Analysis*, TSSOM*, bool, bool);

    /// This tries to solve a key=value pair in main body.
    /// returns true if key was recognized, false otherwise.
    /// third argument is 1 for OK, 0 for ERROR, -1 for undefined.
    bool Interpret(const string&, const string&, int&);

    ///
    bool StoreDefaultKeyValue(const string& t, const string& k, const string& v) {
      default_key_value[t].push_back(make_pair(k, v));
      return true;
    }

    ///
    bool StoreDefaultKeyValue(const string& t, const string& kv);

    ///
    bool StoreDefaultKeyValue(const string& s) {
      size_t x = s.find("::");
      if (x==string::npos)
	return ShowError("StoreDefaultKeyValue("+s+") failed due no ::");

      return StoreDefaultKeyValue(s.substr(0, x), s.substr(x+2));
    }

    ///
    list<pair<string,string> > DefaultKeyValues(const string& t) const {
      map<string,list<pair<string,string> > >::const_iterator i
	= default_key_value.find(t);
      return i==default_key_value.end() ? list<pair<string,string> >()
	: i->second;
    }

    /// Shows all connections.
    void ShowConnections(Connection*, ostream*) const;

    /// Shows all connections.
    void ShowConnections(Connection& c) const { ShowConnections(&c, NULL); }  

    /// Shows all connections.
    void ShowConnections() const { ShowConnections(NULL, &cout); } 

    /// Shows all queries.
    void ShowQueries(Connection&) const;      

    /// Shows all databases.
    bool ShowDataBases(Connection&, const Query&, bool = true,
		       const char* = NULL);

    /// Shows database search path.
    void ShowPath(Connection&) const;

    /// Shows command help.
    void ShowHelp(Connection&) const;

    /// Shows command history.
    void ShowHistory(Connection&) const;

    /// Gives pointer to stored query if found in memory.
    Query *FindQueryInMemory(const string&) const;

    /// Gives pointer to stored query if found in memory (searches by username).
    /// Second argument is the number of query (of the list queries of username)
    /// is wanted
    Query *FindNthQueryInMemory(const string&, size_t) const;

    /// Gives pointer to stored query if found.
    Query *FindQuery(const string&, const Connection*, Analysis* = NULL);

    /// Gives pointer to stored query if found or returns NULL.
    const Query *FindQuery(const Query *q) const {
      RwLockReadQueryList();
      const Query *r = NULL;
      for (size_t i=0; !r && i<query.size(); i++)
	if (query[i]==q)
	  r = q;
      RwUnlockReadQueryList();
      return r;
    }
  
    ///
    void RwLockReadQueryList()    const { RwLockRead(querylistlock); }

    ///
    void RwUnlockReadQueryList()  const { RwUnlockRead(querylistlock); }

    ///
    void RwLockWriteQueryList()         { RwLockWrite(querylistlock); }

    ///
    void RwUnlockWriteQueryList()       { RwUnlockWrite(querylistlock); }

    ///
    void RwLockReadSlaveList()    const { RwLockRead(slavelistlock); }

    ///
    void RwUnlockReadSlaveList()  const { RwUnlockRead(slavelistlock); }

    ///
    void RwLockWriteSlaveList()         { RwLockWrite(slavelistlock); }

    ///
    void RwUnlockWriteSlaveList()       { RwUnlockWrite(slavelistlock); }

#ifdef PICSOM_USE_PTHREADS
    ///
    void RwLockRead(const RwLock& rwlock)    const { rwlock.LockRead(); }

    ///
    void RwUnlockRead(const RwLock& rwlock)  const { rwlock.UnlockRead(); }

    ///
    void RwLockWrite(      RwLock& rwlock)         { rwlock.LockWrite(); }

    ///
    void RwUnlockWrite(      RwLock& rwlock)       { rwlock.UnlockWrite(); }

#else
    void RwLockRead(  const bool&) const {}
    void RwUnlockRead(const bool&) const {}
    void RwLockWrite(       bool&)       {}
    void RwUnlockWrite(     bool&)       {}
#endif // PICSOM_USE_PTHREADS

    /// Returns http://hostname:port if listen_connection is set.
    string HttpListenConnection() const;

    /// Sets the range of ports to listen() for connections.
    void ListenPort(int a = PICSOM_DEFAULT_PORT, int b = PICSOM_DEFAULT_PORT) {
      listen_porta = a;
      listen_portb = b;
    }

    /// Sets the listening ports to a named range.
    bool ListenPort(const string& n, bool only_one = false);

    ///
    pair<int,int> GetListenPortRange() {
      pair<int,int> p(listen_porta, listen_portb);
      return p;
    }

    /// A generated function that resides in userports.C
    static bool UserPorts(const string& n, int& a, int &b);

    /// Returns the number of connections.
    size_t Connections(bool only_non_closed, bool only_selectable) const;

    /// Returns the number of connections not yet closed.
    size_t OpenConnections() const { return Connections(true, false); }

    /// Returns the number of connections MainLoop() may select.
    size_t SelectableConnections() const { return Connections(false, true); }

    ///
    size_t MPIconnections() const;

    /// Pointer to a Connection object.
    Connection *GetConnection(size_t i) {
      return i<connection.size() ? connection[i] : NULL;
    }

    /// Pointer to a Connection object.
    const Connection *GetConnection(size_t i) const {
      return ((PicSOM*)this)->GetConnection(i);
    }

    /// Adds a new connection.
    Connection *AppendConnection(Connection *c) {
      if (c)
	connection.push_back(c);
      return c;
    }

    /// Closes a connection and removes it from the list.
    bool CloseConnection(Connection *c, bool, bool = true, bool = true);

    static bool HasTerminalInput();

    /// Creates a connection which is appended to the list.
    Connection *CreateTerminalConnection();

    /// Creates a connection which is appended to the list.
    Connection *CreateFileConnection(const char *n);

    /// Creates a connection which is appended to the list.
    Connection *CreateStreamConnection(istream& s);

    /// Creates a connection which is appended to the list.
    Connection *CreatePipeConnection(const vector<string>&, bool = false);

    /// Creates a connection which is appended to the list.
    Connection *CreateMPIListenConnection();

    /// Creates a connection which is appended to the list.
    Connection *CreateMPIConnection(const string&, bool);

    /// Creates a connection which is appended to the list.
    Connection *CreateListenConnection(int a, int b);

    /// Creates a connection which is appended to the list.
    Connection *CreateSocketConnection(int i, const RemoteHost& h);

    /// Creates an uplink which is appended to the list.
    Connection *CreateUplinkConnection(const string& address,
				       bool do_conn, bool do_ack);

    /// Creates an HTTP client connection which is appended to the list.
    Connection *CreateHttpClientConnection(const string&, bool = false, int = 0);

    /// Creates a connection which is appended to the list.
    Connection *CreateSoapServerConnection(int);

    /// Creates a connection which is appended to the list.
    Connection *CreateSoapClientConnection(const string&);

    /// Sets up listen_connection.
    bool SetUpListenConnection(bool listen = false, bool thread = false,
			       const string& msg = "");

    /// Sets up mpi_listen_connection.
    bool SetUpMPIListenConnection();

    /// Produces a unique cookie.
    const char *BakeCookieP(const char* = NULL) const;

    /// Makes a list of all queries.  Delete after use!
    const char *CreateQueryListM(const Query&) const;

    /// Dumps out query list.
    void DumpQueryList(ostream* = &cout, bool = true,
		       Query* = NULL, int = 0) const;

    /// Dumps out database list.
    void DumpDataBaseList(ostream* = &cout) const;

    /// Date string used in origins files.
    static string OriginsDateString(const struct timespec&, bool = false);

    /// Date string used in origins files.
    static string OriginsDateStringNow(bool = false);

    /// Inversion of date string used in origins files.
    static struct timespec InverseOriginsDateString(const string&);

    /// Invert stringified time back to binary form.
    static struct timespec InverseTimeStringOld(const char*);

    /// Invert stringified time back to binary form.
    static struct timespec InverseTimeString(const string& s) {
      return InverseTimeStringOld(s.c_str());
    }

    /// Extracts two ASCII digitrs from string and returns int 0..99.
    static int TwoDigitsToInt(const char*, int);

    /// Extracts four ASCII digitrs from string and returns int 0..9999.
    static int FourDigitsToInt(const char*, int);

    /// True if inknown keys should be reported in Query::AddOtherKeyValue().
    bool TraceOtherKeys() { return trace_other_keys; }

    /// Sets trace_other_keys, returns old value.
    bool TraceOtherKeys(bool t) {
      bool r = trace_other_keys; trace_other_keys = t; return r; }

    /// Checks if second type is subset of the first.
    static bool TargetTypeContains(target_type a, target_type b) {
      return (a&b)==b;  // was wrongly a&b==b until 25.08.2005!
    }

    /// Checks if there is any overlap in the target types.
    static bool TargetTypesIntersect(target_type a, target_type b) {
      return a&b;
    }

    /// Masks target_file (and target_segment) away from the given target_type.
    static target_type TargetTypeMasked(target_type t, bool segm_is_mod) {
      if (segm_is_mod)
	return target_type(t&~(target_file|target_segment|target_full));
      else
	return TargetTypeFileMasked(t);
    }

    /// Masks target_file away from the given target_type.
    static target_type TargetTypeFileMasked(target_type t) {
      return target_type(t&~target_file);
    }

    /// Masks target_segment away from the given target_type.
    static target_type TargetTypeSegmentMasked(target_type t) {
      return target_type(t&~target_segment);
    }

    /// Masks target_full away from the given target_type.
    static target_type TargetTypeFullMasked(target_type t) {
      return target_type(t&~target_full);
    }

    /// Masks target_segment and target_full away from the given target_type.
    static target_type TargetTypeSegmentFullMasked(target_type t) {
      return target_type(t&~(target_segment|target_full));
    }

    /// Masks target_file and target_full away from the given target_type.
    static target_type TargetTypeFileFullMasked(target_type t) {
      return target_type(t&~(target_file|target_full));
    }

    /// Returns cbir_algorithm from string.
    static cbir_algorithm CbirAlgorithm(const string&);
  
    /// Returns string from cbir_algorithm.
    static const char *CbirAlgorithmP(cbir_algorithm);

    /// A helper routine for all -Dxxx=v
    static int DebugInteger(const string& s) {
      int v = 1;
      size_t p = s.find('=');
      if (p!=string::npos && s.size()>p+1)
	v = atoi(s.substr(p+1).c_str());
      return v;
    }

    /// Sets debugging of temporary files.
    static void DebugTempFiles(int b) { debug_tmps = b; }

    /// Returns debugging of temporary files.
    static int DebugTempFiles() { return debug_tmps; }

    /// Sets debugging of speech recognizer.
    static void DebugSpeechRecognizer(int b) { debug_osrs = b; }

    /// Returns debugging of speech recognizer.
    static int DebugSpeechRecognizer() { return debug_osrs; }

    /// Sets debugging of memory consumption.
    static void DebugMemoryUsage(int b) { debug_mem = b; }

    /// State of debugging of memory consumption.
    static int DebugMemoryUsage() { return debug_mem; }

    /// Sets time usage debugging.
    static void DebugTimes(int b) { debug_times = b; }

    /// State of time usage debugging.
    static int DebugTimes() { return debug_times; }

    /// Sets octave debugging.
    static void DebugOctave(int b) { debug_octave = b; }

    /// State of octave debugging.
    static int DebugOctave() { return debug_octave; }

    /// Sets debugging of Interpret() calls.
    static void DebugInterpret(bool d) { debug_interpret = d; }

    /// State of debugging of Interpret() calls.
    static bool DebugInterpret() { return debug_interpret; }

    /// Conditionally traces Interpret() calls.
    static void TraceInterpret(bool ready, const string& o, const string& key,
			       const string& val, int result) {
      if (debug_interpret)
	cout << o << "::Interpret(" << key << "=" << val << ") ended"
	     << " with ready=" << ready << " result=" << result << endl;
    }

    ///
    static void KeepTemp(bool k) { keep_temp = k; }

    ///
    static bool KeepTemp() { return keep_temp; }

    /// General routine for object->XML conversion.
    bool AddObjectToXML(XmlDom& xml, Query *q, Connection *c,
			object_type ot, const string& on, const string& os);

    /// Adds system level object information to the XML document.
    bool AddToXML(XmlDom& dom, object_type ot,  const string& a, 
		  const string& b, Connection *c);

    /// Adds databaselist to the XML document.
    bool AddToXMLdatabaselist(XmlDom& dom, const string&, const string&,
			      Connection*);

    /// Adds bouncing instruction to the XML document.
    bool AddToXMLbounce(xmlNodePtr, xmlNsPtr);

    /// Creates a new query and takes care of it.
    Query *NewQuery();

    /// Adds query to the list of queries.
    Query *AppendQuery(Query *q, bool check);

    /// Deletes one query from the list by pointer.
    bool DeleteQuery(Query *q);

    /// Deletes one query from the list by index.
    bool DeleteQuery(size_t i);

    /// Gives the count of queries stored in the memory.
    size_t Nqueries() const { return query.size(); }

    /// Returns pointer to ith query.
    Query *GetQuery(size_t i) const { 
      return i>=Nqueries()? NULL : query[i];
    }

#ifdef PICSOM_USE_PTHREADS
    /// Adds now pthread,Query pair in the list of running queries.
    void AppendRunningQuery(pthread_t p, Query *q);

    /// Removes any finished queries from the pthread,Query pair list.
    void RemoveFinishedQueries(bool force);

    /// Sends cancellation to all running queries.
    void CancelRunningQueries();

    /// Returns the numebr of running queries.
    int NrunningQueries() const { return query_thread.size(); }

    /// Human-readable version of pthread's identity.
    static string ThreadIdentifier(const thread_info_t& t) {
      return ThreadIdentifierUtil(t.id, t.tid);
    }

    /// Common interface all thread registrations.
    string RegisterThread(pthread_t, pid_t,
			  const string&, const string&, void*, void*);

    /// Common interface all thread registrations.
    bool UnregisterThread(const string&);

    /// Common interface all thread registrations.
    bool UnregisterThread(pthread_t p) {
      thread_info_t *t = FindThread(p);
      return t ? UnregisterThread(t->name) :
	ShowError("PicSOM::UnregisterThread(pthread_t) failed");
    }

    ///
    void RwLockReadThreadList() const { RwLockRead(thread_list_lock); }

    ///
    void RwUnlockReadThreadList() const { RwUnlockRead(thread_list_lock); }

    ///
    void RwLockWriteThreadList() { RwLockWrite(thread_list_lock); }

    ///
    void RwUnlockWriteThreadList() { RwUnlockWrite(thread_list_lock); }

    /// A search utility.
    thread_info_t *FindThread(pid_t pid) {
      RwLockReadThreadList();

      thread_info_t *ret = NULL;
      ThreadListSanityCheck();
      for (thread_list_t::iterator i=thread_list.begin();
	   !ret && i!=thread_list.end(); i++)
	if (i->thread_set && i->tid==pid)
	  ret = &*i;

      RwUnlockReadThreadList();

      return ret;
    }

    /// A search utility.
    const thread_info_t *FindThread(pid_t pid) const {
      return ((PicSOM*)this)->FindThread(pid);
    }

    /// A search utility.
    thread_info_t *FindThread(pthread_t p) {
      RwLockReadThreadList();

      thread_info_t *ret = NULL;
      ThreadListSanityCheck();
      for (thread_list_t::iterator i=thread_list.begin();
	   !ret && i!=thread_list.end(); i++)
	if (i->thread_set && pthread_equal(i->id, p))
	  ret = &*i;

      RwUnlockReadThreadList();

      return ret;
    }

    /// A search utility.
    const thread_info_t *FindThread(pthread_t p) const {
      return ((PicSOM*)this)->FindThread(p);
    }

    /// A search utility.
    thread_info_t *FindThread(const string& n) {
      RwLockReadThreadList();

      thread_info_t *ret = NULL;
      ThreadListSanityCheck();
      for (thread_list_t::iterator i=thread_list.begin();
	   !ret && i!=thread_list.end(); i++)
	if (i->name==n)
	  ret = &*i;

      RwUnlockReadThreadList();

      return ret;
    }

    /// A search utility.
    const thread_info_t *FindThread(const string& n) const {
      return ((PicSOM*)this)->FindThread(n);
    }

    /// Conversion from pthread_t to string.
    const string& ThreadIdentifier(pthread_t t) const {
      ThreadListSanityCheck();
      static string empty;
      const thread_info_t *e = FindThread(t);
      return e ? e->name : empty;
    }

    /// Number of registered threads.
    size_t ThreadsTotal() const { return thread_list.size(); }

    /// Number of registered threads.
    size_t Threads(const string&, const string& = "") const;

    /// Number of registered threads.
    size_t ThreadsUninit() const { return Threads("uninit"); }

    /// Number of registered threads.
    size_t ThreadsPending() const { return Threads("pending"); }

    /// Number of registered threads.
    size_t ThreadsRunning() const { return Threads("running"); }

    /// Number of registered threads.
    size_t ThreadsReady() const { return Threads("ready"); }

    /// Number of registered threads.
    size_t ThreadsJoined() const { return Threads("joined"); }

    /// Number of available threads.
    size_t ThreadsAvailable() const {
      return threads<0 ? numeric_limits<size_t>::max() :
	ThreadsRunning()<(size_t)threads ? threads-ThreadsRunning() : 0;
    }

    /// A sleeping loop.
    void SleepIfNoThreadsAvailable();

    /// Checks if any thread is state==ready && phase==created.
    bool JoinThreads(bool);

    /// Executes command on named thread.
    bool ThreadExecuteCommand(thread_info_t*, const string&, bool&);

    /// A sanity check as the name suggests...
    bool ThreadListSanityCheck() const;

#else
    thread_info_t *FindThread(pid_t) { return NULL; }
    const thread_info_t *FindThread(pid_t) const { return NULL; }
    thread_info_t *FindThread(pthread_t) { return NULL; }
    const thread_info_t *FindThread(pthread_t) const { return NULL; }
    thread_info_t *FindThread(const string& n) const { return NULL; }
#endif // PICSOM_USE_PTHREADS

    /// Command given from connections.
    bool ExecuteCommand(const string&, const string&, XmlDom&, Connection*);

    /// Called mostly with slave instances.
    string CreateAnalysis(xmlNodePtr, const string&);

    /// Adds analysis to the list of analyses.
    void AppendAnalysis(Analysis*);

    /// Gives the count of analyses stored in the memory.
    size_t Nanalyses() const { return analysis.size(); }

    /// Returns pointer to ith analysis.
    Analysis *GetAnalysis(size_t i) const {
      return i<analysis.size() ? analysis[i] : NULL;
    }

    /// Finds Analysis by name.
    Analysis *FindAnalysis(const string& an) const;

    /// Finds Analysis by name.
    bool FindAnalysis(Analysis *a) const;

    /// Removes Analysis* from the list and deletes it.
    bool RemoveAnalysis(Analysis *a);

    ///
    bool SetSignalHandlers(bool);

    ///
    void SetSignalHandler(int, void (PicSOM::*)(int));

    ///
    static void SignalHandler(int);

    /// 
    void KillParentAndSelf(int);

    /// This just logs a signal and exits.
    void LogSignalAndExit(int);

    /// This just logs a signal and continues.
    void LogSignalAndContinue(int);

    /// This just logs a signal and resets it.
    void DummySignal(int);

    /// Upon receiving a signal this calls ShowConnections().
    void ShowConnectionsSignal(int);

    /// Upon receiving a signal this calls DumpDataBaseList().
    void DumpDataBaseListSignal(int);

    /// Upon receiving a signal this calls DumpQueryList().
    void DumpQueryListSignal(int);

    /// Upon receiving a signal this calls DumpMemoryUsage().
    void DumpMemoryUsageSignal(int);

    /// Upon receiving a signal this calls ShowTimes().
    void ShowTimesSignal(int);

    /// Upon receiving a signal this calls SaveAllAndExit().
    void SaveAllAndExitSignal(int);

    /// Saves and exits(0).
    void SaveAllAndExit();

    /// Closes all connections.
    bool CloseAllConnections();

    /// Returns save path by query's identity.  Delete after use!
    string SavePath(const string&, bool) const;

    /// Sves queries.
    bool SaveOldQueries(bool force = false) const;

    /// Expunges queries.
    bool ExpungeOldQueries();

    /// Creates a directory if it does not exist.
    bool MakeDirectory(const string& s, bool f = false) {
      return MakeDirectory(s.c_str(), f);
    }

    /// Creates a directory if it does not exist.
    bool MakeDirectory(const char*, bool = false) /*const*/;

    /// Actually calls mkdir().
    bool MkDirHier(const string& d, int m) const;

    /// Actually calls mkdir().
    bool MkDir(const string& d, int m) const;

    /// Actually calls mkdir().
    // bool MkDir(const char*, int) const;

    /// True if not server invocation.
    bool Development() const { return development; }

    /// True if defeult ie. non-user diectory has been set in path.
    bool DefaultDirPath() const { return defaultdirpath; }

    /// Depending on development returns path or ~/picsom.
    string RootDir(const string& = "", bool = false) const;

    ///
    size_t FileSystemFreeSpace(const string&);

    ///
    bool SetTempDirRoot(const string&);

    /// Temporary storage for files.
    const string& TempDirRoot();

    /// Temporary storage for files.
    string TempDirPersonal(const string& = "");

    ///
    bool CleanUpBogusDirs();

    /// Reads in queries specified in readquery.
    bool ReadQueries();

    /// Dumps queries specified in dumpquery.
    bool DumpQueries();

    /// Seeks queries directory for stored queries.
    bool AddToXMLquerylistfull(xmlNodePtr, xmlNsPtr) const;

    /// Seeks queries directory for stored queries.
    bool AddToXMLquerylistfullRecurse(xmlNodePtr, xmlNsPtr, const char*) const;

    /// Add server version information to XML.
    bool AddToXMLversions(XmlDom&) const;

    /// Add analysis information to XML.
    bool AddToXMLanalysislist(XmlDom&, const string&) const;

    /// Add analysis information to XML.
    bool AddToXMLanalysis(XmlDom&, const string&, const string&) const;

    /// Add analysis information to XML.
    bool AddToXMLanalysis(XmlDom&, const Analysis*, const thread_info_t*,
			  const string&) const;

    /// Add server status information to XML.
    bool AddToXMLstatus(XmlDom&) const;

    /// Add connection status information to XML.
    bool AddToXMLconnectionlist(XmlDom&) const;

    /// Add slave status information to XML.
    bool AddToXMLslavelist(XmlDom&) const;

    /// Add slave status information to XML.
    bool AddToXMLslave(XmlDom&, const slave_info_t*) const;

    /// Add thread status information to XML.
    bool AddToXMLthreadlist(XmlDom&) const;

    /// Add thread status information to XML.
    bool AddToXMLthread(XmlDom&, const thread_info_t*) const;

    /// Add thread status information to XML if thread terminated.
    bool AddToXMLthread_rip(XmlDom&, const string&) const;

    /// Helper for the above one.
    const string& ThreadState(const thread_info_t*) const;

    /// A reporting utility.
    string ThreadInfoString(const thread_info_t&, bool) const; 
  
#ifdef PICSOM_USE_PTHREADS
    /// True if called in initila thread.
    bool IsMainThread() const {
      pthread_t self = pthread_self();
      return pthread_equal(self, main_thread);
    }

    /// Returns pthreads_connection.
    bool PthreadsConnection() const { return pthreads_connection; }

    /// Returns pthreads_tssom.
    bool PthreadsTssom() const { return pthreads_tssom; }

    /// Returns debug_threads.
    bool DebugThreads() const { return debug_threads; }

    /// Blocks all signals from calling pthread.
    void BlockSignals();

    /// Shows that a thread has born.
    void ConditionallyAnnounceThread(const string&, pthread_t, pid_t);

    /// Shows that a thread has born.
    void ConditionallyAnnounceThread(const string& s) {
      ConditionallyAnnounceThread(s, main_thread, gettid());
    }

    ///
    bool IsSlaveStatusThread() {
      return slavestatus_thread_set && pthread_self()==slavestatus_thread;
    }

    ///
    bool HasSlaveStatusThread() { return slavestatus_thread_set; }

    ///
    bool ConditionallyStartSlaveStatusThread() {
      bool has = HasSlaveStatusThread();
      if (has)
	return true;
      return StartSlaveStatusThread() ;
    }

    ///
    bool StartSlaveStatusThread();

    ///
    bool SlaveStatusThreadMain();
    
    ///
    bool IsAnalysisThread() {
      return analysis_thread_set && pthread_self()==analysis_thread;
    }

    ///
    bool HasAnalysisThread() { return analysis_thread_set; }

    ///
    bool ConditionallyStartAnalysisThread() {
      bool has = HasAnalysisThread();
      if (has)
	return true;
      return StartAnalysisThread() ;
    }

    ///
    bool StartAnalysisThread();

    ///
    bool AnalysisThreadMain();

    ///
    bool RunInAnalysisThread(const script_list_t&);

    ///
    void RwLockReadAnalysisScriptList() { RwLockRead(analysis_script_list_lock); }

    ///
    void RwUnlockReadAnalysisScriptList() { RwUnlockRead(analysis_script_list_lock); }

    ///
    void RwLockWriteAnalysisScriptList() { RwLockWrite(analysis_script_list_lock); }

    ///
    void RwUnlockWriteAnalysisScriptList() { RwUnlockWrite(analysis_script_list_lock); }

#else
    bool IsMainThread() const { return true;  }
    bool DebugThreads() const { return false; }
#endif // PICSOM_USE_PTHREADS

    /// Allows this yhread to be cancelled now.
    static void ThreadTestCancel() {
#ifdef PICSOM_USE_PTHREADS
      pthread_testcancel();
#endif // PICSOM_USE_PTHREADS
    }

    /// Recommended thread count.
    int Threads() const {
#ifndef PICSOM_USE_PTHREADS
      int threads = 0;
#endif // PICSOM_USE_PTHREADS
      return threads;
    }

    /// This works with Magick++ only...
    bool DisplayImage(istream&, const char* = NULL);

    /// Sets up allowed_databases.
    bool SetAllowedDataBases(const string& n);

    /// Adds one entry in the list of allowed_databases.
    void AddAllowedDataBase(const string& n, bool even_first = false) {
      if ((!allowed_databases.empty() || even_first) &&
	  allowed_databases.find(n)==allowed_databases.end()) {
	allowed_databases.insert(n);
	WriteLog("Database ", n, " added as an allowed database");
      }
    }

    /// Checks for database name in allowed_databases:
    bool IsAllowedDataBase(const string& n) const {
      return allowed_databases.empty()
	|| allowed_databases.find(n)!=allowed_databases.end();
    }

    /// Access to allowed_databases.
    const set<string>& AllowedDataBases() const { return allowed_databases; }

    /// Checks if any of DataBases are busy.
    bool SomeDataBaseBounces() const;

    /// An utility to make HTML files.
    static xmlDocPtr HTMLfile(xmlNodePtr* = NULL, xmlNodePtr* = NULL);

    /// An utility to add <title>...</title> in an HTML file.
    static xmlNodePtr HTMLtitle(xmlNodePtr d, const string& s = "") {
      return AddTag(d, NULL, "title", s);
    }

    /// An utility to add <h1>...</h1> in an HTML file.
    static xmlNodePtr HTMLh1(xmlNodePtr d, const string& s = "") {
      return AddTag(d, NULL, "h1", s);
    }

    /// An utility to add <h2>...</h2> in an HTML file.
    static xmlNodePtr HTMLh2(xmlNodePtr d, const string& s = "") {
      return AddTag(d, NULL, "h2", s);
    }

    /// An utility to add <h3>...</h3> in an HTML file.
    static xmlNodePtr HTMLh3(xmlNodePtr d, const string& s = "") {
      return AddTag(d, NULL, "h3", s);
    }

    /// An utility to add <p>...</p> in an HTML file.
    static xmlNodePtr HTMLp(xmlNodePtr d, const string& s = "") {
      return AddTag(d, NULL, "p", s);
    }

    /// An utility to add <img/> in an HTML file.
    static xmlNodePtr HTMLimg(xmlNodePtr p, const string& l) {
      xmlNodePtr b = AddTag(p, NULL, "img");
      SetProperty(b, "src", l);
      return b;
    }

    /// Helper function to symlink/copy images for generated HTML file.
    static string HTMLlocalLinkImage(const string& im, const string& idir,
				     bool copy=false);


    /// An utility to add <a href=""><img/></a> in an HTML file.
    static void HTMLlocalTnAndImage(xmlNodePtr d, const string& s,
				    DataBase*, const string&, int=0);

    /// An utility to add <@IMAGE@ ... /> in an HTML file.
    static void HTMLserverTnAndImage(xmlNodePtr d, const string& s,
				     const DataBase*, const Query*,
				     const list<pair<string,string> >&);

    /// An utility to add <a href=""><img/></a> in an HTML file.
    static void HTMLlocalMap(xmlNodePtr d, const Query*, int, int, int,
			     const string&);

    /// An utility to add <@IMAGE@ ... /> in an HTML file.
    static void HTMLserverMap(xmlNodePtr d, const Query*, int, int, int);

    /// A function that tries to find a suitable PicSOM executable in
    /// the given directory with the given extension (e.g "fast" or "debug")
    string FindExecutable(const string&, const string&, 
			  const string& = "fast") const;

    /// Access to architecture string.
    const string& SystemArchitecture() const { return sysarchitecture; }

    /// Access to command line args behind -a/-c/-i/...
    const vector<string>& CmdLineArgs() const { return cmdline_args; }

    /// Setting of command line args behind -a/-c/-i/...
    void CmdLineArgs(const vector<string>& v) { cmdline_args = v;  }

    /// Calls system().
    int ExecuteSystem(const string& cmd, bool pre, bool ok, bool err);

    /// Calls system().
    int ExecuteSystem(const vector<string>& args, bool pre, bool ok, bool err) {
      return ExecuteSystem(ToStr(args), pre, ok, err);
    }

    /// Returns the invocation command line.
    const string& CmdLine() const { return cmdline_str; }

    ///
    class detection_stat_t {
    public:
      ///
      detection_stat_t() :
	njobs(0), nok(0), nfailed(0), ntot(0), nskip(0), 
	nfound(0), nnodata(0), ndone(0) {}
      
      ///
      size_t njobs;

      ///
      size_t nok;

      ///
      size_t nfailed;

      ///
      size_t ntot;

      ///
      size_t nskip;

      ///
      size_t nfound;

      ///
      size_t nnodata;

      ///
      size_t ndone;
    };

    ///
    struct speech_result_entry_t {
      ///
      int sframe, eframe;
    
      ///
      struct timespec stime, etime;

      ///
      string str;

      ///
      string qid;
    };

    ///
    class speech_data_entry_t {
    public:
      ///
      speech_data_entry_t(const string& q, const struct timespec& s ,
			  const struct timespec& e ) : qid(q), stime(s), etime(e)
      {}

      ///
      string     qid;

      ///
      struct timespec stime;

      ///
      struct timespec etime;

      ///
      list<speech_result_entry_t> result;
    }; // class speech_data_entry_t

    ///
    speech_data_entry_t *FindSpeechData(const struct timespec&, bool);

    ///
    void DumpSpeechData(const speech_data_entry_t*);

    /// Starts speech recognizer.
    bool SpeechRecognizerRunning() const { return speech_recognizer; }

    /// Starts speech recognizer.
    bool StartSpeechRecognizer(const string&);

    /// Checks if speech recognizer has bee initialized.
    bool SpeechRecognizerAcceptsData() const;

    /// Tests speech recognizer with file input.
    bool TestSpeechRecognizer(const string&);

    /// Calls speech recognizer.
    bool FeedSpeechRecognizer(const string&, const string&,
			      const struct timespec&, const struct timespec&);

    /// Reads speech recognizer.
    bool ProcessSpeechRecognizerOutput();

    /// Reads speech recognizer.
    bool ProcessSpeechRecognizerOutputV1();

    /// Reads speech recognizer.
    bool LoadContext(const string&);

    /// Writes strings to log.
    void WriteLog(const string& m1, const string& m2 = "",
		  const string& m3 = "", const string& m4 = "", 
		  const string& m5 = "") const {
      WriteLogStr(LogIdentity(), m1, m2, m3, m4, m5); }

    ///
    bool StorePidToFile() const;

    /// Solves the system architecture and returns it in a string
    string SolveArchitecture() const; 

    ///
    list<string> Translate(const string&, const string&, const string&,
			   const list<string>&);


    ///
    list<string> TranslateYandex(const string&, const string&,
				 const list<string>&);

    ///
    bool ReadSecrets();

    ///
    string GetSecret(const string&, bool) const;

    ///
    string UnZip(const string&, const string& = "", const string& = "");

  protected:
    /// protected methods first:

    /// Tests all labels of a database.
    bool LabelTest(const string&);

    ///
    bool HttpClientTest(const string&);

    /// Tests all writing streams.
    void StreamsTest();

    /// Tests image reading and writing.
    void ImageTest(const string&);

    /// Starts to log in specified or standard location.
    bool SetLogging(const string& = "");

    /// Opens logging stream.
    bool OpenLog(const string&);

    /// Closes logging stream.
    bool CloseLog() { log.close(); return log.good(); }

    /// Writes contents of a stream to log.
    void WriteLog(ostringstream& os) { WriteLogStr(LogIdentity(), os); }

    /// Gives log form of TSSOM name.
    string LogIdentity() const {
      return ":::";
    }

    /// These are needed in version-related functions.
    typedef pair<string,string> version_data_entry_type;

    /// These are needed in version-related functions.
    typedef list<version_data_entry_type> version_data_type;

    /// Solves the linux duistribution and returns it in a string
    string SolveLinuxDistribution() const; 

    /// Solves all version information at once and then returns it.
    void SolveVersionData() const; // non-static due to knowledge about threads

    /// A helper utility for the one abobe.
    static void SetVersionData(const string& n, const string& v) {
      version_data.push_back(version_data_entry_type(n, v));
    }

    /// Removes atexit() message.
    static PicSOM *AtExitInstance(PicSOM *p) {
      PicSOM *old = atexit_instance;
      atexit_instance = p;
      return old;
    }

    /// Writes a message to log.
    static void WriteLogExit();

    /// Writes version and code-generation flags to log.
    void WriteLogVersion();

    /// Writes version and code-generation flags to log.
    void WriteLogProcessInfo();

    /// Writes a message with signal name and number and optional text.
    void WriteLogSignal(int, const char* = NULL);

    /// Converts signal number to string.
    static const char *SignalNameP(int);

    /// True if signal handling should be set.
    bool HandleSignals() const {
      return use_signals || (start_online && !use_nosignals);
    }

    /// data members start here:

    ///
    struct timespec start_time;

    ///
    static bool has_features_internal;

    ///
    static bool has_segmentation_internal;

    ///
    static bool has_imgrobot_internal;

    ///
    Connection *listen_connection;

    ///
    Connection *mpi_listen_connection;

    ///
    Connection *soap_server;

    ///
    Connection *speech_recognizer;

    ///
    struct timespec speech_recognizer_start_time;

    ///
    struct timespec speech_zero_time;

    ///
    string speech_str;

    ///
    list<speech_data_entry_t> speech_data;

    /// Pointers to signal handler functions.
    static void (PicSOM::*signal_function[128])(int);

    /// Pointers to PicSOM objects for signal catching.
    static PicSOM *signal_instance[128];

    /// Pointer PicSOM object for atexit() processing.
    static PicSOM *atexit_instance;

    /// Logging stream common to all connections.
    ofstream log;

    /// Stored rdbuf of cout;
    streambuf *streambuf_cout;

    /// Stored rdbuf of cerr;
    streambuf *streambuf_cerr;

    /// Stored rdbuf of clog;
    streambuf *streambuf_clog;

    /// True if logging shall include timestamp;
    bool log_timestamp;

    /// True if logging shall include identity information.
    bool log_identity;

    /// True if localhost should be used in urls instead of hostname.
    bool uselocalhost;

    ///
    static string forcedhostname;

    /// Environment variables PICSOM_XXX mapped as XXX
    map<string,string> env;

    /// Root of all rootless files and dirs:
    string path_str;

    /// Command line arguments concatenated.
    string cmdline_str;

    /// Set to true to force exit from MainLoop().
    bool quit;

    /// All connections are listed here.
    vector<Connection*> connection;  

    /// All databases listed here.
    ListOf<DataBase> database;

    /// All queries listed here.
    vector<Query*> query;

    /// All analyses listed here.
    vector<Analysis*> analysis;

    /// True replaces old startup_mode=operation_online
    bool start_online;

    /// True replaces old startup_mode=operation_analyse
    bool start_analyse;

    /// Indicates that only version information should be shown.
    bool start_version_only;

    /// Vector of arguments behind -analyse/-create/...
    vector<string> cmdline_args;

    /// True if Query::AddOtherKeyValue() should report unknown keys.
    bool trace_other_keys;

    /// True if WriteLog() methods shoud be no-ops.
    bool quiet;

    /// True if cin can be read.
    bool has_cin;

    /// PicSOM executable's name, given as argv[0] to main().
    string myname_str;

    /// PicSOM executable's path.
    string mybinary_str;

    /// 0=no, 1=if, 2=yes
    size_t gpupolicy;

    /// -1=none, 0,1,... = deviceid
    int gpudevice;

    /// If set to true, old queries are not stored on disk.
    bool no_save;

    /// Time in seconds before old queries should we removed from core memory.
    int expunge_time;

    /// Reads connections and returns the first one to be read.
    Connection *SelectInput();

    /// Range of socket ports where to listen for connections.
    int listen_porta, listen_portb;

    /// Socket port to create SOAP server in.
    int soap_port;

    /// Controls debugging of temporals.
    static int debug_tmps;

    /// Controls viewing of speech recognition results.
    static int debug_osrs;

    /// Controls viewing of memory usage.
    static int debug_mem;

    /// Debugging of time usage.
    static int debug_times;

    /// Debugging of octave.
    static int debug_octave;

    /// Tracing of Interpret() calls.
    static bool debug_interpret;  

    /// Tracing of Interpret() calls.
    static bool debug_slaves;  

    ///
    static bool keep_temp;

    ///
    static bool force_fast_slave;

    /// This stores all timing information.
    TicTac tics;

    /// port:user@host specifier for ssh port forwarding.
    string ssh_forwarding;

    /// Normally true.  Set false if -server really should not run in background.
    bool allow_fork;

    /// Set to true if fork() was faked do to allow_fork==false.
    bool fork_faked;

    /// True if this process should fork new instances.
    bool guarding;

    /// Child pid of last fork made by a guard process.
    pid_t guarded_pid;

    ///
    bool childlimits;

    /// True if this process is a slave.
    bool is_slave;

    ///
    bool expect_slaves;

    /// If slave, then this is http://mastername:port
    string master_address;

    ///
    string master_auth_token;

    /// True for all but the server invocation.
    bool development;

    /// True if path is set to non-user directory.
    bool defaultdirpath;

    /// True if there is no filesystem access to picsom root dir.
    bool norootdir;

    ///
    string temp_dir_root, temp_dir_root_d;

    ///
    string temp_dir_personal;

    /// True if constructor should set signal handlers.
    bool use_signals;

    /// True if constructor should not set signal handlers.
    bool use_nosignals;

    /// List of allowed databases if restricted with -db ...,...
    set<string> allowed_databases;

    /// Names of query files to read in startup.
    vector<string> readquery;

    /// Names of queries to dump in startup.
    vector<string> dumpquery;

    /// Data saved here in SolveVersionData().
    static version_data_type version_data;  // static though knows about threads

    /// System architecture string (e.g. "linux", "alpha" or "irix")
    string sysarchitecture;

    /// Linux distribution such as "Ubuntu 9.10" from lsb_release -d.
    string linuxdistr;

#ifdef PICSOM_USE_PTHREADS
    /// Initial thread's id.
    pthread_t main_thread;

    /// Another thread started for listening in analysis mode.
    pthread_t listen_thread;

    ///
    pthread_t mpi_listen_thread;

    /// Slave status query loop
    pthread_t slavestatus_thread;

    ///
    pthread_t analysis_thread;

    ///
    bool listen_thread_set, mpi_listen_thread_set;

    ///
    bool slavestatus_thread_set, analysis_thread_set;

    /// Number of threads.
    int threads;

    /// True if pthreads in connections.
    bool pthreads_connection;

    /// True if pthreads in tssoms.
    bool pthreads_tssom;

    /// Mutex for log writing.
    pthread_mutex_t log_mutex;

    /// Mutex for all picsom/process-wide operations.
    pthread_mutex_t mutex;

    /// Debugging of thread launches.
    static bool debug_threads;

    /// Whether to launch a bowser to attach the listening socket.
    static string launch_browser_str;

    /// name of the browser.
    static string browser;

    /// Threads where queries are running RunPartTwo().
    /// (should be converted to use thread_list)
    typedef pair<pthread_t, Query*> query_thread_e;
    typedef list<query_thread_e> query_thread_t;
    query_thread_t query_thread;

    /// Info on running pthreads.
    thread_list_t thread_list;

    ///
    RwLock thread_list_lock;

    ///
    RwLock querylistlock;

    ///
    RwLock slavelistlock;

    ///
    RwLock analysis_script_list_lock;

#else
    bool querylistlock, slavelistlock;
#endif // PICSOM_USE_PTHREADS

    /// Asynchronous analyses.
    script_list_t analysis_script_list;

    /// Info on slaves.
    slave_list_t slave_list;

    ///
    string slavepipe;

    ///
    bool use_mpi_slaves;

    ///
    bool listen_mpi;

    ///
    map<string,list<pair<string,string> > > default_key_value;

    ///
    list<string> octavebasepath;

    ///
    set<string> octavepath;

    ///
    string sqlserver;

    ///
    bool tempsqlitedb;

    ///
    string pidfile;

    ///
    map<string,string> secrets;

  }; // class PicSOM

  /////////////////////////////////////////////////////////////////////////////

  class dtw_spec {
  public:
    ///
    typedef map<string,map<size_t,map<size_t,float> > > cache_t;

    ///
    class fea_t {
    public:
      ///
      fea_t(const string& n, float w = 1.0, const string& d = "euc",
	    const map<size_t,float>& m = map<size_t,float>())
	: name(n), dist(d), wght(w), comp(m) {}

      ///
      string str() const;

      ///
      string name;

      ///
      string dist;

      ///
      float wght;

      ///
      map<size_t,float> comp;

    }; // class fea_t;

    ///
    typedef list<fea_t> flist_t;

    ///
    dtw_spec() {}

    ///
    dtw_spec(const string& f, float w = 1.0, const string& d = "euc",
	     const map<size_t,float>& m = map<size_t,float>()) {
      fea_t e(f, w, d, m);
      flist_t l { e };
      s.push_back(l);
    }

    ///
    static dtw_spec parse(const string&);

    ///
    string str() const;

    ///
    static string str(const flist_t&);

    ///
    list<flist_t> s;

    ///
    pair<vector<float>,vector<float> > wind;


    ///
    string dtwparam;

  }; // class dtw_spec;

  /////////////////////////////////////////////////////////////////////////////

  /// Used for video segments.
  class video_frange {
  public:
    ///
    video_frange() : db(NULL), idx((size_t)-1), begin(0), end(0),
		     begin_t(0.0), end_t(0.0) {}

    ///
    video_frange(const DataBase *db, size_t idx, size_t b, size_t e,
		 const string& t = "", const string& n = "",
		 const string& s = "") :
      db(db), idx(idx), begin(b), end(e), type(t), name(n), text(s),
      begin_t(0.0), end_t(0.0) {}

    ///
    video_frange(const DataBase *db, size_t idx, float b, float e,
		 const string& t = "", const string& n = "",
		 const string& s = "") :
      db(db), idx(idx), begin(0), end(0), type(t), name(n), text(s),
      begin_t(b), end_t(e) {}

    ///
    video_frange(const DataBase *db, size_t idx, const pair<size_t,size_t>& p,
		 const string& t = "", const string& n = "",
		 const string& s = "") :
      db(db), idx(idx), begin(p.first), end(p.second), type(t), name(n),
      text(s), begin_t(0.0), end_t(0.0) {}

    ///
    size_t nframes() const { return end-begin; }

    ///
    string value() const;

    ///
    string str() const {
      stringstream ss;
      ss << "[" << (name!=""?name+" ":"") << value()
	 << (text==""?"":" '"+text+"'") << "]";
      return ss.str();
    }

    ///
    string dashstr() const;

    ///
    video_frange get_union(const video_frange& r) const {
      return video_frange(db, idx, min(begin, r.begin), max(end, r.end));
    }
    
    ///
    video_frange get_intersection(const video_frange& r) const {
      video_frange t(db, idx, max(begin, r.begin), min(end, r.end));
      return t.begin<t.end ? t : empty();
    }
    
    ///
    static const video_frange& empty() {
      static const video_frange e(NULL, (size_t)-1, 0.0f, 0.0f);
      return e;
    }

    ///
    bool is_empty() const {
      return begin==0 && end==0 && begin_t==0.0 && end_t==0.0;
    }

    ///
    const DataBase *db;

    ///
    size_t idx;

    ///
    size_t begin;

    ///
    size_t end;

    ///
    string type;

    ///
    string name;

    ///
    string text;

    ///
    float begin_t;

    ///
    float end_t;

  }; // class video_frange

  /////////////////////////////////////////////////////////////////////////////

  /// Used for matching of video segments.
  class video_frange_match {
  public:
    ///
    video_frange_match(const video_frange& a, const video_frange& b) :
      a(a), b(b) {}
      
    ///
    //static double maxdist = numeric_limits<double>::max()/10;
    //static double maxdist() { return -1.0; };

    ///
    void set_gt_a(const video_frange& r) { gt_a = r; }

    ///
    void set_gt_b(const video_frange& r) { gt_b = r; }

    ///
    double get_value(const string& n) const {
      if (value.find(n)!=value.end())
	return value.find(n)->second;
      return -1;
    }

    ///
    void set_value(const string& n, double v) {
      value[n] = v;
    }

    ///
    string str() {
      stringstream ss;
      ss << a.dashstr();
      if (!gt_a.is_empty())
	ss << " ( "+gt_a.dashstr()+" )";
      ss << " ~ ";
      ss << b.dashstr();
      if (!gt_b.is_empty())
	ss << " ( "+gt_b.dashstr()+" )";
      for (auto i=value.begin(); i!=value.end(); i++)
	ss << " "+i->first+"=" << i->second;
      return ss.str();
    }

    ///
    video_frange a;
      
    ///
    video_frange b;

    ///
    video_frange gt_a;
      
    ///
    video_frange gt_b;

    //
    map<string,double> value;

  }; // class video_frange_match

  /////////////////////////////////////////////////////////////////////////////

  ///
  class textsearchresult_t {
  public:
    ///
    textsearchresult_t(size_t i, float v, const string& t) :
      idx(i), val(v), txt(t) {}

    ///
    string str() const {
      stringstream ss;
      ss << "#" << idx << " " << val << " \"" << txt << "\"";
      return ss.str();
    }

    ///
    size_t idx;

    ///
    float val;

    ///
    string txt;
  }; // class textsearchresult_t

  /////////////////////////////////////////////////////////////////////////////

  ///
  class textsearchspec_t {
  public:
    ///
    textsearchspec_t(const string& n = "") : db(NULL), name(n) {}

    ///
    DataBase *db;

    ///
    string name;

    ///
    string textindex;

    ///
    string textfield;

    ///
    string queryfield;

  }; // textsearchspec_t

  /////////////////////////////////////////////////////////////////////////////

  /// Used for timestamped textual results such as speech recognizer outputs.
  class textline_t {
  public:
    //
    textline_t() : db(NULL), idx(-1), start(-1), end(-1) {}

    //
    textline_t(DataBase *_db, size_t _idx) :
      db(_db), idx(_idx), start(-1), end(-1) {}

    //
    string str(bool = false, bool = true) const;

    //
    bool parse(const string&);

    //
    void add(const pair<string,double>& sv) {
      txt_val.push_back(sv);
    }

    //
    void add(const string& s, double v) {
      add(make_pair(s, v));
    }

    //
    void set(const string& s, double v) {
      txt_val.clear();
      add(make_pair(s, v));
    }

    //
    string get_text() const {
      return empty() ? "" : txt_val[0].first;
    }

    //
    bool has_time_set() const {
      return start!=-1 || end!=-1;
    }

    //
    bool empty() const { return txt_val.empty(); }

    //
    DataBase *db;

    //
    size_t idx;

    //
    double start;

    //
    double end;

    //
    string generator;

    //
    string evaluator;

    //
    vector<pair<string,double> > txt_val;

  };  // class textline_t

} // namespace picsom

#endif // _PICSOM_H_

