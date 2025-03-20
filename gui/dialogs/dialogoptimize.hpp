#pragma once
#include "sme/model.hpp"
#include "sme/optimize.hpp"
#include "sme/optimize_options.hpp"
#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <future>
#include <memory>

namespace Ui {
class DialogOptimize;
}

class DialogOptimize : public QDialog {
  Q_OBJECT

public:
  explicit DialogOptimize(sme::model::Model &model, QWidget *parent = nullptr);
  ~DialogOptimize() override;
  QString getParametersString() const;
  void applyToModel() const;

  enum struct VizMode {
    _2D,
    _3D,
  };

  VizMode getVizMode() const;

private:
  sme::model::Model &m_model;
  std::unique_ptr<Ui::DialogOptimize> ui;
  std::unique_ptr<sme::simulate::Optimization> m_opt;
  std::future<std::size_t> m_optIterations;
  std::size_t m_nPlottedIterations{0};
  QTimer m_plotRefreshTimer;
  VizMode vizMode{VizMode::_2D};
  void init();
  void cmbTarget_currentIndexChanged(int index);
  void cmbMode_currentIndexChanged(int index);
  void btnStartStop_clicked();
  void btnSetup_clicked();
  void updatePlots();
  void updateTabs();
  void finalizePlots();
  void selectZ();
  void updateTargetImage();
  void updateResultImage();
  void updateDifferenceImage();
};
