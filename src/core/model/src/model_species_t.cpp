#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_species.hpp"
#include <QFile>
#include <QImage>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;

SCENARIO("SBML species",
         "[core/model/species][core/model][core][model][species]") {
  GIVEN("Remove species also removes dependents") {
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    model::Model model;
    model.importSBMLString(f.readAll().toStdString());
    auto &s = model.getSpecies();
    auto &r = model.getReactions();
    r.setHasUnsavedChanges(false);
    s.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);

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

    // remove species B_c3
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    s.remove("B_c3");
    REQUIRE(r.getHasUnsavedChanges() == true);
    REQUIRE(s.getHasUnsavedChanges() == true);
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
    REQUIRE(s.getIds("c3").size() == 1);
    REQUIRE(s.getIds("c3")[0] == "A_c3");
    REQUIRE(s.getField("A_c3")->getId() == "A_c3");
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 2);
    REQUIRE(r.getLocation("A_uptake") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("A_uptake").size() == 1);
    REQUIRE(r.getParameterName("A_uptake", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_excretion") == "c1_c2_membrane");
    REQUIRE(r.getParameterIds("B_excretion").size() == 1);
    REQUIRE(r.getParameterName("B_excretion", "k1") == "k1");
    REQUIRE(r.getParameterValue("B_excretion", "k1") == dbl_approx(0.2));
    REQUIRE(r.getIds("c2_c3_membrane").size() == 1);
    REQUIRE(r.getLocation("A_transport") == "c2_c3_membrane");
    REQUIRE(r.getParameterIds("A_transport").size() == 2);
    REQUIRE(r.getParameterName("A_transport", "k1") == "k1");
    REQUIRE(r.getParameterValue("A_transport", "k1") == dbl_approx(0.1));
    REQUIRE(r.getParameterName("A_transport", "k2") == "k2");
    REQUIRE(r.getParameterValue("A_transport", "k2") == dbl_approx(0.1));
    REQUIRE(r.getLocation("B_transport") == "");
    REQUIRE(r.getParameterIds("B_transport").empty());
    REQUIRE(r.getParameterName("B_transport", "k1") == "");
    REQUIRE(r.getParameterValue("B_transport", "k1") == dbl_approx(0.0));

    // remove species A_c2
    s.remove("A_c2");
    REQUIRE(s.getIds("c1").size() == 2);
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getField("A_c1")->getId() == "A_c1");
    REQUIRE(s.getIds("c1")[1] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").size() == 1);
    REQUIRE(s.getIds("c3")[0] == "A_c3");
    REQUIRE(s.getField("A_c3")->getId() == "A_c3");
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 1);
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
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

    // remove species A_c3
    s.remove("A_c3");
    REQUIRE(s.getIds("c1").size() == 2);
    REQUIRE(s.getIds("c1")[0] == "A_c1");
    REQUIRE(s.getField("A_c1")->getId() == "A_c1");
    REQUIRE(s.getIds("c1")[1] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 1);
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
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

    // remove species A_c1
    s.remove("A_c1");
    REQUIRE(s.getIds("c1").size() == 1);
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getIds("c1")[0] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
    REQUIRE(r.getIds("c1_c2_membrane").size() == 1);
    REQUIRE(r.getLocation("A_uptake") == "");
    REQUIRE(r.getParameterIds("A_uptake").empty());
    REQUIRE(r.getParameterName("A_uptake", "k1") == "");
    REQUIRE(r.getParameterValue("A_uptake", "k1") == dbl_approx(0.0));
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

    // remove non-existent species: no-op
    r.setHasUnsavedChanges(false);
    s.setHasUnsavedChanges(false);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);
    s.remove("not_a_species");
    REQUIRE(s.getIds("c1").size() == 1);
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getIds("c1")[0] == "B_c1");
    REQUIRE(s.getField("B_c1")->getId() == "B_c1");
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);
    REQUIRE(r.getHasUnsavedChanges() == false);
    REQUIRE(s.getHasUnsavedChanges() == false);

    // remove species B_c1
    s.remove("B_c1");
    REQUIRE(r.getHasUnsavedChanges() == true);
    REQUIRE(s.getHasUnsavedChanges() == true);
    REQUIRE(s.getIds("c1").empty());
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getField("B_c1") == nullptr);
    REQUIRE(s.getIds("c2").size() == 1);
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getIds("c2")[0] == "B_c2");
    REQUIRE(s.getField("B_c2")->getId() == "B_c2");
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
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

    // remove species B_c2
    s.remove("B_c2");
    REQUIRE(s.getIds("c1").empty());
    REQUIRE(s.getField("A_c1") == nullptr);
    REQUIRE(s.getField("B_c1") == nullptr);
    REQUIRE(s.getIds("c2").empty());
    REQUIRE(s.getField("A_c2") == nullptr);
    REQUIRE(s.getField("B_c2") == nullptr);
    REQUIRE(s.getIds("c3").empty());
    REQUIRE(s.getField("A_c3") == nullptr);
    REQUIRE(s.getField("B_c3") == nullptr);

    REQUIRE(r.getIds("c1").size() == 0);
    REQUIRE(r.getIds("c2").size() == 0);
    REQUIRE(r.getIds("c3").size() == 0);
    REQUIRE(r.getName("A_B_conversion") == "");
    REQUIRE(r.getLocation("A_B_conversion") == "");
    REQUIRE(r.getParameterIds("A_B_conversion").empty());
    REQUIRE(r.getParameterName("A_B_conversion", "k1") == "");
    REQUIRE(r.getParameterValue("A_B_conversion", "k1") == dbl_approx(0.0));
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
}
