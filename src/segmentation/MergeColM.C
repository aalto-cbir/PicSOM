#include <MergeColM.h>
#include <math.h>

static const string vcid =
"@(#)$Id: MergeColM.C,v 1.8 2009/08/05 08:58:06 jorma Exp $";

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::set;

namespace picsom {

  static MergeColM list_entry(true);


  ///////////////////////////////////////////////////////////////////////////

  MergeColM::MergeColM() {    
    numberOfRegions=5;
    sizeThreshold=600;
    useAveraging=false;
    weightDistances=0;
    pixelsToUse=-1;
    weightBias=0.0002;
    varBias=0.8;
  }

  ///////////////////////////////////////////////////////////////////////////

  MergeColM::~MergeColM() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *MergeColM::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////
  
  void MergeColM::UsageInfo(ostream& os) const { 
    os << "MergeColM :" << endl
       << "  options : " << endl
      //      << "    -f <feature> [-o opt1=str1 opt2=...] c1:c2:...:cn w1:w2:...:wn" << endl
       << "    -r <desired # of regions> (default: 5)" << endl
       << "    -s <region area threshold> (in pixels, default: 600) " << endl
       << "    -a combine feature vectors for two regions by averaging " << endl
       << "    -p <# of pixels to use in feature calculation>" << endl
       << "    -w1 weight distances according to size of smaller region" << endl
       << "    -w2 weight distances according to sum of region sizes" << endl
       << "    -b  bias term used in region size weighting" << endl
       << "    -v  bias term used in variance weighting" << endl;

    

  }

  ///////////////////////////////////////////////////////////////////////////

  int MergeColM::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'r':
	if (argc<3){
	EchoError("Not enough commandline arguments for switch -r");
	break;
      }

      option.option = "-r";
      option.addArgument(argv[1]);

      if(sscanf(argv[1],"%d",&(MergeColM::numberOfRegions)) != 1)
	EchoError("switch -r requires an integer specifier.");

      argc--;
      argv++;
      break;

