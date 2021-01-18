#include "dialogimagesize.hpp"
#include "ui_dialogimagesize.h"

static QString dblToString(double val) { return QString::number(val, 'g', 14); }

DialogImageSize::DialogImageSize(const QImage &image, double pixelWidth,
                                 const sme::model::ModelUnits &modelUnits,
                                 QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImageSize>()}, img(image),
      pixelModelUnits(pixelWidth), units(modelUnits) {
  ui->setupUi(this);

  ui->lblImage->setImage(img);

  for (auto *cmb : {ui->cmbUnitsWidth, ui->cmbUnitsHeight}) {
    for (const auto &u : units.getLengthUnits()) {
      cmb->addItem(u.name);
    }
    cmb->setCurrentIndex(units.getLengthIndex());
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

DialogImageSize::~DialogImageSize() = default;

double DialogImageSize::getPixelWidth() const { return pixelModelUnits; }

void DialogImageSize::updateAll() {
  // calculate size of image in local units
  double w = static_cast<double>(img.width()) * pixelLocalUnits;
  ui->txtImageWidth->setText(dblToString(w));
  double h = static_cast<double>(img.height()) * pixelLocalUnits;
  ui->txtImageHeight->setText(dblToString(h));
  // calculate pixel width in model units
  pixelModelUnits = sme::model::rescale(
      pixelLocalUnits,
      units.getLengthUnits().at(ui->cmbUnitsWidth->currentIndex()),
      units.getLength());
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
