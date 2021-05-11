// -*- C++ -*-  $Id: Valued.h,v 2.42 2020/01/01 18:46:54 jormal Exp $
// 
// Copyright 1998-2020 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#ifndef _PICSOM_VALUED_H_
#define _PICSOM_VALUED_H_

#include <object_info.h>
#include <aspects.h>

#include <List.h>

#include <string>
#include <vector>
#include <map>
using namespace std;

static const string Valued_h_vcid =
  "@(#)$Id: Valued.h,v 2.42 2020/01/01 18:46:54 jormal Exp $";

namespace picsom {
  using simple::FloatVector;
  using simple::FloatVectorSet;

  /**
     DOCUMENTATION MISSING
  */

  class VQUnit;
  class Object;
  template <class> class ValuedList;
  typedef ValuedList<Object> ObjectList;
  typedef ValuedList<VQUnit> VQUnitList;

  class Valued {
  public:
    ///
    Valued(double v) : value(v), retained(true) {}

    ///
    Valued(const Valued& v) : value(v.value),retained(v.retained) {}

    /// poison
    Valued() { abort(); }

    /// added 2020-01-01 and is a bit suspicious...
    Valued& operator=(const Valued&) = default;
    
    ///
    double Value() const { return value; }

    /// Sets the value.
    void Value(double v) { value = v; }

    ///
    double AddValue(float v, float a=1.0) { return value += a*v; }

    /// Sets retained flag to false.
    void Discard() { Retained(false); }

    /// Returns true if retained.
    bool Retained() const { return retained; }

    /// Sets retained flag.
    void Retained(bool d) { retained = d; }

    /// Bug compatibility mode.
    static void Pre707Bug(bool m) { pre707bug = m; }

    /// Bug compatibility mode.
    static bool Pre707Bug() { return pre707bug; }

  protected:
    ///
    double value;

    /// Initially true, set false when deselected.
    bool retained;

    ///
    static bool pre707bug;

  }; // class Valued

  /////////////////////////////////////////////////////////////////////////////

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  /** Class VQUnit.
  */

  class VQUnit : public Valued {
  public:
    ///
    VQUnit(int i, double v, int m, int l) : Valued(v), index(i),
					    map_number(m), map_level(l) {}

    ///
    VQUnit(const VQUnit& u) : Valued(u), index(u.index),
			      map_number(u.map_number),
			      map_level(u.map_level) {}

    /// poison
    VQUnit() { abort(); }

    /// The destructor.
    virtual ~VQUnit() {}

    /// Returns the stored index.
    int Index() const { return index; }

    /// Returns the stored map number.
    int MapNumber() const { return map_number; }

    /// Returns the stored map level.
    int MapLevel() const { return map_level; }

    ///
    static void DumpList(const VQUnitList&,
			 const string& = "", const string& = "", bool = false,
			 int mapw = -1);

  protected:
    ///
    int index;

    /// SHOULD BE GET RID OF:
    int map_number;

    /// SHOULD BE GET RID OF:
    int map_level;

  };  // class VQUnit

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /** Class for holding an object in the database.
  */

  class Object : public Valued {
  public:
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /// data structures to hold distribution of relevance among an object and 
  /// its children

  /// currently (temporarily?) stored in class Object

    class relevance_distribution {
    public:
      relevance_distribution() {}
      
      void dump() const { 
	cout << "rootval " << rootval << endl;
	for (size_t i=0; i<subind.size(); i++)
	  cout << "(" << subind[i] << "): " <<subval[i] << endl;
	cout << "/dump" << endl;
      }
      
      float rootval;
      vector<int> subind;
      vector<float> subval;
      
    };  // class relevance_distribution

    /// 
    Object(DataBase*, int, select_type, double = 0, const string& = "",
	   int = -1, const FloatVector* = NULL, 
	   map<string,aspect_data>* = NULL);

    ///
    Object(const Object&);

    /// poison
    Object() { abort(); }

    /// 
    virtual ~Object() {}

    /// added 2020-01-01 and is a bit suspicious...
    Object& operator=(const Object&) = default;
    
    ///
    string DumpStr() const;

    /// This is needed in Valued::AddToSorted().
    virtual Valued *MakeCopy() const { return new Object(*this); }

    /// Returns database from the underlying object_info.
    const DataBase *GetDataBase() const { return oi->db; }

    /// Returns index from the underlying object_info.
    int Index() const { return oi->index; }

    /// Returns label from the underlying object_info.
    const string& Label() const { return oi->label; }

    /// Returns label from the underlying object_info.
    const char *LabelP() const { return Label().c_str(); }

    /// Returns target_type from the underlying object_info.
    target_type TargetType() const { return oi->type; }

    /// Returns parent's index or -1 if itself root object.
    int Parent() const { return oi->default_parent(); }

    vector<int> Parents() const { return oi->parents; }

    /// Returns true if object has any parents
    bool hasParents() const { return !oi->parents.empty(); }

    /// Returns list of children's indices.
    const vector<int>& Children() const { return oi->children; }

    /// Returns true if object has any children
    bool hasChildren() const { return !oi->children.empty(); }

    ///
    select_type SelectType() const { return sel_type; }

    ///
    void SelectType(select_type t) { sel_type = t; }

    ///
    const string& Extra() const { return extra_str; }

    /// returns the rank of the object, ie. its positition in list of shown.
    int Rank() const { return rank; }

    /// Sets the rank.
    void Rank(int r) { rank = r; }

    /// returns the round of the object
    int Round() const { return round; }

    /// Sets the round.
    void Round(int r) { round = r; }

    /// True if Object objects share common object_info.
    bool Match(const Object& i) const { return oi==i.oi; }

    ///
    static bool CheckDuplicates(ObjectList&);

    ///
    static void InsertAndRemoveDuplicates(ObjectList&, int = 0);

    /// Performs insertion sort on images by their values.
    static void SortListByValue(ObjectList&);

    /// Performs insertion sort on images by their ranks.
    static void SortListByRank(ObjectList&);

    /// Dumps out a series of line each containing one Object.
    static void DumpList(const ObjectList&,
			 const string& = "", const string& = "", bool = false);

    /// Makes a dump of the Object in a string.
    string DumpString(int, int = 8, int = 5, int = 7, int = 3, int = 2,
		      const string& = "", const string& = "") const;

    /// returns true if value is negative/zero/positive like ch -/0/+.
    bool Matches(char ch) const {
      return (ch=='-'&&value<0) || (ch=='0'&&value==0) || (ch=='+'&&value>0);
    }

    void Set(char ch) {
      value = ch=='-'?-1 : ch=='0'?0 : 1;
    }

    /// Starts ticking.
    void Tic(const char *f) const;

    /// Stops tacking.
    void Tac(const char *f = NULL) const;

    /// Copies stageinfo.
    void CopyStageInfo(const Object& img) { stagevalues = img.stagevalues; }

    /// Stores stageinfo in XML.
    bool AddToXMLstageinfo(XmlDom&) const;

    /// Stores stageinfo in XML.
    bool AddToXMLstageinfo_detail(XmlDom&, const Query*) const;

    /// Sets value to given stage and map.
    void HandleStageInfo(int si, const FloatVector *vv) {
      if (si>=0 && vv) {
	FloatVector *sv = stagevalues.GetEvenNew(si);
	*sv = *vv;
      } }

    /// Writes all values of given stage in a string.
    string StageInfoText(int, const string&, const string&) const;

    /// Number of stages stored in stagevalues.
    int NstageInfo() const { return stagevalues.Nitems(); }

    /// Number of stages stored in stagevalues.
    const FloatVector& StageInfo(int i) const { return stagevalues[i]; }

    /// Multiplies all stagevalues.
    bool MultiplyStageInfo(float m) {
      for (int i=0; i<NstageInfo(); i++)
	stagevalues[i] *= m;
      return true;
    }

    /// Adds value and combines stageinfo.
    bool Combine(const Object&, bool, float = 1.0);
  
    /// Increment above_value (ie. value retrieved from above in the
    /// relevance sharing algorithm)
    void AboveValue(double val) { above_value += val; }

    /// Return above_value
    double AboveValue() const { return above_value; }

    /// Increment below_value (ie. value retrieved from below in the
    /// relevance sharing algorithm
    void BelowValue(double val, bool max=false) { 
      if (max) {
	if (val > below_value)
	  below_value = val;
      } else
	below_value += val; 
    }

    /// Return below_value
    double BelowValue() const { return below_value; }

    /// Increment sibling_value (ie. value retrieved from sibling in the
    /// relevance sharing algorithm
    void SiblingValue(double val) { sibling_value += val; }

    /// Return sibling_value
    double SiblingValue() const { return sibling_value; }

    void ZeroRelevanceCounters() { 
      below_value=0; above_value=0; sibling_value=0; 
    }

    void SumRelevanceCounters(bool set_to_one=false) { 
      AddValue(below_value+above_value+sibling_value);
      ZeroRelevanceCounters();

      if (set_to_one && value != 0 && value != -1 && value != 1)
	value = value/fabs(value);
    }

    /// Returns value of genuine_relevance flag.
    bool GenuineRelevance() const { return genuine_relevance; }

    /// Sets genuine_relevance flag.
    void GenuineRelevance(bool d) { genuine_relevance = d; }

    /// describes the contribution of subobjects
    /// to the relevance of an object

    relevance_distribution rel_distribution;

    /** Returns true if the named aspect exists. */
    bool HasAspectRelevance(const string& a) const {
      return aspect_relevance.HasAspectRelevance(a);
    }

    /** Returns the relevance of the given aspect, or 0 if the aspect 
     *  doesn't exist (obs! should this be some other value?) */
    double AspectRelevance(const string& a) const {
      return aspect_relevance.AspectRelevance(a);
    }

    /// Returns the aspect_data object of the given aspect
    aspect_data AspectData(const string& a) const {
      return aspect_relevance.AspectData(a);
    }

    /// Sets the relevance of the given aspect
    bool AspectRelevance(const string& a, double v, const string& t = "", 
			 const vector<string>& p = vector<string>()) {
      return aspect_relevance.AspectRelevance(a, v, t, p);
    }

    /// Returns the relevance of all the aspects
    map<string,aspect_data>& Aspects() {
      return aspect_relevance.Aspects();
    }

    /// Returns the relevance of all the aspects
    const map<string,aspect_data>& Aspects() const {
      return aspect_relevance.Aspects();
    }

    /// Sets the relevance of all the aspects.
    void Aspects(const map<string,aspect_data>& a) {
      aspect_relevance.Aspects(a);
    }

    /// Sets the relevance of all aspects to zero (checkbox not selected)
    void ClearAspectRelevance() {
      aspect_relevance.ClearAspectRelevance();
    }

    /// Removes all the aspects from the aspect relevance map
    void ClearAspects() {
      aspect_relevance.ClearAspects();
    }

    /// A comma-seprated list of aspect names.
    string AspectNames() const {
      return AspectNamesCommon(false);
    }
    
    /// A comma-seprated list of aspect names and values.
    string AspectNamesValues() const {
      return AspectNamesCommon(true);
    }
    /// A comma-seprated list of aspect names and values.
    string AspectNamesCommon(bool val) const {
      string ret;

      const map<string,aspect_data>& a = Aspects();
      for (map<string,aspect_data>::const_iterator i=a.begin();
	   i!=a.end(); i++)
	ret += string(ret==""?"":",")+"\""+i->first+"\""
	  +(val?"="+ToStr(i->second.value):"");

      return ret;
    }

  protected:
    /// 
    const object_info *oi;

    /// answer || question || map || show
    select_type sel_type;

    /// Extra text like "mapunit"
    string extra_str;

    /// Numerical record of CBIR stages.
    ListOf<FloatVector> stagevalues;

    /// relevance scores from different bottom level maps
    //vector<float> bottomscores;

    /// object's self-influence on the above listed scores
    //vector<float> selfinfluences;
    
    /// Indicates if relevance is genuine or inherited, true by default
    bool genuine_relevance;

    /// Value recently retrieved from above in the relevance sharing
    /// algorithm
    double above_value;

    /// Value recently retrieved from below in the relevance sharing
    /// algorithm
    double below_value;

    /// Value recently retrieved from sibling in the relevance sharing
    /// algorithm
    double sibling_value;

    /// The relevance information of the aspects of this object
    aspects aspect_relevance;

    /// Rank in the presentation order. Initialized to -1.
    int rank;

    /// Presentation round. Initialized to -1.
    int round;

  public:
    ///
    bool not_really_seen;

  };  // class Object

} // namespace picsom

#endif // _PICSOM_VALUED_H_

// Local Variables:
// mode: lazy-lock
// End:

