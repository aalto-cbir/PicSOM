// -*- C++ -*- 	$Id: SampleColour.h,v 1.6 2008/10/31 09:44:27 jorma Exp $
/**
   \file SampleColour.h

   \brief Declarations and definitions of class SampleColour
   
   SampleColour.h contains declarations and definitions of class the SampleColour, which
   is a class that performs the average colour value feature extraction.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.6 $
   $Date: 2008/10/31 09:44:27 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _SampleColour_
#define _SampleColour_

#include "Colour.h"

namespace picsom {
/// A class that performs the average colour value  extraction.
class SampleColour : public Colour {



 public:
  
  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  SampleColour(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  SampleColour(const string& img, const string& seg,
	       const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  SampleColour(bool) : Colour(false) {}

  // ~SampleColour();

  virtual Feature *Create(const string& s) const {
    return (new SampleColour())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new SampleColourData(t, n, this);
  }

  virtual string Name()          const { return "samplecolour"; }

  virtual string LongName()      const { return "samples of pixel colour"; }

  virtual string ShortText()     const {
    return "Samples of pixel colour values in the specified colour space."; }

  virtual const string& DefaultZoningText() const {
    static string all("1");
    return all; 
  }

  Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual int printMethodOptions(ostream&) const;

  protected:

  virtual bool ProcessOptionsAndRemove(list<string>&); 

  virtual vector<string> UsedDataLabels() const {
    int i;
    
    vector<string> ret;
    for (i=0; i<sampleCount; i++) {
      char tmp[100];
      sprintf(tmp, "%d", i);
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
  virtual bool CalculateCommon(int /* f*/, bool all, int l = -1) {
    EnsureImage();
    if (FrameVerbose())
      cout << "SampleColour::CalculateCommon(" << all << "," << l << "), wxh="
	   << Width() << "x" << Height() << endl;

    for (int s=0; s<sampleCount;s++){
      int x=rand()*(Width()-1)/RAND_MAX;
      int y=rand()*(Height()-1)/RAND_MAX;

      int i = -1;

      try {
	i = DataIndex(s);
      }
      catch (const string& str) {
	if (str.find(") not found") != string::npos)
	  continue;
      }

      if (i<0)
	continue;

      if (all || s==l)
	  CalculateOnePixel(x, y, (PixelBasedData*)GetData(i)); 
      }

    calculated = true;

    return true;
  }

  /// The SampleColourData objects stores the data needed for the feature calculation
  class SampleColourData : public ColourData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */

    SampleColourData(pixeltype t, int n,const Feature *p) : ColourData(t, n,p) 
    {}

    /// Defined because we have virtual member functions...
    virtual ~SampleColourData() {}
    
    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "SampleColourData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */

    /** Adds data from one pixel to the internal storage
	\param v the data vector to add
    */
    
    virtual void AddPixel(const vector<int>&, int, int) {
      throw "SampleColourData::AddPixel(vector<int>) not implemented";
    }
    
    virtual void AddPixel(const vector<float>& v, int x, int y) {
      sum = v; // overwrites the existing value

      ((SampleColour*)Parent())->ColourConversion(sum);
      
      if(isnan(sum[0]) || isnan(sum[1]) || isnan(sum[2]))
	throw string("colour conversion returned nan");

      if (PixelVerbose())
	cout << "SampleColourData::AddPixel(vector<float>)" << endl;
      PixelBasedData::AddPixel(v, x, y);
    }

    /** Removes data from one pixel to the internal storage
	\param v the data vector to remove
    */
    virtual void RemovePixel(const vector<int>& ) {
      throw "SampleColourData::RemovePixel(vector<int>) not implemented";
    }	

    virtual void RemovePixel(const vector<float>&) {
      throw "SampleColourData::RemovePixel(vector<float>) not implemented";
    }

    virtual featureVector Result(bool azc) const {
      if (!azc && !Count())
	throw "SampleColourData::Result() : count==0";

      if (Feature::LabelVerbose())
	cout << "SampleColourData::Result(), count=" << Count();

      for (size_t i=0; i<sum.size(); i++) {
	if (Feature::LabelVerbose())
	  cout << " " << sum[i];
      }

      if (Feature::LabelVerbose())
	cout << endl;

      return sum;
    }

  };
 public:
  int sampleCount;
};
}
#endif

// Local Variables:
// mode: font-lock
// End:
