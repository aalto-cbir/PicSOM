// -*- C++ -*- 	$Id: MPEG7_XM.h,v 1.71 2014/01/24 15:16:20 jorma Exp $
/**
   \file MPEG7_XM.h

   \brief Declarations and definitions of class MPEG7_XM
   
   MPEG7_XM.h contains declarations and definitions of class the
   MPEG7_XM, which is a class that ...

   \author Mats Sjöberg <mats.sjoberg@hut.fi>
   $Revision: 1.71 $
   $Date: 2014/01/24 15:16:20 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _MPEG7_XM_
#define _MPEG7_XM_

#include "External.h"

#ifdef HAVE_OPENCV2_CORE_CORE_HPP
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#endif // HAVE_OPENCV2_CORE_CORE_HPP

namespace picsom {
  enum norm_mode {
    n_none,
    n_face1,
    n_face4
  };

  struct norm_opts {
    bool savegamma;
    bool ellipse;
  };

  enum img_region {
    img_all, img_tl, img_tr, img_bl, img_br
  };

  class MPEG7_XM : public External {
  public:
    /// definition for the current data type
    //typedef float datatype;  // this should be selectable in run time...
  
    /// definition for the current data vector type
    //typedef vector<datatype> vectortype;
  
    typedef map<string,string> param_t;

    static string PREFIX;

    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    MPEG7_XM(const string& img = "",
	     const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt); handler = NULL; forcexmversion = "";
      used_normalization = n_none;
      nopts.savegamma = false;
      nopts.ellipse = false;
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    MPEG7_XM(const string& img, const string& seg,
	     const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt); handler = NULL; forcexmversion = "";
      used_normalization = n_none; nopts.savegamma = false; nopts.ellipse = false;
    }

    //   MPEG7_XM(Segmentation *s, const list<string>& opt = (list<string>)0) {
    //     Initialize("", "", s, opt);
    //   }

    /// This constructor doesn't add an entry in the methods list.
    /// !! maybe it shouldn't but it does !!
    MPEG7_XM(bool) : External(false) {}

    // ~MPEG7_XM();

    virtual Feature *Create(const string& s) const {
      return (new MPEG7_XM())->SetModel(s);
    }

    virtual Feature *SetModel(const string& s) { 
      handler = XMLDescriptorHandler::FindMethod(s,this);
      if (handler == NULL) 
	cout << "MPEG7_XM::MPEG7_XMDATA: Error: " << s << " not implemented." 
	     << endl;
      return this; 
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
      return new MPEG7_XMData(t, n, this);

    }

    virtual void deleteData(void *p){
      delete (MPEG7_XMData*)p;
    }

    /** Overrided to provide support for videos, which are in fact
     *  images, but they cannot be read in with our current library functions.
     *  \return true if the input files are readable as images, false otherwise
     */
    virtual bool ImageReadable() const {
      return handler->ImageReadable();
    } 

    ///
    virtual bool IsVideoFeature() const { 
      return handler->IsVideoFeature(); 
    }
  
    ///
    virtual bool IsImageFeature() const { 
      return handler->IsImageFeature(); 
    }

    ///
    virtual treat_type DefaultWithinFrameTreatment() const {
      return handler->DefaultWithinFrameTreatment(); 
    }

    /** Returns false if per slice feature cannot be created by calculating
	per frame features.
    */
    virtual bool IsPerFrameCombinable() { return !IsVideoFeature(); }
  
    /** Returns a list of the feature names
	\return a list of feature names
    */
    virtual list<string> Models() const { 
      list<string> mlist;
      const XMLDescriptorHandler *xmlist = XMLDescriptorHandler::list_of_methods;
    
      for (const XMLDescriptorHandler *p = xmlist; p; p = p->next_of_methods)
	mlist.push_back(PREFIX+p->Name());
      return mlist;
    }

    virtual string RemoteExecutionOther() const { 
      char host[1000];
      gethostname(host, sizeof host);
      string hoststr = host, itlpc55 = "itl-pc55";
      if (hoststr.substr(0, itlpc55.size())==itlpc55)
	return "";

      return itlpc55;
    }

    virtual vector<string> RemoteExecution() const { 
      return vector<string>();
      // return RemoteExecutionLinux64();
    }

    virtual string Name() const {
      return handler ? handler->DescName() : "MPEG7";
    }

    virtual list<string> listNames() const{
      if(handler) return Feature::listNames();

      // list entry for multiple models

      list<string> l;
      const XMLDescriptorHandler *xmlist
	= XMLDescriptorHandler::list_of_methods;
    
      for (const XMLDescriptorHandler *p = xmlist; p; p = p->next_of_methods)
	l.push_back(PREFIX+p->Name());
      return l;
    }

    virtual string LongName() const {
      if (!handler)
	return "UNDEF";
      return handler->LongName()+" (XM version "+handler->GetXMVersion()+")";
    }

    virtual list<string> listLongNames() const{
      if (handler)
	return Feature::listLongNames();

      // list entry for multiple models

      list<string> l;
      const XMLDescriptorHandler *xmlist
	= XMLDescriptorHandler::list_of_methods;
    
      for (const XMLDescriptorHandler *p = xmlist; p; p = p->next_of_methods)
	l.push_back(PREFIX+p->LongName());
      return l;
    }

    virtual string ShortText() const {
      return handler ? handler->ShortText() : "MPEG7 XM Descriptors";
    }

    virtual list<string> listShortTexts() const{
      if(handler) return Feature::listShortTexts();

      // list entry for multiple models

      list<string> l;
      const XMLDescriptorHandler *xmlist = XMLDescriptorHandler::list_of_methods;
    
      for (const XMLDescriptorHandler *p = xmlist; p; p = p->next_of_methods)
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
      const XMLDescriptorHandler *xmlist = XMLDescriptorHandler::list_of_methods;
    
      for (const XMLDescriptorHandler *p = xmlist; p; p = p->next_of_methods)
	l.push_back(p->TargetType());
      return l;
    }

    virtual string NeededSegmentation() const {
      return handler->NeededSegmentation(); }

    virtual list<string> listNeededSegmentations() const{
      if(handler) return Feature::listNeededSegmentations();

      // list entry for multiple models

      list<string> l;
      const XMLDescriptorHandler *xmlist = XMLDescriptorHandler::list_of_methods;
    
      for (const XMLDescriptorHandler *p = xmlist; p; p = p->next_of_methods)
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

    virtual bool ExecPreProcess(int /*f*/, bool /*all*/, int /*l*/) {
      return true;
      //    return handler->ExecPreProcess(f, all, l); 
    }

    virtual bool ExecPostProcess(int f, bool all, int l);

    virtual vector<string> UsedDataLabels() const {
      return handler->UsedDataLabels();
    }

    virtual bool NeedsConversion() const {
      return handler->NeedsConversion();
    } 

    virtual bool CalculateOneFrame(int f);

    virtual bool SegmentPreProcess(int f, int l);

    virtual bool SegmentPostProcess(int f, int l);

    virtual bool SegmentRemoveTempFiles(int l);
  
    virtual bool ProcessOptionsAndRemove(list<string>&);

    virtual void SetEnsurePre() {
      for (size_t i=0; i<DataElementCount(); i++) {
	PreEnsure(temppar[i]);
	PreEnsure(tempname[i]);
	//      PreEnsure(templog[i]);
	//PreEnsure(tempxml[i]);
      }
    }

    virtual void SetEnsurePost() {
      for (size_t i=0; i<DataElementCount(); i++)
	PostEnsure(tempxml[i]);
    }

    virtual bool ImplementedServer(const string& stype) {
      string ttype = PREFIX+stype;
      return (XMLDescriptorHandler::FindMethod(ttype,NULL) != NULL);
    }

    virtual const string& TempDir() const {
      static const string dir = "/share/local/tmp";
      struct stat buf;

      return !stat(dir.c_str(), &buf) ? dir : External::TempDir();
    }

    const string GetTempImage(int l) const { return tempimage[l]; }

    string forcexmversion;

    string ForceXMVersion() const { return forcexmversion; }
    void ForceXMVersion(const string& s) { forcexmversion = s; }

    norm_mode used_normalization;
    norm_opts nopts;

    void SetNormalization(norm_mode n) { used_normalization = n; }
    norm_mode GetNormalization() const { return used_normalization; }
    bool UseNormalization() const { return used_normalization != n_none; }
 
    void SetNormalizationOpts(string&);

    class XMLDescriptorHandler {
    public:
      // Constructor
      XMLDescriptorHandler(MPEG7_XM *tparent) {
	parent=tparent;
	next_of_methods = NULL;
	parameters["CodingMode"] = "1";
	parameters["NoOfMatches"] = "8";
      }

      // 
      XMLDescriptorHandler(bool) {
	next_of_methods = list_of_methods;
	list_of_methods = this;
      }
    
      // 
      virtual ~XMLDescriptorHandler() {}

      // 
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const = 0;

      // 
      virtual bool HandleXML(xmlDocPtr doc, xmlNodePtr node);

      // 
      virtual bool HandleNode(xmlDocPtr doc, xmlNodePtr cur);

      // 
      virtual bool InDatanames(const xmlChar *name);

      // 
      virtual bool AddContentToData(xmlChar *content);

      // 
      virtual bool HandleContent(const xmlChar *name, xmlChar *content);

      // 
      virtual string Name() const { return "XMLDescriptorHandler"; }

      // 
      virtual int Length() const = 0;

      // 
      virtual string LongName() const = 0;

      // 
      virtual string DescName() const = 0;

      // 
      virtual string ShortText() const = 0;

      // 
      virtual string TargetType() const { return "image"; } // might be = 0

      // 
      virtual string NeededSegmentation() const { return ""; } // might be = 0

      //
      virtual bool IsShapeDescriptor() const { return false; }

      // 
      param_t GetParameters() { return parameters; }

      //
      virtual bool SegmentPrePreProcess() { return true;}

      //
      virtual bool SegmentPreProcess(int, int) { return true; }

      ///
      virtual bool IsImageFeature() const { return true; }

      ///
      virtual bool IsVideoFeature() const { return false; }

      ///
      virtual treat_type DefaultWithinFrameTreatment() const {
	return treat_concatenate; 
      }

      // 
      virtual bool ImageReadable() const { return true; } 

      // 
      virtual vectortype GetData() { return data; }

      //
      virtual vector<string> UsedDataLabels() const {
	if (LabelVerbose())
	  cout << "MPEG7_XM::XMLDescriptorHandler::UsedDataLabels()" << endl;
	return parent->UsedDataLabelsBasic();
      }

      //
      virtual bool NeedsConversion() const {
	string f = parent->SegmentData()->format();
	bool is_video = f.find("video/")==0;
	return IsImageFeature() && is_video;
      }

      // 
      static const XMLDescriptorHandler *list_of_methods;

      // 
      const XMLDescriptorHandler *next_of_methods;

      // 
      string GetXMVersion();

      // 
      virtual string GetPreferredXMVersion() { return "6.1"; }

      // 
      static XMLDescriptorHandler *FindMethod(const string& mname, 
					      MPEG7_XM *tparent) 
      {
	const xmlChar *n = (const xmlChar *)mname.c_str();
	const XMLDescriptorHandler *found = NULL;
	for (const XMLDescriptorHandler *p = list_of_methods; p; 
	     p = p->next_of_methods) 
	  {
	    const string name = string(PREFIX)+p->Name();
	    if (!xmlStrcmp(n,(const xmlChar *)name.c_str()))
	      found = p;
	  }
	if (found != NULL)
	  return found->Create(tparent);
	else 
	  return NULL;
      }

    protected:
      virtual vectortype toVectorType(xmlChar *content);
      virtual void AddVectorToData(vectortype v) {
	External::vectortype::iterator i;
	for (i = v.begin(); i != v.end(); i++) 
	  data.push_back(*i);
      }

      vectortype data;
      vector<string> datanames;
      param_t parameters;

      // private:
      MPEG7_XM *parent;
    };

    class XMLScalableColorHandler : public XMLDescriptorHandler {
    public:
      XMLScalableColorHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("Coefficients");
      }
      XMLScalableColorHandler(bool) : XMLDescriptorHandler(false) {}

      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLScalableColorHandler(tparent);
      }
  
      virtual string Name() const { return "ScalableColor"; }
      virtual string DescName() const { return PREFIX+"ScalableColor"; }
      virtual int Length() const { return 256; }
      virtual string LongName() const { return "Scalable Color"; }
      virtual string ShortText() const { 
	return "256-bin color histogram in HSV color space, encoded by a "
	  "Haar transform"; 
      }
    };

    class XMLColorLayoutHandler : public XMLDescriptorHandler {
    public:
      XMLColorLayoutHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("YDCCoeff");
	datanames.push_back("YACCoeff5");
	datanames.push_back("CbDCCoeff");
	datanames.push_back("CbACCoeff2");
	datanames.push_back("CrDCCoeff");
	datanames.push_back("CrACCoeff2");
      }
      XMLColorLayoutHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLColorLayoutHandler(tparent);
      }
    
      virtual string Name() const { return "ColorLayout"; }
      virtual string DescName() const { return PREFIX+"ColorLayout"; }
      virtual int Length() const { return 12; }
      virtual string LongName() const { return "Color Layout"; }
      virtual string ShortText() const { 
	return "Spatial distribution of dominant colors in YCbCr color system"; 
      }
    };

    class XMLContourShapeHandler : public XMLDescriptorHandler {
    public:
      XMLContourShapeHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	if (GetXMVersion() == "5.5") {
	  datanames.push_back("GlobalCurvatureVector");
	  datanames.push_back("PrototypeCurvatureVector");
	  datanames.push_back("HighestPeak");
	  datanames.push_back("xpeak");
	  datanames.push_back("ypeak");
	} else {
	  datanames.push_back("GlobalCurvature");
	  datanames.push_back("PrototypeCurvature");
	  datanames.push_back("HighestPeakY");
	  datanames.push_back("Peak"); //handled by special code...
	}
      }
      XMLContourShapeHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLContourShapeHandler(tparent);
      }

      //    virtual string GetPreferredXMVersion() { return "6.0"; }
      virtual bool IsShapeDescriptor() const { return true; }
    
      virtual string Name() const { return "ContourShape"; }
      virtual string DescName() const { return PREFIX+"ContourShape"; }
      virtual int Length() const { return 3; }
      virtual string LongName() const { return "Contour Shape"; }
      virtual string ShortText() const { 
	return "Circularity, eccentricity and highest peak information derived "
	  "from the Curvature Scale Space representation of the image."; 
      }

      virtual bool HandleXML(xmlDocPtr doc, xmlNodePtr node);
      virtual bool HandleNode(xmlDocPtr doc, xmlNodePtr cur);

      void AddVectorToX(vectortype& v) { AddVectorTo(peakX,v); }
      void AddVectorToY(vectortype& v) { AddVectorTo(peakY,v); }

    private:
      void AddVectorTo(vectortype& data, vectortype v) {
	External::vectortype::iterator i;
	for (i = v.begin(); i != v.end(); i++)
	  data.push_back(*i);
      }

      vectortype peakX, peakY;

    };

    class XMLColorStructureHandler : public XMLDescriptorHandler {
    public:
      XMLColorStructureHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("Values");
      }
      XMLColorStructureHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLColorStructureHandler(tparent);
      }
    
      virtual string Name() const { return "ColorStructure"; }
      virtual string DescName() const { return PREFIX+"ColorStructure"; }
      virtual int Length() const { return 256; }
      virtual string LongName() const { return "Color Structure"; }
      virtual string ShortText() const { 
	return "Local color structure expressed by using a structuring element "
	  "comprised of image samples";
      }
    };

    class XMLEdgeHistogramHandler : public XMLDescriptorHandler {
    public:
      XMLEdgeHistogramHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("BinCounts");
      }
      XMLEdgeHistogramHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLEdgeHistogramHandler(tparent);
      }
    
      virtual string Name() const { return "EdgeHistogram"; }
      virtual string DescName() const { return PREFIX+"EdgeHistogram"; }
      virtual int Length() const { return 80; }
      virtual string LongName() const { return "Edge Histogram"; }
      virtual string ShortText() const { 
	return "Histogram of five different edge types"; 
      }
    };

    class XMLRegionShapeHandler : public XMLDescriptorHandler {
    public:
      XMLRegionShapeHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("MagnitudeOfART");
      }
      XMLRegionShapeHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLRegionShapeHandler(tparent);
      }
    
      virtual bool IsShapeDescriptor() const { return true; }
      virtual string Name() const { return "RegionShape"; }
      virtual string DescName() const { return PREFIX+"RegionShape"; }
      virtual int Length() const { return 35; }
      virtual string LongName() const { return "Region Shape"; }
      virtual string ShortText() const { 
	return "35 ART coefficients calculated within a disc centered at center "
	  "of the image's Y channel"; 
      }
    };
  
    class XMLDominantColorHandler : public XMLDescriptorHandler {
    public:
      XMLDominantColorHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
      }
      XMLDominantColorHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLDominantColorHandler(tparent);
      }
    
      virtual string Name() const { return "DominantColor"; }
      virtual string DescName() const { return PREFIX+"DominantColor"; }
      virtual int Length() const { return 6; }
      virtual string LongName() const { return "Dominant Color"; }
      virtual string ShortText() const { 
	return "LUV color values of two most dominant colors"; 
      }
    
      virtual bool HandleXML(xmlDocPtr doc, xmlNodePtr node);
      virtual bool HandleNode(xmlDocPtr doc, xmlNodePtr cur);

      //  private:
      multimap<int,vectortype>::iterator GetLargestInHash();
      multimap<int,vectortype> hash;
    };

    class XMLHomogeneousTextureHandler : public XMLDescriptorHandler {
    public:
      XMLHomogeneousTextureHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("Average");
	datanames.push_back("StandardDeviation");
	datanames.push_back("Energy");
	datanames.push_back("EnergyDeviation");
      }
      XMLHomogeneousTextureHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLHomogeneousTextureHandler(tparent);
      }

      virtual string GetPreferredXMVersion() { return "5.5"; }
    
      virtual string Name() const { return "HomogeneousTexture"; }
      virtual string DescName() const { return PREFIX+"HomogeneousText"; }
      virtual int Length() const { return 62; }
      virtual string LongName() const { return "Homogeneous Texture"; }
      virtual string ShortText() const { 
	return "Mean and standard deviation of outputs of scale and orientation "
	  "sensitive filters"; 
      }
    };

    class XMLFaceRecognitionHandler : public XMLDescriptorHandler {
    public:
      XMLFaceRecognitionHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("Feature");
	parameters["FaceHist"] = "/share/imagedb/mats/XM-6.0/share/FaceHist.raw";
      }
      XMLFaceRecognitionHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLFaceRecognitionHandler(tparent);
      }

      virtual string Name() const { return "FaceRecognition"; }
      virtual string DescName() const { return PREFIX+"FaceRecog"; }
      virtual int Length() const { return 48; }
      virtual string LongName() const { return "FaceRecognition"; }
      virtual string ShortText() const { 
	return "The descriptor uses eigenface method to extract face features"; 
      }
      // this is still just a placeholder...
      virtual string NeededSegmentation() const { return "face.box"; }

      ///
      virtual treat_type DefaultWithinFrameTreatment() const {
	return treat_separate; 
      }

      //
      virtual bool SegmentPrePreProcess();

      //
      virtual bool SegmentPreProcess(int, int);

#ifdef HAVE_OPENCV2_CORE_CORE_HPP

      //
      void ApplyGamma(IplImage*, IplImage*, double g);
    
      //
      void NormalizeRegion(IplImage*, IplImage*, IplImage*, img_region);

#endif // HAVE_OPENCV2_CORE_CORE_HPP

      //
      virtual vector<string> UsedDataLabels() const {
	if (LabelVerbose())
	  cout << "MPEG7_XM::XMLFaceRecognitionHandler::UsedDataLabels()" << endl;
	vector<string> ret;
	// ret.push_back("1");
	return ret;
      }
    };

    class XMLAdvancedFaceRecognitionHandler : public XMLDescriptorHandler {
    public:
      XMLAdvancedFaceRecognitionHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	// fixme: just my guess of the XML tags, from looking at the XM code :)

	datanames.push_back("FourierFeature");
	datanames.push_back("CentralFourierFeature");
	datanames.push_back("CompositeFeature");
	datanames.push_back("CentralCompositeFeature");

	// parameters
	parameters["NoOfMatches"] = "200";

	/* Descriptor size setting */
	parameters["numOfFourierFeature"] = "24";
	parameters["numOfCentralFourierFeature"] = "24";
	parameters["extensionFlag"] = "0";
	parameters["numOfCompositeFeature"] = "0";
	parameters["numOfCentralCompositeFeature"] = "0";

	/* Descriptor size for SearchUtilities */
	parameters["numOfFourierFeatureSearch"] = "24";
	parameters["numOfCentralFourierFeatureSearch"] = "24";
	parameters["numOfCompositeFeatureSearch"] = "0";
	parameters["numOfCentralCompositeFeatureSearch"] = "0";

	/* Read Weighting Parameters from Parameter File: if not specified, default parameters are used */
	/* FaceParamFile	parfiles/AdvancedFaceRecognition1.prm */
	/* FaceParamFile	parfiles/AdvancedFaceRecognition2.prm */

	/* Output score format: (xm: XM format(default), tsc: score matrix) */
	/* ScoreFormat	tsc */
	parameters["ScoreFormat"] = "xm";
      }
      XMLAdvancedFaceRecognitionHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLAdvancedFaceRecognitionHandler(tparent);
      }

      virtual string Name() const { return "AdvancedFaceRecognition"; }
      virtual string DescName() const { return PREFIX+"AdvancedFaceRecog"; }
      virtual int Length() const { return 48; } //fixme: again just a guess
      virtual string LongName() const { return "AdvancedFaceRecognition"; }
      virtual string ShortText() const { 
	return "A descriptor of face identity robust to variations "
	  "in pose and illumination conditions. "; 
      }
      // this is still just a placeholder...
      virtual string NeededSegmentation() const { return "face.box"; }
    };

    class XMLMotionActivityHandler : public XMLDescriptorHandler {
    public:
      XMLMotionActivityHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	datanames.push_back("Intensity");
	// Make a translation from DominantDirection sectors (0...7)
	// to X and Y coordinates
	datanames.push_back("DominantDirectionX");
	datanames.push_back("DominantDirectionY");
	//datanames.push_back("SpatialParameters");
	datanames.push_back("Nsr");
	datanames.push_back("Nmr");
	datanames.push_back("Nlr");
	// This is 0 by default 
	// datanames.push_back("SpaLocNumber");
	// XM gives no output with SpaLocNumber 0 !
	// datanames.push_back("SpatialLocalizationParameters");
	datanames.push_back("TemporalParameters"); // 5 pcs
      
      }
      XMLMotionActivityHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLMotionActivityHandler(tparent);
      }

      //    virtual string GetPreferredXMVersion() { return "6.0 hossi"; }
      virtual bool IsImageFeature() const { return false; }
      virtual bool IsVideoFeature() const { return true; }
      // MPEG videos are not readable as images
      virtual bool ImageReadable() const { return false; }
      // Handle some content by ourselves
      virtual bool HandleContent(const xmlChar *name, xmlChar *content);
      virtual string Name() const { return "MotionActivity"; }
      virtual string DescName() const { return PREFIX+"MotionActivity"; }
      virtual int Length() const { return 11; }
      virtual string LongName() const { return "Motion Activity"; }
      virtual string ShortText() const { 
	return "This library implements the motion activity descriptor."
	  "Intensity, direction, spatial and temporal parameters implemented "
	  "but SLMA not yet supported.";
      }
      virtual string TargetType() const { return "video"; }
    };
  
    class XMLGoFGoPColorHandler : public XMLDescriptorHandler {
    public:
      XMLGoFGoPColorHandler(MPEG7_XM *tparent) : 
	XMLDescriptorHandler(tparent)
      {
	// Default aggregation mode is "Average", also "Median" is acceptable
	datanames.push_back("Coefficients"); //default number is 256
      
      }
      XMLGoFGoPColorHandler(bool) : XMLDescriptorHandler(false) {}
      virtual XMLDescriptorHandler *Create(MPEG7_XM *tparent) const {
	return new XMLGoFGoPColorHandler(tparent);
      }
    
      virtual string GetPreferredXMVersion() { return "6.0 hossi"; }
      // MPEG videos are not readable as images
      virtual bool ImageReadable() const { return false; }
      // Handle some content by ourselves
      //virtual bool HandleContent(const xmlChar *name, xmlChar *content);
      virtual string Name() const { return "GoFGoPColor"; }
      virtual string DescName() const { return PREFIX+"GoFGoPColor"; }
      virtual int Length() const { return 256; }
      virtual string LongName() const { return "GoF/GoP Color"; }
      virtual string ShortText() const { 
	return "Used to describe the color characteristics of a collection "
	  "of video frames. Uses ScalableColor to calculate "
	  "256-bin color histogram in HSV color space.";
      }
      virtual string TargetType() const { return "video"; }
    };
  
    virtual XMLDescriptorHandler *getHandler() const {
      return handler;
    }
 
  protected:  
    /** The MPEG7_XMData objects stores the data needed for the 
	feature calculation */
    class MPEG7_XMData : public ExternalData {
    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      MPEG7_XMData(pixeltype t, int n, const MPEG7_XM *tparent) :
	ExternalData(t, n,tparent) {
	parent = tparent;
	handler = parent->getHandler();
      }

      /// Defined because we have virtual member functions...
      virtual ~MPEG7_XMData() {}


      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      virtual int Length() const { 
	return handler->Length();
      }

      /** Returns the name of the feature
	  \return feature name
      */    
      virtual string Name() const { return "MPEG7_XMData"; }
    
      ///
      virtual void SetVector(vectortype v) {
	datavec = v;
      }
    
    protected:
      const MPEG7_XM::XMLDescriptorHandler *handler;
      const MPEG7_XM *parent;

    };
  
    virtual bool SetFromXML(MPEG7_XMData* d,string xmlfile,int l,int maxc);
    xmlNodePtr SearchNode(xmlNodePtr node, const xmlChar *name);
    xmlNodePtr SearchNextNode(xmlNodePtr node, const xmlChar *name);

    XMLDescriptorHandler *handler;

  private:
    vector<string> tempname, templog, temppar, tempxml, tempimage, 
      tempmask;
  };
} // namespace picsom
#endif

// Local Variables:
// mode: font-lock
// End:
