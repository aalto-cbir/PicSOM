#include <AlignmentMarks.h>

static const string vcid="@(#)$Id: AlignmentMarks.C,v 1.15 2009/04/29 14:27:59 vvi Exp $";

namespace picsom{

  static AlignmentMarks list_entry(true);

  ///////////////////////////////////////////////////////////////////////////
  
  AlignmentMarks::AlignmentMarks() {
    
    printIntermediate=false;
    
  }

///////////////////////////////////////////////////////////////////////////
  
  AlignmentMarks::~AlignmentMarks() {

  }

///////////////////////////////////////////////////////////////////////////

  const char *AlignmentMarks::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void AlignmentMarks::UsageInfo(ostream& os) const { 
    os << "AlignmentMarks :" << endl
       << "  options : " << endl
       << "    -p print intermediate results to standard output" << endl;
  }

  ///////////////////////////////////////////////////////////////////////////

  int AlignmentMarks::ProcessOptions(int argc, char** argv) { 

    Option option;
    int argc_old=argc;
    
    while (*argv[0]=='-' && argc>1) {
      option.reset();
      
      switch (argv[0][1]) {

    case 'p':
      option.option= "-p";
      printIntermediate=true;
      break;
    default:
      throw string("AlignmentMarks:processOptions(): unknown option ")+argv[0];
    } // switch


      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 

    } // while
  
    return argc_old-argc;
  }

  ///////////////////////////////////////////////////////////////////////////

  bool AlignmentMarks::Process() {
  
    int maskwidth=10;

    int top,bottom;
    int x1,x2,x3,x4,y1,y2;
    
    top=FindTop();
    if(top==-1) return false;  
    
    bottom=FindBottom();
    if(bottom==-1) return false;
    
    if(printIntermediate){
      cout << "top=" << top<<" bottom="<< bottom << endl;
    }
    
    int *slice=GetVerticalSlice(0,Width(),maskwidth);
    EdgeDetect(slice,Height()+1,maskwidth,y1,y2);
    y2--;
    
    if(y1 <= y2){ top=y1;bottom=y2;}
    
    if(printIntermediate){
      cout << "y1=" << y1<<" y2="<< y2 << endl;
    }

    int limit=top + (int)(0.1*(bottom-top));
    int h=limit-top+1;
    
    slice=GetSlice(top,h,maskwidth);
    
    //if(Verbose()>1){
    //  cout << "Top slice:" << endl;
    //  for(int i=0;i<Width()+2*maskwidth;i++) cout <<i<<": " << (int)slice[i] << endl;
    //}
    
    
    EdgeDetect(slice,Width()+1,maskwidth,x1,x2);
    x2--;
    delete[] slice;
    
    limit=bottom-(int)(0.1*(bottom-top));
    h=bottom-limit+1;
    
    slice=GetSlice(limit,h,maskwidth);
    //if(Verbose()>1){
    //  cout << "Bottom slice:" << endl;
    //  for(int i=0;i<Width()+2*maskwidth;i++) cout <<i<<": " << (int)slice[i] << endl;
    //}
    EdgeDetect(slice,Width()+1,maskwidth,x3,x4);
    x4--;
    delete[] slice;
    
    if(printIntermediate){
      cout <<"x1=" << x1<<" x2="<< x2 << endl;
      cout << "x3=" << x3<<" x4="<< x4 << endl;
    }
    
    // scale the bottom alignment line
    
    double top_factor = 0.87;
    double center=(x3+x4)/2.0;
    double half_len = (x4-x3)*top_factor/2.0;
    x3 = (int)(center-half_len);
    x4 = (int)(center+half_len);
    
    if(printIntermediate){
      cout << "After scaling x3=" << x3<<" x4="<< x4 << endl;
    }
    
    // now write the result
    SegmentationResult result;  
    
    ostringstream *str=new ostringstream;
    (*str) << x1 << " " << top << " " << x2 << " " << top;
  
    result.name = "alignment_marks_top";
    result.type = "line";
    result.value = str->str();
    getImg()->writeFrameResultToXML(this,result);
    delete str;

    str=new ostringstream;
    (*str) << x3 << " " << bottom << " " << x4 << " " << bottom;
  
    result.name = "alignment_marks_bottom";
    result.value = str->str();
    getImg()->writeFrameResultToXML(this,result);
    delete str;
  
  return true;

}

///////////////////////////////////////////////////////////////////////////

int AlignmentMarks::FindTop()
{
  int x,y,s;

  segmentfile *seg=getImg();
  int w=seg->getWidth();
  int h=seg->getHeight();

  for(y=0;y<h;y++)
    for(x=0;x<w;x++){  
      seg->get_pixel_segment(x,y,s);
      if(s) return y;
    }
  return -1;
}

///////////////////////////////////////////////////////////////////////////

int AlignmentMarks::FindBottom()
{
  int x,y,s;

  segmentfile *seg=getImg();
  int w=seg->getWidth();
  int h=seg->getHeight();

  for(y=h-1;y>=0;y--)
    for(x=0;x<w;x++){
      seg->get_pixel_segment(x,y,s);
      if(s) return y;
    }
  
  return -1;
  
}

///////////////////////////////////////////////////////////////////////////

int *AlignmentMarks::GetSlice(int begin, int height,int margin)
{
  segmentfile *seg=getImg();

  int w=seg->getWidth()+2*margin,s;

  int *slice=new int[w];

  int x,y;

  for(x=0;x<w;x++) slice[x]=-height;
  for(y=0;y<height;y++)
    for(x=0;x<seg->getWidth();x++){
      seg->get_pixel_segment(x,y+begin,s);
      if(s) slice[x+margin] += 2;
    }

  if(printIntermediate){
    cout << "Horisontal slice from y=" << begin << " to y=" << begin+height-1 << endl;
    for(x=0;x<seg->getWidth();x++)
      cout << x << " " << slice[x+margin] << endl;
  }
  
  return slice;
}
  
///////////////////////////////////////////////////////////////////////////

int *AlignmentMarks::GetVerticalSlice(int begin, int width,int margin)
{
  segmentfile *seg=getImg();

  int h=seg->getHeight()+2*margin,s;

  int *slice=new int[h];

  int x,y;

  for(y=0;y<h;y++) slice[y]=-width;
  for(y=0;y<seg->getHeight();y++)
    for(x=0;x<width;x++){
      seg->get_pixel_segment(x+begin,y,s);
      if(s) slice[y+margin] += 2;
    }

  if(printIntermediate){
    cout << "Vertical slice from x=" << begin << " to x=" << begin+width-1 << endl;
    for(y=0;y<seg->getHeight();y++)
      cout << y << " " << slice[y+margin] << endl;
  }
  
  return slice;
}
  
///////////////////////////////////////////////////////////////////////////



void AlignmentMarks::EdgeDetect(int *slice,int w,int maskwidth,int &max_pos,int &min_pos)
{
  int convol;
  int x;
  int max,min;

  // calculate directly for 1st position

  convol = 0;

  for(int i=0;i<maskwidth;i++)
    convol += slice[maskwidth+i]-slice[i];

  if(printIntermediate){
    cout << "Edge detect w/ maskwidth=" << maskwidth<<endl;
    cout <<"0 "<< convol << endl;
  }

  max=min=convol;
  max_pos = min_pos=0;

  for(x=1;x<w;x++){
    convol += slice[x-1];
    convol -= 2*slice[x-1+maskwidth];
    convol += slice[x-1+2*maskwidth];
 
    if(convol>max){ max_pos=x; max=convol;}  
    if(convol<min){ min_pos=x; min=convol;}  
    if(printIntermediate){
      cout << x << " "<< convol << endl;
    }
  }

}

///////////////////////////////////////////////////////////////////////////

// bool AlignmentMarks::GetAlignmentMarks(Segmentation &s,
// 				       Segmentation::coord &m1,
// 				       Segmentation::coord &m2,
// 				       Segmentation::coord &m3,
// 				       Segmentation::coord &m4)
// {
//   string dummy;
//   Segmentation::LabeledHash *hash=s.GetDescriptionFields();

//   if(!hash->count("alignment_marks"))
//     return false;

//   istrstream str(((*hash)["alignment_marks"]).c_str());

//   str >> dummy >> m1 >> m2;
//   str >> dummy >> m3 >> m4;

//   return true;

// }	       
///////////////////////////////////////////////////////////////////////////    
    


///////////////////////////////////////////////////////////////////////////

} // namespace picsom


 ///////////////////////////////////////////////////////////////////////////

  // Local Variables:
  // mode: font-lock
  // End:
