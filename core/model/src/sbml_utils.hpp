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

/**
 * @brief Get sampled-field geometry from immutable geometry.
 */
const libsbml::SampledFieldGeometry *
getSampledFieldGeometry(const libsbml::Geometry *geom);
/**
 * @brief Get sampled-field geometry from mutable geometry.
 */
libsbml::SampledFieldGeometry *getSampledFieldGeometry(libsbml::Geometry *geom);
/**
 * @brief Get model geometry object (nullable).
 */
const libsbml::Geometry *getGeometry(const libsbml::Model *model);
/**
 * @brief Get or create model geometry object.
 */
libsbml::Geometry *getOrCreateGeometry(libsbml::Model *model);
/**
 * @brief Ensure default geometry exists for non-spatial compartment models.
 */
void createDefaultCompartmentGeometryIfMissing(libsbml::Model *model);

/**
 * @brief Number of spatial dimensions declared by the model.
 */
unsigned int getNumSpatialDimensions(const libsbml::Model *model);

/**
 * @brief Lookup domain id associated with compartment id.
 */
std::string getDomainIdFromCompartmentId(const libsbml::Model *model,
                                         const std::string &compartmentId);

/**
 * @brief Compartments adjacent to the given compartment, if uniquely defined.
 */
std::optional<std::pair<std::string, std::string>>
getAdjacentCompartments(const libsbml::Model *model,
                        const std::string &compartmentId);

/**
 * @brief Return whether species is marked constant in SBML/spatial context.
 */
bool getIsSpeciesConstant(const libsbml::Species *spec);

/**
 * @brief Get spatial coordinate parameter (x/y/z) from immutable model.
 */
const libsbml::Parameter *
getSpatialCoordinateParam(const libsbml::Model *model,
                          libsbml::CoordinateKind_t kind);

/**
 * @brief Get spatial coordinate parameter (x/y/z) from mutable model.
 */
libsbml::Parameter *getSpatialCoordinateParam(libsbml::Model *model,
                                              libsbml::CoordinateKind_t kind);
} // namespace sme::model
