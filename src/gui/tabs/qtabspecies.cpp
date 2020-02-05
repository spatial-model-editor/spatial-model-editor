#include "qtabspecies.hpp"

#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "dialoganalytic.hpp"
#include "dialogconcentrationimage.hpp"
#include "guiutils.hpp"
#include "logger.hpp"
#include "qlabelmousetracker.hpp"
#include "sbml.hpp"
#include "ui_qtabspecies.h"

QTabSpecies::QTabSpecies(sbml::SbmlDocWrapper &doc,
                         QLabelMouseTracker *mouseTracker, QWidget *parent)
    : QWidget(parent),
      ui{std::make_unique<Ui::QTabSpecies>()},
      sbmlDoc(doc),
      lblGeometry(mouseTracker) {
  ui->setupUi(this);

  connect(ui->listSpecies, &QTreeWidget::currentItemChanged, this,
          &QTabSpecies::listSpecies_currentItemChanged);

  connect(ui->btnAddSpecies, &QPushButton::clicked, this,
          &QTabSpecies::btnAddSpecies_clicked);

  connect(ui->btnRemoveSpecies, &QPushButton::clicked, this,
          &QTabSpecies::btnRemoveSpecies_clicked);

  connect(ui->txtSpeciesName, &QLineEdit::editingFinished, this,
          &QTabSpecies::txtSpeciesName_editingFinished);

  connect(ui->cmbSpeciesCompartment, qOverload<int>(&QComboBox::activated),
          this, &QTabSpecies::cmbSpeciesCompartment_activated);

  connect(ui->chkSpeciesIsSpatial, &QCheckBox::toggled, this,
          &QTabSpecies::chkSpeciesIsSpatial_toggled);

  connect(ui->chkSpeciesIsConstant, &QCheckBox::toggled, this,
          &QTabSpecies::chkSpeciesIsConstant_toggled);

  connect(ui->radInitialConcentrationUniform, &QRadioButton::toggled, this,
          &QTabSpecies::radInitialConcentration_toggled);

  connect(ui->txtInitialConcentration, &QLineEdit::editingFinished, this,
          &QTabSpecies::txtInitialConcentration_editingFinished);

  connect(ui->radInitialConcentrationImage, &QRadioButton::toggled, this,
          &QTabSpecies::radInitialConcentration_toggled);

  connect(ui->radInitialConcentrationAnalytic, &QRadioButton::toggled, this,
          &QTabSpecies::radInitialConcentration_toggled);

  connect(ui->btnEditAnalyticConcentration, &QPushButton::clicked, this,
          &QTabSpecies::btnEditAnalyticConcentration_clicked);

  connect(ui->btnEditImageConcentration, &QPushButton::clicked, this,
          &QTabSpecies::btnEditImageConcentration_clicked);

  connect(ui->txtDiffusionConstant, &QLineEdit::editingFinished, this,
          &QTabSpecies::txtDiffusionConstant_editingFinished);

  connect(ui->btnChangeSpeciesColour, &QPushButton::clicked, this,
          &QTabSpecies::btnChangeSpeciesColour_clicked);
}

QTabSpecies::~QTabSpecies() = default;

void QTabSpecies::loadModelData(const QString &selection) {
  enableWidgets(false);
  // update tree list of species
  auto *ls = ui->listSpecies;
  ls->clear();
  ui->cmbSpeciesCompartment->clear();
  for (int iComp = 0; iComp < sbmlDoc.compartments.size(); ++iComp) {
    // add compartments as top level items
    QString compName = sbmlDoc.compartmentNames[iComp];
    QTreeWidgetItem *comp = new QTreeWidgetItem(ls, {compName});
    // also add to species compartment combo box
    ui->cmbSpeciesCompartment->addItem(compName);
    ls->addTopLevelItem(comp);
    for (const auto &s : sbmlDoc.species.at(sbmlDoc.compartments[iComp])) {
      // add each species as child of compartment
      comp->addChild(
          new QTreeWidgetItem(comp, QStringList({sbmlDoc.getSpeciesName(s)})));
    }
  }
  ls->expandAll();
  selectMatchingOrFirstChild(ls, selection);
}

void QTabSpecies::enableWidgets(bool enable) {
  ui->btnRemoveSpecies->setEnabled(enable);
  ui->txtSpeciesName->setEnabled(enable);
  ui->cmbSpeciesCompartment->setEnabled(enable);
  ui->chkSpeciesIsSpatial->setEnabled(enable);
  ui->chkSpeciesIsConstant->setEnabled(enable);
  ui->radInitialConcentrationUniform->setEnabled(enable);
  ui->txtInitialConcentration->setEnabled(enable);
  ui->radInitialConcentrationAnalytic->setEnabled(enable);
  ui->btnEditAnalyticConcentration->setEnabled(enable);
  ui->radInitialConcentrationImage->setEnabled(enable);
  ui->btnEditImageConcentration->setEnabled(enable);
  ui->txtDiffusionConstant->setEnabled(enable);
  ui->btnChangeSpeciesColour->setEnabled(enable);
}

void QTabSpecies::listSpecies_currentItemChanged(QTreeWidgetItem *current,
                                                 QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  if ((current == nullptr) || (current->parent() == nullptr)) {
    // user selection is not a species
    enableWidgets(false);
    return;
  }

  SPDLOG_DEBUG("item {} / {} selected",
               current->parent()->text(0).toStdString(),
               current->text(0).toStdString());
  enableWidgets(true);
  int compartmentIndex =
      ui->listSpecies->indexOfTopLevelItem(current->parent());
  QString compartmentID = sbmlDoc.compartments.at(compartmentIndex);
  int speciesIndex = current->parent()->indexOfChild(current);
  QString speciesID = sbmlDoc.species.at(compartmentID).at(speciesIndex);
  currentSpeciesId = speciesID;
  SPDLOG_DEBUG("  - species index {}", speciesIndex);
  SPDLOG_DEBUG("  - species Id {}", speciesID.toStdString());
  SPDLOG_DEBUG("  - compartment index {}", compartmentIndex);
  SPDLOG_DEBUG("  - compartment Id {}", compartmentID.toStdString());
  // display species information
  const auto &field = sbmlDoc.mapSpeciesIdToField.at(speciesID);
  ui->txtSpeciesName->setText(current->text(0));
  ui->cmbSpeciesCompartment->setCurrentIndex(compartmentIndex);
  // spatial
  bool isSpatial = field.isSpatial;
  ui->chkSpeciesIsSpatial->setChecked(isSpatial);
  ui->txtDiffusionConstant->setEnabled(isSpatial);
  ui->radInitialConcentrationAnalytic->setEnabled(isSpatial);
  ui->btnEditAnalyticConcentration->setEnabled(isSpatial);
  ui->radInitialConcentrationImage->setEnabled(isSpatial);
  ui->btnEditImageConcentration->setEnabled(isSpatial);
  // constant
  bool isConstant = sbmlDoc.getIsSpeciesConstant(speciesID.toStdString());
  ui->chkSpeciesIsConstant->setChecked(isConstant);
  if (isConstant) {
    ui->txtDiffusionConstant->setEnabled(false);
    ui->lblDiffusionConstantUnits->setText("");
  }
  // initial concentration
  ui->txtInitialConcentration->setText("");
  ui->lblInitialConcentrationUnits->setText("");
  // lblGeometryStatus->setText(
  //    QString("Species '%1' concentration:").arg(current->text(0)));
  lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
  if (field.isUniformConcentration) {
    // scalar
    ui->txtInitialConcentration->setText(
        QString::number(sbmlDoc.getInitialConcentration(speciesID)));
    ui->lblInitialConcentrationUnits->setText(
        sbmlDoc.getModelUnits().getConcentration());
    ui->radInitialConcentrationUniform->setChecked(true);
  } else if (!sbmlDoc
                  .getSpeciesSampledFieldInitialAssignment(
                      speciesID.toStdString())
                  .empty()) {
    // image
    ui->radInitialConcentrationImage->setChecked(true);
  } else {
    // analytic
    ui->radInitialConcentrationAnalytic->setChecked(true);
  }
  radInitialConcentration_toggled();
  // diffusion constant
  if (ui->txtDiffusionConstant->isEnabled()) {
    ui->txtDiffusionConstant->setText(
        QString::number(sbmlDoc.getDiffusionConstant(speciesID)));
    ui->lblDiffusionConstantUnits->setText(
        sbmlDoc.getModelUnits().getDiffusion());
  }
  // colour
  lblSpeciesColourPixmap.fill(sbmlDoc.getSpeciesColour(speciesID));
  ui->lblSpeciesColour->setPixmap(lblSpeciesColourPixmap);
  ui->lblSpeciesColour->setText("");
}

void QTabSpecies::btnAddSpecies_clicked() {
  // get currently selected compartment
  int compartmentIndex = 0;
  if (auto *item = ui->listSpecies->currentItem(); item != nullptr) {
    auto *parent = item->parent() != nullptr ? item->parent() : item;
    compartmentIndex = ui->listSpecies->indexOfTopLevelItem(parent);
  }
  QString compartmentID = sbmlDoc.compartments.at(compartmentIndex);
  bool ok;
  auto speciesName = QInputDialog::getText(
      this, "Add species", "New species name:", QLineEdit::Normal, {}, &ok);
  if (ok && !speciesName.isEmpty()) {
    sbmlDoc.addSpecies(speciesName, compartmentID);
    loadModelData(speciesName);
  }
}

void QTabSpecies::btnRemoveSpecies_clicked() {
  if (auto *item = ui->listSpecies->currentItem();
      (item != nullptr) && (item->parent() != nullptr)) {
    SPDLOG_DEBUG("item {} / {} selected", item->parent()->text(0).toStdString(),
                 item->text(0).toStdString());
    int compartmentIndex = ui->listSpecies->indexOfTopLevelItem(item->parent());
    QString compartmentID = sbmlDoc.compartments.at(compartmentIndex);
    int speciesIndex = item->parent()->indexOfChild(item);
    QString speciesID = sbmlDoc.species.at(compartmentID).at(speciesIndex);
    auto msgbox = newYesNoMessageBox(
        "Remove species?",
        QString("Remove species '%1' from the model?").arg(item->text(0)),
        this);
    connect(msgbox, &QMessageBox::finished, this,
            [speciesID, this](int result) {
              if (result == QMessageBox::Yes) {
                sbmlDoc.removeSpecies(speciesID);
                loadModelData();
              }
            });
    msgbox->open();
  }
}

void QTabSpecies::txtSpeciesName_editingFinished() {
  const QString &name = ui->txtSpeciesName->text();
  sbmlDoc.setSpeciesName(currentSpeciesId, name);
  loadModelData(name);
}

void QTabSpecies::cmbSpeciesCompartment_activated(int index) {
  const auto &currentComp = sbmlDoc.getSpeciesCompartment(currentSpeciesId);
  if (sbmlDoc.compartments[index] != currentComp) {
    sbmlDoc.setSpeciesCompartment(currentSpeciesId,
                                  sbmlDoc.compartments[index]);
    loadModelData(sbmlDoc.getSpeciesName(currentSpeciesId));
  }
}

void QTabSpecies::chkSpeciesIsSpatial_toggled(bool enabled) {
  // if new value differs from previous one - update model
  if (sbmlDoc.getIsSpatial(currentSpeciesId) != enabled) {
    SPDLOG_INFO("setting species {} isSpatial: {}",
                currentSpeciesId.toStdString(), enabled);
    sbmlDoc.setIsSpatial(currentSpeciesId, enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void QTabSpecies::chkSpeciesIsConstant_toggled(bool enabled) {
  const auto &speciesID = currentSpeciesId.toStdString();
  // if new value differs from previous one - update model
  if (sbmlDoc.getIsSpeciesConstant(speciesID) != enabled) {
    SPDLOG_INFO("setting species {} isConstant: {}", speciesID, enabled);
    sbmlDoc.setIsSpeciesConstant(speciesID, enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void QTabSpecies::radInitialConcentration_toggled() {
  if (ui->radInitialConcentrationUniform->isChecked()) {
    ui->txtInitialConcentration->setEnabled(true);
    ui->btnEditAnalyticConcentration->setEnabled(false);
    ui->btnEditImageConcentration->setEnabled(false);
  } else if (ui->radInitialConcentrationImage->isChecked()) {
    ui->txtInitialConcentration->setEnabled(false);
    ui->btnEditAnalyticConcentration->setEnabled(false);
    ui->btnEditImageConcentration->setEnabled(true);
  } else {
    ui->txtInitialConcentration->setEnabled(false);
    ui->btnEditAnalyticConcentration->setEnabled(true);
    ui->btnEditImageConcentration->setEnabled(false);
  }
}

void QTabSpecies::txtInitialConcentration_editingFinished() {
  double initConc = ui->txtInitialConcentration->text().toDouble();
  const auto &speciesID = currentSpeciesId;
  SPDLOG_INFO("setting initial concentration of Species {} to {}",
              speciesID.toStdString(), initConc);
  sbmlDoc.setInitialConcentration(speciesID, initConc);
  // update displayed info for this species
  listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
}

void QTabSpecies::btnEditAnalyticConcentration_clicked() {
  const auto &speciesID = currentSpeciesId;
  SPDLOG_DEBUG("editing analytic initial concentration of species {}...",
               speciesID.toStdString());
  DialogAnalytic dialog(sbmlDoc.getAnalyticConcentration(speciesID),
                        sbmlDoc.getSpeciesGeometry(speciesID),
                        sbmlDoc.getGlobalConstants());
  if (dialog.exec() == QDialog::Accepted) {
    const std::string &expr = dialog.getExpression();
    SPDLOG_DEBUG("  - set expr: {}", expr);
    sbmlDoc.setAnalyticConcentration(speciesID, expr.c_str());
    lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
  }
}

void QTabSpecies::btnEditImageConcentration_clicked() {
  const auto &speciesID = currentSpeciesId;
  SPDLOG_DEBUG("editing initial concentration image for species {}...",
               speciesID.toStdString());
  DialogConcentrationImage dialog(
      sbmlDoc.getSampledFieldConcentration(speciesID),
      sbmlDoc.getSpeciesGeometry(speciesID));
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("  - setting new sampled field concentration array");
    sbmlDoc.setSampledFieldConcentration(speciesID,
                                         dialog.getConcentrationArray());
    lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
  }
}

void QTabSpecies::txtDiffusionConstant_editingFinished() {
  double diffConst = ui->txtDiffusionConstant->text().toDouble();
  const auto &speciesID = currentSpeciesId;
  SPDLOG_INFO("setting Diffusion Constant of Species {} to {}",
              speciesID.toStdString(), diffConst);
  sbmlDoc.setDiffusionConstant(speciesID, diffConst);
}

void QTabSpecies::btnChangeSpeciesColour_clicked() {
  const auto &speciesID = currentSpeciesId;
  SPDLOG_DEBUG("waiting for new colour for species {} from user...",
               speciesID.toStdString());
  QColor newCol = QColorDialog::getColor(sbmlDoc.getSpeciesColour(speciesID),
                                         this, "Choose new species colour",
                                         QColorDialog::DontUseNativeDialog);
  if (newCol.isValid()) {
    SPDLOG_DEBUG("  - set new colour to {:x}", newCol.rgba());
    sbmlDoc.setSpeciesColour(speciesID, newCol);
    listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
  }
}
