// SBML xml annotation read/write

#pragma once

#include <string>
#include <optional>
#include <vector>

namespace libsbml {
class ParametricGeometry;
}

namespace mesh {
class Mesh;
}

namespace sbml {

struct MeshParamsAnnotationData{
    std::vector<std::size_t> maxPoints;
    std::vector<std::size_t> maxAreas;
    std::vector<double> membraneWidths;
};

void removeMeshParamsAnnotation(libsbml::ParametricGeometry *pg);
void addMeshParamsAnnotation(
    libsbml::ParametricGeometry *pg, const mesh::Mesh* mesh);
std::optional<MeshParamsAnnotationData> getMeshParamsAnnotationData(libsbml::ParametricGeometry *pg);

}  // namespace sbml
