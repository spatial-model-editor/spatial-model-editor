#include "dialogimage.hpp"
#include "ui_dialogimage.h"
#include <QLabel>
#include <QPixmap>

DialogImage::DialogImage(QWidget *parent, const QString &title,
                         const QString &message,
                         const sme::common::ImageStack &image)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImage>()} {
  ui->setupUi(this);

  setWindowTitle(title);
  ui->lblMessage->setText(message);
  // todo: fix this to not hard code z=0 slice
  ui->lblImage->setPixmap(QPixmap::fromImage(image[0]));
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImage::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImage::reject);
}

DialogImage::~DialogImage() = default;
