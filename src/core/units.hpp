// units

#pragma once

#include <QString>
#include <QVector>

namespace units {

struct Unit {
  // UI display name
  QString name;
  // UI display symbol
  QString symbol;
  // SBML unit definition: name == (multiplier * 10^scale * kind)^exponent
  QString kind;
  int scale;
  int exponent = 1;
  double multiplier = 1.0;
};

class UnitVector {
 private:
  QVector<Unit> units;
  int index = 0;

 public:
  explicit UnitVector(const QVector<Unit>& unitsVec = {});
  const Unit& get() const;
  const QVector<Unit>& getUnits() const;
  int getIndex() const;
  void setIndex(int newIndex);
};

class ModelUnits {
 private:
  UnitVector time;
  UnitVector length;
  UnitVector volume;
  UnitVector amount;
  QString concentration;
  QString diffusion;
  void updateConcentration();
  void updateDiffusion();

 public:
  ModelUnits(const UnitVector& timeUnits, const UnitVector& lengthUnits,
             const UnitVector& volumeUnits, const UnitVector& amountUnits);
  const Unit& getTime() const;
  int getTimeIndex() const;
  const QVector<Unit>& getTimeUnits() const;
  void setTime(int index);
  const Unit& getLength() const;
  int getLengthIndex() const;
  const QVector<Unit>& getLengthUnits() const;
  void setLength(int index);
  const Unit& getVolume() const;
  int getVolumeIndex() const;
  const QVector<Unit>& getVolumeUnits() const;
  void setVolume(int index);
  const Unit& getAmount() const;
  int getAmountIndex() const;
  const QVector<Unit>& getAmountUnits() const;
  void setAmount(int index);
  const QString& getConcentration() const;
  const QString& getDiffusion() const;
};

double rescale(double val, const Unit& oldUnit, const Unit& newUnit);

// convert pixel width to pixel volume (with unit length in third dimension)
double pixelWidthToVolume(double width, const Unit& lengthUnit,
                          const Unit& volumeUnit);

}  // namespace units
