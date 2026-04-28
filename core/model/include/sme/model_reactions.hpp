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

/**
 * @brief Original/rescaled reaction expression details.
 */
struct ReactionRescaling {
  std::string id{};
  QString reactionName{};
  QString reactionLocation{};
  QString originalExpression{};
  QString rescaledExpression{};
};

/**
 * @brief Potential location for a reaction definition.
 */
struct ReactionLocation {
  /**
   * @brief Location type.
   */
  enum class Type { Compartment, Membrane, Invalid };
  QString id;
  QString name;
  Type type;
};

/**
 * @brief SBML reaction manager.
 */
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
  /**
   * @brief Construct empty reaction model.
   */
  ModelReactions();
  /**
   * @brief Construct reaction model from SBML model.
   */
  explicit ModelReactions(libsbml::Model *model,
                          const ModelCompartments *compartments,
                          const ModelMembranes *membranes,
                          bool isNonSpatialModel);
  /**
   * @brief Ensure all reactions reference valid locations.
   */
  void makeReactionLocationsValid();
  /**
   * @brief Apply precomputed reaction expression rescalings.
   */
  void applySpatialReactionRescalings(
      const std::vector<ReactionRescaling> &reactionRescalings);
  /**
   * @brief Compute expression rescalings for spatial unit consistency.
   */
  [[nodiscard]] std::vector<ReactionRescaling>
  getSpatialReactionRescalings() const;
  /**
   * @brief Returns ``true`` if imported ODE reactions were incomplete.
   */
  [[nodiscard]] bool getIsIncompleteODEImport() const;
  /**
   * @brief Reaction ids for a location id.
   */
  [[nodiscard]] QStringList getIds(const QString &locationId) const;
  /**
   * @brief Reaction ids for a location descriptor.
   */
  [[nodiscard]] QStringList
  getIds(const ReactionLocation &reactionLocation) const;
  /**
   * @brief All available reaction locations.
   */
  [[nodiscard]] std::vector<ReactionLocation> getReactionLocations() const;
  /**
   * @brief Add reaction.
   */
  QString add(const QString &name, const QString &locationId,
              const QString &rateExpression = "1");
  /**
   * @brief Remove reaction.
   */
  void remove(const QString &id);
  /**
   * @brief Remove all reactions involving species.
   */
  void removeAllInvolvingSpecies(const QString &speciesId);
  /**
   * @brief Remove all reactions in a location.
   */
  void removeAllInLocation(const QString &locationId);
  /**
   * @brief Set reaction name.
   */
  QString setName(const QString &id, const QString &name);
  /**
   * @brief Get reaction name.
   */
  [[nodiscard]] QString getName(const QString &id) const;
  /**
   * @brief Get stoichiometric scheme string.
   */
  [[nodiscard]] QString getScheme(const QString &id) const;
  /**
   * @brief Set reaction location id.
   */
  void setLocation(const QString &id, const QString &locationId);
  /**
   * @brief Get reaction location id.
   */
  [[nodiscard]] QString getLocation(const QString &id) const;
  /**
   * @brief Stoichiometry of a species in a reaction.
   */
  [[nodiscard]] double getSpeciesStoichiometry(const QString &id,
                                               const QString &speciesId) const;
  /**
   * @brief Set stoichiometry for species in reaction.
   */
  void setSpeciesStoichiometry(const QString &id, const QString &speciesId,
                               double stoichiometry);
  /**
   * @brief Get kinetic rate expression.
   */
  [[nodiscard]] QString getRateExpression(const QString &id) const;
  /**
   * @brief Set kinetic rate expression.
   */
  void setRateExpression(const QString &id, const QString &expression);
  /**
   * @brief Local parameter ids for reaction.
   */
  [[nodiscard]] QStringList getParameterIds(const QString &id) const;
  /**
   * @brief Set local parameter name.
   */
  QString setParameterName(const QString &reactionId,
                           const QString &parameterId, const QString &name);
  /**
   * @brief Get local parameter name.
   */
  [[nodiscard]] QString getParameterName(const QString &reactionId,
                                         const QString &parameterId) const;
  /**
   * @brief Set local parameter value.
   */
  void setParameterValue(const QString &reactionId, const QString &parameterId,
                         double value);
  /**
   * @brief Get local parameter value.
   */
  [[nodiscard]] double getParameterValue(const QString &reactionId,
                                         const QString &parameterId) const;
  /**
   * @brief Add local parameter to reaction.
   */
  QString addParameter(const QString &reactionId, const QString &name,
                       double value);
  /**
   * @brief Remove local parameter from reaction.
   */
  void removeParameter(const QString &reactionId, const QString &id);
  /**
   * @brief Returns ``true`` if any reaction depends on variable id.
   */
  [[nodiscard]] bool dependOnVariable(const QString &variableId) const;
  /**
   * @brief Unsaved state flag.
   */
  [[nodiscard]] bool getHasUnsavedChanges() const;
  /**
   * @brief Set unsaved state flag.
   */
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
