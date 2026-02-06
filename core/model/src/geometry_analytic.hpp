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

[[nodiscard]] bool hasAnalyticGeometry(const libsbml::Model *model);

[[nodiscard]] common::Volume
getDefaultAnalyticGeometryImageSize(const common::VolumeF &physicalSize,
                                    int maxDimension = 50);

GeometrySampledField
importGeometryFromAnalyticGeometry(const libsbml::Model *model,
                                   const common::VoxelF &physicalOrigin,
                                   const common::VolumeF &physicalSize);

GeometrySampledField importGeometryFromAnalyticGeometry(
    const libsbml::Model *model, const common::VoxelF &physicalOrigin,
    const common::VolumeF &physicalSize, const common::Volume &imageSize);

} // namespace sme::model
