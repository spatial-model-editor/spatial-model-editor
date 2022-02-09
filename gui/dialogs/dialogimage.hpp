#pragma once

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
                       const QString &message = {}, const QImage &image = {});
  ~DialogImage() override;

private:
  std::unique_ptr<Ui::DialogImage> ui;
};
