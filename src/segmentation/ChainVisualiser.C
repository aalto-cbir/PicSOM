#include <ChainVisualiser.h>

static const string vcid="@(#)$Id: ChainVisualiser.C,v 1.3 2009/04/29 14:31:29 vvi Exp $";

static ChainVisualiser list_entry(true);

///////////////////////////////////////////////////////////////////////////

ChainVisualiser::ChainVisualiser() {

}

///////////////////////////////////////////////////////////////////////////

ChainVisualiser::~ChainVisualiser() {

}

///////////////////////////////////////////////////////////////////////////

const char *ChainVisualiser::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void ChainVisualiser::UsageInfo(ostream& os) const { 
  os << "ChainVisualiser :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

int ChainVisualiser::ProcessOptions(int /*argc*/, char** /*argv*/) { 
  return 0;
}

///////////////////////////////////////////////////////////////////////////

bool ChainVisualiser::DoProcess() {
  if (Verbose()>1)
    ShowLinks();

  int x,y; 

  for(y=0;y<Height();y++) for(x=0;x<Width();x++) SetPixelSegment(x,y,0);

  Segmentation::Chains *c=GetChains();

  for(std::list<Segmentation::Segment>::const_iterator it1 = c->segments.begin();
      it1 != c->segments.end(); it1++)
    for(std::list<Segmentation::SubSegment>::const_iterator it2 =
	  (*it1).subsegments.begin(); it2 != (*it1).subsegments.end(); it2++)
     for(std::list<Segmentation::PixelChain>::const_iterator it3 =
	  (*it2).borders.begin(); it3 != (*it2).borders.end(); it3++)
     for(std::list<Segmentation::coord>::const_iterator it4 =
	  (*it3).l.begin(); it4 != (*it3).l.end(); it4++)

       SetPixelSegment((*it4).x,(*it4).y,200);

  return true;

}

///////////////////////////////////////////////////////////////////////////


