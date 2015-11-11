// -*- C++ -*-  $Id: PreCalculatedIndex.C,v 2.6 2011/12/14 13:41:18 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <PreCalculatedIndex.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string PreCalculated_C_vcid =
    "@(#)$Id: PreCalculatedIndex.C,v 2.6 2011/12/14 13:41:18 jorma Exp $";

  static PreCalculatedIndex list_entry(true);

  bool PreCalculatedIndex::debug_precalculated = false;

  /////////////////////////////////////////////////////////////////////////////

  PreCalculatedIndex::PreCalculatedIndex(DataBase *d, const string& n, 
					 const string& f, 
					 const string& p, const string& ps,
                                         const Index* src) : 
    Index(d, n, f, p, ps, src) { 

    string msg = "PreCalculatedIndex : ";
    
    prefile_str = FindFile(Name(), ".pre");
    datindex = -1;
    component = "";

    FeatureTarget(target_video); // should this be somehow smarter?
    if (feature_target==target_no_target)
      ShowError(msg+"feature_target==target_no_target");  

    if (IsPreCalculatedFile(prefile_str))
      ReadPreCalculatedFile();
    else 
      ShowError(msg+"could not recognize <"+prefile_str+">");

    if (!precalc.size())
      WriteLog(msg+"empty precalculated index <"+prefile_str+">");
    else 
      WriteLog(msg+"created PreCalculatedIndex containing "+ 
	       ToStr(precalc.size())+" items");
  }

  /////////////////////////////////////////////////////////////////////////////

  PreCalculatedIndex::~PreCalculatedIndex() {
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PreCalculatedIndex::ReadDataFile(bool /*force*/, bool /*nodata*/,
					bool /*wrlocked*/) {

    string msg = "PreCalculatedIndex::ReadDataFile() : ";

    Tic("ReadDataFile");

    bool ok = true;
    FloatVectorSet datatmp;
    datatmp.FileName(datfile_str);
    datatmp.NoDataRead(false);

    if (!datatmp.Read(datfile_str)) {
      ShowError(msg+"reading <", datfile_str, "> failed");
      ok = false;
    } else {
      stringstream tmptxt;
      tmptxt << " (dim=" << datatmp.VectorLength() << ", "
	     << datatmp.Nitems() << " vectors)";
      WriteLog(msg+"read data file ", ShortFileName(datfile_str), 
	       tmptxt.str());
    }

    if (datindex<0) {
      if (component.size()) {
	datindex = datatmp.GetComponentIndex(component);
	WriteLog(msg+"setting datindex=", ToStr(datindex), 
		 " based on component=", component);
      } else {
	datindex=0;
	WriteLog(msg+"datindex not set, using datindex=0");
      }
    }

    if (datindex > datatmp.VectorLength()) {
      ShowError(msg+"datindex > VectorLength()");
      ok = false;
    } 
    else if (ok)
      for (int i=0; i<datatmp.Nitems(); i++) {
	precalc[datatmp[i].Label()] = datatmp[i][datindex];
      }

    Tac("ReadDataFile");

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool PreCalculatedIndex::ReadPreCalculatedFile() {
    bool debug = debug_precalculated;
    string msg = "PreCalculatedIndex::ReadPreCalculatedFile() : ";

    ifstream pre(prefile_str.c_str());
    if (!pre)
      return ShowError(msg+"could not read <"+prefile_str+">");

    Tic("ReadPreCalculatedFile");

    size_t linecount=0;
    bool datfilefound = false;
    for (;;) {
      string line;
      getline(pre, line);
      if (!pre)
	break;
      if (debug)
	cout << linecount << " : " << line << endl;
      linecount++;

      vector<string> parts = SplitInSpaces(line);
      if (line[0]=='#') { // # PRECALCULATEDFILE bar.dat [index=1|comp=a]
	for (size_t i=0; i<parts.size(); i++) {
	  if (parts[i].find(".dat")!=string::npos) {
	    datfilefound = true;
	    if (debug)
	      cout << "FOUND DAT FILE  : <" << parts[i] << ">" << endl;
	    datfile_str = FindFile(parts[i]);
	  } else {
	    size_t idxpos = parts[i].find("index=");
	    size_t cmppos = parts[i].find("comp=");		    
	    if (idxpos !=string::npos) {
	      string idxval = parts[i].substr(idxpos+6);
	      datindex = atoi(idxval.c_str());
	      if (debug)
		cout << "SETTING datindex based on index= <" << idxval 
		     << ">" << endl;
	    } else if (cmppos !=string::npos) {
	      component = parts[i].substr(cmppos+5);
	      if (debug)
		cout << "SETTING datindex based on comp= <" << component 
		     << ">" << endl;
	    }
	  }
	}
	if (datfilefound) {
	  if (debug)
	    cout << "READING DAT FILE: <" << datfile_str << ">" << endl;
	  if (!ReadDataFile())
	    ShowError(msg+"could not read <"+datfile_str+">");
	}

      } else if (!datfilefound) { // precalculated scores directly in .pre file
	if (parts.size()!=2) {
	  ShowError(msg+"unrecognized line:"+line);
	} else {
	  precalc[parts[1]] = atof(parts[0].c_str());
	}
      }
    }

    Tac("ReadPreCalculatedFile");

    WriteLog("Read precalculated file <"+ShortFileName(prefile_str)+"> (" +
	     ToStr(linecount) + " lines)");

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////
} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

