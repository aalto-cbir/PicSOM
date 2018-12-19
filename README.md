# PicSOM

PicSOM is a framework for image and video analyis, indexing and description.

## Copyright

Copyright 1998-2018 PicSOM Development Group <picsom@ics.aalto.fi>  
Aalto University School of Science  
PO Box 15400, FI-00076 Aalto, FINLAND

## Installation

```
autoreconf -f -i  
./configure --prefix=$HOME/picsom CXXFLAGS=-O3  
make -j 8  
make install
```

## Test

```$HOME/picsom/bin/picsom -v```

## Usage 

### 1. Downloading caffe models

If `caffe` libraries are available then ResNet models can be downloaded:

```
./scripts/download-caffe-resnet-models
```

### 2. Creating a video database

```
picsom=$HOME/picsom/bin/picsom  
mkdir /tmp/dir  
cd /tmp/dir  
wget https://sample-videos.com/video123/mp4/360/big_buck_bunny_360p_5mb.mp4  
$picsom -rw=sql analyse=insert database='testdb(usesqlite=yes,sqlsettings=1,labelformat=4,mediaclips=yes)' - .  
$picsom -rw=sql analyse=insertsubobjects database=testdb target=video+file videoinsert=frames - '*'
```

### 3. Running shot boundary detection

If `caffe` libraries are available and `ResNet-50` model has been downloaded:

```
feat=c_in12_rn50_pool5o_d_c
```

Otherwise:

```
feat=zo5:edgefourier
```

Then:

```
feat=zo5:edgefourier
$picsom -rw=fea -Dfeatures=2 analyse=create extractfeatures=true database=testdb target=image features=$feat  
$picsom -rw=det analyse=shotboundary database=testdb segmentspec=sbd features=$feat - features 0000  
$picsom -rw=sql analyse=shotboundarythreshold database=testdb segmentspec=bb threshold=0.35 detections=sbd::${feat}::20_0:: - 0000
```

### 4. Extracting more visual features

If `caffe` libraries are available and `ResNet-101` model has been downloaded:

```
feat=c_in12_rn101_pool5o_d_a
```

Otherwise:

```
feat=zo5:rgb
```

Then:

```
$picsom -rw=fea analyse=create extractfeatures=true database=testdb target=image queryrestriction='$middleframe(/:t/)' features=$feat
```

