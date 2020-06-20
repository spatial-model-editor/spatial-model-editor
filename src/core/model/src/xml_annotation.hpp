// SBML xml annotation read/write

#pragma once

#include <QRgb>
#include <optional>
#include <string>
#include <vector>

namespace libsbml {
class ParametricGeometry;
class Species;
}  // namespace libsbml

namespace mesh {
class Mesh;
}

namespace model {

struct MeshParamsAnnotationData {
  std::vector<std::size_t> maxPoints;
  std::vector<std::size_t> maxAreas;
  std::vector<double> membraneWidths;
};

void removeMeshParamsAnnotation(libsbml::ParametricGeometry *pg);
void addMeshParamsAnnotation(libsbml::ParametricGeometry *pg,
                             const mesh::Mesh *mesh);
std::optional<MeshParamsAnnotationData> getMeshParamsAnnotationData(
    const libsbml::ParametricGeometry *pg);

void removeSpeciesColourAnnotation(libsbml::Species *species);
void addSpeciesColourAnnotation(libsbml::Species *species, QRgb colour);
std::optional<QRgb> getSpeciesColourAnnotation(const libsbml::Species *species);

}  // namespace sbml
