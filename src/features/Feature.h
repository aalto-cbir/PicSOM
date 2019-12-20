// -*- C++ -*-  $Id: Feature.h,v 1.220 2019/04/01 09:43:05 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file Feature.h

   \brief Declarations and definitions of class Feature
   
   Feature.h contains declarations and definitions of
   class Feature, which is a base class for all the other 
   features.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.220 $
   $Date: 2019/04/01 09:43:05 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _Feature_h_
#define _Feature_h_

#include <Util.h>
#include <videofile.h>
#include <soundfile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm> 
#include <numeric> 
#include <list>
#include <map> 
#include <set>
#include <stdexcept>

using namespace std;

#include <segmentfile.h>
#include <XMLutil.h>

//using namespace picsom;

#include <cox/labeled_vector>
//using namespace ::cox;

// #ifndef __MINGW32__
#include <cox/tictac>
// #endif // __MINGW32__

#include <sys/stat.h>

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

namespace picsom {
  using namespace ::cox;

  /// alpha requires full definition here, not just forward declaration...
  class CompositeRegion {
  public:
    set<int> primitiveRegions;
    int id;
  };

  ///
  class feature_result {
    // using cox::labeled_float_vector;

  public:
    ///
    feature_result() {}
    
    ///
    // feature_result(const feature_result&) { abort(); }

    ///
    size_t dimension() const {
      return data.empty() ? 0 : data.begin()->first.size();
    }

    ///
    void append(const labeled_float_vector& v) throw(string) {
      if (!data.empty() && dimension()!=v.first.size())
	throw string("feature_result::append(vector) : dimension mismatch "
		     +ToStr(dimension())+"!="+ToStr(v.first.size()));
      data.push_back(v);
    }

    ///
    void append(const feature_result& src) throw(string) {
      if (!data.empty() && dimension()!=src.dimension())
	throw string("feature_result::append(feature_result) : "
		     "dimension mismatch "
		     +ToStr(dimension())+"!="+ToStr(src.dimension()));
      data.insert(data.end(), src.data.begin(), src.data.end());
    }

    ///
    XmlDom xml;  // obs! does the default copy constructor break this?

    ///
    list<labeled_float_vector> data;
  }; // class feature_result

  ///
  typedef pair<pair<string,string>,vector<float> > incore_feature_t;

  ///
  class feature_batch_e {
  public:
    ///
    feature_batch_e(const string& f, const incore_feature_t *i,
		    feature_result *r) : fname(f), incore(i), result(r) {}

    ///
    string str() const {
      stringstream ss;
      ss << "\"" << fname << "\" " << (void*)incore << " " << (void*)result;
      return ss.str();
    } 
    
    ///
    string fname;

    ///
    const incore_feature_t *incore;

    ///
    feature_result *result;
  };

  ///
  typedef list<feature_batch_e> feature_batch;
  
  /// A base class for all features that can be used in the feature extraction.
  class Feature {
  public:
    /// the feature vector
    typedef vector<float> featureVector;

    ///
    static const string& PicSOMroot();

    ///
    static void PicSOMroot(const string& r) { picsomroot = r; }

#ifdef HAVE_GLOG_LOGGING_H
    ///
    static void GlogInitDone(bool d) { glog_init_done = d; } 

    ///
    static bool GlogInitDone() { return glog_init_done; } 
#endif // HAVE_GLOG_LOGGING_H

    ///
    static void GpuDeviceId(int d) { gpu_device_id = d; }

    ///
    static int GpuDeviceId() { return gpu_device_id; }

    /** Replacement for main()
     */
    static int Main(int argc, const char **argv) {
      vector<string> args;
      for (int i=0; i<argc; i++)
	args.push_back(argv[i]);

      return Main(args);
    }

    /**
     */
    static int Main(const vector<string>& args) {
      list<incore_feature_t> incore;

      return Main(args, incore);
    }
    
    /** The real thing
     */
    static int Main(const vector<string>&, list<incore_feature_t>&,
		    feature_result* = NULL); 

    /** Per file loop inside Main().
     */
    static bool ProcessOneFile(Feature*&, const string&, const string&,
			       const list<string>&, const string&,
			       incore_feature_t*,
			       bool, bool, bool, bool, const string&,
			       const string&, const string&, const string&,
			       const string&, bool, int&,
			       size_t, int, int, const string&, bool,
			       const string&, int, const string&, const string&,
			       vector<string>&, feature_result*,
			       pair<string,ofstream>&);

    /** A helper routine for Main().
     */
    static void ShowUsage(const string&);

    /** A helper routine for Main().
     */
    static pair<bool,int> SolveVerbosity(const string&);

    /** Returns true if feature data can be reinitialized between
	files by calling ReInitialize().
    */
    virtual bool ReInitializable() const { return false; }

    /** Creates a new instance of a Feature object
	\return a pointer to the object
    */
    virtual Feature *Create(const string&) const = 0;

    /** Reinitializes feature data between files.
	This is a basic operation that can be called from derived classes.
	As such is might not be enough...
    */
    virtual bool ReInitialize() { 
      if (MethodVerbose())
	cout << "Feature::ReInitialize()" << endl;

      RemoveDataLabelsAndIndices();
      RemoveDataElements();
      RemoveSegmentFile();
      RemoveVideoFile();
      RemoveAudioFile();
      RemoveRegionSpec();  // not sure about this...

      return true;
    }

    /**
     */
    void RemoveSegmentFile() {
      delete allocated_segmentfile;
      allocated_segmentfile = segmentfile_ptr = NULL;
    }

    ///
    void RemoveVideoFile() {
      delete videofile_ptr;
      videofile_ptr = NULL;
    }

    ///
    void RemoveAudioFile() {
      delete deprecated_audiofile_ptr;
      deprecated_audiofile_ptr = NULL;
    }
    
    /**
     */
    void RemoveRegionSpec() {
      for (size_t i=0; i<region_spec.size(); i++)
        delete region_spec[i].first;
      region_spec.clear();
    }

    /** Sets the model
	\return a pointer to the object
    */
    virtual Feature *SetModel(const string&) { return this; }

    /** Loads the data.
	This function should be overriden by non-image-based features.
	\param datafile the name of the object file
	\param segmentfile the name of the segmentation file (default "")
	\return true if data was successfully loaded, otherwise false.
    */
    virtual bool LoadObjectData(const string& /*datafile*/, 
				const string& /*segmentfile*/ = "") { 
      return true;
    }

    virtual bool IsSparseVector() const { return false; }

    /// Is the feature a video feature? Default is false.
    virtual bool IsVideoFeature() const { return false; }
    
    /// Is the feature a audios feature? Default is false.
    virtual bool IsAudioFeature() const { return false; }
    
    /// Is the feature an image feature? Override with a false-returning 
    /// function if not.
    virtual bool IsImageFeature() const { return true; }

    /// Returns true if new segmentfile() can be constructed from the
    /// type of objects used by the feature.
    virtual bool ImageReadable() const { return IsImageFeature(); }

    /** Returns false if per slice feature cannot be created by calculating
	per frame features.
    */
    virtual bool IsPerFrameCombinable() { return true; } // = 0;

    /**
     */
    virtual bool IsBatchOperator() { return false; }

    /**
     */
    virtual bool CollectBatch(const string&, incore_feature_t*, 
			      feature_result*);

    /**
     */
    virtual bool ProcessBatch() { return false; }

    /** Returns true if can be invoked with -R swithch.
     */
    virtual bool HasRawInMode() const { return false; }

    /** Returns true if can be invoked with -r swithch.
     */
    virtual bool HasRawOutMode() const { return false; }

    /** Returns true if invoked with -R swithch.
     */
    virtual bool IsRawInMode() const { return raw_in_mode; }

    /** Returns true if invoked with -r swithch.
     */
    virtual bool IsRawOutMode() const { return raw_out_mode; }

    /** Prints and appends raw vector.
     */
    virtual bool ProcessRawOutVector(const vector<float>&, int, int);

    /** Prints the given raw vector.
     */
    virtual bool PrintAndAppendRawVector(const vector<float>&, int, int);

    /** Reads, stores and returns reference to the next raw input vector.
     */
    const vector<float>& NextRawVector(int, int);

    /** Report failure and exit.
     */
    void FailInRawInMode(const string& f) const {
      if (IsRawInMode()) {
	cerr << "Cannot do " << f << " when in raw input mode" << endl;
	abort();
      }
    }
  
    /** Sets intermediate filename. This is used for External classes
     * e.g.
     */
    void SetIntermediateFilename(const string& n) {
      RemoveIntermediateFile();
      if (FrameVerbose())
        cout << "Feature::SetIntermediateFilename() : setting <"
	     << n << ">" << endl;
 
      intermediate_filename = n;
    }

    void RemoveIntermediateFile() {
      if (!intermediate_filename.empty()) {
	if (!KeepTmp()) {
	  if (FrameVerbose())
	    cout << "Feature::RemoveIntermediateFile() : unlinking <"
		 << intermediate_filename << ">" << endl;
	  Unlink(intermediate_filename);
	}
	intermediate_filename = "";
      }
    }

    /** Returns true if the output is histogram instead of average.
     */
    bool IsHistogramMode() const { return !histogramMode.empty(); }

    /** Access to in-core results.
     */
    const feature_result& FeatureResult() const { return result; }

    /** Some brief information of the results.
     */
    static string ResultSummaryStr(const feature_result&);

    /** Adds one result in the result list.
     */
    bool AppendResult(const featureVector& v, const string& s) {
      return AppendResult(make_pair(v, s));
    }

    /** Adds one result in the result list.
     */
    bool AppendResult(const labeled_float_vector& v) {
      try {
	result.append(v);
	return true;
      }
      catch (...) {
	return false;
      }
    }

    /** Stores intermediate results for future use if necessary.
     */
    bool SaveIntermediate(int frame, const list<string>&) const;

    /** Restores intermediate results from previous call if possible.
     */
    list<string> ReadIntermediate(int frame) const;

    /** Restores intermediate results from previous call if possible.
     */
    list<string> ReadIntermediateInner(const list<string>&) const;

