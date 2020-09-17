#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogImageSave;
}

class DialogImageSave : public QDialog {
  Q_OBJECT

public:
  explicit DialogImageSave(const QVector<QImage> &images,
                           const QVector<double> &timepoints, int timepoint = 0,
                           QWidget *parent = nullptr);
  ~DialogImageSave();

private:
  const std::unique_ptr<Ui::DialogImageSave> ui;
  const QVector<QImage> &imgs;
  const QVector<double> &times;

  void radSingleTimepoint_toggled(bool checked);
  void saveImages();
};
