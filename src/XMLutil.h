// -*- C++ -*-  $Id: XMLutil.h,v 2.52 2016/10/25 11:45:20 jorma Exp $
// 
// Copyright 1998-2016 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#ifndef _PICSOM_XMLUTIL_H_
#define _PICSOM_XMLUTIL_H_

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <sstream>

using namespace std;

namespace picsom {
  static const string XMLutil_h_vcid =
    "@(#)$Id: XMLutil.h,v 2.52 2016/10/25 11:45:20 jorma Exp $";

  /**
     DOCUMENTATION MISSING
  */

  /// Make conversion from something to string.
  template <class T> inline string _tostr(T v) {
    stringstream ss;
    ss << v;
    return ss.str();
  }

  ///
  typedef const xmlChar* XMLS;

  ///
  typedef xmlChar* XMLncS;

  /// Create new empty XML document node.
  inline xmlNodePtr NewDocNode(xmlDocPtr doc, xmlNsPtr ns,
			       const string& name) {
    return xmlNewDocNode(doc, ns, (XMLS)name.c_str(), NULL);
  }

  /// Create new XML PI node.
#if defined(LIBXML_VERSION) && LIBXML_VERSION>=20617
  inline xmlNodePtr NewDocPiNode(xmlDocPtr doc,
				 const string& name,
				 const string& txt) {
    return xmlNewDocPI(doc, (XMLS)name.c_str(), (XMLS)txt.c_str());
  }
#else
  inline xmlNodePtr NewDocPiNode(xmlDocPtr, const string&, const string&) {
    return NULL;
  }
#endif // LIBXML_VERSION>=20617

  /// Make temporal conversion from int to XML string.
  inline XMLS Int2XMLS(int i) { return (XMLS)_tostr(i).c_str(); }

  /// Make temporal conversion from float to XML string.
  inline XMLS Float2XMLS(double d) { return (XMLS)_tostr(d).c_str(); }

  ///
  inline bool IsElement(xmlNodePtr p) {
    return p ? p->type==XML_ELEMENT_NODE : false;
  }

  /// A helper to extract a string from a XML node.
  inline string NodeName(xmlNodePtr p) {
    return p ? (const char*)p->name : "";
  }

  /// A helper to extract a string from a XML node.
  inline string NodeChildName(xmlNodePtr p) {
    return p && p->children ? (const char*)p->children->name : "";
  }

  /// A helper to extract a string from a XML node.
  inline string NodeContent(xmlNodePtr p) {
    return p && p->content ? (const char*)p->content : "";
  }

  /// A helper to extract a string from a XML node.
  inline string NodeChildContent(xmlNodePtr p) {
    return p && p->children && p->children->content ?
      (const char*)p->children->content : "";
  }

  /// Adds a simple comment tag in XML document.
  inline xmlNodePtr AddComment(xmlNodePtr p, const string& c) {
    xmlNodePtr q = xmlNewComment((XMLS)c.c_str());
    return xmlAddChild(p, q);
  }

  /// Adds a simple <key>value/key> or <key/> entry in XML document.
  inline xmlNodePtr AddTag(xmlNodePtr p, xmlNsPtr n,
			   const string& k, const string& v = "",
			   bool html = false) {
    if (html)
      return xmlNewChild(p, n, (XMLS)k.c_str(),
			 XMLS(v!="" ? v.c_str() : NULL));
    else
      return xmlNewTextChild(p, n, (XMLS)k.c_str(),
			     XMLS(v!="" ? v.c_str() : NULL));
  }

  /// Adds a simple <key>value</key> entry in XML document.
  template <class T> inline xmlNodePtr AddTag(xmlNodePtr p, xmlNsPtr n,
					      const string& k, const T& v) {
    stringstream s;
    s << v;
    return AddTag(p, n, k, s.str());
  }
  
  /// Adds simple text in the end of given element.
  inline void AddText(xmlNodePtr p, const string& txt) {
    xmlAddChild(p, xmlNewText((XMLS)txt.c_str()));
  }

  /// Adds a simple XML property in an element.
  template <class T> inline void SetProperty(xmlNodePtr p, const string& k,
					     const T& v) {
    stringstream s;
    s << v;
    xmlSetProp(p, (XMLS)k.c_str(), (XMLS)s.str().c_str());
  }

  /// Reads a simple XML property from an element.
  inline string GetProperty(xmlNodePtr node, const string& k) {
    XMLncS v = xmlGetProp(node, (XMLS)k.c_str());
    string ret = v ? (const char*)v : "";
    xmlFree(v);
    return ret;
  }

  /// Extracts content of an element tag.
  string GetXMLkeyValue(xmlNodePtr n, const string& k);

  /// Returns content of the first child if it exists.
  inline string GetXMLchildContent(xmlNodePtr n) {
    return n && n->children && n->children->content ?
      (const char*) n->children->content : "";
  }

  /// Searches for a subnode with given name.
  inline xmlNodePtr FindXMLchildElem(xmlNodePtr ptr, const string& name) {
    if (ptr) {
      ptr = ptr->children;
      while (ptr)
	if (ptr->type==XML_ELEMENT_NODE && NodeName(ptr)==name)
	  return ptr;
	else
	  ptr = ptr->next;
    }
    return NULL;
  }
  
