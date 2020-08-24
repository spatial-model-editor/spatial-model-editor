#include "duneconverter.hpp"
#include "duneini.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "model_compartments.hpp"
#include "model_geometry.hpp"
#include "model_membranes.hpp"
#include "model_reactions.hpp"
#include "model_species.hpp"
#include "model_units.hpp"
#include "pde.hpp"
#include "tiff.hpp"
#include "utils.hpp"
#include <QDir>
#include <QFile>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <string>
#include <utility>

static std::vector<std::string>
makeValidDuneSpeciesNames(const std::vector<std::string> &names) {
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

static bool compartmentContainsNonConstantSpecies(const model::Model &model,
                                                  const QString &compID) {
  const auto &specs = model.getSpecies().getIds(compID);
  return std::any_of(specs.cbegin(), specs.cend(), [m = &model](const auto &s) {
    return !m->getSpecies().getIsConstant(s);
  });
}

template <typename T>
static std::vector<std::size_t>
getIndicesOfSortedVector(const std::vector<T> &unsorted) {
  std::vector<std::size_t> indices(unsorted.size(), 0);
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(),
            [&unsorted = unsorted](std::size_t i1, std::size_t i2) {
              return unsorted[i1] < unsorted[i2];
            });
  return indices;
}

namespace simulate {

DuneConverter::DuneConverter(const model::Model &model, bool forExternalUse,
                             double dt, const QString &iniFilename,
                             int doublePrecision)
    : mesh{model.getGeometry().getMesh()},
      x0{model.getGeometry().getPhysicalOrigin().x()},
      y0{model.getGeometry().getPhysicalOrigin().y()},
      a{model.getGeometry().getPixelWidth()},
      w{model.getGeometry().getImage().width()} {
  QString iniFileDir = QFileInfo(iniFilename).absolutePath();

  IniFile ini;
  double begin_time = 0.0;
  double end_time = 0.02;
  double time_step = dt;

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
  for (const auto &comp : model.getCompartments().getIds()) {
    SPDLOG_TRACE("compartment {}", comp.toStdString());
    // skip compartments which contain no non-constant species
    if (compartmentContainsNonConstantSpecies(model, comp)) {
      ini.addValue(comp, duneCompIndex);
      SPDLOG_TRACE("  -> added with index {}", duneCompIndex);
      gmshCompIndices.insert(gmshCompIndex);
      ++duneCompIndex;
    }
    ++gmshCompIndex;
  }
  for (const auto &mem : model.getMembranes().getMembranes()) {
    QString compA = mem.getCompartmentA()->getId().c_str();
    QString compB = mem.getCompartmentB()->getId().c_str();
    // only add membranes which contain non-constant species
    if (compartmentContainsNonConstantSpecies(model, compA) ||
        compartmentContainsNonConstantSpecies(model, compB)) {
      ini.addValue(mem.getId().c_str(), duneCompIndex);
      SPDLOG_TRACE("membrane {} added with index {}", mem.getId(),
                   duneCompIndex);
      gmshCompIndices.insert(gmshCompIndex);
      ++duneCompIndex;
      independentCompartments = false;
    }
    ++gmshCompIndex;
  }

  // for each compartment
  for (const auto &compartmentID : model.getCompartments().getIds()) {
    SPDLOG_TRACE("compartment {}", compartmentID.toStdString());
    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    for (const auto &s : model.getSpecies().getIds(compartmentID)) {
      if (!model.getSpecies().getIsConstant(s)) {
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
        SPDLOG_TRACE("  - species '{}' -> '{}'", nonConstantSpecies.at(is),
                     duneSpeciesNames.at(is));
      }

      // operator splitting indexing: all set to zero for now...
      ini.addSection("model", compartmentID, "operator");
      for (const auto &speciesID : duneSpeciesNames) {
        ini.addValue(speciesID.c_str(), 0);
      }

      // initial concentrations
      std::vector<std::vector<double>> concs(duneSpeciesNames.size(),
                                             std::vector<double>{});
      auto indices = getIndicesOfSortedVector(duneSpeciesNames);
      std::vector<QString> tiffs;
      ini.addSection("model", compartmentID, "initial");
      for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
        QString name = nonConstantSpecies.at(i).c_str();
        QString duneName = duneSpeciesNames.at(i).c_str();
        double initConc = model.getSpecies().getInitialConcentration(name);
        QString expr = model.getSpecies().getAnalyticConcentration(name);
        const auto *f = model.getSpecies().getField(name);
        if (forExternalUse) {
          if (!f->getIsUniformConcentration()) {
            // for external use: if there is a non-uniform initial condition
            // then make a TIFF & write to same dir as ini file
            auto sampledFieldName =
                QString("%1_initialConcentration").arg(duneName);
            auto sampledFieldFile = QString("%1.tif").arg(sampledFieldName);
            auto tiffFilename = QDir(iniFileDir).filePath(sampledFieldFile);
            SPDLOG_TRACE("Exporting tiff: '{}'", tiffFilename.toStdString());
            double max = utils::writeTIFF(
                tiffFilename.toStdString(),
                f->getCompartment()->getCompartmentImage().size(),
                f->getConcentration(), f->getCompartment()->getPixels(),
                model.getGeometry().getPixelWidth());
            tiffs.push_back(sampledFieldName);
            ini.addValue(duneName,
                         QString("%2*%3(x,y)").arg(max).arg(sampledFieldName));
          } else {
            // otherwise just use initialConcentration value
            ini.addValue(duneName, initConc, doublePrecision);
          }
        } else {
          ini.addValue(duneName, 0.0, doublePrecision);
          // create array of concentration values
          concs[indices[i]] = f->getConcentrationImageArray();
        }
      }
      concentrations.push_back(std::move(concs));
      if (forExternalUse && !tiffs.empty()) {
        ini.addSection("model", "data");
        for (const auto &tiff : tiffs) {
          ini.addValue(tiff, tiff + QString(".tif"));
        }
      }

      // reactions
      ini.addSection("model", compartmentID, "reaction");
      std::size_t nSpecies = nonConstantSpecies.size();

      std::vector<std::string> reacs;
      if (auto reacsInCompartment = model.getReactions().getIds(compartmentID);
          !reacsInCompartment.isEmpty()) {
        reacs = utils::toStdString(reacsInCompartment);
      }
      PDE pde(&model, nonConstantSpecies, reacs, duneSpeciesNames);
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
                     model.getSpecies().getDiffusionConstant(
                         nonConstantSpecies[i].c_str()),
                     doublePrecision);
      }

      // output file
      if (forExternalUse) {
        ini.addSection("model", compartmentID, "writer");
        ini.addValue("file_name", compartmentID);
      }
    }
  }

  // for each membrane do the same
  for (const auto &membrane : model.getMembranes().getMembranes()) {
    QString membraneID = membrane.getId().c_str();
    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    QString compA = membrane.getCompartmentA()->getId().c_str();
    QString compB = membrane.getCompartmentB()->getId().c_str();
    for (const auto &comp : {compA, compB}) {
      for (const auto &s : model.getSpecies().getIds(comp)) {
        if (!model.getSpecies().getIsConstant(s)) {
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

      // operator splitting indexing: all set to zero for now...
      ini.addSection("model", membraneID, "operator");
      for (const auto &speciesID : duneSpeciesNames) {
        ini.addValue(speciesID.c_str(), 0);
      }

      // initial concentrations
      std::vector<std::vector<double>> concs(duneSpeciesNames.size(),
                                             std::vector<double>{});
      auto indices = getIndicesOfSortedVector(duneSpeciesNames);
      ini.addSection("model", membraneID, "initial");
      for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
        QString name = nonConstantSpecies.at(i).c_str();
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     model.getSpecies().getInitialConcentration(name),
                     doublePrecision);
        if (!forExternalUse) {
          const auto *f = model.getSpecies().getField(name);
          concs[indices[i]] = f->getConcentrationImageArray();
        }
      }
      concentrations.push_back(std::move(concs));

      // reactions: want reactions for both neighbouring compartments
      // as well as membrane reactions (that involve species from both
      // compartments in the same reaction)
      ini.addSection("model", membraneID, "reaction");
      std::size_t nSpecies = nonConstantSpecies.size();

      std::vector<std::string> reacs;
      std::vector<std::string> reacScaleFactors;
      for (const auto &comp : {compA, compB}) {
        if (auto reacsInCompartment = model.getReactions().getIds(comp);
            !reacsInCompartment.isEmpty()) {
          for (const auto &r : reacsInCompartment) {
            reacs.push_back(r.toStdString());
            reacScaleFactors.emplace_back("1");
          }
        }
      }
      // divide membrane reaction rates by width of membrane
      if (auto reacsInCompartment = model.getReactions().getIds(membraneID);
          !reacsInCompartment.isEmpty()) {
        const auto &lengthUnit = model.getUnits().getLength();
        const auto &volumeUnit = model.getUnits().getVolume();
        for (const auto &r : reacsInCompartment) {
          double lengthCubedToVolFactor =
              model::pixelWidthToVolume(1.0, lengthUnit, volumeUnit);
          double width = model.getGeometry().getMesh()->getMembraneWidth(
              membraneID.toStdString());
          double scaleFactor = width * lengthCubedToVolFactor;
          SPDLOG_INFO("  - membrane width = {} {}", width,
                      lengthUnit.name.toStdString());
          SPDLOG_INFO("  - [length]^3/[vol] = {}", lengthCubedToVolFactor);
          SPDLOG_INFO("  - dividing flux by {}", scaleFactor);
          reacScaleFactors.push_back(
              utils::dblToQStr(scaleFactor).toStdString());
          reacs.push_back(r.toStdString());
        }
      }
      PDE pde(&model, nonConstantSpecies, reacs, duneSpeciesNames,
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
        // NOTE: setting diffusion to zero for now within membranes
        ini.addValue(duneSpeciesNames.at(i).c_str(), 0, doublePrecision);
      }

      // output file
      if (forExternalUse) {
        ini.addSection("model", membraneID, "writer");
        ini.addValue("file_name", membraneID);
      }
    }
  }

  // logger settings
  ini.addSection("logging");
  if (forExternalUse) {
    ini.addValue("default.level", "info");
  } else if (SPDLOG_ACTIVE_LEVEL < 2) {
    // for debug GUI builds enable verbose DUNE logging
    ini.addValue("default.level", "trace");
    ini.addSection("logging.backend.model");
    ini.addValue("level", "trace");
    ini.addValue("indent", 2);
    ini.addSection("logging.backend.solver");
    ini.addValue("level", "trace");
    ini.addValue("indent", 4);
  } else {
    // for release builds disable DUNE logging
    ini.addValue("default.level", "off");
  }
  iniFile = ini.getText();

  if (forExternalUse) {
    // export ini file
    SPDLOG_TRACE("Exporting dune ini file: '{}'", iniFilename.toStdString());
    if (QFile f(iniFilename); f.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f.write(iniFile.toUtf8());
    } else {
      SPDLOG_ERROR("Failed to export ini file '{}'", iniFilename.toStdString());
    }
    // export gmsh file `grid.msh` in the same dir
    QString gmshFilename = QDir(iniFileDir).filePath("grid.msh");
    SPDLOG_TRACE("Exporting gmsh file: '{}'", gmshFilename.toStdString());
    if (QFile f(gmshFilename); f.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f.write(mesh->getGMSH(gmshCompIndices).toUtf8());
    } else {
      SPDLOG_ERROR("Failed to export gmsh file '{}'",
                   gmshFilename.toStdString());
    }
  }
}

QString DuneConverter::getIniFile() const { return iniFile; }

const std::unordered_set<int> &DuneConverter::getGMSHCompIndices() const {
  return gmshCompIndices;
}

bool DuneConverter::hasIndependentCompartments() const {
  return independentCompartments;
}

const mesh::Mesh *DuneConverter::getMesh() const { return mesh; }

const std::vector<std::vector<std::vector<double>>> &
DuneConverter::getConcentrations() const {
  return concentrations;
}

double DuneConverter::getXOrigin() const { return x0; }

double DuneConverter::getYOrigin() const { return y0; }

double DuneConverter::getPixelWidth() const { return a; }

int DuneConverter::getImageWidth() const { return w; }

} // namespace simulate
