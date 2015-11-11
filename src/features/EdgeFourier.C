// -*- C++ -*-  $Id: EdgeFourier.C,v 1.10 2013/02/25 13:11:12 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <EdgeFourier.h>

// #ifdef HAVE_FFTW3_H
// #include <fftw3.h>
// #endif // HAVE_FFTW3_H

namespace picsom {
  static const char *vcid =
    "$Id: EdgeFourier.C,v 1.10 2013/02/25 13:11:12 jorma Exp $";

  static EdgeFourier list_entry(true);

  //===========================================================================

  string EdgeFourier::Version() const {
    return vcid;
  }

  //===========================================================================

  bool EdgeFourier::ProcessOptionsAndRemove(list<string>&l) {
    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  int EdgeFourier::printMethodOptions(ostream &str) const {
    /// obs! dunno yet

    int ret=Feature::printMethodOptions(str);
 
    return ret;
  }

  //===========================================================================

  Feature *EdgeFourier::Initialize(const string& img, const string& seg,
				   segmentfile *s, const list<string>& opt,
				   bool allocate_data) {
    return Feature::Initialize(img,seg,s,opt,allocate_data);
  }

  //===========================================================================

  Feature::featureVector EdgeFourier::CalculateFFT(float *e) {

    // zero-pad the input image to square (zero-padding is  
    // built-in with imagedata)

    int width = Width(true), height = Height(true);
    int maxdim=max(width,height);

    imagedata idata(maxdim,maxdim,1,imagedata::pixeldata_float);
    
    int x,y,ind=0;
    for (y=0;y<height;y++)
      for (x=0;x<width;x++) 
	idata.set(x,y,e[ind++]);
    
    //  cout << "dumping padded image" << endl;
    //     for (y=0;y<maxdim;y++)
    //       for (x=0;x<maxdim;x++){
    // 	cout << "padded data="<<idata.get_one_float(x,y)
    //       <<" x="<<x<<" y="<<y<<endl;
    //       } 

    // write result to file

    //imagefile::write(idata, "padded.png");

    // stretch the input image so that the larger dimension becomes N

    const int N=512;

    scalinginfo si(maxdim,maxdim,N,N);
    idata.rescale(si);

    //write result to file

    // imagefile::write(idata, "scaled.png");

    real2DFFTData fftd;
    fftd.N=N;
    fftd.data=new float[N*N];
    fftd.extracoeff=new float[2*N];

    ind=0;
    for (y=0;y<N;y++)
      for (x=0;x<N;x++){ 
	fftd.data[ind++]=idata.get_one_float(x,y);
	//	if (x==N-1 || y==N-1)
	//	  cout << "data="<<fftd.data[ind-1]<<" x="<<x<<" y="<<y<<endl;
      }
        
    fftd.transform();

    float *transmag=new float[N*(N/2)]; // ignore the last row of results

    float maxval=0;
    ind=0;

    for (y=0;y<N;y++)
      for (x=0;x<N/2;x++){ 
	const int tgtind=y+x*N;
	float m=sqrt(fftd.data[ind]*fftd.data[ind]+
		     fftd.data[ind+1]*fftd.data[ind+1]);
	m=log(1+m);
	// cout << "m="<<m<<" (ind="<<ind<<endl;
	if (m>maxval) maxval=m;
	transmag[tgtind]=m;
	ind +=2;
      }


    //ind=0;
    //for (y=0;y<N;y++){
    //  const int tgtind=y+(N/2)*N      
    //	float m=sqrt(fftd.extracoeff[ind]*fftd.extracoeff[ind]+
    //		     fftd.extracoeff[ind+1]*fftd.extracoeff[ind+1]);
    //      m=log(1+m);
    //      if (m>maxval) maxval=m;
    //      ind +=2;
    //    }

    if (maxval>0){

      for (ind=0;ind<N*(N/2);ind++)
	transmag[ind] /= maxval;

      // perform K-bin histogram equalisation
      
      const int K=16;
      int hist[K];
      float topval[K];

      // calculate the histogram;
      for (int i=0;i<K;i++) hist[i]=0;
      int tot=0;
      
      for (ind=0;ind<N*(N/2);ind++){
	int bin=(int)(K*transmag[ind]);
	if (bin==K) bin--;
	hist[bin]++;
	tot++;
      }
      
      // cout << "histogram bins:" ;
      for (int i=0;i<K;i++){
	float prev=(i)?topval[i-1]:0;
	topval[i]=prev+hist[i]/(float)tot;
	//      cout << " " << topval[i];
      }
      //    cout << endl;

      for (ind=0;ind<N*(N/2);ind++){
	int bin=(int)(K*transmag[ind]);
	if (bin==K) bin--;
	float rem=transmag[ind]*K-bin;
	float base=(bin)?topval[bin-1]:0;
	transmag[ind]=(base+rem*topval[bin]);
      }

      // cout << "histogram equalised" << endl;

      // filter with a (fW+1)x(fW+1) box filter 

      const int fW=N/16;

      const int tmpsize=(N+fW)*(N/2+fW);

      float *tmpimg=new float[tmpsize],*tmpimg2=new float[tmpsize];

      // copy transmag to center of the array

      for (y=0;y<N/2;y++)
	memcpy(tmpimg+(y+fW/2)*(N+fW)+fW/2,transmag+y*N,N*sizeof(float));

      // augment the array with proper mirror-image rows

      // in y-dir the transform is mirrored in respect to borders

      for (int dy=0;dy<fW/2;dy++){
	memcpy(tmpimg+(fW/2-dy-1)*(N+fW)+fW/2,transmag+dy*N,N*sizeof(float));
	memcpy(tmpimg+(N/2+fW/2+dy)*(N+fW)+fW/2,transmag+(N/2-dy-1)*N,
	       N*sizeof(float));
      }

      // transform is circular in x-dir

      for (y=0;y<N/2+fW;y++){
	int yoffs=y*(N+fW);
	for (int dx=0;dx<fW/2;dx++){
	  tmpimg[fW/2-dx-1+yoffs]=tmpimg[N+fW/2-1-dx+yoffs];
	  tmpimg[N+fW/2+dx+yoffs]=tmpimg[fW/2+dx+yoffs];
	}
      }

      //     imagedata iaugm(N+fW,N/2+fW,1,imagedata::pixeldata_float);

      //     for (x=0;x<N+fW;x++) for (y=0;y<N/2+fW;y++){
      //       iaugm.set(x,y,tmpimg[x+y*(N+fW)]);
      //     }

      //    imagefile::write(iaugm, "augmented.png");

      // filter horisontally tmpimg -> tmpimg2

      float sizeinv=1/((float)(fW+1));

      for (y=0;y<N/2+fW;y++){
	const int yoffset=y*(N+fW);
	for (x=fW/2;x<N+fW/2;x++){
	  float s=0;
	  for (int dx=-fW/2;dx<=fW/2;dx++)
	    s += tmpimg[x+dx+yoffset];
	  tmpimg2[x+yoffset]=s*sizeinv;
	}
      }


      for (y=fW/2;y<N/2+fW/2;y++){
	const int yoffset=y*(N+fW);
	for (x=fW/2;x<N+fW/2;x++){
	  float s=0;
	  for (int dy=-fW/2;dy<=fW/2;dy++)
	    s += tmpimg2[x+(y+dy)*(N+fW)];
	  tmpimg[x+yoffset]=s*sizeinv;
	}
      }


      //       imagedata ifiltered(N,N/2,1,imagedata::pixeldata_float);

      //       for (x=0;x<N;x++) for (y=0;y<N/2;y++){
      //         ifiltered.set(x,y,tmpimg[x+fW/2+(y+fW/2)*(N+fW)]);
      //       }

      //       imagefile::write(ifiltered, "filtered.png");

      // then scale to 16x8 by just averaging
    
      Feature::featureVector ret;
      //     imagedata result(16,8,1,imagedata::pixeldata_float);

      int sF = N/16;

      float invsF=1/((float)(sF*sF));

      for (int tgty=0;tgty<8;tgty++)
	for (int tgtx=0;tgtx<16;tgtx++){
	  float s=0;
	  for (int dy=0;dy<sF;dy++)
	    for (int dx=0;dx<sF;dx++)
	      s += tmpimg[tgtx*sF+dx+(tgty*sF+dy)*(N+fW)];
	  ret.push_back(s*invsF);
	  //	 result.set(tgtx,tgty,s*invsF);
	}


      //    imagefile::write(result, "result.png");

      //     for (x=0;x<N;x++) for (y=0;y<N/2;y++){
      //       ifiltered.set(x,y,tmpimg[x+fW/2+(y+fW/2)*(N+fW)]);
      //     }

      //     imagefile::write(ifiltered, "filtered.png");

      delete [] fftd.data;
      delete [] fftd.extracoeff;
      delete [] transmag;	   
      delete [] tmpimg;
      delete [] tmpimg2;

      //  for (idx=0;idx<N*(N/2+1);idx++){

      //     	sqrt((out[idx][0])*(out[idx][0])+(out[idx][1])*(out[idx][1]));
      //       if (val>maxval) maxval=val;
      //       // cout <<"val="<<val<<endl;
      //     }

      //     cout << "maxval="<<maxval<<endl;

      //     idx=0;
      //     for (y=0;y<N;y++)
      //       for (x=0;x<N/2+1;x++)
      // 	idata.set(x,y,(float)out[idx++][0]/maxval);
	
      //     imagefile::write(idata, "transformed.png");


      return ret;
    } else {
      // transform was all-zero

      cout << "transform is zero" << endl;

      delete [] fftd.data;
      delete [] fftd.extracoeff;
      delete [] transmag; 

      return Feature::featureVector(128,0.0);
    }
  }

  //===========================================================================

} // namespace picsom

