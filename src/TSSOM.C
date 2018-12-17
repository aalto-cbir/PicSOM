// -*- C++ -*-  $Id: TSSOM.C,v 2.207 2018/01/02 10:30:45 jormal Exp $
// 
// Copyright 1998-2018 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <TSSOM.h>

#include <DataBase.h>
#include <Query.h>
#include <Connection.h>
#include <WordHist.h>

#include <gzstream.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string TSSOM_C_vcid =
    "@(#)$Id: TSSOM.C,v 2.207 2018/01/02 10:30:45 jormal Exp $";

  static TSSOM list_entry(true);

  bool TSSOM::backrefsanitychecks = false;

  TSSOM::TSSOM(DataBase *ddd, const string& nnn, const string& ppp, 
               const string& params, const Index* src) :
    VectorIndex(ddd, nnn, nnn, ppp, params, src) {

    type                     = tssom_undef;
    division_changed         = false;

    show_map_values = show_data_values = false;

    meandistance = -1.0;

    if (Name()=="")  // e.g. called from Analysis::Analysis().
      return;

    //metfile_str = FindFile(Name(), ".met");
    codfile_str = FindFile(Name(), ".cod");
    cnvfile_str = FindFile(Name(), ".cnv");
    ordfile_str = FindFile(Name(), ".ord");
    nbrfile_str = FindFile(Name(), ".nbr");

    divfile = FindFile(Name(), ".div");
    if (divfile.Name()=="")
      divfile = FindFile(Name(),"-div.xml");

    if (cnvfile_str=="")
      cnvfile_str = FindFile("convolutions");

    if (db->SqlAll()) {
      string f = "features/";
      if (db->SqlFileExists(f+Name()+".cod"))
	codfile_str = db->TempFile(f+Name()+".cod", true);
      if (db->SqlFileExists(f+Name()+".div"))
	divfile     = db->TempFile(f+Name()+".div", true);
    }

    string adbname = AlienDataBaseName();

    if (adbname!="") {
      Picsom()->AddAllowedDataBase(adbname);

      if (Picsom()->FindDataBase(adbname)) {
        type = TypeFromName();
      
        if (type==tssom_undef)
          ShowError("TSSOM::TSSOM() : database name=<", Name(),
                    "> strange adbname=<", adbname, ">");
      
      } else
        ShowError("TSSOM::TSSOM() : database name=<", Name(),
                  "> adbname=<", adbname, "> not found");
    
    } else
      type = tssom_normal;

    if (feature_target==target_no_target)
      GuessFeatureTarget();

    SetFeatureNames();
  }

  /////////////////////////////////////////////////////////////////////////////

  TSSOM::~TSSOM() {
    if (!DestroyClassifiers())
      ShowError("TSSOM::DestroyClassifiers() failed in "+MapName(true));
  }

  /////////////////////////////////////////////////////////////////////////////
  /*
  void TSSOM::Dump(Simple::DumpMode, ostream& os) const { 
    os << Bold("TSSOM ") << (void*)this
       << " name="                    << ShowString(Name())
       << " longname="                << ShowString(longname_str)
       << " shorttext="               << ShowString(shorttext_str)
       << " path="                    << ShowString(path_str)
       << " type="                    << TypeName(type) << " ("<<int(type)<<")"
       << " datfile[0]="              << ShowString(datfilelist.front())
       << " metfile="                 << ShowString(metfile_str)
       << " codfile="                 << ShowString(codfile_str)
       << " divfile="                 << ShowString((string)divfile)
       << " cnvfile="                 << ShowString(cnvfile_str)
       << " ordfile="                 << ShowString(ordfile_str)
       << " nbrfile="                 << ShowString(nbrfile_str)
       << " convolutions="            << ShowString(convolutions_str)
       << " data.Nitems()="           << data.Nitems()
       << " treesom.Nlevels()="       << treesom.Nlevels()
       << " division_xx.Nitems()="    << division_xx.Nitems()
       << " meandistance="            << meandistance
       << endl;
  }
  */

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::IsRawFeatureName(const string& fn) const {
    return fn=="rgb" || fn=="sift" || fn=="ipld";
  }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::GuessFeatureTarget() {
    if (codfile_str!="") {
      const string tag = "<featuretarget>";
//       cout << "opening cod: " << codfile_str << endl;
      igzstream cod(codfile_str.c_str());
      //ifstream cod(codfile_str.c_str());
      for (;;) {
        string line;
        getline(cod, line);
        if (!cod || line[0]!='#')
          break;
        size_t p = line.find(tag);
        if (p==string::npos)
          continue;
        line.erase(0, p+tag.size());
        p = line.find("<");
        if (p==string::npos)
          continue;
        line.erase(p);
        target_type t = SolveTargetTypes(line, false);
        if (t!=target_no_target) {
          FeatureTarget(t, true);
          return true;
        }
      }
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::IsMakeable(DataBase *db, const string& fn, const string& dirstr,
                         bool create) const {
    string fname = dirstr+"/"+fn; // , metn = fname+".met";
    
    if (IsAlienFeature(fn) &&
        !db->FileExistsPossiblyZipped(fname+".div") &&
        !db->FileExistsPossiblyZipped(fname+"-div.xml") &&
        !db->FileExistsPossiblyZipped(fname+".cod"))
      return false;
    
    bool ok = db->FileExistsPossiblyZipped(fname+".cod") &&
      (db->FileExistsPossiblyZipped(fname+"-div.xml") || 
       db->FileExistsPossiblyZipped(fname+".div"));
    
    if (!ok && create) {
      if (db->UseBinFeaturesRead())
	ok = db->FileExists(fname+".bin");
      else if (db->SqlFeatures())
	ok = true; // obs!
      else
	ok = (db->FileExistsPossiblyZipped(fname+".dat") ||
	      // db->FileExistsPossiblyZipped(fname+".met") ||
	      db->AnyLeafDirectoryFileExists("features", fn+".dat") ||
	      IsRawFeatureName(fn));
    }

    if (!ok && create)
      ok = true;

    return ok;
  }

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::CheckModifiedFiles() {
  if (division_xx.Nitems() && divfile.Modified()) {
    ReadDivisionFile(true);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReadFiles(bool force, bool nodata, bool dat,
                      bool cod, bool div, bool cnv, bool ord, bool nbr) {
//  if (IsBinaryFeature())
//     return true;

  bool ret = true;

  ret = ret && (!dat || ReadDataFile(force, nodata));
  ret = ret && (!cod || ReadMapFile(force, nodata));
  ret = ret && (!div || ReadDivisionFile(force));
  ret = ret && (!cnv || ReadConvolutionFile(force));
  ret = ret && (!ord || ReadOrdFile(force));
  ret = ret && (!nbr || ReadNbrFile(force));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReadMapFile(bool force, bool nodata) {
  string msg = "TSSOM::ReadMapFile() name="+Name()+" : ";

  if (type!=tssom_normal && type!=tssom_alien_map)
    return true;

  if (codfile_str=="")  // ie. when created with SolveExtractions().
    return true;

  if (db->SqlAll() && !FileExists(codfile_str)) {
    size_t p = codfile_str.find("/features/");
    if (p!=string::npos) {
      string f = codfile_str.substr(p+1);
      db->SqlRetrieveFileToFile(f, codfile_str);
      if (PicSOM::DebugTempFiles())
	cout << msg << "filled in <" << codfile_str << "> with "
	     << FileSize(codfile_str) << " bytes" << endl;
    }
  }

  RwLockRead("ReadMapFile()");
  bool really = ReadMapFileReally(force, nodata);
  RwUnlockRead("ReadMapFile()");

  if (!really)
    return true;

  RwLockWrite("ReadMapFile()");
  if (!ReadMapFileReally(force, nodata)) {
    RwUnlockWrite("ReadMapFile()");
    return true;
  }

  Tic("ReadMapFile");

  bool ok = true;
  treesom.TicTacSet(Picsom()->GetTicTac());
  // WriteLog("TSSOM::ReadMapFile() ", nodata?"nodata ":"", codfile);
  treesom.FileName(codfile_str);
  bool nodatawas = treesom.NoDataRead(nodata);
  treesom.RecordSeekIndex(nodata);

  if (!treesom.Read(codfile_str))
    ok = ShowError(msg+"reading <", codfile_str, "> failed");
  treesom.NoDataRead(nodatawas);
  Tac("ReadMapFile");

  bool has_cmdline = false, angry = db->OughtToCalculateFeatures();
  if (!CanCalculateFeatures())
    has_cmdline = SolveFeatureCalculationCommand(true, angry);

  string tsstruct = treesom.Structure();
  char tmptxt[1000];
  sprintf(tmptxt, " (dim=%d, %d levels: %s%s%s)%s",
          treesom.VectorLength(), (int)Nlevels(), tsstruct.c_str(),
          treesom.Topology()==simple::Planar::Torus ? " torus":"",
          treesom.HasRawUmatrix()?" + U-matrix":"",
          has_cmdline?" w/CMDLINE":"");

  string datatxt;
  if (!HasTreeSOM())
    datatxt = " (no tssom)";
  else if (!HasTreeSOMvectors())
    datatxt = " (no vectors)";
  else if (!HasNonZeroTreeSOMvectors())
    datatxt = " (zero vectors)";
  else if (nodata)
    datatxt = " (labels only)";
    
  WriteLog("Read map file ", ShortFileName(codfile_str), datatxt, tmptxt);
  Picsom()->PossiblyShowDebugInformation("ReadMapFile() ending");

  if (treesom.XMLDescription()) {
    if (meandistance > -0.01)
      WriteLog("Warning: meandistance already set before ReadMapfile() <", 
               Name(), "> meandistance=", ToStr(meandistance));

    xmlNodePtr root = xmlDocGetRootElement(treesom.XMLDescription());
    for (xmlNodePtr node = root->children; ok && node; node = node->next)
      if (node->type==XML_ELEMENT_NODE) {
        if (NodeName(node)=="feature")
          for (xmlNodePtr f = node->children; ok && f; f = f->next) {
            if (f->type!=XML_ELEMENT_NODE)
              continue;

            string key = NodeName(f), val = NodeChildContent(f);
            int r = 0;
            if (!Interpret(key, val, r) || !r) {
              // ShowError(msg+"processing ["+key+"]=["+val+"] failed A");
            }
          }

        else {
          string key = NodeName(node), val = NodeChildContent(node);
          int r = 0;
          if (!Interpret(key, val, r) || !r) {
            // ShowError(msg+"processing ["+key+"]=["+val+"] failed B");
          }
        }
      }
  }

  if (ok && feature_target==target_no_target &&
      db->DefaultFeatureTarget()!=target_no_target)
    FeatureTarget(db->DefaultFeatureTarget(), true);

  ok = ok && FindLabelIndices(force);

  RwUnlockWrite("ReadMapFile()");

  return ok;
}
  
///////////////////////////////////////////////////////////////////////////////

bool TSSOM::SetMeanDist(const string& valstr) {
  const char *value = valstr.c_str();

  double epsilon = 0.00000001;
  if (meandistance>epsilon) {
    double md2 = atof(value);
    if (fabs(md2-meandistance)>epsilon)
      ShowError("SetMeanDist() <", Name(), "> meandistance defined ", 
                "twice with different values");    
  }
  meandistance = atof(value);
  WriteLog("SetMeanDist() <", Name(), "> meandistance=",
           ToStr(meandistance));  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::SetBackGround(const string& fnames) {
  string msg = "SetBackGround("+fnames+") : ";

  vector<string> l = SplitInCommas(fnames);

  bool ok = true;
  for (vector<string>::const_iterator i=l.begin(); i!=l.end(); i++) {
    string fname = *i;
    if (!FileExists(fname))
      fname = FindFile(fname);
    if (fname=="") {
      ok = ShowError(msg+"file <"+*i+"> not found");
      continue;
    }

    try {
      imagedata imgd = imagefile(fname, imagedata::pixeldata_uchar).frame(0);
      WriteLog("Background image <"+*i+"> "+imgd.info()+" successfully read");
      background[*i] = imgd;
    }
    catch (const string& err) {
      ok = ShowError(msg+"failed with <"+*i+"> : "+err);
    }
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReadDivisionFile(bool force) {
  RwLockRead("ReadDivisionFile()");
  bool really = ReadDivisionFileReally(force);
  RwUnlockRead("ReadDivisionFile()");
  
  if (!really)
    return true;

  if (divfile.Name()=="") // ie. when created with SolveExtractions().
    return true;

  string hdr = "TSSOM::ReadDivisionFile() : ";

  if (db->SqlAll() && !FileExists(divfile.Name())) {
    string divfile_str = divfile.Name();
    size_t p = divfile_str.find("/features/");
    if (p!=string::npos) {
      string f = divfile_str.substr(p+1);
      db->SqlRetrieveFileToFile(f, divfile_str);
      if (PicSOM::DebugTempFiles())
	cout << hdr << "filled in <" << divfile_str << "> with "
	     << FileSize(divfile_str) << " bytes" << endl;
    }
  }

  RwLockWrite("ReadDivisionFile()");
  if (!ReadDivisionFileReally(force)) {
    RwUnlockWrite("ReadDivisionFile()");
    return true;
  }

  if (division_changed)
    ShowError(hdr+"division_changed==true");

  bool ok = true;

  const string& divname = divfile.Name();
  if (divname.substr(divname.length()-8,8) == "-div.xml")
    ReadDivisionFileXml(divname);
  else {
    Tic("ReadDivisionFile");

    division_xx.FileName(divfile);
    if (!division_xx.Read(divfile)) {
      ShowError(hdr+"reading ", divfile, " failed");
      ok = false;
    }
  }
  divfile.WasRead();
  division_changed = false;

  int setsize = DataSetSizeEvenAlien(), divlenwas = division_xx.VectorLength();

  if (divlenwas!=setsize) {
    char tmptxt[100];
    sprintf(tmptxt, ": setsize=%d divlength=%d%s",
            setsize, divlenwas, divlenwas<setsize?" filling...":"");
    WriteLog(hdr+"size mismatch in ", ShortFileName(divfile), tmptxt);
    if (divlenwas<setsize) {
      IntVector ext(setsize-divlenwas);
      ext.Set(-1);
      for (int i=0; i<division_xx.Nitems(); i++)
        division_xx[i].Append(ext);
    }
  }

  Tac("ReadDivisionFile");

  int nlevels = NlevelsEvenAlien();

  if (!bmu_depths.size()) {
    bmu_depths = vector<int>(nlevels);
    for (int i=0; ok && i<division_xx.Nitems(); i++) {
      const IntVector& dv = division_xx[i];
      int l = -1;
      if (sscanf(dv.Label(), "level=%d", &l)!=1)
        ok = ShowError(hdr+"label is not of form level=%d,bmu=%d");

      else if (l<0 || l>=(int)bmu_depths.size())
        ok = ShowError(hdr+"label level=%d invalid, l="+ToStr(l)+
                       " bmu_depths.size()="+ToStr(bmu_depths.size()));

      else
        bmu_depths[l]++;

      ok = DivisionVectorSanityCheck(dv);
    }
  }
  
  stringstream msgss;
  for (size_t i=0; i<bmu_depths.size(); i++)
    msgss << (i?"+":" ") << bmu_depths[i];
  msgss << "=" << division_xx.Nitems() << " x " << divlenwas;

  if (divlenwas!=setsize)
    msgss << "->" << setsize;

  WriteLog("Read division file ", ShortFileName(divfile), msgss.str());

  Picsom()->PossiblyShowDebugInformation("ReadDivisionFile() ending");

  if (ok && !CreateBackReferences())
    ok = ShowError(hdr+"failed creating backrefs ", divfile);

  SetTargetCounts();

  RwUnlockWrite("ReadDivisionFile()");

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReadDivisionFileXml(const string& divname) {
  bool ok = true;
  string hdr = "TSSOM::ReadDivisionFileXml() : ";
  
  XmlDom doc = XmlDom::ReadDoc(divname);
  if (!doc.SchemaValidate(Picsom()->RootDir("xsd")+"/picsom.xsd"))
    return ShowError("Could not validate XML of division file!");
  
  XmlDom root = doc.Root();

  if (root.NodeName()!="objectindex")
    root = root.FindChild("objectindex");
  
  if (root.NodeName()!="objectindex")
    ok = ShowError(hdr+"<objectindex> not found while reading ", divfile);
    
  int maxlayers = atoi(root.Property("layers").c_str());

  vector<string> dstr = SplitInSomething("+",false,root.Property("depths"));
  bmu_depths = vector<int>(maxlayers);
  
  if ((int)dstr.size() != maxlayers)
    return ShowError("maxlayers != lenght of depths parameter as given in "
                      "div XML file.");
    
  for (int i=0; i<maxlayers; i++)
    bmu_depths[i]=atoi(dstr[i].c_str());

  InitializeDivision(maxlayers);    
  
  divmul_weights.reserve((int)DataSetSize());
  for (int i=0; i<(int)DataSetSize(); i++) {
    divmul_weights.push_back(vector<vector<float> >());
    for (int l=0; l<maxlayers; l++)
        divmul_weights[i].push_back(vector<float>(bmu_depths[l],0.0));
  }

  for (XmlDom oi = root.FirstChild(); oi; oi=oi.Next()) {
    if (!oi.IsElement() || oi.NodeName()!="objectinfo") continue;
    int ind = atoi(oi.Property("index").c_str());
    XmlDom tsloc = oi.FindChild("tssomlocation");
    for (XmlDom bl = tsloc.FirstChild(); bl; bl=bl.Next()) {
      if (!bl.IsElement() || bl.NodeName()!="bmulist") continue;
      int layer = atoi(bl.Property("layer").c_str());
      for (XmlDom bmu = bl.FirstChild(); bmu; bmu=bmu.Next()) {
        if (!bmu.IsElement() || bmu.NodeName()!="bmu") continue;
        int order = atoi(bmu.Property("order").c_str());
        int bmuindex = atoi(bmu.Property("index").c_str());

        float weight = 1.0;
        string bws = bmu.Property("weight");
        if (!bws.empty())
          weight = atof(bws.c_str());
        divmul_weights[ind][layer][order]=weight;          

        IntVector* dv = division_xx.FastGet(DivisionVectorIndex(layer,order));
        dv->FastSet(ind,bmuindex);
        if (order==0) {  // store sqrdist only for first bmu
          string sqrdist = bmu.Property("sqrdist");
          if (!sqrdist.empty()) {
            if (treesom.Level(layer).TrainDistLength()==0)
              treesom.Level(layer).TrainDistSetLength(DataSetSize());
            treesom.Level(layer).TrainDist(ind,atof(sqrdist.c_str()));
          }
        }
      }
    }
  }
  doc.DeleteDoc();
  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::DivisionVectorSanityCheck(const IntVector& v) const {
  // commented out 2.5.2006
  // if (FeatureTarget()==target_no_target)
  //   return true;

  // that !(a&b) check should be checked...
  for (int i=0; i<v.Length(); i++)
    if (v[i]>=0 && !(ObjectsTargetType(i)&FeatureTarget()))
      return ShowError("DivisionVectorSanityCheck() failed with "+
                       Name()+" i="+ToStr(i)+" <"+db->Label(i)
		       +"> v[i]="+ToStr(v[i])+
                       " : object="+TargetTypeString(ObjectsTargetType(i))+ 
                       " vs tssom="+FeatureTargetString());
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReWriteDivisionFile(bool cwd, bool zipped) {
  RwLockWrite("ReWriteDivisionFile()");

  const char *d = division_xx.GetDescription();
  string msg = d ? d : "";
  while (msg.size()>2 && msg.substr(msg.size()-2, 2)=="\n\n")
    msg.erase(msg.size()-1);

  stringstream nn;
  nn << division_xx.Nitems();

  int l = division_xx.Nitems() ? division_xx[0].Length() : 0;
  string msgadd = TreeSOM::DivDescriptionLine(Picsom()->UseTimeStamp(),
                                              nn.str(), l);
  msg += msgadd+"\n";
  division_xx.SetDescription(msg.c_str());

  // this SHOULD go thru a locking/temporary file/renaming sequence !

  string divfilename = cwd ? "./"+Name()+".div" : divfile.Name();
  RenameToOld(divfilename);

  if (zipped)
    msgadd += " (zipped)";

  division_xx.Zipped(zipped);
  bool ok = division_xx.Write(divfilename);
  if (!ok)
    ShowError("TSSOM::ReWriteDivisionFile() failed to write <", divfile, ">");
  else
    WriteLog("Rewrote modified division file ", ShortFileName(divfilename),
             " ", msgadd); 

  divfile.WasRead();
  division_changed = false;

  RwUnlockWrite("ReWriteDivisionFile()");

  return ok;
}

/////////////////////////////////////////////////////////////////////////////////

bool TSSOM::WriteXmlDivisionFile(bool zipped) {
  treesom.BmuDivDepthVector(bmu_depths);
  return treesom.WriteXmlDivisionFile(zipped,division_xx);
}
//    bool ok = true;
//    
//    XmlDom doc = XmlDom::Doc();
//    XmlDom root = doc.Root("objectindex","http://www.cis.hut.fi/picsom/ns",
//                            "picsom");
//    root.Prop("type","tssom");
//    root.Prop("name",Name());
//    root.Prop("layers",Nlevels());
//    
//    stringstream sstr;
//    for (int l=0; l<(int)Nlevels(); l++) 
//      sstr << (l?"+":"") << DivisionDepth(l);
//    root.Prop("depths",sstr.str());

//    int old_i=-1;
//    int old_l=-1;
//    XmlDom oi, tl, bl;
//    
//    for (int i=0; ok && i<(int)DataSetSize(); i++) {
//      for (int l=0; ok && l<(int)Nlevels(); l++) {
//        for (int d=0; ok && d<DivisionDepth(l); d++) {
//          const IntVector& div = Division(l,d);
//          if (div[i] == -1) continue;
//          
//          if (i!=old_i) {
//            oi = root.Element("objectinfo");
//            oi.Prop("index",i);
//            tl = oi.Element("tssomlocation");
//            old_i=i;
//          }    
//          if (l!=old_l) {
//            bl = tl.Element("bmulist");
//            bl.Prop("layer",l);
//            old_l=l;
//          }

//          XmlDom bmu = bl.Element("bmu");
//          bmu.Prop("order",d);
//          bmu.Prop("index",div[i]);
//  //      bmu.Prop("weight",1.0);  // should be 1.0 as default in XML schema (?)
//        }
//      }
//    }
//    string xmlname = Name()+"-div.xml"; 
//    doc.Write(xmlname, true, zipped);
//    doc.DeleteDoc();
//    return ok;
//  }

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReadConvolutionFile(bool force) {
  Obsoleted("TSSOM::ReadConvolutionFile()");

  RwLockRead("ReadConvolutionFile()");
  bool really = ReadConvolutionFileReally(force);
  RwUnlockRead("ReadConvolutionFile()");
  
  if (!really)
    return true;

  RwLockWrite("ReadConvolutionFile()");
  if (!ReadConvolutionFileReally(force)) {
    RwUnlockWrite("ReadConvolutionFile()");
    return true;
  }

  // Obsoleted("TSSOM::ReadConvolutionFile()");

  ifstream cnv(cnvfile_str.c_str());
  if (!cnv.good()) {
    ShowError("TSSOM::ReadConvolutionFile(", Name(), ",",
              ConvolutionFileName(), ") file open error");
    RwUnlockWrite("ReadConvolutionFile()");
    return false;
  }

  convolutions_str = "";

  bool ok = true;
  char line[1000];
  size_t level = 0;
  for (;;) {
    cnv.getline(line, sizeof line);
    if (!cnv.good())
      break;
    if (*line=='#' || strspn(line, " \t")==strlen(line))
      continue;

    if (level)
      convolutions_str += ",";
    convolutions_str += line;
    level++;
  }
  if (level<Nlevels()) {
    sprintf(line, "level=%d, Nlevels()=%d", (int)level, (int)Nlevels());
    ShowError("TSSOM::ReadConvolutionFile(", Name(), ",",
              ConvolutionFileName(), ") failed ", line);
    convolutions_str = "";
    ok = false;
  }

  if (ok)
    WriteLog("Read convolutions file ", ShortFileName(ConvolutionFileName()));
  Picsom()->PossiblyShowDebugInformation("ReadConvolutionFile() ending");

  RwUnlockWrite("ReadConvolutionFile()");

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReadOrdFile(bool force) {
  if (!force && ord.Nitems())
    return true;

  ord.Delete();
  ord.Read(ordfile_str);
  if (!ord.Nitems()) {
    ShowError("TSSOM::ReadOrdFile(", ordfile_str, ") failed");
    return false;
  }

  WriteLog("Read ord file ", ShortFileName(ordfile_str));
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ReadNbrFile(bool force) {
  if (!force && !nearest_neighbour.empty())
    return true;

  nearest_neighbour.clear();

  const string header="nearest neighbour file";

 ifstream is(nbrfile_str.c_str());

 if(!is.good()){
    ShowError("TSSOM::ReadNbrFile(", nbrfile_str, ") failed");
    return false;
 }
 
 string line;
 getline(is, line);

 if(line.find(header)==string::npos){
    ShowError("TSSOM::ReadNbrFile(", nbrfile_str, "): invalid header");
    return false;
 }

 while(is.good()){
   int nunits=-1;
   getline(is, line);
   sscanf(line.c_str(),"%d",&nunits);
   if(!is.good() || nunits<0) break;
   vector<vector<int> > lvec;
   for(int i=0;i<nunits;i++){
     getline(is, line);
     vector<int> v;
     stringstream ss(line);
     while(ss.good()){
       int idx;
       ss >> idx;
       v.push_back(idx);
     }
     lvec.push_back(v);       
   }
   nearest_neighbour.push_back(lvec);
 }


 cout << "TSSOM::ReadNbrFile("<< nbrfile_str<<"): read "<<
   nearest_neighbour.size()<<" levels:"<<endl;
 for(size_t l=0;l<nearest_neighbour.size();l++)
   cout << "level "<<l<<": "<<nearest_neighbour[l].size()<<" units"<<endl; 


  //ord.Delete();
  //ord.Read(ordfile_str);
  //if (!ord.Nitems()) {
  //  ShowError("TSSOM::ReadOrdFile(", ordfile_str, ") failed");
  //  return false;
  //}

  WriteLog("Read nbr file ", ShortFileName(nbrfile_str));
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::FindLabelIndices(bool force) {
  Tic("FindLabelIndices");
  bool ok = true;
  for (size_t i=0; i<Nlevels(); i++)
    ok = ok && FindLabelIndices(i, force);
  Tac("FindLabelIndices");

  WriteLog("Solved map label indices");
  Picsom()->PossiblyShowDebugInformation("FindLabelIndices() ending");

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::FindLabelIndices(int lev, bool force) {
  const TreeSOM& map = Map(lev);
  for (int i=0; i<map.Units(); i++) {
    const char *lab = map.Unit(i)->Label();
    int old = map.Unit(i)->LabelIndex();
    if (!force && old>=0 && lab)
      break;
    if ((old>=0 && !lab) || (old<0 && lab))
      force = true;
    else if (!force)
      continue;

    int idx = lab ? db->LabelIndex(lab) : -1;
    if (lab && idx<0)
      return false;

    map.Unit(i)->LabelIndex(idx);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::CreateBackReferences() {
  Tic("CreateBackReferences");
  int nlevels = NlevelsEvenAlien();

  backreflev.Delete();
  backreflev.GetEvenNew(nlevels-1);
  
  bool ok = true;
  for (int i=0; i<nlevels; i++)
    ok = ok && CreateBackReference(i);
  
  Tac("CreateBackReferences");

  WriteLog("Created back references");
  Picsom()->PossiblyShowDebugInformation("CreateBackReferences() ending");

  return ok && nlevels;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::CreateBackReference(int lev) {
  IntVectorSet& ref = backreflev[lev];
  int l = MapEvenAlien(lev).Units(), i;

  ref.Delete();
  ref.GetEvenNew(l-1);

  if (!HasDiv())
    return true;

  const IntVector& idx = Division(lev, 0);

  //  cout << "TSSOM::CreateBackReference("<<lev<<"): div has length "
  //     <<  idx.Length() << endl;

  /// First we count the numbers of backreferences from each map unit.
  IntVector count(l);
  for (i=0; i<idx.Length(); i++) {
    int j = idx[i];
    if (count.IndexOK(j))
      count[j]++;
    else if (j!=-1) {
      char tmp[1000];
      sprintf(tmp, "name=%s level=%d l=%d i=%d j=%d", Name().c_str(), lev, l, i, j);
      ShowError("TSSOM::CreateBackReference() failure : ", tmp);
    }
  }

  /// Then we allocate backreference vectors of correct length.
  /// (As a result we don't need to do Push() for all images!)
  for (i=0; i<l; i++)
    ref[i].Length(count[i]);

  count.Zero();
  for (i=0; i<idx.Length(); i++)
    if (idx[i]!=-1)
      ref[idx[i]].Set(count[idx[i]]++, i);

  return BackReferenceSanityCheck(lev);
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::BackReferenceSanityCheck(int lev) const {
  if (!backrefsanitychecks)
    return true;

  bool allow_errtype_B = db->EmptyLabelsAllowed();

  Tic("BackReferenceSanityCheck");
  bool ok = true;

  const IntVectorSet& ref = backreflev[lev];

  for (int j=0; j<Map(lev).Units(); j++) {
    const char *l = Map(lev).Unit(j)->Label();

    if (l==NULL && ref[j].Length()==0)
      continue;

    char errtype = 0;

    if (l!=NULL && ref[j].Length()==0)
      errtype = 'A';

    else if (l==NULL && ref[j].Length()!=0)
      errtype = 'B';

    else {
      bool found = false;
      for (int k=0; !found && k<ref[j].Length(); k++)
        if (Label(ref[j][k])==l)
          found = true;

      if (!found) 
        errtype = 'C';
    }

    if (!errtype || (errtype=='B' && allow_errtype_B))
      continue;

    stringstream tmp;
    tmp << errtype << ": name=" << Name() << " lev=" << lev << " j=" << j 
        << " l=" << (l?l:"(null)") << " ref[j].Length()=" << ref[j].Length();

    ShowError("TSSOM::BackReferenceSanityCheck() ", tmp.str());
    for (int k=0; k<ref[j].Length(); k++)
      cerr << "k=" << k << " idx=" << ref[j][k]
           << " label=" << Label(ref[j][k]) << endl;

    ok = false;
  }
 
  WriteLog("Sanity checked back references on level ", ToStr(lev));

  Tac("BackReferenceSanityCheck");

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

void TSSOM::ReduceMemoryUse() {
  // // data_xxx refers to VectorIndex::data ...
  // for (int i=0; i<data_xxx.Nitems(); i++)
  // data_xxx[i].Length(0);
}

  /////////////////////////////////////////////////////////////////////////////

  size_t TSSOM::GuessVectorLength() {
    if (VectorLength())
      return VectorLength();

    if (CheckDB()->SqlFeatures())
      return CheckDB()->SqlFeatureLength(Name(), false);

    if (codfile_str!="")
      return simple::FloatVectorSource::ReadVectorLength(codfile_str.c_str());

    // this will now be false if dat files have not yet been read
    if (DataFileCount(false)) 
      return simple::FloatVectorSource::ReadVectorLength(DataFileName(0).
							 c_str());

    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::AddToXML(XmlDom& xml, object_type ot, const string& namestr,
                       const string& specstr, const Query *q) {
    bool ok = true;

    switch (ot) {

    case ot_mapcollage:
      ok = ok && AddToXMLmapcollage(xml, namestr);
      //    cout << "Returned from TSSOM::AddTomapcollage" << endl;
      break; 

    case ot_featureinfo:
      ok = ok && AddToXMLfeatureinfo(xml, q);
      break;

    case ot_mapunitinfo: 
      ok = ok && AddToXMLmapunitinfo(xml, namestr, specstr);
      ok = ok && db->AddToXMLdatabaseinfo(xml, specstr, NULL, false, true);
      break;

    default:
      ShowError("TSSOM::AddToXML() unknown objecttype: ",
                ObjectTypeP(ot, true));
      ok = false;
    }
  
    return ok;
  }  

///////////////////////////////////////////////////////////////////////////////

// this is now in Index.
// bool TSSOM::AddToXMLfeatureinfo(XmlDom& xml, const Query *q) {
//   //
//   // THIS TAKES TOO MUCH TIME AND IS THEREFORE COMMENTED OUT
//   // ReadMapFile(false, true);
// 
//   // if (feature_target==target_no_target)
//   //  FeatureTarget(target_image);    
// 
//   XmlDom tree = xml.Element("feature");
// 
//   bool ok = AddToXMLfeatureinfo_static(tree);
// 
//   if (ok&&q)
//     ok = q->AddToXMLfeatureinfo_dynamic(tree, this,
//                                         q->FeatureShortNameIndex(Name()));
// 
//   return ok;
// }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::AddToXMLfeatureinfo_static(XmlDom& tree) const {
    VectorIndex::AddToXMLfeatureinfo_static(tree);

    tree.Element("levels", Nlevels());
    tree.Element("type",   TypeName());

    string tss = treesom.Structure();
    string s = "TS-SOM" + (tss!="0x0" ? ":"+tss : "");
    tree.Element("index", s);

    stringstream c;
    if (HasData())
      c << (c.str()!=""?",":"") << (HasDataVectors()?"Dlab+vec":"Dlab");

    if (HasTreeSOM())
      c << (c.str()!=""?",":"") << (HasTreeSOMvectors()?"Mlab+vec":"Mlab");

    if (HasUmatrix())
      c << (c.str()!=""?",":"") << "Umat";

    tree.Element("contains", c.str());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::AddToXMLmapunitinfo(XmlDom& xml, const string& unitstr,
                                  const string& imgsstr) {
    const char *unit = unitstr.c_str();
    const char *imgs = imgsstr.c_str();

    int level;
    IntPoint p;
    SolveLevelAndUnit(unit+Name().size(), &level, &p);  
  
    int x = p.X();
    int y = p.Y();

    char tmp[1024];

    XmlDom vi = xml.Element("mapunitinfo");
  
    sprintf(tmp, "%s[%d][%d,%d]", Name().c_str(), level, x, y);
    vi.Element("unit", string(tmp));
  
    if (Map(level).Unit(x, y)->Label())
      vi.Element("label", Map(level).Unit(x, y)->Label());
  
    IntMatrix myhits;
    IntVector imglist;
    int nhits = 0, idx = Map(level).ToIndex(x, y);

    for (size_t i=0; i<db->Size(); i++)
      if (Division(level, 0)[i] == idx) {
        nhits++;
        imglist.Append(i);
        if (db->ContentsClasses()) {
          IntVector row = db->ContentsRow(i);
          myhits.AppendRow(&row);
          if (myhits.Rows()==1)
            for (int j=0; j<myhits.Columns(); j++)
              myhits.GetColumn(j)->Label(db->ContentsClassLabel(j));
        }
      }

    CollapseAndArrangeRows(myhits);
  
    IntVector csum(myhits.Columns());
    for (int i=0; i<myhits.Columns(); i++)
      csum[i] = myhits.GetColumn(i)->Sum();

    const size_t hitstrlen = 1000;
    char hitstr[2*hitstrlen] = "";
    if (csum.Maximum()==1) {
      for (int i=0; i<myhits.Columns() && strlen(hitstr)<hitstrlen; i++)
        if (csum[i])
          sprintf(hitstr+strlen(hitstr), "%s%s", strlen(hitstr)?", ":"",
                  myhits.GetColumn(i)->Label());
    } else {
      int lim = csum.Maximum()/4;
      IntVector sort;
      sort.SetIndices(csum.Length());
      csum.SortDecreasingly(sort);
      for (int i=0; i<csum.Length() && strlen(hitstr)<hitstrlen; i++)
        if (csum[i]>lim)
          sprintf(hitstr+strlen(hitstr), "%s%s:%d", strlen(hitstr)?",":"",
                  myhits.GetColumn(sort[i])->Label(), csum[i]);
    }
  
    vi.Element("nhits", nhits);

    if (strlen(hitstr))
      vi.Element("hitlist", string(hitstr));

    bool ok = true;
  
    char tmp2[80] = "";
    if (imgs && strlen(imgs)<sizeof tmp2)
      strcpy(tmp2, imgs);  
    char *div = strchr(tmp2, '-');
  
    bool oinfo = true;

    if (!div) {
      ok = ok && db->AddToXMLobjectinfo(xml, Map(level).Unit(p)->Label(),
                                        NULL, false, true, oinfo, true, NULL,
					"");

    } else {
  
      *div = 0;
      int begin = atoi(tmp2);
      int end   = atoi(div+1);

      if (end > imglist.Length()) 
        end = imglist.Length();    

      vi.Element("shown", end);
    
      XmlDom vii = xml.Element("imagelist");

      for (int i=1; i<imglist.Length(); i++)
        if (!strcmp(db->Label(imglist[i]).c_str(),
		    Map(level).Unit(p)->Label())) {
          imglist.Swap(0,i);
          break;
        }
    
      for (int i=begin; i<=end; i++) 
        ok = ok && db->AddToXMLobjectinfo(vii, db->Label(imglist[i-1]),
                                          NULL, false, true, oinfo, true, NULL,
					  "");
    }  

    return ok;
  }

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::AddToXMLmapcollage(XmlDom& xml, const string& namestr) {
  string msg = "TSSOM::AddToXMLmapcollage("+namestr+")";

  if (DataBase::DebugVirtualObjects())
    cout << "Now in "+msg << endl;
  msg += " : ";

  size_t p = namestr.find('[');
  if (p==string::npos)
    return ShowError(msg+"invalid spec 1");

  int level = atoi(namestr.substr(p+1).c_str());
  if (!LevelOK(level))
    return ShowError(msg+"invalid spec 2");

  imagedata b, img = CreateMapCollage(level, 126, 96, 120, 90, b);
  bool persists = true;

  return Connection::AddToXMLimage(xml, ObjectTypeP(ot_mapcollage), 
                                   img, "", persists);
}

///////////////////////////////////////////////////////////////////////////////

bool GetNextTnSpecVal(string& s, char c, int& val) {
  size_t pos=s.find_first_of(c);
  if (pos==string::npos)
    return false; 

  s = s.substr(pos+1);
  if (!isdigit(s[0]))
    return false; 

  val = atoi(s.c_str());
  return true;
}

///////////////////////////////////////////////////////////////////////////////

TSSOM::map_tn_spec_t TSSOM::SolveMapTnSpec(const string& spec_str,
                                           size_t bw, size_t bh,
                                           size_t iw, size_t ih, 
                                           bool keepaspect) {
  map_tn_spec_t spec = {bw, bh, iw, ih, keepaspect};
  string s = spec_str;
  size_t dx=2, dy=2;

  // model: a46x46+2+2
  if (s[0] == 'a') {
    spec.keepaspect = true;
    s = s.substr(1);
  }

  int val = atoi(s.c_str());
  if (val>0) {
    spec.iw=val; 
    spec.ih=val;
    spec.bw=spec.iw+dx*2;
    spec.bh=spec.ih+dy*2;
  } else return spec;
      
  if (GetNextTnSpecVal(s,'x',val)) { 
    spec.ih=val;
    spec.bw=spec.iw+dx*2;
    spec.bh=spec.ih+dy*2;
  } else return spec;
      
  if (GetNextTnSpecVal(s,'+',val)) {
    spec.bw=spec.iw+val*2;
    spec.bh=spec.ih+val*2;
  } else return spec;

  if (GetNextTnSpecVal(s,'+',val)) {
    spec.bh=spec.ih+val*2;
  } 

  return spec;
}

///////////////////////////////////////////////////////////////////////////////

imagedata TSSOM::CreateMapCollage(size_t level, size_t cw, size_t ch,
                                  size_t iw, size_t ih, const imagedata& b,
                                  float c,
                                  bool keepaspect,
                                  size_t tlx, size_t tly,
                                  size_t brx, size_t bry,
                                  const string& s_pre,
                                  const string& s_suf) const {
  string msg = "TSSOM::CreateMapCollage() : ";

  imagedata empty;

  if (!LevelOK(level))
    return empty;

  if (!tlx && !tly && !brx && !bry) {
    brx = Map(level).Width()-1;
    bry = Map(level).Height()-1;
  }

  stringstream ss;
  ss << (keepaspect?"a":"") << iw << "x" << ih;
  string spec = ss.str();

  list<string> labset;

  bool ok = true;
  for (size_t y=tly; ok && y<=bry; y++)
    for (size_t x=tlx; ok && x<=brx; x++) {
      const FloatVector *unit = Map(level).Unit(x, y);
      string label = unit ? unit->LabelStr() : "";

      if (label!="") {
        target_type tt = ObjectsTargetType(db->LabelIndex(label));
        if (!Picsom()->TargetTypeContains(tt, target_image)) {
          string orig_label = label;
          string ttmsg = msg+"object target_type != target_image: ";
          bool video_target = Picsom()->TargetTypeContains(tt, target_video);
          if (video_target)
            label += ":kf1";
          else {
            ShowError(ttmsg+"and no special case has been implemented.");
            label = "";
          }
          if (db->LabelIndexGentle(label) == -1) {
            ttmsg += "and tried unsuccessfully with "+label;
            
            if (video_target) {
              bool label_bad = true;
              size_t l = orig_label.size();
              for (int kf=3; kf > 0 && label_bad; kf--) {
                label = orig_label.substr(0, l-1)+ToStr(kf)+":kf1";
                label_bad = (db->LabelIndexGentle(label) == -1);
                if (label_bad)
                  ttmsg += " and "+label;
              }
            }
            if (db->LabelIndexGentle(label) == -1) {
              ShowError(ttmsg+" and "+label+".");
              label = "";
            }
          }
        }
      }

      if (label!="")
        label = s_pre+label+s_suf;
      labset.push_back(label);
    }

  return db->CreateImageCollage(labset, spec, brx-tlx+1, cw, ch, b, c);
}

///////////////////////////////////////////////////////////////////////////////

FloatVector TSSOM::CreateCVector(int level) {
  // Obsoleted("TSSOM::CreateCVector()");

  FloatVector CV;

  // cout << "TSSOM::CreateCVector(): level: " << level << endl;

  if (!ReadConvolutionFile()) {
    ShowError("TSSOM::CreateCVector(): missing convolutions file");
    return FloatVector(0);
  }

  char tmp[1024];
  strcpy(tmp, convolutions_str.c_str());
  char *ptr2 = tmp;  

  for (int i=0; i<Map().Nlevels(); i++) {  
    char *ptr1 = strchr(ptr2, ',');
    if (ptr1) 
      *ptr1 = 0;

    if (i == level) {
      for (int j=0;;j++) {
        char *ptr3 = strchr(ptr2, ':');
        if (ptr3) 
          *ptr3 = 0;

        CV.Append(atof(ptr2));
        if (!ptr3)
          break;

        ptr2 = ptr3 + 1;        
      }
      break;
    }    
    ptr2 = ptr1 + 1;
  }
  
  // cout << "CreateCVector ending."<< endl;

  return CV;
}

///////////////////////////////////////////////////////////////////////////////

void TSSOM::PGMize(ostream& os, const simple::FloatMatrix& mat) const {
  float maxg = 15, ma = fabs(mat.Maximum()), mi = fabs(mat.Minimum());
  //float mul = fabs(ma)>fabs(mi) ? fabs(ma) : fabs(mi);

  if (ma)
    ma = maxg/(2*ma);
  if (mi)
    mi = maxg/(2*mi);

  os << "P2" << endl
     << mat.Columns() << " " << mat.Rows() << " " << (int)(2*maxg+3) << endl;

  for (int y=0; y<mat.Rows(); y++) {
    for (int x=0; x<mat.Columns(); x++) {
      int shift = 0, value;

      // if (true /*convmapimages*/) {
        if (mat.Get(y, x) > 0) {
          value = (int)floor(maxg/2+mat.Get(y, x)*ma+0.5);
        } else {
          value = (int)floor(maxg/2+mat.Get(y, x)*mi+0.5);
        }

//       } else {
//         if (mat.Get(y, x) > 0.0001) {
//           value = 15;
//         } else if (mat.Get(y, x) < -0.0001) {
//           value = 0;
//         } else {
//           value = 7;
//         }
//       }

//       Log() << "ma=" << ma << " mi=" << mi
//             << "mat.Get(y, x)=" << mat.Get(y, x) << " value=" << value
//             << endl;

//       if (IsInZoomArea(m, l, x, y))
//         shift = (int)maxg+1;
      
      
//       if (IsInSelection(m, l, x, y) && showselections) {
//         shift = 0;
//         value = (int)(2*maxg+3);
        
//       } else 
//         if (IsEmptyMappoint(m, l, x, y))
//           if (shift)
//             value = (int)floor(maxg/2+0.5);
//           else
//             value = (int)(2*maxg+2);
            

      os << value+shift << " ";
    }
    os << endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

IntPoint TSSOM::ZoomCenter(int level, const FloatPoint& p) const {
  const TreeSOM& som = Map(level);
  int w = som.Width()-1, h = som.Height()-1;
  int x = (int)floor(p.X()*w+0.5);
  int y = (int)floor(p.Y()*h+0.5);

  return IntPoint(x, y);
}

///////////////////////////////////////////////////////////////////////////////

void TSSOM::ZoomArea(int level, const FloatPoint& p, int zoomwidth,
                             IntPoint& ul, IntPoint& lr) const {
  const TreeSOM& som = Map(level);
  int w = som.Width()-1, h = som.Height()-1;

  IntPoint xy = ZoomCenter(level, p);
  
  ul.Set(xy.X()-zoomwidth/2, xy.Y()-zoomwidth/2);
  lr.Set(xy.X()+zoomwidth/2, xy.Y()+zoomwidth/2);
  
  if (ul.X()<0) { lr.X(lr.X()-ul.X()); ul.X(0); }
  if (ul.Y()<0) { lr.Y(lr.Y()-ul.Y()); ul.Y(0); }

  if (lr.X()>w) { ul.X(ul.X()-lr.X()+w); lr.X(w); }
  if (lr.Y()>h) { ul.Y(ul.Y()-lr.Y()+h); lr.Y(h); }

  if (ul.X()<0) { ul.X(0); }
  if (ul.Y()<0) { ul.Y(0); }
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::SolveLevelAndUnit(const char *txt, int *level,
                                      IntPoint *ip, FloatPoint *fp) const {
  if (level) *level = -1;
  if (ip) ip->SetUndefined();
  if (fp) fp->SetUndefined();

  if (level && *txt=='[') {
    sscanf(txt+1, "%d", level);
    
    const char *ptr = strchr(txt+1, ']');
    if (ip && ptr) {
      const char *ptr2 = strchr(ptr, '[');
      if (ptr2) {
        int x, y;
        sscanf(ptr2+1, "%d,%d", &x, &y);
        ip->Set(x, y);
      }
    }
    if (fp && ptr) {
      const char *ptr2 = strchr(ptr, '(');
      if (ptr2) {
        float x, y;
        sscanf(ptr2+1, "%f,%f", &x, &y);
        fp->Set(x, y);
      }
    }
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

const char *TSSOM::CreateMapUnitInfo(int /*l*/, int /*x*/, int /*y*/) const {
  Obsoleted("TSSOM::CreateMapUnitInfo()");
  return NULL;

  /*
  char *ret = NULL, tmp[1024];

  sprintf(tmp, "unit=%s[%d][%d,%d]\n", Name().c_str(), l, x, y);
  AppendString(ret, tmp);

  if (Map(l).Unit(x, y)->Label()) {
    sprintf(tmp, "label=%s\n", Map(l).Unit(x, y)->Label());
    AppendString(ret, tmp);
  }

  IntMatrix myhits;
  int nhits = 0, idx = Map(l).ToIndex(x, y);
  for (size_t i=0; i<db->Size(); i++)
    if (Division_y(l, 0)[i] == idx) {
      nhits++;
      if (db->ContentsClasses()) {
        IntVector row = db->ContentsRow(i);
        myhits.AppendRow(&row);
        if (myhits.Rows()==1)
          for (int j=0; j<myhits.Columns(); j++)
            myhits.GetColumn(j)->Label(db->ContentsClassLabel(j));
      }
    }

  //myhits.Dump(DumpLong, log);
  CollapseAndArrangeRows(myhits);
  //myhits.Dump(DumpLong, log);

  IntVector csum(myhits.Columns());
  for (int i=0; i<myhits.Columns(); i++)
    csum[i] = myhits.GetColumn(i)->Sum();

  char hitstr[1000] = "";
  if (csum.Maximum()==1) {
    for (int i=0; i<myhits.Columns(); i++)
      if (csum[i])
        sprintf(hitstr+strlen(hitstr), "%s%s", strlen(hitstr)?", ":"",
                myhits.GetColumn(i)->Label());
  } else {
    int lim = csum.Maximum()/4;
    IntVector sort;
    sort.SetIndices(csum.Length());
    csum.SortDecreasingly(sort);
    for (int i=0; i<csum.Length(); i++)
      if (csum[i]>lim)
        sprintf(hitstr+strlen(hitstr), "%s%s:%d", strlen(hitstr)?",":"",
                myhits.GetColumn(sort[i])->Label(), csum[i]);
  }

  sprintf(tmp, "nhits=%d\n", nhits);
  AppendString(ret, tmp);
  cout << "TSSOM::CreateMapUnitInfo(): " << tmp << flush;

  if (strlen(hitstr)) {
    sprintf(tmp, "hitlist=%s\n", hitstr);
    AppendString(ret, tmp);
    cout << "TSSOM::CreateMapUnitInfo(): " << tmp << flush;
  }

  return ret;
  */
}

///////////////////////////////////////////////////////////////////////////////

void TSSOM::CollapseAndArrangeRows(IntMatrix& mat) {
  for (int p=mat.Columns()-1; p>=0; p--) {
    int q;
    for (q=0; q<mat.Rows(); q++)
      if (mat.Get(q, p))
        break;
    if (q==mat.Rows())
      mat.RemoveColumn(p);
  }

  // mat.Dump(DumpLong, log);

  for (int i=mat.Rows()-1; i>=0; i--)
    for (int j=i-1; j>=0; j--) {
      int k;
      float mul = 0;
      for (k=0; k<mat.Columns(); k++) {
        int a = mat.Get(i, k), b = mat.Get(j, k);
        if (!a&&!b)
          continue;
        if ((!a&&b) || (a&&!b))
          break;
        if (!mul)
          mul = float(a)/b;
        else
          if (float(a)/b!=mul)
            break;
      }
      if (k==mat.Columns()) {
        for (k=0; k<mat.Columns(); k++)
          mat.Add(j, k, mat.Get(i, k));
        mat.RemoveRow(i);
      }
    }
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::CheckDataSetTargetTypes() const {
  int max_err = 100, n_err = 0, i = 0;

  string errhead = "TSSOM::CheckDataSetTargetTypes() ["+Name()+"] : ";
  stringstream msg;

  const FloatVectorSet &set = Data();
  if (!set.Nitems())
    return ShowError(errhead, "empty data set");

  if (feature_target==target_no_target)
    return ShowError(errhead, "feature_target==target_no_target");

  for (; i<set.Nitems() && n_err<max_err; i++) {
    if (!set[i].Label()) {
      msg << "i=" << i << " number=" << set[i].Number() << " empty label";
      ShowError(errhead, msg.str());
      n_err++;
    }

    int idx = set[i].Number();
    if (idx<0) {
      msg << "i=" << i << " label=<" << set[i].Label() << "> number="
          << set[i].Number() << " no index";
      ShowError(errhead, msg.str());
      n_err++;
    }

    bool tt_ok = PicSOM::TargetTypeContains(ObjectsTargetType(idx),
                                            feature_target);

    if (!tt_ok) {
      msg << "target_type [" << FeatureTargetString()
          << "] fails to coincide with object " << CheckDB()->ObjectDump(idx);
      ShowError(errhead, msg.str());
      n_err++;
    }
  }
  
  WriteLog("Checked target type of ", ToStr(i)+"/"+ToStr(set.Nitems()),
           " objects to match [", FeatureTargetString(), "]");
  
  return n_err==0;
}

  /////////////////////////////////////////////////////////////////////////////

  int TSSOM::CheckDataSet(const ground_truth& restr,
			  int verbose, bool allow_empty,
			  vector<size_t>& dat_missing, 
			  vector<size_t>& dat_orphan, 
			  vector<size_t>& dat_zero, 
			  vector<size_t>& div_missing,
			  bool check_orphans,
			  bool forcerecalculate) {
    string msg = "TSSOM::CheckDataSet() <"+Name()+"> : ";

    bool fast_bin_check = VectorIndex::FastBinCheck(); //  fast doesn't detect zero vectors
    bool check_nan_inf  = VectorIndex::NanInfCheck();

    DataBase *db = CheckDB();

    if (codfile_str!="")
      WriteLog("Starting to read in TSSOM COD file");
    ReadMapFile();
    target_type tt = FeatureTarget();
    if (HasTreeSOM() && (tt==target_no_target || tt==target_file))
      return ShowError(msg+"TSSOM's target type is ["+TargetTypeString(tt)+"]");
  
    bool hasdiv = HasDiv();
    if (hasdiv) {
      WriteLog("Starting to read in TSSOM DIV file");
      ReadDivisionFile(true);
    }

    //ReadDataFile();

    if (!HasTreeSOM()) {
      tt = FeatureTarget();
      if (tt==target_no_target || tt==target_file)
	return ShowError(msg+"data's target type is ["+
			 TargetTypeString(tt)+"]");
    }

    if (false && !Data().Nitems() && !allow_empty)
      return ShowError(msg+"empty set");

    // SetDataSetNumbers(false);
    // const FloatVectorSet& dat = Data();
    FloatVectorSet *data = NULL;
    string current_datfile = "";
 
    Tic("TSSOM::CheckDataSet");

    const string augm;
    if (db->UseBinFeaturesRead())
      BinDataOpen(db->OpenReadWriteFea(), db->Size(), false, augm);

    string zzz;
    if (db->UseBinFeaturesRead())
      zzz = " "+string(fast_bin_check?"fast":
		       VectorIndex::BinDataFullTest()?"full":"simple")
	+" bin data test"+string(check_nan_inf&&!fast_bin_check?
				 " with component check":"");
    WriteLog("Starting to check data for zero vectors, orphans, etc."+zzz);

    FloatVector dummy_v(1);
    dummy_v[0] = 42;

    int tot = 0;
    bool ok = true;
    for (size_t i=0; ok && i<db->Size(); i++) {
      if (verbose>2 && !(i%1000))
	cout << TimeStamp() << i << "/" << db->Size() << endl;
      else if (verbose>1 && !(i%1000))
	cout << i << "/" << db->Size() << "\r" << flush;

      target_type fttmask = tt;
      target_type ottmask =
	PicSOM::TargetTypeFileFullMasked(db->ObjectsTargetType(i));

      bool ttmatchold   = db->ObjectsTargetType(i)&tt; // was until 2013-03-19
      bool ttmatchnew   = fttmask==ottmask;            // was until 2014-01-23
      bool ttmatchnewer = (ottmask&fttmask)==ottmask;

      // added 2014-02-25 to prevent matching ottmask==image with "of" segment
      if (ottmask==target_image && fttmask==target_imagesegment)
	ttmatchnewer = false;
      
      // now (2014-01-23) allows sound features to be extracted from videos if
      // feature_target = sound+video

      // if ttmatch should be more often true then we should have multiple
      // tt's each for exact matching...
      // if (ttmatchold!=ttmatchnew)
      //   WarnOnce(msg+"ttmatch rule changed...");
      bool ttmatch = ttmatchnewer;

      bool hit = ttmatch && (restr.empty() || restr[i]==1);

      if (verbose>3)
	cout << db->ObjectDump(i) << " ttmatchold=" << ttmatchold
	     << " ttmatchnew=" << ttmatchnew
	     << " ttmatchnewer=" << ttmatchnewer
	     << " restr.empty()=" << restr.empty()
	     << " restr[i]=" << (int)restr[i]
	     << " hit=" << hit << endl;

      if (!hit)
	continue;

      tot++;

      bool delete_v = db->SqlFeatures() ||
	(db->UseBinFeaturesRead() && !fast_bin_check);
      const FloatVector *v = NULL;

      if (delete_v) {
	v = db->FeatureData(FeatureFileName(), i, false);

      } else if (db->UseBinFeaturesRead() && fast_bin_check) {
	if (BinDataExists(i))
	  v = &dummy_v;

      } else
	v = GetDataFromFile(i, data, current_datfile);

      if (!db->UseBinFeaturesRead() && !db->SqlFeatures() && check_orphans) {
	string datfile = db->MostAuthoritativeDataFile(i, this);
  
	if (datfile=="" && v)
	  return ShowError(msg+"problem 1");
  
	bool is_orphan = db->IsOrphanFileName(datfile);
	bool has_file  = HasDataFileName(datfile);
	bool add_file  = false;  // !HasDataFileName(datfile);
  
	if (datfile!="" && (is_orphan || add_file)) {
	  dat_orphan.push_back(i);
	  if (verbose>0)
	    cout << db->ObjectDump(i) << " orphan in " << Name()
		 << " datfile=<" << datfile << ">"
		 << " is_orphan=" << is_orphan << " has_file="  << has_file
		 << endl;
  
	  if (add_file)
	    AddDataFileName(datfile);
          
	  continue;
	}
      }

      if (forcerecalculate) {
	if (verbose>0)
	  cout << db->ObjectDump(i) << (v?" HAS":" has NOT")
	       << " data in " << Name() << " (re)calculating" << endl;
	dat_missing.push_back(i);
	if (delete_v)
	  delete v;
	continue;
      }

      if (!v) {
	/// string xxx = db->MostAuthoritativeDataFile(i, this);
	dat_missing.push_back(i);
	if (verbose>0)
	  cout << db->ObjectDump(i) << " no data in " << Name() << endl;
	continue;
      }

      if (v->IsZero()) {
	dat_zero.push_back(i);
	if (verbose>0)
	  cout << db->ObjectDump(i) << " zero data in " << Name() << endl;

      } else if (check_nan_inf) {
	for (size_t j=0; j<(size_t)v->Length(); j++) {
	  int cfy = fpclassify(v->Get(j));
	  if (cfy==FP_NAN || cfy==FP_INFINITE) {
	    if (verbose>0)
	      cout << db->ObjectDump(i) << " nan/infinite data in "
		   << Name() << endl;
	    dat_missing.push_back(i);
	    break;
	  }
	}
      }

      vector<int> jset;

      for (size_t j=0; j<(size_t)Nlevels(); j++)
	if (hasdiv && DivisionVectorIndex(j, 0, true)==-1)
	  WarnOnce("DivisionVectorIndex("+ToStr(j)+",0) failed");
      
	else if (!hasdiv || Division(j, 0)[i]==-1)
	  jset.push_back(j);
      
      if (jset.size()) {
	div_missing.push_back(i);
	if (verbose>0)
	  cout << db->ObjectDump(i) << " no div for " << MapName(false, jset)
	       << endl;
      }

      if (delete_v)
	delete v;
    }

    delete data;

    Tac("TSSOM::CheckDataSet");

    return ok ? tot : 0;
  }

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::ApplyRestriction(const string& r, target_type tt) {
  if (r=="")
    return true;

  bool expand = true;
  ground_truth g = db->GroundTruthExpression(r, tt, -1, expand);
  int n = 0, z = 0, p = 0;
  g.TernaryCounts(n, z, p);

  int o = Data().Nitems();

  for (int i=o-1; i>=0; i--) {
    int idx = db->LabelIndex(Data()[i].Label());
    if (g[idx]!=1)
      Data().Remove(i);
    // cout << i << " " << Data()[i].Label() << " "
    //         << idx << " " << g.IndexOK(idx) << " " << g.Get(idx) << endl;
  }

  char msg[1000];
  sprintf(msg, "Restriction of %s by %s with target type %s (%d,%d,%d)=%d"
          " changed data set size %d->%d",
          Name().c_str(), r.c_str(), TargetTypeString(tt).c_str(),
          n, z, p, g.Length(), o, Data().Nitems());

  WriteLog(msg);

  return true;
}

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::Interpret(const string& key, const string& val, int& res) {
    res = 1;

    if (key=="meandistance") {
      SetMeanDist(val);
      return true;
    }

    if (key=="quanterrorL1") {
      vector<string> v = SplitInSpaces(val);
      if ((int)v.size()!=treesom.Nlevels())
        ShowError("mismatch v.size()!=treesom.Nlevels()");
      for (size_t i=0; i<v.size() || (int)i<treesom.Nlevels(); i++)
        if ((int)i<treesom.Nlevels())
          treesom.Level(i).MeanDistance(i<v.size() ? atof(v[i].c_str()) : -1.0);
    }

    if (key=="quanterrorL2") {
      vector<string> v = SplitInSpaces(val);
      if ((int)v.size()!=treesom.Nlevels())
        ShowError("mismatch v.size()!=treesom.Nlevels()");
      for (size_t i=0; i<v.size() || (int)i<treesom.Nlevels(); i++)
        if ((int)i<treesom.Nlevels())
          treesom.Level(i).RmsDistance(i<v.size() ? atof(v[i].c_str()) : -1.0);
    }

    if (key=="background") {
      SetBackGround(val);
      return true;
    }

    if (key=="show_map_values") {
      show_map_values = IsAffirmative(val);
      return true;
    }

    if (key=="show_data_values") {
      show_data_values = IsAffirmative(val);
      return true;
    }

    res = 0;

    return VectorIndex::Interpret(key, val, res);
  }

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::LoadAndMatchFeatures(int idx, bool may_add, bool take_all) {
  // this should be in VectorIndex !
  // MatchFeatures called at the bottom should be a virtual method
  // declared in VectorIndex.

  char errhead[1000];
  sprintf(errhead, "TSSOM::LoadAndMatchFeatures() [%s,%d] : ",
          Name().c_str(), idx);

  const string& label = Label(idx);
  if (label=="")
    return ShowError(errhead, "empty label");

  string in = CalculatedFeaturePath(label, false, false), in1 = in;
  if (!FileExists(in))
    in = CalculatedFeaturePath(label, false, true);

  if (!FileExists(in))
    return ShowError(errhead, "file <"+in1+"> nor <"+in+"> not found");

  WriteLog("Reading feature vectors from ", ShortFileName(in));

  FloatVectorSet set(in.c_str());
  if (!set.Nitems())
    return ShowError(errhead, "no vectors found");

  for (int i=0; i<set.Nitems(); i++) {
    if (set[i].Label()==NULL) {
      ShowError(errhead, "no label in a vector");
      continue;
    }

    if (!take_all && set[i].LabelStr()!=label)
      continue;

    if (CheckDB()->LabelIndexGentle(set[i].Label())==-1) {
      if (!may_add)
        ShowError(errhead, "not allowed to add label <", set[i].Label(), ">");
      else
        if (!CheckDB()->AddLabelAndParents(set[i].Label(), feature_target,
					   false))
          ShowError(errhead, "failed to add label <", set[i].Label(), ">");
    }
  }

  if (!VectorLength())
    return true;

  if (set.VectorLength()!=(int)VectorLength()) {
    stringstream ss;
    ss << "(set.VectorLength()==" << set.VectorLength() << " != "
       << "VectorLength()==" << VectorLength() << ")";
    return ShowError(errhead, "vector lengths differ: ", ss.str());
  }

  size_t oldsize = NdataVectors();
  bool ok = true;
  for (int i=0; ok && i<set.Nitems(); i++)
    if (take_all || set[i].LabelStr()==label)
      ok = MatchFeatures(set[i]);

  if (NdataVectors()!=oldsize)
    WriteLog("Count of stored feature vectors changed from "+ToStr(oldsize)
             +" to "+ToStr(NdataVectors()));

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::MatchFeatures(const FloatVector& vec) {
  string errhead = "TSSOM::MatchFeatures() "+Name()+ " : ";

  if (!HasNonZeroTreeSOMvectors())
    return ShowError(errhead, "no map vectors available");

  int idx = vec.Label() ? LabelIndex(vec.Label()) : -1;
  if (idx<0)
    return ShowError(errhead, "idx<0");

  if (false && !DivisionIndexOldStyle())
    return ShowError(errhead, "not simple old .div file");

  treesom.BmuDivDepthVector(bmu_depths);

  int ncomp = 0;
  for (size_t p=0; p<bmu_depths.size(); p++) ncomp += bmu_depths[p];

//  if (!ncomp || ncomp!=division_xx.Nitems() || bmu_depths.size()!=Nlevels())
//    return ShowError(errhead,"bad bmu_depths!");

  IntVector bmu = treesom.FindBestMatches(vec);

  if (bmu.Length()!=ncomp)
    return ShowError(errhead, "bmu.Length()!=ncomp (", ToStr(bmu.Length()), 
                     "!=", ToStr(ncomp), string(") with <")+vec.Label()+">");

  if (HasData()) {
    FloatVector *c = DataAppendCopy(vec, false);
    c->Number(idx);
  }

  char msg[1000];
  sprintf(msg, "Object #%d <%s> maps to units", idx, vec.Label());

  for (int p=0, i=0; p<(int)Nlevels(); p++) {
    for (int d=0; d<bmu_depths[p]; d++, i++)
      sprintf(msg+strlen(msg), " %d: %d(%d,%d)", p, bmu[i],
              Map(p).XCoord(bmu[i]), Map(p).YCoord(bmu[i]));
  }

  WriteLog(msg);

  SetDivision(idx,bmu);

  AddOneTarget(ObjectsTargetType(idx));

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::SetDivision(int idx, IntVector &bmu) {
  string errhead = "TSSOM::SetDivision() "+Name()+ " : ";

  if (idx<0)
    return ShowError(errhead, "index<0");

  bool use_depth = bmu.Length()>(int)Nlevels();

  for (int p=0, i=0; p<(int)Nlevels(); p++) {
    // only add best "BMU" for each map level...
    backreflev[p][bmu[i]].Push(idx);
    int max_depth = use_depth ? bmu_depths[p] : 1;
    for (int d=0; d<max_depth; d++, i++) {
      if (bmu[i]==-1)
        return ShowError(errhead, "bmu[i]==-1");

      IntVector& divvec = division_xx[i];
      while (divvec.Length()<=idx)
        divvec.Push(-1);

      // it might be not allowed to have divvec[idx]!=-1 ...
      if (divvec[idx]!=-1 && divvec[idx]!=bmu[i]) {
        char msg[1000];
        sprintf(msg, "idx=%d i=%d old=%d new=%d", idx, i, divvec[idx], bmu[i]);
        return ShowError(errhead, "division[i][idx] already set : ", msg);
      }
    
      divvec[idx] = bmu[i];
      if (!Map(p).Unit(bmu[i])->Label()) {
        Map(p).Unit(bmu[i])->Label(Label(idx));
        Map(p).Unit(bmu[i])->LabelIndex(idx);
      }
    }
  }

  division_changed = true;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TSSOM::SolveFeatureCalculationCommand(bool no_recurs, bool angry) {
  string errhead = "TSSOM:SolveFeatureCalculationCommand() [";
  errhead += Name() + "] : ";

  features_command.clear();

  if (!no_recurs && !ReadMapFile(false, true))
    return ShowError(errhead, "ReadMapFile() failed");

  xmlDocPtr xml = treesom.XMLDescription();
  if (!xml)
    return ShowError(errhead, "no xml in");

  xmlNodePtr ts = xmlDocGetRootElement(xml);
  if (!ts || xmlStrcmp(ts->name, (XMLS)"treesom"))
    return ShowError(errhead, " no <treesom> element");

  xmlNodePtr feat = FindXMLchildElem(ts, "feature");
  if (!feat)
    return angry ? ShowError(errhead, " no <feature> element") : false;

  xmlNodePtr cmdline = FindXMLchildElem(feat, "cmdline");
  if (!cmdline || !cmdline->children || !cmdline->children->content)
    return angry ? ShowError(errhead, "no valid <cmdline>") : false;

  bool edit = false;
  string cmdstr = (const char*) cmdline->children->content, cmdwas = cmdstr;

  istringstream ss(cmdstr);
  vector<string> args;
  
  copy(istream_iterator<string>(ss), istream_iterator<string>(), 
       back_inserter(args));
  
  string cmd = args[0];
  size_t p = cmd.find("/picsom/features/features.");
  if (p==string::npos && cmd!="" && cmd[0]!='/')
    p = 0;
  if (p!=string::npos) {
    string exe = Picsom()->FindExecutable("features", "features");
    if (exe!="") {
      args[0] = exe;
      edit = true;
    }
  }

  if (find(args.begin(),args.end(),"-OR") == args.end()) {
    vector<string>::iterator o = args.begin();
    for (; o != args.end() && o->substr(0,2) != "-O"; o++) ;
    if (o != args.end())
      args.insert(args.erase(o),"-OR");
    else
      args.insert(args.begin()+1,"-OR");
  }

  if (edit)
    WriteLog("Features command edited [", cmdwas, "] -> [", ToStr(args), "]");

  FeaturesCommand(args, db->DebugFeatures());

  return true;
}

///////////////////////////////////////////////////////////////////////////////

// bool TSSOM::ReadAndCalculateFeatures(vector<int>& idxv, set<string>& segments) {
//   ReadMapFile(false, true);
//   ReadDivisionFile(false);
//   if (CompatibleTarget(ObjectsTargetType(idxv.front())))
//     return CalculateFeaturesInner(idxv, segments);
//   return true;
// }

///////////////////////////////////////////////////////////////////////////////

  bool TSSOM::CalculateFeatures(vector<size_t>& idxv, set<string>& done_segm,
				XmlDom& xml, bin_data *raw) {
  ReadMapFile(false, true);
  ReadDivisionFile(false);

  target_type tt = ObjectsTargetType(idxv.front());

  if (CompatibleTarget(tt)) {
    list<incore_feature_t> incore;
    return VectorIndex::CalculateFeatures(idxv, incore, done_segm, xml, raw);
  }

  return false;  // was true until 2011-09-05
}

///////////////////////////////////////////////////////////////////////////////

void TSSOM::HackSegmentationCmd(vector<string>& cmd, const string& label) {
  vector<string>::iterator pos = cmd.end();
  vector<string>::iterator start = find(cmd.begin(), cmd.end(), "-i");
  vector<string>::iterator last = cmd.end();
  last--;
  // CalculateFeatures() now appends the filename after calling this
  // should that change be taken into account ?
  
  if (start != cmd.end()) {
    if (start == last)
      return;
    pos = cmd.erase(start);
  }
  
  start = find(cmd.begin(),cmd.end(),"-X");
  if (start != cmd.end()) {
    if (start == last)
      return;
    pos = cmd.erase(start);
  }

  start = find(cmd.begin(),cmd.end(),"-I");
  if (start != cmd.end()) {
    pos = cmd.erase(start);
  }

  if (pos == cmd.end())
    pos = cmd.begin()+1;

  string seg = db->SolveObjectPath(label, "segments", "ib+ps:", true);

  if (seg != "" && FileExists(seg+".seg"))
    cmd.insert(pos, "-X " + seg + ".seg ");
           
  return;
}

  /////////////////////////////////////////////////////////////////////////////

  int TSSOM::AlienDataBaseSize() const { return AlienDataBase()->Size(); }

  const TSSOM *TSSOM::AlienDataTssom() const {
    TSSOM *t = (TSSOM*)db->FindIndex(AlienFeatureName());
    if (!t) {
      ShowError("TSSOM::AlienDataTssom() : FindIndex() failed");
      SimpleAbort();
    }
    t->ReadMapFile();
    return t;
  }

  ///  Utility to give alien map used by native data.
  const TSSOM *TSSOM::AlienMapTssom() const {
    TSSOM *t = (TSSOM*)AlienDataBase()->FindIndex(AlienFeatureName());
    if (!t) {
      ShowError("TSSOM::NlevelsAlienMap() : FindIndex() failed");
      SimpleAbort();
    }
    t->ReadMapFile();
    return t;
  }

  /////////////////////////////////////////////////////////////////////////////

  void TSSOM::SetMetric(FloatVector::DistanceType* dt) {
    if (!HasTreeSOM())
      return;

    for (size_t i=0; i<Nlevels(); i++) {
      TreeSOM& ts = Map(i);
      ts.Metric(dt);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  string TSSOM::CalculatedFeaturePath(const string& label, bool /*isnew*/,
                                      bool strip, bool gzip) const {
    string out = db->CalculatedFeaturePath(label, true);
    if (out=="")
      return "";

    string oname = db->FeatureOutputName(Name());
    if (oname=="")
      oname = Name();

    if (!strip)
      return out+"-"+oname+".dat"+(gzip?".gz":"");

    size_t p = out.rfind('/');
    if (p!=string::npos)
      out.erase(p+1);

    return out+oname+".dat"+(gzip?".gz":"");
  }

  /////////////////////////////////////////////////////////////////////////////

  void *TSSOM::RawDataSourceCallBack(void *d) {
    raw_data_spec_t *s = (raw_data_spec_t*)d;
    return s->tssom->RawDataSource(*s);
  }

  /////////////////////////////////////////////////////////////////////////////

  FloatVector *TSSOM::RawDataSource(raw_data_spec_t& s) {
    if (s.index==s.ntotal) {
      PrepareRawData(s, false);
      return NULL;
    }

    s.number++;
    s.index++;

    size_t i = (size_t)s.randvar.RandomInt(s.vec.size());
    const labeled_float_vector& vec = s.vec[i];

    const char  *l = vec.second=="" ? NULL : vec.second.c_str();
    FloatVector *v = new FloatVector(vec.first.size(), &vec.first[0], l);

    s.vec[i] = NextFeatureVector(s);

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  int TSSOM::PrepareRawData(raw_data_spec_t& s, bool full_init) {
    s.index = 0;

    if (!full_init)
      return 0;

    s.number = 0;
    s.nfiles = 0;

    ground_truth gt = CheckDB()->GroundTruthType(FeatureTarget(), "", "",
                                                 vector<string>());
    s.objects = gt.TernaryAND(CheckDB()->RestrictionGT()).indices(1);
    WriteLog("Solved indices for "+ToStr(s.objects.size())+" objects of type ["
             +TargetTypeString(FeatureTarget())+"] for raw data extraction");

    s.input.clear();

    s.vec.clear();
    s.vec.resize(s.nobjects*s.nsamples);

    for (size_t i=0; i<s.vec.size(); i++)
      s.vec[i] = NextFeatureVector(s);

    return s.vec.empty() ? 0 : s.vec[0].first.size();
  }

  /////////////////////////////////////////////////////////////////////////////

  cox::labeled_float_vector TSSOM::NextFeatureVector(raw_data_spec_t& s) {
    bool debug = false;

    if (s.input.empty())
      s.input = NextFeatureFile(s);

    if (s.input.empty())
      throw string("TSSOM::NextFeatureVector() failed");

    labeled_float_vector v = *s.input.begin();
    s.input.erase(s.input.begin());

    if (!s.mean.empty()) {
      size_t l = s.mean.size();
      if (l==v.first.size() && l==s.stdev.size())        
        for (size_t i=0; i<l; i++) {
          if (debug)
            cout << "NextFeatureVector() i=" << i << " " << v.first[i];
          v.first[i] = (v.first[i]-s.mean[i])/s.stdev[i];
          if (debug)
            cout << " -> " << v.first[i] << endl;
        }
    }

    return v;
  }

  /////////////////////////////////////////////////////////////////////////////

  list<cox::labeled_float_vector> TSSOM::NextFeatureFile(raw_data_spec_t& s) {
    bool debug = false;

    list<labeled_float_vector> l;

    size_t i = (size_t)s.randvar.RandomInt(s.objects.size());
    if (i>=s.objects.size())
      throw string("TSSOM::NextFeatureFile() failing");
    
    int idx = s.objects[i]; 
    const string& lab = CheckDB()->Label(idx);
    string path = CheckDB()->SolveObjectPath(lab);

    WriteLog("Extracting raw "+Name()+" features for <"+lab+"> from <"+
             ShortFileName(path)+">");

    vector<string> cmd(features_command.begin(), features_command.end());
    if (cmd.empty()) { // should check that this is a raw feature...
      cmd.push_back("");
      cmd.push_back(Name());
    }

    SetFeaturesDebugging(cmd);

    cmd[0] = "*picsom-features-internal*";

    for (vector<string>::iterator a=cmd.begin(); a!=cmd.end(); a++)
      if (a->find("-O")==0) {
        cmd.erase(a);
        a = cmd.begin();
      }
    cmd.insert(cmd.begin()+1, "-Ox");
    cmd.insert(cmd.begin()+1, "-r");
    cmd.push_back(path);

    list<incore_feature_t> incore;
    feature_result tmp;
    Feature::Main(cmd, incore, &tmp);
    if (tmp.data.empty())
      throw string("Feature::Main() failed");

    size_t dats = tmp.data.size(), datso = dats;
    size_t maxj = s.nsamples<dats ? s.nsamples : dats;
    for (size_t j=0; j<maxj; j++) {
      size_t k = (size_t)s.randvar.RandomInt(dats);
      if (debug)
        cout << "NextFeatureFile() dats=" << dats << " nsamples=" << s.nsamples
             << " maxj=" << maxj << " k=" << k << endl;

      list<labeled_float_vector>::iterator ki = tmp.data.begin();
      while (k--)
        ki++;
      l.push_back(*ki);

      tmp.data.erase(ki);
      dats--;
    }
    WriteLog("Pooled "+ToStr(maxj)+" raw feature vectors out of "+
             ToStr(datso)+" extracted from <"+lab+">");

    s.nfiles++;

    return l;
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void TSSOM::BmuDepths(const IntVector& v) {
    bmu_depths.clear();
    for (int i=0; i<v.Length(); i++)
      bmu_depths.push_back(v[i]);
  }

  /////////////////////////////////////////////////////////////////////////////

  void TSSOM::InitializeDivision(int ll) {
    if (ll==-1) ll=Nlevels();

    if ((int)bmu_depths.size() < ll) {
      ShowError("TSSOM::InitializeDivision(): bmu_depths not initialized!");
      return;
    }

    for (int l=0; l<ll; l++) {
      for (int d=0; d<bmu_depths[l]; d++) {
        stringstream labss;
        labss << "level=" << l << ",bmu=" << d;
        IntVector* dv = new IntVector((int)DataSetSize(), NULL, 
                                      labss.str().c_str());
        dv->Set(-1);
        division_xx.Append(dv);
      }
    }
  }


  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::CreateClassifier(const string& name, const string& argv,
                               const ground_truth_set& set) {

    string nnn = argv.substr(0, 3), aaa = argv.substr(3);

    if (nnn=="knn")
      return CreateKNN(name, aaa, set);

    if (nnn=="lsc")
      return CreateLSC(name, aaa, set);

    if (nnn=="som")
      return CreateSOM(name, aaa, set);

    return ShowError("TSSOM::CreateClassifier() : ["+argv+
                     "] does not name a know method");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::CreateSOM(const string&, const string&,
                        const ground_truth_set&) {
    // stuff should be moved from Analysis::CreateSOM() to here
    return ShowError("TSSOM::CreateSOM() not implemented yet");
  }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::CreateKNN(const string& knnname, const string& argvin,
                        const ground_truth_set& cls) {
    string argv = argvin;
    if (argv=="")
      argv = "3";

    size_t k = atoi(argv.c_str()), discard = 0;

    size_t p = argv.find_first_not_of("0123456789");
    string opt = p!=string::npos ? argv.substr(p) : "";
    if (opt[0]==',') {
      opt.erase(0, 1);
      discard = atoi(opt.c_str());
      p = opt.find_first_not_of("0123456789");
      opt = p!=string::npos ? opt.substr(p) : "";
    }

    bool sqr_avg   = opt.find('q')!=string::npos;
    bool sum_break = opt.find('b')!=string::npos;

    cox_knn_set_t set = CoxVectors(cls);
    
    classifier_data *c = AddClassifier("knn", knnname);
    if (!c)
      return ShowError("");

    cox::knn& knn = c->knn;
    knn.add(set);
    knn.k(k);
    knn.discard(discard);
    knn.sqr_avg(sqr_avg);
    knn.sum_break(sum_break);

    WriteLog("Successfully created classifier: "+knn.description());

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::CreateLSC(const string&, const string&,
                        const ground_truth_set&) {
    return ShowError("TSSOM::CreateLSC() not implemented yet");
  }

  /////////////////////////////////////////////////////////////////////////////

  TSSOM::cox_knn_set_t TSSOM::CoxVectors(const ground_truth_set& cls) {
    cox_knn_set_t set;
    for (size_t i=0; i<cls.size(); i++) {
      string lab = cls[i].label();
      cox_knn_set_t tmp = CoxVectors(cls[i], lab);
      set.splice(set.end(), tmp);
    }
    return set;
  }

  /////////////////////////////////////////////////////////////////////////////

  TSSOM::cox_knn_set_t TSSOM::CoxVectors(const ground_truth& select,
					 const string& lab) {
    string msg = "CoxVectors() : ";

    cox_knn_set_t ret;

    if (!SetDataSetNumbers(false, false)) {
      ShowError(msg+"setting data set numbers in <", Name(),
                "> failed");
      return ret;
    }

    DataBase *db = CheckDB();

    const FloatVectorSet& data = Data();
    ground_truth not_found = select;
    FloatVectorSet set;
  
    for (int j=0; j<data.Nitems(); j++) {
      int idx = data[j].Number();
      if (select[idx]==1) {
        set.AppendCopy(data[j]);
        if (not_found[idx]!=select[idx]) {
          stringstream ss;
          ss << Name() << " j=" << j << " idx=" << idx
             << " lab=<" << lab << ">" 
             << " not_found[idx]=" << (int)not_found[idx]
             << " != select[idx]=" << (int)select[idx] << " "
             << db->ObjectDump(idx);
          ShowError(msg+ss.str());
          return ret;
        }
      }
      not_found[idx] = 0;
    }

    size_t nnfound = not_found.positives();
    if (nnfound) {
      ShowError(msg+ToStr(nnfound)+" vectors not found");
      return ret;
    }

    map<size_t,int> idxmap;
    for (int j=0; j<set.Nitems(); j++) {
      size_t idx = set[j].Number();
      if (select[idx]!=1) {
        ShowError(msg+"strange vector number");
        return ret;
      }
      if (idxmap.find(idx)!=idxmap.end()) {
        ShowError(msg+"duplicate vector number");
          return ret;
      }
      idxmap[idx] = j;
    }
    
    size_t vl = set.VectorLength();
    for (size_t j=0; j<select.size(); j++)
      if (select[j]==1) {
        const FloatVector& v = set[idxmap[j]];
          const float *fv = v;
          ret.push_back(cox::knn::lvector_t(vector<float>(fv, fv+vl), lab));
      }
    
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////

  // bool TSSOM::() {
  // 
  // }

  /////////////////////////////////////////////////////////////////////////////

  bool TSSOM::State::Initialize(bool keep, size_t mcount, bool do_ent) {
    bool angry = false;

    matrixcount = mcount;
    
    size_t levels = TsSom().Nlevels();

    if (!levels) {
      if (!angry)
	return true;

      return ShowError("TSSOM::State::Initialize() : levels==0");
    }

    const vector<float> nanv(levels, nan());
    if (!keep || tssom_level_weight.size()!=levels)
      tssom_level_weight = nanv;
    
    if (do_ent && (!keep || entropy.size()!=levels))
      entropy = entropy_r = entropy_p = entropy_n = nanv;
    
    if (!keep || vqulist.size()!=levels) {
      vqulist = vector<VQUnitList>(levels);
      objlist = vector<ObjectList>(levels);
    }

    if (!keep || hit.size()!=MatrixCount()*levels) {
      hit     = vector<simple::FloatMatrix>(MatrixCount()*levels);
      cnv     = vector<simple::FloatMatrix>(MatrixCount()*levels);
    }

    return true;
  }

  // class TSSOM ends here

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
