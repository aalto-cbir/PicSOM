// -*- C++ -*-  $Id: NGram.C,v 1.18 2017/04/04 09:58:00 jormal Exp $
// 
// Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <NGram.h>
#ifdef HAVE_GCRYPT_H

namespace picsom {
  static const char *vcid =
    "$Id: NGram.C,v 1.18 2017/04/04 09:58:00 jormal Exp $";

static NGram list_entry(true);

//=============================================================================

string NGram::Version() const {
  return vcid;
}

//=============================================================================

int NGram::printMethodOptions(ostream &str) const {
  /// obs! dunno yet

  // str << "NGram options ('-o option=value'):" << endl;
  str << "ngram-n=[positive integer] (default 3) :"
      << " the number of characters per ngram"
      << endl
      << "ngram-vector-length=[positive integer*256] (default 1024) :"
      << " feature vector length" 
      << endl
      << "ngram-hash-algorithm=[sha1|md5|crc32] (default sha1) :"
      << " the used hash algorithm" 
      << endl
      << "ngram-mode=[word|char] (default char) :"
      << " the used ngram mode"
      << endl
      << "ngram-normalize=[true|false] (default true) :"
      << " normalization by total count" << endl;

  return TextBased::printMethodOptions(str);
}

//=============================================================================

bool NGram::ProcessOptionsAndRemove(list<string> &l) {
   
  for (list<string>::iterator i = l.begin(); i!=l.end(); ) {
    if (FileVerbose())
      cout << "  NGram : <" << *i << ">" << endl;
    
    string key, value;
    SplitOptionString(*i, key, value); 
    
    if (key=="ngram-n") {
      int n = (int) atoi(value.c_str());
      i = l.erase(i);
      if(n <= 0)
	throw "ngram-n=[positive integer]";
      ngram_n = (unsigned int) n;
      continue;
    }

    if (key=="ngram-vector-length") {
      int n = (int) atoi(value.c_str());
      i = l.erase(i);
      if(n <= 0 || n % 256 != 0)
	throw "ngram-vector-length=[positive integer*256]";
      ngram_vector_length = (unsigned int) n;
      continue;
    }

    else if (key=="ngram-separating-space-count") {
      int n = (int) atoi(value.c_str());
      i = l.erase(i);
      if(n < 0)
	throw "ngram-separating-space-count=[non-negative integer]";
      ngram_space_count = n;
      continue;
    }

    if (key=="ngram-skip-space-count") {
      int n = (int) atoi(value.c_str());
      i = l.erase(i);
      if(n <= 0)
	throw "ngram-skip-space-count=[positive integer]";
      ngram_skip_space_count = n;
      continue;
    }

    if (key=="ngram-hash-algorithm") {
      i = l.erase(i);
      if(value == "sha1")
	ngram_hash_algorithm = GCRY_MD_SHA1;
      else if(value == "md5")
	ngram_hash_algorithm = GCRY_MD_MD5;
      else if(value == "crc32")
	ngram_hash_algorithm = GCRY_MD_CRC32;
      else
	throw "ngram-hash-algorithm=[sha1|md5|crc32]";
      continue;
    }

    if (key=="ngram-mode") {
      i = l.erase(i);
      if(value == "word")
	ngram_mode = NGRAM_MODE_WORD;
      else if(value == "char")
	ngram_mode = NGRAM_MODE_CHAR;
      else
	throw "ngram-mode=[word|char]";
      continue;
    }

    if (key=="ngram-normalize") {
      i = l.erase(i);
      if (value == "true")
	ngram_normalize = true;
      else if (value == "false")
	ngram_normalize = false;
      else
	throw "ngram-normalize=[true|false]";
      continue;
    }

    i++;
  }

  return TextBased::ProcessOptionsAndRemove(l);
}

//=============================================================================

NGram::NGramData::shadigest_vect_t
NGram::NGramData::hashsum(const string& st, gcry_md_algos algo) {
  size_t hash_len = gcry_md_get_algo_dlen(algo);
  shadigest_t *buf = (shadigest_t*) malloc(hash_len);
  gcry_md_hash_buffer(algo, buf, st.c_str(), st.length());
  
  shadigest_vect_t ret;
  for (unsigned int i=0; i<hash_len; i++)
    ret.push_back(buf[i]);
  
  free(buf);
  
  return ret;
}

//=============================================================================

NGram::NGramData::shadigest_vect_t
NGram::NGramData::hashsum(const string& st) const {
  return hashsum(st, (gcry_md_algos)ngram_hash_algorithm);
}

//=============================================================================

NGram::vectortype 
NGram::NGramData::CleanUpWhiteSpace(const vectortype &original, 
				    int between_space_count) const {
  vectortype tempvec;
  
  // insert space character in the beginning and end of the vector:
  if(original.size() > 0) 
    tempvec.insert(tempvec.begin(), between_space_count, (datatype)' ');
  else
    return tempvec;
  
  // Make a copy of the character keycode vector.
  // Replace any number of successive white-space characters with 
  // a predefined number of spaces:
  for(unsigned int i = 0; i < original.size(); i++) {
    if(original[i] != (datatype)' ' && 
       original[i] != (datatype)'\t' && 
       original[i] != (datatype)'\n')
      tempvec.push_back(original[i]);
    else {
      // skip to the whitespace character that is before the next 
      // non-whitespace-char:
      while(i < original.size() && 
	    (original[i] == (datatype)' ' || 
	     original[i] == (datatype)'\t' || 
	     original[i] == (datatype)'\n')) 
	i++;
      i--;
      
      // after skipping the whitespaces, add space characters to the copy
      tempvec.insert(tempvec.end(), between_space_count, (datatype)' ');
    }
  }
  
  // if the text doesn't end in a space, add some spaces:
  if(tempvec[tempvec.size()-1] != (datatype)' ')
    tempvec.insert(tempvec.end(), between_space_count, (datatype)' ');
  
  return tempvec;
}


//=============================================================================


/// Does the given string contain n successive spaces.
bool 
NGram::NGramData::ContainsNSuccessiveSpaces(char* buf, 
						    unsigned int n) const {
  char* spaces = new char[n+1];
  for (unsigned int i=0; i < n; i++)
    spaces[i] = ' ';
  spaces[n] = '\0';
  bool ret = (strstr(buf,spaces) != NULL);
  delete[] spaces;
  return ret;
}



//=============================================================================


/// Calculates the character NGram feature vector.
Feature::featureVector NGram::NGramData::CharacterNGramResult() const {
  featureVector v(ngram_vector_length);
  // charactervec contains the character codes of the text (they are 
  // automatically loaded by the methods specified in classes 
  // TextBased and TextBasedData). 
  vectortype tempvec = CleanUpWhiteSpace(charactervec, ngram_space_count);
  if (tempvec.size() == 0)
    return v;
  
  // avoid overflowing:
  unsigned int temp_ngram_n;
  if(tempvec.size() < ngram_n)
    temp_ngram_n = tempvec.size();
  else
    temp_ngram_n = ngram_n;
  
  char *text_window = new char[temp_ngram_n + 1];
  
  int tot_n = 0;
  
  // initialize the text window (temp_ngram_n-1 first characters -- 
  // the text
  // window is shifted forward in the beginning of the loop)
  for(unsigned int j=0; j < temp_ngram_n-1; j++)
    text_window[j+1] = tempvec[j];
  text_window[temp_ngram_n] = '\0'; // string terminator
  
  // move the text window from the beginning to the end of the string
  for (unsigned int i=temp_ngram_n-1; i<tempvec.size(); i++) {
    // move the window one character onwards
    for(unsigned int j=1; j < temp_ngram_n; j++)
      text_window[j-1] = text_window[j];
    text_window[temp_ngram_n-1] = tempvec[i];
    
    if(ContainsNSuccessiveSpaces(text_window, ngram_skip_space_count)) 
      continue; 
    
    if (PixelVerbose())
      cout << "NGram::CharacterNGramResult() [" << text_window << "]"
	   << endl;
    
    // calculate hash sum, and use the first bytes to determine the
    // vector element indices corresponding to this ngram. The number
    // of bytes used depends on the feature vector length.
    shadigest_vect_t sha = hashsum((char*)text_window);
    
    for(unsigned int j=0; j<ngram_vector_length/256; j++) {
      unsigned int vec_index = j*256 + (unsigned int) sha[j];
      v[vec_index]++;
    }
    
    tot_n++;
  } 
  delete[] text_window;
  
  if (ngram_normalize) {
    if (LabelVerbose())
      cout << "NGram::CharacterNGramResult() normalizing with " << tot_n
	   << endl;
    DivideFeatureVector(v, tot_n);
  }
  
  return v;
}


//=============================================================================


/// A function that calculates results for word NGrams.
Feature::featureVector NGram::NGramData::WordNGramResult() const {
  if (LabelVerbose())
    cout << "NGramData::WordNGramResult()" << endl;

  featureVector v(ngram_vector_length);
  
  vector<string> wordvec = TextData().get_words_as_string();
  
  unsigned int temp_ngram_n = ngram_n;
  if (wordvec.size() < ngram_n)
    temp_ngram_n = wordvec.size();
  
  int tot_n = 0;
  for(unsigned int i=0; i <= wordvec.size()-temp_ngram_n; i++) {
    string text = "";
    for (unsigned int j=0; j<temp_ngram_n; j++) 
      text += (j ? " " : "") + wordvec[i+j];
    
    if (PixelVerbose())
      cout << "NGramData::WordNGramResult() [" << text << "]" << endl;
    
    if (text=="")
      continue;

    shadigest_vect_t sha = hashsum(text);
    
    for(unsigned int j=0; j<ngram_vector_length/256; j++) {
      unsigned int vec_index = j*256 + (unsigned int) sha[j];
      v[vec_index]++;
    }
    
    tot_n++;
  }

  if (ngram_normalize) {
    if (LabelVerbose())
      cout << "NGramData::WordNGramResult() normalizing with " << tot_n
	   << endl;
    if (tot_n)
      DivideFeatureVector(v, tot_n);
  }
  
  return v;
}

//=============================================================================

} // namespace picsom

#endif // HAVE_GCRYPT_H

// Local Variables:
// mode: font-lock
// End:

