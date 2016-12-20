#include <FilterbyBoundingbox.h>


using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;

namespace picsom {
  static const string vcid =
    "@(#)$Id: FilterbyBoundingbox.C,v 1.2 2009/08/05 08:57:04 jorma Exp $";

  static FilterbyBoundingbox list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  FilterbyBoundingbox::FilterbyBoundingbox() {
    byHierarchy=false;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  FilterbyBoundingbox::~FilterbyBoundingbox() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *FilterbyBoundingbox::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void FilterbyBoundingbox::UsageInfo(ostream& os) const { 
    os << "FilterbyBoundingbox :" << endl
       << "  options : " << endl
       << "  -i input file" << endl
       << "  -t consider only combinations in the region hierarchy" << endl;

  }

  /////////////////////////////////////////////////////////////////////////////

  int FilterbyBoundingbox::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;



    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {

      case 'i':
      if (argc<3){
	EchoError("Not enough commandline arguments for switch -i");
	break;
      }

      option.option = "-i";
      option.addArgument(argv[1]);

      input_pattern=argv[1];
      argc--;
      argv++;
      break;
	
      case 't':
	option.option = "-t";
	byHierarchy = true;
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
  
  bool FilterbyBoundingbox::Process() {

    string filename = *GetInputImageName();
    if(filename.empty())
      throw("FilterbyBoundingbox: empty image file name.");
    
   
    segmentfile other("",getImg()->FormInputFileName(input_pattern,
						     filename,""));

    SegmentationResultList *result=other.readFrameResultsFromXML("PASgt","box");

    if(Verbose()>1) 
      cout << "Found "<<result->size()<<" bounding boxes" << endl;

    int nr=0;

    list<pair<string,float> > best_results;

    SegmentationResultList::const_iterator it;
    for(it=result->begin();it !=result->end();it++,nr++){

      //      cout << "Found result name=" << it->name
      //	   << " type="<< it->type
      //	   << " value=\""<< it->value << "\""<<endl;

      PredefinedTypes::Box bb(it->value);

      // cout << "Parsed bbox" << endl;
      
      if(Verbose()>1){
	string s;
	bb.write(s);
	cout << "Filtering by bounding box "<<nr<<": "<<s<<endl;
      }

      coord ul(bb.ulx,bb.uly),br(bb.brx,bb.bry);

      if(ul.x<0) ul.x=0;
      if(ul.y<0) ul.y=0;
      if(br.x>getImg()->getWidth()-1) br.x=getImg()->getWidth()-1;
      if(br.y>getImg()->getHeight()-1) br.y=getImg()->getHeight()-1;

      vector<int> *lbls_bbox=(byHierarchy)?getImg()->getUsedLabels():
	getImg()->getUsedLabels(ul,br);
      size_t n_bbox=lbls_bbox->size();

      map<int,int> lbl_to_idx;
      for(size_t i=0;i<n_bbox;i++)
	lbl_to_idx[(*lbls_bbox)[i]]=i;

      vector<int> areas(n_bbox,0), areas_common(n_bbox,0);
      
      // get areas of all segments overlapping the bb
      // (or all areas in case of byHierarchy)

      int x,y,w=getImg()->getWidth(),h=getImg()->getHeight();
      for(x=0;x<w;x++) for(y=0;y<h;y++){
	int s;
	getImg()->get_pixel_segment(x,y,s);
	if(lbl_to_idx.count(s)>0){
	  areas[lbl_to_idx[s]]++;

	  if(x>=bb.ulx&&x<=bb.brx&&y>=bb.uly&&y<=bb.bry)
	    areas_common[lbl_to_idx[s]]++;
	}
      }

      if(!byHierarchy){
      // enumerate all possible region combinations;


      unsigned int nrcomb=1<<n_bbox; // plus 1, we ecxlude the all zeros combination

      if(Verbose()>1)
	getImg()->getUsedLabels(ul,br);cout << "bounding box has " << n_bbox <<" segments -> "<<nrcomb
	     <<" combinations" << endl;

      if(n_bbox>32)
	throw string("FilterbyBoundingbox: too many regions.");

      list<pair<float,int> > best_foms;
      pair<float,int> zeropair(0,0);
      
      for(int i=0;i<4;i++)
	best_foms.push_back(zeropair);

      unsigned int combnr;

      for(combnr=1;combnr<nrcomb ;combnr++){

	int a_region=0;
	int a_common=0;
	for(size_t i=0;i<n_bbox;i++)
	  if(combnr & (1<<i)){
	    // labels_in_comb.insert((*lbls)[i]);
	    a_region += areas[i];
	    a_common += areas_common[i];
	  }

	int a_bbox=(bb.ulx-bb.brx+1)*(bb.uly-bb.bry+1);
	
	int a_union=a_region+a_bbox-a_common;
	
	float fom=((float)a_common)/a_union;

	list<pair<float,int> >::iterator bit;

// 	cout << "Calculated fom:"<<fom<<endl;
// 	cout << "before best_foms has size " << best_foms.size() << endl;
// 	cout << "and contents" << endl;
// 	for(bit=best_foms.begin();bit!=best_foms.end();bit++)
// 	  cout <<" item: " << bit->first << " "<< bit->second << endl;


	for(bit=best_foms.begin();bit!=best_foms.end();bit++)
	  if(fom>bit->first){
	    pair<float,int> tmppair(fom,combnr);
	    best_foms.insert(bit,tmppair);
	    best_foms.pop_back();
	    break;
	  }

	// cout << "after best_foms has size " << best_foms.size() << endl;
// 	cout << "and contents" << endl;
// 	for(bit=best_foms.begin();bit!=best_foms.end();bit++)
// 	  cout <<" item: " << bit->first << " "<< bit->second << endl;
// 	break;
      }
  if(Verbose()>1)
	cout << "Best combinations corresponding to bounding box "<<nr<<":" 
	     << endl;
      
      list<pair<float,int> >::iterator bit;
      for(bit=best_foms.begin();bit!=best_foms.end();bit++){
	set<int> lblset;
	
	if(Verbose()>1)
	  cout << "   fom="<<bit->first<<" :";
	combnr = bit->second;
	for(size_t i=0;i<n_bbox;i++)
	  if(combnr & (1<<i)){
	    lblset.insert((*lbls_bbox)[i]);
	    if(Verbose()>1) cout << " "<<(*lbls_bbox)[i]; 
	  }
	if(Verbose()>1) cout << endl;
	best_results.push_back(pair<string,float>(formVirtualSegmentLabel(lblset),bit->first));
      }


      }
      else{ // byHierarchy

	list<pair<float,int> > best_foms;
	pair<float,int> zeropair(0,0);
      
	for(int i=0;i<4;i++)
	  best_foms.push_back(zeropair);
	
	vector<set<int> > combs=getImg()->flattenHierarchyToPrimitiveRegions();

	if(Verbose()>1)
	  cout << "found "<<combs.size()<<" combined regions in the hierarchy"
	       << endl;

	unsigned int combnr;
	
	for(combnr=0;combnr<combs.size() ;combnr++){
	  int a_region=0;
	  int a_common=0;
	  
	  set<int>::const_iterator sit;
	  for(sit=combs[combnr].begin();sit != combs[combnr].end();sit++){
	    if(lbl_to_idx.count(*sit)){
	      a_region += areas[lbl_to_idx[*sit]];
	      a_common += areas_common[lbl_to_idx[*sit]];
	    }
	  }
	  int a_bbox=(bb.ulx-bb.brx+1)*(bb.uly-bb.bry+1);
	  
	  int a_union=a_region+a_bbox-a_common;

	  float fom =(a_union!=0)?((float)a_common)/a_union:0;
	    
	  // cout << "for combination "<<formVirtualSegmentLabel(combs[combnr])
// 	       << "a_common="<<a_common
// 	       << " a_region="<<a_region
// 	       << " a_bbox="<<a_bbox
// 	       << " a_union="<<a_union
// 	       << " -> fom" << fom << endl;  

	  list<pair<float,int> >::iterator bit;
	  
	  for(bit=best_foms.begin();bit!=best_foms.end();bit++)
	    if(fom>bit->first){
	      //	      cout << "pushing  pair "<<fom<<","<<combnr<<endl;
	      pair<float,int> tmppair(fom,combnr);
	      best_foms.insert(bit,tmppair);
	      best_foms.pop_back();
	      break;
	    }
	}
       

	if(Verbose()>1)
	  cout << "Best combinations corresponding to bounding box "<<nr<<":" 
	       << endl;
	
	list<pair<float,int> >::iterator bit;
	for(bit=best_foms.begin();bit!=best_foms.end();bit++){
	  if(Verbose()>1){
	    cout << "   fom="<<bit->first<<" :";
	    cout << " combs["<<bit->second<<"]:"<<formVirtualSegmentLabel(combs[bit->second])<<endl; 
	    best_results.push_back(pair<string,float>(formVirtualSegmentLabel(combs[bit->second]),bit->first));
	  }
	  
	}
	
      }

      delete lbls_bbox;

    }

    list<pair<string,float> >::iterator bit;
    map<string,string> resmap;

    for(bit=best_results.begin();bit!=best_results.end();bit++){
      stringstream ss;
      ss <<bit->second;
      if(resmap.count(bit->first)){ // combination already found by some 
	                             // other bb
	resmap[bit->first] += " " + ss.str(); 
      }
      else
	resmap[bit->first]= "PAS_VOC_bbox_overlap " + ss.str(); 
    }

    map<string,string>::iterator mit;

    for(mit=resmap.begin();mit!=resmap.end();mit++){
      SegmentationResult r;
      r.name=mit->first;
      r.type="virtualsegment";
      r.value=mit->second;

      if(Verbose()>1){
	cout << "writing result to xml: " << endl;
	cout << " name=" << r.name <<endl;
	cout << " type=" << r.type <<endl;
	cout << " value=" << r.value <<endl;
      }

      getImg()->writeFrameResultToXML(this,r);
    }

    delete result;

    return ProcessNextMethod();
    }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
