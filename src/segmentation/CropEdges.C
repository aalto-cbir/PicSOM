#include <CropEdges.h>
#include <stdlib.h>

static const string vcid="$Id: CropEdges.C,v 1.6 2009/04/30 08:06:02 vvi Exp $";

namespace picsom {
  static CropEdges list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  const char *CropEdges::Version() const { 
    return vcid.c_str();
  }

  /////////////////////////////////////////////////////////////////////////////

  void CropEdges::UsageInfo(ostream& os) const { 
    os << "CropEdges :" << endl
       << "  options : " << endl
       << "    NONE yet." << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  int CropEdges::ProcessOptions(int /*argc*/, char** /*argv*/) { 
    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool CropEdges::Process() {
    if (Verbose()>1)
      ShowLinks();

    // Setting up operational values.
    int top = -1, left = -1;
    int bottom = Height(), right = Width();
    int  di =(int)(0.01 * (Width()<Height()?Width():Height())); // window size
    int dgx = 30 * Width();   // A change of 30 is considered an edge.
    int dgy = 30 * Height();

    vector<long> sumx(Width()), sumy(Height());

    for (int i = 0; i < Width(); i++) {
      long acc = 0;
      for (int j = 0; j < Height(); j++) {
	float r, g, b;
	GetPixelRGB(i, j, r, g, b);
	acc += (int)(256*(r + g + b) / 3);
      }
      sumx[i] = acc;
    }

    for (int i = 0; i < Height(); i++) {
      long acc = 0;
      for (int j = 0; j < Width(); j++) {
	float r, g, b;
	GetPixelRGB(j, i, r, g, b);
	acc += (int)(256*(r + g + b) / 3);
      }
      sumy[i] = acc;
    }

    if (Verbose()>2) {
      for (int i = 0; i < Width(); i++)
	cout << "CropEdges: projection on x=" << i << ": " << sumx[i] << endl;
      for (int i = 0; i < Height(); i++)
	cout << "CropEdges: projection on y=" << i << ": " << sumy[i] << endl;
    }

    for (int i = di; i < (int) (Height() - 1)/7; i++)
      if (abs(sumy[i] - sumy[i - di]) >= dgx)
	top = i;

    for (int i = Height() - 1 - di; i >= (int) (Height() - 1)*5/6; i--)
      if (abs(sumy[i] - sumy[i + di]) >= dgx)
	bottom = i;

    for (int i = di; i < (int) (Width() - 1)/3; i++)
      if (abs(sumx[i] - sumx[i - di]) >= dgy)
	left = i;

    for (int i = Width() - 1 - di; i >= (int) (Width() - 1)*2/3; i--)
      if (abs(sumx[i] - sumx[i + di]) >= dgy)
	right = i;

    if (Verbose()>1)
      cout << "CropEdges: top=" << top << " bottom=" << bottom
	   << " left=" << left << " right=" << right << endl;

    if (Verbose()>2)
      cout << "CropEdges : " << getImg()->accessSegmentation()->info() << endl;

    // Now let's mark the area to be cropped.
    for (int i = 0; i < Width(); i++)
      for (int j = 0; j < Height(); j++) {
	bool v = j>top && j<bottom && i>left && i<right;
	SetPixelSegment(i, j, v);
	if (Verbose()>3)
	  cout << "CropEdges: (" << i << "," << j << ")=" << v << endl;
      }

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////
}