    /** Lists filenaes used to keep intermediate results.
     */
    list<string> IntermediateFiles(int frame) const;

    /** Stub for itermediate file names.
     */
    string IntermediateFileStub(int frame) const;

    /** Simple file reader utility.
     */
    string ReadFile(const string&) const;

    /** Set the verbose/debug level
	\param v the new verbose level
	\return the old verbose level
    */
    static int Verbose(int v) { int o = verbose; verbose = v; return o; }

    /** Adds to the current verbose level
	\param l value to add (default 1)
    */
    static void AddVerbose(int l = 1) { verbose += l; }

    /** Returns the current verbose level
	\return the current verbose level
    */
    static int Verbose() { return verbose; }

    /** Returns true if verbose level is set to show method info
	\return boolean saying if verbose level is set to show method info
    */
    static bool MethodVerbose() { return verbose>0; }

    /** Returns true if verbose level is set to show file info
	\return boolean saying if verbose level is set to show file info
    */
    static bool FileVerbose() { return verbose>1; }

    /** Returns true if verbose level is set to show slice info
	\return boolean saying if verbose level is set to show slice info
    */
    static bool SliceVerbose() { return verbose>2; }

    /** Returns true if verbose level is set to show frame info
	\return boolean saying if verbose level is set to show frame info
    */
    static bool FrameVerbose() { return verbose>3; }

    /** Returns true if verbose level is set to show label info
	\return boolean saying if verbose level is set to show label info
    */
    static bool LabelVerbose() { return verbose>4; }

    /** Returns true if verbose level is set to show key-point info
	\return boolean saying if verbose level is set to show key-point info
    */
    static bool KeyPointVerbose() { return verbose>5; }

    /** Returns true if verbose level is set to show pixel info
	\return boolean saying if verbose level is set to show pixel info
    */
    static bool PixelVerbose() { return verbose>6; }

    /** Returns true if verbose level is set to show more than pixel info
	\return boolean saying if verbose level is set to show all details
    */
    static bool FullVerbose() { return verbose>7; }

    /** Returns true if memory usage is to be debugged
	\return boolean true if memory usage is debugged
    */
    static bool DebugMemoryUsage() { return debug_memory_usage; }

    /** Sets for memory usage debugging
    */
    static void DebugMemoryUsage(bool d) { debug_memory_usage = d; }

    /// this is the stream to which all non-debug output should be directed
    /// it is not nice that it is static, however...
    ostream *fout_xx; 

    /** Access to image segmentation information.
     */
    segmentfile *SegmentData() const throw(string) {
      if (!segmentfile_ptr)
	throw string("segmentdata pointer is null");
      return segmentfile_ptr;
    }

    /** sets input pattern for segment files
	\param p the new input pattern
    */
    // void SetSegmentInputPattern(const string& p) {
    //   SegmentData()->SetInputPattern(p.c_str());
    // }

    /** sets standard naming convention for segment files
	\param p the new input naming pattern
    */
    // void SetStandardSegmentInputNaming(const string& p) {
    //   SegmentData()->SetStandardInputNaming(p.c_str(), 0);
    // }

    /** sets output pattern
	\param n the output pattern
    */
    void SetOutputPattern(const string& n) {
      output_pattern = n;
      if (FileVerbose())
	cout << "Feature::SetOutputPattern() set to \""
	     << output_pattern << "\"" << endl;
    }

    /** sets the actual filename that output should be directed to
	\param n the filename
    */
    void SetOutputFilename(const string& n) { output_filename = n; }

    /** sets raw input pattern
	\param n the raw input pattern
    */
    void SetRawInputPattern(const string& n) {
    raw_input_pattern = n;
      if (FileVerbose())
	cout << "Feature::SetRawInputPattern() set to \""
	     << raw_input_pattern << "\"" << endl;
    }

    /** sets the filename that will be read for raw input
	\param n the filename
    */
    void SetRawInputFilename(const string& n) { raw_input_filename = n; }

    /// sets the output pattern to one of the five standard ones
    void SetStandardRawInputNaming(int t) { 
      SetRawInputPattern(StandardFilenamePattern(t, true)); 
    }

    /// sets the output pattern to one of the five standard ones
    void SetStandardOutputNaming(int t, bool r) { 
      SetOutputPattern(StandardFilenamePattern(t, r)); 
    }

    /** Returns the raw input pattern
	\return the raw input pattern
    */
    const string& RawInputPattern() const { return raw_input_pattern; }

    /** Returns the output pattern
	\return the output pattern
    */
    const string& OutputPattern() const { return output_pattern; }

    /** Returns the output filename
	\return the output filename
    */
    const string& OutputFilename() const { return output_filename; }

    /** Sets the output filename from the current pattern.
	\param f the feature name, eg. rgb, replaced into \%f in the pattern
	\param m the segmentation method, replaced into \%m
	\param i the input file name, replaced into \%i
    */
    void SetOutputFilename(const string&, const string&, const string&);

    /** Forms a filename from the current pattern.
	\param f the feature name, eg. rgb, replaced into \%f in the pattern
	\param m the segmentation method, replaced into \%m
	\param i the input file name, replaced into \%i
    */
    string FilenameFromPattern(const string&, const string&, const string&,
			       const string&) const;

    /** Returns the raw input filename
	\return the raw input filename
    */
    const string& RawInputFilename() const { return raw_input_filename; }

    /** Sets the raw input filename from the current pattern.
	\param f the feature name, eg. rgb, replaced into \%f in the pattern
	\param i the input file name, replaced into \%i
    */
    void SetRawInputFilename(const string&, const string&);

    /// opens output stream, file or cout depending on options
    bool OpenOutputFile(pair<string,ofstream>&, bool&);

    /// closes  output stream, does nothing if this is cout
    void CloseFile() {
      /// currently does nothing...
    }

    /** Returns the standard file naming pattern:
	features/<segm.method>_<feature>.dat
	\return the standard file naming pattern
    */
    static string StandardFilenamePattern(int t, bool r);

    /** Sets the internal variable used to store the segmentation method
	abbreviation (to be used for label generation)
    */
    void SetSegmentationMethodString(const string & m) { segmethod = m; }

    /** A utility function.
    */
    vector<string> SplitFilename(const string&) const;

    /** A utility function.
    */
    static string StripFilename(const string&);

    /** Returns directory where temporary files could be placed.
     */
    virtual const string& TempDir() const { return tempdir; }

    /** Default implementation for the one above.
     */
    void SetTempDir(bool);

    /** Explicit setting.
     */
    static void SetTempDir(const string& d) { static_tempdir = d; }

    /** Creates a name for a temporary file.
     */
    pair<bool,string> TempFile(const string& = "");

    /** Unlinks a file.
     */
    bool Unlink(const string& n) { return !unlink(n.c_str()); }

    /** Creates directory for given file if it doesn't exist.
     */
    static bool EnsureDirectoryExists(const string& file);

    /** Converts image to greyscale.
     */
    string ImageToPGM();

    /** Converts image to greyscale and stores it in a file.
     */
    bool ImageToPGM(const string&);

    /** Adds one feature vector to another
	\param v1 the feature vector that will be added to
	\param v2 the feature vector that is added to v1
    */
    static void AddToFeatureVector(featureVector& v1,
				   const featureVector& v2,
				   bool stric = false,
				   float mult = 1.0) {
      if (stric && v1.size()!=v2.size())
	throw string("Feature::AddToFeatureVector() : sizes differ");

      for (size_t d=0; d<v1.size() && d<v2.size(); d++)
	v1[d] += v2[d]*mult;
    }

    /** Adds one feature vector to another using maximum norm
	\param v1 the feature vector that will be added to
	\param v2 the feature vector that is added to v1
    */
    static void AddToFeatureVectorMax(featureVector& v1,
				      const featureVector& v2,
				      bool stric = false) {
      if (stric && v1.size()!=v2.size())
	throw string("Feature::AddToFeatureVector() : sizes differ");

      for (size_t d=0; d<v1.size() && d<v2.size(); d++)
	if (v1[d] < v2[d])
	  v1[d] = v2[d];
    }

    /** Divides feature vector by a scalar value
	\param v the feature vector to be divided
	\param div the divisor
    */
    static void DivideFeatureVector(featureVector& v, float div) {
      for (size_t d=0; d<v.size(); d++)
	if (div==0)
	  v[d] = 0;
	else
	  v[d] /= div;
    }

    /** Multiplies feature vector by a scalar value
	\param v the feature vector to be multiplied
	\param m scalar multiplier
    */
    static void MultiplyFeatureVector(featureVector& v, float m) {
      for (size_t d=0; d<v.size(); d++)
	v[d] *= m;
    }

    /** calculates the square of the euclidean distance between two vectors
	\param v1 a feature vector  
	\param v2 a feature vector  
	\param start first component to include, default 0 
	\param stop  last component to include, default: -1 -> last 
	\param maxDist termination limit, default: -1 -> no limit  
   
	\return the square distance between v1 and v2
    */
    static float VectorSqrDistance(const featureVector &v1,
				   const featureVector &v2,
				   int start=0,int stop=-1,float maxDist=-1) {
      if(v1.size() != v2.size()) return -1;

      if(stop==-1) stop=v1.size()-1;

      float dist=0;
      float diff;
      for(int dim=start;dim<=stop;dim++){
	diff=v1[dim]-v2[dim];
	dist += diff*diff;

	if(maxDist>0&&dist>=maxDist)
	  return dist;
      }
      
      return dist;
    }

    /** calculates the square of the euclidean length of the given vector
	\param v a feature vector  
	\return the square of the length of v
    */
    static float VectorSqrLength(const featureVector& v, 
				 int start = 0, int stop = -1) {
      if (stop==-1)
	stop = v.size()-1;
		
      float dist = 0;
      for (int dim=start; dim<=stop; dim++)
	dist += v[dim]*v[dim];
		
      return dist;
    }

    /** calculates the inner product of two vectors
    */
    static float VectorDot(const featureVector& v1,
			   const featureVector& v2){
      if (v1.size()!=v2.size())
	throw string("Feature::VectorDot() : sizes differ");
      
      float ret=0;

      for(size_t i=0;i<v1.size(); i++)
	ret += v1[i]*v2[i];

      return ret;
    }

    static void NormaliseFeatureVector(featureVector& v){
      float len = sqrt(VectorSqrLength(v));
      DivideFeatureVector(v,len);
    }

    //
    static void printFeatureVector(ostream& in, const featureVector& v) {
      bool first = true;
      for (featureVector::const_iterator it=v.begin(); it!=v.end(); it++) {
	if (first)
	  first = false;
	else 
	  in << " ";
	in << *it;
      }
    }

    static featureVector concatFeatureVectors(const featureVector& v1,
					      const featureVector& v2) {
      featureVector ret=v1;
      for (size_t d=0; d<v2.size(); d++)
	ret.push_back(v2[d]);
      return ret;
    }

    static bool vectorsSame(const vector<int>& v1, const vector<int>& v2){
      size_t s1 = v1.size();

      if (s1!=v2.size())
	return false;

      for (size_t d=0; d<s1; d++)
	if (v1[d]!=v2[d])
	  return false;

      return true;
    }

    ///
    int *getBorderSuppressionMask(int borderexpand);

    preprocessResult_SobelThresh *getSobelThresh(int *suppressionmask=NULL);

    // the external interface

    /// Destructor
    virtual ~Feature();

    /** This function is called when the labeling has been altered and
	all the features need to be all recalculated.
    */
    bool refreshLabeling();

    /// Types of normalization for histograms.
    enum hist_norm_type {
      hist_norm_none,
      hist_norm_L1,
      hist_norm_L2,
      hist_norm_L1_capped, // each element capped to max
      hist_norm_L2_capped  // 0.2 and then renormalized
    };

    /** Returns the resulting feature vector of a segment
	\param idx the data index of the segment
	\return the resulting feature vector of segment idx
    */
    featureVector ResultVector(int idx, bool azc, hist_norm_type norm) const {
      if (idx<0 || idx>=(int)data.size())
	throw "Feature::ResultVector() : segment label index out of bounds";

      featureVector res = data[idx]->ResultOrHist(azc, norm);
      if (!res.size())
	throw "Feature::ResultVector() : empty feature data element";

      return res;
    }

    featureVector ResultVector(int idx, bool azc, bool norm) const {
      return ResultVector(idx, azc, (norm ? hist_norm_L1 : hist_norm_none));
    }

    /** Concatenates all the feature vectors in one frame
	\return a vector with all the feature vectors in one frame concatenated
    */
    featureVector AggregatePerFrameVector() const;

    /** Returns a random feature vector
	\return a random feature vector
    */
    virtual featureVector getRandomFeatureVector() const = 0;

    /** Returns the total length of all the data vectors
	\param weak true if value zero can be returned
	\return the total length of all the data vectors
    */
    virtual int FeatureVectorSize(bool = false) const;

    /** Returns the total length of data vectors per frame
	\return the total length of data vectors per frame
    */
    virtual int PerFrameFeatureVectorSize() const {
      int l = GetData(0)->Length();

      switch (WithinFrameTreatment()) {
      case treat_concatenate:
	return DataCount()*l;

      case treat_separate:
	return l;

      default:
	return 0;
      }
    }

    /** Returns the number of frames if between-frame treatment is set
	to concatenating pixels, otherwise it returns one. jl?
	\return the concatenation count per pixel
    */
    int ConcatCountPerPixel() const {
      return BetweenFrameTreatment()==treat_pixelconcat ? Nframes() : 1;
    }

    /** Returns the number data elements if within-frame treatment is
	set to concatenate, otherwise it returns one. jl?  
	\return the concatenation count per frame  
    */
    int ConcatCountPerFrame() const {
      return WithinFrameTreatment()==treat_concatenate? DataElementCount() : 1;
    }

    /** Returns the concatenation count per frame times the number of frames.
	\return total concatenation count
    */
    int ConcatCountTotal() const {
      EnsureImage();

      if (BetweenFrameTreatment()==treat_concatenate) {
	return ConcatCountPerFrame()*Nframes();
      }

      return ConcatCountPerFrame();
    }

    /** Prints a list of the registered features
	\param oStream the output stream to print to (default cout)
	\param wide set to true if wide output is prefered
    */
    static void PrintFeatureList(ostream& = cout, bool = false);

    /** Prints a list of the registered features in XML form
	\param oStream the output stream to print to (default cout)
    */
    static void PrintFeatureListXML(ostream*, feature_result*);

    /** Sets the enable flag and additional data for PrintDescription()
	\param print true for printing
	\param all true for more verbose version including version, date, etc.
	\param cmdline goes in XML as <cmdline>
    */
    void SetDescriptionData(bool print, bool all, const string& cmdline) {
      description_print   = print;
      description_all     = all;
      description_cmdline = cmdline;
    }

    /** Prints the description of the feature
	\param os output stream to print to (default cout)
	\param sep prefix text written before each XML line
	\param weak if eg. feature vector size does not need to solved
    */
    bool PrintDescription(ostream& = cout, const string& = "# ",
			  bool = false) const;

    /** Forms the XML description of the feature
     */  
    XmlDom FormDescription(bool = false) const;

    /** Forms description for each feature vector component.
     */
    bool FormComponentDescriptions(XmlDom&) const;

    /** Adds extra feature specific XML description
	\param root the root XML node to add description to
	\param ns XML namespace
    */ 
    virtual void AddExtraDescription(XmlDom&) const {}

    /** Prints XML formed description of feature specified by name
	to specified stream which defaults cout
    */
    static bool PrintFeatureDescription(const string& name, ostream& = cout);

    /** Sets whether vector length should be printed or not
	\param p true for printing
    */
    void SetPrintVectorLength(bool p) { print_vector_length = p; }

    /** Sets whether vector length should be checked against 
	class variable vector_length when printing
	\param c true for checking
    */
    void SetCheckVectorLength(bool c) { check_vector_length = c; }

    /** Sets the variable against which the lengths of the printed 
	feature vectors are possibly checked
	\param l the vector length
    */
    void SetVectorLength(int l) { feature_vector_length = l; }

    /** Gets the variable that indicates 
	the feature vector length
    */

    int GetVectorLength() const { return feature_vector_length; }

    /** Prints the lenght of the feature vector and returns it
	\param s the output stream to print to (default cout)
	\return vector length
    */
    int PrintVectorLength(ostream& s = cout) const {
      //    feature_vector_length=FeatureVectorSize();
      int l = FeatureVectorSize();
      s << l << (IsSparseVector() ? " sparse" : "") << endl;
      return l;
    }

    void SetVectorLengthFake(int l) { feature_vector_length_fake = l; }
    int GetVectorLengthFake() const { return feature_vector_length_fake; }

    /** Print the header, ie. XML + vector length
	\param s the output stream to print to
    */
    void PrintHeader(ostream& s);

    /** Print a string to output stream if set
	\param s the string
    */
    void Print(const string& s) {
      if (fout_xx)
	*fout_xx << s;
    }

    /** Print a feature vector to output stream if set
	\param vec the feature vector to print
	\param lbl the label
    */
    void Print(const featureVector& vec, const string& lbl) {
      if (fout_xx)
	Print(*fout_xx, vec, lbl);
      else
	AppendResult(vec, lbl);
    }

    /** Print a feature vector
	\param s the output stream to print to
	\param vec the feature vector to print
	\param lbl the label
    */
    void Print(ostream& s, const featureVector& vec, const string& lbl);

    /** Print a set of feature vectors by splitting within frame
	\param vec the feature vector to print
	\param ss the slice label
    */
    void SplitAndPrint(const featureVector& vec, int ss) {
      if (fout_xx)
	SplitAndPrint(*fout_xx, vec, ss);
    }

    /** Print a set of feature vectors by splitting within frame
	\param s the output stream to print to
	\param vec the feature vector to print
	\param ss the slice label
    */
    void SplitAndPrint(ostream& s, const featureVector& vec, int ss);

    /// not used, jl?
    void translateBackground();

    /// jl?
    void includeZoneZero(bool i=true){include_zone_zero=i;}

    /** Returns a list of the feature names
	\return a list of feature names
    */
    virtual list<string> Models() const { 
      list<string> mlist;
      mlist.push_back(Name());
      return mlist;
    }

    virtual list<string> listNames() const { 
      list<string> l;
      l.push_back(Name());
      return l;
    }

    virtual list<string> listLongNames() const { 
      list<string> l;
      l.push_back(LongName());
      return l;
    }

    virtual list<string> listShortTexts() const { 
      list<string> l;
      l.push_back(ShortText());
      return l;
    }

    virtual list<string> listTargetTypes() const { 
      list<string> l;
      l.push_back(TargetType());
      return l;
    }

    virtual list<string> listNeededSegmentations() const { 
      list<string> l;
      l.push_back(NeededSegmentation());
      return l;
    }

    /** Returns the name of the feature
	\return feature short name
    */
    virtual string Name() const = 0;

    /** Returns the name of the feature even faked
	\return feature fake name or short name
    */
    string NameFaked() const {
      return fake_name!="" ? fake_name : Name();
    }

    /** Sets fake name
    */
    void SetFakeName(const string& n) { fake_name = n; }

    /** Sets explicit outdir to be used with %b and %r
    */
    void SetOutdir(const string& s) { outdir = s; }

    /** Sets subdir to be ignored from pattern %r
    */
    void SetSubdir(const string& s) { subdir = s; }

    /** Returns a longer name of the feature
	\return feature long name
    */
    virtual string LongName() const = 0;

    /** Returns a description of the feature
	\return short feature description
    */
    virtual string ShortText() const = 0;

    /** Returns the version number of the feature
	\return feature version
    */
    virtual string Version() const = 0;

    /** Returns PicSOM's target_type of the feature in string
	\return target type like "image" or "video"
    */
    virtual string TargetType() const = 0;

    /** Returns segmentation results needed for using the feature extractor
	\return string like "face.box"
    */
    virtual string NeededSegmentation() const { return ""; } // should be = 0;

    /// Zeros the internal storage
    virtual void ZeroPerFrameData() {
      if (data.empty())
	throw "Feature::ZeroPerFrameData() data structure not initialized yet";

      if (FrameVerbose())
	cout << "Feature::ZeroPerFrameData()" << endl;

      for (vector<Data*>::iterator i=data.begin(); i<data.end(); i++)
	(*i)->Zero();
    }

    /** Returns the current number of digits that is displayed for
	floating-point variables.  
	\returns the current precision
    */
    virtual int Precision() const { return precision; }

    /** Sets the current number of digits that is displayed for
	floating-point variables.
	\param p the new precision
    */
    virtual void Precision(int p) { precision = p; }

    /** Returns the width of the image
	\param fast true to skip check
	\return image width
    */
    int Width(bool fast = false)  const {
      if (!fast)
	EnsureImage();
      return CurrentFrame().width();
    }

    /** Returns the height of the image
	\param fast true to skip check
	\return image height
    */
    int Height(bool fast = false) const {
      if (!fast)
	EnsureImage();
      return CurrentFrame().height();
    }

    /** Returns the number of image frames
	\return number of frames
    */
    virtual int Nslices() const {
      return slice_bounds.size() ? slice_bounds.size() : 1;
    }

    /** Returns the lower and upper bounds (incluseive) of image frames
	\return lower and upper bound
    */
    virtual pair<size_t,size_t> FrameRange(size_t s) const {
      if (slice_bounds.empty() && !s)
	return EffectiveFrameRange();

      if (s<slice_bounds.size())
	return slice_bounds[s];

      stringstream ss;
      ss << "Feature::FrameRange(" << s << ") failed with slice_bounds.size()="
	 << slice_bounds.size();
      throw ss.str();
    }

    /** Returns the number of image frames
	\return number of frames
    */
    virtual int Nframes() const { 
      if (incore_imagedata_ptr)
	return 1;
      
      return 
	IsVideoFeature() ? videofile_ptr->get_num_frames() :
	IsAudioFeature() ? deprecated_audiofile_ptr->getDuration() : 
	SegmentData()->getNumFrames(); 
    }

    /** Returns the range of image frames when margins have been removed
	\return start and end frame indices, both inclusive
    */
    pair<size_t,size_t> EffectiveFrameRange() const;

    /** Returns the current frame
	\return the current frame
    */
    const picsom::imagedata& CurrentFrame() const {
      return *SegmentData()->accessFrame();
    }

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
    /** Returns the current frame in OpenCV object
	\return the current frame
    */
    const cv::Mat& CurrentFrameOCVoriginal() const {
      if (!opencvptr)
	throw string("CurrentFrameOCV() called with opencvptr==NULL");
      return *(cv::Mat*)opencvptr;
    }

    /** Returns the current frame in OpenCV object
	\return the current frame
    */
    const cv::Mat& CurrentFrameOCV() const {
      return opencvmat;
    }

    /** Converts picsom::imagedata to cv::Mat
     */
    static cv::Mat imagedata_to_cvMat(const imagedata&);

    /** Converts picsom::imagedata to cv::Mat
     */
    cv::Mat imagedata_to_cvMat() const;

    /** Converts cv::Mat to picsom::imagedata
     */
    static imagedata cvMat_to_imagedata(const cv::Mat&);

#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

    /** Returns availbale OpenCV version or an empty string
    */
    static const string& OCVversion() {
      static string v;
#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
      v = CV_VERSION;
#endif // HAVE_OPENCV2_CORE_CORE_HPP
      return v;
    }

    /** Returns the segment number at position (x,y)
	\param f the frame
	\param x the x coordinate
	\param y the y coordinate
	\return segment number
    */
    int SegmentScalar(int f, int x, int y) const {
      // cox::tictac::func tt(tics, "Feature::SegmentScalar");

      int s = -1;

      if (region_spec.empty()) {
	SegmentData()->get_pixel_segment(f, x, y, s);
	if (PixelVerbose())
	  cout << "Feature::SegmentScalar(" << f << ","
	       << x << "," << y << ") (no region_spec) -> " << s << endl;
	return s;
      }

      s = 0; // every pixel has segment index >= 0 !
      for (size_t i=0; i<region_spec.size(); i++)
	if (region_spec[i].first->contains(x, y)) { // f not used!
	  s = i+1; // FIRST_REGION_BASED_SEGMENT==1
	  break;
	}

      if (PixelVerbose())
	cout << "Feature::SegmentScalar(" << f << ","
	     << x << "," << y << ") (region_specs) -> " << s << endl;
    
      return s;
    }

    /** Returns vector of segment numbers at position (x,y)
	\param f the frame
	\param x the x coordinate
	\param y the y coordinate
	\return segment number vector
    */
    vector<int> SegmentVector(int f, int x, int y) const;

    /// enum of different pixel types
    enum pixeltype {
      pixel_undef,
      pixel_rgb,
      pixel_rgba,
      pixel_grey,
      pixel_dim2, 
      pixel_dim4  
    };

    /** Returns the default pixel type
	\return default pixel type
    */
    virtual pixeltype DefaultPixelType() const = 0;

    /** Returns the current pixel type
	\return current pixel type
    */
    virtual pixeltype PixelType() const {
      return used_pixeltype ? used_pixeltype : DefaultPixelType();
    }

    /** Sets the pixeltype
	\param t the new pixel type
    */
    void PixelType(pixeltype t) { used_pixeltype = t; }

    /** Sets the pixeltype from string
	\param n pixeltype name
    */
    void PixelType(const string& n) { PixelType(PixelTypeName(n)); }

    /** Converts pixeltype to string
	\param t the pixeltype
	\return the name of the pixel type
    */
    static string PixelTypeName(pixeltype t) {
      switch (t) {
      case pixel_undef: return "undef";
      case pixel_rgb:   return "rgb";
      case pixel_rgba:  return "rgba";
      case pixel_grey:  return "grey";
      default: throw "Feature::PixelTypeName() unknown type";
      }
    }
  
    /** Converts string to pixeltype. The options are: rgb, rgba and grey.
	\param n the pixeltype name
	\return the pixeltype
    */
    static pixeltype PixelTypeName(const string& n) {
      if (n=="rgb")
	return pixel_rgb;

      else if (n=="rgba")
	return pixel_rgba;

      else if (n=="grey")
	return pixel_grey;

      else if (n=="2")
	return pixel_dim2;

      else if (n=="4")
	return pixel_dim4;

      return pixel_undef;
      // throw "Feature::PixelTypeName() unknown type name";
    }
  
    /** Returns the byte size of the different pixeltypes
	\param pixeltype
	\return byte size
    */
    static int PixelTypeSize(pixeltype t) {
      switch (t) {
      case pixel_rgb:  return 3;
      case pixel_rgba: return 4;
      case pixel_grey: return 1;
      case pixel_dim2: return 2;
      case pixel_dim4: return 4;
      default: throw "Feature::PixelTypeSize() unknown type";
      }
    }

    /** Returns the byte size of the used pixeltype
     */
    int PixelTypeSize() const { return PixelTypeSize(PixelType()); }

    /** Return name of the pixel channel
     */
    static const string& PixelChannelName(pixeltype t, size_t i) {
      static vector<string> s;
      if (s.empty()) {
	const char *ss[] = { "red", "green", "blue", "alpha", "grey" };
	s.assign(ss, ss+sizeof(ss)/sizeof(*ss));
      }
      if (i>=(size_t)PixelTypeSize(t))
	throw "Feature::PixelChannelName() index error";

      return s[t==pixel_grey ? 4 : i];
    }

    /** Return name of the pixel channel
     */
    const string& PixelChannelName(size_t i) const {
      return PixelChannelName(PixelType(), i);
    }

    /** Set per-pixel multiplier.
     */
    void PixelMultiply(float v) { pixel_multiply = v; }

    /** Return per-pixel multiplier.
     */
    float PixelMultiply() const { return pixel_multiply; }

    /** Get image pixel as a vector of integers, jl?
	\param x x coordinate of the pixel
	\param y y coordinate of the pixel
	\param t pixel type
	\return colour components of the pixel at (x,y)
    */
    vector<int> GetPixelInts(int x, int y, pixeltype);

    /** Get image pixel as a vector of floats, jl?
	\param x x coordinate of the pixel
	\param y y coordinate of the pixel
	\param t pixel type
	\return colour components of the pixel at (x,y)
    */
    vector<float> GetPixelFloats(int x, int y, pixeltype t);

    /** Checks if we have images loaded
	\return true if images are loaded
    */
    bool HasImage() const {
      // cout << "Feature::HasImage()" << endl;
      return SegmentData()->hasImages();
    }

    /** Checks if we have segments loaded
	\return true if segments are loaded
    */
    bool HasSegments() const {
      // cout << "Feature::HasSegments()" << endl;
      return SegmentData()->hasSegments();
    }

    /// Aborts if we don't have images loaded
    void EnsureImage() const; // not really const anymore

    /// Throws exception if we don't have segments loaded
    void EnsureSegmentation() const {
      if (!IsImageFeature())
	return; // obs! Do we need segmentation in non-image features?
      if (!HasSegments()) throw "Feature : no segments"; 
    }

    /** Parse command-line options
	\param argc the number of arguments
	\param argv a list of arguments
    */
    virtual int parseOption(int, const char**);

    /** Returns a option-dependent suffix string for output file names.
     */
    virtual string OptionSuffix() const { return ""; }

    /** Print instructions for all or one feature extraction method
	\param str output stream to print to (default cout)
    */
    static void PrintInstructions(ostream& = cout, const string& = "");

    /** Print instructions for this feature extraction method
	\param str output stream to print to (default cout)
    */
    void printMethodInstructions(ostream& = cout) const;

    /** Prints any specific options for this method 
	\param str output stream to print to (default cout)
    */
    virtual int printMethodOptions(ostream& = cout) const;

    /** Returns feature by method name
	\param n method name
	\return a pointer to a feature jl?
    */
    static const Feature *FindMethod(const string& n) {
      const Feature *found = NULL;
      for (const Feature *p = list_of_methods; p; p = p->next_of_methods) {
	const list<string> ml = p->Models();
	for (list<string>::const_iterator m = ml.begin(); m!=ml.end(); m++)
	  if (*m==n) {
	    if (found)
	      throw string("more than one method matches \"")+n+"\"";
	    else
	      found = p;
          }
      }
      return found;
    }

    /** Finds a feature extraction method by name and creates a new
	instance of it.
	\param n method name
	\return a new feature object of the given type jl?
    */
    static Feature *FindAndCreateMethod(const string& n) {
      const Feature *p = FindMethod(n);
      if (!p)
	throw string("feature extraction method \"")+n+"\" not found";

      return p->Create(n);
    }

    /** Finds a feature extraction method by name and creates a new
	instance of it. The new object is also initialised with the
	given options.
	\param n method name
	\param img initialise to this image
	\param opt list of options to initialise to
	\return a new feature object of the given type jl?
    */
    static Feature *CreateMethod(const string& n, const string& img = "",
				 const list<string>& opt = (list<string>)0,
				 bool allocate_data = false) {
      return FindAndCreateMethod(n)->Initialize(img, "", NULL, opt,
						allocate_data);
    }

    /** Finds a feature extraction method by name and creates a new
	instance of it. The new object is also initialised with the
	given options.
	\param n method name
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
	\return a new feature object of the given type, jl?
    */
    static Feature *CreateMethod(const string& n, const string& img,
				 const string& seg,
				 const list<string>& opt = (list<string>)0,
				 bool allocate_data = false) {
      return FindAndCreateMethod(n)->Initialize(img, seg, NULL, 
						opt, allocate_data);
    }

    static Feature *CreateMethod(const string& n, segmentfile *s,
				 const list<string>& opt = (list<string>)0, 
				 bool allocate_data = false) {
      return FindAndCreateMethod(n)->Initialize("", "", s, opt, allocate_data);
    }

    /// jl
    virtual bool Incremental() const { return false; }

    /**
     */
    typedef list<pair<string,vector<pair<string,string> > > > pp_chain_t;

    /// jl
    virtual bool PreProcess(const string&, imagedata&, const pp_chain_t&,
			    const string&, int) const;

    /// jl
    bool FramePreProcess(imagedata& img, int f) const {
      return PreProcess("frame", img, frame_preprocess_method,
			frame_preprocess_in_use, f);
    }

    /// jl
    bool RegionPreProcess(imagedata& img, int f) const {
      return PreProcess("region", img, region_preprocess_method, "", f);
    }

    /// jl
    imagedata RegionImage(size_t) const;
    
    /** Performs feature calculation for one label
	\param frame the frame to calculate feature from
	\param label the segment label to calculate feature from
	\return true if ok
    */  
    virtual bool CalculateOneLabel(int frame, int label) = 0;

    /** Performs feature calculation for one frame
	\param frame the frame to calculate feature from
	\return true if ok
    */
    virtual bool CalculateOneFrame(int frame) = 0;

    /** Performs feature calculation for one video/audio slice
	\param fbegin the frame to start calculation from
	\param fend the last frame to calculate, inclusive
	\return true if ok
    */
    virtual bool CalculateOneSlice(featureVector&,
				   int /*fbegin*/,
				   int /*fend*/,
				   bool);
    /**
       zzz
    */
    const string& VideoSegmentExtension() const {
      return video_segment_extension;
    }

    /**
       zzz
    */
    void VideoSegmentExtension(const string& e) {
      video_segment_extension = e;
    }

    /**
       Performs feature calculation for one video file
    */ 
    virtual featureVector CalculateVideoSegment(bool print) {
      return CalculatePerFrame(0, -1, print); // default simple implementation
    }

    /**
       Performs feature calculation for one audio file
    */ 
    virtual featureVector CalculateAudioSegment(bool print) {
      return CalculatePerFrame(0, -1, print); // default simple implementation
    }


    /** Performs preprocessing and calls CalculateOneFrame after that
	\param f the frame to process
	\param g frame number equal to f or -1 to omit it
	\param p true to print instead of aggregate result
	\return aggregated result or empty vector
    */
    virtual featureVector CalculatePerFrame(int, int, bool);

    /** This is the outmost loop and meaningfull only for videos
	\param s the slice to process
	\param p true to print instead of aggregate result
	\return aggregated result or empty vector
    */
    virtual featureVector CalculatePerSlice(int, bool);

    /** A function to join per frame results to one slice's results.
     */
    bool CombinePerFrameToOneSlice(featureVector&, int, int, bool);

    ///
    const string ExtractVideoSegment(int fbegin, int fend,
				     const string& ext);

    /** Inner method for video segment extraction. Override for
	feature specific parameters to videofile-functionality.
	\param tp time-point from where to start
	\param dur duration of segment
	\return resulting temporary file
     */
    virtual string ExtractVideoSegmentInner(double tp, double dur,
					    const string& ext);

    ///
    void RemoveVideoSegment() { 
      if (!videosegment_file.empty() && !KeepTmp()) 
	Unlink(videosegment_file); 
    }

    /** May be overriden to return false if normal feature vectors
	are not desired as output.
    */
    virtual bool PrintingEnabled() const { return true; }

    ///
    virtual featureVector CalculateRegion(const Region &r) = 0;

    ///
    virtual bool moveRegion(const Region &r, int from, int to);

    /** Performs feature calculation for each preprocessing methods
	\return true if ok
    */
    virtual bool CalculateAndPrintAllPreProcessings();

    /** Performs feature calculation for each frame and prints the result
	\return true if ok
    */
    virtual bool CalculateAndPrint();

    /** Returns true if this feature has scaling
	\return true if we use scaling
    */
    bool HasScaling() const { return !scaling.empty(); }

    /** Resolves parameters needed for rotating and scaling
	\return true if ok
    */
    bool ResolveRotating(const string&);

    /** Resolves parameters needed for rotating and scaling.
	Called byt the above one and ExtractSegment().
	\return true if ok
    */
    static bool ResolveRotating(const imagedata&, const segmentfile&,
				const string&, scalinginfo&, scalinginfo&,
				string&);

    /** Resolves parameters needed for scaling, jl?
	\return true if ok
    */
    bool ResolveScaling(const string&);

    /** Performs the actual rotating
	\return true if ok
    */
    bool PerformRotating(int f);

    /** Performs the actual scaling
	\return true if ok
    */
    bool PerformScaling(int);

    /// jl
    void ScaleForwards(int& x, int& y) const { scaleinfo.forwards(x, y); }

    /// jl
    void TranslateByScaling(rectangularRegion & ri) const {
      int x0, y0;
      ri.x0y0(x0, y0);
      int x = x0, y = y0;
      ScaleForwards(x, y);
      ri.translate(x-x0, y-y0);
      if (FrameVerbose())
	cout << "Feature::TranslateByScaling() : translated ("
	     << x0 << "," << y0 << ") -> (" << x << "," << y << ")" << endl;
    }

    scalinginfo GetScalinginfo() const { return scaleinfo; } 

    /**
     */
    bool SetPreProcessing(pp_chain_t&, const string&);

    /**
     */
    bool SetFramePreProcessing(const string& s) {
      return SetPreProcessing(frame_preprocess_method, s);
    }

    /**
     */
    bool SetRegionPreProcessing(const string& s) {
      return SetPreProcessing(region_preprocess_method, s);
    }

    /**
     */
    string  ExpandPreProcessingAlias(const string&) const;

    /** Runs the named pre-process method
	\param m method name
	\param f frame number jl?
    */
    bool PreProcessMethod(imagedata&, const string&, const string&, int) const;

    /// Setup method for pre-processing 
    static void SetupPreProcessMethods();

    /** Example of a preprocessing method
	\return true if ok
    */
    bool PreProcess_none(imagedata&, const string&) const;

    /** Example of a preprocessing method
	\return true if ok
    */
    bool PreProcess_multiply(imagedata&, const string&) const;

    /** Example of a preprocessing method
	\return true if ok
    */
    bool PreProcess_shift(imagedata&, const string&) const;

    /** Example of a preprocessing method
	\return true if ok
    */
    bool PreProcess_crop(imagedata&, const string&) const;

    /** Example of a preprocessing method
	\return true if ok
    */
    bool PreProcess_trim(imagedata&, const string&) const;

    /** Example of a preprocessing method
	\return true if ok
    */
    bool PreProcess_resize(imagedata&, const string&) const;

    /** Example of a preprocessing method
	\return true if ok
    */
    bool PreProcess_hflip(imagedata&, const string&) const;

    /** Reads mean&stdev vectors for z-normaliztion from a COD file
     */
    bool SetZnormalize(const string&);

    /** Does normalization of raw values.
     */
    vector<float> DoNormalize(const vector<float>&) const;

    /** Reads in xml from DAT/COD files.
     */
    XmlDom ReadXML(const string&) const;

    /** Returns the used zoning, if it is not set it returns the default
	zoning text
	\return used zoning
    */
    const string& UsedZoningText() const {
      static const string empty = "1";
      return IsRawOutMode() ? empty :
	used_zoning!="" ? used_zoning : DefaultZoningText();
    }

    /** Sets the used zoning
	\param t the used zoning to set
    */
    void UsedZoningText(const string& t) { used_zoning = t; }

    /** Returns the default zoning type
	\return default zoning type
    */
    virtual const string& DefaultZoningText() const {
      static string flat("1");
      return flat;
    }

    /** Returns the used slicing, if it is not set it returns the default
	slicing text
	\return used slicing
    */
    const string& UsedSlicingText() const {
      static const string empty = "1";
      return IsRawOutMode() ? empty :
	used_slicing!="" ? used_slicing : DefaultSlicingText();
    }

    /** Sets the used slicing
	\param t the used slicing to set
    */
    void UsedSlicingText(const string& t) { used_slicing = t; }

    /** Returns the default slicing type
	\return default slicing type
    */
    virtual const string& DefaultSlicingText() const {
      static string flat("1");
      return flat;
    }

    /// treatment types are used to specify the treatment within/between 
    /// frames and slices
    enum treat_type {
      treat_undef,           // 
      treat_separate,        // 
      treat_concatenate,     //
      treat_pixelconcat,     //
      treat_average,         //
      treat_join,            //
      treat_diffseparate,    //
      treat_diffconcat
    };

    /** Converts from string to treat_type
	\param s name of the treatment type
	\return the corresponding treat_type
    */
    static treat_type TreatmentType(const string& s) {
      return TreatmentType(s[0]);
    }

    /** Converts from char to treat_type
	\param t the letter of the treatment type
	\return the corresponding treat_type
    */
    static treat_type TreatmentType(char t) {
      switch (t) {
      case 's': return treat_separate;
      case 'c': return treat_concatenate;
      case 'p': return treat_pixelconcat;
      case 'a': return treat_average;
      case 'j': return treat_join;
      case 'x': return treat_diffseparate;
      case 'd': return treat_diffconcat;
      default : return treat_undef;
      }
    }

    /** Returns the default treatment within frames
	\return default treatment within frames
    */
    virtual treat_type DefaultWithinFrameTreatment() const = 0;

    /** Returns the default treatment between frames
	\return default treatment between frames
    */
    virtual treat_type DefaultBetweenFrameTreatment() const = 0;

    /** Returns the default treatment between slices
	\return default treatment between slices
    */
    virtual treat_type DefaultBetweenSliceTreatment() const = 0;

    /** Returns the current within frames treatment
	\return current within frames treatment
    */
    treat_type WithinFrameTreatment() const {
      return treat_within_frame ? treat_within_frame :
	DefaultWithinFrameTreatment();
    }

    /** Returns the current between frames treatment
	\return current between frames treatment
    */
    treat_type BetweenFrameTreatment() const {
      return treat_between_frame ? treat_between_frame :
	DefaultBetweenFrameTreatment();
    }

    /** Returns the current between slices treatment
	\return current between slices treatment
    */
    treat_type BetweenSliceTreatment() const {
      return treat_between_slice ? treat_between_slice :
	DefaultBetweenSliceTreatment();
    }

    /** Sets the current within frames treatment
	\param t the new within frames treatment
    */
    void WithinFrameTreatment(treat_type t) { treat_within_frame = t; }

    /** Sets the current between frames treatment
	\param t the new between frames treatment
    */
    void BetweenFrameTreatment(treat_type t) { treat_between_frame = t; }

    /** Sets the current between slices treatment
	\param t the new between slices treatment
    */
    void BetweenSliceTreatment(treat_type t) { treat_between_slice = t; }

    /** Returns the name of the current within frames treatment
	\return name of the current within frames treatment
    */
    const string& WithinFrameTreatmentName() const {
      return TreatmentName(WithinFrameTreatment()); }

    /** Returns the name of the current between frames treatment
	\return name of the current between frames treatment
    */
    const string& BetweenFrameTreatmentName() const {
      return TreatmentName(BetweenFrameTreatment()); }

    /** Returns the name of the current between slices treatment
	\return name of the current between slices treatment
    */
    const string& BetweenSliceTreatmentName() const {
      return TreatmentName(BetweenSliceTreatment()); }

    /** Converts treat_type to long name string
	\param t treatment type to convert
	\return treatment type name
    */
    static const string& TreatmentName(treat_type t) {
      static const string un = "undef", se = "separate", co = "concatenate",
	pi = "pixelconcat", av = "average", jo = "join", dx = "diffseparate",
	di = "diffconcat", err = "???";

      switch (t) {
      case treat_undef        : return un;
      case treat_separate     : return se;
      case treat_concatenate  : return co;
      case treat_pixelconcat  : return pi;
      case treat_average      : return av;
      case treat_join         : return jo;
      case treat_diffseparate : return dx;
      case treat_diffconcat   : return di;
      default                 : return err;
      }
    }

    /** Returns short name of the current segmentation method
	\return short name of the current segmentation method
    */
    virtual string GetSegmentationMethodName() const {
      if (!IsImageFeature()) 
	return ""; // obs! Do we need segmentation for non-image features?

      if (segmethod!="")
	return segmethod;

      string m = SegmentData()->collectChainedName();
      if (m!="")
	return m;

      string mname = "sl"+UsedSlicingText()+"+";
      if (region_spec.empty())
	return mname+"zo"+UsedZoningText();
    
      return mname+"rr";  // "rr" ~= "RectangularRegion"
    }

    /** Returns the pattern string that is used to create the label
	Where %m is the segmentation method name and
	%i is the object's label name
	\return current label pattern 
    */
    const string GetLabelPattern() const { return label_pattern; }

    /** Sets the label pattern
	\param p new label pattern
    */
    void SetLabelPattern(const string& p) { label_pattern = p; }

    /** The default or standard label pattern.
     */
    static const string& StandardLabelPattern() { 
      static const string p = "%m:%i%*p";
      return p;
    }

    /** Sets the label pattern to the standard "%m:%i"
	Where %m is the segmentation method name and
	%i is the the feature used 
    */
    void SetStandardLabelPattern() {
      SetLabelPattern(StandardLabelPattern());
    }

    /** Sets forced label.
    */
    void ForcedLabel(const string& s) { forced_label = s; }

    /** Returns forced label if any.
    */
    const string& ForcedLabel() const { return forced_label; }

    /** Forms a data label from the current pattern.
	\param slice number
	\param frame number
	\param segm  number as string
	\return data label
    */
    string Label(int slice, long frame, const string& segm) const {
      return Label(slice, frame, -1, segm);
    }

    /** Forms a data label from the current pattern.
	\param slice number
	\param frame1 frame range start
	\param frame2 frame range stop, inclusive
	\param segm  number as string
	\return data label
    */
    string Label(int slice, long frame1, long frame2,const string& segm) const;

    /** Forms a raw data label from the current pattern.
	\return data label
    */
    string RawLabel(int, int) const;

    /** Returns content of <framespec> tag if it exists.
     */
    string SolveFrameSpec(int) const;

    /** Returns the file name.
	\return file name
    */
    virtual string ObjectDataFileName() const;

    /** Returns the file name starting surely with /.
	\return file name
    */
    string ObjectFullDataFileName() const {
      return FullFileName(ObjectDataFileName());
    }

    /** Returns the full data file name to calculate next
	slice from. Normally the same as ObjectFullDataFileName()
	except when calculating for video segments.
	\return file name
    */
    string CurrentFullDataFileName() {
      if (IsVideoFeature() && !videosegment_file.empty())
        return videosegment_file;
      if (!intermediate_filename.empty())
        return intermediate_filename;
      return ObjectFullDataFileName();
    }

    /** Work horse for the above one
	\return file name
    */
    static string FullFileName(const string&);

    /** Returns the base file name, without the path
	\return base file name
    */
    string BaseFileName() const;

    /** 
     */
    static imagedata ExtractSegment(const string&, const string&,
				    const string&);

  protected:
    /// The default constructor  
    Feature();

    /// Constructor that adds reference to that instance in list_of_methods.  
    Feature(bool) {
      segmentfile_ptr = allocated_segmentfile = NULL; 
      videofile_ptr = NULL;
      next_of_methods = list_of_methods;
      list_of_methods = this;
    }

    /** Initialises the feature object with the given options.
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param s segmentation
	\param opt list of options to initialise to
	\return a pointer to itself
    */
    virtual Feature *Initialize(const string& img, const string& seg,
				segmentfile *s, const list<string>& opt,
				bool allocate_data = false);

    /** A function that serves to notify the user than an obsolete function
	has been called.
	\param f the name of the function
    */
    static void Obsoleted(const string& f) {
      cerr << endl << "Obsoleted " << f << "() called!" << endl << endl;
    }

    /** Opens the image and (if needed) the segmentation file
	\param img image file name
	\param seg segmentation file name (optional)
    */
    void OpenFiles(const string& img, const string& seg = "");

    /** Process command line options before reading data
	\param l list of the given options
	\return true if ok
    */
    virtual bool ProcessPreOptionsAndRemove(list<string>&);

    /** Process command line options after reading data
	\param l list of the given options
	\return true if ok
    */
    virtual bool ProcessOptionsAndRemove(list<string>&);

    /** Splits the option string around the "="-sign, ie key=value
	\param str the option string to split
	\param key the key-part will be assigned to this string
	\param val the value-part will be assigned to this string
    */
    static void SplitOptionString(const string& str, string& key, string& val){
      key = val = "";
      string::size_type eq = str.find('=');
      if (eq==string::npos)
	throw string("Feature::SplitOptionString() : = missing in [")+str+"]";
      key = string(str, 0, eq);
      val = string(str, eq+1);
    }

    /** Returns true if str can be considered affirmative, ie. y, yes, etc...
	\return true if affirmative
    */
    static bool IsAffirmative(const string& str) {
      return str=="y" || str=="yes" || str=="Y" || str=="YES" || str=="Yes"
	|| str=="t" || str=="true" || str=="T" || str=="TRUE" || str=="True"
	|| str=="1";
    }

    /** Returns true if str can be considered to be a boolean, ie y,n,yes,no...
	\return true if boolean
    */
    static bool IsBoolean(const string& str) {
      return IsAffirmative(str)
	|| str=="n" || str=="no" || str=="N" || str=="NO" || str=="No"
	|| str=="f" || str=="false" || str=="F" || str=="FALSE" || str=="False"
	|| str=="0";
    }

    // jl? is this used anymore??
    void processOptions(int argc, const char **argv);

    /// Base class for all the data objects needed in the feature extraction
    class Data {
    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      Data(pixeltype t, int n, const Feature *p) :
	type(t), exts(n), parent(p) {}

      /** Destructor
      */
      virtual ~Data() {}

      /** Returns the pixel type used
	  \return current pixel type
      */
      pixeltype PixelType() const { return type; }

      /** Returns the byte size of the pixel type
	  \return byte size of current pixel type
      */
      int PixelTypeSize() const { return Feature::PixelTypeSize(type); }

      /** Returns the extension
	  \return extension
      */
      int Extension() const { return exts; }

      /** Returns the data unit size (ie (pixel type size)*extension
	  \return data unit size
      */
      virtual int DataUnitSize() const { return exts*PixelTypeSize(); }

      /// Zeros the internal storage
      virtual void Zero() { hist_values.clear(); }

      virtual void ZeroToSize(int /*n*/) {
	Zero();
      }

      /** Returns the name of the feature
	  \return feature name
      */
      virtual string Name() const = 0;

      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      virtual int Length() const = 0;

      /** Returns the lenght of the data contained in the object
	  \return data length
      */
      size_t VectorOrHistogramLength() const {
	return hist_values.size() ? hist_values.size() : (size_t)Length();
      }

      /// Implementation of the  += operator
      virtual Data& operator+=(const Data&) = 0;

      /** Returns the result of the feature extraction
	  \param azc true if zero count of pixels is OK
	  \return feature result vector
      */
      virtual featureVector Result(bool azc) const = 0;

      virtual featureVector ResultOrHist(bool azc, hist_norm_type norm) const {
	if (hist_values.empty())
	  return Result(azc);

	size_t sum = accumulate(hist_values.begin(), hist_values.end(), 0);
	size_t len = hist_values.size();
	featureVector ret(len, 0.0);
	if (sum) {
	  float mul = (norm == hist_norm_L1) ? 1.0/sum : 1.0;
	  for (size_t d = 0; d<len; d++)
	    ret[d] = mul*hist_values[d];
	}
	return ret;
      }

      featureVector ResultOrHist(bool azc, bool norm) const {
	return ResultOrHist(azc, norm ? hist_norm_L1 : hist_norm_none);
      }      

      /** Prints the result, ie the content of the data object
	  \param os the output stream to print to
	  \param azc true if zero count of pixels is OK
      */
      virtual void Print(ostream& os, bool azc) const {
	featureVector r = ResultOrHist(azc, true);
	for (size_t i=0; i<r.size(); i++)
	  os << (i?" ":"") << r[i];
      }

      ///
      const Feature *Parent() const { return parent; }

      ///
      Feature *Parent() { return (Feature*)parent; }

      ///
      void initHistogram();
    
      ///
      void addToHistogram(const featureVector &v, int = 1);

    protected:
      /// the internal pixel data type
      const pixeltype type;

      /// the extension
      const int exts;

      /// parent Feature object
      const Feature *parent; 

      ///
      vector<vector<float> >  hist_limits;

      ///
      vector<size_t> hist_values;

      /// 
      size_t vector2fixedhistbin(const featureVector &c) const;

    };  // class Feature::Data

    class VectorData : public Data {
      // wrapper for a feature vector that can be set 
      // and queried

    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      VectorData(pixeltype t, int n, const Feature *p) : Data(t, n, p) {
	len = 0;
	Zero();
      }

      /// Zeros the internal storage
      virtual void Zero() {
	fV = vector<float>(len, 0);
      }

      void setLength(size_t l) { len = l; }

      /// += operator (not used?)
      virtual Data& operator+=(const Data&) {
	throw "VectorData::operator+=() not defined";
      }

      virtual string Name() const { return "VectorData"; }

      virtual int Length() const { return fV.size(); }

      virtual featureVector Result(bool) const { return fV; }

      void setVector(const featureVector& v) { fV = v; }

    private:
      /// length of the vector can/should? be stored first
      size_t len;

      /// the vector itself
      featureVector fV;
    }; // class VectorData

    /** Creates a Data object of appropriate type
	\param t the pixel data type
	\param n the extension
	\param i the data index (when calculating for different segments)
	\return pointer to the data object
    */       
    virtual Feature::Data *CreateData(pixeltype, int, int) const = 0;

    virtual void deleteData(void *p)=0;
    // derived classes must redefine this in order to 
    // be able to delete temporary data objects

    /// class for reading .dat -files

    class dataset {
    public:
      ///
      void readDataset(const string &fn, string& descr);  

      ///
      bool writeDataset(const string &fn, const string& descr) const;  

      ///
      size_t dim;

      ///
      vector<vector<float> > vec;
    };
    

    /** Returns true if temporary files should be kept
	\return true if temporary files should be kept
    */
    bool KeepTmp() const { return keeptmp; }

    /// true if zero shall be used as an index
    // bool UseZeroIndex() const { return use_background||!ZeroIsBackground();}

    /// true if zero is to be considered background
    // virtual bool ZeroIsBackground() const = 0;

    /** Sets the background parameter, it should be true if we are to
	use the background segment in the calculations
	\param u the new background value
    */
    void UseBackground(bool u) { use_background = u; }

    /** Returns the current value of the background parameter. It is
	true if we are to use the background in the calculations, false
	otherwise.
	\return background value
    */
    bool UseBackground() const { return use_background; }

    void RegionHierarchy(bool h) { region_hierarchy=h; }

    bool RegionHierarchy() const { return region_hierarchy; }

    /** Returns the data index of a segment
	\param s segment
	\param no_throw true if return -1 instead of throw
	\return data index
    */
    int DataIndex(int s, bool no_throw = false) const {
      // cox::tictac::func tt(tics, "Feature::DataIndex");

      int_idx_map::const_iterator i = int2index.find(s);
      if (i==int2index.end()) {
	if (no_throw)
	  return -1;

	char tmp[100];
	sprintf(tmp, "%d", s);
	throw string("Feature::DataIndex(")+tmp+") not found";
      }
      return i->second;
    }

    /** Inverse of the function above.
	\param idx data index
	\param no_throw true if return -1 instead of throw
	\return segment index
    */
    int InverseDataIndex(int idx, bool no_throw = false) const {
      // cox::tictac::func tt(tics, "Feature::DataIndex");

      for (int_idx_map::const_iterator i = int2index.begin();
	   i!=int2index.end(); i++)
	if (i->second==idx)
	  return i->first;

      if (no_throw)
	return -1;

      stringstream ss;
      ss << "Feature::InverseDataIndex(" << idx << ") not found";
      throw ss.str();
    }

    /**
     */
    void RemoveDataLabelsAndIndices() {
      if (FrameVerbose())
	cout << "Feature::RemoveDataLabelsAndIndices()" << endl;
      data_labels.clear();
      int2index.clear();
      string2index.clear();
    }

    /** Creates a list of the data labels, and a mapping from labels
	to indexes and from label-numbers (the number of the image) to
	indexes. The indexes are to the corresponding position in the
	data label vector.  
	\param erase true if old contents is erased first
    */
    void SetDataLabelsAndIndices(bool);

    /** Returns a list of all data labels that have been used
	\return list of used data labels
    */
    virtual vector<string> UsedDataLabels() const { return vector<string>(); }

    /** An implementation for UsedDataLabels()
     */
    vector<string> UsedDataLabelsFromSegmentData() const;

    /** Solves all used data labels first from region specifications
	and then from derived virtual UsedDataLabels() method.
    */
    vector<string> SolveUsedDataLabels() const;

    /** Adds one data label in the case when they are dependent on
	the image frame.
    */
    void AddOneDataLabel(const string& l) {
      data_labels.push_back(l);
    }

   /** Returns a list of all the data labels
	\return list of all data labels
    */
    const vector<string>& DataLabels() const { return data_labels; }

    /** Prints a list of data labels
	\param f the name of the calling function (is printed also)
	\param l list of data labels to print
	\param os the output stream to print to
    */
    static void ShowDataLabels(const string&, const vector<string>&, ostream&);

    /** Return the number of Data objects stored
	\return object count
    */
    size_t DataCount() const { return data.size(); }

    /** Return Data object of index i
	\param i the index
	\return Data object
    */
    Data *GetData(int i) const {
      if (i<0 || i>=(int)DataCount()) {
	char tmp[100];
	sprintf(tmp, "i=%d DataCount()=%d", i, (int)DataCount()); 
	throw string("GetData() falsely indexed: ")+tmp;
      }
      return *(data.begin()+i);
    }

    /** 
     */
    void RemoveDataElements();

    /** Populates the data vector with data objects jl?
	\param erase true if erase old ones first
    */
    void AddDataElements(bool erase);

    /** Return the number of data elements
	\return number of data elements
    */
    virtual size_t DataElementCount() const {
      size_t n = data_labels.size();
      if (LabelVerbose())
	cout << "Feature::DataElementCount() = " << n << endl;
      return n;

      // from RegionBased: n = region_descr.size();  // obs! or _spec ???
    }

    /** Return a textual structure describing data components.
     */
    typedef list<pair<string,string> > comp_descr_e;
    typedef vector<comp_descr_e> comp_descr_t;
    virtual comp_descr_t ComponentDescriptions() const {
      return comp_descr_t();
    }

    // virtual functions that must or may be defined by derived classes

    //   virtual bool updateLabels(int to, int from, 
    // 			    const Segmentation::coordList &list);

    // virtual bool allocateTemporaryStorage(int maxlabel)=0;

    // other parts of the internal interface

    // Segmentation *GetSegmentation() const { return segments; }

    /// True if features have been calculated (jl?)
    bool calculated;

  public:
    /// A function type for preprocessing functions
    typedef bool (Feature::*preprocess_func)(imagedata&, const string&) const;

  private:
    /// A class describing a preprocessing function
    class preprocess_method_info_t {
    public:
      /** Constructor giving the preprocessing function a name and 
	  an actual function
	  \param the name of the preprocessing function
	  \param the actual function
      */
      preprocess_method_info_t(const string& s, preprocess_func f) :
	name(s), func(f) {}

      /// the name of the preprocessing function
      string name;

      /// the actual preprocessing function
      preprocess_func func;
    };

  private:
    /// Initialises the feature object, sets the default internal values
    /** Sets the internal list of options and processes them
	\param l the list of options to set
    */
    void SetAndProcessOptions(const list<string>& l, bool final);
  
    /// Set slicing and zoning if we have supplied a segmentation file
    void PossiblySetSlicingAndZoning() {
      PossiblySetSlicing();
      PossiblySetZoning();
    }

    /// Set slicing if we have supplied a segmentation file
    void PossiblySetSlicing();

    /// Set zoning if we have supplied a segmentation file
    void PossiblySetZoning();

    /// Set frame cacheing
    void SetFrameCacheing();

    // empty function jl?
    bool allocateFeatureVectors(int maxlabel);
      
    /// interpret command line option for histograms normalization
    hist_norm_type HistogramNormalization(const string& fnin);

    /// the following was moved from RegionBased.

    /// Set region specifications from the descriptions and the segmentation

  protected:
    bool SetRegionSpecifications();

    /// data members start from here:


    /** Prints the result vectors and labels from the specified frame
	\param s the slice to print from
	\param f the frame to print from
	\return true if ok
    */
    virtual bool DoPrinting(int s, int f);

    virtual bool DoPrintingSeparate(int s, int f);

    virtual void PairByPredicate(int s, int f);

    /// the segment file
    segmentfile *allocated_segmentfile;
    segmentfile *segmentfile_ptr;

    videofile *videofile_ptr;

    soundfile* deprecated_audiofile_ptr;

    string videosegment_file;

    /// the following was moved from RegionBased.

    /** Return the size of region with index i
	\param i the region index
	\return the region size 
    */
    int RegionSize(int i) const { return accessRegion(i).first->size(); }

    ///
    size_t RegionDescriptorCount() const { return region_descr.size(); }

    ///
    const pair<string,string>& RegionDescriptor(size_t i) const {
      return region_descr[i];
    }

    ///
    void AddRegionDescriptor(const string& d, const string& a) {
      region_descr.push_back(make_pair(d, a));
    }

    ///
    void SetRegionSpecificationWholeImage();

    /** Return the region with a specified index
	\param l the region index
	\return the regioninfo of the region
    */
    const pair<Region*,string>& accessRegion(size_t l) const {
      //   vector<Region *>::const_iterator
      //       i=region_spec.begin();
      //     while (i!=region_spec.end() && l--)
      //       i++;
      //     if (i==region_spec.end())
      //       throw "regioninfo index out of range";

      // shouldn't also this index start from one?

      if (l>=region_spec.size()) 
	throw("regioninfo index out of range");

      return region_spec[l];
    }

    /** Return the number of defined regions
	\return the number of regions
    */
    int NumberOfRegions() const { return region_spec.size(); }

    /// read the codebook for histograms from a file
    bool readHistogramBins(const string &fn);

    //
    int vector2codebookidx(const featureVector &v) const;

    //
    string HistogramSpecification(const string&, const string&) const;

    //
    string HistogramSpecification(const pair<string,string>& p) const {
      return HistogramSpecification(p.first, p.second);
    }

    //
    static bool KMeans(const string&, size_t, size_t, const string&);

#ifndef __MINGW32__
    /// timing structure
    cox::tictac tics;
#endif // __MINGW32__

  private:
    ///
    static string picsomroot;

    /// scaling parameter
    string scaling;

    /// jl
    picsom::scalinginfo scaleinfo;

    ///
    picsom::scalinginfo rotateinfo;

    /// jl? not used?
    bool include_zone_zero;
  
    /// jl
    int forceLabelsUpTo;

    /// Points to the head of a linked list.
    static const Feature *list_of_methods;

    /// Forms the chain of available methods.
    const Feature *next_of_methods;

    /// True if XML description header is printed first
    bool description_print;

    /// True if version, date, user, host etc. are printed in XML header
    bool description_all;

    /// text for XML description header's <cmdline>
    string description_cmdline;

    /// true if vector length is printed before actual vector(s)
    bool print_vector_length;

    /// true if length of printed vectors is
    /// checked against the class variable below
    bool check_vector_length;

    /// 
    int feature_vector_length;

    /// forced vector length (set as Y with command-line option -NX:Y)
    int feature_vector_length_fake;

    /// the verbose (or debug) level
    static int verbose;

    /// true if memory usage is to be used between frames
    static bool debug_memory_usage;

    /// holder for options passed to the program (command-line option etc.)
    list<string> options;

    /// holder for the options not yet processed
    list<string> options_unprocessed;

    /// list of vector of preprocessing methods
    pp_chain_t frame_preprocess_method, region_preprocess_method;

    /// vector of info objects to the preprocessing methods
    static vector<preprocess_method_info_t> preprocess_method_info;

    ///
    string frame_preprocess_in_use;

    /// Z-normalization parameters.
    vector<pair<float,float> > z_norm;

    /// larger than one if not all frames of a video are desired to be used
    int framestep;

    /// the number of digits that is displayed for floating-point variables.
    int precision;

    /// true if we are to use the background segment in the calculations
    bool use_background;

    /// true if we want to input raw features
    bool raw_in_mode;

    /// true if we want to output raw features
    bool raw_out_mode;

    /// true if we want to cache the frames and segments
    bool cacheing;

    /// true if region hierarchy is to be read from segmentation xml
    bool region_hierarchy;

    /// true if features are to be calculated for interest points 
    /// in segmentation xml
    bool interest_points;

    /// true if features are calculated for virtual regions in segmentation xml
    bool virtual_regions;

    /// true if all unspecifed primitive regions are to be excluded
    bool virtual_only;

    /// non-empty -> features are concatenated for region pairs the predicate
    string pair_by_predicate;

    /// location of temporary files
    string tempdir;

    /// location of temporary files
    static string static_tempdir;

    /// if we should keep temporary files
    bool keeptmp;

#ifdef HAVE_GLOG_LOGGING_H
    ///
    static bool glog_init_done;
#endif // HAVE_GLOG_LOGGING_H

    ///
    static int gpu_device_id;

    /// label forced for concatenated files
    string forced_label;

    /// list of all the data labels
    vector<string> data_labels;

    /// definition of label to index pair
    typedef pair<string,int> str_idx_pair;

    /// definition of number to index pair
    typedef pair<int,int> int_idx_pair;

    /// definition of label to index mapping type
    typedef map<string,int> str_idx_map;

    /// definition of number to index mapping type
    typedef map<int,int> int_idx_map;

    /// mapping from labels to indexes
    str_idx_map string2index;

    /// mapping from image numbers to indexes
    int_idx_map int2index;

    /// vector to data-pointers for each segment
    vector<Data*> data;

    /// per pixel multiplier when reading data in
    float pixel_multiply;

    /// stores the current pixel type
    pixeltype used_pixeltype;

    /// stores the current zoning type
    string used_zoning;

    /// stores the current slicing type
    string used_slicing;

    /// stores the current within frames treatment
    treat_type treat_within_frame;

    /// stores the current between frames treatment
    treat_type treat_between_frame;

    /// stores the current between slices treatment
    treat_type treat_between_slice;

    /// raw_input_pattern holds the pattern of the raw input filename
    string raw_input_pattern;

    /// output_pattern holds the pattern of the output filename
    string output_pattern;

    /// holds the actual output filename (generated from the pattern)
    string output_filename;

    /// holds the actual raw input filename (generated from the pattern)
    string raw_input_filename;

    /// forced value of %d and %r
    string outdir;

    /// subdir name that should be removed when processing %r for output_filename
    string subdir;

    /// stored input arguments of call to FormOutputFilename()
    string ofn_method;

    /// stored input arguments of call to FormOutputFilename()
    string ofn_feature;

    /// stored input arguments of call to FormOutputFilename()
    string ofn_files;

    /// the pattern that is used to create labels
    string label_pattern;

    /// interval how often XXXX:sYYYY labels should be issued
    float sec_label_interval;

    /// fake name to be used
    string fake_name;

    /// segmentation method abbreviation to be used in label generation
    string segmethod;

    /// list of region descriptions (was in RegionBased)
    vector<pair<string,string>> region_descr;

    /// list of regions (was in RegionBased)
    //  vector<rectangularRegion> region_spec_rectangular;

    vector<pair<Region*,string> > region_spec;

    ///
    vector<CompositeRegion> composite_regions;

    ///
    string video_segment_extension;

    /// forced framerange with -o framerange=first-last
    pair<size_t,size_t> framerange;

    /// list of slice begin and end frames (inclusive)
    vector<pair<size_t,size_t> > slice_bounds;

    /// unused margin of frames in the start of videos and other sequences
    size_t margin_start_set;

    /// unused margin of frames in the end of videos and other sequences
    size_t margin_end_set;

    /// The output in in-core format.
    feature_result result;

    /// If we store the read-in file somewhere before processing this
    /// filename is stored here. For example for External classes.
    string intermediate_filename;

  public:
    /// histogram mode
    string histogramMode;

    /// name of the histogram file specfification
    string hist_file_spec;

    /// names of individual histogram files
    vector<string> hist_file;

    /// descriptions of individual histogram files
    vector<pair<string,string> > hist_desc;

    /// summary of histogram descriptions
    string hist_desc_sum;

    /// histogram codebook
    vector<featureVector> histogramCodebook;    

    /// used histogram normalization for each separate zone
    hist_norm_type hist_zone_norm;

    /// used histogram normalization for each separate layer
    hist_norm_type hist_layer_norm;

    /// used histogram normalization for the full histogram
    hist_norm_type hist_global_norm;

    /// raw input read from here
    ifstream raw_input_stream;    

    /// raw input vector dimensionality stored here
    size_t raw_input_dim;   

    /// stores the latest vector read from raw input
    vector<float> raw_input_vector;

    /// non-negative values indicate that pixels
    /// that far away from segment borders should be 
    /// excluded fromfeature calculation if this makes sense

    int suppressborders;

    /// hidden secrets...
    void *opencvptr;

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)
    /// Given to OCVFeature-based methods.
    cv::Mat opencvmat;
#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV

    ///
    imagedata *incore_imagedata_ptr;

    ///
    feature_batch batch;
    
  };  // class Feature

} // namespace picsom

#endif // _Feature_h_

// Local Variables:
// mode: font-lock
// End:
