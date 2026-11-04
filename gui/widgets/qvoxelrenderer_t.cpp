#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/image_stack.hpp"
#include <QComboBox>
#include <QSlider>
#include <array>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkVolumeCollection.h>

using namespace sme::test;

// set to e.g. 1000 to interactively inspect the rendering
constexpr int delay_ms{0};

namespace {

struct EventCounter {
  int events{0};
  void increment() { ++events; }
};

class TestQVoxelRenderer : public QVoxelRenderer {
public:
  vtkRenderer *getRenderer() { return renderer; }
  vtkImageData *getImageData() {
    auto *vol =
        vtkVolume::SafeDownCast(renderer->GetVolumes()->GetItemAsObject(0));
    return vtkImageData::SafeDownCast(vol->GetMapper()->GetDataSetInput());
  }
};

QPoint screenPosition(TestQVoxelRenderer &widget,
                      const std::array<double, 3> &world) {
  auto *renderer = widget.getRenderer();
  renderer->SetWorldPoint(world[0], world[1], world[2], 1.0);
  renderer->WorldToDisplay();
  const auto *display = renderer->GetDisplayPoint();
  const auto scale = widget.effectiveDevicePixelRatio();
  const auto height = renderer->GetRenderWindow()->GetSize()[1];
  return {qRound(display[0] / scale),
          qRound((height - 1 - display[1]) / scale)};
}

void lookAlongZ(TestQVoxelRenderer &widget, bool fromAbove = false) {
  std::array<double, 3> center{};
  widget.getImageData()->GetCenter(center.data());
  auto *camera = widget.getRenderer()->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetFocalPoint(center.data());
  camera->SetPosition(center[0], center[1],
                      center[2] + (fromAbove ? 1.0 : -1.0));
  camera->SetViewUp(0.0, 1.0, 0.0);
  widget.getRenderer()->ResetCamera();
  widget.vtkRender();
  wait(0);
}

sme::common::ImageStack makePickingImage(std::size_t depth) {
  sme::common::ImageStack image({9, 7, depth}, QImage::Format_RGB32);
  for (std::size_t z = 0; z < depth; ++z) {
    for (int y = 0; y < image.volume().height(); ++y) {
      for (int x = 0; x < image.volume().width(); ++x) {
        image[z].setPixel(x, y, qRgb(10 + 20 * x, 15 + 25 * y, 20 + 30 * z));
      }
    }
  }
  return image;
}

} // namespace

