#include "catch_wrapper.hpp"
#include "dialogoptimize.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <qtabwidget.h>
#include <qwidget.h>

using namespace sme;
using namespace sme::test;

struct DialogOptimizeWidgets {
  explicit DialogOptimizeWidgets(const DialogOptimize *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbTarget);
    GET_DIALOG_WIDGET(QPushButton, btnSetup);
    GET_DIALOG_WIDGET(QPushButton, btnStartStop);
    GET_DIALOG_WIDGET(QComboBox, cmbMode);
    GET_DIALOG_WIDGET(QLabel, lblResultLabel);
    GET_DIALOG_WIDGET(QLabel, lblTargetLabel);
    GET_DIALOG_WIDGET(QSlider, zaxis);
    GET_DIALOG_WIDGET(QWidget, lblTarget);
    GET_DIALOG_WIDGET(QWidget, lblResult);
    GET_DIALOG_WIDGET(QWidget, lblTarget3D);
    GET_DIALOG_WIDGET(QWidget, lblResult3D);
    GET_DIALOG_WIDGET(QLabel, lblDifferenceLabel);
    GET_DIALOG_WIDGET(QWidget, lblDifference);
    GET_DIALOG_WIDGET(QWidget, lblDifference3D);
    GET_DIALOG_WIDGET(QTabWidget, tabWidget);
  }
  QComboBox *cmbTarget;
  QTabWidget *tabWidget;
  QPushButton *btnSetup;
  QPushButton *btnStartStop;
  QComboBox *cmbMode;
  QLabel *lblResultLabel;
  QLabel *lblTargetLabel;
  QSlider *zaxis;
  QWidget *lblTarget;
  QWidget *lblResult;
  QWidget *lblTarget3D;
  QWidget *lblResult3D;
  QLabel *lblDifferenceLabel;
  QWidget *lblDifference;
  QWidget *lblDifference3D;
};

TEST_CASE("DialogOptimize", "[gui/dialogs/optcost][gui/"
                            "dialogs][gui][optimize]") {
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::OptimizeOptions optimizeOptions{};
  optimizeOptions.optParams.push_back(
      {sme::simulate::OptParamType::ReactionParameter, "name", "k1", "r1", 0.05,
       0.21});
  // optimization cost: absolute difference of concentration of species A from
  // zero after simulation time of 2
  optimizeOptions.optCosts.push_back({sme::simulate::OptCostType::Concentration,
                                      simulate::OptCostDiffType::Absolute,
                                      "name",
                                      "A",
                                      2.0,
                                      1.0,
                                      0,
                                      0,
                                      {}});
  model.getOptimizeOptions() = optimizeOptions;

  SECTION("no optimizeOptions") {
    model.getOptimizeOptions() = {};
    DialogOptimize dia(model);
    DialogOptimizeWidgets widgets(&dia);
    dia.show();
    QTest::qWait(100);
    // no valid initial optimize options
    REQUIRE(widgets.btnStartStop->text() == "Start");
    REQUIRE(widgets.btnStartStop->isEnabled() == false);
    REQUIRE(widgets.btnSetup->isEnabled() == true);
    REQUIRE(dia.getParametersString().isEmpty());
  }
  SECTION("existing optimizeOptions") {
    DialogOptimize dia(model);
    DialogOptimizeWidgets widgets(&dia);
    dia.show();
    QTest::qWait(100);
    // valid initial optimize options
    REQUIRE(widgets.btnStartStop->isEnabled() == true);
    REQUIRE(widgets.btnSetup->isEnabled() == true);
    // no initial parameters
    REQUIRE(dia.getParametersString().isEmpty());
    // evolve params for 3secs
    sendMouseClick(widgets.btnStartStop);
    wait(3000);
    sendMouseClick(widgets.btnStartStop);
    auto lines{dia.getParametersString().split("\n")};
    REQUIRE(lines.size() == 1);
  }
  SECTION("differenceMode") {
    // DialogOptimize dia(model);
    // DialogOptimizeWidgets widgets(&dia);
    // dia.show();
    // QTest::qWait(100); // Ensure the dialog is fully shown
    // REQUIRE(widgets.diffMode->isCheckable() == true);
    // // evolve params for 3secs
    // sendMouseClick(widgets.btnStartStop);
    // wait(3000);
    // sendMouseClick(widgets.btnStartStop);

    // REQUIRE(widgets.lblResult->isVisible() == true);
    // REQUIRE(widgets.lblTarget->isVisible() == true);
    // REQUIRE(widgets.lblTargetLabel->text() == "Target values:");
    // REQUIRE(widgets.lblTarget3D->isVisible() == false);
    // REQUIRE(widgets.lblResult3D->isVisible() == false);
    // REQUIRE(dia.getDifferenceMode() == false);
    // REQUIRE(widgets.diffMode->isChecked() == false);
    // REQUIRE(widgets.diffMode->isEnabled() == true);
    // // TODO: check that the normal image is displayed
    // widgets.diffMode->click();
    // REQUIRE(widgets.diffMode->isChecked() == true);
    // REQUIRE(dia.getDifferenceMode() == true);
    // REQUIRE(dia.getVizMode() == DialogOptimize::VizMode::_2D);
    // REQUIRE(widgets.lblResultLabel->isVisible() == false);
    // REQUIRE(widgets.lblResult->isVisible() == false);
    // REQUIRE(widgets.lblTarget->isVisible() == true);
    // REQUIRE(widgets.lblTargetLabel->text() ==
    //         "Difference between estimated and target image:");
    // REQUIRE(widgets.lblTarget3D->isVisible() == false);
    // REQUIRE(widgets.lblResult3D->isVisible() == false);
    // // TODO: check that the diff image is displayed
  }

  SECTION("visualization mode") {
    DialogOptimize dia(model);
    DialogOptimizeWidgets widgets(&dia);
    dia.show();
    // evolve params for 3secs
    sendMouseClick(widgets.btnStartStop);
    wait(3000);
    sendMouseClick(widgets.btnStartStop);
    REQUIRE(dia.getVizMode() == DialogOptimize::VizMode::_2D);
    REQUIRE(widgets.lblResult->isVisible() == true);
    REQUIRE(widgets.lblResult3D->isVisible() == false);
    REQUIRE(widgets.lblTarget->isVisible() == true);
    REQUIRE(widgets.lblTarget3D->isVisible() == false);
    REQUIRE(widgets.lblDifference->isVisible() == true);
    REQUIRE(widgets.lblDifference3D->isVisible() == false);
    widgets.cmbMode->setCurrentIndex(1); // switch to 3D visualization

    REQUIRE(dia.getVizMode() == DialogOptimize::VizMode::_3D);
    REQUIRE(widgets.lblResult->isVisible() == false);
    REQUIRE(widgets.lblResult3D->isVisible() == true);
    REQUIRE(widgets.lblTarget->isVisible() == false);
    REQUIRE(widgets.lblTarget3D->isVisible() == true);
    REQUIRE(widgets.lblDifference->isVisible() == false);
    REQUIRE(widgets.lblDifference3D->isVisible() == true);
    widgets.cmbMode->setCurrentIndex(0); // switch back to 2D visualization

    REQUIRE(widgets.lblResult->isVisible() == true);
    REQUIRE(widgets.lblResult3D->isVisible() == false);

    REQUIRE(widgets.lblDifference->isVisible() == true);
    REQUIRE(widgets.lblDifference3D->isVisible() == false);

    REQUIRE(widgets.lblTarget->isVisible() == true);
    REQUIRE(widgets.lblTarget3D->isVisible() == false);
  }

  SECTION("visualization mode interacts with difference mode") {
    // DialogOptimize dia(model);
    // DialogOptimizeWidgets widgets(&dia);
    // dia.show();
    // // evolve params for 3secs
    // sendMouseClick(widgets.btnStartStop);
    // wait(3000);
    // sendMouseClick(widgets.btnStartStop);

    // REQUIRE(dia.getVizMode() == DialogOptimize::VizMode::_2D);
    // REQUIRE(dia.getDifferenceMode() == false);
    // REQUIRE(widgets.lblResult->isVisible() == true);
    // REQUIRE(widgets.lblResult3D->isVisible() == false);
    // REQUIRE(widgets.lblTarget->isVisible() == true);
    // REQUIRE(widgets.lblTarget3D->isVisible() == false);

    // widgets.cmbMode->setCurrentIndex(1); // switch to 3D visualization
    // REQUIRE(dia.getVizMode() == DialogOptimize::VizMode::_3D);
    // REQUIRE(widgets.lblResult->isVisible() == false);
    // REQUIRE(widgets.lblResult3D->isVisible() == true);
    // REQUIRE(widgets.lblTarget->isVisible() == false);
    // REQUIRE(widgets.lblTarget3D->isVisible() == true);

    // widgets.diffMode->click();
    // REQUIRE(dia.getDifferenceMode() == true);
    // REQUIRE(widgets.lblResult->isVisible() == false);
    // REQUIRE(widgets.lblResult3D->isVisible() == false);
    // REQUIRE(widgets.lblTargetLabel->text() ==
    //         "Difference between estimated and target image:");
    // REQUIRE(widgets.lblTarget->isVisible() == false);
    // REQUIRE(widgets.lblTarget3D->isVisible() == true);

    // widgets.cmbMode->setCurrentIndex(0); // switch back to 2D visualization
    // REQUIRE(dia.getVizMode() == DialogOptimize::VizMode::_2D);
    // REQUIRE(dia.getDifferenceMode() == true);
    // REQUIRE(widgets.lblResult->isVisible() == false);
    // REQUIRE(widgets.lblResult3D->isVisible() == false);
    // REQUIRE(widgets.lblTarget->isVisible() == true);
    // REQUIRE(widgets.lblTarget3D->isVisible() == false);

    // widgets.diffMode->click(); // switch off difference mode
    // REQUIRE(dia.getVizMode() == DialogOptimize::VizMode::_2D);
    // REQUIRE(widgets.lblResult->isVisible() == true);
    // REQUIRE(widgets.lblResult3D->isVisible() == false);
    // REQUIRE(widgets.lblTarget->isVisible() == true);
    // REQUIRE(widgets.lblTarget3D->isVisible() == false);
    // REQUIRE(widgets.lblTargetLabel->text() == "Target values:");
  }
}
