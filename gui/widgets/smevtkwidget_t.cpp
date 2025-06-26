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
  QSlider slider;
  slider.show();
  QComboBox comboBox;
  comboBox.show();
  auto model = sme::test::getExampleModel(Mod::VerySimpleModel3D);
  meshRenderer.setMesh(*model.getGeometry().getMesh3d(), 0);
  voxelRenderer.setImage(model.getGeometry().getImages());
  wait(delay_ms);
  // mouse interactions with two independent SmeVtkWidgets
  sendMouseDrag(&meshRenderer, {100, 100}, {105, 108});
  wait(delay_ms);
  sendMouseDrag(&voxelRenderer, {110, 120}, {100, 110});
  wait(delay_ms);
  meshRenderer.syncCamera(&voxelRenderer);
  // mouse interactions with two synced camera SmeVtkWidgets
  sendMouseDrag(&meshRenderer, {100, 100}, {110, 106});
  wait(delay_ms);
  sendMouseDrag(&voxelRenderer, {160, 160}, {150, 152});
  wait(delay_ms);
  // change physical size of a voxel
  model.getGeometry().setVoxelSize({1.0, 1.5, 2.0});
  voxelRenderer.setImage(model.getGeometry().getImages());
  meshRenderer.setMesh(*model.getGeometry().getMesh3d(), 0);
  wait(delay_ms);
  sendMouseDrag(&meshRenderer, {100, 100}, {110, 112});
  wait(delay_ms);
  sendMouseDrag(&voxelRenderer, {160, 160}, {150, 151});
  wait(delay_ms);
  // connect a slider and combobox to control the voxelRenderer clipping plane
  voxelRenderer.setClippingPlaneOriginSlider(&slider);
  voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  // sync the clipping plane with the meshRenderer
  meshRenderer.syncClippingPlane(&voxelRenderer);
  voxelRenderer.setImage(model.getGeometry().getImages());
  meshRenderer.setMesh(*model.getGeometry().getMesh3d(), 0);
  QStringList pageUpPageDown{"PgUp",   "PgUp",   "PgUp",   "PgUp",   "PgUp",
                             "PgUp",   "PgUp",   "PgDown", "PgDown", "PgDown",
                             "PgDown", "PgDown", "PgDown", "PgDown"};
  // clip in the z-direction
  sendKeyEvents(&slider, pageUpPageDown);
  wait(delay_ms);
  // clip in the y-direction
  sendKeyEvents(&comboBox, {"Up"});
  sendKeyEvents(&slider, pageUpPageDown);
  wait(delay_ms);
  // clip in the x-direction
  sendKeyEvents(&comboBox, {"Up"});
  sendKeyEvents(&slider, pageUpPageDown);
  wait(delay_ms);
}
