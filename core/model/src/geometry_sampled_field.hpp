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

/**
 * @brief Sampled-field geometry image data and compartment color mapping.
 */
struct GeometrySampledField {
  /**
   * @brief One image per z slice.
   */
  std::vector<QImage> images;
  /**
   * @brief Mapping from compartment id to display color.
   */
  std::vector<std::pair<std::string, QRgb>> compartmentIdColorPairs;
};

/**
 * @brief Return existing sampled-field geometry or create one if needed.
 */
libsbml::SampledFieldGeometry *
getOrCreateSampledFieldGeometry(libsbml::Geometry *geom);

/**
 * @brief Import sampled-field geometry from SBML.
 */
GeometrySampledField importGeometryFromSampledField(
    const libsbml::Geometry *geom,
    const std::vector<QRgb> &importedSampledFieldColors);

/**
 * @brief Export sampled-field geometry images into SBML.
 */
void exportSampledFieldGeometry(libsbml::Geometry *geom,
                                const common::ImageStack &compartmentImages);

} // namespace sme::model
