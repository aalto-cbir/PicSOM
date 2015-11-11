// $Id: Caffe.C,v 1.24 2015/11/10 13:46:16 jorma Exp $	

#include <Caffe.h>

#ifdef HAVE_CAFFE_DATA_LAYERS_HPP

#include <caffe/data_layers.hpp>

namespace picsom {
  static const char *vcid =
    "$Id: Caffe.C,v 1.24 2015/11/10 13:46:16 jorma Exp $";

  static Caffe list_entry(true);

  RwLock Caffe::caffemaplock;

  map<string,Caffe::caffe_t> Caffe::caffemap;

  //===========================================================================

  string Caffe::Version() const {
    return vcid;
  }

  //===========================================================================

  bool Caffe::InitializeCaffe() {
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

  bool Caffe::ProcessOptionsAndRemove(list<string>& l) {
    static const string msg = "Caffe::ProcessOptionsAndRemove() ";

    if (FileVerbose())
      cout << msg << "name=" << Name() << endl;

    for (list<string>::iterator i = l.begin(); i!=l.end();) {
      if (FileVerbose())
	cout << "  <" << *i << ">" << endl;

      string key, value;
      SplitOptionString(*i, key, value);
    
      if (key=="colorspace") {
	//colorspace = value;
	cout << "colorspace option ignored" << endl;
      	i = l.erase(i);
      	continue;
      }

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

      if (key=="resize") { // deprecated
	resize = value;
      
      	i = l.erase(i);
      	continue;
      }

      if (key=="fusion") {  // deprecated
	fusion = value;
      
      	i = l.erase(i);
      	continue;
      }

      i++;
    }

    return RegionBased::ProcessOptionsAndRemove(l);
  }

  //===========================================================================

  bool Caffe::ProcessRegion(const Region& ri, const vector<float>& v,
			    Data *dst) {
    // return ProcessRegionOld(ri, v, dst);
    return ProcessRegionNew(ri, v, dst);
  }

  //===========================================================================

  bool Caffe::ProcessRegionNew(const Region& ri, const vector<float>& v,
			       Data *dst) {
    vector<float>& d = ((CaffeData*)dst)->datavec;

    string msg = "Caffe::ProcessRegion() "+ThreadIdentifierUtil()+" : ";

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

    string resizex = resize;
    if (resizex=="")
      resizex = "256x256";

    //if (windowsize!="227x227")
    //  throw msg+"only windowsize=227x227 is currently supported";

    vector<string> wsize;
    split(wsize, windowsize, is_any_of("x"));
    size_t wx = atoi(wsize.at(0).c_str());
    size_t wy = atoi(wsize.at(0).c_str());
    if (wx!=wy)
      throw msg+ToStr(wx)+"!="+ToStr(wy)+": only square windows are currently supported";

    using caffe::Caffe;
    using caffe::Net;
    using caffe::Layer;
    using caffe::Blob;

    //Tic("RunCaffe()");

    bool debug3 = FileVerbose();
    bool debug4 = FrameVerbose();
    bool debug6 = PixelVerbose();
    bool mean_is_rgb = false;

    int gpudevice = GpuDeviceId();
    bool use_gpu = gpudevice>=0;
    if (use_gpu) {
      Caffe::SetDevice(gpudevice);
      Caffe::set_mode(Caffe::GPU);
    } else
      Caffe::set_mode(Caffe::CPU);

    Caffe::set_phase(Caffe::TEST);

    ProcessBlocksString(img.width(), img.height());
    vector<fv_tree_node*> leaves = BlocksLeaves();
    size_t n_leaves = leaves.size();

    string mapkeyname = model_name+" n_leaves="+ToStr(n_leaves);
    caffemaplock.LockRead();
    auto cmi = caffemap.find(mapkeyname);
    caffemaplock.UnlockRead();
    if (cmi==caffemap.end()) {
      caffemaplock.LockWrite();
      cmi = caffemap.find(mapkeyname);
      if (cmi==caffemap.end()) {
	//Tic("RunCaffe() construct");
	if (MethodVerbose())
	  cout << msg << "Creating caffe model with mapkeyname=<" << mapkeyname
	       << ">" << endl;

	string prototxtorig = model_name+"/prototxt";
	string model        = model_name+"/model";
	string mean         = model_name+"/mean.png";
	string map          = model_name+"/idxmap.txt";
      
	pair<bool,string> tmp = TempFile("caffe-prototxt");
	if (!tmp.first)
	  throw msg+"TempFile(\"caffe-prototxt\") failed";
	string prototxttmp  = tmp.second;
	string protocontin = FileToString(prototxtorig), protocontout;
	vector<string> protolines = SplitInSomething("\n", false, protocontin);
	bool dim_found = false;
	for (size_t i=0; i<protolines.size(); i++) {
	  if (!dim_found && protolines[i].find("input_dim: ")==0) {
	    protocontout += "input_dim: "+ToStr(n_leaves)+"\n";
	    dim_found = true;
	    continue;
	  }
	  protocontout += protolines[i]+"\n";
	}
	if (!dim_found)
	  throw msg+"patching <"+prototxtorig+"> failed";
	if (!StringToFile(protocontout, prototxttmp))
	  throw msg+"writing in <"+prototxttmp+"> failed";

	string logtxt = "Loading caffe ["+prototxtorig+"] with "+ToStr(n_leaves)+
	  " leaves and [model] and [mean.png]";

	vector<size_t> caffe2dbmap;
	if (FileSize(map)>0) {
	  logtxt += " and [idxmap.txt]";
	  string ss = FileToString(map);
	  vector<string> in = SplitInSomething("\n", false, ss);
	  for (auto i=in.begin(); i!=in.end(); i++)
	    if ((*i)[0]>='0' && (*i)[0]<='9')
	      caffe2dbmap.push_back(atoi(i->c_str()));
	}

	if (MethodVerbose())
	  cout << msg << logtxt << endl;

	if (debug3)
	  FLAGS_logtostderr = 1;

	imagedata mimg = imagefile(mean).frame(0);

	if (!mean_is_rgb) { // == is_bgr
	  size_t width = mimg.width(), height = mimg.height();
	  float *mp =  (float*)mimg.raw_float(width*height*3);
	  for (size_t z=0; z<width*height*3; z+=3) {
	    float t = mp[z];
	    mp[z  ] = mp[z+2];
	    mp[z+2] = t;
	  }
	}

	caffe_t caffe(mapkeyname, prototxtorig, model, mean,
		      new Net<float>(prototxttmp), mimg, caffe2dbmap);
	caffemap[mapkeyname] = caffe;
	cmi = caffemap.find(mapkeyname);
	Net<float>& cnet = *cmi->second.net;
	cnet.CopyTrainedLayersFrom(model);

	Unlink(prototxttmp);

	if (MethodVerbose()) {
	  cout << msg << "name: " << cnet.name() << endl;
	  const vector<string>& blob_names  = cnet.blob_names();
	  const vector<string>& layer_names = cnet.layer_names();
	  cout << msg << "layers:";
	  for (size_t i=0; i<layer_names.size(); i++)
	    cout << " " << layer_names[i];
	  cout << endl;
	  cout << msg << "blobs:";
	  for (size_t i=0; i<blob_names.size(); i++)
	    cout << " " << blob_names[i];
	  cout << endl;
	}
	//Tac("RunCaffe() construct");
      }
      caffemaplock.UnlockWrite();
    }

    Net<float>& cnet = *cmi->second.net;
    caffe::TransformationParameter_CS cs = cnet.input_color_space();
    string colorspace = caffe::DataTransformer<float>::ColorSpace(cs);

    if (MethodVerbose())
      cout << msg << "Using caffe model ["+model_name+"] with colorspace=\""+
	colorspace+"\" windowsize=\""+
	windowsize+"\" blocks=\""+blocks+"\" (n_leaves="+ToStr(n_leaves)+
	") layer=\""+layer+"\" (resize=\""+resize+"\" fusion=\""+fusion+"\")"
	   << endl;

    list<vector<float> > emptyres;

    vector<Blob<float>*>& input_blobs = cnet.input_blobs();
    if (debug4)
      cout << msg << "input_blobs.size()=" << input_blobs.size() << endl;
    if (input_blobs.size()!=1) {
      throw ShowError(msg+"input_blobs.size()!=1");
    }

    //size_t ss = 227;  // was
    size_t ss = wx;
    size_t block_size = 3*ss*ss;
    if (input_blobs[0]->count()%block_size)
      throw ShowError(msg+"input_blobs[0]->count() mod block_size != 0 : "+
		      ToStr(input_blobs[0]->count())+" vs "+ToStr(block_size));
      
    size_t n_blocks = input_blobs[0]->count()/block_size;

    if (debug4)
      cout << msg << " input_blobs[0]->count()=" << input_blobs[0]->count()
	   << " input_blobs[0]->num()=" << input_blobs[0]->num()
	   << " input_blobs[0]->channels()=" << input_blobs[0]->channels()
	   << " input_blobs[0]->width()=" << input_blobs[0]->width()
	   << " input_blobs[0]->height()=" << input_blobs[0]->height()
	   << " n_blocks=" << n_blocks
	   << endl;

    // ProcessBlocksString(img.width(), img.height());
    // vector<fv_tree_node*> leaves = BlocksLeaves();
    if (leaves.size()!=n_blocks) 
      throw ShowError(msg+"leaves.size()!=n_blocks : "+
		      ToStr(leaves.size())+" vs "+ToStr(n_blocks));

    vector<float> dataall(n_blocks*block_size);
    map<string,pair<imagedata,imagedata> > imgsrcmap;

    for (size_t li=0; li<leaves.size(); li++) {
      const fv_leaf_recipe& recipe = leaves[li]->leaf_recipe;
      if (MethodVerbose())
	cout << msg << li << " : " << recipe.str() << endl;

      const pair<imagedata,imagedata>& ipair
	= CreateBlocksImage(img, cmi->second.mimg, recipe.imgsrc, imgsrcmap);

      imagedata ipiece = ipair.first.subimage( recipe.ulx, recipe.uly,
					       recipe.lrx, recipe.lry);
      imagedata mpiece = ipair.second.subimage(recipe.ulx, recipe.uly,
					       recipe.lrx, recipe.lry);
      if (ipiece.width()*ipiece.height()*3!=block_size)
	throw ShowError(msg+"ipiece.size()!=block_size"+
			ToStr(ipiece.width()*ipiece.height()*3)+" vs "+
			ToStr(block_size));
      vector<float> dat(block_size);
      float *b = &dat[0]; // *g = b+ss*ss, *r = g+ss*ss;
      for (size_t y=0; y<ipiece.height(); y++)
	for (size_t x=0; x<ipiece.width(); x++) {
	  vector<float> iv = ipiece.get_float(x, y);
	  vector<float> mv = mpiece.get_float(x, y);
	  float R = 255*iv[0], G = 255*iv[1], B = 255*iv[2];
	  float Rmean = 255*mv[0], Gmean = 255*mv[1], Bmean = 255*mv[2];
	  vector<double> in { B, G, R }, mean { Bmean, Gmean, Rmean }; 
	  
	  vector<double> out
	    = caffe::DataTransformer<float>::ColorConvert(cs, in, mean);

	  if (debug6)
	    cout << msg << "li=" << li << "/" << leaves.size()
		 << " x=" << x << " y=" << y << " R=" << R << " G=" << G
		 << " B=" << B << " Rmean=" << Rmean << " Gmean=" << Gmean
		 << " Bmean=" << Bmean << " out[0]=" << out[0]
		 << " out[1]=" << out[1] << " out[2]=" << out[2] << endl;

	  for (size_t ci=0; ci<out.size(); ci++)
	    *(b+ci*ss*ss) = out[ci];

	  b++;
	}
      memcpy(&dataall[0]+li*block_size, &dat[0], block_size*sizeof(float));
    }

    //Tic("RunCaffe() outcopy");

    switch (Caffe::mode()) {
    case Caffe::CPU:
      memcpy(input_blobs[0]->mutable_cpu_data(), &dataall[0],
	     sizeof(float)*input_blobs[0]->count());
      break;
    case Caffe::GPU:
      cudaMemcpy(input_blobs[0]->mutable_gpu_data(), &dataall[0],
		 sizeof(float)*input_blobs[0]->count(),
		 cudaMemcpyHostToDevice);
      break;
    default:
      LOG(FATAL) << "Unknown Caffe mode.";
    }

    //Tac("RunCaffe() outcopy");

    //Tic("RunCaffe() forward");
    //LOG(INFO) << "Start";
    const vector<Blob<float>*>& output_blobs = cnet.ForwardPrefilled();
    //LOG(INFO) << "End";
    //Tac("RunCaffe() forward");

    if (output_blobs.size()!=1)
      throw ShowError(msg+"output_blobs.size()!=1");

    if (debug4)
      cout << msg << " output_blobs[0]->count()=" << output_blobs[0]->count()
	   << " output_blobs[0]->num()=" << output_blobs[0]->num()
	   << " output_blobs[0]->channels()=" << output_blobs[0]->channels()
	   << " output_blobs[0]->width()=" << output_blobs[0]->width()
	   << " output_blobs[0]->height()=" << output_blobs[0]->height()
	   << endl;

    if (output_blobs[0]->num()!=(int)n_leaves)
      throw ShowError(msg+"output_blobs[0]->num()!=n_leaves : "+
		      ToStr(output_blobs[0]->num())+"!="+ToStr(n_leaves));

    float *outarr = new float[output_blobs[0]->count()];

    //Tic("RunCaffe() incopy");

    switch (Caffe::mode()) {
    case Caffe::CPU:
      memcpy(outarr, output_blobs[0]->cpu_data(),
	     sizeof(float)*output_blobs[0]->count());
      break;
    case Caffe::GPU:
      cudaMemcpy(outarr, output_blobs[0]->gpu_data(),
		 sizeof(float)*output_blobs[0]->count(),
		 cudaMemcpyDeviceToHost);
      break;
    default:
      LOG(FATAL) << "Unknown Caffe mode.";
    }
      
    //Tic("RunCaffe() postprocess");

    // after caffe processing...
    for (size_t li=0; li<leaves.size(); li++) {
      if (MethodVerbose())
	cout << msg << li << " : " << leaves[li]->leaf_recipe.str() << endl;
      vector<float> v; // read this from caffe

      string elayer = leaves[li]->leaf_recipe.layer;
      if (elayer=="" || elayer=="default")
	elayer = layer;

      size_t layer_no = 0;
      if (elayer!="") {
	for (; layer_no<cnet.blob_names().size(); layer_no++)
	  if (cnet.blob_names()[cnet.blob_names().size()-1-layer_no]==elayer)
	    break;
  	if (layer_no==cnet.blob_names().size())
	  throw ShowError(msg+"layer <"+elayer+"> not found in "+model_name);
      }

      if (layer_no==0) {
	// obs! not tested yet, remove this message when tested.
	const float *rv = outarr+li*output_blobs[0]->channels();
      	v = vector<float>(rv, rv+output_blobs[0]->channels());

      } else { // layer_no>0
	if (use_gpu)
	  ShowError(msg+"use_gpu with layer_no>0");

	if (false && debug4) {
	  const string output_prefix = "proto";
	  const vector<string>& blob_names = cnet.blob_names();
	  const vector<boost::shared_ptr<Blob<float> > >&
	    blobs = cnet.blobs();
	  for (int blobid = 0; blobid < (int)cnet.blobs().size(); ++blobid) {
	    // Serialize blob
	    cout << msg << "Dumping " << blob_names[blobid] << " "
		 << blobs[blobid]->channels() << "," << blobs[blobid]->num() 
		 << endl;
	    caffe::BlobProto output_blob_proto;
	    blobs[blobid]->ToProto(&output_blob_proto);
	    WriteProtoToBinaryFile(output_blob_proto, 
				   output_prefix + "-binary-" +
				   blob_names[blobid]);
	    //WriteProtoToTextFile(output_blob_proto, 
	    //			 output_prefix + "-text-" +
	    //                          blob_names[blobid]);
	  }
	}

	const vector<boost::shared_ptr<Blob<float> > >& blobs = cnet.blobs();
	size_t elayer_no = blobs.size()-1-layer_no;
	if (MethodVerbose())
	  cout << msg << "    layer=\"" << layer << "\" eleyer=\"" << elayer
	       << "\" layer_no=" << layer_no << " elayer_no=" << elayer_no
	       << endl;

	caffe::BlobProto output_blob_proto;
	blobs[elayer_no]->ToProto(&output_blob_proto);
	auto feature_blob = blobs[elayer_no];
	size_t num_features = feature_blob->num();
	size_t dim_features = feature_blob->count() / num_features;

	float *feature_blob_data = feature_blob->mutable_cpu_data() +
	  feature_blob->offset(li);
	v = vector<float>(feature_blob_data, feature_blob_data+dim_features);

	if (debug4) {
	  cout << msg << "    num_features=" << num_features
	       << " dim_features=" << dim_features
	       << " li=" << li
	       << " feature_blob->offset(li)=" << feature_blob->offset(li)
	       << endl;
	  cout << msg << "    first component values:";
	  for (size_t ii=0; ii<10 && ii<v.size(); ii++)
	    cout << " " << v[ii];
	  cout << endl;
	}

	for (size_t ii=0; ii<v.size(); ii++) {
	  int cfy = fpclassify(v[ii]);
	  if (cfy==FP_NAN || cfy==FP_INFINITE) {
	    cerr << "ERROR: NaN or Inf in index " << ii << "/" << v.size() << endl;
	    // and then????
	    break;
	  }
	}

	// cout << msg << li << " " << v[42] << endl;
      }

      leaves[li]->value(v);
    }

    //Tac("RunCaffe() postprocess");

    delete outarr;

    d = BlocksValue();

    //Tac("RunCaffe()");

    return true;
  }

  //===========================================================================

  bool Caffe::ProcessBlocksString(size_t w, size_t h) {
    string msg = "Caffe::ProcessBlocksString() : ";
    
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

  void Caffe::ProcessBlocksL(size_t w, size_t h, const string& str, 
			     const string& layr, fv_tree_node *parent) {
    string msg = "Caffe::ProcessBlocksL() : ";    

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
  
  void Caffe::ProcessBlocksImgsrc(string& imgsrc, size_t& ix, size_t& iy, 
				  size_t w, size_t h) {
    string msg = "Caffe::ProcessBlocksImgSrc() : ";    

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

  void Caffe::GenerateLeafs(float sx, float sy, size_t gx, size_t gy, 
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
  Caffe::CreateBlocksImage(const imagedata& imgin, const imagedata& mimg,
			   const string& spec,
			   map<string,pair<imagedata,imagedata> >& map) {
    string msg = "Caffe::CreateBlocksImage() : ";

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

  vector<Caffe::fv_tree_node*> Caffe::fv_tree_node::leaves() {
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

  vector<float> Caffe::fv_tree_node::value() {
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

  vector<float> Caffe::fv_tree_node::concatenate(const vector<float> &a, 
						 const vector<float> &b) {
    if (MethodVerbose())
      cout << "CAT(" << a.size() << "," << b.size() << ") ";
    vector<float> res = a;
    res.insert(res.end(), b.begin(), b.end());
    return res;
  }
  
  vector<float> Caffe::fv_tree_node::elementwise_max(const vector<float> &a, 
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
  
  vector<float> Caffe::fv_tree_node::elementwise_sum(const vector<float> &a, 
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
  
  vector<float> Caffe::fv_tree_node::elementwise_div(const vector<float> &a, 
						     size_t b) {
    vector<float> res(a.size());
    for (size_t i=0; i<a.size(); i++)
      res.at(i) = a.at(i) / b;
    return res;
  }
  
  //===========================================================================

  bool Caffe::ProcessRegionOld(const Region& ri, const vector<float>& v,
			    Data *dst) {
    vector<float>& d = ((CaffeData*)dst)->datavec;

    string msg = "Caffe::ProcessRegion() : ";

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

    rectangularRegion rr = *(rectangularRegion*)&ri;
    imagedata img(rr.width(), rr.height(), 3, imagedata::pixeldata_float);
    img.set(v);

    list<imagedata> ilist { img };

    string name = model_name;
    if (name=="")
      throw msg+"model name not set";

    string fusionx = fusion;
    if (fusionx=="")
      fusionx = "amean";

    string resizex = resize;
    if (resizex=="")
      resizex = "256x256";

    bool keep_aspect = false;
    size_t width = atoi(resizex.c_str()), height = 0;
    size_t p = resizex.find("x");
    if (p!=string::npos)
      height = atoi(resizex.substr(p+1).c_str());
    if (resizex.find("a")!=string::npos)
      keep_aspect = true;

    if (layer!="fc6")
      throw msg+"only layer==fc6 is currently supported";
      
    size_t level = 3; // obs!  this should come from layer="fc6"

    using caffe::Net;
    using caffe::Blob;
    using caffe::Caffe;

    //Tic("RunCaffe()");

    bool debug3 = FileVerbose();
    bool debug4 = FrameVerbose();
    bool mean_is_rgb = false;

    int gpudevice = GpuDeviceId();
    bool use_gpu = gpudevice>=0;
    if (use_gpu) {
      Caffe::SetDevice(gpudevice);
      Caffe::set_mode(Caffe::GPU);
    } else
      Caffe::set_mode(Caffe::CPU);

    Caffe::set_phase(Caffe::TEST);

    auto cmi = caffemap.find(name);
    if (cmi==caffemap.end()) {
      //Tic("RunCaffe() construct");

      string prototxt = name+"/prototxt";
      string model    = name+"/model";
      string mean     = name+"/mean.png";
      string map      = name+"/idxmap.txt";
      
      string logtxt = "Loading caffe ["+prototxt+"] and [model] "
	"and [mean.png]";

      vector<size_t> caffe2dbmap;
      if (FileExists(map)) {
	logtxt += " and [idxmap.txt]";
	string ss = FileToString(map);
	vector<string> in = SplitInSomething("\n", false, ss);
	for (auto i=in.begin(); i!=in.end(); i++)
	  if ((*i)[0]>='0' && (*i)[0]<='9')
	    caffe2dbmap.push_back(atoi(i->c_str()));
      }


      logtxt += " with fusion=\""+fusionx+"\" resize=\""+resizex
	+"\" layer=\""+layer+"\"";

      if (MethodVerbose())
	cout << logtxt << endl;

      if (debug3)
	FLAGS_logtostderr = 1;

      imagedata mimg = imagefile(mean).frame(0);
      caffe_t caffe(name, prototxt, model, mean,
		    new Net<float>(prototxt), mimg, caffe2dbmap);
      caffemap[name] = caffe;
      cmi = caffemap.find(name);
      Net<float>& cnet = *cmi->second.net;
      cnet.CopyTrainedLayersFrom(model);

      //Tac("RunCaffe() construct");
    }
    Net<float>& cnet = *cmi->second.net;

    list<vector<float> > emptyres, res;

    for (auto ilp = ilist.begin(); ilp!=ilist.end(); ) {
      vector<Blob<float>*>& input_blobs = cnet.input_blobs();
      if (debug4)
	cout << "input_blobs.size()=" << input_blobs.size() << endl;
      if (input_blobs.size()!=1) {
	throw ShowError(msg+"input_blobs.size()!=1");
      }

      size_t ss = 227, ds = 256-ss;
      vector<float> dataten(10*3*ss*ss);
      if (input_blobs[0]->count()%dataten.size()) {
	throw ShowError(msg+"input_blobs[0]->count() mod dataten.size() != 0 : "+
			ToStr(input_blobs[0]->count())+" vs "+
			ToStr(dataten.size()));
      }

      size_t n_ten_blocks = input_blobs[0]->count()/dataten.size();

      if (debug4)
	cout << " input_blobs[0]->count()=" << input_blobs[0]->count()
	     << " input_blobs[0]->num()=" << input_blobs[0]->num()
	     << " input_blobs[0]->channels()=" << input_blobs[0]->channels()
	     << " input_blobs[0]->width()=" << input_blobs[0]->width()
	     << " input_blobs[0]->height()=" << input_blobs[0]->height()
	     << " n_ten_blocks=" << n_ten_blocks
	     << endl;
      
      vector<float> dataall(n_ten_blocks*dataten.size());

      for (size_t iii=0; iii<n_ten_blocks && ilp!=ilist.end(); iii++, ilp++) {
	//Tic("RunCaffe() imagedata");

	// aspect ratio loss...
	imagedata img = *ilp;
	img.force_three_channel();

	if (!keep_aspect) {
	  scalinginfo si(img.width(), img.height(), width, height);
	  img.rescale(si, 1);

	} else {
	  if (width!=height)
	    throw ShowError(msg+"width!=height does not? work");
	    
	  // center square...
	  size_t iw = img.width(), ih = img.height();
	  size_t wh = iw<ih ? iw : ih, x = 0, y = 0;
	  if (iw>ih)
	    x = (iw-ih)/2;
	  else
	    y = (ih-iw)/2;
	  img = img.subimage(x, y, x+wh-1, y+wh-1);
	  scalinginfo si(wh, wh, width, height);
	  img.rescale(si, 1);
	}

	float *ip = (float*)img.raw_float(width*height*3);
	const float *mp = cmi->second.mimg.raw_float(width*height*3);
	if (mean_is_rgb)
	  for (size_t z=0; z<width*height*3; z++)
	    ip[z] = 255*(ip[z]-mp[z]);
	else // gbr
	  for (size_t z=0; z<width*height*3; z+=3) {
	    ip[z  ] = 255*(ip[z  ]-mp[z+2]); // R
	    ip[z+1] = 255*(ip[z+1]-mp[z+1]); // G
	    ip[z+2] = 255*(ip[z+2]-mp[z  ]); // B
	  }

	float *p = &dataten[0];
	for (size_t j=0; j<5; j++) {
	  size_t dx = (j==0||j==2) ? 0 : ds;
	  size_t dy = (j==0||j==1) ? 0 : ds;
	  if (j==4)
	    dx = dy = ds/2;
	  if (debug4)
	    cout << dx << " " << dy << " " << dx+ss-1 << " " << dy+ss-1 << endl;
	  imagedata cut = img.subimage(dx, dy, dx+ss-1, dy+ss-1);

	  float *q = p+3*ss*ss;
	  const float *cutp = cut.raw_float(ss*ss*3);
	  for (size_t y=0, pxy=0; y<ss; y++) {
	    size_t qxy = (y+1)*ss-1;
	    for (size_t x=0; x<ss; x++, pxy++, qxy--) {
	      int cuti =  3*(cut.width()*y+x); // cut.to_index_w_count(x, y);
	      p[pxy]         = q[qxy]         = cutp[cuti+2]; // B
	      p[pxy+ss*ss]   = q[qxy+ss*ss]   = cutp[cuti+1]; // G
	      p[pxy+ss*ss*2] = q[qxy+ss*ss*2] = cutp[cuti  ]; // R
	    }
	  }
	  p += 2*3*ss*ss;
	}
	//Tac("RunCaffe() imagedata");

	memcpy(&dataall[iii*dataten.size()], &dataten[0],
	       dataten.size()*sizeof(float));
      }

      const float *arr = &dataall[0];

      /*
	cout << "top left B first rows left" << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[3*227+q];
	cout << endl;

	cout << "top left G first rows left" << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+2*227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[51529+3*227+q];
	cout << endl;

	cout << "top left R last row right" << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+2*227+q];
	cout << endl;
	for (size_t q=0; q<=10; q++)
	cout << " " << arr[2*51529+3*227+q];
	cout << endl;
      */

      //Tic("RunCaffe() outcopy");

      switch (Caffe::mode()) {
      case Caffe::CPU:
	memcpy(input_blobs[0]->mutable_cpu_data(), arr,
	       sizeof(float) * input_blobs[0]->count());
	break;
      case Caffe::GPU:
	cudaMemcpy(input_blobs[0]->mutable_gpu_data(), arr,
		   sizeof(float) * input_blobs[0]->count(),
		   cudaMemcpyHostToDevice);
	break;
      default:
	LOG(FATAL) << "Unknown Caffe mode.";
      }  // switch (Caffe::mode())

      //Tac("RunCaffe() outcopy");

      //Tic("RunCaffe() forward");
      //LOG(INFO) << "Start";
      const vector<Blob<float>*>& output_blobs = cnet.ForwardPrefilled();
      //LOG(INFO) << "End";
      //Tac("RunCaffe() forward");

      if (output_blobs.size()!=1) {
	throw ShowError(msg+"output_blobs.size()!=1");
      }

      if (output_blobs[0]->num()!=10*(int)n_ten_blocks) {
	throw ShowError(msg+"output_blobs[0]->num()!=10*n_ten_blocks");
      }

      if (debug4)
	cout << " output_blobs[0]->count()=" << output_blobs[0]->count()
	     << " output_blobs[0]->num()=" << output_blobs[0]->num()
	     << " output_blobs[0]->channels()=" << output_blobs[0]->channels()
	     << " output_blobs[0]->width()=" << output_blobs[0]->width()
	     << " output_blobs[0]->height()=" << output_blobs[0]->height()
	     << endl;

      float *outarr = new float[output_blobs[0]->count()];

      //Tic("RunCaffe() incopy");

      switch (Caffe::mode()) {
      case Caffe::CPU:
	memcpy(outarr, output_blobs[0]->cpu_data(),
	       sizeof(float) * output_blobs[0]->count());
	break;
      case Caffe::GPU:
	cudaMemcpy(outarr, output_blobs[0]->gpu_data(),
		   sizeof(float) * output_blobs[0]->count(),
		   cudaMemcpyDeviceToHost);
	break;
      default:
	LOG(FATAL) << "Unknown Caffe mode.";
      }  // switch (Caffe::mode())
      
      //Tac("RunCaffe() incopy");

      //Tic("RunCaffe() postprocess");

      for (size_t jj=0; jj<n_ten_blocks && res.size()<ilist.size(); jj++) {
	if (level==0) {
	  const string& fusion = fusionx;
	  vector<float> resv(output_blobs[0]->channels());
	  for (int ii=0; ii<output_blobs[0]->channels(); ii++) {
	    if (debug4)
	      cout << "i=" << ii;
	    float sum = 0.0, max = 0.0;
	    for (int j=0; j<output_blobs[0]->num(); j++) {
	      size_t ij = jj*1000*10 + j*1000+ii;
	      if (debug4)
		cout << " " << outarr[ij];
	      sum += outarr[ij];
	      if (outarr[ij]>max)
		max = outarr[ij];
	    }
	    sum /= output_blobs[0]->num();
	    if (debug4)
	      cout << "  " << sum << endl;
	    resv[ii] = fusion=="amean" ? sum : fusion=="max" ? max : 0.0;
	  }
	
	  if (cmi->second.map.size()!=0) {
	    if (cmi->second.map.size()!=resv.size()) {
	      ShowError(msg+"dimensionality error in indexmap A");
	      // return;
	    }
	  
	    vector<float> tmp(output_blobs[0]->channels());
	    for (size_t ti=0; ti<resv.size(); ti++) {
	      size_t dst = cmi->second.map[ti];
	      if (dst>=tmp.size()) {
		ShowError(msg+"dimensionality error in indexmap B");
		// return;
	      }
	      tmp[dst] = resv[ti];
	    }
	    resv = tmp;
	  }
	  res.push_back(resv);

	} else { // level>0
	  if (use_gpu)
	    ShowError(msg+"use_gpu with level>0");

	  if (debug4) {
	    const string output_prefix = "proto";
	    const vector<string>& blob_names = cnet.blob_names();
	    const vector<boost::shared_ptr<Blob<float> > >&
	      blobs = cnet.blobs();
	    for (int blobid = 0; blobid < (int)cnet.blobs().size(); ++blobid) {
	      // Serialize blob
	      cout << "Dumping " << blob_names[blobid] << " "
		   << blobs[blobid]->channels() << "," << blobs[blobid]->num() 
		   << endl;
	      caffe::BlobProto output_blob_proto;
	      blobs[blobid]->ToProto(&output_blob_proto);
	      WriteProtoToBinaryFile(output_blob_proto, 
				     output_prefix + "-binary-" +
				     blob_names[blobid]);
	      //WriteProtoToTextFile(output_blob_proto, 
	      //			 output_prefix + "-text-" +
	      //                          blob_names[blobid]);
	    }
	  }

	  const string& fusion = fusionx;
	  if (level>=cnet.blobs().size())
	    ShowError(msg+"level>=cnet.blobs().size()");
	  size_t elevel = cnet.blobs().size()-1-level;
	  const vector<boost::shared_ptr<Blob<float> > >&
	    blobs = cnet.blobs();
	  caffe::BlobProto output_blob_proto;
	  blobs[elevel]->ToProto(&output_blob_proto);
	  auto feature_blob = blobs[elevel];
	  size_t num_features = feature_blob->num();
	  size_t dim_features = feature_blob->count() / num_features;
	  vector<float> sum(dim_features), max = sum, center;
	  for (size_t ff=0; ff<num_features; ff++) {
	    float *feature_blob_data = feature_blob->mutable_cpu_data() +
	      feature_blob->offset(ff);
	    vector<float> resv(dim_features);
	    memcpy(&resv[0], &feature_blob_data[0], 4*dim_features);
	    if (fusion=="center") {
	      if (num_features==10 && ff==8)
		center = resv;
	    } else if (fusion=="amean")
	      for (size_t vi=0; vi<dim_features; vi++)
		sum[vi] += resv[vi];
	    else if (fusion=="max") {
	      // cout << ff << " " << resv[42] << endl;
	      for (size_t vi=0; vi<dim_features; vi++)
		if (resv[vi]>max[vi])
		  max[vi] = resv[vi];
	    }
	  }
	  if (fusion=="center")
	    res.push_back(center);
	  else if (fusion=="amean") {
	    for (size_t vi=0; vi<dim_features; vi++)
	      sum[vi] /= num_features;
	    res.push_back(sum);
	  } else if (fusion=="max")
	    res.push_back(max);
	}
      }

      //Tac("RunCaffe() postprocess");

      delete outarr;
    }

    if (res.size()!=1)
      throw ShowError(msg+"res.size()!=1");

    d = res.front();

    //Tac("RunCaffe()");

    return true;
  }

  //===========================================================================

} // namespace picsom

#endif // HAVE_CAFFE_DATA_LAYERS_HPP
