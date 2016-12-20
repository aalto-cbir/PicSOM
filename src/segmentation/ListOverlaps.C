#include <ListOverlaps.h>


using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;

namespace picsom {
  static const string vcid="@(#)$Id: ListOverlaps.C,v 1.5 2009/04/30 09:52:57 vvi Exp $";

  static ListOverlaps list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  ListOverlaps::ListOverlaps() {
    //    bb.parse("0,0,0,0");
    showboxes=false;
    segmentareafraction=false;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  ListOverlaps::~ListOverlaps() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *ListOverlaps::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void ListOverlaps::UsageInfo(ostream& os) const { 
    os << "ListOverlaps :" << endl
       << "  options : " << endl
       << "  -b bounding box (default: 0,0,0,0)" << endl
       << "  -a print bounding boxes " << endl
       << "  -s print fraction of segment area inside bounding box " << endl;

  }

  /////////////////////////////////////////////////////////////////////////////

  int ListOverlaps::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;



    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {

      case 'b':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -b");
	break;
      }

      option.option = "-b";
      option.addArgument(argv[1]);

      bb.push_back(PredefinedTypes::Box(argv[1]));
      argc--;
      argv++;
      break;

      case 'a':

      option.option = "-a";
      showboxes=true;
      break;

      case 's':

      option.option = "-s";
      segmentareafraction=true;

      break;

	
      default:
	EchoError(string("unknown option ")+argv[0]);
      } // switch

      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while
    return argc_old-argc;
    
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool ListOverlaps::Process() {

    bool hierarchy=
      (getImg()->accessLastFrameResultNode("","regionhierarchy"))?true:false;

    bool regionsInXML=
      (getImg()->accessLastFrameResultNode("","region"))?true:false;

    if(Verbose()>1){
      cout << "hierarchy="<<hierarchy<<" regionsInXML="<<regionsInXML<<endl;
      for(size_t i=0;i<bb.size();i++){
	string s;
	bb[i].write(s);
	cout << "Filtering by bounding box "<<": "<<s<<endl;
      }
    }

    if(hierarchy || !regionsInXML){

      vector<set<int> > combs;
      if(hierarchy)
	combs=getImg()->flattenHierarchyToPrimitiveRegions(true); // overwrite the bitmap
      

      //coord ul(bb.ulx,bb.uly),br(bb.brx,bb.bry);

      //if(ul.x<0) ul.x=0;
      //if(ul.y<0) ul.y=0;
      //if(br.x>getImg()->getWidth()-1) br.x=getImg()->getWidth()-1;
      //if(br.y>getImg()->getHeight()-1) br.y=getImg()->getHeight()-1;

      map<int,int>  areas,areas_common;
      int a_bbox=0;

      map<int,PredefinedTypes::Box> bboxes;

      // get areas of all the segments in the bitmap

      // collect also bounding boxes

      int x,y,w=getImg()->getWidth(),h=getImg()->getHeight();
      for(x=0;x<w;x++) for(y=0;y<h;y++){
	int s;
	getImg()->get_pixel_segment(x,y,s);
	areas[s]++;

	if(bboxes.count(s)==0){
	  bboxes[s].ulx=bboxes[s].brx=x;
	  bboxes[s].uly=bboxes[s].bry=y;
	}else{
	  bboxes[s].update(x,y);
	}
	for(size_t i=0;i<bb.size();i++){
	  if(bb[i].isInside(coord(x,y))){
	    areas_common[s]++;
	    a_bbox++;
	    break;
	  }
	}
      }
    

      if(!hierarchy){
	map<int,int>::iterator mit;
	for(mit=areas.begin();mit!=areas.end();mit++){
	  
	  cout << "segment "<<mit->first<<":";
	  if(bb.size()==1){

	    // PASoverlap calculated correctly only for a single bounding box

	    int a_region=bboxes[mit->first].area();
	    int a_common=bboxes[mit->first].overlapWith(bb[0]);
	    
	    int a_union=a_region+a_bbox-a_common;
	    float fom=((float)a_common)/a_union;
	    cout << " PASoverlap = "<<fom;
	  }
	  if(segmentareafraction){
	    cout << " overlapfraction=" << 
	      (((float)areas_common[mit->first])/((float)areas[mit->first])); 
	  }
	  if(showboxes){
	    string bstr; bboxes[mit->first].write(bstr);
	    cout <<" bbox: "<<bstr;
	  }
	  cout <<endl;
	} 
      } else{


      // done with the regions in bitmap, now evaluate the measure
      // for composite regions in the hierarchy
      
	for(size_t combnr=0;combnr<combs.size();combnr++){

	  set<int> &lblset=combs[combnr];

	  cout << "segment "<<formVirtualSegmentLabel(lblset)<<":";
	  
	  set<int>::const_iterator sit;
	  PredefinedTypes::Box bBox=bboxes[*combs[combnr].begin()]; 
	  for(sit=combs[combnr].begin();sit != combs[combnr].end();sit++){
	    bBox.update(bboxes[*sit]);
	  }
	  if(bb.size()==1){
	    
	    // PASoverlap calculated correctly only for a single bbox

	    int a_region=bBox.area();
	    int a_common=bBox.overlapWith(bb[0]);

	    int a_union=a_region+a_bbox-a_common;
	    float fom=((float)a_common)/a_union;

	   cout    <<" PASoverlap = "<<fom;
	  }

	  if(segmentareafraction){
	    int a_common=0,a_region=0;
	    for(sit=combs[combnr].begin();sit != combs[combnr].end();sit++){
	      a_common += areas_common[*sit];
	      a_region += areas[*sit];
	    }
	    cout << " overlapfraction=" << 
	      (((float)a_common)/((float)a_region)); 
	  }
	  if(showboxes){
	    string bstr; bBox.write(bstr);
	    cout <<" bbox: "<<bstr;
	  }
	  cout <<endl;
	}
      }

    } else{ // regionsInXML

      SegmentationResultList *rl=
	getImg()->readFrameResultsFromXML("","region");
      for(size_t r=0;r<rl->size();r++){
	Region *rptr = Region::createRegion((*rl)[r].value);
	if(rptr==NULL)
	  throw string("parseAndCreate() region failed");
	coordList l=rptr->listCoordinates();


	cout << "segment "<<r+1<<"("<<(*rl)[r].name
	     <<"):";

	PredefinedTypes::Box bBox=PredefinedTypes::Box::findBoundingBox(l);
	if(bb.size()==1){
	  // PASoverlap calculated correctly only for a single bbox

	  int a_bbox=bb[0].area();
	  int a_region=bBox.area();
	  int a_common=bBox.overlapWith(bb[0]);

	  int a_union=a_region+a_bbox-a_common;
	  float fom=((float)a_common)/a_union;
	  cout <<" PASoverlap = "<<fom;
	}
	if(segmentareafraction){
	  int a_common=0,a_region=0;
	  PredefinedTypes::Box bb_img(0,0,
				      getImg()->getWidth()-1,
				      getImg()->getHeight()-1);
	  for(size_t ii=0;ii<l.size();ii++){
	    if(bb_img.isInside(l[ii])){
	      a_region++;
	      for(size_t i=0;i<bb.size();i++){
		if(bb[i].isInside(l[ii])){
		  a_common++;
		  break;
		}
	      }
	    }
	  }
	  cout << " overlapfraction=" << 
	    (((float)a_common)/((float)a_region)); 
	}
	if(showboxes){
	  string bstr; bBox.write(bstr);
	  cout <<" bbox: "<<bstr;
	}
	cout <<endl;

	delete rptr;

      }
      delete rl;

    } 

    return ProcessNextMethod();
    }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
