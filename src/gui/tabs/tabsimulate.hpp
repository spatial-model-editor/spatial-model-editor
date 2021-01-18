// TabSimulate

#pragma once
#include "dialogdisplayoptions.hpp"
#include "plotwrapper.hpp"
#include "simulate.hpp"
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
namespace sme {
namespace model {
class Model;
}
}
class PlotWrapper;

class TabSimulate : public QWidget {
  Q_OBJECT

public:
  explicit TabSimulate(sme::model::Model &doc, QLabelMouseTracker *mouseTracker,
                       QWidget *parent = nullptr);
  ~TabSimulate() override;
  void loadModelData();
  void stopSimulation();
  void useDune(bool enable);
  void reset();
  [[nodiscard]] sme::simulate::Options getOptions() const;
  void setOptions(const sme::simulate::Options &options);

private:
  std::unique_ptr<Ui::TabSimulate> ui;
  sme::model::Model &sbmlDoc;
  QLabelMouseTracker *lblGeometry;
  std::unique_ptr<PlotWrapper> plt;
  std::unique_ptr<sme::simulate::Simulation> sim;
  sme::simulate::SimulatorType simType{sme::simulate::SimulatorType::DUNE};
  sme::simulate::Options simOptions;
  sme::model::DisplayOptions displayOptions;
  QVector<double> time;
  QVector<QImage> images;
  QStringList compartmentNames;
  std::vector<QStringList> speciesNames;
  std::vector<std::vector<std::size_t>> compartmentSpeciesToDraw;
  std::vector<PlotWrapperObservable> observables;
  std::vector<bool> obsVisible;
  std::future<std::size_t> simSteps;
  QTimer plotRefreshTimer;
  QProgressDialog *progressDialog;

  void btnSimulate_clicked();
  void btnSliceImage_clicked();
  void btnExport_clicked();
  void updateSpeciesToDraw();
  void updatePlotAndImages();
  void btnDisplayOptions_clicked();
  void graphClicked(const QMouseEvent *event);
  void hslideTime_valueChanged(int value);
};
