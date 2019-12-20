// $Id: Torch.C,v 1.7 2019/02/08 09:37:59 jormal Exp $	

#include <Torch.h>

#ifdef HAVE_THC_H

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// #pragma GCC diagnostic ignored "-Wsign-compare"
// #include <torch/data_transformer.hpp>
// #pragma GCC diagnostic pop

namespace picsom {
  static const char *vcid =
    "$Id: Torch.C,v 1.7 2019/02/08 09:37:59 jormal Exp $";

  static Torch list_entry(true);

  RwLock Torch::torchmaplock;

  map<string,Torch::torch_t> Torch::torchmap;

  //===========================================================================

  string Torch::Version() const {
    return vcid;
  }

  //===========================================================================

  bool Torch::InitializeTorch() {
    string msg = "Torch::InitializeTorch() : ";

    root = NULL;

    return true;
  }

  //===========================================================================

  /*

windowsize = WXxWY

layer = fc6 | ...

blocks = FUNC

FUNC = { FUNC + FUNC } | { max(LL) | avg(LL) | vlad(LL) | cat(LL) | SCALE | SCALE;layer }

LL = L | L;layer

L = SCALE:GRID | L,L

SCALE = SXxSY | SXxSYa | S

GRID = GXxGY GRID2 GRID3

GRID2 = | f | s | fs | sf

GRID3 = | %BXxBY

##

windowsize = 227x227

blocks = avg(256x256a:2x2fs;fc6)

blocks = 256+max(512:3x3)

blocks = cat(256;fc7)+max(512:3x3;fc6)

blocks = avg(256x256,512x512:2x2,768x768:4x4)

blocks = cat(256x256:2x2f%8x8)

# compatibility and conventions 

blocks = 256x256+avg(512x512:3x3)

*_d_c  :  blocks = 256x256
*_d_a  :  blocks = avg(256x256:2x2fs)
*_d_m  :  blocks = max(256x256:2x2fs)
*_a_c  :  blocks = 256x256a
*_a_a  :  blocks = avg(256x256a:2x2fs)
*_a_m  :  blocks = max(256x256a:2x2fs)

   */

  bool Torch::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "Torch::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="model") {
	model_name = value;
      
