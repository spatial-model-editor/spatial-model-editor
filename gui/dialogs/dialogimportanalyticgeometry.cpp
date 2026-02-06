#include "dialogimportanalyticgeometry.hpp"
#include "ui_dialogimportanalyticgeometry.h"
#include <QCursor>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QPushButton>

DialogImportAnalyticGeometry::DialogImportAnalyticGeometry(
    const QString &sbmlFilename, const sme::common::Volume &defaultImageSize,
    QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImportAnalyticGeometry>()},
      filename(sbmlFilename), defaultSize(defaultImageSize) {
  ui->setupUi(this);
  const auto numDimensions{
      sme::model::ModelGeometry::getAnalyticGeometryNumDimensions(filename)};
  bool hasEditableZResolution{!numDimensions || *numDimensions > 2};
  ui->lblPreview->setZSlider(ui->slideZIndex);
  ui->spinResolutionX->setValue(defaultSize.width());
  ui->spinResolutionY->setValue(defaultSize.height());
  ui->spinResolutionZ->setValue(
      hasEditableZResolution ? static_cast<int>(defaultSize.depth()) : 1);
  ui->spinResolutionZ->setEnabled(hasEditableZResolution);
  ui->spinResolutionZ->setVisible(hasEditableZResolution);
  ui->lblResolutionY->setVisible(hasEditableZResolution);
  ui->slideZIndex->setVisible(hasEditableZResolution);
  ui->lblZSlice->setVisible(hasEditableZResolution);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImportAnalyticGeometry::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImportAnalyticGeometry::reject);
  connect(ui->spinResolutionX, qOverload<int>(&QSpinBox::valueChanged), this,
          [this](int) { updatePreview(); });
  connect(ui->spinResolutionY, qOverload<int>(&QSpinBox::valueChanged), this,
          [this](int) { updatePreview(); });
  connect(ui->spinResolutionZ, qOverload<int>(&QSpinBox::valueChanged), this,
          [this](int) { updatePreview(); });
  connect(ui->btnResetResolution, &QPushButton::clicked, this,
          &DialogImportAnalyticGeometry::btnResetResolution_clicked);

  updatePreview();
}

DialogImportAnalyticGeometry::~DialogImportAnalyticGeometry() = default;

sme::common::Volume DialogImportAnalyticGeometry::getImageSize() const {
  std::size_t depth{1};
  if (ui->spinResolutionZ->isEnabled()) {
    depth = static_cast<std::size_t>(ui->spinResolutionZ->value());
  }
  return {ui->spinResolutionX->value(), ui->spinResolutionY->value(), depth};
}

void DialogImportAnalyticGeometry::updatePreview() {
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  auto preview{sme::model::ModelGeometry::getAnalyticGeometryPreview(
      filename, getImageSize())};
  QGuiApplication::restoreOverrideCursor();
  if (preview.empty()) {
    ui->lblStatus->setText("Failed to generate analytic geometry preview");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    return;
  }
  ui->lblPreview->setImage(preview);
  ui->lblStatus->setText(QString("Preview: %1 x %2 x %3 voxels")
                             .arg(preview.volume().width())
                             .arg(preview.volume().height())
                             .arg(preview.volume().depth()));
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DialogImportAnalyticGeometry::btnResetResolution_clicked() {
  ui->spinResolutionX->setValue(defaultSize.width());
  ui->spinResolutionY->setValue(defaultSize.height());
  ui->spinResolutionZ->setValue(ui->spinResolutionZ->isEnabled()
                                    ? static_cast<int>(defaultSize.depth())
                                    : 1);
}
