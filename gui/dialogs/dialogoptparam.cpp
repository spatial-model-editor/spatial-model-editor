#include "dialogoptparam.hpp"
#include "sme/logger.hpp"
#include "ui_dialogoptparam.h"
#include <QPushButton>

static QString toQStr(double x) { return QString::number(x, 'g', 14); }

DialogOptParam::DialogOptParam(
    const std::vector<sme::simulate::OptParam> &defaultOptParams,
    const sme::simulate::OptParam *initialOptParam, QWidget *parent)
    : QDialog(parent), m_defaultOptParams{defaultOptParams},
      ui{std::make_unique<Ui::DialogOptParam>()} {
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
      m_optParam = *initialOptParam;
    } else {
      m_optParam = defaultOptParams.front();
    }
    ui->txtLowerBound->setText(toQStr(m_optParam.lowerBound));
    ui->txtUpperBound->setText(toQStr(m_optParam.upperBound));
    for (const auto &defaultOptParam : defaultOptParams) {
      ui->cmbParameter->addItem(defaultOptParam.name.c_str());
      if (defaultOptParam.optParamType == m_optParam.optParamType &&
          defaultOptParam.id == m_optParam.id &&
          defaultOptParam.parentId == m_optParam.parentId) {
        cmbIndex = ui->cmbParameter->count() - 1;
      }
    }
    ui->cmbParameter->setCurrentIndex(cmbIndex);
    connect(ui->cmbParameter, &QComboBox::currentIndexChanged, this,
            &DialogOptParam::cmbParameter_currentIndexChanged);
    connect(ui->txtLowerBound, &QLineEdit::editingFinished, this, [this]() {
      m_optParam.lowerBound = ui->txtLowerBound->text().toDouble();
    });
    connect(ui->txtUpperBound, &QLineEdit::editingFinished, this, [this]() {
      m_optParam.upperBound = ui->txtUpperBound->text().toDouble();
    });
  }
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogOptParam::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogOptParam::reject);
}

DialogOptParam::~DialogOptParam() = default;

const sme::simulate::OptParam &DialogOptParam::getOptParam() const {
  return m_optParam;
}

void DialogOptParam::cmbParameter_currentIndexChanged(int index) {
  m_optParam = m_defaultOptParams[static_cast<std::size_t>(index)];
  ui->txtLowerBound->setText(toQStr(m_optParam.lowerBound));
  ui->txtUpperBound->setText(toQStr(m_optParam.upperBound));
}
