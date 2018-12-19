# PicSOM

PicSOM is a framework for image and video analyis, indexing and description.

## Copyright

Copyright 1998-2018 PicSOM Development Group <picsom@ics.aalto.fi>  
Aalto University School of Science  
PO Box 15400, FI-00076 Aalto, FINLAND

## Installation

```
autoreconf -f -i  
./configure --prefix=$HOME/picsom CFLAGS=-O3  
make -j 8  
make install
```

## Test

```$HOME/picsom/bin/picsom -v```

## Usage 

### 1. Create a video database

```
picsom=$HOME/picsom/bin/picsom  
mkdir tmpdir  
cd tmpdir  
wget https://sample-videos.com/video123/mp4/360/big_buck_bunny_360p_5mb.mp4  
$picsom -rw=sql analyse=insert database='testdb(usesqlite=yes,sqlsettings=1,labelformat=4,mediaclips=yes)' - .  
$picsom -rw=sql analyse=insertsubobjects database=testdb target=video+file videoinsert=frames - '*'  
$picsom -rw=fea -Dfeatures=2 analyse=create extractfeatures=true database=testdb target=image features=zo5:edgefourier
```

### 2. Run shot boundary detection

```
$picsom -rw=det analyse=shotboundary database=testdb segmentspec=sbd features=zo5:edgefourier - features 0000
$picsom -rw=sql analyse=shotboundarythreshold database=testdb segmentspec=bb threshold=0.35 detections=sbd::zo5:edgefourier::20_0:: - 0000
```

### 3. Extract visual features

```
$picsom -rw=fea analyse=create extractfeatures=true database=testdb target=image queryrestriction='$middleframe(/:t/)' features=zo5:rgb
```
