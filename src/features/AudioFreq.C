// -*- C++ -*-  $Id: AudioFreq.C,v 1.5 2016/06/23 10:49:25 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <AudioFreq.h>

namespace picsom {
  static const char *vcid =
    "$Id: AudioFreq.C,v 1.5 2016/06/23 10:49:25 jorma Exp $";

static AudioFreq list_entry(true);

//=============================================================================

string AudioFreq::Version() const {
  return vcid;
}

//=============================================================================

int AudioFreq::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  // str << "AvgAudioEnergy options ('-o option=value'):" << endl;

  return AudioBased::printMethodOptions(str);
}

//=============================================================================

bool AudioFreq::ProcessOptionsAndRemove(list<string> &l) {
   
  return AudioBased::ProcessOptionsAndRemove(l);
}

} // namespace picsom

//=============================================================================

// Local Variables:
// mode: font-lock
// End:
