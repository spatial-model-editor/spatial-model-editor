#include "dialogoptcost.hpp"
#include "dialogimagedata.hpp"
#include "sme/utils.hpp"
#include "ui_dialogoptcost.h"
#include <QMessageBox>

static QString toQStr(double x) { return QString::number(x, 'g', 14); }

static QString targetValuesText(sme::simulate::OptCostType optCostType,
                                bool wrap = false) {
  if (optCostType == sme::simulate::OptCostType::ConcentrationDcdt) {
    return wrap ? "Rate of change\nof concentration"
                : "Rate of change of concentration";
  }
  return "Concentration";
}

static int toIndex(sme::simulate::OptCostDiffType optCostDiffType) {
  switch (optCostDiffType) {
  case sme::simulate::OptCostDiffType::Absolute:
    return 0;
  case sme::simulate::OptCostDiffType::Relative:
    return 1;
  default:
    return 0;
  }
}

static sme::simulate::OptCostDiffType toOptCostDiffTypeEnum(int index) {
  switch (index) {
  case 0:
    return sme::simulate::OptCostDiffType::Absolute;
  case 1:
    return sme::simulate::OptCostDiffType::Relative;
  default:
    return sme::simulate::OptCostDiffType::Absolute;
  }
}

DialogOptCost::DialogOptCost(
    const sme::model::Model &model,
    const std::vector<sme::simulate::OptCost> &defaultOptCosts,
    const sme::simulate::OptCost *initialOptCost, QWidget *parent)
    : QDialog(parent), m_model{model}, m_defaultOptCosts{defaultOptCosts},
      ui{std::make_unique<Ui::DialogOptCost>()} {
  ui->setupUi(this);
  ui->lblFeature->setVisible(false);
  ui->lblImageFeature->setVisible(false);
  adjustSize();
  int cmbIndex{0};
  if (defaultOptCosts.empty()) {
    // model has no species: disable everything except cancel button
    ui->cmbSpecies->setEnabled(false);
    ui->cmbDiffType->setEnabled(false);
    ui->cmbCostType->setEnabled(false);
    ui->txtSimulationTime->setEnabled(false);
    ui->btnEditTargetValues->setEnabled(false);
    ui->txtWeight->setEnabled(false);
    ui->txtEpsilon->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  } else {
    if (initialOptCost != nullptr) {
      m_optCost = *initialOptCost;
    } else {
      m_optCost = defaultOptCosts.front();
    }
    for (const auto &defaultOptCost : defaultOptCosts) {
      ui->cmbSpecies->addItem(defaultOptCost.name.c_str());
      if (defaultOptCost.id == m_optCost.id) {
        cmbIndex = ui->cmbSpecies->count() - 1;
      }
    }
    connect(ui->cmbSpecies, &QComboBox::currentIndexChanged, this,
            &DialogOptCost::cmbSpecies_currentIndexChanged);
    connect(ui->cmbCostType, &QComboBox::currentIndexChanged, this,
            &DialogOptCost::cmbCostType_currentIndexChanged);
    connect(ui->cmbDiffType, &QComboBox::currentIndexChanged, this,
            &DialogOptCost::cmbDiffType_currentIndexChanged);
    connect(ui->txtSimulationTime, &QLineEdit::editingFinished, this,
            &DialogOptCost::txtSimulationTime_editingFinished);
    connect(ui->btnEditTargetValues, &QPushButton::clicked, this,
            &DialogOptCost::btnEditTargetValues_clicked);
    connect(ui->txtWeight, &QLineEdit::editingFinished, this,
            &DialogOptCost::txtWeight_editingFinished);
    connect(ui->txtEpsilon, &QLineEdit::editingFinished, this,
            &DialogOptCost::txtEpsilon_editingFinished);
    ui->cmbDiffType->setCurrentIndex(toIndex(m_optCost.optCostDiffType));
    ui->txtSimulationTime->setText(QString::number(m_optCost.simulationTime));
    ui->txtWeight->setText(QString::number(m_optCost.weight));
    ui->txtEpsilon->setText(QString::number(m_optCost.epsilon));
    ui->txtEpsilon->setEnabled(m_optCost.optCostDiffType ==
                               sme::simulate::OptCostDiffType::Relative);
    ui->cmbSpecies->setCurrentIndex(cmbIndex);
    cmbSpecies_currentIndexChanged(cmbIndex);
  }
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogOptCost::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogOptCost::reject);
}

DialogOptCost::~DialogOptCost() = default;

[[nodiscard]] const sme::simulate::OptCost &DialogOptCost::getOptCost() const {
  return m_optCost;
}

void DialogOptCost::populateTargetTypes() {
  m_targetTypeItems.clear();
  ui->cmbCostType->blockSignals(true);
  ui->cmbCostType->clear();
  m_targetTypeItems.push_back(
      {sme::simulate::OptCostType::Concentration, {}, "Concentration"});
  m_targetTypeItems.push_back({sme::simulate::OptCostType::ConcentrationDcdt,
                               {},
                               "Rate of change of concentration"});
  const auto featureIndex =
      m_model.getFeatures().getIndexFromId(m_optCost.featureId);
  const auto &features = m_model.getFeatures().getFeatures();
  for (const auto &feature : features) {
    if (feature.speciesId != m_optCost.id) {
      continue;
    }
    m_targetTypeItems.push_back(
        {sme::simulate::OptCostType::Feature, feature.id,
         QString("Feature: %1").arg(QString::fromStdString(feature.name))});
  }
  if (m_optCost.optCostType == sme::simulate::OptCostType::Feature &&
      (m_optCost.featureId.empty() || featureIndex >= features.size())) {
    m_targetTypeItems.push_back(
        {sme::simulate::OptCostType::Feature, m_optCost.featureId,
         QString("Feature: <missing> (%1)")
             .arg(QString::fromStdString(m_optCost.featureId))});
  }
  int selectedIndex{0};
  for (int i = 0; i < static_cast<int>(m_targetTypeItems.size()); ++i) {
    const auto &item = m_targetTypeItems[static_cast<std::size_t>(i)];
    ui->cmbCostType->addItem(item.label);
    const bool sameType = item.optCostType == m_optCost.optCostType;
    const bool sameFeature =
        item.optCostType != sme::simulate::OptCostType::Feature ||
        item.featureId == m_optCost.featureId;
    if (sameType && sameFeature) {
      selectedIndex = i;
    }
  }
  ui->cmbCostType->setCurrentIndex(selectedIndex);
  ui->cmbCostType->blockSignals(false);
  updateTargetValuesLabel();
}

void DialogOptCost::cmbSpecies_currentIndexChanged(int index) {
  auto i{static_cast<std::size_t>(index)};
  if (i >= m_defaultOptCosts.size()) {
    return;
  }
  if (m_defaultOptCosts[i].id != m_optCost.id) {
    m_optCost = m_defaultOptCosts[i];
    ui->cmbDiffType->setCurrentIndex(toIndex(m_optCost.optCostDiffType));
    ui->txtSimulationTime->setText(QString::number(m_optCost.simulationTime));
    ui->txtWeight->setText(QString::number(m_optCost.weight));
    ui->txtEpsilon->setText(QString::number(m_optCost.epsilon));
    ui->txtEpsilon->setEnabled(m_optCost.optCostDiffType ==
                               sme::simulate::OptCostDiffType::Relative);
  }
  populateTargetTypes();
  updateImage();
  updateImageFeature();
}

void DialogOptCost::cmbCostType_currentIndexChanged(int index) {
  if (index < 0 ||
      static_cast<std::size_t>(index) >= m_targetTypeItems.size()) {
    return;
  }
  const auto &item = m_targetTypeItems[static_cast<std::size_t>(index)];
  m_optCost.optCostType = item.optCostType;
  if (m_optCost.optCostType == sme::simulate::OptCostType::Feature) {
    m_optCost.featureId = item.featureId;

    // show feature image
    ui->lblFeature->setVisible(true);
    ui->lblImageFeature->setVisible(true);
    updateImageFeature();

  } else {
    m_optCost.featureId.clear();
  }
  updateTargetValuesLabel();
}

void DialogOptCost::cmbDiffType_currentIndexChanged(int index) {
  m_optCost.optCostDiffType = toOptCostDiffTypeEnum(index);
  ui->txtEpsilon->setEnabled(m_optCost.optCostDiffType ==
                             sme::simulate::OptCostDiffType::Relative);
}

void DialogOptCost::txtSimulationTime_editingFinished() {
  m_optCost.simulationTime = ui->txtSimulationTime->text().toDouble();
  ui->txtSimulationTime->setText(toQStr(m_optCost.simulationTime));
}

void DialogOptCost::btnEditTargetValues_clicked() {
  DialogImageDataDataType dataType{DialogImageDataDataType::Concentration};
  if (m_optCost.optCostType == sme::simulate::OptCostType::ConcentrationDcdt) {
    dataType = DialogImageDataDataType::ConcentrationRateOfChange;
  }

  try {
    DialogImageData dialog(m_optCost.targetValues,
                           m_model.getSpeciesGeometry(m_optCost.id.c_str()),
                           m_model.getDisplayOptions().invertYAxis, dataType);
    if (dialog.exec() == QDialog::Accepted) {
      m_optCost.targetValues = dialog.getImageArray();
      updateImage();
    }
  } catch (const std::invalid_argument &e) {
    QMessageBox::warning(
        this,
        QString("Error editing %1 image")
            .arg(targetValuesText(m_optCost.optCostType).toLower()),
        e.what());
  }
}

void DialogOptCost::txtWeight_editingFinished() {
  m_optCost.weight = ui->txtWeight->text().toDouble();
  ui->txtWeight->setText(toQStr(m_optCost.weight));
}

void DialogOptCost::txtEpsilon_editingFinished() {
  m_optCost.epsilon = ui->txtEpsilon->text().toDouble();
  ui->txtEpsilon->setText(toQStr(m_optCost.epsilon));
}

void DialogOptCost::updateTargetValuesLabel() {
  const auto text{targetValuesText(m_optCost.optCostType)};
  const auto isFeatureTarget{m_optCost.optCostType ==
                             sme::simulate::OptCostType::Feature};
  //   ui->lblTargetValuesLabel->setText(
  //       QString("%1:").arg(targetValuesText(m_optCost.optCostType, true)));
  ui->lblTargetDisplayed->setText(
      QString("%1:").arg(targetValuesText(m_optCost.optCostType, true)));

  if (not isFeatureTarget) {
    ui->lblFeature->setVisible(false);
    ui->lblImageFeature->setVisible(false);
    this->adjustSize();
  } else {
    ui->lblFeature->setVisible(true);
    ui->lblImageFeature->setVisible(true);
    ui->btnEditTargetValues->setText(QString("Edit %1").arg(text));
  }
}

void DialogOptCost::updateImage() {
  const auto &imageSize{
      m_model.getSpeciesGeometry(m_optCost.id.c_str()).compartmentImageSize};
  ui->lblImage->setImage(
      sme::common::ImageStack(imageSize, m_optCost.targetValues));
}

void DialogOptCost::updateImageFeature() {
  const auto &features = m_model.getFeatures().getFeatures();
  const auto featureIndex =
      m_model.getFeatures().getIndexFromId(m_optCost.featureId);
  if (featureIndex >= features.size() ||
      !m_model.getFeatures().isValid(featureIndex)) {
    ui->lblImageFeature->setImage({});
    return;
  }

  const auto &feature = features[featureIndex];
  const auto &regions = m_model.getFeatures().getVoxelRegions(featureIndex);
  const auto *comp = m_model.getCompartments().getCompartment(
      QString::fromStdString(feature.compartmentId));
  if (comp == nullptr || regions.empty()) {
    ui->lblImageFeature->setImage({});
    return;
  }

  // Feature regions are stored per compartment voxel. Build a full
  // geometry-sized indexed image so the preview lines up with the target image.
  const auto &vol = m_model.getGeometry().getImages().volume();
  sme::common::ImageStack imageStack(
      vol, sme::common::ImageStack::indexedImageFormat);
  imageStack.fill(0);
  // Palette index 0 is the black background; regions 1..N use the same
  // indexed colors as the feature editor.
  imageStack.setColor(0, qRgb(0, 0, 0));
  sme::common::indexedColors indexedColors;
  const auto nRegions = sme::simulate::getNumRegions(feature.roi);
  for (std::size_t region = 1; region <= nRegions; ++region) {
    imageStack.setColor(static_cast<int>(region),
                        indexedColors[region - 1].rgb());
  }

  // Map each compartment-local region assignment back onto its geometry voxel.
  const auto &voxels = comp->getVoxels();
  for (std::size_t i = 0; i < voxels.size() && i < regions.size(); ++i) {
    const auto &voxel = voxels[i];
    if (static_cast<std::size_t>(voxel.z) < vol.depth()) {
      imageStack[static_cast<std::size_t>(voxel.z)].setPixel(
          voxel.p.x(), voxel.p.y(), static_cast<uint>(regions[i]));
    }
  }
  ui->lblImageFeature->setImage(imageStack);
}
