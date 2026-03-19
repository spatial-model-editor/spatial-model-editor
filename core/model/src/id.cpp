#include "id.hpp"
#include "sme/id.hpp"
#include "sme/logger.hpp"
#include <QString>
#include <QStringList>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

bool isSIdAvailable(const std::string &id, libsbml::Model *model) {
  return model->getElementBySId(id) == nullptr;
}

bool isSpatialIdAvailable(const std::string &id, libsbml::Geometry *geom) {
  return geom->getElementBySId(id) == nullptr;
}

QString nameToUniqueSId(const QString &name, libsbml::Model *model) {
  SPDLOG_DEBUG("name: '{}'", name.toStdString());
  auto id{sme::common::nameToSId(name.toStdString())};
  SPDLOG_DEBUG("  -> '{}'", id);
  // ensure it is unique, i.e. doesn't clash with any other SId in model
  auto uniqueId = sme::common::makeUnique(id, [model](const auto &candidate) {
    return isSIdAvailable(candidate, model);
  });
  if (uniqueId != id) {
    SPDLOG_DEBUG("  -> '{}'", uniqueId);
  }
  return uniqueId.c_str();
}

QString makeUnique(const QString &name, const QStringList &names,
                   const QString &postfix) {
  const auto uniqueName = sme::common::makeUnique(
      name.toStdString(),
      [&names](const auto &candidate) {
        return !names.contains(candidate.c_str());
      },
      postfix.toStdString());
  return uniqueName.c_str();
}

} // namespace sme::model
