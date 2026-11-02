#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qmeshrenderer.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include <array>
#include <cmath>
#include <vtkActorCollection.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkTetra.h>

using namespace sme::test;

// set to e.g. 1000 to interactively inspect the rendering
constexpr int delay_ms{0};

namespace {

class TestQMeshRenderer : public QMeshRenderer {
public:
  vtkDataSet *getSolidMesh() {
    auto *actor =
        vtkActor::SafeDownCast(renderer->GetActors()->GetItemAsObject(0));
    actor->GetMapper()->Update();
    return actor->GetMapper()->GetInput();
  }
};

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

} // namespace

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
