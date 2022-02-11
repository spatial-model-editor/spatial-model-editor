// SBML reactions

#pragma once

#include "sme/geometry.hpp"
#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
}

namespace sme::model {

class ModelCompartments;
class ModelMembranes;

struct ReactionRescaling {
  std::string id{};
  QString reactionName{};
  QString reactionLocation{};
  QString originalExpression{};
  QString rescaledExpression{};
};

struct ReactionLocation {
  enum class Type { Compartment, Membrane, Invalid };
  QString id;
  QString name;
  Type type;
};

class ModelReactions {
private:
  QStringList ids;
  QStringList names;
  QVector<QStringList> parameterIds;
  libsbml::Model *sbmlModel{};
  const ModelCompartments *modelCompartments{};
  const ModelMembranes *modelMembranes{};
  bool hasUnsavedChanges{false};
  bool isIncompleteODEImport{false};

public:
  ModelReactions();
  explicit ModelReactions(libsbml::Model *model,
                          const ModelCompartments *compartments,
                          const ModelMembranes *membranes,
                          bool isNonSpatialModel);
  void makeReactionLocationsValid();
  void applySpatialReactionRescalings(
      const std::vector<ReactionRescaling> &reactionRescalings);
  [[nodiscard]] std::vector<ReactionRescaling>
  getSpatialReactionRescalings() const;
  [[nodiscard]] bool getIsIncompleteODEImport() const;
  [[nodiscard]] QStringList getIds(const QString &locationId) const;
  [[nodiscard]] QStringList
  getIds(const ReactionLocation &reactionLocation) const;
  [[nodiscard]] std::vector<ReactionLocation> getReactionLocations() const;
  QString add(const QString &name, const QString &locationId,
              const QString &rateExpression = "1");
  void remove(const QString &id);
  void removeAllInvolvingSpecies(const QString &speciesId);
  QString setName(const QString &id, const QString &name);
  [[nodiscard]] QString getName(const QString &id) const;
  [[nodiscard]] QString getScheme(const QString &id) const;
  void setLocation(const QString &id, const QString &locationId);
  [[nodiscard]] QString getLocation(const QString &id) const;
  [[nodiscard]] double getSpeciesStoichiometry(const QString &id,
                                               const QString &speciesId) const;
  void setSpeciesStoichiometry(const QString &id, const QString &speciesId,
                               double stoichiometry);
  [[nodiscard]] QString getRateExpression(const QString &id) const;
  void setRateExpression(const QString &id, const QString &expression);
  [[nodiscard]] QStringList getParameterIds(const QString &id) const;
  QString setParameterName(const QString &reactionId,
                           const QString &parameterId, const QString &name);
  [[nodiscard]] QString getParameterName(const QString &reactionId,
                                         const QString &parameterId) const;
  void setParameterValue(const QString &reactionId, const QString &parameterId,
                         double value);
  [[nodiscard]] double getParameterValue(const QString &reactionId,
                                         const QString &parameterId) const;
  QString addParameter(const QString &reactionId, const QString &name,
                       double value);
  void removeParameter(const QString &reactionId, const QString &id);
  [[nodiscard]] bool dependOnVariable(const QString &variableId) const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
