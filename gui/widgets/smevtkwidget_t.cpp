#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qmeshrenderer.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include "smevtkwidget.hpp"

using namespace sme::test;

// set to e.g. 1000 to interactively inspect the rendering
constexpr int delay_ms{0};

TEST_CASE("SmeVtkWidget",
          "[smevtkwidget][gui/widgets/smevtkwidget][gui/widgets][gui]") {
  QMeshRenderer meshRenderer;
  meshRenderer.show();
  meshRenderer.resize(200, 200);
  QVoxelRenderer voxelRenderer;
  voxelRenderer.show();
  voxelRenderer.resize(200, 200);
  auto model = sme::test::getExampleModel(Mod::VerySimpleModel3D);
  meshRenderer.setMesh(*model.getGeometry().getMesh3d(), 0);
  voxelRenderer.setImage(model.getGeometry().getImages());
  wait(delay_ms);
  // mouse interactions with two independent SmeVtkWidgets
  sendMouseDrag(&meshRenderer, {100, 100}, {120, 120});
  wait(delay_ms);
  sendMouseDrag(&voxelRenderer, {110, 120}, {80, 90});
  wait(delay_ms);
  meshRenderer.syncCamera(&voxelRenderer);
  // mouse interactions with two synced SmeVtkWidgets
  sendMouseDrag(&meshRenderer, {100, 100}, {150, 150});
  wait(delay_ms);
  sendMouseDrag(&voxelRenderer, {160, 160}, {80, 50});
  wait(delay_ms);
  // change physical size of a voxel
  model.getGeometry().setVoxelSize({1.0, 1.5, 2.0});
  voxelRenderer.setImage(model.getGeometry().getImages());
  meshRenderer.setMesh(*model.getGeometry().getMesh3d(), 0);
  wait(delay_ms);
  sendMouseDrag(&meshRenderer, {100, 100}, {150, 150});
  wait(delay_ms);
  sendMouseDrag(&voxelRenderer, {160, 160}, {80, 50});
  wait(delay_ms);
}
