// SBML parameters

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

namespace sme::model {

class ModelEvents;
class ModelSpecies;

/**
 * @brief Id/name pair helper.
 */
struct IdName {
  std::string id;
  std::string name;
};

/**
 * @brief Spatial coordinate parameter ids/names.
 */
struct SpatialCoordinates {
  IdName x;
  IdName y;
  IdName z;
};

/**
 * @brief Id/name/value tuple helper.
 */
struct IdNameValue {
  std::string id;
  std::string name;
  double value;
};

/**
 * @brief Id/name/expression tuple helper.
 */
struct IdNameExpr {
  std::string id;
  std::string name;
  std::string expr;
};

/**
 * @brief SBML parameter manager.
 */
class ModelParameters {
private:
  QStringList ids;
  QStringList names;
  SpatialCoordinates spatialCoordinates;
  libsbml::Model *sbmlModel{nullptr};
  bool hasUnsavedChanges{false};
  ModelEvents *modelEvents{nullptr};
  ModelSpecies *modelSpecies{nullptr};

public:
  /**
   * @brief Construct empty parameter model.
   */
  ModelParameters();
  /**
   * @brief Construct parameter model from SBML model.
   */
  explicit ModelParameters(libsbml::Model *model);
  /**
   * @brief Set events dependency pointer.
   */
  void setEventsPtr(ModelEvents *events);
  /**
   * @brief Set species dependency pointer.
   */
  void setSpeciesPtr(ModelSpecies *species);
  /**
   * @brief Parameter ids.
   */
  [[nodiscard]] const QStringList &getIds() const;
  /**
   * @brief Parameter names.
   */
  [[nodiscard]] const QStringList &getNames() const;
  /**
   * @brief Set display name.
   */
  QString setName(const QString &id, const QString &name);
  /**
   * @brief Get display name.
   */
  [[nodiscard]] QString getName(const QString &id) const;
  /**
   * @brief Set parameter expression/value.
   */
  void setExpression(const QString &id, const QString &expr);
  /**
   * @brief Get parameter expression/value.
   */
  [[nodiscard]] QString getExpression(const QString &id) const;
  /**
   * @brief Add new parameter.
   */
  QString add(const QString &name);
  /**
   * @brief Remove parameter.
   */
  void remove(const QString &id);
  /**
   * @brief Spatial coordinate parameters.
   */
  [[nodiscard]] const SpatialCoordinates &getSpatialCoordinates() const;
  /**
   * @brief Set spatial coordinate parameter ids.
   */
  void setSpatialCoordinates(SpatialCoordinates coords);
  /**
   * @brief Symbols available for expression substitution.
   */
  [[nodiscard]] std::vector<IdName>
  getSymbols(const QStringList &compartments = {}) const;
  /**
   * @brief Constant parameters.
   */
  [[nodiscard]] std::vector<IdNameValue> getGlobalConstants() const;
  /**
   * @brief Non-constant parameters.
   */
  [[nodiscard]] std::vector<IdNameExpr> getNonConstantParameters() const;
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
