// -*- C++ -*- 	$Id: TorchVision.h,v 1.5 2020/03/20 12:56:41 jormal Exp $
/**
   \file TorchVision.h

   \brief Declarations and definitions of class TorchVision
   
   TorchVision.h contains declarations and definitions of the class
   TorchVision, which is a class that calculates the mel-cepstrum feature
   using an external command

   \author Mats Sjoberg <mats.sjoberg@hut.fi>
   $Revision: 1.5 $
   $Date: 2020/03/20 12:56:41 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _TorchVision_
#define _TorchVision_

#include "RegionBased.h"
#include <regex.h>

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

namespace picsom {
  /// A class that ...
  class TorchVision : public RegionBased {
  public:
    /// definition for the current data type
    typedef float datatype;  // this should be selectable in run time...
  
    /// definition for the current data vector type
    typedef vector<datatype> vectortype;

    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    TorchVision(const string& img = "", const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt);
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    TorchVision(const string& img, const string& seg,
		 const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt); 
    }

    /// This constructor doesn't add an entry in the methods list.
    /// !! maybe it shouldn't but it does !!
    TorchVision(bool) : RegionBased(false) {}

    virtual Feature *Create(const string& s) const {
      return (new TorchVision())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
      return new VectorData(t, n, this);
    }

    virtual void deleteData(void *p) {
      delete (VectorData*)p;
    }

    /** Overrided to provide support for videos, which are in fact
     *  images, but they cannot be read in with our current library functions.
     *  \return true if the input files are readable as images, false otherwise
     */
    virtual bool ImageReadable() const {
      return false;
    }

    virtual bool IsBatchOperator() { return true; }

    virtual bool ProcessBatch();

    // virtual vector<string> RemoteExecution() const {
    //   return RemoteExecutionLinux64();
    // }

    virtual string Name() const {
      return "torchvision";
    }

    virtual string LongName() const { 
      return "torchvision";
    }

    virtual string ShortText() const {
      return "torchvision inside pytorch.";
    }

    virtual const string& DefaultZoningText() const {
      static string flat("1");
      return flat; 
    }

    virtual vector<string> UsedDataLabels() const {
      vector<string> ret;
      ret.push_back("1");
      return ret;

    }

    virtual string TargetType() const { return "image"; }

    virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

    virtual string Version() const;

    virtual bool ProcessOptionsAndRemove(list<string>&); 

    virtual featureVector getRandomFeatureVector() const;
  
    virtual int printMethodOptions(ostream&) const;

    virtual bool ProcessRegion(const Region&, const vector<float>&, Data*);

    // virtual bool SaveOutput() const { return true; }

    ///
    PyObject *GetModel(const string&, const string&);
    
    ///
    PyObject *TorchTensor(const list<imagedata>&);
    
  protected:  
    ///
    string model;

    ///
    string layer;

    ///
    static map<string,PyObject*> models;
  };
}
#endif

// Local Variables:
// mode: font-lock
// End:
