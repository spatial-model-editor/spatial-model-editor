#include "dialogoptparam.hpp"
#include "sme/logger.hpp"
#include "ui_dialogoptparam.h"
#include <QPushButton>

static QString toQStr(double x) { return QString::number(x, 'g', 14); }

DialogOptParam::DialogOptParam(
    const std::vector<sme::simulate::OptParam> &defaultOptParams,
    const sme::simulate::OptParam *initialOptParam, QWidget *parent)
    : defaultOptParams{defaultOptParams},
      QDialog(parent), ui{std::make_unique<Ui::DialogOptParam>()} {
  ui->setupUi(this);
  int cmbIndex{0};
  if (defaultOptParams.empty()) {
    // model has no parameters: disable everything except cancel button
    ui->cmbParameter->setEnabled(false);
    ui->txtLowerBound->setEnabled(false);
    ui->txtUpperBound->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  } else {
    if (initialOptParam != nullptr) {
      optParam = *initialOptParam;
    } else {
      optParam = defaultOptParams.front();
    }
    ui->txtLowerBound->setText(toQStr(optParam.lowerBound));
    ui->txtUpperBound->setText(toQStr(optParam.upperBound));
    for (const auto &defaultOptParam : defaultOptParams) {
      ui->cmbParameter->addItem(defaultOptParam.name);
      if (defaultOptParam.optParamType == optParam.optParamType &&
          defaultOptParam.id == optParam.id &&
          defaultOptParam.parentId == optParam.parentId) {
        cmbIndex = ui->cmbParameter->count() - 1;
      }
    }
    ui->cmbParameter->setCurrentIndex(cmbIndex);
    connect(ui->cmbParameter, &QComboBox::currentIndexChanged, this,
            &DialogOptParam::cmbParameter_currentIndexChanged);
    connect(ui->txtLowerBound, &QLineEdit::editingFinished, this, [this]() {
      optParam.lowerBound = ui->txtLowerBound->text().toDouble();
    });
    connect(ui->txtUpperBound, &QLineEdit::editingFinished, this, [this]() {
      optParam.upperBound = ui->txtUpperBound->text().toDouble();
    });
  }
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogOptParam::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogOptParam::reject);
}

DialogOptParam::~DialogOptParam() = default;

const sme::simulate::OptParam &DialogOptParam::getOptParam() const {
  return optParam;
}

void DialogOptParam::cmbParameter_currentIndexChanged(int index) {
  optParam = defaultOptParams[index];
  ui->txtLowerBound->setText(toQStr(optParam.lowerBound));
  ui->txtUpperBound->setText(toQStr(optParam.upperBound));
}
