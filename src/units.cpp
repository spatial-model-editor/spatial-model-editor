#include "units.hpp"

#include <cmath>

namespace units {

UnitVector::UnitVector(const QVector<Unit>& unitsVec) : units(unitsVec) {}

const Unit& UnitVector::get() const { return units.at(index); }

const QVector<Unit>& UnitVector::getUnits() const { return units; }

int UnitVector::getIndex() const { return index; }

void UnitVector::setIndex(int newIndex) { index = newIndex; }

void ModelUnits::updateConcentration() {
  concentration =
      QString("%1/%2").arg(getAmount().symbol).arg(getVolume().symbol);
}

void ModelUnits::updateDiffusion() {
  diffusion = QString("%1^2/%2").arg(getLength().symbol).arg(getTime().symbol);
}

ModelUnits::ModelUnits(const UnitVector& timeUnits,
                       const UnitVector& lengthUnits,
                       const UnitVector& volumeUnits,
                       const UnitVector& amountUnits)
    : time(timeUnits),
      length(lengthUnits),
      volume(volumeUnits),
      amount(amountUnits) {
  updateConcentration();
  updateDiffusion();
}

const Unit& ModelUnits::getTime() const { return time.get(); }

int ModelUnits::getTimeIndex() const { return time.getIndex(); }

const QVector<Unit>& ModelUnits::getTimeUnits() const {
  return time.getUnits();
}

void ModelUnits::setTime(int index) {
  time.setIndex(index);
  updateDiffusion();
}

const Unit& ModelUnits::getLength() const { return length.get(); }

int ModelUnits::getLengthIndex() const { return length.getIndex(); }

const QVector<Unit>& ModelUnits::getLengthUnits() const {
  return length.getUnits();
}

void ModelUnits::setLength(int index) {
  length.setIndex(index);
  updateDiffusion();
}

const Unit& ModelUnits::getVolume() const { return volume.get(); }

int ModelUnits::getVolumeIndex() const { return volume.getIndex(); }

const QVector<Unit>& ModelUnits::getVolumeUnits() const {
  return volume.getUnits();
}

void ModelUnits::setVolume(int index) {
  volume.setIndex(index);
  updateConcentration();
}

const Unit& ModelUnits::getAmount() const { return amount.get(); }

int ModelUnits::getAmountIndex() const { return amount.getIndex(); }

const QVector<Unit>& ModelUnits::getAmountUnits() const {
  return amount.getUnits();
}

void ModelUnits::setAmount(int index) {
  amount.setIndex(index);
  updateConcentration();
}

const QString& ModelUnits::getConcentration() const { return concentration; }

const QString& ModelUnits::getDiffusion() const { return diffusion; }

double rescale(double val, const Unit& oldUnit, const Unit& newUnit) {
  // rescale units, assuming same base unit & same exponent
  // e.g. m -> cm, or mol -> mmol
  return (oldUnit.multiplier / newUnit.multiplier) *
         std::pow(10, oldUnit.scale - newUnit.scale) * val;
}

// convert pixel width to pixel volume (with unit length in third dimension)
double pixelWidthToVolume(double width, const Unit& lengthUnit,
                          const Unit& volumeUnit) {
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

};  // namespace units
