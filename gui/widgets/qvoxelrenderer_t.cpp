#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/image_stack.hpp"

using namespace sme::test;

// set to e.g. 1000 to interactively inspect the rendering
constexpr int delay_ms{0};

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
}
