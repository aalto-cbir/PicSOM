// -*- C++ -*-  $Id: Valued.C,v 2.39 2012/06/05 14:05:07 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <ValuedList.h>
#include <Query.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace picsom {
  static const string Valued_C_vcid =
    "@(#)$Id: Valued.C,v 2.39 2012/06/05 14:05:07 jorma Exp $";

  ///
  bool Valued::pre707bug = false;

  /////////////////////////////////////////////////////////////////////////////

  void Object::SortListByValue(ObjectList& list) {
    // This _is_ still used by Query::StageFinalSelect().
    //Obsoleted("Object::SortList(ObjectList&)");

    int k = 0;
    for (size_t j=1; j<list.size(); j++) {
      if (!list[j].Retained())
	continue; 
      float val = list[j].Value();
      for (k=j-1; k>=0; k--)
	if (list[k].Retained() && list[k].Value()>val)
	  break;
      
      /*
      cout << "Object::SortList() val[" << j << "]=" << val
	   << " val[" << k+1 << "]=" << list[k+1].Value() << endl;
      */

      list.move(j, k+1);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void Object::SortListByRank(ObjectList& list) {
    int k = 0;
    for (size_t j=1; j<list.size(); j++) {
      if (!list[j].Retained())
	continue; 
      int r = list[j].Rank();
      for (k=j-1; k>=0; k--)
	if (list[k].Retained() && list[k].Rank()<=r)
	  break;
      
      /*
      cout << "Object::SortList() val[" << j << "]=" << val
	   << " val[" << k+1 << "]=" << list[k+1].Rank() << endl;
      */

      list.move(j, k+1);
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  Object::Object(DataBase *b, int i, select_type t, double v, 
                 const string& e, int si,
                 const FloatVector *vv, 
                 map<string,aspect_data> *ar) : Valued(v) {

    genuine_relevance = true;
    below_value = 0;
    above_value = 0;
    sibling_value = 0;
    not_really_seen = false;
    rank = round = -1;

    if (!b) {
      ShowError("Object::Object() : b==NULL");
      return;
    }

    if (i<0) {                // Query::SelectAroundMapPoint() may do it...
      oi = b->DummyObject();
      if (t!=select_map)
        ShowError("Object::Object() : i<0 && t!=select_map");
    }
    else {
      oi = b->FindObject(i);
      if (!oi) {
        ShowError("Object::Object() : object #", ToStr(i), " not found");
        oi = b->DummyObject();
      }
    }

    sel_type  = t;
    extra_str = e;

    HandleStageInfo(si, vv);

    if (ar)
      aspect_relevance.Aspects(*ar);
    else {
      const DataBase *d = GetDataBase();
      if (d) {
        bool segm_is_mod = d->SegmentIsModifier();
        target_type tt = PicSOM::TargetTypeMasked(TargetType(), segm_is_mod);
        d->DefaultAspects(tt).copyTo(aspect_relevance);
      }
    }

    // cout << DumpStr() << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  Object::Object(const Object& i) : Valued(i) {
    oi                = i.oi;

    sel_type          = i.sel_type;
    extra_str         = i.extra_str;
    stagevalues       = i.stagevalues;

    genuine_relevance = i.genuine_relevance;
    above_value       = i.above_value;
    below_value       = i.below_value;
    sibling_value     = i.sibling_value;
    not_really_seen   = i.not_really_seen;

    aspect_relevance  = i.aspect_relevance;
    rel_distribution  = i.rel_distribution;

    rank              = i.rank;
    round             = i.round;

    if (i.Index()<0 && i.SelectType()!=select_map)
      ShowError("Object::Object(..3...) index<0, type=",
                ToStr((int)i.SelectType()));
  }

///////////////////////////////////////////////////////////////////////////////

void Object::InsertAndRemoveDuplicates(ObjectList& set,
					      int old)  {
  cout << "******* Object::InsertAndRemoveDuplicates() " << endl;
  // This _is_ still used by Query::GatherNewObjects() by CombineMapsVQ().
  // I think no place uses this nomore.  Should be obsoleted ???

  for (size_t i=old; i<set.size(); i++) {
    size_t p = i, q = i;
    double val = set[i].Value();
    for (int j=old-1; j>=0; j--) {
      if (val>set[j].Value())
	p = j;
      else if (q!=i)
	break;
      if (q==i && set[j].Match(set[i])) {
	val = set[j].AddValue(val);  // OBS! AddValue()
	p = q = j;
      }
    }
    set.move(q, p);
    if (q==i)
      old++;
    else {
      set.swap_objects(i, set.size()-1);
      set.erase(set.size()-1);
      i--;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

bool Object::CheckDuplicates(ObjectList& set)  {
  bool ret = true;
  for (size_t i=0; i<set.size(); i++)
    for (size_t j=i+1; j<set.size(); j++)
      if (set[j].Match(set[i])) {
	char tmp[1024];
	sprintf(tmp, "i=%d j=%d label=[%s]", (int)i, (int)j, set[j].LabelP());
	ShowError("Object::CheckDuplicates() found duplicate: ", tmp);
	ret = false;
      }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

string Object::DumpStr() const {
  stringstream aspectss;

  const map<string,aspect_data>& a = Aspects();
  int j = 0;
  /*
  cout << "size=" << a.size() << endl;
  cout << " beg=" << (void*)&*a.begin() << endl;
  cout << " end=" << (void*)&*a.end() << endl;
  */
  for (map<string,aspect_data>::const_iterator i = a.begin();
       i!=a.end(); i++, j++) {
    /*
    cout << " x size=" << a.size() << endl;
    cout << " x  beg=" << (void*)&*a.begin() << endl;
    cout << " x    i=" << (void*)&*i << endl;
    cout << " x  end=" << (void*)&*a.end() << endl;
    string f = i->first;
    double s = i->second.value;
    cout << "j=" << j << " f=\"" << f << "\" s=" << s << endl;
    */
    aspectss << " \"" << i->first << "\"=" << i->second.value;
    if (i->second.type != aspect_data::aspect_default)
      aspectss << "[type=" << i->second.typeStr() << "]";
    if (i->second.type == aspect_data::aspect_coordinates && 
	i->second.params.size() >= 2)
      aspectss << "[x=" << i->second.params[0] << ",y="
	       << i->second.params[1] << "]";
  }

  string aspectstr = aspectss.str();
  // aspectstr = "";

  return string("#") + ToStr(oi->index) + " <" + oi->label + "> (" + 
    ToStr(Value()) + ",b:"+ToStr(BelowValue())+",s:"+ToStr(SiblingValue())+
    ",a:"+ToStr(AboveValue())+")" + (not_really_seen?"[not really seen]":"")+
    aspectstr + " " + SelectTypeChar(sel_type);
}

  /////////////////////////////////////////////////////////////////////////////

  void Object::DumpList(const ObjectList& l, const string& pre,
                        const string& post, bool not_all) {
    const int ndec = 4;

    size_t nitems = l.size();
    if (not_all)
      for (size_t i=0; i<l.size(); i++)
        if (!l[i].Retained())
          nitems--;      

    if (pre!="")
      cout << pre << " " << nitems << (not_all?" retained only":"")  << endl;

    int nd = 1;
    const DataBase *db = nitems ? l[0].GetDataBase() : NULL;
    if (nitems && db)
      nd = db->Size();

    size_t ll = 8;
    int maxrank = -1, maxround = -1;
    for (size_t i=0; i<l.size(); i++)
      if (!not_all || l[i].Retained()) {
        if (l[i].Label().length()>ll)
          ll = l[i].Label().length();
        if (l[i].Rank()>maxrank)
          maxrank = l[i].Rank();
        if (l[i].Round()>maxround)
          maxround = l[i].Round();
      }

    int lx = (int)floor(log10((double)nd))+1;
    int li = (int)floor(log10(double(nitems?nitems:1)))+1;
    int lr = (int)floor(log10(double(maxrank>0?maxrank:99)))+1; // 99 ~ -1
    int lo = maxround==-1 ? 2 : maxround==0 ? 1 :
      (int)floor(log10(double(maxround)))+1;

    stringstream format;
    format << "%" << ndec+3 << "." << ndec << "f";
    string formats = format.str();
    string empty = string(" ")+string(ndec+2, '*');

    for (size_t i=0; i<nitems; i++)
      if (!not_all || l[i].Retained())
        cout << l[i].DumpString(i, ll, li, lx, lr, lo, formats, empty) << endl;

    if (post!="")
      cout << post << endl;
  }

  /////////////////////////////////////////////////////////////////////////////

  string Object::DumpString(int i, int ll, int li, int lx, int lr, int lo,
                            const string& fmt, const string& e) const {
    char tmp[1000] = "";
    if (i>=0)
      sprintf(tmp, "%*d ", li, i);

    sprintf(tmp+strlen(tmp), "%s %-*s %*d @%*d/%*d %c %10.6f %d %s",
            GetDataBase()->Name().c_str(), ll, LabelP(),
            lx, Index(), lr, Rank(), lo, Round(),
            SelectTypeChar(sel_type)[0], value, //genuine_relevance,
            retained, extra_str.c_str());

    stringstream ss;
    ss << tmp;

    for (int s=0; s<stagevalues.Nitems(); s++)
      ss << " ST#" << s+1 << ": " << StageInfoText(s, fmt, e);

    ss << " " << TargetTypeString(TargetType());

    ss << " aspects=[" << AspectNamesValues() << "]";

    return ss.str();
  }

  /////////////////////////////////////////////////////////////////////////////

  bool Object::AddToXMLstageinfo(XmlDom& xml) const {
    for (int s=0; s<stagevalues.Nitems(); s++) {
      stringstream ss;
      ss << "stage-" << s+1;
      xml.Element(ss.str(), StageInfoText(s, "", ""));
    }

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////////

  bool Object::AddToXMLstageinfo_detail(XmlDom& xml,
                                        const Query *q) const {
    XmlDom sil = xml.Element("stageinfolist");
    for (int s=0; s<stagevalues.Nitems(); s++)
      for (int f=0; f<stagevalues[s].Length()-1; f++)
        if (stagevalues[s][f]!=MAXFLOAT) {
          XmlDom si = sil.Element("stageinfo");
          si.Prop("stage",   s+1);
          si.Prop("feature", q->IndexFullName(f));
          si.Prop("score",   stagevalues[s][f]);
        }

    return true;
  }

///////////////////////////////////////////////////////////////////////////////

string Object::StageInfoText(int s, const string& fmt,
				     const string& e) const {
  if (!stagevalues.IndexOK(s))
    return "";

  const string format = fmt!="" ? fmt : "%7.3f";
  const string empty  = e!=""   ? e   : "  *****";

  string buf;

  for (int v=0; v<stagevalues[s].Length(); v++) {
    if (v)
      buf += " ";
    if (v==stagevalues[s].Length()-1)
      buf += "= ";

    if (stagevalues[s][v]!=MAXFLOAT) {
      char tmp[1000];
      sprintf(tmp, format.c_str(), stagevalues[s][v]);
      buf += tmp;

    } else
      buf += empty;
  }

  return buf;
}

///////////////////////////////////////////////////////////////////////////////

bool Object::Combine(const Object& img, bool allow_mix, float factor) {
  AddValue(img.Value(), factor);
  
  if (!allow_mix && (stagevalues.Nitems() || img.stagevalues.Nitems()) &&
      (stagevalues.Nitems()!=1 || img.stagevalues.Nitems()!=1)) {
    char txt[100];
    sprintf(txt, "%d %d", stagevalues.Nitems(), img.stagevalues.Nitems());
    return ShowError("Object::Combine() error #1a: ", txt);
  }

  if (allow_mix && stagevalues.Nitems()!=img.stagevalues.Nitems()) {
    char txt[100];
    sprintf(txt, "%d %d", stagevalues.Nitems(), img.stagevalues.Nitems());
    return ShowError("Object::Combine() error #1b: ", txt);
  }

  for (int j=0; j<stagevalues.Nitems(); j++) {
    FloatVector &dst = stagevalues[j];
    const FloatVector &src = img.stagevalues[j];

    if (dst.Length()!=src.Length() || dst.Length()<2) {
      char txt[100];
      sprintf(txt, "j=%d %d %d", j, dst.Length(), src.Length());
      return ShowError("Object::Combine() error #2: ", txt);
    }
  
    for (int i=0; i<dst.Length()-1; i++) {
      if (!allow_mix && dst[i]!=MAXFLOAT && src[i]!=MAXFLOAT) {
	cout << "this=" <<     DumpString(-1) << endl;
	cout << "img =" << img.DumpString(-1) << endl;
	char txt[100];
	sprintf(txt, "j=%d i=%d", j, i);
	return ShowError("Object::Combine() error #3: ", txt);
      }
      if (src[i]!=MAXFLOAT) {
	if (dst[i]==MAXFLOAT)
	  dst[i] = src[i];
	else
	  dst[i] += src[i];
      }
    }
    dst.Last() = value;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

/// Starts ticking.
void Object::Tic(const char *f) const { GetDataBase()->Tic(f); }
  
/// Stops tacking.
void Object::Tac(const char *f) const { GetDataBase()->Tac(f); }

// class Object ends here

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VQUnit::DumpList(const VQUnitList& l,
			      const string& pre, const string& post,
			      bool not_all, int mapw){
  size_t nitems = l.size();
  if (not_all)
    for (size_t i=0; i<l.size(); i++)
      if (!l[i].Retained())
	nitems--;

  if (pre!="")
    cout << pre << " " << nitems << (not_all?" retained only":"") << endl;

  for (size_t i=0; i<nitems; i++)
    if (!not_all || l[i].Retained()) {
      char tmp[100];
      if(mapw<1)
	sprintf(tmp, "%3d %d %d %8d %+10.7f %d",
		(int)i, l[i].map_number, l[i].map_level, l[i].index,
		l[i].value, l[i].retained);
      else
	sprintf(tmp, "%3d %d %d %8d(x=%d,y=%d)  %+10.7f %d",
		(int)i, l[i].map_number, l[i].map_level, l[i].index,
		l[i].index % mapw, l[i].index / mapw,
		l[i].value, l[i].retained);

      cout << tmp << endl;
    }

  if (post!="")
    cout << post << endl;
}

///////////////////////////////////////////////////////////////////////////////

// class VQUnit ends here

} // namespace picsom

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

using namespace picsom;

#ifdef __GNUC__
#include <ValuedList.C>
template void ValuedList<picsom::VQUnit>::add_and_sort(size_t,const picsom::VQUnit&);
template void ValuedList<picsom::Object>::add_and_sort(size_t,const picsom::Object&);
template bool ValuedList<picsom::Object>::will_be_added(size_t,double) const;
template void ValuedList<picsom::Object>::move(size_t, size_t);
template bool ValuedList<picsom::Object>::check(bool) const;
#endif // __GNUC__

#ifdef _MSC_VER
#include <ValuedList.C>
template class ValuedList<picsom::VQUnit>;
template class ValuedList<picsom::Object>;
#endif // _MSC_VER

#ifdef __alpha
#include <RemoteHost.h>
#include <Query.h>
#include <Source.C>
#include <List.C>
#endif // __alpha

#ifdef sgi
#include <RemoteHost.h>
#include <Query.h>
#include <Analysis.h>
#include <Source.C>
#include <List.C>
#endif // sgi

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
