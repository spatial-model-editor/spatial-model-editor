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
    const QString &analyticExpression, DialogAnalyticDataType dataType,
    const sme::model::SpeciesGeometry &speciesGeometry,
    const sme::model::ModelParameters &modelParameters,
    const sme::model::ModelFunctions &modelFunctions, bool invertYAxis,
    QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogAnalytic>()},
      voxelSize(speciesGeometry.voxelSize),
      voxels(speciesGeometry.compartmentVoxels),
      physicalOrigin(speciesGeometry.physicalOrigin),
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
  }
  imgs.setVoxelSize(speciesGeometry.voxelSize);
  imgs.fill(0);
  values.resize(voxels.size(), 0.0);
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
  // position in pixels (with (0,0) in top-left of image)
  // rescale to physical x,y point (with (0,0) in bottom-left)
  return {physicalOrigin.p.x() +
              voxelSize.width() * static_cast<double>(voxel.p.x()),
          physicalOrigin.p.y() +
              voxelSize.height() *
                  static_cast<double>(imgs.volume().height() - 1 - voxel.p.y()),
          physicalOrigin.z + voxelSize.depth() * static_cast<double>(voxel.z)};
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
    // if concentration contains NaN or inf, show error message
    ui->lblExpressionStatus->setText(
        QString("%1 contains inf (infinity) or NaN (Not a Number)")
            .arg(valueLabel));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->btnExportImage->setEnabled(false);
    imgs.fill(0);
    ui->lblImage->setImage(imgs);
    return;
  }
  if (*std::ranges::min_element(std::as_const(values)) < 0) {
    // if concentration contains negative values, show error message
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
  // normalise displayed pixel intensity to max concentration
  double maxConc{sme::common::max(values)};
  for (std::size_t i = 0; i < voxels.size(); ++i) {
    int intensity = static_cast<int>(255 * values[i] / maxConc);
    imgs[voxels[i].z].setPixel(voxels[i].p,
                               qRgb(intensity, intensity, intensity));
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

void DialogAnalytic::btnExportImage_clicked() {
  QString filename = QFileDialog::getSaveFileName(
      this, QString("Export %1 slice as image").arg(valueLabel),
      QString("%1.png").arg(valueLabel), "PNG (*.png)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".png") {
    filename.append(".png");
  }
  SPDLOG_DEBUG("exporting image slice to file {}", filename.toStdString());
  imgs[0].save(filename);
}
