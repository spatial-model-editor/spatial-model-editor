#include "dialogimageslice.hpp"

#include <QFileDialog>

#include "ui_dialogimageslice.h"

DialogImageSlice::DialogImageSlice(const QVector<QImage>& images,
                                   const QVector<double>& timepoints,
                                   QWidget* parent)
    : QDialog(parent),
      ui{std::make_unique<Ui::DialogImageSlice>()},
      imgs(images),
      time(timepoints) {
  ui->setupUi(this);
  // do smooth interpolation & ignore aspect ratio
  ui->lblImage->setAspectRatioMode(Qt::IgnoreAspectRatio);
  ui->lblImage->setTransformationMode(Qt::SmoothTransformation);

  // default: y axis
  ui->hslideZ->setMaximum(imgs[0].width() - 1);
  ui->cmbImageVerticalAxis->setCurrentIndex(1);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImageSlice::saveSlicedImage);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImageSlice::reject);
  connect(ui->cmbImageVerticalAxis, qOverload<int>(&QComboBox::activated), this,
          [this](int index) {
            if (index == 0) {
              ui->hslideZ->setMaximum(imgs[0].height() - 1);
            } else {
              ui->hslideZ->setMaximum(imgs[0].width() - 1);
            };
            ui->hslideZ->setValue(0);
          });
  connect(ui->hslideZ, &QSlider::valueChanged, this,
          &DialogImageSlice::hslideTime_valueChanged);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogImageSlice::lblImage_mouseOver);
  ui->hslideZ->setValue(ui->hslideZ->maximum() / 2);
}

DialogImageSlice::~DialogImageSlice() = default;

QImage DialogImageSlice::getSlicedImage() const { return slice; }

void DialogImageSlice::hslideTime_valueChanged(int value) {
  if (ui->cmbImageVerticalAxis->currentIndex() == 0) {
    sliceAtY(value);
    ui->lblZLabel->setText(QString("y = %1").arg(value));
  } else {
    sliceAtX(value);
    ui->lblZLabel->setText(QString("x = %1").arg(value));
  }
}

void DialogImageSlice::sliceAtX(int x) {
  slice = QImage(time.size(), imgs[0].height(),
                 QImage::Format_ARGB32_Premultiplied);
  int t = 0;
  for (const auto& img : imgs) {
    for (int y = 0; y < img.height(); ++y) {
      slice.setPixel(t, y, img.pixel(x, y));
    }
    ++t;
  }
  ui->lblImage->setImage(slice);
}

void DialogImageSlice::sliceAtY(int y) {
  slice =
      QImage(time.size(), imgs[0].width(), QImage::Format_ARGB32_Premultiplied);
  int t = 0;
  for (const auto& img : imgs) {
    for (int x = 0; x < img.width(); ++x) {
      slice.setPixel(t, slice.height() - 1 - x, img.pixel(x, y));
    }
    ++t;
  }
  ui->lblImage->setImage(slice);
}

void DialogImageSlice::lblImage_mouseOver(const QPoint& point) {
  int x;
  int y;
  double t = time[point.x()];
  if (ui->cmbImageVerticalAxis->currentIndex() == 0) {
    y = ui->hslideZ->value();
    x = slice.height() - 1 - point.y();
  } else {
    x = ui->hslideZ->value();
    y = slice.height() - 1 - point.y();
  }
  ui->lblMouseLocation->setText(
      QString("Mouse location: (x=%1, y=%2, t=%3)").arg(x).arg(y).arg(t));
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
