// -*- C++ -*- 	$Id: MsgDate.h,v 1.10 2006/06/16 10:35:41 vvi Exp $
/**
   \file MsgDate.h

   \brief Declarations and definitions of class MsgDate
   
   MsgDate.h contains declarations and definitions of class the MsgDate, which
   is a class that performs the message date feature extraction for message
   files.
  
   \author Hannes Muurinen <hannes.muurinen@hut.fi>
   $Revision: 1.10 $
   $Date: 2006/06/16 10:35:41 $
   \bug May be some out there hiding.
   \warning Be warned against all odds!
   \todo 
*/
#ifndef _MsgDate_
#define _MsgDate_

#include <TextBased.h>
#include <math.h>
#include <time.h>

namespace picsom {
/// A class that performs the message date feature extraction.
class MsgDate : public TextBased {
 public:
  /// definition for the current data type
  typedef wchar_t datatype;  // this should be selectable in run time...
  
  /// definition for the current data vector type
  typedef vector<datatype> vectortype;

  /** Constructor
      \param obj initialise to this object
      \param opt list of options to initialise to
  */
  MsgDate(const string& obj = "", const list<string>& opt = (list<string>)0) {
    Initialize(obj, "", NULL, opt);
  }

  /// This constructor doesn't add an entry in the methods list.
  MsgDate(bool) : TextBased(false) { 
  }

  //~MsgDate() {}

  virtual Feature *Create(const string& s) const {
    return (new MsgDate())->SetModel(s);
  }

  virtual Feature::Data *CreateData(pixeltype, int n, int) const {
    //    return new MsgDateData(t, n,this);
    return new MsgDateData(DefaultPixelType(), n, this, TextData());
  }

  virtual void deleteData(void *p){
    delete (MsgDateData*)p;
  }

  virtual string Name()          const { return "msgdate"; }

  virtual string LongName()      const { return "message date"; }

  virtual string ShortText()     const {
    return 
      "A feature calculated using the Date field of an email message.";}

  virtual string Version() const;

  virtual string TargetType() const { return "message"; }

  virtual featureVector getRandomFeatureVector() const { return featureVector(); }

  //  virtual int printMethodOptions(ostream&) const;

  virtual treat_type DefaultWithinTreatment() const {
    return treat_concatenate; }

  virtual treat_type DefaultBetweenTreatment() const {
    return treat_separate; }

  virtual pixeltype DefaultPixelType() const { return pixel_undef; }

  virtual int FeatureVectorSize(bool) const {
    return 9;
  }

  /** Loads the data.
      \param datafile the name of the object file
      \param segmentfile the name of the segmentation file (default "")
      \return true if data was successfully loaded, otherwise false.
  */
  virtual bool LoadObjectData(const string& datafile, 
			      const string& segmentfile = "") { 
    // we can use only_original_text == 'true' here, because we know
    // that a valid message/rfc822 file doesn't contain any UTF8 characters:
    return LoadTextData(datafile, segmentfile, true);
  }

protected:  

  ///
  //  virtual bool ProcessOptionsAndRemove(list<string>&);

  /// The MsgDateData objects store the data needed for the feature calculation
  class MsgDateData : public TextBasedData {
  public:
    /** Constructor
	\param t sets the pixeltype of the data
	\param n sets the extension (jl?)
    */
    MsgDateData(pixeltype t, int n,const Feature *p,
		const textdata &txt)
      : TextBasedData(t, n, p, txt) {
    }

    /// Defined because we have virtual member functions...
    ~MsgDateData() {}

    /** Returns the name of the feature
	\return feature name
    */    
    virtual string Name() const { return "MsgDateData"; }

    /** Returns the lenght of the data contained in the object
	\return data length
    */
    virtual int Length() const { return 9; }

    /// += operator 
    virtual Data& operator+=(const Data &) {
      // this should check that typeof(d) == typeof(*this) !!!
      throw "MsgDateData::operator+=() not defined"; }

    /** Returns the result of the feature extraction
	\return feature result vector
    */
    virtual featureVector Result(bool) const {
      string message = TextData().as_string();

      string::size_type date_start = message.find("Date: ") + 6; 
      if(date_start == string::npos)
	return featureVector(Length());
      string::size_type date_end = message.find("\n", date_start);

      string datestr = message.substr(date_start, date_end-date_start);

      vector<float> datevec = DateStrToDateVector(datestr);
      if(datevec.size() != (int unsigned) Length())
	throw 
	 "MsgDateData::Result() : the file doesn't contain a valid Date field";
      return featureVector(datevec);
    }

  private:
    
    /// Converts an email date text field to a vector of floats.
    /// If all is ok, returns a 9-element vector. Returns empty vector on
    /// failure.
    vector<float> DateStrToDateVector(string date) const {
      vector<float> v;
      int day_of_week, day_of_month, month, year, hour, min, sec;
      string::size_type i;

      // extract the day string, and then remove it:
      date = StripLeadingWhitespace(date);
      i = date.find_first_not_of(
		 "abcdefghijklmnopqrstuvwxyzåaoABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ");
      day_of_week = DayStringToInt(date.substr(0,i));
      date.erase(0,i);
      if (day_of_week < 0)
	return v;

      // extract the day of month:
      if (!ExtractNextInteger(date,&day_of_month))
	return v;
      if (day_of_month < 1 || day_of_month > 31)
	return v;

      // extract the month string and convert it to int:
      date = StripLeadingWhitespace(date);
      i = date.find_first_not_of(
		 "abcdefghijklmnopqrstuvwxyzåaoABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ");
      month = MonthStringToInt(date.substr(0,i));
      date.erase(0,i);
      if (month < 0)
	return v;
      
      // extract the year:
      if (!ExtractNextInteger(date,&year))
	return v;

      // extract the time:
      if (!ExtractNextInteger(date,&hour) ||
	  !ExtractNextInteger(date,&min) ||
	  !ExtractNextInteger(date,&sec))
	return v;
      if (hour < 0 || hour > 23 || min < 0 || min > 59 || 
	  sec < 0 || sec > 60) // leap second..
	return v;

      // obs! timezone information not used!
 
      // The start of next month:
      int n_mon = month+1;
      int n_year = year;
      if (n_mon > 12) {
	n_mon -= 12;
	n_year++;
      }

      // Calculate some specific time instances:
      time_t time = GetTime(sec,min,hour,day_of_month,month,year);
      time_t time_in_1970 = GetTime(0,0,0,1,1,1970);
      time_t beg_of_year = GetTime(0,0,0,1,1,year);
      time_t beg_of_month = GetTime(0,0,0,1,month,year);
      time_t beg_of_day = GetTime(0,0,0,day_of_month,month,year);
      time_t next_year = GetTime(0,0,0,1,1,year+1);
      time_t next_month = GetTime(0,0,0,1,n_mon,n_year);

      // lengths of various time units (days, months, etc):
      float secs_per_day = 86400;
      float secs_per_week = 604800;
      float secs_per_month = difftime(next_month,beg_of_month);
      float secs_per_year = difftime(next_year,beg_of_year);

      // some time differences:
      float secs_since_1970 = difftime(time,time_in_1970);
      float secs_since_beginning_of_year = difftime(time, beg_of_year);
      float secs_since_beginning_of_month = difftime(time, beg_of_month);
      float secs_since_beginning_of_day = difftime(time, beg_of_day);
      float secs_since_beginning_of_week 
	= secs_since_beginning_of_day + day_of_week*secs_per_day;

      // what part of a time unit (day,month,etc) the given time is at 
      // (value between 0 and 1)
      float part_of_day = secs_since_beginning_of_day / secs_per_day;
      float part_of_week = secs_since_beginning_of_week / secs_per_week;
      float part_of_month = secs_since_beginning_of_month / secs_per_month;
      float part_of_year = secs_since_beginning_of_year / secs_per_year;
      
      // Populate the return vector.
      // Convert some of the values to cyclic polar coordinates.
      float x,y;
      v.push_back(secs_since_1970 / secs_per_day); // number of days sice 1970
      Polarize(part_of_year, &x, &y);
      v.push_back(x);
      v.push_back(y);
      Polarize(part_of_month, &x, &y);
      v.push_back(x);
      v.push_back(y);
      Polarize(part_of_week, &x, &y);
      v.push_back(x);
      v.push_back(y);
      Polarize(part_of_day, &x, &y);
      v.push_back(x);
      v.push_back(y);
      
      return v;
    }
    

    /// Converts n (0..1) to polar coordinates (x,y) using sin() and cos().
    /// Returns true on success, false on failure.
    bool Polarize(float n, float *x, float *y) const {
      if(n >= 0 && n <= 1) {
	*x = cos(n*M_PI*2);
	*y = sin(n*M_PI*2);
	return true;
      }
      else {
	*x = 0;
	*y = 0;
	return false;
      }
    }
    
    /// Returns a time_t value corresponding to the given time
    time_t GetTime(int sec, int min, int hour, 
		   int day, int month, int year) const {
      struct tm time;
      time.tm_sec = sec;
      time.tm_min = min;
      time.tm_hour = hour;
      time.tm_mday = day;
      time.tm_mon = month-1; // month must be in { 0, .. , 11 }
      time.tm_year = year > 1900 ? year-1900 : year;
      
      return mktime(&time);
    }
    
    /// returns the given string without any leading whitespace characters
    string StripLeadingWhitespace(string s) const {
      return s.erase(0, s.find_first_not_of(" \t\n"));
    }

    /// Stores the next integer in the given string to in. Removes the integer
    /// and all non-numerical characters before it from the string.
    bool ExtractNextInteger(string &s, int *in) const {
      string::size_type i = s.find_first_of("0123456789");
      if (i == string::npos)
	return false;
      string::size_type j = s.find_first_not_of("0123456789",i);
      string::size_type len;
      if (j == string::npos)
	len = string::npos;
      else
	len = j-i;
      *in = atoi(s.substr(i,len).c_str());
      s.erase(0,j);
      return true;
    }

    /// Converts a day of week string to int (Sunday == 0, Monday == 1, ...).
    /// Returns -1 if not a valid day string.
    int DayStringToInt(const string day) const {
      static struct day_number {
	int num;
	string str;
      } table[] = {
	{ 0, "Sun" },
	{ 1, "Mon" },
	{ 2, "Tue" },
	{ 3, "Wed" },
	{ 4, "Thu" },
	{ 5, "Fri" },
	{ 6, "Sat" },
	{ 0, "Su" },
	{ 1, "Ma" },
	{ 2, "Ti" },
	{ 3, "Ke" },
	{ 4, "To" },
	{ 5, "Pe" },
	{ 6, "La" },
	{ -1, "" },
      }; // This table MUST end with an element, which has num < 0

      for (const day_number *d = table; d->num >= 0; d++)
	if (day == d->str)
	  return d->num;

      return -1;
    }

    /// Converts a month string to month number (January == 1, ...).
    /// Returns -1 if the month string was invalid.
    int MonthStringToInt(const string mon) const {
      static struct month_number {
	int num;
	string str;
      } table[] = {
	{ 1,  "Jan" },
	{ 2,  "Feb" },
	{ 3,  "Mar" },
	{ 4,  "Apr" },
	{ 5,  "May" },
	{ 6,  "Jun" },
	{ 7,  "Jul" },
	{ 8,  "Aug" },
	{ 9,  "Sep" },
	{ 10, "Oct" },
	{ 11, "Nov" },
	{ 12, "Dec" },
	{ 1,  "Tammi" },
	{ 2,  "Helmi" },
	{ 3,  "Maalis" },
	{ 4,  "Huhti" },
	{ 5,  "Touko" },
	{ 6,  "Kesa" },
	{ 7,  "Heina" },
	{ 8,  "Elo" },
	{ 9,  "Syys" },
	{ 10, "Loka" },
	{ 11, "Marras" },
	{ 12, "Joulu" },
	{ -1, "" },
      }; // This table MUST end with an element, which has num <= 0

      for (const month_number *d = table; d->num > 0; d++)
	if (mon == d->str)
	  return d->num;

      return -1;
    }
    
  }; // class MsgDateData


}; // class MsgDate
}
#endif

// Local Variables:
// mode: font-lock
// End:
