// -*- C++ -*-  $Id: segmentfile.h,v 1.121 2019/02/04 09:10:34 jormal Exp $
// 
// Copyright 1998-2019 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _segmentfile_h_
#define _segmentfile_h_

#include <Util.h>
#include <imagefile.h>

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <sstream>

#include <stdio.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

//using namespace picsom;

namespace picsom {

 class coord_float;

 class segmentfile;
 
  /// class to hold coordinate pairs
  class coord {
  public:
    /// x coordinate
    int x;
    /// y coordinate
    int y;

    /// default constructor
    coord(){x=y=0;};
    ///
    coord(int X,int Y){x=X;y=Y;}
    /// copy constructor
    coord(const coord &c){ x=c.x; y=c.y;}
    /// truncates floating point coordinates
    coord(const coord_float &c);
    /// 
    bool operator ==(const coord &c) const {return (x==c.x && y==c.y);} 
    ///
    bool operator !=(const coord &c) const {return (!(*this==c));} 
    /// orders according to y coord first, in case of ties according to x
    bool operator < (const coord &c) const {return (y<c.y||(y==c.y && x<c.x));}
    /// writes coord class to stream
    friend std::ostream &operator<<(std::ostream &out, const coord &c) {
      out << c.x << " " << c.y;
      return out;
    }
    
    /// reads coord class from stream
    friend std::istream& operator >> (std::istream& in, coord &c) {
      in >> c.x >> c.y;
      return in;
    }
  };
  
  /// class to contain floating point coordinate pairs
  class coord_float {
  public:
    ///
    float x;
    ///
    float y;

    /// default constructor
    coord_float(){x=y=0;};
    ///
    coord_float(float X,float Y){x=X;y=Y;}
    /// copy constructor
    coord_float(const coord_float &c){ x=c.x; y=c.y;}
    /// 
    coord_float(const coord &c){ x=c.x; y=c.y;}
    ///
    bool operator ==(const coord_float &c) const {return (x==c.x && y==c.y);} 
    ///
    bool operator !=(const coord_float &c) const {return (!(*this==c));} 
    /// orders according to y coord first, in case of ties according to x
    bool operator < (const coord_float &c) const {
      return (y<c.y || (y==c.y && x<c.x));}

    /// writes class to stream
    friend std::ostream &operator<<(std::ostream &out, const coord_float &c) {
      out << c.x << " " << c.y;
      return out;
    }
    
    /// reads class from stream
    friend std::istream& operator >> (std::istream& in, coord_float &c) {
      in >> c.x >> c.y;
      return in;
    }
  };

  ///
  inline coord::coord(const coord_float &c) { x=(int)c.x; y=(int)c.y;}

  ///
  class labeledCoord : public coord {
  public:
    labeledCoord(int x, int y, int l) : coord(x,y){ label=l;} 
    int label;
  };

  ///
  typedef std::vector<coord> coordList;

  ///
  std::ostream &operator<<(std::ostream &out, const coordList &l);
  
  /// helper function to read a line from a stream
  void ReadLine(std::istream& in, std::string &line);

  ///
  std::istream& operator >> (std::istream& in, coordList &l);

  void string2intvector(const std::string &s,std::vector<int> &v);
  void string2floatvector(const std::string &s,std::vector<float> &v);

			
  bool isintchar(char c);

  bool IsAffirmativeSeg(const std::string& str);

  ///
  std::string formVirtualSegmentLabel(const std::set<int>& s,
				      char delimiter = '+');

  ///
  std::string formVirtualSegmentLabel(const std::vector<int>& s,
				      char delimiter = '+');

  ///
  std::set<int> parseVirtualSegmentLabel(const std::string& s,
					 char delimiter = '+');

  ///
  typedef std::vector<coord_float> coordfList;

  ///
  class NR { // routines from Numerical recipes
  public:

    // conversion functions to NR datatypes

    static float *vectorPtrsFromData(float *d, int l, int u);
    static float **matrixPtrsFromData(float *d, 
				      int l1, int u1,int l2, int u2); 
    static float ***tensor3PtrsFromData(float *d, int l1, int u1,
					int l2, int u2, int l3, int u3);

    static void freeMatrixPtrs(float **m,int l1, int u1,int l2, int u2);
    static void freeTensor3Ptrs(float ***t,int l1, int u1,
				int l2, int u2, int l3, int u3);


    // FFT routines

    static void rlft3(float ***data, float **speq, unsigned long nn1, 
		      unsigned long nn2, unsigned long nn3, int isign);
    static void fourn(float data[], unsigned long nn[], int ndim, int isign);
  };

  ///
  class real2DFFTData{

  public:
    int N;
    float *data;
    float *extracoeff;

    void transform(int exp=1);
    void itransform(){ transform(-1);}


  };

  class RGBTriple {
  public:
    ///
    float r() const {return v[0];}
    
    ///
    void r(float f){v[0]=f;}

    ///
    float g() const {return v[1];}
    
    ///
    void g(float f) {v[1]=f;}

    ///
    float b() const {return v[2];}
    
    ///
    void b(float f) {v[2]=f;}

    ///
    RGBTriple() :v(3) {v[0]=v[1]=v[2]=0;}

    ///
    RGBTriple(const RGBTriple &o) : v(3){
      v[0]=o.v[0];
      v[1]=o.v[1];
      v[2]=o.v[2];
    };
    
    ///
    RGBTriple(float red,float green, float blue)
      : v(3){
      if(red>1 || green > 1 || blue>1 || red < 0 || green < 0 || blue < 0 )
	throw 
	  std::string("Segmentation::RGBTriple::RGBTriple : "
		      "attempt to initialise w/ value outside [0,1]");
      r(red);
      g(green);
      b(blue);
    }

    virtual ~RGBTriple() {}
    
    ///
    float SqrDist(const RGBTriple &o) {
      float dist=0, diff;
      diff=r()-o.r();
      dist += diff*diff;
      diff=g()-o.g();
      dist += diff*diff;
      diff=b()-o.b();
      dist += diff*diff;
      
      return dist;
    }

    bool operator < (const RGBTriple &o) const{
      if(r()!=o.r())
	return r()<o.r();
      else if(g()!=o.g())
	return g()<o.g();
      else return b()<o.b();
    }

    std::vector<float> v;

  };  // class RGBTriple


  struct ltRGB {
    bool operator()(const RGBTriple &t1, const RGBTriple &t2) const
    {
      return t1<t2;
    }
  };  // struct ltRGB

  

  class NamedRGB : public RGBTriple {
  public:
    ///
    NamedRGB(){
      throw std::string("NamedRGB default constructor called");
    }

    NamedRGB(const NamedRGB &o): RGBTriple(o){
      name=o.name;
    }
      
    NamedRGB(const std::string &s,float r,float g, float b) :
      RGBTriple(r,g,b), name(s) {}

    virtual ~NamedRGB(){}

    ///
    std::string name;

    ///
    static RGBTriple GetRGB(const std::string &name){
      if (name=="black")   return RGBTriple(0,0,0);
      if (name=="white")   return RGBTriple(1,1,1);
      if (name=="red")     return RGBTriple(1,0,0);
      if (name=="green")   return RGBTriple(0,1,0);
      if (name=="blue")    return RGBTriple(0,0,1);
      if (name=="cyan")    return RGBTriple(0,1,1);
      if (name=="magenta") return RGBTriple(1,0,1);
      if (name=="yellow")  return RGBTriple(1,1,0);

      throw string("named colour [")+name+"] not known";
     }

  };  // class NamedRGB


//  class PredefinedColours{ 
//   public:
//     ///
//     PredefinedColours();
//     std::vector<NamedRGB> v;
//   };

  class labeledList {
  public:
    coordList l;
    int label;
  };
  
  ///
  typedef std::vector<labeledList> labeledListList;

  class Draw {
  public:
    ///
    static bool LinePixelsToList(coord p1,coord p2, 
				 std::vector<coord_float> &list); 
    ///
    static bool LinePixelsToList(coord p1,coord p2, std::vector<coord> &list); 

    // integer version

    ///
    static bool IntegerPixelsToList(coord p1,coord p2,
				    std::vector<coord> &list,
				    bool flip=true);
    ///
    static int FreemanCodeFromPoints(coord &p1, coord &p2);
  };

  class SegmentationResult {
  public:
    ///
    SegmentationResult() : resultid(-1), methodid(-1), trange_start(-1),
			   trange_end(-1) {}

    ///
    SegmentationResult(const std::string& n, const std::string& t,
		       const std::string& v) :
      name(n), type(t), value(v),resultid(-1), methodid(-1), trange_start(-1),
      trange_end(-1) {}

    ///
    void clear() {
      name = type = value = "";
      methodid = resultid =-1;
    }

    ///
    string Str() const;

    ///
    xmlNodePtr toXML() const;

    ///
    bool parseXML(const xmlNodePtr resultnode);

    ///
    bool ExtractMemberValues();

    ///
    bool StoreMemberValues();

    ///
    friend std::ostream &operator<<(std::ostream &out,
				    const SegmentationResult &r);

    ///
    std::string name;

    ///
    std::string type;

    ///
    std::string value;

    ///
    int resultid;

    ///
    int methodid;

    ///
    float trange_start;

    ///
    float trange_end;

    ///
    string trange_text;

  }; // SegmentationResult

  class SegmentationAttribute{

  public:
    ///
    std::string key;

    ///
    std::string value;

    ///
    xmlNodePtr toXML() const;

    ///
    bool parseXML(const xmlNodePtr resultnode);
  };

  //
  class CorrespArray{ 
    public:
    std::vector<int> v;

      CorrespArray(int i=0): v(i)
      {
	int j;
	for(j=0;j<i;j++)
	  v[j]=j;
      }

      int SmallestCorresponding(int i){ 
	while( i != v[i]){ i = v[i];};
	return i;
      } 
    };

  ///
  typedef std::vector<SegmentationResult> SegmentationResultList;

  class Option {
  public:
    std::string option;
    std::vector<std::string> arglist;
    
    ///
    void reset(){
      option=""; arglist.clear();
    }
    
    ///
    void addArgument(const std::string &s){
      arglist.push_back(s);
    }
    
    xmlNodePtr produceXML() const;

    ///
    bool parseXML(const xmlNodePtr);
    
  };
  
  class Optionlist{
  public:
    ///
    std::vector<Option> list;

    ///
    xmlNodePtr produceXML() const;

    ///
    bool parseXML(const xmlNodePtr);

  };

  class FileInfo{
  public:
    ///
    FileInfo(){
      reset();
    }

    void reset(){
      imgname=imgurl=imgpageurl=imgformat=imgcolordepth=imgwidth=
	imgheight=imgfilesize=imgchecksum=imgdate="**undefined**";
    }

    ///
    FileInfo(const std::string &s1, const std::string &s2, 
	     const std::string &s3, const std::string &s4, 
	     const std::string &s5, const std::string &s6, 
	     const std::string &s7, const std::string &s8, 
	     const std::string &s9, const std::string &s10) :
      imgname(s1),imgurl(s2),imgpageurl(s3),imgformat(s4),imgcolordepth(s5),
      imgwidth(s6),imgheight(s7),imgfilesize(s8),imgchecksum(s9),
      imgdate(s10){}

    ///
    virtual ~FileInfo(){}

    ///
    xmlNodePtr outputXML() const;

    ///
    bool parseXML(const xmlNodePtr node);
    
    ///
    std::string imgname;
    ///
    std::string imgurl;
    ///
    std::string imgpageurl;
    ///
    std::string imgformat;
    ///
    std::string imgcolordepth;
    ///
    std::string imgwidth;
    ///
    std::string imgheight;
    ///
    std::string imgfilesize;
    ///
    std::string imgchecksum;
    ///
    std::string imgdate;
  };

  class ProcessInfo {
  public:
    ///
    ProcessInfo() {
      user = host = procdate = cwd = commandline = "**undefined**";
      framestep = 1;
    }

    ///
    ProcessInfo(const std::string &s1, const std::string &s2, 
		const std::string &s3, const std::string &s4,
		const std::string &s5, const Optionlist &o) :
      user(s1), host(s2),procdate(s3),cwd(s4),commandline(s5),options(o){}

    ///
    virtual ~ProcessInfo(){}

    ///
    xmlNodePtr outputXML() const;

    ///
    bool parseXML(const xmlNodePtr node);

    ///
    std::string user;
    ///
    std::string host;
    ///
    std::string procdate;
    ///
    std::string cwd;
    ///
    std::string commandline;
    ///
    Optionlist options;
    ///
    size_t framestep;

  }; // class ProcessInfo

  class MethodInfo{
  public:
    ///
    MethodInfo(){ abbreviation=fullname=description=version=
		    "**undefined**";}
    ///
    MethodInfo(const std::string &s1, const std::string &s2,
	       const std::string &s3, const std::string &s4,
	       const Optionlist &o) :
      abbreviation(s1),fullname(s2),description(s3),
      version(s4), options(o){}

    ///
    virtual ~MethodInfo(){}

    ///
    xmlNodePtr outputXML() const;

    ///
    bool parseXML(const xmlNodePtr node);


    ///
    std::string abbreviation;
    ///
    std::string fullname;
    ///
    std::string description;
    ///
    std::string version;
    ///
    Optionlist options;
  };



  class ChainedMethod{
  public:
    ///
    virtual MethodInfo getMethodInfo()const =0;
    ///
    virtual const ChainedMethod *getNextMethod()const =0 ;

    ///
    virtual ~ChainedMethod(){}
  };
  
  class MethodList{
  public:

    // these two vectors should be equal in length
    
    std::vector<MethodInfo> methodInfo;

    std::vector<const ChainedMethod *> methodPointers;


    /// this index should be kept always in sync 
    std::map<const ChainedMethod *,int> pointer2idx;
    

    void reset(){
      methodInfo.clear();
      methodPointers.clear();
      pointer2idx.clear();
    }

    xmlNodePtr outputXML() const;

    bool parseXML(const xmlNodePtr node);

    void appendList(const MethodList &other);

    void appendChain(const ChainedMethod *first);

    void appendSingleMethod(const ChainedMethod *method);

    int methodIndex(const ChainedMethod *m) const {
      return (pointer2idx.count(m))?pointer2idx.find(m)->second:-1;
    }

    std::string methodName(int idx) const { 
      const MethodInfo &m=methodInfo[idx];
      return m.fullname;
    }

    size_t count() const{ return methodInfo.size();}
  
    std::string collectChainedName() const;
  };

  class FrameSizeInfo {
  public:
    xmlNodePtr outputXML() const;
    bool parseXML(const xmlNodePtr node);
    
    int lastframe;
    int firstframe;
    int width;
    int height;
  };

  class SequenceInfo {
  public:
    bool frameNrOk(int f) const { return f>=0 && f<=lastframe;}
    coord frameSize(int f) const;
    void setFromImagedata(const imagedata& i);
    void setFromImagefile(imagefile &i);
    bool isCompatible(const SequenceInfo &o) const;
    xmlNodePtr outputXML() const;
    bool parseXML(const xmlNodePtr node);
    void reset(){lastframe=-1;sizeinfo.clear();}
    int nframes() const {return lastframe+1;}

    int lastframe;
    std::vector<FrameSizeInfo> sizeinfo;
  };

  class ProcessInfoProvider {
  public:
    ///
    virtual ProcessInfo getProcessInfo() const =0;

    ///
    virtual ~ProcessInfoProvider(){}
  };

  class ProcessInfoContainer : public ProcessInfoProvider, 
			       public ProcessInfo {
  public:
    ///
    virtual ProcessInfo getProcessInfo() const { return *this; }
  };

  class FileInfoProvider{
  public:
    ///
    virtual FileInfo getFileInfo() const=0;
    
    ///
    virtual ~FileInfoProvider(){}
  };

  class FileInfoContainer : public FileInfoProvider, public FileInfo {
  public:
    ///
    virtual FileInfo getFileInfo() const { return *this; }
  };

  class namespace_spec{

  public:
    struct version{
      ///
      int major;
      ///
      int minor;
    };
   
    ///
    static version required_version(){
      static version v;
      v.major=0;
      v.minor=64;
      return v;
    }
    
    ///
    static string segmentation_url(){
      static string url = "http://www.cis.hut.fi/picsom/segmentation/";
      return url;
    }
    
    ///
    static string segmentation_current_version_url(){
      return segmentation_url() + "0.64/";
    }

    ///
    static void split_url(const string &url, string &version_independent, version &v);
  
  };

  /// vvi
 class XMLTools {
 
  public: 
      /// function to make pointer cast for string comparison
    static int strcmpXML(const xmlChar *s1, const char *s2);

    /// for prettyprinting an XML document
    static char *XMLToString(xmlDocPtr doc,bool pretty=true);

    /// method for looking specified node in an XML tree
    static xmlNodePtr xmlNodeFindDescendant(xmlNodePtr parent,
					    std::vector<std::string> &path);

    /// overloaded version
    static xmlNodePtr xmlNodeFindDescendant(xmlNodePtr parent,
					    const char *path);
    ///
    static xmlNodePtr xmlNodeFindNextSibling(xmlNodePtr curr,
					    const char *name);
    /// returns root->segmentation
    static xmlNodePtr xmlDocGetSegmentation(xmlDocPtr doc);

    ///
    static std::string xmlNodeGetContent(xmlNodePtr node);

    ///
    static int xmlNodeGetInt(xmlNodePtr node);

    ///
    static void removeBlankNodes(xmlDocPtr doc);

    ///
    static void recursivelyRemoveBlanks(xmlNodePtr n);

    ///
    static void showXMLTree(xmlNodePtr start,const std::string &indent="");

    ///
    static void showXMLTree(xmlDocPtr d){
      showXMLTree(xmlDocGetRootElement(d));
    };

    /// 
    static void displayNodeInfo(xmlNodePtr n, const std::string &indent="");

    ///
    static bool  isFormatting(xmlNodePtr n);

    ///
    static  void dumpNode(xmlDocPtr doc,xmlNodePtr node);

  };

  class SimpleRandom { // not too good linear congruential 
                       // random number generator
  public: 
    SimpleRandom() {next=1;}

    unsigned rand(){ 
      next = next*1103515245 + 12345; 
      return (unsigned )(next/65536) % 32768; 
    }

    unsigned randMax(){ return 32768;}

    void srand(unsigned seed) { next=seed; }
    unsigned next;
  };


  class PredefinedTypes { 
    public:
    ///
    class Point{

    public:
      Point(){};
      ///
      Point(const std::string &s){parse(s); }

      ///
      void parse(const std::string &s);

      ///
      void write(std::string &s) const;

      int x;
      int y;
    };

    class Line{
    public:
      Line(){};
      ///
      Line(const std::string &s){parse(s); }

      ///
      void parse(const std::string &s);

      ///
      void write(std::string &s) const;

      int x1;
      int y1;
      int x2;
      int y2;
    };

    class FD {
    public:
    };
    
    class Box {
    public:
      ///
      Box(int a = 0, int b = 0, int c = 0, int d = 0) :
	ulx(a), uly(b), brx(c), bry(d) {}

      ///
      Box(const std::string &s) { parse(s); }
      
      ///
      void parse(const std::string &s);

      ///
      int width() const { return brx-ulx+1; }

      //
      int height() const { return bry-uly+1; }

      ///
      int area() const { return width()*height(); }

      ///
      void write(std::string &s) const;

      ///
      static Box findBoundingBox(const coordList &l);

      ///
      float distanceTo(const Box &o) const;

      ///
      int overlapWith(const Box &o) const;

      ///
      bool isInside(const coord &c) const{
	return c.x >= ulx && c.x <= brx && c.y >= uly && c.y <= bry;
      }

      void update(int x,int y){
	if(x<ulx) ulx=x;
	if(y<uly) uly=y;
	if(x>brx) brx=x;
	if(y>bry) bry=y;
      }

      void update(const Box &o){
	if(o.ulx<ulx) ulx=o.ulx;
	if(o.uly<uly) uly=o.uly;
	if(o.brx>brx) brx=o.brx;
	if(o.bry>bry) bry=o.bry;
      }

      void offset(const coord &c){
	ulx += c.x;
	brx += c.x;
	uly += c.y;
	bry += c.y;
      }

      int ulx;
      int uly;
      int brx;
      int bry;
    };
  
    class RotatedBox {
    public:
      RotatedBox(){};
      ///
      RotatedBox(const std::string &s){parse(s); }

      ///
      void parse(const std::string &s);

      ///
      void write(std::string &s) const;

      int l;
      int t;
      int w;
      int h;
      float theta;
      float xc;
      float yc;
    };

    class CoordList{

    public:
      CoordList(){};
   ///
      CoordList(const std::string &s){parse(s); }

      ///
      void parse(const std::string &s);

      ///
      void write(std::string &s) const;

      picsom::coordList l;
    };

    //class PixelChain{
    //  picsom::PixelChain l;
    //};

    class Region{
    public:
      Region(){};
      ///
      Region(const std::string &s){parse(s); }

      ///
      void parse(const std::string &s);

      ///
      void write(std::string &s) const;

      std::string id;
      std::string value; 
    };
    
    PredefinedTypes();

    ///
    std::vector<string> v;
  };

  /// list of coordinate pairs
  class PixelChain {
  public:
    /// list of coordinate pairs
    std::list<coord> l;
    
    ///
    PixelChain(){};
    /// copy constructor
    PixelChain(const PixelChain &p): l(p.l){};
    
    /// writes class to stream
    friend std::ostream &operator<<(std::ostream &out, const PixelChain &c);
    /// reads class from stream
    friend std::istream& operator >> (std::istream& in, PixelChain &c);

    xmlNodePtr AddToXML(xmlNodePtr parent) const;
    bool ParseXML(xmlNodePtr xmlnode);
  };
  
  /// region consisting of adjacent pixel with same segment label
  class SubSegment {
  public:
    /// this pixel belongs to region
    coord seed;
    /// list of outer and inner borders
    std::list<PixelChain> borders;
    
    ///
    SubSegment(){};
    ///
    SubSegment(const SubSegment &s) : seed(s.seed), borders(s.borders){};
    
    /// writes class to stream
    friend std::ostream &operator<<(std::ostream &out, const SubSegment &s);
    /// reads class from stream
    friend std::istream& operator >> (std::istream& in,SubSegment &s);

    std::string *OutputXML() const;
    xmlNodePtr AddToXML(xmlNodePtr parent) const;
    bool ParseXML(xmlNodePtr xmlnode);
  };

  /// collection nonadjacent subsegments with same label
  class Segment {
  public:
    /// common label
    int label;
    /// list of subsegments
    std::list<SubSegment> subsegments;
    
    ///
    Segment(){};
    /// copy constructor

    Segment(const Segment &s) 
      : label(s.label), subsegments(s.subsegments) {};

    /// writes class to stream
    friend std::ostream &operator<<(std::ostream &out, const Segment &s);
    /// reads class from stream
    friend std::istream& operator >> (std::istream& in,Segment &s);

    xmlNodePtr AddToXML(xmlNodePtr parent) const;
    bool ParseXML(xmlNodePtr xmlnode);
  };

  /// collection of all segments in image
  class Chains {
  public:
    /// list of all segments in image
    std::list<Segment> segments;
    ///
    Chains(){};
    ///
    Chains(Chains &){};

    /// writes class to stream
    friend std::ostream &operator<<(std::ostream &out, const Chains &c);
    /// reads class from stream
    friend std::istream& operator >> (std::istream& in,Chains &c);

    std::string *OutputXML() const;
    bool ParseXML(const std::string &str);
  };

  class preprocessResult{
  public:
     virtual ~preprocessResult(){};
  };

  class preprocessResult_SobelThresh: public preprocessResult{
  public:
    preprocessResult_SobelThresh(){
      d0=d45=d90=d135=NULL;
    }
    
    virtual ~preprocessResult_SobelThresh(){
      delete[] d0;
      delete[] d45;
      delete[] d90;
      delete[] d135;
    }

    int *d0,*d45,*d90,*d135;
  };

  class preprocessResult_SobelMax: public preprocessResult{
  public:
    preprocessResult_SobelMax(){
      d=NULL;
    }
    
    virtual ~preprocessResult_SobelMax(){
      delete[] d;
    }

    float *d;
  };

  class preprocessResult_Sobel: public preprocessResult{
  public:
    preprocessResult_Sobel(){
      ds0=ds45=ds90=ds135=NULL;
      di0=di45=di90=di135=NULL;
    }
    
    virtual ~preprocessResult_Sobel(){
      delete[] ds0;
      delete[] ds45;
      delete[] ds90;
      delete[] ds135;
      delete[] di0;
      delete[] di45;
      delete[] di90;
      delete[] di135;
    }

    float *ds0,*ds45,*ds90,*ds135,*di0,*di45,*di90,*di135;
  };

  class preprocessResult_Cielab: public preprocessResult{
  public:
    preprocessResult_Cielab(){
      l=a=b=NULL;
    }
    
    virtual ~preprocessResult_Cielab(){
      delete[] l;
      delete[] a;
      delete[] b;
    }

    float *l;
    float *a;
    float *b;
  };

class preprocessResult_Cieluv: public preprocessResult{
  public:
    preprocessResult_Cieluv(){
      l=u=v=NULL;
    }
    
    virtual ~preprocessResult_Cieluv(){
      delete[] l;
      delete[] u;
      delete[] v;
    }

    float *l;
    float *u;
    float *v;
  };


class preprocessResult_HueMinMax: public preprocessResult{
  public:
    preprocessResult_HueMinMax(){
      hue=min=max=NULL;
    }
    
    virtual ~preprocessResult_HueMinMax(){
      delete[] hue;
      delete[] min;
      delete[] max;
    }

    float *hue;
    float *min;
    float *max;
  };

class preprocessResult_HueDiffSumQuant256: public preprocessResult{
  public:
    preprocessResult_HueDiffSumQuant256(){
      q=NULL;
    }
    
    virtual ~preprocessResult_HueDiffSumQuant256(){
      delete[] q;
    }

    int *q;
  };


  class Region {
  public:
    // default constructor
    Region(){}
    
    Region(bool) {
      next_type = list_of_types;
      list_of_types = this;
    }

    virtual ~Region() {}

    virtual bool isA(const string&) const { return false; }
    virtual coordList listCoordinates() const = 0;
    virtual const coordList *accessList() const { return NULL; }
    virtual Region *parseAndCreate(const string&) const { return NULL; }

    static Region *createRegion(const string& s) {
      const Region *p;
      for (p=list_of_types; p; p=p->next_type) {
	Region *r = p->parseAndCreate(s);
	if (r)
	  return r;
      }
      return NULL;
    }

    static Region *list_of_types;
    Region *next_type;

    virtual bool contains(int, int) const = 0;
    virtual size_t size() const = 0;
    virtual coord middlePoint() {
      throw string("Region::middlePoint() not implemented");
    }

    virtual operator string() const { return "baseclass Region"; }
  };  // class Region
 
  class rectangularRegion : public Region {
  public:
    /// The default constructor.
    rectangularRegion() : is_set(false) {}

    /// constructor for making list entry
    rectangularRegion(bool b):Region(b){};

    /** The standard constructor.
	\param p a string designating the region description
	\param s a pointer to segmentation data
    */
    rectangularRegion(const string& d,
		      const segmentfile *s = NULL) throw(string);

    virtual Region *parseAndCreate(const string &s) const {
      if(s.find("rect:")!=0)
	return NULL;
      else
	{
	  Region *ret= new rectangularRegion(string(s.c_str()+5));
	  return ret;
	}
    }

    /// Defined because we have virtual member functions...
    virtual ~rectangularRegion() {}

    /** Used by standard constructor for point-centerd regions.
	\param p a string of the form "(x0,y0):wxh", where (x0,y0) is the 
	centre of the region, and w is the width and h the height.
	\param s a pointer to segmentation data. In this case
	(x0,y0) is replaced by name of point-type segmentation result.
    */
    bool SolvePoint(const string& d, const segmentfile *s) throw(string);

    /** Used by standard constructor for box regions.
	\param p a string of the form "ulx,uly,brx,bry", where the 
	coordinates of the region are given.
	\param s a pointer to segmentation data. In this case
	ulx,uly,brx,bry is replaced by name of box-type segmentation result.
    */
    bool SolveBox(const string& d, const segmentfile *s) throw(string);

    /// definitions for virtual functions

    virtual coordList listCoordinates() const{
      coordList ret;

      int i,j;
      for(j=0;j<h;j++) for(i=0;i<w;i++){
	ret.push_back(coord(ulx+i,uly+j));
      }
      
      return ret;

    }

    virtual bool isA(const string &t) const{
      if(t=="rectangular") return true;
      return Region::isA(t);
    }

    virtual bool contains(int x, int y) const {
      return x>=ulx && x<ulx+w && y>=uly && y<uly+h;
    }

    /** Returns the constructor string (x0,y0) wxh"
	\return the constructor string
    */

    virtual operator string() const { return str; }

    /** Returns the size of the region (width * height)
	\return region size, zero if it hasn't been set yet
    */
    virtual size_t size() const { return is_set?w*h:0; }

    /** Returns the width of the region
	\return region width
    */
    int width() const { return is_set?w:0; }

    /** Returns the height of the region
	\return region height
    */
    int height() const { return is_set?h:0; }

    /** Determines (x,y) inside the region that are translated i steps
	from the upper left corner of the region. Each step is taken
	to the right, and starts from the beginning of the next row
	if it comes against the region border.

	\param i the number of steps
	\param x the new x coordinate
	\param y the new y coordinate
    */
    void xy(int i, int& x, int& y) const {
      if (!is_set)
	throw "rectangularRegion not set";
      x = ulx+i%w;
      y = uly+i/w;
    }

    /** Sets the centre of the region
	\param x the new centre x coordinate
	\param y the new centre y coordinate
    */
    void x0y0(int& x, int& y) const { x = x0; y = y0; }

    /** Moves the region by +dx, +dy
	\param dx the horisontal translation
	\param dy the vertical translation
    */
    void translate(int dx, int dy) {
      ulx += dx; uly += dy; x0 += dx; y0 += dy; }

    /** Implementation of the << operator
	\param os the output stream
	\param r the regioninfo to output
    */
    friend ostream& operator<<(ostream& os, const rectangularRegion& r) {
      os << "(" << r.x0 << "," << r.y0 << ")[" << r.w << "x" << r.h << "]";
      return os;
    }

    /** Dumps internal values in a string.
     */
    string dumpstr() const {
      stringstream str;
      str << "ulx=" << ulx << " uly=" << uly
	  << " x0=" << x0 << " y0=" << y0
	  << " w=" << w << " h=" << h;
      return str.str();
    }

    /** documentation missing ??
     */
    void TranslateByScaling(const scalinginfo &si) {
      int x0, y0;
      x0y0(x0, y0);
      int x = x0, y = y0;
      si.forwards(x, y);
      translate(x-x0, y-y0);
    }

    virtual coord middlePoint(){ return coord(x0,y0);}

  private:
    /// boolean that is true if the parameters of the region are set
    bool is_set;

    /// the regioninfo string (x0,y0):wxh or ulx,uly,brx,bry
    string str;

    /// the centre of the region
    int x0, y0;

    /// the upper left corner of the region
    int ulx, uly;
    
    /// width and height of the region
    int w, h;

  };  // class rectangularRegion


  class listRegion : public Region {
  public:

    /// definitions for virtual functions

    listRegion(bool b):Region(b){ coordset=NULL;}

    listRegion(const coordList &list) : l(list){
      coordList::const_iterator it;
      coordset=new std::set<coord>;
      for(it=l.begin();it!=l.end();it++)
	coordset->insert(*it);
    };

    // constructor for efficient interfacing with segmentation

    listRegion(std::vector<coordList *> &v,
	       bool allocate_set=false) {

      for(size_t i=0;i<v.size();i++)
	for(size_t j=0;j<v[i]->size();j++)
	  l.push_back((*v[i])[j]);

	//	l.insert(v[i]->end(),v[i]->begin(),v[i]->end());

      if(allocate_set){
	coordList::const_iterator it;
	coordset=new std::set<coord>;
	for(it=l.begin();it!=l.end();it++)
	  coordset->insert(*it);
      }
      else coordset=NULL;
    };
    
    /// Defined because we have virtual member functions...
    virtual ~listRegion() { delete coordset;}

    virtual coordList listCoordinates() const{
      return l;
    }

    virtual const coordList *accessList() const{ return &l;}

    virtual bool isA(const string &t) const{
      if(t=="listregion") return true;
      return Region::isA(t);
    }

    virtual bool contains(int x, int y) const { 
      if(coordset)
	return (coordset->count(coord(x,y)) != 0);
      else throw string("ListRegion: coordset==NULL");
    }

    virtual size_t size() const { return l.size();} 

    virtual Region *parseAndCreate(const string &) const {
      // yet to be implemented
      return NULL;
    }

    virtual operator string() const { 
      return "listRegion::operator string() not yet implemented.";
    }

  private:
    coordList l;
    std::set<coord> *coordset;

  };  // class listRegion


 class circularRegion : public Region {
  public:

    /// definitions for virtual functions

    circularRegion(bool b):Region(b){}

   circularRegion(float xx,float yy, float rr) : x(xx),y(yy),r(rr){
    };

    // constructor for efficient interfacing with segmentation

    /// Defined because we have virtual member functions...
    virtual ~circularRegion() {}

    virtual coordList listCoordinates() const{

      if(l.empty()){

	coordList ret;

      // only approximate if the middle point is not integer

	float r2=r*r;
	for(int xx=1;xx<r;xx++){
	  int x2=xx*xx;
	  for(int yy=1;yy<r;yy++)
	    if(x2+yy*yy<=r2){
	      ret.push_back(coord((int)x+xx,(int)y+yy));
	      ret.push_back(coord((int)x-xx,(int)y+yy));
	      ret.push_back(coord((int)x+xx,(int)y-yy));
	      ret.push_back(coord((int)x-xx,(int)y-yy));
	    }
	}
	for(int xx=1;xx<=r;xx++){
	  ret.push_back(coord((int)x+xx,(int)y));
	  ret.push_back(coord((int)x-xx,(int)y));
	  ret.push_back(coord((int)x,(int)y+xx));
	  ret.push_back(coord((int)x,(int)y-xx));
	}

	ret.push_back(coord((int)x,(int)y));

	return ret;
      }
	
      return l;
    }

   virtual coordList listCoordinates() {

      if(l.empty()){

      // only approximate if the middle point is not integer

	float r2=r*r;
	for(int xx=1;xx<r;xx++){
	  int x2=xx*xx;
	  for(int yy=1;yy<r;yy++)
	    if(x2+yy*yy<=r2){
	      l.push_back(coord((int)x+xx,(int)y+yy));
	      l.push_back(coord((int)x-xx,(int)y+yy));
	      l.push_back(coord((int)x+xx,(int)y-yy));
	      l.push_back(coord((int)x-xx,(int)y-yy));
	    }
	}
	for(int xx=1;xx<=r;xx++){
	  l.push_back(coord((int)x+xx,(int)y));
	  l.push_back(coord((int)x-xx,(int)y));
	  l.push_back(coord((int)x,(int)y+xx));
	  l.push_back(coord((int)x,(int)y-xx));
	}

	l.push_back(coord((int)x,(int)y));
      }
	
      return l;
    }


    virtual const coordList *accessList() const{ 
      if(l.empty()) listCoordinates();
      return &l;
    }

    virtual bool isA(const string &t) const{
      if(t=="circularregion") return true;
      return Region::isA(t);
    }

    virtual bool contains(int x, int y) const { 
      float dx=x-this->x;
      float dy=y-this->y;
      return (dx*dx+dy*dy<=r*r) ? true : false;
    }

    virtual size_t size() const { 
      if(l.empty()) listCoordinates();
      return l.size();
    } 

    virtual Region *parseAndCreate(const string &) const {
      // yet to be implemented
      return NULL;
    }

    virtual operator string() const { 
      return "circularRegion::operator string() not yet implemented.";
    }

    virtual coord middlePoint(){ return coord((int)x,(int)y);}

  private:
   coordList l;

   float x;
   float y;
   float r;
   
 };  // class circularRegion

  class ipList {
  public:
    void parseFromXML(segmentfile &sf,const string &name="");
    void parseFromXMLNode(xmlNodePtr n);
    void writeToXML(const ChainedMethod *method,segmentfile &sf,
		    const string &name);
    void setHeaderAndParse(const string &h);
    
    void reset();
    
    vector<vector<float> > list;
    map<string,size_t> fieldname2idx;
    map<size_t,string> idx2fieldname;
    
  private:
    string headerstring;
  };
  
  class pairingPredicate {
  public:
    bool isPair(const string &l1, const string &l2){
      return p.count(l1)>0 && p[l1].count(l2)>0;
    }
    map<string,set<string> > p; 
  };

  class _toc_entry{
  public:
    ///
    std::string sequence_name;

    ///
    int tiff_framenr;

    ///
    int tiff_msframenr;

    ///
    int sequence_framenr;
    };

  ///
  typedef std::vector<_toc_entry> _toc_type;

  /// vvi
  class segmentfile { 
  public:
    enum segFileType {empty,xml,tiff};
  
    enum frameSourceType {no_source,file,create
			  //,create_from_model
    };

    enum result_search_type {all,latest_method,last};

    /// vvi
    class _frame_type {
    public:
      ///
      _frame_type(): file(NULL),owndata(false),data(NULL),framenr(-1),
		     fileframe(-1),
		     fileframe_ms(-1),createwidth(-1),createheight(-1)
		      {}
      
      ///
      frameSourceType frameSrc;

      ///
      imagefile *file;

      ///
      bool owndata;

      ///
      imagedata *data;

      ////
      int framenr;  

      ///
      int fileframe;

      ///
      int fileframe_ms;

      ///
      int createwidth;

      ///
      int createheight;

      ///
      //      std::string modelsequence;

      ///
      bool segment;
      
      bool doesFrameNeed32Bits() const { 
	return segment; }
      // this could be optimised later

    };

    class image_sequence{ 

    public:

      ///
      image_sequence(): lastcachedframe(-1) {}

      ///
      void free(){
	for (size_t f=0; f<frames.size(); f++) {
	  if (frames[f].owndata) {
	    delete frames[f].data;
	    frames[f].data=NULL;
	  }

	}
      }

      void freeFrame(int f){
	if (f >= 0 && frames[f].owndata) {
	  delete frames[f].data;
	  frames[f].data=NULL;
	}
      }
      
      std::vector<_frame_type> frames;
      int lastcachedframe;
    };

    ///
    //    typedef std::map<std::string,image_sequence>  _stored_results_type;

    //    class _on_demand_sequence{
    //    public:
    //  std::vector<bool> onDemandVec;
    //  std::vector<frameSourceType> src;
    //};


   //  class _on_demand_type{
//     public:

//       _on_demand_type():imageptr(NULL),segmentptr(NULL) {}
      
//       void reset(){
// 	imageptr=segmentptr=NULL; 
// 	seqs.clear();
//       }
	
//       void initSequence(const string &n,int frames, frameSourceType srcType,
// 			bool loaded=false){
// 	_on_demand_sequence tmp;
// 	tmp.src=std::vector<frameSourceType>(frames,srcType);
// 	tmp.onDemandVec=std::vector<bool>(frames,loaded);
// 	seqs[n]=tmp;

//    	// cout <<"Initialising sequence " << n<< " src=";
//  	switch(srcType){
//  	case no_source:
//  	  // cout << "no_source";
//  	  break;
//  	case file:
//  	  // cout << "file";
//  	  break;
//  	case create:
//  	  // cout << "create";
//  	  break;
// 	  //case create_from_model:
// 	  // cout << "create_from_model";
// 	  //break;
//  	default:
//  	  ; // cout << "UNKNOWN";
//  	}
//  	// cout << endl;

// 	if(n=="image")
// 	  imageptr=&(seqs[n]);
// 	else if(n=="segment")
// 	  segmentptr=&(seqs[n]);

// 	//cout<<"imageptr="<<(void*)imageptr<<endl;
//       }


//       std::vector<bool> *getVec(const std::string &n){
// 	return &(seqs.find(n)->second.onDemandVec);
//       }

//       _on_demand_sequence *getSequence(const std::string &n){
// 	return &(seqs.find(n)->second);
//       }

//       std::vector<bool> *getImgVec(){return &(imageptr->onDemandVec);}
//       std::vector<bool> *getSegVec(){return &(segmentptr->onDemandVec);}

//       _on_demand_sequence *getImgSeq(){ return imageptr;}
//       _on_demand_sequence *getSegSeq(){ return segmentptr;}

//       bool checkName(const std::string &n){
// 	return (seqs.count(n) != 0);
//       }

//     private:
//       map<std::string,_on_demand_sequence> seqs;
//       // shortcuts for two default seqs
//       _on_demand_sequence *imageptr;
//       _on_demand_sequence *segmentptr;

//     };

    ///
    class _filenaming {
    public:
      ///
      std::string input_pattern;
      
      ///
      std::string output_pattern;

      ///
      std::string image_name;
      
      /// 
      std::string in_segment_name;
      
      /// 
      std::string out_segment_name;

    };


  //   class _nodeinfo_type{
//     public:

//       _nodeinfo_type(){ frame=methodresultslist=NULL; };

//       xmlNodePtr frame;

//       xmlNodePtr methodresultslist;

//     };

    ///
    static const string& version() {
      static string v =
	"$Id: segmentfile.h,v 1.121 2019/02/04 09:10:34 jormal Exp $";
      return v;
    }

    ///
    static const string& impl_version();

    ///
    static std::string name_head(){ return "segmentfile::";}

    ///
    static imagedata::pixeldatatype _getSegmentframeDatatype(){
      return imagedata::pixeldata_uint32;
    }

    ///
    segmentfile(const std::string &in_image="",
		const std::string &in_segment="",
		const ChainedMethod *firstmethod=NULL,
		const FileInfoProvider *fip=NULL,
		const ProcessInfoProvider *pip=NULL,
		const bool read_in_image=true,
		bool ondemand=true,
		bool cacheing=true) {
      first=firstmethod;
      fileInfoProvider=fip;
      processInfoProvider=pip;
      init();
      use_ondemand=ondemand;
      if(ondemand==false)
	throw string("segmentfile::segmentfile(): ondemand==false requested.");

      use_cacheing = cacheing;
      openFiles(in_image,in_segment,read_in_image,ondemand,cacheing);
    }

    ///
    ~segmentfile(){
      _free_image_sequences();
      delete input_image_file;
      delete input_segment_file;
      //delete input_description;
      // xmlFreeDoc(xml_description);
      discardAllPreprocessResults();
    }

    ///
    void init();

    ///
    void openFiles(const std::string &in_image_name ="",
		   const std::string &in_segment_name="",
		   const bool read_in_image = true,bool on_demand=true,
		   bool cacheing=true);

    ///
    typedef unsigned char *Matrix;

    ///
    coordList *convertToList(const Matrix &matrix,
			   const std::vector<int> *labels=NULL);
   
    string format() const {
      return input_image_file ? input_image_file->format() : "";
    }

    ///
    double videoFPS() const {
      return input_image_file ? input_image_file->video_fps() : 0.0 ;
    }

    ///
    int getNumFrames() const {
      return description.sequenceinfo.nframes();
    }

    ///
    bool isPrepared() const { return _is_prepared; }

    ///
    void setPrepared(bool p=true) { _is_prepared = p; }

    ///
    int getCurrentFrame() const { return current_frame; }

    ///
    void setCurrentFrame(int f) {
      ensureFrameOk(f);
      current_frame = f;
    }

    ///
    bool isFrameNrOk() const { return isFrameNrOk(current_frame);}

    ///
    bool isFrameNrOk(int f) const { return f>=0 && f<getNumFrames(); }

    ///
    bool isFrameOk() const { return isFrameOk(current_frame); }

    ///
    bool isFrameOk(int f) const { return isFrameNrOk(f); }
					  // && image_frames.frames[f].data
					  //&& segment_frames.frames[f].data); }
    ///
    void ensureFrameOk() const { ensureFrameOk(getCurrentFrame());}

    ///
    void ensureFrameOk(int f) const {
      if (!isFrameOk(f)) {
	char nrstr[20];
	sprintf(nrstr,"%d",f);
	throw name_head() + " ensureFrameOk("+nrstr+"): invalid frame";
      }
    }

    ///
    void ensureFrameNrOk() const { ensureFrameOk(getCurrentFrame());}
  
    ///
    void ensureFrameNrOk(int f) const {
      if (!isFrameNrOk(f))
	throw name_head() + " : invalid frame number";
    }

    ///
    void setFrame(int f) {
      if (!isFrameOk(f))
	throw name_head() + "setFrame() : illegal frame number";
      current_frame = f;
    }

    void advanceFrame() {
      if (!isFrameOk(current_frame+1))
	throw name_head() + "advanceFrame() : current frame beyond bounds";
      current_frame++;
    }

    /// Secret access to the underlying data...
    imagefile *inputImageFile() const { return input_image_file; }

    /// public interface to on-demand frame loading

    imagedata *imageFrame(int f) { 
      //cout << "now in imageFrame("<<f<<")"<<endl;
      //cout << "vec returned is"<<(void*)onDemand.getImgVec()<<endl;
      //if((*onDemand.getImgVec())[f]) 
      //	cout << "already fetched" << endl;
      //else
      //	cout << "fetching needed" << endl;

      return image_frames.frames[f].data != NULL ?
	image_frames.frames[f].data : fetchImageFrame(f);
    } 

    ///
    imagedata *segmentFrame(int f) { 
      return segment_frames.frames[f].data != NULL ?
	segment_frames.frames[f].data : fetchSegmentFrame(f);
    } 

    ///
    imagedata *storedImage(int id) {
      if (stored_images.count(id) == 0)
	throw string("stored image") + " not found.";

      return stored_images[id].data != NULL ? 
	stored_images[id].data : fetchStoredImage(id);
    } 

    ///
    imagedata *storedSegment(int id) {
      if (stored_segments.count(id) == 0)
	throw string("stored segment") + " not found.";

      return stored_segments[id].data != NULL ? 
	stored_segments[id].data : fetchStoredSegment(id);
    } 

    ///
    void discardImageFrame(int f){
      _discard_frame(image_frames.frames[f]);
    }

    ///
    void discardAllImages();

    ///
    void discardSegmentFrame(int f){
      _discard_frame(segment_frames.frames[f]);
    }

    ///
    void discardAllSegments();

    ///
    void discardStoredImage(int id) {
      if (stored_images.count(id) == 0)
	throw string("stored image") + " not found.";
      _discard_frame(stored_images[id]);
    }

    void discardStoredSegment(int id) {
      if (stored_segments.count(id) == 0)
	throw string("stored segment") + " not found.";
      _discard_frame(stored_segments[id]);
    }

    ///
    void moveSegmentationMask(int from, int to, bool discard=true);

    ///
    int getWidth();

    ///
    int getHeight();

    ///
    bool coordinates_ok(int x, int y) {
      if (getNumFrames()==0) return false;
      return imageFrame(getCurrentFrame())->coordinates_ok(x,y);
    }

    ///
    double getMaxValue() const { return 1.0;}

    ///
    double getMinValue() const {return 0;}

    ///
    const std::string &getImageFileName() const;

    ///
    const std::string &getSegmentFileName() const;

    ///
    void readImageFile(const char*,bool,bool);
    
    ///
    void readSegmentFile(const string&, bool); // , bool updateDescription=true);

    ///
    void writeAllImageFrames(const string&);

    ///
    void writeImageFrame(const char *,int f=-1,const char* mimetype=NULL);

    ///
    void writeAllSegmentFrames(const char*,bool includeImages=false,
			       bool includeStoredResults=true,
			       segFileType forceFileType=empty);
    ///
    void writeSegmentFrame(const char*,int f=-1,bool includeImages=false, 
			   bool includeStoredResults=true, 
			   segFileType forceFileType=empty) ;
    
    ///
    void allocateImages();

    ///
    void allocateSegments();

    ///
    void forceImages(const std::vector<imagedata>&);

    ///
    void forceSegments(const std::vector<imagedata>&);

    ///    
    void forceImage(const imagedata&, int);

    ///
    void forceImage(const imagedata &i) { forceImage(i, getCurrentFrame()); }

    ///
    void forceSegment(const imagedata&, int);

    ///
    void forceSegment(const imagedata &i){ forceSegment(i, getCurrentFrame());}

    ///
    void rotateFrame(int f, const scalinginfo &si);

    ///
    void rescaleFrame(int f, const scalinginfo &si, const imagedata *img=NULL,
		      const imagedata *seg=NULL, 
		      bool emptyImg=false,bool emptySeg=false);

    ///
    void rescaleFrame(const scalinginfo &si,const imagedata *img=NULL, 
		      const imagedata *seg=NULL,
		      bool emptyImg=false,bool emptySeg=false){ 
      rescaleFrame(getCurrentFrame(),si,img,seg,emptyImg,emptySeg);
    }

    ///
    imagedata *copyFrame(int f) ;

    ///
    imagedata *copyFrame() { return copyFrame(getCurrentFrame());}

    ///
    std::vector<imagedata *> copyFrames() ;

    ///
    imagedata *getEmptyFrame() ;
  
    ///
    imagedata *accessFrame(int f)  {
      ensureFrameOk(f);
      return imageFrame(f);
    }

    ///
    imagedata *accessFrame()  {
      return accessFrame(getCurrentFrame());
    }

    ///
    std::vector<const imagedata *> accessFrames() ;

    ///
    imagedata *copySegmentation(int f) ;

    ///
    imagedata *copySegmentation() 
    { return copySegmentation(getCurrentFrame());}

    ///
    std::vector<imagedata *> copySegmentations() ;
    
    ///
    imagedata *getEmptySegmentation() ;

    ///  
    const imagedata *accessSegmentation(int f) 
    { ensureFrameOk(f); return segmentFrame(f);}

    ///
    const imagedata *accessSegmentation() 
    { return accessSegmentation(getCurrentFrame());}

    ///
    std::vector<const imagedata *> accessSegmentations() ;

//     ///
//     imagedata *copyResultImage(const std::string &rseq,int f) ;

//     ///
//     imagedata *copyResultImage(const std::string &rseq) 
//     { return copyResultImage(rseq,getCurrentFrame());}

//     ///
//     imagedata *copyParsedResultFrame(const std::string &) ;

//     ///
//     std::vector<imagedata *> copyResultImages(const std::string &rseq) ;
  
//     ///
//     const imagedata *accessResultImage(const std::string &rseq,int f) ;

//     ///
//     const imagedata *accessResultImage(const std::string &rseq) 
//     { return accessResultImage(rseq,getCurrentFrame());}

//     ///
//     const imagedata *accessParsedResultFrame(const std::string &) ;

//     ///
//     std::vector<const imagedata *> accessResultImages(const std::string &rseq) ;

    
    
    /// copy pixels from image to a given array
    bool CopyPixelsWithMask (double src_start_x, 
			     double src_start_y, 
			     double src_scaling, 
			     bool mask_src,
			     vector <unsigned char> &target, 
			     int tgt_w,  
			     int tgt_start_x, 
			     int tgt_start_y,
			     int tgt_stop_x,
			     int tgt_stop_y);
  
    /// 
    //bool compatibleImagesAndSegments() const; lets try to ensure them to be
    //  compatible at all times

    ///
    bool hasSegments() const { // is this really needed ?
      return segment_frames.frames.size()>0; //  && compatibleImagesAndSegments());
    }

    ///
    bool hasImages() const { // is this really needed ?
      return image_frames.frames.size()>0; //  && compatibleImagesAndSegments());
    }

    ///
    void setImage(const imagedata& i) {
      image_frames.free();
      image_frames.frames.clear();
      _frame_type tmp;
      tmp.segment=false;
      tmp.framenr=0;
      tmp.fileframe=0;
      image_frames.frames.push_back(tmp);
      
      description.sequenceinfo.setFromImagedata(i);
      forceImage(i, 0);
    }

    ///
    void setSegment(const imagedata& i) {
      segment_frames.free();
      segment_frames.frames.clear();
      _frame_type tmp;
      tmp.segment=false;
      tmp.framenr=0;
      tmp.fileframe=0;
      segment_frames.frames.push_back(tmp);
      
      description.sequenceinfo.setFromImagedata(i);
      forceSegment(i, 0);
    }

    /// 
    bool emptyImages() {
      return _is_sequence_empty(image_frames);
    }

    /// 
    bool emptySegments() {
      return _is_sequence_empty(segment_frames);
    }

    ///
    void get_pixel_rgb(int f, int x, int y, std::vector<float> &vf)  {
      ensureFrameOk(f);
      vf = imageFrame(f)->get_float(x, y); 
      if (vf.size()!=3) throw string("segmentfile::get_pixel_rgb : "
				     "didn't get all three components.");
    }

    ///
    void get_pixel_rgb(int f, const coord &c, std::vector<float> &vf)  {
      get_pixel_rgb(f, c.x, c.y, vf);
    }

    ///
    void get_pixel_rgb(int x, int y, std::vector<float> &vf)  {
      get_pixel_rgb(getCurrentFrame(), x, y, vf);
    }

    ///
    void get_pixel_rgb(const coord &c, std::vector<float> &vf)  {
      get_pixel_rgb(c.x, c.y, vf);
    }

    /// to help in converting old code
    void get_pixel_rgb(int x, int y, float &r, float &g, float &b)  {
      std::vector<float> vf;
      get_pixel_rgb(x, y, vf);
      r=vf[0];
      g=vf[1];
      b=vf[2];
    }

    /// to help in converting old code
    void get_pixel_rgb(const coord &c, float &r, float &g, float &b)  {
      get_pixel_rgb(c.x, c.y, r, g, b);
    }

    ///
    void set_pixel_rgb(int f, int x, int y, const std::vector<float> &vf)
    {
      ensureFrameOk(f);
      imageFrame(f)->set(x, y, vf); 
    }

    ///
    void set_pixel_rgb(int f,const coord &c, const std::vector<float> &vf) {
      set_pixel_rgb(f, c.x, c.y, vf); 
    }

    ///
    void set_pixel_rgb(int x, int y, const std::vector<float> &vf) {
      set_pixel_rgb(getCurrentFrame(), x, y, vf); 
    }

    ///
    void set_pixel_rgb(const coord &c, const std::vector<float> &vf) {
      set_pixel_rgb(c.x, c.y, vf); 
    }

    /// to help in converting old code
    void set_pixel_rgb(int x, int y, float r, float g, float b) {
      set_pixel_rgb(x, y, RGBTriple(r, g, b).v); 
    }

    /// to help in converting old code
    void set_pixel_rgb(const coord &c, float r, float g, float b) {
      set_pixel_rgb(c.x, c.y, r, g, b); 
    }

    ///
    void get_pixel_grey(int f, int x, int y, float &val) {
      std::vector<float> vf;
      get_pixel_rgb(f, x, y, vf);
      val=(vf[0]+vf[1]+vf[2])/3;
    }

    ///
    void get_pixel_grey(int f, const coord &c, float &val) {
      get_pixel_grey(f, c.x, c.y, val);
    }

    ///
    void get_pixel_grey(int x, int y, float &val)  {
      get_pixel_grey(getCurrentFrame(), x, y, val);
    }

    ///
    void get_pixel_grey(const coord &c, float &val)  {
      get_pixel_grey(c.x, c.y, val);
    }

    ///
    void set_pixel_grey(int f, int x, int y, float val)
    {
      set_pixel_rgb(f, x, y, std::vector<float>(3, val));
    }

    ///
    void set_pixel_grey(int f, const coord &c, float val){
      set_pixel_grey(f, c.x, c.y, val); 
    }

    ///
    void set_pixel_grey(int x, int y, float val){
      set_pixel_grey(getCurrentFrame(), x, y, val); 
    }

    ///
    void set_pixel_grey(const coord &c, float val){
      set_pixel_grey(c.x, c.y, val); 
    }

    ///
    void get_pixel_segment(int f, int x, int y, int &i)  {
      ensureFrameOk(f);
      i=segmentFrame(f)->get_one_uint32(x, y); 
    }

    ///
    void get_pixel_segment(int f,const coord &c, int &i)  {
      get_pixel_segment(f, c.x, c.y, i); 
    }

    ///
    void get_pixel_segment(int x, int y, int &i)  {
      get_pixel_segment(getCurrentFrame(), x, y, i); 
    }

    ///
    void get_pixel_segment(const coord &c, int &i)  {
      get_pixel_segment(c.x, c.y, i); 
    }

    ///
    void get_pixel_segment_extended(int f, int x, int y, int &i,int bklabel,
				    const PredefinedTypes::Box *bb=NULL)  {
      if (bb) {
	if (bb->isInside(coord(x, y)))
	  get_pixel_segment(f, x, y, i);
	else
	  i = bklabel;

      } else {
	if (coordinates_ok(x, y))
	  get_pixel_segment(f, x, y, i);
	else
	  i = bklabel;
      }
    }

    ///
    void get_pixel_segment_extended(int f, const coord &c, int &i, int bklabel,
				    const PredefinedTypes::Box *bb=NULL) {
      get_pixel_segment_extended(f, c.x, c.y, i, bklabel, bb); 
    }

    ///
    void get_pixel_segment_extended(int x, int y, int &i, int bklabel,
				    const PredefinedTypes::Box *bb=NULL) {
      get_pixel_segment_extended(getCurrentFrame(), x, y, i, bklabel, bb); 
    }

    ///
    void get_pixel_segment_extended(const coord &c, int &i, int bklabel,
				    const PredefinedTypes::Box *bb=NULL) {
      get_pixel_segment_extended(c.x, c.y, i, bklabel, bb); 
    }

    ///
    void set_pixel_segment(int f, int x, int y, int i) {
      ensureFrameOk(f);
      segmentFrame(f)->set(x, y, (uint32_t)i); 
    }

    ///
    void set_pixel_segment(int f, const coord &c, int i) {
      set_pixel_segment(f, c.x, c.y, i); 
    }

    ///
    void set_pixel_segment(int x, int y, int i) {
      set_pixel_segment(getCurrentFrame(), x, y, i); 
    }

    ///
    void set_pixel_segment(const coord &c, int i) {
      set_pixel_segment(c.x, c.y, i); 
    }

    ///
    vector<int> getSegmentVector(int x, int y){
      // obs! frame information not used

      vector<int> ret;
      if(region_spec.empty()) {
      	int s;
      	get_pixel_segment(x, y, s);
      	ret.push_back(s);
      	return ret;
       }
      
      for (size_t i=0; i<region_spec.size(); i++) {
	if (region_spec[i]->contains(x, y)) { // f not used!
	  ret.push_back(i+1); // FIRST_REGION_BASED_SEGMENT==1
	}
	if (ret.empty()) ret.push_back(0);
      }
      return ret;
    }

    /// test whether there's pixel with same label in dir
    bool HasNeighbourSeg(const coord &c,int dir) ;

    /// count neighbours
    int NeighbourCountSeg(const coord &c,bool use_diagonals=true) ;
    
    /// NeighbourCount < 8
    bool isSegmentBoundary(const coord &c) ;
    
    ///
    bool isSegmentBoundary(int x, int y) ;

    ///  counts segment labels used
    int GetLabelCount() ;

    ///  collects segment labels in use in a list
    std::vector<int> *getUsedLabels(const coord&ul,const coord &br);

    ///
    std::vector<int> *getUsedLabels(){
      return getUsedLabels(coord(0,0),coord(getWidth()-1,getHeight()-1));
    }

    ///
    std::vector<std::pair<int,std::string> > getUsedLabelsWithText() ;

    ///
    std::set<int> getBackgroundLabels(int frame) const;

    ///
    std::set<int> getBackgroundLabels() const {
      return getBackgroundLabels(getCurrentFrame());
    }

    ///
    void setBackgroundLabels(int frame,const std::set<int> &bs);

    ///
    void setBackgroundLabels(const std::set<int> &bs){
      setBackgroundLabels(getCurrentFrame(),bs);
    }

    ///
    int getFirstFreeLabel() ;

    ///
    int GetFeatureObjectCount()  { return GetLabelCount(); }

    /// counts separate components
    int GetSeparateCount() ;

    /// returns an array containing unique labeling of separate components
    int *GetSeparateLabelingInt(CorrespArray *c=NULL,
				bool fourConnectivity=false,
				bool bordersConnectLabelZero=false,
				const PredefinedTypes::Box *boundingBox=NULL ) ;

    // helper function the above 
    void MarkSame(int *u,int x, int y,
		  const PredefinedTypes::Box &bb,
		  int nbr,int bklabel) ;

    ///
    int getSegmentArea(int label) ;

    ///
    std::map<int,std::set<int> > findNeighbours(bool fourNeighbours=false);

    ///
    std::vector<std::set<int> > getAdjacentCombinations();

    /// 
    labeledListList *changeLabels(int to, const coordList &list);
    
    ///
    labeledListList *changeLabels(int to, const Matrix &matrix);

    ///  same as above but don't spare the original labels
    void translateLabels(int to,const coordList &list);
    
    ///
    coordList listCoordinates(int label) ;

    /// finds bounding box of object (nonzero labels) in segmented image
    bool FindBox(coord &nw, coord &se );

    /// Filter away thin horisontal and vertical lines (5 pixel neighbourhood)
    void FilterLineEdges(const RGBTriple *means,const RGBTriple &bkRGB=
			 RGBTriple(0,0,0));

    ///
    void FilterLineEdges(const RGBTriple &bkRGB=RGBTriple(0,0,0)){ 
      FilterLineEdges(NULL,bkRGB);
    }

    ///
    void FilterLineEdgesByRGB(float colourTolerance=30);

    /// parses region hierarchy in XML and returns list of the 
    /// primitive regions higher level regions consist of

    std::vector<std::set<int> > 
    flattenHierarchyToPrimitiveRegions(bool alterbitmap=false);

    ///
    std::vector<std::pair<std::set<int>,std::string> > parseVirtualRegions();

    /// 
    std::string getZoningText(){ return zoningText;};

    ///
    bool prepareZoning(const std::string& zoningType);

    ///
    bool zoningTiles(int w, int h, int xlen, int ylen, int dx, int dy);

    ///
    bool zoningTiles_old(int tilesX, int tilesY);

    ///
    void parseRegionsFromXML();

    ///
    int regionCount() const {return region_spec.size();}

    ///
    const vector<Region*>& regionSpec() const { return region_spec; }

    void frameCacheing(bool c) {
     use_cacheing = c;
      if(input_image_file!=NULL)
	input_image_file->frame_cacheing(c);
      if(input_segment_file!=NULL)
	input_segment_file->frame_cacheing(c);
    }

    ///
    Region *region(int i) const {return region_spec[i];} 


    char *getXMLDescription() const { return description.outputXML();}

    ///
    xmlNodePtr getMethodInfoListXML() const{
      return description.methods.outputXML();
    }

    ///
    void setFirstInChain(const ChainedMethod *f){ 
      first=f; }

    ///
    void setProcessInfoProvider(const ProcessInfoProvider *p){ 
      processInfoProvider=p;
    }

    ///
    void setFileInfoProvider(const FileInfoProvider *p){
      fileInfoProvider=p;
    }

    ///
    xmlNodePtr writeFrameResultToXML(const ChainedMethod *method,
				     const SegmentationResult &r){
      return writeFrameResultToXML(method,getCurrentFrame(),r);
    }
    ///
    xmlNodePtr writeFrameResultToXML(const ChainedMethod *method,int frame,
				     const SegmentationResult &r){
      return description.addFrameResult(frame,method,r);
    }
    ///
    xmlNodePtr writeFileResultToXML(const ChainedMethod *method,
				    const SegmentationResult &r){
      return description.addFileResult(method,r);
    }

    ///
    SegmentationResultList *readFrameResultsFromXML(int frame,
						    const std::string& name="",
						    const std::string& type="",
						    bool wildcard=false,
						    bool alsoinvalid=false)
      const {
      return description.readFrameResults(NULL,frame,all,name,type,
					  wildcard,alsoinvalid);
    }
    
    ///
    SegmentationResultList *readFrameResultsFromXML(const std::string& name="",
						    const std::string& type="",
						    bool wildcard=false,
						    bool alsoinvalid=false)
      const {
      return readFrameResultsFromXML(getCurrentFrame(),name,type,
				     wildcard,alsoinvalid);
    }

    void invalidateFrameResults(const ChainedMethod *m,int frame,
				const std::string& name="",
				const std::string& type="",
				bool wildcard=false){
      description.invalidateFrameResults(m,frame,all,name,type,wildcard);
    }


    void invalidateFrameResults(const ChainedMethod *m,const std::string& name="",
				const std::string& type="",
				bool wildcard=false){
      invalidateFrameResults(m,getCurrentFrame(),name,type,wildcard);
    }

    ///
    std::vector<xmlNodePtr> accessFrameResultNodes(int frame,
						    const std::string& name="",
						    const std::string& type="",
						    bool wildcard=false,
						   bool alsoinvalid=false)
    {
      return description.accessFrameResultNodes(NULL,frame,all,name,
						type,wildcard,alsoinvalid);
    }    

    ///
    std::vector<xmlNodePtr> accessFrameResultNodes(const std::string& name="",
						   const std::string& type="",
						   bool wildcard=false,bool alsoinvalid=false)
    {
      return accessFrameResultNodes(getCurrentFrame(),name,type,wildcard,alsoinvalid);
    }

    ///
    SegmentationResultList *readLatestMethodFrameResultsFromXML(const ChainedMethod *m,int frame,
						    const std::string& name="",
						    const std::string& type="",
								bool wildcard=false,bool alsoinvalid=false) const
    {
      return 	description.readFrameResults(m,frame,latest_method,name,type,
					     wildcard,alsoinvalid);
    }
    ///
    SegmentationResultList *readLatestMethodFrameResultsFromXML(const ChainedMethod *m,const std::string& name="",
								const std::string& type="",
								bool wildcard=false,bool alsoinvalid=false) const
    {
      return readLatestMethodFrameResultsFromXML(m,getCurrentFrame(),name,type,wildcard,alsoinvalid);
    }

    void invalidateLatestMethodFrameResults(const ChainedMethod *m,int frame,
				const std::string& name="",
				const std::string& type="",
				bool wildcard=false){
      description.invalidateFrameResults(m,frame,latest_method,name,
					 type,wildcard);
    }


    void invalidateLatestMethodFrameResults(const ChainedMethod *m,
					    const std::string& name="",
					    const std::string& type="",
					    bool wildcard=false){
      invalidateLatestMethodFrameResults(m,getCurrentFrame(),name,type,wildcard);
    }


    ///
    std::vector<xmlNodePtr> accessLatestMethodFrameResultNodes(const ChainedMethod *m,int frame,
						   const std::string& name="",
							       const std::string& type="",bool wildcard=false,bool alsoinvalid=false)
    {
      return 	description.accessFrameResultNodes(m,frame,latest_method,name,
						   type,wildcard,alsoinvalid);
    }    

    ///
    std::vector<xmlNodePtr> accessLatestMethodFrameResultNodes(const ChainedMethod *m,const std::string& name="",
							       const std::string& type="",bool wildcard=false,bool alsoinvalid=false)
    {
      return accessLatestMethodFrameResultNodes(m,getCurrentFrame(),name,type,wildcard,alsoinvalid);
    }



    ///
   SegmentationResult *readLastFrameResultFromXML(int frame,
						  const std::string& name,
						  const std::string& type="",bool wildcard=false,bool alsoinvalid=false) const 
    {
	SegmentationResultList *list=
	  description.readFrameResults(NULL,frame,last,name,type,wildcard,alsoinvalid);
	if(list->empty()){
	  delete list;
	  return NULL;
	} else{
	  SegmentationResult *r=new SegmentationResult((*list)[0]);
	  delete list;
	  return r;
	}
    }

    ///
    SegmentationResult *readLastFrameResultFromXML(const std::string& name,
						   const std::string& type="",bool wildcard=false,bool alsoinvalid=false)
      const {
      return readLastFrameResultFromXML(getCurrentFrame(),name,type,wildcard,alsoinvalid);
    }
 
    void invalidateLastFrameResult(const ChainedMethod *m,int frame,
				const std::string& name="",
				const std::string& type="",
				bool wildcard=false){
      description.invalidateFrameResults(m,frame,last,name,
					 type,wildcard);
    }


    void invalidateLastFrameResult(const ChainedMethod *m,const std::string& name="",
				const std::string& type="",
				bool wildcard=false){
      invalidateLastFrameResult(m,getCurrentFrame(),name,type,wildcard);
    }

    ///
    xmlNodePtr accessLastFrameResultNode(int frame,
				     const std::string& name,
					 const std::string& type="",bool wildcard=false,bool alsoinvalid=false){
      std::vector<xmlNodePtr> list=
	description.accessFrameResultNodes(NULL,frame,last,name,type,wildcard,alsoinvalid);
      return list.empty() ? NULL : list[0];

    }
    ///
    xmlNodePtr accessLastFrameResultNode(const std::string& name,
					 const std::string& type="",bool wildcard=false,bool alsoinvalid=false)
    {
      return accessLastFrameResultNode(getCurrentFrame(),name,type,wildcard,alsoinvalid);
    }


    ///
    SegmentationResultList *
    readFileResultsFromXML(const std::string& name="",
			   const std::string& type="",
			   bool wildcard=false,bool alsoinvalid=false) const{
      return description.readFileResults(NULL,all,name,type,wildcard,
					 alsoinvalid);
    }


    void invalidateFileResults(const ChainedMethod *m,
			       const std::string& name="",
			       const std::string& type="",
			       bool wildcard=false){
      description.invalidateFileResults(m,all,name,type,wildcard);
    }


    ///
    std::vector<xmlNodePtr> 
    accessFileResultNodes( const std::string& name="",
			   const std::string& type="",
			   bool wildcard=false,bool alsoinvalid=false){
      return description.accessFileResultNodes(NULL,all,name,type,
					       wildcard,alsoinvalid);
    }

    ///
    SegmentationResultList 
    *readLatestMethodFileResultsFromXML(const ChainedMethod *m,
					const std::string& name="",
					const std::string& type="",
					bool wildcard=false,
					bool alsoinvalid=false) const {
      return description.readFileResults(m, latest_method, name, type,
					 wildcard, alsoinvalid);
    }

    void invalidateLatestMethodFileResults(const ChainedMethod *m,
					   const std::string& name="",
					   const std::string& type="",
					   bool wildcard=false){
      description.invalidateFileResults(m,latest_method,name,type,wildcard);
    }



    ///
    std::vector<xmlNodePtr> accessLatestMethodFileResultNodes( const ChainedMethod *m,const std::string& name="",
							       const std::string& type="",bool wildcard=false,bool alsoinvalid=false){
      return description.accessFileResultNodes(m,latest_method,name,type,wildcard,alsoinvalid);
    }

    ///
    SegmentationResult *readLastFileResultFromXML(const std::string& name,
						  const std::string& type="",bool wildcard=false,bool alsoinvalid=false) const{
      SegmentationResultList *list=
	description.readFileResults(NULL,last,name,type,wildcard,alsoinvalid);
      if(list->empty()){
	delete list;
	return NULL;
      } else{
	SegmentationResult *r=new SegmentationResult((*list)[0]);
	delete list;
	return r;
      }
    }
    
    void invalidateLastFileResult(const ChainedMethod *m,const std::string& name="",
				  const std::string& type="",
				  bool wildcard=false){
      description.invalidateFileResults(m,last,name,type,wildcard);
    }

 ///
    xmlNodePtr accessLastFileResultNode( const std::string& name,
					 const std::string& type="",bool wildcard=false,bool alsoinvalid=false){
      std::vector<xmlNodePtr> list=
	description.accessFileResultNodes(NULL,last,name,type,wildcard,alsoinvalid);
      if(list.empty()){
	return NULL;
      } else{
	return list[0];
      }
    }

    ///
    std::string getLastSequenceAttribute(const std::string &sequence,
					 const std::string &key ="") const{
      return description.attributes.count(sequence)?
	description.attributes.find(sequence)->second.sequenceAttributes.getLastAttribute(key)
	: std::string();
    }
    ///
    std::string getLastImageSequenceAttribute(const std::string &key ="") const
    {
      return getLastSequenceAttribute("image",key);
    }

    ///
    std::string getLastSegmentSequenceAttribute(const std::string &key ="") const
    {
      return getLastSequenceAttribute("segment",key);
    }

    ///
    std::multimap<std::string,std::string>
    getAllSequenceAttributes(const std::string &sequence,
			     const std::string &key ="") const{
      return description.attributes.count(sequence)?
	description.attributes.find(sequence)->second.sequenceAttributes.getAllAttributes(key)
	: std::multimap<std::string,std::string>();
    }

    /// 
    std::multimap<std::string,std::string>
    getAllImageSequenceAttributes(const std::string &key ="")const {
      return getAllSequenceAttributes("image",key);
    }

    /// 
    std::multimap<std::string,std::string>
    getAllSegmentSequenceAttributes(const std::string &key ="")const {
      return getAllSequenceAttributes("segment",key);
    }

    ///
    void addSequenceAttribute(const std::string &sequence,
			      const std::string &key,
			      const std::string &value){
      description.attributes[sequence].sequenceAttributes.addAttribute(key,value);
    }

    ///
    void addImageSequenceAttribute(const std::string &key,
				   const std::string &value){
      addSequenceAttribute("image",key,value);
    }

    ///
    void addSegmentSequenceAttribute(const std::string &key,
				   const std::string &value){
      addSequenceAttribute("segment",key,value);
    }

    ///
    void replaceSequenceAttribute(const std::string &sequence,
				  const std::string &key,
				  const std::string &value){
      description.attributes[sequence].sequenceAttributes.replaceAttribute(key,value);
    }

    ///
    void replaceImageSequenceAttribute(const std::string &key,
				       const std::string &value){
      replaceSequenceAttribute("image",key,value);
    }

    ///
    void replaceSegmentSequenceAttribute(const std::string &key,
				       const std::string &value){
      replaceSequenceAttribute("segment",key,value);
    }

    ///
    void removeSequenceAttribute(const std::string &sequence,
				 const std::string &key=""){
      if(description.attributes.count(sequence))
	description.attributes[sequence].sequenceAttributes.removeAttribute(key);
    }

    ///
    void removeImageSequenceAttribute(const std::string &key=""){
      removeSequenceAttribute("image",key);
    }

    ///
    void removeSegmentSequenceAttribute(const std::string &key=""){
      removeSequenceAttribute("segment",key);
    }

    ///
    std::string getLastFrameAttribute(const std::string &sequence, int frame,
				      const std::string &key ="") const{
      return (description.attributes.count(sequence) && 
	      description.attributes.find(sequence)->second.frameAttributes.count(frame)) ?
	description.attributes.find(sequence)->second.frameAttributes.find(frame)->second.getLastAttribute(key)
	: std::string();
    }

    ///
    std::string getLastImageFrameAttribute(int frame,
					   const std::string &key ="")const {
      return getLastFrameAttribute("image",frame,key);
    }

    ///
    std::string getLastSegmentFrameAttribute(int frame,
					   const std::string &key ="") const{
      return getLastFrameAttribute("segment",frame,key);
    }

    ///
    std::string getLastFrameAttribute(const std::string &sequence,
				      const std::string &key ="") const {
      return getLastFrameAttribute(sequence,getCurrentFrame(),key);
    }

    ///
    std::string getLastImageFrameAttribute(const std::string &key ="")const {
      return getLastFrameAttribute("image",key);
    }

    ///
    std::string getLastSegmentFrameAttribute(const std::string &key ="")const {
      return getLastFrameAttribute("segment",key);
    }

    ///
    std::multimap<std::string,std::string>
    getAllFrameAttributes(const std::string &sequence, int frame,
			  const std::string &key ="") const {
      return (description.attributes.count(sequence) && 
	      description.attributes.find(sequence)->second.frameAttributes.count(frame)) ?
	description.attributes.find(sequence)->second.frameAttributes.find(frame)->second.getAllAttributes(key)
	: std::multimap<std::string,std::string>();
    }

    ///
    std::multimap<std::string,std::string>
    getAllImageFrameAttributes(int frame,const std::string &key ="")const {
      return getAllFrameAttributes("image",frame,key);
    }

    ///
    std::multimap<std::string,std::string>
    getAllSegmentFrameAttributes(int frame,const std::string &key ="")const{
      return getAllFrameAttributes("segment",frame,key);
    }

    ///
    std::multimap<std::string,std::string>
    getAllFrameAttributes(const std::string &sequence,
			  const std::string &key ="")const {
      return getAllFrameAttributes(sequence,getCurrentFrame(),key);
    }

    ///
    std::multimap<std::string,std::string>
    getAllImageFrameAttributes(const std::string &key ="")const{
      return getAllFrameAttributes("image",key);
    }

    ///
    std::multimap<std::string,std::string>
    getAllSegmentFrameAttributes(const std::string &key ="")const{
      return getAllFrameAttributes("segment",key);
    }

    ///
    void addFrameAttribute(const std::string &sequence,int frame,
			   const std::string &key,
			   const std::string & value){
      description.attributes[sequence].frameAttributes[frame].addAttribute(key,value);
    }

    ///
    void addImageFrameAttribute(int frame, const std::string &key,
				const std::string & value){
      addFrameAttribute("image",frame,key,value);
    }

    ///
    void addSegmentFrameAttribute(int frame, const std::string &key,
				const std::string & value){
      addFrameAttribute("segment",frame,key,value);
    }


    ///
    void addFrameAttribute(const std::string &sequence,
			   const std::string &key,
			   const std::string &value){
      addFrameAttribute(sequence,getCurrentFrame(),key,value);
    }


    ///
    void addImageFrameAttribute(const std::string &key,
				const std::string &value){
      addFrameAttribute("image",key,value);
    }

    ///
    void addSegmentFrameAttribute(const std::string &key,
				const std::string &value){
      addFrameAttribute("segment",key,value);
    }


    ///
    void replaceFrameAttribute(const std::string &sequence, int frame,
				  const std::string &key,
			       const std::string &value){
      description.attributes[sequence].frameAttributes[frame].replaceAttribute(key,value);
    }
    ///
    void replaceImageFrameAttribute(int frame, const std::string &key,
				    const std::string &value){
      replaceFrameAttribute("image",frame,key,value);
    }

    ///
    void replaceSegmentFrameAttribute(int frame, const std::string &key,
				    const std::string &value){
      replaceFrameAttribute("segment",frame,key,value);
    }


    ///
    void replaceFrameAttribute(const std::string &sequence,
			       const std::string &key,
			       const std::string &value){
      replaceFrameAttribute(sequence,getCurrentFrame(),key,value);
    } 

    ///
    void replaceImageFrameAttribute(const std::string &key,
				    const std::string &value){
      replaceFrameAttribute("image",key,value);
    } 

    ///
    void replaceSegmentFrameAttribute(const std::string &key,
				    const std::string &value){
      replaceFrameAttribute("segment",key,value);
    } 

    ///
    void removeFrameAttribute(const std::string &sequence, int frame,
			      const std::string &key=""){
      if (description.attributes.count(sequence) && 
	  description.attributes[sequence].frameAttributes.count(frame))
	description.attributes[sequence].frameAttributes[frame].removeAttribute(key);
    }

    ///
    void removeImageFrameAttribute(int frame, const std::string &key=""){
      removeFrameAttribute("image",frame,key);
    }

    ///
    void removeSegmentFrameAttribute(int frame, const std::string &key=""){
      removeFrameAttribute("segment",frame,key);
    }

    ///
    void removeFrameAttribute(const std::string &sequence,
			      const std::string &key=""){
      removeFrameAttribute(sequence,getCurrentFrame(),key);
    }

    ///
    void removeImageFrameAttribute(const std::string &key=""){
      removeFrameAttribute("image",key);
    }

    ///
    void removeSegmentFrameAttribute(const std::string &key=""){
      removeFrameAttribute("segment",key);
    }


    ///
    void copyAttributes(const std::string &tgtseq, const std::string &srcseq){
      if(description.attributes.count(srcseq))
	description.attributes[tgtseq]=description.attributes[srcseq];
      else
	description.attributes.erase(tgtseq);
    }

    void copyFrameAttributes(const std::string &tgtseq, int tgtframe,
			     const std::string &srcseq, int srcframe){
      if(description.attributes.count(srcseq) 
	 && description.attributes.find(srcseq)->second.frameAttributes.count(srcframe))
	description.attributes[tgtseq].frameAttributes[tgtframe]=
	  description.attributes[srcseq].frameAttributes[srcframe];
      else if(description.attributes.count(tgtseq)) 
	description.attributes[tgtseq].frameAttributes.erase(tgtframe);
    }

    /// functions for storing/removing images/segments

    ///
    int storeImage(const imagedata &i); 
    // store image to next free location, return its new identifier
    
    ///
    int storeSegment(const imagedata &i); 

    ///
    bool removeImage(int id);

    /// 
    bool removeSegment(int id);

    ///
    imagedata *copyStoredImage(int id);

    ///
    imagedata *copyStoredSegment(int id);


    void storeIPListAsFrameResult(const ChainedMethod *method, ipList &l,const string &name){
      l.writeToXML(method,*this,name);
    }

    void parseIPListFromLastFrameResult(ipList &l,const string &name){
      l.parseFromXML(*this,name);
    }

    

//     ///
//     std::string storeImageFrames();

//     ///
//     std::string storeSegmentFrames();

//     ///
//     std::string storeAsResult(const std::vector<const imagedata *> images, bool checkSequenceLength=true);
    
//     ///
//     void removeStoredSequence(const std::string &sequenceName);

//     ///
//     void removeParsedStoredSequence(const std::string &rspec);

//     ///
//     std::string storeImageAsFrameResult(const ChainedMethod *method,
// 					int frame,const std::string &name,
// 					bool merge=true);

//     ///
//     std::string storeImageAsFrameResult(const ChainedMethod *method,
// 					const std::string &name,
// 					bool merge=true){
//       return storeImageAsFrameResult(method,getCurrentFrame(),name,merge);
//     }

//     ///
//     std::string storeSegmentAsFrameResult(const ChainedMethod *method,
// 					  int frame,
// 					  const std::string &name,
// 					  bool merge=true);

//     ///
//     std::string storeSegmentAsFrameResult(const ChainedMethod *method,
// 					   const std::string &name,
// 					  bool merge=true){
//       return storeSegmentAsFrameResult(method,getCurrentFrame(),name,merge);
//     }

//     ///
//     std::string storeAsFrameResult(const ChainedMethod *method,int frame,
// 				   const std::string &name, 
// 				   const imagedata *i,bool merge=true);

//     ///
//     std::string storeAsFrameResult(const ChainedMethod *method,const std::string &name, 
// 				   const imagedata *i,bool merge=true){
//       return storeAsFrameResult(method,getCurrentFrame(),name,i,merge);
//     }

//     ///
//     std::string storeImagesAsFileResult(const ChainedMethod *method,
// 					const std::string &name);
//     ///
//     std::string storeSegmentsAsFileResult(const ChainedMethod *method,
// 					  const std::string &name);

//     ///
//     std::string storeAsFileResult(const ChainedMethod *method,
// 				  const string &name, 
// 				  const std::vector<const imagedata *> &v);

//     ///
//     std::string storeAsFileResult(const ChainedMethod *method,
// 				  const string &name, 
// 				  const image_sequence &i);

    ///
    bool getAlignmentMarks(coord &m1,coord &m2,coord &m3,coord &m4);

    /// creates false colour image of the labeling
    imagedata *getColouring(const std::vector<int> *labels,
			    const std::vector<RGBTriple> *colours=NULL) ;

    /// appends false colour image of the labeling to given image 
    imagedata *getColouring(imagedata *img, const std::vector<int> *labels,
			    const std::vector<RGBTriple> *colours=NULL) ;

    /// creates false colour image of the labeling
    imagedata *getAverageRgb() ;

    /// appends false colour image of the labeling to given image 
    imagedata *getAverageRgb(imagedata *img) ;

    /// creates dimmed colour image according to the labeling
    imagedata *getDimmedImage(const std::vector<int> *labels,
			      const std::vector<float> *dims=NULL,
			      float default_dim=0,
			      const vector<Region*> *region_spec=NULL) ;

    /// appends dimmed colour image to given image
    imagedata *getDimmedImage(imagedata *img, 
			      const std::vector<int> *labels,
			      const std::vector<float> *dims=NULL,
			      float default_dim=0,
			      const vector<Region*> *region_spec=NULL) ;

    /// creates hue masked image according to the labeling
    imagedata *getHueMasking(const std::vector<int> *labels,
			     const std::vector<float> *levels=NULL,
			     float default_level=0) ;

    /// appends hue masked image to given image
    imagedata *getHueMasking(imagedata *img, 
			     const std::vector<int> *labels,
			     const std::vector<float> *levels=NULL,
			     float default_level=0) ;

    /// creates crossed image according to the labeling
    imagedata *getCrossedImage(const std::vector<int> *labels,
			       int width, int space);

    /// appends crossed image to given image
    imagedata *getCrossedImage(imagedata *img,
			       const std::vector<int> *labels,
			       int width, int space);

    /// creates image with boundaries marked according to the labeling
    imagedata *markBoundaries(const std::vector<int> *labels=NULL, 
bool outerBoundaries=false,
			      const RGBTriple & = RGBTriple(1.0,1.0,1.0)) ;

    /// marks boundaries to given image
    imagedata *markBoundaries(imagedata *img,
			      const std::vector<int> *labels=NULL, bool outerBoundaries=false,
			      const RGBTriple & = RGBTriple(1.0,1.0,1.0)
			      ) ;
    

    /// creates image with visualisation of the segmentation result
    imagedata *showResult(const std::string &value, 
			  const std::string &type,
			  const std::string &colour) ;

    /// appends visualisation of the segmentation result to the image
    imagedata *showResult(imagedata *img, const std::string &value, 
			  const std::string &type,
			  const RGBTriple &colour) ;
    
    void showDataTypes();

    ///
    const std::string &getInputPattern() const {return naming.input_pattern;}

    ///
    const std::string &getOutputPattern() const {return naming.output_pattern;}

    ///
    void SetOutputPattern(const string& n) { naming.output_pattern = n; }

    ///
    void SetStandardOutputNaming(int i) { SetOutputPattern(StandardNaming(i));}
    
    ///
    void SetNullOutput() { SetOutputPattern("/dev/null"); }
    
    ///
    void SetInputPattern(const string& n) { naming.input_pattern = n; }
  
    ///
    void SetStandardInputNaming(const string&, const string&);

    ///
    static string StandardNaming(int i) {
      if (i==1)
	return "%b/segments/%m%o:%i.seg";
      if (i==2)
	return "%B/segments/%m%o:%i.seg";
      return "segments/%m%o:%i.seg";
    }

    ///
    static string StandardNaming(const string& d) {
      if (d=="")
	return "segments/%m%o:%i.seg";
      if (d=="b" || d=="B")
	return "%"+d+"/segments/%m%o:%i.seg";
      return "";
    }

    ///
    static std::string FormOutputFileName(const std::string&,
				   const std::string&,
				   const std::string& ="");

    
    ///
    static std::string FormInputFileName(const std::string&,
					 const std::string&,
					 const std::string& ="");
    ///
    void formFileNames(const char *img,const char *out,const std::string &m);

    ///
    const std::string &imageName() const {return naming.image_name;}

    ///
    void imageName(const std::string &s){naming.image_name=s;}

    ///
    const std::string &inSegmentName() const {return naming.in_segment_name;}

    ///
    void inSegmentName(const std::string &s){naming.in_segment_name=s;}

    ///
    const std::string &outSegmentName() const {return naming.out_segment_name;}

    ///
    void outSegmentName(const std::string &s){naming.out_segment_name=s;}

    ///
    void showHist();
    
    /// 
    int countFrameColours();

    ///
    std::string collectChainedName() const;

    ///
    preprocessResult *accessPreprocessResult(int f, const std::string &id);

    ///
    preprocessResult *accessPreprocessResult(const std::string &id){
      return accessPreprocessResult(getCurrentFrame(),id);
    }

    ///
    void discardPreprocessResult(int f, const std::string &id);

    ///
    void discardPreprocessResult(const std::string &id){
      discardPreprocessResult(getCurrentFrame(),id);
    }

    void discardAllPreprocessResults();

    preprocessResult *preprocess_SobelThreshold(int f, const std::string &id);

    preprocessResult *preprocess_SobelMax(int f, const std::string &id);

    preprocessResult *preprocess_Sobel(int f, const std::string &id);

    preprocessResult *preprocess_Cielab(int f, const std::string &id);

    preprocessResult *preprocess_Cieluv(int f, const std::string &id);

    preprocessResult *preprocess_HueMinMax(int f, const std::string &id);

    preprocessResult *preprocess_HueDiffSumQuant256(int f,
						    const std::string &id);

    pairingPredicate determinePairingPredicate(const std::string &p);

    void determinePairingPredicateAdjacent(pairingPredicate &p);

    string FrameSpec(int f) const {
      map<int,string>::const_iterator i = description.frameSpecs.find(f);
      return i==description.frameSpecs.end() ? "" : i->second;
    }

    /// functions related to tagging frames being processed 

    bool isFrameProcessed(int f) const {
      return description.processedFrames.count(f)>0 && 
	description.processedFrames.find(f)->second.size()>0;
    }

    std::set<int> listProcessedFrames() const {
      std::set<int> ret;
      for (std::map<int,std::set<int> >::const_iterator
	    it=description.processedFrames.begin();
	  it != description.processedFrames.end(); it++)
	if (it->second.size()>0)
	  ret.insert(it->first);

      return ret;
    }

    std::set<int> listFramesProcessedByPrevious(const ChainedMethod *m) const {
      int idx=description.methods.methodIndex(m)-1;

      std::set<int> ret;
      for (std::map<int,std::set<int> >::const_iterator
	     it=description.processedFrames.begin();
	   it != description.processedFrames.end();it++)
	if (it->second.count(idx)>0)
	  ret.insert(it->first);

      return ret;
    }

    void tagFrameAsProcessed(const ChainedMethod *m,int frame){
      description.tagFrameAsProcessed(m,frame);
    }

    void tagFrameAsProcessed(const ChainedMethod *m){
      tagFrameAsProcessed(m,getCurrentFrame());
    }
    
  private:
    ///
    void _free_image_sequences(){
      image_frames.free();
      segment_frames.free();
      _free_stored_results();
    }
  
    ///
    void _free_stored_results(){

      std::map<int,_frame_type>::iterator it;
      for(it=stored_images.begin(); it != stored_images.end();it++)
	if(it->second.owndata){
	  delete it->second.data;
	  it->second.data=NULL;
	}

      for(it=stored_segments.begin(); it != stored_segments.end();it++)
	if(it->second.owndata){
	  delete it->second.data;
	  it->second.data=NULL;
	}
    }

    ///
    bool _is_sequence_empty(image_sequence &s) {
      // fetch all possibly nonempty frames into memory
      bool empty=true;

      for(size_t f=0;empty && f<s.frames.size();f++)
	{
	  bool wasdiscarded=s.frames[f].data==NULL;
	  if(!wasdiscarded || 
	     (s.frames[f].frameSrc==file && _fetch_frame(s.frames[f]))){
	    empty = s.frames[f].data==NULL || s.frames[f].data->iszero(); 
	    if(wasdiscarded)
	      _discard_frame(s.frames[f]);
	  }
	}

      return empty;
    }

    static xmlNodePtr xmlFromToc(const _toc_type &toc){
      xmlNodePtr ret = xmlNewNode(NULL,(xmlChar*)"tifftoc");
      for(_toc_type::const_iterator it=toc.begin();it!=toc.end();it++)
	_add_toc_entry_to_xml(ret,*it);

      return ret;
    }

  public:
    ///
    void _dump_xml();

  private:
    ///
    _toc_type _read_toc(xmlNodePtr node);

    ///
   _toc_type prepareTOC(bool includeImages, bool includeStoredResults,
			     bool touchFrames=true);

    ///
    static void _add_toc_entry_to_xml(xmlNodePtr n,const _toc_entry &e);

    ///
    void setXMLTOC(xmlNodePtr n);

    ///
    static void checkXMLVersion(const xmlDocPtr doc);

    ///
    //void _insert_frame(image_sequence &s, const _frame_type &f);

    ///
    xmlNodePtr writeMethodInfoToXML(xmlNodePtr methodinfonode, 
				    const ChainedMethod *ip);
    ///
    int resolve_sec(int x, int y, int w, int h);

    ///
    bool zoningTheOriginalOne(int w, int h);

    ///
    bool zoningCenterHorizVert(int w, int h);
    
    ///
    bool zoningHorizVert(int w, int h);
    
    ///
    bool zoningHoriz(int w, int h);
    
    ///
    bool zoningVert(int w, int h);
    
    ///
    bool zoningCenter(int w, int h);
    
    ///
    bool zoningNone(int w, int h);

    ///
    static std::string _find_prefix(const std::string &s);
  
    ///
    static bool readResultFromXML(const xmlNodePtr resultnode,
			   SegmentationResult &result);

//     ///
//     static void checkSequenceName(const std::string &s);

//     ///
//     static void _split_sequence_spec(const std::string &spec,
// 				     std::string &name, int &nr);
 
//     ///
//     void _store_sequence(const std::string &rseq, const image_sequence &s);

//     ///
//     void _store_sequence(const std::string &rseq, 
// 			 const std::vector<const imagedata *> &v);

//     ///
//     string _insert_result_frame(const std::string &rseq, const imagedata *i);    
    
    ///
    void initXMLSkeleton();

    /// 
    static void getStdColouring(int ncols,std::vector<RGBTriple> &colouring);

    ///
    std::map<int,float> * expandLabeling(const std::vector<int> &labels, 
					    const std::vector<float> &values,
					 float std_value);
    
    /// replace "-1" in label list with list of all the used labels
    bool expandLabeling(std::vector<int> &labels);

    ///
    std::map<int,RGBTriple>
    *expandColouring(const std::vector<int> &labels, 
		     const std::vector<RGBTriple> &values, 
		     RGBTriple std_value) ;

    ///
    static std::string _intvector_to_string(const std::vector<int> &v);

    ///
    static std::vector<int> _string_to_intvector(const std::string &s);

    /// on-demand loading of frames
    imagedata *fetchImageFrame(int f){
      
      if(!use_cacheing && image_frames.frames[f].data==NULL){ 
	// cout << "deallocating image frame" << image_frames.lastcachedframe << endl;
	image_frames.freeFrame(image_frames.lastcachedframe);
      }

      image_frames.lastcachedframe=f;

      return _fetch_frame(image_frames.frames[f]);
    }

    /// 
    imagedata *fetchSegmentFrame(int f){
      if(!use_cacheing && segment_frames.frames[f].data==NULL) 
	segment_frames.freeFrame(segment_frames.lastcachedframe);

      segment_frames.lastcachedframe=f;

      return _fetch_frame(segment_frames.frames[f]);
    }

    /// 
    imagedata *fetchStoredImage(int id){
      return stored_images.count(id)?_fetch_frame(stored_images[id]):NULL;
    }

    imagedata *fetchStoredSegment(int id){   
      return stored_images.count(id)?_fetch_frame(stored_images[id]):NULL;
    }

    ///

    ///
    imagedata *_fetch_frame(_frame_type &f);

    ///
    void _discard_frame(_frame_type &f);

    ///
    class _description_type {
    public:
      class _result_collection_type {
      public:
	~_result_collection_type() {
	  freeXML();
	}

	// actual storage for the result nodes
	// both xml and parsed nodes are kept in storage 
	// and should be kept in sync

	std::vector<std::pair<xmlNodePtr,SegmentationResult> > resultnodes;

	// indexes for the storage on <key,type> pairs
	std::multimap<std::string,int> name2idx;
	std::multimap<std::string,int> prefix2idx;
	std::multimap<std::string,int> type2idx;
	std::multimap<int,int> method2idx;

	set<int> invalidatedresults;

	xmlNodePtr addResult(const xmlNodePtr resultnode,
			     bool check_resultid=false);

        xmlNodePtr addResult(const SegmentationResult &result);


	SegmentationResultList 
	*readResults(result_search_type searchtype,
			    const std::string& name="",
			    const std::string& type="",
		     int methodid=-1,
		     bool wildcard=false,bool alsoinvalid=false) const;

	std::vector<xmlNodePtr> 
	accessResultNodes(result_search_type searchtype,
			  const std::string& name="",
			  const std::string& type="",
			  int methodid=-1,
			  bool wildcard=false,bool alsoinvalid=false);

	void invalidateResults(result_search_type searchtype,
			       const std::string& name="",
			       const std::string& type="",
			       int methodid=-1,
			       bool wildcard=false);

	xmlNodePtr outputXML() const;

	void parseXML(const xmlNodePtr n,bool check_resultid=false);

	void freeXML();

	void reset() {
	  freeXML();
	  resultnodes.clear();
	  name2idx.clear();
	  prefix2idx.clear();
	  type2idx.clear();
	  method2idx.clear();
	  invalidatedresults.clear();
	}

	void findmatchingresults(std::set<int> &tgt,
			    result_search_type searchtype,
			    const std::string& name,
			    const std::string& type,
			    int methodid,
				 bool wildcard,
				 bool alsoinvalid) const;

      }; // class _result_collection_type

      _description_type() {
	reset();
      }

      ~_description_type();

      void reset() {
	tifftoc = NULL;
	sequenceinfo.reset();
	fileinfo.reset();
	processinfolist.clear();
	methods.reset();
	fileResults.reset();
	frameResults.clear();
	attributes.clear();
	processedFrames.clear();
	frameSpecs.clear();
      }

      SequenceInfo sequenceinfo;

      xmlNodePtr tifftoc;

      FileInfo fileinfo;

      std::vector<ProcessInfo> processinfolist; 

      MethodList methods;

      _result_collection_type fileResults;
      
      std::map<int,_result_collection_type> frameResults;

      /// difference of results and attributes:
      /// 1) attributes can be removed
      /// 2) only single attribute with given identifier (name,type) allowed
      
      class _attribute_collection_type {
      public:

	_attribute_collection_type(){ reset(); }

	void reset(){
	  values.clear();
	  key2idx.clear();
	  removecount=0;
	}

	std::vector<std::string> values;
	std::multimap<std::string,int> key2idx; 

	int removecount;
	
	xmlNodePtr outputXML() const;

	void parseXML(const xmlNodePtr n);

	std::string getLastAttribute(const std::string &key ="") const;
	std::multimap<std::string,std::string>
	getAllAttributes(const std::string &key ="") const;

	void addAttribute(const std::string &key,const std::string &value);

	void replaceAttribute(const std::string &key,const std::string &value);

	void removeAttribute(const std::string &key="");

      }; // class _attribute_collection_type

      class _sequence_attributes_type {
	public:
	_attribute_collection_type sequenceAttributes;
	std::map<int,_attribute_collection_type> frameAttributes;
      }; // class _sequence_attributes_type

      std::map<std::string,_sequence_attributes_type> attributes;

      std::map<int,std::set<int> > processedFrames; 

      std::map<int,std::string> frameSpecs;

      char *outputXML() const;
      void outputXML(FILE *file) const;
      void _outputXML(FILE *file,char **buffer) const;

      char *outputSingleFrameXML(int f) const;

      bool readXML(const char *s);

      void tagFrameAsProcessed(const ChainedMethod *m, int frame) {
	processedFrames[frame].insert(methods.methodIndex(m));
      }

      xmlNodePtr addFileResult(const ChainedMethod *method,
		     const SegmentationResult& result) {
	SegmentationResult tmpres(result);
	tmpres.methodid=methods.methodIndex(method);
	return fileResults.addResult(tmpres);
      }

      xmlNodePtr addFrameResult(int frame,const ChainedMethod *method,
		     const SegmentationResult& result) {

	SegmentationResult tmpres(result);
	tmpres.methodid=methods.methodIndex(method);
	return frameResults[frame].addResult(tmpres);
      }

      SegmentationResultList 
      *readFrameResults(const ChainedMethod *m,
			int frame,result_search_type searchtype,
			const std::string& name="",
			const std::string& type="",
			bool wildcard=false,bool alsoinvalid=false) const {


	return (frameResults.count(frame))?
	  frameResults.find(frame)->
	  second.readResults(searchtype, name, type,
			     (m)?methods.methodIndex(m):methods.count()-1,
			     wildcard, alsoinvalid) :
	  new SegmentationResultList;
      }

      SegmentationResultList 
      *readFileResults(const ChainedMethod *m,result_search_type searchtype,
			const std::string& name="",
			const std::string& type="",
			bool wildcard=false,bool alsoinvalid=false) const {

	return
	  fileResults.readResults(searchtype,name,type,
				  (m)?methods.methodIndex(m):methods.count()-1,
				  wildcard,alsoinvalid);

      }

      std::vector<xmlNodePtr> 
      accessFrameResultNodes(const ChainedMethod *m,int frame,
			     result_search_type searchtype,
			     const std::string& name="",
			     const std::string& type="",
			     bool wildcard = false,bool alsoinvalid = false) {


	return (frameResults.count(frame))?
	  frameResults.find(frame)->second.
	  accessResultNodes(searchtype,name,type,
			    (m)?methods.methodIndex(m):methods.count()-1,
			    wildcard,alsoinvalid) :
	  std::vector<xmlNodePtr>();
      }

      std::vector<xmlNodePtr> 
      accessFileResultNodes(const ChainedMethod *m,
			    result_search_type searchtype,
			    const std::string& name="",
			    const std::string& type="",
			    bool wildcard = false, bool alsoinvalid = false) {


	return fileResults.
	  accessResultNodes(searchtype,name,type,
			    (m)?methods.methodIndex(m):methods.count()-1,
			    wildcard,alsoinvalid);
      }


      void invalidateFrameResults(const ChainedMethod *m,int frame,
				  result_search_type searchtype,
				  const std::string& name="",
				  const std::string& type="",
				  bool wildcard = false) {
	if (frameResults.count(frame))
	  frameResults[frame].
	    invalidateResults(searchtype,name,type,
			      (m)?methods.methodIndex(m):methods.count()-1,
			      wildcard);
      }

      void invalidateFileResults(const ChainedMethod *m,
				  result_search_type searchtype,
				  const std::string& name="",
				  const std::string& type="",
				  bool wildcard = false) {
	  fileResults.
	    invalidateResults(searchtype,name,type,
			      (m)?methods.methodIndex(m):methods.count()-1,
			      wildcard);
      }

      void augmentProcessInfo(const ProcessInfoProvider *p);

      void refreshFileInfo(const FileInfoProvider *p) {
	if(p)
	  fileinfo=p->getFileInfo();
	else
	  fileinfo.reset();
      }

    }; // class _description_type

  public:
    ///
    _description_type& Description() { return description; }

  private:
    /// data members

    ///
    image_sequence image_frames;

    ///
    image_sequence segment_frames;

    ///
    std::map<int,_frame_type> stored_images;

    ///
    std::map<int,_frame_type> stored_segments;

    ///
    imagefile *input_image_file;

    ///
    imagefile *input_segment_file;

    ///
    _description_type description;

  private:
    ///
    std::string zoningText;
  
    ///
    int current_frame;
  
    ///
    const ChainedMethod *first;
    
    ///
    const FileInfoProvider *fileInfoProvider;
    
    ///
    const ProcessInfoProvider *processInfoProvider;
    
    ///
    _filenaming naming;

    ///
    bool _is_prepared;

    bool _originates_from_tiff;

    // variables related to on-demand loading of frames

    ///
    bool use_ondemand; // should be always true

    //_on_demand_type onDemand;

    bool use_cacheing;

    // data structure for storing preprocessing results

    std::map<std::pair<int,std::string>, preprocessResult*> preprocess_results;

    vector<Region *> region_spec;

  }; // class segmentfile

} // namespace picsom  
  
///////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: font-lock 
// End:

#endif // _segmentfile_h_

