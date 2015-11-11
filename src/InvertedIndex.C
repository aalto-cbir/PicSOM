// -*- C++ -*-  $Id: InvertedIndex.C,v 2.5 2011/12/14 13:41:18 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <InvertedIndex.h>
#include <DataBase.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string InvertedIndex_C_vcid =
    "@(#)$Id: InvertedIndex.C,v 2.5 2011/12/14 13:41:18 jorma Exp $";

  /////////////////////////////////////////////////////////////////////////////

  InvertedIndex::InvertedIndex(DataBase *d, const string& n, const string& f,
			       const string& p,
			       const string& ps, const Index* src) : 
    Index(d, n, f, p, ps, src) { 
  }

  /////////////////////////////////////////////////////////////////////////////

  InvertedIndex::~InvertedIndex() {
  }

  /////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////

  bool InvertedIndex::ReadMetaDataFile(bool force, bool nodata, bool wrlocked, 
			       string filename, size_t maxn) {
    bool debug = false;
    string msg = "Index::ReadMetaDataFile() : ";

    if (force || /*nodata ||*/ wrlocked) {
      cerr << "Index::ReadMetaDataFile(" << force << ", " << nodata << ", "
	   << wrlocked << ")" << endl;
      return ShowError(msg+"strange arguments");
    }

    // obs! feature_target should be solved here!
    if (filename=="")
      filename = metfile_str;
    ifstream met(filename.c_str());
    if (!met)
      return ShowError(msg+"could not read <"+filename+">");

    FloatVectorSet cols(DataSetSize());
    string comment = "Automatically generated meta data\n"
      "Field names (components):\n";

    Tic("ReadMetaDataFile");

    size_t linecount=0;
    for (;;) {
      string line;
      getline(met, line);
      if (!met)
	break;
    
      if (line.find_first_not_of(" \t")==string::npos)
	continue;

      if (line[0]=='#') {
	if (line.find("indices")!=string::npos)
	  binary_classindices = true;

	continue;
      }

      int i = line.size()-1;
      while (i>=0 && isspace(line[i]))
	line.erase(i--);

      Tic("GroundTruthExpression");
      bool expand = false; // was true (and much slower) until 5.4.2006
      ground_truth ivec = db->GroundTruthExpression(line, feature_target, -1,
						    expand);
      Tac("GroundTruthExpression");

      if (debug)
	db->GroundTruthSummaryTable(ivec);

      if (ivec.empty()) {
	meta_data_fields.clear();
	return ShowError(msg+"<"+filename+"> field \"", line, "\" not known");
      }

      if (!nodata) {
	Tic("loop-1");
	FloatVector fvec(ivec.size());
	for (int j=0; j<fvec.Length(); j++)
	  fvec[j] = ivec[j];
	cols.AppendCopy(fvec);
	linecount++;
	Tac("loop-1");
      }

      int ncols = 1;
      pair<string,int> mdf(line, ncols);
      meta_data_fields.push_back(mdf);

      stringstream ss;
      ss << " " << line << " (" << ncols << ")";;;
      comment += ss.str();
      // cout << "!!! [" << line << "] OK !!!" << endl;
      if (maxn != 0 && linecount >= maxn)
	break;
    }

    if (!nodata) {
      bool check_type = true, check_restr = true;  // added 3.5.2006
      Tic("loop-2");
      data.Delete();
      data.VectorLength(cols.Nitems());
      for (int i=0; i<cols.VectorLength(); i++) {
	if (check_type && !db->ObjectsTargetTypeContains(i, feature_target))
	  continue;

	if (check_restr && !db->OkWithRestriction(i))
	  continue;

	FloatVector v(cols.Nitems(), NULL, db->LabelP(i));
	v.Number(i);
	for (int j=0; j<v.Length(); j++)
	  v[j] = cols[j][i];
	data.AppendCopy(v);
      }
      data.SetDescription(comment.c_str());
      Tac("loop-2");
    }

    Tac("ReadMetaDataFile");

    string log = "Read meta data file <"+ShortFileName(filename)+">";
    if (!nodata)
      log += " and processed to "+ToStr(data.Nitems())+" vectors of length "+
	ToStr(data.VectorLength());
    else
      log += " without processing";

    WriteLog(log);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

