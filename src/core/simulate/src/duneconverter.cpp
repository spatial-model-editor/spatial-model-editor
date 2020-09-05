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
#include "simulate_options.hpp"
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

static bool modelHasIndependentCompartments(const model::Model &model) {
  for (const auto &mem : model.getMembranes().getMembranes()) {
    QString compA = mem.getCompartmentA()->getId().c_str();
    QString compB = mem.getCompartmentB()->getId().c_str();
    if (compartmentContainsNonConstantSpecies(model, compA) ||
        compartmentContainsNonConstantSpecies(model, compB)) {
      // membrane with non-constant species -> coupled compartments
      return false;
    }
  }
  return true;
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

static void addGrid(IniFile &ini) {
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");
  ini.addValue("initial_level", 0);
}

static void addModel(IniFile &ini) {
  ini.addSection("model");
  ini.addValue("order", 1);
}

static void addTimeStepping(IniFile &ini,
                            const simulate::DuneOptions &duneOptions,
                            int doublePrecision) {
  ini.addSection("model", "time_stepping");
  ini.addValue("rk_method", duneOptions.integrator.c_str());
  ini.addValue("begin", 0.0, doublePrecision);
  ini.addValue("end", 100.0, doublePrecision);
  ini.addValue("initial_step", duneOptions.dt, doublePrecision);
  ini.addValue("min_step", duneOptions.minDt, doublePrecision);
  ini.addValue("max_step", duneOptions.maxDt, doublePrecision);
  ini.addValue("decrease_factor", duneOptions.decrease, doublePrecision);
  ini.addValue("increase_factor", duneOptions.increase, doublePrecision);
}

static void addLogging(IniFile &ini, bool forExternalUse) {
  ini.addSection("logging.sinks.stdout");
  ini.addValue("level", "trace");
  ini.addSection("logging.default");
  if (forExternalUse) {
    ini.addValue("level", "info");
    ini.addValue("sinks", "stdout");
  } else if (SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG) {
    // for debug GUI builds enable verbose DUNE logging
    ini.addValue("level", "trace");
    ini.addValue("sinks", "stdout");
  } else {
    // for release GUI builds disable DUNE logging
    ini.addValue("level", "off");
    ini.addValue("sinks", "stdout");
  }
}

static void addWriter(IniFile &ini) {
  ini.addSection("model", "writer");
  ini.addValue("file_path", "vtk");
}

DuneConverter::DuneConverter(const model::Model &model, bool forExternalUse,
                             const simulate::DuneOptions &duneOptions,
                             const QString &outputIniFile, int doublePrecision)
    : mesh{model.getGeometry().getMesh()},
      x0{model.getGeometry().getPhysicalOrigin().x()},
      y0{model.getGeometry().getPhysicalOrigin().y()},
      a{model.getGeometry().getPixelWidth()},
      w{model.getGeometry().getImage().width()} {

  independentCompartments = modelHasIndependentCompartments(model);

  QString iniFileDir{QDir::currentPath()};
  std::vector<QString> iniFilenames;
  QString baseIniFile{"dune"};
  if (!outputIniFile.isEmpty()) {
    iniFileDir = QFileInfo(outputIniFile).absolutePath();
    baseIniFile = outputIniFile;
    if (baseIniFile.right(4) == ".ini") {
      baseIniFile.chop(4);
    }
  }

  constexpr double invalidPixelConc{-999.0};
  IniFile iniCommon;
  addGrid(iniCommon);
  addModel(iniCommon);
  addTimeStepping(iniCommon, duneOptions, doublePrecision);
  addLogging(iniCommon, forExternalUse);
  addWriter(iniCommon);

  std::vector<IniFile> inis;
  if (independentCompartments) {
    // create independent ini file for each compartment
    for (const auto &name : model.getCompartments().getNames()) {
      auto fname{QString("%1_%2.ini").arg(baseIniFile).arg(name)};
      iniFilenames.push_back(QDir(iniFileDir).filePath(fname));
      inis.push_back(iniCommon);
    }
  } else {
    // single ini file for model
    auto fname{QString("%1.ini").arg(baseIniFile)};
    iniFilenames.push_back(QDir(iniFileDir).filePath(fname));
    inis.push_back(iniCommon);
  }

  // list of compartments with corresponding gmsh surface index - 1
  if (!independentCompartments) {
    inis[0].addSection("model.compartments");
  }
  gmshCompIndices.clear();
  int gmshCompIndex = 1;
  int duneCompIndex = 0;
  for (const auto &comp : model.getCompartments().getIds()) {
    SPDLOG_TRACE("compartment {}", comp.toStdString());
    // skip compartments which contain no non-constant species
    if (compartmentContainsNonConstantSpecies(model, comp)) {
      if (independentCompartments) {
        inis[static_cast<std::size_t>(duneCompIndex)].addSection(
            "model.compartments");
        inis[static_cast<std::size_t>(duneCompIndex)].addValue(comp, 0);
        SPDLOG_TRACE("  -> added to independent model {}", duneCompIndex);
      } else {
        inis[0].addValue(comp, duneCompIndex);
        SPDLOG_TRACE("  -> added with index {}", duneCompIndex);
      }
      gmshCompIndices.insert(gmshCompIndex);
      ++duneCompIndex;
    }
    ++gmshCompIndex;
  }
  if (!independentCompartments) {
    for (const auto &mem : model.getMembranes().getMembranes()) {
      QString compA = mem.getCompartmentA()->getId().c_str();
      QString compB = mem.getCompartmentB()->getId().c_str();
      // only add membranes which contain non-constant species
      if (compartmentContainsNonConstantSpecies(model, compA) ||
          compartmentContainsNonConstantSpecies(model, compB)) {
        inis[0].addValue(mem.getId().c_str(), duneCompIndex);
        SPDLOG_TRACE("membrane {} added with index {}", mem.getId(),
                     duneCompIndex);
        gmshCompIndices.insert(gmshCompIndex);
        ++duneCompIndex;
      }
      ++gmshCompIndex;
    }
  }

  std::size_t modelIndex{0};
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
      auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
      for (std::size_t is = 0; is < duneSpeciesNames.size(); ++is) {
        SPDLOG_TRACE("  - species '{}' -> '{}'", nonConstantSpecies.at(is),
                     duneSpeciesNames.at(is));
      }

      // initial concentrations
      std::vector<std::vector<double>> concs(duneSpeciesNames.size(),
                                             std::vector<double>{});
      auto indices = getIndicesOfSortedVector(duneSpeciesNames);
      std::vector<QString> tiffs;
      inis[modelIndex].addSection("model", compartmentID, "initial");
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
            inis[modelIndex].addValue(
                duneName, QString("%2*%3(x,y)").arg(max).arg(sampledFieldName));
          } else {
            // otherwise just use initialConcentration value
            inis[modelIndex].addValue(duneName, initConc, doublePrecision);
          }
        } else {
          inis[modelIndex].addValue(duneName, 0.0, doublePrecision);
          // create array of concentration values
          concs[indices[i]] = f->getConcentrationImageArray(invalidPixelConc);
        }
      }
      concentrations.push_back(std::move(concs));
      if (forExternalUse && !tiffs.empty()) {
        inis[modelIndex].addSection("model", "data");
        for (const auto &tiff : tiffs) {
          inis[modelIndex].addValue(tiff, tiff + QString(".tif"));
        }
      }

      // reactions
      inis[modelIndex].addSection("model", compartmentID, "reaction");
      std::size_t nSpecies = nonConstantSpecies.size();

      std::vector<std::string> reacs;
      if (auto reacsInCompartment = model.getReactions().getIds(compartmentID);
          !reacsInCompartment.isEmpty()) {
        reacs = utils::toStdString(reacsInCompartment);
      }
      PDE pde(&model, nonConstantSpecies, reacs, duneSpeciesNames);
      for (std::size_t i = 0; i < nSpecies; ++i) {
        inis[modelIndex].addValue(duneSpeciesNames.at(i).c_str(),
                                  pde.getRHS().at(i).c_str());
      }

      // reaction term jacobian
      inis[modelIndex].addSection("model", compartmentID, "reaction.jacobian");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        for (std::size_t j = 0; j < nSpecies; ++j) {
          QString lhs = QString("d%1__d%2")
                            .arg(duneSpeciesNames.at(i).c_str(),
                                 duneSpeciesNames.at(j).c_str());
          QString rhs = pde.getJacobian().at(i).at(j).c_str();
          inis[modelIndex].addValue(lhs, rhs);
        }
      }

      // diffusion coefficients
      inis[modelIndex].addSection("model", compartmentID, "diffusion");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        inis[modelIndex].addValue(duneSpeciesNames.at(i).c_str(),
                                  model.getSpecies().getDiffusionConstant(
                                      nonConstantSpecies[i].c_str()),
                                  doublePrecision);
      }
    }
    if (independentCompartments) {
      ++modelIndex;
    }
  }

  // for each membrane do the same
  modelIndex = 0;
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
      auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);

      // initial concentrations
      std::vector<std::vector<double>> concs(duneSpeciesNames.size(),
                                             std::vector<double>{});
      auto indices = getIndicesOfSortedVector(duneSpeciesNames);
      inis[modelIndex].addSection("model", membraneID, "initial");
      for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
        QString name = nonConstantSpecies.at(i).c_str();
        inis[modelIndex].addValue(
            duneSpeciesNames.at(i).c_str(),
            model.getSpecies().getInitialConcentration(name), doublePrecision);
        if (!forExternalUse) {
          const auto *f = model.getSpecies().getField(name);
          concs[indices[i]] = f->getConcentrationImageArray(invalidPixelConc);
        }
      }
      concentrations.push_back(std::move(concs));

      // reactions: want reactions for both neighbouring compartments
      // as well as membrane reactions (that involve species from both
      // compartments in the same reaction)
      inis[modelIndex].addSection("model", membraneID, "reaction");
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
        inis[modelIndex].addValue(duneSpeciesNames.at(i).c_str(),
                                  pde.getRHS().at(i).c_str());
      }

      // reaction term jacobian
      inis[modelIndex].addSection("model", membraneID, "reaction.jacobian");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        for (std::size_t j = 0; j < nSpecies; ++j) {
          QString lhs = QString("d%1__d%2")
                            .arg(duneSpeciesNames.at(i).c_str(),
                                 duneSpeciesNames.at(j).c_str());
          QString rhs = pde.getJacobian().at(i).at(j).c_str();
          inis[modelIndex].addValue(lhs, rhs);
        }
      }

      // diffusion coefficients
      inis[modelIndex].addSection("model", membraneID, "diffusion");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        // NOTE: setting diffusion to zero for now within membranes
        inis[modelIndex].addValue(duneSpeciesNames.at(i).c_str(), 0,
                                  doublePrecision);
      }
    }
  }

  for (const auto &ini : inis) {
    iniFiles.push_back(ini.getText());
  }

  if (forExternalUse) {
    // export ini files
    for (std::size_t i = 0; i < iniFiles.size(); ++i) {
      SPDLOG_TRACE("Exporting dune ini file: '{}'",
                   iniFilenames[i].toStdString());
      if (QFile f(iniFilenames[i]);
          f.open(QIODevice::ReadWrite | QIODevice::Text)) {
        f.write(iniFiles[i].toUtf8());
      } else {
        SPDLOG_ERROR("Failed to export ini file '{}'",
                     iniFilenames[i].toStdString());
      }
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

QString DuneConverter::getIniFile(std::size_t compartmentIndex) const {
  return iniFiles[compartmentIndex];
}

const std::vector<QString> &DuneConverter::getIniFiles() const {
  return iniFiles;
}

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
