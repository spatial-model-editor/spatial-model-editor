#include "sme/xml_annotation.hpp"
#include "sme/logger.hpp"
#include "sme/mesh2d.hpp"
#include "sme/serialization.hpp"
#include "sme/utils.hpp"
#include "xml_legacy_annotation.hpp"
#include <fmt/core.h>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

static const char *const annotationName{"spatialModelEditor"};
static const char *const annotationURI{
    "https://github.com/spatial-model-editor"};

static const libsbml::XMLNode *getAnnotation(const libsbml::Model *model) {
  if (model == nullptr || !model->isSetAnnotation()) {
    return nullptr;
  }
  const auto *node{model->getAnnotation()};
  for (unsigned i = 0; i < node->getNumChildren(); ++i) {
    if (const auto &child = node->getChild(i);
        child.getURI() == annotationURI && child.getName() == annotationName) {
      return &child;
    }
  }
  return nullptr;
}

static void removeAnnotation(libsbml::Model *model) {
  if (model == nullptr || !model->isSetAnnotation()) {
    return;
  }
  auto *node{model->getAnnotation()};
  for (unsigned i = 0; i < node->getNumChildren(); ++i) {
    if (const auto &child{node->getChild(i)};
        child.getURI() == annotationURI && child.getName() == annotationName) {
      std::unique_ptr<libsbml::XMLNode> n(node->removeChild(i));
      SPDLOG_INFO("removed annotation {} : '{}'", i, n->toXMLString());
      return;
    }
  }
}

void setSbmlAnnotation(libsbml::Model *model, const Settings &sbmlAnnotations) {
  removeAnnotation(model);
  auto xml{fmt::format("<{name} xmlns=\"{uri}\">{xml}</{name}>",
                       fmt::arg("name", annotationName),
                       fmt::arg("uri", annotationURI),
                       fmt::arg("xml", common::toXml(sbmlAnnotations)))};
  model->appendAnnotation(xml);
}

Settings getSbmlAnnotation(libsbml::Model *model) {
  if (hasLegacyAnnotations(model)) {
    return importAndRemoveLegacyAnnotations(model);
  }
  if (const auto *node{getAnnotation(model)}; node != nullptr) {
    SPDLOG_INFO("annotation: '{}'", node->getChild(0).toXMLString());
    return common::fromXml(node->getChild(0).toXMLString());
  }
  return {};
}

} // namespace sme::model
