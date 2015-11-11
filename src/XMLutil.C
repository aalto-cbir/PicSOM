// -*- C++ -*-  $Id: XMLutil.C,v 2.5 2012/10/22 12:35:27 jorma Exp $
// 
// Copyright 1998-2008 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <XMLutil.h>

#include <fstream>
#include <iostream>
#include <gzstream.h>

namespace picsom {
  static const string XMLutil_C_vcid =
    "@(#)$Id: XMLutil.C,v 2.5 2012/10/22 12:35:27 jorma Exp $";

  //===========================================================================

  string GetXMLkeyValue(xmlNodePtr node, const string& key) {
    for (;;) {
      while (node && node->type!=XML_ELEMENT_NODE)
        node = node->next;
      if (!node)
        return "";

      if (NodeName(node)==key && node->children && node->children->content)
        return NodeChildContent(node);

      node = node->next;
    }
  }

  //===========================================================================
  //===========================================================================

  XmlDom XmlDom::Doc(const string& name, const string& pub,
                            const string& ext) {
    // string pub = "-//xxx//DTD yyy 0.0//EN";
    // string ext = "http://nowhere/xxx.dtd";
    xmlDocPtr doc = xmlNewDoc((XMLS)"1.0");
    if (name!="") {
      xmlDtdPtr dtd = xmlNewDtd(NULL,
                                (XMLS)name.c_str(),
                                (XMLS)(pub=="" ? NULL : pub.c_str()),
                                (XMLS)(ext=="" ? NULL : ext.c_str()));
      xmlAddChild((xmlNodePtr)doc, (xmlNodePtr)dtd);
    }
    return XmlDom(doc, NULL, NULL);
  }
  
  //===========================================================================

  string XmlDom::Stringify(bool pretty) const {
    int tree_was = xmlIndentTreeOutput;
    xmlIndentTreeOutput = true;

    xmlChar *dump = NULL;
    int dumplen;
    const char *enc = NULL; // "ISO-8859-1"

    if (pretty)
      xmlDocDumpFormatMemory(doc, &dump, &dumplen, true);
    else
      xmlDocDumpMemoryEnc(doc, &dump, &dumplen, enc);

    xmlIndentTreeOutput = tree_was;

    string ret = dump ? (char*)dump : "";
    xmlFree(dump);
    
    return ret;
  }
  
  //===========================================================================
  
  bool XmlDom::Write(const string& f, bool pretty, bool gzip) const {
    if (gzip) {
      ogzstream os(f.c_str());
      os << Stringify(pretty);
      return os.good();
    }
    ofstream os(f.c_str());
    return Write(os, pretty);
/*  // This *should* work... but doesn't...

    const char *enc = NULL; // "ISO-8859-1", "UTF-8" ?

    // 0 (uncompressed) to 9 (max compression)
    xmlSetDocCompressMode(doc, gzip?9:0);
    
    if (pretty) xmlKeepBlanksDefault(0);
    int ret = xmlSaveFormatFileEnc(f.c_str(), doc, enc, pretty?1:0);
//    int ret = xmlSaveFileEnc(f.c_str(), doc, enc);
    return ret>0; // false if 0 bytes written or error (ret==-1)
*/
  }
    
  //===========================================================================

  XmlDom XmlDom::FindChild(const string& name, size_t skip) const {
    xmlNodePtr ptr = node ? node->children : NULL;
    while (ptr)
      if (ptr->type==XML_ELEMENT_NODE && ptr->name &&
          name==(const char*)ptr->name && skip--==0)
        return XmlDom(doc, ns, ptr);
      else
        ptr = ptr->next;

    return XmlDom();
  }

  //===========================================================================
    
