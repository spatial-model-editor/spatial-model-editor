#include "dialogintegratoroptions.hpp"

#include <QString>

#include "logger.hpp"
#include "simulate.hpp"
#include "ui_dialogintegratoroptions.h"

static QString dblToQString(double x) {
  return QString("%1").arg(x, 20, 'e', 17);
}

DialogIntegratorOptions::DialogIntegratorOptions(
    const simulate::IntegratorOptions& options, QWidget* parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogIntegratorOptions>()} {
  ui->setupUi(this);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogIntegratorOptions::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogIntegratorOptions::reject);
  connect(ui->btnReset, &QPushButton::clicked, this,
          &DialogIntegratorOptions::resetToDefaults);

  ui->cmbOrder->setCurrentIndex(static_cast<int>(options.order - 1));
  ui->txtRelErr->setText(dblToQString(options.maxRelErr));
  ui->txtAbsErr->setText(dblToQString(options.maxAbsErr));
  ui->txtDt->setText(dblToQString(options.maxTimestep));
}

DialogIntegratorOptions::~DialogIntegratorOptions() = default;

simulate::IntegratorOptions DialogIntegratorOptions::getIntegratorOptions()
    const {
  simulate::IntegratorOptions options;
  options.order = getOrder();
  options.maxRelErr = getMaxRelErr();
  options.maxAbsErr = getMaxAbsErr();
  options.maxTimestep = getMaxDt();
  return options;
}
std::size_t DialogIntegratorOptions::getOrder() const {
  return static_cast<std::size_t>(ui->cmbOrder->currentText().toInt());
}

double DialogIntegratorOptions::getMaxRelErr() const {
  return ui->txtRelErr->text().toDouble();
}

double DialogIntegratorOptions::getMaxAbsErr() const {
  return ui->txtAbsErr->text().toDouble();
}

double DialogIntegratorOptions::getMaxDt() const {
  return ui->txtDt->text().toDouble();
}

void DialogIntegratorOptions::resetToDefaults() {
  ui->cmbOrder->setCurrentIndex(1);
  ui->txtRelErr->setText("0.01");
  ui->txtAbsErr->setText(dblToQString(std::numeric_limits<double>::max()));
  ui->txtDt->setText(dblToQString(std::numeric_limits<double>::max()));
}
