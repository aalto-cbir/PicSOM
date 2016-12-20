// -*- C++ -*-  $Id: OCVBoV.h,v 1.1 2012/04/11 12:27:53 markus Exp $
/**
   \file OCVBoV.h

   \brief Declarations and definitions of class OCVBoV
   
   OCVBoV.h contains declarations and definitions of 
   class the OCVBoV, which
   is a class that calculates some simple geometric descriptors.
  
   \author Mats Sjöberg <mats.sjoberg@aalto.fi>
   $Revision: 1.1 $
   $Date: 2012/04/11 12:27:53 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _OCVBoV_
#define _OCVBoV_

#include <opencv2/features2d/features2d.hpp>

#include <OCVFeature.h>

using namespace cv;

namespace picsom {

  class OCVBoV : public OCVFeature {
  public:

    OCVBoV(const string& img = "",
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, "", NULL, opt); 
      InitializeBoV(); 
    }

    OCVBoV(const string& img, const string& seg,
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, seg, NULL, opt); 
      InitializeBoV(); 
    }

    OCVBoV(bool) : OCVFeature(false) {}

    virtual Feature *Create(const string& s) const
    { return (new OCVBoV())->SetModel(s); }

    virtual string Name() const { return "OCVBoV"; }
    virtual string LongName() const { 
      return "OpenCV bag-of-visual-words feature"; }
    virtual string ShortText() const { 
      return "BoV implementation using OpenCV features2d framework"; }
    virtual string TargetType() const { return "image"; }

    virtual string Version() const;

    int printMethodOptions(ostream&) const;

  protected:

    bool InitializeBoV();

    virtual bool ProcessOptionsAndRemove(list<string>&);

    virtual bool CalculateOpenCV(int f, int l);

    bool TrainVocabulary();
    bool ReadVocabulary();

    static Ptr<FeatureDetector> fd;
    static Ptr<DescriptorExtractor> de;
    static Ptr<DescriptorMatcher> dm;
    static Ptr<BOWImgDescriptorExtractor> be;

    static Mat vocabulary;

    static size_t nimg;
    static Mat traindescriptors;

    string detectorName;
    string descriptorName;
    string matcherName;
    string vocabularyName;

    bool createvocabulary; 
    int nclusters;
    int ntrainvectors;
    int ntrainimages;

  };
}

#endif // _OCVBoV_
