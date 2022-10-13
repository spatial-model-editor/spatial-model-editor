// SBML SampledFieldGeometry
//   - import/export geometry to/from SBML sampled field geometry

#pragma once

#include "sme/image_stack.hpp"
#include <QImage>
#include <QRgb>
#include <string>
#include <vector>

namespace libsbml {
class Geometry;
class SampledFieldGeometry;
} // namespace libsbml

namespace sme::model {

struct GeometrySampledField {
  std::vector<QImage> images;
  std::vector<std::pair<std::string, QRgb>> compartmentIdColourPairs;
};

libsbml::SampledFieldGeometry *
getOrCreateSampledFieldGeometry(libsbml::Geometry *geom);

GeometrySampledField importGeometryFromSampledField(
    const libsbml::Geometry *geom,
    const std::vector<QRgb> &importedSampledFieldColours);

void exportSampledFieldGeometry(libsbml::Geometry *geom,
                                const common::ImageStack &compartmentImages);

} // namespace sme::model
