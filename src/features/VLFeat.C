// $Id: VLFeat.C,v 1.11 2015/11/10 08:02:38 jorma Exp $	

#include <VLFeat.h>

#ifdef HAVE_VL_GENERIC_H

#include <bin_data.h>

#include <vl/fisher.h>
#include <vl/vlad.h>

#include <cox/matrix>

namespace picsom {
  static const char *vcid =
    "$Id: VLFeat.C,v 1.11 2015/11/10 08:02:38 jorma Exp $";

  static VLFeat list_entry(true);

  //===========================================================================

  string VLFeat::Version() const {
    return vcid;
  }

  //===========================================================================

  bool VLFeat::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "VLFeat::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
        cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="pca") {
        pcaname = value;
        i = l.erase(i);
        continue;
      }

      if (key=="gmm") {
        gmmname = value;
        i = l.erase(i);
        continue;
      }

      if (key=="kmeans") {
        kmeansname = value;
        i = l.erase(i);
        continue;
      }

      if (key=="kdtree") {
	vector<string> nm = SplitInCommasObeyParentheses(value);
	if (nm.size()!=2) {
	  cerr << msg << "failed to parse \"" << value << "\"" << endl;
	  return false;
	}
	kdtree_ntrees   = atoi(nm[0].c_str());
	kdtree_maxcomps = atoi(nm[1].c_str());
        i = l.erase(i);
        continue;
      }

      i++;
    }

    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  int VLFeat::printMethodOptions(ostream &str) const {
    /// obs! dunno yet

    int ret = Feature::printMethodOptions(str);

    return ret;
  }

  //===========================================================================

  Feature *VLFeat::Initialize(const string& img, const string& seg,
			      segmentfile *s, const list<string>& opt,
			      bool allocate_data) {
    //
    // this possibly needs some explanation (or more research...)
    //  1) in some other classes Feature::Initialize() is called here last
    //  2) Feature::Initialize() is called here first because it
    //     calls virtual ProcessOptionsAndRemove() that can set gmmname.
    //
    if (!Feature::Initialize(img, seg, s, opt, allocate_data))
      return NULL;

    if (pcaname!="" && !pca.vector_length())
      Initialize_pca();

    if (gmmname!="" && !gmm)
      Initialize_gmm();

    if (kmeansname!="" && !kmeans)
      Initialize_kmeans();

    return this;
  }

  //===========================================================================

  bool VLFeat::Initialize_pca() {
    bin_data d(pcaname);

    cox::matrix<float> m;

    size_t n = d.nobjects();
    for (size_t i=0; i<n; i++) {
      vector<float> v = d.get_float(i);

      if (i==0)
	pca.mean() = v;
      else
	pca.base().append_column(v);
    }

    if (MethodVerbose())
      cout << "VLFeat::Initialize_pca() read in pca <" << gmmname
	   << "> dim in=" << pca.vector_length()
	   << " out=" << pca.base().columns() << endl;

    return true;
  }

  //===========================================================================

  bool VLFeat::Initialize_gmm() {
    bin_data dat(gmmname);
    const bin_data::header& h = dat.get_header_copy();
    size_t draw = h.vdim, dv = (draw-1)/2, nv = 0;
    if (draw!=dv*2+1)
      cerr << "VLFeat::Initialize() dim error with gmm" << endl;

    vector<float> means, covar, prior;
      
    for (size_t i=0; true; i++)
      if (!dat.exists(i)) 
	break;
      else {
	const vector<float> v = dat.get_float(i);
	means.insert(means.end(), &v[0],    &v[dv]);
	covar.insert(covar.end(), &v[dv],   &v[2*dv]);
	prior.insert(prior.end(), &v[2*dv], &v[draw]);
	nv++;
      }
      
    gmm = vl_gmm_new(VL_TYPE_FLOAT, dv, nv);
    vl_gmm_set_means(      gmm, &*means.begin());
    vl_gmm_set_covariances(gmm, &*covar.begin());
    vl_gmm_set_priors(     gmm, &*prior.begin());

    if (MethodVerbose())
      cout << "VLFeat::Initialize_gmm() read in gmm <" << gmmname << "> dim="
	   << descriptor_dim() << " nclust=" << nclusters() << endl;

    do_fisher = true;

    return true;
  }

  //===========================================================================

  bool VLFeat::Initialize_kmeans() {
    bin_data dat(kmeansname);
      
    kmeans = vl_kmeans_new(VL_TYPE_FLOAT, VlDistanceL2);
    vl_kmeans_set_centers(kmeans, dat.vector_address(0),
			  dat.vdim(), dat.nobjects());

    if (MethodVerbose())
      cout << "VLFeat::Initialize_kmeans() read in kmeans <" << kmeansname
	   << "> dim=" << descriptor_dim() << " nclust=" << nclusters()
	   << endl;

    
    if (kdtree_ntrees) {
      kdtree = vl_kdforest_new(VL_TYPE_FLOAT,  descriptor_dim(), kdtree_ntrees,
			       VlDistanceL2);

      vl_kdforest_build(kdtree, nclusters(), means());
      vl_kdforest_set_max_num_comparisons(kdtree, kdtree_maxcomps);
    }

    do_vlad = true;

    return true;
  }

  //===========================================================================

  bool VLFeat::CalculateCommon(int f, bool all, int l) {
    string msg = "VLFeat::CalculateCommon("+ToStr(f)+","+ToStr(all)+","+
      ToStr(l)+") : ";

    // if (!do_fisher && !do_vlad) {
    //   cerr << msg
    // 	   << "either encoding=fisher or encoding=vlad should be specified"
    // 	   << endl;
    //   return false;
    // }

    if (!gmm && !kmeans) {
      cerr << msg << "either gmm=xxx or kmeans=xxx option should be given"
	   << endl;
      return false;
    }

    cox::tictac::func tt(tics, "VLFeat::CalculateCommon");
    
    // obs! only some parameters here, should be in ProcessOptionsAndRemove()
    // too, also scales and geometry should be made specifiable...
    bool normalizeSift = false, renormalize = true, flat_window = true;
    size_t step = 3, binsize = 8;

    EnsureImage();

    int width = Width(true), height = Height(true);

    if (FrameVerbose())
      cout << msg+"wxh="
	   << width << "x" << height << "=" << width*height << endl;

    vector<float> rgbcoeff { 0.2989, 0.5870, 0.1140 };

    imagedata idata = CurrentFrame();
    idata.convert(imagedata::pixeldata_float);
    idata.force_one_channel(rgbcoeff);

    vector<float> dsift;
    size_t descr_size_orig = 0, descr_size_final = 0;
    vector<float> scales { 1.0000, 0.7071, 0.5000, 0.3536, 0.2500 };
    // vector<float> scales { 1.0000 };
    for (size_t i=0; i<scales.size(); i++) {
      if (KeyPointVerbose())
	cout << "Starting vl_dsift_process() in scale " << scales[i] << endl;
      
      imagedata simg = idata;
      if (scales[i]!=1) {
	scalinginfo si(simg.width(), simg.height(),
		       (int)floor(scales[i]*simg.width()+0.5),
		       (int)floor(scales[i]*simg.height()+0.5));
	simg.rescale(si, 1);
      }

      // VlDsiftFilter *sf = vl_dsift_new(simg.width(), simg.height());
      VlDsiftFilter *sf = vl_dsift_new_basic(simg.width(), simg.height(),
					     step, binsize);

      // opts.scales = logspace(log10(1), log10(.25), 5) ;
      // void vl_dsift_set_bounds	(	VlDsiftFilter * 	self,
      // 					int 	minX,
      // 					int 	minY,
      // 					int 	maxX,
      // 					int 	maxY 
      // 					);	
      
      // VlDsiftDescriptorGeometry geom = { 8, 4, 4, 0, 0 };
      // vl_dsift_set_geometry(sf, &geom);
      
      //vl_dsift_set_steps(sf, 3, 3);

      //vl_dsift_set_window_size(sf, 8);

      vl_dsift_set_flat_window(sf, flat_window); // aka fast in matlab

      vector<float> imgvec = simg.get_float();
      const float *img_fp = &imgvec[0];
      // cout << "IMAGE = " << img_fp[0] << " " << img_fp[1] << " "
      //      << img_fp[2] << " ... " << img_fp[41] << endl;

      vl_dsift_process(sf, img_fp);
      
      // if opts.rootSift // false
      // 		descrs{si} = sqrt(descrs{si}) ;
      // end
      // 	if opts.normalizeSift //true
      // 		  descrs{si} = snorm(descrs{si}) ;
      // end

      descr_size_orig = sf->descrSize;
      size_t nf = sf->numFrames;
      const VlDsiftKeypoint *k = sf->frames;
      float *d  = sf->descrs;
      
      if (KeyPointVerbose())
	cout << "  found " << sf->numFrames << " 'frames' in "
	     << simg.info() << endl
	     << "  descriptor dim " << descr_size_orig << endl;
      
      if (PixelVerbose())
	for (size_t i=0; i<nf; i++) {
	  cout << "  i=" << i << " x=" << k[i].x << " y=" << k[i].y
	       << " s=" << k[i].s << " norm=" << k[i].norm;
	  if (FullVerbose()) {
	    cout << " RAW";
	    for (size_t j=0; j<descr_size_orig; j++)
	      cout << " " << d[i*descr_size_orig+j];
	  }
	  cout << endl;
	}

      if (normalizeSift) {
	for (size_t i=0; i<nf; i++) {
	  if (PixelVerbose())
	    cout << "  i=" << i << " x=" << k[i].x << " y=" << k[i].y
		 << " s=" << k[i].s << " norm=" << k[i].norm;
	  double mul = 0.0;
	  for (size_t j=0; j<descr_size_orig; j++)
	    mul += d[i*descr_size_orig+j]*d[i*descr_size_orig+j];
	  if (mul)
	    mul = 1.0/sqrt(mul);
	  if (FullVerbose())
	    cout << " NORM";
	  for (size_t j=0; j<descr_size_orig; j++) {
	    d[i*descr_size_orig+j] *= mul;
	    if (FullVerbose())
	      cout << " " << d[i*descr_size_orig+j];
	  }
	  if (PixelVerbose())
	    cout << endl;
	}
      }

      if (!pca.vector_length()) {
	dsift.insert(dsift.end(), d, d+nf*descr_size_orig);
	descr_size_final = descr_size_orig;

      } else {
	for (size_t i=0; i<nf; i++) {
	  vector<float> vin(d+i*descr_size_orig, d+(i+1)*descr_size_orig);
	  vector<float> vout = pca.projection_coeff(vin);
	  dsift.insert(dsift.end(), vout.begin(), vout.end());
	}
	descr_size_final = pca.base_size();
      }
	  
      vl_dsift_delete(sf);
    }

    size_t numdata = dsift.size()/descr_size_final;
    const float *datain = &dsift[0];

    vector<float> enc((do_fisher?2:1)*descriptor_dim()*nclusters());
    float *dataout = &enc[0];
      
    if (do_fisher) {
      if (FrameVerbose())
	cout << msg << "fisher encoding " << numdata
	     << " descriptors of size " << descr_size_orig
	     << " => " << descr_size_final
	     << " with gmm dimensionality " << descriptor_dim()
	     << endl;
      
      if (descr_size_final!=descriptor_dim()) {
	cerr << msg << "dimensionality mismatch descr_size_final="
	     << descr_size_final << " descriptor_dim()=" << descriptor_dim() 
	     << endl;
	return false;
      }

      vl_fisher_encode(dataout, VL_TYPE_FLOAT, means(), descriptor_dim(),
		       nclusters(), covariances(), priors(),
		       datain, numdata, VL_FISHER_FLAG_IMPROVED) ;
    }

    if (do_vlad) {
      //obs! correct use of pca?

      if (FrameVerbose())
	cout << msg << "vlad encoding " << numdata
	     << " descriptors of size " << descr_size_final << endl;

      vector<vl_uint32> indexes(numdata);
      vector<float> distances(numdata);

      if (kdtree)
	vl_kdforest_query_with_array(kdtree, &indexes[0], 1, numdata, &distances[0], datain);
      else
	vl_kmeans_quantize(kmeans, &indexes[0], &distances[0], datain, numdata);

      vector<float> assignments(numdata*nclusters());
      for (size_t i=0; i<numdata; i++)
	assignments[i * nclusters() + indexes[i]] = 1;
      
      int vlad_flags = VL_VLAD_FLAG_SQUARE_ROOT|VL_VLAD_FLAG_NORMALIZE_COMPONENTS;

      vl_vlad_encode(dataout, VL_TYPE_FLOAT, means(), descriptor_dim(),
		     nclusters(), datain, numdata, &assignments[0],
		     vlad_flags);
    }

    if (renormalize) {
      if (PixelVerbose())
	cout << " RENORM:";
      double mul = 0.0;
      for (size_t j=0; j<enc.size(); j++)
	mul += enc[j]*enc[j];
      if (mul)
	mul = 1.0/sqrt(mul);
      for (size_t j=0; j<enc.size(); j++) {
	if (PixelVerbose())
	  cout << " " << enc[j];
	enc[j] *= mul;
	if (PixelVerbose())
	  cout << "->" << enc[j];
      }
      if (PixelVerbose())
	cout << endl;
    }

    ((VectorData*)GetData(0))->setVector(enc);
    
    return true;
  }

  //===========================================================================

} // namespace picsom

#endif // HAVE_VL_GENERIC_H

// Local Variables:
// mode: font-lock
// End:

