#include "dialogimagesize.hpp"

#include "ui_dialogimagesize.h"

static QString dblToString(double val) { return QString::number(val, 'g', 14); }

DialogImageSize::DialogImageSize(const QImage& image, double pixelWidth,
                                 const units::UnitVector& lengthUnits,
                                 QWidget* parent)
    : QDialog(parent),
      ui(new Ui::DialogImageSize),
      img(image),
      pixelModelUnits(pixelWidth),
      length(lengthUnits) {
  ui->setupUi(this);

  ui->lblImage->setImage(img);

  for (auto* cmb : {ui->cmbUnitsWidth, ui->cmbUnitsHeight}) {
    for (const auto& u : length.units) {
      cmb->addItem(u.symbol);
    }
    cmb->setCurrentIndex(length.index);
  }
  modelUnitSymbol = ui->cmbUnitsWidth->currentText();
  pixelLocalUnits = pixelModelUnits;

  updateAll();

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImageSize::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImageSize::reject);
  connect(ui->txtImageWidth, &QLineEdit::editingFinished, this,
          &DialogImageSize::txtImageWidth_editingFinished);

  connect(ui->txtImageHeight, &QLineEdit::editingFinished, this,
          &DialogImageSize::txtImageHeight_editingFinished);

  connect(ui->cmbUnitsWidth, qOverload<int>(&QComboBox::activated), this,
          [this](int index) {
            ui->cmbUnitsHeight->setCurrentIndex(index);
            updateAll();
          });

  connect(ui->cmbUnitsHeight, qOverload<int>(&QComboBox::activated), this,
          [this](int index) {
            ui->cmbUnitsWidth->setCurrentIndex(index);
            updateAll();
          });

  ui->txtImageWidth->selectAll();
}

double DialogImageSize::getPixelWidth() const { return pixelModelUnits; }

void DialogImageSize::updateAll() {
  // calculate size of image in local units
  double w = static_cast<double>(img.width()) * pixelLocalUnits;
  ui->txtImageWidth->setText(dblToString(w));
  double h = static_cast<double>(img.height()) * pixelLocalUnits;
  ui->txtImageHeight->setText(dblToString(h));
  // calculate pixel width in model units
  pixelModelUnits = units::rescale(
      pixelLocalUnits, length.units.at(ui->cmbUnitsWidth->currentIndex()),
      length.get());
  ui->lblPixelSize->setText(
      QString("%1 %2").arg(pixelModelUnits).arg(modelUnitSymbol));
}

void DialogImageSize::txtImageWidth_editingFinished() {
  double w = ui->txtImageWidth->text().toDouble();
  pixelLocalUnits = w / static_cast<double>(img.width());
  updateAll();
}

void DialogImageSize::txtImageHeight_editingFinished() {
  double h = ui->txtImageHeight->text().toDouble();
  pixelLocalUnits = h / static_cast<double>(img.height());
  updateAll();
}
