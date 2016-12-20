// -*- C++ -*-  $Id: CbirPinView.C,v 2.49 2016/06/22 16:17:26 jorma Exp $
// 
// Copyright 1998-2016 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science and Technology
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <CbirPinView.h>
#include <Analysis.h>

#if defined(PICSOM_HAVE_OCTAVE) && defined(PICSOM_USE_OCTAVE)

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#ifndef MKOCTFILE

#include <Valued.h>
#include <Query.h>

#include <octave/oct.h>
#include <octave/parse.h>

namespace picsom {
  ///
  static const string CbirPinView_C_vcid =
    "@(#)$Id: CbirPinView.C,v 2.49 2016/06/22 16:17:26 jorma Exp $";

  /// This is the "factory" instance of the class.
  static CbirPinView list_entry(true);

  //===========================================================================

  /// This is the principal constructor.
  CbirPinView::CbirPinView(DataBase *db, const string& args)
    : CbirAlgorithm(db, args) {

    string hdr = "CbirPinView::CbirPinView() : ";
    if (debug_stages)
      cout << TimeStamp()
	   << hdr+"constructing an instance with arguments \""
	   << args << "\"" << endl;

    kerneltype = "linr";
    linrel = "AUCB";
    mkl    = "none";
    tensor = false;
    feat   = "raw";
    ridge  = 0.1;

    map<string,string> amap = SplitArgumentString(args);
    for (map<string,string>::const_iterator a=amap.begin(); a!=amap.end(); a++)
      if (a->first=="tensor")
	tensor = a->second=="" || IsAffirmative(a->second);
      else if(a->first=="kerneltype") {
        if (Valid_kerneltype(a->second))
	  kerneltype = a->second;
	else
	  ShowError(hdr+"value kerneltype=["+a->second+"] not in linr|gaus");
      }
      else if (a->first=="linrel") {
	if (Valid_linrel(a->second))
	  linrel = a->second;
	else
	  ShowError(hdr+"value linrel=["+a->second+"] not in AUCB|1UCB|AExp|Comb");

      } else if (a->first=="mkl") {
	if (Valid_mkl(a->second))
	  mkl = a->second;
	else
	  ShowError(hdr+"value mkl=["+a->second+"] not in none|rr|1c|2c");

      } else if (a->first=="feat") {
	if (Valid_feat(a->second))
	  feat = a->second;
	else
	  ShowError(hdr+"value feat=["+a->second+"] not in raw|fsh|som");

      } else if (a->first=="ridge") {
	ridge = atof(a->second.c_str());

      } else
	ShowError(hdr+"key ["+a->first+"] not recognized");

    if (debug_stages)
      cout << TimeStamp()
	   << hdr+"constructed instance with FullName()=\"" << FullName()
	   << "\"" << endl;

    // After calling CbirAlgorithm::CbirAlgorithm(DataBase*) we should
    // have a valid DataBase pointer named database:

    if (false)
      cout << TimeStamp() << hdr
	   << "we are using database \"" << database->Name()
	   << "\" that contains " << database->Size() << " objects" << endl;

    if (!Picsom()->OctaveInializeOnce()) {
      ShowError(hdr+"OctaveInializeOnce() failed");
      return;
    }

    if (!Picsom()->OctaveAddPath(Picsom()->UserHomeDir()+
                                 "/picsom/octave/pinview", true, true)) {
      ShowError(hdr+"OctaveInializeAddPath() failed");
      return;
    }

    /*
    Matrix m(3, 3);
    m(0,0)=m(1,1)=m(2,2)=0.5;
    m(0,1)=m(1,0)=m(2,0)=m(0,2)=m(1,2)=m(2,1)=0.0;
    cout << "m=" << m << endl;
    Matrix n = m.inverse();
    cout << "n=" << n << endl;

    // http://wiki.octave.org/wiki.pl?CategoryFAQ
    ColumnVector NumRands(2);
    NumRands(0) = 9000;
    NumRands(1) = 1;
    octave_value_list f_arg, f_ret;
    f_arg(0) = octave_value(NumRands);
    f_ret = feval("rand", f_arg, 1);
    Matrix unis(f_ret(0).matrix_value());

    ColumnVector a(2);
    a(0) = 5;
    a(1) = 7;
    f_arg(0) = octave_value(a);
    f_ret = feval("picsom_octave_test", f_arg, 1);
    Matrix b(f_ret(0).matrix_value());
    cout << b << endl;
    */
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  const list<CbirAlgorithm::parameter>& CbirPinView::Parameters() const {
    static list<parameter> l;
    if (l.empty()) {
      parameter lin = { "linrel", "xsd:token", "AUCB 1UCB AExp Comb", "AUCB",
                        "LinRel algorithm" };
      parameter mkl = { "mkl",    "xsd:token", "none rr 1c 2c", "none",
			"mode of Multi-Kernel Learning" };
      parameter fea = { "feat",   "xsd:token", "raw fsh som", "raw",
			"feature extractor" };
      parameter ten = { "tensor", "xsd:boolean", "", "false",
                        "tensor ranking SVM" };
      parameter ker = { "kerneltype", "xsd:token", "linr gaus", "linr",
                        "kernel type" };
      //parameter rid = { "ridge", "xsd:float", "", "0.1",
      //                  "ridge parameter of linrel" };

      l.push_back(fea);
      l.push_back(ker);
      l.push_back(lin);
      l.push_back(mkl);
      // l.push_back(rid); // added out-commented 2011-03-01
      l.push_back(ten);
    }

    return l;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirPinView::Initialize(const Query *q, const string& s,
			       CbirAlgorithm::QueryData*& qd) const {
    string hdr = "CbirPinView::Initialize() : ";

    if (!CbirAlgorithm::Initialize(q, s, qd))
      return ShowError(hdr+"CbirAlgorithm::Initialize() failed");

    // After calling CbirAlgorithm::Initialize() we should have a valid Query
    // pointer named qd->query:

    if (false)
      cout << TimeStamp() << hdr << "query's identity is \""
	   << GetQuery(qd)->Identity() << "\" and target type \""
	   << TargetTypeString(GetQuery(qd)->Target()) << "\"" << endl;

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first calls the corresponding method of the base class.
  bool CbirPinView::AddIndex(CbirAlgorithm::QueryData *qd,
                             Index::State *f) const {
    string hdr = "CbirPinView::AddIndex() : ";
    if (!CbirAlgorithm::AddIndex(qd, f))
      return ShowError(hdr+"CbirAlgorithm::AddIndex() failed");

    // After calling CbirAlgorithm::AddIndex() we should have a list
    // of feature names in variable named features:

    if (false) {
      cout << TimeStamp() << hdr
	   << "the following features have been selected: ";
      for (size_t i=0; i<Nindices(qd); i++)
	cout << (i?", ":"") << IndexFullName(qd, i);
      cout << endl;
    }

    /* this ws added in 2.34 (why?) and removed in 2.42 because
       it prevented processing algorithm1=pinview_xxx defaultfeatures=yyy
       from settings.xml

    for (size_t i=0; i<qd->Nindices(); i++)
      if (qd->IndexShortName(i)==f->fullname && qd->IsTsSom(i)) {
        Index *idx = &qd->IndexStaticData(i);
        VectorIndex *vidx = dynamic_cast<VectorIndex*>(idx);
        if (vidx) {
          vidx->ReadDataFile();
          vidx->SetDataSetNumbers(false);
          break;
        }
      }
    */

    return true;
  }

  //===========================================================================

  /// If this virtual method is overwritten in a derived class, it typically
  /// first checks for its own parameters and then calls the
  /// corresponding method of the base class.
  bool CbirPinView::Interpret(CbirAlgorithm::QueryData *qd, const string& k,
			      const string& v, int& r) const {
    string hdr = "CbirPinView::Interpret() : ";

    if (debug_stages)      
      cout << TimeStamp()
	   << hdr+"key=\""+k+"\" value=\""+v+"\"" << endl;

    CbirPinView::QueryData *qde = CastData(qd);

    r = 1;

    if (k=="tensor") {
      qde->tensor = IsAffirmative(v);
      return true;
    }

    if (k=="kerneltype") {
      if (!Valid_kerneltype(v)) {
        r = 0;
        return ShowError(hdr+"kerneltype should be specified as "
			 "kerneltype=linr|gaus");
      }
      qde->linrel = v;
      return true;
    }    

    if (k=="linrel") {
      if (!Valid_linrel(v)) {
        r = 0;
        return ShowError(hdr+"LinRel should be specified as "
			 "linrel=AUCB|1UCB|AExp|Comb");
      }
      qde->linrel = v;
      return true;
    }

    if (k=="mkl") {
      if (!Valid_mkl(v)) {
        r = 0;
        return ShowError(hdr+"MKL should be specified as mkl=none|rr|1c|2c");
      }
      qde->mkl = v;
      return true;
    }

    if (k=="feat") {
      if (!Valid_feat(v)) {
        r = 0;
        return ShowError(hdr+"feat should be specified as feat=raw|fsh|som");
      }
      qde->feat = v;
      return true;
    }

    if (k=="ridge") {
      qde->ridge = atof(v.c_str());
      return true;
    }

    return CbirAlgorithm::Interpret(qd, k, v, r);
  }

  //===========================================================================

  ObjectList CbirPinView::CbirRound(CbirAlgorithm::QueryData *qd,
				    const ObjectList& seenin,
				    size_t maxq) const {
    bool debug_indexvec = false;

    string hdr = "CbirPinView::CbirRound() : ";

    CbirAlgorithm::CbirRound(qd, seenin, maxq);

    if (!qd->GetQuery()->HasAspectWeights()) {
      ShowError(hdr+"aspects=(weight=x.x),gaze(weight=y.y) should"
		" have been explicitly set");
      return ObjectList();
    }

    int offline = 0;
    const Analysis *a = GetQuery(qd)->GetParentAnalysis();
    if (a && a->Identity().find("A:")==0 &&
	a->Identity().find("simulate")!=string::npos)
      offline = 1;

    // const Query::algorithm_data *lower = GetQuery(qd)->NlowerAlgorithms(qd) ?
    //   GetQuery(qd)->LowerAlgorithm(qd, 0) : NULL;

    const Query::algorithm_data *lower
      = GetQuery(qd)->LowerAlgorithm(qd, 0, false);

    int verbose = PicSOM::DebugOctave()-1;
    if (verbose<0)
      verbose = 0;

    if (verbose) {
      cout << TimeStamp() << hdr << "starting " << (offline?"OFF":"ON")
	   << "LINE with " << seenin.size() << " objects already seen" << endl;
      if (lower) {
	size_t n = 0;
	for (size_t i=0; i<lower->result.size(); i++)
	  if (lower->result[i].Retained())
	    n++;

	cout << TimeStamp() << hdr << "lower algorithm [" << lower->fullname
	     << "] returns " << n << "/" << lower->result.size()
	     << " objects" << endl;
      }
    }

    ObjectList seen = seenin;
    Object::SortListByRank(seen);

    if (debug_lists)
      qd->DumpList(seen, "SEEN", false);

    size_t vlen = 33;
    vector<float> dummy(vlen);

    ObjectList list;

    Picsom()->MutexLock();

    CbirPinView::QueryData *qde = CastData(qd);
    qde->indexvec.clear();

    Matrix octseen(seen.size(), 4+vlen);
    for (size_t i=0; i<seen.size(); i++) {
      float grb = seen[i].AspectRelevance("gaze");
      float grc = (grb+1)/2; 
      if (seen[i].HasAspectRelevance("gaze-cont"))
	grc = seen[i].AspectRelevance("gaze-cont");
      octseen(i, 0) = lower ? i : seen[i].Index();
      octseen(i, 1) = seen[i].Value();  // -1 / +1
      octseen(i, 2) = grc;              // [0,1]
      octseen(i, 3) = grb;              // -1 / +1

      vector<float> gaze33 = qd->GetQuery()->FeatureVector("gaze-33",
							   seen[i].Label(),
							   true);
      for (size_t j=0; j<vlen; j++)
	octseen(i, 4+j) = (gaze33.size()==vlen ? gaze33 : dummy)[j];

      if (lower) {
	qde->indexvec.push_back(seen[i].Index());
	if (debug_indexvec)
	  cout << hdr << "INDEXVEC forward  " << seen[i].Index()
	       << " -> " << qde->indexvec.size()-1 << " SEEN" << endl;
      }
    }

    ColumnVector octunseen;

    if (lower) {
      vector<size_t> vj;
      for (size_t i=0; i<lower->result.size(); i++)
	if (lower->result[i].Retained())
	  vj.push_back(lower->result[i].Index());

      octunseen = ColumnVector(vj.size());
      for (size_t i=0; i<vj.size(); i++) {
	octunseen(i) = seen.size()+i;
	qde->indexvec.push_back(vj[i]);
	if (debug_indexvec)
	  cout << hdr << "INDEXVEC forward  " << vj[i]
	       << " -> " << qde->indexvec.size()-1 << " UNSEEN" << endl;
      }

    } else {
      vector<size_t> vj;
      for (size_t i=0; i<database->Size(); i++)
	if (database->ObjectsTargetTypeContains(i, GetQuery(qde)->Target()) &&
	    GetQuery(qde)->CanBeShownRestricted(i, true))
	  vj.push_back(i);

      octunseen = ColumnVector(vj.size());
      for (size_t i=0; i<vj.size(); i++)
	octunseen(i) = vj[i];
    }

    size_t nobj = qde->GetMaxQuestions(maxq);
    size_t nfea = qde->Nindices();
    if (verbose)
      cout << TimeStamp() << hdr << "there are " << octunseen.rows()
	   << " objects to select " << nobj << " from with " << nfea
	   << " features" << endl
	   << TimeStamp() << hdr << "ref = " << (void*)qde << " = "
	   << (long)qde << endl;

    double clickw = qd->GetQuery()->GetAspectWeight("");
    double gazew  = qd->GetQuery()->GetAspectWeight("gaze");
    int rndseed   = qd->GetQuery()->RndSeed();
    int forceread = lower!=NULL;

    octave_value_list f_arg, f_ret;
    f_arg( 0) = octave_value((long)qde);
    f_arg( 1) = octave_value(octseen);
    f_arg( 2) = octave_value(octunseen);
    f_arg( 3) = octave_value(nobj);
    f_arg( 4) = octave_value(qde->linrel);
    f_arg( 5) = octave_value(qde->mkl);
    f_arg( 6) = octave_value(qde->tensor);
    f_arg( 7) = octave_value(qde->kerneltype);
    f_arg( 8) = octave_value(qde->feat);
    f_arg( 9) = octave_value(qde->ridge);
    f_arg(10) = octave_value(clickw);
    f_arg(11) = octave_value(gazew);
    f_arg(12) = octave_value(rndseed);
    f_arg(13) = octave_value(forceread);
    f_arg(14) = octave_value(offline);
    f_arg(15) = octave_value(verbose>1?verbose-1:0);
    f_ret = feval("pinview_cbirround", f_arg, 1);
    Matrix octnew(f_ret(0).matrix_value());
    if (verbose>1)
      cout << octnew << endl;

    /*
    // We can access the results of the other algorithms.
    cout << hdr << "we have " << GetQuery(qde)->NlowerAlgorithms(qde)
    << " \"lower\" CBIR algorithms ready with their results" << endl;
    for (size_t i=0; i<GetQuery(qde)->NlowerAlgorithms(qde); i++) {
    const Query::algorithm_data& a
    = *GetQuery(qde)->LowerAlgorithm(qde, i);
    cout << " index="    << a.index         << endl;
    cout << " fullname=" << a.fullname      << endl;
    cout << " objects="  << a.result.size() << endl;
    }
    */

    for (size_t i=0; int(i)<octnew.rows(); i++) {
      int idx = (int)octnew(i, 0), idxwas = idx;
      if (lower) {
	idx = qde->indexvec[idx];
	if (debug_indexvec)
	  cout << hdr << "INDEXVEC backward " << idxwas
	       << " -> " << idx << " NEW" << endl;
      }
      list.push_back(Object(database, idx, select_question));
    }

    Picsom()->MutexUnlock();

    if (verbose)
      cout << TimeStamp() << hdr << "returning " << list.size()
	   << " objects" << endl;
      
    return list;
  }

  //===========================================================================

} // namespace picsom

//*****************************************************************************

#else // MKOCTFILE

#include <octave/oct.h>
#include <octave/parse.h>

//-----------------------------------------------------------------------------

DEFUN_DLD(picsom_picsom_c_version, args, ,
          "octave call to PicSOM::PicSOM_C_Version().") {
  return octave_value(PicSOM::PicSOM_C_Version());
}

//-----------------------------------------------------------------------------

DEFUN_DLD(pinview_indexfullname, args, ,
          "octave call to CbirPinView::IndexFullName().") {
  if (args.length()!=2)
    return octave_value("");

  CbirPinView::QueryData *qd = (CbirPinView::QueryData*)args(0).long_value();
  int i = args(1).int_value();
  return octave_value(qd->IndexIndexOK(i)?qd->IndexFullName(i):"");
}

//-----------------------------------------------------------------------------

DEFUN_DLD(pinview_indexfullnamelist, args, ,
          "octave call to CbirPinView::IndexFullNameList().") {
  static string_vector empty;

  if (args.length()!=1)
    return octave_value(empty);

  CbirPinView::QueryData *qd = (CbirPinView::QueryData*)args(0).long_value();
  list<string> l = qd->IndexFullNameList();
  string_vector ret(l.size());
  int j = 0;
  for (list<string>::const_iterator i=l.begin(); i!=l.end(); i++)
    ret(j++) = *i;

  return octave_value(ret);
}

//-----------------------------------------------------------------------------

static VectorIndex *VectorIndex_by_name(CbirPinView::QueryData *qd,
                                        const string& name) {
  for (size_t i=0; i<qd->Nindices(); i++)
    if (qd->IndexShortName(i)==name && qd->IsTsSom(i)) {
      Index *idx = &qd->IndexStaticData(i);
      VectorIndex *vidx = dynamic_cast<VectorIndex*>(idx);
      return vidx;
    }
  return NULL;
}

//-----------------------------------------------------------------------------

static TSSOM *TSSOM_by_name(CbirPinView::QueryData *qd,
                            const string& name) {
  VectorIndex *vidx = VectorIndex_by_name(qd, name);
  return dynamic_cast<TSSOM*>(vidx);;
}

//-----------------------------------------------------------------------------

DEFUN_DLD(pinview_featuredata, args, ,
          "octave call to get feature vector data.") {
  bool debug = false;

  static Matrix empty;

  if (args.length()!=2)
    return octave_value(empty);

  CbirPinView::QueryData *qd = (CbirPinView::QueryData*)args(0).long_value();
  string name = args(1).string_value();
  VectorIndex *vidx = VectorIndex_by_name(qd, name);
  if (!vidx)
    return octave_value(empty);

  FloatVectorSet datatmp;
  const FloatVectorSet *data = &datatmp;

  if (qd->indexvec.empty()) {
    vidx->ReadDataFile();
    vidx->SetDataSetNumbers(false);

    data = &vidx->Data();

  } else
    datatmp = vidx->DataByIndices(qd->indexvec);

  int vl = data->VectorLength();

  Matrix mat(data->Nitems(), 1+vl);

  if (debug)
    cout << "pinview_featuredata(" << name << ") "
	 << (qd->indexvec.empty()?"all data ":"subset of data ")
	 << data->Nitems() << " vectors of length " << vl << endl;

  for (int j=0; j<data->Nitems(); j++) {
    mat(j, 0) = qd->indexvec.empty() ? (*data)[j].Number() : j;
    for (int k=0; k<vl; k++)
      mat(j, k+1) = (*data)[j][k];

    if (debug && (j<3||j==data->Nitems()-1)) {
      cout << "pinview_featuredata(" << name << ") j=" << j
	   << " idx=" << mat(j, 0);
      if (!qd->indexvec.empty())
	cout << " trueidx=" << qd->indexvec[j];
      for (int k=0; k<vl && k<10; k++)
	cout << " " << mat(j, k+1);
      cout << endl;
    }
  }

  return octave_value(mat);
}

//-----------------------------------------------------------------------------

DEFUN_DLD(pinview_tssomcoord, args, ,
          "octave call to get TSSOM coordinate data.") {
  static Matrix empty;

  if (args.length()!=2)
    return octave_value(empty);

  CbirPinView::QueryData *qd = (CbirPinView::QueryData*)args(0).long_value();
  string name = args(1).string_value();
  TSSOM *tssom = TSSOM_by_name(qd, name);
  if (!tssom)
    return octave_value(empty);

  tssom->ReadMapFile();
  tssom->ReadDivisionFile();
  size_t lev = tssom->Nlevels();
  if (!lev)
    return octave_value(empty);

  const TreeSOM& ts = tssom->MapEvenAlien(lev-1);
  const IntVector& div = tssom->Division(lev-1, 0);

  Matrix mat(div.Length(), 3);
  for (int j=0; j<div.Length(); j++) {
    mat(j, 0) = j;
    int x = -1, y = -1;
    if (div[j]!=-1) {
      IntPoint p = ts.ToPoint(div[j]);
      x = p.X();
      y = p.Y();
    }
    mat(j, 1) = x;
    mat(j, 2) = y;
  }

  return octave_value(mat);
}

//-----------------------------------------------------------------------------

DEFUN_DLD(pinview_groundtruth, args, ,
          "octave call to get ground truth data.") {
  static ColumnVector empty;

  if (args.length()!=2)
    return octave_value(empty);

  CbirPinView::QueryData *qd = (CbirPinView::QueryData*)args(0).long_value();
  string name = args(1).string_value();
  picsom::DataBase *db = (picsom::DataBase*)qd->GetDataBase();
  const ground_truth& gt = db->GroundTruthExpression(name);

  ColumnVector v(gt.size());
  for (size_t j=0; j<gt.size(); j++)
    v(j+1, gt[j]);

  return octave_value(v);
}

#endif // MKOCTFILE

#endif // PICSOM_HAVE_OCTAVE && PICSOM_USE_OCTAVE

