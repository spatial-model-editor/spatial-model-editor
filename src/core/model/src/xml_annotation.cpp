#include "xml_annotation.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "utils.hpp"
#include <fmt/core.h>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

static const std::string annotationURI{
    "https://github.com/lkeegan/spatial-model-editor"};
static const std::string annotationPrefix{"spatialModelEditor"};
static const std::string annotationNameMesh{"mesh"};
static const std::string annotationNameColour{"colour"};
static const std::string annotationNameDisplayOptions{"displayOptions"};

namespace sme {

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
  removeMeshParamsAnnotation(pg);
  // append annotation with mesh info
  auto xml = fmt::format(
      "<{prefix}:{name} xmlns:{prefix}=\"{uri}\" "
      "{prefix}:maxBoundaryPoints=\"{points}\" "
      "{prefix}:maxTriangleAreas=\"{areas}\" />",
      fmt::arg("prefix", annotationPrefix), fmt::arg("uri", annotationURI),
      fmt::arg("name", annotationNameMesh),
      fmt::arg("points", utils::vectorToString(mesh->getBoundaryMaxPoints())),
      fmt::arg("areas",
               utils::vectorToString(mesh->getCompartmentMaxTriangleArea())));
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
  removeSpeciesColourAnnotation(species);
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

void removeDisplayOptionsAnnotation(libsbml::Model *model) {
  removeAnnotation(model, annotationNameDisplayOptions);
}

void addDisplayOptionsAnnotation(libsbml::Model *model,
                                 const DisplayOptions &displayOptions) {
  if (model == nullptr) {
    return;
  }
  // if there is already an annotation set by us, remove it
  removeDisplayOptionsAnnotation(model);
  // append annotation with mesh info
  auto xml = fmt::format(
      "<{prefix}:{name} xmlns:{prefix}=\"{uri}\" "
      "{prefix}:showMinMax=\"{showMinMax}\" "
      "{prefix}:normaliseOverAllTimepoints=\"{normaliseOverAllTimepoints}\" "
      "{prefix}:normaliseOverAllSpecies=\"{normaliseOverAllSpecies}\" "
      "{prefix}:showSpecies=\"{showSpecies}\" />",
      fmt::arg("prefix", annotationPrefix), fmt::arg("uri", annotationURI),
      fmt::arg("name", annotationNameDisplayOptions),
      fmt::arg("showMinMax", static_cast<int>(displayOptions.showMinMax)),
      fmt::arg("normaliseOverAllTimepoints",
               static_cast<int>(displayOptions.normaliseOverAllTimepoints)),
      fmt::arg("normaliseOverAllSpecies",
               static_cast<int>(displayOptions.normaliseOverAllSpecies)),
      fmt::arg("showSpecies", utils::vectorToString(
                                  utils::toInt(displayOptions.showSpecies))));
  model->appendAnnotation(xml);
  SPDLOG_INFO("appending annotation: {}", xml);
}

std::optional<DisplayOptions>
getDisplayOptionsAnnotation(const libsbml::Model *model) {
  std::optional<DisplayOptions> opts = {};
  if (const auto *node = getAnnotation(model, annotationNameDisplayOptions);
      node != nullptr) {
    opts = DisplayOptions{};
    opts->normaliseOverAllTimepoints =
        static_cast<bool>(utils::stringToVector<int>(node->getAttrValue(
            "normaliseOverAllTimepoints", annotationURI))[0]);
    SPDLOG_INFO("  - normaliseOverAllTimepoints: {}",
                opts->normaliseOverAllTimepoints);
    opts->normaliseOverAllSpecies =
        static_cast<bool>(utils::stringToVector<int>(
            node->getAttrValue("normaliseOverAllSpecies", annotationURI))[0]);
    SPDLOG_INFO("  - normaliseOverAllSpecies: {}",
                opts->normaliseOverAllSpecies);
    opts->showMinMax = static_cast<bool>(utils::stringToVector<int>(
        node->getAttrValue("showMinMax", annotationURI))[0]);
    SPDLOG_INFO("  - showMinMax: {}", opts->showMinMax);
    opts->showSpecies = utils::toBool(utils::stringToVector<int>(
        node->getAttrValue("showSpecies", annotationURI)));
    SPDLOG_INFO("  - showSpecies: {}",
                utils::vectorToString(utils::toInt(opts->showSpecies)));
  }
  return opts;
}

} // namespace model

} // namespace sme
