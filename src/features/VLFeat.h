// -*- C++ -*- 	$Id: VLFeat.h,v 1.7 2015/11/10 08:02:32 jorma Exp $
/**
   \file VLFeat.h

   \brief Declarations and definitions of class VLFeat
   
   VLFeat.h contains declarations and definitions of 
   class the VLFeat, which
   is a class that wraps the VLFeat features
  
   \author Jorma Laaksonen <jorma.laaksonen@aalto.fi>
   $Revision: 1.7 $
   $Date: 2015/11/10 08:02:32 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _VLFeat_
#define _VLFeat_

#include "Feature.h"

#ifdef HAVE_VL_GENERIC_H

#include <vl/dsift.h>
#include <vl/gmm.h>
#include <vl/kmeans.h>
#include <vl/kdtree.h>

#include <cox/subspace>

namespace picsom {
/// A class that wraps VLFeat descriptors
  class VLFeat : public Feature {
  public:

    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    VLFeat(const string& img = "", const list<string>& opt = (list<string>)0)
      :  do_fisher(false), do_vlad(false), gmm(NULL), kmeans(NULL),
	 kdtree(NULL), kdtree_ntrees(0), kdtree_maxcomps(0) {
      Initialize(img, "", NULL, opt);
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    VLFeat(const string& img, const string& seg,
	   const list<string>& opt = (list<string>)0)
      : do_fisher(false), do_vlad(false), gmm(NULL), kmeans(NULL),
	kdtree(NULL), kdtree_ntrees(0), kdtree_maxcomps(0) {
      Initialize(img, seg, NULL, opt);
    }

    /// This constructor doesn't add an entry in the methods list.
    VLFeat(bool) : Feature(false),
		   do_fisher(false), do_vlad(false), gmm(NULL) , kmeans(NULL),
		   kdtree(NULL), kdtree_ntrees(0), kdtree_maxcomps(0) {}

    ///
    ~VLFeat() {
      if (gmm)
	vl_gmm_delete(gmm);
      if (kmeans)
	vl_kmeans_delete(kmeans);
      if (kdtree)
	vl_kdforest_delete(kdtree);
    }

    ///
    virtual string OptionSuffix() const {
      string g = gmmname!="" ? gmmname : kmeansname;
      size_t p = g.rfind('/');
      if (p!=string::npos)
	g.erase(0, p+1);
      p = g.rfind(".bin");
      if (p!=string::npos)
	g.erase(p);
      string k;
      if (kdtree)
	k = "-kd-"+ToStr(kdtree_ntrees)+"-"+ToStr(kdtree_maxcomps);
      string s = "-"+g+k+(do_fisher?"-fv":do_vlad?"-vlad":"zzz");
      return s;
    }

    virtual Feature *Create(const string& s) const {
      return (new VLFeat())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
      VectorData *d = new VectorData(t, n, this);
      vector<float> v = getRandomFeatureVector();
      d->setLength(v.size());
      d->Zero();
      return d;
    }

    virtual void deleteData(void *p) {
      delete (VectorData*)p;
    }

    virtual bool Incremental() const { return false; }

    virtual bool ReInitializable() const { return false; } // should be true...

    virtual bool ReInitialize() {
      // something ?
      return Feature::ReInitialize();
    }
  
    virtual bool CalculateOneFrame(int f) { return CalculateCommon(f, true); }
      
    virtual bool CalculateOneLabel(int f, int l) {
      return CalculateCommon(f, false, l); }

    virtual string TargetType() const { return "segment"; }

    virtual treat_type DefaultWithinFrameTreatment() const {
      return treat_concatenate; }
  
    virtual treat_type DefaultBetweenFrameTreatment() const {
      return treat_separate; }
  
    virtual treat_type DefaultBetweenSliceTreatment() const {
      return treat_separate; }
  
    virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

    virtual string Name()          const { return "vlfeat"; }

    virtual string LongName()      const { return "VLFeat descriptors"; }

    virtual string ShortText()     const {
      return LongName();
    }

    // virtual const string& DefaultZoningText() const {
    //   static string centerdiag("5"); // does not make sense for fixed regions
    //   return centerdiag; 
    // }

    virtual Feature *Initialize(const string& img, const string& seg,
				segmentfile *s, const list<string>& opt,
				bool allocate_data=true);

    ///
    bool Initialize_pca();

    ///
    bool Initialize_gmm();

    ///
    bool Initialize_kmeans();

    virtual string Version() const;

    virtual featureVector getRandomFeatureVector() const{
      size_t dim = 7;
      
      if (gmm) // == fisher
	dim = 2*descriptor_dim()*nclusters();

      if (kmeans) // == vlad
	dim = descriptor_dim()*nclusters();

      if (FrameVerbose())
	cout << "VLFeat::getRandomFeatureVector() returning vector of dim "
	     << dim << endl;

      return vector<float>(dim);
    }

    virtual int printMethodOptions(ostream&) const;

  protected:
    ///
    virtual vector<string> UsedDataLabels() const {
      vector<string> ret { "" };

      if (LabelVerbose())
	ShowDataLabels("VLFeat::UsedDataLabels", ret, cout);
    
      return ret;
    }
  
    /** Function commonly used by CalculateOneFrame and CalculateOneLabel
	\param f the frame to calculate feature from
	\param all true if we are to calculate for all segments
	\param l the dataindex for the segment to calculate for
    */
    virtual bool CalculateCommon(int f, bool all, int l = -1);

  public:
    virtual featureVector CalculateRegion(const Region&) {
      return featureVector();
    }

    virtual bool ProcessOptionsAndRemove(list<string>&); 

  protected:
    ///
    size_t descriptor_dim() const {
      return gmm ? vl_gmm_get_dimension(gmm) :
	kmeans ? vl_kmeans_get_dimension(kmeans) : 0;
    }
    
    ///
    size_t nclusters() const {
      return gmm ? vl_gmm_get_num_clusters(gmm) :
	kmeans ? vl_kmeans_get_num_centers(kmeans) : 0;
    }

    ///
    const void *means() const {
      return gmm ? vl_gmm_get_means(gmm) :
	kmeans ? vl_kmeans_get_centers(kmeans) : NULL;
    }

    ///
    const void *covariances() const {
      return gmm ? vl_gmm_get_covariances(gmm) : NULL;
    }

    ///
    const void *priors() const {
      return gmm ? vl_gmm_get_priors(gmm) : NULL;
    }

    ///
    bool do_fisher;

    ///
    bool do_vlad;

    ///
    string pcaname;

    ///
    string gmmname;

    ///
    string kmeansname;

    ///
    VlGMM *gmm;

    ///
    VlKMeans *kmeans;

    ///
    VlKDForest *kdtree;

    ///
    size_t kdtree_ntrees, kdtree_maxcomps;

    ///
    cox::subspace<float> pca;

  }; // class VLFeat

} // namespace picsom

#endif // HAVE_VL_GENERIC_H

#endif

// Local Variables:
// mode: font-lock
// End:
