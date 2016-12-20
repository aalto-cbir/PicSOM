// -*- C++ -*-	$Id: Tile.h,v 1.2 2006/06/16 12:30:12 vvi Exp $

#ifndef _TILE_H_
#define _TILE_H_

#include <Segmentation.h>

namespace picsom{
  class Tile : public Segmentation {
  public:
    /// 

    Tile();

    Tile(bool b) :Segmentation(b) {}

    /// 
    virtual ~Tile();

    /// Here are the pure virtuals overloaded:
  
    ///
    virtual Segmentation *Create() const { return new Tile(); }

    ///
    virtual const char *Version() const;
    
    ///
    virtual void UsageInfo(ostream& = cout) const;
    
    ///
    virtual const char *MethodName(bool l = false) const {
      return l ? "Tile" : "ti"; }
    
    virtual bool Process(); 
    
    virtual int ProcessOptions(int, char**);
    
    const char *Description() const 
    {return "Tiles the image to rectangular segments";};
    
  protected:

    int xTiles,yTiles;
    bool overlap;
    bool squareTiles;

  };
} // namespace picsom

#endif // _TILE_H_

// Local Variables:
// mode: font-lock
// End:
