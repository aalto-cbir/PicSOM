// -*- C++ -*-  $Id: OCVLbp.C,v 1.22 2017/03/10 09:48:11 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)

#include <OCVLbp.h>

#include <algorithm>
#include <iostream>
#include <fstream>

namespace picsom {
  static const string vcid =
    "$Id: OCVLbp.C,v 1.22 2017/03/10 09:48:11 jormal Exp $";

  static OCVLbp list_entry(true);
  
  /////////////////////////////////////////////////////////////////////////////

  string OCVLbp::Version() const {
    return vcid;
  }

  /////////////////////////////////////////////////////////////////////////////

  int OCVLbp::printMethodOptions(ostream &str) const {
    int ret = 0;

    // str << "crop=X               "
    //     << "crop X pixels around the image (default: X=0)" << endl;
    // str << "crop=X,Y             "
    //     << "crop X pixels from x-axis, Y from y-axis" << endl;
    // str << "crop=X0,X1,Y0,Y1     "
    //     << "crop X0 and X1 pixels from x-axis (left and right), Y0 and Y1 from y-axis" 
    // 	<< endl;
    str << "cslbp=X              "
	<< "CS-LBP (Center-Symmetric LBP) with threshold X (good values for X are in [0,0.02])"
	<< endl; 
    str << "nbh=X,Y              "
        << "use circular neighborhood with X neighbors and radius of Y" << endl;
    str << "map=X                "
        << "mapping for LBP codes. Currently only X=u2 for uniform patterns is supported." 
	<< endl;
    str << "blocks=X,Y,...       "
        << "blocks to calculate LBPs in. Currently supported are:" << endl
	<< "                     "
	<< "1x1,2x2,2x2s,3x3,3x3s,4x4,4x4s,5x5,6x6,7x7,8x8,9x9,10x10,8x15,fgbg,pyramid4,pyramid8" 
	<< endl;
    str << "overlap=1            "
	<< "overlap blocks on non-borders" << endl; 
    str << "normhist=X           "
        << "normalize all histograms to sum to one. X=1 normal, X=2 bins capped to 0.2" 
	<< endl;    
    str << "funnel=modelfile NOT USED "
        << "perform funneling using model from modelfile"<< endl;
    str << "illnorm=X   NOT USED "
        << "perform illumination normalization. X=1: crop before, X=2 crop after" 
	<< endl;
    str << "imgout=1             "
        << "write cropped image to disk as \"out.jpg\"" << endl;    

    return ret+6;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::InitializeLbp() {
    options.crop.clear();
    options.write_image = false;    

    options.lbptype = LBP;
    options.cslbp_threshold = 0.0;
      
    options.nneighbors = 8;
    options.neighborhood = LBP_8NEIGHBORS;
    options.radius = 2.0;

    options.overlapping_blocks = false;

    options.mapping = LBP_MAPPING_NONE;
    lbp_map.mapsize = 0;

    options.normalize_histogram = HISTNORM_NONE;
    
    options.do_funneling = false;    
    funneling_modelfile = "";

    options.do_illnorm = false;
    options.cropfirst = true;

    spoints_min = Point2d(0.0,0.0);
    spoints_max = Point2d(0.0,0.0);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::ProcessOptionsAndRemove(list<string>& opts) {
    for (list<string>::iterator i = opts.begin(); i!=opts.end();) {
      string key, value;
      SplitOptionString(*i, key, value);

      // if (key=="crop") {
      // 	vector<string> cropvals = SplitInCommasObeyParentheses(value);
      // 	switch (cropvals.size()) {
      // 	case (1):
      // 	  options.crop.set(atoi(cropvals[0].c_str()));
      // 	  break;
      // 	case (2):
      // 	  options.crop.set(atoi(cropvals[0].c_str()),atoi(cropvals[1].c_str()));
      // 	  break;
      // 	case (4):
      // 	  options.crop.set(atoi(cropvals[0].c_str()),atoi(cropvals[1].c_str()),
      // 			   atoi(cropvals[2].c_str()),atoi(cropvals[3].c_str()));
      // 	  break;
      // 	default:
      // 	  cerr << "OCVLbp(): Parsing option \"crop\" failed: value=\"" 
      // 	       << value << "\"" << endl;
      // 	  return false;
      // 	}

      // } else 

      // obs! this and "blocks" in Caffe should be harmonized and
      // moved to Feature
      if (key=="blocks") {
	vector<string> blockvals = SplitInCommasObeyParentheses(value);
	vector<string>::const_iterator i;
	for (i = blockvals.begin(); i!=blockvals.end(); i++) {
	  if (*i=="1x1" || *i=="full") {
	    blocktypes.push_back(BLOCKS_1x1); 
	  } else if (*i=="2x2") {
	    blocktypes.push_back(BLOCKS_2x2); 
	  } else if (*i=="2x2s") { 
	    blocktypes.push_back(BLOCKS_2x2); 
	    blocktypes.push_back(BLOCKS_2x2_SPLITS);
	  } else if (*i=="3x3") {
	    blocktypes.push_back(BLOCKS_3x3); 
	  } else if (*i=="3x3s") { 
	    blocktypes.push_back(BLOCKS_3x3); 
	    blocktypes.push_back(BLOCKS_3x3_SPLITS);
	  } else if (*i=="4x4") {
	    blocktypes.push_back(BLOCKS_4x4); 
	  } else if (*i=="4x4s") { 
	    blocktypes.push_back(BLOCKS_4x4); 
	    blocktypes.push_back(BLOCKS_4x4_SPLITS);
	  } else if (*i=="5x5") {
	    blocktypes.push_back(BLOCKS_5x5); 
	  } else if (*i=="6x6") {
	    blocktypes.push_back(BLOCKS_6x6); 
	  } else if (*i=="7x7") {
	    blocktypes.push_back(BLOCKS_7x7); 
	  } else if (*i=="8x8") {
	    blocktypes.push_back(BLOCKS_8x8); 
	  } else if (*i=="9x9") {
	    blocktypes.push_back(BLOCKS_9x9); 
	  } else if (*i=="10x10") {
	    blocktypes.push_back(BLOCKS_10x10); 
	  } else if (*i=="8x15") {
	    blocktypes.push_back(BLOCKS_8x15); 
	  } else if (*i=="pyramid4") {
	    blocktypes.push_back(BLOCKS_1x1); 
	    blocktypes.push_back(BLOCKS_2x2);
	    blocktypes.push_back(BLOCKS_4x4);
	  } else if (*i=="pyramid8") {
	    blocktypes.push_back(BLOCKS_1x1); 
	    blocktypes.push_back(BLOCKS_2x2);
	    blocktypes.push_back(BLOCKS_4x4);
	    blocktypes.push_back(BLOCKS_8x8);
	  } else if (*i=="fgbg") {
	    blocktypes.push_back(BLOCKS_FGBG); 
	  } else {
	    cerr << "OCVLbp(): Parsing option \"blocks\" failed: value=\"" 
		 << value << "\"" << endl;
	    return false;
	  }
	}

      } else if (key=="nbh") {
	options.neighborhood = LBP_CIRCULAR;
	vector<string> nbhvals = SplitInCommasObeyParentheses(value);
	if (nbhvals.size() == 2) {
	  options.nneighbors = atoi(nbhvals[0].c_str());
	  options.radius     = atof(nbhvals[1].c_str());
	  if (options.nneighbors>8) {
	    cerr << "OCVLbp(): Maximum number of neighbors is currently 8" 
		 " due to use of CV_8UC1-type matrices. Should be fixed."
		 << endl;
	    return false;
	  }
	    
	} else {
	  cerr << "OCVLbp(): Parsing option \"nbh\" failed: value=\"" 
	       << value << "\"" << endl;
	  return false;
	}	  

      } else if (key=="map") {
	if (value=="u2")
	  options.mapping = LBP_MAPPING_UNIFORM2;
	else {
	  cerr << "OCVLbp(): Unsupported value for option \"map\": value=\"" 
	       << value << "\"" << endl;
	  return false;
	}

      } else if (key == "normhist")
	if (atoi(value.c_str())==2) {
	  options.normalize_histogram = HISTNORM_CAPPED;
	} else {
	  options.normalize_histogram = HISTNORM_NORMAL;
	}

      else if (key == "cslbp") {
	options.lbptype = CSLBP;
	options.cslbp_threshold = atof(value.c_str());	

      } else if (key == "overlap") 
	options.overlapping_blocks = true;

      else if (key == "imgout")
        options.write_image = true;

      else if (key == "funnel") {
        options.do_funneling = true;
	funneling_modelfile = value;

      } else if (key == "illnorm") {
        options.do_illnorm = true;
	if (atoi(value.c_str())==2)
	  options.cropfirst = false;

      } else { // else continue without erasing option from list
        i++;
        continue;
      }

      i = opts.erase(i);
    }

    return Feature::ProcessOptionsAndRemove(opts);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::CalculateOpenCV(int /*f*/, int l) {
    static const string msg = "OCVLbp::CalculateOpenCV() : ";
    featureVector fv;

    if (LabelVerbose())
      cout << msg << "starting" << endl;

    bool ok = true;

    if (spoints.empty())
      CreateSPoints();

    if (!ok || spoints.empty() || (spoints.size() != options.nneighbors)) {
      cerr << msg << "Missing or wrong pixel neighborhood." << endl;
      return false;
    }

    if (options.lbptype == CSLBP) {
      if (options.neighborhood != LBP_CIRCULAR) {
	cerr << msg << "CS-LBP requires (currently) a circular neighborhood" 
	     << endl;
	return false;
      } else if (options.mapping != LBP_MAPPING_NONE) {
	cerr << msg << "CS-LBP is not compatible with any mappings" 
	     << endl;
	return false;
      }
    }

    if ((options.mapping != LBP_MAPPING_NONE) && lbp_map.mapsize == 0)
      CreateMap();

    // lbp_image =
    //   ocv_image_(Rect(options.crop.x0, options.crop.y0, 
    // 		      ocv_image_.cols-options.crop.x0-options.crop.x1,
    // 		      ocv_image_.rows-options.crop.y0-options.crop.y1));    

    lbp_image = ocv_image_;

    if (options.write_image) {
      string sn;
      if ((size_t)l<RegionDescriptorCount())
	sn = RegionDescriptor(l).first;
      else
	sn = ToStr(l);
      //string oname = "lbp-"+BaseFileName()+"-"+ToStr(f)+"-"+ToStr(l)+".jpg";
      string oname = "lbp-"+BaseFileName()+"_"+sn+".jpg";
      imwrite(oname, lbp_image);
    }

    if (blocktypes.empty())
      blocktypes.push_back(BLOCKS_1x1);
    CreateBlocks();

    MatND storedhist;
    list<pair<Rect,lbp_block_type> >::const_iterator i;
    for (i = lbp_blocks.begin(); i!=lbp_blocks.end(); i++) {
      MatND hist;
      Mat blockimg = lbp_image(i->first); 

      CalculateLbp(blockimg, hist);

      if (i->second == BLOCK_STORE)
	storedhist = hist;
      else if (i->second == BLOCK_SUBTRACT)
	hist -= storedhist;

      if (options.normalize_histogram != HISTNORM_NONE) {
	Scalar sumhist = sum(hist);
	hist /= sumhist[0];
      }
      if (options.normalize_histogram == HISTNORM_CAPPED) {
	for (int ii=0; ii<hist.rows; ii++)
	  if (hist.at<float>(ii)>0.2)
	    hist.at<float>(ii) = 0.2;
	Scalar sumhist = sum(hist);
	hist /= sumhist[0];
      }

      for (int ii=0; ii<hist.rows; ii++)
	fv.push_back(hist.at<float>(ii));
    }

    SetVectorData(l, fv);

    if (LabelVerbose())
      cout << msg << "ending" << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::CreateSPoints() {
    static const string msg = "OCVLbp::CreateSPoints(): ";
    
    if (options.neighborhood == LBP_8NEIGHBORS) {
      spoints.push_back(Point2d(-1.0,-1.0));
      spoints.push_back(Point2d( 0.0,-1.0));
      spoints.push_back(Point2d( 1.0,-1.0));
      spoints.push_back(Point2d(-1.0, 0.0));
      spoints.push_back(Point2d( 1.0, 0.0));
      spoints.push_back(Point2d(-1.0, 1.0));
      spoints.push_back(Point2d( 0.0, 1.0));
      spoints.push_back(Point2d( 1.0, 1.0));

    } else if (options.neighborhood == LBP_CIRCULAR) {
      double a = 2*CV_PI/options.nneighbors;
	
      for (size_t i = 0; i<options.nneighbors; i++)
	spoints.push_back(Point2d(options.radius*cos(i*a), 
				  -options.radius*sin(i*a)));
      
    } else {
      cerr << msg << "Unsupported neighborhood type" 
	   << endl;
      return false;
    }

    vector<Point2d>::const_iterator i;
    for (i = spoints.begin(); i!=spoints.end(); i++) {
      if (i->x < spoints_min.x) spoints_min.x = i->x;
      if (i->y < spoints_min.y) spoints_min.y = i->y;
      if (i->x > spoints_max.x) spoints_max.x = i->x;
      if (i->y > spoints_max.y) spoints_max.y = i->y;
    }
    //cout << "MIN: " << spoints_min.x << "," << spoints_min.y
    //	 << ", MAX: " << spoints_max.x << "," << spoints_max.y
    //	 << endl;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::CreateMap() {
    static const string msg = "OCVLbp::CreateMap(): ";

    unsigned int idx = 0;
    
    if (options.mapping == LBP_MAPPING_UNIFORM2) {
      lbp_map.mapsize = options.nneighbors*(options.nneighbors-1) + 3;
      unsigned int pw = pow(2,options.nneighbors);
      for (unsigned int i=0; i<pw; i++) {
	unsigned int j = (i << 1);
	if (j>pw)
	  j = (j%pw)+1; 
	unsigned int t = (i ^ j);
	size_t numt;
	//cout << "i=" << i << ", j=" << j << ", t=" << t;
	for (numt = 0; t; t >>= 1)
	  numt += t & 1;
	//cout << ", numt=" << numt << ", idx=" << idx << endl; 
	if (numt <= 2) {
	  lbp_map.mapping[i] = idx;
	  idx++;
	} else {
	  lbp_map.mapping[i] = lbp_map.mapsize-1;
	}
      }

    } else {
      cerr << msg << "Unsupported mapping type" << endl;
      return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::CreateBlocks() {
    lbp_blocks.clear();
    
    // if (lbp_blocks.size())
    //   return true;

    list<lbp_blocks_type>::const_iterator i;
    for (i = blocktypes.begin(); i!=blocktypes.end(); i++) {
      switch (*i) {
      default:
      case (BLOCKS_1x1):
	lbp_blocks.push_back(make_pair(Rect(0,0,lbp_image.cols,lbp_image.rows),
				       BLOCK_NORMAL));
	break;
      case (BLOCKS_2x2): {
	CreateBlocks(2,2);
	break;
      }
      case (BLOCKS_2x2_SPLITS): {
	CreateBlockSplits(2,2);
	break;
      }
      case (BLOCKS_3x3): {
	CreateBlocks(3,3);
	break;
      }
      case (BLOCKS_3x3_SPLITS):{
	CreateBlockSplits(3,3);
	break;
      }
      case (BLOCKS_4x4): {
	CreateBlocks(4,4);
	break;
      }
      case (BLOCKS_4x4_SPLITS): {
	CreateBlockSplits(4,4);
	break;
      }
      case (BLOCKS_5x5): {
	CreateBlocks(5,5);
	break;
      }
      case (BLOCKS_6x6): {
	CreateBlocks(6,6);
	break;
      }
      case (BLOCKS_7x7): {
	CreateBlocks(7,7);
	break;
      }
      case (BLOCKS_8x8): {
	CreateBlocks(8,8);
	break;
      }
      case (BLOCKS_9x9): {
	CreateBlocks(9,9);
	break;
      }
      case (BLOCKS_10x10): {
	CreateBlocks(10,10);
	break;
      }
      case (BLOCKS_8x15): {
	CreateBlocks(8,15);
	break;
      }
      case (BLOCKS_FGBG):
	int blockwidth  = 2*lbp_image.cols/3;
	int blockheight = 2*lbp_image.rows/3;
	lbp_blocks.push_back(make_pair(Rect(blockwidth/4,blockheight/4,
					    blockwidth,blockheight),
				       BLOCK_STORE));
	lbp_blocks.push_back(make_pair(Rect(0,0,lbp_image.cols,lbp_image.rows),
				       BLOCK_SUBTRACT));
	break;
      }    
    }
    return true;
  }	

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::CreateBlocks(int cols, int rows) {
    Size gridblock(lbp_image.cols/cols, lbp_image.rows/rows);

    for (int x=0; x<cols; x++)
      for (int y=0; y<rows; y++) {
	Point block_tl(x*gridblock.width, y*gridblock.height);
	Size block = gridblock;

	if (options.overlapping_blocks) {
	  if (block_tl.x + spoints_min.x > 0) {
	    block_tl.x  += spoints_min.x;
	    block.width -= spoints_min.x;
	  }
	  if (block_tl.y + spoints_min.y > 0) {
	    block_tl.y   += spoints_min.y;
	    block.height -= spoints_min.y;
	  }
	  if (block_tl.x + block.width + spoints_max.x < lbp_image.cols)
	    block.width  += spoints_max.x;
	  if (block_tl.y + block.height + spoints_max.y < lbp_image.rows)
	    block.height += spoints_max.y;
	}

	if (0)
	  cout << "CreateBlocks(" << cols << "," << rows << "): Adding block: tl=(" 
	       << block_tl.x << "," << block_tl.y << "), width=" << block.width 
	       << ", height=" << block.height << ", img cols=" << lbp_image.cols 
	       << ", img rows=" << lbp_image.rows << endl; 

	lbp_blocks.push_back(make_pair(Rect(block_tl, block), BLOCK_NORMAL));
      }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::CreateBlockSplits(int cols, int rows) {
    Size gridblock(lbp_image.cols/cols, lbp_image.rows/rows);
    Size block = gridblock;
    
    // Here we assume that block overlapping is always safe.
    if (options.overlapping_blocks) {
      block.width  += spoints_max.x; block.width  -= spoints_min.x;
      block.height += spoints_max.y; block.height -= spoints_min.y;
    }

    for (int x=0; x<(cols-1); x++)
      for (int y=0; y<(rows-1); y++) {
	Point block_tl((2*x+1)*gridblock.width/2, (2*y+1)*gridblock.height/2);
	if (options.overlapping_blocks) {
	  block_tl.x += spoints_min.x;
	  block_tl.y += spoints_min.y;
	}

	if (0)
	  cout << "CreateBlockSplits(" << cols << "," << rows << "): Adding block: tl=(" 
	       << block_tl.x << "," << block_tl.y << "), width=" << block.width 
	       << ", height=" << block.height << ", img cols=" << lbp_image.cols 
	       << ", img rows=" << lbp_image.rows << endl; 

	lbp_blocks.push_back(make_pair(Rect(block_tl, block), BLOCK_NORMAL));
      }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVLbp::CalculateLbp(const Mat &img, MatND &lbphist) {
    static const string msg = "OCVLbp::CalculateLbp(): ";

    Mat grayimg, grayimgd;
    cvtColor(img, grayimg, CV_RGB2GRAY);
    grayimg.convertTo(grayimgd, CV_32FC1);

    // Block size, each LBP code is computed within a block of size bsize
    Size bsize(ceil(MAX(spoints_max.x,0))-floor(MIN(spoints_min.x,0))+1, 
	       ceil(MAX(spoints_max.y,0))-floor(MIN(spoints_min.y,0))+1);

    // Coordinates of origin (0,0) in the block
    Point origin(-floor(MIN(spoints_min.x,0)), -floor(MIN(spoints_min.y,0)));
    
    if (grayimg.cols < bsize.width || grayimg.rows < bsize.height) {
      cerr << msg << "Too small input image." << endl;
      return false;
    }

    // Creating the center pixel matrices imgC and imgdC
    Size csize(grayimg.cols-bsize.width+1, grayimg.rows-bsize.height+1);
    Mat imgC  = grayimg( Rect(origin.x, origin.y, csize.width, csize.height)); 
    Mat imgdC = grayimgd(Rect(origin.x, origin.y, csize.width, csize.height)); 

    Mat result = Mat::zeros(csize, CV_8UC1);

    size_t loop_max = spoints.size();
    if (options.lbptype == CSLBP)
      loop_max /= 2;

    for (size_t i = 0; i<loop_max; i++) {
      Point2d s(spoints.at(i).x+origin.x, spoints.at(i).y+origin.y);
      Point rs(round(s.x), round(s.y));

      // these are for CSLBP:
      Point2d s2(spoints.at((i+loop_max)%spoints.size()).x+origin.x, 
		 spoints.at((i+loop_max)%spoints.size()).y+origin.y);
      Point rs2(round(s.x), round(s.y));

      Mat imgD;

      //Check if interpolation is needed.
      if (options.lbptype != CSLBP &&
	  ((abs((double)s.x - rs.x) < 1e-6) &&
	   (abs((double)s.y - rs.y) < 1e-6))) {
	// Interpolation is not needed, using original datatypes
	//cout << "Interpolation is not needed, using original datatypes"
	//     << endl;

	Mat imgN = grayimg(Rect(rs.x, rs.y, csize.width, csize.height));
	imgD = imgN >= imgC;

      } else {
	// Interpolation needed, using CV_32FC1 images 
	//cout << "Interpolation needed, using CV_32FC1 images" << endl;

	Mat imgdN(csize, CV_32FC1);
	InterpolatedImage(imgdN, grayimgd, s);
	// double minval, maxval;
	// Point minloc, maxloc;
	// minMaxLoc(imgdN, &minval, &maxval, &minloc, &maxloc);
	if (options.lbptype == CSLBP) {
	  Mat imgdN2(csize, CV_32FC1);
	  InterpolatedImage(imgdN2, grayimgd, s2);
	  imgD = imgdN >= (imgdN2 + options.cslbp_threshold*255);
	} else
	  imgD = imgdN >= imgdC;

      }

      imgD = imgD/255;

      int pw = pow(2, i);
      result = result + pw*imgD;
      
    }
    //imwrite("result.bmp", result);

    int hist_bins = pow(2, loop_max);

    if (lbp_map.mapsize > 0) {
      hist_bins = lbp_map.mapsize;
      for (int yy=0; yy<result.rows; yy++)
	for (int xx=0; xx<result.cols; xx++)
	  result.at<unsigned char>(yy,xx) = 
	    lbp_map.mapping[result.at<unsigned char>(yy,xx)];      
    }

    int histSize[1];
    float hranges[2];
    const float* ranges[1];
    int channels[1];

    histSize[0] = hist_bins;
    hranges[0] = 0.0;
    hranges[1] = hist_bins;
    ranges[0] = hranges;
    channels[0]= 0;

    calcHist(&result, 1, channels, Mat(), // do not use mask
	     lbphist, 1, histSize, ranges,
	     true, // the histogram is uniform
	     false);
    
    //for (int ii=0; ii<lbphist.rows; ii++)
    //  cout << lbphist.at<float>(ii) << " ";
    //cout << endl;

    return true;

  }

  /////////////////////////////////////////////////////////////////////////////

  void OCVLbp::InterpolatedImage(Mat &img, const Mat &srcimg,
				 const Point2d &s) {

    Point fs(floor(s.x), floor(s.y));
    Point cs(ceil(s.x),  ceil(s.y));
    
    // Calculate the interpolation weights.
    Point2d t = s - Point2d(fs.x,fs.y);
    double w1 = (1 - t.x) * (1 - t.y);
    double w2 =      t.x  * (1 - t.y);
    double w3 = (1 - t.x) *      t.y ;
    double w4 =      t.x  *      t.y ;

    // Compute interpolated pixel values
    img = w1*srcimg(Rect(fs.x, fs.y, img.cols, img.rows)) + 
      w2*srcimg(Rect(cs.x, fs.y, img.cols, img.rows)) + 
      w3*srcimg(Rect(fs.x, cs.y, img.cols, img.rows)) + 
      w4*srcimg(Rect(cs.x, cs.y, img.cols, img.rows));    
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV
