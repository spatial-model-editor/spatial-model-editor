#include "boundary.hpp"
#include "catch_wrapper.hpp"
#include <QImage>
#include <QPoint>
#include <cmath>

SCENARIO("Boundary", "[core/mesh/boundary][core/mesh][core][boundary]") {
  GIVEN("Loop") {
    // points that make up the loop
    std::vector<QPoint> points{{0, 0}, {2, 0},  {3, 1}, {1, 3},
                               {0, 3}, {-1, 2}, {-1, 1}};
    // 3-point approximation to loop
    std::vector<QPoint> p3{
        {0, 0},
        {3, 1},
        {1, 3},
    };
    WHEN("Non-membrane") {
      mesh::Boundary boundary(points, true);
      REQUIRE(boundary.isLoop() == true);
      REQUIRE(boundary.isMembrane() == false);
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
      REQUIRE(nPoints == 3);
      REQUIRE(boundary.getPoints() == p3);
    }
    WHEN("Membrane") {
      mesh::Boundary boundary(points, true, true, "membrane");
      REQUIRE(boundary.isLoop() == true);
      REQUIRE(boundary.isMembrane() == true);
      REQUIRE(boundary.getMembraneId() == "membrane");
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
      REQUIRE(nPoints == 3);
      REQUIRE(boundary.getPoints() == p3);
    }
  }
  GIVEN("Non-loop") {
    // points that make up the boundary line
    std::vector<QPoint> points{{0, 0}, {2, 0}, {3, 1}, {4, 1},
                               {4, 2}, {5, 3}, {5, 4}};
    // 4-point approximation to line
    std::vector<QPoint> p4{{0, 0}, {2, 0}, {4, 1}, {5, 4}};
    // 3-point approximation to line
    std::vector<QPoint> p3{{0, 0}, {4, 1}, {5, 4}};
    WHEN("Non-membrane") {
      mesh::Boundary boundary(points, false);
      REQUIRE(boundary.isLoop() == false);
      REQUIRE(boundary.isMembrane() == false);
      // use all points
      boundary.setMaxPoints(999);
      REQUIRE(boundary.getMaxPoints() == 999);
      REQUIRE(boundary.getPoints() == points);
      // use 3 points for boundary
      boundary.setMaxPoints(4);
      REQUIRE(boundary.getMaxPoints() == 4);
      REQUIRE(boundary.getPoints() == p4);
      // automatically determine number of points
      auto nPoints = boundary.setMaxPoints();
      REQUIRE(boundary.getMaxPoints() == nPoints);
      REQUIRE(nPoints == 3);
      REQUIRE(boundary.getPoints() == p3);
      // can set indices for first/last points
      mesh::FpIndices fpIndices;
      fpIndices.startPoint = 5;
      fpIndices.endPoint = 2;
      boundary.setFpIndices(fpIndices);
      REQUIRE(boundary.getFpIndices().startPoint == fpIndices.startPoint);
      REQUIRE(boundary.getFpIndices().endPoint == fpIndices.endPoint);
    }
    WHEN("Membrane") {
      mesh::Boundary boundary(points, false, true, "membrane");
      REQUIRE(boundary.isLoop() == false);
      REQUIRE(boundary.isMembrane() == true);
      REQUIRE(boundary.getMembraneId() == "membrane");
      // use all points
      boundary.setMaxPoints(999);
      REQUIRE(boundary.getMaxPoints() == 999);
      REQUIRE(boundary.getPoints() == points);
      // use 3 points for boundary
      boundary.setMaxPoints(4);
      REQUIRE(boundary.getMaxPoints() == 4);
      REQUIRE(boundary.getPoints() == p4);
      // automatically determine number of points
      auto nPoints = boundary.setMaxPoints();
      REQUIRE(boundary.getMaxPoints() == nPoints);
      REQUIRE(nPoints == 3);
      REQUIRE(boundary.getPoints() == p3);
      // can set indices for first/last points
      mesh::FpIndices fpIndices;
      fpIndices.startPoint = 5;
      fpIndices.endPoint = 2;
      boundary.setFpIndices(fpIndices);
      REQUIRE(boundary.getFpIndices().startPoint == fpIndices.startPoint);
      REQUIRE(boundary.getFpIndices().endPoint == fpIndices.endPoint);
    }
  }
}
