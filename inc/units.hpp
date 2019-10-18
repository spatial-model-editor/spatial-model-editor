// units

#pragma once

#include <QString>
#include <QVector>
#include <cmath>

namespace units {

struct Unit {
  // UI display name
  QString name;
  // UI display symbol
  QString symbol;
  // SBML unit definition: name == (multiplier * 10^scale * kind)^exponent
  QString kind;
  int scale;
  int exponent = 1;
  double multiplier = 1.0;
};

struct UnitVector {
  QVector<Unit> units;
  int index = 0;
  inline const Unit& get() const { return units.at(index); }
};

struct ModelUnits {
  UnitVector time;
  UnitVector length;
  UnitVector volume;
  UnitVector amount;
};

inline double rescale(double val, const Unit& oldUnit, const Unit& newUnit) {
  // rescale units, assuming same base unit & same exponent
  // e.g. m -> cm, or mol -> mmol
  return (oldUnit.multiplier / newUnit.multiplier) *
         std::pow(10, oldUnit.scale - newUnit.scale) * val;
}

// convert pixel width to pixel volume (with unit length in third dimension)
inline double pixelWidthToVolume(double width, const Unit& lengthUnit,
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

}  // namespace units
