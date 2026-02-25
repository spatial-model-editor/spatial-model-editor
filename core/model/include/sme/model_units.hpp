// units

#pragma once

#include "sme/image_stack.hpp"
#include <QString>
#include <QVector>

namespace libsbml {
class Model;
}

namespace sme::model {

/**
 * @brief Unit definition component.
 *
 * Represents SBML base-unit form:
 * ``(multiplier * 10^scale * kind)^exponent``.
 */
struct Unit {
  /**
   * @brief Display name.
   */
  QString name;
  /**
   * @brief SBML base kind (e.g. ``second``, ``metre``, ``mole``).
   */
  QString kind;
  /**
   * @brief Power-of-10 scale.
   */
  int scale{0};
  /**
   * @brief Exponent applied to base unit.
   */
  int exponent{1};
  /**
   * @brief Scalar multiplier.
   */
  double multiplier{1.0};
  /**
   * @brief Equality comparison.
   */
  friend bool operator==(const Unit &, const Unit &);
};

/**
 * @brief Format a unit in SBML base-unit notation.
 */
QString unitInBaseUnits(const Unit &unit);

/**
 * @brief Unit choices with current selected index.
 */
class UnitVector {
private:
  QVector<Unit> units;
  int index = 0;

public:
  /**
   * @brief Construct from a set of units and default index.
   */
  explicit UnitVector(const QVector<Unit> &unitsVec = {}, int defaultIndex = 0);
  /**
   * @brief Currently selected unit.
   */
  [[nodiscard]] const Unit &get() const;
  /**
   * @brief Immutable list of available units.
   */
  [[nodiscard]] const QVector<Unit> &getUnits() const;
  /**
   * @brief Mutable list of available units.
   */
  QVector<Unit> &getUnits();
  /**
   * @brief Current selected index.
   */
  [[nodiscard]] int getIndex() const;
  /**
   * @brief Set selected index.
   */
  void setIndex(int newIndex);
};

/**
 * @brief Model unit system and derived display strings.
 */
class ModelUnits {
private:
  UnitVector time{{{"hour", "second", 0, 1, 3600},
                   {"min", "second", 0, 1, 60},
                   {"s", "second", 0},
                   {"ms", "second", -3},
                   {"us", "second", -6}},
                  2};
  UnitVector length{{{"m", "metre", 0},
                     {"dm", "metre", -1},
                     {"cm", "metre", -2},
                     {"mm", "metre", -3},
                     {"um", "metre", -6},
                     {"nm", "metre", -9}},
                    2};
  UnitVector volume{{{"L", "litre", 0},
                     {"dL", "litre", -1},
                     {"cL", "litre", -2},
                     {"mL", "litre", -3},
                     {"m3", "metre", 0, 3},
                     {"dm3", "metre", -1, 3},
                     {"cm3", "metre", -2, 3},
                     {"mm3", "metre", -3, 3}},
                    3};
  UnitVector amount{
      {{"mol", "mole", 0}, {"mmol", "mole", -3}, {"umol", "mole", -6}}, 1};
  QString concentration;
  QString diffusion;
  QString compartmentReaction;
  QString membraneReaction;
  libsbml::Model *sbmlModel;
  bool hasUnsavedChanges{false};
  void updateConcentration();
  void updateDiffusion();
  void updateReactions();

public:
  /**
   * @brief Construct units manager from SBML model.
   */
  explicit ModelUnits(libsbml::Model *model = nullptr);
  /**
   * @brief Selected time unit.
   */
  [[nodiscard]] const Unit &getTime() const;
  /**
   * @brief Selected time unit index.
   */
  [[nodiscard]] int getTimeIndex() const;
  /**
   * @brief Available time units.
   */
  [[nodiscard]] const QVector<Unit> &getTimeUnits() const;
  /**
   * @brief Mutable time unit list.
   */
  QVector<Unit> &getTimeUnits();
  /**
   * @brief Set selected time unit index.
   */
  void setTimeIndex(int index);
  /**
   * @brief Selected length unit.
   */
  [[nodiscard]] const Unit &getLength() const;
  /**
   * @brief Selected length unit index.
   */
  [[nodiscard]] int getLengthIndex() const;
  /**
   * @brief Available length units.
   */
  [[nodiscard]] const QVector<Unit> &getLengthUnits() const;
  /**
   * @brief Mutable length unit list.
   */
  QVector<Unit> &getLengthUnits();
  /**
   * @brief Set selected length unit index.
   */
  void setLengthIndex(int index);
  /**
   * @brief Selected volume unit.
   */
  [[nodiscard]] const Unit &getVolume() const;
  /**
   * @brief Selected volume unit index.
   */
  [[nodiscard]] int getVolumeIndex() const;
  /**
   * @brief Available volume units.
   */
  [[nodiscard]] const QVector<Unit> &getVolumeUnits() const;
  /**
   * @brief Mutable volume unit list.
   */
  QVector<Unit> &getVolumeUnits();
  /**
   * @brief Set selected volume unit index.
   */
  void setVolumeIndex(int index);
  /**
   * @brief Selected amount unit.
   */
  [[nodiscard]] const Unit &getAmount() const;
  /**
   * @brief Selected amount unit index.
   */
  [[nodiscard]] int getAmountIndex() const;
  /**
   * @brief Available amount units.
   */
  [[nodiscard]] const QVector<Unit> &getAmountUnits() const;
  /**
   * @brief Mutable amount unit list.
   */
  QVector<Unit> &getAmountUnits();
  /**
   * @brief Set selected amount unit index.
   */
  void setAmountIndex(int index);
  /**
   * @brief Derived concentration unit string.
   */
  [[nodiscard]] const QString &getConcentration() const;
  /**
   * @brief Derived diffusion unit string.
   */
  [[nodiscard]] const QString &getDiffusion() const;
  /**
   * @brief Derived compartment reaction-rate unit string.
   */
  [[nodiscard]] const QString &getCompartmentReaction() const;
  /**
   * @brief Derived membrane reaction-rate unit string.
   */
  [[nodiscard]] const QString &getMembraneReaction() const;
  /**
   * @brief Unsaved state flag.
   */
  [[nodiscard]] bool getHasUnsavedChanges() const;
  /**
   * @brief Set unsaved state flag.
   */
  void setHasUnsavedChanges(bool unsavedChanges);
};

/**
 * @brief Rescale voxel size between two length units.
 */
sme::common::VolumeF rescale(const sme::common::VolumeF &voxelSize,
                             const Unit &oldUnit, const Unit &newUnit);

/**
 * @brief Conversion factor from volume unit to ``length^3``.
 */
double getVolOverL3(const Unit &lengthUnit, const Unit &volumeUnit);

} // namespace sme::model
