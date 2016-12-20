// -*- C++ -*-  $Id: CbirGVT.C,v 2.12 2011/03/25 11:14:57 jorma Exp $
// 
// Copyright 1998-2011 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <CbirGVT.h>
#include <Analysis.h>
#include <Valued.h>
#include <Query.h>

namespace picsom {
  ///
  static const string CbirGVT_C_vcid =
    "@(#)$Id: CbirGVT.C,v 2.12 2011/03/25 11:14:57 jorma Exp $";

  /// This is the "factory" instance of the class.
  static CbirGVT list_entry(true);

  //===========================================================================

  /// This is the principal constructor.
  CbirGVT::CbirGVT(DataBase *db, const string& args)
    : CbirAlgorithm(db, args) {

    string hdr = "CbirGVT::CbirGVT() : ";

    debug = 0;
    lastroundonly = false;
    
    if (db->Name().find("flickr")!=0)
      ShowError(hdr+" algorithm=gvt works only with database=flickr");

    if (debug_stages)
      cout << TimeStamp()
	   << hdr+"constructing an instance with arguments \""
	   << args << "\"" << endl;

    map<string,string> amap = SplitArgumentString(args);
    for (map<string,string>::const_iterator a=amap.begin(); a!=amap.end(); a++)
      ShowError(hdr+"key ["+a->first+"] not recognized");

    if (debug_stages)
      cout << TimeStamp()
	   << hdr+"constructed instance with FullName()=\"" << FullName()
	   << "\"" << endl;

    // After calling CbirAlgorithm::CbirAlgorithm(DataBase*) we should
    // have a valid DataBase pointer named database:

    if (false)
      cout << TimeStamp() << hdr
	   << "we are using database \"" << database->Name()
	   << "\" that contains " << database->Size() << " objects" << endl;

  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  const list<CbirAlgorithm::parameter>& CbirGVT::Parameters() const {
    static list<parameter> l;
    if (l.empty()) {
    }

    return l;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirGVT::Initialize(const Query *q, const string& s,
			       CbirAlgorithm::QueryData*& qd) const {
    string hdr = "CbirGVT::Initialize() : ";

    if (!CbirAlgorithm::Initialize(q, s, qd))
      return ShowError(hdr+"CbirAlgorithm::Initialize() failed");

    // After calling CbirAlgorithm::Initialize() we should have a valid Query
    // pointer named qd->query:

    if (false)
      cout << TimeStamp() << hdr << "query's identity is \""
	   << GetQuery(qd)->Identity() << "\" and target type \""
	   << TargetTypeString(GetQuery(qd)->Target()) << "\"" << endl;

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirGVT::AddIndex(CbirAlgorithm::QueryData *qd,
                             Index::State *f) const {
    string hdr = "CbirGVT::AddIndex() : ";
    if (!CbirAlgorithm::AddIndex(qd, f))
      return ShowError(hdr+"CbirAlgorithm::AddIndex() failed");

    // After calling CbirAlgorithm::AddIndex() we should have a list
    // of feature names in variable named features:

    if (false) {
      cout << TimeStamp() << hdr
	   << "the following features have been selected: ";
      for (size_t i=0; i<Nindices(qd); i++)
	cout << (i?", ":"") << IndexFullName(qd, i);
      cout << endl;
    }

    /* this ws added in 2.34 (why?) and removed in 2.42 because
       it prevented processing algorithm1=pinview_xxx defaultfeatures=yyy
       from settings.xml

    for (size_t i=0; i<qd->Nindices(); i++)
      if (qd->IndexShortName(i)==f->fullname && qd->IsTsSom(i)) {
        Index *idx = &qd->IndexStaticData(i);
        VectorIndex *vidx = dynamic_cast<VectorIndex*>(idx);
        if (vidx) {
          vidx->ReadDataFile();
          vidx->SetDataSetNumbers(false);
          break;
        }
      }
    */

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first checks for its own parameters and then calls the
  /// corresponding method of the base class.
  bool CbirGVT::Interpret(CbirAlgorithm::QueryData *qd, const string& k,
			      const string& v, int& r) const {
    string hdr = "CbirGVT::Interpret() : ";

    if (debug_stages)      
      cout << TimeStamp()
	   << hdr+"key=\""+k+"\" value=\""+v+"\"" << endl;

    // CbirGVT::QueryData *qde = CastData(qd);

    r = 1;

    if (k=="debug") {
      ((CbirGVT*)this)->debug = atoi(v.c_str());
      return true;
    }

    if (k=="lastroundonly") {
      ((CbirGVT*)this)->lastroundonly = IsAffirmative(v);
      return true;
    }

    return CbirAlgorithm::Interpret(qd, k, v, r);
  }

  //===========================================================================

  ObjectList CbirGVT::CbirRound(CbirAlgorithm::QueryData *qd,
				const ObjectList& seenin,
				size_t maxq) const {
    string hdr = "CbirGVT::CbirRound() : ";

    CbirAlgorithm::CbirRound(qd, seenin, maxq);

    double threshold = 0.653;

    int verbose = 2;
    if (verbose<0)
      verbose = 0;

    if (verbose) {
      cout << TimeStamp() << hdr << "starting with " << seenin.size()
	   << " objects already seen" << endl;
    }

    int lastround = seenin.size() ? seenin[seenin.size()-1].Round() : -1;

    vector<size_t> posi;
    for (size_t i=0; i<seenin.size(); i++) {
      bool sel = false;

      if (verbose>1)
	cout << seenin[i].DumpStr();

      if (seenin[i].Value()>0) {
	sel = true;
	if (verbose>1)
	  cout << " SELECTED because clicked";
      }

      if (seenin[i].AspectRelevance("gaze-cont")>threshold) {
	sel = true;
	if (verbose>1)
	  cout << " SELECTED because gaze-cont>"+ToStr(threshold);
      }

      if (lastroundonly && seenin[i].Round()!=lastround && sel) {
	sel = false;
	if (verbose>1)
	  cout << " UNSELECTED because lastroundonly";
      }

      if (sel)
	posi.push_back(seenin[i].Index());

      if (verbose>1)
	cout << endl;
    }

    size_t nallowed = 0;
    ground_truth allowed(database->Size());
    for (size_t i=0; i<database->Size(); i++)
      if (database->ObjectsTargetTypeContains(i, GetQuery(qd)->Target()) &&
	  GetQuery(qd)->CanBeShownRestricted(i, true)) {
	allowed[i] = 1;
	nallowed++;
      }

    size_t nobj = qd->GetMaxQuestions(maxq);

    if (verbose)
      cout << TimeStamp() << hdr << "there are " << nallowed
	   << " objects to select " << nobj << " with "
	   << posi.size() << " positive samples"
	   << endl;

    size_t nobjperpos = nobj;
    // old rule used before 2011-03-21:
    // if (posi.size())
    //   nobjperpos = 1+115*nobj/(100*posi.size());
    if (posi.size())
      nobjperpos = posi.size()+115*nobj/(100*posi.size());

    if (nobjperpos<100)
      nobjperpos = 100;

    vector<size_t> nobjvec(posi.size(), nobjperpos);

    if (verbose)
      cout << TimeStamp() << hdr << "heuristically requesting "
	   << nobjperpos << " GVT images for each positive sample"
	   << endl;

    ground_truth gvtout = ((CbirGVT*)this)->GetGvtImages(posi, nobjvec,
							 nobj, allowed);

    vector<size_t> gvtsel = gvtout.indices(1);

    ObjectList list;

    size_t nposs = 0;
    for (size_t i=0; i<gvtsel.size(); i++)
      if (allowed[gvtsel[i]]==1) {
	nposs++;
	if (list.size()<nobj)
	  list.push_back(Object(database, gvtsel[i], select_question));
      }

    if (verbose)
      cout << TimeStamp() << hdr << "GVT returned " << gvtsel.size()
	   << " images of which " << nposs << " were allowed and "
	   << list.size() << " first ones selected"
	   << endl;
      
    return list;
  }

  //===========================================================================

} // namespace picsom

//*****************************************************************************

#define WITH_OPENSSL 
#undef HAVE_POLL
#undef HAVE_SNPRINTF
#undef HAVE_RAND_R
#undef HAVE_LOCALTIME_R
#undef HAVE_ISNAN
#undef HAVE_RAND_R
#undef HAVE_LOCALTIME_R
#undef HAVE_SNPRINTF
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include "WrapAuthenticator.h"
#include "GSoapUtils.h"
#include "soapImageCollectionServiceSoapBindingProxy.h"
#include "ImageCollectionServiceSoapBinding.nsmap"

static string getAccessToken(const string &/*username*/)
{
#if 0
    // get username/password
    cout << "Username [" << username << "]: ";
    string login;
    cin >> login;
    if (login.empty()) {
        login = username;
    }
    if (login.empty()) {
        cerr << "No username. Exiting" << endl;
        return "";
    }
    // need to keep the password until the service is created. Then fill it with blank
#ifdef _WIN32
    string passwd_str;
    cout << "Password: ";
    cin >> passwd_str;
    char *passwd = (char *)passwd_str.c_str();
#else
    char *passwd = getpass("Password:");
#endif
    if (passwd == NULL || strlen(passwd) == 0) {
        cerr << "No password. Exiting" << endl;
        return "";
    }
#endif // 0

    // cert verifications fails for an unknown reason...
    // OpenXerox::WrapAuthenticator::accept_invalid_cert = true;
    OpenXerox::WrapAuthenticator::cacert_file = "/etc/ssl/certs/ed524cf5.0";

    string login = "jormalaaksonen";
    char passwd[100] = "pinview";

    // use WRAP authentication for the next calls
    string access_token;
    try {
        access_token = OpenXerox::WrapAuthenticator::getAccessToken(login, passwd);
    }
    catch (OpenXerox::WrapAuthenticatorException &e) {
        cerr << "Authentication failed: " << e.what() << endl;
    }
    //cerr << "Access token = " << access_token << std::endl;

    // The service is created. Fill the password with blank
    if (passwd != NULL) {
        memset(passwd, 0, strlen(passwd));
    }
    return access_token;
}


static void setAuthorization(soap *soap, const string &access_token)
{
    soap->header = (SOAP_ENV__Header *)soap_malloc(soap, sizeof(SOAP_ENV__Header));
    //soap->header->oxns__Authorization = (char*)soap_malloc(soap, (access_token.length() + 1)
    //        * sizeof(char));
    //strcpy(soap->header->oxns__Authorization, access_token.c_str());
    soap->header->oxns__Authorization = (char *)access_token.c_str();
}

//=============================================================================

void *CbirGVT::GetGvtImagesCommon(string& access_token) const {
  string msg = "CbirGVT::GetGvtImagesCommon() : ";
  GetDataBase()->WriteLog(msg+"starting");

  string endpoint, username;

  // Get access_token
  access_token = getAccessToken(username);

  static bool first = true;
  if (first) {
    soap_ssl_init(); // init OpenSSL (just once)
    first = false;
  }

  // Create the soap client
  ImageCollectionServiceSoapBindingProxy *imageCollection = new ImageCollectionServiceSoapBindingProxy;
  if (imageCollection == NULL) {
    cerr << "Cannot connect to Web Service. Exiting." << endl;
    return NULL;
  }

  if (!endpoint.empty()) {
    // User gave an alternate endpoint, use it
    imageCollection->soap_endpoint = endpoint.c_str();
  }

  // Set the alternate cacert if needed
  if (!OpenXerox::GSoap::setCacert(imageCollection)) {
    return NULL;
  }

  // Set the proxy from the https_proxy environment variable
  OpenXerox::GSoap::setHttpsProxy(imageCollection);

  // Set authentication before each call
  setAuthorization(imageCollection, access_token);
  gvt__getLoginName loginname_payload;
  gvt__getLoginNameResponse loginname_response;
  if (imageCollection->getLoginName(&loginname_payload, &loginname_response) == SOAP_OK) {
    // cerr << "Your login name is: " << *loginname_response.return_ << endl;
  }
  else {
    soap_print_fault(imageCollection, stderr);
  }

  setAuthorization(imageCollection, access_token);

  return imageCollection;
}

//=============================================================================

ground_truth CbirGVT::GetGvtImagesOneByOne(size_t idx, size_t nn) {
  string msg = "CbirGVT::GetGvtImagesOneByOne("+ToStr(idx)+","+ToStr(nn)+") : ";

  ground_truth ret(GetDataBase()->Size());

  bool hit = false;
  vector<size_t> cres = FindCache(idx, nn, hit);
  if (hit) {
    cout << msg << "cache hit, returning " << ToStr(cres.size()) << " images"
	 << endl;

    for (size_t i=0; i<cres.size(); i++)
      ret[i] = 1;

    return ret;
  }

  string access_token;

  ImageCollectionServiceSoapBindingProxy *imageCollection =
    (ImageCollectionServiceSoapBindingProxy*)GetGvtImagesCommon(access_token);
  if (!imageCollection)
    return ground_truth();  

  string collection_name = "b12n";

  GetDataBase()->WriteLog(msg+"starting");

  size_t nretpos = 0;
  map<string,string> oi = GetDataBase()->ReadOriginsInfo(idx, false, false);
  string image_url = oi["url"];

  gvt__imageSimilarityUrl imageSimilarity_payload;
  imageSimilarity_payload.CollectionId = &collection_name;
  imageSimilarity_payload.maxResults = nn;
  imageSimilarity_payload.imageUrl = &image_url;
  gvt__imageSimilarityUrlResponse imageSimilarity_response;

  if (imageCollection->imageSimilarityUrl(&imageSimilarity_payload,
					  &imageSimilarity_response)
      == SOAP_OK) {
    GetDataBase()->WriteLog(msg+"ending");

    gvt__imageSimilarityResult *imageSimilarity_result
      = imageSimilarity_response.return_;
    if (debug>1)
      cout << *(imageSimilarity_result->message) << endl;
    vector<gvt__imageSimilarityId*> &similar_images
      = imageSimilarity_result->results;
    vector<gvt__imageSimilarityId*>::iterator it = similar_images.begin();

    for (;it < similar_images.end(); it++) {
      gvt__imageSimilarityId *imageSimilarityId = *it;
      if (debug>2)
	cout << *(imageSimilarityId->imageId) << "\t"
	     << imageSimilarityId->similarity << endl;
      bool ok = false;
      string uin = *imageSimilarityId->imageId, u = uin;
      size_t p = u.find("static.flickr.com");
      if (p!=string::npos) {
	p = u.find('/', p);
	if (p!=string::npos) {
	  p = u.find('/', p+1);
	  u.erase(0, p+1);
	  p = u.find('_');
	  if (p!=string::npos) {
	    u.erase(p);
	    u = string(10-u.size(), '0')+u;
	    int idxr = GetDataBase()->ToIndex(u);
	    if (idxr>=0) {
	      nretpos++;
	      ret[idxr] = 1;
	      ok = true;
	      AddCache(idx, nn, idxr);
	    }
	  }
	}
      }
      if (!ok && debug) {
	if (false)
	  ShowError(msg+"failed with <"+uin+"> -> <"+u+">");
	else
	  cerr << msg+"failed with <"+uin+"> -> <"+u+">" << endl;
      }
    }
  }
  else {
    soap_print_fault(imageCollection, stderr);
  }

  delete imageCollection;

  if (debug)
    cout << msg << "returning " << nretpos << " images" << endl;

  return ret;
}

//=============================================================================

ground_truth CbirGVT::GetGvtImagesMany(const vector<size_t>& idxv,
				       const vector<size_t>& nnv,
				       const ground_truth& allowed) {
  string msg = "CbirGVT::GetGvtImagesMany() : ";

  if (!idxv.size())
    return ground_truth();

  if (idxv.size()!=nnv.size()) {
    ShowError(msg+"vector dimensions differ");
    return ground_truth();
  }

  GetDataBase()->WriteLog(msg+"starting with "+ToStr(idxv.size())+" samples");

  vector<size_t> idxvres;
  vector<int>    nnvres;
  vector<string> image_url_vec;
  map<size_t,size_t> idx2nn;

  ground_truth ret(GetDataBase()->Size());

  size_t nallow = 0;
  for (size_t a=0; a<idxv.size(); a++) {
    bool hit = false;
    vector<size_t> cres = FindCache(idxv[a], nnv[a], hit);
    if (hit) {
      cout << msg << "a=" << a << " idx=" << idxv[a] << " nn=" << nnv[a]
	   << " cache hit, returning " << ToStr(cres.size()) << " images"
	   << endl;

      for (size_t i=0; i<cres.size(); i++) {
	size_t idx = cres[i];
	bool allow = allowed[idx];
	nallow += allow;
	if (debug>2)
	  cout << " index=" << idx << " allow=" << allow << endl;
	if (allow)
	  ret[idx] = 1;
      }

      continue;
    }
    idxvres.push_back(idxv[a]);
    nnvres.push_back(nnv[a]);
    idx2nn[idxv[a]] = nnv[a];

    map<string,string> oi = GetDataBase()->ReadOriginsInfo(idxv[a],
							   false, false);
    string image_url = oi["url"];
    image_url_vec.push_back(image_url);
  }

  GetDataBase()->WriteLog(msg+"found "+ToStr(nallow)+" images in the cache,"
			  " continuing with "+ToStr(nnvres.size())+" samples");

  string access_token;
  ImageCollectionServiceSoapBindingProxy *imageCollection =
    (ImageCollectionServiceSoapBindingProxy*)GetGvtImagesCommon(access_token);
  if (!imageCollection)
    return ground_truth();  

  string collection_name = "b12n";

  size_t nretpos = 0;

  gvt__imageSimilarityUrls imageSimilarity_payloads;
  imageSimilarity_payloads.CollectionId = &collection_name;
  imageSimilarity_payloads.maxResults = nnvres;
  imageSimilarity_payloads.imageUrl = image_url_vec;
  gvt__imageSimilarityUrlsResponse imageSimilarity_responses;

  // imageSimilarityUrls(collectionId, string[] imageUrls, int[] numResults);
  if (imageCollection->imageSimilarityUrls(&imageSimilarity_payloads,
					   &imageSimilarity_responses)
      == SOAP_OK) {
    GetDataBase()->WriteLog(msg+"ending");

    gvt__imageSimilarityResult *imageSimilarity_result
      = imageSimilarity_responses.return_;
    if (debug>1)
      cout << *(imageSimilarity_result->message) << endl;
    vector<gvt__imageSimilarityId*> &similar_images
      = imageSimilarity_result->results;
    vector<gvt__imageSimilarityId*>::iterator it = similar_images.begin();

    float prevsim = 0.0;
    size_t idxx = 0, nnx = 0;

    for (;it < similar_images.end(); it++) {
      gvt__imageSimilarityId *imageSimilarityId = *it;

      bool set_idxx = false;
      if (imageSimilarityId->similarity>prevsim)
	set_idxx = true;
      prevsim = imageSimilarityId->similarity;

      if (debug>2)
	cout << *(imageSimilarityId->imageId) << "\t"
	     << imageSimilarityId->similarity;
      bool ok = false;
      string uin = *imageSimilarityId->imageId, u = uin;
      size_t p = u.find("static.flickr.com");
      if (p!=string::npos) {
	p = u.find('/', p);
	if (p!=string::npos) {
	  p = u.find('/', p+1);
	  u.erase(0, p+1);
	  p = u.find('_');
	  if (p!=string::npos) {
	    u.erase(p);
	    u = string(10-u.size(), '0')+u;
	    int idxr = GetDataBase()->ToIndex(u);
	    if (idxr>=0) {
	      if (set_idxx) {
		idxx = idxr;
		nnx  = idx2nn[idxx];
	      }

	      bool allow = allowed[idxr];
	      nallow  += allow;
	      nretpos += allow;
	      if (debug>2)
		cout << " index=" << idxr << " allow=" << allow << endl;
	      if (allow)
		ret[idxr] = 1;

	      ok = true;
	      AddCache(idxx, nnx, idxr);
	    }
	  }
	}
      }
      if (debug>2)
	cout << endl;

      if (!ok && debug) {
	if (false)
	  ShowError(msg+"failed with <"+uin+"> -> <"+u+">");
	else
	  cerr << msg+"failed with <"+uin+"> -> <"+u+">" << endl;
      }
    }
  }
  else {
    soap_print_fault(imageCollection, stderr);
  }

  delete imageCollection;

  if (debug)
    cout << msg << "returning " << nretpos << " images" << endl;

  return ret;
}

//=============================================================================

ground_truth CbirGVT::GetGvtImagesManyNew(const vector<size_t>& idxv,
					  const vector<size_t>& nnv,
					  size_t nobj,
					  const ground_truth& allowed) {
  string msg = "CbirGVT::GetGvtImagesMany() : ";

  if (!idxv.size())
    return ground_truth();

  if (idxv.size()!=nnv.size()) {
    ShowError(msg+"vector dimensions differ");
    return ground_truth();
  }

  GetDataBase()->WriteLog(msg+"starting with "+ToStr(idxv.size())+" samples");

  vector<size_t> idxvres;
  vector<int>    nnvres;
  vector<string> image_url_vec;
  map<size_t,size_t> idx2nn;

  multimap<size_t,size_t> retmap;

  size_t nallow = 0;
  for (size_t a=0; a<idxv.size(); a++) {
    bool hit = false;
    vector<size_t> cres = FindCache(idxv[a], nnv[a], hit);
    if (hit) {
      cout << msg << "a=" << a << " idx=" << idxv[a] << " nn=" << nnv[a]
	   << " cache hit, returning " << ToStr(cres.size()) << " images"
	   << endl;

      for (size_t i=0; i<cres.size(); i++) {
	size_t idx = cres[i];
	bool allow = allowed[idx];
	nallow += allow;
	if (debug>2)
	  cout << " i=" << i << " index=" << idx << " allow=" << allow << endl;
	if (allow)
	  retmap.insert(make_pair(i, idx));
      }

      continue;
    }
    idxvres.push_back(idxv[a]);
    nnvres.push_back(nnv[a]);
    idx2nn[idxv[a]] = nnv[a];

    map<string,string> oi = GetDataBase()->ReadOriginsInfo(idxv[a],
							   false, false);
    string image_url = oi["url"];

    if (image_url=="")
      ShowError(msg+"image URL not resolved for image #"+ToStr(idxv[a]));

    image_url_vec.push_back(image_url);
  }

  GetDataBase()->WriteLog(msg+"found "+ToStr(nallow)+" images in the cache,"
			  " continuing with "+ToStr(nnvres.size())+" samples");

  if (nnvres.size()) {
    string access_token;
    ImageCollectionServiceSoapBindingProxy *imageCollection =
      (ImageCollectionServiceSoapBindingProxy*)GetGvtImagesCommon(access_token);
    if (!imageCollection)
      return ground_truth();  

    string collection_name = "b12n";

    gvt__imageSimilarityUrls imageSimilarity_payloads;
    imageSimilarity_payloads.CollectionId = &collection_name;
    imageSimilarity_payloads.maxResults = nnvres;
    imageSimilarity_payloads.imageUrl = image_url_vec;
    gvt__imageSimilarityUrlsResponse imageSimilarity_responses;

    // imageSimilarityUrls(collectionId, string[] imageUrls, int[] numResults);
    if (imageCollection->imageSimilarityUrls(&imageSimilarity_payloads,
					     &imageSimilarity_responses)
	== SOAP_OK) {
      GetDataBase()->WriteLog(msg+"ending");

      gvt__imageSimilarityResult *imageSimilarity_result
	= imageSimilarity_responses.return_;
      if (debug>1)
	cout << *(imageSimilarity_result->message) << endl;
      vector<gvt__imageSimilarityId*> &similar_images
	= imageSimilarity_result->results;
      vector<gvt__imageSimilarityId*>::iterator it = similar_images.begin();

      float prevsim = 0.0;
      size_t idxx = 0, nnx = 0;

      size_t j = 0;
      for (;it < similar_images.end(); it++) {
	gvt__imageSimilarityId *imageSimilarityId = *it;

	bool set_idxx = false;
	if (imageSimilarityId->similarity>prevsim)
	  set_idxx = true;
	prevsim = imageSimilarityId->similarity;

	if (debug>2)
	  cout << *(imageSimilarityId->imageId) << "\t"
	       << imageSimilarityId->similarity;
	bool ok = false;
	string uin = *imageSimilarityId->imageId, u = uin;
	size_t p = u.find("static.flickr.com");
	if (p!=string::npos) {
	  p = u.find('/', p);
	  if (p!=string::npos) {
	    p = u.find('/', p+1);
	    u.erase(0, p+1);
	    p = u.find('_');
	    if (p!=string::npos) {
	      u.erase(p);
	      u = string(10-u.size(), '0')+u;
	      int idxr = GetDataBase()->ToIndex(u);
	      if (idxr>=0) {
		if (set_idxx) {
		  idxx = idxr;
		  nnx  = idx2nn[idxx];
		  j = 0;
		}

		bool allow = allowed[idxr];
		nallow  += allow;
		if (debug>2)
		  cout << " j=" << j << " index=" << idxr
		       << " allow=" << allow << endl;
		if (allow)
		  retmap.insert(make_pair(j, idxr));

		ok = true;
		AddCache(idxx, nnx, idxr);

		j++;
	      }
	    }
	  }
	}
	if (debug>2)
	  cout << endl;

	if (!ok && debug) {
	  if (false)
	    ShowError(msg+"failed with <"+uin+"> -> <"+u+">");
	  else
	    cerr << msg+"failed with <"+uin+"> -> <"+u+">" << endl;
	}
      }
    }
    else {
      soap_print_fault(imageCollection, stderr);
    }

    delete imageCollection;
  }

  ground_truth ret(GetDataBase()->Size());

  size_t nretpos = 0;
  for (multimap<size_t,size_t>::const_iterator mi=retmap.begin();
       mi!=retmap.end() && nretpos<nobj; mi++) {
    if (debug>2)
      cout << " checking " << mi->first << " " << mi->second << " "
	   << (int)ret[mi->second] << " " << nretpos << endl;
    nretpos += ret[mi->second]!=1;
    ret[mi->second] = 1;
  }

  if (debug)
    cout << msg << "returning " << nretpos << " images from total of "
	 << nallow << " found" << endl;

  return ret;
}

// ============================================================================

ground_truth CbirGVT::GetGvtImages(const vector<size_t>& v,
				   const vector<size_t>& nn, size_t nobj,
				   const ground_truth& allowed) {
  bool one_by_one = false;

  if (one_by_one) {
    ground_truth ret;

    for (size_t i=0; i<v.size(); i++) {
      if (!i)
	ret = GetGvtImagesOneByOne(v[i], nn[i]);
      else
	ret = ret.TernaryOR(GetGvtImagesOneByOne(v[i], nn[i]));
    }
    return ret;
  }

  return GetGvtImagesManyNew(v, nn, nobj, allowed);
}

// ============================================================================

void CbirGVT::AddCache(size_t in, size_t n, size_t out) {
  string msg = "AddCache("+ToStr(in)+","+ToStr(n)+","+ToStr(out)+") : ";

  bool debugx = debug>1;

  if (false && debugx)
    cout << msg << endl;

  pair<cache_t::iterator,cache_t::iterator> r = cache.equal_range(in);
  for (cache_t::iterator i = r.first; i!=r.second; i++)
    if (i->second.first==n) {
      i->second.second.push_back(out);

      if (debugx)
	cout << msg << " added in old " << i->second.second.size()
	     << "/" << i->second.first << endl;
      return;
    }

  vector<size_t> v;
  v.push_back(out);
  cache.insert(make_pair(in, make_pair(n, v)));

  if (debugx)
    cout << msg << "added new entry, total " << cache.size() << endl;
}

// ============================================================================

vector<size_t> CbirGVT::FindCache(size_t in, size_t n, bool& hit) {
  string msg = "FindCache("+ToStr(in)+","+ToStr(n)+") : ";

  bool debugx = debug>1;

  if (false && debugx)
    cout << msg << endl;

  pair<cache_t::iterator,cache_t::iterator> r = cache.equal_range(in);
  for (cache_t::iterator i = r.first; i!=r.second; i++)
    if (i->second.first>=n) {
      hit = true;

      vector<size_t> ret;
      for (size_t j=0; j<n&&j<i->second.second.size(); j++)
	ret.push_back(i->second.second[j]);

      if (debugx)
	cout << msg << "returning " << ret.size() << "/"
	     << i->second.second.size() << " images" << endl;

      return ret;
    }

  hit = false;

  if (debugx)
    cout << msg << " not found" << endl;

  return vector<size_t>();
}

// ============================================================================

