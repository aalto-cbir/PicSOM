// -*- C++ -*-  $Id: AvgAudioEnergy.C,v 1.4 2013/02/25 11:04:28 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

// compile only if we want to include audio libraries
//#ifdef USE_AUDIO

#include <AvgAudioEnergy.h>

namespace picsom {
  static const char *vcid =
    "$Id: AvgAudioEnergy.C,v 1.4 2013/02/25 11:04:28 jorma Exp $";

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

//#endif //USE_AUDIO

// Local Variables:
// mode: font-lock
// End:
