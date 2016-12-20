#include <TilePyramid.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::list;

static const string vcid =
  "@(#)$Id: TilePyramid.C,v 1.3 2009/04/30 10:26:27 vvi Exp $";

namespace picsom {
  static TilePyramid list_entry(true);

  ///////////////////////////////////////////////////////////////////////////
  
  TilePyramid::TilePyramid() {
    levels=3;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  TilePyramid::~TilePyramid() {
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  const char *TilePyramid::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void TilePyramid::UsageInfo(ostream& os) const { 
    os << "TilePyramid :" << endl
       << "  options : " << endl
       << "    -l number of levels (default: 3)" << endl
       << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int TilePyramid::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'l':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -l");
	  break;
	}
	option.option = "-l";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&levels) != 1)
	  throw string("switch -l requires an integer specifier.");

	argc--;
	argv++;
	break;

      default:
	throw string("unknown option ")+argv[0];
      } // switch

      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
    
    return argc_old-argc;
    
  }
  
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool TilePyramid::Process() {

    vector<int> startX,stopX,startY,stopY;
    int nX,nY;

  
    if(levels<=0 || levels>5)
      throw string("Expected number of levels in range 1-5");

    int tiles=(int)pow((float)2,levels-1);

    int w=Width(),h=Height();
    float tileW,tileH;

    tileW=w/(float)tiles; 
    tileH=h/(float)tiles;

    if(tileW==0) throw string("tileW==0");
    if(tileH==0) throw string("tileH==0");

    for(float start=0;start <w; start += tileW){
      startX.push_back((int)start);
      stopX.push_back((int)(start+tileW-1));
    }
    nX=startX.size();

    stopX[nX-1]=w-1;

    for(float start=0;start <h;start += tileH){
      startY.push_back((int)(start));
      stopY.push_back((int)(start+tileH-1));
    }
    nY=startY.size();

    stopY[nY-1]=h-1;
      
    if(nX != (int) startX.size() ||
       nX != (int) stopX.size() ||
       nY != (int) startY.size() ||
       nY != (int) stopY.size())
      throw string("TilePyramid:Process(): vector size mismatch.");

    SegmentationResult res;
    res.name="regionhierarchy";
    res.type="regionhierarchy";
    res.value="";

    xmlNodePtr resNode=getImg()->writeFrameResultToXML(this,res);

      
    xmlNodePtr hNode=xmlNewTextChild(resNode,NULL,
				     (xmlChar*)"regionhierarchy",
				     NULL);

    xmlNodePtr lNode=xmlNewTextChild(hNode,NULL,
				     (xmlChar*)"leafregions",
				     NULL);
      
    int idx=0;

    for(int x=0;x<nX;x++) for(int y=0;y<nY;y++){

      char valstr[80];

      sprintf(valstr,"rect:%d,%d,%d,%d",startX[x],startY[y],
	      stopX[x],stopY[y]);
      xmlNodePtr rNode=xmlNewTextChild(lNode,NULL,
				       (xmlChar*)"region",
				       (xmlChar*)valstr);
	  
      char labelstr[30];
      sprintf(labelstr,"%d",idx++); // labeling starts from 0	  
      xmlSetProp(rNode,(xmlChar*)"label",(xmlChar*)labelstr);
    }

    

      // output region merges
      
      xmlNodePtr mlistNode=xmlNewTextChild(hNode,NULL,
					   (xmlChar*)"mergelist",
					   NULL);
      int to=idx;
  
      char fromstr[80],labelstr[80];
      int leveloffset=0;

      for(int tgtlevel=levels-1;tgtlevel>=1;tgtlevel--){
	
	int tgtW=(int)pow((float)2,tgtlevel-1);
	int srcW=2*tgtW;
	
	for(int tgtx=0;tgtx<tgtW;tgtx++)
	  for(int tgty=0;tgty<tgtW;tgty++){
	    int srcx=2*tgtx, srcy=2*tgty;
	    int src0=srcy+srcW*srcx+leveloffset;
	    
	    sprintf(fromstr,"%d,%d,%d,%d",src0,src0+1,src0+srcW,src0+srcW+1);
	    
	    xmlNodePtr mNode=xmlNewTextChild(mlistNode,NULL,
					     (xmlChar*)"merge",
					     NULL);

	    xmlSetProp(mNode,(xmlChar*)"fromlist",(xmlChar*)fromstr);
	    sprintf(labelstr,"%d",to++);
	    xmlSetProp(mNode,(xmlChar*)"to",(xmlChar*)labelstr);
	  }
	leveloffset += srcW*srcW;
      }
    
      return ProcessNextMethod();
  }

} // namespace picsom;

///////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: font-lock
// End:
  
