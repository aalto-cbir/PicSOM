// -*- C++ -*-	$Id: SpringModel.h,v 1.4 2004/01/13 11:05:34 vvi Exp $

#ifndef _SPRINGMODEL_H_
#define _SPRINGMODEL_H_

#include <PixelTemplate.h>
#include <Segmentation.h>

namespace picsom {

class SpringModel : public Segmentation {
public:
  /// 
  SpringModel();

  /// 
  SpringModel(bool b) : Segmentation(b) {}

  /// 
  virtual ~SpringModel() {}

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new SpringModel(); }  

  ///
  virtual const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  ///
  virtual const char *MethodName(bool l = false) const {
    return l?"SpringModel":"sm"; }

  ///
  virtual const char *Description() const { return "faceparts"; }

  ///
  virtual bool Process();

  ///
  virtual int ProcessOptions(int, char**);

  ///
  // virtual bool PreventOutput() const;

  ///
  // virtual bool OpenExtraFiles();

  ///
  class state_t {
  public:
    ///
    state_t() {}  // why is this needed ???

    ///
    state_t(int l) : coords(l) {}

  protected:
    ///
    vector<int> coords;

    ///
    float e_pot;

    ///
    float e_img;

    ///
    float e_tot;
  };

  ///
  enum optimization_t {
    optimization_undef,
    optimization_springsonly,
    optimization_springsmoving,
    optimization_springsandimages,
    optimization_imageonly
  };

  ///
  class spring_t {
  public:
    /// (x,y)-(x,y) spring
    spring_t(const state_t *st, float rl, float k,
	     int x1, int y1, int x2, int y2) :
      statep(st), ix1(x1), iy1(y1), ix2(x2), iy2(y2), rest_length(rl), mult(k)
    {}

    /// (x)-(x) spring
    // spring_t(const state_t *st, float rl, float k, int x1, int x2) :
    //   statep(st), ix1(x1), ix2(x2),iy1(-1),iy2(-1), rest_length(rl), mult(k)
    // {}

    double Energy() const { return 0; }

  protected:
    ///
    const state_t *statep;

    /// indices to state pointed to by statep
    int ix1, iy1, ix2, iy2;

    ///
    float rest_length;

    /// "spring constant"
    float mult;
  };

  ///
  int AddPixelTemplate(const PixelTemplate& pt) {
    pixtempl.push_back(pt);
    return pixtempl.size()-1;
  }

  ///
  state_t CreateStateVariable() { return state_t(2*pixtempl.size()); }

  ///
  int AddSpring(const spring_t& s) {
    spring.push_back(s);
    return spring.size()-1;
  }

  ///
  state_t Optimize(const state_t& start,
		   const state_t& low, const state_t& high);

protected:
  ///
  optimization_t optimization;

  ///
  state_t state;  // voi olla etta tarvitaan vain funktioissa

  ///
  vector<spring_t> spring; // list<> ? nyt spring.size() == 6

  /// VOI OLLA ETTÄ SEURAAVA EI _VIELÄ_ TOIMI
  list<PixelTemplate> pixtempl; // vector<> ? pixtempl.size() == 4
};

}

#endif // _SPRINGMODEL_H_

// Local Variables:
// mode: font-lock
// End:
