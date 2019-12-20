// -*- C++ -*-  $Id: segmentfile.C,v 1.181 2019/02/04 09:10:34 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <segmentfile.h>
#include <math.h>

using std::string;
using std::vector;
using std::set;
using std::set_difference;
using std::map;
using std::multimap;
using std::cout;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::stringstream;
using std::pair;

namespace picsom {
  //
  Region *Region::list_of_types=NULL;

  // register the region types
  listRegion region_entry1(false);

  //
  rectangularRegion region_entry2(false);
  circularRegion region_entry3(false);

  ///////////////////////////////////////////////////////////////////////////

  rectangularRegion::rectangularRegion(const string& s,
				       const segmentfile *seg) throw(string) {
    const string msg = string("rectangularRegion(")+s+")";
    is_set = false;

    // cout << msg << endl;

    bool ok = false;

    try {
      ok = ok || SolvePoint(s, seg);
      ok = ok || SolveBox(  s, seg);
    }
    catch (const string& errs) {
      throw msg + errs;
    }

    if (!ok)
      throw msg + " both point and box interpretations failed";

    str = s;
    is_set = true;
  }

  //===========================================================================

  bool rectangularRegion::SolvePoint(const string& s,
				     const segmentfile *seg) throw(string) {
    string msg = "SolvePoint() : ";
    const char *p = s.c_str();

    if (isdigit(*p))
      return false;
    
    if (*p=='(') {
      int n;
      if (sscanf(p, "(%d,%d)%n", &x0, &y0, &n)==2)
	p += n;
      else
	p = NULL;
      
    } else {
      if (!seg)
	throw  msg + " : no segmentation results available";
      
      string name = s;
      string::size_type i = name.find_last_not_of("0123456789x");
      if (name[i]!=':')
	return false;

      name.erase(i);

      SegmentationResult *segres = NULL;
      if (!segres)
	segres = seg->readLastFrameResultFromXML(name, "point");

      if (segres) {
	int x, y;
	const char *val = segres->value.c_str();
	if (sscanf(val, "%d,%d", &x, &y)==2 ||
	    sscanf(val, "%d%d", &x, &y)==2) {
	  x0 = x;
	  y0 = y;
	} else {
	  delete segres;
	  throw msg + " point object <" + name + "> not found";
	}
      }
	
      if (!segres) {
	segres = seg->readLastFrameResultFromXML(name, "box");

	if (segres) {
	  int brx, bry;
	  const char *val = segres->value.c_str();
	  if (sscanf(val, "%d,%d,%d,%d", &ulx, &uly, &brx, &bry)!=4 &&
	      sscanf(val, "%d%d%d%d", &ulx, &uly, &brx, &bry)!=4) {
	    delete segres;
	    throw msg + " box object <" + s + "> not found";
	  }
	  
	  x0 = (ulx+brx)/2;
	  y0 = (uly+bry)/2;
	}
      }
      if (!segres)
	throw  msg + " : segmentation resultlist not available";

      delete segres;

      p += i;
    }

    if (!p || *p!=':' || x0==INT_MAX || y0==INT_MAX)
      throw msg + " : failed 2";

    if (sscanf(p+1, "%dx%d", &w, &h)!=2)
      throw msg + " : failed 3";

    ulx = x0-(w-1)/2;
    uly = y0-(h-1)/2;

    return true;
  }

  //===========================================================================

  bool rectangularRegion::SolveBox(const string& s,
				   const segmentfile *seg) throw(string) {
    string msg = "SolveBox() : ";
    const char *p = s.c_str();
    
    int brx, bry;

    if (isdigit(*p)) {
      if (sscanf(p, "%d,%d,%d,%d", &ulx, &uly, &brx, &bry)!=4)
	throw  msg + " : four coordinates not found";

    } else {
      if (!seg)
	throw  msg + " : no segmentation results available";

      SegmentationResult *segres =
	seg->readLastFrameResultFromXML(s, "box");
      if (!segres)
	return false;

      const char *val = segres->value.c_str();
      if (sscanf(val, "%d,%d,%d,%d", &ulx, &uly, &brx, &bry)!=4 &&
	  sscanf(val, "%d%d%d%d", &ulx, &uly, &brx, &bry)!=4) {
	delete segres;
	throw msg + " box object <" + s + "> not found";
      }

      delete segres;
    }

    w = brx-ulx+1;
    h = bry-uly+1;
    x0 = (brx+ulx)/2;
    y0 = (bry+uly)/2;

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  float *NR::vectorPtrsFromData(float *d, int l, int /*u*/){
    return d-l;
  }

  ///////////////////////////////////////////////////////////////////////////
  float **NR::matrixPtrsFromData(float *d, 
				 int l1, int u1,int l2, int u2){

    typedef float *fptr;

    size_t s1=u1-l1+1;
    size_t s2=u2-l2+1;

    fptr *ret=new fptr[s1];
    int offs=0;
    for(size_t i=0;i<s1;i++){
      ret[i]=vectorPtrsFromData(d+offs,l2,u2);
      offs += s2;
    }

    return ret-l1;

  }

  ///////////////////////////////////////////////////////////////////////////

  float ***NR::tensor3PtrsFromData(float *d, int l1, int u1,
				   int l2, int u2, int l3, int u3){


    typedef float *fptr;
    typedef fptr *pfptr;

    size_t s1=u1-l1+1;
    size_t s2=u2-l2+1;
    size_t s3=u3-l3+1;

    size_t incr=s2*s3;

    pfptr *ret=new pfptr[s1];
    int offs=0;
    for(size_t i=0;i<s1;i++){
      ret[i]=matrixPtrsFromData(d+offs,l2,u2,l2,u2);
      offs += incr;
    }

    return ret-l1;

  }

  ///////////////////////////////////////////////////////////////////////////

  void NR::freeMatrixPtrs(float **m,int l1, int /*u1*/,
			  int /*l2*/, int /*u2*/){
    delete[] (m+l1);
  }

  ///////////////////////////////////////////////////////////////////////////

  void NR::freeTensor3Ptrs(float ***t,int l1, int u1,
			   int l2, int u2, int l3, int u3){
    for(int i=l1;i<=u1;i++)
      freeMatrixPtrs(t[i],l2,u2,l3,u3);
    delete[] (t+l1); 
  }

  ///////////////////////////////////////////////////////////////////////////

  void NR::rlft3(float ***data, float **speq, 
			unsigned long nn1, unsigned long nn2, 
			unsigned long nn3, int isign){
    
    // copied from numerical recipes

    unsigned long i1,i2,i3,j1,j2,j3,nn[4],ii3;
    double theta,wi,wpi,wpr,wr,wtemp;
    float c1,c2,h1r,h1i,h2r,h2i;

    if (((size_t)(1+&data[nn1][nn2][nn3]-&data[1][1][1])) != ((size_t)(nn1*nn2*nn3)))
      throw string ("rlft3: problem with dimensions or contiguity of data array\n");
    c1=0.5;
    c2 = -0.5*isign;
    theta=isign*(6.28318530717959/nn3);
    wtemp=sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi=sin(theta);
    nn[1]=nn1;
    nn[2]=nn2;
    nn[3]=nn3 >> 1;
    if (isign == 1) { // Case of forward transform.
      //Here is where most all of the compute time is spent. 
      fourn(&data[1][1][1]-1,nn,3,isign); 
      for (i1=1;i1<=nn1;i1++) 
	for (i2=1,j2=0;i2<=nn2;i2++) { // Extend data periodically into speq.
	  speq[i1][++j2]=data[i1][i2][1];
	  speq[i1][++j2]=data[i1][i2][2];
	}
    }
    for (i1=1;i1<=nn1;i1++) {
      j1=(i1 != 1 ? nn1-i1+2 : 1);
      // Zero frequency is its own reflection, 
      // otherwise locate corresponding negative frequency
      // in wrap-around order.
      wr=1.0; //Initialize trigonometric recurrence.
      wi=0.0;
      for (ii3=1,i3=1;i3<=(nn3>>2)+1;i3++,ii3+=2) {
	for (i2=1;i2<=nn2;i2++) {
	  if (i3 == 1) { // Equation (12.3.5).
	    j2=(i2 != 1 ? ((nn2-i2)<<1)+3 : 1);
	    h1r=c1*(data[i1][i2][1]+speq[j1][j2]);
	    h1i=c1*(data[i1][i2][2]-speq[j1][j2+1]);
	    h2i=c2*(data[i1][i2][1]-speq[j1][j2]);
	    h2r= -c2*(data[i1][i2][2]+speq[j1][j2+1]);
	    data[i1][i2][1]=h1r+h2r;
	    data[i1][i2][2]=h1i+h2i;
	    speq[j1][j2]=h1r-h2r;
	    speq[j1][j2+1]=h2i-h1i;
	  } else {
	    j2=(i2 != 1 ? nn2-i2+2 : 1);
	    j3=nn3+3-(i3<<1);
	    h1r=c1*(data[i1][i2][ii3]+data[j1][j2][j3]);
	    h1i=c1*(data[i1][i2][ii3+1]-data[j1][j2][j3+1]);
	    h2i=c2*(data[i1][i2][ii3]-data[j1][j2][j3]);
	    h2r= -c2*(data[i1][i2][ii3+1]+data[j1][j2][j3+1]);
	    data[i1][i2][ii3]=h1r+wr*h2r-wi*h2i;
	    data[i1][i2][ii3+1]=h1i+wr*h2i+wi*h2r;
	    data[j1][j2][j3]=h1r-wr*h2r+wi*h2i;
	    data[j1][j2][j3+1]= -h1i+wr*h2i+wi*h2r;
	  }
	}
	wr=(wtemp=wr)*wpr-wi*wpi+wr; //Do the recurrence.
	wi=wi*wpr+wtemp*wpi+wi;
      }
    }
    if (isign == -1) //Case of reverse transform.
      fourn(&data[1][1][1]-1,nn,3,isign);
  }

  ///////////////////////////////////////////////////////////////////////////

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

  void NR::fourn(float data[], unsigned long nn[], int ndim, int isign){

    // copied from Numerical Recipes

    int idim;
    unsigned long i1,i2,i3,i2rev,i3rev,ip1,ip2,ip3,ifp1,ifp2;
    unsigned long ibit,k1,k2,n,nprev,nrem,ntot;
    float tempi,tempr;

    // Double precision for trigonometric recurrences.
    double theta,wi,wpi,wpr,wr,wtemp; 
    
    //Compute total number of complex values.
    for (ntot=1,idim=1;idim<=ndim;idim++) 
      ntot *= nn[idim];
    nprev=1;
    for (idim=ndim;idim>=1;idim--) { //Main loop over the dimensions.
      n=nn[idim];
      nrem=ntot/(n*nprev);
      ip1=nprev << 1;
      ip2=ip1*n;
      ip3=ip2*nrem;
      i2rev=1;
      for (i2=1;i2<=ip2;i2+=ip1) { 
	//This is the bit-reversal section of the routine. 
	if (i2 < i2rev) {
	  for (i1=i2;i1<=i2+ip1-2;i1+=2) {
	    for (i3=i1;i3<=ip3;i3+=ip2) {
	      i3rev=i2rev+i3-i2;
	      SWAP(data[i3],data[i3rev]);
	      SWAP(data[i3+1],data[i3rev+1]);
	    }
	  }
	}
	ibit=ip2 >> 1;
	while (ibit >= ip1 && i2rev > ibit) {
	  i2rev -= ibit;
	  ibit >>= 1;
	}
	i2rev += ibit;
      }
      ifp1=ip1; 

      // Here begins the Danielson-Lanczos section of the routine. 
      while (ifp1 < ip2) {
	ifp2=ifp1 << 1;
	// Initialize for the trig. recurrence.
	theta=isign*6.28318530717959/(ifp2/ip1); 
	wtemp=sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi=sin(theta);
	wr=1.0;
	wi=0.0;
	for (i3=1;i3<=ifp1;i3+=ip1) {
	  for (i1=i3;i1<=i3+ip1-2;i1+=2) {
	    for (i2=i1;i2<=ip3;i2+=ifp2) {
	      k1=i2; //Danielson-Lanczos formula:
	      k2=k1+ifp1;
	      tempr=(float)wr*data[k2]-(float)wi*data[k2+1];
	      tempi=(float)wr*data[k2+1]+(float)wi*data[k2];
	      data[k2]=data[k1]-tempr;
	      data[k2+1]=data[k1+1]-tempi;
	      data[k1] += tempr;
	      data[k1+1] += tempi;
	    }
	  }
	  wr=(wtemp=wr)*wpr-wi*wpi+wr; // Trigonometric recurrence.
	  wi=wi*wpr+wtemp*wpi+wi;
	}
	ifp1=ifp2;
      }
      nprev *= n;
    }
  }
   
  ///////////////////////////////////////////////////////////////////////////

  void real2DFFTData::transform(int exp){

    // check that N is a power of two

    int log2;
    for(log2=0;1<<log2<N;log2++){}
    if(N!=(1<<log2))
      throw string("real2DFFTData::transform(): N must be power of 2.");


    float ***d, **speq;
    d=NR::tensor3PtrsFromData(data,1,1,1,N,1,N);
    //cout << "offset[1][1][1]=" << (int)((&d[1][1][1])-data) << endl;
    //cout << "offset[1][1][N]=" << (int)((&d[1][1][N])-data) << endl;
    //cout << "offset[1][2][1]=" << (int)((&d[1][2][1])-data) << endl;
    speq=NR::matrixPtrsFromData(extracoeff,1,1,1,2*N);
    NR::rlft3(d,speq,1,N,N,exp);
    NR::freeMatrixPtrs(speq,1,1,1,2*N);
    NR::freeTensor3Ptrs(d,1,1,1,N,1,N);
  }
 
 
  ///////////////////////////////////////////////////////////////////////////
  bool Draw::LinePixelsToList(coord p1,coord p2,vector<coord_float> &list)
  {
    if(p1.x==p2.x && p1.y == p2.y){
      list.push_back(p1); 
      return true;
    }

    if(abs(p1.x-p2.x)>abs(p1.y-p2.y)){
      
      int xstep =(p1.x>p2.x)?-1:1;

      // we step the x coordinate as delta_x > delta_y

      float step=xstep*((float)(p2.y-p1.y))/(p2.x-p1.x);

      int x;
      float y=p1.y;

      for(x=p1.x;xstep*x<=xstep*p2.x;x+=xstep){
	list.push_back(coord_float(x,y));
	y += step;
      }
    }

    else{
      // we step the y coordinate as delta_y >= delta_x
      
      int ystep=(p1.y>p2.y)?-1:1;
      
      float step=ystep*((float)(p2.x-p1.x))/(p2.y-p1.y);
      
      int y;
      float x=p1.x;
      
      for(y=p1.y;ystep*y<=ystep*p2.y;y+=ystep){
	list.push_back(coord_float(x,y));
	x += step;
      }
    }
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool Draw::LinePixelsToList(coord p1,
			      coord p2, 
			      vector<coord> &list)
  {
    // integer version
      
    if(p1.x==p2.x && p1.y == p2.y){
      list.push_back(p1); 
      return true;
    }
      
    if(abs(p1.x-p2.x)>abs(p1.y-p2.y))
      return IntegerPixelsToList(p1,p2,list,false);
    else
      return IntegerPixelsToList(coord(p1.y,p1.x),coord(p2.y,p2.x),list,true);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool Draw::IntegerPixelsToList(coord p1,coord p2,vector<coord> &list,bool flip)     {
  
      int x,y;
      
      int xstep =(p1.x>p2.x)?-1:1;
      int ystep =(p1.y>p2.y)?-1:1;
  
      int x_len=xstep*(p2.x-p1.x);
      int y_len=ystep*(p2.y-p1.y);
      
      int error = - x_len/2;
      y=p1.y;       
   
      for(x=p1.x;xstep*x <= xstep*p2.x;x +=xstep){
	if(flip)
	  list.push_back(coord(y,x));
	else
	  list.push_back(coord(x,y));
	
	error += y_len;
	if(error >= 0){
	  y += ystep;
	  error -= x_len;
	}
      } // for
      
      return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  int Draw::FreemanCodeFromPoints(coord &p1, coord &p2)
  {
      
      switch(p2.x-p1.x){
      case -1:
	switch(p2.y-p1.y){
	case -1:
	  return 3;
	case 0:
	  return 4;
	case 1:
	  return 5;
	default:
	  cout << "FreemancodefromPoints: points not adjacent" << endl;
	  return -1;
	}
      case 0:
	switch(p2.y-p1.y){
	case -1:
	  return 2;
	case 1:
	  return 6;
	default:
	  cout << "FreemancodefromPoints: points not adjacent" << endl;
	  return -1;
	}
      case 1:
	switch(p2.y-p1.y){
	case -1:
	  return 1;
	case 0:
	  return 0;
	case 1:
	  return 7;
	default:
	  cout << "FreemancodefromPoints: points not adjacent" << endl;
	  return -1;
	}
      default:
	cout << "FreemancodefromPoints: points not adjacent" << endl;
	return -1;
      } // switch(x)
      
  }

  ///////////////////////////////////////////////////////////////////////////
  xmlNodePtr Option::produceXML() const{
    string errhead("Option::produceXML() : ");

    xmlNodePtr node = xmlNewNode(NULL,(xmlChar *)"option");
    if(!node) throw errhead + "XML node creation failed";

    xmlNewTextChild(node,NULL,(xmlChar*)"optionname",(xmlChar*)option.c_str());


    vector<string>::const_iterator it;
    for(it=arglist.begin();it!=arglist.end();it++){
      xmlNewTextChild(node,NULL,(xmlChar*)"argument",(xmlChar*)(it->c_str()));
    }
    return node;

  }

  ///////////////////////////////////////////////////////////////////////////
  bool Option::parseXML(const xmlNodePtr n){

    if(XMLTools::strcmpXML(n->name,"option")) return false;
    
    xmlNodePtr namenode=XMLTools::xmlNodeFindDescendant(n,"optionname");
    if(!namenode) return false;

    option=XMLTools::xmlNodeGetContent(namenode);

    arglist.clear();

    for(xmlNodePtr nn=XMLTools::xmlNodeFindDescendant(n,"argument");
	nn!= NULL; nn=XMLTools::xmlNodeFindNextSibling(nn,"argument"))
          arglist.push_back(XMLTools::xmlNodeGetContent(nn));

    return true;

  }

  ///////////////////////////////////////////////////////////////////////////
  xmlNodePtr Optionlist::produceXML() const{
    string errhead("Optionlist::produceXML() : ");

    xmlNodePtr node = xmlNewNode(NULL,(xmlChar*)"optionlist");
    if(!node) throw errhead + "XML node creation failed";

    vector<Option>::const_iterator it;

    for(it=list.begin();it!=list.end();it++)
      xmlAddChild(node,it->produceXML());

    return node;

  }
  ///////////////////////////////////////////////////////////////////////////

  bool Optionlist::parseXML(const xmlNodePtr n){

    if(XMLTools::strcmpXML(n->name,"optionlist")) return false;

    list.clear();

    for(xmlNodePtr nn=XMLTools::xmlNodeFindDescendant(n,"option");
	nn!= NULL; nn=XMLTools::xmlNodeFindNextSibling(nn,"option")){
      Option tmp;
      if(!tmp.parseXML(nn)) return false;
      list.push_back(tmp);
    }

    return true;

  }

  ///////////////////////////////////////////////////////////////////////////
  void string2intvector(const string &s, vector<int> &v){
    v.clear();

    size_t offset=0;
    int i;

    while(s[offset]){
      while(s[offset] && !isintchar(s[offset])) offset++;
      //      cout << "offset="<<offset<<endl;
      if(sscanf(s.c_str()+offset,"%d",&i)==1)
	v.push_back(i);
      while(s[offset] && !isintchar(s[offset])) offset++;
      while(s[offset] && isintchar(s[offset])) offset++;
    }
  }
  ///////////////////////////////////////////////////////////////////////////
  void string2floatvector(const string &s, vector<float> &v){
    v.clear();

    size_t offset=0;
    float f;

    while(s[offset]){
      while(s[offset] && isspace(s[offset])) offset++;
      if(sscanf(s.c_str()+offset,"%f",&f)==1)
	v.push_back(f);
      while(s[offset] && !isspace(s[offset])) offset++;
      while(s[offset] && isspace(s[offset])) offset++;
    }
  }
  ///////////////////////////////////////////////////////////////////////////

 bool isintchar(char c){
    if(c>='0'&&c<='9') return true;
    if(c=='-'|| c =='+') return true;
    return false;
  }

  ///////////////////////////////////////////////////////////////////////////
  bool IsAffirmativeSeg(const string& str) {
      return str=="y" || str=="yes" || str=="Y" || str=="YES" || str=="Yes"
	|| str=="t" || str=="true" || str=="T" || str=="TRUE" || str=="True"
	|| str=="1";
    }
  ///////////////////////////////////////////////////////////////////////////

  void namespace_spec::split_url(const string &url, 
				 string &version_independent, 
				 namespace_spec::version &v){

    string errhead("namespace_spec::split_url() : ");

    string::size_type slash = url.find_last_of('/');
    if(slash==string::npos)
      throw errhead + "final slash not found";

    slash=url.find_last_of('/',slash-1);
    if(slash==string::npos)
      throw errhead + "dividing slash not found";

    // slash now has the position of the second last slash
      
    version_independent = url.substr(0,slash+1);

    string v_str = url.substr(slash+1);

    // ensure that the version string corresponds to spec XXX.YYY/
    
    size_t pos=0;
    while(isdigit(*(v_str.c_str()+pos))) pos++;
    if(pos==0 || *(v_str.c_str()+pos) != '.' || 
       sscanf(v_str.c_str(),"%d",&v.major)!= 1)
      throw errhead + "couldn't parse major version number (v_str="+v_str+")"; 

    pos++;
    size_t minor_start=pos;

    while(isdigit(*(v_str.c_str()+pos))) pos++;
    if(pos==minor_start || sscanf(v_str.c_str()+minor_start,"%d",&v.minor)!= 1)
      throw errhead + "couldn't parse minor version number";
  
    if(*(v_str.c_str()+pos) != '/')
      throw errhead +"final slash not found";
      
    if(v_str.length() > pos+1)
      throw errhead + "extra characters in input";
  }   

 ///////////////////////////////////////////////////////////////////////////

  int XMLTools::strcmpXML(const xmlChar *s1, const char *s2) {
    return strcmp((const char*)s1, s2);
  }

  ///////////////////////////////////////////////////////////////////////////

  char *XMLTools::XMLToString(xmlDocPtr doc,bool pretty)
  {
    int tree_was = xmlIndentTreeOutput;
    xmlIndentTreeOutput = pretty;
    
    xmlChar *dump;
    int dumplen;
    xmlDocDumpFormatMemory(doc, &dump, &dumplen, pretty);
    xmlIndentTreeOutput = tree_was;

    return (char*)dump;
  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr XMLTools::xmlNodeFindDescendant( xmlNodePtr parent,
							  const char *path)
  {
    // path is supposed to be in the form "node1->node2->node3";
    //cout<<"XMLTools::xmlNodeFindDescendant(" << path 
    //    << ")" << endl;

    vector<string> p;

    size_t l = strlen(path);
    
    char *cpath = new char[l+1];
    strcpy(cpath,path); 
    
    int start=0;

    for(size_t i=1;i<l;i++)
      if(cpath[i]=='-' && cpath[i+1]=='>'){
	cpath[i]=0;
	p.push_back(cpath+start);
	start=i+2;
      }

    p.push_back(cpath+start);

    delete[] cpath;
    return xmlNodeFindDescendant(parent,p);
  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr XMLTools::xmlNodeFindDescendant( xmlNodePtr parent,
					      vector<string> &path)
  {
    if(!parent) return parent;
    
    xmlNodePtr ret=parent;

    for (size_t i=0; i<path.size(); i++) {
      ret=ret->children;

      while(ret&&strcmpXML(ret->name,path[i].c_str())){
	ret=ret->next;
	if(!ret) return ret;
      }
      if(!ret) return ret;

    }

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr XMLTools::xmlNodeFindNextSibling(xmlNodePtr curr,
						   const char *name)
  {

    if(!curr) return curr;

    xmlNodePtr ret=curr->next;

    while(ret&&strcmpXML(ret->name,name)){
      ret=ret->next;
    }

    return ret;

  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr XMLTools::xmlDocGetSegmentation(xmlDocPtr doc){
    xmlNodePtr segm = xmlDocGetRootElement(doc);
    while(segm&&strcmpXML(segm->name,"segmentation"))
      segm = segm->next;
    
    return segm;
  }


  ///////////////////////////////////////////////////////////////////////////
  string XMLTools::xmlNodeGetContent(xmlNodePtr node){
    char *content = (char*)::xmlNodeGetContent(node);
    string ret;
    if(content) {
      ret=content;
      free(content);
    }
    return ret;
      
  }

  ///////////////////////////////////////////////////////////////////////////
  int XMLTools::xmlNodeGetInt(xmlNodePtr node){
    char *content = (char*)::xmlNodeGetContent(node);
    int ret;
    if(!content || sscanf(content,"%d",&ret)!= 1) 
      throw string("XMLTools::xmlNodeGetInt failed");

    free(content);

    return ret;
  }
  ///////////////////////////////////////////////////////////////////////////
  void XMLTools::removeBlankNodes(xmlDocPtr doc){
    recursivelyRemoveBlanks(xmlDocGetRootElement(doc));
  }
  ///////////////////////////////////////////////////////////////////////////

    void XMLTools::recursivelyRemoveBlanks(xmlNodePtr n)
      {
	if(!n) 
	  throw string("XMLTools:recursivelyRemoveBlanks():")
	    +" NULL pointer encounted";
	
	if(xmlIsBlankNode(n)){
	//||isFormatting(n)){
	  
	 xmlNodePtr n2,nn;
	 for(n2=n->children;n2;n2=nn){
	   nn=n2->next;
	   recursivelyRemoveBlanks(n2);
	 }

	 xmlUnlinkNode(n);
	 xmlFreeNode(n);
	}
       else{
	 xmlNodePtr n2,nn;
	 for(n2=n->children;n2;n2=nn){
	   nn=n2->next;
	   recursivelyRemoveBlanks(n2);
	 }
       }
      }  
  

  ///////////////////////////////////////////////////////////////////////////

  void XMLTools::showXMLTree(xmlNodePtr start,const  string &indent){
    
    displayNodeInfo(start,indent);
    
    cout << indent << "Children:" << endl;
    
    xmlNodePtr n;
    for(n=start->children;n;n=n->next)
      showXMLTree(n,indent+"== ");

  }

  bool  XMLTools::isFormatting(xmlNodePtr n){
    if(n->type != XML_TEXT_NODE) return false;
    xmlChar *ptr,*c;
    ptr=c=::xmlNodeGetContent(n);
    if(!ptr) return true;
    while(*ptr){
      if(!isspace(*ptr)) break;;
      ptr++;
    }
    xmlFree(c);
    return (*ptr==0);
  }

  void XMLTools::displayNodeInfo(xmlNodePtr n,const string &indent){
    cout << indent << "name: " << n->name;
    cout << " type: ";
    switch(n->type){
    case XML_ELEMENT_NODE: 
      cout<<"XML_ELEMENT_NODE"; break; 
    case  XML_ATTRIBUTE_NODE:
      cout<<"XML_ATTRIBUTE_NODE"; break;
    case XML_TEXT_NODE:
      cout<<"XML_TEXT_NODE"; break;
    case XML_CDATA_SECTION_NODE:
      cout<<"XML_CDATA_SECTION_NODE"; break;
    case XML_ENTITY_REF_NODE:
      cout<<"XML_ENTITY_REF_NODE"; break;
    case XML_ENTITY_NODE:
      cout<<"XML_ENTITY_NODE"; break;
    case XML_PI_NODE:
      cout<<"XML_PI_NODE"; break;
    case XML_COMMENT_NODE:
      cout<<"XML_COMMENT_NODE"; break;
    case XML_DOCUMENT_NODE:
      cout<<"XML_DOCUMENT_NODE"; break;
    case XML_DOCUMENT_TYPE_NODE:
      cout<<"XML_DOCUMENT_TYPE_NODE"; break;
    case XML_DOCUMENT_FRAG_NODE:
      cout<<"XML_DOCUMENT_FRAG_NODE"; break;
    case XML_NOTATION_NODE:
      cout<<"XML_NOTATION_NODE"; break;
    case XML_HTML_DOCUMENT_NODE:
      cout<<"XML_HTML_DOCUMENT_NODE"; break;
    case XML_DTD_NODE:
      cout<<"XML_DTD_NODE"; break;
    case XML_ELEMENT_DECL:
      cout<<"XML_ELEMENT_DECL"; break;
    case XML_ATTRIBUTE_DECL:
      cout<<"XML_ATTRIBUTE_DECL"; break;
    case XML_ENTITY_DECL:
      cout<<"XML_ENTITY_DECL"; break;
    case XML_NAMESPACE_DECL:
      cout<<"XML_NAMESPACE_DECL"; break;
    case XML_XINCLUDE_START:
      cout<<"XML_XINCLUDE_START"; break;
    case XML_XINCLUDE_END:
      cout<<"XML_XINCLUDE_END"; break;
    case XML_DOCB_DOCUMENT_NODE:
      cout<<"XML_DOCB_DOCUMENT_NODE"; break;
    }

    cout << endl;
    cout << indent << "isBlank: " << 
      ((xmlIsBlankNode(n)==1) ? "YES" : "NO") << endl; 
    cout << indent << "isFormatting:";
    if(isFormatting(n)) cout << "YES";
    else cout << "NO";
    cout << endl;
    cout << indent+"content:";
    xmlChar *c = ::xmlNodeGetContent(n);
    if(c){ cout << c; xmlFree(c);} else cout << "none";
    cout << endl;

  }
  ///////////////////////////////////////////////////////////////////////////
  void XMLTools::dumpNode(xmlDocPtr doc,xmlNodePtr node){

    xmlBufferPtr b= xmlBufferCreate();
    xmlNodeDump(b,doc,node,0,1);
    cout << xmlBufferContent(b) << endl;
    xmlBufferFree(b);
  }

  ///////////////////////////////////////////////////////////////////////////

  PredefinedTypes::PredefinedTypes(){
    v.push_back("point");
    v.push_back("line");
    v.push_back("fd");
    v.push_back("box");
    v.push_back("rotated-box");
    v.push_back("coordlist");
    v.push_back("pixelchain");
    v.push_back("region");
    v.push_back("regionhierarchy");
    v.push_back("virtualsegment");
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Point::parse(const string &s) {
    const char *ptr = s.c_str();
    if ((sscanf(ptr,"%d%d",&x,&y) != 2) &&
	(sscanf(ptr,"%d,%d",&x,&y) != 2)) 
      throw string("Error in Point::parse()");
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Point::write(string &s) const {
    ostringstream ss;
    ss <<x<<" "<<y;
    s=ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Line::parse(const string &s) {
    const char *ptr = s.c_str();
    if((sscanf(ptr,"%d%d%d%d",&x1,&y1,&x2,&y2) != 4) &&
       (sscanf(ptr,"%d,%d,%d,%d",&x1,&y1,&x2,&y2) != 4)) 
      throw string("Error in Line::parse()"); 
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Line::write(string &s) const {
    ostringstream ss;
    ss <<x1<<" "<<y1<<" "<<x2<<" "<<y2;
    s=ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Box::parse(const string &s){
    const char *ptr=s.c_str();
  
    if((sscanf(ptr,"%d%d%d%d",&ulx,&uly,&brx,&bry) != 4) &&
       (sscanf(ptr,"%d,%d,%d,%d",&ulx,&uly,&brx,&bry) != 4)) 
      throw string("Error in Box::parse(): s=")+s;
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Box::write(string &s) const {
    ostringstream ss;
    ss <<ulx<<" "<<uly<<" "<<brx<<" "<<bry;
    s=ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  PredefinedTypes::Box 
  PredefinedTypes::Box::findBoundingBox(const coordList &l) {
    Box ret;

    size_t n=l.size();
    if(n){
      ret.ulx=ret.brx=l[0].x;
      ret.uly=ret.bry=l[0].y;
      
      for(size_t i=1;i<n;i++){
	if(l[i].x<ret.ulx) ret.ulx=l[i].x;
	if(l[i].x>ret.brx) ret.brx=l[i].x;
	if(l[i].y<ret.uly) ret.uly=l[i].y;
	if(l[i].y>ret.bry) ret.bry=l[i].y;
      }
    }
    return ret;

  }
  ///////////////////////////////////////////////////////////////////////////
  float  PredefinedTypes::Box::distanceTo(const Box &o) const {

    bool x_overlap=false;
    bool y_overlap=false;
    
    if(ulx<o.ulx){
      if(o.ulx <=brx) x_overlap=true;
    }
    else
      if(ulx <= o.brx) x_overlap=true;

    if(uly<o.uly){
      if(o.uly <=bry) y_overlap=true;
    }
    else
      if(uly <= o.bry) y_overlap=true;


    if(x_overlap && y_overlap) return 0;  // bb's touch

    int dx,dy;

    if(ulx<o.ulx) dx=o.ulx-brx;
    else dx=ulx-o.brx;
    if(dx<0) dx=-dx;

    if(uly<o.uly) dy=o.uly-bry;
    else dy=uly-o.bry;
    if(dy<0) dy=-dy;

    if(x_overlap) return dy;
    if(y_overlap) return dx;

    return sqrt((double)(dx*dx+dy*dy));

  }

  ///////////////////////////////////////////////////////////////////////////
  int  PredefinedTypes::Box::overlapWith(const Box &o) const {

    bool x_overlap=false;
    bool y_overlap=false;
    
    if(ulx<o.ulx){
      if(o.ulx <=brx) x_overlap=true;
    }
    else
      if(ulx <= o.brx) x_overlap=true;

    if(uly<o.uly){
      if(o.uly <=bry) y_overlap=true;
    }
    else
      if(uly <= o.bry) y_overlap=true;


    if(!x_overlap || !y_overlap) return 0;  // bb's do not overlap

    PredefinedTypes::Box c; // common area

    c.ulx=max(ulx,o.ulx);
    c.uly=max(uly,o.uly);
    c.brx=min(brx,o.brx);
    c.bry=min(bry,o.bry);

    return c.area();

  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::RotatedBox::parse(const string &s){
     const char *ptr=s.c_str();
     if((sscanf(ptr,"%d%d%d%d%f%f%f",&l,&t,&w,&h,
		&theta,&xc,&yc) != 7) &&
	(sscanf(ptr,"%d,%d,%d,%d,%f,%f,%f",&l,&t,&w,&h,
		&theta,&xc,&yc) != 7)) 
       throw string("Error in RotatedBox::parse()");
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::RotatedBox::write(string &s) const {
    ostringstream ss;
    ss <<l<<" "<<t<<" "<<w<<" "<<h<<" "<<theta<<" "<<xc<<" "<<yc;
    s=ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::CoordList::parse(const string &s) {
    istringstream ss(s);
    ss >> l;
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::CoordList::write(std::string &s) const {
    ostringstream ss;
    ss <<l;
    s=ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Region::parse(const std::string &s){

    size_t col=s.find(':');
    if(col != string::npos){
      id=s.substr(0,col);
      value=s.substr(col+1);
    }
    else throw string("Error in Region::parse()");
  }

 

  ///////////////////////////////////////////////////////////////////////////

  void PredefinedTypes::Region::write(std::string &s) const {
    ostringstream ss;
    ss <<id<<":"<<value;
    s=ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void ipList::parseFromXML(segmentfile &sf,const string &name){

    xmlNodePtr resnode=sf.accessLastFrameResultNode(name,"iplist");

    if(resnode)
      parseFromXMLNode(resnode);
    else
      reset();

  }

  ///////////////////////////////////////////////////////////////////////////

  void ipList::parseFromXMLNode(xmlNodePtr n){

    reset();

    xmlNodePtr lNode=XMLTools::xmlNodeFindDescendant(n,"iplist");
    if(!lNode) return;

   xmlChar *prop=xmlGetProp(lNode,(const xmlChar*)"fields");
   if(prop){

  
     setHeaderAndParse((char*)prop);
     size_t fieldcount=fieldname2idx.size();
     xmlFree(prop);

     for(xmlNodePtr pNode=XMLTools::xmlNodeFindDescendant(lNode,"ipoint");
	 pNode; pNode=XMLTools::xmlNodeFindNextSibling(pNode,"ipoint")){
       string c=XMLTools::xmlNodeGetContent(pNode);
       
       vector<float> v;
       string2floatvector(c,v);

       if(v.size() != fieldcount){
	 throw string("Error parsing ipoint fields from ")+c;
       }
       list.push_back(v);

     }
   }
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void ipList::setHeaderAndParse(const string &h){
    const char *ptr=h.c_str();
    size_t fieldcount=0;
    headerstring=h;
    while(*ptr){
      for(;*ptr&&isspace(*ptr);ptr++){}
      char fieldname[500];
      if(sscanf(ptr,"%s",fieldname)==1){
	idx2fieldname[fieldcount]=fieldname;
	fieldname2idx[fieldname]=fieldcount++;
      }
      for(;*ptr&&!isspace(*ptr);ptr++){}
    }

  }


  ///////////////////////////////////////////////////////////////////////////

  void ipList::writeToXML(const ChainedMethod *method,segmentfile &sf,const string &name){

    SegmentationResult res;
    res.name=name;
    res.type="iplist";
    res.value="";

      xmlNodePtr resNode=sf.writeFrameResultToXML(method,res);

      
      xmlNodePtr lNode=xmlNewTextChild(resNode,NULL,
				       (xmlChar*)"iplist",
				       NULL);

      // form header string

      headerstring="";
      for(size_t f=0;f<fieldname2idx.size();f++){
	if(f) headerstring += " ";
	headerstring += idx2fieldname[f];
      }

      xmlSetProp(lNode,(xmlChar*)"fields",(xmlChar*)headerstring.c_str());



      for(size_t i=0;i<list.size();i++){

	string c;
	char nstr[80];
	vector<float> &v=list[i];
	for(size_t d=0;d<v.size();d++){
	  sprintf(nstr,"%f",v[d]);
	  if(d) c += " ";
	  c += nstr;
	}
	xmlNewTextChild(lNode,NULL,
			(xmlChar*)"ipoint",
			(xmlChar*)c.c_str());
      }      


  }

  ///////////////////////////////////////////////////////////////////////////

  void ipList::reset(){
    list.clear();
    headerstring="";
    fieldname2idx.clear();
    idx2fieldname.clear();
  }
  
  ///////////////////////////////////////////////////////////////////////////


 vector<set<int> > 
 segmentfile::flattenHierarchyToPrimitiveRegions(bool alterbitmap){


   // write primitive regions to bitmap if allowed and 
   // collect list of composite segments as sets of primitive segments

   vector<set<int> > ret;

   xmlNodePtr hNode=accessLastFrameResultNode("","regionhierarchy");

   if(!hNode) throw string("regionhierarchy not found in xml");

   //   vector<dr_merge> mergeList;

   // parse hierarchy from xml
   // start from leaf regions
   xmlNodePtr lNode=XMLTools::xmlNodeFindDescendant(hNode,"regionhierarchy->leafregions");
   if(!hNode) throw string("leafnodeinfo not found in xml");

   vector<int> *lbls=getUsedLabels();
   int n=lbls->size();
   int nParsedRegions=0;

   //bool fixoffset=false;
   //map<int,int> lblidx;

   xmlChar *prop=xmlGetProp(lNode,(const xmlChar*)"inbitmap");
   if(prop && IsAffirmativeSeg((char*)prop)){ // leaf nodes stored in bitmap


	for(int i=0;i<n;i++)
	  {
	    set<int> s;
	    s.insert(i);
	    ret.push_back(s);
	    if((*lbls)[i]<0||(*lbls)[i]>=n)
	      {
		//	if(alterbitmap) fixoffset=true;
		//else 
		throw string("Bitmap doesn't contain a"
			 " valid leaf region representation");
	      }
	    nParsedRegions++;
	  }
	
// 	if(fixoffset){

// 	  for(int i=0;i<n;i++)
// 	    lblidx[(*lbls)[i]]=i;
// 	  int x,y,s,w=getWidth(),h=getHeight();
// 	  for(x=0;x<w;x++) for(y=0;y<h;y++){
// 	    get_pixel_segment(x,y,s);
// 	    set_pixel_segment(x,y,lblidx[s]);
// 	  }
// 	}
	  
   
	

   }else{ // leaf regions in xml -> write them to bitmap

     // cout << "parsing leaf regions from xml" << endl;

      // ! no check for overlapping regions

     if(alterbitmap){
      int x,y,w=getWidth(),h=getHeight();
      for(x=0;x<w;x++) for(y=0;y<h;y++)
	set_pixel_segment(x,y,0);
     }

      for(xmlNodePtr rNode=XMLTools::xmlNodeFindDescendant(lNode,"region");
	  rNode; rNode=XMLTools::xmlNodeFindNextSibling(rNode,"region")){
	xmlChar *lblstr=xmlGetProp(rNode,(const xmlChar*)"label");
        int lbl;
	if(sscanf((char *)lblstr,"%d",&lbl) != 1 || lbl != nParsedRegions){
	  cerr << "lbl " << lbl<< " != " << nParsedRegions << endl;
	  throw string("Leaf regions parsed in wrong order from xml.");
	}
	xmlFree(lblstr);

	// cout << "parsing leaf region lbl " << lbl << endl;
	if(alterbitmap){
	  string rspec=XMLTools::xmlNodeGetContent(rNode);
	  PredefinedTypes::Region reg(rspec);
	
	  if(reg.id == "list") {
	    PredefinedTypes::CoordList cl(reg.value);

	    for(size_t i=0;i<cl.l.size();i++)
	      set_pixel_segment(cl.l[i],lbl);
	  } else if(reg.id =="rect"){
	    rectangularRegion r(reg.value);
	    coordList l=r.listCoordinates();
	    for(size_t i=0;i<l.size();i++)
	      set_pixel_segment(l[i],lbl);
	    
	  } else
	        throw string("Currently only list and rect regions supported");
	

	}

	set<int> s;
	s.insert(lbl);
	ret.push_back(s);
	nParsedRegions++;
      }
   }
    
 
      
   xmlFree(prop);

   // parse merges from xml

   xmlNodePtr mlNode=XMLTools::xmlNodeFindDescendant(hNode,"regionhierarchy->mergelist");
      
      if(!mlNode)
	throw string("<mergelist> not found");

      for(xmlNodePtr mNode=XMLTools::xmlNodeFindDescendant(mlNode,
							   "merge");
	  mNode;
	  mNode=XMLTools::xmlNodeFindNextSibling(mNode,"merge")){
	xmlChar *f1,*f2,*t,*flist;
	int from1,from2,to;

	t=xmlGetProp(mNode,(const xmlChar*)"to");

	//if(t)
	//  cout << "Got property string" << (char*)t<<endl; 

	if(!t || (sscanf((char*)t,"%d",&to)!= 1) || to != nParsedRegions)
	  {
	    if(t)
	    cout << "nParsedRegions=" << nParsedRegions 
		 << " ret.size="<<ret.size()
		 << " to="<<to<<endl;
	    else
	      _dump_xml();

	    throw string("Error in reading property ""to""");
	  }

	set<int> s;

	f1=xmlGetProp(mNode,(const xmlChar*)"from1");
	f2=xmlGetProp(mNode,(const xmlChar*)"from2");
	flist=xmlGetProp(mNode,(const xmlChar*)"fromlist");

	if(flist){

	  // the syntax with fromlist property takes priority

	  char fromliststr[200];
	  vector<int> fvec;

	  if(sscanf((char*)flist,"%s",fromliststr)!= 1)
	    throw string("Error in reading property fromlist");
	 
	  string2intvector(fromliststr,fvec);

	  // cout << "parsed " << fvec.size() << " from fromlist " << fromliststr << endl;

	  vector<int>::const_iterator it;
	  for(it=fvec.begin();it!=fvec.end();it++){
	    if(*it<0||*it>=nParsedRegions)
	      throw string("Fromlist entry not in valid range"); 
	    s.insert(ret[*it].begin(),ret[*it].end());
	  }
 
	}else{

	  // the syntax with from1 and from2 properties
	  if(!f1 || (sscanf((char*)f1,"%d",&from1)!= 1)) 
	    throw string("Error in reading property ""from1""");

	  if(from1<0||from1>=nParsedRegions)
	    throw string("From1 not in valid range"); 

	  if(!f2 || (sscanf((char*)f2,"%d",&from2)!= 1))
	    throw string("Error in reading property ""from2""");

	  if(from2<0||from2>=nParsedRegions)
	    throw string("From2 not in valid range"); 
	  
	  s.insert(ret[from1].begin(),ret[from1].end());
	  s.insert(ret[from2].begin(),ret[from2].end());
	}
	 
	xmlFree(t); xmlFree(f1); xmlFree(f2); xmlFree(flist);

	ret.push_back(s);
	nParsedRegions++;

      }
      delete lbls;
      return ret;
 }

  ///////////////////////////////////////////////////////////////////////////

  vector<pair<set<int>,string> > segmentfile::parseVirtualRegions(){

    vector<pair<set<int>,string> > ret;

    SegmentationResultList *rl=readLatestMethodFrameResultsFromXML(NULL,"","virtualsegment");

    //    cout << "Found "<<rl->size()<<" virtual segments" << endl;

    int nr=0;
    SegmentationResultList::const_iterator it;
    for(it=rl->begin();it !=rl->end();it++,nr++){
      pair<set<int>,string> tmpPair;
      tmpPair.first=parseVirtualSegmentLabel(it->name);
      tmpPair.second=it->value;
      ret.push_back(tmpPair);
    }

    delete rl;
    return ret;

  }

  ///////////////////////////////////////////////////////////////////////////

  string formVirtualSegmentLabel(const set<int>& s, char delimiter) {
    stringstream ss;
    bool first=true;

    for (set<int>::const_iterator it=s.begin(); it!=s.end(); it++) {
      if (first)
	first = false;
      else
	ss << delimiter;   
      ss << *it;
    }

    return ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  string formVirtualSegmentLabel(const vector<int> &s, char delimiter){
    set<int> si;
    si.insert(s.begin(),s.end());
    return formVirtualSegmentLabel(si,delimiter);
  }

 ///////////////////////////////////////////////////////////////////////////

  set<int> parseVirtualSegmentLabel(const string &s, char delimiter){
    
    set<int> ret;
    int i;
    char format[4]="%d,";
    format[3]=delimiter;

    string ss=s+delimiter;

    const char *ptr=ss.c_str();
    for(;*ptr;){
      if(sscanf(ptr,format,&i)!=1)
	throw string("Error in parseVirtualSegmentLabel");
      ret.insert(i);
      for(;*ptr&&*ptr!=delimiter;ptr++){}
      if(*ptr) ptr++;
    }

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void ReadLine(istream& in, string &line)
  {
    line="";
    
    char test;
    in.get(test);    
    while(in.good() && test != '\n'){
      line += test;
      in.get(test);    
    }
    
  }
  ///////////////////////////////////////////////////////////////////////////
  ostream &operator<<(ostream &out, const coordList &l)
  {
    vector<coord>::const_iterator i;
    for(i=l.begin(); i != l.end(); i++)
      out << *i << " ";
    if(!out.good()) throw string("operator << (ostream,coordList):"
				 " stream error.");
    
    return out;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  istream& operator >> (istream& in, coordList &l)
  {

    char test;
    coord co;
    
    l.clear();
    
    string line;
    
    ReadLine(in,line);
    
    istringstream ss(line.c_str());
    
    ss >> test;
    
    while( ss.good()){
      ss.putback(test);
      ss >> co;
      
      l.push_back(co);
      
      ss >> test;
    }
    
    return in;
  }



 ///////////////////////////////////////////////////////////////////////////
  
  xmlNodePtr PixelChain::AddToXML(xmlNodePtr parent) const
    {
      ostringstream str;
      str << *this;
      return xmlNewTextChild(parent,NULL,(xmlChar*)"pixelchain",(xmlChar*)str.str().c_str());
    }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool PixelChain::ParseXML(xmlNodePtr xmlnode)
    {
      if(XMLTools::strcmpXML(xmlnode->name,"pixelchain")) return false;
      istringstream str((char*)xmlNodeGetContent(xmlnode));
      str >> *this;
      
      return true;
    }
  
  ///////////////////////////////////////////////////////////////////////////
  ostream &operator<<(ostream &out, const PixelChain &c) {
    std::list<coord>::const_iterator i;
    for(i=c.l.begin(); i != c.l.end(); i++)
      out << *i << " ";
    if(!out.good()) throw string("operator << (ostream,PixelChain): stream error.");

    return out;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  
  istream& operator >> (istream& in, PixelChain &c){
    
    char test;
    coord co;
    
    c.l.clear();
    
    string line;
    
    ReadLine(in,line);
    
    istringstream ss(line.c_str());
    
    ss >> test;
    
    while( ss.good()){
      ss.putback(test);
      ss >> co;
      
      c.l.push_back(co);
      
      ss >> test;
    }
    
    return in;
  }
  ///////////////////////////////////////////////////////////////////////////

  string *SubSegment::OutputXML() const
    {
          xmlDocPtr doc=xmlNewDoc((xmlChar*)"1.0");
      xmlNodePtr node=xmlNewDocNode(doc,NULL,(xmlChar*)"subsegment",NULL);
      
      xmlDocSetRootElement(doc, node);
      
      AddToXML(node);
      
      xmlChar *buffer;
      int size;
      
      xmlDocDumpMemory(doc,&buffer,&size);  
      
      string *str=new string((char *)buffer);
      free(buffer);
      
      return str;
      
    }

  
  ///////////////////////////////////////////////////////////////////////////
  xmlNodePtr SubSegment::AddToXML(xmlNodePtr parent) const
    {
      int ctr=0;
      
      xmlNodePtr node=xmlNewTextChild(parent,NULL,(xmlChar*)"subsegment",NULL);
      char number[50];
      
      for(std::list<PixelChain>::const_iterator i=borders.begin();
	  i != borders.end(); i++){
	sprintf(number,"%d",ctr++);
	xmlNodePtr chain=(*i).AddToXML(node);
	if(chain)
	  xmlSetProp(chain,(xmlChar*)"index",(xmlChar*)number);
      }
      
      return node;
    }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool SubSegment::ParseXML(xmlNodePtr xmlnode)
    {
      if(XMLTools::strcmpXML(xmlnode->name,"subsegment")) return false;
      
      xmlNodePtr pixelchain;
      for(pixelchain=xmlnode->children;pixelchain;pixelchain=pixelchain->next){
	PixelChain ch;
	borders.push_back(ch);
	borders.back().ParseXML(pixelchain);
      }
      seed=borders.back().l.back();

      return true;
      
    }

  ///////////////////////////////////////////////////////////////////////////
  
  ostream &operator<<(ostream &out, const SubSegment &s) {
    int ctr=0;
    
    for(std::list<PixelChain>::const_iterator i=s.borders.begin();
	i != s.borders.end(); i++)
      out << "cc:chain[" << ctr++ << "]" << endl <<*i << endl
	  << "cc:chain_end" << endl;
    return out;
  }
  ///////////////////////////////////////////////////////////////////////////
  
  istream& operator >> (istream& in, SubSegment &s){
    string str;
    ReadLine(in,str);
    s.borders.clear();
  
    while( str.find("cc:chain[")==0){
      PixelChain ch;
      s.borders.push_back(ch);
      //    cout << "Starting to read border " << endl;
      in >> s.borders.back();
      s.seed=s.borders.back().l.back();
      //    cout << "Finished reading border " << endl;
      ReadLine(in,str); // read away cc:chain_end
      ReadLine(in,str);
    }
    
    return in;
  }
  
  
  ///////////////////////////////////////////////////////////////////////////
  xmlNodePtr Segment::AddToXML(xmlNodePtr parent) const
    {
      int ctr=0;
      
      char number[50];
      
      sprintf(number,"%d",label);
      xmlNodePtr node=xmlNewTextChild(parent,NULL,(xmlChar*)"segment",NULL);
      xmlSetProp(node,(xmlChar*)"label",(xmlChar*)number);      
      
      for(std::list<SubSegment>::const_iterator 
	    i=subsegments.begin();
	  i != subsegments.end(); i++){
	sprintf(number,"%d",ctr++);
	xmlNodePtr subsegment=(*i).AddToXML(node);
	if(subsegment)
	  xmlSetProp(subsegment,(xmlChar*)"index",(xmlChar*)number);
      }
      
      return node;
    }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool Segment::ParseXML(xmlNodePtr xmlnode)
    {
      if(XMLTools::strcmpXML(xmlnode->name,"segment")) return false;
      
      xmlChar *labelstr = xmlGetProp(xmlnode,(xmlChar*)"label");
      if(labelstr){
	int lbl;
	sscanf((char*)labelstr,"%d",&lbl);
	label=lbl;
	free(labelstr);
      }
      
      xmlNodePtr subsegment;
      for(subsegment=xmlnode->children;subsegment;subsegment=subsegment->next){
	SubSegment ss;
	subsegments.push_back(ss);
	//    cout << "Starting to read subsegment" << endl;
	subsegments.back().ParseXML(subsegment);
      }
      
      return true;
      
    }
  
  
  ///////////////////////////////////////////////////////////////////////////
  
  ostream &operator<<(ostream &out, const Segment &s) {  int ctr=0;
  out << "cc:segment[" << s.label << "]" << endl;
  for(
      std::list<SubSegment>::const_iterator 
	i=s.subsegments.begin();
      i != s.subsegments.end(); i++)
    out << "cc:subsegment[" << ctr++ << "]" << endl 
	<<*i << "cc:subsegment_end" << endl;
  out << "cc:segment_end" << endl;
  
  return out;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  istream& operator >> (istream& in,Segment &s){
    string str;
    ReadLine(in,str);
    s.subsegments.clear();
    
    while( str.find("cc:subsegment[")==0){
      SubSegment ss;
      s.subsegments.push_back(ss);
      //    cout << "Starting to read subsegment" << endl;
      in >> s.subsegments.back();
      //    cout << "Subsegment read";
      
      ReadLine(in,str);
    }
    
    return in;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  ostream &operator<<(ostream &out, const Chains &c) {
    std::list<Segment>::const_iterator it;
    for(it=c.segments.begin();it != c.segments.end();it++)
      out << *it;
    
    return out;
  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr SegmentationResult::toXML() const {
    xmlNodePtr xmlresult = xmlNewNode(NULL,(xmlChar*)"result");
    xmlNewProp(xmlresult,(xmlChar*)"name",(xmlChar*)name.c_str());

    xmlNewProp(xmlresult,(xmlChar*)"type",(xmlChar*)type.c_str());

    if(value.size() < 100)
      xmlNewProp(xmlresult,(xmlChar*)"value",(xmlChar*)value.c_str());
    else
      xmlNodeSetContent(xmlresult,(xmlChar*)value.c_str());

    char idstr[80];

    sprintf(idstr,"%d",resultid);
    xmlNewProp(xmlresult,(xmlChar*)"resultid",(xmlChar*)idstr);

    sprintf(idstr,"%d",methodid);
    xmlNewProp(xmlresult,(xmlChar*)"methodid",(xmlChar*)idstr);

    return xmlresult;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool SegmentationResult::parseXML(const xmlNodePtr resultnode) {
    xmlNodePtr node;
    string content;
    clear();

    bool valuefound = false;

    xmlChar *prop;

    prop=xmlGetProp(resultnode,(xmlChar*)"name");
    if(prop){
      name = (char*)prop;
      free(prop);
    }

    prop=xmlGetProp(resultnode,(xmlChar*)"type");
    if(prop){
      type = (char*)prop;
      free(prop);
    }

    prop=xmlGetProp(resultnode,(xmlChar*)"value");
    if(prop){
      value = (char*)prop;
      valuefound=true;
      free(prop);
    }

    prop=xmlGetProp(resultnode,(xmlChar*)"methodid");
    if(prop){
      int val;
      methodid=(sscanf((char*)prop,"%d",&val)==1)?val:-1;
      free(prop);
    }

    prop=xmlGetProp(resultnode,(xmlChar*)"resultid");
    if(prop){
      int val;
      resultid=(sscanf((char*)prop,"%d",&val)==1)?val:-1;
      free(prop);
    }

    if (type!="" && value!="")
      ExtractMemberValues();
      
    for (node=resultnode->children; node; node=node->next) {
      content = XMLTools::xmlNodeGetContent(node);
      
      if (!XMLTools::strcmpXML(node->name, "name"))
	name = content;
      
      else if (!XMLTools::strcmpXML(node->name,"type"))
	type = content;
	
      else if (!XMLTools::strcmpXML(node->name,"value")){
	value = content;
	valuefound=true;
      }
    }

    if(!valuefound){
      value = XMLTools::xmlNodeGetContent(resultnode);
      valuefound=!value.empty();
    }	

    return valuefound && !(name.empty()||type.empty());
  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool SegmentationResult::ExtractMemberValues() {
    if (type=="trangetext" || type=="trange") {
      string s = value;
      for (;;) {
	size_t p = s.find("//");
	if (p==string::npos)
	  break;
	s.replace(p, 2, " ");
      }

      stringstream ss(s);
      ss >> trange_start >> trange_end;
      getline(ss, trange_text);
      while (trange_text[0]==' ')
	trange_text.erase(0, 1);

      // cout << value << "|" << trange_text << endl;

      value += " **extracted**";
    }

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool SegmentationResult::StoreMemberValues() {
    if (type=="trangetext" || type=="trange") {
      stringstream ss;
      ss << trange_start << " " << trange_end << " " << trange_text;
      value = ss.str();
    }

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  ostream& operator<<(ostream &out, const SegmentationResult &r) {
    out << r.name << endl;
    out << r.type << endl;
    out << r.value << endl;
    out << r.methodid << endl;
    out << r.resultid << endl;
    
    return out;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  string SegmentationResult::Str() const {
    stringstream ss;
    ss << name << " " << type << " " << value << " "
       << methodid << " " << resultid;
    
    return ss.str();
  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr SegmentationAttribute::toXML() const {
    xmlNodePtr xmlattr = xmlNewNode(NULL,(xmlChar*)"attribute");
    xmlNewProp(xmlattr,(xmlChar*)"key",(xmlChar*)key.c_str());

    if(value.size() < 100)
      xmlNewProp(xmlattr,(xmlChar*)"value",(xmlChar*)value.c_str());
    else
      xmlNodeSetContent(xmlattr,(xmlChar*)value.c_str());

    return xmlattr;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool SegmentationAttribute::parseXML(const xmlNodePtr attrnode) {
    xmlNodePtr node;
    string content;

    bool valuefound=false;

    xmlChar *prop;

    prop=xmlGetProp(attrnode,(xmlChar*)"key");
    if(prop){
      key= (char*)prop;
      free(prop);
    }

    prop=xmlGetProp(attrnode,(xmlChar*)"value");
    if(prop){
      value = (char*)prop;
      valuefound=true;
      free(prop);
    }

    if(!valuefound)
      for (node=attrnode->children; !valuefound && node; node=node->next) {
	content = XMLTools::xmlNodeGetContent(node);
	if (!XMLTools::strcmpXML(node->name,"value")){
	  value = content;
	  valuefound=true;
	}
      }

    if(!valuefound){
      value = XMLTools::xmlNodeGetContent(attrnode);
      valuefound=!value.empty();
    }	

    return valuefound && !key.empty();
  }

  ///////////////////////////////////////////////////////////////////////////

  string *Chains::OutputXML() const {
    int ctr=0;
      
    xmlDocPtr doc=xmlNewDoc((xmlChar*)"1.0");
    xmlNodePtr node=xmlNewDocNode(doc,NULL,(xmlChar*)"chains",NULL);
      
    xmlDocSetRootElement(doc, node);
      
    char number[50];
    for(std::list<Segment>::const_iterator 
	  i=segments.begin();
	i != segments.end(); i++){
      sprintf(number,"%d",ctr++);
      xmlNodePtr segment=(*i).AddToXML(node);
      if(segment){
	xmlSetProp(segment,(xmlChar*)"index",(xmlChar*)number);      
      }
    }
      
    xmlChar *buffer;
    int size;
      
    xmlDocDumpMemory(doc,&buffer,&size);  
      
    string *str=new string((char *)buffer);
    free(buffer);
      
    return str;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool Chains::ParseXML(const string &str) {
    xmlDocPtr doc=xmlParseDoc((xmlChar*)str.c_str());
    xmlNodePtr xmlnode=xmlDocGetRootElement(doc);
      
    if(XMLTools::strcmpXML(xmlnode->name,"chains")) return false;
      
    xmlNodePtr segment;
    for(segment=xmlnode->children;segment;segment=segment->next){
      Segment s;
      segments.push_back(s);
      //    cout << "Starting to read subsegment" << endl;
      segments.back().ParseXML(segment);
    }
      
    return true;  
  }

  ///////////////////////////////////////////////////////////////////////////
  
  istream& operator >> (istream& in,Chains &c){
    string str;
    ReadLine(in,str);
    
    c.segments.clear();
    while( str.find("cc:segment[")==0){
      Segment seg;
      c.segments.push_back(seg);
      
      string labelstring=str.substr(str.find("[")+1);
      
      istringstream ss(labelstring.erase(labelstring.find("]",1)).c_str());
      ss >> c.segments.back().label;
      
      //    cout << "Starting to read segment label " 
      //	 << c.segments.back().label << endl;
      
      in >> c.segments.back();
      
      
      // if(Segmentation::Verbose()>1)
      //  cout << "Read segment label " << c.segments.back().label << endl;
      ReadLine(in,str);
    }
    
    return in;
  }

  
  ///////////////////////////////////////////////////////////////////////////

  const string& segmentfile::impl_version() {
    static string v =
      "$Id: segmentfile.C,v 1.181 2019/02/04 09:10:34 jormal Exp $";
    return v;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::init() {
    input_image_file=NULL;
    input_segment_file=NULL;
    _is_prepared=false;
    _originates_from_tiff=false;
    use_ondemand=true;
    current_frame=0;
    xmlIndentTreeOutput = true;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::openFiles(const string &in_image_name ,
			      const string &in_segment_name,
			      const bool read_in_image_in,
			      bool on_demand,bool cacheing) {
    bool read_in_image = read_in_image_in; // false;
   //  cout << "In segmentfile::openFiles("<<in_image_name<<","<<
//        in_segment_name<<","<<((read_in_image)?"true,":"false,")<<
//        ((on_demand)?"true)":"false)")<<endl;

    SequenceInfo *seqinfo=NULL;

    use_ondemand = on_demand;

    if (on_demand==false)
      throw string("ERROR: On demand set to false");

    if (in_image_name.empty() && in_segment_name.empty()){
      description.sequenceinfo.reset();
      initXMLSkeleton();
      //initMethodlist();
      return;
    }
    
    if (!in_image_name.empty()) {
      if (read_in_image)
	readImageFile(in_image_name.c_str(),on_demand,cacheing);
      else {
	// Not all files (for example MPG videos(this example is outdated)) 
	// are readable with imagefile()

	// Save "image" information
	input_image_file = new imagefile(); 
	input_image_file->filename(in_image_name);
	input_image_file->default_pixel(imagedata::pixeldata_float);
	input_image_file->frame_cacheing(cacheing);
	description.sequenceinfo.setFromImagefile(*input_image_file); 
	
	// Create one empty frame
	_frame_type tmp;
	tmp.file = NULL; // was input_image_file;, why?
	image_frames.frames.clear();
	tmp.framenr=0;
	tmp.createwidth=1;
	tmp.createheight=1;
	tmp.data=NULL;
	tmp.frameSrc=create;
	image_frames.frames.push_back(tmp);
      }

      seqinfo=new SequenceInfo;
      *seqinfo=description.sequenceinfo; 
      // copy this can be written over by readSegmentFile or initxmlskeleton

      if (in_segment_name.empty()) {
	// cout << "lastframe: " << description.sequenceinfo.lastframe
	//      << " =? " << seqinfo->lastframe << endl;

	allocateSegments();
	initXMLSkeleton();
	description.sequenceinfo=*seqinfo; 
      }
    }
    
    if (!in_segment_name.empty()) {
      readSegmentFile(in_segment_name.c_str(),on_demand);
      
      // if sequence info not specified by the segmentation file,
      // try to copy it from image sequence 
      if (seqinfo && description.sequenceinfo.lastframe<0 &&
	  !description.sequenceinfo.sizeinfo.size())
	description.sequenceinfo=*seqinfo; 

      if (seqinfo && !seqinfo->isCompatible(description.sequenceinfo))
	throw("image file and segmentation file have incompatibe "
	      "sequence characteristics.");

      if (in_image_name.empty() && !hasImages()){ 
	allocateImages();
      }
    }
    
 //    cout << "lastframe: " << description.sequenceinfo.lastframe << " =? ";
//     if(seqinfo)
//       cout << seqinfo->lastframe << endl;
//     else
//       cout << endl;

    delete seqinfo;
  }

  ///////////////////////////////////////////////////////////////////////////

  coordList 
  *segmentfile::convertToList(const segmentfile::Matrix &matrix,
			      const vector<int> *labels){
    coordList *ret=new coordList;
    set<int> *hash=NULL;
    
    if(labels){
      hash=new set<int>;
      for(size_t i=0;i<labels->size();i++)
	hash->insert((*labels)[i]);
    }
    
    int x,y;
    int w=getWidth(), h=getHeight();
    
    for(y=0;y<h;y++) for(x=0;x<w;x++){
      int s;
      if(matrix)
	s=matrix[x+y*w];
      else
	get_pixel_segment(x,y,s);
      
      if(hash){
	if(hash->count(s))
	  ret->push_back(coord(x,y));
      }
      else{
	if(s){
	  ret->push_back(coord(x,y));
	}
      }
    }
    
    if (hash) delete hash;
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////  

  void segmentfile::moveSegmentationMask(int from, int to, bool discard) {
    if (use_ondemand==false)
      throw string("segmentfile::moveSegmentationMask():use_ondemand==false.");
    
    if (segment_frames.frames[from].data==NULL)
      segmentFrame(from); // force the from frame to memory

    if (segment_frames.frames[to].data)
      delete segment_frames.frames[to].data;

    if (discard) {
      segment_frames.frames[to].data=segment_frames.frames[from].data;
      segment_frames.frames[to].owndata=segment_frames.frames[from].owndata;
      segment_frames.frames[from].data=NULL;
    } else {
      segment_frames.frames[to].data=new 
	imagedata(*segment_frames.frames[from].data);
      segment_frames.frames[to].owndata=true;
    }
  }

  ///////////////////////////////////////////////////////////////////////////  

  int segmentfile::getWidth() {
    return isFrameOk() ? imageFrame(getCurrentFrame())->width() : 0;
  }

  ///////////////////////////////////////////////////////////////////////////

  int segmentfile::getHeight()  {
    return isFrameOk() ? imageFrame(getCurrentFrame())->height() : 0;
  }

  ///////////////////////////////////////////////////////////////////////////

  const string &segmentfile::getImageFileName() const{
    static string empty;
    if (input_image_file)
      return input_image_file->filename();
    else return empty;
  }

  ///////////////////////////////////////////////////////////////////////////

  const string &segmentfile::getSegmentFileName() const{
    static string empty;
    if (input_segment_file)
      return input_segment_file->filename();
    else return empty;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::readImageFile(const char *n,bool on_demand,bool cacheing){
    char errhead[1000];
    sprintf(errhead, "segmentfile::readImageFile(%s) :", n);

    // cout << "in " << errhead << endl;

    if (on_demand==false)
      throw string("on_demand set to false in segmentfile::readImageFile()");

    image_frames.free();
    delete input_image_file;
    image_frames.frames.clear();
    
    input_image_file = new imagefile(n); 
    input_image_file->default_pixel(imagedata::pixeldata_float);
    input_image_file->frame_cacheing(cacheing);

    int tot_frames = input_image_file->nframes();
    
    // cout << "about to set up " << tot_frames << " frames" << endl;
    
    int f;
    _frame_type tmp;
    tmp.file = input_image_file;
    tmp.frameSrc = file;
    tmp.data=NULL;
    tmp.segment=false;

    for(f=0;f<tot_frames;f++){
      tmp.framenr=f;
      tmp.fileframe=f;
      image_frames.frames.push_back(tmp);
    }

    // cout << "frames allocated" << endl;  

    description.sequenceinfo.setFromImagefile(*input_image_file);

    // cout << "sequenceinfo set" << endl;  
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::readSegmentFile(const string& sfn, bool on_demand) {
    bool debug = false;

    char errhead[1000];
    sprintf(errhead, "segmentfile::readSegmentFile(%s) :", sfn.c_str());

    if (debug)
      cout << "now in readSegmentFile" << endl;

    if (!on_demand)
      throw string("ERROR: segmentfile::readSegmentFile(): on_demand==false");

    delete input_segment_file;
    input_segment_file=NULL;

    bool read_tiff=true;
    string fn = sfn;

    // testing if file "sfn" exists

    ifstream fin;
    fin.open(fn);
    if (!fin.is_open()) {
      // file does not exist, try gzipped
      fn += ".gz";
      fin.open(fn);
      if (!fin.is_open())
	fn = sfn; // restore filename
    }
    fin.close();

    //cout << "opening new file" << endl;
    string mtype = libmagic_mime_type(fn).first;

    if (mtype.substr(0,10) == "text/plain") {
      read_tiff = false;
    } else {
      try {
        input_segment_file = new imagefile(fn); 
      }
      catch (...) {
	if (debug)
	  cout << "Reading tiff file failed, treating "<<fn<<" as XML file."
	       << endl;
        read_tiff = false;
      }
    }
    if (read_tiff && input_segment_file->format().substr(0,4)=="text") {
      if (debug)
	cout << "Treating text file "<<fn<<" as an XML file." << endl;
      read_tiff = false;
    }

    // lots of copying around follows

    string *input_description;

    if (read_tiff)
      input_description = new string(input_segment_file->description());
    else {
      input_description = new string;
      ifstream f(sfn);
      while (f.good()) {
	char c; 
	f.get(c); 
	*input_description += c;
      }
    }

    if (debug)
      cout << "input description:  " << endl
	   << *input_description << endl;

    if (!description.readXML(input_description->c_str()))
      throw string(errhead) + " : couldn't parse XML description";

    delete input_description;

    if (debug) {
      cout << "xml read" << endl;
      _dump_xml();
    }

    int tot_frames = 0;

    if (read_tiff) {
      tot_frames = input_segment_file->nframes();

      if (debug)
	cout << "Opened file  w/ "<<tot_frames<<" frames."<<endl;
    }

    segment_frames.free();
    _free_stored_results();

    segment_frames.frames.clear();
    stored_images.clear();
    stored_segments.clear();

    // go through table of contents, free and allocate 
    // storage for sequences
      
    xmlNodePtr toc_node = description.tifftoc;
      
    _toc_type toc = _read_toc(toc_node);

    _toc_type::iterator tit;
    int maxframe = -1;
    
    for (tit=toc.begin(); tit!=toc.end(); tit++) {
      if (tit->tiff_framenr>maxframe)
	maxframe = tit->tiff_framenr;
      if (tit->tiff_msframenr>maxframe)
	maxframe = tit->tiff_msframenr;
    }

    if (debug)
      cout << "toc read " << endl;

    // this should include validity checking of toc

    if (read_tiff) {
      if (maxframe+1!=tot_frames) {
	cout << "maxframe= " << toc.size() 
	     << " tot_frames="<<tot_frames<<endl;

	_dump_xml();
	throw string(errhead) + "wrong frame numbers listed in <tifftoc>.";
      }

    } else
      tot_frames=maxframe+1;
    
    if (debug)
      cout << "about to collect sequences" << endl;

    bool imagesintoc = false;

    int toc_frames = toc.size();

    for (int f=0; f<toc_frames; f++)
      if (toc[f].sequence_name=="image")
	imagesintoc = true;

    allocateSegments();
      
    if (imagesintoc) { // since at leat one image was in toc, we assume that
      // all images come from the .seg file
      image_frames.free();
      image_frames.frames.clear();
      allocateImages();
    }

    // go through the parsed toc and read corresponding frames
    // rest of the frames are left in create-on-demand mode

    if (read_tiff) {
      _frame_type tmp;
      
      if (debug)
	cout << "about to read frames" << endl;

      for (int f=0; f<toc_frames; f++) {
	tmp.file = input_segment_file;
	tmp.framenr = toc[f].sequence_framenr;
	tmp.fileframe = toc[f].tiff_framenr;
	tmp.fileframe_ms = toc[f].tiff_msframenr;
	tmp.frameSrc = file;
	tmp.data = NULL; // was already
	string n = toc[f].sequence_name;

	if (!description.sequenceinfo.frameNrOk(tmp.framenr))
	  throw string("Frame nr out of range when reading tiff file");

	if (n=="segment") {
	  tmp.segment = true;
	  segment_frames.frames[tmp.framenr]=tmp;
	}
	if (n=="image") {
	  tmp.segment = false;
	  image_frames.frames[tmp.framenr]=tmp;
	}
	if (n=="stored_segment") {
	  tmp.segment = true;
	  stored_segments[tmp.framenr]=tmp;
	}
	if (n=="stored_image") {
	  tmp.segment = false;
	  stored_images[tmp.framenr]=tmp;
	}
      } // for f<toc_frames
    }

    _is_prepared = true;
    //      _dump_xml();
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::writeAllImageFrames(const string& n) {
    if (use_ondemand==false)
      throw string("use_ondemand==false in segmentfile::writeAllImageFrames");

    imagefile file(n, true, "image/tiff");

    int s = image_frames.frames.size();

    for (int f=0; f<s; f++) {
      bool discard = image_frames.frames[f].data==NULL;
      file.add_frame(*imageFrame(f));
      if (discard)
	discardImageFrame(f);
    }
    file.write();
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::writeImageFrame(const char *n,int f,const char *mimetype) {
    if (use_ondemand==false)
      throw string("use_ondemand==false in segmentfile::writeImageFrame");

    if(f==-1) f=getCurrentFrame();
    ensureFrameOk(f);
    bool discard = image_frames.frames[f].data==NULL;
    imagefile::write(*imageFrame(f),n,mimetype?mimetype:"image/tiff");
    if(discard) discardImageFrame(f);
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::writeAllSegmentFrames(const char *n, bool includeImages,
					  bool includeStoredResults,
					  segFileType forceFileType) {
    if (use_ondemand==false)
      throw string("use_ondemand==false in segmentfile::writeAllSegmentFrames");

    bool write_tiff = (forceFileType==tiff)
      || (forceFileType==empty &&
	  (_originates_from_tiff 
	   || !emptySegments() 
	   || (includeImages /* &&!emptyImages()*/) 
	   || (includeStoredResults &&
	       (stored_segments.size()>0 || stored_images.size()>0)
	       /*&& !emptyStoredResults()*/)));

    // cout << "write_tiff=" << write_tiff << endl;
   
    _toc_type toc;

    if (write_tiff)
      toc = prepareTOC(includeImages,includeStoredResults); 

    xmlNodePtr tocnode = (write_tiff) ? 
      xmlFromToc(toc) : xmlNewNode(NULL,(xmlChar*)"tifftoc");

    setXMLTOC(tocnode);

    if (write_tiff) {
      char *descr = description.outputXML(); 

//       cout << "description:" << endl << descr << endl;
//       cout << "about to open file..." << endl;

      imagefile file(n,true,"image/tiff");
//       cout << "...opened" << endl;

      int frames_written=0;

      for(_toc_type::const_iterator toc_it=toc.begin(); toc_it != toc.end();
	  toc_it++){

	  _frame_type *srcframe;
	  int framenr=toc_it->sequence_framenr;

	  if(toc_it->sequence_name=="segment")
	    srcframe=&(segment_frames.frames[framenr]);
	  else if(toc_it->sequence_name=="image")
	    srcframe=&(image_frames.frames[framenr]);
	  else if(toc_it->sequence_name=="stored_segment")
	    srcframe=&(stored_segments[framenr]);
	  else if(toc_it->sequence_name=="stored_image")
	    srcframe=&(stored_images[framenr]);
	  else
	    throw string("invalid toc");

	  bool discard = srcframe->data==NULL;

	  if(discard)
	    _fetch_frame(*srcframe);

	 //  cout << "adding frame of size " << frame.data->width()
// 	       << "x" << frame.data->height() << endl;
// 	  cout << "datatype = " << frame.data->datatypename() << endl;

	  if(toc_it->tiff_msframenr<0){
	    if(frames_written++!=toc_it->tiff_framenr)
	      throw string("Mismatch between toc and tiff");
	    file.add_frame(*srcframe->data);
	  }
	  else{ // insert two consecutive frames
	    if(frames_written!=toc_it->tiff_framenr 
	       ||frames_written+1!=toc_it->tiff_msframenr)
	      throw string("Mismatch between toc and tiff");
	    vector<imagedata> v=srcframe->data->split_to_2();
	    file.add_frame(v[0]);
	    file.add_frame(v[1]); 
	    frames_written += 2;
	  }

	  if(discard) _discard_frame(*srcframe);

      }

      file.description(descr);
      file.write();
      // cout << "wrote file w/ " << file.nframes() << "frames" << endl;
      free(descr);

    } else { // if(write_tiff)
      FILE *f = fopen(n, "w");
      if (f) {
	description.outputXML(f);
	fclose(f);
      }
    }

    if (!includeStoredResults) { // include them in XML toc in memory anyway
      _toc_type toc=prepareTOC(includeImages,true);
      xmlNodePtr tocnode=xmlFromToc(toc);
      setXMLTOC(tocnode);
      xmlFreeNode(tocnode);
    }
  }
 
  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::writeSegmentFrame(const char* /*sfn*/, int /*fra*/,
				      bool /*includeImages*/,
				      bool /*includeStoredResults*/,
				      segFileType /*forceFileType*/) {


    throw string("Writing single frame currently not implemented");

  }

  ///////////////////////////////////////////////////////////////////////////

//   void segmentfile::appendImage(const imagedata &img)
//   {

//     throw string("segmentfile::appendImage() discontinued.");

//     string errhead = name_head()+"appendImage() : ";

//   }

//   ///////////////////////////////////////////////////////////////////////////

//   void segmentfile::appendSegment(const imagedata &seg)
//   {
//     string errhead = name_head()+"appendSegment() : ";

//     throw string("segmentfile::appendSegment() discontinued.");

//   }

//   ///////////////////////////////////////////////////////////////////////////



    void segmentfile::allocateImages(){

    // string errhead = name_head()+"allocateImages() : ";

    int f;

    image_frames.free();
    image_frames.frames.clear();

    for(f=0;f<=description.sequenceinfo.lastframe;f++){
      _frame_type tmp;
      tmp.framenr=f;
      coord s=description.sequenceinfo.frameSize(f);
      tmp.createwidth=s.x;
      tmp.createheight=s.y;
      tmp.segment=false;
      tmp.frameSrc=create;
      image_frames.frames.push_back(tmp);

    }
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::allocateSegments() {
    // string errhead = name_head()+"allocateSegments() : ";
    segment_frames.free();
    segment_frames.frames.clear();

    for (int f=0; f<=description.sequenceinfo.lastframe; f++) {
      _frame_type tmp;
      tmp.framenr=f;

      coord s=description.sequenceinfo.frameSize(f);

      tmp.createwidth=s.x;
      tmp.createheight=s.y;
      tmp.segment=true;
      tmp.frameSrc=create;
      segment_frames.frames.push_back(tmp);
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::forceImages(const vector<imagedata> &i) {
    if((int)i.size() != description.sequenceinfo.nframes())
      throw("Incompatible number of images");

    image_frames.free();
    image_frames.frames.clear(); // might leak memory 

    _frame_type tmp;
    for (size_t f=0; f<i.size(); f++) {
      tmp.framenr=f;
      tmp.data=new imagedata(i[f]);
      tmp.owndata=true;
      if (tmp.data->count() != 3)
	throw string("segmentfile::forceImages : images must have three channels.");
      tmp.data->convert(imagedata::pixeldata_float);
      tmp.segment=false;
      tmp.frameSrc=no_source;

      coord s=description.sequenceinfo.frameSize(f);
      if(s.x != (int)i[f].width() || s.y != (int)i[f].height())
	throw string("segmentfile::forceImages : incompatible image size");

      image_frames.frames.push_back(tmp);
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::forceSegments(const vector<imagedata> &s) {
    if ((int)s.size() != description.sequenceinfo.nframes())
      throw("Incompatible number of images");

    segment_frames.free();
    segment_frames.frames.clear(); // might leak memory 

    _frame_type tmp;
    for (size_t f=0; f<s.size(); f++) {
      tmp.framenr=f;
      tmp.data=new imagedata(s[f]);
      tmp.owndata=true;
      if (tmp.data->count() != 1)
	throw string("segmentfile::forceSegments : images must have a single channel.");
      tmp.data->convert(_getSegmentframeDatatype());

      coord sz=description.sequenceinfo.frameSize(f);
      if(sz.x != (int)s[f].width() || sz.y != (int)s[f].height())
	throw string("segmentfile::forceSegments : incompatible image size");

      tmp.segment=true;
      tmp.frameSrc=no_source;

      segment_frames.frames.push_back(tmp);
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::forceImage(const imagedata &i, int f) {
    if (!description.sequenceinfo.frameNrOk(f))
      throw name_head() + "forceImage(): bad frame number";  

    coord sz=description.sequenceinfo.frameSize(f);
    if (sz.x != (int)i.width() || sz.y != (int)i.height())
	throw string("segmentfile::forceImage : incompatible image size");

    if (i.count() != 3)
       throw name_head() + "forceImage(): count != 3";  

    _discard_frame(image_frames.frames[f]); // ensure that memory is not allocated

    _frame_type tmp;
    tmp.framenr=f;
    tmp.data=new imagedata(i);
    tmp.owndata=true;
    tmp.data->convert(imagedata::pixeldata_float);
    tmp.segment=false;
    tmp.frameSrc=no_source;

    image_frames.frames[f]=tmp;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::forceSegment(const imagedata& s, int f) {
    if (!description.sequenceinfo.frameNrOk(f))
      throw name_head() + "forceSegment(): bad frame number";  

    coord sz=description.sequenceinfo.frameSize(f);
    if (sz.x != (int)s.width() || sz.y != (int)s.height())
	throw string("segmentfile::forceSegment : incompatible image size");

    if (s.count() != 1)
       throw name_head() + "forceSegment(): count != 1";  
    
    _discard_frame(segment_frames.frames[f]); // ensure that memory is not allocated

    _frame_type tmp;
    tmp.framenr=f;
    tmp.data=new imagedata(s);
    tmp.owndata=true;
    tmp.data->convert(_getSegmentframeDatatype());
    tmp.segment=true;
    tmp.frameSrc=no_source;

    segment_frames.frames[f] = tmp;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::rotateFrame(int f, const scalinginfo &si) {
    if (!isFrameOk(f))
      throw name_head() + "rotateFrame(): bad frame number";  

    imageFrame(f)->rotate(si);
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::rescaleFrame(int f, const scalinginfo &si,
				 const imagedata *img, const imagedata *seg,
				 bool emptyImg, bool emptySeg) {
    if (!isFrameOk(f))
      throw name_head() + "rescaleFrame(): bad frame number";  

    if (img) { // force image
      if ((int)img->width() != si.dst_width() ||
	  (int)img->height() != si.dst_height())
	throw string
	  ("rescaleFrame : given image frame not of appropriate size.");
      forceImage(*img,f);
    } else { 
      if (image_frames.frames[f].frameSrc!=create || 
	  image_frames.frames[f].data!=NULL){

	// if the frame creation is not pending 
	// due to the on-demand mechanism
	if (!emptyImg)
	  imageFrame(f)->rescale(si);
	else
	  imageFrame(f)->resize(si.dst_width(),si.dst_height());
      }
      // use the new size from now on if the frame is generated on the fly
      image_frames.frames[f].createwidth=si.dst_width();
      image_frames.frames[f].createheight=si.dst_height();

    }

    if (seg) {
      if ((int)seg->width() != si.dst_width() ||
	  (int)seg->height() != si.dst_height())
	throw string
	  ("rescaleFrame : given segment frame not of appropriate size.");
      forceSegment(*seg,f);
    } else {
      if (segment_frames.frames[f].frameSrc!=create || 
	  segment_frames.frames[f].data!=NULL){

	// if the creation of the frame is not delayed by the 
	// on-demand machanism
	if (!emptySeg)
	  segmentFrame(f)->rescale(si);
	else
	  segmentFrame(f)->resize(si.dst_width(),si.dst_height());
      }
      // use the new size from now on if the frame is generated on the fly
      segment_frames.frames[f].createwidth=si.dst_width();
      segment_frames.frames[f].createheight=si.dst_height();
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  imagedata *segmentfile::copyFrame(int f) {
    ensureFrameOk(f);
    return new imagedata(*imageFrame(f));
  }
  
  ///////////////////////////////////////////////////////////////////////////

  vector<imagedata *> segmentfile::copyFrames() {
    vector<imagedata *> ret;
    for (int f=0; f<getNumFrames(); f++)
      ret.push_back(new imagedata(*imageFrame(f)));
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  imagedata *segmentfile::getEmptyFrame() {
    coord sz=description.sequenceinfo.frameSize(getCurrentFrame());
    return new imagedata(sz.x,sz.y);
  }

  ///////////////////////////////////////////////////////////////////////////

  vector<const imagedata *>segmentfile::accessFrames() {
    vector<const imagedata *> ret;
    for(int f=0; f<getNumFrames(); f++)
      ret.push_back(imageFrame(f));

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  imagedata *segmentfile::copySegmentation(int f) {
    ensureFrameOk(f);
    return new imagedata(*segmentFrame(f));
  }

  ///////////////////////////////////////////////////////////////////////////

  vector<imagedata *> segmentfile::copySegmentations() {
    vector<imagedata *> ret;
    for(int f=0; f<getNumFrames(); f++)
      ret.push_back(new imagedata(*segmentFrame(f)));
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  imagedata *segmentfile::getEmptySegmentation() {
    coord sz=description.sequenceinfo.frameSize(getCurrentFrame());
    return new imagedata(sz.x,sz.y,1,_getSegmentframeDatatype());
  }

  ///////////////////////////////////////////////////////////////////////////

  vector<const imagedata *> segmentfile::accessSegmentations() {
    vector<const imagedata *> ret;
    for(int f=0;f<getNumFrames();f++)
      ret.push_back(segmentFrame(f));
    
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool segmentfile::CopyPixelsWithMask (double src_start_x, 
					double src_start_y, 
					double src_scaling, 
					bool mask_src,
					vector <unsigned char> &target, 
					int tgt_w,  
					int tgt_start_x, 
					int tgt_start_y,
					int tgt_stop_x,
					int tgt_stop_y) {
    float src_x,src_y,src_step = 1/src_scaling;
    float r,g,b;
    
    int x,y;
    
  //   cout << "Now in CopyPixelsWithMask(" << endl
//        //       << source.Description() << "," << endl
//             << "src_start_x=" << src_start_x << "," << endl
//             << "src_start_y=" << src_start_y << "," << endl
//             << "src scaling=" << src_scaling << "," << endl
//             << "tgt_w=" << tgt_w << "," << endl
//        //       << "tgt_h=" << tgt_h << "," << endl
//             << "tgt_start_x=" << tgt_start_x << "," << endl
//             << "tgt_start_y=" << tgt_start_y << "," << endl
//             << "tgt_stop_x=" << tgt_stop_x << "," << endl
//             << "tgt_stop_y=" << tgt_stop_y << ")" << endl;
      
    for(y=tgt_start_y,src_y=src_start_y;y<=tgt_stop_y;y++){ 
      int ind=3*(y*tgt_w+tgt_start_x);
      for(x=tgt_start_x,src_x=src_start_x;x<=tgt_stop_x;x++){
	
	int sx1=(int)src_x,sy1=(int)src_y;
	float deltax=src_x-sx1;
	float deltay=src_y-sy1;
	int sx2 = sx1 +((deltax)?1:0);
	int sy2 = sy1 +((deltay)?1:0);
	
	int s1 = -1, s2 = -1, s3 = -1, s4 = -1;
	
	if(mask_src){
	  get_pixel_segment_extended(sx1,sy1,s1,0);
	  get_pixel_segment_extended(sx2,sy1,s2,0);
	  get_pixel_segment_extended(sx1,sy2,s3,0);
	  get_pixel_segment_extended(sx2,sy2,s4,0);
	}
	
	if(!mask_src||s1||s2||s3||s4){
	  // mask allows this pixel to be copied

	  // cout << "mask allowed copying" << endl;

	  float r1,g1,b1,r2,g2,b2,r3,g3,b3,r4,g4,b4;
	  
	  get_pixel_rgb(sx1,sy1,r1,g1,b1);
	  get_pixel_rgb(sx2,sy1,r2,g2,b2);
	  get_pixel_rgb(sx1,sy2,r3,g3,b3);
	  get_pixel_rgb(sx2,sy2,r4,g4,b4);
	  
	  if(!s1)
	    r1=g1=b1=255;
	  if(!s2)
	    r2=g2=b2=255;
	  if(!s3)
	    r3=g3=b3=255;
	  if(!s4)
	    r4=g4=b4=255;
	  
	  // perform bilinear interpolation
	  
	  r = (r2-r1)*deltax + (r1-r2-r3+r4)*deltax*deltay + (r3-r1)*deltay+r1;
	  g = (g2-g1)*deltax + (g1-g2-g3+g4)*deltax*deltay + (g3-g1)*deltay+g1;
	  b = (b2-b1)*deltax + (b1-b2-b3+b4)*deltax*deltay + (b3-b1)*deltay+b1;
	  

	  target[ind++]=(unsigned char)(255*r);
	  target[ind++]=(unsigned char)(255*g);
	  target[ind++]=(unsigned char)(255*b);

// 	  cout << "wrote " << (int) target[ind-2] << " "
// 	       << (int) target[ind-1] << " "
// 	       << (int) target[ind] << " to indexes "
// 	       << ind-2 << "-" << ind << endl;
	    
	}
	else{
	  ind += 3;
	  // cout << "mask prevented copying s{1,2,3,4}=" << s1 <<" "<<
	  //  s2 << " " << s3 << " " << s4 << endl;
	}
	
	src_x+= src_step;
      }
      src_y += src_step;   
    }
    
    //  cout << "maxind = " << maxind << endl;
    return true;
  }

 ///////////////////////////////////////////////////////////////////////////
  bool segmentfile::HasNeighbourSeg(const coord &c,int dir){
    
    int delta_x[8]={1,1,0,-1,-1,-1,0,1};
    int delta_y[8]={0,-1,-1,-1,0,1,1,1};
    int l,s;
  
    get_pixel_segment_extended(c.x+delta_x[dir&7],c.y+delta_y[dir&7],s,-1);
    get_pixel_segment_extended(c.x,c.y,l,-1);
  
    // cout << c.x << "," << c.y <<":" << l << "==" << s << " ?" << endl;

    return (l==s);
  }

  ///////////////////////////////////////////////////////////////////////////

  int segmentfile::NeighbourCountSeg(const coord &c, bool use_diagonals) {
    int ctr = 0, incr = use_diagonals ? 1 : 2;
    
    for (int dir=0; dir<8; dir+=incr)
      if (HasNeighbourSeg(c, dir))
	ctr++;
      
    return ctr;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::isSegmentBoundary(const coord& c) {
    for (int dir=0; dir<8; dir++)
      if (!HasNeighbourSeg(c, dir))
	return true;
    return false;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::isSegmentBoundary(int x, int y) {
    coord c(x,y);

    for (int dir=0; dir<8; dir++)
      if (!HasNeighbourSeg(c,dir))
	return true;
    return false;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int segmentfile::GetLabelCount() {
    vector<int> *l = getUsedLabels();
    int ret = l->size();
    delete l;
    
    return ret;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  vector<int> *segmentfile::getUsedLabels(const coord &ul, const coord &br) {
    vector<int> *l = new vector<int>;
    set<int> accu;
    
    for (int y=ul.y; y<=br.y; y++)
      for(int x=ul.x; x<=br.x; x++) {
	int v;
	get_pixel_segment(x, y, v);
	accu.insert(v);
      }

    set<int>::const_iterator it;
    for (it=accu.begin(); it!=accu.end(); it++)
      l->push_back(*it);
    //    cout<<"pushing back "<< i << endl;
    
    return l;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  vector<pair<int,string> > segmentfile::getUsedLabelsWithText(/*int w, int h*/) {
    set<int> accu;
    
    if (zoningText=="centerdiag")
      for (int i=0; i<5; i++)
	accu.insert(i);

    const int w = getWidth(), h = getHeight();
    // if (w==0 || h==0) {
    //   w = getWidth();
    //   h = getHeight();
    // }
    
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++) {
	int v;
	get_pixel_segment(x, y, v);
	accu.insert(v);
      }

    vector<pair<int,string> > l;
    set<int> bs = getBackgroundLabels();
    
    set<int>::const_iterator it;
    for (it=accu.begin(); it!=accu.end(); it++) {
      bool is_background = bs.count(*it);
      const string txt = is_background ? "background" : "";
      l.push_back(make_pair(*it, txt));
    }

    return l;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  set<int> segmentfile::getBackgroundLabels(int frame) const {
    set<int> bs;
    
    string blabels = getLastSegmentFrameAttribute(frame, "backgroundLabels");
    vector<int> v = _string_to_intvector(blabels);

    for (auto it=v.begin(); it!=v.end(); it++)
      bs.insert(*it);

    return bs;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::setBackgroundLabels(int frame, const set<int>& bs) {
    vector<int> v;

    for (auto it=bs.begin(); it!=bs.end(); it++)
      v.push_back(*it);

    replaceSegmentFrameAttribute(frame, "backgroundLabels",
				 _intvector_to_string(v));
  }

  ///////////////////////////////////////////////////////////////////////////
  
  int segmentfile::getFirstFreeLabel() {
    vector<int> *l = getUsedLabels();
    
    for (size_t i=0; i<l->size(); i++) 
      if ((*l)[i]!=(int)i)
	return i;
    
    return l->size();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int segmentfile::GetSeparateCount() {
    int *u = GetSeparateLabelingInt();
    set<int> accu;
    
    for (int y=0; y<getHeight(); y++)
      for(int x=0; x<getWidth(); x++)
	accu.insert(u[x+y*getWidth()]);
    
    delete [] u;
    
    return accu.size();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int *segmentfile::GetSeparateLabelingInt(CorrespArray *c, 
					   bool fourConnectivity,
					   bool bordersConnectLabelZero,
					   const PredefinedTypes::Box
					   *boundingBox){
      
    PredefinedTypes::Box bb;
    if(boundingBox)
      bb=*boundingBox;
    else{
      bb.ulx=bb.uly=0;
      bb.brx=getWidth()-1;
      bb.bry=getHeight()-1;
    }

    int w=bb.width();
    int h=bb.height();

    string str; bb.write(str);

    //cout << "getSeparateLabelingInt(): bb="<<str<<" w="<<w<<" h="<<h<<endl; 

    int x,y,i;
    int u,u2,u3;
    int lastlabel=0;
    int nbr[4],s;
    bool corresp_allocated=(c==NULL);
    int bklabel =(bordersConnectLabelZero)?0:-1;
      
    // 0 - NW
    // 1 - N
    // 2 - NE
    // 3 - W
      
    int *unique=new int[w*h];
    CorrespArray *corresp;
      
    if(c){ corresp=c;}
    else{ corresp=new CorrespArray;}
    
    corresp->v.clear();
    
    corresp->v.push_back(0);
    for(y=bb.uly;y<=bb.bry;y++){
      int yidx=y-bb.uly;
      get_pixel_segment_extended(bb.ulx-1,y-1,nbr[1],bklabel,&bb);
      get_pixel_segment_extended(bb.ulx,y-1,nbr[2],bklabel,&bb);
      get_pixel_segment_extended(bb.ulx-1,y,s,bklabel,&bb);
      
      for(x=bb.ulx;x<=bb.brx;x++){
	int xidx=x-bb.ulx;

	int uidx=xidx+yidx*w;

	nbr[0]=nbr[1];
	nbr[1]=nbr[2];
	nbr[3]=s;
	
	get_pixel_segment_extended(x+1,y-1,nbr[2],bklabel,&bb);
	get_pixel_segment_extended(x,y,s,bklabel,&bb);
	  
	//if(Verbose()>1){
	//	cout << "(x,y)=(" <<x << "," << y << ")" << endl; 
	//	cout << "s="<< s << " Neighborhood: " << nbr[0] << " " 
	//	     << nbr[1] << " " << nbr[2] << " " << nbr[3] << endl;
	// }
	
	  
	for(i=0;i<4;i++){
	  if(fourConnectivity && !(i&1)){
	    //cout << "skipping dir "<<i<<endl;
	    continue;
	  }
	  if(s==nbr[i]){
	    MarkSame(unique,xidx,yidx,bb,i,bklabel);
	    break;
	  }
	}
	if(i==4){ // start new segment
	  unique[uidx]=++lastlabel;
	  corresp->v.push_back(lastlabel);
	  
	  //	    if(Verbose()>1){
	  //cout << "New segment found in ("
	    //     <<x<<","<<y<<")"<< " label = "
	    //     << lastlabel << endl;
	    
	    //	  cout << "s="<< s << "Neighborhood: " << nbr[0] << " " 
	    //       << nbr[1] << " " << nbr[2] << " " << nbr[3] << endl;
	    //}
	    
	  }
	  else{
	    u=unique[uidx];
	    //u1=(y>0)? unique[x+(y-1)*getWidth()]:bklabel;
	    u2=(yidx>0 && xidx<w-1 )? unique[uidx+1-w]:bklabel; // u(x+1,y-1)
	    u3=(xidx>0)? unique[uidx-1]:bklabel; // u(x-1,y)
	    
	    if(fourConnectivity==false){
	      if(u>=0 && u2 >= 0 && nbr[0]==nbr[2] && nbr[0]==s && corresp->v[u]!= corresp->v[u2]){
		//ut << "Bridging " << u << " and "<<u2<<" at ("
		//       <<x<<","<<y<<")"<<endl; 
		if(corresp->SmallestCorresponding(u)>
		   corresp->SmallestCorresponding(u2)){
		  corresp->v[corresp->SmallestCorresponding(u)]=
		    corresp->SmallestCorresponding(u2);
		  corresp->v[u]=corresp->SmallestCorresponding(u2);
		  }
		  else{
		    corresp->v[corresp->SmallestCorresponding(u2)]=
		      corresp->SmallestCorresponding(u);
		    corresp->v[u2]=corresp->SmallestCorresponding(u);
		  }
		  //cout <<"corresp->v[" << u <<"] now " << corresp->v[u] 
	       	  //   <<" and" <<"corresp->v[" << u2 <<"] " << corresp->v[u2]<< endl; 
	    }
	    else if(u>=0 && u3>=0&&nbr[2]==nbr[3] && nbr[2]==s && corresp->v[u]!= corresp->v[u3]){
	      //	       	  cout << "Bridging " << u << " and "<<u3<<" at ("
	      //     <<x<<","<<y<<")"<<endl; 
	      if(corresp->SmallestCorresponding(u)>
		     corresp->SmallestCorresponding(u3)){
		    corresp->v[corresp->SmallestCorresponding(u)]=
		      corresp->SmallestCorresponding(u3);
		    corresp->v[u]=corresp->SmallestCorresponding(u3);
		  }
		  else{
		    corresp->v[corresp->SmallestCorresponding(u3)]=
		      corresp->SmallestCorresponding(u);
		    corresp->v[u3]=corresp->SmallestCorresponding(u);
		  }
	      
	      //	        cout <<"corresp->v[" << u <<"] now " << corresp->v[u] 
	      //		     <<" and" <<"corresp->v[" << u3 <<"] " << corresp->v[u3] << endl;//
	      
	    }
	    }
	    else{ // fourconnectivity
	      if(u>=0 && u3>=0&&nbr[1]==nbr[3] && nbr[1]==s && corresp->v[u]!= corresp->v[u3]){
		if(corresp->SmallestCorresponding(u)>
		   corresp->SmallestCorresponding(u3)){
		  corresp->v[corresp->SmallestCorresponding(u)]=
		    corresp->SmallestCorresponding(u3);
		  corresp->v[u]=corresp->SmallestCorresponding(u3);
		}
		else{
		  corresp->v[corresp->SmallestCorresponding(u3)]=
		    corresp->SmallestCorresponding(u);
		  corresp->v[u3]=corresp->SmallestCorresponding(u);
		}
	      }
	    }
	  }
	}
      }

      if(bordersConnectLabelZero){
      // go through the last line in the pic and mark the labels 
      // corresponding to background to correspond to zero
      
	y=bb.bry;
	i=(h-1)*w;
      
	for(x=bb.ulx;x<=bb.brx;x++) {
	  get_pixel_segment(x,y,s);
	  if(s==0){
	    //cout <<"i="<<i<<" w*h="<<w*h;
	    int ind=unique[i];
	    while(corresp->v[ind]!=ind)
	      {
	      // 	cout << "changing ind " <<ind<< "leading to "<<corresp->v[ind]<<endl;
		int oldind=ind;
		ind=corresp->v[ind];
		corresp->v[oldind]=0;
	      }
	  }
	  i++;
	} 
      }
//        if(Verbose()>1)
// 	{
//  	  cout << "Unique labeling made " << endl;
//  	  cout << "Lastlabel is " << lastlabel << ", correspondence list has "
//  	       << corresp->size() << " entries. " << endl;
	  
//  	  //      for(i=0;i<corresp->size();i++)
//  	  //	cout << i <<" : " << corresp->v[i] << endl;
	  
//  	}
      
      if(corresp_allocated){
	// correspondence information will be destroyed, 
	// so we map it to the labeling
	map<int,int> condensed;
	int newlabel=0;

	for(i=0;i<w*h;i++)
	  if(!condensed.count(corresp->SmallestCorresponding(unique[i])))
	    condensed[corresp->SmallestCorresponding(unique[i])]=newlabel++;

	for(i=0;i<w*h;i++)
	  unique[i]=condensed[corresp->SmallestCorresponding(unique[i])];
	
	delete corresp;
      }
      
      return unique;
      
    }
  
  ///////////////////////////////////////////////////////////////////////////
  void segmentfile::MarkSame(int *u,int x, int y,
			     const PredefinedTypes::Box &bb,
			     int nbr,int bklabel){
    int val=0;
    switch(nbr){
    case 0:
      val=(x>0&&y>0)? u[x-1+(y-1)*bb.width()]:bklabel;
	break;
    case 1: 
      val=(y>0)? u[x+(y-1)*bb.width()]:bklabel;
	break;
    case 2: 
      val=(y>0&&(x<bb.width()))? u[x+1+(y-1)*bb.width()]:bklabel;
	break;
    case 3: 
      val=(x>0)? u[x-1+y*bb.width()]:bklabel;
	break;
	
    }
    u[x+y*bb.width()]=val;
    
  }
  ///////////////////////////////////////////////////////////////////////////
  
  int segmentfile::getSegmentArea(int label){
    int ret=0;
    
    int x,y,s;
    for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
      get_pixel_segment(x,y,s);
      if(s==label) ret++;
    }
    
    return ret;
  }
  ///////////////////////////////////////////////////////////////////////////

  map<int,set<int> > segmentfile::findNeighbours(bool fourNeighbours){
    map<int,set<int> > ret;

    vector<int> *lbls=getUsedLabels();

    if(lbls->size()==1)
      ret[(*lbls)[0]]=set<int>();
    else{

      // coding of dirs
    
      // 0 - E
      // 1 - SW
      // 2 - S
      // 3 - SE
    
      int l=getWidth()*getHeight(),i;
      unsigned char *isNeighbour= new unsigned char[l];
    
      const int xincr[]={1,-1,0,1};
      const int yincr[]={0,1,1,1};

      for(i=0;i<l;i++){
	isNeighbour[i]=15;
      }
      for(i=0;i<getWidth();i++){
	isNeighbour[l-i-1] = 1; // lower neighbours away
      }
    
      for(i=0;i<l;i+=getWidth()){
	isNeighbour[i] &= 13; // SW neighbours away 	
	isNeighbour[i+getWidth()-1] &= 6; // right neighbours away
      }
    
      int x,y,dir;

      for(i=0,x=0,y=0;i<l;i++,x++){
	if(x>=getWidth()){ x=0;y++;}
	int s;
	get_pixel_segment(x,y,s);

	for(dir=0;dir<4;dir+=fourNeighbours ? 2 : 1){
	  if(!(isNeighbour[i]&(1<<dir))) continue;
	  int sc;
	  get_pixel_segment(x+xincr[dir],y+yincr[dir],sc);
	  if(s!=sc){
	    ret[s].insert(sc);
	    ret[sc].insert(s);
	  }
	}
      }
      delete[] isNeighbour;    
    }

    delete lbls;
    
    return ret;
    
  }
  ///////////////////////////////////////////////////////////////////////////

  vector<set<int> > segmentfile::getAdjacentCombinations(){

    set<set<int> > pending;// these structures hold the same elements
    vector<set<int> > ret; 

    vector<int> *lbls=getUsedLabels();
    map<int,set<int> > nbr_primi=findNeighbours();

    for(map<int,set<int> >::iterator mit=nbr_primi.begin();
	mit != nbr_primi.end(); mit++){
      cout << "segment " << mit->first <<" has neighbours"
	   << formVirtualSegmentLabel(mit->second) << endl;
    }

    //cout << "neighbours found" << endl;

    for(vector<int>::iterator lit=lbls->begin();
	lit != lbls->end(); lit++){
      set<int> tmpset; tmpset.insert(*lit);
      pending.insert(tmpset);
      ret.push_back(tmpset);
    }
      
    size_t processed=0;

    while(pending.size()>processed){
      set<int> s=ret[processed];
      set<int> nbr;

      //      cout<<"starting loop with set " << formVirtualSegmentLabel(s) << endl;
      //cout<<"processed="<<processed<<" pending.size="<<pending.size()
      //	  <<" ret.size="<<ret.size()<<endl; 

      set<int>::const_iterator sit;
      for(sit=s.begin(); sit != s.end();sit++){
	const set<int> &nbr_i=nbr_primi[*sit];
	set<int>::const_iterator sit_i;
	for(sit_i=nbr_i.begin(); sit_i != nbr_i.end();sit_i++)
	  if(s.count(*sit_i)==0) nbr.insert(*sit_i);
      }

      //cout << "neighbours of " << formVirtualSegmentLabel(s) << ":" <<endl;
      //cout <<" "<<formVirtualSegmentLabel(nbr)<<endl;

      // now nbr contains the neighbours of s
      for(sit=nbr.begin();sit!=nbr.end();sit++){
	//cout << "considering neighbour " << *sit << endl;
	set<int> tmpset=s;
	tmpset.insert(*sit);
	if(pending.count(tmpset)==0){
	  pending.insert(tmpset);
	  ret.push_back(tmpset);
	}
      }
      processed++;
      if(processed%100 ==0)      cout << processed << " sets processed." << endl;

      }

    return ret;

  }

  ///////////////////////////////////////////////////////////////////////////
  
  labeledListList *segmentfile::changeLabels(int to, const segmentfile::Matrix &matrix)
    {
      coordList *l=convertToList(matrix);
      labeledListList *ret=changeLabels(to,*l);
      delete l;
      return ret;
    }

  ///////////////////////////////////////////////////////////////////////////

  labeledListList *segmentfile::changeLabels(int to, const coordList &list) {
    labeledListList *ret = new labeledListList;
    map<int,int> index;
  
    size_t listCursor;
    for(listCursor=0;listCursor<list.size();listCursor++){
      int oldlabel;
      get_pixel_segment(list[listCursor], oldlabel);
      
      if(index.count(oldlabel)==0){
	index[oldlabel]=ret->size();
	ret->push_back(labeledList());
	(*ret)[index[oldlabel]].label=oldlabel;
      }

      (*ret)[index[oldlabel]].l.push_back(list[listCursor]);
      set_pixel_segment(list[listCursor],to);
    }
    
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::translateLabels(int to,const coordList &list) {
    for(coordList::const_iterator it=list.begin();
	it != list.end();it++)
      set_pixel_segment(*it,to);
  }

  ///////////////////////////////////////////////////////////////////////////

  coordList segmentfile::listCoordinates(int label) {
    coordList ret;
    int x,y,s;
    for(y=0;y<getHeight();y++)
      for(x=0;x<getWidth();x++)
	{
	  get_pixel_segment(x,y,s);
	  if(s==label)
	    ret.push_back(coord(x,y));
	}
    
    return ret;
    
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::FindBox(coord &nw, coord &se ) {
      // find limits of the segmentation image

      int x,y,val;
      bool found=false;
      
      for(nw.y=0;nw.y<getHeight()&&!found;nw.y++)
	for(x=0;x<getWidth();x++){
	  get_pixel_segment(x,nw.y,val);
	  if(val !=0){
	    found=true;
	    break;
	  }
	}
      
      if(!found){
	// no objects in the whole image
	nw.x=nw.y=se.x=se.y=-1;
	return false;
      }
      
      found=false;
      for(se.y=getHeight()-1;!found;se.y--)
	for(x=0;x<getWidth();x++){
	  get_pixel_segment(x,se.y,val);
	  if(val !=0){
	    found=true;
	    break;
	  }
	}
      
      found=false;
      for(se.x=getWidth()-1;!found;se.x--)
	for(y=0;y<getHeight();y++){
	  get_pixel_segment(se.x,y,val);
	  if(val !=0){
	    found=true;
	    break;
	  }
	}
      
      found=false;
      for(nw.x=0;!found;nw.x++)
	for(y=0;y<getHeight();y++){
	  get_pixel_segment(nw.x,y,val);
	  if(val !=0){
	    found=true;
	    break;
	  }
	}
      
      return true;
    }

  ///////////////////////////////////////////////////////////////////////////
  void segmentfile::FilterLineEdges(const RGBTriple *means, 
				    const RGBTriple &bkRGB)
  {
      
    int x,y,delta;
    coordList changed;
    vector<int> newLabels;
    bool nextPixel;
    
    int a[5],b[10],ctr[3]={0,0,0},blabel;
    
    // first vertical lines
    
    for(y=2;y<getHeight()-2;y++) for(x=2;x<getWidth()-2;x++){
      for(delta=-2;delta<=2;delta++){
	get_pixel_segment(x-2,y+delta,b[2+delta]);
	get_pixel_segment(x+2,y+delta,b[7+delta]);
	get_pixel_segment(x,y+delta,a[2+delta]);
      }
	
      nextPixel=false;
      for(delta=1;delta<5;delta++) if(a[0] != a[delta]){
	nextPixel=true;
	break;
      }
      
      if(nextPixel) continue;
      
      for(delta=0;delta<10;delta++) if(a[0] == b[delta]){
	nextPixel=true;
	break;
      }
      
      if(nextPixel) continue;
      
      ctr[0]=ctr[1]=ctr[2]=0;
      
      for(delta=0;delta<10;delta++){
	if(b[0]==b[delta]) ctr[0]++;
	if(b[1]==b[delta]) ctr[1]++;
	if(b[2]==b[delta]) ctr[2]++;
      }

      if(ctr[0]>=8) blabel=b[0];
      else if(ctr[1]>=8) blabel=b[1];
      else if(ctr[2]>=8) blabel=b[2];
      else{
	continue;
      } 
	
      // get rid of this edge pixel
      if(means)
	set_pixel_rgb(x,y,means[blabel].v);
      else
	set_pixel_rgb(x,y,bkRGB.v);

      changed.push_back(coord(x,y));
      newLabels.push_back(blabel);
    }
      
    // horisontal lines
      
    for(y=2;y<getHeight()-2;y++) for(x=2;x<getWidth()-2;x++){
      for(delta=-2;delta<=2;delta++){
	get_pixel_segment(x+delta,y-2,b[2+delta]);
	get_pixel_segment(x+delta,y+2,b[7+delta]);
	get_pixel_segment(x+delta,y,a[2+delta]);
      }
      
      nextPixel=false;
      for(delta=1;delta<5;delta++) if(a[0] != a[delta]){
	nextPixel=true;
	break;
      }
      if(nextPixel) continue;
	
      for(delta=0;delta<10;delta++) if(a[0] == b[delta]){
	nextPixel=true;
	break;
      }
      if(nextPixel) continue;
      
      ctr[0]=ctr[1]=ctr[2]=0;
      
      for(delta=0;delta<10;delta++){
	if(b[0]==b[delta]) ctr[0]++;
	if(b[1]==b[delta]) ctr[1]++;
	if(b[2]==b[delta]) ctr[2]++;
      }
	
      if(ctr[0]>=8) blabel=b[0];
      else if(ctr[1]>=8) blabel=b[1];
      else if(ctr[2]>=8) blabel=b[2];
      else{
	continue;
      } 
	
      // get rid of this edge pixel
      if(means)
	set_pixel_rgb(x,y,means[blabel].v);
      else
	set_pixel_rgb(x,y,bkRGB.v);
      
      changed.push_back(coord(x,y));
      newLabels.push_back(blabel);
      
    }
    
    for(size_t i=0;i<changed.size();i++)
      set_pixel_segment(changed[i].x,changed[i].y,newLabels[i]);
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void segmentfile::FilterLineEdgesByRGB(float colourTolerance)
    {
      
      bool nextPixel;
      
      int x,y,delta;
      
      RGBTriple a[5],b[10],blabel;
      
      int ctr[3]={0,0,0};
      coordList changed;
      vector<RGBTriple> newLabels;
      
      // first vertical lines
      
      for(y=2;y<getHeight()-2;y++) for(x=2;x<getWidth()-2;x++){
	for(delta=-2;delta<=2;delta++){
	  get_pixel_rgb(x-2,y+delta,b[2+delta].v);
	  get_pixel_rgb(x+2,y+delta,b[7+delta].v);
	  get_pixel_rgb(x,y+delta,a[2+delta].v);
	}
	
	nextPixel=false;      
	for(delta=1;delta<5;delta++) if(a[0].SqrDist(a[delta])>colourTolerance){
	  //      cout << "a-test failed" << endl;
	  nextPixel=true;
	  break;
	}
	if(nextPixel) continue;
	
	for(delta=0;delta<10;delta++) if(a[0].SqrDist(b[delta])<colourTolerance){
	  //      cout << "b-test failed" << endl;
	  nextPixel=true;
	  break;
	}
	if(nextPixel) continue;
	
	ctr[0]=ctr[1]=ctr[2]=0;
	
	for(delta=0;delta<10;delta++){
	  if(b[0].SqrDist(b[delta])<colourTolerance) ctr[0]++;
	  if(b[1].SqrDist(b[delta])<colourTolerance) ctr[1]++;
	  if(b[2].SqrDist(b[delta])<colourTolerance) ctr[2]++;
	}
	
	if(ctr[0]>=8) blabel=b[0];
	else if(ctr[1]>=8) blabel=b[1];
	else if(ctr[2]>=8) blabel=b[2];
	else{
	  continue;
	} 
	
	// get rid of the line pixel
	
	changed.push_back(coord(x,y));
	newLabels.push_back(blabel);
	
      }
      
      // horisontal lines
      
      for(y=2;y<getHeight()-2;y++) for(x=2;x<getWidth()-2;x++){
	for(delta=-2;delta<=2;delta++){
	  get_pixel_rgb(x+delta,y-2,b[2+delta].v); 
	  get_pixel_rgb(x+delta,y+2,b[7+delta].v); 
	  get_pixel_rgb(x+delta,y,a[2+delta].v);
	}

	nextPixel=false;
	for(delta=1;delta<5;delta++) if(a[0].SqrDist(a[delta])>colourTolerance){
	  nextPixel=true;
	  break;
	}
	if(nextPixel) continue;
	
	for(delta=0;delta<10;delta++) if(a[0].SqrDist(b[delta])<colourTolerance){
	  nextPixel=true;
	  break;
	}
	if(nextPixel) continue;    
	
	ctr[0]=ctr[1]=ctr[2]=0;
	
	for(delta=0;delta<10;delta++){
	  if(b[0].SqrDist(b[delta])<colourTolerance) ctr[0]++;
	  if(b[1].SqrDist(b[delta])<colourTolerance) ctr[1]++;
	  if(b[2].SqrDist(b[delta])<colourTolerance) ctr[2]++;
	}
	
	if(ctr[0]>=8) blabel=b[0];
	else if(ctr[1]>=8) blabel=b[1];
	else if(ctr[2]>=8) blabel=b[2];
	else{
	  continue;
	} 
   
	// get rid of the line pixel
	
	changed.push_back(coord(x,y));
	newLabels.push_back(blabel);
	
      }

      for(size_t i=0;i<changed.size();i++)
	set_pixel_rgb(changed[i].x,changed[i].y,newLabels[i].v);
      
     //  if(Verbose()>1)
// 	cout << "FilterLineEdgesByRGB: filtered away " << changed.size() 
// 	     << " pixels." << endl; 

    }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::prepareZoning(const string& t) {
    string zoningType = t;
    int w = getWidth(), h = getHeight();

    bool tile = false;
    int tile_w = 0, tile_h = 0, tile_dx = 0, tile_dy = 0; 
    int d = 0;
    if (sscanf(zoningType.c_str(), "%dX%dl%d", &tile_w, &tile_h, &d)==3) {
      return false;
    }
    else if (sscanf(zoningType.c_str(), "%dX%d", &tile_w, &tile_h)==2) {
      tile = true;
      tile_dx = tile_w = getWidth()/tile_w;
      tile_dy = tile_h = getHeight()/tile_h;
    }
    else if (sscanf(zoningType.c_str(), "%dx%dl%d", &tile_w, &tile_h, &d)==3) {
      return false;
    }
    else if (sscanf(zoningType.c_str(), "%dx%d", &tile_w, &tile_h)==2) {
      tile = true;
      tile_dx = tile_w;
      tile_dy = tile_h;
    }

    if (!tile){
      if (zoningType=="1")
	zoningType = "flat";
      else if (zoningType=="5")
	zoningType = "centerdiag";
      else if (zoningType=="4")
	zoningType = "horizvert";
      else if (zoningType=="h")
	zoningType = "horiz";
      else if (zoningType=="v")
	zoningType = "vert";
    }

    bool center = zoningType.find("center") != string::npos;
    bool diag   = zoningType.find("diag")   != string::npos;
    bool horiz  = zoningType.find("horiz")  != string::npos;
    bool vert   = zoningType.find("vert")   != string::npos;
    bool flat   = zoningType.find("flat")   != string::npos;
    
    zoningText = zoningType;
    
    if (tile)
      return zoningTiles(w, h, tile_w, tile_h, tile_dx, tile_dy);

    if (flat)
      return zoningNone(w, h);
    
    if (center  && diag  && !horiz && !vert)
      return zoningTheOriginalOne(w, h);

    if (center  && !diag && horiz  && vert)
      return zoningCenterHorizVert(w, h);

    if (!center && !diag && horiz  && vert)
      return zoningHorizVert(w, h);

    if (center  && !diag && !horiz && !vert)
      return zoningCenter(w, h);

    if (!center && !diag && horiz  && !vert)
      return zoningHoriz(w, h);

    if (!center && !diag && !horiz && vert)
      return zoningVert(w, h);

    zoningText= "Zoning type center=" + string(center?"1":"0");
    zoningText += " diag="  + string(diag?"1":"0");
    zoningText += " horiz=" + string(horiz?"1":"0");
    zoningText += " vert="  + string(vert?"1":"0");
    zoningText += "not defined"; 
    
    return false;
  }

  ///////////////////////////////////////////////////////////////////////////
   
  xmlNodePtr FileInfo::outputXML() const {

    string errhead = "FileInfo::outputXML(): ";

      xmlNodePtr fileinfo=xmlNewNode(NULL,(xmlChar*)"fileinfo");
  
      if(!fileinfo)
	return NULL;

      if(imgcolordepth.empty()) 
	throw errhead+"image color depth not given";
      if(imgwidth.empty()) 
	throw errhead+"image width not given";
      if(imgheight.empty()) 
	throw errhead+"image height not given";

      if(!imgname.empty())
	xmlNewTextChild(fileinfo,NULL,(xmlChar*)"name",(xmlChar*)(imgname.c_str()));
      if(!imgurl.empty())
	xmlNewTextChild(fileinfo,NULL,(xmlChar*)"url",(xmlChar*)(imgurl.c_str()));
      if(!imgpageurl.empty())
	xmlNewTextChild(fileinfo,NULL,(xmlChar*)"pageurl",(xmlChar*)(imgpageurl.c_str()));
      if(!imgformat.empty())
	xmlNewTextChild(fileinfo,NULL,(xmlChar*)"format",(xmlChar*)(imgformat.c_str()));
   
      xmlNewTextChild(fileinfo,NULL,(xmlChar*)"colordepth",(xmlChar*)(imgcolordepth.c_str()));
      xmlNewTextChild(fileinfo,NULL,(xmlChar*)"width",(xmlChar*)(imgwidth.c_str()));
      xmlNewTextChild(fileinfo,NULL,(xmlChar*)"height",(xmlChar*)(imgheight.c_str()));
   
      if(!imgfilesize.empty())
	xmlNewTextChild(fileinfo,NULL,(xmlChar*)"size",(xmlChar*)(imgfilesize.c_str()));
      if(!imgchecksum.empty())
	xmlNewTextChild(fileinfo,NULL,(xmlChar*)"checksum",(xmlChar*)(imgchecksum.c_str()));
      if(!imgdate.empty())
	xmlNewTextChild(fileinfo,NULL,(xmlChar*)"date",(xmlChar*)(imgdate.c_str()));
 
      return fileinfo;
    }
  ///////////////////////////////////////////////////////////////////////////
  bool FileInfo::parseXML(const xmlNodePtr node){

    if(XMLTools::strcmpXML(node->name,"fileinfo")) return false;

    xmlNodePtr field=XMLTools::xmlNodeFindDescendant(node,"name");
    imgname= field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"url");
    imgurl=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"pageurl");
    imgpageurl=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"format");
    imgformat=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"colordepth");
    imgcolordepth=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"width");
    imgwidth=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"height");
    imgheight=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"filesize");
    imgfilesize=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"checksum");
    imgchecksum=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"date");
    imgdate=field ? XMLTools::xmlNodeGetContent(field) : "";

    return true;

  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr ProcessInfo::outputXML() const {
    string errhead = "ProcessInfo::outputXML(): ";

    xmlNodePtr processinfo=xmlNewNode(NULL,(xmlChar*)"processinfo");

    if(!processinfo)
      return NULL;

    if(user.empty()) 
      throw errhead+"user not given";
    if(host.empty()) 
      throw errhead+"host not given";
    if(cwd.empty()) 
      throw errhead+"cwd given";
    if(commandline.empty()) 
      throw errhead+"commandline not given";
    
    xmlNewTextChild(processinfo,NULL,(xmlChar*)"user",(xmlChar*)(user.c_str()));
    xmlNewTextChild(processinfo,NULL,(xmlChar*)"host",(xmlChar*)(host.c_str()));
    xmlNewTextChild(processinfo,NULL,(xmlChar*)"date",(xmlChar*)(procdate.c_str()));
    xmlNewTextChild(processinfo,NULL,(xmlChar*)"cwd",(xmlChar*)(cwd.c_str()));
    xmlNewTextChild(processinfo,NULL,(xmlChar*)"commandline",(xmlChar*)(commandline.c_str()));

    xmlNewTextChild(processinfo,NULL,(xmlChar*)"framestep",
		    (xmlChar*)ToStr(framestep).c_str());

    xmlAddChild(processinfo,options.produceXML());

    return processinfo;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool ProcessInfo::parseXML(const xmlNodePtr node) {
    if(XMLTools::strcmpXML(node->name,"processinfo")) return false;

    xmlNodePtr field=XMLTools::xmlNodeFindDescendant(node,"user");
    user = field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"host");
    host=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"date");
    procdate=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"cwd");
    cwd=field ? XMLTools::xmlNodeGetContent(field) : "";
    field=XMLTools::xmlNodeFindDescendant(node,"commandline");
    commandline=field ? XMLTools::xmlNodeGetContent(field) : "";

    return true;
  }

 ///////////////////////////////////////////////////////////////////////////

    ///
    int segmentfile::storeImage(const imagedata &i){
      int idx=stored_images.size(); // always allocate new id,
	                           // hopefully namespace never ends
	                           // (it shouldn't)

	_frame_type tmp;

	tmp.framenr=idx;
	tmp.data=new imagedata(i);
	tmp.owndata=true;
	tmp.segment=false;
	tmp.frameSrc=no_source;
	stored_images[idx]=tmp;
	
	return idx;

    }
  // store image to next free location, return its new identifier
    
  ///
  int segmentfile::storeSegment(const imagedata &i){
    _frame_type tmp;
    
    int idx=stored_segments.size();
    
    tmp.framenr=idx;
    tmp.data=new imagedata(i);
    tmp.owndata=true;
    tmp.segment=false;
    tmp.data->convert(_getSegmentframeDatatype());
    tmp.frameSrc=no_source;
    stored_images[idx]=tmp;
    
    return idx;
  }

  ///
  bool segmentfile::removeImage(int id){
    if(stored_images.count(id))
      if(stored_images[id].owndata) delete stored_images[id].data;
    stored_images.erase(id);

    return true;
  }

  /// 
  bool segmentfile::removeSegment(int id){
    if(stored_segments.count(id))
      if(stored_segments[id].owndata) delete stored_segments[id].data;
    stored_segments.erase(id);

    return true;
  }

    ///
    imagedata *segmentfile::copyStoredImage(int id){
      const imagedata *model=storedImage(id);
	return new imagedata(*model);

    }

    ///
    imagedata *segmentfile::copyStoredSegment(int id){
      const imagedata *model=storedImage(id);
	return new imagedata(*model);
    }


 ///////////////////////////////////////////////////////////////////////////
  bool segmentfile::getAlignmentMarks(coord &m1,coord &m2,
				      coord &m3,coord &m4) {
    string topstr = "alignment_marks_top";
    string bottomstr = "alignment_marks_bottom";

    SegmentationResult *r1 = readLastFrameResultFromXML(topstr);
    SegmentationResult *r2 = readLastFrameResultFromXML(bottomstr);

    if (r1 && r2) {
      istringstream *str = new istringstream(r1->value.c_str());
      
      (*str) >> m1 >> m2;
      delete str;

      str = new istringstream(r2->value.c_str());
      (*str) >> m3 >> m4;
      delete str;
      
      delete r1;
      delete r2;
      
      return true;
    }
    
    delete r1;
    delete r2;
    
    string *stringptr;
    
    SegmentationResult *result = readLastFrameResultFromXML("alignment_marks");
    if (result)
      stringptr = &(result->value);
    else {
      return false;
    }
    
    istringstream str(stringptr->c_str());

    string dummy;
    str >> dummy >> m1 >> m2;
    str >> dummy >> m3 >> m4;
    
    delete result;
    return true;
  }	   
  
  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getColouring(const vector<int> *labels,
				       const vector<RGBTriple> *colours){
    
    imagedata *img = getEmptyFrame();
    return getColouring(img,labels,colours);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getColouring(imagedata *img, 
				       const vector<int> *labels,
				       const vector<RGBTriple> *colours) {
    if(!labels) return NULL;
    
    vector<RGBTriple> std_colours;
    vector<int> l(*labels);
    
    
    if(!colours || labels->size() != colours->size()){
      expandLabeling(l);
      getStdColouring(l.size(),std_colours);
      colours=&std_colours;
    }
    
    map<int,RGBTriple> *colouring=expandColouring(l,*colours,RGBTriple(0,0,0));
    

 //    map<int,RGBTriple>::iterator cit;
    
//     for(cit=colouring->begin();cit!=colouring->end();cit++)
//       cout << "colour "<<cit->first<<": (" 
// 	   << cit->second.r() << " "
// 	   << cit->second.g() << " " 
// 	   << cit->second.b() << endl;

    int x,y,label;
    
    if((int)img->width() != getWidth()) 
      throw string("segmentfile::getColouring : incompatible widths");
    
    if((int)img->height() != getHeight()) 
      throw string("segmentfile::getColouring : incompatible heights");
    
    for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
      get_pixel_segment(x,y,label);
      //     cout << "Setting colour to ("
//        <<      (*colouring)[label].r() << " "
//        <<     (*colouring)[label].g() << " "
//        <<     (*colouring)[label].b() << endl;
      
      if(!colouring->count(label))
	throw string("segmentfile::getColouring : not colour for all labels.");

      img->set(x,y,(*colouring)[label].v);
    }
  
    return img;

  }
 ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getAverageRgb() {
    imagedata *img = getEmptyFrame();
    return getAverageRgb(img);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getAverageRgb(imagedata *img) {
    
    map<int,int> count;
    map<int,RGBTriple> colours;
    
    int x,y,label;
    vector<float> vf;
    
    if((int)img->width() != getWidth()) 
      throw string("segmentfile::AverageRgb : incompatible widths");
    
    if((int)img->height() != getHeight()) 
      throw string("segmentfile::getAverageRgb : incompatible heights");
    
    for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
      get_pixel_segment(x,y,label);
      get_pixel_rgb(x,y,vf);
      if(!colours.count(label)){
	count[label]=1;
	colours[label].v = vf;
      }
      else{
	count[label]++;
	colours[label].v[0] += vf[0];
	colours[label].v[1] += vf[1];
	colours[label].v[2] += vf[2];
      }
    }

    map<int,RGBTriple>::iterator mit;
    for(mit=colours.begin();mit!=colours.end();mit++){
      int c=count[mit->first];
      mit->second.v[0] /= c;
      mit->second.v[1] /= c;
      mit->second.v[2] /= c;
    }
    

    for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
      get_pixel_segment(x,y,label);
      
      //     cout << "Setting colour to ("
//        <<      (*colouring)[label].r() << " "
//        <<     (*colouring)[label].g() << " "
//        <<     (*colouring)[label].b() << endl;
      
      if(!colours.count(label))
	throw string("segmentfile::getColour: not colour for all labels.");

      img->set(x,y,colours[label].v);
    }
  
    return img;

  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getDimmedImage(const vector<int> 
					 *labels,
					 const vector<float> 
					 *dims,
					 float default_dim, 
					 const vector<Region*> *region_spec) {
    imagedata *img = getEmptyFrame();
    return getDimmedImage(img,labels,dims,default_dim,region_spec);
  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getDimmedImage(imagedata *img,
					 const vector<int> 
					 *labels,
					 const vector<float>
					 *dims,
					 float default_dim,
					 const vector<Region*> *region_spec) {

    if(!labels) return NULL;

    vector<float> std_dims(labels->size(),1.0);
    if(!dims || labels->size() != dims->size())
      dims=&std_dims;
    
    if((int)img->width() != getWidth()) 
      throw string("segmentfile::getDimmedImage : incompatible widths");
      
    if((int)img->height() != getHeight()) 
      throw string("segmentfile::getDimmedImage : incompatible heights");
    
    if(region_spec==NULL){

      map<int,float> *mask=expandLabeling(*labels,*dims,default_dim);
    
      int x,y,label;
      float r,g,b;

      for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
	get_pixel_segment(x,y,label);
	get_pixel_rgb(x,y,r,g,b);
	
	vector<float> vf(3);
	vf[0]=(*mask)[label]*r;
	vf[1]=(*mask)[label]*g;
	vf[2]=(*mask)[label]*b;
	
	img->set(x,y,vf);
      }

      delete mask;
    } else{

      float r,g,b;
      vector<float> vf(3);

      imagedata done(getWidth(), getHeight(), 1, imagedata::pixeldata_uchar);

      for(size_t i=0;i<labels->size();i++){
	size_t l=(*labels)[i];

	cout << "visualising label " << l << endl;

	//	cout << "region_spec has size " << region_spec->size() << endl;

	int w=getWidth();
	int h=getHeight();
	
	if(l>0&&l<=region_spec->size()){

	  // virtual region labels start from 1

	  coordList c=(*region_spec)[l-1]->listCoordinates();
	  float dim=(*dims)[i];
	  //cout << "dim="<<dim<<endl;

	  for(size_t ii=0;ii<c.size();ii++){
	    int x=c[ii].x;
	    int y=c[ii].y;

	    if(x>=0&&x<w&&y>=0&&y<h){

	      //  cout << "visualising (x,y)=("<<x<<","<<y<<")"<<endl;
	      
	      get_pixel_rgb(x,y,r,g,b);
	      
	      vf[0]=dim*r;
	      vf[1]=dim*g;
	      vf[2]=dim*b;
	
	      img->set(x,y,vf);
	      done.set(x, y, (unsigned char)1);
	    }
	  }
	}
      }

      for (int y=0; y<getHeight(); y++)
	for (int x=0; x<getWidth(); x++)
	  if (!done.get_one_uchar(x, y)) {
	    get_pixel_rgb(x, y, r, g, b);
	    vf[0] = default_dim*r;
	    vf[1] = default_dim*g;
	    vf[2] = default_dim*b;
	    img->set(x, y, vf);
	  }
    }
    
    return img;
  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getHueMasking(const vector<int> *labels,
					const vector<float> *levels,
					float default_level) {
    imagedata *img = getEmptyFrame();
    return getHueMasking(img,labels,levels,default_level);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getHueMasking(imagedata *img, 
					const vector<int> *labels,
					const vector<float>*levels,
					float default_level){
    if(!labels) return NULL;

    vector<float> std_levels(labels->size(),1.0);
    if(!levels || labels->size() != levels->size())
      levels=&std_levels;
    
    map<int,float> *mask=expandLabeling(*labels,*levels,default_level);

    int x,y,label;
    float r,g,b;

    if((int)img->width() != getWidth()) 
      throw string("segmentfile::getHueMasking : incompatible widths");

    if((int)img->height() != getHeight()) 
      throw string("segmentfile::getHueMasking : incompatible heights");

    vector<float> vf(3);

    for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
      get_pixel_segment(x,y,label);
      get_pixel_rgb(x,y,r,g,b);

      float ave=(r+g+b)/3;

      vf[0]=(*mask)[label]*r+(1-(*mask)[label])*ave;
      vf[1]=(*mask)[label]*g+(1-(*mask)[label])*ave;
      vf[2]=(*mask)[label]*b+(1-(*mask)[label])*ave;

      img->set(x,y,vf);
    }
  
    delete mask;
    return img;
  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getCrossedImage(const vector<int> *labels,
					  int w, int s) {
    imagedata *img = getEmptyFrame();
    return getCrossedImage(img,labels,w,s);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::getCrossedImage(imagedata *img, 
					  const vector<int> *labels,
					  int w, int s) {
    if(!labels) return NULL;

    const vector<float> *levels = NULL;

    vector<float> std_levels(labels->size(),1.0);
    if(!levels || labels->size() != levels->size())
      levels=&std_levels;
    
    map<int,float> *mask=expandLabeling(*labels,*levels,0.0);

    if((int)img->width() != getWidth()) 
      throw string("segmentfile::getCrossedImage : incompatible widths");

    if((int)img->height() != getHeight()) 
      throw string("segmentfile::getCrossedImage : incompatible heights");

    vector<float> black(3, 0.0);
    vector<float> white(3, 1.0);
    vector<float> vf(3);

    int x,y,label;

    for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
      float r,g,b;
      get_pixel_rgb(x,y,r,g,b);
      vf[0]=r;
      vf[1]=g;
      vf[2]=b;
      img->set(x,y, vf);

      get_pixel_segment(x,y,label);
      if ((*mask)[label]==1.0)
	continue;

      int ra = x-y;
      while (ra<0)
	ra += s;
      ra %= s;

      int rb = (x+y)%s;

      if (ra<w)
	img->set(x,y, black);

      else if (ra>=s/2 && ra<s/2+w)
	img->set(x,y, white);

      else if (rb<w)
	img->set(x,y, black);

      else if (rb>=s/2 && rb<s/2+w)
	img->set(x,y, white);
    }
  
    delete mask;
    return img;
  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::markBoundaries(const vector<int> *labels,
					 bool outerBoundaries,
					 const RGBTriple &rgb) 
  {
    imagedata *img = copyFrame();
    return markBoundaries(img,labels,outerBoundaries,rgb);
  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::markBoundaries(imagedata *img,
					 const vector<int> *labels,
					 bool outerBoundaries,
					 const RGBTriple &rgb)
  {

    if((int)img->width() != getWidth()) 
      throw string("segmentfile::markBoundaries : incompatible widths");
    
    if((int)img->height() != getHeight()) 
      throw string("segmentfile::markBoundaries : incompatible heights");
    
    if(labels){

      // cout << "mark Boundaries of given labels." << endl;

      set<int> includeSegment;
      vector<int> l(*labels);
      expandLabeling(l);
      
      for(size_t i=0;i<l.size();i++)
	{
	  includeSegment.insert(l[i]);
	  //	  cout << "including segment " << l[i] << endl;
	}
      int x,y,label;
      
      int h=getHeight(),w=getWidth();

      for(y=0;y<h;y++) for(x=0;x<w;x++){
	get_pixel_segment(x,y,label);
	if(includeSegment.count(label) && isSegmentBoundary(x,y)){
	  //	  cout << "Marking boundary pixel (" << x <<","<<y<<")"<<endl;
	  img->set(x,y,rgb.v);
	  if(outerBoundaries){
	    for(int dx=-1;dx<=1;dx++){
	      if(x+dx<0) continue;
	      if(x+dx>=w) continue;
	      for(int dy=-1;dy<=1;dy++){
		if(y+dy<0) continue;
		if(y+dy>=h) continue;
		if(dx==0 && dy==0) continue;
		int l1;
		get_pixel_segment(x+dx,y+dy,l1);
		if(l1!=label)
		  img->set(x+dx,y+dy,rgb.v);
	      }
	    }
	  }
	}

      }
    }
    else{
      int x,y;
      for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++)
	if(isSegmentBoundary(x,y)){
	  //cout << "Marking boundary pixel (" << x <<","<<y<<")"<<endl;
	  img->set(x,y,rgb.v);
	}
    }
    
    return img;
  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::showResult(const string &value, 
				     const string &type,
				     const string &colour) {
    imagedata *img = copyFrame();
    return showResult(img,value,type,NamedRGB::GetRGB(colour));
  }

  ///////////////////////////////////////////////////////////////////////////
  imagedata *segmentfile::showResult(imagedata *img,
				     const string &value, 
				     const string &type,
				     const RGBTriple &colour) {

    RGBTriple c(colour);
    value.c_str();
    type.c_str();

    // should be implemented

    return img;

  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::showDataTypes() {
    cout << "Dumping data types: " << endl;

    for (int f=0; f<getNumFrames(); f++)
      cout << "frame " << f << " img: " << imageFrame(f)->datatypename()
	   << endl 
	   << " seg: " << segmentFrame(f)->datatypename() << endl;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::SetStandardInputNaming(const string& met,
					   const string& dir) { 
    string pattern = StandardNaming(dir);
    size_t patpos = pattern.rfind("%m");
    if (patpos!=string::npos)  // was (patpos>=0) when patpos was int
      pattern.replace(patpos, 2, met); 
    SetInputPattern(pattern);
  }

  ///////////////////////////////////////////////////////////////////////////

  string segmentfile::FormOutputFileName(const string& pattern,
					 const string& in_name,
					 const string& mstr) {

    string tmp = in_name;
    string::size_type dotpos   = tmp.rfind(".");

    if (dotpos!=string::npos && (tmp.substr(dotpos)==".gz" ||
				 tmp.substr(dotpos)==".bz2"))
      dotpos = tmp.rfind(".", dotpos-1);

    string::size_type slashpos = tmp.rfind("/");
    if (dotpos!=string::npos &&
	(slashpos==string::npos || dotpos>slashpos))
      tmp.erase(dotpos, string::npos);
    
    if (!pattern.size())
      return tmp+".seg";
    
    if (slashpos!=string::npos)
      tmp.erase(0, slashpos+1);
    string tmp2 = pattern;
    
    bool modified = true;
    while (modified) {
      modified = false;
      
      string::size_type patpos = tmp2.rfind("%i");
      if (patpos!=string::npos) {
	tmp2.replace(patpos, 2, tmp);
	modified = true;
      }
      
      patpos = tmp2.rfind("%m");
      if (patpos!=string::npos) {
	tmp2.replace(patpos, 2, mstr);
	modified = true;
      }
      
      patpos = tmp2.rfind("%o");
      if (patpos!=string::npos) {
	tmp2.replace(patpos, 2, "");  // obs!
	modified = true;
      }

      patpos = tmp2.rfind("%b");
      if (patpos!=string::npos) {
	string dir = in_name;
	size_t q = dir.rfind('/');
	if (q!=string::npos)
	  dir.erase(q);
	else
	  dir = "";

	tmp2.replace(patpos, 2, dir);
	modified = true;
      }

      patpos = tmp2.rfind("%B");
      if (patpos!=string::npos) {
	string dir = in_name;
	size_t q = dir.rfind('/');
	if (q!=string::npos)
	  dir.erase(q);
	else
	  dir = "";

	if (dir.size()>2 && dir.substr(dir.size()-2)==".d") {
	  q = dir.rfind('/');
	  if (q!=string::npos)
	    dir.erase(q);
	  else
	    dir = "";
	}

	tmp2.replace(patpos, 2, dir);
	modified = true;
      }
    }
    
    return tmp2;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  string segmentfile::FormInputFileName(const string& pattern,
					      const string& in_name,
					      const string& methodstr) {
    
    // remove some tokens, then call FormOutputFileName()
    
    string tmp=pattern;
    bool modified=true;
    string::size_type patpos;
    
    while(modified){
      modified=false;
      patpos = tmp.rfind("%m");
      
      if (patpos!=string::npos) {
	tmp.replace(patpos, 2, "");
	modified = true;
      }
      
      patpos = tmp.rfind("%o");
      if (patpos!=string::npos) {
	tmp.replace(patpos, 2, "");
	modified = true;
      }
    }
    
    return FormOutputFileName(tmp,in_name,methodstr);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void segmentfile::formFileNames(const char *img, const char *out, 
				  const string& m) {
    naming.image_name=img;
    
    if (out){
      naming.out_segment_name = out;
    }
    else if(!naming.output_pattern.empty()){
      naming.out_segment_name = FormOutputFileName(naming.output_pattern,
						   img, m);
    }
    
    if(!naming.input_pattern.empty()){
      // cout << "using input pattern " << naming.input_pattern << endl;
      naming.in_segment_name = FormInputFileName(naming.input_pattern,img,m);
    }

  }
  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::showHist() {
    // produces a debug printout showing histograms of segmentations

    int originalFrame=getCurrentFrame();

    map<int,int> accu;
      
    for (int f=0; f<getNumFrames(); f++) {
      setCurrentFrame(f);
      accu.clear();
      for (int y=0; y<getHeight(); y++)
	for(int x=0; x<getWidth(); x++) {
	  int v = 0;
	  get_pixel_segment(x, y, v);
	  accu[v]++;
	}

      map<int,int>::iterator it;
	
      for (it=accu.begin(); it!=accu.end(); it++)
	cout << "count["<<it->first<<"]: " << it->second << endl;
    }
  
    setCurrentFrame(originalFrame);
  }

  ///////////////////////////////////////////////////////////////////////////

  int segmentfile::countFrameColours() {
    set<RGBTriple,ltRGB> m;

    int x,y;
    RGBTriple rgb;

    for(y=0;y<getHeight();y++) for(x=0;x<getWidth();x++){
      get_pixel_rgb(x,y,rgb.v);
      m.insert(rgb);
    }
    
    return m.size();

  }
  ///////////////////////////////////////////////////////////////////////////

  string segmentfile::collectChainedName() const{

    return description.methods.collectChainedName();
  }


  ///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::accessPreprocessResult(int f, const std::string &id){

    const pair<int,string> key(f,id);

    if(preprocess_results.count(key)==0){
      if(id.substr(0,11)=="sobelthresh")
	preprocess_results[key]=preprocess_SobelThreshold(f,id);
      else if(id.substr(0,8)=="sobelmax")
	preprocess_results[key]=preprocess_SobelMax(f,id);
      else if(id.substr(0,5)=="sobel")
	preprocess_results[key]=preprocess_Sobel(f,id);
      else if(id.substr(0,6)=="cielab")
	preprocess_results[key]=preprocess_Cielab(f,id);
      else if(id.substr(0,6)=="cieluv")
	preprocess_results[key]=preprocess_Cielab(f,id);
      else if(id.substr(0,9)=="hueminmax")
	preprocess_results[key]=preprocess_HueMinMax(f,id);
      else if(id.substr(0,18)=="huediffsumquant256")
	preprocess_results[key]=preprocess_HueDiffSumQuant256(f,id);


      else
	throw string("unknown preprocess type: ")+id;
    }

    return preprocess_results[key];
    
  }
  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::discardPreprocessResult(int f, const std::string &id){

    const pair<int,string> key(f,id);
    
    if(preprocess_results.count(key))
      delete preprocess_results[key];
    preprocess_results.erase(key);
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::discardAllPreprocessResults(){
    
    map<std::pair<int,std::string>, preprocessResult*>::iterator it;
    for(it=preprocess_results.begin();it!=preprocess_results.end();it++)
      delete it->second;

    preprocess_results.clear();
  }
  ///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::preprocess_SobelThreshold(int f, const std::string &){

    const bool writefiles=true;

    const int oldf=getCurrentFrame();

    setCurrentFrame(f);

    preprocessResult_SobelThresh *ret=new preprocessResult_SobelThresh;

    const int w=getWidth(),h=getHeight();

    const int l=w*h;

    ret->d0=new int[l];
    ret->d45=new int[l];
    ret->d90=new int[l];
    ret->d135=new int[l];

    float *ds0=new float[l];
    float *ds45=new float[l];
    float *ds90=new float[l];
    float *ds135=new float[l];
    
    float *di0=new float[l];
    float *di45=new float[l];
    float *di90=new float[l];
    float *di135=new float[l];

    float max_ds0=0,max_ds45=0,max_ds90=0,max_ds135=0;
    float max_di0=0,max_di45=0,max_di90=0,max_di135=0;

    float *i_ch=new float[l],*s_ch=new float[l];
    

    // find channels (S,I) of the HSI representation
    vector<float> vf;
    int ind=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vf = imageFrame(f)->get_float(x,y); 
      const float cmin=min(vf[0],min(vf[1],vf[2]));
      const float csum=vf[0]+vf[1]+vf[2];
      
      i_ch[ind]=csum/3;
      s_ch[ind]=csum==0?0:1-3*cmin/csum;
      ind++;
    }

    // form sobel magnitude images (neglecting border of width 1)

    int iu,id;

    float val;

    for(int y=1;y<h-1;y++) {
      ind=y*w+1;
      for(int x=1;x<w-1;x++){

	iu=ind-w;
	id=ind+w;
	
	// s channel
	
	val=ds0[ind]=fabs(-s_ch[iu-1]+s_ch[iu+1]
			  -2*s_ch[ind-1]+2*s_ch[ind+1]
			  -s_ch[id-1]+s_ch[id+1]);
	if(val>max_ds0) max_ds0=val;
	
	val=ds45[ind]=fabs(s_ch[iu]+2*s_ch[iu+1]
			   -s_ch[ind-1]+s_ch[ind+1]
			   -2*s_ch[id-1]-s_ch[id]);
	if(val>max_ds45) max_ds45=val;
	
	val=ds90[ind]=fabs(s_ch[iu-1]+2*s_ch[iu]+s_ch[iu+1]
			   -s_ch[id-1]-2*s_ch[id]-s_ch[id+1]);

	if(val>max_ds90) max_ds90=val;
	
	val=ds135[ind]=fabs(2*s_ch[iu-1]+s_ch[iu]
			    +s_ch[ind-1]-s_ch[ind+1]
			    -s_ch[id]-2*s_ch[id+1]);
	if(val>max_ds135) max_ds135=val;


	// i channel
	
	val=di0[ind]=fabs(-i_ch[iu-1]+i_ch[iu+1]
			  -2*i_ch[ind-1]+2*i_ch[ind+1]
			  -i_ch[id-1]+i_ch[id+1]);
	if(val>max_di0) max_di0=val;
	
	val=di45[ind]=fabs(i_ch[iu]+2*i_ch[iu+1]
			   -i_ch[ind-1]+i_ch[ind+1]
			   -2*i_ch[id-1]-i_ch[id]);
	if(val>max_di45) max_di45=val;
	
	val=di90[ind]=fabs(i_ch[iu-1]+2*i_ch[iu]+i_ch[iu+1]
			   -i_ch[id-1]-2*i_ch[id]-i_ch[id+1]);

	if(val>max_di90) max_di90=val;
	
	val=di135[ind]=fabs(2*i_ch[iu-1]+i_ch[iu]
			    +i_ch[ind-1]-i_ch[ind+1]
			    -i_ch[id]-2*i_ch[id+1]);
	if(val>max_di135) max_di135=val;


	ind++;
      }
    }

    // zero borders

    const int lastrow=(h-1)*w;

    for(int x=0;x<w;x++){
      int i=lastrow+x;
      ds0[x]=ds45[x]=ds90[x]=ds135[x]=0;
      di0[x]=di45[x]=di90[x]=di135[x]=0;
      ds0[i]=ds45[i]=ds90[i]=ds135[i]=0;
      di0[i]=di45[i]=di90[i]=di135[i]=0;
    }

    for(ind=w;ind<w*h;ind+=w){
      ds0[ind]=ds45[ind]=ds90[ind]=ds135[ind]=0;
      di0[ind]=di45[ind]=di90[ind]=di135[ind]=0;
      ds0[ind-1]=ds45[ind-1]=ds90[ind-1]=ds135[ind-1]=0;
      di0[ind-1]=di45[ind-1]=di90[ind-1]=di135[ind-1]=0;
    }

    // threshold and combine channels with OR

    for(ind=0;ind<w*h;ind++){
      ret->d0[ind]=
	(ds0[ind]>=0.35*max_ds0 || di0[ind]>=0.15*max_di0)?1:0; 
      ret->d45[ind]=
	(ds45[ind]>=0.35*max_ds45 || di45[ind]>=0.15*max_di45)?1:0; 
      ret->d90[ind]=
	(ds90[ind]>=0.35*max_ds90 || di90[ind]>=0.15*max_di90)?1:0; 
      ret->d135[ind]=
	(ds135[ind]>=0.35*max_ds135 || di135[ind]>=0.15*max_di135)?1:0; 
    }

    delete [] ds0;
    delete [] ds45;
    delete [] ds90;
    delete [] ds135;

    delete [] di0;
    delete [] di45;
    delete [] di90;
    delete [] di135;


    delete[] i_ch;
    delete[] s_ch;

    if(writefiles){
      cout << "writing edge detection results" << endl;

      picsom::imagedata img(w, h, 1,picsom::imagedata::pixeldata_uchar);
      for(int y=0;y<h;y++) for(int x=0;x<w;x++)
	img.set(x,y,(unsigned char)((ret->d0[x+y*w])?255:0));
      picsom::imagefile::write(img, "edges-0.png");

      for(int y=0;y<h;y++) for(int x=0;x<w;x++)
	img.set(x,y,(unsigned char)((ret->d45[x+y*w])?255:0));
      picsom::imagefile::write(img, "edges-45.png");

      for(int y=0;y<h;y++) for(int x=0;x<w;x++)
	img.set(x,y,(unsigned char)((ret->d90[x+y*w])?255:0));
      picsom::imagefile::write(img, "edges-90.png");

      for(int y=0;y<h;y++) for(int x=0;x<w;x++)
	img.set(x,y,(unsigned char)((ret->d135[x+y*w])?255:0));
      picsom::imagefile::write(img, "edges-135.png");
    
    }

    setCurrentFrame(oldf);
    return ret;
 
  }

  ///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::preprocess_SobelMax(int f, const std::string &){
    const bool writefiles=false;

    const int oldf=getCurrentFrame();

    setCurrentFrame(f);

    preprocessResult_SobelMax *ret=new preprocessResult_SobelMax;

    const int w=getWidth(),h=getHeight();

    const int l=w*h;

    ret->d=new float[l];
   
    float *ds0=new float[l];
    float *ds45=new float[l];
    float *ds90=new float[l];
    float *ds135=new float[l];
    
    float *di0=new float[l];
    float *di45=new float[l];
    float *di90=new float[l];
    float *di135=new float[l];

    float max_ds0=0,max_ds45=0,max_ds90=0,max_ds135=0;
    float max_di0=0,max_di45=0,max_di90=0,max_di135=0;

    float *i_ch=new float[l],*s_ch=new float[l];
    

    // find channels (S,I) of the HSI representation
    vector<float> vf;
    int ind=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vf = imageFrame(f)->get_float(x,y); 
      const float cmin=min(vf[0],min(vf[1],vf[2]));
      const float csum=vf[0]+vf[1]+vf[2];
      
      i_ch[ind]=csum/3;
      s_ch[ind]=csum==0?0:1-3*cmin/csum;

      ind++;
    }

    // form sobel magnitude images (neglecting border of width 1)

    int iu,id;

    float val;

    for(int y=1;y<h-1;y++) {
      ind=y*w+1;
      for(int x=1;x<w-1;x++){

	iu=ind-w;
	id=ind+w;
	
	// s channel
	
	val=ds0[ind]=fabs(-s_ch[iu-1]+s_ch[iu+1]
			  -2*s_ch[ind-1]+2*s_ch[ind+1]
			  -s_ch[id-1]+s_ch[id+1]);
	if(val>max_ds0) max_ds0=val;
	
	val=ds45[ind]=fabs(s_ch[iu]+2*s_ch[iu+1]
			   -s_ch[ind-1]+s_ch[ind+1]
			   -2*s_ch[id-1]-s_ch[id]);
	if(val>max_ds45) max_ds45=val;
	
	val=ds90[ind]=fabs(s_ch[iu-1]+2*s_ch[iu]+s_ch[iu+1]
			   -s_ch[id-1]-2*s_ch[id]-s_ch[id+1]);

	if(val>max_ds90) max_ds90=val;
	
	val=ds135[ind]=fabs(2*s_ch[iu-1]+s_ch[iu]
			    +s_ch[ind-1]-s_ch[ind+1]
			    -s_ch[id]-2*s_ch[id+1]);
	if(val>max_ds135) max_ds135=val;


	// i channel
	
	val=di0[ind]=fabs(-i_ch[iu-1]+i_ch[iu+1]
			  -2*i_ch[ind-1]+2*i_ch[ind+1]
			  -i_ch[id-1]+i_ch[id+1]);
	if(val>max_di0) max_di0=val;
	
	val=di45[ind]=fabs(i_ch[iu]+2*i_ch[iu+1]
			   -i_ch[ind-1]+i_ch[ind+1]
			   -2*i_ch[id-1]-i_ch[id]);
	if(val>max_di45) max_di45=val;
	
	val=di90[ind]=fabs(i_ch[iu-1]+2*i_ch[iu]+i_ch[iu+1]
			   -i_ch[id-1]-2*i_ch[id]-i_ch[id+1]);

	if(val>max_di90) max_di90=val;
	
	val=di135[ind]=fabs(2*i_ch[iu-1]+i_ch[iu]
			    +i_ch[ind-1]-i_ch[ind+1]
			    -i_ch[id]-2*i_ch[id+1]);
	if(val>max_di135) max_di135=val;


	ind++;
      }
    }

    // zero borders

    const int lastrow=(h-1)*w;

    for(int x=0;x<w;x++){
      int i=lastrow+x;
      ds0[x]=ds45[x]=ds90[x]=ds135[x]=0;
      di0[x]=di45[x]=di90[x]=di135[x]=0;
      ds0[i]=ds45[i]=ds90[i]=ds135[i]=0;
      di0[i]=di45[i]=di90[i]=di135[i]=0;
    }

    for(ind=w;ind<w*h;ind+=w){
      ds0[ind]=ds45[ind]=ds90[ind]=ds135[ind]=0;
      di0[ind]=di45[ind]=di90[ind]=di135[ind]=0;
      ds0[ind-1]=ds45[ind-1]=ds90[ind-1]=ds135[ind-1]=0;
      di0[ind-1]=di45[ind-1]=di90[ind-1]=di135[ind-1]=0;
    }

    // sum channels and take maximum

    float n0=(max_di0==0)?0:1.0/(max_di0+max_ds0);
    float n45=(max_di45==0)?0:1.0/(max_di45+max_ds45);
    float n90=(max_di90==0)?0:1.0/(max_di90+max_ds90);
    float n135=(max_di135==0)?0:1.0/(max_di135+max_ds135);

    for(ind=0;ind<w*h;ind++){
      ret->d[ind]= max(max(n0*(ds0[ind]+di0[ind]),
			   n45*(ds45[ind]+di45[ind])),
		       max(n90*(ds90[ind]+di90[ind]),
			   n135*(ds135[ind]+di135[ind])));
    }

    delete [] ds0;
    delete [] ds45;
    delete [] ds90;
    delete [] ds135;

    delete [] di0;
    delete [] di45;
    delete [] di90;
    delete [] di135;


    delete[] i_ch;
    delete[] s_ch;

    if(writefiles){
      cout << "writing edge detection results" << endl;

      picsom::imagedata img(w, h, 1,picsom::imagedata::pixeldata_float);
      for(int y=0;y<h;y++) for(int x=0;x<w;x++)
	img.set(x,y,ret->d[x+y*w]);
      picsom::imagefile::write(img, "edges.png");
    }

    setCurrentFrame(oldf);
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::preprocess_Sobel(int f, const std::string &){

    const int oldf=getCurrentFrame();

    setCurrentFrame(f);

    preprocessResult_Sobel *ret=new preprocessResult_Sobel;

    const int w=getWidth(),h=getHeight();

    const int l=w*h;

    ret->ds0=new float[l];
    ret->ds45=new float[l];
    ret->ds90=new float[l];
    ret->ds135=new float[l];
    
    ret->di0=new float[l];
    ret->di45=new float[l];
    ret->di90=new float[l];
    ret->di135=new float[l];

    float *i_ch=new float[l],*s_ch=new float[l];

    // find channels (S,I) of the HSI representation
    vector<float> vf;
    int ind=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vf = imageFrame(f)->get_float(x,y); 
      const float cmin=min(vf[0],min(vf[1],vf[2]));
      const float csum=vf[0]+vf[1]+vf[2];
      
      i_ch[ind]=csum/3;
      s_ch[ind]=csum==0?0:1-3*cmin/csum;
      ind++;
    }

    // form sobel magnitude images (neglecting border of width 1)

    int iu,id;

    for(int y=1;y<h-1;y++) {
      ind=y*w+1;
      for(int x=1;x<w-1;x++){

	iu=ind-w;
	id=ind+w;
	
	// s channel
	
	ret->ds0[ind]=fabs(-s_ch[iu-1]+s_ch[iu+1]
			  -2*s_ch[ind-1]+2*s_ch[ind+1]
			  -s_ch[id-1]+s_ch[id+1]);
	
	ret->ds45[ind]=fabs(s_ch[iu]+2*s_ch[iu+1]
			   -s_ch[ind-1]+s_ch[ind+1]
			   -2*s_ch[id-1]-s_ch[id]);
	
	ret->ds90[ind]=fabs(s_ch[iu-1]+2*s_ch[iu]+s_ch[iu+1]
			   -s_ch[id-1]-2*s_ch[id]-s_ch[id+1]);

	
	ret->ds135[ind]=fabs(2*s_ch[iu-1]+s_ch[iu]
			    +s_ch[ind-1]-s_ch[ind+1]
			    -s_ch[id]-2*s_ch[id+1]);

	// i channel
	
	ret->di0[ind]=fabs(-i_ch[iu-1]+i_ch[iu+1]
			  -2*i_ch[ind-1]+2*i_ch[ind+1]
			  -i_ch[id-1]+i_ch[id+1]);
	
	ret->di45[ind]=fabs(i_ch[iu]+2*i_ch[iu+1]
			   -i_ch[ind-1]+i_ch[ind+1]
			   -2*i_ch[id-1]-i_ch[id]);
	
	ret->di90[ind]=fabs(i_ch[iu-1]+2*i_ch[iu]+i_ch[iu+1]
			   -i_ch[id-1]-2*i_ch[id]-i_ch[id+1]);

	ret->di135[ind]=fabs(2*i_ch[iu-1]+i_ch[iu]
			    +i_ch[ind-1]-i_ch[ind+1]
			    -i_ch[id]-2*i_ch[id+1]);

	ind++;
      }
    }

    // zero borders

    const int lastrow=(h-1)*w;

    for(int x=0;x<w;x++){
      int i=lastrow+x;
      ret->ds0[x]=ret->ds45[x]=ret->ds90[x]=ret->ds135[x]=0;
      ret->di0[x]=ret->di45[x]=ret->di90[x]=ret->di135[x]=0;
      ret->ds0[i]=ret->ds45[i]=ret->ds90[i]=ret->ds135[i]=0;
      ret->di0[i]=ret->di45[i]=ret->di90[i]=ret->di135[i]=0;
    }

    for(ind=w;ind<w*h;ind+=w){
      ret->ds0[ind]=ret->ds45[ind]=ret->ds90[ind]=ret->ds135[ind]=0;
      ret->di0[ind]=ret->di45[ind]=ret->di90[ind]=ret->di135[ind]=0;
      ret->ds0[ind-1]=ret->ds45[ind-1]=ret->ds90[ind-1]=ret->ds135[ind-1]=0;
      ret->di0[ind-1]=ret->di45[ind-1]=ret->di90[ind-1]=ret->di135[ind-1]=0;
    }

    delete[] i_ch;
    delete[] s_ch;

    setCurrentFrame(oldf);
    return ret;
 
  }

 ///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::preprocess_Cielab(int f, const std::string &){

    const int oldf=getCurrentFrame();

    setCurrentFrame(f);

    preprocessResult_Cielab *ret=new preprocessResult_Cielab;

    const int w=getWidth(),h=getHeight();

    const int l=w*h;

    ret->l=new float[l];
    ret->a=new float[l];
    ret->b=new float[l];

    vector<float> vf;
    int ind=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vf = imageFrame(f)->get_float(x,y); 
      imagedata::rgb_to_cielab(vf[0],vf[1],vf[2]);
      ret->l[ind]=vf[0];
      ret->a[ind]=vf[1];
      ret->b[ind]=vf[2];
      ind++;
    }

    setCurrentFrame(oldf);
    return ret;
  }

///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::preprocess_Cieluv(int f, const std::string &){

    const int oldf=getCurrentFrame();

    setCurrentFrame(f);

    preprocessResult_Cieluv *ret=new preprocessResult_Cieluv;

    const int w=getWidth(),h=getHeight();

    const int l=w*h;

    ret->l=new float[l];
    ret->u=new float[l];
    ret->v=new float[l];

    vector<float> vf;
    int ind=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vf = imageFrame(f)->get_float(x,y); 
      imagedata::rgb_to_cieluv(vf[0],vf[1],vf[2]);
      ret->l[ind]=vf[0];
      ret->u[ind]=vf[1];
      ret->v[ind]=vf[2];
      ind++;
    }

    setCurrentFrame(oldf);
    return ret;
  }
///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::preprocess_HueMinMax(int f, const std::string &){

    const int oldf=getCurrentFrame();

    setCurrentFrame(f);

    preprocessResult_HueMinMax *ret=new preprocessResult_HueMinMax;

    const int w=getWidth(),h=getHeight();

    const int l=w*h;

    ret->hue=new float[l];
    ret->min=new float[l];
    ret->max=new float[l];

    vector<float> vf;
    int ind=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vf = imageFrame(f)->get_float(x,y); 
      imagedata::rgb_to_hueminmax(vf[0],vf[1],vf[2]);
      ret->hue[ind]=vf[0];
      ret->min[ind]=vf[1];
      ret->max[ind]=vf[2];
      ind++;
    }

    setCurrentFrame(oldf);
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  preprocessResult *segmentfile::preprocess_HueDiffSumQuant256(int f, const std::string &){

    const int oldf=getCurrentFrame();

    setCurrentFrame(f);

    preprocessResult_HueDiffSumQuant256 *ret=new preprocessResult_HueDiffSumQuant256;

    const int w=getWidth(),h=getHeight();

    const int l=w*h;

    ret->q=new int[l];

    vector<float> vf;
    int ind=0;

    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      vf = imageFrame(f)->get_float(x,y); 
      imagedata::rgb_to_huediffsum(vf[0],vf[1],vf[2]);
      ret->q[ind++]=imagedata::quantise_huediffsum_256(vf[0],vf[1],vf[2]);
    }

    setCurrentFrame(oldf);
    return ret;
  }


  ///////////////////////////////////////////////////////////////////////////
  pairingPredicate segmentfile::determinePairingPredicate(const string &p){
    pairingPredicate ret;

    if(p=="adjacent")
      determinePairingPredicateAdjacent(ret);
    else
      throw string("Unknown pairing predicate ")+p;
    
    return ret;

  }
  ///////////////////////////////////////////////////////////////////////////
  void segmentfile::determinePairingPredicateAdjacent(pairingPredicate &p){

    int x,y; 
    int w=getWidth(),h=getHeight();

    char str[80],str2[80];

    cout << "determining predicate \"adjacent\"..." << endl;

    for(x=0;x<w;x++) for(y=0;y<h;y++){

      int s,s2;

      get_pixel_segment(x,y,s);
      if(x+1<w){
	get_pixel_segment(x+1,y,s2);
	if(s!=s2){
	  sprintf(str,"%d",s);
	  sprintf(str2,"%d",s2);
	  p.p[str].insert(str2);
	  p.p[str2].insert(str);
	}
      }

      if(y+1<h){
	if(x-1>=0){
	  get_pixel_segment(x-1,y+1,s2);
	  if(s!=s2){
	    sprintf(str,"%d",s);
	    sprintf(str2,"%d",s2);
	    p.p[str].insert(str2);
	    p.p[str2].insert(str);
	  }
	}
	
	get_pixel_segment(x,y+1,s2);
	if(s!=s2){
	  sprintf(str,"%d",s);
	  sprintf(str2,"%d",s2);
	  p.p[str].insert(str2);
	  p.p[str2].insert(str);
	}


	if(x+1<w){
	  get_pixel_segment(x+1,y+1,s2);
	  if(s!=s2){
	    sprintf(str,"%d",s);
	    sprintf(str2,"%d",s2);
	    p.p[str].insert(str2);
	    p.p[str2].insert(str);
	  }
	}

      }
    }

    cout << "...done" << endl;

  }
  

  ///////////////////////////////////////////////////////////////////////////
  void segmentfile::_dump_xml(){
    char*buffer=description.outputXML();
    cout << buffer << endl;
    free(buffer);
  }
  
  ///////////////////////////////////////////////////////////////////////////
 _toc_type segmentfile::_read_toc(xmlNodePtr node){
    string errhead("segmentfile::_read_toc : ");
    
    if(XMLTools::strcmpXML(node->name,"tifftoc"))
      throw errhead + "node is not a <tifftoc>";
    
    _toc_type ret;
    
    xmlNodePtr n;
    int imgcnt=0,segcnt=0;
    for(n=XMLTools::xmlNodeFindDescendant(node,"tocframe");
	n; n=XMLTools::xmlNodeFindNextSibling(n,"tocframe")){
      _toc_entry tmp;
      tmp.sequence_name = XMLTools::xmlNodeGetContent(XMLTools::xmlNodeFindDescendant(n,"sequencename"));
      tmp.tiff_framenr = XMLTools::xmlNodeGetInt(XMLTools::xmlNodeFindDescendant(n,"tiff_framenr"));
      tmp.tiff_msframenr = XMLTools::xmlNodeGetInt(XMLTools::xmlNodeFindDescendant(n,"tiff_msframenr"));
      tmp.sequence_framenr = XMLTools::xmlNodeGetInt(XMLTools::xmlNodeFindDescendant(n,"sequence_framenr"));
      if(tmp.sequence_name=="image") imgcnt++;
      else if(tmp.sequence_name=="segment") segcnt++;
      ret.push_back(tmp);
    }
    
    return ret;
    
  }

 ///////////////////////////////////////////////////////////////////////////
 _toc_type segmentfile::prepareTOC(bool includeImages, 
					bool includeStoredResults,
				   bool /*touchFrames*/) {


    // this fcn creates toc
    // that is read by WriteAllSegmentFrames() that writes just those frames
    // specified in the toc 

    // further, lets change the return type of this fcn to _toc_type
    // that can be more easily read by WriteAllSegmentFrames() 

    // xmlNodePtr ret = xmlNewNode(NULL,(xmlChar*)"tifftoc");


   _toc_type ret;
    
    size_t f;
    _toc_entry e;

    e.tiff_framenr=0;
    e.sequence_name="segment";
    
    for(f=0;f<segment_frames.frames.size();f++){
      
      // write only such frames that are not empty

      if(segment_frames.frames[f].data == NULL && 
	  segment_frames.frames[f].frameSrc!=file)
	continue;
      if(segment_frames.frames[f].data && 
	 segment_frames.frames[f].data->iszero()) 
	continue;

      e.sequence_framenr=f;
      //      e.width=(touchFrames)?((int)segmentFrame(f)->width()):-1;
      //      e.height=(touchFrames)?((int)segmentFrame(f)->height()):-1;
      if(segment_frames.frames[f].doesFrameNeed32Bits()){
	e.tiff_msframenr = e.tiff_framenr+1;
	ret.push_back(e);
	e.tiff_framenr+=2;
      } else {
	e.tiff_msframenr = -1;
	ret.push_back(e);
	e.tiff_framenr++;
      }
    }

    if(includeImages){

      // write all image frames if images are included
      e.sequence_name="image";
      //e.width=(touchFrames)?((int)imageFrame(f)->width()):-1;
      //e.height=(touchFrames)?((int)imageFrame(f)->height()):-1;
      
      for(f=0;f<image_frames.frames.size();f++){
	e.sequence_framenr=f;
	e.tiff_msframenr = -1;
	ret.push_back(e);
	e.tiff_framenr++;
      }
    }
    
    if(includeStoredResults){
      map<int,_frame_type>::const_iterator it;

      e.sequence_name="stored_segment";      
      for(it=stored_segments.begin(); it != stored_segments.end(); it++){
      
      e.sequence_framenr=f;
      if(it->second.doesFrameNeed32Bits()){
	e.tiff_msframenr = e.tiff_framenr+1;
	ret.push_back(e);
	e.tiff_framenr+=2;
      } else {
	e.tiff_msframenr = -1;
	ret.push_back(e);
	e.tiff_framenr++;
      }
      }


      e.sequence_name="stored_image";      
      for(it=stored_segments.begin(); it != stored_segments.end(); it++){
	
	e.sequence_framenr=f;
	e.tiff_msframenr = -1;
	ret.push_back(e);
	e.tiff_framenr++;
      }
    }
    return ret;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::_add_toc_entry_to_xml(xmlNodePtr n,const _toc_entry &e)
  {
    char str[15];

    xmlNodePtr f=xmlNewTextChild(n,NULL,(xmlChar*)"tocframe",NULL);

    sprintf(str,"%d",e.tiff_framenr);
    xmlNewTextChild(f,NULL,(xmlChar*)"tiff_framenr",(xmlChar*)str);
    sprintf(str,"%d",e.tiff_msframenr);
    xmlNewTextChild(f,NULL,(xmlChar*)"tiff_msframenr",(xmlChar*)str);
    xmlNewTextChild(f,NULL,(xmlChar*)"sequencename",(xmlChar*)e.sequence_name.c_str());
    sprintf(str,"%d",e.sequence_framenr);
     xmlNewTextChild(f,NULL,(xmlChar*)"sequence_framenr",(xmlChar*)str);
//  sprintf(str,"%d",e.width);
//     xmlNewTextChild(f,NULL,(xmlChar*)"width",(xmlChar*)str);
//  sprintf(str,"%d",e.height);
//     xmlNewTextChild(f,NULL,(xmlChar*)"height",(xmlChar*)str);

  }

  ///////////////////////////////////////////////////////////////////////////
  void segmentfile::setXMLTOC(xmlNodePtr n){
    if(description.tifftoc)
      xmlFreeNode(description.tifftoc);
    description.tifftoc=xmlCopyNode(n,1);
    description.tifftoc->ns=NULL;
    description.tifftoc->nsDef=NULL;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::checkXMLVersion(const xmlDocPtr doc) {
    string errhead = name_head() + "checkXMLVersion() : ";
    xmlNodePtr segm = XMLTools::xmlDocGetSegmentation(doc);

    xmlNsPtr defns = xmlSearchNs(doc,segm,NULL);

    if (!defns)
      throw errhead + "default namespace not found";

    string url((char*)defns->href);
    string prefix;

    namespace_spec::version v;
    namespace_spec::split_url(url,prefix,v);

    if (namespace_spec::segmentation_url() != prefix)
      throw errhead + "xml namespace different from <"+
	namespace_spec::segmentation_url()+">";

    if (v.major != namespace_spec::required_version().major ||
	v.minor !=  namespace_spec::required_version().minor) {
      stringstream ss;
      ss << "required=" << namespace_spec::required_version().major
	 << "." << namespace_spec::required_version().minor
	 << " present=" << v.major << "." << v.minor;
      throw errhead + "incompatible xml versions: "+ss.str();
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  xmlNodePtr MethodInfo::outputXML() const {

    xmlNodePtr methodinfonode=xmlNewNode(NULL,(xmlChar*)"methodinfo");

    xmlNewTextChild(methodinfonode,NULL,(xmlChar*)"abbreviation",
		    (xmlChar*)(abbreviation.c_str()));
    xmlNewTextChild(methodinfonode,NULL,(xmlChar*)"fullname",
		    (xmlChar*)(fullname.c_str()));
    xmlNewTextChild(methodinfonode,NULL,(xmlChar*)"description",
		    (xmlChar*)(description.c_str()));
    xmlNewTextChild(methodinfonode,NULL,(xmlChar*)"version",
		    (xmlChar*)(version.c_str()));
    xmlAddChild(methodinfonode,options.produceXML());
    
    return methodinfonode;
  } 

  ///////////////////////////////////////////////////////////////////////////

  bool MethodInfo::parseXML(const xmlNodePtr node) {
    if (XMLTools::strcmpXML(node->name, "methodinfo"))
      return false;

    xmlNodePtr field=XMLTools::xmlNodeFindDescendant(node,"abbreviation");
    if(!field) return false;
    abbreviation = XMLTools::xmlNodeGetContent(field);

    field=XMLTools::xmlNodeFindDescendant(node,"fullname");
    if(!field) return false;
    fullname = XMLTools::xmlNodeGetContent(field);

    field=XMLTools::xmlNodeFindDescendant(node,"description");
    if(!field) return false;
    description = XMLTools::xmlNodeGetContent(field);

    field=XMLTools::xmlNodeFindDescendant(node,"version");
    if(!field) return false;
    version = XMLTools::xmlNodeGetContent(field);
    
    field=XMLTools::xmlNodeFindDescendant(node,"optionlist");
    return options.parseXML(field);
  }

  ///////////////////////////////////////////////////////////////////////////
  xmlNodePtr MethodList::outputXML() const {

    xmlNodePtr list=xmlNewNode(NULL,(xmlChar*)"methodinfolist");

    for(size_t i=0;i<methodInfo.size();i++)
	xmlAddChild(list,methodInfo[i].outputXML());

    return list;
  }

  ///////////////////////////////////////////////////////////////////////////
  bool MethodList::parseXML(const xmlNodePtr node){



    if (XMLTools::strcmpXML(node->name, "methodinfolist")){
      return false;
    }

    for(xmlNodePtr infonode
	  =XMLTools::xmlNodeFindDescendant(node,"methodinfo");
	infonode; 
	infonode=XMLTools::xmlNodeFindNextSibling(infonode,"methodinfo")){
      MethodInfo tmpinfo;
      if(!tmpinfo.parseXML(infonode)) return false;
      
      // int idx=methodInfo.size();
      methodInfo.push_back(tmpinfo);
      methodPointers.push_back(NULL);
    }

    return true;

  }
  ///////////////////////////////////////////////////////////////////////////

  void MethodList::appendList(const MethodList &other){

    size_t oc=other.count();

    for(size_t i=0;i<oc;i++){
      appendSingleMethod(other.methodPointers[i]);
    }

  }
  ///////////////////////////////////////////////////////////////////////////

  void MethodList::appendChain(const ChainedMethod *first){

    for(const ChainedMethod *f=first;f!=NULL;f=f->getNextMethod())
      appendSingleMethod(f);
  }
  ///////////////////////////////////////////////////////////////////////////

  void MethodList::appendSingleMethod(const ChainedMethod *method){

    int idx=methodInfo.size();
    methodInfo.push_back(method->getMethodInfo());
    methodPointers.push_back(method);
    if(pointer2idx.count(method))
      throw string("MethodList::appendSingleMethod(): method already in list");
    pointer2idx[method]=idx;

  }

  ///////////////////////////////////////////////////////////////////////////

  string MethodList::collectChainedName() const{

    // form a '+' separated string of abbreviations of the methods

    string ret;

    string errhead("MethodList::collectChainedName() : ");

    for(size_t i=0;i<methodInfo.size();i++){
      if(!ret.empty()) ret += '+';
      ret += methodInfo[i].abbreviation;
    }

    return ret;
  }
  ///////////////////////////////////////////////////////////////////////////
  xmlNodePtr FrameSizeInfo::outputXML() const{
    xmlNodePtr node=xmlNewNode(NULL,(xmlChar*)"framesizeinfo");

    char str[80];

    sprintf(str,"%d",firstframe);
    xmlNewTextChild(node,NULL,(xmlChar*)"firstframe",(xmlChar*)str);

    sprintf(str,"%d",lastframe);
    xmlNewTextChild(node,NULL,(xmlChar*)"lastframe",(xmlChar*)str);

    sprintf(str,"%d",width);
    xmlNewTextChild(node,NULL,(xmlChar*)"width",(xmlChar*)str);

    sprintf(str,"%d",height);
    xmlNewTextChild(node,NULL,(xmlChar*)"height",(xmlChar*)str);
    
    return node;
  }
  ///////////////////////////////////////////////////////////////////////////
  bool FrameSizeInfo::parseXML(const xmlNodePtr node){
    if (XMLTools::strcmpXML(node->name, "framesizeinfo"))
      return false;

    xmlNodePtr field=XMLTools::xmlNodeFindDescendant(node,"firstframe");
    if(!field) return false;
    firstframe = XMLTools::xmlNodeGetInt(field);

    field=XMLTools::xmlNodeFindDescendant(node,"lastframe");
    if(!field) return false;
    lastframe = XMLTools::xmlNodeGetInt(field);

    field=XMLTools::xmlNodeFindDescendant(node,"width");
    if(!field) return false;
    width = XMLTools::xmlNodeGetInt(field);

    field=XMLTools::xmlNodeFindDescendant(node,"height");
    if(!field) return false;
    height = XMLTools::xmlNodeGetInt(field);

    return true;

  }
  
  ///////////////////////////////////////////////////////////////////////////

  coord SequenceInfo::frameSize(int f) const{
    coord ret(-1,-1);
    if (frameNrOk(f)) {
      for (size_t i=0; i<sizeinfo.size(); i++) {
	if (sizeinfo[i].firstframe<=f && sizeinfo[i].lastframe>=f) {
	  ret.x = sizeinfo[i].width;
	  ret.y = sizeinfo[i].height;
	  return ret;
	}
      }
    }
      
    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////

  void SequenceInfo::setFromImagefile(imagefile &i) {
    lastframe=i.nframes()-1;

    // assume all the frames to be of the same size

    FrameSizeInfo tmp;


    tmp.firstframe=0;
    tmp.lastframe=lastframe;
    if(lastframe>=0){
      imagedata &img=i.frame(0);

      tmp.width=img.width();
      tmp.height=img.height();
    }
    else{
      lastframe=tmp.lastframe=0;
      tmp.width=1;
      tmp.height=1;
    }

    sizeinfo.clear();
    sizeinfo.push_back(tmp);
  }

  ///////////////////////////////////////////////////////////////////////////

  void SequenceInfo::setFromImagedata(const imagedata& img) {
    lastframe = 0;
    FrameSizeInfo tmp;
    tmp.firstframe = 0;
    tmp.lastframe = lastframe;
    tmp.width  = img.width();
    tmp.height = img.height();
    sizeinfo.clear();
    sizeinfo.push_back(tmp);
  }

  ///////////////////////////////////////////////////////////////////////////

  bool SequenceInfo::isCompatible(const SequenceInfo &o) const {
    if(lastframe != o.lastframe) return false;

    for(size_t i=0;i<sizeinfo.size();i++){
      const FrameSizeInfo &ii=sizeinfo[i];
      coord os=o.frameSize(ii.firstframe);
      if(os.x != ii.width || os.y != ii.height) return false;

      os=o.frameSize(ii.lastframe);
      if(os.x != ii.width || os.y != ii.height) return false;

    }

    for(size_t i=0;i<o.sizeinfo.size();i++){
      const FrameSizeInfo &ii=o.sizeinfo[i];
      coord os=frameSize(ii.firstframe);
      if(os.x != ii.width || os.y != ii.height) return false;

      os=frameSize(ii.lastframe);
      if(os.x != ii.width || os.y != ii.height) return false;
    }

    return true;

  }
  
  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr SequenceInfo::outputXML() const {
    xmlNodePtr node=xmlNewNode(NULL,(xmlChar*)"sequenceinfo");

    char str[80];
    sprintf(str,"%d",lastframe);
    xmlNewTextChild(node,NULL,(xmlChar*)"lastframe",(xmlChar*)str);
    
    xmlNodePtr list=xmlNewTextChild(node,NULL,(xmlChar*)"framesizeinfolist",
				    NULL);
    
    for(size_t i=0;i<sizeinfo.size();i++)
      xmlAddChild(list,sizeinfo[i].outputXML());
    
    return node;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  bool SequenceInfo::parseXML(const xmlNodePtr node) {
    if (XMLTools::strcmpXML(node->name, "sequenceinfo"))
      return false;

    xmlNodePtr field=XMLTools::xmlNodeFindDescendant(node,"lastframe");
    if(!field) return false;
    lastframe = XMLTools::xmlNodeGetInt(field);

    sizeinfo.clear();

    for(xmlNodePtr infonode=XMLTools::
	  xmlNodeFindDescendant(node,"framesizeinfolist->framesizeinfo");
	infonode; infonode=XMLTools::
	  xmlNodeFindNextSibling(infonode,"framesizeinfo")){
      FrameSizeInfo tmpinfo;
      if(!tmpinfo.parseXML(infonode)) return false;
      
      sizeinfo.push_back(tmpinfo);
    }

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  int segmentfile::resolve_sec(int x, int y, int w, int h) {
    double x1 = x, y1 = y, x0 = w/2.0, y0 = h/2.0;
    double rmax = w*h/(5.0*3.1415926);
    
    if ((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0) < rmax)
      return 2;
    
    bool cond1 = x1*h>y1*w, cond2 = (w-x1)*h>y1*w;

    return cond1 ? (cond2 ? 0 : 3) : (cond2 ? 1 : 4);
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningTheOriginalOne(int w, int h) {
    // const int w = getWidth(), h = getHeight();
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++)
	set_pixel_segment(x, y, resolve_sec(x, y, w, h));
    
    return true;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningCenterHorizVert(int w, int h) {
    // const int w = getWidth(), h = getHeight();
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++) {
	int s = resolve_sec(x, y, w, h);
	if (s!=2) {
	  s = 2*!(x<w/2.0)+!(y<h/2.0);
	  if (s>=2)
	    s++;
	}
	set_pixel_segment(x, y, s);
      }
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningHorizVert(int w, int h) {
    // const int w = getWidth(), h = getHeight();
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++) {
	int s = 2*!(x<w/2.0)+!(y<h/2.0);
	set_pixel_segment(x, y, s);
      }
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningHoriz(int w, int h) {
    // const int w = getWidth(), h = getHeight();
    int xm = w/2;
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++)
	set_pixel_segment(x, y, x>=xm);
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningVert(int w, int h) {
    // const int w = getWidth(), h = getHeight();
    int ym = h/2;
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++)
	set_pixel_segment(x, y, y>=ym);
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningCenter(int w, int h) {
    // const int w = getWidth(), h = getHeight();
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++)
	set_pixel_segment(x, y, (resolve_sec(x, y, w, h)==2));
    
    return true;
  }
  
  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningNone(int w, int h) {
    // const int w = getWidth(), h = getHeight();
    for (int y=0; y<h; y++)
      for (int x=0; x<w; x++)
	set_pixel_segment(x, y, 0);
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningTiles(int w, int h,
				int xlen, int ylen, int dx, int dy) {
    int ntile = 0; // h = getHeight(), w = getWidth();

    for (int y=0; y+dy<=h; y += dy)
      for (int x=0; x+dx<=w; x +=dx) {
	for (int yy=y; yy<y+ylen && yy < h; yy++)
	  for (int xx=x; xx<x+xlen&& xx < w; xx++)
	    set_pixel_segment(xx, yy, ntile);
	ntile++;
      }
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::zoningTiles_old(int tilesX, int tilesY){
    
    int ntile=0;
    int h=getHeight();
    int w=getWidth();

    int xtilelength = (int) ceil(((float)w)/(float)tilesX);
    int ytilelength = (int) ceil(((float)h)/(float)tilesY);

    for (int y=0; y<h; y += ytilelength)
      for (int x=0; x<w; x +=xtilelength) {
	for (int yy=y; yy<y+ytilelength && yy < h; yy++)
	  for (int xx=x; xx<x+xtilelength&& xx < w; xx++)
	    set_pixel_segment(xx,yy,ntile);
	ntile++;
      }
    
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void segmentfile::parseRegionsFromXML() {
    // cout << "in parseRegionsFromXML" << endl;

    for(size_t i=0;i<region_spec.size();i++)
      delete region_spec[i];
    region_spec.clear();
    
    SegmentationResultList *l=
      readFileResultsFromXML("","region");
    SegmentationResultList::const_iterator it;
    for(it=l->begin();it!=l->end();it++){
      region_spec.push_back(Region::createRegion(it->value));
      // cout << "region " << it->value << " found as file result" << endl; 
    }

    delete l;

    l=readFrameResultsFromXML("","region");
    for(it=l->begin();it!=l->end();it++){
      region_spec.push_back(Region::createRegion(it->value));
      // cout << "region " << it->value << " found as frame result" << endl; 
    }

    delete l;

    cout << "parsed "<<region_spec.size()<<" regions from XML" << endl;
  }

  ///////////////////////////////////////////////////////////////

  std::string segmentfile::_find_prefix(const std::string& s) {
    static char digits[]="0123456789";
    
    size_t prefixlast=s.find_last_not_of(digits);
    
    return (prefixlast==string::npos) ? string() : s.substr(0,prefixlast+1); 
    
  }

  ///////////////////////////////////////////////////////////////////////////
  void segmentfile::initXMLSkeleton(){

    description.reset();

    // unnecessary copying follows


    xmlNodePtr tocnode=xmlNewNode(NULL,(xmlChar*)"tifftoc");
    setXMLTOC(tocnode);
    xmlFreeNode(tocnode);

    description.refreshFileInfo(fileInfoProvider);
    description.augmentProcessInfo(processInfoProvider);
    description.methods.appendChain(first);
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::getStdColouring(int ncols, vector<RGBTriple> &colouring){

    int l = (int)ceil(pow((double)ncols,(double)0.33333333));

    int i1,i2,i3,i;
    
    // find colours on a regular grid on RGB cube 
    float unit =1.0/l;
    
    RGBTriple *cols = new RGBTriple[ncols];
    
    for(i=i1=i2=i3=0;i<ncols;i++)
      {
	cols[i].r((i1+0.5)*unit);
	cols[i].g((i2+0.5)*unit);
	cols[i].b((i3+0.5)*unit);
	
	if(++i1==l){
	  i1=0;
	  if(++i2==l){
	    i2=0;
	    i3++;
	  }
	}
      }
    
    // now create new permutation
    
    //  cout << "RAND_MAX=" << RAND_MAX << " RAND_MAX+1.0=" << RAND_MAX+1.0 << endl;
    
    //colouring.push_back(RGBTriple(0,0,0)); // background always black
    // background not included

    SimpleRandom sr;

    for(i=0;i<ncols;i++){
      int ind=(int)((ncols-i)*(sr.rand()/(sr.randMax()+1.0)));
      colouring.push_back(cols[ind]);
      cols[ind]=cols[ncols-i-1];
    }

    delete [] cols;

  }

  ///////////////////////////////////////////////////////////////////////////

  map<int,float> *segmentfile::expandLabeling(const vector<int> &labels, 
					      const vector<float> &values, 
					      float std_value) {
    
    if (labels.size() != values.size())
      throw string("segmentfile::expandLabeling : "
		   "Vector size mismatch in segmentfile::expandLabeling");
    
    vector<int> *l = getUsedLabels();
    
    map<int,float> *result = new map<int,float>;
    size_t i;
    
    for(i=0;i<l->size();i++)
      (*result)[(*l)[i]]=std_value;
    
    for(i=0;i<labels.size();i++){
      if(labels[i]==-1){
	// all nonzero labels
	size_t j;
	for(j=1;j<l->size();j++)
	  if((*l)[j])
	    (*result)[(*l)[j]]=values[i];
	
      }
      else
	(*result)[labels[i]]=values[i];
    }
    
    delete l;
    
    return result;
    
  }

  ///////////////////////////////////////////////////////////////////////////

  bool segmentfile::expandLabeling(vector<int> &labels){
    // this function scans for special entry "-1" in the label list and 
    // replaces it with all the used nonzero labels

    bool append_nonzero=false;

    vector<int> copy(labels);
    labels.clear();

    for(size_t i=0;i<copy.size();i++){
      if(copy[i]==-1){
	vector<int> *u = getUsedLabels();
	for(size_t j=0;j<u->size();j++)
	  labels.push_back((*u)[j]);
	delete u; 
	append_nonzero=true;
      }
      else
	{
	labels.push_back(copy[i]);
	}
    }
    
    return append_nonzero;
  }

  ///////////////////////////////////////////////////////////////////////////
  map<int,RGBTriple>
  *segmentfile::expandColouring(const vector<int> &labels, 
				const vector<RGBTriple> &values, 
				RGBTriple std_value) {

    if(labels.size() != values.size()){
      cout << "Vector size mismatch in segmentfile::expandLabeling" << endl;
      return NULL;
    }

    //    cout << "in expandColouring(), colours are" << endl;
    //for(int c=0;c<values.size();c++)
    //  cout << "c="<<c<<" r="<<values[c].r()<<" g="<<values[c].g()<<" b="
    //	   << values[c].b() << endl;

    vector<int> *l=getUsedLabels();
    
    map<int,RGBTriple> *result = new map<int,RGBTriple>;
    size_t i;

    for(i=0;i<l->size();i++)
      (*result)[(*l)[i]]=std_value;
  
    for(i=0;i<labels.size();i++){
      if(labels[i]==-1){
	// all nonzero labels
	for(size_t j=1;j<l->size();j++)
	  if((*l)[j])
	    (*result)[(*l)[j]]=values[i];
      }
      else
	(*result)[labels[i]]=values[i];
    }

    return result;

  }

  ///////////////////////////////////////////////////////////////////////////
  string segmentfile::_intvector_to_string(const vector<int> &v){
    vector<int>::const_iterator it;
    char num[20];
    string ret;
    
    for(it=v.begin(); it!=v.end();it++){
      if(!ret.empty()) ret +=" ";
      sprintf(num,"%d",*it);
      ret += num;
    }

    return ret;
 
  }
  ///////////////////////////////////////////////////////////////////////////
  vector<int> segmentfile::_string_to_intvector(const string &s){
    vector<int> v;
    int i;
    bool fail=false;

    istringstream ss(s);
    while(!fail){
      ss >> i;
      fail=ss.fail();
      if(!fail)
	v.push_back(i);
    }

    return v;
  }

//   ///////////////////////////////////////////////////////////////////////////
//   void segmentfile::_split_sequence_spec(const string &spec,
// 					 string &name, int &nr){
//     // validity of the specification should be checked here
//     // at the moment it's assumed that the specification 
//     // is valid, i.e. "image_resultXXX[\[YY\]]" or 
//     // "segment_resultXXX[\[YY\]]" 
     
//     string errhead=name_head() + "_split_sequence_spec()";

//     string::size_type leftbracket = spec.find('[');

//     if(leftbracket == string::npos)
//       name=spec;
//     else{
//       name = spec.substr(0,leftbracket);
//       if(sscanf(spec.c_str()+leftbracket+1,"%d",&nr)!= 1)
// 	throw errhead + "couldn't parse frame number in brackets";
//     }
//   }

 
 ///////////////////////////////////////////////////////////////////////////  
 imagedata *segmentfile::_fetch_frame(_frame_type &f){


   //  cout << "in _fetch_frame(f="<<f<<" segment="<<segment<<")"<<endl;

   if(use_ondemand==false)
     throw string("segmentfile::_fetch_frame(): use_on_demand==false");

   if(f.data) return f.data;

   switch(f.frameSrc){
   case no_source:
     throw string("segmentfile::_fetch_frame(): src==no_source");
   case file:
     if(f.file == NULL)
       throw string("segmentfile::_fetch_frame(): src==file, f.file==NULL");

     if(f.segment)	
       f.file->default_pixel(imagedata::pixeldata_uint16);
     // frames are stored with 16 bits in the file
     else 
       f.file->default_pixel(imagedata::pixeldata_float);
    
    if(f.fileframe_ms < 0){
      f.data = &(f.file->frame(f.fileframe));
      f.owndata=false;
      if(f.segment)
	f.data->convert(_getSegmentframeDatatype());	
    } else{ // we combine two images
      vector<imagedata> v;
      v.push_back(input_segment_file->frame(f.fileframe));
      v.push_back(input_segment_file->frame(f.fileframe_ms));
      f.data = new imagedata(imagedata::combine_bits(v));
      f.file->remove_frame(f.fileframe);
      f.file->remove_frame(f.fileframe_ms);
      f.owndata = true;
    }
    if(f.segment){
      if(f.data->count() != 1) {
	f.data->force_one_channel();
      }
    }
    else{
      // cout << "forcing three data channels " << endl;
      f.data->force_three_channel();
    }
    break;

   case create: 

     if(f.createwidth<0 || f.createheight<0)
         throw string("segmentfile::_fetch_frame(): createwidth/height <0");

     if(f.segment)	
       f.data = new imagedata(f.createwidth,f.createheight,1,
			       _getSegmentframeDatatype());
     else
       f.data = new imagedata(f.createwidth,f.createheight,3);
     f.owndata=true;
     break;
   }

   return f.data;
 }

  ///////////////////////////////////////////////////////////////////////////  

  void segmentfile::_discard_frame(_frame_type& f) {
    if (use_ondemand==false) 
      throw string("ERROR: use_ondemand==false");

    if (f.owndata)
      delete f.data;

    f.data=NULL;

    if (f.file) {
      f.file->remove_frame(f.fileframe);
      if(f.fileframe_ms>=0)
	f.file->remove_frame(f.fileframe_ms);
    }
  }

  ///////////////////////////////////////////////////////////////////////////

  segmentfile::_description_type::~_description_type() {
  }

  ///////////////////////////////////////////////////////////////////////////

  char *segmentfile::_description_type::outputXML() const {
    char *buffer;
    _outputXML(NULL,&buffer);
    return buffer;
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::_description_type::outputXML(FILE *file) const {
    _outputXML(file, NULL);
  }

  ///////////////////////////////////////////////////////////////////////////

  void segmentfile::_description_type::_outputXML(FILE *file,
						  char **buffer) const {
    string errhead="segmentfile::_description_type::_outputXML(): ";

    bool set_per_sec_framespec = true;

    xmlDocPtr doc=xmlNewDoc((xmlChar*)"1.0");

    // form libxml xml tree representation doc of description data
    // output to a char array
    // then discard the xml tree

    // setup root element and namespace

    xmlNodePtr segm=xmlNewDocNode(doc,NULL,(xmlChar*)"segmentation",NULL);
    if(!xmlNewNs(segm,(xmlChar *)
		 namespace_spec::segmentation_current_version_url().c_str(),
		 NULL)) 
      throw errhead + "namespace creation failed"; 

    xmlDocSetRootElement(doc, segm);

    //sequenceinfo
    xmlNodePtr n=sequenceinfo.outputXML();
    xmlAddChild(segm,n);
 
    // tifftoc copied as such
    xmlNodePtr toc = (tifftoc!=NULL)?
      xmlCopyNode(tifftoc,1): xmlNewNode(NULL,(xmlChar*)"tifftoc");

    //toc->ns=NULL;
    toc->nsDef=NULL;

    xmlAddChild(segm,toc);

    // fileinfo
    n=fileinfo.outputXML();
    xmlAddChild(segm,n);

    // processinfolist
    n = xmlNewTextChild(segm,NULL,(xmlChar*)"processinfolist",NULL);

    for(vector<ProcessInfo>::const_iterator it=processinfolist.begin();
	it != processinfolist.end(); it++){
      xmlNodePtr nn=it->outputXML();
      xmlAddChild(n,nn);
    }

    // methodinfolist
    n=methods.outputXML();
    xmlAddChild(segm,n);

    // fileresults
    n = xmlNewTextChild(segm,NULL,(xmlChar*)"fileresults",NULL);
    xmlNodePtr rl=fileResults.outputXML();
    xmlAddChild(n,rl);

    map<int,string> framespecs;

    // frames
    n = xmlNewTextChild(segm,NULL,(xmlChar*)"framelist",NULL);
    size_t sec = 0;
    for(map<int,set<int> >::const_iterator it=processedFrames.begin(); 
	it != processedFrames.end();it++){

      if (it->second.size()==0)
	continue;
    
      size_t framestep_ = 24; // obs! hardcoded
      size_t fidx = *it->second.begin();
      if (processinfolist.size()>fidx) {
	framestep_ = processinfolist[fidx].framestep;
	// continue;
	// throw errhead+"processinfolist index error";
      }

      int framenr = it->first;
      xmlNodePtr newFrame=xmlNewTextChild(n,NULL,(xmlChar*)"frame",NULL);
      char framestr[80];
      sprintf(framestr,"%d",framenr);
      xmlNewTextChild(newFrame,NULL,(xmlChar*)"framenr",(xmlChar*)framestr);

      if (set_per_sec_framespec && it->second.size()==1 && framestep_>1) {
	// we assume here that framestep equals to frames per second...
	string framespec = ":s"+ToStr(sec++);
	framespecs[framenr] = framespec;
	xmlNewTextChild(newFrame, NULL, (xmlChar*)"framespec",
			(xmlChar*)framespec.c_str());
      }

      string list=formVirtualSegmentLabel(it->second,' ');
      xmlNewTextChild(newFrame,NULL,(xmlChar*)"processedby",
		      (const xmlChar*)list.c_str());

      if(frameResults.count(framenr)){
	xmlNodePtr rl=frameResults.find(framenr)->second.outputXML();
	xmlAddChild(newFrame,rl);
      }
    }

    // attributes
    n = xmlNewTextChild(segm,NULL,(xmlChar*)"imageattributeslist",NULL);
    sec = 0;
    for(map<string,_sequence_attributes_type>::const_iterator 
	  it=attributes.begin(); it != attributes.end(); it++){
      xmlNodePtr ia=xmlNewTextChild(n,NULL,(xmlChar*)"imageattributes",NULL);
      xmlNewTextChild(ia,NULL,(xmlChar*)"sequencename",
		      (xmlChar*)it->first.c_str());
    
      xmlNodePtr sa = 
	xmlNewTextChild(ia,NULL,(xmlChar*)"sequenceattributes",NULL);
    
      xmlNodePtr al=it->second.sequenceAttributes.outputXML();
      xmlAddChild(sa,al);

      xmlNodePtr falist = 
	xmlNewTextChild(ia,NULL,(xmlChar*)"frameattributeslist",NULL);

      for(map<int,_attribute_collection_type>::const_iterator 
	    fit=it->second.frameAttributes.begin(); 
	  fit != it->second.frameAttributes.end(); fit++){
	int framenr=fit->first;
	xmlNodePtr newFrame=xmlNewTextChild(falist,NULL,
					    (xmlChar*)"frameattributes",NULL);
	char framestr[80];
	sprintf(framestr,"%d",framenr);
	xmlNewTextChild(newFrame,NULL,(xmlChar*)"framenr",(xmlChar*)framestr);

	auto fspecp = framespecs.find(framenr);
	if (fspecp!=framespecs.end()) {
	  string framespec = fspecp->second;
	  xmlNewTextChild(newFrame, NULL, (xmlChar*)"framespec",
			  (xmlChar*)framespec.c_str());
	}

	xmlNodePtr al=fit->second.outputXML();
	xmlAddChild(newFrame,al);
      }
    }

    // xmlReconciliateNs(doc,segm);
    if(file){
      xmlDocDump(file,doc);
    }

    if(buffer){
      *buffer=XMLTools::XMLToString(doc,true);
    }

    xmlFreeDoc(doc);
  }

  ///////////////////////////////////////////////////////////////////////////

  char *segmentfile::_description_type::outputSingleFrameXML(int
							   /*f*/) const {
    throw string("outputSingleFrameXML() not yet implemented");
  }

///////////////////////////////////////////////////////////////////////////

  bool segmentfile::_description_type::readXML(const char *s){
    xmlDocPtr xml_description = xmlParseDoc((xmlChar *)s);

  if(!xml_description) return false;

  // read all information from XML to own data structures
  // then destroy the libxml representation
  checkXMLVersion(xml_description);

  xmlNodePtr seg=XMLTools::xmlDocGetSegmentation(xml_description);
  if(!seg) return false;

  xmlNodePtr n;

  if((n=XMLTools::xmlNodeFindDescendant(seg,"sequenceinfo"))==NULL)
    return false;
  
  sequenceinfo.parseXML(n);

  // tifftoc copied as such

  if(tifftoc)
    xmlFreeNode(tifftoc);

  n=XMLTools::xmlNodeFindDescendant(seg,"tifftoc");
  
  tifftoc=(n!=NULL)?xmlCopyNode(n,1) // recursive copy
    : NULL;

  // fileinfo

  if((n=XMLTools::xmlNodeFindDescendant(seg,"fileinfo"))==NULL)
    return false;
  
  fileinfo.parseXML(n);

  // processinfolist

  if((n=XMLTools::xmlNodeFindDescendant(seg,"processinfolist"))==NULL)
    return false;
  
  processinfolist.clear();

  for(xmlNodePtr nn=XMLTools::xmlNodeFindDescendant(n,"processinfo");
      nn!=NULL; nn=XMLTools::xmlNodeFindNextSibling(nn,"processinfo")){

    ProcessInfo tmp;
    tmp.parseXML(nn);
    processinfolist.push_back(tmp);
  }

  // methodinfolist

  if((n=XMLTools::xmlNodeFindDescendant(seg,"methodinfolist"))==NULL)
    return false;

  methods.reset();
  methods.parseXML(n);

  // fileresults

  fileResults.reset();

  xmlNodePtr rl=XMLTools::xmlNodeFindDescendant(seg,"fileresults->resultlist");
  if(!rl) return false;
  fileResults.parseXML(rl,true);

  // frameresults

  frameResults.clear();
  processedFrames.clear();

  if ((n = XMLTools::xmlNodeFindDescendant(seg,"framelist")))
    for (xmlNodePtr nn=XMLTools::xmlNodeFindDescendant(n, "frame");
	 nn!=NULL; nn=XMLTools::xmlNodeFindNextSibling(nn, "frame")) {

      xmlNodePtr nnn = XMLTools::xmlNodeFindDescendant(nn, "framenr");
      if (!nnn)
	return false;

      int framenr = XMLTools::xmlNodeGetInt(nnn);

      nnn = XMLTools::xmlNodeFindDescendant(nn, "processedby");
      if (!nnn)
	return false;

      processedFrames[framenr]=
	parseVirtualSegmentLabel(XMLTools::xmlNodeGetContent(nnn), ' ');

      nnn = XMLTools::xmlNodeFindDescendant(nn, "resultlist");
      if (nnn) {
	frameResults[framenr] = _result_collection_type();
	_result_collection_type& rc = frameResults[framenr];
	rc.parseXML(nnn, true);
      }

      nnn = XMLTools::xmlNodeFindDescendant(nn, "framespec");
      if (nnn)
	frameSpecs[framenr] = XMLTools::xmlNodeGetContent(nnn);
    }

  // attributes

  for(xmlNodePtr n=XMLTools::
	xmlNodeFindDescendant(seg,"imageattributeslist->imageattributes");
      n!=NULL; n=XMLTools::xmlNodeFindNextSibling(n,"imageattributes")){

    xmlNodePtr nn=XMLTools::xmlNodeFindDescendant(n,"sequencename");
    if(!nn) return false;
    string sequencename=XMLTools::xmlNodeGetContent(nn);
    
    // sequenceattributes    
    xmlNodePtr al=XMLTools::xmlNodeFindDescendant(n,"sequenceattributes->attributelist");
    attributes[sequencename].sequenceAttributes.parseXML(al);
      
    // frameattributes
    for(xmlNodePtr fn=XMLTools::
	  xmlNodeFindDescendant(n,"frameattributeslist->frameattributes");
	fn!=NULL; fn=XMLTools::xmlNodeFindNextSibling(fn,"frameattributes")){
      
      xmlNodePtr nnn=XMLTools::xmlNodeFindDescendant(fn,"framenr");
      if(!nnn) return false;
      int framenr=XMLTools::xmlNodeGetInt(nnn);
      
      al=XMLTools::xmlNodeFindDescendant(fn,"attributelist");
      attributes[sequencename].frameAttributes[framenr].parseXML(al);

      // framespec processing is not implemented here.  should it be?
    }
  }
  
  xmlFreeDoc(xml_description);

  return true;
  }

  ///////////////////////////////////////////////////////////////////////////

  void
  segmentfile::_description_type::augmentProcessInfo(const
						     ProcessInfoProvider *p) {
  if (p)
    processinfolist.push_back(p->getProcessInfo());
  }

  ///////////////////////////////////////////////////////////////////////////

  xmlNodePtr segmentfile::_description_type::
  _result_collection_type::addResult(const xmlNodePtr result,
				     bool check_resultid) {
    // store result in vector, update index

    pair<xmlNodePtr,SegmentationResult> tmppair;
    tmppair.first=xmlCopyNode(result,1);
    tmppair.first->nsDef=NULL;
    tmppair.first->ns=NULL;

    tmppair.second.parseXML(result);

    if (tmppair.second.type=="invalidation") {
      set<int> idxs=parseVirtualSegmentLabel(tmppair.second.value,',');
      invalidatedresults.insert(idxs.begin(),idxs.end());
    }

    int idx=resultnodes.size();

    if (check_resultid && idx!=tmppair.second.resultid)
      throw string("ERROR: Parsed and newly assigned resultid not the same.");
    //  else
    //  if(check_resultid) cout << "resultid check succesful" << endl;

    tmppair.second.resultid=idx;
    char idstr[80];
    sprintf(idstr,"%d",idx);
    //xmlNewProp(tmppair.first,(xmlChar*)"resultid",(xmlChar*)idstr);
    xmlSetProp(tmppair.first,(xmlChar*)"resultid",(xmlChar*)idstr);
    resultnodes.push_back(tmppair);

    name2idx.insert(pair<string,int>(tmppair.second.name,idx));
    prefix2idx.insert(pair<string,int>(_find_prefix(tmppair.second.name),idx));
    type2idx.insert(pair<string,int>(tmppair.second.type,idx));
    method2idx.insert(pair<int,int>(tmppair.second.methodid,idx));

    return tmppair.first;
   }

///////////////////////////////////////////////////////////////////////////
xmlNodePtr segmentfile::_description_type::
_result_collection_type::addResult(const SegmentationResult &result){

  /// for now, implemented in way that causes unnecessary copying

  xmlNodePtr resnode=result.toXML();
  xmlNodePtr ret=addResult(resnode);
  xmlFreeNode(resnode);
  return ret;
}


///////////////////////////////////////////////////////////////////////////

void segmentfile::_description_type::
_result_collection_type::freeXML(){

  for(vector<pair<xmlNodePtr,SegmentationResult> >::iterator 
	it=resultnodes.begin();
      it != resultnodes.end() ; it++){
    // xmlUnlinkNode(it->second); // shouldn't be needed
    xmlFreeNode(it->first);
  }
    
  resultnodes.clear();

}


///////////////////////////////////////////////////////////////////////////
xmlNodePtr segmentfile::_description_type::
_result_collection_type::outputXML() const{

  // outputs a resultlist

  xmlNodePtr rl=xmlNewNode(NULL,(xmlChar*)"resultlist");
  for(vector<pair<xmlNodePtr,SegmentationResult> >::const_iterator 
	it=resultnodes.begin(); it != resultnodes.end(); it++){
    xmlNodePtr r=xmlCopyNode(it->first,1);
    xmlAddChild(rl,r);
  }

  return rl;
  
}

  /////////////////////////////////////////////////////////////////////////

  void segmentfile::_description_type::
  _result_collection_type::parseXML(const xmlNodePtr n,bool check_resultid){
    for(xmlNodePtr nn=XMLTools::xmlNodeFindDescendant(n,"result");
	nn!=NULL; nn=XMLTools::xmlNodeFindNextSibling(nn,"result"))
      addResult(nn,check_resultid);
  }
  
  /////////////////////////////////////////////////////////////////////////

  SegmentationResultList 
  *segmentfile::_description_type::
  _result_collection_type::readResults(result_search_type searchtype,
				       const std::string& name,
				       const std::string& type,
				       int methodid, 
				       bool wildcard,
				       bool alsoinvalid) const {
    set<int> idxs;
    findmatchingresults(idxs, searchtype, name, type, methodid,
			wildcard, alsoinvalid);
    SegmentationResultList *ret = new SegmentationResultList;
    for (set<int>::iterator it = idxs.begin(); it != idxs.end(); it++)
      ret->push_back(resultnodes[*it].second);

    return ret;
  }

///////////////////////////////////////////////////////////////////////////
std::vector<xmlNodePtr> 
segmentfile::_description_type::
_result_collection_type::accessResultNodes(result_search_type searchtype,
					   const string& name,
					   const string& type,
					   int methodid,
					   bool wildcard,bool alsoinvalid){

  set<int> idxs;
  findmatchingresults(idxs,searchtype,name,type,methodid,wildcard,alsoinvalid);
  vector<xmlNodePtr> ret;
  for(set<int>::iterator it=idxs.begin(); it != idxs.end(); it++)
    ret.push_back(resultnodes[*it].first);
  
  return ret;
}
///////////////////////////////////////////////////////////////////////////
void segmentfile::_description_type::
_result_collection_type::invalidateResults(result_search_type searchtype,						  const std::string& name,
					   const std::string& type,
					   int methodid,
					   bool wildcard){

  bool alsoinvalid=true;

  set<int> idxs;
  findmatchingresults(idxs,searchtype,name,type,methodid,wildcard,alsoinvalid);

  // record the invalidation in list of results 

  SegmentationResult res;
  res.name="invalidation";
  res.type="invalidation";
  res.value=formVirtualSegmentLabel(idxs,',');
  res.methodid=methodid;
  
  addResult(res);
}

///////////////////////////////////////////////////////////////////////////

  void segmentfile::_description_type::
  _result_collection_type::findmatchingresults(std::set<int> &tgt,
					       result_search_type searchtype,
					       const std::string& name,
					       const std::string& type,
					       int methodid,
					       bool wildcard,
					       bool alsoinvalid) const {
    tgt.clear();

    bool matchname=!name.empty();
    bool matchtype=!type.empty();
    bool matchmethod=searchtype==latest_method;

    if (!matchname && !matchtype && !matchmethod) {
      // do not do any matching
      if (searchtype==last) {
	int i;
	for (i=(int)resultnodes.size()-1; i>=0 &&
	       invalidatedresults.count(i); i--) {}
	if (i>0)
	  tgt.insert(resultnodes.size()-1);

      } else {
	for (size_t i=0; i<resultnodes.size(); i++)
	  if (invalidatedresults.count(i)==0)
	    tgt.insert(i);
      }
      return;

    } else { // matchbysomething
      if (matchname) {
	pair<multimap<string,int>::const_iterator,
	  multimap<string,int>::const_iterator> range;  
	
	if (wildcard)
	  range = prefix2idx.equal_range(_find_prefix(name));
	else
	  range = name2idx.equal_range(name);
	
	for (multimap<string,int>::const_iterator it=range.first;
	    it != range.second; it++)
	  tgt.insert(it->second);
	
	if (matchmethod || matchtype) {
	  // filter by the other criteria
	  set<int>::iterator it = tgt.begin(), it_next;
	  while (it!=tgt.end())
	    if ((matchmethod && method2idx.count(methodid)==0)||
		(matchtype && type2idx.count(type)==0)){
	      it_next=it;
	      it_next++;
	      tgt.erase(it);
	      it=it_next;
	    } else it++;
	}

      } else if (matchmethod) {
	pair<multimap<int,int>::const_iterator,
	  multimap<int,int>::const_iterator>
	  range = method2idx.equal_range(methodid);

	for(multimap<int,int>::const_iterator it=range.first;
	    it != range.second; it++)
	  tgt.insert(it->second);
	
	if (matchtype) {
	  // filter by type
	  set<int>::iterator it=tgt.begin(), it_next;
	  while (it!=tgt.end())
	    if (type2idx.count(type)==0) {
	      it_next=it;
	      it_next++;
	      tgt.erase(it);
	      it=it_next;
	    } else it++;
	}

      } else {
	// do matching on type only
      
	pair<multimap<string,int>::const_iterator,
	  multimap<string,int>::const_iterator>
	  range = type2idx.equal_range(type);
	
	for (multimap<string,int>::const_iterator it=range.first;
	     it != range.second; it++)
	  tgt.insert(it->second);

	//	for(multimap<string,int>::const_iterator it=type2idx.begin();
	//    it != type2idx.end(); it++) 
	//  cout << it->first << " -> " << it->second << endl;

      }
    } // matchbysomething
      
    // filter out invalidated results if desired

    if (!alsoinvalid) {
      set<int>::iterator it=tgt.begin(), it_next;
      while (it!=tgt.end())
	if (invalidatedresults.count(*it)) {
	  it_next=it; 
	  it_next++;
	  tgt.erase(it);
	  it=it_next;
	} else it++;
    }

    if (searchtype==last && tgt.size()) {
      // get the single last result
      int i = *tgt.rbegin();
      tgt.clear();
      tgt.insert(i);
    }
  }

///////////////////////////////////////////////////////////////////////////
xmlNodePtr segmentfile::_description_type::
_attribute_collection_type::outputXML() const{

  // outputs a attributelist

  xmlNodePtr al=xmlNewNode(NULL,(xmlChar*)"attributelist");
    
  for(multimap<string,int>::const_iterator it=key2idx.begin(); 
      it != key2idx.end(); it++){
    SegmentationAttribute tmpattr;
    tmpattr.key=it->first; tmpattr.value=values[it->second];
    xmlNodePtr a=tmpattr.toXML();
    xmlAddChild(al,a);
  }

  return al;
  
}

  /////////////////////////////////////////////////////////////////////////

  void segmentfile::_description_type::
  _attribute_collection_type::parseXML(const xmlNodePtr n) {
    reset();
    for(xmlNodePtr nn=XMLTools::xmlNodeFindDescendant(n,"attribute");
	nn!=NULL; nn=XMLTools::xmlNodeFindNextSibling(nn,"attribute")){
      SegmentationAttribute tmpattr;
      tmpattr.parseXML(nn);
      addAttribute(tmpattr.key,tmpattr.value);
    }
  }

///////////////////////////////////////////////////////////////////////////
std::string segmentfile::_description_type::
_attribute_collection_type::getLastAttribute(const std::string &key) const
{
  if(key.empty())
    return values[values.size()-1];

  pair<multimap<string,int>::const_iterator,multimap<string,int>::const_iterator> range=key2idx.equal_range(key);

  // to sort by index, insert to a set

  set<int> s;

  for(multimap<string,int>::const_iterator it=range.first;
      it!=range.second; it++)
    s.insert(it->second);

  if(s.size()==0) return "";

  return values[*s.rbegin()];
}

///////////////////////////////////////////////////////////////////////////	
std::multimap<std::string,std::string>
segmentfile::_description_type::
_attribute_collection_type::getAllAttributes(const std::string &key) const{

  multimap<string,string> ret;
  
  pair<multimap<string,int>::const_iterator,multimap<string,int>::const_iterator> range;
  if(key.empty()){
    range.first=key2idx.begin();
    range.second=key2idx.end();
  }
  else range=key2idx.equal_range(key);
  
  for(multimap<string,int>::const_iterator it=range.first; 
      it != range.second; it++)
    ret.insert(pair<string,string>(it->first,values[it->second]));

  return ret;

}


///////////////////////////////////////////////////////////////////////////
void segmentfile::_description_type::
_attribute_collection_type::addAttribute(const std::string &key,const std::string &value){
  int idx=values.size();
  key2idx.insert(pair<string,int>(key,idx));
  values.push_back(value);
}
///////////////////////////////////////////////////////////////////////////	
	void segmentfile::_description_type::
	_attribute_collection_type::replaceAttribute(const std::string &key,const std::string &value){
	  removeAttribute(key);
	  addAttribute(key,value);
	}
///////////////////////////////////////////////////////////////////////////	
void segmentfile::_description_type::
_attribute_collection_type::removeAttribute(const std::string &key){
  
  if(key.empty())
    reset();
  else{
    removecount += key2idx.count(key);

    pair<multimap<string,int>::iterator,
      multimap<string,int>::iterator> range;
    range=key2idx.equal_range(key);
    key2idx.erase(range.first,range.second);

    if(2*removecount > (int)values.size()){
      // over 50% of the attributes discarded -> 
      // compress the vector
      vector<string> newval;
      for(multimap<string,int>::iterator it=key2idx.begin();
	  it != key2idx.end() ; it++){
	newval.push_back(values[it->second]);
	it->second = newval.size()-1;
      }
      values=newval;
      removecount=0;
    }
  }
}
///////////////////////////////////////////////////////////////////////////

}// namespace ?

  




// Local Variables:
// mode: font-lock
// End:

