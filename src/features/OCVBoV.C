// -*- C++ -*-  $Id: OCVBoV.C,v 1.3 2017/03/10 09:10:16 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#if defined(HAVE_OPENCV2_CORE_CORE_HPP) && defined(PICSOM_USE_OPENCV)

#include <OCVBoV.h>

namespace picsom {
  static const string vcid =
    "$Id: OCVBoV.C,v 1.3 2017/03/10 09:10:16 jormal Exp $";

  static OCVBoV list_entry(true);

  Ptr<FeatureDetector>           OCVBoV::fd = NULL;
  Ptr<DescriptorExtractor>       OCVBoV::de = NULL;
  Ptr<DescriptorMatcher>         OCVBoV::dm = NULL;
  Ptr<BOWImgDescriptorExtractor> OCVBoV::be = NULL;

  Mat OCVBoV::vocabulary;

  size_t OCVBoV::nimg = 0;
  Mat OCVBoV::traindescriptors;

  /////////////////////////////////////////////////////////////////////////////

  string OCVBoV::Version() const {
    return vcid;
  }

  /////////////////////////////////////////////////////////////////////////////

  int OCVBoV::printMethodOptions(ostream &str) const {
    //int ret = Feature::printMethodOptions(str);
    int ret = 0;
    str << "detector=            "
        << "[Grid|Pyramid|Dynamic]FAST|STAR|SIFT|SURF|ORB|MSER|GFTT|HARRIS|Dense|SimpleBlob" 
	<< endl;
    str << "descriptor=          "
        << "[Opponent]SIFT|SURF|ORB|BRIEF" << endl;
    str << "matcher=             "
        << "BruteForce|BruteForce-L1|BruteForce-Hamming|BruteForce-HammingLUT|FlannBased"
	<< endl;
    str << "vocabulary=X         "
        << "use/create a BoV vocabulary from/to file X" << endl;
    str << "createvocabulary=X,Y,Z "
        << "create a vocabulary with X clusters, and using max Z vectors from Y images" 
	<< endl;

    return ret+5;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVBoV::ProcessOptionsAndRemove(list<string>& opts) {
    for (list<string>::iterator i = opts.begin(); i!=opts.end();) {
      string key, value;
      SplitOptionString(*i, key, value);

      if (key=="detector")
        detectorName = value;
      else if (key == "descriptor")
        descriptorName = value;
      else if (key == "matcher")
        matcherName = value;
      else if (key == "vocabulary")
        vocabularyName = value;
      else if (key == "createvocabulary") {
	createvocabulary = true;
	vector<string> vparams = SplitInCommasObeyParentheses(value);	
	if (vparams.size() != 3) {
	  cerr << "OCVCentrist(): Parsing option \"createvocabulary\" failed: value=\"" 
	       << value << "\"" << endl;
	  return false;
	}
	nclusters     = atoi(vparams[0].c_str());
	ntrainvectors = atoi(vparams[1].c_str());
	ntrainimages  = atoi(vparams[2].c_str());
      }
      else { // else continue without erasing option from list
        i++;
        continue;
      }

      i = opts.erase(i);
    }

    return Feature::ProcessOptionsAndRemove(opts);
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVBoV::InitializeBoV() {
    detectorName = "SIFT";
    descriptorName = "SIFT";
    matcherName = "BruteForce";
    vocabularyName = "vocabulary.yml";
    createvocabulary = false;
    nclusters = 10;
    ntrainvectors = 1000;
    ntrainimages = 3;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVBoV::CalculateOpenCV(int /*f*/, int l) {
    static const string msg = "OCVBoV::CalculateOpenCV(): ";
    featureVector fv;
    nimg++;

    if (!fd) {
      if (FileVerbose())
	cout << msg << "Creating FeatureDetector: " << detectorName << endl;
      fd = FeatureDetector::create(detectorName);
    }
    if (!fd) {
      cerr << msg << "Invalid feature detector: " << detectorName << endl;
      return false;
    }

    if (!de) {
      if (FileVerbose())
	cout << msg << "Creating DescriptorExtractor: " << descriptorName
	     << endl;      
      de = DescriptorExtractor::create(descriptorName);
    }
    if (!de) {
      cerr << msg << "Invalid descriptor extractor: " << descriptorName << endl;
      return false;
    }

    if (!dm) {
      if (FileVerbose())
	cout << msg << "Creating DescriptorMatcher: " << matcherName << endl;
      dm = DescriptorMatcher::create(matcherName);
    }
    if (!dm) {
      cerr << msg << "Invalid descriptor matcher: " << matcherName << endl;
      return false;
    }

    if (!be) {
      if (FileVerbose())
	cout << msg << "Creating BOWImgDescriptorExtractor: "
	     << descriptorName << "," 
	     << matcherName << endl;      
      be = new BOWImgDescriptorExtractor(de, dm);
    } 
    if (!be) {
      cerr << msg << "Invalid BoV descriptor extractor: " << descriptorName
	   << "," << matcherName << endl;
      return false;
    }

    // Detect keypoints
    vector<KeyPoint> keypoints;
    fd->detect(CurrentFrameOCV(), keypoints);

    if (createvocabulary) {

    // Extract features
      Mat descriptors;
      de->compute(CurrentFrameOCV(), keypoints, descriptors);

      if (descriptors.empty())
	cerr << msg << "No descriptors calculated for image" << BaseFileName()
	     << endl;

      traindescriptors.push_back(descriptors);
      
      fv.push_back(nimg);
      fv.push_back(descriptors.rows);
      fv.push_back(traindescriptors.rows);

      if ((int)nimg == ntrainimages)
	TrainVocabulary();

    } else {
    
      if (vocabulary.rows == 0)
	ReadVocabulary();
      
      if (vocabulary.rows == 0) {
	cerr << msg << "vocabulary is empty" << endl;
	return false;
      }

      if (keypoints.size()) {
	Mat bowdescriptors;
	be->compute(CurrentFrameOCV(), keypoints, bowdescriptors);
      
	for (int i=0; i<bowdescriptors.cols; i++)
	  fv.push_back(bowdescriptors.at<float>(0,i));

      } else 
	for (int i=0; i<vocabulary.rows; i++)
	  fv.push_back(1.0/vocabulary.rows);

    }

    SetVectorData(l, fv);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool OCVBoV::TrainVocabulary() {
    static const string msg = "OCVBoV::TrainVocabulary(): ";

    CV_Assert(de->descriptorType() == CV_32FC1);
    double td_mem_use = 4*de->descriptorSize()*traindescriptors.rows/1048576.0;
    
    if (FileVerbose())
      cout << msg << "starting with " << traindescriptors.rows 
	   << " descriptors taking about " 
	   << setprecision(3) << td_mem_use << " MB of memory" << endl;

    TermCriteria terminate_criterion;
    terminate_criterion.epsilon = FLT_EPSILON;
    BOWKMeansTrainer bowTrainer(nclusters, terminate_criterion,
				3, KMEANS_PP_CENTERS);

    int ndata = MIN(traindescriptors.rows,ntrainvectors);
    Mat indices(ndata, 1, CV_32S);
    for (int i = 0; i<ndata; i++)
      indices.at<int>(i,0) = i;
    randShuffle(indices);

    if (FileVerbose())
      cout << msg << "starting to add descriptors to bowTrainer" << endl;
    for (int i = 0; i<ndata; i++)
      bowTrainer.add(traindescriptors.row(indices.at<int>(i,0)));
    if (FileVerbose())
      cout << msg << "added " << bowTrainer.descripotorsCount() 
	   << " descriptors to bowTrainer" << endl;

    if (FileVerbose())
      cout << msg << "training vocabulary" << endl;
    vocabulary = bowTrainer.cluster();
    if (FileVerbose())
      cout << msg << "training done, vocabulary contains " << vocabulary.rows 
	 << " clusters, nclusters=" << nclusters << endl;

    if (FileVerbose())
      cout << msg << "saving vocabulary to " << vocabularyName << endl;
    FileStorage fs(vocabularyName, FileStorage::WRITE);
    if(fs.isOpened()) 
      fs << "vocabulary" << vocabulary;
    else {
      cerr << msg << "problem occurred in saving the vocabulary" << endl;
      return false;
    }

    be->setVocabulary(vocabulary);

    if (FileVerbose())
      cout << msg << "finished ok" << endl;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool OCVBoV::ReadVocabulary() {
    static const string msg = "OCVBoV::ReadVocabulary(): ";
    if (FileVerbose())
      cout << msg << "reading vocabulary from " << vocabularyName << endl;
    if (vocabulary.rows > 0) 
      cerr << msg << "vocabulary not empty" << endl;

    FileStorage fs(vocabularyName, FileStorage::READ);
    if (fs.isOpened()) {
      fs["vocabulary"] >> vocabulary;
      if (FileVerbose())
	cout << msg << "done reading, vocabulary size now " << vocabulary.rows 
	     << endl;      

      be->setVocabulary(vocabulary);     
      return true;
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  /////////////////////////////////////////////////////////////////////////////
  
  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP && PICSOM_USE_OPENCV


