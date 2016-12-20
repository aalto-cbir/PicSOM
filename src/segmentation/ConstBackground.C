#include <ConstBackground.h>
#include <math.h>

static const string vcid =
"@(#)$Id: ConstBackground.C,v 1.12 2009/04/30 08:03:50 vvi Exp $";

static ConstBackground list_entry(true);

///////////////////////////////////////////////////////////////////////////

ConstBackground::ConstBackground() {

 }

///////////////////////////////////////////////////////////////////////////

ConstBackground::~ConstBackground() {

 }

///////////////////////////////////////////////////////////////////////////

const char *ConstBackground::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void ConstBackground::UsageInfo(ostream& os) const { 
  os << "ConstBackground :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

bool ConstBackground::PreProcess() {

  averages_calculated=false;
  filtered=NULL;

  return true;
}

///////////////////////////////////////////////////////////////////////////
bool ConstBackground::DoProcess() {

  int pixcount=0;
 
  bool ok,changed;
  int x,y,s;
  int passcount=0;

  // for (y=0; y<Height(); y++) for(x=0;x<Width();x++){
//     float r,g,b;

//     Segmentation::GetPixelRGB(x,y,r,g,b);
//     s=sqrt((r*r+g*g+b*b)/3);

//     //  cout << "(x,y)=(" <<x << "," << y <<") (r,g,b)=" << r << "," 
//     // << g << "," << b << ") s=" << s << "(" << (int)(unsigned char) s << ")" << endl; 

//     if(s>254) s=254;

//     SetPixelSegment(x,y,s);
//   }

//    return true;

  for (y=0; y<Height(); y++) for(x=0;x<Width();x++){
    ok=SetPixelSegment(x,y,1);
    if(!ok){
      cerr << "ConstBackground::Process() failed in SetPixelSegment()" << endl;
      return false;
    }
  }
  
   do{
    changed=false;
    passcount++;
    for (y=0; y<Height(); y++) for(x=0;x<Width();x++){
      ok=GetPixelSegment(x,y,s);
      if(!ok){
	cerr << "ConstBackground::Process() failed"
	     << " in GetPixelSegment()" << endl;
	return false;
      }

      if(s==0) continue; // already marked as background

      if(  BackgroundInNeighbour(x,y) &&
	   MedianCompare(x,y)
	  ){
	changed=true;
	ok=SetPixelSegment(x,y,0);
	if(!ok){
	  cerr << "ConstBackground::Process() failed"
	       << " in SetPixelSegment()" << endl;
	  return false;
	}
	pixcount++;
      }
    }

  }while(changed);


  if (Verbose()>1){
  cout << "First phase required "<< passcount << " passes." << endl;
  }

  passcount=0;

   do{
    changed=false;
    passcount++;
    for (y=0; y<Height(); y++) for(x=0;x<Width();x++){
      ok=GetPixelSegment(x,y,s);
      if(!ok){
	cerr << "ConstBackground::Process() failed"
	     << " in GetPixelSegment()" << endl;
	return false;
      }

      if(s==0) continue; // already marked as background

      if( BackgroundInNeighbour(x,y,true)>=2 &&
	  IsPixelCompatibleWithBkgnd(x,y)){
	changed=true;
	ok=SetPixelSegment(x,y,0);
	if(!ok){
	  cerr << "ConstBackground::Process() failed"
	       << " in SetPixelSegment()" << endl;
	  return false;
	}
	pixcount++;
      }
    }

    }while(changed);

  if (Verbose()>1){

    cout << "Second phase required "<< passcount << " passes." << endl;
    cout << "Segments contain " << pixcount << " and " 
	 << Height()*Width()-pixcount << " pixels." << endl;
  }

  if(filtered){
    delete[] filtered;
    filtered=NULL;
  }

  return true;
}
///////////////////////////////////////////////////////////////////////////
bool ConstBackground::PostProcess()
{
  return true;
}

///////////////////////////////////////////////////////////////////////////

int ConstBackground::BackgroundInNeighbour(int x,int y,bool include_diagonals)
{
  if(x<1 || x > Width()-2 || y < 1 || y > Height()-2)
    return 3;

  int delta_x[]={1,1,0,-1,-1,-1,0,1};
  int delta_y[]={0,-1,-1,-1,0,1,1,1};

  int val,ctr=0,dir;

  for(dir=0;dir<8;dir++){
    if (!include_diagonals && dir&1) continue;

    if( !GetPixelSegment(x+delta_x[dir],y+delta_y[dir],val)){
      cerr << "ConstBackground::BackGroundInNeighbour() failed in " 
	 << "GetPixelSegment()" << endl;
      return 0;
    }
    if(val==0) ctr++;
  }

  return ctr;

}

///////////////////////////////////////////////////////////////////////////

bool ConstBackground::MedianCompare(int x,int y)
{
  if(!filtered) MarkCompatible();

  return filtered[x+y*Width()];

}

///////////////////////////////////////////////////////////////////////////

void ConstBackground::MarkCompatible()
{
  filtered = new bool[Height()*Width()];

  int x,y;

  int limit=(2*median_radius+1)*(2*median_radius+1)/2;
  
  for(y=0;y<Height();y++) for(x=0;x<Width();x++)
    filtered[x+y*Width()]=(CountBkgndInNeighbourhood(x,y)>=limit); 
 
}

///////////////////////////////////////////////////////////////////////////

int ConstBackground::CountBkgndInNeighbourhood(int x,int y)
{
  int u,v,ctr;

  ctr=0;

  for(v=y-median_radius;v<=y+median_radius;v++)
    for(u=x-median_radius;u<=x+median_radius;u++)
      if(IsPixelCompatibleWithBkgnd(u,v)) ctr++;

  return ctr;

}

///////////////////////////////////////////////////////////////////////////

bool ConstBackground::IsPixelCompatibleWithBkgnd(int x,int y)
{
  if(!averages_calculated) CalculateAverages();
  
  float r,g,b;

  if(x<0||x>=Width()||y<0||y>=Height()) return true;

  bool ok = GetPixelRGB(x, y, r, g, b);
  if (!ok) {
    cerr << "ConstBackground::IsPixelkCompatibleWithBkgnd()"
	 << " failed in GetPixelRGB(" << x<<","<<y << ")" << endl;
    return false;
  }

  return (fabs(r-avgr) < tolerance.r &&
	  fabs(g-avgg) < tolerance.g &&
	  fabs(b-avgb) < tolerance.b );

}

///////////////////////////////////////////////////////////////////////////
void ConstBackground::CalculateAverages()
{
  int pixcount=0;
  int x,y;
  bool ok;
  float r,g,b;

  avgr=avgb=avgr=0;

  // find out the average colour of border pixels

  for (x=0; x<Width(); x++){
    ok = GetPixelRGB(x, 0, r, g, b);
    if (!ok) {
      cerr << "ConstBackground::Process() failed in GetPixelRGB()" << endl;
    }

    avgr += r;
    avgg += g;
    avgb += b;
    
    pixcount++;

    ok = GetPixelRGB(x, Height()-1, r, g, b);
    if (!ok) {
      cerr << "ConstBackground::Process() failed in GetPixelRGB()" << endl;
    }

    avgr += r;
    avgg += g;
    avgb += b;
    
    pixcount++;
  }

  for (y=1; y<Height()-2; y++){
    ok = GetPixelRGB(0,y, r, g, b);
    if (!ok) {
      cerr << "ConstBackground::Process() failed in GetPixelRGB()" << endl;
    }

    avgr += r;
    avgg += g;
    avgb += b;
    
    pixcount++;

    ok = GetPixelRGB(Width()-1, y, r, g, b);
    if (!ok) {
      cerr << "ConstBackground::Process() failed in GetPixelRGB()" << endl;
    }

    avgr += r;
    avgg += g;
    avgb += b;
    
    pixcount++;
  }

  avgr /= pixcount;
  avgg /= pixcount;
  avgb /= pixcount;

  averages_calculated=true;

  FindThreshold();

  if (Verbose()>1)
    cout << "Averages calculated: (" << avgr << " "<< avgg<<" " << avgb <<")"
	 << endl;
}

///////////////////////////////////////////////////////////////////////////

bool ConstBackground::FindThreshold()
{
 //  rgb accu[32];

//   int x,y,i;
//   float r,g,b;
//   bool ok;

//   for (i=0;i<32;i++) accu[i].r=accu[i].g=accu[i].b=0; 

//   for(y=0;y<Height();y++) for (x=0; x<Width(); x++){
//     ok = GetPixelRGB(x, y, r, g, b);
//     if (!ok) {
//       cerr << "ConstBackground::FindThreshold() failed in GetPixelRGB()" << endl;
//     }
    
//     i=fabs(avgr-r);

//     for(i >>= 3;i<32;i++)
//       accu[i&31].r++;

//     i=fabs(avgg-g);
//     for(i >>= 3;i<32;i++)
//       accu[i&31].g++;

//     i=fabs(avgb-b);
//     for(i >>= 3;i<32;i++)
//       accu[i&31].b++;

//   }    

//   for(i=0;i<32;i++)
//     cout << "accu[" << i <<"].r=" << accu[i].r << endl;

//   int limit=0.2*Width()*Height();

//   cout << "limit " << limit << endl; 

//   for(i=31;i>=0&&accu[i].r > limit; i--);
//   ++i <<= 3;
//   tolerance.r = i;

//   for(i=31;i>=0&&accu[i].g > limit; i--);
//   ++i <<= 3;
//   tolerance.g = i;

//   for(i=31;i>=0&&accu[i].b > limit; i--);
//   ++i <<= 3;
//   tolerance.b = i;

//   if(tolerance.r + tolerance.g + tolerance.b > min_tolerance)
    tolerance.r=tolerance.g=tolerance.b=min_tolerance;

    //    if(Verbose())
    //      cout << "tolerances are (" << tolerance.r << " "
    //   << tolerance.g << " " << tolerance.b << ").";

  return true;

}

///////////////////////////////////////////////////////////////////////////

