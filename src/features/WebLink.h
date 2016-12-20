// -*- C++ -*- 	$Id: WebLink.h,v 1.13 2006/06/16 10:52:21 vvi Exp $
/**
   \file WebLink.h

   \brief Declarations and definitions of class WebLink
   
   WebLink.h contains declarations and definitions of class the WebLink
  
   \author Mats Sjöberg
   $Revision: 1.13 $
   $Date: 2006/06/16 10:52:21 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo Perhaps something?
*/
#ifndef _WebLink_
#define _WebLink_

#include "TextBased.h"
#include "textdata.h"
#include <gcrypt.h>

#define SHA1_ALGO GCRY_MD_SHA1
#define MD5_ALGO  GCRY_MD_MD5

#define WEBLINK_VECTOR_LENGTH 1024

namespace picsom {
/// A class that performs the weblink feature extraction.
class WebLink : public TextBased {
public:
  /// definition for the current data type
  typedef wchar_t datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param obj initialise to this object
      \param opt list of options to initialise to
  */
  WebLink(const string& obj = "", const list<string>& opt = (list<string>)0) {
    Initialize(obj, "", NULL, opt);
  }

  /// This constructor doesn't add an entry in the methods list.
  WebLink(bool) : TextBased(false) {}

  //~WebLink() {}

  virtual Feature *Create(const string& s) const {
    return (new WebLink())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype, int n, int) const {
    //    return new WebLinkData(t, n,this);
    return new WebLinkData(DefaultPixelType(), n, this, TextData());
  }

  virtual void deleteData(void *p){
    delete (WebLinkData*)p;
  }

  virtual bool IsSparseVector() const { return true; }

  virtual string Name()          const { return "weblink"; }

  virtual string LongName()      const { return "weblink"; }

  virtual string ShortText()     const {
    return 
      "Weblink bla bla...";}

  virtual string Version() const;

  virtual string TargetType() const { return "link"; }

  virtual featureVector getRandomFeatureVector() const { return featureVector(); }

  virtual int printMethodOptions(ostream&) const;

  virtual treat_type DefaultWithinTreatment() const {
    return treat_concatenate; }

  virtual treat_type DefaultBetweenTreatment() const {
    return treat_separate; }

  virtual pixeltype DefaultPixelType() const { return pixel_undef; }

  virtual int FeatureVectorSize(bool) const {
    return WEBLINK_VECTOR_LENGTH;
  }

protected:  

  ///
  virtual bool ProcessOptionsAndRemove(list<string>&);

public:
  /// The WebLinkData objects store the data needed for the feature calculation
  class WebLinkData : public TextBasedData {
  public:

    /// definition for the data type used in hash digests
    typedef unsigned char digest_t;

    /// definition for the vector type used to store hash digests
    typedef vector<digest_t> digest_vect_t;

    typedef pair<string, double> urlval_t;
    typedef vector< urlval_t > urlval_vect_t;

    /** Constructor
				\param t sets the pixeltype of the data
				\param n sets the extension (jl?)
    */
    WebLinkData(pixeltype t, int n,const Feature *p, const textdata &txt) 
      : TextBasedData(t, n, p, txt) { }

    /// Defined because we have virtual member functions...
    ~WebLinkData() {}

    /// Calculates the hash digest (type specified by weblink_hash_algorithm).
    /// \param st the string the hash value is calculated for
    /// \return a vector of bytes
    digest_vect_t hashsum(const string &st, int algo) const;

    digest_vect_t hashsum(char *st, int algo) {
      return hashsum(string(st),algo);
    }
    
    digest_vect_t hashsum(const char *st, int algo) const {
      return hashsum(string(st),algo);
    }
    
    /** Returns the name of the feature
				\return feature name
    */    
    virtual string Name() const { return "WebLinkData"; }

    /** Returns the lenght of the data contained in the object
				\return data length
    */
    virtual int Length() const { return WEBLINK_VECTOR_LENGTH; }

    /// += operator 
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "WebLinkData::operator+=() not defined"; }

    /** Returns the result of the feature extraction
				\return feature result vector
    */
    virtual featureVector Result(bool) const {
      return WebLinkResult();
    }

  private:
    /** Calculates the result from the web links
				\return feature result vector
    */
    featureVector WebLinkResult() const;
		
    featureVector UrlResult(string url, double weight=1.0,
														bool skip_orig_url=false) const;
		
    featureVector SumURLs(urlval_vect_t &urls) const;
		
    void ZeroFeatureVector(featureVector &v) const;

    /** Utility function that makes a printable hex-number string from
				a digest vector
				\param v the digest vector
				\return the resulting string
    */
    string DigestVectToString(digest_vect_t& v) const;

    /** Tests if the string is an URL
				\param s the string to test
				\return true if a valid URL
    */
    bool IsURL(const string& s) const;

    /** Sets indexes in vector according to a digest vector
				\param v vector to modify
				\param dig sha1 digest
    */
    void AddSha1Indexes(featureVector &v, const digest_vect_t& dig, double val) 
      const;

  }; // class WebLinkData

private:

}; // class WebLink
}
#endif

// Local Variables:
// mode: font-lock
// End:
