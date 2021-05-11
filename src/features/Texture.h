// -*- C++ -*- 	$Id: Texture.h,v 1.24 2008/10/31 09:44:28 jorma Exp $
/**
   \file Texture.h

   \brief Declarations and definitions of class Texture
   
   Texture.h contains declarations and definitions of class Texture,
   which is a template based feature that is calculated on the
   brightness of the 3x3 neightbouring pixels of each pixel in the
   image
  
   \author Mats Sjoberg <mats.sjoberg@hut.fi>
   $Revision: 1.24 $
   $Date: 2008/10/31 09:44:28 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _Texture_
#define _Texture_

#include "TemplateBased.h"

namespace picsom {

/** Texture is a template based feature that is calculated on the brightness
    of the 3x3 neightbouring pixels of each pixel in the image 
*/
class Texture : public TemplateBased {
public:
  /** Constructor 
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  Texture(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor 
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  Texture(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

  /** Constructor
      \param s segmentation object
      \param opt list of options to initialise to
  */
  Texture(segmentfile *s, const list<string>& opt = (list<string>)0) {
    Initialize("", "", s, opt);
  }

  /// Constructor
  Texture(bool) : TemplateBased(false) {}

  virtual Feature *Create(const string& s) const {
    return (new Texture())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype pt, int n, int i) const {
    return new TextureData(pt, n, i, GetTemplateLength(), this); }


  virtual void deleteData(void *p){
    delete (TextureData*)p;
  }


  virtual string Name()          const { return "texture"; }

  virtual string LongName()      const { return "texture neighborhood"; }

  virtual string ShortText()     const {
    return "Texture feature based on brightness of neighboring pixels."; }

  virtual const string& DefaultZoningText() const {
    static string centerdiag("5");
    return centerdiag;
  }

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const { return featureVector(); }

  /*
  virtual bool probeTileFeatures(int x1,int y1, int x2, int y2,
				 featureVector &v,int probelabel=-1,
                                 bool saveLabeling=false);
  */

  virtual const templatetype& GetTemplate() const { 
    static const templatetype& t = ThreeSquare(); 
    return t;
  }

  //
  virtual void ZeroPerFrameData() {
    TemplateBased::ZeroPerFrameData();
    yArray.clear();
  }
  
 //  virtual void AddPixel(int s, const vector<float>& v) {
//     if (PixelVerbose())
//       cout << "Texture::AddPixel(" << s << ", vector<float>)" << endl;
//     ((TextureData*)GetData(s))->AddPixel(v);
//   }
      
//   virtual void AddPixel(int s, const vector<int>& v) {
//     if (PixelVerbose())
//       cout << "Texture::AddPixel(" << s << ", vector<int>)" << endl;
//     ((TextureData*)GetData(s))->AddPixel(v);
//   }

protected:  

  /** Makes yArray which is needed internally
      \return true if ok
  */
  virtual bool MakeYArray();

  /// The TextureData objects contain data for each segment
  class TextureData : public TemplateBasedData {
  public:
    /** Constructor
	\param pt sets the pixeltype of the data
	\param n sets the extension (jl?)
	\param i sets the data index (segment) this data object corresponds to
	\param t the template length
    */
    TextureData(pixeltype pt, int n, int i, int t,const Feature *p) : 
      TemplateBasedData(pt, n,p), dindex(i), temp_num(t) {
      Zero();
    }

    /// Because we have virtual member functions...
    virtual ~TextureData() {}

    /// Zeros the internal storage
    virtual void Zero() {
      TemplateBasedData::Zero();
      texsum = texsum_t(temp_num);
    }

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "TextureData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return temp_num; }
    
    /** Adds data from one pixel to the internal storage
	\param v the data vector to add
    */
    virtual void AddPixel(const vector<int>& v, int x, int y) {
      if (v.size()!=texsum.size())
	throw "TextureData::AddPixel(vector<int>) size mismatch";
      transform(texsum.begin(), texsum.end(), v.begin(), texsum.begin(),
		plus<texsum_value_t>());
      if (PixelVerbose())
	cout << "TextureData::AddPixel(vector<int>)" << endl;
      TemplateBasedData::AddPixel(v, x, y);
    }
    
    /** Removes data for one pixel from the internal storage
	\param v the data vector to remove
    */

    virtual void RemovePixel(const vector<int>& v) {
      if (v.size()!=texsum.size())
	throw "TextureData::RemovePixel(vector<int>) size mismatch";
      transform(texsum.begin(), texsum.end(), v.begin(), texsum.begin(),
		minus<texsum_value_t>());
      if (PixelVerbose())
	cout << "TextureData::RemovePixel(vector<int>)" << endl;
      TemplateBasedData::RemovePixel(v);
    }
    
    /** Adds data from one pixel to the internal storage
	\param v the data vector to add
    */
    virtual void AddPixel(const vector<float>& v, int x, int y) {
      if (v.size()!=texsum.size())
	throw "TextureData::AddPixel(vector<float>) size mismatch";
      transform(texsum.begin(), texsum.end(), v.begin(), texsum.begin(),
		plus<texsum_value_t>());
      if (PixelVerbose())
	cout << "TextureData::AddPixel(vector<float>)" << endl;
      TemplateBasedData::AddPixel(v, x, y);
    }

    /** Removes data for one pixel from the internal storage
	\param v the data vector to add
    */
    virtual void RemovePixel(const vector<float>& v) {
      if (v.size()!=texsum.size())
	throw "TextureData::RemovePixel(vector<float>) size mismatch";
      transform(texsum.begin(), texsum.end(), v.begin(), texsum.begin(),
		minus<texsum_value_t>());
      if (PixelVerbose())
	cout << "TextureData::RemovePixel(vector<float>)" << endl;
      TemplateBasedData::RemovePixel(v);
    }
    
    /// implementation of the += operator
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "TextureData::operator+=() not defined"; }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool) const;
    
  protected:
    // Type of each component of texture sum
    typedef double texsum_value_t;

    /// Type of texture sum
    typedef vector<texsum_value_t> texsum_t;

    /// texture sum
    texsum_t texsum;

    /// dataindex of segment
    int dindex;

    /// size of template
    int temp_num;
  };

  virtual void CalculateOnePixel(int x, int y, PixelBasedData *d) {
    if (yArray.empty())
      MakeYArray();
    
    TemplateBased::CalculateOnePixel(x,y,d);
  }

  virtual float ProcessPoints(int x, int y, int nx, int ny, 
			      TemplateBasedData * d, int i);

protected:

  /// vector needed in feature calculation
  vector<double> yArray;
};
}
#endif

// Local Variables:
// mode: font-lock
// End:
