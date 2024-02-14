#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/image_stack.hpp"

using namespace sme::test;

// set to e.g. 10000 to interactively inspect the rendering of each ImageStack
constexpr int delay_ms{0};

TEST_CASE("QVoxelRenderer",
          "[qvoxelrenderer][gui/widgets/qvoxelrenderer][gui/widgets][gui]") {
  QVoxelRenderer voxelRenderer;
  voxelRenderer.show();
  auto gray = qRgb(50, 50, 50);
  auto red = qRgb(255, 0, 0);
  auto green = qRgb(0, 255, 0);
  auto blue = qRgb(0, 0, 255);
  SECTION("20x47x10 geometry image with colors for each face") {
    sme::common::Volume volume{20, 47, 10};
    sme::common::ImageStack imageStack(volume, QImage::Format_RGB32);
    REQUIRE(imageStack.volume() == volume);
    wait(delay_ms);
    // fill whole block with gray
    imageStack.fill(gray);
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
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
    // set physical size that maintains existing cubic voxels
    voxelRenderer.setPhysicalSize({20.0, 47.0, 10.0}, "units");
    wait(delay_ms);
    // set physical size that makes z-slices 5x thicker
    voxelRenderer.setPhysicalSize({20.0, 47.0, 50.0}, "units");
    wait(delay_ms);
  }
  SECTION("100x100x1 geometry image") {
    auto model = sme::test::getExampleModel(Mod::VerySimpleModel);
    auto imageStack = model.getGeometry().getImages();
    REQUIRE(imageStack.volume() == sme::common::Volume{100, 100, 1});
    voxelRenderer.setImage(imageStack);
    wait(delay_ms);
  }
}
