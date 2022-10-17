// SBML AnalyticGeometry
//   - import analytic geometry from spatial SBML model
//   - convert to a sampled field geometry

#pragma once

#include "sme/geometry_utils.hpp"
#include <QImage>

namespace libsbml {
class Model;
}

namespace sme::model {

struct GeometrySampledField;

GeometrySampledField
importGeometryFromAnalyticGeometry(const libsbml::Model *model,
                                   const geometry::VoxelF &physicalOrigin,
                                   const geometry::VSizeF &physicalSize);

} // namespace sme::model
