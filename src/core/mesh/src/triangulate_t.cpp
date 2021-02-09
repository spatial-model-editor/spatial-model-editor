#include "catch_wrapper.hpp"
#include "triangulate.hpp"
#include <QImage>
#include <QPoint>
#include <cmath>

using namespace sme;

static double triangleArea(const QPointF &a, const QPointF &b,
                           const QPointF &c) {
  return 0.5 * std::abs(a.x() * b.y() + b.x() * c.y() + c.x() * a.y() -
                        b.x() * a.y() - c.x() * b.y() - a.x() * c.y());
}

static double
maxTriangleArea(const std::vector<QPointF> &points,
                const std::vector<mesh::TriangulateTriangleIndex> &triangles) {
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
    mesh::Boundary boundary({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, true);
    QPointF interiorPoint(5.0, 5.0);
    WHEN("no max areas") {
      mesh::Triangulate tri({boundary}, {{interiorPoint}}, {999});
      // no additional points added: just split square into two triangles
      REQUIRE(tri.getPoints().size() == 4);
      REQUIRE(tri.getTriangleIndices().size() == 1);
      REQUIRE(tri.getTriangleIndices()[0].size() == 2);
    }
    WHEN("max area specified") {
      mesh::Triangulate tri({boundary}, {{interiorPoint}}, {8});
      REQUIRE(tri.getPoints().size() > 4);
      REQUIRE(tri.getTriangleIndices().size() == 1);
      REQUIRE(tri.getTriangleIndices()[0].size() > 2);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[0]) <=
              8);
      // reduce max triangle area: more points & triangles
      mesh::Triangulate tri2({boundary}, {{interiorPoint}}, {4});
      REQUIRE(tri2.getPoints().size() > tri.getPoints().size());
      REQUIRE(tri2.getTriangleIndices().size() == 1);
      REQUIRE(tri2.getTriangleIndices()[0].size() >
              tri.getTriangleIndices()[0].size());
      REQUIRE(maxTriangleArea(tri2.getPoints(), tri2.getTriangleIndices()[0]) <=
              4);
    }
  }
  GIVEN("2 compartments, no fixed points") {
    std::vector<mesh::Boundary> boundaries;
    // inner compartment
    boundaries.push_back(
        mesh::Boundary({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, true));
    std::vector<std::vector<QPointF>> interiorPoints;
    interiorPoints.push_back({{5.0, 5.0}});
    // inner compartment
    boundaries.push_back(
        mesh::Boundary({{4, 4}, {6, 4}, {6, 6}, {4, 6}}, true));
    interiorPoints.push_back({{1.0, 1.0}});
    WHEN("no max area specified") {
      mesh::Triangulate tri(boundaries, interiorPoints, {9999, 9999});
      // minimal triangulation:
      // original 8 points + 1 point to split each outer quadrilateral
      REQUIRE(tri.getPoints().size() == 12);
      REQUIRE(tri.getTriangleIndices().size() == 2);
      // inner compartment: square -> two triangles
      REQUIRE(tri.getTriangleIndices()[0].size() == 2);
      // outer compartment: each quadrilateral -> 3 triangles
      REQUIRE(tri.getTriangleIndices()[1].size() == 12);
    }
    WHEN("max area specified") {
      mesh::Triangulate tri(boundaries, interiorPoints, {5, 3});
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[0]) <=
              5);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[1]) <=
              3);
    }
  }
  GIVEN("2 compartments, 1 shared border, 2 fixed points") {
    std::vector<mesh::Boundary> boundaries;
    // left boundary
    boundaries.push_back(
        mesh::Boundary({{10, 0}, {0, 0}, {0, 10}, {10, 10}}, false));
    // right boundary
    boundaries.push_back(
        mesh::Boundary({{10, 0}, {20, 0}, {20, 10}, {10, 10}}, false));
    // middle shared boundary
    boundaries.push_back(mesh::Boundary({{10, 0}, {10, 10}}, false));

    std::vector<std::vector<QPointF>> interiorPoints;
    // left compartment
    interiorPoints.push_back({{5.0, 5.0}});
    // right compartment
    interiorPoints.push_back({{15.0, 5.0}});

    WHEN("no max area specified") {
      mesh::Triangulate tri(boundaries, interiorPoints, {999, 999});
      // minimal triangulation:
      // original points
      REQUIRE(tri.getPoints().size() == 6);
      REQUIRE(tri.getTriangleIndices().size() == 2);
      // each compartment: quadrilateral -> two triangles
      REQUIRE(tri.getTriangleIndices()[0].size() == 2);
      REQUIRE(tri.getTriangleIndices()[1].size() == 2);
    }
    WHEN("max area specified") {
      mesh::Triangulate tri(boundaries, interiorPoints, {4, 3});
      REQUIRE(tri.getTriangleIndices().size() == 2);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[0]) <=
              4);
      REQUIRE(maxTriangleArea(tri.getPoints(), tri.getTriangleIndices()[1]) <=
              3);
    }
  }
}
