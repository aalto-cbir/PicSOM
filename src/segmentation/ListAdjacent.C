#include <ListAdjacent.h>


using std::cout;
using std::endl;
using std::set;
using std::vector;

namespace picsom {
  static const string vcid="@(#)$Id: ListAdjacent.C,v 1.2 2009/04/30 09:51:39 vvi Exp $";

  static ListAdjacent list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  ListAdjacent::ListAdjacent() {
  }

  /////////////////////////////////////////////////////////////////////////////
  
  ListAdjacent::~ListAdjacent() {

  }

  /////////////////////////////////////////////////////////////////////////////

  const char *ListAdjacent::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void ListAdjacent::UsageInfo(ostream& os) const { 
    os << "ListAdjacent :" << endl
       << "  no options" << endl;  

  }

  /////////////////////////////////////////////////////////////////////////////

  int ListAdjacent::ProcessOptions(int argc, char** argv) {
 
    Option option;
    int argc_old=argc;


    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
     
	
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
  
  bool ListAdjacent::Process() {

    vector<set<int> > adj=getImg()->getAdjacentCombinations();
    
    if(Verbose()>1)
      cout << "Got "<<adj.size() << " combinations." << endl;

    int l=adj.size();
    SegmentationResult res;
    res.type="virtualsegment";
    res.value="";
    for(int i=0;i<l;i++){
      res.name=formVirtualSegmentLabel(adj[i]);
      getImg()->writeFrameResultToXML(this,res);
      if(Verbose()>1)
	cout << "  "<<res.name<<endl;
    }
  
    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
