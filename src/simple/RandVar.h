// -*- C++ -*-  $Id: RandVar.h,v 1.6 2009/11/20 20:48:16 jorma Exp $
// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND

#ifndef _RANDVAR_H_
#define _RANDVAR_H_

#include <Vector.h>

#include <sys/types.h>

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif //HAVE_SYS_TIMES_H

//- RandVar
namespace simple {
class RandVar : public Simple {
public:
  RandVar(int p = MAXINT) { Seed(p); }

  operator double() { return Erand(); }

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "RandVar", NULL }; return n; }

  void Seed(int i) {
    if (i==MAXINT) {
      tms buf;
      i = (int)getpid()+(int)times(&buf);
    }
    seed = randseed[0] = randseed[1] = randseed[2] = (unsigned short)i;
    // Dump();
  }

  int Seed() const { return seed; }

  double Erand() { return erand48(randseed); }
  int RandomInt(int);

  static VectorOf<int> Permutation(int l, int s = MAXINT) {
    return PermutationCorrect(l, s);
  }

  static VectorOf<int> PermutationCorrect(int, int);
  static VectorOf<int> PermutationInCorrect(int, int);

  static VectorOf<float> UniformZeroOne(int, int = MAXINT);

protected:
  int seed;
  unsigned short randseed[3];
};

} // namespace simple
#endif // _RANDVAR_H_

