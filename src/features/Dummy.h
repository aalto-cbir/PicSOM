// -*- C++ -*- $Id: Dummy.h,v 1.1 2013/08/20 12:18:54 jorma Exp $

/**
   \file Dummy.h

   \brief Declarations and definitions of class Dummy
   
   Dummy.h contains declarations and definitions of the class
   Dummy, which is a base class for all feature calculations
   based on individual pixels.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.1 $
   $Date: 2013/08/20 12:18:54 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _Dummy_h_
#define _Dummy_h_

#include <Feature.h>

namespace picsom {
  /// Base class for feature calculations based on individual pixels.
  class Dummy : public Feature {
  public:
    
    Dummy(bool) : Feature(false) {}

    virtual string Name()      const { return "dummy"; }
    
    virtual string LongName()  const { return "dummy value 42"; }
    
    virtual string ShortText() const { return "Always 42."; }

    virtual Feature *Create(const string& s) const {
      return (new Dummy())->SetModel(s);
    }

    virtual Feature::Data *CreateData(pixeltype t, int n, int) const {
      return new DummyData(t, n, this);
    }

    virtual void deleteData(void *p) {
      delete (DummyData*)p;
    }

    virtual string Version() const;

    virtual featureVector getRandomFeatureVector() const {
      return featureVector();
    }

    virtual bool Incremental() const { return true; }

    virtual bool ProcessOptionsAndRemove(list<string>& l);

    virtual bool ReInitializable() const { return false; } // should be true...

    virtual bool ReInitialize() {
      // something ?
      return Feature::ReInitialize();
    }

    virtual bool HasRawOutMode() const { return true; }

    virtual bool HasRawInMode() const { return true; }

    virtual bool CalculateOneFrame(int) { return true; }
      
    virtual bool CalculateOneLabel(int f, int l) {
      return CalculateCommon(f, false, l); }

    virtual string TargetType() const { return "image"; }

    virtual treat_type DefaultWithinFrameTreatment() const {
      return treat_concatenate; }

    virtual treat_type DefaultBetweenFrameTreatment() const {
      return treat_separate; }

    virtual treat_type DefaultBetweenSliceTreatment() const {
      return treat_separate; }

    virtual pixeltype DefaultPixelType() const { return pixel_rgb; }

    /// Data class for pixel based data
    class DummyData : public Data {
    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      DummyData(pixeltype t, int n, const Feature *p) : Data(t, n, p) {
	Zero();
      }

      virtual string Name() const { return "RgbData"; }
      
      virtual int Length() const { return 1; }

      virtual featureVector Result(bool /*azc*/) const {
	featureVector v(1);
	v[0] = 42;
	return v;
      }

      /// Zeros the internal storage
      virtual void Zero() {
        Data::Zero();
        count = 0; 
      }

      /** Returns the number of pixels added
	  \return number of pixels added
      */
      int Count() const { return count; }

      /// Increment the counter
      void IncrementCount() {    
	count++;
	if (PixelVerbose())
	  cout << "DummyData::IncrementCount(), count=" << count << endl;
      }

      /// Deccrement the counter
      void DecrementCount() {    
	count--;
	if(count<0)
	  throw "DummyData::DecrementCount() : count becomes negative"; 
	if (PixelVerbose())
	  cout << "DummyData::DecrementCount(), count=" << count << endl;
      }

      /** Adds data from one pixel to the internal storage
	  \param v the data vector to add
      */
      virtual void AddPixel(const vector<float>& v, int x, int y) {
	if (PixelVerbose())
	  cout << "DummyData::AddPixel(const vector<float>&)" << endl;;

	IncrementCount();
	addToHistogram(v);
	Parent()->ProcessRawOutVector(v, x, y);
      }

      /**  Removes data from one pixel from the internal storage
	   \param v the data vector to remove
      */

      virtual void RemovePixel(const vector<float>& v) {
	if (PixelVerbose())
	  cout << "DummyData::RemovePixel(constvector<float>&)" << endl;;
	DecrementCount();
	addToHistogram(v, -1);
      }
    
      /**  Adds data from one pixel to the internal storage
	   \param v the data vector to add
      */
      virtual void AddPixel(const vector<int>&, int, int) {
	if (PixelVerbose())
	  cout << "DummyData::AddPixel(const vector<int>&)" << endl;;
	IncrementCount();
      }

      /**  Removes data from one pixel from the internal storage
	   \param v the data vector to remove
      */
      virtual void RemovePixel(const vector<int>&) {
	if (PixelVerbose())
	  cout << "DummyData::RemovePixel(constvector<int>&)" << endl;;
	DecrementCount();
      }

      /// += operator (not used jl?)
      virtual Data& operator+=(const Data &) {
	throw "DummyData::operator+=() not defined"; }

    private:
      /// the counter
      int count;
    };

  protected:
    /// This constructor is called by inherited class constructors.
    Dummy() { pixel_debug = 0; }

    //
    virtual vector<string> UsedDataLabels() const {
      return UsedDataLabelsFromSegmentData();
    }

    /** Function commonly used by CalculateOneFrame and CalculateOneLabel
	\param f the frame to calculate feature from
	\param all true if we are to calculate for all segments
	\param l the dataindex for the segment to calculate for
    */
    virtual bool CalculateCommon(int f, bool all, int l = -1);

    /** Function that performs the calculation for one pixel
	\param x x-coordinate of the pixel
	\param y y-coordinate of the pixel
	\param i the dataindex of the segment of the pixel
    */
    virtual void CalculateOnePixel(int x, int y, DummyData *d) {
      // cox::tictac::func tt(tics, "Dummy::CalculateOnePixel");
      vector<float> v = ObtainRawVector(x, y);
      d->AddPixel(v, x, y);
    }

    /** Function that reverses the calculation for one pixel
	\param x x-coordinate of the pixel
	\param y y-coordinate of the pixel
	\param i the dataindex of the segment of the pixel
    */
    virtual void UnCalculateOnePixel(int x, int y, DummyData *d) {
      vector<float> v = ObtainRawVector(x, y);
      d->RemovePixel(v);
    }

    /** Function that calculates or reads from file raw vectors.
	\param x x-coordinate of the pixel
	\param y y-coordinate of the pixel
    */
    virtual vector<float> ObtainRawVector(int x, int y) {
      vector<float> raw;

      if (IsRawInMode())
	raw = NextRawVector(x, y);

      else {
	vector<float> v = GetPixelFloats(x, y, PixelType());
	raw  = CalculateRaw(v);
      }
	
      vector<float> norm = DoNormalize(raw);
      
      return norm;
    }

    /** Function that converts RGB values to raw vector
	\param v RGB values
    */
    virtual vector<float> CalculateRaw(const vector<float>& v) const {
      if (PixelVerbose())
	cout << "Dummy::CalculateRaw() outputting input" << endl;

      return v;
    }

  public:
    virtual featureVector CalculateRegion(const Region& r) {
      DummyData *d=
	(DummyData*)CreateData(PixelType(),ConcatCountPerPixel(),0);
      // obs!  where is d freed ???

      // now it is (maybe not)

      const coordList *lptr=r.accessList();
      coordList::const_iterator it;

      int width = Width(true), height = Height(true);

      if(lptr==NULL){ // the region doesn't support accessing through pointer
	coordList l=r.listCoordinates();
	for(it=l.begin();it!=l.end();it++){
	  if(it->x>0&&it->y>0&&it->x<width&&it->y<height)
	    CalculateOnePixel(it->x,it->y,d);
	}
      } else
	for(it=lptr->begin();it!=lptr->end();it++)
	  if(it->x>0&&it->y>0&&it->x<width&&it->y<height)
	    CalculateOnePixel(it->x,it->y,d);
    

      bool allow_zero_count = true;
      featureVector fV=d->Result(allow_zero_count);
      deleteData(d);

      return fV;
    }

    virtual bool moveRegion(const Region &r, int from, int to) {
      coordList l=r.listCoordinates();
      DummyData *d_to,*d_from;
      d_to = (DummyData*)GetData(DataIndex(to));
      d_from = (DummyData*)GetData(DataIndex(from));

      coordList::const_iterator it;
      for(it=l.begin();it!=l.end();it++){
	CalculateOnePixel(it->x,it->y,d_to);
	UnCalculateOnePixel(it->x,it->y,d_from);
      }

      return true;
    }

    int pixel_debug;

  }; // class Dummy

} // namespace pisom

#endif // _Dummy_h_

// Local Variables:
// mode: font-lock
// End:
