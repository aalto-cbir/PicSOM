// $Id: SLMHeadPose.C,v 1.2 2013/03/22 14:50:44 jorma Exp $	

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <SLMHeadPose.h>
#include <ImageSequenceSource.hpp>

using namespace slmotion;

namespace picsom {
  static const char *vcid =
    "$Id: SLMHeadPose.C,v 1.2 2013/03/22 14:50:44 jorma Exp $";

  static SLMHeadPose list_entry(true);

  //===========================================================================

  string SLMHeadPose::Version() const {
    return vcid;
  }

  //===========================================================================

  bool SLMHeadPose::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "SLMHeadPose::ProcessOptionsAndRemove() ";

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

  bool SLMHeadPose::ProcessRegion(const Region& /*ri*/, const vector<float>& v,
				  Data *dst) {


    if (FrameVerbose())
      cout << "SLMHeadPose::ProcessRegion()" << endl;

    FrameSource *fsp = new ImageSequenceSource(vector<string>());

    BlackBoard bb;
    HeadPose hp(&bb, fsp);
    hp.processRange(0, 1);
    // foo = bb.get<type>(framenumber, string);
    cv::Vec3d vvv = bb.get<cv::Vec3d>(0, "headpose");

    vector<float>& d = ((SLMHeadPoseData*)dst)->datavec;
    if (d.size()!=v.size()) {
      char tmp[100];
      sprintf(tmp, "d.size()=%d v.size()=%d", (int)d.size(), (int)v.size());
      throw string("SLMHeadPose::ProcessRegion() data size mismatch ")+tmp;
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
}

//=============================================================================

// Local Variables:
// mode: font-lock
// End:
