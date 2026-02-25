// SBML AnalyticGeometry
//   - import analytic geometry from spatial SBML model
//   - convert to a sampled field geometry

#pragma once

#include "sme/image_stack.hpp"
#include <QImage>

namespace libsbml {
class Model;
}

namespace sme::model {

struct GeometrySampledField;

/**
 * @brief Returns ``true`` if SBML model contains AnalyticGeometry.
 */
[[nodiscard]] bool hasAnalyticGeometry(const libsbml::Model *model);

/**
 * @brief Compute default raster image size for analytic geometry sampling.
 */
[[nodiscard]] common::Volume
getDefaultAnalyticGeometryImageSize(const common::VolumeF &physicalSize,
                                    int maxDimension = 50);

/**
 * @brief Import analytic geometry into sampled-field representation.
 *
 * Uses an automatically chosen image size.
 */
GeometrySampledField
importGeometryFromAnalyticGeometry(const libsbml::Model *model,
                                   const common::VoxelF &physicalOrigin,
                                   const common::VolumeF &physicalSize);

/**
 * @brief Import analytic geometry into sampled-field representation.
 *
 * Uses an explicit target image size.
 */
GeometrySampledField importGeometryFromAnalyticGeometry(
    const libsbml::Model *model, const common::VoxelF &physicalOrigin,
    const common::VolumeF &physicalSize, const common::Volume &imageSize);

} // namespace sme::model
