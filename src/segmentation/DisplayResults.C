// -*- C++ -*-  $Id: DisplayResults.C,v 1.51 2019/04/24 11:28:40 jormal Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <DisplayResults.h>
#include <math.h>

#ifndef min
#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)
#endif // min

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::list;

namespace picsom{
  static const string vcid =
    "@(#)$Id: DisplayResults.C,v 1.51 2019/04/24 11:28:40 jormal Exp $";

  static DisplayResults list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  DisplayResults::DisplayResults() {
    do_masking = false;
    mark_boundaries=false;
    wideBoundaries=false;
    display_tree=false;
    resultfile=NULL;
    resultvideo=NULL;
    whiteBackground=false;
    virtualregions=false;
    video_fps=0;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  DisplayResults::~DisplayResults() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *DisplayResults::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::UsageInfo(ostream& os) const { 
    os << "DisplayResults :" << endl
       << "  options : " << endl
       << "    -o <filename>|<pattern>  Specifies output filename(s)." << endl
       << "    -O                       Standard output naming (%i.res)." << endl
       << "    -v fps                   Set output to video and framerate to fps" << endl
       << "    -s <frameresultname>=<colour>[/extra] " << endl
       << "    -sx <frameresultname>=<colour>[/extra]  (like -s, but matches opt'l ordinal nr.)" << endl
       << "    -S colour type value (show like frame result)" << endl
       << "    -f <fileresultname>=<colour>[/extra] " << endl
       << "    -fx <fileresultname>=<colour>[/extra] (-f with wildcard matching)" << endl
       << "    -fx tpart=cut            creates output (videos) based on tpart segments" << endl
       << "       supported result types are 'point', 'line', 'box', 'rotated-box', 'coordlist'" << endl
       << "       and 'pixelchain'" << endl  

       << "    -m <label specifier>     shows only specified segments" << endl
       << "    -d <label specifier>     dims other than specified segments" 
       << endl
       << "    -h <label specifier>     shows other segments in shades of grey"
       << endl
       << "    -c <label specifier>     false colouring of specified labels" 
       << endl
       << "    -u <label specifier>     false colouring of specified labels modulated by orig. image " 
       << endl
       << "    -b <label specifier>     mark boundaries of specified labels" 
       << endl
       << "    -B                       same as -b nz" << endl
       << "    -2                       produce 2 pixel wide boundaries" << endl 
       << "    -a                       colour segments w/ their average colour" << endl
       << "    -w                       white background (works with -m)"<< endl
       << endl
       << "    -t                       output hierarchy tree" << endl
       << "    -V                       parse regions from XML" 
       << endl << endl
       << "    only the latest of options {mdhc} is taken into account" << endl
      
       << "    <label specifier> is a list of segment labels separated by"<<endl
       
       << "    '+' signs. 'nz' stands for all nonzero segment labels." << endl
       << "    e.g. '1+2+4', 'nz' " << endl
       << "    /extra can be eg. /text:elephant" << endl
       << endl;
    
  }

  ///////////////////////////////////////////////////////////////////////////
  
  int DisplayResults::ProcessOptions(int argc, char** argv) { 
    Option option;
    int argc_old=argc;
    string tmpstr;
  
    SegmentationResult result;

    while (*argv[0]=='-' && argc>1) {
      string argv0 = argv[0];
      option.reset();
      switch (argv[0][1]) {

      case 's':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -s");
	  break;
	}

      option.option = argv[0];
      option.addArgument(argv[1]);

      ParseResult(argv[1],frameresultlist,argv[0][2]=='x');
      argc--;
      argv++;
      break;

      case 'S':
	if (argc<5) {
	  EchoError("Not enough commandline arguments for switch -S");
	  break;
	}


	option.option = argv[0];
	option.addArgument(argv[1]);
	option.addArgument(argv[2]);
	option.addArgument(argv[3]);
	{
	  TmpResultInfo info;
	  info.colour=argv[1];
	  info.type=argv[2];
	  info.val=argv[3];
	  
	  tmpresultlist.push_back(info);
	}
	argc-=3;
	argv+=3;
	break;

      case 'f':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -f");
	  break;
	}
	
	option.option = argv[0];
	option.addArgument(argv[1]);
	
	ParseResult(argv[1], fileresultlist, argv[0][2]=='x');
	argc--;
	argv++;
	break;

      case 'm':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -m");
	  break;
	}
	
	option.option = "-m";
	option.addArgument(argv[1]);
	
	maskSpecString=argv[1];
	// ParseLabels(argv[1],maskSpec);
	
	do_masking = true;
	masking_method = total_masking;

	argc--;
	argv++;
	break;

      case 'd':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -d");
	  break;
	}

	option.option = "-d";
	option.addArgument(argv[1]);
	
	maskSpecString=argv[1];

	//ParseLabels(argv[1],maskSpec);
	
	do_masking = true;
	masking_method = dim_masking;
	
	argc--;
	argv++;
	break;
	
