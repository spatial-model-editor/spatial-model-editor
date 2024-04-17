#include "catch_wrapper.hpp"
#include "sme/image_stack.hpp"

TEST_CASE("ImageStack",
          "[core/common/image_stack][core/common][core][image_stack]") {
  SECTION("empty") {
    sme::common::ImageStack imageStack{};
    REQUIRE(imageStack.empty() == true);
    REQUIRE(imageStack.valid({0, 0, 0}) == false);
  }
  SECTION("construct from volume") {
    sme::common::ImageStack imageStack({3, 10, 12},
                                       QImage::Format_ARGB32_Premultiplied);
    REQUIRE(imageStack.empty() == false);
    REQUIRE(imageStack.volume().width() == 3);
    REQUIRE(imageStack.volume().height() == 10);
    REQUIRE(imageStack.volume().depth() == 12);
    REQUIRE(imageStack.volume().nVoxels() == 3 * 10 * 12);
    REQUIRE(imageStack.valid({0, 0, 0}) == true);
    REQUIRE(imageStack.valid({1, 1, 11}) == true);
    REQUIRE(imageStack.valid({1, 1, 12}) == false);
    REQUIRE(imageStack.valid({2, 0, 0}) == true);
    REQUIRE(imageStack.valid({3, 0, 0}) == false);
  }
  SECTION("construct from grayscale intensity array") {
    SECTION("all values identical and non-zero should be white") {
      sme::common::ImageStack img1{{2, 2, 1},
                                   std::vector<double>{1.2, 1.2, 1.2, 1.2}};
      REQUIRE(img1.volume().width() == 2);
      REQUIRE(img1.volume().height() == 2);
      REQUIRE(img1.volume().depth() == 1);
      REQUIRE(img1[0].format() == QImage::Format_RGB32);
      REQUIRE(img1[0].pixel(0, 0) == qRgb(255, 255, 255));
      REQUIRE(img1[0].pixel(0, 1) == qRgb(255, 255, 255));
      REQUIRE(img1[0].pixel(1, 0) == qRgb(255, 255, 255));
      REQUIRE(img1[0].pixel(1, 1) == qRgb(255, 255, 255));
    }
    SECTION("all values zero should be black") {
      sme::common::ImageStack img1{{2, 2, 1}, std::vector<double>{0, 0, 0, 0}};
      REQUIRE(img1.volume().width() == 2);
      REQUIRE(img1.volume().height() == 2);
      REQUIRE(img1.volume().depth() == 1);
      REQUIRE(img1[0].pixel(0, 0) == qRgb(0, 0, 0));
      REQUIRE(img1[0].pixel(0, 1) == qRgb(0, 0, 0));
      REQUIRE(img1[0].pixel(1, 0) == qRgb(0, 0, 0));
      REQUIRE(img1[0].pixel(1, 1) == qRgb(0, 0, 0));
    }
    SECTION("pixels start bottom-left, scan along x, end at top right") {
      sme::common::ImageStack img1{{2, 2, 1},
                                   std::vector<double>{1.0, 2.0, 3.0, 4.0}};
      REQUIRE(img1.volume().width() == 2);
      REQUIRE(img1.volume().height() == 2);
      REQUIRE(img1.volume().depth() == 1);
      REQUIRE(img1[0].pixel(0, 1) == qRgb(63, 63, 63));
      REQUIRE(img1[0].pixel(1, 1) == qRgb(127, 127, 127));
      REQUIRE(img1[0].pixel(0, 0) == qRgb(191, 191, 191));
      REQUIRE(img1[0].pixel(1, 0) == qRgb(255, 255, 255));
    }
    SECTION("invalid array volume produces black image") {
      // array too small
      sme::common::ImageStack img1{{11, 13, 1}, std::vector<double>{}};
      REQUIRE(img1.volume().width() == 11);
      REQUIRE(img1.volume().height() == 13);
      REQUIRE(img1.volume().depth() == 1);
      REQUIRE(img1[0].pixel(0, 0) == qRgb(0, 0, 0));
      // array too large
      sme::common::ImageStack img2{{1, 1, 1}, std::vector<double>{1.0, 2.0}};
      REQUIRE(img2.volume().width() == 1);
      REQUIRE(img2.volume().height() == 1);
      REQUIRE(img2.volume().depth() == 1);
      REQUIRE(img2[0].pixel(0, 0) == qRgb(0, 0, 0));
    }
  }
}
