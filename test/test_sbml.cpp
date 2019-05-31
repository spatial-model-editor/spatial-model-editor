#include "catch.hpp"
#include "catch_qt_ostream.h"

#include "sbml.h"

TEST_CASE("load SBML level 2 document") {
  // create simple SBML level 2.4 model
  std::unique_ptr<libsbml::SBMLDocument> document(
      new libsbml::SBMLDocument(2, 4));
  // crate model
  auto *model = document->createModel();
  // create two compartments of different size
  for (int i = 0; i < 2; ++i) {
    auto *comp = model->createCompartment();
    comp->setId("compartment" + std::to_string(i));
    comp->setSize(1e-10 * i);
  }
  // create 2 species inside first compartment with initialConcentration set
  for (int i = 0; i < 2; ++i) {
    auto *spec = model->createSpecies();
    spec->setId("spec" + std::to_string(i) + "c0");
    spec->setCompartment("compartment0");
    spec->setInitialConcentration(i * 1e-12);
  }
  // create 3 species inside second compartment with initialAmount set
  for (int i = 0; i < 3; ++i) {
    auto *spec = model->createSpecies();
    spec->setId("spec" + std::to_string(i) + "c1");
    spec->setCompartment("compartment1");
    spec->setInitialAmount(100 * i);
  }
  // create a reaction: spec0c0 -> spec1c0
  auto *reac = model->createReaction();
  reac->setId("reac1");
  reac->addProduct(model->getSpeciesReference("spec1c0"));
  reac->addReactant(model->getSpeciesReference("spec0c0"));
  auto *kin = model->createKineticLaw();
  kin->setFormula("5*spec0c0");
  reac->setKineticLaw(kin);
  // write SBML document to file
  libsbml::SBMLWriter w;
  w.writeSBML(document.get(), "tmp.xml");

  GIVEN("SBML document") {
    WHEN("loadFile called") {
      sbmlDocWrapper s;
      s.loadFile("tmp.xml");
      THEN("find compartments") {
        REQUIRE(s.compartments.size() == 2);
        REQUIRE(s.compartments[0] == "compartment0");
        REQUIRE(s.compartments[1] == "compartment1");
      }
      THEN("find species") {
        REQUIRE(s.species.size() == 2);
        REQUIRE(s.species["compartment0"].size() == 2);
        REQUIRE(s.species["compartment0"][0] == "spec0c0");
        REQUIRE(s.species["compartment0"][1] == "spec1c0");
        REQUIRE(s.species["compartment1"].size() == 3);
        REQUIRE(s.species["compartment1"][0] == "spec0c1");
        REQUIRE(s.species["compartment1"][1] == "spec1c1");
        REQUIRE(s.species["compartment1"][2] == "spec2c1");
      }
      THEN("find species") {
        REQUIRE(s.species.size() == 2);
        REQUIRE(s.species["compartment0"].size() == 2);
        REQUIRE(s.species["compartment0"][0] == "spec0c0");
        REQUIRE(s.species["compartment0"][1] == "spec1c0");
        REQUIRE(s.species["compartment1"].size() == 3);
        REQUIRE(s.species["compartment1"][0] == "spec0c1");
        REQUIRE(s.species["compartment1"][1] == "spec1c1");
        REQUIRE(s.species["compartment1"][2] == "spec2c1");
      }
    }
  }
}

TEST_CASE("load SBML level 3 document with spatial extension") {
  // create SBML level 3.1.1 model with spatial extension
  libsbml::SpatialPkgNamespaces sbmlns(3, 1, 1);
  libsbml::SBMLDocument document(&sbmlns);

  REQUIRE(1 == 1);
}
