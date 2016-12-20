#include <Tile.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::list;

static const string vcid =
  "@(#)$Id: Tile.C,v 1.5 2009/04/30 10:25:54 vvi Exp $";

namespace picsom {
  static Tile list_entry(true);

  ///////////////////////////////////////////////////////////////////////////
  
  Tile::Tile() {
    xTiles=3;
    yTiles=3;
    overlap=false;
    squareTiles=false;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  Tile::~Tile() {
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  const char *Tile::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void Tile::UsageInfo(ostream& os) const { 
    os << "Tile :" << endl
       << "  options : " << endl
       << "    -T <xTiles>[:<yTiles>] (default: 3:xTiles)" << endl
       << "    -o 50% overlap"   <<endl
       << "    -s square tiles, tile size by max(w,h)"
       << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int Tile::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'T':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -T");
	  break;
	}
	option.option = "-T";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&xTiles) != 1)
	  throw string("switch -T requires an integer specifier.");

	if(strchr(argv[1],':')){
	  if(sscanf(strchr(argv[1],':')+1,"%d",&yTiles) != 1)
	    throw string("switch -T requires an integer specifier after :.");
	}
	else yTiles = xTiles;
	
	argc--;
	argv++;
	break;

  case 'o':

	option.option = "-o";
	overlap=true;

	break;

  case 's':

	option.option = "-s";
	squareTiles=true;

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
  
  bool Tile::Process() {

    vector<int> startX,stopX,startY,stopY;
    int nX,nY;

    
    char valstr[80],lblstr[80];

    int w=Width(),h=Height();
    float tileW,tileH;

    if(squareTiles==false){
      tileW=w/(float)xTiles; 
      tileH=h/(float)yTiles;
    } else{
      tileW=tileH=(w>h)?w/(float)xTiles:h/(float)yTiles;
    }
    //    cout << "w="<<w<<" h="<<h<<endl;
    //cout << "tile sizes determined: tileW="<<tileW<<" tileH="<<tileH<<endl;

    if(tileW==0) throw string("tileW==0");
    if(tileH==0) throw string("tileH==0");

    for(float start=0;start <w; start += tileW){
      startX.push_back((int)start);
      stopX.push_back((int)(start+tileW-1));
    }
    nX=startX.size();

    stopX[nX-1]=w-1;
    if(squareTiles) startX[nX-1]=(int)(w-tileW);

    for(float start=0;start <h;start += tileH){
      startY.push_back((int)(start));
      stopY.push_back((int)(start+tileH-1));
    }
    nY=startY.size();

    stopY[nY-1]=h-1;
    if(squareTiles) startY[nY-1]=(int)(h-tileH);
    
    // if overlap, augment vectors with interpolated values

    // invalid idea !

   //  if(overlap){
//       vector<int> v;
//       v=startX;
//       startX.clear();
//       for(int i=0;i<nX-1;i++){
// 	startX.push_back(v[i]);
// 	startX.push_back((v[i]+v[i+1])/2);
//       }
//       startX.push_back(v[nX-1]);

//       v=stopX;
//       stopX.clear();
//       for(int i=0;i<nX-1;i++){
// 	stopX.push_back(v[i]);
// 	stopX.push_back((v[i]+v[i+1])/2);
//       }
//       stopX.push_back(v[nX-1]);

//       nX=2*nX-1;

//       v=startY;
//       startY.clear();
//       for(int i=0;i<nY-1;i++){
// 	startY.push_back(v[i]);
// 	startY.push_back((v[i]+v[i+1])/2);
//       }
//       startY.push_back(v[nY-1]);

//       v=stopY;
//       stopY.clear();
//       for(int i=0;i<nY-1;i++){
// 	stopY.push_back(v[i]);
// 	stopY.push_back((v[i]+v[i+1])/2);
//       }
//       stopY.push_back(v[nY-1]);
	
//       nY=2*nY-1;
//     }



//     cout << "startX:"<<endl;
//     for(size_t j=0;j<nX;j++)
//       cout <<startX[j]<<endl;

//     cout << "stopX:"<<endl;
//     for(size_t j=0;j<nX;j++)
//       cout <<stopX[j]<<endl;

//     cout << "startY:"<<endl;
//     for(size_t j=0;j<nY;j++)
//       cout <<startY[j]<<endl;

//     cout << "stopY:"<<endl;
//     for(size_t j=0;j<nY;j++)
//       cout <<stopY[j]<<endl;



 
      
    if(nX != (int) startX.size() ||
       nX != (int) stopX.size() ||
       nY != (int) startY.size() ||
       nY != (int) stopY.size())
      throw string("Tile:Process(): vector size mismatch.");
    
    int lbl=1;
    
    for(int x=0;x<nX;x++) for(int y=0;y<nY;y++){

      sprintf(lblstr,"Tile%d",lbl++);
      sprintf(valstr,"rect:%d,%d,%d,%d",startX[x],startY[y],
	      stopX[x],stopY[y]);
      
      SegmentationResult segresult;
      segresult.name=lblstr;
      segresult.type="region";
      segresult.value=valstr;
      
      getImg()->writeFrameResultToXML(this,segresult);
    }

    if(overlap){
      int wincr = (int)(tileW/2), hincr = (int)(tileH/2);

      if(wincr || hincr)
	for(int x=0;x<nX-1;x++) for(int y=0;y<nY-1;y++){

	  sprintf(lblstr,"Tile%d",lbl++);
	  sprintf(valstr,"rect:%d,%d,%d,%d",startX[x]+wincr,startY[y]+hincr,
		  stopX[x]+wincr,stopY[y]+hincr);
      
	  SegmentationResult segresult;
	  segresult.name=lblstr;
	  segresult.type="region";
	  segresult.value=valstr;
      
	  getImg()->writeFrameResultToXML(this,segresult);
	}

    }


    return ProcessNextMethod();
  }

} // namespace picsom;

///////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: font-lock
// End:
  
