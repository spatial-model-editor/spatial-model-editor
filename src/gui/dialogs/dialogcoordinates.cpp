#include "dialogcoordinates.hpp"

#include "ui_dialogcoordinates.h"

DialogCoordinates::DialogCoordinates(const QString &xName, const QString &yName,
                                     QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogCoordinates>()} {
  ui->setupUi(this);
  ui->txtXName->setText(xName);
  ui->txtYName->setText(yName);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogCoordinates::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogCoordinates::reject);
}

DialogCoordinates::~DialogCoordinates() = default;

QString DialogCoordinates::getXName() const { return ui->txtXName->text(); }

QString DialogCoordinates::getYName() const { return ui->txtYName->text(); }
