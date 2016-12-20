#include <GradientMagn.h>

static const string vcid="@(#)$Id: GradientMagn.C,v 1.2 2009/04/30 09:47:00 vvi Exp $";

static GradientMagn list_entry(true);

///////////////////////////////////////////////////////////////////////////

GradientMagn::GradientMagn() {

}

///////////////////////////////////////////////////////////////////////////

GradientMagn::~GradientMagn() {

}

///////////////////////////////////////////////////////////////////////////

const char *GradientMagn::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void GradientMagn::UsageInfo(ostream& os) const { 
  os << "GradientMagn :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

int GradientMagn::ProcessOptions(int /*argc*/, char** /*argv*/) { 
  return 0;
}

///////////////////////////////////////////////////////////////////////////

bool GradientMagn::DoProcess() {
  
 //  int neighbourhoodradius = 5;

  int *grad = new int[Width()*Height()];

  int x,y,i;

  float r,g,b,r1,g1,b1;

  int gr;

  int size;

  for(y=0;y<Height();y++)
    for(x=0;x<Width();x++){
      GetPixelRGB(x,y,r,g,b);
      gr=0; size=0;
      
      if(!OutOfBounds(x,y-1)){
	GetPixelRGB(x,y-1,r1,g1,b1);
	gr+=((r-r1>0)?r-r1:r1-r);
	gr+=((g-g1>0)?g-g1:g1-g);
	gr+=((b-b1>0)?b-b1:b1-b);
	size++;
      }

      if(!OutOfBounds(x-1,y  )){
	GetPixelRGB(x-1,y,r1,g1,b1);
	gr+=((r-r1>0)?r-r1:r1-r);
	gr+=((g-g1>0)?g-g1:g1-g);
	gr+=((b-b1>0)?b-b1:b1-b);
	size++;
      }

      if(!OutOfBounds(x+1,y)){
	GetPixelRGB(x+1,y,r1,g1,b1);
	gr+=((r-r1>0)?r-r1:r1-r);
	gr+=((g-g1>0)?g-g1:g1-g);
	gr+=((b-b1>0)?b-b1:b1-b);
	size++;
      }

      if(!OutOfBounds(x,y+1)){
	GetPixelRGB(x,y+1,r1,g1,b1);
	gr+=((r-r1>0)?r-r1:r1-r);
	gr+=((g-g1>0)?g-g1:g1-g);
	gr+=((b-b1>0)?b-b1:b1-b);
	size++;
      }
      
      grad[x+y*Width()] = gr * 4.0 /size;
    }

  float fraction=0.2;
    
  int upper=0,lower=0;

  int l=Height()*Width();
  
  for(i=0;i<l;i++) if(grad[i]>upper) upper=grad[i];

  int cnt=0;

  for(;cnt<fraction*l;upper = 0.8*upper-1){
    cnt=0;
    for(i=0;i<l;i++) if(grad[i]>=upper) cnt++;
  }

  for(;cnt<fraction*l;lower= 0.8*lower+0.2*upper+1){
    cnt=0;
    for(i=0;i<l;i++) if(grad[i]<=lower) cnt++;
  }

  int threshold=(6*upper+4*lower)/10;
  
  if(Verbose()>1)
    cout << "Gradient thresholds: lower=" << lower << " upper=" <<upper 
	 <<  " threshold=" << threshold << endl;

  for(y=0;y<Height();y++) for(x=0;x<Width();x++)
  //   {
//       int s;
//       s=grad[x+y*Width()];
//       if(s > 254) s=254; 
//       SetPixelSegment(x,y,s);
//     }

  if(grad[x+y*Width()]>=threshold) SetPixelSegment(x,y,1);
    else SetPixelSegment(x,y,0);

  delete[] grad;

  return true;

}

///////////////////////////////////////////////////////////////////////////


