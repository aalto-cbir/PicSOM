#include <RemoveHoles.h>

using std::ostream;
using std::map;

namespace picsom{

static const string vcid="@(#)$Id: RemoveHoles.C,v 1.5 2009/04/30 10:10:05 vvi Exp $";

static RemoveHoles list_entry(true);

///////////////////////////////////////////////////////////////////////////

RemoveHoles::RemoveHoles() {

}

///////////////////////////////////////////////////////////////////////////

RemoveHoles::~RemoveHoles() {

}

///////////////////////////////////////////////////////////////////////////

const char *RemoveHoles::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void RemoveHoles::UsageInfo(ostream& os) const { 
  os << "RemoveHoles :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

int RemoveHoles::ProcessOptions(int /*argc*/, char** /*argv*/) { 
  return 0;
}

///////////////////////////////////////////////////////////////////////////

bool RemoveHoles::Process() {

  // find the largest continuous region that has label 0 in segmented image

  int x,y,v;

  int *unique = getImg()->GetSeparateLabelingInt();

  map<int,int> counts;

  int w=getImg()->getWidth();
  int h=getImg()->getHeight();
  
  for(y=0;y<h;y++) for (x=0;x<w;x++){
    getImg()->get_pixel_segment(x, y, v);
    if(v==0)
      counts[unique[x+y*w]]++;
  }

  if(counts.size()>1){
    int maxlabel=counts.begin()->first;
    map<int,int>::const_iterator it;
    for(it=counts.begin();it!=counts.end();it++)
      if(it->second > counts[maxlabel])
	maxlabel=it->first;
  
    
    int i=0;
    for(y=0;y<h;y++) for(x=0;x<w;x++){
      getImg()->get_pixel_segment(x, y, v);
      if(v==0 && unique[i] != maxlabel)
	getImg()->set_pixel_segment(x,y,1);
      i++;
    }
  }
  
  return ProcessNextMethod();

}

///////////////////////////////////////////////////////////////////////////
}