  bool XmlDom::SchemaValidate(const string& schema_filename) {
    xmlSchemaParserCtxtPtr pcontext = 
      xmlSchemaNewParserCtxt(schema_filename.c_str());
    if (!pcontext) {
      cerr << "Could not create XML Schemas parser context!" << endl;
      return false;
    }

    xmlSchemaPtr schema = xmlSchemaParse(pcontext);
    if (!schema) {
      xmlSchemaFreeParserCtxt(pcontext);
      cerr << "Could not build internal XML Schema structure!" << endl;
      return false;
    }

    xmlSchemaValidCtxtPtr vcontext = xmlSchemaNewValidCtxt(schema);
    if (!vcontext) {
      xmlSchemaFreeParserCtxt(pcontext);
      xmlSchemaFree(schema);
      cerr << "Could not create XML Schemas validation context!" << endl;
      return false;
    }

    int val = xmlSchemaValidateDoc(vcontext, doc);

    xmlSchemaFreeParserCtxt(pcontext);
    xmlSchemaFreeValidCtxt(vcontext);
    xmlSchemaFree(schema);
    
    return val==0;
  }

  //===========================================================================

  XmlDom XmlDom::FindPath(const string& path, bool verbose) const {
    bool r = path[0]=='/';
    XmlDom e = r ? Root() : *this, err;

    for (size_t p = r; p<path.size(); ) {
      string n = path.substr(p);
      size_t l = n.find('/');
      if (l!=string::npos) {
        n.erase(l);
        p += l+1;
      } else
        p = string::npos;

      if (r) {
        if (e.NodeName()!=n) {
          if (verbose)
            cerr << "XmlDom::FindPath(" << path << ") failed with [/"+
              n+"]" << endl;
          return err;
        }
        r = false;
        continue;
      }

      XmlDom c = e.FindChild(n);
      if (!c.IsElement()) {
        if (verbose)
          cerr << "XmlDom::FindPath(" << path << ") failed with ["+n+"]"
                << endl;
        return err;
      }
      e = c;
    }

    return e;
  }

  //===========================================================================
    
  XmlDom XmlDom::AddCopy(const XmlDom& xml) const {
    if (!xml.node || !doc || !node)
      return XmlDom();

    xmlNodePtr c = xmlDocCopyNode(xml.node, doc, 1);
    xmlAddChild(node, c);

    return XmlDom(doc, ns, c);
  }
    
  //===========================================================================
    
  string XmlDom::Dump() const {
    if (!doc || !node)
      return "";

    xmlBufferPtr buf = xmlBufferCreate();
    /*int n =*/ xmlNodeDump(buf, doc, node, 0, 0);
    string ret((const char*)xmlBufferContent(buf));
    xmlBufferFree(buf);

    return ret;
  }

  //===========================================================================

  string XmlDom::UnEscapePercentHexHex(const string& s) {
    string r = s;
    size_t q = 0;
    for (;;) {
      size_t p = r.find('%', q);
      if (p==string::npos || r.size()<p+3)
	break;
      string hex = r.substr(p+1, 2);
      if (hex.find_first_not_of("0123456789ABCDEFabcdef")==string::npos) {
	int v = 0;
	for (size_t i=0; i<2; i++) {
	  int x = hex[i]-'0';
	  if (x>9)
	    x -= 'A'-'0'-10;
	  if (x>15)
	    x -= 'a'-'A';
	  v = 16*v+x;
	}
	r.replace(p, 3, string(1, char(v)));
      }
      q = p+1;
    }
    return r;
  }

  //===========================================================================

  XmlDom XmlDom::RemoveBlanks() {
    if (!doc)
      return XmlDom();

    xmlNodePtr n = node ? node : xmlDocGetRootElement(doc);
    RemoveBlanks_inner(n);

    return *this;
  }

  //===========================================================================

  void XmlDom::RemoveBlanks_inner(xmlNodePtr n) {
    for (;;) {
      bool found = false;
      xmlNodePtr c = n->children;
      while (c) {
        if (xmlIsBlankNode(c)) {
          xmlUnlinkNode(c);
          xmlFreeNode(c);
          found = true;
          break;
        }
        if (c->type==XML_ELEMENT_NODE)
          RemoveBlanks_inner(c);

        c = c->next;
      }
      if (!found)
        break;
    }
  }

  //===========================================================================

  void XmlDom::AddSOAP_ENC_arrayType(bool ns) {
    if (ns)
      Prop("xmlns:SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/");
    Prop("xsi:type", "SOAP-ENC:Array");
    Prop("SOAP-ENC:arrayType", "xsd:anyType[]");
  }

  //===========================================================================

} // namespace picsom

