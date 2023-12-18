#include "tabreactions.hpp"
#include "guiutils.hpp"
#include "qlabelmousetracker.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "ui_tabreactions.h"
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QMessageBox>

QDoubleSpinBoxNoScroll::QDoubleSpinBoxNoScroll(QWidget *parent)
    : QDoubleSpinBox(parent) {}

void QDoubleSpinBoxNoScroll::wheelEvent(QWheelEvent *event) { event->ignore(); }

TabReactions::TabReactions(sme::model::Model &m,
                           QLabelMouseTracker *mouseTracker,
                           QVoxelRenderer *voxelRenderer, QWidget *parent)
    : QWidget{parent}, ui{std::make_unique<Ui::TabReactions>()}, model{m},
      lblGeometry{mouseTracker}, voxGeometry{voxelRenderer} {
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
  ui->lblReactionScheme->clear();
  ui->cmbReactionLocation->clear();
  ui->listReactionParams->clear();
  enableWidgets(false);
  ui->listReactions->clear();
  reactionLocations = model.getReactions().getReactionLocations();
  for (const auto &reactionLocation : reactionLocations) {
    auto reactionIds{model.getReactions().getIds(reactionLocation)};
    bool invalidLocation{reactionLocation.type ==
                         sme::model::ReactionLocation::Type::Invalid};
    if (!invalidLocation || !reactionIds.isEmpty()) {
      auto *comp{new QTreeWidgetItem(ui->listReactions,
                                     QStringList({reactionLocation.name}))};
      if (invalidLocation) {
        comp->setForeground(0, Qt::red);
      }
      ui->listReactions->addTopLevelItem(comp);
      ui->cmbReactionLocation->addItem(reactionLocation.name);
      for (const auto &reactionId : reactionIds) {
        auto *child{new QTreeWidgetItem(
            comp, QStringList({model.getReactions().getName(reactionId)}))};
        if (invalidLocation) {
          child->setForeground(0, Qt::red);
        }
        comp->addChild(child);
      }
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
  auto *item{new QTableWidgetItem(value)};
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
  return item;
}

void TabReactions::listReactions_currentItemChanged(QTreeWidgetItem *current,
                                                    QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  ui->txtReactionName->clear();
  ui->lblReactionScheme->clear();
  ui->listReactionSpecies->clear();
  ui->listReactionParams->setRowCount(0);
  ui->listReactionParams->setColumnCount(3);
  ui->listReactionParams->setHorizontalHeaderLabels({"Name", "Value", "Units"});
  ui->txtReactionRate->clear();
  ui->lblReactionRateUnits->clear();
  ui->lblReactionRateStatus->clear();
  if (current == nullptr) {
    enableWidgets(false);
    return;
  }
  if (current->parent() == nullptr) {
    // current item has no parent -> is a location
    auto locationIndex{static_cast<std::size_t>(
        ui->listReactions->indexOfTopLevelItem(current))};
    const auto &reactionLocation{reactionLocations[locationIndex]};
    if (reactionLocation.type ==
        sme::model::ReactionLocation::Type::Compartment) {
      lblGeometry->setImage(model.getCompartments()
                                .getCompartment(reactionLocation.id)
                                ->getCompartmentImages());
      voxGeometry->setImage(model.getCompartments()
                                .getCompartment(reactionLocation.id)
                                ->getCompartmentImages());
    } else if (reactionLocation.type ==
               sme::model::ReactionLocation::Type::Membrane) {
      lblGeometry->setImage(
          model.getMembranes().getMembrane(reactionLocation.id)->getImages());
      voxGeometry->setImage(
          model.getMembranes().getMembrane(reactionLocation.id)->getImages());
    } else {
      lblGeometry->clear();
    }
    enableWidgets(false);
    return;
  }
  SPDLOG_DEBUG("item {} / {} selected",
               current->parent()->text(0).toStdString(),
               current->text(0).toStdString());
  auto locationIndex{static_cast<std::size_t>(
      ui->listReactions->indexOfTopLevelItem(current->parent()))};
  const auto &reactionLocation{reactionLocations[locationIndex]};
  int reactionIndex{current->parent()->indexOfChild(current)};
  currentReacId = model.getReactions().getIds(reactionLocation)[reactionIndex];
  if (reactionLocation.type == sme::model::ReactionLocation::Type::Invalid) {
    enableWidgets(false);
    ui->txtReactionName->setText(model.getReactions().getName(currentReacId));
    // only available user action is to remove it or set a valid location
    ui->cmbReactionLocation->setCurrentIndex(-1);
    ui->cmbReactionLocation->setEnabled(true);
    ui->btnRemoveReaction->setEnabled(true);
    return;
  }
  enableWidgets(true);
  ui->btnRemoveReactionParam->setEnabled(false);
  QStringList compartments;
  if (reactionLocation.type == sme::model::ReactionLocation::Type::Membrane) {
    const auto *m = model.getMembranes().getMembrane(reactionLocation.id);
    lblGeometry->setImage(m->getImages());
    voxGeometry->setImage(m->getImages());
    compartments = QStringList{m->getCompartmentA()->getId().c_str(),
                               m->getCompartmentB()->getId().c_str()};
    ui->lblReactionRateUnits->setText(model.getUnits().getMembraneReaction());
  } else {
    compartments = QStringList{reactionLocation.id};
    lblGeometry->setImage(model.getCompartments()
                              .getCompartment(reactionLocation.id)
                              ->getCompartmentImages());
    voxGeometry->setImage(model.getCompartments()
                              .getCompartment(reactionLocation.id)
                              ->getCompartmentImages());
    ui->lblReactionRateUnits->setText(
        model.getUnits().getCompartmentReaction());
  }
  SPDLOG_DEBUG("  - reaction index {}", reactionIndex);
  SPDLOG_DEBUG("  - reaction Id {}", currentReacId.toStdString());
  SPDLOG_DEBUG("  - location Id {}", reactionLocation.id.toStdString());

  // display reaction information
  ui->txtReactionName->setText(model.getReactions().getName(currentReacId));
  ui->lblReactionScheme->setText(model.getReactions().getScheme(currentReacId));
  ui->cmbReactionLocation->setCurrentIndex(static_cast<int>(locationIndex));
  ui->txtReactionRate->reset();
  // add model parameters
  for (const auto &[id, name] :
       model.getParameters().getSymbols(compartments)) {
    ui->txtReactionRate->addVariable(id, name);
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
      auto *spinBox = new QDoubleSpinBoxNoScroll(ui->listReactionSpecies);
      spinBox->setSingleStep(1.0);
      spinBox->setDecimals(14);
      spinBox->setRange(std::numeric_limits<double>::lowest(),
                        std::numeric_limits<double>::max());
      comp->addChild(item);
      ui->listReactionSpecies->setItemWidget(item, 1, spinBox);
      auto *schemeLabel{ui->lblReactionScheme};
      QString rId{currentReacId};
      connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
              [schemeLabel, item, &reacs = model.getReactions(), rId,
               sId](double s) {
                if (s == 0) {
                  item->setData(0, Qt::BackgroundRole, {});
                  item->setData(1, Qt::BackgroundRole, {});
                } else {
                  QColor col = s > 0 ? Qt::green : Qt::red;
                  item->setData(0, Qt::BackgroundRole, QBrush(col));
                  item->setData(1, Qt::BackgroundRole, QBrush(col));
                }
                reacs.setSpeciesStoichiometry(rId, sId, s);
                schemeLabel->setText(reacs.getScheme(rId));
              });
      spinBox->setValue(model.getReactions().getSpeciesStoichiometry(rId, sId));
    }
  }
  ui->listReactionSpecies->expandAll();
  ui->listReactionParams->blockSignals(true);
  for (const auto &paramId :
       model.getReactions().getParameterIds(currentReacId)) {
    auto name{model.getReactions().getParameterName(currentReacId, paramId)};
    double value{
        model.getReactions().getParameterValue(currentReacId, paramId)};
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

static int getCurrentTopLevelIndex(const QTreeWidget *treeWidget) {
  if (auto *item = treeWidget->currentItem(); item != nullptr) {
    auto *topLevelItem{item->parent() != nullptr ? item->parent() : item};
    return treeWidget->indexOfTopLevelItem(topLevelItem);
  }
  return 0;
}

void TabReactions::btnAddReaction_clicked() {
  // get currently selected compartment if valid, otherwise first compartment
  auto index{
      static_cast<std::size_t>(getCurrentTopLevelIndex(ui->listReactions))};
  const auto &location{reactionLocations[index]};
  QString locationId{model.getCompartments().getIds()[0]};
  if (location.type != sme::model::ReactionLocation::Type::Invalid) {
    locationId = location.id;
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
  auto result{
      QMessageBox::question(this, "Remove reaction?",
                            QString("Remove reaction '%1' from the model?")
                                .arg(ui->listReactions->currentItem()->text(0)),
                            QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    SPDLOG_INFO("Removing reaction {}", currentReacId.toStdString());
    model.getReactions().remove(currentReacId);
    loadModelData();
  }
}

void TabReactions::txtReactionName_editingFinished() {
  auto name = ui->txtReactionName->text();
  model.getReactions().setName(currentReacId, name);
  ui->listReactions->currentItem()->setText(0, name);
}

void TabReactions::cmbReactionLocation_activated(int index) {
  const auto &location{reactionLocations[static_cast<std::size_t>(index)]};
  if (location.id == model.getReactions().getLocation(currentReacId)) {
    return;
  }
  model.getReactions().setLocation(currentReacId, location.id);
  loadModelData(ui->txtReactionName->text());
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
  auto &reactions{model.getReactions()};
  QString paramId = reactions.getParameterIds(currentReacId)[row];
  auto *item = ui->listReactionParams->item(row, column);
  if (column == 0) {
    // user changed reaction parameter name
    SPDLOG_INFO("Changing reac '{}' parameter '{}' name to '{}'",
                currentReacId.toStdString(), paramId.toStdString(),
                item->text().toStdString());
    auto newName =
        reactions.setParameterName(currentReacId, paramId, item->text());
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
      reactions.setParameterValue(currentReacId, paramId, value);
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
  if (!ok || name.isEmpty()) {
    return;
  }
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
  ui->txtReactionRate->addVariable(id.toStdString(), uniqueName.toStdString());
  ui->listReactionParams->setCurrentItem(itemValue);
  ui->btnRemoveReactionParam->setEnabled(true);
  ui->listReactionParams->blockSignals(false);
}

void TabReactions::btnRemoveReactionParam_clicked() {
  int row = ui->listReactionParams->currentRow();
  if ((row < 0) || (row > ui->listReactionParams->rowCount() - 1)) {
    return;
  }
  const auto &param = ui->listReactionParams->item(row, 0)->text();
  auto result{QMessageBox::question(
      this, "Remove reaction parameter?",
      QString("Remove parameter '%1' from the reaction?").arg(param),
      QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    SPDLOG_INFO("Removing parameter {}", param.toStdString());
    auto paramId = model.getReactions().getParameterIds(currentReacId)[row];
    model.getReactions().removeParameter(currentReacId, paramId);
    ui->listReactionParams->blockSignals(true);
    ui->listReactionParams->removeRow(row);
    ui->listReactionParams->blockSignals(false);
    ui->txtReactionRate->removeVariable(paramId.toStdString());
  }
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
