// QTabSimulate

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class QTabSimulate;
}
class QCustomPlot;
class QCPAbstractPlottable;
class QLabelMouseTracker;
namespace sbml {
class SbmlDocWrapper;
}

class QTabSimulate : public QWidget {
  Q_OBJECT

 public:
  explicit QTabSimulate(sbml::SbmlDocWrapper &doc,
                        QLabelMouseTracker *mouseTracker,
                        QWidget *parent = nullptr);
  ~QTabSimulate();
  void loadModelData();
  void stopSimulation();
  void useDune(bool enable);
  void reset();

 private:
  std::unique_ptr<Ui::QTabSimulate> ui;
  sbml::SbmlDocWrapper &sbmlDoc;
  QLabelMouseTracker *lblGeometry;
  QCustomPlot *pltPlot;
  QVector<QImage> images;
  bool isSimulationRunning = false;
  bool useDuneSimulator = true;

  void btnSimulate_clicked();

  void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);

  void hslideTime_valueChanged(int value);
};
