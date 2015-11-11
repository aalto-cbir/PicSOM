// -*- C++ -*- $Id: TemplateBased.h,v 1.13 2008/10/31 09:44:27 jorma Exp $
/**
   \file TemplateBased.h

   \brief Declarations and definitions of class TemplateBased
   
   TemplateBased.h contains declarations and definitions of class
   TemplateBased, which is an abstract class that calculates the
   features from some neighbourhood of each pixel. The neighbourhood
   is defined by a template, which is given as a vector of x,y-offset
   pairs.
  
   \author Mats Sjöberg <mats.sjoberg@hut.fi>
   $Revision: 1.13 $
   $Date: 2008/10/31 09:44:27 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo So many things, so little time...
*/

#ifndef _TemplateBased_h_
#define _TemplateBased_h_

#include <PixelBased.h>

/** The class TemplateBased is an abstract class that calculates the features
    from some neighbourhood of each pixel. The neighbourhood is defined by a 
    template, which is given as a vector of x,y-offset pairs.
*/
namespace picsom {
class TemplateBased : public PixelBased {
public:

  typedef pair<int,int> tcomp;                ///< x,y-offset pair
  typedef vector<tcomp> templatetype;         ///< template = vector of pairs
  
  /// map type used for caching ready-made templates
  typedef map<string, templatetype> tmaptype;
  static const templatetype null_template;    ///< used for returning errors

  /** A function that returns the current template
      \return the current templatetype
   */
  virtual const templatetype& GetTemplate() const = 0;

  /** Short-cut function that gives the length of the template
      \return current template length
  */
  virtual int GetTemplateLength() const { 
    static const int template_size = GetTemplate().size();
    return template_size;
  }

  /** Returns a template for a w x h rectangle neighbourhood.
      Uses a cache so that a certain neighbourhood template is calculated
      only once. 
      \param w width
      \param h height
      \return a template
  */
  static const templatetype& RectangleNeighborhood(int w, int h) {
    char n[100];
    sprintf(n, "rect:%dx%d", w, h);

    // try to find the template from the cache and return it if successfull
    const templatetype &t = get_template(n);
    if (t != null_template)
      return t;

    /* otherwise generate the template, insert it into the chache 
       and then return it */
    return insert_template(n,create_rectangle(w,h));
  }

  /** Returns a template for a n x n square neighbourhood
      \param n side length of the square
      \return a template
  */
  static const templatetype& SquareNeighborhood(int n) {
    return RectangleNeighborhood(n, n);
  }

  /** Returns a template for a 3x3 neighbourhood
      \return a template
  */
  static const templatetype& ThreeSquare() { return SquareNeighborhood(3); }

  /** Returns a template for a 5x5 neighbourhood
      \return a template
  */
  static const templatetype& FiveSquare() { return SquareNeighborhood(5); }

  /** Returns a template for a displacement, (0,0) and (dx,dy)
      \return a template
  */
  static const templatetype& Displacement(int dx, int dy) {
    char n[100];
    sprintf(n,"disp:%d %d",dx,dy);
    
    const templatetype &t = get_template(n);
    if (t != null_template)
      return t;
    return insert_template(n,create_displacement(dx,dy));
  }

  /// TemplateBasedData is simply a container for any template-based data
  class TemplateBasedData : public PixelBasedData {
  public:
    /// Constructor, doesn't do anything exciting...
    TemplateBasedData(pixeltype t, int n,const Feature *p) : PixelBasedData(t, n,p) {}
  };

protected:
  /// Constructor
  TemplateBased() {}

  /// Constructor
  TemplateBased(bool) : PixelBased(false) {}

  virtual void CalculateOnePixel(int x, int y, PixelBasedData *d);
  virtual void UnCalculateOnePixel(int x, int y, PixelBasedData *d);

  /** Checks that the template is within the image boundaries at point x,y
      \param x x coordinate
      \param y y coordinate
      \return true if template is within the boudaries
  */
  virtual bool CheckTemplateBoundaries(int x, int y);

  /** Processes a certain point x,y with the current template
      \param x x coordinate
      \param y y coordinate
      \param d data storage for the segment
      \return true if ok
   */

  virtual bool ProcessPointWithTemplate(int x, int y, TemplateBasedData *d
					,bool remove=false);
  
  /** Performs some operation based on the centre point and a template point.
      \param x       centre point x-coordinate
      \param y       centre point y-coordinate
      \param nx      template point x-coordinate
      \param ny      template point y-coordinate
      \param d       pointer data storage of the segment
      \param i       index in the template vector
  */
  virtual float ProcessPoints(int x, int y, int nx, int ny, 
			      TemplateBasedData *d, int i) = 0;

private:
  /// map used for caching ready-made templates
  static tmaptype tmap;

  /** Returns the template matching the string, null_template if not found
      \param n string to match
      \return the template
  */
  static const templatetype& get_template(const string n) {
    tmaptype::iterator i = tmap.find(n);
    if (i!=tmap.end())
      return i->second;
    return null_template;
  }

  /** Inserts a template into the cache map
      \param n the name of the template (used as key in the mapping)
      \param t the template to insert
      \returns the same template
   */
  static const templatetype& insert_template(const string n,
					     const templatetype& t) {
    tmap.insert(tmaptype::value_type(n,t));
    return t;
  }

  /** Creates the actual w x h rectangle template
      \param w width
      \param h height
      \return the template
  */
  static const templatetype& create_rectangle(int w, int h) {
    templatetype *t = new templatetype;

    int mx = w/2, my = h/2;
    for (int j=-my; j<h-my; j++) 
      for (int i=-mx; i<w-mx; i++)
	if (i != 0 || j != 0) 
	  t->push_back(tcomp (i,j));
    
    return *t;
  }

  /** Creates the actual displacement template
      \param dx the x displacement
      \param dy the y displacement
      \return the template
  */
  static const templatetype& create_displacement(int dx, int dy) {
    templatetype *t = new templatetype;
    
    t->push_back(tcomp (0,0));
    t->push_back(tcomp (dx,dy));

    return *t;
  }
};
}
#endif // _TemplateBased_h_

// Local Variables:
// mode: font-lock
// End:
