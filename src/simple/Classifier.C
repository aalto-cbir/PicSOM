// 
// Copyright 1994-2009 Jorma Laaksonen <jorma@cis.hut.fi>
// Copyright 1998-2018 PicSOM Development Group <picsom@cis.hut.fi>
// Helsinki University of Technology
// P.O.BOX 5400, FI-02015 HUT, FINLAND
// 

// @(#)$Id: Classifier.C,v 1.12 2018/12/15 23:27:04 jormal Exp $

#ifndef _CLASSIFIER_C_
#define _CLASSIFIER_C_

#include <Classifier.h>
#include <StatQuad.h>

#if SIMPLE_FEAT
#include <AddMul.h>
#endif // SIMPLE_FEAT

#if SIMPLE_CLASS_SSC
#include <SubSpaceClassifier.h>
#endif // SIMPLE_CLASS_SSC

#if SIMPLE_CLASS_SOM
#include <SOM.h>
#endif // SIMPLE_CLASS_SOM

#if SIMPLE_CLASS_LVQ
#include <LVQ.h>
#endif // SIMPLE_CLASS_LVQ

#if SIMPLE_CLASS_COMMITTEE
#include <Committee.h>
#endif // SIMPLE_CLASS_COMMITTEE

namespace simple {

bool Classifier::debug_adapt = false;
bool Classifier::debug_skips = false;

///////////////////////////////////////////////////////////////////////////////

// Classifier::Classifier(const char *name) {
// }

///////////////////////////////////////////////////////////////////////////////

Classifier::~Classifier() {
  delete result_file;
  delete identity;
  delete pre_accuracy_text;
  delete normalization;

  DeleteCrossValidation();

  if (HasAcceptLabels())
    regfree(&accept_labels);
  delete accept_labels_pattern;
}

///////////////////////////////////////////////////////////////////////////////

Classifier& Classifier::Init(const Classifier *m, int cv_too) {
  //  preclassify          	  = m ? m->preclassify             : NULL;
  default_k            	  = m ? m->default_k               : 3; 
  reject_k             	  = m ? m->reject_k                : 0; 
  add_k             	  = m ? m->add_k                   : 0; 
  default_units        	  = m ? m->default_units           : 0;
  default_iter         	  = m ? m->default_iter            : 1;
  default_rep          	  = m ? m->default_rep             : 1;
  perf_block_size      	  = m ? m->perf_block_size         : 1;
  adapt_rule           	  = m ? m->adapt_rule              : 3;
  silent               	  = m ? m->silent                  : 0;
  cross_validation_silent = m ? m->cross_validation_silent : FALSE;
  insider_init         	  = m ? m->insider_init            : TRUE;
  do_eveninit          	  = m ? m->do_eveninit             : TRUE;
  old_cross_validation 	  = m ? m->old_cross_validation    : 0;
  train_set            	  = m ? m->train_set    	   : NULL;
  test_set             	  = m ? m->test_set     	   : NULL;
  current_cfmtrx          = m ? m->current_cfmtrx          : 0;
  force_test_optimization = m ? m->force_test_optimization : FALSE;
  no_train_testing        = m ? m->no_train_testing        : FALSE;
  cross_val_train         = m ? m->cross_val_train         : FALSE;
  track_errors            = m ? m->track_errors            : FALSE;
  optvar                  = m ? m->optvar                  : NULL;
  feat_ext                = m ? m->feat_ext                : NULL;
  do_comp_equal           = m ? m->do_comp_equal           : FALSE;
  do_class_metrics        = m ? m->do_class_metrics        : FALSE;
  cost                    = m ? m->cost                    : NULL;
  tail_length             = m ? m->tail_length             : 0;
  batchtrain              = m ? m->batchtrain              : false;
  batchtrainperiod        = m ? m->batchtrainperiod        : 0;
  batchtrain_threads      = m ? m->batchtrain_threads      : 0;
  sort_cfm                = m ? m->sort_cfm                : true;
  divide_by_numbers       = m ? m->divide_by_numbers       : false;
  bmu_div_depth           = m ? m->bmu_div_depth           : 1;
  use_timestamp           = m ? m->use_timestamp           : true;

  pre_accuracy_text = m ? CopyString(m->pre_accuracy_text) : NULL;
  identity          = m ? CopyString(m->identity)      	   : NULL;
  normalization     = m ? CopyString(m->normalization) 	   : NULL;

  result_file    = NULL;
  current_source = NULL;
  raw_vector     = NULL;

  // error_impulses = NULL;

  tictacptr = &tictac;
  TicTacSetName("Classifier");
  //NoTicTac();

#if SIMPLE_MISC_DECAY
  if (m)
    alpha = m->alpha;
  else
    alpha.SetConstant(0.1).Keep();
#endif // SIMPLE_MISC_DECAY

  if (cv_too) {
    cross_val.Delete();
    if (m) {
      for (int c=0; c<m->NCrossValidation(); c++)
	cross_val.Append(m->CrossValidation(c)->CopyForCrossValidation(TRUE));
      CopyCvSetsFrom(*m);
    }
  }

  cfmtrx_text = NULL;

  accept_labels.re_nsub = MAXINT;
  accept_labels_pattern = NULL;

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Dump(Simple::DumpMode type, ostream& os) const {
  if (type&DumpRecursive)
    FloatVectorSet::Dump(type, os);

  if (type&DumpShort || type&DumpLong) {
    os << Bold("   Classifier ")      << (void*)this
       << " default_units="           << default_units
       << " default_k="               << default_k
       << " reject_k="                << reject_k
       << " add_k="                   << add_k
       << " default_iter="            << default_iter
       << " default_rep="             << default_rep
       << " perf_block_size="         << perf_block_size
       << " insider_init="            << insider_init
       << " do_eveninit="             << do_eveninit
       << " identity="                << ShowString(identity)
       << " pre_accuracy_text="       << ShowString(pre_accuracy_text)
       << " normalization="           << ShowString(normalization)
       << " adapt_rule="              << adapt_rule
       << " silent="                  << silent
       << " cross_validation_silent=" << cross_validation_silent
       << " train_set="               << (void*)train_set
       << " test_set="                << (void*)test_set
       << " batchtrain="              << batchtrain
       << " batchtrain_threads="      << batchtrain_threads
       << " current_cfmtrx="          << current_cfmtrx
       << " force_test_optimization=" << force_test_optimization
       << " no_train_testing="        << no_train_testing
       << " cross_val_train="         << cross_val_train
       << " old_cross_validation="    << old_cross_validation
       << " cross_val.Nitems()="      << cross_val.Nitems()
       << " result_file="             << ShowString(result_file)
       << " track_errors="            << track_errors
       << "(" << errors.Length() << ")"
       << " do_comp_equal="           << do_comp_equal
       << " do_class_metrics="        << do_class_metrics
       << " cost="                    << (void*)cost
       << " raw_vector="              << (void*)raw_vector
       << " feat_ext="                << (void*)feat_ext
       << " accept_labels.re_nsub="   << accept_labels.re_nsub
       << " accept_labels_pattern="   << accept_labels_pattern;

#if SIMPLE_FEAT
    if (feat_ext) {
      os << endl;
      feat_ext->Dump(type, os);
    }
#endif // SIMPLE_FEAT

    os << " optvar=" << (void*)optvar;
    if (optvar) {
      os << endl;
      optvar->Dump(type, os);
    }
    else
      os << endl;
  }

  // CrossValidation is currently not dumped at all.
  // should I enumerate DumpCrossValidation
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::DumpCrossValidation(Simple::DumpMode type,
					     ostream& os) const {
  if (NCrossValidation() && (type&DumpRecursive)) {
    os << endl << "cross_val:" << endl << endl;
    cross_val.Dump(type, os);
  }
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::RandomInit(int l) {
  for (int u=0; u<Nitems(); u++)
    Get(u)->Length(l).Randomize().Normalize();

  return l!=0;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptVector(const FloatVector& vec) {
#if SIMPLE_MISC_DECAY
  switch (adapt_rule) {
  case 0:
    break;
  case 1:
    Adapt1(vec, default_k, alpha);
    break;
  case 2:
    Adapt2(vec, default_k, alpha);
    break;
  case 3:
    Adapt3(vec, default_k, alpha);
    break;
  case 4:
    Adapt4(vec, add_k, alpha);
    break;
  default:
    cerr << "Classifier::AdaptVector rule #" << adapt_rule
	 << " not implemented." << endl;
    return false;
  }
  return true;
#else 
  ShowError("Classifier::AdaptVector SIMPLE_FEAT not defined.");
  return false;
#endif // SIMPLE_MISC_DECAY
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Adapt() {
  if (TrainingSet())
    Adapt(*TrainingSet());
  else
    ShowError("Classifier::Adapt(): no training set");
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Adapt(FloatVectorSource& set) {
//   if (batchtrain) {
//     BatchTrain(set);
//     return;
//   }

  char text[10000];
  sprintf(text, "%s adapting", FileName());
  set.ConditionallySetPrintText(text);

  //  VectorOf<Type> tmp;
  
  set.First();
  for (const FloatVector *vec; (vec = set.Next());) {
//     if (HasPreClassify()) {
//       PreClassify(*vec, tmp);
//       cout << "Preclassified dimension " << vec->Length() << "->"
// 	   << tmp.Length() << endl;
//       vec = &tmp;
//     }

    AdaptVector(*vec);
  }

  AdaptEpochDone(set);
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptNew(bool tra, bool div, bool wdiv, bool vot, bool wcod,
			  double& mdist, double& rmsdist, bool lastiter) {
  if (!tictacptr)
    ShowError("Classifier::AdaptNew() tictacptr not set");

  if (tictacptr==&tictac)
    ShowError("Classifier::AdaptNew() tictacptr==&tictac");

  if (!TrainingSet()) {
    ShowError("Classifier::AdaptNew() trainingset not set");
    return false;
  }

  if (tra && !AdaptTraining())
    return false;

  IntVectorSet bmuset;
  FloatVector distance;
  FloatVectorSet labset;

  if (div && !AdaptDivide(bmuset, distance, labset, mdist, rmsdist))
    return false;

  if (div && !AdaptSetDivide(bmuset, distance))
    return false;

  if (wdiv && !AdaptWriteDivide(lastiter))
    return false;

  if (vot && !AdaptVote(bmuset[0], distance, labset))
    return false;

  if (wcod && !AdaptWrite())
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptTraining() {
  Tic("Classifier::AdaptTraining()");

  FloatVectorSource& set = *TrainingSet();

  char text[1000];
  sprintf(text, "%s adapting", FileName());
  if (batchtrain)
    strcat(text, " with BATCHTRAIN");
  set.ConditionallySetPrintText(text);

  double md = 0.0, rmsd = 0.0;
  bool r = batchtrain ? BatchTrain(NULL, NULL, NULL, md, rmsd) :
    PerVectorTrain(NULL, NULL, NULL, md, rmsd);  

  Tac("Classifier::AdaptTraining()");

  return r;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptDivide(IntVectorSet& bmuset, FloatVector& dist,
			     FloatVectorSet& labs,
			     double& mdist, double& rmsdist) {
  Tic("Classifier::AdaptDivide()");
  bool ok = true;

  FloatVectorSource& set = *TrainingSet();
  int n = DivideByNumbers() ? set.MaxNumber()+1 : set.Nitems();
  if (n<1) {
    ok = ShowError("Classifier::AdaptDivide() has to know trainig set size");
    goto out;
  }
  
  bmuset.Delete();
  for (int i=0; i<bmu_div_depth; i++)
    bmuset.Append(new IntVector(n));

  dist.Length(n);
  labs.Delete();
  labs.GetEvenNew(n-1);

  char text[1000];
  sprintf(text, "dividing %s with %s ", set.FileName(), FileName());
  if (DivideByNumbers())
    sprintf(text+strlen(text), "by %d numbers in [%d..%d] of [0..%d]",
	    set.Nitems(), set.MinNumber(), set.MaxNumber(), n-1);
  else
    strcat(text, "classically");

  if (batchtrain)
    strcat(text, " with BATCHTRAIN");

  sprintf(text+strlen(text), " BMU depth %d", bmu_div_depth);

  set.ConditionallySetPrintText(text);

  cout << PossiblyTimeStamp() << "Classifier::AdaptDivide() " << text << endl;

  ok = batchtrain ? BatchTrain(bmuset.Get(0), &dist, &labs, mdist, rmsdist) :
    PerVectorTrain(&bmuset, &dist, &labs, mdist, rmsdist);

 out:
  Tac("Classifier::AdaptDivide()");

  if (ok) {
    int units = GetNitems();
    ListOf<IntVector> backref;
    CreateBackReferences(units, n, bmuset[0], backref);
  }

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::PerVectorTrain(IntVectorSet *bmuset, FloatVector *dist,
				FloatVectorSet* labs, double& mdist,
				double& rmsdist) {
  Tic("Classifier::PerVectorTrain()");

  FloatVectorSource& set = *TrainingSet();

  bool ok = true;
  int i = 0;
  set.First();
  if (bmuset)
    for (int ii=0; ii<bmuset->Nitems(); ii++)
      bmuset->Get(ii)->Set(-1);

  mdist = rmsdist = 0.0;
  size_t distdiv = 0;

  const IntVector *bmu = bmuset ? bmuset->Get(0) : NULL;  // a shortcut
  IntVector bmuvec(bmuset ? bmuset->Nitems() : 0);
  IntVector *bmuvecp = bmuvec.Length() ? &bmuvec : NULL;

  for (const FloatVector *vec; ok && (vec = set.Next()); i++) {
    //     cout << "i=" << i << " number=" << vec->Number() << " label="
    // 	 << vec->Label() << endl;
    
    if (labs) {
      int j = DivideByNumbers() ? vec->Number() : i;
      FloatVector *jvec = labs->Get(j);
      if (!jvec) {
	char txt[1000];
	sprintf(txt, "DivideByNumbers()=%d vec->Number()=%d i=%d",
		DivideByNumbers(), vec->Number(), i);
	ok = ShowError("Classifier::PerVectorTrain() jvec==NULL while ", txt);
      }
      else
	jvec->Label(vec->Label());
    }

    if (bmuset) {
      int j = DivideByNumbers() ? vec->Number() : i;
      if (bmu->IndexOK(j) && (*bmu)[j]==-1 && dist->IndexOK(j)) {
	(*bmu)[j] = FindBestMatch(*vec, &((*dist)[j]), bmuvecp);
	mdist   += sqrt((*dist)[j]);
	rmsdist += (*dist)[j];
	distdiv++;

	if (bmuvecp)
	  for (int k=1; k<bmuvec.Length(); k++)
	    bmuset->Get(k)->Set(j, bmuvec[k]);

      } else {
	char txt[1000];
	sprintf(txt, "i=%d j=%d vec->Label()=%s vec->Number()=%d "
		"bmu->IndexOK(j)=%d dist->IndexOK(j)=%d (*bmu)[j]=%d",
		i, j, vec->Label(), vec->Number(),
		bmu->IndexOK(j), dist->IndexOK(j),
		bmu->IndexOK(j)?(*bmu)[j]:-999999);
	ok = ShowError("Classifier::PerVectorTrain() index problem: ", txt);
      }
    } else
      if (!SkipLabel(vec->Label())) { 
	if (DebugAdapt())
	  cout << "Classifier::PerVectorTrain() adapting with ["
	       << vec->Label() << "] #" << vec->Number()<< endl;
	// Actually this the only case when
	// something is really trained here ...
	ok = AdaptVector(*vec);      

      } else if (DebugSkips())
	cout << "Classifier::PerVectorTrain() skipping [" << vec->Label()
	     << "]" << endl;
  }

  if (distdiv) {
    mdist /= distdiv;
    rmsdist = sqrt(rmsdist/distdiv);
  }

  if (!Silent())
    cout << PossiblyTimeStamp() << "Classifier::PerVectorTrain() adapted with "
	 << i << " vectors                               " << endl;

  Tac("Classifier::PerVectorTrain()");

  return ok;
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::FindBestMatch(const FloatVector&, float*, IntVector*) const {
  bool first = true;
  if (first) {
    ShowError("Classifier::FindBestMatch() called");
    first = false;
  }
  return -1;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::BatchTrain(IntVector *bmu_out, FloatVector *dist,
			    FloatVectorSet* labs, double& /*mdist*/,
			    double& /*rmsdist*/) {
  if (DivideByNumbers())
    ShowError("Classifier::BatchTrain() may not work "
	      "with DivideByNumbers()==true");

  FloatVectorSource& set = *TrainingSet();
  set.First();

  int n = set.Nitems(), i=0;
  if (n<1) {
    ShowError("Classifier::BatchTrain() has to know trainig set size");
    return false;
  }

  IntVector bmutmp;
  if (!bmu_out)
    bmutmp.Length(n);
  else if (bmu_out->Length()!=n) {
    ShowError("Classifier::BatchTrain() bmu_out->Length()!=n");
    return false;
  }
  IntVector& bmu = bmu_out ? *bmu_out : bmutmp;

  bool is_train = !bmu_out;

  cout << PossiblyTimeStamp() << "Classifier::BatchTrain() "
       << (is_train?"TRAINING":"DIVIDING")
       << " threads=" << batchtrain_threads << endl;

  int units = GetNitems();
  bool ret = true;

  int a = 0;
  for (; ret;) {
    bmu.Set(-1);

    FloatVectorSet sum;
    if (is_train)
      for (i=0; i<units; i++)
	sum.Append(new FloatVector(VectorLength()));

    int nn = is_train ? batchtrainperiod : 0;

#ifdef CLASSIFIER_USE_PTHREADS
    if (batchtrain_threads)
      BatchTrainPthread(bmu, sum, dist, labs, a, nn);
    else
#endif // CLASSIFIER_USE_PTHREADS
      BatchTrainNoThreads(bmu, sum, dist, labs, a, nn);

    if (!is_train || nn)
      return true;

    ListOf<IntVector> backref;
    CreateBackReferences(units, n, bmu, backref);
    ret = BatchTrainEpochDone(sum, backref);

    if (!batchtrainperiod)
      break;

    a += batchtrainperiod;
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::BatchTrainNoThreads(IntVector& bmu, FloatVectorSet& sum,
				     FloatVector *dist, FloatVectorSet* labs,
				     int i, int& nn) {
  Tic("Classifier::BatchTrainNoThreads()");

  cout << PossiblyTimeStamp() << "Classifier::BatchTrainNoThreads() i=" << i
       << " nn=" << nn << endl;

  FloatVectorSource& set = *TrainingSet();

  int units = GetNitems();
  for (const FloatVector *vec; (vec = set.Next()); i++) {
    // cout << "### <" << vec->Label() << ">" << endl;

    if (labs)
      labs->Append(new FloatVector(0, NULL, vec->Label()));
    int u = FindBestMatch(*vec, dist?&((*dist)[i]):NULL);
    if (u<0 || u>=units) {
      ShowError("Classifier::BatchTrain() !IndexOK(u)");
      break;
    }
    bmu[i] = u;
    if (sum.Nitems())
      sum[u] += *vec;

    if (nn && !--nn)
      break;
  }
  
  Tac("Classifier::BatchTrainNoThreads()");
}

///////////////////////////////////////////////////////////////////////////////

#ifdef CLASSIFIER_USE_PTHREADS
void Classifier::BatchTrainPthread(IntVector& bmu, FloatVectorSet& sum,
				   FloatVector *dist, FloatVectorSet* labs,
				   int ii, int& nn) {
  Tic("Classifier::BatchTrainPthread()");

  cout << PossiblyTimeStamp() << "Classifier::BatchTrainPthread() i=" << ii
       << " nn=" << nn << endl;

  FloatVectorSource& set = *TrainingSet();

  const char *errhead = "Classifier::BatchTrainPthread() ";

  int n = batchtrain_threads>=0 ? batchtrain_threads : 16;
    
  pthread_attr_t attr;
  if (pthread_attr_init(&attr))
    ShowError(errhead, "pthread_attr_init() failed");
  
  if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS))
    ShowError(errhead, "pthread_attr_setscope() failed");

  int con = pthread_getconcurrency();
  if (pthread_setconcurrency(n))
    ShowError(errhead, "pthread_setconcurrency() failed");

  batch_train_pthread_data *data = new batch_train_pthread_data[n];
  for (int k=0; k<n; k++)
    data[k].pthread_set = false;

  int loopsleft = n+1;
  bool ready = false;
  for (int i=0; loopsleft; i++) {
    int j = i%n;
    if (data[j].pthread_set) {
      void *void_ret = NULL;
      if (pthread_join(data[j].pthread, &void_ret)) 
	ShowError(errhead, "pthread_join() failed");

      data[j].pthread_set = false;
      loopsleft = n+1;

      batch_train_pthread_data *ret = (batch_train_pthread_data*)void_ret;

      if (ret) {
	int idx = ret->vecidx, u = ret->bmu;
	bmu[idx] = u;

	if (sum.Nitems())
	  sum[u] += ret->vec;

	if (labs)
	  labs->Append(new FloatVector(0, NULL, ret->vec.Label()));

	if (dist)
	  (*dist)[idx] = ret->dist;
      }
    } else
      loopsleft--;

    if (ready)
      continue;

    const FloatVector *vec = set.Next();
    if (vec) {
      if (i>=bmu.Length())
	ShowError(errhead, "too many vectors");

      data[j].pthread_set = false;
      data[j].cfr = this;
      data[j].vec = *vec;
      data[j].vecidx = ii++;
      data[j].bmu = -1;
      data[j].dist = MAXFLOAT;

      // #ifdef __alpha
      void *(*xxx)(void*) = BatchTrainPthread;
      if (pthread_create(&data[j].pthread, &attr, (CFP_pthread) xxx, data+j))
	// #else
	// if (pthread_create(&data[j].pthread, &attr, BatchTrainPthread, data+j))
	// #endif // __alpha
	ShowError(errhead, "pthread_create() failed");
      else
	data[j].pthread_set = true;

    } else
      ready = true;

    if (nn && !--nn)
      ready = true;
  }

  delete [] data;

  pthread_attr_destroy(&attr);

  if (pthread_setconcurrency(con))
    ShowError(errhead, "pthread_setconcurrency() failed");

  Tac("Classifier::BatchTrainPthread()");
}
#endif // CLASSIFIER_USE_PTHREADS

///////////////////////////////////////////////////////////////////////////////

bool Classifier::CreateBackReferences(int units, int n, const IntVector& bmu,
				      ListOf<IntVector>& backref) {
  int i;

  IntVector count(units);
  for (i=0; i<n; i++)
    if (bmu[i]>=0)
      count[bmu[i]]++;

  int sum = count.Sum(), nonzeros = 0;
  double H = 0;
  for (i=0; i<units; i++)
    if (count[i]) {
      double p = count[i]/(double)sum;
      H -= p*log(p);
      nonzeros++;
    }
  double Hr = -H/log(1.0/sum);

  cout << PossiblyTimeStamp() << "Classifier::CreateBackReferences() H=" << H
       << " Hr=" << Hr << " sum=" << sum << " nonzeros=" << nonzeros
       << "/" << units << endl;

  for (i=0; i<units; i++)
    backref.Append(new IntVector(count[i]));

  IntVector idx(units);
  for (i=0; i<n; i++) {
    int u = bmu[i];
    if (backref.IndexOK(u) &&  backref[u].IndexOK(idx[u]))
      backref[u][idx[u]++] = i;
//     else {
//       ShowError("Classifier::CreateBackReferences() erroneous index");
//       return false;
//     }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::BatchTrainEpochDone(const FloatVectorSet& sum,
				     const ListOf<IntVector>& backref) {
  Tic("Classifier::BatchTrainEpochDone()");

  for (int i=0; i<sum.Nitems(); i++) {
    int n = backref[i].Length();
    if (n)
      (*this)[i] = sum[i]/(double)n;
  }

  Tac("Classifier::BatchTrainEpochDone()");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptSetDivide(const IntVectorSet& bmu,
				const FloatVector& d) {
  train_bmu_set = bmu;
  train_dist = d;
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptWriteDivide(bool /*lastiter*/) const {
  ShowError("empty Classifier::AdaptWriteDivide() called");
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptVote(const IntVector&, const FloatVector&,
			   const FloatVectorSet&) {
  ShowError("empty Classifier::AdaptVote() called");
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::AdaptWrite() {
  ShowError("empty Classifier::AdaptWrite() called");
  return true;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::ConditionallyTrackError(const FloatVector& vec,
					 const char *lab) {
  if (!vec.LabelsMatch(lab)) {
    ReportError(vec, lab);

    if (track_errors)
      errors.Push(vec.Number());
  }
  
  //  if (error_impulses)
  //    error_impulses->Push(!vec.LabelsMatch(lab));
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::ReportError(const FloatVector& vec, const char *l){
  if (!IsOpen(errlog))
    return;

  errlog << vec.Number() << " "
	 << vec.Label() << " "
	 << 0 << " "
	 << l << " "
	 << 0
	 << endl;
}

///////////////////////////////////////////////////////////////////////////////

const char *Classifier::ClassifyVector(const FloatVector& vec, float *dist) {
  int arb = FALSE;
  const char *ret = NearestNeighborsClass(vec, default_k, reject_k,
					  dist, &arb);
  if (arb)
    AddArbitraryToConfusionMatrix();

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

StatVar Classifier::Classify(FloatVectorSource& set,
				     const char *txt) {
  //  if (error_impulses)
  //    error_impulses->Length(0);

  errors.Length(0);
  classifications.clear();

  if (OldCrossValidation()>1) {
    DoOldCrossValidation(set);

    return StatVar();
  }

  CurrentSource(&set);

  char text[10000];
  sprintf(text, "%s classifying", FileName());
  if (txt)
    strcat(text, txt);
  set.ConditionallySetPrintText(text);

  // FloatVector tmp;
  
  set.First();
  for (const FloatVector *vec; (vec = set.Next()); ) {
//     if (HasPreClassify()) {
//       PreClassify(*vec, tmp);
//       vec = &tmp;
//     }

    if (!IsLabelAcceptable(vec->Label()))
      continue;

    const char *ret = feat_ext ? ClassifyRawVector(*vec):ClassifyVector(*vec);

    ConditionallyTrackError(*vec, ret);

    AddToConfusionMatrix(vec->Label(), ret);

    if (ret)
      classifications[vec->Number()] = ret;

    if (adapt_rule==4)
      AdaptVector(*vec);
  }

  CurrentSource(NULL);

  return StatVar(ErrorRatio());
}

///////////////////////////////////////////////////////////////////////////////

StatVar Classifier::Classify() {
  StatVar res;

  if (TestingSet()) {
    UseTestingSetConfusionMatrix();
    ClearConfusionMatrix();
    res = Classify(*TestingSet());

  } else if (TrainingSet() && TrainingSet()->CrossValidationTest()) {
    UseTrainingSetConfusionMatrix();
    ClearConfusionMatrix();
    res = Classify(*TrainingSet());

  } else {
    UseCrossValidationConfusionMatrix();
    ClearConfusionMatrix();
    res = CrossValidationTesting();
  }

  ShowResults();

  return res;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::DoOldCrossValidation(FloatVectorSource& /*set*/) {
  cerr << "Classifier::DoOldCrossValidation flushed to bit space" << endl;
//   if (&set != (FloatVectorSource*)this)
//     cerr << "Classifier::DoOldCrossValidation set is redundant this="
// 	 << (void*)this << " &set=" << (void*)&set	 << endl;

//   for (int i=0; i<OldCrossValidation(); i++) {
//     int s = i*Nitems()/OldCrossValidation();
//     int n = (i+1)*Nitems()/OldCrossValidation()-s;

//     FloatVectorSet train, test;
//     LeaveSomeOut(s, n, train, test);
//     test.PrintNumber(GetPrintNumber());
//     test.PrintText(GetPrintText());

//     Classifier tested(train/*, FALSE*/);
//     tested.Init(this, TRUE);
//     tested.OldCrossValidation(FALSE);

//     char text[1000];
//     sprintf(text, " X-val %d/%d", i, OldCrossValidation());
//     tested.Classify(test, HasPrintText() ? text : NULL);

//     // tested.ShowResults();

//     AddToConfusionMatrix(tested.GetConfusionMatrix());
//   }
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::Configuration(const char *str, const char **retstr) {
  int found = FALSE;
  const char *base = str;
  if (retstr)
    *retstr = NULL;

  for (; *str; str++) {
    if (*str==' ' || *str=='\t')
      continue;
    
    int n, d;
    float f = 0;

    if (sscanf(str, "adapt%d%n", &d, &n)) {
      AdaptRule(d);
      str += n-1;
      found = TRUE;

    } else if (sscanf(str, "k%d%n", &d, &n)) {
      DefaultK(d);
      str += n-1;
      found = TRUE;

    } else if (sscanf(str, "reject%d%n", &d, &n)) {
      RejectK(d);
      str += n-1;
      found = TRUE;

    } else if (sscanf(str, "units%d%n", &d, &n)) {
      DefaultUnits(d);
      str += n-1;
      found = TRUE;

    } else if (sscanf(str, "insiders%d%n", &d, &n)) {
      InsiderInit(d);
      str += n-1;
      found = TRUE;

#if SIMPLE_MISC_DECAY
    } else if (sscanf(str, "alpha%f%n", &f, &n)) {
      alpha = f;
      str += n-1;
      found = TRUE;
#endif // SIMPLE_MISC_DECAY

    } else if (sscanf(str, "iter%d%n", &d, &n)) {
      DefaultPerformanceIterations(d);
      str += n-1;
      found = TRUE;

    } else if (sscanf(str, "rep%d%n", &d, &n)) {
      DefaultPerformanceRepetitions(d);
      str += n-1;
      found = TRUE;

    } else if (sscanf(str, "silent%d%n", &d, &n)) {
      Silent(d);
      str += n-1;
      found = TRUE;

    } else if (retstr) {
      *retstr = str;
      return found;

    } else {
	cerr << "Classifier::Configuration(" << base << ") error in: "
	     << str << endl;
	return FALSE;
    }

    f = f+7;
  }

  return found;
}

///////////////////////////////////////////////////////////////////////////////

const char *Classifier::Configuration() const {
  static char conf[1000];
  *conf = 0;

  sprintf(conf+strlen(conf), "adapt%d", AdaptRule());
  sprintf(conf+strlen(conf), "k%d", DefaultK());
  sprintf(conf+strlen(conf), "reject%d", RejectK());
  sprintf(conf+strlen(conf), "units%d", Nitems());
  sprintf(conf+strlen(conf), "insiders%d", InsiderInit());
#if SIMPLE_MISC_DECAY
  sprintf(conf+strlen(conf), "alpha%s", alpha.ToString());
#endif // SIMPLE_MISC_DECAY
  sprintf(conf+strlen(conf), "iter%d", DefaultPerformanceIterations());
  sprintf(conf+strlen(conf), "rep%d", DefaultPerformanceRepetitions());
  sprintf(conf+strlen(conf), "silent%d", Silent());

  return conf;
}

///////////////////////////////////////////////////////////////////////////////

// void Classifier::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// void Classifier::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// void Classifier::() {
// 
// }

///////////////////////////////////////////////////////////////////////////////

void Classifier::ShowResultsHead(ostream& os) const {
  ShowResultsHeadNoK(os);
  os << "k=" << DefaultK() << " ";
  if (RejectK())
    os << "reject-k=" << RejectK() << " ";

  if (adapt_rule) {
    os << "AdaptRule=" << adapt_rule;
    if (adapt_rule==4)
      os << "(add_k=" << add_k << ")";
    os << " ";
  }
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::ShowResultsHeadNoK(ostream& os) const {
  if (Metric())
    os << "Metric=" << Metric()->String() << " ";

  os << "VectorLength=" << VectorLength() << " "
     << "Units=" << Nitems() << " ";
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::ShowResults(int sil, int tail, ostream& os) const {
  if (tail==-1)
    tail = tail_length;

  if (sil==-1)
    sil = Silent();

  if (sil==0) {
    if (sort_cfm)
      SortConfusionMatrix();
    DisplayConfusionMatrix(os);
  }

  if (PreAccuracyText())
    os << PreAccuracyText()<< " ";

  if (sil<2) {
    if (IsTrainingSetConfusionMatrix())
      os << "TrainingSet ";

    if (IsTestingSetConfusionMatrix())
      os << "TestingSet ";

    if (IsCrossValidationConfusionMatrix()) {
      os << "NCrossVal=" << NCrossValidation() << " ";
      cross_val[0].ShowResultsHead(os);

    } else
      ShowResultsHead(os);

    DisplayConfusionAccuracy(tail, os);

  } else if (sil<4)
    os << ErrorRatio();
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Adapt1(const FloatVector& v, int k, double a) {
  IntVector idx(k);
  FloatVector ddd;
  NearestNeighbors(v, idx, ddd);
  FloatVectorSet::Statistics(v, idx, k, "before");

  for (int i=0; i<k && idx[i]>=0; i++) {
    FloatVector *vec = Get(idx[i]);

    if (v.LabelsMatch(*vec))
      vec->MoveTowards(v, a);
    else
      vec->MoveTowards(v, -a);
  }

  NearestNeighbors(v, idx, ddd);
  FloatVectorSet::Statistics(v, idx, k, "after rule1");
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Adapt2(const FloatVector& v, int k, double a) {
  IntVector idx1(k+1), idx2, idx3(k+1);
  FloatVector ddd;
  NearestNeighbors(v, idx1, ddd);

  const char *lab1 = NearestNeighborsClassIdx(idx1, k);
  idx2 = idx1;
  idx2.Swap(k-1, k);
  const char *lab2 = NearestNeighborsClassIdx(idx2, k);

  if (v.LabelsMatch(lab2) && !v.LabelsMatch(lab1)) {
    FloatVectorSet::Statistics(v, idx1, k, "before");
    
    for (int i=k-1; i<k+1 && idx2[i]>=0; i++) {
      FloatVector *vec = Get(idx2[i]);

      if (v.LabelsMatch(*vec))
	vec->MoveTowards(v, a);
      else
	vec->MoveTowards(v, -a);
    }
    NearestNeighbors(v, idx3, ddd);
    /*const char *lab3 =*/ NearestNeighborsClassIdx(idx3, k);
    FloatVectorSet::Statistics(v, idx3, k, "after");

  } else
    FloatVectorSet::Statistics(v, idx1, k, "no change");
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Adapt3(const FloatVector& v, int k, double a) {
  IntVector idx(k+1);
  FloatVector ddd;
  NearestNeighbors(v, idx, ddd);
  FloatVectorSet::Statistics(v, idx, k, "before");

  for (int i=0; i<k+1 && idx[i]>=0; i++) {
    FloatVector *vec = Get(idx[i]);

    if (v.LabelsMatch(*vec))
      vec->MoveTowards(v, a);
    else
      vec->MoveTowards(v, -a);
  }

  NearestNeighbors(v, idx, ddd);
  FloatVectorSet::Statistics(v, idx, k, "after rule3");
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Adapt4(const FloatVector& v, int k, double) {
  IntVector idx(k);
  FloatVector ddd;
  NearestNeighbors(v, idx, ddd);
  FloatVectorSet::Statistics(v, idx, k, "before");

  bool add = false;
  for (int i=0; i<k && !add; i++) {
    if (idx[i]<0) {
      add = true;
      break;
    }

    FloatVector *vec = Get(idx[i]);

    if (!v.LabelsMatch(*vec))
      add = true;
  }
  if (add)
    FloatVectorSet::AppendCopy(v);

  FloatVectorSet::Statistics(v, idx, k, "after rule4");
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::PerformanceTestHead(TestMode mode,
					     StatQuadVector& res,
					     int add, int& iter, int& rep) {
  if (iter==-1)
    iter = DefaultPerformanceIterations();

  if (rep==0)
    rep = DefaultPerformanceRepetitions();

  if (!add && !res.Length())
    res.Length(TestModeLength(mode, iter));
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::PerformanceTestLoop(TestMode mode,
					     FloatVectorSet& train,
					     FloatVectorSet& test,
					     StatQuadVector& res,
					     int iter) {
  train.PrintNumber(!Silent());
  test.PrintNumber( !Silent());

  train.PrintText(Silent()?NULL:"");
  test.PrintText( Silent()?NULL:"");

  train.RandomlyReorder();

  if (do_eveninit)
    EvenInit(train, InsiderInit());
  else
    RandomInit(train[0].Length());

  InitializeParameters();

  if (!res.Length()) {
    const char *tmpname = ResultFileName(NULL, test.FileName());
    ifstream tmp(tmpname);
    if (tmp)
      res.Read(tmpname);
    else
      res.Length(TestModeLength(mode, iter));
  }

  int b = 0, j = -1;
  if (mode&InitialToo || (mode&FinalOnly && iter==0)) {
    SetPerformances(mode, train, test, res[b++]);
    goto loop_start;
  }

  for (j = 0; j<iter; j++) {
    Adapt(train);
    
    if (!(mode&FinalOnly) || j==iter-1)
      SetPerformances(mode, train, test, res[b++]);

  loop_start:
    res.Write(ResultFileName(NULL, test.FileName(), j<iter-1));
  }

  if (b!=TestModeLength(mode, iter))
    cerr << "Classifier::PerformanceTestLoop b==" << b
	 << " l==" << TestModeLength(mode, iter) << endl;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::PerformanceTest(TestMode mode,
					 FloatVectorSet& train,
					 FloatVectorSet& test,
					 StatQuadVector& res,
					 int add, int iter, int rep) {

  PerformanceTestHead(mode, res, add, iter, rep);

  for (int i=0; i<rep; i++)
    PerformanceTestLoop(mode, train, test, res, iter);
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::PerformanceTest(TestMode mode,
					 FloatVectorSet& set,
					 int /*test_size*/,
					 StatQuadVector& res,
					 int add, int iter, int rep) {

  char trainfn[10000], testfn[10000];

  sprintf(trainfn, "rand=train=%s", set.FileName());
  sprintf(testfn,  "rand=test=%s",  set.FileName());

  PerformanceTestHead(mode, res, add, iter, rep);

  for (int i=0; i<rep; i++) {
    FloatVectorSet train, test;
    set.SharedCopyTo(train);
    train.RandomlySplitTo(test, PerformanceSetBlockSize());

    train.FileName(trainfn);
    test.FileName(testfn);

    PerformanceTestLoop(mode, train, test, res, iter);
  }
}

///////////////////////////////////////////////////////////////////////////////

const char *Classifier::ResultFileName(const char *train,
					       const char *test, int snap) {
  if (result_file)
    return result_file;

  static char tmp[1000];

  strcpy(tmp, Identity(train));
  strcat(tmp, "-");
  strcat(tmp, test);
  strcat(tmp, snap ? ".snapshot" : ".result");

  return tmp;
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::TestModeLength(TestMode mode, int n) {
  if (mode&FinalOnly)
    n = 1;

  if (mode&InitialToo)
    n++;

  int l = mode&TrainTestError ? n : 0;

//   if ((mode&TrainTestError)==TrainTestError)
//     l *= 2;
// 
//   if (mode&Reject)
//     l *= 2;

  return l;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::SetPerformances(TestMode mode,
					 FloatVectorSet& train,
					 FloatVectorSet& test,
					 StatQuad& res) {
  if (mode&TrainError) {
    ClearConfusionMatrix();
    Classify(train);
    ShowResults(Silent());

    if (mode&Reject)
      res.TrainReject(RejectionRatio());

    res.TrainError(ErrorRatio());
  }
  if (mode&TestError) {
    ClearConfusionMatrix();
    Classify(test);
    ShowResults(Silent());

    if (mode&Reject)
      res.TestReject(RejectionRatio());

    res.TestError(ErrorRatio());
  }

  Plot();
}

///////////////////////////////////////////////////////////////////////////////

const char *Classifier::Identity(const char *fn) {
  char tmp[1000];
  sprintf(tmp, "knn-%s-%s", fn ? fn : FileName(), Configuration());
  CopyString(identity, tmp);

  return identity;
}

///////////////////////////////////////////////////////////////////////////////

Classifier *Classifier::CopyForCrossValidation(int cv) const {
  Classifier *cls = new Classifier();
  cls->Init(this, cv);

  return cls;
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::Create() {
  FloatVectorSet::Delete();
  
  if (!TrainingSet()) {
    cerr << "Classifier::Create needs training set" << endl;
    return FALSE;
  }

  TrainingSet()->First();
  const FloatVector *vec;
  while ((vec = TrainingSet()->Next()))
#if SIMPLE_FEAT
    if (HasFeatureExtraction())
      FloatVectorSet::AppendCopy(Extract(*vec));
    else
#endif // SIMPLE_FEAT
      FloatVectorSet::AppendCopy(*vec);

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::CopyCvSetsFrom(const Classifier& s) {
  if (NCrossValidation() != s.NCrossValidation()) {
    ShowError("Classifier::CopyCvSetsFrom failed");
    return;
  }

  for (int i=0; i<NCrossValidation(); i++) {
    Classifier *dst = CrossValidation(i);
    Classifier *src = s.CrossValidation(i);
    dst->TrainingSet(src->TrainingSet()->MakeSharedCopy());
    if (src->TestingSet())
      cerr << "Classifier::CopyCvSetsFrom ??? " << endl;
    dst->TestingSet(NULL);
  }
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::CreateCrossValidation(int n) {
  DeleteCrossValidation();

  if (!TrainingSet()) {
    cerr << "Classifier::CreateCrossValidation training set needed" << endl;
    return FALSE;
  }

  while (n--)
    cross_val.Append(CopyForCrossValidation(FALSE));

  for (int i=0; i<NCrossValidation(); i++) {
    int s = i*TrainingSet()->Nitems()/NCrossValidation();
    int e = (i+1)*TrainingSet()->Nitems()/NCrossValidation();

    CrossValidation(i)->Silent(CrossValidationSilent());
    CrossValidation(i)->TrainingSet(TrainingSet()->MakeSharedCopy());
    CrossValidation(i)->TrainingSet()->CrossValidationBegin(s);
    CrossValidation(i)->TrainingSet()->CrossValidationEnd(e);
    CrossValidation(i)->TrainingSet()->CrossValidationTrain(TRUE);
    CrossValidation(i)->TestingSet(NULL);

    if (silent<2)
      cout << "creating cv " << i << "/" << NCrossValidation() << " " << flush;

    CrossValidation(i)->Create();

    if (silent<2)
      cout << "\r" << flush;
  }

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::DeleteCrossValidation() {
  for (int i=0; i<NCrossValidation(); i++) {
    delete CrossValidation(i)->TrainingSet();
    CrossValidation(i)->TrainingSet(NULL);
  }
  cross_val.Delete();

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

StatVar Classifier::CrossValidationTraining() {
  StatVar res;

  for (int i=0; i<NCrossValidation(); i++) {
    CrossValidation(i)->TrainingSet()->CrossValidationTrain(TRUE);
    CrossValidation(i)->BeforeClassifyTrainingSet();
    CrossValidation(i)->Adapt();
    CrossValidation(i)->AfterClassifyTrainingSet();
    res.Add(CrossValidation(i)->ErrorRatio());
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

StatVar Classifier::CrossValidationTesting() {
  StatVar res;

  for (int i=0; i<NCrossValidation(); i++) {
    CrossValidation(i)->TrainingSet()->CrossValidationTest(TRUE);
    CrossValidation(i)->Classify();
    res.Add(CrossValidation(i)->ErrorRatio());
    AddToConfusionMatrix(CrossValidation(i)->GetConfusionMatrix());
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

StatQuad Classifier::UpToThreeTests() {
  StatQuad res;

  if (DoCrossValidationTraining()) {
    UseCrossValidationConfusionMatrix();
    ClearConfusionMatrix();
    // error_impulses = ;
    res.CrossValidationError(CrossValidationTraining());
  }

  if (DoCrossValidationTesting()) {
    UseCrossValidationConfusionMatrix();
    ClearConfusionMatrix();
    // error_impulses = ;
    res.CrossValidationError(CrossValidationTesting());
    ShowResults();
  }

  if (DoTrainingSetTesting()) {
    UseTrainingSetConfusionMatrix();
    ClearConfusionMatrix();
    // error_impulses = &train_errors;
    BeforeClassifyTrainingSet();
    res.TrainError(Classify(*TrainingSet()));
    AfterClassifyTrainingSet();
    ShowResults();
  }

  if (DoTestingSetTesting()) {
    UseTestingSetConfusionMatrix();
    // error_impulses = &test_errors;
    res.TestError(Classify());
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

int Classifier::OptimalVectorLength(int beg, int end) {
  if (end>VectorLength())
    end = VectorLength();

  StatQuadVector res(end-beg+1);

  for (int len=end; len>=beg; len--) {
    Lengthen(len);
    VectorLengthChanged();

    res[len-beg] = UpToThreeTests();
    res[len-beg].X() = len;
    Intermediates(res);
  }

  return beg+OptimizationError().ArgMin();
}

///////////////////////////////////////////////////////////////////////////////

OptVar Classifier::DoOptimizeVectorLength(const OptVar& v) {
  int beg = v.Argc()>1 ? atoi(v.Argv(1)) : 1;
  int end = v.Argc()>2 ? atoi(v.Argv(2)) : VectorLength();
  int stp = v.Argc()>3 ? atoi(v.Argv(3)) : 1;

  if (end>VectorLength())
    end = VectorLength();

  VectorSetOf<StatQuad> list;
  StatQuadVector res;
  OptVar ret, var = v;

  for (int len=end; len>=beg; len-=stp) {
    Lengthen(len);
    VectorLengthChanged();

    var.Ival(len).SetDescriptions();
    OptVar r = Optimize(var.Next());
    var.Result(r.Result()).PrependTo(res);

    if (var.HasNext())
      var.Next(r);

    if (OptimizationError(res).ArgMin()==0)
      ret = var;

    Intermediates(res, list, var);
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

OptVar Classifier::DoOptimizeK(const OptVar& v) {
  int beg = v.Argc()>1 ? atoi(v.Argv(1)) : 1;
  int end = v.Argc()>2 ? atoi(v.Argv(2)) : VectorLength();
  int stp = v.Argc()>3 ? atoi(v.Argv(3)) : 1;

  VectorSetOf<StatQuad> list;
  StatQuadVector res;
  OptVar ret, var = v;

  for (int k=beg; k<=end; k+=stp) {
    DefaultK(k);

    var.Ival(k).SetDescriptions();
    OptVar r = Optimize(var.Next());
    var.Result(r.Result()).AppendTo(res);

    if (var.HasNext())
      var.Next(r);

    if (OptimizationError(res).ArgMin()==res.Length()-1)
      ret = var;

    Intermediates(res, list, var);
  }

  return ret; 
}

///////////////////////////////////////////////////////////////////////////////

OptVar Classifier::DoOptimize(const OptVar& v) {
  if (v.NamesMatch("VectorLength") && v.Type()==OptVar::OptInt)
    return DoOptimizeVectorLength(v);

  else if (v.NamesMatch("K") && v.Type()==OptVar::OptInt)
    return DoOptimizeK(v);

  cerr << "Classifier::DoOptimize failed to solve: " << endl;
  v.Dump(DumpRecursiveLong);

  return OptVar();
}

///////////////////////////////////////////////////////////////////////////////

OptVar Classifier::SortOptVars(const OptVar& var) {
  OptVar ret = var;

  ret.Order("VectorLength", "K");

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

OptVar Classifier::Optimize(const OptVar* var) {
  if (!TrainingSet()) {
    ShowError("Classifier::Optimize needs training set.");
    return OptVar();
  }

  if (!var)
    return OptVar(UpToThreeTests());

  OptVar sorted = SortOptVars(*var);
  sorted.SetDescriptions();

  return DoOptimize(sorted);
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Intermediates(const StatQuadVector& vec, 
				       VectorSetOf<StatQuad>& list,
				       OptVar& ov) {
  if (ov.HasNext()) {
    list.AppendCopy(Results());
    list.Peek()->Label(ov.Value());
    char tmp[2000];
    sprintf(tmp, "%s.all", ov.FileName());
    list.Write(tmp);
  }

  Intermediates(vec, &ov);
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Intermediates(const StatQuadVector& vec, OptVar *ov) {
  //  vec[0].Dump();  vec[1].Dump();
  results = vec;
  // results[0].Dump();  results[1].Dump();
  // Plot();  // this is better done with auxilary plotstatquadvec

  if (ov)
    results.Write(ov->FileName());

  else if (result_file) // sic!
    results.Write(ResultFileName());
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Plot(TestMode type, StatQuadVector *vec) {
  GnuPlot *gp = new GnuPlot();
  
  switch (type&TrainTestRejectError) {
  case TrainError:             // 1
  case TestError:              // 2
  case Reject:                 // 4
    gp->Plot(vec);
    break;

  case TrainTestError:         // 3
    gp->PlotTrainTest(vec);
    break;

  case TrainRejectError:       // 5
  case TestRejectError:        // 6
  case TrainTestRejectError:   // 7
    gp->PlotRejectError(vec);
    break;

  default:                     // 0
    delete gp;
    return;
  }

  plot.Append(gp);
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Plot() {
  for (int i=0; i<plot.Nitems(); i++)
    plot[i].RepeatPreviousPlot();
}

///////////////////////////////////////////////////////////////////////////////

const char *Classifier::ClassifyRawVector(const FloatVector& v,
						  float *d) {
  if (!feat_ext)
    abort();
    
#if SIMPLE_FEAT
  raw_vector = (FloatVector*) &v;
  const char *ret = ClassifyVector((*feat_ext)(v), d);
  raw_vector = NULL;
  return ret;
#else
  if (d)
    *d = v.Sum();
  return NULL;
#endif // SIMPLE_FEAT
}

///////////////////////////////////////////////////////////////////////////////

#if SIMPLE_FEAT
const char *Classifier::ClassifyRawGreyImage(const GreyImage& i,
						     float *d) {
  if (!feat_ext)
    abort();

  return ClassifyVector((*feat_ext)(i), d);
}
#endif // SIMPLE_FEAT

///////////////////////////////////////////////////////////////////////////////

FloatVectorSet Classifier::ClassifiedTo(FloatVectorSource& s,
						   const char *lab) {

  FloatVectorSet res;
  const FloatVector *vec;
  s.First();
  while ((vec = s.Next())) {
    const char *cls = ClassifyVector(*vec);
    AddToConfusionMatrix(vec->Label(), cls);

    if (StringsMatch(lab, cls))
      res.Append(new FloatVector(*vec));
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

FloatVectorSet Classifier::ClassifiedWrongly(FloatVectorSource&
							s) {
  FloatVectorSet res;
  const FloatVector *vec;
  s.First();
  while ((vec = s.Next())) {
    const char *cls = ClassifyVector(*vec);
    AddToConfusionMatrix(vec->Label(), cls);

    if (!vec->LabelsMatch(cls))
      res.Append(new FloatVector(*vec));
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////

StatQuad Classifier::ResultByParameter(double r) const {
  int idx = -1, i;
  for (i=0; i<Results().Length(); i++)
    if (Results()[i].X()==r) {
      if (idx == -1)
	idx = i;
      else
	break;
    }

  if (idx==-1 || i<Results().Length()) {
    cerr << "Classifier::ResultByParameter failed" << endl;
    return StatQuad();
  }

  return Results()[idx];
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::ComponentEqualization(int e) {
  do_comp_equal = e;

#if SIMPLE_FEAT
  if (e && TrainingSet()) {
    FloatVector m = TrainingSet()->Mean();
    FloatVector d = TrainingSet()->StandardDeviation();
    VectorTransform *addmul = new AddMul(-m, d.Inv());
    PrependTransform(addmul);

    for (int i=0; i<Nitems(); i++)
      *Get(i) = addmul->Apply(*Get(i));
  }
#else
  ShowError("Classifier::ComponentEqualization SIMPLE_FEAT not defined.");
#endif // SIMPLE_FEAT
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::ClassSpecificMetrics(int e) {
  do_class_metrics = e;

  if (e && TrainingSet()) {
    ListOf<FloatMatrix > cov = TrainingSet()->AutoCovList();

    for (int i=0; i<cov.Nitems(); i++)
      cov[i] = cov[i].Inverse();

    SetMetrics(cov);

  } else
    SetMetrics(ListOf<FloatMatrix >());
}

///////////////////////////////////////////////////////////////////////////////

FloatVector Classifier::FeatureWeighting(int m, double a,
						    int dump) const {
  if (!TrainingSet()) {
    cerr << "Classifier::FeatureWeighting() needs trainingset" << endl;
    return FloatVector();
  }

  IntVector typ[4];
  typ[0].Length(VectorLength());
  typ[1].Length(VectorLength());
  typ[2].Length(VectorLength());
  typ[3].Length(VectorLength());

  const FloatVector *vec;
  TrainingSet()->First();

  FloatVector mul[2];
  mul[0].Length(VectorLength());
  mul[1].Length(VectorLength());
  IntVector cnt[2];
  cnt[0].Length(VectorLength());
  cnt[1].Length(VectorLength());
  
  while ((vec = TrainingSet()->Next())) {
    FloatVector *ov, *rv;
    Nearests(*vec, ov, rv);

    if (!ov || !rv)
      continue;
    
    double od = (*vec-*ov).NormSquared();
    double rd = (*vec-*rv).NormSquared();
    
    for (int i=0; i<VectorLength(); i++) {
      double oc = (float)((*vec)[i]-(*ov)[i]); oc *= oc;
      double rc = (float)((*vec)[i]-(*rv)[i]); rc *= rc;
      
      int j = 3;
      if (od-oc<rd-rc)
	j = oc>rc;
      else if (oc<rc)
	j = 2;
      typ[j][i]++;

      if (j==1 || j==2) {
	double s = log((rd-rc-od+oc)/(oc-rc));
	mul[j-1][i] += s ; // j==1 ? -s : s;
	cnt[j-1][i]++;
      }
    }
  }
    
  for (int l=0; l<VectorLength(); l++) {
    if (dump)
      cout << l << "  "
	   << typ[0][l] << " " << typ[1][l] << " "
	   << typ[2][l] << " " << typ[3][l] << "  "
	   << mul[0][l] << " " << cnt[0][l] << " "
	   << mul[1][l] << " " << cnt[1][l] << "  ";

    float r = 0;

    switch (m) {
    case 0:
      if (cnt[0][l]+cnt[1][l])
	r = (mul[0][l]+mul[1][l])/(cnt[0][l]+cnt[1][l]);
      break;

    case 1:
      if (cnt[0][l] && cnt[1][l])
	r = (mul[0][l]/cnt[0][l]+mul[1][l]/cnt[1][l])/2;
      break;

    case 2:
      if (cnt[1][l])
	r = mul[1][l]/cnt[1][l];
      break;

    default:
      cerr << "Classifier::FeatureWeighting() unknown mode" << endl;
    }

    mul[0][l] = exp(0.5*a*r);

    if (dump)
      cout << mul[0][l] << endl;
  }

  return mul[0];
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::WilsonEdit(int n) {
  while (n-->0) {
    IntVector ok(Nitems());

    for (int i=0; i<Nitems(); i++)
      ok[i] = IsCorrectlyClassified(*Get(i));
    
    for (int j=Nitems()-1; j>=0; j--)
      if (!ok[j])
	Remove(j);
  }
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::Condensing() {
  FloatVectorSet gb;
  for (int i=0; i<Nitems(); i++)
    gb.Append(Get(i));
  Empty();

  for (;;) {
    int n = 0;

    for (int j=0; j<gb.Nitems(); j++)
      if (!IsCorrectlyClassified(*gb.Get(j))) {
	Append(gb.Get(j));
	gb.Relinquish(j);
	n++;
      }

    if (!n)
      break;

    gb.RemoveNotOwns();
  }
}

///////////////////////////////////////////////////////////////////////////////

IntVector Classifier::DistanceClusters(const FloatMatrix& d, int n, double mx,
				       ListOf<IntVector> *ords) {
  if (!d.IsSquare()) {
    ShowError("Classifier::DistanceClusters matrix should be square");
    return IntVector();
  }
  
  if (n>d.Rows()) {
    ShowError("Classifier::DistanceClusters decreasing n of clusters");
    n = d.Rows();
  }

  int zero = 0;
  IntVector clu1(d.Rows()), cen(1, &zero), clu2;
  for (int nn=1; nn<n; nn++) {
    cen  = DistClustCenters(d, clu1, mx);

    clu2 = DistClustDivide(d, cen);
    FloatVector all(d.Rows());
    for (int l=0; l<d.Rows(); l++)
      all[l] = d(l, cen[clu2[l]]);
    all.SortIncreasingly();
    char tmpname[1000];
    sprintf(tmpname, "clust_dist_%d.m", nn);
    all.WriteMatlab(tmpname);
    
    clu1 = DistClustSplit(d, cen);

    for (int i=0; i<20; i++) {
      cen  = DistClustCenters(d, clu1, mx);
      clu2 = DistClustDivide(d, cen);

//       clu2.Dump(DumpRecursiveLong);
//       IntVector sizes(nn+1);
//       for (int i=0; i<nn+1; i++)
//  	 sizes[i] = clu2.NumberOfEqual(i);
//       sizes.Dump(DumpRecursiveLong);

      int neq = clu2.NumberOfEqual(clu1);
      clu1 = clu2;
      if (neq==clu2.Length())
	break;
    }
  }

  IntVector sizes(n), idx(n);
  for (int i=0; i<n; i++)
    sizes[i] = clu1.NumberOfEqual(i);
  if (sizes.Sum()!=d.Rows()) {
    char tmp[100];
    sprintf(tmp, "sizes.Sum()=%d d.Rows()=%d", sizes.Sum(), d.Rows());
    ShowError("Classifier::DistanceClusters missing/extra units??? ", tmp);
    sizes.Dump(DumpRecursiveLong);
  }

  idx.SetIndices();
  sizes.SortDecreasingly(idx);
  IntVector ord = idx.FindIndices();

  if (!idx.IndexOK(idx) || !idx.AreUnique())
    SimpleAbort();
  if (!ord.IndexOK(ord) || !ord.AreUnique())
    SimpleAbort();

  for (int j=0; j<clu1.Length(); j++)
    clu1[j] = ord[clu1[j]];

  if (ords) {
    ords->Delete();
    for (int k=0; k<clu1.Length(); k++) {
      while (clu1[k]>=ords->Nitems())
	ords->Append(new IntVector);
      (*ords)[clu1[k]].Append(k);
    }

    for (int l=0; l<ords->Nitems(); l++) {
      FloatVector dd(ords->Get(l)->Length());
      for (int m=0; m<ords->Get(l)->Length(); m++) {
	int p = ords->Get(l)->Get(m);
	dd[m] = d(p, cen[idx[clu1[p]]]);
      }
      IntVector sort(ords->Get(l)->Length());
      sort.SetIndices();
      dd.SortIncreasingly(sort);
      ords->Get(l)->Arrange(sort);
    }
    IntVector foo;
    for (int ll=0; ll<ords->Nitems(); ll++)
      foo.Append(*ords->Get(ll));
    if (!foo.IndexOK(foo))
      SimpleAbort();
    if (!foo.AreUnique())
      SimpleAbort();
  }

  return clu1;
}

///////////////////////////////////////////////////////////////////////////////

IntVector Classifier::DistClustCenters(const FloatMatrix& d,
				       const IntVector& c, double mx) {
  if (!d.IsSquare() || d.Rows()!=c.Length()) {
    ShowError("Classifier::DistClustCenters dimension mismatch");
    return IntVector();
  }

  IntVector cen;

  for (int i=0; i<c.Length(); i++) {
    FloatVector fset(c.Length());
    bool found = false;
    for (int k=0; k<fset.Length(); k++)
      if (c[k]==i) {
	fset[k] = 1;
	found = true;
      }
    if (!found)
      break;

    FloatVector norm(d.Columns());
    for (int j=0; j<d.Columns(); j++)
      if (fset[j]) {
	FloatVector v = fset**d.GetColumn(j);
	for (int q=0; q<v.Length(); q++)
	  if (v[q]==MAXFLOAT)
	    v[q] = 0;
	norm[j] =  v.Norm(mx);
      } else
	norm[j] = MAXFLOAT;

    cen.Append(norm.ArgMin());
    if (cen.Peek()<0 || cen.Peek()>=d.Columns())
      SimpleAbort();
  }

  return cen;
}

///////////////////////////////////////////////////////////////////////////////

IntVector Classifier::DistClustDivide(const FloatMatrix& d,const IntVector& c){
  if (!d.IsSquare()) {
    ShowError("Classifier::DistClustDivide dimension mismatch");
    return IntVector();
  }
  
  int q;
  for (q=0; q<c.Length(); q++)
    if (c[q]<0 || c[q]>=d.Rows())
      SimpleAbort();

  IntVector clu(d.Rows());
  for (int i=0; i<d.Rows(); i++) {
    bool found = false;
    FloatVector dist(c.Length());
    for (int j=0; j<c.Length(); j++) {
      if (!d.IndexOK(i, c[j])) {
	ShowError("Classifier::DistClustDivide indices not ok");
	SimpleAbort();
      }
      dist[j] = d.Get(i, c[j]);
      if (dist[j]!=MAXFLOAT)
	found = true;
    }

    clu[i] = found ? dist.ArgMin() : 0;
  }
  return clu;
}

///////////////////////////////////////////////////////////////////////////////

IntVector Classifier::DistClustSplit(const FloatMatrix& d, const IntVector& c){
  if (!d.IsSquare()) {
    ShowError("Classifier::DistClustSplit dimension mismatch");
    return IntVector();
  }

  if (d.Rows()<=c.Length()) {
    ShowError("Classifier::DistClustSplit cannot split further");
    return IntVector();
  }

  IntVector ret = DistClustDivide(d, c);

  int q;
  for (q=0; q<ret.Length(); q++)
    if (ret[q]<0 || ret[q]>=c.Length())
      SimpleAbort();

  FloatVector all(d.Rows());
  for (int l=0; l<d.Rows(); l++)
    all[l] = d(l, c[ret[l]]);

  IntVector idx(all.Length());
  idx.SetIndices();
  all.SortIncreasingly(idx);

  FloatVector tent(all.Length());
  IntVector test(all.Length());
  for (int q2=0; q2<all.Length(); q2++) {
    test[idx[q2]] = 1;
    IntVector cents = DistClustCenters(d, test);
    if (q2!=test.Length()-1 && cents.Length()!=2)
      SimpleAbort();

    double max = 0;
    for (int qq=0; qq<all.Length(); qq++)
      if (test[qq]==0 && d(qq, cents[0])>max)
	max = d(qq, cents[0]);

    tent[q2] = max;
  }
  char tmpname[1000];
  static int nn;
  sprintf(tmpname, "clust_temp_%d.m", ++nn);
  tent.WriteMatlab(tmpname);

  tent += all;
  if (tent.Length()>=6)
    for (int qq=0; qq<3; qq++)
      tent[qq] = tent[tent.Length()-1-qq] = MAXFLOAT;
  double limit = all[tent.ArgMin()]; 

//   FloatVector diff = all.Difference();
//   int mini=(int)floor(0.5*diff.Length()),maxi=(int)floor(0.9*diff.Length());
//   for (int i=0; i<diff.Length(); i++)
//     if (i<=mini || i>maxi)
//       diff[i] = 0;
//   double limit = all[diff.ArgMax()];

  for (int k=0; k<d.Rows(); k++)
    if (d(k, c[ret[k]])>limit)
      ret[k] = c.Length();

  for (q=0; q<ret.Length(); q++)
    if (ret[q]<0 || ret[q]>c.Length())
      SimpleAbort();

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::AcceptLabels(const char *pattern) {
  CopyString(accept_labels_pattern, pattern);

  if (HasAcceptLabels())
    regfree(&accept_labels);

  if (pattern) {
    int res = regcomp(&accept_labels, pattern, REG_EXTENDED|REG_NOSUB);
    if (res) {
      char tmp[1000];
      sprintf(tmp, "%d", res);
      ShowError("Classifier::AcceptLabels() regcomp() returned ", tmp);
      accept_labels.re_nsub = MAXINT;
    }
  } else
    accept_labels.re_nsub = MAXINT;
}

///////////////////////////////////////////////////////////////////////////////

bool Classifier::IsLabelAcceptable(const char *lab) const {
  if (HasAcceptLabels()) {
    int res = regexec(&accept_labels, lab, 0, NULL, REG_NOTBOL|REG_NOTEOL);
    if (!res || res==REG_NOMATCH)
      return !res;

    char tmp[1000];
    sprintf(tmp, "%d", res);
    ShowError("Classifier::AcceptLabels() regcomp() returned ", tmp);
    return false;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

IntVectorSet Classifier::ListAcceptableLabels() const {
  IntVectorSet ret;
  if (accept_labels_pattern)
    for (const char *ptr = accept_labels_pattern; *ptr; ptr++) {
      if (*ptr == '[' || *ptr == ']')
	continue;
      if (*(ptr+1) == '-' && strlen(ptr)>2 && *(ptr+2)!=']') {
	for (char ch = *ptr; ch <= *(ptr+2); ch++) {
	  char str[2] = { ch, 0 };
	  ret.Append(new IntVector(0, NULL, str));
	}
	ptr += 2;
      } else {
	char str[2] = { *ptr, 0 };
	ret.Append(new IntVector(0, NULL, str));
      }
    }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::kNNinitialize(FloatVectorSet& kset, int kk) {
  if (kset.Nitems())
    ShowError("Classifier::kNNinitialize set already in use");

  float max = MAXFLOAT;
  FloatVector tmp(1, &max);

  kset.Delete();
  while (kk-->0)
    kset.AppendCopy(tmp);
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::kNNinsert(FloatVectorSet& kset, const FloatVector& vec,
			   double v) {
  if (v>=kNNlimit(kset))
    return;

  if (!vec.Label()) 
    ShowError("Classifier::kNNinsert no label");

  for (int i=0; i<kset.Nitems(); i++)
    if (v<kset[i][0]) {
      float vv = v;
      kset.Insert(i, new FloatVector(1, &vv, vec.Label()));
      kset.Remove(kset.Nitems()-1);
      kset[i].Number(vec.Number());
      break;
    }
}

///////////////////////////////////////////////////////////////////////////////

const char *Classifier::kNNresult(const FloatVectorSet& kset, int k) {
  if (!kset.Nitems() || kset[0][0]==MAXFLOAT)
    return NULL;

  if (!k)
    k = kset.Nitems();
  if (k>kset.Nitems()) {
    k = kset.Nitems();
    ShowError("Classifier::kNNresult k too large");
  }

  IntVector count(k);
  int i, j = -1, max = 0;

  for (int kk=k; kk>0; kk--) {
    count.Zero();

    for (i=0; i<kk; i++) {
      if (kset[i][0]==MAXFLOAT)
	break;

      if (!kset[i].Label()) {
	ShowError("Classifier::kNNresult no label");
	continue;
      }

      for (j=0; j<i; j++)
	if (!kset[i].LabelsMatch(kset[j]))
	  break;

      count[j]++;
    }
    
    max = count.Maximum(&j);
    if (max==0) {
      ShowError("Classifier::kNNresult max==0");
      return NULL;
    }

    if (count.NumberOfEqual(max)==1) 
      return kset[j].Label();
  }
  ShowError("Classifier::kNNresult error???");

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

double Classifier::kNNlimit(const FloatVectorSet& kset) {
  return kset.Nitems() ? kset.Peek()->Get(0) : -MAXFLOAT;
}

///////////////////////////////////////////////////////////////////////////////

void Classifier::kNNend(FloatVectorSet& kset) {
  kset.Delete();
}

///////////////////////////////////////////////////////////////////////////////

Classifier *Classifier::CreateFromName(const char *str) {
  char name[1024] = "", file[1024] = "";
  const char *delim = " \t),", *ptr;

  for (ptr = str; *ptr; ptr++) {
    name[ptr-str] = *ptr;
    if (strchr(delim, *ptr)) {
      name[ptr-str] = 0;
      break;
    }
  }

  while (*ptr==' ' || *ptr=='\t') ptr++;

  if (*ptr=='(') {
    ptr++;
    while (*ptr==' ' || *ptr=='\t') ptr++;
    
    for (str = ptr; *ptr; ptr++) {
      file[ptr-str] = *ptr;
      if (strchr(delim, *ptr)) {
	file[ptr-str] = 0;
	break;
      }
    }
    while (*ptr==' ' || *ptr=='\t') ptr++;
    if (*ptr!=')') {
      cerr << "Classifier::Create couldn't find matching ) in " << str << endl;
      return NULL;
    }
  }

  const char *fp = *file ? file : NULL;
  Classifier *c = NULL;

  if (!strcmp(name, "Classifier"))         c = new Classifier(fp);

#if SIMPLE_CLASS_SSC
  if (!strcmp(name, "SubSpaceClassifier")) c = new SubSpaceClassifier(fp);
#endif // SIMPLE_CLASS_SSC

#if SIMPLE_CLASS_SOM
  if (!strcmp(name, "SOM"))                c = new SOM(fp);
#endif // SIMPLE_CLASS_SOM

#if SIMPLE_CLASS_LVQ
  if (!strcmp(name, "LVQ"))                c = new LVQ(fp);
#endif // SIMPLE_CLASS_LVQ

#if SIMPLE_CLASS_COMMITTEE
  if (!strcmp(name, "Committee"))          c = new Committee(fp);
#endif // SIMPLE_CLASS_COMMITTEE

  if (!c) {
    cerr << "Classifier::Create couldn't create " << name << "(" << fp << ")"
	 << endl;
    return NULL;
  }
    
  // some configurations to be added...

  return c;
}

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

// bool Classifier::() const {
// 
// }

///////////////////////////////////////////////////////////////////////////////

} // namespace simple

#endif // _CLASSIFIER_C_
