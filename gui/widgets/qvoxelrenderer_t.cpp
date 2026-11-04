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

} // namespace

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
