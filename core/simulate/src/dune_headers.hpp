//  - include dune headers ignoring diagnostic warnings

#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#if (__GNUC__ > 6)
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif
#if (__GNUC__ > 8)
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif
#endif

// Qt defines emit keyword which interferes with a tbb emit() function
#undef emit

#include <dune-copasi-config.hh>

#include <dune/copasi/common/stepper.hh>
#include <dune/copasi/grid/make_multi_domain_grid.hh>
#include <dune/copasi/model/factory.hh>
#include <dune/copasi/model/local_equations/functor_factory_parser.hh>
#include <dune/copasi/model/model.hh>
#include <dune/copasi/parser/context.hh>
#include <dune/copasi/parser/factory.hh>

#include <dune/pdelab/common/trace.hh>

#include <dune/grid/multidomaingrid/mdgridtraits.hh>
#include <dune/grid/multidomaingrid/multidomaingrid.hh>

#include <dune/grid/uggrid.hh>
#include <dune/grid/yaspgrid.hh>

#include <dune/common/exceptions.hh>
#include <dune/common/float_cmp.hh>
#include <dune/common/fvector.hh>
#include <dune/common/indices.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/parametertree.hh>
#include <dune/common/parametertreeparser.hh>

#define emit // restore the Qt empty definition of "emit"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
