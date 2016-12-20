#include <MyMethod.h>


using std::string;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;
using std::set;
using std::vector;
using std::pair;

namespace picsom {
  static const string vcid="@(#)$Id: MyMethod.C,v 1.19 2009/04/29 14:25:22 vvi Exp $";

  static MyMethod list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  MyMethod::MyMethod() {
    includeImages=false;
    showseparate=false;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  MyMethod::~MyMethod() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *MyMethod::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void MyMethod::UsageInfo(ostream& os) const { 
    os << "MyMethod :" << endl
       << "  options : " << endl
       << "  -m myargument" << endl
       << "  -s           outputs separate labeling" << endl;  

  }

  /////////////////////////////////////////////////////////////////////////////

  int MyMethod::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;


    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'm':
	if (argc<3){
	  EchoError("Not enough commandline arguments for switch -m");
	  break;
	}
	
	cout << "processing option" << endl;

	option.option="-m";
	option.addArgument(argv[1]);
	
	argc--;
	argv++;
	break;

      case 's':
	option.option="-s";
	showseparate=true;

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
  
  bool MyMethod::Process() {
    if (Verbose()>1)
      ShowLinks();

    if(true){

      ipList iplist;

      iplist.setHeaderAndParse("x y radius");
      
      vector<float> v;

      v.push_back(30);
      v.push_back(20);
      v.push_back(10);

      iplist.list.push_back(v);
      v.clear();

      v.push_back(30);
      v.push_back(30);
      v.push_back(10);

      iplist.list.push_back(v);
      v.clear();

      v.push_back(30);
      v.push_back(40);
      v.push_back(10);

      iplist.list.push_back(v);
      v.clear();

      getImg()->storeIPListAsFrameResult(this,iplist,"testipdet");

      return ProcessNextMethod();
    }

    int l = (getImg()->getWidth()+getImg()->getHeight())/2;
    
    float avgr = 0, avgg = 0, avgb = 0;
    
    if(!showseparate)
      for (int x=0; x<getImg()->getWidth(); x++)
      for (int y=0; y<getImg()->getHeight(); y++) {
	int s = 1;
	if (y>x)
	  s += 2;
	if (x+y>l)
	  s += 1;
	//	cout << x << "," << y << endl;
	
	float r = 0.0, g = 0.0, b = 0.0;
	GetPixelRGB(x, y, r, g, b);
	// getImg()->get_pixel_rgb(x, y, r, g, b);

	avgr += r;
	avgg += g;
	avgb += b;
	
	SetPixelSegment(x, y, s);
	// getImg()->set_pixel_segment(x, y, s);
      }
    
    if (Verbose()>1) {
      float d = getImg()->getWidth()*getImg()->getHeight();
      cout << "avgr = " << (avgr/d) << endl;
      cout << "avgg = " << (avgg/d) << endl;
      cout << "avgb = " << (avgb/d) << endl;
    }
    
    
    //  AppendChain(l-getImg()->getWidth()/3, l-getImg()->getHeight()/3);
    //   AppendChain(l-getImg()->getWidth()/3, l+getImg()->getHeight()/3);
    //   AppendChain(l+getImg()->getWidth()/3, l+getImg()->getHeight()/3);
    //   AppendChain(l+getImg()->getWidth()/3, l-getImg()->getHeight()/3);
    //   AppendChain(l-getImg()->getWidth()/3, l-getImg()->getHeight()/3);
  
    //   if (Verbose()>1)
    //     cout << "Chain length: " << ChainLength() << endl;
    {
    SegmentationResult myresult;
    myresult.name="myresult";
    myresult.type="point";
    myresult.value="100 200";
    
    getImg()->writeFrameResultToXML(this,myresult);

    myresult.name="myresult1";
    myresult.type="point";
    myresult.value="100 200";
    
    getImg()->writeFrameResultToXML(this,myresult);

    myresult.name="myresult2";
    myresult.type="point";
    myresult.value="100 200";
    
    getImg()->writeFrameResultToXML(this,myresult);

    myresult.name="yourresult2";
    myresult.type="star";
    myresult.value="100 200";
    
    getImg()->writeFrameResultToXML(this,myresult);

    myresult.name="myresult3";
    myresult.type="line";
    myresult.value="100 200";
    
    getImg()->writeFrameResultToXML(this,myresult);

    cout << "results written" << endl;


    SegmentationResultList *l=getImg()->readFrameResultsFromXML("myresult3","");

    cout << "query by name=myresult3" << endl;

    for(size_t i=0;i<l->size();i++)
      cout << (*l)[i] << endl;
    delete l;

    l=getImg()->readFrameResultsFromXML("myresult","",true);

    cout << "query by name=myresult, wildcard=true" << endl;

    for(size_t i=0;i<l->size();i++)
      cout << (*l)[i] << endl;
    delete l;
    
    cout << "query by type=point, lastonly" << endl;
    SegmentationResult *res= getImg()->readLastFrameResultFromXML("","point");
    cout << *res << endl;

    cout << "invalidating myresult3" << endl;

    getImg()->invalidateFrameResults(this,"myresult3");


    cout << "query by name=myresult, wildcard=true" << endl;
    l=getImg()->readFrameResultsFromXML("myresult","",true);
    for(size_t i=0;i<l->size();i++)
      cout << (*l)[i] << endl;
    delete l;

    cout << "query by name=myresult, wildcard=true, alsoinvalid" << endl;
    l=getImg()->readFrameResultsFromXML("myresult","",true,true);
    for(size_t i=0;i<l->size();i++)
      cout << (*l)[i] << endl;
    delete l;


    cout << "invalidate type=point" << endl;

    getImg()->invalidateFrameResults(this,"","point");

    cout << "query by name=any, wildcard=true" << endl;
    l=getImg()->readFrameResultsFromXML("","",true);
    for(size_t i=0;i<l->size();i++)
      cout << (*l)[i] << endl;
    delete l;

    }

    //myresult.name="myresult2";
    //myresult.type="line";
    //myresult.value="90 190 110 190"; // tmpnam(NULL);
    
    //getImg()->writeFileResultToXML(this,myresult);

   //  int x,y;
    
//     for (int x=0; x<getImg()->getWidth(); x++){
//       cout << "x= "<< x;
//       for (int y=0; y<getImg()->getHeight(); y++) {

// 	int s;
// 	getImg()->get_pixel_segment(x,y,s);
// 	cout << s << " ";
//       }
//       cout << endl;
//     }

//     imagedata *i = getImg()->copySegmentation();
//     for (int x=0; x<getImg()->getWidth(); x++){
//       cout << "copy x= "<< x;
//       for (int y=0; y<getImg()->getHeight(); y++) {
// 	int s=i->get_one_uint16(x,y);

// 	cout << s << " ";
//       }
//       cout << endl;
//     }

//     delete i;

    // getImg()->writeAllSegmentFrames("test.tif",true);

    set<int> bs;
    bs.insert(0);
    bs.insert(2);

    getImg()->setBackgroundLabels(bs);

    cout << "storing images" << endl;

    vector<pair<int,string> > lbl=getImg()->getUsedLabelsWithText();
    
    vector<pair<int,string> >::const_iterator it;
    cout << "Used labels" << endl;
    for(it=lbl.begin(); it != lbl.end() ; it++)
      cout << "  " << it->first << ":" << it->second << endl;

    // getImg()->storeSegmentAsFrameResult(this,"myimage");

    if(showseparate){
      int i=0;
      int *lbl = getImg()->GetSeparateLabelingInt();
      for (int y=0; y<getImg()->getHeight(); y++) 
	for (int x=0; x<getImg()->getWidth(); x++){
	  SetPixelSegment(x,y,lbl[i++]);
	}

      delete lbl;

    }


    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
