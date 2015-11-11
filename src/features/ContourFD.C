// $Id: ContourFD.C,v 1.10 2009/04/22 09:55:13 mats Exp $	

#include <ContourFD.h>

namespace picsom {
  static const char *vcid =
    "$Id: ContourFD.C,v 1.10 2009/04/22 09:55:13 mats Exp $";

static ContourFD list_entry(true);

//=============================================================================

string ContourFD::Version() const {
  return vcid;
}



//=============================================================================

Feature::featureVector ContourFD::getRandomFeatureVector() const {
  featureVector ret;

  for(int i=0;i<param.number_of_coefficients*4+2;i++)
    ret.push_back(rand()/(float)RAND_MAX); 
 
  return ret;
}

//=============================================================================

bool ContourFD::ProcessOptionsAndRemove(list<string>&l){

  for (list<string>::iterator i = l.begin(); i!=l.end();) {
   
    string key, value;
    SplitOptionString(*i, key, value);
    
    if (key=="order") {
      if(sscanf(value.c_str(),"%d",&param.number_of_coefficients) != 1)
	throw string("ContourFD::ProcessOptionsAndRemove: couldn't parse order.");
      i = l.erase(i);
      continue;
    }
    else if(key=="highorderemph") {
      param.multiply_by_n2= IsAffirmative(value)?true:false;
      i = l.erase(i);
      continue;
    }
    if (key=="normtrans") {
      param.normalise_translation= IsAffirmative(value)?true:false;
      i = l.erase(i);
      continue;
    }
    if (key=="normstart") {
      param.normalise_startpoint = IsAffirmative(value)?true:false;
      i = l.erase(i);
      continue;
    }
    if (key=="normrot") {
      param.normalise_rotation = IsAffirmative(value)?true:false;
      i = l.erase(i);
      continue;
    }
    if (key=="normscale") {
      param.normalise_scaling = IsAffirmative(value)?true:false;
      i = l.erase(i);
      continue;
    }
    if (key=="normxflip") {
      param.normalise_xmirror = IsAffirmative(value)?true:false;
      i = l.erase(i);
      continue;
    }
    if (key=="normall") { 
      param.normalise_scaling = IsAffirmative(value)?true:false;
      param.multiply_by_n2= IsAffirmative(value)?true:false;
      param.normalise_translation= IsAffirmative(value)?true:false;
      param.normalise_rotation = IsAffirmative(value)?true:false;
      param.normalise_startpoint = IsAffirmative(value)?true:false;

      i = l.erase(i);
      continue;
    }
    else if (key=="postprocessing") {
      if(value=="none")
	param.postprocessing=none;
      else if(value=="rectify")
	param.postprocessing=rectify;
      else if(value=="absolutes")
	param.postprocessing=absolutes;
      else
	throw string("ContourFD::ProcessOptionsAndRemove: unknown postprocessing.");
      i = l.erase(i);
      continue;
    }
    i++;
  }

  return Feature::ProcessOptionsAndRemove(l);

}


//=============================================================================

int ContourFD::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  str << "order=<int>           list coefficients up to order (def. 10)" 
      << endl;
  str << "highorderemph=<bool>  multiply coefficients by sqr of order (def. true)" << endl;
  str << "normtrans=<bool>      normalise against translation (def. true)" << endl;
  str << "normstart=<bool>      normalise against startpoint (def. true)" << endl;
  str << "normrot=<bool>        normalise against rotation (def. false)" << endl;
  str << "normscale=<bool>      normalise against scaling (def. true)" << endl;
  str << "normxflip=<bool>      normalise against x mirroring (def. false)" << endl;
  str << "normall=<bool>        turn on/off all normalisations" << endl;
  str << "postprocessing=none|rectify|absolutes " << endl;

  return ret+8;
}


//=============================================================================
Feature *ContourFD::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{

  param.number_of_coefficients=10;
  param.multiply_by_n2=true;
  param.normalise_translation=true;
  param.normalise_startpoint=true;
  param.normalise_rotation=false;
  param.normalise_scaling=true;
  param.normalise_xmirror=false;
  param.postprocessing=none;
  return Feature::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================

// bool ContourFD::CalculateAndPrint(){
//  const string errhead = "ContourFD::CalculateAndPrint() : ";
  
//   for (int i=0;; i++) {
//     char namestr[50];
//     sprintf(namestr,"contour[%d]",i);
//     SegmentationResult *res=SegmentData()->readLastFrameResultFromXML(namestr,"pixelchain");
//     if(res==NULL) break;

//     PixelChain pc;
//     istringstream ss(res->value.c_str());

//     ss >> pc;
    
//     featureVector fV=CalculateFourierDescriptors(pc);
//     Normalise(fV);

//     char nrstr[50];
//     sprintf(nrstr,"%d",i);

//     Print(fV, *fout);
//     *fout << " " << Label(-1,-1, nrstr) << endl;
//     delete res;
//   }

//   return true;
// }

//=============================================================================

bool ContourFD::CalculateOneLabel(int frame, int label)
{
    EnsureImage();
    SegmentData()->setCurrentFrame(frame);

    set<coord> regionCoord;
    coord seed;

    int ind = DataIndex(label, true);
    if(ind==-1) return false;

    int width = Width(true), height = Height(true);

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++) {

	vector<int> svec = SegmentVector(frame, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  if(svec[j]==label)
	    {
	      if(regionCoord.empty())
		seed=coord(x,y);
	      regionCoord.insert(coord(x,y));
	      
	    }
	}
      }
    
    if(regionCoord.size()>4){ // smaller regions would give nonsense results anyway
      coordList *border=traceBorder(regionCoord,seed);
      ((ContourFDData*)GetData(ind))->Calculate(*border,param);
      delete border;
    }
    else{
      ((ContourFDData*)GetData(ind))->Zero();
    }
    
    calculated = true;

    return true;
}

bool ContourFD::CalculateOneFrame(int frame){
  // cout << "ContourFD::CalculateOneFrame("<<frame<<")"<<endl;

  vector<set<coord> > regionCoord(DataCount());
  vector<coord> seed(DataCount());

   EnsureImage();
   SegmentData()->setCurrentFrame(frame);

    int width = Width(true), height = Height(true);

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++) {

	vector<int> svec = SegmentVector(frame, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  int ind = DataIndex(svec[j], true);
	  // cout << "Got index " << ind << endl;
	  if(ind != -1){ // label in use
	    if(regionCoord[ind].empty())
	      seed[ind]=coord(x,y);
	    regionCoord[ind].insert(coord(x,y));
	  }
	}
      }

    //    cout << "Region coordinates for "<< regionCoord.size() << "collected"
    //	 << endl;

    for(size_t i=0;i<regionCoord.size();i++){
      if(regionCoord[i].size()>4){
	coordList *border=traceBorder(regionCoord[i],seed[i]);
	((ContourFDData*)GetData(i))->Calculate(*border,param);
	delete border;
      }
      else
	((ContourFDData*)GetData(i))->Zero();
    }
    calculated = true;

    return true;
}


coordList *ContourFD::traceBorder(const set<coord> &cs, const coord &seed,
				  bool use_diagonals)
{
  int incr = (use_diagonals)?1:2;
  coord extreme=seed;

  list<coord> *l=new list<coord>;
  
    
  coord c(seed);
  const int delta_x[8]={1,1,0,-1,-1,-1,0,1};
  const int delta_y[8]={0,-1,-1,-1,0,1,1,1};
  int dir,start_dir;

  if( NeighbourCountSeg(c,use_diagonals,cs)==0) // isolated point
    {
      coordList *ret=new coordList;
      
      ret->push_back(c);
      ret->push_back(c);
	
      delete l;
      return ret;
    }
    
  // determine where we came from
  
  for(dir=0;!HasNeighbourSeg(c,(dir+4)&7,cs) || 
	HasNeighbourSeg(c,(dir+4+incr)&7,cs);dir-=incr ) {}
  
  start_dir = dir&7;
  
  do{
    l->push_back(c);
      
    if(c<extreme){
      extreme=c;
    }
    
    for(dir+=6; !HasNeighbourSeg(c,dir,cs);dir+=incr) {}
    dir &= 7;

    c.x += delta_x[dir];
    c.y += delta_y[dir];
  } while (c != seed || dir != start_dir);   
  
  
  while( l->front() != extreme){ // start from extreme coordinate
      l->push_back(l->front());
      l->pop_front();
  }
  
  // close contour
  l->push_back(l->front());
  
  coordList *ret=new coordList;
  
  for(list<coord>::const_iterator it=l->begin(); it != l->end(); it++)
    ret->push_back(*it);

  delete l;
  return ret;
} 



///////////////////////////////////////////////////////////////////////////
bool ContourFD::HasNeighbourSeg(const coord &c,int dir,const set<coord> &cs){
  
  //  cout << "ContourFD::HasNeighbourSeg(("<<c<<"),"<<dir<<"): ";

  const int delta_x[8]={1,1,0,-1,-1,-1,0,1};
  const int delta_y[8]={0,-1,-1,-1,0,1,1,1};
  coord nc(c.x+delta_x[dir&7],c.y+delta_y[dir&7]);
  bool ret (cs.count(nc));

  //cout <<"neighbour in ("<<nc<<")="<<ret<<endl;
  return ret;

}

///////////////////////////////////////////////////////////////////////////

int ContourFD::NeighbourCountSeg(const coord &c,bool use_diagonals,const set<coord> &cs){
  int dir,ctr=0;
  int incr = (use_diagonals)?1:2;
    
  for(dir=0;dir<8;dir+=incr) 
    if(HasNeighbourSeg(c,dir,cs)) ctr++;
    
  return ctr;
      
}

