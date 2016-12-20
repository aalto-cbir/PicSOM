// $Id: PicSOMLucene3.java,v 1.3 2016/01/07 10:47:02 jorma Exp $	

/**
*/

import org.apache.lucene.LucenePackage;
import org.apache.lucene.util.Version;
import org.apache.lucene.index.Term;
import org.apache.lucene.index.IndexWriter;
import org.apache.lucene.index.IndexWriterConfig;
import org.apache.lucene.index.IndexWriterConfig.OpenMode;
import org.apache.lucene.document.*;
import org.apache.lucene.store.FSDirectory;

import org.apache.lucene.analysis.Analyzer;

import java.util.Properties;
import java.util.Enumeration;
import java.util.Date;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.FileReader;

import org.apache.lucene.index.FilterIndexReader;
import org.apache.lucene.index.IndexReader;
import org.apache.lucene.queryParser.QueryParser;
import org.apache.lucene.search.IndexSearcher;
import org.apache.lucene.search.Query;
import org.apache.lucene.search.TopDocs;
import org.apache.lucene.search.ScoreDoc;
//import org.apache.lucene.wordnet.SynExpand;
import java.io.InputStreamReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.List;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Set;
import org.apache.lucene.analysis.standard.StandardAnalyzer;
import org.apache.lucene.analysis.standard.ClassicAnalyzer;
import org.apache.lucene.analysis.en.EnglishAnalyzer;
import org.apache.lucene.analysis.de.GermanAnalyzer;
import org.apache.lucene.analysis.fr.FrenchAnalyzer;
import org.apache.lucene.analysis.fi.FinnishAnalyzer;
import org.apache.lucene.analysis.KeywordAnalyzer;
//import org.apache.lucene.analysis.snowball.SnowballAnalyzer;

public class PicSOMLucene3 {
    public static String version =
	"$Id: PicSOMLucene3.java,v 1.3 2016/01/07 10:47:02 jorma Exp $";

    ///
    public Hashtable<String,String> attrtable;
    public static void main(String[] args) {
	PicSOMLucene3 psl = new PicSOMLucene3();

	for (int i=0; i<args.length && !psl.quit;) {
	    final String a = args[i];//args{0, 3,6,...}
	    final String b = i<args.length-1 ? args[i+1] : null;
	    final String c = i<args.length-2 ? args[i+2] : null;
	    if (psl.verbose)
		System.out.println("Processing command line argument <" +
				   a +">");

	    if (a.equals("--compile")) {
		i++;

	    } else if (a.equals("--")) {
		i++;
		psl.interactive();

	    } else {
		int iplus = psl.interpret(true, a, b, c);
		if (iplus==0) {
		    String usage = "picsom-lucene"
			+" [--analyzer analyzername|list]"
			+" --index idxdir"
			+" --add-string|file|files id string|file";
		    System.err.println("Usage: " + usage);
		    System.exit(1);
		}
                //System.out.println(iplus);
		i += iplus;
	    }
	}

	psl.finalize();
    }

    ///
    int interpret(boolean cmdline, String ain, String b, String c) {
	String a = cmdline ? ain.substring(2) : ain;

	if (verbose) {
	    System.out.println("Interpreting <"+a+"> <"+(b!=null?b:"nil")+"> <"+
			       (c!=null?c:"nil")+">");
	}

	if (a.equals("version")) {
	    System.out.println("* "+System.getProperty("java.vendor")+
			       " "+System.getProperty("java.version")+
			       " "+System.getProperty("java.home"));
	    System.out.println("* "+System.getProperty("java.vm.vendor")+
			       " "+System.getProperty("java.vm.version")+
			       " "+System.getProperty("java.vm.name"));
	    System.out.println("* "+LucenePackage.get().toString());
	    System.out.println("* "+version);
	    System.out.println("* "+luceneVersion.toString());
	    return 1;
	}

	if (a.equals("quit")) {
	    quit = true;
	    return 1;
	}

	if (a.equals("verbose")) {
	    verbose = true;
	    return 1;
	}

	if (a.equals("index") && b!=null) {
	    openIndex(b);
	    return 2;
	}

	if (a.equals("analyzer") && b!=null) {
	    if (b.equals("list")) {
		AnalyzerFactory3.listAnalyzers();
		return 2;
	    }
	    if (idxname==null) {
		analyzerName = b;
		return 2;
	    }
	    System.err.println("analyzer cannot be set after index");
	}

	if (a.equals("textfield") && b!=null) {
	    textfield = b;
	    return 2;
	}

	if (a.equals("add-string") && b!=null && c!=null) {
	    addString(b, c);
	    return 3;
	}

	if (a.equals("add-file") && b!=null && c!=null) {
	    addFile(b, c);
	    return 3;
	}
 
	if (a.equals("add-attribute") && b!=null && c!=null) {
            //add (key:attribute, text:text) to hash table
            //System.out.println(b + " "+ c);
            attrtable.put(b.trim(), c.trim());
            
            return 3;
        }

        if (a.equals("add-document") && b!=null) {
            //write hash table in the file
	    addDocument(b, attrtable);
	    attrtable.clear();

	    return 2;
	}

        if (a.equals("update-document") && b!=null) {
	    updateDocument(b, attrtable);
	    attrtable.clear();

	    return 2;
	}

	if (a.equals("add-files") && b!=null) {
	    addFiles(b);
	    return 2;
	}

	if (a.equals("add-files-by-name") && b!=null) {
	    addFilesByName(b);
	    return 2;
	}

	if (a.equals("get-labels")) {
	    getLabels();
	    return 1;
	}

	if (a.equals("retrieve-document") && b!=null) {
	    retrieveDocument(b);
	    return 2;
	}

	if (a.equals("search-string") && b!=null) {
	    searchString(b);
	    return 2;
	}

	if (a.equals("search-file") && b!=null) {
	    searchFile(b);
	    return 2;
	}

	if (a.equals("search-files") && b!=null) {
	    searchFiles(b);
	    return 2;
	}

	if (a.equals("flush-index")) {
	    flushIndex();
	    return 2;
	}

	System.err.println("interpret("+ain+") failed");

	return 0;
    }	    

    /// 
    boolean interactive() {
	try {
	    BufferedReader in =
		new BufferedReader(new InputStreamReader(System.in, "UTF-8"));
	    while (!quit) {
		String line = in.readLine();

		if (line == null || line.length() == -1)
		    break;

		String[] l = line.split("\\s+", 2);
		String a = l[0], b = l.length>1 ? l[1] : null, c = null;
		//System.out.println("1 ["+a+"] ["+b+"] ["+c+"]");

		if (a.length()>0 && a.substring(0, 1).equals("#"))
		    continue;

		if (a.equals("add-string") || a.equals("add-file") ||
		    a.equals("add-attribute")) {
		    String[] m = b.split("\\s+", 2);
		    b = m[0];
		    c = m[1];
		    //System.out.println("2 ["+a+"] ["+b+"] ["+c+"]");
		}
		if (interpret(false, a, b, c)==0)
		    return false;
		System.out.println("* ok");
	    
	    }
	} catch (IOException e) {
	    System.err.println("interactive() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(2);
	}
	return true;
    }

    ///
    private PicSOMLucene3() {
        analyzerName = "standard";
	textfield    = "maintext";
        attrtable = new Hashtable<String,String>();
    }

    ///
    protected void flushIndex() {
        finalize();
	openIndex(idxname);
    }

    ///
    protected void finalize() {
        optimizeWriter();
        closeWriter();
        closeReader();
    }

    ///
    boolean createReader(boolean relaxed) {
	if (reader!=null)
	    return true;

	try {
	    reader = IndexReader.open(FSDirectory.open(new File(idxname)));

	    if (verbose)
		System.out.println("  Created index reader '" +idxname+"'");

	    if (normsField != null) {
		reader = new OneNormsReader(reader, normsField);

		if (verbose)
		    System.out.println("  Created one norms reader '" +
				       normsField+"'");
	    }

	} catch (IOException e) {
	    if (relaxed)
		return false;

	    System.err.println("createReader() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(2);
	}

	return true;
    }

    ///
    boolean openIndex(final String idir) {
	idxname = idir;
	idxdir = new File(idir);

	// nitems = 0;
	// indexDocs(writer, docDir, textfield);
	// System.out.println("  Indexed " +nitems+ " documents");
	// System.out.println("  Optimizing...");

	return true;
    }

    ///
    void checkAnalyzer(Analyzer an) {
	if (an == null) {
	    System.out.println("ERROR: No such analyzer implemented: "+
			       analyzerName);
	    AnalyzerFactory3.listAnalyzers();
	    System.exit(1);
	} else {
	    if (verbose)
		System.out.println("  Selecting "+analyzerName+" analyzer.");
	}
    }

    ///
    boolean optimizeWriter() {
	if (writer==null)
	    return true;

	/* deprecated
	try {
	    writer.optimize();

	} catch (IOException e) {
	    System.err.println("optimizeIndex() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}
	*/

	if (verbose)
	    System.out.println("  Optimized writer index");

 	return true;
    }

    ///
    IndexWriter getWriter() {
	if (writer == null) {
	    IndexWriterConfig.OpenMode openmode = OpenMode.CREATE_OR_APPEND;
	    try {
		Analyzer analyzer = AnalyzerFactory3.create(analyzerName,
							   luceneVersion);
		checkAnalyzer(analyzer);

		IndexWriterConfig iwc = new IndexWriterConfig(luceneVersion,
							      analyzer);
		iwc.setOpenMode(openmode);

		writer = new IndexWriter(FSDirectory.open(idxdir), iwc);

	    } catch (IOException e) {
		System.err.println("getWriter() caught a " + e.getClass() +
				   "\n with message: " + e.getMessage());
		System.exit(2);
	    }
            
	    if (verbose)
		System.out.println("  Created index writer '" +idxdir+ "'");
        }

        return writer;
    }

    ///
    boolean closeWriter() {
	if (writer==null)
	    return true;

	try {
	    writer.close();
	    writer = null;

	} catch (IOException e) {
	    System.err.println("closeWriter() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	if (verbose)
	    System.out.println("  Closed writer index");

 	return true;
    }

    ///
    boolean closeReader() {
	if (reader==null)
	    return true;

	try {
	    reader.close();
	    reader = null;

	} catch (IOException e) {
	    System.err.println("closeReader() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	if (verbose)
	    System.out.println("  Closed reader index");

 	return true;
    }

    /// DEPRECATED!
    boolean addString(final String id, final String str) {
	Field.Store store_yes	 = Field.Store.YES;
	Field.Store store_no 	 = Field.Store.NO;
	Field.Index analyzed_yes = Field.Index.ANALYZED;
	Field.Index analyzed_no  = Field.Index.NOT_ANALYZED;

	Document doc = new Document();
	doc.add(new Field(textfield, str, store_yes, analyzed_yes));
	doc.add(new Field("label",    id, store_yes, analyzed_yes));

        String strx = str;
	if (strx.length()>50)
	    strx = strx.substring(0, 50)+"...";

	if (verbose)
	    System.out.println("  Added ["+textfield+"] label=<"+id
			       +"> : \""+strx+"\"");

	IndexWriter wrt = getWriter();
	try {
	    wrt.addDocument(doc);

	} catch (Exception e) {
	    System.err.println("addString() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return true;
    }

    ///
    boolean addFile(final String id, final String fname) {
	String text = "";
	try {
	    FileReader fr = new FileReader(fname);
	    BufferedReader reader = new BufferedReader(fr);
	    String line = null;
	    while ((line=reader.readLine()) != null)
		text += line+" ";
	    fr.close();
	} catch (Exception e) {
	    System.err.println("addFile("+id+","+fname+
			       ") caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return addString(id, text);
    }

    ///
    boolean addFiles(final String fname) {
	return addFilesCommon(fname, false);
    }

    ///
    boolean addFilesByName(final String fname) {
	return addFilesCommon(fname, true);
    }

    ///
    boolean addFilesCommon(final String fname, boolean byname) {
	try {
	    FileReader fr = new FileReader(fname);
	    BufferedReader reader = new BufferedReader(fr);
	    String line = null;
	    while ((line=reader.readLine()) != null) {
		if (byname==false) {
		    String[] a = line.split("\\s+", 2);
		    addFile(a[0], a[1]);
		} else
		    addFile(line, line);
	    }
	    fr.close();
	} catch (Exception e) {
	    System.err.println("addFiles() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return true;
    }

    boolean documentExists(final String label) {
	if (verbose)
	    System.out.println("  Checking existence of \""+label+"\"");

	if (!createReader(true))
	    return false;

	try {
	    String quotedlabel = "\""+label+"\"";
	    String analyzerNamex = "keyword";
	    Analyzer analyzer = AnalyzerFactory3.create(analyzerNamex,
						       luceneVersion);
	    checkAnalyzer(analyzer);

	    String field = "label";
	    QueryParser parser = new QueryParser(luceneVersion, field,
						 analyzer);
	    Query query = parser.parse(quotedlabel);

	    IndexSearcher searcher = new IndexSearcher(reader);
	    TopDocs topDocs = searcher.search(query, 9999999);
	    ScoreDoc[] hits = topDocs.scoreDocs;

	    return hits.length>0;

	} catch (org.apache.lucene.index.IndexNotFoundException e) {
	    return false;

	} catch (IOException e) {
	    System.err.println("documentExists() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	} catch (org.apache.lucene.queryParser.ParseException e) {
	    System.err.println("documentExists() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return false;
    }

    boolean addDocument(final String labelin, Hashtable table) {
	String label = labelin.trim();
	if (documentExists(label)) {
	     System.err.println("addDocument() : document with label <" +
				label + "> already exists");
	     return false;
	}

        Field.Store store_yes	 = Field.Store.YES;
	Field.Store store_no 	 = Field.Store.NO;
	Field.Index analyzed_yes = Field.Index.ANALYZED;
	Field.Index analyzed_no  = Field.Index.NOT_ANALYZED;
        //add all document after mapping all text to correspound fields
       
        Document doc = new Document();
	doc.add(new Field("label", label, store_yes, analyzed_no));

        Set attrSet = table.keySet();
        Iterator attrIte = attrSet.iterator();

        while (attrIte.hasNext()) {
            // change the textfields to correspound attribute 
	    // ex. caption, title, text, etc
            Object attr = attrIte.next();
            Object maintext = table.get(attr);
            doc.add(new Field(attr.toString(), maintext.toString(),
			      store_yes, analyzed_yes));
            String strx = maintext.toString();
            if (strx.length()>50)
		strx = strx.substring(0, 50)+"...";
            if (verbose)
		System.out.println("  Added in <"+label+"> ["+
				   attr.toString()+"] : \""+strx+"\"");
	}

	IndexWriter wrt = getWriter();
	try {
	    wrt.addDocument(doc);

	} catch (Exception e) {
	    System.err.println("addDocument() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

        return true;
    }

    boolean updateDocument(final String labelin, Hashtable table) {
	String label = labelin.trim();
	if (!documentExists(label)) {
	     System.err.println("updateDocument() : document with label <" +
				label + "> does not exist");
	     return false;
	}

	String quotedlabel = "\""+label+"\"";

        Field.Store store_yes	 = Field.Store.YES;
	Field.Store store_no 	 = Field.Store.NO;
	Field.Index analyzed_yes = Field.Index.ANALYZED;
	Field.Index analyzed_no  = Field.Index.NOT_ANALYZED;
        //add all document after mapping all text to correspound fields
       
	Document doc = new Document();

	try {
	    String analyzerNamex = "keyword";
	    Analyzer analyzer = AnalyzerFactory3.create(analyzerNamex,
						       luceneVersion);
	    checkAnalyzer(analyzer);

	    String field = "label";
	    QueryParser parser = new QueryParser(luceneVersion, field,
						 analyzer);
	    Query query = parser.parse(quotedlabel);

	    IndexSearcher searcher = new IndexSearcher(reader);
	    TopDocs topDocs = searcher.search(query, 9999999);
	    ScoreDoc[] hits = topDocs.scoreDocs;

	    int docId = hits[0].doc;
	    doc = searcher.doc(docId);

	} catch (IOException e) {
	    System.err.println("retrieveDocument() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	} catch (org.apache.lucene.queryParser.ParseException e) {
	    System.err.println("retrieveDocument() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

        Set attrSet = table.keySet();
        Iterator attrIte = attrSet.iterator();

        while (attrIte.hasNext()) {
            Object attr = attrIte.next();
            Object maintext = table.get(attr);
	    doc.removeFields(attr.toString());
            doc.add(new Field(attr.toString(), maintext.toString(),
			      store_yes, analyzed_yes));
            String strx = maintext.toString();
            if (strx.length()>50)
		strx = strx.substring(0, 50)+"...";
            if (verbose)
		System.out.println("  Updated in <"+label+"> ["+
				   attr.toString()+"] : \""+strx+"\"");
	}

	IndexWriter wrt = getWriter();
	try {
	    Term term = new Term("label", label);
	    wrt.updateDocument(term, doc);

	} catch (Exception e) {
	    System.err.println("updateDocument() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

        return true;
    }

    /** Use the norms from one field for all fields.  Norms are read
     * into memory, using a byte of memory per document per searched
     * field.  This can cause search of large collections with a large
     * number of fields to run out of memory.  If all of the fields
     * contain only a single token, then the norms are all identical,
     * then single norm vector may be shared. */
    private static class OneNormsReader extends FilterIndexReader {
	private String field;

	public OneNormsReader(IndexReader in, String field) {
	    super(in);
	    this.field = field;
	}

	public byte[] norms(String field) throws IOException {
	    return in.norms(this.field);
	}
    }

    ///
    boolean getLabels() {
	if (verbose)
	    System.out.println("  Getting labels");

	if (!createReader(true))
	    return true;

	try {
	    int max = reader.maxDoc();
	    for (int i=0; i<max; i++) {
		Document doc = reader.document(i);
		List<Fieldable> fields = doc.getFields();
		Iterator fieldIte = fields.iterator();
		String label = "", other = "";
		while (fieldIte.hasNext()) {
		    Fieldable f = (Fieldable)fieldIte.next();
		    String key = f.name();
		    String val = f.stringValue();
		    if (key.equals("label"))
			label = val.equals("") ? "???" : val;
		    else if (!val.equals(""))
			other += (other.equals("") ? "" : " ")+key;
		}
		System.out.println("* "+label+" : "+other);
	    }

	} catch (IOException e) {
	    System.err.println("getLabels() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return true;
    }

    ///
    boolean retrieveDocument(final String str) {
	if (verbose)
	    System.out.println("  Retrieving \""+str+"\"");

	createReader(false);

	try {
	    String quotedstr = "\""+str+"\"";
	    String analyzerNamex = "keyword";
	    Analyzer analyzer = AnalyzerFactory3.create(analyzerNamex,
						       luceneVersion);
	    checkAnalyzer(analyzer);

	    String field = "label";
	    QueryParser parser = new QueryParser(luceneVersion, field,
						 analyzer);
	    Query query = parser.parse(quotedstr);

	    IndexSearcher searcher = new IndexSearcher(reader);
	    TopDocs topDocs = searcher.search(query, 9999999);
	    ScoreDoc[] hits = topDocs.scoreDocs;

	    for (int i=0; i<hits.length; i++) {
		int docId = hits[i].doc;
		Document doc = searcher.doc(docId);
		List<Fieldable> fields = doc.getFields();
		Iterator fieldIte = fields.iterator();
		while (fieldIte.hasNext()) {
		    Fieldable f = (Fieldable)fieldIte.next();
		    String key = f.name();
		    String val = f.stringValue();
		    System.out.println("* "+key+" : "+val);
		}
	    }

	} catch (IOException e) {
	    System.err.println("retrieveDocument() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	} catch (org.apache.lucene.queryParser.ParseException e) {
	    System.err.println("retrieveDocument() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return true;
    }

    ///
    boolean searchString(final String str) {
	boolean show = true;

	if (verbose)
	    System.out.println("  Searching ["+textfield+"] \""+str+"\"");

	createReader(false);

	try {
	    Analyzer analyzer = AnalyzerFactory3.create(analyzerName,
						       luceneVersion);
	    checkAnalyzer(analyzer);

	    String field = textfield;
	    QueryParser parser = new QueryParser(luceneVersion, field,
						 analyzer);
	    Query query = parser.parse(str);

	    if (verbose)
		System.out.println("  query.toString()=\""+query.toString()
				   +"\"");

	    IndexSearcher searcher = new IndexSearcher(reader);
	    TopDocs topDocs = searcher.search(query, 9999999);
	    ScoreDoc[] hits = topDocs.scoreDocs;

	    for (int i=0; i<hits.length; i++) {
		int docId = hits[i].doc;
		Document doc = searcher.doc(docId);
		String label = doc.get("label");
		float score = hits[i].score;
		String txt = show ? doc.get(field) : "";
		String txtx = txt;
		if (txtx!=null && txtx.length()>50)
		    txtx = txtx.substring(0, 50)+"...";

		System.out.println("* "+label+" "+score+" "+txtx);
	    }

	} catch (IOException e) {
	    System.err.println("searchString() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	} catch (org.apache.lucene.queryParser.ParseException e) {
	    System.err.println("searchString() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return true;
    }

    ///
    boolean searchFile(final String fname) {
	String text = "";
	try {
	    FileReader fr = new FileReader(fname);
	    BufferedReader reader = new BufferedReader(fr);
	    String line = null;
	    while ((line=reader.readLine()) != null)
		text += line+" ";
	    fr.close();
	} catch (Exception e) {
	    System.err.println("searchFile() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return searchString(text);
    }

    ///
    boolean searchFiles(final String fname) {
	try {
	    FileReader fr = new FileReader(fname);
	    BufferedReader reader = new BufferedReader(fr);
	    String line = null;
	    while ((line=reader.readLine()) != null) {
		searchFile(line);
	    }
	    fr.close();
	} catch (Exception e) {
	    System.err.println("searchFiles() caught a " + e.getClass() +
			       "\n with message: " + e.getMessage());
	    System.exit(3);
	}

	return true;
    }

    ///
    String idxname;

    ///
    File idxdir;
    
    ///
    String analyzerName;

    ///
    String textfield;

    ///
    static final Version luceneVersion = Version.LUCENE_35;
    //static final Version luceneVersion = Version.LUCENE_36;

    ///
    IndexWriter writer;

    ///
    boolean verbose = false;

    ///
    IndexReader reader;

    ///
    String normsField = null;

    ///
    boolean quit = false;

}

// 
// /*
// import org.apache.lucene.analysis.Analyzer;
// import org.apache.lucene.analysis.standard.StandardAnalyzer;
// import org.apache.lucene.analysis.standard.ClassicAnalyzer;
// import org.apache.lucene.analysis.en.EnglishAnalyzer;
// import org.apache.lucene.analysis.snowball.SnowballAnalyzer;
// 
// import org.apache.lucene.util.Version;
// */
// 
class AnalyzerFactory3 {
    private AnalyzerFactory3() {}

    public static Analyzer create(String analyzerName, Version vers) {
        /* StandardAnalyzer: Filters StandardTokenizer with
           StandardFilter, LowerCaseFilter and StopFilter, using a
           list of English stop words. */
        if (analyzerName.equals("standard"))
           return new StandardAnalyzer(vers);
        /* EnglishAnalyzer: Builds an analyzer with the default stop words... 
           mats: apparently also KStemmer?
         */
        else if (analyzerName.equals("english"))
            return new EnglishAnalyzer(vers);
        /* SnowballAnalyzer: Filters StandardTokenizer with
           StandardFilter, LowerCaseFilter, StopFilter and
           SnowballFilter. */
        else if (analyzerName.equals("snowball"))
            return null; //deprecated new SnowballAnalyzer(vers, "English");
        /* ClassicAnalyzer: Filters ClassicTokenizer with
           ClassicFilter, LowerCaseFilter and StopFilter, using a list
           of English stop words. */
        else if (analyzerName.equals("classic"))
            return new ClassicAnalyzer(vers);
        else if (analyzerName.equals("german"))
            return new GermanAnalyzer(vers);
        else if (analyzerName.equals("french"))
            return new FrenchAnalyzer(vers);
        else if (analyzerName.equals("finnish"))
            return new FinnishAnalyzer(vers);
        else if (analyzerName.equals("keyword"))
            return new KeywordAnalyzer();

        return null;
    }

    public static void listAnalyzers() {
	System.out.println("Supported analyzers:");
	System.out.println("  standard\t(default) Filters StandardTokenizer with StandardFilter, LowerCaseFilter and StopFilter, using a list of English stop words.");
	System.out.println("  english \tAnalyzer for English.");
	System.out.println("  classic \tFilters ClassicTokenizer with ClassicFilter, LowerCaseFilter and StopFilter, using a list of English stop words.");
	System.out.println("  german  \tAnalyzer for German language."); 
	System.out.println("  french  \tAnalyzer for French language.");
	System.out.println("  finnish \tAnalyzer for Finnish.");
	System.out.println("  keyword \tAnalyzer for vanilla keywords.");
    }

}

