// SBML xml legacy annotation read/write
// This was used for versions <= v1.0.9
// It is still used for importing models created using these older versions,
// during which process the legacy annotations are removed from the sbml

#pragma once

#include "sme/model_settings.hpp"
#include "sme/serialization.hpp"
#include "sme/simulate_options.hpp"
#include <QRgb>
#include <optional>
#include <string>
#include <vector>

namespace libsbml {
class ParametricGeometry;
class Species;
class Model;
} // namespace libsbml

namespace sme {

namespace mesh {
class Mesh2d;
}

namespace model {

/**
 * @brief Returns ``true`` if legacy SME annotations are present.
 */
bool hasLegacyAnnotations(const libsbml::Model *model);
/**
 * @brief Import legacy annotations into current settings and remove them.
 */
Settings importAndRemoveLegacyAnnotations(libsbml::Model *model);

/**
 * @brief Remove legacy mesh-parameter annotation from parametric geometry.
 */
void removeMeshParamsAnnotation(libsbml::ParametricGeometry *pg);
/**
 * @brief Parse legacy mesh-parameter annotation.
 */
std::optional<MeshParameters>
getMeshParamsAnnotationData(const libsbml::ParametricGeometry *pg);

/**
 * @brief Remove legacy species color annotation.
 */
void removeSpeciesColorAnnotation(libsbml::Species *species);
/**
 * @brief Parse legacy species color annotation.
 */
std::optional<QRgb> getSpeciesColorAnnotation(const libsbml::Species *species);

/**
 * @brief Remove legacy display-options annotation.
 */
void removeDisplayOptionsAnnotation(libsbml::Model *model);
/**
 * @brief Parse legacy display-options annotation.
 */
std::optional<model::DisplayOptions>
getDisplayOptionsAnnotation(const libsbml::Model *model);

} // namespace model

} // namespace sme
