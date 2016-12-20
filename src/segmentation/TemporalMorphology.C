// -*- C++ -*-  $Id: TemporalMorphology.C,v 1.4 2016/06/22 10:54:41 jorma Exp $
// 
// Copyright 1998-2014 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <TemporalMorphology.h>

namespace picsom {
  static const string vcid =
    "@(#)$Id: TemporalMorphology.C,v 1.4 2016/06/22 10:54:41 jorma Exp $";

  static TemporalMorphology list_entry(true);

  /////////////////////////////////////////////////////////////////////////////

  TemporalMorphology::TemporalMorphology() {
    dilate = 1.0;
    erode  = 0.2;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  TemporalMorphology::~TemporalMorphology() {
  }

  /////////////////////////////////////////////////////////////////////////////

  const char *TemporalMorphology::Version() const { 
    return vcid.c_str();
  }
  
  /////////////////////////////////////////////////////////////////////////////

  void TemporalMorphology::UsageInfo(ostream& os) const { 
    os << "TemporalMorphology :" << endl
       << "  options : " << endl
       << "  -m myargument" << endl
       << "  -s           outputs separate labeling" << endl;  

  }

  /////////////////////////////////////////////////////////////////////////////

  int TemporalMorphology::ProcessOptions(int argc, char** argv) {
    Option option;
    int argc_old=argc;

    while (*argv[0]=='-' && argc>1) {
      option.reset();
      switch (argv[0][1]) {
      case 'd':
      case 'e':
	if (argc<3) {
	  EchoError("Not enough commandline arguments for switch "+
		    string(argv[0]));
	  break;
	}

	option.option = argv[0];
	option.addArgument(argv[1]);

	if (argv[0][1]=='d')
	  dilate = atof(argv[1]);

	if (argv[0][1]=='e')
	  erode = atof(argv[1]);

	argc--;
	argv++;
	break;

      case 's':
	option.option="-s";
	// showseparate=true;

	break;
	
      default:
	EchoError(string("unknown option ")+argv[0]);
      } // switch

      if(!option.option.empty())
	addToSwitches(option);

      argc--;
      argv++; 
    } // while

    return argc_old-argc;
  }

  /////////////////////////////////////////////////////////////////////////////
  
  bool TemporalMorphology::ProcessGlobal() {
    string rangetype = "trangetext";
    bool debug = Verbose()>2;

    if (Verbose()>1)
      ShowLinks();

    SegmentationResultList *results =
      getImg()->readFileResultsFromXML("", rangetype, true);
    if (!results)
      return false;

    if (Verbose()>1)
      cout << results->size() << " file results found" << endl;

    map<string,list<SegmentationResult> > pername;

    for (SegmentationResultList::iterator ri=results->begin();
         ri!=results->end(); ri++) {
      if (debug)
	cout << "[" << ri->trange_start << "][" << ri->trange_end
	     << "][" << ri->type << "]["
	     << ri->name << "][" << ri->trange_text << "]" << endl;

      pername[ri->name].push_back(*ri);
    }

    delete results;
    //getImg()->invalidateFileResults(this);

    for (auto ni=pername.begin(); ni!=pername.end(); ni++) {
      for (size_t round=0;; round++) {
	if (debug)
	  cout << "name=" << ni->first << " dilation round=" << round
	       << " n=" << ni->second.size() << endl;

	bool found = false;
	for (auto ra=ni->second.begin(); ra!=ni->second.end(); ra++) {
	  for (auto rb=ni->second.begin(); rb!=ni->second.end();) {
	    if (ra==rb) {
	      rb++;
	      continue;
	    }

	    if (debug)
	      cout << "a=[" << ra->trange_start << "]["
		   << ra->trange_end << "][" << ra->type << "]["
		   << ra->name << "][" << ra->trange_text << "]"
		   << " b=[" << rb->trange_start << "]["
		   << rb->trange_end << "][" << rb->type << "]["
		   << rb->name << "][" << rb->trange_text << "] ";

	    if (ra->type!=rb->type || ra->name!=rb->name ||
		ra->trange_text!=rb->trange_text) {
	      if (debug)
		cout << "not matching" << endl;
	      rb++;
	      continue;
	    }

	    if (ra->trange_start<=rb->trange_start &&
		ra->trange_end>=rb->trange_end) {
	      rb = ni->second.erase(rb);
	      found = true;
	      if (debug)
		cout << "is part" << endl;
	      continue;
	    }

	    if (rb->trange_start>=ra->trange_end &&
		rb->trange_start-ra->trange_end<=dilate) {
	      ra->trange_end = rb->trange_end;
	      rb = ni->second.erase(rb);
	      found = true;
	      if (debug)
		cout << "dilated" << endl;
	      continue;
	    }
	    rb++;
	    if (debug)
		cout << endl;
	  }
	}
	if (!found)
	  break;
      }

      if (debug)
	cout << "name=" << ni->first << " erosion n="
	     << ni->second.size() << endl;

      for (auto ra=ni->second.begin(); ra!=ni->second.end();) {
	if (debug)
	  cout << "a=[" << ra->trange_start << "][" << ra->trange_end << "]["
	       << ra->type << "]["
	       << ra->name << "][" << ra->trange_text << "] ";

	if (ra->trange_end-ra->trange_start<erode) {
	  ra = ni->second.erase(ra);
	  if (debug)
	    cout << "eroded" << endl;
	  continue;
	}

	if (debug)
	  cout << endl;
	
	SegmentationResult r = *ra;
	size_t p = r.name.find(':');
	if (p!=string::npos)
	  r.name.erase(0, p+1);
	r.name.insert(0, ChainedMethodName()+":");
	r.StoreMemberValues();
	getImg()->writeFileResultToXML(this, r);

	ra++;
      }
    }

    return ProcessNextMethod();
  }

  /////////////////////////////////////////////////////////////////////////////

} // namespace picsom

// Local Variables:
// mode: font-lock
// End:
