// -*- C++ -*-  $Id: PythonUtil.C,v 2.9 2021/05/11 13:14:20 jormal Exp $
// 
// Copyright 1998-2021 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <PythonUtil.h>
#include <TimeUtil.h>
#include <Util.h>

#include <iostream>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string PythonUtil_C_vcid =
  "@(#)$Id: PythonUtil.C,v 2.9 2021/05/11 13:14:20 jormal Exp $";

  static bool   python_initialized = false;
  static string pythonhome;
  
  /////////////////////////////////////////////////////////////////////////////

  const string& PythonHome() {
    return pythonhome;
  }

  /////////////////////////////////////////////////////////////////////////////

  void PythonHome(const string& s) {
    pythonhome = s;
  }  

  /////////////////////////////////////////////////////////////////////////////

  bool PythonInitialized() {
    return python_initialized;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool ConditionallyInitializePython(const string& h) {
    if (!PythonInitialized() && PythonHome()=="")
      PythonHome(h);
			  
    return PythonInitialized() || InitializePython();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  bool InitializePython() {
#ifdef PICSOM_USE_PYTHON
    string pyv = "2";
#ifdef PICSOM_USE_PYTHON3
    pyv = "3";
#endif // PICSOM_USE_PYTHON3
    
    if (pythonhome!="") {
      if (!DirectoryExists(pythonhome))
	return ShowError("InitializePython() : directory <"+pythonhome
			 +"> inexistent");
      //setenv("PYTHONHOME", pythonhome.c_str(), 1);
      //obs! out-commented in Ububtu 16.04->20.04 upgrade
    }
    Py_Initialize();
    PyEval_InitThreads();
    //PyEval_SaveThread(); // obs!
    python_initialized = true;
    
    cout << TimeStamp() << "Python" << pyv << " initialized"
	 << (pythonhome!=""?" with PYTHONHOME="+pythonhome:"")
	 << endl;
    
    return true;
#else
    return ShowError("PICSOM_USE_PYTHON not defined in InitializePython()");
#endif // PICSOM_USE_PYTHON
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  const string& PythonVersion(bool full) {
#ifdef PICSOM_USE_PYTHON
    static string short_v, long_v;
    if (short_v=="") {
      short_v = long_v = Py_GetVersion();
      size_t p = short_v.find_first_of(" \t\n");
      if (p!=string::npos)
	short_v.erase(p);
    }
    return full ? long_v : short_v;
#else
    static string unknown = "python not used";
    return full ? unknown : unknown;
#endif // PICSOM_USE_PYTHON
  }
    
  /////////////////////////////////////////////////////////////////////////////

  map<string,PyObject*> PythonExecute(const string& cmd,
				      const map<string,PyObject*>& invar,
				      const list<string>& outvar) {
#ifdef PICSOM_USE_PYTHON
    PyObject* main_module = PyImport_AddModule("__main__"); // borrowed
    PyObject* main_dict = PyModule_GetDict(main_module); // borrowed
    for (const auto& i : invar) {
      PyObject *key = PyUnicode_FromString(i.first.c_str());
      if (PyObject_SetItem(main_dict, key, i.second)) {
	ShowError("PyObject_SetItem() failed");
	return {};  // obs!
      }
      Py_DECREF(key);
    }

    PyRun_SimpleString(cmd.c_str());

    map<string,PyObject*> r;
    
    for (const auto& i : outvar) {
      PyObject *key = PyUnicode_FromString(i.c_str());
      PyObject *o = PyObject_GetItem(main_dict, key);
      if (!o) {
	ShowError("PyObject_GetItem() failed");
	return {};  // obs!
      }
      Py_DECREF(key);
      r[i] = o;
    }

    return r;
    
#else
    if (cmd==""&&invar.empty()&&outvar.empty())
      return {};
    return {};
#endif // PICSOM_USE_PYTHON
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  vector<size_t> TensorShape(PyObject *t) {
#ifdef PICSOM_USE_PYTHON
    if (string(t->ob_type->tp_name)!="Tensor")
      return {};

    vector<size_t> s;
    vector<PyObject*> r;
    PyObject *x = t;
    for (;;) {
      Py_ssize_t l = (*x->ob_type->tp_as_sequence->sq_length)(x);
      if (l<0) {
	PyErr_Clear();
        break;
      }
      s.push_back(l);
      PyObject *i = PyObject_GetIter(x);
      r.insert(r.begin(), i);
      x = PyIter_Next(i);
      r.insert(r.begin(), x);
    }

    for (auto i : r)
      Py_DECREF(i);

    return s;

#else
    if (t)
      return {};
    return {};
#endif // PICSOM_USE_PYTHON
  }
    
  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

