#include "tabfunctions.hpp"

#include <QInputDialog>
#include <QMessageBox>

#include "guiutils.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "ui_tabfunctions.h"
#include "utils.hpp"

TabFunctions::TabFunctions(model::Model &doc, QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabFunctions>()}, sbmlDoc(doc) {
  ui->setupUi(this);
  connect(ui->listFunctions, &QListWidget::currentRowChanged, this,
          &TabFunctions::listFunctions_currentRowChanged);
  connect(ui->btnAddFunction, &QPushButton::clicked, this,
          &TabFunctions::btnAddFunction_clicked);
  connect(ui->btnRemoveFunction, &QPushButton::clicked, this,
          &TabFunctions::btnRemoveFunction_clicked);
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

void TabFunctions::loadModelData(const QString &selection) {
  auto *list = ui->listFunctions;
  list->clear();
  ui->btnRemoveFunctionParam->setEnabled(false);
  list->addItems(sbmlDoc.getFunctions().getNames());
  selectMatchingOrFirstItem(list, selection);
  bool enable = list->count() > 0;
  ui->txtFunctionName->setEnabled(enable);
  ui->listFunctionParams->setEnabled(enable);
  ui->txtFunctionDef->setEnabled(enable);
}

void TabFunctions::listFunctions_currentRowChanged(int row) {
  ui->txtFunctionName->clear();
  ui->listFunctionParams->clear();
  ui->txtFunctionDef->clear();
  ui->lblFunctionDefStatus->clear();
  if ((row < 0) || (row > sbmlDoc.getFunctions().getIds().size() - 1)) {
    ui->btnAddFunctionParam->setEnabled(false);
    ui->btnRemoveFunctionParam->setEnabled(false);
    ui->btnRemoveFunction->setEnabled(false);
    return;
  }
  const auto &funcs = sbmlDoc.getFunctions();
  auto id = funcs.getIds()[row];
  SPDLOG_DEBUG("Function {} selected", id.toStdString());
  ui->txtFunctionName->setText(funcs.getName(id));
  auto args = funcs.getArguments(id);
  ui->txtFunctionDef->setVariables(utils::toStdString(args));
  ui->listFunctionParams->addItems(args);
  if (!args.isEmpty()) {
    ui->listFunctionParams->setCurrentRow(0);
  } else {
    ui->btnRemoveFunctionParam->setEnabled(false);
  }
  ui->txtFunctionDef->setPlainText(funcs.getExpression(id));
  ui->btnAddFunctionParam->setEnabled(true);
  ui->btnRemoveFunctionParam->setEnabled(!args.isEmpty());
  ui->btnRemoveFunction->setEnabled(true);
}

void TabFunctions::btnAddFunction_clicked() {
  bool ok;
  auto functionName = QInputDialog::getText(
      this, "Add function", "New function name:", QLineEdit::Normal, {}, &ok);
  if (ok) {
    sbmlDoc.getFunctions().add(functionName);
    loadModelData(functionName);
  }
}

void TabFunctions::btnRemoveFunction_clicked() {
  int row = ui->listFunctions->currentRow();
  if ((row < 0) || (row > sbmlDoc.getFunctions().getIds().size() - 1)) {
    return;
  }
  auto msgbox =
      newYesNoMessageBox("Remove function?",
                         QString("Remove function '%1' from the model?")
                             .arg(ui->listFunctions->currentItem()->text()),
                         this);
  connect(msgbox, &QMessageBox::finished, this,
          [&s = sbmlDoc, row, this](int result) {
            if (result == QMessageBox::Yes) {
              auto funcId = s.getFunctions().getIds()[row];
              SPDLOG_INFO("Removing function {}", funcId.toStdString());
              s.getFunctions().remove(funcId);
              this->loadModelData();
            }
          });
  msgbox->open();
}

void TabFunctions::listFunctionParams_currentRowChanged(int row) {
  bool valid = (row >= 0) && (row < ui->listFunctionParams->count());
  ui->btnRemoveFunctionParam->setEnabled(valid);
}

void TabFunctions::btnAddFunctionParam_clicked() {
  bool ok;
  auto param =
      QInputDialog::getText(this, "Add function parameter",
                            "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok && !param.isEmpty()) {
    SPDLOG_INFO("Adding parameter {}", param.toStdString());
    auto id = sbmlDoc.getFunctions().getIds()[ui->listFunctions->currentRow()];
    auto argId = sbmlDoc.getFunctions().addArgument(id, param);
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
      auto p = item->text().toStdString();
      SPDLOG_INFO("Removing parameter {}", p);
      ui->txtFunctionDef->removeVariable(p);
      auto id =
          sbmlDoc.getFunctions().getIds()[ui->listFunctions->currentRow()];
      sbmlDoc.getFunctions().removeArgument(id, item->text());
      delete item;
    }
  });
  msgbox->open();
}

void TabFunctions::txtFunctionDef_mathChanged(const QString &math, bool valid,
                                              const QString &errorMessage) {
  if (valid) {
    SPDLOG_INFO("new math: {}", math.toStdString());
    ui->lblFunctionDefStatus->setText("");
    auto id = sbmlDoc.getFunctions().getIds()[ui->listFunctions->currentRow()];
    sbmlDoc.getFunctions().setExpression(id, math);
  } else {
    SPDLOG_INFO("math err: {}", errorMessage.toStdString());
    ui->lblFunctionDefStatus->setText(errorMessage);
  }
}
