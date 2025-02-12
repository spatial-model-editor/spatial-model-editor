#include "catch_wrapper.hpp"
#include "sme/image_stack_impl.hpp"

TEST_CASE("ImageStackImpl",
          "[core/common/image_stack][core/common][core][image_stack]") {
  SECTION("1 image, 1 color") {
    QSize sz(20, 20);
    auto color = qRgb(255, 0, 0);
    auto img = QImage(sz, QImage::Format_RGB32);
    img.fill(color);
    auto colorTable = sme::common::getCombinedColorTable({img, img});
    REQUIRE(colorTable.size() == 1);
    REQUIRE(colorTable[0] == color);
  }
  SECTION("different images in stack have different colors") {
    QSize sz(11, 8);
    auto c1 = qRgb(255, 0, 0);
    auto c2 = qRgb(255, 250, 0);
    auto c3 = qRgb(255, 140, 0);
    auto c4 = qRgb(255, 20, 0);
    auto c5 = qRgb(255, 0, 66);
    auto img1 = QImage(sz, QImage::Format_RGB32);
    img1.fill(c1);
    auto img2 = QImage(sz, QImage::Format_RGB32);
    img2.fill(c2);
    auto img3 = QImage(sz, QImage::Format_RGB32);
    img3.fill(c3);
    auto img4 = QImage(sz, QImage::Format_RGB32);
    img4.fill(c4);
    img4.setPixel(2, 3, c5);
    img4.setPixel(6, 3, c2);
    auto img5 = QImage(40, 20, QImage::Format_RGB32);
    img5.fill(c1);
    auto colorTable =
        sme::common::getCombinedColorTable({img1, img2, img3, img4, img5});
    REQUIRE(colorTable.size() == 5);
    REQUIRE(colorTable.contains(c1));
    REQUIRE(colorTable.contains(c2));
    REQUIRE(colorTable.contains(c3));
    REQUIRE(colorTable.contains(c4));
    REQUIRE(colorTable.contains(c5));
  }
  SECTION("images with >256 colors are reduced to 256 colors") {
    QSize sz(512, 512);
    auto img1 = QImage(sz, QImage::Format_RGB32);
    auto img2 = QImage(sz, QImage::Format_RGB32);
    for (int y = 0; y < sz.height(); ++y) {
      for (int x = 0; x < sz.width(); ++x) {
        auto c1 = qRgb(x % 256, y % 256, (15 + x + y) % 128);
        auto c2 = qRgb(x % 256, y % 256, (15 + x + y) % 128 + 128);
        img1.setPixel(x, y, c1);
        img2.setPixel(x, y, c2);
      }
    }
    auto colorTable = sme::common::getCombinedColorTable({img1, img2});
    REQUIRE(colorTable.size() == 256);
  }
}
