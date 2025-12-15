#include "tabevents.hpp"
#include "dialoganalytic.hpp"
#include "guiutils.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include "ui_tabevents.h"
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
  connect(ui->cmbEventVariable, qOverload<int>(&QComboBox::activated), this,
          &TabEvents::cmbEventVariable_activated);
  connect(ui->btnSetSpeciesConcentration, &QPushButton::clicked, this,
          &TabEvents::btnSetSpeciesConcentration_clicked);
  connect(ui->txtExpression, &QPlainTextMathEdit::mathChanged, this,
          &TabEvents::txtExpression_mathChanged);
}

TabEvents::~TabEvents() = default;

void TabEvents::loadModelData(const QString &selection) {
  currentEventId.clear();
  ui->listEvents->clear();
  ui->lblSpeciesExpression->clear();
  ui->stkValue->setCurrentIndex(0);
  ui->txtExpression->clearVariables();
  ui->txtExpression->reset();
  ui->cmbEventVariable->clear();
  variableIds.clear();
  for (const auto &cId : model.getCompartments().getIds()) {
    const auto &compartmentName{model.getCompartments().getName(cId)};
    for (const auto &sId : model.getSpecies().getIds(cId)) {
      variableIds.push_back(sId);
      ui->cmbEventVariable->addItem(compartmentName + "/" +
                                    model.getSpecies().getName(sId));
    }
  }
  nSpecies = static_cast<int>(variableIds.size());
  for (const auto &id : model.getParameters().getIds()) {
    variableIds.push_back(id);
    ui->cmbEventVariable->addItem(model.getParameters().getName(id));
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
  ui->cmbEventVariable->setEnabled(enable);
  ui->stkValue->setEnabled(enable);
  ui->btnRemoveEvent->setEnabled(enable);
  if (!enable) {
    ui->txtExpression->setEnabled(false);
    ui->btnSetSpeciesConcentration->setEnabled(false);
  }
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
  if (auto i{static_cast<int>(
          variableIds.indexOf(events.getVariable(currentEventId)))};
      i >= 0) {
    ui->cmbEventVariable->setEnabled(true);
    ui->cmbEventVariable->setCurrentIndex(i);
    cmbEventVariable_activated(i);
  }
  enableWidgets(true);
}

void TabEvents::btnAddEvent_clicked() {
  if (variableIds.isEmpty()) {
    QMessageBox::information(
        this, "Model has no species or parameters",
        "To add events, the model must contain species or parameters.");
    return;
  }
  bool ok{false};
  auto eventName = QInputDialog::getText(
      this, "Add event", "New event name:", QLineEdit::Normal, {}, &ok);
  if (ok && !eventName.isEmpty()) {
    auto newEventName{model.getEvents().add(eventName, variableIds[0])};
    loadModelData(newEventName);
  }
}

void TabEvents::btnRemoveEvent_clicked() {
  int row = ui->listEvents->currentRow();
  if ((row < 0) || (row > model.getEvents().getIds().size() - 1)) {
    return;
  }

  auto result{
      QMessageBox::question(this, "Remove event?",
                            QString("Remove event '%1' from the model?")
                                .arg(ui->listEvents->currentItem()->text()),
                            QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    SPDLOG_INFO("Removing event {}", currentEventId.toStdString());
    model.getEvents().remove(currentEventId);
    loadModelData();
  }
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
  ui->lblExpressionStatus->setText(ui->txtExpression->getErrorMessage());
}

void TabEvents::cmbEventVariable_activated(int index) {
  if (!ui->cmbEventVariable->isEnabled() || index < 0 ||
      index >= variableIds.size()) {
    ui->lblSpeciesExpression->clear();
    ui->stkValue->setCurrentIndex(0);
    ui->stkValue->setEnabled(false);
    return;
  }
  ui->stkValue->setEnabled(true);
  model.getEvents().setVariable(currentEventId, variableIds[index]);
  bool isSpecies{index < nSpecies};
  bool invertYAxis{model.getDisplayOptions().invertYAxis};
  if (isSpecies) {
    ui->stkValue->setCurrentIndex(1);
    ui->btnSetSpeciesConcentration->setEnabled(true);
    auto img =
        DialogAnalytic(model.getEvents().getExpression(currentEventId),
                       DialogAnalyticDataType::Concentration,
                       model.getSpeciesGeometry(variableIds[index]),
                       model.getParameters(), model.getFunctions(), invertYAxis)
            .getImage();
    if (invertYAxis) {
      img = img.flipped(Qt::Orientation::Vertical);
    }
    ui->lblSpeciesExpression->setPixmap(QPixmap::fromImage(img));
    ui->txtExpression->setEnabled(false);
  } else {
    ui->stkValue->setCurrentIndex(0);
    ui->btnSetSpeciesConcentration->setEnabled(false);
    ui->txtExpression->setEnabled(true);
    ui->txtExpression->importVariableMath(
        model.getEvents().getExpression(currentEventId).toStdString());
  }
}

void TabEvents::btnSetSpeciesConcentration_clicked() {
  int iSpecies{ui->cmbEventVariable->currentIndex()};
  if (iSpecies >= nSpecies) {
    return;
  }
  QString sId{variableIds[iSpecies]};
  bool invertYAxis{model.getDisplayOptions().invertYAxis};
  DialogAnalytic dialog(model.getEvents().getExpression(currentEventId),
                        DialogAnalyticDataType::Concentration,
                        model.getSpeciesGeometry(sId), model.getParameters(),
                        model.getFunctions(), invertYAxis);
  if (dialog.exec() == QDialog::Accepted) {
    const std::string &expr{dialog.getExpression()};
    SPDLOG_DEBUG("  - set expr: {}", expr);
    model.getEvents().setExpression(currentEventId, expr.c_str());
    auto img = dialog.getImage();
    if (invertYAxis) {
      img = img.flipped(Qt::Orientation::Vertical);
    }
    ui->lblSpeciesExpression->setPixmap(QPixmap::fromImage(img));
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