void ContourFD::ContourFDData::Calculate(const coordList &chain,const Param &p)
{

  //  cout << "Input chain: " << endl;
  //  cout << chain;

  const double PI=3.1415926536;

  bool flipx=false;

  featureVector fV(p.number_of_coefficients*4+2);

  int order;
  double length;
  std::vector<coord>::const_iterator c; 
  c=chain.begin();

  complex<double> old_pos,new_pos;

  // find out the length of the chain
  length=0;
  old_pos=complex<double>((*c).x,(*c).y);
  for(c++; c!= chain.end(); c++){
    new_pos=complex<double>((*c).x,(*c).y);
    length += abs(new_pos-old_pos);
    old_pos=new_pos;
  }

  double s=0; // parametre in direction of the contour

  complex<double> pos_exp;
  complex<double> pos_exp_old;
  complex<double> delta_z,z_prime;
  complex<double> pos_coeff,neg_coeff;
  complex<double> exp_factor; 

  for(order=1;order<(int)(fV.size()-2)/4+1;order++){
    pos_exp_old=1; 
    s=0;
    pos_coeff=neg_coeff=0;
    exp_factor= -2.0*complex<double>(0,1)*PI*(order/length);

    c=chain.begin();
    old_pos= complex<double>(flipx ? -(*c).x : (*c).x,(*c).y);
    for(c++; c!= chain.end(); c++){
      new_pos=complex<double>(flipx ? -(*c).x : (*c).x,(*c).y);

      delta_z= new_pos-old_pos;
      s += abs(delta_z);
      pos_exp = exp(exp_factor*s);
      z_prime=delta_z/abs(delta_z);
      
      pos_coeff += z_prime*(pos_exp-pos_exp_old);
      neg_coeff += z_prime*conj(pos_exp-pos_exp_old);

      old_pos=new_pos;
      pos_exp_old = pos_exp;
    }

    double norm = length/(4*PI*PI*order*order);

    fV[2+(order-1)*4]=real(pos_coeff)*norm;
    fV[2+(order-1)*4+1]=imag(pos_coeff)*norm;
    fV[2+(order-1)*4+2]=real(neg_coeff)*norm;
    fV[2+(order-1)*4+3]=imag(neg_coeff)*norm;

    if(order==1 && p.normalise_xmirror){
      
      // the angle of the major semi-axis
      float phi = 0.5*(arg(pos_coeff)+arg(neg_coeff));
      
      // actually it's determined only modulo PI, we bring it to [0,PI)
      
      while(phi<0) phi += PI;
      while(phi>=PI) phi -= PI; 
      
      // we flip x direction if the major semi-axis is descending

      if(phi > 0.5*PI){
	flipx=true;
	order--; // recalculate first order coefficients
      }

    }

  }

  // calculate coefficient z_0
  
  pos_coeff=0;

  c=chain.begin();
  old_pos=complex<double>((*c).x,(*c).y);
  for(c++; c!= chain.end(); c++){
    new_pos=complex<double>((*c).x,(*c).y);
    delta_z= new_pos-old_pos;
    s = abs(delta_z);
    pos_coeff += s*(new_pos+old_pos)/2.0;
    old_pos=new_pos;
  }

  double norm = 1/length;

  fV[0]=real(pos_coeff)*norm;
  fV[1]=imag(pos_coeff)*norm;
  Normalise(fV,p);

  v=fV;

  switch(p.postprocessing){
  case rectify:
    for(size_t i=0;i<v.size();i++)
      if(v[i]<0) v[i]=-v[i];
    break;
  case absolutes:
    v.clear();
    v.push_back(fV[0]);
    v.push_back(fV[1]);
    for(size_t i=2;i<fV.size();i+=2)
      v.push_back(sqrt(fV[i]*fV[i]+fV[i+1]*fV[i+1]));
    break;
  case none:
    break;
  }



}

//=============================================================================

void ContourFD::ContourFDData::Normalise(featureVector &fV,const Param &p){
  
  if(p.normalise_translation){
    fV[0]=fV[1]=0;
  }
  
  complex<double> zpa(fV[2],fV[3]);
  complex<double> zna(fV[4],fV[5]);

  double psi_pos = arg(zpa);
  double psi_neg = arg(zna);

  double theta = (p.normalise_startpoint) ? (psi_neg-psi_pos)/2:0;
  double psi = (p.normalise_rotation) ? (psi_neg+psi_pos)/2:0;
  double alpha = (p.normalise_scaling) ? 1/(abs(zpa)+abs(zna)):1;

  complex<double> J(0,1);

  for(int order=1;order <(int)(fV.size()-2)/4+1;order++){
      
    complex<double> zpb(fV[2+(order-1)*4],fV[2+(order-1)*4+1]);
    complex<double> znb(fV[2+(order-1)*4+2],fV[2+(order-1)*4+3]);

    zpb *= alpha*exp(J*(order*theta-psi));
    znb *= alpha*exp(J*(-order*theta-psi));
    
    if(p.multiply_by_n2){
      zpb *= order*order;
      znb *= order*order;
    }

    fV[2+(order-1)*4]  =real(zpb);
    fV[2+(order-1)*4+1]=imag(zpb);
    fV[2+(order-1)*4+2]=real(znb);
    fV[2+(order-1)*4+3]=imag(znb);

  }
}
}

//=============================================================================


// Local Variables:
// mode: font-lock
// End:

