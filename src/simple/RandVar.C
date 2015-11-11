// -*- C++ -*-  $Id: RandVar.C,v 1.4 2009/11/20 20:48:16 jorma Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND

#include <RandVar.h>

namespace simple {

///////////////////////////////////////////////////////////////////////////

void RandVar::Dump(Simple::DumpMode /*dt*/, ostream& os) const {
  os << "RandVar " << (void*)this
     << " seed=" << seed
     << " randseed=" << randseed[0] << "," << randseed[1] << "," << randseed[2]
     << endl;
}

///////////////////////////////////////////////////////////////////////////

int RandVar::RandomInt(int m) {
  int max = 2, n;

  for (n=1; max<m; n++, max *= 2) {}

  for (;;) {
    int res = 0;
    for (int n1=0; n1<n; n1++)
      res = (res<<1) + (int(nrand48(randseed))&1);

    if (res<m)
      return res;
  }
}

///////////////////////////////////////////////////////////////////////////

IntVector RandVar::PermutationCorrect(int l, int p) {
  IntVector vec(l);
  vec.SetIndices();

  RandVar rnd(p);

  for (int j=0; j<l-1; j++)
    vec.Swap(j, j+rnd.RandomInt(l-j));

  return vec;
}

///////////////////////////////////////////////////////////////////////////

IntVector RandVar::PermutationInCorrect(int l, int p) {
  RandVar rnd(p);
  IntVector vec(l);
  
  for (int i=0; i<l; i++)
    vec.Set(i, i);

  for (int j=0; j<l; j++)
    vec.Swap(j, rnd.RandomInt(l));

  // vec.Dump(DumpLong);

  return vec;
}

///////////////////////////////////////////////////////////////////////////

FloatVector RandVar::UniformZeroOne(int l, int p) {
  RandVar rnd(p);
  FloatVector vec(l);
  
  const int max = 123456;

  for (int i=0; i<l; i++)
    vec[i] = rnd.RandomInt(max)/double(max);

  // vec.Dump(DumpLong);

  return vec;
}

///////////////////////////////////////////////////////////////////////////

} // namespace simple

