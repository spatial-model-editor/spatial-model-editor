#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qmeshrenderer.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include "smevtkwidget.hpp"
#include <array>
#include <memory>
#include <vtkCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>

using namespace sme::test;

// set to e.g. 1000 to interactively inspect the rendering
constexpr int delay_ms{0};

namespace {

struct RenderCounter {
  int renders{0};
  std::array<double, 3> cameraPosition{};
  void record(vtkObject *caller, unsigned long, void *) {
    ++renders;
    auto *window = vtkRenderWindow::SafeDownCast(caller);
    window->GetRenderers()->GetFirstRenderer()->GetActiveCamera()->GetPosition(
        cameraPosition.data());
  }
};

class TestSmeVtkWidget : public SmeVtkWidget {
public:
  vtkCamera *getCamera() { return renderer->GetActiveCamera(); }
  vtkRenderWindowInteractor *getInteractor() {
    return renderWindow->GetInteractor();
  }
  void countRenders(RenderCounter &counter) {
    renderWindow->AddObserver(vtkCommand::EndEvent, &counter,
                              &RenderCounter::record);
  }
};

void showWidget(TestSmeVtkWidget &widget, int x) {
  widget.resize(200, 200);
  widget.move(x, 0);
  widget.show();
  wait(0);
}

} // namespace

TEST_CASE("SmeVtkWidget linked views render camera changes once",
          "[smevtkwidget][gui/widgets/smevtkwidget][gui/widgets][gui]") {
  RenderCounter firstRenders;
  RenderCounter secondRenders;
  TestSmeVtkWidget first;
  TestSmeVtkWidget second;
  showWidget(first, 0);
  showWidget(second, 250);
  first.syncCamera(&second);
  wait(0);
  REQUIRE(first.getCamera() == second.getCamera());
  first.countRenders(firstRenders);
  second.countRenders(secondRenders);

  SECTION("mouse state changes do not render the linked peer") {
    first.getInteractor()->SetEventInformation(100, 100);
    second.getInteractor()->SetEventInformation(110, 120);
    REQUIRE(firstRenders.renders == 0);
    REQUIRE(secondRenders.renders == 0);
    // Mouse handling may redraw the source widget's orientation marker, but
    // must not render the peer when the shared camera has not changed.
    first.getInteractor()->MouseMoveEvent();
    wait(0);
    REQUIRE(secondRenders.renders == 0);
    firstRenders.renders = 0;
    second.getInteractor()->MouseMoveEvent();
    wait(0);
    REQUIRE(firstRenders.renders == 0);
  }
  SECTION("a burst of programmatic camera changes renders the final state") {
    auto *camera = first.getCamera();
    camera->Azimuth(15);
    camera->Elevation(10);
    camera->Dolly(1.5);
    std::array<double, 3> finalPosition{};
    camera->GetPosition(finalPosition.data());
    REQUIRE(firstRenders.renders == 0);
    REQUIRE(secondRenders.renders == 0);
    wait(0);
    REQUIRE(firstRenders.renders == 1);
    REQUIRE(secondRenders.renders == 1);
    REQUIRE(firstRenders.cameraPosition == finalPosition);
    REQUIRE(secondRenders.cameraPosition == finalPosition);
    wait(0);
    REQUIRE(firstRenders.renders == 1);
    REQUIRE(secondRenders.renders == 1);
  }
  SECTION("an explicit render cancels that view's pending camera render") {
    first.getCamera()->Azimuth(15);
    first.vtkRender();
    REQUIRE(firstRenders.renders == 1);
    wait(0);
    REQUIRE(firstRenders.renders == 1);
    REQUIRE(secondRenders.renders == 1);
  }
  SECTION("hidden views refresh when shown") {
    second.hide();
    wait(0);
    firstRenders.renders = 0;
    secondRenders.renders = 0;
    first.getCamera()->Azimuth(15);
    std::array<double, 3> finalPosition{};
    first.getCamera()->GetPosition(finalPosition.data());
    wait(0);
    REQUIRE(firstRenders.renders == 1);
    REQUIRE(secondRenders.renders == 0);
    second.show();
    wait(0);
    REQUIRE(secondRenders.renders >= 1);
    REQUIRE(secondRenders.cameraPosition == finalPosition);
  }
  SECTION("actual interaction updates the peer to the final camera position") {
    sendMouseDrag(&first, {100, 100}, {110, 106});
    wait(0);
    REQUIRE(firstRenders.renders > 0);
    REQUIRE(secondRenders.renders > 0);
    std::array<double, 3> finalPosition{};
    first.getCamera()->GetPosition(finalPosition.data());
    REQUIRE(secondRenders.cameraPosition == finalPosition);
    firstRenders.renders = 0;
    secondRenders.renders = 0;
    sendMouseWheel(&second, 1);
    wait(0);
    REQUIRE(firstRenders.renders > 0);
    REQUIRE(secondRenders.renders > 0);
    second.getCamera()->GetPosition(finalPosition.data());
    REQUIRE(firstRenders.cameraPosition == finalPosition);
  }
}

