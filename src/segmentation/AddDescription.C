// -*- C++ -*-  $Id: AddDescription.C,v 1.16 2014/10/29 14:02:31 jorma Exp $
// 
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <AddDescription.h>

using std::cout;

namespace picsom {
  static const string vcid =
    "@(#)$Id: AddDescription.C,v 1.16 2014/10/29 14:02:31 jorma Exp $";
  
  static AddDescription list_entry(true);
  
  ///////////////////////////////////////////////////////////////////////////

  AddDescription::AddDescription() {
    methodname_long = "AddDescription";
    methodname_short = "ad";
  }

  ///////////////////////////////////////////////////////////////////////////

  AddDescription::~AddDescription() {
  }

  ///////////////////////////////////////////////////////////////////////////

  const char *AddDescription::Version() const { 
    return vcid.c_str();
  }

  ///////////////////////////////////////////////////////////////////////////

  void AddDescription::UsageInfo(ostream& os) const { 
    os << "AddDescription :" << endl
       << "  options : " << endl
       << "    -n   <short/longmethodname> to replace ad/AddDescription"
       << endl
       << "    -a   <name> <type> <value>  add frame result to all frames." 
       << endl
       << "    -aa  <name>.<type>=<value>  add frame result to all frames." 
       << endl
       << "    -ag  <name> <type> <value>  add specified file result" << endl
       << "    -aag <name>.<type>=<value>  add specified file result" << endl

       << "    -as <framenr> <name> <type> <value> add result to one frame"
       << endl
       << "    -aas <framenr> <name>.<type>=<value> add result to one frame"
       << endl
      
       << endl
       << "Any number of annotations can be given on command line." << endl;
      // << "The ordering of various results in the resultlists is not" << endl
      //<< "guaranteed if multiple annotations are given." << endl;

  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  int AddDescription::ProcessOptions(int argcx, char **argvx) { 
    Option option;
    pendingAnnotation ann;
    
    vector<string> a;
    bool is_opt = true;
    for (int i=0; i<argcx; i++) {
      string s = argvx[i];
      if (s[0]!='-')
	is_opt = false;

      if ((true||is_opt) && s=="-f" && i+1<argcx) {
	string f = FileToString(argvx[i+1]);
	size_t p = 0;
	for (;;) {
	  size_t q = f.find_first_not_of(" \t\n\r", p);
	  if (q==string::npos)
	    break;

	  size_t r = f.find_first_of(" \t\n\r", q+1);
	  string o = r==string::npos ? f.substr(q) : f.substr(q, r-q);
	  a.push_back(o);

	  p = r;
	}
	i++;
	continue;
      }

      a.push_back(s);
    }

    int argc = a.size(); // , argc_old = argc;

    size_t i = 0;
    while (i+1<a.size() && a[i][0]=='-') {
      option.reset();

      switch (a[i][1]) {
      case 'n': {
	methodname_short = a[i+1];
	size_t p = methodname_short.find('/');
	if (p!=string::npos) {
	  methodname_long = methodname_short.substr(p+1);
	  methodname_short.erase(p);
	}
	argc--;
	i++;
	break;
      }

      case 'a': {
	bool onearg     = a[i][2]=='a';
	bool fileresult = false;
	bool oneframe   = false;

	switch (a[i][2+(onearg?1:0)]) {
	case 'g':
	  fileresult = true;
	  break;

	case 's':
	  oneframe = true;
	}
 
	if (argc<5-(onearg ? 2 : 0)+(oneframe?1:0)) {
	  stringstream ss;
	  ss << "argc=" << argc << " onearg=" << onearg
	     << " oneframe=" << oneframe;
	  EchoError("Not enough commandline arguments for switch -a* "+
		    ss.str());
	  break;
	}  

	option.option = a[i];

	for (int ctr=0; ctr<1+(onearg?0:2) + (oneframe?1:0); ctr++)
	  option.addArgument(a[i+ctr+1]);
      
	if (oneframe) {
	  if (sscanf(a[i+1].c_str(),"%d", &ann.frame)!=1)
	    throw string("AddDescription::ProcessOptions(): could not parse "
			 "frame number from ") + a[i];
	  i++;
	  argc--;
	}

	if (onearg) {
	  string annstr(a[i+1]);
	  size_t dotpos=annstr.find('.');
	  if (dotpos==string::npos)
	    throw string("AddDescription::ProcessOptions(): could not "
			 "find . in ")+annstr;

	  size_t eqpos=annstr.find('=');
	  if (eqpos==string::npos)
	    throw string("AddDescription::ProcessOptions(): could not "
			 "find = in ")+annstr;

	  ann.result.name  = annstr.substr(0, dotpos);
	  ann.result.type  = annstr.substr(dotpos+1, eqpos-dotpos-1);
	  ann.result.value = annstr.substr(eqpos+1);

	  i++;
	  argc--;

	} else {
	  ann.result.name  = a[i+1];
	  ann.result.type  = a[i+2];
	  ann.result.value = a[i+3];
	  argc-=3;
	  i+=3;
	}

	if (Verbose()>1) {
	  cout << "Name is "  << ann.result.name << endl;
	  cout << "type is "  << ann.result.type << endl;
	  cout << "value is " << ann.result.value << endl;
	}
      
	if (Verbose() && !CheckIfKnownType(ann.result.type))
	  cout << "WARNING: result type " << ann.result.type
	       << " not predefined."
	       << endl;
      
	// used to be "ann.allframes  = !oneframe" until 2012-06-05
	ann.allframes  = !oneframe && !fileresult;
	ann.fileresult = fileresult;

	// AddResultToXML(result);
	pending.push_back(ann);

	break;
      }

      default:
	EchoError(string("unknown option ")+a[i]);
      } // switch

      if (!option.option.empty())
	addToSwitches(option);

      argc--;
      i++;
    } // while

    // obs! this is temporarily? hardcoded...
    return argcx-1;  // so we assume there will be only the filename...

    // return argc_old-argc;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool AddDescription::Process() {
    if (Verbose()>2)
      cout << "AddDescription::Process() pending.size() is "
	   << pending.size() << endl;

    for (size_t i=0; i<pending.size(); i++) {
      if (Verbose()>2)
	cout << "  i=" << i << " curr.frame=" << getImg()->getCurrentFrame()
	     << " : " << pending[i].Str() << endl;

      if (pending[i].fileresult) {
	// nothing?

      } else if (pending[i].allframes || 
		 pending[i].frame==getImg()->getCurrentFrame()) {
	if (Verbose()>2)
	  cout << "  writing " << pending[i].Str() << endl;
	getImg()->writeFrameResultToXML(this, pending[i].result);
      }
    }

    return ProcessNextMethod();
  }
  
  ///////////////////////////////////////////////////////////////////////////

  bool AddDescription::ProcessGlobal() {
    if (Verbose()>2)
      cout << "AddDescription::ProcessGlobal() pending.size() is "
	   << pending.size() << endl;

    for (size_t i=0; i<pending.size(); i++) {
      if (Verbose()>2)
	cout << "  i=" << i << " curr.frame=" << getImg()->getCurrentFrame()
	     << " : " << pending[i].Str() << endl;

     if (pending[i].fileresult) {
	getImg()->writeFileResultToXML(this, pending[i].result);
      }
    }

    return ProcessGlobalNextMethod();
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  bool AddDescription::CheckIfKnownType(const string &type) {
    static PredefinedTypes predefined_types;

    for (size_t i=0; i<predefined_types.v.size(); i++)
      if (type == predefined_types.v[i])
	return true;
    
    return false;
  }
  
  ///////////////////////////////////////////////////////////////////////////
  
  string AddDescription::pendingAnnotation::Str() const {
    stringstream ss;
    ss << "(" << result.Str() << ") frame=" << frame << " allframes="
       << allframes << " fileresult=" << fileresult;
    return ss.str();
  }
  
  ///////////////////////////////////////////////////////////////////////////
}
