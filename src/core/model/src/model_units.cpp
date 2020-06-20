#include "model_units.hpp"
#include "logger.hpp"
#include <cmath>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace model {

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
  return;
}

static std::optional<int> getUnitIndex(libsbml::Model *model,
                                       const std::string &id,
                                       const QVector<Unit> &units) {
  if (model == nullptr) {
    return {};
  }
  SPDLOG_INFO("SId: {}", id);
  // by default assume id is a SBML base unit
  std::string kind = id;
  double multiplier = 1.0;
  int exponent = 1;
  int scale = 0;
  const auto *unitdef = model->getUnitDefinition(id);
  if (unitdef != nullptr && unitdef->getNumUnits() == 1) {
    // if id is a UnitDefinition, then get unit kind & scaling factors
    const auto *unit = unitdef->getUnit(0);
    kind = libsbml::UnitKind_toString(unit->getKind());
    multiplier = unit->getMultiplier();
    exponent = unit->getExponent();
    scale = unit->getScale();
  }
  SPDLOG_INFO("  = ({} * 1e{} {})^{}", multiplier, scale, kind, exponent);
  for (int i = 0; i < units.size(); ++i) {
    const auto &u = units.at(i);
    if (u.kind.toStdString() == kind &&
        std::fabs((u.multiplier - multiplier) / multiplier) < 1e-10 &&
        u.exponent == exponent && u.scale == scale) {
      SPDLOG_INFO("  -> {}", u.name.toStdString());
      return i;
    }
  }
  SPDLOG_WARN("  -> matching unit not found");
  return {};
}

UnitVector::UnitVector(const QVector<Unit> &unitsVec, int defaultIndex)
    : units{unitsVec}, index{defaultIndex} {}

const Unit &UnitVector::get() const { return units.at(index); }

const QVector<Unit> &UnitVector::getUnits() const { return units; }

int UnitVector::getIndex() const { return index; }

void UnitVector::setIndex(int newIndex) { index = newIndex; }

void ModelUnits::updateConcentration() {
  concentration =
      QString("%1/%2").arg(getAmount().symbol).arg(getVolume().symbol);
}

void ModelUnits::updateDiffusion() {
  diffusion = QString("%1^2/%2").arg(getLength().symbol).arg(getTime().symbol);
}

ModelUnits::ModelUnits(libsbml::Model *model) : sbmlModel{model} {
  setTimeIndex(getUnitIndex(model, model->getTimeUnits(), time.getUnits())
                   .value_or(time.getIndex()));
  setLengthIndex(getUnitIndex(model, model->getLengthUnits(), length.getUnits())
                     .value_or(length.getIndex()));
  setVolumeIndex(getUnitIndex(model, model->getVolumeUnits(), volume.getUnits())
                     .value_or(volume.getIndex()));
  setAmountIndex(
      getUnitIndex(model, model->getSubstanceUnits(), amount.getUnits())
          .value_or(amount.getIndex()));
  updateConcentration();
  updateDiffusion();
}

const Unit &ModelUnits::getTime() const { return time.get(); }

int ModelUnits::getTimeIndex() const { return time.getIndex(); }

const QVector<Unit> &ModelUnits::getTimeUnits() const {
  return time.getUnits();
}

void ModelUnits::setTimeIndex(int index) {
  time.setIndex(index);
  if (sbmlModel != nullptr) {
    auto *timeUnitDef = getOrCreateUnitDef(sbmlModel, sbmlModel->getTimeUnits(),
                                           "unit_of_time");
    sbmlModel->setTimeUnits(timeUnitDef->getId());
    setSBMLUnitDef(timeUnitDef, getTime());
  }
  updateDiffusion();
}

const Unit &ModelUnits::getLength() const { return length.get(); }

int ModelUnits::getLengthIndex() const { return length.getIndex(); }

const QVector<Unit> &ModelUnits::getLengthUnits() const {
  return length.getUnits();
}

void ModelUnits::setLengthIndex(int index) {
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
    u.name.append(" squared");
    u.exponent *= 2;
    sbmlModel->setAreaUnits(areaUnitDef->getId());
    setSBMLUnitDef(areaUnitDef, u);
  }
  updateDiffusion();
}

const Unit &ModelUnits::getVolume() const { return volume.get(); }

int ModelUnits::getVolumeIndex() const { return volume.getIndex(); }

const QVector<Unit> &ModelUnits::getVolumeUnits() const {
  return volume.getUnits();
}

void ModelUnits::setVolumeIndex(int index) {
  volume.setIndex(index);
  if (sbmlModel != nullptr) {
    auto *volumeUnitDef = getOrCreateUnitDef(
        sbmlModel, sbmlModel->getVolumeUnits(), "unit_of_volume");
    sbmlModel->setVolumeUnits(volumeUnitDef->getId());
    setSBMLUnitDef(volumeUnitDef, getVolume());
  }
  updateConcentration();
}

const Unit &ModelUnits::getAmount() const { return amount.get(); }

int ModelUnits::getAmountIndex() const { return amount.getIndex(); }

const QVector<Unit> &ModelUnits::getAmountUnits() const {
  return amount.getUnits();
}

void ModelUnits::setAmountIndex(int index) {
  amount.setIndex(index);
  if (sbmlModel != nullptr) {
    auto *substanceUnitDef = getOrCreateUnitDef(
        sbmlModel, sbmlModel->getSubstanceUnits(), "unit_of_substance");
    sbmlModel->setSubstanceUnits(substanceUnitDef->getId());
    sbmlModel->setExtentUnits(substanceUnitDef->getId());
    setSBMLUnitDef(substanceUnitDef, getAmount());
  }
  updateConcentration();
}

const QString &ModelUnits::getConcentration() const { return concentration; }

const QString &ModelUnits::getDiffusion() const { return diffusion; }

double rescale(double val, const Unit &oldUnit, const Unit &newUnit) {
  // rescale units, assuming same base unit & same exponent
  // e.g. m -> cm, or mol -> mmol
  return (oldUnit.multiplier / newUnit.multiplier) *
         std::pow(10, oldUnit.scale - newUnit.scale) * val;
}

// convert pixel width to pixel volume (with unit length in third dimension)
double pixelWidthToVolume(double width, const Unit &lengthUnit,
                          const Unit &volumeUnit) {
  // assume base unit of length is always metre
  // length^3 unit in m^3:
  double l3 = std::pow(lengthUnit.multiplier * std::pow(10, lengthUnit.scale),
                       3 * lengthUnit.exponent);
  // volume unit in base volume unit:
  double v = std::pow(volumeUnit.multiplier * std::pow(10, volumeUnit.scale),
                      volumeUnit.exponent);
  // if base volume unit is litre, not m^3, need to should convert l3 from m^3
  // to litre, i.e. multiply by 1e3
  if (volumeUnit.kind == "litre") {
    l3 *= 1000;
  }
  return width * width * l3 / v;
}

} // namespace model
