// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: TreeSOM.C,v 1.34 2011/03/20 06:53:00 jorma Exp $

#include <TreeSOM.h>
#include <XMLutil.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif //HAVE_PWD_H

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif // HAVE_WINSOCK2_H

#include <sys/stat.h>

#include <sstream>

namespace simple {

///////////////////////////////////////////////////////////////////////////////

TreeSOM::TreeSOM(int w, int h, int l, TopologyType t,double a, double r) 
  : SOM(w, h, l, t,a, r){
  up = down = NULL;
  adaptive_level = -1;
  multi_resolution_correction = false;
  show_aqe = false;
  divxml = false;
}

///////////////////////////////////////////////////////////////////////////////

TreeSOM::TreeSOM(const char *name) : SOM(NULL) {
  up = down = NULL;
  adaptive_level = -1;
  multi_resolution_correction = false;
  show_aqe = false;
  if (name)
    Read(name);
}

///////////////////////////////////////////////////////////////////////////////

TreeSOM::~TreeSOM() {
  delete down;
  if (up)
    up->down = NULL;
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::Dump(Simple::DumpMode dt, ostream& os) const {
  if (dt&DumpRecursive)
    SOM::Dump(dt, os);

  os << Bold("TreeSOM ")   << (void*)this
     << " Nlevels()="      << Nlevels()
     << " adaptive_level=" << adaptive_level
     << " up="   << up
     << " down=" << down
     << endl;

  if (down && (dt&DumpRecursive)) {
    down->Dump(dt, os);
  }
}

///////////////////////////////////////////////////////////////////////////////

// int TreeSOM::Write() {
//   if (adapt_1)
//     SOM1().Write(SOM1().Classifier::FileName());

//   if (adapt_2)
//     SOM2().Write(SOM2().Classifier::FileName());

//   return TRUE;
// }

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::ReadCommon(istream *isp, gzFile gz) {
  int nlevels = 0;
  for (int lno = 0;; lno++) {
    char line[1024];
    bool toolong = false;
    bool ok = FloatVectorSource::GetLine(isp, gz, line, sizeof line, toolong);
    if (!ok) {
      if (!lno)
	ShowError("TreeSOM::ReadCommon() file could not be opened");
      else {
	char tmp[100];
	sprintf(tmp, "#%d ", lno);
	ShowError("TreeSOM::ReadCommon() line ", tmp, " file ended too early");
      }
      return false;
    }

    if (toolong) {
      char tmp[100];
      sprintf(tmp, "#%d ", lno);
      ShowError("TreeSOM::Read() line ", tmp, "[", line, "] line too long");
      return false;
    }

    if (*line != '#') {
      ShowError("TreeSOM::Read() header not found");
      return false;
    }

    if (!strncmp(line, TREESOMHEADER, strlen(TREESOMHEADER))) {
      nlevels = atoi(line+strlen(TREESOMHEADER));
      if (nlevels<1) {
	ShowError("TreeSOM::Read() zero or less levels");
	return false;
      }
      SetLevels(nlevels);
      break;
    }
  }    

  for (TreeSOM *ts = this; ts; ts = ts->down) 
    if (!ts->SOM::ReadCommon(isp, gz))
      return false;

  if (Nlevels()!=nlevels) {
    char tmp[1024];
    sprintf(tmp, "in header: %d read: %d", nlevels, Nlevels());
    ShowError("TreeSOM::Read() nlevels ", tmp);
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::ReadOld(istream& is) {
  int nlevels = 0;
  for (int lno = 0;; lno++) {
    char line[1024];
    is.getline(line, sizeof line);

    if (!is || !is.gcount()) {
      if (!lno)
	ShowError("TreeSOM::Read() file could not be opened");
      else {
	char tmp[100];
	sprintf(tmp, "#%d ", lno);
	ShowError("TreeSOM::Read() line ", tmp, " file ended too early");
      }
      return false;
    }

    if (is.gcount()==sizeof line) {
      char tmp[100];
      sprintf(tmp, "#%d ", lno);
      ShowError("TreeSOM::Read() line ", tmp, "[", line, "] line too long");
      return false;
    }

    if (*line != '#') {
      ShowError("TreeSOM::Read() header not found");
      return false;
    }

    if (!strncmp(line, TREESOMHEADER, strlen(TREESOMHEADER))) {
      nlevels = atoi(line+strlen(TREESOMHEADER));
      if (nlevels<1) {
	ShowError("TreeSOM::Read() zero or less levels");
	return false;
      }
      SetLevels(nlevels);
      break;
    }
  }    

  for (TreeSOM *ts = this; ts; ts = ts->down)
    if (!ts->SOM::Read(is))
      return false;

  if (Nlevels()!=nlevels) {
    char tmp[1024];
    sprintf(tmp, "in header: %d read: %d", nlevels, Nlevels());
    ShowError("TreeSOM::Read() nlevels ", tmp);
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::Write(ostream *osp, gzFile gz) {
  char fn[1024];
  if (!FileName())
    strcpy(fn, "NULL");
  else if (*FileName()!='/' && getcwd(fn, sizeof fn)) {
    strcat(fn, "/");
    if (!strncmp(FileName(), "./", 2))
      strcat(fn, FileName()+2);
    else
      strcat(fn, FileName());

  } else
    strcpy(fn, FileName());

  stringstream meandistance, rmsdistance;
  for (int i=0; i<Nlevels(); i++) {
    meandistance << (i?" ":"") << Level(i).mean_distance;
    rmsdistance  << (i?" ":"") << Level(i).rms_distance;
  }
  string mstr = meandistance.str(), rmsstr = rmsdistance.str();

  char name[1024];
  const char *slash = strrchr(fn, '/');
  strcpy(name, slash ? slash+1 : fn);
  char *dot = strlen(name)>4 ? name+strlen(name)-4 : NULL;
  if (dot && !strcmp(dot, ".cod"))
    *dot = 0;

  char firstlines[1000];
  sprintf(firstlines, "#\n%s%d\n#", TREESOMHEADER, Nlevels());
  PutLine(osp, gz, firstlines);
  Flush(osp, gz);

  char id[1024] = "???";
  if (TrainingSet() && TrainingSet()->FileName())
    strcpy(id, TrainingSet()->FileName());

  time_t t = time(NULL);
  char *timestr = asctime(localtime(&t));
  *strchr(timestr, '\n') = 0;

  passwd *pw = getpwuid(getuid());
  const char *user = pw ? pw->pw_name : NULL;

  char hostname[1000];
  gethostname(hostname, sizeof hostname);

  char vl[10], md[10]="0.0";
  sprintf(vl, "%d", VectorLength());

  char mtrc[1000] = "";
  if (Metric())
    strcpy(mtrc, Metric()->String().c_str());
  
  char norm[1000] = "";
  if (Normalization())
    strcpy(norm, Normalization());

  typedef const xmlChar* XMLS;
  
  xmlNsPtr   ns   = NULL;
  xmlDocPtr  doc  = xmlNewDoc((XMLS)"1.0");
  xmlNodePtr root = xmlNewDocNode(doc, ns, (XMLS)"treesom", NULL);
  xmlDocSetRootElement(doc, root);
  xmlNewTextChild(root, ns, (XMLS)"name", (XMLS)name);
  xmlNewTextChild(root, ns, (XMLS)"saved-as", (XMLS)fn);
  xmlNewTextChild(root, ns, (XMLS)"input-data", (XMLS)id);
  xmlNewTextChild(root, ns, (XMLS)"vectorlength", (XMLS)vl);
  xmlNewTextChild(root, ns, (XMLS)"meandistance", (XMLS)md);

  xmlNewTextChild(root, ns, (XMLS)"quanterrorL1", (XMLS)mstr.c_str());
  xmlNewTextChild(root, ns, (XMLS)"quanterrorL2", (XMLS)rmsstr.c_str());

  if (*mtrc)
    xmlNewTextChild(root, ns, (XMLS)"metric", (XMLS)mtrc);
  if (*norm)
    xmlNewTextChild(root, ns,(XMLS)"normalization",(XMLS)norm);
  if (multi_resolution_correction)
    xmlNewTextChild(root, ns, (XMLS)"multirescorrect", (XMLS)"yes");
  xmlNewTextChild(root, ns, (XMLS)"date", (XMLS)timestr);
  xmlNewTextChild(root, ns, (XMLS)"user", (XMLS)user);
  xmlNewTextChild(root, ns, (XMLS)"host", (XMLS)hostname);

  for (map<string,string>::const_iterator mi=description_extra.begin();
       mi!=description_extra.end(); mi++)
    xmlNewTextChild(root, ns, (XMLS)mi->first.c_str(),
		    (XMLS)mi->second.c_str());

  xmlNodePtr node = root;
  if (TrainingSet()) {
    if (TrainingSet()->XMLDescription()) {
      xmlNodePtr croot = xmlDocGetRootElement(TrainingSet()->XMLDescription());
      xmlNodePtr ccopy = xmlCopyNode(croot, 1);
      // for (xmlNodePtr b = ccopy; b ; b = b->next)
      //   if (xmlIsBlankNode(b)) {
      // 	xmlNodePtr n = b->prev;
      // 	xmlDeeletNode(b);
      //   }
      xmlAddChild(root, ccopy);
      node = ccopy;
    }

    char setsize[100];
    sprintf(setsize, "%d", TrainingSet()->Nitems());
    xmlNewTextChild(node, ns, (XMLS)"setsize", (XMLS)setsize);
  }

  if (codefileextravars.size()) {
    for (map<string,string>::const_iterator ev = codefileextravars.begin();
	 ev!=codefileextravars.end(); ev++)
      xmlNewTextChild(node, ns, (XMLS)ev->first.c_str(),
		      (XMLS)ev->second.c_str());
  }

  WriteDescription(NULL, osp, gz, doc);
  xmlFreeDoc(doc);

  PutLine(osp, gz, "#");
  Flush(osp, gz);
  
  int l = 0;
  for (TreeSOM *ts = this; ts; ts = ts->down) {
    ts->SetDescription("");

    char comment[1000];
    sprintf(comment, "#\n# layer %d/%d", l, Nlevels());
    PutLine(osp, gz, comment);
    Flush(osp, gz);

    ts->Sparse(Sparse());
    ts->WriteLabelsOnly(WriteLabelsOnly());
    if (!ts->SOM::Write(osp, gz))
      return false;
    l++;
  }

  return IsGood(osp, gz);
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::WriteOld(ostream& /*os*/) {
  return false;
  /*
  char fn[1024];
  if (!FileName())
    strcpy(fn, "NULL");
  else if (*FileName()!='/') {
    getcwd(fn, sizeof fn);
    strcat(fn, "/");
    if (!strncmp(FileName(), "./", 2))
      strcat(fn, FileName()+2);
    else
      strcat(fn, FileName());

  } else
    strcpy(fn, FileName());

  os << "#" << endl;
  os << TREESOMHEADER << Nlevels() << endl;
  os << "#"  << endl;
  os << "# original ts-som filename=" << fn << endl;
  os << "#"  << endl;
  os << "# input data filename=" << TrainingSet()->FileName() << endl;
  os << "# input data setsize=" << TrainingSet()->Nitems() << endl;
  os << "# input data vectorlength=" << TrainingSet()->VectorLength() << endl;
  os << "#"  << endl;

  const char *d = TrainingSet()->GetDescription();
  while (d && *d) {
    os << "# input data ";
    for (;;) {
      char tmp[1024];
      strncpy(tmp, d, sizeof(tmp)-1);
      tmp[sizeof(tmp)-1] = 0;
      char *nl = strchr(tmp, '\n');
      if (nl)
	*nl = 0;
      os << tmp;
      d += strlen(tmp);
      if (nl) {
	os << endl;
	d++;
	break;
      }
    }
  }

  os << "#"  << endl;
  
  int l = 0;
  for (TreeSOM *ts = this; ts; ts = ts->down) {
    os << "#" << endl
       << "# layer " << l << "/" << Nlevels() << endl;
    if (!ts->SOM::Write(os))
      return false;
    l++;
  }

  return os.good();
  */
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::SetDivDescription(IntVectorSet& div, int l) const {
  stringstream nn;

  for (int i=0; i<Nlevels(); i++)
    nn << (i?"+":"") << Level(i).train_bmu_set.Nitems();

  string msg = "\nthis is treesom division file\n\n";
  msg += DivDescriptionLine(UseTimeStamp(), nn.str(), l)+"\n";
  div.SetDescription(msg.c_str());
}

///////////////////////////////////////////////////////////////////////////////

string TreeSOM::DivDescriptionLine(bool tstamp, const string& nn, int l) {
  stringstream ss;
  ss << (tstamp?TimeStampP():"") << " " << nn << " x " << l;

  // char tmp[1000];
  // sprintf(tmp, "%s %d x %d\n", TimeStampP(), n, l);

  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::DivideXX(const FloatVectorSet& dataset, const char *name,
		     bool zipit) {
  //
  // this routine is not really used any more if ever...
  //
  char tmp[1024];
  if (!name && FileName()) {
    sprintf(tmp, "%s.div", FileName());
    name = tmp;
  }
  if (!name) {
    ShowError("PicSOM::Divide() no file name");
    return false;
  }

  IntVectorSet set;
  vector<double> rmse(Nlevels()), me(Nlevels());
  SetDivDescription(set, dataset.Nitems());

  for (int lev = 0; lev<Nlevels(); lev++) {
    char lab[1024];
    sprintf(lab, "level=%d", lev);

    IntVector idx(dataset.Nitems(), NULL, lab);
    double dsum = 0.0, d2sum = 0.0;
    for (int i=0; i<dataset.Nitems(); i++) {
      float d2 = 0.0;
      idx[i] = LevelBestMatchingUnit(lev, dataset[i], &d2);
      d2sum += d2;
      dsum  += sqrt(d2);
    }
    rmse[lev] = sqrt(d2sum/dataset.Nitems());
    me[lev]   = dsum/dataset.Nitems();

    set.AppendCopy(idx);
  }

  set.Zipped(zipit);

  return set.Write(name);
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::AdaptWriteDivide(bool lastiter) const {
  if (!FileName()) {
    ShowError("TreeSOM::AdaptWriteDivide() no filename");
    return false;
  }

  char name[1024], nameold[1024];
  strcpy(name, FileName());
  size_t l = strlen(name);
  if (l>4 && !strcmp(name+l-4, ".cod"))
    strcpy(name+l-4, ".div");
  else
    strcat(name, ".div");
  sprintf(nameold, "%s.old", name);

  cout << PossiblyTimeStamp()
       << "TreeSOM::AdaptWriteDivide() filename=<" << name << ">"; // << endl;

  IntVectorSet set;
  int lll = Nlevels() ? Level(0).train_bmu_set[0].Length() : 0;
  SetDivDescription(set, lll);

  for (int lev=0; lev<Nlevels(); lev++) {
    cout << "   level=" << lev << " BMUs="
	 << Level(lev).train_bmu_set.Nitems() << " ("
	 << Level(lev).bmu_div_depth << ")";
    for (int b=0; b<Level(lev).train_bmu_set.Nitems(); b++) {
      IntVector idx = Level(lev).train_bmu_set[b];
      char lab[1024];
      sprintf(lab, "level=%d,bmu=%d", lev, b);
      idx.Label(lab);
      set.AppendCopy(idx);
/*      if (b==0) {
        const FloatVector& dist = Level(lev).train_dist;
        if (dist.Length()!=idx.Length())
          return ShowError("dist.Length()!=idx.Length()");
        for (int i=0; i<dist.Length(); i++)
          if (idx[i]!=-1)
            cout << i << " " << sqrt(dist[i]) << endl;
      }*/
    }
  }

  cout << endl;

  if (lastiter && divxml)
    WriteXmlDivisionFile(Zipped(),set);

  bool zipit = Zipped();
  set.Zipped(zipit);
  
  rename(name, nameold);

  return set.Write(name);
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::AdaptWrite() {
  for (TreeSOM *ts = this; ts; ts = ts->down)
    ts->CalculateRawUmatrix();

  char nameold[1024];
  sprintf(nameold, "%s.old", FileName());
  rename(FileName(), nameold);

  if (!Silent())
    cout << PossiblyTimeStamp() << "TreeSOM::AdaptWrite() filename=<"
	 << FileName() << ">" << endl;

  timespec_t starttime, endtime;
  SetTimeNow(starttime);
  bool ok = Write(FileName());
  SetTimeNow(endtime);

  struct stat st;
  if (ok && !Silent() && !stat(FileName(), &st)) {
    double s = endtime.tv_sec-starttime.tv_sec+
      (endtime.tv_nsec-starttime.tv_nsec)/(1000*1000*1000.0);
    cout << PossiblyTimeStamp() << "TreeSOM::AdaptWrite() wrote " << st.st_size
	 << " bytes in " << s << " seconds (" << st.st_size/(1024*1024*s)
	 << " Mbytes/s)" << endl;
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::Voting(VotingMethod m, int level) {
  int slevel = level>=0 ? level : 0;
  int elevel = level>=0 ? level : Nlevels()-1;

  for (int l=slevel; l<=elevel; l++) {
    Level(l).SOM::Voting(m);
    //cout << "TreeSOM::Voting() " << (void*)&Level(l) << " "
    // << int(Level(l).SOM::Voting()) << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::Silent(int m, int level) {
  int slevel = level>=0 ? level : 0;
  int elevel = level>=0 ? level : Nlevels()-1;

  for (int l=slevel; l<=elevel; l++) {
    Level(l).SOM::Silent(m);
    //cout << "TreeSOM::Voting() " << (void*)&Level(l) << " "
    // << int(Level(l).SOM::Voting()) << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::Train(int level_x, bool do_vote, bool do_div) {
  IntVector nitervec(traincount);
  int nitervalues = Nlevels()+multi_resolution_correction;
  if (nitervec.Length()!=nitervalues) {
    char tmp[1024];
    sprintf(tmp, "(%d!=%d)", nitervec.Length(), nitervalues);
    ShowError("TreeSOM::Train() nitervec.Length()!=nitervalues ", tmp);
  }
  if (multi_resolution_correction && nitervec.Length()<2)
    return false;

  while (nitervec.Length()<Nlevels())
    nitervec.Push(nitervec.Peek());

  IntVector btperiod(batchtrainperiod);
  if (batchtrain && btperiod.Length()) {			// batchtrain
    if (btperiod.Length()!=Nlevels()) {				// btperiod
      char tmp[1024];
      sprintf(tmp, "(%d!=%d)", btperiod.Length(), Nlevels());
      ShowError("TreeSOM::Train() btperiod.Length()!=Nlevels() ", tmp);
    }								// end btperiod
    while (btperiod.Length()<Nlevels())
      btperiod.Push(btperiod.Peek());
  } else							// end batchtrain else
    btperiod.Length(Nlevels());

  int slevel = level_x>=0 ? level_x : 0;
  int elevel = level_x>=0 ? level_x : Nlevels()-1;

  int nvectors = 0;
  if (Level(0).TrainingSet())
    nvectors = CountNotSkipped(*Level(0).TrainingSet());

  // nvectors = Level(0).TrainingSet()->Nitems();
  
  if (!nvectors) {						// nvectors
    ShowError("TreeSOM::Train(): nvectors==0");
    nvectors = 10000;
  } else if (!Silent())						// end nvectors
    cout << PossiblyTimeStamp()
	 << "TreeSOM::Train(): nvectors=" << nvectors << " out of "
	 << Level(0).TrainingSet()->Nitems() << endl;

  bool new_mode = !false, debug = false, ok = true;

  int mrescorr = multi_resolution_correction ? nitervec[0] : 1;

  for (int mloop=0; mloop<mrescorr; mloop++) {		// mloop
    for (int l=slevel; ok && l<=elevel; l++) {		// l
      TreeSOM& ts = Level(l);
      double& mdist   = ts.mean_distance;
      double& rmsdist = ts.rms_distance;

      int niter = nitervec[l+multi_resolution_correction];

      if (new_mode && l && niter && !mloop)
	ts.InitializeFromAbove();

      ts.BatchTrainSettings(batchtrain, btperiod[l], batchtrain_threads);
      BatchTrainSettings(batchtrain, btperiod[l], batchtrain_threads);

      adaptive_level = l;

      int prog_interval = debug ? 1 : 1000;
      ts.ProgressInterval(prog_interval);
      if (debug)
	ts.Silent(0);

      for (int i=0; ok && i<niter; i++) {		// i
	if (new_mode) {					// new_mode
	  double r0 = 12, rlim = ts.Width()/2;
	  if (r0>rlim)
	    r0 = rlim;
	  ts.NewRadiusAndAlpha(i+mloop*niter, mrescorr*niter, nvectors, r0, 0.2);
	} else						// end new_mode else
	  ts.OldRadiusAndAlpha(i+mloop*niter, mrescorr*niter);

	if (Silent()<2) {				// Silent
	  cout << PossiblyTimeStamp()
	       << "TreeSOM::Train starting mloop " << mloop << "/" << mrescorr
	       << " iter " << i << "/" << niter
	       << " on level " << l << "/" << Nlevels()
	       << " (" << ts.Width() << "x" << ts.Height() << ") with "
	       << nvectors << " vectors" << " radius=" << ts.Radius()
               << " radius_base=" << ts.RadiusBase()
               << " neighbourhood_kernel=" << int(ts.neighbourhood_kernel);
	  if (!batchtrain)
	    cout << " alpha=" << ts.Alpha();
	  if (Topology()==Torus)
	    cout << " torus";
	  if (batchtrain)
	    cout << " BATCHTRAIN";
	  cout << "                                                      "
	       << endl;
          cout << "radius.Dump(): ";
          ts.radius.Dump();
	}						// end Silent

	char ptxt[1024];
	sprintf(ptxt, "TreeSOM::Train mloop=%d/%d level=%d/%d iter=%d/%d ",
		mloop, mrescorr, l, Nlevels(), i, niter);
	ProgressText(ptxt);
	
	bool  vot = do_div; // i==niter-1;

	bool do_wdiv = do_div && (i==niter-1);
	bool do_wcod = vot    && (i==niter-1);

	ok = AdaptNew(true, do_div, do_wdiv, vot, do_wcod, mdist, rmsdist, 
	              i==niter-1 && !MultiResolutionCorrection());

	TicTacSummary();
      }						        // end i
      
      if (niter==0) 
	ok = AdaptNew(false, do_div, do_div, do_vote, do_vote, mdist, rmsdist, 
	              true);
    }						        // end l

    if (MultiResolutionCorrection()) {			// MRC
      CorrectFromLevelBelow();
      if (mloop==mrescorr-1) 
        for (int l=slevel; ok && l<=elevel; l++) {
          adaptive_level = l; 
	  TreeSOM& ts = Level(l);
	  double& mdist   = ts.mean_distance;
	  double& rmsdist = ts.rms_distance;
          ok = AdaptNew(false, do_div, do_div, do_div, true, mdist, rmsdist, 
                        true);
        }
    }							// end MRC

    if (ShowAQE() && (mloop==mrescorr-1))
      for (int l=slevel; ok && l<=elevel; l++) {	// l
        TreeSOM& ts = Level(l);
        ts.AverageQuantisationError(true);
      }							// end l
  }							// end mloop

  adaptive_level = -1;

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::OldRadiusAndAlpha(int i, int niter) {
  Radius(0.5*Width()*(niter-i)/niter);
  Alpha(0.2*(niter-i)/niter);
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::NewRadiusAndAlpha(int i, int niter, int nvec,
				double r, double a) {
  Radius(r-(r-1)*i/niter).SetFractionLife(1/3.0, nvec).SetBase(RadiusBase());
  Alpha( a*(niter-i)/niter).SetFractionLife(1/3.0, nvec);

  if (DebugAdapt())
    cout << "TreeSOM::NewRadiusAndAlpha() : i=" << i << " niter=" << niter
	 << " nvec=" << nvec << " r=" << r << " a=" << a << endl
	 << " radius=" << radius << endl
	 << " alpha =" << alpha  << endl;
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::ReLabel(int level) {
  int slevel = level>=0 ? level : 0;
  int elevel = level>=0 ? level : Nlevels()-1;

  for (int l=slevel; l<=elevel; l++)
    DoVoteLabels(l, *TrainingSet());
}

///////////////////////////////////////////////////////////////////////////////

int TreeSOM::LevelBestMatchingUnit(int lev, const FloatVector& vec,
				   float *d, IntVector *bmuvec) const {
  // cout << " TreeSOM::LevelBestMatchingUnit(lev=" << lev << ")" << endl;
  /*
  int unit = -1;
  for (int i=0; i<=lev; i++)
    unit = Level(i).RestrictedSearch(unit, vec, d);
  */

  int unit2 = -1;
  if (lev==0)
    unit2 = Level(0).RestrictedSearch(unit2, vec, d, NULL, bmuvec);
  else
    unit2 = Level(lev).RestrictedSearch(Level(lev-1).
					train_bmu_set[0][vec.Number()],
					vec, d, NULL, bmuvec);

  return unit2;

  /*
  if (unit!=unit2)
    cout << "XXX unit=" << unit << " unit2=" << unit2 << endl;

  return unit;
  */
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::AdaptVector(const FloatVector& vec) {
  Tic("TreeSOM::AdaptVector()");
  int unit = LevelBestMatchingUnit(adaptive_level, vec);
  //cout << "TreeSOM::AdaptVector() unit="<<unit<<endl;
  if (unit==-1)
    return false;

  TreeSOM& ts = Level(adaptive_level);
  ts.Learn(vec, unit);
  Tac("TreeSOM::AdaptVector()");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::AdaptEpochDone(FloatVectorSource& /*set*/) {
  ShowError("TreeSOM::AdaptEpochDone() should be obsolete");
//   if (do_voting)
//     DoVoteLabels(adaptive_level, set);
}

///////////////////////////////////////////////////////////////////////////////

void TreeSOM::DoVoteLabels(int level, FloatVectorSource& set) {
  Tic("TreeSOM::DoVoteLabels()");
  Level(level).EmptyVotes();
  set.First();
  for (const FloatVector *vec; (vec = set.Next());) {
    float dist = 0;
    int unit = LevelBestMatchingUnit(level, *vec, &dist);
    Level(level).AddEitherVote(unit, vec->Label(), dist);
    
    ShowProgress(*vec, "TreeSOM::DoVoteLabels() ");
  }
  Level(level).ConditionallyMajorityVoteLabels();
  if (Silent()<2) {
    if (Level(level).Width()<=20)
      Level(level).Display();
    Level(level).Dump(DumpRecursiveShort);
  }
  Tac("TreeSOM::DoVoteLabels()");
}

///////////////////////////////////////////////////////////////////////////////

int TreeSOM::RestrictedSearchOld(int u, const FloatVector& vec,
				 float *dd) const {
  //Tic("TreeSOM::RestrictedSearch()");

  int xs, xe, ys, ye;
  char pointstr[1000] = "";

  if (u==-1) {
    xs = ys = 0;
    xe = Width()-1;
    ye = Height()-1;

  } else {

    if (!up)
      SimpleAbort();
    
    IntPoint mp = up->ToPoint(u);
    sprintf(pointstr, "%d=(%d,%d)->", u, mp.X(), mp.Y());

    float xmul = float(Width())/up->Width();
    float ymul = float(Height())/up->Height();
    
    FloatPoint fp(xmul*mp.X()+(xmul-1)/2, ymul*mp.Y()+(ymul-1)/2);

    sprintf(pointstr+strlen(pointstr), "[%fx%f]->(%f,%f) ", xmul, ymul,
	    fp.X(), fp.Y());

    xs = (int)floor(fp.X()-xmul);
    ys = (int)floor(fp.Y()-ymul);
    xe = (int)ceil( fp.X()+xmul);
    ye = (int)ceil( fp.Y()+ymul);
    
    xs = xs<0 ? 0 : xs>Width()-1  ? Width()-1  : xs;
    xe = xe<0 ? 0 : xe>Width()-1  ? Width()-1  : xe;
    ys = ys<0 ? 0 : ys>Height()-1 ? Height()-1 : ys;
    ye = ye<0 ? 0 : ye>Height()-1 ? Height()-1 : ye;
  }

//   cout << "TreeSOM::RestrictedSearch() "
//        << pointstr
//        << (xe-xs+1) << "x" << (ye-ys+1) << endl;

  int dci = DistanceComparisonInterval();
  float mind = MAXFLOAT;
  int mini = -1;
  for (int x=xs; x<=xe; x++)
    for (int y=ys; y<=ye; y++) {
      // Tic("TreeSOM::RestrictedSearch(): DistanceSquared()");
      float ref = dci ? mind : MAXFLOAT; // mind;

      float d = Unit(x, y)->DistanceSquaredXX(vec, Metric(), ref, dci);

      if (ref==MAXFLOAT && (d==MAXFLOAT || isnan(d))) {
	ShowError("TreeSOM::RestrictedSearch() failed in DistanceSquaredXX():",
		  isnan(d) ? " NaN" : " MAXFLOAT");
	return -1;
      }

      // Tac("TreeSOM::RestrictedSearch(): DistanceSquared()");
      if (d<mind) {
	mind = d;
	mini = ToIndex(x, y);
      }
    }

  if (dd)
    *dd = mind;

  //Tac("TreeSOM::RestrictedSearch()");

  return mini;
}

///////////////////////////////////////////////////////////////////////////////

int TreeSOM::RestrictedSearch(int u, const FloatVector& vec, float *dd,
			      gzFile gz, IntVector *bmuvec) const {
  //Tic("TreeSOM::RestrictedSearch()");

  int xs, xe, ys, ye;
  char pointstr[1000] = "";

  if (u==-1) {
    xs = ys = 0;
    xe = Width()-1;
    ye = Height()-1;

  } else {

    if (!up)
      SimpleAbort();
    
    IntPoint mp = up->ToPoint(u);
    sprintf(pointstr, "%d=(%d,%d)->", u, mp.X(), mp.Y());

    float xmul = float(Width())/up->Width();
    float ymul = float(Height())/up->Height();
    
    FloatPoint fp(xmul*mp.X()+(xmul-1)/2, ymul*mp.Y()+(ymul-1)/2);

    sprintf(pointstr+strlen(pointstr), "[%fx%f]->(%f,%f) ", xmul, ymul,
	    fp.X(), fp.Y());

    xs = (int)floor(fp.X()-xmul);
    ys = (int)floor(fp.Y()-ymul);
    xe = (int)ceil( fp.X()+xmul);
    ye = (int)ceil( fp.Y()+ymul);
    
    switch(Topology()){
    case Rectangular:
      xs = xs<0 ? 0 : xs>Width()-1  ? Width()-1  : xs;
      xe = xe<0 ? 0 : xe>Width()-1  ? Width()-1  : xe;
      ys = ys<0 ? 0 : ys>Height()-1 ? Height()-1 : ys;
      ye = ye<0 ? 0 : ye>Height()-1 ? Height()-1 : ye;
      break;
    case Torus:
      break;
    default:
      ShowError("TreeSOM::RestrictedSearch(): unknown topology.");
      SimpleAbort();
    }
  }

//   cout << "TreeSOM::RestrictedSearch() "
//        << pointstr
//        << (xe-xs+1) << "x" << (ye-ys+1) << endl;

  int dci = DistanceComparisonInterval();

  int vlen = bmuvec && bmuvec->Length() ? bmuvec->Length() : 1;
  FloatVector mindv(vlen);
  mindv.Set(MAXFLOAT);
  IntVector miniv(vlen);
  miniv.Set(-1);

  for (int y=ys; y<=ye; y++) {
    int yn=y;

    if (Topology()==Torus) {
      if (yn<0)
	yn += Height();
      else if (yn>=Height())
	yn -= Height();
    }

    for (int x=xs; x<=xe; x++) {
      int xn = x;

      if (Topology()==Torus) {
	if (xn<0)
	  xn += Width();
	else if(xn>=Width())
	  xn -= Width();
      }

      // Tic("TreeSOM::RestrictedSearch(): DistanceSquared()");
      float ref = dci ? mindv.Peek() : MAXFLOAT;
      
      float d = SolveDistanceSquared(xn, yn, vec, Metric(), ref, dci, gz);
      
      if (ref==MAXFLOAT && (d==MAXFLOAT || isnan(d))) {
	ShowError("TreeSOM::RestrictedSearch() failed in DistanceSquaredXX():",
		  isnan(d) ? " NaN" : " MAXFLOAT");
	return -1;
      }

      // Tac("TreeSOM::RestrictedSearch(): DistanceSquared()");
      if (d<mindv.Peek()) {
	int idx = -1;
	mindv.InsertSortFixedIncreasingly(d, &idx);
	miniv.Insert(idx, ToIndex(xn, yn), miniv.Length()-1);
      }
    }
  }

  if (dd)
    *dd = mindv[0];

  if (bmuvec)
    *bmuvec = miniv;

  //Tac("TreeSOM::RestrictedSearch()");

  return miniv[0];
}

///////////////////////////////////////////////////////////////////////////////

double TreeSOM::SolveDistanceSquared(int x, int y, const FloatVector& vec,
				     const FloatVector::DistanceType *metr,
				     double ref, int dci, gzFile gz) const {
  if (Unit(x, y)->HasVector())
    return Unit(x, y)->DistanceSquaredXX(vec, metr, ref, dci);

  int idx = ToIndex(x, y);
  
  FloatVector *v = SeekAndReadOne(idx, NULL, gz);
  if (!v || !v->HasVector()) {
    ShowError("TreeSOM::SolveDistanceSquared() : SeekAndReadOne() failed.");
    return MAXFLOAT;
  }

  double d = v->DistanceSquaredXX(vec, metr, ref, dci);

  delete v;

  return d;
}

///////////////////////////////////////////////////////////////////////////////

IntVector TreeSOM::FindBestMatches(const FloatVector& v, bool use_depth) {
  if (!Units()) {
    ShowError("TreeSOM::FindBestMatches() no units in the SOM");
    return IntVector();
  }

  gzFile gz = NULL;
  if (!Unit(0)->HasVector()) {
    if (!RecordSeekIndex()) {
      ShowError("TreeSOM::FindBestMatches() no record_seek_index");
      return IntVector();
    }
    // cout << "re-opening <" << FileName() << ">" << endl;
    gz = gzopen(FileName(), "r");
    if (!gz) {
      ShowError("TreeSOM::FindBestMatches() failed in gzopen()");
      return IntVector();
    }
    int len = ReadHead(NULL, gz);
    if (!len) {
      gzclose(gz);
      ShowError("TreeSOM::FindBestMatches() len==0");
      return IntVector();
    }
    if (len!=VectorLength()) {
      gzclose(gz);
      char tmp[100];
      sprintf(tmp, "%d!=%d", len, VectorLength());
      ShowError("TreeSOM::FindBestMatches() len!=vectorlength : ", tmp);
      return IntVector();
    }
  }


  IntVector bmu;
  for (int i=0; i<Nlevels(); i++) {
    TreeSOM& ts = Level(i);
    IntVector bmuvec(use_depth ? ts.bmu_div_depth : 1);
    ts.RestrictedSearch(i?bmu[i-1]:-1, v, NULL, gz, &bmuvec);
    bmu.Append(bmuvec);
  }

  gzclose(gz);

  return bmu;
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::InitializeFromAbove() {
  if (!up || up->Width()<2 || up->Height()<2)
    return false;

  Tic("TreeSOM::InitializeFromAbove()");

  cout << PossiblyTimeStamp() << "TreeSOM::InitializeFromAbove()" << endl;

  int uw = up->Width(), uh = up->Height();

  double mx = float( Width())/uw;
  double my = float(Height())/uh;

  double sx = (mx-1)/2;
  double sy = (my-1)/2;

  for (int x=0; x<Width(); x++)
    for (int y=0; y<Height(); y++) {
      double ux = (x-sx)/mx;
      double uy = (y-sy)/my;

      int x0 = (int)floor(ux), y0 = (int)floor(uy);

      if (Topology()!=Torus) {
	if (x0<0)
	  x0 = 0;
	if (y0<0)
	  y0 = 0;
	
	if (x0>=uw-1)
	  x0 = uw-2;
	if (y0>=uh-1)
	  y0 = uh-2;
      }

      double px = ux-x0, py = uy-y0, rx = 1-px, ry = 1-py;
      double c0 = rx*ry, cx = px*ry, cy = py*rx, c1 = px*py;

      /*
      cout << "x=" << x << " y=" << y << "  ux=" << ux << " uy=" << uy
	   << "  x0=" << x0 << " y0=" << y0
	   << "  c0=" << c0 << " cx=" << cx << " cy=" << cy << " c1=" << c1
	   << endl;
      */
      switch(Topology()){
      case Rectangular:
	{
	  const FloatVector& v00 = *up->Unit(x0,   y0  );
	  const FloatVector& v01 = *up->Unit(x0,   y0+1);
	  const FloatVector& v10 = *up->Unit(x0+1, y0  );
	  const FloatVector& v11 = *up->Unit(x0+1, y0+1);
	  
	  Unit(x, y)->CopyValue(v00*c0+v01*cy+v10*cx+v11*c1);
	  break;
	}
      case Torus:
	{
	  const FloatVector& v00 = *up->Unit((x0+uw)%uw,   (y0+uh)%uh  );
	  const FloatVector& v01 = *up->Unit((x0+uw)%uw,   (y0+1+uh)%uh);
	  const FloatVector& v10 = *up->Unit((x0+1+uw)%uw, (y0+uh)%uh  );
	  const FloatVector& v11 = *up->Unit((x0+1+uw)%uw, (y0+1+uh)%uh);
	  
	  Unit(x, y)->CopyValue(v00*c0+v01*cy+v10*cx+v11*c1);
	  break;
	}
      default:
	ShowError("TreeSOM::InitializeFromAbove() : unknown topology");
	SimpleAbort();
      }
    }

  Tac("TreeSOM::InitializeFromAbove()");

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// By Philip Prentis 

bool TreeSOM::CorrectFromLevelBelow() {
  if (!down) 
    return false;

  Tic("TreeSOM::CorrectFromLevelBelow()");

  cout << PossiblyTimeStamp() << "TreeSOM::CorrectFromLevelBelow() " << endl;
  
  down->CorrectFromLevelBelow();
  int dw = down->Width(), dh = down->Height();

  int mx = dw/Width();  
  int my = dh/Height();

  for (int x=0; x<Width(); x++)
    for (int y=0; y<Height(); y++) {
      const FloatVector& temp = *Unit(x, y);
      Unit(x, y)->CopyValue(temp*double(0));
      for (int xd=mx*x; xd<(mx*x)+mx; xd++) 
        for (int yd=my*y; yd<(my*y)+my; yd++) {
          Unit(x, y)->CopyValue(*Unit(x,y)+(*down->Unit(xd, yd)/double(mx*my)));
        }
    }

  Tac("TreeSOM::CorrectFromLevelBelow()");

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// By Philip Prentis

double TreeSOM::AverageQuantisationError(bool recalculate) {
  cout << PossiblyTimeStamp()
       << "TreeSOM::AverageQuantisationError(" << recalculate << "), ";
  cout << "Level: " << Level() << ", ";
  float tsqper = 0.0;
  if (recalculate || (average_quantisation_error < 0)) {
    average_quantisation_error = 0;
    FloatVectorSource& set = *TrainingSet();
    set.First();
    int count = 0;
    int tsq = 0;
    int lev = Level();
    for (const FloatVector *vec; (vec = set.Next());) {
      IntVector nn(1);
      FloatVector ddd;
      NearestNeighbors(*vec, nn, ddd);
      //average_quantisation_error += double(((float*)ddd)[0]);
      
      float dist;
      //IntVector *bmuvec = NULL;
      //FindBestMatch(*vec, &dist, bmuvec);
      if (TopLevel().LevelBestMatchingUnit(lev, *vec, &dist) != nn[0])
        tsq += 1;
      average_quantisation_error += dist;
      count++;
    }
    average_quantisation_error = count ? average_quantisation_error/float(count): -1;
    tsqper = 1-(tsq/float(count));
  }
  cout << "average_quantisation_error == " << average_quantisation_error 
       << " tsq = " << tsqper << endl;
  return average_quantisation_error;
}

///////////////////////////////////////////////////////////////////////////////

const char *TreeSOM::StructureM() const {
  const char *ds = down ? down->StructureM() : NULL;
  
  char s[1000] = "";
  if (!up && Topology()==Torus)
    strcpy(s, "T");
  sprintf(s+strlen(s), "%dx%d%s%s", Width(), Height(), ds?",":"", ds?ds:"");
  
  delete [] ds;

  return CopyString(s);
}

///////////////////////////////////////////////////////////////////////////////

bool TreeSOM::WriteXmlDivisionFile(bool zipped, const IntVectorSet& set) const {
  bool ok = true;
  
  picsom::XmlDom doc = picsom::XmlDom::Doc();
  picsom::XmlDom root = doc.Root("objectindex",
                                 "http://www.cis.hut.fi/picsom/ns",
                                 "picsom");
  string name = FileName();
  size_t slashpos = name.find_last_of('/');
  size_t dotpos = name.find_last_of('.');
  name = name.substr(slashpos+1,dotpos-slashpos-1);
  
  root.Prop("type","tssom");
  root.Prop("name",name);
  root.Prop("layers",Nlevels());
  
  stringstream sstr;
  for (int l=0; l<(int)Nlevels(); l++) 
    sstr << (l?"+":"") << Level(l).BmuDivDepth();
  root.Prop("depths",sstr.str());

  int old_i=-1;
  int old_l=-1;
  picsom::XmlDom oi, tl, bl;
  
  int datasetsize = set[0].Length();
  //int datasetsize = Nlevels() ? Level(0).train_bmu_set[0].Length() : 0;
  
  for (int i=0; ok && i<(int)datasetsize; i++) {
    int ii=0;
    for (int l=0; ok && l<(int)Nlevels(); l++) {
      for (int d=0; ok && d<Level(l).BmuDivDepth(); d++) {
        
        if (ii>=set.nitems) continue;
        // instead like this: IntVector idx = Level(lev).train_bmu_set[d]; ??
        const IntVector& div = set[ii++];
        
        if (div[i] == -1) continue;
/*        
        cout << "i=" << i << "/" << datasetsize 
             << " l=" << l << "/" << nlevels
             << " d=" << d << "/" << Level(l).BmuDivDepth()
             << " ii=" << ii
             << " div[i]=" << div[i]
             << endl;*/
             
        if (i!=old_i) {
          oi = root.Element("objectinfo");
          oi.Prop("index",i);
          tl = oi.Element("tssomlocation");
          old_i=i;
        }    
        if (l!=old_l) {
          bl = tl.Element("bmulist");
          bl.Prop("layer",l);
          old_l=l;
        }

        picsom::XmlDom bmu = bl.Element("bmu");
        bmu.Prop("order",d);
        bmu.Prop("index",div[i]);
//      bmu.Prop("weight",1.0);  // should be 1.0 as default in XML schema (?)

        if (d==0) {
          const FloatVector& dist = Level(l).train_dist;
          if (dist.Length()) { // if we have distances, i.e. during training
            if (dist.Length()!=div.Length())
              return ShowError("dist.Length()!=div.Length()");
            bmu.Prop("sqrdist",dist[i]);
          }
        }
      }
    }
  }
  string xmlname = name+"-div.xml"; 
  doc.Write(xmlname, true, zipped);
  doc.DeleteDoc();
  return ok;
}


///////////////////////////////////////////////////////////////////////////////

} // namespace simple


