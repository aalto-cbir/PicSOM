#include <SpringModel.h>

namespace picsom {

static const string vcid="@(#)$Id: SpringModel.C,v 1.4 2009/04/30 10:24:44 vvi Exp $";

// static SpringModel list_entry(true);

///////////////////////////////////////////////////////////////////////////

SpringModel::SpringModel() {
  /*
  // use gradient-based optimization
  Iterate = true;

  // no debug prints,please
  DEBUG = 0;
  */
}

///////////////////////////////////////////////////////////////////////////

// SpringModel::~SpringModel() {
// 
// }

///////////////////////////////////////////////////////////////////////////

const char *SpringModel::Version() const { 
  return vcid.c_str();
}

///////////////////////////////////////////////////////////////////////////

void SpringModel::UsageInfo(ostream& os) const { 
  os << "SpringModel :" << endl;
}

///////////////////////////////////////////////////////////////////////////

 int SpringModel::ProcessOptions(int argc, char** /*argv*/) { 
  int argc_old=argc;

/*
  int res;
  while(argv[0][0]== '-' && argc > 1) {

    // "-noiter"
    res = strcmp(&argv[0][1],"noiter");
    if(res == 0) {
      Iterate = false;
      argc--;
      argv++;
    }
    // "-m"
    if( argv[0][1] == 'm' ) {
      argc--;
      argv++;

      res = strncmp(&argv[0][0],"r-eye=",6);
        // "-m r-eye=r_eye2.png"
        if(res == 0) {
          strcpy(Fname_R_EYE, &argv[0][6]);
          if(Verbose()) 
            cout <<"Using -m r-eye="<<Fname_R_EYE<<" as parameter."<<endl;

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"l-eye=",6);
        // "-m l-eye=l_eye2.png"
        if(res == 0) {
          strcpy(Fname_L_EYE, &argv[0][6]);
          if(Verbose()) 
            cout <<"Using -m l-eye="<<Fname_L_EYE<<" as parameter."<<endl;

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"nose=",5);
        // "-m nose=nose2.png"
        if(res == 0) {
          strcpy(Fname_NOSE, &argv[0][5]);
          if(Verbose()) 
            cout <<"Using -m nose="<<Fname_NOSE<<" as parameter."<<endl;

          argc--;
          argv++;
        }

      res = strncmp(&argv[0][0],"mouth=",6);
        // "-m mouth=mouth2.png"
        if(res == 0) {
          strcpy(Fname_MOUTH, &argv[0][6]);
          if(Verbose()) 
            cout <<"Using -m mouth="<<Fname_MOUTH<<" as parameter."<<endl;

          argc--;
          argv++;
        }
    }

  }
  */
  return argc_old-argc;
}

///////////////////////////////////////////////////////////////////////////

bool SpringModel::Process() {
  if (Verbose()>1)
    ShowLinks();

  return ProcessNextMethod();
}

///////////////////////////////////////////////////////////////////////////

 SpringModel::state_t SpringModel::Optimize(const SpringModel::state_t&
					    start,
					    const SpringModel::state_t&
					    /*low*/,
					   const SpringModel::state_t&
					    /*high*/) {
  return start;
}

///////////////////////////////////////////////////////////////////////////

}

// Local Variables:
// mode: font-lock
// End:
