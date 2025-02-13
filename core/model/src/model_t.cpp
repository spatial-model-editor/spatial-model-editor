#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model_test_utils.hpp"
#include "sme/mesh2d.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;
using namespace sme::test;

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
  // create two compartments of different volume
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
  kin->setFormula("5*spec0c0*compartment0");
  reac->setKineticLaw(kin);
  // write SBML document to file
  libsbml::SBMLWriter().writeSBML(document.get(), filename);
}

TEST_CASE("SBML: import SBML doc without geometry",
          "[core/model/model][core/model][core][model]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmplvl2model.xml");
  // import SBML model
  model::Model s;
  s.importSBMLFile("tmplvl2model.xml");
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getErrorMessage().isEmpty());
  // export it again
  s.exportSBMLFile("tmplvl2model.xml");
  SECTION("upgrade SBML doc and add default 3d spatial geometry") {
    // load new model
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromFile("tmplvl2model.xml"));
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    REQUIRE(model->getLevel() == 3);
    REQUIRE(model->getVersion() == 2);
    for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
      REQUIRE(model->getCompartment(i)->getSpatialDimensions() == 3);
    }
    REQUIRE(doc->isPackageEnabled("spatial") == true);
    auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
        model->getPlugin("spatial"));
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->isSetGeometry() == true);
    auto *geom = plugin->getGeometry();
    REQUIRE(geom != nullptr);
    REQUIRE(geom->getNumCoordinateComponents() == 3);
    REQUIRE(geom->getCoordinateComponent(0)->getType() ==
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
    REQUIRE(geom->getCoordinateComponent(1)->getType() ==
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
    REQUIRE(geom->getCoordinateComponent(2)->getType() ==
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z);

    REQUIRE(geom->getNumGeometryDefinitions() == 0);
  }
  SECTION("import geometry & assign compartments") {
    // import geometry image & assign compartments to colors
    s.getGeometry().importGeometryFromImages(
        common::ImageStack{{QImage(":/geometry/single-pixels-3x1.png")}},
        false);
    s.getCompartments().setColor("compartment0", 0xffaaaaaa);
    s.getCompartments().setColor("compartment1", 0xff525252);
    // export it again
    s.exportSBMLFile("tmplvl2model.xml");
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromFile("tmplvl2model.xml"));
    auto *model = doc->getModel();
    auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
        model->getPlugin("spatial"));
    auto *geom = plugin->getGeometry();
    auto *sfgeom = dynamic_cast<libsbml::SampledFieldGeometry *>(
        geom->getGeometryDefinition(0));
    auto *sf = geom->getSampledField(sfgeom->getSampledField());
    auto sfvals = common::stringToVector<QRgb>(sf->getSamples());
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
    REQUIRE(static_cast<unsigned>(sfvol0->getSampledValue()) == 1);
    REQUIRE(static_cast<unsigned>(sfvol0->getSampledValue()) ==
            static_cast<unsigned>(sfvals[1]));

    auto *scp1 = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        model->getCompartment("compartment1")->getPlugin("spatial"));
    REQUIRE(scp1->isSetCompartmentMapping() == true);
    auto *sfvol1 = sfgeom->getSampledVolumeByDomainType(
        scp1->getCompartmentMapping()->getDomainType());
    CAPTURE(sfvol1->getSampledValue());
    REQUIRE(static_cast<unsigned>(sfvol1->getSampledValue()) == 2);
    REQUIRE(static_cast<unsigned>(sfvol1->getSampledValue()) ==
            static_cast<unsigned>(sfvals[2]));
    SECTION("import concentration & set diff constants") {
      // import concentration
      s.getSpecies().setSampledFieldConcentration("spec0c0", {0.0, 0.0, 0.0});
      REQUIRE(
          s.getSpecies().getConcentrationImages("spec0c0").volume().depth() ==
          1);
      REQUIRE(s.getSpecies().getConcentrationImages("spec0c0")[0].size() ==
              QSize(3, 1));
      REQUIRE(s.getSpecies().getConcentrationImages("spec0c0")[0].pixel(1, 0) ==
              qRgb(0, 0, 0));
      REQUIRE(s.getSpecies().getConcentrationImages("spec0c0")[0].pixel(0, 0) ==
              0);
      REQUIRE(s.getSpecies().getConcentrationImages("spec0c0")[0].pixel(2, 0) ==
              0);
      // set spec1c1conc to zero -> black pixel
      s.getSpecies().setSampledFieldConcentration("spec1c1", {0.0, 0.0, 0.0});
      REQUIRE(s.getSpecies().getConcentrationImages("spec1c1")[0].pixel(0, 0) ==
              0);
      REQUIRE(s.getSpecies().getConcentrationImages("spec1c1")[0].pixel(1, 0) ==
              0);
      REQUIRE(s.getSpecies().getConcentrationImages("spec1c1")[0].pixel(2, 0) ==
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
      s.exportSBMLFile("tmplvl2model2.xml");
      // import model again, recover concentration & compartment assignments
      model::Model s2;
      s2.importSBMLFile("tmplvl2model2.xml");
      REQUIRE(s2.getCompartments().getColor("compartment0") == 0xffaaaaaa);
      REQUIRE(s2.getCompartments().getColor("compartment1") == 0xff525252);
      REQUIRE(s2.getSpecies().getConcentrationImages("spec0c0")[0].pixel(
                  1, 0) == qRgb(0, 0, 0));
      REQUIRE(s2.getSpecies().getConcentrationImages("spec0c0")[0].pixel(
                  0, 0) == 0);
      REQUIRE(s2.getSpecies().getConcentrationImages("spec0c0")[0].pixel(
                  2, 0) == 0);
      REQUIRE(s2.getSpecies().getConcentrationImages("spec1c1")[0].pixel(
                  0, 0) == 0);
      REQUIRE(s2.getSpecies().getConcentrationImages("spec1c1")[0].pixel(
                  1, 0) == 0);
      REQUIRE(s2.getSpecies().getConcentrationImages("spec1c1")[0].pixel(
                  2, 0) == qRgb(0, 0, 0));
      REQUIRE(s2.getSpecies().getConcentrationImages("spec2c1")[0].pixel(
                  0, 0) == 0);
      REQUIRE(s2.getSpecies().getConcentrationImages("spec2c1")[0].pixel(
                  1, 0) == 0);
      REQUIRE(s2.getSpecies().getConcentrationImages("spec2c1")[0].pixel(
                  2, 0) == qRgb(0, 0, 0));

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

TEST_CASE("SBML: name clashes", "[core/model/model][core/model][core][model]") {
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
  common::unique_C_ptr<char> xmlChar{
      libsbml::writeSBMLToString(document.get())};
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

TEST_CASE("SBML: import SBML level 2 document",
          "[core/model/model][core/model][core][model]") {
  // create simple SBML level 2.4 model
  createSBMLlvl2doc("tmpmodelimport.xml");
  // import SBML model
  model::Model s;
  s.importSBMLFile("tmpmodelimport.xml");
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getErrorMessage().isEmpty());

  // this is a non-spatial model:
  REQUIRE(s.getReactions().getIsIncompleteODEImport());
  REQUIRE(s.getGeometry().getHasImage() == false);
  REQUIRE(s.getGeometry().getIsValid() == false);

  // compartments
  REQUIRE(s.getCompartments().getIds().size() == 2);
  REQUIRE(s.getCompartments().getIds()[0] == "compartment0");
  REQUIRE(s.getCompartments().getIds()[1] == "compartment1");

  // species
  REQUIRE(s.getSpecies().getIds("compartment0").size() == 2);
  REQUIRE(s.getSpecies().getIds("compartment0")[0] == "spec0c0");
  REQUIRE(s.getSpecies().getIds("compartment0")[1] == "spec1c0");
  REQUIRE(s.getSpecies().getIds("compartment1").size() == 3);
  REQUIRE(s.getSpecies().getIds("compartment1")[0] == "spec0c1");
  REQUIRE(s.getSpecies().getIds("compartment1")[1] == "spec1c1");
  REQUIRE(s.getSpecies().getIds("compartment1")[2] == "spec2c1");

  // reactions don't have a location in original model,
  // and we wait until the geometry is assigned to make our best guess,
  // so at this point looking up reactions by compartment gives nothing:
  REQUIRE(s.getReactions().getIds("compartment0").size() == 0);
  REQUIRE(s.getReactions().getLocation("reac1") == "");
  // the rest of the reaction information is there though
  REQUIRE(s.getReactions().getName("reac1") == "reac1");
  REQUIRE(s.getReactions().getSpeciesStoichiometry("reac1", "spec1c0") ==
          dbl_approx(1));
  REQUIRE(s.getReactions().getSpeciesStoichiometry("reac1", "spec0c0") ==
          dbl_approx(-1));
  REQUIRE(s.getReactions().getRateExpression("reac1") ==
          "5 * spec0c0 * compartment0");
  REQUIRE(s.getReactions().getScheme("reac1") == "spec0c0 -> spec1c0");

  // import geometry image
  s.getGeometry().importGeometryFromImages(
      common::ImageStack{{QImage(":/geometry/single-pixels-3x1.png")}}, false);
  REQUIRE(s.getReactions().getIsIncompleteODEImport());
  REQUIRE(s.getGeometry().getHasImage() == true);
  REQUIRE(s.getGeometry().getIsValid() == false);

  // assign compartments to colors
  s.getCompartments().setColor("compartment0", 0xffaaaaaa);
  s.getCompartments().setColor("compartment1", 0xff525252);
  REQUIRE(s.getReactions().getIsIncompleteODEImport());
  REQUIRE(s.getGeometry().getHasImage() == true);
  REQUIRE(s.getGeometry().getIsValid() == true);

  // reaction locations are now assigned
  REQUIRE(s.getReactions().getIds("compartment0").size() == 1);
  REQUIRE(s.getReactions().getLocation("reac1") == "compartment0");

  // finalize import: rescale reactions
  auto reactionRescalings{s.getReactions().getSpatialReactionRescalings()};
  s.getReactions().applySpatialReactionRescalings(reactionRescalings);
  REQUIRE(s.getReactions().getIsIncompleteODEImport() == false);
  REQUIRE(s.getGeometry().getHasImage() == true);
  REQUIRE(s.getGeometry().getIsValid() == true);
  REQUIRE(s.getReactions().getRateExpression("reac1") == "5 * spec0c0");

  // doc is now sbml level(3,2) with spatial plugin required & enabled
  auto doc{toSbmlDoc(s)};
  REQUIRE(doc->getLevel() == 3);
  REQUIRE(doc->getVersion() == 2);
  REQUIRE(doc->getPackageRequired("spatial") == true);
  REQUIRE(dynamic_cast<libsbml::SpatialModelPlugin *>(
              doc->getModel()->getPlugin("spatial")) != nullptr);

  SECTION("Compartment Colors") {
    QRgb col1 = 0xffaaaaaa;
    QRgb col2 = 0xff525252;
    QRgb col3 = 0xffffffff;
    // can get CompartmentID from color
    REQUIRE(s.getCompartments().getIdFromColor(col1) == "compartment0");
    REQUIRE(s.getCompartments().getIdFromColor(col2) == "compartment1");
    REQUIRE(s.getCompartments().getIdFromColor(col3) == "");
    // can get color from CompartmentID"
    REQUIRE(s.getCompartments().getColor("compartment0") == col1);
    REQUIRE(s.getCompartments().getColor("compartment1") == col2);
    SECTION("new color assigned") {
      s.getCompartments().setColor("compartment0", col1);
      s.getCompartments().setColor("compartment1", col2);
      s.getCompartments().setColor("compartment0", col3);
      REQUIRE(s.getCompartments().getIdFromColor(col1) == "");
      REQUIRE(s.getCompartments().getIdFromColor(col2) == "compartment1");
      REQUIRE(s.getCompartments().getIdFromColor(col3) == "compartment0");
      REQUIRE(s.getCompartments().getColor("compartment0") == col3);
      REQUIRE(s.getCompartments().getColor("compartment1") == col2);
    }
    SECTION("existing color re-assigned") {
      s.getCompartments().setColor("compartment0", col1);
      s.getCompartments().setColor("compartment1", col2);
      s.getCompartments().setColor("compartment0", col2);
      REQUIRE(s.getCompartments().getIdFromColor(col1) == "");
      REQUIRE(s.getCompartments().getIdFromColor(col2) == "compartment0");
      REQUIRE(s.getCompartments().getIdFromColor(col3) == "");
      REQUIRE(s.getCompartments().getColor("compartment0") == col2);
      REQUIRE(s.getCompartments().getColor("compartment1") == 0);
    }
  }
}

TEST_CASE("SBML: create new model, import geometry from image",
          "[core/model/model][core/model][core][model]") {
  model::Model s;
  REQUIRE(s.getGeometry().getHasImage() == false);
  REQUIRE(s.getGeometry().getIsValid() == false);
  REQUIRE(s.getIsValid() == false);
  REQUIRE(s.getErrorMessage().isEmpty());
  s.createSBMLFile("new");
  s.getCompartments().add("comp");
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getErrorMessage().isEmpty());
  REQUIRE(s.getGeometry().getHasImage() == false);
  REQUIRE(s.getGeometry().getIsValid() == false);
  SECTION("Single pixel image") {
    common::ImageStack imgs{{QImage(1, 1, QImage::Format_RGB32)}};
    QRgb col = QColor(12, 243, 154).rgba();
    imgs[0].setPixel(0, 0, col);
    s.getGeometry().importGeometryFromImages(imgs, false);
    REQUIRE(s.getIsValid() == true);
    REQUIRE(s.getErrorMessage().isEmpty());
    REQUIRE(s.getGeometry().getHasImage() == true);
    REQUIRE(s.getGeometry().getIsValid() == false);
    s.getCompartments().setColor("comp", col);
    REQUIRE(s.getIsValid() == true);
    REQUIRE(s.getErrorMessage().isEmpty());
    REQUIRE(s.getGeometry().getHasImage() == true);
    REQUIRE(s.getGeometry().getIsValid() == true);
  }
}

TEST_CASE("SBML: import uint8 sampled field",
          "[core/model/model][core/model][core][model]") {
  auto doc{getTestSbmlDoc("very-simple-model-uint8")};
  // original model: 2d 100m x 100m image
  // volume of compartments are not set in original spatial model
  REQUIRE(doc->getModel()->getCompartment("c1")->isSetSize() == false);
  REQUIRE(doc->getModel()->getCompartment("c2")->isSetSize() == false);
  REQUIRE(doc->getModel()->getCompartment("c3")->isSetSize() == false);
  // compartments are 2d in original spatial model
  REQUIRE(doc->getModel()->getCompartment("c1")->getSpatialDimensions() == 2);
  REQUIRE(doc->getModel()->getCompartment("c2")->getSpatialDimensions() == 2);
  REQUIRE(doc->getModel()->getCompartment("c3")->getSpatialDimensions() == 2);
  // compartments have explicit units
  REQUIRE(doc->getModel()->getCompartment("c1")->isSetUnits() == true);
  REQUIRE(doc->getModel()->getCompartment("c2")->isSetUnits() == true);
  REQUIRE(doc->getModel()->getCompartment("c3")->isSetUnits() == true);
  auto s{getTestModel("very-simple-model-uint8")};
  doc = toSbmlDoc(s);
  // after import, model is now 3d
  // z direction size set to 1 by default, so 100 m x 100 m x 1 m geometry image
  // volume of pixel is 1m^3 = 1e3 litres
  // after import, compartment volume is set based on geometry image
  REQUIRE(doc->getModel()->getCompartment("c1")->isSetSize() == true);
  REQUIRE(doc->getModel()->getCompartment("c1")->getSize() ==
          dbl_approx(5441000.0));
  REQUIRE(doc->getModel()->getCompartment("c2")->isSetSize() == true);
  REQUIRE(doc->getModel()->getCompartment("c2")->getSize() ==
          dbl_approx(4034000.0));
  REQUIRE(doc->getModel()->getCompartment("c3")->isSetSize() == true);
  REQUIRE(doc->getModel()->getCompartment("c3")->getSize() ==
          dbl_approx(525000.0));
  // compartments are now 3d
  REQUIRE(doc->getModel()->getCompartment("c1")->getSpatialDimensions() == 3);
  REQUIRE(doc->getModel()->getCompartment("c2")->getSpatialDimensions() == 3);
  REQUIRE(doc->getModel()->getCompartment("c3")->getSpatialDimensions() == 3);
  // no explicit units: inferred from model units & number of dimensions
  REQUIRE(doc->getModel()->getCompartment("c1")->isSetUnits() == false);
  REQUIRE(doc->getModel()->getCompartment("c2")->isSetUnits() == false);
  REQUIRE(doc->getModel()->getCompartment("c3")->isSetUnits() == false);

  const auto &img{s.getGeometry().getImages()};
  REQUIRE(img[0].colorCount() == 3);
  REQUIRE(s.getCompartments().getColor("c1") ==
          common::indexedColors()[0].rgb());
  REQUIRE(s.getCompartments().getColor("c2") ==
          common::indexedColors()[1].rgb());
  REQUIRE(s.getCompartments().getColor("c3") ==
          common::indexedColors()[2].rgb());
  // species A_c1 has initialAmount 11 -> converted to concentration
  REQUIRE(s.getSpecies().getInitialConcentration("A_c1") ==
          dbl_approx(11.0 / 5441000.0));
  // species A_c2 has no initialAmount or initialConcentration -> defaulted to 0
  REQUIRE(s.getSpecies().getInitialConcentration("A_c2") == dbl_approx(0.0));
}

TEST_CASE("SBML: ABtoC.xml", "[core/model/model][core/model][core][model]") {
  auto s{getExampleModel(Mod::ABtoC)};
  SECTION("SBML document") {
    SECTION("imported") {
      REQUIRE(s.getCompartments().getIds().size() == 1);
      REQUIRE(s.getCompartments().getIds()[0] == "comp");
      REQUIRE(s.getSpecies().getIds("comp").size() == 3);
      REQUIRE(s.getSpecies().getIds("comp")[0] == "A");
      REQUIRE(s.getSpecies().getIds("comp")[1] == "B");
      REQUIRE(s.getSpecies().getIds("comp")[2] == "C");
      auto g = s.getSpeciesGeometry("A");
      REQUIRE(g.modelUnits.getAmount().name == s.getUnits().getAmount().name);
      REQUIRE(g.voxelSize.width() == dbl_approx(1.0));
      REQUIRE(g.voxelSize.height() == dbl_approx(1.0));
      REQUIRE(g.voxelSize.depth() == dbl_approx(1.0));
      REQUIRE(g.physicalOrigin.p.x() == dbl_approx(0.0));
      REQUIRE(g.physicalOrigin.p.y() == dbl_approx(0.0));
      REQUIRE(g.physicalOrigin.z == dbl_approx(0.0));
      REQUIRE(g.compartmentVoxels.size() == 3149);
      REQUIRE(g.compartmentImageSize == sme::common::Volume{100, 100, 1});
    }
    SECTION("change model name") {
      REQUIRE(s.getName() == "");
      QString newName = "new model name";
      s.setName(newName);
      REQUIRE(s.getName() == newName);
    }
    SECTION("add / remove compartment") {
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
    SECTION("add / remove species") {
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
    SECTION("image geometry imported, assigned to compartment") {
      common::ImageStack imgs{{QImage(":/geometry/circle-100x100.png")}};
      QRgb col = QColor(144, 97, 193).rgba();
      REQUIRE(imgs[0].pixel(50, 50) == col);
      s.getGeometry().importGeometryFromImages(imgs, false);
      s.getCompartments().setColor("comp", col);
      REQUIRE(s.getGeometry().getImages().volume().depth() == 1);
      REQUIRE(s.getGeometry().getImages()[0].size() == QSize(100, 100));
      REQUIRE(s.getGeometry().getImages()[0].pixel(50, 50) == col);
      REQUIRE(s.getMembranes().getMembranes().empty());
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
      REQUIRE(s.getSpecies().getColor("A") == 0xffe60003);
      REQUIRE(s.getSpecies().getColor("B") == 0xff00b41b);
      REQUIRE(s.getSpecies().getColor("C") == 0xfffbff00);
      // change species colors
      auto newA = QColor(12, 12, 12).rgb();
      auto newB = QColor(123, 321, 1).rgb();
      auto newC = QColor(0, 22, 99).rgb();
      s.getSpecies().setColor("A", newA);
      s.getSpecies().setColor("B", newB);
      s.getSpecies().setColor("C", newC);
      REQUIRE(s.getSpecies().getColor("A") == newA);
      REQUIRE(s.getSpecies().getColor("B") == newB);
      REQUIRE(s.getSpecies().getColor("C") == newC);
      s.getSpecies().setColor("A", newC);
      REQUIRE(s.getSpecies().getColor("A") == newC);
    }
  }
}

TEST_CASE("SBML: very-simple-model.xml",
          "[core/model/model][core/model][core][model]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  SECTION("SBML document") {
    REQUIRE(s.getCompartments().getIds().size() == 3);
    REQUIRE(s.getCompartments().getIds()[0] == "c1");
    REQUIRE(s.getCompartments().getIds()[1] == "c2");
    REQUIRE(s.getCompartments().getIds()[2] == "c3");
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
  SECTION("species name changed") {
    REQUIRE(s.getSpecies().getName("A_c1") == "A_out");
    s.getSpecies().setName("A_c1", "long name with Spaces");
    REQUIRE(s.getSpecies().getName("A_c1") == "long name with Spaces");
    REQUIRE(s.getSpecies().getName("B_c2") == "B_cell");
    s.getSpecies().setName("B_c2", "non-alphanumeric chars allowed: @#$%^&*(_");
    REQUIRE(s.getSpecies().getName("B_c2") ==
            "non-alphanumeric chars allowed: @#$%^&*(_");
  }
  SECTION("species compartment changed") {
    REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
    REQUIRE(s.getSpecies().getIds("c1") == QStringList{"A_c1", "B_c1"});
    REQUIRE(s.getSpecies().getIds("c2") == QStringList{"A_c2", "B_c2"});
    REQUIRE(s.getSpecies().getIds("c3") == QStringList{"A_c3", "B_c3"});
    s.getSpecies().setCompartment("A_c1", "c2");
    REQUIRE(s.getSpecies().getCompartment("A_c1") == "c2");
    REQUIRE(s.getSpecies().getIds("c1") == QStringList{"B_c1"});
    REQUIRE(s.getSpecies().getIds("c2") == QStringList{"A_c1", "A_c2", "B_c2"});
    REQUIRE(s.getSpecies().getIds("c3") == QStringList{"A_c3", "B_c3"});
    s.getSpecies().setCompartment("A_c1", "c1");
    REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
    REQUIRE(s.getSpecies().getIds("c1") == QStringList{"A_c1", "B_c1"});
    REQUIRE(s.getSpecies().getIds("c2") == QStringList{"A_c2", "B_c2"});
    REQUIRE(s.getSpecies().getIds("c3") == QStringList{"A_c3", "B_c3"});
  }
  SECTION("invalid calls are no-ops") {
    REQUIRE(s.getSpecies().getCompartment("non_existent_species").isEmpty());
    REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
    s.getSpecies().setCompartment("A_c1", "invalid_compartment");
    REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
    s.getSpecies().setCompartment("invalid_species", "invalid_compartment");
    REQUIRE(s.getSpecies().getCompartment("A_c1") == "c1");
  }
  SECTION("add/remove empty reaction") {
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
  SECTION("set reaction") {
    REQUIRE(s.getReactions().getIds("c2").size() == 0);
    REQUIRE(s.getReactions().getIds("c3").size() == 1);
    s.getReactions().add("re ac~!1", "c2");
    REQUIRE(s.getReactions().getLocation("re_ac1") == "c2");
    s.getReactions().setName("re_ac1", "new Name");
    s.getReactions().setLocation("re_ac1", "c3");
    s.getReactions().setSpeciesStoichiometry("re_ac1", "A_c3", 1);
    s.getReactions().setSpeciesStoichiometry("re_ac1", "B_c3", -2.0123);
    s.getReactions().addParameter("re_ac1", "const 1", 0.2);
    s.getReactions().setRateExpression("re_ac1", "0.2 + A_c3 * B_c3 * const_1");
    REQUIRE(s.getReactions().getName("re_ac1") == "new Name");
    REQUIRE(s.getReactions().getLocation("re_ac1") == "c3");
    REQUIRE(s.getReactions().getRateExpression("re_ac1") ==
            "0.2 + A_c3 * B_c3 * const_1");
    REQUIRE(s.getReactions().getSpeciesStoichiometry("re_ac1", "A_c3") ==
            dbl_approx(1));
    REQUIRE(s.getReactions().getSpeciesStoichiometry("re_ac1", "B_c3") ==
            dbl_approx(-2.0123));
    REQUIRE(s.getReactions().getScheme("re_ac1") == "2.0123 B_nucl -> A_nucl");
  }
  SECTION("change reaction location") {
    REQUIRE(s.getReactions().getIds("c2").size() == 0);
    REQUIRE(s.getReactions().getIds("c3").size() == 1);
    s.getReactions().setLocation("A_B_conversion", "c2");
    REQUIRE(s.getReactions().getIds("c2").size() == 1);
    REQUIRE(s.getReactions().getIds("c3").size() == 0);
    REQUIRE(s.getReactions().getName("A_B_conversion") == "A to B conversion");
    REQUIRE(s.getReactions().getLocation("A_B_conversion") == "c2");
    s.getReactions().setLocation("A_B_conversion", "c1");
    REQUIRE(s.getReactions().getIds("c1").size() == 1);
    REQUIRE(s.getReactions().getIds("c2").size() == 0);
    REQUIRE(s.getReactions().getName("A_B_conversion") == "A to B conversion");
    REQUIRE(s.getReactions().getLocation("A_B_conversion") == "c1");
    s.getReactions().remove("A_B_conversion");
    REQUIRE(s.getReactions().getIds("c1").size() == 0);
  }
}

TEST_CASE("SBML: load model, refine mesh, save",
          "[core/model/model][core/model][core][model][mesh]") {
  auto s{getExampleModel(Mod::ABtoC)};
  REQUIRE(s.getGeometry().getMesh2d()->getNumBoundaries() == 1);
  REQUIRE(s.getGeometry().getMesh2d()->getBoundaryMaxPoints(0) == 16);
  auto oldNumTriangleIndices =
      s.getGeometry().getMesh2d()->getTriangleIndicesAsFlatArray(0).size();
  // refine boundary and mesh
  s.getGeometry().getMesh2d()->setCompartmentMaxTriangleArea(0, 32);
  REQUIRE(s.getGeometry().getMesh2d()->getNumBoundaries() == 1);
  REQUIRE(s.getGeometry().getMesh2d()->getTriangleIndicesAsFlatArray(0).size() >
          oldNumTriangleIndices);
  auto maxArea = s.getGeometry().getMesh2d()->getCompartmentMaxTriangleArea(0);
  auto numTriangleIndices =
      s.getGeometry().getMesh2d()->getTriangleIndicesAsFlatArray(0).size();
  // save SBML doc
  s.exportSBMLFile("tmpmodelmeshrefine.xml");
  // import again
  model::Model s2;
  s2.importSBMLFile("tmpmodelmeshrefine.xml");
  REQUIRE(s.getGeometry().getMesh2d()->getNumBoundaries() == 1);
  REQUIRE(s2.getGeometry().getMesh2d()->getCompartmentMaxTriangleArea(0) ==
          maxArea);
  REQUIRE(
      s2.getGeometry().getMesh2d()->getTriangleIndicesAsFlatArray(0).size() ==
      numTriangleIndices);
}

TEST_CASE("SBML: load single compartment model, change volume of geometry",
          "[core/model/model][core/model][core][model][mesh]") {
  auto s{getExampleModel(Mod::ABtoC)};
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(1.0));
  // 100x100 image, 100m x 100m physical volume
  REQUIRE(s.getGeometry().getPhysicalSize().width() == dbl_approx(100.0));
  REQUIRE(s.getGeometry().getPhysicalSize().height() == dbl_approx(100.0));
  // z direction assumed to be 1 in length units
  REQUIRE(s.getGeometry().getPhysicalSize().depth() == dbl_approx(1.0));
  // 3149 pixels, pixel is 1m^3, volume units litres
  REQUIRE(s.getCompartments().getSize("comp") == dbl_approx(3149 * 1000));

  // change voxel width & height: compartment sizes, interior points updated
  s.getGeometry().setVoxelSize({0.01, 0.01, 1.0});
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(0.01));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(0.01));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(1.0));
  // physical volume rescaled
  REQUIRE(s.getGeometry().getPhysicalSize().width() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getPhysicalSize().height() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getPhysicalSize().depth() == dbl_approx(1.0));
  // compartment sizes rescaled: pixel is now 0.01*0.01*1 = 1e-4 m^2
  REQUIRE(s.getCompartments().getSize("comp") == dbl_approx(314.9));
  auto interiorPoint{s.getCompartments().getInteriorPoints("comp").value()[0]};
  // 2-d interior points rescaled
  REQUIRE(interiorPoint.x() == dbl_approx(0.485));
  REQUIRE(interiorPoint.y() == dbl_approx(0.515));
  // todo: z interior point?

  // change voxel depth: compartment sizes, interior points updated
  s.getGeometry().setVoxelSize({0.01, 0.01, 0.1});
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(0.01));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(0.01));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(0.1));
  // physical volume rescaled
  REQUIRE(s.getGeometry().getPhysicalSize().width() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getPhysicalSize().height() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getPhysicalSize().depth() == dbl_approx(0.1));
  // compartment sizes rescaled: pixel is now 0.01*0.01*0.1 = 1e-5 m^2
  REQUIRE(s.getCompartments().getSize("comp") == dbl_approx(31.49));
  interiorPoint = s.getCompartments().getInteriorPoints("comp").value()[0];
  // 2-d interior points not affected
  REQUIRE(interiorPoint.x() == dbl_approx(0.485));
  REQUIRE(interiorPoint.y() == dbl_approx(0.515));
}

TEST_CASE("SBML: load multi-compartment model, change volume of geometry",
          "[core/model/model][core/model][core][model][mesh]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  // 100m x 100m x 1m geometry, volume units: litres
  REQUIRE(s.getGeometry().getPhysicalSize().width() == dbl_approx(100.0));
  REQUIRE(s.getGeometry().getPhysicalSize().height() == dbl_approx(100.0));
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(1.0));
  REQUIRE(s.getUnits().getLength().name == "m");
  REQUIRE(s.getUnits().getVolume().name == "L");
  // volume of 1 pixel = 1m^3 = 1e3 litres
  REQUIRE(s.getCompartments().getSize("c1") == dbl_approx(5441 * 1e3));
  REQUIRE(s.getCompartments().getSize("c2") == dbl_approx(4034 * 1e3));
  REQUIRE(s.getCompartments().getSize("c3") == dbl_approx(525 * 1e3));
  // area of 1 pixel = 1m^2
  REQUIRE(s.getCompartments().getSize("c1_c2_membrane") == dbl_approx(338));
  REQUIRE(s.getCompartments().getSize("c2_c3_membrane") == dbl_approx(108));
  auto interiorPoint{s.getCompartments().getInteriorPoints("c1").value()[0]};
  REQUIRE(interiorPoint.x() == dbl_approx(68.5));
  REQUIRE(interiorPoint.y() == dbl_approx(83.5));
  // change voxel width/height: compartment/membrane sizes, interior points
  // updated
  double a = 1.1285;
  s.getGeometry().setVoxelSize({a, a, 1.0});
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(a));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(a));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(1.0));
  REQUIRE(s.getGeometry().getPhysicalSize().width() == dbl_approx(100.0 * a));
  REQUIRE(s.getGeometry().getPhysicalSize().height() == dbl_approx(100.0 * a));
  REQUIRE(s.getGeometry().getPhysicalSize().depth() == dbl_approx(1.0));
  REQUIRE(s.getCompartments().getSize("c1") == dbl_approx(a * a * 5441 * 1e3));
  REQUIRE(s.getCompartments().getSize("c2") == dbl_approx(a * a * 4034 * 1e3));
  REQUIRE(s.getCompartments().getSize("c3") == dbl_approx(a * a * 525 * 1e3));
  REQUIRE(s.getCompartments().getSize("c1_c2_membrane") == dbl_approx(a * 338));
  REQUIRE(s.getCompartments().getSize("c2_c3_membrane") == dbl_approx(a * 108));
  interiorPoint = s.getCompartments().getInteriorPoints("c1").value()[0];
  REQUIRE(interiorPoint.x() == dbl_approx(a * 68.5));
  REQUIRE(interiorPoint.y() == dbl_approx(a * 83.5));
  // change voxel depth: compartment/membrane sizes, interior points updated
  double d = 0.937694;
  s.getGeometry().setVoxelSize({a, a, d});
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(a));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(a));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(d));
  REQUIRE(s.getGeometry().getPhysicalSize().width() == dbl_approx(100.0 * a));
  REQUIRE(s.getGeometry().getPhysicalSize().height() == dbl_approx(100.0 * a));
  REQUIRE(s.getGeometry().getPhysicalSize().depth() == dbl_approx(1.0 * d));
  REQUIRE(s.getCompartments().getSize("c1") ==
          dbl_approx(a * a * d * 5441 * 1e3));
  REQUIRE(s.getCompartments().getSize("c2") ==
          dbl_approx(a * a * d * 4034 * 1e3));
  REQUIRE(s.getCompartments().getSize("c3") ==
          dbl_approx(a * a * d * 525 * 1e3));
  REQUIRE(s.getCompartments().getSize("c1_c2_membrane") ==
          dbl_approx(a * d * 338));
  REQUIRE(s.getCompartments().getSize("c2_c3_membrane") ==
          dbl_approx(a * d * 108));
  interiorPoint = s.getCompartments().getInteriorPoints("c1").value()[0];
  REQUIRE(interiorPoint.x() == dbl_approx(a * 68.5));
  REQUIRE(interiorPoint.y() == dbl_approx(a * 83.5));
  // export sbml, import sbml, check all sizes preserved
  auto xml{s.getXml().toStdString()};
  model::Model s2;
  s2.importSBMLString(xml);
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(a));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(a));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(d));
  REQUIRE(s2.getGeometry().getPhysicalSize().width() == dbl_approx(100.0 * a));
  REQUIRE(s2.getGeometry().getPhysicalSize().height() == dbl_approx(100.0 * a));
  REQUIRE(s2.getCompartments().getSize("c1") ==
          dbl_approx(a * a * d * 5441 * 1e3));
  REQUIRE(s2.getCompartments().getSize("c2") ==
          dbl_approx(a * a * d * 4034 * 1e3));
  REQUIRE(s2.getCompartments().getSize("c3") ==
          dbl_approx(a * a * d * 525 * 1e3));
  REQUIRE(s2.getCompartments().getSize("c1_c2_membrane") ==
          dbl_approx(a * d * 338));
  REQUIRE(s2.getCompartments().getSize("c2_c3_membrane") ==
          dbl_approx(a * d * 108));
  interiorPoint = s2.getCompartments().getInteriorPoints("c1").value()[0];
  REQUIRE(interiorPoint.x() == dbl_approx(a * 68.5));
  REQUIRE(interiorPoint.y() == dbl_approx(a * 83.5));
}

TEST_CASE("SBML: load .xml model, simulate, save as .sme, load .sme",
          "[core/model/model][core/model][core][model]") {
  auto s{getExampleModel(Mod::ABtoC)};
  simulate::Simulation sim(s);
  sim.doTimesteps(0.1, 2);
  s.exportSMEFile("tmpmodelsmetest.sme");
  model::Model s2;
  s2.importFile("tmpmodelsmetest.sme");
  REQUIRE(s2.getCompartments().getIds() == s.getCompartments().getIds());
  REQUIRE(s.getXml() == s2.getXml());
  REQUIRE(s2.getSimulationData().timePoints.size() == 3);
  REQUIRE(s2.getSimulationData().timePoints[2] == dbl_approx(0.2));
  REQUIRE(s2.getSimulationData().concPadding.size() == 3);
  REQUIRE(s2.getSimulationData().concPadding[2] == dbl_approx(0));
  model::Model s3;
  s3.importFile("tmpmodelsmetest.sme");
  // do something that causes ModelCompartments to clear the simulation results
  // https://github.com/spatial-model-editor/spatial-model-editor/issues/666
  s3.getGeometry().setVoxelSize({1.2, 1.2, 1.2}, true);
  REQUIRE(s3.getSimulationData().timePoints.size() == 0);
}

TEST_CASE("SBML: import multi-compartment SBML doc without spatial geometry",
          "[core/model/model][core/model][core][model]") {
  auto s{getTestModel("non-spatial-multi-compartment")};
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
  auto &geometry{s.getGeometry()};
  auto &compartments{s.getCompartments()};
  // reactions in original xml model have no compartment
  auto &reactions{s.getReactions()};
  REQUIRE(reactions.getLocation("trans") == "");
  REQUIRE(reactions.getLocation("conv") == "");
  REQUIRE(reactions.getLocation("degrad") == "");
  REQUIRE(reactions.getLocation("ex") == "");
  REQUIRE(geometry.getIsValid() == false);
  REQUIRE(geometry.getHasImage() == false);
  REQUIRE(reactions.getIsIncompleteODEImport() == true);
  // import a geometry image
  geometry.importGeometryFromImages(
      common::ImageStack{{QImage(":test/geometry/cell.png")}}, false);
  auto colors{geometry.getImages().colorTable()};
  REQUIRE(colors.size() == 4);
  REQUIRE(geometry.getIsValid() == false);
  REQUIRE(geometry.getHasImage() == true);
  REQUIRE(reactions.getIsIncompleteODEImport() == true);
  // assign each compartment to a color region in the image
  compartments.setColor("cyt", colors[1]);
  compartments.setColor("nuc", colors[2]);
  compartments.setColor("org", colors[3]);
  compartments.setColor("ext", colors[0]);
  REQUIRE(geometry.getIsValid() == true);
  REQUIRE(geometry.getHasImage() == true);
  REQUIRE(reactions.getIsIncompleteODEImport() == true);
  // all reactions are now assigned to a valid location
  REQUIRE(reactions.getLocation("trans") == "cyt_nuc_membrane");
  REQUIRE(reactions.getLocation("conv") == "cyt");
  REQUIRE(reactions.getLocation("degrad") == "cyt");
  REQUIRE(reactions.getLocation("ex") == "ext_cyt_membrane");
  // reaction rates have not yet been rescaled
  REQUIRE(symEq(reactions.getRateExpression("trans"),
                "Henri_Michaelis_Menten__irreversible(A, Km, V)"));
  REQUIRE(symEq(reactions.getRateExpression("conv"), "k1 * B"));
  REQUIRE(
      symEq(reactions.getRateExpression("degrad"), "cyt * (k1 * C - k2 * D)"));
  REQUIRE(symEq(reactions.getRateExpression("ex"), "D * k1"));

  auto reactionRescalings{reactions.getSpatialReactionRescalings()};
  reactions.applySpatialReactionRescalings(reactionRescalings);
  REQUIRE(reactions.getIsIncompleteODEImport() == false);
  // reaction rates are rescaled by their compartment volumes / membrane areas
  REQUIRE(symEq(
      reactions.getRateExpression("trans"),
      "0.00367647058823529 * Henri_Michaelis_Menten__irreversible(A, Km, V)"));
  REQUIRE(symEq(reactions.getRateExpression("conv"),
                "6.84181718664477e-5 * k1 * B"));
  REQUIRE(symEq(reactions.getRateExpression("degrad"), "k1 * C - k2 * D"));
  REQUIRE(
      symEq(reactions.getRateExpression("ex"), "0.00166666666666667 * k1 * D"));
}

TEST_CASE("SBML: import SBML doc with compressed sampledField",
          "[core/model/model][core/model][core][model]") {
  auto s{getTestModel("all_SpatialImage_SpatialUseCompression")};
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getName() == "CellOrganizer2_7");
  REQUIRE(s.getGeometry().getIsValid() == true);
  REQUIRE(s.getGeometry().getHasImage() == true);
  REQUIRE(s.getGeometry().getImages().volume().width() == 33);
  REQUIRE(s.getGeometry().getImages().volume().height() == 30);
  REQUIRE(s.getGeometry().getImages().volume().depth() == 24);
  REQUIRE(s.getCompartments().getIds().size() == 4);
  REQUIRE(s.getCompartments().getCompartment("EC")->nVoxels() == 12640);
  REQUIRE(s.getCompartments().getCompartment("cell")->nVoxels() == 6976);
  REQUIRE(s.getCompartments().getCompartment("nuc")->nVoxels() == 4040);
  REQUIRE(s.getCompartments().getCompartment("vesicle")->nVoxels() == 104);
  // export and re-import, check compartment geometry hasn't changed
  s.exportSBMLFile("compressedExported.xml");
  sme::model::Model s2;
  s2.importSBMLFile("compressedExported.xml");
  REQUIRE(s2.getIsValid() == true);
  REQUIRE(s2.getName() == "CellOrganizer2_7");
  REQUIRE(s2.getGeometry().getIsValid() == true);
  REQUIRE(s2.getGeometry().getHasImage() == true);
  REQUIRE(s.getGeometry().getImages().volume().width() == 33);
  REQUIRE(s.getGeometry().getImages().volume().height() == 30);
  REQUIRE(s.getGeometry().getImages().volume().depth() == 24);
  REQUIRE(s2.getCompartments().getIds().size() == 4);
  REQUIRE(s.getCompartments().getCompartment("EC")->nVoxels() == 12640);
  REQUIRE(s.getCompartments().getCompartment("cell")->nVoxels() == 6976);
  REQUIRE(s.getCompartments().getCompartment("nuc")->nVoxels() == 4040);
  REQUIRE(s.getCompartments().getCompartment("vesicle")->nVoxels() == 104);
}

TEST_CASE("Import Combine archive",
          "[combine][archive][core/model/model][core/model][core][model]") {
  QString filename{"liver-simplified.omex"};
  createBinaryFile("archives/" + filename, filename);
  model::Model s;
  s.importFile("i-dont-exist.omex");
  REQUIRE(s.getIsValid() == false);
  s.importFile(filename.toStdString());
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getCurrentFilename() == "liver-simplified");
  REQUIRE(s.getCompartments().getIds().size() == 2);
  REQUIRE(s.getCompartments().getCompartment("cytoplasm")->nVoxels() == 7800);
  REQUIRE(s.getCompartments().getCompartment("nucleus")->nVoxels() == 481);
  REQUIRE(s.getMembranes().getIds().size() == 1);
  const auto &membrane{
      s.getMembranes().getMembrane("cytoplasm_nucleus_membrane")};
  REQUIRE(membrane->getIndexPairs(sme::geometry::Membrane::X).size() == 48);
  REQUIRE(membrane->getIndexPairs(sme::geometry::Membrane::Y).size() == 50);
  REQUIRE(membrane->getIndexPairs(sme::geometry::Membrane::Z).size() == 0);
}
