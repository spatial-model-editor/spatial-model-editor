#include "dialogconcentrationimage.hpp"
#include "guiutils.hpp"
#include "sme/logger.hpp"
#include "sme/model_units.hpp"
#include "ui_dialogconcentrationimage.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolTip>

DialogConcentrationImage::DialogConcentrationImage(
    const std::vector<double> &concentrationArray,
    const sme::model::SpeciesGeometry &speciesGeometry, bool invertYAxis,
    const QString &windowTitle, bool isRateOfChange, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogConcentrationImage>()},
      points(speciesGeometry.compartmentPoints),
      width(speciesGeometry.pixelWidth), origin(speciesGeometry.physicalOrigin),
      qpi(speciesGeometry.compartmentImageSize,
          speciesGeometry.compartmentPoints) {
  ui->setupUi(this);
  setWindowTitle(windowTitle);

  colourMinConc = QImage(32, 32, QImage::Format_ARGB32_Premultiplied);
  colourMaxConc = colourMinConc;
  colourMinConc.fill(QColor(0, 0, 0));
  colourMaxConc.fill(QColor(255, 255, 255));
  ui->lblMinConcColour->setPixmap(QPixmap::fromImage(colourMinConc));
  ui->lblMaxConcColour->setPixmap(QPixmap::fromImage(colourMaxConc));

  const auto &units = speciesGeometry.modelUnits;
  lengthUnit = units.getLength().name;
  quantityName = "concentration";
  if (isRateOfChange) {
    quantityName = "concentration rate of change";
  }
  quantityUnit =
      QString("%1/%2").arg(units.getAmount().name).arg(units.getVolume().name);
  if (isRateOfChange) {
    quantityUnit = QString("%1/%2").arg(quantityUnit).arg(units.getTime().name);
  }
  ui->lblMinConcUnits->setText(quantityUnit);
  ui->lblMaxConcUnits->setText(quantityUnit);
  img = QImage(speciesGeometry.compartmentImageSize,
               QImage::Format_ARGB32_Premultiplied);
  img.fill(0);

  ui->lblImage->displayGrid(ui->chkGrid->isChecked());
  ui->lblImage->displayScale(ui->chkScale->isChecked());
  ui->lblImage->invertYAxis(invertYAxis);
  QSizeF physicalSize;
  physicalSize.rwidth() =
      static_cast<double>(speciesGeometry.compartmentImageSize.width()) *
      speciesGeometry.pixelWidth;
  physicalSize.rheight() =
      static_cast<double>(speciesGeometry.compartmentImageSize.height()) *
      speciesGeometry.pixelWidth;
  ui->lblImage->setPhysicalSize(physicalSize, lengthUnit);

  if (concentrationArray.empty()) {
    SPDLOG_DEBUG("empty initial concentrationArray - "
                 "setting concentration to zero everywhere");
    importConcentrationArray(std::vector<double>(
        static_cast<std::size_t>(img.width() * img.height()), 0.0));
  } else {
    importConcentrationArray(concentrationArray);
  }

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogConcentrationImage::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogConcentrationImage::reject);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogConcentrationImage::lblImage_mouseOver);
  connect(ui->chkGrid, &QCheckBox::stateChanged, this,
          &DialogConcentrationImage::chkGrid_stateChanged);
  connect(ui->chkScale, &QCheckBox::stateChanged, this,
          &DialogConcentrationImage::chkScale_stateChanged);
  connect(ui->btnImportImage, &QPushButton::clicked, this,
          &DialogConcentrationImage::btnImportImage_clicked);
  connect(ui->btnExportImage, &QPushButton::clicked, this,
          &DialogConcentrationImage::btnExportImage_clicked);
  connect(ui->cmbExampleImages, &QComboBox::currentTextChanged, this,
          &DialogConcentrationImage::cmbExampleImages_currentTextChanged);
  connect(ui->btnSmoothImage, &QPushButton::clicked, this,
          &DialogConcentrationImage::smoothConcentration);
  connect(ui->txtMinConc, &QLineEdit::editingFinished, this, [this]() {
    rescaleConcentration(ui->txtMinConc->text().toDouble(),
                         ui->txtMaxConc->text().toDouble());
  });
  connect(ui->txtMaxConc, &QLineEdit::editingFinished, this, [this]() {
    rescaleConcentration(ui->txtMinConc->text().toDouble(),
                         ui->txtMaxConc->text().toDouble());
  });

  lblImage_mouseOver(points.front());
}

DialogConcentrationImage::~DialogConcentrationImage() = default;

std::vector<double> DialogConcentrationImage::getConcentrationArray() const {
  int size = img.size().width() * img.size().height();
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: where (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  std::vector<double> arr(static_cast<std::size_t>(size), 0.0);
  for (std::size_t i = 0; i < concentration.size(); ++i) {
    const auto &point = points[i];
    arr[pointToConcentrationArrayIndex(point)] = concentration[i];
  }
  return arr;
}

QPointF
DialogConcentrationImage::physicalPoint(const QPoint &pixelPoint) const {
  // position in pixels (with (0,0) in top-left of image)
  // rescale to physical x,y point (with (0,0) in bottom-left)
  QPointF physical;
  physical.setX(origin.x() + width * static_cast<double>(pixelPoint.x()));
  physical.setY(origin.x() +
                width * static_cast<double>(img.height() - 1 - pixelPoint.y()));
  return physical;
}

std::size_t DialogConcentrationImage::pointToConcentrationArrayIndex(
    const QPoint &point) const {
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  return static_cast<std::size_t>(point.x() +
                                  img.width() * (img.height() - 1 - point.y()));
}

void DialogConcentrationImage::importConcentrationArray(
    const std::vector<double> &concentrationArray) {
  int compImagePixels = img.width() * img.height();
  if (static_cast<int>(concentrationArray.size()) != compImagePixels) {
    SPDLOG_ERROR("mismatch between array size {} and compartment pixels {}",
                 concentrationArray.size(), compImagePixels);
    throw std::invalid_argument("invalid concentration array size");
  }
  // get concentration at each point in compartment
  concentration.clear();
  concentration.reserve(points.size());
  std::transform(points.cbegin(), points.cend(),
                 std::back_inserter(concentration),
                 [this, &concentrationArray](const auto &p) {
                   return concentrationArray[pointToConcentrationArrayIndex(p)];
                 });
  updateImageFromConcentration();
}

void DialogConcentrationImage::importConcentrationImage(
    const QImage &concentrationImage) {
  QImage concImg = concentrationImage;
  if (concImg.size() != img.size()) {
    SPDLOG_WARN(
        "mismatch between concentration image size {}x{},"
        "and compartment image size {}x{}: rescaling concentration image",
        concImg.width(), concImg.height(), img.width(), img.height());
    concImg = concImg.scaled(img.size(), Qt::IgnoreAspectRatio,
                             Qt::SmoothTransformation);
  }
  concentration.clear();
  concentration.reserve(points.size());
  std::transform(points.cbegin(), points.cend(),
                 std::back_inserter(concentration), [&concImg](const auto &p) {
                   return static_cast<double>(concImg.pixel(p));
                 });
  rescaleConcentration(0, 1);
  updateImageFromConcentration();
}

static QString toQStr(double x) { return QString::number(x, 'g', 14); }

static std::pair<double, double> getMinMax(const std::vector<double> &vec) {
  auto [iterMin, iterMax] = std::minmax_element(vec.cbegin(), vec.cend());
  double min = *iterMin;
  double max = *iterMax;
  if (min == max) {
    // check for case where concentration the same at every point
    max += 1;
  }
  return {min, max};
}

void DialogConcentrationImage::updateImageFromConcentration() {
  // update min/max concentrations
  const auto [newMin, newMax] = getMinMax(concentration);
  SPDLOG_DEBUG("min {}, max {}", newMin, newMax);
  ui->txtMinConc->setText(toQStr(newMin));
  ui->txtMaxConc->setText(toQStr(newMax));
  // normalise intensity to max concentration
  img.fill(0);
  for (std::size_t i = 0; i < points.size(); ++i) {
    int intensity =
        static_cast<int>(255 * (concentration[i] - newMin) / (newMax - newMin));
    img.setPixel(points[i], QColor(intensity, intensity, intensity).rgb());
  }
  ui->lblImage->setImage(img);
}

void DialogConcentrationImage::rescaleConcentration(double newMin,
                                                    double newMax) {
  SPDLOG_INFO("linearly rescaling concentration");
  if (newMin < 0) {
    SPDLOG_WARN("Min concentration cannot be smaller than zero: "
                "setting it to zero");
    newMin = 0;
  }
  ui->txtMinConc->setText(toQStr(newMin));
  if (newMax <= newMin) {
    SPDLOG_WARN("Max concentration cannot be smaller than Min concentration: "
                "setting it equal to Min concentration + 1");
    newMax = newMin + 1;
  }
  ui->txtMaxConc->setText(toQStr(newMax));
  SPDLOG_INFO("  - new range [{}, {}]", newMin, newMax);
  double oldMin;
  double oldMax;
  std::tie(oldMin, oldMax) = getMinMax(concentration);
  SPDLOG_INFO("  - old range [{}, {}]", oldMin, oldMax);
  std::transform(
      concentration.begin(), concentration.end(), concentration.begin(),
      [&newMin, &newMax, &oldMin, &oldMax](double c) {
        return newMin + (newMax - newMin) * (c - oldMin) / (oldMax - oldMin);
      });
}

void DialogConcentrationImage::gaussianFilter(const QPoint &direction,
                                              double sigma) {
  // 1-d gaussian filter
  constexpr double norm = 0.3989422804;
  constexpr double epsilon = 1e-4;
  QVector<double> gaussianWeights(1, norm / sigma);
  while (gaussianWeights.back() / gaussianWeights.front() > epsilon) {
    auto x = static_cast<double>(gaussianWeights.size());
    gaussianWeights.push_back(norm * std::exp(-x * x / (2 * sigma * sigma)));
  }
  SPDLOG_TRACE("using {} +/- neighbours for 1-d filter with sigma={}",
               gaussianWeights.size() - 1, sigma);
  auto arr = getConcentrationArray();
  std::size_t concIndex = 0;
  for (const auto &p : points) {
    concentration[concIndex] *= gaussianWeights[0];
    auto i = pointToConcentrationArrayIndex(p);
    auto iUp = i;
    auto iDn = i;
    for (int iGauss = 1; iGauss < gaussianWeights.size(); ++iGauss) {
      auto iNeighbour = qpi.getIndex(p + iGauss * direction);
      if (iNeighbour) {
        iUp = pointToConcentrationArrayIndex(points[*iNeighbour]);
      }
      concentration[concIndex] += gaussianWeights[iGauss] * arr[iUp];
      iNeighbour = qpi.getIndex(p - iGauss * direction);
      if (iNeighbour) {
        iDn = pointToConcentrationArrayIndex(points[*iNeighbour]);
      }
      concentration[concIndex] += gaussianWeights[iGauss] * arr[iDn];
    }
    ++concIndex;
  }
}

void DialogConcentrationImage::smoothConcentration() {
  double sigma = 0.5;
  gaussianFilter(QPoint(1, 0), sigma);
  gaussianFilter(QPoint(0, 1), sigma);
  updateImageFromConcentration();
}

void DialogConcentrationImage::lblImage_mouseOver(QPoint point) {
  auto index = qpi.getIndex(point);
  if (!index) {
    ui->lblConcentration->setText("");
    return;
  }
  auto physical = physicalPoint(point);
  ui->lblConcentration->setText(QString("x=%1 %2\ny=%3 %2\n%4:\n%5 %6")
                                    .arg(physical.x())
                                    .arg(lengthUnit)
                                    .arg(physical.y())
                                    .arg(quantityName)
                                    .arg(concentration[*index])
                                    .arg(quantityUnit));
}

void DialogConcentrationImage::chkGrid_stateChanged(int state) {
  ui->lblImage->displayGrid(state == Qt::Checked);
}
void DialogConcentrationImage::chkScale_stateChanged(int state) {
  ui->lblImage->displayScale(state == Qt::Checked);
}

void DialogConcentrationImage::btnImportImage_clicked() {
  auto concImg =
      getImageFromUser(this, "Import species concentration from image");
  if (!concImg.isNull()) {
    importConcentrationImage(concImg);
  }
}

void DialogConcentrationImage::btnExportImage_clicked() {
  QString filename = QFileDialog::getSaveFileName(
      this, QString("Export species %1 as image").arg(quantityName), "conc.png",
      "PNG (*.png)");
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

void DialogConcentrationImage::cmbExampleImages_currentTextChanged(
    const QString &text) {
  if (ui->cmbExampleImages->currentIndex() != 0) {
    SPDLOG_DEBUG("import {}", text.toStdString());
    QImage concImg(QString(":/concentration/%1.png").arg(text));
    importConcentrationImage(concImg);
    ui->cmbExampleImages->setCurrentIndex(0);
  }
}
