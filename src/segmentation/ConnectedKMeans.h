// -*- C++ -*-	$Id: ConnectedKMeans.h,v 1.4 2001/04/28 07:21:10 vvi Exp $

#ifndef _CONNECTEDKMEANS_H_
#define _CONNECTEDKMEANS_H_

#include <KMeans.h>

class ConnectedKMeans : public KMeans {
public:
  /// 
  ConnectedKMeans();

  ConnectedKMeans(bool b) :KMeans(b) {}

  /// 
  virtual ~ConnectedKMeans();

  /// Here are the pure virtuals overloaded:
  
  ///
  virtual Segmentation *Create() const { return new ConnectedKMeans(); }

  ///
  virtual const char *Version() const;

  ///
  virtual void UsageInfo(ostream& = cout) const;

  virtual const char *MethodName(bool l = false) const {
    return l ? "ConnectedKMeans" : "ck"; }

  ///
  // virtual bool Process(); inherited from parents

  virtual int ProcessOptions(int, char**);

  const char *Description() const 
  {return "RGB based K means clustering combined with a simple Gibbs RF.";};

protected:
  virtual int AssignLabels();

  virtual float ImageVariance();

  virtual bool PreProcess();
  virtual bool DoProcess();
  virtual bool PostProcess();
  
  float adjacency_energy;
  float default_variance;

  int *original_labels;

};

#endif // _CONNECTEDKMEANS_H_

// Local Variables:
// mode: font-lock
// End:
