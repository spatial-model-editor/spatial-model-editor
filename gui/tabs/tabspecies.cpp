#include "tabspecies.hpp"
#include "dialoganalytic.hpp"
#include "dialogimagedata.hpp"
#include "guiutils.hpp"
#include "qlabelmousetracker.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "ui_tabspecies.h"
#include <QButtonGroup>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>

TabSpecies::TabSpecies(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                       QVoxelRenderer *voxelRenderer, QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabSpecies>()}, model(m),
      lblGeometry(mouseTracker), voxGeometry(voxelRenderer) {
  ui->setupUi(this);
  // set up radio button groups
  auto *initialConcGroup = new QButtonGroup(this);
  initialConcGroup->addButton(ui->radInitialConcentrationUniform);
  initialConcGroup->addButton(ui->radInitialConcentrationAnalytic);
  initialConcGroup->addButton(ui->radInitialConcentrationImage);
  auto *diffusionGroup = new QButtonGroup(this);
  diffusionGroup->addButton(ui->radDiffusionConstantUniform);
  diffusionGroup->addButton(ui->radDiffusionConstantAnalytic);
  diffusionGroup->addButton(ui->radDiffusionConstantImage);
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
  connect(ui->radDiffusionConstantUniform, &QRadioButton::toggled, this,
          &TabSpecies::radDiffusionConstant_toggled);
  connect(ui->txtDiffusionConstant, &QLineEdit::editingFinished, this,
          &TabSpecies::txtDiffusionConstant_editingFinished);
  connect(ui->radDiffusionConstantAnalytic, &QRadioButton::toggled, this,
          &TabSpecies::radDiffusionConstant_toggled);
  connect(ui->radDiffusionConstantImage, &QRadioButton::toggled, this,
          &TabSpecies::radDiffusionConstant_toggled);
  connect(ui->btnEditAnalyticDiffusionConstant, &QPushButton::clicked, this,
          &TabSpecies::btnEditAnalyticDiffusionConstant_clicked);
  connect(ui->btnEditImageDiffusionConstant, &QPushButton::clicked, this,
          &TabSpecies::btnEditImageDiffusionConstant_clicked);
  connect(ui->btnChangeSpeciesColor, &QPushButton::clicked, this,
          &TabSpecies::btnChangeSpeciesColor_clicked);
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
  ui->radDiffusionConstantUniform->setEnabled(enable);
  ui->txtDiffusionConstant->setEnabled(enable);
  ui->radDiffusionConstantAnalytic->setEnabled(enable);
  ui->btnEditAnalyticDiffusionConstant->setEnabled(enable);
  ui->radDiffusionConstantImage->setEnabled(enable);
  ui->btnEditImageDiffusionConstant->setEnabled(enable);
  ui->btnChangeSpeciesColor->setEnabled(enable);
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
  ui->radInitialConcentrationAnalytic->setEnabled(isSpatial);
  ui->btnEditAnalyticConcentration->setEnabled(isSpatial);
  ui->radInitialConcentrationImage->setEnabled(isSpatial);
  ui->btnEditImageConcentration->setEnabled(isSpatial);
  ui->radDiffusionConstantUniform->setEnabled(isSpatial);
  ui->radDiffusionConstantAnalytic->setEnabled(isSpatial);
  ui->btnEditAnalyticDiffusionConstant->setEnabled(isSpatial);
  ui->radDiffusionConstantImage->setEnabled(isSpatial);
  ui->btnEditImageDiffusionConstant->setEnabled(isSpatial);
  ui->txtDiffusionConstant->setEnabled(isSpatial);
  // constant
  bool isConstant = model.getSpecies().getIsConstant(currentSpeciesId);
  ui->chkSpeciesIsConstant->setChecked(isConstant);
  if (isConstant) {
    ui->radDiffusionConstantUniform->setEnabled(false);
    ui->txtDiffusionConstant->setEnabled(false);
    ui->lblDiffusionConstantUnits->setText("");
  }
  // initial concentration
  ui->txtInitialConcentration->setText("");
  ui->lblInitialConcentrationUnits->setText("");
  lblGeometry->setImage(
      model.getSpecies().getConcentrationImages(currentSpeciesId));
  voxGeometry->setImage(
      model.getSpecies().getConcentrationImages(currentSpeciesId));
  auto concentrationType{
      model.getSpecies().getInitialConcentrationType(currentSpeciesId)};
  if (concentrationType == sme::model::SpatialDataType::Uniform) {
    // scalar
    ui->txtInitialConcentration->setText(QString::number(
        model.getSpecies().getInitialConcentration(currentSpeciesId)));
    ui->lblInitialConcentrationUnits->setText(
        model.getUnits().getConcentration());
    ui->radInitialConcentrationUniform->setChecked(true);
  } else if (concentrationType == sme::model::SpatialDataType::Image) {
    ui->radInitialConcentrationImage->setChecked(true);
  } else {
    // analytic
    ui->radInitialConcentrationAnalytic->setChecked(true);
  }
  radInitialConcentration_toggled();

  // diffusion constant
  auto diffusionConstantType{
      model.getSpecies().getDiffusionConstantType(currentSpeciesId)};
  if (diffusionConstantType == sme::model::SpatialDataType::Uniform) {
    // scalar
    ui->txtDiffusionConstant->setText(QString::number(
        model.getSpecies().getDiffusionConstant(currentSpeciesId)));
    ui->lblDiffusionConstantUnits->setText(model.getUnits().getDiffusion());
    ui->radDiffusionConstantUniform->setChecked(true);
  } else if (diffusionConstantType == sme::model::SpatialDataType::Image) {
    ui->radDiffusionConstantImage->setChecked(true);
  } else {
    // analytic
    ui->radDiffusionConstantAnalytic->setChecked(true);
  }
  radDiffusionConstant_toggled();
  // color
  lblSpeciesColorPixmap.fill(model.getSpecies().getColor(currentSpeciesId));
  ui->lblSpeciesColor->setPixmap(lblSpeciesColorPixmap);
  ui->lblSpeciesColor->setText("");
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
      DialogAnalyticDataType::Concentration,
      model.getSpeciesGeometry(currentSpeciesId), model.getParameters(),
      model.getFunctions(), model.getDisplayOptions().invertYAxis);
  if (dialog.exec() == QDialog::Accepted) {
    const std::string &expr = dialog.getExpression();
    SPDLOG_DEBUG("  - set expr: {}", expr);
    model.getSpecies().setAnalyticConcentration(currentSpeciesId, expr.c_str());
    lblGeometry->setImage(
        model.getSpecies().getConcentrationImages(currentSpeciesId));
    voxGeometry->setImage(
        model.getSpecies().getConcentrationImages(currentSpeciesId));
  }
}

void TabSpecies::btnEditImageConcentration_clicked() {
  SPDLOG_DEBUG("editing initial concentration image for species {}...",
               currentSpeciesId.toStdString());
  DialogImageData dialog(
      model.getSpecies().getSampledFieldConcentration(currentSpeciesId),
      model.getSpeciesGeometry(currentSpeciesId),
      model.getDisplayOptions().invertYAxis,
      DialogImageDataDataType::Concentration);
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("  - setting new sampled field concentration array");
    model.getSpecies().setSampledFieldConcentration(currentSpeciesId,
                                                    dialog.getImageArray());
    const auto img =
        model.getSpecies().getConcentrationImages(currentSpeciesId);
    lblGeometry->setImage(img);
    voxGeometry->setImage(img);
  }
}

void TabSpecies::radDiffusionConstant_toggled() {
  if (ui->radDiffusionConstantUniform->isEnabled() &&
      ui->radDiffusionConstantUniform->isChecked()) {
    ui->txtDiffusionConstant->setEnabled(true);
    ui->btnEditAnalyticDiffusionConstant->setEnabled(false);
    ui->btnEditImageDiffusionConstant->setEnabled(false);
  } else if (ui->radDiffusionConstantAnalytic->isEnabled() &&
             ui->radDiffusionConstantAnalytic->isChecked()) {
    ui->txtDiffusionConstant->setEnabled(false);
    ui->btnEditAnalyticDiffusionConstant->setEnabled(true);
    ui->btnEditImageDiffusionConstant->setEnabled(false);
  } else if (ui->radDiffusionConstantImage->isEnabled() &&
             ui->radDiffusionConstantImage->isChecked()) {
    ui->txtDiffusionConstant->setEnabled(false);
    ui->btnEditAnalyticDiffusionConstant->setEnabled(false);
    ui->btnEditImageDiffusionConstant->setEnabled(true);
  }
}

void TabSpecies::txtDiffusionConstant_editingFinished() {
  double diffConst = ui->txtDiffusionConstant->text().toDouble();
  SPDLOG_INFO("setting Diffusion Constant of Species {} to {}",
              currentSpeciesId.toStdString(), diffConst);
  model.getSpecies().setDiffusionConstant(currentSpeciesId, diffConst);
}

void TabSpecies::btnEditAnalyticDiffusionConstant_clicked() {
  SPDLOG_DEBUG("editing analytic diffusion constant of species {}...",
               currentSpeciesId.toStdString());
  DialogAnalytic dialog(
      model.getSpecies().getAnalyticDiffusionConstant(currentSpeciesId),
      DialogAnalyticDataType::DiffusionConstant,
      model.getSpeciesGeometry(currentSpeciesId), model.getParameters(),
      model.getFunctions(), model.getDisplayOptions().invertYAxis);
  if (dialog.exec() == QDialog::Accepted) {
    const std::string &expr = dialog.getExpression();
    SPDLOG_DEBUG("  - set expr: {}", expr);
    model.getSpecies().setAnalyticDiffusionConstant(currentSpeciesId,
                                                    expr.c_str());
  }
}

void TabSpecies::btnEditImageDiffusionConstant_clicked() {
  SPDLOG_DEBUG("editing diffusion constant image for species {}...",
               currentSpeciesId.toStdString());
  DialogImageData dialog(
      model.getSpecies().getSampledFieldDiffusionConstant(currentSpeciesId),
      model.getSpeciesGeometry(currentSpeciesId),
      model.getDisplayOptions().invertYAxis,
      DialogImageDataDataType::DiffusionConstant);
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("  - setting new sampled field diffusion constant array");
    model.getSpecies().setSampledFieldDiffusionConstant(currentSpeciesId,
                                                        dialog.getImageArray());
  }
}

void TabSpecies::btnChangeSpeciesColor_clicked() {
  SPDLOG_DEBUG("waiting for new color for species {} from user...",
               currentSpeciesId.toStdString());
  QColor newCol = QColorDialog::getColor(
      model.getSpecies().getColor(currentSpeciesId), this,
      "Choose new species color", QColorDialog::DontUseNativeDialog);
  if (newCol.isValid()) {
    SPDLOG_DEBUG("  - set new color to {:x}", newCol.rgb());
    model.getSpecies().setColor(currentSpeciesId, newCol.rgb());
    listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
  }
}
