// 
// Copyright 1994-2005 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2005 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: SubSpace.C,v 1.5 2009/11/20 20:48:16 jorma Exp $

#include <Vector.h>
#include <SubSpace.h>
#include <Histogram.h>

namespace simple {

///////////////////////////////////////////////////////////////////////////

SubSpace::SubSpace(int d, int n) : FloatMatrix(d, n) {
  mean.Length(d);
  eigenvalues.Length(n);
  forced_basesize = -1;
  transfer        = NULL;
  //  gamma_a = gamma_b = 0;
}

///////////////////////////////////////////////////////////////////////////

SubSpace::SubSpace(const SubSpace& m) : SourceOf<FloatVector>(m),
					FloatMatrix(m) {
  eigenvalues     = m.eigenvalues;
  mean            = m.mean;
  forced_basesize = m.forced_basesize;
  transfer        = m.transfer;
  //gamma_a         = m.gamma_a;
  //gamma_b         = m.gamma_b;
  pdf_params      = m.pdf_params;
}

///////////////////////////////////////////////////////////////////////////

SubSpace::SubSpace(const char *name) : FloatMatrix(name) {
  FromMatrix();
  forced_basesize = -1;
  transfer        = NULL;
  // gamma_a = gamma_b = 0;
}

///////////////////////////////////////////////////////////////////////////

SubSpace::SubSpace(const FloatMatrix& b,
		   const FloatVector& v, const FloatVector& m) :
  FloatMatrix(b), eigenvalues(v), mean(m) {
  Check();
  forced_basesize = -1;
  transfer        = NULL;
  // gamma_a = gamma_b = 0;
}

///////////////////////////////////////////////////////////////////////////

SubSpace::~SubSpace() {
}

///////////////////////////////////////////////////////////////////////////

SubSpace& SubSpace::operator=(const SubSpace& m) {
  *(FloatMatrix*)this = m;

  mean            = m.mean;
  eigenvalues     = m.eigenvalues;
  forced_basesize = m.forced_basesize;
  transfer        = m.transfer;
  // gamma_a         = m.gamma_a;
  // gamma_b         = m.gamma_b;
  pdf_params      = m.pdf_params;

  return *this;
}

///////////////////////////////////////////////////////////////////////////

void SubSpace::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&DumpRecursive)
    FloatMatrix::Dump(type, os);

  if (type&DumpShort || type&DumpLong) {
    os << "    SubSpace "     << (void*)this
       << " forced_basesize=" << forced_basesize
       << " transfer="        << VerboseTransfer()
//        << " gamma_a="        << gamma_a
//        << " gamma_b="        << gamma_b
       << " NPdfParams()="    << NPdfParams()
       << endl;
  }

  if (type&DumpLong) {
    os << "mean ";
    mean.Dump(type, os);
    os << "eigenvalues ";
    eigenvalues.Dump(type, os);
    os << "pdf_params ";
    pdf_params.Dump(type, os);
  }
}

///////////////////////////////////////////////////////////////////////////

static struct {
  double (*func)(double, double);
  const char *name;
} transfer_array[] = {
  { NULL,           "SQR" },
  { SubSpace::POS,  "POS" },
  { SubSpace::ABS,  "ABS" },
  { SubSpace::MSM,  "MSM" },
  { NULL,           NULL  }
};

const char *SubSpace::VerboseTransferShort(double (*f)(double, double)) { 
//   cout << "SubSpace::VerboseTransferShort(" << (void*)f << ") "
//        << "transfer_array[3].func=" << (void*)transfer_array[3].func <<endl;
//   cout << "SubSpace::POS=" << (void*)SubSpace::POS << endl;
//   cout << "SubSpace::ABS=" << (void*)SubSpace::ABS << endl;
//   cout << "SubSpace::MSM=" << (void*)SubSpace::MSM << endl;

  int i = 0;
  while (transfer_array[i].func!=f && transfer_array[i].name)
    i++;

  return transfer_array[i].name;
}

const char *SubSpace::VerboseTransfer(double (*f)(double, double)) { 
  stringstream ss;
  ss << "0x" << f; 

  static char str[100];
  strcpy(str, ss.str().c_str());


// #if defined(__alpha__)
//   sprintf(str, "0x%p", f);
// #elif defined(__GNUC__) && __GNUC__==4
//   sprintf(str, "0x%p", static_cast<void*>(f));
// #else
//   sprintf(str, "0x%p", (void*)f);
// #endif

  int i = 0;
  while (transfer_array[i].func!=f && transfer_array[i].name)
    i++;

  if (transfer_array[i].name)
    sprintf(str+strlen(str), " (%s)", transfer_array[i].name);

  return str;
}

///////////////////////////////////////////////////////////////////////////

double (*SubSpace::VerboseTransfer(const char *str))(double, double) { 
  int i = 0;
  while (transfer_array[i].name && strcmp(str, transfer_array[i].name))
    i++;

  return transfer_array[i].name ? transfer_array[i].func : NULL;
}

///////////////////////////////////////////////////////////////////////////

SubSpace& SubSpace::RotateTowards(const FloatVector& v, double mu) {
  for (int i=0; i<Columns(); i++)
    *GetColumn(i) += v*(mu*GetColumn(i)->DotProduct(v));

  return *this;
}

///////////////////////////////////////////////////////////////////////////

SubSpace& SubSpace::StochasticGradientAscent(const FloatVector& x,
					     double ga, int rev) {
  FloatVector sum(x);
  if (rev)
    sum.Multiply(-1);

  for (int i=0; i<Columns(); i++) {
    FloatVector  w = *GetColumn(i);
    double       y = w.DotProduct(x);
    sum           -= w*y;
    FloatVector  d = sum*(ga*y);
    *GetColumn(i) += d;
    sum           -= w*y;
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////

double SubSpace::LengthOnSquared(const FloatVector& v, int l) const {
  SolveBaseSize(l);

  register double sum = 0;
  for (int i=0; i<l; i++)
    sum += Transfer(GetColumn(i)->DotProduct(v), eigenvalues[i]);

  return sum;
}

///////////////////////////////////////////////////////////////////////////

SubSpace& SubSpace::BaseSize(int dim) {
  if (dim<0) {
    cerr << "SubSpace::BaseSize negative: dim=" << dim << endl;
    dim = BaseSize();
  }

  while (BaseSize()>dim)
    RemoveColumn(BaseSize()-1);

  if (BaseSize()<dim)
    cerr << "SubSpace::BaseSize adding empty columns..." << endl;

  while (BaseSize()<dim)
    AppendColumn();

  eigenvalues.Lengthen(BaseSize());

  return *this;
}

///////////////////////////////////////////////////////////////////////////

SubSpace& SubSpace::ForcedEigenValueBaseSize(double v) {
  int i = 0;
  for (; i<BaseSize(); i++)
    if (eigenvalues[i]<v)
      break;

  return ForcedBaseSize(i);
}

///////////////////////////////////////////////////////////////////////////

SubSpace& SubSpace::ForcedEigenValueSumBaseSize(double v) {
  int i = 0;
  double sum = 0;
  for (; i<BaseSize(); i++) {
    sum += eigenvalues[i];
    if (sum>v)
      break;
  }

  return ForcedBaseSize(i);
}

///////////////////////////////////////////////////////////////////////////

SubSpace& SubSpace::ForcedEigenValueSqrSumBaseSize(double v) {
  int i = 0;
  double sum = 0;
  for (; i<BaseSize(); i++) {
    sum += eigenvalues[i]*eigenvalues[i];
    if (sum>v)
      break;
  }

  return ForcedBaseSize(i);
}

///////////////////////////////////////////////////////////////////////////

FloatVectorSet SubSpace::Projection(FloatVectorSource& in,
				    const FloatVector *mm) const {
  FloatVectorSet out;
  const FloatVector *vec;
  in.First();

  while ((vec = in.Next())) {
    FloatVector tmp = *vec;
    if (mm)
      tmp -= *mm;

    out.Append(new FloatVector(Projection(tmp)));
  }

  return out;
}

///////////////////////////////////////////////////////////////////////////

FloatVectorSet SubSpace::Coefficients(FloatVectorSource& in,
				      const FloatVector *mm) const {
  FloatVectorSet out;
  const FloatVector *vec;
  in.First();

  while ((vec = in.Next())) {
    FloatVector tmp = *vec;
    if (mm)
      tmp -= *mm;

    out.Append(new FloatVector(Coefficients(tmp, FALSE)));
  }

  return out;
}

///////////////////////////////////////////////////////////////////////////

FloatVector SubSpace::Coefficients(const FloatVector& v, int m,
                                   int dim) const {
  bool debug = false;
  SolveBaseSize(dim);
  if (m) {
    FloatVector tmp = v-mean;
    if (debug)
      for (int i=0; i<tmp.Length(); i++) {
        char xxx[100];
        sprintf(xxx, "%2d %9g %9g %9g %9g %9g", i, v[i], mean[i], tmp[i],
                GetColumn(0)->Get(i), GetColumn(0)->Get(i)*tmp[i]); 
        cout << xxx << endl;
      }
    return FloatMatrix::PreTransMultiply(tmp, dim);
  }

  return FloatMatrix::PreTransMultiply(v, dim);
}

///////////////////////////////////////////////////////////////////////////

// FloatVectorSet SubSpace::Projection(/*const*/ DataSet& in,
// 				    const FloatVector *mean) const {
//   FloatVectorSet out;
//   const FloatVector *vec;
//   in.Rewind();
// 
//   while (vec = in.Get()) {
//     if (mean)
//       out.Append(Projection(*vec-*mean).Copy());
//     else
//       out.Append(Projection(*vec).Copy());
//   }
// 
//   return out;
// }

///////////////////////////////////////////////////////////////////////////

int SubSpace::FromMatrix() {
  FloatVector *ptr = GetColumn(Columns()-1);
  if (!ptr)
    return FALSE;

  mean = *ptr;
  if (!RemoveColumn(Columns()-1))
    return FALSE;

  ptr = GetColumn(Columns()-1);
  if (!ptr)
    return FALSE;

  eigenvalues = *ptr;
  if (!RemoveColumn(Columns()-1))
    return FALSE;

  eigenvalues.Lengthen(Columns());

  char *tmp = (char*)CopyString(GetColumn(0)->Label());
  if (tmp) {
    if (strchr(tmp+1, ';')) {
      ForcedBaseSize(atoi(strchr(tmp+1, ';')+1));
      *strchr(tmp+1, ';') = 0;
    }
    Label(tmp);
    GetColumn(0)->Label(NULL);
  }
  delete tmp;

  return Check();
}

///////////////////////////////////////////////////////////////////////////

FloatMatrix SubSpace::ToMatrix() const {
  Check();

  FloatMatrix m(*this);
  FloatVector *v = new FloatVector(eigenvalues);
  v->Lengthen(VectorLength());
  m.AppendColumn(v);
  m.AppendColumn(new FloatVector(mean));

  if (ForcedBaseSize()>=0) {
    char *tmp = new char[strlen(Label())+100];
    sprintf(tmp, "%s;%d", Label(), ForcedBaseSize());
    m.GetColumn(0)->Label(tmp);
    delete tmp;
  } else
    m.GetColumn(0)->Label(Label());

  return m;
}

///////////////////////////////////////////////////////////////////////////

int SubSpace::Check() const {
  int r = TRUE;

  if (Columns()==0) {
    cerr << "SubSpace::Check empty subspace" << endl;
    r = FALSE;
  }

  if (Columns()!=eigenvalues.Length()) {
    cerr << "SubSpace::Check columns=" << Columns()
	 << " length=" << eigenvalues.Length() << endl;
    r = FALSE;
  }

  for (int i=0; r && i<Columns(); i++) {
    if (GetColumn(i)->Length()!=VectorLength()) {
      cerr << "SubSpace::Check column " << i << " vector length "
	   << GetColumn(i)->Length() << " mean's length "
	   << VectorLength() << endl;
      r = FALSE;
    }
    if (GetColumn(i)->Label()) {
      cerr << "SubSpace::Check column " << i << " has label "
	   << GetColumn(i)->Label() << endl;
      r = FALSE;
    }
  }  

  return r;
}

///////////////////////////////////////////////////////////////////////////

void SubSpace::CoefficientSigns(const FloatVector& vec, int subm) const {
  FloatVector c = Coefficients(vec, subm);

  for (int i=0; i<EffectiveBaseSize(); i++)
    cout << (c[i]>0 ? "+" : c[i]<0 ? "-" : "0");

  cout << " " << Label() << endl;
}

///////////////////////////////////////////////////////////////////////////

void SubSpace::CoefficientStatistics(FloatVectorSet& set, const FloatVector *m,
				     ostream& os, const char *file,
				     const char *mfile) {
  os << Label() << " " << EffectiveBaseSize() << "/" << BaseSize() << " "
     << Mean().Norm() << endl;

  FloatVectorSet pr = Coefficients(set, m);
  float min = pr.Minimum(), max = pr.Maximum();

  FloatVector res = set.NormSquared(m), resqrt = res.Sqrt();
  float rmax = resqrt.Maximum();
  if (rmax>max)
    max = rmax;

  int n = (int)sqrt((double)pr.Nitems());

  for (int i=0; i<=EffectiveBaseSize(); i++) {
    os << "\t\t\t";

    resqrt = res.Sqrt();
    for (int j=0; j<resqrt.Length(); j++)
      pr[j].Append(resqrt[j]);

    char tmpf[1000];
    sprintf(tmpf, "%s-res-%d", file, i);
    Histogram::Statistics(resqrt, os, tmpf, n, min, max);

    os << endl;

    if (i==EffectiveBaseSize())
      break;

    os << i+1 << " " << mean[i] << " " << eigenvalues[i] << " | ";

    FloatVector cmp = pr.Component(i);
    sprintf(tmpf, "%s-%d", file, i);
    Histogram::Statistics(cmp, os, tmpf, n, min, max);

    float v = cmp.Variance();

    os << " | "
       << eigenvalues[i]/pr.Nitems()   << " "
       << eigenvalues[i]/pr.Nitems()-v << " ";

    if (i==0)
      os << eigenvalues[i]/pr.Nitems()-v-Mean().NormSquared();

    os << endl;

    res -= cmp.Sqr();
  }

  if (mfile)
    pr.WriteGnuPlot(mfile);
}

///////////////////////////////////////////////////////////////////////////

// void SubSpace::() {
// 
// }

///////////////////////////////////////////////////////////////////////////

// void SubSpace::() {
// 
// }

///////////////////////////////////////////////////////////////////////////

// void SubSpace::() {
// 
// }

///////////////////////////////////////////////////////////////////////////

// void SubSpace::() {
// 
// }

///////////////////////////////////////////////////////////////////////////

// ostream& operator<<(ostream& os, const SubSpace& m) {
//   char buf[50];
// 
//   for (int y=0; y<m.height; y++) {
//     for (int x=0; x<m.width; x++) {
//       sprintf(buf, "%4.1f%c", m.Get(x, y), x<m.width-1 ? ' ' : '\0');
//       os << buf;
//     }
//     os << endl;
//   }
// 
//   return os;
// }

// istream& operator>>(istream& is, SubSpace& m) {
//   for (int y=0; y<m.height; y++)
//     for (int x=0; x<m.width; x++) {
//       float v;
//       is >> v;
//       m.Set(x, y, v);
//     }
//   
//   return is;
// }

} // namespace simple

///////////////////////////////////////////////////////////////////////////

