#include <ConnectedKMeans.h>
#include <math.h>
#include <il/ilHistEqImg.h>
#include <il/ilBlendImg.h>

static const string vcid =
"@(#)$Id: ConnectedKMeans.C,v 1.12 2009/04/29 14:32:55 vvi Exp $";

static ConnectedKMeans list_entry(true);


///////////////////////////////////////////////////////////////////////////

ConnectedKMeans::ConnectedKMeans() {
  default_variance=400;
  adjacency_energy=5;
}

///////////////////////////////////////////////////////////////////////////

ConnectedKMeans::~ConnectedKMeans() {
}

///////////////////////////////////////////////////////////////////////////

const char *ConnectedKMeans::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void ConnectedKMeans::UsageInfo(ostream& os) const { 
  os << "ConnectedKMeans :" << endl
     << "  options : " << endl
     << "    -k <# of means> (default: 13)" << endl
     << "    -m    preserves separate segment labels in output" 
     << "    -a adjacency energy (default: 4)" << endl;
}

///////////////////////////////////////////////////////////////////////////

int ConnectedKMeans::ProcessOptions(int argc, char** argv)
{
  string switches;
  int argc_old=argc;

  while (*argv[0]=='-' && argc>1) {
    if(!switches.empty()) switches += " ";
    switch (argv[0][1]) {
    case 'k':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -k");
	break;
      }

      switches += "-k ";
      switches += argv[1];

      if(sscanf(argv[1],"%d",&(KMeans::number_of_means)) != 1)
	EchoError("switch -k requires an integer specifier.");

      argc--;
      argv++;
      break;

    case 'a':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -a");
	break;
      }

      switches += "-a ";
      switches += argv[1];

      if(sscanf(argv[1],"%f",&(ConnectedKMeans::adjacency_energy)) != 1)
	EchoError("switch -a requires a floating point argument");

      argc--;
      argv++;
      break;

    case 'm':
      switches += "-m ";
      output_multiple=true;
      break;
      
    default:
      EchoError(string("unknown option ")+argv[0]);
    } // switch
    argc--;
    argv++;
 
  } // while
  
  if(!switches.empty()) AddToXMLMethodOptions(switches);

  return argc_old-argc;

}

///////////////////////////////////////////////////////////////////////////

int ConnectedKMeans::AssignLabels()
{
  int x,y,old_s,new_s,s;
  int max_changed=10;
  float r,g,b;

  int l=Height()*Width();

  float *dist = new float[number_of_means];
  float *energy = new float[number_of_means];
  std::vector<int> nc(number_of_means);
  std::vector<int> process_next_time(l,1);

  int changed=0;

  for(y=0;y<Height();y++) for(x=0;x<Width();x++)
    if(!GetPixelSegment(x,y,original_labels[x+y*Width()])){
      delete[] dist;  delete[] energy;
      return -1;
    }

  do{
    changed=0;
    int ind=0;

    for(y=0;y<Height();y++) for(x=0;x<Width();x++){
      if(!GetPixelSegment(x,y,old_s) || !GetPixelRGB(x,y,r,g,b)){
	delete[] dist; delete[] energy;
	return -1;
      }
      
      if(!(process_next_time[ind]&1)) continue;

      int dir;
      for(s=0;s<number_of_means;s++) nc[s]=0;
      
      int delta_x[8]={1,1,0,-1,-1,-1,0,1};
      int delta_y[8]={0,-1,-1,-1,0,1,1,1};

      for(dir=0;dir<8;dir++){
	GetPixelSegmentExtended(x+delta_x[dir&7],y+delta_y[dir&7],s);
	
	nc[s]++;
      }

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

	//		dist[s] *= default_variance/(variance[s]+1) ;

// 	diff = gr[s]-grad[x+y*Width()];
// 	diff *= 255.0/(max_grad/1.2);
// 	diff *= diff;
// 	dist[s] += diff;
	   
	energy[s]=dist[s] /default_variance  -2*adjacency_energy*nc[s];
      }
      
      new_s=0;
      for(s=1;s<number_of_means;s++){
	if(energy[s]<energy[new_s])
	  new_s=s;
      }

      if(new_s != old_s){
	changed++;
	if(!SetPixelSegment(x,y,new_s)){
	  delete[] dist;
	  return -1;
	}
 	for(dir=0;dir<8;dir++)
 	  if(!OutOfBounds(x+delta_x[dir],y+delta_y[dir]))
 	    process_next_time[x+delta_x[dir]+(y+delta_y[dir])*Width()] |= 2;
	
      }
      ind++;
    }
    
    for(x=0;x<l;x++)
    if(process_next_time[x]&2)
	 process_next_time[x]=1;
       else
	 process_next_time[x]=0;

   //  cout << "Energy minimisation: " <<changed << " pixels changed." << endl;
  } while(changed > max_changed);

  delete[] dist;
  delete[] energy;

  changed=0;

  for(y=0;y<Height();y++) for(x=0;x<Width();x++){
    if(!GetPixelSegment(x,y,new_s)){
      return -1;
    }
    
    if(original_labels[x+y*Width()] != new_s){
      changed++;
    }
  }

  return changed;
  
}

///////////////////////////////////////////////////////////////////////////

bool ConnectedKMeans::PreProcess()
{
  bool ret=KMeans::PreProcess();
  original_labels= new int[Width()*Height()];
  return ret;
}

///////////////////////////////////////////////////////////////////////////

bool ConnectedKMeans::PostProcess()
{
  //  cout << "calling KMeans::PostProcess" << endl;
  bool ret=KMeans::PostProcess();

  //cout << "retrned from call" << endl;


  delete[] original_labels;

  //cout << "PostProcess finishing." << endl;

  return ret;
}

///////////////////////////////////////////////////////////////////////////

bool ConnectedKMeans::DoProcess()
{

  int borderwidth=10;
  int changed;
  int max_changes=10;

  int x,y,s;
  bool ok;

  // mark backround pixels

  for (y=0;y<Height();y++) for (x=0; x<Width(); x++){
    if(!previous_method) {
      if(x < borderwidth || Width()-1-x < borderwidth ||
	 y < borderwidth || Height()-1-y < borderwidth)
	SetPixelSegment(x,y,0);
      else
	SetPixelSegment(x,y,1);
      }
    else{
      ok = previous_method->GetPixelSegment(x,y,s,true);
      if (!ok) {
	cerr << "CbFilter::Process() failed in GetPixelSegment()" << endl;
      }
      if(s)
	SetPixelSegment(x,y,0);
      else
	SetPixelSegment(x,y,1);
    }
  }
  
  do{
    if(!KMeans::CalculateMeans()) return false;
    changed=KMeans::AssignLabels();
    if (changed<0) return false;
    if(Verbose()>1)
      cout << changed << " pixels changed label." << endl;
  } while(changed > max_changes);

  float v=ImageVariance();
  cout << "Variance is " << v << endl;
  float blendfactor = 0;

#ifdef USE_IL

  imgdata bl_data;

  while(GetImageObject()->getType()==Segmentation::imgdata::imgdata_il && 
	v<2500 && blendfactor < 1.0){
    blendfactor += 0.1;

    ilHistEqImg eqimg(GetImageObject()->getILImage());
    bl_data.setILImage(new ilBlendImg(GetImageObject()->getILImage(),
				     &eqimg,1.0-blendfactor));

    if(!CopyImage(&bl_data, false)) return false;
    v=ImageVariance();
    if(Verbose()) cout << "Variance is " << v << endl;
  } 
#endif // USE_IL 

 adjacency_energy = 5 + blendfactor*25;

 if(!KMeans::CalculateMeans()) return false;
  if(!FilterLineEdges(means)) return false;

  if(!CalculateGradient()) return false;
  do{
    if(!KMeans::CalculateMeans()) return false;
    changed=KMeans::AssignLabels();
    if (changed<0) return false;
    if(Verbose()>1)
      cout << changed << " pixels changed label." << endl;
  } while(changed > max_changes);

  do{
    if(!CalculateMeans()) return false;
   //  cout << "means now:" << endl;
//     for(s=0;s<number_of_means;s++)
//       cout << "("<<mean[3*s]<<","<<mean[3*s+1]<<","<<mean[3*s+2]<<") ("
// 	   << variance[s] << ")" << endl;

      changed=AssignLabels();
      if (changed<0) return false;
      if(Verbose()>1)
        cout << changed << " pixels changed label." << endl;
    } while(changed > max_changes);


  // assume that largest segment is the background 
  // we weight anyway with the square of the average distance 
  // of segment from the center

  std::vector<int> dist(number_of_means,0);
  std::vector<int> count(number_of_means,0);

  int cx=Width()/2;
  int cy=Height()/2;

  for (y=0;y<Height();y++) for (x=0; x<Width(); x++){
     if(!GetPixelSegment(x,y,s)) return false;
     int delta=x-cx;
     dist[s]+=delta*delta;
     delta=y-cy;
     dist[s]+=delta*delta;
     count[s]++;
  }

  int back_label=0;

  for(s=1;s<number_of_means;s++){
    if(dist[s]>dist[back_label])
      back_label=s;
  }
  
 //  cout << "back label is " << back_label << endl;

   for (y=0;y<Height();y++) for (x=0; x<Width(); x++){
     if(!GetPixelSegment(x,y,s)) return false;
     int val=1;
     if(output_multiple){
       val=s;
       if(s==0) val=back_label;
     }
     if(s==back_label) val=0;
     if(!SetPixelSegment(x,y,val)) return false;
   }
  
 //   cout << "DoProcess finished." << endl;

  return true;

}  
  
///////////////////////////////////////////////////////////////////////////

float ConnectedKMeans::ImageVariance()
{
  int x,y;
  float sum[3]={0,0,0},sq=0;
  float r,g,b;
  int n=Width()*Height();
  
  for(y=0;y<Height();y++) for(x=0;x<Width();x++){
    if(!GetPixelRGB(x,y,r,g,b)) return -1;
    sum[0] += r; 
    sum[1] += g; 
    sum[2] += b; 
    sq += r*r+g*g+b*b;
  }

  sum[0] /= n; sum[1] /= n; sum[2] /= n;

  return sq/n-sum[0]*sum[0]-sum[1]*sum[1]-sum[2]*sum[2]; 
}

///////////////////////////////////////////////////////////////////////////
