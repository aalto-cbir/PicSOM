// $Id: SimpleGeometric.C,v 1.2 2008/09/29 10:22:40 markus Exp $	

#ifdef __linux__

#include <SimpleGeometric.h>

namespace picsom {
  static const char *vcid =
    "$Id: SimpleGeometric.C,v 1.2 2008/09/29 10:22:40 markus Exp $";

static SimpleGeometric list_entry(true);

//=============================================================================

string SimpleGeometric::Version() const {
  return vcid;
}
//=============================================================================

bool SimpleGeometric::ProcessOptionsAndRemove(list<string>&l){

  return Feature::ProcessOptionsAndRemove(l);

}


//=============================================================================

int SimpleGeometric::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  int ret=Feature::printMethodOptions(str);
  return ret;
}

//=============================================================================
Feature *SimpleGeometric::Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data)
{
  return Feature::Initialize(img,seg,s,opt,allocate_data);
}


//=============================================================================
} // namespace picsom

#endif // __linux__

// Local Variables:
// mode: font-lock
// End:

