// -*- C++ -*- 	$Id: SimpleGeometric.h,v 1.5 2016/10/25 08:13:20 jorma Exp $
/**
   \file SimpleGeometric.h

   \brief Declarations and definitions of class SimpleGeometric
   
   SimpleGeometric.h contains declarations and definitions of 
   class the SimpleGeometric, which
   is a class that calculates some simple geometric descriptors.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.5 $
   $Date: 2016/10/25 08:13:20 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _SimpleGeometric_
#define _SimpleGeometric_

#include "Feature.h"

namespace picsom {
/// A class that performs the extractionof a few simple geometric descriptors.
class SimpleGeometric : public Feature {

 public:

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  SimpleGeometric(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  SimpleGeometric(const string& img, const string& seg,
	      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  SimpleGeometric(bool) : Feature(false) {}

  // ~SimpleGeometric();

  virtual Feature *Create(const string& s) const {
    return (new SimpleGeometric())->SetModel(s);
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
  
  virtual bool CalculateOneFrame(int f) { return CalculateCommon(f, true); }
      
  virtual bool CalculateOneLabel(int f, int l) {
    return CalculateCommon(f, false, l); }

  virtual string TargetType() const { return "segment"; }

  virtual treat_type DefaultWithinFrameTreatment() const {
    return treat_concatenate; }
  
  virtual treat_type DefaultBetweenFrameTreatment() const {
    return treat_separate; }
  
  virtual treat_type DefaultBetweenSliceTreatment() const {
    return treat_separate; }
  
  virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

  // copied from EdgeHist

  virtual string Name()          const { return "geom"; }

  virtual string LongName()      const { return "siple geometric features"; }

  virtual string ShortText()     const {
    return "Location, extent and log(w/h) of a region."; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5"); // does not make sense for fixed regions
    return centerdiag; 
  }

  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  // this is still just a placeholder...
  virtual string  NeededSegmentation() const { return "any"; }

  virtual featureVector getRandomFeatureVector() const{
    return vector<float>(4);
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
    cox::tictac::func tt(tics, "SimpleGeometric::CalculateCommon");

    map<int,int> posX,posY,maxX,maxY,minX,minY,ctr;
    

    EnsureImage();

    int width = Width(true), height = Height(true);

    if (FrameVerbose())
      cout << "SimpleGeometric::CalculateCommon(" << all << "," << l << "), wxh="
	   << width << "x" << height << "=" << width*height << endl;

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++) {

	vector<int> svec = SegmentVector(f, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  int s = svec[j];
	  if (s<0){
	    //	  cerr << "(x,y)=(" <<x<<","<<y<<"), s="<<s<<endl; 
	    throw "segment label<0";
	  }
	  
	  ctr[s]++;
	  posX[s] += x;
	  posY[s] += y;
	  if(minX.count(s)==0){
	    minX[s]=maxX[s]=x;
	    minY[s]=maxY[s]=y;
	  }
	  else{
	    if(x<minX[s]) minX[s]=x;
	    if(x>maxX[s]) maxX[s]=x;
	    if(y<minY[s]) minY[s]=y;
	    if(y>maxY[s]) maxY[s]=y;
	  }
	
	}
      }

    for(map<int,int>::iterator it=ctr.begin();
	it!=ctr.end(); it++){
      int i= DataIndex(it->first, true);

      int s=it->first;
      
      float x=posX[s]/((float)ctr[s]);
      float y=posY[s]/((float)ctr[s]);

      x /= width;
      y /= height;

      float ext=sqrt(ctr[s]/((float)width*height));

      float w=maxX[s]-minX[s]+1;
      float h=maxY[s]-minY[s]+1;

      featureVector v;
      v.push_back(x);
      v.push_back(y);
      v.push_back(ext);
      v.push_back(log(w/h));

      if(i>=0){
	((VectorData*)GetData(i))->setVector(v);
      }
    }

    return true;
  }

public:
  virtual featureVector CalculateRegion(const Region& r) {


    int width = Width(true), height = Height(true);

    int posX=0,posY=0,maxX=0,maxY=0,minX=0,minY=0,ctr=0;

    const coordList *lptr=r.accessList();
    coordList::const_iterator it;

    if(lptr==NULL){ // the region doesn't support accessing through pointer
      coordList l=r.listCoordinates();
      for(it=l.begin();it!=l.end();it++){
	ctr++;
	posX += it->x;
	posY += it->y;
	if(ctr==1){
	  minX=maxX=it->x;
	  minY=maxY=it->y;
	}
	else{
	  if(it->x<minX) minX=it->x;
	  if(it->x>maxX) maxX=it->x;
	  if(it->y<minY) minY=it->y;
	  if(it->y>maxY) maxY=it->y;
	}
      }
    } else
      for(it=lptr->begin();it!=lptr->end();it++){
	ctr++;
	posX += it->x;
	posY += it->y;
	if(ctr==1){
	  minX=maxX=it->x;
	  minY=maxY=it->y;
	}
	else{
	  if(it->x<minX) minX=it->x;
	  if(it->x>maxX) maxX=it->x;
	  if(it->y<minY) minY=it->y;
	  if(it->y>maxY) maxY=it->y;
	}

      }

    float x=posX/((float)ctr);
    float y=posY/((float)ctr);
    
    x /= width;
    y /= height;
    
    float ext=sqrt(ctr/((float)width*height));
    
    float w=maxX-minX+1;
    float h=maxY-minY+1;
    
    featureVector v;
    v.push_back(x);
    v.push_back(y);
    v.push_back(ext);
    v.push_back(log(w/h));
    
    return v;
  }

  virtual bool moveRegion(const Region &/*r*/, int /*from*/, int /*to*/) {
    throw string("SimpleGeometric::moveRegion() not implemented");
  }

  virtual bool ProcessOptionsAndRemove(list<string>&); 

protected:
 

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
