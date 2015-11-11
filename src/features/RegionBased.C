// $Id: RegionBased.C,v 1.24 2015/01/20 12:49:49 jorma Exp $	

#include <RegionBased.h>

namespace picsom {
//static const char *vcid = "$Id: RegionBased.C,v 1.24 2015/01/20 12:49:49 jorma Exp $";

//=============================================================================

bool RegionBased::ProcessOptionsAndRemove(list<string>& opts) {
  const string msg = "RegionBased::ProcessOptionsAndRemove()";

  if (FileVerbose())
    cout << msg << endl;
  
  for (list<string>::iterator i = opts.begin(); i!=opts.end();) {
    if (FileVerbose())
      cout << "  <" << *i << ">" << endl;
    
    string key, value;
    SplitOptionString(*i, key, value);
        
    if (key=="dump") {
      if (!IsBoolean(value))
	throw msg + " : option dump should have boolean value";
	  
      do_dump = IsAffirmative(value);
      i = opts.erase(i);
      continue;
    }

    if (key=="storeregion") {
      storeregion = value;
      i = opts.erase(i);
      continue;
    }

    i++;
  }
  
  bool ok = Feature::ProcessOptionsAndRemove(opts);

  if (ok && !RegionDescriptorCount()) {
    char tmp[100];
    int w = Width(), h = Height();
    sprintf(tmp, "(%d,%d):%dx%d", (w-1)/2, (h-1)/2, w, h);
    AddRegionDescriptor(tmp, "");
    SetRegionSpecifications();

    if (FileVerbose())
      cout << msg << " added all image [" << tmp << "]" << endl;
  }
  
  return ok;
}

//=============================================================================

bool RegionBased::CalculateOneFrame(int f) {
  char msg[100];
  sprintf(msg, "RegionBased::CalculateOneFrame(%d)", f);

  if (FrameVerbose())
    cout << msg << " : starting" << endl;
  
  bool ok = true;

  int n = DataElementCount();
  for (int i=0; ok && i<n; i++)
    if (!CalculateOneLabel(f, i))
      ok = false;

  if (FrameVerbose())
    cout << msg << " : ending" << endl;

  return ok;
}

//=============================================================================

bool RegionBased::CalculateOneLabel(int f, int l) {
  char lstr[100];
  sprintf(lstr, "%d,%d", f, l);
  string msg = string("RegionBased::CalculateOneLabel(")+lstr+")";

  if (l>=NumberOfRegions())
    throw string("RegionBased::CalculateOneLabel() region does not exist");

  auto rrr = accessRegion(l);
  if (rrr.first->isA("rectangular")==false)
    throw string("class RegionBased currently supports only rectangular")
      + " regions";

  const rectangularRegion *rr=(const rectangularRegion*)rrr.first;
  rectangularRegion ri = *rr;

  msg += "  ["+(string)ri+"] == "+ri.dumpstr();

  if (LabelVerbose())
    cout << msg << endl;

  if (HasScaling()) {
    ri.TranslateByScaling(GetScalinginfo());
    if (LabelVerbose())
      cout << "  translated to " << ri.dumpstr() << endl;
  }

  pixeltype pt = PixelType();
  vector<float> regiondata;

  // obs! x0,y0 initializations added 2015-01-20
  int s = ri.size(), x0 = 0, y0 = 0, x1, y1;

  imagedata *extract = NULL;
  if (storeregion!="") {
    ri.xy(0,   x0, y0);
    ri.xy(s-1, x1, y1);
    extract = new imagedata(x1-x0+1, y1-y0+1,
			    CurrentFrame().count(),  CurrentFrame().type());
  }

  for (int i=0; i<s; i++) {
    int xs = -1, ys = -1; // obs! were uninitialized until 2013-08-20
    ri.xy(i, xs, ys);
    int x = min(Width()-1,  max(xs, 0));
    int y = min(Height()-1, max(ys, 0));
    vector<float> v = GetPixelFloats(x, y, pt);
    
    regiondata.insert(regiondata.end(), v.begin(), v.end());

    if (extract)
      extract->set(xs-x0, ys-y0, CurrentFrame().get_float(x, y));
  }

  if (extract)
    imagefile::write(*extract, storeregion);
  delete extract;

  Data *data_dst = GetData(l);
 
  if ((int)regiondata.size()!=s*data_dst->DataUnitSize()) {
    char tmp[100];
    sprintf(tmp, "%d != %d = %d x %d", (int)regiondata.size(),
	    s*data_dst->DataUnitSize(), s, data_dst->DataUnitSize());
    throw msg+ " regiondata size mismatch: "+tmp;
  }

  if (do_dump) {
    char tmp[100] = "";
    if (DataElementCount()>1)
      sprintf(tmp, "%d", l);
    int ff = (Nframes()>1 && ConcatCountPerPixel()==1) ? f : -1;
    Dump(ri, regiondata, data_dst, Label(-1, ff, tmp)+".dump");
  }

  return ProcessRegion(ri, regiondata, data_dst);
}

//=============================================================================

Feature::featureVector RegionBased::CalculateRegion(const Region &r) {
  pixeltype pt = PixelType();
  vector<float> regiondata;

  coordList l=r.listCoordinates();

  coordList::const_iterator it;

  for (it=l.begin(); it!=l.end(); it++) {
    vector<float> v = GetPixelFloats(it->x, it->y, pt);
    regiondata.insert(regiondata.end(), v.begin(), v.end());
  }

  Data *d=CreateData(PixelType(),ConcatCountPerPixel(),0);
  ProcessRegion(r,regiondata,d);

  bool allow_zero_count = true;
  featureVector ret =d->Result(allow_zero_count);
  deleteData(d);
  return ret; 
}

//=============================================================================

void RegionBased::Dump(const Region& /*ri*/, const vector<float>& /*v*/,
		       Data * /*dst*/, const string& /*fname*/) {
  /*
  SegmentInterface si(NULL, NULL);
  si.SetImageSize(ri.width(), ri.height());
  vector<float>::const_iterator i = v.begin();
  int l = dst->DataUnitSize();
  for (int y=0; y<ri.height(); y++)
    for (int x=0; x<ri.width(); x++, i+=l) {
      vector<float> tmp(i, i+l);
      if (l==1)
	si.SetPixelGrey(x, y, 255*tmp[0]);
      else if (l==2)
	si.SetPixelRGB(x, y, 255*tmp[0], 255*tmp[1], 0);
      else
	si.SetPixelRGB(x, y, 255*tmp[0], 255*tmp[1], 255*tmp[2]);
    }

  Segmentation::imgdata result;
  result.createImage(ri.width(), ri.height(), 3);
  if (si.CopyToImageObject(&result))
    throw "RegionBased::Dump() CopyToImageObject() failed";
  result.writeFile(fname.c_str());

  if (FileVerbose())
    cout << "RegionBased::Dump() dumped to " << fname << " l=" << l << endl;
  */
}
}
//=============================================================================

// Local Variables:
// mode: font-lock
// End:

