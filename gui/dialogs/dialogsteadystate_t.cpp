#include "../widgets/qlabelmousetracker.hpp"
#include "../widgets/qvoxelrenderer.hpp"
#include "catch_wrapper.hpp"
#include "dialogdisplayoptions.hpp"
#include "dialogsteadystate.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <catch2/catch_test_macros.hpp>
#include <qtabwidget.h>
#include <qtestsupport_core.h>
#include <qwidget.h>
#include <spdlog/spdlog.h>

using namespace sme;
using namespace sme::test;

struct DialogSteadystateWidgets {
  explicit DialogSteadystateWidgets(const DialogSteadystate *dialog) {
    GET_DIALOG_WIDGET(QLabel, tolStepLabel);
    GET_DIALOG_WIDGET(QLineEdit, convIntervalInput);
    GET_DIALOG_WIDGET(QVoxelRenderer, valuesPlot3D);
    GET_DIALOG_WIDGET(QLineEdit, timeoutInput);
    GET_DIALOG_WIDGET(QLabel, zlabel);
    GET_DIALOG_WIDGET(QLabel, timeoutLabel);
    GET_DIALOG_WIDGET(QLabel, convergenceLabel);
    GET_DIALOG_WIDGET(QFrame, line);
    GET_DIALOG_WIDGET(QLabel, convIntervalLabel);
    GET_DIALOG_WIDGET(QLineEdit, tolStepInput);
    GET_DIALOG_WIDGET(QSlider, zaxis);
    GET_DIALOG_WIDGET(QLabel, modeLabel);
    GET_DIALOG_WIDGET(QComboBox, cmbPlotting);
    GET_DIALOG_WIDGET(QLineEdit, toleranceInput);
    GET_DIALOG_WIDGET(QComboBox, cmbConvergence);
    GET_DIALOG_WIDGET(QLabel, valuesLabel);
    GET_DIALOG_WIDGET(QLabelMouseTracker, valuesPlot);
    GET_DIALOG_WIDGET(QLabel, toleranceLabel);
    GET_DIALOG_WIDGET(QCustomPlot, errorPlot);
    GET_DIALOG_WIDGET(QPushButton, btnStartStop);
    GET_DIALOG_WIDGET(QPushButton, btnReset);
    GET_DIALOG_WIDGET(QPushButton, btnDisplayOptions);
    GET_DIALOG_WIDGET(QDialogButtonBox, buttonBox);
  }

  // Widgets for the dialog
  QLabel *tolStepLabel;
  QLineEdit *convIntervalInput;
  QVoxelRenderer *valuesPlot3D;
  QLineEdit *timeoutInput;
  QLabel *zlabel;
  QLabel *timeoutLabel;
  QLabel *convergenceLabel;
  QFrame *line;
  QLabel *convIntervalLabel;
  QLineEdit *tolStepInput;
  QSlider *zaxis;
  QLabel *modeLabel;
  QComboBox *cmbPlotting;
  QLineEdit *toleranceInput;
  QComboBox *cmbConvergence;
  QLabel *valuesLabel;
  QLabelMouseTracker *valuesPlot;
  QLabel *toleranceLabel;
  QCustomPlot *errorPlot;
  QPushButton *btnStartStop;
  QPushButton *btnReset;
  QPushButton *btnDisplayOptions;
  QDialogButtonBox *buttonBox;
};

TEST_CASE("DialogSteadyState", "[gui/dialogs/steadystate][gui/"
                               "dialogs][gui][steadystate]") {
  auto model{getExampleModel(Mod::ABtoC)};
  auto model3D{getExampleModel(Mod::GrayScott3D)};

  SECTION("initialization 2D") {
    DialogSteadystate dia(model);
    DialogSteadystateWidgets widgets(&dia);
    dia.show();
    // valid initial steady state options
    REQUIRE(widgets.btnStartStop->isEnabled() == true);
    REQUIRE(widgets.btnReset->isEnabled() == true);
    REQUIRE(widgets.btnDisplayOptions->isEnabled() == true);
    REQUIRE(widgets.cmbConvergence->isEnabled() == true);
    REQUIRE(widgets.tolStepInput->isEnabled() == true);

    REQUIRE(widgets.toleranceInput->text() == "1e-06");
    REQUIRE(widgets.convIntervalInput->text() == "1");
    REQUIRE(widgets.timeoutInput->text() == "3600");
    REQUIRE(widgets.tolStepInput->text() == "10");
    REQUIRE(widgets.cmbPlotting->currentText() == "2D");
    REQUIRE(widgets.cmbConvergence->currentText() == "Relative");

    REQUIRE(widgets.valuesLabel->text() == "current values:");
    REQUIRE(widgets.toleranceLabel->text() == "Stop tolerance: ");
    REQUIRE(widgets.timeoutLabel->text() == "Timeout [s]:");
    REQUIRE(widgets.convIntervalLabel->text() ==
            "Time interval between convergence checks [s]: ");
    REQUIRE(widgets.convergenceLabel->text() == "Convergence mode:");
    REQUIRE(widgets.tolStepLabel->text() == "Steps to convergence:");
    REQUIRE(widgets.errorPlot->isVisible() == true);
    REQUIRE(widgets.valuesPlot3D->isVisible() == false);
    REQUIRE(widgets.valuesPlot->isVisible() == true);

    REQUIRE(widgets.zaxis->minimum() == 0);
    REQUIRE(widgets.zaxis->value() == 0);
    REQUIRE(widgets.zaxis->isVisible() == false);
    REQUIRE(widgets.zlabel->isVisible() == false);
    REQUIRE(widgets.zlabel->text() == "z-axis");
    REQUIRE(widgets.cmbPlotting->isEnabled() == false);
  }

  SECTION("inialization 3D") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    REQUIRE(widgets3D.zaxis->minimum() == 0);
    REQUIRE(widgets3D.zaxis->maximum() == 49);
    REQUIRE(widgets3D.zaxis->value() == 0);
    REQUIRE(widgets3D.cmbPlotting->isEnabled() == true);
    REQUIRE(widgets3D.cmbPlotting->currentText() == "2D");
    REQUIRE(widgets3D.valuesPlot3D->isVisible() == false);
    REQUIRE(widgets3D.valuesPlot->isVisible() == true);
  }

  SECTION("set parameters") {
    DialogSteadystate dia(model);
    DialogSteadystateWidgets widgets(&dia);
    dia.show();
    widgets.toleranceInput->setText("1e-5");
    widgets.convIntervalInput->setText("0.1");
    widgets.timeoutInput->setText("10");
    widgets.tolStepInput->setText("5");
    widgets.cmbConvergence->setCurrentText("Relative");
    widgets.cmbPlotting->setCurrentText("3D");
    widgets.zaxis->setValue(10);

    const auto &sim = dia.getSimulator();

    REQUIRE(sim.getStepsToConvergence() == 5);
    REQUIRE(sim.getStopTolerance() == 1e-5);
    REQUIRE(sim.getDt() == 0.1);
    REQUIRE(sim.getTimeout() == 10000); // 10 seconds in milliseconds
    REQUIRE(sim.getConvergenceMode() ==
            sme::simulate::SteadyStateConvergenceMode::relative);
    REQUIRE(sim.getSimulatorType() == sme::simulate::SimulatorType::DUNE);
  }

  SECTION("run_to_convergence") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    ModalWidgetTimer mwt;
    mwt.setIgnoredWidget(&dia3D);
    mwt.start();
    sendMouseClick(widgets3D.btnStartStop);
    REQUIRE(mwt.getResult() == "The simulation has converged.");
    const auto &sim = dia3D.getSimulator();
    REQUIRE(sim.hasConverged() == true);
    REQUIRE(sim.getStepsBelowTolerance() >= sim.getStepsToConvergence());
    QTest::qWait(2000);
    REQUIRE(dia3D.isRunning() == false);

    // check plot updates
    REQUIRE(widgets3D.errorPlot->graphCount() == 2);
    auto errorGraph = widgets3D.errorPlot->graph(0);
    auto tolGraph = widgets3D.errorPlot->graph(1);
    auto data = errorGraph->data();
    auto toldata = tolGraph->data();
    auto lastData = (data->begin() + data->size() - 1);
    auto lastTol = (toldata->begin() + toldata->size() - 1);

    REQUIRE(data->size() == toldata->size());
    REQUIRE(data->size() > 0);
    auto error = sim.getLatestError();
    REQUIRE(lastData->value == error);
    REQUIRE(lastTol->value == sim.getStopTolerance());
    REQUIRE(lastData->key == lastTol->key);
  }

  SECTION("run_stop_resume") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    ModalWidgetTimer mwt;
    mwt.setIgnoredWidget(&dia3D);
    mwt.start();
    const auto &sim = dia3D.getSimulator();
    // start simulation
    sendMouseClick(widgets3D.btnStartStop);
    // stop simulation after 50ms
    QTimer::singleShot(
        50, [&widgets3D]() { sendMouseClick(widgets3D.btnStartStop); });

    REQUIRE(mwt.getResult() == "The simulation has been stopped.");

    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getSolverStopRequested() == true);
    REQUIRE(sim.getStepsBelowTolerance() < sim.getStepsToConvergence());
    REQUIRE(widgets3D.btnStartStop->text() == "Resume");
    REQUIRE(widgets3D.btnReset->isEnabled() == true);
    REQUIRE(dia3D.isRunning() == false);

    // resume simulation and run until convergence
    mwt.start();
    sendMouseClick(widgets3D.btnStartStop);

    REQUIRE(mwt.getResult() == "The simulation has converged.");

    REQUIRE(sim.hasConverged() == true);
    REQUIRE(sim.getStepsBelowTolerance() >= sim.getStepsToConvergence());
    REQUIRE(widgets3D.btnStartStop->text() == "Start");
    REQUIRE(widgets3D.btnReset->isEnabled() == true);
  }

  SECTION("run_stop_reset") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    const auto &sim = dia3D.getSimulator();
    ModalWidgetTimer mwt;
    mwt.setIgnoredWidget(&dia3D);
    mwt.start();

    // start simulation
    sendMouseClick(widgets3D.btnStartStop);
    // stop simulation after 50ms
    QTimer::singleShot(
        50, [&widgets3D]() { sendMouseClick(widgets3D.btnStartStop); });

    REQUIRE(mwt.getResult() == "The simulation has been stopped.");

    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStepsBelowTolerance() < sim.getStepsToConvergence());
    REQUIRE(sim.getLatestError() < std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep() > 0.0);

    // reset everything
    sendMouseClick(widgets3D.btnReset);

    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStepsBelowTolerance() == 0);
    REQUIRE(sim.getStepsToConvergence() == 10);
    REQUIRE(sim.getStopTolerance() == 1e-6);
    REQUIRE(sim.getDt() == 1);
    REQUIRE(sim.getTimeout() == 3600000); // 1 hour in milliseconds
    REQUIRE(sim.getLatestError() == std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep() == 0.0);
  }

  SECTION("zslider_functionality") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();

    ModalWidgetTimer mwt;
    mwt.setIgnoredWidget(&dia3D);
    mwt.start();

    // start simulation
    sendMouseClick(widgets3D.btnStartStop);

    REQUIRE(mwt.getResult() == "The simulation has converged.");

    // // slide the zaxis around
    widgets3D.zaxis->setValue(25); // interval: [0,49]

    REQUIRE(widgets3D.zaxis->value() == 25);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->value() == 25);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->isVisible() == true);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->maximum() == 49);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->minimum() == 0);
  }

  SECTION("run_then_open_dialogDisplayOptions") {
    SPDLOG_CRITICAL("DialogSteadystate: run_then_open_dialogDisplayOptions");

    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    ModalWidgetTimer mwt;
    mwt.setIgnoredWidget(&dia3D);
    mwt.start();

    // start simulation
    sendMouseClick(widgets3D.btnStartStop);
    // stop simulation after 50ms
    QTimer::singleShot(
        50, [&widgets3D]() { sendMouseClick(widgets3D.btnStartStop); });

    REQUIRE(mwt.getResult() == "The simulation has been stopped.");

    mwt.start();

    // open display options dialog
    sendMouseClick(widgets3D.btnDisplayOptions);

    REQUIRE(mwt.getResult() == "Display options");
  }
}
