// $Id: Cepstrum.C,v 1.10 2014/02/03 09:21:16 jorma Exp $	

#include <iterator>

#include <Cepstrum.h>
#include <fcntl.h>

namespace picsom {
  static const char *vcid =
    "$Id: Cepstrum.C,v 1.10 2014/02/03 09:21:16 jorma Exp $";

static Cepstrum list_entry(true);

//=============================================================================

string Cepstrum::Version() const {
  return vcid;
}

//=============================================================================

Feature::featureVector Cepstrum::getRandomFeatureVector() const {
  featureVector ret;
  return ret;
}

//=============================================================================

int Cepstrum::printMethodOptions(ostream&) const {
  return 0;
}

//=============================================================================

bool Cepstrum::CalculateOneFrame(int f) {
  char msg[100];
  sprintf(msg, "Cepstrum::CalculateOneFrame(%d)", f);
  
  if (FrameVerbose())
    cout << msg << " : starting" << endl;
  
  bool ok = true;
  
  if (!CalculateOneLabel(f, 0))
    ok = false;
  
  if (FrameVerbose())
    cout << msg << " : ending" << endl;
  
  return ok;
}

//=============================================================================

External::execchain_t Cepstrum::GetExecChain(int /*f*/, bool /*all*/, 
					     int /*l*/) {
  string exec  = "/share/imagedb/picsom/external/linux64/cepstract";
  string filep = ObjectFullDataFileName();

  execargv_t execargv;
  execargv.push_back(exec);
  execargv.push_back("-in");
  execargv.push_back(filep);

  execspec_t execspec(execargv, execenv_t(), "", "");

  return execchain_t(1, execspec);
}

//=============================================================================

bool Cepstrum::ExecPostProcess(int /*f*/, bool /*all*/, int l) {
  string str = GetOutput();
  istringstream ss(str);
  vector<float> res;

  copy(istream_iterator<float>(ss), istream_iterator<float>(), 
       back_inserter(res));
 
  CepstrumData *data_dst = (CepstrumData *)GetData(l);

  data_dst->AddValue(res);

  return true;
}
}
//=============================================================================

// Local Variables:
// mode: font-lock
// End:
