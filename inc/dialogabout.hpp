#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogAbout;
}

class DialogAbout : public QDialog {
  Q_OBJECT

 public:
  explicit DialogAbout(QWidget* parent = nullptr);

 private:
  std::shared_ptr<Ui::DialogAbout> ui;
};