    case 's':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -s");
	break;
      }

      option.option = "-s";
      option.addArgument(argv[1]);

      if(sscanf(argv[1],"%d",&(MergeColM::sizeThreshold)) != 1)
	EchoError("switch -s requires an integer specifier.");

      argc--;
      argv++;
      break;


    case 'f':
      if (argc<5){
	EchoError("Not enough commandline arguments for switch -f");
	break;
      }
      option.option = "-f";
      {
	int nr_opts=featureInterface.parseFeatureSpec(argv+1);
	int nrargs=3+2*nr_opts;
	
	int j;

	for(j=1;j<=nrargs;j++)
	  option.addArgument(argv[j]);
	
	argc -= nrargs;
	argv += nrargs;
      }
      break;

      case 'a':
	option.option = "-a";
	useAveraging=true;
	break;

      case 'p':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -p");
	  break;
	}

	option.option = "-p";
	option.addArgument(argv[1]);

	if(sscanf(argv[1],"%d",&(MergeColM::pixelsToUse)) != 1)
	  EchoError("switch -p requires an integer specifier.");

	argc--;
	argv++;
	break;

      case 'w':
	
	if(argv[0][2]=='1'){
	  option.option = "-w1";
	  weightDistances=1;
	  break;
	}
	else if(argv[0][2]=='2'){
	  option.option = "-w2";
	  weightDistances=2;
	  break;
	}
	else if(argv[0][2]=='3'){
	  option.option = "-w3";
	  weightDistances=3;
	  break;
	}
	else
	  throw string("KMeans::processOptions(): unknown option ")+argv[0];

	// cout << "weightDistances=" << weightDistances << endl;

	break;

      case 'b':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -b");
	  break;
	}

	option.option = "-b";
	option.addArgument(argv[1]);
      
	if(sscanf(argv[1],"%f",&(MergeColM::weightBias)) != 1)
	  EchoError("switch -b requires a float specifier.");
	
	argc--;
	argv++;
	break;

      case 'v':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -v");
	  break;
	}

	option.option = "-v";
	option.addArgument(argv[1]);
      
	if(sscanf(argv[1],"%f",&(MergeColM::varBias)) != 1)
	  EchoError("switch -v requires a float specifier.");
	
	argc--;
	argv++;
	break;


      default:
	throw string("MergeColM::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;

  }


  ///////////////////////////////////////////////////////////////////////////

  bool MergeColM::Process() {

    // preProcess
    
    int i;

    char **fspec = new char*[3];
    fspec[0] = strdup("colm");
    fspec[1] = strdup("0:1:2:3:4:5");
    fspec[2] = strdup("1:1:1:1:1:1");

    featureInterface.featureSpec.clear();
    featureInterface.featureClass.clear();
  
    featureInterface.parseFeatureSpec(fspec);
    for (size_t u=0; u<3; u++)
      delete fspec[u];
    delete fspec;

    featureInterface.createMethods(getImg());

    regionList.clear();
    while(queue.size()) queue.pop();

    // doProcess

    if(Verbose() > 1){

      // dump info about the parametres from the command line
      cout << "number of feature classes: " << featureInterface.featureClass.size() << endl;


      for(i=0;(unsigned)i<featureInterface.featureClass.size();i++){
	cout << "feature class " << i << endl;
	cout << "name: " << featureInterface.featureClass[i].name << endl;
	cout << "options: " ;
	list<string>::const_iterator it;
	for(it=featureInterface.featureClass[i].options.begin();
	    it!=featureInterface.featureClass[i].options.end(); it++)
	  cout << *it << " ";
	cout << endl;

      }

      cout << "number of feature specifications: " << featureInterface.featureSpec.size() << endl;
      
      for(i=0;(unsigned)i<featureInterface.featureSpec.size();i++){
	cout << "feature specification " << i << endl;
	cout << "class: " << featureInterface.featureSpec[i].c << endl;
	cout << "index: " << featureInterface.featureSpec[i].index << endl;
	cout << "weight: " << featureInterface.featureSpec[i].weight << endl;
      }
    

    }

    listRegions();

    joinSmallRegions();

    cout << "After joining small regions, regionCount= " << regionCount 
	 << endl;
    
    fillPriorityQueue();
    
    while(regionCount>numberOfRegions){
    if(Verbose()>1) cout <<"region count:" << regionCount 
      << " queue size: " << queue.size() << endl; 
			   mergeTopRegions();
      }
    
    // read result from lists

    int label=0;
    for(i=0;(unsigned)i<regionList.size();i++){
      Region *r = &(regionList[i]);
      if(r->joinedTo != i) continue;

      int j;
      for(j=0;(unsigned)j<r->l.size();j++)
	getImg()->set_pixel_segment(r->l[j].x,r->l[j].y,label);
      label++;

    }

    featureInterface.freeClasses();
    
    return ProcessNextMethod();

  }
  
  ///////////////////////////////////////////////////////////////////////////

  bool MergeColM::listRegions()
  {
    // enumerate dirs
    
    // 0 - E
    // 1 - SW
    // 2 - S
    // 3 - SE
    
    int incr[4];
    int l=Width()*Height(),i;
    unsigned char *isNeighbour= new unsigned char[l];
    
    incr[0]=1;
    incr[1]=Width()-1;
    incr[2]=Width();
    incr[3]=Width()+1;
    
    for(i=0;i<l;i++){
      isNeighbour[i]=15;
    }
    for(i=0;i<Width();i++){
      isNeighbour[l-i-1] = 1; // lower neighbours away
    }
    
    for(i=0;i<l;i+=Width()){
      isNeighbour[i+Width()-1] &= 6; // right neighbours away
    }
    
    int x,y,dir;

    lbl=getImg()->GetSeparateLabelingInt();
    int maxLabel=0;
    
    for(i=0;i<l;i++)
      if(lbl[i]>maxLabel) maxLabel=lbl[i];
    
    regionList.clear();
    regionList.resize(maxLabel+1);
    
    for(i=0,x=0,y=0;i<l;i++,x++){
      if(x>=Width()){ x=0;y++;}
      
      regionList[lbl[i]].joinedTo=lbl[i];
      regionList[lbl[i]].l.push_back(coord(x,y));

      for(dir=0;dir<4;dir++)
	if(isNeighbour[i]&(1<<dir))
	  if(lbl[i]!=lbl[i+incr[dir]]){
	    regionList[lbl[i]].nList.insert(lbl[i+incr[dir]]);
	    regionList[lbl[i+incr[dir]]].nList.insert(lbl[i]);
	  }
	  
    }

    regionCount=0;
  
    for(i=0;(unsigned)i<regionList.size();i++){
      if(regionList[i].l.size()) regionCount++;
      else regionList[i].joinedTo=-1;
    }

    if(Verbose()>1) cout << "Found " << regionCount << " separate regions." << endl;
    
    
    delete lbl;
    delete isNeighbour;

    return true;

  }

  ///////////////////////////////////////////////////////////////////////////
  
  bool MergeColM::joinSmallRegions()
  {
    
    
    int i;
    // first look for small regions that have only one neighbour
    
    // dumpRegionList();
    
    for(i=0;(unsigned)i<regionList.size();i++){
      if(regionList[i].joinedTo==i)
	if((int)regionList[i].l.size()<=sizeThreshold 
	   && (int)regionList[i].nList.size()==1)
	  
	  mergeRegions(i,*regionList[i].nList.begin(),false);
    }    

    // then merge all small regions to their closest neighbours
  
    // if possible, do not join all small regions to one larger one

    //    set<int> noJoin;
    /// bool noJoinEncountered;

    // do{
    //  cout << "new joining round" << endl;
    //  noJoin.clear();
    //  noJoinEncountered=false;
      for(i=0;(unsigned)i<regionList.size();i++)
	if(regionList[i].joinedTo==i){
	  while(regionList[i].joinedTo==i && (int)regionList[i].l.size()<=sizeThreshold){
	    //    if(noJoin.count(i)>0)
	    //  noJoinEncountered=true;
	    // else{
	      updateFeatureValues(i);
	      mergeRegions(i,findClosest(i),true);
	      // int newnr =i;
	      //while(regionList[newnr].joinedTo!=newnr) 
	      //	newnr=regionList[newnr].joinedTo;

	      //noJoin.insert(newnr);
	      //set<int>::const_iterator it; 
	      //for(it=regionList[newnr].nList.begin();
	      //	  it!=regionList[newnr].nList.end();
	      //	  it++)
	      //	noJoin.insert(*it);
	      //}
	  }
	}
  //} while(noJoinEncountered);
 
      return true;
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void MergeColM::mergeRegions(int i1, int i2, bool updateFeatures)
  {
    //  cout << "Merging regions " << i1 << " and " << i2 << endl;
    
    if(i1>i2){
      int tmp=i1;
      i1=i2;
      i2=tmp;
    }

    Region *r1=&(regionList[i1]),*r2=&(regionList[i2]);

    //cout << "merging sizes " << r1->l.size() << " and " << r2->l.size()
    //	 << endl;

    set<int>::iterator it;
    for(it=r2->nList.begin(); it != r2->nList.end(); it++){
      if(*it == i1) continue;
      
      r1->nList.insert(*it);
      Region *r=&(regionList[*it]);
      r->nList.erase(i2);
      r->nList.insert(i1);
    }
    
    r1->nList.erase(i2);

    if(updateFeatures && useAveraging){
      int i;
      for(i=0;i<(int)r1->fV.size();i++){
	r1->fV[i] = (r1->l.size()*r1->fV[i]+r2->l.size()*r2->fV[i])/
	  (r1->l.size()+r2->l.size());
      }
    }


    int i;
    for(i=0;i<(int)r2->l.size();i++)
      r1->l.push_back(r2->l[i]);
    
    r2->l.clear();
    r2->nList.clear();
    r2->joinedTo=i1;

    regionCount--;

    vector<coordList *> vc;
    vc.push_back(&r1->l);

    if(updateFeatures && !useAveraging)
      featureInterface.calculateFeatures(vc,r1->fV,pixelsToUse); // inefficient

  }

  ///////////////////////////////////////////////////////////////////////////
  void MergeColM::fillPriorityQueue(){
    currentTime=0;
    int i;

    Region *r,*r2;
    
    for(i=0;i<(int)regionList.size();i++){
      r=&(regionList[i]);
      if(r->joinedTo==i){
	r->timestamp=0;
	updateFeatureValues(i);
	set<int>::iterator it;
	for(it=r->nList.begin(); it != r->nList.end(); it++)
	  if(*it > i){
	    
	    regionDiff d;
	    d.timestamp=0;
	    d.r1=i;
	    d.r2=*it;
	    r2 = &(regionList[*it]);

	    // we take negative because of the ordering
	    // of the priority queue

	    d.dist=-Feature::VectorSqrDistance(r->fV,r2->fV,0,2);
	    
	    // normalise by variance

	    d.dist /= varBias + r->fV[3]*r->fV[3] + r2->fV[3]*r2->fV[3] +
	      r->fV[4]*r->fV[4] + r2->fV[4]*r2->fV[4] +
	      r->fV[5]*r->fV[5] + r2->fV[5]*r2->fV[5];

	    if(weightDistances==1){
	      float s = regionList[*it].l.size();
	      if(s>r->l.size())
		s= r->l.size();
	      
	      s /= Height();
	      s /= Width();
	      
	      d.dist *= weightBias + sqrt(s);
	      
	    }
	    
	    else if(weightDistances==2){
	      // cout << "Weighting with sum." << endl;
	      float s = regionList[*it].l.size();
	      s += r->l.size();
	      
	      s /= Height();
	      s /= Width();
	      
	      d.dist *= weightBias + sqrt(s);
	      
	    }

	    else if(weightDistances==3){

	      //	      throw string("weighting 3 used iun filling the queue");
	      
	      //	      cout << "Weighting with sum." << endl;
	      float s = regionList[*it].l.size();
	      s += r->l.size();
	      
	      s /= Height();
	      s /= Width();
	      
	      d.dist *= weightBias + s;

	      

	    }

	    
	    queue.push(d);
	  //	  cout << "push ("<<d.r1<<","<<d.r2<<")" << "w/ dist=" << d.dist << endl; 

	  }
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////
  void MergeColM::mergeTopRegions(){
    
    regionDiff d=queue.top();
    queue.pop();

    if(d.r1>d.r2){
      int tmp=d.r1;
      d.r1=d.r2;
      d.r2=tmp;
    }
    
    Region *r1=&(regionList[d.r1]);
    Region *r2=&(regionList[d.r2]);

    if(d.timestamp < r1->timestamp || d.timestamp < r2->timestamp) return;

    currentTime++;
    int i=d.r1;

    //cout << "Merging regions " << d.r1 <<" and " << d.r2 
    //	 << " w\ d=" << d.dist << endl;

    mergeRegions(d.r1,d.r2,true);

    set<int>::iterator it;
    for(it=r1->nList.begin(); it != r1->nList.end(); it++){
      regionDiff d;
      d.timestamp=currentTime;
      d.r1=i;
      d.r2=*it;
      d.dist=-Feature::VectorSqrDistance(r1->fV,regionList[*it].fV);

      if(weightDistances==1){
	float s = regionList[*it].l.size();
	if(s>r1->l.size())
	  s= r1->l.size();
      
	s /= Height();
	s /= Width();

	d.dist *= weightBias + sqrt(s);      
      
      }

      else if(weightDistances==2){
	float s = regionList[*it].l.size();
	s += r1->l.size();
	
	s /= Height();
	s /= Width();

	d.dist *= weightBias + sqrt(s);      
      }

      else if(weightDistances==3){

	float s = regionList[*it].l.size();
	s += r1->l.size();
	
	s /= Height();
	s /= Width();

	d.dist *= weightBias + s;      
      }
      //cout << "Pushing ("<<d.r1<<","<<d.r2<<") d="<<d.dist<<endl;
      //cout <<"sizes are " << r1->l.size() << " and " << regionList[*it].l.size() << endl;
      queue.push(d);
    }
    r1->timestamp=currentTime;
    r2->timestamp=currentTime;
    
    // dumpRegionList();
    
    
  }

  ///////////////////////////////////////////////////////////////////////////
  void MergeColM::dumpRegionList(){
    int i;
    cout << "Dumping region list:" << endl;
    for(i=0;i<(int)regionList.size();i++){
      //int j;
      Region *r = &(regionList[i]);
      cout << "regionList[" <<i<<"]: joinedTo=" << r->joinedTo
	 << " coordlist size: " << r->l.size() << endl;
      //    for(j=0;j<r->l.size();j++)
      //  cout << " (" << r->l[j]<<") ";
      
      cout << endl << "nList: ";
      set<int>::iterator it;
      for(it=r->nList.begin();it != r->nList.end();it++)
	cout << " " << *it;
      
      cout << endl;
      
    }
  }
  ///////////////////////////////////////////////////////////////////////////
  void MergeColM::updateFeatureValues(int i)
  {
    //cout << "updating features for region " << i<<endl; 
    
    Region *r = &(regionList[i]);

    vector<coordList *> vc;
    vc.push_back(&r->l);

    if(r->fV.empty()){
      featureInterface.calculateFeatures(vc,r->fV,pixelsToUse);
      //cout << "calculating features for region " << i << endl ;
    }

    set<int>::iterator it;
    for(it=r->nList.begin(); it != r->nList.end(); it++){
      Region *r = &(regionList[*it]);
      if(r->fV.empty()){
	vector<coordList *> vc;
	vc.push_back(&r->l);
	//  cout << "calculating features for region " << *it << endl ;
	featureInterface.calculateFeatures(vc,r->fV,pixelsToUse);
      }
    }

  }

  ///////////////////////////////////////////////////////////////////////////
  int MergeColM::findClosest(int i){

    Region *r1=&(regionList[i]);

    int minind=-1;
    float mindist=9999999999999999.0;
    
    set<int>::iterator it;
    for(it=r1->nList.begin(); it != r1->nList.end(); it++){
      Region *r = &(regionList[*it]);
      float dist=Feature::VectorSqrDistance(r->fV,r1->fV,0,2);

      dist /= 0.3 + r->fV[3]*r->fV[3] + r1->fV[3]*r1->fV[3] +
	r->fV[4]*r->fV[4] + r1->fV[4]*r1->fV[4] +
	r->fV[5]*r->fV[5] + r1->fV[5]*r1->fV[5];
      

      if(dist < mindist){
	mindist=dist;
	minind=*it;
      }
    }
    
    return minind;

  }
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  
  
  list<string> MergeColM::divideOptions(string options)
  {
    
    // primitive option string splitter
    // better one should be used

    int pos=0;
    char *str;
    
    list<string> ret;

    str=new char[options.size()+1];
    strcpy(str,options.c_str());
    
    int startind;

    for(;;){
      while(isspace(str[pos])){}
      if(!str[pos]) break;
      startind=pos;
      while(str[pos] && !isspace(str[pos])){}
      if(!str[pos]) break;
      str[pos]=0;
      ret.push_back(string(str+startind));
    }

    delete str;
    return ret;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
 

} // namespace picsom

  ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
