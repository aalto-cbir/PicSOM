// -*- C++ -*- 	$Id: Caffe.h,v 1.16 2016/01/26 11:54:38 jorma Exp $

#ifndef _Caffe_
#define _Caffe_

#include "RegionBased.h"

#ifdef HAVE_CAFFE_DATA_TRANSFORMER_HPP

// #define USE_MKL

#include <cuda_runtime.h>
#include <cstring>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <caffe/caffe.hpp>
#pragma GCC diagnostic pop

#include <boost/algorithm/string.hpp>
using namespace boost;

#include <RwLock.h>

namespace picsom {
  /// Feature calculation with libcaffe
  class Caffe : public RegionBased {
  public:
    ///
    class caffe_t {
    public:
      ///
      caffe_t() : net(NULL) {}

      ///
      caffe_t(const string& na, const string& pt, const string& mo,
	      const string& me, caffe::Net<float> *ne,
	      const imagedata& mi, const vector<size_t>& ma) :
	name(na), prototxt(pt), model(mo), mean(me), net(ne),
	mimg(mi), map(ma) {}

      ///
      ~caffe_t() { /*delete net;*/ }

      ///
      string name, prototxt, model, mean;

      ///
      caffe::Net<float> *net;

      ///
      imagedata mimg;

      ///
      vector<size_t> map;

    }; // class caffe_t


    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    Caffe(const string& img = "", const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt);
      InitializeCaffe();
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    Caffe(const string& img, const string& seg,
	  const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt);
      InitializeCaffe();
    }

    /** Constructor
	\param s segmentation object
	\param opt list of options to initialise to
    */
    Caffe(segmentfile *s, const list<string>& opt = (list<string>)0) {
      Initialize("", "", s, opt);
      InitializeCaffe();
    }

    /// Constructor
    Caffe(bool) : RegionBased(false) {}

    virtual Feature *Create(const string& s) const {
      return (new Caffe())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype pt, int n, int i) const {
      return new CaffeData(pt, n, RegionSize(i), this);
    }

    virtual void deleteData(void *p) {
      delete (CaffeData*)p;
    }

    virtual string Name()          const { return "caffe"; }

    virtual string LongName()      const { return "caffe detections"; }

    virtual string ShortText()     const {
      return "Caffe detections, different versions."; }

    virtual string Version() const;

    virtual string TargetType() const { return "image"; }

    // this is still just a placeholder...
    virtual string  NeededSegmentation() const { return "region.box"; }

    virtual featureVector getRandomFeatureVector() const {
      return featureVector();
    }

    virtual int printMethodOptions(ostream&) const { return 0; }

    virtual bool ProcessRegion(const Region&, const vector<float>&, Data*);

    bool ProcessRegionNew(const Region&, const vector<float>&, Data*);

    bool ProcessRegionOld(const Region&, const vector<float>&, Data*);

    virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

    // virtual int DataElementCount() const { return NumberOfRegions(); }
    // Feature::DataElementCount() should do (vvi), this one doesn't

  protected:  

    bool InitializeCaffe();

    virtual bool ProcessOptionsAndRemove(list<string>&);

    ///
    bool ProcessBlocksString(size_t, size_t);

    ///
    typedef enum {
      FV_NODE_UNDEF, FV_NODE_LEAF,FV_NODE_CAT,FV_NODE_MAX,FV_NODE_AVG,FV_NODE_VLAD
    } fv_tree_node_type;

    ///
    class fv_leaf_recipe {
    public:
      ///
      fv_leaf_recipe() : ulx(0), uly(0), lrx(0), lry(0) {}

      ///
      string str() const { 
	stringstream ss;
	ss << "imgsrc=\"" << imgsrc << "\" layer=\"" << layer << "\""
	   << " ulx=" << ulx << " uly=" << uly
	   << " lrx=" << lrx << " lry=" << lry;
	return ss.str();
      }

      ///
      string imgsrc; // something like 256x256a

      ///
      string layer; 

      ///
      size_t ulx, uly, lrx, lry; // corners, both inclusive: w=lrx-ulx+1

    }; // class fv_leaf_recipe

    ///
    class fv_tree_node {
    public:

      fv_tree_node() : idx(-1), node_type(FV_NODE_UNDEF) {} 
      
      vector<fv_tree_node*> leaves(); 
      
      vector<float> value();

      void value(const vector<float>& v) {
	if (node_type!=FV_NODE_LEAF)
	  throw string("node_type!=FV_NODE_LEAF");
	leaf_value = v;
      }

      vector<float> concatenate(const vector<float> &a, const vector<float> &b);
      
      vector<float> elementwise_max(const vector<float> &a, const vector<float> &b);
      
      vector<float> elementwise_sum(const vector<float> &a, const vector<float> &b);
      
      vector<float> elementwise_div(const vector<float> &a, size_t b);

      int idx;
      fv_tree_node_type node_type;
      vector<fv_tree_node*> children;
      fv_leaf_recipe leaf_recipe;
      vector<float> leaf_value;
    }; // class fv_tree_node

    ///
    void ProcessBlocksL(size_t w, size_t h, const string&, const string&, fv_tree_node*);

    ///
    void ProcessBlocksImgsrc(string& imgsrc, size_t& ix, size_t& iy, size_t w, size_t h);

    ///
    void GenerateLeafs(float sx, float sy, size_t gx, size_t gy, 
		       size_t bx, size_t by, size_t wx, size_t wy,
		       const string& imagesrc, const string& layer,
		       fv_tree_node *parent);

    ///
    vector<fv_tree_node*> BlocksLeaves() { return root->leaves(); }

    ///
    vector<float> BlocksValue() { return root->value(); }

    ///
    const pair<imagedata,imagedata>&
    CreateBlocksImage(const imagedata&, const imagedata&,
		      const string&, map<string,pair<imagedata,imagedata> >&);

    /// Data object for raw data
    class CaffeData : public RegionBasedData {
    public:
      friend class Caffe;

      /** Constructor
	  \param pt sets the pixeltype of the data
	  \param n sets the extension (jl?)
	  \param l length of data vector
      */
      CaffeData(pixeltype pt, int n, int l,const Feature *p) : 
	RegionBasedData(pt, n,p), datavec(l*DataUnitSize()) { Zero(); }

      /// Because we have virtual member functions...
      virtual ~CaffeData() {}

      /// Zeros the internal data
      virtual void Zero() { RegionBasedData::Zero(); }

      /** Returns the name of the feature
	  \return feature name
      */    
      virtual string Name() const { return "CaffeData"; }

      /** Returns the size of the internal data
	  \return data length
      */
      virtual int Length() const { return datavec.size(); }

      /// += operator
      virtual Data& operator+=(const Data &) {
	// this should check that typeof(d) == typeof(*this) !!!
	throw "CaffeData::operator+=() not defined"; }

      /** Returns the result of the feature extraction
	  \return feature result vector
      */
      virtual featureVector Result(bool /*allow_zero_count*/) const {
	if (Feature::LabelVerbose())
	  cout << "CaffeData::Result()" << endl;

	return datavec;
      }
    
    protected:
      /// Stores the internal data
      vector<float> datavec;
    };

    ///
    static map<string,caffe_t> caffemap;

    ///
    static RwLock caffemaplock;

    ///
    string model_name;

    ///
    string blocks;

    ///
    size_t gidx;

    ///
    string layer;

    ///
    string windowsize;

    ///
    // string colorspace;

    ///
    string resize;  // deprecated

    ///
    string fusion;  // deprecated

    ///
    fv_tree_node *root;
  };
}

#endif // HAVE_CAFFE_DATA_LAYERS_HPP

#endif

// Local Variables:
// mode: font-lock
// End:
