#include "../widgets/qlabelmousetracker.hpp"
#include "../widgets/qvoxelrenderer.hpp"
#include "catch_wrapper.hpp"
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

TEST_CASE("DialogSteadyState", "[gui/dialogs/steadystate][gui/"
                               "dialogs][gui][steadystate]") {
  auto model{getExampleModel(Mod::ABtoC)};
  auto model3D{getExampleModel(Mod::GrayScott3D)};
  DialogSteadystate dia(model);
  DialogSteadystate dia3D(model3D);
  DialogSteadystateWidgets widgets(&dia);
  DialogSteadystateWidgets widgets3D(&dia3D);
  dia.show();
  QTest::qWait(100);

  SECTION("initialization 2D") {
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
    dia3D.show();
    QTest::qWait(100);
    REQUIRE(widgets3D.zaxis->minimum() == 0);
    REQUIRE(widgets3D.zaxis->maximum() == 49);
    REQUIRE(widgets3D.zaxis->value() == 0);
    REQUIRE(widgets3D.cmbPlotting->isEnabled() == true);
    REQUIRE(widgets3D.cmbPlotting->currentText() == "2D");
    REQUIRE(widgets3D.valuesPlot3D->isVisible() == false);
    REQUIRE(widgets3D.valuesPlot->isVisible() == true);
  }

  SECTION("set parameters") {
    widgets.toleranceInput->setText("1e-5");
    widgets.convIntervalInput->setText("0.1");
    widgets.timeoutInput->setText("10");
    widgets.tolStepInput->setText("5");
    widgets.cmbConvergence->setCurrentText("Relative");
    widgets.cmbPlotting->setCurrentText("3D");
    widgets.zaxis->setValue(10);

    QTest::qWait(100);

    const auto &sim = dia.getSimulator();

    REQUIRE(sim.getStepsToConvergence() == 5);
    REQUIRE(sim.getStopTolerance() == 1e-5);
    REQUIRE(sim.getDt() == 0.1);
    REQUIRE(sim.getTimeout() == 10000); // 10 seconds in milliseconds
    REQUIRE(sim.getConvergenceMode() ==
            sme::simulate::SteadystateConvergenceMode::relative);
    REQUIRE(sim.getSimulatorType() == sme::simulate::SimulatorType::Pixel);
  }
}
