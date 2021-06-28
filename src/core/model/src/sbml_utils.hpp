// Utilities for working with SBML

#pragma once

#include "sbml_utils.hpp"
#include <optional>
#include <sbml/SBMLTypes.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <string>
#include <utility>

namespace libsbml {
class Geometry;
class Model;
class SampledFieldGeometry;
class Species;
class Parameter;
} // namespace libsbml

namespace sme::model {

const libsbml::SampledFieldGeometry *
getSampledFieldGeometry(const libsbml::Geometry *geom);
libsbml::SampledFieldGeometry *getSampledFieldGeometry(libsbml::Geometry *geom);
const libsbml::Geometry *getGeometry(const libsbml::Model *model);
libsbml::Geometry *getOrCreateGeometry(libsbml::Model *model);
void createDefaultCompartmentGeometryIfMissing(libsbml::Model *model);

unsigned int getNumSpatialDimensions(const libsbml::Model *model);

std::string getDomainIdFromCompartmentId(const libsbml::Model *model,
                                         const std::string &compartmentId);

std::optional<std::pair<std::string, std::string>>
getAdjacentCompartments(const libsbml::Model *model,
                        const std::string &compartmentId);

bool getIsSpeciesConstant(const libsbml::Species *spec);

const libsbml::Parameter *
getSpatialCoordinateParam(const libsbml::Model *model,
                          libsbml::CoordinateKind_t kind);

libsbml::Parameter *getSpatialCoordinateParam(libsbml::Model *model,
                                              libsbml::CoordinateKind_t kind);

} // namespace sme
