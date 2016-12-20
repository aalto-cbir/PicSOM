// -*- C++ -*-	$Id: Snake.h,v 1.2 2000/10/30 20:41:40 vvi Exp $

#ifndef _SNAKE_H_
#define _SNAKE_H__

#include <PrePostProcessor.h>

class Snake : public PrePostProcessor {
public:
  /// 
  Snake();

  /// 
  Snake(bool b) : PrePostProcessor(b) {}

  /// 
  virtual ~Snake();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new Snake(); }  

  ///
  const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"Snake":"sn"; }

  ///
  virtual bool DoProcess();
  virtual bool PreProcess();
  virtual bool PostProcess();

  ///
  virtual int ProcessOptions(int, char**);

  virtual const char *Description() const {return "basic snake";};

protected:
  class ContentStatistics{
  public:
    void Zero(){sum_r=sum_g=sum_b=sqr_sum=pixelcount=0;}
    ContentStatistics & operator+=(const ContentStatistics & rhs)
    {
      sum_r += rhs.sum_r;
      sum_g += rhs.sum_g;
      sum_b += rhs.sum_b;

      sqr_sum += rhs.sqr_sum;
      pixelcount += rhs.pixelcount;
      return *this;
    }

    ContentStatistics & operator-=(const ContentStatistics & rhs)
    {
      sum_r -= rhs.sum_r;
      sum_g -= rhs.sum_g;
      sum_b -= rhs.sum_b;

      sqr_sum -= rhs.sqr_sum;
      pixelcount -= rhs.pixelcount;
      return *this;
    }

    float sum_r;
    float sum_g;
    float sum_b;
    float sqr_sum;
    float pixelcount;

    friend ostream &operator<<(ostream &out, const ContentStatistics &c) {
      out << "sum: (r,g,b)=("<<c.sum_r<<","
	  <<c.sum_g<<","<<c.sum_b<<")"<<endl;
      out << "sqr_sum="<<c.sqr_sum<<" pixelcount=" 
	  << c.pixelcount << endl;
      return out;
    }
  };

  class SnakeArc;

  class SnakeNode{
  public:

    SnakeNode();
    void Displace(int max_d);

    coord loc;
    SnakeNode *prevnode;
    SnakeNode *nextnode;
    SnakeArc *prevedge;
    SnakeArc *nextedge;

    double displacement_energy;
    ContentStatistics stats;

  };

  class SnakeArc{
  public:

    SnakeArc();

    SnakeNode *startnode;
    SnakeNode *endnode;

    double gradient_energy;
    double direction_energy;
    double distance_energy;
    ContentStatistics stats;
  };

  SnakeNode *nodes;
  int nodecount;
  SnakeArc *edges;
  float *grad_x, *grad_y;
  float *smoothgrad;
  float *grad_magn;

  double imagesum_r;
  double imagesum_g;
  double imagesum_b;

  double imagesqrsum;

  ContentStatistics globalstats;
  float contentenergy;

  int *cumul_sum_r;
  int *cumul_sum_g;
  int *cumul_sum_b;
  int *cumul_sqr_sum;
  int *cumul_pixcount;

  double mul[4]; // coefficients for the various terms in the total energy

  void CalculateGradients();
  void InitSnake();

  bool CheckIntersections(SnakeNode *n);
  bool CheckIntersections(SnakeArc *e);
  bool TestIntersection(SnakeArc *e1, SnakeArc *e2);

  int GetSetCode(SnakeNode *n, SnakeArc *e);

  double CalculateOverlapEnergy(SnakeArc *e, float *field);

  void CalculateDisplacementEnergy(SnakeNode *n);
  void CalculateGradientEnergy(SnakeArc *e);
  void CalculateDirectionEnergy(SnakeArc *e);
  void CalculateDistanceEnergy(SnakeArc *e);
  float CalculateContentEnergy();

  void CalculateSums();

  void CalculateAllContentStatistics();
  void ChangeContentStatistics(SnakeNode *moved);
  void CalculateContentStatistics(SnakeArc *e,
				  std::vector<coord> &pixellist);
  void CalculateContentStatistics(SnakeNode *n,coord &previous,coord &next);

};



#endif // _SNAKE_H_

// Local Variables:
// mode: font-lock
// End:
