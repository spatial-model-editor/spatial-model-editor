#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qmeshrenderer.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <vtkActorCollection.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkIdList.h>
#include <vtkMapper.h>
#include <vtkMath.h>
#include <vtkTetra.h>
#include <vtkTriangle.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWeakPointer.h>

using namespace sme::test;

// set to e.g. 1000 to interactively inspect the rendering
constexpr int delay_ms{0};

namespace {

class TestQMeshRenderer : public QMeshRenderer {
  vtkActor *getActor(std::size_t index) {
    return vtkActor::SafeDownCast(
        renderer->GetActors()->GetItemAsObject(index));
  }

  vtkDataSet *getActorMesh(std::size_t index) {
    auto *actor = getActor(index);
    actor->GetMapper()->Update();
    return actor->GetMapper()->GetInput();
  }

public:
  bool isSolidVisible(std::size_t part) {
    return getActor(2 * part)->GetVisibility() != 0;
  }
  int getNumberOfActors() { return renderer->GetActors()->GetNumberOfItems(); }
  vtkDataSet *getSolidMesh(std::size_t part = 0) {
    return getActorMesh(2 * part);
  }
  vtkDataSet *getWireframeMesh(std::size_t part) {
    return getActorMesh(2 * part + 1);
  }
};

template <std::size_t CellSize>
void checkCells(vtkDataSet *data,
                const std::vector<std::array<std::size_t, CellSize>> &expected,
                int cellType) {
  REQUIRE(data->GetNumberOfCells() == static_cast<vtkIdType>(expected.size()));
  vtkNew<vtkIdList> ids;
  for (vtkIdType i = 0; i < data->GetNumberOfCells(); ++i) {
    CAPTURE(i);
    REQUIRE(data->GetCellType(i) == cellType);
    data->GetCellPoints(i, ids);
    REQUIRE(ids->GetNumberOfIds() == CellSize);
    for (vtkIdType j = 0; j < static_cast<vtkIdType>(CellSize); ++j) {
      REQUIRE(ids->GetId(j) == static_cast<vtkIdType>(expected[i][j]));
    }
  }
}

double meshVolume(vtkDataSet *mesh) {
  vtkNew<vtkDataSetTriangleFilter> tetrahedra;
  tetrahedra->SetInputData(mesh);
  tetrahedra->Update();
  auto *grid = tetrahedra->GetOutput();
  double volume{0.0};
  for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i) {
    auto *points = grid->GetCell(i)->GetPoints();
    std::array<std::array<double, 3>, 4> vertices{};
    for (vtkIdType j = 0; j < 4; ++j) {
      points->GetPoint(j, vertices[j].data());
    }
    volume += std::abs(
        vtkTetra::ComputeVolume(vertices[0].data(), vertices[1].data(),
                                vertices[2].data(), vertices[3].data()));
  }
  return volume;
}

double membraneArea(vtkDataSet *mesh) {
  double area{0.0};
  for (vtkIdType i = 0; i < mesh->GetNumberOfCells(); ++i) {
    auto *cell = mesh->GetCell(i);
    REQUIRE(cell->GetCellDimension() == 2);
    auto *points = cell->GetPoints();
    std::array<std::array<double, 3>, 3> vertices{};
    points->GetPoint(0, vertices[0].data());
    // Clipping a triangle can leave a convex quadrilateral.
    for (vtkIdType j = 1; j + 1 < points->GetNumberOfPoints(); ++j) {
      points->GetPoint(j, vertices[1].data());
      points->GetPoint(j + 1, vertices[2].data());
      area += vtkTriangle::TriangleArea(vertices[0].data(), vertices[1].data(),
                                        vertices[2].data());
    }
  }
  return area;
}

double wireframeLength(vtkDataSet *mesh, vtkPlane *plane) {
  double length{0.0};
  for (vtkIdType i = 0; i < mesh->GetNumberOfCells(); ++i) {
    auto *cell = mesh->GetCell(i);
    REQUIRE(cell->GetCellType() == VTK_LINE);
    auto *points = cell->GetPoints();
    std::array<double, 3> start{};
    std::array<double, 3> end{};
    points->GetPoint(0, start.data());
    points->GetPoint(1, end.data());
    REQUIRE(plane->EvaluateFunction(start.data()) >= -1e-6);
    REQUIRE(plane->EvaluateFunction(end.data()) >= -1e-6);
    length +=
        std::sqrt(vtkMath::Distance2BetweenPoints(start.data(), end.data()));
  }
  return length;
}

} // namespace

TEST_CASE("QMeshRenderer preserves compartment and membrane connectivity",
          "[qmeshrenderer][gui/widgets/qmeshrenderer][gui/widgets][gui]") {
  sme::mesh::GMSHMesh gmshMesh{
      {{0.0, 0.0, 0.0},
       {1.0, 0.0, 0.0},
       {0.0, 1.0, 0.0},
       {0.0, 0.0, 1.0},
       {1.0, 1.0, 1.0},
       {2.0, 0.0, 0.0}},
      {{{2, 0, 1, 3}, 7}, {{3, 1, 2, 4}, 7}, {{2, 4, 1, 5}, 11}},
      {}};
  std::size_t membraneCells{1};
  SECTION("compartments share a membrane") {}
  SECTION("disconnected compartments have an empty membrane") {
    gmshMesh.vertices.insert(
        gmshMesh.vertices.end(),
        {{3.0, 0.0, 0.0}, {4.0, 0.0, 0.0}, {3.0, 1.0, 0.0}, {3.0, 0.0, 1.0}});
    gmshMesh.tetrahedra.back().vertexIndices = {6, 7, 8, 9};
    membraneCells = 0;
  }
  const auto red = qRgb(255, 0, 0);
  const auto green = qRgb(0, 255, 0);
  const sme::mesh::Mesh3d mesh(gmshMesh, {{red, 7}, {green, 11}}, {6, 4, 3},
                               {0.5, 2.0, 3.0}, {5.0, 6.0, 7.0}, {red, green},
                               {{"membrane", {red, green}}});
  REQUIRE(mesh.isValid());
  REQUIRE(mesh.getTetrahedronIndices()[0].size() == 2);
  REQUIRE(mesh.getTetrahedronIndices()[1].size() == 1);
  REQUIRE(mesh.getMembraneTriangleIndices()[0].size() == membraneCells);
  TestQMeshRenderer renderer;
  QVoxelRenderer voxelRenderer;
  renderer.resize(200, 200);
  renderer.show();

  // Rebuilding and clearing must not leave dangling references to the arrays
  // created locally while assembling each compartment and membrane.
  for (int rebuild = 0; rebuild < 2; ++rebuild) {
    CAPTURE(rebuild);
    renderer.setMesh(mesh, 0);
    for (std::size_t i = 0; i < mesh.getNumberOfCompartments(); ++i) {
      checkCells(renderer.getSolidMesh(i), mesh.getTetrahedronIndices()[i],
                 VTK_TETRA);
    }
    renderer.setMembraneIndex(0);
    checkCells(renderer.getSolidMesh(mesh.getNumberOfCompartments()),
               mesh.getMembraneTriangleIndices()[0], VTK_TRIANGLE);
    renderer.setRenderMode(QMeshRenderer::RenderMode::Wireframe);
    REQUIRE(renderer.getWireframeMesh(0)->GetNumberOfCells() == 9);
    REQUIRE(renderer.getWireframeMesh(1)->GetNumberOfCells() == 6);
    REQUIRE(renderer.getWireframeMesh(2)->GetNumberOfCells() ==
            static_cast<vtkIdType>(3 * membraneCells));
    renderer.clear();
    REQUIRE(renderer.getNumberOfActors() == 0);
  }

  // The connected pipeline owns its data and filters after setMesh returns.
  // Rebuild with a different compartment count, then restore both compartments
  // and their membrane to check selection bookkeeping as well as lifetime.
  auto singleCompartmentMesh = gmshMesh;
  singleCompartmentMesh.tetrahedra.resize(2);
  const sme::mesh::Mesh3d singleMesh(singleCompartmentMesh, {{red, 7}},
                                     {6, 4, 3}, {0.5, 2.0, 3.0},
                                     {5.0, 6.0, 7.0}, {red});
  REQUIRE(singleMesh.isValid());
  renderer.syncClippingPlane(&voxelRenderer);
  for (int rebuild = 0; rebuild < 2; ++rebuild) {
    renderer.setMesh(singleMesh, 0);
    REQUIRE(renderer.getNumberOfActors() == 2);
    REQUIRE(renderer.isSolidVisible(0));
    REQUIRE(renderer.getSolidMesh()->GetNumberOfCells() == 2);

    renderer.setMesh(mesh, 1);
    REQUIRE(renderer.getNumberOfActors() == 6);
    REQUIRE(renderer.isSolidVisible(0));
    REQUIRE(renderer.isSolidVisible(1));
    REQUIRE_FALSE(renderer.isSolidVisible(2));
    renderer.setMembraneIndex(0);
    REQUIRE_FALSE(renderer.isSolidVisible(0));
    REQUIRE_FALSE(renderer.isSolidVisible(1));
    REQUIRE(renderer.isSolidVisible(2));
    REQUIRE(renderer.getSolidMesh(2)->GetNumberOfCells() ==
            static_cast<vtkIdType>(membraneCells));
    renderer.setRenderMode(QMeshRenderer::RenderMode::Wireframe);
    REQUIRE(renderer.getWireframeMesh(2)->GetNumberOfCells() ==
            static_cast<vtkIdType>(3 * membraneCells));
    renderer.clear();
    REQUIRE(renderer.getNumberOfActors() == 0);
  }
}

TEST_CASE("QMeshRenderer clips membrane areas and wireframe lengths",
          "[qmeshrenderer][gui/widgets/qmeshrenderer][gui/widgets][gui]") {
  // Two tetrahedra share a right-triangle membrane in the xy plane. Splitting
  // halfway along either leg leaves one quarter of the membrane's area.
  const sme::mesh::GMSHMesh gmshMesh{{{0.0, 0.0, 0.0},
                                      {1.0, 0.0, 0.0},
                                      {0.0, 1.0, 0.0},
                                      {0.0, 0.0, 1.0},
                                      {0.0, 0.0, -1.0}},
                                     {{{0, 1, 2, 3}, 1}, {{0, 2, 1, 4}, 2}},
                                     {}};
  const auto red = qRgb(255, 0, 0);
  const auto green = qRgb(0, 255, 0);
  const sme::mesh::Mesh3d mesh(gmshMesh, {{red, 1}, {green, 2}}, {8, 12, 16},
                               {1.0, 1.0, 1.0}, {5.0, 6.0, 7.0}, {red, green},
                               {{"membrane", {red, green}}});
  REQUIRE(mesh.isValid());
  REQUIRE(mesh.getMembraneTriangleIndices()[0].size() == 1);
  QVoxelRenderer voxelRenderer;
  TestQMeshRenderer renderer;
  renderer.resize(200, 200);
  renderer.show();
  renderer.syncClippingPlane(&voxelRenderer);
  auto *plane = voxelRenderer.getClippingPlane();
  const std::array<double, 3> origin{5.0, 6.0, 7.0};
  const std::array<double, 3> lengths{8.0, 12.0, 8.0};
  const double fullArea = lengths[0] * lengths[1] / 2.0;
  const double diagonal = std::hypot(lengths[0], lengths[1]);

  for (int axis = 0; axis < 2; ++axis) {
    CAPTURE(axis);
    std::array<double, 3> normal{};
    normal[axis] = 1.0;
    plane->SetNormal(normal.data());
    plane->SetOrigin(origin.data());
    renderer.setMesh(mesh, 2);
    REQUIRE(membraneArea(renderer.getSolidMesh(2)) == dbl_approx(fullArea));
    renderer.setRenderMode(QMeshRenderer::RenderMode::Wireframe);
    REQUIRE(wireframeLength(renderer.getWireframeMesh(2), plane) ==
            dbl_approx(lengths[0] + lengths[1] + diagonal));

    auto cut = origin;
    cut[axis] += lengths[axis] / 2.0;
    plane->SetOrigin(cut.data());
    renderer.setRenderMode(QMeshRenderer::RenderMode::Solid);
    REQUIRE(membraneArea(renderer.getSolidMesh(2)) ==
            dbl_approx(fullArea / 4.0));
    renderer.setRenderMode(QMeshRenderer::RenderMode::Wireframe);
    const double membraneCutLength = (lengths[axis] + diagonal) / 2.0;
    REQUIRE(wireframeLength(renderer.getWireframeMesh(2), plane) ==
            dbl_approx(membraneCutLength));
    const double tetrahedronCutLength =
        membraneCutLength + std::hypot(lengths[axis], lengths[2]) / 2.0;
    for (std::size_t part = 0; part < 2; ++part) {
      REQUIRE(wireframeLength(renderer.getWireframeMesh(part), plane) ==
              dbl_approx(tetrahedronCutLength));
    }

    // Reverse the shared plane without rebuilding the pipelines. Wireframes
    // retain only original mesh edges, without introducing a new cut edge.
    normal[axis] = -1.0;
    plane->SetNormal(normal.data());
    renderer.setRenderMode(QMeshRenderer::RenderMode::Solid);
    REQUIRE(membraneArea(renderer.getSolidMesh(2)) ==
            dbl_approx(3.0 * fullArea / 4.0));
    renderer.setRenderMode(QMeshRenderer::RenderMode::Wireframe);
    REQUIRE(wireframeLength(renderer.getWireframeMesh(2), plane) ==
            dbl_approx(membraneCutLength + lengths[1 - axis]));
    for (std::size_t part = 0; part < 2; ++part) {
      REQUIRE(wireframeLength(renderer.getWireframeMesh(part), plane) ==
              dbl_approx(tetrahedronCutLength + lengths[1 - axis] + lengths[2] +
                         std::hypot(lengths[1 - axis], lengths[2])));
    }

    normal[axis] = 1.0;
    plane->SetNormal(normal.data());
    cut[axis] = origin[axis] + lengths[axis] + 1.0;
    plane->SetOrigin(cut.data());
    renderer.vtkRender();
    REQUIRE(renderer.getSolidMesh(2)->GetNumberOfCells() == 0);
    for (std::size_t part = 0; part < 3; ++part) {
      REQUIRE(renderer.getWireframeMesh(part)->GetNumberOfCells() == 0);
    }
  }
}

TEST_CASE(
    "QMeshRenderer retains the clipping plane after its source is destroyed",
    "[qmeshrenderer][gui/widgets/qmeshrenderer][gui/widgets][gui]") {
  const sme::mesh::GMSHMesh gmshMesh{
      {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},
      {{{0, 1, 2, 3}, 1}},
      {}};
  const auto color = qRgb(255, 0, 0);
  const sme::mesh::Mesh3d mesh(gmshMesh, {{color, 1}}, {8, 12, 16},
                               {1.0, 1.0, 1.0}, {5.0, 6.0, 7.0}, {color});
  REQUIRE(mesh.isValid());
  constexpr double fullVolume = 8.0 * 12.0 * 16.0 / 6.0;
  auto voxelRenderer = std::make_unique<QVoxelRenderer>();
  auto meshRenderer = std::make_unique<TestQMeshRenderer>();
  meshRenderer->resize(200, 200);
  meshRenderer->show();
  meshRenderer->syncClippingPlane(voxelRenderer.get());
  // Observe the plane without keeping it alive on behalf of the renderer.
  vtkWeakPointer<vtkPlane> plane = voxelRenderer->getClippingPlane();
  plane->SetNormal(1.0, 0.0, 0.0);
  plane->SetOrigin(9.0, 6.0, 7.0);
  SECTION("source is destroyed before the first mesh is built") {}
  SECTION("source is destroyed after a clipped mesh is built") {
    meshRenderer->setMesh(mesh, 0);
    REQUIRE(meshVolume(meshRenderer->getSolidMesh()) ==
            dbl_approx(fullVolume / 8.0));
  }
  voxelRenderer.reset();
  meshRenderer->clear();
  REQUIRE(plane != nullptr);
  meshRenderer->setMesh(mesh, 0);
  REQUIRE(meshVolume(meshRenderer->getSolidMesh()) ==
          dbl_approx(fullVolume / 8.0));
  plane->SetOrigin(4.0, 6.0, 7.0);
  meshRenderer->vtkRender();
  REQUIRE(meshVolume(meshRenderer->getSolidMesh()) == dbl_approx(fullVolume));
  meshRenderer.reset();
  REQUIRE(plane == nullptr);
}

TEST_CASE("QMeshRenderer clips tetrahedra with a shared plane",
          "[qmeshrenderer][gui/widgets/qmeshrenderer][gui/widgets][gui]") {
  // A single tetrahedron with nonzero origin and different lengths along each
  // axis makes the retained volume and the exposed cut faces easy to check.
  const sme::mesh::GMSHMesh gmshMesh{
      {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},
      {{{0, 1, 2, 3}, 1}},
      {}};
  const auto color = qRgb(255, 0, 0);
  const sme::mesh::Mesh3d mesh(gmshMesh, {{color, 1}}, {8, 12, 16},
                               {1.0, 1.0, 1.0}, {5.0, 6.0, 7.0}, {color});
  REQUIRE(mesh.isValid());
  const std::array<double, 3> origin{5.0, 6.0, 7.0};
  const std::array<double, 3> lengths{8.0, 12.0, 16.0};
  constexpr double fullVolume = 8.0 * 12.0 * 16.0 / 6.0;
  QVoxelRenderer voxelRenderer;
  TestQMeshRenderer meshRenderer;
  meshRenderer.show();
  meshRenderer.resize(200, 200);
  meshRenderer.syncClippingPlane(&voxelRenderer);
  auto *plane = voxelRenderer.getClippingPlane();

  for (int axis = 0; axis < 3; ++axis) {
    CAPTURE(axis);
    std::array<double, 3> normal{};
    normal[axis] = 1.0;
    plane->SetNormal(normal.data());
    auto planeOrigin = origin;
    planeOrigin[axis] -= 1.0;
    plane->SetOrigin(planeOrigin.data());
    meshRenderer.setMesh(mesh, 0);
    REQUIRE(meshVolume(meshRenderer.getSolidMesh()) == dbl_approx(fullVolume));

    // Move the existing plane through the mesh without rebuilding the pipeline.
    planeOrigin[axis] = origin[axis] + 0.5 * lengths[axis];
    plane->SetOrigin(planeOrigin.data());
    meshRenderer.vtkRender();
    auto *clipped = meshRenderer.getSolidMesh();
    REQUIRE(meshVolume(clipped) == dbl_approx(fullVolume / 8.0));
    bool hasCutFace{false};
    for (vtkIdType i = 0; i < clipped->GetNumberOfCells(); ++i) {
      auto *cell = clipped->GetCell(i);
      for (int j = 0; j < cell->GetNumberOfFaces(); ++j) {
        auto *face = cell->GetFace(j);
        bool onPlane{true};
        for (vtkIdType k = 0; k < face->GetNumberOfPoints(); ++k) {
          onPlane &= face->GetPoints()->GetPoint(k)[axis] == planeOrigin[axis];
        }
        hasCutFace |= onPlane;
      }
    }
    REQUIRE(hasCutFace);

    // The other side retains the rest of the volume, including non-tetrahedral
    // cells produced by the table-based clipper.
    normal[axis] = -1.0;
    plane->SetNormal(normal.data());
    meshRenderer.vtkRender();
    REQUIRE(meshVolume(meshRenderer.getSolidMesh()) ==
            dbl_approx(7.0 * fullVolume / 8.0));

    normal[axis] = 1.0;
    plane->SetNormal(normal.data());
    planeOrigin[axis] = origin[axis] + lengths[axis] + 1.0;
    plane->SetOrigin(planeOrigin.data());
    meshRenderer.vtkRender();
    REQUIRE(meshRenderer.getSolidMesh()->GetNumberOfCells() == 0);

    // A plane on the boundary must retain the complete tetrahedron.
    plane->SetOrigin(origin.data());
    meshRenderer.vtkRender();
    REQUIRE(meshVolume(meshRenderer.getSolidMesh()) == dbl_approx(fullVolume));
  }
}

TEST_CASE("QMeshRenderer",
          "[qmeshrenderer][gui/widgets/qmeshrenderer][gui/widgets][gui]") {
  QMeshRenderer meshRenderer;
  std::vector<int> mouseClicks;
  QObject::connect(&meshRenderer, &QMeshRenderer::mouseClicked,
                   [&mouseClicks](int i) { mouseClicks.push_back(i); });
  meshRenderer.show();
  meshRenderer.resize(200, 200);
  auto model = sme::test::getExampleModel(Mod::VerySimpleModel3D);
  auto &mesh = *model.getGeometry().getMesh3d();
  meshRenderer.setMesh(mesh, 0);
  // zoom in
  sendMouseWheel(&meshRenderer, 1);
  sendMouseWheel(&meshRenderer, 1);
  // highlight each compartment in turn
  for (std::size_t i = 0; i < mesh.getNumberOfCompartments(); ++i) {
    meshRenderer.setCompartmentIndex(i);
    wait(delay_ms);
  }
  // highlight each membrane in turn
  for (std::size_t i = 0; i < mesh.getNumberOfMembranes(); ++i) {
    meshRenderer.setMembraneIndex(i);
    wait(delay_ms);
  }
  // change the compartment colors
  meshRenderer.setColors({0xff00ff, 0x00ff00, 0x0000ff, 0xffff00, 0x00ffff});
  wait(delay_ms);
  meshRenderer.setCompartmentIndex(2);
  wait(delay_ms);
  // change the render mode
  meshRenderer.setRenderMode(QMeshRenderer::RenderMode::Wireframe);
  wait(delay_ms);
  meshRenderer.setRenderMode(QMeshRenderer::RenderMode::Solid);
  wait(delay_ms);
  // click outside a compartment
  REQUIRE(mouseClicks.size() == 0);
  sendMouseClick(&meshRenderer, {1, 1});
  REQUIRE(mouseClicks.size() == 0);
  // click on outer compartment with index 0
  sendMouseClick(&meshRenderer, {100, 100});
  REQUIRE(mouseClicks.size() == 1);
  REQUIRE(mouseClicks[0] == 0);
  // new mesh with reset camera
  meshRenderer.setMesh(mesh, 0);
  wait(delay_ms);
}
