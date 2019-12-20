// -*- C++ -*-  $Id: Trajectory.C,v 1.11 2019/04/10 10:31:09 jormal Exp $	

//  r -d6 trajectory -o densetraj=trajectory-dense-raw-0-27-kmeans1000.bin -o hog=trajectory-dense-raw-28-123-kmeans1000.bin -o hof=trajectory-dense-raw-124-231-kmeans1000.bin -o mbhx=trajectory-dense-raw-232-327-kmeans1000.bin -o mbhy=trajectory-dense-raw-328-423-kmeans1000.bin person01_boxing_d1_uncomp.avi

#include <Trajectory.h>

#ifdef HAVE_DENSE_TRAJECTORY_RELEASE_V1_2_DENSETRACK_H

#include <cox/matrix>
#include <cox/blas>

#include <time.h>

namespace picsom {
  static const char *vcid =
    "$Id: Trajectory.C,v 1.11 2019/04/10 10:31:09 jormal Exp $";

  static Trajectory list_entry(true);

  //===========================================================================

  string Trajectory::Version() const {
    return vcid;
  }

  //===========================================================================

  bool Trajectory::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "Trajectory::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
        cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="densetraj") {
        densetrajname = value;
        i = l.erase(i);
        continue;
      }

      if (key=="hog") {
        hogname = value;
        i = l.erase(i);
        continue;
      }

      if (key=="hof") {
        hofname = value;
        i = l.erase(i);
        continue;
      }

      if (key=="mbhx") {
        mbhxname = value;
        i = l.erase(i);
        continue;
      }

      if (key=="mbhy") {
        mbhyname = value;
        i = l.erase(i);
        continue;
      }

      i++;
    }

    return Feature::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  int Trajectory::printMethodOptions(ostream &str) const {
    /// obs! dunno yet

    int ret = Feature::printMethodOptions(str);

    return ret;
  }

  //===========================================================================

  Feature *Trajectory::Initialize(const string& img, const string& seg,
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

    VideoSegmentExtension(".avi");

    if (IsRawOutMode())
      return this;

    size_t tr_dim = 30;

    if (densetrajname!="" && !densetraj.is_open()) {
      densetraj.open(densetrajname, false, 1.0,
		     bin_data::header::format_float, 32*tr_dim, tr_dim);
      if (MethodVerbose())
	cout << "Trajectory::Initialize() read in densetraj <" << densetrajname
	     << "> " << densetraj.nobjects() << "x" << densetraj.vdim() << endl;
    }
    if (hogname!="" && !hog.is_open()) {
      hog.open(hogname, false, 1.0,
	       bin_data::header::format_float, 32*96, 96);
      if (MethodVerbose())
	cout << "Trajectory::Initialize() read in hog <" << hogname
	     << "> " << hog.nobjects() << "x" << hog.vdim() << endl;
    }

    if (hofname!="" && !hof.is_open()) {
      hof.open(hofname, false, 1.0,
	       bin_data::header::format_float, 32*108, 108);
      if (MethodVerbose())
	cout << "Trajectory::Initialize() read in hof <" << hofname
	     << "> " << hof.nobjects() << "x" << hof.vdim() << endl;
    }

    if (mbhxname!="" && !mbhx.is_open()) {
      mbhx.open(mbhxname, false, 1.0,
		bin_data::header::format_float, 32*96, 96);
      if (MethodVerbose())
	cout << "Trajectory::Initialize() read in mbhx <" << mbhxname
	     << "> " << mbhx.nobjects() << "x" << mbhx.vdim() << endl;
    }

    if (mbhyname!="" && !mbhy.is_open()) {
      mbhy.open(mbhyname, false, 1.0,
		bin_data::header::format_float, 32*96, 96);
      if (MethodVerbose())
	cout << "Trajectory::Initialize() read in mbhy <" << mbhyname
	     << "> " << mbhy.nobjects() << "x" << mbhy.vdim() << endl;
    }

    return this;
  }

  //===========================================================================

  Feature::featureVector Trajectory::CalculateVideoSegment(bool /*print*/) {
    string msg = "Trajectory::CalculateVideoSegment() : ";
    if (FileVerbose())
      cout << msg << "videosegment_file=<" << videosegment_file
	   << ">" << endl;

    bool dense = true;

    if (dense)
      return DenseTrajectory(videosegment_file);

    return featureVector();
  }

  //===========================================================================

  void Trajectory::Desc2Vec(std::vector<float>& desc, DescInfo& descInfo,
			    TrackInfo& trackInfo, vector<float>& res) {
    int tStride = cvFloor(trackInfo.length/descInfo.ntCells);
    float norm = 1./float(tStride);
    int dim = descInfo.dim;
    int pos = 0;
    // printf("[");
    for (int i = 0; i < descInfo.ntCells; i++) {
      std::vector<float> vec(dim);
      for (int t = 0; t < tStride; t++)
	for (int j = 0; j < dim; j++)
	  vec[j] += desc[pos++];
      for (int j = 0; j < dim; j++) {
	res.push_back(vec[j]*norm);
	// printf("%.7f\t", vec[j]*norm);
      }
    }
    // printf("]");
  }

  //===========================================================================

  bool Trajectory::ProcessRawTraj(const vector<float>& f) {
    const size_t tr_dim = 30, vl = tr_dim+3*96+108;
    if (f.size()!=vl) {
      cerr << "Vector dimension is " << f.size() << " when " << vl
	   << " was expected" << endl;
      return false;
    }

    vector<float> d(tr_dim);

    // for (size_t i=2; i<30; i++)
    //   d[i-2] = f[i]-f[i-2];

    for (size_t i=0; i<30; i++)
      d[i] = f[i];
    
    // double l = 0;
    // for (size_t i=0; i<d.size(); i+=2)
    //   l += sqrt(d[i]*d[i]+d[i+1]*d[i+1]);
    
    // for (size_t i=0; i<d.size(); i+=2)
    //   d[i] /= l;

    auto p = f.begin()+30;
    d.insert(d.end(), p, f.end());
    // cout << d.size() << endl;

    if (IsRawOutMode())
      return PrintAndAppendRawVector(d, 0, 0);

    int h = 0;
    size_t dim = 0;
    h = SolveHit(d,   0,  tr_dim, densetraj, dim);
    SetHit(h, densetrajhist, dim);
    if (KeyPointVerbose())
      cout << "Hit in bin " << h << " of densetraj" << endl;

    h = SolveHit(d,  tr_dim, tr_dim+96, hog,       dim);
    SetHit(h, hoghist, dim);
    if (KeyPointVerbose())
      cout << "Hit in bin " << h << " of hog" << endl;

    h = SolveHit(d, tr_dim+96, tr_dim+96+108, hof,       dim);
    SetHit(h, hofhist, dim);
    if (KeyPointVerbose())
      cout << "Hit in bin " << h << " of hof" << endl;

    h = SolveHit(d, tr_dim+96+108, tr_dim+2*96+108, mbhx,      dim);
    SetHit(h, mbhxhist, dim);
    if (KeyPointVerbose())
      cout << "Hit in bin " << h << " of mbhx" << endl;

    h = SolveHit(d, tr_dim+2*96+108, tr_dim+3*96+108, mbhy,      dim);
    SetHit(h, mbhyhist, dim);
    if (KeyPointVerbose())
      cout << "Hit in bin " << h << " of mbhy" << endl;

    return true;
  }
  
  //===========================================================================

  int Trajectory::SolveHit(const vector<float>& vin, size_t b, size_t e,
			   bin_data& d, size_t& dim) {

    if (!d.is_open()) {
      dim = 0;
      return -1;
    }

    dim = d.nobjects();
    vector<float> v(vin.begin()+b, vin.begin()+e);

    int ret = -1;
    double min = numeric_limits<double>::max();
    for (size_t i=0; i<d.nobjects(); i++) {
      vector<float> r = d.get_float(i);
      double dd = cox::blas::euclidean_squared_distance(r, v);
      if (dd<min) {
	min = dd;
	ret = i;
      }
    }

    return ret;
  }
  
  //===========================================================================

  bool Trajectory::SetHit(int hit, vector<size_t>& hist, size_t dim) {
    if (hit<0)
      return true;

    if (hist.size()==0)
      hist = vector<size_t>(dim);

    if (hit>=(int)hist.size()) {
      cerr << "Index error in Trajectory::SetHit()" << endl;
      return false;
    }

    hist[hit]++;

    return true;
  }
  
  //===========================================================================

  bool Trajectory::AppendHist(vector<float>& v,
			      const vector<size_t>& hist, size_t n,
			      size_t l) {

    if (n)
      for (auto i=hist.begin(); i!=hist.end(); i++)
	v.push_back(float(*i)/n);

    else
      for (size_t i=0; i<l; i++)
	v.push_back(0);

    return true;
  }
  
  //===========================================================================

  Feature::featureVector 
  Trajectory::CreateFeatureVector(size_t n) {
    vector<float> v;
    AppendHist(v, densetrajhist, n, densetraj.nobjects());
    AppendHist(v, hoghist,       n, hog.nobjects());
    AppendHist(v, hofhist,       n, hof.nobjects());
    AppendHist(v, mbhxhist,      n, mbhx.nobjects());
    AppendHist(v, mbhyhist,      n, mbhy.nobjects());

    densetrajhist.clear();
    hoghist.clear();
    hofhist.clear();
    mbhxhist.clear();
    mbhyhist.clear();

    if (FileVerbose())
      cout << "Trajectory::CreateFeatureVector() created " << v.size()
	   << "-dimensional features vector from " << n << " samples" << endl;

    return v;
  }
  
  //===========================================================================

  Feature::featureVector Trajectory::DenseTrajectory(const string& f) {
    bool print_all = false, print_traj = false;

    if (FileVerbose())
      cout << "Trajectory::ProcessRawTraj() " << f << endl;
    
    using namespace cv;
    featureVector empty;
    int show_track = 0;

    int argc = 1;
    char **argv = new char*[2];
    argv[0] = (char*)"picsom";
    argv[1] = NULL;

    VideoCapture capture;
    char* video = (char*)f.c_str();
    int flag = arg_parse(argc, argv);
    capture.open(video);

    if (!capture.isOpened()) {
      fprintf(stderr, "Could not initialize capturing..\n");
      return empty;
    }

    int frame_num = 0;
    TrackInfo trackInfo;
    DescInfo hogInfo, hofInfo, mbhInfo;

    InitTrackInfo(&trackInfo, track_length, init_gap);
    InitDescInfo(&hogInfo, 8, false, patch_size, nxy_cell, nt_cell);
    InitDescInfo(&hofInfo, 9, true, patch_size, nxy_cell, nt_cell);
    InitDescInfo(&mbhInfo, 8, false, patch_size, nxy_cell, nt_cell);

    Size down_scale_size(0, 0);
    SeqInfo seqInfo;
    InitSeqInfo(&seqInfo, video);
    if (seqInfo.width>800) {
      size_t w = 720;
      down_scale_size = Size(w, round(float(seqInfo.height)*w/seqInfo.width));
      seqInfo.width  = down_scale_size.width;
      seqInfo.height = down_scale_size.height;
    }

    if (flag)
      seqInfo.length = end_frame - start_frame + 1;

    if (FileVerbose())
      fprintf(stderr, "video size, length: %d, width: %d, height: %d\n",
	      seqInfo.length, seqInfo.width, seqInfo.height);

    if (show_track == 1)
      namedWindow("DenseTrack", 0);

    size_t n_samples = 0;

    Mat image, prev_grey, grey;

    std::vector<float> fscales(0);
    std::vector<Size> sizes(0);

    std::vector<Mat> prev_grey_pyr(0), grey_pyr(0), flow_pyr(0);
    std::vector<Mat> prev_poly_pyr(0), poly_pyr(0); // for optical flow

    std::vector<std::list<Track> > xyScaleTracks;
    int init_counter = 0; // indicate when to detect new feature points
    while (true) {
      Mat frame_orig;
      //int i, j, c;

      // get a new frame
      capture >> frame_orig;
      if (frame_orig.empty())
	break;

      if (frame_num < start_frame || frame_num > end_frame) {
	frame_num++;
	continue;
      }

      Mat frame;
      if (down_scale_size.width) {
	resize(frame_orig, frame, down_scale_size);
	if (false) {
	  static size_t nn = 0;
	  stringstream ss;
	  ss << "frame-" << nn++ << ".jpeg";
	  imwrite(ss.str(), frame);
	}

      } else
	frame = frame_orig;

      if (frame_num == start_frame) {
	image.create(frame.size(), CV_8UC3);
	grey.create(frame.size(), CV_8UC1);
	prev_grey.create(frame.size(), CV_8UC1);

	InitPry(frame, fscales, sizes);

	BuildPry(sizes, CV_8UC1, prev_grey_pyr);
	BuildPry(sizes, CV_8UC1, grey_pyr);

	BuildPry(sizes, CV_32FC2, flow_pyr);
	BuildPry(sizes, CV_32FC(5), prev_poly_pyr);
	BuildPry(sizes, CV_32FC(5), poly_pyr);

	xyScaleTracks.resize(scale_num);

	frame.copyTo(image);
	cvtColor(image, prev_grey, CV_BGR2GRAY);

	for (int iScale = 0; iScale < scale_num; iScale++) {
	  if (iScale == 0)
	    prev_grey.copyTo(prev_grey_pyr[0]);
	  else
	    resize(prev_grey_pyr[iScale-1], prev_grey_pyr[iScale],
		   prev_grey_pyr[iScale].size(), 0, 0, INTER_LINEAR);

	  // dense sampling feature points
	  std::vector<Point2f> points(0);
	  DenseSample(prev_grey_pyr[iScale], points, quality, min_distance);

	  // save the feature points
	  std::list<Track>& tracks = xyScaleTracks[iScale];
	  for (size_t i = 0; i < points.size(); i++)
	    tracks.push_back(Track(points[i], trackInfo, hogInfo, hofInfo, mbhInfo));
	}

	// compute polynomial expansion
	my::FarnebackPolyExpPyr(prev_grey, prev_poly_pyr, fscales, 7, 1.5);

	frame_num++;
	continue;
      }

      init_counter++;
      frame.copyTo(image);
      cvtColor(image, grey, CV_BGR2GRAY);

      // compute optical flow for all scales once
      my::FarnebackPolyExpPyr(grey, poly_pyr, fscales, 7, 1.5);
      my::calcOpticalFlowFarneback(prev_poly_pyr, poly_pyr, flow_pyr, 10, 2);

      for (int iScale = 0; iScale < scale_num; iScale++) {
	if (iScale == 0)
	  grey.copyTo(grey_pyr[0]);
	else
	  resize(grey_pyr[iScale-1], grey_pyr[iScale], grey_pyr[iScale].size(), 0, 0, INTER_LINEAR);

	int width = grey_pyr[iScale].cols;
	int height = grey_pyr[iScale].rows;

	// compute the integral histograms
	DescMat* hogMat = InitDescMat(height+1, width+1, hogInfo.nBins);
	HogComp(prev_grey_pyr[iScale], hogMat->desc, hogInfo);

	DescMat* hofMat = InitDescMat(height+1, width+1, hofInfo.nBins);
	HofComp(flow_pyr[iScale], hofMat->desc, hofInfo);

	DescMat* mbhMatX = InitDescMat(height+1, width+1, mbhInfo.nBins);
	DescMat* mbhMatY = InitDescMat(height+1, width+1, mbhInfo.nBins);
	MbhComp(flow_pyr[iScale], mbhMatX->desc, mbhMatY->desc, mbhInfo);

	// track feature points in each scale separately
	std::list<Track>& tracks = xyScaleTracks[iScale];
	for (std::list<Track>::iterator iTrack = tracks.begin(); iTrack != tracks.end();) {
	  int index = iTrack->index;
	  Point2f prev_point = iTrack->point[index];
	  int x = std::min<int>(std::max<int>(cvRound(prev_point.x), 0), width-1);
	  int y = std::min<int>(std::max<int>(cvRound(prev_point.y), 0), height-1);

	  Point2f point;
	  point.x = prev_point.x + flow_pyr[iScale].ptr<float>(y)[2*x];
	  point.y = prev_point.y + flow_pyr[iScale].ptr<float>(y)[2*x+1];
 
	  if (point.x <= 0 || point.x >= width || point.y <= 0 || point.y >= height) {
	    iTrack = tracks.erase(iTrack);
	    continue;
	  }

	  // get the descriptors for the feature point
	  RectInfo rect;
	  GetRect(prev_point, rect, width, height, hogInfo);
	  GetDesc(hogMat, rect, hogInfo, iTrack->hog, index);
	  GetDesc(hofMat, rect, hofInfo, iTrack->hof, index);
	  GetDesc(mbhMatX, rect, mbhInfo, iTrack->mbhX, index);
	  GetDesc(mbhMatY, rect, mbhInfo, iTrack->mbhY, index);
	  iTrack->addPoint(point);

	  // draw the trajectories at the first scale
	  if (show_track == 1 && iScale == 0)
	    DrawTrack(iTrack->point, iTrack->index, fscales[iScale], image);

	  // if the trajectory achieves the maximal length
	  if (iTrack->index >= trackInfo.length) {
	    std::vector<Point2f> trajectory(trackInfo.length+1);
	    for(int i = 0; i <= trackInfo.length; ++i)
	      trajectory[i] = iTrack->point[i]*fscales[iScale];
				
	    float mean_x(0), mean_y(0), var_x(0), var_y(0), length(0);
	    if (IsValid(trajectory, mean_x, mean_y, var_x, var_y, length)) {
	      if (print_all) {
		printf("%d\t%f\t%f\t%f\t%f\t%f\t%f\t", frame_num, mean_x, mean_y, var_x, var_y,
		       length, fscales[iScale]);

		// for spatio-temporal pyramid
		printf("%f\t", std::min<float>(std::max<float>(mean_x/float(seqInfo.width), 0), 0.999));
		printf("%f\t", std::min<float>(std::max<float>(mean_y/float(seqInfo.height), 0), 0.999));
		printf("%f\t", std::min<float>(std::max<float>((frame_num - trackInfo.length/2.0 -
								start_frame)/float(seqInfo.length), 0), 0.999));
	      }

	      vector<float> rawtraj;
	      // output the trajectory
	      for (int i = 0; i < trackInfo.length; ++i) {
		if (print_all || print_traj)
		  printf("%f\t%f\t", trajectory[i].x,trajectory[i].y);
		rawtraj.push_back(trajectory[i].x);
		rawtraj.push_back(trajectory[i].y);
	      }

	      Desc2Vec(iTrack->hog, hogInfo, trackInfo, rawtraj);
	      Desc2Vec(iTrack->hof, hofInfo, trackInfo, rawtraj);
	      Desc2Vec(iTrack->mbhX, mbhInfo, trackInfo, rawtraj);
	      Desc2Vec(iTrack->mbhY, mbhInfo, trackInfo, rawtraj);
	      n_samples++;

	      if (print_all) {
		PrintDesc(iTrack->hog, hogInfo, trackInfo);
		PrintDesc(iTrack->hof, hofInfo, trackInfo);
		PrintDesc(iTrack->mbhX, mbhInfo, trackInfo);
		PrintDesc(iTrack->mbhY, mbhInfo, trackInfo);
	      }

	      if (print_all || print_traj) 
		printf("\n");

	      ProcessRawTraj(rawtraj);
	    }

	    iTrack = tracks.erase(iTrack);
	    continue;
	  }
	  ++iTrack;
	}
	ReleDescMat(hogMat);
	ReleDescMat(hofMat);
	ReleDescMat(mbhMatX);
	ReleDescMat(mbhMatY);

	if (init_counter != trackInfo.gap)
	  continue;

	// detect new feature points every initGap frames
	std::vector<Point2f> points(0);
	for(std::list<Track>::iterator iTrack = tracks.begin(); iTrack != tracks.end(); iTrack++)
	  points.push_back(iTrack->point[iTrack->index]);

	DenseSample(grey_pyr[iScale], points, quality, min_distance);
	// save the new feature points
	for(size_t i = 0; i < points.size(); i++)
	  tracks.push_back(Track(points[i], trackInfo, hogInfo, hofInfo, mbhInfo));
      }

      init_counter = 0;
      grey.copyTo(prev_grey);
      for (size_t i = 0; i < (size_t)scale_num; i++) {
	grey_pyr[i].copyTo(prev_grey_pyr[i]);
	poly_pyr[i].copyTo(prev_poly_pyr[i]);
      }

      frame_num++;

      if ( show_track == 1 ) {
	imshow( "DenseTrack", image);
	int c = cvWaitKey(3);
	if ((char)c == 27) break;
      }
    }

    if ( show_track == 1 )
      destroyWindow("DenseTrack");

    return CreateFeatureVector(n_samples);
  }

  //===========================================================================

  bool Trajectory::Initialize_pca() {
    // bin_data d(pcaname);

    // cox::matrix<float> m;

    // size_t n = d.nobjects();
    // for (size_t i=0; i<n; i++) {
    //   vector<float> v = d.get_float(i);

    //   if (i==0)
    // 	pca.mean() = v;
    //   else
    // 	pca.base().append_column(v);
    // }

    // if (MethodVerbose())
    //   cout << "Trajectory::Initialize_pca() read in pca <" << gmmname
    // 	   << "> dim in=" << pca.vector_length()
    // 	   << " out=" << pca.base().columns() << endl;

    return true;
  }

  //===========================================================================

  bool Trajectory::Initialize_gmm() {
    // bin_data dat(gmmname);
    // const bin_data::header& h = dat.get_header_copy();
    // size_t draw = h.vdim, dv = (draw-1)/2, nv = 0;
    // if (draw!=dv*2+1)
    //   cerr << "Trajectory::Initialize() dim error with gmm" << endl;

    // vector<float> means, covar, prior;
      
    // for (size_t i=0; true; i++)
    //   if (!dat.exists(i)) 
    // 	break;
    //   else {
    // 	const vector<float> v = dat.get_float(i);
    // 	means.insert(means.end(), &v[0],    &v[dv]);
    // 	covar.insert(covar.end(), &v[dv],   &v[2*dv]);
    // 	prior.insert(prior.end(), &v[2*dv], &v[draw]);
    // 	nv++;
    //   }
      
    // gmm = vl_gmm_new(VL_TYPE_FLOAT, dv, nv);
    // vl_gmm_set_means(      gmm, &*means.begin());
    // vl_gmm_set_covariances(gmm, &*covar.begin());
    // vl_gmm_set_priors(     gmm, &*prior.begin());

    // if (MethodVerbose())
    //   cout << "Trajectory::Initialize_gmm() read in gmm <" << gmmname << "> dim="
    // 	   << descriptor_dim() << " nclust=" << nclusters() << endl;

    // do_fisher = true;

    return true;
  }

  //===========================================================================

  bool Trajectory::Initialize_kmeans() {
    // bin_data dat(kmeansname);
      
    // kmeans = vl_kmeans_new(VL_TYPE_FLOAT, VlDistanceL2);
    // vl_kmeans_set_centers(kmeans, dat.vector_address(0),
    // 			  dat.vdim(), dat.nobjects());

    // if (MethodVerbose())
    //   cout << "Trajectory::Initialize_kmeans() read in kmeans <" << kmeansname
    // 	   << "> dim=" << descriptor_dim() << " nclust=" << nclusters()
    // 	   << endl;

    
    // if (kdtree_ntrees) {
    //   kdtree = vl_kdforest_new(VL_TYPE_FLOAT,  descriptor_dim(), kdtree_ntrees,
    // 			       VlDistanceL2);

    //   vl_kdforest_build(kdtree, nclusters(), means());
    //   vl_kdforest_set_max_num_comparisons(kdtree, kdtree_maxcomps);
    // }

    // do_vlad = true;

    return true;
  }

  //===========================================================================

  bool Trajectory::CalculateCommon(int f, bool all, int l) {
    string msg = "Trajectory::CalculateCommon("+ToStr(f)+","+ToStr(all)+","+
      ToStr(l)+") : ";

    // if (!do_fisher && !do_vlad) {
    //   cerr << msg
    // 	   << "either encoding=fisher or encoding=vlad should be specified"
    // 	   << endl;
    //   return false;
    // }

    // if (!gmm && !kmeans) {
    //   cerr << msg << "either gmm=xxx or kmeans=xxx option should be given"
    // 	   << endl;
    //   return false;
    // }

    cox::tictac::func tt(tics, "Trajectory::CalculateCommon");
    
    // obs! only some parameters here, should be in ProcessOptionsAndRemove()
    // too, also scales and geometry should be made specifiable...
    // bool normalizeSift = false, renormalize = true, flat_window = true;
    // size_t step = 3, binsize = 8;

    EnsureImage();

    int width = Width(true), height = Height(true);

    if (FrameVerbose())
      cout << msg+"wxh="
	   << width << "x" << height << "=" << width*height << endl;

    vector<float> rgbcoeff { 0.2989, 0.5870, 0.1140 };

    imagedata idata = CurrentFrame();
    idata.convert(imagedata::pixeldata_float);
    idata.force_one_channel(rgbcoeff);

    // vector<float> dsift;
    // size_t descr_size_orig = 0, descr_size_final = 0;
    // vector<float> scales { 1.0000, 0.7071, 0.5000, 0.3536, 0.2500 };
    // // vector<float> scales { 1.0000 };
    // for (size_t i=0; i<scales.size(); i++) {
    //   if (KeyPointVerbose())
    // 	cout << "Starting vl_dsift_process() in scale " << scales[i] << endl;
      
    //   imagedata simg = idata;
    //   if (scales[i]!=1) {
    // 	scalinginfo si(simg.width(), simg.height(),
    // 		       (int)floor(scales[i]*simg.width()+0.5),
    // 		       (int)floor(scales[i]*simg.height()+0.5));
    // 	simg.rescale(si, 1);
    //   }

    //   // VlDsiftFilter *sf = vl_dsift_new(simg.width(), simg.height());
    //   // VlDsiftFilter *sf = vl_dsift_new_basic(simg.width(), simg.height(),
    //   // 					     step, binsize);

    //   // opts.scales = logspace(log10(1), log10(.25), 5) ;
    //   // void vl_dsift_set_bounds	(	VlDsiftFilter * 	self,
    //   // 					int 	minX,
    //   // 					int 	minY,
    //   // 					int 	maxX,
    //   // 					int 	maxY 
    //   // 					);	
      
    //   // VlDsiftDescriptorGeometry geom = { 8, 4, 4, 0, 0 };
    //   // vl_dsift_set_geometry(sf, &geom);
      
    //   //vl_dsift_set_steps(sf, 3, 3);

    //   //vl_dsift_set_window_size(sf, 8);

    //   vl_dsift_set_flat_window(sf, flat_window); // aka fast in matlab

    //   vector<float> imgvec = simg.get_float();
    //   const float *img_fp = &imgvec[0];
    //   // cout << "IMAGE = " << img_fp[0] << " " << img_fp[1] << " "
    //   //      << img_fp[2] << " ... " << img_fp[41] << endl;

    //   vl_dsift_process(sf, img_fp);
      
    //   // if opts.rootSift // false
    //   // 		descrs{si} = sqrt(descrs{si}) ;
    //   // end
    //   // 	if opts.normalizeSift //true
    //   // 		  descrs{si} = snorm(descrs{si}) ;
    //   // end

    //   descr_size_orig = sf->descrSize;
    //   size_t nf = sf->numFrames;
    //   const VlDsiftKeypoint *k = sf->frames;
    //   float *d  = sf->descrs;
      
    //   if (KeyPointVerbose())
    // 	cout << "  found " << sf->numFrames << " 'frames' in "
    // 	     << simg.info() << endl
    // 	     << "  descriptor dim " << descr_size_orig << endl;
      
    //   if (PixelVerbose())
    // 	for (size_t i=0; i<nf; i++) {
    // 	  cout << "  i=" << i << " x=" << k[i].x << " y=" << k[i].y
    // 	       << " s=" << k[i].s << " norm=" << k[i].norm;
    // 	  if (FullVerbose()) {
    // 	    cout << " RAW";
    // 	    for (size_t j=0; j<descr_size_orig; j++)
    // 	      cout << " " << d[i*descr_size_orig+j];
    // 	  }
    // 	  cout << endl;
    // 	}

    //   if (normalizeSift) {
    // 	for (size_t i=0; i<nf; i++) {
    // 	  if (PixelVerbose())
    // 	    cout << "  i=" << i << " x=" << k[i].x << " y=" << k[i].y
    // 		 << " s=" << k[i].s << " norm=" << k[i].norm;
    // 	  double mul = 0.0;
    // 	  for (size_t j=0; j<descr_size_orig; j++)
    // 	    mul += d[i*descr_size_orig+j]*d[i*descr_size_orig+j];
    // 	  if (mul)
    // 	    mul = 1.0/sqrt(mul);
    // 	  if (FullVerbose())
    // 	    cout << " NORM";
    // 	  for (size_t j=0; j<descr_size_orig; j++) {
    // 	    d[i*descr_size_orig+j] *= mul;
    // 	    if (FullVerbose())
    // 	      cout << " " << d[i*descr_size_orig+j];
    // 	  }
    // 	  if (PixelVerbose())
    // 	    cout << endl;
    // 	}
    //   }

    //   if (!pca.vector_length()) {
    // 	dsift.insert(dsift.end(), d, d+nf*descr_size_orig);
    // 	descr_size_final = descr_size_orig;

    //   } else {
    // 	for (size_t i=0; i<nf; i++) {
    // 	  vector<float> vin(d+i*descr_size_orig, d+(i+1)*descr_size_orig);
    // 	  vector<float> vout = pca.projection_coeff(vin);
    // 	  dsift.insert(dsift.end(), vout.begin(), vout.end());
    // 	}
    // 	descr_size_final = pca.base_size();
    //   }
	  
    //   vl_dsift_delete(sf);
    // }

    // size_t numdata = dsift.size()/descr_size_final;
    // const float *datain = &dsift[0];

    // vector<float> enc((do_fisher?2:1)*descriptor_dim()*nclusters());
    // float *dataout = &enc[0];
      
    // if (do_fisher) {
    //   if (FrameVerbose())
    // 	cout << msg << "fisher encoding " << numdata
    // 	     << " descriptors of size " << descr_size_orig
    // 	     << " => " << descr_size_final
    // 	     << " with gmm dimensionality " << descriptor_dim()
    // 	     << endl;
      
    //   if (descr_size_final!=descriptor_dim()) {
    // 	cerr << msg << "dimensionality mismatch descr_size_final="
    // 	     << descr_size_final << " descriptor_dim()=" << descriptor_dim() 
    // 	     << endl;
    // 	return false;
    //   }

    //   // vl_fisher_encode(dataout, VL_TYPE_FLOAT, means(), descriptor_dim(),
    //   // 		       nclusters(), covariances(), priors(),
    //   // 		       datain, numdata, VL_FISHER_FLAG_IMPROVED) ;
    // }

    // if (do_vlad) {
    //   //obs! correct use of pca?

    //   if (FrameVerbose())
    // 	cout << msg << "vlad encoding " << numdata
    // 	     << " descriptors of size " << descr_size_final << endl;

    //   vector<vl_uint32> indexes(numdata);
    //   vector<float> distances(numdata);

    //   if (kdtree)
    // 	vl_kdforest_query_with_array(kdtree, &indexes[0], 1, numdata, &distances[0], datain);
    //   else
    // 	vl_kmeans_quantize(kmeans, &indexes[0], &distances[0], datain, numdata);

    //   vector<float> assignments(numdata*nclusters());
    //   for (size_t i=0; i<numdata; i++)
    // 	assignments[i * nclusters() + indexes[i]] = 1;
      
    //   // int vlad_flags = VL_VLAD_FLAG_SQUARE_ROOT|VL_VLAD_FLAG_NORMALIZE_COMPONENTS;

    //   // vl_vlad_encode(dataout, VL_TYPE_FLOAT, means(), descriptor_dim(),
    //   // 		     nclusters(), datain, numdata, &assignments[0],
    //   // 		     vlad_flags);
    // }

    // if (renormalize) {
    //   if (PixelVerbose())
    // 	cout << " RENORM:";
    //   double mul = 0.0;
    //   for (size_t j=0; j<enc.size(); j++)
    // 	mul += enc[j]*enc[j];
    //   if (mul)
    // 	mul = 1.0/sqrt(mul);
    //   for (size_t j=0; j<enc.size(); j++) {
    // 	if (PixelVerbose())
    // 	  cout << " " << enc[j];
    // 	enc[j] *= mul;
    // 	if (PixelVerbose())
    // 	  cout << "->" << enc[j];
    //   }
    //   if (PixelVerbose())
    // 	cout << endl;
    // }

    // ((VectorData*)GetData(0))->setVector(enc);
    
    return true;
  }

  //===========================================================================

} // namespace picsom

#endif // HAVE_DENSE_TRAJECTORY_RELEASE_V1_2_DENSETRACK_H

// Local Variables:
// mode: font-lock
// End:

