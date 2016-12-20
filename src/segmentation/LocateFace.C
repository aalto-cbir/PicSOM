
#include <LocateFace.h>

#include <cmath>

#include <cox/fft>
#include <cox/matrix>
//#include <matrix>
#include <imagecolor.h>

#include <string.h>
#include <stdio.h>

#define myMIN(x,y) (x < y ? x : y) // Required by IsInside.
#define myMAX(x,y) (x > y ? x : y)

#define round(x) (int)floor((x)+0.5)
#define f_ellipse_s(x,y,x0,y0,a,b) ((x)-(x0))*((x)-(x0))/((a)*(a)) + ((y)-(y0))*((y)-(y0))/((b)*(b)) - 1

namespace picsom {

  static const string vcid="$Id: LocateFace.C,v 1.15 2009/04/30 09:54:31 vvi Exp $";

  static LocateFace list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  const char *LocateFace::Version() const { 
    return vcid.c_str();
  }

  /////////////////////////////////////////////////////////////////////////////

  void LocateFace::UsageInfo(ostream& os) const { 
    os << "LocateFace :" << endl
       << "  options : " << endl
       << "    -raw                    Set the head box to whole image to the result XML file." << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  int LocateFace::ProcessOptions(int argc, char** argv) {
    int argc_old=argc;
    int res;
    while(argv[0][0]== '-' && argc > 1) {

      // "-raw"
      res = strcmp(&argv[0][1],"raw");
      if(res == 0) {
      	bRaw = true;
        argc--;
        argv++;
      }
    }
    return argc_old-argc;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool LocateFace::Process() {
    if (!ProcessRoz())
      return false;
    
    /*
    if (Verbose()>1)
      ShowLinks();

    bool new_method = false;
    if (!new_method) {
      if (!ProcessJpakkane())
	return false;
    }
    else
      if (!ProcessNew())
	return false;
    */
    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////
  bool LocateFace::ProcessRoz() {
    if (bRaw){
      // if bRaw==true, set both face box to the image box
      // and make the head box
      // and save the result.
      stringstream facebox;
      facebox << (int)0 << " " << (int)0 << " "
	      << (int)Width()-1 << " " << (int)Height()-1;
      
      SegmentationResult myresult;
      myresult.name="face";
      myresult.type="box";
      myresult.value=facebox.str();
      AddResultToXML(myresult);
      
      stringstream headbox;
      int side = myMAX((int)Width(), (int)Height());
      int ltx = (int)(Width()-side)/2;
      int lty = (int)(Height()-side)/2;
      int rbx = ltx + side - 1;
      int rby = lty + side - 1;
      headbox << ltx << " " << lty << " "
	      << rbx << " " << rby;
      
      myresult.name="head";
      myresult.type="box";
      myresult.value=headbox.str();
      AddResultToXML(myresult);
      	
      return true;
    }  	
  	
    const int sectors = 360;
    const int maxside = Width() > Height() ? Width():Height();
    double x0 = Width()/2;
    double y0 = Height()/2;
    const int foudlevel = 5;
    vector<complex<cmplx_t> > coordxy = vector<complex<cmplx_t> >(sectors);
    coord polygon[sectors+1];
    // double degree = M_PI*2/360;
    int angle1 = 30;
    int angle2 = 150;
    vector<double> coef(5);
    double ld,sd;

    CalculateGradient();

    // get the polar coordinates in sweepimg member
    SweepImage(sectors, round(x0), round(y0));

    // find the max change (border) along each radial line of each sector
    FindMaxOnLines(coordxy, round(x0), round(y0), (double)maxside, 0.08, 0.1);

    if (Verbose()>2)
      ShowPoints(coordxy, "before-Foud");
    
    // smoothen the border points
    Foud(coordxy, foudlevel);
    
    if (Verbose()>2)
      ShowPoints(coordxy, "after-Foud");
    
    // fit an ellipse using the points in [0,angle1] and [angle2,sectors]
    LocateRoughEllipse(coordxy, angle1, angle2, coef);
    
    if (Verbose()>2)
      cout << "coef = (" << coef[0] << "," << coef[1] << "," << coef[2] << "," << coef[3] << "," << coef[4] << ")"<< endl;

    // convert the coefficients to parameters of standard form 
    ConvertEllipseParameters(coef, x0, y0, ld, sd);

    if (Verbose()>2)
      cout << "x0 = " << x0
	   << "y0 = " << y0
	   << "ld = " << ld
	   << "sd = " << sd
	   << endl;

    // adjust the ellipse for the border searching in the second round
    sd = sd * 1.1;
    ld = sd * 1.4;

    // set the segment to mask the second round
    for (int i = 0; i < Width(); i++) {
      for (int j = 0; j < Height(); j++) {
	if (j > round(y0) && f_ellipse_s(i,j,x0,y0,sd,ld)>0)
	  SetPixelSegment(i, j, 0);
      }
    }

    // get the polar coordinates again, masked by previous segmentation result
    SweepImage(sectors, round(x0), round(y0));

    // find the max change (border) again
    FindMaxOnLines(coordxy, round(x0), round(y0), sd, 0.75, 0.2);

    if (Verbose()>2)
      ShowPoints(coordxy, "before-Foud");

    // smoothen the border points
    Foud(coordxy, foudlevel);

    if (Verbose()>2)
      ShowPoints(coordxy, "after-Foud");

    // ensure the border points within the image rectangle
    for (int i=0;i<sectors;i++)
      {
	double ix = coordxy[i].real(), iy = coordxy[i].imag();
	if (ix<0)
	  ix = 0;
	if (iy<0)
	  iy = 0;
	if (ix>=Width())
	  ix = Width();
	if (iy>=Height())
	  iy = Height();
	coordxy[i] = complex<cmplx_t>(ix, iy);
      }

    if (Verbose()>1)
      ShowPoints(coordxy, "final");

    // set the final segementation result
    double maxx = 0;
    double maxy = 0;
    double minx = Width();
    double miny = Height();
    for (int i = 0; i < sectors; i++) {
      if (coordxy[i].real() > maxx)
	maxx = coordxy[i].real();
      if (coordxy[i].real() < minx)
	minx = coordxy[i].real();
      if (coordxy[i].imag() > maxy)
	maxy = coordxy[i].imag();
      if (coordxy[i].imag() < miny)
	miny = coordxy[i].imag();

      polygon[i].x = round(coordxy[i].real());
      polygon[i].y = round(coordxy[i].imag());
    }
    polygon[sectors] = polygon[0]; // Required by the IsInside function.

    // Now marking the segmented face area. 
    for (int x=0; x < Width(); x++) {
      for (int y=0; y < round(miny); y++)
	SetPixelSegment(x, y, 0);
      for (int y=round(maxy)+1; y < Height(); y++)
	SetPixelSegment(x, y, 0);
    }

    for (int y=round(miny); y <= round(maxy); y++) {
      for (int x=0; x < Width(); x++) {
	bool in = IsInside(coord(x, y), polygon, sectors);
	SetPixelSegment(x, y, in);
      }
    }

    // Facebox is the minimum box that contains the face.
    stringstream facebox;
    facebox << (int)minx << " " << (int)miny << " "
	    << (int)maxx << " " << (int)maxy;

    SegmentationResult myresult;
    myresult.name="face";
    myresult.type="box";
    myresult.value=facebox.str();
    AddResultToXML(myresult);

    // Now creating another box that is a square.
    if ((maxx-minx) < (maxy-miny)) { // Vertical
      minx = (int)((minx+maxx-maxy+miny)/2);
      maxx = minx + (maxy-miny);
    }
    else {
      miny = (miny+maxy)/2-(maxx-minx)/2;
      maxy = miny + (maxx-minx);
    }

    // Headbox is a square centered at the face. It is less tightly
    // lined than FaceBox.
    stringstream headbox;
    headbox << (int)minx << " " << (int)miny << " "
	    << (int)maxx << " " << (int)maxy;

    myresult.name="head";
    myresult.type="box";
    myresult.value=headbox.str();
    AddResultToXML(myresult);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool LocateFace::LocateRoughEllipse(vector<complex<cmplx_t> >& coordxy, int angle1, int angle2,  vector<double>& coef){
  using cox::matrix;

  int sectors = coordxy.size();

  int n = angle1 + (sectors-angle2);
  
  vector<double> x(n), y(n);
  for (int i=0,j=0;i<sectors;i++)
    {
      if (i<angle1 || i>=angle2)
	{
	  x[j] = coordxy[i].real();
	  y[j] = coordxy[i].imag();
	  j++;
	}
    }
    
  vector<double> xx(n),yy(n),ones(n);
  for (int i=0;i<n;i++){
    xx[i] = x[i] * x[i];
    yy[i] = y[i] * y[i];
    ones[i] = 1;
  }
  
  matrix<double> D(n,5);
  
  D.set_column(0, xx);
  D.set_column(1, yy);
  D.set_column(2, x);
  D.set_column(3, y);
  D.set_column(4, ones);
  
  matrix<double> DT = D.transpose();
  
  matrix<double> S = DT*D;
  
  matrix<double> C(5,5);
  for(int i=0;i<5;i++)
    for(int j=0;j<5;j++)
      C(i,j) = 0;
  C(0,1) = -2.0;
  C(1,0) = -2.0;
  
  pair<list<vector<double> >, vector<double> > ret = S.generalized_eigenvectors(C);

  list<vector<double> > gevec = ret.first;
  vector<double> geval = ret.second;
  
  int i=0;
  list<vector<double> >::iterator iter;
  for (iter=gevec.begin();iter!=gevec.end();iter++)
    {
      if (geval[i]<0 && abs(geval[i])<1e+20-10)  //simulate ~isinf(X)
	{
	  coef = *iter;
	  return true;
	}
      i++;
    }            
  return false; //no qualified eigen value found
  }

  /////////////////////////////////////////////////////////////////////////////

  bool LocateFace::ConvertEllipseParameters(vector<double>& coef, double& x0, double& y0, double& ld, double& sd) {
  double a,b,c,d,e,fo;
  a = coef[0];
  b = coef[1];
  c = coef[2];
  d = coef[3];
  e = coef[4];
  x0 = -c/(2*a);
  y0 = -d/(2*b);
  fo = -e + c*c/(4*a) + d*d/(4*b);
  if (fo/a<0 ||fo/b<0)
    return false;
  if (b<a)
    {
      ld = sqrt(fo/a);
      sd = sqrt(fo/b);
    }
  else
    {
      ld = sqrt(fo/b);
      sd = sqrt(fo/a);
    }
  return true;
 }

  /////////////////////////////////////////////////////////////////////////////

  bool LocateFace::ProcessJpakkane() {
  const int sectors = 256;
  const int maxside = Width() > Height() ? Width():Height();
  int x0 = Width()/2;
  int y0 = Height()/2;
  const int foudlevel = 5;
  vector<complex<cmplx_t> > coordxy = vector<complex<cmplx_t> >(sectors);
  coord polygon[sectors+1];
  const double base = 0.08;
  
  CalculateGradient();

  SweepImage(sectors, x0, y0);

  FindMaxOnLines0(coordxy, x0, y0, maxside, base);

  if (Verbose()>2)
    ShowPoints(coordxy, "max-on-lines-1");

  Foud(coordxy, foudlevel);

  double maxy = 0, maxx = 0;
  double minx = Width(), miny = Height();

  for (int i = 0; i < sectors; i++) {
    if (coordxy[i].real() > maxx)
      maxx = coordxy[i].real();
    if (coordxy[i].real() < minx)
      minx = coordxy[i].real();
    if (coordxy[i].imag() > maxy)
      maxy = coordxy[i].imag();
    if (coordxy[i].imag() < miny)
      miny = coordxy[i].imag();
  }
  miny = 0.9*miny;
  minx = 0.9*minx;
  maxy = 1.1*maxy;
  maxx = 1.1*maxx;

  // Marking the background area.
  for (int i = 0; i < Width(); i++) {
    for (int j = 0; j < Height(); j++) {
      if (i < minx || j < miny || i > maxx || j > maxy)
	SetPixelSegment(i, j, 0);
    }
  }

  RemoveEdgeErrors(coordxy, x0);

  SweepImage(sectors, x0, y0);

  FindMaxOnLines0(coordxy, x0, y0, maxside, base);

  if (Verbose()>2)
    ShowPoints(coordxy, "max-on-lines-2");

  Foud(coordxy, foudlevel);

  // Find maxima and minima.
  maxx = 0;
  maxy = 0;
  minx = Width();
  miny = Height();
  for (int i = 0; i < sectors; i++) {
    if (coordxy[i].real() > maxx)
      maxx = coordxy[i].real();
    if (coordxy[i].real() < minx)
      minx = coordxy[i].real();
    if (coordxy[i].imag() > maxy)
      maxy = coordxy[i].real();
    if (coordxy[i].imag() < miny)
      miny = coordxy[i].real();
  }

  /*
  // Marking the background area.
  for (int i = 0; i < Width(); i++) {
    for (int j = 0; j < Height(); j++) {
      if (i < minx || j < miny || i > maxx || j > maxy)
        SetPixelSegment(i, j, 0);
    }
  }
  // We have the wanted fourier description coordinates.
  // Here we mark them on the segmented image.
  for(i = 0; i < sectors; i++)
     SetPixelSegment(coordxy[i].real(), coordxy[i].imag(), 255 * (i%2));
  */

  if (Verbose()>2)
    ShowPoints(coordxy, "fd");

  // Find maxima and minima.
  maxx = 0;
  maxy = 0;
  minx = Width();
  miny = Height();
  for (int i = 0; i < sectors; i++) {
    if (coordxy[i].real() > maxx)
      maxx = coordxy[i].real();
    if (coordxy[i].real() < minx)
      minx = coordxy[i].real();
    if (coordxy[i].imag() > maxy)
      maxy = coordxy[i].imag();
    if (coordxy[i].imag() < miny)
      miny = coordxy[i].imag();

    polygon[i].x = (int)floor(coordxy[i].real()+0.5);
    polygon[i].y = (int)floor(coordxy[i].imag()+0.5);
  }
  polygon[sectors] = polygon[0]; // Required by the IsInside function.
  maxx = (int) maxx;
  minx = (int) minx;
  maxy = (int) maxy;
  miny = (int) miny;

  // Now marking the segmented face area. 
  for (int x=0; x < Width(); x++) {
    for (int y=0; y < miny; y++)
      SetPixelSegment(x, y, 0);
    for (int y=(int)maxy+1; y < Height(); y++)
      SetPixelSegment(x, y, 0);
  }
  
  for (int y=(int)miny; y <= maxy; y++) {
    for (int x=0; x < Width(); x++) {
      bool in = IsInside(coord(x, y), polygon, sectors);
      SetPixelSegment(x, y, in);
      if (Verbose()>3)
	cout << "LocateFace : (" << x << "," << y << ")=" << in << endl;
    }
  }

  // Facebox is the minimum box that contains the face.
  stringstream facebox;
  facebox << (int)minx << " " << (int)miny << " "
	  << (int)maxx << " " << (int)maxy;

  if (Verbose()>1)
    cout << "LocateFace : facebox=" << facebox.str() << endl;

  SegmentationResult myresult;
  myresult.name="face";
  myresult.type="box";
  myresult.value=facebox.str();
  AddResultToXML(myresult);
  // getImg()->writeFrameResultToXML(this, myresult);

  // Now creating another box that is a square.
  if ((maxx-minx) < (maxy-miny)) { // Vertical
    minx = (int)((minx+maxx-maxy+miny)/2);
    maxx = minx + (maxy-miny);
  }
  else {
    miny = (miny+maxy)/2-(maxx-minx)/2;
    maxy = miny + (maxx-minx);
  }

  // Headbox is a square centered at the face. It is less tightly
  // lined than FaceBox.
  stringstream headbox;
  headbox << (int)minx << " " << (int)miny << " "
	  << (int)maxx << " " << (int)maxy;

  if (Verbose()>1)
    cout << "LocateFace : headbox=" << headbox.str() << endl;

  myresult.name="head";
  myresult.value=headbox.str();
  AddResultToXML(myresult);
  // getImg()->writeFrameResultToXML(this, myresult);

  /*
  // Ja tama on sitten testauskayttoon, laittaa rajatun alueen
  // sisalle sen kohdan naaman.
  float R, G, B;
  for(i=0; i<Width(); i++)
    for(j=0; j<Height(); j++) {
      if(j < miny || j > maxy || i < minx || i > maxx) {
	SetPixelSegment(i, j, 0);
	  }
      else {
	GetPixelRGB(i, j, R, G, B);
	SetPixelSegment(i, j, (R+G+B)/3);
      }
    }

  */

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void LocateFace::ShowPoints(const vector<complex<cmplx_t> >& v,
			      const string& s) const {
    imagedata img = *getImg()->accessFrame();
    img.force_three_channel();
    for (size_t i=0; i<v.size(); i++)
      img.set((int)floor(v[i].real()+0.5), (int)floor(v[i].imag()+0.5),
	      imagecolor<float>("red"));
    
    imagefile::displaysettings stgs;
    stgs.title = s;
    imagefile::display(img, stgs);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool LocateFace::ProcessNew() {
    const int sectors = 256;
    //    const double angle = M_PI*2/sectors; // angle is measured as radians.
    //    const int maxside = Width() > Height() ? Width():Height();
    //    int i,j;
    //    int x0 = Width()/2;
    //    int y0 = Height()/2;
    //    const int foudlevel = 5;
    vector<complex<cmplx_t> > coords(sectors);
    //    coord polygon[sectors+1];
    //    const double base = 0.08;
    //    const double ampl = 0.1;

    debug_disp_set.fork   = true;
    debug_disp_set.resize = false;
    
    try {
      CalculateGradient();
    }
    catch (const string& s) {
      cerr << "Cannot calculate gradient in LocateFace::Process(): "
	   << s << endl;
      return false;
    }

    int xmid = -1;
    try {
      xmid = HorizontalSymmetryAxis(gradimg);
      cout << "xmid=" << xmid << endl;
    }
    catch (const string& s) {
      cerr << "Unable to calculate HorizontalSymmetryAxis in "
	   << "LocateFace::Process(): " << s << endl;
      return false;
    }

    try {
      int ycirc = MatchUpperHalfCircles(gradimg, xmid);
      cout << "ycirc=" << ycirc << endl;
    }
    catch (const string& s) {
      cerr << "Unable to calculate MatchUpperHalfCircles in "
	   << "LocateFace::Process(): " << s << endl;
      return false;
    }

    /*
    try {
      SweepImage(360, x0, y0);
    }
    catch (const string& s) {
      cerr << "Unable to calculate sweep in LocateFace::Process(): "
	   << s << endl;
      return false;
    }
    */

    /*

    FindMaxOnLines(coords, x0, y0, maxside, base, ampl);

    for(i = 0; i < sectors; i++)
      SetPixelSegment((int)coords[i].real()(), (int)coords[i].imag(), 200);

    Foud(coords, foudlevel);

    double maxx, maxy, minx, miny;
    maxy = maxx = 0;
    minx = Width();
    miny = Height();

    for(i = 0; i < sectors; i++) {
      if(coords[i].real() > maxx)
        maxx = coords[i].real();
      if(coords[i].real() < minx)
        minx = coords[i].real();
      if(coords[i].imag() > maxy)
        maxy = coords[i].imag();
      if(coords[i].imag() < miny)
        miny = coords[i].imag();
    }
    miny = 0.9*miny;
    minx = 0.9*minx;
    maxy = 1.1*maxy;
    maxx = 1.1*maxx;

    // Marking the background area.
    for(i = 0; i < Width(); i++) {
      for(j = 0; j < Height(); j++) {
        if(i < minx || j < miny || i > maxx || j > maxy)
  	SetPixelSegment(i, j, 0);
      }
    }
    RemoveEdgeErrors(coords, x0);

    SweepImage(angle, x0, y0);
    FindMaxOnLines(sweeped, coords, x0,y0, maxside, base, ampl);
  /!*
    for(i = 0; i < sectors; i++)
      SetPixelSegment(coords[i].real(), coords[i].imag(), 100);
  *!/
    Foud(coords, foudlevel);

    // Find maxima and minima.
    maxx = 0;
    maxy = 0;
    minx = Width();
    miny = Height();
    for(i = 0; i < sectors; i++) {
      if(coords[i].real() > maxx)
        maxx = coords[i].real();
      if(coords[i].real() < minx)
        minx = coords[i].real();
      if(coords[i].imag() > maxy)
        maxy = coords[i].imag();
      if(coords[i].imag() < miny)
        miny = coords[i].imag();
    }

    /!*
    // Marking the background area.
    for(i = 0; i < Width(); i++) {
      for(j = 0; j < Height(); j++) {
        if(i < minx || j < miny || i > maxx || j > maxy)
          SetPixelSegment(i, j, 0);
      }
    }
    // We have the wanted fourier description coordinates.
    // Here we mark them on the segmented image.
    for(i = 0; i < sectors; i++)
       SetPixelSegment(coords[i].real(), coords[i].imag(), 255 * (i%2));
    *!/

    // Find maxima and minima.
    maxx = 0;
    maxy = 0;
    minx = Width();
    miny = Height();
    for(i = 0; i < sectors; i++) {
      if(coords[i].real() > maxx) {
        maxx = coords[i].real();
      }
      if(coords[i].real() < minx) {
        minx = coords[i].real();
      }
      if(coords[i].imag() > maxy) {
        maxy = coords[i].imag();
      }
      if(coords[i].imag() < miny) {
        miny = coords[i].imag();
      }
      polygon[i].x = (int)floor(coords[i].real()+0.5);
      polygon[i].y = (int)floor(coords[i].imag()+0.5);
    }
    polygon[sectors] = polygon[0]; // Required by the IsInside function.

    // int imaxx = (int) maxx;
    // int iminx = (int) minx;
    int imaxy = (int) maxy;
    int iminy = (int) miny;

    // Now marking the segmented face area. 
    int x, y;
    for(x=0; x < Width(); x++)
      for(y=0; y < iminy; y++)
        SetPixelSegment(x, y, 0);
    
    for(y=iminy; y <= imaxy; y++) {
      for(x=0; x < Width(); x++) {
        if(IsInside(coord(x, y), polygon, sectors))
  	SetPixelSegment(x, y, 1); // This pixel is a part of the face.
        else
  	SetPixelSegment(x, y, 0); // This one isn't.
      }
    }

    for(x=0; x < Width(); x++) 
      for(y=imaxy+1; y < Height(); y++)
        SetPixelSegment(x, y, 0);

    // Facebox is the minimum box that contains the face.
    char boxarea[100]; // Should be enough to hold 4 coordinates.
    sprintf(boxarea, "%d %d %d %d", (int)minx, (int)miny, (int)maxx, (int)maxy);
    SegmentationResult facebox("face", "box", boxarea);
    AddResultToXML(facebox);
    // getImg()->writeFrameResultToXML(this, facebox);

    // Now creating another box that is a square.
    if((maxx-minx) < (maxy-miny)) { // Vertical
      minx = (int)((minx+maxx-maxy+miny)/2);
      maxx = minx + (maxy-miny);
    }
    else {
      miny = (miny+maxy)/2-(maxx-minx)/2;
      maxy = miny + (maxx-minx);
    }

    // Boundingbox is a square centered at the face. It is less tightly
    // lined than FaceBox.
    sprintf(boxarea, "%d %d %d %d", (int)minx, (int)miny, (int)maxx, (int)maxy);
    SegmentationResult headbox("head", "box", boxarea);
    AddResultToXML(headbox);
    // getImg()->writeFrameResultToXML(this, headbox);

    /!*
    // Ja tama on sitten testauskayttoon, laittaa rajatun alueen
    // sisalle sen kohdan naaman.
    float R, G, B;
    for(i=0; i<Width(); i++)
      for(j=0; j<Height(); j++) {
        if(j < miny || j > maxy || i < minx || i > maxx) {
  	SetPixelSegment(i, j, 0);
  	  }
        else {
  	GetPixelRGB(i, j, R, G, B);
  	SetPixelSegment(i, j, (R+G+B)/3);
        }
      }

    *!/

    */

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  void LocateFace::CalculateGradient() {
    StartMessage("LocateFace::CalculateGradient");

    vector<float> h(3), v(3, 1.0);
    h[0] = -(h[2]=1);
    gradimg = getImg()->accessFrame()->absolute_gradient(h, v);
    gradimg.force_one_channel();

    DebugDisplay(gradimg);
    FinishMessage("LocateFace::CalculateGradient");
  }

  /////////////////////////////////////////////////////////////////////////////

  int LocateFace::HorizontalSymmetryAxis(const imagedata& img) const {
    StartMessage("LocateFace::HorizontalSymmetryAxis");

    unsigned int w = img.width(), h = img.height();
    unsigned int xb = w/12, xe = w-xb;
    unsigned int yb = h/3,  ye = h-yb;
    unsigned int rw = w/2,  dx = (w-rw)/2;

    imagedata r(rw, img.height(), 1);

    for (unsigned int x=0; x<r.width(); x++)
      for (unsigned int y=0; y<r.height(); y++) {
	float s = 0;
	for (unsigned int i=1; i<img.width(); i++) {
	  int lx = x+dx-i, rx = x+dx+i;
	  if (lx<(int)xb || rx>=(int)xe)
	    break;
	  s += img.get_one_float(lx, y)*img.get_one_float(rx, y);
	}
	r.set(x, y, s);
      }

    float maxs = 0;
    int   maxx = -1;
    vector<float> rsum(r.width());
    for (unsigned int x=0; x<r.width(); x++) {
      float s = 0;
      for (unsigned int y=yb; y<ye; y++)
	s += r.get_one_float(x, y);
      rsum[x] = s;
      if (s>maxs) {
	maxs = s;
	maxx = x;
      }
    }

    r.normalize_01();

    if (DebugDisplay()) {
      imagedata tmp = r;
      tmp.force_three_channel();
      for (unsigned int y=0; y<tmp.height(); y++) {
	vector<float> v = tmp.get_float(maxx, y);
	v[1] = v[2] = 0;
	tmp.set(maxx, y, v);
      }
      DebugDisplay(tmp);
    }

    FinishMessage("LocateFace::HorizontalSymmetryAxis");

    return maxx+dx;
  }

  /////////////////////////////////////////////////////////////////////////////

  int LocateFace::MatchUpperHalfCircles(const imagedata& img,
					unsigned int x) const {
    StartMessage("LocateFace::MatchUpperHalfCircles");

    unsigned int w = img.width(); //, h = img.height();
    unsigned int maxx = x>w/2 ? w-x-1 : x;

    vector<float> hf(6);
    hf[0] = hf[1] = hf[2] = -1;
    hf[3] = hf[4] = hf[5] = +1;

    for (unsigned int y = 160; y<=220; y+=20) {

      unsigned int maxr = y<maxx ? y : maxx;
      vector<float> rho(2*maxr), phi(180);
      for (unsigned int i=0; i<rho.size(); i++)
	rho[i] = i;
      for (unsigned int i=0; i<phi.size(); i++)
	phi[i] = M_PI*i/phi.size()-M_PI;

      imagedata pol = img.to_polar(rho, phi, x, y);
      DebugDisplay(pol);

      imagedata hgrad = pol.convolve_horizontal(hf);
      valarray<float> vsum = hgrad.vertical_sum();
      valarray<float> absvsum = vsum.apply(&std::fabs);
      for (unsigned int i=0; i<absvsum.size(); i++)
	cout << absvsum[i] << " ";
      cout << endl; 
      cout << "*** y=" << y << " sum=" << absvsum.max() << endl;

    }

//     if (DebugDisplay()) {
//       imagedata tmp = hgrad;
//       tmp.normalize_01();
//       DebugDisplay(tmp);
//     }

    FinishMessage("LocateFace::MatchUpperHalfCircles");

    return 17;
  }

  /////////////////////////////////////////////////////////////////////////////

  float LocateFace::max_value_index(const valarray<float>&a, int& j) {
    float max = a[0];
    j = 0;
    for (unsigned int i=1; i<a.size(); i++)
      if (a[i]>max)
	max = a[j=i];
    return max;
  }

  /////////////////////////////////////////////////////////////////////////////

  void LocateFace::SweepImage(int rows, int x0, int y0) {
    const int maxside = Width() > Height() ? Width():Height();

    const imagedata& prevseg = *getImg()->accessSegmentation();
    if (Verbose()>2)
      cout << "LocateFace::SweepImage : " << prevseg.info() << endl;

    sweepimg = gradimg.to_polar(maxside, rows, x0, y0);
    maskimg  = prevseg.to_polar(maxside, rows, x0, y0);

    for (unsigned int a=0; a<maskimg.width(); a++)
      for (unsigned int b=0; b<maskimg.height(); b++) {
	long m = maskimg.get_one_uint32(a, b);
	if (Verbose()>3)
	  cout << "LocateFace::SweepImage maskimg(" << a << "," << b << ")="
	       << m << endl;
	if (!m)
	  sweepimg.set(a, b, (float)0.0);
      }

    imagefile::displaysettings stgs;

    if (Verbose()>2) {
      stgs.title = "sweepimage-1";
      imagefile::display(sweepimg, stgs);
    }

    imagedata xxx = sweepimg.from_polar(gradimg.width(), gradimg.height(),
					maxside, rows, x0, y0);

    if (Verbose()>2) {
      stgs.title = "sweepimage-2";
      imagefile::display(xxx, stgs);
    }
  }

  /////////////////////////////////////////////////////////////////////////////


  // Computes a low level Fourier descriptor from given coordinates.
  // To be exact, calculates a FFT, zeroes all
  // elements between level and (size-level+1) and then calculates IFFT.

  void LocateFace::Foud(vector<complex<cmplx_t> >& coords, const int level) {
    int size = coords.size();
    double reaverage = 0;
    double imaverage = 0;

    for (int i = 0; i < size; i++){
      reaverage += coords[i].real();
      imaverage += coords[i].imag();
    }
    complex<cmplx_t> avg(reaverage/size, imaverage/size);

    for (int i = 0; i < size; i++)
      coords[i] -= avg;

    vector<complex<cmplx_t> > fd;
    try {
      fd = cox::fft<complex<cmplx_t>,complex<cmplx_t> >::f(coords);
    }
    catch (const std::exception& e) {
      cerr << "ERROR: " << e.what() << endl;
      return;
    }

    for (int i = level; i < size-level+1; i++)
      fd[i] = 0;

    try {
      coords = cox::fft<complex<cmplx_t>,complex<cmplx_t> >::b(fd);
    }
    catch (const std::exception& e) {
      cerr << "ERROR: " << e.what() << endl;
      return;
    }

    for (int i = 0; i < size; i++)
      coords[i] += avg;
  }

  /////////////////////////////////////////////////////////////////////////////

  void LocateFace::FindMaxOnLines(vector<complex<cmplx_t> >& coordxy, int x0, int y0, double ref_length, double b, double ampl) {
    int sectors = coordxy.size();
    const int maxside = Width() > Height() ? Width():Height();
    double base, amplitude;
    const double angle = M_PI*2/sectors;
    double ta;

    base = b * ref_length; // base is the minimum distance from x0, y0,
    amplitude = ampl * ref_length; // it is reached at M_PI/2 and 3M_PI/2.
    for (int i = 0; i < sectors; i++) {
      float max = 0;
      int max_j = -1;
      ta = (double)i * angle;
      while (ta > M_PI)
	ta -= M_PI;
//      for (int j = round(base + amplitude * abs(ta-M_PI/2)); j < maxside; j++) {
      for (int j = round(base + amplitude * (M_PI/2 - abs(ta-M_PI/2))); j < maxside/2; j++) {
	float v = sweepimg.get_one_float(j, i);
	if (v > max) {
	  max = v;
	  max_j = j;
	}
      }
      
      double x = x0 - (max_j * sin(((double)i*angle-M_PI/2)));
      double y = y0 + (max_j * cos(((double)i*angle-M_PI/2)));

      if (Verbose()>4)
	cout << "FindMaxOnLines() : i=" << i << " x=" << x << " y=" << y
	     << endl;

      coordxy[i] = complex<cmplx_t>(x, y);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  void LocateFace::FindMaxOnLines0(vector<complex<cmplx_t> >& coordxy,
				  const int x0, const int y0,
				  const int maxside, const double b) {
    int sectors = coordxy.size();

    imagedata testimg;
    if (Verbose()>2) {
      testimg = sweepimg;
      testimg.force_three_channel();
    }

    int base = int(b * maxside); // base is the minimum distance from x0, y0,
    for (int i = 0; i < sectors; i++) {
      float max = 0;
      int max_j = -1;
      for (int j = base; j < maxside; j++) {
	float v = sweepimg.get_one_float(j, i);
	if (v > max) {
	  max = v;
	  max_j = j;
	}
	if (Verbose()>2)
	  if (testimg.coordinates_ok(j, i))
	    testimg.set(j, i, imagecolor<float>("green"));
	  else
	    cerr << "bad coordinates A (" << j << "," << i << ")" << endl;
      }
      if (Verbose()>2)
	if (testimg.coordinates_ok(max_j, i))
	  testimg.set(max_j, i, imagecolor<float>("red"));
	else
	  cerr << "bad coordinates B (" << max_j << "," << i << ")" << endl;
      
      const double angle = M_PI*2/sectors;
      double x = x0 - (max_j * sin(((double)i*angle-M_PI/2)));
      double y = y0 + (max_j * cos(((double)i*angle-M_PI/2)));

      if (Verbose()>4)
	cout << "FindMaxOnLines() : i=" << i << " x=" << x << " y=" << y
	     << endl;

      coordxy[i] = complex<cmplx_t>(x, y);
    }

    if (Verbose()>2) {
      imagefile::displaysettings stgs;
      stgs.title = "sweepimage-1-after-fmol";
      imagefile::display(testimg, stgs);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  /* Sometimes the sides have strong edges that CropEdges
     isn't able to remove. This function detects and removes them.
     (hopefully)
  */

  void LocateFace::RemoveEdgeErrors(vector<complex<cmplx_t> >& coords,
				    const int x0) {
    const int sectors = coords.size();
    const int rightbegin = 0;
    const int rightend   = sectors/16; // plus/minus
    const int leftbegin  = 7*sectors/16;
    const int leftend    = 9*sectors/16;

    //for (int i = 0; i<sectors; i+=10)
    //cout << i << " " << coords[i].real() << "," << coords[i].imag() << endl;

    // picsom::imagedata tmp(Width(), Height(), 1);

    double avgleft = 0, avgright = 0;

    for (int i = rightbegin; i < rightend; i++) 
      avgright += coords[i].real()+coords[sectors-1-i].real();
    avgright /= (2*rightend);

    for (int i = leftbegin; i < leftend; i++) 
      avgleft += coords[i].real();
    avgleft /= leftend-leftbegin;

    /*
    cout << "rightbegin=" << rightbegin << " rightend=" << rightend
	 << " avgright=" << avgright << endl
	 << "leftbegin=" << leftbegin << " leftend=" << leftend
	 << " avgleft=" << avgleft << endl;      
    */

    if (avgright - x0 > 1.6*(x0 - avgleft)) // The right side has an error.
      for (int x = x0/4 + int(3*avgright/4); x < Width(); x++)
        for (int y = 0; y < Height(); y++) {
	  SetPixelSegment(x, y, 0); // Masking out the erroneous area.
	  // cout << "A " << x << "," << y << endl;
	  // tmp.set(x, y, 1.0f);
	}
    else if (x0 - avgleft > 1.6*(avgright - x0)) // The left side has an error.
      for (int x = x0/4 + int(3*avgleft/4); x >= 0; x--)
        for (int y = 0; y < Height(); y++) {
	  SetPixelSegment(x, y, 0);
	  // cout << "B " << x << "," << y << endl;
	  // tmp.set(x, y, 1.0f);
	}

    // picsom::imagefile::display(tmp);
  }

  /////////////////////////////////////////////////////////////////////////////

  // This function is inefficient. If more speed is required this should be
  // rewritten with a proper polygon-filler algorithm.
  // (The reason it isn't already done is that I couldn't find one.)

  bool LocateFace::IsInside(coord p, coord *polygon, const int N) {
    int counter = 0;

    coord p1 = polygon[0];
    for (int i=1;i<=N;i++) {
      coord p2 = polygon[i % N];
      if (p.y > myMIN(p1.y,p2.y)) {
        if (p.y <= myMAX(p1.y,p2.y)) {
          if (p.x <= myMAX(p1.x,p2.x)) {
            if (p1.y != p2.y) {
              double xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
              if (p1.x == p2.x || p.x <= xinters)
                counter++;
            }
          }
        }
      }
      p1 = p2;
    }

    return counter%2;
  }

} // namespace picsom

