## 
## Copyright 1998-2008 PicSOM Development Group
## Helsinki University of Technology
## P.O.BOX 5400, FI-02015 HUT, FINLAND
## picsom@cis.hut.fi
## 

# @(#)$Id: Makefile.am,v 1.3 2008/10/31 11:55:33 jorma Exp $

AM_MAKEINFOFLAGS = --no-split
TEXI2DVI = texi2dvi --batch
TEXI2HTML = texi2html
TEXI2HTML_FLAGS = -split_chapter

info_TEXINFOS = picsom.texi databases.texi features.texi

# Files from texi2dvi that should be removed, but which Automake does
# not know.
#CLEANFILES = autoconf.ACs  autoconf.cvs  autoconf.MSs  autoconf.prs \
#             autoconf.ATs  autoconf.evs  autoconf.fns  autoconf.ovs \
#             autoconf.tmp \
#             autoconf*.html standards*.html


## ----------------------------- ##
## Other documentation formats.  ##
## ----------------------------- ##

html: picsom_1.html databases_1.html features_1.html

##

JPGTOEPS = convert
FIG2DEV  = fig2dev
EPSTOPDF = epstopdf

SUFFIXES = .pdf .eps .jpg .fig

.fig.eps: ; $(FIG2DEV) -L ps $< $@

.fig.jpg: ; $(FIG2DEV) -L jpeg $< $@

.jpg.eps: ; $(JPGTOEPS) $< $@

.eps.pdf: ; $(EPSTOPDF) $<

databases.dvi  :: dbstruct.eps
databases.html :: dbstruct.jpg
databases.pdf  :: dbstruct.pdf

features.dvi   :: image-zones.eps segments.eps mugshot-eye.eps
features.html  :: image-zones.jpg segments.jpg mugshot-eye.jpg
features.pdf   :: image-zones.pdf segments.pdf mugshot-eye.pdf

picsom.dvi     :: capture1.eps capture2.eps capture3.eps 
picsom.html    :: capture1.jpg capture2.jpg capture3.jpg 
picsom.pdf     :: capture1.pdf capture2.pdf capture3.pdf 

HTMLS = databases.html features.html picsom.html
PDFS  = databases.pdf  features.pdf  picsom.pdf 

#------------------------------------------------------------------------------

all : doc info $(HTMLS) $(PDFS)

if DOXYGEN_ENABLED
doc: 
	$(DOXYGEN) Doxyfile
else
.PHONY: doc
doc:
	@echo "No doxygen available"
endif

#------------------------------------------------------------------------------
