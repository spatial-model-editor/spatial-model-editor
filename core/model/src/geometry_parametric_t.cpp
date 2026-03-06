#include "catch_wrapper.hpp"
#include "geometry_parametric.hpp"
#include "model_test_utils.hpp"
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

} // namespace

TEST_CASE(
    "Geometry Parametric import supports optional triangle fallback",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();
  REQUIRE(compartmentIds.size() >= 2);
  REQUIRE(compartmentColorsQ.size() >= 2);
  REQUIRE(compartmentColorsQ[0] != 0);
  REQUIRE(compartmentColorsQ[1] != 0);

  const std::vector<QRgb> compartmentColors(compartmentColorsQ.cbegin(),
                                            compartmentColorsQ.cend());
  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {{0.0, 0.0, 0.0},
                       {1.0, 0.0, 0.0},
                       {0.0, 1.0, 0.0},
                       {0.0, 0.0, 1.0},
                       {1.0, 1.0, 1.0}};
  gmshMesh.tetrahedra = {{{0, 1, 2, 3}, 1}, {{1, 2, 3, 4}, 2}};
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {compartmentColors[0], 1}, {compartmentColors[1], 2}};
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
  REQUIRE(compartmentIds.size() >= 2);
  REQUIRE(compartmentColorsQ.size() >= 2);
  REQUIRE(compartmentColorsQ[0] != 0);
  REQUIRE(compartmentColorsQ[1] != 0);

  const std::vector<QRgb> compartmentColors(compartmentColorsQ.cbegin(),
                                            compartmentColorsQ.cend());

  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {{0.0, 0.0, 0.0},
                       {1.0, 0.0, 0.0},
                       {0.0, 1.0, 0.0},
                       {0.0, 0.0, 1.0},
                       {1.0, 1.0, 1.0}};
  gmshMesh.tetrahedra = {{{0, 1, 2, 3}, 1}, {{1, 2, 3, 4}, 2}};

  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {compartmentColors[0], 1}, {compartmentColors[1], 2}};
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
  REQUIRE(pg->getSpatialPoints()->getActualArrayDataLength() == 15);
  REQUIRE(pg->getNumParametricObjects() == 2);
  for (unsigned i = 0; i < pg->getNumParametricObjects(); ++i) {
    const auto *obj = pg->getParametricObject(i);
    REQUIRE(obj != nullptr);
    REQUIRE(obj->getPointIndexLength() == 12);
  }

  const auto importResult = model::importFixedMeshFromParametricGeometry(
      model, compartmentIds, compartmentColorsQ, false);
  REQUIRE(importResult.importedMesh.has_value());
  REQUIRE(importResult.diagnostic.isEmpty());
  REQUIRE(importResult.importedMesh->mesh.vertices.size() == 5);
  REQUIRE(importResult.importedMesh->mesh.tetrahedra.size() == 2);
  REQUIRE(importResult.importedMesh->colorTagPairs.size() == 2);

  std::set<int> physicalTags;
  for (const auto &tet : importResult.importedMesh->mesh.tetrahedra) {
    physicalTags.insert(tet.physicalTag);
  }
  REQUIRE(physicalTags == std::set<int>{1, 2});
}

TEST_CASE(
    "Geometry Parametric export fails for non-3D geometry",
    "[core/model/geometry_parametric][core/model][core][model][geometry]") {
  auto s{getExampleModel(Mod::VerySimpleModel3D)};
  const auto compartmentIds = s.getCompartments().getIds();
  const auto compartmentColorsQ = s.getCompartments().getColors();
  REQUIRE(compartmentIds.size() >= 1);
  REQUIRE(compartmentColorsQ.size() >= 1);
  REQUIRE(compartmentColorsQ[0] != 0);

  const std::vector<QRgb> compartmentColors(compartmentColorsQ.cbegin(),
                                            compartmentColorsQ.cend());
  mesh::GMSHMesh gmshMesh;
  gmshMesh.vertices = {
      {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
  gmshMesh.tetrahedra = {{{0, 1, 2, 3}, 1}};
  const std::vector<std::pair<QRgb, int>> colorTagPairs{
      {compartmentColors[0], 1}};
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
