// -*- C++ -*- $Id: ContourFD.h,v 1.11 2016/10/25 08:10:51 jorma Exp $

/**
   \file ContourFD.h

   \brief Declarations and definitions of class ContourFD
   
   \author Ville Viitaniemi <Ville.Viitaniemi@hut.fi>
   $Revision: 1.11 $
   $Date: 2016/10/25 08:10:51 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _ContourFD_h_
#define _ContourFD_h_

#include <Feature.h>
#include <Segmentation.h>

namespace picsom {
class ContourFD : public Feature {
public:

  enum postproc_t {none,rectify,absolutes};

  class Param{
  public:
    
    int number_of_coefficients;
    bool multiply_by_n2;
    bool normalise_translation;
    bool normalise_startpoint;
    bool normalise_rotation;
    bool normalise_scaling;
    bool normalise_xmirror;
    postproc_t postprocessing;
  };

  virtual bool Incremental() const { return false; }

 ContourFD(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 

  /// This constructor doesn't add an entry in the methods list.
  ContourFD(bool) : Feature(false) {}

  virtual treat_type DefaultWithinTreatment() const {
    return treat_concatenate; }

  virtual treat_type DefaultBetweenTreatment() const {
    return treat_separate; }

  virtual pixeltype DefaultPixelType() const { return pixel_rgb; } 

  virtual Feature *Create(const string& s) const {
    return (new ContourFD())->SetModel(s);
  }


  virtual string Name()          const { return "contourfd"; }

  virtual string LongName()      const { return "fourier descriptors of contours"; }

  virtual string ShortText()     const {
    return "Fourier descriptors of contours "; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5");
    return centerdiag; 
  }

  virtual string Version() const;

  virtual string TargetType() const { return "segment"; }

  // this is still just a placeholder...
  virtual string  NeededSegmentation() const { return "any"; }

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
    case rectify:
      return 4*param.number_of_coefficients+2;
    case absolutes:
      return 2*param.number_of_coefficients+2;
    }
    return 0;
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

  picsom::coordList *traceBorder(const std::set<picsom::coord> &cs, 
				const picsom::coord &seed,
				bool use_diagonals=true);

  bool HasNeighbourSeg(const picsom::coord &c,int dir,
		       const std::set<picsom::coord> &cs);
  int NeighbourCountSeg(const picsom::coord &c,
			bool use_diagonals,
			const std::set<picsom::coord> &cs);

 class ContourFDData : public Data {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
   ContourFDData(pixeltype t, int n,const Feature *p) : Data(t, n,p) { 
     Zero();
     v=featureVector(((ContourFD*)p)->FeatureVectorSize(true));
   }

   virtual ~ContourFDData(){}
   virtual void Zero() {v=featureVector(Length());}
   virtual string Name() const { return "ContourFDData"; }
   virtual int Length() const { return v.size(); }
   virtual Data& operator+=(const Data &) {
     throw "ContourFDData::operator+=() not defined"; 
   }

   virtual void Calculate(const coordList &l,const Param &p);

   void Normalise(featureVector &fV,const Param &p);

    virtual featureVector Result(bool) const {
      return v;
    }

 private:
   featureVector v;

 };



protected:

  /// This constructor is called by inherited class constructors.
  ContourFD() {}  // automatic default constructor should do...
 
  virtual vector<string> UsedDataLabels() const {
    vector<pair<int,string> > labs = SegmentData()->getUsedLabelsWithText();
    vector<pair<int,string> >::iterator i;
    
    if (LabelVerbose()) {
      cout << "ContourFD::UsedDataLabels() : " << endl;
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
      ShowDataLabels("ContourFD::UsedDataLabels", ret, cout);
    
    return ret;
  }

  virtual bool ProcessOptionsAndRemove(list<string>&); 


  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new ContourFDData(t, n,this);
  }

  virtual void deleteData(void *p){
    delete (ContourFDData*)p;
  }

public: // this is needed to keep alpha compiler happy

  Param param;

};
}
#endif // _ContourFD_h_

// Local Variables:
// mode: font-lock
// End:
