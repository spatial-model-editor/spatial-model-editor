// SBML sId and Name utility functions

#pragma once

#include <QString>
#include <QStringList>
#include <string>

namespace libsbml {
class Model;
class Geometry;
} // namespace libsbml

namespace sme::model {

/**
 * @brief Check if model id is available as an SBML ``sId``.
 */
bool isSIdAvailable(const std::string &id, libsbml::Model *model);

/**
 * @brief Check if id is available in spatial geometry namespace.
 */
bool isSpatialIdAvailable(const std::string &id, libsbml::Geometry *geom);

/**
 * @brief Convert display name to unique SBML ``sId``.
 */
QString nameToUniqueSId(const QString &name, libsbml::Model *model);

/**
 * @brief Create unique name by appending incrementing postfix.
 */
QString makeUnique(const QString &name, const QStringList &names,
                   const QString &postfix = "_");

} // namespace sme::model
