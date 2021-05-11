// $Id: TorchVision.C,v 1.8 2020/03/20 12:56:41 jormal Exp $	

#include <iterator>

#include <TorchVision.h>
#include <PythonUtil.h>

#include <fcntl.h>

namespace picsom {
  static const char *vcid =
    "$Id: TorchVision.C,v 1.8 2020/03/20 12:56:41 jormal Exp $";

  static TorchVision list_entry(true);

  map<string,PyObject*> TorchVision::models;

  //===========================================================================

  string TorchVision::Version() const {
    return vcid;
  }

  //===========================================================================

  bool TorchVision::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "TorchVision::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
        cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="model") {
	model = value;
      	i = l.erase(i);
      	continue;
      }

      if (key=="layer") {
	layer = value;
      	i = l.erase(i);
      	continue;
      }

      i++;
    }

    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  Feature::featureVector TorchVision::getRandomFeatureVector() const {
    featureVector ret;
    return ret;
  }

  //===========================================================================

  int TorchVision::printMethodOptions(ostream&) const {
    return 0;
  }

  //===========================================================================

  bool TorchVision::ProcessRegion(const Region&, const vector<float>&,
			    Data*) {
    return false;
  }

  //===========================================================================

  PyObject *TorchVision::GetModel(const string& mname, const string& lnum) {
    string modelname = mname+"_"+lnum;
    string msg = "TorchVision::GetModel("+modelname+") : ";
#ifdef PICSOM_USE_PYTHON
    bool debug = false;

    if (debug)
      cout << msg << "finding" << endl;
    if (models.find(modelname)!=models.end()) {
      if (debug)
	cout << msg << "found" << endl;
      return models[modelname];
    }
    
    if (debug)
      cout << msg << "creating" << endl;
    PyObject *pModule = PyImport_ImportModule("torchvision.models");
    if (!pModule) {
      PyErr_Print();
      ShowError(msg+"PyImport_ImportModule() failed for torchvision.models");
      return NULL;  // obs!
    }
    PyObject *pDict   = PyModule_GetDict(pModule); // borrowed
    PyObject *pClass  = PyDict_GetItemString(pDict, mname.c_str()); // borrowed
    if (!PyCallable_Check(pClass)) {
      PyErr_Print();
      ShowError(msg+"PyDict_GetItemString() failed");
      return NULL; // obs!
    }
    PyObject *pArgs = PyDict_New();
    PyObject *key = PyUnicode_FromString("pretrained");
    PyObject_SetItem(pArgs, key, Py_True);  // steals
    key = PyUnicode_FromString("progress");
    PyObject_SetItem(pArgs, key, Py_False); // steals

    PyObject *etuple = PyTuple_New(0);
    PyObject *modelp = PyObject_Call(pClass, etuple, pArgs);
    Py_DECREF(etuple);
    Py_DECREF(pArgs);
    if (!PyCallable_Check(modelp)) {
      PyErr_Print();
      ShowError(msg+"PyObject_Call() failed");
      return NULL; // obs!
    }

    // PythonExecute("import torch; print(aax)", { {"aax", modelp} }, {});

    string cmd = "import torch; ";
    cmd += modelname+" = torch.nn.Sequential(*list("+mname+
      ".children())[:-"+lnum+"])";
    // cout << "[" << cmd << "]" << endl;

    auto out = PythonExecute(cmd, { {mname, modelp} }, { modelname }); 
    Py_DECREF(modelp);
    Py_DECREF(pModule);

    PyObject *modelx = out[modelname];
    // PythonExecute("import torch; print(aaa)", { {"aaa", modelx} }, {});
    
    PyObject *eval = PyUnicode_FromString("eval");
    PyObject *t = PyObject_CallMethodObjArgs(modelx, eval, NULL);
    if (!t) {
      PyErr_Print();
      ShowError(msg+"PyObject_CallMethodObjArgs() failed for eval()");
      return NULL; // obs!
    }

    if (debug)
      cout << msg << "created" << endl;
    models[modelname] = modelx;
    if (debug)
      cout << msg << "cached" << endl;
    
    return modelx;
    
#else
    ShowError(msg+"PICSOM_USE_PYTHON not defined");
    return NULL;
#endif // PICSOM_USE_PYTHON
  }
  
  //===========================================================================

  PyObject *TorchVision::TorchTensor(const list<imagedata>& l) {
    string msg = "TorchVision::TorchTensor() : ";

#ifdef PICSOM_USE_PYTHON
    size_t w = 0, h = 0;
    for (const auto& i : l) {
      if (i.count()!=3) {
	 ShowError(msg+"images should have 3 channels");
	 return NULL;
      }
      if (i.type()!=imagedata::pixeldata_float) {
	 ShowError(msg+"images should be float type");
	 return NULL;
      }
      if ((w!=0 || h!=0) && (w!=i.width() || h!=i.height())) {
	 ShowError(msg+"all images should have the same size");
	 return NULL;
      }
      w = i.width();
      h = i.height();
    }
    if (w==0 || h==0) {
      ShowError(msg+"image size not known");
      return NULL;
    }

    auto img = l.begin();
    PyObject *a = PyList_New(l.size());
    for (size_t i=0; i<l.size(); i++) {
      const float *v = img->raw_float(3*w*h);
      PyObject *b = PyList_New(3);
      for (size_t j=0; j<3; j++) {
	PyObject *c = PyList_New(h);
	for (size_t y=0; y<h; y++) {
	  PyObject *d = PyList_New(w);
	  for (size_t x=0; x<w; x++) {
	    int k = img->to_index(x, y, j);
	    PyList_SetItem(d, x, PyFloat_FromDouble(v[k]));
	  }
	  PyList_SetItem(c, y, d);
	}
	PyList_SetItem(b, j, c);
      }
      PyList_SetItem(a, i, b);
      img++;
    }
    
    PyObject *pModule = PyImport_ImportModule("torch");
    if (!pModule) {
      PyErr_Print();
      ShowError(msg+"PyImport_ImportModule() failed for torch");
      return NULL;  // obs!
    }
    PyObject *pDict   = PyModule_GetDict(pModule); // borrowed
    Py_DECREF(pModule);
    PyObject *pClass  = PyDict_GetItemString(pDict, "tensor"); // borrowed
    if (!PyCallable_Check(pClass)) {
      PyErr_Print();
      return NULL; // obs!
    }
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, a); // steals

    PyObject *t = PyObject_CallObject(pClass, pArgs);
    PyErr_Print();
    Py_DECREF(pArgs);

    return t;
    
#else
    ShowError(msg+ToStr(l.size())+"images but PICSOM_USE_PYTHON not defined");
    return NULL;
#endif // PICSOM_USE_PYTHON
  }
  
  //===========================================================================

  bool TorchVision::ProcessBatch() {
    string msg = "TorchVision::ProcessBatch() : ";

#ifdef PICSOM_USE_PYTHON
    if (FileVerbose())
      cout << TimeStamp() << msg << batch.size() << " entries to process"
	   << endl;

    // model = "resnext50_32x4d";
    // layer = "1";

    if (model=="" || layer=="")
      return ShowError(msg+"both model and layer should be set");
    
    if (!ConditionallyInitializePython("./venv"))
      return ShowError(msg+"python initialization failed");

    if (MethodVerbose()) {
      string pythonv = Py_GetVersion();
      cout << TimeStamp() << msg << "python version = " << PythonVersion()
	   << endl;
    }
    
    vector<float> negmean {  -0.485,  -0.456,  -0.406 };
    vector<float> invstd  { 1/0.229, 1/0.224, 1/0.225 };
    list<imagedata> imglist;

    for (const auto& i : batch) { // pre loop
      if (FileVerbose()) {
	cout << TimeStamp() << msg << "  " << i.fname;
	if (i.incore)
	  cout << " incore=[" << i.incore->type << "," 
	       <<  i.incore->ident
	       << ",len=" << i.incore->vec.size() << "]";
	if (i.result)
	  cout << " result.size()=" << i.result->data.size();
	cout << endl;
      }
      string fname = i.fname;
     
      imagedata img;
      if (fname=="" && i.incore && i.incore->type=="string::filename*") {
	void *ptr = NULL;
	sscanf(i.incore->ident.c_str(), "%p", &ptr);
	fname = *(string*)ptr;
      }
      if (fname!="")
	img = imagefile(fname).frame(0);
      else if (i.incore && i.incore->type=="picsom::imagedata*") {
	imagedata *incore_imagedata_ptr = NULL;
	sscanf(i.incore->ident.c_str(), "%p", &incore_imagedata_ptr);
	img = *incore_imagedata_ptr;
      }
      
      size_t w = 224, h = 224;
      scalinginfo sc(img.width(), img.height(), w, h);
      sc.stretch(true);
      img.rescale(sc, 1);
      if (FrameVerbose())
	cout << TimeStamp() << msg << "  B " << img.info() << endl;

      img.add_multiply(negmean, invstd);
      imglist.push_back(img);
    } // pre loop
    
    if (FrameVerbose())
      cout << TimeStamp() << msg << "getting GIL" << endl;
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (FrameVerbose())
      cout << TimeStamp() << msg << "creating tensor" << endl;

    PyObject *tensor = TorchTensor(imglist);
    if (!tensor) {
       ShowError(msg+"TorchTensor() failed");
      return NULL; // obs!
    }
    // cout << "shape = "+ToStr(TensorShape(tensor)) << endl;

    if (FrameVerbose())
      cout << TimeStamp() << msg << "getting model" << endl;

    PyObject *modelp = GetModel(model, layer);
    if (!modelp) {
      ShowError(msg+"GetModel("+model+","+layer+") failed");
      return NULL; // obs!
    }
    // PythonExecute("import torch; print(aaz)", { {"aaz", modelp} }, {});
    
    PyObject *pModulex = PyImport_ImportModule("torch");
    if (!pModulex) {
      PyErr_Print();
      ShowError(msg+"PyImport_ImportModule() failed for torch");
      return NULL;  // obs!
    }
    PyObject *pDictx   = PyModule_GetDict(pModulex); // borrowed

    PyObject *pClass  = PyDict_GetItemString(pDictx, "no_grad"); // borrowed
    if (!PyCallable_Check(pClass)) {
      PyErr_Print();
      ShowError(msg+"PyDict_GetItemString() failed for no_grad");
      return NULL; // obs!
    }
    PyObject *pArgs = PyTuple_New(0);

    PyObject *t = PyObject_CallObject(pClass, pArgs);
    Py_DECREF(pArgs);
    Py_DECREF(pModulex);
    if (!t) {
      PyErr_Print();
      ShowError(msg+"PyObject_CallObject() failed for no_grad()");
      return NULL; // obs!
    }
    
    PyObject *xtuple = PyTuple_New(1);
    PyTuple_SetItem(xtuple, 0, tensor); // steals

    if (FrameVerbose())
      cout << TimeStamp() << msg << "feed forwarding" << endl;

    PyObject *r = PyObject_CallObject(modelp, xtuple);   // THIS IS THE CALL
    if (!r) {
      PyErr_Print();
      ShowError(msg+"PyObject_CallObject() failed");
      return NULL;  // obs!
    }
    Py_DECREF(xtuple);

    if (string(r->ob_type->tp_name)!="Tensor") {
      PyErr_Print();
      ShowError(msg+"PyObject_CallObject() failed to return a tensor");
      return NULL;  // obs!
    }
    // cout << "shape = "+ToStr(TensorShape(r)) << endl;
    
    if (!r->ob_type->tp_as_sequence || !r->ob_type->tp_as_sequence->sq_length) {
      PyErr_Print();
      ShowError(msg+"PyObject_CallObject() returned a non-sequence tensor");
      return NULL;  // obs!
    }

    Py_ssize_t rrrr = (*r->ob_type->tp_as_sequence->sq_length)(r);
    
    if (size_t(rrrr) != batch.size()) {
      PyErr_Print();
      ShowError(msg+"PyObject_CallObject() returned "+ToStr(rrrr)+" when "+
		ToStr(batch.size())+" were expected");
      return NULL;  // obs!
    }

    PyObject *iter = PyObject_GetIter(r);
    if (!iter) {
      PyErr_Print();
      ShowError(msg+"PyObject_CallObject() returned non-iterable tensor");
      return NULL;  // obs!
    }
    
    if (FrameVerbose())
      cout << TimeStamp() << msg << "reading result" << endl;

    for (auto& i : batch) {  // post loop
      if (FileVerbose())
	cout << TimeStamp() << msg << "B "<< i.fname << endl;
      
      PyObject *xxx = PyIter_Next(iter);
      if (!xxx) {
	PyErr_Print();
	ShowError(msg+"PyIter_Next() returned NULL");
	return NULL;  // obs!
      }

      if (!xxx->ob_type->tp_as_sequence ||
	  !xxx->ob_type->tp_as_sequence->sq_length) {
	PyErr_Print();
	ShowError(msg+"PyObject_CallObject() returned a non-sequence tensor");
	return NULL;  // obs!
      }

      Py_ssize_t xxxl = (*xxx->ob_type->tp_as_sequence->sq_length)(xxx);
      vector<float> vec(xxxl);
      
      PyObject *ix = PyObject_GetIter(xxx);
      if (!ix) {
	PyErr_Print();
	ShowError(msg+
		  "PyObject_CallObject() returned non-iterable tensor 2nd D");
	return NULL;  // obs!
      }
      for (size_t jj=0; jj<size_t(xxxl); jj++) {
	PyObject *yyy = PyIter_Next(ix);
	if (!yyy) {
	  PyErr_Print();
	  ShowError(msg+"PyIter_Next() returned NULL 2nd D");
	  return NULL;  // obs!
	}
	if (!yyy->ob_type->tp_as_number->nb_float) {
	  PyErr_Print();
	  ShowError(msg+"PyIter_Next() returned non-float 2nd D");
	  return NULL;  // obs!
	}
	double fff = PyFloat_AsDouble(yyy);
	vec[jj] = float(fff);

	Py_DECREF(yyy);
      }
      
      Py_DECREF(ix);
      Py_DECREF(xxx);

      string label = i.fname;
      size_t p = label.rfind('/');
      if (p!=string::npos)
	label.erase(0, p+1);
      p = label.rfind('.');
      if (p!=string::npos)
	label.erase(p);

      if (i.result) {
	labeled_float_vector lv { vec, label };
	i.result->append(lv);
      } else {
	if (DataCount()==0) {
    	  if (false && !fout_xx)
    	    fout_xx = &cout;
	  
    	  AddOneDataLabel("");
    	  SetVectorLengthFake(vec.size());
    	  AddDataElements(false);
    	  ((VectorData*)GetData(0))->setLength(vec.size());
    	}
	if (fout_xx)
	  Print(*fout_xx, vec, label);
      }
    }

    Py_DECREF(iter);
    Py_DECREF(r);

    if (FrameVerbose())
      cout << TimeStamp() << msg << "releasing GIL" << endl;
    PyGILState_Release(gstate);
    
    if (FrameVerbose())
      cout << TimeStamp() << msg << "done all" << endl;

    batch.clear();
    
    return true;
    
#else
    ShowError(msg+"PICSOM_USE_PYTHON not defined");
    return false;
#endif // PICSOM_USE_PYTHON
  }

} // namespace picsom

//=============================================================================

// Local Variables:
// End:
