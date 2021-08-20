#include "catch_wrapper.hpp"
#include "dialogdisplayoptions.hpp"
#include "plotwrapper.hpp"
#include "qt_test_utils.hpp"
#include <QStringList>

using namespace sme::test;

SCENARIO("DialogDisplayOptions",
         "[gui/dialogs/displayoptions][gui/dialogs][gui][displayoptions]") {
  GIVEN("Existing options, no observables") {
    QStringList compartments{"c1", "c2"};
    std::vector<QStringList> species;
    species.push_back({"s1_c1", "s2_c1"});
    species.push_back({"s1_c2", "s2_c2", "s3_c2"});
    sme::model::DisplayOptions opts;
    opts.showSpecies = {true, false, false, true, false};
    opts.showMinMax = false;
    opts.normaliseOverAllTimepoints = false;
    opts.normaliseOverAllSpecies = false;
    DialogDisplayOptions dia(compartments, species, opts, {});
    ModalWidgetTimer mwt;
    WHEN("user does nothing") {
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getShowMinMax() == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user checks first compartment: enables all species in compartment") {
      mwt.addUserAction({"Space"});
      mwt.start();
      dia.exec();
      opts.showSpecies[0] = true;
      opts.showSpecies[1] = true;
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN(
        "user checks, then unchecks first compartment: disables all species in "
        "compartment") {
      mwt.addUserAction({"Space", "Space"});
      mwt.start();
      dia.exec();
      opts.showSpecies[0] = false;
      opts.showSpecies[1] = false;
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user unchecks first species") {
      mwt.addUserAction({"Down", "Space"});
      mwt.start();
      dia.exec();
      opts.showSpecies[0] = false;
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user unchecks then checks first species") {
      mwt.addUserAction({"Down", "Space", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user toggles show min/max checkbox") {
      mwt.addUserAction({"Tab", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getShowMinMax() == true);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user toggles show min/max checkbox twice") {
      mwt.addUserAction({"Tab", "Space", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getShowMinMax() == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user changes timepoint normalisation") {
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == true);
    }
    WHEN("user changes species normalisation") {
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getNormaliseOverAllTimepoints() == true);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user changes both normalisations") {
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Down", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getNormaliseOverAllTimepoints() == true);
      REQUIRE(dia.getNormaliseOverAllSpecies() == true);
    }
    WHEN("user checks both compartment: enables all species") {
      mwt.addUserAction({"Space", "Down", "Down", "Down", "Space"});
      mwt.start();
      dia.exec();
      opts.showSpecies = {true, true, true, true, true};
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user checks & unchecks both compartments: disable all species") {
      mwt.addUserAction(
          {"Space", "Space", "Down", "Down", "Down", "Space", "Space"});
      mwt.start();
      dia.exec();
      opts.showSpecies = {false, false, false, false, false};
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
  }
  GIVEN("Existing options, existing observables") {
    QStringList compartments{"c1", "c2"};
    std::vector<QStringList> species;
    species.push_back({"s1_c1", "s2_c1"});
    species.push_back({"s1_c2", "s2_c2", "s3_c2"});
    sme::model::DisplayOptions opts;
    opts.showSpecies = {true, false, false, true, false};
    opts.showMinMax = false;
    opts.normaliseOverAllTimepoints = false;
    opts.normaliseOverAllSpecies = false;
    std::vector<PlotWrapperObservable> observables;
    observables.push_back({"1", "1", true});
    observables.push_back({"s1_c1*2", "s1_c1*2", true});
    DialogDisplayOptions dia(compartments, species, opts, observables);
    ModalWidgetTimer mwt;
    WHEN("user does nothing") {
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getShowMinMax() == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].visible == true);
      REQUIRE(dia.getObservables()[1].visible == true);
    }
    WHEN("user hides first observable") {
      mwt.addUserAction({"Tab", "Tab", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].visible == false);
      REQUIRE(dia.getObservables()[1].visible == true);
    }
    WHEN("user hides both observables") {
      mwt.addUserAction({"Tab", "Tab", "Space", "Down", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].visible == false);
      REQUIRE(dia.getObservables()[1].visible == false);
    }
    WHEN("user adds new observable") {
      ModalWidgetTimer mwt2;
      mwt2.addUserAction({"1", "+", "2"});
      mwt.addUserAction({"Tab", "Tab", "Tab", "Space"}, true, &mwt2);
      mwt.start();
      dia.exec();
      REQUIRE(dia.getObservables().size() == 3);
      REQUIRE(dia.getObservables()[2].name == "1+2");
      REQUIRE(dia.getObservables()[2].expression == "1+2");
      REQUIRE(dia.getObservables()[2].visible == true);
    }
    WHEN("user edits observable") {
      ModalWidgetTimer mwt2;
      mwt2.addUserAction({"Right", "-", "2"});
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Space"}, true, &mwt2);
      mwt.start();
      dia.exec();
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].name == "1-2");
      REQUIRE(dia.getObservables()[0].expression == "1-2");
      REQUIRE(dia.getObservables()[0].visible == true);
      REQUIRE(dia.getObservables()[1].name == "s1_c1*2");
      REQUIRE(dia.getObservables()[1].expression == "s1_c1*2");
      REQUIRE(dia.getObservables()[1].visible == true);
    }
    WHEN("user removes observable") {
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getObservables().size() == 1);
      REQUIRE(dia.getObservables()[0].name == "s1_c1*2");
      REQUIRE(dia.getObservables()[0].expression == "s1_c1*2");
      REQUIRE(dia.getObservables()[0].visible == true);
    }
  }
}
