// SBML xml legacy annotation read/write
// This was used for versions <= v1.0.9
// It is still used for importing models created using these older versions,
// during which process the legacy annotations are removed from the sbml

#pragma once

#include "model_settings.hpp"
#include "simulate_options.hpp"
#include "serialization.hpp"
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
class Mesh;
}

namespace model {

bool hasLegacyAnnotations(const libsbml::Model *model);
Settings importAndRemoveLegacyAnnotations(libsbml::Model *model);

void removeMeshParamsAnnotation(libsbml::ParametricGeometry *pg);
std::optional<MeshParameters>
getMeshParamsAnnotationData(const libsbml::ParametricGeometry *pg);

void removeSpeciesColourAnnotation(libsbml::Species *species);
std::optional<QRgb> getSpeciesColourAnnotation(const libsbml::Species *species);

void removeDisplayOptionsAnnotation(libsbml::Model *model);
std::optional<model::DisplayOptions>
getDisplayOptionsAnnotation(const libsbml::Model *model);

} // namespace model

} // namespace sme
