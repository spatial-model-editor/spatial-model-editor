#include "boundaries.hpp"
#include "catch_wrapper.hpp"
#include "polyline_simplifier.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace sme;

static std::size_t
countVertices(const std::vector<mesh::Boundary> &boundaries) {
  return std::accumulate(
      boundaries.cbegin(), boundaries.cend(), std::size_t{0},
      [](std::size_t n, const auto &b) { return n + b.getPoints().size(); });
}

TEST_CASE(
    "PolylineSimplifier",
    "[core/mesh/polyline_simplifier][core/mesh][core][polyline_simplifier]") {
  SECTION("Invalid inputs are no-ops") {
    std::vector<mesh::Boundary> b{
        mesh::Boundary({QPoint(1, 1), QPoint(1, 3), QPoint(3, 1)}, true)};
    mesh::PolylineSimplifier polylineSimplifier(b);
    REQUIRE(b.front().getAllPoints().size() == 3);
    REQUIRE(b.front().isLoop() == true);
    b.clear();
    REQUIRE(b.empty());
    polylineSimplifier.setMaxPoints(b, 123);
    REQUIRE(b.empty());
  }
  SECTION("Concave loops") {
    QImage img(":/geometry/concave-cell-nucleus-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(35, 20);
    QRgb col2 = img.pixel(40, 50);
    std::vector<mesh::Boundary> boundaries{
        mesh::Boundaries(img, {col0, col1, col2}, 0).getBoundaries()};
    mesh::PolylineSimplifier polylineSimplifier(boundaries);
    polylineSimplifier.setMaxPoints(boundaries, 20);
    // for concave loops: max points equal to number of vertices
    REQUIRE(polylineSimplifier.getMaxPoints() == 20);
    REQUIRE(countVertices(boundaries) == 20);
    polylineSimplifier.setMaxPoints(boundaries, 24);
    REQUIRE(polylineSimplifier.getMaxPoints() == 24);
    REQUIRE(countVertices(boundaries) == 24);
    polylineSimplifier.setMaxPoints(boundaries, 15);
    REQUIRE(polylineSimplifier.getMaxPoints() == 15);
    REQUIRE(countVertices(boundaries) == 15);
    // automatic choice of max points:
    polylineSimplifier.setMaxPoints(boundaries, 0);
    REQUIRE(polylineSimplifier.getMaxPoints() > 0);
    REQUIRE(countVertices(boundaries) > 0);
    // setting maxPoints below minimium possible value sets it to minimum
    polylineSimplifier.setMaxPoints(boundaries, 3);
    REQUIRE(polylineSimplifier.getMaxPoints() == 11);
    REQUIRE(countVertices(boundaries) == 11);
  }
  SECTION("Mix of loops and lines") {
    QImage img(":/geometry/liver-cells-200x100.png");
    QRgb col0 = img.pixel(24, 50);
    QRgb col1 = img.pixel(76, 14);
    QRgb col2 = img.pixel(20, 15);
    std::vector<mesh::Boundary> boundaries{
        mesh::Boundaries(img, {col0, col1, col2}, 0).getBoundaries()};
    mesh::PolylineSimplifier polylineSimplifier(boundaries);
    // set max points too small
    polylineSimplifier.setMaxPoints(boundaries, 12);
    REQUIRE(polylineSimplifier.getMaxPoints() == Catch::Approx(61).margin(2));
    // more vertices than points, as some vertices present in multiple boundary
    // lines
    REQUIRE(countVertices(boundaries) == Catch::Approx(97).margin(2));
    // set max points too large
    polylineSimplifier.setMaxPoints(boundaries, 999999);
    REQUIRE(polylineSimplifier.getMaxPoints() == 626);
    REQUIRE(countVertices(boundaries) == 662);
    polylineSimplifier.setMaxPoints(boundaries, 88);
    REQUIRE(polylineSimplifier.getMaxPoints() == 88);
    REQUIRE(countVertices(boundaries) == 124);
    polylineSimplifier.setMaxPoints(boundaries, 150);
    REQUIRE(polylineSimplifier.getMaxPoints() == 150);
    REQUIRE(countVertices(boundaries) == 186);
    // setting maxPoints to current value is a no-op
    polylineSimplifier.setMaxPoints(boundaries, 150);
    REQUIRE(polylineSimplifier.getMaxPoints() == 150);
    REQUIRE(countVertices(boundaries) == 186);
    polylineSimplifier.setMaxPoints(boundaries, 144);
    REQUIRE(polylineSimplifier.getMaxPoints() == 144);
    REQUIRE(countVertices(boundaries) == 180);
    // automatic choice of max points:
    polylineSimplifier.setMaxPoints(boundaries, 0);
    REQUIRE(polylineSimplifier.getMaxPoints() > 0);
    REQUIRE(countVertices(boundaries) > 0);
  }
}
