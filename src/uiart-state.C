// -*- C++ -*-  $Id: uiart-state.C,v 1.5 2009/05/28 13:18:06 ajanki Exp $
// 
// Copyright 2008-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Copyright 2008-2009 UI-ART project group
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <uiart-state.h>

namespace uiart {

  //
  VideoSource::VideoSource(const string& n) : Source(video, n) {
  }

  //
  VideoData::VideoData(const VideoSource *src) : Data(0, 0, 0) {
    ref = src;
  }

} // namespace uiart

