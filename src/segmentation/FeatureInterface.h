// -*- C++ -*-	$Id: FeatureInterface.h,v 1.10 2007/05/16 10:39:49 vvi Exp $

#ifndef _FEATUREINTERFACE_H_
#define _FEATUREINTERFACE_H_

#include <Feature.h>

namespace picsom{
  class fSpec{
    public:
      int c;
      int index;
      float weight;
      
      fSpec(){ c=-1; weight=1;}
      fSpec(int cl,int i, float w):c(cl),index(i),weight(w){}
     

    };

    class fClass{
    public:
      Feature *ptr;
      std::string name;
      std::list<std::string> options;

      fClass(){ptr=NULL;}
      fClass(const std::string &n,
	     const std::list<std::string> &o):ptr(NULL),name(n),options(o){}


    };

    class fInterface{
    public:
      fInterface(){};

      void createMethods(segmentfile *img){
	unsigned int i;
	img->setPrepared();
	for(i=0;i<featureClass.size();i++){
	  featureClass[i].ptr=
	    Feature::CreateMethod(featureClass[i].name.c_str(),
				  img,
				  featureClass[i].options,
				  false); // do not allocate data
	}
      }

      void freeClasses(){

	size_t i;
	for(i=0;i<featureClass.size();i++){
	  delete featureClass[i].ptr;
	  featureClass[i].ptr=NULL;
	}

      }

      void calculateFeatures(std::vector<coordList *> &vc, 
			     Feature::featureVector &v,
			     int pixelsToUse =0){

	int size=0;
	for(size_t i=0;i<vc.size();i++)
	  size += vc[i]->size();

	//cout << "size="<<size<<endl;

	coordList newlist;
	if(pixelsToUse>0 && size > 2*pixelsToUse){
	  for(size_t l=0;l<vc.size();l++)
	    for(size_t i=0;i<pixelsToUse*(vc[l]->size())/size;i++)
	      newlist.push_back((*vc[l])[rand() % vc[l]->size()]);

	  vc.clear();
	  vc.push_back(&newlist);
	}

	Feature::featureVector *tmpF = 
	  new Feature::featureVector[featureClass.size()];
	unsigned int i;

	listRegion lr(vc);

	for(i=0;i<featureClass.size();i++){
	  tmpF[i]=featureClass[i].ptr->CalculateRegion(lr);
	}

	v.clear();

	for(i=0;i<featureSpec.size();i++)
	  v.push_back(featureSpec[i].weight*tmpF[featureSpec[i].c][featureSpec[i].index]);
    
	delete [] tmpF;

      }

      int parseFeatureSpec(char **argv){
	std::list<std::string> opts;
			     //std:/:list<int> &inds,
			     //			   std::list<int> &weights)
	int oind=1;

	while(argv[oind][0]=='-' && argv[oind][1]=='o'){
	  opts.push_back(argv[oind+1]);
	  oind +=2;
	}

	int index=featureClass.size();
	featureClass.push_back(fClass(argv[0],opts));

	const char *iptr=argv[oind];
	const char *wptr=argv[oind+1];	  

	string istr(argv[oind]); 
	string wstr(argv[oind+1]); 

	size_t spos;

	if((spos=istr.find('-'))!=string::npos){

	  istr[spos]=' ';
	  int start,stop;
	  sscanf(istr.c_str(),"%d %d",&start,&stop);
	  char nstr[10];
	  sprintf(nstr,"%d",start);
	  istr=nstr;
	  for(int i=start+1;i<=stop;i++){
	    sprintf(nstr,"%d",i);
	    istr += ":";
	    istr += nstr;
	  }

	  iptr=istr.c_str();
// 	  	  cout << "mapped feature spec " << argv[oind] << " to " <<
// 	     iptr << endl;
	}

	bool allones=(wstr=="a");

	while(iptr){

	  if(!allones && !wptr){
	    throw std::string("No weights for all indices ");
	    break;
	  }
	  int i;
	  float w;
	  if(sscanf(iptr,"%d",&i) != 1){
	    throw std::string("Couldn't parse index from ")+argv[oind];
	    break;
	  }
	  else
	    if(!allones && sscanf(wptr,"%f",&w) != 1){
	      throw std::string("Couldn't parse weight from ")+argv[oind+1];
	      break;
	    }
	  
	  if(allones) w=1;

	  featureSpec.push_back(fSpec(index,i,w));
	  
	 iptr=strchr(iptr,':'); if(iptr) iptr++;
	 if(!allones){
	   wptr=strchr(wptr,':'); if(wptr) wptr++;
	 }
	} // while

	return opts.size();
       }

      vector<fSpec> featureSpec;
      vector<fClass> featureClass;


    };
}



#endif //_FEATUREINTERFACE_H_