TEST_CASE("QVoxelRenderer picks source voxels in physical coordinates",
          "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  std::vector<std::pair<QRgb, sme::common::Voxel>> clicks;
  QSlider slider;
  QComboBox comboBox;
  TestQVoxelRenderer voxelRenderer;
  voxelRenderer.setClippingPlaneOriginSlider(&slider);
  voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  auto image = makePickingImage(5);
  sme::common::VoxelF origin{};
  SECTION("unit spacing and zero origin") {}
  SECTION("anisotropic spacing") { image.setVoxelSize({0.5, 2.0, 3.0}); }
  SECTION("positive physical origin") { origin = {100.0, 200.0, 300.0}; }
  SECTION("negative physical origin and anisotropic spacing") {
    image.setVoxelSize({0.5, 2.0, 3.0});
    origin = {-20.0, -30.0, -40.0};
  }
  SECTION("device pixels differ from widget coordinates") {
    voxelRenderer.setCustomDevicePixelRatio(2.0);
    image.setVoxelSize({0.5, 2.0, 3.0});
    origin = {-20.0, -30.0, -40.0};
  }
  voxelRenderer.resize(400, 300);
  voxelRenderer.show();
  voxelRenderer.setImage(image, origin);
  wait(0);
  lookAlongZ(voxelRenderer);
  QObject::connect(&voxelRenderer, &QVoxelRenderer::mouseClicked,
                   [&clicks](QRgb color, sme::common::Voxel voxel) {
                     clicks.emplace_back(color, voxel);
                   });
  const auto clickVoxel = [&](const sme::common::Voxel &voxel) {
    const auto &spacing = image.voxelSize();
    const std::array<double, 3> world{
        origin.p.x() + spacing.width() * voxel.p.x(),
        origin.p.y() +
            spacing.height() * (image.volume().height() - 1 - voxel.p.y()),
        origin.z + spacing.depth() * static_cast<double>(voxel.z)};
    const auto pos = screenPosition(voxelRenderer, world);
    const auto previousClicks = clicks.size();
    sendMouseMove(&voxelRenderer, pos);
    sendMouseClick(&voxelRenderer, pos);
    REQUIRE(clicks.size() == previousClicks + 1);
    REQUIRE(clicks.back().first == image[voxel.z].pixel(voxel.p));
    REQUIRE(clicks.back().second == voxel);
  };
  clickVoxel({2, 1, 0});
  clickVoxel({5, 4, 0});
  // Reusing the scalar buffer must not leave picking with an earlier color.
  image[0].setPixel(2, 1, qRgb(220, 40, 80));
  voxelRenderer.setImage(image, origin);
  clickVoxel({2, 1, 0});
  const auto previousClicks = clicks.size();
  sendMouseMove(&voxelRenderer, {1, 1});
  sendMouseClick(&voxelRenderer, {1, 1});
  REQUIRE(clicks.size() == previousClicks);

  // Looking along +z now exposes slice 2 at the clipping plane.
  auto *plane = voxelRenderer.getClippingPlane();
  plane->SetOrigin(origin.p.x(), origin.p.y(),
                   origin.z + 2.0 * image.voxelSize().depth());
  voxelRenderer.vtkRender();
  clickVoxel({2, 1, 2});
  plane->SetOrigin(origin.p.x(), origin.p.y(),
                   origin.z + 5.0 * image.voxelSize().depth());
  voxelRenderer.vtkRender();
  const auto clippedClicks = clicks.size();
  sendMouseClick(&voxelRenderer, {200, 150});
  REQUIRE(clicks.size() == clippedClicks);
  voxelRenderer.setImage({});
  sendMouseClick(&voxelRenderer, {200, 150});
  REQUIRE(clicks.size() == clippedClicks);
}

TEST_CASE(
    "QVoxelRenderer maps the duplicated display slice to the source slice",
    "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  std::vector<std::pair<QRgb, sme::common::Voxel>> clicks;
  QSlider slider;
  QComboBox comboBox;
  TestQVoxelRenderer voxelRenderer;
  voxelRenderer.setClippingPlaneOriginSlider(&slider);
  voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  auto image = makePickingImage(1);
  image.setVoxelSize({0.5, 2.0, 3.0});
  const sme::common::VoxelF origin{-20.0, -30.0, -40.0};
  voxelRenderer.resize(400, 300);
  voxelRenderer.show();
  voxelRenderer.setImage(image, origin);
  wait(0);
  QObject::connect(&voxelRenderer, &QVoxelRenderer::mouseClicked,
                   [&clicks](QRgb color, sme::common::Voxel voxel) {
                     clicks.emplace_back(color, voxel);
                   });
  for (bool fromAbove : {false, true}) {
    CAPTURE(fromAbove);
    lookAlongZ(voxelRenderer, fromAbove);
    const std::array<double, 3> world{
        origin.p.x() + 2.0 * image.voxelSize().width(),
        origin.p.y() + 5.0 * image.voxelSize().height(), origin.z};
    const auto pos = screenPosition(voxelRenderer, world);
    sendMouseMove(&voxelRenderer, pos);
    sendMouseClick(&voxelRenderer, pos);
    REQUIRE(clicks.size() == (fromAbove ? 2 : 1));
    REQUIRE(clicks.back().first == image[0].pixel(2, 1));
    REQUIRE(clicks.back().second == sme::common::Voxel{2, 1, 0});
  }
}

TEST_CASE("QVoxelRenderer modifies the clipping plane once per slider change",
          "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  EventCounter planeChanges;
  QSlider slider;
  QComboBox comboBox;
  QVoxelRenderer voxelRenderer;
  voxelRenderer.setClippingPlaneOriginSlider(&slider);
  voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  sme::common::ImageStack image({4, 3, 2}, QImage::Format_RGB32);
  image.fill(qRgb(50, 50, 50));
  voxelRenderer.setImage(image);
  voxelRenderer.resize(200, 200);
  voxelRenderer.show();
  wait(0);
  voxelRenderer.getClippingPlane()->AddObserver(
      vtkCommand::ModifiedEvent, &planeChanges, &EventCounter::increment);
  slider.setValue(35);
  REQUIRE(planeChanges.events == 1);
}

TEST_CASE(
    "QVoxelRenderer renders image changes without relying on clipping controls",
    "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  EventCounter renders;
  QSlider slider;
  QComboBox comboBox;
  TestQVoxelRenderer voxelRenderer;
  SECTION("no clipping controls") {}
  SECTION("only a clipping slider") {
    voxelRenderer.setClippingPlaneOriginSlider(&slider);
  }
  SECTION("only a clipping direction combobox") {
    voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  }
  SECTION("both clipping controls") {
    voxelRenderer.setClippingPlaneOriginSlider(&slider);
    voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  }
  voxelRenderer.resize(200, 200);
  voxelRenderer.show();
  wait(delay_ms);
  voxelRenderer.getRenderer()->GetRenderWindow()->AddObserver(
      vtkCommand::EndEvent, &renders, &EventCounter::increment);
  sme::common::ImageStack image({4, 3, 2}, QImage::Format_RGB32);
  image.fill(qRgb(50, 50, 50));
  sme::common::VoxelF origin{1.0, 2.0, 3.0};
  const auto checkRendered = [&]() {
    renders.events = 0;
    voxelRenderer.setImage(image, origin);
    // Check immediately: receiving a Qt paint event or interacting with the
    // widget must not be necessary to refresh its VTK rendering.
    REQUIRE(renders.events > 0);
  };
  checkRendered();
  image.fill(qRgb(120, 60, 30));
  checkRendered();
  image.rescaleXY({6, 4});
  checkRendered();
  image.setVoxelSize({0.5, 2.0, 3.0});
  checkRendered();
  origin = {5.0, 6.0, 7.0};
  checkRendered();
  image.clear();
  checkRendered();
  REQUIRE(voxelRenderer.getImageData()->GetNumberOfPoints() == 0);
}

TEST_CASE("QVoxelRenderer updates values without resetting the view",
          "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  EventCounter cameraResets;
  EventCounter renders;
  QSlider slider;
  QComboBox comboBox;
  TestQVoxelRenderer voxelRenderer;
  voxelRenderer.setClippingPlaneOriginSlider(&slider);
  voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  voxelRenderer.show();
  voxelRenderer.resize(200, 200);
  sme::common::ImageStack image({4, 3, 2}, QImage::Format_RGB32);
  image.setVoxelSize({0.5, 2.0, 3.0});
  image.fill(qRgb(50, 50, 50));
  const sme::common::VoxelF origin{5.0, 6.0, 7.0};
  voxelRenderer.setImage(image, origin);
  wait(delay_ms);
  auto *renderer = voxelRenderer.getRenderer();
  renderer->AddObserver(vtkCommand::ResetCameraEvent, &cameraResets,
                        &EventCounter::increment);
  renderer->GetRenderWindow()->AddObserver(vtkCommand::EndEvent, &renders,
                                           &EventCounter::increment);
  auto *camera = renderer->GetActiveCamera();
  camera->Azimuth(15);
  camera->Elevation(10);
  camera->Dolly(1.5);
  std::array<double, 3> cameraPosition{};
  std::array<double, 3> cameraFocalPoint{};
  std::array<double, 3> cameraViewUp{};
  camera->GetPosition(cameraPosition.data());
  camera->GetFocalPoint(cameraFocalPoint.data());
  camera->GetViewUp(cameraViewUp.data());
  comboBox.setCurrentText("x");
  slider.setValue(35);
  auto *plane = voxelRenderer.getClippingPlane();
  std::array<double, 3> planeOrigin{};
  plane->GetOrigin(planeOrigin.data());
  const auto planeMTime = plane->GetMTime();
  auto *data = voxelRenderer.getImageData();
  // Keep the original array alive so replacing it cannot accidentally reuse
  // the same address and make the allocation-reuse check pass.
  vtkSmartPointer<vtkDataArray> scalars = data->GetPointData()->GetScalars();
  auto *storage = scalars->GetVoidPointer(0);
  std::array<double, 2> range{};
  scalars->GetRange(range.data(), 0);
  REQUIRE(range == std::array<double, 2>{50.0, 50.0});

  for (bool indexed : {false, true}) {
    CAPTURE(indexed);
    auto frame = image;
    const auto red = indexed ? 180 : 120;
    frame.fill(qRgb(red, 60, 30));
    frame[0].setPixel(0, 0, qRgb(21, 42, 63));
    if (indexed) {
      frame.convertToIndexed();
    }
    renders.events = 0;
    voxelRenderer.setImage(frame, origin);

    REQUIRE(cameraResets.events == 0);
    REQUIRE(renders.events > 0);
    for (int axis = 0; axis < 3; ++axis) {
      REQUIRE(camera->GetPosition()[axis] == cameraPosition[axis]);
      REQUIRE(camera->GetFocalPoint()[axis] == cameraFocalPoint[axis]);
      REQUIRE(camera->GetViewUp()[axis] == cameraViewUp[axis]);
      REQUIRE(plane->GetOrigin()[axis] == planeOrigin[axis]);
    }
    REQUIRE(comboBox.currentText() == "x");
    REQUIRE(slider.value() == 35);
    REQUIRE(plane->GetMTime() == planeMTime);
    REQUIRE(data->GetPointData()->GetScalars() == scalars.GetPointer());
    REQUIRE(scalars->GetVoidPointer(0) == storage);
    // Cached scalar ranges must be invalidated along with the GPU texture.
    scalars->GetRange(range.data(), 0);
    REQUIRE(range == std::array<double, 2>{21.0, static_cast<double>(red)});
    // Values retain the existing reversed-y layout and RGB-derived fourth
    // component, including when the source image uses an indexed palette.
    for (int component = 0; component < 4; ++component) {
      REQUIRE(data->GetScalarComponentAsDouble(0, 2, 0, component) ==
              std::array<double, 4>{21.0, 42.0, 63.0, 42.0}[component]);
    }
    REQUIRE(data->GetScalarComponentAsDouble(0, 0, 0, 0) == red);
  }
}

TEST_CASE("QVoxelRenderer resets the view when geometry changes",
          "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  EventCounter cameraResets;
  QSlider slider;
  QComboBox comboBox;
  TestQVoxelRenderer voxelRenderer;
  voxelRenderer.setClippingPlaneOriginSlider(&slider);
  voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  voxelRenderer.show();
  voxelRenderer.resize(200, 200);
  sme::common::ImageStack image({4, 3, 1}, QImage::Format_RGB32);
  image.setVoxelSize({0.5, 2.0, 3.0});
  image.fill(qRgb(50, 50, 50));
  sme::common::VoxelF origin{5.0, 6.0, 7.0};
  voxelRenderer.setImage(image, origin);
  wait(delay_ms);
  voxelRenderer.getRenderer()->AddObserver(
      vtkCommand::ResetCameraEvent, &cameraResets, &EventCounter::increment);
  comboBox.setCurrentText("x");
  slider.setValue(35);

  SECTION("different dimensions with the same voxel count") {
    image = sme::common::ImageStack({3, 4, 1}, QImage::Format_RGB32);
    image.setVoxelSize({0.5, 2.0, 3.0});
    image.fill(qRgb(80, 80, 80));
  }
  SECTION("two slices replace one duplicated slice") {
    image = sme::common::ImageStack({4, 3, 2}, QImage::Format_RGB32);
    image.setVoxelSize({0.5, 2.0, 3.0});
    image.fill(qRgb(80, 80, 80));
  }
  SECTION("different voxel spacing") { image.setVoxelSize({1.5, 2.0, 3.0}); }
  SECTION("different physical origin") { origin = {15.0, -4.0, 2.0}; }

  voxelRenderer.setImage(image, origin);
  REQUIRE(cameraResets.events == 1);
  REQUIRE(comboBox.currentText() == "z");
  REQUIRE(slider.value() == 0);
  auto *plane = voxelRenderer.getClippingPlane();
  REQUIRE(plane->GetNormal()[0] == 0.0);
  REQUIRE(plane->GetNormal()[1] == 0.0);
  REQUIRE(plane->GetNormal()[2] == 1.0);
  REQUIRE(plane->GetOrigin()[2] ==
          dbl_approx(origin.z - image.voxelSize().depth()));
  auto *data = voxelRenderer.getImageData();
  REQUIRE(data->GetDimensions()[0] == image.volume().width());
  REQUIRE(data->GetDimensions()[1] == image.volume().height());
  REQUIRE(data->GetDimensions()[2] == 2);
  REQUIRE(data->GetSpacing()[0] == image.voxelSize().width());
  REQUIRE(data->GetOrigin()[0] == origin.p.x());
  REQUIRE(data->GetOrigin()[1] == origin.p.y());
  REQUIRE(data->GetOrigin()[2] == origin.z);
}

TEST_CASE("QVoxelRenderer initializes clipping controls added after an image",
          "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  EventCounter cameraResets;
  QSlider slider;
  QComboBox comboBox;
  TestQVoxelRenderer voxelRenderer;
  sme::common::ImageStack image({4, 3, 2}, QImage::Format_RGB32);
  image.setVoxelSize({0.5, 2.0, 3.0});
  image.fill(qRgb(50, 50, 50));
  const sme::common::VoxelF origin{5.0, 6.0, 7.0};
  voxelRenderer.setImage(image, origin);
  voxelRenderer.show();
  voxelRenderer.resize(200, 200);
  wait(delay_ms);
  voxelRenderer.getRenderer()->AddObserver(
      vtkCommand::ResetCameraEvent, &cameraResets, &EventCounter::increment);
  voxelRenderer.setClippingPlaneOriginSlider(&slider);
  voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
  voxelRenderer.setImage(image, origin);
  REQUIRE(cameraResets.events == 0);
  REQUIRE(voxelRenderer.getClippingPlane()->GetOrigin()[2] == dbl_approx(4.0));
  slider.setValue(slider.maximum());
  REQUIRE(voxelRenderer.getClippingPlane()->GetOrigin()[2] == dbl_approx(16.0));
}

