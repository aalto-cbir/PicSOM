// -*- C++ -*- 	$Id: Raw.h,v 1.17 2006/06/16 10:52:10 vvi Exp $

#ifndef _Raw_
#define _Raw_

#include "RegionBased.h"

namespace picsom {
/// Feature calculation on raw data, with or without normalization.
class Raw : public RegionBased {
 public:
  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  Raw(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  Raw(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

  /** Constructor
      \param s segmentation object
      \param opt list of options to initialise to
  */
  Raw(segmentfile *s, const list<string>& opt = (list<string>)0) {
    Initialize("", "", s, opt);
  }

  /// Constructor
  Raw(bool) : RegionBased(false) {}

  virtual Feature *Create(const string& s) const {
    return (new Raw())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype pt, int n, int i) const {
    return new RawData(pt, n, RegionSize(i), this);
  }

  virtual void deleteData(void *p){
    delete (RawData*)p;
  }

  virtual string Name()          const { return "raw"; }

  virtual string LongName()      const { return "raw data"; }

  virtual string ShortText()     const {
    return "Raw data, with or without normalization."; }

  virtual string Version() const;

  virtual string TargetType() const { return "image"; }

  // this is still just a placeholder...
  virtual string  NeededSegmentation() const { return "region.box"; }

  virtual featureVector getRandomFeatureVector() const { return featureVector(); }

  virtual int printMethodOptions(ostream&) const { return 0; }

  virtual bool ProcessRegion(const Region&, const vector<float>&, Data*);

  // virtual int DataElementCount() const { return NumberOfRegions(); }
  // Feature::DataElementCount() should do (vvi), this one doesn't

 protected:  
  virtual bool ProcessOptionsAndRemove(list<string>&);

  /// Data object for raw data
  class RawData : public RegionBasedData {
  public:
    friend class Raw;

    /** Constructor
	\param pt sets the pixeltype of the data
	\param n sets the extension (jl?)
	\param l length of data vector
    */
    RawData(pixeltype pt, int n, int l,const Feature *p) : RegionBasedData(pt, n,p),
					  datavec(l*DataUnitSize()) { Zero(); }

    /// Because we have virtual member functions...
    virtual ~RawData() {}

    /// Zeros the internal data
    virtual void Zero() { RegionBasedData::Zero(); }

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "RawData"; }

    /** Returns the size of the internal data
	\return data length
    */
    virtual int Length() const { return datavec.size(); }

    /// += operator
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "RawData::operator+=() not defined"; }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool /*allow_zero_count*/) const {
      if (Feature::LabelVerbose())
	cout << "RawData::Result()" << endl;

      return datavec;
    }
    
  protected:
    /// Stores the internal data
    vector<float> datavec;
  };

  /// jl
  string conversion;
};
}
#endif

// Local Variables:
// mode: font-lock
// End:
