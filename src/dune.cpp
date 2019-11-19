#include "dune.hpp"

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

#include <config.h>

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/parametertree.hh>
#include <dune/common/parametertreeparser.hh>
#include <dune/copasi/enum.hh>
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

#include <QFile>
#include <QImage>
#include <QPainter>
#include <algorithm>

#include "logger.hpp"
#include "mesh.hpp"
#include "pde.hpp"
#include "sbml.hpp"
#include "symbolic.hpp"
#include "tiff.hpp"
#include "utils.hpp"

constexpr int DuneDimensions = 2;
constexpr int DuneFEMOrder = 1;

using HostGrid = Dune::UGGrid<DuneDimensions>;
using MDGTraits = Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 1>;
using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
using ModelTraits =
    Dune::Copasi::ModelMultiDomainDiffusionReactionTraits<Grid, DuneFEMOrder>;
using Model = Dune::Copasi::ModelMultiDomainDiffusionReaction<ModelTraits>;

static std::vector<std::string> makeValidDuneSpeciesNames(
    const std::vector<std::string> &names) {
  std::vector<std::string> duneNames = names;
  // muparser reserved words, taken from:
  // https://beltoforion.de/article.php?a=muparser&p=features
  std::vector<std::string> reservedNames{
      {"sin",  "cos",  "tan",   "asin",  "acos",  "atan", "sinh",
       "cosh", "tanh", "asinh", "acosh", "atanh", "log2", "log10",
       "log",  "ln",   "exp",   "sqrt",  "sign",  "rint", "abs",
       "min",  "max",  "sum",   "avg"}};
  // dune-copasi reserved words:
  reservedNames.insert(reservedNames.end(), {"x", "y", "t", "pi", "dim"});
  for (auto &name : duneNames) {
    SPDLOG_TRACE("name {}", name);
    std::string duneName = name;
    name = "";
    // if species name clashes with a reserved name, append an underscore
    if (std::find(reservedNames.cbegin(), reservedNames.cend(), duneName) !=
        reservedNames.cend()) {
      duneName.append("_");
    }
    // if species name clashes with another species name,
    // append another underscore
    while (std::find(duneNames.cbegin(), duneNames.cend(), duneName) !=
           duneNames.cend()) {
      duneName.append("_");
    }
    name = duneName;
    SPDLOG_TRACE("  -> {}", name);
  }
  return duneNames;
}

namespace dune {

const QString &IniFile::getText() const { return text; }

void IniFile::addSection(const QString &str) {
  if (!text.isEmpty()) {
    text.append("\n");
  }
  text.append(QString("[%1]\n").arg(str));
}

void IniFile::addSection(const QString &str1, const QString &str2) {
  addSection(QString("%1.%2").arg(str1, str2));
}

void IniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3) {
  addSection(QString("%1.%2.%3").arg(str1, str2, str3));
}

void IniFile::addValue(const QString &var, const QString &value) {
  text.append(QString("%1 = %2\n").arg(var, value));
}

void IniFile::addValue(const QString &var, int value) {
  addValue(var, QString::number(value));
}

void IniFile::addValue(const QString &var, double value, int precision) {
  addValue(var, QString::number(value, 'g', precision));
}

void IniFile::clear() { text.clear(); }

static bool compartmentContainsNonConstantSpecies(
    const sbml::SbmlDocWrapper &doc, const QString &compID) {
  const auto &specs = doc.species.at(compID);
  return std::any_of(specs.cbegin(), specs.cend(), [doc](const auto &s) {
    return !doc.getIsSpeciesConstant(s.toStdString());
  });
}

DuneConverter::DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc, double dt,
                             int doublePrecision)
    : doc(SbmlDoc) {
  double begin_time = 0.0;
  double end_time = 0.02;
  double time_step = dt;

  mapDuneNameToColour.clear();
  mapSpeciesIdsToDuneNames.clear();

  // grid
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");
  ini.addValue("initial_level", 0);

  // simulation
  ini.addSection("model");
  ini.addValue("begin_time", begin_time, doublePrecision);
  ini.addValue("end_time", end_time, doublePrecision);
  ini.addValue("time_step", time_step, doublePrecision);
  ini.addValue("order", 1);

  // list of compartments with corresponding gmsh surface index - 1
  ini.addSection("model.compartments");
  gmshCompIndices.clear();
  int gmshCompIndex = 1;
  int duneCompIndex = 0;
  for (const auto &comp : doc.compartments) {
    SPDLOG_TRACE("compartment {}", comp.toStdString());
    // skip compartments which contain no non-constant species
    if (compartmentContainsNonConstantSpecies(doc, comp)) {
      ini.addValue(comp, duneCompIndex);
      SPDLOG_TRACE("  -> added with index {}", duneCompIndex);
      gmshCompIndices.insert(gmshCompIndex);
      ++duneCompIndex;
    }
    ++gmshCompIndex;
  }
  for (const auto &mem : doc.membraneVec) {
    QString compA = mem.compA->compartmentID.c_str();
    QString compB = mem.compB->compartmentID.c_str();
    // skip membranes which contain no non-constant species
    if (compartmentContainsNonConstantSpecies(doc, compA) ||
        compartmentContainsNonConstantSpecies(doc, compB)) {
      ini.addValue(mem.membraneID.c_str(), duneCompIndex);
      SPDLOG_TRACE("membrane {} added with index {}", mem.membraneID,
                   duneCompIndex);
      gmshCompIndices.insert(gmshCompIndex);
      ++duneCompIndex;
    }
    ++gmshCompIndex;
  }

  // for each compartment
  for (const auto &compartmentID : doc.compartments) {
    SPDLOG_TRACE("compartment {}", compartmentID.toStdString());
    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    for (const auto &s : doc.species.at(compartmentID)) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }
    if (!nonConstantSpecies.empty()) {
      ini.addSection("model", compartmentID);
      ini.addValue("begin_time", begin_time, doublePrecision);
      ini.addValue("end_time", end_time, doublePrecision);
      ini.addValue("time_step", time_step, doublePrecision);

      auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
      for (std::size_t is = 0; is < duneSpeciesNames.size(); ++is) {
        SPDLOG_TRACE("  - species {}", duneSpeciesNames.at(is));
        QColor col = doc.getSpeciesColour(nonConstantSpecies.at(is).c_str());
        mapDuneNameToColour[duneSpeciesNames.at(is)] = col;
        mapSpeciesIdsToDuneNames[nonConstantSpecies.at(is)] =
            duneSpeciesNames.at(is);
        SPDLOG_TRACE("     - colour {:x}", col.rgba());
      }

      // operator splitting indexing: all set to zero for now...
      ini.addSection("model", compartmentID, "operator");
      for (const auto &speciesID : duneSpeciesNames) {
        ini.addValue(speciesID.c_str(), 0);
      }

      // initial concentrations
      std::vector<QString> tiffs;
      ini.addSection("model", compartmentID, "initial");
      for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
        QString name = nonConstantSpecies.at(i).c_str();
        QString duneName = duneSpeciesNames.at(i).c_str();
        double initConc = doc.getInitialConcentration(name);
        QString expr = doc.getAnalyticConcentration(name);
        QString sampledField =
            doc.getSpeciesSampledFieldInitialAssignment(name.toStdString())
                .c_str();
        if (!sampledField.isEmpty()) {
          // if there is a sampledField then make a TIFF
          auto sampledFieldFile = QString("%1.tif").arg(sampledField);
          double max = utils::writeTIFF(sampledFieldFile.toStdString(),
                                        doc.mapSpeciesIdToField.at(name),
                                        doc.getPixelWidth());
          tiffs.push_back(sampledField);
          ini.addValue(duneName,
                       QString("%2*%3(x,y)").arg(max).arg(sampledField));
        } else if (!expr.isEmpty()) {
          // otherwise, initialAssignments take precedence:
          std::string e = doc.inlineExpr(expr.toStdString());
          symbolic::Symbolic sym(e, {"x", "y"}, doc.getGlobalConstants());
          ini.addValue(duneName, sym.simplify().c_str());
        } else {
          // otherwise just use initialConcentration value
          ini.addValue(duneName, initConc, doublePrecision);
        }
      }
      if (!tiffs.empty()) {
        ini.addSection("model", "data");
        for (const auto &tiff : tiffs) {
          ini.addValue(tiff, tiff + QString(".tif"));
        }
      }

      // reactions
      ini.addSection("model", compartmentID, "reaction");
      std::size_t nSpecies =
          static_cast<std::size_t>(nonConstantSpecies.size());

      std::vector<std::string> reacs;
      if (doc.reactions.find(compartmentID) != doc.reactions.cend()) {
        reacs = utils::toStdString(doc.reactions.at(compartmentID));
      }
      pde::PDE pde(&doc, nonConstantSpecies, reacs, duneSpeciesNames);
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     pde.getRHS().at(i).c_str());
      }

      // reaction term jacobian
      ini.addSection("model", compartmentID, "reaction.jacobian");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        for (std::size_t j = 0; j < nSpecies; ++j) {
          QString lhs = QString("d%1__d%2")
                            .arg(duneSpeciesNames.at(i).c_str(),
                                 duneSpeciesNames.at(j).c_str());
          QString rhs = pde.getJacobian().at(i).at(j).c_str();
          ini.addValue(lhs, rhs);
        }
      }

      // diffusion coefficients
      ini.addSection("model", compartmentID, "diffusion");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     doc.getDiffusionConstant(nonConstantSpecies.at(i).c_str()),
                     doublePrecision);
      }

      // output file
      ini.addSection("model", compartmentID, "writer");
      ini.addValue("file_name", compartmentID);
    }
  }

  // for each membrane do the same
  for (const auto &membrane : doc.membraneVec) {
    QString membraneID = membrane.membraneID.c_str();
    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    QString compA = membrane.compA->compartmentID.c_str();
    QString compB = membrane.compB->compartmentID.c_str();
    for (const auto &comp : {compA, compB}) {
      for (const auto &s : doc.species.at(comp)) {
        if (!doc.getIsSpeciesConstant(s.toStdString())) {
          nonConstantSpecies.push_back(s.toStdString());
        }
      }
    }
    if (!nonConstantSpecies.empty()) {
      ini.addSection("model", membraneID);
      ini.addValue("begin_time", begin_time, doublePrecision);
      ini.addValue("end_time", end_time, doublePrecision);
      ini.addValue("time_step", time_step, doublePrecision);

      auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
      for (std::size_t is = 0; is < duneSpeciesNames.size(); ++is) {
        mapDuneNameToColour[duneSpeciesNames.at(is)] =
            doc.getSpeciesColour(nonConstantSpecies.at(is).c_str());
        mapSpeciesIdsToDuneNames[nonConstantSpecies.at(is)] =
            duneSpeciesNames.at(is);
      }

      // operator splitting indexing: all set to zero for now...
      ini.addSection("model", membraneID, "operator");
      for (const auto &speciesID : duneSpeciesNames) {
        ini.addValue(speciesID.c_str(), 0);
      }

      // initial concentrations
      ini.addSection("model", membraneID, "initial");
      for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
        ini.addValue(
            duneSpeciesNames.at(i).c_str(),
            doc.getInitialConcentration(nonConstantSpecies.at(i).c_str()),
            doublePrecision);
      }

      // reactions: want reactions for both neighbouring compartments
      // as well as membrane reactions (that involve species from both
      // compartments in the same reaction)
      ini.addSection("model", membraneID, "reaction");
      std::size_t nSpecies =
          static_cast<std::size_t>(nonConstantSpecies.size());

      std::vector<std::string> reacs;
      std::vector<std::string> reacScaleFactors;
      for (const auto &comp : {compA, compB}) {
        if (doc.reactions.find(comp) != doc.reactions.cend()) {
          for (const auto &r : doc.reactions.at(comp)) {
            reacs.push_back(r.toStdString());
            reacScaleFactors.push_back("1");
          }
        }
      }
      // divide membrane reaction rates by width of membrane
      if (doc.reactions.find(membraneID) != doc.reactions.cend()) {
        const auto &lengthUnit = SbmlDoc.getModelUnits().getLength();
        const auto &volumeUnit = SbmlDoc.getModelUnits().getVolume();
        for (const auto &r : doc.reactions.at(membraneID)) {
          double lengthCubedToVolFactor =
              units::pixelWidthToVolume(1.0, lengthUnit, volumeUnit);
          double width = doc.mesh->getMembraneWidth(membraneID.toStdString());
          double scaleFactor = width * lengthCubedToVolFactor;
          SPDLOG_INFO("  - membrane width = {} {}", width,
                      lengthUnit.symbol.toStdString());
          SPDLOG_INFO("  - [length]^3/[vol] = {}", lengthCubedToVolFactor);
          SPDLOG_INFO("  - dividing flux by {}", scaleFactor);
          reacScaleFactors.push_back(
              QString::number(scaleFactor, 'g', 17).toStdString());
          reacs.push_back(r.toStdString());
        }
      }
      pde::PDE pde(&doc, nonConstantSpecies, reacs, duneSpeciesNames,
                   reacScaleFactors);
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     pde.getRHS().at(i).c_str());
      }

      // reaction term jacobian
      ini.addSection("model", membraneID, "reaction.jacobian");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        for (std::size_t j = 0; j < nSpecies; ++j) {
          QString lhs = QString("d%1__d%2")
                            .arg(duneSpeciesNames.at(i).c_str(),
                                 duneSpeciesNames.at(j).c_str());
          QString rhs = pde.getJacobian().at(i).at(j).c_str();
          ini.addValue(lhs, rhs);
        }
      }

      // diffusion coefficients
      ini.addSection("model", membraneID, "diffusion");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     doc.getDiffusionConstant(nonConstantSpecies.at(i).c_str()),
                     doublePrecision);
      }

      // output file
      ini.addSection("model", membraneID, "writer");
      ini.addValue("file_name", membraneID);
    }
  }

  // logger settings
  ini.addSection("logging");
  if (SPDLOG_ACTIVE_LEVEL >= 2) {
    // for release builds disable DUNE logging
    ini.addValue("default.level", "off");
  } else {
    // for debug builds enable verbose DUNE logging
    ini.addValue("default.level", "trace");
    ini.addSection("logging.backend.model");
    ini.addValue("level", "trace");
    ini.addValue("indent", 2);

    ini.addSection("logging.backend.solver");
    ini.addValue("level", "trace");
    ini.addValue("indent", 4);
  }
}

QString DuneConverter::getIniFile() const { return ini.getText(); }

QColor DuneConverter::getSpeciesColour(const std::string &duneName) const {
  return mapDuneNameToColour.at(duneName);
}

const std::unordered_set<int> &DuneConverter::getGMSHCompIndices() const {
  return gmshCompIndices;
}

class DuneSimulation::DuneImpl {
 public:
  Dune::ParameterTree config;
  std::shared_ptr<Grid> grid_ptr;
  std::shared_ptr<HostGrid> host_grid_ptr;
  std::unique_ptr<Model> model;
  void init(const std::string &iniFile);
};

void DuneSimulation::DuneImpl::init(const std::string &iniFile) {
  std::stringstream ssIni(iniFile);
  Dune::ParameterTreeParser::readINITree(ssIni, config);

  // init Dune logging if not already done
  if (!Dune::Logging::Logging::initialized()) {
    Dune::Logging::Logging::init(
        Dune::FakeMPIHelper::getCollectiveCommunication(),
        config.sub("logging"));
    if (SPDLOG_ACTIVE_LEVEL >= 2) {
      // for release builds disable DUNE logging
      Dune::Logging::Logging::mute();
    }
  }
  // NB: msh file needs to be file for gmshreader
  // todo: generate DUNE mesh directly
  std::tie(grid_ptr, host_grid_ptr) =
      Dune::Copasi::GmshReader<Grid>::read("grid.msh", config);

  // initialize model
  model = std::make_unique<Model>(grid_ptr, config.sub("model"));
}

void DuneSimulation::initDuneModel(const sbml::SbmlDocWrapper &sbmlDoc,
                                   double dt) {
  geometrySize = sbmlDoc.getCompartmentImage().size();
  geometrySize *= sbmlDoc.getPixelWidth();
  dune::DuneConverter dc(sbmlDoc, dt);

  // export gmsh file `grid.msh` in the same dir
  QFile f2("grid.msh");
  if (f2.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f2.write(sbmlDoc.mesh->getGMSH(dc.getGMSHCompIndices()).toUtf8());
    f2.close();
  } else {
    SPDLOG_ERROR("Cannot write to file grid.msh");
  }

  pDuneImpl->init(dc.getIniFile().toStdString());

  initCompartmentNames();
  initSpeciesNames(dc);
}

void DuneSimulation::initCompartmentNames() {
  compartmentNames = pDuneImpl->config.sub("model.compartments").getValueKeys();
  for (std::size_t i = 0; i < compartmentNames.size(); ++i) {
    const auto &name = compartmentNames.at(i);
    std::size_t iDune =
        pDuneImpl->config.sub("model.compartments").get<std::size_t>(name);
    SPDLOG_DEBUG("compartment[{}]: {} - Dune index {}", i, name, iDune);
    if (i != iDune) {
      SPDLOG_WARN("Mismatch between Dune compartment order and index:");
      SPDLOG_WARN("compartment[{}]: {} - Dune index {}", i, name, iDune);
    }
  }
}

void DuneSimulation::initSpeciesNames(const DuneConverter &dc) {
  speciesNames.clear();
  mapSpeciesIdsToDuneNames = dc.mapSpeciesIdsToDuneNames;
  speciesColours.clear();
  for (const auto &compName : compartmentNames) {
    SPDLOG_DEBUG("compartment: {}", compName);
    // NB: species index is position in *sorted* list of species names
    // so make copy of list of names from ini file and sort it
    auto names =
        pDuneImpl->config.sub("model." + compName + ".initial").getValueKeys();
    std::sort(names.begin(), names.end());
    speciesNames.push_back(std::move(names));
    auto &speciesColoursCompartment = speciesColours.emplace_back();
    for (const auto &name : speciesNames.back()) {
      const QColor &c = dc.getSpeciesColour(name);
      speciesColoursCompartment.push_back(c);
      SPDLOG_DEBUG("  - species: {} --> colour {:x}", name, c.rgba());
    }
  }
}

static std::pair<QPoint, QPoint> getBoundingBox(const QTriangleF &t,
                                                const QPointF &scale) {
  // get triangle bounding box in physical units
  QPointF fmin(t[0].x(), t[0].y());
  QPointF fmax = fmin;
  for (std::size_t i = 1; i < 3; ++i) {
    fmin.setX(std::min(fmin.x(), t[i].x()));
    fmax.setX(std::max(fmax.x(), t[i].x()));
    fmin.setY(std::min(fmin.y(), t[i].y()));
    fmax.setY(std::max(fmax.y(), t[i].y()));
  }
  // convert physical points to pixel locations
  return std::make_pair<QPoint, QPoint>(
      QPoint(static_cast<int>(fmin.x() * scale.x()),
             static_cast<int>(fmin.y() * scale.y())),
      QPoint(static_cast<int>(fmax.x() * scale.x()),
             static_cast<int>(fmax.y() * scale.y())));
}

void DuneSimulation::updatePixels() {
  pixels.clear();
  for (std::size_t compIndex = 0; compIndex < compartmentNames.size();
       ++compIndex) {
    auto &pixelsComp = pixels.emplace_back();
    const auto &gridview =
        pDuneImpl->grid_ptr->subDomain(static_cast<int>(compIndex))
            .leafGridView();
    SPDLOG_TRACE("compartment[{}]: {}", compIndex,
                 compartmentNames.at(compIndex));
    // get vertices of triangles:
    for (const auto e : elements(gridview)) {
      auto &pixelsTriangle = pixelsComp.emplace_back();
      const auto &geo = e.geometry();
      assert(geo.type().isTriangle());
      auto ref = Dune::referenceElement(geo);
      QPointF c0(geo.corner(0)[0], geo.corner(0)[1]);
      QPointF c1(geo.corner(1)[0], geo.corner(1)[1]);
      QPointF c2(geo.corner(2)[0], geo.corner(2)[1]);
      auto [pMin, pMax] = getBoundingBox({{c0, c1, c2}}, scaleFactor);
      SPDLOG_TRACE("  - bounding box ({},{}) - ({},{})", pMin.x(), pMin.y(),
                   pMax.x(), pMax.y());
      for (int x = pMin.x(); x < pMax.x() + 1; ++x) {
        for (int y = pMin.y(); y < pMax.y() + 1; ++y) {
          auto localPoint =
              e.geometry().local({static_cast<double>(x) / scaleFactor.x(),
                                  static_cast<double>(y) / scaleFactor.y()});
          if (ref.checkInside(localPoint)) {
            pixelsTriangle.push_back(
                {QPoint(x, y), {localPoint[0], localPoint[1]}});
          }
        }
      }
      SPDLOG_TRACE("    - found {} pixels", pixelsTriangle.size());
    }
  }
}

void DuneSimulation::updateTriangles() {
  triangles.clear();
  for (std::size_t compIndex = 0; compIndex < compartmentNames.size();
       ++compIndex) {
    triangles.emplace_back();
    const auto &gridview =
        pDuneImpl->grid_ptr->subDomain(static_cast<int>(compIndex))
            .leafGridView();
    SPDLOG_TRACE("compartment[{}]: {}", compIndex,
                 compartmentNames.at(compIndex));
    // get vertices of triangles:
    for (const auto e : elements(gridview)) {
      const auto &geo = e.geometry();
      assert(geo.type().isTriangle());
      QPointF c0(geo.corner(0)[0], geo.corner(0)[1]);
      QPointF c1(geo.corner(1)[0], geo.corner(1)[1]);
      QPointF c2(geo.corner(2)[0], geo.corner(2)[1]);
      triangles.back().push_back({{c0, c1, c2}});
    }
    SPDLOG_TRACE("  - found {} triangles", triangles.back().size());
  }
}

void DuneSimulation::updateBarycentricWeights() {
  SPDLOG_TRACE("geometry size: {}x{}", geometrySize.width(),
               geometrySize.height());
  SPDLOG_TRACE("image size: {}x{}", imageSize.width(), imageSize.height());
  SPDLOG_TRACE("scaleFactor: {} x {} (where pixel = scaleFactor*physical): {}",
               scaleFactor.x(), scaleFactor.y());
  weights.clear();
  for (const auto &comp : triangles) {
    SPDLOG_TRACE("compartment with {} triangles:", comp.size());
    weights.emplace_back();
    for (const auto &t : comp) {
      auto &triangleWeights = weights.back().emplace_back();
      auto [pMin, pMax] = getBoundingBox(t, scaleFactor);
      SPDLOG_TRACE("  - bounding box ({},{}) - ({},{})", pMin.x(), pMin.y(),
                   pMax.x(), pMax.y());
      // get weights for each point
      double denom = (t[1].y() - t[2].y()) * (t[0].x() - t[2].x()) +
                     (t[2].x() - t[1].x()) * (t[0].y() - t[2].y());
      QPointF w1((t[1].y() - t[2].y()) / denom, (t[2].x() - t[1].x()) / denom);
      QPointF w2((t[2].y() - t[0].y()) / denom, (t[0].x() - t[2].x()) / denom);
      for (int x = pMin.x(); x < pMax.x() + 1; ++x) {
        for (int y = pMin.y(); y < pMax.y() + 1; ++y) {
          double W1 =
              w1.x() * (static_cast<double>(x) / scaleFactor.x() - t[2].x()) +
              w1.y() * (static_cast<double>(y) / scaleFactor.y() - t[2].y());
          double W2 =
              w2.x() * (static_cast<double>(x) / scaleFactor.x() - t[2].x()) +
              w2.y() * (static_cast<double>(y) / scaleFactor.y() - t[2].y());
          double W3 = 1.0 - W1 - W2;
          if (W1 >= 0 && W2 >= 0 && W3 >= 0) {
            // if all weights positive: add point
            triangleWeights.push_back({QPoint(x, y), {W1, W2, W3}});
          }
        }
      }
      SPDLOG_TRACE("  - triangle with {} pixels", triangleWeights.size());
    }
  }
}

