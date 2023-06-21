#include "sme/duneconverter.hpp"
#include "duneconverter_impl.hpp"
#include "duneini.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/mesh.hpp"
#include "sme/model.hpp"
#include "sme/pde.hpp"
#include "sme/simulate_options.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QFile>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <string>
#include <utility>

namespace sme::simulate {

static void addGrid(IniFile &ini) {
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");
  ini.addValue("initial_level", 0);
  ini.addValue("dimension", 2);
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

  ini.addSection("model", "time_stepping", "newton");
  ini.addValue("reduction", duneOptions.newtonRelErr, doublePrecision);
  ini.addValue("min_linear_reduction", 1e-3, doublePrecision);
  ini.addValue("fixed_linear_reduction", "false");
  ini.addValue("max_iterations", 40);
  ini.addValue("absolute_limit", duneOptions.newtonAbsErr, doublePrecision);
  ini.addValue("reassemble_threshold", 0.0, doublePrecision);
  ini.addValue("keep_matrix", "true");
  ini.addValue("force_iteration", "false");

  ini.addSection("model", "time_stepping", "newton.linear_search");
  ini.addValue("strategy", "hackbuschReusken");
  ini.addValue("max_iterations", 10);
  ini.addValue("damping_factor", 0.5, doublePrecision);
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

static void addCompartment(
    IniFile &ini, const model::Model &model,
    const std::map<std::string, double, std::less<>> &substitutions,
    int doublePrecision, bool forExternalUse, const QString &iniFileDir,
    std::vector<std::vector<std::vector<double>>> &concentrations,
    const QString &compartmentId, std::size_t &simDataCompartmentIndex) {
  const auto &lengthUnit = model.getUnits().getLength();
  const auto &volumeUnit = model.getUnits().getVolume();
  double volOverL3{model::getVolOverL3(lengthUnit, volumeUnit)};
  std::vector<std::string> extraReactionVars{
      "time", model.getParameters().getSpatialCoordinates().x.id,
      model.getParameters().getSpatialCoordinates().y.id};
  std::vector<std::string> relabelledExtraReactionVars{"t", "x", "y"};

  SPDLOG_TRACE("compartment {}", compartmentId.toStdString());
  auto nonConstantSpecies = getNonConstantSpecies(model, compartmentId);
  if (nonConstantSpecies.empty()) {
    // need to add a dummy species with no reactions:
    // dune doesn't support simulating an empty compartment
    QString dummySpeciesName{"dummySpeciesForEmptyCompartment"};
    ini.addSection("model", compartmentId, "initial");
    ini.addValue(dummySpeciesName, "0");
    // empty concentrations vector: gridfunction just returns zero everywhere
    concentrations.emplace_back(1, std::vector<double>{});
    ini.addSection("model", compartmentId, "reaction");
    ini.addValue(dummySpeciesName, "0");
    ini.addSection("model", compartmentId, "reaction.jacobian");
    ini.addValue(QString("d%1__d%2").arg(dummySpeciesName, dummySpeciesName),
                 "0");
    ini.addSection("model", compartmentId, "diffusion");
    ini.addValue(dummySpeciesName, "0");
    for (const auto &membrane : model.getMembranes().getMembranes()) {
      auto cA{membrane.getCompartmentA()->getId().c_str()};
      auto cB{membrane.getCompartmentB()->getId().c_str()};
      QString otherCompId;
      if (cA == compartmentId) {
        otherCompId = cB;
      } else if (cB == compartmentId) {
        otherCompId = cA;
      }
      if (!otherCompId.isEmpty()) {
        ini.addSection("model", compartmentId, "boundary", otherCompId,
                       "outflow");
        ini.addValue(dummySpeciesName, "0");
        ini.addSection("model", compartmentId, "boundary", otherCompId,
                       "outflow", "jacobian");
        ini.addValue(
            QString("d%1_i__d%2_i").arg(dummySpeciesName, dummySpeciesName),
            "0");
      }
    }
    return;
  }
  auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
  for (std::size_t is = 0; is < duneSpeciesNames.size(); ++is) {
    SPDLOG_TRACE("  - species '{}' -> '{}'", nonConstantSpecies[is],
                 duneSpeciesNames[is]);
  }

  // initial concentrations
  auto &concs{concentrations.emplace_back(duneSpeciesNames.size(),
                                          std::vector<double>{})};
  // these smeToDuneIndices map from a dune species index to the SME species
  // index:
  auto duneToSmeIndices{common::getIndicesOfSortedVector(duneSpeciesNames)};
  // we want the inverse mapping: from SME index to dune index
  auto smeToDuneIndices = duneToSmeIndices;
  for (std::size_t i = 0; i < smeToDuneIndices.size(); ++i) {
    smeToDuneIndices[duneToSmeIndices[i]] = i;
  }
  for (std::size_t i = 0; i < smeToDuneIndices.size(); ++i) {
    SPDLOG_INFO("  - SME species {} -> DUNE species {} [{}]", i,
                smeToDuneIndices[i], duneSpeciesNames[i]);
  }
  std::vector<QString> tiffs;
  ini.addSection("model", compartmentId, "initial");
  for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
    QString name{nonConstantSpecies[i].c_str()};
    QString duneName{duneSpeciesNames[i].c_str()};
    double initConc = model.getSpecies().getInitialConcentration(name);
    // convert A/V to A/L^3
    initConc /= volOverL3;
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
        auto conc = f->getConcentrationImageArray();
        for (auto &c : conc) {
          // convert A/V to A/L^3
          c /= volOverL3;
        }
        double max = common::writeTIFF(
            tiffFilename.toStdString(),
            f->getCompartment()->getCompartmentImages()[0].size(), conc,
            model.getGeometry().getVoxelSize());
        tiffs.push_back(sampledFieldName);
        ini.addValue(duneName,
                     QString("%1*%2(x,y)").arg(max).arg(sampledFieldName));
      } else {
        // otherwise just use initialConcentration value
        ini.addValue(duneName, initConc, doublePrecision);
      }
    } else {
      ini.addValue(duneName, 0.0, doublePrecision);
      // create array of concentration values
      if (const auto &simConcs{model.getSimulationData().concentration};
          simConcs.size() > 1) {
        // use concentrations from existing simulation data
        auto simField{*f};
        const std::size_t nPixels{f->getCompartment()->nVoxels()};
        const std::size_t padding{model.getSimulationData().concPadding.back()};
        const std::size_t stride{padding + nonConstantSpecies.size()};
        std::vector<double> c(nPixels, 0.0);
        SPDLOG_INFO("using simulation concentration data for species {}",
                    name.toStdString());
        SPDLOG_INFO("- species index {}", i);
        SPDLOG_INFO("- species dune index {}", smeToDuneIndices[i]);
        for (std::size_t iPixel = 0; iPixel < nPixels; ++iPixel) {
          c[iPixel] =
              simConcs.back()[simDataCompartmentIndex][iPixel * stride + i];
        }
        simField.setConcentration(c);
        concs[smeToDuneIndices[i]] = simField.getConcentrationImageArray();
      } else {
        concs[smeToDuneIndices[i]] = f->getConcentrationImageArray();
      }
      for (auto &c : concs[smeToDuneIndices[i]]) {
        // convert A/V to A/L^3
        c /= volOverL3;
      }
    }
  }
  if (!nonConstantSpecies.empty()) {
    ++simDataCompartmentIndex;
  }
  if (forExternalUse && !tiffs.empty()) {
    ini.addSection("model", "data");
    for (const auto &tiff : tiffs) {
      ini.addValue(tiff, tiff + QString(".tif"));
    }
  }

  // reactions
  ini.addSection("model", compartmentId, "reaction");
  std::size_t nSpecies{nonConstantSpecies.size()};

  auto reacs = common::toStdString(model.getReactions().getIds(compartmentId));
  PdeScaleFactors scaleFactors;
  scaleFactors.species = volOverL3;
  scaleFactors.reaction = 1.0 / volOverL3;
  SPDLOG_INFO("  - multiplying species by [vol]/[length]^3 = {}",
              scaleFactors.species);
  SPDLOG_INFO("  - multiplying reactions by [length]^3/[vol] = {}",
              scaleFactors.reaction);

  Pde pde(&model, nonConstantSpecies, reacs, duneSpeciesNames, scaleFactors,
          extraReactionVars, relabelledExtraReactionVars, substitutions);
  for (std::size_t i = 0; i < nSpecies; ++i) {
    ini.addValue(duneSpeciesNames[i].c_str(), pde.getRHS()[i].c_str());
  }

  // reaction term jacobian
  ini.addSection("model", compartmentId, "reaction.jacobian");
  for (std::size_t i = 0; i < nSpecies; ++i) {
    for (std::size_t j = 0; j < nSpecies; ++j) {
      QString lhs =
          QString("d%1__d%2")
              .arg(duneSpeciesNames[i].c_str(), duneSpeciesNames[j].c_str());
      QString rhs = pde.getJacobian()[i][j].c_str();
      ini.addValue(lhs, rhs);
    }
  }

  // diffusion coefficients
  ini.addSection("model", compartmentId, "diffusion");
  for (std::size_t i = 0; i < nSpecies; ++i) {
    QString sId{nonConstantSpecies[i].c_str()};
    double diffConst{model.getSpecies().getDiffusionConstant(sId)};
    ini.addValue(duneSpeciesNames[i].c_str(), diffConst, doublePrecision);
  }

  // membrane flux terms
  for (const auto &membrane : model.getMembranes().getMembranes()) {
    auto cA = membrane.getCompartmentA()->getId().c_str();
    auto cB = membrane.getCompartmentB()->getId().c_str();
    QString otherCompId;
    if (cA == compartmentId) {
      otherCompId = cB;
    } else if (cB == compartmentId) {
      otherCompId = cA;
    }
    if (!otherCompId.isEmpty()) {
      ini.addSection("model", compartmentId, "boundary", otherCompId,
                     "outflow");
      QString membraneID = membrane.getId().c_str();
      auto mReacs =
          common::toStdString(model.getReactions().getIds(membraneID));
      auto nonConstantSpeciesOther = getNonConstantSpecies(model, otherCompId);
      auto duneSpeciesNamesOther =
          makeValidDuneSpeciesNames(nonConstantSpeciesOther);

      auto mSpecies = nonConstantSpecies;
      for (const auto &s : nonConstantSpeciesOther) {
        mSpecies.push_back(s);
      }
      auto mDuneSpecies = duneSpeciesNames;
      for (auto &s : mDuneSpecies) {
        s.append("_i");
      }
      for (const auto &s : duneSpeciesNamesOther) {
        mDuneSpecies.push_back(s + "_o");
      }
      PdeScaleFactors mScaleFactors;
      mScaleFactors.species = volOverL3;
      mScaleFactors.reaction = -1.0;
      SPDLOG_INFO("  - multiplying species by [vol]/[length]^3 = {}",
                  mScaleFactors.species);

      Pde pdeBcs(&model, mSpecies, mReacs, mDuneSpecies, mScaleFactors,
                 extraReactionVars, relabelledExtraReactionVars, substitutions);
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames[i].c_str(), pdeBcs.getRHS()[i].c_str());
      }

      // reaction term jacobian
      ini.addSection("model", compartmentId, "boundary", otherCompId, "outflow",
                     "jacobian");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        for (std::size_t j = 0; j < mDuneSpecies.size(); ++j) {
          QString lhs =
              QString("d%1__d%2")
                  .arg(duneSpeciesNames[i].c_str(), mDuneSpecies[j].c_str());
          QString rhs = pdeBcs.getJacobian()[i][j].c_str();
          ini.addValue(lhs, rhs);
        }
      }
    }
  }
}

DuneConverter::DuneConverter(
    const model::Model &model,
    const std::map<std::string, double, std::less<>> &substitutions,
    bool forExternalUse, const QString &outputIniFile, int doublePrecision)
    : mesh{model.getGeometry().getMesh()},
      origin{model.getGeometry().getPhysicalOrigin()},
      voxelSize{model.getGeometry().getVoxelSize()},
      imageSize{model.getGeometry().getImages().volume()} {

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

  IniFile iniCommon;
  addGrid(iniCommon);
  addModel(iniCommon);
  addTimeStepping(iniCommon, model.getSimulationSettings().options.dune,
                  doublePrecision);
  addLogging(iniCommon, forExternalUse);
  addWriter(iniCommon);

  std::vector<IniFile> inis;
  if (independentCompartments) {
    // create independent ini file for each compartment
    for (const auto &comp : model.getCompartments().getIds()) {
      const auto &name{model.getCompartments().getName(comp)};
      auto filename{QString("%1_%2.ini").arg(baseIniFile).arg(name)};
      iniFilenames.push_back(QDir(iniFileDir).filePath(filename));
      inis.push_back(iniCommon);
    }
  } else {
    // single ini file for model
    auto filename{QString("%1.ini").arg(baseIniFile)};
    iniFilenames.push_back(QDir(iniFileDir).filePath(filename));
    inis.push_back(iniCommon);
  }

  // list of compartments with corresponding gmsh surface index - 1
  if (!independentCompartments) {
    inis[0].addSection("model.compartments");
  }
  int duneCompIndex{0};
  for (const auto &comp : model.getCompartments().getIds()) {
    SPDLOG_TRACE("compartment {}", comp.toStdString());
    if (independentCompartments) {
      inis[static_cast<std::size_t>(duneCompIndex)].addSection(
          "model.compartments");
      inis[static_cast<std::size_t>(duneCompIndex)].addValue(comp, 0);
      SPDLOG_TRACE("  -> added to independent model {}", duneCompIndex);
    } else {
      inis[0].addValue(comp, duneCompIndex);
      SPDLOG_TRACE("  -> added with index {}", duneCompIndex);
    }
    ++duneCompIndex;
  }

  std::size_t modelIndex{0};
  std::size_t simDataCompartmentIndex{0};
  // for each compartment
  for (const auto &compId : model.getCompartments().getIds()) {
    addCompartment(inis[modelIndex], model, substitutions, doublePrecision,
                   forExternalUse, iniFileDir, concentrations, compId,
                   simDataCompartmentIndex);
    if (independentCompartments) {
      ++modelIndex;
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
      if (QFile f(iniFilenames[i]); f.open(
              QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
        f.write(iniFiles[i].toUtf8());
      } else {
        SPDLOG_ERROR("Failed to export ini file '{}'",
                     iniFilenames[i].toStdString());
      }
    }
    // export gmsh file `grid.msh` in the same dir
    QString gmshFilename = QDir(iniFileDir).filePath("grid.msh");
    SPDLOG_TRACE("Exporting gmsh file: '{}'", gmshFilename.toStdString());
    if (QFile f(gmshFilename);
        f.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
      f.write(mesh->getGMSH().toUtf8());
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

bool DuneConverter::hasIndependentCompartments() const {
  return independentCompartments;
}

const mesh::Mesh *DuneConverter::getMesh() const { return mesh; }

const std::vector<std::vector<std::vector<double>>> &
DuneConverter::getConcentrations() const {
  return concentrations;
}

common::VoxelF DuneConverter::getOrigin() const { return origin; }

common::VolumeF DuneConverter::getVoxelSize() const { return voxelSize; }

common::Volume DuneConverter::getImageSize() const { return imageSize; }

} // namespace sme::simulate
