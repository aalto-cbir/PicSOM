// -*- C++ -*-  $Id: Segmentation.h,v 1.117 2013/01/25 16:27:41 jorma Exp $
// 
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _SEGMENTATION_H_
#define _SEGMENTATION_H_

#define IMAGE_PRESENT_CHECK

#include <segmentfile.h>

#include <string.h>
#include <math.h>

#include <iostream>
#include <list>
using namespace std;

namespace picsom {
  ///
  void addToEscapedString(string &str, char *a);

  ///
  string collectEscapedString(int argc, char **argv);

  ///
  string processArg(const char *str);
 

  class Switcherror{
  public:
    const char *method;
    Switcherror(const char *c): method(c) {}
  };

  class Segmentation : public ChainedMethod {
  public:
    static const string& PicSOMroot();

    static void PicSOMroot(const string& r) { picsomroot = r; }

    class BoolArraySeg {
    public:
      BoolArraySeg(Segmentation *s,bool initial=false,bool out=false) : 
	out_mark(out),seg(s)
      {
	int size,i;
	
	size=seg->getImg()->getWidth()*seg->getImg()->getHeight();
	array = new bool[size];
	for(i=0;i<size;i++) array[i]=initial;
	
      };

      ~BoolArraySeg(){delete[] array;};

      bool Ask(const coord &c) const {
	if(!seg->getImg()->coordinates_ok(c.x,c.y))
	  return out_mark;
	else
	  return array[c.x+c.y*seg->getImg()->getWidth()];
      }

      bool Ask(int x,int y ) const {
	if(!seg->getImg()->coordinates_ok(x,y))
	  return out_mark;
	else
	  return array[x+y*seg->getImg()->getWidth()];
      }

      void Set(const coord &c, bool value=true) {
	if(!seg->getImg()->coordinates_ok(c.x,c.y))
	  out_mark = value;
	else
	  array[c.x+c.y*seg->getImg()->getWidth()]=value;
      }
      
    private:
      bool *array;
      bool out_mark;
      Segmentation *seg;
    };
  
  protected:
    /// 
    Segmentation();

    /// 
    Segmentation(bool);

    /// 
    Segmentation(const Segmentation& s) : ChainedMethod(s) {
      cout << "Segmentation(const Segmentation&) called" << endl;
      abort();
    }

  public:

    ///
    static void MainShowUsage(const string& n);

    ///
    static int Main(const vector<string>&);

    ///
    static int Main(int argc, char **argv);

    ///
    static int ViewXML(int argc, char **argv);

    /// poisoned
    bool ScaleImage(const Segmentation *src,
		    int tlx, int tly, int brx, int  bry, int w, int h);
  
    ///
    virtual ~Segmentation();

    /// implementations for pure virtual methods declared in 
    // class ChainedMethod

    ///
    virtual MethodInfo getMethodInfo() const;

    ///
    virtual const ChainedMethod *getNextMethod()const{
      return next_method;
    }

    /// The following six pure virtual methods need to be overloaded:

    ///
    virtual Segmentation *Create() const = 0;

    ///
    virtual const char *Version() const = 0;

    ///
    virtual void UsageInfo(ostream& = cout) const = 0;
    
    ///
    virtual const char *MethodName(bool = false) const = 0;
    
    ///
    virtual const char *Description() const = 0;
    
    /// 
    virtual bool Process() = 0;

    /// The last virtual methods need to be overloaded only if needed.

    ///
    virtual bool ProcessGlobal(){ return true;} 

    ///
    virtual int ProcessOptions(int, char**) { return 0; }

    ///
    virtual bool PreventOutput() const { return false; }
    
    ///
    virtual bool OpenExtraFiles() { return true; }
    
    ///
    virtual bool CloseExtraFiles(){ return true; }

    ///
    virtual bool ReadInputDescription() { return true; }
    
    ///
    // virtual bool ZeroIsBackground() const { return true; }
    
    ///
    virtual bool IncludeImagesInXML() const {return false; }
    
    virtual bool Cleanup() {return CleanupPreviousMethod();}

    /// Non-virtual methods start here.

    ///
    bool ProcessAllFrames(int framestep=1);

    ///
    bool SeekToFrame(int f) { seek_to_frame = f; return true; }

    ///
    static void ListMethods(ostream& = cout, bool = false);

    ///
    static bool ShowUsage(const char* = NULL, ostream& = cout);

    ///
    static bool ShowVersion(const char* = NULL, ostream& = cout);

    ///
    static Segmentation *CreateMethod(const char*);

    ///
    static int Verbose() { return verbose; }
    
    ///
    static void Verbose(int l) { verbose = l; }
    
    ///
    static void AddVerbose(int l = 1) { verbose += l; }
    
    ///
    static void EchoError(const string& str);
   
    /// Concatenates this and all next_methods' short names with '+'.
    string ChainedMethodName() const;
    
    ///
    const Segmentation *FirstMethod() const {
      return previous_method ? previous_method->FirstMethod() : this;
    }

    ///
    Segmentation *FirstMethod() {
      return previous_method ? previous_method->FirstMethod() : this;
    }

    ///
    const Segmentation *LastMethod() const {
      return next_method ? next_method->LastMethod() : this;
    }
    
    ///
    Segmentation *LastMethod() {
      return next_method ? next_method->LastMethod() : this;
    }

    ///
    Segmentation *NextMethod(){ return next_method; }

    ///
    bool CoordinatesOK(int x, int y) const {
      return FirstMethod()->getImg()->coordinates_ok(x, y);
    }

    ///
    void NextMethod(Segmentation *s) {
      if (next_method)
	cerr << "Segmentation::NextMethod() : next_method already set." << endl;
      next_method = s;
      s->previous_method = this;
    }
    
    ///
    void ShowLinks() const {
      cout << MethodName(true) << ": this=0x" << this
		<< " previous_method=0x" << previous_method;
      if (previous_method)
	cout << "(" << previous_method->MethodName(true) << ")";
      cout << " next_method=0x" << next_method;
      if (next_method)
	cout << "(" << next_method->MethodName(true) << ")";
      cout << endl;
    }
    
    ///
    bool ProcessNextMethod();

    ///
    bool ProcessGlobalNextMethod();

    ///
    bool CleanupPreviousMethod();

    ///
   
    /// Associates images and files.
    bool SetFileNames();

    ///
    void SetPreviousNames(const string &m){
      previous_methods = m;
    }

    const string &GetPreviousNames(){ return previous_methods;}

    /// Flushes and closes all files.
    bool CloseFiles();

    /// Closes only the image and out_segment files.
    bool CloseFilesInner();

    /// Checks that image has been read in.
    bool HasImage() const { 
      if(!image) return false;
      return getImg()->getWidth()*getImg()->getHeight(); }

    /// Checks that segmentation has been generated or read in.
    // should this be recursive
    bool HasSegmentation() const { 
      if(has_segments) return true;
      if(previous_method) return previous_method->HasSegmentation();
      else return false;
    }
   
    /// gives access to chain code decomposition of image
    Chains *GetChains(bool prev=false, bool return_empty=false){
      return (prev || (!chains_produced&&!return_empty)) && previous_method ? 
	      previous_method->GetChains() : &chains;
    }

    /// declare that segmentation return has produced chain codes
    void AnnounceChains();

    ///
    void DisplayInfo();

    ///
    const string& ResultFiles() const { return result_files; }
  
    ///
    void AddToResultFiles(const string& n) {
      if (result_files.length()) result_files += ", ";
      result_files += n; }

    ///
    void ClearResultFiles() { result_files = ""; }
    
    /// 
    bool ChainsProduced() const {return chains_produced;};

    ///
    bool ExtractChains();
    
    ///
    bool AddSegments(Chains &ch);

    ///
    bool AddBorders(SubSegment &subs,const coord &seed, bool outeronly=false);

    ///
    PixelChain *TraceContour(const coord &seed,PixelChain &chain,
			   bool use_diagonals=true); 
    
  private:
    
    /// This is used by methods that work only for the first method in the chain.
    void ensure_first_method() const {
      if (previous_method)
	throw string("Segmentation : not the first method");
    }

    /// This is used by methods that work only when an input image exists.
    void ensure_first_frame() const {
       if (getImg()->hasImages())
	throw string("Segmentation : no image loaded");
    }
    
  public:


    ///
    const string *GetInputImageName() const {
      if (previous_method)
	return previous_method->GetInputImageName();
      else
	return &image_name;
    }

    const string *GetInputSegmentName() const {
      if (previous_method)
	return previous_method->GetInputSegmentName();
      else
	return &in_segment_name;
    }

    const string *GetOutputSegmentName() const {
      if (previous_method)
	return previous_method->GetOutputSegmentName();
      else
	return &out_segment_name;
    }


    const Optionlist Options() const { return options;}

    void addToSwitches(const Option &o){ options.list.push_back(o);};

    /// This is used by Feature::CalculatePerFrame(f).
    // bool ReadImageFrame(int f) { return OpenImageFile(NULL, f); }

  private:
    ///
    void Initialize();

  protected:
    ///
    //virtual bool CreateRealOutFile(const char *n, int w, int h);

    /** Issues a function starting message if enough verbosity.
	\param f function name with or without (...).
    */ 
    void StartMessage(const string& f) const {
      if (Verbose()>2)
	cout << f << (f.find('(')!=string::npos?"":"()")
	     << " starting" << endl;
    }

    /** Issues a function finishing message if enough verbosity.
	\param f function name with or without (...).
    */ 
    void FinishMessage(const string& f) const {
      if (Verbose()>2)
	cout << f << (f.find('(')!=string::npos?"":"()")
	     << " finishing" << endl;
    }

    /** Returns true if enough verbosity for image displays for debugging purposes.
	\return true if enough verbosity.
    */
    bool DebugDisplay() const { return Verbose()>4; }

    /** Displays the argument image for debugging purposes if enough verbosity.
	\param img image.
	\param set settings unless not deafulting to debug_disp_set.
    */
    void DebugDisplay(const imagedata& img,
		      const imagefile::displaysettings& set) const {
      if (DebugDisplay())
	imagefile::display(img, set);
    }
    
    void DebugDisplay(const imagedata& img) const {
      DebugDisplay(img, debug_disp_set);
    }

    /** Returns true if memory usage is to be debugged
	\return boolean true if memory usage is debugged
    */
    static bool DebugMemoryUsage() { return debug_memory_usage; }

    /** Sets for memory usage debugging
    */
    static void DebugMemoryUsage(bool d) { debug_memory_usage = d; }

    /** 
	\return boolean true if background labels are tagged as 
	frameattributes
    */

    static bool TagBackgroundLabels() { return tag_background_labels; }

    /** Sets for memory usage debugging
    */
    static void TagBackgroundLabels(bool t) { tag_background_labels = t; }
    
  private:
    
    ///
    bool WriteOutfile();


    ///
    void ImagePresentCheck() const {
#ifdef IMAGE_PRESENT_CHECK
      if(!image) throw string("Segmentation::ImagePresentCheck() failed");
#endif
    }

  private:
    /// 
    static Segmentation *list_of_methods; 

  protected:
    ///
    Segmentation *next_method;

    ///
    Segmentation *previous_method;


  private:

    /// 
    segmentfile *image;

    ///
    int seek_to_frame;

  public:
    segmentfile *getImg() const {
      ImagePresentCheck(); 
      return image;
    }

    /// Width of the image obtained from segmentfile.
    int Width() const { return getImg()->getWidth(); }

    /// Height of the image obtained from segmentfile.
    int Height() const { return getImg()->getHeight(); }

    /// An utility for accessing image data.
    bool GetPixelRGB(int x, int y, float& r, float& g, float& b) const {
      try {
	getImg()->get_pixel_rgb(x, y, r, g, b);
	return true;
      }
      catch (...) {
	return false;
      }
    }

    /// An utility for accessing image data.
    bool GetPixelGrey(int x, int y, float& v) const {
      try {
	getImg()->get_pixel_grey(x, y, v);
	return true;
      }
      catch (...) {
	return false;
      }
    }

    /// An utility for accessing image data.
    bool SetPixelGrey(int x, int y, float v) const {
      try {
	getImg()->set_pixel_grey(x, y, v);
	return true;
      }
      catch (...) {
	return false;
      }
    }

    /// An utility for setting segment values.
    bool SetPixelSegment(int x, int y, int s) const {
      try {
	getImg()->set_pixel_segment(x, y, s);
	return true;
      }
      catch (...) {
	return false;
      }
    }

    /// An utility for setting segment values.
    bool SetPixelSegment(double x, double y, int s) const {
      return SetPixelSegment((int)floor(x+0.5), (int)floor(y+0.5), s);
    }

    /// An utility for getting XML form results.
    SegmentationResultList *ReadResultsFromXML(const string& n,
					       const string& t = "",
					       bool wc = false) const {
      try {
	return getImg()->readFrameResultsFromXML(n, t, wc);
      }
      catch (...) {
	return NULL;
      }
    }

    /// An utility for setting XML form results.
    bool AddResultToXML(const SegmentationResult& r) const {
      try {
	getImg()->writeFrameResultToXML(this, r);
	return true;
      }
      catch (...) {
	return false;
      }
    }

  protected:
    ///
    string image_name;
    
    /// 
    string in_segment_name;
    
    /// 
    string out_segment_name;

    /// Image display parameters for debugging purposes.
    imagefile::displaysettings debug_disp_set;

  private:
    ///
    
    static int verbose;
    
    /// true if memory usage is to be used between frames
    static bool debug_memory_usage;

    ///
    /*static*/ string previous_methods; // records the names of the methods 
                                         // read from the input 
                                         // segmentation file
    
    ///
    Chains chains;
    
    /// 
    bool chains_produced;
    
    ///
    string result_files;

    ///
    Optionlist options;

    bool prevent_output;

    ///
    static bool tag_background_labels;

    /// jl
    bool read_all_frames_at_once;

    ///
    bool has_segments;
  public:
    ///
    void announceSegmentations(){has_segments=true;}

    ///
    void setStorage(segmentfile *s) {
      FirstMethod()->_set_image_ptr(s);
    }

  private:
    void _set_image_ptr(segmentfile *s);

  protected:
    ///
    list<string> method_descriptions;

    ///
    static string picsomroot;

  }; // class Segmentation
  
} // namespace picsom

#endif // _SEGMENTATION_H_

// Local Variables:
// mode: font-lock
// End:

