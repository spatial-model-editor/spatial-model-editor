#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "sbml_test_data/yeast_glycolysis.hpp"
#include "utils.hpp"
#include <QFile>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;

// avoid using std::to_string because of "multiple definition of vsnprintf"
// mingw issue on windows
template <typename T> static std::string toString(const T &x) {
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
    comp->setSpatialDimensions(static_cast<unsigned int>(3));
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

SCENARIO("SBML: import SBML doc without geometry",
         "[core/model/model][core/model][core][model]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmp.xml");
  // import SBML model
  model::Model s;
  s.importSBMLFile("tmp.xml");
  REQUIRE(s.getIsValid() == true);
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
    for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
      REQUIRE(model->getCompartment(i)->getSpatialDimensions() == 2);
    }
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
    s.getGeometry().importGeometryFromImage(
        QImage(":/geometry/single-pixels-3x1.png"));
    s.getCompartments().setColour("compartment0", 0xffaaaaaa);
    s.getCompartments().setColour("compartment1", 0xff525252);
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
    THEN("import concentration & set diff constants") {
      // import concentration
      s.getSpecies().setSampledFieldConcentration("spec0c0", {0.0, 0.0, 0.0});
      REQUIRE(s.getSpecies().getConcentrationImage("spec0c0").size() ==
              QSize(3, 1));
      REQUIRE(s.getSpecies().getConcentrationImage("spec0c0").pixel(1, 0) ==
              qRgb(0, 0, 0));
      REQUIRE(s.getSpecies().getConcentrationImage("spec0c0").pixel(0, 0) == 0);
      REQUIRE(s.getSpecies().getConcentrationImage("spec0c0").pixel(2, 0) == 0);
      // set spec1c1conc to zero -> black pixel
      s.getSpecies().setSampledFieldConcentration("spec1c1", {0.0, 0.0, 0.0});
      REQUIRE(s.getSpecies().getConcentrationImage("spec1c1").pixel(0, 0) == 0);
      REQUIRE(s.getSpecies().getConcentrationImage("spec1c1").pixel(1, 0) == 0);
      REQUIRE(s.getSpecies().getConcentrationImage("spec1c1").pixel(2, 0) ==
              qRgb(0, 0, 0));
      s.getSpecies().setSampledFieldConcentration("spec2c1", {0.0, 0.0, 0.0});
      s.getSpecies().setIsSpatial("spec0c0", true);
      s.getSpecies().setIsSpatial("spec1c0", true);
      s.getSpecies().setIsSpatial("spec0c1", true);
      s.getSpecies().setDiffusionConstant("spec0c0", 0.123);
      s.getSpecies().setDiffusionConstant("spec1c0", 0.1);
      s.getSpecies().setDiffusionConstant("spec1c0", 0.999999);
      s.getSpecies().setDiffusionConstant("spec0c1", 23.1 + 1e-12);
      CAPTURE(s.getSpecies().getDiffusionConstant("spec0c0"));
      CAPTURE(s.getSpecies().getDiffusionConstant("spec1c0"));
      CAPTURE(s.getSpecies().getDiffusionConstant("spec0c1"));
      REQUIRE(s.getSpecies().getDiffusionConstant("spec0c0") ==
              dbl_approx(0.123));
      REQUIRE(s.getSpecies().getDiffusionConstant("spec1c0") ==
              dbl_approx(0.999999));
      REQUIRE(s.getSpecies().getDiffusionConstant("spec0c1") ==
              dbl_approx(23.1 + 1e-12));

      // export model
      s.exportSBMLFile("tmp2.xml");
      // import model again, recover concentration & compartment assignments
      model::Model s2;
      s2.importSBMLFile("tmp2.xml");
      REQUIRE(s2.getCompartments().getColour("compartment0") == 0xffaaaaaa);
      REQUIRE(s2.getCompartments().getColour("compartment1") == 0xff525252);
      REQUIRE(s2.getSpecies().getConcentrationImage("spec0c0").pixel(1, 0) ==
              qRgb(0, 0, 0));
      REQUIRE(s2.getSpecies().getConcentrationImage("spec0c0").pixel(0, 0) ==
              0);
      REQUIRE(s2.getSpecies().getConcentrationImage("spec0c0").pixel(2, 0) ==
              0);
      REQUIRE(s2.getSpecies().getConcentrationImage("spec1c1").pixel(0, 0) ==
              0);
      REQUIRE(s2.getSpecies().getConcentrationImage("spec1c1").pixel(1, 0) ==
              0);
      REQUIRE(s2.getSpecies().getConcentrationImage("spec1c1").pixel(2, 0) ==
              qRgb(0, 0, 0));
      REQUIRE(s2.getSpecies().getConcentrationImage("spec2c1").pixel(0, 0) ==
              0);
      REQUIRE(s2.getSpecies().getConcentrationImage("spec2c1").pixel(1, 0) ==
              0);
      REQUIRE(s2.getSpecies().getConcentrationImage("spec2c1").pixel(2, 0) ==
              qRgb(0, 0, 0));

      CAPTURE(s2.getSpecies().getDiffusionConstant("spec0c0"));
      CAPTURE(s2.getSpecies().getDiffusionConstant("spec1c0"));
      CAPTURE(s2.getSpecies().getDiffusionConstant("spec0c1"));
      REQUIRE(s2.getSpecies().getDiffusionConstant("spec0c0") ==
              dbl_approx(0.123));
      REQUIRE(s2.getSpecies().getDiffusionConstant("spec1c0") ==
              dbl_approx(0.999999));
      REQUIRE(s2.getSpecies().getDiffusionConstant("spec0c1") ==
              dbl_approx(23.1 + 1e-12));
    }
  }
}

SCENARIO("SBML: name clashes", "[core/model/model][core/model][core][model]") {
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
  model::Model s;
  s.importSBMLString(xmlChar.get());
  REQUIRE(s.getCompartments().getIds().size() == 3);
  REQUIRE(s.getCompartments().getNames()[0] == "comp");
  REQUIRE(s.getCompartments().getNames()[1] == "comp_");
  REQUIRE(s.getCompartments().getNames()[2] == "comp__");
  REQUIRE(s.getSpecies().getIds("compartment0").size() == 3);
  REQUIRE(s.getSpecies().getName(s.getSpecies().getIds("compartment0")[0]) ==
          "spec");
  REQUIRE(s.getSpecies().getName(s.getSpecies().getIds("compartment0")[1]) ==
          "spec_comp");
  REQUIRE(s.getSpecies().getName(s.getSpecies().getIds("compartment0")[2]) ==
          "spec_comp_comp");
  REQUIRE(s.getSpecies().getIds("compartment1").size() == 3);
  REQUIRE(s.getSpecies().getName(s.getSpecies().getIds("compartment1")[0]) ==
          "spec_comp_");
  REQUIRE(s.getSpecies().getName(s.getSpecies().getIds("compartment1")[1]) ==
          "spec_comp__comp_");
  REQUIRE(s.getSpecies().getName(s.getSpecies().getIds("compartment1")[2]) ==
          "spec_comp__comp__comp_");
  REQUIRE(s.getSpecies().getIds("compartment2").size() == 0);
  // add a species with a clashing name
  auto newName = s.getSpecies().add("spec", "compartment0");
  REQUIRE(newName == "spec_comp_comp_comp");
  auto newName2 = s.getSpecies().add("spec", "compartment1");
  REQUIRE(newName2 == "spec_comp__comp__comp__comp_");
}

SCENARIO("SBML: import SBML level 2 document",
         "[core/model/model][core/model][core][model]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmp.xml");
  // import SBML model
  model::Model s;
  s.importSBMLFile("tmp.xml");
  REQUIRE(s.getIsValid() == true);

  // import geometry image & assign compartments to colours
  s.getGeometry().importGeometryFromImage(
      QImage(":/geometry/single-pixels-3x1.png"));
  s.getCompartments().setColour("compartment0", 0xffaaaaaa);
  s.getCompartments().setColour("compartment1", 0xff525252);

  GIVEN("SBML document & geometry image") {
    THEN("find compartments") {
      REQUIRE(s.getCompartments().getIds().size() == 2);
      REQUIRE(s.getCompartments().getIds()[0] == "compartment0");
      REQUIRE(s.getCompartments().getIds()[1] == "compartment1");
    }
    THEN("find species") {
      REQUIRE(s.getSpecies().getIds("compartment0").size() == 2);
      REQUIRE(s.getSpecies().getIds("compartment0")[0] == "spec0c0");
      REQUIRE(s.getSpecies().getIds("compartment0")[1] == "spec1c0");
      REQUIRE(s.getSpecies().getIds("compartment1").size() == 3);
      REQUIRE(s.getSpecies().getIds("compartment1")[0] == "spec0c1");
      REQUIRE(s.getSpecies().getIds("compartment1")[1] == "spec1c1");
      REQUIRE(s.getSpecies().getIds("compartment1")[2] == "spec2c1");
    }
    THEN("find reaction (divided by compartment volume factor)") {
      const auto &reacs = s.getReactions();
      REQUIRE(reacs.getIds("compartment0").size() == 1);
      REQUIRE(reacs.getIds("compartment0")[0] == "reac1");
      REQUIRE(reacs.getName("reac1") == "reac1");
      REQUIRE(reacs.getLocation("reac1") == "compartment0");
      REQUIRE(reacs.getSpeciesStoichiometry("reac1", "spec1c0") ==
              dbl_approx(1));
      REQUIRE(reacs.getSpeciesStoichiometry("reac1", "spec0c0") ==
              dbl_approx(-1));
      REQUIRE(reacs.getRateExpression("reac1") == "5 * spec0c0 / compartment0");
      REQUIRE(reacs.getScheme("reac1") == "spec0c0 -> spec1c0");
    }
    WHEN("exportSBMLFile called") {
      THEN("exported file is a SBML level (3,2) document with spatial "
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
      QRgb col3 = 0xffffffff;
      WHEN("compartment colours have been assigned") {
        THEN("can get CompartmentID from colour") {
          REQUIRE(s.getCompartments().getIdFromColour(col1) == "compartment0");
          REQUIRE(s.getCompartments().getIdFromColour(col2) == "compartment1");
          REQUIRE(s.getCompartments().getIdFromColour(col3) == "");
        }
        THEN("can get colour from CompartmentID") {
          REQUIRE(s.getCompartments().getColour("compartment0") == col1);
          REQUIRE(s.getCompartments().getColour("compartment1") == col2);
        }
      }
      WHEN("new colour assigned") {
        s.getCompartments().setColour("compartment0", col1);
        s.getCompartments().setColour("compartment1", col2);
        s.getCompartments().setColour("compartment0", col3);
        THEN("unassign old colour mapping") {
          REQUIRE(s.getCompartments().getIdFromColour(col1) == "");
          REQUIRE(s.getCompartments().getIdFromColour(col2) == "compartment1");
          REQUIRE(s.getCompartments().getIdFromColour(col3) == "compartment0");
          REQUIRE(s.getCompartments().getColour("compartment0") == col3);
          REQUIRE(s.getCompartments().getColour("compartment1") == col2);
        }
      }
      WHEN("existing colour re-assigned") {
        s.getCompartments().setColour("compartment0", col1);
        s.getCompartments().setColour("compartment1", col2);
        s.getCompartments().setColour("compartment0", col2);
        THEN("unassign old colour mapping") {
          REQUIRE(s.getCompartments().getIdFromColour(col1) == "");
          REQUIRE(s.getCompartments().getIdFromColour(col2) == "compartment0");
          REQUIRE(s.getCompartments().getIdFromColour(col3) == "");
          REQUIRE(s.getCompartments().getColour("compartment0") == col2);
          REQUIRE(s.getCompartments().getColour("compartment1") == 0);
        }
      }
    }
  }
}

SCENARIO("SBML: create new model, import geometry from image",
         "[core/model/model][core/model][core][model]") {
  model::Model s;
  REQUIRE(s.getGeometry().getHasImage() == false);
  REQUIRE(s.getGeometry().getIsValid() == false);
  REQUIRE(s.getIsValid() == false);
  s.createSBMLFile("new");
  s.getCompartments().add("comp");
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getGeometry().getHasImage() == false);
  REQUIRE(s.getGeometry().getIsValid() == false);
  GIVEN("Single pixel image") {
    QImage img(1, 1, QImage::Format_RGB32);
    QRgb col = QColor(12, 243, 154).rgba();
    img.setPixel(0, 0, col);
    img.save("tmp.png");
    s.getGeometry().importGeometryFromImage(QImage("tmp.png"));
    REQUIRE(s.getIsValid() == true);
    REQUIRE(s.getGeometry().getHasImage() == true);
    REQUIRE(s.getGeometry().getIsValid() == false);
    s.getCompartments().setColour("comp", col);
    REQUIRE(s.getIsValid() == true);
    REQUIRE(s.getGeometry().getHasImage() == true);
    REQUIRE(s.getGeometry().getIsValid() == true);
  }
}

SCENARIO("SBML: import uint8 sampled field",
         "[core/model/model][core/model][core][model]") {
  model::Model s;
  QFile f(":/test/models/very-simple-model-uint8.xml");
  f.open(QIODevice::ReadOnly);
  std::string xml{f.readAll().toStdString()};
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(xml.c_str()));
  // size of compartments are not set in original model
  REQUIRE(doc->getModel()->getCompartment("c1")->isSetSize() == false);
  REQUIRE(doc->getModel()->getCompartment("c2")->isSetSize() == false);
  REQUIRE(doc->getModel()->getCompartment("c3")->isSetSize() == false);
  s.importSBMLString(xml);
  xml = s.getXml().toStdString();
  doc.reset(libsbml::readSBMLFromString(xml.c_str()));
  // after import, compartment size is set based on geometry image
  REQUIRE(doc->getModel()->getCompartment("c1")->isSetSize() == true);
  REQUIRE(doc->getModel()->getCompartment("c1")->getSize() ==
          dbl_approx(5441.0));
  REQUIRE(doc->getModel()->getCompartment("c2")->isSetSize() == true);
  REQUIRE(doc->getModel()->getCompartment("c2")->getSize() ==
          dbl_approx(4034.0));
  REQUIRE(doc->getModel()->getCompartment("c3")->isSetSize() == true);
  REQUIRE(doc->getModel()->getCompartment("c3")->getSize() ==
          dbl_approx(525.0));

  const auto &img = s.getGeometry().getImage();
  REQUIRE(img.colorCount() == 3);
  REQUIRE(s.getCompartments().getColour("c1") ==
          utils::indexedColours()[0].rgb());
  REQUIRE(s.getCompartments().getColour("c2") ==
          utils::indexedColours()[1].rgb());
  REQUIRE(s.getCompartments().getColour("c3") ==
          utils::indexedColours()[2].rgb());
  // species A_c1 has initialAmount 11 -> converted to concentration
  REQUIRE(s.getSpecies().getInitialConcentration("A_c1") ==
          dbl_approx(11.0 / 5441.0));
  // species A_c2 has no initialAmount or initialConcentration -> defaulted to 0
  REQUIRE(s.getSpecies().getInitialConcentration("A_c2") == dbl_approx(0.0));
}

SCENARIO("SBML: ABtoC.xml", "[core/model/model][core/model][core][model]") {
  model::Model s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  GIVEN("SBML document") {
    WHEN("importSBMLFile called") {
      THEN("find compartments") {
        REQUIRE(s.getCompartments().getIds().size() == 1);
        REQUIRE(s.getCompartments().getIds()[0] == "comp");
      }
      THEN("find species") {
        REQUIRE(s.getSpecies().getIds("comp").size() == 3);
        REQUIRE(s.getSpecies().getIds("comp")[0] == "A");
        REQUIRE(s.getSpecies().getIds("comp")[1] == "B");
        REQUIRE(s.getSpecies().getIds("comp")[2] == "C");
      }
      THEN("find species geometry") {
        auto g = s.getSpeciesGeometry("A");
        REQUIRE(g.modelUnits.getAmount().name == s.getUnits().getAmount().name);
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
      s.getCompartments().add("newComp !");
      REQUIRE(s.getCompartments().getIds().size() == 2);
      REQUIRE(s.getCompartments().getIds()[0] == "comp");
      REQUIRE(s.getCompartments().getIds()[1] == "newComp_");
      REQUIRE(s.getCompartments().getNames().size() == 2);
      REQUIRE(s.getCompartments().getNames()[0] == "comp");
      REQUIRE(s.getCompartments().getNames()[1] == "newComp !");
      REQUIRE(s.getCompartments().getName("newComp_") == "newComp !");
      REQUIRE(s.getSpecies().getIds("newComp_").size() == 0);
      REQUIRE(s.getReactions().getIds("newComp_").size() == 0);
      // add compartment with same name: GUI appends underscore
      s.getCompartments().add("newComp !");
      REQUIRE(s.getCompartments().getIds().size() == 3);
      REQUIRE(s.getCompartments().getIds()[0] == "comp");
      REQUIRE(s.getCompartments().getIds()[1] == "newComp_");
      REQUIRE(s.getCompartments().getIds()[2] == "newComp__");
      REQUIRE(s.getCompartments().getNames().size() == 3);
      REQUIRE(s.getCompartments().getNames()[0] == "comp");
      REQUIRE(s.getCompartments().getNames()[1] == "newComp !");
      REQUIRE(s.getCompartments().getNames()[2] == "newComp !_");
      // remove compartment
      s.getCompartments().remove("newComp__");
      REQUIRE(s.getCompartments().getIds().size() == 2);
      REQUIRE(s.getCompartments().getNames().size() == 2);
      // add species to compartment
      s.getSpecies().add("q", "newComp_");
      REQUIRE(s.getSpecies().getIds("newComp_").size() == 1);
      REQUIRE(s.getSpecies().getIds("newComp_")[0] == "q");
      s.getSpecies().add(" !Wq", "newComp_");
      REQUIRE(s.getSpecies().getIds("newComp_").size() == 2);
      REQUIRE(s.getSpecies().getIds("newComp_")[0] == "q");
      REQUIRE(s.getSpecies().getIds("newComp_")[1] == "_Wq");
      REQUIRE(s.getSpecies().getName("_Wq") == " !Wq");
      // removing a compartment also removes the species in it, and any
      // reactions involving them
      REQUIRE(s.getReactions().getIds("comp").size() == 1);
      s.getCompartments().remove("comp");
      REQUIRE(s.getReactions().getIds("comp").isEmpty());
      REQUIRE(s.getSpecies().getIds("comp").isEmpty());
      // no compartments left, so no species either
      s.getCompartments().remove("newComp_");
      REQUIRE(s.getSpecies().getIds("newComp_").isEmpty());
    }
    WHEN("add / remove species") {
      REQUIRE(s.getSpecies().getIds("comp").size() == 3);
      s.getSpecies().add("1 stup!d N@ame?", "comp");
      REQUIRE(s.getSpecies().getIds("comp").size() == 4);
      REQUIRE(s.getSpecies().getIds("comp")[3] == "_1_stupd_Name");
      REQUIRE(s.getSpecies().getName("_1_stupd_Name") == "1 stup!d N@ame?");
      REQUIRE(s.getSpecies().getCompartment("_1_stupd_Name") == "comp");
      REQUIRE(s.getSpecies().getIsSpatial("_1_stupd_Name") == true);
      REQUIRE(s.getSpecies().getIsConstant("_1_stupd_Name") == false);
      REQUIRE(s.getSpecies().getDiffusionConstant("_1_stupd_Name") ==
              dbl_approx(1.0));
      REQUIRE(s.getSpecies().getInitialConcentration("_1_stupd_Name") ==
              dbl_approx(0.0));
      // add another species with the same name: GUI appends _compartmentName
      s.getSpecies().add("1 stup!d N@ame?", "comp");
      REQUIRE(s.getSpecies().getIds("comp").size() == 5);
      REQUIRE(s.getSpecies().getIds("comp")[4] == "_1_stupd_Name_comp");
      REQUIRE(s.getSpecies().getName("_1_stupd_Name_comp") ==
              "1 stup!d N@ame?_comp");
      REQUIRE(s.getSpecies().getCompartment("_1_stupd_Name_comp") == "comp");
      REQUIRE(s.getSpecies().getIsSpatial("_1_stupd_Name_comp") == true);
      REQUIRE(s.getSpecies().getIsConstant("_1_stupd_Name_comp") == false);
      REQUIRE(s.getSpecies().getDiffusionConstant("_1_stupd_Name_comp") ==
              dbl_approx(1.0));
      REQUIRE(s.getSpecies().getInitialConcentration("_1_stupd_Name_comp") ==
              dbl_approx(0.0));
      // remove species _1_stupd_Name
      s.getSpecies().remove("_1_stupd_Name_comp");
      REQUIRE(s.getSpecies().getIds("comp").size() == 4);
      REQUIRE(s.getSpecies().getIds("comp")[0] == "A");
      REQUIRE(s.getSpecies().getIds("comp")[1] == "B");
      REQUIRE(s.getSpecies().getIds("comp")[2] == "C");
      REQUIRE(s.getSpecies().getIds("comp")[3] == "_1_stupd_Name");
      // remove non-existent species is a no-op
      s.getSpecies().remove("QQ_1_stupd_NameQQ");
      REQUIRE(s.getSpecies().getIds("comp").size() == 4);
      s.getSpecies().remove("Idontexist");
      REQUIRE(s.getSpecies().getIds("comp").size() == 4);
      // remove species involved in a reaction: also removes reaction
      REQUIRE(s.getReactions().getIds("comp").size() == 1);
      s.getSpecies().remove("A");
      REQUIRE(s.getSpecies().getIds("comp").size() == 3);
      REQUIRE(s.getReactions().getIds("comp").size() == 0);
      s.getSpecies().remove("B");
      REQUIRE(s.getSpecies().getIds("comp").size() == 2);
      REQUIRE(s.getReactions().getIds("comp").size() == 0);
      s.getSpecies().remove("C");
      REQUIRE(s.getSpecies().getIds("comp").size() == 1);
      s.getSpecies().remove("_1_stupd_Name");
      REQUIRE(s.getSpecies().getIds("comp").size() == 0);
    }
    WHEN("image geometry imported, assigned to compartment") {
      QImage img(":/geometry/circle-100x100.png");
      QRgb col = QColor(144, 97, 193).rgba();
      REQUIRE(img.pixel(50, 50) == col);
      img.save("tmp.png");
      s.getGeometry().importGeometryFromImage(QImage("tmp.png"));
      s.getCompartments().setColour("comp", col);
      THEN("getCompartmentImage returns image") {
        REQUIRE(s.getGeometry().getImage().size() == QSize(100, 100));
        REQUIRE(s.getGeometry().getImage().pixel(50, 50) == col);
      }
      THEN("find membranes") {
        REQUIRE(s.getMembranes().getMembranes().empty());
      }
      THEN("find reactions") {
        REQUIRE(s.getReactions().getIds("comp").size() == 1);
        REQUIRE(s.getReactions().getIds("comp")[0] == "r1");
        REQUIRE(s.getReactions().getName("r1") == "r1");
        REQUIRE(s.getReactions().getSpeciesStoichiometry("r1", "C") ==
                dbl_approx(1));
        REQUIRE(s.getReactions().getSpeciesStoichiometry("r1", "A") ==
                dbl_approx(-1));
        REQUIRE(s.getReactions().getSpeciesStoichiometry("r1", "B") ==
                dbl_approx(-1));
        REQUIRE(s.getReactions().getParameterIds("r1").size() == 1);
        REQUIRE(s.getReactions().getParameterName("r1", "k1") == "k1");
        REQUIRE(s.getReactions().getParameterValue("r1", "k1") ==
                dbl_approx(0.1));
        REQUIRE(s.getReactions().getRateExpression("r1") == "A * B * k1");
        REQUIRE(s.getReactions().getScheme("r1") == "A + B -> C");
      }
      THEN("species have correct colours") {
        REQUIRE(s.getSpecies().getColour("A") == 0xffe60003);
        REQUIRE(s.getSpecies().getColour("B") == 0xff00b41b);
        REQUIRE(s.getSpecies().getColour("C") == 0xfffbff00);
      }
      WHEN("species colours changed") {
        auto newA = QColor(12, 12, 12).rgb();
        auto newB = QColor(123, 321, 1).rgb();
        auto newC = QColor(0, 22, 99).rgb();
        s.getSpecies().setColour("A", newA);
        s.getSpecies().setColour("B", newB);
        s.getSpecies().setColour("C", newC);
        REQUIRE(s.getSpecies().getColour("A") == newA);
        REQUIRE(s.getSpecies().getColour("B") == newB);
        REQUIRE(s.getSpecies().getColour("C") == newC);
        s.getSpecies().setColour("A", newC);
        REQUIRE(s.getSpecies().getColour("A") == newC);
      }
    }
  }
}

SCENARIO("SBML: very-simple-model.xml",
         "[core/model/model][core/model][core][model]") {
  QFile f(":/models/very-simple-model.xml");
  f.open(QIODevice::ReadOnly);
  model::Model s;
  s.importSBMLString(f.readAll().toStdString());
  GIVEN("SBML document") {
    THEN("find compartments") {
      REQUIRE(s.getCompartments().getIds().size() == 3);
      REQUIRE(s.getCompartments().getIds()[0] == "c1");
      REQUIRE(s.getCompartments().getIds()[1] == "c2");
      REQUIRE(s.getCompartments().getIds()[2] == "c3");
    }
    THEN("find species") {
      REQUIRE(s.getSpecies().getIds("c1").size() == 2);
      REQUIRE(s.getSpecies().getIds("c1")[0] == "A_c1");
      REQUIRE(s.getSpecies().getIds("c1")[1] == "B_c1");
      REQUIRE(s.getSpecies().getIds("c2").size() == 2);
      REQUIRE(s.getSpecies().getIds("c2")[0] == "A_c2");
      REQUIRE(s.getSpecies().getIds("c2")[1] == "B_c2");
      REQUIRE(s.getSpecies().getIds("c3").size() == 2);
      REQUIRE(s.getSpecies().getIds("c3")[0] == "A_c3");
      REQUIRE(s.getSpecies().getIds("c3")[1] == "B_c3");
      REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
      REQUIRE(s.getSpecies().getCompartment("A_c2") == "c2");
      REQUIRE(s.getSpecies().getCompartment("A_c3") == "c3");
      REQUIRE(s.getSpecies().getCompartment("B_c1") == "c1");
      REQUIRE(s.getSpecies().getCompartment("B_c2") == "c2");
      REQUIRE(s.getSpecies().getCompartment("B_c3") == "c3");
    }
    WHEN("species name changed") {
      REQUIRE(s.getSpecies().getName("A_c1") == "A_out");
      s.getSpecies().setName("A_c1", "long name with Spaces");
      REQUIRE(s.getSpecies().getName("A_c1") == "long name with Spaces");
      REQUIRE(s.getSpecies().getName("B_c2") == "B_cell");
      s.getSpecies().setName("B_c2",
                             "non-alphanumeric chars allowed: @#$%^&*(_");
      REQUIRE(s.getSpecies().getName("B_c2") ==
              "non-alphanumeric chars allowed: @#$%^&*(_");
    }
    WHEN("species compartment changed") {
      REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
      REQUIRE(s.getSpecies().getIds("c1") == QStringList{"A_c1", "B_c1"});
      REQUIRE(s.getSpecies().getIds("c2") == QStringList{"A_c2", "B_c2"});
      REQUIRE(s.getSpecies().getIds("c3") == QStringList{"A_c3", "B_c3"});
      s.getSpecies().setCompartment("A_c1", "c2");
      REQUIRE(s.getSpecies().getCompartment("A_c1") == "c2");
      REQUIRE(s.getSpecies().getIds("c1") == QStringList{"B_c1"});
      REQUIRE(s.getSpecies().getIds("c2") ==
              QStringList{"A_c1", "A_c2", "B_c2"});
      REQUIRE(s.getSpecies().getIds("c3") == QStringList{"A_c3", "B_c3"});
      s.getSpecies().setCompartment("A_c1", "c1");
      REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
      REQUIRE(s.getSpecies().getIds("c1") == QStringList{"A_c1", "B_c1"});
      REQUIRE(s.getSpecies().getIds("c2") == QStringList{"A_c2", "B_c2"});
      REQUIRE(s.getSpecies().getIds("c3") == QStringList{"A_c3", "B_c3"});
    }
    WHEN("invalid get species compartment call returns empty string") {
      REQUIRE(s.getSpecies().getCompartment("non_existent_species").isEmpty());
    }
    WHEN("invalid species compartment change call is a no-op") {
      REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
      s.getSpecies().setCompartment("A_c1", "invalid_compartment");
      REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
      s.getSpecies().setCompartment("invalid_species", "invalid_compartment");
      REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
    }
    WHEN("add/remove empty reaction") {
      REQUIRE(s.getReactions().getIds("c2").size() == 0);
      s.getReactions().add("re ac~!1", "c2");
      REQUIRE(s.getReactions().getIds("c2").size() == 1);
      REQUIRE(s.getReactions().getIds("c2")[0].toStdString() == "re_ac1");
      REQUIRE(s.getReactions().getName("re_ac1") == "re ac~!1");
      REQUIRE(s.getReactions().getLocation("re_ac1") == "c2");
      REQUIRE(s.getReactions().getRateExpression("re_ac1") == "1");
      s.getReactions().remove("re_ac1");
      REQUIRE(s.getReactions().getIds("c2").size() == 0);
    }
    WHEN("set reaction") {
      REQUIRE(s.getReactions().getIds("c2").size() == 0);
      REQUIRE(s.getReactions().getIds("c3").size() == 1);
      s.getReactions().add("re ac~!1", "c2");
      REQUIRE(s.getReactions().getLocation("re_ac1") == "c2");
      s.getReactions().setName("re_ac1", "new Name");
      s.getReactions().setLocation("re_ac1", "c3");
      s.getReactions().setSpeciesStoichiometry("re_ac1", "A_c3", 1);
      s.getReactions().setSpeciesStoichiometry("re_ac1", "B_c3", -2.0123);
      s.getReactions().addParameter("re_ac1", "const 1", 0.2);
      s.getReactions().setRateExpression("re_ac1",
                                         "0.2 + A_c3 * B_c3 * const_1");
      REQUIRE(s.getReactions().getName("re_ac1") == "new Name");
      REQUIRE(s.getReactions().getLocation("re_ac1") == "c3");
      REQUIRE(s.getReactions().getRateExpression("re_ac1") ==
              "0.2 + A_c3 * B_c3 * const_1");
      REQUIRE(s.getReactions().getSpeciesStoichiometry("re_ac1", "A_c3") ==
              dbl_approx(1));
      REQUIRE(s.getReactions().getSpeciesStoichiometry("re_ac1", "B_c3") ==
              dbl_approx(-2.0123));
      REQUIRE(s.getReactions().getScheme("re_ac1") ==
              "2.0123 B_nucl -> A_nucl");
    }
    WHEN("change reaction location") {
      REQUIRE(s.getReactions().getIds("c2").size() == 0);
      REQUIRE(s.getReactions().getIds("c3").size() == 1);
      s.getReactions().setLocation("A_B_conversion", "c2");
      REQUIRE(s.getReactions().getIds("c2").size() == 1);
      REQUIRE(s.getReactions().getIds("c3").size() == 0);
      REQUIRE(s.getReactions().getName("A_B_conversion") ==
              "A to B conversion");
      REQUIRE(s.getReactions().getLocation("A_B_conversion") == "c2");
      s.getReactions().setLocation("A_B_conversion", "c1");
      REQUIRE(s.getReactions().getIds("c1").size() == 1);
      REQUIRE(s.getReactions().getIds("c2").size() == 0);
      REQUIRE(s.getReactions().getName("A_B_conversion") ==
              "A to B conversion");
      REQUIRE(s.getReactions().getLocation("A_B_conversion") == "c1");
      s.getReactions().remove("A_B_conversion");
      REQUIRE(s.getReactions().getIds("c1").size() == 0);
    }
  }
}

SCENARIO("SBML: load model, refine mesh, save",
         "[core/model/model][core/model][core][model][mesh]") {
  model::Model s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  REQUIRE(s.getGeometry().getMesh()->getNumBoundaries() == 1);
  REQUIRE(s.getGeometry().getMesh()->getBoundaryMaxPoints(0) == 16);
  auto oldNumTriangleIndices =
      s.getGeometry().getMesh()->getTriangleIndicesAsFlatArray(0).size();
  // refine boundary and mesh
  s.getGeometry().getMesh()->setCompartmentMaxTriangleArea(0, 32);
  REQUIRE(s.getGeometry().getMesh()->getNumBoundaries() == 1);
  REQUIRE(s.getGeometry().getMesh()->getTriangleIndicesAsFlatArray(0).size() >
          oldNumTriangleIndices);
  auto maxArea = s.getGeometry().getMesh()->getCompartmentMaxTriangleArea(0);
  auto numTriangleIndices =
      s.getGeometry().getMesh()->getTriangleIndicesAsFlatArray(0).size();
  // save SBML doc
  s.exportSBMLFile("tmp.xml");

  // import again
  model::Model s2;
  s2.importSBMLFile("tmp.xml");
  REQUIRE(s.getGeometry().getMesh()->getNumBoundaries() == 1);
  REQUIRE(s2.getGeometry().getMesh()->getCompartmentMaxTriangleArea(0) ==
          maxArea);
  REQUIRE(s2.getGeometry().getMesh()->getTriangleIndicesAsFlatArray(0).size() ==
          numTriangleIndices);
}

SCENARIO("SBML: load model, change size of geometry, save",
         "[core/model/model][core/model][core][model][mesh]") {
  model::Model s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  std::vector<double> v = s.getGeometry().getMesh()->getVerticesAsFlatArray();
  REQUIRE(s.getGeometry().getMesh()->getBoundaryMaxPoints(0) == 16);
  REQUIRE(s.getGeometry().getMesh()->getCompartmentMaxTriangleArea(0) == 40);
  double v0 = v[0];
  double v1 = v[1];
  double v2 = v[2];
  double v3 = v[3];
  // change size of geometry, but not of compartments
  double a = 0.01;
  s.getGeometry().setPixelWidth(a);
  v = s.getGeometry().getMesh()->getVerticesAsFlatArray();
  REQUIRE(v[0] == dbl_approx(a * v0));
  REQUIRE(v[1] == dbl_approx(a * v1));
  REQUIRE(v[2] == dbl_approx(a * v2));
  REQUIRE(v[3] == dbl_approx(a * v3));
}

SCENARIO("SBML: load .xml model, simulate, save as .sme, load .sme",
         "[core/model/model][core/model][core][model]") {
  model::Model s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  simulate::Simulation sim(s);
  sim.doTimesteps(0.1, 2);
  s.exportSMEFile("test.sme");
  model::Model s2;
  s2.importFile("test.sme");
  REQUIRE(s2.getCompartments().getIds() == s.getCompartments().getIds());
  REQUIRE(s.getXml() == s2.getXml());
  REQUIRE(s.getSimulationData().timePoints.size() == 3);
  REQUIRE(s.getSimulationData().timePoints[2] == dbl_approx(0.2));
  REQUIRE(s.getSimulationData().concPadding.size() == 3);
  REQUIRE(s.getSimulationData().concPadding[2] == dbl_approx(0));
}

SCENARIO("SBML: import multi-compartment SBML doc without spatial geometry",
         "[core/model/model][core/model][core][model][Q]") {
  model::Model s;
  // compartments:
  // - cyt: {B, C, D}
  // - nuc: {A}
  // - org: {}
  // - ext: {Dext}
  // reactions:
  // - trans: A -> B
  // - conv: B -> C
  // - degrad: C -> D
  // - ex: D -> Dext
  QFile f(":test/models/non-spatial-multi-compartment.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  auto &geometry{s.getGeometry()};
  auto &compartments{s.getCompartments()};
  auto &membranes{s.getMembranes()};
  // reactions in original xml model have no compartment
  auto &reactions{s.getReactions()};
  REQUIRE(geometry.getIsValid() == false);
  REQUIRE(geometry.getHasImage() == false);
  // these ones are located by sme based on species all being in the same
  // compartment, and are divided by the compartment volume
  REQUIRE(reactions.getLocation("conv") == "cyt");
  // note this one didn't have a cyt factor originally: although it only affects
  // species in the cyt compartment, it was actually thought of as a membrane
  // reaction in the original model (but no way to tell this from the ODE xml
  // model)
  REQUIRE(symEq(reactions.getRateExpression("conv"), "B * k1 / cyt"));
  REQUIRE(reactions.getLocation("degrad") == "cyt");
  REQUIRE(symEq(reactions.getRateExpression("degrad"), "C * k1 - D * k2"));
  // these are membrane reactions: until we have geometry they have no defined
  // location here:
  REQUIRE(reactions.getLocation("trans") == "");
  REQUIRE(symEq(reactions.getRateExpression("trans"),
                "Henri_Michaelis_Menten__irreversible(A, Km, V)"));
  REQUIRE(reactions.getLocation("ex") == "");
  REQUIRE(symEq(reactions.getRateExpression("ex"), "k1 * D"));
  geometry.importGeometryFromImage(QImage(":test/geometry/cell.png"));
  auto colours{geometry.getImage().colorTable()};
  REQUIRE(colours.size() == 4);
  // assign each compartment to a colour region in the image
  compartments.setColour("cyt", colours[1]);
  compartments.setColour("nuc", colours[2]);
  compartments.setColour("org", colours[3]);
  compartments.setColour("ext", colours[0]);
  REQUIRE(geometry.getIsValid() == true);
  REQUIRE(geometry.getHasImage() == true);
  // all reactions are now assigned to a valid location
  REQUIRE(reactions.getLocation("conv") == "cyt");
  REQUIRE(reactions.getLocation("degrad") == "cyt");
  REQUIRE(reactions.getLocation("trans") == "cyt_nuc_membrane");
  REQUIRE(reactions.getLocation("ex") == "ext_cyt_membrane");
  // membrane reaction rates are divided by membrane area
  REQUIRE(symEq(
      reactions.getRateExpression("trans"),
      "Henri_Michaelis_Menten__irreversible(A, Km, V) / cyt_nuc_membrane"));
  REQUIRE(
      symEq(reactions.getRateExpression("ex"), "k1 * D / ext_cyt_membrane"));
}
