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
  ini.addValue("path", "grid.msh");
  ini.addValue("dimension", 2);
}

static void addModel(IniFile &ini) {
  ini.addSection("model");
  ini.addValue("order", 1);
  ini.addValue("parser_type", "SymEngineSBML");
}

static void addTimeStepping(IniFile &ini,
                            const simulate::DuneOptions &duneOptions,
                            int doublePrecision) {
  ini.addSection("model", "time_step_operator");
  ini.addValue("time_begin", 0.0, doublePrecision);
  ini.addValue("time_end", 100.0, doublePrecision);
  ini.addValue("time_step_initial", duneOptions.dt, doublePrecision);
  ini.addValue("time_step_increase_factor", duneOptions.increase,
               doublePrecision);
  ini.addValue("time_step_decrease_factor", duneOptions.decrease,
               doublePrecision);
  ini.addValue("time_step_min", duneOptions.minDt, doublePrecision);
  ini.addValue("time_step_max", duneOptions.maxDt, doublePrecision);
  ini.addSection("model", "time_step_operator", "linear_solver");
  ini.addValue("type", duneOptions.linearSolver.c_str());
  ini.addSection("model", "time_step_operator", "nonlinear_solver");
  ini.addValue("type", duneOptions.integrator.c_str());
  ini.addValue("convergence_condition.relative_tolerance",
               duneOptions.newtonRelErr, doublePrecision);
  ini.addValue("convergence_condition.absolute_tolerance",
               duneOptions.newtonAbsErr, doublePrecision);
}

static void addWriter(IniFile &ini) {
  ini.addSection("model", "writer.vtk");
  ini.addValue("path", "vtk");
}

static void addCompartment(
    IniFile &ini, const model::Model &model,
    const std::map<std::string, double, std::less<>> &substitutions,
    int doublePrecision, bool forExternalUse, const QString &iniFileDir,
    std::unordered_map<std::string, std::vector<double>> &concentrations,
    std::unordered_map<std::string, std::vector<std::string>> &speciesNames,
    const QString &compartmentId, std::size_t &simDataCompartmentIndex) {
  const auto &lengthUnit = model.getUnits().getLength();
  const auto &volumeUnit = model.getUnits().getVolume();
  double volOverL3{model::getVolOverL3(lengthUnit, volumeUnit)};
  std::vector<std::string> extraReactionVars{
      "time", model.getParameters().getSpatialCoordinates().x.id,
      model.getParameters().getSpatialCoordinates().y.id,
      model.getParameters().getSpatialCoordinates().z.id};
  std::vector<std::string> relabelledExtraReactionVars{
      "time", "position_x", "position_y", "position_z"};

  SPDLOG_TRACE("compartment {}", compartmentId.toStdString());

  auto nonConstantSpecies = getNonConstantSpecies(model, compartmentId);
  auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
  speciesNames[compartmentId.toStdString()] = [&model, &compartmentId,
                                               &duneSpeciesNames]() {
    std::vector<std::string> duneSpeciesNamesSmeIndices;
    std::size_t duneNameIndex{0};
    for (const auto &s : model.getSpecies().getIds(compartmentId)) {
      if (!model.getSpecies().getIsConstant(s)) {
        duneSpeciesNamesSmeIndices.push_back(duneSpeciesNames[duneNameIndex++]);
        SPDLOG_INFO("  - SME species {} -> DUNE species {}", s.toStdString(),
                    duneSpeciesNamesSmeIndices.back());
      } else {
        // if SME species is not in dune sim dune species name is empty string
        duneSpeciesNamesSmeIndices.emplace_back();
        SPDLOG_INFO(
            "  - SME species {} is constant -> not present in DUNE simulation",
            s.toStdString());
      }
    }
    return duneSpeciesNamesSmeIndices;
  }();

  // construct reactions
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

  std::vector<QString> tiffs;
  std::size_t nSpecies{nonConstantSpecies.size()};
  if (nSpecies == 0) {
    // dummy species with no reactions if compartment is empty
    auto dummySpeciesName{QString("%1_dummy_species").arg(compartmentId)};
    ini.addSection("model", "scalar_field", dummySpeciesName);
    ini.addValue("compartment", compartmentId);
    ini.addValue("initial.expression", "0");
    // todo: this reaction expression is a hack to avoid timestep -> 0
    ini.addValue("reaction.expression", "0.1");
    ini.addValue(
        QString("reaction.jacobian.%1.expression").arg(dummySpeciesName), "0");
    ini.addValue("storage.expression", "1");
    return;
  }
  for (std::size_t i = 0; i < nSpecies; ++i) {
    QString name{nonConstantSpecies[i].c_str()};
    QString duneName{duneSpeciesNames[i].c_str()};
    ini.addSection("model", "scalar_field", duneName);
    ini.addValue("compartment", compartmentId);
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
        ini.addValue("initial.expression",
                     QString("%1*%2(position_x,position_y)")
                         .arg(max)
                         .arg(sampledFieldName));
      } else {
        // otherwise just use initialConcentration value
        ini.addValue("initial.expression", initConc, doublePrecision);
      }
    } else {
      ini.addValue("initial.expression", 0.0, doublePrecision);
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
        for (std::size_t iPixel = 0; iPixel < nPixels; ++iPixel) {
          c[iPixel] =
              simConcs.back()[simDataCompartmentIndex][iPixel * stride + i];
        }
        simField.setConcentration(c);
        concentrations[duneName.toStdString()] =
            simField.getConcentrationImageArray();
      } else {
        concentrations[duneName.toStdString()] =
            f->getConcentrationImageArray();
      }
      for (auto &c : concentrations[duneName.toStdString()]) {
        // convert A/V to A/L^3
        c /= volOverL3;
      }
    }

    // Factor to multiply d/dt term by in PDE
    ini.addValue("storage.expression", "1");

    // reactions
    ini.addValue("reaction.expression", pde.getRHS()[i].c_str());
    for (std::size_t j = 0; j < nSpecies; ++j) {
      QString lhs = QString("reaction.jacobian.%1.expression")
                        .arg(duneSpeciesNames[j].c_str());
      QString rhs = pde.getJacobian()[i][j].c_str();
      ini.addValue(lhs, rhs);
    }

    // diffusion coefficient
    QString sId{nonConstantSpecies[i].c_str()};
    double diffConst{model.getSpecies().getDiffusionConstant(sId)};
    ini.addValue(QString("cross_diffusion.%1.expression").arg(duneName),
                 diffConst, doublePrecision);

    // membrane flux terms
    // todo: should collect membranes outside of this loop over species!
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
        QString membraneID = membrane.getId().c_str();
        auto mReacs =
            common::toStdString(model.getReactions().getIds(membraneID));
        auto nonConstantSpeciesOther =
            getNonConstantSpecies(model, otherCompId);
        auto duneSpeciesNamesOther =
            makeValidDuneSpeciesNames(nonConstantSpeciesOther);

        auto mSpecies = nonConstantSpecies;
        for (const auto &s : nonConstantSpeciesOther) {
          mSpecies.push_back(s);
        }
        auto mDuneSpecies = duneSpeciesNames;
        for (const auto &s : duneSpeciesNamesOther) {
          mDuneSpecies.push_back(s);
        }
        PdeScaleFactors mScaleFactors;
        mScaleFactors.species = volOverL3;
        mScaleFactors.reaction = -1.0;
        SPDLOG_INFO("  - multiplying species by [vol]/[length]^3 = {}",
                    mScaleFactors.species);

        Pde pdeBcs(&model, mSpecies, mReacs, mDuneSpecies, mScaleFactors,
                   extraReactionVars, relabelledExtraReactionVars,
                   substitutions);
        // reaction expression
        ini.addValue(QString("outflow.%1.expression").arg(otherCompId),
                     pdeBcs.getRHS()[i].c_str());
        // reaction expression jacobian
        for (std::size_t j = 0; j < mDuneSpecies.size(); ++j) {
          QString lhs = QString("outflow.%1.jacobian.%2.expression")
                            .arg(otherCompId, mDuneSpecies[j].c_str());
          QString rhs = pdeBcs.getJacobian()[i][j].c_str();
          ini.addValue(lhs, rhs);
        }
      }
    }
  }
  if (!nonConstantSpecies.empty()) {
    ++simDataCompartmentIndex;
  }
  if (forExternalUse && !tiffs.empty()) {
    for (const auto &tiff : tiffs) {
      ini.addSection("parser_context", tiff);
      ini.addValue("type", "tiff");
      ini.addValue("path", tiff + QString(".tif"));
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

  QString iniFileDir{QDir::currentPath()};
  QString baseIniFile{"dune"};
  if (!outputIniFile.isEmpty()) {
    iniFileDir = QFileInfo(outputIniFile).absolutePath();
    baseIniFile = outputIniFile;
    if (baseIniFile.right(4) == ".ini") {
      baseIniFile.chop(4);
    }
  }
  auto filename{QString("%1.ini").arg(baseIniFile)};
  auto iniFilename{QDir(iniFileDir).filePath(filename)};

  IniFile iniCommon;
  addGrid(iniCommon);
  addModel(iniCommon);
  addTimeStepping(iniCommon, model.getSimulationSettings().options.dune,
                  doublePrecision);
  addWriter(iniCommon);

  // list of compartments with corresponding gmsh surface index
  if (forExternalUse) {
    // compartments is a top level section when running externally
    iniCommon.addSection("compartments");
  } else {
    // otherwise it is part of model
    iniCommon.addSection("model", "compartments");
  }
  int duneCompIndex{1};
  for (const auto &comp : model.getCompartments().getIds()) {
    SPDLOG_TRACE("compartment {}", comp.toStdString());
    iniCommon.addValue(QString("%1.expression").arg(comp),
                       QString("(gmsh_id == %1)").arg(duneCompIndex));
    SPDLOG_TRACE("  -> added with index {}", duneCompIndex);
    if (!forExternalUse) {
      // also need an id which determines the sub-grid for this compartment
      // TODO: confirm this matches the sub-grid indices that we assign when
      // constructing the grid
      iniCommon.addValue(QString("%1.id").arg(comp), duneCompIndex - 1);
    }
    ++duneCompIndex;
  }

  std::size_t simDataCompartmentIndex{0};
  // for each compartment
  for (const auto &compId : model.getCompartments().getIds()) {
    addCompartment(iniCommon, model, substitutions, doublePrecision,
                   forExternalUse, iniFileDir, concentrations, speciesNames,
                   compId, simDataCompartmentIndex);
    compartmentNames.push_back(compId.toStdString());
  }

  iniFile = iniCommon.getText();

  if (forExternalUse) {
    // export ini files
    SPDLOG_TRACE("Exporting dune ini file: '{}'", iniFilename.toStdString());
    if (QFile f(iniFilename);
        f.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
      f.write(iniFile.toUtf8());
    } else {
      SPDLOG_ERROR("Failed to export ini file '{}'", iniFilename.toStdString());
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

QString DuneConverter::getIniFile() const { return iniFile; }

const mesh::Mesh *DuneConverter::getMesh() const { return mesh; }

const std::unordered_map<std::string, std::vector<double>> &
DuneConverter::getConcentrations() const {
  return concentrations;
}

[[nodiscard]] const std::unordered_map<std::string, std::vector<std::string>> &
DuneConverter::getSpeciesNames() const {
  return speciesNames;
}

[[nodiscard]] const std::vector<std::string> &
DuneConverter::getCompartmentNames() const {
  return compartmentNames;
}

common::VoxelF DuneConverter::getOrigin() const { return origin; }

common::VolumeF DuneConverter::getVoxelSize() const { return voxelSize; }

common::Volume DuneConverter::getImageSize() const { return imageSize; }

} // namespace sme::simulate
