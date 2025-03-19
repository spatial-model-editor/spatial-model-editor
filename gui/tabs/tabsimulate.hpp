// TabSimulate

#pragma once
#include "dialogdisplayoptions.hpp"
#include "plotwrapper.hpp"
#include "sme/image_stack.hpp"
#include "sme/simulate.hpp"
#include <QWidget>
#include <future>
#include <memory>

namespace Ui {
class TabSimulate;
}
class QCustomPlot;
class QCPAbstractPlottable;
class QCPItemStraightLine;
class QCPTextElement;
class QLabelMouseTracker;
class QVoxelRenderer;
namespace sme::model {
class Model;
} // namespace sme::model
class PlotWrapper;

class TabSimulate : public QWidget {
  Q_OBJECT

public:
  explicit TabSimulate(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                       QVoxelRenderer *voxelRenderer,
                       QWidget *parent = nullptr);
  ~TabSimulate() override;
  void loadModelData();
  void stopSimulation();
  void useSimulator(std::string simulator);
  void importTimesAndIntervalsOnNextLoad();
  void reset();
  void setOptions(const sme::simulate::Options &options);
  void invertYAxis(bool enable);

private:
  std::unique_ptr<Ui::TabSimulate> ui;
  sme::model::Model &model;
  QLabelMouseTracker *lblGeometry;
  QVoxelRenderer *voxGeometry;
  std::unique_ptr<PlotWrapper> plt;
  std::unique_ptr<sme::simulate::Simulation> sim;
  sme::model::DisplayOptions displayOptions;
  QVector<double> time;
  QVector<sme::common::ImageStack> images;
  QStringList compartmentNames;
  std::vector<QStringList> speciesNames;
  std::vector<std::vector<std::size_t>> compartmentSpeciesToDraw;
  std::vector<PlotWrapperObservable> observables;
  std::future<std::size_t> simSteps;
  QTimer plotRefreshTimer;
  QProgressDialog *progressDialog;
  bool importTimesAndIntervals{false};
  bool flipYAxis{false};

  std::optional<std::vector<std::pair<std::size_t, double>>>
  parseSimulationTimes();
  void btnSimulate_clicked();
  void btnSliceImage_clicked();
  void btnExport_clicked();
  void updateSpeciesToDraw();
  void updatePlotAndImages();
  void finalizePlotAndImages();
  void btnDisplayOptions_clicked();
  void graphClicked(const QMouseEvent *event);
  void hslideTime_valueChanged(int value);
  sme::simulate::SimulatorType
  chooseAlternativeSimulator(std::string_view simtype);
  const char *chooseAlternativeSimulator(sme::simulate::SimulatorType simtype);
};
