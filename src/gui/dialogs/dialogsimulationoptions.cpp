#include "dialogsimulationoptions.hpp"
#include "logger.hpp"
#include "ui_dialogsimulationoptions.h"
#include "utils.hpp"
#include <QString>

static QString dblToQString(double x) {
  return QString("%1").arg(x, 0, 'e', 5);
}

static int toIndex(simulate::PixelIntegratorType integrator) {
  switch (integrator) {
  case simulate::PixelIntegratorType::RK101:
    return 0;
    break;
  case simulate::PixelIntegratorType::RK212:
    return 1;
    break;
  case simulate::PixelIntegratorType::RK323:
    return 2;
    break;
  case simulate::PixelIntegratorType::RK435:
    return 3;
    break;
  default:
    return 0;
  }
}

static simulate::PixelIntegratorType toEnum(int index) {
  switch (index) {
  case 0:
    return simulate::PixelIntegratorType::RK101;
    break;
  case 1:
    return simulate::PixelIntegratorType::RK212;
    break;
  case 2:
    return simulate::PixelIntegratorType::RK323;
    break;
  case 3:
    return simulate::PixelIntegratorType::RK435;
    break;
  default:
    return simulate::PixelIntegratorType::RK101;
  }
}

DialogSimulationOptions::DialogSimulationOptions(
    const simulate::Options &options, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogSimulationOptions>()},
      opt{options} {
  ui->setupUi(this);
  setupConnections();
  loadDuneOpts();
  loadPixelOpts();
}

DialogSimulationOptions::~DialogSimulationOptions() = default;

const simulate::Options &DialogSimulationOptions::getOptions() const {
  return opt;
}

void DialogSimulationOptions::setupConnections() {
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogSimulationOptions::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogSimulationOptions::reject);
  // Dune tab
  connect(ui->txtDuneDt, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneDt_editingFinished);
  connect(ui->chkDuneVTK, &QCheckBox::stateChanged, this,
          &DialogSimulationOptions::chkDuneVTK_stateChanged);
  connect(ui->btnDuneReset, &QPushButton::clicked, this,
          &DialogSimulationOptions::resetDuneToDefaults);
  // Pixel tab
  connect(ui->cmbPixelIntegrator,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          &DialogSimulationOptions::cmbPixelIntegrator_currentIndexChanged);
  connect(ui->txtPixelAbsErr, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtPixelAbsErr_editingFinished);
  connect(ui->txtPixelRelErr, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtPixelRelErr_editingFinished);
  connect(ui->txtPixelDt, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtPixelDt_editingFinished);
  connect(ui->chkPixelMultithread, &QCheckBox::stateChanged, this,
          &DialogSimulationOptions::chkPixelMultithread_stateChanged);
  connect(ui->spnPixelThreads, qOverload<int>(&QSpinBox::valueChanged), this,
          &DialogSimulationOptions::spnPixelThreads_valueChanged);
  connect(ui->chkPixelCSE, &QCheckBox::stateChanged, this,
          &DialogSimulationOptions::chkPixelCSE_stateChanged);
  connect(ui->spnPixelOptLevel, qOverload<int>(&QSpinBox::valueChanged), this,
          &DialogSimulationOptions::spnPixelOptLevel_valueChanged);
  connect(ui->btnPixelReset, &QPushButton::clicked, this,
          &DialogSimulationOptions::resetPixelToDefaults);
}

void DialogSimulationOptions::loadDuneOpts() {
  ui->txtDuneDt->setText(dblToQString(opt.dune.dt));
  ui->chkDuneVTK->setChecked(opt.dune.writeVTKfiles);
}

void DialogSimulationOptions::txtDuneDt_editingFinished() {
  opt.dune.dt = ui->txtDuneDt->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::chkDuneVTK_stateChanged() {
  opt.dune.writeVTKfiles = ui->chkDuneVTK->isChecked();
}

void DialogSimulationOptions::resetDuneToDefaults() {
  opt.dune = simulate::DuneOptions{};
  loadDuneOpts();
}

void DialogSimulationOptions::loadPixelOpts() {
  ui->cmbPixelIntegrator->setCurrentIndex(toIndex(opt.pixel.integrator));
  ui->txtPixelRelErr->setText(dblToQString(opt.pixel.maxErr.rel));
  ui->txtPixelAbsErr->setText(dblToQString(opt.pixel.maxErr.abs));
  ui->txtPixelDt->setText(dblToQString(opt.pixel.maxTimestep));
  ui->chkPixelMultithread->setChecked(opt.pixel.enableMultiThreading);
#ifndef SPATIAL_MODEL_EDITOR_USE_TBB
  ui->chkPixelMultithread->setChecked(false);
  ui->chkPixelMultithread->setEnabled(false);
  ui->txtPixelThreads->setText(1);
  ui->txtPixelThreads->setEnabled(false);
#endif
  if (opt.pixel.enableMultiThreading) {
    ui->spnPixelThreads->setEnabled(true);
    int threads = static_cast<int>(opt.pixel.maxThreads);
    if (threads > ui->spnPixelThreads->maximum()) {
      threads = 0;
    }
    ui->spnPixelThreads->setValue(threads);
  } else {
    ui->spnPixelThreads->setEnabled(false);
  }
  ui->chkPixelCSE->setChecked(opt.pixel.doCSE);
  int lvl = static_cast<int>(opt.pixel.optLevel);
  if (lvl > ui->spnPixelOptLevel->maximum()) {
    lvl = ui->spnPixelOptLevel->maximum();
  }
  ui->spnPixelOptLevel->setValue(lvl);
}

void DialogSimulationOptions::cmbPixelIntegrator_currentIndexChanged(
    int index) {
  opt.pixel.integrator = toEnum(index);
}

void DialogSimulationOptions::txtPixelAbsErr_editingFinished() {
  opt.pixel.maxErr.abs = ui->txtPixelAbsErr->text().toDouble();
  loadPixelOpts();
}

void DialogSimulationOptions::txtPixelRelErr_editingFinished() {
  opt.pixel.maxErr.rel = ui->txtPixelRelErr->text().toDouble();
  loadPixelOpts();
}

void DialogSimulationOptions::txtPixelDt_editingFinished() {
  opt.pixel.maxTimestep = ui->txtPixelDt->text().toDouble();
  loadPixelOpts();
}

void DialogSimulationOptions::chkPixelMultithread_stateChanged() {
  opt.pixel.enableMultiThreading = ui->chkPixelMultithread->isChecked();
  loadPixelOpts();
}

void DialogSimulationOptions::spnPixelThreads_valueChanged(int value) {
  opt.pixel.maxThreads = static_cast<std::size_t>(value);
  loadPixelOpts();
}

void DialogSimulationOptions::chkPixelCSE_stateChanged() {
  opt.pixel.doCSE = ui->chkPixelCSE->isChecked();
}

void DialogSimulationOptions::spnPixelOptLevel_valueChanged(int value) {
  opt.pixel.optLevel = static_cast<unsigned>(value);
}

void DialogSimulationOptions::resetPixelToDefaults() {
  opt.pixel = simulate::PixelOptions{};
  loadPixelOpts();
}
