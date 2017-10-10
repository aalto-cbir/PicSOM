// -*- C++ -*-  $Id: picsom-main.C,v 2.2 2017/02/24 14:50:47 jorma Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <PicSOM.h>

//-----------------------------------------------------------------------------

int main(int argc, const char **argv) {
  bool traditional = true;
  if (traditional)
    return picsom::PicSOM::Main(argc, argv);
  
  picsom::PicSOM *pp = NULL;
  int r = picsom::PicSOM::Main(argc, argv, &pp);
  if (r) {
    cerr << "picsom::PicSOM::Main(argc, argv, &pp) returned " << r << endl;
    return r;
  }
  cout << "picsom::PicSOM::Main(argc, argv, &pp) : pp=" << pp << endl;
  delete pp;

  return 0;
}

//-----------------------------------------------------------------------------
