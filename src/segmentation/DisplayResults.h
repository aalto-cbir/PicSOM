// -*- C++ -*-  $Id: DisplayResults.h,v 1.15 2009/02/04 09:38:13 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _DISPLAYRESULTS_H_
#define _DISPLAYRESULTS_H_

#include <Segmentation.h>
#include <videofile.h>
#include <stdio.h>

namespace picsom {

  class DisplayResults : public Segmentation {
  public:

    enum masking_type {
      total_masking,
      dim_masking,
      hue_masking,
      false_colour,
      modulated_colour,
      average_rgb
    };

    class ResultInfo {
    public:
      ResultInfo() : wildcard(false) {}

      string name;
      string colour;
      string modifier;
      string text;
      string textplace;
      bool wildcard;
    };

    class TmpResultInfo {
    public:
      string colour;
      string type;
      string val;
    };

    class dr_merge {
    public:
      dr_merge() {
	from1=from2=level=-1;
	l=lb=NULL;
      }

      int from1;
      int from2;
      coordList *l;
      coordList *lb;
      int level;
    };

    /// 
    DisplayResults();
    
    /// 
    DisplayResults(bool b) : Segmentation(b) {}
    
    /// 
    virtual ~DisplayResults();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new DisplayResults(); }  
    
    ///
    const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"DisplayResults":"dr"; }
    
    ///
    virtual bool Process();
    
    ///
    virtual int ProcessOptions(int, char**);
    
    virtual const char *Description() const { 
      return 
	"Combines segmentation results with the input image for visualisation.";
    };
    
    ///
    virtual bool PreventOutput() const { return true; }
    
    ///
    virtual bool OpenExtraFiles();

    ///
    virtual bool CloseExtraFiles();

    ///
    void AddResultFrame(imagedata& d);

    ///
    void AddResultCutFrame(imagedata& d);
    
    ///
    void SetVideoFrameRate(double fps) { video_fps = fps>0?fps:25; }

    ///
    void DisplayTree();

  protected:
    
    void RemoveRegionSpec() {
      for(size_t i=0; i<region_spec.size(); i++)
	delete region_spec[i];
      region_spec.clear();
    }


    ///
    void SetResultName(const char *n){result_pattern=n;}
    
    ///
    void SetStandardResultNaming(){ SetResultName("%i.res");}
    //"segments/%m:%i.res");}
    
    ///
    void DrawLine(int x1,int y1, int x2,int y2,
		  const RGBTriple &rgb);
    ///
    void DrawBox(int x1,int y1, int x2,int y2,
		 const RGBTriple &rgb);

    ///
    void DrawRotatedBox(int l, int t, int w,int h,
			float theta, float xc, float yc,
			const RGBTriple &rgb);
    ///
    void DrawPoint(int x, int y,
		   const RGBTriple &rgb);
    ///
    void DrawFourierContour(std::list<double> coefficients,
			    const RGBTriple &rgb);
    
    ///
    void DrawText(const PredefinedTypes::Box&, const string&, const string&);

    ///
    std::string FormResultName();
    
    ///
    void ParseResult(const char *result, std::vector<ResultInfo> &resultlist,
                     bool wildcard = false);
    
    ///
    void ParseLabels(const char *spec,std::vector<int> &result) const;
    
    ///
    void ShowFrameResult(const ResultInfo &info);

    ///
    void ShowTmpResult(const TmpResultInfo &info);

    ///
    void ShowFileResult(const ResultInfo &info);

    /// Makes a vector<bool> to decide frames to be cutted out.
    void MakeCutFrames(const SegmentationResultList *resultlist);

    /// Makes a blank frame which is inserted between subclips.
    void MakeBlankFrame();

    ///
    void ShowResultList(const SegmentationResultList *resultlist,
			const ResultInfo &info);
    
    ///
    void DoMasking(void);
    
    ///
    void MarkBoundaries(void);

    ///
    void find_boundary(const coordList &l, coordList &lb, int lbl,
		       const coordList *lb1=NULL, const coordList *lb2=NULL);

    ///
    static void label_hierarchy(std::vector<dr_merge> &mergeList,int root,int level);

    ///
    void WhitenBackground();

    ///
    string result_pattern;
    
    ///
    string result_filename;
    
    ///
    std::vector<ResultInfo> frameresultlist;

    ///
    std::vector<TmpResultInfo> tmpresultlist;

    ///
    std::vector<ResultInfo> fileresultlist;
    
    ///
    bool do_masking;
    
    ///
    bool mark_boundaries;

    //
    bool wideBoundaries;
    
    ///
    bool display_tree;

    ///
    bool virtualregions;

    /// 
    bool whiteBackground;

    ///
    masking_type masking_method;
    
    ///
    std::vector<int> maskSpec;
    std::vector<int> boundarySpec;

    std::string maskSpecString;
    std::string boundarySpecString;

    std::vector<bool> cutFrames;

    imagedata *resultframe;
    imagefile *resultfile;

    imagedata *blankframe;

    videofile *resultvideo;

    double video_fps;

    /// list of regions 

    vector<Region*> region_spec;
    
  };
}

#endif // _DISPLAYRESULTS_H_

// Local Variables:
// mode: font-lock
// End:
