#pragma once

#include "sme/image_stack.hpp"
#include <QDialog>
#include <QImage>
#include <memory>

namespace Ui {
class DialogImage;
}

class DialogImage : public QDialog {
  Q_OBJECT

public:
  explicit DialogImage(QWidget *parent = nullptr, const QString &title = {},
                       const QString &message = {},
                       const sme::common::ImageStack &image = {});
  ~DialogImage() override;

private:
  std::unique_ptr<Ui::DialogImage> ui;
};
