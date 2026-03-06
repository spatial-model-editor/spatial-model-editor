#include "catch_wrapper.hpp"
#include "math_test_utils.hpp"
#include "model_test_utils.hpp"
#include "sbml_utils.hpp"
#include "sme/gmsh.hpp"
#include "sme/mesh2d.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include "sme/version.hpp"
#include "sme/xml_annotation.hpp"
#include <algorithm>
#include <fstream>
#include <map>
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

[[nodiscard]] static mesh::GMSHMesh makeThreeCompartmentFixedMesh3d() {
  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                       {0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 1.0}};
  gmshMesh.tetrahedra = {
      {{{0, 1, 2, 3}}, 1}, {{{1, 2, 3, 4}}, 2}, {{{1, 3, 4, 5}}, 3}};
  return gmshMesh;
}

[[nodiscard]] static bool
writeParametricSurfaceTrianglesToGmsh(const QString &modelName,
                                      const std::string &filename) {
  auto doc = getTestSbmlDoc(modelName);
  if (doc == nullptr || doc->getModel() == nullptr) {
    return false;
  }
  const auto *model = doc->getModel();
  const auto *plugin = dynamic_cast<const libsbml::SpatialModelPlugin *>(
      model->getPlugin("spatial"));
  if (plugin == nullptr || !plugin->isSetGeometry()) {
    return false;
  }
  const auto *geom = plugin->getGeometry();
  if (geom == nullptr) {
    return false;
  }

  const libsbml::ParametricGeometry *pg{nullptr};
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (const auto *def = geom->getGeometryDefinition(i);
        def != nullptr && def->getIsActive() && def->isParametricGeometry()) {
      pg = dynamic_cast<const libsbml::ParametricGeometry *>(def);
      break;
    }
  }
  if (pg == nullptr || !pg->isSetSpatialPoints() ||
      !pg->getSpatialPoints()->isSetArrayData()) {
    return false;
  }

  std::vector<double> points;
  pg->getSpatialPoints()->getArrayData(points);
  if (points.size() < 3 || points.size() % 3 != 0) {
    return false;
  }
  const auto nNodes = points.size() / 3;

  std::map<std::string, int, std::less<>> domainTypeToTag;
  struct Triangle {
    int tag;
    int n1;
    int n2;
    int n3;
  };
  std::vector<Triangle> triangles;
  for (unsigned iObj = 0; iObj < pg->getNumParametricObjects(); ++iObj) {
    const auto *obj = pg->getParametricObject(iObj);
    if (obj == nullptr) {
      continue;
    }
    if (obj->getPolygonType() !=
        libsbml::PolygonKind_t::SPATIAL_POLYGONKIND_TRIANGLE) {
      continue;
    }
    auto [it, inserted] = domainTypeToTag.try_emplace(
        obj->getDomainType(), static_cast<int>(domainTypeToTag.size()) + 1);
    (void)inserted;
    const auto tag = it->second;

    std::vector<int> pointIndex;
    obj->getPointIndex(pointIndex);
    if (pointIndex.size() % 3 != 0) {
      return false;
    }
    for (std::size_t i = 0; i < pointIndex.size(); i += 3) {
      const auto p0 = pointIndex[i];
      const auto p1 = pointIndex[i + 1];
      const auto p2 = pointIndex[i + 2];
      if (p0 < 0 || p1 < 0 || p2 < 0 ||
          static_cast<std::size_t>(p0) >= nNodes ||
          static_cast<std::size_t>(p1) >= nNodes ||
          static_cast<std::size_t>(p2) >= nNodes) {
        return false;
      }
      triangles.push_back({tag, p0 + 1, p1 + 1, p2 + 1});
    }
  }
  if (triangles.empty()) {
    return false;
  }

  std::ofstream out(filename);
  if (!out) {
    return false;
  }

  out << "$MeshFormat\n";
  out << "2.2 0 8\n";
  out << "$EndMeshFormat\n";

  out << "$PhysicalNames\n";
  out << domainTypeToTag.size() << "\n";
  for (const auto &[domainType, tag] : domainTypeToTag) {
    out << "2 " << tag << " \"" << domainType << "\"\n";
  }
  out << "$EndPhysicalNames\n";

  out << "$Nodes\n";
  out << nNodes << "\n";
  for (std::size_t i = 0; i < nNodes; ++i) {
    out << (i + 1) << " " << points[3 * i] << " " << points[3 * i + 1] << " "
        << points[3 * i + 2] << "\n";
  }
  out << "$EndNodes\n";

  out << "$Elements\n";
  out << triangles.size() << "\n";
  for (std::size_t i = 0; i < triangles.size(); ++i) {
    const auto &t = triangles[i];
    out << (i + 1) << " 2 2 " << t.tag << " " << t.tag << " " << t.n1 << " "
        << t.n2 << " " << t.n3 << "\n";
  }
  out << "$EndElements\n";
  return true;
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
      s.getSpecies().setAnalyticDiffusionConstant("spec1c0", "x+1");
      s.getSpecies().setCrossDiffusionConstant("spec1c0", "spec0c0",
                                               "spec0c0 + 1");
      REQUIRE(
          symEq(s.getSpecies().getCrossDiffusionConstant("spec1c0", "spec0c0"),
                "spec0c0 + 1"));
      REQUIRE(s.getSpecies()
                  .getCrossDiffusionConstant("spec1c0", "spec0c1")
                  .isEmpty());
      const auto spec1c0DiffusionBeforeInvalidExpr =
          s.getSpecies().getField("spec1c0")->getDiffusionConstant();
      s.getSpecies().setAnalyticDiffusionConstant("spec1c0", "x+(");
      REQUIRE(symEq(s.getSpecies().getAnalyticDiffusionConstant("spec1c0"),
                    "x + 1"));
      REQUIRE(s.getSpecies().getField("spec1c0")->getDiffusionConstant() ==
              spec1c0DiffusionBeforeInvalidExpr);
      s.getSpecies().setSampledFieldDiffusionConstant("spec0c1",
                                                      {0.0, 1.0, 2.0});
      CAPTURE(s.getSpecies().getDiffusionConstant("spec0c0"));
      CAPTURE(s.getSpecies().getDiffusionConstant("spec1c0"));
      CAPTURE(s.getSpecies().getDiffusionConstant("spec0c1"));
      REQUIRE(s.getSpecies().getDiffusionConstant("spec0c0") ==
              dbl_approx(0.123));
      REQUIRE(s.getSpecies().getDiffusionConstant("spec1c0") ==
              dbl_approx(0.999999));
      REQUIRE(s.getSpecies().getDiffusionConstant("spec0c1") ==
              dbl_approx(23.1 + 1e-12));
      REQUIRE(s.getSpecies().getDiffusionConstantType("spec0c0") ==
              model::SpatialDataType::Uniform);
      REQUIRE(s.getSpecies().getDiffusionConstantType("spec1c0") ==
              model::SpatialDataType::Analytic);
      REQUIRE(s.getSpecies().getDiffusionConstantType("spec0c1") ==
              model::SpatialDataType::Image);
      auto spec0c1DiffArray =
          s.getSpecies().getSampledFieldDiffusionConstant("spec0c1");

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
      REQUIRE(s2.getSpecies().getDiffusionConstantType("spec0c0") ==
              model::SpatialDataType::Uniform);
      REQUIRE(s2.getSpecies().getDiffusionConstantType("spec1c0") ==
              model::SpatialDataType::Analytic);
      REQUIRE(s2.getSpecies().getDiffusionConstantType("spec0c1") ==
              model::SpatialDataType::Image);
      REQUIRE(symEq(s2.getSpecies().getAnalyticDiffusionConstant("spec1c0"),
                    "x + 1"));
      REQUIRE(
          symEq(s2.getSpecies().getCrossDiffusionConstant("spec1c0", "spec0c0"),
                "spec0c0 + 1"));
      REQUIRE(s2.getSpecies()
                  .getCrossDiffusionConstant("spec1c0", "spec0c1")
                  .isEmpty());
      REQUIRE(s2.getSpecies().getSampledFieldDiffusionConstant("spec0c1") ==
              spec0c1DiffArray);
    }
    SECTION("diffusion field updated on compartment change") {
      QImage img(4, 1, QImage::Format_RGB32);
      img.setPixel(0, 0, 0xffaaaaaa);
      img.setPixel(1, 0, 0xffaaaaaa);
      img.setPixel(2, 0, 0xff525252);
      img.setPixel(3, 0, 0xffffffff);
      s.getGeometry().importGeometryFromImages(common::ImageStack{{img}},
                                               false);
      s.getCompartments().setColor("compartment0", 0xffaaaaaa);
      s.getCompartments().setColor("compartment1", 0xff525252);
      s.getSpecies().setIsSpatial("spec0c0", true);
      s.getSpecies().setSampledFieldDiffusionConstant("spec0c0",
                                                      {0.0, 1.0, 2.0, 3.0});
      const auto *comp0 = s.getCompartments().getCompartment("compartment0");
      const auto *comp1 = s.getCompartments().getCompartment("compartment1");
      const auto *fieldBefore = s.getSpecies().getField("spec0c0");
      REQUIRE(comp0 != nullptr);
      REQUIRE(comp1 != nullptr);
      REQUIRE(fieldBefore != nullptr);
      REQUIRE(fieldBefore->getDiffusionConstant().size() == comp0->nVoxels());
      s.getSpecies().setCompartment("spec0c0", "compartment1");
      const auto *fieldAfter = s.getSpecies().getField("spec0c0");
      REQUIRE(fieldAfter != nullptr);
      REQUIRE(fieldAfter->getDiffusionConstant().size() == comp1->nVoxels());
    }
    SECTION("sampled diffusion field and imported geometry size mismatch") {
      s.getSpecies().setIsSpatial("spec0c0", true);
      s.getSpecies().setSampledFieldDiffusionConstant("spec0c0",
                                                      {0.0, 1.0, 2.0});
      QImage img(4, 1, QImage::Format_RGB32);
      img.fill(0xffaaaaaa);
      s.getGeometry().importGeometryFromImages(common::ImageStack{{img}},
                                               false);
      REQUIRE_NOTHROW(
          s.getGeometry().setVoxelSize(s.getGeometry().getVoxelSize()));
    }
  }
}

TEST_CASE("SBML: default geometry avoids xyz parameter id clashes",
          "[core/model/model][core/model][core][model]") {
  createSBMLlvl2doc("tmpmodel_xyz_param_clash.xml");
  std::unique_ptr<libsbml::SBMLDocument> docIn(
      libsbml::readSBMLFromFile("tmpmodel_xyz_param_clash.xml"));
  REQUIRE(docIn != nullptr);
  auto *modelIn = docIn->getModel();
  REQUIRE(modelIn != nullptr);
  for (const auto *id : {"x", "y", "z"}) {
    auto *param = modelIn->createParameter();
    param->setId(id);
    param->setConstant(true);
    param->setValue(0.0);
  }
  libsbml::SBMLWriter().writeSBML(docIn.get(), "tmpmodel_xyz_param_clash.xml");

  model::Model s;
  s.importSBMLFile("tmpmodel_xyz_param_clash.xml");
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getErrorMessage().isEmpty());

  auto doc{toSbmlDoc(s)};
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);

  // pre-existing non-spatial parameters keep their ids
  REQUIRE(model->getParameter("x") != nullptr);
  REQUIRE(model->getParameter("y") != nullptr);
  REQUIRE(model->getParameter("z") != nullptr);

  const auto *xParam = model::getSpatialCoordinateParam(
      model, libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  const auto *yParam = model::getSpatialCoordinateParam(
      model, libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  const auto *zParam = model::getSpatialCoordinateParam(
      model, libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z);
  REQUIRE(xParam != nullptr);
  REQUIRE(yParam != nullptr);
  REQUIRE(zParam != nullptr);

  REQUIRE(xParam->getId() == "x_");
  REQUIRE(yParam->getId() == "y_");
  REQUIRE(zParam->getId() == "z_");
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

TEST_CASE("SBML: load 3d model, refine mesh cell volume, save",
          "[core/model/model][core/model][core][model][mesh]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  auto *mesh3d = s.getGeometry().getMesh3d();
  REQUIRE(mesh3d != nullptr);
  REQUIRE(mesh3d->isValid());

  mesh3d->setCompartmentMaxCellVolume(0, 8);
  const auto expectedMaxCellVolumes = mesh3d->getCompartmentMaxCellVolume();
  REQUIRE_FALSE(expectedMaxCellVolumes.empty());
  for (const auto v : expectedMaxCellVolumes) {
    REQUIRE(v == 8);
  }

  s.exportSBMLFile("tmpmodelmeshrefine3d.xml");
  REQUIRE(s.getMeshParameters().maxCellVolumes == expectedMaxCellVolumes);

  model::Model s2;
  s2.importSBMLFile("tmpmodelmeshrefine3d.xml");
  REQUIRE(s2.getMeshParameters().maxCellVolumes == expectedMaxCellVolumes);
  REQUIRE(s2.getGeometry().getMesh3d() != nullptr);
  REQUIRE(s2.getGeometry().getMesh3d()->isValid());
  REQUIRE(s2.getGeometry().getMesh3d()->getCompartmentMaxCellVolume() ==
          expectedMaxCellVolumes);
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
  auto xml = s.getXml();
  REQUIRE(
      xml.contains(QString("<!-- Created by Spatial Model Editor version %1")
                       .arg(common::SPATIAL_MODEL_EDITOR_VERSION)));
  model::Model s2;
  s2.importSBMLString(xml.toStdString());
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

TEST_CASE("SBML: load model, change geometry origin",
          "[core/model/model][core/model][core][model][mesh][origin]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  REQUIRE(s.getGeometry().getPhysicalOrigin().p.x() == dbl_approx(0.0));
  REQUIRE(s.getGeometry().getPhysicalOrigin().p.y() == dbl_approx(0.0));
  REQUIRE(s.getGeometry().getPhysicalOrigin().z == dbl_approx(0.0));
  auto p0{s.getGeometry().getPhysicalPoint({0, 0, 0})};
  REQUIRE(p0.p.x() == dbl_approx(0.5));
  REQUIRE(p0.p.y() == dbl_approx(99.5));
  REQUIRE(p0.z == dbl_approx(0.5));
  const auto c1Size{s.getCompartments().getSize("c1")};
  const auto c2Size{s.getCompartments().getSize("c2")};
  const auto c3Size{s.getCompartments().getSize("c3")};

  s.getGeometry().setPhysicalOrigin({1.25, -2.5, 3.75});
  REQUIRE(s.getGeometry().getPhysicalOrigin().p.x() == dbl_approx(1.25));
  REQUIRE(s.getGeometry().getPhysicalOrigin().p.y() == dbl_approx(-2.5));
  REQUIRE(s.getGeometry().getPhysicalOrigin().z == dbl_approx(3.75));
  auto p1{s.getGeometry().getPhysicalPoint({0, 0, 0})};
  REQUIRE(p1.p.x() == dbl_approx(1.75));
  REQUIRE(p1.p.y() == dbl_approx(97.0));
  REQUIRE(p1.z == dbl_approx(4.25));
  REQUIRE(s.getCompartments().getSize("c1") == dbl_approx(c1Size));
  REQUIRE(s.getCompartments().getSize("c2") == dbl_approx(c2Size));
  REQUIRE(s.getCompartments().getSize("c3") == dbl_approx(c3Size));

  auto xml = s.getXml();
  model::Model s2;
  s2.importSBMLString(xml.toStdString());
  REQUIRE(s2.getGeometry().getPhysicalOrigin().p.x() == dbl_approx(1.25));
  REQUIRE(s2.getGeometry().getPhysicalOrigin().p.y() == dbl_approx(-2.5));
  REQUIRE(s2.getGeometry().getPhysicalOrigin().z == dbl_approx(3.75));
  auto p2{s2.getGeometry().getPhysicalPoint({0, 0, 0})};
  REQUIRE(p2.p.x() == dbl_approx(1.75));
  REQUIRE(p2.p.y() == dbl_approx(97.0));
  REQUIRE(p2.z == dbl_approx(4.25));
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
  auto xml = s.getXml();
  auto xml2 = s2.getXml();
  // only compare xml after comment as it contains a datetime stamp
  REQUIRE(xml.slice(xml.indexOf("<sbml")) == xml2.slice(xml2.indexOf("<sbml")));
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

TEST_CASE("SBML: fixed mesh round-trip via ParametricGeometry",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto &colors = s.getCompartments().getColors();
  REQUIRE(colors.size() >= 3);
  REQUIRE(colors[0] != 0);
  REQUIRE(colors[1] != 0);
  REQUIRE(colors[2] != 0);

  auto gmshMesh = makeThreeCompartmentFixedMesh3d();
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {colors[0], 1}, {colors[1], 2}, {colors[2], 3}};
  s.getGeometry().setImportedGmshMesh(gmshMesh, colorTagPairs);
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh3d() != nullptr);
  REQUIRE(s.getGeometry().getMesh3d()->isValid());

  auto doc = toSbmlDoc(s);
  REQUIRE(doc != nullptr);
  auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
      doc->getModel()->getPlugin("spatial"));
  REQUIRE(plugin != nullptr);
  auto *geom = plugin->getGeometry();
  REQUIRE(geom != nullptr);
  const libsbml::ParametricGeometry *pg{nullptr};
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (const auto *def = geom->getGeometryDefinition(i);
        def->isParametricGeometry() && def->getIsActive()) {
      pg = dynamic_cast<const libsbml::ParametricGeometry *>(def);
    }
  }
  REQUIRE(pg != nullptr);
  REQUIRE(pg->isSetSpatialPoints());
  REQUIRE(pg->getSpatialPoints()->getActualArrayDataLength() == 18);
  REQUIRE(pg->getNumParametricObjects() == 3);
  const auto *obj = pg->getParametricObject(0);
  REQUIRE(obj != nullptr);
  REQUIRE(obj->getPointIndexLength() == 12);
  const auto *obj1 = pg->getParametricObject(1);
  REQUIRE(obj1 != nullptr);
  REQUIRE(obj1->getPointIndexLength() == 12);
  const auto *obj2 = pg->getParametricObject(2);
  REQUIRE(obj2 != nullptr);
  REQUIRE(obj2->getPointIndexLength() == 12);

  model::Model s2;
  s2.importSBMLString(s.getXml().toStdString(), "fixed-mesh.xml");
  REQUIRE(s2.getGeometry().hasImportedMesh());
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  // fixed imported topology should be active immediately after SBML import
  REQUIRE(s2.getGeometry().getMesh3d() != nullptr);
  REQUIRE(s2.getGeometry().getMesh3d()->isValid());
  const auto &tetrahedra =
      s2.getGeometry().getMesh3d()->getTetrahedronIndices();
  REQUIRE(tetrahedra.size() >= 3);
  REQUIRE(tetrahedra[0].size() == 1);
  REQUIRE(tetrahedra[1].size() == 1);
  REQUIRE(tetrahedra[2].size() == 1);
}

TEST_CASE("SBML: mesh source remains voxel when ParametricGeometry is present",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto &colors = s.getCompartments().getColors();
  REQUIRE(colors.size() >= 3);

  auto gmshMesh = makeThreeCompartmentFixedMesh3d();
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {colors[0], 1}, {colors[1], 2}, {colors[2], 3}};
  s.getGeometry().setImportedGmshMesh(gmshMesh, colorTagPairs);
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().hasImportedMesh());

  model::Model s2;
  s2.importSBMLString(s.getXml().toStdString(), "fixed-mesh-preserve.xml");
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  REQUIRE(s2.getGeometry().hasImportedMesh());

  s2.getMeshParameters().meshSourceType = model::MeshSourceType::VoxelGeometry;
  s2.getGeometry().updateMesh();
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::VoxelGeometry);

  model::Model s3;
  s3.importSBMLString(s2.getXml().toStdString(),
                      "voxel-source-with-parametric.xml");
  REQUIRE(s3.getGeometry().hasImportedMesh());
  REQUIRE(s3.getMeshParameters().meshSourceType ==
          model::MeshSourceType::VoxelGeometry);
}

TEST_CASE("SBML: incompatible fixed mesh ParametricGeometry falls back with "
          "diagnostic",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto &colors = s.getCompartments().getColors();
  REQUIRE(colors.size() >= 3);
  REQUIRE(colors[0] != 0);
  REQUIRE(colors[1] != 0);
  REQUIRE(colors[2] != 0);

  auto gmshMesh = makeThreeCompartmentFixedMesh3d();
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {colors[0], 1}, {colors[1], 2}, {colors[2], 3}};
  s.getGeometry().setImportedGmshMesh(gmshMesh, colorTagPairs);
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  auto doc = toSbmlDoc(s);
  REQUIRE(doc != nullptr);

  auto *plugin = dynamic_cast<libsbml::SpatialModelPlugin *>(
      doc->getModel()->getPlugin("spatial"));
  REQUIRE(plugin != nullptr);
  auto *geom = plugin->getGeometry();
  REQUIRE(geom != nullptr);
  libsbml::ParametricGeometry *pg{nullptr};
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (auto *def = geom->getGeometryDefinition(i);
        def->isParametricGeometry() && def->getIsActive()) {
      pg = dynamic_cast<libsbml::ParametricGeometry *>(def);
    }
  }
  REQUIRE(pg != nullptr);
  auto *obj = pg->getParametricObject(0);
  REQUIRE(obj != nullptr);
  obj->setPointIndex(std::vector<int>{0, 1, 2});
  obj->setPointIndexLength(3);

  common::unique_C_ptr<char> xmlChar{libsbml::writeSBMLToString(doc.get())};
  model::Model s2;
  s2.importSBMLString(xmlChar.get(), "invalid-fixed-mesh.xml");
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  REQUIRE_FALSE(s2.getGeometry().hasImportedMesh());
  REQUIRE(!s2.getGeometry().getFixedMeshImportDiagnostic().isEmpty());
  REQUIRE(
      s2.getGeometry().getFixedMeshImportDiagnostic().contains("pointIndex"));
  s2.getGeometry().updateMesh();
  REQUIRE(s2.getGeometry().getMesh3d() != nullptr);
  REQUIRE(s2.getGeometry().getMesh3d()->isValid());
}

TEST_CASE("Switching 3d mesh source to fixed captures current voxel mesh",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh3d() != nullptr);
  REQUIRE(s.getGeometry().getMesh3d()->isValid());

  const auto getTetCounts = [&]() {
    std::vector<std::size_t> counts;
    const auto &tetrahedra =
        s.getGeometry().getMesh3d()->getTetrahedronIndices();
    counts.reserve(tetrahedra.size());
    for (const auto &compTets : tetrahedra) {
      counts.push_back(compTets.size());
    }
    return counts;
  };

  const auto voxelCounts = getTetCounts();

  const auto &colors = s.getCompartments().getColors();
  REQUIRE(colors.size() >= 3);
  auto gmshMesh = makeThreeCompartmentFixedMesh3d();
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {colors[0], 1}, {colors[1], 2}, {colors[2], 3}};
  s.getGeometry().setImportedGmshMesh(gmshMesh, colorTagPairs);
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh3d() != nullptr);
  REQUIRE(s.getGeometry().getMesh3d()->isValid());
  const auto importedFixedCounts = getTetCounts();

  s.getMeshParameters().meshSourceType = model::MeshSourceType::VoxelGeometry;
  s.getGeometry().updateMesh();
  const auto voxelCountsAfterSwitchBack = getTetCounts();
  // Re-meshing voxel geometry is not guaranteed to produce identical
  // tetrahedron counts every time, but switching away from fixed mode must not
  // keep using the imported fixed topology.
  REQUIRE(voxelCountsAfterSwitchBack != importedFixedCounts);
  REQUIRE(voxelCountsAfterSwitchBack.size() == voxelCounts.size());

  s.getGeometry().captureCurrentMeshAsFixedTopology();
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh3d() != nullptr);
  REQUIRE(s.getGeometry().getMesh3d()->isValid());
  const auto capturedFixedCounts = getTetCounts();

  REQUIRE(capturedFixedCounts == voxelCountsAfterSwitchBack);
  REQUIRE(capturedFixedCounts != importedFixedCounts);
}

TEST_CASE("SBML: fixed 2d mesh round-trip via ParametricGeometry",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  REQUIRE(s.getGeometry().getImages().volume().depth() == 1);

  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());

  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().hasImportedMesh());
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());

  const auto &triangles = s.getGeometry().getMesh2d()->getTriangleIndices();
  std::vector<std::size_t> triangleCounts;
  triangleCounts.reserve(triangles.size());
  for (const auto &compTriangles : triangles) {
    triangleCounts.push_back(compTriangles.size());
  }

  model::Model s2;
  s2.importSBMLString(s.getXml().toStdString(), "fixed-mesh-2d.xml");
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  REQUIRE(s2.getGeometry().hasImportedMesh());
  REQUIRE(s2.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s2.getGeometry().getMesh2d()->isValid());

  const auto &triangles2 = s2.getGeometry().getMesh2d()->getTriangleIndices();
  REQUIRE(triangles2.size() == triangleCounts.size());
  for (std::size_t i = 0; i < triangleCounts.size(); ++i) {
    REQUIRE(triangles2[i].size() == triangleCounts[i]);
  }

  s2.getMeshParameters().maxPoints = {4};
  s2.getMeshParameters().maxAreas.assign(triangleCounts.size(), 1);
  s2.getGeometry().updateMesh();
  REQUIRE(s2.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s2.getGeometry().getMesh2d()->isValid());
  const auto &triangles3 = s2.getGeometry().getMesh2d()->getTriangleIndices();
  REQUIRE(triangles3.size() == triangleCounts.size());
  for (std::size_t i = 0; i < triangleCounts.size(); ++i) {
    REQUIRE(triangles3[i].size() == triangleCounts[i]);
  }
}

TEST_CASE(
    "SBML: saving fixed 2d mesh preserves voxel meshing max-area settings",
    "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  const auto expectedMaxPoints = s.getMeshParameters().maxPoints;
  const auto expectedMaxAreas = s.getMeshParameters().maxAreas;
  REQUIRE_FALSE(expectedMaxPoints.empty());
  REQUIRE_FALSE(expectedMaxAreas.empty());

  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().hasImportedMesh());

  s.exportSBMLFile("tmp-fixed-2d-preserve-voxel-mesh-settings.xml");
  REQUIRE(s.getMeshParameters().maxPoints == expectedMaxPoints);
  REQUIRE(s.getMeshParameters().maxAreas == expectedMaxAreas);

  model::Model s2;
  s2.importSBMLFile("tmp-fixed-2d-preserve-voxel-mesh-settings.xml");
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  REQUIRE(s2.getMeshParameters().maxPoints == expectedMaxPoints);
  REQUIRE(s2.getMeshParameters().maxAreas == expectedMaxAreas);
}

TEST_CASE("2d Gmsh fixed mesh import uses triangle topology",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::SingleCompartmentDiffusion)};
  REQUIRE(s.getGeometry().getImages().volume().depth() == 1);

  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {
      {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}};
  gmshMesh.triangles = {{{0, 1, 2}, 1}, {{1, 3, 2}, 1}};
  const auto image = mesh::voxelizeGMSHMesh(gmshMesh, 24);
  REQUIRE(!image.empty());
  REQUIRE(image.volume().depth() == 1);

  s.getGeometry().importGeometryFromImages(image, false);
  const auto &ids = s.getCompartments().getIds();
  REQUIRE(!ids.isEmpty());
  s.getCompartments().setColor(ids[0], common::indexedColors()[0].rgb());
  s.getGeometry().setImportedGmshMesh(gmshMesh);
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().hasImportedMesh());
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());

  const auto &triangles = s.getGeometry().getMesh2d()->getTriangleIndices();
  std::size_t nTriangles{0};
  for (const auto &compTriangles : triangles) {
    nTriangles += compTriangles.size();
  }
  REQUIRE(nTriangles == gmshMesh.triangles.size());

  s.getMeshParameters().maxPoints = {4};
  s.getMeshParameters().maxAreas = {1};
  s.getGeometry().updateMesh();
  const auto &triangles2 = s.getGeometry().getMesh2d()->getTriangleIndices();
  std::size_t nTriangles2{0};
  for (const auto &compTriangles : triangles2) {
    nTriangles2 += compTriangles.size();
  }
  REQUIRE(nTriangles2 == nTriangles);
}

TEST_CASE("2d Gmsh fixed mesh import requires compartment color mapping",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::SingleCompartmentDiffusion)};
  REQUIRE(s.getGeometry().getImages().volume().depth() == 1);

  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {
      {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}};
  gmshMesh.triangles = {{{0, 1, 2}, 1}, {{1, 3, 2}, 1}};
  const auto image = mesh::voxelizeGMSHMesh(gmshMesh, 24);
  REQUIRE(!image.empty());
  REQUIRE(image.volume().depth() == 1);

  s.getGeometry().importGeometryFromImages(image, false);
  const auto &ids = s.getCompartments().getIds();
  REQUIRE(!ids.isEmpty());
  for (const auto &id : ids) {
    s.getCompartments().setColor(id, 0);
  }
  REQUIRE_FALSE(s.getGeometry().hasImportedMesh());

  s.getGeometry().setImportedGmshMesh(gmshMesh);
  REQUIRE(s.getGeometry().hasImportedMesh());
}

TEST_CASE("2d Gmsh fixed mesh import rejects unmapped triangle tags",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::SingleCompartmentDiffusion)};
  REQUIRE(s.getGeometry().getImages().volume().depth() == 1);

  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {
      {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}};
  gmshMesh.triangles = {{{0, 1, 2}, 1}, {{1, 3, 2}, 2}};
  const auto image = mesh::voxelizeGMSHMesh(gmshMesh, 24);
  REQUIRE(!image.empty());
  REQUIRE(image.volume().depth() == 1);

  s.getGeometry().importGeometryFromImages(image, false);
  const auto &ids = s.getCompartments().getIds();
  REQUIRE(ids.size() == 1);
  s.getCompartments().setColor(ids[0], common::indexedColors()[0].rgb());
  REQUIRE_FALSE(s.getGeometry().hasImportedMesh());

  s.getGeometry().setImportedGmshMesh(gmshMesh);
  REQUIRE(s.getGeometry().hasImportedMesh());
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE_FALSE(s.getGeometry().getIsMeshValid());
  REQUIRE(s.getGeometry().getMesh2d() == nullptr);
  REQUIRE(s.getGeometry().getFixedMeshImportDiagnostic().contains(
      "tags do not match"));
}

TEST_CASE("2d Gmsh fixed mesh import in empty model is applied after color "
          "assignment",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::SingleCompartmentDiffusion)};
  const auto initialIds = s.getCompartments().getIds();
  REQUIRE(initialIds.size() == 1);
  s.getCompartments().remove(initialIds[0]);
  REQUIRE(s.getCompartments().getIds().isEmpty());

  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {
      {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}};
  gmshMesh.triangles = {{{0, 1, 2}, 1}, {{1, 3, 2}, 1}};
  const auto image = mesh::voxelizeGMSHMesh(gmshMesh, 24);
  REQUIRE(!image.empty());
  REQUIRE(image.volume().depth() == 1);

  s.getGeometry().importGeometryFromImages(image, false);
  s.getGeometry().setImportedGmshMesh(gmshMesh);
  REQUIRE(s.getGeometry().hasImportedMesh());

  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE_FALSE(s.getGeometry().getIsValid());
  REQUIRE_FALSE(s.getGeometry().getIsMeshValid());
  REQUIRE(s.getGeometry().getMesh2d() == nullptr);

  const auto newCompartmentId = s.getCompartments().add("compartment");
  s.getCompartments().setColor(newCompartmentId,
                               common::indexedColors()[0].rgb());
  REQUIRE(s.getGeometry().getIsValid());
  REQUIRE(s.getGeometry().getIsMeshValid());
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());

  const auto &triangles = s.getGeometry().getMesh2d()->getTriangleIndices();
  std::size_t nTriangles{0};
  for (const auto &compTriangles : triangles) {
    nTriangles += compTriangles.size();
  }
  REQUIRE(nTriangles == gmshMesh.triangles.size());
}

TEST_CASE("fixed 2d mesh is invalid when a compartment is assigned background "
          "color",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  REQUIRE(s.getGeometry().getImages().volume().depth() == 1);

  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                       {1.0, 1.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 1.0, 0.0}};
  gmshMesh.triangles = {{{0, 1, 2}, 1}, {{1, 4, 3}, 2}, {{4, 5, 3}, 2}};
  const auto image = mesh::voxelizeGMSHMesh(gmshMesh, 24, true);
  REQUIRE(!image.empty());
  REQUIRE(image.volume().depth() == 1);

  std::map<QRgb, std::size_t> colorCount;
  for (int y = 0; y < image[0].height(); ++y) {
    for (int x = 0; x < image[0].width(); ++x) {
      ++colorCount[image[0].pixel(x, y)];
    }
  }
  REQUIRE(colorCount.size() >= 3);
  const auto backgroundIt = std::max_element(
      colorCount.cbegin(), colorCount.cend(),
      [](const auto &a, const auto &b) { return a.second < b.second; });
  REQUIRE(backgroundIt != colorCount.cend());
  const auto backgroundColor = backgroundIt->first;
  std::vector<QRgb> meshedColors;
  for (const auto &[color, count] : colorCount) {
    if (color != backgroundColor && count > 0) {
      meshedColors.push_back(color);
    }
  }
  REQUIRE(meshedColors.size() == 2);

  s.getGeometry().importGeometryFromImages(image, false);
  const auto &ids = s.getCompartments().getIds();
  REQUIRE(ids.size() >= 3);
  s.getCompartments().setColor(ids[0], meshedColors[0]);
  s.getCompartments().setColor(ids[1], meshedColors[1]);
  s.getCompartments().setColor(ids[2], backgroundColor);

  s.getGeometry().setImportedGmshMesh(gmshMesh);
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE_FALSE(s.getGeometry().getIsMeshValid());
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE_FALSE(s.getGeometry().getMesh2d()->isValid());
  REQUIRE(s.getGeometry().getMesh2d()->getErrorMessage().find("no triangles") !=
          std::string::npos);
}

TEST_CASE("fixed 3d mesh is invalid when a compartment is assigned background "
          "color",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  REQUIRE(s.getGeometry().getImages().volume().depth() > 1);

  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                       {0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 1.0}};
  gmshMesh.tetrahedra = {{{0, 1, 2, 3}, 1}, {{1, 2, 3, 4}, 2}};
  const auto image = mesh::voxelizeGMSHMesh(gmshMesh, 24, true);
  REQUIRE(!image.empty());
  REQUIRE(image.volume().depth() > 1);

  std::map<QRgb, std::size_t> colorCount;
  for (std::size_t z = 0; z < image.volume().depth(); ++z) {
    for (int y = 0; y < image[0].height(); ++y) {
      for (int x = 0; x < image[0].width(); ++x) {
        ++colorCount[image[z].pixel(x, y)];
      }
    }
  }
  REQUIRE(colorCount.size() >= 3);
  const auto backgroundIt = std::max_element(
      colorCount.cbegin(), colorCount.cend(),
      [](const auto &a, const auto &b) { return a.second < b.second; });
  REQUIRE(backgroundIt != colorCount.cend());
  const auto backgroundColor = backgroundIt->first;
  std::vector<QRgb> meshedColors;
  for (const auto &[color, count] : colorCount) {
    if (color != backgroundColor && count > 0) {
      meshedColors.push_back(color);
    }
  }
  REQUIRE(meshedColors.size() == 2);

  s.getGeometry().importGeometryFromImages(image, false);
  const auto &ids = s.getCompartments().getIds();
  REQUIRE(ids.size() >= 3);
  s.getCompartments().setColor(ids[0], meshedColors[0]);
  s.getCompartments().setColor(ids[1], meshedColors[1]);
  s.getCompartments().setColor(ids[2], backgroundColor);

  s.getGeometry().setImportedGmshMesh(gmshMesh);
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE_FALSE(s.getGeometry().getIsMeshValid());
  REQUIRE(s.getGeometry().getMesh3d() != nullptr);
  REQUIRE_FALSE(s.getGeometry().getMesh3d()->isValid());
  REQUIRE(s.getGeometry().getMesh3d()->getErrorMessage().find(
              "no tetrahedra") != std::string::npos);
}

TEST_CASE("Gmsh fixed mesh import ignores empty topology",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::SingleCompartmentDiffusion)};
  REQUIRE_FALSE(s.getGeometry().hasImportedMesh());

  s.getGeometry().setImportedGmshMesh(mesh::GMSHMesh{});
  REQUIRE_FALSE(s.getGeometry().hasImportedMesh());
}

TEST_CASE("SBML: 2d ParametricGeometry-only import keeps fixed topology",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());

  auto doc = toSbmlDoc(s);
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);
  auto *spatial =
      dynamic_cast<libsbml::SpatialModelPlugin *>(model->getPlugin("spatial"));
  REQUIRE(spatial != nullptr);
  auto *geom = spatial->getGeometry();
  REQUIRE(geom != nullptr);
  for (int i = static_cast<int>(geom->getNumGeometryDefinitions()) - 1; i >= 0;
       --i) {
    auto *def = geom->getGeometryDefinition(static_cast<unsigned int>(i));
    if (def != nullptr && def->isParametricGeometry()) {
      continue;
    }
    auto removed = std::unique_ptr<libsbml::GeometryDefinition>(
        geom->removeGeometryDefinition(static_cast<unsigned int>(i)));
    REQUIRE(removed != nullptr);
  }

  model::Model s2;
  common::unique_C_ptr<char> xmlChar{libsbml::writeSBMLToString(doc.get())};
  s2.importSBMLString(xmlChar.get(), "parametric-only-2d.xml");
  REQUIRE(s2.getIsValid() == true);
  REQUIRE(s2.getGeometry().getHasImage() == true);
  REQUIRE(s2.getGeometry().hasImportedMesh() == true);
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  REQUIRE(s2.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s2.getGeometry().getMesh2d()->isValid());
}

TEST_CASE(
    "SBML: 2d ParametricGeometry-only import preserves annotated voxel source",
    "[core/model/model][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getMeshParameters().meshSourceType =
      model::MeshSourceType::FixedImportedMesh;
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());

  auto doc = toSbmlDoc(s);
  REQUIRE(doc != nullptr);
  auto *sbmlModel = doc->getModel();
  REQUIRE(sbmlModel != nullptr);
  auto settings = sme::model::getSbmlAnnotation(sbmlModel);
  settings.meshParameters.meshSourceType = model::MeshSourceType::VoxelGeometry;
  sme::model::setSbmlAnnotation(sbmlModel, settings);

  auto *spatial = dynamic_cast<libsbml::SpatialModelPlugin *>(
      sbmlModel->getPlugin("spatial"));
  REQUIRE(spatial != nullptr);
  auto *geom = spatial->getGeometry();
  REQUIRE(geom != nullptr);
  for (int i = static_cast<int>(geom->getNumGeometryDefinitions()) - 1; i >= 0;
       --i) {
    auto *def = geom->getGeometryDefinition(static_cast<unsigned int>(i));
    if (def != nullptr && def->isParametricGeometry()) {
      continue;
    }
    auto removed = std::unique_ptr<libsbml::GeometryDefinition>(
        geom->removeGeometryDefinition(static_cast<unsigned int>(i)));
    REQUIRE(removed != nullptr);
  }

  model::Model s2;
  common::unique_C_ptr<char> xmlChar{libsbml::writeSBMLToString(doc.get())};
  s2.importSBMLString(xmlChar.get(), "parametric-only-annotated-voxel-2d.xml");
  REQUIRE(s2.getIsValid() == true);
  REQUIRE(s2.getGeometry().getHasImage() == true);
  REQUIRE(s2.getGeometry().hasImportedMesh() == true);
  REQUIRE(s2.getGeometry().getFixedMeshImportDiagnostic().isEmpty());
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::VoxelGeometry);
  REQUIRE(s2.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s2.getGeometry().getMesh2d()->isValid());
}

TEST_CASE("SBML: parametric-two-compartments import and round-trip",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getTestModel("parametric-two-compartments")};
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getGeometry().getHasImage() == true);
  REQUIRE(s.getGeometry().hasImportedMesh() == true);
  REQUIRE(s.getGeometry().getFixedMeshImportDiagnostic().isEmpty());
  REQUIRE(s.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);

  const auto &ids = s.getCompartments().getIds();
  REQUIRE(ids.size() == 2);
  const auto iEC = ids.indexOf("EC");
  const auto iCell = ids.indexOf("cell");
  REQUIRE(iEC >= 0);
  REQUIRE(iCell >= 0);

  REQUIRE(s.getCompartments().getColor("EC") != 0);
  REQUIRE(s.getCompartments().getColor("cell") != 0);
  REQUIRE(s.getCompartments().getCompartment("EC")->nVoxels() > 0);
  REQUIRE(s.getCompartments().getCompartment("cell")->nVoxels() > 0);

  std::vector<std::size_t> voxelCounts(static_cast<std::size_t>(ids.size()), 0);
  for (int i = 0; i < ids.size(); ++i) {
    voxelCounts[static_cast<std::size_t>(i)] =
        s.getCompartments().getCompartment(ids[i])->nVoxels();
  }

  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh3d() != nullptr);
  REQUIRE_FALSE(s.getGeometry().getMesh3d()->isValid());
  REQUIRE(s.getGeometry().getMesh3d()->getErrorMessage().find(
              "no tetrahedra") != std::string::npos);
  const auto &tetrahedra = s.getGeometry().getMesh3d()->getTetrahedronIndices();
  REQUIRE(tetrahedra.size() == static_cast<std::size_t>(ids.size()));
  REQUIRE(tetrahedra[static_cast<std::size_t>(iCell)].size() > 0);
  REQUIRE(tetrahedra[static_cast<std::size_t>(iEC)].empty());

  std::vector<std::size_t> tetrahedronCounts;
  tetrahedronCounts.reserve(tetrahedra.size());
  for (const auto &compTetrahedra : tetrahedra) {
    tetrahedronCounts.push_back(compTetrahedra.size());
  }

  model::Model s2;
  s2.importSBMLString(s.getXml().toStdString(), "parametric-two-roundtrip.xml");
  REQUIRE(s2.getIsValid() == true);
  REQUIRE(s2.getGeometry().getHasImage() == true);
  REQUIRE(s2.getGeometry().hasImportedMesh() == true);
  REQUIRE(s2.getGeometry().getFixedMeshImportDiagnostic().isEmpty());
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  REQUIRE(s2.getCompartments().getIds() == ids);
  REQUIRE(s2.getGeometry().getImages().volume() ==
          s.getGeometry().getImages().volume());
  for (int i = 0; i < ids.size(); ++i) {
    REQUIRE(s2.getCompartments().getColor(ids[i]) ==
            s.getCompartments().getColor(ids[i]));
    REQUIRE(s2.getCompartments().getCompartment(ids[i])->nVoxels() ==
            voxelCounts[static_cast<std::size_t>(i)]);
  }
  s2.getGeometry().updateMesh();
  REQUIRE(s2.getGeometry().getMesh3d() != nullptr);
  REQUIRE_FALSE(s2.getGeometry().getMesh3d()->isValid());
  REQUIRE(s2.getGeometry().getMesh3d()->getErrorMessage().find(
              "no tetrahedra") != std::string::npos);
  const auto &tetrahedra2 =
      s2.getGeometry().getMesh3d()->getTetrahedronIndices();
  REQUIRE(tetrahedra2.size() == tetrahedronCounts.size());
  for (std::size_t i = 0; i < tetrahedronCounts.size(); ++i) {
    REQUIRE(tetrahedra2[i].size() == tetrahedronCounts[i]);
  }
}

TEST_CASE("SBML: parametric-three-compartments import and round-trip",
          "[core/model/model][core/model][core][model][geometry]") {
  auto s{getTestModel("parametric-three-compartments")};
  REQUIRE(s.getIsValid() == true);
  REQUIRE(s.getGeometry().getHasImage() == true);
  REQUIRE(s.getGeometry().hasImportedMesh() == true);
  REQUIRE(s.getGeometry().getFixedMeshImportDiagnostic().isEmpty());
  REQUIRE(s.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);

  const auto &ids = s.getCompartments().getIds();
  REQUIRE(ids.size() == 3);
  const auto iEC = ids.indexOf("EC");
  const auto iCell = ids.indexOf("cell");
  const auto iNuc = ids.indexOf("nuc");
  REQUIRE(iEC >= 0);
  REQUIRE(iCell >= 0);
  REQUIRE(iNuc >= 0);

  REQUIRE(s.getCompartments().getColor("EC") != 0);
  REQUIRE(s.getCompartments().getColor("cell") != 0);
  REQUIRE(s.getCompartments().getColor("nuc") != 0);
  REQUIRE(s.getCompartments().getCompartment("EC")->nVoxels() > 0);
  REQUIRE(s.getCompartments().getCompartment("cell")->nVoxels() > 0);
  REQUIRE(s.getCompartments().getCompartment("nuc")->nVoxels() > 0);

  std::vector<std::size_t> voxelCounts(static_cast<std::size_t>(ids.size()), 0);
  for (int i = 0; i < ids.size(); ++i) {
    voxelCounts[static_cast<std::size_t>(i)] =
        s.getCompartments().getCompartment(ids[i])->nVoxels();
  }

  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh3d() != nullptr);
  REQUIRE_FALSE(s.getGeometry().getMesh3d()->isValid());
  REQUIRE(s.getGeometry().getMesh3d()->getErrorMessage().find(
              "no tetrahedra") != std::string::npos);
  const auto &tetrahedra = s.getGeometry().getMesh3d()->getTetrahedronIndices();
  REQUIRE(tetrahedra.size() == static_cast<std::size_t>(ids.size()));
  REQUIRE(tetrahedra[static_cast<std::size_t>(iCell)].size() > 0);
  REQUIRE(tetrahedra[static_cast<std::size_t>(iNuc)].size() > 0);
  REQUIRE(tetrahedra[static_cast<std::size_t>(iEC)].empty());

  std::vector<std::size_t> tetrahedronCounts;
  tetrahedronCounts.reserve(tetrahedra.size());
  for (const auto &compTetrahedra : tetrahedra) {
    tetrahedronCounts.push_back(compTetrahedra.size());
  }

  model::Model s2;
  s2.importSBMLString(s.getXml().toStdString(),
                      "parametric-three-roundtrip.xml");
  REQUIRE(s2.getIsValid() == true);
  REQUIRE(s2.getGeometry().getHasImage() == true);
  REQUIRE(s2.getGeometry().hasImportedMesh() == true);
  REQUIRE(s2.getGeometry().getFixedMeshImportDiagnostic().isEmpty());
  REQUIRE(s2.getMeshParameters().meshSourceType ==
          model::MeshSourceType::FixedImportedMesh);
  REQUIRE(s2.getCompartments().getIds() == ids);
  REQUIRE(s2.getGeometry().getImages().volume() ==
          s.getGeometry().getImages().volume());
  for (int i = 0; i < ids.size(); ++i) {
    REQUIRE(s2.getCompartments().getColor(ids[i]) ==
            s.getCompartments().getColor(ids[i]));
    REQUIRE(s2.getCompartments().getCompartment(ids[i])->nVoxels() ==
            voxelCounts[static_cast<std::size_t>(i)]);
  }
  s2.getGeometry().updateMesh();
  REQUIRE(s2.getGeometry().getMesh3d() != nullptr);
  REQUIRE_FALSE(s2.getGeometry().getMesh3d()->isValid());
  REQUIRE(s2.getGeometry().getMesh3d()->getErrorMessage().find(
              "no tetrahedra") != std::string::npos);
  const auto &tetrahedra2 =
      s2.getGeometry().getMesh3d()->getTetrahedronIndices();
  REQUIRE(tetrahedra2.size() == tetrahedronCounts.size());
  for (std::size_t i = 0; i < tetrahedronCounts.size(); ++i) {
    REQUIRE(tetrahedra2[i].size() == tetrahedronCounts[i]);
  }
}

TEST_CASE("SBML: debug export parametric surface triangles to gmsh",
          "[core/model/model][core/model][core][model][geometry][.]") {
  REQUIRE(writeParametricSurfaceTrianglesToGmsh(
      "parametric-two-compartments",
      "parametric-two-compartments-triangles.msh"));
  REQUIRE(writeParametricSurfaceTrianglesToGmsh(
      "parametric-three-compartments",
      "parametric-three-compartments-triangles.msh"));
}
