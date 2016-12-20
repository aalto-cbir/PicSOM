// $Id: WebLink.C,v 1.8 2008/09/29 10:22:40 markus Exp $	

#include <WebLink.h>

namespace picsom {
  static const char *vcid =
    "$Id: WebLink.C,v 1.8 2008/09/29 10:22:40 markus Exp $";

static WebLink list_entry(true);

//=============================================================================

string WebLink::Version() const {
  return vcid;
}

//=============================================================================


int WebLink::printMethodOptions(ostream &/*str*/) const {
  return 0; //Feature::printMethodOptions(str);
}


//=============================================================================

bool WebLink::ProcessOptionsAndRemove(list<string> &/*l*/) {
  //   return Feature::ProcessOptionsAndRemove(l);
  return true;
}

//=============================================================================

WebLink::WebLinkData::digest_vect_t 
WebLink::WebLinkData::hashsum(const string &st, int algo) const {
  // GCRY_MD_MD5 GCRY_MD_SHA1
  size_t hash_len = gcry_md_get_algo_dlen(algo);
  digest_t *buf;
  buf = (digest_t*) malloc(hash_len);
  gcry_md_hash_buffer(algo, buf, st.c_str(), st.length());
  
  digest_vect_t ret;
  
  for(unsigned int i=0; i<hash_len; i++)
    ret.push_back(buf[i]);
  
  free(buf);
  
  return ret;
}

//=============================================================================

Feature::featureVector WebLink::WebLinkData::WebLinkResult() const {
  // start with zero vector
  featureVector v(WEBLINK_VECTOR_LENGTH);
  ZeroFeatureVector(v);

  textlines_t lines = TextData().get_lines_as_words();
  urlval_vect_t urls;

  for (textlines_t::const_iterator linep = lines.begin(); 
       linep != lines.end(); linep++) {
    string firstword = linep->at(0);
    if (firstword == "#") continue; // skip comments
    if (firstword == "page" && linep->at(1) == "=") {
      string url = linep->at(2);
      AddToFeatureVectorMax(v,UrlResult(url,10.0,true),true);
    } else
      AddToFeatureVectorMax(v,UrlResult(firstword,1.0),true);
  }
  
  return v;
}

//=============================================================================


/** This function takes an url, and splits it into its domain-part and
    different directories, and calculates hashsum from each part and
    from the entire url and sums the random vectors together.
    
    eg. http://www.hut.fi/Opinnot/index.html
    result = 
      hashsum(http://www.hut.fi/Opinnot/index.html)*w1 +
      hashsum(http://www.hut.fi/Opinnot/)*w2 + 
      hashsum(http://www.hut.fi)*w3
		
*/
Feature::featureVector 
WebLink::WebLinkData::UrlResult(string url, double weight, 
																bool skip_orig_url) const 
{
	string msg = "WebLink::WebLinkData::UrlResult(): ";
  urlval_vect_t urlparts;
  double wt = 1.0;
  
  // push entire url with inital weight
	if (!skip_orig_url) {
		urlparts.push_back(urlval_t(url,wt));

		if (LabelVerbose())
			cout << msg << "Adding url: " << url << " - " << wt << endl;
	}
  
  string::size_type endpos = 6; // pos at "http://"
  string::size_type slashpos = url.rfind("/");

  while (slashpos > endpos && slashpos != string::npos) {
    // now check the part before last slash
    string part = url.substr(0,slashpos+1);
    if (part != url) { // if part == url the basepath was a directory
      wt = wt/1.1;

      urlparts.push_back(urlval_t(part,wt));
      if (LabelVerbose())
				cout << msg << "Adding url part: " << part << " - " << wt << endl;

    }
    // continue checking with the rest of the url
    url = url.substr(0,slashpos);	
    slashpos = url.rfind("/");
  }
	
  featureVector ret = SumURLs(urlparts);
	double len = sqrt(VectorSqrLength(ret));
	DivideFeatureVector(ret,len/sqrt(weight));
	if (LabelVerbose())
		cout << msg << "created vector with length=" << VectorSqrLength(ret) << endl;
  return ret;
}

//=============================================================================

Feature::featureVector WebLink::WebLinkData::SumURLs(urlval_vect_t &urls) const
{
  featureVector v(WEBLINK_VECTOR_LENGTH);
  ZeroFeatureVector(v);

  for (urlval_vect_t::const_iterator urlp = urls.begin();
       urlp != urls.end(); urlp++) {
    string url = urlp->first;
    if (IsURL(url)) {
      digest_vect_t dig_sha = hashsum(url,SHA1_ALGO);
      if (LabelVerbose())
				cout << "WebLink::WebLinkData::SumURLs(): " << url << " => " << DigestVectToString(dig_sha)
						 << endl;
      AddSha1Indexes(v,dig_sha,urlp->second);
    }
  }
  return v;
}

//=============================================================================

void WebLink::WebLinkData::ZeroFeatureVector(featureVector &v) const {
  for (featureVector::iterator it = v.begin(); it != v.end(); it++)
    *it=0;
}

//=============================================================================

string WebLink::WebLinkData::DigestVectToString(digest_vect_t& v) const {
  string s;
  for (digest_vect_t::const_iterator dit = v.begin(); dit != v.end(); 
       dit++) {
    char tmp[5];
    sprintf(tmp,"%02x",*dit);
    s += tmp;
  }
  return s;  
}

//=============================================================================

bool WebLink::WebLinkData::IsURL(const string& s) const {
  // fixme: do some fancier test here...
  return (s.find("http://") != string::npos);
}      
    
//=============================================================================

void WebLink::WebLinkData::AddSha1Indexes(Feature::featureVector &v, 
					  const digest_vect_t& dig, 
					  double val) const 
{
  for (int i=0; i<4; i++) {
    int idx = dig[i]+256*i;
    if (v[idx] < val) v[idx] = val;
  }

}
}
//=============================================================================


// Local Variables:
// mode: font-lock
// End:
