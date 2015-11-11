##
## Copyright 1998-2008 PicSOM Development Group
## Helsinki University of Technology
## P.O.BOX 5400, FI-02015 HUT, FINLAND
## picsom@cis.hut.fi
##

# @(#)$Id: configure.ac,v 1.5 2010/03/25 09:24:50 jorma Exp $

AC_INIT(PicSOM, 0.1, picsom@cis.hut.fi)
AM_INIT_AUTOMAKE

AC_PREREQ(2.59)
AC_COPYRIGHT(PicSOM Development Group)
AC_REVISION($Revision: 1.5 $)

AC_CONFIG_SRCDIR([NEWS])
AC_CONFIG_FILES([Makefile])

#------------------------------------------------------------------------------

#DX_INIT_DOXYGEN(picsom,Doxyfile)

AC_PATH_PROG(DOXYGEN, doxygen, no)
AM_CONDITIONAL(DOXYGEN_ENABLED, test "x$DOXYGEN" != "xno")
AC_SUBST(DOXYGEN)

AC_PATH_PROG(FIG2DEV,fig2dev, no)
export FIG2DEV;
if test "$FIG2DEV" = "no"; then
  AC_MSG_WARN([Unable to find a fig2dev application]);
fi
AC_SUBST(FIG2DEV)

#------------------------------------------------------------------------------

AC_OUTPUT
