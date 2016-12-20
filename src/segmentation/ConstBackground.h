// -*- C++ -*-	$Id: ConstBackground.h,v 1.12 2000/08/10 09:34:18 vvi Exp $

#ifndef _CONSTBACKGROUND_H_
#define _CONSTBACKGROUND_H_

#include <PrePostProcessor.h>

class ConstBackground : public PrePostProcessor {
public:
  /// 
  ConstBackground();

  ConstBackground(bool b) : PrePostProcessor(b) {}

  /// 
  virtual ~ConstBackground();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new ConstBackground(); }

  ///
  virtual const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  virtual const char *MethodName(bool l = false) const {
    return l ? "ConstBackground" : "cb"; }

  ///
  const char *Description() const 
  {return "Marking pixels similar and adjacent (in RGB) to border as background";};


protected:
  virtual bool PreProcess();
  virtual bool DoProcess();
  virtual bool PostProcess();

  virtual int BackgroundInNeighbour(int x,int y,bool include_diagonals=false);
  virtual bool MedianCompare(int x,int y);
  virtual void MarkCompatible();
  virtual int CountBkgndInNeighbourhood(int x,int y);
  virtual bool IsPixelCompatibleWithBkgnd(int x,int y);
  virtual void CalculateAverages();
  virtual bool FindThreshold();

  static const int min_tolerance=19;  // in intensity levels
  static const int median_radius=2; 

  struct rgb {int r; int g; int b;};
  rgb tolerance;

  bool *filtered;

  float avgr,avgg,avgb;
  bool averages_calculated;
};

#endif // _CONSTBACKGROUND_H_

// Local Variables:
// mode: font-lock
// End:
