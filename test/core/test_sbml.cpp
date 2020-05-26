#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QFile>

#include "catch_wrapper.hpp"
#include "mesh.hpp"
#include "sbml.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "sbml_test_data/yeast_glycolysis.hpp"
#include "utils.hpp"

// avoid using std::to_string because of "multiple definition of vsnprintf"
// mingw issue on windows
template <typename T>
static std::string toString(const T &x) {
  std::ostringstream ss;
  ss << x;
  return ss.str();
}

static void createSBMLlvl2doc(const std::string &filename) {
  std::unique_ptr<libsbml::SBMLDocument> document(
      new libsbml::SBMLDocument(2, 4));
  // create model
  auto *model = document->createModel();
  // create two compartments of different size
  for (int i = 0; i < 2; ++i) {
    auto *comp = model->createCompartment();
    comp->setId("compartment" + toString(i));
    comp->setSize(1e-10 * i);
  }
  // create 2 species inside first compartment with initialConcentration set
  for (int i = 0; i < 2; ++i) {
    auto *spec = model->createSpecies();
    spec->setId("spec" + toString(i) + "c0");
    spec->setCompartment("compartment0");
    spec->setInitialConcentration(i * 1e-12);
  }
  // create 3 species inside second compartment with initialAmount set
  for (int i = 0; i < 3; ++i) {
    auto *spec = model->createSpecies();
    spec->setId("spec" + toString(i) + "c1");
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

SCENARIO("SBML: import SBML doc without geometry", "[core][sbml]") {
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

    REQUIRE(geom->getNumGeometryDefinitions() == 0);
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
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(1, 0) == qRgb(0, 0, 0));
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(0, 0) == 0);
      REQUIRE(s.getConcentrationImage("spec0c0").pixel(2, 0) == 0);
      // set spec1c1conc to zero -> black pixel
      s.setSampledFieldConcentration("spec1c1", {0.0, 0.0, 0.0});
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(0, 0) == 0);
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(1, 0) == 0);
      REQUIRE(s.getConcentrationImage("spec1c1").pixel(2, 0) == qRgb(0, 0, 0));
      s.setSampledFieldConcentration("spec2c1", {0.0, 0.0, 0.0});
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
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(1, 0) == qRgb(0, 0, 0));
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(0, 0) == 0);
      REQUIRE(s2.getConcentrationImage("spec0c0").pixel(2, 0) == 0);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(0, 0) == 0);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(1, 0) == 0);
      REQUIRE(s2.getConcentrationImage("spec1c1").pixel(2, 0) == qRgb(0, 0, 0));
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(0, 0) == 0);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(1, 0) == 0);
      REQUIRE(s2.getConcentrationImage("spec2c1").pixel(2, 0) == qRgb(0, 0, 0));

      CAPTURE(s2.getDiffusionConstant("spec0c0"));
      CAPTURE(s2.getDiffusionConstant("spec1c0"));
      CAPTURE(s2.getDiffusionConstant("spec0c1"));
      REQUIRE(s2.getDiffusionConstant("spec0c0") == dbl_approx(0.123));
      REQUIRE(s2.getDiffusionConstant("spec1c0") == dbl_approx(0.999999));
      REQUIRE(s2.getDiffusionConstant("spec0c1") == dbl_approx(23.1 + 1e-12));
    }
  }
}

SCENARIO("SBML: name clashes", "[core][sbml]") {
  std::unique_ptr<libsbml::SBMLDocument> document(
      new libsbml::SBMLDocument(2, 4));
  // create model
  auto *model = document->createModel();
  // create 3 compartments with the same name
  for (int i = 0; i < 3; ++i) {
    auto *comp = model->createCompartment();
    comp->setId("compartment" + toString(i));
    comp->setName("comp");
  }
  // create 3 species inside first two compartments with the same names
  for (int iComp = 0; iComp < 2; ++iComp) {
    for (int i = 0; i < 3; ++i) {
      auto *spec = model->createSpecies();
      spec->setId("spec" + toString(i) + "c" + toString(iComp));
      spec->setName("spec");
      spec->setCompartment("compartment" + toString(iComp));
    }
  }
  std::unique_ptr<char, decltype(&std::free)> xmlChar(
      libsbml::writeSBMLToString(document.get()), &std::free);
  sbml::SbmlDocWrapper s;
  s.importSBMLString(xmlChar.get());
  REQUIRE(s.compartments.size() == 3);
  REQUIRE(s.compartmentNames[0] == "comp");
  REQUIRE(s.compartmentNames[1] == "comp_");
  REQUIRE(s.compartmentNames[2] == "comp__");
  REQUIRE(s.species.size() == 3);
  REQUIRE(s.species["compartment0"].size() == 3);
  REQUIRE(s.getSpeciesName(s.species["compartment0"][0]) == "spec");
  REQUIRE(s.getSpeciesName(s.species["compartment0"][1]) == "spec_comp");
  REQUIRE(s.getSpeciesName(s.species["compartment0"][2]) == "spec_comp_comp");
  REQUIRE(s.species["compartment1"].size() == 3);
  REQUIRE(s.getSpeciesName(s.species["compartment1"][0]) == "spec_comp_");
  REQUIRE(s.getSpeciesName(s.species["compartment1"][1]) == "spec_comp__comp_");
  REQUIRE(s.getSpeciesName(s.species["compartment1"][2]) ==
          "spec_comp__comp__comp_");
  REQUIRE(s.species["compartment2"].size() == 0);
  // add a species with a clashing name
  auto newName = s.addSpecies("spec", "compartment0");
  REQUIRE(newName == "spec_comp_comp_comp");
  auto newName2 = s.addSpecies("spec", "compartment1");
  REQUIRE(newName2 == "spec_comp__comp__comp__comp_");
}

SCENARIO("SBML: import SBML level 2 document", "[core][sbml]") {
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
      REQUIRE(r.species.size() == 2);
      REQUIRE(r.species[0].id == "spec1c0");
      REQUIRE(r.species[1].id == "spec0c0");
      REQUIRE(r.expression == "5 * spec0c0 / compartment0");
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

SCENARIO("SBML: create new model, import geometry from image", "[core][sbml]") {
  sbml::SbmlDocWrapper s;
  REQUIRE(s.hasGeometryImage == false);
  REQUIRE(s.hasValidGeometry == false);
  REQUIRE(s.isValid == false);
  s.createSBMLFile("new");
  REQUIRE(s.isValid == true);
  REQUIRE(s.hasGeometryImage == false);
  REQUIRE(s.hasValidGeometry == false);
  GIVEN("Single pixel image") {
    QImage img(1, 1, QImage::Format_RGB32);
    QRgb col = QColor(12, 243, 154).rgba();
    img.setPixel(0, 0, col);
    img.save("tmp.png");
    s.importGeometryFromImage("tmp.png");
    REQUIRE(s.isValid == true);
    REQUIRE(s.hasGeometryImage == true);
    REQUIRE(s.hasValidGeometry == false);
    THEN("getCompartmentImage returns image") {
      REQUIRE(s.getCompartmentImage().size() == QSize(1, 1));
      REQUIRE(s.getCompartmentImage().pixel(0, 0) == col);
    }
  }
}

SCENARIO("SBML: import uint8 sampled field", "[core][sbml]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/test/models/very-simple-model-uint8.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  const auto &img = s.getCompartmentImage();
  REQUIRE(img.colorCount() == 3);
  REQUIRE(s.getCompartmentColour("c1") == qRgb(127, 127, 127));
  REQUIRE(s.getCompartmentColour("c2") == qRgb(0, 0, 0));
  REQUIRE(s.getCompartmentColour("c3") == qRgb(255, 255, 255));
  // undefined compartment sizes -> defaulted to 1
  REQUIRE(s.getCompartmentSize("c1") == dbl_approx(1.0));
  REQUIRE(s.getCompartmentSize("c2") == dbl_approx(1.0));
  REQUIRE(s.getCompartmentSize("c3") == dbl_approx(1.0));
  // species A_c1 has initialAmount 11 -> converted to concentration
  REQUIRE(s.getInitialConcentration("A_c1") == dbl_approx(11.0));
  // species A_c2 has no initialAmount or initialConcentration -> defaulted to 0
  REQUIRE(s.getInitialConcentration("A_c2") == dbl_approx(0.0));
}

SCENARIO("SBML: ABtoC.xml", "[core][sbml]") {
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
      THEN("find species geometry") {
        auto g = s.getSpeciesGeometry("A");
        REQUIRE(g.modelUnits.getAmount().symbol ==
                s.getModelUnits().getAmount().symbol);
        REQUIRE(g.pixelWidth == dbl_approx(1.0));
        REQUIRE(g.physicalOrigin.x() == dbl_approx(0.0));
        REQUIRE(g.physicalOrigin.y() == dbl_approx(0.0));
        REQUIRE(g.compartmentPoints.size() == 3149);
        REQUIRE(g.compartmentImageSize == QSize(100, 100));
      }
    }
    WHEN("change model name") {
      REQUIRE(s.getName() == "");
      QString newName = "new model name";
      s.setName(newName);
      REQUIRE(s.getName() == newName);
    }
    WHEN("add / remove compartment") {
      // add compartment
      s.addCompartment("newComp !");
      REQUIRE(s.compartments.size() == 2);
      REQUIRE(s.compartments[0] == "comp");
      REQUIRE(s.compartments[1] == "newComp_");
      REQUIRE(s.compartmentNames.size() == 2);
      REQUIRE(s.compartmentNames[0] == "comp");
      REQUIRE(s.compartmentNames[1] == "newComp !");
      REQUIRE(s.getCompartmentName("newComp_") == "newComp !");
      REQUIRE(s.species.at("newComp_").size() == 0);
      REQUIRE(s.reactions.at("newComp_").size() == 0);
      // add compartment with same name: GUI appends underscore
      s.addCompartment("newComp !");
      REQUIRE(s.compartments.size() == 3);
      REQUIRE(s.compartments[0] == "comp");
      REQUIRE(s.compartments[1] == "newComp_");
      REQUIRE(s.compartments[2] == "newComp__");
      REQUIRE(s.compartmentNames.size() == 3);
      REQUIRE(s.compartmentNames[0] == "comp");
      REQUIRE(s.compartmentNames[1] == "newComp !");
      REQUIRE(s.compartmentNames[2] == "newComp !_");
      // remove compartment
      s.removeCompartment("newComp__");
      REQUIRE(s.compartments.size() == 2);
      REQUIRE(s.compartmentNames.size() == 2);
      // add species to compartment
      s.addSpecies("q", "newComp_");
      REQUIRE(s.species.at("newComp_").size() == 1);
      REQUIRE(s.species.at("newComp_")[0] == "q");
      s.addSpecies(" !Wq", "newComp_");
      REQUIRE(s.species.at("newComp_").size() == 2);
      REQUIRE(s.species.at("newComp_")[0] == "q");
      REQUIRE(s.species.at("newComp_")[1] == "__Wq");
      REQUIRE(s.getSpeciesName("__Wq") == " !Wq");
      // removing a compartment also removes the species in it, and any
      // reactions involving them
      REQUIRE(s.reactions.at("comp").size() == 1);
      s.removeCompartment("comp");
      REQUIRE(s.reactions.find("comp") == s.reactions.cend());
      REQUIRE(s.species.find("comp") == s.species.cend());
      // no compartments left, so no species either
      s.removeCompartment("newComp_");
      REQUIRE(s.species.empty());
    }
    WHEN("add / remove species") {
      REQUIRE(s.species["comp"].size() == 3);
      s.addSpecies("1 stup!d N@ame?", "comp");
      REQUIRE(s.species["comp"].size() == 4);
      REQUIRE(s.species["comp"][3] == "_1_stupd_Name");
      REQUIRE(s.getSpeciesName("_1_stupd_Name") == "1 stup!d N@ame?");
      REQUIRE(s.getSpeciesCompartment("_1_stupd_Name") == "comp");
      REQUIRE(s.getIsSpatial("_1_stupd_Name") == true);
      REQUIRE(s.getIsSpeciesConstant("_1_stupd_Name") == false);
      REQUIRE(s.getDiffusionConstant("_1_stupd_Name") == dbl_approx(1.0));
      REQUIRE(s.getInitialConcentration("_1_stupd_Name") == dbl_approx(0.0));
      // add another species with the same name: GUI appends _compartmentName
      s.addSpecies("1 stup!d N@ame?", "comp");
      REQUIRE(s.species["comp"].size() == 5);
      REQUIRE(s.species["comp"][4] == "_1_stupd_Name_comp");
      REQUIRE(s.getSpeciesName("_1_stupd_Name_comp") == "1 stup!d N@ame?_comp");
      REQUIRE(s.getSpeciesCompartment("_1_stupd_Name_comp") == "comp");
      REQUIRE(s.getIsSpatial("_1_stupd_Name_comp") == true);
      REQUIRE(s.getIsSpeciesConstant("_1_stupd_Name_comp") == false);
      REQUIRE(s.getDiffusionConstant("_1_stupd_Name_comp") == dbl_approx(1.0));
      REQUIRE(s.getInitialConcentration("_1_stupd_Name_comp") ==
              dbl_approx(0.0));
      // remove species _1_stupd_Name
      s.removeSpecies("_1_stupd_Name_comp");
      REQUIRE(s.species["comp"].size() == 4);
      REQUIRE(s.species["comp"][0] == "A");
      REQUIRE(s.species["comp"][1] == "B");
      REQUIRE(s.species["comp"][2] == "C");
      REQUIRE(s.species["comp"][3] == "_1_stupd_Name");
      // remove non-existent species is a no-op
      s.removeSpecies("QQ_1_stupd_NameQQ");
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
      s.removeSpecies("_1_stupd_Name");
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
        REQUIRE(r.species.size() == 3);
        REQUIRE(r.species[0].id == "C");
        REQUIRE(r.species[0].value == dbl_approx(1.0));
        REQUIRE(r.species[1].id == "A");
        REQUIRE(r.species[1].value == dbl_approx(-1.0));
        REQUIRE(r.species[2].id == "B");
        REQUIRE(r.species[2].value == dbl_approx(-1.0));
        REQUIRE(r.constants.size() == 1);
        REQUIRE(r.constants[0].id == "k1");
        REQUIRE(r.constants[0].name == "k1");
        REQUIRE(r.constants[0].value == dbl_approx(0.1));
        REQUIRE(r.expression == "A * B * k1");
      }
      THEN("species have default colours") {
        REQUIRE(s.getSpeciesColour("A") == utils::indexedColours()[0].rgb());
        REQUIRE(s.getSpeciesColour("B") == utils::indexedColours()[1].rgb());
        REQUIRE(s.getSpeciesColour("C") == utils::indexedColours()[2].rgb());
      }
      WHEN("species colours changed") {
        auto newA = QColor(12, 12, 12).rgb();
        auto newB = QColor(123, 321, 1).rgb();
        auto newC = QColor(0, 22, 99).rgb();
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

SCENARIO("SBML: very-simple-model.xml", "[core][sbml]") {
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
    }
    WHEN("species name changed") {
      REQUIRE(s.getSpeciesName("A_c1") == "A_out");
      s.setSpeciesName("A_c1", "long name with Spaces");
      REQUIRE(s.getSpeciesName("A_c1") == "long name with Spaces");
      REQUIRE(s.getSpeciesName("B_c2") == "B_cell");
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
    WHEN("add/remove empty reaction") {
      REQUIRE(s.reactions.at("c2").size() == 0);
      s.addReaction("re ac~!1", "c2");
      REQUIRE(s.reactions.at("c2").size() == 1);
      REQUIRE(s.reactions.at("c2")[0].toStdString() == "re_ac1");
      auto r = s.getReaction("re_ac1");
      REQUIRE(r.id == "re_ac1");
      REQUIRE(r.name == "re ac~!1");
      REQUIRE(r.locationId == "c2");
      REQUIRE(r.species.empty() == true);
      REQUIRE(r.constants.empty() == true);
      REQUIRE(r.expression.empty() == true);
      s.removeReaction("re_ac1");
      REQUIRE(s.reactions.at("c2").size() == 0);
    }
    WHEN("set reaction") {
      REQUIRE(s.reactions.at("c2").size() == 0);
      REQUIRE(s.reactions.at("c3").size() == 1);
      s.addReaction("re ac~!1", "c2");
      auto r = s.getReaction("re_ac1");
      REQUIRE(r.locationId == "c2");
      r.name = "new Name";
      r.locationId = "c3";
      r.species.push_back({"A_c3", "A c3", 1});
      r.species.push_back({"B_c3", "B c3", -2});
      r.constants.push_back({"c1", "const 1", 0.2});
      r.expression = "0.2 + A_c3 * B_c3 * c1";
      s.setReaction(r);
      auto r2 = s.getReaction("re_ac1");
      REQUIRE(r2.id == "re_ac1");
      REQUIRE(r2.name == "new Name");
      REQUIRE(r2.locationId == "c3");
      REQUIRE(r2.species[0].id == "A_c3");
      REQUIRE(r2.species[0].value == dbl_approx(1));
      REQUIRE(r2.species[1].id == "B_c3");
      REQUIRE(r2.species[1].value == dbl_approx(-2));
      REQUIRE(r2.constants[0].id == "c1");
      REQUIRE(r2.constants[0].value == dbl_approx(0.2));
      REQUIRE(r2.expression == "0.2 + A_c3 * B_c3 * c1");
    }
    WHEN("change reaction location") {
      REQUIRE(s.reactions.at("c2").size() == 0);
      REQUIRE(s.reactions.at("c3").size() == 1);
      s.setReactionLocation("A_B_conversion", "c2");
      REQUIRE(s.reactions.at("c2").size() == 1);
      REQUIRE(s.reactions.at("c3").size() == 0);
      auto r = s.getReaction("A_B_conversion");
      REQUIRE(r.id == "A_B_conversion");
      REQUIRE(r.name == "A to B conversion");
      REQUIRE(r.locationId == "c2");
      REQUIRE(r.species.empty() == true);
      REQUIRE(r.constants.empty() == true);
      REQUIRE(r.expression.empty() == true);
      s.setReactionLocation("A_B_conversion", "c1");
      REQUIRE(s.reactions.at("c1").size() == 1);
      REQUIRE(s.reactions.at("c2").size() == 0);
      auto r2 = s.getReaction("A_B_conversion");
      REQUIRE(r2.id == "A_B_conversion");
      REQUIRE(r2.name == "A to B conversion");
      REQUIRE(r2.locationId == "c1");
      s.removeReaction("A_B_conversion");
      REQUIRE(s.reactions.at("c1").size() == 0);
    }
  }
}

SCENARIO("SBML: yeast-glycolysis.xml", "[core][sbml][inlining]") {
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
      THEN("find functions") {
        REQUIRE(s.functions.size() == 17);
        REQUIRE(s.functions[0].toStdString() == "HK_kinetics");
        auto f = s.getFunctionDefinition("HK_kinetics");
        REQUIRE(f.id == "HK_kinetics");
        REQUIRE(f.name == "HK kinetics");
        REQUIRE(f.arguments.size() == 10);
        REQUIRE(f.arguments[0] == "A");
        REQUIRE(f.arguments[1] == "B");
        REQUIRE(f.arguments[2] == "P");
        REQUIRE(f.arguments[3] == "Q");
        REQUIRE(f.expression ==
                "Vmax * (A * B / (Kglc * Katp) - P * Q / (Kglc * Katp * Keq)) "
                "/ ((1 + A / Kglc + P / Kg6p) * (1 + B / Katp + Q / Kadp))");
        auto emptyF = s.getFunctionDefinition("non_existent_function");
        REQUIRE(emptyF.id.empty() == true);
        REQUIRE(emptyF.name.empty() == true);
        REQUIRE(emptyF.arguments.empty() == true);
        REQUIRE(emptyF.expression.empty() == true);
      }
    }
  }
  GIVEN("inline fn: Glycogen_synthesis_kinetics") {
    std::string expr = "Glycogen_synthesis_kinetics(abc)";
    std::string inlined = "(abc)";
    REQUIRE(s.inlineExpr(expr) == inlined);
  }
  GIVEN("inline fn: ATPase_0") {
    std::string expr = "ATPase_0( a,b)";
    std::string inlined = "(b * a)";
    REQUIRE(s.inlineExpr(expr) == inlined);
  }
  GIVEN("inline fn: PDC_kinetics") {
    std::string expr = "PDC_kinetics(a,V,k,n)";
    std::string inlined = "(V * (a / k)^n / (1 + (a / k)^n))";
    REQUIRE(s.inlineExpr(expr) == inlined);
  }
  GIVEN("edit function: PDC_kinetics") {
    auto f = s.getFunctionDefinition("PDC_kinetics");
    REQUIRE(f.arguments.size() == 4);
    REQUIRE(f.arguments[0] == "A");
    REQUIRE(f.arguments[1] == "Vmax");
    REQUIRE(f.arguments[2] == "Kpyr");
    REQUIRE(f.arguments[3] == "nH");
    f.arguments.push_back("x");
    f.name = "newName!";
    f.expression = "(V*(x/k)^n/(1+(a/k)^n))";
    s.setFunctionDefinition(f);
    auto newF = s.getFunctionDefinition("PDC_kinetics");
    REQUIRE(newF.name == "newName!");
    REQUIRE(newF.arguments.size() == 5);
    REQUIRE(newF.arguments[0] == "A");
    REQUIRE(newF.arguments[1] == "Vmax");
    REQUIRE(newF.arguments[2] == "Kpyr");
    REQUIRE(newF.arguments[3] == "nH");
    REQUIRE(newF.arguments[4] == "x");
    REQUIRE(newF.expression == "V * (x / k)^n / (1 + (a / k)^n)");
    std::string expr = "PDC_kinetics(a,V,k,n,Q)";
    std::string inlined = "(V * (Q / k)^n / (1 + (a / k)^n))";
    REQUIRE(s.inlineExpr(expr) == inlined);
    REQUIRE(std::find(s.functions.cbegin(), s.functions.cend(),
                      "PDC_kinetics") != s.functions.cend());
    s.removeFunction("PDC_kinetics");
    REQUIRE(std::find(s.functions.cbegin(), s.functions.cend(),
                      "PDC_kinetics") == s.functions.cend());
    REQUIRE(s.getFunctionDefinition("PDC_kinetics").name.empty());
    // removing a non-existent function is no-op
    REQUIRE(s.functions.size() == 16);
    REQUIRE_NOTHROW(s.removeFunction("I don't exist"));
    REQUIRE(s.functions.size() == 16);
  }
  GIVEN("add function") {
    s.addFunction("func N~!me");
    auto f = s.getFunctionDefinition("func_Nme");
    REQUIRE(f.id == "func_Nme");
    REQUIRE(f.name == "func N~!me");
    REQUIRE(f.arguments.empty() == true);
    REQUIRE(f.expression == "0");
  }
}

SCENARIO("SBML: load model, refine mesh, save", "[core][sbml][mesh]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  REQUIRE(s.mesh->getBoundaryMaxPoints(0) == 16);
  REQUIRE(s.mesh->getNumBoundaries() == 1);
  REQUIRE(s.mesh->getCompartmentMaxTriangleArea(0) == 92);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 48);
  // refine boundary and mesh
  s.mesh->setBoundaryMaxPoints(0, 20);
  s.mesh->setCompartmentMaxTriangleArea(0, 32);
  REQUIRE(s.mesh->getNumBoundaries() == 1);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 140);
  // save SBML doc
  s.exportSBMLFile("tmp.xml");

  // import again
  sbml::SbmlDocWrapper s2;
  s2.importSBMLFile("tmp.xml");
  REQUIRE(s.mesh->getNumBoundaries() == 1);
  REQUIRE(s2.mesh->getBoundaryMaxPoints(0) == 20);
  REQUIRE(s2.mesh->getCompartmentMaxTriangleArea(0) == 32);
  REQUIRE(s2.mesh->getTriangleIndices(0).size() == 3 * 140);
}

SCENARIO("SBML: load model, change size of geometry, save",
         "[core][sbml][mesh]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  std::vector<double> v = s.mesh->getVertices();
  REQUIRE(s.mesh->getBoundaryMaxPoints(0) == 16);
  REQUIRE(s.mesh->getCompartmentMaxTriangleArea(0) == 92);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 48);
  double v0 = v[0];
  double v1 = v[1];
  double v2 = v[2];
  double v3 = v[3];
  REQUIRE(s.getCompartmentSize("comp") == dbl_approx(3149.0));
  // change size of geometry, but not of compartments
  double a = 0.01;
  s.setPixelWidth(a);
  v = s.mesh->getVertices();
  REQUIRE(v[0] == dbl_approx(a * v0));
  REQUIRE(v[1] == dbl_approx(a * v1));
  REQUIRE(v[2] == dbl_approx(a * v2));
  REQUIRE(v[3] == dbl_approx(a * v3));
  REQUIRE(s.getCompartmentSize("comp") == dbl_approx(3149.0));
  // change size of geometry & update compartment volumes accordingly
  a = 1.6e-13;
  s.setPixelWidth(a);
  s.setCompartmentSizeFromImage("comp");
  v = s.mesh->getVertices();
  REQUIRE(v[0] == dbl_approx(a * v0));
  REQUIRE(v[1] == dbl_approx(a * v1));
  REQUIRE(v[2] == dbl_approx(a * v2));
  REQUIRE(v[3] == dbl_approx(a * v3));
  REQUIRE(s.getCompartmentSize("comp") == dbl_approx(8.06144e-20));
}

SCENARIO("SBML: Delete mesh annotation, load as read-only mesh",
         "[core][sbml][mesh]") {
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
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 48);

  // changing maxBoundaryPoints or maxTriangleAreas is a no-op:
  s.mesh->setBoundaryMaxPoints(0, 99);
  s.mesh->setCompartmentMaxTriangleArea(0, 3);
  REQUIRE(s.mesh->getTriangleIndices(0).size() == 3 * 48);

  // save SBML doc
  s.exportSBMLFile("tmp.xml");
  // import again: mesh is still read-only
  sbml::SbmlDocWrapper s2;
  s2.importSBMLFile("tmp.xml");
  REQUIRE(s2.mesh->isReadOnly() == true);
  REQUIRE(s2.mesh->getTriangleIndices(0).size() == 3 * 48);
}
