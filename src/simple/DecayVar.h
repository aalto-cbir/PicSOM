// -*- C++ -*-  $Id: DecayVar.h,v 1.3 2009/11/20 20:48:15 jorma Exp $
// 
// Copyright 1994-2008 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _DECAYVAR_H_
#define _DECAYVAR_H_

#include <Simple.h>
#include <string.h>

enum DvMode {
  dvNone, dvConstant, dvLinear, dvInverse, dvExponential
};

//- DecayVar
namespace simple {
class DecayVar : public Simple {
public:
  DecayVar(double v = 0, DvMode m = dvConstant, double b = 0, double x = 0) {
    start  = value  = v;
    dvmode = m;
    base   = b;
    mult   = dvmode==dvExponential ? x : 0;
    end    = dvmode==dvLinear ? (int)x : 0;
    round  = 0;
  }

  DecayVar(const char *s) { From(s); }

  void Clear() {
    value  = start = base = mult = 0;
    round  = end = 0;
    dvmode = dvNone;
  }

  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& os = cout) const {
    os << "DecayVar " << (void*)this
       << " dvmode=" << ModeChar() << " (" << dvmode << ")"
       << " value=" << value
       << " start=" << start
       << " base=" << base
       << " mult=" << mult
       << " end=" << end
       << " round=" << round
       << endl;
  }

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "DecayVar", NULL }; return n; }

  operator double() const { return Value();    }
  operator const char*() const { return ToString(); }

  double operator-() const { return -Value(); }

  DecayVar& operator=(double v)  { value  = v; return *this; }
  DecayVar& operator+=(double v) { value += v; return *this; }
  DecayVar& operator-=(double v) { value -= v; return *this; }

  DecayVar& operator++() { return Decay(); }

  int operator==(const DecayVar& v) const {
    return dvmode==v.dvmode && value==v.value && start==v.start &&
      base==v.base && mult==v.mult && end==v.end && round==v.round;
  }

  int operator!=(const DecayVar& s) const { return !operator==(s); }

  int operator<(const DecayVar& v) const  { return value< v.value; }
  int operator>(const DecayVar& v) const  { return value> v.value; }
  int operator>=(const DecayVar& v) const { return value>=v.value; }
  int operator<=(const DecayVar& v) const { return value<=v.value; }

  DecayVar& SetMode(DvMode v)       { dvmode = v; return *this; }
  DecayVar& SetConstant(double v)    { value = v; return SetMode(dvConstant);   }
  DecayVar& SetLinear(int v)         { end   = v; return SetMode(dvLinear);     }
  DecayVar& SetInverse()             {            return SetMode(dvInverse);    }
  DecayVar& SetExponential(double v) { mult  = v; return SetMode(dvExponential);}
  DecayVar& SetBase( double v)       { base  = v; return *this; }
  DecayVar& SetStart(double v)       { start = v; return *this; }

  DecayVar& SetHalfLife(int v) { return SetFractionLife(0.5, v); }

  DecayVar& SetFractionLife(double f, int v) {
    return SetExponential(pow(f, 1.0/v)); }

  int IsSet()         const { return dvmode!=dvNone;        }
  int IsConstant()    const { return dvmode==dvConstant;    }
  int IsLinear()      const { return dvmode==dvLinear;      }
  int IsInverse()     const { return dvmode==dvInverse;     }
  int IsExponential() const { return dvmode==dvExponential; }

  double Start() const { return start; }
  double Value() const { return value; }
  int Round()    const { return round; }

  DecayVar& Keep(double v) { start = value = v; return *this; }
  DecayVar& Keep()         { start = value;     return *this; }

  DecayVar& Revert() { value = start; round = 0; return *this; }

  DecayVar& Decay() {
    if (IsSet()) {
      if (IsLinear()) value = round<end ? start-(start-base)*round/end : base;
      else if (IsInverse()) value = (start-base)/(round+1)+base;
      else if (IsExponential()) value = (value-base)*mult+base;

      round++;
    }

    return *this;
  };

  const char *ToString() const {
    char tmp[1000];
    sprintf(tmp, "%g%c", value, ModeChar());

    if (IsLinear() || IsInverse() || IsExponential()) {
      if (value!=start)
	sprintf(tmp+strlen(tmp), "%gS", start);

      if (IsLinear())
	sprintf(tmp+strlen(tmp), "%dE", end); // corrected 16.3.2005

      if (IsExponential())
	sprintf(tmp+strlen(tmp), "%gM", mult); // added 16.3.2005

      if (base)
	sprintf(tmp+strlen(tmp), "%gB", base);

      if (round)
	sprintf(tmp+strlen(tmp), "%dR", round);
    }
    static char *txt;
    return CopyString(txt, tmp);
  }

  const char *ToLongString() const {
    char tmp[1000];

    sprintf(tmp, "%s rounds=%d initial=%g", ModeText(), round, start);
    if (!IsConstant())
      sprintf(tmp+strlen(tmp), " final=%g", value);
    if (IsLinear()) 
      sprintf(tmp+strlen(tmp), " endround=%d", end);
    if (IsExponential())
      sprintf(tmp+strlen(tmp), " multiplier=%g", mult);
    if (base!=0)
      sprintf(tmp+strlen(tmp), " base=%g", base);

    static char *txt;
    return CopyString(txt, tmp);
  }

  char ModeChar() const {
    if (IsConstant())    return 'C';
    if (IsLinear())      return 'L';
    if (IsInverse())     return 'I';
    if (IsExponential()) return 'X';
    return 0;
  }

  const char *ModeText() const {
    if (IsConstant())    return "constant";
    if (IsLinear())      return "linear";
    if (IsInverse())     return "inverse";
    if (IsExponential()) return "exponential";
    return "???";
  }

  void Print(ostream& os = cout, bool verb = false) const {
    os << (verb ? ToLongString() : ToString());
  }

  void From(const char *str) {
    Clear();

    float f[5];
    char  c[5];

    int n = sscanf(str, "%f%c%f%c%f%c%f%c%f%c", f, c, f+1, c+1, f+2, c+2,
		   f+3, c+3, f+4, c+4);
    if (n%2 || n<2) {
      cerr << "DecayVar::From(" << str << ") failed with n=" << n << endl;
      abort();
    }

    switch (c[0]) {
    case 'C': SetMode(dvConstant);    break;
    case 'L': SetMode(dvLinear);      break;
    case 'I': SetMode(dvInverse);     break;
    case 'X': SetMode(dvExponential); break;
    default:
      cerr << "DecayVar::From(" << str << ") failed: " << c[0] << endl;
      abort();
    }

    Keep(f[0]);

    n /= 2;

    for (int i=1; i<n; i++)
      switch (c[i]) {
      case 'S': start = f[i];      break;
      case 'B': base  = f[i];      break;
      case 'E': end   = (int)f[i]; break;
      case 'R': round = (int)f[i]; break;
      default:
	cerr << "DecayVar::From(" << str << ") failed: " << c[i] << endl;
	abort();
      }
  }

  friend ostream& operator<<(ostream& os, const DecayVar& p) {
    p.Print(os);
    return os;
  }

protected:
  DvMode dvmode;
  double value;
  double start;
  double base;
  double mult;
  int end;
  int round;

  static void NotImplemented(const char *txt) {
    cerr << "Not implemented DecayVar::" << txt << endl;
  }
  static void NotImplementedAbort(const char *txt) {
    NotImplemented(txt);
    abort();
  }
};

} // namespace simple
#endif // _DECAYVAR_H_

