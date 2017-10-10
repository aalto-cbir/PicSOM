## -*- Makefile -*-  $Id: Makefile.am,v 2.37 2017/06/20 08:16:25 jormal Exp $
## 
## Copyright 1998-2017 PicSOM Development Group <picsom@ics.aalto.fi>
## Aalto University School of Science
## PO Box 15400, FI-00076 Aalto, FINLAND
## 

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libpicsom.la

bin_PROGRAMS    = picsom
#bin_PROGRAMS    = 

if PICSOM_STATIC
  bin_PROGRAMS  += picsom-static
endif

if PICSOM_SOAP
  bin_PROGRAMS += picsom-soap
endif

SOURCEFILELIST = PicSOM.C DataBase.C Analysis.C Query.C Index.C \
		 VectorIndex.C TSSOM.C SVM.C InvertedIndex.C WordInverse.C \
		 PreCalculatedIndex.C DummyIndex.C Util.C RemoteHost.C \
		 Valued.C Connection.C XMLutil.C base64.C \
		 libsvm.C liblinear.C tron.C picsom-templates.C PmSVM.C \
		 CbirAlgorithm.C CbirRandom.C CbirStageBased.C \
		 CbirPicsom.C PicSOMserver.C uiart-state.C bin_data.C \
		 ontology.C image/videofile.C image/imagedata.C \
		 simple/Simple.C simple/SubSpace.C simple/DataSet.C \
		 simple/SOM.C simple/TreeSOM.C simple/ConfusionMatrix.C \
		 simple/RandVar.C simple/Histogram.C simple/gzstream.C \
		 segmentation/Segmentation.C segmentation/segmentfile.C \
		 segmentation/AddDescription.C segmentation/DisplayResults.C \
                 segmentation/AlignmentMarks.C segmentation/Binarise.C \
                 segmentation/ByTypicality.C segmentation/CreateThumbnail.C \
                 segmentation/CropEdges.C \
                 segmentation/FaceParts.C segmentation/FilterbyBoundingbox.C \
                 segmentation/FilterbyColor.C segmentation/FilterCentral.C \
                 segmentation/FLandmark.C segmentation/GenerateHeatmap.C \
                 segmentation/GKMeans.C \
                 segmentation/ImportBitmap.C segmentation/KMeans.C \
                 segmentation/ListAdjacent.C segmentation/ListOverlaps.C \
                 segmentation/LocateFaceBox.C segmentation/MergeColM.C \
                 segmentation/MergeNeighbourhood.C segmentation/MergeSame.C \
                 segmentation/OCVface.C segmentation/PickLargest.C \
                 segmentation/PixelTemplate.C \
                 segmentation/Random.C segmentation/RegionMerge.C \
                 segmentation/RegionMerger.C segmentation/RemoveHoles.C \
                 segmentation/RestoreScale.C segmentation/SampleRandom.C \
                 segmentation/ScaleDown.C segmentation/SeparateMultipart.C \
                 segmentation/SpringModel.C segmentation/TagDark.C \
                 segmentation/TemporalMorphology.C segmentation/Tile.C \
                 segmentation/TestInterface.C segmentation/TilePyramid.C  \
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
                 features/OCVMlhmsLbp.C features/Trajectory.C features/Torch.C

otherincludedir = $(includedir)/picsom

otherinclude_HEADERS = Analysis.h aspects.h base64.h bin_data.h CbirAlgorithm.h CbirExample.h \
                  CbirGVT.h CbirPicsom.h CbirPinView.h CbirRandom.h CbirStageBased.h \
                  Connection.h DataBase.h DummyIndex.h features/AudioBased.h \
                  features/audiodata.h features/AudioFreq.h features/AvgAudioEnergy.h \
                  features/Caffe.h features/Cepstrum.h features/ColM.h \
                  features/ColorLayout.h features/ColorSIFT.h features/Colour.h \
                  features/ContourFD.h features/CVC.h features/DominantColor.h \
                  features/Dummy.h features/EdgeCooccurence.h features/EdgeFourier.h \
                  features/EdgeHist.h features/EdgeHistogram.h features/External.h \
                  features/ExternalSound.h features/Feature.h \
                  features/ImprovedTrajectory.h features/IPLD.h features/madaudiofile.h \
                  features/MPEG7_XM.h features/MsgDate.h features/MuvisFeat.h \
                  features/NGram.h features/OCVBoV.h features/OCVCentrist.h \
                  features/OCVFeature.h features/OCVHoG.h features/OCVKeyPoint.h \
                  features/OCVLbp.h features/OCVMlhmsLbp.h features/pdffile.h \
                  features/PixelBased.h features/PorterStemmer.h features/PseudoMPEG7.h \
                  features/Raw.h features/RegionBased.h features/Rgb.h \
                  features/SampleColour.h features/ScalableColor.h \
                  features/sfaudiofile.h features/SIFT.h features/SimpleGeometric.h \
                  features/SLMHeadPose.h features/soundfile.h features/soundfile_impl.h \
                  features/TemplateBased.h features/TextBased.h features/textdata.h \
                  features/textfile.h features/Texture.h features/Torch.h \
                  features/Trajectory.h features/VLFeat.h features/WebLink.h \
                  features/WordHist.h features/ZernikeMom.h ground_truth.h \
                  image/imagecolor.h image/imagedata.h image/imagefile.h \
                  image/imagescaling.h image/videofile.h Index.h InvertedIndex.h \
                  liblinear.h libsvm.h ModifiableFile.h object_info.h \
                  object_set.h ontology.h picsom-config.h.in PicSOM.h PicSOMserver.h \
                  PmSVM.h PreCalculatedIndex.h Query.h RemoteHost.h RwLock.h \
                  segmentation/AddDescription.h segmentation/AlignmentMarks.h \
                  segmentation/Binarise.h segmentation/ByTypicality.h \
                  segmentation/ChainCode.h segmentation/ChainVisualiser.h \
                  segmentation/ConnectedKMeans.h segmentation/ConstBackground.h \
                  segmentation/CreateThumbnail.h segmentation/CropEdges.h \
                  segmentation/DisplayResults.h segmentation/EndBlocker.h \
                  segmentation/FaceParts.h segmentation/FeatureInterface.h \
                  segmentation/FilterbyBoundingbox.h segmentation/FilterbyColor.h \
                  segmentation/FilterCentral.h segmentation/FLandmark.h \
                  segmentation/GenerateHeatmap.h segmentation/GKMeans.h \
                  segmentation/GradientMagn.h segmentation/ImportBitmap.h \
                  segmentation/KMeans.h segmentation/LBModel.h \
                  segmentation/ListAdjacent.h segmentation/ListOverlaps.h \
                  segmentation/LocateFaceBox.h segmentation/LocateFace.h \
                  segmentation/MedFilt.h segmentation/MergeColM.h \
                  segmentation/MergeNeighbourhood.h segmentation/MergeSame.h \
                  segmentation/MyMethod.h segmentation/OCVface.h \
                  segmentation/PickLargest.h segmentation/PixelTemplate.h \
                  segmentation/PrePostProcessor.h segmentation/Random.h \
                  segmentation/RegionMerge.h segmentation/RegionMerger.h \
                  segmentation/RemoveHoles.h segmentation/RestoreScale.h \
                  segmentation/SampleRandom.h segmentation/ScaleDown.h \
                  segmentation/ScaleSal.h segmentation/Segmentation.h \
                  segmentation/SegmentationTree.h segmentation/segmentfile.h \
                  segmentation/SeparateMultipart.h segmentation/Snake.h \
                  segmentation/SpringModel.h segmentation/TagDark.h \
                  segmentation/TemporalMorphology.h segmentation/TestInterface.h \
                  segmentation/TestSegment.h segmentation/Tile.h \
                  segmentation/TilePyramid.h simple/Classifier.h \
                  simple/ConfusionMatrix.h simple/DataSet.h simple/DecayVar.h \
                  simple_defines.h simple/FigFile.h simple/GnuPlot.h simple/gzstream.h \
                  simple/Histogram.h simple/List.h simple/Matrix.h simple/OptVar.h \
                  simple/Planar.h simple/Point.h simple/RandVar.h simple/SimpleComplex.h \
                  simple/Simple.h simple/SOM.h simple/Source.h simple/StatQuad.h \
                  simple/StatVar.h simple/SubSpace.h simple/Transformation.h \
                  simple/TreeSOM.h simple/Vector.h simple/VectorSet.h \
                  simple/VectorSource.h SVM.h TicTac.h TimeUtil.h tron.h TSSOM.h \
                  uiart-state.h Util.h Valued.h ValuedList.h VectorIndex.h WordInverse.h \
                  XMLutil.h

coxincludedir = $(includedir)/cox

coxinclude_HEADERS = cox/blas cox/classifier cox/demangle cox/knn cox/labeled \
		     cox/labeled_vector cox/lsc cox/matlab cox/matrix \
                     cox/sompak cox/stat cox/subspace cox/tictac

###

nodist_otherinclude_HEADERS = picsom-config.h

###

libpicsom_la_SOURCES  = $(SOURCEFILELIST)

EXTRA_libpicsom_la_SOURCES  = image/imagefile-unimpl.C image/imagefile-opencv.C
EXTRA_libpicsom_la_SOURCES += CbirExample.C CbirMyMethod.C CbirPinView.C

libpicsom_la_DEPENDENCIES  = image/libpicsom_la-imagefile-@IMAGEFILEIMPL@.lo @CBIRALGORITHMSLA@

libpicsom_la_CPPFLAGS = -std=c++11 -Wno-deprecated \
           -I./simple -I./image -I./features -I./segmentation \
	   -DPICSOM_IMAGEDATA_FAST -DPICSOM_USE_OPENCV @NOPTHREADS@ \
	   @XML_CPPFLAGS@ @LIBGCRYPT_CFLAGS@ @CSOAP_CFLAGS@ \
	   @OCTAVE_CFLAGS@ @MYSQL_CFLAGS@

libpicsom_la_LIBADD = image/libpicsom_la-imagefile-@IMAGEFILEIMPL@.lo @CBIRALGORITHMSLA@

libpicsom_la_LDFLAGS  = -version-info 0:35

###

picsom_SOURCES = picsom-main.C

picsom_DEPENDENCIES = libpicsom.la 

picsom_CPPFLAGS = $(libpicsom_la_CPPFLAGS)

picsom_LDADD = -L./.libs -lpicsom

###

picsom_static_SOURCES = $(SOURCEFILELIST) picsom-main.C

## if FEATURES_EDGEFOURIER
##   picsom_static_SOURCES += features/EdgeFourier.C
## endif

EXTRA_picsom_static_SOURCES  = image/imagefile-unimpl.C image/imagefile-opencv.C
EXTRA_picsom_static_SOURCES += CbirExample.C CbirMyMethod.C CbirPinView.C

picsom_static_DEPENDENCIES = image/picsom-imagefile-@IMAGEFILEIMPL@.o @CBIRALGORITHMS@

picsom_static_CPPFLAGS = -std=c++11 -Wno-deprecated \
           -I./simple -I./image -I./features -I./segmentation \
	   -DPICSOM_IMAGEDATA_FAST -DPICSOM_USE_OPENCV @NOPTHREADS@ \
	   @XML_CPPFLAGS@ @LIBGCRYPT_CFLAGS@ @CSOAP_CFLAGS@ \
	   @OCTAVE_CFLAGS@ @MYSQL_CFLAGS@

picsom_static_LDADD = image/picsom-imagefile-@IMAGEFILEIMPL@.o \
               @CBIRALGORITHMS@ @XML_LIBS@ @LIBGCRYPT_LIBS@ \
               @LIBPTHREAD@ @CSOAP_LIBS@ @OCTAVE_LIBS@ @MYSQL_LDFLAGS@

###

picsom_soap_SOURCES  = picsom-soap.C PicSOMserver.C base64.C

picsom_soap_CPPFLAGS = -std=c++11 @CSOAP_CFLAGS@

picsom_soap_LDADD    = @LIBPTHREAD@ @CSOAP_LIBS@

