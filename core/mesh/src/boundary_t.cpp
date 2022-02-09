#include "boundary.hpp"
#include "catch_wrapper.hpp"
#include <QPoint>
#include <cmath>

using namespace sme;

TEST_CASE("Boundary", "[core/mesh/boundary][core/mesh][core][boundary]") {
  SECTION("Loop") {
    // points that make up the loop
    std::vector<QPoint> points{{0, 0}, {2, 0},  {3, 1}, {1, 3},
                               {0, 3}, {-1, 2}, {-1, 1}};
    // 3-point approximation to loop
    std::vector<QPoint> p3{
        {0, 0},
        {3, 1},
        {1, 3},
    };
    // 4-point approximation to loop
    std::vector<QPoint> p4{
        {0, 0},
        {3, 1},
        {1, 3},
        {-1, 2},
    };
    mesh::Boundary boundary(points, true);
    REQUIRE(boundary.isValid() == true);
    REQUIRE(boundary.isLoop() == true);
    // use all points
    boundary.setMaxPoints(999);
    REQUIRE(boundary.getMaxPoints() == 999);
    REQUIRE(boundary.getPoints() == points);
    // use 3 points for boundary
    boundary.setMaxPoints(3);
    REQUIRE(boundary.getMaxPoints() == 3);
    REQUIRE(boundary.getPoints() == p3);
    // automatically determine number of points
    auto nPoints = boundary.setMaxPoints();
    REQUIRE(boundary.getMaxPoints() == nPoints);
    REQUIRE(nPoints == 4);
    REQUIRE(boundary.getPoints() == p4);
  }
  SECTION("Non-loop") {
    // points that make up the boundary line
    std::vector<QPoint> points{{0, 0}, {2, 0}, {3, 1}, {4, 1},
                               {4, 2}, {5, 3}, {5, 4}};
    // 4-point approximation to line
    std::vector<QPoint> p4{{0, 0}, {2, 0}, {4, 1}, {5, 4}};
    // 3-point approximation to line
    std::vector<QPoint> p3{{0, 0}, {4, 1}, {5, 4}};
    mesh::Boundary boundary(points, false);
    REQUIRE(boundary.isValid() == true);
    REQUIRE(boundary.isLoop() == false);
    // use all points
    boundary.setMaxPoints(999);
    REQUIRE(boundary.getMaxPoints() == 999);
    REQUIRE(boundary.getPoints() == points);
    // use 3 points for boundary
    boundary.setMaxPoints(3);
    REQUIRE(boundary.getMaxPoints() == 3);
    REQUIRE(boundary.getPoints() == p3);
    // automatically determine number of points
    auto nPoints = boundary.setMaxPoints();
    REQUIRE(boundary.getMaxPoints() == nPoints);
    REQUIRE(nPoints == 4);
    REQUIRE(boundary.getPoints() == p4);
  }
}
