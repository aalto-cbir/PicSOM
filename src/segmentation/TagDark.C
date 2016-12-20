#include <TagDark.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::map;

static const string vcid=
"@(#)$Id: TagDark.C,v 1.4 2009/04/30 10:24:16 vvi Exp $";
namespace picsom {
  static TagDark list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  TagDark::TagDark() {
  }

  ///////////////////////////////////////////////////////////////////////////
  
  TagDark::~TagDark() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *TagDark::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void TagDark::UsageInfo(ostream& os) const { 
    os << "TagDark :" << endl
       << "  options : " << endl
       << "    none" 
       << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int TagDark::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      default:
	throw string("TagDark::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);
    
      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;
  
  }
  ///////////////////////////////////////////////////////////////////////////

  bool TagDark::Process(){
    int v;

    segmentfile *sf=getImg();
    int w=sf->getWidth(),h=sf->getHeight();

    map<int,set<int> > nbr = sf->findNeighbours(false); // 8-neighbours

    vector<int> *lbls=sf->getUsedLabels();


    set<int> borderSegments;
    set<int> backgroundSegments;
    
    // collect border segments

    for (int y=0; y<h; y++){
      sf->get_pixel_segment(0, y, v);
      borderSegments.insert(v);
      sf->get_pixel_segment(w-1, y, v);
      borderSegments.insert(v);
    }

    for(int x=0; x<w; x++) {
      sf->get_pixel_segment(x, 0, v);
      borderSegments.insert(v);
      sf->get_pixel_segment(x, h-1, v);
      borderSegments.insert(v);
    }
    
    map<int,vector<float> > ave;
    map<int,float> var;
    map<int,int> ctr,centralArea;

    // initialise the data structures for average/variance collection
    
    vector<float> zerovec(3,0.0);

    for(size_t i=0;i<lbls->size();i++){
      int lbl=(*lbls)[i];
      ave[lbl]=zerovec;
      var[lbl]=0;
      ctr[lbl]=0;
      centralArea[lbl]=0;
    }
    
    vector<float> vf;
    
    // collect averages

    const float marginW=0.15;
    
    for (int y=0; y<h; y++)
      for(int x=0; x<w; x++) {
	sf->get_pixel_rgb(x, y, vf);
	sf->get_pixel_segment(x, y, v);
      
	ave[v][0] += vf[0];
	ave[v][1] += vf[1];
	ave[v][2] += vf[2];
	ctr[v]++;
	if(x>marginW*w && x < (1-marginW)*w && y > marginW*h && y < (1-marginW)*h)
	  centralArea[v]++;
      }
    
    for(size_t i=0;i<lbls->size();i++){
      int lbl=(*lbls)[i];
      if(ctr[lbl]){
	ave[lbl][0]/=ctr[lbl];
	ave[lbl][1]/=ctr[lbl];
	ave[lbl][2]/=ctr[lbl];
      }
    }
    
    // collect variances
    
    for (int y=0; y<h; y++)
      for(int x=0; x<w; x++) {
	sf->get_pixel_rgb(x, y, vf);
	sf->get_pixel_segment(x, y, v);
	
	float d;
	
	d=vf[0]-ave[v][0];
	var[v] += d*d;
	
	d=vf[1]-ave[v][1];
	var[v] += d*d;
	
	d=vf[2]-ave[v][2];
	var[v] += d*d;
      }

    float avevar=0;

    for(size_t i=0;i<lbls->size();i++){
      int lbl=(*lbls)[i];
      if(ctr[lbl])
	var[lbl]/=ctr[lbl];
      avevar += var[lbl];
    }
  
    avevar /= lbls->size();

    // dump variances

    //for(size_t i=0;i<lbls->size();i++){
    // int lbl=(*lbls)[i];
      //cout << "var["<<lbl<<"]"<<var[lbl]<<" ctr="<<ctr[lbl]<<endl;
    //}; 

    // find the threshold for variance

    float thr=0.3;
    bool thrFound=false;
    while(!thrFound){
      thr += 0.05;
      set<int>::iterator it;
      for(it=borderSegments.begin();!thrFound&&it!=borderSegments.end();it++)
	if(var[*it]<= thr*avevar)
	  thrFound=true;
    }

    thr *= 3.3;

    //    cout << "threshold : " << thr*avevar;

    // now determine the typical background colour

    // collect border segments that fullfill the variance criterion

    set<int> smallVarBorder;

    set<int>::iterator it;
    for(it=borderSegments.begin();it!=borderSegments.end();it++)
      if(var[*it]<= thr*avevar)
	smallVarBorder.insert(*it);

    map<int,float> area;
    set<int>::iterator it_i,it_j;

    const float varbias=0.002;

    for(it_i=smallVarBorder.begin();it_i!=smallVarBorder.end();it_i++)
      for(it_j=smallVarBorder.begin();it_j!=smallVarBorder.end();it_j++){
	float d,cum=0;
	d=ave[*it_i][0]-ave[*it_j][0];
	cum += d*d;
	d=ave[*it_i][1]-ave[*it_j][1];
	cum += d*d;
	d=ave[*it_i][2]-ave[*it_j][2];
	cum += d*d;

	if(cum<=thr*avevar){
	  float val=(ctr[*it_j]-centralArea[*it_j])/(var[*it_j]+varbias);
	  if(area.count(*it_i))
	    area[*it_i] += val;
	  else
	    area[*it_i] = val;
	}
      }

    // find maximum area

    float maxa=-1;
    int maxind;
    map<int,float>::iterator mit;
    for(mit=area.begin();mit != area.end(); mit++)
      if(mit->second>maxa){
	maxa=mit->second;
	maxind=mit->first;
      }
	

    // typical colour, tag the appropriate border segments


    for(it=borderSegments.begin();it!=borderSegments.end();it++){

	float d,cum=0;
	d=ave[maxind][0]-ave[*it][0];
	cum += d*d;
	d=ave[maxind][1]-ave[*it][1];
	cum += d*d;
	d=ave[maxind][2]-ave[*it][2];
	cum += d*d;

      if(var[*it]<= thr*avevar && cum <= thr*avevar)
	backgroundSegments.insert(*it);
    }

    bool changed;

    // iterate through the remaining segments until all 
    // flat segments neighbouring already tagged background get tagged

    do{
      changed=false;
      for(it=backgroundSegments.begin();it!=backgroundSegments.end();it++){
	set<int>::iterator it2;
	for(it2=nbr[*it].begin();it2!=nbr[*it].end();it2++){
	  if(backgroundSegments.count(*it2)) continue;

	float d,cum=0;
	d=ave[maxind][0]-ave[*it2][0];
	cum += d*d;
	d=ave[maxind][1]-ave[*it2][1];
	cum += d*d;
	d=ave[maxind][2]-ave[*it2][2];
	cum += d*d;

	if(var[*it2]<= thr*avevar && cum <= thr*avevar){
	  backgroundSegments.insert(*it2);
	  changed=true;
	}
	}
      }

    } while (changed);

    cout << "found "<< backgroundSegments.size() << " background segments"<<endl;

    // results are in the lists, now just translate them into the bitmap

    int current_lbl=1;
    map<int,int> lbltrans;

    for(size_t i=0;i<lbls->size();i++){
      int lbl=(*lbls)[i];
      if(backgroundSegments.count(lbl)){
	lbltrans[lbl]=0;
	//cout << "tagging label "<<lbl<<" as background"<<endl;
      }
      else
	lbltrans[lbl]=current_lbl++;
    }

    // translation table constructed

    for (int y=0; y<h; y++)
      for(int x=0; x<w; x++) {
	sf->get_pixel_segment(x, y, v);
	// cout << "translating "<<v<<" -> "<<lbltrans[v]<<endl;
	sf->set_pixel_segment(x, y, lbltrans[v]);
      }

    return ProcessNextMethod();

  }  
       
  ///////////////////////////////////////////////////////////////////////////
}

// Local Variables:
// mode: font-lock
// End:



