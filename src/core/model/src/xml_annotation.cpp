#include "xml_annotation.hpp"

#include <fmt/core.h>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <memory>

#include "logger.hpp"
#include "mesh.hpp"
#include "utils.hpp"

static const std::string annotationURI{
    "https://github.com/lkeegan/spatial-model-editor"};
static const std::string annotationPrefix{"spatialModelEditor"};
static const std::string annotationNameMesh{"mesh"};
static const std::string annotationNameColour{"colour"};

namespace model {

static const libsbml::XMLNode *
getAnnotation(const libsbml::SBase *parent, const std::string &annotationName) {
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
  auto xml = fmt::format(
      "<{prefix}:{name} xmlns:{prefix}=\"{uri}\" "
      "{prefix}:maxBoundaryPoints=\"{points}\" "
      "{prefix}:maxTriangleAreas=\"{areas}\" "
      "{prefix}:membraneWidths=\"{widths}\" />",
      fmt::arg("prefix", annotationPrefix), fmt::arg("uri", annotationURI),
      fmt::arg("name", annotationNameMesh),
      fmt::arg("points", utils::vectorToString(mesh->getBoundaryMaxPoints())),
      fmt::arg("areas",
               utils::vectorToString(mesh->getCompartmentMaxTriangleArea())),
      fmt::arg("widths", utils::vectorToString(mesh->getBoundaryWidths())));
  pg->appendAnnotation(xml);
  SPDLOG_INFO("appending annotation: {}", xml);
}

std::optional<MeshParamsAnnotationData>
getMeshParamsAnnotationData(const libsbml::ParametricGeometry *pg) {
  std::optional<MeshParamsAnnotationData> dat = {};
  if (const auto *node = getAnnotation(pg, annotationNameMesh);
      node != nullptr) {
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

void removeSpeciesColourAnnotation(libsbml::Species *species) {
  removeAnnotation(species, annotationNameColour);
}

void addSpeciesColourAnnotation(libsbml::Species *species, QRgb colour) {
  if (species == nullptr) {
    return;
  }
  // if there is already an annotation set by us, remove it
  removeAnnotation(species, annotationNameColour);
  // append annotation with colour
  auto xml = fmt::format(
      "<{prefix}:{name} xmlns:{prefix}=\"{uri}\" "
      "{prefix}:colour=\"{colour}\" />",
      fmt::arg("prefix", annotationPrefix), fmt::arg("uri", annotationURI),
      fmt::arg("name", annotationNameColour), fmt::arg("colour", colour));
  species->appendAnnotation(xml);
  SPDLOG_INFO("Species: {}", species->getId());
  SPDLOG_INFO("  - appending annotation: {}", xml);
}

std::optional<QRgb>
getSpeciesColourAnnotation(const libsbml::Species *species) {
  if (const auto *node = getAnnotation(species, annotationNameColour);
      node != nullptr) {
    auto colour = utils::stringToVector<QRgb>(
        node->getAttrValue("colour", annotationURI))[0];
    SPDLOG_INFO("Species: {}", species->getId());
    SPDLOG_INFO("  - colour: {:x}", colour);
    return colour;
  }
  return {};
}

} // namespace model
