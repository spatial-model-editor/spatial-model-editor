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

GeometrySampledField
importGeometryFromAnalyticGeometry(const libsbml::Model *model,
                                   const common::VoxelF &physicalOrigin,
                                   const common::VolumeF &physicalSize);

} // namespace sme::model
