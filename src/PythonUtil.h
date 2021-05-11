// -*- C++ -*-  $Id: PythonUtil.h,v 2.4 2020/03/20 12:38:03 jormal Exp $
// 
// Copyright 1998-2020 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_PYTHONUTIL_H_
#define _PICSOM_PYTHONUTIL_H_

#include <picsom-config.h>

#if defined(HAVE_PYTHON_H)
#  if defined(PICSOM_USE_PYTHON2) || defined(PICSOM_USE_PYTHON3)
#    define PICSOM_USE_PYTHON 1
#    include <Python.h>
#  else
typedef void PyObject;
#  endif // PICSOM_USE_PYTHON2 | PICSOM_USE_PYTHON3
#else
typedef void PyObject;
#endif // HAVE_PYTHON_H

#include <string>
#include <vector>
#include <list>
#include <map>
using namespace std;

namespace picsom {
  static const string PythonUtil_h_vcid =
    "@(#)$Id: PythonUtil.h,v 2.4 2020/03/20 12:38:03 jormal Exp $";

  /**
     DOCUMENTATION MISSING
  */

  ///
  bool PythonInitialized();

  ///
  bool ConditionallyInitializePython(const string& = "");

  ///
  bool InitializePython();

  ///
  const string& PythonHome();

  ///
  void PythonHome(const string&);

  ///
  const string& PythonVersion(bool = false);

  ///
  map<string,PyObject*> PythonExecute(const string&,
				      const map<string,PyObject*>&,
				      const list<string>&);
  ///
  vector<size_t> TensorShape(PyObject*);
  
} // namespace picsom

#endif // _PICSOM_PYTHONUTIL_H_

