// $Id: WordHist.C,v 1.19 2016/10/25 08:13:35 jorma Exp $	

#include <WordHist.h>

#include <random>

namespace picsom {
  static const string vcid =
    "$Id: WordHist.C,v 1.19 2016/10/25 08:13:35 jorma Exp $";

  static WordHist list_entry(true);

  map<string,WordHist::w2v_data> WordHist::w2v_map;

  //===========================================================================

  string WordHist::Version() const {
    return vcid;
  }

  //===========================================================================

  int WordHist::printMethodOptions(ostream &str) const {
    str << "dict=[dictionary-file] : name of the dictionary file" << endl
        << "stop=[stopword-file]   : name of the stop word list file" << endl
	<< "top=[integer]          : number of used top words" << endl
	<< "linedump=[anything]    : forces dumping mode" << endl;

    return TextBased::printMethodOptions(str);
  }

  //===========================================================================

  bool WordHist::ProcessOptionsAndRemove(list<string>& l) {
    for (list<string>::iterator i = l.begin(); i!=l.end(); ) {
      if (MethodVerbose())
	cout << "  <" << *i << ">" << endl;
    
      string key, value;
      SplitOptionString(*i, key, value); 

      if (key=="linedump") {
	linedump = true;
 	i = l.erase(i);
	continue;
      }
    
      if (key=="zero2random") {
	zero2random = true;
 	i = l.erase(i);
	continue;
      }
    
      if (key=="w2v") {
	w2vfile = value;
 	i = l.erase(i);
	continue;
      }

      if (key=="dict") {
	dictfile = value;
 	i = l.erase(i);
	continue;
      }

      if (key=="stop") {
	stopfile = value;
 	i = l.erase(i);
	continue;
      }

      if (key=="top") {
	top_set_size = atoi(value.c_str());
 	i = l.erase(i);
	continue;
      }

      i++;
    }

    return TextBased::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  const vector<float>& WordHist::word2vec(const string& word,
					  const string& file) {
    static const vector<float> empty;

    auto m = word2vec_find(file);

    return m ? m->tovec(word) : empty;
  }
  
  //===========================================================================

  int WordHist::word2vec_index(const string& word, const string& file) {
    static const vector<float> empty;
    
    auto m = word2vec_find(file);
    if (!m)
      return -1;

    auto i = m->wmap.find(word);
    
    return i==m->wmap.end() ? -1 : i->second;
  }
  
  //===========================================================================

  const WordHist::w2v_data *WordHist::word2vec_find(const string& file) {
    auto m = w2v_map.find(file);
    if (m==w2v_map.end() && file=="" && w2v_map.size()==1)
      m = w2v_map.begin();

    if (m==w2v_map.end() && file!="") {
      w2v_map[file] = w2v_data();
      m = w2v_map.find(file);
      m->second.read(file);
    }

    return m==w2v_map.end() ? NULL : &m->second;
  }
  
  //===========================================================================

  WordHist::w2v_data::w2v_data(const string& file) {
    if (file!="")
      read(file);
  }

  //===========================================================================

  bool WordHist::w2v_data::read(const string& file) {
    ifstream fs(file);
    unsigned long long words = 0, size = 0;
    fs >> words >> size;
    // cout << "  words=" << words << " size=" << size << endl;
    for (size_t i=0; fs && i<words; i++) {
      char c;
      fs.get(c);
      if (c!='\n')
	return false;

      string w;
      while (fs) {
	fs.get(c);
	if (c==' ')
	  break;
	w += c;
      }
      // cout << "<" << w << ">" << endl;
      vector<float> v(size);
      fs.read((char*)&v[0], size*sizeof(float));
      if (fs) {
	double s = 0;
	for (size_t j=0; j<v.size(); j++)
	  s += v[j]*v[j];
	s = s ? 1/sqrt(s) : 0;
	for (size_t j=0; j<v.size(); j++)
	  v[j] *= s;

	vec.push_back(v);
	vocab.push_back(w);
	wmap[w] = i;
      }
    }

    return true;
  }

  //===========================================================================

  bool WordHist::ReadW2vFile() {
    if (MethodVerbose())
      cout << "WordHist::ReadW2vFile() [" << w2vfile << "]" << endl;

    w2v = word2vec_find(w2vfile);
    for (size_t b=0; b<w2v->vocab.size(); b++)
      AddDictEntry(w2v->vocab[b], b, 0, 0, 0, 0, 0);

    return true;
  }

  //===========================================================================

  bool WordHist::ReadDictFile() {
    if (MethodVerbose())
      cout << "WordHist::ReadDictFile() [" << dictfile << "]" << endl;

    ifstream ifstr;
    ifstr.open(dictfile.c_str());

    if (!ifstr) {
      cerr << "WordHist::ReadDictFile() unable to open <" << dictfile << ">"
	   << endl;
      return false;
    }

    float tot_d = 0, tot_n = 0;

    string row;
    size_t idx = 0;
    while (getline(ifstr, row)) {
      if (WordVerbose())
	cout << "WordHist::ReadDictFile() : [" << row << "]" << endl;
      
      if (!row.size() || row[0]=='#')
	continue;

      if (row.find(AllWordsLabel())==0) {
	istringstream iss;
	iss.str(row.substr(AllWordsLabel().size()));
	iss >> tot_d >> tot_n;
	continue;
      }

      size_t s = row.find_first_of(" \t");
      string word = row.substr(0, s);
      istringstream iss;
      iss.str(row.substr(s));

      float d = 0, n = 0, c = 0;
      iss >> d >> n >> c;

      AddDictEntry(word, idx++, d, n, c, tot_d, tot_n);
    }

    if (MethodVerbose())
      cout << "WordHist::ReadDictFile() : size=" << dict.size()
	   << " tot_d=" << tot_d << " tot_n=" << tot_n << endl;

    return true;
  }

  //===========================================================================

  bool WordHist::ReadStopFile() {
    if (MethodVerbose())
      cout << "WordHist::ReadStopFile() [" << stopfile << "]" << endl;

    ifstream ifstr;
    ifstr.open(stopfile.c_str());

    if (!ifstr) {
      cerr << "WordHist::ReadStopFile() unable to open <" << stopfile << ">"
	   << endl;
      return false;
    }

    int tot = 0, find = 0, dup = 0;
    string row;
    while (getline(ifstr, row)) {
      if (WordVerbose())
	cout << "WordHist::ReadStopFile() : [" << row << "] ";
      
      if (!row.size() || row[0]=='#') {
	if (WordVerbose())
	  cout << "COMMENT" << endl;
	continue;
      }

      tot++;

      string word = row;
      
      const char *white = " \t";
      word.erase(0, word.find_first_not_of(white));
      word.erase(word.find_last_not_of(white)+1);
      if (DoStem())
	Stem(word);

      if (WordVerbose())
	cout << "[" << word << "] ";

      dict_t::iterator i = dict.find(word);
      if (i==dict.end()) {
	if (WordVerbose())
	  cout << "NOT FOUND" << endl;
	continue;
      }

      if (i->second.stop) {
	if (WordVerbose())
	  cout << "DUPLICATE" << endl;
	dup++;
	continue;
      }

      if (WordVerbose())
	cout << endl;

      find++;
      i->second.stop = true;
    }

    if (MethodVerbose())
      cout << "WordHist::ReadStopFile() : tot=" << tot
	   << " find=" << find << " dup=" << dup << endl;

    return true;
  }

  //===========================================================================

  bool WordHist::FinalizeWordIndex() {
    if (MethodVerbose())
      cout << "WordHist::FinalizeWordIndex()" << endl;

    if (top_set_size) {
      multimap<float,string> map;
      for (dict_t::const_iterator i = dict.begin(); i!=dict.end(); i++) {
	if (!i->second.stop)
	  map.insert(pair<float,string>(i->second.n, i->first));
	while((int)map.size()>top_set_size)
	  map.erase(map.begin());
      }

      set<string> wset;
      for (multimap<float,string>::const_iterator i=map.begin();
	   i!=map.end(); i++) 
	wset.insert(i->second);

      for (dict_t::iterator i = dict.begin(); i!=dict.end(); i++) {
	i->second.unused = wset.find(i->first)==wset.end();
	if (MethodVerbose())
	  cout << "WordHist::FinalizeWordIndex() : "
	       << i->first << " : unused=" << i->second.unused << endl;
      }
    }

    int idx = 0;
    for (dict_t::iterator i = dict.begin(); i!=dict.end(); i++)
      i->second.index = !i->second.stop && !i->second.unused ? idx++ : -1;

    vect_size = w2v ? w2v->size() : idx;

    return true;
  }

  //===========================================================================

  bool WordHist::TextPreProcess(int f, bool all, int l) {
    if (LabelVerbose())
      cout << "WordHist::TextPreProcess(" << f << "," << all << ","
	   << l << ")" << endl;

    if (!dict.empty())
      return true;

    if (dictfile!="" && w2vfile!="") {
      cerr << "Cannot have both dictfile and w2vfile set" << endl;
      return false;
    }

    bool ok = true;

    if (w2vfile!="")
      ok = ReadW2vFile();

    if (dictfile!="")
      ok = ReadDictFile();

    if (ok && stopfile!="")
      ok = ReadStopFile();

    if (ok)
      ok = FinalizeWordIndex();

    return ok;
  }

  //===========================================================================

  bool WordHist::TextPostProcess(int f, bool all, int l) {
    if (LabelVerbose())
      cout << "WordHist::TextPostProcess(" << f << "," << all << ","
	   << l << ")" << endl;

    int j = 0;

    if (!PrintingEnabled())
      return ((WordHistData*)GetData(j))->ShowCount(cout);

    return true;
  }

  //===========================================================================

  Feature::featureVector WordHist::Result(const WordHistData& d, bool) const {
    bool use_used_values = false, use_orig = true;

    if (LabelVerbose())
      cout << "WordHist::Result() : weight="
	   << WeightTypeName(WeightType()) << endl;

    featureVector res(FeatureVectorSize(false));

    float tot = 0, used_tot = 0, max = 0, used_max = 0;
    for (WordHistData::count_t::const_iterator i=d.count.begin();
	 i!=d.count.end(); i++) {
      tot += i->second;
      if (i->second>max)
	max = i->second;

      dict_t::const_iterator j = dict.find(i->first);
      if (j!=dict.end()) {
	int idx = j->second.index;
	if (idx>=0 && idx<(int)res.size()) {
	  used_tot += i->second;
	  if (i->second>used_max)
	    used_max = i->second;
	}
      }
    }

    if (LabelVerbose())
      cout << " tot=" << tot << " used_tot=" << used_tot
	   << " max=" << max << " used_max=" << used_max << endl;

    if (use_used_values) {
      tot = used_tot;
      max = used_max;
    }

    size_t maxidx = w2v ? w2v->vec.size() : res.size();
    bool iszero = true;
    for (WordHistData::count_t::const_iterator i=d.count.begin();
	 i!=d.count.end(); i++) {
      if (i->first==AllWordsLabel())
	continue;

      dict_t::const_iterator j = dict.find(i->first);
      if (j==dict.end()) {
	if (LabelVerbose())
	  cout << "WordHist::Result() [" << i->first << "] not found" << endl;
	continue;
      }
      
      int idx = use_orig ? j->second.orig_idx : j->second.index;
      if (idx<0 || idx>=(int)maxidx) {
	if (LabelVerbose())
	  cout << "WordHist::Result() failed with index " << idx
	       << " of [" << i->first << "]" << endl;
	continue;
      }

      float tf = 0.0, idf = 0.0;

      switch (WeightTypeTf()) {
      case weight_undef:
      case tf_div_all:
	tf = i->second/tot;
	break;

      case tf_div_max:
	tf = i->second/max;
	break;

      case tf_one:
	tf = 1.0;
	break;

      case tf_pure:
	tf = i->second;
	break;

      case tf_log:
	tf = log(1.0+i->second);
	break;

      default:
	throw("processing of tf weight_type ["+WeightTypeName(WeightTypeTf())
	      +"] not defined");
      }

      switch (WeightTypeIdf()) {
      case weight_undef:
      case idf_one:
	idf = 1.0;
	break;

      case idf_pure:
	idf = j->second.idf;
	break;

      case idf_log:
	idf = j->second.log_idf;
	break;

      default:
	throw("processing of idf weight_type ["+WeightTypeName(WeightTypeIdf())
	      +"] not defined");
      }
      
      double tfidf = tf * idf;
      if (w2v) {
	const vector<float>& v = w2v->vec[idx];
	for (size_t j=0; j<res.size(); j++)
	  res[j] += tfidf*v[j];

      } else
	res[idx] = tfidf;

      iszero = false;

      if (LabelVerbose())
	cout << i->first << " (" << idx << ") : " << i->second << " -> "
	     << tf << " * " << idf << " = " << tfidf << endl; 
    }

    if (iszero && zero2random) {
      if (LabelVerbose())
	cout << "WordHist::Result() : filling zero vector with random vector"
	     << endl;

      random_device rd;
      mt19937 gen(rd());
      normal_distribution<> d;
      double ss = 0;
      for (size_t i=0; i<res.size(); i++) {
	double v = d(gen);
	ss += v*v;
	res[i] = v;
      }
      ss = sqrt(ss);
      for (size_t i=0; i<res.size(); i++)
	res[i] /= ss;
    }

    return res;
  }

  //===========================================================================
  //===========================================================================

  void WordHist::WordHistData::AddData(const string& s) {
    count[s]++;
    if (P()->WordVerbose())
      cout << "WordHistData::AddData(" << s << ") -> " << count[s] << endl;

    if (P()->linedump)
      P()->Print(s+" ");
  }

  //===========================================================================

  bool WordHist::WordHistData::ShowCount(ostream& os) {
    if (P()->linedump) {
      os << P()->Label(-1, -1, "") << endl;
      return true;
    }

    if (P()->WordVerbose())
      cout << "WordHistData::ShowCount()" << endl;

    int tot = 0;
    for (count_t::const_iterator i=count.begin(); i!=count.end(); i++)
      tot += i->second;

    os << "# word histogram of " << P()->BaseFileName() << " -> "
       << P()->Label(-1, -1, "") << endl;
    os << AllWordsLabel() << " 1 " << tot << endl;
    for (count_t::const_iterator i=count.begin(); i!=count.end(); i++)
      if (i->first!=AllWordsLabel())
	os << i->first << " 1 " << i->second << " " << tot << endl;

    return true;
  }

  //===========================================================================

  bool WordHist::IsSparseVector() const {
    return false;
  }

  //===========================================================================

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

