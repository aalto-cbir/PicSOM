// -*- C++ -*-	$Id: CreateThumbnail.h,v 1.4 2004/03/26 11:34:34 jorma Exp $

#ifndef _CREATETHUMBNAIL_H_
#define _CREATETHUMBNAIL_H_

#include <Segmentation.h>

namespace picsom {

  class CreateThumbnail : public Segmentation {
  public:
    /// 
    CreateThumbnail();

    /// 
    CreateThumbnail(bool b) : Segmentation(b) {}

    /// 
    virtual ~CreateThumbnail();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new CreateThumbnail(); }  

    ///
    const char *Version() const;

    ///
    virtual void UsageInfo(ostream& = cout) const;

    ///
    virtual const char *MethodName(bool l = false) const {
      return l?"CreateThumbnail":"ct"; }

    ///
    virtual bool Process();

    ///
    virtual int ProcessOptions(int, char**);

    ///
    virtual const char *Description() const {return "createthumbnail";};

  protected:
    ///
    bool WriteThumbnail(const string& filename, const int x0, const int y0,
			const int x1, const int y1, const int w, const int h,
			const bool coloroutput = true);

    /// obsoleted...
    bool WriteThumbnailOld(const string& filename, const int x0, const int y0,
			   const int x1, const int y1, const double scale,
			   const bool coloroutput = true,
			   const float normalizer = 1);

    /// obsoleted...
    unsigned char* Rescale(int x0, int y0, int x1, int y1,
			   double scale, float normalizer, size_t&);

    string CreateOutputFileName(const string& inputname) const {
      string thumbname = inputname;
      size_t dot = thumbname.rfind('.');
      if (dot!=string::npos) {
	string ext = thumbname.substr(dot);
	if (ext==".gz" || ext==".bz2")
	  dot = thumbname.rfind('.', dot-1);
      }

      if (dot!=string::npos)
	thumbname.resize(dot);

      if (!tn_dir_output)
	return thumbname+"_t.png";

      size_t slash = thumbname.rfind('/');
      if (slash!=string::npos)
	thumbname.erase(0, slash+1);

      string dir = "tn-100x100/";
      return thumbname.insert(0, dir)+".png";
    }

    /// false by default, output goes to xxx_t.png . 
    /// When true, output goes to tn-100x100/xxx.png . 
    bool tn_dir_output;

  }; // class CreateThumbnail

} // namespace picsom


#endif // _CREATETHUMBNAIL_H_

// Local Variables:
// mode: font-lock
// End:
