#include "catch_wrapper.hpp"
#include "dialogoptsetup.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>

using namespace sme;
using namespace sme::test;

struct DialogOptSetupWidgets {
  explicit DialogOptSetupWidgets(const DialogOptSetup *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbAlgorithm);
    GET_DIALOG_WIDGET(QSpinBox, spinIslands);
    GET_DIALOG_WIDGET(QSpinBox, spinPopulation);
    GET_DIALOG_WIDGET(QListWidget, lstTargets);
    GET_DIALOG_WIDGET(QPushButton, btnAddTarget);
    GET_DIALOG_WIDGET(QPushButton, btnEditTarget);
    GET_DIALOG_WIDGET(QPushButton, btnRemoveTarget);
    GET_DIALOG_WIDGET(QListWidget, lstParameters);
    GET_DIALOG_WIDGET(QPushButton, btnAddParameter);
    GET_DIALOG_WIDGET(QPushButton, btnEditParameter);
    GET_DIALOG_WIDGET(QPushButton, btnRemoveParameter);
  }
  QComboBox *cmbAlgorithm;
  QSpinBox *spinIslands;
  QSpinBox *spinPopulation;
  QListWidget *lstTargets;
  QPushButton *btnAddTarget;
  QPushButton *btnEditTarget;
  QPushButton *btnRemoveTarget;
  QListWidget *lstParameters;
  QPushButton *btnAddParameter;
  QPushButton *btnEditParameter;
  QPushButton *btnRemoveParameter;
};

TEST_CASE("DialogOptSetup", "[gui/dialogs/optsetup][gui/"
                            "dialogs][gui][optimize]") {
  auto model{getExampleModel(Mod::CircadianClock)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  SECTION("No initial OptimizeOptions") {
    DialogOptSetup dia(model);
    DialogOptSetupWidgets widgets(&dia);
    dia.show();
    ModalWidgetTimer mwt;
    SECTION("user does nothing") {
      REQUIRE(widgets.lstTargets->count() == 0);
      REQUIRE(widgets.btnAddTarget->isEnabled());
      REQUIRE(widgets.btnEditTarget->isEnabled() == false);
      REQUIRE(widgets.btnRemoveTarget->isEnabled() == false);
      REQUIRE(widgets.lstParameters->count() == 0);
      REQUIRE(widgets.btnAddParameter->isEnabled());
      REQUIRE(widgets.btnEditParameter->isEnabled() == false);
      REQUIRE(widgets.btnRemoveParameter->isEnabled() == false);
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
      widgets.cmbAlgorithm->setCurrentIndex(1);
      sendKeyEvents(widgets.spinIslands,
                    {"Delete", "Backspace", "1", "5", "Enter"});
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::GPSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 15);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 2);
      sendKeyEvents(widgets.spinPopulation,
                    {"Delete", "Backspace", "3", "2", "Enter"});
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::GPSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 15);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 32);
    }
    SECTION("user adds targets") {
      REQUIRE(widgets.lstTargets->count() == 0);
      REQUIRE(dia.getOptimizeOptions().optCosts.empty());
      // click add target, then cancel: no-op
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendMouseClick(widgets.btnAddTarget);
      REQUIRE(widgets.lstTargets->count() == 0);
      REQUIRE(dia.getOptimizeOptions().optCosts.empty());
      // click add target, change values, press ok
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbSpecies")->setCurrentIndex(1);
        dialog->findChild<QComboBox *>("cmbCostType")->setCurrentIndex(1);
        sendKeyEventsToQLineEdit(
            dialog->findChild<QLineEdit *>("txtSimulationTime"),
            {"4", "Enter"});
        dialog->findChild<QComboBox *>("cmbDiffType")->setCurrentIndex(1);
        sendKeyEventsToQLineEdit(dialog->findChild<QLineEdit *>("txtWeight"),
                                 {"2", "Enter"});
      });
      mwt.start();
      sendMouseClick(widgets.btnAddTarget);
      REQUIRE(widgets.lstTargets->count() == 1);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
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
      sendMouseClick(widgets.btnAddTarget);
      REQUIRE(widgets.lstTargets->count() == 2);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/Mt [Rate of change of concentration at t=4, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
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
      sendKeyEvents(widgets.lstTargets, {"Up"});
      REQUIRE(widgets.lstTargets->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnEditTarget);
      REQUIRE(widgets.lstTargets->count() == 2);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/Mt [Rate of change of concentration at t=4, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
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
      REQUIRE(widgets.lstTargets->currentRow() == 0);
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbSpecies")->setCurrentIndex(2);
      });
      mwt.start();
      sendMouseClick(widgets.btnEditTarget);
      REQUIRE(widgets.lstTargets->count() == 2);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/Mp [Concentration at t=100, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
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
      REQUIRE(widgets.lstTargets->currentRow() == 0);
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbCostType")->setCurrentIndex(1);
      });
      mwt.start();
      sendMouseDoubleClick(widgets.lstTargets->item(0));
      REQUIRE(mwt.getResult() == "Edit Optimization Target");
      REQUIRE(widgets.lstTargets->count() == 2);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/Mp [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
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
      REQUIRE(widgets.lstTargets->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnRemoveTarget);
      REQUIRE(widgets.lstTargets->count() == 2);
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
      sendMouseClick(widgets.btnRemoveTarget);
      REQUIRE(widgets.lstTargets->count() == 1);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
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
      sendMouseClick(widgets.btnRemoveTarget);
      REQUIRE(widgets.lstTargets->count() == 0);
      REQUIRE(optCosts.empty());
    }
    SECTION("user adds parameters") {
      REQUIRE(widgets.lstParameters->count() == 0);
      const auto &optParams{dia.getOptimizeOptions().optParams};
      REQUIRE(optParams.empty());
      // click add parameter, then cancel: no-op
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendMouseClick(widgets.btnAddParameter);
      REQUIRE(widgets.lstParameters->count() == 0);
      REQUIRE(optParams.empty());
      // click add parameter, change values, press ok
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbParameter")->setCurrentIndex(2);
        sendKeyEventsToQLineEdit(
            dialog->findChild<QLineEdit *>("txtLowerBound"), {"2", "Enter"});
        sendKeyEventsToQLineEdit(
            dialog->findChild<QLineEdit *>("txtUpperBound"), {"5", "Enter"});
      });
      mwt.start();
      sendMouseClick(widgets.btnAddParameter);
      REQUIRE(widgets.lstParameters->count() == 1);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
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
      sendMouseClick(widgets.btnAddParameter);
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      sendKeyEvents(widgets.lstParameters, {"Up"});
      REQUIRE(widgets.lstParameters->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnEditParameter);
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      REQUIRE(widgets.lstParameters->currentRow() == 0);
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbParameter")->setCurrentIndex(3);
      });
      mwt.start();
      sendMouseClick(widgets.btnEditParameter);
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [4, 4]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      REQUIRE(widgets.lstParameters->currentRow() == 0);
      mwt.addUserAction([](QWidget *dialog) {
        sendKeyEventsToQLineEdit(
            dialog->findChild<QLineEdit *>("txtLowerBound"), {"1", "Enter"});
        sendKeyEventsToQLineEdit(
            dialog->findChild<QLineEdit *>("txtUpperBound"), {"2", "Enter"});
      });
      mwt.start();
      sendMouseDoubleClick(widgets.lstParameters->item(0));
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [1, 2]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      REQUIRE(widgets.lstParameters->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 2);
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
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 1);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 0);
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
    DialogOptSetupWidgets widgets(&dia);
    dia.show();
    const auto &optParams{dia.getOptimizeOptions().optParams};
    const auto &optCosts{dia.getOptimizeOptions().optCosts};
    ModalWidgetTimer mwt;
    SECTION("user does nothing") {
      REQUIRE(widgets.lstTargets->count() == 2);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
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
      REQUIRE(widgets.btnAddTarget->isEnabled());
      REQUIRE(widgets.btnEditTarget->isEnabled() == false);
      REQUIRE(widgets.btnRemoveTarget->isEnabled() == false);
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Mp_transcription_K [1, 2]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
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
      REQUIRE(widgets.btnAddParameter->isEnabled());
      REQUIRE(widgets.btnEditParameter->isEnabled() == false);
      REQUIRE(widgets.btnRemoveParameter->isEnabled() == false);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.optAlgorithmType ==
              simulate::OptAlgorithmType::PSO);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 5);
      REQUIRE(dia.getOptimizeOptions().optAlgorithm.population == 17);
    }
    SECTION("user adds targets") {
      REQUIRE(widgets.lstTargets->count() == 2);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
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
      sendMouseClick(widgets.btnAddTarget);
      REQUIRE(widgets.lstTargets->count() == 2);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
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
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbSpecies")->setCurrentIndex(1);
        dialog->findChild<QComboBox *>("cmbCostType")->setCurrentIndex(1);
      });
      mwt.start();
      sendMouseClick(widgets.btnAddTarget);
      REQUIRE(widgets.lstTargets->count() == 3);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(2)->text() ==
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
      sendMouseClick(widgets.btnAddTarget);
      REQUIRE(widgets.lstTargets->count() == 4);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(widgets.lstTargets->item(3)->text() ==
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
      sendKeyEvents(widgets.lstTargets, {"Up"});
      REQUIRE(widgets.lstTargets->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnEditTarget);
      REQUIRE(widgets.lstTargets->count() == 4);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/P0 [Concentration at t=18, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(widgets.lstTargets->item(3)->text() ==
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
      REQUIRE(widgets.lstTargets->currentRow() == 0);
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbSpecies")->setCurrentIndex(1);
      });
      mwt.start();
      sendMouseClick(widgets.btnEditTarget);
      REQUIRE(widgets.lstTargets->count() == 4);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/Mt [Concentration at t=100, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(widgets.lstTargets->item(3)->text() ==
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
      REQUIRE(widgets.lstTargets->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnRemoveTarget);
      REQUIRE(widgets.lstTargets->count() == 4);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/Mt [Concentration at t=100, absolute difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(2)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(widgets.lstTargets->item(3)->text() ==
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
      sendMouseClick(widgets.btnRemoveTarget);
      REQUIRE(widgets.lstTargets->count() == 3);
      REQUIRE(widgets.lstTargets->item(0)->text() ==
              "cell/T0 [Rate of change of concentration at t=9, relative "
              "difference]");
      REQUIRE(widgets.lstTargets->item(1)->text() ==
              "cell/Mt [Rate of change of concentration at t=100, absolute "
              "difference]");
      REQUIRE(widgets.lstTargets->item(2)->text() ==
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
      sendMouseClick(widgets.btnRemoveTarget);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(widgets.btnRemoveTarget);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(widgets.btnRemoveTarget);
      REQUIRE(widgets.lstTargets->count() == 0);
      REQUIRE(optCosts.empty());
    }
    SECTION("user adds parameters") {
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Mp_transcription_K [1, 2]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
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
      sendMouseClick(widgets.btnAddParameter);
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Mp_transcription_K [1, 2]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
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
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbParameter")->setCurrentIndex(2);
        sendKeyEventsToQLineEdit(
            dialog->findChild<QLineEdit *>("txtLowerBound"), {"2", "Enter"});
        sendKeyEventsToQLineEdit(
            dialog->findChild<QLineEdit *>("txtUpperBound"), {"5", "Enter"});
      });
      mwt.start();
      sendMouseClick(widgets.btnAddParameter);
      REQUIRE(widgets.lstParameters->count() == 3);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Mp_transcription_K [1, 2]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(widgets.lstParameters->item(2)->text() ==
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
      sendMouseClick(widgets.btnAddParameter);
      REQUIRE(widgets.lstParameters->count() == 4);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Mp_transcription_K [1, 2]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(widgets.lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(widgets.lstParameters->item(3)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      sendKeyEvents(widgets.lstParameters, {"Up"});
      REQUIRE(widgets.lstParameters->currentRow() == 0);
      mwt.addUserAction({"Down", "Down", "Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnEditParameter);
      REQUIRE(widgets.lstParameters->count() == 4);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Mp_transcription_K [1, 2]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(widgets.lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(widgets.lstParameters->item(3)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      REQUIRE(widgets.lstParameters->currentRow() == 0);
      mwt.addUserAction([](QWidget *dialog) {
        dialog->findChild<QComboBox *>("cmbParameter")->setCurrentIndex(3);
      });
      mwt.start();
      sendMouseClick(widgets.btnEditParameter);
      REQUIRE(widgets.lstParameters->count() == 4);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [4, 4]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(widgets.lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(widgets.lstParameters->item(3)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      REQUIRE(widgets.lstParameters->currentRow() == 0);
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 4);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Reaction 'Mp transcription' / n [4, 4]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(widgets.lstParameters->item(2)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(widgets.lstParameters->item(3)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 3);
      REQUIRE(widgets.lstParameters->item(0)->text() ==
              "Mp_transcription_n [0.2, 0.4]");
      REQUIRE(widgets.lstParameters->item(1)->text() ==
              "Reaction 'Mp transcription' / K [2, 5]");
      REQUIRE(widgets.lstParameters->item(2)->text() ==
              "Reaction 'sP' / k [0.9, 0.9]");
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
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 2);
      REQUIRE(optParams.size() == 2);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 1);
      REQUIRE(optParams.size() == 1);
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(widgets.btnRemoveParameter);
      REQUIRE(widgets.lstParameters->count() == 0);
      REQUIRE(optParams.empty());
    }
  }
  SECTION("DUNE simulator: nthreads = 1") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/800
    model.getSimulationSettings().simulatorType =
        sme::simulate::SimulatorType::DUNE;
    model.getOptimizeOptions().optAlgorithm.islands = 4;
    DialogOptSetup dia(model);
    DialogOptSetupWidgets widgets(&dia);
    dia.show();
    REQUIRE(widgets.spinIslands->value() == 1);
    REQUIRE(widgets.spinIslands->maximum() == 1);
    REQUIRE(dia.getOptimizeOptions().optAlgorithm.islands == 1);
  }
}
