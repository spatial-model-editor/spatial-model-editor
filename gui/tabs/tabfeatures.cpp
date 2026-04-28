#include "tabfeatures.hpp"
#include "dialoganalytic.hpp"
#include "guiutils.hpp"
#include "qlabelmousetracker.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/feature_options.hpp"
#include "sme/geometry.hpp"
#include "sme/image_stack.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include "ui_tabfeatures.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <algorithm>

namespace {

sme::simulate::RoiType currentRoiType(const QComboBox *combo) {
  return static_cast<sme::simulate::RoiType>(combo->currentData().toInt());
}

int currentAxis(const QComboBox *combo) { return combo->currentData().toInt(); }

} // namespace

TabFeatures::TabFeatures(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                         QVoxelRenderer *voxelRenderer, QWidget *parent)
    : QWidget{parent}, ui{std::make_unique<Ui::TabFeatures>()}, model{m},
      lblGeometry{mouseTracker}, voxGeometry{voxelRenderer} {
  ui->setupUi(this);
  connect(ui->listFeatures, &QListWidget::currentRowChanged, this,
          &TabFeatures::listFeatures_currentRowChanged);
  connect(ui->btnAddFeature, &QPushButton::clicked, this,
          &TabFeatures::btnAddFeature_clicked);
  connect(ui->btnRemoveFeature, &QPushButton::clicked, this,
          &TabFeatures::btnRemoveFeature_clicked);
  connect(ui->txtFeatureName, &QLineEdit::editingFinished, this,
          &TabFeatures::txtFeatureName_editingFinished);
  connect(ui->cmbCompartment, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &TabFeatures::cmbCompartment_currentIndexChanged);
  connect(ui->cmbSpecies, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &TabFeatures::cmbSpecies_currentIndexChanged);
  connect(ui->cmbRoiType, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &TabFeatures::cmbRoiType_currentIndexChanged);
  connect(ui->btnEditExpression, &QPushButton::clicked, this,
          &TabFeatures::btnEditExpression_clicked);
  connect(ui->spnNRegions, qOverload<int>(&QSpinBox::valueChanged), this,
          &TabFeatures::spnNRegions_valueChanged);
  connect(ui->btnImportImage, &QPushButton::clicked, this,
          &TabFeatures::btnImportImage_clicked);
  connect(ui->spnBuiltInThickness, qOverload<int>(&QSpinBox::valueChanged),
          this, &TabFeatures::spnBuiltInThickness_valueChanged);
  connect(ui->cmbBuiltInAxis, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &TabFeatures::cmbBuiltInAxis_currentIndexChanged);
  connect(ui->cmbReduction, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &TabFeatures::cmbReduction_currentIndexChanged);
  ui->cmbRoiType->addItem("Analytic",
                          static_cast<int>(sme::simulate::RoiType::Analytic));
  ui->cmbRoiType->addItem("Image",
                          static_cast<int>(sme::simulate::RoiType::Image));
  ui->cmbRoiType->addItem("Depth",
                          static_cast<int>(sme::simulate::RoiType::Depth));
  ui->cmbRoiType->addItem("Axis slices",
                          static_cast<int>(sme::simulate::RoiType::AxisSlices));
  ui->cmbBuiltInAxis->addItem("x", 0);
  ui->cmbBuiltInAxis->addItem("y", 1);
  ui->cmbBuiltInAxis->addItem("z", 2);
  ui->cmbReduction->addItem("Average");
  ui->cmbReduction->addItem("Sum");
  ui->cmbReduction->addItem("Min");
  ui->cmbReduction->addItem("Max");
  ui->cmbReduction->addItem("First quantile");
  ui->cmbReduction->addItem("Median");
  ui->cmbReduction->addItem("Third quantile");
}

TabFeatures::~TabFeatures() = default;

void TabFeatures::loadModelData(const QString &selection) {
  currentFeatureIndex = -1;
  ui->listFeatures->clear();
  const auto &features = model.getFeatures().getFeatures();
  for (const auto &feat : features) {
    auto *item = new QListWidgetItem(QString::fromStdString(feat.name));
    item->setData(Qt::UserRole, QString::fromStdString(feat.id));
    ui->listFeatures->addItem(item);
  }
  if (ui->listFeatures->count() > 0) {
    int selectedIndex = 0;
    if (!selection.isEmpty()) {
      for (int i = 0; i < ui->listFeatures->count(); ++i) {
        if (ui->listFeatures->item(i)->data(Qt::UserRole).toString() ==
            selection) {
          selectedIndex = i;
          break;
        }
      }
    }
    ui->listFeatures->setCurrentRow(selectedIndex);
  }
  bool enable = ui->listFeatures->count() > 0;
  enableWidgets(enable);
}

void TabFeatures::enableWidgets(bool enable) {
  ui->txtFeatureName->setEnabled(enable);
  ui->cmbCompartment->setEnabled(enable);
  ui->cmbSpecies->setEnabled(enable);
  ui->cmbRoiType->setEnabled(enable);
  ui->stackRoiSettings->setEnabled(enable);
  ui->cmbReduction->setEnabled(enable);
  ui->btnRemoveFeature->setEnabled(enable);
  ui->lblRegionColors->setEnabled(enable);
  if (enable) {
    updateRoiWidgetEnableState();
  } else {
    ui->txtExpression->setEnabled(false);
    ui->btnEditExpression->setEnabled(false);
    ui->spnNRegions->setEnabled(false);
    ui->lblRegionColors->setNumberOfRegions(0);
    ui->btnImportImage->setEnabled(false);
    ui->lblBuiltInThicknessLabel->setEnabled(false);
    ui->spnBuiltInThickness->setEnabled(false);
    ui->lblBuiltInAxisLabel->setEnabled(false);
    ui->cmbBuiltInAxis->setEnabled(false);
  }
}

void TabFeatures::updateRoiWidgetEnableState() {
  const auto roiType = currentRoiType(ui->cmbRoiType);
  const bool analytic = roiType == sme::simulate::RoiType::Analytic;
  const bool image = roiType == sme::simulate::RoiType::Image;
  const bool depth = roiType == sme::simulate::RoiType::Depth;
  const bool axis = roiType == sme::simulate::RoiType::AxisSlices;
  if (analytic) {
    ui->stackRoiSettings->setCurrentWidget(ui->pageRoiAnalytic);
  } else if (image) {
    ui->stackRoiSettings->setCurrentWidget(ui->pageRoiImage);
  } else if (depth) {
    ui->stackRoiSettings->setCurrentWidget(ui->pageRoiDepth);
  } else if (axis) {
    ui->stackRoiSettings->setCurrentWidget(ui->pageRoiAxis);
  }
  ui->txtExpression->setEnabled(analytic);
  ui->btnEditExpression->setEnabled(analytic);
  ui->spnNRegions->setEnabled(true);
  ui->btnImportImage->setEnabled(image);
  ui->lblBuiltInThicknessLabel->setEnabled(depth);
  ui->spnBuiltInThickness->setEnabled(depth);
  ui->lblBuiltInAxisLabel->setEnabled(axis);
  ui->cmbBuiltInAxis->setEnabled(axis);
  updateRegionColors();
}

void TabFeatures::updateRegionColors() {
  if (!ui->lblRegionColors->isEnabled() || currentFeatureIndex < 0) {
    ui->lblRegionColors->setNumberOfRegions(0);
    return;
  }

  ui->lblRegionColors->setNumberOfRegions(
      static_cast<std::size_t>(std::max(1, ui->spnNRegions->value())));
}

void TabFeatures::updateRoiImage() {
  if (lblGeometry == nullptr || currentFeatureIndex < 0) {
    return;
  }
  const auto &features = model.getFeatures().getFeatures();
  if (static_cast<std::size_t>(currentFeatureIndex) >= features.size()) {
    return;
  }
  const auto &origin = model.getGeometry().getPhysicalOrigin();
  if (!model.getFeatures().isValid(
          static_cast<std::size_t>(currentFeatureIndex))) {
    lblGeometry->setImage(model.getGeometry().getImages());
    if (voxGeometry != nullptr) {
      voxGeometry->setImage(model.getGeometry().getImages(), origin);
    }
    return;
  }
  const auto &regions = model.getFeatures().getVoxelRegions(
      static_cast<std::size_t>(currentFeatureIndex));
  const auto &feat = features[static_cast<std::size_t>(currentFeatureIndex)];
  auto nRegions = sme::simulate::getNumRegions(feat.roi);
  const auto *comp = model.getCompartments().getCompartment(
      QString::fromStdString(feat.compartmentId));
  if (comp == nullptr || regions.empty()) {
    lblGeometry->setImage(model.getGeometry().getImages());
    if (voxGeometry != nullptr) {
      voxGeometry->setImage(model.getGeometry().getImages(), origin);
    }
    return;
  }
  const auto &vol = model.getGeometry().getImages().volume();
  sme::common::ImageStack imageStack(
      vol, sme::common::ImageStack::indexedImageFormat);
  imageStack.fill(0);
  // color table: index 0 = black background, indices 1..nRegions use the
  // shared indexed plotting palette.
  imageStack.setColor(0, qRgb(0, 0, 0));
  sme::common::indexedColors indexedColors;
  for (std::size_t region = 1; region <= nRegions; ++region) {
    imageStack.setColor(static_cast<int>(region),
                        indexedColors[region - 1].rgb());
  }
  const auto &voxels = comp->getVoxels();
  for (std::size_t i = 0; i < voxels.size() && i < regions.size(); ++i) {
    const auto &v = voxels[i];
    if (static_cast<std::size_t>(v.z) < vol.depth()) {
      imageStack[static_cast<std::size_t>(v.z)].setPixel(
          v.p.x(), v.p.y(), static_cast<uint>(regions[i]));
    }
  }
  lblGeometry->setImage(imageStack);
  if (voxGeometry != nullptr) {
    voxGeometry->setImage(imageStack, origin);
  }
}

void TabFeatures::listFeatures_currentRowChanged(int row) {
  currentFeatureIndex = -1;
  ui->txtFeatureName->clear();
  ui->cmbCompartment->clear();
  ui->cmbSpecies->clear();
  enableWidgets(false);
  const auto &features = model.getFeatures().getFeatures();
  if (row < 0 || static_cast<std::size_t>(row) >= features.size()) {
    if (lblGeometry != nullptr) {
      lblGeometry->setImage(model.getGeometry().getImages());
    }
    if (voxGeometry != nullptr) {
      voxGeometry->setImage(model.getGeometry().getImages(),
                            model.getGeometry().getPhysicalOrigin());
    }
    return;
  }
  currentFeatureIndex = row;
  const auto &feat = features[static_cast<std::size_t>(row)];
  SPDLOG_DEBUG("Feature {} selected", feat.name);
  ui->txtFeatureName->setText(QString::fromStdString(feat.name));
  // populate compartment combo
  ui->cmbCompartment->blockSignals(true);
  const auto &compIds = model.getCompartments().getIds();
  const auto &compNames = model.getCompartments().getNames();
  for (int i = 0; i < compIds.size(); ++i) {
    ui->cmbCompartment->addItem(compNames[i], compIds[i]);
  }
  int compIdx = compIds.indexOf(QString::fromStdString(feat.compartmentId));
  if (compIdx >= 0) {
    ui->cmbCompartment->setCurrentIndex(compIdx);
  }
  ui->cmbCompartment->blockSignals(false);
  // populate species combo for the selected compartment
  ui->cmbSpecies->blockSignals(true);
  QString compId = QString::fromStdString(feat.compartmentId);
  auto specIds = model.getSpecies().getIds(compId);
  auto specNames = model.getSpecies().getNames(compId);
  for (int i = 0; i < specIds.size(); ++i) {
    ui->cmbSpecies->addItem(specNames[i], specIds[i]);
  }
  int specIdx = specIds.indexOf(QString::fromStdString(feat.speciesId));
  if (specIdx >= 0) {
    ui->cmbSpecies->setCurrentIndex(specIdx);
  }
  ui->cmbSpecies->blockSignals(false);
  ui->cmbRoiType->blockSignals(true);
  ui->cmbRoiType->setCurrentIndex(
      ui->cmbRoiType->findData(static_cast<int>(feat.roi.roiType)));
  ui->cmbRoiType->blockSignals(false);
  ui->txtExpression->setText(QString::fromStdString(feat.roi.expression));
  ui->spnNRegions->setValue(static_cast<int>(feat.roi.numRegions));
  ui->spnBuiltInThickness->blockSignals(true);
  ui->spnBuiltInThickness->setValue(sme::simulate::getRoiParameterInt(
      feat.roi, sme::simulate::roi_param::depthThicknessVoxels, 1));
  ui->spnBuiltInThickness->blockSignals(false);
  ui->cmbBuiltInAxis->blockSignals(true);
  auto axisIndex =
      ui->cmbBuiltInAxis->findData(sme::simulate::getRoiParameterInt(
          feat.roi, sme::simulate::roi_param::axis, 0));
  ui->cmbBuiltInAxis->setCurrentIndex(std::max(axisIndex, 0));
  ui->cmbBuiltInAxis->blockSignals(false);
  ui->cmbReduction->blockSignals(true);
  ui->cmbReduction->setCurrentIndex(static_cast<int>(feat.reduction));
  ui->cmbReduction->blockSignals(false);
  enableWidgets(true);
  updateRoiImage();
}

void TabFeatures::btnAddFeature_clicked() {
  bool ok{false};
  auto name = QInputDialog::getText(
      this, "Add feature", "New feature name:", QLineEdit::Normal, {}, &ok);
  if (ok && !name.isEmpty()) {
    const auto &compIds = model.getCompartments().getIds();
    std::string compartmentId;
    std::string speciesId;
    if (!compIds.isEmpty()) {
      compartmentId = compIds[0].toStdString();
      auto specIds = model.getSpecies().getIds(compIds[0]);
      if (!specIds.isEmpty()) {
        speciesId = specIds[0].toStdString();
      }
    }
    sme::simulate::RoiSettings roi;
    roi.roiType = sme::simulate::RoiType::Analytic;
    roi.expression = "1";
    auto featureIndex =
        model.getFeatures().add(name.toStdString(), compartmentId, speciesId,
                                roi, sme::simulate::ReductionOp::Average);
    loadModelData(QString::fromStdString(
        model.getFeatures().getFeatures()[featureIndex].id));
  }
}

void TabFeatures::btnRemoveFeature_clicked() {
  if (currentFeatureIndex < 0) {
    return;
  }
  auto result{
      QMessageBox::question(this, "Remove feature?",
                            QString("Remove feature '%1' from the model?")
                                .arg(ui->listFeatures->currentItem()->text()),
                            QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    SPDLOG_INFO("Removing feature {}", currentFeatureIndex);
    model.getFeatures().remove(static_cast<std::size_t>(currentFeatureIndex));
    loadModelData();
  }
}

void TabFeatures::txtFeatureName_editingFinished() {
  if (currentFeatureIndex < 0) {
    return;
  }
  const auto &name = ui->txtFeatureName->text();
  const auto &features = model.getFeatures().getFeatures();
  if (static_cast<std::size_t>(currentFeatureIndex) >= features.size() ||
      name.toStdString() ==
          features[static_cast<std::size_t>(currentFeatureIndex)].name) {
    return;
  }
  const auto featureId =
      features[static_cast<std::size_t>(currentFeatureIndex)].id;
  auto newName = QString::fromStdString(model.getFeatures().setName(
      static_cast<std::size_t>(currentFeatureIndex), name.toStdString()));
  ui->txtFeatureName->setText(newName);
  loadModelData(QString::fromStdString(featureId));
}

void TabFeatures::cmbCompartment_currentIndexChanged(int index) {
  if (currentFeatureIndex < 0 || index < 0) {
    return;
  }
  auto compId = ui->cmbCompartment->itemData(index).toString();
  // update species list for new compartment
  ui->cmbSpecies->blockSignals(true);
  ui->cmbSpecies->clear();
  auto specIds = model.getSpecies().getIds(compId);
  auto specNames = model.getSpecies().getNames(compId);
  for (int i = 0; i < specIds.size(); ++i) {
    ui->cmbSpecies->addItem(specNames[i], specIds[i]);
  }
  ui->cmbSpecies->blockSignals(false);
  QString specId;
  if (!specIds.isEmpty()) {
    specId = specIds[0];
    ui->cmbSpecies->setCurrentIndex(0);
  }
  model.getFeatures().setCompartmentAndSpecies(
      static_cast<std::size_t>(currentFeatureIndex), compId.toStdString(),
      specId.toStdString());
  updateRoiImage();
}

void TabFeatures::cmbSpecies_currentIndexChanged(int index) {
  if (currentFeatureIndex < 0 || index < 0) {
    return;
  }
  auto compId = ui->cmbCompartment->currentData().toString();
  auto specId = ui->cmbSpecies->itemData(index).toString();
  model.getFeatures().setCompartmentAndSpecies(
      static_cast<std::size_t>(currentFeatureIndex), compId.toStdString(),
      specId.toStdString());
  updateRoiImage();
}

void TabFeatures::cmbRoiType_currentIndexChanged(int index) {
  updateRoiWidgetEnableState();
  if (currentFeatureIndex < 0 || index < 0) {
    return;
  }
  const auto &features = model.getFeatures().getFeatures();
  auto roi = features[static_cast<std::size_t>(currentFeatureIndex)].roi;
  roi.roiType = currentRoiType(ui->cmbRoiType);
  if (roi.roiType == sme::simulate::RoiType::Depth) {
    sme::simulate::setRoiParameterInt(
        roi, sme::simulate::roi_param::depthThicknessVoxels,
        ui->spnBuiltInThickness->value());
  } else if (roi.roiType == sme::simulate::RoiType::AxisSlices) {
    sme::simulate::setRoiParameterInt(roi, sme::simulate::roi_param::axis,
                                      currentAxis(ui->cmbBuiltInAxis));
  }
  model.getFeatures().setRoi(static_cast<std::size_t>(currentFeatureIndex),
                             roi);
  updateRoiImage();
}

void TabFeatures::btnEditExpression_clicked() {
  if (currentFeatureIndex < 0) {
    return;
  }
  const auto &features = model.getFeatures().getFeatures();
  auto roi = features[static_cast<std::size_t>(currentFeatureIndex)].roi;
  const auto *comp =
      model.getCompartments().getCompartment(QString::fromStdString(
          features[static_cast<std::size_t>(currentFeatureIndex)]
              .compartmentId));
  if (comp == nullptr) {
    return;
  }
  sme::model::SpeciesGeometry compartmentGeometry{
      comp->getImageSize(), comp->getVoxels(),
      model.getGeometry().getPhysicalOrigin(),
      model.getGeometry().getVoxelSize(), model.getUnits()};
  DialogAnalytic dialog(
      QString::fromStdString(roi.expression), DialogAnalyticDataType::RoiRegion,
      compartmentGeometry, model.getParameters(), model.getFunctions(),
      model.getDisplayOptions().invertYAxis, roi.numRegions, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  roi.expression = dialog.getExpression();
  model.getFeatures().setRoi(static_cast<std::size_t>(currentFeatureIndex),
                             roi);
  ui->txtExpression->setText(QString::fromStdString(roi.expression));
  updateRoiImage();
}

void TabFeatures::spnNRegions_valueChanged(int value) {
  if (currentFeatureIndex < 0) {
    return;
  }
  const auto &features = model.getFeatures().getFeatures();
  auto roi = features[static_cast<std::size_t>(currentFeatureIndex)].roi;
  roi.numRegions = static_cast<std::size_t>(value);
  model.getFeatures().setRoi(static_cast<std::size_t>(currentFeatureIndex),
                             roi);
  updateRegionColors();
  updateRoiImage();
}

void TabFeatures::btnImportImage_clicked() {
  if (currentFeatureIndex < 0) {
    return;
  }
  QString filename = QFileDialog::getOpenFileName(
      this, "Import ROI mask image", "",
      "Image files (*.png *.bmp *.jpg *.tiff);;All files (*.*)");
  if (filename.isEmpty()) {
    return;
  }
  QImage img(filename);
  if (img.isNull()) {
    return;
  }
  const auto &features = model.getFeatures().getFeatures();
  auto roi = features[static_cast<std::size_t>(currentFeatureIndex)].roi;
  roi.regionImage.clear();
  roi.regionImage.reserve(static_cast<std::size_t>(img.width() * img.height()));
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      roi.regionImage.push_back(
          static_cast<std::size_t>(qGray(img.pixel(x, y))));
    }
  }
  roi.roiType = sme::simulate::RoiType::Image;
  model.getFeatures().setRoi(static_cast<std::size_t>(currentFeatureIndex),
                             roi);
  ui->cmbRoiType->blockSignals(true);
  ui->cmbRoiType->setCurrentIndex(
      ui->cmbRoiType->findData(static_cast<int>(roi.roiType)));
  ui->cmbRoiType->blockSignals(false);
  updateRoiWidgetEnableState();
  updateRoiImage();
}

void TabFeatures::spnBuiltInThickness_valueChanged(int value) {
  if (currentFeatureIndex < 0) {
    return;
  }
  const auto &features = model.getFeatures().getFeatures();
  auto roi = features[static_cast<std::size_t>(currentFeatureIndex)].roi;
  if (roi.roiType != sme::simulate::RoiType::Depth) {
    return;
  }
  sme::simulate::setRoiParameterInt(
      roi, sme::simulate::roi_param::depthThicknessVoxels, value);
  model.getFeatures().setRoi(static_cast<std::size_t>(currentFeatureIndex),
                             roi);
  updateRoiImage();
}

void TabFeatures::cmbBuiltInAxis_currentIndexChanged(int index) {
  if (currentFeatureIndex < 0 || index < 0) {
    return;
  }
  const auto &features = model.getFeatures().getFeatures();
  auto roi = features[static_cast<std::size_t>(currentFeatureIndex)].roi;
  if (roi.roiType != sme::simulate::RoiType::AxisSlices) {
    return;
  }
  sme::simulate::setRoiParameterInt(roi, sme::simulate::roi_param::axis,
                                    currentAxis(ui->cmbBuiltInAxis));
  model.getFeatures().setRoi(static_cast<std::size_t>(currentFeatureIndex),
                             roi);
  updateRoiImage();
}

void TabFeatures::cmbReduction_currentIndexChanged(int index) {
  if (currentFeatureIndex < 0 || index < 0) {
    return;
  }
  model.getFeatures().setReduction(
      static_cast<std::size_t>(currentFeatureIndex),
      static_cast<sme::simulate::ReductionOp>(index));
}
