#pragma once

#include <QDialog>
#include <memory>

#include "model_units.hpp"

namespace Ui {
class DialogUnits;
}

class DialogUnits : public QDialog {
  Q_OBJECT

public:
  explicit DialogUnits(const model::ModelUnits &units,
                       QWidget *parent = nullptr);
  ~DialogUnits();
  int getTimeUnitIndex() const;
  int getLengthUnitIndex() const;
  int getVolumeUnitIndex() const;
  int getAmountUnitIndex() const;

private:
  std::unique_ptr<Ui::DialogUnits> ui;
  const model::ModelUnits &units;
  void cmbTime_currentIndexChanged(int index);
  void cmbLength_currentIndexChanged(int index);
  void cmbVolume_currentIndexChanged(int index);
  void cmbAmount_currentIndexChanged(int index);
};
