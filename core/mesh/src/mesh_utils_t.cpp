#include "catch_wrapper.hpp"
#include "mesh_utils.hpp"
#include <QImage>
#include <QPoint>
#include <cmath>

using namespace sme;

TEST_CASE("MeshUtils", "[core/mesh/mesh_utils][core/mesh][core][mesh_utils]") {
  SECTION("makeBinaryMask: single colour") {
    QImage img(10, 20, QImage::Format_RGB32);
    QRgb bg{qRgb(1, 2, 3)};
    QRgb fg{qRgb(21, 22, 32)};
    img.fill(bg);
    img.setPixel(3, 6, fg);
    img.setPixel(9, 8, fg);

    auto mask_fg{mesh::makeBinaryMask(img, fg)};
    REQUIRE(mask_fg.cols == img.width());
    REQUIRE(mask_fg.rows == img.height());
    REQUIRE(mask_fg.at<uint8_t>(6, 3) == 255);
    REQUIRE(mask_fg.at<uint8_t>(8, 9) == 255);
    REQUIRE(mask_fg.at<uint8_t>(3, 6) == 0);
    REQUIRE(mask_fg.at<uint8_t>(9, 8) == 0);

    auto mask_bg{mesh::makeBinaryMask(img, bg)};
    REQUIRE(mask_bg.cols == img.width());
    REQUIRE(mask_bg.rows == img.height());
    REQUIRE(mask_bg.at<uint8_t>(6, 3) == 0);
    REQUIRE(mask_bg.at<uint8_t>(8, 9) == 0);
    REQUIRE(mask_bg.at<uint8_t>(3, 6) == 255);
    REQUIRE(mask_bg.at<uint8_t>(9, 8) == 255);
  }
}
