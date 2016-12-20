#include <ChainCode.h>


static char *vcid="@(#)$Id: ChainCode.C,v 1.14 2001/06/04 13:11:20 vvi Exp $";

static ChainCode list_entry(true);

///////////////////////////////////////////////////////////////////////////

ChainCode::ChainCode() {
}

///////////////////////////////////////////////////////////////////////////

ChainCode::~ChainCode() {

}

///////////////////////////////////////////////////////////////////////////

const char *ChainCode::Version() const { 
  return vcid;
}

///////////////////////////////////////////////////////////////////////////

void ChainCode::UsageInfo(ostream& os) const { 
  os << "ChainCode :" << endl
     << "  options : " << endl
     << "    NONE yet." << endl;
}

///////////////////////////////////////////////////////////////////////////

bool ChainCode::DoProcess() {

  // GeneratePattern(1);
  InvalidateOutputChains();

  AddSegments(*GetChains(false,true));
  AnnounceChains();

  if(Verbose()>1) {
    cout << *GetChains() << endl;
    cout << "Chain codes extracted." << endl;
  }
   
  return true;

}





///////////////////////////////////////////////////////////////////////////

