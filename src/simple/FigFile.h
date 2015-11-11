// -*- C++ -*-  @(#)$Id: FigFile.h,v 1.5 2011/12/05 09:40:11 jorma Exp $
// 
// Copyright 1994-2007 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2007 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

#ifndef _FIGFILE_H_
#define _FIGFILE_H_

#include <VectorSet.h>
#include <Transformation.h>

#define FIGVERSION "3.2"

//- FigFile
namespace simple {
class FigFile : public Simple {
public:
  class FigAttr {
  public:
    FigAttr() { compo = NULL; SetDefaults(); ResetCompo(); }

    virtual ~FigAttr() {}

    virtual void Dump(Simple::DumpMode = DumpDefault, ostream& = cout) const {}
    
    virtual const char **SimpleClassNames() const {
      static const char *n[] = { "FigFile::FigAttr", NULL }; return n; }

    void SetDefaults() {
      line_style = pen_style = forward_arrow = backward_arrow = 0;
      join_style = cap_style = 0;
      fill_color = 7;
      thickness = 1;
      pen_color = area_fill = radius = -1;
      style_val = 0.0;
      depth = 100;

      arrow_type = arrow_style = 0;
      arrow_thickness = 1.0;
      arrow_width = 60.0;
      arrow_height = 120.0;

      // color = -1;
      font = 0;
      font_flags = 0;
      font_size = 12;
      angle = 0;
    }

    stringstream *Compo() { return compo; }

    void ResetCompo() {
      delete compo;
      compo = NULL;
      minx = miny = MAXINT;
      maxx = maxy = -MAXINT;
    }

    void BeginCompo() {
      ResetCompo();
      compo = new stringstream;
    }

    void AddToCompo(int x, int y) {
      if (x<minx) minx = x;
      if (x>maxx) maxx = x;
      if (y<miny) miny = y;
      if (y>maxy) maxy = y;

      if (!compo)
	ShowError("FigFile::FigAttr::AddtoCompo() error");
    }

    void GetCompoSize(int& xmin, int& xmax, int& ymin, int& ymax) {
      if (!compo)
	ShowError("FigFile::FigAttr::GetCompoSize() error");
	
      xmin = minx; xmax = maxx; ymin = miny; ymax = maxy; }

    const char *CompoStr() {
      if (!compo)
	ShowError("FigFile::FigAttr::CompoStr() error");
    
      // this is relly not used anywhere by PicSOM, so it might not work...
      return compo ? compo->str().c_str() : NULL;
    }

    int   line_style, thickness, pen_color, fill_color;
    int   depth, pen_style, area_fill;
    float style_val;
    int   join_style, cap_style, radius, forward_arrow, backward_arrow;
    int   arrow_type, arrow_style;
    float arrow_thickness, arrow_width, arrow_height;
    int   font, font_flags; // color, 
    float font_size, angle;
    int minx, miny, maxx, maxy;

  protected:
    /// Compound is stored temporarily here.
    stringstream *compo;
  };

  /// 
  FigFile() {
    version = papersize = NULL;
    PushDefaultAttr();
    SetDefaults();
  }

  /// 
  FigFile(const FigFile& ff) : Simple(ff) {
    version = papersize = NULL;
    PushDefaultAttr();
    CopySettings(ff);
  }

  /// 
  FigFile(const char *s) {
    version = papersize = NULL;
    file.open(s);
    PushDefaultAttr();
    SetDefaults();
  }

  /// 
  virtual ~FigFile() {
    delete version;
    delete papersize;
  }

  /// 
  FigFile& operator=(const FigFile& ff) {
    CopySettings(ff);
    return *this;
  }
    
  /// All Simple-stemming classes must ...
  virtual const char **SimpleClassNames() const { static const char *n[] = {
    "FigFile", NULL }; return n; }
  virtual void Dump(Simple::DumpMode = DumpDefault, ostream& os = cout) const {
    os << "FigFile " << (void*)this
       << endl;
  }

  FigAttr& Attr() { return *stack.Peek(); }
  const FigAttr& Attr() const { return *stack.Peek(); }

  FigAttr *CompoAttr() { 
    for (int i=stack.Nitems()-1; i>=0; i--)
      if (stack[i].Compo())
	return stack.Get(i);
    return NULL;
  }

  ostream& Os() {
    if (CompoAttr())
      return *CompoAttr()->Compo();
    return file;
  }

  ///
  FigFile& Open(const char *name) {
    file.open(name);
    return *this;
  }

  ///
  FigFile& WriteHeader() {
    file << "#FIG " << version << endl
	 << ( landscape ? "Landscape" : "Portrait"   ) << endl
	 << ( centered  ? "Center"    : "Flush Left" ) << endl
	 << ( metric    ? "Metric"    : "Inches"     ) << endl
         << papersize << endl
	 << magnification << endl
         << ( multiple  ? "Multiple"  : "Single"     ) << endl
         << transpcolor << endl
         << resolution << " " << 2 << endl;

    return *this;
  }

  ///
  FigFile& Close() {
    file.close();
    return *this;
  }

  ///
  FigFile& SetDefaults() {
    CopyString(version, FIGVERSION);
    CopyString(papersize, "Letter");
    landscape = centered = true;
    metric = multiple = false;
    magnification = 100;
    transpcolor = -2;
    resolution = 1200;

    return *this;
  }

  ///
  FigFile& CopySettings(const FigFile&) {

    return *this;
  }

  ///
  FigFile& BeginCompound() {
    PushSettings();
    Attr().BeginCompo();

    return *this;
  }

  ///
  FigFile& EndCompound() {
    int x0, y0, x1, y1;
    Attr().GetCompoSize(x0, x1, y0, y1);
    x0 -= 150; y0 -= 150; x1 += 150; y1 += 150;

    const char *str = Attr().CompoStr();
    PopSettings();

    Os() << "6 " << x0<< " " << y0 << " " << x1 << " " << y1 << endl
	 << str
	 << "-6" << endl;

    delete str;

    return *this;
  }

  FigFile& PushDefaultAttr() { stack.AppendCopy(defaults); return *this; }
  FigFile& PushSettings() { stack.AppendCopy(Attr()); return *this; }
  FigFile& PopSettings()  { delete stack.Pop(); return *this; }

  void DoConv(double x, double y, int& ix, int& iy) const {
    if (inverse_y)
      y = -y;
    tr.Apply(x, y);
    ix = (int)floor(x+0.5);
    iy = (int)floor(y+0.5);
  }

  const char *Conv(double x, double y) const {
    int intx, inty;
    DoConv(x, y, intx, inty);
    static char tmp[100];
    sprintf(tmp, "%d %d", intx, inty);
    return tmp;
  }

  const char *ForwardArrowLine() const {
    const FigAttr& a = Attr();
    if (!a.forward_arrow)
      return "";

    static char tmp[100];
    sprintf(tmp, "\t%d %d %f %f %f\n", a.arrow_type, a.arrow_style,
	    a.arrow_thickness, a.arrow_width, a.arrow_height);
    return tmp;
  }
 
  const char *BackwardArrowLine() const {
    const FigAttr& a = Attr();
    if (!a.backward_arrow)
      return "";

    static char tmp[100];
    sprintf(tmp, "\t%d %d %f %f %f\n", a.arrow_type, a.arrow_style,
	    a.arrow_thickness, a.arrow_width, a.arrow_height);
    return tmp;
  }
 
  FigFile& LineBoxCommon(float *x, float *y, int n) {
    const FigAttr& a = Attr();
    Os() << a.line_style << " "
	 << a.thickness << " "
	 << a.pen_color << " "
	 << a.fill_color << " "
	 << a.depth << " "
	 << a.pen_style << " "
	 << a.area_fill << " "
	 << a.style_val << " "
	 << a.join_style << " "
	 << a.cap_style << " "
	 << a.radius << " "
	 << a.forward_arrow << " "
	 << a.backward_arrow << " "
	 << n << endl
	 << ForwardArrowLine()
	 << BackwardArrowLine();

    for (int i=0; i<n; i++) {
      int xint, yint;
      DoConv(x[i], y[i], xint, yint);
      Os() << "\t" << xint << " " << yint << endl;

      if (CompoAttr())
	CompoAttr()->AddToCompo(xint, yint);
//       else
// 	cout << "no compo" << endl;
    }
    return *this;
  }

  FigFile& Box(double x0, double y0, double x1, double y1) {
    float fx[5] = { (float)x0, (float)x1, (float)x1, (float)x0, (float)x0 };
    float fy[5] = { (float)y0, (float)y0, (float)y1, (float)y1, (float)y0 };
    Os() << "2 2 ";
    return LineBoxCommon(fx, fy, 5);
  }

  FigFile& LineCommon(float *x, float *y, int n) {
    Os() << "2 1 ";
    return LineBoxCommon(x, y, n); 
  }

  FigFile& Line(double x0, double y0, double x1, double y1) {
    float fx[2] = { (float)x0, (float)x1 };
    float fy[2] = { (float)y0, (float)y1 };
    return LineCommon(fx, fy, 2);
  }

  FigFile& Line(const FloatPointVector& vec) {
    float *fx = new float[vec.Length()], *fy = new float[vec.Length()];

    for (int i=0; i<vec.Length(); i++) {
      fx[i] = vec[i].X();
      fy[i] = vec[i].Y();
    }

    LineCommon(fx, fy, vec.Length());

    delete fx;
    delete fy;

    return *this;
  }

  FigFile& Point(double x, double y) {
    float fx = x, fy = y;
    return LineCommon(&fx, &fy, 1);
  }

  FigFile& PointSet(const FloatVectorSet& set) {
    for (int i=0; i<set.Nitems(); i++)
      Point(set[i][0], set[i][1]);

    return *this;
  }


  FigFile& TextCommon(double x, double y, const char *txt,
		      double l=0, double h=0) {
    const FigAttr& a = Attr();
    Os() << a.pen_color << " "
	 << a.depth << " "
	 << a.pen_style << " "
	 << a.font << " "
	 << a.font_size << " "
	 << a.angle << " "
	 << a.font_flags << " "
	 << h << " " << l << " " << Conv(x, y)
	 << " " << txt << "\\001" << endl;
    return *this;    
  }

  FigFile& LeftText(double x, double y, const char *txt,
		    double l=0, double h=0) {
    Os() << "4 0 ";
    return TextCommon(x, y, txt, l, h);
  }

  FigFile& CenterText(double x, double y, const char *txt,
		      double l=0, double h=0) {
    Os() << "4 1 ";
    return TextCommon(x, y, txt, l, h);
  }

  FigFile& RightText(double x, double y, const char *txt,
		      double l=0, double h=0) {
    Os() << "4 2 ";
    return TextCommon(x, y, txt, l, h);
  }

  FigFile& ForwardArrow(int v) { Attr().forward_arrow = v; return *this; }
  FigFile& Thickness(int v)    { Attr().thickness     = v; return *this; }
  FigFile& StyleVal(double v)  { Attr().style_val     = v; return *this; }
  FigFile& PenColor(int v)     { Attr().pen_color     = v; return *this; }
  FigFile& FillColor(int v)    { Attr().fill_color    = v; return *this; }
  FigFile& Depth(int v)        { Attr().depth         = v; return *this; }
  FigFile& LineStyle(int v)    { Attr().line_style    = v; return *this; }
  FigFile& JoinStyle(int v)    { Attr().join_style    = v; return *this; }
  FigFile& CapStyle(int v)     { Attr().cap_style     = v; return *this; }
  FigFile& FontSize(double v)  { Attr().font_size     = v; return *this; }

  FigFile& PenColor(const char *n)  { return PenColor(ColorIdx(n));      }
  FigFile& FillColor(const char *n) { return FillColor(ColorIdx(n));     }
  FigFile& LineStyle(const char *n) { return LineStyle(LineStyleIdx(n)); }
  FigFile& JoinStyle(const char *n) { return JoinStyle(JoinStyleIdx(n)); }
  FigFile& CapStyle(const char *n)  { return CapStyle(CapStyleIdx(n));   }

  static int CommonNameValue(const char **list, const char *name, int def) {
    for (int i=0; list[i]; i++)
      if (!strcmp(list[i], name))
	return i;
    return def;
  }

  static const char **Colornames() {
    static const char *names[] = {
      "Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Yellow", "White",
	"Blue4", "Blue3", "Blue2", "LtBlue", "Green4", "Green3", "Green2",
	"Cyan4", "Cyan3", "Cyan2", "Red4", "Red3", "Red2", 
	"Magenta4", "Magenta3", "Magenta2", "Brown4", "Brown3", "Brown2", 
	"Pink4", "Pink3", "Pink2", "Pink", "Gold", NULL };
    return names;
  }

  static int ColorIdx(const char *c) {
    return CommonNameValue(Colornames(), c, -1);
  }

  static int LineStyleIdx(const char *c) {
    static const char *names[] = {
      "Solid", "Dashed", "Dotted", "Dash-dotted", 
	"Dash-double-dotted", "Dash-triple-dotted", NULL };

    return CommonNameValue(names, c, -1);
  }

  static int JoinStyleIdx(const char *c) {
    static const char *names[] = {
	"Miter", "Bevel", "Round", NULL };

    return CommonNameValue(names, c, 0);
  }

  static int CapStyleIdx(const char *c) {
    static const char *names[] = {
      "Butt", "Round", "Projecting", NULL };

    return CommonNameValue(names, c, 0);
  }

  double Inverse(double l) { return l/tr.Scale(); }

  FigFile& SetTransformation(double x, double y, double s) {
    tr.Set(x, y, s); return *this;
  }

  FigFile& InverseY(int v) { inverse_y = v; return *this; }
  

protected:
  /// Output goes here.
  ofstream file;

  /// #FIG version
  char *version;

  /// Landscape/Portrait
  bool landscape;

  /// Center/Flush Left
  bool centered;
  
  /// Metric/Inches
  bool metric;

  /// Letter/Legal/...
  char *papersize;

  /// 100.0
  float magnification;

  /// Single/Multiple
  bool multiple;

  /// -2, -1, ...
  int transpcolor;

  /// 1200, ....
  int resolution;

  /// Default settings are const.
  const FigAttr defaults;

  /// Saved settings.
  ListOf<FigAttr> stack;  

  /// Dunno
  Transformation tr;

  /// Dunno
  int inverse_y;
};

} // namespace simple
#endif // _FIGFILE_H_
