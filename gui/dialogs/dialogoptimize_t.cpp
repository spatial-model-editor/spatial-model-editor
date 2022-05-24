#include "catch_wrapper.hpp"
#include "dialogoptimize.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include <QPushButton>

using namespace sme;
using namespace sme::test;

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
  ModalWidgetTimer mwt;
  SECTION("no optimizeOptions") {
    model.getOptimizeOptions() = {};
    DialogOptimize dia(model);
    // get pointers to widgets within dialog
    auto *btnStart{dia.findChild<QPushButton *>("btnStart")};
    REQUIRE(btnStart != nullptr);
    auto *btnStop{dia.findChild<QPushButton *>("btnStop")};
    REQUIRE(btnStop != nullptr);
    auto *btnSetup{dia.findChild<QPushButton *>("btnSetup")};
    REQUIRE(btnSetup != nullptr);
    auto *btnApplyToModel{dia.findChild<QPushButton *>("btnApplyToModel")};
    REQUIRE(btnApplyToModel != nullptr);
    // no valid initial optimize options
    REQUIRE(btnStart->isEnabled() == false);
    REQUIRE(btnStop->isEnabled() == false);
    REQUIRE(btnSetup->isEnabled() == true);
    REQUIRE(btnApplyToModel->isEnabled() == false);
  }
  SECTION("existing optimizeOptions") {
    DialogOptimize dia(model);
    // get pointers to widgets within dialog
    auto *btnStart{dia.findChild<QPushButton *>("btnStart")};
    REQUIRE(btnStart != nullptr);
    auto *btnStop{dia.findChild<QPushButton *>("btnStop")};
    REQUIRE(btnStop != nullptr);
    auto *btnSetup{dia.findChild<QPushButton *>("btnSetup")};
    REQUIRE(btnSetup != nullptr);
    auto *btnApplyToModel{dia.findChild<QPushButton *>("btnApplyToModel")};
    REQUIRE(btnApplyToModel != nullptr);
    // valid initial optimize options
    REQUIRE(btnStart->isEnabled() == true);
    REQUIRE(btnStop->isEnabled() == false);
    REQUIRE(btnSetup->isEnabled() == true);
    REQUIRE(btnApplyToModel->isEnabled() == false);
  }
}
