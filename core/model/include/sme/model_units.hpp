// units

#pragma once

#include "sme/image_stack.hpp"
#include <QString>
#include <QVector>

namespace libsbml {
class Model;
}

namespace sme::model {

struct Unit {
  QString name;
  // SBML unit definition: (multiplier * 10^scale * kind)^exponent
  QString kind;
  int scale{0};
  int exponent{1};
  double multiplier{1.0};
};

bool operator==(const Unit &, const Unit &);

QString unitInBaseUnits(const Unit &unit);

class UnitVector {
private:
  QVector<Unit> units;
  int index = 0;

public:
  explicit UnitVector(const QVector<Unit> &unitsVec = {}, int defaultIndex = 0);
  [[nodiscard]] const Unit &get() const;
  [[nodiscard]] const QVector<Unit> &getUnits() const;
  QVector<Unit> &getUnits();
  [[nodiscard]] int getIndex() const;
  void setIndex(int newIndex);
};

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
  explicit ModelUnits(libsbml::Model *model = nullptr);
  [[nodiscard]] const Unit &getTime() const;
  [[nodiscard]] int getTimeIndex() const;
  [[nodiscard]] const QVector<Unit> &getTimeUnits() const;
  QVector<Unit> &getTimeUnits();
  void setTimeIndex(int index);
  [[nodiscard]] const Unit &getLength() const;
  [[nodiscard]] int getLengthIndex() const;
  [[nodiscard]] const QVector<Unit> &getLengthUnits() const;
  QVector<Unit> &getLengthUnits();
  void setLengthIndex(int index);
  [[nodiscard]] const Unit &getVolume() const;
  [[nodiscard]] int getVolumeIndex() const;
  [[nodiscard]] const QVector<Unit> &getVolumeUnits() const;
  QVector<Unit> &getVolumeUnits();
  void setVolumeIndex(int index);
  [[nodiscard]] const Unit &getAmount() const;
  [[nodiscard]] int getAmountIndex() const;
  [[nodiscard]] const QVector<Unit> &getAmountUnits() const;
  QVector<Unit> &getAmountUnits();
  void setAmountIndex(int index);
  [[nodiscard]] const QString &getConcentration() const;
  [[nodiscard]] const QString &getDiffusion() const;
  [[nodiscard]] const QString &getCompartmentReaction() const;
  [[nodiscard]] const QString &getMembraneReaction() const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

sme::common::VolumeF rescale(const sme::common::VolumeF &voxelSize,
                             const Unit &oldUnit, const Unit &newUnit);

double getVolOverL3(const Unit &lengthUnit, const Unit &volumeUnit);

} // namespace sme::model
