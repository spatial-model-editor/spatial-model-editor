#include "catch_wrapper.hpp"
#include "contour_map.hpp"
#include <QSize>

SCENARIO("ContourMap",
         "[core/mesh/contour_map][core/mesh][core][contour_map]") {
  GIVEN("two compartment contours, one edge contour") {
    mesh::Contours contours;
    contours.domainEdges = {{{0, 0}, {1, 0}, {2, 0}}};
    contours.compartmentEdges = {{{0, 0}, {0, 1}, {0, 2}},
                                 {{0, 0}, {1, 0}, {1, 1}}};
    // 2x2 pixel image -> 3x3 contour edge vertices
    QSize imgSize(2, 2);
    mesh::ContourMap contourMap(imgSize, contours);
    REQUIRE(contourMap.getContourIndices({0,0})[0] == 2);
    REQUIRE(contourMap.getContourIndices({0,0})[1] == 0);
    REQUIRE(contourMap.getContourIndices({0,0})[2] == 1);
    REQUIRE(contourMap.isFixedPoint({0,0}) == true);
    REQUIRE(contourMap.getContourIndices({0,1})[0] == 0);
    REQUIRE(contourMap.getContourIndices({0,1})[1] == -1);
    REQUIRE(contourMap.getContourIndices({0,1})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({0,1}) == false);
    REQUIRE(contourMap.getContourIndices({0,2})[0] == 0);
    REQUIRE(contourMap.getContourIndices({0,2})[1] == -1);
    REQUIRE(contourMap.getContourIndices({0,2})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({0,2}) == false);
    REQUIRE(contourMap.getContourIndices({1,0})[0] == 2);
    REQUIRE(contourMap.getContourIndices({1,0})[1] == 1);
    REQUIRE(contourMap.getContourIndices({1,0})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({1,0}) == false);
    REQUIRE(contourMap.getContourIndices({1,1})[0] == 1);
    REQUIRE(contourMap.getContourIndices({1,1})[1] == -1);
    REQUIRE(contourMap.getContourIndices({1,1})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({1,1}) == false);
    REQUIRE(contourMap.getContourIndices({1,2})[0] == -1);
    REQUIRE(contourMap.getContourIndices({1,2})[1] == -1);
    REQUIRE(contourMap.getContourIndices({1,2})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({1,2}) == false);
    REQUIRE(contourMap.getContourIndices({2,0})[0] == 2);
    REQUIRE(contourMap.getContourIndices({2,0})[1] == -1);
    REQUIRE(contourMap.getContourIndices({2,0})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({2,0}) == false);
    REQUIRE(contourMap.getContourIndices({2,1})[0] == -1);
    REQUIRE(contourMap.getContourIndices({2,1})[1] == -1);
    REQUIRE(contourMap.getContourIndices({2,1})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({2,1}) == false);
    REQUIRE(contourMap.getContourIndices({2,2})[0] == -1);
    REQUIRE(contourMap.getContourIndices({2,2})[1] == -1);
    REQUIRE(contourMap.getContourIndices({2,2})[2] == -1);
    REQUIRE(contourMap.isFixedPoint({2,2}) == false);
  }
}
