// -*- C++ -*- $Id: TextBased.h,v 1.35 2013/10/01 10:56:14 jorma Exp $

/**
   \file TextBased.h

   \brief Declarations and definitions of class TextBased
   
   TextBased.h contains declarations and definitions of class the
   TextBased, which is a base class for feature calculations that
   calculate features from text objects.
  
   \author Hannes Muurinen <hannes.muurinen@hut.fi>
   $Revision: 1.35 $
   $Date: 2013/10/01 10:56:14 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _TextBased_h_
#define _TextBased_h_

#include <Feature.h>
#include <textfile.h>
#include <PorterStemmer.h>

namespace picsom {
  /// Base class for text features
  class TextBased : public Feature {
  public:
    /// weighting alternatives
    enum weight_type {
      weight_undef = 0,
      tf_one       = 1, //< 
      tf_pure      = 2, //< term frequency
      tf_log       = 3, //< logarithm (1+term frequency)
      tf_div_all   = 4, //<
      tf_div_max   = 5, //<
      //
      idf_one      = 256*1, //<
      idf_pure     = 256*2, //<
      idf_log      = 256*3  //<
    };

    ///
    static weight_type WeightTypeName(const string&);

    ///
    static const string& WeightTypeName(weight_type);

    ///
    weight_type WeightType() const { return weight; }

    ///
    weight_type WeightTypeTf() const { return weight_type(weight%256); }

    ///
    weight_type WeightTypeIdf() const { return weight_type(weight/256*256); }

    ///
    void WeightType(weight_type w) throw(logic_error) {
      weight = w;
      int t = WeightTypeTf(), d = WeightTypeIdf()/256;
      if (t<1 || t>5 || d<1 || d>3) {
	stringstream ss;
	ss << w;
	throw logic_error("Not valid weight_type ["+ss.str()+"]");
      }
    }

    ///
    void WeightType(const string& s) throw(logic_error) {
      weight_type w = WeightTypeName(s);
      if (!w)
	throw logic_error("Not valid weight_type ["+s+"]");
      WeightType(w); 
    }

    /// definition for the current data type
    typedef wchar_t datatype;  // this should be selectable in run time...
  
    /// definition for the current data vector type
    typedef vector<datatype> vectortype;

    virtual string TargetType() const { return "text"; }

    virtual bool Incremental() const { return true; }

    /// if true, the text is depunctuated in the preprocessing phase
    virtual bool DoDepunctuate() const { return do_depunctuate; }

    /// if true, the text is converted to lowercase in the preprocessing phase
    virtual bool DoLowerCase() const { return do_lowercase; }

    /// if true, the text is stemmed in the preprocessing phase
    virtual bool DoStem() const { return do_stem; }

    ///
    virtual featureVector CalculatePerFrame(int f, int ff, bool print) {
      bool ok = CalculateOneFrame(f);
      if (print && ok)
	DoPrinting(-1, ff);
      
      return featureVector();
    }

    virtual bool CalculateOneFrame(int f) {
      if (FrameVerbose())
	cout << "TextBased::CalculateOneFrame(" << f << ")" << endl;

      return CalculateCommon(f, true);
    }
 
    virtual bool CalculateOneLabel(int f, int l) {
      if (LabelVerbose())
	cout << "TextBased::CalculateOneFrame(" << f << "," << ")" << endl;

      return CalculateCommon(f, false, l); 
    }
  
    virtual treat_type DefaultWithinFrameTreatment() const {
      return treat_concatenate; 
    }

    virtual treat_type DefaultBetweenFrameTreatment() const {
      return treat_separate; 
    }

    virtual treat_type DefaultBetweenSliceTreatment() const {
      return treat_separate; 
    }

    virtual pixeltype DefaultPixelType() const { return pixel_undef; }
 
    virtual featureVector CalculateRegion(const Region &/*r*/){
      TextBasedData *d=
	(TextBasedData*)CreateData(PixelType(),ConcatCountPerPixel(),0);
    
      cout << "TextBased::CalculateRegion not implemented!!" << endl;

      bool allow_zero_count = true;
      return d->Result(allow_zero_count);
    }

    /** Loads the data.
	\param datafile the name of the object file
	\param segmentfile the name of the segmentation file (default "")
	\return true if data was successfully loaded, otherwise false.
    */
    virtual bool LoadObjectData(const string& datafile, 
				const string& segmentfile = "") { 
      return LoadTextData(datafile, segmentfile, false);
    }

    /** This actually loads the data, called by the above one.
	\param datafile the name of the object file
	\param segmentfile the name of the segmentation file (default "")
	\param load_only_original do we only load the original text string, or
	should we also make char set conversion?
	\return true if data was successfully loaded, otherwise false.
    */
    bool LoadTextData(const string& datafile, 
		      const string& /*segmentfile*/,
		      bool load_only_original);

    // all classes derived from this class aren't image features
    virtual bool IsImageFeature() const { return false; } 
  
    virtual int FeatureVectorSize(bool) const = 0;

    virtual int Nframes() const { return 1; }

    virtual string ObjectDataFileName() const {
      return txtdat.filename();
    }

    virtual int printMethodOptions(ostream& = cout) const;

    /** Returns true if verbose level is set to show word-level info
	\return boolean saying if verbose level is set to show word-level info
    */
    static bool WordVerbose() { return KeyPointVerbose(); }

    /** Returns true if verbose level is set to show word-level detail info
	\return boolean saying if verbose level shows word-level detail info
    */
    static bool WordDetailVerbose() { return PixelVerbose(); }

    /// Data class for pixel based data
    class TextBasedData : public Data {
    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      TextBasedData(pixeltype t, int n, const Feature *p, const textdata &txt) 
	: Data(t, n,p) { 
	txtdatptr = txt;
	charactervec = vectortype();
      }

      /// Defined because we have virtual member functions...
      virtual ~TextBasedData() {}

      /// Zeros the internal storage
      virtual void Zero() { 
	charactervec = vectortype(charactervec.size());
      }

      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      virtual int Length() const { return charactervec.size(); }

      /** Returns the result of the feature extraction
	  \return feature result vector
      */
      virtual featureVector Result(bool) const {
	featureVector v;
	for (size_t i = 0; i<charactervec.size(); i++) 
	  v.push_back((float) charactervec[i]);

	return v;
      }

      virtual void AddData(float c) {
	charactervec.push_back((datatype)c);
      }

      virtual void AddData(datatype c) {
	charactervec.push_back(c);
      }

      virtual void AddData(const string& s ) {
	for(size_t i=0; i<s.length(); i++)
	  charactervec.push_back((datatype) s.at(i) );
      }

      virtual void RemoveData(const float) {
	charactervec.pop_back();
      }

      virtual void SetVector(const vectortype& v) {
	charactervec.assign(v.begin(), v.end());
      }

      virtual void SetVector(const vector<int>& v) {
	charactervec.assign(v.begin(), v.end());
      }

      virtual int DataUnitSize() const { return exts; } 

      virtual string Name() const { return "TextBasedData";}

      virtual textdata TextData() { return txtdatptr; }

      virtual textdata TextData() const 
      { return txtdatptr; }

    protected:
      /// internal storage of the text data
      vectortype charactervec;
    
      /// pointer to the textdata object
      textdata txtdatptr;

    }; // class TextBasedData
  
  protected:

    /// This constructor is called by inherited class constructors.
    TextBased() : ftype(textfile::file_text), stemmer_z(NULL),
		  do_depunctuate(true), do_lowercase(true),
		  do_stem(false), weight(weight_undef) {}

    /// This constructor doesn't add an entry in the methods list.
    /// !! maybe it shouldn't but it does !!
    TextBased(bool) : Feature(false) {}

    virtual ~TextBased() {
      if (stemmer_z) {
	if (FileVerbose())
	  cout << "Destroying stemmer" << endl;
	free_stemmer(stemmer_z);
      }
    }

    /// 
    virtual bool ProcessPreOptionsAndRemove(list<string>&);

    // f frame, all all segments, l segment data index
    virtual bool CalculateCommon(int f, bool all, int l = -1) {
      TextPreProcess(f, all, l);

      // EnsureText(); // should we make sure that this is a text object?
      if (FrameVerbose())
	cout << "TextBased::CalculateCommon(" << f << "," << all << "," << l 
	     << ")" << endl;

      int s = 1;
      int j = -1;
      
      try {
	j = DataIndex(s);
      }
      catch (const string& str) {
	if (str.find(") not found") != string::npos) {
	  // continue;
        }
      }

      bool ok = true;

      if (j>=0 && (all || s==l))
	switch (TextUnit()) {
	case textdata::unit_char:
	  ok = CalculateCharWise(j);
	  break;
	case textdata::unit_word:
	  ok = CalculateWordWise(j);
	  break;
	case textdata::unit_line:
	  ok = CalculateLineWise(j);
	  break;
	default:
	  cerr << "TextBased::CalculateCommon() : switch FAILED" << endl;
	  ok = false;
	}
      else
	ok = false;

      TextPostProcess(f, all, l);

      return ok;
    }

    // 
    bool CalculateCharWise(int j) {
      if (FrameVerbose())
	cout << "TextBased::CalculateCharWise(" << j
	     << ") : length=" << TextData().length() << endl;

      bool ok = true;

      for (int unsigned i=0; ok && i<TextData().length(); i++)
	ok = CalculateOneCharacter(i, (TextBasedData*)GetData(j)); 

      return ok;
    }

    // 
    bool CalculateWordWise(int j) {
      if (FrameVerbose())
	cout << "TextBased::CalculateWordWise(" << j
	     << ") : count=" << TextData().wordCount() << endl;

      bool ok = true;

      for (int unsigned i=0; i<TextData().wordCount(); i++)
	ok = CalculateOneWord(i, (TextBasedData*)GetData(j)); 

      return ok;
    }

    // 
    bool CalculateLineWise(int j) {
      if (FrameVerbose())
	cout << "TextBased::CalculateLineWise(" << j
	     << ") : count=" << TextData().lineCount() << endl;

      bool ok = true;

      for (int unsigned i=0; i<TextData().lineCount(); i++)
	ok = CalculateOneLine(i, (TextBasedData*)GetData(j)); 

      return ok;
    }

    virtual bool TextPreProcess(int /*f*/, bool /*all*/, int /*l*/) {
      return true;
    }

    virtual bool TextPostProcess(int /*f*/, bool /*all*/, int /*l*/) {
      return true;
    }

  public:
    /// if true, the text is depunctuated in the preprocessing phase
    void SetDoDepunctuate(bool v) { do_depunctuate = v; }

    /// if true, the text is converted to lowercase in the preprocessing phase
    void SetDoLowerCase(bool v) { do_lowercase = v; }

    /// if true, the text is stemmed in the preprocessing phase
    void SetDoStem(bool v) { do_stem = v; }

    ///
    virtual string WordPreProcess(const string& s) {
      if (WordVerbose())
	cout << "TextBased::WordPreProcess(" << s << ")" << endl;
      string r = s;

      if (DoDepunctuate())
	DePunctuate(r);

      if (DoLowerCase())
	LowerCase(r);

      if (DoStem())
	Stem(r);
      
      return r;
    }

  protected:

    // just add the character codes to the vector of characters
    virtual bool CalculateOneCharacter(int i, TextBasedData *d) {
      wchar_t c = CharacterAt(i);

      if (WordVerbose()) {
	stringstream ss;
	if (c>=32 && c<127)
	  ss << char(c);

	cout << "TextBased::CalculateOneCharacter(" << i << ","
	     << (void*)d << ") : c=<" << ss.str() << "> " << c
	     << endl;
      }

      d->AddData(c);

      return true;
    }

    /// The default implementation just gets the ith word from the text data 
    // and passes it on to the TextBasedData object.
    virtual bool CalculateOneWord(int i, TextBasedData *d) {
      string s = WordAt(i);

      if (WordVerbose()) {
	cout << "TextBased::CalculateOneWord(" << i << ","
	     << (void*)d << ") : s=<" << s << ">" 
	     << endl;
      }

      s = WordPreProcess(s);

      if (s!="")
	d->AddData(s);

      return true;
    }

    /// The default implementation just gets the ith line from the text data 
    // and passes it on to the TextBasedData object.
    virtual bool CalculateOneLine(int i, TextBasedData *d) {
      string s = LineAt(i);

      if (WordVerbose()) {
	cout << "TextBased::CalculateOneWord(" << i << ","
	     << (void*)d << ") : s=<" << s << ">" 
	     << endl;
      }

      d->AddData(s);

      return true;
    }

    virtual textdata& TextData() { return txtdat.Data(); }

    virtual const textdata& TextData() const { return txtdat.Data(); }

    virtual wchar_t CharacterAt(const int unsigned i) { 
      return TextData().get(i); }

    virtual string WordAt(const int unsigned i) {
      return TextData().getWord(i);
    }

    virtual string LineAt(const int unsigned i) {
      return TextData().getLine(i);
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const = 0;

    virtual vector<string> UsedDataLabels() const {
      vector<string> ret;

      char tmp[100];
      sprintf(tmp, "%d", 1);
      ret.push_back(tmp);

      if (LabelVerbose())
	ShowDataLabels("TextBased::UsedDataLabels", ret, cout);
    
      return ret;
    }
  
    //
    virtual textdata::unit TextUnit() const { return textdata::unit_char; }

    void Stem(char *cstr) {
      if (!stemmer_z) {
	if (FileVerbose())
	  cout << "Creating stemmer" << endl;
	stemmer_z = create_stemmer();
      }

      cstr[stem(stemmer_z, cstr, strlen(cstr)-1) + 1] = 0;
    }
  
    ///
    void Stem(string &str_in) {
      string str = str_in;
      size_t ss = str.size();
      if (ss>2 && str.substr(ss-2)=="'s")
	str.erase(ss-2);
      else if (ss>1 && str.substr(ss-1)=="'")
	str.erase(ss-1);

      char *cstr = new char[str.length()+1];
      str.copy(cstr, string::npos);
      cstr[str.length()] = 0;
      Stem(cstr);

      if (WordDetailVerbose())
	cout << "TextBased::Stem() : [" << str << "] -> [" << cstr << "]"
	     << endl;

      str_in = cstr;
      delete cstr;
    }
  
    ///
    void LowerCase(string &str) {
      string r = str;
      for (size_t i=0; i<r.size(); i++)
	if (r[i]>='A' && r[i]<='Z')
	  r[i] += 32;

      if (WordDetailVerbose())
	cout << "TextBased::LowerCase() : [" << str << "] -> [" << r << "]"
	     << endl;
      
      str = r;
    }

    ///
    void DePunctuate(string &str) {
      string r = str;
      for (size_t i=0; i<r.size(); )
	if ((r[i]>='!' && r[i]<='/' && r[i]!='-' && r[i]!='\'') ||
	    (r[i]>=':' && r[i]<='?'))
	  r.erase(i, 1);
	else
	  i++;

      if (WordDetailVerbose())
	cout << "TextBased::DePunctuate() : [" << str << "] -> [" << r << "]"
	     << endl;
      
      str = r;
    }

  public:
    ///
    typedef map<weight_type,string> weight_type_names_t;

  private:
    ///
    static void PrepareWeightTypeNames();

    ///
    textfile::file_type ftype;

    ///
    textfile txtdat;

    ///
    struct stemmer *stemmer_z;

    ///
    bool do_depunctuate;

    ///
    bool do_lowercase;

    ///
    bool do_stem;

    ///
    weight_type weight; 

    ///
    static weight_type_names_t weight_type_names;

  }; // class TextBased

} // namespace picsom

#endif // _TextBased_h_

// Local Variables:
// mode: font-lock
// End:
