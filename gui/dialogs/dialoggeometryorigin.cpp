#include "dialoggeometryorigin.hpp"
#include "ui_dialoggeometryorigin.h"
#include <QMessageBox>
#include <cmath>

static QString toQStr(double x) { return QString::number(x, 'g', 14); }

DialogGeometryOrigin::DialogGeometryOrigin(const sme::common::VoxelF &origin,
                                           const QString &lengthUnit,
                                           QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogGeometryOrigin>()},
      origin(origin) {
  ui->setupUi(this);
  ui->txtXOrigin->setText(toQStr(origin.p.x()));
  ui->txtYOrigin->setText(toQStr(origin.p.y()));
  ui->txtZOrigin->setText(toQStr(origin.z));
  ui->lblXUnits->setText(lengthUnit);
  ui->lblYUnits->setText(lengthUnit);
  ui->lblZUnits->setText(lengthUnit);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogGeometryOrigin::acceptIfValid);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogGeometryOrigin::reject);
}

DialogGeometryOrigin::~DialogGeometryOrigin() = default;

const sme::common::VoxelF &DialogGeometryOrigin::getOrigin() const {
  return origin;
}

void DialogGeometryOrigin::acceptIfValid() {
  bool validX{false};
  bool validY{false};
  bool validZ{false};
  const auto x{ui->txtXOrigin->text().toDouble(&validX)};
  const auto y{ui->txtYOrigin->text().toDouble(&validY)};
  const auto z{ui->txtZOrigin->text().toDouble(&validZ)};
  if (!(validX && validY && validZ && std::isfinite(x) && std::isfinite(y) &&
        std::isfinite(z))) {
    QMessageBox::warning(this, "Invalid origin values",
                         "Please enter valid finite numeric values for x, y "
                         "and z origin.");
    return;
  }
  origin = {x, y, z};
  accept();
}
