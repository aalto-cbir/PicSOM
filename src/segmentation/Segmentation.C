// -*- C++ -*-  $Id: Segmentation.C,v 1.144 2014/10/29 14:08:22 jorma Exp $
// 
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <Segmentation.h>

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif // HAVE_WINSOCK2_H

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif //HAVE_PWD_H

#include <Feature.h>
#include <cctype>

using std::string;
using std::vector;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::cerr;

namespace picsom {
  static const string vcid =
    "@(#)$Id: Segmentation.C,v 1.144 2014/10/29 14:08:22 jorma Exp $";

  Segmentation *Segmentation::list_of_methods;

  int Segmentation::verbose = 0;

  // string Segmentation::previous_methods;

  bool Segmentation::debug_memory_usage = false;

  bool Segmentation::tag_background_labels = true;
  
  //string SegmentInterface::description_x; 
  
  string Segmentation::picsomroot;

  ///////////////////////////////////////////////////////////////////////////
  
  const string& Segmentation::PicSOMroot() {
    if (picsomroot=="")
      picsomroot = "/share/imagedb/picsom";
    return picsomroot;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void addToEscapedString(string &str, char *a){
    string p = processArg(a);
    if(!str.empty() && !p.empty()) 
      str += " ";
    str += p;
  }

  ///////////////////////////////////////////////////////////////////////////

  string collectEscapedString(int argc, char **argv){
    string ret;
    int i;
    for (i=0; i<argc; i++)
      addToEscapedString(ret, argv[i]);

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  string processArg(const char *str) {
    int src;
    string ret;
    
    if (str[0]==0)
      return string("\\ ");
    
    for (src=0; str[src]; src++){
      if (isspace(str[src]) || str[src]=='"' 
	  || str[src]== '\\' || str[src]=='\'' ){
	ret += '\\';
      }
      ret += str[src];
    }

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  Segmentation::Segmentation() {
    Initialize();
  }

  ///////////////////////////////////////////////////////////////////////////

  Segmentation::Segmentation(bool) {
    Initialize();
    next_method = list_of_methods;
    list_of_methods = this;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  Segmentation::~Segmentation() {
    if (next_method && next_method->previous_method)
      delete next_method;
  }

  ///////////////////////////////////////////////////////////////////////////

  void Segmentation::MainShowUsage(const string& n) {
    string sn = segmentfile::StandardNaming("");
    cerr 
      << endl
      << "USAGE: "<<n<< " [switches] <method> [options] <images> ..." <<endl
      << "       "<<n<< " -l            Lists all methods."         	 <<endl
      << "       "<<n<< " -f            Lists available image features."<<endl
      << "       "<<n<< " -h <method>   Describes a method."        	 <<endl
      << "       "<<n<< " -h            Describes all methods."     	 <<endl
      << "       "<<n<< " -v <method>   Shows version of a method." 	 <<endl
      << "       "<<n<< " -v            Shows all method versions." 	 <<endl
      << "       "<<n<< " -x            Views the content of an XML file."<<endl
      << endl
      << "SWITCHES:" << endl		  
      << " -d | -d[d...] | -d<int>  Verbose."                         << endl
      << " -m | -m[m...] | -m<int>  Verbose in imagefile class."      << endl
      << " -dm                      Show memory usage between frames." << endl
      << " -o <filename>|<pattern>  Specifies output filename(s)."    << endl
      << " -O                       Standard output naming."          << endl
      << " -Ob                      Standard output naming with dir." << endl
      << " -OX                      Inhibit .seg output."             << endl
      << " -nb                      No tagging label zero as bkgnd. " << endl 
      << " -i <filename>|<pattern>  Specifies input filename."        << endl
      << " -I  <method descr.>      Standard input name."             << endl
      << " -Ib <method descr.>      Standard input name with dir."    << endl
      << " -s<N>                    Framestep of N frames, default=1" << endl
      << endl
      << "EXAMPLE PATTERNS:" << endl
      << "    %i.seg  -- extension changed to seg (default)."      << endl
      << "    " << sn << "  -- dir/method-options:name (-O)."      << endl
      << "    " << "segments/<method specifier>:%i.seg --"
      << "   (-I <methodname>)" << endl
      << endl
      << "CHAINING METHODS:" << endl
      << "  " << n << " [switches] <method_1> [options_1] + <method_2>"
      << " [options_2] + <method_3> [options_3] <images> ..."      << endl
      << endl;
  }

  ///////////////////////////////////////////////////////////////////////////

  int Segmentation::Main(const vector<string>& a) {
    char **aptr = new char*[a.size()+1];
    for (size_t i=0; i<a.size(); i++)
      aptr[i] = strdup(a[i].c_str());
    aptr[a.size()] = NULL;

    int r = Main(a.size(), aptr);

    for (size_t i=0; i<a.size(); i++)
      delete aptr[i];
    delete aptr;

    return r;
  }

  ///////////////////////////////////////////////////////////////////////////

  int Segmentation::Main(int argc, char **argv) {
    const char *n = argv[0];
    const char *method = NULL;

    int methodcount=0;
    try {
      if (argc==2 && !strcmp(argv[1], "-l")) {
	ListMethods();
	return 0;
      }

      if (argc==2 && !strcmp(argv[1], "-f")) {
	Feature::PrintFeatureList();
	return 0;
      }

      if ((argc==2 || argc==3) &&
	  (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-v"))) {
      
	if (!strcmp(argv[1], "-h") && ShowUsage(method = argv[2]))
	  return 0;
      
	if (!strcmp(argv[1], "-v")) {
	  if (argc==2)
	    cout << vcid << endl;
	  if (ShowVersion(method = argv[2]))
	    return 0;
	}

	if (method)
	  cerr << n << ": method \"" << method << "\" is unknown."
	       << endl << endl;
	argc = 0;
      }
    
      if (argc>1 && !strcmp(argv[1], "-x")) {
	string trick = string(argv[0])+" "+string(argv[1]);
	char **argvx = new char*[argc-1];
	argvx[0] = (char*)trick.c_str();
	for (int ai=1; ai<argc-1; ai++)
	  argvx[ai] = argv[ai+1];
	int ret = ViewXML(argc-1, argvx);
	delete argvx;
	return ret;
      }

      if (argc<3) {
	MainShowUsage(n);
	return 1;
      }

      //mp_set_numthreads(1);
      //mp_numthreads();
    
      string switches("");
      ProcessInfoContainer pic;
      FileInfoContainer fic;

      bool setinputpattern=false;
      string input_pattern;
      bool setstandardinput=false;
      bool setstandardinputwithdir=false;
      bool setstandardinputwithdirnod=false;

      bool setoutputpattern=false;
      string output_pattern;
      bool setstandardoutput=false;
      bool setstandardoutputwithdir=false;
      bool setstandardoutputwithdirnod=false;

      bool setnulloutput = false;

      Option option;
    
      pic.commandline=collectEscapedString(argc,argv);

      int framestep=1;

      while (*argv[1]=='-' && argc>3) {
	string optstr = argv[1];
	option.reset();
	switch (argv[1][1]) {
	case 'o':
	  if (argc<5)
	    throw Switcherror(method);

	  option.option="-o";
	  option.addArgument(argv[2]);
	
	  //      addToEscapedString(switches,argv[2]);
	
	  /// SetOutputPattern(argv[2]);
	  setoutputpattern=true;
	  output_pattern=argv[2];

	  argc--;
	  argv++;
	  break;
	
	case 'O':
	  if (optstr=="-O") {
	    option.option     = "-O";
	    setstandardoutput = true;

	  } else if (optstr=="-Ob") {
	    option.option     = "-Ob";
	    setstandardoutputwithdir = true;

	  } else if (optstr=="-OB") {
	    option.option     = "-OB";
	    setstandardoutputwithdirnod = true;

	  } else if (optstr=="-OX") {
	    option.option = "-OX";
	    setnulloutput = true;

	  } else
	    goto switch_problem;

	  break;

	case 'i':
	  if (argc<5)
	    throw Switcherror(method);
	  option.option = "-i";
	  option.addArgument(argv[2]);
	
	  setinputpattern=true;
	  input_pattern=argv[2];

	  //	SetInputPattern(argv[2]);
	  argc--;
	  argv++;
	  break;

	case 'I':
	  if (argc<5)
	    throw Switcherror(method);
	
	  if (optstr=="-I")
	    setstandardinput = true;

	  else if (optstr=="-Ib")
	    setstandardinputwithdir = true;

	  else if (optstr=="-IB")
	    setstandardinputwithdirnod = true;

	  else
	    goto switch_problem;

	  option.option = optstr;
	  option.addArgument(argv[2]);

	  input_pattern=argv[2];
	
	  if (Verbose()>1)
	    cout << "Using standard input naming with method prefix ["
		 << input_pattern << "]"
		 << (setstandardinputwithdir?" and dir from image":"")
		 << (setstandardinputwithdirnod?" and dir without dots from image":"")
		 << endl;

	  argc--;
	  argv++;
	  break;
	
	case 'd':
	  option.option = argv[1];
	  if (argv[1][2]=='m') {
		DebugMemoryUsage(true);
		break;
	  }

	  {
	    int vn = isdigit(argv[1][2])?atoi(argv[1]+2):strspn(argv[1]+1, "d");
	    AddVerbose(vn);
	  }
	  break;

	case 'n':
	  option.option = argv[1];
	  if(argv[1][2]=='b'){
	    TagBackgroundLabels(false);
	  }


	case 'm':
	  option.option = argv[1];
	  {
	    int vn = isdigit(argv[1][2])?atoi(argv[1]+2):strspn(argv[1]+1, "m");
	    imagefile::debug_impl(vn);
	  }
	  break;

	case 's':
	  //cerr << "Switch -s<N> for setting framestep is YET only partially"
	  //     << " implemented" << endl;
	  {
	    int sn = isdigit(argv[1][2])?atoi(argv[1]+2):1;
	    pic.framestep = framestep = sn;
	    option.option = argv[1];
	  }
	  break;

	default:
	switch_problem:
	  cerr << n << ": switch \"" << argv[1] << "\" is unknown."
	       << endl << endl;
	  throw Switcherror(method);
	}
	argc--;
	argv++;
	if(!option.option.empty())
	  pic.options.list.push_back(option);
      }
    
      const char *verbmsg = "You may want to increase verbosity with -d .";
    
      time_t t = time(NULL);
      if(t==-1) throw string("segmentation.C : couldn't determine time");

      char *timestr = asctime(localtime(&t));
      *strchr(timestr, '\n') = 0;

      char cwd[1000];
      if(!getcwd(cwd,sizeof cwd))
	throw string("segmentation.C: couldn't get cwd");

      passwd *pw = getpwuid(getuid());
      if(!pw) 
	throw string("segmentation.C: couldn't read password entry");
      const char *user = pw->pw_name;

      char hostname[1000];
      if(gethostname(hostname, sizeof hostname))
	throw string("segmentation.C: gethostname() failed");

      pic.user=user;
      pic.host=hostname;
      pic.procdate=timestr;
      pic.cwd=cwd;
  
      Segmentation *first = NULL, *last = NULL;
      segmentfile *image;

      for (;;) {
	if (last) {
	  int nargs = last->ProcessOptions(argc, argv);
	  argc -= nargs;
	  argv += nargs;
	}
	if (!last || !strcmp(*argv, "+")) {
	  Segmentation *segm = CreateMethod(method = argv[1]);
	  methodcount++;
	
	  if (!segm) {
	    cerr << n << ": Creation of segmentation method failed. "
		 << verbmsg << endl;
	    throw Switcherror(method);
	  }
	  if (Verbose()>1)
	    cout << "Created segmentation method " << segm->MethodName(true)
		 << endl;
	
	  if (!first)
	    first = segm;
	
	  if (last)
	    last->NextMethod(segm);
	
	  last = segm;
	
	  argc -= 2;
	  argv += 2;
	
	} else
	  break;
      }
    
      //   cout << "method chain constructed." <<  endl;
    
      while (argc-->0) {
	// first->AddFollowingMethodsToXML(xmldoc,xmlmethodlist);
      
	// cout << "about to set file names for " << *argv << endl;
      
	image = new segmentfile();

	if (setstandardinput) {
	  image->SetStandardInputNaming(input_pattern, "");
	  first->SetPreviousNames(input_pattern);

	} else if (setstandardinputwithdir) {
	  image->SetStandardInputNaming(input_pattern, "b");
	  first->SetPreviousNames(input_pattern);

	} else if (setstandardinputwithdirnod) {
	  image->SetStandardInputNaming(input_pattern, "B");
	  first->SetPreviousNames(input_pattern);

	} else if (setinputpattern)
	  image->SetInputPattern(input_pattern.c_str());
      
	if (setstandardoutput)
	  image->SetStandardOutputNaming(0);
	else if (setstandardoutputwithdir)
	  image->SetStandardOutputNaming(1);
	else if (setstandardoutputwithdirnod)
	  image->SetStandardOutputNaming(2);
	else if (setnulloutput)
	  image->SetNullOutput();
	else if (setoutputpattern)
	  image->SetOutputPattern(output_pattern.c_str());
	else
	  image->SetOutputPattern("%i.seg");

	image->formFileNames(*argv, NULL, first->ChainedMethodName());
	image->setFileInfoProvider(&fic);
	image->setProcessInfoProvider(&pic);
	image->setFirstInChain(first);

	first->setStorage(image);

	if (!first->SetFileNames()) {
	  cerr << n << ": Setting file names" << *argv << " failed. "
	       << verbmsg << endl;
	  return 2;
	}
      
	image->openFiles(*first->GetInputImageName(),
			 *first->GetInputSegmentName());

	// 			      first,&fic,&pic);

	//cout << "image object created" << endl;
	//image->showDataTypes();
      
	char nrstr[30];
	sprintf(nrstr,"%d",image->getWidth());
	fic.imgwidth = nrstr;
	sprintf(nrstr,"%d",image->getHeight());
	fic.imgheight=nrstr;
      
	// there could be an option to give name of contents file as an argument
      
	// cout << "going to open extra files" << endl;
      
	if (!first->OpenExtraFiles()) {
	  cerr << n << ": Opening of extra files for " << *argv << " failed. "
	       << verbmsg << endl;
	  return 2;
	}		    
      
	if (Verbose()>1) {
	  //first->DisplayImageInfo();
	  //first->DisplayOutputInfo();
	}
      
	// cout << "About to start processing" << endl;
	image->frameCacheing(false);

      	if (!first->ProcessAllFrames(framestep)) {
	  cerr << n << ": Segmentation failed. " << verbmsg << endl;
	  return 2;
	}
      
	// cout << "Closing files" << endl;
      
	first->CloseFiles();
	argv++;
      
	// cout << "deleting image object" << endl;
      
	first->setStorage(NULL);
	delete image;
      }
    
      delete first;
    
      return 0;
    }
    catch(const Switcherror &e){
      if (e.method)
	cerr << n << ": method \"" << e.method << "\" is unknown."
	     << endl << endl;
      MainShowUsage(n);
      return 1;
    }
    catch(const string &e){
      cerr << "ERROR: <" << e << ">" << endl;
      return 1;
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  bool Segmentation::ProcessAllFrames(int framestep) {
    // note: should no longer be redefined in
    // inherited classes
    ImagePresentCheck();

    std::set<int> bs;
    bs.insert(0);
      
    bool ret = true;
    if (!ProcessGlobal())
      ret = false;

    for (int f=0; f<image->getNumFrames(); f+=framestep) {
      if (DebugMemoryUsage()){
	cerr << "About to process frame " << f << ", memory usage:" << endl; 
	DumpMemoryUsage(cerr, true);
      }

      image->tagFrameAsProcessed(this,f);

      image->setCurrentFrame(f);
      if(TagBackgroundLabels())	image->setBackgroundLabels(bs);
      if (Verbose()>1 && image->getNumFrames()>1) {
        cout << "Starting to process frame " << f << endl;
	// image->countFrameColours() << " colours." << std::endl;
      }
      if (!Process())
	ret = false;
      
      image->discardImageFrame(f);
     
      if (seek_to_frame!=-1) {
	if (seek_to_frame<f+framestep)
	  cerr << "INVALID seek_to_frame, discarding..." << endl;
	else {
	  if (Verbose()>1)
	    cout << "Seeking to frame " << seek_to_frame << endl;

	  while (f<image->getNumFrames() && f<seek_to_frame)
	    f += framestep;
	  if (f<image->getNumFrames())
	    f -= framestep;
	}

	seek_to_frame = -1;
      }
    }

    LastMethod()->Cleanup();

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  void Segmentation::ListMethods(ostream& os, bool brief) {
    bool first = true;
    int col = 0;
    for (const Segmentation *m = list_of_methods; m; m = m->next_method)
      if (!brief)
	os << m->MethodName(false) << " - " << m->MethodName(true) << endl;
      else {
	string n = m->MethodName(true);
	if (!first)
	  os << ", ";      
	first = false;
	if (col+n.size()>60) {
	  os << endl << "\t";
	  col = 0;
	}
	os << n;       
	col += n.size()+2;
      }
  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool Segmentation::ShowUsage(const char *method, ostream& os) {
    bool needs_endl = false;
    for (const Segmentation *m = list_of_methods; m; m = m->next_method)
      if (!method || !strcmp(method, m->MethodName())) {
	if (needs_endl)
	  os << endl;
	m->UsageInfo(os);
	needs_endl = true;
      }
    
    return needs_endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool Segmentation::ShowVersion(const char *method, ostream& os) {
    
    if (!method)
      os << vcid << endl;
    bool ret = false;
    for (const Segmentation *m = list_of_methods; m; m = m->next_method)
      if (!method || !strcmp(method, m->MethodName())) {
	os << m->Version() << endl;
	ret = true;
      }
    
    return ret;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  Segmentation *Segmentation::CreateMethod(const char *method) {
    for (const Segmentation *m = list_of_methods; m; m = m->next_method)
      if (!strcmp(method, m->MethodName())){
	Segmentation *s=m->Create();
	return s;
      }
    
    return NULL;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  string Segmentation::ChainedMethodName() const {
    string str = MethodName();
    if (!previous_method && !previous_methods.empty())
      str = previous_methods + "+" + str;  
    return next_method ? str+"+"+next_method->ChainedMethodName() : str;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void Segmentation::EchoError(const string& str) {
    cerr << str << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void Segmentation::Initialize() {
    next_method = previous_method = NULL;
    image = NULL;
    chains_produced=false;
    has_segments=false;
    prevent_output=false;
    seek_to_frame = -1;

    //read_all_frames_at_once = false;
    
    method_descriptions.clear();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  MethodInfo Segmentation::getMethodInfo() const{
    MethodInfo ret;
    
    ret.abbreviation=MethodName();
    ret.fullname=MethodName(true);
    ret.description=Description();
    ret.version=Version();
    ret.options=Options();

    return ret;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool Segmentation::ProcessNextMethod() {
    if (!next_method)
      return true;
    
    image->tagFrameAsProcessed(next_method,image->getCurrentFrame());
    return next_method->Process();
  }

  ///////////////////////////////////////////////////////////////////////////

  bool Segmentation::ProcessGlobalNextMethod() {
    if (!next_method)
      return true;
    
    return next_method->ProcessGlobal();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool Segmentation::CleanupPreviousMethod() {
    if (!previous_method)
      return true;
    
    return previous_method->Cleanup();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool Segmentation::SetFileNames(){
    image_name = getImg()->imageName();
    for (Segmentation *g = this; g; g = g->next_method) {
      g->ClearResultFiles();
      g->OpenExtraFiles();
    }
    out_segment_name = getImg()->outSegmentName();
    if (!LastMethod()->PreventOutput())
      AddToResultFiles(out_segment_name);
    
    in_segment_name = getImg()->inSegmentName(); 

    if (verbose) {
      cout << ChainedMethodName() << ": " << image_name << " -> ";
      bool first = true;
      for (const Segmentation *s = this; s; s = s->next_method) {
	const string& res = s->ResultFiles();
	if (!res.length())
	  continue;
	cout << (first?"":", ") << res;
	first = false;
      }
      cout << endl;
    }
    
    return true;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool Segmentation::WriteOutfile() {
    // sdata_needs_write = false;

    //    if (next_method)
    //  return next_method->WriteOutfile();


    if (PreventOutput())
      return true;

    string dir = out_segment_name;
    if (dir[0]!='/') {
      char cwd[1000];
      if (!getcwd(cwd,sizeof cwd))
	throw string("Segmentation::WriteOutfile() : couldn't get cwd");
      dir = string(cwd)+"/"+dir;
    }
    size_t q = 0;
    for (; dir[q] ;) {
      size_t r = dir.find('/', q+1);
      if (r==string::npos)
	break;
      string subdir = dir.substr(0, r);
      struct stat st;
      if (stat(subdir.c_str(), &st)) {
	int mode = 02775;
#if MKDIR_ONE_ARG
	mkdir(subdir.c_str());
#else
	mkdir(subdir.c_str(), mode);
#endif // MKDIR_ONE_ARG
	chmod(subdir.c_str(), mode);
      }
      q = r;
    }
    
    getImg()->writeAllSegmentFrames(out_segment_name.c_str(),
				    IncludeImagesInXML(),true);


    return true;
  }


  ///////////////////////////////////////////////////////////////////////////

  bool Segmentation::CloseFiles() {

    for(Segmentation *g=FirstMethod();g;g=g->next_method)
      if(!g->CloseExtraFiles()) return false;

    return CloseFilesInner();
    
  }

  ///////////////////////////////////////////////////////////////////////////

    bool Segmentation::CloseFilesInner (){

    return WriteOutfile();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void Segmentation::DisplayInfo() {
    cout << "Image file <" << image_name      << ">" << endl
	 << "Input segment file <" << in_segment_name << ">" << endl
	 << "Output segment file <" << out_segment_name << ">" << endl
	 << "   width=" << getImg()->getWidth()       << endl 
	 << "  height=" << getImg()->getHeight()      << endl
	 << "   frames=" << getImg()->getNumFrames()    << endl
      //        << "   compr=" << img.getCompression() << endl
      ;
  }



  ///////////////////////////////////////////////////////////////////////////

  void Segmentation::AnnounceChains()
    {
      chains_produced=true;
      
      string *ptr=(GetChains()->OutputXML());
      
      SegmentationResult result;
      result.name="chaincodes";
      result.type="XML";
      result.value=*ptr;
      
      getImg()->writeFrameResultToXML(this,result);
      
      delete ptr;
    }

 
 
  
  ///////////////////////////////////////////////////////////////////////////
  
 //  bool LinePixelsToList(coord p1,
// 			coord p2, 
// 			std::vector<coord_float> &list)
//     {
      
//       if(p1.x==p2.x && p1.y == p2.y){
// 	list.push_back(p1); 
// 	return true;
//       }
      
//       if(abs(p1.x-p2.x)>abs(p1.y-p2.y)){
	
	
// 	int xstep =(p1.x>p2.x)?-1:1;
	
// 	// we step the x coordinate as delta_x > delta_y
	
// 	// if(p1.x>p2.x){ // swap the points
// 	//       coord tmp=p1;
// 	//       p1=p2;
// 	//       p2=tmp;
// 	//     }
	
// 	float step=xstep*((float)(p2.y-p1.y))/(p2.x-p1.x);
	
// 	int x;
// 	float y=p1.y;
	
// 	for(x=p1.x;xstep*x<=xstep*p2.x;x+=xstep){
// 	  list.push_back(coord_float(x,y));
// 	  //        cout << "  pushing " << coord_float(x,y) << endl;
// 	  y += step;
// 	}
	
//       }
      
//       else{
// 	// we step the y coordinate as delta_y >= delta_x
	
// 	int ystep=(p1.y>p2.y)?-1:1;
	
// 	//    if(p1.y>p2.y){ // swap the points
// 	//       coord tmp=p1;
// 	//       p1=p2;
// 	//       p2=tmp;
// 	//     }
	
// 	float step=ystep*((float)(p2.x-p1.x))/(p2.y-p1.y);
	
// 	int y;
// 	float x=p1.x;
	
// 	for(y=p1.y;ystep*y<=ystep*p2.y;y+=ystep){
// 	  list.push_back(coord_float(x,y));
// 	  //        cout << "  pushing " << coord_float(x,y) << endl;
// 	  x += step;
// 	}
//       }
      
//       return true;
//     }
  

///////////////////////////////////////////////////////////////////////////
 bool Segmentation::AddBorders(SubSegment &subs,const coord &seed,bool outeronly)
{

  int delta_x[8]={1,1,0,-1,-1,-1,0,1};
  int delta_y[8]={0,-1,-1,-1,0,1,1,1};
  bool out_border=true;

  std::vector<coord> stack;
  int dir,label;

  subs.seed=seed;

  BoolArraySeg subsegment(this);
  BoolArraySeg mark(this);

  stack.push_back(seed);
  getImg()->get_pixel_segment(seed.x,seed.y,label);

  if(label==0) out_border = false;

  if(Verbose()>1) 
    cout << "Starting marking subsegment with seed " << seed 
	 << ", label=" << label << endl;
  
  while(!stack.empty()){
    coord c=stack.back();
    stack.pop_back();

    if(!getImg()->coordinates_ok(c.x,c.y)){
      if(label==0) subsegment.Set(c); // label 0 denotes background
      continue;
    }

    if(!subsegment.Ask(c)){
      int s;

      getImg()->get_pixel_segment(c.x,c.y,s);
	
      if(s==label){
	subsegment.Set(c);
	for(dir=0;dir<8;dir++)
	  stack.push_back(coord(c.x+delta_x[dir],c.y+delta_y[dir]));
      }
    }
  }

  int x,y;

  for(y=0;y<getImg()->getHeight();y++) for(x=0;x<getImg()->getWidth();x++)  {
    if(!out_border && outeronly) return true;
    if(subsegment.Ask(x,y)){
      for(dir=0;
	  dir<8 &&
	    (getImg()->HasNeighbourSeg(coord(x,y),dir) || mark.Ask(x+delta_x[dir],
							 y+delta_y[dir]))
	    ; dir+=(out_border)?1:2){} // only differing 4-neighbours are considered as border

      if(dir<8) // border pixel
	{ // border pixel



	subs.borders.push_back(PixelChain());
	TraceContour(coord(x,y),subs.borders.back(),out_border);

	// mark this "hole" in subsegment as processed
	// 4-connectivity is used in the "hole"

	stack.push_back(coord(x+delta_x[dir],y+delta_y[dir]));
  
	while(!stack.empty()){
	  coord c=stack.back();
	  stack.pop_back();

	  if(subsegment.Ask(c) || mark.Ask(c)) continue;
	  
	  //	  if(Verbose()>1)
	  //	    cout << "Mark (" << c << ")" << endl;

	  mark.Set(c);
	  if(!getImg()->coordinates_ok(c.x,c.y)){
	    // all border pixels of image have to be checked
	    int u,v;
	    for(v=0;v<getImg()->getHeight();v++){
	      stack.push_back(coord(0,v));
	      stack.push_back(coord(getImg()->getWidth()-1,v));
	    }

	    for(u=0;u<getImg()->getWidth();u++){
	      stack.push_back(coord(u,0));
	      stack.push_back(coord(u,getImg()->getHeight()-1));
	    }
	  } 
	  else{
	    for(dir=0;dir<8;dir+=(out_border)?1:2)
	      {
		stack.push_back(coord(c.x+delta_x[dir],c.y+delta_y[dir]));
	      }
	  }

	}
	out_border=false;
	}
     

    }
  }
  
  return true;
 
}

///////////////////////////////////////////////////////////////////////////

  PixelChain *Segmentation::TraceContour(const coord &seed,PixelChain &chain,bool use_diagonals){
    
    int label;
    int incr = (use_diagonals)?1:2;
    coord extreme=seed;
    
    getImg()->get_pixel_segment(seed.x,seed.y,label);
    
    coord c(seed);
    int delta_x[8]={1,1,0,-1,-1,-1,0,1};
    int delta_y[8]={0,-1,-1,-1,0,1,1,1};
    int dir,start_dir;

    if(Verbose()>1){
      cout << "Starting contour tracing from point " << seed
	   << " using incr=" << incr << endl; 
    }
    
    if( getImg()->NeighbourCountSeg(c,use_diagonals)==0) // isolated point
      {
        if(Verbose()>1)
	  cout << "Isolated point." << endl;
	
	chain.l.push_back(c);
	chain.l.push_back(c);
	
	return &chain;
      }
    
    // determine where we came from
    
    for(dir=0;!getImg()->HasNeighbourSeg(c,(dir+4)&7) || getImg()->HasNeighbourSeg(c,(dir+4+incr)&7);dir-=incr ){}
    
    start_dir = dir&7;
    
    if(Verbose()>1)
      cout << "start_dir = " << start_dir << endl;
    
    do{
      chain.l.push_back(c);
      
      //    if(Verbose()>1)
      // cout << "Pushing " << c << endl;
      
      if(c<extreme){
	extreme=c;
      }
      
      for(dir+=6; !getImg()->HasNeighbourSeg(c,dir);dir+=incr){}
      dir &= 7;
      //        if(Verbose()>1)
      // cout << "Moving to dir " << dir << endl;
      c.x += delta_x[dir];
      c.y += delta_y[dir];
    } while (c != seed || dir != start_dir);   
    
    //  if(Verbose()>1)
    //    cout << "Re-arranging list." << endl;
    
    while( chain.l.front() != extreme)
      {
	chain.l.push_back(chain.l.front());
	chain.l.pop_front();
      }
    
    chain.l.push_back(chain.l.front());
    
    //  if(Verbose()>1)
    //   cout << "Contour trace finished." << endl;
    
    return &chain;
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  bool Segmentation::ExtractChains()
  {
    if(!AddSegments(chains)) return false;
    AnnounceChains();
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  bool Segmentation::AddSegments(Chains &ch){
    
    int x,y;
    int s;
    size_t i;

    CorrespArray corresp;
    
    int *unique = getImg()->GetSeparateLabelingInt(&corresp);
    
    bool *seen=new bool[corresp.v.size()];
    
    for(i=0;i<corresp.v.size();i++) seen[i]=false;
    
    i=0;
    for(y=0;y<getImg()->getHeight();y++) for(x=0;x<getImg()->getWidth();x++){
      unique[i]=corresp.SmallestCorresponding(unique[i]);
      i++;
    }
    
    // cout << "corresp.size ="<< corresp.size() << endl << "corresp:" << endl;
    //   for(i=0;i<corresp.size();i++)
    //     cout << corresp.SmallestCorresponding(i) << endl;
    
    i=0;
    
    for(y=0;y<getImg()->getHeight();y++) for(x=0;x<getImg()->getWidth();x++){
      
      if(!seen[unique[i]]){
	if(Verbose()>1){
	  cout << "New unique area found in (" << x <<","<<y<<"): " 
	       << unique[i] << endl;
	}
	seen[unique[i]]=true;
	getImg()->get_pixel_segment(x,y,s);
	
	if(Verbose()>1)
	  cout << "segment label is "<< s <<endl;
	
	// search for the segment s
	
	std::list<Segment>::iterator it;
	
	for(it=ch.segments.begin();(*it).label != s && it !=ch.segments.end();it++)
	  //	cout << "label of current list entry : " << (*it).label << endl;
	  
	  if(Verbose()>1)
	    cout << "Iterator after list traversal : " << *it << endl;
	
	if(it==ch.segments.end()){
	  if(Verbose()>1) cout << "Found new label " << s << endl;
	  for(it=ch.segments.begin();(*it).label < s && it != ch.segments.end();it++){}
	  
	  //it=
	  //ch.segments.insert(it,seg);
	  
	  ch.segments.push_back(Segment());
	  it=ch.segments.end();
	  it--;
	  
	  
	  (*it).label=s;
	  
	}
	// cout << "The new segment: " << (*it) << endl; 
	(*it).subsegments.push_back(SubSegment());
	AddBorders((*it).subsegments.back(),coord(x,y));
      }
      
      i++;
    }
    
    delete[] seen;
    delete[] unique;
    
    return true;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void Segmentation::_set_image_ptr(segmentfile *s) {
    image = s;

    if (next_method)
      next_method->_set_image_ptr(s);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool xy_ok(const pair<int,int>& xy, int w, int h) {
    int x = xy.first, y = xy.second;
    return x>=0 && y>=0 && x<w && y<h;
  }

  //---------------------------------------------------------------------------

  void process_one(const string& argv1, bool debug, bool hist, bool xml,
		   const pair<int,int>& showxy,
		   const pair<int,int>& showbox1,
		   const pair<int,int>& showbox2,
		   bool pixeldump, int segmentselect, bool resultdump,
		   bool noframes) {

    if (debug)
      cout << "about to create segmentfile <" << argv1 << ">" << endl;

    try {
      segmentfile sf("", argv1);
      if (debug)
	cout << "segmentfile constructed" << endl;
      if (debug)
	cout << argv1 << " : " << sf.getNumFrames()
	     << " segmentation image frames" << endl;

      if (hist) {

	map<int,int> accu;
      
	for (int f=0; f<sf.getNumFrames(); f++){
	  cout << "frame "<<f<<endl;
	  sf.setCurrentFrame(f);
	  accu.clear();
	  for (int y=0; y<sf.getHeight(); y++)
	    for(int x=0; x<sf.getWidth(); x++) {
	      int v;
	      sf.get_pixel_segment(x, y, v);
	      accu[v]++;
	    }
	  map<int,int>::iterator it;
	
	  for (it=accu.begin(); it!=accu.end(); it++)
	    cout << "count["<<it->first<<"]: " << it->second << endl;

	  sf.discardImageFrame(f);
	  sf.discardSegmentFrame(f);

	}
      }
    
      if (debug)
	cout << "method name: " << sf.collectChainedName() << endl;
    
      if (xml) {
	char *buffer = sf.getXMLDescription();
	cout << buffer << flush;
	free(buffer);
      }

      for (int f=0; !noframes && f<sf.getNumFrames(); f++) {
	sf.setCurrentFrame(f);
	int dx = (int)ceil(sf.getWidth()/79.0);

	bool show_ok = xy_ok(showxy, sf.getWidth(), sf.getHeight());
	int show_x = -2, show_y = -2;
	if (show_ok) {
	  show_x = (int)floor((showxy.first +dx/2)/float(dx));
	  show_y = (int)floor((showxy.second+dx/2)/float(dx));
	}

	bool box_ok = xy_ok(showbox1, sf.getWidth(), sf.getHeight()) &&
	  xy_ok(showbox2, sf.getWidth(), sf.getHeight());
	int box_x1 = -2, box_x2 = -2, box_y1 = -2, box_y2 = -2;
	if (box_ok) {
	  box_x1 = (int)floor((showbox1.first +dx/2)/float(dx));
	  box_y1 = (int)floor((showbox1.second+dx/2)/float(dx));
	  box_x2 = (int)floor((showbox2.first +dx/2)/float(dx));
	  box_y2 = (int)ceil ((showbox2.second+dx/2)/float(dx));
	}

	if (pixeldump) {
	  cout << "--------------------------------------------------------"
	       << endl << "pixel dump of " << sf.getWidth() << "x"
	       << sf.getHeight() << " sized frame " << f << "/"
	       <<	sf.getNumFrames() << " in "
	       << dx << "x" << dx << " blocks";
	  if (show_ok)
	    cout << " (showing pixel " << showxy.first << "," << showxy.second
		 << " -> " << show_x << "," << show_y << ")";
	  if (box_ok)
	    cout << " (showing box " << showbox1.first << "," << showbox1.second
		 << "," << showbox2.first << "," << showbox2.second
		 << " -> " << box_x1 << "," << box_y1
		 << "," << box_x2 << "," << box_y2 << ")";
	  cout << " :" << endl;

	  for (int y=0; y<sf.getHeight(); y+=dx) {
	    for (int x=(show_ok||box_ok)?-dx:0; x<sf.getWidth(); x+=dx) {
	      if (x>=0) {
		char ch = '.';
		int s;
		sf.get_pixel_segment(x, y, s);
		if (segmentselect==-1 || s==segmentselect) {
		  if (s<0)
		    ch = '-';
		  else if (s<=9)
		    ch = '0'+s;
		  else if (s-10<='Z'-'A')
		    ch = 'A'+s-10;		
		}
		cout << ch;
	      }

	      char delim = ' ';

	      if (box_ok && y>=box_y1*dx && y<box_y2*dx &&
		  (x==(box_x1-1)*dx || x==box_x2*dx))
		delim = x==box_x2*dx ? ']' : '[';

	      if (show_ok && y==show_y*dx && (x==(show_x-1)*dx || x==show_x*dx))
		delim = x==show_x*dx ? '<' : '>';

	      cout << delim;
	    }
	    cout << endl;
	  }    
	}

	if (show_ok) {
	  int s;
	  sf.get_pixel_segment(showxy.first, showxy.second, s);

	  cout << "frame " << f << " pixel " << showxy.first
	       << "," << showxy.second << " is in segment " << s << endl;
	}
	sf.discardImageFrame(f);
	sf.discardSegmentFrame(f);
      }

      if (resultdump) {
	cout << "FILE <" << argv1 << "> RESULTS:" << endl;
	SegmentationResultList *results = sf.readFileResultsFromXML("","");
		
	SegmentationResultList::const_iterator it;
	for(it=results->begin();it!=results->end();it++){
	  cout << "  NAME:  "<<it->name<<endl;
	  cout << "  TYPE:  "<<it->type<<endl;
	  cout << "  VALUE: "<<it->value<<endl;
	  cout << "  METHODID:"<<it->methodid<<endl;
	  cout << "  RESULTID:"<<it->resultid<<endl<<endl;
	}

	set<int> flist=sf.listProcessedFrames();

	for (set<int>::const_iterator it=flist.begin();
	     !noframes &&it!=flist.end(); it++){
	  int f=*it;
	  cout << "FRAME [" << f << "] RESULTS:" << endl;
	  results = sf.readFrameResultsFromXML(f,"","");
		
	  for(SegmentationResultList::const_iterator it=results->begin();
	      it!=results->end();it++){
	    cout << "  NAME:  "<<it->name<<endl;
	    cout << "  TYPE:  "<<it->type<<endl;
	    cout << "  VALUE: "<<it->value<<endl;
	    cout << "  METHODID:"<<it->methodid<<endl;
	    cout << "  RESULTID:"<<it->resultid<<endl<<endl;
	  }

	  if (results->empty())
	    cout << endl;
	} 
      }
    }
    
    catch (const char *txt) {
      cerr << "ERROR: " << txt << endl;
    }
    catch (const string& str) {
      cerr << "string-ERROR: " << str << endl;
    }
    catch (const std::exception& e) {
      cerr << "std-ERROR: " << e.what() << endl;
    }
  }

  //---------------------------------------------------------------------------

  int Segmentation::ViewXML(int argc, char **argv) {
    const char *n = argv[0];

    bool xml = true;
    bool hist = false;
    bool pixeldump = false;
    bool debug = false;
    bool resultdump=false;
    bool noframes=false;
    int segmentselect = -1;
    pair<int,int> showxy(-1, -1), showbox1(-1, -1), showbox2(-1, -1);

    while (argc>1 && argv[1][0]=='-') {
      string a = argv[1]+1;
      if (a=="noxml")
	xml = false;

      else if (a=="hist")
	hist = true;

      else if (a.substr(0, 9)=="pixeldump") {
	pixeldump = true;
	if (a[9]=='=')
	  segmentselect = atoi(a.substr(10).c_str());
      }

      else if (a.find("showxy=")==0 &&
	       sscanf(a.substr(7).c_str(), "%d,%d",
		      &showxy.first, &showxy.second)==2) {
	// nothing really !
      }

      else if (a.find("showbox=")==0 &&
	       sscanf(a.substr(8).c_str(), "%d,%d,%d,%d",
		      &showbox1.first, &showbox1.second,
		      &showbox2.first, &showbox2.second)==4) {
	// nothing really !
      }

      else if (a=="resultdump")
	resultdump=true;

      else if (a=="noframes")
	noframes=true;

      else if (a=="debug")
	debug = true;

      else if (a=="help" || a=="h")
	goto usage;

      else if (a=="version" || a=="v") {
	cout << vcid << endl;
	return 0;
      }

      else {
	cerr << n << " : unknow switch [" << argv[1] << "]" << endl;
	goto usage;
      }

      argc--;
      argv++;
    }

    if (argc<2) {
    usage:
      cerr << "USAGE: " << n << " [-help | -h] [-version | -v] [-debug]"
	   << " [-noxml] [-noframes] [-hist] [-pixeldump[=S]] [-showxy=x,y]"
	   << " [-showbox=x1,y1,x2,y2] [-resultdump]"
	   << " <segmentation files ... >"
	   << endl;
      return 1;
    }

    while (argc>=2) {
      process_one(argv[1], debug, hist, xml, showxy, showbox1, showbox2,
		  pixeldump, segmentselect, resultdump, noframes);
      argc--;
      argv++;
    }

    return 0;
  }

  ///////////////////////////////////////////////////////////////////////////
  
//   void PrintHashEntry(const Segmentation::HashEntry & entry) {
//     cout << entry.first << ":" << endl <<entry.second << endl; 
//   }
  
  ///////////////////////////////////////////////////////////////////////////
  // ScaleImage - Scales down image
  // Scales selected area ( (tlx,tly)->(brx,bry) from image (*src) 
  // to create a new image w x h (this).
  //
  // @param *src:const Segmentation   source data (image before scaling)
  // @param tlx: int            top left x-coordinate of area to be scaled
  // @param tly: int            top left y-coordinate of area to be scaled
  // @param brx: int            bottom right x-coordinate of area to be scaled
  // @param bry: int            bottom right y-coordinate of area to be scaled
  // @param w:   int            Width of becoming (scaled) image
  // @param h:   int            Height of becoming (scaled) image
  // @return  bool              ok or not
  //

 
  ///////////////////////////////////////////////////////////////////////////

  bool Segmentation::ScaleImage(const Segmentation  * /* src */,
				int /* tlx */, int /* tly */, 
				int /* brx */, int /* bry */,
				int /* w*/, int /*h*/) {

    throw "scaleImage no longer in class Segmentation. cf. class imagedata";
  }

//   int DEBUG2 = 0; // for second -level " "
//   int DEBUG3 = 0; // for third -level " " (really slow !!)

//   // Sets this to have w x h dimensions
//   //SetImageSize(w, h);

//   string errhead = "Segmentation::img_pixel_data::coopyTile : ";

//   if(src->type != pixeldata_float3)
//     throw errhead + " implemented only for float3 images";

//   // deallocate old arrays

//   delete df; delete di;
//   di = NULL;  

//   // allocate new storage 
//   df = new float[3*w*h];
//   type = src->type;
//   width=w; height=h;


//   // DoubleChecks that given limit does not exceed actual image dimensions
//   if (tlx < 0) {
//     if(Verbose()) 
//       cout << "ScaleImage: tlx -coord below zero! (tlx): "<<tlx<<endl;
//     tlx = 0;
//   }
//   if (tly < 0) {
//     if(Verbose()) 
//       cout << "ScaleImage: tly -coord below zero! (tly):"<<tly<<endl;
//     tly = 0;
//   }
//   if (brx > src->getWidth()-1) {
//     if(Verbose()) 
//       cout << "ScaleImage: brx -coord out of picture! (brx):"<<brx<<endl;
//     brx = src->getWidth()-1;
//  }
//   if (bry > src->getHeight()-1) {
//     if(Verbose()) 
//       cout << "ScaleImage: bry -coord out of picture! (bry):"<<bry<<endl;
//     bry = src->getHeight()-1;
//   }

//   // Dimensions of the selected area in original image
//   const int old_h = (bry - tly) + 1;
//   const int old_w = (brx - tlx) + 1;

//   // Scaling factors in x and y directions
//   const float ScaleX = (float)old_w / (float)w;
//   const float ScaleY = (float)old_h / (float)h;

//   // IF NEW IMAGE DIMENSIONS ARE GREATER THAN ORIGINAL IMAGE'S
//   // SELECTED AREA THEN QUIT()
//   if ( old_h < h || old_w < w ) {
//     throw errhead + "New image dimensions are greater than original image's selected area!";
//   }

//   // Variables used for calculation of single pixel in the
//   // new scaled image
//   float s; // How many pixels left to complete single pixel scaling
//   int   i; // Temp pixel counter, how many pixels have been processed
//   int   x = 0,
//         y = 0,
//         Y = 0;  // new + old img col,row positions
//   bool ok; // temporary status for all ok?

//   // Weights for partial pixels at beginning and end of calculated area
//   float   begdiff, enddiff;
//   float   mean_r, mean_g, mean_b;   // For temporary mean calculations
//   RGBTriple   temp;                    // temp storage for rgb values

//   float *rarr = new float[old_h*w];	// array for width-scaled image (r)
//   float *garr = new float[old_h*w];	// array for width-scaled image (g)
//   float *barr = new float[old_h*w];	// array for width-scaled image (b)

// // X,Y == original image index reference
// // x,y == scaled image index (0..w & 0..h)

// // Scaling image in x-coordinate direction
// // thus we shrink the width of image
// // and the height remains the same
// // We'll do this once for every selected pixel in orig img
//   for(Y = tly; Y <= bry; Y++) {
//     for( x = 0; x < w; x++) {
//       if(DEBUG3) cout << "x,Y: "<<x<<","<<Y<<endl;
//       // reset mean and s
//       mean_r = 0;      	mean_g = 0;      	mean_b = 0;
//       s = ScaleX;

//       // counter; how many pixels forward have we moved from left edge
//       i = 0;

//       // the relative left part to be shrank into single pixel
//       // i.e. the weight of the rgb value to be used calculating mean
//       begdiff = (float)ceil( (float)x * ScaleX ) - (float)x * ScaleX;
//       if(DEBUG2) cout << "bg: "<<begdiff<<"/"<<"s: "<<s<<"/";

//       // if the relative left part is not complete pixel
//       if( begdiff != (float)0 ) {
//         if( ( floor( (float)x*ScaleX )+ tlx) > brx || Y>bry ) {
//         cout<<"Scaleimage Error1: floor(x*ScaleX)+tlx>brx"<<
// 	"x,ScaleX,tlx,brx"<<x<<" "<<ScaleX<<" "<<tlx<<" "<<brx<<endl;
//         abort();
//         }
//         ok = src->get_float3( floor( (float)x*ScaleX ) + tlx, Y,temp);
//         if(!ok) {
//           cout<<"Error! Exit:phase1"<<"(x,Y):"<<x<<","<<Y<<
//           " tlx: "<<tlx<<" (i,s):"<<i<<","<<s<<endl;
//           abort();
//         }

//         // increasing the mean with the weighted value of partial pixels
//         mean_r += begdiff * temp.r;
//         mean_g += begdiff * temp.g;
//         mean_b += begdiff * temp.b;
//         // decreasing already calculated part from total number of calculated pixels
//         s = s - begdiff;
//         if(DEBUG2) cout<<" mr:"<<mean_r<<" mg: "<<mean_g<<" mb: "<<mean_b<<endl;
//       }
//       // if relative left part is really a complete pixel
//       // (begdiff==0) then do nothing since this is taken care inside next while

//       // the center area (the whole pixels in original image) to be shrank into single pixel
//       while( s >= (float)1 ) {
//         if( (ceil( (float)x*ScaleX)+i+tlx)>brx || Y>bry) {
//           cout<<"Scaleimage Error2: ceil(x*ScaleX)+i+tlx>brx"<<
//           "x,ScaleX,i,tlx,brx"<<x<<" "<<ScaleX<<" "<<i<<" "<<tlx<<" "<<brx<<endl;
//           abort();
//         }
//         ok = src->get_float3(ceil(x*ScaleX) +i +tlx , Y,temp);
//         if(!ok) {
//           if(Verbose()) 
//             cout<<"Error! Exit:phase2"<<"(x,Y):"<<x<<","<<Y<<" (i,tlx) "<<i<<","<<tlx<<endl;
//           abort();
//         }

//         // increasing the mean with the value of complete pixel
//         mean_r += temp.r;
//         mean_g += temp.g;
//         mean_b += temp.b;
//         i++;
//         if(DEBUG2) cout<<" mr:"<<mean_r<<" mg: "<<mean_g<<" mb: "<<mean_b<<endl;

//         // decreasing counter to know how we will end up to end of area
//         // where there is no more complete pixel left necessary
//         s = s - (float)1; 
//      }

//       // the relative part of the right side of area to be shrank into single pixel
//       // i.e. the weight of the rgb value to be used calculating mean
//       //enddiff = (float)(x + 1)*ScaleX - (float)floor( (float)(x+1)*ScaleX );
//       enddiff = s;
//       if(DEBUG2) cout <<"edif:"<<enddiff<<endl;

//       if( enddiff != (float)0 ) {
//         if( (floor( (float)(x+1)*ScaleX )+tlx)>brx ||Y>bry) {
//           //cout<<"Scaleimage Error3: floor((x+1)*ScaleX)+tlx>brx"<<
//           //"x,ScaleX,tlx,brx"<<x<<" "<<ScaleX<<" "<<tlx<<" "<<brx<<endl;
//           //abort();
//           ok = src->get_float3(floor( (float)(x+1)*ScaleX )+tlx-1, Y,temp) ;
//         }
//         else 
//           ok = src->get_float3(floor( (float)(x+1)*ScaleX )+tlx, Y,temp) ;
//         if(!ok) {
//           if(Verbose())
//             cout<<"ScaleImage Error! Exit:phase3"<<"(x,Y):"<<x<<","<<Y<<" i,tlx"<<i<<" "<<tlx<<endl;
//           abort();
//         }
//         // increasing the mean with the weighted value of partial pixels
//         mean_r += enddiff * temp.r;
//         mean_g += enddiff * temp.g;
//         mean_b += enddiff * temp.b;
//         if(DEBUG2) cout<<" mr:"<<mean_r<<" mg: "<<mean_g<<" mb: "<<mean_b<<endl;
//       }
//       // if relative right part is really a complete pixel
//       // (enddiff==0) then do nothing since this is taken care inside while

//       // divide mean with scaling factor
//       mean_r = mean_r / ScaleX;
//       mean_g = mean_g / ScaleX;
//       mean_b = mean_b / ScaleX;

//       // .. and we have value for new pixel !
//       if( ((Y-tly)*w+x)>old_h*w ) {
//         cout<<"ScaleImage Error4: ArrayOutOfBoundary!"<<endl;
//         cout<<"(Y-tly)*w+x > old_h*w:"<<Y<<"-"<<tly<<"*"<<w<<"+"<<x
//         <<">"<<old_h<<"*"<<w<<endl;
//         abort();
//       }
//       rarr[(Y-tly)*w+x] = (float)(int)mean_r;
//       garr[(Y-tly)*w+x] = (float)(int)mean_g;
//       barr[(Y-tly)*w+x] = (float)(int)mean_b;
//     }
//   }

//   for(x=0;x<w;x++){
//     for(y=0;y<h;y++){
//       if(DEBUG3) cout << "y,x: "<<y<<" , "<<x<<endl;
//         mean_r = 0;   	mean_g = 0;   	mean_b = 0;
//         s = ScaleY;

//         // counter; how many pixels forward have we moved from the top
//         i = 0;

//        // the relative top part to be shrank into single pixel
//        // i.e. the weight of the rgb value to be used calculating mean
//        begdiff = (float)ceil( (float)y*ScaleY ) - (float)y*ScaleY;
//        if(DEBUG2) cout<<w<<"w "<<h<<"h "<<endl;

//        if( begdiff != (float)0 ) {
//          if(( (floor( (float)y*ScaleY) )*w+x)>old_h*w) {
//            cout<<"Scaleimage Error5: floor(y*ScaleY)*w+x>old_h*w"<<
//            "y,ScaleY,w,x,old_h,w"<<y<<" "<<ScaleY<<" "<<w<<" "<<x<<" "
//            <<old_h<<" "<<w<<endl;
//            abort();
//          }
//          mean_r += begdiff * rarr[(int) floor( (float)y*ScaleY)*w+x];
//          mean_g += begdiff * garr[(int) floor( (float)y*ScaleY)*w+x];
//          mean_b += begdiff * barr[(int) floor( (float)y*ScaleY)*w+x];
//          // decreasing already calculated part from total number of calculated pixels
//          s = s - begdiff;
//         if(DEBUG2) cout<<" mr:"<<mean_r<<" mg: "<<mean_g<<" mb: "<<mean_b<<endl;
//       }
//       // if relative top part is really a complete pixel
//       // (begdiff==0) then do nothing since this is taken care inside next while

//       // the center area (the whole pixels in original image) to be shrank into single pixel
//       while( s >= (float)1 ) {
//         // increasing the mean with the value of complete pixel
//         if( ((int) (ceil((float)y*ScaleY ) +i)*w +x)>old_h*w) {
//           cout<<"ScaleImage Error6: ArrayOutOfBoundary!"<<endl;
//           cout<<"(int) (ceil((float)y*ScaleY ) +i)*w +x)>old_h*w"<<
//           y<<"*"<<ScaleY<<"+"<<i<<"*"<<w<<"+"<<x<<">"<<old_h<<"*"<<w<<endl;
//           abort();
//         }
//         mean_r += rarr[(int) (ceil( (float)y*ScaleY ) +i)*w +x];
//         mean_g += garr[(int) (ceil( (float)y*ScaleY ) +i)*w +x];
//         mean_b += barr[(int) (ceil( (float)y*ScaleY ) +i)*w +x];
//         i++;
//         // decreasing counter to know how we will end up to end of area
//         // where there is no more complete pixel left necessary
//         s = s - (float)1;
//       }

//       // the relative part of the bottom side of area to be shrank into single pixel
//       // i.e. the weight of the rgb value to be used calculating mean
//       //enddiff = (float)(y+1)*ScaleY - (float)floor( (float)(y+1)*ScaleY );
//       enddiff=s;
//       if( enddiff != (float)0 ) {
//         if( ((int)(floor( ScaleY*(float)(y+1)) )*w+x)>old_h*w) {
//           // in case of the last line
//           //cout<<"ScaleImage Error7: ArrayOutOfBoundary!"<<endl;
//           //cout<<"(int) (ceil((float)(y+1)*ScaleY )*w +x)>old_h*w"<<
//           //y<<"*"<<ScaleY<<"*"<<w<<"+"<<x<<">"<<old_h<<"*"<<w<<endl;
//           mean_r += enddiff * rarr[(int)(floor( ScaleY*(float)(y)) )*w+x];
//           mean_g += enddiff * garr[(int)(floor( ScaleY*(float)(y)) )*w+x];
//           mean_b += enddiff * barr[(int)(floor( ScaleY*(float)(y)) )*w+x];
//           //abort();
//         }
//         else {
//           mean_r += enddiff * rarr[(int)(floor( ScaleY*(float)(y+1)) )*w+x];
//           mean_g += enddiff * garr[(int)(floor( ScaleY*(float)(y+1)) )*w+x];
//           mean_b += enddiff * barr[(int)(floor( ScaleY*(float)(y+1)) )*w+x];
//         }
//         if(DEBUG2) cout<<" mr:"<<mean_r<<" mg: "<<mean_g<<" mb: "<<mean_b<<endl;
//       }
//       // if relative right part is really a complete pixel
//       // (enddiff==0) then do nothing since this is taken care inside while

//       // divide mean with scaling factor
//       mean_r = mean_r / ScaleY;
//       mean_g = mean_g / ScaleY;
//       mean_b = mean_b / ScaleY;

//       // .. and we have value for new pixel !
//       if(x>=w || y>=h) {
//         cout<<"Error dim8"<<endl;
//         abort();
//       }
//       temp.r = mean_r; // 2002-06-19 vvi
//       temp.g = mean_g;
//       temp.b = mean_b;

//       ok = set_float3(x, y, temp);
//       if (!ok) {
//         cout<<"Error! Exit:phase4"<<"(x,y):"<<x<<","<<y<<" i,tlx"<<i<<" "<<tlx<<" s: "<<s<<endl;
//         abort();
//       }
//     }
//   }

// /*
//   // UNCOMMENT THIS if you want ScaleImage to print its output to file
//   // named "ScaleImage.png"

//   // Saving Scaled image to file
//   imgdata result;
//   result.createFile("ScaleImage.png",Width(),Height(),3);
//   if (CopyToImageObject(&result)) {
//     cout<<"Error! Exit:phase5"<<" Width,Height: "<<Width()<<" "<<Height()<<endl;
//     abort();
//   }
//   result.closeFile();
//   // end of file saving
// */


//   delete[] rarr;
//   delete[] garr;
//   delete[] barr;
//   if(Verbose()) cout << "ScaleImage finished OK"<<endl;

//   return ok;
// }
  // ///////////////////////////////////////////////////////////////////////////
  
  // const char *SegmentInterface::Version() const { 
  //   return vcid;
  // }
  
  // ///////////////////////////////////////////////////////////////////////////
  
// void SegmentInterface::UsageInfo(ostream& os) const { 
//   os << "SegmentInterface should be used for reading " << endl;
//   os << "existing segmentation files only. " << endl;
// }

// ///////////////////////////////////////////////////////////////////////////

// const char *SegmentInterface::Description() const {
  
//   description_x = "";
//   std::list<string>::const_iterator it;

//   for(it=method_descriptions.begin();it != method_descriptions.end();it++)
//     description_x += *it+" + "; 

//   size_t pos=description_x.rfind("+");
//   if(pos != string::npos)
//     description_x.erase(pos);

//   return description_x.c_str();

// }

//
///////////////////////////////////////////////////////////////////////////


// ///////////////////////////////////////////////////////////////////////////
// #ifdef USE_IL
// ilMemoryImg *Segmentation::img_pixel_data::getILImage() const
// {
//   switch(type){
//   case pixeldata_int:
//     {
//       int *buffer = new int[width*height];
//       memcpy(buffer,di,width*height*sizeof(int));
//       iflSize size(width,height,1);
//       return new ilMemoryImg(buffer,size,iflUInt);
//     }
//     break;
//   case pixeldata_float:
//     {
//       float *buffer = new float[width*height];
//       memcpy(buffer,df,width*height*sizeof(float));
//       iflSize size(width,height,1);
//       return new ilMemoryImg(buffer,size,iflFloat);
//     }
//     break;

//   case pixeldata_float3:
//     {
//       float *buffer = new float[width*height*3];
//       memcpy(buffer,df,width*height*3*sizeof(float));
//       iflSize size(width,height,3);
//       return new ilMemoryImg(buffer,size,iflFloat);
//     }
//     break;

//  default:
//    throw string("Segmentation::img_pixel_data::getILImage : unknown image type");
//    return NULL;
//   }
// }
// #endif

// ///////////////////////////////////////////////////////////////////////////
// #ifdef USE_MAGICK
// ImageMagick::Image *Segmentation::img_pixel_data::getMagickImage() const
// {
//   throw string("Segmentation::img_pixel_data::GetMagickImage : not yet implemented");
//   return NULL;
// }
// #endif

// ///////////////////////////////////////////////////////////////////////////

// static Zoning list_entry(true);

// ///////////////////////////////////////////////////////////////////////////

// const char *Zoning::Version() const { 
//   return vcid;
// }

// ///////////////////////////////////////////////////////////////////////////

// void Zoning::UsageInfo(ostream& os) const { 
//   os << "Zoning :" << endl
//      << "  options : " << endl
//      << "    -Zn  = -Z   = no zoning" << endl
//      << "    -Zd         = diagonal zoning" << endl
//      << "    -Zc         = circular zoning" << endl
//      << "    -Zdc = -Zcd = diagonal&circular zoning (DEFAULT)" << endl;
// }

// ///////////////////////////////////////////////////////////////////////////

// int Zoning::ProcessOptions(int argc, char **argv) { 
//   int argc_old = argc;
//   string switches;

//   while (*argv[0]=='-' && argc>1) {

//     const char *ptr = argv[0]+2;
//     switch (argv[0][1]) {
//     case 'Z':
//       if (strspn(ptr, "cdn")!= strlen(ptr))
// 	EchoError(string("unknown option ")+argv[0]);
//       else {
// 	addToSwitches(argv[0]);
// 	circ = diag = false;
// 	while (*ptr)
// 	  switch (*ptr++) {
// 	  case 'n': circ = diag = false;  break;
// 	  case 'c': circ        = true;   break;
// 	  case 'd':        diag = true;   break;
// 	  }
//       }
//       break;

//     default:
//       EchoError(string("unknown option ")+argv[0]);
//     } // switch
    
//     argc--;
//     argv++; 
//   } // while
  
//   return argc_old-argc;
// }

// ///////////////////////////////////////////////////////////////////////////

// bool Zoning::Process() {
//   if (Verbose()>1)
//     ShowLinks();

//   if (Verbose()>1)
//     cout << "Zoning::Process() : width=" << Width()
// 	 << " height=" << Height() << endl;

//   double x0 = Width()/2.0, y0 = Height()/2.0;
//   double r2 = Width()*Height()/(5.0*3.1415926), r = sqrt(r2);

//   for (int y=0; y<Height(); y++)
//     for (int x=0; x<Width(); x++) {
//       int s = -1;
//       if (diag) {
// 	bool c1 = x*Height()>y*Width();
// 	bool c2 = (Width()-x)*Height()>y*Width();

// 	if (circ && x>=x0-r && x<=x0+r && y>=y0-r && y<=y0+r &&
// 	    (x-x0)*(x-x0)+(y-y0)*(y-y0)<r2)
// 	  s = 3;

// 	else if ( c1 &&  c2) s = 1;
// 	else if (!c1 &&  c2) s = 2;
// 	else if ( c1 && !c2) s = 4;
// 	else if (!c1 && !c2) s = 5;

// 	if (Verbose()>2)
// 	  cout << " (" << x << "," << y << ") -> " << s << endl;
//       }
//       else
// 	s = 1;

//       if (!getImg()->set_pixel_segment(x, y, s)) {
// 	cerr << "Zoning::Process() failed in SetSegment("
// 	     << x << "," << y << "," << s << ")" << endl;
// 	return false;
//       }
//     }

//   return ProcessNextMethod();
// }
} // namespace picsom

///////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: font-lock
// End:

