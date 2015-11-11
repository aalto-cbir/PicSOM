// $Id: Raw.C,v 1.9 2013/03/21 08:36:42 jorma Exp $	

#include <Raw.h>

namespace picsom {
  static const char *vcid =
    "$Id: Raw.C,v 1.9 2013/03/21 08:36:42 jorma Exp $";

  static Raw list_entry(true);

  //===========================================================================

  string Raw::Version() const {
    return vcid;
  }

  //===========================================================================

  bool Raw::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "Raw::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="convert") {
	if (value=="255" || value=="z")
	  conversion = value;
	else
	  throw msg + " : convert conversion not understood";
      
	i = l.erase(i);
	continue;
      }

      i++;
    }

    return RegionBased::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  bool Raw::ProcessRegion(const Region& /*ri*/, const vector<float>& v,
			  Data *dst) {
    vector<float>& d = ((RawData*)dst)->datavec;

    if (FrameVerbose())
      cout << "Raw::ProcessRegion() : d.size()=" << d.size() << endl;

    if (d.size()!=v.size()) {
      char tmp[100];
      sprintf(tmp, "d.size()=%d v.size()=%d", (int)d.size(), (int)v.size());
      throw string("Raw::ProcessRegion() data size mismatch ")+tmp;
    }

    d = v;

    if (conversion=="255")
      for (size_t i=0; i<d.size(); i++)
	d[i] *= 255;

    else if (conversion=="z") {
      double s = 0, ss = 0;
      int l = d.size();
      for (int i=0; i<l; i++) {
	s  += d[i];
	ss += d[i]*d[i];
      }
      double mean = s/l;
      double sd   = sqrt((ss-s*s/l)/(l-1));
      for (int i=0; i<l; i++)
	d[i] = (d[i]-mean)/sd;
    }

    return true;
  }

  //===========================================================================

} // namespace picsom

