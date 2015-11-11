// -*- C++ -*- 	$Id: NGram.h,v 1.20 2013/06/26 07:13:49 jorma Exp $
/**
   \file NGram.h

   \brief Declarations and definitions of class NGram
   
   NGram.h contains declarations and definitions of class the NGram, which
   is a class that performs the character n-gram value feature extraction
   for text objects.
  
   \author Hannes Muurinen <hannes.muurinen@hut.fi>
   $Revision: 1.20 $
   $Date: 2013/06/26 07:13:49 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo Perhaps something?
*/
#ifndef _NGram_
#define _NGram_

#include "TextBased.h"
#include <gcrypt.h>

#ifndef _HASH_ALGORITHM_
#define _HASH_ALGORITHM_ GCRY_MD_SHA1
#endif // _HASH_ALGORITHM_

#define NGRAM_DEFAULT_FEATURE_VECTOR_SIZE 1024 // any n*256 will do, 
                                               // where n is positive integer

#define NGRAM_DEFAULT_N 3

#define NGRAM_MODE_WORD 1
#define NGRAM_MODE_CHAR 2
#define NGRAM_DEFAULT_MODE NGRAM_MODE_CHAR

namespace picsom {
/// A class that performs the ngram feature extraction.
class NGram : public TextBased {
 public:
  /// definition for the current data type
  typedef wchar_t datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param obj initialise to this object
      \param opt list of options to initialise to
  */
  NGram(const string& obj = "", const list<string>& opt = (list<string>)0) {
    ngram_n = NGRAM_DEFAULT_N;
    ngram_vector_length = NGRAM_DEFAULT_FEATURE_VECTOR_SIZE;
    ngram_hash_algorithm = _HASH_ALGORITHM_;
    ngram_mode = NGRAM_DEFAULT_MODE;
    ngram_skip_space_count = -1;
    ngram_space_count = -1;
    ngram_normalize = true;
    
    Initialize(obj, "", NULL, opt);
  }

  /// This constructor doesn't add an entry in the methods list.
  NGram(bool) : TextBased(false) { 
    ngram_n = NGRAM_DEFAULT_N; 
    ngram_vector_length = NGRAM_DEFAULT_FEATURE_VECTOR_SIZE;
    ngram_hash_algorithm = _HASH_ALGORITHM_;
    ngram_mode = NGRAM_DEFAULT_MODE;
    ngram_skip_space_count = -1;
    ngram_space_count = -1;
    ngram_normalize = true;
  }

  //~NGram() {}

  virtual Feature *Create(const string& s) const {
    return (new NGram())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype, int n, int) const {
    //    return new NGramData(t, n,this);
    return new NGramData(DefaultPixelType(), n, this, TextData(), 
			 ngram_n, ngram_vector_length,
			 ngram_hash_algorithm,
			 ngram_mode, ngram_space_count,
			 ngram_skip_space_count, ngram_normalize);
  }

  virtual void deleteData(void *p){
    delete (NGramData*)p;
  }


  virtual string Name()          const { return "ngram"; }

  virtual string LongName()      const { return "n-grams"; }

  virtual string ShortText()     const {
    return 
      "A text feature calculated using SHA-1 sums of character/word n-grams.";}

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const { return featureVector(); }

  virtual string OptionSuffix() const {
    string opt = "-";
    if (ngram_mode==NGRAM_MODE_WORD)
      opt += "w";
    if (ngram_n!=3) {
      stringstream ss;
      ss << ngram_n;
      opt += ss.str();
    }
    return opt!="-" ? opt : "";
  }

  virtual int printMethodOptions(ostream&) const;

  virtual treat_type DefaultWithinTreatment() const {
    return treat_concatenate; }

  virtual treat_type DefaultBetweenTreatment() const {
    return treat_separate; }

  virtual pixeltype DefaultPixelType() const { return pixel_undef; }

  virtual int FeatureVectorSize(bool) const {
    return ngram_vector_length;
  }

protected:  

  ///
  virtual bool ProcessOptionsAndRemove(list<string>&);


public:
  /// The NGramData objects store the data needed for the feature calculation
  class NGramData : public TextBasedData {
  public:

    /// definition for the data type used in hash digests
    typedef unsigned char shadigest_t;

    /// definition for the vector type used to store hash digests
    typedef vector<shadigest_t> shadigest_vect_t;

    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    NGramData(pixeltype t, int n, const Feature *p,
	      const textdata& txt, unsigned int ng_n, unsigned int ng_v,
	      int ng_h, int ng_m, int ng_sc, int ng_ssc, bool ng_norm) 
      : TextBasedData(t, n, p, txt) {
	ngram_n = ng_n;
	ngram_vector_length = ng_v;
	ngram_hash_algorithm = ng_h;
	ngram_mode = ng_m;
	ngram_space_count = ng_sc >= 0 ? ng_sc : ngram_n-1; // def: ngram_n-1
	ngram_skip_space_count = ng_ssc >= 0 ? ng_ssc : ngram_n;
	ngram_normalize = ng_norm;
    }

    /// Defined because we have virtual member functions...
    ~NGramData() {}

    /// Calculates the hash digest (type specified by ngram_hash_algorithm).
    /// \param st the string the hash value is calculated for
    /// \return a vector of bytes
    static shadigest_vect_t hashsum(const string& st, gcry_md_algos);

    shadigest_vect_t hashsum(const string& st) const;

    shadigest_vect_t hashsum(const char *st) {
      return hashsum(string(st));
    }
    
    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "NGramData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return ngram_vector_length; }

    /// += operator 
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "NGramData::operator+=() not defined"; }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool) const {
      if(ngram_mode == NGRAM_MODE_CHAR)
	return CharacterNGramResult();
      else if(ngram_mode == NGRAM_MODE_WORD)
	return WordNGramResult();
      else
	throw "NGram::Result() : invalid ngram_mode";
    }

  private:

    /// Converts any number of white-space to a specified number of spaces
    /// between the words.
    vectortype CleanUpWhiteSpace(const vectortype &original, 
				 int between_space_count) const;


    /// Does the given string contain n successive spaces.
    bool ContainsNSuccessiveSpaces(char* buf, unsigned int n) const;


    /// Calculates the character NGram feature vector.
    featureVector CharacterNGramResult() const;


    /// A function that calculates results for word NGrams.
    featureVector WordNGramResult() const;


    unsigned int ngram_n;
    unsigned int ngram_vector_length;
    int ngram_hash_algorithm;
    int ngram_mode;
    int ngram_space_count;
    int ngram_skip_space_count;
    int ngram_normalize;
  }; // class NGramData

 private:
  /// The "n" of the NGram -- default is NGRAM_DEFAULT_N
  unsigned int ngram_n;

  /// The feature vector length (n*256, where n==positive integer)
  unsigned int ngram_vector_length;

  /// The used hash algorithm
  int ngram_hash_algorithm;

  /// The mode (word/char)
  int ngram_mode;

  /// How many spaces are used to separate words
  int ngram_space_count;

  /// How many successive spaces the char NGram can contain before 
  /// it is discarded.
  int ngram_skip_space_count;

  /// Set to true to get vectors normalized by the total number of ngrams
  bool ngram_normalize;

}; // class NGram
}
#endif

// Local Variables:
// mode: font-lock
// End:
