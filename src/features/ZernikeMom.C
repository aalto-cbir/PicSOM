// $Id: ZernikeMom.C,v 1.6 2016/10/25 08:14:43 jorma Exp $	

#include <ZernikeMom.h>

namespace picsom {
  static const char *vcid =
    "$Id: ZernikeMom.C,v 1.6 2016/10/25 08:14:43 jorma Exp $";

static ZernikeMom list_entry(true);

//=============================================================================

string ZernikeMom::Version() const {
  return vcid;
}

//=============================================================================

Feature::featureVector ZernikeMom::getRandomFeatureVector() const{
  featureVector ret;

  for(int i=0;i<NumberOfCoefficients(param.maxorder)*2;i++)
    ret.push_back(rand()/(float)RAND_MAX); 
 
  return ret;
}

//=============================================================================

bool ZernikeMom::ProcessOptionsAndRemove(list<string>&l){

  for (list<string>::iterator i = l.begin(); i!=l.end();) {
   
    string key, value;
    SplitOptionString(*i, key, value);
    
    if (key=="order") {
      if(sscanf(value.c_str(),"%d",&param.maxorder) != 1)
	throw string("ZernikeMom::ProcessOptionsAndRemove: couldn't parse order.");
      i = l.erase(i);
      continue;
    }
    else if(key=="highorderemph") {
      param.multiply_by_n2= IsAffirmative(value)?true:false;
      i = l.erase(i);
      continue;
    }
    else if (key=="postprocessing") {
      if(value=="none")
	param.postprocessing=none;
      else if(value=="absolutes")
	param.postprocessing=absolutes;
      else
	throw string("ZernikeMom::ProcessOptionsAndRemove: unknown postprocessing.");
      i = l.erase(i);
      continue;
    }
    i++;
  }

  return Feature::ProcessOptionsAndRemove(l);

}


//=============================================================================

int ZernikeMom::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  str << "order=<int>           list coefficients up to order (def. 10)" 
      << endl;
  str << "highorderemph=<bool>  multiply coefficients by sqr of order (def. true)" << endl;
  str << "postprocessing=none|absolutes " << endl;

  return ret+3;
}


//=============================================================================
Feature *ZernikeMom::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{

  param.maxorder=5;
  param.postprocessing=none;
  return Feature::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================

bool ZernikeMom::CalculateOneLabel(int frame, int label)
{
  const double PI = 3.141592654;

    EnsureImage();
    SegmentData()->setCurrentFrame(frame);

    TabulateFactorials(param.maxorder);

    FindSegmentNormalisations(label);

    int nc=NumberOfCoefficients(param.maxorder);

    vector<double> A_real(nc,0.0);
    vector<double> A_imag(nc,0.0);

    int width = Width(true), height = Height(true);

    int resind=0;

    for(int n=0;n<=param.maxorder;n++)
      for(int m=n&1;m<=n;m+=2){
	for (int y=0; y<height; y++)
	  for (int x=0; x<width; x++){
	    vector<int> svec = SegmentVector(frame, x, y);
	    for (size_t j=0; j<svec.size(); j++) {
	      if(svec[j]==label){
		double xtilde=(x-XAvg(label))*Scaling(label);
		double ytilde=(y-YAvg(label))*Scaling(label);
		double roo=sqrt(xtilde*xtilde+ytilde*ytilde);
		double theta;
		if(ytilde || xtilde)
		  theta=atan2(ytilde,xtilde);
		else
		  theta=0;
		double r=R(n,m,roo);
		r *= Scaling(label)*Scaling(label)*(n+1)/PI;
		A_real[resind] += r*cos(m*theta);
		A_imag[resind] -= r*sin(m*theta);
	      }
	    }
	  }
	resind++;
      }


    int ind = DataIndex(label, true);
    if(ind==-1) return false;

    ((ZernikeMomData*)GetData(ind))->SetData(A_real,A_imag,param);

    calculated = true;

    return true;
}

//=============================================================================

bool ZernikeMom::CalculateOneFrame(int frame){
  const double PI = 3.141592654;

  // cout << "ZernikeMom::CalculateOneFrame("<<frame<<")"<<endl;

   EnsureImage();
   SegmentData()->setCurrentFrame(frame);
   
   TabulateFactorials(param.maxorder);

   FindSegmentNormalisations();
   
   int nc=NumberOfCoefficients(param.maxorder);

   vector<double> zerovec(nc,0.0);

   vector<vector<double> > A_real(DataCount(),zerovec);
   vector<vector<double> > A_imag(DataCount(),zerovec);

    int width = Width(true), height = Height(true);

    int resind=0;

    for(int n=0;n<=param.maxorder;n++)
      for(int m=n&1;m<=n;m+=2){
	for (int y=0; y<height; y++)
	  for (int x=0; x<width; x++){
	    vector<int> svec = SegmentVector(frame, x, y);
	    for (size_t j=0; j<svec.size(); j++) {
	      int dataind = DataIndex(svec[j], true);
	      if(dataind != -1){
		int label=svec[j];
		double xtilde=(x-XAvg(label))*Scaling(label);
		double ytilde=(y-YAvg(label))*Scaling(label);
		double roo=sqrt(xtilde*xtilde+ytilde*ytilde);
		double theta;
		if(ytilde || xtilde)
		  theta=atan2(ytilde,xtilde);
		else
		  theta=0;

		double r=R(n,m,roo);
		r *= Scaling(label)*Scaling(label)*(n+1)/PI;
		A_real[dataind][resind] += r*cos(m*theta);
		A_imag[dataind][resind] -= r*sin(m*theta);
	      }
	    }
	  }
	resind++;
      }

    for(size_t i=0;i<A_real.size();i++){
      ((ZernikeMomData*)GetData(i))->SetData(A_real[i],A_imag[i],param);
    }
    calculated = true;

    return true;
}

//=============================================================================

  double ZernikeMom::R(int n,int m,double roo){
    int l=(n-m)/2;
    double ret=0;
    for(int s=0;s<=l;s++)
      ret += Coefficient(s,m,n)*pow(roo,n-2*s);

    return ret;
  }
//=============================================================================
  void ZernikeMom::TabulateFactorials(int maxorder){
    factorial=vector<int>(maxorder+1);
    factorial[0]=1;
    for(int i=1;i<=maxorder;i++)
      factorial[i]=i*factorial[i-1];
  }
//=============================================================================
  void ZernikeMom::FindSegmentNormalisations(){

    // first find the middle points of segments

    map<int,int> counts;

    int frame=SegmentData()->getCurrentFrame();

    x_avg.clear();
    y_avg.clear();
    scaling.clear();

    int width = Width(true), height = Height(true);
    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++){
	vector<int> svec = SegmentVector(frame, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  const int label=svec[j];
	  if(counts.count(label))
	    counts[label]++;
	  else
	    counts[label]=1;

	  if(x_avg.count(label)){
	    x_avg[label] += x;
	    y_avg[label] += y; 
	  }
	  else{
	    x_avg[label] = x;
	    y_avg[label] = y;
	  } 
	}
      }

    map<int,double>::iterator it;
    for(it=x_avg.begin();it != x_avg.end();it++){
      const int l=it->first;
      it->second /= counts[l];
      y_avg[l] /= counts[l];
    }

    // now the segment middle points are calculated

    // then find the sizes of the segments, i.e. their largest
    // (x-x_avg)^2 + (y-y_avg)^2

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++){
	vector<int> svec = SegmentVector(frame, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  const int label=svec[j];
	  double dx=x-x_avg[label];
	  double dy=y-y_avg[label];
	  double r2=dx*dx+dy*dy;
	  if(scaling.count(label)){
	    if(r2>scaling[label]) scaling[label]=r2;
	  }
	  else
	    scaling[label]=r2;
	}
      }

    // invert the scaling and take square root

    for(it=scaling.begin();it != scaling.end();it++)
      if(it->second)
	it->second=1/sqrt(it->second);
	
  }

//=============================================================================

  void ZernikeMom::FindSegmentNormalisations(int label){

    int frame=SegmentData()->getCurrentFrame();

    // first find the middle point of the segment

    int count=0;

    x_avg[label]=0;
    y_avg[label]=0;
    scaling[label]=0;

    int width = Width(true), height = Height(true);
    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++) {
	vector<int> svec = SegmentVector(frame, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  if (svec[j]==label) {
	    count++;
	    x_avg[label] += x;
	    y_avg[label] += y; 
	  }
	}
      }

    x_avg[label] /= count;
    y_avg[label] /= count;

    // now the segment middle points are calculated

    // then find the sizes of the segments, i.e. their largest
    // (x-x_avg)^2 + (y-y_avg)^2

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++){
	vector<int> svec = SegmentVector(frame, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  if(svec[j]==label){
	    double dx=x-x_avg[label];
	    double dy=y-y_avg[label];
	    double r2=dx*dx+dy*dy;
	      if(r2>scaling[label]) scaling[label]=r2;
	  }
	}
      }

    // invert the scaling and take square root

    if(scaling[label])
      scaling[label]=1/sqrt(scaling[label]);

  }

  //=============================================================================

  void ZernikeMom::ZernikeMomData::SetData(const vector<double> &real,
					 const vector<double> &imag,
					 const Param &p)
  {
    switch(p.postprocessing){
    case none:
      v.clear();
      for(size_t i=0;i<real.size();i++){
	v.push_back(real[i]);
	v.push_back(imag[i]);
      }
      break;
    case absolutes:
      v.clear();
      for(size_t i=0;i<real.size();i++)
	v.push_back(sqrt(real[i]*real[i]+imag[i]*imag[i]));
      break;
    default:
      break;
    }

  }
}
  //=============================================================================


// Local Variables:
// mode: font-lock
// End:

