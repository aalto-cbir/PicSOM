## -*- C++ -*-  $Id: Makefile.am,v 2.34 2015/11/11 10:21:21 jorma Exp $
## 
## Copyright 1998-2015 PicSOM Development Group <picsom@ics.aalto.fi>
## Aalto University School of Science
## PO Box 15400, FI-00076 Aalto, FINLAND
## 

bin_PROGRAMS    = picsom

if PICSOM_SOAP
  bin_PROGRAMS += picsom-soap
endif

picsom_SOURCES = PicSOM.C DataBase.C Analysis.C Query.C Index.C \
		 VectorIndex.C TSSOM.C SVM.C InvertedIndex.C WordInverse.C \
		 PreCalculatedIndex.C DummyIndex.C Util.C RemoteHost.C \
		 Valued.C Connection.C XMLutil.C base64.C missing-c-utils.C \
		 libsvm.C liblinear.C tron.C picsom-templates.C PmSVM.C \
		 picsom-main.C CbirAlgorithm.C CbirRandom.C CbirStageBased.C \
		 CbirPicsom.C PicSOMserver.C uiart-state.C bin_data.C \
		 ontology.C image/videofile.C image/imagedata.C \
		 simple/Simple.C simple/SubSpace.C simple/DataSet.C \
		 simple/SOM.C simple/TreeSOM.C simple/ConfusionMatrix.C \
		 simple/RandVar.C simple/Histogram.C simple/gzstream.C \
		 segmentation/Segmentation.C segmentation/segmentfile.C \
		 segmentation/AddDescription.C segmentation/DisplayResults.C \
		 features/Feature.C features/textfile.C \
		 features/PorterStemmer.C features/PixelBased.C \
		 features/Rgb.C features/RegionBased.C features/Raw.C \
		 features/TemplateBased.C features/Texture.C \
		 features/ColM.C features/Colour.C features/SampleColour.C \
		 features/ContourFD.C features/ZernikeMom.C \
		 features/EdgeHist.C features/EdgeCooccurence.C \
		 features/ColorLayout.C features/OCVCentrist.C \
		 features/ScalableColor.C features/DominantColor.C \
		 features/EdgeHistogram.C features/SimpleGeometric.C \
		 features/TextBased.C features/WordHist.C \
		 features/External.C features/IPLD.C features/ColorSIFT.C \
                 features/OCVLbp.C features/OCVFeature.C \
                 features/madaudiofile.C features/soundfile_impl.C \
                 features/sfaudiofile.C features/audiodata.C \
                 features/AudioBased.C features/AudioFreq.C \
                 features/AvgAudioEnergy.C features/EdgeFourier.C \
                 features/NGram.C features/Caffe.C features/VLFeat.C \
                 features/OCVMlhmsLbp.C features/Trajectory.C

## if FEATURES_EDGEFOURIER
##   picsom_SOURCES += features/EdgeFourier.C
## endif

EXTRA_picsom_SOURCES  = image/imagefile-unimpl.C image/imagefile-opencv.C
EXTRA_picsom_SOURCES += CbirExample.C CbirMyMethod.C CbirPinView.C

picsom_DEPENDENCIES = image/picsom-imagefile-@IMAGEFILEIMPL@.o @CBIRALGORITHMS@

picsom_CPPFLAGS = -std=c++11 -Wno-deprecated \
           -I./simple -I./image -I./features -I./segmentation \
	   -DPICSOM_IMAGEDATA_FAST @NOPTHREADS@ \
	   @XML_CPPFLAGS@ @LIBGCRYPT_CFLAGS@ @CSOAP_CFLAGS@ \
	   @OCTAVE_CFLAGS@ @MYSQL_CFLAGS@

picsom_LDADD = image/picsom-imagefile-@IMAGEFILEIMPL@.o \
               @CBIRALGORITHMS@ @XML_LIBS@ @LIBGCRYPT_LIBS@ \
               @LIBPTHREAD@ @CSOAP_LIBS@ @OCTAVE_LIBS@ @MYSQL_LDFLAGS@

picsom_soap_SOURCES  = picsom-soap.C PicSOMserver.C base64.C

picsom_soap_CPPFLAGS = -std=c++11 @CSOAP_CFLAGS@

picsom_soap_LDADD    = @LIBPTHREAD@ @CSOAP_LIBS@

