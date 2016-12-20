#include <TestSegment.h>
#include <math.h>

static const string vcid =
"@(#)$Id: TestSegment.C,v 1.7 2009/04/30 10:23:55 vvi Exp $";

static TestSegment list_entry(true);

///////////////////////////////////////////////////////////////////////////

const char *TestSegment::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void TestSegment::UsageInfo(ostream& os) const { 
  os << "TestSegment :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

int TestSegment::ProcessOptions(int argc, char **argv) {
  int argc_old = argc;
  string switches;

  while (*argv[0]=='-' && argc>1) {
    if (!switches.empty())
      switches += " ";

    switch (argv[0][1]) {
    case 'l':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -l");
	break;
      }

      switches += string("-l ")+argv[1];
      limit = atof(argv[1]);
      argc--;
      argv++;
      break;

    default:
      EchoError(string("unknown option ")+argv[0]);
    } // switch

    argc--;
    argv++; 
  } // while

  if (!switches.empty())
    AddToXMLMethodOptions(switches);

  return argc_old-argc;
}

///////////////////////////////////////////////////////////////////////////

bool TestSegment::DoProcess() {

  // implements a (inverting)
  // pass through reading the segmentation directly from
  // input file

  // each zero pixel is considered as object (label -> 1),
  // nonzeros are background (label -> 0) 

  for(int y=0;y<Height();y++) for(int x=0;x<Width();x++){
    float r,g,b;
    if(!GetPixelRGB(x,y,r,g,b)){
      cerr << "TestSegment::Process() failed in GetPixelRGB("
	   << x << "," << y << ")" << endl;
      return false;
    }
    int s=(r+g+b>3*limit)?0:1;
    if(!SetPixelSegment(x,y,s)){
      cerr << "TestSegment::Process() failed in SetPixelSegment()" << endl;
      return false;
    }
    if (Verbose()>1)
      cout << "(x,y)=("<< x << "," << y << ") = "
	   << r << "," << g << "," << b << " -> " << s << endl;

  }
  return true;
}

///////////////////////////////////////////////////////////////////////////


void TestSegment::GeneratePattern(int ind)
{
  int x,y;
  bool ok;

  for (y=0; y<Height(); y++) for(x=0;x<Width();x++){
    ok=SetPixelSegment(x,y,0);
    if(!ok){
      cerr << "TestSegment::Process() failed in SetPixelSegment()" << endl;
    }
  }
  
  switch(ind){
  case 0:
    SetPixelSegment(2,1,1);
    SetPixelSegment(2,2,1);
    SetPixelSegment(3,2,1);
    SetPixelSegment(1,3,1);
    SetPixelSegment(2,3,1);
    SetPixelSegment(3,3,1);
    SetPixelSegment(4,3,1);
    SetPixelSegment(5,3,1);
    SetPixelSegment(1,4,1);
    SetPixelSegment(2,4,1);
    SetPixelSegment(3,4,1);
    SetPixelSegment(4,4,1);
    SetPixelSegment(5,4,1);
    break;

  case 1:
    SetSegmentationRegion(coord(1,1),coord(3,3),1);
    SetSegmentationRegion(coord(5,1),coord(8,3),1);
    SetSegmentationRegion(coord(1,5),coord(8,10),2);
    SetSegmentationRegion(coord(3,6),coord(5,8),0);
    break;

  }
}  

///////////////////////////////////////////////////////////////////////////

void TestSegment::SetSegmentationRegion(const coord &nw, const coord &se,
					int label)
{
  int x,y;
  if(nw.x > se.x || nw.y > se.y) return;
  
  for(y=nw.y;y<=se.y;y++) for(x=nw.x;x<=se.x;x++)
    SetPixelSegment(x,y,label);

}

///////////////////////////////////////////////////////////////////////////
