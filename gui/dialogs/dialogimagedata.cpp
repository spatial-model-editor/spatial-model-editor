#include "dialogimagedata.hpp"
#include "guiutils.hpp"
#include "sme/logger.hpp"
#include "sme/model_units.hpp"
#include "ui_dialogimagedata.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolTip>

DialogImageData::DialogImageData(
    const std::vector<double> &dataArray,
    const sme::model::SpeciesGeometry &speciesGeometry, bool invertYAxis,
    DialogImageDataDataType dataType, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImageData>()},
      voxelSize(speciesGeometry.voxelSize),
      voxels(speciesGeometry.compartmentVoxels),
      physicalOrigin(speciesGeometry.physicalOrigin),
      imgs{speciesGeometry.compartmentImageSize,
           QImage::Format_ARGB32_Premultiplied},
      qpi(speciesGeometry.compartmentImageSize,
          speciesGeometry.compartmentVoxels) {
  ui->setupUi(this);
  ui->lblImage->setZSlider(ui->slideZIndex);
  const auto &units = speciesGeometry.modelUnits;
  lengthUnit = units.getLength().name;
  if (dataType == DialogImageDataDataType::Concentration) {
    setWindowTitle("Concentration image data");
    quantityName = "concentration";
    quantityUnit = units.getConcentration();
  } else if (dataType == DialogImageDataDataType::ConcentrationRateOfChange) {
    setWindowTitle("Concentration rate of change image data");
    quantityName = "concentration rate of change";
    quantityUnit = QString("%1/%2")
                       .arg(units.getConcentration())
                       .arg(units.getTime().name);
  } else if (dataType == DialogImageDataDataType::DiffusionConstant) {
    setWindowTitle("Diffusion constant image data");
    quantityName = "diffusion constant";
    quantityUnit = units.getDiffusion();
  }

  colorMinConc = QImage(32, 32, QImage::Format_ARGB32_Premultiplied);
  colorMaxConc = colorMinConc;
  colorMinConc.fill(QColor(0, 0, 0));
  colorMaxConc.fill(QColor(255, 255, 255));
  ui->lblMinConcColor->setPixmap(QPixmap::fromImage(colorMinConc));
  ui->lblMaxConcColor->setPixmap(QPixmap::fromImage(colorMaxConc));

  ui->lblMinConcUnits->setText(quantityUnit);
  ui->lblMaxConcUnits->setText(quantityUnit);
  imgs.setVoxelSize(speciesGeometry.voxelSize);
  imgs.fill(0);

  ui->lblImage->displayGrid(ui->chkGrid->isChecked());
  ui->lblImage->displayScale(ui->chkScale->isChecked());
  ui->lblImage->invertYAxis(invertYAxis);
  ui->lblImage->setPhysicalUnits(lengthUnit);
  if (dataArray.empty()) {
    SPDLOG_DEBUG("empty initial array - "
                 "setting concentration to zero everywhere");
    importArray(std::vector<double>(
        speciesGeometry.compartmentImageSize.nVoxels(), 0.0));
  } else {
    importArray(dataArray);
  }

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImageData::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImageData::reject);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogImageData::lblImage_mouseOver);
  connect(ui->chkGrid, &QCheckBox::checkStateChanged, this,
          &DialogImageData::chkGrid_stateChanged);
  connect(ui->chkScale, &QCheckBox::checkStateChanged, this,
          &DialogImageData::chkScale_stateChanged);
  connect(ui->btnImportImage, &QPushButton::clicked, this,
          &DialogImageData::btnImportImage_clicked);
  connect(ui->btnExportImage, &QPushButton::clicked, this,
          &DialogImageData::btnExportImage_clicked);
  connect(ui->cmbExampleImages, &QComboBox::currentTextChanged, this,
          &DialogImageData::cmbExampleImages_currentTextChanged);
  connect(ui->btnSmoothImage, &QPushButton::clicked, this,
          &DialogImageData::smoothData);
  connect(ui->txtMinConc, &QLineEdit::editingFinished, this, [this]() {
    rescaleData(ui->txtMinConc->text().toDouble(),
                ui->txtMaxConc->text().toDouble());
  });
  connect(ui->txtMaxConc, &QLineEdit::editingFinished, this, [this]() {
    rescaleData(ui->txtMinConc->text().toDouble(),
                ui->txtMaxConc->text().toDouble());
  });
  lblImage_mouseOver(voxels.front());
}

DialogImageData::~DialogImageData() = default;

const std::vector<double> &DialogImageData::getData() const { return data; }

std::vector<double> DialogImageData::getImageArray() const {
  std::vector<double> arr(imgs.volume().nVoxels(), 0.0);
  for (std::size_t i = 0; i < data.size(); ++i) {
    arr[pointToArrayIndex(voxels[i])] = data[i];
  }
  return arr;
}

sme::common::VoxelF
DialogImageData::physicalPoint(const sme::common::Voxel &voxel) const {
  // position in pixels (with (0,0) in top-left of image)
  // rescale to physical x,y point (with (0,0) in bottom-left)
  return {physicalOrigin.p.x() +
              voxelSize.width() * static_cast<double>(voxel.p.x()),
          physicalOrigin.p.y() +
              voxelSize.height() *
                  static_cast<double>(imgs.volume().height() - 1 - voxel.p.y()),
          physicalOrigin.z + voxelSize.depth() * static_cast<double>(voxel.z)};
}

std::size_t
DialogImageData::pointToArrayIndex(const sme::common::Voxel &voxel) const {
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left
  return sme::common::voxelArrayIndex(imgs.volume(), voxel, true);
}

void DialogImageData::importArray(
    const std::vector<double> &concentrationArray) {
  if (concentrationArray.size() != imgs.volume().nVoxels()) {
    SPDLOG_ERROR("mismatch between array size {} and compartment pixels {}",
                 concentrationArray.size(), imgs.volume().nVoxels());
    throw std::invalid_argument("invalid concentration array size");
  }
  // get concentration at each point in compartment
  data.clear();
  data.reserve(voxels.size());
  std::transform(voxels.cbegin(), voxels.cend(), std::back_inserter(data),
                 [this, &concentrationArray](const auto &voxel) {
                   return concentrationArray[pointToArrayIndex(voxel)];
                 });
  updateImageFromData();
}

void DialogImageData::importImage(
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
  data.clear();
  data.reserve(voxels.size());
  std::transform(voxels.cbegin(), voxels.cend(), std::back_inserter(data),
                 [concImage](const auto &voxel) {
                   return static_cast<double>(
                       (*concImage)[voxel.z].pixel(voxel.p));
                 });
  rescaleData(0, 1);
  updateImageFromData();
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

void DialogImageData::updateImageFromData() {
  // update min/max concentrations
  const auto [newMin, newMax] = getMinMax(data);
  SPDLOG_DEBUG("min {}, max {}", newMin, newMax);
  ui->txtMinConc->setText(toQStr(newMin));
  ui->txtMaxConc->setText(toQStr(newMax));
  // normalise intensity to max concentration
  imgs.fill(0);
  for (std::size_t i = 0; i < voxels.size(); ++i) {
    const auto &voxel{voxels[i]};
    int intensity =
        static_cast<int>(255 * (data[i] - newMin) / (newMax - newMin));
    imgs[voxel.z].setPixel(voxel.p,
                           QColor(intensity, intensity, intensity).rgb());
  }
  ui->lblImage->setImage(imgs);
}

void DialogImageData::rescaleData(double newMin, double newMax) {
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
  std::tie(oldMin, oldMax) = getMinMax(data);
  SPDLOG_INFO("  - old range [{}, {}]", oldMin, oldMax);
  std::transform(data.begin(), data.end(), data.begin(),
                 [&newMin, &newMax, &oldMin, &oldMax](double c) {
                   return newMin +
                          (newMax - newMin) * (c - oldMin) / (oldMax - oldMin);
                 });
}

void DialogImageData::gaussianFilter(const sme::common::Voxel &direction,
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
  auto arr = getImageArray();
  std::size_t concIndex = 0;
  for (const auto &voxel : voxels) {
    data[concIndex] *= gaussianWeights[0];
    auto i = pointToArrayIndex(voxel);
    auto iUp = i;
    auto iDn = i;
    for (int iGauss = 1; iGauss < gaussianWeights.size(); ++iGauss) {
      auto iNeighbour = qpi.getIndex(voxel + iGauss * direction);
      if (iNeighbour) {
        iUp = pointToArrayIndex(voxels[*iNeighbour]);
      }
      data[concIndex] += gaussianWeights[iGauss] * arr[iUp];
      iNeighbour = qpi.getIndex(voxel - iGauss * direction);
      if (iNeighbour) {
        iDn = pointToArrayIndex(voxels[*iNeighbour]);
      }
      data[concIndex] += gaussianWeights[iGauss] * arr[iDn];
    }
    ++concIndex;
  }
}

void DialogImageData::smoothData() {
  double sigma = 0.5;
  gaussianFilter({1, 0, 0}, sigma);
  gaussianFilter({0, 1, 0}, sigma);
  gaussianFilter({0, 0, 1}, sigma);
  updateImageFromData();
}

void DialogImageData::lblImage_mouseOver(const sme::common::Voxel &voxel) {
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
                                    .arg(data[*index])
                                    .arg(quantityUnit));
}

void DialogImageData::chkGrid_stateChanged(int state) {
  ui->lblImage->displayGrid(state == Qt::Checked);
}
void DialogImageData::chkScale_stateChanged(int state) {
  ui->lblImage->displayScale(state == Qt::Checked);
}

void DialogImageData::btnImportImage_clicked() {
  auto concImg =
      getImageFromUser(this, "Import species concentration from image");
  if (!concImg.empty()) {
    importImage(concImg);
  }
}

void DialogImageData::btnExportImage_clicked() {
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

void DialogImageData::cmbExampleImages_currentTextChanged(const QString &text) {
  if (ui->cmbExampleImages->currentIndex() != 0) {
    SPDLOG_DEBUG("import {}", text.toStdString());
    QImage concImg(QString(":/concentration/%1.png").arg(text));
    importImage(sme::common::ImageStack({concImg}));
    ui->cmbExampleImages->setCurrentIndex(0);
  }
}
