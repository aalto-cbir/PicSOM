// -*- C++ -*-  $Id: OCVLbp.h,v 1.11 2013/02/25 13:51:38 jorma Exp $
// 
// Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

/**
   \file OCVLbp.h

   \brief Declarations and definitions of class OCVLbp and 
   
   OCVLbp.h contains declarations and definitions of 
   class the OCVLbp, which
   is a class that ...
  
   \author Markus Koskela <markus.koskela@aalto.fi>
   $Revision: 1.11 $
   $Date: 2013/02/25 13:51:38 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _OCVLbp_
#define _OCVLbp_

#include <OCVFeature.h>

#ifdef HAVE_OPENCV2_CORE_CORE_HPP

#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

namespace picsom {

  typedef enum {LBP,CSLBP} lbp_type;

  typedef enum {LBP_8NEIGHBORS,LBP_CIRCULAR} lbp_neighborhood_type;

  typedef enum {LBP_MAPPING_NONE,LBP_MAPPING_UNIFORM2,
		LBP_MAPPING_ROTATION_INVARIANT,
		LBP_MAPPING_UNIFORM2_ROTATION_INVARIANT} lbp_mapping_type;

  typedef enum {BLOCKS_1x1,BLOCKS_2x2,BLOCKS_2x2_SPLITS,BLOCKS_3x3,
		BLOCKS_3x3_SPLITS,BLOCKS_4x4,BLOCKS_4x4_SPLITS,
		BLOCKS_5x5,BLOCKS_6x6,BLOCKS_7x7,BLOCKS_8x8,
		BLOCKS_9x9,BLOCKS_10x10,BLOCKS_8x15,BLOCKS_FGBG} lbp_blocks_type;

  typedef enum {BLOCK_NORMAL,BLOCK_STORE,BLOCK_SUBTRACT} lbp_block_type;

  typedef enum {HISTNORM_NONE,HISTNORM_NORMAL,HISTNORM_CAPPED} histnorm_type;

  struct crop_t {
    size_t x0;
    size_t x1;
    size_t y0;
    size_t y1;
    crop_t(): x0(0),x1(0),y0(0),y1(0) { }
    void clear() { x0 = x1 = y0 = y1 = 0; }
    void set(size_t val) { x0 = x1 = y0 = y1 = val; }
    void set(size_t xval, size_t yval) { x0 = x1 = xval; y0 = y1 = yval; }
    void set(size_t _x0, size_t _x1, size_t _y0, size_t _y1) { 
      x0 = _x0; x1 = _x1; y0 = _y0; y1 = _y1; }
    //bool isSet() { if ( x0 || x1 || y0 || y1 ) return true; return false; }
  };

  struct lbp_opts {
    crop_t crop;
    bool write_image;
    lbp_type lbptype;
    lbp_neighborhood_type neighborhood;
    size_t nneighbors;
    double radius;
    lbp_mapping_type mapping;
    histnorm_type normalize_histogram;
    double cslbp_threshold;
    bool overlapping_blocks;
    bool do_funneling;
    bool do_illnorm;
    bool cropfirst;
  };

  struct lbp_map_t {
    map<unsigned int,unsigned int> mapping;
    size_t mapsize;
  };

  class OCVLbp : public OCVFeature {
  public:

    OCVLbp(const string& img = "",
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, "", NULL, opt); 
      InitializeLbp();
    }

    OCVLbp(const string& img, const string& seg,
                const list<string>& opt = (list<string>)0) { 
      Initialize(img, seg, NULL, opt); 
      InitializeLbp();
    }

    OCVLbp(bool) : OCVFeature(false) { }

    ~OCVLbp() { }

    virtual Feature *Create(const string& s) const
    { return (new OCVLbp())->SetModel(s); }

    virtual string Name() const { return "OCVLbp"; }
    virtual string LongName() const { return "OpenCV LBP feature"; }
    virtual string ShortText() const { return LongName(); }
    virtual string TargetType() const { return "image"; }

    virtual string Version() const;

    int printMethodOptions(ostream&) const;

  protected:

    bool InitializeLbp();

    virtual bool ProcessOptionsAndRemove(list<string>&);

    virtual bool CalculateOpenCV(int f, int l);

    bool CalculateLbp(const Mat &img, MatND &lbphist);

    void InterpolatedImage(Mat &img, const Mat &srcimg, const Point2d &s);

    bool CreateSPoints();

    bool CreateMap();

    bool CreateBlocks();

    bool CreateBlocks(int,int);

    bool CreateBlockSplits(int,int);

    lbp_opts options;

    /// Neighborhood sampling points
    vector<Point2d> spoints;

    /// Mapping table for LBP codes
    lbp_map_t lbp_map;

    //// Image to calculate LBP codes from. Either ocv_image or cropped from it.
    Mat lbp_image;    

    list<lbp_blocks_type> blocktypes;

    list<pair<Rect,lbp_block_type> > lbp_blocks;

    Point2d spoints_min;
    Point2d spoints_max;

    string detectorName;
    string descriptorName;

    string funneling_modelfile;

  }; // class OCVLbp

} // namespace picsom

#endif // HAVE_OPENCV2_CORE_CORE_HPP

#endif // _OCVLbp_
