#include <Snake.h>
#include <math.h>
#include <il/ilMemoryImg.h>
#include <il/ilConvImg.h>
#include <il/ilGBlurImg.h>

#define SMALL_NUMBER 1
static const string vcid="@(#)$Id: Snake.C,v 1.6 2009/04/30 10:25:03 vvi Exp $";

static Snake list_entry(true);

///////////////////////////////////////////////////////////////////////////

Snake::Snake() {
  
  // coefficient of the various terms in the total 
  // energy

  mul[0]=3;//0.8;  // curvature energy
  mul[1]=0; //0.5;  // gradient magnitude under snake arcs 
  mul[2]=0; //;    // edge direction energy
  mul[3]=0; //50;    // distance from strong edges 
  mul[4]=1; // target contrast energy
}

///////////////////////////////////////////////////////////////////////////

Snake::~Snake() {

}

///////////////////////////////////////////////////////////////////////////

const char *Snake::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void Snake::UsageInfo(ostream& os) const { 
  os << "Snake :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

int Snake::ProcessOptions(int /*argc*/, char** /*argv*/) { 
  return 0;
}

///////////////////////////////////////////////////////////////////////////

bool Snake::DoProcess() {

  return TestProcess();

  int maxfail=300;
  int max_displacement=6;
  int round_ctr=0;

  //   cout << "In Snake::DoProcess" << endl;

  CalculateGradients();
  CalculateSums();

  //  cout << "Gradients calculated." << endl;

  InitSnake();
  //  BruteForceCalculateContentStatistics();
  CalculateAllContentStatistics();

  //  if(Verbose()>1)
  //  DumpDebugData();

  // TestIntersectionTesting();

  cout << "Snake initialised." << endl;

  //  return true;

  int i;
 //  for(i=0;i<nodecount;i++)
//     cout << "&nodes["<<i<<"]="<< nodes + i <<" "<<"loc:"<<nodes[i].loc
// 	 << "   &edges["<<i<<"]= " << edges+i << endl;

  int failcount=0;
  while( failcount < maxfail){

    int node;
    do {
      node = nodecount*rand()/RAND_MAX;
    } while (node>=nodecount);  

    cout << "round #"<<round_ctr++<<": selected node " <<node<< endl;

    SnakeNode *n =nodes+node;

    //     cout << "n: " << n << endl
    //	 << "nextnode: " << n->nextnode << endl
    //	 << "prevnode: " << n->prevnode << endl
// 	 << "nextedge: " << n->nextedge << endl
//          << "prevedge: " << n->prevedge << endl;
    
    double old_e_disp = n->displacement_energy 
      + n->prevnode->displacement_energy
      + n->nextnode->displacement_energy;
    double old_e_g = n->prevedge->gradient_energy
      + n->nextedge->gradient_energy;
    double old_e_dir=n->prevedge->direction_energy
      + n->nextedge->direction_energy;
    double old_e_dist= n->prevedge->distance_energy
      + n->nextedge->distance_energy;


    //BruteForceCalculateContentStatistics();

    //globalstats = bruteforcestats;
    // cout << globalstats << endl;


    double old_e_c = CalculateContentEnergy();

    Segmentation::coord oldcoord(n->loc);
    
    while(n->loc.x == oldcoord.x && n->loc.y==oldcoord.y)
      n->Displace(max_displacement);

    //    cout << "Done displacement" << endl;

    if(OutOfBounds(n->loc) || !CheckIntersections(n)){
      n->loc=oldcoord;
      cout << "Failed geometry check." << endl;
      // this is not counted as a failed trial move
      continue;
    }

    //    cout << "Geometric checks ready." << endl;

    CalculateDisplacementEnergy(n);
    CalculateDisplacementEnergy(n->nextnode);
    CalculateDisplacementEnergy(n->prevnode);
    CalculateGradientEnergy(n->prevedge);
    CalculateDirectionEnergy(n->prevedge);
    CalculateDistanceEnergy(n->prevedge);
    CalculateGradientEnergy(n->nextedge);
    CalculateDirectionEnergy(n->nextedge);
    CalculateDistanceEnergy(n->nextedge);
    
    cout << " changing content statistics." << endl;

    ChangeContentStatistics(n);

    // cout << " replacing content statistics: " << endl << globalstats 
    // 	 << "with ";       

    //BruteForceCalculateContentStatistics();
    cout <<"globalstats: " << globalstats << endl;

    // globalstats -= bruteforcestats;

    // if(globalstats != bruteforcestats){
    //    cout << "Warning: shortcut statistics calculation does not match brute force calculation." << endl<<"Brute force stats: " << endl << bruteforcestats
    // << endl;

    //  cout << "difference: " << globalstats << endl;
    //}

    // globalstats = bruteforcestats;

    //    cout <<"changed." << endl;

    double new_e_disp = n->displacement_energy 
      +n->prevnode->displacement_energy
      +n->nextnode->displacement_energy;
    double new_e_g = n->prevedge->gradient_energy
      + n->nextedge->gradient_energy;
    double new_e_dir=n->prevedge->direction_energy
      + n->nextedge->direction_energy;
    double new_e_dist= n->prevedge->distance_energy
      + n->nextedge->distance_energy;
    double new_e_c = CalculateContentEnergy();

    double delta_disp = new_e_disp - old_e_disp;
    double delta_g = new_e_g - old_e_g;
    double delta_dir = new_e_dir - old_e_dir;
    double delta_dist = new_e_dist - old_e_dist;
    double delta_c = new_e_c - old_e_c;

    double delta_tot = mul[0]*delta_disp + mul[1]*delta_g+
      mul[2]*delta_dir + mul[3]*delta_dist +mul[4]*delta_c;
    
 //    cout << "comparing energies: (new-old=delta) weighted" << endl;
//     cout << "Displacement: " << new_e_disp <<"-"<< old_e_disp << "="
// 	 << delta_disp << " w:" << mul[0]*delta_disp << endl;

//     cout << "Gradient: " << new_e_g <<"-"<< old_e_g << "="
// 	 << delta_g << " w:" << mul[1]*delta_g << endl;

//     cout << "Direction: " << new_e_dir <<"-"<< old_e_dir << "="
// 	 << delta_dir << " w:" << mul[2]*delta_dir << endl;

//     cout << "Distance: " << new_e_dist <<"-"<< old_e_dist << "="
// 	 << delta_dist << " w:" << mul[3]*delta_dist << endl;
  
//     cout << "Content: " << new_e_c <<"-"<< old_e_c << "="
// 	 << delta_c << " w:" << mul[4]*delta_c << endl;

//     cout << "Total delta: " << delta_tot << endl;

    if(isnan(delta_tot)){
      // probably got a NaN as result
      cout << "Abnormal energy delta." << endl;

      ShowLocs();

      cout << "old_loc: " << oldcoord << endl;
     
      BruteForceCalculateContentStatistics();

      std::vector<Segmentation::coord> list;

      for(i=0;i<nodecount;i++)
	Segmentation::Draw::LinePixelsToList(edges[i].startnode->loc,edges[i].endnode->loc,list);
      int x,y;
      
      for(y=0;y<Height();y++) for(x=0;x<Width();x++){
	float r,g,b;
	GetPixelRGB(x,y,r,g,b);
	SetPixelSegment(x,y,(r+g+b)/3);
      }

      int label=0;
      for(i=0;i<list.size();i++){
	label ^= 255;
	SetPixelSegment(list[i].x,list[i].y,label);
      }

      return ProcessNextMethod();

    }


    if(!(delta_tot<0)){ // reject the move
      
      n->loc=oldcoord;

      // restore energies;

      CalculateDisplacementEnergy(n);
      CalculateDisplacementEnergy(n->nextnode);
      CalculateDisplacementEnergy(n->prevnode);
      CalculateGradientEnergy(n->prevedge);
      CalculateDirectionEnergy(n->prevedge);
      CalculateDistanceEnergy(n->prevedge);
      CalculateGradientEnergy(n->nextedge);
      CalculateDirectionEnergy(n->nextedge);
      CalculateDistanceEnergy(n->nextedge);
      ChangeContentStatistics(n);
      cout << "Move rejected. globalstats returned to:" << endl
	   << globalstats << ends;

      // BruteForceCalculateContentStatistics();
      
      failcount++;
    }
    else 
      failcount=0; // reset after the succesful move


    cout << "Failcount =" << failcount << endl;

  }

  // int i;
  std::vector<Segmentation::coord_float> list;

  for(i=0;i<nodecount;i++)
    Segmentation::Draw::LinePixelsToList(edges[i].startnode->loc,edges[i].endnode->loc,list);
  int x,y;

  for(y=0;y<Height();y++) for(x=0;x<Width();x++){
    float r,g,b;
    GetPixelRGB(x,y,r,g,b);
    SetPixelSegment(x,y,(r+g+b)/3);
  }

  int label=0;
  for(i=0;i<list.size();i++){
    label ^= 255;
    SetPixelSegment(list[i].x,list[i].y,label);
  }

  //  int x,y;

//  for(y=0;y<Height();y++) for(x=0;x<Width();x++)
//     SetPixelSegment(x,y,255*smoothgrad[x+y*Width()]);
  
  return true;

}
///////////////////////////////////////////////////////////////////////////
bool Snake::TestProcess()
{
  int maxfail=30000;
  int max_displacement=5;
  int round_ctr=0;

  //   cout << "In Snake::DoProcess" << endl;

  CalculateGradients();
  CalculateSums();

  //  cout << "Gradients calculated." << endl;

  //TestInitSnake();
  InitSnake();
  //  BruteForceCalculateContentStatistics();
  CalculateAllContentStatistics();

  cout << "Initial statistics: " << endl << globalstats << endl;

  if(Verbose()>1)
    DumpDebugData();

  ShowLocs();
  ShowNodes();
  ShowEdges();

  //nodes[1].loc=coord(21,43);
  
  // ChangeContentStatistics(nodes+1);

  ShowLocs();
  ShowNodes();
  ShowEdges();
  
  cout << "Changed statistics: " << endl << globalstats << endl;

  CalculateAllContentStatistics();
  cout << "Recalculated statistics: " << endl << globalstats << endl;

  //  return true;

  // TestIntersectionTesting();

  cout << "Snake initialised." << endl;

  //  return true;

  int i;
 //  for(i=0;i<nodecount;i++)
//     cout << "&nodes["<<i<<"]="<< nodes + i <<" "<<"loc:"<<nodes[i].loc
// 	 << "   &edges["<<i<<"]= " << edges+i << endl;

  int failcount=0;
  while( failcount < maxfail){

    int node;
    do {
      node = nodecount*rand()/RAND_MAX;
    } while (node>=nodecount);  

    cout << "round #"<<round_ctr++<<": selected node " <<node<< endl;

    SnakeNode *n =nodes+node;

    //     cout << "n: " << n << endl
    //	 << "nextnode: " << n->nextnode << endl
    //	 << "prevnode: " << n->prevnode << endl
// 	 << "nextedge: " << n->nextedge << endl
//          << "prevedge: " << n->prevedge << endl;
    
    double old_e_disp = n->displacement_energy 
      + n->prevnode->displacement_energy
      + n->nextnode->displacement_energy;
    double old_e_g = n->prevedge->gradient_energy
      + n->nextedge->gradient_energy;
    double old_e_dir=n->prevedge->direction_energy
      + n->nextedge->direction_energy;
    double old_e_dist= n->prevedge->distance_energy
      + n->nextedge->distance_energy;


    //BruteForceCalculateContentStatistics();

    //globalstats = bruteforcestats;
    // cout << globalstats << endl;


    double old_e_c = CalculateContentEnergy();

    Segmentation::coord oldcoord(n->loc);
    
    while(n->loc.x == oldcoord.x && n->loc.y==oldcoord.y){
      n->Displace(max_displacement);
      cout <<"trying location ("<<n->loc<<")"<<endl;
    }

    //    cout << "Done displacement" << endl;

    if(OutOfBounds(n->loc) || !CheckIntersections(n)){
      n->loc=oldcoord;
      cout << "Failed geometry check." << endl;
      // this is not counted as a failed trial move
      continue;
    }

    //    cout << "Geometric checks ready." << endl;

    CalculateDisplacementEnergy(n);
    CalculateDisplacementEnergy(n->nextnode);
    CalculateDisplacementEnergy(n->prevnode);
    CalculateGradientEnergy(n->prevedge);
    CalculateDirectionEnergy(n->prevedge);
    CalculateDistanceEnergy(n->prevedge);
    CalculateGradientEnergy(n->nextedge);
    CalculateDirectionEnergy(n->nextedge);
    CalculateDistanceEnergy(n->nextedge);
    
    cout << " changing content statistics." << endl;

    ChangeContentStatistics(n);

    // cout << " replacing content statistics: " << endl << globalstats 
    // 	 << "with ";       

    // BruteForceCalculateContentStatistics();
//     cout <<"globalstats: " << globalstats << endl;

    // globalstats -= bruteforcestats;

   //   if(globalstats != bruteforcestats){
//         cout << "Warning: shortcut statistics calculation does not match brute force calculation." << endl<<"Brute force stats: " << endl << bruteforcestats
//       << endl;
    //     }
    //  cout << "difference: " << globalstats << endl;
    //}

    // globalstats = bruteforcestats;

    //    cout <<"changed." << endl;

    double new_e_disp = n->displacement_energy 
      +n->prevnode->displacement_energy
      +n->nextnode->displacement_energy;
    double new_e_g = n->prevedge->gradient_energy
      + n->nextedge->gradient_energy;
    double new_e_dir=n->prevedge->direction_energy
      + n->nextedge->direction_energy;
    double new_e_dist= n->prevedge->distance_energy
      + n->nextedge->distance_energy;
    double new_e_c = CalculateContentEnergy();

    double delta_disp = new_e_disp - old_e_disp;
    double delta_g = new_e_g - old_e_g;
    double delta_dir = new_e_dir - old_e_dir;
    double delta_dist = new_e_dist - old_e_dist;
    double delta_c = new_e_c - old_e_c;

    double delta_tot = mul[0]*delta_disp + mul[1]*delta_g+
      mul[2]*delta_dir + mul[3]*delta_dist +mul[4]*delta_c;
    
 //    cout << "comparing energies: (new-old=delta) weighted" << endl;
//     cout << "Displacement: " << new_e_disp <<"-"<< old_e_disp << "="
// 	 << delta_disp << " w:" << mul[0]*delta_disp << endl;

//     cout << "Gradient: " << new_e_g <<"-"<< old_e_g << "="
// 	 << delta_g << " w:" << mul[1]*delta_g << endl;

//     cout << "Direction: " << new_e_dir <<"-"<< old_e_dir << "="
// 	 << delta_dir << " w:" << mul[2]*delta_dir << endl;

//     cout << "Distance: " << new_e_dist <<"-"<< old_e_dist << "="
// 	 << delta_dist << " w:" << mul[3]*delta_dist << endl;
  
//     cout << "Content: " << new_e_c <<"-"<< old_e_c << "="
// 	 << delta_c << " w:" << mul[4]*delta_c << endl;

//     cout << "Total delta: " << delta_tot << endl;

    if(isnan(delta_tot)){
      // probably got a NaN as result
      cout << "Abnormal energy delta." << endl;

      ShowLocs();

      cout << "old_loc: " << oldcoord << endl;
     
      BruteForceCalculateContentStatistics();

      std::vector<Segmentation::coord> list;

      for(i=0;i<nodecount;i++)
	Segmentation::Draw::LinePixelsToList(edges[i].startnode->loc,edges[i].endnode->loc,list);
      int x,y;
      
      for(y=0;y<Height();y++) for(x=0;x<Width();x++){
	float r,g,b;
	GetPixelRGB(x,y,r,g,b);
	SetPixelSegment(x,y,(r+g+b)/3);
      }

      int label=0;
      for(i=0;i<list.size();i++){
	label ^= 255;
	SetPixelSegment(list[i].x,list[i].y,label);
      }

      return ProcessNextMethod();

    }


    if(!(delta_tot<0)){ // reject the move
      
      n->loc=oldcoord;

      // restore energies;

      CalculateDisplacementEnergy(n);
      CalculateDisplacementEnergy(n->nextnode);
      CalculateDisplacementEnergy(n->prevnode);
      CalculateGradientEnergy(n->prevedge);
      CalculateDirectionEnergy(n->prevedge);
      CalculateDistanceEnergy(n->prevedge);
      CalculateGradientEnergy(n->nextedge);
      CalculateDirectionEnergy(n->nextedge);
      CalculateDistanceEnergy(n->nextedge);
      ChangeContentStatistics(n);
      cout << "Move rejected. globalstats returned to:" << endl
	   << globalstats << ends;

      // BruteForceCalculateContentStatistics();
      
      failcount++;
    }
    else 
      failcount=0; // reset after the succesful move


    cout << "Failcount =" << failcount << endl;

  }

  // int i;
  std::vector<Segmentation::coord_float> list;

  for(i=0;i<nodecount;i++)
    Segmentation::Draw::LinePixelsToList(edges[i].startnode->loc,edges[i].endnode->loc,list);
  int x,y;

  for(y=0;y<Height();y++) for(x=0;x<Width();x++){
    float r,g,b;
    GetPixelRGB(x,y,r,g,b);
    SetPixelSegment(x,y,(r+g+b)/3);
  }

  int label=0;
  for(i=0;i<list.size();i++){
    label ^= 255;
    SetPixelSegment(list[i].x,list[i].y,label);
  }

  //  int x,y;

//  for(y=0;y<Height();y++) for(x=0;x<Width();x++)
//     SetPixelSegment(x,y,255*smoothgrad[x+y*Width()]);
  
  return true;

}

///////////////////////////////////////////////////////////////////////////

bool Snake::PreProcess()
{
  grad_x=new float[Width()*Height()];
  grad_y=new float[Width()*Height()];
  smoothgrad=new float[Width()*Height()];
  grad_magn=new float[Width()*Height()];
  cumul_sum_r=new int[Width()*Height()];
  cumul_sum_g=new int[Width()*Height()];
  cumul_sum_b=new int[Width()*Height()];
  cumul_sqr_sum_r=new int[Width()*Height()];
  cumul_sqr_sum_g=new int[Width()*Height()];
  cumul_sqr_sum_b=new int[Width()*Height()];
  cumul_pixcount=new int[Width()*Height()];
 
  return true;  
}

///////////////////////////////////////////////////////////////////////////

bool Snake::PostProcess()
{
  delete[] grad_x;
  delete[] grad_y;
  delete[] smoothgrad;
  delete[] grad_magn;
  delete[] cumul_sum_r;
  delete[] cumul_sum_g;
  delete[] cumul_sum_b;
  delete[] cumul_sqr_sum_r; 
  delete[] cumul_sqr_sum_g; 
  delete[] cumul_sqr_sum_b; 

  delete[] cumul_pixcount; 
 
  delete[] nodes;
  delete[] edges;

  return true;
}

///////////////////////////////////////////////////////////////////////////
void Snake::InitSnake()
{
  int sidelength=10;
  
  int top=Height()/4;
  int bottom=Height()*6/8;
  int left=Width()/4;
  int right=Width()*6/8;

  nodecount = 4*sidelength-4;
  
  nodes = new SnakeNode[nodecount];
  edges = new SnakeArc[nodecount];

  nodes[0].prevnode=nodes+nodecount-1;
  nodes[nodecount-1].nextnode=nodes;
  nodes[nodecount-1].nextedge=edges+nodecount-1;
  

  int i;
  for(i=0;i<nodecount-1;i++)
    nodes[i].nextnode = nodes+i+1;

  for(i=1;i<nodecount;i++)
    nodes[i].prevnode = nodes+i-1;

  for(i=0;i<nodecount;i++){
    nodes[i].nextedge=edges+i;
    edges[i].startnode=nodes+i;
    nodes[i].nextnode->prevedge=edges+i;
    nodes[i].prevnode->nextedge->endnode=nodes+i;
  }

  for(i=0;i<sidelength-1;i++){
    nodes[i].loc = coord(left,(((sidelength-1)-i)*top+i*bottom)/(sidelength-1));
    nodes[i+sidelength-1].loc = coord((((sidelength-1)-i)*left+i*right)/(sidelength-1),bottom);
    nodes[i+2*sidelength-2].loc = coord(right,(((sidelength-1)-i)*bottom+i*top)/(sidelength-1));
    nodes[i+3*sidelength-3].loc = coord((((sidelength-1)-i)*right+i*left)/(sidelength-1),top);
  }

  for(i=0;i<nodecount;i++){
    CalculateDisplacementEnergy(nodes+i);
    CalculateGradientEnergy(edges+i);
    CalculateDirectionEnergy(edges+i);
    CalculateDistanceEnergy(edges+i);
  }  

}

///////////////////////////////////////////////////////////////////////////

void Snake::TestInitSnake()
{

  int sidelength=8;
  
  int top=20;
  int bottom=40;
  int left=20;
  int right=80;

  nodecount = 28;
  
  nodes = new SnakeNode[nodecount];
  edges = new SnakeArc[nodecount];

  nodes[0].prevnode=nodes+nodecount-1;
  nodes[nodecount-1].nextnode=nodes;
  nodes[nodecount-1].nextedge=edges+nodecount-1;
  

  int i;
  for(i=0;i<nodecount-1;i++)
    nodes[i].nextnode = nodes+i+1;

  for(i=1;i<nodecount;i++)
    nodes[i].prevnode = nodes+i-1;

  for(i=0;i<nodecount;i++){
    nodes[i].nextedge=edges+i;
    edges[i].startnode=nodes+i;
    nodes[i].nextnode->prevedge=edges+i;
    nodes[i].prevnode->nextedge->endnode=nodes+i;
  }

  for(i=0;i<sidelength-1;i++){
    nodes[i].loc = coord(left,(((sidelength-1)-i)*top+i*bottom)/(sidelength-1));
    nodes[i+sidelength-1].loc = coord((((sidelength-1)-i)*left+i*right)/(sidelength-1),bottom);
    nodes[i+2*sidelength-2].loc = coord(right,(((sidelength-1)-i)*bottom+i*top)/(sidelength-1));
    nodes[i+3*sidelength-3].loc = coord((((sidelength-1)-i)*right+i*left)/(sidelength-1),top);
  }

  for(i=0;i<nodecount;i++){
    CalculateDisplacementEnergy(nodes+i);
    CalculateGradientEnergy(edges+i);
    CalculateDirectionEnergy(edges+i);
    CalculateDistanceEnergy(edges+i);
  }  

} 

///////////////////////////////////////////////////////////////////////////
void Snake::CalculateGradients()
{
  int x,y;
  float r1,g1,b1,r2,g2,b2;

  int w=Width();
  int h=Height();

  
  for(y=0;y<h;y++){
    GetPixelRGB(0,y,r1,g1,b1);
    GetPixelRGB(1,y,r2,g2,b2);
    grad_x[y*w]=((r2-r1)*(r2-r1)+(g2-g1)*(g2-g1)+(b2-b1)*(b2-b1));

    for(x=1;x<w-1;x++){
      GetPixelRGB(x-1,y,r1,g1,b1);
      GetPixelRGB(x+1,y,r2,g2,b2);
      grad_x[x+y*w]=((r2-r1)*(r2-r1)+(g2-g1)*(g2-g1)+(b2-b1)*(b2-b1))/4;
    }

    GetPixelRGB(w-1,y,r1,g1,b1);
    GetPixelRGB(w-2,y,r2,g2,b2);
    grad_x[w-1+y*w]=((r2-r1)*(r2-r1)+(g2-g1)*(g2-g1)+(b2-b1)*(b2-b1));

  }

  for(x=0;x<w;x++){
    GetPixelRGB(x,0,r1,g1,b1);
    GetPixelRGB(x,1,r2,g2,b2);
    grad_y[x]=((r2-r1)*(r2-r1)+(g2-g1)*(g2-g1)+(b2-b1)*(b2-b1));

    for(y=1;y<h-1;y++){
      GetPixelRGB(x,y-1,r1,g1,b1);
      GetPixelRGB(x,y+1,r2,g2,b2);
      grad_y[x+y*w]=((r2-r1)*(r2-r1)+(g2-g1)*(g2-g1)+(b2-b1)*(b2-b1))/4;
    }

    GetPixelRGB(x,h-1,r1,g1,b1);
    GetPixelRGB(x,h-2,r2,g2,b2);
    grad_y[x+(h-1)*w]=((r2-r1)*(r2-r1)+(g2-g1)*(g2-g1)+(b2-b1)*(b2-b1));
  }

  float *magn=new float[w*h];

  for(y=0;y<h;y++) for(x=0;x<w;x++){
    int ind=x+y*w;
    grad_magn[ind]=magn[ind]=sqrt(grad_x[ind]+grad_y[ind]); 
  }
  
//   cout << "Forming kernel" << endl;

//   float kernelData[41*41];
//   for(y=0;y<41;y++) for(x=0;x<41;x++) {
//     float r2=(x-20)*(x-20)+(y-20)*(y-20);
//     kernelData[x+41*y]=0.015*exp(-0.015*r2);
//     //  cout << "kernelData[" << x<< "]["<<y<<"]=" << kernelData[x+41*y]<< endl;
//   }

   iflSize size(w,h,1);
   float fillval=0;
   iflPixel fillpixel(iflFloat,1,&fillval);

  ilMemoryImg img(magn,size,iflFloat);
  img.setFill(fillpixel);

  //  ilKernel kernel(iflFloat,kernelData,81,81);

//   cout << "Testing kernel" << endl;
//   for(x=0;x<5;x++) for(y=0;y<5;y++)
//     cout << "Element ("<<x<<","<<y<<"): " << kernel.getElement(x,y) << endl;


  //  ilConvImg conv(&img,&kernel,0.2);

 ilConfig cfg(iflFloat);

  ilGBlurImg conv(&img,1,41,41);

   conv.getTile(0, 0, w, h, smoothgrad,&cfg);


  int i;
  double min=99999999,max=-99999999;


  for(i=0;i<w*h;i++){
    if(smoothgrad[i]<min) min=smoothgrad[i];
    if(smoothgrad[i]>max) max=smoothgrad[i];
  }

  cout << "min: " << min << " max:" << max << endl;

  max -= min;
  if(!max) max=0.5;

  for(i=0;i<w*h;i++){
    smoothgrad[i] -= min;
    smoothgrad[i] /= max;
  }
 
  
} 

///////////////////////////////////////////////////////////////////////////
void Snake::CalculateSums()
{
  imagesum_r=imagesum_g=imagesum_b=0;
  imagesqrsum_r=imagesqrsum_b=imagesqrsum_b=0;

  int x,y;
  float r,g,b;
  int cumul_r,cumul_g,cumul_b,cumul_sqr_r,cumul_sqr_g,cumul_sqr_b;

  for(y=0;y<Height();y++){
    cumul_r=cumul_g=cumul_b=cumul_sqr_r=cumul_sqr_g=cumul_sqr_b=0;
    for(x=0;x<Width();x++){
      GetPixelRGB(x,y,r,g,b);
      cumul_r += r;
      cumul_g += g;
      cumul_b += b;
      cumul_sqr_r += r*r;
      cumul_sqr_g += g*g;
      cumul_sqr_b += b*b;
      cumul_sum_r[x+y*Width()]=cumul_r;
      cumul_sum_g[x+y*Width()]=cumul_g;
      cumul_sum_b[x+y*Width()]=cumul_b;
      cumul_sqr_sum_r[x+y*Width()]=cumul_sqr_r;
      cumul_sqr_sum_g[x+y*Width()]=cumul_sqr_g;
      cumul_sqr_sum_b[x+y*Width()]=cumul_sqr_b;
      cumul_pixcount[x+y*Width()]=x+1;
    }

    imagesum_r += cumul_r;
    imagesum_g += cumul_g;
    imagesum_b += cumul_b;

    imagesqrsum_r += cumul_sqr_r;
    imagesqrsum_g += cumul_sqr_g;
    imagesqrsum_b += cumul_sqr_b;

  }
}

///////////////////////////////////////////////////////////////////////////
void Snake::DumpDebugData()
{
  int x,y;

  for(y=0;y<Height();y++){
    for(x=0;x<Width();x++){
      cout << "pixel ("<<x<<","<<y<<")"<<" ";
      cout << "c_pix="<<cumul_pixcount[x+y*Width()];
      cout << " cs_r="<< cumul_sum_r[x+y*Width()];
      cout << " cs_g="<< cumul_sum_r[x+y*Width()];
      cout << " cs_b="<< cumul_sum_r[x+y*Width()];
      cout << " cs_sqr_r" << cumul_sqr_sum_r[x+y*Width()];
      cout << " cs_sqr_g" << cumul_sqr_sum_g[x+y*Width()];
      cout << " cs_sqr_b" << cumul_sqr_sum_b[x+y*Width()]<<endl;

    }
  }
  
  cout << "image sums: r="<< imagesum_r;
  cout << " g=" << imagesum_g;
  cout << " b=" << imagesum_b;
  cout << " sqr=" << imagesqrsum_r<<" "<< imagesqrsum_g<<" "
       <<imagesqrsum_b<<endl;
}

///////////////////////////////////////////////////////////////////////////
void Snake::ShowLocs()
{
  int i;
  for(i=0;i<nodecount;i++){
	cout << "&nodes["<<i<<"]="<< nodes + i <<" "<<"loc:"<<nodes[i].loc
	     << "   &edges["<<i<<"]= " << edges+i << endl;
	cout << "  startloc: " << edges[i].startnode <<" ("<< edges[i].startnode->loc; 
	cout << ") endloc: " << edges[i].endnode <<" ("
	     << edges[i].endnode->loc << ")" << endl;
  }
      cout << "globalstats: " << globalstats;
}

///////////////////////////////////////////////////////////////////////////
void Snake::ShowNodes()
{
  int i,j;
  for(i=0;i<nodecount;i++){
    cout << "node["<<i<<"]: ("<<nodes[i].loc<<")" << endl;
    cout << " stats: " << nodes[i].stats << endl;
  }
}

///////////////////////////////////////////////////////////////////////////
void Snake::ShowEdges()
{
  int i,j;
  for(i=0;i<nodecount;i++){
    vector<coord> list;
    Draw::LinePixelsToList(edges[i].startnode->loc,edges[i].endnode->loc,list);
    cout << "edge["<<i<<"]: ("<<edges[i].startnode->loc <<")-("
	 << edges[i].endnode->loc<<")" << endl;

    for(j=0;j<list.size();j++)
      cout << " (" << list[j] << ")";
    cout << endl;
    cout << " stats: " << edges[i].stats << endl;

  }
}

///////////////////////////////////////////////////////////////////////////
void Snake::CalculateDisplacementEnergy(SnakeNode *n)
{
  float cx=n->prevnode->loc.x+n->nextnode->loc.x;
  cx /= 2;

  float cy=n->prevnode->loc.y+n->nextnode->loc.y;
  cy /= 2;

  n->displacement_energy=(n->loc.x-cx)*(n->loc.x-cx) 
    + (n->loc.y-cy)*(n->loc.y-cy);

}
///////////////////////////////////////////////////////////////////////////

double Snake::CalculateOverlapEnergy(SnakeArc *e, float *field)
{
  std::vector<Segmentation::coord_float> list;
  
  Segmentation::Draw::LinePixelsToList(e->startnode->loc, 
				       e->endnode->loc, list);
  
  if(list.size()==0) return 0;
  
  double ene=0;
  int w=Width();
  
  int i;
  for(i=0;i<list.size();i++) {
    float fx=list[i].x;
    float fy=list[i].y;

    int ix=fx, iy=fy;
    float fract_x=fx-ix, fract_y=fy-iy;
    int ix2=ix + ((fract_x)?1:0);
    int iy2=iy + ((fract_y)?1:0);

    float v1,v2,v3,v4;

    v1=field[ix+iy*w];
    v2=field[ix2+iy*w];
    v3=field[ix+iy2*w];
    v4=field[ix2+iy2*w];

    // bilinear interpolation
    
    ene += (v2-v1)*fract_x
      + (v1-v2-v3+v4)*fract_x*fract_y
      + (v3-v1)*fract_y+v1;
  }    
  
  ene *= sqrt(double((e->startnode->loc.x-e->endnode->loc.x)
		     *(e->startnode->loc.x-e->endnode->loc.x)
		     +(e->startnode->loc.y-e->endnode->loc.y)
		     *(e->startnode->loc.y-e->endnode->loc.y))) / list.size();

  return ene;
}

///////////////////////////////////////////////////////////////////////////
void Snake::CalculateGradientEnergy(SnakeArc *e)
{
  e->gradient_energy = -CalculateOverlapEnergy(e,grad_magn);
}

///////////////////////////////////////////////////////////////////////////
void Snake::CalculateDirectionEnergy(SnakeArc *e)
{
  e->direction_energy = 0;
}

///////////////////////////////////////////////////////////////////////////
void Snake::CalculateDistanceEnergy(SnakeArc *e)
{
  e->distance_energy= -CalculateOverlapEnergy(e,smoothgrad);
}

///////////////////////////////////////////////////////////////////////////

bool Snake::CheckIntersections(SnakeNode *n)
{
  if(!CheckIntersections(n->prevedge)) return false;
  return CheckIntersections(n->nextedge);
}

///////////////////////////////////////////////////////////////////////////

bool Snake::CheckIntersections(SnakeArc *e)
{
  SnakeNode *current;

  int setcode, setcode_old;

  //  setcode_old=15; // all areas
  setcode_old=GetSetCode(e->endnode->nextnode,e);

  for(current=e->endnode->nextnode;
      current->nextnode->nextedge != e; current = current->nextnode)
    {
      // cout << "current node: " << current << endl;

      setcode=GetSetCode(current->nextnode,e);
      //      cout << "setcode: " << setcode << " old: " << setcode_old << endl;
      if(!(setcode & setcode_old)){
	//	cout << "Testing edge " << current->nextedge << " against " << e<<endl ; 
	if(TestIntersection(e,current->nextedge)){
	 //  if(!TestIntersectionBrute(e,current->nextedge)){
// 	    cout << "NOTE! Intersection reported even if not detected by"
// 		 << "brute force calculation." 
// 		 << e << "("<<e->startnode->loc
// 		 << ")-(" <<e->endnode->loc<<") intersects with " 
// 		 << current->nextedge <<"("<<current->nextedge->startnode->loc
// 		 << ")-(" <<current->nextedge->endnode->loc
// 		 <<")"<< endl; 
// 	  }

	  
// 	   cout << "edge " << e << " intersects with " << current->nextedge 
// 	       << endl; 
	  return false;
	}
	else{
// 	  if(TestIntersectionBrute(e,current->nextedge)){
// 	    cout << "Intersection not reported even if detected by"
// 		 << "brute force calculation." 
// 		 << e << "("<<e->startnode->loc
// 		 << ")-(" <<e->endnode->loc<<") intersects with " 
// 		 << current->nextedge <<"("<<current->nextedge->startnode->loc
// 		 << ")-(" <<current->nextedge->endnode->loc
// 		 <<")"<< endl; 
// 	    exit(3);
// 	  }
	}
      }
      else{
// 	if(TestIntersection(e,current->nextedge)){
// 	  cout << "Intersection occurred even if setcode didn't indicate the"
// 	       <<" possibility. " << e << "("<<e->startnode->loc
// 	       << ")-(" <<e->endnode->loc<<") intersects with " 
//                << current->nextedge <<"("<<current->nextedge->startnode->loc
// 	       << ")-(" <<current->nextedge->endnode->loc
// 	       <<")"<< endl; 
// 	  exit(2);
// 	  return false;
// 	}
      }

      setcode_old = setcode;
    }
  
  return true;

}

///////////////////////////////////////////////////////////////////////////
int Snake::GetSetCode(SnakeNode *n, SnakeArc *e)
{
  // Computes the code describing the spatial relations of the given node
  // to the bounding box of the reference edge
  // bit 0 - right from e
  // bit 1 - up from e
  // bit 2 - left from e
  // bit 3 - down from e
  
//    cout << "Startpoint : " << e->startnode->loc << endl;
//    cout << "Endpoint : " << e->endnode->loc << endl;
//    cout << "New point : " << n->loc << endl;

  int right,up,left,down;

  if(e->startnode->loc.x < e->endnode->loc.x){
    left = e->startnode->loc.x;
    right = e->endnode->loc.x;
  } 
  else{
    right = e->startnode->loc.x;
    left = e->endnode->loc.x;
  }

  if(e->startnode->loc.y < e->endnode->loc.y){
    up = e->startnode->loc.y;
    down = e->endnode->loc.y;
  } 
  else{
    down = e->startnode->loc.y;
    up = e->endnode->loc.y;
  }

//   cout << "(left right up down)= (" << left<<" "<<right<<" "<<up<<" "
//        <<down<<")"<<endl;

  int code=0;

  if(n->loc.x > right) code |= 1;
  if(n->loc.y < up) code |= 2;
  if(n->loc.x < left) code |= 4;
  if(n->loc.y > down) code |= 8;

  return code;

}

///////////////////////////////////////////////////////////////////////////

bool Snake::TestIntersection(SnakeArc *e1, SnakeArc *e2)
{
  Segmentation::coord p1=e1->startnode->loc;
  Segmentation::coord p2=e1->endnode->loc;
  Segmentation::coord p3=e2->startnode->loc;
  Segmentation::coord p4=e2->endnode->loc;

  int x_diff1 = p2.x-p1.x;
  int x_diff2 = p4.x-p3.x;
  int y_diff1 = p2.y-p1.y;
  int y_diff2 = p4.y-p3.y;
  
  int s_den = y_diff2*(p1.x-p3.x)-x_diff2*(p1.y-p3.y);
  int s_nom = x_diff2*y_diff1-y_diff2*x_diff1;

  if(s_nom==0)
    {
    if (s_den!=0) return false;

    // on the same line

    int l2,l3,l4;

    // handle vertical lines as special case
    if(x_diff1==0 && x_diff2 ==0){
      l2 = y_diff1;
      l3 = p3.y-p1.y;
      l4 = p4.y-p1.y;
    }
    else{
      l2 = x_diff1;
      l3 = p3.x-p1.x;
      l4 = p4.x-p1.x;
    }
    
    if(l3 < 0 && l3 < l2 && l4 < 0 && l4 < l2) return false;
    if(l3 > 0 && l3 > l2 && l4 > 0 && l4 > l2) return false;
    return true;
    }

  int r_den = y_diff1*(p3.x-p1.x)-x_diff1*(p3.y-p1.y);
  int r_nom = - s_nom;

  float s=s_den/(float)s_nom;
  float r=r_den/(float)r_nom;

  //  cout << "intersection parameters: s="<<s<<"  r="<<r<<endl;

  if(s>=0 && s <= 1 && r >= 0 && r <= 1)
    return true;
  else
    return false;
}

///////////////////////////////////////////////////////////////////////////

bool Snake::TestIntersectionBrute(SnakeArc *e1, SnakeArc *e2)
{
  Segmentation::coord p1=e1->startnode->loc;
  Segmentation::coord p2=e1->endnode->loc;
  Segmentation::coord p3=e2->startnode->loc;
  Segmentation::coord p4=e2->endnode->loc;

  vector<coord> list1,list2;

  Segmentation::Draw::LinePixelsToList(p1,p2,list1);
  Segmentation::Draw::LinePixelsToList(p3,p4,list1);

  int i;
  int l=Width()*Height();
  bool *table=new bool[l];
  for(i=0;i<l;i++) table[i]=false;
  
  for(i=0;i<list1.size();i++)
    table[list1[i].x+list1[i].y*Width()]=true;

  bool ret=false;

  for(i=0;i<list2.size();i++)
    if(table[list2[i].x+list2[i].y*Width()]){
      ret=true;
      break;
    }
  delete table;
  return ret;

}


///////////////////////////////////////////////////////////////////////////
void Snake::CalculateAllContentStatistics()
{
  std::vector<coord> list1,list2;
  std::vector<coord> *prevlist,*currentlist, *tmplist;
  SnakeNode *node=nodes;

  Segmentation::Draw::LinePixelsToList(node->prevnode->loc,node->loc,list1);
  
  currentlist = &list1;
  prevlist=&list2;

  globalstats.Zero();

  do{
    tmplist=prevlist;
    prevlist=currentlist;
    currentlist=tmplist;

    currentlist->clear();
    Segmentation::Draw::LinePixelsToList(node->loc,node->nextnode->loc,*currentlist);

    CalculateContentStatistics(node->nextedge,*currentlist);
    globalstats += node->nextedge->stats;

    coord pc((*prevlist)[prevlist->size()-2]),nc((*currentlist)[1]);

    //    cout << "Processing node (" << node->loc<<")"<<endl;
    // cout << " pc=("<<pc<<") nc=("<<nc<<")"<<endl;

    CalculateContentStatistics(node,pc,nc);
    globalstats += node->stats;

    node=node->nextnode;

  } while(node != nodes);

//  cout << "Content staistics calculated" << endl;
//   cout << "global statistics: " << endl;
//   cout << "  sum=" << globalstats.sum << endl;
//   cout << "  sqr_sum=" << globalstats.sqr_sum << endl;
//   cout << "  pixelcount=" << globalstats.pixelcount << endl;

}

///////////////////////////////////////////////////////////////////////////
void Snake::BruteForceCalculateContentStatistics()
{
  std::vector<coord> list1;
  SnakeNode *node=nodes;
  float r,g,b;

  Segmentation::Draw::LinePixelsToList(node->prevnode->loc,node->loc,list1);
  
  bruteforcestats.Zero();

  do{
      Segmentation::Draw::LinePixelsToList(node->loc,node->nextnode->loc,
					   list1);
      node=node->nextnode;
  } while(node != nodes);

  // cout << "BruteForce: lines are in list."<< endl;

  int *mark=new int[Width()*Height()];

  int l,x,y;
  for(l=0;l<Width()*Height();l++) mark[l]=0;

 //  cout << "zeroed all marks"   << endl;

  // mark the borders

  for(x=0;x<Width();x++){
    mark[x]=2;
    mark[x+Width()*(Height()-1)]=2;
  }

  // cout <<"vertical borders done" << endl;

  for(y=0;y<Height();y++){
    mark[y*Width()]=2;
    mark[y*Width()+(Width()-1)]=2;
  }
  
  for(l=0;l<list1.size();l++){
    // cout << "Setting border pixel" << endl;
    mark[list1[l].x+list1[l].y*Width()]=1;
  }
  
 //  cout << "mark array prepared." << endl;

  bool changed=true;
  int fill_ctr=0;

  while(changed){
    changed=false;
    for(y=1;y<Height()-1;y++) for(x=1;x<Width()-1;x++){
      int ind=y*Width()+x;
      if(mark[ind]) continue;
      if(mark[ind-Width()]==2 ||
	 mark[ind-1]==2 ||
	 mark[ind+1]==2 ||
	 mark[ind+Width()]==2){
	mark[ind]=2;
	//cout << "dofill" << endl;
	fill_ctr++;
	changed=true;
      }
    }
  }

  int ind;

  cout << "filling done, fill_ctr=" << fill_ctr << endl;

  for(y=0;y<Height();y++) for(x=0;x<Width();x++)
    if(mark[ind=x+y*Width()]!=2){
      GetPixelRGB(x,y,r,g,b);
      bruteforcestats.pixelcount++;

      bruteforcestats.sum_r += cumul_sum_r[ind];
      bruteforcestats.sum_g += cumul_sum_g[ind];
      bruteforcestats.sum_b += cumul_sum_b[ind];
      
      bruteforcestats.sqr_sum_r += cumul_sqr_sum_r[ind];
      bruteforcestats.sqr_sum_g += cumul_sqr_sum_g[ind];
      bruteforcestats.sqr_sum_b += cumul_sqr_sum_b[ind];
      if(x>0){
	bruteforcestats.sum_r -= cumul_sum_r[ind-1];
	bruteforcestats.sum_g -= cumul_sum_g[ind-1];
	bruteforcestats.sum_b -= cumul_sum_b[ind-1];
      
	bruteforcestats.sqr_sum_r -= cumul_sqr_sum_r[ind-1];
	bruteforcestats.sqr_sum_g -= cumul_sqr_sum_g[ind-1];
	bruteforcestats.sqr_sum_b -= cumul_sqr_sum_b[ind-1];
      }
    }
  
   cout << "Brute force stats: " << endl << bruteforcestats << endl;
  
}

///////////////////////////////////////////////////////////////////////////
float Snake::CalculateContentEnergy()
{

 //  cout << "In Snake::CalculateContentEnergy()" << endl;
//   cout << "global statistics: " << endl;
//   cout << globalstats;

  // calculates negative of log likelihood for
  // gaussian distribution of snake content
  // rgb channels (which are assumed independent)

  int N_t = globalstats.pixelcount;
  int N_b = Width()*Height()-N_t;

//   cout << "Pixelcounts: N_t="<<N_t<<" N_b="<<N_b<<endl;
//   cout << "Image sums: (r,g,b)=("<<imagesum_r <<","<<imagesum_g <<","
//        <<imagesum_b<<")" << endl
//        << "sqrsum=("<<imagesqrsum_r<<","<<imagesqrsum_g<<","
//           << imagesqrsum_b<<")"<<endl;

  float theta_t_r = ((float)globalstats.sum_r) /N_t;
  theta_t_r *= -theta_t_r;
  theta_t_r += ((float)globalstats.sqr_sum_r) /N_t; 


  float theta_t_g = ((float)globalstats.sum_g) /N_t;
  theta_t_g *= -theta_t_g;
  theta_t_g += ((float)globalstats.sqr_sum_g) /N_t; 

  float theta_t_b = ((float)globalstats.sum_b) /N_t;
  theta_t_b *= -theta_t_b;
  theta_t_b += ((float)globalstats.sqr_sum_b) /N_t; 

  float theta_b_r = ((float)(imagesum_r-globalstats.sum_r)) /N_b;
  theta_b_r *= -theta_b_r;
  theta_b_r += ((float)(imagesqrsum_r-globalstats.sqr_sum_r)) /N_b;

  float theta_b_g = ((float)(imagesum_g-globalstats.sum_g)) /N_b;
  theta_b_g *= -theta_b_g;
  theta_b_g += ((float)(imagesqrsum_g-globalstats.sqr_sum_g)) /N_b;

  float theta_b_b = ((float)(imagesum_b-globalstats.sum_b)) /N_b;
  theta_b_b *= -theta_b_b;
  theta_b_b += ((float)(imagesqrsum_b-globalstats.sqr_sum_b)) /N_b;

 //  cout << " theta_t_r=" <<theta_t_r
//        << " theta_t_g=" <<theta_t_g
//        << " theta_t_b=" <<theta_t_b
//        << " theta_b_r=" <<theta_b_r
//        << " theta_b_g=" <<theta_b_g
//        << " theta_b_b=" <<theta_b_b << endl;

  contentenergy = N_t*(log(theta_t_r+SMALL_NUMBER)+log(theta_t_g+SMALL_NUMBER)+log(theta_t_b+SMALL_NUMBER)) + 
    N_b*(log(theta_b_r+SMALL_NUMBER)+log(theta_t_g+SMALL_NUMBER)+log(theta_t_b+SMALL_NUMBER));

  return contentenergy;

}

///////////////////////////////////////////////////////////////////////////

void Snake::ChangeContentStatistics(SnakeNode *moved)
{
 //   cout << "Before : nextedge->stats: " << moved->nextedge->stats << endl
//        <<"prevedge->stats: " << moved->prevedge->stats << endl
//        <<"prevnode->stats: " << moved->prevnode->stats << endl
//        <<"moved->stats: " << moved->stats << endl
//        <<"nextnode->stats: " << moved->nextnode->stats << endl;

  globalstats -= moved->prevedge->stats;
  globalstats -= moved->nextedge->stats;

  globalstats -= moved->prevnode->stats;
  globalstats -= moved->stats;
  globalstats -= moved->nextnode->stats;

  std::vector<coord> list[4];

  Segmentation::Draw::LinePixelsToList(moved->prevnode->prevnode->loc,moved->prevnode->loc,list[0]);
  Segmentation::Draw::LinePixelsToList(moved->prevnode->loc,moved->loc,list[1]);

  Segmentation::Draw::LinePixelsToList(moved->loc,moved->nextnode->loc,list[2]);
  Segmentation::Draw::LinePixelsToList(moved->nextnode->loc,moved->nextnode->nextnode->loc,
		   list[3]);

  coord pc(list[0][list[0].size()-2]),nc(list[1][1]);
  CalculateContentStatistics(moved->prevnode,pc,nc);
  CalculateContentStatistics(moved->prevedge,list[1]);
  pc=coord(list[1][list[1].size()-2]);nc=coord(list[2][1]);
  CalculateContentStatistics(moved,pc,nc);
  CalculateContentStatistics(moved->nextedge,list[2]);
  pc=coord(list[2][list[2].size()-2]);nc=coord(list[3][1]);
  CalculateContentStatistics(moved->nextnode,pc,nc);

 //  cout << "After : nextedge->stats: " << moved->nextedge->stats << endl
//        <<"prevedge->stats: " << moved->prevedge->stats << endl
//        <<"prevnode->stats: " << moved->prevnode->stats << endl
//        <<"moved->stats: " << moved->stats << endl
//        <<"nextnode->stats: " << moved->nextnode->stats << endl;

  globalstats += moved->prevedge->stats;
  globalstats += moved->nextedge->stats;

  globalstats += moved->prevnode->stats;
  globalstats += moved->stats;
  globalstats += moved->nextnode->stats;

} 

///////////////////////////////////////////////////////////////////////////

void Snake::CalculateContentStatistics(SnakeArc *e, 
				       std::vector<coord> &pixellist)
{
  // determine whether this edge is the minimum x edge, 
  // maximum x edge or horisontal

  coord p1=e->startnode->loc, p2=e->endnode->loc;
  coord delta(p2.x-p1.x,-p2.y+p1.y);

  // here normal right handed cartesian coordinates are used
  // in determination of the octant instead of the coords
  // normally used in computers
  // => delta.y is negated
  

  int edgecode,pixelcode;

  // edgecode determines how the pixels along the edge contribute to the
  // statistics, default is with code zero (=don't contribute) 

  // 1 - pixel code is 1 unless y coord. of next pixel is same as current 
  // 2 - pixel code is 1 in any case
  // 3 - pixel code is 1 unless  y coord. of prev. pixel is same as current

  // 5 - pixel code is -1 unless y coord. of next pixel is same as current 
  // 6 - pixel code is -1 in any case
  // 7 - pixel code is -1 unless  y coord. of prev. pixel is same as current

  e->stats.Zero();

  if(delta.y==0){ // horisontal
    return;
  }
  else if(delta.y>0){
    if(delta.y < delta.x) // first octant
      edgecode=1;
    else if(delta.y >= -delta.x) // 2nd and 3rd octants
      edgecode=2;
    else // 4th octant
      edgecode=3;
  }
  else{
    if(delta.y > delta.x) // 5th octant
      edgecode=5;
    else if( -delta.y >= delta.x) // 6th and 7th octants
      edgecode=6;
    else // 8th octant
      edgecode=7;
  }
      
  if(edgecode<=3) pixelcode=1;
  else pixelcode=-1;

  int index;
  for(index=1;index<pixellist.size()-1;index++)
    {
      if(edgecode&1){ // conditional codes 1,3,5,7
	if(edgecode&2){ // test previous pixel
	  if(pixellist[index-1].y==pixellist[index].y)
	    continue;
	}
	else{
	  if(pixellist[index+1].y==pixellist[index].y)
	    continue;
	}
      } // conditions now fulfilled

      int ind;

      if(pixelcode==1){
	ind=pixellist[index].x+pixellist[index].y*Width();
	e->stats.sum_r+=cumul_sum_r[ind];
	e->stats.sum_g+=cumul_sum_g[ind];
	e->stats.sum_b+=cumul_sum_b[ind];
	e->stats.sqr_sum_r+=cumul_sqr_sum_r[ind];
	e->stats.sqr_sum_g+=cumul_sqr_sum_g[ind];
	e->stats.sqr_sum_b+=cumul_sqr_sum_b[ind];
	e->stats.pixelcount+=cumul_pixcount[ind];
      }
      else{
	if(pixellist[index].x==0)
	  continue;
	ind=pixellist[index].x-1+pixellist[index].y*Width();
	e->stats.sum_r-=cumul_sum_r[ind];
	e->stats.sum_g-=cumul_sum_g[ind];
	e->stats.sum_b-=cumul_sum_b[ind];
	e->stats.sqr_sum_r-=cumul_sqr_sum_r[ind];
	e->stats.sqr_sum_g-=cumul_sqr_sum_g[ind];
	e->stats.sqr_sum_b-=cumul_sqr_sum_b[ind];
	e->stats.pixelcount-=cumul_pixcount[ind];
      }

    } // for
		    
//   cout << "edge stats calculated for edge (" <<
//     e->startnode->loc << ")-("<< e->endnode->loc << "). Edge code=" << edgecode << endl;
  
}

///////////////////////////////////////////////////////////////////////////

void Snake::CalculateContentStatistics(SnakeNode *n, 
				       coord &previous,
				       coord &next)
{
  static int segment_coding[8][8]={{0,1,1,1,1,0,0,0},
                            {0,1,1,1,1,2,0,0},
                            {0,1,1,1,1,2,2,0},
                  	    {0,1,1,1,1,2,2,2},
	                    {-1,0,0,0,0,-1,-1,-1},
	                    {-1,2,0,0,0,-1,-1,-1},
	                    {-1,2,2,0,0,-1,-1,-1},
		            {-1,2,2,2,0,-1,-1,-1}};

  int code=segment_coding[Segmentation::Draw::FreemanCodeFromPoints(previous,n->loc)]
    [Segmentation::Draw::FreemanCodeFromPoints(n->loc,next)];
  int ind;

  switch(code){
  case 0:
    n->stats.Zero();
    break;

  case -1: 
    if(n->loc.x==0){
      n->stats.Zero();
      break;
    }
    ind=n->loc.x-1+n->loc.y*Width();
    n->stats.sum_r=-cumul_sum_r[ind];
    n->stats.sum_g=-cumul_sum_g[ind];
    n->stats.sum_b=-cumul_sum_b[ind];
    n->stats.sqr_sum_r=-cumul_sqr_sum_r[ind];
    n->stats.sqr_sum_g=-cumul_sqr_sum_g[ind];
    n->stats.sqr_sum_b=-cumul_sqr_sum_b[ind];
    n->stats.pixelcount=-cumul_pixcount[ind];
    break;
  case 2:
    ind=n->loc.x-1+n->loc.y*Width();
    if(n->loc.x==0){
      n->stats.Zero();
    }
    else{
      n->stats.sum_r=-cumul_sum_r[ind];
      n->stats.sum_g=-cumul_sum_g[ind];
      n->stats.sum_b=-cumul_sum_b[ind];
      n->stats.sqr_sum_r=-cumul_sqr_sum_r[ind];
      n->stats.sqr_sum_g=-cumul_sqr_sum_g[ind];
      n->stats.sqr_sum_b=-cumul_sqr_sum_b[ind];
      n->stats.pixelcount=-cumul_pixcount[ind];
    }

    ind++;
    n->stats.sum_r+=cumul_sum_r[ind];
    n->stats.sum_g+=cumul_sum_g[ind];
    n->stats.sum_b+=cumul_sum_b[ind];
    n->stats.sqr_sum_r+=cumul_sqr_sum_r[ind];
    n->stats.sqr_sum_g+=cumul_sqr_sum_g[ind];
    n->stats.sqr_sum_b+=cumul_sqr_sum_b[ind];
    n->stats.pixelcount+=cumul_pixcount[ind];
    break;

  case 1:
    ind=n->loc.x+n->loc.y*Width();
    n->stats.sum_r=cumul_sum_r[ind];
    n->stats.sum_g=cumul_sum_g[ind];
    n->stats.sum_b=cumul_sum_b[ind];
    n->stats.sqr_sum_r=cumul_sqr_sum_r[ind];
    n->stats.sqr_sum_g=cumul_sqr_sum_g[ind];
    n->stats.sqr_sum_b=cumul_sqr_sum_b[ind];
    n->stats.pixelcount=cumul_pixcount[ind];
    break;
  } // switch(code)

}


///////////////////////////////////////////////////////////////////////////
void Snake::TestIntersectionTesting()
{
  int i;

  SnakeArc  e1,e2;
  SnakeNode e1_1,e1_2,e2_1,e2_2;

  e1.startnode = &e1_1; e1.endnode=&e1_2;
  e2.startnode = &e2_1; e2.endnode=&e2_2;

  e1_1.loc=coord(50,20); e1_2.loc=coord(50,40);

  for(i=0;i<1000;i++){
    e2_1.loc = coord(100.0/RAND_MAX*rand(),80.0/RAND_MAX*rand());
    e2_2.loc = coord(100.0/RAND_MAX*rand(),80.0/RAND_MAX*rand());

    cout << "Testing lines ("<<e1_1.loc<<")-("<<e1_2.loc<<") and ("
	 <<e2_1.loc<<")-("<<e2_2.loc<<")"<<endl;
    if(TestIntersection(&e1,&e2)) cout << "INTERSECTION !"<< endl;
    else cout << "no intersection" << endl;
  }
}

///////////////////////////////////////////////////////////////////////////

Snake::SnakeNode::SnakeNode()
{
  
}

///////////////////////////////////////////////////////////////////////////

void Snake::SnakeNode::Displace(int max_d)
{
  int delta;
  do{
   delta = (2*max_d+1.0)*(-0.5+rand()/RAND_MAX);
  } while (abs(delta)>max_d);

  loc.x += delta;
  do{
   delta = (2*max_d+1.0)*(-0.5+rand()/RAND_MAX);
  } while (abs(delta)>max_d);

  loc.y += delta;
}

///////////////////////////////////////////////////////////////////////////
Snake::SnakeArc::SnakeArc()
{
}

///////////////////////////////////////////////////////////////////////////