      case 'h':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -h");
	  break;
	}

	option.option = "-h";
	option.addArgument(argv[1]);

	maskSpecString=argv[1];

	//ParseLabels(argv[1],maskSpec);

	do_masking = true;
	masking_method = hue_masking;

	argc--;
	argv++;
	break;

      case 'c':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -c");
	  break;
	}

	option.option = "-c";
	option.addArgument(argv[1]);

	maskSpecString=argv[1];
	// ParseLabels(argv[1],maskSpec);

	do_masking = true;
	masking_method = false_colour;

	argc--;
	argv++;
	break;

      case 'u':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -u");
	  break;
	}

	option.option = "-u";
	option.addArgument(argv[1]);

	maskSpecString=argv[1];
	// ParseLabels(argv[1],maskSpec);

	do_masking = true;
	masking_method = modulated_colour;

	argc--;
	argv++;
	break;

      case 'a':
	option.option = "-a";
	do_masking = true;
	masking_method = average_rgb;

	break;

      case 'w':
	option.option = "-w";
	whiteBackground=true;

	break;

      case 't':
	option.option = "-t";
	display_tree = true;

	break;

      case 'V':
	option.option = "-V";
	virtualregions=true;

	break;


      case 'b':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -b");
	  break;
	}

	option.option = "-b";
	option.addArgument(argv[1]);

	boundarySpecString = argv[1];
	// ParseLabels(argv[1],boundarySpec);

	mark_boundaries  = true;

	argc--;
	argv++;
	break;

      case 'B':
	option.option = "-B";
	boundarySpecString="nz";

	// ParseLabels("nz",boundarySpec);
	mark_boundaries  = true;

	break;

      case '2':
	option.option = "-2";
	wideBoundaries=true;

	break;

      case 'o':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch -o");
	  break;
	}

	option.option = "-o";
	option.addArgument(argv[1]);

	SetResultName(argv[1]);
	argc--;
	argv++;
	break;

      case 'O':
	option.option = argv0;
	if (argv0=="-OB")
	  SetResultName("%B/%m:%i.res");
	else if (argv0=="-O")
	  SetStandardResultNaming();
	else {
	  EchoError("option <"+argv0+"> not understood");
	  break;
	}
	  
	break;
      
      case 'v':
	option.option = "-v";
	if (argc>=3) {
	  SetVideoFrameRate(atof(argv[1]));
	  argc--;
	  argv++;
	} else
	  SetVideoFrameRate(0);

	break;

      default:
	EchoError(string("unknown option ")+argv[0]);
      } // switch

      if(!option.option.empty())
	addToSwitches(option);      

      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;
  }

///////////////////////////////////////////////////////////////////////////

  bool DisplayResults::OpenExtraFiles() {
    result_filename = FormResultName();
    
    if (!result_filename.length())
      throw string("Empty result_filename in DisplayResults::InitialiseDr().");

    //string filename = *GetInputImageName();
    if (video_fps > 0/*filename.substr(filename.length()-4) == ".mpg"*/) {
      videofile::debug(Verbose()?1:0);
      resultvideo = new videofile(result_filename, true);
    } else
      resultfile = new imagefile(result_filename,true,"image/tiff");
    //if(Verbose()>1)
    //  cout << "Formed result file name " <<result_filename<<endl;

    AddToResultFiles(result_filename);
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool DisplayResults::CloseExtraFiles() {
    if (resultvideo) {
      if(Verbose()>1)
	cout << "Writing "<<resultvideo->get_num_frames()
	     << " frames to video file " << result_filename << endl;

      resultvideo->write(video_fps);

      delete resultvideo;
      resultvideo=NULL;      

      if (!resultfile) return true;
    }

    if (Verbose()>1)
      cout << "Writing "<<resultfile->nframes()<<" frames to file "
	   <<result_filename<<endl;
    
    if (!resultfile)
      return false;

    resultfile->write();
    //   cout << "write done" << endl;
    delete resultfile;
    resultfile=NULL;

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::AddResultFrame(imagedata& d) {
    if (resultvideo){
      if (cutFrames.empty())
	resultvideo->add_frame(d);
      else 
	AddResultCutFrame(d);
    }

    if (resultfile) 
      resultfile->add_frame(d);
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::AddResultCutFrame(imagedata& d) {
    size_t cf = getImg()->getCurrentFrame();
    size_t nf = getImg()->getNumFrames();

    if (cutFrames.at(cf)) {
      if ((cf>1 && !cutFrames.at(cf-2)) || 
	  (cf<nf-1 && !cutFrames.at(cf+2))) {
	size_t x,y,w=d.width(),h=d.height();
	for(x=0;x<w;x++) for(y=0;y<h;y++) {
	  vector<float> rgb = d.get_float(x,y);
	  rgb[0] /= 1.5; rgb[1] /= 1.5; rgb[2] /= 1.5;
	  d.set(x,y,rgb);
	}

      } else if ((cf>0 && !cutFrames.at(cf-1)) || 
		 (cf<nf && !cutFrames.at(cf+1))) {
	size_t x,y,w=d.width(),h=d.height();
	for(x=0;x<w;x++) for(y=0;y<h;y++) {
	  vector<float> rgb = d.get_float(x,y);
	  rgb[0] /= 3.0; rgb[1] /= 3.0; rgb[2] /= 3.0;
	  d.set(x,y,rgb);
	}
      }
      d.set(0,0,NamedRGB::GetRGB("red").v); 
      d.set(0,1,NamedRGB::GetRGB("blue").v);
      resultvideo->add_frame(d);

      if (cf<nf && !cutFrames.at(cf+1)) {
// 	for (size_t bl=0; bl<5; bl++) 
// 	  resultvideo->add_frame(*blankframe);

	size_t sf = cf+2;
	while (sf<nf && !cutFrames.at(sf))
	  sf++;

	if (sf<nf)
	  SeekToFrame(sf);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  bool DisplayResults::Process() {
    if (Verbose()>1)
      ShowLinks();

    if (false && Verbose()>2)
      getImg()->showHist();

    resultframe=NULL;

    if (display_tree) {
      DisplayTree();
      return ProcessNextMethod();
    }

    if(virtualregions){
      // parse regions from XML to region_spec

      RemoveRegionSpec();
    
      SegmentationResultList *l=
	getImg()->readFileResultsFromXML("","region");
      SegmentationResultList::const_iterator it;
      for(it=l->begin();it!=l->end();it++){
	region_spec.push_back(Region::createRegion(it->value));
      }

      delete l;

      l=getImg()->readFrameResultsFromXML("","region");
      for(it=l->begin();it!=l->end();it++){
	region_spec.push_back(Region::createRegion(it->value));
      }

      delete l;

      cout << "region_spec parsed" << endl;

    }


    if (!maskSpecString.empty())
      ParseLabels(maskSpecString.c_str(), maskSpec);
      
    if (!boundarySpecString.empty())
      ParseLabels(boundarySpecString.c_str(), boundarySpec);
      
    if (do_masking)
      DoMasking();

    if (mark_boundaries)
      MarkBoundaries();
    
    if (!resultframe)
      resultframe = getImg()->copyFrame();

    if (Verbose()>1)
      cout << "frame "<<getImg()->getCurrentFrame()<<": Showing "
	   << frameresultlist.size() << " frame results." << endl;
    
    for (size_t i=0; i<frameresultlist.size(); i++)
      ShowFrameResult(frameresultlist[i]);

    for (size_t i=0; i<tmpresultlist.size(); i++)
      ShowTmpResult(tmpresultlist[i]);
    
    for (size_t i=0; i<fileresultlist.size(); i++)
      ShowFileResult(fileresultlist[i]);
  
    AddResultFrame(*resultframe);
    delete resultframe;
    resultframe=NULL;

    return ProcessNextMethod();
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::DisplayTree() {
    //      cout << "dumping xml" << endl;
    //getImg()->_dump_xml();

    xmlNodePtr hNode=getImg()->accessLastFrameResultNode("","regionhierarchy");

    if (!hNode)
      throw string("regionhierarchy not found in xml");

    vector<dr_merge> mergeList;

    // parse hierarchy from xml
    // start from leaf regions
    xmlNodePtr lNode=XMLTools::xmlNodeFindDescendant(hNode,"regionhierarchy->leafregions");
    if (!hNode)
      throw string("leafnodeinfo not found in xml");

    xmlChar *prop=xmlGetProp(lNode,(const xmlChar*)"inbitmap");
    string propstr = prop?(char*)prop:"";

    if (propstr=="yes" || propstr=="true"){ // leaf nodes stored in bitmap
      vector<int> *lbls=getImg()->getUsedLabels();
      int n=lbls->size();
      for(int i=0;i<n;i++)
	if((*lbls)[i]<0||(*lbls)[i]>=n)
	  throw string("Bitmap doesn't contain a"
		       " valid leaf region representation");

      delete lbls; lbls=NULL;

      mergeList= vector<dr_merge>(n);
      for(int i=0;i<n;i++)
	mergeList[i].l=new coordList;

      int x,y,w=getImg()->getWidth(),h=getImg()->getHeight(),s;
      for(x=0;x<w;x++) for(y=0;y<h;y++){
	getImg()->get_pixel_segment(x,y,s);
	mergeList[s].l->push_back(coord(x,y));	  

      }

    } else { // leaf regions in xml
      for(xmlNodePtr rNode=XMLTools::xmlNodeFindDescendant(lNode,"region");
	  rNode; rNode=XMLTools::xmlNodeFindNextSibling(rNode,"region")){
	xmlChar *lblstr=xmlGetProp(rNode,(const xmlChar*)"label");
	unsigned int lbl;
	if(sscanf((char *)lblstr,"%u",&lbl) != 1 || lbl != mergeList.size())
	  throw string("Leaf regions parsed in wrong order from xml.");

	xmlFree(lblstr);

	cout << "parsing leaf region lbl" << lbl << endl;

	string rspec=XMLTools::xmlNodeGetContent(rNode);
	PredefinedTypes::Region reg(rspec);
	  
	if(reg.id != "list") 
	  throw string("Currently only list regions supported");

	PredefinedTypes::CoordList cl(reg.value);

	dr_merge tmpMerge;
	tmpMerge.l=new coordList(cl.l);
	mergeList.push_back(tmpMerge);
      }
    }
      
    xmlFree(prop);

    // wipe clean segmentation bitmap and use it as
    // scratch bitmap from now on 

    int x,y,w=getImg()->getWidth(),h=getImg()->getHeight();
    for(x=0;x<w;x++) for(y=0;y<h;y++)
      getImg()->set_pixel_segment(x,y,0);

    // extract boundaries for leave regions


    int nleaves=mergeList.size();
    // cout << "parsed " << nleaves << " leaf regions." << endl;


    for(int i=0;i<nleaves;i++){
      mergeList[i].lb=new coordList;
      find_boundary(*(mergeList[i].l),*(mergeList[i].lb),i);
    }

    // parse merges from xml

    xmlNodePtr mlNode=XMLTools::xmlNodeFindDescendant(hNode,"regionhierarchy->mergelist");
      
    if(!mlNode)
      throw string("<mergelist> not found");

    for(xmlNodePtr mNode=XMLTools::xmlNodeFindDescendant(mlNode,
							 "merge");
	mNode;
	mNode=XMLTools::xmlNodeFindNextSibling(mNode,"merge")){
      xmlChar *f1,*f2,*t;
      unsigned int from1,from2,to;

      t=xmlGetProp(mNode,(const xmlChar*)"to");

      //if(t)
      //  cout << "Got property string" << (char*)t<<endl; 

      if(!t || (sscanf((char*)t,"%u",&to)!= 1) || to != mergeList.size())
	throw string("Error in reading property \"to\"");

      f1=xmlGetProp(mNode,(const xmlChar*)"from1");
      if(!f1 || (sscanf((char*)f1,"%u",&from1)!= 1) || // from1<0 || 
	 from1 >= mergeList.size())
	throw string("Error in reading property \"from1\"");

      f2=xmlGetProp(mNode,(const xmlChar*)"from2");
      if(!f2 || (sscanf((char*)f2,"%u",&from2)!= 1) || // from2<0 || 
	 from2 >= mergeList.size())
	throw string("Error in reading property \"from2\"");

      xmlFree(t); xmlFree(f1); xmlFree(f2);


      dr_merge tmpMerge;
      tmpMerge.from1=from1;
      tmpMerge.from2=from2;
      mergeList.push_back(tmpMerge);

    }

    // label the merges with hierarchy levels

    for(int i=mergeList.size()-1;i>=0;i--)
      if(mergeList[i].level<0) // not labeled yet
	label_hierarchy(mergeList,i,0);

      
    int maxlevel=mergeList[0].level;
    for(int i=1;i<nleaves;i++) // maxlevel is found among leaf regions
      if(mergeList[i].level>maxlevel) maxlevel=mergeList[i].level;

    if(Verbose())
      cout << "Region hierarchy has "<<maxlevel+1<< " levels." << endl;

    // synthesise images

    vector<float> blackVec(NamedRGB::GetRGB("black").v);
    vector<float> whiteVec(NamedRGB::GetRGB("white").v);
    vector<float> blueVec(NamedRGB::GetRGB("blue").v);
    vector<float> redVec(NamedRGB::GetRGB("red").v);

    for(int level=maxlevel;level>=0;level--){
      resultframe=getImg()->copyFrame();


      for(size_t i=0; i<mergeList.size(); i++){

	dr_merge &m=mergeList[i];

	if(m.level > level) continue;
	else if(m.lb != NULL){ // leaf region from this or upper level
	  coordList::const_iterator it;
	  for(it=m.lb->begin();it!=m.lb->end();it++)
	    resultframe->set(it->x,it->y,redVec);
	}

	if(m.level != level) continue;

	if(m.lb==NULL){
	  if(m.from1<0 ||m.from2<0)
	    throw string("Leaf region boundary list missing.");

	  // l=l1+l2
	  m.l=new coordList(*mergeList[m.from1].l);
	  m.l->insert(m.l->end(),mergeList[m.from2].l->begin(),
		      mergeList[m.from2].l->end());

	  m.lb=new coordList;

	  find_boundary(*m.l,*m.lb,i,mergeList[m.from1].lb,
			mergeList[m.from2].lb);

	  // draw the lists to the resultframe
	  coordList::const_iterator it;
	  for(it=mergeList[m.from1].lb->begin();
	      it!=mergeList[m.from1].lb->end();it++)
	    resultframe->set(it->x,it->y,blackVec);

	  for(it=mergeList[m.from2].lb->begin();
	      it!=mergeList[m.from2].lb->end();it++)
	    resultframe->set(it->x,it->y,whiteVec);
	    
	  for(it=m.lb->begin();it!=m.lb->end();it++)
	    resultframe->set(it->x,it->y,blueVec);
	    
	  delete mergeList[m.from1].l; mergeList[m.from1].l=NULL;
	  delete mergeList[m.from1].lb; mergeList[m.from1].lb=NULL;
	  delete mergeList[m.from2].l; mergeList[m.from2].l=NULL;
	  delete mergeList[m.from2].lb; mergeList[m.from2].lb=NULL;

	}

      }

      AddResultFrame(*resultframe);
      delete resultframe;
      resultframe=NULL;
    }

    for(size_t i=0; i<mergeList.size(); i++){
      delete mergeList[i].l;
      delete mergeList[i].lb;
    }

    mergeList.clear();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  string DisplayResults::FormResultName() {
    string pattern = result_pattern.length()?result_pattern:"%i.res";
    string filename = *GetInputImageName();
    if(filename.empty())
      filename=*GetInputSegmentName();
    return getImg()->FormOutputFileName(pattern,filename,
					FirstMethod()->ChainedMethodName());
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::ShowFrameResult(const ResultInfo& info) {
    SegmentationResultList *resultlist = 
      getImg()->readFrameResultsFromXML(info.name, "", info.wildcard);
    ShowResultList(resultlist, info);
    delete resultlist;
  }


  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::ShowTmpResult(const TmpResultInfo& info) {
    SegmentationResultList resultlist;
    SegmentationResult tmpresult;

    tmpresult.type=info.type;
    tmpresult.value=info.val;

    ResultInfo i;
    i.colour=info.colour;

    resultlist.push_back(tmpresult);

    ShowResultList(&resultlist,i);
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::ShowResultList(const SegmentationResultList *resultlist,
				      const ResultInfo &info) {
    if (!resultlist)
      return;

    bool debug = Verbose()>1;

    RGBTriple rgb = NamedRGB::GetRGB(info.colour);

    for (size_t i=0; i<resultlist->size(); i++) {
      const SegmentationResult& sr = (*resultlist)[i];

      if (debug)
	cout << sr.Str() << endl;

      const string& mod   = info.modifier;
      const string& type  = sr.type;
      const string& value = sr.value;
      const char *val     = value.c_str();

      if (type=="line") {
	PredefinedTypes::Line l(value);

	  //int x1,y1,x2,y2;
	  //if (sscanf(val,"%d,%d,%d,%d",&x1,&y1,&x2,&y2)==4 ||
	  //  sscanf(val,"%d%d%d%d",&x1,&y1,&x2,&y2)==4)
	  DrawLine(l.x1,l.y1,l.x2,l.y2,rgb);
      }

      else if (type=="box") {
	if (debug)
	  cout << "  Result: box [" << value << "] \"" << info.text << "\""
	       << endl;

	PredefinedTypes::Box b(value);
	//int x1,y1,x2,y2;
	//if (sscanf(val,"%d,%d,%d,%d",&x1,&y1,&x2,&y2)==4 ||
	//    sscanf(val,"%d%d%d%d",&x1,&y1,&x2,&y2)==4)
	DrawBox(b.ulx,b.uly,b.brx,b.bry,rgb);
	if (info.text!="")
	  DrawText(b, info.text, info.textplace);
      }

      else if (type=="rotated-box") {
	PredefinedTypes::RotatedBox b(value);
	//	int l,t,w,h;
	//float theta,xc,yc;
	//if (sscanf(val,"%d,%d,%d,%d,%f,%f,%f",&l,&t,&w,&h,&theta,&xc,&yc)==7 ||
	//   sscanf(val,"%d%d%d%d%f%f%f",&l,&t,&w,&h,&theta,&xc,&yc)==7) {
	  DrawRotatedBox(b.l,b.t,b.w,b.h,b.theta,b.xc,b.yc,rgb);
	  //}
      }

      else if (type=="point") {
	PredefinedTypes::Point p(value);
	//int x,y;
	//if (sscanf(val,"%d,%d",&x,&y)==2 || sscanf(val,"%d%d",&x,&y)==2) {
	if (mod.substr(0, 5)=="cross") {
	    int l = atoi(mod.substr(5).c_str());
	    if (!l)
	      l = 2;
	    DrawLine(p.x-l, p.y  , p.x+l, p.y  , rgb);
	    DrawLine(p.x  , p.y-l, p.x  , p.y+l, rgb);
	    continue;
	  }
	  if (mod.substr(0, 6)=="dcross") {
	    int l = atoi(mod.substr(6).c_str());
	    if (!l)
	      l = 3;
	    for (int a=-1; a<=1; a+=2)
	      for (int b=-1; b<=1; b+=2) {
		DrawLine(p.x+a, p.y+b, p.x+a,   p.y+b*l, rgb);
		DrawLine(p.x+a, p.y+b, p.x+a*l, p.y+b,   rgb);
	      }
	    continue;
	  }
	  DrawPoint(p.x,p.y,rgb);
	  //}
      }

      else if (type=="fd") {
	double d;
	int i=0;
	std::list<double> list;
	while(sscanf(val,"%lf",&d)==1){
	  list.push_back(d);
	  while(isspace(val[i])) i++;
	  while(!isspace(val[i])) i++;
	} 
	DrawFourierContour(list,rgb);
      }
      else if(type=="pixelchain" || type=="coordlist"){
	PredefinedTypes::CoordList l(value);
	//istringstream ss(val);
	//PixelChain pc;
	//ss >> pc;
	vector<coord>::const_iterator it;
	for(it=l.l.begin();it!=l.l.end();it++)
      	  resultframe->set(it->x,it->y,rgb.v);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::ShowFileResult(const ResultInfo &info) {
    SegmentationResultList *resultlist
      = getImg()->readFileResultsFromXML(info.name, "", info.wildcard);

    if (info.colour == "cut") {
      if (cutFrames.empty()) {
	MakeCutFrames(resultlist);
	MakeBlankFrame();
      }
		
    } else 
      ShowResultList(resultlist, info);

    delete resultlist;
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::MakeCutFrames(const SegmentationResultList
				     *resultlist) {
    for (int f=0; f<getImg()->getNumFrames(); f++)
      cutFrames.push_back(false);
    
    for (size_t i=0; i<resultlist->size(); i++) {
      const SegmentationResult& sr = (*resultlist)[i];
      const string& type = sr.type;
      if (type != "frange")
	break;
      const string& value=sr.value;
      
      string begin, end, rest;
      size_t space=value.find(' ');
      if (space != string::npos){
	begin=value.substr(0,space);
	end = value.substr(space+1);
	space = end.find(' ');
	if (space != string::npos) {
	  rest = end.substr(space+1);
	  end = end.substr(0,space);
	}
	for (int f=atoi(begin.c_str()); f<=atoi(end.c_str()); f++)
	  if (f<getImg()->getNumFrames())
	    cutFrames.at(f) = true;	  
      }

      if (Verbose()>1)
	cout << sr.name << " : type=" << type
	     << ", value=" << value << ", begin=" << begin
	     << ", end=" << end << ", rest=" << rest << endl;
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::MakeBlankFrame() {
    blankframe = getImg()->copyFrame();
    size_t x,y,w=getImg()->getWidth(),h=getImg()->getHeight();
    vector<float> blackVec(NamedRGB::GetRGB("black").v);
    // x starts from 1 to keep mencoder from dying
    for(x=1;x<w;x++) for(y=0;y<h;y++) 
      blankframe->set(x,y,blackVec);	
    blankframe->set(0,0,NamedRGB::GetRGB("red").v); 
    blankframe->set(0,1,NamedRGB::GetRGB("blue").v); 
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::ParseResult(const char *result,
				   vector<ResultInfo> &resultlist,
				   bool wildcard) {
    char *tmpstr = new char[strlen(result)+1];
    strcpy(tmpstr, result);

    char *equal = strchr(tmpstr, '=');
    if (!equal) {
      delete tmpstr;    
      throw string( "DisplayResults::ParseResult() no '=' in [")+result+"]";
    }

    *(equal++) = 0;
    char *slash = strchr(equal, '/');
    if (slash)
      *(slash++) = 0;
   
    string modifier = slash ? slash : "";

    ResultInfo info;

    if (modifier.find("text:")==0) {
      size_t s = modifier.find('/');
      if (s!=string::npos)
        s = modifier.size();
      info.text = modifier.substr(5, s-5);
      modifier.erase(0, s+1);

      info.textplace = "tc";
    }

    info.name     = tmpstr;
    info.colour   = equal;
    info.modifier = modifier;
    info.wildcard = wildcard;

    resultlist.push_back(info);
    
    delete tmpstr;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::ParseLabels(const char *spec, std::vector<int> &result)
    const {
    size_t i;
    int label;
    result.clear();
    
    char *tempstr=new char[strlen(spec)+1];
    strcpy(tempstr,spec);
    
    for(i=0;i<strlen(tempstr);i++)
      if(tempstr[i]=='+') tempstr[i] = ' ';
    i=0;
    
    while(i<strlen(tempstr)){
      while(isspace(tempstr[i])) i++;
      if(tempstr[i]=='n' && tempstr[i+1]=='z'){
	// nz
	result.push_back(-1);
      }
      else{
	sscanf(tempstr+i,"%d",&label);
	result.push_back(label);
      }
      while(tempstr[i] != 0 && !isspace(tempstr[i])) i++;
    }
    
    delete tempstr;
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::DrawLine(int x1, int y1, int x2, int y2,
				const RGBTriple &rgb) {
    std::vector<coord_float> list;

    Draw::LinePixelsToList(coord(x1,y1),coord(x2,y2),list);

    for (size_t i=0; i<list.size(); i++)
      if (getImg()->coordinates_ok((int)list[i].x,(int)list[i].y))
        resultframe->set((int)list[i].x,(int)list[i].y,rgb.v);
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::DrawBox(int x1,int y1, int x2,int y2,
                               const RGBTriple &rgb) {
    std::vector<coord_float> list;
    
    Draw::LinePixelsToList(coord(x1,y1),coord(x1,y2),list);
    Draw::LinePixelsToList(coord(x1,y2),coord(x2,y2),list);
    Draw::LinePixelsToList(coord(x2,y1),coord(x2,y2),list);
    Draw::LinePixelsToList(coord(x1,y1),coord(x2,y1),list);

    if (wideBoundaries) {
      Draw::LinePixelsToList(coord(x1-1,y1-1),coord(x1-1,y2+1),list);
      Draw::LinePixelsToList(coord(x1-1,y2+1),coord(x2+1,y2+1),list);
      Draw::LinePixelsToList(coord(x2+1,y1-1),coord(x2+1,y2+1),list);
      Draw::LinePixelsToList(coord(x1-1,y1-1),coord(x2+1,y1-1),list);
      Draw::LinePixelsToList(coord(x1+1,y1+1),coord(x1+1,y2-1),list);
      Draw::LinePixelsToList(coord(x1+1,y2-1),coord(x2-1,y2-1),list);
      Draw::LinePixelsToList(coord(x2-1,y1+1),coord(x2-1,y2-1),list);
      Draw::LinePixelsToList(coord(x1+1,y1+1),coord(x2-1,y1+1),list);
    }
    
    for (size_t i=0; i<list.size(); i++)
      if (getImg()->coordinates_ok((int)list[i].x,(int)list[i].y))
        resultframe->set((int)list[i].x,(int)list[i].y,rgb.v);
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::DrawRotatedBox(int l, int t, int w, int h,
				      float theta, float xc, float yc,
				      const RGBTriple &rgb) {
    scalinginfo si(-theta, xc, yc);
    float xlt, ylt, xlb, ylb, xrt, yrt, xrb, yrb;
    float l1, t1, r1, b1;
    xlt = (float)l;
    ylt = (float)t;
    si.rotate_dst_xy_c(xlt,ylt,l1,t1);
    r1 = l1 + w;
    b1 = t1 + h;
    si.rotate_src_xy_c(xlb, ylb, l1, b1);
    si.rotate_src_xy_c(xrt, yrt, r1, t1);
    si.rotate_src_xy_c(xrb, yrb, r1, b1);

    int wimg = (int)getImg()->getWidth();
    int himg = (int)getImg()->getHeight();

    int nxlt = (int)min(wimg-1, max(0,round(xlt)));
    int nylt = (int)min(himg-1, max(0,round(ylt)));
    int nxlb = (int)min(wimg-1, max(0,round(xlb)));
    int nylb = (int)min(himg-1, max(0,round(ylb)));
    int nxrt = (int)min(wimg-1, max(0,round(xrt)));
    int nyrt = (int)min(himg-1, max(0,round(yrt)));
    int nxrb = (int)min(wimg-1, max(0,round(xrb)));
    int nyrb = (int)min(himg-1, max(0,round(yrb)));  

    std::vector<coord_float> list;
    
    Draw::LinePixelsToList(coord(nxlt,nylt),coord(nxlb,nylb),list);
    Draw::LinePixelsToList(coord(nxlb,nylb),coord(nxrb,nyrb),list);
    Draw::LinePixelsToList(coord(nxrb,nyrb),coord(nxrt,nyrt),list);
    Draw::LinePixelsToList(coord(nxrt,nyrt),coord(nxlt,nylt),list);

    int nxc = (int)round(xc);
    int nyc = (int)round(yc);
    int L = 5;
    Draw::LinePixelsToList(coord(nxc, nyc-L), coord(nxc, nyc+L), list);
    Draw::LinePixelsToList(coord(nxc-L, nyc), coord(nxc+L, nyc), list);
    
    for (size_t i=0; i<list.size(); i++)
      if (getImg()->coordinates_ok((int)list[i].x,(int)list[i].y))
        resultframe->set((int)list[i].x,(int)list[i].y,rgb.v);
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::DrawPoint(int x, int y, const RGBTriple &rgb) {
    std::vector<coord_float> list;
    
    list.push_back(coord_float(x,y));
    list.push_back(coord_float(x-1,y));
    list.push_back(coord_float(x+1,y));
    list.push_back(coord_float(x,y-1));
    list.push_back(coord_float(x,y+1));
    
    for (size_t i=0; i<list.size(); i++)
      if (getImg()->coordinates_ok((int)list[i].x,(int)list[i].y))
        resultframe->set((int)list[i].x,(int)list[i].y,rgb.v);
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::DrawFourierContour(list<double> /* coefficients*/,
					  const RGBTriple &/*rgb*/) {
    // drawing routines ought to be added
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::DrawText(const PredefinedTypes::Box& b,
                                const string& t, const string& /*p*/) {
    

    imagedata txt = imagefile::render_text(t, 15.0);
    txt.convert(imagedata::pixeldata_float);
    txt.force_three_channel();

    size_t dx = (b.brx-b.ulx-txt.width())/2;
    int y = b.uly-txt.height();

    resultframe->copyAsSubimage(txt, b.ulx+dx, y);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::DoMasking() {
    if (resultframe)
      throw string("DisplayResults::DoMasking() : resultframe != NULL");

    switch(masking_method){
    case total_masking:

    resultframe=getImg()->getEmptyFrame();
      if(whiteBackground&&virtualregions){
	const vector<float> v(3,1.0);

	const int w=Width(),h=Height();
	for(int x=0;x<w;x++) for(int y=0;y<h;y++){
	  resultframe->set(x,y,v);
	}
      }

      resultframe=getImg()->getDimmedImage(resultframe,&maskSpec,NULL,0,
                                           virtualregions?&region_spec:NULL);
      if(!resultframe)
	throw string("DisplayResults::DoMasking() : getDimmedImage() failed");
      if(whiteBackground&&!virtualregions)
	WhitenBackground();

      break;
    case dim_masking:
      resultframe=getImg()->getEmptyFrame();
      if(whiteBackground&&virtualregions){
	const vector<float> v(3,1.0);

	const int w=Width(),h=Height();
	for(int x=0;x<w;x++) for(int y=0;y<h;y++){
	  resultframe->set(x,y,v);
	}
      }

      resultframe=getImg()->getDimmedImage(resultframe,&maskSpec,NULL,0.3,virtualregions?&region_spec:NULL);
      if(!resultframe)
	throw string("DisplayResults::DoMasking() : getDimmedImage() failed");

      break;
    case hue_masking:
      if(virtualregions)
	throw string("hue_masking not implemented for virtual regions");
      resultframe=getImg()->getHueMasking(&maskSpec);
      if(!resultframe)
	throw string("DisplayResults::DoMasking() : getHueMasking() failed");
      break;
    case false_colour:
      if(virtualregions)
	throw string("false_colour not implemented for virtual regions");
      resultframe=getImg()->getColouring(&maskSpec);
      if(!resultframe)
	throw string("DisplayResults::DoMasking() : getColouring() failed");
      // break; // this was active until 2017-11-29
      if(whiteBackground)
	WhitenBackground();
      break; // added 2017-11-29

    case modulated_colour:
      resultframe=getImg()->getColouring(&maskSpec);

      if(!resultframe)
	throw string("DisplayResults::DoMasking() : getColouring() failed");
      {
	const int w=Width(),h=Height();
	vector<float> v;
	for(int x=0;x<w;x++) for(int y=0;y<h;y++){
	  getImg()->get_pixel_rgb(x,y,v);
	  float mul=0.5+0.5*(v[0]+v[1]+v[2])/3;
	  v=resultframe->get_float(x,y);
	  v[0]*=mul; v[1]*=mul; v[2]*=mul;
	  resultframe->set(x,y,v);
	}
      }
      if(whiteBackground)
	WhitenBackground();

      break;

    case average_rgb:
      //      if(Verbose()>1)
	cout << "Marking averages." << endl;
      resultframe=getImg()->getAverageRgb();
      if(!resultframe)
	throw string("DisplayResults::DoMasking() : getAverageRgb() failed");
      break;
    default:
      break;
    }
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void DisplayResults::MarkBoundaries() {
    cout << "Marking boundaries" << endl;
    if(!resultframe)
      resultframe=getImg()->copyFrame();
    getImg()->markBoundaries(resultframe,&boundarySpec,wideBoundaries);
  }
 
///////////////////////////////////////////////////////////////////////////
  
 void DisplayResults::find_boundary(const coordList &l, coordList &lb, int lbl,
				    const coordList *lb1, const coordList *lb2)
 {


   //   PredefinedTypes::Box bb;

   lb.clear();
   if(l.empty()) return;

   //bb.ulx=bb.brx=l[0].x;
   //bb.uly=bb.bry=l[0].y;

   int n=l.size();
   for(int i=0;i<n;i++){
     getImg()->set_pixel_segment(l[i],lbl+1);
     //if(l[i].x<bb.ulx) bb.ulx=l[i].x;
     //if(l[i].x>bb.brx) bb.brx=l[i].x;
     //if(l[i].y<bb.uly) bb.uly=l[i].y;
     //if(l[i].y>bb.bry) bb.bry=l[i].y;
   }

   if(lb1==NULL&&lb2==NULL){
     for(int i=0;i<n;i++)
       if(getImg()->isSegmentBoundary(l[i]))
	 lb.push_back(l[i]);
   }
   else{
     if(lb1)
       for(size_t i=0;i<lb1->size();i++)
	 if(getImg()->isSegmentBoundary((*lb1)[i]))
	   lb.push_back((*lb1)[i]);
     if(lb2)
       for(size_t i=0;i<lb2->size();i++)
	 if(getImg()->isSegmentBoundary((*lb2)[i]))
	   lb.push_back((*lb2)[i]);

   }
   
   //   for(int x=bb.ulx;x<=brx;x++) for(int y=bb.uly;y<=bry;y++)
   //  getImg()->setPixelSegment(x,y,0);


 }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::label_hierarchy(std::vector<dr_merge> &mergeList,int root,int level){
    mergeList[root].level=level;

    if(mergeList[root].from1 >=0) 
      label_hierarchy(mergeList,mergeList[root].from1,level+1); 

    if(mergeList[root].from2 >=0) 
      label_hierarchy(mergeList,mergeList[root].from2,level+1); 
  }

  ///////////////////////////////////////////////////////////////////////////

  void DisplayResults::WhitenBackground(){

    set<int> maskset;

    for(size_t i=0;i<maskSpec.size();i++)
      maskset.insert(maskSpec[i]);

    const vector<float> v(3,1.0);

    const int w=Width(),h=Height();
    int s;
    for(int x=0;x<w;x++) for(int y=0;y<h;y++){
      getImg()->get_pixel_segment(x,y,s);
      if(maskset.count(s)==0 && (s==0 && maskset.count(-1)==0))
	resultframe->set(x,y,v);
    }
    
  }
 
  ///////////////////////////////////////////////////////////////////////////
}
