#include "dialogeditunit.hpp"
#include "logger.hpp"
#include "ui_dialogeditunit.h"
#include <QPushButton>

DialogEditUnit::DialogEditUnit(const sme::model::Unit &unit, const QString &unitType,
                               QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogEditUnit>()}, u(unit) {
  ui->setupUi(this);
  if (!unitType.isEmpty()) {
    setWindowTitle(QString("Edit Unit of %1").arg(unitType));
  }
  ui->txtName->setText(u.name);
  ui->txtMultipler->setText(QString::number(u.multiplier));
  ui->txtScale->setText(QString::number(u.scale));
  ui->txtExponent->setText(QString::number(u.exponent));

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogEditUnit::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogEditUnit::reject);
  connect(ui->txtName, &QLineEdit::textEdited, this,
          &DialogEditUnit::txtName_textEdited);
  connect(ui->txtMultipler, &QLineEdit::textEdited, this,
          &DialogEditUnit::txtMultiplier_textEdited);
  connect(ui->txtScale, &QLineEdit::textEdited, this,
          &DialogEditUnit::txtScale_textEdited);
  connect(ui->txtExponent, &QLineEdit::textEdited, this,
          &DialogEditUnit::txtExponent_textEdited);

  updateLblBaseUnits();
}

DialogEditUnit::~DialogEditUnit() = default;

const sme::model::Unit &DialogEditUnit::getUnit() const { return u; }

void DialogEditUnit::updateLblBaseUnits() {
  if (validScale && validExponent && validMultipler) {
    ui->lblBaseUnits->setText(
        QString("1 %1 = %2").arg(u.name).arg(sme::model::unitInBaseUnits(u)));
  }
}

void DialogEditUnit::setIsValidState(QWidget *widget, bool valid,
                                     const QString &errorMessage) {
  ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)
      ->setEnabled(valid);
  if (valid) {
    widget->setStyleSheet({});
    return;
  }
  widget->setStyleSheet("background-color: red");
  ui->lblBaseUnits->setText(errorMessage);
}

void DialogEditUnit::txtName_textEdited(const QString &text) {
  u.name = text;
  updateLblBaseUnits();
}

void DialogEditUnit::txtMultiplier_textEdited(const QString &text) {
  double newMultiplier = text.toDouble(&validMultipler);
  if (validMultipler) {
    u.multiplier = newMultiplier;
  }
  setIsValidState(ui->txtMultipler, validMultipler,
                  "Invalid value: Multiplier must be a double");
  updateLblBaseUnits();
}

void DialogEditUnit::txtScale_textEdited(const QString &text) {
  int newScale = text.toInt(&validScale);
  if (validScale) {
    u.scale = newScale;
  }
  setIsValidState(ui->txtScale, validScale,
                  "Invalid value: Scale must be an integer");
  updateLblBaseUnits();
}

void DialogEditUnit::txtExponent_textEdited(const QString &text) {
  int newExponent = text.toInt(&validExponent);
  if (validExponent) {
    u.exponent = newExponent;
  }
  setIsValidState(ui->txtScale, validExponent,
                  "Invalid value: Exponent must be an integer");
  updateLblBaseUnits();
}
