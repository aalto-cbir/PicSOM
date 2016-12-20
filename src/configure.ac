## -*- C++ -*-  $Id: configure.ac,v 2.112 2016/12/19 21:11:40 jorma Exp $
## 
## Copyright 1998-2016 PicSOM Development Group <picsom@ics.aalto.fi>
## Aalto University School of Science
## PO Box 15400, FI-00076 Aalto, FINLAND
## 

AC_INIT(PicSOM, 0.34, picsom@ics.aalto.fi)
AM_INIT_AUTOMAKE([subdir-objects])

AC_PREREQ(2.59)
AC_COPYRIGHT(PicSOM Development Group)
AC_REVISION($Revision: 2.112 $)

AC_CONFIG_SRCDIR([PicSOM.C])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)

# The following should be used in MSYS:
# but these should come from Makefile.include...
#CXXFLAGS='-std=gnu++0x'
#CXXFLAGS='-std=c++0x'

# Checks for libraries.

AX_LIB_MYSQL() 

# Checks for header files. (Not yet available in Ubuntu 8.10)
#AC_DEFAULT_INCLUDES()

AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT

# Some common? headers first
AC_CHECK_HEADERS([sys/types.h math.h time.h limits.h stdio.h mkl.h])

# Headers found only in linux
AC_CHECK_HEADERS([sys/syscall.h ext/stdio_filebuf.h])

# Headers found only in non-WIN32
AC_CHECK_HEADERS([netdb.h pwd.h detdb.h sys/resource.h sys/utsname.h \
		  sys/socket.h netinet/in.h arpa/inet.h unistd.h \
		  sys/loadavg.h sys/param.h sys/wait.h inttypes.h strings.h \
		  sys/resource.h sys/stat.h sys/mman.h \
		  glog/logging.h cuda_runtime.h])

# Headers found only in WIN32
AC_CHECK_HEADERS([winsock2.h winreg.h basetsd.h io.h process.h ws2tcpip.h])

# Are these needed ?
AC_CHECK_HEADERS([stropts.h sys/times.h regex.h sys/cdefs.h sys/ioctl.h \
                  fcntl.h poll.h stdlib.h string.h wchar.h wctype.h])

# Headers that are always needed
AC_CHECK_HEADER([zlib.h],[],[errors="$errors|zlib.h missing."])

# Headers that trigger some specials
AC_CHECK_HEADERS([mad.h],[],
        [warnings="$warnings|libmad use disabled due to missing header."])

AC_CHECK_HEADERS([sndfile.h],[],
        [warnings="$warnings|libsndfile use disabled due to missing header."])

AC_CHECK_HEADERS([audiofile.h],[],
        [warnings="$warnings|libaudiofile use disabled due to missing header."])

AC_CHECK_HEADERS([fftw3.h],[],
        [warnings="$warnings|libfftw3 use disabled due to missing header."])

AC_CHECK_HEADERS([magic.h],[],
        [warnings="$warnings|libmagic use disabled due to missing header."])

AC_CHECK_HEADERS([sqlite3.h],[],
        [warnings="$warnings|SQLite3 use disabled due to missing header."])

AC_CHECK_HEADERS([mysql.h],[],
        [warnings="$warnings|MySQL use disabled due to missing header."])

AC_CHECK_HEADERS([mpi.h],[],
        [warnings="$warnings|MPI use disabled due to missing header."])

AC_CHECK_HEADERS([openssl/ssl.h],[],
        [warnings="$warnings|OpenSSL use disabled due to missing header."])

AC_CHECK_HEADERS([wn.h],[],
        [warnings="$warnings|WordNet use disabled due to missing header."])

AC_CHECK_HEADERS([WNSim.h],[],
        [warnings="$warnings|WNSim use disabled due to missing header."])

AC_CHECK_HEADERS([jaula.h],[],
        [warnings="$warnings|JAULA use disabled due to missing header."])

AC_CHECK_HEADERS([json/json.h],[],
        [warnings="$warnings|jsoncpp use disabled due to missing header."])

AC_CHECK_HEADERS([aspell.h],[],
        [warnings="$warnings|Aspell use disabled due to missing header."])

AC_CHECK_HEADERS([flandmark_detector.h],[],
        [warnings="$warnings|libflandmark use disabled due to missing header."])

AC_CHECK_HEADERS([vl/generic.h],[],
        [warnings="$warnings|vlfeat use disabled due to missing header."])

AC_CHECK_HEADERS([dense_trajectory_release_v1.2/DenseTrack.h],[],
        [warnings="$warnings|trajectory use disabled due to missing header."])

AC_CHECK_HEADERS([improved_trajectory_release/DenseTrackStab.h],[],
        [warnings="$warnings|improvedtrajectory use disabled due to missing header."])

AC_CHECK_HEADERS([caffe/caffe.hpp caffe/data_transformer.hpp],[],
        [warnings="$warnings|Caffe use disabled due to missing header."])

AC_CHECK_HEADERS([libavcodec/avcodec.h libavdevice/avdevice.h   \
		  libavfilter/avfilter.h libavformat/avformat.h \
		  libavutil/avutil.h libswscale/swscale.h] \
		 ,,,[#define __STDC_CONSTANT_MACROS])

AC_CHECK_DECLS([CLOCK_REALTIME],,,[#include<time.h>])
AC_CHECK_DECLS([environ])
AC_CHECK_DECLS([M_PI],,,[#include<math.h>])
AC_CHECK_DECLS([W_OK])
AC_CHECK_DECLS([STDIN_FILENO,STDOUT_FILENO,STDERR_FILENO],
	AC_DEFINE(HAVE_STDXXX_FILENO,1,[...]),,[#include <unistd.h>])

AC_CHECK_TYPES([time_t, ssize_t, uid_t, pid_t, mode_t])
AC_CHECK_TYPES([struct timespec])
AC_CHECK_TYPES([timespec_t],,,[#include<time.h>])
AC_CHECK_TYPES([sockaddr_in],,,[#include<netinet/in.h>])
AC_CHECK_TYPES([sigset_t],,,[#include<signal.h>])

AC_CHECK_TYPES([struct hostent],,,[
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
])

AC_CHECK_LIB(rt,      clock_gettime)

# these are replicated in missing-c-utils.[Ch]
AC_CHECK_FUNCS([fork kill sleep _sleep getuid getpwuid getpwnam inet_aton  \
 		ntohs times clock_gettime localtime_r strptime nanosleep \
                gethostname gethostbyname getpagesize getloadavg \
                getcwd getpid link symlink rmdir round \
		strcasecmp mkstemp erand48 nrand48 rand_r chmod mkdir access \
                popen pclose erf gamma snprintf fcntl unlink \
                getaddrinfo freeaddrinfo])

# these are used with #ifdef HAVE_xxx in some place where needed
AC_CHECK_FUNCS([sysconf strsignal getppid pipe ttyname waitpid setenv isatty \
		readlink])

# these are perhaps not needed at all
AC_CHECK_FUNCS([gethostbyname inet_ntoa memmove memset])

# don't yet know what to do with this one...
AC_CHECK_FUNCS([mremap])

AC_CHECK_LIB(lapack,    main)

AC_CHECK_FUNCS([ilaenv_], [], 
  [warnings="$warnings|LAPACK use disabled due to missing library."])

AM_PATH_LIBGCRYPT
AC_CHECK_HEADERS([gcrypt.h])

AM_ICONV

AC_CHECK_LIB(mad,       main)
AC_CHECK_LIB(sndfile,   main)
AC_CHECK_LIB(audiofile, main)
AC_CHECK_LIB(fftw3,     main)

AC_CHECK_LIB(sqlite3, sqlite3_open)

AC_CHECK_LIB(dl, dlopen)

AC_CHECK_LIB(mpi,     main)
AC_CHECK_LIB(mpi_cxx, main)

AC_CHECK_LIB(flandmark_shared, main)

has_pthreads=false
AC_CHECK_LIB(pthread, pthread_create, has_pthreads=true)
if test "$has_pthreads" = "true"; then
  AC_CHECK_HEADER([pthread.h],[],[has_pthreads=false])
fi
if test "$has_pthreads" = "true"; then
  AC_SUBST(LIBPTHREAD,"-lpthread")
  notices="$notices|POSIX threads will be used."
else
  AC_SUBST(NOPTHREADS,"-DNO_PTHREADS")
  notices="$notices|POSIX threads not available."
fi

# general stuff
AC_CHECK_LIB(magic, main)

# needed in MSYS/MinGW
AH_TEMPLATE([MKDIR_ONE_ARG], [Defined in MSYS/MinGW])
AC_CHECK_LIB(gdi32,  main)
AC_CHECK_LIB(ws2_32, main, [AC_DEFINE(MKDIR_ONE_ARG, 1)])

# general stuff
AC_CHECK_LIB(z,     main)
AC_CHECK_LIB(iconv, main)
AC_CHECK_LIB(regex, main)

# needed in SunOS
AC_CHECK_LIB(nsl, main)
AC_CHECK_LIB(socket, main)

AC_CHECK_LIB(jpeg,      main)
AC_CHECK_LIB(tiff,      main)
AC_CHECK_LIB(png,       main)

AC_CHECK_LIB(gfortran,  main)
AC_CHECK_LIB(blas,      main)

AC_CHECK_LIB(ssl,       main)
AC_CHECK_LIB(crypto,    main)
AC_CHECK_LIB(aspell,    main)
AC_CHECK_LIB(wordnet,   main)
AC_CHECK_LIB(WN,        main)
AC_CHECK_LIB(vl,        main)

## ImageMagick++, GraphicsMagick++ and picsom::imagefile
#magicktype="imagemagick"
#AC_CHECK_HEADER([Magick++.h],[],[magicktype="none"])
#AC_CHECK_LIB(Magick,    main,,magicktype="none")
#AC_CHECK_LIB(Wand,      main)
#AC_CHECK_LIB(Magick++,  main,,magicktype="none")
#
#if test "$magicktype" = "none"; then
#  magicktype="graphicsmagick"
#  save_cxxflags="$CXXFLAGS"
#  save_cppflags="$CPPFLAGS"
#  gminc1=-I/usr/include/GraphicsMagick
#  gminc2=-I/opt/local/include/GraphicsMagick
#  CPPFLAGS="$CPPFLAGS $gminc1 $gminc2"
#  INCLUDES=-I/tmp/include
#  AC_CHECK_HEADER([GraphicsMagick/Magick++.h], \
#                  [CXXFLAGS="$CXXFLAGS $gminc1 $gminc2"], \
#                  [magicktype="none"])
#  AC_CHECK_LIB(GraphicsMagick,    main,,magicktype="none")
#  AC_CHECK_LIB(GraphicsMagick++,  main,,magicktype="none")
#  if test "$magicktype" = "none"; then
#    CXXFLAGS="$save_cxxflags"
#    CPPFLAGS="$save_cppflags"
#  fi
#fi
#
#if test "$magicktype" = "none"; then
#  warnings="$warnings|No imagefile support from ImageMagick++"
#  warnings="$warnings nor GraphicsMagick++ available."
#else
#  AC_CHECK_HEADER([tiffio.h],[],[
#    magicktype="none"
#    warnings="$warnings|No imagefile support available"
#    warnings="$warnings due to missing tiffio.h."
#  ])
#fi

magicktype="opencv"
AC_CHECK_HEADERS([opencv2/core/core.hpp],[],magicktype="none")
AC_CHECK_HEADERS([opencv2/core/gpumat.hpp],[],[])
AC_CHECK_HEADERS([opencv2/nonfree/nonfree.hpp],[],[])
AC_CHECK_HEADERS([opencv2/nonfree/gpu.hpp],[],[])
AC_CHECK_LIB(opencv_core,       main,,magicktype="none")
AC_CHECK_LIB(opencv_highgui,    main,,magicktype="none")
AC_CHECK_LIB(opencv_flann,      main,,magicktype="none")
AC_CHECK_LIB(opencv_features2d, main,,magicktype="none")
AC_CHECK_LIB(opencv_objdetect,  main,,magicktype="none")
AC_CHECK_LIB(opencv_imgproc,    main,,magicktype="none")

if test "$magicktype" = "none"; then
  warnings="$warnings|OpenCV use disabled due to missing header or library."
fi

if test "$magicktype" = "none"; then
  AC_SUBST(IMAGEFILEIMPL,unimpl)
else
  AC_SUBST(IMAGEFILEIMPL,opencv)
fi

## # fftw3 header+lib and features/EdgeFourier.C
## havefftw3="yes"
## AC_CHECK_HEADER([fftw3.h],[],[havefftw3="no"])
## AC_CHECK_LIB(fftw3, main,[],[havefftw3="no"])
## if test "$havefftw3" = "no"; then
##   warnings="$warnings|EdgeFourier features disabled due to missing fftw3."
## fi
## AM_CONDITIONAL([FEATURES_EDGEFOURIER], [test x$havefftw3 = xyes])

# The following possibly needs to be commented out in MSYS:
AM_PATH_XML2(2.0.0,[],[errors="$errors|xml2 library missing."])

AH_TEMPLATE([PICSOM_USE_CSOAP], [Defined if we have support for SOAP])
havecsoap=yes
AM_PATH_MYCSOAP(1.1.0, [AC_DEFINE(PICSOM_USE_CSOAP, 1)],[
  havecsoap=no
  warnings="$warnings|SOAP support disabled due to missing libcsoap."
])
AM_CONDITIONAL([PICSOM_SOAP], [test x$havecsoap = xyes])

AC_CHECK_LIB(avcodec,   main)
AC_CHECK_LIB(avformat,  main)
AC_CHECK_LIB(avutil,    main)
AC_CHECK_LIB(swscale,   main)

AC_CHECK_LIB(caffe,     main)
AC_CHECK_LIB(glog,      main)

AH_TEMPLATE([PICSOM_HAVE_OCTAVE], [Defined if we have support for Octave])
AM_PATH_MYOCTAVE(3.0.0, [AC_DEFINE(PICSOM_HAVE_OCTAVE, 1)],[
                       warnings="$warnings|Octave support disabled."])

# AM_WITH_REGEX
# AM_PATH_XINE
# iconv.h
# regex.h

#------------------------------------------------------------------------------

# --enable-cbir=XXX

CBIRALGORITHMS=""
AC_ARG_ENABLE([cbir],
[AS_HELP_STRING([--enable-cbir],
  [Specify external CbirAlgorithms to be compiled and linked in PicSOM,
   e.g., --enable-cbir=Example,MyMethod,...])],
[ if test "X$enableval" = "X"; then
	errors="$errors|--enable-cbir is missing an argument."
  else
	echo ""
	cbirs=`echo $enableval | tr , " "`
	for cbir in $cbirs
	do
		cbiro=picsom-Cbir$cbir.'$(OBJEXT)'
		echo Enabling CbirAlgorithm Cbir$cbir
		CBIRALGORITHMS="$CBIRALGORITHMS $cbiro"
	done
	echo ""
  fi
])
AC_SUBST([CBIRALGORITHMS])

#------------------------------------------------------------------------------

AC_OUTPUT

if test "$no_create" = "yes"
then
  exit 0
fi

#------------------------------------------------------------------------------

ac_save_ifs="$IFS"
IFS="|"
if test "$verbose" = "yes"; then
    for msg in $notices; do
        if test -n "$msg"; then
            echo >&2
            echo "NOTICE: $msg";
           #AC_MSG_NOTICE([$msg])
        fi
    done
fi
for msg in $warnings; do
    if test -n "$msg"; then
        echo >&2
        echo "WARNING: $msg";
       #AC_MSG_WARN([$msg])
    fi
done
if test -n "$errors"; then
   for msg in $errors; do
       if test -n "$msg"; then
	  echo >&2;
	  echo "ERROR: $msg";
       fi
   done
   IFS="$ac_save_ifs"
   echo "Configure ending due to above errors."
   exit 1   
  #AC_MSG_ERROR([Configure ending due to above errors.])
fi
IFS="$ac_save_ifs"

#------------------------------------------------------------------------------

echo
echo "Configure finished successfully."
echo "Run '${ac_make} depend && ${ac_make}' to compile."
echo

#------------------------------------------------------------------------------
