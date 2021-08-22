#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogAbout;
}

class DialogAbout : public QDialog {
  Q_OBJECT

public:
  explicit DialogAbout(QWidget *parent = nullptr);
  ~DialogAbout();

private:
  std::unique_ptr<Ui::DialogAbout> ui;
};
