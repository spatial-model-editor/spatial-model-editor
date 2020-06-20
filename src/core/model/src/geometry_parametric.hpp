// SBML ParametricGeometry
//   - import/export parametric geometry

#pragma once

#include <QImage>
#include <QRgb>
#include <memory>
#include <string>
#include <vector>

namespace mesh {
class Mesh;
}

namespace libsbml {
class Model;
class Geometry;
class ParametricGeometry;
class ParametricObject;
}  // namespace libsbml

namespace model {

class ModelCompartments;
class ModelGeometry;
class ModelMembranes;

libsbml::ParametricGeometry *getParametricGeometry(libsbml::Geometry *geom);

libsbml::ParametricObject *getOrCreateParametricObject(
    libsbml::Model *model, const std::string &compartmentID);

std::vector<QPointF> getInteriorPixelPoints(
    const ModelGeometry *modelGeometry,
    const ModelCompartments *modelCompartments);

std::unique_ptr<mesh::Mesh> importParametricGeometryFromSBML(
    libsbml::Model *model, const ModelGeometry *modelGeometry,
    const ModelCompartments *modelCompartments,
    const ModelMembranes *modelMembranes);

void writeGeometryMeshToSBML(libsbml::Model *model, const mesh::Mesh *mesh,
                             const ModelCompartments &modelCompartments);

}  // namespace model
