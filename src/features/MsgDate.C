// $Id: MsgDate.C,v 1.3 2008/09/29 10:22:40 markus Exp $	

#include <MsgDate.h>

namespace picsom {
  static const char *vcid =
    "$Id: MsgDate.C,v 1.3 2008/09/29 10:22:40 markus Exp $";

static MsgDate list_entry(true);

//=============================================================================

string MsgDate::Version() const {
  return vcid;
}

//=============================================================================
}

// Local Variables:
// mode: font-lock
// End:

