#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/model_reactions.hpp"
#include <QImage>
#include <memory>

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML reactions",
          "[core/model/reactions][core/model][core][model][reactions]") {
  SECTION("Remove reactions") {
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
  SECTION("Spatial reactions whose location is not set") {
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
  SECTION("Spatial reaction whose name & location is not set") {
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
  SECTION("Membrane reactions: import new geometry image without losing "
          "reactions") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[0]).size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[1]).size() == 2);
    QImage img(":/geometry/single-pixels-3x1.png");
    m.getGeometry().importGeometryFromImages({img}, false);
    m.getGeometry().setVoxelSize({1.0, 1.0, 1.0});
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
    m.getGeometry().importGeometryFromImages({img}, false);
    m.getGeometry().setVoxelSize({1.0, 1.0, 1.0});
    REQUIRE(m.getGeometry().getIsValid() == false);
    m.getCompartments().setColour("c1", img.pixel(2, 0));
    m.getCompartments().setColour("c2", img.pixel(1, 0));
    m.getCompartments().setColour("c3", img.pixel(0, 0));
    REQUIRE(m.getGeometry().getIsValid() == true);
    REQUIRE(m.getMembranes().getIds().size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[0]).size() == 2);
    REQUIRE(m.getReactions().getIds(m.getMembranes().getIds()[1]).size() == 2);
  }
  SECTION("Membrane reactions removed when compartment removed") {
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
      "Membrane reactions -> invalid location if membrane no longer exists") {
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
    m.getGeometry().importGeometryFromImages({img}, false);
    m.getGeometry().setVoxelSize({1.0, 1.0, 1.0});
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
  SECTION("SBML Modifer Species: compartment reaction") {
    // note: these are only exported to make the SBML valid, we don't use them
    auto m{getExampleModel(Mod::LiverSimplified)};
    auto &r{m.getReactions()};
    auto ids{r.getIds("cytoplasm")};
    REQUIRE(ids.size() == 12);
    REQUIRE(ids[0] == "IkBap_degradation");
    auto expr{r.getRateExpression(ids[0])};
    REQUIRE(expr == "IkBa_p * IkBa_p_degradation_k1");
    r.setRateExpression("IkBap_degradation", expr + " + 0");
    auto doc{toSbmlDoc(m)};
    const auto *reac{doc->getModel()->getReaction("IkBap_degradation")};
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 1);
    REQUIRE(reac->getReactant(0)->getSpecies() == "IkBa_p");
    REQUIRE(reac->getListOfProducts()->size() == 0);
    REQUIRE(reac->getListOfModifiers()->size() == 0);
    // add species to expression with zero stoich (i.e. modifiers)
    r.setRateExpression("IkBap_degradation",
                        expr + " + TNFa + cos(some_func(x, IKKb))");
    doc = toSbmlDoc(m);
    reac = doc->getModel()->getReaction("IkBap_degradation");
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 1);
    REQUIRE(reac->getReactant(0)->getSpecies() == "IkBa_p");
    REQUIRE(reac->getListOfProducts()->size() == 0);
    REQUIRE(reac->getListOfModifiers()->size() == 2);
    REQUIRE(reac->getModifier("TNFa")->getSpecies() == "TNFa");
    REQUIRE(reac->getModifier("IKKb")->getSpecies() == "IKKb");
    // make IKKb stoich non-zero: no longer a modifier
    r.setSpeciesStoichiometry("IkBap_degradation", "IKKb", 2);
    // make IkBa_p stoich zero: now a modifier
    r.setSpeciesStoichiometry("IkBap_degradation", "IkBa_p", 0);
    doc = toSbmlDoc(m);
    reac = doc->getModel()->getReaction("IkBap_degradation");
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 0);
    REQUIRE(reac->getListOfProducts()->size() == 1);
    REQUIRE(reac->getProduct(0)->getSpecies() == "IKKb");
    REQUIRE(reac->getListOfModifiers()->size() == 2);
    REQUIRE(reac->getModifier("IkBa_p")->getSpecies() == "IkBa_p");
    REQUIRE(reac->getModifier("TNFa")->getSpecies() == "TNFa");
    // IKKb -> IkBa_cytoplasm, TNFa -> p65_cytoplasm in expression
    r.setRateExpression(
        "IkBap_degradation",
        expr + " + p65_cytoplasm + cos(some_func(x, IkBa_cytoplasm))");
    doc = toSbmlDoc(m);
    reac = doc->getModel()->getReaction("IkBap_degradation");
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 0);
    REQUIRE(reac->getListOfProducts()->size() == 1);
    REQUIRE(reac->getProduct(0)->getSpecies() == "IKKb");
    REQUIRE(reac->getListOfModifiers()->size() == 3);
    REQUIRE(reac->getModifier("IkBa_p")->getSpecies() == "IkBa_p");
    REQUIRE(reac->getModifier("p65_cytoplasm")->getSpecies() ==
            "p65_cytoplasm");
    REQUIRE(reac->getModifier("IkBa_cytoplasm")->getSpecies() ==
            "IkBa_cytoplasm");
    // remove p65 from expr
    r.setRateExpression("IkBap_degradation",
                        expr + " + cos(some_func(x, IkBa_cytoplasm))");
    doc = toSbmlDoc(m);
    reac = doc->getModel()->getReaction("IkBap_degradation");
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 0);
    REQUIRE(reac->getListOfProducts()->size() == 1);
    REQUIRE(reac->getProduct(0)->getSpecies() == "IKKb");
    REQUIRE(reac->getListOfModifiers()->size() == 2);
    REQUIRE(reac->getModifier("IkBa_cytoplasm")->getSpecies() ==
            "IkBa_cytoplasm");
    REQUIRE(reac->getModifier("IkBa_p")->getSpecies() == "IkBa_p");
    // set IKKb stoich to zero: not a modifier as no longer present in expr
    r.setSpeciesStoichiometry("IkBap_degradation", "IKKb", 0);
    doc = toSbmlDoc(m);
    reac = doc->getModel()->getReaction("IkBap_degradation");
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 0);
    REQUIRE(reac->getListOfProducts()->size() == 0);
    REQUIRE(reac->getListOfModifiers()->size() == 2);
    REQUIRE(reac->getModifier("IkBa_cytoplasm")->getSpecies() ==
            "IkBa_cytoplasm");
    REQUIRE(reac->getModifier("IkBa_p")->getSpecies() == "IkBa_p");
    // set IKKb stoich to negative
    r.setSpeciesStoichiometry("IkBap_degradation", "IKKb", -0.2);
    doc = toSbmlDoc(m);
    reac = doc->getModel()->getReaction("IkBap_degradation");
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 1);
    REQUIRE(reac->getReactant(0)->getSpecies() == "IKKb");
    REQUIRE(reac->getListOfProducts()->size() == 0);
    REQUIRE(reac->getListOfModifiers()->size() == 2);
    REQUIRE(reac->getModifier("IkBa_cytoplasm")->getSpecies() ==
            "IkBa_cytoplasm");
    REQUIRE(reac->getModifier("IkBa_p")->getSpecies() == "IkBa_p");
  }
  SECTION("SBML Modifer Species: membrane reaction") {
    // note: these are only exported to make the SBML valid, we don't use them
    auto m{getExampleModel(Mod::LiverSimplified)};
    auto &r{m.getReactions()};
    auto ids{r.getIds("cytoplasm_nucleus_membrane")};
    REQUIRE(ids.size() == 4);
    REQUIRE(ids[0] == "p65_shuttle");
    auto expr{r.getRateExpression(ids[0])};
    REQUIRE(expr ==
            "p65_shuttle_k1 * p65_cytoplasm - p65_shuttle_k2 * p65_nucleus");
    r.setRateExpression("p65_shuttle", expr + " + 0");
    auto doc{toSbmlDoc(m)};
    const auto *reac{doc->getModel()->getReaction("p65_shuttle")};
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 1);
    REQUIRE(reac->getReactant(0)->getSpecies() == "p65_cytoplasm");
    REQUIRE(reac->getListOfProducts()->size() == 1);
    REQUIRE(reac->getProduct(0)->getSpecies() == "p65_nucleus");
    REQUIRE(reac->getListOfModifiers()->size() == 0);
    // add more products and reactants
    r.setSpeciesStoichiometry("p65_shuttle", "IkBa_cytoplasm", 1.2);
    r.setSpeciesStoichiometry("p65_shuttle", "IkBa_nucleus", -0.77);
    // add some species to expression that have zero stoich (modifiers)
    r.setRateExpression("p65_shuttle",
                        expr + "sin(IkBa_cytoplasm) + myfunc(2.3, x, TNFa)");
    doc = toSbmlDoc(m);
    reac = doc->getModel()->getReaction("p65_shuttle");
    REQUIRE(reac != nullptr);
    REQUIRE(reac->getListOfReactants()->size() == 2);
    REQUIRE(reac->getReactant("p65_cytoplasm")->getSpecies() ==
            "p65_cytoplasm");
    REQUIRE(reac->getReactant("IkBa_nucleus")->getSpecies() == "IkBa_nucleus");
    REQUIRE(reac->getListOfProducts()->size() == 2);
    REQUIRE(reac->getProduct("p65_nucleus")->getSpecies() == "p65_nucleus");
    REQUIRE(reac->getProduct("IkBa_cytoplasm")->getSpecies() ==
            "IkBa_cytoplasm");
    REQUIRE(reac->getListOfModifiers()->size() == 1);
    REQUIRE(reac->getModifier("TNFa")->getSpecies() == "TNFa");
  }
}
