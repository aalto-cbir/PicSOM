// -*- C++ -*- 	$Id: ImprovedTrajectory.h,v 1.3 2016/12/19 08:15:43 jorma Exp $
/**
   \file ImprovedTrajectory.h

   \brief Declarations and definitions of class ImprovedTrajectory
   
   ImprovedTrajectory.h contains declarations and definitions of 
   class the ImprovedTrajectory, which
   is a class that wraps the ImprovedTrajectory features
  
   \author Jorma Laaksonen <jorma.laaksonen@aalto.fi>
   $Revision: 1.3 $
   $Date: 2016/12/19 08:15:43 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _ImprovedTrajectory_
#define _ImprovedTrajectory_

#include "Feature.h"
#include "bin_data.h"

#ifdef HAVE_IMPROVED_TRAJECTORY_RELEASE_DENSETRACKSTAB_H

#include "improved_trajectory_release/DenseTrackStab.h"
#include "improved_trajectory_release/Initialize.h"
#include "improved_trajectory_release/Descriptors.h"
#include "improved_trajectory_release/OpticalFlow.h"

#include <cox/subspace>

namespace picsom {
    using namespace improved_trajectory_release;

/// A class that wraps ImprovedTrajectory descriptors
  class ImprovedTrajectory : public Feature {

  public:

    /** Constructor
	\param img initialise to this image
	\param opt list of options to initialise to
    */
    ImprovedTrajectory(const string& img = "", const list<string>& opt = (list<string>)0) {
      Initialize(img, "", NULL, opt);
    }

    /** Constructor
	\param img initialise to this image
	\param seg initialise with this segmentation
	\param opt list of options to initialise to
    */
    ImprovedTrajectory(const string& img, const string& seg,
	       const list<string>& opt = (list<string>)0) {
      Initialize(img, seg, NULL, opt);
    }

    /// This constructor doesn't add an entry in the methods list.
    ImprovedTrajectory(bool) : Feature(false) {}

    ///
    ~ImprovedTrajectory() {
      // if (gmm)
      // 	vl_gmm_delete(gmm);
      // if (kmeans)
      // 	vl_kmeans_delete(kmeans);
      // if (kdtree)
      // 	vl_kdforest_delete(kdtree);
    }

    ///
    virtual string OptionSuffix() const {
      // string g = gmmname!="" ? gmmname : kmeansname;
      // size_t p = g.rfind('/');
      // if (p!=string::npos)
      // 	g.erase(0, p+1);
      // p = g.rfind(".bin");
      // if (p!=string::npos)
      // 	g.erase(p);
      // string k;
      // if (kdtree)
      // 	k = "-kd-"+ToStr(kdtree_ntrees)+"-"+ToStr(kdtree_maxcomps);
      // string s = "-"+g+k+(do_fisher?"-fv":do_vlad?"-vlad":"zzz");
      // return s;
      return "";
    }

    virtual Feature *Create(const string& s) const {
      return (new ImprovedTrajectory())->SetModel(s);
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

    virtual bool HasRawOutMode() const { return true; }

    virtual bool IsImageFeature() const { return false; }

    virtual bool IsVideoFeature() const { return true; }

    virtual bool IsPerFrameCombinable() { return false; }

    virtual bool Incremental() const { return false; }

    virtual bool ReInitializable() const { return false; } // should be true...

    virtual bool ReInitialize() {
      // something ?
      return Feature::ReInitialize();
    }
  
    virtual featureVector CalculateVideoSegment(bool print);

    featureVector ImprovedDenseTrajectory(const string&);

    virtual bool CalculateOneFrame(int) { return false; }
      
    virtual bool CalculateOneLabel(int, int) { return false; }

    virtual string TargetType() const { return "video"; }

    virtual treat_type DefaultWithinFrameTreatment() const {
      return treat_concatenate; }
  
    virtual treat_type DefaultBetweenFrameTreatment() const {
      return treat_separate; }
  
    virtual treat_type DefaultBetweenSliceTreatment() const {
      return treat_separate; }
  
    virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

    virtual string Name()          const { return "improvedtrajectory"; }

    virtual string LongName()      const { return "ImprovedTrajectory descriptors"; }

    virtual string ShortText()     const {
      return LongName();
    }

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
      size_t dim = 0;
      dim += densetraj.nobjects();
      dim += hog.nobjects();
      dim += hof.nobjects();
      dim += mbhx.nobjects();
      dim += mbhy.nobjects();
      
      if (FrameVerbose())
	cout << "ImprovedTrajectory::getRandomFeatureVector() returning vector of dim "
	     << dim << endl;

      return vector<float>(dim);
    }

    virtual int printMethodOptions(ostream&) const;

    void Desc2Vec(std::vector<float>& desc, DescInfo& descInfo,
		  TrackInfo& trackInfo, vector<float>& res);

    bool ProcessRawTraj(const vector<float>&);

    int SolveHit(const vector<float>&, size_t, size_t, bin_data& d, size_t&);

    bool SetHit(int, vector<size_t>&, size_t);

    bool AppendHist(vector<float>&, const vector<size_t>&, size_t, size_t);

    featureVector CreateFeatureVector(size_t);

  protected:
    ///
    virtual vector<string> UsedDataLabels() const {
      vector<string> ret { "" };

      if (LabelVerbose())
	ShowDataLabels("ImprovedTrajectory::UsedDataLabels", ret, cout);
    
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
    // size_t descriptor_dim() const {
    //   return gmm ? vl_gmm_get_dimension(gmm) :
    // 	kmeans ? vl_kmeans_get_dimension(kmeans) : 0;
    // }
    
    // ///
    // size_t nclusters() const {
    //   return gmm ? vl_gmm_get_num_clusters(gmm) :
    // 	kmeans ? vl_kmeans_get_num_centers(kmeans) : 0;
    // }

    // ///
    // const void *means() const {
    //   return gmm ? vl_gmm_get_means(gmm) :
    // 	kmeans ? vl_kmeans_get_centers(kmeans) : NULL;
    // }

    // ///
    // const void *covariances() const {
    //   return gmm ? vl_gmm_get_covariances(gmm) : NULL;
    // }

    // ///
    // const void *priors() const {
    //   return gmm ? vl_gmm_get_priors(gmm) : NULL;
    // }

    ///
    string densetrajname, hogname, hofname, mbhxname, mbhyname;

    ///
    bin_data densetraj, hog, hof, mbhx, mbhy;

    ///
    vector<size_t> densetrajhist, hoghist, hofhist, mbhxhist, mbhyhist;

  }; // class ImprovedTrajectory

} // namespace picsom

#endif // HAVE_IMPROVED_TRAJECTORY_RELEASE_DENSETRACKSTAB_H

#endif

// Local Variables:
// mode: font-lock
// End:
