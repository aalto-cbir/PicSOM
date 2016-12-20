// -*- C++ -*-  $Id: MuvisFeat.C,v 1.17 2016/06/23 10:49:26 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <MuvisFeat.h>
#include <fcntl.h>

#include <dlfcn.h>

namespace picsom {
  static const char *vcid =
    "$Id: MuvisFeat.C,v 1.17 2016/06/23 10:49:26 jorma Exp $";

static MuvisFeat list_entry(true);
/*
static MuvisFeat::MuvisFexHandler list_ff(true);
static MuvisFeat::MuvisAFexHandler list_af(true);*/
const MuvisFeat::MuvisFeatHandler *MuvisFeat::MuvisFeatHandler::list_of_methods;

// Prefix to use for feature names
string MuvisFeat::PREFIX="Muvis-";

//=============================================================================

string MuvisFeat::Version() const {
  return vcid;
}

//=============================================================================

Feature::featureVector MuvisFeat::getRandomFeatureVector() const{
  featureVector ret;
  return ret;
}

//=============================================================================

int MuvisFeat::printMethodOptions(ostream&) const {
  return 0;
}

//=============================================================================

bool MuvisFeat::CalculateOneFrame(int f) {
  char msg[100];
  sprintf(msg, "MuvisFeat::CalculateOneFrame(%d)", f);
  
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

bool MuvisFeat::ExecPreProcess(int /*f*/, bool /*all*/, int /*l*/) {
  //  return SegmentPreProcess(l);
  return true;
}

//=============================================================================

External::execchain_t MuvisFeat::GetExecChain(int /*f*/, bool /*all*/, int /*l*/) {
  execchain_t et;
  return et;
}

//=============================================================================

bool MuvisFeat::ExecPostProcess(int /*f*/, bool /*all*/, int /*l*/) {
  return true;
}

  //=============================================================================

  bool MuvisFeat::ProcessOptionsAndRemove(list<string>& l) {
    return External::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  void MuvisFeat::SolveMethods() {
    if (MuvisFeatHandler::list_of_methods)
      return;

    bool debug = false; // MethodVerbose()

    const char *fl[] = {
      "FeXDoCo",
      "FexEHD7",
      "HistHSV",
      "HistRGB",
      "HistYUV",
      "TextureGLCM",
      "AFeXMFCC",
      NULL
    };

    const MuvisFeatHandler **prev = &MuvisFeatHandler::list_of_methods;
    for (const char **l = fl; *l; l++) {
      string fn = *l;
      
      MuvisFeatHandler *h;
      if (fn == "AFeXMFCC") 
        h = new MuvisAFexHandler(this);
      else
        h = new MuvisFexHandler(this);
        
      if (!h->ReadSO(fn)) {
        if (debug)
          cerr << "  muvis_fex_read(" << fn << ") failed" << endl;
	continue;
      }

      if (debug)
	cout << fn << " mapped" << endl;

      if (!h->BindParam()) {
	cerr << "  fex_bind() failed with " << fn << endl;
	continue;
      }

      *prev = h;
      prev = &h->next_of_methods;
    }
  }

  //===========================================================================

  bool MuvisFeat::MuvisFexHandler::ReadSO(const string& f) {
    bool debug = false;

    void *dlh = parent->OpenDL(f);
    if (!dlh) {
      if (debug)
	cout << "dlopen(" << f << ") failed: " << parent->DLError() << endl;
      return false;
    }

    fex_bind     = (FEX_BIND)    (long)dlsym(dlh, "_Z8Fex_BindP8FexParam");
    fex_init     = (FEX_INIT)    (long)dlsym(dlh, "_Z8Fex_InitPd");
    fex_extract  = (FEX_EXTRACT) (long)dlsym(dlh, "_Z11Fex_Extract10FrameParamRi");
    fex_distance = (FEX_DISTANCE)(long)dlsym(dlh, "_Z15Fex_GetDistancePdS_i");
    fex_exit     = (FEX_EXIT)    (long)dlsym(dlh, "_Z8Fex_ExitP8FexParam");

    if (debug) {
      cout << "fex_bind     = " << fex_bind     << endl;
      cout << "fex_init     = " << fex_init     << endl;
      cout << "fex_extract  = " << fex_extract  << endl;
      cout << "fex_distance = " << fex_distance << endl;
      cout << "fex_exit     = " << fex_exit     << endl;
    }

    return fex_bind && fex_init && fex_extract &&
      fex_distance && fex_exit;
  }

  //===========================================================================

  bool MuvisFeat::MuvisAFexHandler::ReadSO(const string& f) {
    bool debug = false;

    void *dlh = parent->OpenDL(f);
    if (!dlh) {
      if (debug)
	cout << "dlopen(" << f << ") failed:" << parent->DLError() << endl;
      return false;
    }

    fex_bind     = (AFEX_BIND)    (long)dlsym(dlh, "_Z9AFex_BindP9AFexParam");
    fex_init     = (AFEX_INIT)    (long)dlsym(dlh, "_Z9AFex_InitPdiii");
    fex_extract  = (AFEX_EXTRACT) (long)dlsym(dlh, "_Z12AFex_Extract11AFrameParamRi");
    fex_distance = (AFEX_DISTANCE)(long)dlsym(dlh, "_Z16AFex_GetDistancePdS_i");
    fex_exit     = (AFEX_EXIT)    (long)dlsym(dlh, "_Z9AFex_ExitP9AFexParam");

    if (debug) {
      cout << "fex_bind     = " << fex_bind     << endl;
      cout << "fex_init     = " << fex_init     << endl;
      cout << "fex_extract  = " << fex_extract  << endl;
      cout << "fex_distance = " << fex_distance << endl;
      cout << "fex_exit     = " << fex_exit     << endl;
    }

    return fex_bind && fex_init && fex_extract &&
      fex_distance && fex_exit;
  }

  
  //===========================================================================

  void MuvisFeat::MuvisFexHandler::
  CopyMethods(const MuvisFeat::MuvisFexHandler& h) {
    fex_bind     = h.fex_bind;
    fex_init     = h.fex_init;
    fex_extract  = h.fex_extract;
    fex_distance = h.fex_distance;
    fex_exit     = h.fex_exit;
  }

  //===========================================================================
  
  void MuvisFeat::MuvisAFexHandler::
  CopyMethods(const MuvisFeat::MuvisAFexHandler& h) {
    fex_bind     = h.fex_bind;
    fex_init     = h.fex_init;
    fex_extract  = h.fex_extract;
    fex_distance = h.fex_distance;
    fex_exit     = h.fex_exit;
  }

  //===========================================================================

  bool MuvisFeat::MuvisFexHandler::BindParam() {
    FexParam *p = new FexParam();
    if (!(fex_bind)(p))
      return false;

    fex_param = p;

    param.clear();
    for (unsigned int i=0; i<p->feat_param_no; i++)
      param.push_back(make_pair(fourcc2string(p->feat_param_fourcc[i]),
				p->feat_param_default[i]));
    if (MethodVerbose())
      ShowParam();
    
    return true;
  }

  //===========================================================================

  bool MuvisFeat::MuvisAFexHandler::BindParam() {
    AFexParam *p = new AFexParam();
    if (!(fex_bind)(p))
      return false;

    fex_param = p;

    param.clear();
    for (unsigned int i=0; i<p->feat_param_no; i++)
      param.push_back(make_pair(fourcc2string(p->feat_param_fourcc[i]),
				p->feat_param_default[i]));
    if (MethodVerbose())
      ShowParam();
    
    return true;
  }
  
  //===========================================================================

  void MuvisFeat::MuvisFexHandler::ShowParam() const {
    FexParam *p = fex_param;
    if (!p) {
      cout << "fex_param==NULL" << endl;
      return;
    }

    cout << "  feat_name = \"" << p->feat_name << "\""
	 << endl;
    cout << "  feat_fourcc = \"" << fourcc2string(p->feat_fourcc) << "\""
	 << endl;
    cout << "  feat_param_no = " << p->feat_param_no << endl;
    for (unsigned int i=0; i<p->feat_param_no; i++)
      cout << "    i=" << i
	   << " feat_param_fourcc[i] = \""
	   << fourcc2string(p->feat_param_fourcc[i]) << "\""
	   << " feat_param_default[i] = " << p->feat_param_default[i]
	   << endl;
    cout << "  ftype = " << p->ftype << endl;
  }

  //===========================================================================

  void MuvisFeat::MuvisAFexHandler::ShowParam() const {
    AFexParam *p = fex_param;
    if (!p) {
      cout << "fex_param==NULL" << endl;
      return;
    }

    cout << "  feat_name = \"" << p->feat_name << "\""
	 << endl;
    cout << "  feat_fourcc = \"" << fourcc2string(p->feat_fourcc) << "\""
	 << endl;
    cout << "  feat_param_no = " << p->feat_param_no << endl;
    for (unsigned int i=0; i<p->feat_param_no; i++)
      cout << "    i=" << i
	   << " feat_param_fourcc[i] = \""
	   << fourcc2string(p->feat_param_fourcc[i]) << "\""
	   << " feat_param_default[i] = " << p->feat_param_default[i]
	   << endl;
  }
  
  //===========================================================================

  double *MuvisFeat::MuvisFeatHandler::ParamVec() const {
    double *v = new double[param.size()];
    for (size_t i=0; i<param.size(); i++)
      v[i] = param[i].second;
    return v;
  }

  //===========================================================================

  MuvisFeat::MuvisFeatHandler
    *MuvisFeat::MuvisFexHandler::Create(const MuvisFeat *p) const {
    MuvisFexHandler *h = new MuvisFexHandler(p);
    h->CopyMethods(*this);
    h->BindParam();
    return h;
  }

  //===========================================================================

  MuvisFeat::MuvisFeatHandler
    *MuvisFeat::MuvisAFexHandler::Create(const MuvisFeat *p) const {
    MuvisAFexHandler *h = new MuvisAFexHandler(p);
    h->CopyMethods(*this);
    h->BindParam();
    return h;
  }

  //===========================================================================

  bool MuvisFeat::CalculateOneLabel(int /*f*/, int /*l*/) {
    if (!handler)
      return false;

    string hdr = "MuvisFeat::CalculateOneLabel() : ";

    int p = handler->API_Init();
    
    if (MethodVerbose())
      cout << hdr << "p=" << p << endl;

    if (!p) {
      cerr << hdr << "API_Init() failed" << endl;
      return false;
    }

    int len = 0;
    double *vec = handler->API_Calculate(len);
    
    if (MethodVerbose())
      cout << hdr << "len=" << len << endl;

    if (len<1) {
      cerr << hdr << "API_Calculate() failed" << endl;
      return false;
    }

    SetVectorLength(len);

    if (DataCount()!=1) {
      cerr << hdr << "DataCount()!=1:" << DataCount() << endl;
      return false;
    }

    MuvisFeatData *d = (MuvisFeatData*)GetData(0);
    d->ZeroToSize(len);
    d->SetVector(vector<float>(vec, vec+len));

    int r = handler->API_Exit();
    if (MethodVerbose())
      cout << hdr << "r=" << r << endl;

    if (!r) {
      cerr << hdr << "API_exit() failed" << endl;
      return false;
    }

    return true;
  }

  //===========================================================================
  
  double* MuvisFeat::MuvisFexHandler::API_Calculate(int& len) {
    picsom::imagedata img = parent->CurrentFrame();
    vector<unsigned char> buf;

    FrameType ft = fex_param->ftype;
    if (ft == 0) {
      buf = img.create_yuv420();
    } else if (ft == 1) {
      img.convert(imagedata::pixeldata_uchar);
      img.force_three_channel();
      buf = img.get_uchar();
    }

    FrameParam fp;
    fp.buffer = &buf[0];
    fp.Xs     = img.width();
    fp.Ys     = img.height();

    return (fex_extract)(fp, len);
  }
  
  //===========================================================================

  int MuvisFeat::MuvisAFexHandler::API_Init() {
    soundfile* afile = parent->audiofile_ptr;
    double *args = ParamVec();
    int f_dur = 100; // frame in milliseconds, fixme: should be cmdline parameter
    int p = (*fex_init)(args, afile->getSampleRate(), afile->getNChannels(), f_dur);
    delete args;
    return p;
  }

  //===========================================================================
  
  double* MuvisFeat::MuvisAFexHandler::API_Calculate(int& len) {
    soundfile* afile = parent->audiofile_ptr;
    audiodata ad = afile->getNextAudioSlice(afile->getDuration());

    AFrameParam fp;
    fp.buffer = (short*)ad.getData();
    fp.buf_len = ad.getDataLengthInBytes()/sizeof(short);
    
    // _unknown, _voiced, _unvoiced
    fp.a_type = _unknown;
    
    // _uncertain, _env_noise, _music, _silence, _speech, _nonsilent, _notclassified
    fp.f_class = _notclassified; 

    return (fex_extract)(fp, len);
  }
} // namespace picsom

//=============================================================================

// Local Variables:
// mode: font-lock
// End:
