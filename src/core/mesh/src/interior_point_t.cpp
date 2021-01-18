#include "interior_point.hpp"
#include "catch_wrapper.hpp"
#include <QImage>
#include <cmath>

using namespace sme;

SCENARIO("InteriorPoint", "[core/mesh/interior_point][core/mesh][core][interior_point]") {
  GIVEN("Single pixel") {
    QImage img(1, 1,QImage::Format_RGB32);
    QRgb col{qRgb(1,2,3)};
    img.fill(col);
    auto points{mesh::getInteriorPoints(img, {col})};
    REQUIRE(points.size() == 1);
    REQUIRE(points[0].size() == 1);
    REQUIRE(points[0][0].x() == dbl_approx(0.5));
    REQUIRE(points[0][0].y() == dbl_approx(0.5));
  }
}
