// $Id: ColorLayout.C,v 1.3 2008/09/29 10:07:37 jorma Exp $	

#include <ColorLayout.h>

namespace picsom {
  static const char *vcid =
    "$Id: ColorLayout.C,v 1.3 2008/09/29 10:07:37 jorma Exp $";

static ColorLayout list_entry(true);

//=============================================================================

string ColorLayout::Version() const {
  return vcid;
}
//=============================================================================

bool ColorLayout::ProcessOptionsAndRemove(list<string>&l){

  return Feature::ProcessOptionsAndRemove(l);

}


//=============================================================================

int ColorLayout::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  return ret;
}

//=============================================================================
Feature *ColorLayout::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  return Feature::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================

  Feature::featureVector ColorLayout::CalculateMask(char *mask){

    // the mask specifies, which pixels to include in calculation  
    // nonzero -> include


    int w = Width(true), h = Height(true);

    // find the bounding box of the mask to claculate the dividing grid

 
    PredefinedTypes::Box bb;

    int ind=0;
    bool first=true;

    for(int y=0;y<h;y++)
      for(int x=0;x<w;x++)
	if(mask[ind++]){
	  if(first){
	    bb.ulx=bb.brx=x;
	    bb.uly=bb.bry=y;
	    first=false;
	  }else{
	    if(x<bb.ulx) bb.ulx=x;
	    if(x>bb.brx) bb.brx=x;
	    if(y<bb.uly) bb.uly=y;
	    if(y>bb.bry) bb.bry=y;
	  }
	}

    //string bbstr; bb.write(bbstr);
    //cout << "bounding box" << bbstr << endl;

    if(first) // not a single pixel in the mask
      return featureVector(12, 0.0);

    // prepare the division to the grid

    vector<int> xdiv(9),ydiv(9);

    int bbw=bb.width(),bbh=bb.height();

    for(int i=0;i<8;i++)
      xdiv[i]=bb.ulx + i*bbw/8;
    xdiv[8]=bb.brx+1;

    for(int i=0;i<8;i++)
      ydiv[i]=bb.uly + i*bbh/8;
    ydiv[8]=bb.bry+1;


    // collect average colour of each grid element (i,j)

    float d[8][8][3];
    size_t count[8][8];

    vector<float> vf;
    size_t globalcount=0;
    vector<float> globalavg(3, 0.0);

    for(int i=0;i<8;i++)
      for(int j=0;j<8;j++){
	count[i][j]=0;
	d[i][j][0]=d[i][j][1]=d[i][j][2]=0;
	for(int x=xdiv[i];x<xdiv[i+1];x++)
	  for(int y=ydiv[j];y<ydiv[j+1];y++){
	    const int ind=x+y*w;
	    if(mask[ind]){
	      count[i][j]++;
	      vf= GetPixelFloats(x, y, pixel_rgb);
	      imagedata::rgb_to_YCbCr(vf[0],vf[1],vf[2]);
	      d[i][j][0] += vf[0];
	      d[i][j][1] += vf[1];
	      d[i][j][2] += vf[2];
	    }
	  }

	if(count[i][j]){
	  d[i][j][0] /= count[i][j];
	  d[i][j][1] /= count[i][j];
	  d[i][j][2] /= count[i][j];

	  globalcount++;
	  globalavg[0] += d[i][j][0];
	  globalavg[1] += d[i][j][1];
	  globalavg[2] += d[i][j][2];
	}
      }
    
    globalavg[0] /= globalcount; // division by zero should not be possible
    globalavg[1] /= globalcount;
    globalavg[2] /= globalcount;

    // representative colours found, pad the empty positions in the 
    // grid with average colour of the non-empty positions

    for(int i=0;i<8;i++)
      for(int j=0;j<8;j++)
	if(count[i][j]==0){
	  d[i][j][0]=globalavg[0];
	  d[i][j][1]=globalavg[1];
	  d[i][j][2]=globalavg[2];
	}
	  
//  cout << "globalavg: ("<<globalavg[0]
// 	 <<globalavg[1] << " "
// 	 <<globalavg[2] << "), globalcount="<<globalcount<<endl;

//     cout << "grid of representative colours:"<<endl; 
    
//     for(int i=0;i<8;i++){
//       for(int j=0;j<8;j++)
// 	cout <<"("<<d[i][j][0]<<" "
// 	     <<d[i][j][1]<<" "
// 	     <<d[i][j][2]<<") ";
//       cout << endl;
//     }

    // quantise the representative colours

    int dquant[8][8][3];

    for(int i=0;i<8;i++)
      for(int j=0;j<8;j++){
	dquant[i][j][0]=(int)(219*d[i][j][0]+16);
	dquant[i][j][1]=(int)(224*d[i][j][1]+128);
	dquant[i][j][2]=(int)(224*d[i][j][2]+128);
      }

//   cout << "quantised:"<<endl; 
    
//     for(int i=0;i<8;i++){
//       for(int j=0;j<8;j++)
// 	cout <<"("<<dquant[i][j][0]<<" "
// 	     <<dquant[i][j][1]<<" "
// 	     <<dquant[i][j][2]<<") ";
//       cout << endl;
//     }


    // perform the full dct even though all the coefficients are not needed

    // define the m-matrix
    const float pi=3.14926536;

    double m[8][8];

    for( int i=0; i<8; i++ ) {
      double s=(i==0) ? sqrt(0.125) : 0.5;
      for( int j=0; j<8; j++ ) {
	m[i][j] = s*cos((pi/8.0)*i*(j+0.5));
      }
    }
    
    int c[8][8][3];

    for(int comp=0;comp<3;comp++){

      int i, j, k;
      double s;
      double tmp[8][8];
      for( i=0; i<8; i++ ) {
	for( j=0; j<8; j++ ) {
	  s = 0.0;
	  for( k=0; k<8; k++ )
	    s += m[j][k]*dquant[i][k][comp];
	  tmp[i][j] = s;
	}
      }
      for( j=0; j<8; j++ ) {
	for( i=0; i<8; i++ ) {
	  s = 0.0;
	  for( k=0; k<8; k++ )
	    s += m[i][k]*tmp[k][j];
	  c[i][j][comp] = (int)trunc(s+0.499999);
	}
      }
    }

    // quantise the coefficients and collect to feature vector

    featureVector ret;
    ret.push_back(quant_Y_DC(c[0][0][0])); // Y DC
    ret.push_back(quant_CbCr_DC(c[0][0][1])); // Cb DC
    ret.push_back(quant_CbCr_DC(c[0][0][2])); // Cr DC

    // 5 Y channel AC components

    ret.push_back(quant_Y_AC(c[1][0][0])); 
    ret.push_back(quant_Y_AC(c[0][1][0])); 
    ret.push_back(quant_Y_AC(c[0][2][0])); 
    ret.push_back(quant_Y_AC(c[1][1][0])); 
    ret.push_back(quant_Y_AC(c[2][0][0])); 
    

    // 2 Cb channel AC components

    ret.push_back(quant_CbCr_AC(c[1][0][1])); 
    ret.push_back(quant_CbCr_AC(c[0][1][1])); 

    // 2 Cr channel AC components

    ret.push_back(quant_CbCr_AC(c[1][0][2])); 
    ret.push_back(quant_CbCr_AC(c[0][1][2])); 

    return ret;

  }

//=============================================================================
} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

