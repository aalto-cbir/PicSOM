// -*- C++ -*-	$Id: ScaleSal.h,v 1.2 2004/02/10 16:14:02 vvi Exp $

#ifndef _SCALESAL_H_
#define _SCALESAL_H_

#include <Segmentation.h>
#include <stdio.h>
// #include <fftw3.h>

namespace picsom {

  class XYS{
    
  public:
    XYS(int X,int Y, int S) : x(X),y(Y),s(S) {}; 
    short int x;
    short int y;
    short int s;
  };

  struct ltXYS{
    bool operator()(const XYS &x1, const XYS &x2) const
    {
      if(x1.x<x2.x) return true;
      if(x1.y<x2.y) return true;
      if(x1.s<x2.s) return true;
      return false;
    }
  };


  class salPoint: public XYS {
public: 
    salPoint(int X, int Y, int S,float Sal) : XYS(X,Y,S),sal(Sal){}
    salPoint(const std::pair<XYS,float> &o) : XYS(o.first),sal(o.second){}
    float sal;
  };

  bool ltSal(const salPoint &v1,
	     const salPoint &v2){
    return v1.sal < v2.sal;
  }

  bool gtSal(const salPoint &v1,
	     const salPoint &v2){
    return v1.sal > v2.sal;
  }


  class salCluster : public salPoint {
  public:
    salCluster(int X, int Y, int S, float Sal) : salPoint(X,Y,S,Sal){}
    salCluster(const std::pair<XYS,float> &o) : salPoint(o){};
    int dist;
    int ind;

    int calcSqrDistTo(const XYS &o){
      int d=(x-o.x);
      dist =d*d;
      d=(y-o.y);
      dist += d*d;
      d=(s-o.s);
      dist += d*d;
      return dist;
    }

  };

  bool clusterCloser(const salCluster &c1,const salCluster &c2){
    //    cout << "comparing distances" << c1.dist <<" and "<<c2.dist << endl;
    return c1.dist < c2.dist;
  }

  bool clusterFurther(const salCluster &c1,const salCluster &c2){
    return c1.dist > c2.dist;
  }
      

  class Histogram{
  public:
    void resize(int s){count.resize(s);}
    void reset(int s){
      count = std::vector<int>(s,(int)0);
      totalCount=0;
    }
    
    void addPoint(int val){
      // no checking here whether val is a valid index
      //std::cout << "in Histogram::addPoint("<<val<<")"<<std::endl;
      count[val]++;
      totalCount++;
    }
    
    void dump(std::ostream &os) const{
      size_t i;
      for(i=0;i<count.size();i++)
	os << "count[" <<i<<":"<<count[i]<<std::endl;
    }
    float calcEntropy() const{
      //std::cout << "calcEntropy, totalCount=" << totalCount << std::endl;
      if(!totalCount) return 0;
      //dump(std::cout);

      float h=0,logN=log((double)totalCount);
      size_t i;
      for(i=0;i<count.size();i++)
	if(count[i])
	  h += count[i]*(log((double)count[i])-logN);

      return -h / (totalCount*log(2.0));
    }


    // the absolute difference of two histograms

    float calcDifference(const Histogram &o) const{
      float d=0;
      float cumd=0;
      size_t i;

      for(i=0;i<count.size();i++){
	d = ((float)count[i])/totalCount;
	// std::cout << d << std::endl;
	d -= ((float)o.count[i])/o.totalCount;
	cumd += (d>0) ? d : -d;
      }
      
      return cumd;
    }

    int totalCount;
    std::vector<int> count;
  };


  class ScaleSal : public Segmentation {
  public:
    /// 
    ScaleSal();

    /// 
    ScaleSal(bool b) : Segmentation(b) {}

    /// 
    virtual ~ScaleSal();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new ScaleSal(); }  

    ///
    virtual const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"ScaleSal":"ss"; }

    ///
    virtual const char *Description() const { return "scalesal"; }
    
    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);
    
  protected:

    void createDeltaLists(int startScale, int stopScale);

    void addPoint(Histogram &h,int x, int y){
      //std::cout << "addPoint(" << x <<","<<y<<")"<<std::endl;
      if(!getImg()->coordinates_ok(x,y)) return;
      //std::cout << "coordinates (" << x << "," << y << ")" << std::endl;
      h.addPoint(quantised_image[x+y*getImg()->getWidth()]);
    }

    void quantise();

    int qsteps() const { return  1 << 9;}

    void collectSalientPoints(int x, int y,int minscale,int maxscale);

    void kadirClustering();

    void labelRegions();

    void findKNearest(std::vector<salCluster> **sList, 
		      std::vector<salCluster> **result, int K);
    
    //    void fftw_test();

    // static void multiplyComplexVector(fftw_complex *v1, 
    //			      const fftw_complex *v2, int len);

    int minscale,maxscale;
    int quantLevels;

    // these lists list the coordinates of the pixels 
    // that form the arc of a circle between angles (0,pi/4]

    std::vector<coordList> deltaList;

    std::vector<salCluster> *saliencyAcc;
    std::vector<salCluster> *clusteredSal;

    int *quantised_image;

    float maxThusFar;
    float globalThreshold;
    float varThreshold;
    int numberOfMeans;
    bool skipClustering;

  int clustersToKeep;

  // int iterations;
  //  int fftN;
  //  int fftK;

  };

} // namespace picsom
#endif // _SCALESAL_H_

// Local Variables:
// mode: font-lock
// End:
