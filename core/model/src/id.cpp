#include "id.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
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
  const std::string charsToConvertToUnderscore = " -_/";
  std::string id;
  // remove any non-alphanumeric chars, convert spaces etc to underscores
  for (auto c : name.toStdString()) {
    if (sme::common::isalnum(c)) {
      id.push_back(c);
    } else if (charsToConvertToUnderscore.find(c) != std::string::npos) {
      id.push_back('_');
    }
  }
  // first char must be a letter or underscore
  if (!sme::common::isalpha(id.front()) && id.front() != '_') {
    id = "_" + id;
  }
  SPDLOG_DEBUG("  -> '{}'", id);
  // ensure it is unique, i.e. doesn't clash with any other SId in model
  while (!isSIdAvailable(id, model)) {
    id.append("_");
    SPDLOG_DEBUG("  -> '{}'", id);
  }
  return id.c_str();
}

QString makeUnique(const QString &name, const QStringList &names,
                   const QString &postfix) {
  QString uniqueName{name};
  while (names.contains(uniqueName)) {
    uniqueName.append(postfix);
  }
  return uniqueName;
}

} // namespace sme::model
