// -*- C++ -*-  $Id: Feature.C,v 1.261 2019/04/10 12:29:28 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <Feature.h>

#ifndef NO_PTHREADS
#include <pthread.h>
#endif // NO_PTHREADS

#ifdef sgi
#include <dlfcn.h>
#else
#define dlopen(a,b)
#define RTLD_LAZY 0
#endif // sgi

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif // HAVE_WINSOCK2_H

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif // HAVE_PWD_H

#if defined(sgi) || defined(__alpha)
#include <sys/procfs.h>
#endif // defined(sgi) || defined(__alpha)

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef HAVE_GLOG_LOGGING_H
#include <glog/logging.h>
#endif // HAVE_GLOG_LOGGING_H

namespace picsom {
#ifdef HAVE_GLOG_LOGGING_H
  ///
  bool Feature::glog_init_done = false;
#endif 

  ///
  int Feature::gpu_device_id = -1;

  ///
  const Feature *Feature::list_of_methods;

  ///
  vector<Feature::preprocess_method_info_t> Feature::preprocess_method_info;

  ///
  int Feature::verbose = 0;

  ///
  bool Feature::debug_memory_usage = false;

  ///
  string Feature::picsomroot;

  ///
  string Feature::static_tempdir;

  //===========================================================================

  Feature::Feature() {
    if (MethodVerbose())
      cout << "Constructing a Feature instance." << endl;

    fout_xx = NULL;
    framestep = 1;
    precision = 7;
    use_background = calculated = raw_in_mode = raw_out_mode = false;
    cacheing = true;
    region_hierarchy=false;
    interest_points=false;
    virtual_regions=false;
    virtual_only=false;
    hist_zone_norm = hist_norm_L1;
    hist_layer_norm = hist_global_norm = hist_norm_none;
    print_vector_length = description_print = description_all = true;
    check_vector_length=false;
    feature_vector_length = -1;
    feature_vector_length_fake = -1;
    treat_between_slice = treat_undef;
    treat_between_frame = treat_undef;
    treat_within_frame  = treat_undef;
    used_pixeltype = pixel_undef;
    pixel_multiply = 1.0;
    keeptmp = false;
    next_of_methods = NULL;
    allocated_segmentfile = segmentfile_ptr = NULL;
    videofile_ptr = NULL;
    deprecated_audiofile_ptr = NULL;
    margin_start_set = margin_end_set = 0;
    raw_input_dim = 0;
    suppressborders = -1;
    sec_label_interval = 0;

    framerange.first = framerange.second = 0;

    if (static_tempdir=="")
      SetTempDir(true);
    else
      tempdir = static_tempdir;

    opencvptr = NULL;
    incore_imagedata_ptr = NULL;
  }

  //===========================================================================

  int Feature::Main(const vector<string>& argv,
		    list<incore_feature_t>& incore,
		    feature_result *result) {
    try {
#if defined(HAVE_GLOG_LOGGING_H) && defined(PICSOM_USE_CAFFE)
      if (!glog_init_done) {
	string progname = "features";
	google::InitGoogleLogging(progname.c_str());
	FLAGS_logtostderr = 0;
	glog_init_done = true;
      }
#endif // HAVE_GLOG_LOGGING_H && PICSOM_USE_CAFFE

      bool first = true, cmd_shown = false;
      const string& myname = argv[0];
      string msg = "Feature::Main() ";
      string fname;
      int argc = argv.size()-1+incore.size();
      unsigned int pos = 1;

      if (!argc) {
        ShowUsage(myname);
        return 2;
      }

      string sp = " ", cmd = myname, segmfile;
      string input_pat, std_naming_pat, std_naming_dir;
      string specific_out_pat, label_pattern, fake, outdir, subdir;
      int std_out_pat = -1, std_rawin_pat = -1, fakelength = -1;
      bool raw_out = false;
      bool now_opts = false, testing = false, quiet = false;
      bool print_header = true;
      bool force_reuse = false, force_no_reuse = false;
      bool allow_file_errors = false;
      bool continue_after_error = true;
      size_t concat = 0, ncolon = 0;
      list<string> opts;
      int retval = 0;

      int vlen=-1;
      vector<string> filelist;

      pair<string,ofstream> outfile;

      Feature *fptr = NULL;

      auto incore_i = incore.begin();

      // feature_result dummy_result; if (!result) result = &dummy_result;

      while (argc+concat/2>0) {
        string astr;
        if (pos<argv.size())
          astr = argv[pos];
        else if (concat)
          astr = "";
        else if (pos<argv.size()+incore.size())
	  astr = "";
	else
          throw string("argv[pos] problem");

        if (fname=="")
          if (astr[0]=='-') {
            switch (astr[1]) {
            case 'd':
              if (astr[2]=='m') {
                DebugMemoryUsage(true);
                break;
              }
	      /* fall-thru */ // I guess...
            case 'D':
            case 'm': {
              pair<bool,int> verb = SolveVerbosity(astr.substr(1));
              if (verb.second>0) {
		string extra;
		if (!verb.first && astr[1]=='d')
		  extra = 
		    verb.second==1 ? " (method)" :
		    verb.second==2 ? " (file)" :
		    verb.second==3 ? " (slice)" :
		    verb.second==4 ? " (frame)" :
		    verb.second==5 ? " (label)" :
		    verb.second==6 ? " (keypoint)" :
		    verb.second==7 ? " (pixel)" : " (full)";
                cout << msg << (verb.first?"Adding to":"Setting")
                     << (astr[1]=='D' ? " Segmentation" :
                         astr[1]=='m' ? " imagefile" : "")
                     << " debug level " << verb.second << extra << endl;
	      }
              if (verb.first)
                if (astr[1]=='d')
                  AddVerbose(verb.second);
                else if (astr[1]=='D')
                  ; // Segmentation::AddVerbose(verb.second);
                else
                  imagefile::debug_impl(imagefile::debug_impl()+verb.second);
              
              else
                if (astr[1]=='d')
                  Verbose(verb.second);
                else if (astr[1]=='D')
                  ; // Segmentation::Verbose(verb.second);
                else
                  imagefile::debug_impl(verb.second);
              break;
              if (astr[2] && isdigit(astr[2])) {
                int l = atoi(astr.substr(2).c_str());
                if (l>0) 
                  cout << msg << "Setting debug level to " << l << endl;
                if (astr[1]=='d')
                  Verbose(l);
                else {} // Segmentation::Verbose(l);
              } else {
                int l = strspn(astr.substr(1).c_str(),"d");
                cout << msg << "Setting debug level to " << l << endl;
                if (astr[1]=='d')
                  AddVerbose(l);
                else {} // Segmentation::AddVerbose(l);
              }
              break;
            }
          
            case 'l':
              if (astr[2]=='x') {
                PrintFeatureListXML(result ? NULL : &cout, result);
              } else
                PrintFeatureList(cout, true);
              break;

            case 'h': {
              string fn;
              if (pos+1<argv.size()) {
                fn = argv[pos+1];
                argc--;
                pos++;
              }
              if (astr[2]=='x')
                PrintFeatureDescription(fn, cout);
              else
                PrintInstructions(cout, fn);
              break;
            }

            case 't':
              if (astr[2]=='t') {
                cerr << myname << " : -tt switch should be the first one"
                     << endl;
                goto switch_error;
              }
              else
                testing = true;
              // cmd += sp+a;
              break;

            case 'q':
              quiet = true;
              cmd += sp+astr;
              break;

            case 'x':
              print_header = false;
              cmd += sp+astr;
              break;

            case 'u':
              force_no_reuse = true;
              cmd += sp+astr;
              break;

            case 'U':
              force_reuse = true;
              cmd += sp+astr;
              break;

            case 'b':
              opts.push_back("background=true");
              cmd += sp+astr;
              break;

            case 'T':
              opts.push_back("regionhierarchy=true");
              cmd += sp+astr;
              break;

	    case 'k':
	      if (astr=="-kmeans" && argc==5) {
		return KMeans(argv[2], atoi(argv[3].c_str()),
			      atoi(argv[4].c_str()), argv[5]) ? 0 : 3;
	      }
	      if (astr=="-keep")
		opts.push_back("keeptmp=true");
	      else
		opts.push_back("interestpoints=true");
	      cmd += sp+astr;
	      break;

	    case 'g':
	      if (astr=="-gpu") {
		GpuDeviceId(0);
		cmd += sp+astr;
	      } else
		goto switch_error;
	      break;

            case 'v':
              opts.push_back("virtualregions=true");
              cmd += sp+astr;
              break;

            case 'V':
              opts.push_back("virtualonly=true");
              cmd += sp+astr;
              break;

            case 'r':
	      if (astr.find("-root=")==0)
		opts.push_back(astr.substr(1));
	      else {
		raw_out = true;
		opts.push_back("rawoutmode=true");
	      }
	      cmd += sp+astr;
              break;

            case 'R':
              opts.push_back("rawinmode=true");
              cmd += sp+astr;
              std_rawin_pat = astr[2]==0   ? 0 : astr[2]=='b' ? 1
                :                    astr[2]=='s' ? 2 : astr[2]=='S' ? 3
                :                    astr[2]=='R' ? 4 : astr[2]=='x' ? 5 
                :                    astr[2]=='r' ? 6 : -1;
              if (std_rawin_pat==-1)
                goto switch_error;
              break;

            case 'e':
              allow_file_errors = true;
              cmd += sp+astr;
              break;

            case 'p':
              if (argc<2)
                goto switch_error;
          
              argc--;
              pos++;
              opts.push_back(string("pairbypredicate=")+argv[pos]);

              cmd += sp+astr+sp+argv[pos];
              break;

            case 'W':
              if (!TreatmentType(astr[2]))
                goto switch_error;
              opts.push_back("withinframe="+astr.substr(2));
              cmd += sp+astr;
              break;
            
            case 'B':
              if (!TreatmentType(astr[2]))
                goto switch_error;
              opts.push_back("betweenframe="+astr.substr(2));
              cmd += sp+astr;
              break;

            case 'S':
              if (!TreatmentType(astr[2]))
                goto switch_error;
              opts.push_back("betweenslice="+astr.substr(2));
              cmd += sp+astr;
              break;

            case 'Z': {
              string zt(astr.substr(2));
              if (zt.empty())
                goto switch_error;
              opts.push_back("zoning="+zt);
              cmd += sp+astr;
              break;
            }

            case 'z': {
              string zt(astr.substr(2));
              if (zt.empty())
                goto switch_error;
              opts.push_back("slicing="+zt);
              cmd += sp+astr;
              break;
            }

            case 'C': {
              concat = atoi(astr.substr(2).c_str());
              if (!concat || concat%2!=1)
                goto switch_error;
              cmd += sp+astr;
              break;
            }

            case 'c': {
              string cachep(astr.substr(2));
              if (cachep.empty())
                goto switch_error;
              opts.push_back("cacheing="+cachep);
              cmd += sp+astr;
              break;
            }

            case 'Q':
              if (!PixelTypeName(astr.substr(2)))
                goto switch_error;
              opts.push_back("pixeltype="+astr.substr(2));
              cmd += sp+astr;
              break;

            case 's':
	      if (astr[2]=='L')
		opts.push_back("seclabelinterval="+astr.substr(3));
	      else {
		if (!atoi(astr.substr(2).c_str()))
		  goto switch_error;
		opts.push_back("framestep="+astr.substr(2));
	      }
              cmd += sp+astr;
              break;

            case 'P':
              if (!atoi(astr.substr(2).c_str()))
                goto switch_error;
              opts.push_back("precision="+astr.substr(2));
              cmd += sp+astr;
              break;

            case 'M': {
              int m = atoi(astr.substr(2).c_str());
              if (!m)
                goto switch_error;
              stringstream ss;
              ss << pow(double(2.0), double(m));
              opts.push_back("multiply="+ss.str());
              cmd += sp+astr;
              break;
            }

            case 'X':
              if (argc<2)
                goto switch_error;
          
              argc--;
              pos++;
              cmd += sp+astr+sp+argv[pos];
              segmfile = argv[pos];
              break;

            case 'i':
              {
                int t = astr[2]==0 ? 0 : astr[2]=='i' ? 1 : -1;
                if (t==-1)
                  goto switch_error;

		if (t==0) {
		  if (argc<2)
		    goto switch_error;

		  argc--;
		  pos++;
		  cmd += sp+astr+sp+argv[pos];
		  input_pat = argv[pos];

		} else {
		  cmd += sp+astr;
		  input_pat = "%i.seg";
		}
              }

              break;

            case 'I':
              if (argc<2)
                goto switch_error;
          
              argc--;
              pos++;
              cmd += sp+astr+sp+argv[pos];
	      std_naming_pat = argv[pos];
	      std_naming_dir = "";
	      if (astr.size()>3)
		goto switch_error;
	      if (astr.size()==3) {
		std_naming_dir = astr.substr(2);
		if (std_naming_dir!="b" && std_naming_dir!="B")
                  goto switch_error;
	      }
              break;

            case 'o':
              if (argc<2)
                goto switch_error;
          
              argc--;
              pos++;
              cmd += sp+astr+sp+argv[pos];
	      if (astr[2]=='d')
		outdir = argv[pos];
	      else
		specific_out_pat = argv[pos];
              break;

            case 'O':          
              cmd += sp+astr;
              std_out_pat = astr[2]==0   ? 0 : astr[2]=='b' ? 1
                :           astr[2]=='s' ? 2 : astr[2]=='S' ? 3
                :           astr[2]=='R' ? 4 : astr[2]=='x' ? 5 
                :           astr[2]=='r' ? 6 : -1;
              if (std_out_pat==-1)
                goto switch_error;

              break;

            case 'L':
              cmd += sp+astr;
              label_pattern = astr.substr(2);
              break;

            case 'n':
              cmd += sp+astr;
              subdir = astr.substr(2);
              break;

            case 'N':
              cmd += sp+astr;
              ncolon = astr.find(":");
              if (ncolon!=string::npos &&
		  astr.find_first_not_of("0123456789", ncolon+1)==string::npos) {
                fake = astr.substr(2, ncolon-2);
                fakelength = atoi(astr.substr(ncolon+1).c_str());
                if (!fakelength)
                  goto switch_error;
              } else 
                fake = astr.substr(2);
              break;

            default:
              goto switch_error;
            }

          } else // astr[0] != '-'
            if (argc>1) {
              fname = astr;
              cmd += sp+fname;
              if (argc>3)
                now_opts = true;

            } else {
              ShowUsage(myname);
              return 2;
            }

        else { // fname != NULL
          if (MethodVerbose() && !cmd_shown) {
            cout << msg << "Feature extraction command:";
            for (size_t i=0; i<argv.size(); i++)
              cout << " " << argv[i];
            cout << endl;
            cmd_shown = true;
          }

          if (now_opts && astr[0]=='-') {
            switch (astr[1]) {
            case 'o':
              if (argc<3)
                goto switch_error;
          
              argc--;
              pos++;
              opts.push_back(argv[pos]);
              cmd += sp + astr + sp + argv[pos];
              goto next_arg;

            default:
              goto switch_error;
            }
          }
          now_opts = false;

	  incore_feature_t *incorep = NULL;
	  if (astr=="" && incore_i!=incore.end()) {
	    incorep = &*incore_i;
	    incore_i++;
	  }

          try {
            if (!ProcessOneFile(fptr, cmd, fname, opts, astr, incorep,
                                first&&print_header&&!quiet,
                                !testing, first&&print_header&&!quiet, !quiet,
                                segmfile, label_pattern, input_pat,
				std_naming_dir, std_naming_pat, first,
				vlen, concat, std_rawin_pat,
                                std_out_pat, specific_out_pat, raw_out, fake,
				fakelength, outdir, subdir,
                                filelist, result, outfile)) {
	      retval = 1;
	      if (!continue_after_error)
		return retval;
	      else
		cerr << "Feature::Main() : continuing even though "
		     << fname << " on <" << astr << "> failed" << endl;
	    }
          } 
          catch (const string& excp) {
            bool is_file_err = excp.find("::read_frame_impl()")!=string::npos ||
              (excp.find("open_read_impl(")!=string::npos &&
              excp.find("application/x-empty")!=string::npos);
            if (!allow_file_errors || !is_file_err)
              throw(excp);
          }

          if (fptr) {  // fptr==NULL if concat and not enough files yet...
            bool can_reuse = fptr->ReInitializable();
            if ((!can_reuse || force_no_reuse) &&
                (can_reuse || !force_reuse) && !fptr->IsBatchOperator()) {
              if (MethodVerbose())
                cout << msg << "Destroying method data" << endl;
              delete fptr;
              fptr = NULL;
            }

            first = false;
          }
        }

      next_arg:
        argc--;
        pos++;
      }

      if (fptr && fptr->IsBatchOperator() && !fptr->ProcessBatch())
	retval = 1;
      
      delete fptr;

      if (MethodVerbose() && result)
        cout << msg << "Returning in-core results: "
             << ResultSummaryStr(*result) << endl;

      return retval;

    switch_error:
      cerr << myname << " : switch \"" << argv[pos] << "\" not understood"
           << endl;
      ShowUsage(myname);
      return 2;
    }
    catch (const char *txt) {
      cerr << "ERROR: " << txt << endl;
      return 3;
    }
    catch (const string& str) {
      cerr << "string-ERROR: " << str << endl;
      return 3;
    }
    catch (const std::exception& e) {
      cerr << "std-ERROR: " << e.what() << endl;
      return 3;
    }
  }

  //---------------------------------------------------------------------------

  bool Feature::ProcessOneFile(Feature*& f,
                               const string& cmd, const string& feat,
                               const list<string>& opts_in,
                               const string& one_file_in,
			       incore_feature_t *incorep,
                               bool print_xml, bool print_all_xml,
                               bool print_vl,
                               bool print_data, const string& segmfile_in,
                               const string& label_pattern,
                               const string& input_pat,
                               const string& std_naming_dir,
                               const string& std_naming_pat,
                               bool first, int& vector_length, size_t concat,
                               int std_rawin_pat, int std_out_pat,
                               const string& specific_out_pat,
                               bool raw_out, const string& fake, int fakelength,
                               const string& outdir, const string& subdir,
			       vector<string>& filelist,
                               feature_result *result,
                               pair<string,ofstream>& outfile) {

    if (FileVerbose())
      cout << "ProcessOneFile() beginning" << endl;

    string one_file_x = one_file_in, opts_extra;
    if (one_file_x.find('<')!=string::npos &&
	one_file_x.find('>')==one_file_x.size()-1) {
      size_t p = one_file_x.find('<');
      opts_extra = one_file_x.substr(p+1, one_file_x.size()-p-2);
      one_file_x.erase(p);
    }

    string all_files = one_file_x, focus_file = one_file_x;

    if (concat) {
      if (one_file_in.find_first_of("<>")!=string::npos) {
	cerr << "ProcessOneFile() : < or > detected in filename"
	     << " while concat=true" << endl;
	return false;
      }
	
      filelist.push_back(one_file_in);
      while (filelist.size()>concat)
        filelist.erase(filelist.begin());

      size_t mp = (concat-1)/2, nxtra = 0;
      for (size_t i=0; i<filelist.size(); i++)
        if (filelist[i].substr(0, 3)=="../")
          nxtra++;
        else
          break;
      
      bool skip = filelist.size()<=mp+nxtra;
      if (filelist.size()>mp && filelist[mp].substr(0, 3)=="../")
        skip = true;

      if (!skip)
        focus_file = filelist[mp-concat+filelist.size()];

      all_files = "";
      for (size_t i=0; i<filelist.size(); i++)
        all_files += (all_files=="" || filelist[i]=="" ? "" : ",")+filelist[i];

      if (FileVerbose()) {
        cout << "concat=" << concat << " mp=" << mp << " nxtra=" << nxtra
             << " skip=" << skip << " focus_file=" << focus_file
             << " all_files=" << all_files;
        for (size_t i=0; i<filelist.size(); i++)
          cout << " [" << filelist[i] << "]";
        cout << endl;
      }

      if (skip)
        return true;
    }

    string all_files_or_incore = all_files;
    if (all_files_or_incore=="" && incorep) {
      if (incorep->first.first=="picsom::imagedata*")
	all_files_or_incore = "/dev/null/incore/imagedata/"
	  +incorep->first.second;
      else if (incorep->first.first=="string::filename*") {
	string *p = NULL;
	sscanf(incorep->first.second.c_str(), "%p", &p);
	all_files_or_incore = all_files = *p;
      } else
	throw string("incore data type <")+incorep->first.first+">";
    }

    string segmfile = segmfile_in;

    if (input_pat!="" || std_naming_pat!="") {
      segmentfile image;

      image.SetInputPattern(input_pat.c_str());

      if (std_naming_pat!="") {
	// until 2011-12-16 this was unconditional. why?
	image.SetStandardInputNaming(std_naming_pat, std_naming_dir);
      }

      // is it really aimed so that the same segmfile which is set in the
      // first call to ProcessOneFile() is used in the following calls???
      if (segmfile=="")
        segmfile = image.FormInputFileName(image.getInputPattern(),
                                           all_files, std_naming_pat);
    }
    
    if (FileVerbose())
      cout << "Using input segmentation [" << segmfile << "]" << endl;

    list<string> opts_eff = opts_in;
    if (opts_extra!="") { // obs! now assume it can be only one, ie. framerange=a-b
      if (FileVerbose())
	cout << "Adding extra options [" << opts_extra << "]" << endl;
      
      opts_eff.push_back(opts_extra);
    }

    if (!f) {
      if (MethodVerbose()) {
	const string& ocv = OCVversion();
	cout << "OCV " << (ocv==""?"not available":"version "+ocv)
	     << endl
	     << "Creating method data" << endl;
      }

      f = CreateMethod(feat, all_files_or_incore, segmfile, opts_eff);
      if (!f) {
	if (MethodVerbose())
	  cout << "CreateMethod() failed with <" << feat << "> for <"
	       << one_file_in << ">" << endl;
	return false;
      }

      f->SetStandardLabelPattern();
      if (label_pattern!="")
        f->SetLabelPattern(label_pattern);

      if (std_rawin_pat!=-1)
        f->SetStandardRawInputNaming(std_rawin_pat);

      if (std_out_pat!=-1)
        f->SetStandardOutputNaming(std_out_pat, raw_out);

      if (specific_out_pat!="")
        f->SetOutputPattern(specific_out_pat);

    } else {
      if (MethodVerbose())
        cout << "Reinitializing method data" << endl;

      if (!f->ReInitialize())
        throw string("ReInitialize() failed");

      f->Initialize(all_files, segmfile, NULL, opts_eff, false);
    }

    // if (f->IsBatchOperator()) {
    //   if (FileVerbose())
    // 	cout << "  IsBatchOperator()==true, collecting..." << endl;
    //   return f->CollectBatch(all_files, incorep, result); // was one_file_x
    // }
    
    string segmtmp;

    if (std_naming_pat=="")
      segmtmp = f->GetSegmentationMethodName();
    else {
      segmtmp = std_naming_pat;
      // cout << "segmtmp now " << segmtmp << endl;
      f->SetSegmentationMethodString(segmtmp);
    }

    if (concat) {
      f->ForcedLabel(StripFilename(focus_file));
      stringstream ss;
      ss << "cc" << concat << (segmtmp!=""?"+":"") << segmtmp;
      segmtmp = ss.str();
    }

    f->SetOutdir(outdir);
    f->SetSubdir(subdir);
    
    string featfname = fake!="" ? fake : feat;
    f->SetRawInputFilename(featfname, all_files);
    f->SetOutputFilename(segmtmp, featfname, all_files);
    bool new_file = false;
    if (!f->OpenOutputFile(outfile, new_file)) {
      if (FileVerbose())
        cout << "ProcessOneFile() ending in file opening error" << endl;

      return false;
    }
    if (fake!="")
      f->SetFakeName(fake);

    if (f->IsBatchOperator()) {
      if (FileVerbose())
	cout << "  IsBatchOperator()==true, collecting..." << endl;
      return f->CollectBatch(all_files, incorep, result); // was one_file_x
    }
    
    if (print_xml || (new_file&&!first))
      f->SetDescriptionData(true, print_all_xml, cmd);
    else
      f->SetDescriptionData(false, false, "");

    f->SetVectorLengthFake(fakelength);

    if (!print_vl)
      f->SetVectorLength(vector_length);

    f->SetCheckVectorLength(true);
    f->SetPrintVectorLength(print_vl || (new_file&&!first));

    if (result)
      result->xml = f->FormDescription(true);

    bool ok = true;

    try {
      if (print_data)
	f->CalculateAndPrintAllPreProcessings();

    } catch (const string& err) {
      ok = false;
      cerr << "ERROR in CalculateAndPrintAllPreProcessings() with <"
	   << one_file_in << "> : " << err << endl;
    }
    
    if (print_vl)
      vector_length = f->GetVectorLength();

    if (result) {
      if (FileVerbose())
        cout << "Calculated in-core results: "
             << ResultSummaryStr(f->FeatureResult()) << endl;
      result->append(f->FeatureResult());
      // result->xml = f->FormDescription(true);
    }

    if (FileVerbose())
      cout << "ProcessOneFile() ending" << endl;

    return ok;
  }

  //---------------------------------------------------------------------------

  void Feature::ShowUsage(const string& n) {
    cerr << "USAGE : " << n
         << " [<-switches> ...] <feature> [<-o option=value> ...] <files> ..."
         << endl;
    cerr << "  <-switches>:" << endl;
    cerr << "    -gpu : use GPU" << endl;
    cerr << "    -l  : list features" << endl;
    cerr << "    -lx : list features in XML format" << endl;
    cerr << "    -h  : help on feature's or features' options" << endl;
    cerr << "    -hx : description of feature or features in XML format"<<endl;
    cerr << "    -q  : be quiet" << endl;
    cerr << "    -x  : omit header" << endl;
    cerr << "    -d  : increase debugging" << endl;
    cerr << "    -D  : increase debugging in Segmentation class" << endl;
    cerr << "    -m  : increase debugging in imagefile class" << endl;
    cerr << "    -dm : debug memory usage" << endl;
    cerr << "    -t  : testing with limited XML header" << endl;
    cerr << "    -tt : run in individual threads (has to be 1st)" << endl;
    cerr << "    -b  : force calculation for background" << endl;
    cerr << "    -T  : parse region hierarchy from XML" << endl;
    cerr << "    -v  : calculate virtual regions specified in XML" << endl;
    cerr << "    -k  : calculate interest points specified in XML" << endl;
    cerr << "    -keep : keep temp files" << endl;
    cerr << "    -p <predicate> : pair regions by <predicate>" << endl; 
    cerr << "    -u  : force recreation of feature data between files" <<endl; 
    cerr << "    -U  : force reuse of feature data between files" << endl; 
    cerr << "    -e  : ignore broken image files" << endl; 
    cerr << "    -W<c|s|a|p|j|x|d>   : within frame treatment" << endl;
    cerr << "    -B<c|s|a|p|j|x|d>   : between frame treatment" << endl;
    cerr << "    -S<c|s|a|p|j|x|d>   : between slice treatment" << endl;
    cerr << "    -Z<1|5|h|v|wXh|wxh> : zoning" << endl;
    cerr << "    -z<num>  : slicing" << endl;
    cerr << "    -C<num>  : concatenation" << endl;
    cerr << "    -s<num>  : framestep (default: 1)" << endl;
    cerr << "    -sL<num> : second label interval" << endl;
    cerr << "    -P<num>  : precision (default: 7)" << endl;
    cerr << "    -M<num>  : multiply pixel values by 2^num" << endl;
    cerr << "    -Q<rgb|rgba|grey|2|4> : pixel model" << endl;
    cerr << "    -X <segmentation file>" << endl;
    cerr << "    -i <segmentation file pattern>" << endl;
    cerr << "    -ii : equal to -i %i.seg" << endl;
    cerr << "    -I <std segmentation file pattern>" << endl;
    cerr << "    -Ib <std segmentation file pattern> (uses %b dir)" << endl;
    cerr << "    -IB <std segmentation file pattern> (uses %B dir)" << endl;
    cerr << "    -O  : send output to file features/%m:%f%o%h.dat" << endl;
    cerr << "    -Ob : send output to file %b/features/%m:%f%o%h.dat" << endl;
    cerr << "    -Os : send output to file features/%i-%m:%f%o%h.dat" << endl;
    cerr << "    -OS : send output to file %b/features/%i-%m:%f%o%h.dat"<<endl;
    cerr << "    -Or : send output to file %r/features/%m:%f%o%h.dat" << endl;
    cerr << "    -OR : send output to file %r/features/%i-%m:%f%o%h.dat"<<endl;
    cerr << "    -Ox : suppress all normal output" << endl;
    cerr << "    -o <pattern> : send output to specified file (pattern)"<<endl;
    cerr << "    -od <dir> : send output to specified directory %r or %b"<<endl;
    cerr << "    -root=DIR : set root dir" << endl; 
    cerr << "    -r  : output raw data" << endl; 
    cerr << "    -Rx : input raw data, x is any of [ bsSrRx]" << endl; 
    cerr << "    -L<pattern> : set label naming"<< endl;
    cerr << "    -N<name> : use fake feature name; <name:dim> sets also fake dimensionality"<< endl;
    cerr << "    -n<subdir> : subdir that should be ignored when processing %r pattern"<< endl;
    cerr << "    -cn, -cN0 : turn cacheing off (default on)"<< endl;
    cerr << "    -kmeans <file> <k> <niter>"<< endl;
    cerr << "PATTERNS:" << endl;
    cerr << "    %i    -- filename part" << endl;
    cerr << "    %m:%i -- segmentationmethod:filenumber (default)" << endl;
    cerr << "    %f    -- feature name, possibly faked" << endl;
    cerr << "    %o    -- feature options" << endl;
    cerr << "    %h    -- histogram specification" << endl;
    cerr << "    %b    -- directory of input file" << endl;
    cerr << "    %r    -- root directory (parent directory of %b"
         << " if it ends with .d, otherwise same as %b)" << endl;
    cerr << "    Just -L means empty pattern, that is no label is given."
         << endl;
  }

  //---------------------------------------------------------------------------

  pair<bool,int> Feature::SolveVerbosity(const string& s) {
    size_t n = 0;
    while (n<s.size() && s[n]==s[0])
      n++;

    if (n==s.size())
      return pair<bool,int>(true, n);

    return  pair<bool,int>(false, atoi(s.substr(n).c_str()));
  }

  //-----------------------------------------------------------------------------

  Feature *Feature::Initialize(const string& img, const string& seg,
			       segmentfile *s, const list<string>& opt,
			       bool allocate_data) {
    string msg = "Feature::Initialize() "+ThreadIdentifierUtil()+" : ";

    if (seg!="" && s)
      throw msg+"seg and s not allowed simultaneously";
  
    if (img!="" && s)
      throw msg+"img and s not allowed simultaneously";
  
    if (img=="" && !s)
      return this;

#ifndef __MINGW32__
    tics.set_name(Name());
    // tics.start();
    cox::tictac::func tt(tics, "Feature::Initialize");
#endif

    incore_imagedata_ptr = NULL;
    if (FileVerbose())
      cout << msg << "INCORE imagedata removed" << endl;

    if (FileVerbose())
      cout << msg << "starting..." << endl;

    // this is the place where mats moved it 11 Nov 2004 ...
    SetAndProcessOptions(opt, false);
  
    // after 2014-01-23 this should happen with audio features...
    if (!IsImageFeature() && !IsVideoFeature() /*&& !IsAudioFeature()*/) {
      if (FileVerbose())
	cout << " non-image feature, calling LoadObjectData()" << endl;

      LoadObjectData(img, seg); // this function should be overriden 
      // by non-image-based child classes
    } else {

      if (!ImageReadable() && img!="") {
	if (FileVerbose())
	  cout << " non-readable image, calling segmentfile()" << endl;

	allocated_segmentfile = new segmentfile(img, seg,
						NULL, NULL, NULL,
						false); // don't read 'img'
	segmentfile_ptr = allocated_segmentfile;

	if (IsVideoFeature()) {
	  videofile_ptr = new videofile(img);
	  if (!videofile_ptr->has_video())
	    throw msg+"bad video file!";
	}
	if (IsAudioFeature()) { // after 2014-01-23 this should not happen...
	  //#ifdef PICSOM_USE_AUDIO
	  deprecated_audiofile_ptr = new soundfile;
	  deprecated_audiofile_ptr->open(img);
	  if (!deprecated_audiofile_ptr->hasAudio())
	    throw msg+"bad audio file!";
	  //#else
	  //throw msg+"PICSOM_USE_AUDIO not defined!";
	  //#endif // PICSOM_USE_AUDIO
	}

      } else if (img!="" || seg!="") {
	string imgeff = img;
	if (imgeff.find("/dev/null/incore/imagedata/")==0) {
	  imgeff = "";
	  string addr = img.substr(27);
	  sscanf(addr.c_str(), "%p", &incore_imagedata_ptr);
	  if (FileVerbose())
	    cout << "INCORE imagedata given @ "
		 << ToStr(incore_imagedata_ptr) << endl;
	}

	if (FileVerbose())
	  cout << " image or segments specified, calling segmentfile("
	       << imgeff << "," << seg << ")" << endl;

	segmentfile_ptr = allocated_segmentfile = new segmentfile(imgeff, seg);
	imagefile *imgfile = segmentfile_ptr->inputImageFile();

	if (incore_imagedata_ptr) {
	  imagedata seg(incore_imagedata_ptr->width(),
			incore_imagedata_ptr->height(), 
			1, imagedata::pixeldata_uint32);
	  segmentfile_ptr->setImage(*incore_imagedata_ptr);
	  segmentfile_ptr->setSegment(seg);
	}
	
	if (imgeff!="" && imgfile->nframes()==0) {
	  if (FileVerbose())
	    cout << " imagefile <" << img << "> broken, no valid frames" << endl;
	  // delete allocated_segmentfile; obs! ???
	  return NULL;
	}

	if (imgfile && imgfile->impl_version().find("opencv")!=string::npos)
	  opencvptr = imgfile->impl_data();

	if (FileVerbose() && imgfile) {
	  cout << " imagefile info: " << imgfile->info();
	  if (opencvptr)
	    cout << " has OpenCV data @ " << opencvptr;
	  cout << endl;
	}

      } else {
	segmentfile_ptr = s;
	if (!segmentfile_ptr) {
	  if (FileVerbose())
	    cout << " no image NOR segments NOR pointer specified, ending..."
		 << endl;

	  return this;
	}
      
	if (FileVerbose())
	  cout << " image NOR segments, pointer set" << endl;
      }
    }

    VideoSegmentExtension(".mpg");  // this was hardcoded until 2015-10-29

    // this is the place where it was until mats moved it 11 Nov 2004 ...
    SetAndProcessOptions(opt, true);

    if (BetweenFrameTreatment()==treat_pixelconcat) {
      if (FileVerbose())
	cout << msg << "BetweenFrameTreatment()==treat_pixelconcat" << endl;
      EnsureImage();
      // image->ReadAllFramesAtOnce(true); // default is always false
    }
  
    if (IsImageFeature()) {
      PossiblySetSlicingAndZoning();
      SetFrameCacheing();
    }

    if (IsVideoFeature() || IsAudioFeature())
      PossiblySetSlicing();
  
    if (!rotateinfo.is_rotate_set()) {
      ResolveScaling(scaling);
    }

    if (allocate_data) {
      throw msg+"allocate_data==true strongly deprecated now";
      // SetDataLabelsAndIndices(false);
      // AddDataElements(false);
    }

    if (FileVerbose())
      cout << msg << "ending..." << endl;
  
    return this;
  }

  //===========================================================================
  
  void Feature::EnsureImage() const { // not really const anymore ...
    if (!HasImage() && incore_imagedata_ptr) {
      SegmentData()->setImage(*incore_imagedata_ptr);
      imagedata tmpimg(incore_imagedata_ptr->width(),
		       incore_imagedata_ptr->height(),
		       1, imagedata::pixeldata_uint32);
      SegmentData()->setSegment(tmpimg);

      if (FileVerbose())
	cout << "Feature::EnsureImage() : copied from imagedata @ "
	     << ToStr(incore_imagedata_ptr) << endl;

      // imagefile::display(*incore_imagedata_ptr);
    }

    if (!HasImage())
      abort();
    // throw "Feature : no image";
  }

  //===========================================================================
  
  const string& Feature::PicSOMroot() {
    if (picsomroot=="") {
      const char *e = getenv("PICSOM_ROOT");
      if (e)
	picsomroot = e;
      if (picsomroot=="")
	picsomroot = "/share/imagedb/picsom";
      if (!DirectoryExists(picsomroot))
	picsomroot = "/triton/ics/project/imagedb/picsom";
      if (!DirectoryExists(picsomroot))
	picsomroot = ".";
    }
    return picsomroot;
  }

  //===========================================================================

  void Feature::SetAndProcessOptions(const list<string>& l, bool final) {
    if (MethodVerbose())
      cout << "Feature::ProcessOptions() name=" << Name()
	   << " final=" << final << endl;

    if (!final) {
      // obs! is this the only one that might need reinitialization 
      //      or is this a wrong palce to do this?
      framerange = make_pair(0, 0); 

      options = options_unprocessed = l;

      if (!ProcessPreOptionsAndRemove(options_unprocessed))
	throw "ProcessPreOptionsAndRemove() failed";

      return;
    }    

    if (!ProcessOptionsAndRemove(options_unprocessed))
      throw "ProcessOptionsAndRemove() failed";
    
    if (options_unprocessed.size()) {
      string msg = "SetAndProcessOptions() not all options processed :";
      for (list<string>::const_iterator i = options_unprocessed.begin();
	   i!=options_unprocessed.end(); i++)
	msg += string(" [")+ *i + "]";
      throw msg;
    }
  }

  //===========================================================================

  bool Feature::ProcessPreOptionsAndRemove(list<string>&) {
    static const string msg = "Feature::ProcessPreOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    return true;
  }

  //===========================================================================

  bool Feature::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "Feature::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value, args;
      SplitOptionString(*i, key, value);

      size_t p = key.find('(');
      if (p!=string::npos && key[key.size()-1]==')') {
	args = key.substr(p+1, key.size()-p-2);
	key.erase(p);
      }

      if (args!="" && key!="region") {
	cerr << msg << "-o key(args)=val possible only when key=region"
	     << endl;
	return false;
      }
    
      if (key=="zoning") {
	UsedZoningText(value);
	i = l.erase(i);
	continue;
      }

      if (key=="slicing") {
	UsedSlicingText(value);
	i = l.erase(i);
	continue;
      }

      if (key=="framerange") {
	size_t p = value.find('-');
	if (p==string::npos) {
	  cerr << msg << "-o framerange=first-last failed tofind dash in \"" << value << "\""
	       << endl;
	  return false;
	}
	framerange.first  = atoi(value.substr(0, p).c_str());
	framerange.second = atoi(value.substr(p+1).c_str());
	i = l.erase(i);
	continue;
      }

      if (key=="margin") {
	margin_start_set = margin_end_set = atoi(value.c_str());
	i = l.erase(i);
	continue;
      }

      if (key=="pixeltype") {
	PixelType(value);
	i = l.erase(i);
	continue;
      }

      if (key=="multiply") {
	PixelMultiply(atof(value.c_str()));
	i = l.erase(i);
	continue;
      }

      if (key=="background") {
	if (IsBoolean(value))
	  use_background = IsAffirmative(value);
	else
	  throw msg+"background !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="rawinmode") {
	if (IsBoolean(value))
	  raw_in_mode = IsAffirmative(value);
	else
	  throw msg+"rawoutmode !(true|false)";
      
	if (raw_in_mode && !HasRawInMode())
	  throw msg+"rawinmode not available";

	i = l.erase(i);
	continue;
      }

      if (key=="rawoutmode") {
	if (IsBoolean(value))
	  raw_out_mode = IsAffirmative(value);
	else
	  throw msg+"rawoutmode !(true|false)";
      
	if (raw_out_mode && !HasRawOutMode())
	  throw msg+"rawoutmode not available";

	i = l.erase(i);
	continue;
      }

      if (key=="regionhierarchy") {
	if (IsBoolean(value))
	  region_hierarchy = IsAffirmative(value);
	else
	  throw msg+"regionhierarchy !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="interestpoints") {
	if (IsBoolean(value))
	  interest_points = IsAffirmative(value);
	else
	  throw msg+"interestpoints !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="virtualregions") {
	if (IsBoolean(value))
	  virtual_regions = IsAffirmative(value);
	else
	  throw msg+"virtualregions !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="virtualonly") {
	if (IsBoolean(value)){
	  virtual_regions = IsAffirmative(value);
	  virtual_only=IsAffirmative(value);
	}
	else
	  throw msg+"virtualonly !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="pairbypredicate") {
	pair_by_predicate = value;
	i = l.erase(i);
	continue;
      }

      if (key=="framestep") {
	// there should be a way to use the framestep stored in .seg file
	framestep = atoi(value.c_str());
	i = l.erase(i);
	continue;
      }

      if (key=="seclabelinterval") {
	sec_label_interval = atof(value.c_str());
	i = l.erase(i);
	continue;
      }

      if (key=="precision") {
	precision = atoi(value.c_str());
	i = l.erase(i);
	continue;
      }

      if (key=="withinframe") {
	WithinFrameTreatment(TreatmentType(value));
	i = l.erase(i);
	continue;
      }

      if (key=="betweenframe") {
	BetweenFrameTreatment(TreatmentType(value));
	i = l.erase(i);
	continue;
      }

      if (key=="betweenslice") {
	BetweenSliceTreatment(TreatmentType(value));
	i = l.erase(i);
	continue;
      }

      if (key=="framepreprocess" || key=="preprocess") {
	SetFramePreProcessing(value);
	i = l.erase(i);
	continue;
      }

      if (key=="regionpreprocess") {
	SetRegionPreProcessing(value);
	i = l.erase(i);
	continue;
      }

      if (key=="z-normalize") {
	if (!SetZnormalize(value))
	  throw msg+"processing z-normalize="+value+" failed";
	i = l.erase(i);
	continue;
      }

      if (key=="keeptmp") {
	keeptmp = IsAffirmative(value);
	i = l.erase(i);
	continue;
      }

      if (key=="root") {
	PicSOMroot(value);
	i = l.erase(i);
	continue;
      }

      int kl = key.length();
      if (key=="width"  || (kl>6 && key.substr(kl-6)==".width") ||
	  key=="height" || (kl>7 && key.substr(kl-7)==".height") ||
	  key=="scale") {
	scaling = *i;
	i = l.erase(i);
	continue;
      }

      if (key=="region") {
	AddRegionDescriptor(value, args);
	i = l.erase(i);
	if (FileVerbose())
	  cout << msg << " added region spec [" << region_descr.back().first
	       << "," << region_descr.back().second << "]" << endl;
	continue;
      }

      if (key=="rotatedbox") {
	ResolveRotating(*i);
	i = l.erase(i);
	continue;
      }

      if (key=="histogram") {
	histogramMode = value;
	i = l.erase(i);
	continue;
      }

      if (key=="histbins") {
	histogramMode = "codebook";
	readHistogramBins(value);
	i = l.erase(i);
	continue;
      }

      if (key=="histzonenorm") {
	if (IsBoolean(value)) 
	  if (IsAffirmative(value)) {
	    hist_zone_norm = hist_norm_L1;
	  } else {
	    hist_zone_norm = hist_norm_none;
	  }
	else
	  hist_zone_norm = HistogramNormalization(value);
	i = l.erase(i);
	continue;
      }

      if (key=="histlayernorm") {
	if (IsBoolean(value)) 
	  if (IsAffirmative(value)) {
	    hist_layer_norm = hist_norm_L1;
	  } else {
	    hist_layer_norm = hist_norm_none;
	  }
	else
	  hist_layer_norm = hist_norm_none;
	i = l.erase(i);
	continue;
      }

      if (key=="histglobalnorm") {
	if (IsBoolean(value)) 
	  if (IsAffirmative(value)) {
	    hist_global_norm = hist_norm_L1;
	  } else {
	    hist_global_norm = hist_norm_none;
	  }
	else
	  hist_global_norm = hist_norm_none;
	i = l.erase(i);
	continue;
      }

      if (key=="cacheing") {
	if (IsBoolean(value))
	  cacheing = IsAffirmative(value);
	else
	  throw msg+"cacheing !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="suppressborders") {
	if (sscanf(value.c_str(), "%d", &suppressborders) != 1)
	  throw msg+"couldn't parse border suppression margin";
	i = l.erase(i);
	continue;
      }

      i++;
    }

    if (HasScaling() && !RegionDescriptorCount())
      throw msg + " : cannot be scaling and whole image region simultaneously";

    if (IsImageFeature() || IsVideoFeature()) // if added 2014-01-23
      SetRegionSpecifications();

    return true;
  }

  //===========================================================================

  bool Feature::SetRegionSpecifications() {
    string msg = "Feature::SetRegionSpecifications() : ";

    RemoveRegionSpec();

    int f = SegmentData()->getCurrentFrame();

    vector<pair<string,string>> region_descr_new;

    for (auto i=region_descr.begin(); i!=region_descr.end(); i++) {
      string expand;
      try {
	rectangularRegion *rr = new rectangularRegion(i->first, SegmentData());
	region_spec.push_back(make_pair(rr, i->second));
	region_descr_new.push_back(*i);

      } catch (...) {
	SegmentationResultList *r
	  = SegmentData()->readFrameResultsFromXML(f, i->first, "box", true);
	for (auto j=r->begin(); j!=r->end(); j++) {
	  if (LabelVerbose()) {
	    cout << "  NAME:     " << j->name  << endl;
	    cout << "  TYPE:     " << j->type  << endl;
	    cout << "  VALUE:    " << j->value << endl;
	    cout << "  METHODID: " << j->methodid << endl;
	    cout << "  RESULTID: " << j->resultid << endl << endl;
	  }
	  rectangularRegion *rr = new rectangularRegion(j->name, SegmentData());
	  region_spec.push_back(make_pair(rr, i->second));
	  region_descr_new.push_back(make_pair(j->name, i->second));
	  expand += (j==r->begin()?" => ":", ")+j->name;
	}
	delete r;
	if (expand=="") {
	  if (FrameVerbose())
	    cout << msg << "failed to expand \"" << i->first << "\"" << endl;
	  return false;
	}
      }

      if (FrameVerbose())
	cout << msg << "added " << i->first << "," << i->second << expand
	     << endl;
    }

    region_descr = region_descr_new;

    return true;
  }

  //===========================================================================

  void Feature::SetRegionSpecificationWholeImage() {
    string msg = "Feature::SetRegionSpecificationWholeImage() : ";

    region_descr.clear();

    char tmp[100];
    int w = Width(), h = Height();
    sprintf(tmp, "(%d,%d):%dx%d", (w-1)/2, (h-1)/2, w, h);
    AddRegionDescriptor(tmp, "");
    SetRegionSpecifications();

    if (FileVerbose())
      cout << msg << " added whole image [" << tmp << "]" << endl;
  }
  
  //===========================================================================

  imagedata Feature::RegionImage(size_t r) const {
    string msg = " Feature::RegionImage("+ToStr(r)+") : ";

    if (LabelVerbose())
      cout << msg << "called" << endl;

    try {
      const string& spec = accessRegion(r).second;
      const Region& reg = *accessRegion(r).first;
      const rectangularRegion& rr
	= *dynamic_cast<const rectangularRegion*>(&reg);

      size_t s = rr.size();
      int tlx, tly, brx, bry;
      rr.xy(0,   tlx, tly);
      rr.xy(s-1, brx, bry);

      if (LabelVerbose())
	cout << msg << "  spec=" << spec << " tlx=" << tlx
	     << " tly=" << tly << " brx=" << brx << " bry=" << bry << endl;

      int sw = brx-tlx+1, sh = bry-tly+1, dw = brx-tlx+1, dh = bry-tly+1;
      int xo = tlx, yo = tly;

      if (spec=="of2lfw") {
	float nw = 112, nh = 112;
	int w = brx-tlx+1, h = bry-tly+1;
	float scale = sqrt(float(w)*h/(nw*nh));

	int x0 = (int)floor(tlx-0.5*scale*(250-nw)+0.5);
	int x1 = (int)floor(brx+0.5*scale*(250-nw)+0.5);
	int y0 = (int)floor(tly-0.5*scale*(250-nh)+0.5);
	int y1 = (int)floor(bry+0.5*scale*(250-nh)+0.5);

	if (LabelVerbose())
	  cout << msg << "   ==> x0=" << x0 << " x1=" << x1
	       << " y0=" << y0 << " y1=" << y1 << endl;

	sw = x1-x0+1;
	sh = y1-y0+1;
	dw = dh = 250;
	xo = x0;
	yo = y0;

      } else if (spec!="") {
	cerr << msg << "spec=<" << spec << "> not understood" << endl;
	return imagedata();
      }

      if (LabelVerbose())
	cout << msg << "   ==> sw=" << sw << " sh=" << sh
	     << " dw=" << dw << " dh=" << dh
	     << " xo=" << xo << " yo=" << yo << endl;
      
      int base = SegmentData()->getCurrentFrame();
      imagedata img = *SegmentData()->accessFrame(base);

      try {
	scalinginfo si(sw, sh, dw, dh, xo, yo);
	si.stretch(true);
	img.rescale(si);    

      } catch (const string& e) {
	cerr << msg+e << endl;
	return imagedata();
      }

      int f = SegmentData()->getCurrentFrame();
      RegionPreProcess(img, f);

      return img;

    } catch (...) {
      cerr << msg << "FAILED" << endl;
      return imagedata();
    }
  }

  //===========================================================================

  vector<int> Feature::SegmentVector(int f, int x, int y) const {
    vector<int> ret;

    if (region_spec.empty()) {
      int s = -1;
      SegmentData()->get_pixel_segment(f, x, y, s);
      if (PixelVerbose())
	cout << "Feature::SegmentVector(" << f << ","
	     << x << "," << y << ") (no region_spec) -> " << s << endl;
      ret.push_back(s);
      for (size_t i=0; i<composite_regions.size(); i++)
	if (composite_regions[i].primitiveRegions.count(s))
	  ret.push_back(composite_regions[i].id);

    } else {
      for (size_t i=0; i<region_spec.size(); i++) {

	if (region_spec[i].first->contains(x, y)) // f not used!
	  ret.push_back(i+1); // FIRST_REGION_BASED_SEGMENT==1

	if (PixelVerbose())
	   cout << "region[" << i << "] " << (string)*region_spec[i].first
		<< " (" << (void*)region_spec[i].first << ") : "
		<< region_spec[i].first->contains(x, y)
		<< endl;

	if (false && region_spec[i].first->contains(x, y))
	  TrapMeHere();

      }
      if (ret.empty())
	ret.push_back(0);
    }

    if (PixelVerbose()) {
      cout << "Feature::SegmentVector(" << f << ","
	   << x << "," << y << ") -> [";
      for (size_t i=0; i<ret.size(); i++)
	cout << ret[i] << (i<ret.size()-1?" ":"");
      cout << "]" << endl;
    }

    return ret;
  }

//=============================================================================

void Feature::OpenFiles(const string& img, const string& seg) {
  if (FileVerbose())
    cout << "Feature::OpenFiles(" << img << "," << seg << ")" << endl;

  const string& pat = ""; // Segmentation::InputPattern();

  if (img.size() && (seg.size()||pat.size())) {
    if (FileVerbose() && pat.size())
      cout << "Feature::OpenFiles(...) : InputPattern()=" << pat << endl;

    SegmentData()->openFiles(img, seg);

    // !!! SetSegments(new SegmentInterface(seg.c_str(), img.c_str()));
    // SetImage(segments);
    if (!HasImage())
      throw string("failed opening image file \"")+img+"\"";
    if (!HasSegments())
      throw string("failed opening segmentation file \"")+seg+"\"";

    if (FileVerbose())
      cout << "Feature::OpenFiles() opened image file [" << img
           << "] and segment file [" << seg << "]" << endl;
    
    return;
  }

  if (img.size()) {
    // !!! SetImage(new SegmentInterface(img.c_str(), SegmentInterface::image_file));

    SegmentData()->openFiles(img);

    if (!HasImage())
      throw string("failed opening image file \"")+img+"\"";

    if (FileVerbose())
      cout << "Feature::OpenFiles() opened image file [" << img << "]" << endl;
  }
  
  if (seg.size()) {
    throw string("OpenFiles() no image name");
    // SetSegments(new SegmentInterface(seg.c_str(), NULL));
    // if (!HasSegments())
    //   throw string("failed opening segmentation file \"")+seg+"\"";
  }
}

  //===========================================================================

  pair<size_t,size_t> Feature::EffectiveFrameRange() const {
    if (framerange.first!=0 && framerange.second!=0) {
      if (SliceVerbose())
	cout << "Feature::EffectiveFrameRange() : "
	     << framerange.first << "-" << framerange.second << endl;
      
      return framerange;
    }

    size_t ms = margin_start_set, me = margin_end_set, n = Nframes();
    for (;;) {
      if (n>ms+me)
	return make_pair(ms, n-1-me);
      if (ms==0 && me==0) {
	stringstream ss;
	ss << "EffectiveFrameRange() failed: nframes=" << Nframes()
	   << " margin_start=" << margin_start_set
	   << " margin_end=" << margin_end_set;
	throw ss.str();
      }
      if (me>0 && me>=ms)
	me--;
      else if (ms>0)
	ms--;
    }
  }

//=============================================================================

void Feature::PossiblySetSlicing() {
#ifndef __MINGW32__
  cox::tictac::func tt(tics, "Feature::PossiblySetSlicing");
#endif

  string st = UsedSlicingText();
  size_t nf = Nframes(), ns;
  pair<size_t,size_t> fse = EffectiveFrameRange();
  size_t enf = fse.second-fse.first+1;
  float frac;
  size_t overlap=0;

  if (st[0] == 's') { // slices given in seconds
    float fps;
    if (IsVideoFeature())
      fps = (float)videofile_ptr->get_frame_rate();
    else if (IsAudioFeature())
      fps = deprecated_audiofile_ptr->getSampleRate();
    else
      throw "Time slices ("+st+
	") only applicable for video and audio features!";
    
    string rest = st.substr(1);
    size_t lpos = rest.find("ls",0);
    if (lpos != string::npos) {
      overlap = (size_t)floor(fps*atof(rest.substr(lpos+2).c_str()));
      rest = rest.substr(0,lpos);
    }
    frac = fps*atof(rest.c_str());
    ns = (size_t)ceil(float(enf)/(frac-overlap));
  } else {
    ns = atoi(st.c_str());
    frac = float(enf)/ns;
  }

  if (ns<1)
    throw string("Feature::PossiblySetSlicing() : ns<1");

  if (SliceVerbose())
    cout << "Feature::PossiblySetSlicing() : UsedSlicingText()=[" << st
         << "] nframes=" << nf << " margin_start=" << margin_start_set
         << " margin_end=" << margin_end_set << " -> eff_nframes=" << enf
         << " (" << fse.first << "," << fse.second << ")" << endl;

  slice_bounds.clear();

  if (ns==1) {
    if (SliceVerbose())
      cout << " 1 slice: #0=[" << fse.first << ".." << fse.second << "]"
           << endl;
    return;
  }

  if (SliceVerbose())
    cout << " " << ns << " slices: ";

  for (size_t sbegin=fse.first, i=0; i<ns; i++) {
    size_t a = fse.first+(int)floor((i+1)*frac)-i*overlap;
    size_t send = a>0?a-1:0;
    if (send<sbegin)
      send = sbegin;
    if (send >= enf) send=enf-1;
    slice_bounds.push_back(make_pair(sbegin, send));

    if (SliceVerbose())
      cout << " #" << i << "=[" << sbegin << ".." << send << "]";

    sbegin = a-overlap;
  }

  if (SliceVerbose())
    cout << endl;
}

//=============================================================================

void Feature::PossiblySetZoning() {
#ifndef __MINGW32__
  cox::tictac::func tt(tics, "Feature::PossiblySetZoning");
#endif

  bool skip = SegmentData()->isPrepared();

  if (FrameVerbose()) { 
    cout << "Feature::PossiblySetZoning() ";
    if (!skip)
      cout << "[" << UsedZoningText() << "]";
    else
      cout << "...skipping...";

    if (incore_imagedata_ptr)
      cout << " INCORE " << incore_imagedata_ptr->info() << endl;
    else {
      int base = SegmentData()->getCurrentFrame();
      imagedata& img = *SegmentData()->accessFrame(base);
      cout << " " << img.info() << endl;
    }
  }

  if (skip)
    return;

  if (!SegmentData()->prepareZoning(UsedZoningText()))
    throw "Feature::PossiblySetZoning() INCORE [" + UsedZoningText() + "] failed";
}

//=============================================================================

void Feature::SetFrameCacheing() {
#ifndef __MINGW32__
  cox::tictac::func tt(tics, "Feature::SetFrameCacheing");
#endif

  if (FrameVerbose()) { 
    cout << "Feature::SetFrameCacheing() " << cacheing << endl;
  }

  SegmentData()->frameCacheing(cacheing);
}

//=============================================================================

/*
void Feature::initialiseLeaf(int argc, const char **argv, bool parent) {
  // this should be called when all the virtual tables 
  // have been set

  if(parent) return;

  zoningString=DefaultZoningText();
  include_zone_zero = false;
  forceLabelsUpTo = 0;
  processOptions(argc, argv);
  allocateFeatureVectors(MAX_PIXEL_LABEL);
  // allocateTemporaryStorage(MAX_PIXEL_LABEL);
}
*/

//=============================================================================

bool Feature::allocateFeatureVectors(int /*maxlabel*/) {
  // featureVectors.resize(maxlabel+1);
  return true;
}

//=============================================================================

void Feature::processOptions(int argc, const char **argv) {
  int i=0;
  while(i<argc){
    int nr=parseOption(argc-i,&argv[i]);
    if(nr<1){
      cerr << "Unknown command line option: " << argv[i] << endl;
      exit(-1);
    }
    i += nr;
  }
}

//=============================================================================

Feature::~Feature() {
  RemoveDataElements();
  RemoveSegmentFile();
  RemoveVideoFile();
  RemoveAudioFile();
  RemoveRegionSpec();
  RemoveIntermediateFile();
  CloseFile();

#ifndef __MINGW32__
  if (tics.is_active())
    tics.summary(true, false);
#endif
}

//=============================================================================

int Feature::parseOption(int argc, const char **argv) {
  if(argv[0][0]=='-')
    switch(argv[0][1]) {
    case 'Z':    
      include_zone_zero = true;
      //      cout << "Parsed option -Z" << endl;
      return 1;
    case 'F':
      if(argc<2) return 0;
      if(sscanf(argv[1],"%d",&forceLabelsUpTo) == 1)
        return 2;
      else
        return 0;
    }
  return 0;
}

//=============================================================================

void Feature::translateBackground() {
  // cout << "Translating zone zero. include_zone_zero="<<
  //  include_zone_zero << endl;

  if(!include_zone_zero){
//     int x,y,w,h,s = 0;
//     w=segments->getImg()->getWidth();
//     h=segments->getImg()->getHeight();

//     for(y=0;y<h;y++) for(x=0;x<w;x++){
//       segments->getImg()->get_pixel_segment(x,y,s);
//       if(s==0)
//         segments->getImg()->set_pixel_segment(x,y,-1);
//     }
   }
}

//=============================================================================

void Feature::PrintInstructions(ostream& str, const string& m) {
  for (const Feature *p = list_of_methods; p; p=p->next_of_methods)
    if (m=="" || m==p->Name()) {
      p->printMethodInstructions(str);
      if (m=="" && p->next_of_methods!=NULL)
        str << endl;
    }
}

//=============================================================================

void Feature::printMethodInstructions(ostream &str) const {
  str << Name() << " : " << ShortText() << endl;

  stringstream strStr;
  printMethodOptions(strStr);
  string optstr = strStr.str();
  if (optstr.size()) {
    str << "  options :" << endl;
    if (optstr.substr(optstr.size()-1, 1)!="\n")
      optstr += "\n";
    size_t p = 0;
    for (;;) {
      size_t n = optstr.find('\n', p);
      cout << "    "
           << (n!=string::npos ? optstr.substr(p, n-p+1) : optstr.substr(p));
      if (n==string::npos)
        break;
      p = n+1;
    }
  }
}

//=============================================================================

int Feature::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=0;
  str << "histogram=fixed|lbg  output histogram of the feature values" << endl 
      << endl;
  str << "suppressborders=<margin> (default -1 -> no suppression)"
      << endl;
  return ret+2;
}

//=============================================================================

bool Feature::refreshLabeling() {
  // this is called when the labeling has been altered and all the features 
  // need to be all recalculated
  throw("Feature::refreshLabeling()");

  // return Calculate();
}

//=============================================================================

/*
bool Feature::moveRegion(int to, int from, const Segmentation::coordList &list)
{

  if (!list.size())
    return 0;
   
  // here we could check if the pixels are actually labeled in the way
  // the argument 'from lets us believe
   
  return updateLabels(to,from,list);
}
*/

//============================================================================ 

/*
bool Feature::moveRegion(int to, int from, const Segmentation::Matrix &matrix){
  Segmentation::coordList *list=GetSegmentation()->convertToList(matrix);
  bool ret=moveRegion(to,from,*list);
  delete list;
  return ret;
}
*/

//============================================================================ 

