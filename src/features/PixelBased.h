// -*- C++ -*- $Id: PixelBased.h,v 1.45 2013/03/14 09:49:15 mats Exp $

/**
   \file PixelBased.h

   \brief Declarations and definitions of class PixelBased
   
   PixelBased.h contains declarations and definitions of the class
   PixelBased, which is a base class for all feature calculations
   based on individual pixels.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.45 $
   $Date: 2013/03/14 09:49:15 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _PixelBased_h_
#define _PixelBased_h_

#include <Feature.h>

namespace picsom {
  /// Base class for feature calculations based on individual pixels.
  class PixelBased : public Feature {
  public:
    virtual bool Incremental() const { return true; }

    virtual bool ProcessOptionsAndRemove(list<string>& l);

    virtual bool ReInitializable() const { return false; } // should be true...

    virtual bool ReInitialize() {
      // something ?
      return Feature::ReInitialize();
    }

    virtual bool HasRawOutMode() const { return true; }

    virtual bool HasRawInMode() const { return true; }

    virtual bool CalculateOneFrame(int f) { return CalculateCommon(f, true); }
      
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
    class PixelBasedData : public Data {
    public:
      /** Constructor
	  \param t sets the pixeltype of the data
	  \param n sets the extension (jl?)
      */
      PixelBasedData(pixeltype t, int n, const Feature *p) : Data(t, n, p) {
	Zero();
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
	  cout << "PixelBasedData::IncrementCount(), count=" << count << endl;
      }

      /// Deccrement the counter
      void DecrementCount() {    
	count--;
	if(count<0)
	  throw "PixelBasedData::DecrementCount() : count becomes negative"; 
	if (PixelVerbose())
	  cout << "PixelBasedData::DecrementCount(), count=" << count << endl;
      }

      /** Adds data from one pixel to the internal storage
	  \param v the data vector to add
      */
      virtual void AddPixel(const vector<float>& v, int x, int y) {
	if (PixelVerbose())
	  cout << "PixelBasedData::AddPixel(const vector<float>&)" << endl;;

	IncrementCount();
	addToHistogram(v);
	Parent()->ProcessRawOutVector(v, x, y);
      }

      /**  Removes data from one pixel from the internal storage
	   \param v the data vector to remove
      */

      virtual void RemovePixel(const vector<float>& v) {
	if (PixelVerbose())
	  cout << "PixelBasedData::RemovePixel(constvector<float>&)" << endl;;
	DecrementCount();
	addToHistogram(v, -1);
      }
    
      /**  Adds data from one pixel to the internal storage
	   \param v the data vector to add
      */
      virtual void AddPixel(const vector<int>&, int, int) {
	if (PixelVerbose())
	  cout << "PixelBasedData::AddPixel(const vector<int>&)" << endl;;
	IncrementCount();
      }

      /**  Removes data from one pixel from the internal storage
	   \param v the data vector to remove
      */
      virtual void RemovePixel(const vector<int>&) {
	if (PixelVerbose())
	  cout << "PixelBasedData::RemovePixel(constvector<int>&)" << endl;;
	DecrementCount();
      }

      /// += operator (not used jl?)
      virtual Data& operator+=(const Data &) {
	throw "PixelBasedData::operator+=() not defined"; }

    private:
      /// the counter
      int count;
    };

  protected:
    /// This constructor is called by inherited class constructors.
    PixelBased() { pixel_debug = 0; }

    /// This constructor doesn't add an entry in the methods list.
    PixelBased(bool) : Feature(false) {}

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
    virtual void CalculateOnePixel(int x, int y, PixelBasedData *d) {
      // cox::tictac::func tt(tics, "PixelBased::CalculateOnePixel");
      vector<float> v = ObtainRawVector(x, y);
      d->AddPixel(v, x, y);
    }

    /** Function that reverses the calculation for one pixel
	\param x x-coordinate of the pixel
	\param y y-coordinate of the pixel
	\param i the dataindex of the segment of the pixel
    */
    virtual void UnCalculateOnePixel(int x, int y, PixelBasedData *d) {
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
	cout << "PixelBased::CalculateRaw() outputting input" << endl;

      return v;
    }

  public:
    virtual featureVector CalculateRegion(const Region& r) {
      PixelBasedData *d=
	(PixelBasedData*)CreateData(PixelType(),ConcatCountPerPixel(),0);
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
      PixelBasedData *d_to,*d_from;
      d_to = (PixelBasedData*)GetData(DataIndex(to));
      d_from = (PixelBasedData*)GetData(DataIndex(from));

      coordList::const_iterator it;
      for(it=l.begin();it!=l.end();it++){
	CalculateOnePixel(it->x,it->y,d_to);
	UnCalculateOnePixel(it->x,it->y,d_from);
      }

      return true;
    }

    int pixel_debug;

  }; // class PixelBased

} // namespace pisom

#endif // _PixelBased_h_

// Local Variables:
// mode: font-lock
// End:
