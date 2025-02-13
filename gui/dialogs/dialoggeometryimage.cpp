#include "dialoggeometryimage.hpp"
#include "sme/logger.hpp"
#include "ui_dialoggeometryimage.h"

static QString toQStr(double val) { return QString::number(val, 'g', 14); }

DialogGeometryImage::DialogGeometryImage(
    const sme::common::ImageStack &image, sme::common::VolumeF voxelSize,
    const sme::model::ModelUnits &modelUnits, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogGeometryImage>()},
      originalImage(image), voxelModelUnits(voxelSize), units(modelUnits) {
  coloredImage = originalImage;
  coloredImage.convertToIndexed();
  rescaledImage = coloredImage;
  colorTable = coloredImage.colorTable();
  ui->setupUi(this);
  ui->lblImage->setZSlider(ui->slideZIndex);
  ui->lblImage->setImage(rescaledImage);
  ui->btnApplyColors->setEnabled(false);
  for (const auto &u : units.getLengthUnits()) {
    ui->cmbUnitsWidth->addItem(u.name);
  }
  ui->cmbUnitsWidth->setCurrentIndex(units.getLengthIndex());
  modelUnitSymbol = ui->cmbUnitsWidth->currentText();
  voxelLocalUnits = voxelModelUnits;
  auto imageSizeLocalUnits{rescaledImage.volume() * voxelLocalUnits};
  ui->txtImageWidth->setText(toQStr(imageSizeLocalUnits.width()));
  ui->txtImageHeight->setText(toQStr(imageSizeLocalUnits.height()));
  ui->txtImageDepth->setText(toQStr(imageSizeLocalUnits.depth()));
  ui->spinPixelsX->setValue(rescaledImage.volume().width());
  ui->spinPixelsY->setValue(rescaledImage.volume().height());
  ui->lblZSlices->setText(
      QString("x %1 z-slices").arg(rescaledImage.volume().depth()));
  updateColors();
  updateVoxelSize();

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogGeometryImage::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogGeometryImage::reject);
  connect(ui->lblImage, &QLabelMouseTracker::mouseClicked, this,
          &DialogGeometryImage::lblImage_mouseClicked);
  connect(ui->txtImageWidth, &QLineEdit::editingFinished, this,
          &DialogGeometryImage::txtImageWidth_editingFinished);
  connect(ui->txtImageHeight, &QLineEdit::editingFinished, this,
          &DialogGeometryImage::txtImageHeight_editingFinished);
  connect(ui->txtImageDepth, &QLineEdit::editingFinished, this,
          &DialogGeometryImage::txtImageDepth_editingFinished);
  connect(ui->cmbUnitsWidth, qOverload<int>(&QComboBox::activated), this,
          &DialogGeometryImage::updateVoxelSize);
  connect(ui->spinPixelsX, qOverload<int>(&QSpinBox::valueChanged), this,
          &DialogGeometryImage::spinPixelsX_valueChanged);
  connect(ui->spinPixelsY, qOverload<int>(&QSpinBox::valueChanged), this,
          &DialogGeometryImage::spinPixelsY_valueChanged);
  connect(ui->btnResetPixels, &QPushButton::clicked, this,
          &DialogGeometryImage::btnResetPixels_clicked);
  connect(ui->btnSelectColors, &QPushButton::clicked, this,
          &DialogGeometryImage::btnSelectColors_clicked);
  connect(ui->btnApplyColors, &QPushButton::clicked, this,
          &DialogGeometryImage::btnApplyColors_clicked);
  connect(ui->btnResetColors, &QPushButton::clicked, this,
          &DialogGeometryImage::btnResetColors_clicked);

  ui->txtImageWidth->selectAll();
}

DialogGeometryImage::~DialogGeometryImage() = default;

sme::common::VolumeF DialogGeometryImage::getVoxelSize() const {
  return voxelModelUnits;
}

bool DialogGeometryImage::imageSizeAltered() const { return alteredSize; }

bool DialogGeometryImage::imageColorsAltered() const { return alteredColors; }

const sme::common::ImageStack &DialogGeometryImage::getAlteredImage() const {
  return ui->lblImage->getImage();
}

void DialogGeometryImage::updateVoxelSize() {
  // Physical volume in local units
  bool isValidDouble{false};
  double imageWidth{ui->txtImageWidth->text().toDouble(&isValidDouble)};
  if (!isValidDouble) {
    return;
  }
  double imageHeight{ui->txtImageHeight->text().toDouble(&isValidDouble)};
  if (!isValidDouble) {
    return;
  }
  double imageDepth{ui->txtImageDepth->text().toDouble(&isValidDouble)};
  if (!isValidDouble) {
    return;
  }
  sme::common::VolumeF volumeLocalUnits{imageWidth, imageHeight, imageDepth};
  // Physical volume in model units
  const auto &localUnit{
      units.getLengthUnits().at(ui->cmbUnitsWidth->currentIndex())};
  const auto &modelUnit{units.getLength()};
  sme::common::VolumeF volumeModelUnits{
      sme::model::rescale(volumeLocalUnits, localUnit, modelUnit)};
  // Physical voxel volume in model units
  voxelModelUnits = volumeModelUnits / rescaledImage.volume();
  ui->lblImage->setPhysicalUnits(modelUnit.name);
  ui->lblPixelSize->setText(QString("%1 %4 x %2 %4 x %3 %4")
                                .arg(voxelModelUnits.width())
                                .arg(voxelModelUnits.height())
                                .arg(voxelModelUnits.depth())
                                .arg(modelUnitSymbol));
}

void DialogGeometryImage::updateColors() {
  auto n{static_cast<int>(colorTable.size())};
  QImage colorTableImage(1, 1, QImage::Format_Indexed8);
  colorTableImage.fill(qRgb(0, 0, 0));
  if (n > 0) {
    int w{std::max(n, ui->lblColors->width())};
    int h{std::max(1, ui->lblColors->height())};
    int pixelsPerColor{w / n};
    colorTableImage = QImage(w, h, QImage::Format_Indexed8);
    colorTableImage.setColorTable(colorTable);
    for (int x = 0; x < w; ++x) {
      auto colorIndex{
          static_cast<QRgb>(std::clamp(x / pixelsPerColor, 0, n - 1))};
      for (int y = 0; y < h; ++y) {
        colorTableImage.setPixel(x, y, colorIndex);
      }
    }
  }
  if (n == 1) {
    ui->lblLabelColors->setText("1 color:");
  } else {
    ui->lblLabelColors->setText(QString("%1 colors:").arg(n));
  }
  ui->lblColors->setPixmap(QPixmap::fromImage(colorTableImage));
}

void DialogGeometryImage::enableWidgets(bool enable) {
  ui->txtImageWidth->setEnabled(enable);
  ui->cmbUnitsWidth->setEnabled(enable);
  ui->txtImageHeight->setEnabled(enable);
  ui->txtImageDepth->setEnabled(enable);
  ui->spinPixelsX->setEnabled(enable);
  ui->spinPixelsY->setEnabled(enable);
  ui->btnResetPixels->setEnabled(enable);
  ui->btnSelectColors->setEnabled(enable);
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void DialogGeometryImage::lblImage_mouseClicked(
    QRgb col, [[maybe_unused]] sme::common::Voxel voxel) {
  if (!selectingColors) {
    return;
  }
  if (colorTable.contains(col)) {
    return;
  }
  colorTable.push_back(col);
  ui->btnApplyColors->setEnabled(true);
  updateColors();
}

void DialogGeometryImage::txtImageWidth_editingFinished() { updateVoxelSize(); }

void DialogGeometryImage::txtImageHeight_editingFinished() {
  updateVoxelSize();
}

void DialogGeometryImage::txtImageDepth_editingFinished() { updateVoxelSize(); }

void DialogGeometryImage::spinPixelsX_valueChanged(int value) {
  if (value <= 0) {
    return;
  }
  if (value != coloredImage.volume().width()) {
    alteredSize = true;
  }
  if (ui->chkKeepAspectRatio->isChecked()) {
    rescaledImage = coloredImage.scaledToWidth(value);
    ui->spinPixelsY->blockSignals(true);
    ui->spinPixelsY->setValue(rescaledImage.volume().height());
    ui->spinPixelsY->blockSignals(false);
  } else {
    rescaledImage = coloredImage.scaled(value, rescaledImage.volume().height());
  }
  ui->lblImage->setImage(rescaledImage);
  updateVoxelSize();
}

void DialogGeometryImage::spinPixelsY_valueChanged(int value) {
  if (value <= 0) {
    return;
  }
  if (value != coloredImage.volume().height()) {
    alteredSize = true;
  }
  if (ui->chkKeepAspectRatio->isChecked()) {
    rescaledImage = coloredImage.scaledToHeight(value);
    ui->spinPixelsX->blockSignals(true);
    ui->spinPixelsX->setValue(rescaledImage.volume().width());
    ui->spinPixelsX->blockSignals(false);
  } else {
    rescaledImage = coloredImage.scaled(rescaledImage.volume().width(), value);
  }
  ui->lblImage->setImage(rescaledImage);
  updateVoxelSize();
}

void DialogGeometryImage::btnResetPixels_clicked() {
  alteredSize = false;
  rescaledImage = coloredImage;
  ui->lblImage->setImage(rescaledImage);
  ui->spinPixelsX->setValue(rescaledImage.volume().width());
  ui->spinPixelsY->setValue(rescaledImage.volume().height());
  updateVoxelSize();
}

static int distance(QRgb a, QRgb b) {
  int dr{qRed(a) - qRed(b)};
  int dg{qGreen(a) - qGreen(b)};
  int db{qBlue(a) - qBlue(b)};
  return dr * dr + dg * dg + db * db;
}

static void reduceImageToTheseColors(sme::common::ImageStack &images,
                                     const QVector<QRgb> &colorTable) {
  for (auto &image : images) {
    // map each index in image colorTable to the index of
    // the nearest color in colorTable
    auto nOld{static_cast<int>(image.colorTable().size())};
    QVector<int> newIndex(nOld, 0);
    for (int iOld = 0; iOld < nOld; ++iOld) {
      int dist{std::numeric_limits<int>::max()};
      for (int iNew = 0; iNew < colorTable.size(); ++iNew) {
        if (auto d{distance(image.color(iOld), colorTable.at(iNew))};
            d < dist) {
          dist = d;
          newIndex[iOld] = iNew;
        }
      }
    }
    // set each pixel to the corresponding new index
    for (int x = 0; x < image.width(); ++x) {
      for (int y = 0; y < image.height(); ++y) {
        int iOld{static_cast<int>(image.pixelIndex(x, y))};
        image.setPixel(x, y, static_cast<QRgb>(newIndex[iOld]));
      }
    }
    // update image colorTable to the new one
    image.setColorTable(colorTable);
  }
}

void DialogGeometryImage::btnSelectColors_clicked() {
  selectingColors = true;
  colorTable.clear();
  updateColors();
  enableWidgets(false);
}

void DialogGeometryImage::btnApplyColors_clicked() {
  alteredColors = true;
  selectingColors = false;
  reduceImageToTheseColors(coloredImage, colorTable);
  enableWidgets(true);
  ui->btnApplyColors->setEnabled(false);
  spinPixelsX_valueChanged(ui->spinPixelsX->value());
}

void DialogGeometryImage::btnResetColors_clicked() {
  alteredColors = false;
  selectingColors = false;
  enableWidgets(true);
  coloredImage = originalImage;
  coloredImage.convertToIndexed();
  colorTable = coloredImage.colorTable();
  updateColors();
  spinPixelsX_valueChanged(ui->spinPixelsX->value());
}
