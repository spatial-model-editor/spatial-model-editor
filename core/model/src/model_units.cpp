#include "sme/model_units.hpp"
#include "sme/logger.hpp"
#include <cmath>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

bool operator==(const Unit &a, const Unit &b) {
  constexpr double maxMultiplierRelativeDiff{1e-10};
  return a.kind == b.kind && a.exponent == b.exponent && a.scale == b.scale &&
         std::fabs((a.multiplier - b.multiplier) / a.multiplier) <
             maxMultiplierRelativeDiff;
}

QString unitInBaseUnits(const Unit &unit) {
  QString s;
  if (unit.exponent != 1) {
    s = "(";
  }
  if (unit.multiplier != 1.0) {
    s.append(QString::number(unit.multiplier, 'g', 14));
    s.append(" ");
    if (unit.scale != 0) {
      s.append("* ");
    }
  }
  if (unit.scale != 0) {
    s.append(QString("10^(%1) ").arg(unit.scale));
  }
  s.append(unit.kind);
  if (unit.exponent != 1) {
    s.append(QString(")^%1").arg(unit.exponent));
  }
  return s;
}

static libsbml::UnitDefinition *
getOrCreateUnitDef(libsbml::Model *model, const std::string &Id,
                   const std::string &defaultId) {
  auto *unitdef = model->getUnitDefinition(Id);
  if (unitdef == nullptr) {
    // if no existing unitdef, create one
    std::string newId = defaultId;
    // ensure it is unique among unitdef Ids
    // note: they are in a different namespace to other SBML objects
    while (model->getUnitDefinition(newId) != nullptr) {
      newId.append("_");
      SPDLOG_DEBUG("  -> {}", newId);
    }
    SPDLOG_DEBUG("creating UnitDefinition {}", newId);
    unitdef = model->createUnitDefinition();
    unitdef->setId(newId);
    unitdef->setName(defaultId);
  }
  return unitdef;
}

static void setSBMLUnitDef(libsbml::UnitDefinition *unitdef, const Unit &u) {
  if (unitdef == nullptr) {
    return;
  }
  unitdef->setName(u.name.toStdString());
  unitdef->getListOfUnits()->clear();
  auto *unit = unitdef->createUnit();
  unit->setKind(libsbml::UnitKind_forName(u.kind.toStdString().c_str()));
  unit->setMultiplier(u.multiplier);
  unit->setScale(u.scale);
  unit->setExponent(u.exponent);
}

static int getOrAddUnitIndex(libsbml::Model *model, const std::string &id,
                             QVector<Unit> &units, int defaultIndex) {
  SPDLOG_INFO("SId: {}", id);
  Unit u;
  if (libsbml::UnitKind_isValidUnitKindString(id.c_str(), model->getLevel(),
                                              model->getVersion())) {
    // check if id is a SBML base unit
    u.kind = id.c_str();
    u.multiplier = 1.0;
    u.exponent = 1;
    u.scale = 0;
  } else if (auto *unitdef = model->getUnitDefinition(id);
             unitdef != nullptr && unitdef->getNumUnits() == 1) {
    // if it is a UnitDef with a single unit, import it
    if (unitdef->getName().empty()) {
      unitdef->setName(unitdef->getId());
    }
    u.name = unitdef->getName().c_str();
    const auto *unit = unitdef->getUnit(0);
    u.kind = libsbml::UnitKind_toString(unit->getKind());
    u.multiplier = unit->getMultiplier();
    u.exponent = unit->getExponent();
    u.scale = unit->getScale();
  } else {
    SPDLOG_INFO("  -> failed to import unit, using default");
    return defaultIndex;
  }
  SPDLOG_INFO("  = {}", unitInBaseUnits(u).toStdString());
  // look for an existing equivalent unit in the model
  for (int i = 0; i < units.size(); ++i) {
    if (units[i] == u) {
      SPDLOG_INFO("  -> equivalent existing unit '{}'",
                  units[i].name.toStdString());
      return i;
    }
  }
  // otherwise add imported unit to model & return its index
  units.push_back(std::move(u));
  SPDLOG_INFO("  -> creating new unit '{}'", units.back().name.toStdString());
  return static_cast<int>(units.size() - 1);
}

UnitVector::UnitVector(const QVector<Unit> &unitsVec, int defaultIndex)
    : units{unitsVec}, index{defaultIndex} {}

const Unit &UnitVector::get() const { return units.at(index); }

const QVector<Unit> &UnitVector::getUnits() const { return units; }

QVector<Unit> &UnitVector::getUnits() { return units; }

int UnitVector::getIndex() const { return index; }

void UnitVector::setIndex(int newIndex) { index = newIndex; }

void ModelUnits::updateConcentration() {
  concentration = QString("%1/%2").arg(getAmount().name).arg(getVolume().name);
}

void ModelUnits::updateDiffusion() {
  diffusion = QString("%1^2/%2").arg(getLength().name).arg(getTime().name);
}

void ModelUnits::updateReactions() {
  compartmentReaction = QString("%1/%2").arg(concentration).arg(getTime().name);
  membraneReaction = QString("%1 / %2^2 / %3")
                         .arg(getAmount().name)
                         .arg(getLength().name)
                         .arg(getTime().name);
}

ModelUnits::ModelUnits(libsbml::Model *model) : sbmlModel{model} {
  hasUnsavedChanges = true;
  if (model != nullptr) {
    setTimeIndex(getOrAddUnitIndex(model, model->getTimeUnits(),
                                   time.getUnits(), time.getIndex()));
    setLengthIndex(getOrAddUnitIndex(model, model->getLengthUnits(),
                                     length.getUnits(), length.getIndex()));
    setVolumeIndex(getOrAddUnitIndex(model, model->getVolumeUnits(),
                                     volume.getUnits(), volume.getIndex()));
    setAmountIndex(getOrAddUnitIndex(model, model->getSubstanceUnits(),
                                     amount.getUnits(), amount.getIndex()));
  }
  updateConcentration();
  updateDiffusion();
  updateReactions();
}

const Unit &ModelUnits::getTime() const { return time.get(); }

int ModelUnits::getTimeIndex() const { return time.getIndex(); }

const QVector<Unit> &ModelUnits::getTimeUnits() const {
  return time.getUnits();
}

QVector<Unit> &ModelUnits::getTimeUnits() { return time.getUnits(); }

void ModelUnits::setTimeIndex(int index) {
  hasUnsavedChanges = true;
  time.setIndex(index);
  if (sbmlModel != nullptr) {
    auto *timeUnitDef = getOrCreateUnitDef(sbmlModel, sbmlModel->getTimeUnits(),
                                           "unit_of_time");
    sbmlModel->setTimeUnits(timeUnitDef->getId());
    setSBMLUnitDef(timeUnitDef, getTime());
  }
  updateDiffusion();
  updateReactions();
}

const Unit &ModelUnits::getLength() const { return length.get(); }

int ModelUnits::getLengthIndex() const { return length.getIndex(); }

const QVector<Unit> &ModelUnits::getLengthUnits() const {
  return length.getUnits();
}

QVector<Unit> &ModelUnits::getLengthUnits() { return length.getUnits(); }

void ModelUnits::setLengthIndex(int index) {
  hasUnsavedChanges = true;
  length.setIndex(index);
  if (sbmlModel != nullptr) {
    auto *lengthUnitDef = getOrCreateUnitDef(
        sbmlModel, sbmlModel->getLengthUnits(), "unit_of_length");
    sbmlModel->setLengthUnits(lengthUnitDef->getId());
    setSBMLUnitDef(lengthUnitDef, getLength());

    // also set units of area as length^2
    auto *areaUnitDef = getOrCreateUnitDef(sbmlModel, sbmlModel->getAreaUnits(),
                                           "unit_of_area");
    Unit u = getLength();
    u.name.append("_squared");
    u.exponent *= 2;
    sbmlModel->setAreaUnits(areaUnitDef->getId());
    setSBMLUnitDef(areaUnitDef, u);
  }
  updateDiffusion();
  updateReactions();
}

const Unit &ModelUnits::getVolume() const { return volume.get(); }

int ModelUnits::getVolumeIndex() const { return volume.getIndex(); }

const QVector<Unit> &ModelUnits::getVolumeUnits() const {
  return volume.getUnits();
}

QVector<Unit> &ModelUnits::getVolumeUnits() { return volume.getUnits(); }

void ModelUnits::setVolumeIndex(int index) {
  hasUnsavedChanges = true;
  volume.setIndex(index);
  if (sbmlModel != nullptr) {
    auto *volumeUnitDef = getOrCreateUnitDef(
        sbmlModel, sbmlModel->getVolumeUnits(), "unit_of_volume");
    sbmlModel->setVolumeUnits(volumeUnitDef->getId());
    setSBMLUnitDef(volumeUnitDef, getVolume());
  }
  updateConcentration();
  updateReactions();
}

const Unit &ModelUnits::getAmount() const { return amount.get(); }

int ModelUnits::getAmountIndex() const { return amount.getIndex(); }

const QVector<Unit> &ModelUnits::getAmountUnits() const {
  return amount.getUnits();
}

QVector<Unit> &ModelUnits::getAmountUnits() { return amount.getUnits(); }

void ModelUnits::setAmountIndex(int index) {
  hasUnsavedChanges = true;
  amount.setIndex(index);
  if (sbmlModel != nullptr) {
    auto *substanceUnitDef = getOrCreateUnitDef(
        sbmlModel, sbmlModel->getSubstanceUnits(), "unit_of_substance");
    sbmlModel->setSubstanceUnits(substanceUnitDef->getId());
    sbmlModel->setExtentUnits(substanceUnitDef->getId());
    setSBMLUnitDef(substanceUnitDef, getAmount());
  }
  updateConcentration();
  updateReactions();
}

const QString &ModelUnits::getConcentration() const { return concentration; }

const QString &ModelUnits::getDiffusion() const { return diffusion; }

const QString &ModelUnits::getCompartmentReaction() const {
  return compartmentReaction;
}

const QString &ModelUnits::getMembraneReaction() const {
  return membraneReaction;
}

bool ModelUnits::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelUnits::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

double rescale(double val, const Unit &oldUnit, const Unit &newUnit) {
  // rescale units, assuming same base unit & unit exponent
  // e.g. m -> cm, or mol -> mmol
  return (oldUnit.multiplier / newUnit.multiplier) *
         std::pow(10, oldUnit.scale - newUnit.scale) * val;
}

static double getV(const Unit &volumeUnit) {
  if (volumeUnit.kind == "metre") {
    return std::pow(volumeUnit.multiplier * std::pow(10, volumeUnit.scale),
                    volumeUnit.exponent);
  } else if (volumeUnit.kind == "litre") {
    // 1 L = 1e-3 m^3 --> subtract 3 from scale:
    return volumeUnit.multiplier * std::pow(10, volumeUnit.scale - 3);
  }
  SPDLOG_WARN("unsupported Volume base unit: '{}'",
              volumeUnit.kind.toStdString());
  return 1;
}

static double getL3(const Unit &lengthUnit) {
  if (lengthUnit.kind == "metre") {
    return std::pow(lengthUnit.multiplier * std::pow(10, lengthUnit.scale), 3);
  }
  SPDLOG_WARN("unsupported Length base unit: '{}'",
              lengthUnit.kind.toStdString());
  return 1;
}

double getVolOverL3(const Unit &lengthUnit, const Unit &volumeUnit) {
  return getV(volumeUnit) / getL3(lengthUnit);
}

} // namespace sme::model
