// SBML math

#pragma once

#include <QImage>
#include <QRgb>
#include <string>
#include <vector>

namespace libsbml {
class Geometry;
class SampledFieldGeometry;
} // namespace libsbml

namespace sbml {

struct GeometrySampledField {
  libsbml::SampledFieldGeometry *sampledFieldGeometry;
  QImage image;
  std::vector<std::pair<std::string, QRgb>> compartmentIdColourPairs;
};

GeometrySampledField
importGeometryFromSampledField(const libsbml::Geometry *geom);

} // namespace sbml
