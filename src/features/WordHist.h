// -*- C++ -*- 	$Id: WordHist.h,v 1.16 2018/01/30 09:54:21 jormal Exp $

/**
   \file WordHist.h

   \brief Declarations and definitions of class WordHist
   
   WordHist.h contains declarations and definitions of class the WordHist, 
   which is a class that performs word nhistogram feature extraction
   for text objects.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.16 $
   $Date: 2018/01/30 09:54:21 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo Perhaps something?
*/

#ifndef _WordHist_h_
#define _WordHist_h_

#include "TextBased.h"

namespace picsom {

  // A class that performs the wordhist feature extraction.
  class WordHist : public TextBased {
  public:

    // definition for the current data type
    typedef wchar_t datatype;  // this should be selectable in run time...
  
    // definition for the current data vector type
    typedef vector<datatype> vectortype;

    //
    class dict_entry {
    public:
      dict_entry(const string& w, size_t i, float _d, float _n, float _c,
		 float td, float tn) :
	word(w), orig_idx(i), d(_d), n(_n), c(_c), tot_d(td), tot_n(tn),
	index(-1), stop(false), unused(false) {
	idf     = d ? tot_d/d  : 0;
	log_idf = d ? log(idf) : 0;
      }
	
      string word;     //< the "word" aka "term"

      size_t orig_idx; //< original row in the dict file

      float d;         //< the number of documents where "word" occurs

      float n;         //< the total number of times "word" occurs

      float c;         //< the total number of words in documents with "word"

      float tot_d;     //< the total number of documents in the collection

      float tot_n;     //< the total number of words in the collection

      float idf;       //< the inverse document frequency

      float log_idf;   //< and its logarithm

      ///
      int index;

      ///
      bool stop;

      ///
      bool unused;

      ///
      vector<float> v;

    }; // class dict_entry

    //
    typedef map<string,dict_entry> dict_t;

    class w2v_data {
    public:
      ///
      w2v_data(const string& = "");

      ///
      bool read(const string&);

      ///
      size_t size() const {
	return vec_r.size() ? vec_r[0].size() : 0;
      }

      ///
      const vector<float>& to_vec_raw(const string& word) const {
	static const vector<float> empty;
	auto i = wmap.find(word);
	return i==wmap.end() ? empty : vec_r[i->second];
      }

      ///
      const vector<float>& to_vec_norm(const string& word) const {
	static const vector<float> empty;
	auto i = wmap.find(word);
	return i==wmap.end() ? empty : vec_n[i->second];
      }

      ///
      string file;

      ///
      vector<string> vocab;

      ///
      vector<vector<float> > vec_r;

      ///
      vector<vector<float> > vec_n;

      ///
      map<string,size_t> wmap;
    };

    ///
    static map<string,w2v_data> w2v_map;

    ///
    static const vector<float>& word2vec_raw(const string&,
					     const string& = "");

    ///
    static const vector<float>& word2vec_norm(const string&,
					      const string& = "");

    ///
    static int word2vec_index(const string&, const string& = "");

    ///
    static const w2v_data *word2vec_find(const string&);

    ///
    static const vector<string>& word2vec_vocabulary(const string&);

    /** Constructor
	\param obj initialise to this object
	\param opt list of options to initialise to
    */
    WordHist(const string& obj = "",
	     const list<string>& opt = (list<string>)0) {
      vect_size = top_set_size = 0;
      w2v = NULL;
      linedump = zero2random = false;
      Initialize(obj, "", NULL, opt);
    }
    
    WordHist(bool) : TextBased(false) { }
	
    virtual bool ReInitializable() const { return true; }
    
    virtual bool ReInitialize() {
      // something ?
      return Feature::ReInitialize();
    }

    virtual Feature *Create(const string& s) const {
      return (new WordHist())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype, int n, int) const {
      //    return new WordHistData(t, n,this);
      return new WordHistData(DefaultPixelType(), n, this, TextData());
    }

    virtual void deleteData(void *p){
      delete (WordHistData*)p;
    }

