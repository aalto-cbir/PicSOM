// -*- C++ -*-  $Id: AvgAudioEnergy.C,v 1.5 2016/06/23 10:49:25 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <AvgAudioEnergy.h>

namespace picsom {
  static const char *vcid =
    "$Id: AvgAudioEnergy.C,v 1.5 2016/06/23 10:49:25 jorma Exp $";

static AvgAudioEnergy list_entry(true);

//=============================================================================

string AvgAudioEnergy::Version() const {
  return vcid;
}

//=============================================================================

int AvgAudioEnergy::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  // str << "AvgAudioEnergy options ('-o option=value'):" << endl;

  return AudioBased::printMethodOptions(str);
}

//=============================================================================

bool AvgAudioEnergy::ProcessOptionsAndRemove(list<string> &l) {
   
  return AudioBased::ProcessOptionsAndRemove(l);
}

} // namespace picsom

//=============================================================================

// Local Variables:
// mode: font-lock
// End:
