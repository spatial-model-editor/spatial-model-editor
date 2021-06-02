#include "dialoganalytic.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "model_units.hpp"
#include "ui_dialoganalytic.h"
#include <QFileDialog>
#include <QPushButton>
#include "model_geometry.hpp"
#include "model_parameters.hpp"
#include "model_functions.hpp"

DialogAnalytic::DialogAnalytic(
    const QString &analyticExpression,
    const sme::model::SpeciesGeometry &speciesGeometry,
    const sme::model::ModelParameters &modelParameters,
    const sme::model::ModelFunctions &modelFunctions, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogAnalytic>()},
      points(speciesGeometry.compartmentPoints),
      width(speciesGeometry.pixelWidth), origin(speciesGeometry.physicalOrigin),
      qpi(speciesGeometry.compartmentImageSize,
          speciesGeometry.compartmentPoints) {
  ui->setupUi(this);

  const auto &units = speciesGeometry.modelUnits;
  lengthUnit = units.getLength().name;
  concentrationUnit =
      QString("%1/%2").arg(units.getAmount().name).arg(units.getVolume().name);
  img = QImage(speciesGeometry.compartmentImageSize,
               QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  concentration.resize(points.size(), 0.0);
  // add x,y variables
  const auto &spatialCoordinates{modelParameters.getSpatialCoordinates()};
  ui->txtExpression->addVariable(spatialCoordinates.x.id,
                                 spatialCoordinates.x.name);
  ui->txtExpression->addVariable(spatialCoordinates.y.id,
                                 spatialCoordinates.y.name);
  for (const auto &function : modelFunctions.getSymbolicFunctions()) {
    ui->txtExpression->addFunction(function);
  }
  // todo: add non-constant parameters somewhere?
  ui->txtExpression->setConstants(modelParameters.getGlobalConstants());
  QSizeF physicalSize;
  physicalSize.rwidth() =
      static_cast<double>(speciesGeometry.compartmentImageSize.width()) *
      speciesGeometry.pixelWidth;
  physicalSize.rheight() =
      static_cast<double>(speciesGeometry.compartmentImageSize.height()) *
      speciesGeometry.pixelWidth;
  ui->lblImage->displayGrid(ui->chkGrid->isChecked());
  ui->lblImage->displayScale(ui->chkScale->isChecked());
  ui->lblImage->setPhysicalSize(physicalSize, lengthUnit);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogAnalytic::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogAnalytic::reject);
  connect(ui->chkGrid, &QCheckBox::stateChanged, this,
          [ui = ui.get()](int state) {
            ui->lblImage->displayGrid(state == Qt::Checked);
          });
  connect(ui->chkScale, &QCheckBox::stateChanged, this,
          [ui = ui.get()](int state) {
            ui->lblImage->displayScale(state == Qt::Checked);
          });
  connect(ui->txtExpression, &QPlainTextMathEdit::mathChanged, this,
          &DialogAnalytic::txtExpression_mathChanged);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogAnalytic::lblImage_mouseOver);
  connect(ui->btnExportImage, &QPushButton::clicked, this,
          &DialogAnalytic::btnExportImage_clicked);
  ui->txtExpression->importVariableMath(analyticExpression.toStdString());
  lblImage_mouseOver(points.front());
  ui->txtExpression->setFocus();
}

DialogAnalytic::~DialogAnalytic() = default;

const std::string &DialogAnalytic::getExpression() const {
  return variableExpression;
}

const QImage &DialogAnalytic::getImage() const { return img; }

bool DialogAnalytic::isExpressionValid() const { return expressionIsValid; }

QPointF DialogAnalytic::physicalPoint(const QPoint &pixelPoint) const {
  // position in pixels (with (0,0) in top-left of image)
  // rescale to physical x,y point (with (0,0) in bottom-left)
  QPointF physical;
  physical.setX(origin.x() + width * static_cast<double>(pixelPoint.x()));
  physical.setY(origin.x() +
                width * static_cast<double>(img.height() - 1 - pixelPoint.y()));
  return physical;
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
    img.fill(0);
    ui->lblImage->setImage(img);
    return;
  }
  // calculate concentration
  ui->txtExpression->compileMath();
  std::vector<double> vars{0, 0};
  for (std::size_t i = 0; i < points.size(); ++i) {
    auto physical = physicalPoint(points[i]);
    vars[0] = physical.x();
    vars[1] = physical.y();
    concentration[i] = ui->txtExpression->evaluateMath(vars);
  }
  if (std::find_if(concentration.cbegin(), concentration.cend(), [](auto c) {
        return std::isnan(c);
      }) != concentration.cend()) {
    // if concentration contains NaN, show error message
    ui->lblExpressionStatus->setText(
        "concentration contains NaN (Not a Number)");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->btnExportImage->setEnabled(false);
    img.fill(0);
    ui->lblImage->setImage(img);
    return;
  }
  if (*std::min_element(concentration.cbegin(), concentration.cend()) < 0) {
    // if concentration contains negative values, show error message
    ui->lblExpressionStatus->setText("concentration cannot be negative");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->btnExportImage->setEnabled(false);
    img.fill(0);
    ui->lblImage->setImage(img);
    return;
  }
  ui->lblExpressionStatus->setText("");
  expressionIsValid = true;
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  ui->btnExportImage->setEnabled(true);
  displayExpression = math.toStdString();
  variableExpression = ui->txtExpression->getVariableMath();
  // normalise displayed pixel intensity to max concentration
  double maxConc =
      *std::max_element(concentration.cbegin(), concentration.cend());
  for (std::size_t i = 0; i < points.size(); ++i) {
    int intensity = static_cast<int>(255 * concentration[i] / maxConc);
    img.setPixel(points[i], QColor(intensity, intensity, intensity).rgb());
  }
  ui->lblImage->setImage(img);
}

void DialogAnalytic::lblImage_mouseOver(QPoint point) {
  if (!expressionIsValid) {
    return;
  }
  auto index = qpi.getIndex(point);
  if (!index) {
    ui->lblConcentration->setText("");
    return;
  }
  auto physical = physicalPoint(point);
  ui->lblConcentration->setText(QString("x=%1 %2, y=%3 %2, concentration=%4 %5")
                                    .arg(physical.x())
                                    .arg(lengthUnit)
                                    .arg(physical.y())
                                    .arg(concentration[*index])
                                    .arg(concentrationUnit));
}

void DialogAnalytic::btnExportImage_clicked() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Export species concentration as image", "conc.png", "PNG (*.png)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".png") {
    filename.append(".png");
  }
  SPDLOG_DEBUG("exporting concentration image to file {}",
               filename.toStdString());
  img.save(filename);
}
