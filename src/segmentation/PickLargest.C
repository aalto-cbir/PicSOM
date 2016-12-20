#include <PickLargest.h>

using std::cout;
using std::endl;
using std::ostream;

namespace picsom{

static const string vcid="@(#)$Id: PickLargest.C,v 1.6 2009/04/30 10:02:49 vvi Exp $";

static PickLargest list_entry(true);

///////////////////////////////////////////////////////////////////////////

PickLargest::PickLargest() {

}

///////////////////////////////////////////////////////////////////////////

PickLargest::~PickLargest() {

}

///////////////////////////////////////////////////////////////////////////

const char *PickLargest::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void PickLargest::UsageInfo(ostream& os) const { 
  os << "PickLargest :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

int PickLargest::ProcessOptions(int /*argc*/, char** /*argv*/) { 
  return 0;
}

///////////////////////////////////////////////////////////////////////////

bool PickLargest::Process() {

  int x,y,v;

  int *unique = getImg()->GetSeparateLabelingInt();

  map<int,int> counts;

  int w=getImg()->getWidth();
  int h=getImg()->getHeight();
  
  for(y=0;y<h;y++) for (x=0;x<w;x++){
    getImg()->get_pixel_segment(x, y, v);
    if(v!=0)
      counts[unique[x+y*w]]++;
  }

  if(counts.size()>0){
    int maxlabel=counts.begin()->first;
    map<int,int>::const_iterator it;
    for(it=counts.begin();it!=counts.end();it++)
      if(it->second > counts[maxlabel])
	maxlabel=it->first;
  

    int i=0;
    for(y=0;y<h;y++) for(x=0;x<w;x++){
      if(unique[i]==maxlabel)
	getImg()->set_pixel_segment(x,y,1);
      else
	getImg()->set_pixel_segment(x,y,0);
      i++;
    }
  }

  return ProcessNextMethod();
  
}

///////////////////////////////////////////////////////////////////////////
}

