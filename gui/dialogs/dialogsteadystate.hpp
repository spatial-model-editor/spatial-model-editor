#pragma once

#include "sme/model.hpp"
#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <memory>

namespace Ui {
class DialogSteadystate;
}

class DialogSteadystate : public QDialog {
  Q_OBJECT
private:
  enum struct VizMode {
    _2D,
    _3D,
  };
  std::unique_ptr<Ui::DialogSteadystate> ui;
  QTimer m_plotRefreshTimer;
  VizMode vizmode;
  void init();
  void solver_currentIndexChanged(int index);
  void convergenceMode_currentIndexChanged(int index);
  void plottingMode_currentIndexChanged(int index);
  void timeout_input();
  void tolerance_input();
  void btnStartStop_clicked();
  void updatePlots();
  void finalizePlots();
  void selectZ();

public:
  DialogSteadystate(sme::model::Model &model, QWidget *parent = nullptr);
  DialogSteadystate() = default;
  ~DialogSteadystate() override = default;
  VizMode getVizMode() const;
};
