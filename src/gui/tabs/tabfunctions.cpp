#include "tabfunctions.hpp"
#include <QInputDialog>
#include <QMessageBox>
#include "guiutils.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "ui_tabfunctions.h"
#include "utils.hpp"

TabFunctions::TabFunctions(sme::model::Model &m, QWidget *parent)
    : QWidget{parent}, ui{std::make_unique<Ui::TabFunctions>()}, model{m} {
  ui->setupUi(this);
  connect(ui->listFunctions, &QListWidget::currentRowChanged, this,
          &TabFunctions::listFunctions_currentRowChanged);
  connect(ui->btnAddFunction, &QPushButton::clicked, this,
          &TabFunctions::btnAddFunction_clicked);
  connect(ui->btnRemoveFunction, &QPushButton::clicked, this,
          &TabFunctions::btnRemoveFunction_clicked);
  connect(ui->txtFunctionName, &QLineEdit::editingFinished, this,
          &TabFunctions::txtFunctionName_editingFinished);
  connect(ui->listFunctionParams, &QListWidget::currentRowChanged, this,
          &TabFunctions::listFunctionParams_currentRowChanged);
  connect(ui->btnAddFunctionParam, &QPushButton::clicked, this,
          &TabFunctions::btnAddFunctionParam_clicked);
  connect(ui->btnRemoveFunctionParam, &QPushButton::clicked, this,
          &TabFunctions::btnRemoveFunctionParam_clicked);
  connect(ui->txtFunctionDef, &QPlainTextMathEdit::mathChanged, this,
          &TabFunctions::txtFunctionDef_mathChanged);
}

TabFunctions::~TabFunctions() = default;

void TabFunctions::clearDisplay() {
  currentFunctionId.clear();
  ui->txtFunctionName->clear();
  ui->listFunctionParams->clear();
  ui->txtFunctionDef->clear();
  ui->lblFunctionDefStatus->clear();
  ui->btnAddFunctionParam->setEnabled(false);
  ui->btnRemoveFunctionParam->setEnabled(false);
  ui->btnRemoveFunction->setEnabled(false);
}

void TabFunctions::loadModelData(const QString &selection) {
  clearDisplay();
  ui->listFunctions->clear();
  ui->listFunctions->addItems(model.getFunctions().getNames());
  selectMatchingOrFirstItem(ui->listFunctions, selection);
  bool enable = ui->listFunctions->count() > 0;
  ui->txtFunctionName->setEnabled(enable);
  ui->listFunctionParams->setEnabled(enable);
  ui->txtFunctionDef->setEnabled(enable);
}

void TabFunctions::listFunctions_currentRowChanged(int row) {
  clearDisplay();
  if ((row < 0) || (row > model.getFunctions().getIds().size() - 1)) {
    return;
  }
  const auto &funcs = model.getFunctions();
  auto id = funcs.getIds()[row];
  currentFunctionId = id;
  SPDLOG_DEBUG("Function {} selected", id.toStdString());
  ui->txtFunctionName->setText(funcs.getName(id));
  auto args = funcs.getArguments(id);
  // reset variables to only built-in functions
  ui->txtFunctionDef->reset();
  ui->txtFunctionDef->setVariables(sme::utils::toStdString(args));
  // add model functions
  for (const auto &function : model.getFunctions().getSymbolicFunctions()) {
    ui->txtFunctionDef->addFunction(function);
  }
  ui->listFunctionParams->addItems(args);
  if (!args.isEmpty()) {
    ui->listFunctionParams->setCurrentRow(0);
  } else {
    ui->btnRemoveFunctionParam->setEnabled(false);
  }
  ui->txtFunctionDef->importVariableMath(funcs.getExpression(id).toStdString());
  ui->btnAddFunctionParam->setEnabled(true);
  ui->btnRemoveFunctionParam->setEnabled(!args.isEmpty());
  ui->btnRemoveFunction->setEnabled(true);
}

void TabFunctions::btnAddFunction_clicked() {
  bool ok{false};
  auto functionName = QInputDialog::getText(
      this, "Add function", "New function name:", QLineEdit::Normal, {}, &ok);
  if (ok && !functionName.isEmpty()) {
    auto newFunctionName = model.getFunctions().add(functionName);
    loadModelData(newFunctionName);
  }
}

void TabFunctions::btnRemoveFunction_clicked() {
  if (currentFunctionId.isEmpty()) {
    return;
  }
  auto msgbox =
      newYesNoMessageBox("Remove function?",
                         QString("Remove function '%1' from the model?")
                             .arg(ui->listFunctions->currentItem()->text()),
                         this);
  connect(msgbox, &QMessageBox::finished, this, [this](int result) {
    if (result == QMessageBox::Yes) {
      SPDLOG_INFO("Removing function {}", currentFunctionId.toStdString());
      model.getFunctions().remove(currentFunctionId);
      this->loadModelData();
    }
  });
  msgbox->open();
}

void TabFunctions::txtFunctionName_editingFinished() {
  const QString &name = ui->txtFunctionName->text();
  if (name == model.getSpecies().getName(currentFunctionId)) {
    return;
  }
  auto newName = model.getFunctions().setName(currentFunctionId, name);
  ui->txtFunctionName->setText(newName);
  loadModelData(newName);
}

void TabFunctions::listFunctionParams_currentRowChanged(int row) {
  bool valid = (row >= 0) && (row < ui->listFunctionParams->count());
  ui->btnRemoveFunctionParam->setEnabled(valid);
}

void TabFunctions::btnAddFunctionParam_clicked() {
  bool ok{false};
  auto param =
      QInputDialog::getText(this, "Add function parameter",
                            "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok && !param.isEmpty()) {
    SPDLOG_INFO("Adding parameter {}", param.toStdString());
    auto argId = model.getFunctions().addArgument(currentFunctionId, param);
    ui->listFunctionParams->addItem(argId);
    ui->txtFunctionDef->addVariable(param.toStdString());
    ui->listFunctionParams->setCurrentRow(ui->listFunctionParams->count() - 1);
  }
}

void TabFunctions::btnRemoveFunctionParam_clicked() {
  int row = ui->listFunctionParams->currentRow();
  if ((row < 0) || (row > ui->listFunctionParams->count() - 1)) {
    return;
  }
  auto *item = ui->listFunctionParams->currentItem();
  const auto param = item->text();
  auto msgbox = newYesNoMessageBox(
      "Remove function parameter?",
      QString("Remove function parameter '%1' from the model?").arg(param),
      this);
  connect(msgbox, &QMessageBox::finished, this, [item, this](int result) {
    if (result == QMessageBox::Yes) {
      auto paramId = item->text().toStdString();
      SPDLOG_INFO("Removing parameter {}", paramId);
      ui->txtFunctionDef->removeVariable(paramId);
      model.getFunctions().removeArgument(currentFunctionId, item->text());
      delete item;
    }
  });
  msgbox->open();
}

void TabFunctions::txtFunctionDef_mathChanged(const QString &math, bool valid,
                                              const QString &errorMessage) {
  if (!valid) {
    SPDLOG_INFO("math err: {}", errorMessage.toStdString());
    ui->lblFunctionDefStatus->setText(errorMessage);
    return;
  }
  SPDLOG_INFO("new math: {}", math.toStdString());
  ui->lblFunctionDefStatus->setText("");
  model.getFunctions().setExpression(
      currentFunctionId, ui->txtFunctionDef->getVariableMath().c_str());
}
