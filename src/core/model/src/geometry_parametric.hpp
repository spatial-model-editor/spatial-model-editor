// SBML ParametricGeometry
//   - import/export parametric geometry

#pragma once

#include <QImage>
#include <QRgb>
#include <memory>
#include <string>
#include <vector>

namespace libsbml {
class Model;
class Geometry;
class ParametricGeometry;
class ParametricObject;
} // namespace libsbml

namespace sme {

namespace mesh {
class Mesh;
}

namespace model {

class ModelCompartments;
class ModelGeometry;
class ModelMembranes;
struct Settings;

const libsbml::ParametricGeometry *
getParametricGeometry(const libsbml::Geometry *geom);

libsbml::ParametricGeometry *getParametricGeometry(libsbml::Geometry *geom);

const libsbml::ParametricObject *
getParametricObject(const libsbml::Model *model,
                    const std::string &compartmentID);

libsbml::ParametricObject *
getOrCreateParametricObject(libsbml::Model *model,
                            const std::string &compartmentID);

std::vector<std::vector<QPointF>>
getInteriorPixelPoints(const ModelGeometry *modelGeometry,
                       const ModelCompartments *modelCompartments);

std::unique_ptr<mesh::Mesh>
importParametricGeometryFromSBML(const libsbml::Model *model,
                                 const ModelGeometry *modelGeometry,
                                 const ModelCompartments *modelCompartments,
                                 const Settings *annotation);

void writeGeometryMeshToSBML(libsbml::Model *model, const mesh::Mesh *mesh,
                             const ModelCompartments &modelCompartments);

} // namespace model

} // namespace sme
