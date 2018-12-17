// -*- C++ -*- 	$Id: Torch.h,v 1.5 2018/12/16 07:51:34 jormal Exp $

#ifndef _Torch_
#define _Torch_

#include "RegionBased.h"

#ifdef HAVE_THC_H

// #define USE_MKL

#include <THC.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luaT.h>
}

#include <cuda_runtime.h>
#include <cstring>

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// #pragma GCC diagnostic ignored "-Wsign-compare"
// #pragma GCC diagnostic ignored "-Wignored-qualifiers"
// #include <torch/torch.hpp>
// #pragma GCC diagnostic pop

#include <boost/algorithm/string.hpp>
using namespace boost;

#include <RwLock.h>

namespace picsom {
  /// Feature calculation with libtorch
  class Torch : public RegionBased {
  public:
    ///
    class torch_t {
    public:
      ///
      torch_t() : L(NULL), n_layers(0) {}

      ///
      torch_t(const string& na, const string& mo,  const string& me, 
	      const vector<pair<float,float> >& ms,
	      const vector<size_t>& ma, const vector<string>& dof);

      ///
      ~torch_t();

      ///
      string name, model, mean;

      ///
      lua_State *L;

      ///
      vector<pair<float,float> > meanstd;

      ///
      vector<size_t> map;

      ///
      size_t n_layers;

    }; // class torch_t


    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    Torch(const string& img = "", const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt);
      InitializeTorch();
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    Torch(const string& img, const string& seg,
	  const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt);
      InitializeTorch();
    }

    /** Constructor
	\param s segmentation object
	\param opt list of options to initialise to
    */
    Torch(segmentfile *s, const list<string>& opt = (list<string>)0) {
      Initialize("", "", s, opt);
      InitializeTorch();
    }

    /// Constructor
    Torch(bool) : RegionBased(false) {}

    virtual Feature *Create(const string& s) const {
      return (new Torch())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype pt, int n, int i) const {
      return new TorchData(pt, n, RegionSize(i), this);
    }

    virtual void deleteData(void *p) {
      delete (TorchData*)p;
    }

    virtual string Name()          const { return "torch"; }

    virtual string LongName()      const { return "torch detections"; }

    virtual string ShortText()     const {
      return "Torch detections, different versions."; }

    virtual string Version() const;

    virtual string TargetType() const { return "image"; }

    // this is still just a placeholder...
    virtual string  NeededSegmentation() const { return "region.box"; }

    virtual featureVector getRandomFeatureVector() const {
      return featureVector();
    }

    virtual int printMethodOptions(ostream&) const { return 0; }

    virtual bool ProcessRegion(const Region&, const vector<float>&, Data*);

    virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

    // virtual int DataElementCount() const { return NumberOfRegions(); }
    // Feature::DataElementCount() should do (vvi), this one doesn't

  protected:  
    ///
    bool InitializeTorch();

    ///
    virtual bool ProcessOptionsAndRemove(list<string>&);

    static bool LuaDoString(const torch_t&, const string&, bool = true);

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
    class TorchData : public RegionBasedData {
    public:
      friend class Torch;

      /** Constructor
	  \param pt sets the pixeltype of the data
	  \param n sets the extension (jl?)
	  \param l length of data vector
      */
      TorchData(pixeltype pt, int n, int l,const Feature *p) : 
	RegionBasedData(pt, n,p), datavec(l*DataUnitSize()) { Zero(); }

      /// Because we have virtual member functions...
      virtual ~TorchData() {}

      /// Zeros the internal data
      virtual void Zero() { RegionBasedData::Zero(); }

      /** Returns the name of the feature
	  \return feature name
      */    
      virtual string Name() const { return "TorchData"; }

      /** Returns the size of the internal data
	  \return data length
      */
      virtual int Length() const { return datavec.size(); }

      /// += operator
      virtual Data& operator+=(const Data &) {
	// this should check that typeof(d) == typeof(*this) !!!
	throw "TorchData::operator+=() not defined"; }

      /** Returns the result of the feature extraction
	  \return feature result vector
      */
      virtual featureVector Result(bool /*allow_zero_count*/) const {
	if (Feature::LabelVerbose())
	  cout << "TorchData::Result()" << endl;

	return datavec;
      }
    
    protected:
      /// Stores the internal data
      vector<float> datavec;
    };

    ///
    static map<string,torch_t> torchmap;

    ///
    static RwLock torchmaplock;

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
    vector<string> dofiles;

    ///
    fv_tree_node *root;
  };
}

#endif // HAVE_THC_H

#endif

// Local Variables:
// mode: font-lock
// End:
