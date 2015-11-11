// -*- C++ -*-  $Id: AudioBased.C,v 1.6 2013/02/25 11:04:52 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

// compile only if we want to include audio libraries
//#ifdef USE_AUDIO

#include <AudioBased.h>

namespace picsom {

  //===========================================================================

  int AudioBased::printMethodOptions(ostream &str) const {
    str << "AudioBased options ('-o option=value'):" << endl;
    str << "sliceduration=[integer] :"
	<< " audio slice duration in milliseconds. A negative value is equal" 
	<< endl
	<< "                         " 
	<< " to the duration of the whole audio file." << endl;

    return Feature::printMethodOptions(str);
  }

  //===========================================================================

  bool AudioBased::ProcessOptionsAndRemove(list<string> &l) {
    for (list<string>::iterator i = l.begin(); i!=l.end(); ) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;
    
      string key, value;
      SplitOptionString(*i, key, value); 
    
      if (key=="sliceduration") {
	int n = (int) atoi(value.c_str());
	i = l.erase(i);
	if (n == 0)
	  throw "sliceduration=[duration in milliseconds,"
	    " negative for whole audio file duration]";
	sliceDuration = n;
	continue;
      }

      i++;
    } 
    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

} // namespace picsom

//#endif // USE_AUDIO

// Local Variables:
// mode: font-lock
// End:





