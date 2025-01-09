#include "dialogsimulationoptions.hpp"
#include "guiutils.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include "ui_dialogsimulationoptions.h"
#include <QString>
#include <oneapi/tbb/info.h>

static QString dblToQString(double x) {
  return QString("%1").arg(x, 0, 'e', 5);
}

static int toIndex(sme::simulate::PixelIntegratorType integrator) {
  switch (integrator) {
  case sme::simulate::PixelIntegratorType::RK101:
    return 0;
  case sme::simulate::PixelIntegratorType::RK212:
    return 1;
  case sme::simulate::PixelIntegratorType::RK323:
    return 2;
  case sme::simulate::PixelIntegratorType::RK435:
    return 3;
  default:
    return 0;
  }
}

static sme::simulate::PixelIntegratorType toPixelIntegratorEnum(int index) {
  switch (index) {
  case 0:
    return sme::simulate::PixelIntegratorType::RK101;
  case 1:
    return sme::simulate::PixelIntegratorType::RK212;
  case 2:
    return sme::simulate::PixelIntegratorType::RK323;
  case 3:
    return sme::simulate::PixelIntegratorType::RK435;
  default:
    return sme::simulate::PixelIntegratorType::RK101;
  }
}

DialogSimulationOptions::DialogSimulationOptions(
    const sme::simulate::Options &options, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogSimulationOptions>()},
      opt{options} {
  ui->setupUi(this);
  setupConnections();
  loadDuneOpts();
  loadPixelOpts();
}

DialogSimulationOptions::~DialogSimulationOptions() = default;

const sme::simulate::Options &DialogSimulationOptions::getOptions() const {
  return opt;
}

void DialogSimulationOptions::setupConnections() {
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogSimulationOptions::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogSimulationOptions::reject);
  // Dune tab
  connect(ui->cmbDuneIntegrator,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          &DialogSimulationOptions::cmbDuneIntegrator_currentIndexChanged);
  connect(ui->txtDuneDt, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneDt_editingFinished);
  connect(ui->txtDuneMinDt, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneMinDt_editingFinished);
  connect(ui->txtDuneMaxDt, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneMaxDt_editingFinished);
  connect(ui->txtDuneIncrease, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneIncrease_editingFinished);
  connect(ui->txtDuneDecrease, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneDecrease_editingFinished);
  connect(ui->chkDuneVTK, &QCheckBox::checkStateChanged, this,
          &DialogSimulationOptions::chkDuneVTK_stateChanged);
  connect(ui->txtDuneNewtonRel, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneNewtonRel_editingFinished);
  connect(ui->txtDuneNewtonAbs, &QLineEdit::editingFinished, this,
          &DialogSimulationOptions::txtDuneNewtonAbs_editingFinished);
  connect(ui->cmbDuneLinearSolver,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          &DialogSimulationOptions::cmbDuneLinearSolver_currentIndexChanged);
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
  connect(ui->chkPixelMultithread, &QCheckBox::checkStateChanged, this,
          &DialogSimulationOptions::chkPixelMultithread_stateChanged);
  connect(ui->spnPixelThreads, qOverload<int>(&QSpinBox::valueChanged), this,
          &DialogSimulationOptions::spnPixelThreads_valueChanged);
  connect(ui->chkPixelCSE, &QCheckBox::checkStateChanged, this,
          &DialogSimulationOptions::chkPixelCSE_stateChanged);
  connect(ui->spnPixelOptLevel, qOverload<int>(&QSpinBox::valueChanged), this,
          &DialogSimulationOptions::spnPixelOptLevel_valueChanged);
  connect(ui->btnPixelReset, &QPushButton::clicked, this,
          &DialogSimulationOptions::resetPixelToDefaults);
}

void DialogSimulationOptions::loadDuneOpts() {
  selectMatchingOrFirstItem(ui->cmbDuneIntegrator, opt.dune.integrator.c_str());
  opt.dune.integrator = ui->cmbDuneIntegrator->currentText().toStdString();
  ui->txtDuneDt->setText(dblToQString(opt.dune.dt));
  ui->txtDuneMinDt->setText(dblToQString(opt.dune.minDt));
  ui->txtDuneMaxDt->setText(dblToQString(opt.dune.maxDt));
  ui->txtDuneIncrease->setText(dblToQString(opt.dune.increase));
  ui->txtDuneDecrease->setText(dblToQString(opt.dune.decrease));
  ui->chkDuneVTK->setChecked(opt.dune.writeVTKfiles);
  ui->txtDuneNewtonRel->setText(dblToQString(opt.dune.newtonRelErr));
  ui->txtDuneNewtonAbs->setText(dblToQString(opt.dune.newtonAbsErr));
  selectMatchingOrFirstItem(ui->cmbDuneLinearSolver,
                            opt.dune.linearSolver.c_str());
  opt.dune.linearSolver = ui->cmbDuneLinearSolver->currentText().toStdString();
}

void DialogSimulationOptions::cmbDuneIntegrator_currentIndexChanged(
    [[maybe_unused]] int index) {
  opt.dune.integrator = ui->cmbDuneIntegrator->currentText().toStdString();
}

void DialogSimulationOptions::txtDuneDt_editingFinished() {
  opt.dune.dt = ui->txtDuneDt->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::txtDuneMinDt_editingFinished() {
  opt.dune.minDt = ui->txtDuneMinDt->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::txtDuneMaxDt_editingFinished() {
  opt.dune.maxDt = ui->txtDuneMaxDt->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::txtDuneIncrease_editingFinished() {
  opt.dune.increase = ui->txtDuneIncrease->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::txtDuneDecrease_editingFinished() {
  opt.dune.decrease = ui->txtDuneDecrease->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::chkDuneVTK_stateChanged() {
  opt.dune.writeVTKfiles = ui->chkDuneVTK->isChecked();
}

void DialogSimulationOptions::txtDuneNewtonRel_editingFinished() {
  opt.dune.newtonRelErr = ui->txtDuneNewtonRel->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::txtDuneNewtonAbs_editingFinished() {
  opt.dune.newtonAbsErr = ui->txtDuneNewtonAbs->text().toDouble();
  loadDuneOpts();
}

void DialogSimulationOptions::cmbDuneLinearSolver_currentIndexChanged(
    [[maybe_unused]] int index) {
  opt.dune.linearSolver = ui->cmbDuneLinearSolver->currentText().toStdString();
}

void DialogSimulationOptions::resetDuneToDefaults() {
  opt.dune = sme::simulate::DuneOptions{};
  loadDuneOpts();
}

void DialogSimulationOptions::loadPixelOpts() {
  ui->cmbPixelIntegrator->setCurrentIndex(toIndex(opt.pixel.integrator));
  ui->txtPixelRelErr->setText(dblToQString(opt.pixel.maxErr.rel));
  ui->txtPixelAbsErr->setText(dblToQString(opt.pixel.maxErr.abs));
  ui->txtPixelDt->setText(dblToQString(opt.pixel.maxTimestep));
  ui->chkPixelMultithread->setChecked(opt.pixel.enableMultiThreading);
  ui->spnPixelThreads->setMaximum(oneapi::tbb::info::default_concurrency());
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
  opt.pixel.integrator = toPixelIntegratorEnum(index);
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
  opt.pixel = sme::simulate::PixelOptions{};
  loadPixelOpts();
}
