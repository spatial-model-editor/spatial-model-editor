#include "catch_wrapper.hpp"
#include "dialogoptimize.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include <QComboBox>
#include <QPushButton>

using namespace sme;
using namespace sme::test;

struct DialogOptimizeWidgets {
  explicit DialogOptimizeWidgets(const DialogOptimize *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbTarget);
    GET_DIALOG_WIDGET(QPushButton, btnSetup);
    GET_DIALOG_WIDGET(QPushButton, btnStartStop);
  }
  QComboBox *cmbTarget;
  QPushButton *btnSetup;
  QPushButton *btnStartStop;
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
}
