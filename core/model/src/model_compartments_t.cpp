#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/model_compartments.hpp"
#include <memory>

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML compartments",
          "[core/model/compartments][core/model][core][model][compartments]") {
  SECTION("Remove compartment also removes dependents") {
    auto model{getExampleModel(Mod::VerySimpleModel)};
    REQUIRE(model.getHasUnsavedChanges() == false);
    auto &c = model.getCompartments();
    REQUIRE(c.getHasUnsavedChanges() == false);
    c.setHasUnsavedChanges(false);
    REQUIRE(c.getHasUnsavedChanges() == false);
    auto &s = model.getSpecies();
    REQUIRE(s.getHasUnsavedChanges() == false);
    s.setHasUnsavedChanges(false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    auto &r = model.getReactions();
    REQUIRE(r.getHasUnsavedChanges() == false);
    r.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    auto &m = model.getMembranes();
    REQUIRE(m.getHasUnsavedChanges() == false);
    m.setHasUnsavedChanges(false);
    REQUIRE(m.getHasUnsavedChanges() == false);

    // initial compartments
    REQUIRE(c.getIds().size() == 3);
    REQUIRE(c.getIds()[0] == "c1");
    REQUIRE(c.getName("c1") == "Outside");
    REQUIRE(c.getSize("c1") == dbl_approx(5441000));
    REQUIRE(c.getIds()[1] == "c2");
    REQUIRE(c.getName("c2") == "Cell");
    REQUIRE(c.getSize("c2") == dbl_approx(4034000));
    REQUIRE(c.getIds()[2] == "c3");
    REQUIRE(c.getName("c3") == "Nucleus");
    REQUIRE(c.getSize("c3") == dbl_approx(525000));

    // initial membranes
    REQUIRE(m.getIds().size() == 2);
    REQUIRE(m.getIds()[0] == "c1_c2_membrane");
    REQUIRE(m.getName("c1_c2_membrane") == "Outside <-> Cell");
    REQUIRE(m.getIds()[1] == "c2_c3_membrane");
    REQUIRE(m.getName("c2_c3_membrane") == "Cell <-> Nucleus");

    // initial species
    REQUIRE(s.getIds("c1").size() == 2);
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getField("A_c1")->getId() == "A_c1");
    REQUIRE(s.getIds("c1")[1] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 2);
    REQUIRE(s.getIds("c2")[0] == "A_c2");
    REQUIRE(s.getField("A_c2")->getId() == "A_c2");
    REQUIRE(s.getIds("c2")[1] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").size() == 2);
    REQUIRE(s.getIds("c3")[0] == "A_c3");
    REQUIRE(s.getField("A_c3")->getId() == "A_c3");
    REQUIRE(s.getIds("c3")[1] == "B_c3");
    REQUIRE(s.getField("B_c3")->getId() == "B_c3");

    // initial reactions
    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 1);
    REQUIRE(r.getName("A_B_conversion") == "A to B conversion");
    REQUIRE(r.getLocation("A_B_conversion") == "c3");
    REQUIRE(r.getParameterIds("A_B_conversion").size() == 1);
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.3));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 2);
    REQUIRE(r.getLocation("A_uptake") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("A_uptake").size() == 1);
    REQUIRE(r.getParameterName("A_uptake", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
    REQUIRE(r.getIds("c2_c3_membrane").size() == 2);
    REQUIRE(r.getLocation("A_transport") == "c2_c3_membrane");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getParameterName("A_transport", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.1));
    REQUIRE(r.getParameterName("A_transport", "k2") == "k2");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_transport") == "c2_c3_membrane");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);
    REQUIRE(r.getParameterName("B_transport", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.1));

    SECTION("remove c1") {
      REQUIRE(c.getHasUnsavedChanges() == false);
      REQUIRE(m.getHasUnsavedChanges() == false);
      REQUIRE(s.getHasUnsavedChanges() == false);
      REQUIRE(r.getHasUnsavedChanges() == false);
      c.remove("c1");
      REQUIRE(c.getHasUnsavedChanges() == true);
      // compartments
      REQUIRE(c.getIds().size() == 2);
      REQUIRE(c.getNames().size() == 2);
      REQUIRE(c.getName("c1") == "");
      REQUIRE(c.getIds()[0] == "c2");
      REQUIRE(c.getNames()[0] == "Cell");
      REQUIRE(c.getName("c2") == "Cell");
      REQUIRE(c.getIds()[1] == "c3");
      REQUIRE(c.getNames()[1] == "Nucleus");
      REQUIRE(c.getName("c3") == "Nucleus");

      // membranes
      REQUIRE(m.getHasUnsavedChanges() == true);
      REQUIRE(m.getIds().size() == 1);
      REQUIRE(m.getName("c1_c2_membrane") == "");
      REQUIRE(m.getIds()[0] == "c2_c3_membrane");
      REQUIRE(m.getName("c2_c3_membrane") == "Cell <-> Nucleus");

      // species
      REQUIRE(s.getHasUnsavedChanges() == true);
      REQUIRE(s.getIds("c1").empty());
      REQUIRE(s.getField("A_c1") == nullptr);
      REQUIRE(s.getField("B_c1") == nullptr);
      REQUIRE(s.getIds("c2").size() == 2);
      REQUIRE(s.getIds("c2")[0] == "A_c2");
      REQUIRE(s.getField("A_c2")->getId() == "A_c2");
      REQUIRE(s.getIds("c2")[1] == "B_c2");
      REQUIRE(s.getField("B_c2")->getId() == "B_c2");
      REQUIRE(s.getIds("c3").size() == 2);
      REQUIRE(s.getIds("c3")[0] == "A_c3");
      REQUIRE(s.getField("A_c3")->getId() == "A_c3");
      REQUIRE(s.getIds("c3")[1] == "B_c3");
      REQUIRE(s.getField("B_c3")->getId() == "B_c3");

      // reactions
      REQUIRE(r.getHasUnsavedChanges() == true);
      REQUIRE(r.getIds("c1").size() == 0);
      REQUIRE(r.getIds("c2").size() == 0);
      REQUIRE(r.getIds("c3").size() == 1);
      REQUIRE(r.getName("A_B_conversion") == "A to B conversion");
      REQUIRE(r.getLocation("A_B_conversion") == "c3");
      REQUIRE(r.getParameterIds("A_B_conversion").size() == 1);
      REQUIRE(r.getParameterName("A_B_conversion", "k1") == "k1");
      REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.3));
      REQUIRE(r.getIds("c1_c2_membrane").empty());
      REQUIRE(r.getLocation("A_uptake") == "");
      REQUIRE(r.getParameterIds("A_uptake").empty());
      REQUIRE(r.getParameterName("A_uptake", "k1") == "");
      REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
      REQUIRE(r.getLocation("B_excretion") == "");
      REQUIRE(r.getParameterIds("B_excretion").empty());
      REQUIRE(r.getParameterName("B_excretion", "k1") == "");
      REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.0));
      REQUIRE(r.getIds("c2_c3_membrane").size() == 2);
      REQUIRE(r.getLocation("A_transport") == "c2_c3_membrane");
      REQUIRE(r.getParameterIds("A_transport").size() == 2);
      REQUIRE(r.getParameterName("A_transport", "k1") == "k1");
      REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.1));
      REQUIRE(r.getParameterName("A_transport", "k2") == "k2");
      REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.1));
      REQUIRE(r.getLocation("B_transport") == "c2_c3_membrane");
      REQUIRE(r.getParameterIds("B_transport").size() == 1);
      REQUIRE(r.getParameterName("B_transport", "k1") == "k1");
      REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.1));
    }
    SECTION("remove c2") {
      c.remove("c2");
      // compartments
      REQUIRE(c.getIds().size() == 2);
      REQUIRE(c.getNames().size() == 2);
      REQUIRE(c.getName("c1") == "Outside");
      REQUIRE(c.getIds()[0] == "c1");
      REQUIRE(c.getNames()[0] == "Outside");
      REQUIRE(c.getName("c2") == "");
      REQUIRE(c.getIds()[1] == "c3");
      REQUIRE(c.getNames()[1] == "Nucleus");
      REQUIRE(c.getName("c3") == "Nucleus");

      // membranes
      REQUIRE(m.getIds().empty());
      REQUIRE(m.getNames().empty());
      REQUIRE(m.getName("c1_c2_membrane") == "");
      REQUIRE(m.getName("c2_c3_membrane") == "");

      // species
      REQUIRE(s.getIds("c1").size() == 2);
      REQUIRE(s.getIds("c1")[0] == "A_c1");
      REQUIRE(s.getField("A_c1")->getId() == "A_c1");
      REQUIRE(s.getIds("c1")[1] == "B_c1");
      REQUIRE(s.getIds("c2").empty());
      REQUIRE(s.getField("A_c2") == nullptr);
      REQUIRE(s.getField("B_c2") == nullptr);
      REQUIRE(s.getIds("c3").size() == 2);
      REQUIRE(s.getIds("c3")[0] == "A_c3");
      REQUIRE(s.getField("A_c3")->getId() == "A_c3");
      REQUIRE(s.getIds("c3")[1] == "B_c3");
      REQUIRE(s.getField("B_c3")->getId() == "B_c3");

      // reactions
      REQUIRE(r.getIds("c1").size() == 0);
      REQUIRE(r.getIds("c2").size() == 0);
      REQUIRE(r.getIds("c3").size() == 1);
      REQUIRE(r.getName("A_B_conversion") == "A to B conversion");
      REQUIRE(r.getLocation("A_B_conversion") == "c3");
      REQUIRE(r.getParameterIds("A_B_conversion").size() == 1);
      REQUIRE(r.getParameterName("A_B_conversion", "k1") == "k1");
      REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.3));
      REQUIRE(r.getIds("c1_c2_membrane").empty());
      REQUIRE(r.getLocation("A_uptake") == "");
      REQUIRE(r.getParameterIds("A_uptake").empty());
      REQUIRE(r.getParameterName("A_uptake", "k1") == "");
      REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
      REQUIRE(r.getLocation("B_excretion") == "");
      REQUIRE(r.getParameterIds("B_excretion").empty());
      REQUIRE(r.getParameterName("B_excretion", "k1") == "");
      REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.0));
      REQUIRE(r.getIds("c2_c3_membrane").empty());
      REQUIRE(r.getLocation("A_transport") == "");
      REQUIRE(r.getParameterIds("A_transport").empty());
      REQUIRE(r.getParameterName("A_transport", "k1") == "");
      REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.0));
      REQUIRE(r.getParameterName("A_transport", "k2") == "");
      REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.0));
      REQUIRE(r.getLocation("B_transport") == "");
      REQUIRE(r.getParameterIds("B_transport").empty());
      REQUIRE(r.getParameterName("B_transport", "k1") == "");
      REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));
    }
    SECTION("remove c3") {
      c.remove("c3");
      // compartments
      REQUIRE(c.getIds().size() == 2);
      REQUIRE(c.getNames().size() == 2);
      REQUIRE(c.getIds()[0] == "c1");
      REQUIRE(c.getNames()[0] == "Outside");
      REQUIRE(c.getName("c1") == "Outside");
      REQUIRE(c.getIds()[1] == "c2");
      REQUIRE(c.getNames()[1] == "Cell");
      REQUIRE(c.getName("c2") == "Cell");
      REQUIRE(c.getName("c3") == "");

      // membranes
      REQUIRE(m.getIds().size() == 1);
      REQUIRE(m.getIds()[0] == "c1_c2_membrane");
      REQUIRE(m.getName("c1_c2_membrane") == "Outside <-> Cell");
      REQUIRE(m.getName("c2_c3_membrane") == "");

      // species
      REQUIRE(s.getIds("c1").size() == 2);
      REQUIRE(s.getIds("c1")[0] == "A_c1");
      REQUIRE(s.getField("A_c1")->getId() == "A_c1");
      REQUIRE(s.getIds("c1")[1] == "B_c1");
      REQUIRE(s.getIds("c2").size() == 2);
      REQUIRE(s.getIds("c2")[0] == "A_c2");
      REQUIRE(s.getField("A_c2")->getId() == "A_c2");
      REQUIRE(s.getIds("c2")[1] == "B_c2");
      REQUIRE(s.getField("B_c2")->getId() == "B_c2");
      REQUIRE(s.getIds("c3").empty());
      REQUIRE(s.getField("A_c3") == nullptr);
      REQUIRE(s.getField("B_c3") == nullptr);

      // reactions
      REQUIRE(r.getIds("c1").size() == 0);
      REQUIRE(r.getIds("c2").size() == 0);
      REQUIRE(r.getIds("c3").size() == 0);
      REQUIRE(r.getName("A_B_conversion") == "");
      REQUIRE(r.getLocation("A_B_conversion") == "");
      REQUIRE(r.getParameterIds("A_B_conversion").empty());
      REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
      REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0));
      REQUIRE(r.getIds("c1_c2_membrane").size() == 2);
      REQUIRE(r.getLocation("A_uptake") == "c1_c2_membrane");
      REQUIRE(r.getParameterIds("A_uptake").size() == 1);
      REQUIRE(r.getParameterName("A_uptake", "k1") == "k1");
      REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.1));
      REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
      REQUIRE(r.getParameterIds("B_excretion").size() == 1);
      REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
      REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
      REQUIRE(r.getIds("c2_c3_membrane").empty());
      REQUIRE(r.getLocation("A_transport") == "");
      REQUIRE(r.getParameterIds("A_transport").empty());
      REQUIRE(r.getParameterName("A_transport", "k1") == "");
      REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.0));
      REQUIRE(r.getParameterName("A_transport", "k2") == "");
      REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.0));
      REQUIRE(r.getLocation("B_transport") == "");
      REQUIRE(r.getParameterIds("B_transport").empty());
      REQUIRE(r.getParameterName("B_transport", "k1") == "");
      REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));
    }
  }
  SECTION("New model: add compartment, remove it again") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/506
    model::Model m;
    m.createSBMLFile("no-geom");
    auto &compartments{m.getCompartments()};
    REQUIRE(compartments.getIds().size() == 0);
    compartments.add("c");
    REQUIRE(compartments.getIds().size() == 1);
    REQUIRE(compartments.getHasUnsavedChanges() == true);
    compartments.setHasUnsavedChanges(false);
    REQUIRE(compartments.getHasUnsavedChanges() == false);
    // invalid remove call is a no-op
    compartments.remove("idontexist");
    REQUIRE(compartments.getIds().size() == 1);
    REQUIRE(compartments.getHasUnsavedChanges() == false);
    compartments.remove("c");
    REQUIRE(compartments.getIds().size() == 0);
    REQUIRE(compartments.getHasUnsavedChanges() == true);
  }
  SECTION("Setting invalid compartment color is a no-op") {
    auto model{getExampleModel(Mod::VerySimpleModel)};
    auto &compartments{model.getCompartments()};
    REQUIRE(compartments.getIds().size() == 3);
    REQUIRE(compartments.getIds()[0] == "c1");
    REQUIRE(compartments.getIds()[1] == "c2");
    REQUIRE(compartments.getIds()[2] == "c3");
    REQUIRE(compartments.getHasUnsavedChanges() == false);
    // check initial sizes
    const auto &initialSizes{compartments.getInitialCompartmentSizes()};
    REQUIRE(initialSizes.at("c1") == dbl_approx(5441000.0));
    REQUIRE(initialSizes.at("c2") == dbl_approx(4034000.0));
    REQUIRE(initialSizes.at("c3") == dbl_approx(525000.0));
    QRgb c1{qRgb(0, 2, 0)};
    QRgb c2{qRgb(144, 97, 193)};
    QRgb c3{qRgb(197, 133, 96)};
    QRgb cInvalid{qRgb(1, 2, 3)};
    auto cols{model.getCompartments().getColors()};
    REQUIRE(cols.size() == 3);
    REQUIRE(cols[0] == c1);
    REQUIRE(cols[1] == c2);
    REQUIRE(cols[2] == c3);
    // invalid id, valid color
    compartments.setColor("notme", c3);
    REQUIRE(compartments.getHasUnsavedChanges() == false);
    REQUIRE(compartments.getColors() == cols);
    // invalid id, invalid color
    compartments.setColor("notme", c3);
    REQUIRE(compartments.getHasUnsavedChanges() == false);
    REQUIRE(compartments.getColors() == cols);
    // valid id, invalid color
    compartments.setColor("c1", cInvalid);
    REQUIRE(compartments.getHasUnsavedChanges() == false);
    REQUIRE(compartments.getColors() == cols);
    // valid id, valid color
    compartments.setColor("c1", c3);
    REQUIRE(compartments.getHasUnsavedChanges() == true);
    REQUIRE(compartments.getColor("c1") == c3);
    REQUIRE(compartments.getSize("c1") == dbl_approx(initialSizes.at("c3")));
    REQUIRE(compartments.getColor("c2") == c2);
    REQUIRE(compartments.getSize("c2") == dbl_approx(initialSizes.at("c2")));
    REQUIRE(compartments.getColor("c3") == 0);
    REQUIRE(compartments.getSize("c3") == dbl_approx(0));
    // valid id, invalid color
    compartments.setHasUnsavedChanges(false);
    compartments.setColor("c3", cInvalid);
    REQUIRE(compartments.getHasUnsavedChanges() == false);
    REQUIRE(compartments.getColor("c1") == c3);
    REQUIRE(compartments.getColor("c2") == c2);
    REQUIRE(compartments.getColor("c3") == 0);
  }
  SECTION("Very simple model: remove compartment using const ref to id") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/685
    auto m{getExampleModel(Mod::VerySimpleModel)};
    REQUIRE(m.getCompartments().getIds().size() == 3);
    REQUIRE(m.getCompartments().getIds()[0] == "c1");
    REQUIRE(m.getCompartments().getIds()[1] == "c2");
    REQUIRE(m.getCompartments().getIds()[2] == "c3");
    const auto &cId{m.getCompartments().getIds()[1]};
    REQUIRE(cId == "c2");
    // #685: arg here is a const ref to the compartment id in the vector of ids,
    // so when it is removed from the vector, the const ref now points to the
    // following compartment id, i.e in this case it changed its value to "c3"
    // halfway through the remove() call
    m.getCompartments().remove(cId);
    REQUIRE(m.getCompartments().getIds().size() == 2);
    REQUIRE(m.getCompartments().getIds()[0] == "c1");
    REQUIRE(m.getCompartments().getIds()[1] == "c3");
  }
}
