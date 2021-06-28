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

bool isSIdAvailable(const std::string &id, libsbml::Model *model);

bool isSpatialIdAvailable(const std::string &id, libsbml::Geometry *geom);

QString nameToUniqueSId(const QString &name, libsbml::Model *model);

QString makeUnique(const QString &name, const QStringList &names,
                   const QString &postfix = "_");

} // namespace sme
