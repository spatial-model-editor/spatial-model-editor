#include "qtabreactions.hpp"

#include <QInputDialog>
#include <QMessageBox>
#include <QSpinBox>

#include "guiutils.hpp"
#include "logger.hpp"
#include "qlabelmousetracker.hpp"
#include "ui_qtabreactions.h"

QTabReactions::QTabReactions(sbml::SbmlDocWrapper &doc,
                             QLabelMouseTracker *mouseTracker, QWidget *parent)
    : QWidget(parent),
      ui{std::make_unique<Ui::QTabReactions>()},
      sbmlDoc(doc),
      lblGeometry(mouseTracker) {
  ui->setupUi(this);

  connect(ui->listReactions, &QTreeWidget::currentItemChanged, this,
          &QTabReactions::listReactions_currentItemChanged);

  connect(ui->btnAddReaction, &QPushButton::clicked, this,
          &QTabReactions::btnAddReaction_clicked);

  connect(ui->btnRemoveReaction, &QPushButton::clicked, this,
          &QTabReactions::btnRemoveReaction_clicked);

  connect(ui->txtReactionName, &QLineEdit::editingFinished, this,
          &QTabReactions::txtReactionName_editingFinished);

  connect(ui->cmbReactionLocation, qOverload<int>(&QComboBox::activated), this,
          &QTabReactions::cmbReactionLocation_activated);

  connect(ui->listReactionParams, &QTableWidget::currentCellChanged, this,
          &QTabReactions::listReactionParams_currentCellChanged);

  connect(ui->btnAddReactionParam, &QPushButton::clicked, this,
          &QTabReactions::btnAddReactionParam_clicked);

  connect(ui->btnRemoveReactionParam, &QPushButton::clicked, this,
          &QTabReactions::btnRemoveReactionParam_clicked);

  connect(ui->txtReactionRate, &QPlainTextMathEdit::mathChanged, this,
          &QTabReactions::txtReactionRate_mathChanged);

  connect(ui->btnSaveReactionChanges, &QPushButton::clicked, this,
          &QTabReactions::btnSaveReactionChanges_clicked);
}

QTabReactions::~QTabReactions() = default;

void QTabReactions::loadModelData(const QString &selection) {
  ui->cmbReactionLocation->clear();
  ui->listReactionParams->clear();
  enableWidgets(false);
  auto *ls = ui->listReactions;
  ls->clear();
  for (int i = 0; i < sbmlDoc.compartments.size(); ++i) {
    // add compartment as top level item
    const auto &name = sbmlDoc.compartmentNames.at(i);
    QTreeWidgetItem *comp = new QTreeWidgetItem(ls, QStringList({name}));
    ls->addTopLevelItem(comp);
    ui->cmbReactionLocation->addItem(name);
    for (const auto &reacID : sbmlDoc.reactions.at(sbmlDoc.compartments[i])) {
      // add each reaction as child of compartment
      comp->addChild(new QTreeWidgetItem(
          comp, QStringList({sbmlDoc.getReactionName(reacID)})));
    }
  }
  for (int i = 0; i < sbmlDoc.membranes.size(); ++i) {
    // add compartment as top level item
    const auto &name = sbmlDoc.membraneNames.at(i);
    QTreeWidgetItem *memb = new QTreeWidgetItem(ls, QStringList({name}));
    ls->addTopLevelItem(memb);
    ui->cmbReactionLocation->addItem(name);
    for (const auto &reacID : sbmlDoc.reactions.at(sbmlDoc.membranes[i])) {
      // add each reaction as child of compartment
      memb->addChild(new QTreeWidgetItem(
          memb, QStringList({sbmlDoc.getReactionName(reacID)})));
    }
  }
  ls->expandAll();
  selectMatchingOrFirstChild(ls, selection);
}

void QTabReactions::enableWidgets(bool enable) {
  ui->btnRemoveReaction->setEnabled(enable);
  ui->txtReactionName->setEnabled(enable);
  ui->cmbReactionLocation->setEnabled(enable);
  ui->listReactionSpecies->setEnabled(enable);
  ui->listReactionParams->setEnabled(enable);
  ui->btnAddReactionParam->setEnabled(enable);
  ui->btnRemoveReactionParam->setEnabled(enable);
  ui->txtReactionRate->setEnabled(enable);
  ui->btnSaveReactionChanges->setEnabled(enable);
}

void QTabReactions::listReactions_currentItemChanged(
    QTreeWidgetItem *current, QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  ui->txtReactionName->clear();
  ui->listReactionSpecies->clear();
  ui->listReactionParams->setRowCount(0);
  ui->txtReactionRate->clear();
  ui->lblReactionRateStatus->clear();
  ui->btnSaveReactionChanges->setEnabled(false);
  if (current != nullptr && current->parent() == nullptr) {
    // user selected a compartment or membrane: update image
    int i = ui->listReactions->indexOfTopLevelItem(current);
    if (i < sbmlDoc.compartments.size()) {
      lblGeometry->setImage(
          sbmlDoc.mapCompIdToGeometry.at(sbmlDoc.compartments.at(i))
              .getCompartmentImage());
    } else {
      i -= sbmlDoc.compartments.size();
      lblGeometry->setImage(sbmlDoc.getMembraneImage(sbmlDoc.membranes.at(i)));
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
  bool isMembrane = false;
  int locationIndex = ui->listReactions->indexOfTopLevelItem(current->parent());
  int compartmentIndex = locationIndex;
  if (compartmentIndex >= sbmlDoc.compartments.size()) {
    isMembrane = true;
    compartmentIndex -= sbmlDoc.compartments.size();
  }
  QString compartmentID;
  if (isMembrane) {
    compartmentID = sbmlDoc.membranes.at(compartmentIndex);
    lblGeometry->setImage(sbmlDoc.getMembraneImage(compartmentID));
  } else {
    compartmentID = sbmlDoc.compartments.at(compartmentIndex);
    lblGeometry->setImage(
        sbmlDoc.mapCompIdToGeometry.at(compartmentID).getCompartmentImage());
  }
  int reactionIndex = current->parent()->indexOfChild(current);
  QString reactionID = sbmlDoc.reactions.at(compartmentID).at(reactionIndex);
  SPDLOG_DEBUG("  - reaction index {}", reactionIndex);
  SPDLOG_DEBUG("  - reaction Id {}", reactionID.toStdString());
  SPDLOG_DEBUG("  - compartment index {}", compartmentIndex);
  SPDLOG_DEBUG("  - compartment Id {}", compartmentID.toStdString());

  // display reaction information
  currentReac = sbmlDoc.getReaction(reactionID);
  currentReacLocIndex = locationIndex;
  ui->txtReactionName->setText(currentReac.name.c_str());
  ui->cmbReactionLocation->setCurrentIndex(locationIndex);
  // reset variables to only built-in functions
  ui->txtReactionRate->setVariables(
      {"sin", "cos", "exp", "log", "ln", "pow", "sqrt"});
  // get model global parameters
  for (const auto &[id, name, value] : sbmlDoc.getGlobalConstants()) {
    ui->txtReactionRate->addVariable(id, name);
  }
  // add model functions
  for (const auto &f : sbmlDoc.functions) {
    auto func = sbmlDoc.getFunctionDefinition(f);
    ui->txtReactionRate->addVariable(func.id, func.name);
  }
  // species stoich
  for (const auto &compID : currentReac.compartments) {
    auto *comp = new QTreeWidgetItem;
    comp->setText(0, sbmlDoc.getCompartmentName(compID.c_str()));
    ui->listReactionSpecies->addTopLevelItem(comp);
    for (const auto &s : sbmlDoc.species.at(compID.c_str())) {
      ui->txtReactionRate->addVariable(s.toStdString(),
                                       sbmlDoc.getSpeciesName(s).toStdString());
      auto *item = new QTreeWidgetItem;
      item->setText(0, sbmlDoc.getSpeciesName(s));
      auto *spinBox = new QSpinBox(ui->listReactionSpecies);
      spinBox->setRange(-99, 99);
      comp->addChild(item);
      ui->listReactionSpecies->setItemWidget(item, 1, spinBox);
      connect(spinBox, qOverload<int>(&QSpinBox::valueChanged),
              [lst = ui->listReactionSpecies, item](int i) {
                if (i == 0) {
                  item->setData(0, Qt::BackgroundRole, {});
                  item->setData(1, Qt::BackgroundRole, {});
                } else {
                  QColor col = i > 0 ? Qt::green : Qt::red;
                  item->setData(0, Qt::BackgroundRole, QBrush(col));
                  item->setData(1, Qt::BackgroundRole, QBrush(col));
                }
              });
      for (const auto &[speciesID, name, stoich] : currentReac.species) {
        if (speciesID == s.toStdString()) {
          spinBox->setValue(static_cast<int>(stoich));
        }
      }
    }
  }
  ui->listReactionSpecies->expandAll();
  for (const auto &[id, name, value] : currentReac.constants) {
    auto *itemName = new QTableWidgetItem(name.c_str());
    itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    auto *itemValue =
        new QTableWidgetItem(QString("%1").arg(value, 14, 'g', 14));
    itemValue->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                        Qt::ItemIsEditable);
    int n = ui->listReactionParams->rowCount();
    ui->listReactionParams->setRowCount(n + 1);
    ui->listReactionParams->setItem(n, 0, itemName);
    ui->listReactionParams->setItem(n, 1, itemValue);
    ui->txtReactionRate->addVariable(id, name);
  }
  ui->txtReactionRate->importVariableMath(currentReac.expression.c_str());
}

void QTabReactions::btnAddReaction_clicked() {
  // get currently selected compartment
  int index = 0;
  if (auto *item = ui->listReactions->currentItem(); item != nullptr) {
    auto *parent = item->parent() != nullptr ? item->parent() : item;
    index = ui->listReactions->indexOfTopLevelItem(parent);
  }
  QString locationId;
  int nComps = sbmlDoc.compartments.size();
  if (index < nComps) {
    locationId = sbmlDoc.compartments.at(index);
  } else {
    // if a membrane, then just use the first compartment
    locationId = sbmlDoc.compartments[0];
  }
  bool ok;
  auto reactionName = QInputDialog::getText(
      this, "Add reaction", "New reaction name:", QLineEdit::Normal, {}, &ok);
  if (ok) {
    sbmlDoc.addReaction(reactionName, locationId);
    loadModelData(reactionName);
  }
}

void QTabReactions::btnRemoveReaction_clicked() {
  auto msgbox =
      newYesNoMessageBox("Remove reaction?",
                         QString("Remove reaction '%1' from the model?")
                             .arg(ui->listReactions->currentItem()->text(0)),
                         this);
  connect(msgbox, &QMessageBox::finished, this, [this](int result) {
    if (result == QMessageBox::Yes) {
      SPDLOG_INFO("Removing reaction {}", currentReac.id);
      sbmlDoc.removeReaction(currentReac.id.c_str());
      loadModelData();
    }
  });
  msgbox->open();
}

void QTabReactions::txtReactionName_editingFinished() {
  auto name = ui->txtReactionName->text();
  currentReac.name = name.toStdString();
  sbmlDoc.setReaction(currentReac);
  ui->listReactions->currentItem()->setText(0, name);
}

void QTabReactions::cmbReactionLocation_activated(int index) {
  QString locationId;
  int nComps = sbmlDoc.compartments.size();
  if (index < nComps) {
    locationId = sbmlDoc.compartments.at(index);
  } else {
    locationId = sbmlDoc.membranes.at(index - nComps);
  }
  if (locationId.toStdString() == currentReac.locationId) {
    return;
  }
  auto msgbox = newYesNoMessageBox(
      "Change reaction location?",
      QString("Change reaction '%1' location? (Species stoichiometry and rate "
              "equation will be removed)")
          .arg(ui->listReactions->currentItem()->text(0)),
      this);
  connect(msgbox, &QMessageBox::finished, this,
          [index, locationId, this](int result) {
            if (result == QMessageBox::Yes) {
              sbmlDoc.setReactionLocation(currentReac.id.c_str(), locationId);
              currentReacLocIndex = index;
              currentReac.locationId = locationId.toStdString();
              loadModelData(
                  currentReac.name.c_str());  // todo: select this reaction
            } else {
              // reset location
              ui->cmbReactionLocation->setCurrentIndex(currentReacLocIndex);
            }
          });
  msgbox->open();
}

void QTabReactions::listReactionParams_currentCellChanged(int currentRow,
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

void QTabReactions::btnAddReactionParam_clicked() {
  bool ok;
  auto name =
      QInputDialog::getText(this, "Add reaction parameter",
                            "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok && !name.isEmpty()) {
    SPDLOG_INFO("Adding reaction parameter {}", name.toStdString());
    auto id = sbmlDoc.nameToSId(name);
    auto *itemName = new QTableWidgetItem(name);
    itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    auto *itemValue = new QTableWidgetItem("0.0");
    itemValue->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                        Qt::ItemIsEditable);
    int n = ui->listReactionParams->rowCount();
    ui->listReactionParams->setRowCount(n + 1);
    ui->listReactionParams->setItem(n, 0, itemName);
    ui->listReactionParams->setItem(n, 1, itemValue);
    ui->txtReactionRate->addVariable(id.toStdString(), name.toStdString());
    currentReac.constants.push_back(
        {id.toStdString(), name.toStdString(), 0.0});
    ui->listReactionParams->setCurrentItem(itemValue);
  }
}

void QTabReactions::btnRemoveReactionParam_clicked() {
  int row = ui->listReactionParams->currentRow();
  if ((row < 0) || (row > ui->listReactionParams->rowCount() - 1)) {
    return;
  }
  const auto &param = ui->listReactionParams->item(row, 0)->text();
  auto msgbox = newYesNoMessageBox(
      "Remove reaction parameter?",
      QString("Remove reaction parameter '%1' from the model?").arg(param),
      this);
  connect(msgbox, &QMessageBox::finished, this, [param, row, this](int result) {
    if (result == QMessageBox::Yes) {
      SPDLOG_INFO("Removing parameter {}", param.toStdString());
      auto id = currentReac.constants.at(static_cast<std::size_t>(row)).id;
      currentReac.constants.erase(currentReac.constants.begin() + row);
      ui->listReactionParams->removeRow(row);
      ui->txtReactionRate->removeVariable(id);
    }
  });
  msgbox->open();
}

void QTabReactions::txtReactionRate_mathChanged(const QString &math, bool valid,
                                                const QString &errorMessage) {
  ui->btnSaveReactionChanges->setEnabled(valid);
  if (valid) {
    SPDLOG_INFO("new math: {}", math.toStdString());
    ui->lblReactionRateStatus->setText("");
  } else {
    SPDLOG_INFO("math err: {}", errorMessage.toStdString());
    ui->lblReactionRateStatus->setText(errorMessage);
  }
}

void QTabReactions::btnSaveReactionChanges_clicked() {
  if (!ui->txtReactionRate->mathIsValid()) {
    return;
  }
  currentReac.name = ui->txtReactionName->text().toStdString();
  int index = ui->cmbReactionLocation->currentIndex();
  QString locationId;
  int nComps = sbmlDoc.compartments.size();
  if (index < nComps) {
    locationId = sbmlDoc.compartments.at(index);
  } else {
    locationId = sbmlDoc.membranes.at(index - nComps);
  }
  currentReac.locationId = locationId.toStdString();
  // get reactants/products
  currentReac.species.clear();
  const auto *lst = ui->listReactionSpecies;
  for (int iLoc = 0; iLoc < lst->topLevelItemCount(); ++iLoc) {
    const auto *comp = lst->topLevelItem(iLoc);
    std::string compartmentId =
        currentReac.compartments[static_cast<std::size_t>(iLoc)];
    SPDLOG_INFO("compartmentId: {}", compartmentId);
    const auto &species = sbmlDoc.species.at(compartmentId.c_str());
    for (int iSpec = 0; iSpec < comp->childCount(); ++iSpec) {
      std::string speciesId = species.at(iSpec).toStdString();
      auto speciesName =
          sbmlDoc.getSpeciesName(speciesId.c_str()).toStdString();
      int stoich = 0;
      if (const auto *spin = qobject_cast<const QSpinBox *>(
              lst->itemWidget(comp->child(iSpec), 1));
          spin != nullptr) {
        stoich = spin->value();
      }
      currentReac.species.push_back(
          {speciesId, speciesName, static_cast<double>(stoich)});
      SPDLOG_INFO("- speciesId {}: stoich {}", speciesId, stoich);
    }
  }
  // add parameter values
  for (int i = 0; i < ui->listReactionParams->rowCount(); ++i) {
    auto &param = currentReac.constants.at(static_cast<std::size_t>(i));
    param.value = ui->listReactionParams->item(i, 1)->text().toDouble();
    SPDLOG_INFO("param {} = {}", param.id, param.value);
    // todo: add proper checking of numerical value
  }
  // set expression
  currentReac.expression = ui->txtReactionRate->getVariableMath();
  sbmlDoc.setReaction(currentReac);
}
