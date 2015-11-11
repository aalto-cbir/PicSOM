// -*- C++ -*- $Id: TemplateBased.C,v 1.11 2008/09/29 10:22:40 markus Exp $

#include <TemplateBased.h>

namespace picsom {
//static const char *vcid = "$Id: TemplateBased.C,v 1.11 2008/09/29 10:22:40 markus Exp $";
TemplateBased::tmaptype TemplateBased::tmap;
const TemplateBased::templatetype TemplateBased::null_template = 
TemplateBased::templatetype(1,TemplateBased::tcomp(0,0));

///////////////////////////////////////////////////////////////////////////////

void TemplateBased::CalculateOnePixel(int x, int y, PixelBasedData *d) {
  bool ok = true;

  ok = CheckTemplateBoundaries(x, y);

  if (ok)
    ProcessPointWithTemplate(x, y, (TemplateBasedData*)d);
}

void TemplateBased::UnCalculateOnePixel(int x, int y, PixelBasedData *d) {
  bool ok = true;

  ok = CheckTemplateBoundaries(x, y);

  if (ok)
    ProcessPointWithTemplate(x, y, (TemplateBasedData*)d,true);
}


///////////////////////////////////////////////////////////////////////////////
/* checks that the template doesn't go outside the image boundaries when
   placed on (x,y) */
bool TemplateBased::CheckTemplateBoundaries(int x, int y) {
  bool ok = true;
  int w = Width(), h = Height();

  const templatetype& t = GetTemplate();
  
  for (int i=0; i<GetTemplateLength() && ok; i++) {
    int nx = x+t[i].first, ny = y+t[i].second;
    if (nx < 0 || nx >= w || ny < 0 || ny >= h)
      ok = false;
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool TemplateBased::ProcessPointWithTemplate(int x, int y, 
					     TemplateBasedData *d,
					     bool remove) {
  bool ok = true;
  templatetype t = GetTemplate();
  vector<float> v;
  for (int i=0; i<GetTemplateLength() && ok; i++) {
    int nx = x+t[i].first, ny = y+t[i].second;
    v.push_back(ProcessPoints(x,y,nx,ny,d,i));
  }
  
  if (remove)
    d->RemovePixel(v);
  else
    d->AddPixel(v, x, y);

  return ok;
}

}
// Local Variables:
// mode: font-lock
// End:
