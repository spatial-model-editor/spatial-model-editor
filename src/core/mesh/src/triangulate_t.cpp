#include <QImage>
#include <QPoint>

#include "catch_wrapper.hpp"
#include "triangulate.hpp"

static double triangleArea(const QPointF &a, const QPointF &b,
                           const QPointF &c) {
  return 0.5 * std::abs(a.x() * b.y() + b.x() * c.y() + c.x() * a.y() -
                        b.x() * a.y() - c.x() * b.y() - a.x() * c.y());
}

static double maxTriangleArea(
    const std::vector<QPointF> &points,
    const std::vector<mesh::TriangleIndex> &triangles) {
  double maxArea = 0;
  for (const auto &t : triangles) {
    double area = triangleArea(points[t[0]], points[t[1]], points[t[2]]);
    maxArea = std::max(maxArea, area);
  }
  return maxArea;
}

SCENARIO("Triangulate",
         "[core/mesh/triangulate][core/mesh][core][triangulate]") {
  GIVEN("1 compartment") {
    mesh::TriangulateBoundaries tb;
    tb.boundaryPoints = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    tb.boundaries = {{{0, 1}, {1, 2}, {2, 3}, {3, 0}}};
    auto &comp = tb.compartments.emplace_back();
    comp.interiorPoint = {5.0, 5.0};
    auto &bp = tb.boundaryProperties.emplace_back();
    bp.isLoop = true;
    bp.isMembrane = false;
    bp.boundaryIndex = 1;
    WHEN("no max areas") {
      tb.compartments[0].maxTriangleArea = 9999;
      mesh::Triangulate tri(tb);
      // no additional points added: just split square into two triangles
      REQUIRE(tri.getPoints().size() == 4);
      REQUIRE(tri.getTriangleIndices().size() == 1);
      REQUIRE(tri.getTriangleIndices()[0].size() == 2);
      REQUIRE(tri.getRectangleIndices().empty() == true);
    }
    WHEN("max area specified") {
      tb.compartments[0].maxTriangleArea = 8;
      mesh::Triangulate tri(tb);
      REQUIRE(tri.getPoints().size() > 4);
      REQUIRE(tri.getTriangleIndices().size() == 1);
      REQUIRE(tri.getTriangleIndices()[0].size() > 2);
      REQUIRE(tri.getRectangleIndices().empty() == true);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[0]) <=
              tb.compartments[0].maxTriangleArea);
      // reduce max triangle area: more points & triangles
      tb.compartments[0].maxTriangleArea = 4;
      mesh::Triangulate tri2(tb);
      REQUIRE(tri2.getPoints().size() > tri.getPoints().size());
      REQUIRE(tri2.getTriangleIndices().size() == 1);
      REQUIRE(tri2.getTriangleIndices()[0].size() >
              tri.getTriangleIndices()[0].size());
      REQUIRE(tri2.getRectangleIndices().empty() == true);
      REQUIRE(maxTriangleArea(tri2.getPoints(), tri2.getTriangleIndices()[0]) <=
              tb.compartments[0].maxTriangleArea);
    }
  }
  GIVEN("2 compartments, no fixed points") {
    mesh::TriangulateBoundaries tb;
    tb.boundaryPoints = {{0, 0}, {10, 0}, {10, 10}, {0, 10},
                         {3, 3}, {7, 3},  {7, 7},   {3, 7}};
    tb.boundaries = {{{0, 1}, {1, 2}, {2, 3}, {3, 0}},
                     {{4, 5}, {5, 6}, {6, 7}, {7, 4}}};
    // outer compartment
    auto &compOuter = tb.compartments.emplace_back();
    compOuter.interiorPoint = {1.0, 1.0};
    auto &bpOuter = tb.boundaryProperties.emplace_back();
    bpOuter.isLoop = true;
    bpOuter.isMembrane = false;
    bpOuter.boundaryIndex = 0;
    // inner compartment
    auto &compInner = tb.compartments.emplace_back();
    compInner.interiorPoint = {5.0, 5.0};
    auto &bpInner = tb.boundaryProperties.emplace_back();
    bpInner.isLoop = true;
    bpInner.isMembrane = true;
    bpInner.boundaryIndex = 1;
    bpInner.width = 0.8;
    WHEN("no max area specified") {
      tb.compartments[0].maxTriangleArea = 999;
      tb.compartments[1].maxTriangleArea = 999;
      mesh::Triangulate tri(tb);
      // minimal triangulation:
      // original 8 points + one Steiner point for each outer segment
      REQUIRE(tri.getPoints().size() == 12);
      REQUIRE(tri.getTriangleIndices().size() == 2);
      // outer compartment: each quadrilateral -> two triangles
      REQUIRE(tri.getTriangleIndices()[0].size() == 8);
      // inner compartment: square -> two triangles
      REQUIRE(tri.getTriangleIndices()[1].size() == 2);
      REQUIRE(tri.getRectangleIndices().size() == 1);
    }
    WHEN("max area specified") {
      tb.compartments[0].maxTriangleArea = 5;
      tb.compartments[1].maxTriangleArea = 3;
      mesh::Triangulate tri(tb);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[0]) <=
              tb.compartments[0].maxTriangleArea);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[1]) <=
              tb.compartments[1].maxTriangleArea);
    }
  }
  GIVEN("2 compartments, 1 shared border, 2 fixed points") {
    mesh::TriangulateBoundaries tb;
    tb.boundaryPoints = {{10, 0}, {10, 10}, {0, 0}, {0, 10}, {20, 0}, {20, 10}};
    // left boundary
    tb.boundaries.push_back({{0, 2}, {2, 3}, {3, 1}});
    // right boundary
    tb.boundaries.push_back({{0, 4}, {4, 5}, {5, 1}});
    // middle shared boundary
    tb.boundaries.push_back({{0, 1}});
    // left compartment
    auto &compLeft = tb.compartments.emplace_back();
    compLeft.interiorPoint = {5.0, 5.0};
    auto &bpLeft = tb.boundaryProperties.emplace_back();
    bpLeft.isLoop = false;
    bpLeft.isMembrane = false;
    bpLeft.boundaryIndex = 0;
    bpLeft.innerFPIndices = {0, 1};
    // right compartment
    auto &compRight = tb.compartments.emplace_back();
    compRight.interiorPoint = {15.0, 5.0};
    auto &bpRight = tb.boundaryProperties.emplace_back();
    bpRight.isLoop = false;
    bpRight.isMembrane = false;
    bpRight.boundaryIndex = 1;
    bpRight.innerFPIndices = {2, 3};
    // membrane
    auto &bpMembrane = tb.boundaryProperties.emplace_back();
    bpMembrane.isLoop = false;
    bpMembrane.isMembrane = true;
    bpMembrane.boundaryIndex = 2;
    bpMembrane.width = 0.66;
    bpMembrane.innerFPIndices = {2, 3};
    bpMembrane.outerFPIndices = {0, 1};
    // fixed points
    mesh::TriangulateFixedPoints tfp;
    tfp.nFPs = 2;
    tfp.newFPs = {{9.5, 0.5}, {9.5, 9.5}, {10.5, 0.5}, {10.5, 9.5}};
    WHEN("no max area specified") {
      tb.compartments[0].maxTriangleArea = 999;
      tb.compartments[1].maxTriangleArea = 999;
      mesh::Triangulate tri(tb, tfp);
      // minimal triangulation:
      // original points
      REQUIRE(tri.getPoints().size() == 8);
      REQUIRE(tri.getTriangleIndices().size() == 2);
      // each compartment: quadrilateral -> two triangles
      REQUIRE(tri.getTriangleIndices()[0].size() == 2);
      REQUIRE(tri.getTriangleIndices()[1].size() == 2);
      REQUIRE(tri.getRectangleIndices().size() == 1);
      // single rectangle for membrane
      REQUIRE(tri.getRectangleIndices()[0].size() == 1);
    }
    WHEN("max area specified") {
      tb.compartments[0].maxTriangleArea = 4;
      tb.compartments[1].maxTriangleArea = 3;
      mesh::Triangulate tri(tb, tfp);
      REQUIRE(tri.getTriangleIndices().size() == 2);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[0]) <=
              tb.compartments[0].maxTriangleArea);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[1]) <=
              tb.compartments[1].maxTriangleArea);
    }
  }
}