TEST_CASE("SmeVtkWidget defers explicit renders until shown",
          "[smevtkwidget][gui/widgets/smevtkwidget][gui/widgets][gui]") {
  RenderCounter renders;
  QWidget parent;
  TestSmeVtkWidget widget;
  widget.resize(200, 200);
  SECTION("before the widget has first been shown") {}
  SECTION("after the widget has been hidden") {
    showWidget(widget, 0);
    widget.hide();
    wait(0);
  }
  SECTION("when an ancestor is hidden") {
    widget.setParent(&parent);
    widget.show();
  }
  widget.countRenders(renders);
  for (int i = 0; i < 3; ++i) {
    widget.getCamera()->Azimuth(5);
    widget.vtkRender();
    wait(0);
    REQUIRE(renders.renders == 0);
  }
  std::array<double, 3> position{};
  widget.getCamera()->GetPosition(position.data());
  if (widget.parentWidget() != nullptr) {
    parent.show();
  } else {
    widget.show();
  }
  wait(0);
  REQUIRE(renders.renders == 1);
  REQUIRE(renders.cameraPosition == position);
  wait(0);
  REQUIRE(renders.renders == 1);
}

TEST_CASE("SmeVtkWidget camera observers follow resynchronization",
          "[smevtkwidget][gui/widgets/smevtkwidget][gui/widgets][gui]") {
  RenderCounter firstRenders;
  RenderCounter secondRenders;
  RenderCounter thirdRenders;
  TestSmeVtkWidget first;
  TestSmeVtkWidget second;
  TestSmeVtkWidget third;
  showWidget(first, 0);
  showWidget(second, 250);
  showWidget(third, 500);
  first.syncCamera(&second);
  wait(0);
  // Replace an earlier link, then repeat the same link to check idempotence.
  first.syncCamera(&third);
  first.syncCamera(&third);
  wait(0);
  first.countRenders(firstRenders);
  second.countRenders(secondRenders);
  third.countRenders(thirdRenders);
  second.getCamera()->Azimuth(15);
  wait(0);
  REQUIRE(firstRenders.renders == 0);
  REQUIRE(secondRenders.renders == 1);
  REQUIRE(thirdRenders.renders == 0);
  secondRenders.renders = 0;
  third.getCamera()->Azimuth(15);
  wait(0);
  REQUIRE(firstRenders.renders == 1);
  REQUIRE(secondRenders.renders == 0);
  REQUIRE(thirdRenders.renders == 1);
}

TEST_CASE("SmeVtkWidget can be destroyed with a shared camera render pending",
          "[smevtkwidget][gui/widgets/smevtkwidget][gui/widgets][gui]") {
  RenderCounter renders;
  TestSmeVtkWidget remaining;
  showWidget(remaining, 0);
  auto other = std::make_unique<TestSmeVtkWidget>();
  showWidget(*other, 250);
  remaining.syncCamera(other.get());
  wait(0);
  remaining.countRenders(renders);
  remaining.getCamera()->Azimuth(15);
  other.reset();
  wait(0);
  REQUIRE(renders.renders == 1);
  renders.renders = 0;
  remaining.getCamera()->Azimuth(15);
  wait(0);
  REQUIRE(renders.renders == 1);
}

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
