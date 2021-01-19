#include "tabreactions.hpp"
#include <QInputDialog>
#include <QMessageBox>
#include <QSpinBox>
#include "guiutils.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "qlabelmousetracker.hpp"
#include "ui_tabreactions.h"

TabReactions::TabReactions(sme::model::Model &model,
                           QLabelMouseTracker *mouseTracker, QWidget *parent)
    : QWidget{parent}, ui{std::make_unique<Ui::TabReactions>()}, model{model},
      lblGeometry{mouseTracker} {
  ui->setupUi(this);

  connect(ui->listReactions, &QTreeWidget::currentItemChanged, this,
          &TabReactions::listReactions_currentItemChanged);
  connect(ui->btnAddReaction, &QPushButton::clicked, this,
          &TabReactions::btnAddReaction_clicked);
  connect(ui->btnRemoveReaction, &QPushButton::clicked, this,
          &TabReactions::btnRemoveReaction_clicked);
  connect(ui->txtReactionName, &QLineEdit::editingFinished, this,
          &TabReactions::txtReactionName_editingFinished);
  connect(ui->cmbReactionLocation, qOverload<int>(&QComboBox::activated), this,
          &TabReactions::cmbReactionLocation_activated);
  connect(ui->listReactionParams, &QTableWidget::currentCellChanged, this,
          &TabReactions::listReactionParams_currentCellChanged);
  connect(ui->listReactionParams, &QTableWidget::cellChanged, this,
          &TabReactions::listReactionParams_cellChanged);
  connect(ui->btnAddReactionParam, &QPushButton::clicked, this,
          &TabReactions::btnAddReactionParam_clicked);
  connect(ui->btnRemoveReactionParam, &QPushButton::clicked, this,
          &TabReactions::btnRemoveReactionParam_clicked);
  connect(ui->txtReactionRate, &QPlainTextMathEdit::mathChanged, this,
          &TabReactions::txtReactionRate_mathChanged);
}

TabReactions::~TabReactions() = default;

void TabReactions::loadModelData(const QString &selection) {
  currentReacId.clear();
  ui->cmbReactionLocation->clear();
  ui->listReactionParams->clear();
  enableWidgets(false);
  ui->listReactions->clear();
  QStringList locIds{model.getCompartments().getIds()};
  QStringList locNames{model.getCompartments().getNames()};
  locIds += model.getMembranes().getIds();
  locNames += model.getMembranes().getNames();
  for (int i = 0; i < locIds.size(); ++i) {
    const auto &locId = locIds[i];
    const auto &locName = locNames[i];
    QTreeWidgetItem *comp =
        new QTreeWidgetItem(ui->listReactions, QStringList({locName}));
    ui->listReactions->addTopLevelItem(comp);
    ui->cmbReactionLocation->addItem(locName);
    for (const auto &reacId : model.getReactions().getIds(locId)) {
      // add each reaction as child of compartment
      comp->addChild(new QTreeWidgetItem(
          comp, QStringList({model.getReactions().getName(reacId)})));
    }
  }
  ui->listReactions->expandAll();
  selectMatchingOrFirstChild(ui->listReactions, selection);
}

void TabReactions::enableWidgets(bool enable) {
  ui->btnRemoveReaction->setEnabled(enable);
  ui->txtReactionName->setEnabled(enable);
  ui->cmbReactionLocation->setEnabled(enable);
  ui->listReactionSpecies->setEnabled(enable);
  ui->listReactionParams->setEnabled(enable);
  ui->btnAddReactionParam->setEnabled(enable);
  ui->btnRemoveReactionParam->setEnabled(enable);
  ui->txtReactionRate->setEnabled(enable);
}

static QTableWidgetItem *newQTableWidgetItem(const QString &value) {
  auto *item = new QTableWidgetItem(value);
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
  return item;
}

void TabReactions::listReactions_currentItemChanged(QTreeWidgetItem *current,
                                                    QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  ui->txtReactionName->clear();
  ui->listReactionSpecies->clear();
  ui->listReactionParams->setRowCount(0);
  ui->listReactionParams->setColumnCount(3);
  ui->listReactionParams->setHorizontalHeaderLabels({"Name", "Value", "Units"});
  ui->txtReactionRate->clear();
  ui->lblReactionRateUnits->clear();
  ui->lblReactionRateStatus->clear();
  if (current != nullptr && current->parent() == nullptr) {
    // user selected a compartment or membrane: update image
    int i = ui->listReactions->indexOfTopLevelItem(current);
    if (auto numComps = model.getCompartments().getIds().size(); i < numComps) {
      lblGeometry->setImage(model.getCompartments()
                                .getCompartments()[static_cast<std::size_t>(i)]
                                ->getCompartmentImage());
    } else {
      i -= numComps;
      lblGeometry->setImage(model.getMembranes()
                                .getMembranes()[static_cast<std::size_t>(i)]
                                .getImage());
    }
  }
  if ((current == nullptr) || (current->parent() == nullptr)) {
    // selection is not a reaction
    enableWidgets(false);
    return;
  }
  SPDLOG_DEBUG("item {} / {} selected",
               current->parent()->text(0).toStdString(),
               current->text(0).toStdString());
  enableWidgets(true);
  ui->btnRemoveReactionParam->setEnabled(false);
  bool isMembrane{false};
  int locationIndex{ui->listReactions->indexOfTopLevelItem(current->parent())};
  int compartmentIndex = locationIndex;
  if (auto numComps = model.getCompartments().getIds().size();
      compartmentIndex >= numComps) {
    isMembrane = true;
    compartmentIndex -= numComps;
  }
  QString compartmentId;
  QStringList compartments;
  if (isMembrane) {
    const auto &m =
        model.getMembranes()
            .getMembranes()[static_cast<std::size_t>(compartmentIndex)];
    lblGeometry->setImage(m.getImage());
    compartmentId = m.getId().c_str();
    compartments = QStringList{m.getCompartmentA()->getId().c_str(),
                               m.getCompartmentB()->getId().c_str()};
    ui->lblReactionRateUnits->setText(model.getUnits().getMembraneReaction());
  } else {
    compartmentId = model.getCompartments().getIds()[compartmentIndex];
    compartments = QStringList{compartmentId};
    lblGeometry->setImage(model.getCompartments()
                              .getCompartment(compartmentId)
                              ->getCompartmentImage());
    ui->lblReactionRateUnits->setText(
        model.getUnits().getCompartmentReaction());
  }
  int reactionIndex = current->parent()->indexOfChild(current);
  QString reactionId =
      model.getReactions().getIds(compartmentId)[reactionIndex];
  SPDLOG_DEBUG("  - reaction index {}", reactionIndex);
  SPDLOG_DEBUG("  - reaction Id {}", reactionId.toStdString());
  SPDLOG_DEBUG("  - compartment index {}", compartmentIndex);
  SPDLOG_DEBUG("  - compartment Id {}", compartmentId.toStdString());

  // display reaction information
  currentReacId = reactionId;
  ui->txtReactionName->setText(model.getReactions().getName(currentReacId));
  ui->cmbReactionLocation->setCurrentIndex(locationIndex);
  // reset variables to only built-in functions
  ui->txtReactionRate->resetToDefaultFunctions();
  // add model parameters
  for (const auto &[id, name] : model.getParameters().getSymbols()) {
    ui->txtReactionRate->addVariable(id, name);
  }
  // add any reaction localParameters
  for (const auto &id : model.getReactions().getParameterIds(currentReacId)) {
    ui->txtReactionRate->addVariable(
        id.toStdString(),
        model.getReactions().getParameterName(currentReacId, id).toStdString());
  }
  // add model functions
  for (const auto &function : model.getFunctions().getSymbolicFunctions()) {
    ui->txtReactionRate->addFunction(function);
  }
  // species stoich
  for (const auto &compID : compartments) {
    auto *comp = new QTreeWidgetItem;
    comp->setText(0, model.getCompartments().getName(compID));
    ui->listReactionSpecies->addTopLevelItem(comp);
    for (const auto &sId : model.getSpecies().getIds(compID)) {
      auto sName = model.getSpecies().getName(sId);
      ui->txtReactionRate->addVariable(sId.toStdString(), sName.toStdString());
      auto *item = new QTreeWidgetItem;
      item->setText(0, sName);
      auto *spinBox = new QSpinBox(ui->listReactionSpecies);
      spinBox->setRange(-99, 99);
      comp->addChild(item);
      ui->listReactionSpecies->setItemWidget(item, 1, spinBox);
      connect(spinBox, qOverload<int>(&QSpinBox::valueChanged),
              [item, &reacs = model.getReactions(), reactionId, sId](int i) {
                if (i == 0) {
                  item->setData(0, Qt::BackgroundRole, {});
                  item->setData(1, Qt::BackgroundRole, {});
                } else {
                  QColor col = i > 0 ? Qt::green : Qt::red;
                  item->setData(0, Qt::BackgroundRole, QBrush(col));
                  item->setData(1, Qt::BackgroundRole, QBrush(col));
                }
                reacs.setSpeciesStoichiometry(reactionId, sId, i);
              });
      spinBox->setValue(
          model.getReactions().getSpeciesStoichiometry(reactionId, sId));
    }
  }
  ui->listReactionSpecies->expandAll();
  ui->listReactionParams->blockSignals(true);
  for (const auto &paramId :
       model.getReactions().getParameterIds(currentReacId)) {
    auto name = model.getReactions().getParameterName(currentReacId, paramId);
    double value =
        model.getReactions().getParameterValue(currentReacId, paramId);
    auto *itemName = newQTableWidgetItem(name);
    auto *itemValue =
        newQTableWidgetItem(QString("%1").arg(value, 14, 'g', 14));
    int n = ui->listReactionParams->rowCount();
    ui->listReactionParams->setRowCount(n + 1);
    ui->listReactionParams->setItem(n, 0, itemName);
    ui->listReactionParams->setItem(n, 1, itemValue);
    ui->txtReactionRate->addVariable(paramId.toStdString(), name.toStdString());
  }
  ui->listReactionParams->blockSignals(false);
  ui->txtReactionRate->importVariableMath(
      model.getReactions().getRateExpression(currentReacId).toStdString());
}

void TabReactions::btnAddReaction_clicked() {
  // get currently selected compartment
  int index = 0;
  if (auto *item = ui->listReactions->currentItem(); item != nullptr) {
    auto *parent = item->parent() != nullptr ? item->parent() : item;
    index = ui->listReactions->indexOfTopLevelItem(parent);
  }
  QString locationId;
  int nComps = model.getCompartments().getIds().size();
  if (index < nComps) {
    locationId = model.getCompartments().getIds()[index];
  } else {
    locationId = model.getMembranes().getIds()[index - nComps];
  }
  bool ok;
  auto reactionName = QInputDialog::getText(
      this, "Add reaction", "New reaction name:", QLineEdit::Normal, {}, &ok);
  if (ok) {
    model.getReactions().add(reactionName, locationId);
    loadModelData(reactionName);
  }
}

void TabReactions::btnRemoveReaction_clicked() {
  auto msgbox =
      newYesNoMessageBox("Remove reaction?",
                         QString("Remove reaction '%1' from the model?")
                             .arg(ui->listReactions->currentItem()->text(0)),
                         this);
  connect(msgbox, &QMessageBox::finished, this, [this](int result) {
    if (result == QMessageBox::Yes) {
      SPDLOG_INFO("Removing reaction {}", currentReacId.toStdString());
      model.getReactions().remove(currentReacId);
      loadModelData();
    }
  });
  msgbox->open();
}

void TabReactions::txtReactionName_editingFinished() {
  auto name = ui->txtReactionName->text();
  model.getReactions().setName(currentReacId, name);
  ui->listReactions->currentItem()->setText(0, name);
}

void TabReactions::cmbReactionLocation_activated(int index) {
  QString locationId;
  int nComps = model.getCompartments().getIds().size();
  if (index < nComps) {
    locationId = model.getCompartments().getIds().at(index);
  } else {
    locationId = model.getMembranes().getIds()[index - nComps];
  }
  if (locationId == model.getReactions().getLocation(currentReacId)) {
    return;
  }
  model.getReactions().setLocation(currentReacId, locationId);
  loadModelData(currentReacId);
}

void TabReactions::listReactionParams_currentCellChanged(int currentRow,
                                                         int currentColumn,
                                                         int previousRow,
                                                         int previousColumn) {
  Q_UNUSED(currentColumn);
  Q_UNUSED(previousRow);
  Q_UNUSED(previousColumn);
  bool valid =
      (currentRow >= 0) && (currentRow < ui->listReactionParams->rowCount());
  ui->btnRemoveReactionParam->setEnabled(valid);
}

void TabReactions::listReactionParams_cellChanged(int row, int column) {
  if (column < 0 || column > 1 || row < 0 ||
      row >= ui->listReactionParams->rowCount()) {
    return;
  }
  auto &reacs = model.getReactions();
  QString paramId = reacs.getParameterIds(currentReacId)[row];
  auto *item = ui->listReactionParams->item(row, column);
  if (column == 0) {
    // user changed reaction parameter name
    SPDLOG_INFO("Changing reac '{}' parameter '{}' name to '{}'",
                currentReacId.toStdString(), paramId.toStdString(),
                item->text().toStdString());
    auto newName = reacs.setParameterName(currentReacId, paramId, item->text());
    if (newName != item->text()) {
      SPDLOG_INFO("  -> '{}' to avoid nameclash", newName.toStdString());
      ui->listReactionParams->blockSignals(true);
      item->setText(newName);
      ui->listReactionParams->blockSignals(false);
    }
    // update math expression with new parameter name
    ui->txtReactionRate->blockSignals(true);
    ui->txtReactionRate->removeVariable(paramId.toStdString());
    ui->txtReactionRate->addVariable(paramId.toStdString(),
                                     newName.toStdString());
    ui->txtReactionRate->blockSignals(false);
    ui->txtReactionRate->importVariableMath(
        model.getReactions().getRateExpression(currentReacId).toStdString());
    return;
  }
  if (column == 1) {
    // user changed reaction parameter value
    bool isValidDouble{false};
    SPDLOG_INFO("Changing parameter value to '{}'", item->text().toStdString());
    double value = item->text().toDouble(&isValidDouble);
    if (isValidDouble) {
      item->setBackground({});
      reacs.setParameterValue(currentReacId, paramId, value);
    } else {
      SPDLOG_WARN("Invalid number ignored: '{}'", item->text().toStdString());
      item->setBackground(Qt::red);
    }
  }
}

void TabReactions::btnAddReactionParam_clicked() {
  bool ok;
  auto name =
      QInputDialog::getText(this, "Add reaction parameter",
                            "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok && !name.isEmpty()) {
    SPDLOG_INFO("Adding reaction parameter {}", name.toStdString());
    auto id = model.getReactions().addParameter(currentReacId, name, 0.0);
    auto uniqueName = model.getReactions().getParameterName(currentReacId, id);
    auto *itemName = newQTableWidgetItem(uniqueName);
    auto *itemValue = newQTableWidgetItem("0.0");
    ui->listReactionParams->blockSignals(true);
    int n = ui->listReactionParams->rowCount();
    ui->listReactionParams->setRowCount(n + 1);
    ui->listReactionParams->setItem(n, 0, itemName);
    ui->listReactionParams->setItem(n, 1, itemValue);
    ui->txtReactionRate->addVariable(id.toStdString(),
                                     uniqueName.toStdString());
    ui->listReactionParams->setCurrentItem(itemValue);
    ui->btnRemoveReactionParam->setEnabled(true);
    ui->listReactionParams->blockSignals(false);
  }
}

void TabReactions::btnRemoveReactionParam_clicked() {
  int row = ui->listReactionParams->currentRow();
  if ((row < 0) || (row > ui->listReactionParams->rowCount() - 1)) {
    return;
  }
  const auto &param = ui->listReactionParams->item(row, 0)->text();
  auto msgbox = newYesNoMessageBox(
      "Remove reaction parameter?",
      QString("Remove parameter '%1' from the reaction?").arg(param), this);
  connect(msgbox, &QMessageBox::finished, this, [param, row, this](int result) {
    if (result == QMessageBox::Yes) {
      SPDLOG_INFO("Removing parameter {}", param.toStdString());
      auto paramId = model.getReactions().getParameterIds(currentReacId)[row];
      model.getReactions().removeParameter(currentReacId, paramId);
      ui->listReactionParams->blockSignals(true);
      ui->listReactionParams->removeRow(row);
      ui->listReactionParams->blockSignals(false);
      ui->txtReactionRate->removeVariable(paramId.toStdString());
    }
  });
  msgbox->open();
}

void TabReactions::txtReactionRate_mathChanged(const QString &math, bool valid,
                                               const QString &errorMessage) {
  if (!valid) {
    SPDLOG_INFO("math err: {}", errorMessage.toStdString());
    ui->lblReactionRateStatus->setText(errorMessage);
    return;
  }
  SPDLOG_INFO("new math: {}", math.toStdString());
  ui->lblReactionRateStatus->setText("");
  model.getReactions().setRateExpression(
      currentReacId, ui->txtReactionRate->getVariableMath().c_str());
}
