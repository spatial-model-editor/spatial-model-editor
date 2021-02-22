#include "dialogimage.hpp"
#include "ui_dialogimage.h"
#include <QLabel>
#include <QPixmap>

DialogImage::DialogImage(QWidget *parent, const QString &title,
                         const QString &message, const QImage &image)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImage>()} {
  ui->setupUi(this);

  setWindowTitle(title);
  ui->lblMessage->setText(message);
  ui->lblImage->setPixmap(QPixmap::fromImage(image));
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImage::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImage::reject);
}

DialogImage::~DialogImage() = default;
