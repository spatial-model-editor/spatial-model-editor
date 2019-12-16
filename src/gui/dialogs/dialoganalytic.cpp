#include "dialoganalytic.hpp"

#include <QPushButton>

#include "logger.hpp"
#include "ui_dialoganalytic.h"
#include "units.hpp"

DialogAnalytic::DialogAnalytic(const QString& analyticExpression,
                               const sbml::SpeciesGeometry& speciesGeometry,
                               QWidget* parent)
    : QDialog(parent),
      ui{std::make_unique<Ui::DialogAnalytic>()},
      points(speciesGeometry.compartmentPoints),
      width(speciesGeometry.pixelWidth),
      origin(speciesGeometry.physicalOrigin),
      qpi(speciesGeometry.compartmentImageSize,
          speciesGeometry.compartmentPoints) {
  ui->setupUi(this);

  const auto& units = speciesGeometry.modelUnits;
  lengthUnit = units.getLength().symbol;
  concentrationUnit = QString("%1/%2")
                          .arg(units.getAmount().symbol)
                          .arg(units.getVolume().symbol);
  img = QImage(speciesGeometry.compartmentImageSize,
               QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  concentration.resize(points.size(), 0.0);
  // add some built-in functions, as well as x,y variables
  ui->txtExpression->setVariables(
      {"x", "y", "sin", "cos", "exp", "log", "ln", "pow", "sqrt"});

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogAnalytic::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogAnalytic::reject);
  connect(ui->txtExpression, &QPlainTextMathEdit::mathChanged, this,
          &DialogAnalytic::txtExpression_mathChanged);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogAnalytic::lblImage_mouseOver);

  ui->txtExpression->setPlainText(analyticExpression);
  lblImage_mouseOver(points.front());
}

DialogAnalytic::~DialogAnalytic() = default;

const std::string& DialogAnalytic::getExpression() const { return expression; }

bool DialogAnalytic::isExpressionValid() const { return expressionIsValid; }

QPointF DialogAnalytic::physicalPoint(const QPoint& pixelPoint) const {
  // position in pixels (with (0,0) in top-left of image)
  // rescale to physical x,y point (with (0,0) in bottom-left)
  QPointF physical;
  physical.setX(origin.x() + width * static_cast<double>(pixelPoint.x()));
  physical.setY(origin.x() +
                width * static_cast<double>(img.height() - 1 - pixelPoint.y()));
  return physical;
}

void DialogAnalytic::txtExpression_mathChanged(const QString& math, bool valid,
                                               const QString& errorMessage) {
  SPDLOG_DEBUG("math {}", math.toStdString());
  SPDLOG_DEBUG("  - is valid: {}", valid);
  SPDLOG_DEBUG("  - error: {}", errorMessage.toStdString());
  expressionIsValid = false;
  ui->lblConcentration->setText("");
  expression.clear();
  if (!valid) {
    // if expression not valid, show error message
    ui->lblExpressionStatus->setText(errorMessage);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    img.fill(0);
    ui->lblImage->setImage(img);
    return;
  }
  // calculate concentration
  ui->txtExpression->compileMath();
  auto vars = std::vector<double>(2, 0);
  for (std::size_t i = 0; i < points.size(); ++i) {
    auto physical = physicalPoint(points[i]);
    vars[0] = physical.x();
    vars[1] = physical.y();
    concentration[i] = ui->txtExpression->evaluateMath(vars);
  }
  if (*std::min_element(concentration.cbegin(), concentration.cend()) < 0) {
    // if concentration contains negative values, show error message
    ui->lblExpressionStatus->setText("concentration cannot be negative");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    img.fill(0);
    ui->lblImage->setImage(img);
    return;
  }
  ui->lblExpressionStatus->setText("");
  expressionIsValid = true;
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  expression = math.toStdString();
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
