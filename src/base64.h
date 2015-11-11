// -*- C++ -*-  $Id: base64.h,v 2.0 2005/06/08 07:58:22 jorma Exp $

#ifndef _base64_h_
#define _base64_h_

#include <iostream>
using namespace std;

namespace b64 {
	bool encode(istream& input, ostream& output);
	bool decode(istream& input, ostream& output);
}

#endif // _base64_h_

