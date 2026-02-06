#include "dialogimportgeometrygmsh.hpp"
#include "ui_dialogimportgeometrygmsh.h"
#include <QFileDialog>
#include <QGuiApplication>
#include <QPushButton>
#include <algorithm>

DialogImportGeometryGmsh::DialogImportGeometryGmsh(int maxVoxelsPerDimension,
                                                   QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImportGeometryGmsh>()} {
  ui->setupUi(this);

  ui->lblImage->setZSlider(ui->slideZIndex);
  ui->lblImage->displayGrid(ui->chkGrid->isChecked());
  ui->lblImage->displayScale(ui->chkScale->isChecked());
  ui->lblImage->setAspectRatioMode(Qt::AspectRatioMode::KeepAspectRatio);
  ui->spinMaxDimension->setValue(std::max(1, maxVoxelsPerDimension));

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImportGeometryGmsh::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImportGeometryGmsh::reject);
  connect(ui->btnBrowse, &QPushButton::clicked, this,
          &DialogImportGeometryGmsh::btnBrowse_clicked);
  connect(ui->txtFilename, &QLineEdit::returnPressed, this,
          &DialogImportGeometryGmsh::readMesh);
  connect(ui->spinMaxDimension, qOverload<int>(&QSpinBox::valueChanged), this,
          &DialogImportGeometryGmsh::spinMaxDimension_valueChanged);
  connect(ui->chkIncludeBackground, &QCheckBox::checkStateChanged, this,
          &DialogImportGeometryGmsh::chkIncludeBackground_stateChanged);
  connect(ui->chkGrid, &QCheckBox::checkStateChanged, this,
          &DialogImportGeometryGmsh::chkGrid_stateChanged);
  connect(ui->chkScale, &QCheckBox::checkStateChanged, this,
          &DialogImportGeometryGmsh::chkScale_stateChanged);

  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  setStatusInfo("Select a Gmsh mesh (.msh) file");
}

DialogImportGeometryGmsh::~DialogImportGeometryGmsh() {
  if (ui != nullptr) {
    disconnect(ui->txtFilename, nullptr, this, nullptr);
    disconnect(ui->spinMaxDimension, nullptr, this, nullptr);
    disconnect(ui->chkIncludeBackground, nullptr, this, nullptr);
    disconnect(ui->chkGrid, nullptr, this, nullptr);
    disconnect(ui->chkScale, nullptr, this, nullptr);
    disconnect(ui->btnBrowse, nullptr, this, nullptr);
    disconnect(ui->buttonBox, nullptr, this, nullptr);
  }
}

const sme::common::ImageStack &DialogImportGeometryGmsh::getImage() const {
  return image;
}

void DialogImportGeometryGmsh::btnBrowse_clicked() {
  QString filename =
      QFileDialog::getOpenFileName(this, "Import geometry from Gmsh mesh", "",
                                   "Gmsh mesh (*.msh);; All files (*.*)");
  if (filename.isEmpty()) {
    return;
  }
  ui->txtFilename->setText(filename);
  readMesh();
}

void DialogImportGeometryGmsh::spinMaxDimension_valueChanged(
    [[maybe_unused]] int value) {
  voxelizeMesh();
}

void DialogImportGeometryGmsh::chkIncludeBackground_stateChanged(
    [[maybe_unused]] int state) {
  voxelizeMesh();
}

void DialogImportGeometryGmsh::chkGrid_stateChanged(int state) {
  ui->lblImage->displayGrid(state == Qt::Checked);
}

void DialogImportGeometryGmsh::chkScale_stateChanged(int state) {
  ui->lblImage->displayScale(state == Qt::Checked);
}

void DialogImportGeometryGmsh::setStatusError(const QString &message) {
  ui->lblStatus->setStyleSheet("font-weight: bold; color: red");
  ui->lblStatus->setText(message);
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void DialogImportGeometryGmsh::setStatusInfo(const QString &message) {
  ui->lblStatus->setStyleSheet({});
  ui->lblStatus->setText(message);
}

void DialogImportGeometryGmsh::readMesh() {
  mesh.reset();
  image = {};
  ui->lblImage->setImage(image);

  auto filename = ui->txtFilename->text().trimmed();
  if (filename.isEmpty()) {
    setStatusInfo("Select a Gmsh mesh (.msh) file");
    return;
  }

  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  mesh = sme::mesh::readGMSHMesh(filename);
  QGuiApplication::restoreOverrideCursor();
  if (!mesh.has_value()) {
    setStatusError(QString("Failed to parse Gmsh file '%1'").arg(filename));
    return;
  }

  voxelizeMesh();
}

void DialogImportGeometryGmsh::voxelizeMesh() {
  if (!mesh.has_value()) {
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    return;
  }

  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  image = sme::mesh::voxelizeGMSHMesh(*mesh, ui->spinMaxDimension->value(),
                                      ui->chkIncludeBackground->isChecked());
  QGuiApplication::restoreOverrideCursor();
  if (image.empty()) {
    setStatusError("Failed to voxelize mesh");
    return;
  }

  ui->lblImage->setImage(image);
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  auto vol = image.volume();
  setStatusInfo(QString("Voxelized geometry: %1 x %2 x %3")
                    .arg(vol.width())
                    .arg(vol.height())
                    .arg(vol.depth()));
}
