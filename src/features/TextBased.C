// $Id: TextBased.C,v 1.9 2011/08/16 09:55:55 jorma Exp $	

#include <TextBased.h>

namespace picsom {
  TextBased::weight_type_names_t TextBased::weight_type_names;

  //===========================================================================

  int TextBased::printMethodOptions(ostream &str) const {
    str << "text-mode=[text|html|xml] (default text) :"
	<< " the text mode, html/xml mode removes tags" << endl
	<< "depunctuate=[true|false]  (default true) :"
	<< " whether words are depunctuated first"      << endl
	<< "lowercase=[true|false]    (default true) :"
	<< " whether words are lowercased first" << endl
	<< "stem=[true|false]        (default false) :"
	<< " whether words are stemmed first" << endl
	<< "weight=[(tfp|tfl|tfa|tfm)+(idfn|idfp|idfl)] :"
	<< " term weighting method" << endl;

    return Feature::printMethodOptions(str);
  }

  //===========================================================================

  bool TextBased::ProcessPreOptionsAndRemove(list<string> &l) {
    static const string msg = "TextBased::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;
    
      string key, value;
      SplitOptionString(*i, key, value); 

      if (key=="text-mode") {
	if (value == "text") 
	  ftype = textfile::file_text;
	else if (value == "html")
	  ftype = textfile::file_html;
	else if (value == "xml")
	  throw "xml mode not implemented yet";
	else 
	  throw "text-mode=[text|html|xml]";

	i = l.erase(i);
	continue;
      }

      if (key=="depunctuate") {
	if (IsBoolean(value))
	  do_depunctuate = IsAffirmative(value);
	else
	  throw msg+"do_depunctuate !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="lowercase") {
	if (IsBoolean(value))
	  do_lowercase = IsAffirmative(value);
	else
	  throw msg+"do_lowercase !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="stem") {
	if (IsBoolean(value))
	  do_stem = IsAffirmative(value);
	else
	  throw msg+"do_stem !(true|false)";
      
	i = l.erase(i);
	continue;
      }

      if (key=="weight") {
	try {
	  WeightType(value);
	}
	catch (...) {
	  throw msg+"weight type ["+value+"] not solved";
	}

	i = l.erase(i);
	continue;
      }

      i++;
    }

    return Feature::ProcessPreOptionsAndRemove(l);
  }

  //===========================================================================

  bool TextBased::LoadTextData(const string& datafile, 
			       const string& /*segmentfile*/,
			       bool load_only_original) {
    if (FileVerbose())
      cout << "TextBased::LoadTextData([" << datafile << "],"
	   << load_only_original << ")" << endl;

    vector<string> part = SplitFilename(datafile);
    if (part.empty())
      return false;

    txtdat = textfile(part[0], load_only_original, ftype);
    for (size_t i=1; i<part.size(); i++) {
      textfile tmp = textfile(part[i], load_only_original, ftype);
      txtdat.append(tmp);
    }

    return true;
  }

  //===========================================================================

  void TextBased::PrepareWeightTypeNames() {
    if (!weight_type_names.empty())
      return;

    weight_type_names[tf_one]     = "tfn";
    weight_type_names[tf_pure]    = "tfp";
    weight_type_names[tf_log]     = "tfl";
    weight_type_names[tf_div_all] = "tfa";
    weight_type_names[tf_div_max] = "tfm";

    weight_type_names[idf_one]    = "idfn";
    weight_type_names[idf_pure]   = "idfp";
    weight_type_names[idf_log]    = "idfl";

    for (int i=1; i<=5; i++)
      for (int j=1; j<=3; j++)
	weight_type_names[weight_type(i+j*256)] =
	  weight_type_names[weight_type(i)]+"+"+
	  weight_type_names[weight_type(j*256)];
  }

  //===========================================================================

  TextBased::weight_type TextBased::WeightTypeName(const string& s) {
    PrepareWeightTypeNames();

    for (weight_type_names_t::const_iterator i = weight_type_names.begin();
	 i!=weight_type_names.end(); i++)
      if (i->second==s)
	return i->first;
    
    return weight_undef;
  }

  //===========================================================================

  const string& TextBased::WeightTypeName(TextBased::weight_type w) {
    PrepareWeightTypeNames();

    weight_type_names_t::const_iterator i = weight_type_names.find(w);
    if (i!=weight_type_names.end())
      return i->second;

    static const string undef = "UNDFEF";
    return undef;
  }

  //===========================================================================

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:





