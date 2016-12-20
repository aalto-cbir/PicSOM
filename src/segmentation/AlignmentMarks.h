// -*- C++ -*-	$Id: AlignmentMarks.h,v 1.6 2003/09/17 09:10:37 vvi Exp $

#ifndef _ALIGNMENTMARKS_H_
#define _ALIGNMENTMARKS_H_

#include <Segmentation.h>

namespace picsom{

  class AlignmentMarks : public Segmentation {
  public:
    /// 
    AlignmentMarks();

    /// 
    AlignmentMarks(bool b) :Segmentation(b) {}

    /// 
    virtual ~AlignmentMarks();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new AlignmentMarks(); }  

    ///
    const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"AlignmentMarks":"am"; }
    
    ///
    virtual bool Process();
    ///
    virtual int ProcessOptions(int, char**);
    
    virtual const char *Description() const {return "alignment marks";};
    
    
  protected:
    
    int FindTop();
    int FindBottom();
    int *GetSlice(int begin, int height,int margin);
    int *GetVerticalSlice(int begin,int width,int margin);
    
    void EdgeDetect(int *slice,int w,int maskwidth,int &max_pos,int &min_pos);

    bool printIntermediate;

  };
} // namespace picsom

#endif // _ALIGNMENTMARKS_H_

// Local Variables:
// mode: font-lock
// End:
