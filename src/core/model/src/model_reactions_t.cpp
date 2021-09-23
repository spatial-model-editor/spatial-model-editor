#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_reactions.hpp"
#include "model_test_utils.hpp"
#include <QImage>
#include <memory>

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML reactions",
          "[core/model/reactions][core/model][core][model][reactions]") {
  SECTION("ModelReactions") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    auto &r{m.getReactions()};
    r.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").size() == 1);
    REQUIRE(r.getIds("c3")[0] == "A_B_conversion");
    REQUIRE(r.getParameterIds("A_B_conversion").size() == 1);
    REQUIRE(r.getIds("c1_c2_membrane").size() == 2);
    REQUIRE(r.getIds("c1_c2_membrane")[0] == "A_uptake");
    REQUIRE(r.getParameterIds("A_uptake").size() == 1);
    REQUIRE(r.getIds("c1_c2_membrane")[1] == "B_excretion");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getIds("c2_c3_membrane").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[0] == "A_transport");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[1] == "B_transport");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);

    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "A_c1") == dbl_approx(-1.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "B_c1") == dbl_approx(0.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "A_c2") == dbl_approx(1.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "B_c2") == dbl_approx(0.0));
    REQUIRE(r.getScheme("A_uptake") == "A_out -> A_cell");
    // move reaction to cell: A_out no longer valid reaction species so removed
    r.setLocation("A_uptake", "c2");
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "A_c1") == dbl_approx(0.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "B_c1") == dbl_approx(0.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "A_c2") == dbl_approx(1.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "B_c2") == dbl_approx(0.0));
    REQUIRE(r.getScheme("A_uptake") == " -> A_cell");
    // move reaction to outside: A_cell no longer valid & removed
    r.setLocation("A_uptake", "c1");
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "A_c1") == dbl_approx(0.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "B_c1") == dbl_approx(0.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "A_c2") == dbl_approx(0.0));
    REQUIRE(r.getSpeciesStoichiometry("A_uptake", "B_c2") == dbl_approx(0.0));
    REQUIRE(r.getScheme("A_uptake") == "");
    // move reaction back to starting location
    r.setLocation("A_uptake", "c1_c2_membrane");
    REQUIRE(r.getScheme("A_uptake") == "");

    // remove parameter from non-existent reaction: no-op
    r.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    r.removeParameter("i_dont_exist", "i_dont_exist");
    REQUIRE(r.getHasUnsavedChanges() == false);

    // remove non-existent reaction parameter: no-op
    r.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(r.getParameterIds("A_B_conversion").size() == 1);
    r.removeParameter("A_B_conversion", "i_dont_exist");
    REQUIRE(r.getParameterIds("A_B_conversion").size() == 1);
    REQUIRE(r.getHasUnsavedChanges() == false);

    // remove reaction parameter
    r.removeParameter("A_B_conversion", "k1");
    REQUIRE(r.getParameterIds("A_B_conversion").size() == 0);
    REQUIRE(r.getHasUnsavedChanges() == true);

    // remove reaction
    r.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    r.remove("A_B_conversion");
    REQUIRE(r.getHasUnsavedChanges() == true);
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").empty());
    REQUIRE(r.getIds("c1_c2_membrane").size() == 2);
    REQUIRE(r.getIds("c1_c2_membrane")[0] == "A_uptake");
    REQUIRE(r.getParameterIds("A_uptake").size() == 1);
    REQUIRE(r.getIds("c1_c2_membrane")[1] == "B_excretion");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getIds("c2_c3_membrane").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[0] == "A_transport");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[1] == "B_transport");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);

    // remove reaction
    r.remove("A_uptake");
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").empty());
    REQUIRE(r.getIds("c1_c2_membrane").size() == 1);
    REQUIRE(r.getIds("c1_c2_membrane")[0] == "B_excretion");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getIds("c2_c3_membrane").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[0] == "A_transport");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[1] == "B_transport");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);

    // remove reaction
    r.remove("B_excretion");
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").empty());
    REQUIRE(r.getIds("c1_c2_membrane").empty());
    REQUIRE(r.getIds("c2_c3_membrane").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[0] == "A_transport");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getIds("c2_c3_membrane")[1] == "B_transport");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);

    // remove reaction
    r.remove("A_transport");
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").empty());
    REQUIRE(r.getIds("c1_c2_membrane").empty());
    REQUIRE(r.getIds("c2_c3_membrane").size() == 1);
    REQUIRE(r.getIds("c2_c3_membrane")[0] == "B_transport");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);

    // remove non-existent reaction: no-op
    r.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    r.remove("i_dont_exist");
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").empty());
    REQUIRE(r.getIds("c1_c2_membrane").empty());
    REQUIRE(r.getIds("c2_c3_membrane").size() == 1);
    REQUIRE(r.getIds("c2_c3_membrane")[0] == "B_transport");
    REQUIRE(r.getParameterIds("B_transport").size() == 1);
    REQUIRE(r.getHasUnsavedChanges() == false);

    // remove reaction
    r.remove("B_transport");
    REQUIRE(r.getHasUnsavedChanges() == true);
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").empty());
    REQUIRE(r.getIds("c1_c2_membrane").empty());
    REQUIRE(r.getIds("c2_c3_membrane").empty());
  }
  SECTION("Model with spatial reactions whose location is not set") {
    auto doc{getTestSbmlDoc("fish_300x300")};
    {
      const auto *reac0{doc->getModel()->getReaction("re0")};
      REQUIRE(reac0->isSetCompartment() == false);
      const auto *reac1{doc->getModel()->getReaction("re1")};
      REQUIRE(reac1->isSetCompartment() == false);
      const auto *reac2{doc->getModel()->getReaction("re2")};
      REQUIRE(reac2->isSetCompartment() == false);
    }
    auto m{getTestModel("fish_300x300")};
    const auto &reactions{m.getReactions()};
    // reaction location inferred from species involved
    REQUIRE(reactions.getIds("fish").size() == 3);
    REQUIRE(reactions.getIds("fish")[0] == "re0");
    REQUIRE(reactions.getIds("fish")[1] == "re1");
    REQUIRE(reactions.getIds("fish")[2] == "re2");
    {
      auto doc2{toSbmlDoc(m)};
      const auto *reac0{doc2->getModel()->getReaction("re0")};
      REQUIRE(reac0->isSetCompartment() == true);
      REQUIRE(reac0->getCompartment() == "fish");
      const auto *reac1{doc2->getModel()->getReaction("re1")};
      REQUIRE(reac1->isSetCompartment() == true);
      REQUIRE(reac1->getCompartment() == "fish");
      const auto *reac2{doc2->getModel()->getReaction("re2")};
      REQUIRE(reac2->isSetCompartment() == true);
      REQUIRE(reac2->getCompartment() == "fish");
    }
  }
  SECTION("Model with spatial reaction whose name & location is not set") {
    auto doc{getTestSbmlDoc("example2D")};
    const auto *reac{doc->getModel()->getReaction(0)};
    REQUIRE(reac->isSetCompartment() == false);
    REQUIRE(reac->isSetName() == false);
    auto m{getTestModel("example2D")};
    const auto &reactions{m.getReactions()};
    // reaction location inferred from species involved
    REQUIRE(reactions.getIds("Nucleus_Cytosol_membrane").size() == 1);
    REQUIRE(reactions.getIds("Nucleus_Cytosol_membrane")[0] == "re1");
    // reaction name uses Id if not originally set
    REQUIRE(reactions.getName("re1") == "re1");
    auto doc2{toSbmlDoc(m)};
    const auto *reac2{doc2->getModel()->getReaction(0)};
    REQUIRE(reac2->isSetCompartment() == true);
    REQUIRE(reac2->getCompartment() == "Nucleus_Cytosol_membrane");
    REQUIRE(reac2->isSetName() == true);
    REQUIRE(reac2->getName() == reac2->getId());
  }
  SECTION("membrane reactions: import new geometry image without losing "
          "reactions") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[0]).size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[1]).size() == 2);
    QImage img(":/geometry/single-pixels-3x1.png");
    m.getGeometry().importGeometryFromImage(img, false);
    m.getGeometry().setPixelWidth(1.0);
    REQUIRE(m.getGeometry().getIsValid() == false);
    // assign valid compartment colours
    m.getCompartments().setColour("c1", img.pixel(0, 0));
    m.getCompartments().setColour("c2", img.pixel(1, 0));
    m.getCompartments().setColour("c3", img.pixel(2, 0));
    REQUIRE(m.getGeometry().getIsValid() == true);
    // recover membrane ids and membrane reactions
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[0]).size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[1]).size() == 2);
    // repeat with different but valid colour assignments
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/679
    m.getGeometry().importGeometryFromImage(img, false);
    m.getGeometry().setPixelWidth(1.0);
    REQUIRE(m.getGeometry().getIsValid() == false);
    m.getCompartments().setColour("c1", img.pixel(2, 0));
    m.getCompartments().setColour("c2", img.pixel(1, 0));
    m.getCompartments().setColour("c3", img.pixel(0, 0));
    REQUIRE(m.getGeometry().getIsValid() == true);
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[0]).size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[1]).size() == 2);
  }
  SECTION("membrane reactions removed when compartment removed") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getMembranes().getIds()[0] == "c1_c2_membrane");
    REQUIRE(m.getMembranes().getIds()[1] == "c2_c3_membrane");
    auto locations{m.getReactions().getReactionLocations()};
    REQUIRE(locations.size() == 6);
    REQUIRE(locations[0].id == "c1");
    REQUIRE(locations[0].name == "Outside");
    REQUIRE(locations[0].type ==
            sme::model::ReactionLocation::Type::Compartment);
    REQUIRE(locations[1].id == "c2");
    REQUIRE(locations[2].id == "c3");
    REQUIRE(locations[3].id == "c1_c2_membrane");
    REQUIRE(locations[3].type == sme::model::ReactionLocation::Type::Membrane);
    REQUIRE(locations[4].id == "c2_c3_membrane");
    REQUIRE(locations[5].id == "invalid");
    REQUIRE(locations[5].type == sme::model::ReactionLocation::Type::Invalid);
    REQUIRE(m.getReactions().getIds(locations[0]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[1]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[2]).size() == 1);
    REQUIRE(m.getReactions().getIds(locations[3]).size() == 2);
    REQUIRE(m.getReactions().getIds(locations[4]).size() == 2);
    REQUIRE(m.getReactions().getIds(locations[5]).size() == 0);
    m.getCompartments().remove("c1");
    REQUIRE(m.getMembranes().getIds().size() == 1);
    REQUIRE(m.getMembranes().getIds()[0] == "c2_c3_membrane");
    locations = m.getReactions().getReactionLocations();
    REQUIRE(locations.size() == 4);
    REQUIRE(locations[0].id == "c2");
    REQUIRE(locations[0].type ==
            sme::model::ReactionLocation::Type::Compartment);
    REQUIRE(locations[1].id == "c3");
    REQUIRE(locations[2].id == "c2_c3_membrane");
    REQUIRE(locations[2].type == sme::model::ReactionLocation::Type::Membrane);
    REQUIRE(locations[3].id == "invalid");
    REQUIRE(locations[3].type == sme::model::ReactionLocation::Type::Invalid);
    REQUIRE(m.getReactions().getIds(locations[0]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[1]).size() == 1);
    REQUIRE(m.getReactions().getIds(locations[2]).size() == 2);
    REQUIRE(m.getReactions().getIds(locations[3]).size() == 0);
    m.getCompartments().remove("c2");
    REQUIRE(m.getMembranes().getIds().size() == 0);
    locations = m.getReactions().getReactionLocations();
    REQUIRE(locations.size() == 2);
    REQUIRE(locations[0].id == "c3");
    REQUIRE(locations[0].type ==
            sme::model::ReactionLocation::Type::Compartment);
    REQUIRE(locations[1].id == "invalid");
    REQUIRE(locations[1].type == sme::model::ReactionLocation::Type::Invalid);
    REQUIRE(m.getReactions().getIds(locations[0]).size() == 1);
    REQUIRE(m.getReactions().getIds(locations[1]).size() == 0);
  }
  SECTION(
      "membrane reactions -> invalid location if membrane no longer exists") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getMembranes().getIds()[0] == "c1_c2_membrane");
    REQUIRE(m.getMembranes().getIds()[1] == "c2_c3_membrane");
    auto locations{m.getReactions().getReactionLocations()};
    REQUIRE(locations.size() == 6);
    REQUIRE(locations[0].id == "c1");
    REQUIRE(locations[0].name == "Outside");
    REQUIRE(locations[0].type ==
            sme::model::ReactionLocation::Type::Compartment);
    REQUIRE(locations[1].id == "c2");
    REQUIRE(locations[2].id == "c3");
    REQUIRE(locations[3].id == "c1_c2_membrane");
    REQUIRE(locations[3].type == sme::model::ReactionLocation::Type::Membrane);
    REQUIRE(locations[4].id == "c2_c3_membrane");
    REQUIRE(locations[5].id == "invalid");
    REQUIRE(locations[5].type == sme::model::ReactionLocation::Type::Invalid);
    REQUIRE(m.getReactions().getIds(locations[0]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[1]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[2]).size() == 1);
    REQUIRE(m.getReactions().getIds(locations[3]).size() == 2);
    REQUIRE(m.getReactions().getIds(locations[4]).size() == 2);
    REQUIRE(m.getReactions().getIds(locations[5]).size() == 0);
    QImage img(":/geometry/single-pixels-3x1.png");
    m.getGeometry().importGeometryFromImage(img, false);
    m.getGeometry().setPixelWidth(1.0);
    REQUIRE(m.getGeometry().getIsValid() == false);
    // assign compartments such that c1-c3 and c3-c2 share borders,
    // i.e. remove c1-c2 membrane from model geometry
    m.getCompartments().setColour("c1", img.pixel(0, 0));
    m.getCompartments().setColour("c3", img.pixel(1, 0));
    m.getCompartments().setColour("c2", img.pixel(2, 0));
    REQUIRE(m.getGeometry().getIsValid() == true);
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getMembranes().getIds()[0] == "c1_c3_membrane");
    // previous id is maintained for c3-c2 membrane:
    REQUIRE(m.getMembranes().getIds()[1] == "c2_c3_membrane");
    locations = m.getReactions().getReactionLocations();
    REQUIRE(locations[0].id == "c1");
    REQUIRE(locations[0].name == "Outside");
    REQUIRE(locations[0].type ==
            sme::model::ReactionLocation::Type::Compartment);
    REQUIRE(locations[1].id == "c2");
    REQUIRE(locations[2].id == "c3");
    REQUIRE(locations[3].id == "c1_c3_membrane");
    REQUIRE(locations[3].type == sme::model::ReactionLocation::Type::Membrane);
    REQUIRE(locations[4].id == "c2_c3_membrane");
    REQUIRE(locations[5].id == "invalid");
    REQUIRE(locations[5].type == sme::model::ReactionLocation::Type::Invalid);
    REQUIRE(m.getReactions().getIds(locations[0]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[1]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[2]).size() == 1);
    REQUIRE(m.getReactions().getIds(locations[3]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[4]).size() == 2);
    // c1-c2 membrane reactions now have invalid locations:
    REQUIRE(m.getReactions().getIds(locations[5]).size() == 2);
    // reassign compartment geometry so that c1-c2 membrane exists again:
    REQUIRE(m.getGeometry().getIsValid() == true);
    m.getCompartments().setColour("c3", img.pixel(2, 0));
    REQUIRE(m.getGeometry().getIsValid() == false);
    m.getCompartments().setColour("c2", img.pixel(1, 0));
    REQUIRE(m.getGeometry().getIsValid() == true);
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getMembranes().getIds()[0] == "c1_c2_membrane");
    REQUIRE(m.getMembranes().getIds()[1] == "c2_c3_membrane");
    locations = m.getReactions().getReactionLocations();
    REQUIRE(locations.size() == 6);
    REQUIRE(locations[0].id == "c1");
    REQUIRE(locations[0].name == "Outside");
    REQUIRE(locations[0].type ==
            sme::model::ReactionLocation::Type::Compartment);
    REQUIRE(locations[1].id == "c2");
    REQUIRE(locations[2].id == "c3");
    REQUIRE(locations[3].id == "c1_c2_membrane");
    REQUIRE(locations[3].type == sme::model::ReactionLocation::Type::Membrane);
    REQUIRE(locations[4].id == "c2_c3_membrane");
    REQUIRE(locations[5].id == "invalid");
    REQUIRE(locations[5].type == sme::model::ReactionLocation::Type::Invalid);
    REQUIRE(m.getReactions().getIds(locations[0]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[1]).size() == 0);
    REQUIRE(m.getReactions().getIds(locations[2]).size() == 1);
    REQUIRE(m.getReactions().getIds(locations[3]).size() == 2);
    REQUIRE(m.getReactions().getIds(locations[4]).size() == 2);
    REQUIRE(m.getReactions().getIds(locations[5]).size() == 0);
  }
}
