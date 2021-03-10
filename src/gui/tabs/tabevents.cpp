#include "tabevents.hpp"
#include "guiutils.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "ui_tabevents.h"
#include "utils.hpp"
#include <QInputDialog>
#include <QMessageBox>

TabEvents::TabEvents(sme::model::Model &m, QWidget *parent)
    : QWidget{parent}, ui{std::make_unique<Ui::TabEvents>()}, model{m} {
  ui->setupUi(this);
  connect(ui->listEvents, &QListWidget::currentRowChanged, this,
          &TabEvents::listEvents_currentRowChanged);
  connect(ui->btnAddEvent, &QPushButton::clicked, this,
          &TabEvents::btnAddEvent_clicked);
  connect(ui->btnRemoveEvent, &QPushButton::clicked, this,
          &TabEvents::btnRemoveEvent_clicked);
  connect(ui->txtEventName, &QLineEdit::editingFinished, this,
          &TabEvents::txtEventName_editingFinished);
  connect(ui->txtEventTime, &QLineEdit::editingFinished, this,
          &TabEvents::txtEventTime_editingFinished);
  connect(ui->cmbEventParam, qOverload<int>(&QComboBox::activated), this,
          &TabEvents::cmbEventParam_activated);
  connect(ui->txtExpression, &QPlainTextMathEdit::mathChanged, this,
          &TabEvents::txtExpression_mathChanged);
}

TabEvents::~TabEvents() = default;

void TabEvents::loadModelData(const QString &selection) {
  currentEventId.clear();
  ui->listEvents->clear();
  ui->txtExpression->clearVariables();
  ui->txtExpression->resetToDefaultFunctions();
  ui->cmbEventParam->clear();
  ui->cmbEventParam->addItems(model.getParameters().getNames());
  for (const auto &[id, name] : model.getParameters().getSymbols()) {
    ui->txtExpression->addVariable(id, name);
  }
  for (const auto &function : model.getFunctions().getSymbolicFunctions()) {
    ui->txtExpression->addFunction(function);
  }
  for (const auto &id : model.getEvents().getIds()) {
    auto name = model.getEvents().getName(id);
    ui->listEvents->addItem(name);
  }
  ui->lblExpressionStatus->clear();
  selectMatchingOrFirstItem(ui->listEvents, selection);
  bool enable{ui->listEvents->count() > 0};
  enableWidgets(enable);
}

void TabEvents::enableWidgets(bool enable) {
  ui->txtEventName->setEnabled(enable);
  ui->txtEventTime->setEnabled(enable);
  ui->cmbEventParam->setEnabled(enable);
  ui->txtExpression->setEnabled(enable);
  ui->btnRemoveEvent->setEnabled(enable);
}

void TabEvents::listEvents_currentRowChanged(int row) {
  currentEventId.clear();
  ui->txtEventName->clear();
  ui->txtEventTime->clear();
  ui->txtExpression->clear();
  ui->lblExpressionStatus->clear();
  enableWidgets(false);
  if ((row < 0) || (row > model.getEvents().getIds().size() - 1)) {
    return;
  }
  const auto &events = model.getEvents();
  currentEventId = events.getIds()[row];
  SPDLOG_DEBUG("Event {} selected", currentEventId.toStdString());
  ui->txtEventName->setText(events.getName(currentEventId));
  ui->txtEventTime->setText(QString::number(events.getTime(currentEventId)));
  if (auto i = ui->cmbEventParam->findText(
          model.getParameters().getName(events.getVariable(currentEventId)));
      i >= 0) {
    ui->cmbEventParam->setCurrentIndex(i);
  }
  ui->txtExpression->importVariableMath(
      events.getExpression(currentEventId).toStdString());
  enableWidgets(true);
}

void TabEvents::btnAddEvent_clicked() {
  if (model.getParameters().getIds().isEmpty()) {
    QMessageBox::information(
        this, "Model has no parameters",
        "To add events, the model must contain parameters.");
    return;
  }
  bool ok{false};
  auto eventName = QInputDialog::getText(
      this, "Add event", "New event name:", QLineEdit::Normal, {}, &ok);
  if (ok && !eventName.isEmpty()) {
    auto newEventName{
        model.getEvents().add(eventName, model.getParameters().getIds()[0])};
    loadModelData(newEventName);
  }
}

void TabEvents::btnRemoveEvent_clicked() {
  int row = ui->listEvents->currentRow();
  if ((row < 0) || (row > model.getEvents().getIds().size() - 1)) {
    return;
  }
  auto msgbox =
      newYesNoMessageBox("Remove event?",
                         QString("Remove event '%1' from the model?")
                             .arg(ui->listEvents->currentItem()->text()),
                         this);
  connect(msgbox, &QMessageBox::finished, this, [this](int result) {
    if (result == QMessageBox::Yes) {
      SPDLOG_INFO("Removing event {}", currentEventId.toStdString());
      model.getEvents().remove(currentEventId);
      loadModelData();
    }
  });
  msgbox->open();
}

void TabEvents::txtEventName_editingFinished() {
  const QString &name{ui->txtEventName->text()};
  if (name == model.getSpecies().getName(currentEventId)) {
    return;
  }
  auto newName{model.getEvents().setName(currentEventId, name)};
  ui->txtEventName->setText(newName);
  loadModelData(newName);
}

void TabEvents::txtEventTime_editingFinished() {
  bool validDouble{false};
  double time{ui->txtEventTime->text().toDouble(&validDouble)};
  if (!validDouble || time < 0) {
    ui->lblExpressionStatus->setText("Time must be a positive number");
    return;
  }
  ui->lblExpressionStatus->setText("");
  model.getEvents().setTime(currentEventId, time);
}

void TabEvents::cmbEventParam_activated(int index) {
  if (index < model.getParameters().getIds().size()) {
    model.getEvents().setVariable(currentEventId,
                                  model.getParameters().getIds()[index]);
  }
}

void TabEvents::txtExpression_mathChanged(const QString &math, bool valid,
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
  model.getEvents().setExpression(currentEventId, math);
}