vector<string> Feature::UsedDataLabelsFromSegmentData() const {
  vector<pair<int,string> > labs = SegmentData()->getUsedLabelsWithText();
  vector<pair<int,string> >::iterator i;

  if (LabelVerbose()) {
    cout << "PixelBased::UsedDataLabels() : " << endl;
    for (i=labs.begin(); i<labs.end(); i++)
      cout << "  " << i->first << " : \"" << i->second << "\"" << endl;
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
    ShowDataLabels("PixelBased::UsedDataLabels", ret, cout);
    
  return ret;
}

//============================================================================ 

vector<string> Feature::SolveUsedDataLabels() const {
  bool debug = false;

#ifndef __MINGW32__
  cox::tictac::func tt(tics, "Feature::SolveUsedDataLabels");
#endif

  if (region_spec.empty()) {
    if (LabelVerbose())
      cout << "Feature::SolveUsedDataLabels(): region_spec empty"<<endl;
    vector<string> ret;
    if (virtual_only==false)
      ret = UsedDataLabels();

    for (size_t i=0; i<composite_regions.size(); i++)    
      ret.push_back(formVirtualSegmentLabel(composite_regions[i].
                                            primitiveRegions));

    return ret;
  }

  // copied from RegionBased:

  vector<string> l;
  int j = 1;  // FIRST_REGION_BASED_SEGMENT==1
  for (auto i=region_spec.begin(); i!=region_spec.end(); i++) {
    string seg = *i->first;

    if (debug) 
      cout << "[" << seg << "] j=" << j << endl;

    if (seg=="") {
      char tmp[100];
      sprintf(tmp, "%d", j);
      l.push_back(tmp);

    } else
      l.push_back(seg);

    j++;
  }
  
  if (LabelVerbose())
    ShowDataLabels("Feature::SolveUsedDataLabels", l, cout);

  return l;
}

//============================================================================ 

void Feature::SetDataLabelsAndIndices(bool erase) {
  if (erase)
    RemoveDataLabelsAndIndices();

  else {
    if (data_labels.size())
      throw "Feature::SetDataLabelsAndIndices() data_labels not empty";

    if (int2index.size())
      throw "Feature::SetDataLabelsAndIndices() int2index not empty";

    if (string2index.size())
      throw "Feature::SetDataLabelsAndIndices() string2index not empty";
  }

  vector<string> labels = SolveUsedDataLabels();

  /*
  // force calculation of specified labels
  int myforceLabelsUpTo = 0;
  vector<int>::iterator it = labels.begin();
  for (i=1; i<=myforceLabelsUpTo; i++){
    while (it != labels.end() && *it<i) { it++;}
    if (it==labels.end() || *it > i)
      it = labels.insert(it, i);
  }
  */

  /*cout << "labels: " << labels.size() << endl;
  for (int i=0;i<labels.size();i++)
  cout << labels[i] << endl; */


  sort(labels.begin(), labels.end()); // later on, something smarter...

  data_labels = labels;  

  map<string,int> composite_labels;

  for (size_t i=0; i<composite_regions.size(); i++){
    composite_labels[formVirtualSegmentLabel(composite_regions[i].
					     primitiveRegions)]
      = composite_regions[i].id;
  }

  for (size_t i=0; i<data_labels.size(); i++) {
    string2index.insert(str_idx_pair(data_labels[i], i));

    if (composite_labels.count(data_labels[i]))
      int2index.insert(int_idx_pair(composite_labels[data_labels[i]], i));

    if (data_labels[i].find_first_not_of("0123456789")==string::npos)
      int2index.insert(int_idx_pair(atoi(data_labels[i].c_str()), i));
    else
      int2index.insert(int_idx_pair(i+1, i)); // see note below!

    // note: this else case was added 2013-03-20
    // this works for calculating PixelBased for -o region=zzz
    // string2index: "face0"->0 "face1"->1
    // int2index: 1->0 2->1 where 0 is background ala SegmentVector()...
  }

  if (LabelVerbose()) {
    cout << "Feature::SetDataLabelsAndIndices() :" << endl << " string2index:";
    for (auto i=string2index.begin(); i!=string2index.end(); i++)
      cout << " \"" << i->first << "\"->" << i->second;
    cout << endl << " int2index:";
    for (auto i=int2index.begin(); i!=int2index.end(); i++)
      cout << " " << i->first << "->" << i->second;
    cout << endl;
  }
}

//============================================================================ 

void Feature::ShowDataLabels(const string& f, const vector<string>& l,
                             ostream& os) {
  os << f << "() :";
  for (vector<string>::const_iterator i=l.begin(); i!=l.end(); i++)
    os << " \"" << *i << "\"";
  os << endl;
}

//============================================================================ 

Feature::featureVector Feature::AggregatePerFrameVector() const {
  EnsureSegmentation();
  
  const vector<string>& labels = DataLabels();

  if (!labels.size())
    throw "AggregatePerFrameVector() no labels in segmentation";

  if (FrameVerbose()) {
    cout << "Feature::AggregatePerFrameVector() labels.size()="
         << labels.size() << " :";
    for (size_t i=0; i<labels.size(); i++)
      cout << (i==0?" ":", ") << labels[i];
    cout << endl;
  }

  featureVector aggregate;

  bool azc = true;  // "allow_zero_count"
  for (size_t i=0; i<labels.size(); i++) {
    const featureVector v = ResultVector(i, azc, hist_zone_norm);
    
    if (LabelVerbose())
      cout << "Feature::AggregatePerFrameVector() : " << i << "/"
           << labels.size() << " size=" << v.size() << endl;

    aggregate.insert(aggregate.end(), v.begin(), v.end());
  }

  if (hist_global_norm == hist_norm_L1) {
    float sum = accumulate(aggregate.begin(), aggregate.end(), 0.0);
    float mul = sum ? 1.0/sum : 1.0;
    for (size_t d = 0; d<aggregate.size(); d++)
      aggregate[d] *= mul;
  }

  if (!aggregate.size())
    throw "empty AggregatePerFrameVector";

  return aggregate;
}

//=============================================================================

void Feature::PrintFeatureList(ostream &oStream, bool wide) {
  bool first = true;
  int col = 0;
  for (const Feature *p = list_of_methods; p; p=p->next_of_methods) {
    const list<string> nl = p->listNames();
    const list<string> ll = p->listLongNames();
    const list<string> tl = p->listShortTexts();

    list<string>::const_iterator n = nl.begin();
    list<string>::const_iterator l = ll.begin();
    list<string>::const_iterator t = tl.begin(); 

    for (; n!=nl.end(); n++,l++,t++) {
      if (wide) {
        oStream << *n << " = " << *l << " : " << *t << endl;
        continue;
      }
        
      if (!first)
        oStream << ", ";      
      first = false;
      
      if (col+n->length() > 60) {
        oStream << endl << "\t";
        col = 0;
      }
      oStream << *n;       
      col += n->length()+2;
    }
  }
  
  if (!wide && col>0)
    oStream << endl;
}

  //===========================================================================

  void Feature::PrintFeatureListXML(ostream *oStr, feature_result *res) {
    XmlDom doc  = XmlDom::Doc();
    XmlDom root = doc.Root("featurelist");

    for (const Feature *p = list_of_methods; p; p=p->next_of_methods) {
      const list<string> nl = p->listNames();
      const list<string> tl = p->listTargetTypes();
      const list<string> sl = p->listNeededSegmentations();

      list<string>::const_iterator n = nl.begin();
      list<string>::const_iterator t = tl.begin();
      list<string>::const_iterator s = sl.begin();

      for (; n!=nl.end() && t!=tl.end() && s!=sl.end(); n++, t++, s++) {
        string on = *n, z = p->DefaultZoningText();
        if (z!="1")
          on = "zo"+z+":"+on;

        XmlDom f = root.Element("feature");
        f.Prop("name", *n);
        if (on!=*n)
          f.Prop("outputname", on);

	// f.Prop("vectorlength", p->GetVectorLength());  // always==0...

        f.Prop("target", *t);
        f.Prop("hasrawin",  string(p->HasRawInMode() ?"yes":"no"));
        f.Prop("hasrawout", string(p->HasRawOutMode()?"yes":"no"));

        if (*s!="")
          f.Prop("segmentation", *s);
      }
    }

    if (oStr)
      doc.Write(*oStr, true);

    if (res)
      res->xml = doc;
    else
      doc.DeleteDoc();
  }

  //===========================================================================

string Feature::ResultSummaryStr(const feature_result& res) {
  stringstream ss;
  typedef list<labeled_float_vector> ll;
  const ll& data = res.data;
  const string pls = data.size()==1 ? "" : "s";

  ss << res.data.size() << " vector" << pls;
  if (data.size()) {
    ss << " of dimensionality " << res.dimension();
    bool found = false;
    for (ll::const_iterator i=data.begin(); !found && i!=data.end(); i++)
      found = i->second!="";
    if (found) {
      ss << " with label" << pls << " <" << data.begin()->second << ">";
      if (data.size()>1)
        ss << "..<" << data.rbegin()->second << ">";
    }
  }

  return ss.str();
}

//=============================================================================

void Feature::PrintHeader(ostream& os) { 
  if (description_print)
    PrintDescription(os);

  if (print_vector_length)
    feature_vector_length = PrintVectorLength(os);

  else if (check_vector_length && feature_vector_length==-1)
    feature_vector_length = FeatureVectorSize();

  print_vector_length = description_print = false;
}

//=============================================================================

void Feature::Print(ostream& os, const featureVector& vec, const string& lbl) { 
  if (IsRawOutMode())
    return;

  if (lbl=="")
    throw string("Feature::Print(): empty label");

  PrintHeader(os);

  int old_p = os.precision(Precision());
  bool sparse = IsSparseVector();

  if (sparse) {
    for (size_t i=0; i<vec.size(); i++) {
      if (!vec[i]) continue;
      os << i;
      if (vec[i] != 1)
        os << ":" << vec[i];
      os << " ";
    }
    os << ":";

  } else
    for (size_t i=0; i<vec.size(); i++)
      os << (i?" ":"") << vec[i]; 
    
  os << " " << lbl << endl;

  os.precision(old_p);

  AppendResult(vec, lbl);
}

//=============================================================================

  void Feature::SplitAndPrint(ostream& os, const featureVector& vin, int s) {
    if (WithinFrameTreatment()!=treat_separate ||
        BetweenFrameTreatment()!=treat_concatenate)
      throw string("Feature::SplitAndPrint() : "
                   "within!=separate || between!=concatenate");

    int ss = Nslices()>1 ? s : -1;
    int nf = Nframes(), ll = GetData(0)->Length();
 
    const vector<string>& labels = DataLabels();
    size_t nparts = WithinFrameTreatment()==treat_concatenate?1:labels.size();
    
    size_t len = size_t(feature_vector_length>=0 ? 
                        feature_vector_length : FeatureVectorSize());

    if (SliceVerbose())
      cout << "Feature::SplitAndPrint() : vin.size()=" << vin.size()
           << " s=" << s << " ss=" << ss << " nf=" << nf << " ll=" << ll
           << " nparts=" << nparts << " len=" << len << endl;

    for (size_t j=0; j<nparts; j++) {
      // featureVector vec(vin.begin()+j*len, vin.begin()+(j+1)*len);
      featureVector vec;
      for (size_t k=0; k<size_t(nf); k++) {
        size_t p = j+k*nparts;
        vec.insert(vec.end(), vin.begin()+p*ll, vin.begin()+(p+1)*ll);
      }
      Print(os, vec, Label(ss, -1, nparts>1 ? labels[j] : ""));
    }
  }

  //===========================================================================

  XmlDom Feature::FormDescription(bool weak) const { 
    string text = ShortText(), zt = ""; // segments->getZoningText();
    if (zt != "")
      text += " " + zt;
  
    time_t t = time(NULL);
    struct tm tmstruct;
    char buf[100];
#ifdef __MINGW32__
    char *timestr = asctime(localtime(&t));
#else
    char *timestr = asctime_r(localtime_r(&t, &tmstruct), buf);
#endif // __MINGW32__
    *strchr(timestr, '\n') = 0;

    string optionstxt;
    for (list<string>::const_iterator o=options.begin(); o!=options.end(); o++)
      optionstxt += string(optionstxt.size()?" ":"")+*o;
 
    XmlDom doc  = XmlDom::Doc();
    XmlDom root = doc.Root("feature");
    root.Element("name",         NameFaked());
    if (Name()==NameFaked()) {
      root.Element("longname",   LongName());
      root.Element("shorttext",  text);
    }
    root.Element("target",       TargetType());
    root.Element("vectorlength", FeatureVectorSize(weak));
    root.Element("options",      optionstxt);
    root.Element("hasrawin",     HasRawInMode() ?"yes":"no");
    root.Element("hasrawout",    HasRawOutMode()?"yes":"no");

    if (IsRawOutMode()) {
      XmlDom rawdata = root.Element("rawdata");
      rawdata.Prop("width",  Width());
      rawdata.Prop("height", Height());
    }

    if (hist_desc_sum!="")
      root.Element("histogram", hist_desc_sum);
    if (hist_file_spec!="")
      root.Element("histogramfile", hist_file_spec);

    XmlDom c = root.Element("components");
    FormComponentDescriptions(c);

    if (description_all) {
      char cwdbuf[2048];
      char  *cwd = getcwd(cwdbuf, sizeof cwdbuf);
      passwd *pw = getpwuid(getuid());
      const char *user = pw ? pw->pw_name : NULL;
      char hostname[1000], *hostnamep = hostname;
      gethostname(hostname, sizeof hostname);

      root.Element("version", Version());
      root.Element("date",    timestr);
      root.Element("user",    user);
      root.Element("host",    hostnamep);
      root.Element("cwd",     cwd);
      root.Element("cmdline", description_cmdline);
    }

    AddExtraDescription(root);

    // from RegionBased: (is it needed in all cases ?)
    if (!region_spec.empty()) {
      string all;
      for (size_t n=DataElementCount(), i=0; i<n; i++)
        all += string(i?";":"")+(string)*accessRegion(i).first;
      root.Element("region", all);
    }

    return doc;
  }

  //===========================================================================

  bool Feature::FormComponentDescriptions(XmlDom& p) const {
    bool debug = false;

    if (data.empty())
      return false;

    const vector<string>& ll = DataLabels();
    if (ll.empty())
      return false;

    const comp_descr_t descr = ComponentDescriptions();
    if (descr.empty())
      return true;

    for (int s=0, i=0; s<Nslices(); s++) {
      for (int f=0; f<Nframes(); f++) {
        for (size_t l=0; l<ll.size(); l++) {
          for (int j=0; j<GetData(0)->Length(); j++) {
            
            if (MethodVerbose()&&debug)
              cout << " * i=" << i << " s=" << s << " f=" << f << " l=" << l
                   << " j=" << j << endl;

            size_t jj = size_t(j);
            if (jj<descr.size()) {
              XmlDom c = p.Element("component");
              stringstream idx;
              idx << i;
              c.Prop("index", idx.str());
              
              for (comp_descr_e::const_iterator e = descr[jj].begin();
                   e!=descr[jj].end(); e++) {
                string val = e->second;
                if (e->first=="name" && ll.size()>1 &&
                    WithinFrameTreatment()==treat_concatenate) {
                  stringstream n;
                  n << "_" << l;
                  val += n.str();
                }
                c.Prop(e->first, val);
              }
            }

            // if (MethodVerbose())
            //   cout << endl;

            i++;
          }

          if (WithinFrameTreatment()!=treat_concatenate)
            break;
        }

        if (BetweenFrameTreatment()!=treat_concatenate)
          break;
      }

      if (BetweenSliceTreatment()!=treat_concatenate)
        break;
    }

    return true;
  }

  //===========================================================================

  bool Feature::PrintDescription(ostream& os, const string& sep,
                                 bool weak) const {
    XmlDom doc = FormDescription(weak);
    if (!doc.DocOK())
      return false;

    string xmlstr = doc.Stringify(true);
    char *ptr = (char*)xmlstr.c_str();
    while (ptr && *ptr) {
      char *nlptr = strchr(ptr, '\n');
      if (nlptr) {
        *nlptr = 0;
        os << sep << ptr << endl;
        ptr = nlptr+1;
      } else
        break;
    }

    doc.DeleteDoc();

    return true;
  }

  //===========================================================================

  bool Feature::PrintFeatureDescription(const string& fn, ostream& os) {
    list<string> opts;

    if (fn=="") {
      XmlDom doc  = XmlDom::Doc();
      XmlDom root = doc.Root("featurelist");
      
      for (const Feature *p = list_of_methods; p; p=p->next_of_methods) {
        const list<string> nl = p->listNames();
        for (list<string>::const_iterator m = nl.begin(); m!=nl.end(); m++) {
          Feature *f = CreateMethod(*m, "", "", opts);
          if (f) {
            XmlDom fd = f->FormDescription(true);
            if (fd) {
              XmlDom r = fd.Root();
              root.AddCopy(r);
            }
            delete f;
          }
        }
      }

      bool ok = doc.Write(os);
      doc.DeleteDoc();

      return ok;
    }

    Feature *f = CreateMethod(fn, "", "", opts);
    if (!f)
      return false;

    bool ok = f->PrintDescription(os, "", true);

    delete f;

    return ok;
  }

  //===========================================================================

  int Feature::FeatureVectorSize(bool weak) const {
    int fakelength = GetVectorLengthFake();
    if (fakelength>-1)
      return fakelength;

    if (data.empty()) {
      if (weak)
	return 0;
      else
	throw "Feature::FeatureVectorSize() data structure not initialized yet";
    }

    int nslices = Nslices(), nframes = Nframes();
    size_t ll = GetData(0)->VectorOrHistogramLength(), lsum = ll;

    for (size_t i=1, n=DataCount(); i<n; i++) {
      size_t lc = GetData(i)->VectorOrHistogramLength();

      switch (WithinFrameTreatment()) {
      case treat_concatenate:
	lsum += lc;
	break;

      case treat_separate:
	if (lc!=ll) {
	  char tmp[1000];
	  sprintf(tmp,"Feature::FeatureVectorSize()"
		  " unequal-sized data elements %d != %d", (int)lc, (int)ll);
	  throw string(tmp);
	}
	break;

      default:
	throw string("Feature::FeatureVectorSize() within treatment <")+
	  WithinFrameTreatmentName() + "> not implemented";
      }
    }

    if (WithinFrameTreatment()==treat_separate && !pair_by_predicate.empty())
      lsum *= 2;

    if (BetweenFrameTreatment()==treat_concatenate)
      lsum *= nframes;

    if (BetweenSliceTreatment()==treat_concatenate)
      lsum *= nslices;

    if (BetweenSliceTreatment()==treat_diffconcat)
      lsum *= (nslices-1);

    if (FileVerbose() || lsum==0)
      cout << "Feature::FeatureVectorSize() :"
	   << " within_frame="  << WithinFrameTreatmentName()
	   << " between_frame=" << BetweenFrameTreatmentName() 
	   << " between_slice=" << BetweenSliceTreatmentName()
	   << " slices=" << nslices
	   << " frames=" << nframes << " items=" << DataCount()
	   << " length=" << ll << " results to size=" << lsum << endl;

    if (!lsum)
      throw "Feature::FeatureVectorSize() returning zero";

    return lsum;
  }

  //===========================================================================

  bool Feature::CalculateAndPrintAllPreProcessings() {
    const string errhead = "Feature::CalculateAndPrintAllPreProcessings() : ";

    if (MethodVerbose())
      cout << errhead << "starting with " << frame_preprocess_method.size()
	   << " file preprocessing methods" << endl;

    if (frame_preprocess_method.size()==0)
      return CalculateAndPrint();

    int base = SegmentData()->getCurrentFrame();
    imagedata orig = *SegmentData()->accessFrame(base);

    bool ok = true;
    for (auto i=frame_preprocess_method.begin();
	 i!=frame_preprocess_method.end(); i++) {
      if (MethodVerbose())
	cout << errhead << "starting with file preprocessing '" << i->first
	     << "'" << endl;
      frame_preprocess_in_use = i->first;
      ok = CalculateAndPrint();
      RemoveDataElements();
      RemoveDataLabelsAndIndices();
      frame_preprocess_in_use = "";
      int basenow = SegmentData()->getCurrentFrame();
      if (base==basenow) // preprocess=resize() with videos fails otherwise
	*SegmentData()->accessFrame(base) = orig;
    }

    return ok;
  }

//=============================================================================

  bool Feature::CalculateAndPrint() {
#ifndef __MINGW32__
  cox::tictac::func tt(tics, "Feature::CalculateAndPrint");
#endif

  const string errhead = "Feature::CalculateAndPrint() : ";

  treat_type bft = BetweenFrameTreatment(), bst = BetweenSliceTreatment();

  if (bst==treat_undef)
    throw errhead+"between slice treatment not defined";

  if (bst!=treat_concatenate  && bst!=treat_separate &&
      bst!=treat_diffseparate && bst!=treat_diffconcat)
    throw errhead+"between slice treatment \""+BetweenSliceTreatmentName()+
      "\" not implemented yet";

  if (bst==treat_concatenate && bft!=treat_concatenate && bft!=treat_average)
    throw errhead+"combination betweenslice==\"concatenate\" &&"
      " betweenframe==\""+BetweenFrameTreatmentName()+
      "\" not implemented yet";

  if (FileVerbose())
    cout << errhead+"betweenslice="+BetweenSliceTreatmentName()
         << " betweenframe="+BetweenFrameTreatmentName()
         << " withinframe="+WithinFrameTreatmentName()
         << endl;

  SetDataLabelsAndIndices(false);
  // AddDataElements(false); 2018-06-21 moved after FramePreProcess()
  
  featureVector concat;
  
  int nslices = Nslices();

  bool print_slice = bst==treat_separate, enabled = PrintingEnabled();
  
  if (FileVerbose()) {
    stringstream tmp;
    tmp << "print_slice=" << print_slice << " enabled=" << enabled << endl;
    cout << errhead+tmp.str();
  }

  if (IsRawOutMode() && fout_xx)
    PrintHeader(*fout_xx);

  featureVector prev_slice;

  for (int i=0; i<nslices; i++) {
    featureVector slicevec = CalculatePerSlice(i, print_slice&&enabled);

    if (!print_slice && slicevec.empty() && enabled)
      throw errhead+"empty slicevec";

    if (bst==treat_concatenate)
      concat.insert(concat.end(), slicevec.begin(), slicevec.end());

    if ((bst==treat_diffconcat || bst==treat_diffseparate) && i>0) {
      if (SliceVerbose())
        cout << errhead+"calculating between slice difference" << endl;

      featureVector diffvec = slicevec;
      AddToFeatureVector(diffvec, prev_slice, true, -1.0);
      if (bst==treat_diffconcat)
        concat.insert(concat.end(), diffvec.begin(), diffvec.end());
      else if (enabled)
        Print(diffvec, Label(i, -1, ""));
    }

    prev_slice = slicevec;
  }

  if (enabled && (bst==treat_concatenate || bst==treat_diffconcat))
    Print(concat, Label(-1, -1, ""));

  return true;
}

  //===========================================================================

  void Feature::RemoveDataElements() {
    if (FrameVerbose() && data.size())
      cout << "Feature::RemoveDataElements() n=" << data.size() << endl;

    for (vector<Data*>::iterator i=data.begin(); i!=data.end(); i++)
      delete *i;

    data.clear();
  }

  //===========================================================================

  void Feature::AddDataElements(bool erase) {
    if (erase)
      RemoveDataElements();
    
    else if (!data.empty())
      throw "AddDataElements() : data not empty";

    for (size_t n=DataElementCount(), i=0; i<n; i++) {
      data.push_back(CreateData(PixelType(), ConcatCountPerPixel(), i));
      if (LabelVerbose())
        cout << "Feature::AddDataElements() " << i << "/" << n << " \""
             << (i<data_labels.size() ? data_labels[i] : "*FAKE*")
             << "\" added " << data.back()->Name() << " data elements of size "
             << data.back()->Length() << endl;
    }
  }

//=============================================================================

Feature::featureVector Feature::CalculatePerSlice(int s, bool print) {
#ifndef __MINGW32__
  cox::tictac::func tt(tics, "Feature::CalculatePerSlice");
#endif

  stringstream msg;
  msg << "Feature::CalculatePerSlice(" << s << "," << print << ") : ";

  if (BetweenFrameTreatment()==treat_undef)
    throw msg.str()+"between frame treatment not defined";

  if (BetweenFrameTreatment()!=treat_concatenate &&
      BetweenFrameTreatment()!=treat_separate    &&
      BetweenFrameTreatment()!=treat_pixelconcat &&
      BetweenFrameTreatment()!=treat_average)
    throw msg.str()+"between frame treatment \""+BetweenFrameTreatmentName()+
      "\" not implemented yet";

  if (false && BetweenFrameTreatment()==treat_concatenate &&
      WithinFrameTreatment()!=treat_concatenate)
    throw msg.str()+"combination of betweenframe==concatenate && "
      +"withinframe!=concatenate treatment not implemented yet";

  if (framestep<1)
    throw msg.str()+"framestep should be larger than zero";

  if (SliceVerbose())
    cout << msg.str() << "betweenframe=" << BetweenFrameTreatmentName()
         << " framestep=" << framestep <<endl;

  pair<int,int> frames = FrameRange(s);
  int fbegin = frames.first, fend = frames.second;

  if (framestep!=1) {
    while (fbegin%framestep)
      fbegin++;
    while (fend%framestep)
      fend--;

    // if we don't have a frame that is dividible with framestep, use the 
    // middlemost frame:
    if (fbegin>fend)
      fbegin = fend = (frames.first+frames.second)/2;
  }


  if (fbegin>fend)
    throw msg.str()+"empty frame range";

  //  int vs = PerFrameFeatureVectorSize();
  //if (!vs)
  //  throw msg.str()+"per frame feature vector size is zero";

  if (SliceVerbose())
    cout << msg.str() << "calculating for range " << fbegin << ".." << fend
	 << " with framestep=" << framestep << endl;

  treat_type bft = BetweenFrameTreatment();
  bool print_here = (print && (bft==treat_concatenate || bft==treat_average))
    || IsVideoFeature() /*|| IsAudioFeature()*/;

  featureVector collect; //(vs);

  if (IsPerFrameCombinable())
    CombinePerFrameToOneSlice(collect, fbegin, fend, print && !print_here);
  else {
    if (fbegin == fend) { fbegin-=2; fend+=2; }
    CalculateOneSlice(collect, fbegin, fend, print && !print_here);
  }

  if (print_here) {
    if (IsVideoFeature() || IsAudioFeature()) {
      stringstream segm;
      segm << fbegin << "-" << fend;
      if (!collect.empty())
        Print(collect,Label(-1, Nslices()>1 ? fbegin : -1, 
                            Nslices()>1 ? fend : -1, ""));
    }
    else if (WithinFrameTreatment()==treat_separate)
      SplitAndPrint(collect, s);
    else
      Print(collect, Label(Nslices()>1 ? s : -1, -1, ""));
  }

  if (print_here)
    collect.clear();

  if (SliceVerbose())
    cout << msg.str() << "returning vector of length " << collect.size()<<endl;

  return collect;
}

//=============================================================================

bool Feature::CombinePerFrameToOneSlice(featureVector& collect,
                                        int fbegin, int fend, bool print) {

  stringstream msg;
  msg << "Feature::CombinePerFrameToOneSlice(" << fbegin << ","
      << fend << "," << print << ") : ";

  size_t vs = 0, nframes = 0; //  was once vs = collect.size();

  //  if (SliceVerbose())
  //  cout << msg.str() << "starting with vector length=" << vs << endl;

  for (int i=fbegin; i<=fend; i+=framestep) {
    int ff = fbegin==fend ? -1 : i;
    
    featureVector framevec = CalculatePerFrame(i, ff, print);
    nframes++;

    if (i==fbegin) {
      vs = framevec.size();
      if (SliceVerbose())
        cout << msg.str() << "1st frame resulted in vector length=" << vs
             << endl;
      collect = featureVector(vs);
    }

    if (!print && framevec.size()!=vs)
      throw msg.str()+"framevec has wrong size";

    if (BetweenFrameTreatment()==treat_concatenate) {
      if (i==fbegin)
        collect.clear();
      collect.insert(collect.end(), framevec.begin(), framevec.end());
    }

    else if (BetweenFrameTreatment()==treat_average)
      AddToFeatureVector(collect, framevec, true);

    if (data.size())
      ZeroPerFrameData();

    if (BetweenFrameTreatment()==treat_pixelconcat)
      break;
  }

  if (SliceVerbose())
    cout << msg.str() << "finished with vector length=" << collect.size()
         << " and nframes=" << nframes << endl;

  if (BetweenFrameTreatment()==treat_average)
    DivideFeatureVector(collect, nframes);

  return true;
}

  //===========================================================================

  bool Feature::CalculateOneSlice(featureVector& collect, int fbegin, int fend,
                                  bool print) { 
    stringstream msg; 
    msg << "CalculateOneSlice(" << fbegin << ", " << fend << ") : ";

    bool video = IsVideoFeature();
    bool audio = IsAudioFeature();
    
    if (!video && !audio)
      return true;

    if (audio && Nslices() != 1)
      throw msg.str()+"not yet implemented for audio slices";
    
    featureVector framevec;
    if (video) {
      const string filename = ExtractVideoSegment(fbegin, fend,
						  VideoSegmentExtension());
      if (!filename.empty()) {
        videosegment_file = filename;
        try {
          framevec = CalculateVideoSegment(print);
        } catch (const string& str) {
          ShowError(str);
        }
        RemoveVideoSegment();
      } else 
        throw msg.str()+"could not extract video slice";
    }
    if (audio)
      framevec = CalculateAudioSegment(print);
      
    collect = featureVector(framevec.size());
    collect.clear();
    collect.insert(collect.end(), framevec.begin(), framevec.end());
    
    return true;
  }

  //===========================================================================

  const string Feature::ExtractVideoSegment(int fbegin, int fend,
					    const string& ext) {
    if (!videofile_ptr)
      return "";

    // No error if duration was below 2 second limit, then mencoder
    // often fails...
    // float min_dur = 2.0; 
    float min_dur = 0.0;

    double fps = videofile_ptr->get_frame_rate();
    double tp = (double)fbegin/fps;
    double dur = (double)(fend-fbegin+1)/fps;

    videofile::debug(FileVerbose()?1:0);
    string filename = ExtractVideoSegmentInner(tp, dur, ext);

    bool has_video = videofile(filename).has_video();

    // No error if duration was below 2 second limit, then mencoder
    // often fails...
    if (!has_video && dur>=min_dur)
      throw string("Feature::ExtractVideoSegment(): extraction failed!");

    return has_video ? filename : "";
  }

  //===========================================================================

  string Feature::ExtractVideoSegmentInner(double tp, double dur,
					   const string& ext) {
    // string ext = ".mpg"; // this has effect on the result...
    //                      at least avconv doesn't work...

    // until 2014-01-23:
    // return videofile_ptr->extract_video_segment_to_tmp(ext, tp, dur,
    // 						          vcodec_copy,
    // 						          acodec_null,
    //                                                    VF_MENCODER);

    // since 2014-01-23:

    if (Name()=="MPEG7-MotionActivity")
      return videofile_ptr->
	extract_video_segment_to_tmp(ext, tp, dur, vcodec_mpeg1,
				     acodec_null, VF_MENCODER,
				     "-vf scale=640:267 -fps 25");

    return videofile_ptr->extract_video_segment_to_tmp(ext, tp, dur,
						       vcodec_copy,
						       acodec_null,
						       VF_FFMPEG);
    // MotionActivity hack for database=affective
    // videofile_ptr->extract_video_segment_to_tmp(ext, tp, dur, vcodec_mpeg1,
    // 					    acodec_null, VF_MENCODER,
    //                                             "-vf scale=640:267");
  }

//=============================================================================

Feature::featureVector Feature::CalculatePerFrame(int f, int ff, bool print) {
#ifndef __MINGW32__
  cox::tictac::func tt(tics, "Feature::CalculatePerFrame");
#endif

  stringstream msg;
  msg << "Feature::CalculatePerFrame(" << f << "," << ff << ","
      << print << ") : ";

  bool data_allocation_needed=false;

  if (WithinFrameTreatment()==treat_undef)
    throw msg.str()+"within frame treatment not defined";

  if (FrameVerbose())
    cout << msg.str() << "withinframe=" << WithinFrameTreatmentName() << endl;
  
  if (DebugMemoryUsage())
    DumpMemoryUsage(cerr, true);

//   if (!image->ReadImageFrame(f))
//     throw msg.str()+"ReadImageFrame() failed";

  if (!incore_imagedata_ptr)
    SegmentData()->setCurrentFrame(f);

  if (region_hierarchy) { // parse region hierarchy from segmentation xml
    data_allocation_needed=true;
   
    // map<int,set<int> > composites;
    composite_regions.clear();
    vector<set<int> > rl=SegmentData()->flattenHierarchyToPrimitiveRegions(true);

    int nleaves=SegmentData()->GetLabelCount();

    vector<set<int> >::const_iterator it; 
    for(it=rl.begin();it!=rl.end();it++){
      if(it->size()<2) continue;
      CompositeRegion tmpRegion;
      tmpRegion.id=nleaves+composite_regions.size();
      tmpRegion.primitiveRegions=*it;
      composite_regions.push_back(tmpRegion);
    }

  } else if (virtual_regions) {
    data_allocation_needed=true;
    composite_regions.clear();

    SegmentData()->setCurrentFrame(f);
    vector<pair<set<int>,string> > rl= SegmentData()->parseVirtualRegions();
    int first_free=SegmentData()->GetLabelCount();

    vector<pair<set<int>,string> >::const_iterator rit;
    for(rit=rl.begin();rit!=rl.end();rit++){
        CompositeRegion tmpRegion;
        tmpRegion.id=first_free++;
        tmpRegion.primitiveRegions=rit->first;
        composite_regions.push_back(tmpRegion);
    }
  }

  if (interest_points) {
    RemoveRegionSpec();

    ipList iplist;
    SegmentData()->parseIPListFromLastFrameResult(iplist, "");
    
    if (LabelVerbose()) 
      cout << "parsed " << iplist.list.size() << " interest points from XML"
	   << endl;
    
    if (iplist.fieldname2idx.count("x")>0 &&
	iplist.fieldname2idx.count("y")>0 &&
	iplist.fieldname2idx.count("radius")>0) {

      size_t x_idx=iplist.fieldname2idx["x"];
      size_t y_idx=iplist.fieldname2idx["y"];
      size_t r_idx=iplist.fieldname2idx["radius"];

      for(size_t i=0; i<iplist.list.size(); i++)
	region_spec
	  .push_back(make_pair(new circularRegion(iplist.list[i][x_idx],
						  iplist.list[i][y_idx],
						  iplist.list[i][r_idx]),
			       ""));
      data_allocation_needed = true;

    } else
      throw string("currently only x y radius interest points supported");

  } else if (region_descr.empty()) {
    // regions specified in segmentation file only used if
    // no regions are given on command line
    
    RemoveRegionSpec();
    
    SegmentationResultList *l=
      SegmentData()->readFileResultsFromXML("", "region");
    SegmentationResultList::const_iterator it;
    for (it=l->begin(); it!=l->end(); it++) {
      region_spec.push_back(make_pair(Region::createRegion(it->value), ""));
      if (LabelVerbose()) 
        cout << "region " << it->value << " found as file result" << endl; 
      data_allocation_needed = true;
    }

    delete l;

    l = SegmentData()->readFrameResultsFromXML("", "region");
    for (it=l->begin(); it!=l->end(); it++) {
      region_spec.push_back(make_pair(Region::createRegion(it->value), ""));
      if (LabelVerbose()) 
        cout << "region " << it->value << " found as frame result" << endl; 
      data_allocation_needed = true;
    }

    delete l;
  }
  
  PossiblySetZoning();  // added 13.9.2004

  if (f && WithinFrameTreatment()==treat_concatenate && !data_labels.empty()) {
    vector<string> labels = SolveUsedDataLabels();
    sort(labels.begin(), labels.end()); // later on, something smarter...
    if (labels!=data_labels)
      throw msg.str()+"labels of existing image segments differ";
  }

  if (data_allocation_needed ||(f && WithinFrameTreatment()==treat_separate)) {
    SetDataLabelsAndIndices(true);
    AddDataElements(true);
  }

  bool ok = true;

  PerformRotating(f);
  PerformScaling(f);
  
  int base = SegmentData()->getCurrentFrame();
  imagedata& img = incore_imagedata_ptr ? *incore_imagedata_ptr
      : *SegmentData()->accessFrame(base);
  if (!FramePreProcess(img, f))
    ok = false;

  if (ok) {
    // added 2018-06-21 for t_c_ features...
    if (RegionDescriptorCount()==1) {
      // assume "all image" added in RegionBased::ProcessOptionsAndRemove()
      SetRegionSpecificationWholeImage();
    }
    PossiblySetZoning();  // added 2014-01-22 so it's set multiple times...
    AddDataElements(false); // moved here 2018-06-21
  }

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
  opencvmat = imagedata_to_cvMat();
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

  if (ok) {
    if (FrameVerbose())
      cout << msg.str() << "calling CalculateOneFrame() with "
	   << img.info() << endl;

    if (!CalculateOneFrame(f))
      ok = false;
  }

  if (!ok) {
    cerr << "Feature::CalculatePerFrame: Warning: calculation failed, "
      "using zero vector." << endl;
    switch (WithinFrameTreatment()) {
    case treat_separate: {
      const vector<string>& l = DataLabels();
      int j=0;
      for (vector<string>::const_iterator i=l.begin(); i!=l.end(); i++)
        GetData(j++)->ZeroToSize(FeatureVectorSize());
      break;
    }
    case treat_concatenate:
      GetData(f)->ZeroToSize(FeatureVectorSize());
      break;
    case treat_pixelconcat:
    case treat_average:
    case treat_join:
    default:
      throw msg.str()+"within frame treatment \""+
        WithinFrameTreatmentName()+ "\" not implemented yet";
    }
    ok = true;
  }

  if (print && ok)
    DoPrinting(-1, ff);

  featureVector concat;

  if (!print)
    concat = AggregatePerFrameVector();

  if (FrameVerbose())
    cout << msg.str() << "returning vector of length " << concat.size() <<endl;

  return concat;
}

  //===========================================================================

  bool Feature::DoPrinting(int slice, int frame) {
    static const string errhead = "Feature::DoPrinting() : ";

    switch (WithinFrameTreatment()) {
    case treat_separate:
      DoPrintingSeparate(slice, frame);
      break;

    case treat_concatenate:
      Print(AggregatePerFrameVector(), Label(slice, frame, ""));
      break;

    case treat_pixelconcat:
    case treat_average:
    case treat_join:
    default:
      throw errhead+"within frame treatment \""+
	WithinFrameTreatmentName()+ "\" not implemented yet";
    }

    return true;
  }

  //===========================================================================

  bool Feature::DoPrintingSeparate(int slice, int frame) {
    static const string errhead = "Feature::DoPrintingSeparate() : ";
    bool azc = true, norm = true;
    string lbl;

    if (pair_by_predicate.empty()) {
      if (interest_points) {
	for (size_t j=0; j<region_spec.size(); j++) {
	    //	cout << "printing ip #" << j << endl;
	  coord c = region_spec[j].first->middlePoint();
	  Print(ResultVector(j, azc, norm), RawLabel(c.x, c.y));
	} 
      } else {
	const vector<string>& l = DataLabels();
	int j=0;
	for (auto i=l.begin(); i!=l.end(); i++)
	  Print(ResultVector(j++, azc, norm), Label(slice, frame, *i));
      }
    } else {
      cout << "pairing by predicate " << pair_by_predicate << endl;
      cout << "slice="<<slice<<" frame="<<frame<<endl; 
      PairByPredicate(slice,frame);
    }

    return true;
  }

//=============================================================================

  void Feature::PairByPredicate(int slice,int frame){

     const vector<string>& l = DataLabels();
     bool azc = true, norm = true;
     
     //int frame_was=SegmentData()->getCurrentFrame();
     //SegmentData()->setCurrentFrame(frame);
     
     pairingPredicate pp=SegmentData()->determinePairingPredicate(pair_by_predicate);
     
     int j1=0;
     for (vector<string>::const_iterator i1=l.begin(); i1!=l.end(); i1++){
       featureVector v1=ResultVector(j1++, azc, norm);
       int j2=0;
       for (vector<string>::const_iterator i2=l.begin(); i2!=l.end(); i2++){
         featureVector cv=concatFeatureVectors(v1,ResultVector(j2++,azc,norm));
         if (pp.isPair(*i1,*i2)) {
           string clabel=*i1+"p"+*i2;
           Print(cv, Label(slice, frame, clabel));
         }
       }
     }

     //SegmentData()->setCurrentFrame(frame_was);

  }

  //===========================================================================

  string Feature::ObjectDataFileName() const {
    return SegmentData()->getImageFileName();
  }

  //===========================================================================

  string Feature::FullFileName(const string& p) {
    char curpath[1000];

    if (p=="" || p[0]=='/' || !getcwd(curpath, sizeof curpath))
      return p;

    curpath[sizeof(curpath)-1] = 0;
    
    return string(curpath)+"/"+p;
  }

//=============================================================================

string Feature::BaseFileName() const {
//   if (!image)
//     throw "Feature::BaseFileName() : no image";
  
  string base_all = ObjectDataFileName(), ret;
  vector<string> part = SplitFilename(base_all);

  for (size_t j=0; j<part.size(); j++) {
    string base = StripFilename(part[j]);
    ret += (ret==""?"":",")+base;
  }
  
  return ret;
}

//=============================================================================

  string Feature::RawLabel(int x, int y) const {
    stringstream msgss;
    msgss << "Feature::RawLabel() : ";
    string msg = msgss.str();

    string l = Label(-1, -1, -1, "");

    if (l=="") {
      if (LabelVerbose())
        cout << msg+"returning empty" << endl;
      return "";
    }

    string xy;
    if (x>=0 && y>=0) {
      stringstream ss;
      ss << "(" << x << "," << y << ")";
      xy = ss.str();
    }

    string segmspec;

    bool includesegmid=true;

    if(includesegmid){

      segmspec="_";

      int f=SegmentData()->getCurrentFrame();

      vector<int> sv = SegmentVector(f, x, y);
      for (size_t svi=0; svi<sv.size(); svi++){
	char str[80];
	sprintf(str,"%d",sv[svi]);
	if(svi)
	  segmspec += "+";
	segmspec += str;
      }
      

    }

    return l+xy+segmspec;

  }

  //===========================================================================

  string Feature::Label(int slice, long frame1, long frame2,
                        const string& segm) const {
    stringstream msg;
    msg << "Feature::Label() slice=" << slice << " frame1=" << frame1
        << " frame2=" << frame2 << " segm=[" << segm << "] : ";

    if (GetLabelPattern()=="") {
      if (LabelVerbose())
        cout << msg.str() << "returning empty" << endl;
      return "";
    }

    stringstream ss;
    ss << (forced_label!="" ? forced_label : BaseFileName());
    if (LabelVerbose())
      cout << msg.str() << "A base=[" << ss.str() << "]" << endl;

    if (slice>=0)
      ss << "_" << slice;
    if (LabelVerbose())
      cout << msg.str() << "B base=[" << ss.str() << "]" << endl;

    if (frame1>=0) {
      bool frameset = false;
      if (sec_label_interval) {
	string frspec = SolveFrameSpec(frame1);
	if (frspec!="") {
	  ss << frspec;
	  frameset = true;
	}
      }
      if (!frameset) {
	ss << ":" << frame1;
	if (frame2>=0)
	  ss << "-" << frame2;
      }
    }
    if (LabelVerbose())
      cout << msg.str() << "C base=[" << ss.str() << "]" << endl;

    if (IsImageFeature() && segm!="")
      ss << "_"+segm;
    if (LabelVerbose())
      cout << msg.str() << "D base=[" << ss.str() << "]" << endl;

    string base    = ss.str();
    string method  = GetSegmentationMethodName();
    string pattern = GetLabelPattern();

    if (LabelVerbose())
      cout << msg.str() << "E method=[" << method << "] pattern=[" << pattern
           << "] std.patt=[" << StandardLabelPattern() << "]" << endl;

    if (segm=="" && pattern==StandardLabelPattern()) // pattern=="%m:..."
      pattern.erase(0, 3);

    if (LabelVerbose())
      cout << msg.str() << "F pattern=[" << pattern << "]" << endl;

    size_t p = pattern.find("%*p");
    if (p!=string::npos) {
      pattern.erase(p, 3);
      if (frame_preprocess_in_use!="")
	pattern.insert(p, "*"+frame_preprocess_in_use);
    }

    if (LabelVerbose())
      cout << msg.str() << "G pattern=[" << pattern << "]" << endl;

    if (method.substr(0, 4)=="sl1+" && slice<0)
      method.erase(0, 4);

    if (LabelVerbose())
      cout << msg.str() << "H method=[" << method << "]" << endl;

    if (IsImageFeature())
      base = segmentfile::FormOutputFileName(pattern, base, method);
 
    if (IsVideoFeature() || IsAudioFeature())
      base = FilenameFromPattern(pattern, segm, "", base);

    if (LabelVerbose())
      cout << msg.str() << "returning [" << base << "]" << endl;

    return base;
  }

  //===========================================================================

  string Feature::SolveFrameSpec(int f) const {
    // OLD COMMENT FOLLOWS... since 2014-01-23 IsAudioFeature() _should_ work
    // obs! IsAudioFeature() doesn't yet work with AvgAudioEnergy
    // maybe it doesn work with others neither...
    bool audiofeature_faking = false;

    if (audiofeature_faking || IsAudioFeature()) {
      // obs! here we assume that AudioBased::sliceDuration = 1000 
      int sdur = 1000;
      if (sdur==1000)
	return ":s"+ToStr(f);
      return "";
    }

    // until 2014-01-23 this was not effective, now it may be broken...
    // float fpsf = SegmentData()->videoFPS();
    int fps = 25; //obs! hard-coded frame rate
    // cout << fpsf << " " << fps << endl;

    if (f%fps==0)
      return ":s"+ToStr(f/fps);

    return "";
  }

  //===========================================================================

  string Feature::StandardFilenamePattern(int t, bool r) {
    const string g = "features/%m:%f%o%h";       // 0
    const string b = "%b/features/%m:%f%o%h";    // 1
    const string s = "features/%i-%m:%f%o%h";    // 2
    const string S = "%b/features/%i-%m:%f%o%h"; // 3
    const string R = "%r/features/%i-%m:%f%o%h"; // 4
    const string rr = "%r/features/%m:%f%o%h";   // 6
    const string x = "/dev/null";                // 5

    if (t==5)
      return x;

    const string e = r ? ".raw" : ".dat";

    string a = (t==1 ? b : t==2 ? s : t==3 ? S : t==4 ? R : t==6 ? rr : g)+e;

    if (r) {
      size_t p = a.find("%m:");
      if (p!=string::npos)
        a.erase(p, 3);

      p = a.find("%h");
      if (p!=string::npos)
        a.erase(p, 2);
    }

    if (MethodVerbose())
      cout << "StandardFilenamePattern(" << t << "," << r << ") returns <"
           << a << ">" << endl;

    return a;
  }

  //===========================================================================

  void Feature::SetRawInputFilename(const string& f, const string& i) {
    string inpat  = RawInputPattern();
    string outpat = FilenameFromPattern(inpat, "", f, i);
    SetRawInputFilename(outpat);
  }

  //===========================================================================

  void Feature::SetOutputFilename(const string& mm, const string& f,
                                  const string& i) {
    ofn_method  = mm;
    ofn_feature = f;
    ofn_files   = i;

    string inpat  = OutputPattern();
    string outpat = FilenameFromPattern(inpat, mm, f, i);
    SetOutputFilename(outpat);
  }

  //===========================================================================

  string Feature::FilenameFromPattern(const string& pat, const string& mm,
                                      const string& f, const string& i) const {
    string mx = mm;

    size_t zp = mx.find("sl1+");
    if (zp!=string::npos && BetweenSliceTreatment()!=treat_concatenate)
      mx.erase(zp, 4);

    zp = mx.find("zo1");
    if (zp!=string::npos) {
      mx.erase(zp, 3);
      if (zp>0 && mx[zp-1]=='+')
        mx.erase(zp-1, 1);
      else if (mx[zp]=='+')
        mx.erase(zp, 1);
    }

    string pattern = pat;

    size_t p = pattern.rfind("%b");
    if (p!=string::npos) {
      string tmp = outdir;
      if (tmp=="") {
	tmp = i;
	size_t q = tmp.rfind('/');
	if (q!=string::npos)
	  tmp.erase(q);
	else
	  tmp = ".";
      }
      pattern.replace(p, 2, tmp);
    }

    p = pattern.rfind("%r");
    if (p!=string::npos) {
      string tmp = outdir;
      if (tmp=="") {
	tmp = i;
	size_t q = tmp.rfind('/');
	if (q!=string::npos) {
	  tmp.erase(q);
	  if (tmp.substr(tmp.length()-2) == ".d") {
	    q = tmp.rfind('/');
	    if (q!=string::npos)
	      tmp.erase(q);
	    else
	      tmp = ".";
	  }
	} else
	  tmp = ".";
      }
      if (subdir!="") {
	size_t q = tmp.rfind("/"+subdir);
	if (q!=string::npos && q==tmp.size()-subdir.size()-1)
	  tmp.erase(q);
      }

      pattern.replace(p, 2, tmp);
    }

    p = pattern.rfind("%m");
    if (p!=string::npos) {
      if (mx!="")
        pattern.replace(p, 2, mx);
      else {
        pattern.replace(p, 2, "");
        if (pattern[p]==':')
          pattern.replace(p, 1, "");
      }
    }
  
    // this `f' argument might be a bit bogus when we have `this' available...
    p = pattern.rfind("%f");
    if (p!=string::npos)
      pattern.replace(p, 2, f);
  
    p = pattern.rfind("%o");
    if (p!=string::npos)
      pattern.replace(p, 2, OptionSuffix());
  
    p = pattern.rfind("%h");
    if (p!=string::npos) {
      string hdpart = hist_desc_sum=="" ? "" : "="+hist_desc_sum;
      pattern.replace(p, 2, hdpart);
    }

    p = pattern.rfind("%i");
    if (p!=string::npos) {
      string tmp = i;
      size_t q = tmp.rfind('/');
      if (q!=string::npos)
        tmp.erase(0, q+1);
      q = tmp.rfind('.');
      if (q!=string::npos)
        tmp.erase(q);

      pattern.replace(p, 2, tmp);
    }  

    if ((FileVerbose()&&!IsRawOutMode()) || KeyPointVerbose())
      cout << "Feature::FilenameFromPattern() formed \"" << pattern << "\""
           << " ([" << mm << "]=>[" << mx << "])" << endl;

    return pattern;
  }

  //===========================================================================

  bool Feature::OpenOutputFile(pair<string,ofstream>& ostr, bool& new_file) {
    fout_xx = &cout;

    if (OutputPattern()=="") {
      if (MethodVerbose())
        cout << "Output directed to cout due empty OutputPattern()" << endl;
      return true;
    }

    const string ofile = OutputFilename();
    if (ofile.empty()) {
      cout << "Error: output file name not generated!" << endl;
      
    } else if (ofile=="/dev/null") {
      fout_xx = NULL;
      if (MethodVerbose())
        cout << "Output directed /dev/null" << endl;
      
    } else {
      if (ostr.second.is_open() && ofile!=ostr.first) {
        if (MethodVerbose()) 
          cout << "Closing output file <" << ostr.first << ">" << endl;

        ostr.first = "";
        ostr.second.close();
      }

      if (!ostr.second.is_open()) {
        EnsureDirectoryExists(ofile);
        ostr.second.open(ofile.c_str(), ofstream::out|ofstream::trunc);
        if (!ostr.second) {
          cerr << "Error: output file <" << ofile << "> could not be opened"
               << endl;
          return false;
        }
        if (MethodVerbose()) 
          cout << "Opened <" << ofile << "> for output" << endl;

        ostr.first = ofile;
        new_file = true;

      } else {
        if (ofile!=ostr.first) {
          cout << "Error: output file name mismatch, ofile=<" << ofile
               << "> current=<" << ostr.first << ">"
               << endl;
        }

        if (MethodVerbose()) 
          cout << "Continuing to output to file <" << ostr.first << ">"
               << endl;
      }

      fout_xx = &ostr.second;
      
      if (MethodVerbose()) 
        cout << "Redirecting output to <" << ofile << ">" << endl;
    }
    
    return true;
  }

  //===========================================================================

  vector<string> Feature::SplitFilename(const string& s) const {
    if ((FileVerbose()&&!IsRawOutMode()) || KeyPointVerbose())
      cout << "Feature::SplitFilename(" << s << ") :";

    vector<string> ret;
    for (size_t b = 0, comma = 0; comma!=s.size(); b = comma+1) {
      comma = s.find(',', b);
      if (comma==string::npos)
        comma = s.size();

      ret.push_back(s.substr(b, comma-b));

      if (FileVerbose())
        cout << " [" << ret.back() << "]";
    }

    if (FileVerbose())
      cout << endl;

    return ret;
  }

  //===========================================================================

  string Feature::StripFilename(const string& s) {
    if (FileVerbose())
      cout << "Feature::StripFilename(" << s << ") -> <";

    string base = s;
    size_t i = base.rfind('/');
    if (i!=string::npos)
      base.erase(0, i+1);
  
    i = base.rfind('.');
    if (i!=string::npos)
      base.erase(i);

    if (FileVerbose())
      cout << base << ">" << endl;

    return base;
  }

  //===========================================================================

  vector<float> Feature::GetPixelFloats(int x, int y, pixeltype t) {
    int l = ConcatCountPerPixel(), o = 0;

    vector<float> v(l*PixelTypeSize(t));

    int base = SegmentData()->getCurrentFrame();
  
    float m = PixelMultiply();

    for (int i=0; i<l; i++) {
      vector<float> p = SegmentData()->accessFrame(base+i)->get_float(x, y);
      if (p.size()!=3)
        throw "Feature::GetPixelFloats() p.size()!=3";
    
      if (PixelVerbose())
        cout << "Segmentation::GetPixelRGB01(" << base+i << ","
             << x << "," << y << ")->(" << p[0] << "," << p[1] << "," << p[2]
             << ") @" << (void*)SegmentData()->accessFrame(base+i) << endl;
    
      switch (t) {
      case pixel_rgba: v[o+3] = 0; /* fall-thru */
      case pixel_rgb:  v[o] = m*p[0]; v[o+1] = m*p[1]; v[o+2] = m*p[2]; break;
      case pixel_grey: v[o] = m*(p[0]+p[1]+p[2])/3; break;
      default: throw "Feature::GetPixelFloats() unknown type";
      }
      o += PixelTypeSize(t);
    }
  
    if (PixelVerbose()) {
      cout << "Feature::GetPixelFloats(" << x << "," << y << ") :";
      for (size_t j=0; j<v.size(); j++)
        cout << " " << v[j];
      cout << " base=" << base << " multiply=" << m << endl;
    }

    return v;
  }

  //===========================================================================

  bool Feature::moveRegion(const Region& r, int from, int to) {
    // default implementation does it with brute force

    coordList l=r.listCoordinates();

    coordList::const_iterator it;
    for(it=l.begin();it!=l.end();it++)
      SegmentData()->set_pixel_segment(*it,to);

    if (CalculateOneLabel(SegmentData()->getCurrentFrame(),from) &&
        CalculateOneLabel(SegmentData()->getCurrentFrame(),to))
      return true;

    return false;
  }

  //===========================================================================

  bool Feature::ResolveRotating(const string& rotating) {
    string descr;
    if (!ResolveRotating(CurrentFrame(), *SegmentData(), rotating,
                         rotateinfo, scaleinfo, descr))
      return false;
    
    AddRegionDescriptor(descr, "");
    
    return true;
  }

  //===========================================================================

  bool Feature::ResolveRotating(const imagedata& img, const segmentfile& sf,
                                const string& rotspec, scalinginfo& rotinfo,
                                scalinginfo& sclinfo, string& descr) {
    string msghead = "Feature::ResolveRotating("+rotspec+") : ";

    string key, value = rotspec;
    if (rotspec.find('=')!=string::npos)
      SplitOptionString(rotspec, key, value);

    string rotatedbox = value, boxsize;
    size_t colon = value.find(':');
    if (colon!=string::npos){
      rotatedbox = value.substr(0, colon);
      boxsize    = value.substr(colon+1);
    }

    int wimgorig = img.width(), himgorig = img.height();
    int lorig = 0, torig = 0, wboxorig = wimgorig-1, hboxorig = himgorig-1;
    float theta = 0.0, xc = 0.0, yc = 0.0;

    if (!rotatedbox.empty()) {
      SegmentationResult *segres =
        sf.readLastFrameResultFromXML(rotatedbox, "rotated-box");

      if (segres) {
	const char *valstr = segres->value.c_str();
	if (sscanf(valstr, "%d,%d,%d,%d,%f,%f,%f", &lorig, &torig, &wboxorig,
		   &hboxorig, &theta, &xc, &yc)!=7 &&
	    sscanf(valstr, "%d%d%d%d%f%f%f", &lorig, &torig, &wboxorig,
		   &hboxorig, &theta, &xc, &yc)!=7) {
	  delete segres;
	  throw msghead+"rotated-box object <" + rotatedbox + "> not found";
	}
	delete segres;
      }

      if (!segres) {
	segres = sf.readLastFrameResultFromXML(rotatedbox, "box");

	if (segres) {
	  int xtmp = 0, ytmp = 0;
	  const char *valstr = segres->value.c_str();
	  if (sscanf(valstr, "%d,%d,%d,%d",
		     &lorig, &torig, &xtmp, &ytmp)!=4 &&
	      sscanf(valstr, "%d%d%d%d",
		     &lorig, &torig, &xtmp, &ytmp)!=4) {
	    delete segres;
	    throw msghead+"box object <" + rotatedbox + "> not found";
	  }
	  delete segres;
	  wboxorig = xtmp-lorig+1, hboxorig = ytmp-torig+1;
	}
      }

      if (!segres)
        throw msghead+"segmentation result "+rotatedbox+" not available";
    }

    int wbox = wboxorig, hbox = hboxorig;

    size_t times = boxsize.find('x');
    if (times!=string::npos) {
      wbox = atoi(boxsize.substr(0, times).c_str());
      hbox = atoi(boxsize.substr(times+1).c_str());
    }

    float wscale = (float)wbox/wboxorig;
    float hscale = (float)hbox/hboxorig;
    int wimg = (int)floor(wimgorig*wscale+0.5);
    //int himg = (int)floor(himgorig*wscale+0.5);
    int himg = (int)floor(himgorig*hscale+0.5);

    sclinfo = picsom::scalinginfo(wimgorig, himgorig, wimg, himg);
    rotinfo = picsom::scalinginfo(-theta, xc, yc);

    float fl, ft;
    rotinfo.rotate_dst_xy((float)lorig-xc, (float)torig-yc, fl, ft); 

    int x0_box, y0_box;
    x0_box = (int)floor((0.5*wboxorig+(fl+xc))*wscale+0.5);
    //y0_box = (int)floor((0.5*hboxorig+(ft+yc))*wscale+0.5);
    y0_box = (int)floor((0.5*hboxorig+(ft+yc))*hscale+0.5);

    char tmp[1024];
    sprintf(tmp, "(%d,%d):%dx%d", x0_box, y0_box, wbox, hbox);
    descr = tmp;

    return true;
  }

//=============================================================================

bool Feature::ResolveScaling(const string& scal_spec) {
  if (scal_spec.empty())
    return true;

  string msghead = "Feature::ResolveScaling("+scal_spec+") : ";

  string key, value;
  SplitOptionString(scal_spec, key, value);
 
  bool do_scale = false, do_width = false;
  int kl = key.length();
  string box;
  if (key=="scale") {
    do_scale = true;

  } else if (key=="width" || key.substr(kl-6)==".width") {
    do_width = true;
    box = key=="width" ? "" : key.substr(0, kl-6);

  } else if (key=="height" || key.substr(kl-7)==".height") {
    do_width = false;
    box = key=="height" ? "" : key.substr(0, kl-7);

  } else 
    throw msghead+"not scale nor width nor height";

  int worig = Width(), horig = Height();
  int x0 = 0, y0 = 0, x1 = worig-1, y1 = horig-1;

  if (!box.empty()) {
    SegmentationResult *segres = 
      SegmentData()->readLastFrameResultFromXML(box, "box");

    if (!segres)
      throw msghead+"segmentation result " + box + " not available";

    const char *valstr = segres->value.c_str();
    if (sscanf(valstr, "%d,%d,%d,%d", &x0, &y0, &x1, &y1)!=4 &&
        sscanf(valstr, "%d%d%d%d", &x0, &y0, &x1, &y1)!=4) {
      delete segres;
      throw msghead+"box object <" + box + "> not found";
    }
    delete segres;
  }

  float dimval = atof(value.c_str());
  float dimmul = do_scale ? dimval :
    dimval/(do_width ? x1+1-x0 : y1+1-y0);

  int wresult = (int)floor(worig*dimmul+0.5);
  int hresult = (int)floor(horig*dimmul+0.5);

  scaleinfo = picsom::scalinginfo(worig, horig, wresult, hresult);

  int r_x0 = x0, r_y0 = y0, r_x1 = x1, r_y1 = y1;
  ScaleForwards(r_x0, r_y0);
  ScaleForwards(r_x1, r_y1);

  if (FrameVerbose())
    cout << msghead << box << "." << (do_width?"WIDTH":"HEIGHT")
         << "=" << dimval << " (x" << dimmul << ")"
         << "  frame: " << worig << "x" << horig
         << " -> " << wresult << "x" << hresult
         << "  box: " << x0 << "," << y0 << "," << x1 << "," << y1
         << " -> " << r_x0 << "," << r_y0 << "," << r_x1 << "," << r_y1
         << endl;

  return true;
}

//=============================================================================

bool Feature::PerformRotating(int f) {
  if (!rotateinfo.is_rotate_set())
    return true;

  char tmp[100];
  sprintf(tmp, "Feature::PerformRotating(%d)", f);
  string msghead = tmp;

  if (!HasImage())
    throw msghead + " : no image";

  SegmentData()->rotateFrame(f, rotateinfo);

  if (FrameVerbose())
    cout << msghead << endl;

  return true;
}

//=============================================================================

bool Feature::PerformScaling(int f) {
  if (!scaleinfo.is_set())
    return true;

  char tmp[100];
  sprintf(tmp, "Feature::PerformScaling(%d)", f);
  string msghead = tmp;

  if (!HasImage())
    throw msghead + " : no image";

  // image->RescaleImageNew(f, scaleinfo);
  // throw "image resclaing now not implemented";

  string str1 = SegmentData()->imageFrame(f)->info();

  scaleinfo.stretch(true);

  SegmentData()->rescaleFrame(f, scaleinfo);

  string str2 = SegmentData()->imageFrame(f)->info();

//   if (GetSegmentation() && segments!=image)
//     GetSegmentation()->RescaleImageNew(f, scaleinfo);

  if (FrameVerbose())
    cout << msghead << " " << str1 << " -> " << str2 << endl;

  return true;
}

  //===========================================================================

  string Feature::ExpandPreProcessingAlias(const string& sin) const {
    string msg = "Feature::ExpandPreProcessingAlias("+sin+") : ";

    string all_0 = ":";
    string all_a = "a0:shift(a0) a1:shift(a1) a2:shift(a2) a3:shift(a3)"; 
    string all_b = "b0:shift(b0) b1:shift(b1) b2:shift(b2) b3:shift(b3)"; 

    string s = sin;

    size_t p = string::npos;

    if (p==string::npos) {
      p = s.find("shift(all_0)");
      if (p!=string::npos)
	s = all_0;
    }
    if (p==string::npos) {
      p = s.find("shift(all_a)");
      if (p!=string::npos)
	s = all_a;
    }
    if (p==string::npos) {
      p = s.find("shift(all_b)");
      if (p!=string::npos)
	s = all_b;
    }
    if (p==string::npos) {
      p = s.find("shift(all_0a)");
      if (p!=string::npos)
	s = all_0+" "+all_a;
    }
    if (p==string::npos) {
      p = s.find("shift(all_0b)");
      if (p!=string::npos)
	s = all_0+" "+all_b;
    }
    if (p==string::npos) {
      p = s.find("shift(all_ab)");
      if (p!=string::npos)
	s = all_a+" "+all_b;
    }
    if (p==string::npos) {
      p = s.find("shift(all_0ab)");
      if (p!=string::npos)
	s = all_0+" "+all_a+" "+all_b;
    }

    if (p!=string::npos) {
      string pre = sin.substr(0, p), pos;
      size_t x = sin.find(')', p);
      if (x!=string::npos)
	pos = sin.substr(x+1);

      vector<string> ss = SplitInSpaces(s);
      for (size_t i=0; i<ss.size(); i++) {
	size_t q = ss[i].find(':');
	ss[i].insert(q+1, pre);
	ss[i] += pos;
      }
      s = JoinWithString(ss, " ");
    }

    if (MethodVerbose())
      cout << msg << "\"" << sin << "\" expanded to \"" << s << "\"" << endl;

    return s;
  }

  //===========================================================================

  bool Feature::SetPreProcessing(pp_chain_t& ppm, const string& spec) {
    string msg = "Feature::SetPreProcessing("+spec+") : ";

    vector<string> parts = SplitInSpaces(spec);
    for (size_t p=0; p<parts.size();) {
      string sraw = parts[p];
      string s = ExpandPreProcessingAlias(sraw);
      vector<string> parts2 = SplitInSpaces(s);
      if (parts2.size()>1) {
	auto i = parts.begin()+p;
	parts.erase(i);
	parts.insert(i, parts2.begin(), parts2.end());
      } else
	p++;
    }

    for (size_t p=0; p<parts.size(); p++) {
      string s = parts[p];
      size_t q = s.find(':');
      if (q==string::npos)
	return ShowError(msg+"failed to find ':' in '"+s+"'");
      string n = s.substr(0, q);
      s.erase(0, q+1);

      vector<pair<string,string> > descr;
      vector<string> methods = SplitInCommasObeyParentheses(s);
      for (size_t i=0; i<methods.size(); i++) {
	string m = methods[i], arg;
	q = m.find('(');
	if (q!=string::npos) {
	  arg = m.substr(q+1, m.size()-q-2);
	  m.erase(q);
	}
	descr.push_back(make_pair(m, arg));
      }

      ppm.push_back(make_pair(n, descr));

      if (MethodVerbose())
	cout << msg << "defined preprocessing method '"+n+"' from '"+spec+"'"
	     << endl;
    }

    return true;
  }

  //===========================================================================

  bool Feature::PreProcess(const string& ot, imagedata& imgd,
			   const pp_chain_t& ppm, const string& m,
			   int f) const {
    string msg = "Feature::PreProcess("+m+","+ToStr(f)+") : ";

    bool display_it = false; // FrameVerbose();
    // display_it = true;

    if (FrameVerbose())
      cout << msg << "for " << ot << " starting " << imgd.info() << endl;

    SetupPreProcessMethods();

    auto i = ppm.begin();
    while (i!=ppm.end()&&i->first!=m)
      i++;
    if (i==ppm.end())
      return m=="" ? true : ShowError(msg+"method '"+m+"' not found");

    for (auto j=i->second.begin(); j!=i->second.end(); j++)
      if (!PreProcessMethod(imgd, j->first, j->second, f))
	return ShowError(msg+"method '"+m+"' failed with "+j->first
			 +"("+j->second+")");

    if (FrameVerbose())
      cout << msg << "for " << ot << " ending " << imgd.info() << endl;

    if (display_it)
      imagefile::display(imgd);

    return true;
  }

  //===========================================================================

  bool Feature::PreProcessMethod(imagedata& imgd, const string& m,
				 const string& a, int f) const {
    string msg = "Feature::PreProcessMethod("+m+","+a+","+ToStr(f)+") : ";
    
    if (FrameVerbose())
      cout << msg << "starting" << endl;

    if (m=="" && a=="")
      return true;

    for (size_t i=0; i<preprocess_method_info.size(); i++)
      if (preprocess_method_info[i].name==m)
	return (this->*preprocess_method_info[i].func)(imgd, a);

    throw msg+"named method not found";
  }

  //===========================================================================

  void Feature::SetupPreProcessMethods() {
    static bool done = false;
    if (done)
      return;

    if (FrameVerbose())
      cout << "Feature::SetupPreProcessMethods()" << endl;

    done = true;

    vector<preprocess_method_info_t>& pmi = preprocess_method_info;
    typedef preprocess_method_info_t e;

    pmi.push_back(e("none",     &Feature::PreProcess_none));
    pmi.push_back(e("multiply", &Feature::PreProcess_multiply));
    pmi.push_back(e("shift",    &Feature::PreProcess_shift));
    pmi.push_back(e("crop",     &Feature::PreProcess_crop));
    pmi.push_back(e("trim",     &Feature::PreProcess_trim));
    pmi.push_back(e("resize",   &Feature::PreProcess_resize));
    pmi.push_back(e("hflip",    &Feature::PreProcess_hflip));
  }

  //===========================================================================

  bool Feature::PreProcess_none(imagedata& imgd, const string& arg) const {
    string msg = "Feature::PreProcess_none("+arg+") : ";
    if (FrameVerbose())
      cout << msg << imgd.info() << endl;

    return true;
  }

  //===========================================================================

  bool Feature::PreProcess_multiply(imagedata& imgd, const string& arg) const {
    string msg = "Feature::PreProcess_multiply("+arg+") : ";

    if (FrameVerbose())
      cout << msg << "starting " << imgd.info() << endl;

    if (arg=="")
      return ShowError(msg+"argument needed");

    float m = atof(arg.c_str());
    imgd = imgd.multiply(m);

    // int base = SegmentData()->getCurrentFrame();
    // imagedata *img = SegmentData()->accessFrame(base);
    // *img = img->multiply(m);

    if (FrameVerbose())
      cout << msg << "ending " << imgd.info() << endl;

    return true;
  }

  //===========================================================================

  bool Feature::PreProcess_shift(imagedata& img, const string& a) const {
    string msg = "Feature::PreProcess_shift("+a+") : ";

    int x = 0, y = 0;
    if (a=="0")
      ; // nothing
    else if (a=="a0")
      y = -1;
    else if (a=="a1")
      x = 1;
    else if (a=="a2")
      y = 1;
    else if (a=="a3")
      x = -1;

    else if (a=="b0") {
      x = 1; y = -1;
    } else if (a=="b1") {
      x = 1; y = 1;
    } else if (a=="b2") {
      x = -1; y = 1;
    } else if (a=="b3") {
      x = -1; y = -1;

    } else if (sscanf(a.c_str(), "%d,%d", &x, &y)!=2)
      return ShowError(msg+"x,y should be given as argument");

    // int base = SegmentData()->getCurrentFrame();
    // imagedata& img = *SegmentData()->accessFrame(base);

    if (FrameVerbose())
      cout << msg << "shifting " << img.info() << " with x=" << x
	   << " y=" << y << endl;

    scalinginfo si(img.width(), img.height(), img.width(), img.height(),
		   0, 0, x, y);
    si.stretch(true);

    img.rescale(si);    

    if (FrameVerbose())
      cout << msg << "result " << img.info() << endl;

    return true;
  }

  //===========================================================================

  static int abs_or_rat(const string& v, size_t m) {
    if (v.substr(0, 2)=="0.")
      return floor(atof(v.c_str())*m+0.5);

    return atoi(v.c_str());
  }

  bool Feature::PreProcess_crop(imagedata& img, const string& a) const {
    string msg = "Feature::PreProcess_crop("+a+") : ";

    if (a=="")
      return ShowError(msg+"cannot have empty argument");
    
    int l = 0, r = 0, t = 0, b = 0;

    vector<string> av = SplitInCommasObeyParentheses(a);

    if (av.size()==1)
      l = r = t = b = atoi(av[0].c_str());

    else if (av.size()==2) {
      l = r = atoi(av[0].c_str());
      t = b = atoi(av[1].c_str());

    } else if (av.size()==4) {
      l = abs_or_rat(av[0], img.width());
      r = abs_or_rat(av[1], img.width());
      t = abs_or_rat(av[2], img.height());
      b = abs_or_rat(av[3], img.height());

    } else 
      return ShowError(msg+"arguments not understood");

    // int base = SegmentData()->getCurrentFrame();
    // imagedata& img = *SegmentData()->accessFrame(base);

    if (FrameVerbose())
      cout << msg << "cropping " << img.info() << " with l=" << l
	   << " t=" << t << " r=" << r << " b=" << b << endl;

    imagedata sub = img.subimage(l, t, img.width()-1-r, img.height()-1-b);
    img = sub;

    if (FrameVerbose())
      cout << msg << "result " << img.info() << endl;

    return true;
  }

  //===========================================================================

  bool Feature::PreProcess_trim(imagedata& img, const string& a) const {
    string msg = "Feature::PreProcess_trim("+a+") : ";

    if (a!="")
      return ShowError(msg+"trim should not have argument");
    
    float th = 3.0/32;

    int l = 0, r = 0, t = 0, b = 0;

    size_t w = img.width(), h = img.height();
    bool xl = true, xr = true, xt = true, xb = true;
    for (size_t x=0; x<w/2; x++) {
      double dlm = 0, drm = 0;
      for (size_t y=1; y<h; y++) {
	vector<float> v1 = img.get_float(x,     y  );
	vector<float> v2 = img.get_float(x,     y-1);
	vector<float> v3 = img.get_float(w-1-x, y  );
	vector<float> v4 = img.get_float(w-1-x, y-1);
	double dl = 0, dr = 0;
	for (size_t i=0; i<3; i++) {
	  dl += fabs(v1[i]-v2[i]);
	  dr += fabs(v3[i]-v4[i]);
	}
	if (dl>dlm) dlm = dl;
	if (dr>drm) drm = dr;
      }

      if (dlm>th)
	xl = false;
      if (drm>th)
	xr = false;

      l += xl;
      r += xr;
      
      if (PixelVerbose())
	cout << "x : " << x << " " << dlm << " " << drm
	     << " (" << th << ") " << xl << " " << xr << endl;

      if (!xl && !xr)
	break;
    }

    for (size_t y=0; y<h/2; y++) {
      double dtm = 0, dbm = 0;
      for (size_t x=1; x<w; x++) {
	vector<float> v1 = img.get_float(x,   y    );
	vector<float> v2 = img.get_float(x-1, y    );
	vector<float> v3 = img.get_float(x,   h-1-y);
	vector<float> v4 = img.get_float(x-1, h-1-y);
	double dt = 0, db = 0;
	for (size_t i=0; i<3; i++) {
	  dt += fabs(v1[i]-v2[i]);
	  db += fabs(v3[i]-v4[i]);
	}
	if (dt>dtm) dtm = dt;
	if (db>dbm) dbm = db;
      }

      if (dtm>th)
	xt = false;
      if (dbm>th)
	xb = false;

      t += xt;
      b += xb;

      if (PixelVerbose())
	cout << "y : " << y << " " << dtm << " " << dbm 
	     << " (" << th << ") " << xt << " " << xb << endl;

      if (!xt && !xb)
	break;
    }

    if (FrameVerbose())
      cout << msg << "trimming " << img.info() << " with l=" << l
	   << " t=" << t << " r=" << r << " b=" << b << endl;

    imagedata sub = img.subimage(l, t, img.width()-1-r, img.height()-1-b);
    img = sub;

    if (FrameVerbose())
      cout << msg << "result " << img.info() << endl;

    return true;
  }

  //===========================================================================

  bool Feature::PreProcess_resize(imagedata& img, const string& a) const {
    string msg = "Feature::PreProcess_resize("+a+") : ";

    float mul = atof(a.c_str());
    if (!mul)
      return ShowError(msg+"scaling factor should be given as argument");

    if (FrameVerbose())
      cout << msg << "resizing " << img.info() << " with scale="
	   << mul << endl;

    scalinginfo si(img.width(), img.height(),
		   (int)floor(mul*img.width()+0.5),
		   (int)floor(mul*img.height()+0.5));

    img.rescale(si);    

    if (FrameVerbose())
      cout << msg << "result " << img.info() << endl;

    return true;
  }

  //===========================================================================

  bool Feature::PreProcess_hflip(imagedata& img, const string& a) const {
    string msg = "Feature::PreProcess_hflip("+a+") : ";

    if (a!="")
      return ShowError(msg+"no argument should be given");

    if (FrameVerbose())
      cout << msg << "horizontal flipping " << img.info() << endl;

    for (size_t y=0; y<img.height(); y++)
      for (size_t x=0; x<img.width()/2; x++) {
	vector<float> v = img.get_float(x, y);
	img.set(x, y, img.get_float(img.width()-1-x, y));
	img.set(img.width()-1-x, y, v);
      }

    return true;
  }

  //===========================================================================

  bool Feature::SetZnormalize(const string& cod) {
    string msg = "Feature::SetZnormalize("+cod+")";
    if (MethodVerbose())
      cout << msg  << endl;
    msg+" : ";

    z_norm.clear();

    XmlDom xml = ReadXML(cod);
    if (!xml.DocOK()) {
      cerr << msg+"ReadXML() failed" << endl;
      return false;
    }

    XmlDom root = xml.Root();
    if (root.NodeName()!="feature")
      root = root.FindChild("feature");
    if (root.NodeName()!="feature") {
      cerr << msg+"<feature> not found" << endl;
      return false;
    }

    XmlDom comps = root.FindChild("components");
    if (!comps) {
      cerr << msg+"<components> not found" << endl;
      return false;
    }

    for (XmlDom comp = comps.FirstChild(); comp; comp=comp.Next()) {
      if (!comp.IsElement() || comp.NodeName()!="component")
        continue;
      z_norm.push_back(make_pair(atof(comp.Property("mean") .c_str()),
                                 atof(comp.Property("stdev").c_str())));
    }

    xml.DeleteDoc();

    return true;
  }

  //===========================================================================

   vector<float> Feature::DoNormalize(const vector<float>& raw) const {
    if (PixelVerbose())
      cout << "Feature::DoNormalize()" << endl;
    
    if (z_norm.empty())
      return raw;

    if (z_norm.size()!=raw.size())
      throw string("Feature::DoNormalize() z_norm.size()!=raw.size()");

    vector<float> v(z_norm.size());
    for (size_t i=0; i<v.size(); i++)
      v[i] = (raw[i]-z_norm[i].first)/z_norm[i].second;

    return v;
  }

  //===========================================================================

  XmlDom Feature::ReadXML(const string& fn) const {
    string cmd = "zcat -f "+fn;
    FILE *dat = popen(cmd.c_str(), "r");
    if (!dat) {
      cerr << "Feature::ReadXML() : failed to open " << fn
           << " for reading." << endl;
      exit(-1);
    }

    string xmlstr;
    
    const int LINELENGTH=1000000;
    char *line = new char[LINELENGTH];
    while (!feof(dat)) {
      if (!fgets(line, LINELENGTH, dat)) {
        cerr << "error reading dat file" << endl;
        exit(-1);
      }

      if (line[0]!='#')
        break;

      string str(line);
      size_t p = str.find_first_not_of("#");
      if (p!=string::npos)
        str.erase(0, p);
      p = str.find_first_not_of(" \t\n");
      if (p==string::npos) {
        if (xmlstr=="")
          continue;
        else
          break;
      }
      str.erase(0, p);

      if (xmlstr=="" && str.find("<?xml version")==string::npos)
        continue;
      
      xmlstr += str;
    }

    delete line;
    fclose(dat);

    if (xmlstr=="")
      return XmlDom();

    xmlDocPtr doc = xmlParseMemory(xmlstr.c_str(), xmlstr.size());
    xmlNsPtr ns = NULL;
    return XmlDom(doc, ns, NULL);
  }

  //===========================================================================

  string Feature::IntermediateFileStub(int f) const {
    string inpat = StandardFilenamePattern(4, false);  // like -OR
    string ofile = FilenameFromPattern(inpat, ofn_method,
                                       ofn_feature, ofn_files);
    if (ofile.empty() || ofile.rfind(".dat")!=ofile.size()-4)
      return "";

    ofile.erase(ofile.size()-4);

    stringstream ss;
    if (f>=0)
      ss << "+" << f << "-";
    ofile += ss.str();

    if (FileVerbose())
      cout << "IntermediateFileStub(" << f << ") returns ["+ofile+"]" << endl;

    return ofile;
  }

  //===========================================================================

  bool Feature::SaveIntermediate(int f, const list<string>& res) const {
    if (res.empty())
      return false;
    
    string ofile = IntermediateFileStub(f);
    if (ofile.empty())
      return false;

    size_t i = 0;
    for (list<string>::const_iterator r=res.begin(); r!=res.end(); r++, i++) {
      char idx[3];
      sprintf(idx, "%02d", (int)i);

      string fname = ofile+idx+".tmp";

      ofstream os(fname.c_str());
      os << *r;
      if (!os)
        return false;
      
      if (FileVerbose())
        cout << "Wrote " << r->size() << " bytes in intermediate file <"
             << fname << ">" << endl;
    }

    return true;
  }

  //===========================================================================

  list<string> Feature::IntermediateFiles(int f) const {
    string ofile = IntermediateFileStub(f);

    list<string> l;

    string dir = ofile;
    size_t p = dir.rfind('/');
    if (p==string::npos)
      return l;

    dir.erase(p);
    
    string stub = ofile.substr(p+1);

    set<string> set;

#ifdef HAVE_DIRENT_H
    DIR *dv = opendir(dir.c_str());  
    for (dirent *dpv; dv && (dpv = readdir(dv)); ) {
      string name = dpv->d_name;
      if (name.find(stub)==0) {
        string fullname = dir+"/"+name;
        set.insert(fullname);
        if (FileVerbose())
          cout << "Found intermediate file <" << fullname << ">" << endl;
      }
    }
    if (dv)
      closedir(dv);
#endif // HAVE_DIRENT_H

    l.insert(l.end(), set.begin(), set.end());

    return l;
  }

  //===========================================================================

  list<string> Feature::ReadIntermediate(int f) const {
    list<string> fname = IntermediateFiles(f);
    return ReadIntermediateInner(fname);
  }

  //===========================================================================

  list<string> Feature::ReadIntermediateInner(const list<string>& fname) const{
    list<string> l;

    for (list<string>::const_iterator i=fname.begin(); i!=fname.end(); i++) {
      string s = ReadFile(*i);
      l.push_back(s);
      if (FileVerbose())
        cout << "Read " << s.size() << " bytes of intermediate data from <"
             << *i << ">" << endl;
    }

    return l;
  }

  //===========================================================================

  string Feature::ReadFile(const string& fname) const {
    ifstream is(fname.c_str());
    string s;
    const size_t bufsize = 1024*1024*4;
    char *buf = new char[bufsize];
    while (is) {
      is.read(buf, bufsize);
      streamsize n = is.gcount();
      s.append(buf, n);
    }
    delete buf;

    return s;
  }

  //===========================================================================

  bool Feature::ProcessRawOutVector(const vector<float>& v, int x, int y) {
    return !IsRawOutMode() || PrintAndAppendRawVector(v, x, y);
  }

  //===========================================================================

  bool Feature::PrintAndAppendRawVector(const vector<float>& v, int x, int y) {
    if (!IsRawOutMode())
      return true;

    string l = RawLabel(x, y);
    
    if (fout_xx) {
      for (size_t i=0; i<v.size(); i++)
        *fout_xx << (i?" ":"") << v[i];

      *fout_xx << (l!=""?" ":"") << l << endl;
    }

    AppendResult(v, l);

    return true;
  }

  //===========================================================================

  bool Feature::readHistogramBins(const string& fnin) {
    hist_file.clear();
    hist_desc.clear();
    histogramCodebook.clear();

    size_t lp = fnin.find('{'), rp = fnin.find('}');
    if (lp<rp && rp!=string::npos) {
      string mid = fnin.substr(lp+1, rp-lp-1);
      while (mid!="") {
        size_t com = mid.find(',');
        string part = mid.substr(0, com);
        mid.erase(0, com+1);
        hist_file.push_back(fnin.substr(0, lp)+part+fnin.substr(rp+1));
        if (mid==part)
          break;
      }

    } else
      hist_file.push_back(fnin);

    for (vector<string>::const_iterator fni=hist_file.begin();
         fni!=hist_file.end(); fni++) {
      const string& fn = *fni;

      string descr;
      dataset bindata;
      bindata.readDataset(fn, descr);
      histogramCodebook.insert(histogramCodebook.end(),
                               bindata.vec.begin(), bindata.vec.end());
      hist_desc.push_back(make_pair(descr, fn));

      if (FileVerbose())
        cout << "Inserted " <<  bindata.vec.size() <<" entries of type ["
             << descr << "] from <" << fn << "> to histogram codebook" << endl;
     }
    
    hist_file_spec = fnin;
    hist_desc_sum = "";

    if (hist_desc.size()>1 && hist_desc[0].first!="") {
      bool mismatch = false;
      for (size_t i=1; !mismatch && i<hist_desc.size(); i++)
        if (hist_desc[i].first!=hist_desc[i-1].first)
          mismatch = true;
      if (!mismatch) {
        stringstream ss;
        ss << hist_desc.size() << "X" << HistogramSpecification(hist_desc[0]);
        hist_desc_sum = ss.str();
      }
    }
    if (hist_desc_sum=="")
      for (size_t i=0; i<hist_desc.size(); i++)
        hist_desc_sum += (i?"+":"")+HistogramSpecification(hist_desc[i]);

    if (FileVerbose())
      cout << "Created codebook of " << histogramCodebook.size()
           <<" entries of type [" << hist_desc_sum << "] from <"
           << hist_file_spec << ">" << endl;
    
    return true;
  }

  //===========================================================================

  Feature::hist_norm_type Feature::HistogramNormalization(const string& fnin) {
    if (fnin == "L2" || fnin == "l2")
      return hist_norm_L2;
    else if (fnin == "L1" || fnin == "l1")
      return hist_norm_L1;
    else if (fnin == "L2C" || fnin == "l2c")
      return hist_norm_L2_capped;
    else if (fnin == "L1C" || fnin == "l1c")
      return hist_norm_L1_capped;
    else if (fnin == "none")
      return hist_norm_none;
    ShowError("Feature::HistogramNormalization() : unable to interpret [", 
	      fnin, "]");
    return hist_norm_none;	
  }
  
  //===========================================================================

  const vector<float>& Feature::NextRawVector(int x, int y) {
    if (!raw_input_stream.is_open()) {
      raw_input_dim = 0;

      if (FileVerbose())
        cout << "Opening raw input file <" << raw_input_filename << ">"
             << endl;
        
      raw_input_stream.open(raw_input_filename.c_str());
      if (!raw_input_stream)
        cout << "Feature::NextRawVector() : failed to open <"
             << raw_input_filename << ">" << endl;

      while (raw_input_stream) {
        string line;
        getline(raw_input_stream, line);
        if (!raw_input_stream)
          break;
        if (line[0]=='#')
          continue;
        raw_input_dim = atoi(line.c_str());
        if (FileVerbose())
          cout << "Raw input vector dimensionality is " << raw_input_dim
               << endl;
        break;
      }
    }

    string line;
    getline(raw_input_stream, line);
    if (raw_input_stream) {
      if (raw_input_vector.size()!=raw_input_dim)
        raw_input_vector  = vector<float>(raw_input_dim);
      stringstream oss(line);
      for (size_t i=0; i<raw_input_vector.size(); i++)
        oss >> raw_input_vector[i];

      if (PixelVerbose())
        cout << "Feature::NextRawVector(" << x << "," << y << ") success"
             << endl;
    }

    if (!raw_input_stream) {
      raw_input_dim = 0;
      raw_input_vector = vector<float>(0);
    }

    return raw_input_vector;
  }

  //===========================================================================

  string Feature::HistogramSpecification(const string& descr,
                                         const string& fname) const {
    if (descr=="")
      return "";

    string s = descr;
    if (s.find("rect ")==0)
      s.erase(0, 5);
    size_t p = s.find(' ');
    if (p!=string::npos) {
      s[p] = 'x';
      p = s.find(' ', p);
      if (p!=string::npos)
        s.erase(p);
    }

    p = fname.rfind('/');
    if (p==string::npos)
      p = 0;
    p = fname.find(s, p);
    size_t q = fname.find(".cod", p);
    if (q!=string::npos)
      s = fname.substr(p, q-p);

    return s;
  }

  //===========================================================================

  int Feature::vector2codebookidx(const featureVector& v) const {
    if (histogramCodebook.empty() || histogramCodebook[0].size()!=v.size())
      return -1;
    
    int minind = 0;
    float mindist = VectorSqrDistance(histogramCodebook[0], v);
    for (size_t idx=1, n=histogramCodebook.size(); idx<n; idx++){
      float d = VectorSqrDistance(histogramCodebook[idx], v, 0, -1, mindist);
      if (d<mindist) {
        minind  = idx;
        mindist = d;
      }
    }

    // cout << "featurevector ";
    // printFeatureVector(cout,v);
    // cout << endl;
    // cout << " -> index " << minind << endl;

    return minind;
  }

  //===========================================================================

  void Feature::Data::addToHistogram(const featureVector& v, int add) {
    if (Parent()->histogramMode.empty())
      return;

    if (Parent()->histogramMode=="fixed") {
      if (hist_values.empty()){
        // first invocation, allocate storage for histogram
        size_t nbins = 1, dim = hist_limits.size();
        for (size_t d=0; d<dim; d++)
          nbins *= hist_limits[d].size()+1;
        
        hist_values = vector<size_t>(nbins, 0); 
        
        if (FileVerbose())
          cout << "allocated fixed histogram of size "<< hist_values.size()
               << endl;
      }
      hist_values[vector2fixedhistbin(v)] += add;

    } else if (Parent()->histogramMode=="codebook") {
      if (hist_values.empty()){
        hist_values = vector<size_t>(Parent()->histogramCodebook.size(), 0); 
        if (FileVerbose())
          cout << "allocated codebook histogram of size "<< hist_values.size()
               << endl;
      }
      int idx = Parent()->vector2codebookidx(v);
      if (idx>=0 && idx<(int)hist_values.size())
        hist_values[idx] += add;
      else
        cerr << "histogram index " << idx
             << " is outside of allowed range 0.." << hist_values.size()-1
             << endl;

    } else
      cerr << "histogram mode <" << Parent()->histogramMode
           << "> unknown" << endl;
  }

  //===========================================================================

  void Feature::Data::initHistogram() {

    // calling this is necessary only if no vectors are added to histogram
    // but it's desirable to allocate the histogram vector anyway 

    if (Parent()->histogramMode.empty())
      return;

    if (Parent()->histogramMode=="fixed") {
      if (hist_values.empty()){
        // first invocation, allocate storage for histogram
        size_t nbins = 1, dim = hist_limits.size();
        for (size_t d=0; d<dim; d++)
          nbins *= hist_limits[d].size()+1;
        
        hist_values = vector<size_t>(nbins, 0); 
        
        if (FileVerbose())
          cout << "allocated fixed histogram of size "<< hist_values.size()
               << endl;
      }

    } else if (Parent()->histogramMode=="codebook") {
      if (hist_values.empty()){
        hist_values = vector<size_t>(Parent()->histogramCodebook.size(), 0); 
        if (FileVerbose())
          cout << "allocated codebook histogram of size "<< hist_values.size()
               << endl;
      }
    } else
      cerr << "histogram mode <" << Parent()->histogramMode
           << "> unknown" << endl;
  }

  //===========================================================================

  size_t Feature::Data::vector2fixedhistbin(const featureVector& v) const {
    const size_t dim=v.size();

    if(dim!=hist_limits.size()) {
      cout << "hist_limits.size="<<hist_limits.size()<<endl;
      throw string("Dimension mismatch in forming a histogram");
    }

    // cout << "featurevector ";
    //printFeatureVector(cout,v);
    //cout << endl;

    size_t ret=0;

    int quantum=1;

    for(size_t d=0;d<dim;d++){
      int nbins=hist_limits[d].size()+1;
      int dimbin;
        
      for(dimbin=0; dimbin<nbins-1 && v[d]>hist_limits[d][dimbin]; dimbin++) {}
      //        cout << "v="<<v[d]<<" hist_limits["<<dimbin<<"]="
      //             <<hist_limits[d][dimbin]<<endl;
        
      //cout << "d="<<d<<" dimbin="<<dimbin<<endl;
      ret += quantum*dimbin;
      quantum *= nbins;
    }

    // cout << " -> index " << ret << endl;

    return ret;
  }

  //===========================================================================

  void Feature::dataset::readDataset(const string& fn, string& descr) {
    const int LINELENGTH = 1000000;

    string cmd = "zcat -f "+fn;

    FILE *dat = popen(cmd.c_str(), "r");

    if (!dat) {
      cerr << "Failed to open " << fn << " for reading." << endl;
      exit(-1);
    }

    int dimx = 0;

    char line[LINELENGTH];

    while (!feof(dat)) {
      if (!fgets(line, LINELENGTH, dat)) {
        cerr << "error reading dat file A" << endl;
        exit(-1);
      }
    
      //    cerr <<"READ LINE:" <<endl << line;

      if (line[0]=='#') {
        while (line[strlen(line)-1] != '\n')
          if (!fgets(line, LINELENGTH, dat)) {
            cerr << "error reading dat file B" << endl;
            exit(-1);
          }
        continue;
      }

      if (sscanf(line,"%d",&dimx) != 1) {
        cerr << "couldn't parse dimension from first line: " << endl;
        cerr << line;
        exit(-1);
      }

      //cerr << "Read info line " << line << endl;

      dim = dimx;

      string tmp = line;
      size_t p = tmp.find_first_not_of(" \t");
      if (p!=string::npos)
        p = tmp.find_first_of(" \t", p);
      if (p!=string::npos)
        p = tmp.find_first_not_of(" \t", p);
      descr = p!=string::npos ? tmp.substr(p) : "";
      p = descr!="" ? descr.find_last_not_of(" \t\n") : string::npos;
      if (p!=string::npos)
        descr.erase(p+1);      

      int readdatavec = 0;
  
      while (!feof(dat)) {
        if (!fgets(line, LINELENGTH, dat)) {
          break;
          //cerr << "error reading dat file" << endl;
          //exit(1);
        }

        // cerr <<"READ LINE:" <<endl << line;
      
        if (line[0]=='#') {
          while (line[strlen(line)-1] != '\n')
            if (!fgets(line, LINELENGTH, dat)) {
              cerr << "error reading dat file C" << endl;
              exit(-1);
            }
          continue;
        }

        vector<float> v(dim);
    
        int offset=0;
        for (size_t i=0; i<dim; i++) {
          while (line[offset] && isspace(line[offset]))
            offset++;

          if (sscanf(line+offset,"%f",&(v[i]))!=1) {
            cerr << "data reading error, line=" << line<<endl;
            exit(-1);
          }
          while(line[offset] && !isspace(line[offset]))
            offset++;
        }

        if (v.size()==dim) {
          vec.push_back(v);

          //      char lblstr[80];
          //      if(sscanf(line+offset,"%s",lblstr) != 1)
          //        labels.push_back("");
          //      else
          //        labels.push_back(lblstr);

          readdatavec++;
        }
      }

      //cout << "dataset: read " << readdatavec 
      //         << " data vectors, dim= " << dim << endl;
    }

    pclose(dat);
  }

  //===========================================================================

  bool Feature::dataset::writeDataset(const string& fn,
				      const string& /*descr*/) const {
    ofstream os(fn);
    os << dim << endl;
    for (size_t i=0; i<vec.size(); i++) {
      for (size_t c=0; c<vec[i].size(); c++)
	os << (c?" ":"") << vec[i][c];
      os << endl;
    }
    return os.good();
  }

  //===========================================================================

  bool Feature::EnsureDirectoryExists(const string& file) {
      size_t s = 0;
      for (;;) {
        size_t n = file.find('/', s);
        if (n==string::npos)
          break;

        string subdir = file.substr(0, n);
        struct stat st;
        if (stat(subdir.c_str(), &st)) {
          int mode = 02775;
#ifdef MKDIR_ONE_ARG
          mkdir(subdir.c_str());
#else
          mkdir(subdir.c_str(), mode);
#endif // MKDIR_ONE_ARG
          chmod(subdir.c_str(), mode);
        }
        s = n+1;
      }
      return true;
    }

  //===========================================================================

  string Feature::ImageToPGM() {
    if (PixelType()!=pixel_grey) {
      cout << "ERROR: Feature:ImageToPGM() : PixelType()!=pixel_grey" <<endl;
      return "";
    }

    EnsureImage();
    int width = Width(true), height = Height(true);
    string data(width*height, '\0');

    size_t i = 0;
    for (int y=0; y<height; y++) {
      for (int x=0; x<width; x++, i++) {
        vector<float> vf = GetPixelFloats(x, y, PixelType());
        int v = (int)floor(255*vf[0]+0.5);
        data[i] = v;
      }
    }

    stringstream hdr;
    hdr << "P5" << endl << width << " " << height << endl << 255 << endl;

    return hdr.str()+data;
  }

  //===========================================================================

  bool Feature::ImageToPGM(const string& fname) {
    string pgm = ImageToPGM();
    if (pgm.empty())
      return false;

    ofstream os(fname.c_str());
    os.write(pgm.c_str(), pgm.size());

    if (os && FileVerbose())
      cout << "Stored " << pgm.size() << " bytes of PGM image in <"
           << fname << ">" << endl;

    return os.good();
  }

  //===========================================================================

  void Feature::SetTempDir(bool init) {
    static string init_tempdir;

#ifndef NO_PTHREADS
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
#endif // NO_PTHREADS

    if (init && init_tempdir!="")
      tempdir = init_tempdir;
    else {
      char *p = getenv("TMPDIR");
      tempdir = p && *p ? p : "/var/tmp";
    }
    if (init_tempdir=="")
      init_tempdir = tempdir;

#ifndef NO_PTHREADS
    pthread_mutex_unlock(&mutex);
#endif // NO_PTHREADS
  }

  //===========================================================================

  pair<bool,string> Feature::TempFile(const string& patin) {
    string defpat = "picsom_features_";
    string pat = patin!="" ? patin : defpat;
    string name = pat;

    if (name[0]!='/')
      name = TempDir()+"/"+name;

    if (name.rfind("XXXXXX")!=name.size()-6)
      name += "XXXXXX";

    char tmp[1000];
    strncpy(tmp, name.c_str(), sizeof tmp);
    tmp[sizeof(tmp)-1] = 0;

    int fd = mkstemp(tmp);
    if (fd==-1)
      return make_pair(false, name);

    close(fd);

    return make_pair(true, string(tmp));
  }

  //===========================================================================

  imagedata Feature::ExtractSegment(const string& img, const string& seg,
                                    const string& spec) {
    int f = 0;
    
    segmentfile segimg(img, seg);
    imagedata& data = *segimg.accessFrame(f);

    scalinginfo rotinfo, sclinfo;
    string reg;
    if (!ResolveRotating(data, segimg, spec, rotinfo, sclinfo, reg))
      return imagedata();

    sclinfo.stretch(true);

    segimg.rotateFrame( f, rotinfo);  // from PerformRotating()
    segimg.rescaleFrame(f, sclinfo);  // from PerformScaling()

    Region *regionspec = new rectangularRegion(reg, &segimg);
    coordList l = regionspec->listCoordinates();
    delete regionspec;  // adapted from RegionBased::CalculateRegion()

    int xmin = l.begin()->x, xmax = l.rbegin()->x;
    int ymin = l.begin()->y, ymax = l.rbegin()->y;
//     for (coordList::const_iterator i=l.begin(); i!=l.end(); i++) {
//       if (i->x<xmin)
//         xmin = i->x;
//       else if (i->x>xmax)
//         xmax = i->x;
//       if (i->y<ymin)
//         ymin = i->y;
//       else if (i->y>ymax)
//         ymax = i->y;
//     }
    // cout << xmin << "," << ymin << " -> " << xmax << "," << ymax << endl;

    imagedata imgdata = data;
    int dx = 0, dy = 0;

    if (xmin<0 || ymin<0 ||
        xmax>=(int)imgdata.width() || ymax>=(int)imgdata.height()) {
      imagedata pink(1, 1, 3);
      vector<float> pv(3);
      pv[0] = 1;
      pv[1] = pv[2] = 0.75;
      pink.set(0, 0, pv);

      int nw = imgdata.width(), nh = imgdata.height();
      if (xmin<0) {
        nw -= xmin;
        dx = -xmin;
      }
      if (ymin<0) {
        nh -= ymin;
        dy = -ymin;
      }
      if (xmax>=(int)imgdata.width())
        nw += xmax-imgdata.width()+1;
      if (ymax>=(int)imgdata.height())
        nh += ymax-imgdata.height()+1;
      
      scalinginfo si(1, 1, nw, nh);
      pink.rescale(si);
      
      pink.copyAsSubimage(imgdata, dx, dy);

      imgdata = pink;
    }

    return imgdata.subimage(dx+xmin, dy+ymin, dx+xmax, dy+ymax);
  }

  //===========================================================================

  int *Feature::getBorderSuppressionMask(int borderexpand){

    EnsureImage();
    EnsureSegmentation();

    int width = Width(true), height = Height(true);
    int size=width*height;

    int f=SegmentData()->getCurrentFrame();

    int *suppressionmask=new int[size];
    memset(suppressionmask,0,size*sizeof(int));

    int idx=0;

    for (int y=0;y<height;y++)
      for (int x=0;x<width;x++) {
	vector<int> svec = SegmentVector(f, x, y);
	vector<int> snew;

	if (x<width-1) {
	  snew = SegmentVector(f, x+1, y);
	    
	  if (!vectorsSame(svec,snew)) {
	    suppressionmask[idx]=1;
	    suppressionmask[idx+1]=1;
	  }
	}

	if (y<height-1) {
	  if (x>0) {
	    snew = SegmentVector(f, x-1, y+1);
	    
	    if (!vectorsSame(svec,snew)) {
	      suppressionmask[idx]=1;
	      suppressionmask[idx+width-1]=1;
	    }
	  }

	  snew = SegmentVector(f, x, y+1);

	  if (!vectorsSame(svec,snew)) {
	    suppressionmask[idx]=1;
	    suppressionmask[idx+width]=1;
	  }

	  if (x<width-1) {
	    snew = SegmentVector(f, x+1, y+1);
		
	    if (!vectorsSame(svec,snew)) {
	      suppressionmask[idx]=1;
	      suppressionmask[idx+width+1]=1;
	    }
	  }
	}
	idx++;
      }
      // expand the border mask

    if (borderexpand>0) {
      int templatew=2*borderexpand+1;
      int templatesize=templatew*templatew;

      int *templatemask=new int[templatesize];

      int templateorg=borderexpand*templatew+borderexpand;

      for(int dx=-borderexpand;dx<=borderexpand;dx++)
	for(int dy=-borderexpand;dy<=borderexpand;dy++)
	  templatemask[templateorg+dx+dy*templatew]=
	    (dx*dx+dy*dy<=borderexpand*borderexpand)?1:0;

      int *expmask=new int[size];
      memset(expmask,0,size*sizeof(int));

      int idx=0;

      for(int y=0;y<height;y++)
	for(int x=0;x<width;x++){
	  int mindx=-borderexpand;
	  int maxdx=borderexpand;
	  int mindy=-borderexpand;
	  int maxdy=borderexpand;
	  
	  if(x+mindx<0) mindx=x;
	  if(x+maxdx>=width) maxdx=width-x-1;
	  
	  if(y+mindy<0) mindy=y;
	  if(y+maxdy>=height) maxdy=height-y-1;

	  for(int dx=mindx;dx<=maxdx;dx++)
	    for(int dy=mindy;dy<=maxdy;dy++)
	      if(templatemask[templateorg+dx+dy*templatew] &&
		 suppressionmask[idx+dx+dy*width])
		expmask[idx]=1;

	  idx++;
	}

      delete[] templatemask;
      delete[] suppressionmask;
      suppressionmask=expmask;
    }

    return suppressionmask;
  }

  //===========================================================================

  preprocessResult_SobelThresh *Feature::getSobelThresh(int *suppressionmask){
     EnsureImage();
     preprocessResult_SobelThresh* ret= new preprocessResult_SobelThresh;

     int width = Width(true), height = Height(true);
     int size=width*height;

     int f=SegmentData()->getCurrentFrame();

     ret->d0=new int[size];
     ret->d45=new int[size];
     ret->d90=new int[size];
     ret->d135=new int[size];

     preprocessResult_Sobel* sptr=(preprocessResult_Sobel*)
       SegmentData()->accessPreprocessResult(f,"sobel");

     // suppress specified pixels in the edge image

     if(suppressionmask)
       for(int i=0;i<size;i++)
	 if(suppressionmask[i]){
	   sptr->ds0[i]=sptr->ds45[i]=sptr->ds90[i]=sptr->ds135[i]=0;
	   sptr->di0[i]=sptr->di45[i]=sptr->di90[i]=sptr->di135[i]=0;
	 }

     // now threshold channels

     // first find the maxima of all channels

     float max_ds0=0,max_ds45=0,max_ds90=0,max_ds135=0;
     float max_di0=0,max_di45=0,max_di90=0,max_di135=0;

     for(int i=0;i<size;i++){
       if(sptr->ds0[i]>max_ds0) max_ds0=sptr->ds0[i];
       if(sptr->ds45[i]>max_ds45) max_ds45=sptr->ds45[i];
       if(sptr->ds90[i]>max_ds90) max_ds90=sptr->ds90[i];
       if(sptr->ds135[i]>max_ds135) max_ds135=sptr->ds135[i];

       if(sptr->di0[i]>max_di0) max_di0=sptr->di0[i];
       if(sptr->di45[i]>max_di45) max_di45=sptr->di45[i];
       if(sptr->di90[i]>max_di90) max_di90=sptr->di90[i];
       if(sptr->di135[i]>max_di135) max_di135=sptr->di135[i];
     }

     // then combine channels and threshold

     for(int i=0;i<size;i++){
      ret->d0[i]=
	(sptr->ds0[i]>=0.35*max_ds0 || sptr->di0[i]>=0.15*max_di0)?1:0; 
      ret->d45[i]=
	(sptr->ds45[i]>=0.35*max_ds45 || sptr->di45[i]>=0.15*max_di45)?1:0; 
      ret->d90[i]=
	(sptr->ds90[i]>=0.35*max_ds90 || sptr->di90[i]>=0.15*max_di90)?1:0; 
      ret->d135[i]=
	(sptr->ds135[i]>=0.35*max_ds135 || sptr->di135[i]>=0.15*max_di135)?1:0; 
    }

     SegmentData()->discardPreprocessResult(f,"sobel");
     return ret;
  }

  //===========================================================================

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
  
  cv::Mat Feature::imagedata_to_cvMat(const imagedata& imgin) {
    imagedata img(imgin);
    img.convert(imagedata::pixeldata_uchar);

    cv::Mat m = cv::Mat::zeros(img.height(), img.width(), CV_8UC3);
    for (size_t x=0; x<img.width(); x++)
      for (size_t y=0; y<img.height(); y++) {
	vector<unsigned char> vin = img.get_uchar(x, y);
	cv::Vec3b vout(vin[2], vin[1], vin[0]);
	m.at<cv::Vec3b>(y, x) = vout;
      }
    
    return m;
  }

  //===========================================================================

  cv::Mat Feature::imagedata_to_cvMat() const {
    int base = SegmentData()->getCurrentFrame();
    const imagedata& imgin = incore_imagedata_ptr ?
      *incore_imagedata_ptr : *SegmentData()->accessFrame(base);
    return imagedata_to_cvMat(imgin);
  }

  //===========================================================================

  imagedata Feature::cvMat_to_imagedata(const cv::Mat& m) {
    imagedata img(m.cols, m.rows, m.channels(), imagedata::pixeldata_float);
    for (size_t x=0; x<(size_t)m.cols; x++)
      for (size_t y=0; y<(size_t)m.rows; y++) {
	switch (m.channels()) {
	case 1: {
	  auto in = m.at<float>(y, x);
	  vector<float> out { in };
	  img.set(x, y, out);
	  break;
	}
	case 2: {
	  auto in = m.at<cv::Vec2f>(y, x);
	  vector<float> out { in[0], in[1] };
	  img.set(x, y, out);
	  break;
	}
	case 3: {
	  auto in = m.at<cv::Vec3f>(y, x);
	  vector<float> out {in[2], in[1], in[0]};
	  img.set(x, y, out);
	  break;
	}
	default:
	  abort();
	}
      }
    return img;
  }

#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

  //===========================================================================

  bool Feature::KMeans(const string& fin, size_t k, size_t iter,
		       const string& fout) {
    string descr;
    dataset data;
    data.readDataset(fin, descr);
    size_t dim = data.dim;
    dataset means;
    means.dim = dim;
    size_t n = data.vec.size(), m = (RAND_MAX/n)*n, nan = (size_t)-1;

    cout << "read " << n << " vectors of dimensionality " << dim << endl;

    for (size_t i=0; i<k; i++) {
      size_t r = nan;
      while (r==nan) {
	size_t rx = rand();
	if (rx>=m)
	  continue;
	r = rx%n;
      }
      means.vec.push_back(data.vec[r]);
    }

    vector<size_t> aprev(n);
    for (size_t i=0; i<iter; i++) {
      cout << "starting iteration " << i;

      double totd = 0;
      vector<size_t> a(n, nan);
      for (size_t j=0; j<n; j++) {
	float mind = numeric_limits<float>::max();
	size_t minl = nan;
	for (size_t l=0; l<k; l++) {
	  float d = 0;
	  for (size_t c=0; c<dim; c++) {
	    float e = means.vec[l][c]-data.vec[j][c];
	    d += e*e;
	  }
	  if (d<mind) {
	    mind = d;
	    minl = l;
	  }
	}
	totd += mind;
	a[j] = minl;
      }

      cout << " avg.dist=" << totd/n;

      size_t ndiff = 0;
      for (size_t j=0; j<n; j++)
	if (a[j]!=aprev[j])
	  ndiff++;

      if (!ndiff) {
	cout << " -- converged" << endl;
	break;
      } else
	cout << " -- " << ndiff << " changes" << endl;

      aprev = a;

      for (size_t l=0; l<k; l++) {
	vector<float> v(dim);
	size_t nh = 0;
	for (size_t j=0; j<n; j++)
	  if (a[j]==l) {
	    for (size_t c=0; c<dim; c++)
	      v[c] += data.vec[j][c];
	    nh++;
	  }
	if (nh) {
	  for (size_t c=0; c<dim; c++)
	    v[c] /= float(nh);
	  means.vec[l] = v;
	}
      }
    }

    return means.writeDataset(fout, descr);
  }

  //===========================================================================

  bool Feature::CollectBatch(const string& f, incore_feature_t *i,
			     feature_result *r) {
    string msg = "Feature::CollectBatch() : ";
    batch.push_back(feature_batch_e(f, i, r));
    if (FileVerbose())
      cout << msg << (void*)this << " [" << batch.back().str()
	   << "] collected " << batch.size() << endl;
    return true;
  }
  
  //===========================================================================

} // namespace picsom

//=============================================================================
//=============================================================================

// Local Variables:
// mode: font-lock
// End:
