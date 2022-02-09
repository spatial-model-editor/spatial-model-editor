#pragma once

#include "sme/model.hpp"
#include "sme/simulate.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogExport;
}

class PlotWrapper;

class DialogExport : public QDialog {
  Q_OBJECT

public:
  explicit DialogExport(const QVector<QImage> &images,
                        const PlotWrapper *plotWrapper,
                        sme::model::Model &model,
                        const sme::simulate::Simulation &simulation,
                        int timepoint = 0, QWidget *parent = nullptr);
  ~DialogExport();

private:
  const std::unique_ptr<Ui::DialogExport> ui;
  const QVector<QImage> &imgs;
  const PlotWrapper *plot;
  sme::model::Model &m;
  const sme::simulate::Simulation &sim;

  void radios_toggled(bool checked);
  void doExport();
  void saveImage();
  void saveImages();
  void saveCSV();
};
