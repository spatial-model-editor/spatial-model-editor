#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogCoordinates;
}

class DialogCoordinates : public QDialog {
  Q_OBJECT

public:
  explicit DialogCoordinates(const QString &xName, const QString &yName,
                             const QString &zName, QWidget *parent = nullptr);
  ~DialogCoordinates();
  QString getXName() const;
  QString getYName() const;
  QString getZName() const;

private:
  std::unique_ptr<Ui::DialogCoordinates> ui;
};
