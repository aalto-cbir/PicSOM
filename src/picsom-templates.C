// -*- C++ -*-  $Id: picsom-templates.C,v 2.3 2012/08/20 08:41:12 jorma Exp $
// 
// Copyright 1998-2012 PicSOM Development Group <picsom@cis.hut.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

#include <PicSOM.h>

//-----------------------------------------------------------------------------

#if !defined(HAVE_CONFIG_H)
#if defined(__GNUC__)

#if !defined(__MINGW32__)

#include <Util.h>
#include <Valued.h>
#include <Connection.h>

#include <Source.C>
#include <List.C>
#include <Point.C>
#include <Vector.C>
#include <VectorSource.C>
#include <VectorSet.C>
#include <Matrix.C>
#include <GnuPlot.C>
#include <FigFile.h>
#include <Classifier.C>

using namespace simple;

template class ListOf<FigFile::FigAttr>;

template class ListOf<GnuPlot>;
template class ListOf<Classifier>;

template class VectorOf<int>;
template class VectorOf<float>;
template class VectorOf<double>;
template class VectorOf<PointOf<int> >;
template class VectorOf<PointOf<float> >;
template class VectorOf<StatQuad>;

template class MatrixOf<int>;
template class MatrixOf<float>;

template class ListOf<MatrixOf<float> >;
template class ListOf<MatrixOf<int> >;

template class ListOf<VectorOf<int> >;
template class ListOf<VectorOf<float> >;
template class ListOf<VectorOf<StatQuad> >;

template class SourceOf<VectorOf<StatQuad> >;

template class SourceOf<VectorOf<float> >;
template class VectorSourceOf<float>;
template class VectorSetOf<float>;

template class VectorSourceOf<int>;
template class SourceOf<VectorSetOf<int> >;

template class ListOf<SOM::Vote>;

template class SourceOf<GnuPlotData>;
template class ListOf<GnuPlotData>;
template class ListOf<VectorSetOf<int> >;

#ifdef __INTEL_COMPILER

template void VectorSetOf<int>::Initialize();

#endif // __INTEL_COMPILER

template VectorOf<StatVar>::VectorOf(int, StatVar const*, char const*, bool);

template class VectorSetOf<int>;
template class SourceOf<VectorOf<int> >;
template class SourceOf<ListOf<VectorOf<int> > >;
template class ListOf<ListOf<VectorOf<int> > >;
template class PointOf<int>;
template class PointOf<float>;
template class SourceOf<SOM::Vote>;
template class SourceOf<Classifier>;
template class SourceOf<GnuPlot>;
template class SourceOf<picsom::DataBase>;
template class SourceOf<FigFile::FigAttr>;

template class ListOf<ListOf<MatrixOf<float> > >;

#else // __MINGW32__

#include <Source.C>
#include <List.C>
#include <Vector.C>
#include <VectorSource.C>
#include <VectorSet.C>
#include <Matrix.C>
#include <Point.C>
#include <Classifier.C>
#include <GnuPlot.C>

#include <SOM.h>
#include <FigFile.h>
#include <Query.h>

namespace simple {
template class VectorOf<double>;
template class VectorOf<float>;
template class VectorSourceOf<float>;
template class SourceOf<VectorOf<float> >;
template class VectorSetOf<float>;
template class MatrixOf<float>;

template class VectorOf<int>;
template class VectorSourceOf<int>;
template class SourceOf<VectorOf<int> >;
template class VectorSetOf<int>;
template class MatrixOf<int>;

template class ListOf<VectorOf<float> >;
template class ListOf<VectorOf<int> >;

template class VectorOf<PointOf<float> >;

template class ListOf<SOM::Vote>;

template class ListOf<FigFile::FigAttr>;

template class ListOf<MatrixOf<float> >;
template class ListOf<ListOf<MatrixOf<float> > >;

template class SourceOf<VectorSetOf<float> >;
template class SourceOf<ListOf<MatrixOf<float> > >;

template class VectorOf<PointOf<int> >;
template class SourceOf<VectorOf<StatQuad> >;
template class SourceOf<VectorSetOf<int> >;
template class SourceOf<MatrixOf<int> >;

template double VectorOf<float>::ChiSquareTest(VectorOf<float> const&, std::basic_ostream<char, std::char_traits<char> >&, char const*) const;

template class VectorOf<StatQuad>;
template class PointOf<int>;
  template class VectorOf<StatVar>;

}

#endif // __MINGW32__

#endif // __GNUC__

#endif // !HAVE_CONFIG_H

//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#ifdef __GNUC__
#include <Source.C>
#include <List.C>
#include <Vector.C>
#include <VectorSource.C>
#include <VectorSet.C>
#include <Matrix.C>
#include <Point.C>
#include <Classifier.C>
#include <GnuPlot.C>

#include <SOM.h>
#include <FigFile.h>
#include <Query.h>

namespace simple {

template class VectorOf<double>;

template class VectorOf<float>;
template class VectorSourceOf<float>;
template class SourceOf<VectorOf<float> >;
template class VectorSetOf<float>;
template class MatrixOf<float>;

template class VectorOf<int>;
template class VectorSourceOf<int>;
template class SourceOf<VectorOf<int> >;
template class VectorSetOf<int>;
template class MatrixOf<int>;

template class VectorOf<StatVar>;
template class VectorOf<StatQuad>;

template class ListOf<VectorOf<float> >;
template class ListOf<VectorOf<int> >;

template class ListOf<VectorSetOf<int> >;

template class VectorOf<PointOf<float> >;

template class ListOf<SOM::Vote>;

template class ListOf<FigFile::FigAttr>;

template class ListOf<MatrixOf<float> >;
template class ListOf<ListOf<MatrixOf<float> > >;

template class SourceOf<VectorSetOf<float> >;
template class SourceOf<ListOf<MatrixOf<float> > >;

template class VectorOf<PointOf<int> >;
template class SourceOf<VectorOf<StatQuad> >;
template class SourceOf<VectorSetOf<int> >;
template class SourceOf<MatrixOf<int> >;

template double VectorOf<float>::ChiSquareTest(VectorOf<float> const&, std::basic_ostream<char, std::char_traits<char> >&, char const*) const;template class PointOf<int>;

} // namespace simple

#endif // __GNUC__
#endif // HAVE_CONFIG_H

//-----------------------------------------------------------------------------

#ifdef _MSC_VER

#include <DataBase.h>
#include <Query.h>

#include <Source.C>
#include <List.C>
#include <Vector.C>
#include <VectorSource.C>
#include <VectorSet.C>
#include <Matrix.C>
#include <Point.C>

#include <GnuPlot.h>
#include <FigFile.h>
#include <Classifier.h>

template class VectorOf<int>;
template class VectorOf<float>;
template class VectorOf<double>;
template class VectorOf<StatQuad>;

template SourceOf<VectorOf<float> >;

template class VectorSourceOf<float>;
template class VectorSourceOf<StatQuad>;

template class VectorSetOf<int>;
template class VectorSetOf<float>;
template class VectorSetOf<StatQuad>;

template class MatrixOf<int>;
template class MatrixOf<float>;

template class PointOf<int>;
template class PointOf<float>;

template class VectorOf<PointOf<int> >;
template class VectorOf<PointOf<float> >;

template class ListOf<VectorOf<int> >;
template class ListOf<VectorOf<float> >;
template class ListOf<VectorOf<StatQuad> >;

template ListOf<class MatrixOf<int> >;
template ListOf<class MatrixOf<float> >;

template ListOf<ListOf<MatrixOf<float> > >;

template class ListOf<FigFile::FigAttr>;
template class ListOf<GnuPlot>;
template class ListOf<Classifier>;

template ListOf<SOM::Vote>;

template class ListOf<picsom::DataBase>;
template class ListOf<picsom::Query>;

#endif // _MSC_VER

//-----------------------------------------------------------------------------

#ifdef __alpha

#include <Query.h>

#include <Source.C>
#include <List.C>
#include <Vector.C>
#include <VectorSource.C>
#include <VectorSet.C>
#include <Matrix.C>
#include <SOM.C>
#include <FigFile.h>

#pragma instantiate ListOf<MatrixOf<int> >

#pragma instantiate VectorOf<int>
#pragma instantiate VectorOf<float>
#pragma instantiate VectorOf<double>

#pragma instantiate VectorOf<PointOf<int> >
#pragma instantiate VectorOf<PointOf<float> >
#pragma instantiate VectorOf<StatVar>
#pragma instantiate VectorOf<StatQuad>

#pragma instantiate MatrixOf<int>
#pragma instantiate MatrixOf<float>

#pragma instantiate VectorSetOf<int>
#pragma instantiate VectorSetOf<float>

#pragma instantiate ListOf<VectorOf<float> >
#pragma instantiate ListOf<VectorOf<PointOf<int> > >
#pragma instantiate ListOf<VectorOf<PointOf<float> > >

#pragma instantiate ListOf<VectorSetOf<float> >

#pragma instantiate SourceOf<VectorOf<float> >

#pragma instantiate VectorSourceOf<float>
#pragma instantiate VectorSourceOf<StatQuad>

#pragma instantiate ListOf<SOM::Vote>

#pragma instantiate ListOf<MatrixOf<float> >
#pragma instantiate ListOf<ListOf<MatrixOf<float> > >

#endif // __alpha

//-----------------------------------------------------------------------------

#ifdef sgi

#include <Query.h>

#include <FigFile.h>
#include <Source.C>
#include <List.C>
#include <Matrix.C>

#pragma can_instantiate void ListOf<MatrixOf<int> >::Append(MatrixOf<int>*,int)

#pragma can_instantiate void ListOf<MatrixOf<float> >::Append(MatrixOf<float>*,int)
#endif // sgi

//-----------------------------------------------------------------------------