void DuneSimulation::updateSpeciesConcentrations() {
  concentrations.clear();
  maxConcs.clear();
  for (std::size_t compIndex = 0; compIndex < compartmentNames.size();
       ++compIndex) {
    maxConcs.emplace_back();
    SPDLOG_TRACE("compartment[{}]: {}", compIndex,
                 compartmentNames.at(compIndex));
    const auto &gridview =
        pDuneImpl->grid_ptr->subDomain(static_cast<int>(compIndex))
            .leafGridView();
    const auto &species = speciesNames.at(compIndex);
    const auto &compTriangles = triangles.at(compIndex);
    auto &comp = concentrations.emplace_back();
    for (std::size_t iSpecies = 0; iSpecies < species.size(); ++iSpecies) {
      auto &spec = comp.emplace_back();
      auto gf = pDuneImpl->model->get_grid_function(pDuneImpl->model->states(),
                                                    compIndex, iSpecies);
      using GF = decltype(gf);
      using Range = typename GF::Traits::RangeType;
      using Domain = typename GF::Traits::DomainType;
      Range result;
      spec.reserve(compTriangles.size());
      double avC = 0;
      double maxC = 0;
      for (const auto e : elements(gridview)) {
        auto &corners = spec.emplace_back();
        corners.reserve(3);
        for (const auto &dom : {Domain{0, 0}, Domain{1, 0}, Domain{0, 1}}) {
          gf.evaluate(e, dom, result);
          corners.push_back(result[0]);
          avC += corners.back();
          maxC = std::max(maxC, corners.back());
        }
      }
      avC /= static_cast<double>(spec.size() * 3);
      mapSpeciesIDToAvConc[species.at(iSpecies)] = avC;
      maxConcs.back().push_back(maxC);
      SPDLOG_TRACE("  - species[{}]: {}", iSpecies, species.at(iSpecies));
      SPDLOG_TRACE("    - avg = {}", avC);
      SPDLOG_TRACE("    - max = {}", maxC);
    }
  }
}

DuneSimulation::DuneSimulation(const sbml::SbmlDocWrapper &sbmlDoc, double dt,
                               QSize imgSize)
    : pDuneImpl(std::make_shared<DuneImpl>()) {
  initDuneModel(sbmlDoc, dt);
  setImageSize(imgSize);
  updateSpeciesConcentrations();
}

void DuneSimulation::doTimestep(double dt) {
  double endTime = pDuneImpl->model->current_time() + dt;
  while (pDuneImpl->model->current_time() < endTime) {
    pDuneImpl->model->step();
  }
  updateSpeciesConcentrations();
}

QRgb DuneSimulation::pixelColour(std::size_t iComp,
                                 const std::vector<double> &concs) const {
  double alpha = 1.0 / static_cast<double>(concs.size());
  int r = 0;
  int g = 0;
  int b = 0;
  for (std::size_t iSpecies = 0; iSpecies < concs.size(); ++iSpecies) {
    const QColor &col = speciesColours.at(iComp).at(iSpecies);
    double c = concs[iSpecies] * alpha;
    c /= maxConcs[iComp][iSpecies];
    r += static_cast<int>(col.red() * c);
    g += static_cast<int>(col.green() * c);
    b += static_cast<int>(col.blue() * c);
  }
  return qRgb(r, g, b);
}

QImage DuneSimulation::getConcImage(bool linearInterpolationOnly) const {
  QImage img(imageSize, QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  using GF = decltype(
      pDuneImpl->model->get_grid_function(pDuneImpl->model->states(), 0, 0));

  for (std::size_t iComp = 0; iComp < compartmentNames.size(); ++iComp) {
    std::size_t nSpecies = speciesNames.at(iComp).size();
    // get grid function for each species in this compartment
    std::vector<GF> gridFunctions;
    gridFunctions.reserve(nSpecies);
    for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
      gridFunctions.push_back(pDuneImpl->model->get_grid_function(
          pDuneImpl->model->states(), iComp, iSpecies));
    }
    const auto &domainWeights = weights.at(iComp);
    const auto &gridview =
        pDuneImpl->grid_ptr->subDomain(static_cast<int>(iComp)).leafGridView();
    std::size_t iTriangle = 0;
    for (const auto e : elements(gridview)) {
      SPDLOG_TRACE("triangle {}", iTriangle);
      if (linearInterpolationOnly) {
        for (const auto &[point, w] : domainWeights[iTriangle]) {
          SPDLOG_TRACE("  - point ({},{})", point.x(), point.y());
          SPDLOG_TRACE("  - weights {}", w);
          std::vector<double> localConcs;
          localConcs.reserve(nSpecies);
          // interpolate linearly between corner values
          for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
            const auto &conc = concentrations[iComp][iSpecies][iTriangle];
            double c = w[0] * conc[0] + w[1] * conc[1] + w[2] * conc[2];
            // replace negative values with zero
            localConcs.push_back(c < 0 ? 0 : c);
          }
          img.setPixel(point, pixelColour(iComp, localConcs));
        }
      } else {
        for (const auto &pair : pixels[iComp][iTriangle]) {
          // evaluate DUNE grid function at this pixel location
          // convert pixel->global->local
          Dune::FieldVector<double, 2> localPoint = {pair.second[0],
                                                     pair.second[1]};
          SPDLOG_TRACE("  - pixel ({},{}) -> -> local ({},{})", pair.first.x(),
                       pair.first.y(), localPoint[0], localPoint[1]);
          GF::Traits::RangeType result;
          std::vector<double> localConcs;
          localConcs.reserve(nSpecies);
          for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
            gridFunctions[iSpecies].evaluate(e, localPoint, result);
            SPDLOG_TRACE("    - species[{}] = {}", iSpecies, result[0]);
            // replace negative values with zero
            localConcs.push_back(result[0] < 0 ? 0 : result[0]);
          }
          img.setPixel(pair.first, pixelColour(iComp, localConcs));
        }
      }
      ++iTriangle;
    }
  }
  // (0,0) pixel is bottom-left in the above, but (0,0) is top-left in QImage:
  return img.mirrored(false, true);
}

double DuneSimulation::getAverageConcentration(
    const std::string &speciesID) const {
  const auto &duneName = mapSpeciesIdsToDuneNames.at(speciesID);
  return mapSpeciesIDToAvConc.at(duneName);
}

void DuneSimulation::setImageSize(const QSize &imgSize) {
  // maintain aspect-ratio
  double scale = std::min(imgSize.width() / geometrySize.width(),
                          imgSize.height() / geometrySize.height());
  imageSize.setWidth(static_cast<int>(scale * geometrySize.width()));
  imageSize.setHeight(static_cast<int>(scale * geometrySize.height()));
  scaleFactor.setX(scale);
  scaleFactor.setY(scale);
  updatePixels();
  updateTriangles();
  updateBarycentricWeights();
}

}  // namespace dune
