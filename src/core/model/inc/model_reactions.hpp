// SBML reactions

#pragma once

#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
class Species;
} // namespace libsbml

namespace geometry {
class Membrane;
}

namespace model {

class ModelReactions {
private:
  QStringList ids;
  QStringList names;
  QVector<QStringList> parameterIds;
  libsbml::Model *sbmlModel = nullptr;

public:
  ModelReactions();
  explicit ModelReactions(libsbml::Model *model,
                          const std::vector<geometry::Membrane> &membranes);
  QStringList getIds(const QString &locationId) const;
  QString add(const QString &name, const QString &locationId,
              const QString &rateExpression = "1");
  void remove(const QString &id);
  void removeAllInvolvingSpecies(const QString &speciesId);
  QString setName(const QString &id, const QString &name);
  QString getName(const QString &id) const;
  void setLocation(const QString &id, const QString &locationId);
  QString getLocation(const QString &id) const;
  int getSpeciesStoichiometry(const QString &id,
                              const QString &speciesId) const;
  void setSpeciesStoichiometry(const QString &id, const QString &speciesId,
                               int stoichiometry);
  QString getRateExpression(const QString &id) const;
  void setRateExpression(const QString &id, const QString &expression);
  QStringList getParameterIds(const QString &id) const;
  QString setParameterName(const QString &reactionId,
                           const QString &parameterId, const QString &name);
  QString getParameterName(const QString &reactionId,
                           const QString &parameterId) const;
  void setParameterValue(const QString &reactionId, const QString &parameterId,
                         double value);
  double getParameterValue(const QString &reactionId,
                           const QString &parameterId) const;
  QString addParameter(const QString &reactionId, const QString &name,
                       double value);
  void removeParameter(const QString &reactionId, const QString &id);
};

} // namespace model
