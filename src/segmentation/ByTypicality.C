#include <ByTypicality.h>

#define round(x) (int)floor((x)+0.5)

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::list;

namespace picsom{
  static const string vcid="@(#)$Id: ByTypicality.C,v 1.2 2009/04/29 14:29:11 vvi Exp $";

  static ByTypicality list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  ByTypicality::ByTypicality() {
    primionly=false;
    doScaling=true;
    modulateIntensity=false;
    resultfile=NULL;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  ByTypicality::~ByTypicality() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *ByTypicality::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void ByTypicality::UsageInfo(ostream& os) const { 
    os << "ByTypicality :" << endl
       << "  options : " << endl
       << "    -o <filename>|<pattern>  Specifies output filename(s)." << endl
       << "    -O                       Standard output naming (%i.res)." << endl
       << "    -s <scorefile> " << endl
       << "    -p score only leaf level segments" << endl
       << "    -m modulate intensity of original image" << endl
       << "    -n toggle off intra-image scaling (assumes scores to be [0,1])" << endl
       << endl;
    
  }

  ///////////////////////////////////////////////////////////////////////////
  
  int ByTypicality::ProcessOptions(int argc, char** argv) { 
    Option option;
    int argc_old=argc;
    string tmpstr;
  
    SegmentationResult result;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {

      case 's':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -s");
	  break;
	}

      option.option = "-s";
      option.addArgument(argv[1]);

      readScoreFile(argv[1]);

      //      scorefile=argv[1];

      argc--;
      argv++;
      break;

   case 'p':
      option.option = "-p";
      primionly=true;
      break;

   case 'm':
      option.option = "-m";
      modulateIntensity=true;
      break;

   case 'n':
      option.option = "-n";
      doScaling=false;
      break;

    case 'o':
      if (argc<3){
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

      option.option = "-O";
      SetStandardResultNaming();
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

  bool ByTypicality::OpenExtraFiles() {
    result_filename = FormResultName();

    if (result_filename.length()){
      resultfile = new imagefile(result_filename,true,"image/tiff");
    }
    //if(Verbose()>1)
    //  cout << "Formed result file name " <<result_filename<<endl;
    AddToResultFiles(result_filename);
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool ByTypicality::CloseExtraFiles(){


    if(!resultfile) return true;

    if(Verbose()>1)
      cout << "Writing "<<resultfile->nframes()<<" frames to file "
	   <<result_filename<<endl;
    

    resultfile->write();
    //   cout << "write done" << endl;
    delete resultfile;
    resultfile=NULL;

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool ByTypicality::Process() {

    //   int x,y;
    //   float r,g,b;

    if(Verbose()>1){
      getImg()->showHist();
      //      cout << "frame has " << getImg()->countFrameColours() << " colours" 
      //   << endl;
    }

    resultframe=NULL;

    resultframe=showTypicality();
    
    resultfile->add_frame(*resultframe);
    delete resultframe;
    resultframe=NULL;

    return ProcessNextMethod();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  string ByTypicality::FormResultName()
  {
    string pattern = result_pattern.length()?result_pattern:"%i.res";
    string filename = *GetInputImageName();
    if(filename.empty())
      filename=*GetInputSegmentName();
    return getImg()->FormOutputFileName(pattern,filename,FirstMethod()->ChainedMethodName());
  }
  
  ///////////////////////////////////////////////////////////////////////////

  imagedata *ByTypicality::showTypicality(){

    // parse root label from current image/segmentfile name

    string root = *GetInputImageName();
    if(root.empty())
      root=*GetInputSegmentName();

    if(root.find(':')!=string::npos)
      root=root.substr(root.find(':')!=string::npos);

    root=root.substr(0,root.find('.'));

    if(Verbose()>1) cout << "parsed root " << root << endl;

    int w=Width(), h=Height();

    map<int,float> finalscores;

    const map<set<int>,float> &m=scoremap[root];

    if(Verbose()>1) cout << m.size() << " scores for current image" << endl;

    map<set<int>,float>::const_iterator it;
    for(it=m.begin();it!=m.end();it++){
      const set<int> &s=it->first;
      if(primionly && s.size()>1) continue;
      
      set<int>::const_iterator sit;
      for(sit=s.begin();sit!=s.end();sit++)
	finalscores[*sit] += it->second;
    }
     
    float maxscore=-9999,minscore=9999;

    //    map<int,float>::iterator fit;
    //    for(fit=finalscores.begin();fit!=finalscores.end();fit++)
    //      if(maxscore<fit->second) maxscore=fit->second;

    //    if(Verbose()>1) cout << "max. score: " << maxscore << endl;

    //    if(maxscore != 0)
    //      for(fit=finalscores.begin();fit!=finalscores.end();fit++)
    //	fit->second /= maxscore;

    float *sc_array=new float[w*h];

    imagedata *ret=getImg()->copyFrame();
    getImg()->parseRegionsFromXML();

    vector<float> vf(3);

    int idx=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vector<int> seg=getImg()->getSegmentVector(x,y);
      float val=0;
      for(size_t i=0;i<seg.size();i++)
	val += finalscores[seg[i]];
      
      sc_array[idx++]=val;
      if(val>maxscore) maxscore=val;
      if(val<minscore) minscore=val;
    }
  
    idx=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      float val;
      if(doScaling)
	val=(maxscore>minscore)?(sc_array[idx++]-minscore)/(maxscore-minscore):0;
      else
	val=(sc_array[idx++]>0)?1:0;

      if(modulateIntensity){
	vf=ret->get_float(x,y);
	vf[0] = val*vf[0]+(1-val);
	vf[1] = val*vf[1]+(1-val);
	vf[2] = val*vf[2]+(1-val);
      }
      else
	vf=vector<float>(3,val);

      ret->set(x,y,vf);
    }

    delete[] sc_array;

    return ret;

  }

  ///////////////////////////////////////////////////////////////////////////
  void ByTypicality::readScoreFile(string filename){
    FILE *f=fopen(filename.c_str(),"r");

    if(!f) throw string("Couldn't open score file ")+filename;

    char c1[200];
    float score;

    while(!feof(f)){
      if(fscanf(f,"%s%f",c1,&score)==2){
	string lbl=c1;
	if(lbl.find(':')!=string::npos)
	  lbl=lbl.substr(lbl.find(':')+1);
	
	lbl=lbl.substr(0,lbl.find('_'));

	string setstr=c1;
	setstr=setstr.substr(setstr.find('_')+1);

	set<int> s=parseVirtualSegmentLabel(setstr);

	//if(Verbose()>1) cout
	//  << "parsed " << s.size()<<" labels and root " << lbl << "from "
	//  "score file entry" << c1 << ", score= " << score << endl;

	scoremap[lbl][s]=score;
      }
    }

    fclose(f);
  }

  ///////////////////////////////////////////////////////////////////////////




}
