#include "dialogimageslice.hpp"
#include "sme/logger.hpp"
#include "ui_dialogimageslice.h"
#include <QFileDialog>
#include <algorithm>

DialogImageSlice::DialogImageSlice(
    const sme::common::ImageStack &geometryImage,
    const QVector<sme::common::ImageStack> &images,
    const QVector<double> &timepoints, bool invertYAxis, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImageSlice>()},
      imgs{images}, time{timepoints},
      startPoint{0, geometryImage.volume().height() - 1},
      endPoint{geometryImage.volume().width() - 1, 0} {
  ui->setupUi(this);

  ui->lblImage->setAspectRatioMode(Qt::IgnoreAspectRatio);
  ui->lblImage->setTransformationMode(Qt::SmoothTransformation);
  ui->lblSlice->setImage(geometryImage[z_index], invertYAxis);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImageSlice::saveSlicedImage);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImageSlice::reject);
  connect(ui->cmbSliceType, qOverload<int>(&QComboBox::activated), this,
          &DialogImageSlice::cmbSliceType_activated);
  connect(ui->chkAspectRatio, &QCheckBox::clicked, this,
          [lbl = ui->lblImage](bool checked) {
            if (checked) {
              lbl->setAspectRatioMode(Qt::KeepAspectRatio);
            } else {
              lbl->setAspectRatioMode(Qt::IgnoreAspectRatio);
            }
          });
  connect(ui->chkSmoothInterpolation, &QCheckBox::clicked, this,
          [lbl = ui->lblImage](bool checked) {
            if (checked) {
              lbl->setTransformationMode(Qt::SmoothTransformation);
            } else {
              lbl->setTransformationMode(Qt::FastTransformation);
            }
          });
  connect(ui->lblSlice, &QLabelSlice::mouseDown, this,
          &DialogImageSlice::lblSlice_mouseDown);
  connect(ui->lblSlice, &QLabelSlice::sliceDrawn, this,
          &DialogImageSlice::lblSlice_sliceDrawn);
  connect(ui->lblSlice, &QLabelSlice::mouseWheelEvent, this,
          &DialogImageSlice::lblSlice_mouseWheelEvent);
  connect(ui->lblSlice, &QLabelSlice::mouseOver, this,
          &DialogImageSlice::lblSlice_mouseOver);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogImageSlice::lblImage_mouseOver);

  // initial slice type: vertical
  ui->cmbSliceType->setCurrentIndex(1);
  cmbSliceType_activated(ui->cmbSliceType->currentIndex());
}

DialogImageSlice::~DialogImageSlice() = default;

QImage DialogImageSlice::getSlicedImage() const { return slice[0]; }

void DialogImageSlice::updateSlicedImage() {
  const auto &pixels = ui->lblSlice->getSlicePixels();
  auto np{static_cast<int>(pixels.size())};
  auto nt{static_cast<int>(time.size())};
  slice =
      sme::common::ImageStack({nt, np, 1}, QImage::Format_ARGB32_Premultiplied);
  int t = 0;
  for (const auto &img : imgs) {
    int y = np - 1;
    for (const auto &pixel : pixels) {
      slice[0].setPixel(t, y, img[z_index].pixel(pixel));
      --y;
    }
    ++t;
  }
  ui->lblImage->setImage(slice);
}

void DialogImageSlice::saveSlicedImage() {
  QString filename = QFileDialog::getSaveFileName(this, "Save sliced image", "",
                                                  "PNG (*.png)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".png") {
    filename.append(".png");
  }
  getSlicedImage().save(filename);
}

void DialogImageSlice::cmbSliceType_activated(int index) {
  if (index == 2) {
    sliceType = SliceType::Custom;
    lblSlice_sliceDrawn(startPoint, endPoint);
    return;
  }
  if (index == 0) {
    sliceType = SliceType::Horizontal;
  } else if (index == 1) {
    sliceType = SliceType::Vertical;
  }
  lblSlice_mouseDown(
      QPoint(imgs[0].volume().width() / 2, imgs[0].volume().height() / 2));
}

void DialogImageSlice::lblSlice_mouseDown(QPoint point) {
  if (sliceType == SliceType::Horizontal) {
    horizontal = point.y();
    ui->lblSlice->setHorizontalSlice(horizontal);
    updateSlicedImage();
  } else if (sliceType == SliceType::Vertical) {
    vertical = point.x();
    ui->lblSlice->setVerticalSlice(vertical);
    updateSlicedImage();
  }
}

void DialogImageSlice::lblSlice_sliceDrawn(QPoint start, QPoint end) {
  if (sliceType == SliceType::Custom) {
    startPoint = start;
    endPoint = end;
    ui->lblSlice->setSlice(startPoint, endPoint);
    updateSlicedImage();
  }
}

void DialogImageSlice::lblSlice_mouseWheelEvent(int delta) {
  int dp = delta / std::abs(delta);
  QPoint p(std::clamp(horizontal + dp, 0, imgs[0].volume().height() - 1),
           std::clamp(vertical + dp, 0, imgs[0].volume().width() - 1));
  lblSlice_mouseDown(p);
}

void DialogImageSlice::lblSlice_mouseOver(QPoint point) {
  ui->lblMouseLocation->setText(
      QString("Mouse location: (x=%1, y=%2)").arg(point.x()).arg(point.y()));
}

void DialogImageSlice::lblImage_mouseOver(const sme::common::Voxel &voxel) {
  double t = time[voxel.p.x()];
  auto i{static_cast<std::size_t>(slice[0].height() - 1 - voxel.p.y())};
  const auto &p = ui->lblSlice->getSlicePixels()[i];
  ui->lblMouseLocation->setText(QString("Mouse location: (x=%1, y=%2, t=%3)")
                                    .arg(p.x())
                                    .arg(p.y())
                                    .arg(t));
}
