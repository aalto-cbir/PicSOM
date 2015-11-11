// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: SubSpace.h,v 1.3 2009/11/20 20:48:16 jorma Exp $

// -*- C++ -*-

#ifndef _SUBSPACE_H_
#define _SUBSPACE_H_

#include <Matrix.h>

// typedef ListOf<FloatVector> FloatVectorList;

namespace simple {
class SubSpace : public FloatMatrix {
public:
  SubSpace(int = 0, int = 0);
  SubSpace(const SubSpace&);
  SubSpace(const char*);
  SubSpace(const FloatMatrix&, const FloatVector&, const FloatVector&);
  virtual ~SubSpace();

  SubSpace&    operator=(const SubSpace&);
  FloatVector& operator[](int i) const { return *GetColumn(i); }

  virtual void Dump(DumpMode = DumpRecursiveShort, ostream& = cout) const;

  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "SubSpace", "Matrix", "VectorSet", "List", NULL }; return n; }

  virtual int VectorLength() const { return mean.Length(); }
  virtual void VectorLength(int)   { abort(); }

  int IndexOK(int i, int j) const { return FloatMatrix::IndexOK(i, j); }
  int IndexOK(int i) const        { return FloatVectorSet::IndexOK(i); }

  int Check() const;
  int FromMatrix();
  FloatMatrix ToMatrix() const;

  const FloatVector& Mean() const { return mean; }
  FloatVector& Mean() { return mean; }
  void Mean(const FloatVector& m) { mean = m; }

  int Read(istream& is)   { return FloatMatrix::Read(&is, NULL) && FromMatrix(); }
  int Read(const char *n) { return FloatMatrix::Read(n)  && FromMatrix(); }

  int Write(ostream& os)   { return ToMatrix().Write(&os, NULL); }
  int Write(const char *n) { return ToMatrix().Write(n);  }

  void SolveBaseSize(int& l) const {
    if (l<0)
      l = EffectiveBaseSize();
    if (l>BaseSize())
      l = BaseSize();
  }

  int BaseSize() const { return Columns(); }
  SubSpace& BaseSize(int dim);

  int ForcedBaseSize() const { return forced_basesize; }
  SubSpace& ForcedBaseSize(int dim) { forced_basesize = dim; return *this; }
  SubSpace& ForcedEigenValueBaseSize(double);
  SubSpace& ForcedEigenValueSumBaseSize(double);
  SubSpace& ForcedEigenValueSqrSumBaseSize(double);

  int EffectiveBaseSize() const {
    return forced_basesize>=0 ? forced_basesize : BaseSize(); }

  FloatVector MeanProjection(int dim = -1) const {
    return Projection(Mean(), FALSE, dim); }

  FloatVector MeanCoefficients(int dim = -1) const {
    return Coefficients(Mean(), FALSE, dim); }

  FloatVector Projection(const FloatVector& v,
			 int sub_mean = FALSE, int dim = -1) const {
    if (!Columns())
      return FloatVector(v.Length(), NULL, v.Label());
    else if (sub_mean)
      return FloatMatrix::PreMultiply(Coefficients(v, TRUE,  dim), dim)+mean;
    else
      return FloatMatrix::PreMultiply(Coefficients(v, FALSE, dim), dim);
  }

  FloatVectorSet Projection(FloatVectorSource&,
			    const FloatVector* = NULL) const;

  FloatVector Coefficients(const FloatVector& v,
			   int m = TRUE, int dim = -1) const;

  FloatVector ReverseCoefficients(const FloatVector& v,
				  int m = TRUE, int dim = -1) const {
    SolveBaseSize(dim);
    return m ? FloatMatrix::PreMultiply(v, dim)+mean :
    FloatMatrix::PreMultiply(v, dim); }

  FloatVectorSet Coefficients(FloatVectorSource&,
			      const FloatVector* = NULL) const;

  FloatVector AugmentedCoefficients(const FloatVector& v,
				    int m = TRUE, int dim = -1) const {
    FloatVector ret = Coefficients(v, m, dim);
    float vl  = m ? (v-mean).NormSquared() : v.NormSquared();
    return ret.Append(sqrt(vl-ret.NormSquared()));
  }

  double DistanceTo(const FloatVector& v, int m = TRUE, int dim = -1) const {
    return AugmentedCoefficients(v, m, dim).Peek();
  }

  double LengthOnSquared(const FloatVector&, int = -1) const;
  double LengthOn(const FloatVector& v, int l = -1) const {
    return sqrt(LengthOnSquared(v, l)); }

  double DotProductWithOne(const FloatVector& v, int i) const {
    return IndexOK(i) ? GetColumn(i)->DotProduct(v) : 0; }
  double LengthOnOneSquared(const FloatVector& v, int i) const {
    return Transfer(DotProductWithOne(v, i), eigenvalues[i]); }

  SubSpace& RotateTowards(const FloatVector&, double);
  SubSpace& StochasticGradientAscent(const FloatVector&, double, int = FALSE);

  double Transfer(double l, double v) const {
    if (transfer)
      return transfer(l, v);
    else
      return l*l;
  }

  double (*Transfer() const )(double, double) { return transfer; }
  void Transfer(double (*f)(double, double))  { transfer = f; }
  void Transfer(const char *str) { transfer = VerboseTransfer(str); }

  const char *VerboseTransfer() const { return VerboseTransfer(transfer); }
  static double (*VerboseTransfer(const char*))(double, double);
  static const char *VerboseTransfer(double(*)(double, double));
  static const char *VerboseTransferShort(double(*)(double, double));

  static double POS(double a, double  ) { return a>0 ? a*a : 0; }
  static double ABS(double a, double  ) { return fabs(a)*a; }
  static double MSM(double a, double b) { return a*a*b; }

  void CoefficientSigns(const FloatVector&, int = TRUE) const;
  double EigenValue(int i) const { return eigenvalues.Get(i); }
  const FloatVector& EigenValue() const { return eigenvalues; }
  FloatVector& EigenValue() { return eigenvalues; }

  void CoefficientStatistics(FloatVectorSet&, const FloatVector* = NULL,
			     ostream& = cout, const char* = NULL,
			     const char* = NULL);

//   void Gamma(double a, double b) {
//     gamma_a = a; gamma_b =b; }
  
//   double GammaProb(double ls) const {
//     return gamma_a ? FloatVector::GammaPdf(ls, gamma_a, gamma_b) : 0;
//   }

//   double GammaProb(const FloatVector& v) const {
//     return GammaProb(v.NormSquared()-LengthOnSquared(v));
//   }

//   double Gamma_a() const { return gamma_a; }
//   double Gamma_b() const { return gamma_b; }

  int NPdfParams() const { return pdf_params.Length(); }
  double PdfParam(int i) const { return pdf_params.Get(i); }
  SubSpace& PdfParam(int i, double v) {
    if (pdf_params.Length()<i+1)
      pdf_params.Lengthen(i+1);
     pdf_params.Set(i, v);
     return *this;
  }

protected:
  FloatVector eigenvalues;
  FloatVector mean;
  int forced_basesize;

  //  double gamma_a, gamma_b;
  FloatVector pdf_params;

  double (*transfer)(double, double);
};

} // namespace simple
#endif // _SUBSPACE_H_

