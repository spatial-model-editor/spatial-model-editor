#pragma once

#include "model_units.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogUnits;
}

class DialogUnits : public QDialog {
  Q_OBJECT

public:
  explicit DialogUnits(sme::model::ModelUnits &units,
                       QWidget *parent = nullptr);
  ~DialogUnits();
  int getTimeUnitIndex() const;
  int getLengthUnitIndex() const;
  int getVolumeUnitIndex() const;
  int getAmountUnitIndex() const;

private:
  std::unique_ptr<Ui::DialogUnits> ui;
  sme::model::ModelUnits &units;
  void cmbTime_currentIndexChanged(int index);
  void cmbLength_currentIndexChanged(int index);
  void cmbVolume_currentIndexChanged(int index);
  void cmbAmount_currentIndexChanged(int index);
  void btnEditTime_pressed();
  void btnEditLength_pressed();
  void btnEditVolume_pressed();
  void btnEditAmount_pressed();
};
