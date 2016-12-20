#include <PixelTemplate.h>

namespace picsom {
  static const string vcid =
    "@(#)$Id: PixelTemplate.C,v 1.9 2009/08/05 08:57:04 jorma Exp $";

  /////////////////////////////////////////////////////////////////////////////

  const char *PixelTemplate::Version() const { 
    return vcid.c_str();
  }

  /////////////////////////////////////////////////////////////////////////////

  PixelTemplate::PixelTemplate(const imagedata& s,
				int tlx, int tly, int brx, int bry,
			       int w, int h):imagedata(s)
   
  {
    convert(pixeldata_float);
    force_one_channel();
    scalinginfo si(brx-tlx+1, bry-tly+1, w, h, tlx, tly);
    rescale(si);
  }
  
  float PixelTemplate::Corr2(PixelTemplate* pTmpl){
     
    float dotprod = 0, v1=0, v2=0;
    float sqaure2 = 0;
    
    for( unsigned int y = 0; y < height(); y++) {
      for( unsigned int x = 0; x < width(); x++) {

	v1 =  pTmpl->get_one_float(x, y);
	v2 =  this->get_one_float(x, y);
	
	dotprod += v1*v2;
	sqaure2 += v1*v1;
      }
    }
    return dotprod / sqrt(sqaure2);
  }

///////////////////////////////////////////////////////////////////////////

// Calculates the correlation between template tmpl and picture *this
// within the area bounded by (tlx,tly) -> (brx,bry) 
// calculation will happen for each pixel if dx & dy ==1
// otherwise will skip dx pixels horizontally and dy pixels vertically

 PixelTemplate *PixelTemplate::Correlate(const /*Segmentation*/
					 PixelTemplate& tmpl,
					 int tlx, int tly, int brx, int bry,
					 int dx, int dy) const {
   
  int DEBUG2 = 0; // for more debugging info
  // last prints ALL pixel combinations, dont use unless REALLY necessary

  float v1 = 0,         // temp storage of GetPixelGrey and SetPixelGrey
        v2 = 0;         // temp storage of GetPixelGrey and SetPixelGrey
  float dotprod = 0; 	// result of dot product
  int x,y,X,Y;		// (X,Y) for image, (x,y) for template
  int max_x = 0,        // temp storage for maximum corr x-coordinate
      max_y = 0;        // temp storage for maximum corr y-coordinate
  float max_res = -1; 	// maximum correlation result (not really useful)
  int LX,LY,RX,RY;	// left & right edge of image area to be normalized
  int lx,ly,rx,ry;	// left & right edge of template area to be normalized
  int center_x,center_y;// center coordinates of template
  int even_x,even_y;    // is the # of pixels in template even or odd?
  bool ok;		      // temp result for many methods

  // *res defined for final correlation image 
  PixelTemplate *res = new PixelTemplate(width(),height());

  // ok = res->SetImageSize(Width(), Height());
  // /*ok =*/ res->getImg()->accessFrame()->resize(Width(), Height());

  


//   if (!ok) {
//     if(Verbose()) cout << "Correlate Error 1:SetImageSize "<<endl;
//     abort();
//   }
  // initialize result to be completely non-correlative
  for(y = 0; y < (int)res->height(); y++){
    for(x=0; x < (int)res->width(); x++){
      //ok =
	res->set(x,y,(float)-1);
	//if(!ok) {
        //if(Verbose())
	//cout << "Correlate Error 2: Initializing result image"<<endl;
        //abort();
	//      }
    }
  }

  // Calculate center of template
  // Two coordinate sets: lower case (=template) & UPPER case (=img area)
  // !!! ASSUMES THAT template has its data between !!!
  // !!! (0,0) -> (width-1,height-1) !!!
  center_x = (int)ceil((float)tmpl.width()/(float)2)-1;  // rounded down
  center_y = (int)ceil((float)tmpl.height()/(float)2)-1; // ""
  // sets even_x = 1 if x has even number of pixels (same for even_y)
  even_x = ( ceil((float)tmpl.width()/(float)2) == tmpl.width()/2)? 1:0;
  even_y = ( ceil((float)tmpl.height()/(float)2) ==tmpl.height()/2)? 1:0;


  // Normalization is only done for image area that is currently under
  // template (and as well under dot product calculation)

  // We go through every pixel in selected area of the original image
  // dx and dy are used for 'skipping' (faster search)
  for( Y = tly; Y <= bry; Y += dy ) {
    for( X = tlx; X <= brx; X += dx ) {

      // center_x = X && center_y = Y since this is how tmpl is positioned

     
      // Calculate left side edge of IMAGE normalization zone 
      // (the area that is currently masked with template
      // and is still inside given area of image) 
      // Since center is always rounded *down* even(x,y) has no effect
      LX = X-center_x;		
      if(LX < tlx) LX = tlx;	
      LY = Y-center_y;
      if(LY < tly) LY = tly;

      // Calculate right side edge of IMAGE normalization zone 
      // (the area that is currently masked with template
      // and is still inside given area of image) 
      // ! Here even_x and even_y must be considered as in here:
      RX = X+center_x+even_x; 
      RY = Y+center_y+even_y;
      if(RX > brx) RX = brx;
      if(RY > bry) RY = bry;
      if(DEBUG2) cout<<"LX,LY RX,RY"<<LX<<","<<LY<<" "<<RX<<","<<RY<<endl;


      // Calculate left side edge of TEMPLATE normalization zone 
      // (the area that is currently masked with template
      // and is still inside given area of image) 
      // Since center is always rounded *down* even(x,y) has no effect
      lx = LX - (X-center_x);
      if(LX < 0) lx = 0;	
      ly = LY - (Y-center_y);
      if(LY < 0) ly = 0;

      // Calculate right side edge of TEMPLATE normalization zone 
      // (the area that is currently masked with template
      // and is still inside given area of image) 
      // ! Here even_x and even_y must be considered as in here:
      rx = (RX - LX) + lx ; 
      ry = (RY - LY) + ly;
      //if(rx > brx) RX = brx;
      //if(RY > bry) RY = bry;
      if(DEBUG2) cout<<"lx,ly rx,ry"<<LX<<","<<LY<<" "<<RX<<","<<RY<<endl;
     

      // *n_tmp is 'tmpl' template after normalization
      PixelTemplate *n_tmp = new PixelTemplate(tmpl.width(),tmpl.height());

      // ok = n_tmp->SetImageSize(tmpl.getWidth(), tmpl.getHeight());
      ///*ok =*/ n_tmp->getImg()->accessFrame()->resize(tmpl.getWidth(),
      //						  tmpl.getHeight());
      
//       if (!ok) {
//         if(Verbose()) cout << "Correlate Error 3:SetImageSize"<<endl;
//         abort();
//       } 
      // First we normalize (possibly partial) template and store it to n_tmp

      ok = Normalize(&tmpl, n_tmp, lx, ly, rx, ry);
      if (!ok) {
        if(Verbose()) cout << "Correlate Error 4: normalizing template"<<endl;
        abort();
      }

      // Define *norm for temporary normalized orig.img 
      // Contains: area of image currently under tmpl 
      // (area that template masks!)
      PixelTemplate *norm = new PixelTemplate(n_tmp->width(),n_tmp->height());
      //norm->appendImage(imagedata(n_tmp->getWidth(),n_tmp->getHeight());
      // normalized orig.imgsize=templatesize !
      // ok = norm->SetImageSize(n_tmp->Width(), n_tmp->Height()); 
			//      norm->getImg()->accessFrame()->resize(n_tmp->Width(), n_tmp->Height());

//       if (!ok) {
//         if(Verbose()) cout << "Correlate Error 5:SetImageSize"<<endl;
//         abort();
//       }
      // Normalize area that is currently masked by (possibly partial) template
      ok = Normalize(this, norm, LX, LY, RX, RY);
      if(!ok) {
        if(Verbose()) cout<<"Correlate Error 6: Normalize not ok"<<endl;
        abort();
      }


      // reset all vars used in calculation
      dotprod = 0;      v1=0;       v2=0;

      // calc dot product for pixels that are inside templ mask&given mask
      if(DEBUG2) cout << "y:0->RY-LY x:0->RX-LX "<<RY-LY<<" "<<RX-LX<<endl;
      for( y = 0; y <= (RY-LY); y++) {
        for( x = 0; x <= (RX-LX); x++) {

          v1 =  n_tmp->get_one_float(x, y);
	  v2 =  norm->get_one_float(x, y);

          if(!ok) {
            if(Verbose())
	      cout<<"Correlate Error 8:GetPixelGrey: (x,y):"<<(x)<<","<<(y)<<endl;
            abort();
          }

	  if(DEBUG2)
	    cout<<"(x,y):("<<x<<","<<y<<") v1: "<<v1<<" v2: "<<v2<<endl;
          // dot product calculation
          dotprod += v1*v2;
        }
      }

      // saving best correlation and its center position
      if((float)dotprod > (float)max_res) {
	max_res = dotprod;
	max_x = X;
	max_y = Y;
      }

      if(DEBUG2) cout <<"X: "<<X<<" Y: "<<Y<<" dotprod: "<<dotprod<<endl;
      // divide by number of terms to get result between [-1,1] !
      dotprod = (float)dotprod / (float)( ((RY-LY)+1) * ((RX-LX)+1) );
      if(dotprod < (float)-1 || dotprod > (float)1) {
        if(Verbose()) {
         cout<<"Correlate Error 9: Dotproduct boundary error!"<<endl;
         //cout<<"Dprod: "<<dotprod<<" "<<endl;
         //cout<<"Error_5:GetPixelGrey: (x,y):"<<(x)<<","<<(y)<<endl;
         //cout<<"Error_5:GetPixelGrey: (X,Y):"<<(X)<<","<<(Y)<<endl;
         //cout<<"Error_5:GetPixelGrey: (LX,LY):"<<(LX)<<","<<(LY)<<endl;
         //cout<<"Error_5:GetPixelGrey: (RX,RY):"<<(RX)<<","<<(RY)<<endl;
	 //cout<<"even x,y"<<even_x<<" "<<even_y<<endl;
         //cout<<" RY-LY: "<<RY-LY<<" LY-tly "<<LY-tly<<" RX-LX "<<RX-LX<<" LX-tlx "<<LX-tlx<<endl;
         //cout << "X,Y center_x,center_y "<<X<<","<<Y<<" "<<center_x<<","<<center_y<<endl;
         //cout << "even_x,even_y "<<even_x<<","<<even_y<<endl;
        }
        abort();
      }

      res->set(X, Y, dotprod);

    delete n_tmp;
    delete norm;
    //end of for-loops
    }
  }
 
  // save maximum correlation coordinates for later use
  res->SetCorrMax_x(max_x);
  res->SetCorrMax_y(max_y);
  res->SetOptCorrMax_x(max_x);
  res->SetOptCorrMax_y(max_y);

  // divide by number of terms to get result between [-1,1] !
  max_res = (float)max_res / (float)(tmpl.width()*tmpl.height());
  res->SetCorrMax(max_res);
  res->SetOptCorrMax(max_res);
/*

// UNCOMMENT these C-like comments if you want Correlate to print 
// its result into a file called Correlation.png

// // // SAVING CORRELATION IMAGE TO FILE \\ \\ \\
  // *corrpic defined for final correlation image 
  PixelTemplate *corrpic = new PixelTemplate();
  ok = corrpic->SetImageSize(*this);
  if (!ok) {
    if(Verbose()) cout << "Correlate Error 11:SetImageSize "<<endl;
    return NULL;
  }
  float v;
  for(y=0; y < corrpic->Height(); y++) {
    for(x=0; x < corrpic->Width(); x++) {
      ok = res->GetPixelGrey(x,y,v);
      if(!ok) {
        if(Verbose()) cout << "Correlate Error 12: Creating correlation image"<<endl;
        abort();
      }
      v = ((float)1+(float)(-v))*(float)127.5; // to greyscale
      ok = corrpic->SetPixelGrey(x,y,v);
      if(!ok) {
        if(Verbose()) cout << "Correlate Error 13: Creating correlation image"<<endl;
        abort();
      }
    }
  }
  imgdata result;
  result.createFile("Correlation.png",corrpic->Width(),corrpic->Height(),3);
  if (corrpic->CopyToImageObject(&result)) {
    if(Verbose()) cout<<"Correlate Error 14:CopyToImgObj"<<endl;
    abort();
  }
  result.closeFile();
  delete corrpic;
// // // END OF SAVING CORRELATION IMAGE TO FILE \\ \\ \\
*/

  return res;
 }

///////////////////////////////////////////////////////////////////////////
/// simple version (no error checking, limits must be set
/// from calling function so that (brx,bry) will *never* exceed
/// *src image dimensions and *tgt must have big enough size!
 bool PixelTemplate::Normalize(const PixelTemplate *src,
			       PixelTemplate *tgt,
			       int tlx, int tly, int brx, int bry) const {
  float mean,var,v;
  int x,y;
  int DEBUG = 0;  // 1 for printing debugging information
        
  mean = 0;
  for( y=tly; y<= bry; y++ ) {
    for( x=tlx; x<= brx; x++ ) {
      /*ok =*/ v = src->get_one_float(x,y);
//       if(!ok) {
//         if(Verbose()) cout << "Norm ERROR1:(x,y)"<<x<<","<<y<<endl;
//         abort();
//       }
      mean += v;
    }
  }
 
  // divide by number of pixels and we have mean!
  mean = mean /( (bry-tly+1)*(brx-tlx+1) );
  if(DEBUG) cout << "mean: " <<mean<<endl;
      
  var = 0;
  for( y=tly; y<= bry; y++ ) {
    for( x=tlx; x<= brx; x++ ) {
      /*ok =*/ v = src->get_one_float(x,y);
//       if(!ok) {
//         if(Verbose()) cout << "Norm ERROR2:(x,y)"<<x<<","<<y<<endl;
//         abort();
//       }
      var += (v-mean)*(v-mean);
    }  
  }  
  
  // divide by number of pixels and we have variance!
  var = var / ( (bry-tly+1)*(brx-tlx+1) );
  if(DEBUG) cout << "var: " <<var<<endl;
      
  for( y=tly; y<= bry; y++ ) {
    for( x=tlx; x<= brx; x++ ) {
      /*ok =*/ v=src->get_one_float(x,y);
//       if(!ok) {
//         if(Verbose()) cout << "Norm ERROR3:(x,y)"<<x<<","<<y<<endl;
//         abort();
//       }

      // the actual normalization
      if(var!=0) v = (v-mean)/(float)sqrt(var);
      else {
        //cout<<"ZERO-variance in Normalization encountered!"<<endl;
        v = v-mean;
      }     
      //cout << "x: "<<x<<" y: "<<y<<" v: "<<v<<endl;
      //      ok = 
      tgt->set( x-tlx, y-tly, v);
//if(!ok) {
//      if(Verbose()) cout << "ERROR4:(x,y)"<<x<<","<<y<<endl;
//      abort();
//    }
  }
  }
  
  return true;
 }
}

// Local Variables:
// mode: font-lock
// End:
