// units

#pragma once

#include <QString>
#include <QVector>

namespace libsbml {
class Model;
}

namespace model {

struct Unit {
  // UI display name
  QString name;
  // UI display symbol
  QString symbol;
  // SBML unit definition: name == (multiplier * 10^scale * kind)^exponent
  QString kind;
  int scale{0};
  int exponent{1};
  double multiplier{1.0};
};

class UnitVector {
private:
  QVector<Unit> units;
  int index = 0;

public:
  explicit UnitVector(const QVector<Unit> &unitsVec = {}, int defaultIndex = 0);
  const Unit &get() const;
  const QVector<Unit> &getUnits() const;
  int getIndex() const;
  void setIndex(int newIndex);
};

class ModelUnits {
private:
  UnitVector time{{{"hour", "h", "second", 0, 1, 3600},
                   {"minute", "m", "second", 0, 1, 60},
                   {"second", "s", "second", 0},
                   {"millisecond", "ms", "second", -3},
                   {"microsecond", "us", "second", -6}},
                  2};
  UnitVector length{{{"metre", "m", "metre", 0},
                     {"decimetre", "dm", "metre", -1},
                     {"centimetre", "cm", "metre", -2},
                     {"millimetre", "mm", "metre", -3},
                     {"micrometre", "um", "metre", -6},
                     {"nanometre", "nm", "metre", -9}},
                    2};
  UnitVector volume{{{"litre", "L", "litre", 0},
                     {"decilitre", "dL", "litre", -1},
                     {"centilitre", "cL", "litre", -2},
                     {"millilitre", "mL", "litre", -3},
                     {"cubic metre", "m^3", "metre", 0, 3},
                     {"cubic decimetre", "dm^3", "metre", -1, 3},
                     {"cubic centimetre", "cm^3", "metre", -2, 3},
                     {"cubic millimetre", "mm^3", "metre", -3, 3}},
                    3};
  UnitVector amount{
      {{"mole", "mol", "mole", 0}, {"millimole", "mmol", "mole", -3}}, 1};
  QString concentration;
  QString diffusion;
  libsbml::Model *sbmlModel;
  void updateConcentration();
  void updateDiffusion();

public:
  explicit ModelUnits(libsbml::Model *model = nullptr);
  const Unit &getTime() const;
  int getTimeIndex() const;
  const QVector<Unit> &getTimeUnits() const;
  void setTimeIndex(int index);
  const Unit &getLength() const;
  int getLengthIndex() const;
  const QVector<Unit> &getLengthUnits() const;
  void setLengthIndex(int index);
  const Unit &getVolume() const;
  int getVolumeIndex() const;
  const QVector<Unit> &getVolumeUnits() const;
  void setVolumeIndex(int index);
  const Unit &getAmount() const;
  int getAmountIndex() const;
  const QVector<Unit> &getAmountUnits() const;
  void setAmountIndex(int index);
  const QString &getConcentration() const;
  const QString &getDiffusion() const;
};

double rescale(double val, const Unit &oldUnit, const Unit &newUnit);

// convert pixel width to pixel volume (with unit length in third dimension)
double pixelWidthToVolume(double width, const Unit &lengthUnit,
                          const Unit &volumeUnit);

} // namespace model
