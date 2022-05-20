#include "catch_wrapper.hpp"
#include "dialogoptcost.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include <QLineEdit>

using namespace sme;
using namespace sme::test;

TEST_CASE("DialogOptCost", "[gui/dialogs/optcost][gui/"
                           "dialogs][gui][optimize]") {
  auto model{getExampleModel(Mod::CircadianClock)};
  std::vector<simulate::OptCost> defaultOptCosts;
  defaultOptCosts.push_back({simulate::OptCostType::ConcentrationDcdt,
                             simulate::OptCostDiffType::Relative, "cell/P0",
                             "P0", 12.0, 1.2, 0, 0,
                             std::vector<double>{1.0, 2.0, 3.0}, 1e-12});
  defaultOptCosts.push_back({simulate::OptCostType::Concentration,
                             simulate::OptCostDiffType::Relative, "cell/Mt",
                             "Mt", 10.0, 1.0, 0, 1,
                             std::vector<double>{1.0, 2.0, 3.0}, 1e-14});
  defaultOptCosts.push_back({simulate::OptCostType::Concentration,
                             simulate::OptCostDiffType::Absolute, "cell/Mp",
                             "Mp", 15.0, 0.6, 0, 2,
                             std::vector<double>{1.0, 2.0, 3.0}, 1e-12});
  defaultOptCosts.push_back({simulate::OptCostType::ConcentrationDcdt,
                             simulate::OptCostDiffType::Relative, "cell/T0",
                             "T0", 19.0, 0.1, 0, 3,
                             std::vector<double>{1.0, 2.0, 3.0}, 1e-13});
  ModalWidgetTimer mwt;
  SECTION("no possible costs") {
    DialogOptCost dia(model, {});
    mwt.addUserAction();
    mwt.start();
    dia.exec();
  }
  SECTION("no pre-selected cost") {
    DialogOptCost dia(model, defaultOptCosts);
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptCost().name == "cell/P0");
    REQUIRE(dia.getOptCost().id == "P0");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(12.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.2));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-12));
  }
  SECTION("user does nothing: correct initial values") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().targetValues ==
            std::vector<double>{1.0, 2.0, 3.0});
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
  }
  SECTION("user changes cost type") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    // get pointers to widgets within dialog
    auto *txtEpsilon{dia.findChild<QLineEdit *>("txtEpsilon")};
    REQUIRE(txtEpsilon != nullptr);
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    REQUIRE(txtEpsilon->isEnabled());
    mwt.addUserAction({"Tab", "Down", "Tab", "Up"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::ConcentrationDcdt);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Absolute);
    REQUIRE(txtEpsilon->isEnabled() == false);
  }
  SECTION("user changes species") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    mwt.addUserAction({"Down", "Down"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptCost().name == "cell/T0");
    REQUIRE(dia.getOptCost().id == "T0");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(19.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(0.1));
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
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    mwt.addUserAction({"Tab", "Tab", "Tab", "6", "2", ".", "8"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(62.8));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
  }
  SECTION("user changes scale factor") {
    DialogOptCost dia(model, defaultOptCosts, &defaultOptCosts[1]);
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "6", "2", ".", "8"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(62.8));
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
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-14));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
    mwt.addUserAction(
        {"Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "1", "e", "-", "1", "7"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptCost().name == "cell/Mt");
    REQUIRE(dia.getOptCost().id == "Mt");
    REQUIRE(dia.getOptCost().simulationTime == dbl_approx(10.0));
    REQUIRE(dia.getOptCost().scaleFactor == dbl_approx(1.0));
    REQUIRE(dia.getOptCost().epsilon == dbl_approx(1e-17));
    REQUIRE(dia.getOptCost().speciesIndex == 1);
    REQUIRE(dia.getOptCost().compartmentIndex == 0);
    REQUIRE(dia.getOptCost().optCostType ==
            simulate::OptCostType::Concentration);
    REQUIRE(dia.getOptCost().optCostDiffType ==
            simulate::OptCostDiffType::Relative);
  }
}
