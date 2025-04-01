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

// helper for catching QMessageBox dialogs
struct ModalChecker {
  QTimer checker;
  std::string messageContent;

  void waitForConvergence(DialogSteadystate &dialog,
                          const DialogSteadystateWidgets &widgets) {
    messageContent = "";
    // this didn't work with the ModalTimer we already have?. It always found
    // the dialog itself and closed it before stuff was ready, and didn't latch
    // onto the QMessageBox...?
    QTimer::singleShot(0, [this]() {
      checker.setInterval(250);
      checker.connect(&checker, &QTimer::timeout, [&]() {
        for (QWidget *w : QApplication::topLevelWidgets()) {
          QMessageBox *box = qobject_cast<QMessageBox *>(w);
          if (box && box->isVisible()) {
            messageContent = box->windowTitle().toStdString();
            // Click OK
            QTest::mouseClick(box->button(QMessageBox::Ok), Qt::LeftButton);
            break;
          }
        }
      });
      checker.start();
    });

    const auto &sim = dialog.getSimulator();
    sendMouseClick(widgets.btnStartStop);
    while (sim.hasConverged() == false) {
      QTest::qWait(1000);
    }
    QTest::qWait(2000);
  }

  void waitForThenStop(DialogSteadystate &dialog,
                       const DialogSteadystateWidgets &widgets, int time_ms) {
    messageContent = "";

    QTimer::singleShot(0, [this]() {
      checker.setInterval(250);
      checker.connect(&checker, &QTimer::timeout, [&]() {
        for (QWidget *w : QApplication::topLevelWidgets()) {
          QMessageBox *box = qobject_cast<QMessageBox *>(w);
          if (box && box->isVisible()) {
            messageContent = box->windowTitle().toStdString();
            // Click OK
            QTest::mouseClick(box->button(QMessageBox::Ok), Qt::LeftButton);
            break;
          }
        }
      });
      checker.start();
    });

    const auto &sim = dialog.getSimulator();
    sendMouseClick(widgets.btnStartStop);

    QTest::qWait(time_ms);
    sendMouseClick(widgets.btnStartStop);
  }

  void stop() { checker.stop(); }
};

TEST_CASE("DialogSteadyState", "[gui/dialogs/steadystate][gui/"
                               "dialogs][gui][steadystate]") {
  auto model{getExampleModel(Mod::ABtoC)};
  auto model3D{getExampleModel(Mod::GrayScott3D)};

  SECTION("initialization 2D") {
    DialogSteadystate dia(model);
    DialogSteadystateWidgets widgets(&dia);
    dia.show();
    QTest::qWait(100);
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
    dia.accept();
  }

  SECTION("inialization 3D") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    QTest::qWait(100);
    REQUIRE(widgets3D.zaxis->minimum() == 0);
    REQUIRE(widgets3D.zaxis->maximum() == 49);
    REQUIRE(widgets3D.zaxis->value() == 0);
    REQUIRE(widgets3D.cmbPlotting->isEnabled() == true);
    REQUIRE(widgets3D.cmbPlotting->currentText() == "2D");
    REQUIRE(widgets3D.valuesPlot3D->isVisible() == false);
    REQUIRE(widgets3D.valuesPlot->isVisible() == true);
    dia3D.accept();
  }

  SECTION("set parameters") {
    DialogSteadystate dia(model);
    DialogSteadystateWidgets widgets(&dia);
    dia.show();
    QTest::qWait(100);
    widgets.toleranceInput->setText("1e-5");
    widgets.convIntervalInput->setText("0.1");
    widgets.timeoutInput->setText("10");
    widgets.tolStepInput->setText("5");
    widgets.cmbConvergence->setCurrentText("Relative");
    widgets.cmbPlotting->setCurrentText("3D");
    widgets.zaxis->setValue(10);

    QTest::qWait(2000);

    const auto &sim = dia.getSimulator();

    REQUIRE(sim.getStepsToConvergence() == 5);
    REQUIRE(sim.getStopTolerance() == 1e-5);
    REQUIRE(sim.getDt() == 0.1);
    REQUIRE(sim.getTimeout() == 10000); // 10 seconds in milliseconds
    REQUIRE(sim.getConvergenceMode() ==
            sme::simulate::SteadystateConvergenceMode::relative);
    REQUIRE(sim.getSimulatorType() == sme::simulate::SimulatorType::Pixel);
  }

  SECTION("run_to_convergence") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    ModalChecker checker;
    checker.waitForConvergence(dia3D, widgets3D);
    const auto &sim = dia3D.getSimulator();

    REQUIRE(sim.hasConverged() == true);
    REQUIRE(sim.getStepsBelowTolerance() >= sim.getStepsToConvergence());
    REQUIRE(dia3D.isRunning() == true);
    REQUIRE(checker.messageContent == "Simulation converged");
    checker.stop();
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
    auto error = sim.getLatestError().load();
    REQUIRE(lastData->value == error);
    REQUIRE(lastTol->value == sim.getStopTolerance());
    REQUIRE(lastData->key == lastTol->key);
  }

  SECTION("run_stop_resume") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    ModalChecker checker;
    const auto &sim = dia3D.getSimulator();

    checker.waitForThenStop(dia3D, widgets3D, 2000);
    QTest::qWait(2000);
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getSolverStopRequested() == true);
    REQUIRE(sim.getStepsBelowTolerance() < sim.getStepsToConvergence());
    REQUIRE(checker.messageContent == "Simulation stopped");
    REQUIRE(widgets3D.btnStartStop->text() == "Resume");
    REQUIRE(widgets3D.btnReset->isEnabled() == true);
    REQUIRE(dia3D.isRunning() == true);
    checker.stop();
    REQUIRE(dia3D.isRunning() == false);

    // resume simulation and run until convergence
    ModalChecker convChecker;
    convChecker.waitForConvergence(dia3D, widgets3D);

    REQUIRE(sim.hasConverged() == true);
    REQUIRE(sim.getStepsBelowTolerance() >= sim.getStepsToConvergence());
    REQUIRE(widgets3D.btnStartStop->text() == "Start");
    REQUIRE(widgets3D.btnReset->isEnabled() == true);
    convChecker.stop();
  }

  SECTION("run_stop_reset") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    const auto &sim = dia3D.getSimulator();
    ModalChecker checker;
    checker.waitForThenStop(dia3D, widgets3D, 2000);
    QTest::qWait(2000);

    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStepsBelowTolerance() < sim.getStepsToConvergence());
    REQUIRE(checker.messageContent == "Simulation stopped");
    REQUIRE(sim.getLatestError().load() < std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep().load() > 0.0);

    checker.stop();

    // reset everything
    sendMouseClick(widgets3D.btnReset);

    QTest::qWait(2000);

    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStepsBelowTolerance() == 0);
    REQUIRE(sim.getStepsToConvergence() == 10);
    REQUIRE(sim.getStopTolerance() == 1e-6);
    REQUIRE(sim.getDt() == 1);
    REQUIRE(sim.getTimeout() == 3600000); // 1 hour in milliseconds
    REQUIRE(sim.getLatestError().load() == std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep().load() == 0.0);

    QTest::qWait(1000);
  }

  SECTION("zslider_functionality") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    const auto &sim = dia3D.getSimulator();

    ModalChecker checker;
    checker.waitForConvergence(dia3D, widgets3D);
    checker.stop();

    // // slide the zaxis around
    widgets3D.zaxis->setValue(25); // interval: [0,49]

    QTest::qWait(2000);
    REQUIRE(widgets3D.zaxis->value() == 25);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->value() == 25);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->isVisible() == true);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->maximum() == 49);
    REQUIRE(widgets3D.valuesPlot->getZSlider()->minimum() == 0);
  }

  SECTION("run_then_open_dialogDisplayOptions") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();
    ModalChecker checker;
    checker.waitForThenStop(dia3D, widgets3D, 1500);
    checker.stop();

    DialogDisplayOptions *displayOptionsDialog = nullptr;

    // add this here to avoid blocking by dialog
    QTimer::singleShot(0, [&]() {
      while (displayOptionsDialog == nullptr) {
        for (QWidget *w : QApplication::topLevelWidgets()) {
          displayOptionsDialog =
              dia3D.findChild<DialogDisplayOptions *>("DialogDisplayOptions");
          if (displayOptionsDialog != nullptr &&
              displayOptionsDialog->isVisible()) {
            break;
          }
        }

        REQUIRE(displayOptionsDialog->isVisible() == true);
        displayOptionsDialog->accept();
        QTest::qWait(1000);
      }
    });

    // open display options dialog
    sendMouseClick(widgets3D.btnDisplayOptions);
  }

  SECTION("run_then_reject") {
    DialogSteadystate dia3D(model3D);
    DialogSteadystateWidgets widgets3D(&dia3D);
    dia3D.show();

    // add this here to avoid blocking by dialog
    QTimer::singleShot(2000, [&]() {
      sendMouseClick(widgets3D.buttonBox->button(QDialogButtonBox::Cancel));
      QTest::qWait(1000);
    });

    ModalChecker checker;
    checker.waitForConvergence(dia3D, widgets3D);
    checker.stop();
    QTest::qWait(1500);
    const auto &sim = dia3D.getSimulator();
    REQUIRE(sim.hasConverged() == false);
    REQUIRE(sim.getStepsBelowTolerance() < sim.getStepsToConvergence());
    REQUIRE(sim.getLatestError().load() < std::numeric_limits<double>::max());
    REQUIRE(sim.getLatestStep().load() > 0.0);
    REQUIRE(dia3D.isRunning() == false);
    REQUIRE(dia3D.isVisible() == false);
  }
}
