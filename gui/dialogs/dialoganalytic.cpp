#include "dialoganalytic.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/model_functions.hpp"
#include "sme/model_geometry.hpp"
#include "sme/model_parameters.hpp"
#include "sme/model_units.hpp"
#include "ui_dialoganalytic.h"
#include <QFileDialog>
#include <QPushButton>

DialogAnalytic::DialogAnalytic(
    const QString &analyticExpression, DialogAnalyticDataType analyticDataType,
    const sme::model::SpeciesGeometry &speciesGeometry,
    const sme::model::ModelParameters &modelParameters,
    const sme::model::ModelFunctions &modelFunctions, bool invertYAxis,
    std::size_t maxNumRegions, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogAnalytic>()},
      voxelSize(speciesGeometry.voxelSize),
      voxels(speciesGeometry.compartmentVoxels),
      physicalOrigin(speciesGeometry.physicalOrigin),
      dataType(analyticDataType), numRegions(maxNumRegions),
      imgs{speciesGeometry.compartmentImageSize,
           QImage::Format_ARGB32_Premultiplied},
      qpi(speciesGeometry.compartmentImageSize,
          speciesGeometry.compartmentVoxels) {
  ui->setupUi(this);

  const auto &units = speciesGeometry.modelUnits;
  lengthUnit = units.getLength().name;
  if (dataType == DialogAnalyticDataType::Concentration) {
    setWindowTitle("Concentration");
    valueLabel = "concentration";
    valueUnit = units.getConcentration();
  } else if (dataType == DialogAnalyticDataType::DiffusionConstant) {
    setWindowTitle("Diffusion constant");
    valueLabel = "diffusion constant";
    valueUnit = units.getDiffusion();
  } else if (dataType == DialogAnalyticDataType::RoiRegion) {
    setWindowTitle("ROI regions");
    valueLabel = "ROI region";
    ui->lblImage->setWhatsThis(
        "<h4>ROI regions</h4>"
        "<p>A preview of the ROI regions resulting from the supplied analytic "
        "expression.</p>");
  }
  imgs.setVoxelSize(speciesGeometry.voxelSize);
  imgs.fill(0);
  values.resize(voxels.size(), 0.0);
  roiRegions.resize(voxels.size(), 0);
  // add x,y,z variables
  const auto &spatialCoordinates{modelParameters.getSpatialCoordinates()};
  ui->txtExpression->addVariable(spatialCoordinates.x.id,
                                 spatialCoordinates.x.name);
  ui->txtExpression->addVariable(spatialCoordinates.y.id,
                                 spatialCoordinates.y.name);
  ui->txtExpression->addVariable(spatialCoordinates.z.id,
                                 spatialCoordinates.z.name);
  for (const auto &function : modelFunctions.getSymbolicFunctions()) {
    ui->txtExpression->addFunction(function);
  }
  // todo: add non-constant parameters somewhere?
  ui->txtExpression->setConstants(modelParameters.getGlobalConstants());
  ui->lblImage->displayGrid(ui->chkGrid->isChecked());
  ui->lblImage->displayScale(ui->chkScale->isChecked());
  ui->lblImage->setPhysicalUnits(lengthUnit);
  ui->lblImage->setPhysicalOrigin(physicalOrigin);
  ui->lblImage->invertYAxis(invertYAxis);
  ui->lblImage->setZSlider(ui->slideZIndex);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogAnalytic::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogAnalytic::reject);
  connect(ui->chkGrid, &QCheckBox::checkStateChanged, this,
          [ui_ptr = ui.get()](int state) {
            ui_ptr->lblImage->displayGrid(state == Qt::Checked);
          });
  connect(ui->chkScale, &QCheckBox::checkStateChanged, this,
          [ui_ptr = ui.get()](int state) {
            ui_ptr->lblImage->displayScale(state == Qt::Checked);
          });
  connect(ui->txtExpression, &QPlainTextMathEdit::mathChanged, this,
          &DialogAnalytic::txtExpression_mathChanged);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogAnalytic::lblImage_mouseOver);
  connect(ui->btnExportImage, &QPushButton::clicked, this,
          &DialogAnalytic::btnExportImage_clicked);
  ui->txtExpression->importVariableMath(analyticExpression.toStdString());
  lblImage_mouseOver(voxels.front());
  ui->txtExpression->setFocus();
}

DialogAnalytic::~DialogAnalytic() = default;

const std::string &DialogAnalytic::getExpression() const {
  return variableExpression;
}

const QImage &DialogAnalytic::getImage() const { return imgs[0]; }

bool DialogAnalytic::isExpressionValid() const { return expressionIsValid; }

sme::common::VoxelF
DialogAnalytic::physicalPoint(const sme::common::Voxel &voxel) const {
  // position of voxel centre in physical coordinates
  return {physicalOrigin.p.x() +
              voxelSize.width() * (static_cast<double>(voxel.p.x()) + 0.5),
          physicalOrigin.p.y() +
              voxelSize.height() * (static_cast<double>(imgs.volume().height() -
                                                        1 - voxel.p.y()) +
                                    0.5),
          physicalOrigin.z +
              voxelSize.depth() * (static_cast<double>(voxel.z) + 0.5)};
}

void DialogAnalytic::txtExpression_mathChanged(const QString &math, bool valid,
                                               const QString &errorMessage) {
  SPDLOG_DEBUG("math {}", math.toStdString());
  SPDLOG_DEBUG("  - is valid: {}", valid);
  SPDLOG_DEBUG("  - error: {}", errorMessage.toStdString());
  expressionIsValid = false;
  ui->lblConcentration->setText("");
  displayExpression.clear();
  variableExpression.clear();
  if (!valid) {
    // if expression not valid, show error message
    ui->lblExpressionStatus->setText(errorMessage);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->btnExportImage->setEnabled(false);
    imgs.fill(0);
    ui->lblImage->setImage(imgs);
    return;
  }
  // compile expression
  if (!ui->txtExpression->compileMath()) {
    // if compile fails, show error message
    ui->lblExpressionStatus->setText(ui->txtExpression->getErrorMessage());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->btnExportImage->setEnabled(false);
    imgs.fill(0);
    ui->lblImage->setImage(imgs);
    return;
  }
  // calculate concentration
  std::vector<double> vars{0, 0, 0};
  for (std::size_t i = 0; i < voxels.size(); ++i) {
    auto physical = physicalPoint(voxels[i]);
    vars[0] = physical.p.x();
    vars[1] = physical.p.y();
    vars[2] = physical.z;
    values[i] = ui->txtExpression->evaluateMath(vars);
  }
  if (std::ranges::find_if(std::as_const(values), [](auto c) {
        return std::isnan(c) || std::isinf(c);
      }) != values.cend()) {
    // if the expression contains NaN or inf, show error message
    ui->lblExpressionStatus->setText(
        QString("%1 contains inf (infinity) or NaN (Not a Number)")
            .arg(valueLabel));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->btnExportImage->setEnabled(false);
    imgs.fill(0);
    ui->lblImage->setImage(imgs);
    return;
  }
  if ((dataType == DialogAnalyticDataType::Concentration ||
       dataType == DialogAnalyticDataType::DiffusionConstant) &&
      *std::ranges::min_element(std::as_const(values)) < 0) {
    // concentration-like quantities cannot contain negative values
    ui->lblExpressionStatus->setText(
        QString("%1 cannot be negative").arg(valueLabel));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->btnExportImage->setEnabled(false);
    imgs.fill(0);
    ui->lblImage->setImage(imgs);
    return;
  }
  ui->lblExpressionStatus->setText("");
  expressionIsValid = true;
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  ui->btnExportImage->setEnabled(true);
  displayExpression = math.toStdString();
  variableExpression = ui->txtExpression->getVariableMath();
  if (dataType == DialogAnalyticDataType::RoiRegion) {
    imgs.fill(0);
    sme::common::indexedColors indexedColors;
    for (std::size_t i = 0; i < voxels.size(); ++i) {
      const auto region = static_cast<int>(values[i]);
      if (region > 0 && static_cast<std::size_t>(region) <= numRegions) {
        roiRegions[i] = static_cast<std::size_t>(region);
        imgs[voxels[i].z].setPixel(voxels[i].p,
                                   indexedColors[roiRegions[i] - 1].rgb());
      } else {
        roiRegions[i] = 0;
        imgs[voxels[i].z].setPixel(voxels[i].p, qRgb(0, 0, 0));
      }
    }
  } else {
    // normalise displayed pixel intensity to max scalar value
    const double maxValue{sme::common::max(values)};
    for (std::size_t i = 0; i < voxels.size(); ++i) {
      int intensity = 0;
      if (maxValue > 0) {
        intensity = static_cast<int>(255 * values[i] / maxValue);
      }
      imgs[voxels[i].z].setPixel(voxels[i].p,
                                 qRgb(intensity, intensity, intensity));
    }
  }
  ui->lblImage->setImage(imgs);
}

void DialogAnalytic::lblImage_mouseOver(const sme::common::Voxel &voxel) {
  if (!expressionIsValid) {
    return;
  }
  auto index = qpi.getIndex(voxel);
  if (!index) {
    ui->lblConcentration->setText("");
    return;
  }
  auto physical = physicalPoint(voxel);
  if (dataType == DialogAnalyticDataType::RoiRegion) {
    ui->lblConcentration->setText(
        QString(
            "x: %1 %4, y: %2 %4, z: %3 %4, expression value: %5, region: %6")
            .arg(physical.p.x())
            .arg(physical.p.y())
            .arg(physical.z)
            .arg(lengthUnit)
            .arg(values[*index])
            .arg(roiRegions[*index]));
  } else {
    ui->lblConcentration->setText(
        QString("x: %1 %4, y: %2 %4, z: %3 %4, %5: %6 %7")
            .arg(physical.p.x())
            .arg(physical.p.y())
            .arg(physical.z)
            .arg(lengthUnit)
            .arg(valueLabel)
            .arg(values[*index])
            .arg(valueUnit));
  }
}

void DialogAnalytic::btnExportImage_clicked() {
  QString exportLabel{valueLabel};
  if (dataType == DialogAnalyticDataType::RoiRegion) {
    exportLabel = "roi-regions";
  }
  QString filename = QFileDialog::getSaveFileName(
      this, QString("Export %1 slice as image").arg(exportLabel),
      QString("%1.png").arg(exportLabel), "PNG (*.png)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".png") {
    filename.append(".png");
  }
  SPDLOG_DEBUG("exporting image slice to file {}", filename.toStdString());
  imgs[0].save(filename);
}
