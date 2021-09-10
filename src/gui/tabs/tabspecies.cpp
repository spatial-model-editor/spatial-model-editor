#include "tabspecies.hpp"
#include "dialoganalytic.hpp"
#include "dialogconcentrationimage.hpp"
#include "guiutils.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "qlabelmousetracker.hpp"
#include "ui_tabspecies.h"
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>

TabSpecies::TabSpecies(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                       QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabSpecies>()}, model(m),
      lblGeometry(mouseTracker) {
  ui->setupUi(this);
  connect(ui->listSpecies, &QTreeWidget::currentItemChanged, this,
          &TabSpecies::listSpecies_currentItemChanged);
  connect(ui->btnAddSpecies, &QPushButton::clicked, this,
          &TabSpecies::btnAddSpecies_clicked);
  connect(ui->btnRemoveSpecies, &QPushButton::clicked, this,
          &TabSpecies::btnRemoveSpecies_clicked);
  connect(ui->txtSpeciesName, &QLineEdit::editingFinished, this,
          &TabSpecies::txtSpeciesName_editingFinished);
  connect(ui->cmbSpeciesCompartment, qOverload<int>(&QComboBox::activated),
          this, &TabSpecies::cmbSpeciesCompartment_activated);
  connect(ui->chkSpeciesIsSpatial, &QCheckBox::toggled, this,
          &TabSpecies::chkSpeciesIsSpatial_toggled);
  connect(ui->chkSpeciesIsConstant, &QCheckBox::toggled, this,
          &TabSpecies::chkSpeciesIsConstant_toggled);
  connect(ui->radInitialConcentrationUniform, &QRadioButton::toggled, this,
          &TabSpecies::radInitialConcentration_toggled);
  connect(ui->txtInitialConcentration, &QLineEdit::editingFinished, this,
          &TabSpecies::txtInitialConcentration_editingFinished);
  connect(ui->radInitialConcentrationImage, &QRadioButton::toggled, this,
          &TabSpecies::radInitialConcentration_toggled);
  connect(ui->radInitialConcentrationAnalytic, &QRadioButton::toggled, this,
          &TabSpecies::radInitialConcentration_toggled);
  connect(ui->btnEditAnalyticConcentration, &QPushButton::clicked, this,
          &TabSpecies::btnEditAnalyticConcentration_clicked);
  connect(ui->btnEditImageConcentration, &QPushButton::clicked, this,
          &TabSpecies::btnEditImageConcentration_clicked);
  connect(ui->txtDiffusionConstant, &QLineEdit::editingFinished, this,
          &TabSpecies::txtDiffusionConstant_editingFinished);
  connect(ui->btnChangeSpeciesColour, &QPushButton::clicked, this,
          &TabSpecies::btnChangeSpeciesColour_clicked);
}

TabSpecies::~TabSpecies() = default;

void TabSpecies::loadModelData(const QString &selection) {
  enableWidgets(false);
  // update tree list of species
  auto *ls = ui->listSpecies;
  ls->clear();
  ui->cmbSpeciesCompartment->clear();
  for (const auto &compId : model.getCompartments().getIds()) {
    // add compartments as top level items
    QString compName = model.getCompartments().getName(compId);
    QTreeWidgetItem *comp = new QTreeWidgetItem(ls, {compName});
    // also add to species compartment combo box
    ui->cmbSpeciesCompartment->addItem(compName);
    ls->addTopLevelItem(comp);
    for (const auto &speciesName : model.getSpecies().getNames(compId)) {
      // add each species as child of compartment
      comp->addChild(new QTreeWidgetItem(comp, QStringList({speciesName})));
    }
  }
  ls->expandAll();
  selectMatchingOrFirstChild(ls, selection);
}

void TabSpecies::enableWidgets(bool enable) {
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

void TabSpecies::listSpecies_currentItemChanged(QTreeWidgetItem *current,
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
  QString compartmentID = model.getCompartments().getIds()[compartmentIndex];
  int speciesIndex = current->parent()->indexOfChild(current);
  currentSpeciesId = model.getSpecies().getIds(compartmentID)[speciesIndex];
  SPDLOG_DEBUG("  - species index {}", speciesIndex);
  SPDLOG_DEBUG("  - species Id {}", currentSpeciesId.toStdString());
  SPDLOG_DEBUG("  - compartment index {}", compartmentIndex);
  SPDLOG_DEBUG("  - compartment Id {}", compartmentID.toStdString());
  // display species information
  const auto *field = model.getSpecies().getField(currentSpeciesId);
  ui->txtSpeciesName->setText(current->text(0));
  ui->cmbSpeciesCompartment->setCurrentIndex(compartmentIndex);
  // spatial
  bool isSpatial = field->getIsSpatial();
  ui->chkSpeciesIsSpatial->setChecked(isSpatial);
  ui->txtDiffusionConstant->setEnabled(isSpatial);
  ui->radInitialConcentrationAnalytic->setEnabled(isSpatial);
  ui->btnEditAnalyticConcentration->setEnabled(isSpatial);
  ui->radInitialConcentrationImage->setEnabled(isSpatial);
  ui->btnEditImageConcentration->setEnabled(isSpatial);
  // constant
  bool isConstant = model.getSpecies().getIsConstant(currentSpeciesId);
  ui->chkSpeciesIsConstant->setChecked(isConstant);
  if (isConstant) {
    ui->txtDiffusionConstant->setEnabled(false);
    ui->lblDiffusionConstantUnits->setText("");
  }
  // initial concentration
  ui->txtInitialConcentration->setText("");
  ui->lblInitialConcentrationUnits->setText("");
  lblGeometry->setImage(
      model.getSpecies().getConcentrationImage(currentSpeciesId));
  auto concentrationType{
      model.getSpecies().getInitialConcentrationType(currentSpeciesId)};
  if (concentrationType == sme::model::ConcentrationType::Uniform) {
    // scalar
    ui->txtInitialConcentration->setText(QString::number(
        model.getSpecies().getInitialConcentration(currentSpeciesId)));
    ui->lblInitialConcentrationUnits->setText(
        model.getUnits().getConcentration());
    ui->radInitialConcentrationUniform->setChecked(true);
  } else if (concentrationType == sme::model::ConcentrationType::Image) {
    ui->radInitialConcentrationImage->setChecked(true);
  } else {
    // analytic
    ui->radInitialConcentrationAnalytic->setChecked(true);
  }
  radInitialConcentration_toggled();
  // diffusion constant
  if (ui->txtDiffusionConstant->isEnabled()) {
    ui->txtDiffusionConstant->setText(QString::number(
        model.getSpecies().getDiffusionConstant(currentSpeciesId)));
    ui->lblDiffusionConstantUnits->setText(model.getUnits().getDiffusion());
  }
  // colour
  lblSpeciesColourPixmap.fill(model.getSpecies().getColour(currentSpeciesId));
  ui->lblSpeciesColour->setPixmap(lblSpeciesColourPixmap);
  ui->lblSpeciesColour->setText("");
}

void TabSpecies::btnAddSpecies_clicked() {
  // get currently selected compartment
  int compartmentIndex = 0;
  if (auto *item = ui->listSpecies->currentItem(); item != nullptr) {
    auto *parent = item->parent() != nullptr ? item->parent() : item;
    compartmentIndex = ui->listSpecies->indexOfTopLevelItem(parent);
  }
  QString compartmentID = model.getCompartments().getIds()[compartmentIndex];
  bool ok;
  auto speciesName = QInputDialog::getText(
      this, "Add species", "New species name:", QLineEdit::Normal, {}, &ok);
  if (ok && !speciesName.isEmpty()) {
    auto newName = model.getSpecies().add(speciesName, compartmentID);
    loadModelData(newName);
  }
}

void TabSpecies::btnRemoveSpecies_clicked() {
  if (auto *item = ui->listSpecies->currentItem();
      (item != nullptr) && (item->parent() != nullptr)) {
    SPDLOG_DEBUG("item {} / {} selected", item->parent()->text(0).toStdString(),
                 item->text(0).toStdString());
    int compartmentIndex = ui->listSpecies->indexOfTopLevelItem(item->parent());
    QString compartmentID = model.getCompartments().getIds()[compartmentIndex];
    int speciesIndex = item->parent()->indexOfChild(item);
    QString speciesID = model.getSpecies().getIds(compartmentID)[speciesIndex];
    auto result{QMessageBox::question(
        this, "Remove species?",
        QString("Remove species '%1' from the model?").arg(item->text(0)),
        QMessageBox::Yes | QMessageBox::No)};
    if (result == QMessageBox::Yes) {
      model.getSpecies().remove(speciesID);
      loadModelData();
    }
  }
}

void TabSpecies::txtSpeciesName_editingFinished() {
  const QString &name = ui->txtSpeciesName->text();
  if (name == model.getSpecies().getName(currentSpeciesId)) {
    return;
  }
  QString newName = model.getSpecies().setName(currentSpeciesId, name);
  ui->txtSpeciesName->setText(newName);
  loadModelData(newName);
}

void TabSpecies::cmbSpeciesCompartment_activated(int index) {
  const auto &currentCompartmentId{
      model.getSpecies().getCompartment(currentSpeciesId)};
  const auto &newCompartmentId{model.getCompartments().getIds()[index]};
  if (newCompartmentId == currentCompartmentId) {
    return;
  }
  model.getSpecies().setCompartment(currentSpeciesId, newCompartmentId);
  loadModelData(model.getSpecies().getName(currentSpeciesId));
}

void TabSpecies::chkSpeciesIsSpatial_toggled(bool enabled) {
  // if new value differs from previous one - update model
  if (model.getSpecies().getIsSpatial(currentSpeciesId) != enabled) {
    SPDLOG_INFO("setting species {} isSpatial: {}",
                currentSpeciesId.toStdString(), enabled);
    model.getSpecies().setIsSpatial(currentSpeciesId, enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void TabSpecies::chkSpeciesIsConstant_toggled(bool enabled) {
  const auto &speciesID = currentSpeciesId;
  // if new value differs from previous one - update model
  if (model.getSpecies().getIsConstant(speciesID) != enabled) {
    SPDLOG_INFO("setting species {} isConstant: {}", speciesID.toStdString(),
                enabled);
    model.getSpecies().setIsConstant(speciesID, enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void TabSpecies::radInitialConcentration_toggled() {
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

void TabSpecies::txtInitialConcentration_editingFinished() {
  double initConc = ui->txtInitialConcentration->text().toDouble();
  SPDLOG_INFO("setting initial concentration of Species {} to {}",
              currentSpeciesId.toStdString(), initConc);
  model.getSpecies().setInitialConcentration(currentSpeciesId, initConc);
  // update displayed info for this species
  listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
}

void TabSpecies::btnEditAnalyticConcentration_clicked() {
  SPDLOG_DEBUG("editing analytic initial concentration of species {}...",
               currentSpeciesId.toStdString());
  DialogAnalytic dialog(
      model.getSpecies().getAnalyticConcentration(currentSpeciesId),
      model.getSpeciesGeometry(currentSpeciesId), model.getParameters(),
      model.getFunctions(), model.getDisplayOptions().invertYAxis);
  if (dialog.exec() == QDialog::Accepted) {
    const std::string &expr = dialog.getExpression();
    SPDLOG_DEBUG("  - set expr: {}", expr);
    model.getSpecies().setAnalyticConcentration(currentSpeciesId, expr.c_str());
    lblGeometry->setImage(
        model.getSpecies().getConcentrationImage(currentSpeciesId));
  }
}

void TabSpecies::btnEditImageConcentration_clicked() {
  SPDLOG_DEBUG("editing initial concentration image for species {}...",
               currentSpeciesId.toStdString());
  DialogConcentrationImage dialog(
      model.getSpecies().getSampledFieldConcentration(currentSpeciesId),
      model.getSpeciesGeometry(currentSpeciesId),
      model.getDisplayOptions().invertYAxis);
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("  - setting new sampled field concentration array");
    model.getSpecies().setSampledFieldConcentration(
        currentSpeciesId, dialog.getConcentrationArray());
    lblGeometry->setImage(
        model.getSpecies().getConcentrationImage(currentSpeciesId));
  }
}

void TabSpecies::txtDiffusionConstant_editingFinished() {
  double diffConst = ui->txtDiffusionConstant->text().toDouble();
  SPDLOG_INFO("setting Diffusion Constant of Species {} to {}",
              currentSpeciesId.toStdString(), diffConst);
  model.getSpecies().setDiffusionConstant(currentSpeciesId, diffConst);
}

void TabSpecies::btnChangeSpeciesColour_clicked() {
  SPDLOG_DEBUG("waiting for new colour for species {} from user...",
               currentSpeciesId.toStdString());
  QColor newCol = QColorDialog::getColor(
      model.getSpecies().getColour(currentSpeciesId), this,
      "Choose new species colour", QColorDialog::DontUseNativeDialog);
  if (newCol.isValid()) {
    SPDLOG_DEBUG("  - set new colour to {:x}", newCol.rgb());
    model.getSpecies().setColour(currentSpeciesId, newCol.rgb());
    listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
  }
}