  /// A helper to extract a string from a XML node.
  inline string AttrName(xmlAttrPtr p) {
    return p ? (const char*)p->name : "";
  }

  /// A helper to extract a string from a XML node.
  inline string AttrContent(xmlAttrPtr p) {
    return p && p->children && p->children->content ?
      (const char*)p->children->content : "";
  }

  /////////////////////////////////////////////////////////////////////////////

  ///
  class XmlDom {
  public:
    ///
    XmlDom(xmlDocPtr d = NULL, xmlNsPtr s = NULL, xmlNodePtr n = NULL) :
      doc(d), ns(s), node(n) {}
    
    ///
    XmlDom(xmlNodePtr n, xmlNsPtr s = NULL) : ns(s), node(n) {
      doc = n ? n->doc : NULL;
    }
    
    ///
    operator bool() const { return doc && node; }
    
    ///
    static XmlDom ReadDoc(const string& fn) {
      xmlDocPtr doc = xmlReadFile(fn.c_str(),NULL,0);
      return XmlDom(doc, NULL, NULL);
    }

    ///
    static XmlDom Doc(const string& name = "",
		      const string& pub = "",
		      const string& ext = "");
		      
    ///
    static XmlDom Parse(const string& file) {
      xmlDocPtr doc = xmlParseFile(file.c_str());
      xmlNsPtr ns = NULL; // obs!?
      return XmlDom(doc, ns, NULL);
    }

    ///
    static XmlDom FromString(const string& str) {
      xmlDocPtr doc = xmlParseMemory(str.c_str(), str.size());
      xmlNsPtr ns = NULL; // obs!?
      return XmlDom(doc, ns, NULL);
    }
    
    ///
    static XmlDom Copy(xmlDocPtr src) {
      xmlDocPtr c = xmlCopyDoc(src, 1);
      return XmlDom(c, NULL, NULL);  // obs!? shoud ns be something?
    }

    ///
    bool DocOK() const { return doc; }

    ///
    bool NodeOK() const { return node; }

    ///
    void DeleteDocOld() const { if (doc) xmlFreeDoc(doc); }

    ///
    void DeleteDoc() {
      if (doc)
        xmlFreeDoc(doc);
      *this = XmlDom();
    }

    ///
    XmlDom Ns(const string& ref, const string& nsn) const {
      xmlNsPtr new_ns = !node ? NULL :
	xmlNewNs(node, (XMLS)(ref!=""?ref.c_str():NULL),
		 (XMLS)(nsn!=""?nsn.c_str():NULL));
      if (node && new_ns)
	xmlSetNs(node, new_ns);
      return XmlDom(doc, new_ns, node);
    }

    ///
    XmlDom Root() const {
      xmlNodePtr root = doc ? xmlDocGetRootElement(doc) : NULL;
      return XmlDom(doc, root ? root->ns : NULL, root); 
      // return XmlDom(doc, ns, root);
    }

    ///
    XmlDom Root(const string& s,
		const string& ref = "", const string& nsn = "") const {
      xmlNodePtr n = NewDocNode(doc, ns, s);
      xmlDocSetRootElement(doc, n);
      XmlDom ret(doc, ns, n);
      return ref=="" && nsn=="" ? ret : ret.Ns(ref, nsn);
    }

    ///
    XmlDom PiNode(const string& s, const string& c) const {
      xmlNodePtr pi = NewDocPiNode(doc, s, c);
      xmlAddChild((xmlNodePtr)doc, pi);
      return XmlDom(doc, ns, pi);
    }

    ///
    bool IsElement() const {
      return node ? node->type==XML_ELEMENT_NODE : false;
    }

    ///
    bool IsText() const { return node ? node->type==XML_TEXT_NODE : false; }

    ///
    bool IsComment() const {
      return node ? node->type==XML_COMMENT_NODE : false;
    }

    ///
    XmlDom FirstChild() const {
      return XmlDom(doc, ns, node ? node->children : NULL);
    }

    ///
    XmlDom FirstElementChild(const string& n = "") const {
      XmlDom c = XmlDom(doc, ns, node ? node->children : NULL);
      while (c && (!c.IsElement() || (n!="" && n!=c.NodeName())))
        c = c.Next();
      return c;
    }

    ///
    XmlDom Next() const {
      return XmlDom(doc, ns, node ? node->next : NULL);
    }
   
    ///
    XmlDom NextElement(const string& n = "") const {
      XmlDom c = XmlDom(doc, ns, node ? node->next : NULL);
      while (c && (!c.IsElement() || (n!="" && n!=c.NodeName())))
        c = c.Next();
      return c;
    }
  
    ///
    XmlDom LastElementChild(const string& n = "") const {
      XmlDom c = FirstElementChild(n), d;
      for (; c; c=d) {
	d = c.NextElement(n);
	if (!d)
	  break;
      }
      return c;
    }

    ///
    string NodeName() const {
      return node ? (const char*)node->name : "";
    }

    ///
    string Property(const string& n) const {
      XMLncS v = node ? xmlGetProp(node, (XMLS)n.c_str()) : NULL;
      string ret = v ? (const char*)v : "";
      xmlFree(v);
      return ret;
    }

    ///
    int PropertyInt(const string& n) const {
      XMLncS v = node ? xmlGetProp(node, (XMLS)n.c_str()) : NULL;
      string s = v ? (const char*)v : "";
      xmlFree(v);
      return atoi(s.c_str());
    }

    ///
    list<pair<string,string> > Properties() const {
      list<pair<string,string> > res;
      for (_xmlAttr *p = node->properties; p; p=p->next) {
	string name = (char*)p->name, val;
	if (p->children && p->children->content)
	  val = (char*)p->children->content;

	res.push_back(make_pair(name, val));
      }

      return res;
    }

    ///
    map<string,string> PropertyMap() const {
      list<pair<string,string> > res = Properties();
      return map<string,string>(res.begin(), res.end());
    }

    /// Adds a simple <key/> entry in XML document.
    XmlDom Element(const string& k) const {
      return Element(k, string());
    }

    /// Adds a simple <key/> entry in XML document.
    XmlDom Element(const string& k, const char *v) const {
      return Element(k, string(v?v:""));
    }

    ///
    template <class T> XmlDom Element(const string& k, const T& v,
                                      size_t p = 0, bool html = false) const {
      stringstream ss;
      ss << v;
      string s = ss.str();

      if (p) {
        T x = T();
        ss >> x;
        if (x!=v) {
          stringstream ss2;
          ss2 << scientific;
          ss2.precision(p);
          ss2 << v;
          s = ss2.str();
        }
      }

      xmlNodePtr n = AddTag(node, ns, k, s, html);
      return XmlDom(doc, ns, n);
    }

    ///
    void MoveBefore(const XmlDom& r) {
      xmlAddPrevSibling(r.node, node);
    }

    ///
    void MoveAfter(const XmlDom& r) {
      xmlAddNextSibling(r.node, node);
    }

    /// Copies and adds a subtree.
    XmlDom CopyElement(xmlNodePtr e, bool r = true) {
      if (!e)
        return XmlDom();
      xmlNodePtr f = xmlCopyNode(e, r);
      xmlSetNs(f, ns);
      return XmlDom(doc, ns, f);
    }

    /// Copies and adds a subtree.
    XmlDom CopyElement(const XmlDom& e, bool r = true) {
      return CopyElement(e.node, r);
    }

    /// Adds a simple XML property in an element.
    template <class T> void Prop(const string& k, const T& v) {
      stringstream s;
      s << v;
      xmlSetProp(node, (XMLS)k.c_str(), (XMLS)s.str().c_str());
    }

    /// Converts XML to string.
    string Stringify(bool pretty = false) const;

    /// Writes the XML tree in a stream.
    bool Write(ostream& os, bool pretty = false) const {
      os << Stringify(pretty);
      return os.good();
    }

    /// Writes the XML tree in a file.
    bool Write(const string& f, bool pretty = false, bool gzip = false) const;

    /// Finds the named child (non-recursively).
    XmlDom FindChild(const string& name, size_t skip = 0) const;
    
    bool SchemaValidate(const string& schema_filename);

    /// XPath alike routine
    XmlDom FindPath(const string& path, bool verbose) const;

    /// XPath alike routine
    string FindContent(const string& path, bool verbose) const {
      return FindPath(path, verbose).FirstChildContent();
    }

    /// Adds a copy of node.
    XmlDom AddCopy(const XmlDom& xml) const;
  
    /// Content of node if any.
    string Content() const {
      return node && node->content ? (const char*)node->content : "";
    }

    /// Adds text as content of node.
    bool Content(const string& txt) const {
      if (!node)
	return false;
      xmlNodeAddContent(node, (XMLS)txt.c_str());
      return true;
    }

    /// Content of node if any.
    string FirstChildContent() const {
      return node && node->children && node->children->content ?
	(const char*)node->children->content : "";
    }
    
    /// String dump of node.
    string Dump() const;

    ///
    template <class T> T Value() const {
      stringstream ss;
      ss << FirstChildContent();
      T v;
      ss >> v;
      return v;
    }

    ///
    template <class T> vector<T> VectorValue() const {
      stringstream ssn, ssv;
      ssn << Property("size");
      size_t n = 0;
      ssn >> n;
      ssv << FirstChildContent();
      vector<T> v(n);
      for (size_t i=0; i<n; i++)
        ssv >> v[i];
      return v;
    }

    ///
    static string UnEscapePercentHexHex(const string&);

    ///
    XmlDom RemoveBlanks();

    ///
    void RemoveBlanks_inner(xmlNodePtr n);

    ///
    void AddSOAP_ENC_arrayType(bool);

    ///
    xmlDocPtr  doc;

    ///
    xmlNsPtr   ns;

    ///
    xmlNodePtr node;

  };  // class XmlDom

} // namespace picsom

#endif // _PICSOM_XMLUTIL_H_

