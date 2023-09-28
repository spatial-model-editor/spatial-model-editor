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
  colorTable = coloredImage[0].colorTable();
  ui->setupUi(this);
  ui->lblImage->setZSlider(ui->slideZIndex);
  ui->lblImage->setImage(rescaledImage);
  ui->btnApplyColours->setEnabled(false);
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
  updateColours();
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
  connect(ui->btnSelectColours, &QPushButton::clicked, this,
          &DialogGeometryImage::btnSelectColours_clicked);
  connect(ui->btnApplyColours, &QPushButton::clicked, this,
          &DialogGeometryImage::btnApplyColours_clicked);
  connect(ui->btnResetColours, &QPushButton::clicked, this,
          &DialogGeometryImage::btnResetColours_clicked);

  ui->txtImageWidth->selectAll();
}

DialogGeometryImage::~DialogGeometryImage() = default;

sme::common::VolumeF DialogGeometryImage::getVoxelSize() const {
  return voxelModelUnits;
}

bool DialogGeometryImage::imageSizeAltered() const { return alteredSize; }

bool DialogGeometryImage::imageColoursAltered() const { return alteredColours; }

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
  ui->lblImage->setPhysicalSize(volumeModelUnits, modelUnit.name);
  ui->lblPixelSize->setText(QString("%1 %4 x %2 %4 x %3 %4")
                                .arg(voxelModelUnits.width())
                                .arg(voxelModelUnits.height())
                                .arg(voxelModelUnits.depth())
                                .arg(modelUnitSymbol));
}

void DialogGeometryImage::updateColours() {
  auto n{static_cast<int>(colorTable.size())};
  QImage colourTableImage(1, 1, QImage::Format_Indexed8);
  colourTableImage.fill(qRgb(0, 0, 0));
  if (n > 0) {
    int w{std::max(n, ui->lblColours->width())};
    int h{std::max(1, ui->lblColours->height())};
    int pixelsPerColour{w / n};
    colourTableImage = QImage(w, h, QImage::Format_Indexed8);
    colourTableImage.setColorTable(colorTable);
    for (int x = 0; x < w; ++x) {
      auto colourIndex{
          static_cast<QRgb>(std::clamp(x / pixelsPerColour, 0, n - 1))};
      for (int y = 0; y < h; ++y) {
        colourTableImage.setPixel(x, y, colourIndex);
      }
    }
  }
  if (n == 1) {
    ui->lblLabelColours->setText("1 colour:");
  } else {
    ui->lblLabelColours->setText(QString("%1 colours:").arg(n));
  }
  ui->lblColours->setPixmap(QPixmap::fromImage(colourTableImage));
}

void DialogGeometryImage::enableWidgets(bool enable) {
  ui->txtImageWidth->setEnabled(enable);
  ui->cmbUnitsWidth->setEnabled(enable);
  ui->txtImageHeight->setEnabled(enable);
  ui->txtImageDepth->setEnabled(enable);
  ui->spinPixelsX->setEnabled(enable);
  ui->spinPixelsY->setEnabled(enable);
  ui->btnResetPixels->setEnabled(enable);
  ui->btnSelectColours->setEnabled(enable);
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void DialogGeometryImage::lblImage_mouseClicked(
    QRgb col, [[maybe_unused]] sme::common::Voxel voxel) {
  if (!selectingColours) {
    return;
  }
  if (colorTable.contains(col)) {
    return;
  }
  colorTable.push_back(col);
  ui->btnApplyColours->setEnabled(true);
  updateColours();
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

static void reduceImageToTheseColours(sme::common::ImageStack &images,
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

void DialogGeometryImage::btnSelectColours_clicked() {
  selectingColours = true;
  colorTable.clear();
  updateColours();
  enableWidgets(false);
}

void DialogGeometryImage::btnApplyColours_clicked() {
  alteredColours = true;
  selectingColours = false;
  reduceImageToTheseColours(coloredImage, colorTable);
  enableWidgets(true);
  ui->btnApplyColours->setEnabled(false);
  spinPixelsX_valueChanged(ui->spinPixelsX->value());
}

void DialogGeometryImage::btnResetColours_clicked() {
  alteredColours = false;
  selectingColours = false;
  enableWidgets(true);
  coloredImage = originalImage;
  coloredImage.convertToIndexed();
  colorTable = coloredImage[0].colorTable();
  updateColours();
  spinPixelsX_valueChanged(ui->spinPixelsX->value());
}
