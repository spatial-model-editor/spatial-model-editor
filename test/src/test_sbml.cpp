#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QFile>
#include <fstream>

#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "sbml.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "sbml_test_data/yeast_glycolysis.hpp"
#include "utils.hpp"

/*
// NOTE:
// getParametricObjectByDomainType was returning null when compiled with gcc9
// todo: reproducible test case showing this failing...
  for (unsigned i = 0; i < parageom->getNumParametricObjects(); ++i) {
    const auto *po = parageom->getParametricObject(i);
    std::string id = po->getId();
    std::string dt = po->getDomainType();
    spdlog::debug("{} ::   * {} -> {}", fn, po->getId(), po->getDomainType());
    const auto *po2 = parageom->getParametricObject(id);
    spdlog::debug("{} ::   get with ID ->null? {}", fn, po2 == nullptr);
    spdlog::debug("{} ::   ->* {} -> {}", fn, po2->getId(),
                  po2->getDomainType());
    const auto *po3 = parageom->getParametricObjectByDomainType(dt);
    spdlog::debug("{} ::   get with domainType ->null? {}", fn, po3 == nullptr);
    spdlog::debug("{} ::   ->* {} -> {}", fn, po3->getId(),
                  po3->getDomainType());
  }
*/

static void createSBMLlvl2doc(const std::string &filename) {
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
  // write SBML document to file
  libsbml::SBMLWriter().writeSBML(document.get(), filename);
}

SCENARIO("SBML: import SBML doc without geometry", "[sbml][non-gui]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmp.xml");
  // import SBML model
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  REQUIRE(s.isValid == true);
  // export it again
  s.exportSBMLFile("tmp.xml");
  THEN("upgrade SBML doc and add default 2d spatial geometry") {
    // load new model
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromFile("tmp.xml"));
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    REQUIRE(model->getLevel() == 3);
    REQUIRE(model->getVersion() == 2);

    REQUIRE(doc->isPackageEnabled("spatial") == true);
    auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
        model->getPlugin("spatial"));
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->isSetGeometry() == true);
    auto *geom = plugin->getGeometry();
    REQUIRE(geom != nullptr);
    REQUIRE(geom->getNumCoordinateComponents() == 2);
    REQUIRE(geom->getCoordinateComponent(0)->getType() ==
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
    REQUIRE(geom->getCoordinateComponent(1)->getType() ==
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);

    REQUIRE(geom->getNumGeometryDefinitions() == 1);
    REQUIRE(geom->getGeometryDefinition(0)->isSampledFieldGeometry() == true);
    REQUIRE(geom->getGeometryDefinition(0)->getIsActive() == true);
    auto *sfgeom = dynamic_cast<libsbml::SampledFieldGeometry *>(
        geom->getGeometryDefinition(0));
    REQUIRE(sfgeom != nullptr);
    for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
      auto *comp = model->getCompartment(i);
      auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
          comp->getPlugin("spatial"));
      REQUIRE(scp->isSetCompartmentMapping() == true);
      std::string domainTypeID = scp->getCompartmentMapping()->getDomainType();
      REQUIRE(geom->getDomainByDomainType(domainTypeID) ==
              geom->getDomain(comp->getId() + "_domain"));
      REQUIRE(sfgeom->getSampledVolumeByDomainType(domainTypeID)->getId() ==
              comp->getId() + "_sampledVolume");
    }
  }
  WHEN("import geometry & assign compartments") {
    // import geometry image & assign compartments to colours
    s.importGeometryFromImage(":/geometry/single-pixels-3x1.png");
    s.setCompartmentColour("compartment0", 0xffaaaaaa);
    s.setCompartmentColour("compartment1", 0xff525252);
    // export it again
    s.exportSBMLFile("tmp.xml");

    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromFile("tmp.xml"));
    auto *model = doc->getModel();
    auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
        model->getPlugin("spatial"));
    auto *geom = plugin->getGeometry();
    auto *sfgeom = dynamic_cast<libsbml::SampledFieldGeometry *>(
        geom->getGeometryDefinition(0));
    auto *sf = geom->getSampledField(sfgeom->getSampledField());
    auto sfvals = utils::stringToVector<QRgb>(sf->getSamples());
    REQUIRE(sf->getNumSamples1() == 3);
    REQUIRE(sf->getNumSamples2() == 1);
    CAPTURE(sfvals[0]);
    CAPTURE(sfvals[1]);
    CAPTURE(sfvals[2]);

    auto *scp0 = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        model->getCompartment("compartment0")->getPlugin("spatial"));
    REQUIRE(scp0->isSetCompartmentMapping() == true);
    auto *sfvol0 = sfgeom->getSampledVolumeByDomainType(
        scp0->getCompartmentMapping()->getDomainType());
    CAPTURE(sfvol0->getSampledValue());
    REQUIRE(static_cast<unsigned>(sfvol0->getSampledValue()) == 0xffaaaaaa);
    REQUIRE(static_cast<unsigned>(sfvol0->getSampledValue()) ==
            static_cast<unsigned>(sfvals[1]));

    auto *scp1 = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        model->getCompartment("compartment1")->getPlugin("spatial"));
    REQUIRE(scp1->isSetCompartmentMapping() == true);
    auto *sfvol1 = sfgeom->getSampledVolumeByDomainType(
        scp1->getCompartmentMapping()->getDomainType());
    CAPTURE(sfvol1->getSampledValue());
    REQUIRE(static_cast<unsigned>(sfvol1->getSampledValue()) == 0xff525252);
    REQUIRE(static_cast<unsigned>(sfvol1->getSampledValue()) ==
            static_cast<unsigned>(sfvals[2]));

    WHEN("import concentration & set diff constants") {
      // import concentration
      s.setSampledFieldConcentration("spec0c0", {0.0, 0.0, 0.0});
      REQUIRE(s.getConcentrationImage("spec0c0").size() == QSize(3, 1));
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(1, 0) ==
              QColor(0, 0, 0).rgba());
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(0, 0) == 0);
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(2, 0) == 0);
      // set spec1c1conc to zero -> black pixel
      s.setSampledFieldConcentration("spec0c0", {0.0, 0.0, 0.0});
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(2, 0) == 0xff000000);
      s.setIsSpatial("spec0c0", true);
      s.setIsSpatial("spec1c0", true);
      s.setIsSpatial("spec0c1", true);
      s.setDiffusionConstant("spec0c0", 0.123);
      s.setDiffusionConstant("spec1c0", 0.1);
      s.setDiffusionConstant("spec1c0", 0.999999);
      s.setDiffusionConstant("spec0c1", 23.1 + 1e-12);
      CAPTURE(s.getDiffusionConstant("spec0c0"));
      CAPTURE(s.getDiffusionConstant("spec1c0"));
      CAPTURE(s.getDiffusionConstant("spec0c1"));
      REQUIRE(s.getDiffusionConstant("spec0c0") == dbl_approx(0.123));
      REQUIRE(s.getDiffusionConstant("spec1c0") == dbl_approx(0.999999));
      REQUIRE(s.getDiffusionConstant("spec0c1") == dbl_approx(23.1 + 1e-12));

      // export model
      s.exportSBMLFile("tmp2.xml");
      // import model again, recover concentration & compartment assignments
      sbml::SbmlDocWrapper s2;
      s2.importSBMLFile("tmp2.xml");
      REQUIRE(s2.getCompartmentColour("compartment0") == 0xffaaaaaa);
      REQUIRE(s2.getCompartmentColour("compartment1") == 0xff525252);
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(1, 0) ==
              QColor(0, 0, 0).rgba());
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(0, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(2, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(2, 0) == 0xff000000);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(0, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(1, 0) == 0x00000000);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(2, 0) ==
              QColor(0, 0, 0).rgba());

      CAPTURE(s2.getDiffusionConstant("spec0c0"));
      CAPTURE(s2.getDiffusionConstant("spec1c0"));
      CAPTURE(s2.getDiffusionConstant("spec0c1"));
      REQUIRE(s2.getDiffusionConstant("spec0c0") == dbl_approx(0.123));
      REQUIRE(s2.getDiffusionConstant("spec1c0") == dbl_approx(0.999999));
      REQUIRE(s2.getDiffusionConstant("spec0c1") == dbl_approx(23.1 + 1e-12));
    }
  }
}

SCENARIO("SBML: import SBML level 2 document", "[sbml][non-gui]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmp.xml");
  // import SBML model
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  REQUIRE(s.isValid == true);

  // import geometry image & assign compartments to colours
  s.importGeometryFromImage(":/geometry/single-pixels-3x1.png");
  s.setCompartmentColour("compartment0", 0xffaaaaaa);
  s.setCompartmentColour("compartment1", 0xff525252);

  GIVEN("SBML document & geometry image") {
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
    THEN("find reaction (divided by compartment volume factor)") {
      REQUIRE(s.reactions.at("compartment0").size() == 1);
      REQUIRE(s.reactions.at("compartment0")[0] == "reac1");
      auto r = s.getReaction("reac1");
      REQUIRE(r.id == "reac1");
      REQUIRE(r.name == "reac1");
      REQUIRE(r.products.size() == 1);
      REQUIRE(r.products[0].first == "spec1c0");
      REQUIRE(r.reactants.size() == 1);
      REQUIRE(r.reactants[0].first == "spec0c0");
      REQUIRE(r.fullExpression == "5 * spec0c0 / compartment0");
      REQUIRE(r.inlinedExpression == "5 * spec0c0 / compartment0");
    }
    WHEN("exportSBMLFile called") {
      THEN(
          "exported file is a SBML level (3,2) document with spatial "
          "extension enabled & required") {
        s.exportSBMLFile("export.xml");
        std::unique_ptr<libsbml::SBMLDocument> doc(
            libsbml::readSBMLFromFile("export.xml"));
        REQUIRE(doc->getLevel() == 3);
        REQUIRE(doc->getVersion() == 2);
        REQUIRE(doc->getPackageRequired("spatial") == true);
        REQUIRE(dynamic_cast<libsbml::SpatialModelPlugin *>(
                    doc->getModel()->getPlugin("spatial")) != nullptr);
      }
    }
    GIVEN("Compartment Colours") {
      QRgb col1 = 0xffaaaaaa;
      QRgb col2 = 0xff525252;
      QRgb col3 = 17423;
      WHEN("compartment colours have been assigned") {
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
}

SCENARIO("SBML: import geometry from image", "[sbml][non-gui]") {
  sbml::SbmlDocWrapper s;
  REQUIRE(s.hasGeometryImage == false);
  REQUIRE(s.hasValidGeometry == false);
  REQUIRE(s.isValid == false);
  GIVEN("Single pixel image") {
    QImage img(1, 1, QImage::Format_RGB32);
    QRgb col = QColor(12, 243, 154).rgba();
    img.setPixel(0, 0, col);
    img.save("tmp.png");
    s.importGeometryFromImage("tmp.png");
    REQUIRE(s.hasGeometryImage == true);
    THEN("getCompartmentImage returns image") {
      REQUIRE(s.getCompartmentImage().size() == QSize(1, 1));
      REQUIRE(s.getCompartmentImage().pixel(0, 0) == col);
    }
    THEN("image contains no membranes") {
      // todo
    }
  }
}

SCENARIO("SBML: ABtoC.xml", "[sbml][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  GIVEN("SBML document") {
    WHEN("importSBMLFile called") {
      THEN("find compartments") {
        REQUIRE(s.compartments.size() == 1);
        REQUIRE(s.compartments[0] == "comp");
      }
      THEN("find species") {
        REQUIRE(s.species.size() == 1);
        REQUIRE(s.species["comp"].size() == 3);
        REQUIRE(s.species["comp"][0] == "A");
        REQUIRE(s.species["comp"][1] == "B");
        REQUIRE(s.species["comp"][2] == "C");
      }
    }
    WHEN("add / remove species") {
      REQUIRE(s.isSIdAvailable("_1_stupd_Name") == true);
      REQUIRE(s.species["comp"].size() == 3);
      s.addSpecies("1 stup!d N@ame?", "comp");
      REQUIRE(s.species["comp"].size() == 4);
      REQUIRE(s.species["comp"][3] == "_1_stupd_Name");
      REQUIRE(s.getSpeciesName("_1_stupd_Name") == "1 stup!d N@ame?");
      REQUIRE(s.getSpeciesCompartment("_1_stupd_Name") == "comp");
      REQUIRE(s.isSIdAvailable("_1_stupd_Name") == false);
      REQUIRE(s.getIsSpatial("_1_stupd_Name") == true);
      REQUIRE(s.getIsSpeciesConstant("_1_stupd_Name") == false);
      REQUIRE(s.getDiffusionConstant("_1_stupd_Name") == dbl_approx(1.0));
      REQUIRE(s.getInitialConcentration("_1_stupd_Name") == dbl_approx(0.0));
      // add another species with the same name: GUI appends underscore
      s.addSpecies("1 stup!d N@ame?", "comp");
      REQUIRE(s.species["comp"].size() == 5);
      REQUIRE(s.species["comp"][4] == "_1_stupd_Name_");
      REQUIRE(s.getSpeciesName("_1_stupd_Name_") == "1 stup!d N@ame?");
      REQUIRE(s.getSpeciesCompartment("_1_stupd_Name_") == "comp");
      REQUIRE(s.isSIdAvailable("_1_stupd_Name_") == false);
      REQUIRE(s.getIsSpatial("_1_stupd_Name_") == true);
      REQUIRE(s.getIsSpeciesConstant("_1_stupd_Name_") == false);
      REQUIRE(s.getDiffusionConstant("_1_stupd_Name_") == dbl_approx(1.0));
      REQUIRE(s.getInitialConcentration("_1_stupd_Name_") == dbl_approx(0.0));
      // remove species _1_stupd_Name
      s.removeSpecies("_1_stupd_Name");
      REQUIRE(s.species["comp"].size() == 4);
      REQUIRE(s.species["comp"][0] == "A");
      REQUIRE(s.species["comp"][1] == "B");
      REQUIRE(s.species["comp"][2] == "C");
      REQUIRE(s.species["comp"][3] == "_1_stupd_Name_");
      // remove non-existent species is a no-op
      s.removeSpecies("_1_stupd_Name");
      REQUIRE(s.species["comp"].size() == 4);
      s.removeSpecies("Idontexist");
      REQUIRE(s.species["comp"].size() == 4);
      // remove species involved in a reaction: also removes reaction
      REQUIRE(s.reactions.at("comp").size() == 1);
      s.removeSpecies("A");
      REQUIRE(s.species["comp"].size() == 3);
      REQUIRE(s.reactions.at("comp").size() == 0);
      s.removeSpecies("B");
      REQUIRE(s.species["comp"].size() == 2);
      REQUIRE(s.reactions.at("comp").size() == 0);
      s.removeSpecies("C");
      REQUIRE(s.species["comp"].size() == 1);
      s.removeSpecies("_1_stupd_Name_");
      REQUIRE(s.species["comp"].size() == 0);
    }
    WHEN("image geometry imported, assigned to compartment") {
      QImage img(":/geometry/circle-100x100.png");
      QRgb col = QColor(144, 97, 193).rgba();
      REQUIRE(img.pixel(50, 50) == col);
      img.save("tmp.png");
      s.importGeometryFromImage("tmp.png");
      s.setCompartmentColour("comp", col);
      THEN("getCompartmentImage returns image") {
        REQUIRE(s.getCompartmentImage().size() == QSize(100, 100));
        REQUIRE(s.getCompartmentImage().pixel(50, 50) == col);
      }
      THEN("find membranes") { REQUIRE(s.membraneVec.empty()); }
      THEN("find reactions") {
        REQUIRE(s.reactions.at("comp").size() == 1);
        REQUIRE(s.reactions.at("comp")[0] == "r1");
        auto r = s.getReaction("r1");
        REQUIRE(s.getReactionName("r1") == "r1");
        REQUIRE(r.id == "r1");
        REQUIRE(r.name == "r1");
        REQUIRE(r.products.size() == 1);
        REQUIRE(r.products[0].first == "C");
        REQUIRE(r.products[0].second == dbl_approx(1.0));
        REQUIRE(r.reactants.size() == 2);
        REQUIRE(r.reactants[0].first == "A");
        REQUIRE(r.reactants[0].second == dbl_approx(1.0));
        REQUIRE(r.reactants[1].first == "B");
        REQUIRE(r.reactants[0].second == dbl_approx(1.0));
        REQUIRE(r.constants.size() == 1);
        REQUIRE(r.constants[0].first == "k1");
        REQUIRE(r.constants[0].second == dbl_approx(0.1));
        REQUIRE(r.fullExpression == "A * B * k1");
        REQUIRE(r.inlinedExpression == "A * B * k1");
      }
      THEN("species have default colours") {
        REQUIRE(s.getSpeciesColour("A") == utils::indexedColours()[0]);
        REQUIRE(s.getSpeciesColour("B") == utils::indexedColours()[1]);
        REQUIRE(s.getSpeciesColour("C") == utils::indexedColours()[2]);
      }
      WHEN("species colours changed") {
        QColor newA = QColor(12, 12, 12);
        QColor newB = QColor(123, 321, 1);
        QColor newC = QColor(0, 22, 99);
        s.setSpeciesColour("A", newA);
        s.setSpeciesColour("B", newB);
        s.setSpeciesColour("C", newC);
        REQUIRE(s.getSpeciesColour("A") == newA);
        REQUIRE(s.getSpeciesColour("B") == newB);
        REQUIRE(s.getSpeciesColour("C") == newC);
        s.setSpeciesColour("A", newC);
        REQUIRE(s.getSpeciesColour("A") == newC);
      }
    }
  }
}

SCENARIO("SBML: very-simple-model.xml", "[sbml][non-gui]") {
  QFile f(":/models/very-simple-model.xml");
  f.open(QIODevice::ReadOnly);
  sbml::SbmlDocWrapper s;
  s.importSBMLString(f.readAll().toStdString());
  GIVEN("SBML document") {
    THEN("find compartments") {
      REQUIRE(s.compartments.size() == 3);
      REQUIRE(s.compartments[0] == "c1");
      REQUIRE(s.compartments[1] == "c2");
      REQUIRE(s.compartments[2] == "c3");
    }
    THEN("find species") {
      REQUIRE(s.species.size() == 3);
      REQUIRE(s.species["c1"].size() == 2);
      REQUIRE(s.species["c1"][0] == "A_c1");
      REQUIRE(s.species["c1"][1] == "B_c1");
      REQUIRE(s.species["c2"].size() == 2);
      REQUIRE(s.species["c2"][0] == "A_c2");
      REQUIRE(s.species["c2"][1] == "B_c2");
      REQUIRE(s.species["c3"].size() == 2);
      REQUIRE(s.species["c3"][0] == "A_c3");
      REQUIRE(s.species["c3"][1] == "B_c3");
      REQUIRE(s.getSpeciesCompartment("A_c1") == "c1");
      REQUIRE(s.getSpeciesCompartment("A_c2") == "c2");
      REQUIRE(s.getSpeciesCompartment("A_c3") == "c3");
      REQUIRE(s.getSpeciesCompartment("B_c1") == "c1");
      REQUIRE(s.getSpeciesCompartment("B_c2") == "c2");
      REQUIRE(s.getSpeciesCompartment("B_c3") == "c3");
      REQUIRE(s.getSpeciesCompartmentSize("A_c1") ==
              dbl_approx(s.getCompartmentSize("c1")));
      REQUIRE(s.getSpeciesCompartmentSize("A_c2") ==
              dbl_approx(s.getCompartmentSize("c2")));
      REQUIRE(s.getSpeciesCompartmentSize("B_c3") ==
              dbl_approx(s.getCompartmentSize("c3")));
    }
    WHEN("species name changed") {
      REQUIRE(s.getSpeciesName("A_c1") == "A");
      s.setSpeciesName("A_c1", "long name with Spaces");
      REQUIRE(s.getSpeciesName("A_c1") == "long name with Spaces");
      REQUIRE(s.getSpeciesName("B_c2") == "B");
      s.setSpeciesName("B_c2", "non-alphanumeric chars allowed: @#$%^&*(_");
      REQUIRE(s.getSpeciesName("B_c2") ==
              "non-alphanumeric chars allowed: @#$%^&*(_");
    }
    WHEN("species compartment changed") {
      REQUIRE(s.getSpeciesCompartment("A_c1") == "c1");
      REQUIRE(s.species.at("c1") == QStringList{"A_c1", "B_c1"});
      REQUIRE(s.species.at("c2") == QStringList{"A_c2", "B_c2"});
      REQUIRE(s.species.at("c3") == QStringList{"A_c3", "B_c3"});
      s.setSpeciesCompartment("A_c1", "c2");
      REQUIRE(s.getSpeciesCompartment("A_c1") == "c2");
      REQUIRE(s.species.at("c1") == QStringList{"B_c1"});
      REQUIRE(s.species.at("c2") == QStringList{"A_c2", "B_c2", "A_c1"});
      REQUIRE(s.species.at("c3") == QStringList{"A_c3", "B_c3"});
      s.setSpeciesCompartment("A_c1", "c1");
      REQUIRE(s.getSpeciesCompartment("A_c1") == "c1");
      REQUIRE(s.species.at("c1") == QStringList{"B_c1", "A_c1"});
      REQUIRE(s.species.at("c2") == QStringList{"A_c2", "B_c2"});
      REQUIRE(s.species.at("c3") == QStringList{"A_c3", "B_c3"});
    }
    WHEN("invalid get species compartment call returns empty string") {
      REQUIRE(s.getSpeciesCompartment("non_existent_species").isEmpty());
    }
    WHEN("invalid species compartment change call is a no-op") {
      REQUIRE(s.getSpeciesCompartment("A_c1") == "c1");
      s.setSpeciesCompartment("A_c1", "invalid_compartment");
      REQUIRE(s.getSpeciesCompartment("A_c1") == "c1");
      s.setSpeciesCompartment("invalid_species", "invalid_compartment");
      REQUIRE(s.getSpeciesCompartment("A_c1") == "c1");
    }
    WHEN("check if Model contains given SId") {
      // species
      REQUIRE(s.isSIdAvailable("A_c1") == false);
      REQUIRE(s.isSIdAvailable("A_c1_q") == true);
      // compartments
      REQUIRE(s.isSIdAvailable("c1") == false);
      REQUIRE(s.isSIdAvailable("c12") == true);
      // reactions
      REQUIRE(s.isSIdAvailable("A_uptake") == false);
      // (non-local) parameters
      REQUIRE(s.isSIdAvailable("B_c2_diffusionConstant") == false);
      REQUIRE(s.isSIdAvailable("A_c3_diffusionConstant") == false);
      REQUIRE(s.isSIdAvailable("A_c3_diffusion_Constant") == true);
      REQUIRE(s.isSIdAvailable("A_c3_diffusionConst") == true);
      REQUIRE(s.isSIdAvailable("x") == false);
      REQUIRE(s.isSIdAvailable("y") == false);
    }
    WHEN("check if Geometry (spatial part of model) contains given SpId") {
      REQUIRE(s.isSpatialIdAvailable("xCoord") == false);
      REQUIRE(s.isSpatialIdAvailable("xCoor") == true);
      REQUIRE(s.isSpatialIdAvailable("xBoundaryMin") == false);
      REQUIRE(s.isSpatialIdAvailable("c1_domain") == false);
      REQUIRE(s.isSpatialIdAvailable("c1_domainType") == false);
      REQUIRE(s.isSpatialIdAvailable("c1_sampledVolume") == false);
      REQUIRE(s.isSpatialIdAvailable("c1_triangles") == false);
      REQUIRE(s.isSpatialIdAvailable("geometry") == false);
      REQUIRE(s.isSpatialIdAvailable("geometryImage") == false);
      REQUIRE(s.isSpatialIdAvailable("parametricGeometry") == false);
      REQUIRE(s.isSpatialIdAvailable("spatialPoints") == false);
    }
  }
}

SCENARIO("SBML: yeast-glycolysis.xml", "[sbml][non-gui][inlining]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::yeast_glycolysis().xml));
  // write SBML document to file
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");

  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  GIVEN("SBML document") {
    WHEN("importSBMLFile called") {
      THEN("find compartments") {
        REQUIRE(s.compartments.size() == 1);
        REQUIRE(s.compartments[0] == "compartment");
      }
      THEN("find species") {
        REQUIRE(s.species.size() == 1);
        REQUIRE(s.species["compartment"].size() == 25);
      }
    }
  }
  GIVEN("inline fn: Glycogen_synthesis_kinetics") {
    std::string expr = "Glycogen_synthesis_kinetics(abc)";
    std::string inlined = "(abc)";
    THEN("return inlined fn") { REQUIRE(s.inlineExpr(expr) == inlined); }
  }
  GIVEN("inline fn: ATPase_0") {
    std::string expr = "ATPase_0( a,b)";
    std::string inlined = "(b * a)";
    THEN("return inlined fn") { REQUIRE(s.inlineExpr(expr) == inlined); }
  }
  GIVEN("inline fn: PDC_kinetics") {
    std::string expr = "PDC_kinetics(a,V,k,n)";
    std::string inlined = "(V * (a / k)^n / (1 + (a / k)^n))";
    THEN("return inlined fn") { REQUIRE(s.inlineExpr(expr) == inlined); }
  }
}

SCENARIO("SBML: load model, refine mesh, save", "[sbml][mesh][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  REQUIRE(s.mesh->getBoundaryMaxPoints(1) == 16);
  REQUIRE(s.mesh->getCompartmentMaxTriangleArea(0) == 72);
  REQUIRE(s.mesh->getVertices().size() == 2 * 44);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 70);
  // refine boundary and mesh
  s.mesh->setBoundaryMaxPoints(1, 20);
  s.mesh->setCompartmentMaxTriangleArea(0, 32);
  REQUIRE(s.mesh->getVertices().size() == 2 * 89);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 148);
  // save SBML doc
  s.exportSBMLFile("tmp.xml");

  // import again
  sbml::SbmlDocWrapper s2;
  s2.importSBMLFile("tmp.xml");
  REQUIRE(s2.mesh->getBoundaryMaxPoints(1) == 20);
  REQUIRE(s2.mesh->getCompartmentMaxTriangleArea(0) == 32);
  REQUIRE(s2.mesh->getVertices().size() == 2 * 89);
  REQUIRE(s2.mesh->getTriangleIndices(0).size() == 3 * 148);
}

SCENARIO("SBML: load model, change size of geometry, save",
         "[sbml][mesh][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  std::vector<double> v = s.mesh->getVertices();
  REQUIRE(s.mesh->getBoundaryMaxPoints(1) == 16);
  REQUIRE(s.mesh->getCompartmentMaxTriangleArea(0) == 72);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 70);
  REQUIRE(v.size() == 2 * 44);
  REQUIRE(v[0] == dbl_approx(17.0));
  REQUIRE(v[1] == dbl_approx(45.0));
  REQUIRE(v[2] == dbl_approx(17.0));
  REQUIRE(v[3] == dbl_approx(57.0));
  REQUIRE(s.getCompartmentSize("comp") == dbl_approx(3149.0));
  // change size of geometry, but not of compartments
  s.setPixelWidth(0.01);
  v = s.mesh->getVertices();
  REQUIRE(v[0] == dbl_approx(0.17));
  REQUIRE(v[1] == dbl_approx(0.45));
  REQUIRE(v[2] == dbl_approx(0.17));
  REQUIRE(v[3] == dbl_approx(0.57));
  REQUIRE(s.getCompartmentSize("comp") == dbl_approx(3149.0));
  // change size of geometry & update compartment volumes accordingly
  s.setPixelWidth(1.6e-13);
  s.setCompartmentSizeFromImage("comp");
  v = s.mesh->getVertices();
  REQUIRE(v[0] == dbl_approx(2.72e-12));
  REQUIRE(v[1] == dbl_approx(7.2e-12));
  REQUIRE(v[2] == dbl_approx(2.72e-12));
  REQUIRE(v[3] == dbl_approx(9.12e-12));
  REQUIRE(s.getCompartmentSize("comp") == dbl_approx(8.06144e-20));
}

SCENARIO("SBML: Delete mesh annotation, load as read-only mesh",
         "[sbml][mesh][non-gui]") {
  // delete mesh info annotation
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
  auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
      doc->getModel()->getPlugin("spatial"));
  auto *geom = plugin->getGeometry();
  libsbml::ParametricGeometry *parageom = nullptr;
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (geom->getGeometryDefinition(i)->isParametricGeometry()) {
      parageom = dynamic_cast<libsbml::ParametricGeometry *>(
          geom->getGeometryDefinition(i));
    }
  }
  auto *annotation = parageom->getAnnotation();
  annotation->removeChildren();

  // load model: without annotation should load as read-only mesh
  sbml::SbmlDocWrapper s;
  std::unique_ptr<char, decltype(&std::free)> xmlChar(
      libsbml::writeSBMLToString(doc.get()), &std::free);
  s.importSBMLString(xmlChar.get());

  REQUIRE(s.mesh->isReadOnly() == true);
  REQUIRE(s.mesh->getVertices().size() == 2 * 44);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 70);

  // changing maxBoundaryPoints or maxTriangleAreas is a no-op:
  s.mesh->setBoundaryMaxPoints(1, 99);
  s.mesh->setCompartmentMaxTriangleArea(0, 3);
  REQUIRE(s.mesh->getVertices().size() == 2 * 44);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 70);

  // save SBML doc
  s.exportSBMLFile("tmp.xml");
  // import again: mesh is still read-only
  sbml::SbmlDocWrapper s2;
  s2.importSBMLFile("tmp.xml");
  REQUIRE(s2.mesh->isReadOnly() == true);
  REQUIRE(s2.mesh->getVertices().size() == 2 * 44);
  REQUIRE(s2.mesh->getTriangleIndices(0).size() == 3 * 70);
}
