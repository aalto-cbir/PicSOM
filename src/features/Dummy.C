// $Id: Dummy.C,v 1.1 2013/08/20 12:19:03 jorma Exp $	

#include <Dummy.h>

namespace picsom {
  static const string vcid =
    "$Id: Dummy.C,v 1.1 2013/08/20 12:19:03 jorma Exp $";

  static Dummy list_entry(true);

  //===========================================================================

  string Dummy::Version() const {
    return vcid;
  }

  //===========================================================================

  bool Dummy::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "Dummy::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="pixel_debug") {
	pixel_debug = atoi(value.c_str());
	i = l.erase(i);
	continue;
      }

      i++;
    }

    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  bool Dummy::CalculateCommon(int f, bool all, int l) {
#ifndef __MINGW32__
    cox::tictac::func tt(tics, "Dummy::CalculateCommon");
#endif // __MINGW32__

    EnsureImage();
    int width = Width(true), height = Height(true);

    if (FrameVerbose())
      cout << "Dummy::CalculateCommon(" << all << "," << l << "), wxh="
	   << width << "x" << height << "=" << width*height << endl;

    picsom::imagedata *idata = NULL;
    if (pixel_debug&2)
      idata = new picsom::imagedata(width, height, 1,
				    picsom::imagedata::pixeldata_uchar);

    for (int y=0; y<height; y++)
      for (int x=0; x<width; x++) {
	if (PixelVerbose())
	  cout << endl;

	vector<int> svec = SegmentVector(f, x, y);
	for (size_t j=0; j<svec.size(); j++) {
	  int s = svec[j];
	  if (s<0) {
	    //	  cerr << "(x,y)=(" <<x<<","<<y<<"), s="<<s<<endl; 
	    throw "segment label<0";
	  }
	
	  if (idata && j==0)
	    idata->set(x, y, (unsigned char)s);

	  int i = DataIndex(s, true);
	  if (pixel_debug&1)
	    cout << x << "," << y << " " << s << " -> " << i << endl;

	  if (i<0)
	    continue;

	  if (all || s==l)
	    CalculateOnePixel(x, y, (DummyData*)GetData(i));
	}
      }

    calculated = true;

    if (idata) {
      if (pixel_debug&4)
	picsom::imagefile::debug_impl(3);

      string fname = Label(-1, -1, "")+"_segments.png";
      picsom::imagefile::write(*idata, fname);

      delete idata;
    }

    return true;
  }

  //===========================================================================

} // namespace picsom

//=============================================================================

// Local Variables:
// mode: font-lock
// End:

