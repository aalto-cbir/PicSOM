#include <KMeans.h>
#include <math.h>

using std::string;
using std::ostream;
using std::cout;
using std::endl;

static const string vcid =
"@(#)$Id: KMeans.C,v 1.13 2009/04/30 09:50:04 vvi Exp $";
namespace picsom {
  static KMeans list_entry(true);

  ///////////////////////////////////////////////////////////////////////////

  KMeans::KMeans() {
    number_of_means=13;
    filter_line_edges=false;
    filter_by_rgb=false;

    partToUse=-1;
    randomizeUnused=false;

    mean=NULL;
    variance=NULL;
    gr=NULL;
    means=NULL;

  }

  ///////////////////////////////////////////////////////////////////////////
  
  KMeans::~KMeans() {
    delete[] mean;
    delete[] variance;
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *KMeans::Version() const { 
    return vcid.c_str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  void KMeans::UsageInfo(ostream& os) const { 
    os << "KMeans :" << endl
       << "  options : " << endl
       << "    -k <# of means>  (default: 13)" << endl
       << "    -f               filter line edges in resulting segmentation" 
       << endl
       << "    -p <# of points/cluster to use in k-means" << endl
       << "    -P equivalent to -p 50" << endl 
       << "    -r <tolerance>   filter line edges in input image" << endl
       << "    -R               same as -r 0.08"
       << "    -z               randomly replace unused labels" << endl
       << endl;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int KMeans::ProcessOptions(int argc, char** argv)
  {
    Option option;
    int argc_old=argc;

    //  cout << "Beginning to process options for KMeans, argc="<<argc<< endl;
    //   cout << "*argv[0]=" << *argv[0] << endl;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'k':
	if (argc<3){
	  throw string("KMeans: Not enough commandline arguments for switch -k");
	  break;
	}
	
	option.option="-k";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%d",&(KMeans::number_of_means)) != 1)
	  throw string("KMeans: switch -k requires an integer specifier.");
	
	argc--;
	argv++;
	break;

      case 'f':
	
	option.option="-f";
	filter_line_edges=true;

	break;

      case 'r':
	if (argc<3){
	  throw string("KMeans: Not enough commandline arguments for switch -r");
	  break;
	}
	
	option.option="-r";
	option.addArgument(argv[1]);
	
	if(sscanf(argv[1],"%f",&(KMeans::filter_tolerance)) != 1)
	  throw string("KMeans: switch -r requires an integer specifier.");
	
	filter_by_rgb=true;

	argc--;
	argv++;
	break;

      case 'R':
	
	option.option="-R";
	filter_by_rgb=true;
	filter_tolerance = 0.08;

	break;

      case 'z':
	
	option.option="-z";
	randomizeUnused=true; 
	break;


  case 'p':
      if (argc<3){
	throw string ("KMeans: Not enough commandline arguments for switch -p");
	break;
      }

      option.option="-p";
      option.addArgument(argv[1]);

      if(sscanf(argv[1],"%d",&(KMeans::partToUse)) != 1)
	throw string("KMeans::processOptions() switch -p requires an integer specifier.");

      argc--;
      argv++;
      break;
    case 'P':
      option.option="-P";
      partToUse=50;
      break;
	
      default:
	throw string("KMeans::processOptions(): unknown option ")+argv[0];
      } // switch
      
      if(!option.option.empty())
	addToSwitches(option);
    
      argc--;
      argv++; 
    } // while
  
    return argc_old-argc;
  
  }


  ///////////////////////////////////////////////////////////////////////////

  bool KMeans::Process()
  {

    int oldPartToUse = partToUse;

    // preProcess

//     cout << "KMeans::Process()" << endl;
    
    mean = new float[3*number_of_means];
    means = new RGBTriple[number_of_means];
    
    //variance= new float[number_of_means];
    //gr= new float[number_of_means];
    

    int w=getImg()->getWidth();
    int h=getImg()->getHeight();
    
    //grad= new float[w*h];
    //if(!CalculateGradient()) return false;
    
    int borderwidth=10;
    int changed;
    int max_changes=10;
    
    int x,y /* ,s */;

    if(borderwidth*4>h) borderwidth=h/4;
    if(borderwidth*4>w) borderwidth=w/4;


    // bool ok;
    
    // mark backround pixels
    
    //     cout << "Marking background" << endl;

    if(filter_by_rgb)
      getImg()->FilterLineEdgesByRGB(filter_tolerance);


    if(partToUse>0){

      coord dim(w,h);

      max_changes=0;
      pixelCount=partToUse*number_of_means;
      if(pixelCount>dim.x*dim.y){
	partToUse=-1;
      }
      else{

	//	cout << "pixel count to use: " << pixelCount << endl;
	// cout << "image dimension: " << dim.x*dim.y << endl;

	lbl_part=new unsigned int[pixelCount];
	fVal_part=new float[3*pixelCount]; 
	int i;

	for(i=0;i<pixelCount;i++){
	  int jx=rand()%dim.x;
	  int jy=rand()%dim.y;

	  getImg()->get_pixel_rgb(jx,jy,fVal_part[3*i],
				  fVal_part[3*i+1], fVal_part[3*i+2]);

	  lbl_part[i]=i/partToUse;
	}
      }
    }

    if(partToUse<=0){
      for (y=0;y<h;y++) 
	for (x=0; x<w; x++){
	  //if(!previous_method) {
	  if(x < borderwidth || w-1-x < borderwidth ||
	     y < borderwidth || h-1-y < borderwidth)
	    getImg()->set_pixel_segment(x,y,0);
	  else{
	    if(number_of_means < 2)
	      getImg()->set_pixel_segment(x,y,0);
	    else if(number_of_means == 2)
	      getImg()->set_pixel_segment(x,y,1);
	    else{

	      int lbl = number_of_means;
	      while(lbl>=number_of_means){
		lbl=1+(int)(((float)rand())/((float)RAND_MAX)*(number_of_means-1));
	      }
	      //	      cout << "randomly set lbl to " << lbl;
	      getImg()->set_pixel_segment(x,y,lbl);
	    }
	  }
	} // for
    }



    do{
      if(!CalculateMeans(randomizeUnused)) return false;
      //int s;
	     //              cout << "means now:" << endl;
	     //      for(s=0;s<number_of_means;s++)
	     //      	cout << "("<<mean[3*s]<<","<<mean[3*s+1]<<","<<mean[3*s+2]<<")"<<endl;
      
      changed=AssignLabels();
      if (changed<0) return false;
      if(Verbose()>1)
	cout << changed << " pixels changed label." << endl;
    } while(changed > max_changes);

    if(filter_line_edges){
      getImg()->FilterLineEdges();
      //  if(!CalculateGradient()) return false;
    
      do{
	if(!CalculateMeans()) return false;
	changed=AssignLabels();
	if (changed<0) return false;
	if(Verbose()>1)
	  cout << changed << " pixels changed label." << endl;
      } while(changed > max_changes);
    } // if (filter_line_edges)
    
  //   for (y=0;y<h;y++) for (x=0; x<w; x++){
//      int s;
//        getImg()->get_pixel_segment(x,y,s);
//        cout << "x y s = "<<x<<" "<<y<<" "<<s<<endl;
//    }    

    // postProcess

    if(partToUse>0){
      partToUse=-1;
      delete[] lbl_part;
      delete[] fVal_part;
      AssignLabels();
    }
    
    delete[] means;
    delete[] mean;
    means=NULL;
    mean=NULL;

    //delete[] grad;
   
//     vector<int> *l=getImg()->getUsedLabels();
//     cout << "Used labels:" ;
//     for(int li=0;li<l->size();li++)
//       cout <<" "<<(*l)[li];
//     cout << endl;

    //  delete l;

    partToUse=oldPartToUse;

    return ProcessNextMethod();
    
  }  
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool KMeans::CalculateMeans(bool randomise)
  {
    // cout << "in KMeans::CalculateMeans()" << endl;

    int x,y,s;
    float r,g,b;
    
    int *count=new int[number_of_means];
    
    for(s=0;s<number_of_means;s++){
      //      cout << "zeroing, s="<<s<<endl;
      //variance[s]=gr[s]=
      mean[3*s]=mean[3*s+1]=mean[3*s+2]=count[s]=0;
    }
    int w=getImg()->getWidth();
    int h=getImg()->getHeight();
    
    if(partToUse >0){
      for(int i=0;i<pixelCount;i++){
	s=lbl_part[i];
	//	cout << "s=" << s << endl;
	r=fVal_part[3*i];
	g=fVal_part[3*i+1];
	b=fVal_part[3*i+2];

	mean[3*s] += r;
	mean[3*s+1] += g;
	mean[3*s+2] += b;
      
	// cout << "(r,g,b) = (" << r << ","<<g<<","<<b<<")"<<endl;

	//gr[s] += grad[x+y*w];
      
	//variance[s] += r*r+g*g+b*b;
	count[s]++;
      }
    }else{
      for(y=0;y<h;y++) for(x=0;x<w;x++){
	getImg()->get_pixel_segment(x,y,s);
	getImg()->get_pixel_rgb(x,y,r,g,b);
      
	//cout << "x="<<x<<"y="<<y<<" s="<<s<<" r="<<r<<" g="<<g<<" b="<<b<<endl; 

	mean[3*s] += r;
	mean[3*s+1] += g;
	mean[3*s+2] += b;
      
	//gr[s] += grad[x+y*w];
      
	//variance[s] += r*r+g*g+b*b;
	count[s]++;
      }
    }

      for(s=0;s<number_of_means;s++){
	//    cout << "count["<<s<<"]="<< count[s] << endl;
	
	if(count[s] > 0){
	  mean[3*s] /= count[s];
	  mean[3*s+1] /= count[s];
	  mean[3*s+2] /= count[s];
	  
	  //gr[s] /= count[s];
	  
	  //variance[s] /= count[s];
	
	  //	  variance[s] -= mean[3*s]*mean[3*s];
	  //variance[s] -= mean[3*s+1]*mean[3*s+1];
	  //variance[s] -= mean[3*s+2]*mean[3*s+2];
	  
	  //variance[s] *= count[s]/(count[s]-0.5);
	}
	else{ // this cluster is useless -> try new one at random
	  if(randomise){
	    mean[3*s]=((float)rand())/RAND_MAX; 
	    mean[3*s+1]=((float)rand())/RAND_MAX; 
	    mean[3*s+2]=((float)rand())/RAND_MAX; 
	    
	    //gr[s]=((float)rand()*max_grad)/RAND_MAX;
	    
	    //variance[s]=0.25; // from hat
	  }
	}
      }
    
    delete[] count;
    
    int i;
    for(i=0;i<number_of_means;i++){
      means[i].r(mean[3*i]);
      means[i].g(mean[3*i+1]);
      means[i].b(mean[3*i+2]);
    }
    
    return true;
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int KMeans::AssignLabels()
  {
    // cout << "in KMeans::AssignLabels" << endl;
    
    int x,y,old_s,new_s,s;
    float r,g,b;
    
    float *dist = new float[number_of_means];
    int changed=0;

    int w=getImg()->getWidth();
    int h=getImg()->getHeight();
    if(partToUse>0){
      for(int i=0;i<pixelCount;i++){
	old_s=lbl_part[i];
	r=fVal_part[3*i];
	g=fVal_part[3*i+1];
	b=fVal_part[3*i+2];

	//      getImg()->get_pixel_rgb(x,y,r,g,b);
      
      for(s=0;s<number_of_means;s++){
	dist[s]=0;
	
	float diff = mean[3*s]-r;
	diff *= diff;
	dist[s] += diff;
	
	diff = mean[3*s+1]-g;
	diff *= diff;
	dist[s] += diff;
	
	diff = mean[3*s+2]-b;
	diff *= diff;
	dist[s] += diff;
	
	//      diff = gr[s]-grad[x+y*Width()];
	//       diff *= 255.0/(max_grad/2);
	//       diff *= diff;
	//       dist[s] += diff;
	
	// dist[s] /= variance[s];
      }
      
      new_s=0;
      for(s=1;s<number_of_means;s++){
	if(dist[s]<dist[new_s])
	  new_s=s;
      }
      
      if(new_s != old_s){
	changed++;
	lbl_part[i]=new_s;
	//	getImg()->set_pixel_segment(x,y,new_s);
      }
    }

    }else{
    for(y=0;y<h;y++) for(x=0;x<w;x++){
      getImg()->get_pixel_segment(x,y,old_s);
      getImg()->get_pixel_rgb(x,y,r,g,b);
      
      for(s=0;s<number_of_means;s++){
	dist[s]=0;
	
	float diff = mean[3*s]-r;
	diff *= diff;
	dist[s] += diff;
	
	diff = mean[3*s+1]-g;
	diff *= diff;
	dist[s] += diff;
	
	diff = mean[3*s+2]-b;
	diff *= diff;
	dist[s] += diff;
	
	//      diff = gr[s]-grad[x+y*Width()];
	//       diff *= 255.0/(max_grad/2);
	//       diff *= diff;
	//       dist[s] += diff;
	
	// dist[s] /= variance[s];
      }
      
      new_s=0;
      for(s=1;s<number_of_means;s++){
	if(dist[s]<dist[new_s])
	  new_s=s;
      }
      
      if(new_s != old_s){
	changed++;
	getImg()->set_pixel_segment(x,y,new_s);
      }
    }
    }

    delete[] dist;
    return changed;
    
  }
  
  ///////////////////////////////////////////////////////////////////////////
  bool  KMeans::CalculateGradient()
  {
    
    int x,y;
    
    float r,g,b,r1,g1,b1;
    
    float gr;
    
    int size;
    
    max_grad=0;

    int w=getImg()->getWidth();
    int h=getImg()->getHeight();
    
    for(y=0;y<h;y++)
      for(x=0;x<w;x++){
	getImg()->get_pixel_rgb(x,y,r,g,b);
	gr=0; size=0;
	
	if(getImg()->coordinates_ok(x,y-1)){
	  getImg()->get_pixel_rgb(x,y-1,r1,g1,b1);
	  gr+=((r-r1>0)?r-r1:r1-r);
	  gr+=((g-g1>0)?g-g1:g1-g);
	  gr+=((b-b1>0)?b-b1:b1-b);
	  size++;
	}
	
	if(getImg()->coordinates_ok(x-1,y  )){
	  getImg()->get_pixel_rgb(x-1,y,r1,g1,b1);
	  gr+=((r-r1>0)?r-r1:r1-r);
	  gr+=((g-g1>0)?g-g1:g1-g);
	  gr+=((b-b1>0)?b-b1:b1-b);
	  size++;
	}
	
	if(getImg()->coordinates_ok(x+1,y)){
	  getImg()->get_pixel_rgb(x+1,y,r1,g1,b1);
	  gr+=((r-r1>0)?r-r1:r1-r);
	  gr+=((g-g1>0)?g-g1:g1-g);
	  gr+=((b-b1>0)?b-b1:b1-b);
	  size++;
	}
	
	if(getImg()->coordinates_ok(x,y+1)){
	  getImg()->get_pixel_rgb(x,y+1,r1,g1,b1);
	  gr+=((r-r1>0)?r-r1:r1-r);
	  gr+=((g-g1>0)?g-g1:g1-g);
	  gr+=((b-b1>0)?b-b1:b1-b);
	  size++;
	}
	
	gr *= 4.0/size;
	grad[x+y*w] = gr;
	if(gr>max_grad) max_grad=gr;
      }
    
    return true;
    
  }
///////////////////////////////////////////////////////////////////////////
}

// Local Variables:
// mode: font-lock
// End:



