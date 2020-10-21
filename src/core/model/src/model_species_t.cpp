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

SCENARIO("SBML species",
         "[core/model/species][core/model][core][model][species]") {
  GIVEN("Remove species also removes field") {
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    model::Model model;
    model.importSBMLString(f.readAll().toStdString());
    auto &s = model.getSpecies();
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

    // remove species
    s.remove("B_c3");
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

    // remove species
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

    // remove species
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

    // remove species
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

    // remove species
    s.remove("B_c1");
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

    // remove species
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
  }
}
