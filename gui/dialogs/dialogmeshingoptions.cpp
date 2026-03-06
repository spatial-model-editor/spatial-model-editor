#include "dialogmeshingoptions.hpp"
#include "ui_dialogmeshingoptions.h"

DialogMeshingOptions::DialogMeshingOptions(
    std::size_t boundarySimplificationType, std::size_t meshSourceType,
    QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogMeshingOptions>()} {
  ui->setupUi(this);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogMeshingOptions::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogMeshingOptions::reject);
  ui->cmbBoundarySimplificationType->setCurrentIndex(
      static_cast<int>(boundarySimplificationType));
  ui->cmbMeshSourceType->setCurrentIndex(static_cast<int>(meshSourceType));
}

DialogMeshingOptions::~DialogMeshingOptions() = default;

std::size_t DialogMeshingOptions::getBoundarySimplificationType() const {
  return static_cast<std::size_t>(
      ui->cmbBoundarySimplificationType->currentIndex());
}

std::size_t DialogMeshingOptions::getMeshSourceType() const {
  return static_cast<std::size_t>(ui->cmbMeshSourceType->currentIndex());
}
