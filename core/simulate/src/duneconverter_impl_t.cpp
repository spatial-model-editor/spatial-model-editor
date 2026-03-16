#include "catch_wrapper.hpp"
#include "duneconverter_impl.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("DUNE: DuneConverter impl",
          "[core/simulate/duneconverter][core/simulate][core][duneconverter]") {
  SECTION("Species Names") {
    std::vector<std::string> names{"a",   "position_x", "position_y", "time",
                                   "a_i", "cos",        "position_x_"};
    auto duneNames = simulate::makeValidDuneSpeciesNames(names);
    REQUIRE(duneNames.size() == names.size());
    REQUIRE(duneNames[0] == "a");
    REQUIRE(duneNames[1] == "position_x__");
    REQUIRE(duneNames[2] == "position_y_");
    REQUIRE(duneNames[3] == "time_");
    REQUIRE(duneNames[4] == "a_i");
    REQUIRE(duneNames[5] == "cos_");
    REQUIRE(duneNames[6] == "position_x_");
  }
  SECTION("ABtoC model") {
    auto s{getExampleModel(Mod::ABtoC)};
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "comp") == true);
    auto simulatedSpecies = simulate::getSimulatedSpecies(s, "comp");
    REQUIRE(simulatedSpecies.size() == 3);
    REQUIRE(simulatedSpecies[0] == "A");
    REQUIRE(simulatedSpecies[1] == "B");
    REQUIRE(simulatedSpecies[2] == "C");

    // scalar constant species are excluded from the simulated field set
    s.getSpecies().setIsSpatial("A", false);
    s.getSpecies().setIsSpatial("B", false);
    s.getSpecies().setIsSpatial("C", false);
    s.getSpecies().setIsConstant("A", true);
    s.getSpecies().setIsConstant("B", true);
    s.getSpecies().setIsConstant("C", true);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "comp") == false);
    REQUIRE(simulate::getSimulatedSpecies(s, "comp").empty());

    // spatial constant species remain part of the simulated field set
    s.getSpecies().setIsSpatial("A", true);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "comp") == true);
    REQUIRE(simulate::getSimulatedSpecies(s, "comp") ==
            std::vector<std::string>{"A"});
  }
  SECTION("very-simple-model model") {
    auto s{getExampleModel(Mod::VerySimpleModel)};
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c1") == true);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c2") == true);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c3") == true);
    REQUIRE(simulate::getSimulatedSpecies(s, "c1").size() == 1);
    REQUIRE(simulate::getSimulatedSpecies(s, "c2").size() == 2);
    REQUIRE(simulate::getSimulatedSpecies(s, "c3").size() == 2);

    // remove only simulated species in compartment c1
    s.getSpecies().remove("B_c1");
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c1") == false);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c2") == true);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c3") == true);
    REQUIRE(simulate::getSimulatedSpecies(s, "c1").empty());
    REQUIRE(simulate::getSimulatedSpecies(s, "c2").size() == 2);
    REQUIRE(simulate::getSimulatedSpecies(s, "c3").size() == 2);

    // remove membrane reactions
    s.getReactions().remove("A_uptake");
    s.getReactions().remove("A_transport");
    s.getReactions().remove("B_excretion");
    s.getReactions().remove("B_transport");
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c1") == false);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c2") == true);
    REQUIRE(simulate::compartmentContainsSimulatedSpecies(s, "c3") == true);
    REQUIRE(simulate::getSimulatedSpecies(s, "c1").empty());
    REQUIRE(simulate::getSimulatedSpecies(s, "c2").size() == 2);
    REQUIRE(simulate::getSimulatedSpecies(s, "c3").size() == 2);
  }
}
