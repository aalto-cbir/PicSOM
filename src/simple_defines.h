// -*- C++ -*-

// $Id: simple_defines.h,v 2.0 2005/06/08 07:58:22 jorma Exp $

#define USE_DBMALLOC   0

#ifndef NO_PTHREADS
#define SIMPLE_USE_PTHREADS
#endif // NO_PTHREADS

#ifdef SIMPLE_USE_PTHREADS
#define CLASSIFIER_USE_PTHREADS
#endif // SIMPLE_USE_PTHREADS

#define SIMPLE_IMAGE   1
#define SIMPLE_FILE    1
#define SIMPLE_FEAT    0
#define SIMPLE_PLANAR  1

#define SIMPLE_CLASS_SGC       1
#define SIMPLE_CLASS_COMMITTEE 0
#define SIMPLE_CLASS_LLR       0
#define SIMPLE_CLASS_LSC       1
#define SIMPLE_CLASS_QDA       0
#define SIMPLE_CLASS_LVQ       0
#define SIMPLE_CLASS_SOM       1
#define SIMPLE_CLASS_TREESOM   1
#define SIMPLE_CLASS_PICSOM    1
#define SIMPLE_CLASS_LSCSOM    0
#define SIMPLE_CLASS_SSC       0
#define SIMPLE_CLASS_IMAGESOM  0
#define SIMPLE_CLASS_OCR       0
#define SIMPLE_CLASS_OCRTYPE   0

#define SIMPLE_DATA_VERODATA     0
#define SIMPLE_DATA_DEMODATA     0
#define SIMPLE_DATA_NISTDATA     0
#define SIMPLE_DATA_POSTIDATA    0
#define SIMPLE_DATA_UNIPENDATA   1
#define SIMPLE_DATA_STROKEDGLYPH 1
#define SIMPLE_DATA_SUBJECT      1

#define SIMPLE_MISC_DECAY 1

