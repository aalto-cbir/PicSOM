// -*- C++ -*- $Id: RegionBased.h,v 1.17 2013/02/12 22:32:30 jorma Exp $

/**
   \file RegionBased.h

   \brief Declarations and definitions of class RegionBased
   
   RegionBased.h contains declarations and definitions of class the
   RegionBased, which is a base class for feature calculations based
   on image regions.
  
   \author Jorma Laaksonen <jorma.laaksonen@hut.fi>
   $Revision: 1.17 $
   $Date: 2013/02/12 22:32:30 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/
#ifndef _RegionBased_h_
#define _RegionBased_h_

#include <Feature.h>

namespace picsom {
/// Base class for feature calculations based on image regions.
class RegionBased : public Feature {
public:
  virtual bool Incremental() const { return true; }

  virtual bool CalculateOneFrame(int f);
      
  virtual bool CalculateOneLabel(int f, int l);

  /** Process an individual region
      \param ri structure containing info about the region
      \param v pixel data from the region
      \param dst pointer to the Data object that will be used
   */
  virtual bool ProcessRegion(const Region&, const vector<float>&,
			     Data*) = 0;

  virtual featureVector CalculateRegion(const Region &r);

  /** Dump region to file
      \param ri structure containing info about the region
      \param v pixel data from the region
      \param dst pointer to the Data object that will be used
      \param fname file to dump data to
  */
  void Dump(const Region&, const vector<float>&, Data*, const string&);

//   virtual const string& DefaultZoningText() const {
//     static string flat("flat");
//     return flat;
//   }

  virtual treat_type DefaultWithinFrameTreatment() const {
    return treat_concatenate; }

  virtual treat_type DefaultBetweenFrameTreatment() const {
    return treat_separate; }

  virtual treat_type DefaultBetweenSliceTreatment() const {
    return treat_separate; }

  virtual pixeltype DefaultPixelType() const { return pixel_grey; }

//   virtual void AddExtraDescription(XmlDom&) const {
//     string all;
//     for (int i=0; i<DataElementCount(); i++)
//       all += string(i?";":"")+(string)accessRegion(i);
//     xmlNewChild(root, ns, (XMLS)"region", (XMLS)all.c_str());
//   }

  /// Data class for region based data
  class RegionBasedData : public Data {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
        \param n sets the extension (jl?)
    */
    RegionBasedData(pixeltype t, int n, const Feature *p) : Data(t, n,p) {}

    /// Zeros the internal data
    virtual void Zero() {}

    /// += operator
    virtual Data& operator+=(const Data &) {
      throw "RegionBasedData::operator+=() not defined"; }
  };

protected:
  /// This constructor is called by inherited class constructors.
  RegionBased() : do_dump(false) {}

  /// This constructor doesn't add an entry in the methods list.
  RegionBased(bool) : Feature(false) {}

  virtual bool ProcessOptionsAndRemove(list<string>&);

  //   virtual int DataElementCount() const {
  //     int n = region_descr.size();  // obs! or _spec ???
  //     if (LabelVerbose())
  //       cout << "RegionBased::DataElementCount() = " << n << endl;
  //     return n;
  //   }

  // virtual bool ZeroIsBackground() const { return false; }

  //   virtual vector<string> UsedDataLabels() const {
  //     vector<string> l;
  //     int j = 1;  // or should this be 0 ??
  //     list<string>::const_iterator i;
  //     for (i=region_descr.begin(); i!=region_descr.end(); i++) {
  //       // obs! or _spec ???
  //       char tmp[100];
  //       sprintf(tmp, "%d", j++);
  //       l.push_back(tmp);  // might sometime be *i
  //     }
  
  //     if (LabelVerbose())
  //       ShowDataLabels("RegionBased::UsedDataLabels", l, cout);
  
  //     return l;
  //   }

//   /** Return the region with a specified index
//       \param l the region index
//       \return the regioninfo of the region
//   */
//   const rectangularRegion& accessRegion(int l) const {
//     list<rectangularRegion>::const_iterator i = region_spec.begin();
//     while (i!=region_spec.end() && l--)
//       i++;
//     if (i==region_spec.end())
//       throw "regioninfo index out of range";
//     return *i;
//   }

//   /** Return the size of region with index i
//       \param i the region index
//       \return the region size 
//   */
//   int RegionSize(int i) const { return accessRegion(i).size(); }

//   /// Set region specifications from the descriptions and the segmentation
//   void SetRegionSpecifications();

  // data members begin:

  /// true if we should automatically dump the data to a file while calculating
  bool do_dump;

  ///
  string storeregion;

};
}
#endif // _RegionBased_h_

// Local Variables:
// mode: font-lock
// End:
