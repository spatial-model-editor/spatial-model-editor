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
      voxelSize(speciesGeometry.voxelSize),
      voxels(speciesGeometry.compartmentVoxels),
      physicalOrigin(speciesGeometry.physicalOrigin),
      imgs{speciesGeometry.compartmentImageSize,
           QImage::Format_ARGB32_Premultiplied},
      qpi(speciesGeometry.compartmentImageSize,
          speciesGeometry.compartmentVoxels) {
  ui->setupUi(this);
  ui->lblImage->setZSlider(ui->slideZIndex);
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
  imgs.fill(0);

  ui->lblImage->displayGrid(ui->chkGrid->isChecked());
  ui->lblImage->displayScale(ui->chkScale->isChecked());
  ui->lblImage->invertYAxis(invertYAxis);
  sme::common::VolumeF physicalSize{speciesGeometry.compartmentImageSize *
                                    speciesGeometry.voxelSize};
  ui->lblImage->setPhysicalSize(physicalSize, lengthUnit);

  if (concentrationArray.empty()) {
    SPDLOG_DEBUG("empty initial concentrationArray - "
                 "setting concentration to zero everywhere");
    importConcentrationArray(std::vector<double>(
        speciesGeometry.compartmentImageSize.nVoxels(), 0.0));
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
  lblImage_mouseOver(voxels.front());
}

DialogConcentrationImage::~DialogConcentrationImage() = default;

std::vector<double> DialogConcentrationImage::getConcentrationArray() const {
  std::vector<double> arr(imgs.volume().nVoxels(), 0.0);
  for (std::size_t i = 0; i < concentration.size(); ++i) {
    arr[pointToConcentrationArrayIndex(voxels[i])] = concentration[i];
  }
  return arr;
}

sme::common::VoxelF
DialogConcentrationImage::physicalPoint(const sme::common::Voxel &voxel) const {
  // position in pixels (with (0,0) in top-left of image)
  // rescale to physical x,y point (with (0,0) in bottom-left)
  return {physicalOrigin.p.x() +
              voxelSize.width() * static_cast<double>(voxel.p.x()),
          physicalOrigin.p.y() +
              voxelSize.height() *
                  static_cast<double>(imgs.volume().height() - 1 - voxel.p.y()),
          physicalOrigin.z + voxelSize.depth() * static_cast<double>(voxel.z)};
}

std::size_t DialogConcentrationImage::pointToConcentrationArrayIndex(
    const sme::common::Voxel &voxel) const {
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  return static_cast<std::size_t>(
      voxel.p.x() +
      imgs.volume().width() * (imgs.volume().height() - 1 - voxel.p.y()) +
      imgs.volume().width() * imgs.volume().height() * voxel.z);
}

void DialogConcentrationImage::importConcentrationArray(
    const std::vector<double> &concentrationArray) {
  if (concentrationArray.size() != imgs.volume().nVoxels()) {
    SPDLOG_ERROR("mismatch between array size {} and compartment pixels {}",
                 concentrationArray.size(), imgs.volume().nVoxels());
    throw std::invalid_argument("invalid concentration array size");
  }
  // get concentration at each point in compartment
  concentration.clear();
  concentration.reserve(voxels.size());
  std::transform(
      voxels.cbegin(), voxels.cend(), std::back_inserter(concentration),
      [this, &concentrationArray](const auto &voxel) {
        return concentrationArray[pointToConcentrationArrayIndex(voxel)];
      });
  updateImageFromConcentration();
}

void DialogConcentrationImage::importConcentrationImage(
    const sme::common::ImageStack &concentrationImage) {
  // todo: need to handle incorrectly sized z-stacks properly here
  if (concentrationImage.volume().depth() != imgs.volume().depth()) {
    SPDLOG_WARN("Incorrect image depth {}, should be {} - ignoring",
                concentrationImage.volume().depth(), imgs.volume().height());
    return;
  }
  auto *concImage{&concentrationImage};
  sme::common::ImageStack rescaledConcentrationImage{};
  // rescale incorrectly sized images in x-y directions
  if (concentrationImage.volume() != imgs.volume()) {
    SPDLOG_WARN(
        "mismatch between concentration image volume {}x{},"
        "and compartment image volume {}x{}: rescaling concentration image",
        concentrationImage.volume().width(),
        concentrationImage.volume().height(), imgs.volume().width(),
        imgs.volume().height());
    rescaledConcentrationImage = concentrationImage;
    rescaledConcentrationImage.rescaleXY(
        {imgs.volume().width(), imgs.volume().height()});
    concImage = &rescaledConcentrationImage;
  }
  concentration.clear();
  concentration.reserve(voxels.size());
  std::transform(
      voxels.cbegin(), voxels.cend(), std::back_inserter(concentration),
      [concImage](const auto &voxel) {
        return static_cast<double>((*concImage)[voxel.z].pixel(voxel.p));
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
  imgs.fill(0);
  for (std::size_t i = 0; i < voxels.size(); ++i) {
    const auto &voxel{voxels[i]};
    int intensity =
        static_cast<int>(255 * (concentration[i] - newMin) / (newMax - newMin));
    imgs[voxel.z].setPixel(voxel.p,
                           QColor(intensity, intensity, intensity).rgb());
  }
  ui->lblImage->setImage(imgs);
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

void DialogConcentrationImage::gaussianFilter(
    const sme::common::Voxel &direction, double sigma) {
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
  for (const auto &voxel : voxels) {
    concentration[concIndex] *= gaussianWeights[0];
    auto i = pointToConcentrationArrayIndex(voxel);
    auto iUp = i;
    auto iDn = i;
    for (int iGauss = 1; iGauss < gaussianWeights.size(); ++iGauss) {
      auto iNeighbour = qpi.getIndex(voxel + iGauss * direction);
      if (iNeighbour) {
        iUp = pointToConcentrationArrayIndex(voxels[*iNeighbour]);
      }
      concentration[concIndex] += gaussianWeights[iGauss] * arr[iUp];
      iNeighbour = qpi.getIndex(voxel - iGauss * direction);
      if (iNeighbour) {
        iDn = pointToConcentrationArrayIndex(voxels[*iNeighbour]);
      }
      concentration[concIndex] += gaussianWeights[iGauss] * arr[iDn];
    }
    ++concIndex;
  }
}

void DialogConcentrationImage::smoothConcentration() {
  double sigma = 0.5;
  gaussianFilter({1, 0, 0}, sigma);
  gaussianFilter({0, 1, 0}, sigma);
  gaussianFilter({0, 0, 1}, sigma);
  updateImageFromConcentration();
}

void DialogConcentrationImage::lblImage_mouseOver(
    const sme::common::Voxel &voxel) {
  auto index = qpi.getIndex(voxel);
  if (!index) {
    ui->lblConcentration->setText("");
    return;
  }
  auto physical = physicalPoint(voxel);
  ui->lblConcentration->setText(QString("x=%1 %2\ny=%3 %2\nz=%4 %2\n%5:\n%6 %7")
                                    .arg(physical.p.x())
                                    .arg(lengthUnit)
                                    .arg(physical.p.y())
                                    .arg(physical.z)
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
  if (!concImg.empty()) {
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
  // todo: export 3d tiff stack instead of first slice?
  imgs[0].save(filename);
}

void DialogConcentrationImage::cmbExampleImages_currentTextChanged(
    const QString &text) {
  if (ui->cmbExampleImages->currentIndex() != 0) {
    SPDLOG_DEBUG("import {}", text.toStdString());
    QImage concImg(QString(":/concentration/%1.png").arg(text));
    importConcentrationImage(sme::common::ImageStack({concImg}));
    ui->cmbExampleImages->setCurrentIndex(0);
  }
}
