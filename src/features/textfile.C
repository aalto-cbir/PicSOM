// -*- C++ -*-  $Id: textfile.C,v 1.15 2016/02/08 20:32:31 jorma Exp $
// 
// Copyright 1998-2009 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 TKK, FINLAND
// 

#include <textfile.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif // HAVE_STRING_H

#include <regex.h>
#include <iconv.h>

#ifndef ICONV_CONST
#define ICONV_CONST const
#endif // ICONV_CONST

namespace picsom {

// void textfile::get_text(xmlNodePtr node,xmlChar *rstr) {
// 	xmlNodePtr cur = NULL;

// 	for (cur = node; cur; cur = cur->next) {
// 		if (cur->type == XML_TEXT_NODE && xmlStrlen(cur->content) > 0)
// 			xmlStrcat(rstr,cur->content);
// 		get_text(cur->children,rstr);
// 	}
// }


// char *textfile::remove_xml(const char *cstr) {
// 	xmlChar *xstr = (xmlChar *)cstr;

// 	xmlDocPtr doc = xmlParseDoc(xstr);
// 	xmlNodePtr root = xmlDocGetRootElement(doc);

// 	xmlChar *rstr = new xmlChar[100000];
// 	rstr[0] = '\0';
// 	get_text(root,rstr);

// 	xmlFreeDoc(doc);

// 	return (char *)rstr;
// }

  bool textfile::remove_html(string& str) {
    string tstr;
    regex_t re;
    const int SUBS = 1;
    regmatch_t pmatch[SUBS+1];
    
    const char *pattern = "(<[^<>]+>[\r\t\n]*)";
    if (regcomp(&re, pattern, REG_EXTENDED) != 0) {
      cerr << "textfile::remove_html: bad regex pattern" << endl;
      return false;
    }
    int status = regexec(&re, str.c_str(), (size_t) SUBS+1, &pmatch[0], 0);
    
    while (status == 0) {
      tstr = str.substr(0,size_t(pmatch[1].rm_so))
	+str.substr(size_t(pmatch[1].rm_eo));
      str = tstr;
      status = regexec(&re, str.c_str(), (size_t) SUBS+1, &pmatch[0], 0);
    }
    return true;
  }
  
  
  void textfile::readfile(const std::string& f, bool only_original, 
			  file_type ftype) 
  {
    _filename    = f;
    
    ifstream filestr(f.c_str());
    if(!filestr) {
      cerr << "textfile::readfile() : Couldn't open file <" << f << ">" <<endl;
      _filename = "";
      _data = textdata();
      return;
    }
    
    // get length of file:
    filestr.seekg (0, ios::end);
    int length = filestr.tellg();
    filestr.seekg (0, ios::beg);
    
    char* text = (char*) malloc((length+1)*sizeof(char));
    
    // The text in wide format has 4 bytes for each character. Value length*4
    // is the maximum number of bytes the converted string can contain.
    // If the text contains utf-8 coded multi-byte characters, multiple
    // chars in the original text can belong to the same character, and
    // thus we won't need all of this allocated memory. However, we can't
    // know the number of utf-8 coded multibyte characters in advance, and
    // we'll have to allocate memory according to the "worst-case" scenario.
    // If the text contains only US-ASCII (7-bit) characters, all of this
    // memory is used. The more non-ASCII characters the UTF-8 text 
    // contains, the more of this allocated memory is left unused.
    char* utext = (char*) malloc(4*(length+1)*sizeof(char));
    
    textdata d;
    
    filestr.read((char*)text, length);
    filestr.close();
    text[length] = 0;
    
    string tmp = text;

    switch (ftype) {
    case file_html:
      if (!remove_html(tmp)) {
	_filename = "";
	_data = textdata();
	return;
      }
      strcpy(text, tmp.c_str());
      break;
    case file_text:
      // do nothing
      break;
    default:
      cerr << "Warning: file type not implemented!" << endl;
    }
    
    // store also the original string (without char set conversion)
    d.original_string(string(text));
    d.only_original(only_original);
    
    // check if we want to do the wchar conversion, or do we just save the
    // text string as it is:
    if (!only_original && length > 0) {
      // First we'll assume that the given text is in UTF8-format, and we'll
      // try to convert it to WCHAR_T-format (note that 7bit US-ASCII text is
      // UTF8-compatible):

      iconv_t cd = 0;
      cd = iconv_open("WCHAR_T", "UTF-8");

      char *textptr  = text;
      char *utextptr = utext;
      size_t inbytesleft = length+1;
      size_t outbytesleft = (length+1)*4;
      
      int convi = iconv(cd, &textptr, &inbytesleft, &utextptr, &outbytesleft);
      
      if (convi == -1) {
	// If the text contained non-UTF8-compatible characters, we'll assume
	// that the text is in 8bit LATIN1-format:
	cd = iconv_open("WCHAR_T", "LATIN1");
	convi = iconv(cd, (ICONV_CONST char**)&text, &inbytesleft,
		      &utextptr, &outbytesleft);
      }
      
      // Throw error if the conversion failed (do we need to support some
      // other character sets besides UTF8/US-ASCII and LATIN1?)
      if (convi == -1)
	throw 
	  "textfile::readfile() : unsupported charset (tried UTF-8, Latin1)";
      
      // At this point, the memory pointed by utext contains the text in
      // wchar_t format (four bytes / character). Make a type change from
      // char* to wchar_t*:
      wchar_t* wtext = (wchar_t*) utext;
      
      // Insert the wide characters to the textdata object:
      wchar_t* wchar = wtext;
      for(unsigned int i=0; i<wcslen(wchar); i++) 
	d.push(wchar[i]);

      iconv_close(cd);
    }
    
    free(text);
    free(utext);
    
    _data = d;
  }
}
