// units

#pragma once

#include <QString>
#include <QVector>

namespace libsbml {
class Model;
}

namespace model {

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
  const Unit &get() const;
  const QVector<Unit> &getUnits() const;
  QVector<Unit> &getUnits();
  int getIndex() const;
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
  UnitVector amount{{{"mol", "mole", 0}, {"mmol", "mole", -3}}, 1};
  QString concentration;
  QString diffusion;
  QString compartmentReaction;
  QString membraneReaction;
  libsbml::Model *sbmlModel;
  void updateConcentration();
  void updateDiffusion();
  void updateReactions();

public:
  explicit ModelUnits(libsbml::Model *model = nullptr);
  const Unit &getTime() const;
  int getTimeIndex() const;
  const QVector<Unit> &getTimeUnits() const;
  QVector<Unit> &getTimeUnits();
  void setTimeIndex(int index);
  const Unit &getLength() const;
  int getLengthIndex() const;
  const QVector<Unit> &getLengthUnits() const;
  QVector<Unit> &getLengthUnits();
  void setLengthIndex(int index);
  const Unit &getVolume() const;
  int getVolumeIndex() const;
  const QVector<Unit> &getVolumeUnits() const;
  QVector<Unit> &getVolumeUnits();
  void setVolumeIndex(int index);
  const Unit &getAmount() const;
  int getAmountIndex() const;
  const QVector<Unit> &getAmountUnits() const;
  QVector<Unit> &getAmountUnits();
  void setAmountIndex(int index);
  const QString &getConcentration() const;
  const QString &getDiffusion() const;
  const QString &getCompartmentReaction() const;
  const QString &getMembraneReaction() const;
};

double rescale(double val, const Unit &oldUnit, const Unit &newUnit);

// convert pixel width to pixel volume (with unit length in third dimension)
double pixelWidthToVolume(double width, const Unit &lengthUnit,
                          const Unit &volumeUnit);

} // namespace model
