// -*- C++ -*- 	$Id: EdgeFourier.h,v 1.11 2016/10/25 08:11:19 jorma Exp $
/**
   \file EdgeFourier.h

   \brief Declarations and definitions of class EdgeFourier
   
   EdgeFourier.h contains declarations and definitions of class the EdgeFourier, which
   is a class that performs sobel edge histogram extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.11 $
   $Date: 2016/10/25 08:11:19 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _EdgeFourier_
#define _EdgeFourier_

#include "Feature.h"

namespace picsom {
/// A class that performs the average colour value  extraction.
class EdgeFourier : public Feature {

 public:

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  EdgeFourier(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  EdgeFourier(const string& img, const string& seg,
	      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  EdgeFourier(bool) : Feature(false) {}

  // ~EdgeFourier();

  virtual Feature *Create(const string& s) const {
    return (new EdgeFourier())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    VectorData *d=new VectorData(t, n, this);
    vector<float> v=getRandomFeatureVector();
    d->setLength(v.size());
    d->Zero();
    return d;
  }


  virtual void deleteData(void *p){
    delete (VectorData*)p;
  }

  // copied from PixelBased

 virtual bool Incremental() const { return false; }

  virtual bool ReInitializable() const { return false; } // should be true...

  virtual bool ReInitialize() {
    // something ?
    return Feature::ReInitialize();
  }
  
  virtual bool CalculateOneFrame(int f) { 

    // OBS! Side effect: discards preprocessing result sobelmax

    bool ret= CalculateCommon(f, true);
    SegmentData()->discardPreprocessResult(f,"sobelmax");
    return ret;
}
      
  virtual bool CalculateOneLabel(int f, int l) {
    return CalculateCommon(f, false, l); }

  virtual string TargetType() const { return "image"; }

  virtual treat_type DefaultWithinFrameTreatment() const {
    return treat_concatenate; }
  
  virtual treat_type DefaultBetweenFrameTreatment() const {
    return treat_separate; }
  
  virtual treat_type DefaultBetweenSliceTreatment() const {
    return treat_separate; }
  
  virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

  // copied from EdgeHist

  virtual string Name()          const { return "edgefourier"; }

  virtual string LongName()      const { return "FFT of edge image"; }

  virtual string ShortText()     const {
    return "16x16 FFT of Sobel edge image."; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5");
    return centerdiag; 
  }

  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const{
    return vector<float>(128);
  }

  /*
  virtual bool probeTileFeatures(int x1,int y1, int x2, int y2,
			 featureVector &v,int probelabel=-1,
				 bool saveLabeling=false);
  */

  virtual int printMethodOptions(ostream&) const;

  protected:

  // copied from PixelBased

  virtual vector<string> UsedDataLabels() const {
    vector<pair<int,string> > labs = SegmentData()->getUsedLabelsWithText();
    vector<pair<int,string> >::iterator i;

    if (LabelVerbose()) {
      cout << "PixelBased::UsedDataLabels() : " << endl;
      for (i=labs.begin(); i<labs.end(); i++)
	cout << "  " << i->first << " : \"" << i->second << "\"" << endl;
    }

    if (!UseBackground()) {
      for (i=labs.begin(); i<labs.end(); )
	if (i->second=="background")
	  labs.erase(i);
	else
	  i++;
    }
    
    vector<string> ret;
    for (i=labs.begin(); i<labs.end(); i++) {
      char tmp[100];
      sprintf(tmp, "%d", i->first);
      ret.push_back(tmp);
    }
    
    if (LabelVerbose())
      ShowDataLabels("PixelBased::UsedDataLabels", ret, cout);
    
    return ret;
  }
  
  /** Function commonly used by CalculateOneFrame and CalculateOneLabel
      \param f the frame to calculate feature from
      \param all true if we are to calculate for all segments
      \param l the dataindex for the segment to calculate for
   */
  virtual bool CalculateCommon(int f, bool all, int l = -1) {
    // cox::tictac::func tt(tics, "EdgeFourier::CalculateCommon");

    map<int,float*> maskedImages;

    EnsureImage();
    rptr=(preprocessResult_SobelMax*)
      SegmentData()->accessPreprocessResult(f,"sobelmax");

    int width = Width(true), height = Height(true);
    int size=width*height;

    int *suppressionmask=NULL;

    if(suppressborders>=0){
      // borders will be suppressed, so prepare mask
      suppressionmask=getBorderSuppressionMask(suppressborders);      
    }

    if (FrameVerbose())
      cout << "EdgeFourier::CalculateCommon(" << all << "," << l << "), wxh="
	   << width << "x" << height << "=" << width*height << endl;

    //    cout << "dumping edge detection result" << endl;

    //for (int i=0; i<size; i++)
    //  cout << rptr->d[i] << endl;


    picsom::imagedata *idata = NULL;
    bool debug = false;
    if (debug)
      idata = new picsom::imagedata(width, height, 1,
				    picsom::imagedata::pixeldata_float);

    if(suppressionmask)
      for(int idx=0;idx<width*height;idx++)
	if(suppressionmask[idx])
	  rptr->d[idx]=0;

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++) {

	vector<int> svec = SegmentVector(f, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  int s = svec[j];
	  if (s<0){
	    //	  cerr << "(x,y)=(" <<x<<","<<y<<"), s="<<s<<endl; 
	    throw "segment label<0";
	  }
	
	  if(maskedImages.count(s)==0){
	    // allocate new zero mask

	    maskedImages[s]=new float[size];
	    memset(maskedImages[s],0,size*sizeof(float));
	  }

	  const int idx=x+y*width;

	  maskedImages[s][idx]=rptr->d[idx];

	}
      }

    for(map<int,float*>::iterator it=maskedImages.begin();
	it!=maskedImages.end(); it++){
      int i= DataIndex(it->first, true);

      if(idata){
	int idx=0;
	for(int y=0;y<height;y++)
	  for(int x=0;x<width;x++)
	    idata->set(x,y,it->second[idx++]);
	
	char fname[80];
	sprintf(fname,"masked-%d.png",it->first);
	
	imagefile::write(*idata,fname);

      }
	

      if(i>=0){
	((VectorData*)GetData(i))->setVector(CalculateFFT(it->second));
      }

      

      delete[] it->second;
      it->second=NULL;
    }

    delete idata;
    delete[] suppressionmask;

    return true;
  }

public:
  virtual featureVector CalculateRegion(const Region& r) {

    rptr=(preprocessResult_SobelMax*)
      SegmentData()->accessPreprocessResult("sobelmax");

    VectorData *d=
      (VectorData*)CreateData(PixelType(),ConcatCountPerPixel(),0);
    // obs!  where is d freed ???

    // now it is (maybe not)

    int width = Width(true), height = Height(true);
    int size=width*height;

    float *maskedImage=new float[size];
    memset(maskedImage,0,size*sizeof(float));
    

    const coordList *lptr=r.accessList();
    coordList::const_iterator it;

    if(lptr==NULL){ // the region doesn't support accessing through pointer
      coordList l=r.listCoordinates();
      for(it=l.begin();it!=l.end();it++){
	const int idx=it->x+it->y*width;
	maskedImage[idx]=rptr->d[idx];
      }
    } else
      for(it=lptr->begin();it!=lptr->end();it++){
	const int idx=it->x+it->y*width;
	maskedImage[idx]=rptr->d[idx];
      }
    

    featureVector fV=CalculateFFT(maskedImage);
    deleteData(d);

    delete[] maskedImage;

    return fV;
  }

  virtual bool moveRegion(const Region &/*r*/, int /*from*/, int /*to*/) {
    throw string("EdgeFourier::moveRegion() not implemented");
  }

  virtual bool ProcessOptionsAndRemove(list<string>&); 

protected:
 
  preprocessResult_SobelMax *rptr;
  
  featureVector CalculateFFT(float *e);

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
