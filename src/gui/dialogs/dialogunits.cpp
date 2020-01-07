#include "dialogunits.hpp"

#include "logger.hpp"
#include "ui_dialogunits.h"

DialogUnits::DialogUnits(const units::ModelUnits& modelUnits, QWidget* parent)
    : QDialog(parent),
      ui{std::make_unique<Ui::DialogUnits>()},
      units(modelUnits) {
  ui->setupUi(this);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogUnits::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogUnits::reject);
  connect(ui->cmbTime, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &DialogUnits::cmbTime_currentIndexChanged);
  connect(ui->cmbLength, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &DialogUnits::cmbLength_currentIndexChanged);
  connect(ui->cmbVolume, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &DialogUnits::cmbVolume_currentIndexChanged);
  connect(ui->cmbAmount, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &DialogUnits::cmbAmount_currentIndexChanged);

  for (const auto& u : units.getTimeUnits()) {
    ui->cmbTime->addItem(u.symbol);
  }
  for (const auto& u : units.getLengthUnits()) {
    ui->cmbLength->addItem(u.symbol);
  }
  for (const auto& u : units.getVolumeUnits()) {
    ui->cmbVolume->addItem(u.symbol);
  }
  for (const auto& u : units.getAmountUnits()) {
    ui->cmbAmount->addItem(u.symbol);
  }

  ui->cmbTime->setCurrentIndex(units.getTimeIndex());
  ui->cmbLength->setCurrentIndex(units.getLengthIndex());
  ui->cmbVolume->setCurrentIndex(units.getVolumeIndex());
  ui->cmbAmount->setCurrentIndex(units.getAmountIndex());
}

DialogUnits::~DialogUnits() = default;

int DialogUnits::getTimeUnitIndex() const {
  return ui->cmbTime->currentIndex();
}

int DialogUnits::getLengthUnitIndex() const {
  return ui->cmbLength->currentIndex();
}

int DialogUnits::getVolumeUnitIndex() const {
  return ui->cmbVolume->currentIndex();
}

int DialogUnits::getAmountUnitIndex() const {
  return ui->cmbAmount->currentIndex();
}

static QString baseUnitString(const units::Unit& unit) {
  QString s;
  if (unit.exponent != 1) {
    s = "(";
  }
  if (unit.multiplier != 1.0) {
    s.append(QString::number(unit.multiplier, 'g', 14));
    s.append(" ");
    if (unit.scale != 0) {
      s.append("* ");
    }
  }
  if (unit.scale != 0) {
    s.append(QString("10^(%1) ").arg(unit.scale));
  }
  s.append(unit.kind);
  if (unit.exponent != 1) {
    s.append(QString(")^%1").arg(unit.exponent));
  }
  return s;
}

void DialogUnits::cmbTime_currentIndexChanged(int index) {
  const auto& unit = units.getTimeUnits().at(index);
  ui->lblTime->setText(unit.name);
  ui->lblSITime->setText(baseUnitString(unit));
}

void DialogUnits::cmbLength_currentIndexChanged(int index) {
  const auto& unit = units.getLengthUnits().at(index);
  ui->lblLength->setText(unit.name);
  ui->lblSILength->setText(baseUnitString(unit));
}

void DialogUnits::cmbVolume_currentIndexChanged(int index) {
  const auto& unit = units.getVolumeUnits().at(index);
  ui->lblVolume->setText(unit.name);
  ui->lblSIVolume->setText(baseUnitString(unit));
}

void DialogUnits::cmbAmount_currentIndexChanged(int index) {
  const auto& unit = units.getAmountUnits().at(index);
  ui->lblAmount->setText(unit.name);
  ui->lblSIAmount->setText(baseUnitString(unit));
}
