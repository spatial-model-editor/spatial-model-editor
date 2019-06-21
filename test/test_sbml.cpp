#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "sbml.h"

SCENARIO("import SBML level 2 document", "[sbml]") {
  // create simple SBML level 2.4 model
  std::unique_ptr<libsbml::SBMLDocument> document(
      new libsbml::SBMLDocument(2, 4));
  // create model
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
  reac->addProduct(model->getSpecies("spec1c0"));
  reac->addReactant(model->getSpecies("spec0c0"));
  auto *kin = model->createKineticLaw();
  kin->setFormula("5*spec0c0");
  reac->setKineticLaw(kin);
  // create a function
  auto *func = model->createFunctionDefinition();
  func->setId("func1");
  // write SBML document to file
  libsbml::SBMLWriter w;
  w.writeSBML(document.get(), "tmp.xml");

  SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  REQUIRE(s.isValid == true);

  GIVEN("SBML document") {
    WHEN("importSBMLFile called") {
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
      // Note: need to import geometry image before we have the reactions
      /* THEN("find reactions") {
        REQUIRE(s.reactions.at("compartment0").size() == 1);
        REQUIRE(s.reactions.at("compartment0")[0] == "reac1");
      }*/
      THEN("find functions") {
        REQUIRE(s.functions.size() == 1);
        REQUIRE(s.functions[0] == "func1");
      }
    }
  }
  GIVEN("Compartment Colours") {
    QRgb col1 = 34573423;
    QRgb col2 = 1334573423;
    QRgb col3 = 17423;
    WHEN("compartment colours have not been assigned") {
      THEN("unassigned colours map to null CompartmentIDs") {
        REQUIRE(s.getCompartmentID(col1) == "");
        REQUIRE(s.getCompartmentID(123) == "");
      }
      THEN("invalid/unassigned CompartmentIDs map to null colour") {
        REQUIRE(s.getCompartmentColour("compartment0") == 0);
        REQUIRE(s.getCompartmentColour("invalid_comp") == 0);
      }
    }
    WHEN("compartment colours have been assigned") {
      s.setCompartmentColour("compartment0", col1);
      s.setCompartmentColour("compartment1", col2);
      THEN("can get CompartmentID from colour") {
        REQUIRE(s.getCompartmentID(col1) == "compartment0");
        REQUIRE(s.getCompartmentID(col2) == "compartment1");
        REQUIRE(s.getCompartmentID(col3) == "");
      }
      THEN("can get colour from CompartmentID") {
        REQUIRE(s.getCompartmentColour("compartment0") == col1);
        REQUIRE(s.getCompartmentColour("compartment1") == col2);
      }
    }
    WHEN("new colour assigned") {
      s.setCompartmentColour("compartment0", col1);
      s.setCompartmentColour("compartment1", col2);
      s.setCompartmentColour("compartment0", col3);
      THEN("unassign old colour mapping") {
        REQUIRE(s.getCompartmentID(col1) == "");
        REQUIRE(s.getCompartmentID(col2) == "compartment1");
        REQUIRE(s.getCompartmentID(col3) == "compartment0");
        REQUIRE(s.getCompartmentColour("compartment0") == col3);
        REQUIRE(s.getCompartmentColour("compartment1") == col2);
      }
    }
    WHEN("existing colour re-assigned") {
      s.setCompartmentColour("compartment0", col1);
      s.setCompartmentColour("compartment1", col2);
      s.setCompartmentColour("compartment0", col2);
      THEN("unassign old colour mapping") {
        REQUIRE(s.getCompartmentID(col1) == "");
        REQUIRE(s.getCompartmentID(col2) == "compartment0");
        REQUIRE(s.getCompartmentID(col3) == "");
        REQUIRE(s.getCompartmentColour("compartment0") == col2);
        REQUIRE(s.getCompartmentColour("compartment1") == 0);
      }
    }
  }
}

SCENARIO("import geometry from image", "[sbml]") {
  SbmlDocWrapper s;
  REQUIRE(s.hasGeometry == false);
  GIVEN("Single pixel image") {
    QImage img(1, 1, QImage::Format_RGB32);
    QRgb col = QColor(12, 243, 154).rgba();
    img.setPixel(0, 0, col);
    img.save("tmp.bmp");
    s.importGeometryFromImage("tmp.bmp");
    REQUIRE(s.hasGeometry == true);
    THEN("getCompartmentImage returns image") {
      REQUIRE(s.getCompartmentImage().size() == QSize(1, 1));
      REQUIRE(s.getCompartmentImage().pixel(0, 0) == col);
    }
    THEN("image contains no membranes") {
      // todo
    }
  }
}

TEST_CASE("load SBML level 3 document with spatial extension") {
  // create SBML level 3.1.1 model with spatial extension
  libsbml::SpatialPkgNamespaces sbmlns(3, 1, 1);
  libsbml::SBMLDocument document(&sbmlns);

  REQUIRE(1 == 1);
}
