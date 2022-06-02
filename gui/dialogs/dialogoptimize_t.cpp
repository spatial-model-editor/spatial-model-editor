#include "catch_wrapper.hpp"
#include "dialogoptimize.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include <QPushButton>

using namespace sme;
using namespace sme::test;

TEST_CASE("DialogOptimize", "[gui/dialogs/optcost][gui/"
                            "dialogs][gui][optimize][Q]") {
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
  ModalWidgetTimer mwt;
  SECTION("no optimizeOptions") {
    model.getOptimizeOptions() = {};
    DialogOptimize dia(model);
    // get pointers to widgets within dialog
    auto *btnStartStop{dia.findChild<QPushButton *>("btnStartStop")};
    REQUIRE(btnStartStop != nullptr);
    auto *btnSetup{dia.findChild<QPushButton *>("btnSetup")};
    REQUIRE(btnSetup != nullptr);
    // no valid initial optimize options
    REQUIRE(btnStartStop->text() == "Start");
    REQUIRE(btnStartStop->isEnabled() == false);
    REQUIRE(btnSetup->isEnabled() == true);
    REQUIRE(dia.getParametersString().isEmpty());
  }
  SECTION("existing optimizeOptions") {
    DialogOptimize dia(model);
    // get pointers to widgets within dialog
    auto *btnStartStop{dia.findChild<QPushButton *>("btnStartStop")};
    REQUIRE(btnStartStop != nullptr);
    auto *btnSetup{dia.findChild<QPushButton *>("btnSetup")};
    REQUIRE(btnSetup != nullptr);
    // valid initial optimize options
    REQUIRE(btnStartStop->isEnabled() == true);
    REQUIRE(btnSetup->isEnabled() == true);
    // no initial parameters
    REQUIRE(dia.getParametersString().isEmpty());
    // evolve params for 3secs
    sendMouseClick(btnStartStop);
    wait(3000);
    sendMouseClick(btnStartStop);
    auto lines{dia.getParametersString().split("\n")};
    REQUIRE(lines.size() == 1);
  }
}
