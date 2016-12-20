// -*- C++ -*- 	$Id: MuvisFeat.h,v 1.10 2011/08/25 17:34:21 jorma Exp $
/**
   \file MuvisFeat.h

   \brief Declarations and definitions of class MuvisFeat
   
   MuvisFeat.h contains declarations and definitions of class the
   MuvisFeat, which is a class that ...

   \author Mats Sjöberg <mats.sjoberg@tkk.fi>
   $Revision: 1.10 $
   $Date: 2011/08/25 17:34:21 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _MuvisFeat_
#define _MuvisFeat_

#include "External.h"
#include "audiodata.h"

#include <muvis/Fex_API.h>
#include <muvis/AFeXMFCC/AFex_API.h>

namespace picsom {
/// A class that ...
class MuvisFeat : public External {
public:
  typedef map<string,string> param_t;

  static string PREFIX;

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  MuvisFeat(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt); handler = NULL;
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  MuvisFeat(const string& img, const string& seg,
      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt); handler = NULL;
  }

//   MuvisFeat(Segmentation *s, const list<string>& opt = (list<string>)0) {
//     Initialize("", "", s, opt);
//   }

  /// This constructor doesn't add an entry in the methods list.
  /// !! maybe it shouldn't but it does !!
  MuvisFeat(bool) : External(false) {
    SolveMethods();
  }

  ///
  void SolveMethods();

  // ~MuvisFeat();

  virtual Feature *Create(const string& s) const {
    return (new MuvisFeat())->SetModel(s);
  }

  virtual Feature *SetModel(const string& s) { 
    handler = MuvisFeatHandler::FindMethod(s, this);
    if (!handler) 
      cout << "MuvisFeat::SetModel: Error: " << s << " not implemented." 
	   << endl;
    return this; 
  }

  virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
    return new MuvisFeatData(t, n, this);

  }

  virtual void deleteData(void *p){
    delete (MuvisFeatData*)p;
  }

  /** Overrided to provide support for videos, which are in fact
   *  images, but they cannot be read in with our current library functions.
   *  \return true if the input files are readable as images, false otherwise
   */
  virtual bool ImageReadable() const {
    return handler ? handler->ImageReadable() : false;
  } 

  ///
  virtual bool IsVideoFeature() const { 
    return handler ? handler->IsVideoFeature() : false; 
  }
  
  ///
  virtual bool IsImageFeature() const { 
    return handler ? handler->IsImageFeature() : false; 
  }
  
  ///
  virtual bool IsAudioFeature() const { 
    return handler ? handler->IsAudioFeature() : false; 
  }
  
  /** Returns false if per slice feature cannot be created by calculating
      per frame features.
  */
  virtual bool IsPerFrameCombinable() { 
    return !IsVideoFeature() && !IsAudioFeature(); 
  }
  
  /** Returns a list of the feature names
      \return a list of feature names
  */
  virtual list<string> Models() const { 
    list<string> mlist;
    const MuvisFeatHandler *xmlist = MuvisFeatHandler::list_of_methods;
    
    for (const MuvisFeatHandler *p = xmlist; p; p = p->next_of_methods)
      mlist.push_back(PREFIX+p->Name());
    return mlist;
  }

  virtual string Name() const {
    return handler ? handler->DescName() : "MuvisFeat";
  }

  virtual list<string> listNames() const{
    if(handler) return Feature::listNames();

    // list entry for multiple models

    list<string> l;
    const MuvisFeatHandler *xmlist = MuvisFeatHandler::list_of_methods;
    
    for (const MuvisFeatHandler *p = xmlist; p; p = p->next_of_methods)
      l.push_back(PREFIX+p->Name());
    return l;
  }

  virtual string LongName() const { 
    return handler ? handler->LongName(): "UNDEF"; 
  }

  virtual list<string> listLongNames() const{
    if (handler)
      return Feature::listLongNames();

    // list entry for multiple models

    list<string> l;
    const MuvisFeatHandler *xmlist = MuvisFeatHandler::list_of_methods;
    
    for (const MuvisFeatHandler *p = xmlist; p; p = p->next_of_methods)
      l.push_back(PREFIX+p->LongName());
    return l;
  }

  virtual string ShortText() const {
    return handler ? handler->ShortText() : "Muvis features";
  }

  virtual list<string> listShortTexts() const{
    if (handler)
      return Feature::listShortTexts();

    // list entry for multiple models

    list<string> l;
    const MuvisFeatHandler *xmlist = MuvisFeatHandler::list_of_methods;
    
    for (const MuvisFeatHandler *p = xmlist; p; p = p->next_of_methods)
      l.push_back(p->ShortText());
    return l;
  }

  virtual string TargetType() const {
    return handler ? handler->TargetType() : "UNDEF";
  }

  virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

  virtual list<string> listTargetTypes() const{
    if(handler) return Feature::listTargetTypes();

    // list entry for multiple models

    list<string> l;
    const MuvisFeatHandler *xmlist = MuvisFeatHandler::list_of_methods;
    
    for (const MuvisFeatHandler *p = xmlist; p; p = p->next_of_methods)
      l.push_back(p->TargetType());
    return l;
  }

  virtual string NeededSegmentation() const {
    return handler->NeededSegmentation(); 
  }

  virtual list<string> listNeededSegmentations() const{
    if(handler) return Feature::listNeededSegmentations();

    // list entry for multiple models

    list<string> l;
    const MuvisFeatHandler *xmlist = MuvisFeatHandler::list_of_methods;
    
    for (const MuvisFeatHandler *p = xmlist; p; p = p->next_of_methods)
      l.push_back(p->NeededSegmentation());
    return l;
  }

  virtual const string& DefaultZoningText() const {
    static string flat("1");
    return flat; 
  }

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const;
  
  virtual int printMethodOptions(ostream&) const;

  virtual execchain_t GetExecChain(int f, bool all, int l);

  virtual bool ExecPreProcess(int f, bool all, int l);

  virtual bool ExecPostProcess(int f, bool all, int l);

  virtual vector<string> UsedDataLabels() const {
    return handler ? handler->UsedDataLabels() : vector<string>();
  }

  virtual bool CalculateOneFrame(int f);

  virtual bool CalculateOneLabel(int f, int l);

  virtual bool ProcessOptionsAndRemove(list<string>&);
  
  ///
  static string fourcc2string(long v) {
    string s((char*)&v, 4);
    for (size_t i=0; i<s.size();) {
      if (s[i]==0) {
	s.erase(i, 1);
	continue;
      }
      i++;
    }
    return s;
  }
  
  class MuvisFeatHandler {
  public:
    // Constructor
    MuvisFeatHandler(const MuvisFeat *tparent) {
      parent=tparent;
      next_of_methods = NULL;
      length_tested = 0;
    }

    // 
    MuvisFeatHandler(bool) {
      next_of_methods = list_of_methods;
      list_of_methods = this;
    }
    
    // 
    virtual ~MuvisFeatHandler() {}

    //
    virtual MuvisFeatHandler *Create(const MuvisFeat *tparent) const = 0;

    //
    virtual bool ReadSO(const string&) = 0;

    //
    virtual bool BindParam() = 0;

    //
    virtual void ShowParam() const = 0;

    //
    //virtual void CopyMethods(const MuvisFeat::MuvisFeatHandler&) = 0;

    // 
    //virtual MuvisFeatHandler *Create(const MuvisFeat *tparent) const = 0;

    // 
    virtual string Name() const { return "XMLDescriptorHandler"; }

    //
    virtual int Length() const {
      return length_tested;
    }

    // 
    virtual string LongName() const = 0;

    // 
    virtual string DescName() const = 0;

    // 
    virtual string ShortText() const = 0;

    // 
    virtual string TargetType() const = 0;

    // 
    virtual string NeededSegmentation() const { return ""; }

    //
    virtual bool IsShapeDescriptor() const { return false; }

    // 
    param_t GetParameters() { return parameters; }

    //
    virtual bool SegmentPreProcess(int, int) { return true; }

    ///
    virtual bool IsImageFeature() const { return true; }

    ///
    virtual bool IsVideoFeature() const { return false; }

    ///
    virtual bool IsAudioFeature() const { return false; }
    
    // 
    virtual bool ImageReadable() const { return true; } 

    // 
    virtual vectortype GetData() { return data; }

    //
    virtual vector<string> UsedDataLabels() const {
      if (LabelVerbose())
	cout << "MuvisFeat::MuvisFeatHandler::UsedDataLabels()" << endl;
      return parent->UsedDataLabelsBasic();
    }

    // 
    static const MuvisFeatHandler *list_of_methods;

    // 
    const MuvisFeatHandler *next_of_methods;

    //
    static MuvisFeatHandler *FindMethod(const string& mname, 
					    const MuvisFeat *tparent) {
      const MuvisFeatHandler *found = NULL;
      for (const MuvisFeatHandler *p = list_of_methods; p; 
	   p = p->next_of_methods) {
	const string name = string(PREFIX)+p->Name();
	if (mname == name) found = p;
      }
      return found ? found->Create(tparent) : NULL;
    }

    //
    virtual int API_Init() = 0;

    //
    virtual int API_Exit() = 0;

    //
    virtual double* API_Calculate(int&) = 0;

    //
    vector<pair<string,double> > param;

    //
    double *ParamVec() const;

    //
    int length_tested;


  protected:

    vectortype data;
    vector<string> datanames;
    param_t parameters;

    // private:
    const MuvisFeat *parent;
  };
  
  virtual MuvisFeatHandler *getHandler() const {
    return handler;
  }
  
  class MuvisFexHandler : public MuvisFeatHandler {
  public:
    MuvisFexHandler(const MuvisFeat *tparent) : 
      MuvisFeatHandler(tparent) {
      fex_param = NULL;
    }
      
    MuvisFexHandler(bool) : MuvisFeatHandler(false) {}

    //
    virtual MuvisFeatHandler *Create(const MuvisFeat *tparent) const;

    // 
    virtual string Name() const {
      return fex_param ? fourcc2string(fex_param->feat_fourcc) : "";
    }    

    // 
    virtual string LongName() const { return Name(); }

    // 
    virtual string DescName() const {
      return fex_param ? fex_param->feat_name : "";
    }

    // 
    virtual string ShortText() const { return DescName(); }

    // 
    virtual string TargetType() const { return "image"; }

    //
    virtual bool ReadSO(const string&);

    //
    virtual bool BindParam();

    //
    virtual void ShowParam() const;

    //
    virtual void CopyMethods(const MuvisFeat::MuvisFexHandler&);

    //
    virtual int API_Init() {
      double *args = ParamVec();
      int p = (*fex_init)(args);
      delete args;
      return p;
    }

    //
    virtual int API_Exit() {
      return (fex_exit)(fex_param);
    }

    //
    virtual double* API_Calculate(int& len);

  protected:
    FEX_BIND     fex_bind;
    FEX_INIT     fex_init;
    FEX_EXTRACT  fex_extract;
    FEX_DISTANCE fex_distance;
    FEX_EXIT     fex_exit;
    FexParam    *fex_param;

  };
    
  class MuvisAFexHandler : public MuvisFeatHandler {
  public:
    MuvisAFexHandler(const MuvisFeat *tparent) : 
      MuvisFeatHandler(tparent) {
      fex_param = NULL;
    }
      
    MuvisAFexHandler(bool) : MuvisFeatHandler(false) {}

    //
    virtual MuvisFeatHandler *Create(const MuvisFeat *tparent) const;

    // 
    virtual string Name() const {
      return fex_param ? fourcc2string(fex_param->feat_fourcc) : "";
    }    

    // 
    virtual string LongName() const { return Name(); }

    // 
    virtual string DescName() const {
      return fex_param ? fex_param->feat_name : "";
    }

    // 
    virtual string ShortText() const { return DescName(); }

    // 
    virtual string TargetType() const { return "sound"; }

    //
    virtual bool ReadSO(const string&);

    //
    virtual bool BindParam();

    //
    virtual void ShowParam() const;

    //
    virtual void CopyMethods(const MuvisFeat::MuvisAFexHandler&);
    
    ///
    virtual bool IsImageFeature() const { return false; }

    ///
    virtual bool IsVideoFeature() const { return false; }

    ///
    virtual bool IsAudioFeature() const { return true; }
    
    ///
    virtual bool ImageReadable() const { return false; } 

    //
    virtual int API_Init();

    //
    virtual int API_Exit() {
      return (fex_exit)(fex_param);
    }

    //
    virtual double* API_Calculate(int& len);


  protected:
    AFEX_BIND     fex_bind;
    AFEX_INIT     fex_init;
    AFEX_EXTRACT  fex_extract;
    AFEX_DISTANCE fex_distance;
    AFEX_EXIT     fex_exit;
    AFexParam    *fex_param;
  };
 
protected:  
  /** The MuvisFeatData objects stores the data needed for the 
      feature calculation */
  class MuvisFeatData : public ExternalData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    MuvisFeatData(pixeltype t, int n, const MuvisFeat *tparent) :
      ExternalData(t, n,tparent) {
	parent = tparent;
	handler = parent->getHandler();
    }

    /// Defined because we have virtual member functions...
    virtual ~MuvisFeatData() {}

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "MuvisFeatData"; }
    
  protected:
    const MuvisFeat::MuvisFeatHandler *handler;
    const MuvisFeat *parent;

  };

  MuvisFeatHandler *handler;

private:
  vector<string> tempname, templog, temppar, tempxml, tempimage, 
    tempmask;
};
}
#endif

// Local Variables:
// mode: font-lock
// End:
