#include "catch_wrapper.hpp"
#include "geometry_parametric.hpp"
#include "model_test_utils.hpp"
#include "sme/mesh2d.hpp"
#include "sme/mesh3d.hpp"
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <set>

using namespace sme;
using namespace sme::test;

namespace {

const libsbml::ParametricGeometry *
getActiveParametricGeometry(const libsbml::Model *model) {
  if (model == nullptr) {
    return nullptr;
  }
  const auto *plugin = dynamic_cast<const libsbml::SpatialModelPlugin *>(
      model->getPlugin("spatial"));
  if (plugin == nullptr) {
    return nullptr;
  }
  const auto *geom = plugin->getGeometry();
  if (geom == nullptr) {
    return nullptr;
  }
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (const auto *def = geom->getGeometryDefinition(i);
        def != nullptr && def->getIsActive() && def->isParametricGeometry()) {
      return dynamic_cast<const libsbml::ParametricGeometry *>(def);
    }
  }
  return nullptr;
}

libsbml::ParametricGeometry *
getActiveParametricGeometry(libsbml::Model *model) {
  return const_cast<libsbml::ParametricGeometry *>(
      getActiveParametricGeometry(const_cast<const libsbml::Model *>(model)));
}

mesh::GMSHMesh makeThreeCompartmentGmshMesh() {
  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                       {0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 1.0}};
  gmshMesh.tetrahedra = {
      {{{0, 1, 2, 3}}, 1}, {{{1, 2, 3, 4}}, 2}, {{{1, 3, 4, 5}}, 3}};
  return gmshMesh;
}

} // namespace

TEST_CASE(
    "Geometry Parametric import supports optional triangle fallback",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();
  REQUIRE(compartmentIds.size() >= 3);
  REQUIRE(compartmentColorsQ.size() >= 3);
  REQUIRE(compartmentColorsQ[0] != 0);
  REQUIRE(compartmentColorsQ[1] != 0);
  REQUIRE(compartmentColorsQ[2] != 0);

  const std::vector<QRgb> compartmentColors(compartmentColorsQ.cbegin(),
                                            compartmentColorsQ.cend());
  const auto gmshMesh = makeThreeCompartmentGmshMesh();
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {compartmentColors[0], 1},
      {compartmentColors[1], 2},
      {compartmentColors[2], 3}};
  mesh::Mesh3d mesh3d(gmshMesh, colorTagPairs, {12, 12, 12}, {1.0, 1.0, 1.0},
                      {0.0, 0.0, 0.0}, compartmentColors, {});
  REQUIRE(mesh3d.isValid());

  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);
  const auto exportResult = model::exportFixedMeshToParametricGeometry(
      model, mesh3d, compartmentIds, compartmentColorsQ);
  REQUIRE(exportResult.exported);

  auto *pg = getActiveParametricGeometry(model);
  REQUIRE(pg != nullptr);
  REQUIRE(pg->getNumParametricObjects() > 0);
  while (pg->getNumParametricObjects() > 1) {
    auto removed = std::unique_ptr<libsbml::ParametricObject>(
        pg->removeParametricObject(1));
    REQUIRE(removed != nullptr);
  }
  auto *obj = pg->getParametricObject(0);
  REQUIRE(obj != nullptr);
  const std::vector<int> singleTriangle{0, 1, 2};
  obj->setPointIndex(singleTriangle);
  obj->setPointIndexLength(static_cast<int>(singleTriangle.size()));

  const auto noFallback = model::importFixedMeshFromParametricGeometry(
      model, compartmentIds, compartmentColorsQ, false);
  REQUIRE_FALSE(noFallback.importedMesh.has_value());
  REQUIRE_FALSE(noFallback.diagnostic.isEmpty());

  const auto withFallback = model::importFixedMeshFromParametricGeometry(
      model, compartmentIds, compartmentColorsQ, true);
  REQUIRE(withFallback.importedMesh.has_value());
  REQUIRE(withFallback.diagnostic.isEmpty());

  const auto &imported = *withFallback.importedMesh;
  REQUIRE(imported.mesh.vertices.size() > 3);
  REQUIRE(imported.mesh.tetrahedra.size() > 0);
  REQUIRE(imported.colorTagPairs.size() > 0);

  for (const auto &tet : imported.mesh.tetrahedra) {
    REQUIRE(tet.physicalTag > 0);
    for (const auto idx : tet.vertexIndices) {
      REQUIRE(idx < imported.mesh.vertices.size());
    }
  }
}

TEST_CASE(
    "Geometry Parametric import rejects invalid tetra face encoding",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getTestModel("parametric-three-compartments")};
  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);

  auto *pg = getActiveParametricGeometry(model);
  REQUIRE(pg != nullptr);
  REQUIRE(pg->getNumParametricObjects() > 0);

  auto *obj = pg->getParametricObject(0);
  REQUIRE(obj != nullptr);

  const auto compartmentIds = s.getCompartments().getIds();
  REQUIRE(!compartmentIds.isEmpty());
  const auto firstCompartmentId = compartmentIds[0].toStdString();
  const auto *compartment = model->getCompartment(firstCompartmentId);
  REQUIRE(compartment != nullptr);
  const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
      compartment->getPlugin("spatial"));
  REQUIRE(scp != nullptr);
  REQUIRE(scp->isSetCompartmentMapping());
  obj->setDomainType(scp->getCompartmentMapping()->getDomainType());

  const std::vector<int> duplicateFaceEncoding{0, 1, 2, 0, 1, 2,
                                               0, 1, 3, 0, 2, 3};
  obj->setPointIndex(duplicateFaceEncoding);
  obj->setPointIndexLength(static_cast<int>(duplicateFaceEncoding.size()));

  const auto result = model::importFixedMeshFromParametricGeometry(
      model, compartmentIds, s.getCompartments().getColors(), false);
  REQUIRE_FALSE(result.importedMesh.has_value());
  REQUIRE_FALSE(result.diagnostic.isEmpty());
}

TEST_CASE(
    "Geometry Parametric export and import round-trip tetrahedra",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();
  REQUIRE(compartmentIds.size() >= 3);
  REQUIRE(compartmentColorsQ.size() >= 3);
  REQUIRE(compartmentColorsQ[0] != 0);
  REQUIRE(compartmentColorsQ[1] != 0);
  REQUIRE(compartmentColorsQ[2] != 0);

  const std::vector<QRgb> compartmentColors(compartmentColorsQ.cbegin(),
                                            compartmentColorsQ.cend());

  const auto gmshMesh = makeThreeCompartmentGmshMesh();

  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {compartmentColors[0], 1},
      {compartmentColors[1], 2},
      {compartmentColors[2], 3}};
  mesh::Mesh3d mesh3d(gmshMesh, colorTagPairs, {12, 12, 12}, {1.0, 1.0, 1.0},
                      {0.0, 0.0, 0.0}, compartmentColors, {});
  REQUIRE(mesh3d.isValid());

  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);

  const auto exportResult = model::exportFixedMeshToParametricGeometry(
      model, mesh3d, compartmentIds, compartmentColorsQ);
  REQUIRE(exportResult.exported);
  REQUIRE(exportResult.diagnostic.isEmpty());

  const auto *pg = getActiveParametricGeometry(model);
  REQUIRE(pg != nullptr);
  REQUIRE(pg->isSetSpatialPoints());
  REQUIRE(pg->getSpatialPoints()->getActualArrayDataLength() == 18);
  REQUIRE(pg->getNumParametricObjects() == 3);
  for (unsigned i = 0; i < pg->getNumParametricObjects(); ++i) {
    const auto *obj = pg->getParametricObject(i);
    REQUIRE(obj != nullptr);
    REQUIRE(obj->getPointIndexLength() == 12);
  }

  const auto importResult = model::importFixedMeshFromParametricGeometry(
      model, compartmentIds, compartmentColorsQ, false);
  REQUIRE(importResult.importedMesh.has_value());
  REQUIRE(importResult.diagnostic.isEmpty());
  REQUIRE(importResult.importedMesh->mesh.vertices.size() == 6);
  REQUIRE(importResult.importedMesh->mesh.tetrahedra.size() == 3);
  REQUIRE(importResult.importedMesh->colorTagPairs.size() == 3);

  std::set<int> physicalTags;
  for (const auto &tet : importResult.importedMesh->mesh.tetrahedra) {
    physicalTags.insert(tet.physicalTag);
  }
  REQUIRE(physicalTags == std::set<int>{1, 2, 3});
}

TEST_CASE(
    "Geometry Parametric export and import round-trip triangles",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();

  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);

  const auto exportResult = model::exportFixedMeshToParametricGeometry(
      model, *s.getGeometry().getMesh2d(), compartmentIds, compartmentColorsQ);
  REQUIRE(exportResult.exported);
  REQUIRE(exportResult.diagnostic.isEmpty());

  const auto *pg = getActiveParametricGeometry(model);
  REQUIRE(pg != nullptr);
  REQUIRE(pg->isSetSpatialPoints());
  REQUIRE(pg->getSpatialPoints()->getActualArrayDataLength() > 0);
  const auto *spatial = dynamic_cast<const libsbml::SpatialModelPlugin *>(
      model->getPlugin("spatial"));
  REQUIRE(spatial != nullptr);
  const auto *geom = spatial->getGeometry();
  REQUIRE(geom != nullptr);
  const auto nCoords = geom->getNumCoordinateComponents();
  REQUIRE((nCoords == 2 || nCoords == 3));
  REQUIRE(pg->getSpatialPoints()->getActualArrayDataLength() %
              static_cast<int>(nCoords) ==
          0);
  REQUIRE(pg->getNumParametricObjects() > 0);

  const auto importResult = model::importFixedMeshFromParametricGeometry(
      model, compartmentIds, compartmentColorsQ, false);
  REQUIRE(importResult.importedMesh2d.has_value());
  REQUIRE(importResult.diagnostic.isEmpty());

  const auto &triangles = s.getGeometry().getMesh2d()->getTriangleIndices();
  const auto &importedTriangles = importResult.importedMesh2d->triangleIndices;
  REQUIRE(importedTriangles.size() == triangles.size());
  for (std::size_t i = 0; i < triangles.size(); ++i) {
    REQUIRE(importedTriangles[i].size() == triangles[i].size());
  }
}

TEST_CASE(
    "Geometry Parametric export fails for non-3D geometry",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();
  REQUIRE(compartmentIds.size() >= 3);
  REQUIRE(compartmentColorsQ.size() >= 3);
  REQUIRE(compartmentColorsQ[0] != 0);
  REQUIRE(compartmentColorsQ[1] != 0);
  REQUIRE(compartmentColorsQ[2] != 0);

  const std::vector<QRgb> compartmentColors(compartmentColorsQ.cbegin(),
                                            compartmentColorsQ.cend());
  const auto gmshMesh = makeThreeCompartmentGmshMesh();
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {compartmentColors[0], 1},
      {compartmentColors[1], 2},
      {compartmentColors[2], 3}};
  mesh::Mesh3d mesh3d(gmshMesh, colorTagPairs, {8, 8, 8}, {1.0, 1.0, 1.0},
                      {0.0, 0.0, 0.0}, compartmentColors, {});
  REQUIRE(mesh3d.isValid());

  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);
  auto *spatial =
      dynamic_cast<libsbml::SpatialModelPlugin *>(model->getPlugin("spatial"));
  REQUIRE(spatial != nullptr);
  auto *geom = spatial->getGeometry();
  REQUIRE(geom != nullptr);
  while (geom->getNumCoordinateComponents() > 2) {
    auto removed = std::unique_ptr<libsbml::CoordinateComponent>(
        geom->removeCoordinateComponent(geom->getNumCoordinateComponents() -
                                        1));
    REQUIRE(removed != nullptr);
  }
  REQUIRE(geom->getNumCoordinateComponents() == 2);

  const auto exportResult = model::exportFixedMeshToParametricGeometry(
      model, mesh3d, compartmentIds, compartmentColorsQ);
  REQUIRE_FALSE(exportResult.exported);
  REQUIRE(exportResult.diagnostic.contains("expected 3 coordinate components"));
}

TEST_CASE(
    "Geometry Parametric export fails when 3d domain mapping is missing",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();
  REQUIRE(compartmentIds.size() >= 3);
  REQUIRE(compartmentColorsQ.size() >= 3);

  const auto gmshMesh = makeThreeCompartmentGmshMesh();
  const std::vector<QRgb> compartmentColors(compartmentColorsQ.cbegin(),
                                            compartmentColorsQ.cend());
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {compartmentColors[0], 1},
      {compartmentColors[1], 2},
      {compartmentColors[2], 3}};
  mesh::Mesh3d mesh3d(gmshMesh, colorTagPairs, {12, 12, 12}, {1.0, 1.0, 1.0},
                      {0.0, 0.0, 0.0}, compartmentColors, {});
  REQUIRE(mesh3d.isValid());

  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);
  const auto missingCompartmentId = compartmentIds[1].toStdString();
  auto *compartment = model->getCompartment(missingCompartmentId);
  REQUIRE(compartment != nullptr);
  auto *scp = static_cast<libsbml::SpatialCompartmentPlugin *>(
      compartment->getPlugin("spatial"));
  REQUIRE(scp != nullptr);
  REQUIRE(scp->isSetCompartmentMapping());
  scp->unsetCompartmentMapping();

  const auto exportResult = model::exportFixedMeshToParametricGeometry(
      model, mesh3d, compartmentIds, compartmentColorsQ);
  REQUIRE_FALSE(exportResult.exported);
  REQUIRE(exportResult.diagnostic.contains("missing domainType mapping for "
                                           "compartment") == true);
}

TEST_CASE(
    "Geometry Parametric export fails when 2d domain mapping is missing",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();
  const auto &triangles = s.getGeometry().getMesh2d()->getTriangleIndices();
  REQUIRE(compartmentIds.size() == static_cast<int>(triangles.size()));

  int iExportable{-1};
  for (int i = 0; i < static_cast<int>(triangles.size()); ++i) {
    if (!triangles[static_cast<std::size_t>(i)].empty() &&
        i < compartmentColorsQ.size() && compartmentColorsQ[i] != 0) {
      iExportable = i;
      break;
    }
  }
  REQUIRE(iExportable >= 0);

  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);
  const auto missingCompartmentId =
      compartmentIds[static_cast<int>(iExportable)].toStdString();
  auto *compartment = model->getCompartment(missingCompartmentId);
  REQUIRE(compartment != nullptr);
  auto *scp = static_cast<libsbml::SpatialCompartmentPlugin *>(
      compartment->getPlugin("spatial"));
  REQUIRE(scp != nullptr);
  REQUIRE(scp->isSetCompartmentMapping());
  scp->unsetCompartmentMapping();

  const auto exportResult = model::exportFixedMeshToParametricGeometry(
      model, *s.getGeometry().getMesh2d(), compartmentIds, compartmentColorsQ);
  REQUIRE_FALSE(exportResult.exported);
  REQUIRE(exportResult.diagnostic.contains("missing domainType mapping for "
                                           "compartment") == true);
}

TEST_CASE(
    "Geometry Parametric import reports malformed spatial points",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getTestModel("parametric-three-compartments")};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColors = s.getCompartments().getColors();

  SECTION("missing spatialPoints arrayData") {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    pg->unsetSpatialPoints();

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("missing spatialPoints arrayData") ==
            true);
  }

  SECTION("invalid spatialPoints length for coordinate dimension") {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    auto *points = pg->getSpatialPoints();
    REQUIRE(points != nullptr);
    const std::vector<double> invalidPoints{0.0, 0.0, 0.0, 1.0};
    points->setArrayData(invalidPoints);
    points->setArrayDataLength(static_cast<int>(invalidPoints.size()));

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("spatialPoints length") == true);
  }

  SECTION("unsupported number of coordinate components") {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *spatial = dynamic_cast<libsbml::SpatialModelPlugin *>(
        model->getPlugin("spatial"));
    REQUIRE(spatial != nullptr);
    auto *geom = spatial->getGeometry();
    REQUIRE(geom != nullptr);
    while (geom->getNumCoordinateComponents() > 1) {
      auto removed = std::unique_ptr<libsbml::CoordinateComponent>(
          geom->removeCoordinateComponent(geom->getNumCoordinateComponents() -
                                          1));
      REQUIRE(removed != nullptr);
    }
    REQUIRE(geom->getNumCoordinateComponents() == 1);

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("Unsupported ParametricGeometry "
                                       "dimensions: 1") == true);
  }
}

TEST_CASE(
    "Geometry Parametric import reports object mapping diagnostics",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getTestModel("parametric-three-compartments")};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColors = s.getCompartments().getColors();
  REQUIRE(compartmentIds.size() >= 2);
  REQUIRE(compartmentColors.size() >= 2);

  SECTION("no mapped ParametricObject domain types") {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    REQUIRE(pg->getNumParametricObjects() > 0);
    for (unsigned i = 0; i < pg->getNumParametricObjects(); ++i) {
      auto *obj = pg->getParametricObject(i);
      REQUIRE(obj != nullptr);
      obj->setDomainType("__unmapped_domain_type__");
    }

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("No ParametricObject entries matched "
                                       "compartment domainType mappings") ==
            true);
  }

  SECTION("unsupported polygon type") {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    REQUIRE(pg->getNumParametricObjects() > 0);
    auto *obj = pg->getParametricObject(0);
    REQUIRE(obj != nullptr);
    const auto *compartment =
        model->getCompartment(compartmentIds[0].toStdString());
    REQUIRE(compartment != nullptr);
    const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
        compartment->getPlugin("spatial"));
    REQUIRE(scp != nullptr);
    REQUIRE(scp->isSetCompartmentMapping());
    obj->setDomainType(scp->getCompartmentMapping()->getDomainType());
    obj->setPolygonType(static_cast<libsbml::PolygonKind_t>(999));

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("unsupported polygonType") == true);
  }

  SECTION("mapped object points to out-of-range compartment index") {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    REQUIRE(pg->getNumParametricObjects() > 0);

    const auto lastCompartmentId = compartmentIds.back().toStdString();
    const auto *compartment = model->getCompartment(lastCompartmentId);
    REQUIRE(compartment != nullptr);
    const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
        compartment->getPlugin("spatial"));
    REQUIRE(scp != nullptr);
    REQUIRE(scp->isSetCompartmentMapping());
    const auto domainType = scp->getCompartmentMapping()->getDomainType();

    for (unsigned i = 0; i < pg->getNumParametricObjects(); ++i) {
      auto *obj = pg->getParametricObject(i);
      REQUIRE(obj != nullptr);
      obj->setDomainType(domainType);
    }

    QVector<QRgb> fewerColors = compartmentColors;
    fewerColors.resize(compartmentColors.size() - 1);
    REQUIRE(fewerColors.size() < compartmentColors.size());

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, fewerColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("out-of-range compartment index") ==
            true);
  }

  SECTION("mapped object with empty point index") {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    REQUIRE(pg->getNumParametricObjects() > 0);
    auto *obj = pg->getParametricObject(0);
    REQUIRE(obj != nullptr);
    const auto *compartment =
        model->getCompartment(compartmentIds[0].toStdString());
    REQUIRE(compartment != nullptr);
    const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
        compartment->getPlugin("spatial"));
    REQUIRE(scp != nullptr);
    REQUIRE(scp->isSetCompartmentMapping());
    obj->setDomainType(scp->getCompartmentMapping()->getDomainType());
    const std::vector<int> emptyPointIndex{};
    obj->setPointIndex(emptyPointIndex);
    obj->setPointIndexLength(0);

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("unsupported pointIndex length 0") ==
            true);
  }
}

TEST_CASE(
    "Geometry Parametric import reports invalid 2d triangles",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColors = s.getCompartments().getColors();

  const auto make2dParametricModel = [&]() {
    auto doc{toSbmlDoc(s)};
    REQUIRE(doc != nullptr);
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    const auto exportResult = model::exportFixedMeshToParametricGeometry(
        model, *s.getGeometry().getMesh2d(), compartmentIds, compartmentColors);
    REQUIRE(exportResult.exported);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    REQUIRE(pg->getNumParametricObjects() > 0);
    return doc;
  };

  SECTION("pointIndex length not divisible by 3") {
    auto doc = make2dParametricModel();
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    auto *obj = pg->getParametricObject(0);
    REQUIRE(obj != nullptr);
    const std::vector<int> badIndexLength{0, 1};
    obj->setPointIndex(badIndexLength);
    obj->setPointIndexLength(static_cast<int>(badIndexLength.size()));

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("expected 3*n entries") == true);
  }

  SECTION("out-of-range triangle vertex index") {
    auto doc = make2dParametricModel();
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    auto *obj = pg->getParametricObject(0);
    REQUIRE(obj != nullptr);
    const std::vector<int> outOfRangeIndex{0, 1, 999999};
    obj->setPointIndex(outOfRangeIndex);
    obj->setPointIndexLength(static_cast<int>(outOfRangeIndex.size()));

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("out-of-range spatialPoints index") ==
            true);
  }

  SECTION("degenerate triangle") {
    auto doc = make2dParametricModel();
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    auto *pg = getActiveParametricGeometry(model);
    REQUIRE(pg != nullptr);
    auto *obj = pg->getParametricObject(0);
    REQUIRE(obj != nullptr);
    const std::vector<int> degenerateTriangle{0, 0, 1};
    obj->setPointIndex(degenerateTriangle);
    obj->setPointIndexLength(static_cast<int>(degenerateTriangle.size()));

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, compartmentColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("degenerate triangles") == true);
  }

  SECTION("no valid triangles after skipping hidden compartments") {
    auto doc = make2dParametricModel();
    auto *model = doc->getModel();
    REQUIRE(model != nullptr);
    QVector<QRgb> hiddenColors = compartmentColors;
    for (auto &color : hiddenColors) {
      color = 0;
    }

    const auto result = model::importFixedMeshFromParametricGeometry(
        model, compartmentIds, hiddenColors, false);
    REQUIRE_FALSE(result.importedMesh.has_value());
    REQUIRE_FALSE(result.importedMesh2d.has_value());
    REQUIRE(result.diagnostic.contains("no valid triangles") == true);
  }
}

TEST_CASE(
    "Geometry Parametric export 2d fails for unsupported dimensions",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getGeometry().updateMesh();
  REQUIRE(s.getGeometry().getMesh2d() != nullptr);
  REQUIRE(s.getGeometry().getMesh2d()->isValid());

  auto doc{toSbmlDoc(s)};
  REQUIRE(doc != nullptr);
  auto *model = doc->getModel();
  REQUIRE(model != nullptr);
  auto *spatial =
      dynamic_cast<libsbml::SpatialModelPlugin *>(model->getPlugin("spatial"));
  REQUIRE(spatial != nullptr);
  auto *geom = spatial->getGeometry();
  REQUIRE(geom != nullptr);
  while (geom->getNumCoordinateComponents() > 1) {
    auto removed = std::unique_ptr<libsbml::CoordinateComponent>(
        geom->removeCoordinateComponent(geom->getNumCoordinateComponents() -
                                        1));
    REQUIRE(removed != nullptr);
  }
  REQUIRE(geom->getNumCoordinateComponents() == 1);

  const auto exportResult = model::exportFixedMeshToParametricGeometry(
      model, *s.getGeometry().getMesh2d(), s.getCompartments().getIds(),
      s.getCompartments().getColors());
  REQUIRE_FALSE(exportResult.exported);
  REQUIRE(exportResult.diagnostic.contains(
      "expected 2 or 3 coordinate components"));
}
