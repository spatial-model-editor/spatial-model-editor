#include "catch_wrapper.hpp"
#include "dialogoptcost.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/utils.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

using namespace sme;
using namespace sme::test;

struct DialogOptCostWidgets {
  explicit DialogOptCostWidgets(const DialogOptCost *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbSpecies);
    GET_DIALOG_WIDGET(QComboBox, cmbCostType);
    GET_DIALOG_WIDGET(QLineEdit, txtSimulationTime);
    GET_DIALOG_WIDGET(QPushButton, btnEditTargetValues);
    GET_DIALOG_WIDGET(QComboBox, cmbDiffType);
    GET_DIALOG_WIDGET(QLineEdit, txtWeight);
    GET_DIALOG_WIDGET(QLineEdit, txtEpsilon);
  }
  QComboBox *cmbSpecies;
  QComboBox *cmbCostType;
  QLineEdit *txtSimulationTime;
  QPushButton *btnEditTargetValues;
  QComboBox *cmbDiffType;
  QLineEdit *txtWeight;
  QLineEdit *txtEpsilon;
};

TEST_CASE("DialogOptCost", "[gui/dialogs/optcost][gui/"
                           "dialogs][gui][optimize]") {
  auto model{getExampleModel(Mod::CircadianClock)};
  std::vector<simulate::OptCost> defaultOptCosts;
  defaultOptCosts.push_back({simulate::OptCostType::ConcentrationDcdt,
                             simulate::OptCostDiffType::Relative, "cell/P0",
                             "P0", 12.0, 1.2, 0, 0, std::vector<double>{},
                             1e-12});
  defaultOptCosts.push_back({simulate::OptCostType::Concentration,
                             simulate::OptCostDiffType::Relative, "cell/Mt",
                             "Mt", 10.0, 1.0, 0, 1, std::vector<double>{},
                             1e-14});
  defaultOptCosts.push_back({simulate::OptCostType::Concentration,
                             simulate::OptCostDiffType::Absolute, "cell/Mp",
                             "Mp", 15.0, 0.6, 0, 2, std::vector<double>{},
                             1e-12});
  defaultOptCosts.push_back({simulate::OptCostType::ConcentrationDcdt,
                             simulate::OptCostDiffType::Relative, "cell/T0",
                             "T0", 19.0, 0.1, 0, 3, std::vector<double>{},
                             1e-13});
  SECTION("no possible costs") {
    DialogOptCost dia(model, {});
    dia.show();
  }
  SECTION("no pre-selected cost: defaults to first one") {
    DialogOptCost dia(model, defaultOptCosts);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/P0");
    REQUIRE(dia.getOptCost().id == "P0");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(12.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.2));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-12));
  }
  SECTION("user does nothing: correct initial values") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    dia.show();
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().targetValues.empty());
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
  }
  SECTION("user changes cost type") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    DialogOptCostWidgets widgets(&dia);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    REQUIRE(widgets.txtEpsilon->isEnabled());
    widgets.cmbCostType->setCurrentIndex(1);
    widgets.cmbDiffType->setCurrentIndex(0);
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::ConcentrationDcdt);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Absolute);
    REQUIRE(widgets.txtEpsilon->isEnabled() == false);
  }
  SECTION("user changes species") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    DialogOptCostWidgets widgets(&dia);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    widgets.cmbSpecies->setCurrentIndex(3);
    REQUIRE(dia.getOptCost().name == "cell/T0");
    REQUIRE(dia.getOptCost().id == "T0");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(19.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(0.1));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-13));
    REQUIRE(dia.getOptCost().speciesIndex == 3);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::ConcentrationDcdt);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
  }
  SECTION("user changes simulation time") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    DialogOptCostWidgets widgets(&dia);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    sendKeyEvents(
        widgets.txtSimulationTime,
        {"Backspace", "Backspace", "Backspace", "6", "2", ".", "8", "Enter"});
    CAPTURE(widgets.txtSimulationTime->text().toStdString());
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(62.8));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
  }
  SECTION("user changes target conc values") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    DialogOptCostWidgets widgets(&dia);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    REQUIRE(dia.getOptCost().targetValues.empty());
    // set conc from image in range [1,2]
    ModalWidgetTimer mwt;
    mwt.addUserAction({"1", "Tab", "2"});
    mwt.start();
    sendMouseClick(widgets.btnEditTargetValues);
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    REQUIRE(dia.getOptCost().targetValues.size() == 10000);
    REQUIRE(mwt.getResult() == "Set target Concentration");
    // all pixels within compartment are set to 1
    REQUIRE(sme::common::max(dia.getOptCost().targetValues) == dbl_approx(1.0));
    // pixels outside of compartment are zero
    REQUIRE(sme::common::min(dia.getOptCost().targetValues) == dbl_approx(0.0));
  }
  SECTION("user changes target dcdt values") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[0]);
    DialogOptCostWidgets widgets(&dia);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/P0");
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::ConcentrationDcdt);
    REQUIRE(dia.getOptCost().targetValues.empty());
    // set conc dcdt from image in range [1.2,2]
    ModalWidgetTimer mwt;
    mwt.addUserAction({"1", ".", "2", "Tab", "2"});
    mwt.start();
    sendMouseClick(widgets.btnEditTargetValues);
    REQUIRE(dia.getOptCost().name == "cell/P0");
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::ConcentrationDcdt);
    REQUIRE(mwt.getResult() == "Set target Rate of change of concentration");
    // all pixels within compartment are set to 1.2
    REQUIRE(sme::common::max(dia.getOptCost().targetValues) == dbl_approx(1.2));
    // pixels outside of compartment are zero
    REQUIRE(sme::common::min(dia.getOptCost().targetValues) == dbl_approx(0.0));
  }
  SECTION("user changes scale factor") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    DialogOptCostWidgets widgets(&dia);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    sendKeyEvents(widgets.txtWeight, {"Backspace", "Backspace", "Backspace",
                                      "6", "2", ".", "8", "Enter"});
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(62.8));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
  }
  SECTION("user changes epsilon") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    DialogOptCostWidgets widgets(&dia);
    dia.show();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    sendKeyEvents(widgets.txtEpsilon,
                  {"Backspace", "Backspace", "Backspace", "Backspace",
                   "Backspace", "1", "e", "-", "1", "7", "Enter"});
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().weight == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-17));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
  }
}
