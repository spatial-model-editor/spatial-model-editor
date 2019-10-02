#include "dialogdimensions.hpp"

#include "ui_dialogdimensions.h"

static QString dblToString(double val) { return QString::number(val, 'g', 14); }

DialogDimensions::DialogDimensions(const QSize& imageSize, double pixelWidth,
                                   QWidget* parent)
    : QDialog(parent),
      ui(new Ui::DialogDimensions),
      imgSize(imageSize),
      pixel(pixelWidth) {
  ui->setupUi(this);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogDimensions::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogDimensions::reject);

  updateAll();
  connect(ui->txtImageWidth, &QLineEdit::editingFinished, this,
          &DialogDimensions::txtImageWidth_editingFinished);

  connect(ui->txtImageHeight, &QLineEdit::editingFinished, this,
          &DialogDimensions::txtImageHeight_editingFinished);
  ui->txtImageWidth->selectAll();
}

double DialogDimensions::getPixelWidth() const { return pixel; }

bool DialogDimensions::resizeCompartments() const {
  return ui->chkResize->isChecked();
}

void DialogDimensions::updateAll() {
  double w = static_cast<double>(imgSize.width()) * pixel;
  ui->txtImageWidth->setText(dblToString(w));
  double h = static_cast<double>(imgSize.height()) * pixel;
  ui->txtImageHeight->setText(dblToString(h));
}

void DialogDimensions::txtImageWidth_editingFinished() {
  double w = ui->txtImageWidth->text().toDouble();
  pixel = w / static_cast<double>(imgSize.width());
  updateAll();
}

void DialogDimensions::txtImageHeight_editingFinished() {
  double h = ui->txtImageHeight->text().toDouble();
  pixel = h / static_cast<double>(imgSize.height());
  updateAll();
}
