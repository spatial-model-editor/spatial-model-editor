#include "catch_wrapper.hpp"
#include "duneconverter_impl.hpp"
#include "model.hpp"
#include <QFile>

SCENARIO("DUNE: DuneConverter impl",
         "[core/simulate/duneconverter][core/simulate][core][duneconverter]") {
  GIVEN("Species Names") {
    std::vector<std::string> names{"a",   "x",  "y",     "t",    "a_i",
                                   "cos", "x_", "out_o", "out_a"};
    auto duneNames = simulate::makeValidDuneSpeciesNames(names);
    REQUIRE(duneNames.size() == names.size());
    REQUIRE(duneNames[0] == "a");
    REQUIRE(duneNames[1] == "x__");
    REQUIRE(duneNames[2] == "y_");
    REQUIRE(duneNames[3] == "t_");
    REQUIRE(duneNames[4] == "a_i_");
    REQUIRE(duneNames[5] == "cos_");
    REQUIRE(duneNames[6] == "x_");
    REQUIRE(duneNames[7] == "out_o_");
    REQUIRE(duneNames[8] == "out_a");
  }
  GIVEN("ABtoC model") {
    model::Model s;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "comp") == true);
    auto nonConstantSpecies = simulate::getNonConstantSpecies(s, "comp");
    REQUIRE(nonConstantSpecies.size() == 3);
    REQUIRE(nonConstantSpecies[0] == "A");
    REQUIRE(nonConstantSpecies[1] == "B");
    REQUIRE(nonConstantSpecies[2] == "C");
    REQUIRE(simulate::modelHasIndependentCompartments(s) == true);

    // make all species constant
    s.getSpecies().setIsConstant("A", true);
    s.getSpecies().setIsConstant("B", true);
    s.getSpecies().setIsConstant("C", true);
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "comp") ==
            false);
    REQUIRE(simulate::getNonConstantSpecies(s, "comp").empty());
    REQUIRE(simulate::modelHasIndependentCompartments(s) == true);
  }
  GIVEN("very-simple-model model") {
    model::Model s;
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c1") == true);
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c2") == true);
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c3") == true);
    REQUIRE(simulate::getNonConstantSpecies(s, "c1").size() == 1);
    REQUIRE(simulate::getNonConstantSpecies(s, "c2").size() == 2);
    REQUIRE(simulate::getNonConstantSpecies(s, "c3").size() == 2);
    REQUIRE(simulate::modelHasIndependentCompartments(s) == false);

    // remove only non-constant species in compartment c1
    s.getSpecies().remove("B_c1");
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c1") == false);
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c2") == true);
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c3") == true);
    REQUIRE(simulate::getNonConstantSpecies(s, "c1").empty());
    REQUIRE(simulate::getNonConstantSpecies(s, "c2").size() == 2);
    REQUIRE(simulate::getNonConstantSpecies(s, "c3").size() == 2);
    REQUIRE(simulate::modelHasIndependentCompartments(s) == false);

    // remove membrane reactions
    s.getReactions().remove("A_uptake");
    s.getReactions().remove("A_transport");
    s.getReactions().remove("B_excretion");
    s.getReactions().remove("B_transport");
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c1") == false);
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c2") == true);
    REQUIRE(simulate::compartmentContainsNonConstantSpecies(s, "c3") == true);
    REQUIRE(simulate::getNonConstantSpecies(s, "c1").empty());
    REQUIRE(simulate::getNonConstantSpecies(s, "c2").size() == 2);
    REQUIRE(simulate::getNonConstantSpecies(s, "c3").size() == 2);
    REQUIRE(simulate::modelHasIndependentCompartments(s) == true);
  }
}