    virtual bool IsSparseVector() const;

    virtual string Name()         const { return "wordhist"; }

    virtual string LongName()     const { return "word histogram"; }

    virtual string ShortText()    const {
      return "A text feature of word histogram.";
    }

    virtual string Version() const;

    virtual featureVector getRandomFeatureVector() const {
      return featureVector();
    }

    virtual string OptionSuffix() const {
      string opt;

      if (DoStem())
	opt = "-s";

      if (WeightType()) {
	opt += "-";
	opt += WeightTypeName(WeightTypeTf()).substr(2);
	opt += WeightTypeName(WeightTypeIdf()).substr(3);
      }

      return opt;
    }

    virtual int printMethodOptions(ostream&) const;

    virtual treat_type DefaultWithinTreatment() const {
      return treat_concatenate; }

    virtual treat_type DefaultBetweenTreatment() const {
      return treat_separate; }

    virtual pixeltype DefaultPixelType() const { return pixel_undef; }

    virtual int FeatureVectorSize(bool) const {
      if (FileVerbose())
	cout << "WordHist::FeatureVectorSize() returns " << vect_size << endl;

      return vect_size;
    }

    virtual textdata::unit TextUnit() const { return textdata::unit_word; }

    virtual bool TextPreProcess(int, bool, int);

    virtual bool TextPostProcess(int, bool, int);

    virtual bool PrintingEnabled() const {
      return dictfile!="" || w2vfile!="";
    }

    static const string& AllWordsLabel() {
      static const string star = "*";
      return star;
    }

    //
    bool ReadW2vFile();

    // 
    bool ReadDictFile();

    // 
    bool ReadStopFile();

    // 
    bool FinalizeWordIndex();

    //
    bool AddDictEntry(const string& w, size_t i,
		      float d, float n, float c, float td, float tn) {
      dict.insert(dict_t::value_type(w, dict_entry(w, i, d, n, c, td, tn)));

      return true;
    }

  protected:  

    //
    virtual bool ProcessOptionsAndRemove(list<string>&);
    
    // The WordHistData -- the data needed for the feature calculation
    class WordHistData : public TextBasedData {
    public:

    /** Constructor
        \param t sets the pixeltype of the data
        \param n sets the extension (jl?)
    */
      WordHistData(pixeltype t, int n, const Feature *p,
		   const textdata &txt) : TextBasedData(t, n, p, txt) {

      }

      // Defined because we have virtual member functions...
      ~WordHistData() {}

      /** Returns the name of the feature
          \return feature name
      */    
      virtual string Name() const { return "WordHistData"; }

      /** Returns the lenght of the data contained in the object
          \return data length
      */
      virtual int Length() const { return P()->FeatureVectorSize(false); }

      // += operator 
      virtual Data& operator+=(const Data &) {
        // this should check that typeof(d) == typeof(*this) !!!
        throw "WordHistData::operator+=() not defined"; }

      /** Returns the result of the feature extraction
          \return feature result vector
      */
      virtual featureVector Result(bool azc) const {
	return P()->Result(*this, azc);
      }
      
      virtual void AddData(float c) { TextBasedData::AddData(c); }

      virtual void AddData(datatype c) { TextBasedData::AddData(c); }

      virtual void AddData(const string&);

      bool ShowCount(ostream&);

      // private:
      //
      WordHist *P() const { return (WordHist*)parent; }      

      //
      typedef map<string,int> count_t;

      count_t count;

    }; // class WordHistData

  public:
    //
    featureVector Result(const WordHistData&, bool /*azc*/) const;

  private:
    //
    int vect_size;

    //
    int top_set_size;

    //
    string w2vfile;

    //
    string dictfile;

    //
    string stopfile;

    //
    dict_t dict;

    //
    bool linedump;
    
    //
    bool zero2random;

    //
    const w2v_data *w2v;

  }; // class WordHist

} // namespace picsom

#endif // _WordHist_h_

// Local Variables:
// mode: font-lock
// End:
