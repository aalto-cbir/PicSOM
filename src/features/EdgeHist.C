// $Id: EdgeHist.C,v 1.2 2008/09/29 10:22:40 markus Exp $	

#include <EdgeHist.h>

namespace picsom {
  static const char *vcid =
    "$Id: EdgeHist.C,v 1.2 2008/09/29 10:22:40 markus Exp $";

static EdgeHist list_entry(true);

//=============================================================================

string EdgeHist::Version() const {
  return vcid;
}
//=============================================================================

bool EdgeHist::ProcessOptionsAndRemove(list<string>&l){

  return PixelBased::ProcessOptionsAndRemove(l);

}


//=============================================================================

int EdgeHist::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  return ret;
}

//=============================================================================
Feature *EdgeHist::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  return PixelBased::Initialize(img,seg,s,opt,allocate_data);
}

//=============================================================================


//=============================================================================
} // namespace picsom

// Local Variables:
// mode: font-lock
// End:

