#include "dialogunits.hpp"
#include "dialogeditunit.hpp"
#include "logger.hpp"
#include "ui_dialogunits.h"

DialogUnits::DialogUnits(model::ModelUnits &modelUnits, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogUnits>()},
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
  connect(ui->btnEditTime, &QPushButton::pressed, this,
          &DialogUnits::btnEditTime_pressed);
  connect(ui->btnEditLength, &QPushButton::pressed, this,
          &DialogUnits::btnEditLength_pressed);
  connect(ui->btnEditVolume, &QPushButton::pressed, this,
          &DialogUnits::btnEditVolume_pressed);
  connect(ui->btnEditAmount, &QPushButton::pressed, this,
          &DialogUnits::btnEditAmount_pressed);
  for (const auto &u : units.getTimeUnits()) {
    ui->cmbTime->addItem(u.name);
  }
  for (const auto &u : units.getLengthUnits()) {
    ui->cmbLength->addItem(u.name);
  }
  for (const auto &u : units.getVolumeUnits()) {
    ui->cmbVolume->addItem(u.name);
  }
  for (const auto &u : units.getAmountUnits()) {
    ui->cmbAmount->addItem(u.name);
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

void DialogUnits::cmbTime_currentIndexChanged(int index) {
  const auto &unit = units.getTimeUnits().at(index);
  ui->lblSITime->setText(model::unitInBaseUnits(unit));
}

void DialogUnits::cmbLength_currentIndexChanged(int index) {
  const auto &unit = units.getLengthUnits().at(index);
  ui->lblSILength->setText(model::unitInBaseUnits(unit));
}

void DialogUnits::cmbVolume_currentIndexChanged(int index) {
  const auto &unit = units.getVolumeUnits().at(index);
  ui->lblSIVolume->setText(model::unitInBaseUnits(unit));
}

void DialogUnits::cmbAmount_currentIndexChanged(int index) {
  const auto &unit = units.getAmountUnits().at(index);
  ui->lblSIAmount->setText(model::unitInBaseUnits(unit));
}

void DialogUnits::btnEditTime_pressed() {
  int i{ui->cmbTime->currentIndex()};
  auto &u = units.getTimeUnits()[i];
  DialogEditUnit dialog(u, "Time");
  if (dialog.exec() == QDialog::Accepted) {
    u = dialog.getUnit();
    SPDLOG_INFO("New unit of time: {} ({})", u.name.toStdString());
    SPDLOG_INFO("  = {}", model::unitInBaseUnits(u).toStdString());
    ui->cmbTime->setItemText(i, u.name);
    cmbTime_currentIndexChanged(i);
  }
}

void DialogUnits::btnEditLength_pressed() {
  int i{ui->cmbLength->currentIndex()};
  auto &u = units.getLengthUnits()[i];
  DialogEditUnit dialog(u, "Length");
  if (dialog.exec() == QDialog::Accepted) {
    u = dialog.getUnit();
    SPDLOG_INFO("New unit of length: {} ({})", u.name.toStdString());
    SPDLOG_INFO("  = {}", model::unitInBaseUnits(u).toStdString());
    ui->cmbLength->setItemText(i, u.name);
    cmbLength_currentIndexChanged(i);
  }
}

void DialogUnits::btnEditVolume_pressed() {
  int i{ui->cmbVolume->currentIndex()};
  auto &u = units.getVolumeUnits()[i];
  DialogEditUnit dialog(u, "Volume");
  if (dialog.exec() == QDialog::Accepted) {
    u = dialog.getUnit();
    SPDLOG_INFO("New unit of volume: {} ({})", u.name.toStdString());
    SPDLOG_INFO("  = {}", model::unitInBaseUnits(u).toStdString());
    ui->cmbVolume->setItemText(i, u.name);
    cmbVolume_currentIndexChanged(i);
  }
}

void DialogUnits::btnEditAmount_pressed() {
  int i{ui->cmbAmount->currentIndex()};
  auto &u = units.getAmountUnits()[i];
  DialogEditUnit dialog(u, "Amount");
  if (dialog.exec() == QDialog::Accepted) {
    u = dialog.getUnit();
    SPDLOG_INFO("New unit of amount: {} ({})", u.name.toStdString());
    SPDLOG_INFO("  = {}", model::unitInBaseUnits(u).toStdString());
    ui->cmbAmount->setItemText(i, u.name);
    cmbAmount_currentIndexChanged(i);
  }
}
