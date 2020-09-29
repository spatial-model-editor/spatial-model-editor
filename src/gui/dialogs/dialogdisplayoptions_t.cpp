#include <QFile>
#include <QStringList>

#include "catch_wrapper.hpp"
#include "dialogdisplayoptions.hpp"
#include "qt_test_utils.hpp"

SCENARIO("DialogDisplayOptions",
         "[gui/dialogs/displayoptions][gui/dialogs][gui][displayoptions]") {
  GIVEN("Existing options") {
    QStringList compartments{"c1", "c2"};
    std::vector<QStringList> species;
    species.push_back({"s1_c1", "s2_c1"});
    species.push_back({"s1_c2", "s2_c2", "s3_c2"});
    std::vector<bool> showSpecies{true, false, false, true, false};
    DialogDisplayOptions dia(compartments, species, showSpecies, false, false,
                             false);
    ModalWidgetTimer mwt;
    WHEN("user does nothing") {
      REQUIRE(dia.getShowSpecies() == showSpecies);
      REQUIRE(dia.getShowMinMax() == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user checks first compartment: enables all species in compartment") {
      mwt.addUserAction({"Space"});
      mwt.start();
      dia.exec();
      showSpecies[0] = true;
      showSpecies[1] = true;
      REQUIRE(dia.getShowSpecies() == showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN(
        "user checks, then unchecks first compartment: disables all species in "
        "compartment") {
      mwt.addUserAction({"Space", "Space"});
      mwt.start();
      dia.exec();
      showSpecies[0] = false;
      showSpecies[1] = false;
      REQUIRE(dia.getShowSpecies() == showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user unchecks first species") {
      mwt.addUserAction({"Down", "Space"});
      mwt.start();
      dia.exec();
      showSpecies[0] = false;
      REQUIRE(dia.getShowSpecies() == showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user unchecks then checks first species") {
      mwt.addUserAction({"Down", "Space", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getShowSpecies() == showSpecies);
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
      mwt.addUserAction({"Tab", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == true);
    }
    WHEN("user changes species normalisation") {
      mwt.addUserAction({"Tab", "Tab", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getNormaliseOverAllTimepoints() == true);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user changes both normalisations") {
      mwt.addUserAction({"Tab", "Tab", "Down", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getNormaliseOverAllTimepoints() == true);
      REQUIRE(dia.getNormaliseOverAllSpecies() == true);
    }
    WHEN("user checks both compartment: enables all species") {
      mwt.addUserAction({"Space", "Down", "Down", "Down", "Space"});
      mwt.start();
      dia.exec();
      showSpecies = {true, true, true, true, true};
      REQUIRE(dia.getShowSpecies() == showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    WHEN("user checks & unchecks both compartments: disable all species") {
      mwt.addUserAction(
          {"Space", "Space", "Down", "Down", "Down", "Space", "Space"});
      mwt.start();
      dia.exec();
      showSpecies = {false, false, false, false, false};
      REQUIRE(dia.getShowSpecies() == showSpecies);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
  }
}
