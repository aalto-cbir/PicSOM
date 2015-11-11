// -*- C++ -*- 	$Id: ScalableColor.h,v 1.2 2008/10/31 09:44:27 jorma Exp $
/**
   \file ScalableColor.h

   \brief Declarations and definitions of class ScalableColor
   
   ScalableColor.h contains declarations and definitions of class 
   ScalableColor, which
   calculates the Haar-transform of quantised HSV colour histogram,
   mimicking the corresponding MPEG-7 descriptor
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.2 $
   $Date: 2008/10/31 09:44:27 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _ScalableColor_
#define _ScalableColor_

#include "PseudoMPEG7.h"

namespace picsom {

class ScalableColor : public PseudoMPEG7 {

 public:

  /** Constructor
      \param img initialise to this image
      \param opt list of options to initialise to
  */
  ScalableColor(const string& img = "", const list<string>& opt = (list<string>)0) {
    Initialize(img, "", NULL, opt);
  }

  /** Constructor
      \param img initialise to this image
      \param seg initialise with this segmentation
      \param opt list of options to initialise to
  */
  ScalableColor(const string& img, const string& seg,
	      const list<string>& opt = (list<string>)0) {
    Initialize(img, seg, NULL, opt);
  }

 
  /// This constructor doesn't add an entry in the methods list.
  ScalableColor(bool) : PseudoMPEG7(false) {}

  // ~ScalableColor();

  virtual Feature *Create(const string& s) const {
    return (new ScalableColor())->SetModel(s);
  }

 
  // copied from PixelBased

  virtual string Name()          const { return "scalablecolor"; }

  virtual string LongName()      const { return "Scalable color"; }

  virtual string ShortText()     const {
    return "Haar transform of quantised HSV colour histogram."; }


  virtual Feature *Initialize(const string& img, const string& seg,
		      segmentfile *s, const list<string>& opt,
		      bool allocate_data=true);

  virtual string Version() const;

  virtual featureVector getRandomFeatureVector() const{
    return vector<float>(256);
  }

  virtual int printMethodOptions(ostream&) const;

  virtual bool ProcessOptionsAndRemove(list<string>&); 

protected:

  virtual featureVector CalculateMask(char *mask);

  int histquant4(int in){

    ensure_quant_table();

    return quant_table[in];

  }

  void ensure_quant_table(){
    if(quant_table.size() != 2048){
      quant_table.resize(2048);

      int i;
      for(i=0;i<1;i++)
	quant_table[i]=0;

      for(;i<3;i++)
	quant_table[i]=1;

      for(;i<10;i++)
	quant_table[i]=2;

      for(;i<22;i++)
	quant_table[i]=3;

      for(;i<41;i++)
	quant_table[i]=4;

      for(;i<67;i++)
	quant_table[i]=5;

      for(;i<102;i++)
	quant_table[i]=6;

      for(;i<145;i++)
	quant_table[i]=7;

      for(;i<198;i++)
	quant_table[i]=8;

      for(;i<262;i++)
	quant_table[i]=9;

      for(;i<336;i++)
	quant_table[i]=10;

      for(;i<422;i++)
	quant_table[i]=11;

      for(;i<520;i++)
	quant_table[i]=12;

      for(;i<630;i++)
	quant_table[i]=13;

      for(;i<753;i++)
	quant_table[i]=14;

      for(;i<2048;i++)
	quant_table[i]=15;
    }
      
  }

  vector<int> quant_table;

  void haar1d(vector<float> &vec)
  {
    
    int n=vec.size();

     int i=0;
     int w=n;
     float *vecp = new float[n];
     float div=1.0/sqrt(2.0);
     for(i=0;i<n;i++)
          vecp[i] = 0;

     while(w>1)
     {
          w/=2;
          for(i=0;i<w;i++)
          {
	    vecp[i] = (vec[2*i] + vec[2*i+1])*div;
	    vecp[i+w] = (vec[2*i] - vec[2*i+1])*div;
          }

          for(i=0;i<(w*2);i++)
	    vec[i] = vecp[i];
     }

     delete [] vecp;
  }

};
}
#endif

// Local Variables:
// mode: font-lock
// End:
