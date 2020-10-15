#include "catch_wrapper.hpp"
#include "model_reactions.hpp"
#include <QFile>
#include <QImage>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

SCENARIO("SBML reactions",
         "[core/model/reactions][core/model][core][model][reactions]") {
  GIVEN("ModelReactions") {
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
    model::ModelReactions r(doc->getModel(), {});
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

    // remove reaction
    r.remove("A_B_conversion");
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

    // remove reaction
    r.remove("B_transport");
    REQUIRE(r.getIds("c1").empty());
    REQUIRE(r.getIds("c2").empty());
    REQUIRE(r.getIds("c3").empty());
    REQUIRE(r.getIds("c1_c2_membrane").empty());
    REQUIRE(r.getIds("c2_c3_membrane").empty());
  }
}
