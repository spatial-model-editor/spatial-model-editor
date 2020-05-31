#include "xml_annotation.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <fmt/core.h>
#include <memory>

#include "logger.hpp"
#include "mesh.hpp"
#include "utils.hpp"

static const std::string annotationURI{
    "https://github.com/lkeegan/spatial-model-editor"};

static const std::string annotationPrefix{"spatialModelEditor"};

static const std::string annotationNameMesh{"mesh"};

namespace sbml {

static const libsbml::XMLNode *
getAnnotation(libsbml::SBase *parent, const std::string &annotationName) {
  if (parent == nullptr || !parent->isSetAnnotation()) {
    return nullptr;
  }
  auto *node = parent->getAnnotation();
  for (unsigned i = 0; i < node->getNumChildren(); ++i) {
    const auto &child = node->getChild(i);
    if (child.getURI() == annotationURI &&
        child.getPrefix() == annotationPrefix &&
        child.getName() == annotationName) {
      return &child;
    }
  }
  return nullptr;
}

static void removeAnnotation(libsbml::SBase *parent,
                             const std::string &annotationName) {
  if (parent == nullptr || !parent->isSetAnnotation()) {
    return;
  }
  auto *node = parent->getAnnotation();
  for (unsigned i = 0; i < node->getNumChildren(); ++i) {
    if (const auto &child = node->getChild(i);
        child.getURI() == annotationURI &&
        child.getPrefix() == annotationPrefix &&
        child.getName() == annotationName) {
      std::unique_ptr<libsbml::XMLNode> n(node->removeChild(i));
      SPDLOG_INFO("removed annotation {} : '{}'", i, n->toXMLString());
      return;
    }
  }
}

void removeMeshParamsAnnotation(libsbml::ParametricGeometry *pg) {
  removeAnnotation(pg, annotationNameMesh);
}

void addMeshParamsAnnotation(libsbml::ParametricGeometry *pg,
                             const mesh::Mesh *mesh) {
  if (mesh == nullptr) {
    return;
  }
  // if there is already an annotation set by us, remove it
  removeAnnotation(pg, annotationNameMesh);
  // append annotation with mesh info
  SPDLOG_CRITICAL("<{0}:{2} xmlns:{0}=\"{1}\" {0}:maxBoundaryPoints=\"{3}\"",
                  annotationPrefix, annotationURI, annotationNameMesh,
                  utils::vectorToString(mesh->getBoundaryMaxPoints()));
  std::string xml = "<";
  xml.append(annotationPrefix);
  xml.append(":");
  xml.append(annotationNameMesh);
  xml.append(" xmlns:");
  xml.append(annotationPrefix);
  xml.append("=\"");
  xml.append(annotationURI);
  xml.append("\" ");
  xml.append(annotationPrefix);
  xml.append(":maxBoundaryPoints=\"");
  xml.append(utils::vectorToString(mesh->getBoundaryMaxPoints()));
  xml.append("\" ");
  xml.append(annotationPrefix);
  xml.append(":maxTriangleAreas=\"");
  xml.append(utils::vectorToString(mesh->getCompartmentMaxTriangleArea()));
  xml.append("\" ");
  xml.append(annotationPrefix);
  xml.append(":membraneWidths=\"");
  xml.append(utils::vectorToString(mesh->getBoundaryWidths()));
  xml.append("\"/>");
  pg->appendAnnotation(xml);
  SPDLOG_INFO("appending annotation: {}", xml);
}

std::optional<MeshParamsAnnotationData>
getMeshParamsAnnotationData(libsbml::ParametricGeometry *pg) {
  std::optional<MeshParamsAnnotationData> dat = {};
  if (auto *node = getAnnotation(pg, "mesh"); node != nullptr) {
    auto &d = dat.emplace();
    d.maxPoints = utils::stringToVector<std::size_t>(
        node->getAttrValue("maxBoundaryPoints", annotationURI));
    SPDLOG_INFO("  - maxBoundaryPoints: {}",
                utils::vectorToString(d.maxPoints));
    d.maxAreas = utils::stringToVector<std::size_t>(
        node->getAttrValue("maxTriangleAreas", annotationURI));
    SPDLOG_INFO("  - maxTriangleAreas: {}", utils::vectorToString(d.maxAreas));
    d.membraneWidths = utils::stringToVector<double>(
        node->getAttrValue("membraneWidths", annotationURI));
    SPDLOG_INFO("  - membraneWidths: {}",
                utils::vectorToString(d.membraneWidths));
  }
  return dat;
}

} // namespace sbml
