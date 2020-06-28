#include "tabparameters.hpp"

#include "guiutils.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "ui_tabparameters.h"
#include "utils.hpp"
#include <QInputDialog>
#include <QMessageBox>

TabParameters::TabParameters(model::Model &doc, QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabParameters>()}, sbmlDoc(doc) {
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
  auto *list = ui->listParameters;
  list->clear();
  ui->txtExpression->clearVariables();
  ui->txtExpression->clearFunctions();
  ui->txtExpression->addDefaultFunctions();
  for (const auto &[id, name] : sbmlDoc.getParameters().getSymbols()) {
    ui->txtExpression->addVariable(id, name);
  }
  for (const auto &id : sbmlDoc.getFunctions().getIds()) {
    auto name = sbmlDoc.getFunctions().getName(id);
    ui->txtExpression->addFunction(id.toStdString(), name.toStdString());
  }
  for (const auto &id : sbmlDoc.getParameters().getIds()) {
    auto name = sbmlDoc.getParameters().getName(id);
    list->addItem(name);
  }
  ui->lblExpressionStatus->clear();
  selectMatchingOrFirstItem(list, selection);
  bool enable = list->count() > 0;
  ui->txtParameterName->setEnabled(enable);
  ui->txtExpression->setEnabled(enable);
}

void TabParameters::listParameters_currentRowChanged(int row) {
  ui->txtParameterName->clear();
  ui->txtExpression->clear();
  ui->lblExpressionStatus->clear();
  if ((row < 0) || (row > sbmlDoc.getParameters().getIds().size() - 1)) {
    ui->txtParameterName->setEnabled(false);
    ui->txtExpression->setEnabled(false);
    ui->btnRemoveParameter->setEnabled(false);
    return;
  }
  const auto &params = sbmlDoc.getParameters();
  auto id = params.getIds()[row];
  SPDLOG_DEBUG("Parameter {} selected", id.toStdString());
  ui->txtParameterName->setText(params.getName(id));
  ui->txtExpression->importVariableMath(params.getExpression(id).toStdString());
  ui->txtParameterName->setEnabled(true);
  ui->txtExpression->setEnabled(true);
  ui->btnRemoveParameter->setEnabled(true);
}

void TabParameters::btnAddParameter_clicked() {
  bool ok;
  auto paramName = QInputDialog::getText(
      this, "Add parameter", "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok) {
    sbmlDoc.getParameters().add(paramName);
    loadModelData(paramName);
  }
}

void TabParameters::btnRemoveParameter_clicked() {
  int row = ui->listParameters->currentRow();
  if ((row < 0) || (row > sbmlDoc.getParameters().getIds().size() - 1)) {
    return;
  }
  auto msgbox =
      newYesNoMessageBox("Remove parameter?",
                         QString("Remove parameter '%1' from the model?")
                             .arg(ui->listParameters->currentItem()->text()),
                         this);
  connect(msgbox, &QMessageBox::finished, this,
          [&s = sbmlDoc, row, this](int result) {
            if (result == QMessageBox::Yes) {
              auto paramId = s.getParameters().getIds()[row];
              SPDLOG_INFO("Removing parameter {}", paramId.toStdString());
              s.getParameters().remove(paramId);
              this->loadModelData();
            }
          });
  msgbox->open();
}

void TabParameters::txtParameterName_editingFinished() {
  const QString &name = ui->txtParameterName->text();
  auto id = sbmlDoc.getParameters().getIds()[ui->listParameters->currentRow()];
  if (name == sbmlDoc.getSpecies().getName(id)) {
    return;
  }
  QString newName = sbmlDoc.getParameters().setName(id, name);
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
  auto id = sbmlDoc.getParameters().getIds()[ui->listParameters->currentRow()];
  sbmlDoc.getParameters().setExpression(id, math);
}
