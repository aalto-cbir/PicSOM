#include <GKMeans.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;
using std::list;

static const string vcid =
"@(#)$Id: GKMeans.C,v 1.5 2009/04/30 08:39:02 vvi Exp $";

namespace picsom{
  static GKMeans list_entry(true);

  ///////////////////////////////////////////////////////////////////////////
  
  GKMeans::GKMeans() {
    useWeights=false;
    estimateVariance=false;
    refineResult=false;
    
    useMRFRelaxation=false;
    
    xTileSize=32;
    yTileSize=32;
    
    numberOfMeans=5;
    partToUse=-1;
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  GKMeans::~GKMeans() {
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  const char *GKMeans::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void GKMeans::UsageInfo(ostream& os) const { 
    os << "GKMeans :" << endl
       << "  options : " << endl
       << "    -k <# of means> (default: 5)" << endl
       << "    -w do not weight clusters w/ # of points in k-means (default)"
       << endl
       << "    -W weight clusters w/ # of points in k-means" << endl
       << "    -v do not estimate variance in k-means (default)" << endl
       << "    -V estimate variance in k-means" << endl
       << "    -m do not use MRF relaxation in k-means (default)" << endl
       << "    -M use MRF relaxation in k-means" << endl
       << "    -T <xTileSize>[:<yTileSize>] (default: 32:xTileSize)" << endl
       << "    -f <feature_name> <feature_options> <component_list> <weight_list>" << endl
       << "    -r refine results inside tiles after k-means" << endl
       << "    -p <# of points/cluster to use in k-means" << endl
       << "    -P equivalent to -p 50" 
       << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int GKMeans::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'k':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -k");
	  break;
	}
	
	option.option = "-k";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(GKMeans::numberOfMeans)) != 1)
	  throw string("switch -k requires an integer specifier.");
	
	argc--;
	argv++;
	break;
	
      case 'w':
	option.option = "-w";
	useWeights=false;
	break;
	
      case 'W':
	option.option = "-W";
	useWeights=true;
	break;
	
      case 'v':
	option.option = "-v";
	estimateVariance=false;
	break;

      case 'V':
	
	option.option = "-V";
	estimateVariance=true;
	break;
	
      case 'm':
	option.option = "-m";
	useMRFRelaxation=false;
	break;
	
      case 'M':
	option.option = "-M";
	useMRFRelaxation=true;
	break;
	
      case 'T':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -T");
	  break;
	}
	option.option = "-T";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(GKMeans::xTileSize)) != 1)
	  throw string("switch -T requires an integer specifier.");

	if(strchr(argv[1],':')){
	  if(sscanf(strchr(argv[1],':')+1,"%d",&(GKMeans::yTileSize)) != 1)
	    throw string("switch -T requires an integer specifier after :.");
	}
	else yTileSize = xTileSize;
	
	argc--;
	argv++;
	break;
	
      case 'f':
	if (argc<5){
	  throw string("Not enough commandline arguments for switch -f");
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
	
      case 'r':
	option.option = "-r";
	refineResult=true;
	break;      
      case 'p':
	if (argc<3){
	  throw string("Not enough commandline arguments for switch -p");
	  break;
	}
	
	option.option = "-p";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(GKMeans::partToUse)) != 1)
	  throw string("switch -p requires an integer specifier.");
	
	argc--;
	argv++;
	break;
      case 'P':
	option.option = "-P";
	partToUse=50;
	break;
      default:
	throw string("unknown option ")+argv[0];
      } // switch

      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
    
    return argc_old-argc;
    
  }
  
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool GKMeans::Process()
  {

    // preprocess
    
    featureInterface.createMethods(getImg());
    
    mean = vector<Feature::featureVector>(numberOfMeans,Feature::featureVector(featureInterface.featureSpec.size(),0));
    
    //  cout << "Allocated " << mean.size()<<" means w/ " << mean[0].size()
    //     << " elements each." << endl;
    
    
    
    variance = vector<Feature::featureVector>
      (numberOfMeans,Feature::featureVector(featureInterface.featureSpec.size(),0));
    weight = vector<int>(numberOfMeans,0);
    
    dim.x = (int)ceil((float)Width()/xTileSize);
    dim.y = (int)ceil((float)Height()/yTileSize);
    
    lbl=new char[dim.x*dim.y];
    fVal=new Feature::featureVector[dim.x*dim.y]; 

    if(Verbose() > 1){
      
      // dump info about the parametres from the command line
      
      cout << "tilesize: " << coord(xTileSize,yTileSize) << endl;
      cout << "image size : " << dim << " tiles" << endl;
      
      cout << "useWeights: " << useWeights << endl;
      cout << "estimateVariance: " << estimateVariance << endl;
      cout << "useMRFRelaxation: " << useMRFRelaxation << endl;
      
      cout << "numberOfMeans: " << numberOfMeans << endl;
      
      
      cout << "number of feature classes: " << featureInterface.featureClass.size() << endl;
      unsigned int i;
      
      for(i=0;i<featureInterface.featureClass.size();i++){
	cout << "feature class " << i << endl;
	cout << "name: " << featureInterface.featureClass[i].name << endl;
	cout << "options: "<<endl;
	list<string>::const_iterator it;
	for(it=featureInterface.featureClass[i].options.begin();
	    it!=featureInterface.featureClass[i].options.end(); it++)
	  cout << "\t" << *it << endl;
      }
      
      cout << "number of feature specifications: " << featureInterface.featureSpec.size() << endl;
      
      for(i=0;i<featureInterface.featureSpec.size();i++){
	cout << "feature specification " << i << endl;
	cout << "class: " << featureInterface.featureSpec[i].c << endl;
	cout << "index: " << featureInterface.featureSpec[i].index << endl;
	cout << "weight: " << featureInterface.featureSpec[i].weight << endl;
      }
    }
    
    int x,y;
    
    for(y=0;y<dim.y;y++) for(x=0;x<dim.x;x++){
      if(Verbose()>2)    
      	cout << "Calculating features for tile (" << x<<","<<y<<endl;
      calculateFeatures(x,y,fVal[x+y*dim.x]); 
            if(Verbose()>2) {
      	cout << "Got feature vector [";
      	size_t i;
      	for(i=0;i<fVal[x+y*dim.x].size();i++)
      	  cout << fVal[x+y*dim.x][i] << " " << endl;
      	cout << "]" << endl; 
            }
	
    }
    
    if(partToUse>0){
      pixelCount=partToUse*numberOfMeans;
      if(pixelCount>dim.x*dim.y){
	partToUse=-1;
      }
      else{
	
	cout << "pixel count to use: " << pixelCount << endl;
	cout << "image dimension: " << dim.x*dim.y << endl;
	
	lbl_part=new char[pixelCount];
	fVal_part=new Feature::featureVector[pixelCount]; 
	int i;
	for(i=0;i<pixelCount;i++){
	  int j=rand()%(dim.x*dim.y);
	  fVal_part[i]=fVal[j];
	}
      }
    }

    // doProcess
    
    if(dim.x==0 || dim.y ==0 || featureInterface.featureSpec.size()==0) 
      return ProcessNextMethod(); 
    
    int changed;
    int max_changes=3;
    
    // bool ok;
    
    // randomly initialize pixels
    
    if(partToUse>0)
      for (x=0;x<pixelCount;x++)
	{
	  int j=rand() % numberOfMeans;
	  lbl_part[x]=j;
	  //cout << "lbl_part["<<x<<"]="<<(int)lbl_part[x]<<endl;
	}
    else
      for (x=0;x<dim.x*dim.y;x++)
	lbl[x]=rand() % numberOfMeans;
    
    
    // cout << "Initialisation done." << endl;
    
    do{
      if(!CalculateMeans()) return false;
      if(Verbose()>1){  
	cout << "means now:" << endl;
	int s,c;
	for(s=0;s<numberOfMeans;s++){
	  cout<<"("<<mean[s][0];
	  for(c=1;(unsigned)c<mean[s].size();c++)
	    cout <<","<<mean[s][c];
	  cout<<")"<<endl;
	}
      }
    
      changed=AssignLabels();
      if (changed<0) return false;
      if(Verbose()>1)
	cout << changed << " pixels changed label." << endl;
    } while(changed > max_changes);

  cout << "initial classification done" << endl;

    if(refineResult) // rVal=new Feature::featureVector[xTileSize*yTileSize]; 
      for(y=0;y<Height();y++) for(x=0;x<Width();x++){
	Feature::featureVector tempF;
	recalculateFeatures(x,y,tempF);
	
	int s,new_s;
	
	float dist;
	float mindist=9999999999.0;
	new_s = 0;
	for(s=0;s<numberOfMeans;s++){
	  dist = Feature::VectorSqrDistance(tempF,mean[s]);
	  
	  if(estimateVariance){ 
	    if(variance[s][0])
	      dist /= variance[s][0];
	    else if(dist!=0) continue;
	  }
	
	  if(useWeights)
	    dist -= log((double)weight[s]);
	  
	  if(dist < mindist){
	    new_s = s;
	    mindist=dist;
	  }
	}

	
	getImg()->set_pixel_segment(x,y,new_s);
	
	if(partToUse){
	  delete lbl_part;
	  delete fVal_part;
	}

      }
    else{
      
      if(partToUse>0){
	partToUse=-1;
	delete lbl_part;
	delete fVal_part;
	AssignLabels();
      }

      //     cout << "dimension: " << coord(Width(),Height()) << endl;  
      for(y=0;y<dim.y;y++)
	for(x=0;x<dim.x;x++){
	  int j,k;
	  
	  for(j=0;j<yTileSize && y*yTileSize+j<Height();j++)
	    for(k=0;k<xTileSize && x*xTileSize+k<Width();k++){
	      getImg()->set_pixel_segment(x*xTileSize+k,y*yTileSize+j,lbl[x+y*dim.x]);
	    }
	}
    }

    featureInterface.freeClasses();

    delete lbl;
    delete fVal;
    
    return ProcessNextMethod();

  }
  
  ///////////////////////////////////////////////////////////////////////////

  bool GKMeans::CalculateMeans()
  {
    int s,c,pos;

    //  cout << "Calculating means" << endl;
    
    int vlen=mean[0].size();

    // cout << "vlen=" << vlen << endl;
    
    weight=vector<int>(numberOfMeans,0);
    
    for(s=0;s<numberOfMeans;s++){
      // mean[s]=Feature::featureVector(vlen,0); // doesn't compile in sphinx

      mean[s]=Feature::featureVector();
      for(int i=0;i<vlen;i++) mean[s].push_back(0);

      variance[s]=Feature::featureVector();
      variance[s].push_back(0); 
    }
    
    // cout << "means initialised" << endl;
    
    if(partToUse>0){
      for(pos=0;pos<pixelCount;pos++){
	// cout << "pos=" << pos << "lbl_part[pos]="<<(int)lbl_part[pos]<<endl;
	Feature::AddToFeatureVector(mean[lbl_part[pos]],fVal_part[pos]);
	//cout << "Added to feature vector" << endl;
	//cout<<"("<<fVal_part[pos][0];
	//  for(c=1;(unsigned)c<fVal_part[pos].size();c++)
	//    cout <<","<<fVal_part[pos][c];
	//  cout<<")"<<endl;

	for(c=0;c<vlen;c++)
	  variance[lbl_part[pos]][0] += fVal_part[pos][c] * fVal_part[pos][c];
	weight[lbl_part[pos]]++;
      }
    }
    else
      for(pos=0;pos<dim.x*dim.y;pos++){
	Feature::AddToFeatureVector(mean[lbl[pos]],fVal[pos]);
	for(c=0;c<vlen;c++)
	  variance[lbl[pos]][0] += fVal[pos][c] * fVal[pos][c];
	weight[lbl[pos]]++;
      }   
    
    
    // cout << "Statistics accumulated" << endl;
    
    for(s=0;s<numberOfMeans;s++){
      if(weight[s] > 0){
	
	Feature::DivideFeatureVector(mean[s],weight[s]);
	
	variance[s][0] /= weight[s];
	
	for(c=0;c<vlen;c++)
	  variance[s][0] -= mean[s][c]*mean[s][c];
	
	variance[s][0] *= weight[s]/(weight[s]-0.5);
      }
    }
    
    return true;
    
  }
  
  ///////////////////////////////////////////////////////////////////////////

  int GKMeans::AssignLabels()
  {
    int new_s,s,pos;

    int changed=0;
    
    if(partToUse >0)
      
      for(pos=0;pos<pixelCount;pos++){
	
	float dist;
	float mindist=9999999999.0;
	new_s = 0;
	for(s=0;s<numberOfMeans;s++){
	  dist = Feature::VectorSqrDistance(fVal_part[pos],mean[s]);
	  
	  if(estimateVariance){ 
	    if(variance[s][0])
	      dist /= variance[s][0];
	    else if(dist!=0) continue;
	  }
	  
	  if(useWeights)
	    dist -= log((double)weight[s]);
	  
	  if(dist < mindist){
	    new_s = s;
	    mindist=dist;
	  }
	}

	if(new_s != lbl_part[pos]){
	  lbl_part[pos]=new_s;
	  changed++;
	}
      } // partToUse
    else for(pos=0;pos<dim.x*dim.y;pos++){

      float dist;
      float mindist=9999999999.0;
      new_s = 0;
      for(s=0;s<numberOfMeans;s++){
	dist = Feature::VectorSqrDistance(fVal[pos],mean[s]);
	
	if(estimateVariance){ 
	  if(variance[s][0])
	    dist /= variance[s][0];
	  else if(dist!=0) continue;
	}
	
	if(useWeights)
	  dist -= log((double)weight[s]);
	
	if(dist < mindist){
	  new_s = s;
	  mindist=dist;
	}
      }
      
      if(new_s != lbl[pos]){
	lbl[pos]=new_s;
	changed++;
      }
    }

    return changed;

  }


  ///////////////////////////////////////////////////////////////////////////

  void GKMeans::calculateFeatures(int x, int y, Feature::featureVector &v){
      
    int x1,x2,y1,y2;
  
    x1=x*xTileSize; x2 = x1 + xTileSize-1;
    if(x2>=Width()) x2 = Width()-1;

    y1=y*yTileSize; y2 = y1 + yTileSize-1;
    if(y2>=Height()) y2 = Height()-1;
    
    int i,j;
    coordList l;
  

    for(i=x1;i<=x2;i++) for(j=y1;j<=y2;j++)
      l.push_back(coord(i,j));

    vector<coordList *> vc;
    vc.push_back(&l);
    
    featureInterface.calculateFeatures(vc,v,partToUse);
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void GKMeans::recalculateFeatures(int x, int y, Feature::featureVector &v){
    
    int x1,x2,y1,y2;
    
    x1=x-1; if(x1<0) x1 = 0;
    x2=x+1; if(x2>=Width()) x2 = Width()-1;
    
    y1=y-1; if(y1<0) y1 = 0;
    y2=y+1; if(y2>=Height()) y2 = Height()-1;
    
    int i,j;
    coordList l;
    
    for(i=x1;i<=x2;i++) for(j=y1;j<=y2;j++)
      l.push_back(coord(i,j));

    vector<coordList *> vc;
    vc.push_back(&l);
    
    featureInterface.calculateFeatures(vc,v);
  }
}

  ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
  
