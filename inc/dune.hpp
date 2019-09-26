// DUNE solver interface
//  - DuneSimulation: wrapper to dune-copasi library
//  - DuneConverter: construct DUNE ini file
//  - iniFile class: simple ini file generation one line at a time

#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic push
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
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/parametertree.hh>
#include <dune/common/parametertreeparser.hh>
#include <dune/copasi/gmsh_reader.hh>
#include <dune/copasi/model_diffusion_reaction.cc>
#include <dune/copasi/model_diffusion_reaction.hh>
#include <dune/copasi/model_multidomain_diffusion_reaction.cc>
#include <dune/copasi/model_multidomain_diffusion_reaction.hh>
#include <dune/grid/io/file/gmshreader.hh>
#include <dune/grid/multidomaingrid.hh>
#include <dune/logging/logging.hh>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <QString>

#include "sbml.hpp"

namespace dune {

class iniFile {
 private:
  QString text;

 public:
  const QString &getText() const;
  void addSection(const QString &str);
  void addSection(const QString &str1, const QString &str2);
  void addSection(const QString &str1, const QString &str2,
                  const QString &str3);
  void addValue(const QString &var, const QString &value);
  void addValue(const QString &var, int value);
  void addValue(const QString &var, double value, int precision);
  void clear();
};

class DuneConverter {
 public:
  explicit DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc,
                         int doublePrecision = 15);
  QString getIniFile() const;

 private:
  const sbml::SbmlDocWrapper &doc;
  iniFile ini;
};

constexpr int dim = 2;
using HostGrid = Dune::UGGrid<dim>;
using MDGTraits = Dune::mdgrid::DynamicSubDomainCountTraits<dim, 1>;
using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;

using QTriangleF = std::array<QPointF, 3>;

class DuneSimulation {
 private:
  // Dune objects
  Dune::ParameterTree config;
  std::shared_ptr<Grid> grid_ptr;
  std::shared_ptr<HostGrid> host_grid_ptr;
  std::unique_ptr<Dune::Copasi::ModelMultiDomainDiffusionReaction<dune::Grid>>
      model;
  // index of compartment/species name in these vectors is the Dune index:
  std::vector<std::string> compartmentNames;
  std::vector<std::vector<std::string>> speciesNames;
  std::map<std::string, double> mapSpeciesIDToAvConc;
  // dimensions of model
  QSizeF geometrySize;
  // triangles that make up each compartment
  std::vector<std::vector<QTriangleF>> triangles;
  void initDuneModel(const sbml::SbmlDocWrapper &sbmlDoc);
  void updateCompartmentNames();
  void updateSpeciesNames();
  void updateTriangles();

 public:
  explicit DuneSimulation(const sbml::SbmlDocWrapper &sbmlDoc);
  void doTimestep(double dt);
  QImage getConcImage(const QSize &imageSize = QSize(800, 800));
  double getAverageConcentration(const std::string &speciesID) const;
};

}  // namespace dune