      	i = l.erase(i);
      	continue;
      }

      // obs! this and "blocks" in OCVLbp and CVC  should be harmonized and
      // moved to Feature
      if (key=="blocks") {
	blocks = value;
	
      	i = l.erase(i);
      	continue;
      }

      if (key=="layer") {
	layer = value;
      
      	i = l.erase(i);
      	continue;
      }

      if (key=="windowsize") {
	windowsize = value;
      
      	i = l.erase(i);
      	continue;
      }

      if (key=="dofiles") {
	dofiles.push_back(value);
      
      	i = l.erase(i);
      	continue;
      }

      i++;
    }

    return RegionBased::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  Torch::torch_t::torch_t(const string& na, const string& mo,  const string& me, 
			  const vector<pair<float,float> >& ms,
			  const vector<size_t>& ma,
			  const vector<string>& dofile) :
    name(na), model(mo), mean(me), L(NULL), meanstd(ms), map(ma), n_layers(0) {
    string msg = "torch_t::torch_t(...) : ";
    
    L = lua_open();
	
    if (L==NULL) {
      cerr << msg << "lua_open() failed" << endl;
      return;
    }

    if (MethodVerbose())
      cout << msg << "lua_open() successfull : " << (void*)L << endl;

    luaL_openlibs(L);

    if (MethodVerbose())
      cout << msg << "lua_openlibs() successfull" << endl;

    string s = "package.path = '/scratch/cs/imagedb/picsom/databases/torch/torch/densecap/?.lua;' .. package.path";
    if (!LuaDoString(*this, s, false)) {
      L = NULL;
      return;
    }

    if (MethodVerbose())
      cout << msg << "prepending package.path successfull" << endl;

    list<string> require { "torch", "paths", "xlua", "optim", "nn"
	// , "image", "qttorch"
	};

    list<string> require_cuda { "cutorch", "cudnn", "cunn" };
    // migh also require "image" but ... 
    // module 'qt' not found:No LuaRocks module found for qt

    for (auto i=require.begin(); i!=require.end(); i++) {
      string s = "require '"+*i+"'";
      if (!LuaDoString(*this, s, false)) {
	L = NULL;
	return;
      }
    }

    bool use_gpu = GpuDeviceId()>-1;
    if (use_gpu)
      for (auto i=require_cuda.begin(); i!=require_cuda.end(); i++) {
	string s = "require '"+*i+"'";
	if (!LuaDoString(*this, s, false)) {
	  L = NULL;
	  return;
	}
      }
    
    for (auto i=dofile.begin(); i!=dofile.end(); i++) {
      string s = "dofile('"+*i+"')";
      if (!LuaDoString(*this, s, false)) {
	L = NULL;
	return;
      }
    }

    s = "model = torch.load('"+model+"')";
    if (!LuaDoString(*this, s, false)) {
      L = NULL;
      return;
    }

    if (!LuaDoString(*this, "n_layers = #model", false)) {
      L = NULL;
      return;
    }

    lua_getglobal(L, "n_layers");
    n_layers = lua_tonumber(L, -1);
    lua_pop(L, 1);
  }

  //===========================================================================

  Torch::torch_t::~torch_t() { 
    string msg = "torch_t::~torch_t() : ";
      
    if (MethodVerbose())
      cout << msg << " *not* closing lua " << (void*)L << endl;

    // if (MethodVerbose())
    //   cout << msg << " closing lua " << (void*)L << endl;

    // if (L)
    //   lua_close(L); 
  }

  //===========================================================================

  bool Torch::ProcessRegion(const Region& ri, const vector<float>& v,
			    Data *dst) {

    vector<float>& d = ((TorchData*)dst)->datavec;

    string msg = "Torch::ProcessRegion() "+ThreadIdentifierUtil()+" : ";

    if (FrameVerbose())
      cout << msg << "d.size()=" << d.size() << endl;

    if (d.size()!=v.size()) {
      char tmp[100];
      sprintf(tmp, "d.size()=%d v.size()=%d", (int)d.size(), (int)v.size());
      throw msg+"data size mismatch "+tmp;
    }

    if (!ri.isA("rectangular")) {
      throw msg+"region is not rectangular";
    }

    if (model_name=="")
      throw msg+"model name not set";

    rectangularRegion rr = *(rectangularRegion*)&ri;
    imagedata img(rr.width(), rr.height(), 3, imagedata::pixeldata_float);
    img.set(v);
    img.force_three_channel();
    // imagefile::display(img);

    if (windowsize=="")
      throw msg+"windowsize should be set";

    vector<string> wsize;
    split(wsize, windowsize, is_any_of("x"));
    size_t wx = atoi(wsize.at(0).c_str());
    size_t wy = atoi(wsize.at(0).c_str());
    if (wx!=wy)
      throw msg+ToStr(wx)+"!="+ToStr(wy)+
	": only square windows are currently supported";

    // using torch::Torch;
    // using torch::Net;
    // using torch::Layer;
    // using torch::Blob;

    //Tic("RunTorch()");

    //bool debug3 = FileVerbose();
    //bool debug4 = FrameVerbose();

    int gpudevice = GpuDeviceId();
    bool use_gpu = gpudevice>=0;
    if (use_gpu) {
      // Torch::SetDevice(gpudevice);
      // Torch::set_mode(Torch::GPU);
    } else
      {}
      // Torch::set_mode(Torch::CPU);

    ProcessBlocksString(img.width(), img.height());
    vector<fv_tree_node*> leaves = BlocksLeaves();
    size_t n_leaves = leaves.size();

    string mapkeyname = model_name+" n_leaves="+ToStr(n_leaves);
    torchmaplock.LockRead();
    auto cmi = torchmap.find(mapkeyname);
    torchmaplock.UnlockRead();
    if (cmi==torchmap.end()) {
      torchmaplock.LockWrite();
      cmi = torchmap.find(mapkeyname);
      if (cmi==torchmap.end()) {
	//Tic("RunTorch() construct");
	if (MethodVerbose())
	  cout << msg << "Creating torch model with mapkeyname=<" << mapkeyname
	       << ">" << endl;

	bool use_gpu = GpuDeviceId()>-1;

	string model   = model_name+"/"+(use_gpu?"gpu":"cpu")+"-model";
	string map     = model_name+"/idxmap.txt";
	string meanstd = model_name+"/meanstd.txt";
      
	// pair<bool,string> tmp = TempFile("torch-prototxt");
	// if (!tmp.first)
	//   throw msg+"TempFile(\"torch-prototxt\") failed";
	// string prototxttmp  = tmp.second;
	// string protocontin = FileToString(prototxtorig), protocontout;
	// vector<string> protolines = SplitInSomething("\n", false, protocontin);
	// bool dim_found = false;
	// for (size_t i=0; i<protolines.size(); i++) {
	//   if (!dim_found && protolines[i].find("input_dim: ")==0) {
	//     protocontout += "input_dim: "+ToStr(n_leaves)+"\n";
	//     dim_found = true;
	//     continue;
	//   }
	//   protocontout += protolines[i]+"\n";
	// }
	// if (!dim_found)
	//   throw msg+"patching <"+prototxtorig+"> failed";
	// if (!StringToFile(protocontout, prototxttmp))
	//   throw msg+"writing in <"+prototxttmp+"> failed";

	string logtxt = "Loading torch ["+model+"] with "+
	  ToStr(n_leaves)+" leaves and [model]";

	vector<size_t> torch2dbmap;
	if (FileSize(map)>0) {
	  logtxt += " and [*.idxmap.txt]";
	  string ss = FileToString(map);
	  vector<string> in = SplitInSomething("\n", false, ss);
	  for (auto i=in.begin(); i!=in.end(); i++)
	    if ((*i)[0]>='0' && (*i)[0]<='9')
	      torch2dbmap.push_back(atoi(i->c_str()));
	}

	if (MethodVerbose())
	  cout << msg << logtxt << endl;

	vector<pair<float,float> > ms;
	for (size_t i=0; i<3; i++)
	  ms.push_back(make_pair(0.5, 0.225));

	if (FileExists(meanstd)) {
	  ms.clear();
	  if (MethodVerbose())
	    cout << msg << "Reading <"+meanstd+">" << endl;
	  vector<string> sv = SplitInSomething("\n\r", false,
					       FileToString(meanstd));
	  for (auto i=sv.begin(); i!=sv.end(); i++) {
	    string s = *i;
	    while (s[0]==' ' || s[0]=='\t')
	      s.erase(0);
	    if (s.size()==0 || s[0]=='#')
	      continue;
	    float mean = 0, std = 1;
	    istringstream(s) >> mean >> std;
	    ms.push_back(make_pair(mean, std));
	  }
	  if (ms.size()!=3) {
	    cerr << msg << "Read " << ms.size() << " instead of 3 from <"
		 << meanstd << ">" << endl;return false;
	  }
	}

	if (MethodVerbose())
	  cout << msg << "meanstd = (" << ms[0].first << ","
	       << ms[1].first << "," << ms[2].first << ") ("
	       << ms[0].second << "," << ms[1].second << ","
	       << ms[2].second << ")" << endl;

	vector<string> dofilesx;
	for (auto i=dofiles.begin(); i!=dofiles.end(); i++)
	  dofilesx.push_back(model_name+"/"+*i);

	torchmap[mapkeyname] = torch_t(mapkeyname, model,
				       meanstd, ms, torch2dbmap,
				       dofilesx);
	cmi = torchmap.find(mapkeyname);
	// Net<float>& cnet = *cmi->second.net;
	// cnet.CopyTrainedLayersFrom(model);

	// nlink(prototxttmp);

	// if (MethodVerbose()) {
	//   cout << msg << "name: " << cnet.name() << endl;
	//   const vector<string>& blob_names  = cnet.blob_names();
	//   const vector<string>& layer_names = cnet.layer_names();
	//   cout << msg << "layers:";
	//   for (size_t i=0; i<layer_names.size(); i++)
	//     cout << " " << layer_names[i];
	//   cout << endl;
	//   cout << msg << "blobs:";
	//   for (size_t i=0; i<blob_names.size(); i++)
	//     cout << " " << blob_names[i];
	//   cout << endl;
	// }
	//Tac("RunTorch() construct");
      }
      torchmaplock.UnlockWrite();
    }

    lua_State *L = cmi->second.L;
    if (!L)
      throw msg+"lua_State L==NULL";

    set<string> all_layers;
    for (size_t li=0; li<leaves.size(); li++) {
      const fv_leaf_recipe& recipe = leaves[li]->leaf_recipe;
      if (MethodVerbose())
	cout << msg << li << " : " << recipe.str() << endl;
      string elayer = recipe.layer;
      if (elayer=="" || elayer=="default")
	throw msg+"elayer not solved";
      all_layers.insert(elayer);
      size_t l = atoi(elayer.c_str());
      if (l<1 || l>15)
	throw msg+"l<1 || l>15";
      size_t was = cmi->second.n_layers;
      while (l<cmi->second.n_layers) {
	LuaDoString(cmi->second, "model:remove()");
	cmi->second.n_layers--;
      }
      size_t is = cmi->second.n_layers;
      if (was!=is && MethodVerbose())
	cout << msg << li << " " << recipe.str() << " \"" << elayer
	     << "\" : n_layers reduced " << was << " -> " << is << endl;
    }

    if (all_layers.size()!=1) {
      cerr << msg << "more/less than one layer:";
      for (auto i=all_layers.begin(); i!=all_layers.end(); i++)
	cerr << " " << *i;
      cerr << endl;
    }

    if (cmi->second.n_layers==0)
      throw msg+"n_layers==0";

    list<vector<float> > emptyres;

    // const vector<Blob<float>*>& input_blobs = cnet.input_blobs();
    // if (debug4)
    //   cout << msg << "input_blobs.size()=" << input_blobs.size() << endl;
    // if (input_blobs.size()!=1) {
    //   throw ShowError(msg+"input_blobs.size()!=1");
    // }

    //size_t ss = 227;  // was
    size_t ss = wx;
    size_t block_size = 3*ss*ss;
    if (!block_size)
      throw msg+"block_size==0";

    // if (input_blobs[0]->count()%block_size)
    //   throw ShowError(msg+"input_blobs[0]->count() mod block_size != 0 : "+
    // 		      ToStr(input_blobs[0]->count())+" vs "+ToStr(block_size));

    size_t n_blocks = 1;
    // size_t n_blocks = input_blobs[0]->count()/block_size;

    // if (debug4)
    //   cout << msg << " input_blobs[0]->count()=" << input_blobs[0]->count()
    // 	   << " input_blobs[0]->num()=" << input_blobs[0]->num()
    // 	   << " input_blobs[0]->channels()=" << input_blobs[0]->channels()
    // 	   << " input_blobs[0]->width()=" << input_blobs[0]->width()
    // 	   << " input_blobs[0]->height()=" << input_blobs[0]->height()
    // 	   << " n_blocks=" << n_blocks
    // 	   << endl;

    // ProcessBlocksString(img.width(), img.height());
    // vector<fv_tree_node*> leaves = BlocksLeaves();
    if (leaves.size()!=n_blocks) 
      throw ShowError(msg+"leaves.size()!=n_blocks : "+
		      ToStr(leaves.size())+" vs "+ToStr(n_blocks));

    vector<float> dataall(n_blocks*block_size);
    map<string,pair<imagedata,imagedata> > imgsrcmap;

    //   size_t layer_no = 0;
    for (size_t li=0; li<leaves.size(); li++) {
      const fv_leaf_recipe& recipe = leaves[li]->leaf_recipe;
      if (MethodVerbose())
	cout << msg << li << " : " << recipe.str() << endl;

      const pair<imagedata,imagedata>& ipair
	= CreateBlocksImage(img, img, recipe.imgsrc, imgsrcmap);

      imagedata ipiece = ipair.first.subimage( recipe.ulx, recipe.uly,
					       recipe.lrx, recipe.lry);
      imagedata mpiece = ipair.second.subimage(recipe.ulx, recipe.uly,
					       recipe.lrx, recipe.lry);
      if (ipiece.width()*ipiece.height()*3!=block_size)
	throw ShowError(msg+"ipiece.size()!=block_size"+
			ToStr(ipiece.width()*ipiece.height()*3)+" vs "+
			ToStr(block_size));

      if (FileVerbose()) {
	cout << ipiece.info() << endl;
	vector<float> ttt = ipiece.get_float(0, 0);
	cout << ttt[0] << " " << ttt[1] << " " << ttt[2] << endl;
      }

      const vector<pair<float,float> >& ms = cmi->second.meanstd;

      for (size_t y=0; y<ipiece.height(); y++)
	for (size_t x=0; x<ipiece.width(); x++) {
	  vector<float> iv = ipiece.get_float(x, y);
	  for (size_t c=0; c<3; c++)
	    iv[c] = (iv[c]-ms[c].first)/ms[c].second;
	  ipiece.set(x, y, iv);
	}
      
      if (FileVerbose()) {
	vector<float> ttt = ipiece.get_float(0, 0);
	cout << ttt[0] << " " << ttt[1] << " " << ttt[2] << endl;
      }

      vector<size_t> vsize, vstride;
      vector<float> vx = ipiece.get_ordered_float("cyx", vsize, vstride);
      
      THFloatStorage *s = THFloatStorage_newWithData(vx.data(), vx.size());
      THFloatTensor *t = THFloatTensor_newWithStorage3d(s, 0, vsize[0], vstride[0],
							vsize[1], vstride[1],
							vsize[2], vstride[2]);

      luaT_pushudata(L, t, "torch.FloatTensor");
      lua_setglobal(L, "t");
      //LuaDoString(cmi->second, "print(t)");

      if (use_gpu) {
	LuaDoString(cmi->second, "t = t:cuda()");
	//LuaDoString(cmi->second, "print(t)");
      }
      //LuaDoString(cmi->second, "print(t)");
      //LuaDoString(cmi->second, "print(model)");
      LuaDoString(cmi->second, "o = model:forward(t)");
      if (use_gpu) {
	LuaDoString(cmi->second, "cutorch.synchronize()");
	LuaDoString(cmi->second, "o = o:float()");
      }
      //LuaDoString(cmi->second, "print(o)");

      lua_getglobal(L, "o");
      void *p = luaT_checkudata(L, -1, "torch.FloatTensor");
      THFloatTensor *o = (THFloatTensor*)p;
      THFloatStorage *v = THFloatTensor_storage(o);
      size_t vd = THFloatTensor_nDimension(o);
      // cout << p << " " << v << " " << vo << " " << vd << endl;
      // for (size_t i=0; i<vd; i++)
      //   cout << " " << i << " " << THFloatTensor_size(o, i)
      //        << " " << THFloatTensor_stride(o, i) << endl;

      size_t max_dim = 0, max_dim_i = 0;
      if (FileVerbose())
	cout << msg << "Torch returned Tensor(";
      for (size_t i=0; i<vd; i++) {
	size_t d = THFloatTensor_size(o, i);
	if (d>max_dim) {
	  max_dim = d;
	  max_dim_i = i;
	}
	if (FileVerbose())
	  cout << (i?",":"") << d;
      }	
      if (FileVerbose()) {
	cout << ") with strides:";
	for (size_t i=0; i<vd; i++)
	  cout << " " << THFloatTensor_stride(o, i);
	cout << endl;
      }

      float *d = THFloatStorage_data(v)+THFloatTensor_storageOffset(o);
      vector<float> vv(max_dim);
      for (size_t i=0; i<vv.size(); i++) {
	size_t j = i*THFloatTensor_stride(o, max_dim_i);
	vv[i] = d[j];
      }
      
      //THFloatTensor_free(t);
      //THFloatStorage_free(s);      
      lua_pop(L, 1);

      // vector<float> esum;
      // if (vd==1) {
      // 	vv = vector<float>((size_t)THFloatTensor_size(o, 0));
      // 	esum = vector<float>(1);
      // 	for (size_t j=0; j<vv.size(); j++) {
      // 	  size_t ij = j*THFloatTensor_stride(o, 0);
      // 	  if (FileVerbose())
      // 	    cout << " " << ij << "/" << d[ij] << endl;
      // 	  vv[j] = d[ij];
      // 	  esum[0] += exp(d[ij]);
      // 	}
      // }
      // if (vd==2) {
      // 	esum = vector<float>(THFloatTensor_size(o, 0));
      // 	vv = vector<float>((size_t)THFloatTensor_size(o, 1));
      // 	for (size_t i=0; i<vv.size(); i++) {
      // 	  if (FileVerbose())
      // 	    cout << i;
      // 	  for (size_t j=0; j<esum.size(); j++) {
      // 	    size_t ij = j*THFloatTensor_stride(o, 0)+i*THFloatTensor_stride(o, 1);
      // 	    if (FileVerbose())
      // 	      cout << " " << ij << "/" << d[ij];
      // 	    if (j==0)
      // 	      vv[i] = d[ij];
      // 	    esum[j] += exp(d[ij]);
      // 	  }
      // 	  if (FileVerbose())
      // 	    cout << endl;
      // 	}
      // }
      //
      // if (FileVerbose()) {
      // 	cout << "***";
      // 	for (size_t j=0; j<esum.size(); j++)
      // 	  cout << " " << esum[j];
      // 	cout << endl;
      // }

      leaves[li]->value(vv); 
    }

    //Tic("RunTorch() outcopy");

    // switch (Torch::mode()) {
    // case Torch::CPU:
    //   memcpy(input_blobs[0]->mutable_cpu_data(), &dataall[0],
    // 	     sizeof(float)*input_blobs[0]->count());
    //   break;
    // case Torch::GPU:
    //   cudaMemcpy(input_blobs[0]->mutable_gpu_data(), &dataall[0],
    // 		 sizeof(float)*input_blobs[0]->count(),
    // 		 cudaMemcpyHostToDevice);
    //   break;
    // default:
    //   LOG(FATAL) << "Unknown Torch mode.";
    // }

    //Tac("RunTorch() outcopy");

    //Tic("RunTorch() forward");
    //LOG(INFO) << "Start";
    // const vector<Blob<float>*>& output_blobs = cnet.ForwardPrefilled();
    //LOG(INFO) << "End";
    //Tac("RunTorch() forward");

    // if (output_blobs.size()!=1)
    //   throw ShowError(msg+"output_blobs.size()!=1");

    // if (debug4)
    //   cout << msg << " output_blobs[0]->count()=" << output_blobs[0]->count()
    // 	   << " output_blobs[0]->num()=" << output_blobs[0]->num()
    // 	   << " output_blobs[0]->channels()=" << output_blobs[0]->channels()
    // 	   << " output_blobs[0]->width()=" << output_blobs[0]->width()
    // 	   << " output_blobs[0]->height()=" << output_blobs[0]->height()
    // 	   << endl;

    // if (output_blobs[0]->num()!=(int)n_leaves)
    //   throw ShowError(msg+"output_blobs[0]->num()!=n_leaves : "+
    // 		      ToStr(output_blobs[0]->num())+"!="+ToStr(n_leaves));

    // float *outarr = new float[output_blobs[0]->count()];

    // //Tic("RunTorch() incopy");

    // switch (Torch::mode()) {
    // case Torch::CPU:
    //   memcpy(outarr, output_blobs[0]->cpu_data(),
    // 	     sizeof(float)*output_blobs[0]->count());
    //   break;
    // case Torch::GPU:
    //   cudaMemcpy(outarr, output_blobs[0]->gpu_data(),
    // 		 sizeof(float)*output_blobs[0]->count(),
    // 		 cudaMemcpyDeviceToHost);
    //   break;
    // default:
    //   LOG(FATAL) << "Unknown Torch mode.";
    // }
      
    //Tic("RunTorch() postprocess");

    // after torch processing...
    // for (size_t li=0; li<leaves.size(); li++) {
    //   if (MethodVerbose())
    // 	cout << msg << li << " : " << leaves[li]->leaf_recipe.str() << endl;
    //   vector<float> v; // read this from torch

    //   string elayer = leaves[li]->leaf_recipe.layer;
    //   if (elayer=="" || elayer=="default")
    // 	elayer = layer;

    //   size_t layer_no = 0;
    //   if (elayer!="") {
    // 	// for (; layer_no<cnet.blob_names().size(); layer_no++)
    // 	//   if (cnet.blob_names()[cnet.blob_names().size()-1-layer_no]==elayer)
    // 	//     break;
    // 	// if (layer_no==cnet.blob_names().size())
    // 	//   throw ShowError(msg+"layer <"+elayer+"> not found in "+model_name);
    //   }

    //   if (layer_no==0) {
    // 	// // obs! not tested yet, remove this message when tested.
    // 	// const float *rv = outarr+li*output_blobs[0]->channels();
    //   	// v = vector<float>(rv, rv+output_blobs[0]->channels());

    //   } else { // layer_no>0
    // 	if (use_gpu)
    // 	  ShowError(msg+"use_gpu with layer_no>0");

    // 	// if (false && debug4) {
    // 	//   const string output_prefix = "proto";
    // 	//   const vector<string>& blob_names = cnet.blob_names();
    // 	//   const vector<boost::shared_ptr<Blob<float> > >&
    // 	//     blobs = cnet.blobs();
    // 	//   for (int blobid = 0; blobid < (int)cnet.blobs().size(); ++blobid) {
    // 	//     // Serialize blob
    // 	//     cout << msg << "Dumping " << blob_names[blobid] << " "
    // 	// 	 << blobs[blobid]->channels() << "," << blobs[blobid]->num() 
    // 	// 	 << endl;
    // 	//     torch::BlobProto output_blob_proto;
    // 	//     blobs[blobid]->ToProto(&output_blob_proto);
    // 	//     WriteProtoToBinaryFile(output_blob_proto, 
    // 	// 			   output_prefix + "-binary-" +
    // 	// 			   blob_names[blobid]);
    // 	//     //WriteProtoToTextFile(output_blob_proto, 
    // 	//     //			 output_prefix + "-text-" +
    // 	//     //                          blob_names[blobid]);
    // 	//   }
    // 	// }

    // 	// const vector<boost::shared_ptr<Blob<float> > >& blobs = cnet.blobs();
    // 	// size_t elayer_no = blobs.size()-1-layer_no;
    // 	// if (MethodVerbose())
    // 	//   cout << msg << "    layer=\"" << layer << "\" eleyer=\"" << elayer
    // 	//        << "\" layer_no=" << layer_no << " elayer_no=" << elayer_no
    // 	//        << endl;

    // 	// torch::BlobProto output_blob_proto;
    // 	// blobs[elayer_no]->ToProto(&output_blob_proto);
    // 	// auto feature_blob = blobs[elayer_no];
    // 	// size_t num_features = feature_blob->num();
    // 	// size_t dim_features = feature_blob->count() / num_features;

    // 	// float *feature_blob_data = feature_blob->mutable_cpu_data() +
    // 	//   feature_blob->offset(li);
    // 	// v = vector<float>(feature_blob_data, feature_blob_data+dim_features);

    // 	// if (debug4) {
    // 	//   cout << msg << "    num_features=" << num_features
    // 	//        << " dim_features=" << dim_features
    // 	//        << " li=" << li
    // 	//        << " feature_blob->offset(li)=" << feature_blob->offset(li)
    // 	//        << " feature_blob->width()=" << feature_blob->width()
    // 	//        << " feature_blob->height()=" << feature_blob->height()
    // 	//        << " feature_blob->channels()=" << feature_blob->channels()
    // 	//        << endl;
    // 	//   cout << msg << "    first component values:";
    // 	//   for (size_t ii=0; ii<10 && ii<v.size(); ii++)
    // 	//     cout << " " << v[ii];
    // 	//   cout << endl;
    // 	// }

    // 	// for (size_t ii=0; ii<v.size(); ii++) {
    // 	//   int cfy = fpclassify(v[ii]);
    // 	//   if (cfy==FP_NAN || cfy==FP_INFINITE) {
    // 	//     cerr << "ERROR: NaN or Inf in index " << ii << "/" << v.size() << endl;
    // 	//     // and then????
    // 	//     break;
    // 	//   }
    // 	// }

    // 	// cout << msg << li << " " << v[42] << endl;
    //   }

    //   leaves[li]->value(v);
    // }

    //Tac("RunTorch() postprocess");

    // delete outarr;

    d = BlocksValue();

    //Tac("RunTorch()");

    return true;
  }

  //===========================================================================

  bool Torch::LuaDoString(const torch_t& t, const string& s, bool thr) {
    string msg = "Torch::LuaDoString() : ";
    if (!t.L) {
      if (thr)
	throw msg+"L==NULL";
      else {
	cerr << msg+"L==NULL" << endl;
	return false;
      }
    }

    if (luaL_dostring(t.L, s.c_str())) {
      if (thr)
	throw msg+"\""+s+"\" failed";
      else {
	cerr << msg+"\""+s+"\" failed" << endl;
	return false;
      }
    }
    
    if (MethodVerbose())
      cout << msg << "\"" << s << "\" successful" << endl;
    
    return true;
  }

  //===========================================================================

  bool Torch::ProcessBlocksString(size_t w, size_t h) {
    string msg = "Torch::ProcessBlocksString() : ";
    
    delete root;
    root = new fv_tree_node;
    root->node_type = FV_NODE_CAT;
    gidx = 0;

    if (MethodVerbose())
      cout << msg << blocks << endl;
    vector<string> funcs;
    split(funcs, blocks, is_any_of("+"));

    for (vector<string>::const_iterator ita = funcs.begin(); 
	 ita != funcs.end(); ++ita) {

      fv_tree_node *n = new fv_tree_node;
      string func = trim_copy(*ita);
      if (MethodVerbose())
	cout << msg << func << endl;

      if (find_first(func, "(")) {
	vector<string> funcparts;
	split(funcparts, func, is_any_of("("));
	
	if      (funcparts.at(0) == "max")
	  n->node_type = FV_NODE_MAX;
	else if (funcparts.at(0) == "cat")
	  n->node_type = FV_NODE_CAT;
	else if (funcparts.at(0) == "avg")
	  n->node_type = FV_NODE_AVG;
	else {
	  throw ShowError(msg+"unrecognized function: "+funcparts.at(0));
	}
	
	string lls = funcparts.at(1);
	trim_right_if(lls, is_any_of(")"));		
	string layr = "default";
	if (find_first(lls, ";")) {
	  vector<string> funcparts2;
	  split(funcparts2, lls, is_any_of(";"));
	  lls = funcparts2.at(0);
	  layr = funcparts2.at(1);
	}
	if (MethodVerbose())
	  cout << msg << "  " << lls << endl;

	vector<string> llparts;
	split(llparts, lls, is_any_of(","));
	for (vector<string>::const_iterator itb = llparts.begin(); 
	     itb != llparts.end(); ++itb) {

	  string l = trim_copy(*itb);
	  ProcessBlocksL(w, h, l, layr, n);
	}

      } else {      
	n->node_type = FV_NODE_CAT;
	string layr = "default";
	if (find_first(func, ";")) {
	  vector<string> funcparts2;
	  split(funcparts2, func, is_any_of(";"));
	  func = funcparts2.at(0);
	  layr = funcparts2.at(1);
	}
	ProcessBlocksL(w, h, func, layr, n);
      }
      root->children.push_back(n);
    }
    
    if (MethodVerbose()) {
      vector<fv_tree_node*> leaves = BlocksLeaves();
      cout << "nleaves=" << leaves.size() << " :";
      for (size_t li=0; li<leaves.size(); li++)
	cout << " " << leaves[li]->idx;
      cout << endl;
    }

    return true; 
  }

  //===========================================================================

  void Torch::ProcessBlocksL(size_t w, size_t h, const string& str, 
			     const string& layr, fv_tree_node *parent) {
    string msg = "Torch::ProcessBlocksL() : ";    

    if (MethodVerbose())
      cout << "    " << str << endl;
    string imgsrc = str;
    // image size, grid size, boundary size:
    size_t ix = 0, iy = 0, gx = 1, gy = 1, bx = 0, by = 0;
    bool flip = false, spl = false;

    if (find_first(str, ":")) {
      vector<string> lparts;
      split(lparts, str, is_any_of(":"));
      imgsrc = lparts.at(0);
      ProcessBlocksImgsrc(imgsrc, ix, iy, w, h);

      string grid = lparts.at(1);
      if (find_first(grid, "%")) {
	vector<string> gparts;    
	split(gparts, grid, is_any_of("%"));
	grid = gparts.at(0);
	vector<string> bparts;    
	split(bparts, gparts.at(1), is_any_of("x"));
	bx = atoi(bparts.at(0).c_str());
	by = atoi(bparts.at(1).c_str());
      }
      if (find_first(grid, "f"))
	flip = true;
      if (find_first(grid, "s"))
	spl = true;
      trim_right_if(grid, is_any_of("fs"));
      trim_right_if(grid, is_any_of("fs"));        

      vector<string> gparts;    
      split(gparts, grid, is_any_of("x"));
      gx = atoi(gparts.at(0).c_str());
      gy = atoi(gparts.at(1).c_str());

    } else {
      ProcessBlocksImgsrc(imgsrc, ix, iy, w, h);
    }

    if (MethodVerbose())
      cout << "    ix=" << ix << ", iy=" << iy << ", gx=" << gx << ", gy=" << gy 
	   << ", bx=" << bx << ", by=" << by 
	   << ", flip=" << flip << ", spl=" << spl << ", layr=" << layr << endl;
    
    // window size:
    vector<string> wsize;
    split(wsize, windowsize, is_any_of("x"));
    size_t wx = atoi(wsize.at(0).c_str());
    size_t wy = atoi(wsize.at(0).c_str());

    // stride:
    float sx = gx>1 ? float(ix - 2*bx - wx)/(gx-1) : 0;
    float sy = gy>1 ? float(iy - 2*by - wy)/(gy-1) : 0;

    // only center block:
    if (gx==1)
      bx = (ix-wx)/2;
    if (gy==1)
      by = (iy-wy)/2;

    GenerateLeafs(sx, sy, gx, gy, bx, by, wx, wy, imgsrc, layr, parent);

    if (spl)
      GenerateLeafs(sx, sy, gx-1, gy-1, bx+sx/2, by+sy/2, wx, wy, imgsrc, 
		    layr, parent);

    if (flip)
      GenerateLeafs(sx, sy, gx, gy, bx, by, wx, wy, imgsrc+"f", 
		    layr, parent);
    
    if (spl && flip) {
      size_t z = 0;
      if (gx==2 && 2*bx!=ix-wx) // for compatibility with the *Old() version...
	z = 1;
      GenerateLeafs(sx, sy, gx-1, gy-1, z+bx+sx/2, by+sy/2, wx, wy, imgsrc+"f", 
		    layr, parent);
    }
  }

  //===========================================================================
  
  void Torch::ProcessBlocksImgsrc(string& imgsrc, size_t& ix, size_t& iy, 
				  size_t w, size_t h) {
    string msg = "Torch::ProcessBlocksImgSrc() : ";    

    if (find_first(imgsrc, "x")) {
      vector<string> imgparts;
      split(imgparts, imgsrc, is_any_of("x"));
      ix = atoi(imgparts.at(0).c_str());
      trim_right_if(imgparts.at(1), is_any_of("a"));
      iy = atoi(imgparts.at(1).c_str());
    } else {
      if (ends_with(imgsrc, "a"))
	  throw ShowError(msg+"failed to parse: "+imgsrc);
      if (w>h) {
	iy = atoi(imgsrc.c_str());
	ix = w*iy/h;
      } else {
	ix = atoi(imgsrc.c_str());
	iy = w*ix/h;
      }
      stringstream ss;
      ss << ix << "x" << iy;
      imgsrc = ss.str();
    }
  }

  //===========================================================================

  void Torch::GenerateLeafs(float sx, float sy, size_t gx, size_t gy, 
			    size_t bx, size_t by, size_t wx, size_t wy,
			    const string& imagesrc, const string& layer,
			    fv_tree_node *parent) { 

    for (size_t i=0; i<gx; i++) {
      for (size_t j=0; j<gy; j++) {
	fv_tree_node *n = new fv_tree_node;
	n->node_type = FV_NODE_LEAF;
	n->idx = gidx++;
	// OBS! THIS THE PLACE WHERE RECIPES ARE CREATED
	n->leaf_recipe.imgsrc = imagesrc; // "256x256a"; // or without "a" or with "f"
	n->leaf_recipe.layer  = layer;                   //"fc6"; 
	n->leaf_recipe.ulx    = bx + round(i*sx);          //14; // or  15?
	n->leaf_recipe.uly    = by + round(j*sy);          //14; // or  15?
	n->leaf_recipe.lrx    = n->leaf_recipe.ulx + wx - 1; //240; // or 241?
	n->leaf_recipe.lry    = n->leaf_recipe.uly + wy - 1; //240; // or 241?
	
	if (MethodVerbose())
	  cout << n->leaf_recipe.str() << endl;

	parent->children.push_back(n);
      }
    }
  }

  //===========================================================================

  const pair<imagedata,imagedata>&
  Torch::CreateBlocksImage(const imagedata& imgin, const imagedata& mimg,
			   const string& spec,
			   map<string,pair<imagedata,imagedata> >& map) {
    string msg = "Torch::CreateBlocksImage() : ";

    pair<imagedata,imagedata> imgpair;
    //bool subtract_mean = true;
    auto imp = map.find(spec);
    if (imp==map.end()) {
      size_t p = spec.find("f");
      if (p!=string::npos) { // flip
	// subtract_mean = false;
	string specr = spec;
	specr.erase(p, 1);
	imgpair = CreateBlocksImage(imgin, mimg, specr, map);
	imagedata& imgi = imgpair.first;
	imagedata& imgm = imgpair.second;
	for (size_t y=0; y<imgi.height(); y++)
	  for (size_t x=0; x<imgi.width()/2; x++) {
	    vector<float> t = imgi.get_float(x, y);
	    imgi.set(x, y, imgi.get_float(imgi.width()-1-x, y));
	    imgi.set(imgi.width()-1-x, y, t);
	    t = imgm.get_float(x, y);
	    imgm.set(x, y, imgm.get_float(imgm.width()-1-x, y));
	    imgm.set(imgm.width()-1-x, y, t);
	  }

      } else {
	imgpair.first  = imgin;
	imgpair.second = mimg;
	bool keep_aspect = false;
	size_t width = atoi(spec.c_str()), height = 0;
	size_t p = spec.find("x");
	if (p!=string::npos)
	  height = atoi(spec.substr(p+1).c_str());
	if (spec.find("a")!=string::npos)
	  keep_aspect = true;

	if (!width || !height)
	  throw ShowError(msg+"spec \""+spec+"\" is zero sized");

	imagedata& imgi = imgpair.first;
	imagedata& imgm = imgpair.second;

	if (!keep_aspect) {
	  scalinginfo sii(imgi.width(), imgi.height(), width, height);
	  imgi.rescale(sii, 1);
	  scalinginfo sim(imgm.width(), imgm.height(), width, height);
	  imgm.rescale(sim, 1);

	} else {
	  if (width!=height)
	    throw ShowError(msg+"width!=height does not? work");
	    
	  // center square...
	  size_t iw = imgi.width(), ih = imgi.height();
	  size_t wh = iw<ih ? iw : ih, x = 0, y = 0;
	  if (iw>ih)
	    x = (iw-ih)/2;
	  else
	    y = (ih-iw)/2;
	  scalinginfo sii(wh, wh, width, height);
	  imgi = imgi.subimage(x, y, x+wh-1, y+wh-1);
	  imgi.rescale(sii, 1);
	  // obs! this was active until 2014-10-21...
	  // imgm = imgm.subimage(x, y, x+wh-1, y+wh-1);
	  scalinginfo sim(imgm.width(), imgm.height(), width, height);
	  imgm.rescale(sim, 1);
	}
      }

   // if (subtract_mean) {
   // 	size_t width = img.width(), height = img.height();  
   // 	float *ip = (float*)img.raw_float(width*height*3);
   // 	imagedata mtmp;
   // 	if (mimg.width()!=width || mimg.height()!=height) {
   // 	  mtmp = mimg;
   // 	  scalinginfo si(mtmp.width(), mtmp.height(), width, height);
   // 	  mtmp.rescale(si, 1);
   // 	}
   // 	const float *mp = mtmp.isempty() ? mimg.raw_float(width*height*3)
   // 	  : mtmp.raw_float(width*height*3);
   // 	for (size_t z=0; z<width*height*3; z++)
   // 	  ip[z] = 255*(ip[z]-mp[z]);
   // }

      if (MethodVerbose())
	cout << "Created blocks image \"" << spec << "\" "
	     << imgpair.first.info() << " from " << imgin.info() << " and "
	     << imgpair.second.info() << " from " << mimg.info() << endl;
      
      map[spec] = imgpair;
      imp = map.find(spec);
    }

    return imp->second;
  }

  //===========================================================================

  vector<Torch::fv_tree_node*> Torch::fv_tree_node::leaves() {
    if (node_type == FV_NODE_LEAF)
      return vector<fv_tree_node*> { this };

    vector<fv_tree_node*> res;
    for (size_t i=0; i<children.size(); i++) {
      vector<fv_tree_node*> v = children[i]->leaves();
      res.insert(res.end(), v.begin(), v.end());
    }

    return res;
  }

  //===========================================================================

  vector<float> Torch::fv_tree_node::value() {
    if (node_type == FV_NODE_LEAF)
      return leaf_value;
    
    vector<float> res;
    
    for (vector<fv_tree_node *>::iterator it = children.begin(); 
	 it != children.end(); ++it) {
      switch (node_type) {
      case (FV_NODE_CAT):
	res = concatenate(res,(*it)->value());
	break;
      case (FV_NODE_MAX):
	res = elementwise_max(res,(*it)->value());
	break;
      case (FV_NODE_AVG):
	res = elementwise_sum(res,(*it)->value());
	break;
      case (FV_NODE_VLAD):
      default:
	throw string("FV_NODE_VLAD or something else not implemented");
	break;
      }
    }
    if (node_type == FV_NODE_AVG)
      res = elementwise_div(res, children.size());
    
    return res;
  }  

  vector<float> Torch::fv_tree_node::concatenate(const vector<float> &a, 
						 const vector<float> &b) {
    if (MethodVerbose())
      cout << "CAT(" << a.size() << "," << b.size() << ") ";
    vector<float> res = a;
    res.insert(res.end(), b.begin(), b.end());
    return res;
  }
  
  vector<float> Torch::fv_tree_node::elementwise_max(const vector<float> &a, 
						     const vector<float> &b) {
    if (MethodVerbose())
      cout << "MAX(" << a.size() << "," << b.size() << ") ";
    if (!a.size())
      return b;
    
    vector<float> res(a.size());
    for (size_t i=0; i<a.size(); i++)
      res.at(i) = (a.at(i) > b.at(i) ? a.at(i) : b.at(i));
    return res;
  }
  
  vector<float> Torch::fv_tree_node::elementwise_sum(const vector<float> &a, 
						     const vector<float> &b) {
    if (MethodVerbose())
      cout << "SUM(" << a.size() << "," << b.size() << ") ";
    if (!a.size())
      return b;
    
    vector<float> res(a.size());
    for (size_t i=0; i<a.size(); i++)
      res.at(i) = a.at(i) + b.at(i);
    return res;
  }
  
  vector<float> Torch::fv_tree_node::elementwise_div(const vector<float> &a, 
						     size_t b) {
    vector<float> res(a.size());
    for (size_t i=0; i<a.size(); i++)
      res.at(i) = a.at(i) / b;
    return res;
  }
  
  //===========================================================================

} // namespace picsom

#endif // HAVE_THC_H

