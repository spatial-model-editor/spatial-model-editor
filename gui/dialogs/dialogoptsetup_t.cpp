#include "catch_wrapper.hpp"
#include "dialogoptsetup.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>

using namespace sme;
using namespace sme::test;

TEST_CASE("DialogOptSetup", "[gui/dialogs/optsetup][gui/"
                            "dialogs][gui][optimize]") {
  auto model{getExampleModel(Mod::CircadianClock)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  SECTION("No initial OptimizeOptions") {
    DialogOptSetup dia(model);
    // get pointers to widgets within dialog
    auto *cmbAlgorithm{dia.findChild<QComboBox *>("cmbAlgorithm")};
    REQUIRE(cmbAlgorithm != nullptr);
    auto *spinIslands{dia.findChild<QSpinBox *>("spinIslands")};
    REQUIRE(spinIslands != nullptr);
    auto *spinPopulation{dia.findChild<QSpinBox *>("spinPopulation")};
    REQUIRE(spinPopulation != nullptr);
    auto *lstTargets{dia.findChild<QListWidget *>("lstTargets")};
    REQUIRE(lstTargets != nullptr);
    auto *lstParameters{dia.findChild<QListWidget *>("lstParameters")};
    REQUIRE(lstParameters != nullptr);
    auto *btnAddTarget{dia.findChild<QPushButton *>("btnAddTarget")};
    REQUIRE(btnAddTarget != nullptr);
    auto *btnEditTarget{dia.findChild<QPushButton *>("btnEditTarget")};
    REQUIRE(btnEditTarget != nullptr);
    auto *btnRemoveTarget{dia.findChild<QPushButton *>("btnRemoveTarget")};
    REQUIRE(btnRemoveTarget != nullptr);
    auto *btnAddParameter{dia.findChild<QPushButton *>("btnAddParameter")};
    REQUIRE(btnAddParameter != nullptr);
    auto *btnEditParameter{dia.findChild<QPushButton *>("btnEditParameter")};
    REQUIRE(btnEditParameter != nullptr);
    auto *btnRemoveParameter{
        dia.findChild<QPushButton *>("btnRemoveParameter")};
    REQUIRE(btnRemoveParameter != nullptr);
    ModalWidgetTimer mwt;
    SECTION("user does nothing") {
      REQUIRE(lstTargets->count() == 0);
      REQUIRE(btnAddTarget->isEnabled());
      REQUIRE(btnEditTarget->isEnabled() == false);
      REQUIRE(btnRemoveTarget->isEnabled() == false);
      REQUIRE(lstParameters->count() == 0);
      REQUIRE(btnAddParameter->isEnabled());
      REQUIRE(btnEditParameter->isEnabled() == false);
      REQUIRE(btnRemoveParameter->isEnabled() == false);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::PSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 1);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 2);
      REQUIRE(dia.getOptimizeOptions().optParams.empty());
      REQUIRE(dia.getOptimizeOptions().optCosts.empty());
    }
    SECTION("user modifies algorithm options") {
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::PSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 1);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 2);
      sendKeyEvents(cmbAlgorithm, {"Down"});
      sendKeyEvents(spinIslands, {"Delete", "Backspace", "1", "5", "Enter"});
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::GPSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 15);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 2);
      sendKeyEvents(spinPopulation, {"Delete", "Backspace", "3", "2", "Enter"});
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::GPSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 15);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 32);
    }
    SECTION("user adds targets") {
      dia.show();
      REQUIRE(lstTargets->count() == 0);
      REQUIRE(dia.getOptimizeOptions().optCosts.empty());
      // click add target, then cancel: no-op
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendMouseClick(btnAddTarget);
      REQUIRE(lstTargets->count() == 0);
      REQUIRE(dia.getOptimizeOptions().optCosts.empty());
      // click add target, change values, press ok
      mwt.addUserAction({"Down", "Tab", "Down", "Tab", "4", "Tab", "Tab",
                         "Down", "Tab", "2", "Enter"});
      mwt.start();
      sendMouseClick(btnAddTarget);
      REQUIRE(lstTargets->count() == 1);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/Mt [Rate of change of concentration at t=4, relative "
              "difference]");
      const auto &optCosts{dia.getOptimizeOptions().optCosts};
      REQUIRE(optCosts.size() == 1);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[0].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(4.0));
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 1);
      REQUIRE(optCosts[0].weight == 2);
      // add another target with default values
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnAddTarget);
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/Mt [Rate of change of concentration at t=4, relative "
              "difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[0].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(4.0));
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 1);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[1].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 0);
      // user edits first target, but clicks cancel
      sendKeyEvents(lstTargets, {"Up"});
      REQUIRE(lstTargets->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(btnEditTarget);
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/Mt [Rate of change of concentration at t=4, relative "
              "difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[0].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(4.0));
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 1);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[1].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 0);
      // user edits first target & changes target species (other target values
      // get reset to defaults)
      REQUIRE(lstTargets->currentRow() == 0);
      mwt.addUserAction({"Down", "Enter"});
      mwt.start();
      sendMouseClick(btnEditTarget);
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/Mp [Concentration at t=100, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 2);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[1].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 0);
      // user edits first target by double-clicking on it, changes opt cost type
      REQUIRE(lstTargets->currentRow() == 0);
      mwt.addUserAction({"Tab", "Down"});
      mwt.start();
      sendMouseDoubleClick(lstTargets->item(0));
      REQUIRE(mwt.getResult() == "Edit Optimization Target");
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/Mp [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 2);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[1].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 0);
      // user deletes first target, but clicks cancel
      REQUIRE(lstTargets->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 2);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[1].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 0);
      // user deletes first target
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      REQUIRE(lstTargets->count() == 1);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 1);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 0);
      // user deletes remaining target
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      REQUIRE(lstTargets->count() == 0);
      REQUIRE(optCosts.empty());
    }
    SECTION("user adds parameters") {
      REQUIRE(lstParameters->count() == 0);
      const auto &optParams{dia.getOptimizeOptions().optParams};
      REQUIRE(optParams.empty());
      // click add parameter, then cancel: no-op
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendMouseClick(btnAddParameter);
      REQUIRE(lstParameters->count() == 0);
      REQUIRE(optParams.empty());
      // click add parameter, change values, press ok
      mwt.addUserAction(
          {"Down", "Down", "Tab", "2", "Tab", "5", "Tab", "Enter"});
      mwt.start();
      sendMouseClick(btnAddParameter);
      REQUIRE(lstParameters->count() == 1);
      REQUIRE(lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(optParams.size() == 1);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[0].lowerBound == dbl_approx(2));
      REQUIRE(optParams[0].upperBound == dbl_approx(5));
      // add another parameter with default values
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnAddParameter);
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(lstParameters->item(1)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[0].lowerBound == dbl_approx(2));
      REQUIRE(optParams[0].upperBound == dbl_approx(5));
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "k");
      REQUIRE(optParams[1].parentId == "sP");
      REQUIRE(optParams[1].name == "Reaction 'sP' / k");
      REQUIRE(optParams[1].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[1].upperBound == dbl_approx(0.9));
      // user edits first parameter, but clicks cancel
      sendKeyEvents(lstParameters, {"Up"});
      REQUIRE(lstParameters->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(btnEditParameter);
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(lstParameters->item(1)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[0].lowerBound == dbl_approx(2));
      REQUIRE(optParams[0].upperBound == dbl_approx(5));
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "k");
      REQUIRE(optParams[1].parentId == "sP");
      REQUIRE(optParams[1].name == "Reaction 'sP' / k");
      REQUIRE(optParams[1].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[1].upperBound == dbl_approx(0.9));
      // user edits first parameter & changes parameter (lower/upper bounds get
      // reset to default for this param)
      REQUIRE(lstParameters->currentRow() == 0);
      mwt.addUserAction({"Down", "Enter"});
      mwt.start();
      sendMouseClick(btnEditParameter);
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [4, 4]");
      REQUIRE(lstParameters->item(1)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "n");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / n");
      REQUIRE(optParams[0].lowerBound == dbl_approx(4));
      REQUIRE(optParams[0].upperBound == dbl_approx(4));
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "k");
      REQUIRE(optParams[1].parentId == "sP");
      REQUIRE(optParams[1].name == "Reaction 'sP' / k");
      REQUIRE(optParams[1].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[1].upperBound == dbl_approx(0.9));
      // user edits first parameter by doubleclick & changes bound
      REQUIRE(lstParameters->currentRow() == 0);
      mwt.addUserAction({"Tab", "1", "Tab", "2", "Enter"});
      mwt.start();
      sendMouseDoubleClick(lstParameters->item(0));
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [1, 2]");
      REQUIRE(lstParameters->item(1)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "n");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / n");
      REQUIRE(optParams[0].lowerBound == dbl_approx(1));
      REQUIRE(optParams[0].upperBound == dbl_approx(2));
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "k");
      REQUIRE(optParams[1].parentId == "sP");
      REQUIRE(optParams[1].name == "Reaction 'sP' / k");
      REQUIRE(optParams[1].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[1].upperBound == dbl_approx(0.9));
      // user deletes first parameter, but clicks cancel
      REQUIRE(lstParameters->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "n");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / n");
      REQUIRE(optParams[0].lowerBound == dbl_approx(1));
      REQUIRE(optParams[0].upperBound == dbl_approx(2));
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "k");
      REQUIRE(optParams[1].parentId == "sP");
      REQUIRE(optParams[1].name == "Reaction 'sP' / k");
      REQUIRE(optParams[1].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[1].upperBound == dbl_approx(0.9));
      // user deletes first parameter
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 1);
      REQUIRE(lstParameters->item(0)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 1);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "k");
      REQUIRE(optParams[0].parentId == "sP");
      REQUIRE(optParams[0].name == "Reaction 'sP' / k");
      REQUIRE(optParams[0].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[0].upperBound == dbl_approx(0.9));
      // user deletes remaining parameter
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 0);
      REQUIRE(optParams.empty());
    }
  }
  SECTION("Given existing OptimizeOptions") {
    sme::simulate::OptimizeOptions optimizeOptions{};
    optimizeOptions.optAlgorithm.islands = 5;
    optimizeOptions.optAlgorithm.population = 17;
    optimizeOptions.optParams.push_back(
        {sme::simulate::OptParamType::ReactionParameter, "Mp_transcription_K",
         "K", "Mp_transcription", 1, 2});
    optimizeOptions.optParams.push_back(
        {sme::simulate::OptParamType::ReactionParameter, "Mp_transcription_n",
         "n", "Mp_transcription", 0.2, 0.4});
    auto &optCost0{optimizeOptions.optCosts.emplace_back()};
    optCost0.optCostType = sme::simulate::OptCostType::Concentration;
    optCost0.optCostDiffType = sme::simulate::OptCostDiffType::Absolute;
    optCost0.name = "cell/P0";
    optCost0.id = "P0";
    optCost0.simulationTime = 18.0;
    optCost0.weight = 0.23;
    optCost0.compartmentIndex = 0;
    optCost0.speciesIndex = 0;
    optCost0.targetValues = {};
    auto &optCost1{optimizeOptions.optCosts.emplace_back()};
    optCost1.optCostType = sme::simulate::OptCostType::ConcentrationDcdt;
    optCost1.optCostDiffType = sme::simulate::OptCostDiffType::Relative;
    optCost1.name = "cell/T0";
    optCost1.id = "T0";
    optCost1.simulationTime = 9.0;
    optCost1.weight = 0.99;
    optCost1.compartmentIndex = 0;
    optCost1.speciesIndex = 3;
    optCost1.targetValues = {};
    model.getOptimizeOptions() = optimizeOptions;
    DialogOptSetup dia(model);
    const auto &optParams{dia.getOptimizeOptions().optParams};
    const auto &optCosts{dia.getOptimizeOptions().optCosts};
    // get pointers to widgets within tab
    auto *lstTargets{dia.findChild<QListWidget *>("lstTargets")};
    REQUIRE(lstTargets != nullptr);
    auto *lstParameters{dia.findChild<QListWidget *>("lstParameters")};
    REQUIRE(lstParameters != nullptr);
    auto *btnAddTarget{dia.findChild<QPushButton *>("btnAddTarget")};
    REQUIRE(btnAddTarget != nullptr);
    auto *btnEditTarget{dia.findChild<QPushButton *>("btnEditTarget")};
    REQUIRE(btnEditTarget != nullptr);
    auto *btnRemoveTarget{dia.findChild<QPushButton *>("btnRemoveTarget")};
    REQUIRE(btnRemoveTarget != nullptr);
    auto *btnAddParameter{dia.findChild<QPushButton *>("btnAddParameter")};
    REQUIRE(btnAddParameter != nullptr);
    auto *btnEditParameter{dia.findChild<QPushButton *>("btnEditParameter")};
    REQUIRE(btnEditParameter != nullptr);
    auto *btnRemoveParameter{
        dia.findChild<QPushButton *>("btnRemoveParameter")};
    REQUIRE(btnRemoveParameter != nullptr);
    ModalWidgetTimer mwt;
    SECTION("user does nothing") {
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(18.0));
      REQUIRE(optCosts[0].name == "cell/P0");
      REQUIRE(optCosts[0].id == "P0");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 0);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].id == "T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      REQUIRE(btnAddTarget->isEnabled());
      REQUIRE(btnEditTarget->isEnabled() == false);
      REQUIRE(btnRemoveTarget->isEnabled() == false);
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(lstParameters->item(0)->text() == "Mp_transcription_K [1, 2]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Mp_transcription_K");
      REQUIRE(optParams[0].lowerBound == 1);
      REQUIRE(optParams[0].upperBound == 2);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      REQUIRE(btnAddParameter->isEnabled());
      REQUIRE(btnEditParameter->isEnabled() == false);
      REQUIRE(btnRemoveParameter->isEnabled() == false);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::PSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 5);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 17);
    }
    SECTION("user adds targets") {
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(18.0));
      REQUIRE(optCosts[0].name == "cell/P0");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 0);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      // click add target, then cancel: no-op
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendMouseClick(btnAddTarget);
      REQUIRE(lstTargets->count() == 2);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(optCosts.size() == 2);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(18.0));
      REQUIRE(optCosts[0].name == "cell/P0");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 0);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      // click add target, change values, press ok
      mwt.addUserAction({"Down", "Tab", "Down", "Tab", "Enter"});
      mwt.start();
      sendMouseClick(btnAddTarget);
      REQUIRE(lstTargets->count() == 3);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(optCosts.size() == 3);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(18.0));
      REQUIRE(optCosts[0].name == "cell/P0");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 0);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      REQUIRE(optCosts[2].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[2].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[2].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[2].compartmentIndex == 0);
      REQUIRE(optCosts[2].speciesIndex == 1);
      // add another target with default values
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnAddTarget);
      REQUIRE(lstTargets->count() == 4);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(lstTargets->item(3)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 4);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(18.0));
      REQUIRE(optCosts[0].name == "cell/P0");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 0);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      REQUIRE(optCosts[2].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[2].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[2].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[2].compartmentIndex == 0);
      REQUIRE(optCosts[2].speciesIndex == 1);
      REQUIRE(optCosts[3].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[3].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[3].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[3].compartmentIndex == 0);
      REQUIRE(optCosts[3].speciesIndex == 0);
      // user edits first target, but clicks cancel
      sendKeyEvents(lstTargets, {"Up"});
      REQUIRE(lstTargets->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(btnEditTarget);
      REQUIRE(lstTargets->count() == 4);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(lstTargets->item(3)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 4);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(18.0));
      REQUIRE(optCosts[0].name == "cell/P0");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 0);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      REQUIRE(optCosts[2].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[2].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[2].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[2].compartmentIndex == 0);
      REQUIRE(optCosts[2].speciesIndex == 1);
      REQUIRE(optCosts[3].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[3].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[3].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[3].compartmentIndex == 0);
      REQUIRE(optCosts[3].speciesIndex == 0);
      // user edits first target & changes target species (other target values
      // get reset to defaults)
      REQUIRE(lstTargets->currentRow() == 0);
      mwt.addUserAction({"Down", "Enter"});
      mwt.start();
      sendMouseClick(btnEditTarget);
      REQUIRE(lstTargets->count() == 4);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/Mt [Concentration at t=100, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(lstTargets->item(3)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 4);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[0].id == "Mt");
      REQUIRE(optCosts[0].name == "cell/Mt");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 1);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].id == "T0");
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      REQUIRE(optCosts[2].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[2].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[2].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[2].id == "Mt");
      REQUIRE(optCosts[2].compartmentIndex == 0);
      REQUIRE(optCosts[2].speciesIndex == 1);
      REQUIRE(optCosts[3].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[3].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[3].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[3].id == "P0");
      REQUIRE(optCosts[3].compartmentIndex == 0);
      REQUIRE(optCosts[3].speciesIndex == 0);
      // user deletes first target, but clicks cancel
      REQUIRE(lstTargets->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      REQUIRE(lstTargets->count() == 4);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/Mt [Concentration at t=100, absolute difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(lstTargets->item(3)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 4);
      REQUIRE(optCosts[0].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[0].id == "Mt");
      REQUIRE(optCosts[0].name == "cell/Mt");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 1);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[1].id == "T0");
      REQUIRE(optCosts[1].name == "cell/T0");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 3);
      REQUIRE(optCosts[2].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[2].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[2].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[2].id == "Mt");
      REQUIRE(optCosts[2].compartmentIndex == 0);
      REQUIRE(optCosts[2].speciesIndex == 1);
      REQUIRE(optCosts[3].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[3].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[3].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[3].id == "P0");
      REQUIRE(optCosts[3].compartmentIndex == 0);
      REQUIRE(optCosts[3].speciesIndex == 0);
      // user deletes first target
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      REQUIRE(lstTargets->count() == 3);
      REQUIRE(lstTargets->item(0)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(lstTargets->item(1)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(lstTargets->item(2)->text() ==
              "cell/P0 [Concentration at t=100, absolute difference]");
      REQUIRE(optCosts.size() == 3);
      REQUIRE(optCosts[0].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[0].optCostDiffType ==
              simulate::OptCostDiffType::Relative);
      REQUIRE(optCosts[0].simulationTime == dbl_approx(9.0));
      REQUIRE(optCosts[0].id == "T0");
      REQUIRE(optCosts[0].name == "cell/T0");
      REQUIRE(optCosts[0].compartmentIndex == 0);
      REQUIRE(optCosts[0].speciesIndex == 3);
      REQUIRE(optCosts[1].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[1].optCostType ==
              simulate::OptCostType::ConcentrationDcdt);
      REQUIRE(optCosts[1].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[1].id == "Mt");
      REQUIRE(optCosts[1].compartmentIndex == 0);
      REQUIRE(optCosts[1].speciesIndex == 1);
      REQUIRE(optCosts[2].optCostDiffType ==
              simulate::OptCostDiffType::Absolute);
      REQUIRE(optCosts[2].optCostType == simulate::OptCostType::Concentration);
      REQUIRE(optCosts[2].simulationTime == dbl_approx(100.0));
      REQUIRE(optCosts[2].id == "P0");
      REQUIRE(optCosts[2].compartmentIndex == 0);
      REQUIRE(optCosts[2].speciesIndex == 0);
      // user deletes remaining targets
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveTarget);
      REQUIRE(lstTargets->count() == 0);
      REQUIRE(optCosts.empty());
    }
    SECTION("user adds parameters") {
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(lstParameters->item(0)->text() == "Mp_transcription_K [1, 2]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Mp_transcription_K");
      REQUIRE(optParams[0].lowerBound == 1);
      REQUIRE(optParams[0].upperBound == 2);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      // click add parameter, then cancel: no-op
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendMouseClick(btnAddParameter);
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(lstParameters->item(0)->text() == "Mp_transcription_K [1, 2]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(optParams.size() == 2);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Mp_transcription_K");
      REQUIRE(optParams[0].lowerBound == 1);
      REQUIRE(optParams[0].upperBound == 2);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      // click add parameter, change values, press ok
      mwt.addUserAction(
          {"Down", "Down", "Tab", "2", "Tab", "5", "Tab", "Enter"});
      mwt.start();
      sendMouseClick(btnAddParameter);
      REQUIRE(lstParameters->count() == 3);
      REQUIRE(lstParameters->item(0)->text() == "Mp_transcription_K [1, 2]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(optParams.size() == 3);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Mp_transcription_K");
      REQUIRE(optParams[0].lowerBound == 1);
      REQUIRE(optParams[0].upperBound == 2);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      REQUIRE(optParams[2].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[2].id == "K");
      REQUIRE(optParams[2].parentId == "Mp_transcription");
      REQUIRE(optParams[2].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[2].lowerBound == 2);
      REQUIRE(optParams[2].upperBound == 5);
      // add another parameter with default values
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnAddParameter);
      REQUIRE(lstParameters->count() == 4);
      REQUIRE(lstParameters->item(0)->text() == "Mp_transcription_K [1, 2]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(lstParameters->item(3)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 4);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Mp_transcription_K");
      REQUIRE(optParams[0].lowerBound == 1);
      REQUIRE(optParams[0].upperBound == 2);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      REQUIRE(optParams[2].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[2].id == "K");
      REQUIRE(optParams[2].parentId == "Mp_transcription");
      REQUIRE(optParams[2].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[2].lowerBound == 2);
      REQUIRE(optParams[2].upperBound == 5);
      REQUIRE(optParams[3].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[3].id == "k");
      REQUIRE(optParams[3].parentId == "sP");
      REQUIRE(optParams[3].name == "Reaction 'sP' / k");
      REQUIRE(optParams[3].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[3].upperBound == dbl_approx(0.9));
      // user edits first parameter, but clicks cancel
      sendKeyEvents(lstParameters, {"Up"});
      REQUIRE(lstParameters->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(btnEditParameter);
      REQUIRE(lstParameters->count() == 4);
      REQUIRE(lstParameters->item(0)->text() == "Mp_transcription_K [1, 2]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(lstParameters->item(3)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 4);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "K");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Mp_transcription_K");
      REQUIRE(optParams[0].lowerBound == 1);
      REQUIRE(optParams[0].upperBound == 2);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      REQUIRE(optParams[2].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[2].id == "K");
      REQUIRE(optParams[2].parentId == "Mp_transcription");
      REQUIRE(optParams[2].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[2].lowerBound == 2);
      REQUIRE(optParams[2].upperBound == 5);
      REQUIRE(optParams[3].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[3].id == "k");
      REQUIRE(optParams[3].parentId == "sP");
      REQUIRE(optParams[3].name == "Reaction 'sP' / k");
      REQUIRE(optParams[3].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[3].upperBound == dbl_approx(0.9));
      // user edits first parameter & changes parameter (lower/upper bounds get
      // reset to default for this param)
      REQUIRE(lstParameters->currentRow() == 0);
      mwt.addUserAction({"Down", "Enter"});
      mwt.start();
      sendMouseClick(btnEditParameter);
      REQUIRE(lstParameters->count() == 4);
      REQUIRE(lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [4, 4]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(lstParameters->item(3)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 4);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "n");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / n");
      REQUIRE(optParams[0].lowerBound == 4);
      REQUIRE(optParams[0].upperBound == 4);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      REQUIRE(optParams[2].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[2].id == "K");
      REQUIRE(optParams[2].parentId == "Mp_transcription");
      REQUIRE(optParams[2].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[2].lowerBound == 2);
      REQUIRE(optParams[2].upperBound == 5);
      REQUIRE(optParams[3].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[3].id == "k");
      REQUIRE(optParams[3].parentId == "sP");
      REQUIRE(optParams[3].name == "Reaction 'sP' / k");
      REQUIRE(optParams[3].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[3].upperBound == dbl_approx(0.9));
      // user deletes first parameter, but clicks cancel
      REQUIRE(lstParameters->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 4);
      REQUIRE(lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [4, 4]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(lstParameters->item(3)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 4);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "n");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Reaction 'Mp transcription' / n");
      REQUIRE(optParams[0].lowerBound == 4);
      REQUIRE(optParams[0].upperBound == 4);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "n");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Mp_transcription_n");
      REQUIRE(optParams[1].lowerBound == 0.2);
      REQUIRE(optParams[1].upperBound == 0.4);
      REQUIRE(optParams[2].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[2].id == "K");
      REQUIRE(optParams[2].parentId == "Mp_transcription");
      REQUIRE(optParams[2].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[2].lowerBound == 2);
      REQUIRE(optParams[2].upperBound == 5);
      REQUIRE(optParams[3].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[3].id == "k");
      REQUIRE(optParams[3].parentId == "sP");
      REQUIRE(optParams[3].name == "Reaction 'sP' / k");
      REQUIRE(optParams[3].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[3].upperBound == dbl_approx(0.9));
      // user deletes first parameter
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 3);
      REQUIRE(lstParameters->item(0)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(lstParameters->item(1)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(lstParameters->item(2)->text() == "Reaction 'sP' / k [0.9, 0.9]");
      REQUIRE(optParams.size() == 3);
      REQUIRE(optParams[0].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[0].id == "n");
      REQUIRE(optParams[0].parentId == "Mp_transcription");
      REQUIRE(optParams[0].name == "Mp_transcription_n");
      REQUIRE(optParams[0].lowerBound == 0.2);
      REQUIRE(optParams[0].upperBound == 0.4);
      REQUIRE(optParams[1].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[1].id == "K");
      REQUIRE(optParams[1].parentId == "Mp_transcription");
      REQUIRE(optParams[1].name == "Reaction 'Mp transcription' / K");
      REQUIRE(optParams[1].lowerBound == 2);
      REQUIRE(optParams[1].upperBound == 5);
      REQUIRE(optParams[2].optParamType ==
              simulate::OptParamType::ReactionParameter);
      REQUIRE(optParams[2].id == "k");
      REQUIRE(optParams[2].parentId == "sP");
      REQUIRE(optParams[2].name == "Reaction 'sP' / k");
      REQUIRE(optParams[2].lowerBound == dbl_approx(0.9));
      REQUIRE(optParams[2].upperBound == dbl_approx(0.9));
      // user deletes remaining parameters
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 2);
      REQUIRE(optParams.size() == 2);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 1);
      REQUIRE(optParams.size() == 1);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveParameter);
      REQUIRE(lstParameters->count() == 0);
      REQUIRE(optParams.empty());
    }
  }
  SECTION("DUNE simulator: nthreads = 1") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/800
    auto model{getExampleModel(Mod::CircadianClock)};
    model.getSimulationSettings().simulatorType =
        sme::simulate::SimulatorType::DUNE;
    model.getOptimizeOptions().optAlgorithm.islands = 4;
    DialogOptSetup dia(model);
    auto *spinIslands{dia.findChild<QSpinBox *>("spinIslands")};
    REQUIRE(spinIslands != nullptr);
    REQUIRE(spinIslands->value() == 1);
    REQUIRE(spinIslands->maximum() == 1);
    REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 1);
  }
}