TEST_CASE("QVoxelRenderer",
          "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  QVoxelRenderer voxelRenderer;
  voxelRenderer.show();
  voxelRenderer.resize(200, 200);
  std::vector<QRgb> clicks;
  QObject::connect(&voxelRenderer, &QVoxelRenderer::mouseClicked,
                   [&clicks](QRgb c) { clicks.push_back(c); });
  wait(delay_ms);
  auto gray = qRgb(50, 50, 50);
  auto red = qRgb(255, 0, 0);
  auto green = qRgb(0, 255, 0);
  auto blue = qRgb(0, 0, 255);
  SECTION("20x47x10 geometry image with colors for each face") {
    sme::common::Volume volume{20, 47, 10};
    sme::common::ImageStack imageStack(volume, QImage::Format_RGB32);
    REQUIRE(imageStack.volume() == volume);
    // fill whole block with gray
    imageStack.fill(gray);
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
    // click in corner of widget - should miss the volume
    REQUIRE(clicks.empty());
    sendMouseMove(&voxelRenderer, {1, 1});
    sendMouseClick(&voxelRenderer, {1, 1});
    wait(delay_ms);
    REQUIRE(clicks.empty());
    // click on centre of widget - should hit the gray volume
    QPoint centre{voxelRenderer.size().width() / 2,
                  voxelRenderer.size().height() / 2};
    sendMouseMove(&voxelRenderer, centre);
    sendMouseClick(&voxelRenderer, centre);
    wait(delay_ms);
    REQUIRE(clicks.size() == 1);
    REQUIRE(clicks.back() == gray);
    // blue z=0 face
    imageStack[0].fill(blue);
    imageStack[1].fill(blue);
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
    // green y=0 face
    for (std::size_t z = 0; z < volume.depth(); ++z) {
      for (int x = 0; x < volume.width(); ++x) {
        imageStack[z].setPixel(x, 0, green);
        imageStack[z].setPixel(x, 1, green);
      }
    }
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
    // red x=0 face
    for (std::size_t z = 0; z < volume.depth(); ++z) {
      for (int y = 0; y < volume.height(); ++y) {
        imageStack[z].setPixel(0, y, red);
        imageStack[z].setPixel(1, y, red);
      }
    }
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
    // set physical size that makes z-slices 5x thicker
    imageStack.setVoxelSize({1.0, 1.0, 5.0});
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
  }
  SECTION("100x100x1 geometry image") {
    auto model = sme::test::getExampleModel(Mod::VerySimpleModel);
    auto imageStack = model.getGeometry().getImages();
    REQUIRE(imageStack.volume() == sme::common::Volume{100, 100, 1});
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
    // changing the voxel size shouldn't change the rendered opacity
    for (double x : {1.0, 0.1, 0.01, 0.001}) {
      imageStack.setVoxelSize({x, x, x});
      voxelRenderer.setImage(imageStack);
      wait(delay_ms);
    }
  }
  SECTION("clipping plane uses physical origin") {
    auto model = sme::test::getExampleModel(Mod::SingleCompartmentDiffusion3D);
    auto imageStack = model.getGeometry().getImages();
    const auto &origin = model.getGeometry().getPhysicalOrigin();
    REQUIRE(origin.p.x() == dbl_approx(-50.0));
    REQUIRE(origin.p.y() == dbl_approx(-50.0));
    REQUIRE(origin.z == dbl_approx(-50.0));
    QSlider slider;
    QComboBox comboBox;
    voxelRenderer.setClippingPlaneOriginSlider(&slider);
    voxelRenderer.setClippingPlaneNormalCombobox(&comboBox);
    voxelRenderer.setImage(imageStack, origin);
    wait(delay_ms);
    const double xPad = imageStack.voxelSize().width();
    const double zPad = imageStack.voxelSize().depth();
    std::array<double, 3> planeOrigin{};
    voxelRenderer.getClippingPlane()->GetOrigin(planeOrigin.data());
    REQUIRE(planeOrigin[2] == dbl_approx(origin.z - zPad));

    slider.setValue(slider.maximum() / 2);
    wait(delay_ms);
    voxelRenderer.getClippingPlane()->GetOrigin(planeOrigin.data());
    REQUIRE(planeOrigin[2] == dbl_approx(0.0));

    comboBox.setCurrentText("x");
    slider.setValue(0);
    wait(delay_ms);
    voxelRenderer.getClippingPlane()->GetOrigin(planeOrigin.data());
    REQUIRE(planeOrigin[0] == dbl_approx(origin.p.x() - xPad));

    slider.setValue(slider.maximum());
    wait(delay_ms);
    voxelRenderer.getClippingPlane()->GetOrigin(planeOrigin.data());
    REQUIRE(planeOrigin[0] == dbl_approx(50.0 + xPad));
  }
}
