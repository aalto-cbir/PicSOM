#ifndef _LBMODEL_H_
#define _LBMODEL_H_

#include <Segmentation.h>

#include <cox/matrix>

namespace picsom {
  class LBModel {
  public:
    LBModel() {}

    LBModel(int a, int b) : dim_origin(a), 
      dim_subspace(b),
      Mf(a),
      projection(a,b),
      lamdaM(b) {
	rho = 0.0;
      };
    	
    ~LBModel() {}
    
    int dim_origin;
    int dim_subspace;
    vector<float> Mf;
    cox::matrix<float> projection;
    vector<float> lamdaM;
    float rho;
   
    static LBModel* ReadModel(string fname) {
      int dim_origin, dim_subspace;
      FILE* fp = fopen(fname.c_str(), "r");

      if (fscanf(fp, "%d\n", &dim_origin)!=1 ||
          fscanf(fp, "%d\n", &dim_subspace)!=1)
	return NULL;

      LBModel* pModel = new LBModel(dim_origin, dim_subspace);
      
      int i,j;
      float tmp1;
      vector<float> tmp2(dim_subspace);
      
      for (i=0;i<dim_origin;i++) {
    	if (fscanf(fp, "%f\n", &tmp1)!=1)
          return NULL;
    	pModel->Mf[i] = tmp1;
      }
      
      for (i=0;i<dim_origin;i++){
	for (j=0;j<dim_subspace;j++){      
	  if (fscanf(fp, "%f ", &tmp1)!=1)
	    return NULL;
	  tmp2[j] = tmp1;
	}
	pModel->projection.set_row(i, tmp2);
      }

      for (i=0;i<dim_subspace;i++) {
        if (fscanf(fp, "%f\n", &tmp1)!=1)
	  return NULL;
    	pModel->lamdaM[i] = tmp1;
      }
    
      if (fscanf(fp, "%f\n", &tmp1)!=1)
        return NULL;
      pModel->rho = tmp1;
    
      fclose(fp);
    
      return pModel;
    }

    static vector<float> PrepareVector(imagedata& idata){
      using cox::matrix;
      int h = idata.height();
      int w = idata.width();
      
      matrix<float> I(idata.get_float(), h, w);
      vector<float> X = idata.get_float();

      matrix<float> Ih(h-1,w);
      for (int r=0;r<h-1;r++)
	for (int c=0;c<w;c++)
	  Ih(r,c) = I(r+1,c) - I(r,c);
      
      matrix<float> Iv(h,w-1);
      for (int c=0;c<w-1;c++)
  	for (int r=0;r<h;r++)
  	  Iv(r,c) = I(r+1,c) - I(r,c);
      
      vector<float> Xh = Ih.get_vector();
      vector<float> Xv = Iv.get_vector();
      
      vector<float> Xr = I.row_sum();
      vector<float> Xc = I.column_sum();
      
      vector<float> tilde_X = normalize_vector(X);
      vector<float> tilde_Xh = normalize_vector(Xh);
      vector<float> tilde_Xv = normalize_vector(Xv);
      vector<float> tilde_Xr = normalize_vector(Xr);
      vector<float> tilde_Xc = normalize_vector(Xc);
      
      size_t dd = tilde_X.size() + tilde_Xh.size() + tilde_Xv.size() + tilde_Xr.size() + tilde_Xc.size();
      vector<float> tilde_Y(dd);
      int iy = 0;
      for (size_t i=0;i<tilde_X.size();i++) {
	tilde_Y[iy++] = tilde_X[i];
      }
      for (size_t i=0;i<tilde_Xh.size();i++) {
	tilde_Y[iy++] = tilde_Xh[i];
      }
      for (size_t i=0;i<tilde_Xv.size();i++) {
	tilde_Y[iy++] = tilde_Xv[i];
      }
      for (size_t i=0;i<tilde_Xr.size();i++) {
	tilde_Y[iy++] = tilde_Xr[i];
      }
      for (size_t i=0;i<tilde_Xc.size();i++) {
	tilde_Y[iy++] = tilde_Xc[i];
      }
      
      return normalize_vector(tilde_Y);
    }
    
    float Score(vector<float>& Y) {
      using cox::matrix;
      int N = projection.rows();
      int M = projection.cols();
      
      vector<float> zm = projection.left_trans_multiply(Y);
      
      float part1 = 0.0;
      for (int i=0;i<M;i++)
	part1 += zm[i] * zm[i] / lamdaM[i];
      
      float zm2 = matrix<float>::dot_product(zm, zm);
      float ym2 = 0.0;
      for (int i=0;i<N;i++)
	ym2 += (Y[i]-Mf[i]) * (Y[i]-Mf[i]);
      float part2 = (ym2-zm2) / rho;
      
      float part3 = 0.0;
      for (int i=0;i<M;i++)
	part3 += log(lamdaM[i]);
      
      float part4 = (N-M)*log(rho);
      
      return part1 + part2 + part3 + part4;
    }

    static vector<float> normalize_vector(vector<float>& v) {
    	int d = v.size();
    	float sum = 0.0;
    	for (int i=0;i<d;i++) {
    	  sum += v[i];
    	}
    	float mean = sum / d;
    	
    	float sum2 = 0.0;
    	for (int i=0;i<d;i++) {
    	  sum2 += (v[i] - mean) * (v[i] - mean);
    	}
    	float std = sqrt(sum2 / (d-1));
    	
    	vector<float> ret(d);
    	for (int i=0;i<d;i++) {
	  if (std>0)
	    ret[i] = (v[i] - mean) / std;
	  else
	    ret[i] = 0.0;
    	}
    	return ret;
    }
  };

  class LBBox {
  public:
    LBBox() : l(0), t(0), w(0), degree(0.0), deltaf(0.0), deltan(0.0) {}
    int l;
    int t;
    int w;
    float degree;
    float deltaf;
    float deltan;
    };  

} // namespace picsom

#endif // _LBMODEL_H_

// Local Variables:
// mode: font-lock
// End:
