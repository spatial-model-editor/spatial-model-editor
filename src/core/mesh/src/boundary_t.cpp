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
      // non-membrane inner points are just QPointF versions of QPoint points
      auto &inner = boundary.getInnerPoints();
      for (std::size_t i = 0; i < inner.size(); ++i) {
        QPointF diff = inner[i] - p3[i];
        REQUIRE(dbl_approx(diff.manhattanLength()) == 0);
      }
      // non-membrane boundaries don't have outer points
      REQUIRE(boundary.getOuterPoints().empty() == true);
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
      // membrane inner points are displaced
      REQUIRE(boundary.getInnerPoints().size() == boundary.getPoints().size());
      const auto &inner = boundary.getInnerPoints();
      const auto &outer = boundary.getOuterPoints();
      REQUIRE(inner.size() == boundary.getPoints().size());
      REQUIRE(outer.size() == boundary.getPoints().size());
      // matching inner/outer points separated by membrane width
      for (std::size_t i = 0; i < inner.size(); ++i) {
        QPointF d = inner[i] - outer[i];
        double distance = std::hypot(d.x(), d.y());
        REQUIRE(dbl_approx(distance) == boundary.getMembraneWidth());
      }
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
      // non-membrane inner points are just QPointF versions of QPoint points
      const auto &inner = boundary.getInnerPoints();
      for (std::size_t i = 0; i < inner.size(); ++i) {
        QPointF diff = inner[i] - p3[i];
        REQUIRE(dbl_approx(diff.manhattanLength()) == 0);
      }
      // non-membrane boundaries don't have outer points
      REQUIRE(boundary.getOuterPoints().empty() == true);
      // can set indices for first/last points
      mesh::FpIndices fpIndices;
      fpIndices.startPoint = 5;
      fpIndices.endPoint = 2;
      boundary.setFpIndices(fpIndices);
      REQUIRE(boundary.getFpIndices().startPoint == fpIndices.startPoint);
      REQUIRE(boundary.getFpIndices().endPoint == fpIndices.endPoint);
      // can replace inner start/end points & assign corresponding new index
      QPointF startFP(0.23, -0.12);
      QPointF endFP(5.029, 4.126);
      boundary.setInnerStartPoint(startFP, 2);
      boundary.setInnerEndPoint(endFP, 9);
      REQUIRE(dbl_approx((boundary.getInnerPoints().front() - startFP)
                             .manhattanLength()) == 0);
      REQUIRE(dbl_approx((boundary.getInnerPoints().back() - endFP)
                             .manhattanLength()) == 0);
      REQUIRE(boundary.getInnerFpIndices().startPoint == 2);
      REQUIRE(boundary.getInnerFpIndices().endPoint == 9);
    }
    WHEN("Membrane") {
      mesh::Boundary boundary(points, false, true, "membrane");
      boundary.setMembraneWidth(0.5);
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
      // matching inner/outer points separated by membrane width
      const auto &inner = boundary.getInnerPoints();
      const auto &outer = boundary.getOuterPoints();
      REQUIRE(inner.size() == boundary.getPoints().size());
      CAPTURE(inner);
      REQUIRE(outer.size() == boundary.getPoints().size());
      CAPTURE(outer);
      for (std::size_t i = 0; i < inner.size(); ++i) {
        QPointF d = inner[i] - outer[i];
        double distance = std::hypot(d.x(), d.y());
        CAPTURE(i);
        CAPTURE(inner[i]);
        CAPTURE(outer[i]);
        CAPTURE(d);
        REQUIRE(dbl_approx(distance) == boundary.getMembraneWidth());
      }
      // can set indices for first/last points
      mesh::FpIndices fpIndices;
      fpIndices.startPoint = 5;
      fpIndices.endPoint = 2;
      boundary.setFpIndices(fpIndices);
      REQUIRE(boundary.getFpIndices().startPoint == fpIndices.startPoint);
      REQUIRE(boundary.getFpIndices().endPoint == fpIndices.endPoint);
      // can replace inner/outer start/end points & assign corresponding new
      // index: note inner/outer FPs in general are no longer separated by
      // membrane width
      QPointF innerStartFP(0.23, -0.12);
      QPointF innerEndFP(5.029, 4.126);
      boundary.setInnerStartPoint(innerStartFP, 2);
      boundary.setInnerEndPoint(innerEndFP, 9);
      REQUIRE(dbl_approx((boundary.getInnerPoints().front() - innerStartFP)
                             .manhattanLength()) == 0);
      REQUIRE(dbl_approx((boundary.getInnerPoints().back() - innerEndFP)
                             .manhattanLength()) == 0);
      REQUIRE(boundary.getInnerFpIndices().startPoint == 2);
      REQUIRE(boundary.getInnerFpIndices().endPoint == 9);
      // outer FPs
      QPointF outerStartFP(0.33, -0.22);
      QPointF outerEndFP(5.129, 4.426);
      boundary.setOuterStartPoint(outerStartFP, 11);
      boundary.setOuterEndPoint(outerEndFP, 13);
      REQUIRE(dbl_approx((boundary.getOuterPoints().front() - outerStartFP)
                             .manhattanLength()) == 0);
      REQUIRE(dbl_approx((boundary.getOuterPoints().back() - outerEndFP)
                             .manhattanLength()) == 0);
      REQUIRE(boundary.getOuterFpIndices().startPoint == 11);
      REQUIRE(boundary.getOuterFpIndices().endPoint == 13);
    }
  }
}
