#include <cstdio>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <cassert>

#include <iostream>
#include <algorithm>
#include <numeric>

#include <sys/timeb.h>

namespace PmSVM {

#define __USE_DOUBLE

#ifdef __USE_DOUBLE
	typedef double NUMBER;
	const double BIGVALUE = HUGE_VAL;
#else
	typedef float NUMBER;
	const float BIGVALUE = HUGE_VALF;
#endif


class model
{
public:
	NUMBER* _dec_values;
public:
	int nr_class;
	int nr_feature;
	NUMBER p; // p should be negative
	NUMBER *w;
	int *label;		/* label of each class */
public:
	model();
	~model();
public:
        int PredictValues(const int* ix,const NUMBER* vx,NUMBER *dec_values);
	int Predict(const int* ix,const NUMBER* vx);
	int FindLabel(const int newl) const;
};

class problem
{
private:
	int num_class;
	int* labels;
  //private:
public:
	size_t allocated;
	int* index_buf;
	NUMBER* value_buf;
  //private:
	int l, n;
	int *y;
	int** indexes;
	NUMBER** values;
public:
	problem();
	~problem();
	void Clear();
public:
        void Load(const char* filename,const NUMBER bias=0,const int maxdim=0);
	void Train(model& model_,const NUMBER C,const NUMBER p);
	void Clone(problem& dest);
	NUMBER CrossValidation(const int nr_fold,const NUMBER C,const NUMBER p);
public:
	int FindLabel(const int newl) const;
	int GetNumClass() { return num_class; }
	int GetNumExamples() { return l; }
	int GetLabel(const int index) { assert(index>=0 && index<l); return y[index]; }
	const int* GetFeatureIndexes(const int index) { assert(index>=0 && index<l); return indexes[index]; };
	const NUMBER* GetFeatureValues(const int index) { assert(index>=0 && index<l); return values[index]; };
private:
	void GroupClasses(int** start_ret=NULL, int** count_ret=NULL,int* perm=NULL);
	void Solve_l2r_l1l2_svc(NUMBER* w,const NUMBER C,const NUMBER p,const int solver_type,const NUMBER* fvalues,const NUMBER* logxplus) const;
};

}
