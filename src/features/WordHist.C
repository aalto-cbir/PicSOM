// $Id: WordHist.C,v 1.16 2011/08/16 09:55:44 jorma Exp $	

#include <WordHist.h>

namespace picsom {
  static const string vcid =
    "$Id: WordHist.C,v 1.16 2011/08/16 09:55:44 jorma Exp $";

  static WordHist list_entry(true);

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

      AddDictEntry(word, d, n, c, tot_d, tot_n);
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

    vect_size = idx;

    return true;
  }

  //===========================================================================

  bool WordHist::TextPreProcess(int f, bool all, int l) {
    if (LabelVerbose())
      cout << "WordHist::TextPreProcess(" << f << "," << all << ","
	   << l << ")" << endl;

    if (!dict.empty())
      return true;

    bool ok = true;

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
    bool use_used_values = false;

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
      
      int idx = j->second.index;
      if (idx<0 || idx>=(int)res.size()) {
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
      
      res[idx] = tf * idf;

      if (LabelVerbose())
	cout << i->first << " (" << idx << ") : " << i->second << " -> "
	     << tf << " * " << idf << " = " << res[idx] << endl; 
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

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

