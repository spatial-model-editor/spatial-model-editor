#include "xml_legacy_annotation.hpp"
#include "sbml_utils.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
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
static const std::string annotationNameColor{"color"};
static const std::string annotationNameDisplayOptions{"displayOptions"};

namespace sme::model {

static const libsbml::XMLNode *
getAnnotation(const libsbml::SBase *parent, const std::string &annotationName) {
  if (parent == nullptr || !parent->isSetAnnotation()) {
    return nullptr;
  }
  const auto *node{parent->getAnnotation()};
  for (unsigned i = 0; i < node->getNumChildren(); ++i) {
    if (const auto &child{node->getChild(i)};
        child.getURI() == annotationURI &&
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
  auto *node{parent->getAnnotation()};
  for (unsigned i = 0; i < node->getNumChildren(); ++i) {
    if (const auto &child{node->getChild(i)};
        child.getURI() == annotationURI &&
        child.getPrefix() == annotationPrefix &&
        child.getName() == annotationName) {
      std::unique_ptr<libsbml::XMLNode> n(node->removeChild(i));
      SPDLOG_INFO("removed annotation {} : '{}'", i, n->toXMLString());
      return;
    }
  }
}

static const libsbml::ParametricGeometry *
getParametricGeometry(const libsbml::Geometry *geom) {
  if (geom == nullptr) {
    return nullptr;
  }
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (auto *def{geom->getGeometryDefinition(i)};
        def->getIsActive() && def->isParametricGeometry()) {
      return dynamic_cast<const libsbml::ParametricGeometry *>(def);
    }
  }
  return nullptr;
}

static libsbml::ParametricGeometry *
getParametricGeometry(libsbml::Geometry *geom) {
  return const_cast<libsbml::ParametricGeometry *>(
      getParametricGeometry(const_cast<const libsbml::Geometry *>(geom)));
}

bool hasLegacyAnnotations(const libsbml::Model *model) {
  const unsigned nSpecies{model->getNumSpecies()};
  if (getAnnotation(model, annotationNameDisplayOptions) != nullptr) {
    return true;
  }
  for (unsigned i = 0; i < nSpecies; ++i) {
    if (getAnnotation(model->getSpecies(i), annotationNameColor) != nullptr) {
      return true;
    }
  }
  if (auto *geom = getGeometry(model); geom != nullptr) {
    if (auto *pg{getParametricGeometry(geom)};
        pg != nullptr && getAnnotation(pg, annotationNameMesh) != nullptr) {
      return true;
    }
  }
  return false;
}

Settings importAndRemoveLegacyAnnotations(libsbml::Model *model) {
  Settings sbmlAnnotation{};
  if (auto opt{getDisplayOptionsAnnotation(model)}; opt.has_value()) {
    sbmlAnnotation.displayOptions = opt.value();
  }
  removeDisplayOptionsAnnotation(model);
  const unsigned nSpecies{model->getNumSpecies()};
  for (unsigned i = 0; i < nSpecies; ++i) {
    auto *species{model->getSpecies(i)};
    if (auto col{getSpeciesColorAnnotation(species)}; col.has_value()) {
      sbmlAnnotation.speciesColors[species->getId()] = col.value();
    }
    removeSpeciesColorAnnotation(species);
  }
  if (auto *pg{getParametricGeometry(getOrCreateGeometry(model))};
      pg != nullptr) {
    if (auto mp{getMeshParamsAnnotationData(pg)}; mp.has_value()) {
      sbmlAnnotation.meshParameters = mp.value();
    }
    removeMeshParamsAnnotation(pg);
  }
  return sbmlAnnotation;
}

void removeMeshParamsAnnotation(libsbml::ParametricGeometry *pg) {
  removeAnnotation(pg, annotationNameMesh);
}

std::optional<MeshParameters>
getMeshParamsAnnotationData(const libsbml::ParametricGeometry *pg) {
  std::optional<MeshParameters> dat = {};
  if (const auto *node = getAnnotation(pg, annotationNameMesh);
      node != nullptr) {
    auto &d = dat.emplace();
    d.maxPoints = common::stringToVector<std::size_t>(
        node->getAttrValue("maxBoundaryPoints", annotationURI));
    SPDLOG_INFO("  - maxBoundaryPoints: {}",
                common::vectorToString(d.maxPoints));
    d.maxAreas = common::stringToVector<std::size_t>(
        node->getAttrValue("maxTriangleAreas", annotationURI));
    SPDLOG_INFO("  - maxTriangleAreas: {}", common::vectorToString(d.maxAreas));
  }
  return dat;
}

void removeSpeciesColorAnnotation(libsbml::Species *species) {
  removeAnnotation(species, annotationNameColor);
}

std::optional<QRgb> getSpeciesColorAnnotation(const libsbml::Species *species) {
  if (const auto *node = getAnnotation(species, annotationNameColor);
      node != nullptr) {
    auto color = common::stringToVector<QRgb>(
        node->getAttrValue("color", annotationURI))[0];
    SPDLOG_INFO("Species: {}", species->getId());
    SPDLOG_INFO("  - color: {:x}", color);
    return color;
  }
  return {};
}

void removeDisplayOptionsAnnotation(libsbml::Model *model) {
  removeAnnotation(model, annotationNameDisplayOptions);
}

std::optional<DisplayOptions>
getDisplayOptionsAnnotation(const libsbml::Model *model) {
  std::optional<DisplayOptions> opts = {};
  if (const auto *node = getAnnotation(model, annotationNameDisplayOptions);
      node != nullptr) {
    opts = DisplayOptions{};
    opts->normaliseOverAllTimepoints =
        static_cast<bool>(common::stringToVector<int>(node->getAttrValue(
            "normaliseOverAllTimepoints", annotationURI))[0]);
    SPDLOG_INFO("  - normaliseOverAllTimepoints: {}",
                opts->normaliseOverAllTimepoints);
    opts->normaliseOverAllSpecies =
        static_cast<bool>(common::stringToVector<int>(
            node->getAttrValue("normaliseOverAllSpecies", annotationURI))[0]);
    SPDLOG_INFO("  - normaliseOverAllSpecies: {}",
                opts->normaliseOverAllSpecies);
    opts->showMinMax = static_cast<bool>(common::stringToVector<int>(
        node->getAttrValue("showMinMax", annotationURI))[0]);
    SPDLOG_INFO("  - showMinMax: {}", opts->showMinMax);
    opts->showSpecies = common::toBool(common::stringToVector<int>(
        node->getAttrValue("showSpecies", annotationURI)));
    SPDLOG_INFO("  - showSpecies: {}",
                common::vectorToString(common::toInt(opts->showSpecies)));
  }
  return opts;
}

} // namespace sme::model
