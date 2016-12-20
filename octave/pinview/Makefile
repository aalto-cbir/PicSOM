## -*- C++ -*-  $Id: Makefile,v 1.7 2013/02/09 08:44:02 jorma Exp $
## 
## Copyright 1998-2013 PicSOM Development Group <picsom@ics.aalto.fi>
## Aalto University School of Science
## PO Box 15400, FI-00076 Aalto, FINLAND
## 

SRC         = ../../src
CBIRPINVIEW = $(SRC)/CbirPinView.C
OWNINCLUDES = -I$(SRC) -I$(SRC)/segmentation -I$(SRC)/simple -I$(SRC)/image

OCTAVECONFIG  = $(shell which octave-config)
OCTINCLUDEDIR = $(shell $(OCTAVECONFIG) -p OCTINCLUDEDIR)
OCTLIBDIR     = $(shell $(OCTAVECONFIG) -p OCTLIBDIR)

MYSQLCONFIG  = $(shell which mysql_config)
MYSQLCFLAGSX = $(shell $(MYSQLCONFIG) --cflags)
MYSQLCFLAGS  = -I/usr/include/mysql
MYSQLLDFALGS = $(shell $(MYSQLCONFIG) --libs)

CSOAPCONFIG  = $(shell which csoap-config)
CSOAPCFLAGSX = $(shell $(CSOAPCONFIG) --cflags)
CSOAPLDFLAGS = $(shell $(CSOAPCONFIG) --libs)

CSOAPCFLAGS  = $(patsubst -pthread,,$(CSOAPCFLAGSX))

FLAGS = $(OWNINCLUDES) $(MYSQLCFLAGS) $(CSOAPCFLAGS) -I$(OCTINCLUDEDIR)

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
	DARWINENV = DL_LDFLAGS="-bundle -bundle_loader $(SRC)/picsom"
endif

CbirPinView.oct : $(CBIRPINVIEW)
	env INCFLAGS=-I. $(DARWINENV) \
	  mkoctfile -DMKOCTFILE $(FLAGS) -v $(CBIRPINVIEW)
	ln -sf CbirPinView.oct pinview_indexfullnamelist.oct
	ln -sf CbirPinView.oct pinview_indexfullname.oct
	ln -sf CbirPinView.oct pinview_featuredata.oct
	ln -sf CbirPinView.oct pinview_tssomcoord.oct
	rm $(SRC)/CbirPinView.o

