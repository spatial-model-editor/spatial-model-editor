#include "tabparameters.hpp"
#include <QInputDialog>
#include <QMessageBox>
#include "guiutils.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "ui_tabparameters.h"
#include "utils.hpp"

TabParameters::TabParameters(sme::model::Model &m, QWidget *parent)
    : QWidget{parent}, ui{std::make_unique<Ui::TabParameters>()}, model{m} {
  ui->setupUi(this);
  connect(ui->listParameters, &QListWidget::currentRowChanged, this,
          &TabParameters::listParameters_currentRowChanged);
  connect(ui->btnAddParameter, &QPushButton::clicked, this,
          &TabParameters::btnAddParameter_clicked);
  connect(ui->btnRemoveParameter, &QPushButton::clicked, this,
          &TabParameters::btnRemoveParameter_clicked);
  connect(ui->txtParameterName, &QLineEdit::editingFinished, this,
          &TabParameters::txtParameterName_editingFinished);
  connect(ui->txtExpression, &QPlainTextMathEdit::mathChanged, this,
          &TabParameters::txtExpression_mathChanged);
}

TabParameters::~TabParameters() = default;

void TabParameters::loadModelData(const QString &selection) {
  currentParameterId.clear();
  ui->listParameters->clear();
  ui->txtExpression->clearVariables();
  ui->txtExpression->resetToDefaultFunctions();
  for (const auto &[id, name] : model.getParameters().getSymbols()) {
    ui->txtExpression->addVariable(id, name);
  }
  for (const auto &function : model.getFunctions().getSymbolicFunctions()) {
    ui->txtExpression->addFunction(function);
  }
  for (const auto &id : model.getParameters().getIds()) {
    auto name = model.getParameters().getName(id);
    ui->listParameters->addItem(name);
  }
  ui->lblExpressionStatus->clear();
  selectMatchingOrFirstItem(ui->listParameters, selection);
  bool enable = ui->listParameters->count() > 0;
  ui->txtParameterName->setEnabled(enable);
  ui->txtExpression->setEnabled(enable);
}

void TabParameters::listParameters_currentRowChanged(int row) {
  currentParameterId.clear();
  ui->txtParameterName->clear();
  ui->txtExpression->clear();
  ui->lblExpressionStatus->clear();
  ui->txtParameterName->setEnabled(false);
  ui->txtExpression->setEnabled(false);
  ui->btnRemoveParameter->setEnabled(false);
  if ((row < 0) || (row > model.getParameters().getIds().size() - 1)) {
    return;
  }
  const auto &params = model.getParameters();
  currentParameterId = params.getIds()[row];
  SPDLOG_DEBUG("Parameter {} selected", currentParameterId.toStdString());
  ui->txtParameterName->setText(params.getName(currentParameterId));
  ui->txtExpression->importVariableMath(
      params.getExpression(currentParameterId).toStdString());
  ui->txtParameterName->setEnabled(true);
  ui->txtExpression->setEnabled(true);
  ui->btnRemoveParameter->setEnabled(true);
}

void TabParameters::btnAddParameter_clicked() {
  bool ok{false};
  auto paramName = QInputDialog::getText(
      this, "Add parameter", "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok && !paramName.isEmpty()) {
    auto newParamName = model.getParameters().add(paramName);
    loadModelData(newParamName);
  }
}

void TabParameters::btnRemoveParameter_clicked() {
  int row = ui->listParameters->currentRow();
  if ((row < 0) || (row > model.getParameters().getIds().size() - 1)) {
    return;
  }
  auto msgbox =
      newYesNoMessageBox("Remove parameter?",
                         QString("Remove parameter '%1' from the model?")
                             .arg(ui->listParameters->currentItem()->text()),
                         this);
  connect(msgbox, &QMessageBox::finished, this, [this](int result) {
    if (result == QMessageBox::Yes) {
      SPDLOG_INFO("Removing parameter {}", currentParameterId.toStdString());
      model.getParameters().remove(currentParameterId);
      loadModelData();
    }
  });
  msgbox->open();
}

void TabParameters::txtParameterName_editingFinished() {
  const QString &name = ui->txtParameterName->text();
  if (name == model.getSpecies().getName(currentParameterId)) {
    return;
  }
  auto newName = model.getParameters().setName(currentParameterId, name);
  ui->txtParameterName->setText(newName);
  loadModelData(newName);
}

void TabParameters::txtExpression_mathChanged(const QString &math, bool valid,
                                              const QString &errorMessage) {
  if (!ui->txtExpression->isEnabled()) {
    ui->lblExpressionStatus->setText("");
    return;
  }
  if (!valid) {
    SPDLOG_INFO("math err: {}", errorMessage.toStdString());
    ui->lblExpressionStatus->setText(errorMessage);
    return;
  }
  SPDLOG_INFO("new math: {}", math.toStdString());
  ui->lblExpressionStatus->setText("");
  model.getParameters().setExpression(currentParameterId, math);
}
