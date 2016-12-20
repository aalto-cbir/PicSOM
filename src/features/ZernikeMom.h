// -*- C++ -*- $Id: ZernikeMom.h,v 1.5 2016/10/25 08:14:25 jorma Exp $

/**
   \file ZernikeMom.h

   \brief Declarations and definitions of class ZernikeMom
   
   \author Ville Viitaniemi <Ville.Viitaniemi@hut.fi>
   $Revision: 1.5 $
   $Date: 2016/10/25 08:14:25 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _ZernikeMom_h_
#define _ZernikeMom_h_

#include <Feature.h>
#include <Segmentation.h>

namespace picsom {
class ZernikeMom : public Feature {
public:

  enum postproc_t {none,absolutes};

  class Param{
  public:
    int maxorder;
    bool multiply_by_n2;
    postproc_t postprocessing;
  };

  virtual bool Incremental() const { return false; }

 ZernikeMom(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 

  /// This constructor doesn't add an entry in the methods list.
  ZernikeMom(bool) : Feature(false) {}

  virtual treat_type DefaultWithinTreatment() const {
    return treat_concatenate; }

  virtual treat_type DefaultBetweenTreatment() const {
    return treat_separate; }

  virtual pixeltype DefaultPixelType() const { return pixel_rgb; } 

  virtual Feature *Create(const string& s) const {
    return (new ZernikeMom())->SetModel(s);
  }


  virtual string Name()          const { return "zernikemom"; }

  virtual string LongName()      const { return "zernike moments"; }

  virtual string ShortText()     const {
    return "Zernike moments of the segmentation mask "; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5");
    return centerdiag; 
  }

  virtual string Version() const;

  virtual string TargetType() const { return "segment"; }

  virtual featureVector getRandomFeatureVector() const;

  virtual int printMethodOptions(ostream&) const;

  //virtual bool CalculateAndPrint();

  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  // do-nothing  overriders for pure virtual functions

  virtual bool CalculateOneLabel(int frame, int label);

  virtual bool CalculateOneFrame(int frame);

  virtual featureVector CalculateRegion(const Region& /*r*/) {
    featureVector fV;
    return fV;
  }

  virtual int FeatureVectorSize(bool ) const {
    switch(param.postprocessing){
    case none:
      return 2*NumberOfCoefficients(param.maxorder);
    case absolutes:
      return NumberOfCoefficients(param.maxorder);
    }
    return 0;
  }


  static int NumberOfCoefficients(int order){
    int ret=0;
    for(int a=0;a<=order;a++)
      ret += a/2+1;

    return ret;
  }

  virtual treat_type DefaultWithinFrameTreatment() const{
    return treat_separate;
  }

  /** Returns the default treatment between frames
      \return default treatment between frames
  */
  virtual treat_type DefaultBetweenFrameTreatment() const{
    return treat_separate;
  }
  

  /** Returns the default treatment between slices
      \return default treatment between slices
  */
  virtual treat_type DefaultBetweenSliceTreatment()const{
    return treat_separate;
  }

 class ZernikeMomData : public Data {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
   ZernikeMomData(pixeltype t, int n,const Feature *p) : Data(t, n,p) { 
     Zero();
     v=featureVector(((ZernikeMom*)p)->FeatureVectorSize(true));
   }

   virtual ~ZernikeMomData(){}
   virtual void Zero() {v=featureVector(Length());}
   virtual string Name() const { return "ZernikeMomData"; }
   virtual int Length() const { return v.size(); }
   virtual Data& operator+=(const Data &) {
     throw "ZernikeMomData::operator+=() not defined"; 
   }

   virtual void SetData(const vector<double> &real,
			const vector<double> &imag,const Param &p);
   
   virtual featureVector Result(bool) const {
     return v;
   }

 private:
   featureVector v;

 };



protected:

  /// This constructor is called by inherited class constructors.
  ZernikeMom() {}  // automatic default constructor should do...
 
  virtual vector<string> UsedDataLabels() const {
    vector<pair<int,string> > labs = SegmentData()->getUsedLabelsWithText();
    vector<pair<int,string> >::iterator i;
    
    if (LabelVerbose()) {
      cout << "ZernikeMom::UsedDataLabels() : " << endl;
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
      ShowDataLabels("ZernikeMom::UsedDataLabels", ret, cout);
    
    return ret;
  }

  virtual bool ProcessOptionsAndRemove(list<string>&); 


  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new ZernikeMomData(t, n,this);
  }

  virtual void deleteData(void *p){
    delete (ZernikeMomData*)p;
  }

  double R(int n,int m,double roo);

  double Coefficient(int s,int m,int n){
    const int u=(m+n)/2;
    const int l=(n-m)/2;
    return ((s&1)?(-1):(+1))*factorial[n-s]/(factorial[s]*factorial[u-s]*factorial[l-s]);
  }

  void TabulateFactorials(int maxorder);

  void FindSegmentNormalisations();

  void FindSegmentNormalisations(int label);

  double XAvg(int label){return x_avg[label];}

  double YAvg(int label){return y_avg[label];}

  double Scaling(int label){return scaling[label];}

  vector<int> factorial;
  map<int,double> x_avg;
  map<int,double> y_avg;
  map<int,double> scaling;

public: // this is needed to keep alpha compiler happy

  Param param;


};
}
#endif // _ZernikeMom_h_

// Local Variables:
// mode: font-lock
// End:
