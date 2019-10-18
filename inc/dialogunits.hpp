#pragma once

#include <QDialog>
#include <memory>

#include "units.hpp"

namespace Ui {
class DialogUnits;
}

class DialogUnits : public QDialog {
  Q_OBJECT

 public:
  explicit DialogUnits(const units::ModelUnits& units,
                       QWidget* parent = nullptr);
  int getTimeUnitIndex() const;
  int getLengthUnitIndex() const;
  int getVolumeUnitIndex() const;
  int getAmountUnitIndex() const;

 private:
  std::shared_ptr<Ui::DialogUnits> ui;
  const units::ModelUnits& units;
  void cmbTime_currentIndexChanged(int index);
  void cmbLength_currentIndexChanged(int index);
  void cmbVolume_currentIndexChanged(int index);
  void cmbAmount_currentIndexChanged(int index);
};
