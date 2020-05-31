// SBML SampledFieldGeometry
//   - import geometry from SBML sampled field geometry
//   -

#pragma once

#include <QImage>
#include <QRgb>
#include <string>
#include <vector>

namespace libsbml {
class Geometry;
class SampledFieldGeometry;
} // namespace libsbml

namespace model {

struct GeometrySampledField {
  QImage image;
  std::vector<std::pair<std::string, QRgb>> compartmentIdColourPairs;
};

libsbml::SampledFieldGeometry *
getOrCreateSampledFieldGeometry(libsbml::Geometry *geom);

GeometrySampledField importGeometryFromSampledField(libsbml::Geometry *geom);

void exportSampledFieldGeometry(libsbml::Geometry *geom,
                                const QImage &compartmentImage);

} // namespace sbml
