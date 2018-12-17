// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2018 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: OptVar.h,v 1.4 2018/12/15 23:27:04 jormal Exp $

// -*- C++ -*-

#ifndef _OPTVAR_H_
#define _OPTVAR_H_

#include <Vector.h>

//- OptVar
namespace simple {
class OptVar : public Simple {
public:
  enum OptVarType {
    OptNone, OptInt, OptFloat, OptIntVector, OptFloatVector
  };

  OptVar(OptVarType t = OptNone, const char *n = NULL) {
    name = CopyString(n);
    descr = NULL;
    Init();
    type = t;
    CheckPredefined();
  }

  OptVar(const StatQuad& sq) {
    name = descr = NULL;
    Init();
    result = sq;
  }

  OptVar(const OptVar& v) : Simple(v) {
    name = CopyString(v.name);
    descr = CopyString(v.descr);
    Init(&v);
    if (v.next)
      Next(*v.next);
  }

  virtual ~OptVar() { delete name; delete descr; delete next; }

  virtual void Dump(DumpMode dm = DumpRecursiveShort, ostream& os= cout) const{
    os << Bold("OptVar ")  << (void*)this
       << " name="   << ShowString(name)
       << " Argc()=" << Argc();
    for (int i=0; i<Argc(); i++)
      os << " Argv(" << i << ")=\"" << Argv(i) << "\"";
    os << " type="   << VerboseType(type)
       << " descr="  << ShowString(descr);
    if (type!=OptNone) {
      os << " value=";      
      ShowValue(os);
      os << " ";
    }
    os << " result=" << result
       << " next="   << (void*)next
       << endl;
    if (next && dm&DumpRecursive)
      next->Dump(dm, os);
  }

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "OptVar", NULL }; return n; }

  operator StatQuad() {
    return result; }
  operator const StatQuad& () const {  
    return result; }
  // the previous doesn't work with sgi CC-7.0 -64. Use Result().
  const StatQuad& RefResult() const { return result; }

  OptVar& operator=(const OptVar& v) {
    CopyString(name, v.name);
    CopyString(descr, v.descr);
    Init(&v);
    if (v.next)
      Next(*v.next);

    return *this;
  }

  OptVar& operator=(int    i) { return Ival(i); }
  OptVar& operator=(double f) { return Fval(f); }

  int Ival() const { return ival; }
  double Fval() const { return fval; } // return type was int until 4.2.1998 !!

  OptVar& Ival(int i)    { ival = i; return *this; }
  OptVar& Fval(double f) { fval = f; return *this; }

  OptVar& Set(int i, const StatQuad& r) { return Ival(i).Result(r); }
  OptVar& Set(double f, const StatQuad& r) { return Fval(f).Result(r); }

  void Init(const OptVar *v = NULL) {
    ival = v ? v->ival : 0;
    fval = v ? v->fval : 0.0; 
    type = v ? v->type : OptNone;
    if (v) {
      ivec = v->ivec;
      fvec = v->fvec;
      result = v->result;

    } else {
      ivec.Length(0);
      fvec.Length(0);
      result = StatQuad();
    }

    next = NULL;
  }

  const char *Name() const { return name; }
  OptVar& Name(const char *n) { CopyString(name, n); return *this; }

  int NamesMatch(const char *n) const {
    return StringsMatch(Argv(0), n);
  }

  int NamesMatch(const OptVar& v) const {
    return NamesMatch(v.Argv(0));
  }

  int Argc() const {
    int n = 0;
    const char *ptr = name;
    while (ptr) {
      n++;
      ptr = strchr(ptr, ' ') ? strchr(ptr, ' ')+1 : NULL;
    }
    return n;
  }

  const char *Argv(int i) const {
    static char ret[1000];
    *ret = 0;
    const char *ptr = name;
    while (i-- && ptr)
      ptr = strchr(ptr, ' ') ? strchr(ptr, ' ')+1 : NULL;

    if (!ptr)
      return NULL;

    strcpy(ret, ptr);
    if (strchr(ret, ' '))
      *strchr(ret, ' ') = 0;

    return ret;
  }

  void CheckPredefined() const {
  }

  const char *VerboseType() const { return VerboseType(type); }

  static const char *VerboseType(OptVarType t) {
    switch (t) {
    case OptNone: 	 return "OptNone";
    case OptInt:  	 return "OptInt";
    case OptFloat:       return "OptFloat";
    case OptIntVector:   return "OptIntVector";
    case OptFloatVector: return "OptFloatVector";
    default: return NULL;
    }
  }
  
  void ShowValue(ostream& os = cout) const {
    switch (type) {
    case OptInt:  	 
    case OptFloat:       os << Value(); break;
    case OptIntVector:   ivec.Dump(DumpLong, os); break;
    case OptFloatVector: fvec.Dump(DumpLong, os); break;
    default:;
    }
  }

  const char *Value() const {
    static char val[1000];
    switch (type) {
    case OptInt:   sprintf(val, "%d", ival); break;
    case OptFloat: sprintf(val, "%g", fval); break;
    default: *val = 0; ShowError("OptVar::Value() type=", VerboseType());
      SimpleAbort();
    }

    return val;
  }

  int HasNext() const { return next!=NULL; }

  OptVar *Next() const { return next; }
  // void Next(OptVar *n} { next = n; }

  OptVar& Next(const OptVar& n) {
    delete next;
    next = NULL;
    return AppendCopy(n); }

  // OptVar& Append(OptVar& n) { return Append(&n); }

  OptVar& AppendCopy(const OptVar& n) {
    return Append(new OptVar(n)); }

  OptVar& Append(OptVar *n) {
    if (!next)
      next = n;
    else
      next->Append(n);

    return *this;
  }

  static void Append(OptVar*& b, OptVar *n) {
    if (b)
      b->Append(n);
    else
      b = n;
  }

  StatQuad Result() const { return result; }
  OptVar& Result(const StatQuad& r) { result = r; return *this; }

  OptVarType Type()  const  { return type; }
  OptVar& TypeNone()        { type = OptNone;        return *this; }
  OptVar& TypeInt()         { type = OptInt;         return *this; }
  OptVar& TypeFloat()       { type = OptFloat; 	     return *this; }
  OptVar& TypeIntVector()   { type = OptIntVector;   return *this; }
  OptVar& TypeFloatVector() { type = OptFloatVector; return *this; }

  OptVar& Order(const char *v1, const char *v2) { 
    for (const OptVar *var = this; var ; var = var->next)
      if (var->NamesMatch(v2))
	for (const OptVar *var2 = var->next; var2; var2 = var2->next)
	  if (var2->NamesMatch(v1)) {
	    ShowError("OptVar::Order( ", v1, " , ", v2, " ) detected.");
	    SimpleAbort();
	  }

    return *this;
  }

  const char *Description() const { return descr; }
  OptVar& Description(const char *d) {
    CopyString(descr, d);
    if (next) {
      char tmp[2000];
      sprintf(tmp, "%s-%s=%s", descr?descr:"optvar", Argv(0), Value());
      next->Description(tmp);
    }

    return *this;
  }
  
  OptVar& SetDescriptions() { return Description(Description()); }

  const char *FileName() const {
    static char ret[1000];
    sprintf(ret, "%s.res", descr?descr:"OPTVAR");
    return ret;
  }

  const OptVar& AppendTo(StatQuadVector& sqv) const {
    sqv.Append(Result());
    switch (type) {
    case OptInt:   sqv.Last().X(ival); break;
    case OptFloat: sqv.Last().X(fval); break;
    default:
      ShowError("OptVar::AppendTo: ", VerboseType(), " not implemented.");
    }
    return *this;
  }

  const OptVar& PrependTo(StatQuadVector& sqv) const {
    sqv.Prepend(Result());
    switch (type) {
    case OptInt:   sqv[0].X(ival); break;
    case OptFloat: sqv[0].X(fval); break;
    default:
      ShowError("OptVar::PrependTo: ", VerboseType(), " not implemented.");
    }
    return *this;
  }

  const OptVar& Display(ostream& os = cout) const {
    for (const OptVar *v = this; v; v = v->Next()) {
      os << v->Name() << " -> " << v->Value() << endl;
    }
    result.Display(os);
    os << endl;

    return *this;
  }

protected:
  const char *name, *descr;

  OptVarType type;

  int         ival;
  float       fval;
  IntVector   ivec;
  FloatVector fvec;

  // OptVar *up_limit, *down_limit, *direction;
  OptVar *next;

  StatQuad result;
};

} // namespace simple
#endif // _OPTVAR_H_

