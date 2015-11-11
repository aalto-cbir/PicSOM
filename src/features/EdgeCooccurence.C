// $Id: EdgeCooccurence.C,v 1.2 2008/09/29 10:22:40 markus Exp $	

#include <EdgeCooccurence.h>

namespace picsom {
  static const char *vcid =
    "$Id: EdgeCooccurence.C,v 1.2 2008/09/29 10:22:40 markus Exp $";

static EdgeCooccurence list_entry(true);

//=============================================================================

string EdgeCooccurence::Version() const {
  return vcid;
}
//=============================================================================

bool EdgeCooccurence::ProcessOptionsAndRemove(list<string>&l){

  return PixelBased::ProcessOptionsAndRemove(l);

}


//=============================================================================

int EdgeCooccurence::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  return ret;
}

//=============================================================================
Feature *EdgeCooccurence::Initialize(const string& img, const string& seg,
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

