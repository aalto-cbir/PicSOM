// -*- C++ -*- 	$Id: PseudoMPEG7.h,v 1.8 2016/10/25 08:13:10 jorma Exp $
/**
   \file PseudoMPEG7.h

   \brief Declarations and definitions of class PseudoMPEG7
   
   PseudoMPEG7.h contains declarations and definitions of class 
   PseudoMPEG7, which
   is a base class for rewrites of code thet extract MPEG7-like 
   visual descriptors
  
   \author Ville Viitanemi <Ville.Viitaniemi@hut.fi>
   $Revision: 1.8 $
   $Date: 2016/10/25 08:13:10 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _PseudoMPEG7_
#define _PseudoMPEG7_

#include "Feature.h"

namespace picsom {
/// A class that performs the average colour value  extraction.
class PseudoMPEG7 : public Feature {

 public:

 /// This constructor doesn't add an entry in the methods list.
 PseudoMPEG7(bool) : Feature(false) {}

  PseudoMPEG7(): Feature(){}

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

  virtual featureVector getRandomFeatureVector()const=0;

  virtual bool Incremental() const { return false; }

  virtual bool ReInitializable() const { return false; } // should be true...

  virtual bool ReInitialize() {
    // something ?
    return Feature::ReInitialize();
  }
  
  virtual bool CalculateOneFrame(int f) { 

    // OBS! Side effect: discards all preprocessing results from segmentfile

    bool ret= CalculateCommon(f, true);
    SegmentData()->discardAllPreprocessResults();
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

  // use the flat-1 default

  //  virtual const string& DefaultZoningText() const {
  //    static string centerdiag("5");
  //    return centerdiag; 
  //  }

  protected:

  // copied from PixelBased

  virtual vector<string> UsedDataLabels() const {
    vector<pair<int,string> > labs = SegmentData()->getUsedLabelsWithText();
    vector<pair<int,string> >::iterator i;

    if (LabelVerbose()) {
      cout << "PseudoMPEG7::UsedDataLabels() : " << endl;
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
      ShowDataLabels("PseudoMPEG7::UsedDataLabels", ret, cout);
    
    return ret;
  }
  
  /** Function commonly used by CalculateOneFrame and CalculateOneLabel
      \param f the frame to calculate feature from
      \param all true if we are to calculate for all segments
      \param l the dataindex for the segment to calculate for
   */
  virtual bool CalculateCommon(int f, bool all, int l = -1) {
#ifndef __MINGW32__
    cox::tictac::func tt(tics, "PseudoMPEG7::CalculateCommon");
#endif // __MINGW32__

    map<int,char*> masks;

    EnsureImage();

    int width = Width(true), height = Height(true);
    int size=width*height;

    if (FrameVerbose())
      cout << "PseudoMPEG7::CalculateCommon(" << all << "," << l << "), wxh="
	   << width << "x" << height << "=" << width*height << endl;

    picsom::imagedata *idata = NULL;
    bool debug = false;
    if (debug)
      idata = new picsom::imagedata(width, height, 1,
				    picsom::imagedata::pixeldata_float);

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++) {

	vector<int> svec = SegmentVector(f, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  int s = svec[j];
	  if (s<0){
	    //	  cerr << "(x,y)=(" <<x<<","<<y<<"), s="<<s<<endl; 
	    throw "segment label<0";
	  }
	
	  if(masks.count(s)==0){
	    // allocate new zero mask

	    masks[s]=new char[size];
	    memset(masks[s],0,size*sizeof(char));
	  }

	  const int idx=x+y*width;

	  masks[s][idx]=1;

	}
      }

    for(map<int,char*>::iterator it=masks.begin();
	it!=masks.end(); it++){
      int i= DataIndex(it->first, true);

      if(idata){
	int idx=0;
	for(int y=0;y<height;y++)
	  for(int x=0;x<width;x++)
	    idata->set(x,y,(unsigned char)it->second[idx++]);
	
	char fname[80];
	sprintf(fname,"masked-%d.png",it->first);
	
	imagefile::write(*idata,fname);

      }
	
      if(i>=0){
	((VectorData*)GetData(i))->setVector(CalculateMask(it->second));
      }
      delete[] it->second;
      it->second=NULL;
    }

    delete idata;

    return true;
  }

public:
  virtual featureVector CalculateRegion(const Region& r) {

    VectorData *d=
      (VectorData*)CreateData(PixelType(),ConcatCountPerPixel(),0);
    // obs!  where is d freed ???

    // now it is (maybe not)

    int width = Width(true), height = Height(true);
    int size=width*height;

    char *mask=new char[size];
    memset(mask,0,size*sizeof(char));
    
    const coordList *lptr=r.accessList();
    coordList::const_iterator it;

    if(lptr==NULL){ // the region doesn't support accessing through pointer
      coordList l=r.listCoordinates();
      for(it=l.begin();it!=l.end();it++){
	if(it->x>0&&it->y>0&&it->x<width&&it->y<height){
	  const int idx=it->x+it->y*width;
	  mask[idx]=1;
	}
      }
    } else
      for(it=lptr->begin();it!=lptr->end();it++){
	if(it->x>0&&it->y>0&&it->x<width&&it->y<height){
	  const int idx=it->x+it->y*width;
	  mask[idx]=1;
	}
      }
    

    featureVector fV=CalculateMask(mask);
    deleteData(d);

    delete[] mask;

    return fV;
  }

  virtual bool moveRegion(const Region &/*r*/, int /*from*/, int /*to*/) {
    throw string("PseudoMPEG7::moveRegion() not implemented");
  }

protected:

  virtual featureVector CalculateMask(char *mask)=0;

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
